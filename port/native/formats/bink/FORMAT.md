# The Bink 1 format, as far as this decoder establishes it

Bink is RAD Game Tools' movie format. The client plays one Bink movie, its
intro (`openning.bik`, `src/movie/CBink.cpp`). This file describes what
`bf_bink` reads, how each part was established, and where it stops.
All integers are little endian. "Bits" are read from a little-endian bit
stream: each byte from its lowest bit up, bytes in order.

## Files examined

| File | Where | Signature | Size | Content |
|---|---|---|---|---|
| `data\video\ad\01.bik` | inside the game's `data.grf` (zlib entry, 2,222,584 bytes) | `BIKi` | 512x256 | 450 frames at 10000000/333333 = 30.000 fps, no audio |
| `openning.bik` | not found: not in the June 2021 kRO install (loose files, `data.grf`, `rdata.grf`) nor in the other client folders searched | | | |
| `ActivisionLogo.bik` | public sample (samples.ffmpeg.org) | `BIKi` | 720x486 | 312 frames at 29.97 fps, stereo DCT audio 22500 Hz |
| `logo_lucas.bik` | public sample | `BIKi` | 640x480 | 266 frames, stereo DCT audio 44000 Hz |
| `logo_legal.bik` | public sample | `BIKi` | 640x480 | 150 frames, no audio |
| `original.bik` | public sample | `BIKi` | 512x384 | 280 frames at 25 fps, stereo RDFT audio 44100 Hz |
| `Fmv12.bik` | public sample | `BIKf` | 640x480 | 301 frames at 20 fps, stereo RDFT audio 44100 Hz |

The samples stand in for the movie the game does not ship: they exercise the
audio codecs and an older revision. None of them, nor any decoded frame, is in
git; they live under `~/Games/Bifrost/test/bink/`. No Bink 2 (`KB2`) file was
found anywhere.

`CBink.cpp` copies a 512x347 rectangle (`0x200 x 0x15b`) of the decoded
frame to the screen, so the 2008 intro was probably 512 pixels wide.

## Container

Header, 44 bytes (48 for revision `k`):

| Offset | Size | Meaning | 01.bik |
|---|---|---|---|
| 0 | 3 | `BIK` (Bink 2: `KB2`) | `BIK` |
| 3 | 1 | revision letter: `b` (oldest), `d`, `f`, `g`, `h`, `i`, `k` | `i` |
| 4 | 4 | file size minus 8 | 2,222,576 |
| 8 | 4 | number of frames | 450 |
| 12 | 4 | largest frame record in bytes | 22,488 |
| 16 | 4 | number of frames again (all files examined) | 450 |
| 20 | 4 | width | 512 |
| 24 | 4 | height | 256 |
| 28 | 4 | frame rate numerator | 10,000,000 |
| 32 | 4 | frame rate denominator | 333,333 |
| 36 | 4 | video flags: 0x00100000 alpha plane, 0x00020000 grey; bits 28..31 a width/height doubling or interlacing mode (MultimediaWiki; not implemented) | 0 |
| 40 | 4 | number of audio tracks | 0 |
| (44) | 4 | revision `k` only: unknown, skipped | |

Then, per audio track in turn: 4 bytes largest decoded packet (bytes of PCM),
then 2 bytes sample rate + 2 bytes flags (0x4000 16-bit, 0x2000 stereo,
0x1000 DCT codec, else RDFT), then 4 bytes track id.

Then the frame index: frames + 1 offsets from the file start. Bit 0 marks a
key frame; the last entry is the end of the file. In 01.bik the first
offset is 0x739 (= 1848 | key) = 44 + 451 * 4, the last 2,222,584. Every file
examined has a single key frame, the first, so seeking means decoding from
the start.

A frame record is, for each audio track, a 4-byte length and that many bytes
(an audio packet, or nothing when the length is 0), followed by the video
record, which runs to the next index entry.

## Video

Planes Y, Cb, Cr at 4:2:0 (plus an alpha plane at luma size when flagged),
coded in 8x8 blocks: luma `ceil(w/8) x ceil(h/8)` blocks, chroma
`ceil(w/16) x ceil(h/16)`. Pixel values are video range (16..235) before
revision `k`.

Video record: if alpha, (revision >= `i`: 32 bits skipped) the alpha plane;
then (revision >= `i`) 32 bits skipped (a size, unused); then the planes Y,
and the two chroma planes, in the order Cb, Cr before revision `h` and Cr, Cb
from `h` on. After each plane the stream is aligned to 32 bits. Revision `k`
may code a whole plane as one fill value (1 bit, then 8 bits).

**Bundles.** Each plane's data is split into nine streams of values
("bundles"): block types, 16x16 sub-block types, colours, two-colour patterns,
motion x, motion y, intra DC, inter DC, run lengths. At the start of a plane
each bundle reads its symbol code (the colours bundle first reads 16 codes for
its high nibbles, chosen by the previous high nibble); the two DC bundles have
none. A symbol code is 4 bits selecting one of 16 fixed prefix codes for the
values 0..15 (code 0 is "4 raw bits"), and a mapping of the code's leaves to
values: either a list of up to 8 values given first, the rest ascending, or
1..4 passes that merge neighbouring runs of the identity order bit by bit.

Before each row of blocks, every bundle whose values are all used reads a
count (its width grows with the plane width: `floor(log2(n + 511)) + 1` bits,
n = blocks per row, or a multiple of it); a count of 0 ends the bundle for the
plane. Then the values: block types (with symbols 12..15 meaning "repeat the
last type 4, 8, 12, 32 times"), colours (high nibble, low nibble; sign and
magnitude around 128 before revision `i`), patterns (two nibbles per byte),
motion (value and sign), runs, each with a one-bit "all the same" shortcut;
DCs as an 11-bit start (inter: 10 bits and sign) and deltas in groups of 8 with
a 4-bit width per group.

**Block types** (from the block-type bundle; `16x16` takes its kind from the
sub-type bundle and covers a 2x2 group of blocks, the other three entries of
which are skipped):

| Type | Block |
|---|---|
| 0 | skip: the previous frame |
| 1 | 16x16: run, intra, fill, pattern or raw at half resolution, doubled |
| 2 | motion: 8x8 of the previous frame at an offset (x, y bundles) |
| 3 | runs of colours along one of 16 scan patterns (4 bits) |
| 4 | motion plus a residue (7-bit count, coefficients by bit planes) |
| 5 | intra DCT: DC from its bundle, AC coefficients, 4-bit quantiser |
| 6 | fill with one colour |
| 7 | motion plus a DCT-coded difference |
| 8 | two colours, one bit per pixel from 8 pattern bytes |
| 9 | 64 raw colour values |

**Coefficients** of DCT and residue blocks are coded in a scan order of 2x2
groups by bit planes from the most significant: a list of pending groups,
each opened by a bit, splits into 4-coefficient groups and single
coefficients (see `read_dct_coeffs` and `read_residue` in
`src/bf_bink_video.cpp`).

**Quantisers.** 16 intra and 16 inter tables, indexed by scan position. This
decoder computes them as `round(seed[p] * s[row] * s[col] * step * 4096)`
with the base matrices `seed` (`kIntraSeed`, `kInterSeed`), the transform's
scale factors `s0 = 1, sk = sqrt(2) cos(k pi/16)`, and the 16 steps
1, 4/3, 5/3, 2, 8/3, 7/2, 4, 5, 6, 8, 12, 17, 22, 28, 34, 44. Compared with
the precomputed tables in the public decoder, 84 of the 2048 entries differ
by one unit in 4.12-ish fixed point. No rounding rule tried (double, float,
rounded factors at 2^-8..2^-31) reproduces them all. Effect, measured: 20
luma pixels in about 300 million differ by 1 from the reference decoder over
the five files with DCT-heavy content; with the precomputed tables the output
is identical (checked in a scratch build, not committed).

**Transform.** An integer 8x8 inverse DCT of the AAN kind (constants 2896,
3784, -5352, 2217 with products shifted right by 11), columns first, then rows
rounded with `(x + 127) >> 8`.

**Pixel arithmetic.** Results are taken modulo 256, not clamped. Evidence:
with clamping, whole blocks came out 0 or 255 inside areas of 180..220 (for
example `ActivisionLogo.bik` frame 46 at 544,376: an intra block of 0s whose
neighbours are 184..208); modulo 256 gives values that continue the
neighbourhood and matches the reference exactly.

## Audio

One packet per track per frame: 4 bytes "decoded size" (bytes of 16-bit PCM
the packet stands for), then blocks until the packet ends, each aligned to 32
bits. Block length `N` (the frame length) is 512 below 22050 Hz, 1024 below
44100 Hz, 2048 from 44100 Hz up. The RDFT codec codes the channels interleaved
as one signal at `channels x rate`, so `N` doubles for stereo; the DCT codec
codes each channel separately within one block (after 2 unused bits).

Per channel: two floats (5-bit exponent, 23-bit mantissa, sign; raw IEEE in
revision `b`), the DC and the Nyquist (RDFT) or first two (DCT) coefficients;
one 8-bit quantiser per critical band (bands from the Bark edges 100 Hz ...
24.5 kHz, scaled to `N` and the rate, quantiser `exp(i * 0.152892) * root`,
`root = 2 / (sqrt(N) * 32768)` for RDFT, `sqrt(N) / 32768` for DCT);
then the coefficients from index 2 in runs (8, or 8 times a table value) of a
4-bit width, each nonzero value followed by its sign.

Transforms, as established against the reference decoder's output:

- RDFT: `x[t] = c0/2 + c1/2 (-1)^t + sum_k (c[2k] cos(2 pi k t/N) + c[2k+1] sin(2 pi k t/N))`.
- DCT: `x[t] = (2/N) (c0/2 + sum_k c[k] cos(pi k (t + 1/2)/N))` with `c0`
  doubled first. The factor 2/N was found by comparison: with 1/N the output
  correlated with the reference at 0.9999993 and was exactly half as loud.

Each block emits `N - N/16` samples (per coded signal); its first `N/16`
samples are cross-faded linearly with the last `N/16` of the previous block.

**Length.** The decoder produces a little more sound than the movie lasts:
`original.bik` 11.233 s for 11.200 s of video, `logo_lucas.bik` 8.880 s for
8.867 s. The packets' decoded-size fields add up to the movie length exactly
(`original.bik`: 1,975,680 bytes = 493,920 frames = 11.200 s), so the excess
is the encoder's padding of the last block; a player can stop at the
declared total.

## Verification

`bifrost-bink-test --yuv` writes every decoded frame as raw yuv420p and each
track as WAV; the same files decoded by FFmpeg 7 (`ffmpeg -i X -f rawvideo
-pix_fmt yuv420p`, `-f s16le`) were compared byte by byte:

| File | Frames | Pixels differing | Audio |
|---|---|---|---|
| 01.bik | 450 | 0 | none |
| logo_legal.bik | 150 | 0 | none |
| Fmv12.bik (`f`) | 301 | 0 | RDFT: max difference 1 LSB, SNR 104 dB |
| original.bik | 280 | 2 (by 1) | RDFT: max 1 LSB, 108 dB |
| logo_lucas.bik | 266 | 1 (by 1) | DCT: max 1 LSB, 79 dB |
| ActivisionLogo.bik | 312 | 17 (by 1) | DCT: max 1 LSB, 70 dB |

Chroma planes are identical in every file. The remaining pixel differences are
the quantiser tables (above).

## Where it stops

- Revision `b` (the oldest Bink 1, a different bitstream) is refused at open.
- Revisions `c`, `d`, `g`, `h` and `k`, the alpha plane and the grey flag are
  implemented from the description but no file of those kinds was available.
  `h` differs from `f` (tested) only in the chroma order and from `i` (tested)
  only in the colour coding and the 32-bit skips.
- Bink 2 (`KB2`) is refused.
- The doubling and interlacing modes of the header flags (bits 28..31) are
  not implemented: the planes come out at the coded size. All files examined
  have these bits clear.
- Chroma order before revision `h`: this decoder follows the reference
  decoder (Cb first), and its output for `Fmv12.bik` (`f`) is identical to the
  reference's. MultimediaWiki says the opposite (`f` and `g` "YVU"). The
  sample's colours are too close to grey to tell by eye (mean chroma within
  4 of 128), so for an `f` or `g` intro the colours should be checked once:
  a swap shows as blue and orange exchanged.
- The game's own intro movie was not found, so its revision is unknown.

## Sources

Read to understand the format; no code was copied, the decoder is written
from the description above:

- MultimediaWiki, "Bink Container", "Bink Video" and "Bink Audio" pages
  (wiki.multimedia.cx), based on Mike Melanson's description of the format.
- FFmpeg's `libavformat/bink.c`, `libavcodec/bink.c`, `binkdata.h`,
  `binkdsp.c`, `binkaudio.c` (read; FFmpeg 7 was also run as a reference
  decoder for the comparisons above, outside the build).
- The format's constant tables (scan order, run patterns, the 16 prefix
  codes, the base quantiser matrices, the critical band edges) are facts of
  the bitstream; `src/bf_bink_tables.cpp` holds them in this module's own
  layout, the prefix codes as bit strings in reading order.
