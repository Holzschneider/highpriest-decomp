"""SQL parsing and filters, frames and manifests, accounts, the restart guard."""

import pathlib  # noqa: E402
import sys  # noqa: E402
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))  # port/harness

import os
import unittest
from unittest import mock

from bfharness import paths
from bfharness.client import Client, ClientError
from bfharness.accounts import GM, Accounts, registration_userid, unique_name
from bfharness.frames import FrameError, Frames
from bfharness.mockdb import MockDb, seed
from bfharness.server import Server, ServerError
from bfharness.sql import Logs, MockSql, SqlError, parse_batch, quote, where_sql

RUN = paths.build_dir() / "selftest" / "units"


class SqlText(unittest.TestCase):
    def test_parse_batch(self):
        out = "id\tname\tmessage\tmap\n1\tBFProbe\ta\\tb\\nc\tprontera\n2\t1234\tNULL\tprt_fild08\n"
        rows = parse_batch(out)
        self.assertEqual(rows[0], {"id": 1, "name": "BFProbe", "message": "a\tb\nc", "map": "prontera"})
        self.assertEqual(rows[1]["name"], "1234")       # names stay text
        self.assertIsNone(rows[1]["message"])
        self.assertEqual(parse_batch(""), [])

    def test_quote_and_where(self):
        self.assertEqual(quote("it's \\"), "'it\\'s \\\\'")
        self.assertEqual(quote(None), "NULL")
        w = where_sql({"char_id": 5, "amount": (">", 0), "type": ("in", ["S", "N"]), "x": None})
        self.assertEqual(w, "`char_id` = 5 AND `amount` > 0 AND `type` IN ('S', 'N') AND `x` IS NULL")
        with self.assertRaises(SqlError):
            where_sql({"a; DROP": 1})


class MockLogs(unittest.TestCase):
    def setUp(self):
        RUN.mkdir(parents=True, exist_ok=True)
        self.path = RUN / "logs.jsonl"
        if self.path.exists():
            self.path.unlink()
        self.db = MockDb(self.path)
        seed(self.db)
        self.logs = Logs(MockSql(self.db))

    def test_marker_window(self):
        self.db.put("picklog", char_id=7, type="P", nameid=909, amount=1, map="prt_fild08")
        m = self.logs.mark()
        self.assertEqual(self.logs.picked_up(7, 909, after=m), [])
        self.db.put("picklog", char_id=7, type="P", nameid=909, amount=1, map="prt_fild08")
        self.db.put("picklog", char_id=7, type="P", nameid=909, amount=-1, map="prt_fild08")
        self.db.put("picklog", char_id=1002, type="M", nameid=909, amount=-1, map="prt_fild08")
        self.assertEqual(len(self.logs.picked_up(7, 909, after=m)), 1)
        self.assertEqual(len(self.logs.picked_up(7, 909)), 2)
        self.assertEqual(len(self.logs.monster_drops(1002, map="prt_fild08", after=m)), 1)

    def test_chat_and_commands(self):
        m = self.logs.mark()
        self.db.put("chatlog", type="O", src_charid=3, src_map="prontera", src_map_x=150, src_map_y=160,
                    message="hello world")
        self.db.put("atcommandlog", account_id=2000100, char_id=3, command="@warp prontera 150 150")
        self.assertEqual(self.logs.chat(3, contains="lo wo", after=m)[0]["src_map_x"], 150)
        self.assertEqual(self.logs.chat(3, contains="nothing", after=m), [])
        self.assertEqual(len(self.logs.atcommands(char_id=3, contains="@warp", after=m)), 1)

    def test_keyed_tables(self):
        self.assertEqual(self.logs.char("BFProbe")["last_x"], 155)
        self.db.update("char", 150000, last_x=160)
        self.assertEqual(self.logs.position("BFProbe"), ("prontera", 160, 183))
        self.assertEqual(len(self.logs.chars(2000101)), 1)

    def test_accounts(self):
        acc = Accounts(self.logs.db, self.logs)
        aid = acc.create_account("bfhunit", "pw", group_id=99)
        self.assertEqual(self.logs.account("bfhunit")["group_id"], 99)
        self.assertEqual(acc.create_account("bfhunit", "pw"), aid)
        c = acc.set_char("BFProbe", map="prt_fild08", x=170, y=372, job=1)
        self.assertEqual((c["last_map"], c["last_x"], c["class"]), ("prt_fild08", 170, 1))
        self.db.update("char", 150000, online=1)
        with self.assertRaises(SqlError):
            acc.set_char("BFProbe", zeny=5)

    def test_names(self):
        n = unique_name(tag="c")
        self.assertTrue(n.startswith("bfhc") and len(n) <= 23 and n.isalnum())
        self.assertEqual(registration_userid("bfhnew", "M"), "bfhnew_M")
        with self.assertRaises(ValueError):
            registration_userid("abc")
        self.assertEqual(GM.warp("prontera", 150, 150), "@warp prontera 150 150")


class FramesTest(unittest.TestCase):
    def test_guards(self):
        with self.assertRaises(FrameError):
            Frames("x", root=paths.REPO / "port")
        with self.assertRaises(FrameError):
            Frames("../escape", root=RUN)
        f = Frames("unit", root=RUN / "frames", allow_in_repo=True)
        with self.assertRaises(FrameError):
            f.write_manifest(RUN / "m.png")
        f.records.append({"name": "b", "sha256": "0" * 64, "width": 2, "height": 1})
        f.records.append({"name": "a", "sha256": "1" * 64, "width": 2, "height": 1})
        text = f.write_manifest(RUN / "m.sha256").read_text()
        self.assertEqual(text.splitlines()[1:], ["1" * 64 + "  2x1  a.png", "0" * 64 + "  2x1  b.png"])
        self.assertNotIn(os.path.expanduser("~"), text)

    def test_default_root_is_outside_git(self):
        self.assertFalse(paths.inside_repo(paths.frames_root()))
        self.assertEqual(paths.frames_root().parts[-3:], (paths.app_name(), "test", "frames"))


class ServerGuard(unittest.TestCase):
    def test_restart_refused_without_permission(self):
        s = Server()
        with mock.patch.dict(os.environ, {"BF_SERVER_RESTART": ""}), \
                mock.patch.object(Server, "compose", side_effect=AssertionError("must not run")):
            with self.assertRaises(ServerError):
                s.restart()

    def test_server_dir_is_main_checkout(self):
        with mock.patch.dict(os.environ, {"BF_SERVER_DIR": ""}):
            os.environ.pop("BF_SERVER_DIR")
            d = paths.server_dir()
        self.assertTrue((d / "docker-compose.yml").exists(), d)

    def test_socket_path_fits(self):
        self.assertLessEqual(len(str(paths.socket_path("ctl"))), paths.SOCKET_PATH_MAX)


class GameFolderCheck(unittest.TestCase):
    """The trace's game folder against the one the harness stored or passed."""

    def client(self, tmp, line, folder):
        c = Client(tmp, command=["true"])
        c.game_folder = pathlib.Path(folder)
        c.trace_file.write_text("[       0 file ] file: game folder %s (setting)\n"
                                "[       2 mode ] start: /x/bifrost, command line \"\", game folder %s\n"
                                % (line, line))
        return c

    def test_same_folder_passes(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            c = self.client(tmp, "/a/b c/game", "/a/b c/game/")
            self.assertEqual(c.check_game_folder(), "/a/b c/game")

    def test_fallback_to_default_fails(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            c = self.client(tmp, str(pathlib.Path.home() / "Games" / paths.app_name()), "/long/" + "L" * 1100)
            with self.assertRaises(ClientError):
                c.check_game_folder()

    def test_no_start_line_is_not_checked(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            c = Client(tmp, command=["true"])
            c.game_folder = pathlib.Path("/a")
            self.assertIsNone(c.check_game_folder())


if __name__ == "__main__":
    unittest.main()
