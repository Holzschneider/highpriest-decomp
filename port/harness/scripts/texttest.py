"""M9.4 text test screen: the windows that hold the most text, in the service's font.

    BF_CLIENT=<client> python3 port/harness/run.py texttest

The client started with BF_TEXTTEST=1 opens the status, equipment, item,
skill and option windows over the login
screen (LoginMode.cpp, LSM_IDENTRY). No server is needed: nothing is sent.
The frame goes to <game folder>/test/frames/texttest/ (game artwork, outside
git) and is judged by eye against the 2008 English screenshots under
<game folder>/test/reference/ (sw-*-window.gif): labels whole, not clipped
by their boxes, not overlapping the next control, lines not overlapping.
The windows' report (state `windows`) is kept in the run's report.
"""

NAME = "texttest"
MILESTONE = "M9"
DESCRIPTION = "status, equipment, item, skill and option windows over the login screen, for the font check"
NEEDS_SERVER = False

EXPECT = ["STATUSWND", "EQUIPWND", "ITEMWND", "SKILLLISTWND", "OPTIONWND"]


def run(t):
    ctl = t.launch(env={"BF_TEXTTEST": "1"})
    with t.step("windows open", "M9.4: the text test windows open over the login screen") as s:
        ctl.wait_mode("login", 60)
        ctl.wait_window("LOGINWND")
        for name in EXPECT:
            ctl.wait_window(name, timeout=10)
        st = ctl.state("windows")
        wins = st.get("windows", [])
        s.state("windows", [(w["name"], w["rect"]) for w in wins])
        names = {w["name"] for w in wins}
        for name in EXPECT:
            s.check(name in names, "%s is open" % name)
        if not t.mock:
            ctl.move(1010, 760)            # the cursor away from the windows
        s.frame("texttest")
