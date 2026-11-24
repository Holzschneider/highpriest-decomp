# strings: displayed Korean literals (M9.2)

About 70 texts in the client's code are shown to the player without going through `msgStringTable.txt`: billing notices, pet states, name-balloon brackets, the server-list captions, error boxes (technical plan 3.5). The source keeps the original CP949 literal; this module replaces it with English at run time.

## Pieces

| What | Where |
|---|---|
| Scanner and classifier | `port/tools/strings.py` |
| Every Korean literal with its class and the reason | `port/data/strings/literals.tsv` |
| Review list of the decisions, grouped by rule | `port/data/strings/review.txt` |
| The table to translate: original bytes (hex), English, Korean for reference | `port/data/strings/translation.tsv` |
| The call sites `bf_tr()` will wrap | `port/data/strings/callsites.tsv` (file, line, function, shape, hex key) |
| The lookup | `include/bf_strings.h`, `src/bf_strings.cpp` (static library `bf_strings`) |
| Its test | `test/strings_test.cpp` (`bifrost-strings-test`) |

`port/data/assemble.py` copies `translation.tsv` to the game folder as `data/bf_strings.tsv`; the lookup reads it at run time (`bf_strings_load(path)`, or on the first `bf_tr` from `$BF_STRINGS_FILE`, else `$BF_CLIENT_DIR/data/bf_strings.tsv`). A translation therefore changes without a rebuild.

The lookup is a native module, not a compat adapter, because it stays after M11: it is called from `src/` and has no Windows shape.

## Commands

```
python3 port/tools/strings.py --summary          # counts per class and per reason
python3 port/tools/strings.py                    # rewrite port/data/strings/* (translations already entered are kept)
python3 port/tools/strings.py --check            # files in sync with the source; every English text keeps the printf conversions and is Windows-1252
cmake -S port/native/strings -B build/native/strings -G Ninja && cmake --build build/native/strings && build/native/strings/bifrost-strings-test
```

Run `strings.py` again after any change to a displayed literal in `src/`; `--check` fails when the files are stale.

## Classes

- **displayed**: drawn, put in chat, a message box (`ErrorMsg` is a `MessageBoxA`), a window caption, a name balloon, a format string shown to the player.
- **path**: file and folder names inside the archives. They stay as they are. The call (`UIBmp`, `SetSprName`, `PlayWave`, `Get`, ...) or the variable it is assigned to (`m_pcJobNameTable`, `bmpName`, ...) decides first, the shape of the text (a folder separator, a resource extension) second.
- **other**: the built-in Korean message table (replaced by `msgStringTable.txt` for the `america` service), debug traces, registry and log text, typed chat commands (`/ㅗ`, `/기쁨`), strings compared with text, the anti-cheat modules (not run for the English service), obfuscated and binary byte strings.

A decision that is wrong for one literal is corrected in `OVERRIDES` at the top of the classification section of `strings.py` (`file:line` to class and reason), not by editing the generated files.

## For the step that wires it (M9.3 and after)

Wrap each site of `callsites.tsv`: `bf_tr("...")` for an argument, assignment or return; the translation can be longer than the original, so the buffers they end in (`char msg[256]`, `strcpy` into fixed fields) need a length check at the site. Initialisers of static data would run before the table is loaded; none of the sites is one (shape column). Call `bf_strings_load()` once at start-up with the game folder's `data/bf_strings.tsv`; M9.5 reads `bf_strings_get_stats()` and `bf_strings_each_fallback()`.

## Doubtful

- `sexTable` (`Session2.cpp`): Korean kinship words searched in the pet's speech and replaced by their other-gender form. Both halves are classed `other`, because the English `pettalk` table cannot contain them; if a service ever feeds Korean pet text, the replacement half is displayed.
- The anti-cheat modules (`cSecureGamePack`, `cKeyProtector`, `FindHack`, `NPGameLib`) hold a few message boxes. They are classed `other` on the assumption that the `america` service type never runs them (technical plan 3.5 says it skips GameGuard).

## Coverage report (M9.5)

`include/bf_coverage.h`, `src/bf_coverage.cpp` (in `bf_strings`, with CoreFoundation for CP949 and Windows-1252 decoding) collect, per session, what was not English: the `bf_tr` fallbacks (`untranslated`, with the caller of `bf_tr` as `symbol+offset`), table lookups that fell back to a placeholder (`placeholder`: the client's `bf_port_missing` with a `table.missing.<table>` counter — items, jobs, skills, message ids, map names), and text the client draws that is Korean (`korean`: CP949 bytes that decode as Hangul, drawn as Hangul or as Windows-1252 mojibake) or outside Windows-1252 (`foreign`). The drawing hook is in the gdi adapter's `TextOut` (`port/compat/gdi`); each distinct text is recorded once with a count and the first call frames outside the drawing helpers. The client writes the report as JSON at exit to `BF_COVERAGE=<path>` and serves it as the control socket's `coverage` part. `port/harness/scripts/english_session.py` plays the M9 session and passes only when the report is empty.
