# compat/bink: the Bink SDK of CBink on the port's Bink reader

Stage A adapter (M12.4a; M11 deletes it). `src/movie/CBink.cpp` plays the intro movie with RAD's Bink SDK (binkw32 1.5). Under `BF_PORT` it includes [`port/compat/include/bf_bink_sdk.h`](../include/bf_bink_sdk.h) instead of the SDK's `RAD.H` and `BINK.H`, and its own code runs unchanged: the names below come from this adapter, on the Bink reader [`port/native/formats/bink`](../../native/formats/bink), the sound module [`port/native/audio`](../../native/audio) and the sys clock. Pictures go the client's own way: `BinkCopyToBuffer` into the locked back buffer, then `Blt` of the movie's rectangle to the primary, which the DirectDraw adapter stretches over the window ([d3d README](../d3d/README.md)).

## Names

| SDK name | Port | Notes |
|---|---|---|
| `BinkOpen(name, flags)` | reads the file (`bf_file_resolve` in the game folder; an absolute path as it is) and `bf_bink_open_memory` | `BINKFROMMEMORY`: `name` is the file's bytes, the size from the header. NULL and a `gfx` trace line (`bink: openning.bik not found in the game folder`) when absent or not a Bink 1 movie; counter `bink.missing` |
| `BfBinkOpenMemory(data, size, name)` | `bf_bink_open_memory` | port only: what `CBink::InitOpenning` uses for a movie from the archives (`BF_INTRO_MOVIE`); the name is for the trace and the control socket |
| `BinkDoFrame` | `bf_bink_decode_next`, `bf_bink_seek` when `FrameNum` is not the next frame | sets `LastFrameNum`; the first frame starts the clock and the sound |
| `BinkNextFrame` | `FrameNum + 1`; after the last frame back to 1, as the SDK | |
| `BinkWait` | the sys clock (`bf_clock_us`) against `(FrameNum - 1) / fps` after frame 1 | non-zero while early and while paused. Frames are never skipped |
| `BinkCopyToBuffer` | `bf_bink_copy_pixels` | `BINKSURFACE32` → BGRA, `565`, `555`/`5551` → 1:5:5:5; the width is `pitch / bytes`. Others: -1 and one trace line |
| `BinkDDSurfaceType` | `IDirectDrawSurface7::GetPixelFormat` | 32-bit X8R8G8B8 → `BINKSURFACE32` (the DirectDraw adapter's back buffer), 16-bit 5:6:5 and 5:5:5 |
| `BinkGoto(b, frame, flags)` | `FrameNum = frame` (clamped), clock and sound moved to it | `Break('g')` goes to 1500, i.e. the last frame |
| `BinkPause` | stops the clock and the sound; resuming starts the rest of the track | `CBink::Pause` does not call it under `BF_PORT` (spec section 3: the client keeps running in the background) |
| `BinkClose` | `bf_bink_close`, the voice stopped | trace `bink: close <name> after frame n of m (ended|stopped)` |
| `BinkSetSoundSystem`, `BinkOpenDirectSound` (`BinkSoundUseDirectSound`) | a flag: sound wanted | |
| `BinkGetError` | the last failure as text | |

**Sound.** At `BinkOpen` the first sound track is decoded completely from a second reader handle with the picture turned off (`bf_bink_set_video_enabled`, added to the reader for this), cut to the picture's length, and played as one sound-module voice (volume 1, under the effects volume) started with frame 1 and tagged with the movie's name, so it shows in the control socket's `sound` part. Pause and `BinkGoto` stop it and start the rest from the movie's position. When the engine has not been started yet (the intro plays before `InitMSS`), the adapter starts it (`bf_audio_start`, device); Miles adopts it later.

**State.** `bf_compat_bink_get_state` ([include/bf_compat_bink.h](include/bf_compat_bink.h)): the open movie, its frame, sound and counts of movies opened and closed, for the control socket's `movie` part and event (`port/compat/stubs/bf_control_movie.cpp`, PROTOCOL.md).

## The client's side (src/movie/CBink.cpp under BF_PORT)

- `WinMain` calls `g_Bink.OpenningMain(1)` before the login mode, as the 2008 executable does (the call was missing from the reconstruction).
- `OpenningMain(1)` returns at once when the option `SkipIntroScreens` is on (the default; trace `intro: skipped (option SkipIntroScreens)`), then, as in 2008, when the registry's `BINKMODE` is 1 (a key other than R stopped the movie once). `OpenningMain(0)` (the login window's intro button) always plays.
- `InitOpenning`: `openning.bik` from the game folder, as in 2008; `BF_INTRO_MOVIE` names another movie, an absolute path or a name the file manager finds in the archives or the game folder (`data\video\ad\01.bik`). Absent: `intro: no movie (...)`, and the client goes on.
- The loop calls `bf_port_frame` (control socket, window size, captures) and stops on `g_sysQuit` (Cmd+Q) as well as on `WM_QUIT`; any key stops the movie as in 2008 (`WndProc` → `CBink::Break`).
- The back buffer is locked `DDLOCK_WRITEONLY`: Bink only writes, and the adapter would otherwise read the whole target back every frame.
- Trace lines (`ui`): `intro: start, N frames`, `intro: end at frame n of N (ended|stopped)`.
- The login window's intro button: `UILoginWnd` offers it only when `..\openning.bik` exists and only for the services 0, 3, 10, 11, 13 and 14 (confirmed in the 2008 executable); the port's data is service america, so it never shows, as in 2008.

## Build and test

```sh
cmake --preset debug && cmake --build --preset debug --target bf_compat_bink bifrost-compat-bink-test
build/cmake/debug/compat/bink/bifrost-compat-bink-test
```

The test runs CBink's call sequences through the SDK names, drawing through the DirectDraw adapter offscreen (640x480), with its own clock (`bf_clock_set_source`) and the sound engine OFFLINE: missing and bad movies, `BinkDDSurfaceType`, the public sample `test/bink/samples/original.bik` played whole (frame 1 copied into the back buffer exactly, the sound voice started with frame 1, `BinkWait`'s 40 ms, a 100 ms pause, the length to the microsecond), the stretched window checked at 25 points of frame 140, `BinkGoto(1500)`, stopping early, `BINKFROMMEMORY`, no sound system. Without the sample under the game folder the movie checks say `skipped`. Scripted play in the client: `port/harness/scripts/intro.py`.

## Paragraph for port/README.md

> | [`bink`](compat/bink/README.md) | the Bink SDK names `src/movie/CBink.cpp` calls (`BinkOpen` ... `BinkClose`, `BinkDDSurfaceType`, the sound system); the intro movie with its sound | `formats/bink`, `audio` |
