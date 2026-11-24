#!/usr/bin/env python3
"""Plays three bifrost-bot instances against each other on the test server
and confirms every round trip in the server's SQL logs.

  port/bot/selftest.py [--build] [--keep-logs] [--host 127.0.0.1] [--port 6900]

  GM  bfadmin / bfadmin    character BfBotGM, sets the scene with GM commands
  A   bfbota_M / bfbot     character BfBotA (account registered on first run)
  B   bfbotb_M / bfbot     character BfBotB, answers with its default policies

Scenario: the GM gives A zeny (#zeny) and calls A and B to its cell (@recall);
A walks a cell; A whispers B and B answers; A chats in public and B echoes;
A creates a party and invites B, B accepts; A asks B to trade, B accepts, A
puts in 1 zeny, both confirm; the GM invites B into its guild, B accepts.

Every step is checked in the events the bots report and then in the SQL
tables (chatlog, party / char, zenylog, guild_member), and the rows are
printed. Exit status 0 when every check passes. Event logs of the bots go to
build/port/bot-selftest/. Standard library only.
"""
import argparse, json, os, subprocess, sys, threading, time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
BOT = os.path.join(ROOT, 'build', 'port', 'bin', 'bifrost-bot')
COMPOSE = os.path.join(ROOT, 'port', 'server', 'docker-compose.yml')
LOGDIR = os.path.join(ROOT, 'build', 'port', 'bot-selftest')


class Bot:
    """One bifrost-bot process, driven through stdin, events read from stdout."""

    def __init__(self, label, user, password, char, host, port, sets=()):
        self.label, self.char = label, char
        self.events = []
        self.cond = threading.Condition()
        self.log = open(os.path.join(LOGDIR, label + '.events'), 'w')
        err = open(os.path.join(LOGDIR, label + '.stderr'), 'w')
        args = [BOT, '--host', host, '--port', str(port), '--user', user, '--pass', password,
                '--char-name', char, '--login-retries', '8']
        for s in sets:
            args += ['--set', s]
        self.proc = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=err,
                                     text=True, bufsize=1)
        threading.Thread(target=self._read, daemon=True).start()

    def _read(self):
        for line in self.proc.stdout:
            self.log.write(line)
            self.log.flush()
            try:
                event = json.loads(line)
            except ValueError:
                continue
            with self.cond:
                self.events.append(event)
                self.cond.notify_all()
        with self.cond:
            self.events.append({'event': 'eof'})
            self.cond.notify_all()

    def mark(self):
        with self.cond:
            return len(self.events)

    def send(self, cmd, **fields):
        fields['cmd'] = cmd
        self.proc.stdin.write(json.dumps(fields) + '\n')
        self.proc.stdin.flush()

    def wait(self, predicate, timeout=10, since=0):
        """The first event from index `since` on that satisfies predicate."""
        deadline = time.time() + timeout
        i = since
        with self.cond:
            while True:
                while i < len(self.events):
                    e = self.events[i]
                    i += 1
                    if predicate(e):
                        return e
                    if e['event'] == 'eof':
                        return None
                left = deadline - time.time()
                if left <= 0:
                    return None
                self.cond.wait(left)

    def wait_event(self, name, timeout=10, since=0, **match):
        return self.wait(lambda e: e['event'] == name and all(e.get(k) == v for k, v in match.items()),
                         timeout, since)

    def state(self):
        m = self.mark()
        self.send('state')
        return self.wait_event('state', 5, m)

    def quit(self):
        if self.proc.poll() is None:
            try:
                self.send('quit')
            except (BrokenPipeError, OSError):
                pass
        try:
            self.proc.wait(5)
        except subprocess.TimeoutExpired:
            self.proc.kill()


def sql(query):
    out = subprocess.run(['docker', 'compose', '-f', COMPOSE, 'exec', '-T', 'db', 'mariadb', '-uragnarok',
                          '-pragnarok', 'ragnarok', '-B', '-e', query],
                         capture_output=True, text=True)
    if out.returncode != 0:
        raise RuntimeError('SQL failed: ' + out.stderr.strip())
    lines = out.stdout.splitlines()
    return lines[:1], [l.split('\t') for l in lines[1:]]


def sql_wait(query, check, timeout=10):
    """Repeats a query until check(rows) holds (the char server writes some
    tables a moment after the event)."""
    deadline = time.time() + timeout
    while True:
        head, rows = sql(query)
        if check(rows) or time.time() > deadline:
            return head, rows
        time.sleep(0.5)


class Report:
    def __init__(self):
        self.results = []

    def check(self, name, ok, detail=''):
        self.results.append((name, ok))
        print('%-4s %s%s' % ('ok' if ok else 'FAIL', name, ('  ' + detail) if detail else ''), flush=True)
        return ok

    def rows(self, head, rows):
        for line in head:
            print('       | ' + line.replace('\t', ' | '))
        for r in rows:
            print('       | ' + ' | '.join(r))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--build', action='store_true', help='run port/bot/build.sh first')
    ap.add_argument('--host', default='127.0.0.1')
    ap.add_argument('--port', type=int, default=6900)
    args = ap.parse_args()

    if args.build or not os.path.exists(BOT):
        subprocess.run([os.path.join(ROOT, 'port', 'bot', 'build.sh')], check=True)
    os.makedirs(LOGDIR, exist_ok=True)
    tag = time.strftime('%H%M%S')
    party_name = 'BfP' + tag
    r = Report()

    _, rows = sql('SELECT COALESCE(MAX(id),0) FROM zenylog')
    zeny_mark = int(rows[0][0])

    quiet = ('echo_chat=off', 'echo_whisper=off')
    bots = []
    post = []
    try:
        gm = Bot('gm', 'bfadmin', 'bfadmin', 'BfBotGM', args.host, args.port, quiet)
        bots.append(gm)
        # One at a time: the account server limits new registrations.
        r.check('BfBotGM logs in and enters the map', gm.wait_event('entered', 60) is not None)
        a = Bot('a', 'bfbota_M', 'bfbot', 'BfBotA', args.host, args.port)
        bots.append(a)
        r.check('BfBotA logs in and enters the map', a.wait_event('entered', 60) is not None)
        b = Bot('b', 'bfbotb_M', 'bfbot', 'BfBotB', args.host, args.port)
        bots.append(b)
        r.check('BfBotB logs in and enters the map', b.wait_event('entered', 60) is not None)
        if not all(ok for _, ok in r.results):
            return finish(r, bots, post)
        time.sleep(1)

        # --- scene: zeny for A, A and B to the GM's cell
        sa = a.state()
        m = a.mark()
        gm.send('gm', text='#zeny BfBotA 100')
        e = a.wait_event('zeny', 10, m)
        r.check('scene: #zeny gives BfBotA 100 zeny', e is not None and e['change'] == 100,
                'zeny %s -> %s' % (sa['zeny'], e['zeny'] if e else '?'))
        m = gm.mark()
        gm.send('gm', text='#allskill BfBotA')
        e = gm.wait(lambda e: e['event'] == 'notice' and 'skill' in e['text'], 10, m)
        r.check('scene: #allskill gives BfBotA the Basic Skill (needed to trade and form a party)', e is not None,
                repr(e['text']) if e else 'no notice')
        # The GM steps aside first so that @recall really moves the others:
        # between two cells next to the start point, so that repeated runs
        # stay in the same place.
        g = gm.state()['position']
        to = (155, 185) if (g['x'], g['y']) != (155, 185) else (155, 183)
        m = gm.mark()
        gm.send('move', x=to[0], y=to[1])
        gm.wait(lambda e: e['event'] == 'position' and (e.get('arrived') or e.get('stopped')), 8, m)
        time.sleep(0.6)  # the bot's arrival is an estimate; let the server's walk end too
        g = gm.state()['position']
        for bot in (a, b):
            m = bot.mark()
            gm.send('gm', text='@recall ' + bot.char)
            e = bot.wait(lambda e: e['event'] == 'position' and e.get('warped'), 10, m)
            r.check('scene: @recall brings %s to (%d, %d)' % (bot.char, g['x'], g['y']),
                    e is not None and (e['x'], e['y']) == (g['x'], g['y']),
                    'at (%s, %s)' % (e['x'], e['y']) if e else 'no warp')
        time.sleep(1)

        # --- walking
        m = a.mark()
        a.send('move', x=g['x'] + 1, y=g['y'])
        e = a.wait(lambda e: e['event'] == 'position' and (e.get('arrived') or e.get('stopped')), 8, m)
        pos = a.state()['position']
        r.check('BfBotA walks to (%d, %d)' % (g['x'] + 1, g['y']), (pos['x'], pos['y']) == (g['x'] + 1, g['y']),
                'state position (%d, %d)' % (pos['x'], pos['y']))
        sb = b.state()
        r.check('BfBotA sees BfBotB by name', any(x['name'] == 'BfBotB' for x in a.state()['actors']))

        # --- whisper and the echo
        m = a.mark()
        a.send('whisper', name='BfBotB', text='hello ' + tag)
        e = a.wait_event('whisper', 10, m, **{'from': 'BfBotB'})
        r.check('whisper A -> B, B answers', e is not None and e['text'] == 'echo: hello ' + tag,
                repr(e['text']) if e else 'no answer')
        # --- public chat and the echo
        m = a.mark()
        a.send('say', text='hi all ' + tag)
        e = a.wait_event('chat', 10, m, **{'from': 'BfBotB'})
        r.check('public chat A, B echoes', e is not None and e['text'] == 'echo: hi all ' + tag,
                repr(e['text']) if e else 'no echo')
        q = ("SELECT c.id, c.time, c.type, ch.name AS src, c.dst_charname, c.message FROM chatlog c "
             "JOIN `char` ch ON ch.char_id = c.src_charid WHERE c.message LIKE '%%%s%%' ORDER BY c.id" % tag)
        head, rows = sql_wait(q, lambda rows: len(rows) >= 4)
        want = [('W', 'BfBotA', 'BfBotB', 'hello ' + tag), ('W', 'BfBotB', 'BfBotA', 'echo: hello ' + tag),
                ('O', 'BfBotA', None, 'hi all ' + tag), ('O', 'BfBotB', None, 'echo: hi all ' + tag)]
        got = [(row[2], row[3], row[4], row[5]) for row in rows]
        ok = all(any(g2[0] == w[0] and g2[1] == w[1] and (w[2] is None or g2[2] == w[2]) and g2[3] == w[3]
                     for g2 in got) for w in want)
        r.check('chatlog has both whispers and both public lines', ok)
        r.rows(head, rows)

        # --- party
        for bot in (a, b):
            if bot.state()['party']:
                m = bot.mark()
                bot.send('leave_party')
                bot.wait_event('party_left', 5, m)
        m = a.mark()
        a.send('create_party', name=party_name)
        e = a.wait_event('party_created', 10, m)
        r.check('BfBotA creates party %s' % party_name, e is not None and e['result'] == 0,
                'result %s' % (e['result'] if e else '?'))
        mb = b.mark()
        m = a.mark()
        a.send('invite_party', name='BfBotB')
        e = b.wait_event('party_invite', 10, mb)
        r.check('BfBotB receives the party invitation', e is not None and e['party'] == party_name)
        e = a.wait_event('party_invite_result', 10, m)
        r.check('BfBotB accepts (answer 2)', e is not None and e['answer'] == 2,
                'answer %s' % (e['answer'] if e else '?'))
        q = ("SELECT p.party_id, p.name, p.leader_char, ch.char_id, ch.name FROM party p "
             "JOIN `char` ch ON ch.party_id = p.party_id WHERE p.name = '%s' ORDER BY ch.name" % party_name)
        head, rows = sql_wait(q, lambda rows: len(rows) >= 2)
        r.check('party table: BfBotA and BfBotB are members', sorted(row[4] for row in rows) == ['BfBotA', 'BfBotB'])
        r.rows(head, rows)

        # --- trade: 1 zeny from A to B
        za, zb = a.state()['zeny'], b.state()['zeny']
        ma, mb = a.mark(), b.mark()
        a.send('request_trade', name='BfBotB')
        e = b.wait_event('trade_request', 10, mb)
        r.check('BfBotB receives the trade request', e is not None and e['from'] == 'BfBotA')
        ea = a.wait_event('trade_open', 10, ma)
        eb = b.wait_event('trade_open', 10, mb)
        r.check('trade window opens on both sides', ea is not None and eb is not None)
        m = a.mark()
        a.send('trade_add', kind='zeny', amount='1')
        e = b.wait_event('trade_item', 10, mb)
        r.check('BfBotB sees 1 zeny offered', e is not None and e['item'] == 0 and e['count'] == 1)
        a.send('trade_ok')
        # rAthena acknowledges a zeny offer only when the offering side locks.
        e = a.wait_event('trade_add_result', 10, m)
        r.check('BfBotA\'s 1 zeny is acknowledged (index 0, result 0)', e is not None and e['index'] == 0 and e['result'] == 0)
        ea = a.wait_event('trade_done', 10, ma)
        eb = b.wait_event('trade_done', 10, mb)
        r.check('both confirm, trade completes', ea is not None and eb is not None and ea['result'] == 0 and eb['result'] == 0)
        time.sleep(0.5)
        za2, zb2 = a.state()['zeny'], b.state()['zeny']
        r.check('zeny moved: BfBotA %d -> %d, BfBotB %d -> %d' % (za, za2, zb, zb2), za2 == za - 1 and zb2 == zb + 1)
        q = ("SELECT z.id, z.time, ch.name, z.src_id, z.type, z.amount, z.map FROM zenylog z "
             "JOIN `char` ch ON ch.char_id = z.char_id WHERE z.id > %d ORDER BY z.id" % zeny_mark)
        head, rows = sql_wait(q, lambda rows: sum(1 for row in rows if row[4] == 'T') >= 2)
        trades = [(row[2], row[4], row[5]) for row in rows if row[4] == 'T']
        r.check('zenylog: trade rows -1 for BfBotA, +1 for BfBotB',
                ('BfBotA', 'T', '-1') in trades and ('BfBotB', 'T', '1') in trades)
        r.rows(head, rows)

        # --- guild: the GM's guild invites B. rAthena kicks a member who
        # rejoins a guild he has left ("** Guild Mismatch **" in its log),
        # so every run uses a new guild: B leaves, the GM breaks its old
        # guild and founds BfG<time>.
        sb = b.state()
        if sb['guild_id']:
            m = b.mark()
            b.send('leave_guild')
            b.wait_event('guild_left', 10, m)
            time.sleep(1)
        if gm.state()['guild_id']:
            m = gm.mark()
            gm.send('gm', text='@breakguild')
            gm.wait(lambda e: e['event'] == 'notice' and 'guild' in e['text'].lower(), 5, m)
            time.sleep(1)
        m = gm.mark()
        gm.send('gm', text='@guild BfG' + tag)
        gm.wait_event('guild', 10, m)
        sg = gm.state()
        r.check('scene: BfBotGM founds guild BfG%s' % tag, sg['guild_id'] != 0 and sg['guild'] == 'BfG' + tag,
                '%s (%s)' % (sg['guild'], sg['guild_id']))
        time.sleep(1)
        mb, m = b.mark(), gm.mark()
        gm.send('invite_guild', aid=str(sb['aid']))
        e = b.wait_event('guild_invite', 10, mb)
        r.check('BfBotB receives the guild invitation', e is not None and e['guild_id'] == sg['guild_id'])
        e = gm.wait_event('guild_invite_result', 10, m)
        r.check('BfBotB accepts (answer 2)', e is not None and e['answer'] == 2, 'answer %s' % (e['answer'] if e else '?'))
        e = b.wait_event('guild', 5, mb)
        r.check('BfBotB is told it joined (ZC_CHANGE_GUILD)', e is not None and e['guild_id'] == sg['guild_id'])
        # The char server writes guild_member on its autosave timer; the
        # character's guild_id is written when it logs out, so this is
        # checked after the bots have quit.
        post.append(lambda: check_guild_sql(r, sg['guild_id'], tag))
    except Exception as ex:  # report, then still log the bots out
        r.check('scenario ran to the end', False, '%s: %s' % (type(ex).__name__, ex))
    return finish(r, bots, post)


def check_guild_sql(r, guild_id, tag):
    q = ("SELECT ch.char_id, ch.name, ch.guild_id, g.name AS guild, g.master FROM `char` ch "
         "JOIN guild g ON g.guild_id = ch.guild_id WHERE ch.guild_id = %d ORDER BY ch.name" % guild_id)
    head, rows = sql_wait(q, lambda rows: any(row[1] == 'BfBotB' for row in rows), 10)
    r.check('char table after logout: BfBotB belongs to guild BfG%s' % tag, any(row[1] == 'BfBotB' for row in rows))
    r.rows(head, rows)
    head, rows = sql('SELECT guild_id, char_id, position FROM guild_member WHERE guild_id = %d' % guild_id)
    print('     guild_member (written on the char server\'s autosave timer, may lag):')
    r.rows(head, rows)


def finish(r, bots, post=()):
    for bot in bots:
        m = bot.mark()
        bot.quit()
        e = bot.wait_event('exit', 3, m)
        if e is not None:
            r.check('%s exits cleanly, no unframed packets' % bot.char, e['code'] == 0 and not e['unknown_headers'],
                    '%d packets read' % e['packets'])
    for check in post:
        try:
            check()
        except Exception as ex:
            r.check('post-check', False, '%s: %s' % (type(ex).__name__, ex))
    failed = [n for n, ok in r.results if not ok]
    print('\nbifrost-bot selftest: %d checks, %d failed%s' % (len(r.results), len(failed),
          '' if not failed else ': ' + '; '.join(failed)))
    print('event logs: ' + LOGDIR)
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
