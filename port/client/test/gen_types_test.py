#!/usr/bin/env python3
"""Writes the layout test of bf_types.h against the Wine headers (M11.0b).

  gen_types_test.py --cxx CLANG++ --header bf_types.h --out types_test.cpp
                    [--wine-flags FLAGS] [--flags FLAGS]

Everything the test checks is read from bf_types.h by clang, not listed by
hand: the typedefs, records (and their fields), enums and enumerators from
the AST of the header (its #include lines removed, so only its own
declarations remain), the macros from `clang -E -dD`. The fields of the
Wine records come from clang's record-layout dump of a unit that includes
common.h with the build's flags. Only the sample arguments of the
function-like macros are written here (SAMPLES); a function-like macro
without samples stops the generator, so a new one cannot go unchecked.

The generated unit (compiled with the build's flags, Wine headers included)
sets aside the header's macro names (push_macro), includes bf_types.h inside
namespace bft, captures the value of every macro, restores the names as they
were (the prefix's __max, the C library's NULL), includes common.h as a src/
unit does, and compares at run
time: typedefs by type (std::is_same for scalars; records, enums, pointers
and arrays element by element, records paired by tag), records by size,
alignment, field names, offsets and field types, enumerators by value and
the enum by underlying type, value macros by value and type, the others by
their full expansion (whitespace removed).
"""
import argparse
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile

# Sample arguments of the function-like macros: 'value' compares the result
# (value and type), 'text' the expansion.
SAMPLES = {
    'LOBYTE': ('value', ['0x1234', '0xfffffabc', '-1']),
    'HIBYTE': ('value', ['0x1234', '0xfffffabc', '-1']),
    'LOWORD': ('value', ['0x12345678', '-1', '0x123456789aLL']),
    'HIWORD': ('value', ['0x12345678', '-1', '0x123456789aLL']),
    'MAKEWORD': ('value', ['0x12, 0x34', '0x1ff, 0x2ff', '-1, -2']),
    'MAKELONG': ('value', ['0x1234, 0x5678', '-1, -2', '0x12345, 0x6789a']),
    'MAKEWPARAM': ('value', ['0x1234, 0x5678', '-1, -2']),
    'MAKELPARAM': ('value', ['0x1234, 0x5678', '-1, -2']),
    'MAKELRESULT': ('value', ['0x1234, 0x5678', '-1, -2']),
    'RGB': ('value', ['0x12, 0x34, 0x56', '300, -1, 2', '255, 255, 255']),
    'PALETTERGB': ('value', ['0x12, 0x34, 0x56']),
    'GetRValue': ('value', ['0x123456', '0xffeeddcc', '-1']),
    'GetGValue': ('value', ['0x123456', '0xffeeddcc', '-1']),
    'GetBValue': ('value', ['0x123456', '0xffeeddcc', '-1']),
    '__max': ('value', ['3, 7', '-2, 1', '2.5, 1']),
    '__min': ('value', ['3, 7', '-2, 1', '2.5, 1']),
    'max': ('value', ['3, 7', '-2, 1', '2.5, 1']),
    'min': ('value', ['3, 7', '-2, 1', '2.5, 1']),
    'SUCCEEDED': ('value', ['0', '-1', '0x80004005', '1']),
    'FAILED': ('value', ['0', '-1', '0x80004005', '1']),
    'D3DFVF_TEXCOORDSIZE1': ('value', ['0', '1', '3']),
    'D3DFVF_TEXCOORDSIZE2': ('value', ['0', '1', '3']),
    'D3DFVF_TEXCOORDSIZE3': ('value', ['0', '1', '3']),
    'D3DFVF_TEXCOORDSIZE4': ('value', ['0', '1', '3']),
    'ZeroMemory': ('text', ['p, n']),
    'FillMemory': ('text', ['p, n, c']),
    'CopyMemory': ('text', ['d, s, n']),
    'MoveMemory': ('text', ['d, s, n']),
    '__declspec': ('text', ['dllimport', 'selectany']),
    '_declspec': ('text', ['dllimport']),
}
# Object-like macros whose expansion is not an integer constant.
TEXT_MACROS = {'WINAPI', 'CALLBACK', 'APIENTRY', 'APIPRIVATE', 'PASCAL', 'CDECL', 'WINAPIV',
               'FAR', 'NEAR', '_far', '_near', 'CONST', 'VOID', 'NULL'}
SKIP_MACROS = {'BF_TYPES_H'}


def run(cmd, **kw):
    p = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if p.returncode != 0:
        sys.exit('command failed: %s\n%s' % (' '.join(cmd), p.stderr[-4000:]))
    return p.stdout


def own_decls(cxx, header):
    """Typedefs, records and enums of the header itself."""
    text = open(header).read()
    stripped = re.sub(r'^\s*#\s*include\b.*$', '', text, flags=re.M)
    with tempfile.NamedTemporaryFile('w', suffix='.h', delete=False) as f:
        f.write(stripped)
        tmp = f.name
    try:
        ast = json.loads(run([cxx, '-x', 'c++', '-std=gnu++14', '-target', 'arm64-apple-macos14',
                              '-fsyntax-only', '-Xclang', '-ast-dump=json', tmp]))
        pp = run([cxx, '-x', 'c++', '-std=gnu++14', '-target', 'arm64-apple-macos14', '-E', '-dD',
                  tmp])
    finally:
        os.unlink(tmp)
    typedefs, records, enums = [], [], []
    by_id = {}
    for d in ast.get('inner', []):
        if d.get('isImplicit'):
            continue
        k = d['kind']
        if k == 'TypedefDecl':
            typedefs.append(d['name'])
            owned = None
            for t in d.get('inner', []):
                if 'ownedTagDecl' in t:
                    owned = t['ownedTagDecl']['id']
            if owned and owned in by_id and not by_id[owned].get('name'):
                by_id[owned]['name'] = d['name']  # an anonymous tag, named by its typedef
        elif k in ('CXXRecordDecl', 'RecordDecl') and d.get('completeDefinition'):
            r = {'name': d.get('name'), 'fields': [x['name'] for x in d.get('inner', [])
                                                   if x['kind'] == 'FieldDecl']}
            by_id[d['id']] = r
            records.append(r)
        elif k == 'EnumDecl':
            e = {'name': d.get('name'), 'items': [x['name'] for x in d.get('inner', [])
                                                  if x['kind'] == 'EnumConstantDecl']}
            by_id[d['id']] = e
            enums.append(e)
    macros = []
    cur = None
    for line in pp.splitlines():
        m = re.match(r'# \d+ "([^"]+)"', line)
        if m:
            cur = m.group(1)
            continue
        if cur == tmp and line.startswith('#define '):
            m = re.match(r'#define (\w+)(\()?', line)
            if m.group(1) not in SKIP_MACROS:
                macros.append((m.group(1), bool(m.group(2))))
    return typedefs, records, enums, macros


def wine_fields(cxx, wine_flags, records, workdir):
    """{record name: [field names]} as Wine declares them, from the layout dump."""
    src = os.path.join(workdir, 'wine_layouts.cpp')
    with open(src, 'w') as f:
        f.write('#include "common.h"\n')
        for i, r in enumerate(records):
            f.write('int bft_size_%d = sizeof(::%s);\n' % (i, r['name']))
    out = run([cxx, '-x', 'c++', '-std=gnu++14'] + wine_flags +
              ['-fsyntax-only', '-Xclang', '-fdump-record-layouts', src])
    fields = {}
    for block in out.split('*** Dumping AST Record Layout')[1:]:
        lines = block.strip('\n').splitlines()
        m = re.match(r'\s*\d+ \| (?:(?:struct|union|class) )?(\S+)$', lines[0])
        if not m:
            continue
        name, fl = m.group(1), []
        for l in lines[1:]:
            fm = re.match(r'\s*[\d:-]+ \|   (?! )(.*?)(\w+)$', l)
            if fm:
                fl.append(fm.group(2))
        fields.setdefault(name, fl)
    return fields


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cxx', required=True)
    ap.add_argument('--header', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--wine-flags', required=True, help='the build flags, one shell string')
    o = ap.parse_args()
    wine_flags = shlex.split(o.wine_flags)
    typedefs, records, enums, macros = own_decls(o.cxx, o.header)
    for name, fn in macros:
        if fn and name not in SAMPLES:
            sys.exit('gen_types_test.py: no samples for the function-like macro %s (SAMPLES)' % name)
    wf = wine_fields(o.cxx, wine_flags, records, os.path.dirname(os.path.abspath(o.out)))

    w = []
    w.append('// Generated by port/client/test/gen_types_test.py from %s. Do not edit.'
             % os.path.basename(o.header))
    w.append('#include <stddef.h>\n#include <string.h>\n#include <stdio.h>\n#include <type_traits>')
    # Every macro the header defines is set aside first (the prefix defines
    # __max and __min, the C library NULL) and restored after the capture,
    # so that common.h then sees what a src/ unit sees today.
    for name, fn in macros:
        w.append('#pragma push_macro("%s")\n#undef %s' % (name, name))
    w.append('namespace bft {\n#include "bf_types.h"\n}')
    w.append('#define BFT_STR2(...) #__VA_ARGS__\n#define BFT_STR(...) BFT_STR2(__VA_ARGS__)')
    caps = []  # (name, kind, cpp-id, expression text)
    w.append('namespace bft { namespace cap {')
    n = 0
    for name, fn in macros:
        if fn:
            mode, args = SAMPLES[name]
            for a in args:
                expr = '%s(%s)' % (name, a)
                ident = 'm%d' % n
                n += 1
                caps.append((expr, mode, ident))
        else:
            ident = 'm%d' % n
            n += 1
            caps.append((name, 'text' if name in TEXT_MACROS else 'value', ident))
    for expr, mode, ident in caps:
        if mode == 'value':
            w.append('constexpr auto %s = %s;' % (ident, expr))
        else:
            w.append('static const char *const %s = BFT_STR(%s);' % (ident, expr))
    w.append('}}')
    for name, fn in macros:
        w.append('#undef %s\n#pragma pop_macro("%s")' % (name, name))
    w.append('#undef BF_TYPES_H')
    w.append('#include "common.h"')
    w.append('''
namespace {
int failures, checks;
void check(bool ok, const char *what, const char *name)
{
	checks++;
	if (!ok) {
		failures++;
		printf("FAIL %s %s\\n", what, name);
	}
}
bool same_text(const char *a, const char *b)
{
	for (;;) {
		while (*a == ' ' || *a == '\\t') a++;
		while (*b == ' ' || *b == '\\t') b++;
		if (*a != *b) return false;
		if (!*a) return true;
		a++, b++;
	}
}
template <class A, class B> struct same_tag : std::false_type {};
template <class A, class B, bool = std::is_class<A>::value && std::is_class<B>::value,
          bool = std::is_enum<A>::value && std::is_enum<B>::value>
struct same_shape_ : std::is_same<A, B> {};
template <class A, class B> struct same_shape_<A, B, true, false> : same_tag<A, B> {};
template <class A, class B> struct same_shape_<A, B, false, true> : same_tag<A, B> {};
template <class A, class B> struct same_shape : same_shape_<A, B> {};
template <class A, class B> struct same_shape<A *, B *> : same_shape<A, B> {};
template <class A, class B> struct same_shape<const A, const B> : same_shape<A, B> {};
template <class A, class B> struct same_shape<volatile A, volatile B> : same_shape<A, B> {};
template <class A, class B> struct same_shape<const volatile A, const volatile B> : same_shape<A, B> {};
template <class A, class B, size_t N> struct same_shape<A[N], B[N]> : same_shape<A, B> {};
template <class A, class B> struct same_shape<A &, B &> : same_shape<A, B> {};
template <class A, class B> bool same_value(A a, B b)
{
	return std::is_same<A, B>::value && a == b;
}
}''')
    for r in records:
        w.append('template <> struct same_tag<bft::%s, ::%s> : std::true_type {};' % (r['name'], r['name']))
    for e in enums:
        if e['name']:
            w.append('template <> struct same_tag<bft::%s, ::%s> : std::true_type {};' % (e['name'], e['name']))
    w.append('\nint main()\n{')
    for t in typedefs:
        w.append('\tcheck(same_shape<bft::%s, ::%s>::value, "typedef", "%s");' % (t, t, t))
    nf = 0
    for r in records:
        x = r['name']
        w.append('\tcheck(sizeof(bft::%s) == sizeof(::%s), "sizeof", "%s");' % (x, x, x))
        w.append('\tcheck(alignof(bft::%s) == alignof(::%s), "alignof", "%s");' % (x, x, x))
        theirs = wf.get(x)
        w.append('\tcheck(%s, "field names", "%s: bf %s, Wine %s");' % (
            'true' if theirs == r['fields'] else 'false', x, ' '.join(r['fields']),
            ' '.join(theirs) if theirs is not None else '(no layout)'))
        for f in r['fields']:
            nf += 1
            w.append('\tcheck(offsetof(bft::%s, %s) == offsetof(::%s, %s), "offset", "%s.%s");'
                     % (x, f, x, f, x, f))
            w.append('\tcheck(same_shape<decltype(((bft::%s *)0)->%s), decltype(((::%s *)0)->%s)>::value, '
                     '"field type", "%s.%s");' % (x, f, x, f, x, f))
    ne = 0
    for e in enums:
        if e['name']:
            x = e['name']
            w.append('\tcheck(std::is_same<std::underlying_type<bft::%s>::type, std::underlying_type<::%s>::type>::value, '
                     '"underlying type", "%s");' % (x, x, x))
        for it in e['items']:
            ne += 1
            w.append('\tcheck((long long)bft::%s == (long long)::%s, "enumerator", "%s");' % (it, it, it))
    for expr, mode, ident in caps:
        q = expr.replace('\\', '\\\\').replace('"', '\\"')
        if mode == 'value':
            w.append('\tcheck(same_value(bft::cap::%s, %s), "macro value", "%s");' % (ident, expr, q))
        else:
            w.append('\tcheck(same_text(bft::cap::%s, BFT_STR(%s)), "macro text", "%s");' % (ident, expr, q))
    w.append('\tprintf("%%s: %d typedefs, %d records (%d fields), %d enums (%d enumerators), %d macros '
             '(%d checked expressions): %%d checks, %%d failed\\n", failures ? "FAIL" : "ok", checks, failures);'
             % (len(typedefs), len(records), nf, len(enums), ne, len(macros), len(caps)))
    w.append('\treturn failures ? 1 : 0;\n}')
    with open(o.out, 'w') as f:
        f.write('\n'.join(w) + '\n')


if __name__ == '__main__':
    main()
