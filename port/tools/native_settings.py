#!/usr/bin/env python3
"""Rewrites the client's registry calls to its settings keys (M11.4).

  native_settings.py [--tree DIR] [--dry-run] [FILE ...]

In the client files that keep their settings in the registry (FILES, by
default the list below, relative to the tree), every call of the registry
on HKEY_LOCAL_MACHINE goes to port/client/include/bf_setting_key.h, which
maps the same paths onto bf_settings as the registry adapter of stage A did
(port/compat/win32/src/bf_win32_registry.cpp):

  RegOpenKeyEx(HKEY_LOCAL_MACHINE, P, 0, access, &K)       bf_setting_open(K, P)
  RegCreateKeyEx(HKEY_LOCAL_MACHINE, P, ..., &K, &D)       bf_setting_create(K, P)
  RegCloseKey(K)                                           bf_setting_close(K)
  RegDeleteKey(HKEY_LOCAL_MACHINE, P)                      bf_setting_remove(P)
  RegSetValueEx(K, N, 0, REG_DWORD, (LPBYTE)&X, 4)         bf_setting_set(K, N, X)
  RegSetValueEx(K, N, 0, REG_SZ, (LPBYTE)S, n)             bf_setting_set_string(K, N, S, n)
  RegSetValueEx(K, N, 0, other, D, n)                      bf_setting_write(K, N, BF_SETTING_..., D, n)
  RegQueryValueEx(K, N, 0, &T, (LPBYTE)&X, &L)             bf_setting_get(K, N, X)        when L is 4 here
  RegQueryValueEx(K, N, 0, &T, D, &L)                      bf_setting_read(K, N, D, &L)   otherwise

(each with or without the A suffix; sizeof(DWORD) counts as 4). The
one-byte members of NARROW, which the client reads and writes as 4 bytes,
keep that with bf_setting_write(..., BF_SETTING_INT, &X, 4) and
bf_setting_read(..., &X, &L). The new
calls return true where the registry returned ERROR_SUCCESS, so a test of
the result is rewritten with it: `call != ERROR_SUCCESS` (or `!= 0`) becomes
`!call`, `== ERROR_SUCCESS` (`== 0`) becomes `call`; a variable that holds
such a result (`LRESULT lResult = RegOpenKeyExA(...)`) becomes `bool`, and
its tests in the same function are rewritten the same way.

"L is 4 here": the nearest assignment to the length variable above the
call in the same function is 4 or sizeof(DWORD), and every query between
them is itself rewritten to bf_setting_get. A query of an integer leaves
the length at 4, so the value read is the same; bf_setting_get checks at
compile time that X has 4 bytes, as bf_setting_set does for writes. Any
other query keeps its length variable and its in/out size.

Declarations follow: `HKEY` becomes `bf_setting_key`; the type and length
variables (`DWORD REG_TYPE`, `DWORD DATALENGTH`) and the disposition
(`DWORD dwDisposition`) are deleted with their assignments once nothing
else in the function uses them; `#include "bf_setting_key.h"` goes after
`#include "common.h"`. Nothing else on a line changes.

Running the tool again on a rewritten tree changes nothing. It refuses
(exit 1, nothing written) when a call does not have one of the shapes
above.
"""
import argparse
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
FILES = [
    'UIWindowMgr.cpp', 'Session.cpp', 'Session2.cpp', 'GameMode.cpp', 'UIFrameWnd2.cpp',
    'UIFrameWnd3.cpp', 'window/UIFrameWnd.cpp', 'window/UIEmotionWnd.cpp', 'WinMain.cpp',
    'movie/CBink.cpp',
]
INCLUDE = '#include "bf_setting_key.h"'
CALL = re.compile(r'\bReg(OpenKeyEx|CreateKeyEx|QueryValueEx|SetValueEx|CloseKey|DeleteKey)A?\s*\(')
FOUR = re.compile(r'^(4|0x4|sizeof\s*\(\s*DWORD\s*\))$')
CAST = re.compile(r'^\(\s*(LPBYTE|BYTE\s*\*|const\s+BYTE\s*\*|LPCBYTE)\s*\)\s*')
LVALUE = re.compile(r'^[A-Za-z_]\w*(\s*(\.|->)\s*[A-Za-z_]\w*|\[[^\[\]]+\])*$')
# One-byte members the 2008 client reads and writes as 4 bytes (a REG_DWORD
# at their address): the value stored holds the bytes after them too, and a
# read sets those bytes again. Kept byte for byte with the general calls
# (bf_setting_get/set would refuse them at compile time).
NARROW = {'m_chatWndStickOn', 'm_onStSubChat', 'm_onBtSubChat', 'g_session.m_battleMsgOnOffSet[i]'}
TYPES = {'REG_DWORD': 'BF_SETTING_INT', 'REG_SZ': 'BF_SETTING_STRING', 'REG_EXPAND_SZ': 'BF_SETTING_STRING',
         'REG_BINARY': 'BF_SETTING_DATA'}


class Refused(Exception):
    pass


def close_paren(text, open_at):
    """Index of the parenthesis closing the one at open_at (strings and chars skipped)."""
    depth, i = 0, open_at
    while i < len(text):
        c = text[i]
        if c in '"\'':
            j = i + 1
            while text[j] != c:
                j += 2 if text[j] == '\\' else 1
            i = j
        elif c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise Refused('unbalanced call')


def split_args(text):
    args, depth, cur, i = [], 0, '', 0
    while i < len(text):
        c = text[i]
        if c in '"\'':
            j = i + 1
            while text[j] != c:
                j += 2 if text[j] == '\\' else 1
            cur += text[i:j + 1]
            i = j + 1
            continue
        if c in '([':
            depth += 1
        elif c in ')]':
            depth -= 1
        if c == ',' and depth == 0:
            args.append(cur.strip())
            cur = ''
        else:
            cur += c
        i += 1
    args.append(cur.strip())
    return args


def strip_parens(expr):
    expr = expr.strip()
    while expr.startswith('(') and close_paren(expr, 0) == len(expr) - 1:
        expr = expr[1:-1].strip()
    return expr


def uncast(expr):
    return strip_parens(CAST.sub('', expr.strip()))


def address_of(expr):
    """X for (LPBYTE)&X, (LPBYTE)(&X); None for anything else."""
    e = uncast(expr)
    if not e.startswith('&'):
        return None
    x = strip_parens(e[1:])
    return x if LVALUE.match(x) else None


def key_of(arg):
    a = arg.strip()
    if not a.startswith('&'):
        raise Refused('key argument %r is not &K' % arg)
    return a[1:].strip()


def functions(text):
    """(start, end) of every function: from after one "}" at column 0 to the next."""
    out, start = [], 0
    for m in re.finditer(r'^}', text, re.M):
        out.append((start, m.end()))
        start = m.end()
    return out


def rewrite_function(body, stats):
    """The body of one function, rewritten."""
    calls = []
    for m in CALL.finditer(body):
        end = close_paren(body, m.end() - 1)
        calls.append((m.start(), end + 1, m.group(1), split_args(body[m.end():end])))
    if not calls:
        return body

    # The length variable's state at each query: assignments and queries in text order.
    events = []
    for m in re.finditer(r'(?<![\w.>&])([A-Za-z_]\w*)\s*=\s*([^;=][^;]*);', body):
        events.append((m.start(), 'assign', m.group(1), m.group(2).strip()))
    for c in calls:
        events.append((c[0], 'call', c))
    events.sort(key=lambda e: e[0])
    four = {}            # length variable -> 4 known
    helpers = set()      # the type, length and disposition variables the calls name
    replacement = {}     # call start -> new text
    for ev in events:
        if ev[1] == 'assign':
            four[ev[2]] = bool(FOUR.match(ev[3]))
            continue
        start, end, name, args = ev[2]
        if name == 'OpenKeyEx':
            if len(args) != 5 or args[0] != 'HKEY_LOCAL_MACHINE':
                raise Refused('RegOpenKeyEx%r' % (args,))
            new = 'bf_setting_open(%s, %s)' % (key_of(args[4]), args[1])
        elif name == 'CreateKeyEx':
            if len(args) != 9 or args[0] != 'HKEY_LOCAL_MACHINE':
                raise Refused('RegCreateKeyEx%r' % (args,))
            new = 'bf_setting_create(%s, %s)' % (key_of(args[7]), args[1])
            helpers.add(args[8].lstrip('&').strip())
        elif name == 'CloseKey':
            if len(args) != 1:
                raise Refused('RegCloseKey%r' % (args,))
            new = 'bf_setting_close(%s)' % args[0]
        elif name == 'DeleteKey':
            if len(args) != 2 or args[0] != 'HKEY_LOCAL_MACHINE':
                raise Refused('RegDeleteKey%r' % (args,))
            new = 'bf_setting_remove(%s)' % args[1]
        elif name == 'SetValueEx':
            if len(args) != 6:
                raise Refused('RegSetValueEx%r' % (args,))
            k, n, _, t, d, s = args
            x = address_of(d)
            if t == 'REG_DWORD' and x in NARROW and FOUR.match(s):
                new = 'bf_setting_write(%s, %s, BF_SETTING_INT, &%s, %s)' % (k, n, x, s)
            elif t == 'REG_DWORD' and x and FOUR.match(s):
                new = 'bf_setting_set(%s, %s, %s)' % (k, n, x)
            elif t in ('REG_SZ', 'REG_EXPAND_SZ'):
                new = 'bf_setting_set_string(%s, %s, %s, %s)' % (k, n, uncast(d), s)
            elif t in TYPES:
                new = 'bf_setting_write(%s, %s, %s, %s, %s)' % (k, n, TYPES[t], uncast(d), s)
            else:
                raise Refused('RegSetValueEx type %r' % t)
        else:  # QueryValueEx
            if len(args) != 6 or not args[5].startswith('&'):
                raise Refused('RegQueryValueEx%r' % (args,))
            k, n, _, t, d, l = args
            length = l[1:].strip()
            helpers.update((t.lstrip('&').strip(), length))
            x = address_of(d)
            if x and x not in NARROW and four.get(length):
                new = 'bf_setting_get(%s, %s, %s)' % (k, n, x)
            else:
                new = 'bf_setting_read(%s, %s, %s, %s)' % (k, n, uncast(d), l)
                four[length] = False   # unknown after a read of any size
        replacement[start] = (end, new, name)

    # Splice, rewriting a comparison of the result that follows the call.
    out, pos, results = '', 0, set()
    for start in sorted(replacement):
        end, new, name = replacement[start]
        out += body[pos:start]
        tail = re.match(r'\s*(==|!=)\s*(ERROR_SUCCESS|0)\b', body[end:])
        if tail:
            new = new if tail.group(1) == '==' else '!' + new
            end += tail.end()
        else:
            m = re.search(r'(\b(LRESULT|LONG)\s+)?([A-Za-z_]\w*)\s*=\s*$', out)
            if m:
                results.add(m.group(3))
                if m.group(1):
                    out = out[:m.start()] + 'bool ' + out[m.start(1) + len(m.group(1)):]
        out += new
        pos = end
        stats[name] = stats.get(name, 0) + 1
    out += body[pos:]

    # The variables that held results: bool, and their tests.
    for var in sorted(results):
        assigned = re.findall(r'\b%s\s*=(?!=)\s*([^;]*);' % re.escape(var), out)
        if any(not a.lstrip('!').startswith('bf_setting_') for a in assigned):
            raise Refused('%s also holds something else' % var)
        out = re.sub(r'\b(LRESULT|LONG)(\s+)%s\s*;' % re.escape(var), r'bool\2%s;' % var, out)
        out = re.sub(r'\b%s\s*==\s*(ERROR_SUCCESS|0)\b' % re.escape(var), var, out)
        out = re.sub(r'\b%s\s*!=\s*(ERROR_SUCCESS|0)\b' % re.escape(var), '!' + var, out)

    out = re.sub(r'\bHKEY(?=\s+[A-Za-z_])', 'bf_setting_key', out)

    # Variables left with nothing to do: declarations and assignments go.
    for var in sorted(helpers):
        uses = [m for m in re.finditer(r'\b%s\b' % re.escape(var), out)]
        dead = re.compile(r'^[ \t]*(DWORD[ \t]+)?%s([ \t]*=[ \t]*[^;\n]*)?;[ \t]*\n' % re.escape(var), re.M)
        lines = list(dead.finditer(out))
        if uses and len(uses) == len(lines) and any(l.group(1) for l in lines):
            out = dead.sub('', out)
            stats['variables'] = stats.get('variables', 0) + 1
    return out


def rewrite(text, stats):
    pieces, pos = [], 0
    for start, end in functions(text):
        pieces.append(text[pos:start])
        pieces.append(rewrite_function(text[start:end], stats))
        pos = end
    pieces.append(text[pos:])
    new = ''.join(pieces)
    if CALL.search(new):
        raise Refused('a registry call outside a function body')
    if new != text and INCLUDE not in new:
        new, n = re.subn(r'^#include "common.h"\n', '#include "common.h"\n' + INCLUDE + '\n', new, count=1, flags=re.M)
        if n != 1:
            raise Refused('no #include "common.h" for the new include')
    return new


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--tree', default=os.path.join(ROOT, 'src'))
    ap.add_argument('--dry-run', action='store_true', help='count the rewrites, write nothing')
    ap.add_argument('files', nargs='*', default=FILES)
    args = ap.parse_args()
    done = {}
    for rel in args.files:
        path = os.path.join(args.tree, rel)
        text = open(path, 'rb').read().decode('latin-1')
        stats = {}
        try:
            new = rewrite(text, stats)
        except Refused as e:
            sys.exit('%s: %s' % (rel, e))
        done[rel] = (path, text, new, stats)
    for rel, (path, text, new, stats) in done.items():
        if new == text:
            continue
        print('%s: %s' % (rel, ', '.join('%s %d' % kv for kv in sorted(stats.items()))))
        if not args.dry_run:
            open(path, 'wb').write(new.encode('latin-1'))
    return 0


if __name__ == '__main__':
    sys.exit(main())
