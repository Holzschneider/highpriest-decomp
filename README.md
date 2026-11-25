# HighPriest decompilation

C++ source for the Ragnarok Online HighPriest client (kRO, built 2008-11-05), reconstructed from the original executable and its program database. Compiled with the compilers of the time, it links to an executable that equals the original `HighPriest.exe` byte for byte:

```
SHA-256  9abc36c88376387d85c878c0c879d9af8ef66eb1f5152638085dc6995e3f4742
size     3,338,345 bytes (PE32, i386)
```

The source covers the game and the libraries the original linked statically: the Visual C++ runtime, Lua 5.0, the GameGuard client library, and the objects of the DirectX and Miles libraries. The DLLs the game imports are outside the scope.

## Building

The Windows build (the `Makefile`, the compiler and library lists of `sources.mk`, the compilers and SDK files of `tools/`, the runtime sources of `vendor/`, and the parts of `src/` only it compiles: `src/library/`, `src/imports/`, `src/res/`, GameGuard and the other units the port leaves out) is on `main`, where `make check` builds `HighPriest.exe` and compares it with the original byte for byte. This branch, `macos-port`, removed them in M11.11: it builds the macOS client only, with CMake and Ninja, `cmake --preset debug && cmake --build --preset debug`. [port/README.md](port/README.md) ("Build and fast check") describes the presets, the targets and the checks.

## Steering markers

Most functions match from ordinary source. In some, Visual C++ 6.0 makes a decision (which calls to inline, the order of commutative operands, the order of sections) from something the surviving evidence does not show, such as the number of declarations a lost header held. Those places carry a marker: a macro from one of the `src/rag_*.h` headers that changes that count and emits no code.

| Marker | Uses | Stands in for |
|---|---:|---|
| `RAG_BUDGET_FILL` | 41 | source that enlarged a function's inline budget |
| `RAG_BUDGET_SITES` | 15 | inline candidate call sites |
| `RAG_ENTROPY_SHIFT`, `RAG_ENTROPY_FILL_256` | 20 | declarations ahead of a function |
| `RAG_SYMBOL_SHIFT`, `RAG_SYMBOL_CHAIN` | 5 | symbols inside a function |

They are in 22 files; `grep -rlE 'RAG_(BUDGET|ENTROPY|SYMBOL)' src --exclude='rag_*.h'` lists them. Of the image's 7,546 procedures, 231 pieces of code (305,026 bytes) match only with a marker in effect. Each header explains its marker, and [docs/compiler-notes.md](docs/compiler-notes.md) records the compiler behaviour behind them.

## Layout

- `src/`: the reconstruction. `src/library/` holds the sources of statically linked libraries, `src/imports/` the definitions of the import libraries, `src/res/` the resources.
- `vendor/`: Visual C++ runtime sources that `src/library/` includes.
- `tools/`: the compilers and SDK files the build reads ([docs/toolchain.md](docs/toolchain.md)), and `tools/stamp.py`.
- `docs/`: the toolchain's provenance and the compiler notes.
- `build/`: outputs, untracked.

## macOS port

This branch recovers a general Ragnarok Online client from the source and ports it to macOS as a native arm64 application. [docs/port/spec.md](docs/port/spec.md), [docs/port/technical-plan.md](docs/port/technical-plan.md) and [docs/port/implementation-plan.md](docs/port/implementation-plan.md) describe it; [port/](port/README.md) holds its tools, probes and test server. The Windows build above does not read `port/`.

## Licence

The build files, `tools/stamp.py` and the documentation are dedicated to the public domain under [CC0 1.0](CC0-1.0.txt). Everything else belongs to its owners and is not covered: the reconstructed game source, the bundled compilers, SDK files and runtime sources, and the third-party libraries. [THIRD-PARTY.md](THIRD-PARTY.md) lists them with the notices their licences require.
