"""One check against the real test server, without a client: the seeded
account, every log table readable, and log rows found by time window and by
marker. Read-only. Skipped when Docker or the server is not available."""

import pathlib  # noqa: E402
import sys  # noqa: E402
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))  # port/harness

import shutil
import sys
import unittest

from bfharness.server import Server
from bfharness.sql import LOG_TABLES, Logs, Sql


def note(msg):
    sys.stderr.write("\n    [live] %s " % msg)


class LiveServer(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which("docker"):
            raise unittest.SkipTest("docker not installed")
        cls.server = Server()
        if not cls.server.online():
            raise unittest.SkipTest("test server not online (port/harness/run.py --server-status)")
        cls.sql = Sql(cls.server)
        cls.logs = Logs(cls.sql)

    def test_seeded_account(self):
        acc = self.logs.account("bftest")
        self.assertIsNotNone(acc)
        self.assertEqual(acc["account_id"], 2000101)
        self.assertEqual(acc["group_id"], 0)
        self.assertEqual(self.logs.account("bfadmin")["group_id"], 99)

    def test_log_tables_readable(self):
        counts = {}
        for table, (tcol, idc) in LOG_TABLES.items():
            rows = self.sql.query("SELECT COUNT(*) AS n, MAX(%s) AS last FROM `%s`" % ("`%s`" % tcol, table))
            counts[table] = rows[0]["n"]
        note("rows per log table: %s" % counts)
        self.assertEqual(set(counts), set(LOG_TABLES))

    def test_marker(self):
        m = self.logs.mark()
        self.assertRegex(m.time, r"^\d{4}-\d\d-\d\d \d\d:\d\d:\d\d$")
        self.assertEqual(set(m.ids), {t for t, (_, i) in LOG_TABLES.items() if i})
        # nothing in the picklog after a marker taken just now, unless the
        # server's own activity wrote a row in between; then its id is higher
        for r in self.logs.items(after=m):
            self.assertGreater(r["id"], m.ids["picklog"])

    def find_by_window(self, table):
        tcol, idc = LOG_TABLES[table]
        newest = self.sql.select(table, order="-" + (idc or tcol), limit=1)
        if not newest:
            note("%s: no rows yet (nothing has generated any); window search not exercised" % table)
            return None
        row = newest[0]
        t = row[tcol]
        found = self.sql.select(table, {tcol: (">=", t), idc: row[idc]} if idc else {tcol: (">=", t)})
        self.assertTrue(found, "%s row at %s found by its time window" % (table, t))
        before = self.sql.select(table, {tcol: ("<", t), idc: row[idc]})
        self.assertEqual(before, [], "and not before it")
        note("%s: newest row %s at %s found by time window" % (table, row[idc], t))
        return row

    def test_picklog_by_time_window(self):
        row = self.find_by_window("picklog")
        if row:
            from bfharness.sql import Marker
            m = Marker(row["time"], {"picklog": row["id"] - 1})
            self.assertIn(row["id"], [r["id"] for r in self.logs.items(char_id=row["char_id"], after=m)])

    def test_atcommandlog_by_time_window(self):
        self.find_by_window("atcommandlog")

    def test_loginlog_window(self):
        rows = self.sql.select("loginlog", {"user": "bftest", "rcode": 100}, order="-time", limit=1)
        if not rows:
            note("loginlog: no successful bftest login yet")
            return
        from bfharness.sql import Marker
        m = Marker(rows[0]["time"], {})
        self.assertTrue(self.logs.logins("bftest", after=m))
        note("loginlog: last bftest login %s found by time window" % rows[0]["time"])


if __name__ == "__main__":
    unittest.main()
