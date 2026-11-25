"""framecmp.py (M11.0c): masks from the reference runs, the tolerance outside
them, the tile check inside them; on synthetic frames."""

import pathlib  # noqa: E402
import sys  # noqa: E402
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))  # port/harness

import random
import shutil
import tempfile
import unittest
from pathlib import Path

import framecmp

W, H = 96, 64
BLINK = (60, 10, 8, 12)      # x, y, w, h: changes from capture to capture


def scene(phase=0, shift=0, blink_colour=None, nudge=0):
    """Rows of RGB: a textured background, a 'window' with text-like
    strokes, and a blinking box whose colour depends on phase."""
    rnd = random.Random(7)
    tex = [[(rnd.randrange(60, 200), rnd.randrange(60, 200), rnd.randrange(60, 200)) for _ in range(W)]
           for _ in range(H)]
    rows = []
    for y in range(H):
        r = bytearray()
        for x in range(W):
            sx = min(W - 1, max(0, x - shift))
            c = tex[y][sx]
            if 10 <= sx < 50 and 20 <= y < 50:
                c = (230, 230, 240) if (sx + y) % 7 else (10, 10, 10)
            bx, by, bw, bh = BLINK
            if bx <= x < bx + bw and by <= y < by + bh:
                c = blink_colour or ((200, 40, 40) if phase % 2 else (40, 40, 200))
            r += bytes(min(255, v + nudge) for v in c)
        rows.append(bytes(r))
    return rows


class FrameCmp(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="bf-framecmp-"))

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def run_dir(self, name, frames):
        d = self.tmp / name
        d.mkdir()
        for fname, rows in frames.items():
            framecmp.write_png(d / (fname + ".png"), W, H, rows)
        return d

    def reference(self):
        a = self.run_dir("a", {"f": scene(0), "f.s1": scene(1), "f.s2": scene(0)})
        b = self.run_dir("b", {"f": scene(1), "f.s1": scene(0), "f.s2": scene(1)})
        ref = framecmp.build_reference([a, b], self.tmp / "ref", "test", log=lambda *_: None)
        return ref

    def compare(self, rows, w=W, h=H):
        d = self.tmp / ("new%d" % len(list(self.tmp.iterdir())))
        d.mkdir()
        framecmp.write_png(d / "f.png", w, h, rows if w == W else [r[:w * 3] for r in rows][:h])
        return framecmp.compare(self.tmp / "ref", d, out=d / "diff", log=lambda *_: None)

    def test_png_round_trip(self):
        rows = scene(1)
        framecmp.write_png(self.tmp / "x.png", W, H, rows)
        self.assertEqual(framecmp.read_png(self.tmp / "x.png"), (W, H, rows))

    def test_mask_is_the_blinking_box_widened(self):
        ref = self.reference()
        f = ref["frames"]["f"]
        bx, by, bw, bh = BLINK
        self.assertEqual(f["varying_px"], bw * bh)
        d = framecmp.DILATE
        widened = (min(W, bx + bw + d) - max(0, bx - d)) * (min(H, by + bh + d) - max(0, by - d))
        self.assertEqual(f["masked_px"], widened)
        self.assertEqual(f["captures"], 6)

    def test_same_scene_matches(self):
        self.reference()
        self.assertTrue(self.compare(scene(0))["passed"])
        self.assertTrue(self.compare(scene(1))["passed"])

    def test_rounding_within_tolerance(self):
        self.reference()
        self.assertTrue(self.compare(scene(0, nudge=framecmp.TOLERANCE))["passed"])

    def test_shift_by_one_pixel_fails(self):
        self.reference()
        s = self.compare(scene(0, shift=1))
        self.assertFalse(s["passed"])
        self.assertGreater(s["results"][0]["pixels_beyond"], 100)

    def test_masked_region_turned_green_fails(self):
        self.reference()
        s = self.compare(scene(0, blink_colour=(0, 255, 0)))
        self.assertFalse(s["passed"])
        self.assertEqual(s["results"][0]["pixels_beyond"], 0)
        self.assertGreater(s["results"][0]["tiles_out_of_range"], 0)

    def test_size_change_fails(self):
        self.reference()
        s = self.compare(scene(0), w=W - 2)
        self.assertFalse(s["passed"])
        self.assertIn("size", s["results"][0]["reason"])

    def game_reference(self):
        """The same scene as a game-mode frame whose 'window' (x 10-50,
        y 20-50) is the strict region; the rest is world."""
        import json
        a = self.run_dir("a", {"f": scene(0), "f.s1": scene(1), "f.s2": scene(0)})
        b = self.run_dir("b", {"f": scene(1), "f.s1": scene(0), "f.s2": scene(1)})
        (a / "f.json").write_text(json.dumps({"mode": "game", "scale": 1, "windows": [[10, 20, 40, 30]]}))
        return framecmp.build_reference([a, b], self.tmp / "ref", "test", log=lambda *_: None)

    def poke(self, rows, pts, colour):
        rows = [bytearray(r) for r in rows]
        for x, y in pts:
            rows[y][3 * x:3 * x + 3] = bytes(colour)
        return [bytes(r) for r in rows]

    def test_world_tolerates_a_few_pixels(self):
        ref = self.game_reference()
        self.assertTrue(ref["frames"]["f"]["world"])
        s = self.compare(self.poke(scene(0), [(2, 2), (90, 60)], (0, 0, 0)))
        self.assertTrue(s["passed"], s)
        self.assertEqual(s["results"][0]["world_beyond"], 2)

    def test_world_shift_fails(self):
        self.game_reference()
        self.assertFalse(self.compare(scene(0, shift=1))["passed"])

    def test_one_pixel_in_a_window_fails(self):
        self.game_reference()
        s = self.compare(self.poke(scene(0), [(30, 30)], (90, 90, 90)))
        self.assertFalse(s["passed"])
        self.assertEqual(s["results"][0]["pixels_beyond"], 1)

    def test_magenta_in_the_world_fails(self):
        self.game_reference()
        s = self.compare(self.poke(scene(0), [(2, 2)], (255, 0, 255)))
        self.assertFalse(s["passed"])
        self.assertEqual(s["results"][0]["magenta"], 1)

    def test_named_mask(self):
        import json
        a = self.run_dir("a", {"f": scene(0)})
        b = self.run_dir("b", {"f": scene(0)})
        (a / "f.json").write_text(json.dumps({"mode": "login", "scale": 1,
                                              "masks": [{"rect": [0, 0, 8, 8], "reason": "clock"}]}))
        ref = framecmp.build_reference([a, b], self.tmp / "ref", "test", log=lambda *_: None)
        self.assertEqual(ref["frames"]["f"]["named_masks"][0]["reason"], "clock")
        self.assertTrue(self.compare(self.poke(scene(0), [(3, 3)], (0, 0, 0)))["passed"])
        self.assertFalse(self.compare(self.poke(scene(0), [(30, 3)], (0, 0, 0)))["passed"])

    def test_bmp_reader(self):
        rows = scene(0)
        stride = (W * 3 + 3) & ~3
        body = b"".join(bytes(v for x in range(W) for v in (r[3 * x + 2], r[3 * x + 1], r[3 * x])) +
                        b"\0" * (stride - W * 3) for r in reversed(rows))
        import struct
        head = b"BM" + struct.pack("<IHHI", 54 + len(body), 0, 0, 54) + \
            struct.pack("<IiiHHIIiiII", 40, W, H, 1, 24, 0, len(body), 0, 0, 0, 0)
        (self.tmp / "x.bmp").write_bytes(head + body)
        self.assertEqual(framecmp.read_bmp(self.tmp / "x.bmp"), (W, H, rows))


if __name__ == "__main__":
    unittest.main()
