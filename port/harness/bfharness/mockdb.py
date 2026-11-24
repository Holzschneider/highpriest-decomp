"""The mock client's stand-in for the server's database: rows appended to a
JSON-lines file, one {"table": ..., "row": ...} per line. The mock client
writes what the server would log; MockSql reads it with the same readers.

Log tables are append-only and get ids as the server's AUTO_INCREMENT would.
`login`, `char`, `inventory` and `mail` hold current rows: a later row with the same
key replaces the earlier one.
"""

import datetime
import json
import os
import threading

from .sql import LOG_TABLES

KEYED = {"login": "account_id", "char": "char_id", "inventory": "id", "mail": "id"}


def utcnow():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d %H:%M:%S")


class MockDb:
    def __init__(self, path):
        self.path = str(path)
        self.lock = threading.Lock()

    def now(self):
        return utcnow()

    def _read(self):
        if not os.path.exists(self.path):
            return []
        with open(self.path, encoding="utf-8") as f:
            out = []
            for line in f:
                try:
                    out.append(json.loads(line))
                except ValueError:
                    pass  # a line still being written
            return out

    def rows(self, table):
        with self.lock:
            entries = [e["row"] for e in self._read() if e["table"] == table]
        key = KEYED.get(table)
        if not key:
            return entries
        cur = {}
        for r in entries:
            if r.get("_deleted"):
                cur.pop(r[key], None)
            else:
                cur[r[key]] = r
        return list(cur.values())

    def put(self, table, **row):
        """Append a row; a log table's id and time column are filled in."""
        with self.lock:
            existing = [e["row"] for e in self._read() if e["table"] == table]
            if table in LOG_TABLES:
                tcol, idc = LOG_TABLES[table]
                row.setdefault(tcol, utcnow())
                if idc:
                    row[idc] = max([r.get(idc, 0) for r in existing] or [0]) + 1
            with open(self.path, "a", encoding="utf-8") as f:
                f.write(json.dumps({"table": table, "row": row}) + "\n")
        return row

    def update(self, table, key_value, **changes):
        key = KEYED[table]
        cur = [r for r in self.rows(table) if r[key] == key_value]
        row = dict(cur[0]) if cur else {key: key_value}
        row.update(changes)
        return self.put(table, **row)


def seed(db):
    """The test server's seeded accounts (port/server/sql/seed_accounts.sql) and
    the character bifrost-netprobe made, so the mock starts where the server is."""
    if db.rows("login"):
        return
    db.put("login", account_id=2000100, userid="bfadmin", user_pass="bfadmin", sex="M",
           group_id=99, logincount=0, lastlogin=None)
    db.put("login", account_id=2000101, userid="bftest", user_pass="bftest", sex="F",
           group_id=0, logincount=0, lastlogin=None)
    db.put("char", char_id=150000, account_id=2000101, char_num=0, name="BFProbe", **{
        "class": 0, "base_level": 1, "job_level": 1, "zeny": 0, "last_map": "prontera",
        "last_x": 155, "last_y": 183, "online": 0, "max_hp": 40, "hp": 40, "max_sp": 11, "sp": 11})
