"""Launching the client under test.

The binary is BF_CLIENT (a path, or a command line split on spaces), started
with `--control <socket>` and the environment the port's instrumentation reads
(port/README.md, "Instrumentation"): BF_TRACE, BF_TRACE_FILE, BF_PKT_LOG,
BF_CLIENT_DIR, and BF_CONTROL for the socket as well. Its stdout and stderr go
to files in the run directory; its trace and exit code are collected when it
stops, and it is killed when it outlives its timeout.
"""

import os
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
        self.command = list(command) if command else client_command(mock)
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
        env.update(self.extra_env)
        return env

    def start(self):
        if self.control_path.exists():
            self.control_path.unlink()
        argv = self.command + ["--control", str(self.control_path)] + self.args
        self._out = open(self.stdout_file, "wb")
        self.proc = subprocess.Popen(argv, env=self.environment(), stdout=self._out,
                                     stderr=subprocess.STDOUT, cwd=str(self.run_dir),
                                     start_new_session=True)
        self.started = time.monotonic()
        self.argv = argv
        return self

    def connect(self, timeout=60.0):
        """Wait for the socket, connect, say hello. Returns the Control."""
        deadline = time.monotonic() + timeout
        while not self.control_path.exists():
            if self.proc.poll() is not None:
                raise ClientError("client exited with %d before opening its socket"
                                  % self.proc.returncode)
            if time.monotonic() >= deadline:
                raise ClientError("no control socket after %.0fs" % timeout)
            time.sleep(0.05)
        self.ctl = Control(self.control_path, transcript=self.transcript,
                           connect_timeout=max(1.0, deadline - time.monotonic()))
        self.ctl.hello()
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
