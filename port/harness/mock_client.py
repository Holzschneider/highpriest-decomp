#!/usr/bin/env python3
"""A mock of the client's control socket, for the harness's own tests.

Serves port/native/control/PROTOCOL.md over the socket named by --control or
BF_CONTROL, with a fake game behind it: login -> charselect (-> makechar) ->
loading -> game, a character that walks where the ground is clicked, NPCs, a
shop, a Job Master, Porings that drop Jellopy, chat and a few atcommands.
What the server would log goes to the mock database (BF_MOCK_DB, see
bfharness/mockdb.py) in the shape of rAthena's tables, so the scripts read the
same evidence from the mock as from the real server.

It is a test double, not a model of the client: picking is a flat grid of
16 logical pixels per cell centred on the own character, everything happens
in a few logic steps, and captures are synthetic PNGs (no game artwork).
"""

import json
import os
import selectors
import signal
import socket
import struct
import sys
import time
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from bfharness import PROTOCOL_VERSION, paths  # noqa: E402
from bfharness.mockdb import MockDb, seed       # noqa: E402

STEP = 1.0 / 60
CELL = 16
WALK_STEPS = 6              # logic steps per cell
KEYS = set("abcdefghijklmnopqrstuvwxyz0123456789") | {"f%d" % i for i in range(1, 13)} | {
    "escape", "enter", "tab", "space", "backspace", "delete", "insert", "home", "end",
    "pageup", "pagedown", "left", "right", "up", "down", "minus", "equal", "leftbracket",
    "rightbracket", "backslash", "semicolon", "quote", "comma", "period", "slash", "grave",
    "kpenter", "kpplus", "kpminus", "kpmultiply", "kpdivide", "kpdecimal",
    "shift", "ctrl", "alt", "cmd"} | {"kp%d" % i for i in range(10)}
MODS = {"shift", "ctrl", "alt", "cmd"}
WID = {"BASICINFOWND": 0, "CHATWND": 1, "LOGINWND": 3, "MAKECHARWND": 4, "SELECTCHARWND": 5,
       "WAITWND": 6, "LOADINGWND": 7, "SAYDIALOGWND": 16, "CHOOSEWND": 17, "RESTARTWND": 19,
       "NOTICECONFIRMWND": 20, "ITEMSHOPWND": 22, "ITEMSELLWND": 24, "CHOOSESELLBUYWND": 25,
       "ITEMWND": 8, "EQUIPWND": 10, "STATUSWND": 11, "OPTIONWND": 13, "SKILLLISTWND": 37,
       "PARTYSETTINGWND": 53, "WHISPERWND": -1,
       "MAILLISTWND": 98, "MAILVIEWWND": 99}
COMMANDS = ["hello", "key", "pointer", "capture", "state", "wait", "resize", "quit"]
EVENTS = ["mode", "map", "chat", "window", "sound", "bye", "error"]
RESERVED = {"step", "pause", "resume", "render", "subscribe", "trace", "config", "act"}
DETAIL = {"login": "LSM_IDENTRY", "connecting": "LSM_WAITRESP_FROM_ACSVR",
          "charselect": "LSM_SELECTCHAR", "makechar": "LSM_MAKECHAR",
          "loading": "LSM_READY_FOR_ZONESVR", "game": "MT_GAME", "quitting": "MT_QUIT"}
ITEMS = {501: ("Red Potion", 50), 502: ("Orange Potion", 200), 909: ("Jellopy", 6)}
JOBS = {"Swordman": 1, "Mage": 2, "Archer": 3, "Acolyte": 4, "Merchant": 5, "Thief": 6}
MAPS = {"prontera": (312, 392), "prt_fild08": (400, 400), "prt_in": (180, 180), "izlude": (256, 256)}
NPCS = [  # map, x, y, name, sprite, role
    ("prontera", 153, 193, "Job Master", 123, "jobmaster"),
    ("prontera", 156, 187, "Kafra Employee", 113, "talk"),
    ("prontera", 156, 22, "Gate Guard", 105, "talk"),
    ("prt_in", 126, 76, "Tool Dealer", 53, "shop"),
    ("prontera", 146, 86, "Mailbox", 888, "mailbox"),   # npc/other/mail.txt
]
MAILBOX_FEE = 130
PORINGS = [("prt_fild08", 170, 370), ("prt_fild08", 176, 366), ("prt_fild08", 165, 362), ("prt_fild08", 170, 360)]
PORTALS = [("prontera", 156, 18, "prt_fild08", 170, 375), ("prt_fild08", 170, 378, "prontera", 156, 22),
           ("prontera", 179, 184, "prt_in", 60, 73), ("prt_in", 60, 77, "prontera", 175, 188)]
# Sound (the `sound` part): the music per map as mp3nametable.txt has it, and a
# stream that loops every MOCK_MUSIC_SECONDS (the mock's files are short).
MUSIC = {"login": "bgm\\01.mp3", "prontera": "bgm\\08.mp3", "prt_in": "bgm\\08.mp3",
         "prt_fild08": "bgm\\12.mp3", "izlude": "bgm\\26.mp3"}
MOCK_MUSIC_SECONDS = 4.0
BUTTON_WAV = "버튼소리.wav"   # "button sound", as UIFrameWnd.cpp names it
ALT_WINDOWS = {"e": "ITEMWND", "q": "EQUIPWND", "s": "SKILLLISTWND", "a": "STATUSWND", "o": "OPTIONWND"}


class ProtocolError(Exception):
    def __init__(self, code, message=""):
        Exception.__init__(self, message)
        self.code = code
        self.message = message


def cp1252_ok(text):
    try:
        text.encode("cp1252")
        return True
    except UnicodeEncodeError:
        return False


def write_png(path, width, height, rects, background):
    """An RGBA PNG: a background and filled rectangles (device pixels)."""
    row = bytearray(bytes(background) * width)
    rows = [bytearray(row) for _ in range(height)]
    for (x, y, w, h, color) in rects:
        x0, y0 = max(0, x), max(0, y)
        x1, y1 = min(width, x + w), min(height, y + h)
        if x1 <= x0 or y1 <= y0:
            continue
        span = bytes(color) * (x1 - x0)
        for yy in range(y0, y1):
            rows[yy][x0 * 4:x1 * 4] = span
    raw = b"".join(b"\0" + bytes(r) for r in rows)

    def chunk(kind, data):
        c = kind + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xffffffff)

    data = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 1)) + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(data)
    return data


class Window:
    def __init__(self, name, rect, controls=(), data=None):
        self.name = name
        self.id = WID[name]
        self.rect = list(rect)
        self.controls = [dict(c) for c in controls]
        self.data = data or {}
        self.focus_control = next((c["id"] for c in self.controls if c["kind"] == "edit"), None)

    def control(self, cid):
        return next((c for c in self.controls if c["id"] == cid), None)

    def hit(self, x, y):
        for c in self.controls:
            cx, cy, cw, ch = c["rect"]
            if cx <= x < cx + cw and cy <= y < cy + ch:
                return c
        return None

    def to_json(self, focused):
        out = []
        for c in self.controls:
            d = {k: c[k] for k in ("id", "kind", "rect", "text")}
            if c["kind"] == "list":
                d["items"] = list(c.get("items", []))
            if c.get("password"):
                d["text"] = "*" * len(c["text"])
            d["focus"] = focused and c["id"] == self.focus_control
            out.append(d)
        return {"id": self.id, "name": self.name, "rect": self.rect, "focus": focused, "controls": out}


def ctl(cid, kind, x, y, w, h, text="", **extra):
    d = {"id": cid, "kind": kind, "rect": [x, y, w, h], "text": text}
    d.update(extra)
    return d


class Conn:
    def __init__(self, sock):
        self.sock = sock
        self.buf = b""
        self.inbox = []
        self.seq = 0
        self.events = []
        self.reply_seq = 0
        self.pending = None
        self.out = b""


class Mock:
    def __init__(self, path, db, trace):
        self.path = path
        self.db = db
        self.trace_file = open(trace, "a", encoding="utf-8") if trace else None
        self.t0 = time.monotonic()
        self.frame = 0
        self.running = True
        self.conn = None
        self.sel = selectors.DefaultSelector()
        self.width, self.height, self.scale, self.fullscreen = 1024, 768, 1, False
        self.pointer = [0, 0]
        self.buttons = set()
        self.inputs = []          # one per logic step: callables
        self.input_token = 0
        self.consumed = 0
        self.mode = "start"
        self.mode_timer = 0
        self.windows = []
        self.account = None
        self.self_ = None
        self.map = None
        self.actors = []
        self.next_actor = 110000000
        self.chat_line = ""
        self.attack = None
        self.pick = None
        self.talk = None
        self.cart = {}
        self.selected_slot = 0
        self.last_click = (-1, 0, 0)
        self.after_connect = None
        self.bye_reason = "quit"
        self.stream = {"file": "", "started": None, "opened": 0}
        self.voices = []          # the `sound` part's recent voices
        self.voice_serial = 0
        self.voices_started = 0
        self.seed()
        self.set_mode("login")

    # -- infrastructure ------------------------------------------------------

    def now_ms(self):
        return int((time.monotonic() - self.t0) * 1000)

    def trace(self, cat, msg):
        if self.trace_file:
            self.trace_file.write("%8d [%s] %s\n" % (self.now_ms(), cat, msg))
            self.trace_file.flush()

    def logical(self):
        return self.width // self.scale, self.height // self.scale

    def emit(self, _event, **fields):
        name = _event
        self.trace("ctl", "event %s %s" % (name, json.dumps(fields)))
        c = self.conn
        if not c:
            return
        c.seq += 1
        msg = {"event": name, "seq": c.seq, "t": self.now_ms()}
        msg.update(fields)
        c.events.append(msg)
        self.send(c, msg)

    def send(self, c, msg):
        c.out += (json.dumps(msg) + "\n").encode("utf-8")
        try:
            n = c.sock.send(c.out)
            c.out = c.out[n:]
        except BlockingIOError:
            pass
        except OSError:
            self.drop(c)

    def reply(self, c, rid, /, **result):
        msg = {"id": rid, "ok": True, "seq": c.seq}
        msg.update(result)
        c.reply_seq = c.seq
        self.send(c, msg)

    def error(self, c, rid, code, message="", /, **extra):
        msg = {"id": rid, "ok": False, "seq": c.seq, "error": {"code": code, "message": message}}
        msg.update(extra)
        c.reply_seq = c.seq
        self.send(c, msg)

    def drop(self, c):
        if self.conn is c:
            self.conn = None
        try:
            self.sel.unregister(c.sock)
        except (KeyError, ValueError):
            pass
        c.sock.close()

    # -- mock database -------------------------------------------------------

    def seed(self):
        seed(self.db)

    def account_rows(self):
        return {r["userid"]: r for r in self.db.rows("login")}

    # -- modes and windows ---------------------------------------------------

    def set_mode(self, mode, timer=0):
        if mode == self.mode:
            return
        prev, self.mode = self.mode, mode
        self.mode_timer = timer
        self.trace("mode", "%s -> %s" % (prev, mode))
        self.emit("mode", mode=mode, previous=prev, detail=DETAIL.get(mode, mode))
        for w in list(self.windows):
            self.close_window(w.name)
        L = self.logical()
        cx, cy = L[0] // 2, L[1] // 2
        if mode == "login":
            self.play_stream(MUSIC["login"])
            self.open_window(Window("LOGINWND", [cx - 140, cy - 60, 280, 120], [
                ctl("id_edit", "edit", cx - 40, cy - 30, 130, 16),
                ctl("pw_edit", "edit", cx - 40, cy - 8, 130, 16, password=True),
                ctl("ok", "button", cx + 40, cy + 30, 40, 20, "Login"),
                ctl("cancel", "button", cx + 90, cy + 30, 40, 20, "Exit")]))
        elif mode in ("connecting", "loading"):
            self.open_window(Window("WAITWND" if mode == "connecting" else "LOADINGWND",
                                    [cx - 100, cy - 20, 200, 40]))
        elif mode == "charselect":
            slots = []
            chars = self.chars()
            for i in range(3):
                c = chars.get(i)
                slots.append(ctl("slot:%d" % i, "button", cx - 270 + i * 180, cy - 150, 160, 220,
                                 c["name"] if c else ""))
            self.open_window(Window("SELECTCHARWND", [cx - 290, cy - 180, 580, 360], slots + [
                ctl("ok", "button", cx + 150, cy + 150, 60, 20, "OK"),
                ctl("make", "button", cx + 80, cy + 150, 60, 20, "Make"),
                ctl("cancel", "button", cx + 220, cy + 150, 60, 20, "Cancel")]))
        elif mode == "makechar":
            self.open_window(Window("MAKECHARWND", [cx - 290, cy - 180, 580, 360], [
                ctl("name_edit", "edit", cx - 200, cy + 100, 140, 16),
                ctl("ok", "button", cx + 150, cy + 150, 60, 20, "OK"),
                ctl("cancel", "button", cx + 220, cy + 150, 60, 20, "Cancel")]))
        elif mode == "game":
            self.open_window(Window("BASICINFOWND", [0, 0, 220, 120]))
            self.open_window(Window("CHATWND", [0, L[1] - 120, 400, 120],
                                    [ctl("chat_edit", "edit", 4, L[1] - 20, 392, 16),
                                     ctl("whisper_edit", "edit", 4, L[1] - 40, 90, 16)]))
            self.window("CHATWND").focus_control = "chat_edit"

    def open_window(self, w):
        self.windows.insert(0, w)
        self.emit("window", action="open", wid=w.id, name=w.name)
        return w

    def close_window(self, name):
        for w in self.windows:
            if w.name == name:
                self.windows.remove(w)
                self.emit("window", action="close", wid=w.id, name=w.name)
                return True
        return False

    def window(self, name):
        return next((w for w in self.windows if w.name == name), None)

    def chars(self):
        if not self.account:
            return {}
        return {c["char_num"]: c for c in self.db.rows("char") if c["account_id"] == self.account["account_id"]}

    def system(self, text, channel="system"):
        self.emit("chat", channel=channel, **{"from": None, "text": text})

    # -- input ---------------------------------------------------------------

    def on_text(self, ch):
        w = self.windows[0] if self.windows else None
        if self.mode == "game" and (not w or w.name in ("CHATWND", "BASICINFOWND")) and \
                self.window("CHATWND").focus_control == "whisper_edit":
            c = self.window("CHATWND").control("whisper_edit")   # the whisper name field (M8.1b)
            if len(c["text"]) < 23:
                c["text"] += ch
            return
        if self.mode == "game" and (not w or w.name in ("CHATWND", "BASICINFOWND")):
            self.chat_line += ch
            self.window("CHATWND").control("chat_edit")["text"] = self.chat_line
            return
        if w and w.focus_control:
            c = w.control(w.focus_control)
            if len(c["text"]) < c.get("limit", 23):
                c["text"] += ch

    def on_key(self, key, down, mods):
        if not down:
            return
        if key == "q" and "cmd" in mods:
            self.trace("input", "cmd+q")
            self.running = False
            return
        w = self.windows[0] if self.windows else None
        if key == "tab" and w:
            edits = [c["id"] for c in w.controls if c["kind"] == "edit"]
            if edits:
                i = edits.index(w.focus_control) if w.focus_control in edits else -1
                w.focus_control = edits[(i + 1) % len(edits)]
        elif key == "backspace" and self.mode == "game" and not self.talk and \
                self.window("CHATWND").focus_control == "whisper_edit":
            c = self.window("CHATWND").control("whisper_edit")
            c["text"] = c["text"][:-1]
        elif key == "backspace":
            if self.mode == "game" and not self.talk:
                self.chat_line = self.chat_line[:-1]
                self.window("CHATWND").control("chat_edit")["text"] = self.chat_line
            elif w and w.focus_control:
                c = w.control(w.focus_control)
                c["text"] = c["text"][:-1]
        elif key in ("enter", "kpenter"):
            self.on_enter()
        elif key == "escape":
            self.on_escape()
        elif key in ("left", "right") and self.mode == "charselect":
            self.selected_slot = (self.selected_slot + (1 if key == "right" else -1)) % 3
        elif "alt" in mods and self.mode == "game" and key in ALT_WINDOWS:
            # The basic window's shortcuts toggle a window (UIWindowMgr::ProcessPushButton).
            name = ALT_WINDOWS[key]
            if not self.close_window(name):
                self.open_window(Window(name, [300, 100, 280, 200]))

    def on_enter(self):
        if self.mode == "login":
            self.submit_login()
        elif self.mode == "charselect":
            self.select_char()
        elif self.mode == "makechar":
            self.make_char()
        elif self.mode == "game":
            if self.window("SAYDIALOGWND"):
                self.close_window("SAYDIALOGWND")
                self.talk = None
                return
            line, self.chat_line = self.chat_line, ""
            self.window("CHATWND").control("chat_edit")["text"] = ""
            whisper = self.window("CHATWND").control("whisper_edit")["text"]
            if line and whisper and not line.startswith("/"):
                self.whisper(whisper, line)
            elif line:
                self.say(line)

    def on_escape(self):
        if self.mode == "game":
            for name in ("CHOOSEWND", "SAYDIALOGWND", "ITEMSHOPWND", "ITEMSELLWND",
                         "CHOOSESELLBUYWND", "RESTARTWND"):
                if self.close_window(name):
                    self.talk = None
                    return
            L = self.logical()
            self.open_window(Window("RESTARTWND", [L[0] // 2 - 80, L[1] // 2 - 40, 160, 80], [
                ctl("charselect", "button", L[0] // 2 - 70, L[1] // 2 - 30, 140, 20, "Character select"),
                ctl("exit", "button", L[0] // 2 - 70, L[1] // 2, 140, 20, "Exit")]))
        elif self.mode == "makechar":
            self.set_mode("charselect")
        elif self.mode == "charselect":
            self.account = None
            self.set_mode("login")

    def on_button(self, button, down, x, y):
        if down:
            self.buttons.add(button)
        else:
            self.buttons.discard(button)
        if button != "left" or down:
            return
        now = self.frame
        dbl = self.last_click[0] >= now - 30 and abs(self.last_click[1] - x) < 4 and abs(self.last_click[2] - y) < 4
        self.last_click = (now, x, y)
        for w in self.windows:
            wx, wy, ww, wh = w.rect
            if wx <= x < wx + ww and wy <= y < wy + wh:
                c = w.hit(x, y)
                if c:
                    if c["kind"] == "edit":
                        w.focus_control = c["id"]
                    else:
                        self.on_control(w, c, dbl)
                elif w.name == "MAILLISTWND" and x - wx < 20 and 77 <= y - wy < 137:
                    self.mail_write()     # UIMailListWnd::OnLBtnDown: the write tab
                return
        if self.mode == "game":
            self.on_world_click(x, y)

    def on_control(self, w, c, dbl):
        cid = c["id"]
        if w.name == "LOGINWND":
            if cid == "ok":
                self.submit_login()
            elif cid == "cancel":
                self.running = False
        elif w.name == "NOTICECONFIRMWND":
            self.close_window(w.name)
        elif w.name == "SELECTCHARWND":
            if cid.startswith("slot:"):
                self.selected_slot = int(cid[5:])
                if dbl:
                    self.select_char()
            elif cid == "ok":
                self.select_char()
            elif cid == "make":
                if self.selected_slot not in self.chars():
                    self.set_mode("makechar")
            elif cid == "cancel":
                self.account = None
                self.set_mode("login")
        elif w.name == "MAKECHARWND":
            if cid == "ok":
                self.make_char()
            else:
                self.set_mode("charselect")
        elif w.name == "RESTARTWND":
            self.save_char()
            if cid == "charselect":
                self.set_mode("charselect")
            else:
                self.running = False
        elif w.name == "CHOOSESELLBUYWND":
            self.close_window(w.name)
            if cid == "buy":
                self.open_shop()
            elif cid == "sell":
                self.open_sell()
        elif w.name in ("ITEMSHOPWND", "ITEMSELLWND"):
            if cid.startswith("item:"):
                nid = int(cid[5:])
                self.cart[nid] = self.cart.get(nid, 0) + 1
            elif cid == "ok":
                (self.buy if w.name == "ITEMSHOPWND" else self.sell)()
                self.close_window(w.name)
                self.cart = {}
            elif cid == "cancel":
                self.close_window(w.name)
                self.cart = {}
        elif w.name == "CHOOSEWND":
            self.close_window(w.name)
            self.choose(cid)
        elif w.name == "PARTYSETTINGWND":
            self.close_window(w.name)
            if cid == "ok":
                self.make_party(w.data["name"])
        elif w.name == "SAYDIALOGWND" and cid in ("next", "close"):
            self.close_window(w.name)
            if self.talk and self.talk.get("role") == "mailbox":
                self.mailbox_step(cid)
        elif w.name == "MAILLISTWND":
            if cid == "close":
                self.close_window(w.name)
            elif cid.startswith("header"):
                self.mail_open(c["mail_id"])
            elif cid == "send_mail":
                self.mail_send()
        elif w.name == "MAILVIEWWND" and cid == "close":
            self.close_window(w.name)

    # -- login, characters ---------------------------------------------------

    def submit_login(self):
        self.play_wave(BUTTON_WAV, 0.0)
        w = self.window("LOGINWND")
        userid, pw = w.control("id_edit")["text"], w.control("pw_edit")["text"]
        accounts = self.account_rows()
        acc = accounts.get(userid)
        if not acc and len(userid) >= 6 and userid[-2:] in ("_M", "_F") and userid[:-2] not in accounts:
            aid = max(r["account_id"] for r in accounts.values()) + 1
            acc = self.db.put("login", account_id=aid, userid=userid[:-2], user_pass=pw, sex=userid[-1],
                              group_id=0, logincount=0, lastlogin=None)
            self.db.put("loginlog", ip="127.0.0.1", user=userid[:-2], rcode=100,
                        log="account creation (mock)")
            userid = userid[:-2]
        self.set_mode("connecting", timer=10)
        if acc and acc["user_pass"] == pw:
            self.account = acc
            self.db.put("loginlog", ip="127.0.0.1", user=userid, rcode=100, log="login ok")
            self.db.update("login", acc["account_id"], logincount=(acc.get("logincount") or 0) + 1,
                           lastlogin=self.db.now())
            self.after_connect = "charselect"
        else:
            self.db.put("loginlog", ip="127.0.0.1", user=userid, rcode=1, log="Invalid password")
            self.after_connect = "loginfail"

    def select_char(self):
        self.play_wave(BUTTON_WAV, 0.0)
        c = self.chars().get(self.selected_slot)
        if not c:
            self.set_mode("makechar")
            return
        self.db.put("charlog", char_msg="char select", account_id=c["account_id"],
                    char_num=c["char_num"], name=c["name"])
        self.self_ = dict(c)
        self.set_mode("loading", timer=20)

    def make_char(self):
        name = self.window("MAKECHARWND").control("name_edit")["text"]
        if not 4 <= len(name) <= 23 or any(r["name"] == name for r in self.db.rows("char")):
            self.open_window(Window("NOTICECONFIRMWND", [300, 300, 200, 60],
                                    [ctl("ok", "button", 380, 330, 40, 20, "OK")]))
            return
        cid = max([r["char_id"] for r in self.db.rows("char")] or [150000]) + 1
        self.db.put("char", char_id=cid, account_id=self.account["account_id"], char_num=self.selected_slot,
                    name=name, **{"class": 0, "base_level": 1, "job_level": 1, "zeny": 0,
                                  "last_map": "prontera", "last_x": 155, "last_y": 183, "online": 0,
                                  "max_hp": 40, "hp": 40, "max_sp": 11, "sp": 11})
        self.db.put("charlog", char_msg="make new char", account_id=self.account["account_id"],
                    char_num=self.selected_slot, name=name)
        self.set_mode("charselect")

    def save_char(self):
        s = self.self_
        if not s:
            return
        self.db.update("char", s["char_id"], last_map=self.map, last_x=s["x"], last_y=s["y"],
                       base_level=s["base_level"], job_level=s["job_level"], zeny=s["zeny"],
                       online=0, party_id=s.get("party_id", 0), guild_id=s.get("guild_id", 0),
                       **{"class": s["class"]})
        self.self_ = None
        self.map = None

    # -- sound ---------------------------------------------------------------

    def play_stream(self, name):
        if self.stream["file"] == name:      # PlayStream returns early for the same name
            return
        self.stream.update(file=name, started=time.monotonic(), opened=self.stream["opened"] + 1)

    def distance_to(self, a):
        s = self.self_
        return 5.0 * ((a["x"] - s["x"]) ** 2 + (a["y"] - s["y"]) ** 2) ** 0.5   # 5 world units per cell

    def play_wave(self, name, dist):
        """PlayDigital: full volume (100 of 127) within 40 units, 100 * 40 / d to 250, nothing beyond."""
        if dist >= 250:
            return
        vol = 100 if dist <= 40 else int(100 * 40.0 / dist)
        self.voice_serial += 1
        self.voices_started += 1
        v = {"serial": self.voice_serial, "file": name,
             "volume": vol / 127.0, "pan": 0.0, "distance": dist, "loop": False, "started": True, "duration": 0.5}
        self.voices = (self.voices + [v])[-16:]
        self.emit("sound", **v)

    def sound_state(self):
        st = self.stream
        dur = MOCK_MUSIC_SECONDS
        played = time.monotonic() - st["started"] if st["started"] else 0.0
        return {"mode": "silent", "max_voices": 48, "voices_started": self.voices_started, "voices_playing": 0,
                "voices_stolen": 0, "voices_rejected": 0, "decodes": self.voices_started, "decode_failures": 0,
                "effects_volume": 1.0, "music_volume": 1.0,
                "stream": {"file": "/mock/" + st["file"].replace("\\", "/").upper() if st["file"] else None,
                           "status": "playing" if st["file"] else "none", "loops": int(played // dur),
                           "duration": dur, "position": played % dur, "volume": 100 / 127.0,
                           "opened": st["opened"], "failures": 0},
                "recent": list(self.voices)}

    # -- world ---------------------------------------------------------------

    def enter_map(self, name, x, y):
        s = self.self_
        self.map = name
        s["x"], s["y"], s["dest"], s["path_wait"] = x, y, None, 0
        self.actors = []
        for (m, ax, ay, nm, job, role) in NPCS:
            if m == name:
                self.add_actor("npc", nm, job, ax, ay, role=role)
        for (m, ax, ay) in PORINGS:
            if m == name:
                self.add_actor("monster", "Poring", 1002, ax, ay, hp=3)
        for (m, ax, ay, *_dst) in PORTALS:
            if m == name:
                self.add_actor("portal", "", 45, ax, ay)
        self.attack = self.pick = self.talk = None
        self.play_stream(MUSIC.get(name, "bgm\\01.mp3"))
        self.trace("game", "enter %s %d %d" % (name, x, y))
        self.emit("map", map=name, x=x, y=y)

    def add_actor(self, type_, name, job, x, y, **extra):
        self.next_actor += 1
        a = {"id": self.next_actor, "type": type_, "name": name, "job": job, "x": x, "y": y}
        a.update(extra)
        self.actors.append(a)
        return a

    def screen_of(self, x, y):
        L = self.logical()
        sx = L[0] // 2 + (x - self.self_["x"]) * CELL
        sy = L[1] // 2 - (y - self.self_["y"]) * CELL
        return {"x": sx, "y": sy} if 0 <= sx < L[0] and 0 <= sy < L[1] else None

    def hover(self, x, y):
        """The pointer resting on an actor makes its name known, as the
        client's name request on hover does (PROTOCOL.md, `actors`)."""
        if self.mode != "game" or not self.self_:
            return
        for a in self.actors:
            p = self.screen_of(a["x"], a["y"])
            if p and abs(p["x"] - x) <= CELL // 2 and abs(p["y"] - y) <= CELL // 2:
                a["named"] = True

    def shown_name(self, a):
        if a["type"] == "item" or a.get("named"):
            return a["name"]
        return "?name"

    def cell_at(self, x, y):
        if self.mode != "game" or not self.self_:
            return None
        L = self.logical()
        cx = self.self_["x"] + int(round((x - L[0] // 2) / CELL))
        cy = self.self_["y"] - int(round((y - L[1] // 2) / CELL))
        w, h = MAPS.get(self.map, (400, 400))
        if 0 <= cx < w and 0 <= cy < h:
            return {"x": cx, "y": cy}
        return None

    def on_world_click(self, x, y):
        for a in self.actors:
            p = self.screen_of(a["x"], a["y"])
            if p and abs(p["x"] - x) <= CELL // 2 and abs(p["y"] - y) <= CELL // 2 and a["type"] != "portal":
                self.interact(a)
                return
        cell = self.cell_at(x, y)
        if cell:
            self.attack = self.pick = None
            self.walk_to(cell["x"], cell["y"])

    def walk_to(self, x, y):
        self.self_["dest"] = {"x": x, "y": y}

    def interact(self, a):
        self.trace("game", "interact %s %s" % (a["type"], a["name"]))
        if a["type"] == "monster":
            self.attack = a
            self.walk_to(a["x"], a["y"] + 1)
        elif a["type"] == "item":
            self.pick = a
            self.walk_to(a["x"], a["y"])
        elif a["type"] == "npc":
            self.talk = a
            L = self.logical()
            if a["role"] == "shop":
                self.open_window(Window("CHOOSESELLBUYWND", [L[0] // 2 - 60, L[1] // 2 - 30, 120, 60], [
                    ctl("buy", "button", L[0] // 2 - 55, L[1] // 2 - 20, 50, 20, "Buy"),
                    ctl("sell", "button", L[0] // 2 + 5, L[1] // 2 - 20, 50, 20, "Sell"),
                    ctl("cancel", "button", L[0] // 2 - 25, L[1] // 2 + 5, 50, 20, "Cancel")]))
            elif a["role"] == "mailbox":
                self.mailbox_step(None)
            elif a["role"] == "jobmaster":
                s = self.self_
                if s["class"] != 0 or s["job_level"] < 10:
                    self.open_window(Window("SAYDIALOGWND", [L[0] // 2 - 150, L[1] // 2 - 60, 300, 120],
                                            [ctl("text", "text", L[0] // 2 - 140, L[1] // 2 - 50, 280, 60,
                                                 "[Job Master] You need job level 10 as a Novice.")]))
                    self.system("[Job Master] You need job level 10 as a Novice.", "normal")
                else:
                    ctls = [ctl("choice:%s" % n, "button", L[0] // 2 - 90, L[1] // 2 - 60 + i * 20, 180, 18, n)
                            for i, n in enumerate(JOBS)]
                    ctls.append(ctl("choice:cancel", "button", L[0] // 2 - 90, L[1] // 2 + 60, 180, 18, "Cancel"))
                    self.open_window(Window("CHOOSEWND", [L[0] // 2 - 100, L[1] // 2 - 70, 200, 160], ctls))
            else:
                self.open_window(Window("SAYDIALOGWND", [L[0] // 2 - 150, L[1] // 2 - 60, 300, 120],
                                        [ctl("text", "text", L[0] // 2 - 140, L[1] // 2 - 50, 280, 60,
                                             "[%s] Hello." % a["name"])]))

    def choose(self, cid):
        name = cid.split(":", 1)[1]
        s = self.self_
        if self.talk and self.talk.get("role") == "mailbox":
            if name == "Yes.":
                self.talk["paid"] = s["zeny"] >= MAILBOX_FEE
                if self.talk["paid"]:
                    self.add_zeny(-MAILBOX_FEE, "N")
                self.mailbox_step("close-pending")
            else:
                self.talk = None
            return
        if name in JOBS:
            s["class"], s["job_level"] = JOBS[name], 1
            self.system("[Job Master] You are now a %s." % name, "normal")
            self.trace("game", "job change %s" % name)
        self.talk = None

    # -- mail (npc/other/mail.txt, then rAthena's pre-RODEX mail) ------------

    def mailbox_step(self, after):
        """The Mailbox's dialog: text and next, the menu, a closing line and
        close, then `openmail` opens the mail list."""
        L = self.logical()
        rect = [L[0] // 2 - 150, L[1] // 2 - 60, 300, 120]
        button = lambda cid: ctl(cid, "button", L[0] // 2 + 90, L[1] // 2 + 35, 42, 20, "")
        if after is None:
            self.open_window(Window("SAYDIALOGWND", rect, [
                ctl("text", "text", rect[0] + 10, rect[1] + 10, 280, 60, "[Mailbox] 130 zeny?"), button("next")]))
        elif after == "next":
            self.open_window(Window("CHOOSEWND", [L[0] // 2 - 100, L[1] // 2 - 70, 200, 80], [
                ctl("choice:Yes.", "button", L[0] // 2 - 90, L[1] // 2 - 60, 180, 18, "Yes."),
                ctl("choice:No.", "button", L[0] // 2 - 90, L[1] // 2 - 40, 180, 18, "No.")]))
        elif after == "close-pending":
            self.open_window(Window("SAYDIALOGWND", rect, [
                ctl("text", "text", rect[0] + 10, rect[1] + 10, 280, 60, "[Mailbox] Thank you."), button("close")]))
        elif after == "close":
            paid = self.talk.get("paid")
            self.talk = None
            if paid:
                self.mail_list()

    def inbox(self):
        return [m for m in self.db.rows("mail") if m["dest_id"] == self.self_["char_id"]]

    def mail_list(self):
        L = self.logical()
        x, y = L[0] // 2 - 300, L[1] // 2 - 220
        ctls = [ctl("close", "button", x + 288, y + 4, 7, 7)]
        for i, m in enumerate(self.inbox()[:7]):
            ctls.append(ctl("from_name" + (":%d" % (2 * i + 1) if i else ""), "button", x + 70, y + 32 + 50 * i,
                            120, 14, m["send_name"]))
            ctls.append(ctl("header" + (":%d" % (2 * i + 2) if i else ""), "button", x + 70, y + 49 + 50 * i,
                            190, 14, m["title"], mail_id=m["id"]))
        w = self.window("MAILLISTWND") or self.open_window(Window("MAILLISTWND", [x, y, 300, 400]))
        w.controls = ctls
        w.focus_control = None

    def mail_write(self):
        w = self.window("MAILLISTWND")
        x, y = w.rect[0], w.rect[1]
        w.controls = [c for c in w.controls if c["id"] == "close"] + [
            ctl("edit:0", "edit", x + 90, y + 34, 110, 16, limit=23),
            ctl("edit:1", "edit", x + 90, y + 65, 170, 16, limit=39),
            ctl("edit:2", "edit", x + 41, y + 100, 238, 216, limit=198),
            ctl("edit:3", "edit", -100, -100, 65, 16, limit=9),
            ctl("send_mail", "button", x + 183, y + 370, 43, 21),
            ctl("cancel_mail", "button", x + 231, y + 370, 43, 21)]
        w.focus_control = "edit:2"

    def mail_open(self, mid):
        m = next((m for m in self.inbox() if m["id"] == mid), None)
        if not m:
            return
        self.db.update("mail", mid, status=2)        # MAIL_READ
        L = self.logical()
        x, y = L[0] // 2, L[1] // 2 - 220
        self.close_window("MAILVIEWWND")
        self.open_window(Window("MAILVIEWWND", [x, y, 300, 400], [
            ctl("close", "button", x + 288, y + 4, 7, 7),
            ctl("list:0", "list", x + 13, y + 96, 258, 216, items=[m["message"]]),
            ctl("return_mail", "button", x + 127, y + 370, 43, 21),
            ctl("send_mail", "button", x + 175, y + 370, 43, 21),
            ctl("delete_mail", "button", x + 223, y + 370, 43, 21)]))

    def mail_send(self):
        w = self.window("MAILLISTWND")
        to, title, text = (w.control("edit:%d" % i)["text"] for i in range(3))
        dest = next((c for c in self.db.rows("char") if c["name"] == to), None)
        if not title:
            return
        if not dest:
            self.system("The recipient does not exist.")
            return
        mid = max([m["id"] for m in self.db.rows("mail")] or [0]) + 1
        s = self.self_
        self.db.put("mail", id=mid, send_name=s["name"], send_id=s["char_id"], dest_name=to,
                    dest_id=dest["char_id"], title=title, message=text, time=int(time.time()), status=0,
                    zeny=0, type=0)
        self.system("You have successfully mailed a message.")

    def open_shop(self):
        L = self.logical()
        ctls = [ctl("item:%d" % n, "item", L[0] // 2 - 140, L[1] // 2 - 80 + i * 20, 200, 18, ITEMS[n][0])
                for i, n in enumerate((501, 502))]
        ctls += [ctl("ok", "button", L[0] // 2 + 70, L[1] // 2 + 60, 50, 20, "Buy"),
                 ctl("cancel", "button", L[0] // 2 + 125, L[1] // 2 + 60, 50, 20, "Cancel")]
        self.open_window(Window("ITEMSHOPWND", [L[0] // 2 - 150, L[1] // 2 - 90, 330, 180], ctls))

    def open_sell(self):
        L = self.logical()
        inv = self.inventory()
        ctls = [ctl("item:%d" % r["nameid"], "item", L[0] // 2 - 140, L[1] // 2 - 80 + i * 20, 200, 18,
                    "%s %d" % (ITEMS.get(r["nameid"], ("?item", 0))[0], r["amount"]))
                for i, r in enumerate(inv)]
        ctls += [ctl("ok", "button", L[0] // 2 + 70, L[1] // 2 + 60, 50, 20, "Sell"),
                 ctl("cancel", "button", L[0] // 2 + 125, L[1] // 2 + 60, 50, 20, "Cancel")]
        self.open_window(Window("ITEMSELLWND", [L[0] // 2 - 150, L[1] // 2 - 90, 330, 180], ctls))

    def inventory(self):
        return [r for r in self.db.rows("inventory") if r["char_id"] == self.self_["char_id"]]

    def add_item(self, nameid, amount, log_type):
        s = self.self_
        inv = {r["nameid"]: r for r in self.inventory()}
        if nameid in inv:
            r = inv[nameid]
            n = r["amount"] + amount
            if n <= 0:
                self.db.put("inventory", id=r["id"], _deleted=True)
            else:
                self.db.update("inventory", r["id"], amount=n)
        elif amount > 0:
            iid = max([r["id"] for r in self.db.rows("inventory")] or [0]) + 1
            self.db.put("inventory", id=iid, char_id=s["char_id"], nameid=nameid, amount=amount, equip=0)
        self.db.put("picklog", char_id=s["char_id"], type=log_type, nameid=nameid, amount=amount,
                    refine=0, map=self.map)

    def add_zeny(self, amount, log_type, src_id=None):
        s = self.self_
        s["zeny"] += amount
        self.db.put("zenylog", char_id=s["char_id"], src_id=src_id or s["char_id"], type=log_type,
                    amount=amount, map=self.map)

    def buy(self):
        cost = sum(ITEMS[n][1] * k for n, k in self.cart.items())
        if cost > self.self_["zeny"]:
            self.system("You do not have enough zeny.")
            return
        for n, k in self.cart.items():
            self.add_item(n, k, "S")
        self.add_zeny(-cost, "S")

    def sell(self):
        have = {r["nameid"]: r["amount"] for r in self.inventory()}
        gain = 0
        for n, k in self.cart.items():
            k = min(k, have.get(n, 0))
            if k:
                self.add_item(n, -k, "S")
                gain += (ITEMS.get(n, ("", 0))[1] // 2) * k
        if gain:
            self.add_zeny(gain, "S")

    def say(self, line):
        s = self.self_
        acc = self.account
        if line.startswith("@"):
            if acc.get("group_id", 0) < 99:
                self.system("%s: unknown command." % line.split()[0])
                return
            self.db.put("atcommandlog", account_id=acc["account_id"], char_id=s["char_id"],
                        char_name=s["name"], map=self.map, command=line)
            self.atcommand(line.split())
            return
        # Party and guild (M8.1b), as the 2008 client sends them: /organize
        # opens the sharing rules first, % and $ prefix a party and a guild line.
        words = line.split()
        if words and words[0] in ("/organize", "/guild") and len(words) == 2:
            if words[0] == "/organize":
                L = self.logical()
                self.open_window(Window("PARTYSETTINGWND", [L[0] // 2 - 100, L[1] // 2 - 90, 200, 180], [
                    ctl("ok", "button", L[0] // 2 + 8, L[1] // 2 + 66, 42, 20, "OK"),
                    ctl("cancel", "button", L[0] // 2 + 54, L[1] // 2 + 66, 42, 20, "Cancel")],
                    data={"name": words[1]}))
            else:
                self.make_guild(words[1])
            return
        if line.startswith("/"):
            return   # emotes and the like: nothing the server logs
        kind = "O"
        if line.startswith("%") and s.get("party_id"):
            kind, line = "P", line[1:]
        elif line.startswith("$") and s.get("guild_id"):
            kind, line = "G", line[1:]
        self.db.put("chatlog", type=kind, type_id=s.get({"P": "party_id", "G": "guild_id"}.get(kind), 0),
                    src_charid=s["char_id"], src_accountid=acc["account_id"],
                    src_map=self.map, src_map_x=s["x"], src_map_y=s["y"], dst_charname=None, message=line)
        self.emit("chat", channel={"O": "normal", "P": "party", "G": "guild"}[kind], text=line,
                  **{"from": s["name"]})

    def whisper(self, name, line):
        s = self.self_
        self.db.put("chatlog", type="W", type_id=0, src_charid=s["char_id"],
                    src_accountid=self.account["account_id"], src_map=self.map, src_map_x=s["x"],
                    src_map_y=s["y"], dst_charname=name, message=line)
        if not any(r["name"] == name and r.get("online") for r in self.db.rows("char")):
            self.system("%s is not online." % name)

    def make_party(self, name):
        s = self.self_
        pid = max([r["party_id"] for r in self.db.rows("party")] or [0]) + 1
        self.db.put("party", party_id=pid, name=name, exp=0, item=0, leader_id=self.account["account_id"],
                    leader_char=s["char_id"])
        s["party_id"] = pid
        self.system("Party has successfully been organized.")

    def make_guild(self, name):
        s = self.self_
        if not any(r["nameid"] == 714 for r in self.inventory()):
            self.system("You need the necessary item to create a Guild.")
            return
        self.add_item(714, -1, "C")
        gid = max([r["guild_id"] for r in self.db.rows("guild")] or [0]) + 1
        self.db.put("guild", guild_id=gid, name=name, char_id=s["char_id"], master=s["name"], guild_lv=1)
        self.db.put("guild_member", guild_id=gid, char_id=s["char_id"], position=0)
        s["guild_id"] = gid
        self.system("[%s] Guild has been created." % name)

    def atcommand(self, argv):
        s = self.self_
        cmd, args = argv[0][1:].lower(), argv[1:]
        try:
            if cmd == "warp":
                self.enter_map(args[0], int(args[1]), int(args[2]))
            elif cmd == "where":
                self.system("%s %d %d" % (self.map, s["x"], s["y"]))
            elif cmd == "zeny":
                s["zeny"] += int(args[0])
                self.db.put("zenylog", char_id=s["char_id"], src_id=s["char_id"], type="A",
                            amount=int(args[0]), map=self.map)
            elif cmd == "item":
                self.add_item(int(args[0]), int(args[1]) if len(args) > 1 else 1, "A")
            elif cmd == "jlvl":
                s["job_level"] = min(50, s["job_level"] + int(args[0]))
            elif cmd == "blvl":
                s["base_level"] = min(99, s["base_level"] + int(args[0]))
            elif cmd == "jobchange":
                s["class"] = int(args[0]) if args[0].isdigit() else JOBS[args[0].capitalize()]
            elif cmd == "monster":
                for i in range(int(args[1]) if len(args) > 1 else 1):
                    self.add_actor("monster", "Poring", 1002, s["x"] + 2 + i, s["y"] + 2, hp=3)
            elif cmd == "heal":
                s["hp"], s["sp"] = s["max_hp"], s["max_sp"]
            else:
                self.system("Unknown command.")
                return
        except (IndexError, ValueError, KeyError):
            self.system("Usage error.")
            return
        self.system("%s done." % cmd)

    def update_world(self):
        s = self.self_
        d = s.get("dest")
        if d:
            s["path_wait"] += 1
            if s["path_wait"] >= WALK_STEPS:
                s["path_wait"] = 0
                s["x"] += (d["x"] > s["x"]) - (d["x"] < s["x"])
                s["y"] += (d["y"] > s["y"]) - (d["y"] < s["y"])
                if (s["x"], s["y"]) == (d["x"], d["y"]):
                    s["dest"] = None
                    self.arrived()
                    return
                for (m, px, py, dm, dx, dy) in PORTALS:
                    if m == self.map and (s["x"], s["y"]) == (px, py):
                        self.enter_map(dm, dx, dy)
                        return
        if self.attack and not s.get("dest"):
            a = self.attack
            if a not in self.actors:
                self.attack = None
                return
            if self.frame % 20 == 0:
                a["hp"] -= 1
                self.play_wave("attack_fist.wav", 0.0)
                self.play_wave("poring_damage.wav", self.distance_to(a))
            if a["hp"] <= 0:
                self.play_wave("poring_die.wav", self.distance_to(a))
                self.actors.remove(a)
                self.attack = None
                self.db.put("picklog", char_id=a["job"], type="M", nameid=909, amount=-1, refine=0, map=self.map)
                self.add_actor("item", "Jellopy", 909, a["x"], a["y"])
                s["base_exp"] = s.get("base_exp", 0) + 2
                self.trace("game", "killed %s" % a["name"])

    def ambient(self):
        """Each monster makes its move sound every two seconds."""
        if self.frame % 120 == 0:
            for a in self.actors:
                if a["type"] == "monster":
                    self.play_wave("poring_move.wav", self.distance_to(a))

    def arrived(self):
        s = self.self_
        if self.pick and (s["x"], s["y"]) == (self.pick["x"], self.pick["y"]):
            a, self.pick = self.pick, None
            if a in self.actors:
                self.actors.remove(a)
                self.add_item(a["job"], 1, "P")
                self.trace("game", "picked up %s" % a["name"])
        for (m, px, py, dm, dx, dy) in PORTALS:
            if m == self.map and (s["x"], s["y"]) == (px, py):
                self.enter_map(dm, dx, dy)

    # -- the loop ------------------------------------------------------------

    def tick(self):
        if self.inputs:
            ev = self.inputs.pop(0)
            ev()
            self.consumed += 1
        if self.mode in ("connecting", "loading"):
            self.mode_timer -= 1
            if self.mode_timer <= 0:
                if self.mode == "connecting":
                    if self.after_connect == "charselect":
                        self.set_mode("charselect")
                    else:
                        self.set_mode("login")
                        L = self.logical()
                        self.open_window(Window("NOTICECONFIRMWND", [L[0] // 2 - 100, L[1] // 2 - 30, 200, 60],
                                                [ctl("ok", "button", L[0] // 2 - 20, L[1] // 2, 40, 20, "OK")]))
                else:
                    s = self.self_
                    self.db.update("char", s["char_id"], online=1)
                    s.update(online=1, x=s["last_x"], y=s["last_y"], dest=None, path_wait=0)
                    self.set_mode("game")
                    self.enter_map(s["last_map"], s["last_x"], s["last_y"])
        elif self.mode == "game":
            self.update_world()
            if self.mode == "game" and self.self_:
                self.ambient()
        self.frame += 1

    def queue_input(self, events):
        self.inputs.extend(events)
        self.input_token += len(events)
        return self.input_token

    def run(self):
        if os.path.exists(self.path):
            os.unlink(self.path)
        srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        srv.bind(self.path)
        srv.listen(4)
        srv.setblocking(False)
        self.sel.register(srv, selectors.EVENT_READ, "accept")
        self.trace("ctl", "listening on %s" % self.path)
        next_tick = time.monotonic()
        try:
            while self.running:
                timeout = max(0.0, next_tick - time.monotonic())
                for key, _ in self.sel.select(timeout):
                    if key.data == "accept":
                        self.accept(srv)
                    else:
                        self.read(key.data)
                if time.monotonic() >= next_tick:
                    self.tick()
                    next_tick += STEP
                    if next_tick < time.monotonic() - 1:
                        next_tick = time.monotonic()
                    if self.conn and self.conn.pending:
                        self.check_pending(self.conn)
                if self.conn:
                    self.process(self.conn)
        finally:
            self.emit("bye", reason=self.bye_reason)
            if self.conn:
                try:
                    self.conn.sock.setblocking(True)
                    self.conn.sock.sendall(self.conn.out)
                except OSError:
                    pass
                self.conn.sock.close()
            srv.close()
            if os.path.exists(self.path):
                os.unlink(self.path)
            if self.mode == "game":
                self.save_char()
            self.trace("ctl", "exit")

    def accept(self, srv):
        sock, _ = srv.accept()
        if self.conn:
            sock.sendall(b'{"event":"error","code":"busy"}\n')
            sock.close()
            return
        sock.setblocking(False)
        self.conn = Conn(sock)
        self.sel.register(sock, selectors.EVENT_READ, self.conn)
        self.trace("ctl", "connected")

    def read(self, c):
        try:
            data = c.sock.recv(65536)
        except BlockingIOError:
            return
        except OSError:
            data = b""
        if not data:
            self.trace("ctl", "disconnected")
            self.drop(c)
            return
        c.buf += data
        while b"\n" in c.buf:
            line, c.buf = c.buf.split(b"\n", 1)
            if line.strip():
                c.inbox.append(line)

    def process(self, c):
        while c.inbox and not c.pending and self.conn is c and self.running:
            line = c.inbox.pop(0)
            rid = None
            try:
                try:
                    msg = json.loads(line.decode("utf-8"))
                except (ValueError, UnicodeDecodeError):
                    raise ProtocolError("bad_request", "not JSON")
                if not isinstance(msg, dict):
                    raise ProtocolError("bad_request", "not an object")
                rid = msg.get("id")
                cmd = msg.get("cmd")
                if not isinstance(cmd, str):
                    raise ProtocolError("bad_request", "no cmd")
                if cmd in RESERVED:
                    raise ProtocolError("unsupported", "%s is reserved" % cmd)
                handler = getattr(self, "cmd_" + cmd, None) if cmd in COMMANDS else None
                if not handler:
                    raise ProtocolError("unknown_command", cmd)
                handler(c, rid, msg)
            except ProtocolError as e:
                self.error(c, rid, e.code, e.message)
            except Exception as e:  # a bug in the mock
                self.trace("err", "internal: %r" % e)
                self.error(c, rid, "internal", repr(e))

    def check_pending(self, c):
        done = c.pending()
        if done:
            c.pending = None

    # -- commands ------------------------------------------------------------

    def cmd_hello(self, c, rid, msg):
        self.reply(c, rid, protocol=PROTOCOL_VERSION, client=paths.app_name(), version="mock-0",
                   pid=os.getpid(), commands=COMMANDS, events=EVENTS)

    def _after_input(self, c, rid, token, **result):
        def pending():
            if self.consumed >= token:
                self.reply(c, rid, **result)
                return True
            return False
        c.pending = pending

    def cmd_key(self, c, rid, msg):
        action = msg.get("action")
        if action == "text":
            text = msg.get("text")
            if not isinstance(text, str):
                raise ProtocolError("bad_request", "text needs a string")
            if not cp1252_ok(text):
                raise ProtocolError("bad_text", "not representable in Windows-1252")
            if any(ord(ch) < 0x20 or ord(ch) == 0x7f for ch in text):
                raise ProtocolError("bad_text", "control characters are keys, not text")
            token = self.queue_input([(lambda ch=ch: self.on_text(ch)) for ch in text] or [lambda: None])
        elif action in ("down", "up", "press"):
            key = msg.get("key")
            mods = msg.get("mods") or []
            if key not in KEYS:
                raise ProtocolError("bad_key", repr(key))
            if not isinstance(mods, list) or not set(mods) <= MODS:
                raise ProtocolError("bad_request", "mods: %r" % mods)
            evs = []
            if action in ("down", "press"):
                evs.append(lambda: self.on_key(key, True, mods))
            if action in ("up", "press"):
                evs.append(lambda: self.on_key(key, False, mods))
            token = self.queue_input(evs)
        else:
            raise ProtocolError("bad_request", "action: %r" % action)
        self._after_input(c, rid, token)

    def cmd_pointer(self, c, rid, msg):
        action = msg.get("action")
        button = msg.get("button", "left")
        if button not in ("left", "right", "middle"):
            raise ProtocolError("bad_request", "button: %r" % button)
        x, y = msg.get("x"), msg.get("y")
        if (x is None) != (y is None) or (x is not None and not (isinstance(x, int) and isinstance(y, int))):
            raise ProtocolError("bad_request", "x and y are integers, both or neither")
        evs = []
        if x is not None:
            def mv(x=x, y=y):
                self.pointer = [x, y]
                self.hover(x, y)
            evs.append(mv)
        px, py = (x, y) if x is not None else tuple(self.pointer)
        if action == "move":
            if x is None:
                raise ProtocolError("bad_request", "move needs x and y")
        elif action in ("down", "up"):
            evs.append(lambda: self.on_button(button, action == "down", px, py))
        elif action in ("click", "doubleclick"):
            for _ in range(2 if action == "doubleclick" else 1):
                evs.append(lambda: self.on_button(button, True, px, py))
                evs.append(lambda: self.on_button(button, False, px, py))
        elif action == "wheel":
            if not isinstance(msg.get("dy"), int):
                raise ProtocolError("bad_request", "wheel needs dy")
            evs.append(lambda: None)
        else:
            raise ProtocolError("bad_request", "action: %r" % action)
        token = self.queue_input(evs)
        self._after_input(c, rid, token, x=px, y=py)

    def cmd_capture(self, c, rid, msg):
        path = msg.get("path")
        if not isinstance(path, str) or not os.path.isabs(path):
            raise ProtocolError("bad_request", "path must be absolute")
        if not os.path.isdir(os.path.dirname(path)):
            raise ProtocolError("io", "no directory %s" % os.path.dirname(path))
        start = self.frame

        def pending():
            if self.frame <= start:
                return False
            try:
                data = self.render(path)
            except OSError as e:
                self.error(c, rid, "io", str(e))
                return True
            import hashlib
            L = self.logical()
            self.reply(c, rid, path=path, width=self.width, height=self.height, scale=self.scale,
                       logical={"w": L[0], "h": L[1]}, frame=self.frame,
                       sha256=hashlib.sha256(data).hexdigest())
            return True
        c.pending = pending

    def render(self, path):
        """A synthetic frame: mode colour, windows, actors, self (device pixels)."""
        bg = {"login": (40, 60, 120, 255), "connecting": (40, 40, 40, 255), "charselect": (60, 40, 100, 255),
              "makechar": (60, 80, 60, 255), "loading": (0, 0, 0, 255), "game": (70, 110, 60, 255)}
        k = self.scale
        rects = []
        if self.mode == "game":
            for a in self.actors:
                p = self.screen_of(a["x"], a["y"])
                if p:
                    col = {"npc": (240, 200, 60, 255), "monster": (240, 120, 160, 255),
                           "item": (120, 220, 240, 255)}.get(a["type"], (200, 200, 200, 255))
                    rects.append(((p["x"] - 6) * k, (p["y"] - 12) * k, 12 * k, 12 * k, col))
            L = self.logical()
            rects.append(((L[0] // 2 - 6) * k, (L[1] // 2 - 14) * k, 12 * k, 14 * k, (255, 255, 255, 255)))
        for w in reversed(self.windows):
            x, y, ww, hh = w.rect
            rects.append((x * k, y * k, ww * k, hh * k, (200, 200, 210, 255)))
            for ctrl in w.controls:
                cx, cy, cw, ch = ctrl["rect"]
                rects.append((cx * k, cy * k, cw * k, ch * k, (255, 255, 255, 255) if ctrl["kind"] == "edit"
                              else (120, 130, 170, 255)))
        return write_png(path, self.width, self.height, rects, bg.get(self.mode, (0, 0, 0, 255)))

    def state(self, fields):
        L = self.logical()
        s = self.self_ if self.mode == "game" else None
        out = {}
        want = set(fields) if fields else None

        def put(name, value):
            if want is None or name in want:
                out[name] = value
        put("mode", self.mode)
        put("detail", DETAIL.get(self.mode, self.mode))
        put("frame", self.frame)
        put("view", {"width": self.width, "height": self.height, "scale": self.scale,
                     "logical": {"w": L[0], "h": L[1]}, "fullscreen": self.fullscreen, "focused": True})
        put("pointer", {"x": self.pointer[0], "y": self.pointer[1], "buttons": sorted(self.buttons),
                        "cell": self.cell_at(*self.pointer)})
        put("map", {"name": self.map, "width": MAPS.get(self.map, (400, 400))[0],
                    "height": MAPS.get(self.map, (400, 400))[1]} if s else None)
        put("self", {"id": s["account_id"], "char_id": s["char_id"], "name": s["name"], "job": s["class"],
                     "base_level": s["base_level"], "job_level": s["job_level"], "x": s["x"], "y": s["y"],
                     "dir": 0, "moving": s.get("dest") is not None, "dest": s.get("dest"),
                     "hp": s["hp"], "max_hp": s["max_hp"], "sp": s["sp"], "max_sp": s["max_sp"],
                     "zeny": s["zeny"], "weight": 0, "max_weight": 2400} if s else None)
        put("actors", [{"id": a["id"], "type": a["type"], "name": self.shown_name(a), "job": a["job"], "x": a["x"],
                        "y": a["y"], "screen": self.screen_of(a["x"], a["y"])} for a in self.actors] if s else [])
        put("windows", [w.to_json(i == 0) for i, w in enumerate(self.windows)])
        put("sound", self.sound_state())
        return out

    def cmd_state(self, c, rid, msg):
        fields = msg.get("fields")
        if fields is not None and not (isinstance(fields, list) and all(isinstance(f, str) for f in fields)):
            raise ProtocolError("bad_request", "fields is a list of names")
        self.reply(c, rid, **self.state(fields))

    def condition(self, cond, after, c):
        """None if not met; else (met, event or None)."""
        event = None
        for k, v in cond.items():
            if k == "mode":
                if self.mode != v:
                    return None
            elif k == "map":
                if self.mode != "game" or self.map != v:
                    return None
            elif k == "window":
                if (self.window(v) is not None) != bool(cond.get("open", True)):
                    return None
            elif k == "open":
                continue
            elif k == "position":
                s = self.self_
                if self.mode != "game" or (s["x"], s["y"]) != (v["x"], v["y"]):
                    return None
            elif k == "moving":
                s = self.self_
                if self.mode != "game" or (s.get("dest") is not None) != bool(v):
                    return None
            elif k == "frames":
                if self.frame < cond["_frame0"] + v:
                    return None
            elif k == "_frame0":
                continue
            elif k == "event":
                match = cond.get("match") or {}
                contains = cond.get("contains")
                for e in c.events:
                    if e["seq"] > after and e["event"] == v and \
                            (contains is None or contains in (e.get("text") or "")) and \
                            all(e.get(mk) == mv for mk, mv in match.items()):
                        event = e
                        break
                else:
                    return None
            elif k in ("match", "contains"):
                continue
            else:
                raise ProtocolError("bad_request", "unknown condition %r" % k)
        return {kk: vv for kk, vv in cond.items() if not kk.startswith("_")}, event

    def cmd_wait(self, c, rid, msg):
        cond = msg.get("until")
        if not isinstance(cond, dict) or not cond:
            raise ProtocolError("bad_request", "until is a condition object")
        timeout = msg.get("timeout", 10000)
        if not isinstance(timeout, int) or not 0 <= timeout <= 600000:
            raise ProtocolError("bad_request", "timeout is 0..600000 ms")
        after = msg.get("after", c.reply_seq)
        cond = dict(cond, _frame0=self.frame)
        deadline = time.monotonic() + timeout / 1000.0
        self.condition(cond, after, c)   # raises on a bad condition

        def pending():
            r = self.condition(cond, after, c)
            if r:
                met, event = r
                extra = {"event": event} if event else {}
                self.reply(c, rid, met=met, mode=self.mode, frame=self.frame, **extra)
                return True
            if time.monotonic() >= deadline:
                self.error(c, rid, "timeout", "condition not met in %d ms" % timeout,
                           mode=self.mode, frame=self.frame)
                return True
            return False
        if not pending():
            c.pending = pending

    def cmd_resize(self, c, rid, msg):
        w, h = msg.get("width", self.width), msg.get("height", self.height)
        scale = msg.get("scale", self.scale)
        fs = msg.get("fullscreen", self.fullscreen)
        if not all(isinstance(v, int) for v in (w, h)) or w < 320 or h < 240 or w > 8192 or h > 8192:
            raise ProtocolError("bad_request", "width and height are 320..8192")
        if scale not in (1, 2, 3):
            raise ProtocolError("bad_request", "scale is 1, 2 or 3")
        if self.fullscreen and fs and ("width" in msg or "height" in msg):
            raise ProtocolError("bad_state", "cannot size a fullscreen window")
        if fs and not self.fullscreen:
            w, h = 2560, 1600
        self.width, self.height, self.scale, self.fullscreen = w, h, scale, bool(fs)
        L = self.logical()
        for win in self.windows:  # clamp into view
            win.rect[0] = max(0, min(win.rect[0], L[0] - win.rect[2]))
            win.rect[1] = max(0, min(win.rect[1], L[1] - win.rect[3]))
        start = self.frame

        def pending():
            if self.frame > start:
                self.reply(c, rid, view=self.state(["view"])["view"])
                return True
            return False
        c.pending = pending

    def cmd_quit(self, c, rid, msg):
        self.reply(c, rid)
        self.running = False


def main():
    argv = sys.argv[1:]
    path = os.environ.get("BF_CONTROL")
    if "--control" in argv:
        path = argv[argv.index("--control") + 1]
    if not path:
        print("mock_client: no --control path and no BF_CONTROL", file=sys.stderr)
        return 2
    db = os.environ.get("BF_MOCK_DB") or os.path.join(os.path.dirname(path), "mock-db.jsonl")
    m = Mock(path, MockDb(db), os.environ.get("BF_TRACE_FILE"))
    def on_term(*_):
        m.bye_reason = "signal"
        m.running = False
    signal.signal(signal.SIGTERM, on_term)
    m.run()
    return 0


if __name__ == "__main__":
    sys.exit(main())
