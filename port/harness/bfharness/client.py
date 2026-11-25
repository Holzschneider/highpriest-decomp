"""Launching the client under test.

The binary is BF_CLIENT (a path, or a command line split on spaces), started
with `--control <socket>` and the environment the port's instrumentation reads
(port/README.md, "Instrumentation"): BF_TRACE, BF_TRACE_FILE, BF_PKT_LOG,
BF_CLIENT_DIR, and BF_CONTROL for the socket as well. Its stdout and stderr go
to files in the run directory; its trace and exit code are collected when it
stops, and it is killed when it outlives its timeout.
"""

import os
import plistlib
import re
import shlex
import signal
import subprocess
import sys
import time
from pathlib import Path

from . import paths
from .control import Control, ControlError

DEFAULT_TRACE = "err,net,mode,stub"
MOCK = paths.HARNESS_DIR / "mock_client.py"


class ClientError(RuntimeError):
    pass


def app_bundle():
    """The bundle BF_APP names, or None (the bare binary of BF_CLIENT)."""
    env = os.environ.get("BF_APP")
    if not env:
        return None
    app = Path(env).expanduser().resolve()
    if not (app / "Contents" / "Info.plist").is_file():
        raise ClientError("BF_APP=%s is not an application bundle" % env)
    return app


def app_executable(app):
    with open(app / "Contents" / "Info.plist", "rb") as f:
        info = plistlib.load(f)
    return app / "Contents" / "MacOS" / info["CFBundleExecutable"]


# Variables taken from the harness's own environment into a bundle launch,
# besides those the harness sets: the port's, and the sanitizers' options.
APP_ENV_PREFIXES = ("BF_", "ASAN_", "UBSAN_", "TSAN_", "LSAN_")
# What `open` itself runs with: launchd gives the app the rest, as for Finder.
FINDER_ENV = ("HOME", "USER", "LOGNAME")
GONE = "gone without its exit line"   # poll() of a client that ended unexpectedly
EXIT_LINE = re.compile(r"\] exit: WinMain returned (-?\d+)")


def running_pids(executable):
    """Pids of the processes whose executable is `executable` (ps's comm
    column is the full path LaunchServices started)."""
    out = subprocess.run(["ps", "-axww", "-o", "pid=,comm="], capture_output=True,
                         text=True).stdout
    pids = set()
    for line in out.splitlines():
        pid, _, comm = line.strip().partition(" ")
        if comm.strip() == str(executable):
            pids.add(int(pid))
    return pids


class AppProcess:
    """The client started by LaunchServices, in the shape of the Popen the
    rest of the harness uses (pid, poll, wait, returncode). It is launchd's
    child, not ours, and `open -W` cannot wait for it on this macOS
    ("Unable to block on application"), so:
    - pid: the new process running the bundle's executable after `open`
      returned, confirmed by the pid the control socket's hello reports;
    - liveness: kill(pid, 0);
    - exit status: the trace line `exit: WinMain returned N` that the client
      writes on every orderly exit (category mode, which a bundle launch
      always traces); a process that is gone without it has status None,
      one the harness killed -SIGTERM or -SIGKILL, as Popen reports them."""

    def __init__(self, executable, trace_file, before):
        self.executable = executable
        self.trace_file = trace_file
        self.pid = None
        self.returncode = None
        self._signal = None
        self._gone = False
        self._before = before

    def find(self, timeout=30.0):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            new = running_pids(self.executable) - self._before
            if len(new) == 1:
                self.pid = new.pop()
                return self.pid
            if len(new) > 1:   # another launch of the same bundle at once: the socket decides
                return None
            time.sleep(0.1)
        raise ClientError("no process of %s after %.0fs" % (self.executable, timeout))

    def _alive(self):
        try:
            os.kill(self.pid, 0)
            return True
        except ProcessLookupError:
            return False
        except PermissionError:
            return True

    def poll(self):
        if self._gone:
            return GONE
        if self.returncode is not None or self.pid is None:
            return self.returncode
        if self._alive():
            return None
        if self._signal is not None:
            self.returncode = -self._signal
        else:
            text = self.trace_file.read_text(errors="replace") if self.trace_file.exists() else ""
            found = EXIT_LINE.findall(text)
            if not found:
                self._gone = True   # ended without its exit line (a crash): status unknown
                return GONE
            self.returncode = int(found[-1])
        return self.returncode

    def wait(self, timeout=None):
        deadline = None if timeout is None else time.monotonic() + timeout
        while self.poll() is None:
            if deadline is not None and time.monotonic() >= deadline:
                raise subprocess.TimeoutExpired(str(self.executable), timeout)
            time.sleep(0.05)
        return self.returncode   # None when GONE

    def send_signal(self, sig):
        if self.pid is None or self.poll() is not None:
            return
        self._signal = sig
        try:
            os.kill(self.pid, sig)
        except ProcessLookupError:
            pass


def client_command(mock=False):
    if mock:
        return [sys.executable, str(MOCK)]
    env = os.environ.get("BF_CLIENT")
    if not env:
        raise ClientError("BF_CLIENT is not set: name the client binary to test")
    return shlex.split(env)


class Client:
    def __init__(self, run_dir, command=None, args=(), env=None, trace=None, mock=False,
                 timeout=600.0):
        self.run_dir = Path(run_dir)
        self.run_dir.mkdir(parents=True, exist_ok=True)
        self.app = None if (mock or command) else app_bundle()
        self.command = list(command) if command else ([str(self.app)] if self.app else client_command(mock))
        self.args = list(args)
        self.extra_env = dict(env or {})
        self.trace_cats = trace or os.environ.get("BF_TRACE", DEFAULT_TRACE)
        self.timeout = timeout
        self.control_path = paths.socket_path("ctl")
        self.trace_file = self.run_dir / "client-trace.txt"
        self.pkt_log = self.run_dir / "client-packets.txt"
        self.stdout_file = self.run_dir / "client-stdout.txt"
        self.transcript = self.run_dir / "control-transcript.txt"
        self.proc = None
        self.ctl = None
        self.started = None
        self.exit_code = None
        self.killed = False

    def environment(self):
        env = dict(os.environ)
        env.update({
            "BF_TRACE": self.trace_cats,
            "BF_TRACE_FILE": str(self.trace_file),
            "BF_PKT_LOG": str(self.pkt_log),
            "BF_CLIENT_DIR": str(paths.client_dir()),
            "BF_CONTROL": str(self.control_path),
        })
        # Open on a Retina screen when there is one (port/native/app/README.md), so
        # frames come out at the same scale wherever the key window happens to be.
        env.setdefault("BF_WINDOW_SCREEN", "retina")
        env.update(self.extra_env)
        return env

    def app_environment(self):
        """What a bundle launch carries with `open --env`: the harness's
        variables, the script's, and the BF_ and sanitizer variables of the
        harness's own environment. The game folder goes into the settings of
        the launch's BF_HOME instead of BF_CLIENT_DIR (module docstring)."""
        env = self.environment()
        keep = {k: v for k, v in env.items()
                if k in self.extra_env or k.startswith(APP_ENV_PREFIXES)}
        if "mode" not in keep["BF_TRACE"].split(",") and "all" not in keep["BF_TRACE"].split(","):
            keep["BF_TRACE"] += ",mode"   # the exit line AppProcess reads
        home = keep.get("BF_HOME")
        if "BF_CLIENT_DIR" not in self.extra_env and home:
            del keep["BF_CLIENT_DIR"]
            self.game_folder = self.store_game_folder(Path(home))
        else:
            self.game_folder = Path(keep["BF_CLIENT_DIR"])
        return keep

    def store_game_folder(self, home):
        """Paths/GameFolder in the Settings.plist of `home`, as the chooser
        stores it (port/native/prefs); a value already there is kept."""
        folder = home / "Library" / "Application Support" / paths.app_name()
        folder.mkdir(parents=True, exist_ok=True)
        plist = folder / "Settings.plist"
        try:
            with open(plist, "rb") as f:
                values = plistlib.load(f)
        except (OSError, plistlib.InvalidFileException):
            values = {"FormatVersion": 1}
        group = next((k for k in values if k.lower() == "paths" and isinstance(values[k], dict)), "Paths")
        values.setdefault(group, {})
        stored = next((v for k, v in values[group].items() if k.lower() == "gamefolder"), None)
        if stored:
            return Path(stored)
        chosen = self.chosen_folder()
        values[group]["GameFolder"] = str(chosen)
        with open(plist, "wb") as f:
            plistlib.dump(values, f)
        return chosen

    def chosen_folder(self):
        """BF_APP_FOLDER: a path, `links` (a folder of links to the game
        folder's entries, in the run directory), or by default the game folder."""
        want = os.environ.get("BF_APP_FOLDER", "")
        game = paths.client_dir()
        if want == "links":
            links = self.run_dir / "game"
            links.mkdir(exist_ok=True)
            for entry in game.iterdir():
                link = links / entry.name
                if not link.is_symlink() and not link.exists():
                    link.symlink_to(entry)
            return links
        return Path(want).expanduser() if want else game

    def start(self):
        if self.control_path.exists():
            self.control_path.unlink()
        if self.app:
            return self.start_app()
        argv = self.command + ["--control", str(self.control_path)] + self.args
        self._out = open(self.stdout_file, "wb")
        self.proc = subprocess.Popen(argv, env=self.environment(), stdout=self._out,
                                     stderr=subprocess.STDOUT, cwd=str(self.run_dir),
                                     start_new_session=True)
        self.started = time.monotonic()
        self.argv = argv
        return self

    def start_app(self):
        """The bundle through LaunchServices (module docstring)."""
        env = self.app_environment()
        exe = app_executable(self.app)
        argv = ["open", "-n", "--stdout", str(self.stdout_file), "--stderr", str(self.stdout_file)]
        for k in sorted(env):
            argv += ["--env", "%s=%s" % (k, env[k])]
        argv += [str(self.app), "--args", "--control", str(self.control_path)] + self.args
        self.stdout_file.write_bytes(b"")
        if self.trace_file.exists():   # AppProcess reads the exit line of this launch only
            self.trace_file.unlink()
        self._out = open(self.run_dir / "open-stdout.txt", "wb")
        before = running_pids(exe)
        finder = {k: os.environ[k] for k in FINDER_ENV if k in os.environ}
        r = subprocess.run(argv, env=finder, stdout=self._out, stderr=subprocess.STDOUT, cwd="/")
        self.started = time.monotonic()
        self.argv = argv
        with open(self.run_dir / "launch.txt", "a") as f:   # evidence: how it was launched
            f.write("cd / && env -i %s %s\n" % (" ".join("%s=%s" % (k, shlex.quote(v)) for k, v in finder.items()),
                                             shlex.join(argv)))
        if r.returncode != 0:
            raise ClientError("open %s exited with %d" % (self.app, r.returncode))
        self.proc = AppProcess(exe, self.trace_file, before)
        self.proc.find()
        return self

    def connect(self, timeout=60.0):
        """Wait for the socket, connect, say hello. Returns the Control."""
        deadline = time.monotonic() + timeout
        while not self.control_path.exists():
            if self.proc.poll() is not None:
                raise ClientError("client exited with %s before opening its socket"
                                  % self.proc.returncode)
            if time.monotonic() >= deadline:
                raise ClientError("no control socket after %.0fs" % timeout)
            time.sleep(0.05)
        self.ctl = Control(self.control_path, transcript=self.transcript,
                           connect_timeout=max(1.0, deadline - time.monotonic()))
        info = self.ctl.hello()
        if isinstance(self.proc, AppProcess) and info.get("pid"):
            self.proc.pid = info["pid"]   # the process that answers is the one launched
        return self.ctl

    def overdue(self):
        return self.started is not None and time.monotonic() - self.started > self.timeout

    def stop(self, grace=15.0):
        """Ask the client to quit, wait for it, kill it if it does not exit.
        Returns the exit code (negative: killed by that signal)."""
        if self.proc is None:
            return None
        if self.proc.poll() is None and self.ctl and not self.ctl.closed:
            try:
                self.ctl.quit()
            except ControlError:
                pass
        try:
            self.proc.wait(timeout=grace)
        except subprocess.TimeoutExpired:
            self.kill()
        if self.ctl:
            self.ctl.close()
        self.exit_code = self.proc.returncode
        self._out.close()
        if self.control_path.exists():
            try:
                self.control_path.unlink()
            except OSError:
                pass
        return self.exit_code

    def kill(self):
        self.killed = True
        if isinstance(self.proc, AppProcess):
            self.proc.send_signal(signal.SIGTERM)
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.send_signal(signal.SIGKILL)
                self.proc.wait(timeout=5)
            return
        try:
            os.killpg(self.proc.pid, signal.SIGTERM)
            self.proc.wait(timeout=5)
        except (ProcessLookupError, subprocess.TimeoutExpired):
            try:
                os.killpg(self.proc.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            self.proc.wait()

    def trace(self):
        return self.trace_file.read_text(errors="replace") if self.trace_file.exists() else ""

    def trace_lines(self, *needles):
        """Trace lines containing any of the needles (all lines without needles)."""
        lines = self.trace().splitlines()
        if not needles:
            return lines
        return [l for l in lines if any(n in l for n in needles)]

    def packets(self):
        return self.pkt_log.read_text(errors="replace").splitlines() if self.pkt_log.exists() else []
