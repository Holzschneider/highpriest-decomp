"""M8.1a movement and camera: the camera's turn, zoom and tilt, played with
the mouse, reported by the client and seen in frames; a walk clicked with the
camera turned and zoomed lands on the picked cell, the server agreeing.

Done when (checklist M8.1a, with walk.py, resize_pick.py, warp.py and
session.py): in Prontera a right-drag turns the view by the drag's length in
degrees (CGameMode::ProcessRBtn 0x588030: longitude += pointer dx per
frame), the wheel zooms by 24.3 units a notch and Shift+wheel tilts by 5
degrees a notch (ProcessWheel 0x587C00), a right double-click turns the view
back to north (longitude 0); each change is read from the `camera` state
part (dest and, once eased, the view as drawn) and a frame is taken; with
the camera turned and zoomed, a click on the ground walks to the cell the
pointer is on, and a line said there is logged in chatlog at that cell.

Set-up: the script's own character (account bfhcamera, BfhCameraman, made
in SQL on first use; `--param user= password= char=`) at prontera 155,183.
"""

import time

from bfharness.play import enter_game, login, own_character, own_home, say_and_confirm, token, walk_click

NAME = "camera"
MILESTONE = "M8"
DESCRIPTION = "right-drag turn, wheel zoom, Shift+wheel tilt, right double-click reset; a walk with the camera turned"
MANIFEST = "port/harness/manifests/m8-camera.sha256"
NEEDS_SERVER = True

START = (155, 183)
NOTCH = 24.3        # ProcessWheel: distance per wheel notch
TILT = 5.0          # ProcessWheel with Shift: latitude per notch


def angle(a, b):
    """b - a in degrees, in (-180, 180]."""
    d = (b - a) % 360.0
    return d - 360.0 if d > 180.0 else d


def settled(ctl, timeout=8.0):
    """The camera once the view has eased to its destination."""
    end = time.monotonic() + timeout
    while True:
        c = ctl.state("camera")["camera"]
        d = c["dest"]
        if (abs(angle(c["longitude"], d["longitude"])) < 0.5 and abs(c["latitude"] - d["latitude"]) < 0.5
                and abs(c["distance"] - d["distance"]) < 0.5) or time.monotonic() > end:
            return c
        ctl.wait({"frames": 5}, timeout=10)


def free_line(ctl, length):
    """A horizontal line of `length` logical pixels, both ends off actors
    (the right button pressed on an NPC talks to it, as in the 2008 client)."""
    v = ctl.state("view")["view"]
    actors = [a for a in ctl.state("actors")["actors"] if a["type"] != "item" and a["screen"]]

    def free(x, y):
        if any(abs(a["screen"]["x"] - x) < 40 and -120 < y - a["screen"]["y"] < 25 for a in actors):
            return False
        ctl.move(x, y)
        ctl.wait({"frames": 3}, timeout=10)
        return not any(w["name"] == "NAMEBALLOONTEXT" for w in ctl.state("windows")["windows"])

    for dy in (180, 220, 140, -200, -240):
        x, y = v["logical"]["w"] // 2 - length // 2, v["logical"]["h"] // 2 + dy
        if free(x, y) and free(x + length, y):
            return x, y
    raise AssertionError("no line free of actors for the drag")


def run(t):
    user, password, name = own_character(t, "bfhcamera", "BfhCameraman")
    with t.step("set-up: place character", "SQL, offline: prontera %d,%d" % START) as s:
        c = t.accounts.set_char(name, map="prontera", x=START[0], y=START[1])
        s.state("char row", {k: c[k] for k in ("char_id", "last_map", "last_x", "last_y", "online")})

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)

    with t.step("camera at the start", "the map's own camera, reported") as s:
        cam0 = settled(ctl)
        s.state("camera", cam0)
        s.check(cam0 is not None, "camera reported in game mode")
        s.frame("start")

    with t.step("turn by right-drag", "ProcessRBtn: longitude follows the pointer's x") as s:
        length = 90
        x, y = free_line(ctl, length)
        ctl.drag(x, y, x + length, y, button="right", steps=10)
        cam1 = settled(ctl)
        s.state("camera", cam1)
        turned = angle(cam0["dest"]["longitude"], cam1["dest"]["longitude"])
        s.check(abs(turned - length) <= 2.0, "turned by %.1f degrees for a %d-pixel drag" % (turned, length))
        s.check(abs(angle(cam1["longitude"], cam1["dest"]["longitude"])) < 0.5, "the view eased to the turn")
        s.equal(ctl.state("windows")["windows"] and [w["name"] for w in ctl.state("windows")["windows"]
                                                       if w["name"] in ("SAYDIALOGWND", "CHOOSEWND")], [],
                "no NPC dialog opened by the drag")
        s.frame("turned")

    with t.step("zoom with the wheel", "ProcessWheel: 24.3 units a notch, back again") as s:
        v = ctl.state("view")["view"]
        cx, cy = v["logical"]["w"] // 2, v["logical"]["h"] // 2 + 180
        for _ in range(3):
            ctl.wheel(1, cx, cy)
        cam2 = settled(ctl)
        s.state("camera after 3 notches up", cam2)
        out = cam2["dest"]["distance"] - cam1["dest"]["distance"]
        s.check(abs(abs(out) - 3 * NOTCH) < 0.5, "distance changed by %.1f (3 notches)" % out)
        s.frame("zoomed")
        for _ in range(3):
            ctl.wheel(-1, cx, cy)
        cam3 = settled(ctl)
        s.state("camera after 3 notches down", cam3)
        s.check(abs(cam3["dest"]["distance"] - cam1["dest"]["distance"]) < 0.5, "back at the first distance")
        for _ in range(2):
            ctl.wheel(-1 if out > 0 else 1, cx, cy)     # two notches closer
        cam_zoom = settled(ctl)
        s.state("camera two notches closer", cam_zoom)
        s.check(cam_zoom["dest"]["distance"] < cam1["dest"]["distance"], "closer than at the start")

    with t.step("tilt with Shift+wheel", "ProcessWheel with Shift: 5 degrees a notch") as s:
        before = cam_zoom
        ctl.key("down", "shift")
        ctl.wheel(1, cx, cy)
        ctl.wheel(1, cx, cy)
        ctl.key("up", "shift")
        cam4 = settled(ctl)
        s.state("camera", cam4)
        tilt = cam4["dest"]["latitude"] - before["dest"]["latitude"]
        s.check(abs(abs(tilt) - 2 * TILT) < 0.5, "latitude changed by %.1f (2 notches)" % tilt)
        s.frame("tilted")

    with t.step("walk with the camera turned, zoomed and tilted", "picking follows the camera; the server agrees") as s:
        s.state("camera", ctl.state("camera")["camera"])
    cell = walk_click(t, ctl, 0, 120, "camera turned")
    with t.step("the server's position after the walk", "chatlog src_map_x/y at the picked cell") as s:
        me = ctl.state("self")["self"]
        say_and_confirm(t, ctl, s, "bfh camera %s" % token(), me["char_id"], expect_pos=(cell["x"], cell["y"]),
                        map="prontera")
        s.frame("walked")

    with t.step("right double-click", "ProcessRBtn BTN_DBLCLK: the view turns back to longitude 0") as s:
        x, y = free_line(ctl, 0)
        ctl.pointer("down", x, y, "right")
        ctl.pointer("up", x, y, "right")
        ctl.pointer("down", x, y, "right")
        ctl.pointer("up", x, y, "right")
        cam5 = settled(ctl)
        s.state("camera", cam5)
        s.check(abs(angle(cam5["dest"]["longitude"], 0.0)) < 0.5,
                "longitude back to 0 (was %.1f)" % cam4["dest"]["longitude"])
        s.frame("reset")
