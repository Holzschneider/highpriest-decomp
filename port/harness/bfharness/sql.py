"""The test server's database and the rAthena log tables.

Two backends with one interface (`select`, `execute`, `now`, `max_id`):
`Sql` talks to MariaDB through `docker compose exec -T db mariadb --batch`;
`MockSql` reads the rows the mock client writes (mockdb.py). The readers in
`Logs` are written against that interface, so a script reads its evidence the
same way against the real server and against the mock.

Time windows: a `Marker` taken before an action holds the database's own clock
(log rows are stamped with the database's NOW()) and the highest row id of
every log table that has one. Rows "after the marker" are rows with a higher
id, which is exact; for `loginlog`, which has no id, it is time >= marker time
(one-second granularity).
"""

import re
import time


# table: (time column, id column)
LOG_TABLES = {
    "picklog": ("time", "id"),
    "zenylog": ("time", "id"),
    "chatlog": ("time", "id"),
    "atcommandlog": ("atcommand_date", "atcommand_id"),
    "npclog": ("npc_date", "npc_id"),
    "branchlog": ("branch_date", "branch_id"),
    "mvplog": ("mvp_date", "mvp_id"),
    "loginlog": ("time", None),
    "charlog": ("time", "id"),
}

# Columns that hold text even when the value looks like a number.
TEXT_COLUMNS = {
    "userid", "user_pass", "email", "name", "char_name", "dst_charname", "message",
    "command", "mes", "map", "src_map", "last_map", "save_map", "user", "log", "ip",
    "last_ip", "char_msg", "pincode", "web_auth_token", "type", "sex", "time",
    "atcommand_date", "npc_date", "branch_date", "mvp_date", "lastlogin", "birthdate",
}

PICK_TYPES = {
    "P": "picked up or dropped by a player", "M": "dropped by a monster", "L": "looted by a monster",
    "S": "NPC shop", "N": "NPC script", "T": "trade", "V": "vending", "A": "GM command",
    "C": "consumed", "R": "storage", "D": "stolen", "O": "produced", "U": "MVP reward",
}
CHAT_TYPES = {"O": "global", "W": "whisper", "P": "party", "G": "guild", "M": "main", "C": "clan"}


class SqlError(RuntimeError):
    pass


def quote(v):
    if v is None:
        return "NULL"
    if isinstance(v, bool):
        return "1" if v else "0"
    if isinstance(v, (int, float)):
        return str(v)
    s = str(v).replace("\\", "\\\\").replace("'", "\\'").replace("\0", "\\0")
    return "'" + s + "'"


def ident(name):
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
        raise SqlError("bad identifier %r" % name)
    return "`" + name + "`"


_UNESCAPE = {"n": "\n", "t": "\t", "\\": "\\", "0": "\0", "r": "\r"}


def _unescape(s):
    return re.sub(r"\\(.)", lambda m: _UNESCAPE.get(m.group(1), m.group(1)), s)


def _value(col, raw):
    if raw == "NULL":
        return None
    v = _unescape(raw)
    if col not in TEXT_COLUMNS and re.fullmatch(r"-?\d+", v):
        return int(v)
    return v


def parse_batch(text):
    """Parse `mariadb --batch` output (tab-separated, header line) into dicts."""
    lines = text.split("\n")
    while lines and lines[-1] == "":
        lines.pop()
    if not lines:
        return []
    cols = lines[0].split("\t")
    rows = []
    for line in lines[1:]:
        vals = line.split("\t")
        rows.append({c: _value(c, v) for c, v in zip(cols, vals)})
    return rows


def where_sql(where):
    """{col: value} for equality, {col: (op, value)} with op one of
    = != < <= > >= like in."""
    parts = []
    for col, cond in (where or {}).items():
        if isinstance(cond, tuple):
            op, val = cond
        else:
            op, val = "=", cond
        op = op.lower()
        if op == "in":
            parts.append("%s IN (%s)" % (ident(col), ", ".join(quote(x) for x in val)))
        elif val is None and op in ("=", "!="):
            parts.append("%s IS %sNULL" % (ident(col), "NOT " if op == "!=" else ""))
        elif op in ("=", "!=", "<", "<=", ">", ">=", "like"):
            parts.append("%s %s %s" % (ident(col), op.upper(), quote(val)))
        else:
            raise SqlError("bad operator %r" % op)
    return " AND ".join(parts)


class Marker:
    """A point in the logs: the database clock and the last id per log table."""

    def __init__(self, time, ids):
        self.time = time
        self.ids = ids

    def to_json(self):
        return {"time": self.time, "ids": self.ids}

    def __repr__(self):
        return "Marker(%s)" % self.time


class _Backend:
    def select(self, table, where=None, order=None, limit=None):
        raise NotImplementedError

    def execute(self, statement):
        raise NotImplementedError

    def now(self):
        raise NotImplementedError

    def max_id(self, table, column):
        raise NotImplementedError

    def mark(self):
        ids = {t: self.max_id(t, idc) for t, (_, idc) in LOG_TABLES.items() if idc}
        return Marker(self.now(), ids)

    def after(self, table, marker):
        """The where-clause part selecting rows of a log table after a marker."""
        if marker is None:
            return {}
        tcol, idc = LOG_TABLES[table]
        if idc:
            return {idc: (">", marker.ids.get(table, 0))}
        return {tcol: (">=", marker.time)}


class Sql(_Backend):
    """MariaDB of the test server, through docker compose."""

    def __init__(self, server=None, user="ragnarok", password="ragnarok", database="ragnarok"):
        if server is None:
            from .server import Server
            server = Server()
        self.server = server
        self.user, self.password, self.database = user, password, database
        self.log = []  # every statement, for the report

    def run(self, statement, timeout=60):
        self.log.append(statement)
        r = self.server.compose("exec", "-T", "db", "mariadb", "-u" + self.user,
                                "-p" + self.password, self.database, "--batch",
                                input=statement, timeout=timeout, check=False)
        if r.returncode != 0:
            raise SqlError("%s\n-> %s" % (statement, r.stderr.strip()))
        return r.stdout

    def query(self, statement):
        return parse_batch(self.run(statement))

    def execute(self, statement):
        self.run(statement)

    def select(self, table, where=None, order=None, limit=None):
        s = "SELECT * FROM %s" % ident(table)
        w = where_sql(where)
        if w:
            s += " WHERE " + w
        if order:
            desc = order.startswith("-")
            s += " ORDER BY %s%s" % (ident(order.lstrip("-")), " DESC" if desc else "")
        if limit:
            s += " LIMIT %d" % int(limit)
        return self.query(s)

    def now(self):
        return self.query("SELECT DATE_FORMAT(NOW(), '%Y-%m-%d %H:%i:%s') AS now")[0]["now"]

    def max_id(self, table, column):
        r = self.query("SELECT COALESCE(MAX(%s), 0) AS m FROM %s" % (ident(column), ident(table)))
        return int(r[0]["m"])

    def mark(self):
        # one round trip instead of one per table
        parts = ["DATE_FORMAT(NOW(), '%Y-%m-%d %H:%i:%s') AS now"]
        for t, (_, idc) in LOG_TABLES.items():
            if idc:
                parts.append("(SELECT COALESCE(MAX(%s), 0) FROM %s) AS %s" % (ident(idc), ident(t), ident(t)))
        row = self.query("SELECT " + ", ".join(parts))[0]
        return Marker(row.pop("now"), {k: int(v) for k, v in row.items()})


class MockSql(_Backend):
    """The mock client's rows (mockdb.MockDb), behind the same interface."""

    def __init__(self, db):
        self.db = db
        self.log = []

    def select(self, table, where=None, order=None, limit=None):
        self.log.append("select %s %r" % (table, where))
        rows = [r for r in self.db.rows(table) if _matches(r, where or {})]
        if order:
            key = order.lstrip("-")
            rows.sort(key=lambda r: (r.get(key) is None, r.get(key)), reverse=order.startswith("-"))
        return rows[:limit] if limit else rows

    def execute(self, statement):
        raise SqlError("the mock database takes no SQL; use MockDb.put")

    def now(self):
        return self.db.now()

    def max_id(self, table, column):
        return max([r.get(column) or 0 for r in self.db.rows(table)] or [0])


def _like(pattern, value):
    rx = "".join(".*" if c == "%" else "." if c == "_" else re.escape(c) for c in pattern)
    return re.fullmatch(rx, str(value), re.S | re.I) is not None


def _matches(row, where):
    for col, cond in where.items():
        op, val = cond if isinstance(cond, tuple) else ("=", cond)
        v = row.get(col)
        op = op.lower()
        if op == "=" and v != val:
            return False
        if op == "!=" and v == val:
            return False
        if op == "in" and v not in val:
            return False
        if op == "like" and not _like(val, v):
            return False
        if op in ("<", "<=", ">", ">="):
            if v is None:
                return False
            if not {"<": v < val, "<=": v <= val, ">": v > val, ">=": v >= val}[op]:
                return False
    return True


class Logs:
    """Readers for the evidence the M5-M8 scripts use."""

    def __init__(self, backend):
        self.db = backend

    def mark(self):
        return self.db.mark()

    def _log(self, table, where, after, order=None):
        w = dict(where)
        w.update(self.db.after(table, after))
        return self.db.select(table, w, order=order or LOG_TABLES[table][1] or LOG_TABLES[table][0])

    # accounts and characters (current rows, not logs)

    def account(self, userid):
        r = self.db.select("login", {"userid": userid})
        return r[0] if r else None

    def char(self, name):
        r = self.db.select("char", {"name": name})
        return r[0] if r else None

    def chars(self, account_id):
        return self.db.select("char", {"account_id": account_id}, order="char_num")

    def position(self, name):
        """(map, x, y) as last saved: on logout, map-server change, or autosave."""
        c = self.char(name)
        return (c["last_map"], c["last_x"], c["last_y"]) if c else None

    def inventory(self, char_id, nameid=None):
        w = {"char_id": char_id}
        if nameid is not None:
            w["nameid"] = nameid
        return self.db.select("inventory", w)

    # log tables

    def logins(self, userid, after=None, ok=True):
        w = {"user": userid}
        if ok:
            w["rcode"] = 100
        return self._log("loginlog", w, after, order="time")

    def charlog(self, account_id=None, name=None, msg=None, after=None):
        w = {}
        if account_id is not None:
            w["account_id"] = account_id
        if name is not None:
            w["name"] = name
        if msg is not None:
            w["char_msg"] = msg
        return self._log("charlog", w, after)

    def items(self, char_id=None, nameid=None, type=None, after=None, map=None, sign=None):
        """picklog rows. sign=+1 for gains, -1 for losses."""
        w = {}
        if char_id is not None:
            w["char_id"] = char_id
        if nameid is not None:
            w["nameid"] = nameid
        if type is not None:
            w["type"] = type
        if map is not None:
            w["map"] = map
        if sign:
            w["amount"] = (">", 0) if sign > 0 else ("<", 0)
        return self._log("picklog", w, after)

    def picked_up(self, char_id, nameid=None, after=None):
        """Item nameid picked up from the ground by char_id after the marker."""
        return self.items(char_id, nameid, "P", after, sign=+1)

    def monster_drops(self, mob_id=None, nameid=None, map=None, after=None):
        """Monster deaths that dropped something: picklog type M, char_id is the mob id."""
        return self.items(mob_id, nameid, "M", after, map=map, sign=-1)

    def bought(self, char_id, nameid=None, after=None):
        return self.items(char_id, nameid, "S", after, sign=+1)

    def sold(self, char_id, nameid=None, after=None):
        return self.items(char_id, nameid, "S", after, sign=-1)

    def zeny(self, char_id, type=None, after=None):
        w = {"char_id": char_id}
        if type is not None:
            w["type"] = type
        return self._log("zenylog", w, after)

    def chat(self, char_id=None, contains=None, type=None, after=None):
        """chatlog rows; src_map, src_map_x, src_map_y are the server's own
        position of the speaker at that moment."""
        w = {}
        if char_id is not None:
            w["src_charid"] = char_id
        if type is not None:
            w["type"] = type
        if contains is not None:
            w["message"] = ("like", "%" + contains + "%")
        return self._log("chatlog", w, after)

    def atcommands(self, char_id=None, account_id=None, contains=None, after=None):
        w = {}
        if char_id is not None:
            w["char_id"] = char_id
        if account_id is not None:
            w["account_id"] = account_id
        if contains is not None:
            w["command"] = ("like", "%" + contains + "%")
        return self._log("atcommandlog", w, after)

    def npc(self, char_id=None, after=None):
        return self._log("npclog", {} if char_id is None else {"char_id": char_id}, after)

    def branches(self, char_id=None, after=None):
        return self._log("branchlog", {} if char_id is None else {"char_id": char_id}, after)

    def mvp(self, char_id=None, after=None):
        return self._log("mvplog", {} if char_id is None else {"kill_char_id": char_id}, after)


def wait_rows(fn, timeout=10.0, interval=0.5):
    """Poll fn() until it returns a non-empty result; return it ([] on timeout).
    Log rows are written synchronously, but character saves pass through the
    char server and arrive a moment later."""
    deadline = time.monotonic() + timeout
    while True:
        r = fn()
        if r or time.monotonic() >= deadline:
            return r
        time.sleep(interval)
