#!/usr/bin/env python3
"""Finds numeric packet ids in the client's handlers and senders (M10.2).

  packetids.py [--tree DIR] [--names] [--quiet] [--self-test]

Every packet id in src/ is to be written as a HEADER_ enumerator of
src/packet.h; the numbers themselves live only in that enumeration, and the
lengths only in the table of src/packet.cpp. This tool lists every integer
literal that src/ still uses as a packet id, as FILE:LINE: RULE: TEXT, and
exits 1 if there is any. --names adds the enumerator the value has in
packet.h (or "no name"), which is what a rewrite would put there.

What counts as a packet id (comments and string literals are blanked first,
so nothing in them is found):

  case      a case label of a switch on a packet type: a switch whose
            controlling expression names packetType, PacketType,
            packetHeader or GetPacketType(...). Labels of switches nested
            inside it are not its labels and are not looked at.
  header    a literal stored into, or compared with, a header field or
            variable: X.PacketType, X->PacketType, packetType, packetHeader
            (or PacketHeader) on one side of =, ==, != and a lone literal on
            the other. "Lone" means the literal is the whole operand: it is
            followed by ; , ) or && / ||, so an expression such as
            `0x80 & PacketInfo` (the GIF reader's own "packet") is not one.
            Also GetPacketType(...) compared with a lone literal.
  table     a literal used as a key of the length table or as the argument
            of a function that takes a packet id: g_packetLenMap[N],
            GetPacketSize(N), IsFixedPacket(N), bf_packet_name(N).
  init      a PACKET_ structure initialised with a literal header:
            PACKET_X p = { N, ... } with N other than 0 (`= {0}` clears a
            structure and names no packet).
  store     a literal written to the first two bytes of a buffer:
            *(WORD *)buf = N, *(short *)buf = N, *(unsigned short *)buf = N;
            and the same cast compared with a literal (==, !=) when the
            buffer's name has buf, packet or pkt in it.

Not packet ids, and so not reported:
  - the enumerators' own definitions in packet.h (HEADER_X = N);
  - case labels of any other switch (FindHack's GameGuard error codes are
    `case 120:` too);
  - literals in message macros and the like (PUSH_HELP_MSG(0x9b) is a
    message-table id), byte arrays, GUIDs, sizes and offsets;
  - masks applied to a header (`packetType & 0xffff`), which the "lone"
    rule leaves out;
  - two-byte magic numbers of files and images (SafeAPI's
    `*(unsigned short *)header == 0x5a4d`, "MZ"): the buffer is not named
    as a packet buffer;
  - the GameGuard and Gron messenger protocols' own numbers (not RO packets:
    device/GronPacket.* has its own table and no HEADER_ names).

--self-test runs the rules on a synthetic text holding one instance of each
kind and each exclusion, and fails unless exactly the instances are found.
"""
import argparse, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))

LIT = r'(?:0[xX][0-9a-fA-F]+|\d+)[uUlL]*'
LONE_END = r'(?=\s*(?:;|,|\)|&&|\|\||$))'
RAW16 = r'(?:WORD|short|unsigned\s+short|USHORT|u_short|uint16_t|int16_t)'
HEADER_SIDE = r'(?:[\w\]\)]\s*(?:\.|->)\s*PacketType|\b(?:packetType|packetHeader|PacketHeader))\b'

RULES = [
    ('header', re.compile(HEADER_SIDE + r'\s*(?:==|!=|=(?!=))\s*\(?\s*(' + LIT + r')\s*\)?' + LONE_END)),
    ('header', re.compile(r'(?<![\w.&|<>=!+\-*/])\s*(' + LIT + r')\s*(?:==|!=)\s*' + HEADER_SIDE)),
    ('table', re.compile(r'\bg_packetLenMap\s*\[\s*(' + LIT + r')\s*\]')),
    ('table', re.compile(r'\b(?:GetPacketSize|IsFixedPacket|bf_packet_name)\s*\(\s*(' + LIT + r')\s*\)')),
    ('init', re.compile(r'\bPACKET_\w+\s+\w+\s*=\s*\{\s*(' + LIT + r')\s*[,}]')),
    ('header', re.compile(r'\bGetPacketType\s*\([^()]*\)\s*(?:==|!=)\s*\(?\s*(' + LIT + r')\s*\)?' + LONE_END)),
    ('store', re.compile(r'\*\s*\(\s*' + RAW16 + r'\s*\*\s*\)\s*\(?\s*\w*(?:buf|Buf|packet|Packet|pkt|Pkt)\w*\s*\)?\s*(?:==|!=)\s*(' + LIT + r')' + LONE_END)),
    ('store', re.compile(r'\*\s*\(\s*(?:WORD|short|unsigned\s+short|USHORT|u_short|uint16_t|int16_t)\s*\*\s*\)\s*\(?\s*\w+\s*\)?\s*=\s*(' + LIT + r')\s*;')),
]
SWITCH = re.compile(r'\bswitch\s*\(')
PACKET_SWITCH = re.compile(r'packetType|PacketType|packetHeader|PacketHeader|GetPacketType')
CASE = re.compile(r'\bcase\s+\(?\s*(' + LIT + r')\s*\)?\s*:')


def blank(text):
    """Comments and string/char literals replaced by spaces; newlines kept."""
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith('//', i):
            j = text.find('\n', i)
            j = n if j < 0 else j
            out.append(' ' * (j - i)); i = j
        elif text.startswith('/*', i):
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append(re.sub(r'[^\n]', ' ', text[i:j])); i = j
        elif c in '"\'':
            j = i + 1
            while j < n and text[j] != c and text[j] != '\n':
                j += 2 if text[j] == '\\' else 1
            j = min(j + 1, n)
            out.append(c + re.sub(r'[^\n]', ' ', text[i + 1:j - 1]) + (c if j - 1 > i else ''))
            i = j
        else:
            out.append(c); i += 1
    return ''.join(out)


def matching(text, i, open_, close):
    """Index just past the bracket that closes the one at text[i]."""
    depth = 0
    for j in range(i, len(text)):
        if text[j] == open_:
            depth += 1
        elif text[j] == close:
            depth -= 1
            if depth == 0:
                return j + 1
    return len(text)


def packet_switch_cases(text):
    """(offset, literal) of every case label belonging to a packet-type switch."""
    found = []
    for m in SWITCH.finditer(text):
        cond_end = matching(text, m.end() - 1, '(', ')')
        if not PACKET_SWITCH.search(text[m.end():cond_end]):
            continue
        body = text.find('{', cond_end)
        if body < 0:
            continue
        end = matching(text, body, '{', '}')
        # Blank nested switches so their labels are not taken for ours.
        inner = list(text[body + 1:end - 1])
        for n in SWITCH.finditer(text, body + 1, end - 1):
            nb = text.find('{', matching(text, n.end() - 1, '(', ')'))
            if nb < 0 or nb >= end:
                continue
            ne = matching(text, nb, '{', '}')
            for k in range(n.start(), ne):
                if text[k] != '\n':
                    inner[k - body - 1] = ' '
        inner = ''.join(inner)
        for c in CASE.finditer(inner):
            found.append((body + 1 + c.start(), c.group(1)))
    return found


def header_names(tree):
    names = {}
    text = open(os.path.join(tree, 'packet.h'), 'rb').read().decode('latin-1')
    for name, value in re.findall(r'\b(HEADER_\w+)\s*=\s*(0x[0-9a-fA-F]+|\d+)\s*,', text):
        names.setdefault(int(value, 0), name)
    return names


def scan_file(path):
    """[(start, end, rule, literal, line)] of the packet ids in one file,
    start/end being the literal's offsets in the file's text."""
    raw = open(path, 'rb').read().decode('latin-1')
    text = blank(raw)
    starts = [0] + [m.end() for m in re.finditer('\n', text)]

    def line_of(off):
        lo, hi = 0, len(starts) - 1
        while lo < hi:
            mid = (lo + hi + 1) // 2
            if starts[mid] <= off:
                lo = mid
            else:
                hi = mid - 1
        return lo + 1

    found, seen = [], set()
    for off, lit in packet_switch_cases(text):
        at = text.index(lit, off)
        found.append((at, at + len(lit), 'case', lit, line_of(at)))
        seen.add(at)
    for rule, rx in RULES:
        for m in rx.finditer(text):
            if m.start(1) in seen:
                continue
            if rule == 'init' and int(m.group(1).rstrip('uUlL'), 0) == 0:
                continue
            seen.add(m.start(1))
            found.append((m.start(1), m.end(1), rule, m.group(1), line_of(m.start(1))))
    found.sort()
    return found


def source_files(tree):
    for base, dirs, files in os.walk(tree):
        dirs.sort()
        for f in sorted(files):
            if f.endswith(('.cpp', '.c', '.h', '.inl')):
                yield os.path.join(base, f)


def scan(tree):
    hits = []
    for path in source_files(tree):
        found = scan_file(path)
        if not found:
            continue
        rel = os.path.relpath(path, os.path.dirname(tree))
        lines = open(path, 'rb').read().decode('latin-1').split('\n')
        for start, end, rule, lit, ln in found:
            hits.append((rel, ln, rule, lit, lines[ln - 1].strip()))
    return hits


SELF_TEST = '''
void f(char *buf, PACKET_X *q, int packetType) {
	switch (packetType) {
	case 120:                                   // FOUND case
		switch (q->type) { case 7: break; }     // nested switch: not ours
		break;
	case HEADER_ZC_SAY_DIALOG: break;
	}
	switch (dwResult) { case 120: break; }      // GameGuard code: not a packet
	switch (c.GetPacketType(buf)) { case 0x9b: break; }   // FOUND case
	q->PacketType = 0x72;                       // FOUND header
	p.PacketType	= 275;                      // FOUND header
	if (packetHeader == 0x25d) {}               // FOUND header
	if (0x25d != p.PacketType) {}               // FOUND header
	if (c.GetPacketType(buf) == 0x8c) {}        // FOUND header
	WORD PacketType = 0x80 & PacketInfo;        // GIF field: not a packet
	name(packetType & 0xffff);                  // mask: not a packet
	g_packetLenMap[0xa7] = 8;                   // FOUND table
	Send(GetPacketSize(0x94), buf);             // FOUND table
	PACKET_CZ_ENTER e = { 0x9b, 1 };            // FOUND init
	PACKET_CZ_ENTER z = {0};                    // cleared: not a packet
	*(WORD *)buf = 0x187;                       // FOUND store
	if (*(unsigned short *)packet != 0x187) {}  // FOUND store
	if (*(unsigned short*)header == 0x5a4d) {}  // "MZ": not a packet
	PUSH_HELP_MSG(0x9b);                        // message id: not a packet
	char *s = "p.PacketType = 0x99;";           // string: not code
	// p.PacketType = 0x99;                        comment: not code
}
'''


def self_test():
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, 'self_test.cpp')
        with open(path, 'w') as f:
            f.write(SELF_TEST)
        found = scan_file(path)
    lines = SELF_TEST.split('\n')
    want = sorted(i + 1 for i, l in enumerate(lines) if 'FOUND' in l)
    got = sorted(ln for _, _, _, _, ln in found)
    for start, end, rule, lit, ln in found:
        print('  line %d: %s %s' % (ln, rule, lit))
    if got != want:
        print('packetids self-test: FAILED, expected lines %s, found %s' % (want, got))
        return 1
    print('packetids self-test: %d instances found, %d exclusions left alone' %
          (len(got), sum(1 for l in lines if ': not' in l or 'not code' in l)))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--tree', default=os.path.join(ROOT, 'src'))
    ap.add_argument('--names', action='store_true', help='show the enumerator each value has in packet.h')
    ap.add_argument('--quiet', action='store_true', help='only the summary line')
    ap.add_argument('--self-test', action='store_true', help='check the rules on a synthetic text')
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    tree = os.path.abspath(args.tree)
    names = header_names(tree) if args.names else {}
    hits = scan(tree)
    if not args.quiet:
        for rel, ln, rule, lit, src in hits:
            extra = ''
            if args.names:
                v = int(lit.rstrip('uUlL'), 0)
                extra = ' [%s]' % names.get(v, 'no name 0x%x' % v)
            print('%s:%d: %s: %s%s' % (rel, ln, rule, src, extra))
    files = len({h[0] for h in hits})
    by_rule = {}
    for h in hits:
        by_rule[h[2]] = by_rule.get(h[2], 0) + 1
    detail = ', '.join('%s %d' % (r, by_rule[r]) for r in sorted(by_rule)) or 'none'
    print('packetids: %d numeric packet id(s) in %d file(s) (%s)' % (len(hits), files, detail))
    return 1 if hits else 0


if __name__ == '__main__':
    sys.exit(main())
