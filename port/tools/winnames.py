#!/usr/bin/env python3
"""Reports the Win32 and DirectX names the built client sources still use,
grouped by the step of M11 that removes them.

  winnames.py [--build DIR] [--step ID ...] [--names] [--files N] [--json FILE]
              [-j N]

  --build   the CMake build directory (default: build/cmake/debug), whose
            compile_commands.json lists the units (see nowin.py)
  --step    report only these groups (M11.1 ... M11.10, keep, crt, shared,
            other); default: all
  --names   list every name of a group with its count and files
  --files   files listed per group (default: all; 0 hides them)
  --json    also write every (name, group, kind, header, file, count) row

What counts as a name, mechanically:

  1. A synthetic unit includes common.h and every Windows, DirectX, adapter
     and library header (nowin.py's kinds) that any built unit of src/ reads,
     compiled with the flags of a src/ unit. Its declarations (clang's AST:
     functions, variables, typedefs, structs, enums and their constants, at
     file scope or inside extern "C") and its macros (`clang -E -dD`) whose
     location is in one of those headers are the names.
  2. Every built src/ unit is preprocessed (`clang -E -dD`); a line of a
     src/ file counts as compiled when the conditional block it sits in
     (the lines between two #if/#ifdef/#ifndef/#elif/#else/#endif) produced
     output in at least one unit. Identifiers on compiled lines (comments
     and string literals removed) are matched against the names; a match
     after `.`, `->` or `Class::` is a member of the client's own and does
     not count. Counts are occurrences; files are the src/ files that hold
     them (headers included), not only the units.
  3. A function name's group is the adapter that defines it: the
     port/compat or port/client/src source file with a definition of the name at column 0
     (ADAPTER_STEP below). A function name that no adapter defines and that
     the client defines as a member of a class of its own (`Class::Name(`)
     is the client's member (a call of the Windows function would not
     link): dropped. Names the adapters do not define (types,
     constants, macros, functions defined inline by a header) take the
     group of their header (HEADER_STEP).

By hand: ADAPTER_STEP and HEADER_STEP (which file belongs to which step),
the name patterns of NAME_RULES, which decide before both (for the
kernel32 files that serve several steps, bf_win32_base.cpp and
bf_win32_misc.cpp, and for the constants and types the port keeps: the
`keep` group), and the client headers of nowin.py's SDK_HEADERS. A name
port/client/include/bf_types.h declares is always `keep`; names the keep
rules match but the header does not declare are listed on their own line,
and the client headers of nowin.py's SDK_HEADERS.

Groups: M11.1 time and threads, M11.2 network, M11.3 files, M11.4 settings,
M11.5 shell, M11.6 sound, M11.7 input and window, M11.8 text, M11.9
graphics, M11.10 Granny and Bink; `keep`: types, structs, macros and
constants that stay as the port's own (bf_types.h); `crt`: the C runtime
names of the prefix and bf_crt.cpp, which live in port/client (M11.0d);
`shared`: names several steps use (handles, last error), removed by the
last of them; `other`: none of the rules matched (listed by name).
"""
import argparse
import collections
import concurrent.futures
import json
import os
import re
import shlex
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import nowin  # noqa: E402

ROOT = nowin.ROOT
GROUPS = [
    ('M11.1', 'time and threads'), ('M11.2', 'network'), ('M11.3', 'files'),
    ('M11.4', 'settings'), ('M11.5', 'shell'), ('M11.6', 'sound'),
    ('M11.7', 'input and window'), ('M11.8', 'text'), ('M11.9', 'graphics'),
    ('M11.10', 'Granny and Bink'), ('keep', 'kept as the port\'s own (bf_types.h)'),
    ('crt', 'CRT helpers and port code (port/client)'),
    ('shared', 'used by several steps'), ('other', 'not classified'),
]

# Adapter source or port source (relative to port/) -> group. None: decided by name.
ADAPTER_STEP = [
    ('compat/win32/src/bf_win32_thread.cpp', 'M11.1'),
    ('compat/win32/src/bf_win32_file.cpp', 'M11.3'),
    ('compat/win32/src/bf_win32_registry.cpp', 'M11.4'),
    ('compat/win32/src/bf_win32_shell.cpp', 'M11.5'),
    ('compat/win32/src/bf_win32_base.cpp', None),
    ('compat/win32/src/bf_win32_misc.cpp', 'M11.5'),
    ('compat/miles/', 'M11.6'),
    ('compat/input/src/', 'M11.7'),
    ('client/src/bf_main.cpp', 'M11.7'),
    ('compat/gdi/src/', 'M11.8'),
    ('compat/d3d/src/', 'M11.9'),
    ('compat/granny/bf_compat_granny.cpp', 'M11.10'),
    ('compat/bink/bf_compat_bink.cpp', 'M11.10'),
    ('client/src/bf_crt.cpp', 'crt'),
    ('client/src/bf_zlib.cpp', 'other'),
]

# The port's own headers that declare CRT and Windows-shaped names (they moved
# from port/compat/include to port/client/include in M11.0d, so nowin.py no
# longer calls them compat): still read for names, group `crt` (or M11.3 for
# the file calls bf_crt.h declares and port/compat/win32 defines).
PORT_HEADERS = tuple(os.path.join(nowin.ROOT, 'port', 'client', 'include', h)
                     for h in ('bf_prefix.h', 'bf_crt.h', 'bf_port.h', 'bf_replay.h'))

# Header basename (lower case) -> group, for names no adapter defines.
HEADER_STEP = {
    'mmsystem.h': 'M11.6', 'dsound.h': 'M11.6', 'mss.h': 'M11.6',
    'winsock.h': 'M11.2', 'winsock2.h': 'M11.2',
    'fileapi.h': 'M11.3', 'memoryapi.h': 'M11.3', 'io.h': 'M11.3', 'direct.h': 'M11.3',
    'winreg.h': 'M11.4',
    'shellapi.h': 'M11.5', 'shlwapi.h': 'M11.5', 'shtypes.h': 'M11.5', 'shlobj.h': 'M11.5',
    'tlhelp32.h': 'M11.5', 'wincrypt.h': 'M11.5', 'nb30.h': 'M11.5', 'libloaderapi.h': 'M11.5',
    'winuser.h': 'M11.7', 'winuser.rh': 'M11.7', 'dinput.h': 'M11.7', 'imm.h': 'M11.7',
    'wingdi.h': 'M11.8', 'winnls.h': 'M11.8', 'mbstring.h': 'M11.8', 'stringapiset.h': 'M11.8',
    'ddraw.h': 'M11.9', 'd3d.h': 'M11.9', 'd3dtypes.h': 'M11.9', 'd3dcaps.h': 'M11.9',
    'ijl.h': 'M11.9', 'objbase.h': 'M11.9', 'unknwn.h': 'M11.9', 'combaseapi.h': 'M11.9',
    'cguid.h': 'M11.9', 'objidl.h': 'M11.9', 'objidlbase.h': 'M11.9', 'guiddef.h': 'M11.9',
    'granny.h': 'M11.10', 'bink.h': 'M11.10', 'rad.h': 'M11.10', 'radbase.h': 'M11.10',
    'bf_bink_sdk.h': 'M11.10',
    'synchapi.h': 'M11.1', 'processthreadsapi.h': 'M11.1', 'process.h': 'M11.1',
    'sysinfoapi.h': 'M11.1', 'profileapi.h': 'M11.1', 'timezoneapi.h': 'M11.1',
    'windef.h': 'keep', 'minwindef.h': 'keep', 'winnt.h': 'keep', 'basetsd.h': 'keep',
    'wtypes.h': 'keep', 'wtypesbase.h': 'keep', 'winerror.h': 'keep',
    'minmax.h': 'keep', 'minwinbase.h': 'keep', 'windows.h': 'keep', 'sal.h': 'keep',
    'bf_prefix.h': 'crt', 'bf_crt.h': 'crt', 'conio.h': 'crt',
    'bf_port.h': 'other', 'bf_replay.h': 'other',
}

# (regex on the name, group): checked first, in order.
NAME_RULES = [(re.compile(p), g) for p, g in [
    # kept as the port's own constants (the plan's decision, progress log
    # "M11 planned"): virtual keys, the client's window messages, the D3D 7
    # render, blend, texture-stage, primitive and vertex-format numbering,
    # the vertex and matrix layouts, the colour macros.
    (r'^D3DVERTEXBUFFER\w*$|^D3DVBCAPS_\w+$|^D3D_OK$|^D3DPTFILTERCAPS_\w+$|^IsEqual(IID|GUID)$|'
     r'^_?GUID$|^LPGUID$|^REF(IID|GUID|CLSID)$|^IID$|^CLSID$', 'M11.9'),
    (r'^_?L?P?(U?LARGE_INTEGER)$', 'M11.1'),
    (r'^_?RTL_CRITICAL_SECTION$', 'M11.1'),
    (r'^_?EXCEPTION_POINTERS$|^_?EXCEPTION_RECORD$', 'M11.5'),
    (r'^(tag)?TEXTMETRICA?$|^L?P?TEXTMETRICA?$', 'M11.8'),
    (r'^WSA\w+$', 'M11.2'),
    (r'^(P?TOKEN_\w+|SE_\w+|PROCESS_\w+|P?LUID\w*|OSVERSIONINFO\w*|VER_\w+|FARPROC|'
     r'HMODULE)$', 'M11.5'),
    (r'^HLOCAL$|^HGLOBAL$', 'M11.3'),
    (r'^(VK|WM|MK)_', 'keep'),
    (r'^D3D(BLEND|FVF|PT|TSS|TOP|TA|RENDERSTATE|RS|CMP|CULL|SHADE|FILL|TFG|TFN|TFP|TEXF|'
     r'TADDRESS|ZB|FOG|ANTIALIAS|MCS|STENCILOP|TEXTUREOP|TEXTUREFILTER|TEXTUREMAGFILTER|'
     r'TEXTUREMINFILTER|TEXTUREMIPFILTER|TEXTUREADDRESS|TEXTURESTAGESTATETYPE|'
     r'RENDERSTATETYPE|PRIMITIVETYPE|ZBUFFERTYPE|BLENDOP|CMPFUNC|SHADEMODE|FILLMODE|'
     r'CULL|FOGMODE|WRAP|DP_|COLORVALUE|COLOR|VALUE|VECTOR|MATRIX|TLVERTEX|LVERTEX|VERTEX|'
     r'RGBA?|DIVIDE|VAL|_)', 'keep'),
    (r'^_?D3D(BLEND|PRIMITIVETYPE|TEXTURESTAGESTATETYPE|TEXTUREOP|RENDERSTATETYPE|'
     r'TEXTUREFILTER|TEXTUREADDRESS|CMPFUNC|CULL|SHADEMODE|FILLMODE|FOGMODE|ZBUFFERTYPE|'
     r'STENCILOP|ANTIALIASMODE|TEXTUREMAGFILTER|TEXTUREMINFILTER|TEXTUREMIPFILTER|'
     r'COLORVALUE|VECTOR|MATRIX|TLVERTEX|LVERTEX|VERTEX)$', 'keep'),
    (r'^RGBA?_(MAKE|GET\w+|SETALPHA|TORGB)$|^RGB_MAKE$|^(RGB|PALETTERGB|GetRValue|GetGValue|'
     r'GetBValue|LOBYTE|HIBYTE|LOWORD|HIWORD|MAKEWORD|MAKELONG|MAKELPARAM|MAKEWPARAM|'
     r'MAKELRESULT|ZeroMemory|FillMemory|CopyMemory|MoveMemory|MAX_PATH|min|max|__max|__min|'
     r'WINAPI|APIENTRY|CALLBACK|FAR|NEAR|PASCAL|CONST|TRUE|FALSE|IN|OUT|OPTIONAL|VOID|'
     r'byte|SUCCEEDED|FAILED|S_OK|S_FALSE|E_FAIL|E_\w+)$', 'keep'),
    (r'^(tag|_)?(RECT|POINT|SIZE|PALETTEENTRY|RGBQUAD|BITMAPFILEHEADER|BITMAPINFOHEADER|'
     r'BITMAPINFO|FILETIME|SYSTEMTIME|LOGFONTA?)$', 'keep'),
    (r'^(LP|P|NP)?(RECT|POINT|SIZE|PALETTEENTRY|RGBQUAD|BITMAPFILEHEADER|BITMAPINFOHEADER|'
     r'BITMAPINFO|FILETIME|SYSTEMTIME|LOGFONTA?)$', 'keep'),
    (r'^BI_(RGB|RLE4|RLE8|BITFIELDS)$|^(FW|ANSI|DEFAULT|HANGEUL|HANGUL|SHIFTJIS|GB2312|'
     r'CHINESEBIG5|SYMBOL|OEM|THAI|VIETNAMESE|BALTIC|RUSSIAN|EASTEUROPE|GREEK|TURKISH|'
     r'ARABIC|HEBREW|JOHAB|MAC)_(CHARSET|THIN|EXTRALIGHT|LIGHT|NORMAL|REGULAR|MEDIUM|'
     r'SEMIBOLD|DEMIBOLD|BOLD|EXTRABOLD|HEAVY|BLACK|DONTCARE)$', 'keep'),
    # time and threads
    (r'^(Sleep|SleepEx|timeGetTime|timeBeginPeriod|timeEndPeriod|timeGetDevCaps|'
     r'GetTickCount(64)?|QueryPerformance\w+|GetLocalTime|GetSystemTime\w*|'
     r'FileTimeTo\w+|SystemTimeTo\w+|LocalFileTimeToFileTime|GetTimeZoneInformation|'
     r'_beginthread(ex)?|_endthread(ex)?|Tls\w+|Interlocked\w+|\w*CriticalSection\w*|'
     r'WaitFor\w+|CreateEventA?|SetEvent|ResetEvent|CreateMutexA?|ReleaseMutex|'
     r'CreateSemaphoreA?|ReleaseSemaphore|CreateThread|ExitThread|TerminateThread|'
     r'ResumeThread|SuspendThread|GetCurrentThread(Id)?|SetThreadPriority|GetThreadPriority|'
     r'GetExitCodeThread|SwitchToThread|LPTHREAD_START_ROUTINE|CRITICAL_SECTION|'
     r'LPCRITICAL_SECTION|INFINITE|WAIT_\w+|THREAD_PRIORITY_\w+|TIMECAPS|TIME_\w+|'
     r'TIMERR_\w+|LPTIMECALLBACK|timeSetEvent|timeKillEvent)$', 'M11.1'),
    # the port's own code (port/client; the bf_ names of the C runtime helpers and the glue)
    (r'^bf_', 'crt'),
    # files and memory
    (r'^(\w*File(A|W|Ex|ExA|ExW|Size|SizeEx|Pointer|PointerEx|Time|Attributes\w*|Mapping\w*)?|'
     r'CreateFileA?|FindFirstFileA?|FindNextFileA?|FindClose|MapViewOfFile\w*|'
     r'UnmapViewOfFile|VirtualAlloc|VirtualFree|VirtualQuery|VirtualProtect|Global\w+|'
     r'Local(Alloc|Free|Lock|Unlock|Size|ReAlloc)|Heap\w+|GetProcessHeap|'
     r'(Create|Remove|Set|Get)(Current)?DirectoryA?|GetFullPathNameA?|GetTempPathA?|'
     r'GetDiskFreeSpace\w*|GetDriveTypeA?|GetLogicalDrives|GetModuleFileNameA?|'
     r'_?(open|close|read|write|lseek|tell|access|mkdir|rmdir|chdir|getcwd|findfirst|'
     r'findnext|findclose|filelength|stat|unlink|chmod|eof|commit|locking|sopen|creat|'
     r'fileno|setmode|splitpath|makepath|fullpath)|_finddata_t|WIN32_FIND_DATAA?|'
     r'LPWIN32_FIND_DATAA?|INVALID_HANDLE_VALUE|INVALID_FILE_SIZE|GENERIC_(READ|WRITE)|'
     r'FILE_\w+|OPEN_EXISTING|OPEN_ALWAYS|CREATE_ALWAYS|CREATE_NEW|TRUNCATE_EXISTING|'
     r'PAGE_\w+|MEM_\w+|FILE_MAP_\w+|SECTION_\w+|_O_\w+|O_BINARY|_A_\w+|MEMORYSTATUS|'
     r'GMEM_\w+|LMEM_\w+|HEAP_\w+)$', 'M11.3'),
    # settings
    (r'^(Reg(Open|Close|Create|Query|Set|Delete|Enum|Flush)\w*|HKEY_\w+|KEY_\w+|REG_\w+|GetPrivateProfile\w+|WritePrivateProfile\w+|'
     r'GetProfile\w+|WriteProfile\w+|HKEY|PHKEY)$', 'M11.4'),
    # shell, processes, libraries, system information
    (r'^(MessageBox\w*|MB_\w+|ID(OK|CANCEL|ABORT|RETRY|IGNORE|YES|NO|CLOSE|HELP)|'
     r'ShellExecute\w*|SW_\w+|CreateProcessA?|WinExec|OpenProcess|TerminateProcess|'
     r'ExitProcess|GetCurrentProcess(Id)?|GetExitCodeProcess|SetPriorityClass|'
     r'\w+_PRIORITY_CLASS|GetCommandLineA?|GetVersionExA?|OSVERSIONINFOA?|GetSystemInfo|'
     r'SYSTEM_INFO|GlobalMemoryStatus\w*|LoadLibraryA?|GetProcAddress|FreeLibrary|'
     r'GetModuleHandleA?|SetUnhandledExceptionFilter|Crypt\w+|Netbios|CoInitialize\w*|'
     r'CoUninitialize|OutputDebugStringA?|FormatMessageA?|GetSystemDirectoryA?|'
     r'GetWindowsDirectoryA?|GetUserDefaultLangID|GetUserDefaultLCID|GetComputerNameA?|'
     r'GetUserNameA?|STARTUPINFOA?|PROCESS_INFORMATION|CreateToolhelp32Snapshot|'
     r'Process32\w+|Module32\w+|Thread32\w+|PROCESSENTRY32\w*|MODULEENTRY32\w*|TH32CS_\w+|'
     r'SH\w+|Path\w+|StrStr\w*|StrCmp\w*|_?spawn\w+|P_WAIT|P_NOWAIT|STARTF_\w+|'
     r'FORMAT_MESSAGE_\w+|LPTOP_LEVEL_EXCEPTION_FILTER|EXCEPTION_\w+)$', 'M11.5'),
    # text and code pages
    (r'^(MultiByteToWideChar|WideCharToMultiByte|GetACP|GetOEMCP|IsDBCSLeadByte\w*|'
     r'GetCPInfo\w*|lstr\w+|CharNext\w*|CharPrev\w*|CharUpper\w*|CharLower\w*|'
     r'IsCharAlpha\w*|MulDiv|CP_\w+|MB_(PRECOMPOSED|COMPOSITE|USEGLYPHCHARS|'
     r'ERR_INVALID_CHARS)|WC_\w+|_mbs\w+|_ismbb\w+|_ismbc\w+|w?sprintfA?|wvsprintfA?|'
     r'(SUB)?LANG_\w+|MAKELANGID|PRIMARYLANGID|SUBLANGID|LANGID)$', 'M11.8'),
    # shared handle and error functions
    (r'^(CloseHandle|DuplicateHandle|GetLastError|SetLastError|HANDLE|LPHANDLE|'
     r'ERROR_\w+|NO_ERROR)$', 'shared'),
]]


TYPES_H = os.path.join(ROOT, 'port', 'client', 'include', 'bf_types.h')


def types_h_names():
    """The names port/client/include/bf_types.h declares (empty if absent)."""
    if not os.path.exists(TYPES_H):
        return set()
    text = re.sub(r'/\*.*?\*/', '', open(TYPES_H).read(), flags=re.S)
    names = set(re.findall(r'^\s*#\s*define\s+(\w+)', text, re.M))
    while True:  # inline member bodies out of the structures
        t = re.sub(r'(\{[^{}]*)\{[^{}]*\}', r'\1', text)
        if t == text:
            break
        text = t
    names |= set(re.findall(r'\b(?:struct|enum)\s+(\w+)\s*\{', text))
    for m in re.finditer(r'\btypedef\b([^;{]*(?:\{[^}]*\})?[^;]*);', text, re.S):
        tail = re.sub(r'\{[^}]*\}', '', m.group(1))
        for part in tail.split(','):
            w = re.findall(r'(\w+)\s*(?:\[[^]]*\])?\s*$', part.strip())
            if w:
                names.add(w[0])
    for m in re.finditer(r'\benum\b[^{;]*\{([^}]*)\}', text):
        names |= {x.split('=')[0].strip() for x in m.group(1).split(',') if x.strip()}
    return names


TYPES = types_h_names()


def group_of(name, header, adapter, rules_only=False):
    if name in TYPES and not rules_only:
        return 'keep'  # declared by the port's own types header
    base = os.path.basename(header).lower()
    if HEADER_STEP.get(base) == 'M11.10' or base in ('mss.h', 'ijl.h', 'npgamelib.h'):
        return HEADER_STEP.get(base, 'other')  # the libraries' own names
    for rx, g in NAME_RULES:
        if rx.match(name):
            return g
    if rules_only:
        return None
    if adapter:
        for prefix, g in ADAPTER_STEP:
            if adapter.startswith(prefix) and g:
                return g
    if base in HEADER_STEP:
        return HEADER_STEP[base]
    return 'other'


# ---------------------------------------------------------------- the names

DECL_KINDS = {'FunctionDecl', 'VarDecl', 'TypedefDecl', 'TypeAliasDecl', 'CXXRecordDecl',
              'RecordDecl', 'EnumDecl', 'EnumConstantDecl', 'FunctionTemplateDecl',
              'ClassTemplateDecl'}
CONTAINERS = {'TranslationUnitDecl', 'LinkageSpecDecl', 'EnumDecl', 'NamespaceDecl'}
LOC = re.compile(r'(<scratch space>|<built-in>|/[^\s:<>,]+):\d+:\d+')
FLAGS = {'used', 'referenced', 'implicit', 'invalid', 'constexpr', 'inline', 'extern',
         'static', 'definition', 'imported', 'hidden', 'cinit', 'callinit', 'listinit',
         'prev', 'struct', 'class', 'union', 'enum', 'module', 'private', 'nrvo',
         'parenlistinit', 'tls', 'mutable', 'cxx_tls', 'external', 'internal'}
IDENT = re.compile(r'[A-Za-z_]\w*')


def src_unit_args(entry, mode):
    """The unit's compile command for -E / -fsyntax-only, without the unit."""
    args = nowin.unit_args(entry)[:-2]  # drop -M -MG
    return [a for a in args if os.path.realpath(os.path.join(entry['directory'], a))
            != os.path.realpath(os.path.join(entry['directory'], entry['file']))] + mode


class Wanted(dict):
    """Header path -> repository-relative name, any spelling of the path."""
    def __missing__(self, p):
        r = os.path.realpath(p) if not p.startswith('<') else p
        v = dict.get(self, r) if r != p else None
        self[p] = v
        return v

    def get(self, p, d=None):
        v = self[p]
        return d if v is None else v

    def __contains__(self, p):
        return self[p] is not None


def parse_ast(text, wanted, other):
    """{name: (kind, header)} of the declarations located in a wanted header;
    the names declared anywhere else go into the set other."""
    out = {}
    last = ''
    stack = []
    for line in text.splitlines():
        m = re.match(r'^([|` -]*)([A-Za-z]+) 0x[0-9a-f]+', line)
        if not m:
            for f in LOC.findall(line):
                last = f
            continue
        depth = len(m.group(1)) // 2
        kind = m.group(2)
        del stack[depth:]
        parent_ok = all(k in CONTAINERS for k in stack)
        stack.append(kind)
        rest = line[m.end():]
        # the source range <...>, then the decl's own location
        begin_file = None
        if rest.startswith(' prev 0x'):
            rest = re.sub(r'^ prev 0x[0-9a-f]+', '', rest)
        if rest.startswith(' parent 0x'):
            rest = re.sub(r'^ parent 0x[0-9a-f]+', '', rest)
        rng = ''
        if rest.startswith(' <'):
            depth_b, i = 0, 1
            while i < len(rest):
                if rest[i] == '<':
                    depth_b += 1
                elif rest[i] == '>':
                    depth_b -= 1
                    if depth_b == 0:
                        break
                i += 1
            rng, rest = rest[1:i + 1], rest[i + 1:]
        locs = LOC.findall(rng)
        if locs:
            begin_file = locs[0]
            last = locs[-1]
        lm = re.match(r'^ (<scratch space>|<built-in>|/[^\s:<>,]+|line|col):[\d:]+', rest)
        if lm:
            if lm.group(1) not in ('line', 'col'):
                last = lm.group(1)
            rest = rest[lm.end():]
        if kind not in DECL_KINDS or not parent_ok:
            continue
        f = last
        if f.startswith('<'):
            f = begin_file or ''
        if f.startswith('<'):
            continue
        if f not in wanted:
            other_file = True
        else:
            other_file = False
        rm = re.search(r'\b(?:struct|class|union) (?!definition\b)(\w+)', rest)
        if kind in ('CXXRecordDecl', 'RecordDecl'):
            if not rm:
                continue
            name = rm.group(1)
        else:
            head = rest.split("'", 1)[0]
            words = [w for w in IDENT.findall(head) if w not in FLAGS]
            if not words:
                continue
            name = words[0]
        if name == 'operator':
            continue
        if other_file:
            if not f.endswith(SYNTHETIC):
                other.add(name)
        else:
            out.setdefault(name, (kind, wanted[f]))
    return out


def parse_macros(text, wanted, other):
    out = {}
    cur = None
    path = ''

    for line in text.splitlines():
        if line.startswith('# '):
            m = re.match(r'# \d+ "([^"]+)"', line)
            if m:
                path = m.group(1)
                cur = wanted.get(path)
            continue
        if line.startswith('#define '):
            m = IDENT.match(line, 8)
            if not m:
                continue
            if cur:
                out.setdefault(m.group(0), ('macro', cur))
            elif not path.startswith('<') and not path.endswith(SYNTHETIC):
                other.add(m.group(0))
    return out


# ------------------------------------------------------------- the src files

def preprocess(item):
    """Lines of src/ files with output, and the headers a src/ file includes."""
    entry, = item
    args = src_unit_args(entry, ['-E', '-dD', os.path.join(entry['directory'], entry['file'])])
    p = subprocess.run(args, cwd=entry['directory'], capture_output=True)
    if p.returncode != 0:
        return entry['file'], None, None, None, p.stderr.decode('latin-1')
    src = os.path.join(ROOT, 'src') + os.sep
    active = collections.defaultdict(set)
    files, direct = set(), set()
    cur, line, insrc = None, 0, False
    for raw in p.stdout.split(b'\n'):
        if raw.startswith(b'# ') and raw[2:3].isdigit():
            m = re.match(rb'# (\d+) "([^"]+)"( 1)?', raw)
            if m:
                if m.group(3) and insrc:
                    direct.add(m.group(2).decode('latin-1'))
                cur = m.group(2).decode('latin-1')
                files.add(cur)
                line = int(m.group(1))
                insrc = os.path.realpath(cur).startswith(src)
                if insrc:
                    cur = os.path.realpath(cur)
                continue
        if insrc and raw.strip():
            active[cur].add(line)
        line += 1
    return entry['file'], dict(active), files, direct, ''


SYNTHETIC = 'all_windows_headers.cpp'
# A class, struct, union or enum the client defines (not a forward
# declaration), or a typedef of its own.
OWN = re.compile(r'^\s*(?:typedef\s+)?(?:class|struct|union|enum)\s+([A-Za-z_]\w*)\s*[:{]'
                 r'|^\s*typedef\b[^;(]*?\b([A-Za-z_]\w*)\s*(?:\[[^]]*\]\s*)*;')
# Class::member( in a src/ file: a member function the client defines.
MEMBER_DEF = re.compile(r'\b([A-Za-z_]\w*)\s*::\s*([A-Za-z_]\w*)\s*\(')
COND = re.compile(r'^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b')


def lex(path):
    """Per line: [(identifier, member)] with comments and literals removed."""
    text = open(path, 'rb').read().decode('latin-1')
    lines = text.split('\n')
    result = [[] for _ in lines]
    conds = [bool(COND.match(l)) for l in lines]
    i, n, ln = 0, len(text), 0
    prev = ''
    prev2 = ''
    while i < n:
        c = text[i]
        if c == '\n':
            ln += 1
            i += 1
            continue
        if c == '/' and text.startswith('//', i):
            j = text.find('\n', i)
            while j != -1 and text[j - 1] == '\\':
                ln += 1
                j = text.find('\n', j + 1)
            i = n if j == -1 else j
            continue
        if c == '/' and text.startswith('/*', i):
            j = text.find('*/', i + 2)
            j = n if j == -1 else j + 2
            ln += text.count('\n', i, j)
            i = j
            continue
        if c in '"\'':
            j = i + 1
            while j < n and text[j] != c and text[j] != '\n':
                j += 2 if text[j] == '\\' else 1
            ln += text.count('\n', i, min(j, n))
            i = j + 1
            prev, prev2 = 'lit', prev
            continue
        if c.isalpha() or c == '_':
            m = IDENT.match(text, i)
            w = m.group(0)
            member = prev in ('.', '->') or (prev == '::' and prev2 == 'id')
            result[ln].append((w, member))
            prev, prev2 = 'id', prev
            i = m.end()
            continue
        if c.isspace():
            i += 1
            continue
        if text.startswith('->', i) or text.startswith('::', i):
            prev, prev2 = text[i:i + 2], prev
            i += 2
            continue
        prev, prev2 = c, prev
        i += 1
    return lines, result, conds


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--build', default=os.path.join(ROOT, 'build', 'cmake', 'debug'))
    ap.add_argument('--step', action='append', default=[])
    ap.add_argument('--names', action='store_true')
    ap.add_argument('--files', type=int, default=-1)
    ap.add_argument('--json')
    ap.add_argument('-j', type=int, default=os.cpu_count() or 4)
    o = ap.parse_args()

    db = os.path.join(o.build, 'compile_commands.json')
    if not os.path.exists(db):
        sys.exit('no %s (see nowin.py --help)' % db)
    src = os.path.join(ROOT, 'src') + os.sep
    entries, seen = [], set()
    for e in json.load(open(db)):
        f = os.path.realpath(os.path.join(e['directory'], e['file']))
        if f.startswith(src) and f not in seen and os.path.exists(f):
            seen.add(f)
            entries.append(e)
    entries.sort(key=lambda e: e['file'])

    # 2. preprocess the units
    with concurrent.futures.ProcessPoolExecutor(o.j) as pool:
        pre = list(pool.map(preprocess, [(e,) for e in entries]))
    for f, a, _, _, err in pre:
        if a is None:
            sys.stderr.write('%s: preprocessing failed\n%s' % (f, err))
    active = collections.defaultdict(set)
    headers, direct = set(), set()
    names_dirs = nowin.windows_names(), nowin.system_dirs()
    for _, a, files, d, _ in pre:
        if a is None:
            continue
        for k, v in a.items():
            active[k] |= v
        headers |= files
        direct |= d
    wanted = Wanted()
    for h in headers:
        if h.startswith('<'):
            continue
        if nowin.classify(h, *names_dirs) or os.path.realpath(h) in PORT_HEADERS:
            wanted[os.path.realpath(h)] = nowin.rel(h)

    # 1. the names, from a synthetic unit that includes every such header
    cxx = next(e for e in entries if e['file'].endswith('.cpp'))
    out_dir = os.path.join(ROOT, 'build', 'port', 'winnames')
    os.makedirs(out_dir, exist_ok=True)
    syn = os.path.join(out_dir, SYNTHETIC)
    incs = sorted({os.path.realpath(h) for h in direct if h in wanted}, key=lambda p: (not p.endswith('common.h'), p))
    with open(syn, 'w') as f:
        f.write('#include "common.h"\n')
        for h in incs:
            f.write('#include "%s"\n' % h)
    ast = subprocess.run(src_unit_args(cxx, ['-fsyntax-only', '-Xclang', '-ast-dump', syn]),
                         cwd=cxx['directory'], capture_output=True)
    if ast.returncode != 0:
        sys.stderr.write('the synthetic unit %s does not compile:\n%s'
                         % (syn, ast.stderr.decode('latin-1')[-3000:]))
        return 2
    mac = subprocess.run(src_unit_args(cxx, ['-E', '-dD', syn]), cwd=cxx['directory'],
                         capture_output=True)
    sysnames = set()
    universe = parse_macros(mac.stdout.decode('latin-1'), wanted, sysnames)
    universe.update(parse_ast(ast.stdout.decode('latin-1'), wanted, sysnames))
    # Names the C and C++ libraries or the SDK declare too (NULL, select,
    # access, std::max) are not Windows names; names the client declares
    # itself (a class XMLDocument of its own, a #define) are its own. Both are
    # dropped, except the names the port keeps by rule.
    own = set()
    for path in {nowin.canonical(p) for p in active}:
        if path in wanted:
            continue
        for l in open(path, encoding='latin-1'):
            m = OWN.match(l)
            if m:
                own.add(m.group(1) or m.group(2))
    dropped = {}
    for w in list(universe):
        if (w in sysnames or w in own) and group_of(w, universe[w][1], None, rules_only=True) != 'keep':
            dropped[w] = 'library' if w in sysnames else 'client'
            del universe[w]

    # 3. which adapter defines a function
    adapters = {}
    port = os.path.join(ROOT, 'port')
    walks = [os.walk(os.path.join(port, 'compat')),
             os.walk(os.path.join(port, 'client', 'src'))]
    for top, dirs, files in (w for walk in walks for w in walk):
        if 'winheaders' in top or '/test' in top:
            continue
        for fn in sorted(files):
            if not fn.endswith('.cpp') or fn.startswith('bifrost-'):
                continue
            path = os.path.join(top, fn)
            relp = os.path.relpath(path, port)
            for l in open(path, encoding='latin-1'):
                if not l[:1].isalpha() or l.rstrip().endswith(';') or l.startswith('#'):
                    continue
                m = re.match(r'^[^(=;]*?\b([A-Za-z_]\w*)\s*\(', l)
                if m and m.group(1) in universe:
                    adapters.setdefault(m.group(1), relp)

    # Member functions of the client's own classes that bear a Windows
    # function's name, because windows.h's macros renamed them before the
    # code was recovered (UIWindow::TextOutA, DrawDC::GetTextExtentPoint32A,
    # UIWindowMgr::CreateDCA, CRenderer::SetPixelFormat). Once no adapter
    # defines the Windows function, an unqualified use of the name (a
    # declaration in the class, a call inside a member) is the member's: a
    # call of the Windows function would not link. Dropped as the client's.
    members = set()
    for path in {nowin.canonical(p) for p in active}:
        if path in wanted:
            continue
        for l in open(path, encoding='latin-1'):
            for m in MEMBER_DEF.finditer(l):
                if m.group(1) in own:
                    members.add(m.group(2))
    for w in list(universe):
        if (w in members and universe[w][0] == 'FunctionDecl' and w not in adapters
                and group_of(w, universe[w][1], None, rules_only=True) != 'keep'):
            dropped[w] = 'client'
            del universe[w]

    # 4. count
    rows = collections.Counter()
    for path in sorted({nowin.canonical(p) for p in active}):
        if path in wanted:
            continue  # a Windows or library header copied into src/
        lines, toks, conds = lex(path)
        act = set().union(*[v for k, v in active.items() if nowin.canonical(k) == path])
        # segments between conditional directives
        seg_active, start = {}, 0
        segs = []
        for i, c in enumerate(conds + [True]):
            if c:
                segs.append((start, i))
                start = i + 1
        on = [False] * len(lines)
        for a, b in segs:
            if any((k + 1) in act for k in range(a, b)):
                for k in range(a, b):
                    on[k] = True
        r = os.path.relpath(path, ROOT)
        for k, ts in enumerate(toks):
            if not on[k] or conds[k]:
                continue
            for w, member in ts:
                if not member and w in universe:
                    rows[(w, r)] += 1

    table = []
    for (w, r), n in rows.items():
        kind, header = universe[w]
        table.append((w, group_of(w, header, adapters.get(w)), kind, header, r, n))
    if o.json:
        with open(o.json, 'w') as f:
            json.dump([dict(zip(('name', 'group', 'kind', 'header', 'file', 'count'), t))
                       for t in sorted(table)], f, indent=0)

    if TYPES:
        missing = sorted({t[0] for t in table if t[1] == 'keep' and t[0] not in TYPES})
        print('keep group, not declared by port/client/include/bf_types.h (%d): %s\n'
              % (len(missing), ' '.join(missing) or '-'))
    order = [g for g, _ in GROUPS]
    title = dict(GROUPS)
    by_group = collections.defaultdict(list)
    for t in table:
        by_group[t[1]].append(t)
    print('units: %d   src files with a name: %d   names in the headers: %d   adapters '
          'defining one: %d' % (len(entries), len({t[4] for t in table}), len(universe),
                                len(adapters)))
    print('dropped: %d names the C/C++ library or SDK also declare, %d the client declares '
          'itself' % (sum(1 for v in dropped.values() if v == 'library'),
                      sum(1 for v in dropped.values() if v == 'client')))
    if o.names:
        for k in ('library', 'client'):
            print('  %s: %s' % (k, ' '.join(sorted(w for w, v in dropped.items() if v == k))))
    print('\n%-7s %-40s %7s %6s %6s' % ('group', '', 'sites', 'files', 'names'))
    for g in order:
        ts = by_group.get(g, [])
        print('%-7s %-40s %7d %6d %6d' % (g, title[g], sum(t[5] for t in ts),
                                         len({t[4] for t in ts}), len({t[0] for t in ts})))
    for g in order:
        if o.step and g not in o.step:
            continue
        ts = by_group.get(g, [])
        if not ts:
            continue
        print('\n== %s %s: %d sites in %d files, %d names' % (
            g, title[g], sum(t[5] for t in ts), len({t[4] for t in ts}), len({t[0] for t in ts})))
        per_file = collections.Counter()
        for t in ts:
            per_file[t[4]] += t[5]
        if o.files != 0:
            items = sorted(per_file.items(), key=lambda kv: (-kv[1], kv[0]))
            if o.files > 0:
                items = items[:o.files]
            print('files: ' + ', '.join('%s:%d' % kv for kv in items))
        per_name = collections.Counter()
        name_files = collections.defaultdict(list)
        for t in ts:
            per_name[t[0]] += t[5]
            name_files[t[0]].append('%s:%d' % (t[4], t[5]))
        items = sorted(per_name.items(), key=lambda kv: (-kv[1], kv[0]))
        if o.names:
            for w, n in items:
                kind, header = universe[w]
                print('  %-34s %5d  %-16s %s  [%s]' % (w, n, kind, header,
                                                      ', '.join(sorted(name_files[w]))))
        else:
            print('names: ' + ', '.join('%s %d' % kv for kv in items[:40])
                  + (' ...' if len(items) > 40 else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main())
