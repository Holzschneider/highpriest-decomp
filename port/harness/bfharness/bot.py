"""A second player for the scene: bifrost-bot (port/bot) logged in beside the
client, driven through its stdin, its events read from its stdout.

The bot is set-up, never the action under test: it stands where a script
puts it (an atcommand typed by the bot on a group-99 account), spawns a
monster for the frame, says a line the client must show. Its events go to
`<run dir>/bot-<label>.events`, its stderr beside them.

    bot = Bot.start(t, "walk", user="bfhwalkgm", password="bfbot", char="BfhWalkGM", gm=True)
    bot.gm("@warp prontera 158 180")
    bot.wait(lambda e: e.get("event") == "position" and e.get("warped"))
    bot.quit()

With --mock there is no server: `Bot.start` returns None and scripts skip
what needs the bot.
"""

import json
import os
import subprocess
import threading
import time
from pathlib import Path

from . import paths


def binary():
    """BF_BOT, else build/port/bin/bifrost-bot of this checkout (port/bot/build.sh)."""
    env = os.environ.get("BF_BOT")
    return Path(env) if env else paths.REPO / "build" / "port" / "bin" / "bifrost-bot"


class BotError(AssertionError):
    pass


class Bot:
    def __init__(self, run_dir, label, user, password, char, host="127.0.0.1", port=6900):
        self.label, self.char = label, char
        self.events = []
        self.cond = threading.Condition()
        self.log = open(Path(run_dir) / ("bot-%s.events" % label), "w")
        err = open(Path(run_dir) / ("bot-%s.stderr" % label), "w")
        exe = binary()
        if not exe.exists():
            raise BotError("no %s (build it with port/bot/build.sh)" % exe)
        args = [str(exe), "--host", host, "--port", str(port), "--user", user, "--pass", password,
                "--char-name", char, "--login-retries", "8", "--set", "echo_chat=off",
                "--set", "echo_whisper=off"]
        self.proc = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=err,
                                     text=True, bufsize=1)
        threading.Thread(target=self._read, daemon=True).start()

    @classmethod
    def start(cls, t, label, user, password, char, gm=False, timeout=60):
        """Log a bot in (creating its account, with group 99 when gm, and its
        character on first use); None with --mock."""
        if t.mock:
            return None
        t.accounts.create_account(user, password, group_id=99 if gm else 0)
        bot = cls(t.run_dir, label, user, password, char)
        t.bots = getattr(t, "bots", []) + [bot]
        bot.entered = bot.wait(lambda e: e.get("event") == "entered", timeout)
        return bot

    def _read(self):
        for line in self.proc.stdout:
            self.log.write(line)
            self.log.flush()
            try:
                event = json.loads(line)
            except ValueError:
                continue
            with self.cond:
                self.events.append(event)
                self.cond.notify_all()
        with self.cond:
            self.events.append({"event": "eof"})
            self.cond.notify_all()

    def mark(self):
        with self.cond:
            return len(self.events)

    def send(self, cmd, **fields):
        fields["cmd"] = cmd
        self.proc.stdin.write(json.dumps(fields) + "\n")
        self.proc.stdin.flush()

    def gm(self, command):
        """An atcommand, typed into the bot's public chat (set-up only)."""
        self.send("gm", text=command)

    def say(self, text):
        self.send("say", text=text)

    def move(self, x, y):
        self.send("move", x=int(x), y=int(y))

    def wait(self, predicate, timeout=10.0, since=0):
        """The first event from index `since` on that satisfies predicate."""
        deadline = time.monotonic() + timeout
        i = since
        with self.cond:
            while True:
                while i < len(self.events):
                    e = self.events[i]
                    i += 1
                    if e.get("event") == "eof":
                        raise BotError("bot %s ended: %r" % (self.label, self.events[-6:]))
                    if predicate(e):
                        return e
                left = deadline - time.monotonic()
                if left <= 0:
                    raise BotError("bot %s: no such event in %.0fs; last: %r" % (self.label, timeout, self.events[-4:]))
                self.cond.wait(left)

    def warp(self, map, x, y, timeout=20.0):
        """@warp the bot (a group-99 account) and wait until it stands there."""
        # Only a position at the cell counts: the bot's first position after
        # entering the map is also reported as warped, and the server may
        # drop an @warp sent before the bot's map has loaded (it is then not
        # in atcommandlog), so the command is sent again.
        for attempt in range(3):
            since = self.mark()
            self.gm("@warp %s %d %d" % (map, x, y))
            try:
                return self.wait(lambda e: e.get("event") == "position" and e.get("map", "").startswith(map)
                                 and (e.get("warped") or e.get("arrived"))
                                 and abs(e.get("x", -9) - x) <= 1 and abs(e.get("y", -9) - y) <= 1,
                                 timeout / 3, since)
            except BotError:
                if attempt == 2:
                    raise

    def quit(self, timeout=10.0):
        if self.proc.poll() is None:
            try:
                self.send("quit")
            except (BrokenPipeError, ValueError):
                pass
            try:
                self.proc.wait(timeout)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        self.log.close()
        return self.proc.returncode


def stop_all(t):
    for bot in getattr(t, "bots", []):
        bot.quit()
    t.bots = []
