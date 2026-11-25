#!/usr/bin/env python3
"""Source rewrites that make the Visual C++ 6.0 sources acceptable to clang
on a 64-bit target. Each rule is mechanical and leaves the 32-bit meaning
unchanged.

  codemod.py --tree DIR [--rule NAME ...] [--dry-run]

Rules (all run when none is named):

  long32      'long' and 'unsigned long' become 'int' and 'unsigned int'
              ('long long' and 'long double' stay; so does the misspelt
              'double long' of GuildInfo.cpp, which is a double).
              Visual C++ makes long 32 bits wide; macOS makes it 64, which
              changes every packet and file structure that uses it.
  allocator   Drops the explicit std::allocator<V> argument of maps. Visual
              C++ 6.0 wanted the mapped type there; the standard wants the
              pair type, which is the default.
  enumfwd     Gives the forward-declared enumeration WINDOWID a fixed
              underlying type, which standard C++ requires.
  typename    Adds 'typename' in front of dependent type names.
  time_t      The 'time_t' fields of the structures in packet.h become 'int'.
              Visual C++ 6.0 makes time_t a 32-bit long; here it is 64 bits,
              which moves the fields that follow in every packet that carries
              one. Only structure fields in packet.h, not locals or other files.
  ptrcast     Widens casts of pointers to int, at the places the compiler
              reports (needs --diagnostics, a file of clang error output).

Third-party interface headers are left alone unless --all-files is given.
"""
import argparse, os, re, sys

# None left in src/ since M11.11 (Mss.h, BINK.H, RAD.H, radbase.h, granny.h,
# ijl.h, DbgHelp.h and WinCrypt.h were deleted in M11.6, M11.10 and M11.11).
THIRD_PARTY = set()

def is_lua(rel):
    name = os.path.basename(rel)
    return re.fullmatch(r'l[a-z]+\.[ch]|lua\.h|lualib\.h', name) is not None

def rule_long32(text, rel):
    text = re.sub(r'\bunsigned\s+long\b(?!\s+long\b)', 'unsigned int', text)
    text = re.sub(r'\bsigned\s+long\b(?!\s+long\b)', 'int', text)
    text = re.sub(r'(?<!\blong\s)(?<!\bdouble\s)\blong\b(?!\s+(?:long|double)\b)', 'int', text)
    return text.replace('int int ', 'int ')

def rule_allocator(text, rel):
    return re.sub(r',\s*std::allocator<[^<>]*(?:<[^<>]*>[^<>]*)*>\s*>', ' >', text)

def rule_enumfwd(text, rel):
    text = re.sub(r'\benum WINDOWID;', 'enum WINDOWID : int;', text)
    return re.sub(r'\benum WINDOWID\s*\{', 'enum WINDOWID : int {', text)

def rule_typename(text, rel):
    if rel != 'ItemInfo.h':
        return text
    text = text.replace('\tstatic T::referent_type nullDesc;', '\tstatic typename T::referent_type nullDesc;')
    return text.replace('\tT::iterator iter = table.find(itemName);', '\ttypename T::iterator iter = table.find(itemName);')

def rule_time_t(text, rel):
    if rel != 'packet.h':
        return text
    return re.sub(r'(?m)^(\s*)time_t(\s+\w+\s*;)', r'\1int\2', text)

RULES = {'long32': rule_long32, 'allocator': rule_allocator, 'enumfwd': rule_enumfwd, 'typename': rule_typename,
         'time_t': rule_time_t}

def apply_ptrcast(tree, diagnostics, dry_run):
    """Rewrites '(int)expr' and 'int(expr)' to use INT_PTR where clang reported
    'cast from pointer to smaller type'. Returns the number of casts rewritten."""
    sites = {}
    pattern = re.compile(r'^(.*?):(\d+):(\d+): error: cast from pointer to smaller type')
    for line in open(diagnostics, errors='replace'):
        m = pattern.match(line)
        if m:
            path = os.path.realpath(m.group(1))
            sites.setdefault(path, set()).add((int(m.group(2)), int(m.group(3))))
    done = 0
    root = os.path.realpath(tree)
    for path, places in sites.items():
        if not path.startswith(root + os.sep):
            continue
        lines = open(path, 'rb').read().decode('latin-1').split('\n')
        # Rightmost first, so earlier columns stay valid.
        for row, column in sorted(places, reverse=True):
            text = lines[row - 1]
            at = column - 1
            for old in ('(int)', 'int(', '(unsigned int)', '(DWORD)', '(LONG)', '(UINT)'):
                if text.startswith(old, at):
                    new = {'int(': 'INT_PTR('}.get(old, '(INT_PTR)')
                    lines[row - 1] = text[:at] + new + text[at + len(old):]
                    done += 1
                    break
        if not dry_run:
            open(path, 'wb').write('\n'.join(lines).encode('latin-1'))
    return done

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--tree', required=True, help='the src directory to rewrite in place')
    ap.add_argument('--rule', action='append', choices=sorted(RULES) + ['ptrcast'])
    ap.add_argument('--diagnostics', help='clang output, for the ptrcast rule')
    ap.add_argument('--all-files', action='store_true')
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()

    rules = args.rule or sorted(RULES)
    changed = 0
    for directory, subdirs, files in os.walk(args.tree):
        subdirs[:] = [d for d in subdirs if d not in ('library', 'imports', 'res')]
        for name in files:
            if not name.lower().endswith(('.cpp', '.h', '.c')):
                continue
            path = os.path.join(directory, name)
            rel = os.path.relpath(path, args.tree)
            if not args.all_files and (rel in THIRD_PARTY or is_lua(rel)):
                continue
            original = open(path, 'rb').read().decode('latin-1')
            text = original
            for rule in rules:
                if rule in RULES:
                    text = RULES[rule](text, rel)
            if text != original:
                changed += 1
                if not args.dry_run:
                    open(path, 'wb').write(text.encode('latin-1'))
    print(f'{changed} files changed by {", ".join(r for r in rules if r in RULES) or "no text rule"}')

    if 'ptrcast' in rules:
        if not args.diagnostics:
            sys.exit('the ptrcast rule needs --diagnostics')
        print(f'{apply_ptrcast(args.tree, args.diagnostics, args.dry_run)} pointer casts widened')

if __name__ == '__main__':
    main()
