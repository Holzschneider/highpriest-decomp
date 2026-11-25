#!/usr/bin/env python3
"""Writes the layout test of bf_types.h (M11.0b; against a reference since
M11.11, when the Wine headers left the build).

  gen_types_test.py --header bf_types.h --lang c++|c --out UNIT
                    --reference wine_types.ref [--cxx CLANG++] [--cc CLANG]
  gen_types_test.py --header bf_types.h --write-reference wine_types.ref
                    [--wine-flags FLAGS] [--cxx CLANG++] [--cc CLANG]

Everything the test checks is read from bf_types.h by clang, not listed by
hand: the typedefs, records (and their fields), enums and enumerators from
the AST of the header (its #include lines removed, so only its own
declarations remain), the macros from `clang -E -dD`, once as C++14 and
once as gnu89 C (the header has a C branch: WCHAR, NULL, no min and max).
Only the sample arguments of the function-like macros are written here
(SAMPLES); a function-like macro without samples stops the generator, so a
new one cannot go unchecked.

A generated unit describes every name as one line, `key<TAB>value`:

  typedef T            the type, spelled out (C++: scalars by name, records
                       and enums by the tag bf_types.h gives them, pointers,
                       arrays, cv, functions element by element; C: size,
                       alignment, clang's type class, signedness)
  record R             size and alignment
  fields R             the field names, in order
  field R.f            offset (and the field's type, C++)
  enum E               the underlying type (C++)
  enumerator X         the value
  macro M / M(args)    value and type of the result (value macros), or the
                       expansion with whitespace removed (TEXT_MACROS,
                       'text' samples)

--write-reference compiles that unit twice per language against the Wine
headers (the flags of port/tools/flags.sh, the Windows includes src/common.h
had before M11.11, BF_WIN_HEADERS or the vendored tree), runs it and writes
what Wine says: the reference, committed as port/client/test/wine_types.ref.
The field names on the Wine side come from clang's record-layout dump.
--reference writes the unit the test builds: bf_types.h alone, with a src/
unit's flags, compared at run time with the reference line by line; it
prints one FAIL line per difference (and per name the reference lacks:
a new name needs the reference rewritten with the Wine headers back, see
port/README.md) and a summary.
"""
import argparse
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))

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
    'MAKELANGID': ('value', ['0x09, 0x01', '0x3ff, 0x3f', '-1, -1']),
    'PRIMARYLANGID': ('value', ['0x0409', '0xffff', '-1']),
    'SUBLANGID': ('value', ['0x0409', '0xffff', '-1']),
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

# What a src/ unit read through common.h before M11.11 replaced these lines
# with bf_types.h: the Wine side of the reference.
WINE_INCLUDES = '''#define DIRECTINPUT_VERSION 0x700
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#define D3D_OVERLOADS
#include <d3d.h>
#include <dsound.h>
#include <dinput.h>
#include <shlwapi.h>
'''
WINE_INCLUDES_CXX = WINE_INCLUDES + '''#include <string>
#include <map>
#include <algorithm>
#include <vector>
#include <list>
using namespace std;
'''
INTEGER = re.compile(r'^(const )?((un)?signed |)(char|short|int|long|long long|char16_t|wchar_t|'
                     r'unsigned|_Bool|bool)$')


def run(cmd, **kw):
    p = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if p.returncode != 0:
        sys.exit('command failed: %s\n%s' % (' '.join(cmd), p.stderr[-4000:]))
    return p.stdout


def lang_args(lang):
    return ['-x', 'c++', '-std=gnu++14'] if lang == 'c++' else ['-x', 'c', '-std=gnu89']


def own_decls(compiler, header, lang):
    """Typedefs (with their canonical type), records, enums and macros of the
    header itself, as the language sees them."""
    text = open(header).read()
    stripped = re.sub(r'^\s*#\s*include\b.*$', '', text, flags=re.M)
    with tempfile.NamedTemporaryFile('w', suffix='.h', delete=False) as f:
        f.write(stripped)
        tmp = f.name
    base = [compiler] + lang_args(lang) + ['-target', 'arm64-apple-macos14']
    try:
        ast = json.loads(run(base + ['-fsyntax-only', '-Xclang', '-ast-dump=json', tmp]))
        pp = run(base + ['-E', '-dD', tmp])
    finally:
        os.unlink(tmp)
    typedefs, records, enums = [], [], []
    by_id = {}
    for d in ast.get('inner', []):
        if d.get('isImplicit'):
            continue
        k = d['kind']
        if k == 'TypedefDecl':
            t = d.get('type', {})
            typedefs.append((d['name'], t.get('desugaredQualType', t.get('qualType', ''))))
            owned = None
            for x in d.get('inner', []):
                if 'ownedTagDecl' in x:
                    owned = x['ownedTagDecl']['id']
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
    for name, fn in macros:
        if fn and name not in SAMPLES:
            sys.exit('gen_types_test.py: no samples for the function-like macro %s (SAMPLES)' % name)
    return typedefs, [r for r in records if r['name']], enums, macros


def wine_fields(compiler, lang, wine_flags, records, workdir):
    """{record name: [field names]} as Wine declares them, from the layout dump."""
    src = os.path.join(workdir, 'wine_layouts.' + ('cpp' if lang == 'c++' else 'c'))
    with open(src, 'w') as f:
        f.write(WINE_INCLUDES_CXX if lang == 'c++' else WINE_INCLUDES)
        for i, r in enumerate(records):
            f.write('int bft_size_%d = sizeof(%s);\n' % (i, record_type(r['name'], lang)))
    out = run([compiler] + lang_args(lang) + wine_flags +
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


_tags = {}


def record_type(name, lang):
    """How C names the record: by its typedef when it has one, else by tag."""
    return name if lang == 'c++' or name in _tags.get('typedefs', ()) else 'struct ' + name


def c_str(s):
    return '"%s"' % s.replace('\\', '\\\\').replace('"', '\\"')


CXX_SUPPORT = r'''
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <type_traits>
namespace bft {
template <class T> struct tag { static const char *name() { return 0; } };
template <class T, class = void> struct complete : std::false_type {};
template <class T> struct complete<T, decltype(void(sizeof(T)))> : std::true_type {};
template <class T> struct desc;
template <class T, bool = std::is_enum<T>::value> struct enum_desc { static std::string s() { return "?"; } };
template <class T> struct enum_desc<T, true> {
	static std::string s() { return "enum(" + desc<typename std::underlying_type<T>::type>::s() + ")"; }
};
template <class T> std::string size_of(std::true_type) { return std::to_string(sizeof(T)); }
template <class T> std::string size_of(std::false_type) { return "incomplete"; }
#define BFT_NAME(t) if (std::is_same<T, t>::value) return #t;
template <class T> struct desc {
	static std::string s()
	{
		BFT_NAME(void) BFT_NAME(bool) BFT_NAME(char) BFT_NAME(signed char) BFT_NAME(unsigned char)
		BFT_NAME(wchar_t) BFT_NAME(char16_t) BFT_NAME(char32_t) BFT_NAME(short) BFT_NAME(unsigned short)
		BFT_NAME(int) BFT_NAME(unsigned int) BFT_NAME(long) BFT_NAME(unsigned long) BFT_NAME(long long)
		BFT_NAME(unsigned long long) BFT_NAME(float) BFT_NAME(double) BFT_NAME(long double)
		BFT_NAME(decltype(nullptr))
		if (tag<T>::name())
			return std::string(std::is_enum<T>::value ? "enum " : std::is_union<T>::value ? "union " : "struct ")
				+ tag<T>::name();
		if (std::is_enum<T>::value)
			return enum_desc<T>::s();
		if (std::is_class<T>::value || std::is_union<T>::value)
			return std::string(std::is_union<T>::value ? "union" : "struct") + "(size "
				+ size_of<T>(complete<T>()) + ")";
		return "?";
	}
};
#undef BFT_NAME
template <class T> struct desc<T *> { static std::string s() { return desc<T>::s() + " *"; } };
template <class T> struct desc<T &> { static std::string s() { return desc<T>::s() + " &"; } };
template <class T> struct desc<const T> { static std::string s() { return "const " + desc<T>::s(); } };
template <class T> struct desc<volatile T> { static std::string s() { return "volatile " + desc<T>::s(); } };
template <class T> struct desc<const volatile T> { static std::string s() { return "const volatile " + desc<T>::s(); } };
template <class T, size_t N> struct desc<T[N]> { static std::string s() { return desc<T>::s() + "[" + std::to_string(N) + "]"; } };
template <class T, size_t N> struct desc<const T[N]> { static std::string s() { return "const " + desc<T>::s() + "[" + std::to_string(N) + "]"; } };
template <class... A> struct args;
template <> struct args<> { static std::string s() { return ""; } };
template <class A> struct args<A> { static std::string s() { return desc<A>::s(); } };
template <class A, class B, class... C> struct args<A, B, C...> { static std::string s() { return desc<A>::s() + ", " + args<B, C...>::s(); } };
template <class R, class... A> struct desc<R(A...)> { static std::string s() { return desc<R>::s() + "(" + args<A...>::s() + ")"; } };
template <class R, class... A> struct desc<R(A..., ...)> { static std::string s() { return desc<R>::s() + "(" + args<A...>::s() + ", ...)"; } };
template <class T> std::string number(T v, std::true_type) { return std::is_signed<T>::value ? std::to_string((long long)v) : std::to_string((unsigned long long)v); }
template <class T> std::string number(T v, std::false_type) { char b[64]; snprintf(b, sizeof b, "%.17g", (double)v); return b; }
template <class T> std::string value(T v)
{
	typedef typename std::conditional<std::is_enum<T>::value, long long, T>::type N;
	return "type " + desc<T>::s() + " value " + number((N)v, std::integral_constant<bool, std::is_integral<N>::value>());
}
}
#define BFT_STR2(...) #__VA_ARGS__
#define BFT_STR(...) BFT_STR2(__VA_ARGS__)
'''

C_SUPPORT = r'''
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define BFT_STR2(x) #x
#define BFT_STR(x) BFT_STR2(x)
'''

RUN_SUPPORT = r'''
static int bft_failures, bft_checks;
static void bft_line(const char *key, const char *value, const char *want)
{
#ifdef BFT_DUMP
	printf("%s\t%s\n", key, value);
	(void)want;
#else
	bft_checks++;
	if (!want) {
		bft_failures++;
		printf("FAIL %s: bf_types.h has it, the reference does not (bf: %s)\n", key, value);
	} else if (strcmp(value, want)) {
		bft_failures++;
		printf("FAIL %s: bf %s, Wine %s\n", key, value, want);
	}
#endif
}
static const char *bft_text(char *buf, size_t size, const char *text)
{
	size_t n = 0;
	for (; *text && n + 1 < size; text++)
		if (*text != ' ' && *text != '\t')
			buf[n++] = *text;
	buf[n] = 0;
	return buf;
}
'''


def generate(lang, header, decls, dump, ref, fields_of):
    """The unit, as text. ref: {key: value} (None for the dump)."""
    typedefs, records, enums, macros = decls
    cxx = lang == 'c++'
    _tags['typedefs'] = {t for t, _ in typedefs}
    w = ['// Generated by port/client/test/gen_types_test.py from %s (%s, %s). Do not edit.'
         % (os.path.basename(header), lang, 'the Wine side, for the reference' if dump
            else 'checked against the reference')]
    if dump:
        w.append('#define BFT_DUMP 1')
        w.append(WINE_INCLUDES_CXX if cxx else WINE_INCLUDES)
    else:
        w.append('#include "bf_types.h"')
    w.append(CXX_SUPPORT if cxx else C_SUPPORT)
    w.append(RUN_SUPPORT)
    if cxx:
        for r in records:
            w.append('template <> struct bft::tag<%s> { static const char *name() { return "%s"; } };'
                     % (r['name'], r['name']))
        for e in enums:
            if e['name']:
                w.append('template <> struct bft::tag<%s> { static const char *name() { return "%s"; } };'
                         % (e['name'], e['name']))
    keys = []

    def emit(key, expr_cxx, expr_c_fmt=None):
        """expr_cxx: a std::string expression (C++); expr_c_fmt: (format, args) (C)."""
        keys.append(key)
        want = c_str(ref[key]) if ref is not None and key in ref else '0'
        if cxx:
            w.append('\tbft_line(%s, (%s).c_str(), %s);' % (c_str(key), expr_cxx, want))
        else:
            fmt, args = expr_c_fmt
            w.append('\tsnprintf(buf, sizeof buf, %s, %s); bft_line(%s, buf, %s);'
                     % (c_str(fmt), ', '.join(args), c_str(key), want))

    w.append('\nint main(void)\n{')
    w.append('\tchar buf[512], text[512];\n\t(void)text;')
    for t, canon in typedefs:
        key = 'typedef %s' % t
        if cxx:
            emit(key, 'bft::desc<%s>::s()' % t)
        elif INTEGER.match(canon.strip()):
            emit(key, None, ('size %d align %d class %d signed %d',
                             ['(int)sizeof(%s)' % t, '(int)__alignof__(%s)' % t,
                              '__builtin_classify_type(*(%s *)0)' % t, '(int)((%s)-1 < (%s)0)' % (t, t)]))
        elif canon.strip() == 'void':
            emit(key, None, ('void%s', ['""']))
        else:
            emit(key, None, ('size %d align %d class %d',
                             ['(int)sizeof(%s)' % t, '(int)__alignof__(%s)' % t,
                              '__builtin_classify_type(*(%s *)0)' % t]))
    for r in records:
        x = r['name']
        rt = record_type(x, lang)
        emit('record %s' % x, 'std::string("size ") + std::to_string(sizeof(%s)) + " align " + '
             'std::to_string(alignof(%s))' % (x, x),
             ('size %d align %d', ['(int)sizeof(%s)' % rt, '(int)__alignof__(%s)' % rt]))
        names = fields_of(r)
        emit('fields %s' % x, 'std::string(%s)' % c_str(' '.join(names)), ('%s', [c_str(' '.join(names))]))
        for f in r['fields']:
            emit('field %s.%s' % (x, f),
                 'std::string("offset ") + std::to_string(offsetof(%s, %s)) + " type " + '
                 'bft::desc<decltype(((%s *)0)->%s)>::s()' % (x, f, x, f),
                 ('offset %d', ['(int)offsetof(%s, %s)' % (rt, f)]))
    for e in enums:
        if e['name'] and cxx:
            emit('enum %s' % e['name'], 'bft::desc<std::underlying_type<%s>::type>::s()' % e['name'])
        for it in e['items']:
            emit('enumerator %s' % it, 'std::to_string((long long)%s)' % it, ('%lld', ['(long long)%s' % it]))
    for name, fn in macros:
        if fn:
            mode, samples = SAMPLES[name]
            exprs = ['%s(%s)' % (name, a) for a in samples]
        else:
            mode, exprs = ('text' if name in TEXT_MACROS else 'value'), [name]
        for expr in exprs:
            key = 'macro %s' % expr
            if mode == 'text':
                emit(key, 'std::string(bft_text(text, sizeof text, BFT_STR(%s)))' % expr,
                     ('%s', ['bft_text(text, sizeof text, BFT_STR(%s))' % expr]))
            else:
                emit(key, 'bft::value(%s)' % expr,
                     ('class %d size %d signed %d value %s',
                      ['__builtin_classify_type(%s)' % expr, '(int)sizeof(%s)' % expr,
                       '(int)(((0 ? (%s) : 0) - 1) < 0)' % expr,
                       '__builtin_classify_type(%s) == 8 ? (snprintf(text, sizeof text, "%%.17g", (double)(%s)), text) : '
                       '(((0 ? (%s) : 0) - 1) < 0) ? (snprintf(text, sizeof text, "%%lld", (long long)(%s)), text) : '
                       '(snprintf(text, sizeof text, "%%llu", (unsigned long long)(%s)), text)'
                       % (expr, expr, expr, expr, expr)]))
    if ref is not None:
        dropped = sorted(set(ref) - set(keys))
        w.append('\t/* in the reference, no longer in bf_types.h: %d */' % len(dropped))
        for k in dropped:
            w.append('\tprintf("note: %%s is in the reference but no longer in bf_types.h\\n", %s);' % c_str(k))
    w.append('#ifndef BFT_DUMP')
    w.append('\tprintf("%%s: %s, %d typedefs, %d records, %d enums, %d macros: %%d checks against the '
             'reference, %%d failed\\n", bft_failures ? "FAIL" : "ok", bft_checks, bft_failures);'
             % (lang, len(typedefs), len(records), len(enums), len(macros)))
    w.append('\treturn bft_failures ? 1 : 0;\n#else\n\treturn 0;\n#endif\n}')
    return '\n'.join(w) + '\n'


def read_reference(path, lang):
    ref, section = {}, None
    for line in open(path):
        line = line.rstrip('\n')
        if line.startswith('[') and line.endswith(']'):
            section = line[1:-1]
        elif line and not line.startswith('#') and section == lang:
            k, _, v = line.partition('\t')
            ref[k] = v
    return ref


def wine_flags_from_build():
    """The flags of a src/ unit, from flags.sh, with the Wine headers."""
    wine = os.environ.get('BF_WIN_HEADERS') or os.path.join(ROOT, 'port', 'compat', 'winheaders')
    if not os.path.isfile(os.path.join(wine, 'windows.h')):
        sys.exit('gen_types_test.py: no windows.h in %s; set BF_WIN_HEADERS (port/README.md says how '
                 'to get the Wine headers back from history)' % wine)
    out = run(['bash', '-c', '. "$ROOT/port/tools/flags.sh" && printf "%s\\n" "$BF_TARGET $BF_DEFINES $BF_INCLUDES"'],
              env=dict(os.environ, ROOT=ROOT))
    return shlex.split(out.strip()) + ['-I' + os.path.join(ROOT, 'src'), '-isystem', wine]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cxx', default='clang++')
    ap.add_argument('--cc', default='clang')
    ap.add_argument('--header', default=os.path.join(ROOT, 'port', 'client', 'include', 'bf_types.h'))
    ap.add_argument('--lang', choices=('c++', 'c'), default='c++')
    ap.add_argument('--out', help='the unit to write (with --reference)')
    ap.add_argument('--reference', help='the reference the unit compares with')
    ap.add_argument('--write-reference', metavar='REF', help='run the Wine side and write the reference')
    ap.add_argument('--wine-flags', help='the build flags with the Wine headers, one shell string '
                    '(default: from port/tools/flags.sh)')
    o = ap.parse_args()
    if o.write_reference:
        flags = shlex.split(o.wine_flags) if o.wine_flags else wine_flags_from_build()
        work = tempfile.mkdtemp(prefix='bft-')
        text = ['# What the Wine headers give each name of bf_types.h, as the build compiled',
                '# them before M11.11 (arm64, the flags of port/tools/flags.sh). Written by',
                '# port/client/test/gen_types_test.py --write-reference; do not edit.']
        for lang in ('c++', 'c'):
            compiler = o.cxx if lang == 'c++' else o.cc
            decls = own_decls(compiler, o.header, lang)
            wf = wine_fields(compiler, lang, flags, decls[1], work)
            missing = [r['name'] for r in decls[1] if r['name'] not in wf]
            if missing:
                sys.exit('gen_types_test.py: no Wine layout for %s' % ', '.join(missing))
            unit = os.path.join(work, 'dump.' + ('cpp' if lang == 'c++' else 'c'))
            open(unit, 'w').write(generate(lang, o.header, decls, True, None, lambda r: wf[r['name']]))
            exe = os.path.join(work, 'dump-' + lang.replace('+', 'x'))
            run([compiler] + lang_args(lang) + flags + [unit, '-o', exe])
            text.append('[%s]' % lang)
            text.extend(run([exe]).rstrip('\n').split('\n'))
        open(o.write_reference, 'w').write('\n'.join(text) + '\n')
        print('gen_types_test.py: %d lines in %s' % (len(text), o.write_reference))
        return
    if not (o.reference and o.out):
        ap.error('--reference and --out, or --write-reference')
    compiler = o.cxx if o.lang == 'c++' else o.cc
    decls = own_decls(compiler, o.header, o.lang)
    ref = read_reference(o.reference, o.lang)
    with open(o.out, 'w') as f:
        f.write(generate(o.lang, o.header, decls, False, ref, lambda r: r['fields']))


if __name__ == '__main__':
    main()
