# bf_bink: Bink 1 movie decoder

Decodes the Bink movies the client plays (`src/movie/CBink.cpp`, the intro
`openning.bik`): video into YUV planes or 32/16-bit pixels, sound into
16-bit PCM. Our own code, written from public descriptions of the format;
what the format is and how it was established is in [FORMAT.md](FORMAT.md).
Links Accelerate (the audio transforms) and nothing else.

The contract is [include/bf_bink.h](include/bf_bink.h).

## Build and test

```
cmake -S port/native/formats/bink -B build/native/bink -G Ninja
cmake --build build/native/bink
build/native/bink/bifrost-bink-test [--yuv] [--every N] [file.bik ...]
```

Without game data the test checks the prefix codes, the audio transforms
against plain sums, and decodes a 16x16 movie it builds bit by bit. With
the game folder (`BF_CLIENT_DIR`, default `~/Games/Bifrost`) it decodes
`data\video\ad\01.bik` from `data.grf`, `openning.bik` from the folder if
present, every `*.bik` in `test/bink/samples/`, and files named on the
command line, completely; it writes every 30th frame as PNG and each audio
track as WAV into `test/bink/<name>/` (game artwork, outside git). `--yuv`
adds all frames as raw yuv420p for comparison with another decoder.

## For the client adapter (M12.4a)

How the Bink SDK calls in `CBink.cpp` map:

| SDK | bf_bink |
|---|---|
| `BinkOpen(name, 0)` | `bf_bink_open_file` (look the name up in the game folder) |
| `bink->Frames`, `FrameNum` (1-based) | `info.frames`, `bf_bink_frame() + 1` |
| `BinkDoFrame` | `bf_bink_decode_next` (decodes the frame and queues its sound) |
| `BinkCopyToBuffer(..., BinkDDSurfaceType(s))` | `bf_bink_copy_pixels` with `BF_BINK_BGRA8888`, `RGB565` or `XRGB1555` |
| `BinkNextFrame` | nothing: the next `decode_next` moves on |
| `BinkWait` | the adapter's clock against `rate_num / rate_den` (the module reads no clock) |
| `BinkGoto(b, 1500, BINKGOTOQUICK)` | `bf_bink_seek(b, 1499)`; movies have one key frame, so this decodes from the start |
| `BinkPause` | the adapter stops calling and pauses its voice |
| sound (`BinkSoundUseDirectSound`) | feed `bf_bink_audio_read` into one audio voice per track |
| `BinkClose` | `bf_bink_close` |

The adapter (`port/compat/bink`) reads a movie's sound ahead with a second
handle and `bf_bink_set_video_enabled(b, 0)`, which decodes only the sound
(the test checks that it yields the same samples).

The decoder yields slightly more sound than picture (up to one block, about
30 ms; see FORMAT.md, "Length"); a player that follows the picture can drop
the rest.

## Untested

Revisions `c`, `d`, `g`, `h`, `k`, the alpha plane and the header's doubling
flags: no such files were found. Revisions `f` and `i` are tested against a
reference decoder (identical pictures except for 20 pixels off by one in 300
million; sound within 1 LSB). The game's own intro movie was not found.

## Paragraph for port/README.md

> **Bink** (`port/native/formats/bink`): our decoder for Bink 1 movies, for
> the intro (M12.4). `bifrost-bink-test` decodes `data\video\ad\01.bik` from
> `data.grf` and any `*.bik` under `~/Games/Bifrost/test/bink/samples/`,
> writing frames and sound to `~/Games/Bifrost/test/bink/`. The format and
> its evidence are in `port/native/formats/bink/FORMAT.md`.
