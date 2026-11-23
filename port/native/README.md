# Native modules

The parts of the port that stay: code written for macOS, with no Windows types, that the client will call directly once the adapters in `port/compat` are gone (M11). Each module is developed and tested on its own, before and independently of the client.

## Layout

```
port/native/<module>/
    include/bf_<module>.h    the contract
    src/                     C++17 or Objective-C++ (ARC)
    test/                    the test program's sources
    CMakeLists.txt
    README.md                what it offers, how to test it, what is untested
```

| Module | Offers |
|---|---|
| `sys` | The clock, threads, the game folder and file lookup, the write locations, settings, the Keychain, alerts |
| `codec` | zlib and JPEG decoding |
| `app` | The window, the event queue, key and pointer state, the clipboard |
| `control` | The control socket: JSON lines, command registry |
| `gfx` | The Metal renderer for the client's pre-transformed vertices |
| `text` | CoreText drawing into bitmaps |
| `audio` | AVAudioEngine voices and one music stream, decoding |
| `formats/granny`, `formats/bink` | Readers for the two proprietary formats |

## Rules for every module

- **The header is the contract.** It is a C header usable from C++14 (`extern "C"`, plain C types, opaque handles, no exceptions across the boundary, no Objective-C types: an AppKit or Metal object crosses as `void *` with its class named in a comment). Names start with `bf_<module>_`. Every function documents its thread: the client's main thread unless it says otherwise.
- **No Windows types and no client headers.** A module does not include anything from `src/` or `port/compat`. What the client needs in Windows shape is the adapter's job. Reading `src/` to learn what the client needs is expected.
- **Strings:** file paths and displayed text cross as UTF-8 unless a function says it takes bytes in a named code page (the text module does, because the client holds CP949 and CP1252 bytes).
- **One clock.** Only `sys` reads the time (`bf_clock_*`). Other modules call it or take a time as an argument.
- **One event queue.** Input reaches the client only through the queue in `app`; the control socket injects into the same queue.
- **Drawing goes to a target the caller names:** a `CAMetalLayer` or an offscreen texture. Nothing assumes a visible window.
- **Failure is a return value** plus a trace line through `port/instrument/bf_trace.h`, never an abort or an alert of the module's own.
- **Apple frameworks and zlib only.** No third-party code.
- **Build:** `CMakeLists.txt` defines a static library `bf_<module>` and a test program `bifrost-<module>-test`. It works standalone (`cmake -S port/native/<module> -B build/native/<module> -G Ninja && cmake --build build/native/<module>`) and under `add_subdirectory`, so it sets properties on its own targets only: C++17, `-fobjc-arc` for `.mm`, deployment target macOS 14, arm64.
- **Test program:** prints one line per check, exits non-zero on a failure, and runs without game data. Checks that need game data read it from `BF_CLIENT_DIR` (default `~/Games/Bifrost`) and say `skipped` when it is absent. Pictures it writes go to the build directory, or under `~/Games/Bifrost/test/` when they show game artwork.
