"""Where things are: the repository, the application name, the game folder,
the test server and the harness's own output."""

import os
import re
import subprocess
import tempfile
from pathlib import Path

HARNESS_DIR = Path(__file__).resolve().parent.parent      # port/harness
REPO = HARNESS_DIR.parent.parent                           # the checkout this file is in


def app_name():
    """The application name, from port/name.cmake (the one place it is defined)."""
    text = (REPO / "port" / "name.cmake").read_text()
    m = re.search(r'set\(BF_APP_NAME\s+"([^"]+)"\)', text)
    if not m:
        raise RuntimeError("BF_APP_NAME not found in port/name.cmake")
    return m.group(1)


def client_dir():
    """The game folder: BF_CLIENT_DIR, by default ~/Games/<name> (spec 5)."""
    env = os.environ.get("BF_CLIENT_DIR")
    return Path(env).expanduser() if env else Path.home() / "Games" / app_name()


def frames_root():
    """Captured frames that may show game artwork: outside git (CLAUDE.md, Fences)."""
    env = os.environ.get("BF_FRAMES_DIR")
    return Path(env).expanduser() if env else client_dir() / "test" / "frames"


def build_dir():
    """The harness's own output, in the checkout's ignored build directory."""
    return REPO / "build" / "harness"


def main_checkout():
    """The main worktree of this repository (worktrees share its .git)."""
    try:
        out = subprocess.run(
            ["git", "-C", str(REPO), "rev-parse", "--path-format=absolute", "--git-common-dir"],
            capture_output=True, text=True, check=True).stdout.strip()
        return Path(out).parent
    except (OSError, subprocess.CalledProcessError):
        return REPO


def server_dir():
    """The compose project of the test server: BF_SERVER_DIR, else port/server of
    the main checkout. A worktree must not `up` its own copy, since the compose
    project name is fixed and the mounts would point elsewhere."""
    env = os.environ.get("BF_SERVER_DIR")
    return Path(env).expanduser() if env else main_checkout() / "port" / "server"


SOCKET_PATH_MAX = 103


def socket_path(tag):
    """A path for the control socket. macOS limits socket paths to 103 bytes;
    when the build directory is too deep, the system temporary directory is
    used instead."""
    name = "%s-%d.sock" % (tag, os.getpid())
    p = build_dir() / "sock" / name
    if len(str(p)) <= SOCKET_PATH_MAX:
        p.parent.mkdir(parents=True, exist_ok=True)
        return p
    return Path(tempfile.gettempdir()) / ("bf-" + name)


def inside_repo(path):
    try:
        Path(path).resolve().relative_to(REPO.resolve())
        return True
    except ValueError:
        return False
