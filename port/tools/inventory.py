#!/usr/bin/env python3
"""Lists every place outside the single-player files that still refers to the
single-player game HighPriest.

  port/tools/inventory.py [--tree DIR] [--names-from REV] [--names]
                          [--markdown [--verdicts FILE]]

The single-player game lives in five files: HP_Battle.cpp, HP_BuySell.cpp,
HP_RestTravell.cpp, HP_SaveLoad.cpp and HP_Tetris.cpp/.h. The names it is
made of are taken from those files mechanically, as they are at the git
revision --names-from (default main, which keeps the game), so the tool still
works once the files are gone:

  functions  every function defined at file scope in the five .cpp files
             (for methods, the method name), and every class defined in
             HP_Tetris.h. A name also defined outside the five files is left
             out.
  globals    every variable the five .cpp files declare at file scope, with
             or without 'extern', that is named in at most four other files.
             The online client's own globals (g_session, g_windowMgr, ...)
             are named in dozens of files and drop out by this rule.
  tagged     identifiers the five files use that carry the game's tag, 'HP_'
             at the start or '_HP' at the end, and are defined outside them
             (CSession::HP_AddItem, CGameMode::Zc_Notify_Entry_HP, ...), and
             every function, type or file-scope constant defined with such a
             name anywhere.
  read       functions of the online classes whose whole body is
             single-player, found by reading (READ below); they only seed:
  closure    functions defined outside the five files, class members and
             file-scope variables that the single-player code uses and
             nothing else does: every other line that names them is a
             declaration, their own definition, or inside their own body or
             a single-player body. Repeated until nothing is added.
  patterns   GetGameActorByAid/GetGameActorByAID with a literal id: the game
             gives its actors fixed ids, an online client only knows ids the
             server sent.
  strings    string literals that name the game: 'HighPriest' in any case,
             'data_hp', or a name ending in '_HP'.

Every line of --tree (default src) outside the five files that mentions one
of those names in code (not in comments) is reported with its file, line and
enclosing function; so are the five files if present and the lines of
sources.mk that list them. Exit status 0 when nothing is found, 1 otherwise.

  --names     print the derived name lists instead of the sites
  --markdown  print the sites as a Markdown table, one row per enclosing
              function, with the verdicts of --verdicts (lines of
              'file enclosing-function<TAB>verdict<TAB>note')
"""
import argparse, os, re, subprocess, sys
from collections import OrderedDict, defaultdict

HP_FILES = ['HP_Battle.cpp', 'HP_BuySell.cpp', 'HP_RestTravell.cpp', 'HP_SaveLoad.cpp',
            'HP_Tetris.cpp', 'HP_Tetris.h']
GLOBAL_MAX_FILES = 4
SOURCE_EXT = ('.cpp', '.c', '.h', '.hpp', '.inl')
CONTROL = {'if', 'for', 'while', 'switch', 'catch', 'return', 'sizeof', 'do', 'else'}
KEYWORDS = CONTROL | {'extern', 'static', 'const', 'volatile', 'unsigned', 'signed', 'struct',
                      'class', 'enum', 'union', 'typedef', 'int', 'char', 'short', 'long',
                      'bool', 'float', 'double', 'void', 'inline', '__stdcall', '__cdecl'}
TAG = re.compile(r'^(HP_\w+|\w+_HP)$')
STRING_TAG = re.compile(r'highpriest|data_hp|\w_HP\b', re.I)
IDENT = re.compile(r'[A-Za-z_]\w*')
# The single-player game gives its actors fixed ids (1 the acolyte, 2 the
# blacksmith, 100 the boy, ...); an online client only knows ids the server sent.
FIXED_ID = re.compile(r'\bGetGameActorByA[Ii][Dd]\s*\(\s*(?:0x[0-9a-fA-F]+|[0-9]+)\s*\)')


def strip(text):
    """Returns (code, strings): code with comments removed and string and
    character literals blanked (newlines kept), and the string literals with
    their line numbers."""
    out, strings, i, n, line = [], [], 0, len(text), 1
    while i < n:
        c = text[i]
        if c == '/' and text.startswith('//', i):
            j = text.find('\n', i)
            j = n if j < 0 else j
            out.append(' ' * (j - i)); i = j
        elif c == '/' and text.startswith('/*', i):
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            seg = text[i:j]; out.append(re.sub(r'[^\n]', ' ', seg)); line += seg.count('\n'); i = j
        elif c in '"\'':
            j = i + 1
            while j < n and text[j] != c and text[j] != '\n':
                j += 2 if text[j] == '\\' else 1
            j = min(j + 1, n)
            if c == '"':
                strings.append((line, text[i + 1:j - 1]))
            out.append(c + ' ' * (j - i - 2) + c if j - i >= 2 else c); i = j
        else:
            if c == '\n':
                line += 1
            out.append(c); i += 1
    return ''.join(out), strings


def label_of(header):
    """Names the scope a '{' opens, from the text before it."""
    h = ' '.join(header.split())
    h = re.sub(r'^\s*#.*$', '', h)
    if re.search(r'\bnamespace\s*\w*\s*$', h) or re.search(r'extern\s+"C"\s*$', h):
        return ('ns', None)
    m = re.search(r'\b(class|struct|union)\s+(\w+)[^()]*$', h)
    if m:
        return ('type', m.group(2))
    if re.search(r'\benum\b', h) or h.endswith('=') or re.search(r'=\s*$', h):
        return ('block', None)
    # A function: the qualified name before the first parenthesis.
    m = re.search(r'((?:~?\w+\s*::\s*)*~?\w+)\s*\(', h)
    if m and m.group(1).split('::')[-1].strip() not in CONTROL:
        # A constructor's initializer list may follow; take the first one.
        return ('func', re.sub(r'\s+', '', m.group(1)))
    return ('block', None)


class Unit:
    def __init__(self, path, rel):
        self.rel = rel
        with open(path, 'rb') as f:
            self.raw = f.read().decode('latin-1')
        self.code, self.strings = strip(self.raw)
        self.lines = self.code.split('\n')
        self.raw_lines = self.raw.split('\n')
        self.scope_at = []          # per line: enclosing function name or ''
        self.toplevel = []          # (line, statement text) of file-scope statements
        self.functions = []         # (line, qualified name) of function definitions
        self.types = []             # (line, name) of class/struct definitions
        self._scan()

    def _scan(self):
        stack, pending, pending_line, headers = [], '', 1, []
        for no, text in enumerate(self.lines, 1):
            # Preprocessor lines take no part in the scope structure.
            if text.lstrip().startswith('#'):
                self.scope_at.append(self._enclosing(stack, pending))
                continue
            for ch in text:
                if ch == '{':
                    if pending.strip() and not stack or all(k in ('type', 'ns') for k, _ in stack):
                        kind, name = label_of(pending)
                    else:
                        kind, name = label_of(pending) if pending.strip() else ('block', None)
                        if kind == 'type':
                            kind = 'block'
                    if kind == 'func' and stack and any(k == 'func' for k, _ in stack):
                        kind, name = 'block', None
                    if kind == 'func':
                        cls = [n for k, n in stack if k == 'type']
                        q = '::'.join(cls + [name]) if cls and '::' not in name else name
                        self.functions.append((pending_line, q))
                        headers.append((pending_line, no, q))
                        name = q
                    elif kind == 'type':
                        self.types.append((pending_line, name))
                    stack.append((kind, name)); pending = ''
                elif ch == '}':
                    if stack:
                        stack.pop()
                    pending = ''
                elif ch == ';':
                    if all(k == 'ns' for k, _ in stack):
                        self.toplevel.append((pending_line, pending.strip()))
                    pending = ''
                else:
                    if not pending.strip() and not ch.isspace():
                        pending_line = no
                    pending += ch
            pending += ' '
            self.scope_at.append(self._enclosing(stack, pending))
        # A definition's header lines belong to the function they open.
        for first, brace, q in headers:
            for no in range(first, brace):
                if self.scope_at[no - 1] == '(file scope)' or self.scope_at[no - 1].endswith('(declarations)'):
                    self.scope_at[no - 1] = q

    @staticmethod
    def _enclosing(stack, pending):
        names = [n for k, n in stack if k == 'func']
        if names:
            return names[-1]
        types = [n for k, n in stack if k == 'type']
        return '::'.join(types) + ' (declarations)' if types else '(file scope)'


class Blob(Unit):
    def __init__(self, rel, data):
        self.rel = rel
        self.raw = data.decode('latin-1')
        self.code, self.strings = strip(self.raw)
        self.lines = self.code.split('\n')
        self.raw_lines = self.raw.split('\n')
        self.scope_at, self.toplevel, self.functions, self.types = [], [], [], []
        self._scan()


def load(tree):
    units = OrderedDict()
    for base, dirs, files in os.walk(tree):
        dirs.sort()
        for name in sorted(files):
            if name.endswith(SOURCE_EXT):
                path = os.path.join(base, name)
                units[os.path.relpath(path, tree)] = Unit(path, os.path.relpath(path, tree))
    return units


def load_rev(repo, rev, prefix='src/'):
    """The sources of src/ as they are at a git revision."""
    ls = subprocess.run(['git', '-C', repo, 'ls-tree', '-r', '--name-only', rev, prefix],
                        check=True, stdout=subprocess.PIPE).stdout.decode().split('\n')
    paths = [p for p in ls if p.endswith(SOURCE_EXT)]
    batch = subprocess.run(['git', '-C', repo, 'cat-file', '--batch'], check=True,
                           input=''.join('%s:%s\n' % (rev, p) for p in paths).encode(),
                           stdout=subprocess.PIPE).stdout
    units, pos = OrderedDict(), 0
    for p in paths:
        nl = batch.index(b'\n', pos)
        size = int(batch[pos:nl].split()[2])
        units[p[len(prefix):]] = Blob(p[len(prefix):], batch[nl + 1:nl + 1 + size])
        pos = nl + 1 + size + 1
    return units


def declared_names(stmt):
    """Variable names a file-scope declaration statement declares."""
    s = ' '.join(stmt.split())
    if not s or s.startswith(('typedef', 'using', 'template')) or re.match(r'^(class|struct|enum|union)\s+\w+$', s):
        return []
    if '(' in s.split('=')[0]:
        return []               # a function declaration
    s = re.sub(r'\[[^\]]*\]', '', s)
    names = []
    for piece in re.split(r',(?![^{]*})', s):
        piece = piece.split('=')[0].strip()
        ids = [t for t in IDENT.findall(piece) if t not in KEYWORDS]
        if ids:
            names.append(ids[-1])
    # The first piece also holds the type: 'short a, b' -> a and b.
    return names


def derive(units):
    hp = {r: u for r, u in units.items() if os.path.basename(r) in HP_FILES and os.path.dirname(r) == ''}
    others = {r: u for r, u in units.items() if r not in hp}
    words = {r: set(IDENT.findall(u.code)) for r, u in units.items()}

    defined_elsewhere = set()
    for u in others.values():
        for _, q in u.functions:
            defined_elsewhere.add(q.split('::')[-1])
        for _, t in u.types:
            defined_elsewhere.add(t)

    functions = OrderedDict()
    for r in HP_FILES:
        u = hp.get(r)
        if not u:
            continue
        for line, q in u.functions:
            name = q.split('::')[-1]
            functions.setdefault(name, (r, line, q))
        if r.endswith('.h'):
            for line, t in u.types:
                functions.setdefault(t, (r, line, 'class ' + t))
    functions = OrderedDict((n, v) for n, v in functions.items() if n not in defined_elsewhere)

    candidates = OrderedDict()
    for r in HP_FILES:
        u = hp.get(r)
        if not u or r.endswith('.h'):
            continue
        for line, stmt in u.toplevel:
            for n in declared_names(stmt):
                candidates.setdefault(n, (r, line))
    globals_, dropped = OrderedDict(), OrderedDict()
    for n, v in candidates.items():
        users = [r for r in others if n in words[r]]
        (globals_ if len(users) <= GLOBAL_MAX_FILES else dropped)[n] = (v, users)

    # Names the online code defines: functions, classes and file-scope
    # variables (enumerators such as AM_UPDATEGAGE_HP are not among them).
    defined = set(defined_elsewhere)
    for u in others.values():
        for _, stmt in u.toplevel:
            defined.update(declared_names(stmt))
        for no, text in enumerate(u.lines):
            if u.scope_at[no].endswith('(declarations)'):
                defined.update(re.findall(r'(\w+)\s*\(', text))
    used_by_hp = set().union(*[words[r] for r in hp]) if hp else set()
    tagged = OrderedDict()
    for n in sorted(used_by_hp):
        if TAG.match(n) and n in defined and n not in functions and n not in globals_:
            homes = [r for r in others if n in words[r]]
            if homes:
                tagged[n] = homes
    for r, u in others.items():
        own = [q.split('::')[-1] for _, q in u.functions] + [t for _, t in u.types if t.startswith('HP_')]
        own += [n for _, stmt in u.toplevel for n in declared_names(stmt) if n.startswith('HP_')]
        for n in own:
            if TAG.match(n) and n not in tagged and n not in functions:
                tagged[n] = [r]
    closure = close(hp, others, set(functions) | set(globals_) | set(tagged) | set(READ))
    return hp, others, functions, globals_, dropped, tagged, closure


def close(hp, others, names):
    """Names of the online code that only the single-player code uses.

    A function defined outside the five files, a class member or a file-scope
    variable is a single-player name when the five files or the body of a
    single-player function use it, and every other line that names it is a
    declaration, its own definition, or inside its own body or the body of a
    single-player function. Repeated until nothing is added."""
    names = set(names)
    member_decl = re.compile(r'(~?\w+)\s*(?:\(|(?:\[[^\]]*\])*\s*(?:[;,=]|$))')
    defs = defaultdict(set)            # name -> {(file, scope)} of its bodies
    declared, types, inline = set(), set(), set()
    sp_types = set(n + ' (declarations)' for n in names)
    for r, u in others.items():
        for _, q in u.functions:
            defs[q.split('::')[-1]].add((r, q))
        types.update(t for _, t in u.types)
        for _, stmt in u.toplevel:
            declared.update(declared_names(stmt))
        for no, text in enumerate(u.lines):
            scope = u.scope_at[no]
            if scope.endswith('(declarations)') and scope not in sp_types and not text.lstrip().startswith('#'):
                t = text
                while True:
                    t2 = re.sub(r'\([^()]*\)', '()', t)
                    if t2 == t:
                        break
                    t = t2
                t = re.sub(r'\{[^{}]*\}', '', t)
                declared.update(n for n in member_decl.findall(t) if n not in KEYWORDS)
    for n, homes in defs.items():
        if all(r.endswith('.h') for r, _ in homes):
            inline.add(n)       # accessors written in a class body stay
    candidates = ((declared | set(defs)) - names) - types - inline
    occ = defaultdict(list)            # name -> [(file, scope, is_declaration)]
    pat = IDENT
    for r, u in others.items():
        for no, text in enumerate(u.lines):
            if text.lstrip().startswith('#'):
                continue
            scope = u.scope_at[no]
            decl = scope.endswith('(declarations)') or scope == '(file scope)'
            for n in set(pat.findall(text)) & candidates:
                occ[n].append((r, scope, decl))
    hp_words = set().union(*[set(IDENT.findall(u.code)) for u in hp.values()]) if hp else set()
    added = OrderedDict()
    while True:
        bodies = set()
        for n in names:
            bodies.update(q for _, q in defs.get(n, ()))
        grew = False
        for n in sorted(candidates - names):
            own = set(q for _, q in defs.get(n, ()))
            used_sp = n in hp_words
            ok = True
            for r, scope, decl in occ.get(n, ()):
                if scope in bodies:
                    used_sp = True
                elif not decl and scope not in own:
                    ok = False
                    break
            if ok and used_sp:
                names.add(n)
                added[n] = sorted(set(r for r, _, _ in occ.get(n, ())))
                grew = True
        if not grew:
            return added


# Functions of the online classes whose whole body is single-player code,
# found by reading (recovery inventory, docs/port/recovery-inventory.md). They
# only seed the closure; everything they alone use is then found by the rule.
READ = OrderedDict([
    ('ProcessDamageSituation', 'CGameMode: plays the damage queue g_DM_List the single-player battle fills'),
    ('GainExp', 'CGameMode: client-side experience and level-up of the single-player character'),
])


def find_sites(units, others, names):
    sites = []
    pat = re.compile(r'\b(' + '|'.join(sorted(map(re.escape, names), key=len, reverse=True)) + r')\b') if names else None
    for r, u in others.items():
        hits = defaultdict(list)
        if pat:
            for no, text in enumerate(u.lines, 1):
                found = sorted(set(pat.findall(text)))
                if found:
                    hits[no] += found
        for no, text in enumerate(u.lines, 1):
            if FIXED_ID.search(text):
                hits[no].append('fixed actor id')
        for no, s in u.strings:
            if STRING_TAG.search(s):
                hits[no].append('"%s"' % s[:40])
        for no in sorted(hits):
            sites.append((r, no, u.scope_at[no - 1], hits[no], u.raw_lines[no - 1].strip()))
    return sites


def build_lists(root):
    out = []
    mk = os.path.join(root, 'sources.mk')
    if os.path.exists(mk):
        for no, text in enumerate(open(mk, encoding='latin-1'), 1):
            if re.search(r'\bHP_(Battle|BuySell|RestTravell|SaveLoad|Tetris)\b', text):
                out.append(('sources.mk', no, '(build list)', ['HP_*'], text.strip()))
    return out


def main():
    ap = argparse.ArgumentParser()
    root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
    ap.add_argument('--tree', default=os.path.join(root, 'src'))
    ap.add_argument('--names', action='store_true')
    ap.add_argument('--markdown', action='store_true')
    ap.add_argument('--verdicts')
    ap.add_argument('--names-from', default='main', metavar='REV',
                    help="git revision whose src/ the names are derived from (default: main, "
                         "which keeps the single-player game); '-' derives them from --tree")
    a = ap.parse_args()

    units = load(a.tree)
    ref = units if a.names_from == '-' else load_rev(root, a.names_from)
    hp, _, functions, globals_, dropped, tagged, closure = derive(ref)
    others = OrderedDict((r, u) for r, u in units.items()
                         if not (os.path.dirname(r) == '' and os.path.basename(r) in HP_FILES))
    hp = OrderedDict((r, u) for r, u in units.items() if r not in others)
    if a.names:
        print('# functions (%d)' % len(functions))
        for n, (r, line, q) in functions.items():
            print('%-28s %s:%d %s' % (n, r, line, q))
        print('# globals (%d), named in at most %d other files' % (len(globals_), GLOBAL_MAX_FILES))
        for n, ((r, line), users) in globals_.items():
            print('%-28s %s:%d  also in: %s' % (n, r, line, ' '.join(users) or '-'))
        print('# left out: file-scope names of the five files that %d+ other files use (%d)'
              % (GLOBAL_MAX_FILES + 1, len(dropped)))
        print(' '.join(dropped))
        print('# tagged (%d)' % len(tagged))
        for n, homes in tagged.items():
            print('%-28s %s' % (n, ' '.join(homes)))
        print('# classed single-player by reading (%d)' % len(READ))
        for n, why in READ.items():
            print('%-28s %s' % (n, why))
        print('# used only by the single-player code (%d)' % len(closure))
        for n, homes in closure.items():
            print('%-28s %s' % (n, ' '.join(homes)))
        return 0

    names = list(functions) + list(globals_) + list(tagged) + list(READ) + list(closure)
    sites = find_sites(units, others, names) + build_lists(os.path.dirname(a.tree))
    present = [r for r in HP_FILES if r in hp]
    sites = [(r, 0, '(file)', ['file'], 'single-player source file') for r in present] + sites

    if a.markdown:
        verdicts = {}
        if a.verdicts:
            for text in open(a.verdicts, encoding='utf-8'):
                if text.strip() and not text.startswith('#'):
                    k, v, note = (text.rstrip('\n').split('\t') + ['', ''])[:3]
                    verdicts[k] = (v, note)
        groups = OrderedDict()
        for s in sites:
            groups.setdefault((s[0], s[2]), []).append(s)
        print('| file | enclosing function | sites | verdict | lines and names |')
        print('|---|---|---:|---|---|')
        for (r, fn), items in groups.items():
            v, note = verdicts.get('%s %s' % (r, fn), ('?', ''))
            names = OrderedDict()
            for _, no, _, found, _ in items:
                for f in found:
                    names.setdefault(f.replace('|', '\\|'), []).append(str(no))
            where = '; '.join('%s: %s' % (n, ','.join(ls[:12]) + (',...' if len(ls) > 12 else ''))
                              for n, ls in names.items())
            print('| `%s` | `%s` | %d | %s | %s |' % (r, fn, len(items), v + (' ' + note if note else ''), where))
        return 0 if not sites else 1

    for r, no, fn, found, text in sites:
        print('%s:%d: [%s] %s: %s' % (r, no, fn, ','.join(found), text[:120]))
    files = len(set(s[0] for s in sites))
    print('%d sites in %d files, %d enclosing functions' % (len(sites), files, len(set((s[0], s[2]) for s in sites))),
          file=sys.stderr)
    return 0 if not sites else 1


if __name__ == '__main__':
    sys.exit(main())
