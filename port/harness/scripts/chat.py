"""M8.1b chat: public, whisper, party and guild with the bot; emotes.

Done when (implementation plan M8.1, checklist M8.1b): played through the
client against a second player (bifrost-bot), each kind of chat goes both
ways and the server logs it: a public line (chatlog type O) and the bot's
public line in the client's chat window; a whisper typed with the chat
window's whisper name field (type W) and the bot's whisper shown; a party
made with /organize, the bot invited from the player menu (right-click on
it), party chat with the % prefix (type P) both ways and the `party` row;
a guild made with /guild (an Emperium in the inventory), the bot invited
from the player menu, guild chat with the $ prefix (type G) both ways and
the `guild` row and `guild_member` of the master; an emote by Alt+1 (the bot receives
ZC_EMOTION with the client's id and type 0), one typed as /heart (type 3),
and the bot's emote drawn over it in the client (`actors[].emotion`).

Set-up (SQL, offline, before the client logs in): a fresh account and
character for the client and one for the bot, both made by bifrost-bot's
character creation; both placed at prontera 156,175/158,175; Basic Skill 9
(rAthena refuses /organize below 7 and emotes below 2) and an Emperium
(item 714) for the client. With --mock the seeded bftest/BFProbe plays
alone: the bot's half of each exchange is skipped.
"""

import time

from bfharness.accounts import unique_name
from bfharness.bot import Bot
from bfharness.play import enter_game, login, own_home, token
from bfharness.sql import wait_rows

NAME = "chat"
MILESTONE = "M8"
DESCRIPTION = "public, whisper, party and guild chat with the bot both ways; emotes; chatlog O/W/P/G"
MANIFEST = "port/harness/manifests/m8-chat.sha256"
NEEDS_SERVER = True

MAP = "prontera"
CELL = (156, 175)
BOT_CELL = (158, 175)
EMPERIUM = 714
NV_BASIC = 1


def setup_chars(t):
    """Two fresh characters (client and bot), made by bifrost-bot, then
    placed and given the skill and item by SQL while offline."""
    me = {"user": unique_name("bfhc"), "password": "bfchat", "char": unique_name("Bfc")}
    bot = {"user": unique_name("bfhcb"), "password": "bfbot", "char": unique_name("Bfcb")}
    for who, label in ((me, "make-client-char"), (bot, "make-bot-char")):
        b = Bot.start(t, label, who["user"], who["password"], who["char"])
        b.quit()
        t.bots.remove(b)
        wait_rows(lambda: [r for r in [t.logs.char(who["char"])] if r and not r["online"]], timeout=20)
    for who, cell in ((me, CELL), (bot, BOT_CELL)):
        c = t.accounts.set_char(who["char"], map=MAP, x=cell[0], y=cell[1])
        who["char_id"] = c["char_id"]
        who["account_id"] = c["account_id"]
        t.db.execute("REPLACE INTO `skill` (`char_id`, `id`, `lv`, `flag`) VALUES (%d, %d, 9, 0)"
                     % (c["char_id"], NV_BASIC))
    t.db.execute("INSERT INTO `inventory` (`char_id`, `nameid`, `amount`, `identify`) VALUES (%d, %d, 1, 1)"
                 % (me["char_id"], EMPERIUM))
    return me, bot


def chat_line(ctl, after, channel, text, timeout=10):
    return ctl.wait_event("chat", lambda e: e.get("channel") == channel and text in (e.get("text") or ""),
                          after=after, timeout=timeout)


def menu_entry(ctl, s, actor, words):
    """Right-click the actor (the 2008 client's player menu) and click the
    entry whose text holds one of `words`."""
    x, y = actor["screen"]["x"], actor["screen"]["y"] - 30
    ctl.move(x, y)
    ctl.wait({"frames": 5}, timeout=10)
    ctl.click(x, y, button="right")
    ctl.wait_window("MENUWND", timeout=10)
    w = ctl.window("MENUWND")
    s.state("player menu", w)
    for c in w.get("controls", []):
        if any(word in c["text"].lower() for word in words):
            ctl.click_control("MENUWND", c["id"])
            return c["text"]
    raise AssertionError("no menu entry with %r: %r" % (words, [c["text"] for c in w.get("controls", [])]))


def run(t):
    t.bots = []
    if t.mock:
        me = {"user": "bftest", "password": "bftest", "char": "BFProbe"}
        me.update(char_id=t.logs.char("BFProbe")["char_id"])
        bot = None
        botc = None
        with t.step("set-up", "mock: bftest/BFProbe, no second player") as s:
            s.note("mock: the bot's half of each exchange is skipped")
            t.mockdb.put("inventory", id=9000, char_id=me["char_id"], nameid=EMPERIUM, amount=1, equip=0)
    else:
        with t.step("set-up: two characters", "SQL offline: place, Basic Skill 9, an Emperium") as s:
            me, botc = setup_chars(t)
            s.state("client", me)
            s.state("bot", botc)
            s.rows("client's Emperium", t.logs.inventory(me["char_id"], nameid=EMPERIUM))
        with t.step("set-up: the bot enters", "bifrost-bot beside the client's cell") as s:
            bot = Bot.start(t, "chat", botc["user"], botc["password"], botc["char"])
            s.state("bot entered", bot.entered)

    ctl = t.launch(env=own_home(t))
    login(t, ctl, me["user"], me["password"])
    st = enter_game(t, ctl)
    me_id = st["self"]["id"]
    with t.step("in game", "both characters on the map") as s:
        s.equal(st["map"]["name"], MAP, "map")
        ctl.wait({"frames": 60}, timeout=20)
        windows = ctl.state("windows")
        chat = ctl.window("CHATWND", windows)
        s.state("CHATWND", chat)
        s.check(chat is not None, "chat window open")
        ids = [c["id"] for c in chat.get("controls", [])]
        s.check("chat_edit" in ids and "whisper_edit" in ids, "chat line and whisper field")
        if bot:
            a = ctl.actor(lambda a: a["type"] == "pc" and a["name"] == botc["char"])
            s.state("bot actor", a)
            s.check(a is not None and a["screen"] is not None, "the bot on screen")
            bot_actor = a

    # -- public chat ---------------------------------------------------------
    with t.step("public chat: client to bot", "chatlog type O; the bot hears it") as s:
        text = "bfh public %s" % token()
        mark = t.logs.mark()
        after = ctl.last_seq
        since = bot.mark() if bot else 0
        ctl.say(text)
        s.state("own line", chat_line(ctl, after, "normal", text))
        rows = s.rows("chatlog O", wait_rows(lambda: t.logs.chat(char_id=me["char_id"], contains=text, type="O",
                                                                 after=mark)))
        s.equal(rows[-1]["src_map"], MAP, "chatlog map")
        if bot:
            s.state("bot heard", bot.wait(lambda e: e.get("event") == "chat" and text in e.get("text", ""),
                                          10, since))
    if bot:
        with t.step("public chat: bot to client", "the client's chat window shows the bot's line") as s:
            text = "bfh bot public %s" % token()
            after = ctl.last_seq
            bot.say(text)
            ev = chat_line(ctl, after, "normal", text)
            s.state("chat event", ev)
            s.equal(ev.get("from"), botc["char"], "speaker")
            s.frame("public")

    # -- whisper -------------------------------------------------------------
    target = botc["char"] if bot else "BfhNobody"
    with t.step("whisper: client to bot", "whisper name field; chatlog type W") as s:
        text = "bfh whisper %s" % token()
        mark = t.logs.mark()
        after = ctl.last_seq
        since = bot.mark() if bot else 0
        ctl.click_control("CHATWND", "whisper_edit")
        ctl.type_text(target)
        ctl.click_control("CHATWND", "chat_edit")
        ctl.type_text(text)
        chat = ctl.window("CHATWND")
        s.state("CHATWND before Enter", chat)
        ctl.press("enter")
        rows = s.rows("chatlog W", wait_rows(lambda: t.logs.chat(char_id=me["char_id"], contains=text, type="W",
                                                                 after=mark)))
        if bot:
            s.equal(rows[-1]["dst_charname"], target, "whisper's receiver")
            # Delivered (ZC_ACK_WHISPER 0): the 2008 client opens the 1:1
            # window with the receiver and writes the own line there.
            own = chat_line(ctl, after, "whisper", text)
            s.state("own line", own)
            s.equal((own.get("from"), own.get("window")), (me["char"], target), "own line in the 1:1 window")
            s.state("window event", ctl.wait_event("window", lambda e: e.get("action") == "open" and
                                                   e.get("name") == "WHISPERWND", after=after, timeout=5))
            ev = bot.wait(lambda e: e.get("event") == "whisper" and text in e.get("text", ""), 10, since)
            s.state("bot received", ev)
            s.equal(ev.get("from"), me["char"], "whisper's sender as the bot sees it")
    if bot:
        with t.step("whisper: bot to client", "the client opens a 1:1 window and shows the whisper there") as s:
            text = "bfh bot whisper %s" % token()
            after = ctl.last_seq
            bot.send("whisper", name=me["char"], text=text)
            ev = chat_line(ctl, after, "whisper", text)
            s.state("chat event", ev)
            s.equal(ev.get("from"), botc["char"], "whisper's sender")
            # The 2008 client's default (m_isShowWhisperWnd): a whisper from
            # someone opens a 1:1 window named after him, the line goes there.
            s.equal(ev.get("window"), botc["char"], "the 1:1 window the line went to")
            s.check(ctl.window("WHISPERWND") is not None, "the 1:1 window is open")
            s.frame("whisper")

    # -- party ---------------------------------------------------------------
    party = unique_name("BfP")[:20]
    with t.step("party: /organize", "the party is made; party row") as s:
        mark = t.logs.mark()
        # The whisper name stays in its field and would make the next lines
        # whispers: empty it as a player would.
        ctl.click_control("CHATWND", "whisper_edit")
        ctl.press("end")
        for _ in range(len(target)):
            ctl.press("backspace")
        texts = {c["id"]: c["text"] for c in ctl.window("CHATWND")["controls"]}
        s.equal(texts.get("whisper_edit"), "", "whisper name field emptied")
        ctl.click_control("CHATWND", "chat_edit")
        ctl.say("/organize %s" % party)
        # The 2008 client asks for the sharing rules first (UIPartySettingWnd);
        # its OK sends CZ_MAKE_GROUP2.
        ctl.wait_window("PARTYSETTINGWND", timeout=10)
        w = ctl.window("PARTYSETTINGWND")
        s.state("PARTYSETTINGWND", w)
        s.frame("party-setting")
        ctl.click_control("PARTYSETTINGWND", "ok")
        rows = s.rows("party row", wait_rows(lambda: t.db.select("party", {"name": party}), timeout=15))
        s.equal(rows[0]["leader_char"], me["char_id"], "party leader")
    if bot:
        with t.step("party: invite the bot", "player menu: invite; the bot accepts") as s:
            since = bot.mark()
            s.note("menu entry %r" % menu_entry(ctl, s, bot_actor, ("party",)))
            s.state("bot invited", bot.wait(lambda e: e.get("event") == "party_invite", 15, since))
            s.state("bot joined", bot.wait(lambda e: e.get("event") == "party_list" and
                                           me["char"] in e.get("members", []), 15, since))
    with t.step("party chat: client to party", "% prefix; chatlog type P") as s:
        text = "bfh party %s" % token()
        mark = t.logs.mark()
        after = ctl.last_seq
        since = bot.mark() if bot else 0
        ctl.say("%" + text)
        rows = s.rows("chatlog P", wait_rows(lambda: t.logs.chat(char_id=me["char_id"], contains=text, type="P",
                                                                 after=mark)))
        s.state("own line", chat_line(ctl, after, "party", text))
        if bot:
            s.state("bot received", bot.wait(lambda e: e.get("event") == "party_chat" and
                                             text in e.get("text", ""), 10, since))
    if bot:
        with t.step("party chat: bot to client", "the client shows the party line") as s:
            text = "bfh bot party %s" % token()
            after = ctl.last_seq
            bot.send("party_say", text=text)
            ev = chat_line(ctl, after, "party", text)
            s.state("chat event", ev)
            s.equal(ev.get("from"), botc["char"], "speaker")
            s.frame("party")

    # -- guild ---------------------------------------------------------------
    guild = unique_name("BfG")[:20]
    with t.step("guild: /guild", "the guild is made with the Emperium; guild row") as s:
        mark = t.logs.mark()
        ctl.say("/guild %s" % guild)
        rows = s.rows("guild row", wait_rows(lambda: t.db.select("guild", {"name": guild}), timeout=15))
        s.equal(rows[0]["char_id"], me["char_id"], "guild master")
        s.rows("Emperium consumed (picklog)", wait_rows(lambda: t.logs.items(char_id=me["char_id"],
                                                                             nameid=EMPERIUM, after=mark)))
        guild_id = rows[0]["guild_id"]
    if bot:
        with t.step("guild: invite the bot", "player menu: invite; the bot accepts") as s:
            since = bot.mark()
            s.note("menu entry %r" % menu_entry(ctl, s, bot_actor, ("guild",)))
            s.state("bot invited", bot.wait(lambda e: e.get("event") == "guild_invite", 15, since))
            s.state("bot joined", bot.wait(lambda e: e.get("event") == "guild" and
                                           e.get("guild_id") == guild_id, 15, since))
    with t.step("guild chat: client to guild", "$ prefix; chatlog type G") as s:
        text = "bfh guild %s" % token()
        mark = t.logs.mark()
        after = ctl.last_seq
        since = bot.mark() if bot else 0
        ctl.say("$" + text)
        rows = s.rows("chatlog G", wait_rows(lambda: t.logs.chat(char_id=me["char_id"], contains=text, type="G",
                                                                 after=mark)))
        s.state("own line", chat_line(ctl, after, "guild", text))
        if bot:
            s.state("bot received", bot.wait(lambda e: e.get("event") == "guild_chat" and
                                             text in e.get("text", ""), 10, since))
    if bot:
        with t.step("guild chat: bot to client", "the client shows the guild line") as s:
            text = "bfh bot guild %s" % token()
            after = ctl.last_seq
            bot.send("guild_say", text=text)
            ev = chat_line(ctl, after, "guild", text)
            s.state("chat event", ev)
            s.equal(ev.get("from"), botc["char"], "speaker")
            s.frame("guild")

    # -- emotes --------------------------------------------------------------
    for label, play, etype in (("Alt+1", lambda: ctl.press("1", "alt"), 0),
                               ("/heart", lambda: ctl.say("/heart"), 3)):
        with t.step("emote: %s" % label, "the bot receives ZC_EMOTION type %d from the client" % etype) as s:
            # The client sends an emote 2.5 s after the last at the earliest
            # and only once its own balloon has ended (CGameMode MM_REQ_EMOTION,
            # MM_PLAYER_EMOTION_END); rAthena takes one a second.
            time.sleep(3.5)
            since = bot.mark() if bot else 0
            play()
            if bot:
                ev = bot.wait(lambda e: e.get("event") == "emotion" and e.get("id") == me_id, 10, since)
                s.state("bot saw", ev)
                s.equal(ev.get("type"), etype, "emotion type")
            else:
                s.note("mock: no second player to see it")
    if bot:
        with t.step("emote: the bot's", "the client draws the bot's emote (ZC_EMOTION type 1)") as s:
            time.sleep(2.2)
            since = bot.mark()
            bot.send("emotion", type=1)
            s.state("bot's own", bot.wait(lambda e: e.get("event") == "emotion" and e.get("self"), 10, since))
            deadline = time.monotonic() + 5
            seen = None
            while time.monotonic() < deadline and seen is None:
                for a in ctl.state("actors")["actors"]:
                    if a["id"] == bot_actor["id"] and a.get("emotion") is not None:
                        seen = a
                time.sleep(0.1)
            s.state("bot actor", seen)
            s.check(seen is not None, "an emote balloon over the bot")
            s.equal(seen["emotion"], 1, "emotion.act action 1 (ET_QUESTION)")
            s.frame("emote")

    # -- after logout: membership saved -------------------------------------
    t.stop_client()
    if bot:
        bot.quit()
        t.bots.remove(bot)
    with t.step("saved membership", "char.party_id and guild_id of both after logout; guild_member rows") as s:
        p = t.db.select("party", {"name": party})[0]
        for who in (me, botc) if botc else (me,):
            rows = wait_rows(lambda: [r for r in [t.logs.char(who["char"])]
                                      if r and not r["online"] and r["party_id"] == p["party_id"]
                                      and r["guild_id"] == guild_id], timeout=20)
            s.rows("%s: party %d, guild %d" % (who["char"], p["party_id"], guild_id), rows)
        # guild_member gets the master at creation; the char server writes
        # the other members when its autosave timer reaches the guild (one
        # guild per autosave_time / number of guilds), so the bot's
        # membership is read from char.guild_id above.
        rows = s.rows("guild_member: the master", t.db.select("guild_member", {"guild_id": guild_id}))
        s.check(any(r["char_id"] == me["char_id"] for r in rows), "the client's character is a member")
