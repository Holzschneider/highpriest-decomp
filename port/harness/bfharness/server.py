"""The test server: rAthena and MariaDB from port/server, through docker compose.

Several sessions share one server. `ensure_up` starts it only when it is not
running; `restart` is guarded and meant for the lead after a configuration
change (port/server/import), never for a test script.
"""

import json
import os
import socket
import subprocess
import time

from . import paths

ONLINE_MARKER = "Map Server is now online"
PORTS = {"login": 6900, "char": 6121, "map": 5121}
RESTART_ENV = "BF_SERVER_RESTART"


class ServerError(RuntimeError):
    pass


class Server:
    def __init__(self, directory=None, host="127.0.0.1"):
        self.dir = paths.server_dir() if directory is None else directory
        self.host = host
        self.ports = {k: int(os.environ.get("BF_%s_PORT" % k.upper(), v)) for k, v in PORTS.items()}

    # -- docker compose ------------------------------------------------------

    def compose(self, *args, input=None, timeout=120, check=True):
        cmd = ["docker", "compose", "-f", str(self.dir / "docker-compose.yml")] + list(args)
        r = subprocess.run(cmd, cwd=str(self.dir), input=input, capture_output=True,
                           text=True, errors="replace", timeout=timeout)   # the server logs Windows-1252 names
        if check and r.returncode != 0:
            raise ServerError("%s: exit %d: %s" % (" ".join(cmd[3:]), r.returncode, r.stderr.strip()))
        return r

    def status(self):
        """{service: {"state": ..., "health": ..., "name": ...}} for db and rathena."""
        r = self.compose("ps", "--all", "--format", "json", check=False)
        out = r.stdout.strip()
        items = []
        if out.startswith("["):
            items = json.loads(out)
        else:
            items = [json.loads(line) for line in out.splitlines() if line.strip()]
        return {i.get("Service"): {"state": i.get("State"), "health": i.get("Health"),
                                   "name": i.get("Name")} for i in items}

    def started_at(self):
        """Start time of the rathena container (ISO string), or None."""
        st = self.status().get("rathena")
        if not st or st["state"] != "running":
            return None
        r = subprocess.run(["docker", "inspect", "-f", "{{.State.StartedAt}}", st["name"]],
                           capture_output=True, text=True)
        return r.stdout.strip() or None

    def logs(self, since=None, tail=None):
        args = ["logs", "--no-color"]
        if since:
            args += ["--since", since]
        if tail:
            args += ["--tail", str(tail)]
        return self.compose(*(args + ["rathena"]), check=False).stdout

    def ports_open(self):
        for port in self.ports.values():
            try:
                with socket.create_connection((self.host, port), timeout=1):
                    pass
            except OSError:
                return False
        return True

    def online(self):
        """Running, the map server reported online since the container started,
        and the three ports accept connections."""
        since = self.started_at()
        if not since:
            return False
        return ONLINE_MARKER in self.logs(since=since) and self.ports_open()

    def wait_online(self, timeout=300):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.online():
                return True
            time.sleep(2)
        raise ServerError("map server not online after %ds" % timeout)

    def ensure_up(self, timeout=300):
        """Start the server if it is not running; wait until it is online.
        Returns "running" if it already was, "started" otherwise."""
        if self.online():
            return "running"
        st = self.status().get("rathena")
        if not st or st["state"] != "running":
            self.compose("up", "-d", timeout=900)
        self.wait_online(timeout)
        return "started"

    def restart(self, allow=False, timeout=300):
        """Restart the rathena service (not the database), so that a changed
        configuration in port/server/import takes effect. Every connected
        client is dropped, other sessions included: only the lead calls this,
        with allow=True or BF_SERVER_RESTART=1 in the environment."""
        if not (allow or os.environ.get(RESTART_ENV) == "1"):
            raise ServerError("restart refused: shared server; pass allow=True or set %s=1"
                              % RESTART_ENV)
        self.compose("restart", "rathena", timeout=300)
        time.sleep(2)
        self.wait_online(timeout)
