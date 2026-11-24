"""Captured frames and their checksum manifest.

Frames that may show game artwork never enter git (CLAUDE.md, Fences): they
go to ~/Games/<name>/test/frames/<script>/<name>.png (or BF_FRAMES_DIR). The
repository keeps a manifest of their checksums at a path the script names,
one line per frame: `<sha256>  <width>x<height>  <name>.png`.
"""

import hashlib
import re
from pathlib import Path

from . import paths


class FrameError(RuntimeError):
    pass


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def png_size(path):
    with open(path, "rb") as f:
        head = f.read(24)
    if head[:8] != b"\x89PNG\r\n\x1a\n" or head[12:16] != b"IHDR":
        raise FrameError("%s is not a PNG" % path)
    return int.from_bytes(head[16:20], "big"), int.from_bytes(head[20:24], "big")


class Frames:
    def __init__(self, script, root=None, allow_in_repo=False):
        if not re.fullmatch(r"[A-Za-z0-9_.-]+", script):
            raise FrameError("bad script name %r" % script)
        self.dir = Path(root or paths.frames_root()) / script
        if paths.inside_repo(self.dir) and not allow_in_repo:
            # only the harness's own synthetic frames (mock runs, under build/) may live there
            raise FrameError("frames directory %s is inside the repository" % self.dir)
        self.dir.mkdir(parents=True, exist_ok=True)
        self.records = []

    def path(self, name):
        if not re.fullmatch(r"[A-Za-z0-9_.-]+", name):
            raise FrameError("bad frame name %r" % name)
        return self.dir / (name + ".png")

    def capture(self, ctl, name):
        """Capture a frame through the control socket and check what the client
        reported against the file it wrote."""
        p = self.path(name)
        r = ctl.capture(p)
        if not p.exists():
            raise FrameError("client reported %s but wrote nothing" % p)
        digest = sha256(p)
        w, h = png_size(p)
        if r.get("sha256") != digest:
            raise FrameError("%s: client says sha256 %s, file has %s" % (name, r.get("sha256"), digest))
        if (r.get("width"), r.get("height")) != (w, h):
            raise FrameError("%s: client says %sx%s, file is %dx%d"
                             % (name, r.get("width"), r.get("height"), w, h))
        rec = {"name": name, "path": str(p), "sha256": digest, "width": w, "height": h,
               "scale": r.get("scale"), "logical": r.get("logical"), "frame": r.get("frame")}
        self.records.append(rec)
        return rec

    def write_manifest(self, repo_path):
        """Write the checksum manifest into the repository (text only)."""
        target = Path(repo_path)
        if not target.is_absolute():
            target = paths.REPO / target
        if target.suffix.lower() in (".png", ".bmp", ".jpg", ".jpeg", ".tga"):
            raise FrameError("a manifest is text, not %s" % target.suffix)
        target.parent.mkdir(parents=True, exist_ok=True)
        lines = ["# frames under %s/; regenerate with port/harness/run.py" % _display(self.dir)]
        for r in sorted(self.records, key=lambda r: r["name"]):
            lines.append("%s  %dx%d  %s.png" % (r["sha256"], r["width"], r["height"], r["name"]))
        target.write_text("\n".join(lines) + "\n")
        return target


def _display(p):
    """The frames directory as it is written into git: relative to the game
    folder or the home directory, so no user name or application name is
    spelled out in the repository."""
    for base, label in ((paths.client_dir(), "<game folder>"), (paths.REPO, "<repo>"), (Path.home(), "~")):
        try:
            return "%s/%s" % (label, Path(p).resolve().relative_to(Path(base).resolve()))
        except ValueError:
            pass
    return str(p)
