"""Effects drawn from .str files, shown on the own character; no failed
assertion, no sanitizer report.

A repro for the soak's `assertion failed: iTex < cTex (EZeffect.cpp:40)`
(follow-up of M8.6): an effect that starts in a frame without a game tick
is drawn before its first ProcessEZ2STR (CRagEffect::OnProcess returns early
while g_isSkipFrame is 0), and the 2008 constructor leaves the per-layer
draw flags and frames as the heap gives them; a block freed by another
effect then drew its layers with frames past this effect's textures.

Set-up: a fresh group-99 account and Novice on prt_fild08 250,200; each
effect is shown by rAthena's `@effect <id>` (ZC_NOTIFY_EFFECT2, typed by the
client) and left to play `--param wait` seconds (default 2). `--param ids`
takes a comma list, or `all` for every id CRagEffect::Init gives a .str name
(read from src/RagEffect.h and .cpp; about 170, 5 minutes). With the
recycled heap it fails now and then; under the `asan` preset with
ASAN_OPTIONS=malloc_fill_byte=85:max_malloc_fill_size=1048576 every new
effect starts on 0x55 bytes, and the default ids fail at the first effect
when the bug is there (UBSan: 1.46602e+13 outside the range of int,
RagEffect.cpp RenderAniClip). Real client only (it passes with --mock).
"""

import os
import re
import time

from bfharness.accounts import unique_name
from bfharness.play import enter_game, fresh_character, login, own_home

NAME = "str_effects"
MILESTONE = "M8"
DESCRIPTION = ".str effects shown on the own character by @effect; no failed assertion"
MANIFEST = None
NEEDS_SERVER = True

START = ("prt_fild08", 250, 200)
# the red potion (EF_POTION1, found in the soak), Stone Curse, Angelus, Cure, Provoke
DEFAULT_IDS = "204,195,41,66,67,204,195,41,66,67"


def str_effect_ids():
    """The effect ids CRagEffect::Init loads a .str for, from the source."""
    root = os.path.join(os.path.dirname(__file__), "..", "..", "..", "src")
    h = open(os.path.join(root, "RagEffect.h"), encoding="latin1").read()
    at = h.find("EF_ANGEL,")
    body = h[h.find("{", h.rfind("enum", 0, at)) + 1:h.find("}", at)]
    body = re.sub(r"//.*|/\*.*?\*/|^\s*#.*$", "", body, flags=re.S | re.M)
    values, value = {}, -1
    for tok in (t.strip() for t in body.split(",")):
        if not tok:
            continue
        name, _, given = (x.strip() for x in tok.partition("="))
        value = int(given, 0) if given else value + 1
        values[name] = value
    src = open(os.path.join(root, "RagEffect.cpp"), encoding="latin1").read()
    a = src.find("CRagEffect :: Init(")
    seg = src[a:src.find("LoadEzEffect(StrName)", a)]
    ids, cases = set(), []
    for line in seg.split("\n"):
        cases += re.findall(r"case\s+(EF_\w+)", line)
        if re.search(r'sprintf\(StrName,\s*"[^"]+"', line):
            ids.update(values[c] for c in cases if c in values)
        if "break" in line:
            cases = []
    return sorted(ids)


def run(t):
    spec = t.param("ids", DEFAULT_IDS)
    ids = str_effect_ids() if spec == "all" else [int(x) for x in spec.split(",") if x]
    wait = float(t.param("wait", "2"))
    if t.mock:
        with t.step("show the effects", "real client only") as s:
            s.note("mock: %d ids, nothing to show" % len(ids))
        return
    user, name = unique_name(tag="e"), unique_name(tag="E")
    fresh_character(t, user, "bfh-test", name, place=START)
    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, "bfh-test")
    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)
    t.gm.ctl = ctl
    with t.step("show the effects", "@effect <id> for %d ids, %.1f s each; no failed assertion"
                % (len(ids), wait)) as s:
        seen = []
        for i in ids:
            t.gm.run("@effect %d" % i)
            time.sleep(wait)
            lines = t.client.trace_lines("assertion failed")
            if len(lines) > len(seen):
                s.note("effect %d: %s" % (i, lines[len(seen):]))
                seen = lines
        s.state("effect ids shown", ids)
        s.check(not seen, "no failed assertion (%d lines)" % len(seen))
