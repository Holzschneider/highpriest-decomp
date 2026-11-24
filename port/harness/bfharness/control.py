"""Client side of the control protocol (port/native/control/PROTOCOL.md)."""

import json
import socket
import threading
import time

from . import PROTOCOL_VERSION


class ControlError(RuntimeError):
    def __init__(self, code, message="", reply=None):
        RuntimeError.__init__(self, "%s: %s" % (code, message))
        self.code = code
        self.reply = reply or {}


class Control:
    """One connection to the client's control socket.

    A reader thread takes every line off the socket: replies are matched to
    requests by id, events are kept in order (`events`) and can be waited for
    on this side (`wait_event`) as well as through the client's `wait`.
    Every line in both directions is appended to `transcript` (a file path),
    which the runner keeps as evidence."""

    def __init__(self, path, transcript=None, connect_timeout=30.0):
        self.path = str(path)
        self.transcript = open(transcript, "a", encoding="utf-8") if transcript else None
        self.sock = None
        self._next = 1
        self._replies = {}
        self._cond = threading.Condition()
        self.events = []
        self.closed = False
        self.last_seq = 0
        self.info = None
        self._connect(connect_timeout)
        self._reader = threading.Thread(target=self._read, name="control-reader", daemon=True)
        self._reader.start()

    def _connect(self, timeout):
        deadline = time.monotonic() + timeout
        while True:
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                s.connect(self.path)
                self.sock = s
                return
            except OSError as e:
                s.close()
                if time.monotonic() >= deadline:
                    raise ControlError("connect", "%s: %s" % (self.path, e))
                time.sleep(0.1)

    def _log(self, direction, line):
        if self.transcript:
            self.transcript.write("%.3f %s %s\n" % (time.time(), direction, line))
            self.transcript.flush()

    def _read(self):
        buf = b""
        try:
            while True:
                chunk = self.sock.recv(65536)
                if not chunk:
                    break
                buf += chunk
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    if line.strip():
                        self._dispatch(line.decode("utf-8", "replace"))
        except OSError:
            pass
        with self._cond:
            self.closed = True
            self._cond.notify_all()

    def _dispatch(self, line):
        self._log("<", line)
        try:
            msg = json.loads(line)
        except ValueError:
            return
        with self._cond:
            if "ok" not in msg and "event" in msg:
                self.events.append(msg)
                self.last_seq = max(self.last_seq, msg.get("seq", 0))
            else:
                self._replies[msg.get("id")] = msg
            self._cond.notify_all()

    # -- requests ------------------------------------------------------------

    def request(self, cmd, _timeout=30.0, **args):
        """Send a request, wait for its reply, return it; raise ControlError
        for an error reply, a closed connection or no reply in time."""
        with self._cond:
            rid = self._next
            self._next += 1
        msg = dict(args)
        msg["id"] = rid
        msg["cmd"] = cmd
        line = json.dumps(msg, ensure_ascii=False)
        self._log(">", line)
        try:
            self.sock.sendall(line.encode("utf-8") + b"\n")
        except OSError as e:
            raise ControlError("closed", str(e))
        timeout = _timeout
        deadline = time.monotonic() + timeout
        with self._cond:
            while rid not in self._replies:
                if self.closed:
                    raise ControlError("closed", "connection closed before reply to %s" % cmd)
                left = deadline - time.monotonic()
                if left <= 0:
                    raise ControlError("no_reply", "%s: no reply in %.0fs" % (cmd, timeout))
                self._cond.wait(left)
            reply = self._replies.pop(rid)
        if not reply.get("ok"):
            err = reply.get("error") or {}
            raise ControlError(err.get("code", "error"), err.get("message", ""), reply)
        return reply

    def send_raw(self, line):
        """Send a line as it is (protocol tests)."""
        self._log(">", line)
        self.sock.sendall(line.encode("utf-8") + b"\n")

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass
        if self.transcript:
            self.transcript.close()
            self.transcript = None

    # -- commands ------------------------------------------------------------

    def hello(self):
        r = self.request("hello", protocol=PROTOCOL_VERSION, name="bfharness")
        if r.get("protocol") != PROTOCOL_VERSION:
            raise ControlError("protocol", "client speaks version %r, harness %d"
                               % (r.get("protocol"), PROTOCOL_VERSION))
        self.info = r
        return r

    def key(self, action, key=None, mods=None, text=None):
        args = {"action": action}
        if key is not None:
            args["key"] = key
        if mods:
            args["mods"] = list(mods)
        if text is not None:
            args["text"] = text
        return self.request("key", **args)

    def press(self, key, *mods):
        return self.key("press", key, mods)

    def type_text(self, text):
        return self.key("text", text=text)

    def say(self, text):
        """Type a chat line (or an atcommand) and send it with Enter."""
        self.type_text(text)
        return self.press("enter")

    def pointer(self, action, x=None, y=None, button=None, dy=None):
        args = {"action": action}
        if x is not None:
            args["x"], args["y"] = int(x), int(y)
        if button:
            args["button"] = button
        if dy is not None:
            args["dy"] = int(dy)
        return self.request("pointer", **args)

    def move(self, x, y):
        return self.pointer("move", x, y)

    def click(self, x=None, y=None, button="left"):
        return self.pointer("click", x, y, button)

    def double_click(self, x=None, y=None):
        return self.pointer("doubleclick", x, y)

    def wheel(self, dy, x=None, y=None):
        return self.pointer("wheel", x, y, dy=dy)

    def drag(self, x0, y0, x1, y1, button="right", steps=8):
        self.pointer("down", x0, y0, button)
        for i in range(1, steps + 1):
            self.move(x0 + (x1 - x0) * i // steps, y0 + (y1 - y0) * i // steps)
        return self.pointer("up", x1, y1, button)

    def capture(self, path, timeout=30.0):
        return self.request("capture", _timeout=timeout, path=str(path))

    def state(self, *fields):
        args = {"fields": list(fields)} if fields else {}
        return self.request("state", **args)

    def wait(self, until, timeout=10.0, after=None):
        args = {"until": until, "timeout": int(timeout * 1000)}
        if after is not None:
            args["after"] = after
        return self.request("wait", _timeout=timeout + 10, **args)

    def wait_mode(self, mode, timeout=30.0):
        return self.wait({"mode": mode}, timeout)

    def wait_map(self, name, timeout=60.0):
        return self.wait({"map": name}, timeout)

    def wait_window(self, name, open=True, timeout=10.0):
        return self.wait({"window": name, "open": open}, timeout)

    def wait_arrival(self, timeout=30.0):
        return self.wait({"moving": False}, timeout)

    def resize(self, width=None, height=None, scale=None, fullscreen=None):
        args = {}
        for k, v in (("width", width), ("height", height), ("scale", scale), ("fullscreen", fullscreen)):
            if v is not None:
                args[k] = v
        return self.request("resize", **args)

    def quit(self):
        return self.request("quit")

    # -- events, on this side ------------------------------------------------

    def wait_event(self, name, pred=None, after=0, timeout=10.0):
        """The first event `name` with seq > after for which pred(event) holds."""
        deadline = time.monotonic() + timeout
        with self._cond:
            while True:
                for e in self.events:
                    if e.get("event") == name and e.get("seq", 0) > after and (pred is None or pred(e)):
                        return e
                left = deadline - time.monotonic()
                if self.closed or left <= 0:
                    raise ControlError("timeout", "no %s event in %.0fs" % (name, timeout))
                self._cond.wait(left)

    def events_named(self, name, after=0):
        with self._cond:
            return [e for e in self.events if e.get("event") == name and e.get("seq", 0) > after]

    # -- finding things in state --------------------------------------------

    def window(self, name, state=None):
        st = state or self.state("windows")
        for w in st["windows"]:
            if w["name"] == name:
                return w
        return None

    def control_rect(self, window, control_id, state=None):
        w = self.window(window, state)
        if not w:
            raise ControlError("not_found", "window %s not open" % window)
        for c in w.get("controls", []):
            if c["id"] == control_id:
                return c["rect"]
        raise ControlError("not_found", "no control %s in %s" % (control_id, window))

    def click_control(self, window, control_id):
        x, y, w, h = self.control_rect(window, control_id)
        return self.click(x + w // 2, y + h // 2)

    def actor(self, pred, state=None, learn_names=True):
        """The first actor of `state` (default: a fresh one) that pred accepts.
        When none does and some on-screen actors' names are still "?name"
        (the 2008 client asks the server for a name only when the pointer
        rests on the actor, PROTOCOL.md `actors`), the pointer rests on each
        of them once and the actors are read again."""
        st = state or self.state("actors")
        for a in st["actors"]:
            if pred(a):
                return a
        unknown = [a for a in st["actors"] if a.get("name") == "?name" and a.get("screen")]
        if not learn_names or not unknown:
            return None
        for a in unknown:
            self.move(a["screen"]["x"], a["screen"]["y"])
            self.wait({"frames": 10}, timeout=10.0)
        return self.actor(pred, learn_names=False)
