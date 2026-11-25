#!/usr/bin/env python3
"""Reference frames and their comparison (M11.0c).

M11 replaces the Windows-shaped adapters under port/compat with native calls,
one subsystem at a time. After each step the reference frames must still
match. This tool builds the reference from captures at the base commit and
compares a later capture against it. Standard library only.

    framecmp.py reference --out <ref dir> [--commit <sha>] <frames dir> <frames dir> ...
    framecmp.py compare <ref dir> <frames dir> [--out <diff dir>] [--only name,name]

A frames directory holds what port/harness/scripts/m11_frames.py captured:
per frame `<name>.png` and its samples `<name>.s1.png`, `<name>.s2.png`
(the same scene again, 400 ms and 800 ms later).

What "match" means (the definition, written before the reference was taken):

1. Reproducible where the client is. The scenes are replayed from a
   recorded packet log (no server, no other player, the same packets every
   time), the camera does not ease and the weather is off (m11_frames.py),
   so most pixels of every frame are byte-identical from run to run at one
   commit. Outside game mode (login, server list, selection, creation) the
   whole frame is; in game mode the UI windows are, and the world nearly is.
2. Masks only where the picture moves by itself. The reference is built from
   several runs at the base commit (m11_reference.py: four), three captures
   per frame each (the primary and its two samples, 400 ms apart, together
   spanning more than the 500 ms half-period of the edit caret). A frame's
   mask is every pixel in which any of these captures differs from another,
   by any amount, widened by 3 device pixels. Added to it: the rectangles the capturing script
   names in <name>.json with a reason (one so far: the date and time
   CMode::ScreenShot stamps on the read-back, which changes every second
   and every day). What the masks hold, read
   from the reference's masks: the caret of the focused edit field
   (UIEditCtrl::OnDraw blinks it on timeGetTime()/500), the animated game
   cursor and the cell marker under it, animated models and sprites, water,
   effects (chimney smoke, the potion), the read-back's stamp;
   reference.json lists each frame's masked share.
3. Regions. A frame's capture records the mode and the open UI windows
   (control socket `state`, a <name>.json beside the PNG). Strict region:
   the whole frame outside game mode, the windows' rectangles in game mode.
   World region: the rest of a game-mode frame (ground, models, sprites,
   effects, name and chat balloons, the cursor over the world).
4. Outside the mask, in the strict region: every pixel within 2 per channel
   (0-255) of the reference, except at most 16 isolated pixels (none of
   their 8 neighbours beyond the tolerance): the chat window is
   translucent, and an animated world behind it (water at a shore) can
   show a phase at a mask's edge that the reference did not see. A correct
   change may round a blend differently by one step; a moved, filtered,
   re-keyed or re-blended UI changes many pixels by far more, and one wrong
   glyph changes a connected stroke.
5. Outside the mask, in the world region: at most 2 % of the region's pixels
   beyond that tolerance. The world is not fully reproducible at the base
   commit: isolated polygon-edge pixels and random smoke puffs past the mask
   differ, and some lit patches come out in one of two variants per run
   (prt_in's floor: 0.75 % of the frame, 1.3 % of its world region; not
   from uninitialised locals, a build with -ftrivial-auto-var-init=pattern
   shows the same two variants). Four reference runs all show the same
   variant with a chance of 1 in 8; the 2 % covers that case. A shifted
   world changes 40-65 % of the region (the planted half-pixel fault).
6. Inside the mask, per 32x32 device-pixel tile holding at least 32 masked
   pixels: the mean colour of the masked pixels within the range of the
   reference captures' means of that tile, widened by 24 per channel (an
   animated thing that vanishes, turns black or magenta, or is blended
   wrongly). Judged for tiles wholly in the strict region; counted and
   reported only for the others (random particles in the world, also behind
   a window's edge, move tile means by more).
   The pointer box (48x56 logical pixels from 8 above and left of the
   pointer the capture records) is reported, not judged, by points 4-6:
   the cursor animates on the clock in every mode, and the captures do not
   see every phase of it. A cursor drawn anywhere else than in that box
   (wrong position, wrong scale) still fails points 4 or 5.
7. Anywhere: no pixel of placeholder magenta (R and B at least 240, G at
   most 16) where the reference has none.
8. Same size in device pixels, or the frame fails. A frame whose mask
   covers more than 10 % of it is reported as weak (it still has to match).

Output of `compare`: one line per frame (PASS/FAIL, pixels beyond the
tolerance outside the mask, tiles out of range inside it, masked share),
`verdict.json`, and `<name>.diff.png` per frame: the new capture darkened,
the mask tinted blue, pixels beyond the tolerance red, failing tiles
outlined in yellow. Exit status 0 when every reference frame matches.
"""

import argparse
import datetime
import json
import os
import re
import struct
import sys
import zlib
from pathlib import Path

TOLERANCE = 2          # per channel, outside the mask
DILATE = 3             # device pixels the mask is widened by
TILE = 32              # device pixels, the tile of the masked-region check
TILE_MIN = 32          # masked pixels a tile needs to be checked
TILE_MARGIN = 24       # per channel, around the reference's range of tile means
WORLD_SHARE = 0.02     # share of the world region allowed beyond the tolerance
ISOLATED = 16          # isolated pixels allowed beyond the tolerance in the strict region
WEAK_SHARE = 0.10      # mask share above which a frame is reported as weak
SAMPLE_RE = re.compile(r"^(?P<name>[A-Za-z0-9_-]+)(?:\.s(?P<k>\d+))?\.png$")


# ---- PNG and BMP --------------------------------------------------------------

def read_png(path):
    """(width, height, rows): rows are bytes of RGB, 3 per pixel."""
    data = Path(path).read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("%s is not a PNG" % path)
    pos, idat, w = 8, [], None
    while pos < len(data):
        n, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        pos += 12 + n
        if kind == b"IHDR":
            w, h, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body[:13])
        elif kind == b"IDAT":
            idat.append(body)
    if w is None or depth != 8 or ctype not in (0, 2, 6) or interlace:
        raise ValueError("%s: unsupported PNG" % path)
    bpp = {0: 1, 2: 3, 6: 4}[ctype]
    raw = zlib.decompress(b"".join(idat))
    stride = w * bpp
    rows, prev, p = [], bytes(stride), 0
    for _ in range(h):
        f = raw[p]
        line = raw[p + 1:p + 1 + stride]
        p += 1 + stride
        if f == 0:
            out = line
        elif f == 2:
            out = _add_bytes(line, prev)
        else:
            cur = bytearray(line)
            if f == 1:
                for x in range(bpp, stride):
                    cur[x] = (cur[x] + cur[x - bpp]) & 255
            elif f == 3:
                for x in range(stride):
                    a = cur[x - bpp] if x >= bpp else 0
                    cur[x] = (cur[x] + ((a + prev[x]) >> 1)) & 255
            elif f == 4:
                for x in range(stride):
                    a = cur[x - bpp] if x >= bpp else 0
                    b = prev[x]
                    c = prev[x - bpp] if x >= bpp else 0
                    pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                    cur[x] = (cur[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
            out = bytes(cur)
        rows.append(out)
        prev = out
    if bpp == 3:
        return w, h, rows
    if bpp == 4:
        return w, h, [_drop_alpha(r) for r in rows]
    return w, h, [bytes(v for g in r for v in (g, g, g)) for r in rows]


def _add_bytes(a, b):
    """Byte-wise (a + b) mod 256 without carries between bytes (PNG's Up filter)."""
    n = len(a)
    hi = int.from_bytes(b"\x80" * n, "big")
    lo = hi ^ int.from_bytes(b"\xff" * n, "big")
    x, y = int.from_bytes(a, "big"), int.from_bytes(b, "big")
    return (((x & lo) + (y & lo)) ^ ((x ^ y) & hi)).to_bytes(n, "big")


def _drop_alpha(row):
    out = bytearray(len(row) // 4 * 3)
    out[0::3], out[1::3], out[2::3] = row[0::4], row[1::4], row[2::4]
    return bytes(out)


def write_png(path, w, h, rows, gray=False):
    """Rows of RGB bytes (or one byte per pixel with gray=True), filter 0."""
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xffffffff)
    raw = b"".join(b"\0" + bytes(r) for r in rows)
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 0 if gray else 2, 0, 0, 0)
    Path(path).write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(raw, 6))
                           + chunk(b"IEND", b""))


def read_bmp(path):
    """(width, height, rows) of an uncompressed 24- or 32-bit BMP (the client's
    Ctrl+Print Screen file), rows top to bottom as RGB."""
    d = Path(path).read_bytes()
    if d[:2] != b"BM":
        raise ValueError("%s is not a BMP" % path)
    off = struct.unpack("<I", d[10:14])[0]
    w, h = struct.unpack("<ii", d[18:26])
    bits, comp = struct.unpack("<HI", d[28:34])
    if bits not in (24, 32) or comp not in (0, 3):
        raise ValueError("%s: %d bits, compression %d" % (path, bits, comp))
    bpp = bits // 8
    stride = (w * bpp + 3) & ~3
    rows = []
    for i in range(abs(h)):
        y = abs(h) - 1 - i if h > 0 else i
        line = d[off + y * stride:off + y * stride + w * bpp]
        out = bytearray(w * 3)
        out[0::3], out[1::3], out[2::3] = line[2::bpp], line[1::bpp], line[0::bpp]
        rows.append(bytes(out))
    return w, abs(h), rows


# ---- masks: one Python integer per row, bit x = pixel x ----------------------

_NONZERO = re.compile(rb"[^\x00]")
_DIM = bytes(v * 2 // 5 for v in range(256))


def differing_pixels(a, b, threshold=0):
    """Pixel indices where rows a and b (RGB bytes) differ by more than
    threshold in some channel."""
    if a == b:
        return []
    x = (int.from_bytes(a, "big") ^ int.from_bytes(b, "big")).to_bytes(len(a), "big")
    seen, out = -1, []
    for m in _NONZERO.finditer(x):
        px = m.start() // 3
        if px == seen:
            continue
        o = px * 3
        if threshold == 0 or max(abs(a[o] - b[o]), abs(a[o + 1] - b[o + 1]), abs(a[o + 2] - b[o + 2])) > threshold:
            out.append(px)
            seen = px
    return out


def bits_of(pixels):
    v = 0
    for p in pixels:
        v |= 1 << p
    return v


def dilate(mask, w, r):
    if r <= 0:
        return list(mask)
    full = (1 << w) - 1
    horiz = []
    for m in mask:
        v = m
        for k in range(1, r + 1):
            v |= (m << k) | (m >> k)
        horiz.append(v & full)
    out = []
    h = len(mask)
    for y in range(h):
        v = 0
        for yy in range(max(0, y - r), min(h, y + r + 1)):
            v |= horiz[yy]
        out.append(v)
    return out


def mask_rows_to_png(path, w, mask):
    rows = []
    for m in mask:
        row = bytearray(w)
        x = m
        while x:
            low = x & -x
            row[low.bit_length() - 1] = 255
            x ^= low
        rows.append(row)
    write_png(path, w, len(mask), rows, gray=True)


def mask_from_png(path):
    w, h, rows = read_png(path)
    return w, h, [bits_of([i // 3 for i in range(0, len(r), 3) if r[i]]) if any(r) else 0 for r in rows]


def set_bits(v):
    while v:
        low = v & -v
        yield low.bit_length() - 1
        v ^= low


def tile_means(w, h, rows, mask):
    """{"tx,ty": [n, r, g, b]}: count and mean colour of the masked pixels per tile."""
    acc = {}
    for y in range(h):
        m = mask[y]
        if not m:
            continue
        row, ty = rows[y], y // TILE
        for x in set_bits(m):
            k = (x // TILE, ty)
            a = acc.get(k)
            if a is None:
                a = acc[k] = [0, 0, 0, 0]
            o = 3 * x
            a[0] += 1
            a[1] += row[o]
            a[2] += row[o + 1]
            a[3] += row[o + 2]
    return {"%d,%d" % k: [a[0], a[1] / a[0], a[2] / a[0], a[3] / a[0]] for k, a in acc.items()}


# ---- frames directories -------------------------------------------------------

def frame_sets(d):
    """{name: [primary path, sample paths...]} of a frames directory."""
    sets = {}
    for p in sorted(Path(d).glob("*.png")):
        m = SAMPLE_RE.match(p.name)
        if not m or p.name.endswith(".diff.png") or p.name.endswith(".mask.png"):
            continue
        sets.setdefault(m.group("name"), []).append((int(m.group("k") or 0), p))
    return {k: [p for _, p in sorted(v)] for k, v in sets.items()}


def sidecar(png_path):
    try:
        return json.loads(Path(png_path).with_suffix(".json").read_text())
    except (OSError, ValueError):
        return {}


def device_rect(r, sc, w, h):
    x0, y0 = max(0, r[0] * sc), max(0, r[1] * sc)
    x1, y1 = min(w, (r[0] + r[2]) * sc), min(h, (r[1] + r[3]) * sc)
    return [x0, y0, x1 - x0, y1 - y0] if x1 > x0 and y1 > y0 else None


def regions(png_path, w, h):
    """(strict rectangles, world: bool, cursor box or None, named masks) in
    device pixels, from the capture's <name>.json (mode, scale, windows,
    pointer, masks); without it the whole frame is strict."""
    info = sidecar(png_path)
    sc = info.get("scale") or 1
    p = info.get("pointer")
    cursor = device_rect([p[0] - 8, p[1] - 8, 48, 56], sc, w, h) if p else None
    masks = [dict(m, device=device_rect(m["rect"], sc, w, h)) for m in info.get("masks", [])]
    if info.get("mode") != "game":
        return [[0, 0, w, h]], False, cursor, masks
    rects = [r for r in (device_rect(r, sc, w, h) for r in info.get("windows", [])) if r]
    return rects, True, cursor, masks


def rect_rows(rects, w, h):
    """One integer per row, bit x set inside any of rects."""
    rows = [0] * h
    for x, y, rw, rh in rects:
        bits = ((1 << rw) - 1) << x
        for yy in range(y, min(h, y + rh)):
            rows[yy] |= bits
    return rows


def is_magenta(row, x):
    o = 3 * x
    return row[o] >= 240 and row[o + 1] <= 16 and row[o + 2] >= 240


def build_reference(dirs, out, commit=None, log=print):
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    sets = [frame_sets(d) for d in dirs]
    names = sorted(set.intersection(*[set(s) for s in sets])) if sets else []
    missing = sorted(set.union(*[set(s) for s in sets]) - set(names)) if sets else []
    ref = {"created": datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds"),
           "commit": commit, "runs": [str(d) for d in dirs],
           "definition": {"tolerance": TOLERANCE, "dilate": DILATE, "tile": TILE, "tile_min": TILE_MIN,
                          "tile_margin": TILE_MARGIN, "weak_share": WEAK_SHARE, "world_share": WORLD_SHARE,
                          "isolated": ISOLATED},
           "frames": {}, "left_out": missing}
    for name in names:
        paths = [p for s in sets for p in s[name]]
        w, h, base = read_png(paths[0])
        imgs = [(w, h, base)]
        mask = [0] * h
        for p in paths[1:]:
            w2, h2, rows = read_png(p)
            if (w2, h2) != (w, h):
                raise ValueError("%s: %dx%d, the reference's first capture is %dx%d" % (p, w2, h2, w, h))
            imgs.append((w2, h2, rows))
            for y in range(h):
                if rows[y] != base[y]:
                    mask[y] |= bits_of(differing_pixels(base[y], rows[y]))
        raw = sum(bin(m).count("1") for m in mask)
        strict, world, cursor, named = regions(paths[0], w, h)
        for m in named:
            if m["device"]:
                for y, bits in enumerate(rect_rows([m["device"]], w, h)):
                    mask[y] |= bits
        mask = dilate(mask, w, DILATE)
        masked = sum(bin(m).count("1") for m in mask)
        tiles = {}
        for _, _, rows in imgs:
            for k, (n, r, g, b) in tile_means(w, h, rows, mask).items():
                if n < TILE_MIN:
                    continue
                t = tiles.setdefault(k, [n, [r, g, b], [r, g, b]])
                t[1] = [min(t[1][i], v) for i, v in enumerate((r, g, b))]
                t[2] = [max(t[2][i], v) for i, v in enumerate((r, g, b))]
        Path(out / (name + ".png")).write_bytes(Path(paths[0]).read_bytes())
        if Path(paths[0]).with_suffix(".json").exists():
            Path(out / (name + ".json")).write_bytes(Path(paths[0]).with_suffix(".json").read_bytes())
        if masked:
            mask_rows_to_png(out / (name + ".mask.png"), w, mask)
        elif (out / (name + ".mask.png")).exists():
            (out / (name + ".mask.png")).unlink()
        ref["frames"][name] = {"width": w, "height": h, "captures": len(paths), "varying_px": raw,
                               "strict": strict, "world": world, "cursor": cursor,
                               "named_masks": [{"rect": m["device"], "reason": m.get("reason")} for m in named],
                               "masked_px": masked, "masked_share": round(masked / float(w * h), 6),
                               "weak": masked > WEAK_SHARE * w * h,
                               "tiles": {k: [round(v, 2) for v in t[1] + t[2]] for k, t in tiles.items()}}
        log("%-24s %5dx%-5d %d captures  varying %7d px  masked %6.3f %%  %s%s"
            % (name, w, h, len(paths), raw, 100.0 * masked / (w * h),
               "game: %d window(s) strict" % len(strict) if world else "all strict",
               "  (weak)" if masked > WEAK_SHARE * w * h else ""))
    (out / "reference.json").write_text(json.dumps(ref, indent=1) + "\n")
    if missing:
        log("left out (not in every run): %s" % ", ".join(missing))
    return ref


def compare_frame(refdir, info, name, new_path, diff_path=None):
    w, h = info["width"], info["height"]
    nw, nh, new = read_png(new_path)
    res = {"frame": name, "size": [nw, nh], "masked_share": info["masked_share"], "weak": info["weak"]}
    if (nw, nh) != (w, h):
        res.update(verdict="FAIL", reason="size %dx%d, reference %dx%d" % (nw, nh, w, h))
        return res
    _, _, ref = read_png(Path(refdir) / (name + ".png"))
    mp = Path(refdir) / (name + ".mask.png")
    mask = mask_from_png(mp)[2] if mp.exists() else [0] * h
    strict = rect_rows(info.get("strict", [[0, 0, w, h]]), w, h)
    world = info.get("world", False)
    world_px = w * h - sum(bin(r).count("1") for r in strict) if world else 0
    bad_strict = bad_world = bad_cursor = worst = magenta = 0
    bad_px, strict_px = [], set()
    cb = info.get("cursor")
    cursor = rect_rows([cb], w, h) if cb else [0] * h
    for y in range(h):
        if new[y] == ref[y]:
            continue
        for x in differing_pixels(ref[y], new[y], TOLERANCE):
            if is_magenta(new[y], x) and not is_magenta(ref[y], x):
                magenta += 1
            if (mask[y] >> x) & 1:
                continue
            if (cursor[y] >> x) & 1:
                bad_cursor += 1
                continue
            if (strict[y] >> x) & 1:
                bad_strict += 1
                strict_px.add((x, y))
            else:
                bad_world += 1
            o = 3 * x
            worst = max(worst, max(abs(ref[y][o + c] - new[y][o + c]) for c in range(3)))
            if diff_path is not None:
                bad_px.append((x, y))
    tiles_bad, tiles_world = [], 0
    means = tile_means(w, h, new, mask)
    for k, rng in info["tiles"].items():
        lo, hi = rng[0:3], rng[3:6]
        n, *rgb = means.get(k, [0, 0, 0, 0])
        if n >= TILE_MIN and all(lo[i] - TILE_MARGIN <= v <= hi[i] + TILE_MARGIN for i, v in enumerate(rgb)):
            continue
        tx, ty = (int(v) for v in k.split(","))
        cx, cy = min(w - 1, tx * TILE + TILE // 2), min(h - 1, ty * TILE + TILE // 2)
        in_cursor = cb and cb[0] <= cx < cb[0] + cb[2] and cb[1] <= cy < cb[1] + cb[3]
        x0, y0 = tx * TILE, ty * TILE
        tw = min(TILE, w - x0)
        full = (1 << tw) - 1
        inside = all((strict[y] >> x0) & full == full for y in range(y0, min(h, y0 + TILE)))
        if in_cursor or (world and not inside):
            tiles_world += 1
            continue
        tiles_bad.append({"tile": k, "mean": [round(v, 1) for v in rgb] if n >= TILE_MIN else None,
                          "ref": [lo, hi]})
    share = bad_world / float(world_px) if world_px else 0.0
    isolated = sum(1 for (x, y) in strict_px
                   if not any((x + i, y + j) in strict_px for i in (-1, 0, 1) for j in (-1, 0, 1) if i or j))
    strict_ok = bad_strict == 0 or (isolated == bad_strict and isolated <= ISOLATED)
    ok = strict_ok and not tiles_bad and magenta == 0 and share <= WORLD_SHARE
    res.update(verdict="PASS" if ok else "FAIL", pixels_beyond=bad_strict, isolated=isolated,
               world_beyond=bad_world,
               cursor_beyond=bad_cursor,
               world_share=round(share, 5), worst=worst, magenta=magenta,
               tiles_out_of_range=len(tiles_bad), tiles_world=tiles_world, tiles_checked=len(info["tiles"]),
               tiles=tiles_bad[:20])
    if diff_path is not None:
        write_diff(diff_path, w, h, new, mask, bad_px, [t["tile"] for t in tiles_bad])
    return res


def write_diff(path, w, h, rows, mask, bad_px, bad_tiles):
    out = []
    for y in range(h):
        dim = bytearray(rows[y].translate(_DIM))
        m = mask[y]
        for x in set_bits(m):
            o = 3 * x
            dim[o], dim[o + 1], dim[o + 2] = dim[o] // 2, dim[o + 1] // 2, min(255, dim[o + 2] + 110)
        out.append(dim)
    for x, y in bad_px:
        out[y][3 * x:3 * x + 3] = b"\xff\x20\x20"
    for k in bad_tiles:
        tx, ty = (int(v) for v in k.split(","))
        x0, y0 = tx * TILE, ty * TILE
        for i in range(TILE):
            for (x, y) in ((x0 + i, y0), (x0 + i, y0 + TILE - 1), (x0, y0 + i), (x0 + TILE - 1, y0 + i)):
                if x < w and y < h:
                    out[y][3 * x:3 * x + 3] = b"\xff\xff\x00"
    write_png(path, w, h, out)


def compare(refdir, frames_dir, out=None, only=None, log=print):
    refdir = Path(refdir)
    ref = json.loads((refdir / "reference.json").read_text())
    sets = frame_sets(frames_dir)
    if out:
        Path(out).mkdir(parents=True, exist_ok=True)
    results = []
    for name, info in sorted(ref["frames"].items()):
        if only and name not in only:
            continue
        if name not in sets:
            r = {"frame": name, "verdict": "FAIL", "reason": "not captured"}
        else:
            r = compare_frame(refdir, info, name, sets[name][0],
                              Path(out) / (name + ".diff.png") if out else None)
        results.append(r)
        if "reason" in r:
            log("%-24s FAIL  %s" % (name, r["reason"]))
        else:
            log("%-15s %s  strict: %6d px beyond (%d isolated), %3d tiles out  world: %6.3f %% beyond (%d px), %3d tiles  "
                "cursor box %4d px  worst %3d  magenta %d  masked %5.2f %%%s"
                % (name, r["verdict"], r["pixels_beyond"], r["isolated"], r["tiles_out_of_range"],
                   100 * r["world_share"],
                   r["world_beyond"], r["tiles_world"], r["cursor_beyond"], r["worst"], r["magenta"],
                   100 * r["masked_share"],
                   "  weak" if r["weak"] else ""))
    extra = sorted(set(sets) - set(ref["frames"]))
    passed = all(r["verdict"] == "PASS" for r in results) and bool(results)
    summary = {"reference": str(refdir), "reference_commit": ref.get("commit"), "frames_dir": str(frames_dir),
               "passed": passed, "matched": sum(r["verdict"] == "PASS" for r in results),
               "frames": len(results), "not_in_reference": extra, "results": results}
    if out:
        (Path(out) / "verdict.json").write_text(json.dumps(summary, indent=1) + "\n")
    log("%s: %d of %d frames match the reference%s"
        % ("PASS" if passed else "FAIL", summary["matched"], len(results),
           ("; not in the reference: " + ", ".join(extra)) if extra else ""))
    return summary


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("reference", help="build a reference from two or more capture runs")
    r.add_argument("--out", required=True)
    r.add_argument("--commit")
    r.add_argument("dirs", nargs="+")
    c = sub.add_parser("compare", help="compare a capture run with a reference")
    c.add_argument("reference")
    c.add_argument("frames")
    c.add_argument("--out")
    c.add_argument("--only")
    a = ap.parse_args(argv)
    if a.cmd == "reference":
        build_reference(a.dirs, a.out, a.commit)
        return 0
    s = compare(a.reference, a.frames, a.out, set(a.only.split(",")) if a.only else None)
    return 0 if s["passed"] else 1


if __name__ == "__main__":
    sys.exit(main())
