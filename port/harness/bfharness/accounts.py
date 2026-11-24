"""Test accounts, characters, and scene set-up.

Two ways to set a scene up (spec 7: game-master commands for set-up only; the
step under test is always played through the client):

* **SQL, offline.** Before the client logs a character in: create an account,
  put an existing character on a map and cell, set its job, levels or zeny.
  The map server reads the row at login and overwrites it at every save, so
  `set_char` refuses a character that is online. Use it for the starting
  position of a script and anything a script must not spend play time on.

* **Atcommands, online.** While the character is in game: the client types
  `@warp`, `@jobchange`, `@zeny`, `@item`, `@monster` ... into its chat line,
  as a game master would (`GM.run`). The server logs each in `atcommandlog`,
  so the set-up is part of the evidence. Needs an account of group 99 (the
  seeded `bfadmin`, or `create_account(..., group_id=99)`).

New accounts: `create_account` inserts the row directly (immediately usable,
any group). `registration_userid` gives the `<name>_M` / `<name>_F` form the
login server turns into a new account on first login (new_account: yes in
port/server/import/login_conf.txt), which is what a player would do and what
the character-creation script uses when it must start from nothing.
"""

import os
import time

from .sql import SqlError

SEEDED = {"bftest": "bftest", "bfadmin": "bfadmin"}
NAME_MAX = 23           # NAME_LENGTH - 1 for both userid and character name
TEST_PREFIX = "bfh"     # accounts and characters the harness creates


def unique_name(prefix=TEST_PREFIX, tag=""):
    """A fresh name of at most 23 characters, letters and digits only."""
    stamp = _base36(int(time.time() * 100) % 36 ** 7) + _base36(os.getpid() % 1296).rjust(2, "0")
    return (prefix + tag + stamp)[:NAME_MAX]


def _base36(n):
    digits = "0123456789abcdefghijklmnopqrstuvwxyz"
    s = ""
    while True:
        n, r = divmod(n, 36)
        s = digits[r] + s
        if n == 0:
            return s


def registration_userid(userid, sex="F"):
    """The userid to type at login to register `userid` (4-21 characters)."""
    if sex not in ("M", "F"):
        raise ValueError("sex is M or F")
    if not 4 <= len(userid) <= NAME_MAX - 2:
        raise ValueError("userid must be 4-21 characters to register")
    return "%s_%s" % (userid, sex)


class Accounts:
    def __init__(self, sql, logs):
        self.sql = sql      # a Sql (real) backend for writes
        self.logs = logs

    def create_account(self, userid, password, sex="F", group_id=0, email="test@localhost"):
        """Insert a login row; returns its account_id. An existing account of
        that name is returned unchanged."""
        cur = self.logs.account(userid)
        if cur:
            return cur["account_id"]
        if len(userid) > NAME_MAX or len(password) > 32:
            raise ValueError("userid or password too long")
        mock = getattr(self.sql, "db", None)
        if mock is not None:  # MockSql
            aid = max([r["account_id"] for r in mock.rows("login")] or [2000000]) + 1
            mock.put("login", account_id=aid, userid=userid, user_pass=password, sex=sex,
                     group_id=int(group_id), logincount=0, lastlogin=None)
            return aid
        self.sql.execute(
            "INSERT INTO `login` (`userid`, `user_pass`, `sex`, `email`, `group_id`) "
            "VALUES (%s, %s, %s, %s, %d)" % tuple(
                [_q(userid), _q(password), _q(sex), _q(email), int(group_id)]))
        return self.logs.account(userid)["account_id"]

    def make_char(self, userid, name, slot=0, map="prontera", x=155, y=183):
        """Set-up: a Novice in `slot` of the account (made like the character
        server makes one: stats 5, hair 1, 42/11 HP/SP); returns its row. An
        existing character of that name is returned unchanged."""
        c = self.logs.char(name)
        if c:
            return c
        acc = self.logs.account(userid)
        if not acc:
            raise SqlError("no account %r" % userid)
        mock = getattr(self.sql, "db", None)
        if mock is not None:  # MockSql
            cid = max([r["char_id"] for r in mock.rows("char")] or [150000]) + 1
            mock.put("char", char_id=cid, account_id=acc["account_id"], char_num=int(slot), name=name, **{
                "class": 0, "base_level": 1, "job_level": 1, "zeny": 0, "last_map": map,
                "last_x": int(x), "last_y": int(y), "online": 0, "max_hp": 42, "hp": 42,
                "max_sp": 11, "sp": 11})
            return self.logs.char(name)
        self.sql.execute(
            "INSERT INTO `char` (`account_id`, `char_num`, `name`, `class`, `str`, `agi`, `vit`, `int`, "
            "`dex`, `luk`, `max_hp`, `hp`, `max_sp`, `sp`, `hair`, `hair_color`, `last_map`, `last_x`, "
            "`last_y`, `save_map`, `save_x`, `save_y`) VALUES (%d, %d, %s, 0, 5, 5, 5, 5, 5, 5, 42, 42, "
            "11, 11, 1, 0, %s, %d, %d, 'prontera', 155, 183)"
            % (acc["account_id"], int(slot), _q(name), _q(map), int(x), int(y)))
        return self.logs.char(name)

    def set_char(self, name, map=None, x=None, y=None, job=None, base_level=None,
                 job_level=None, zeny=None, save_point=False, hp=None, status_point=None):
        """Change an offline character's row before it logs in."""
        c = self.logs.char(name)
        if not c:
            raise SqlError("no character %r" % name)
        if c.get("online"):
            raise SqlError("character %r is online; use GM atcommands instead" % name)
        sets = []
        if map is not None:
            sets += ["`last_map` = %s" % _q(map), "`last_x` = %d" % int(x), "`last_y` = %d" % int(y)]
            if save_point:
                sets += ["`save_map` = %s" % _q(map), "`save_x` = %d" % int(x), "`save_y` = %d" % int(y)]
        for col, val in (("class", job), ("base_level", base_level),
                         ("job_level", job_level), ("zeny", zeny), ("hp", hp),
                         ("status_point", status_point)):
            if val is not None:
                sets.append("`%s` = %d" % (col, int(val)))
        mock = getattr(self.sql, "db", None)
        if mock is not None:  # MockSql
            changes = {"class" if k == "job" else k: v for k, v in
                       (("last_map", map), ("last_x", x), ("last_y", y), ("job", job),
                        ("base_level", base_level), ("job_level", job_level), ("zeny", zeny),
                        ("hp", hp), ("status_point", status_point))
                       if v is not None}
            mock.update("char", c["char_id"], **changes)
            return self.logs.char(name)
        if sets:
            self.sql.execute("UPDATE `char` SET %s WHERE `char_id` = %d AND `online` = 0"
                             % (", ".join(sets), c["char_id"]))
        return self.logs.char(name)


def _q(v):
    from .sql import quote
    return quote(v)


class GM:
    """Atcommands typed by the client into its chat line (online set-up).

    Each method returns the command string; `run` types it through a Control
    connection. The commands are rAthena's (doc/atcommands.txt)."""

    def __init__(self, control=None):
        self.ctl = control

    def run(self, command):
        if self.ctl is None:
            raise RuntimeError("GM.run needs a control connection")
        self.ctl.say(command)
        return command

    @staticmethod
    def warp(map, x, y):
        return "@warp %s %d %d" % (map, x, y)

    @staticmethod
    def jobchange(job):
        return "@jobchange %s" % job

    @staticmethod
    def base_level(n):
        return "@blvl %d" % n

    @staticmethod
    def job_level(n):
        return "@jlvl %d" % n

    @staticmethod
    def zeny(n):
        return "@zeny %d" % n

    @staticmethod
    def item(item, amount=1):
        return "@item %s %d" % (item, amount)

    @staticmethod
    def monster(mob, amount=1):
        return "@monster %s %d" % (mob, amount)

    @staticmethod
    def heal():
        return "@heal"

    @staticmethod
    def where():
        """The server replies with the map and cell it holds for the character."""
        return "@where"
