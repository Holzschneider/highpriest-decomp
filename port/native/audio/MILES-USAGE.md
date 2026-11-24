# How the client uses Miles, and what the adapter maps it to

The specification of the Miles adapter (step M7.2a, `port/compat`). It lists every `AIL_*` function the client calls, where, how, and the `bf_audio_*` call (header: [`include/bf_audio.h`](include/bf_audio.h)) that replaces it. Gathered from `src/device/Sound.cpp`, `src/resource/Wave.cpp`, `src/Mode.cpp`, `src/WinMain.cpp`, `src/Session.cpp`, `src/GameMode*.cpp`, `src/LoginMode.cpp` and `src/window/UIFrameWnd2.cpp`. `src/device/Mss.h` declares about 400 more; nothing calls them.

## The path that runs

`WinMain.cpp` reads `SOUNDMODE` from the registry and then overwrites it: `g_registry.m_soundMode = 1`. So `g_soundMode` is 1, the **2D path**, always:

- `InitMSS` opens the digital driver at 22050 Hz, 16 bit, **1 channel** (mode 1 sets `digitalchannels = 1`), and allocates `numSamples` sample handles: 16 by default (`NUMSAMPLETYPE` missing → 2 → 16), 32 or 48 by setting. The 3D branch (`m_soundMode == 2`) never runs.
- `CWave::Play` → `PlayDigital(fileImage, x, y, z, maxDist=250, minDist=40, vFactor)`. The volume is `digital2DVolume` (0..127, default 100), 0 beyond 250 units (the sound is not started at all), `digital2DVolume * 40 / dist` between 40 and 250, then times `vFactor`. **No pan, no rate, no looping** are ever set: Miles mixed to mono.
- `PlayDigital` takes the first handle whose status is `SMP_DONE`; when none is free it traces `Failed PlayDigital` and drops the sound. That is the client's own voice limit. The module's stealing rule never fires as long as the adapter starts the engine with `max_voices` ≥ the number of handles (use 48, `BF_AUDIO_DEFAULT_VOICES`).
- One music stream: `PlayStream("bgm\\01.mp3")` in `LoginMode.cpp`; `PlayStream("bgm\\08.mp3")` in `GameMode.cpp` (`PlayBGM`, `TT_MUSIC_ON_OFF`) and `GameMode2.cpp` (`MM_REFRESH_MUSIC`). Loop count 0 (forever), read from the file, not memory. `PlayStream` returns early when the name equals the current one.

**Map music is a HighPriest leftover:** `CGameMode::PlayBGM` computes `g_session.GetMp3Name(m_rswName)` (the `mp3nametable.txt` lookup, default `bgm\\01.mp3`), stores it in `m_streamFileName`, and then plays the hard-coded `bgm\\08.mp3`. The other two call sites do the same. "Map music changes on map change" (M7 Done when) needs those three calls to play `m_streamFileName.c_str()`. That is a recovery fix in `src/`, not part of the adapter.

## Call by call

Volumes in Miles are 0..127. The adapter maps them linearly to 0..1 (`v / 127.0f`) and back (`lroundf(x * 127)`). Miles 6 applied its own volume curve, so loudness may differ. The owner's play test judges.

### Start-up and shutdown

| Function | Call site, arguments | Maps to |
|---|---|---|
| `AIL_set_redist_directory(".\\")` | `InitMSS` | nothing |
| `AIL_startup()` | `InitMSS`; `MSS_auto_cleanup` (`src/library/mss/cleanup.c`) registers `AIL_shutdown` with `atexit` | nothing; return 1 |
| `AIL_set_preference(DIG_USE_WAVEOUT, …)` | `InitMSS` | nothing |
| `AIL_open_digital_driver(rate, bits, channels, AIL_OPEN_DIGITAL_FORCE_PREFERENCE)` | `InitMSS`; 0 means "Audio System Init Failed" and `g_soundMode = 0` | `bf_audio_start(&{BF_AUDIO_MODE_DEVICE, 48, 0})`. Return a non-null `HDIGDRIVER` token **even when it returns `BF_AUDIO_MODE_SILENT`**: with no device the client must keep its sound mode, so voices and the stream are still counted. Return 0 only for -1. The rate, bits and channels requested are ignored: the engine mixes at the device rate. |
| `AIL_allocate_sample_handle(dig)` | `InitMSS`, `numSamples` times | a new adapter `HSAMPLE` (holds the current `bf_audio_voice`, the file image, the volume); status `SMP_DONE` |
| `AIL_release_sample_handle(h)` | `UnInitMSS` | `bf_audio_voice_stop(h->voice)`, free the handle |
| `AIL_close_digital_driver(dig)` | `UnInitMSS` | `bf_audio_stop()` |
| `AIL_shutdown()` | `UnInitMSS`, and again at exit through `atexit` | nothing (idempotent) |

### Sound effects (2D)

| Function | Call site, arguments | Maps to |
|---|---|---|
| `AIL_sample_status(h)` | `PlayDigital`: looking for `SMP_DONE` | `bf_audio_voice_playing(h->voice) ? SMP_PLAYING : SMP_DONE` |
| `AIL_init_sample(h)` | `PlayDigital` | stop the voice if any; status `SMP_DONE`; volume back to Miles' default (127) |
| `AIL_set_sample_file(h, fileImage, 0)` | `PlayDigital`; `fileImage` is `CWave::m_fileImage` (see below); 0 means failure | look up or create the `bf_audio_sound` for this image (cache below); return 0 when it cannot be decoded |
| `AIL_set_sample_volume(h, volume)` | `PlayDigital`, 0..127 after distance scaling | remember `volume / 127.0f`; if a voice is playing, `bf_audio_voice_set_volume` |
| `AIL_start_sample(h)` | `PlayDigital` | `h->voice = bf_audio_play(sound, volume, 0, 0)` |
| `AIL_end_sample(h)` | `EndDigitalSound` (mode changes: `LoginMode.cpp`, `GameMode.cpp`) | `bf_audio_voice_stop(h->voice)` |

`PauseDigitalSound` is declared in `Sound.h` and never defined or called.

### Decoding (`resource/Wave.cpp`, `CWave::Load`)

`CWave::Load` reads the whole file, then:

```
type = AIL_file_type(image, length)
PCM_WAV        -> m_fileImage = image                (freed later with delete)
ADPCM_WAV      -> AIL_WAV_info(image, &info); AIL_decompress_ADPCM(&info, &m_fileImage, 0); delete image
MPEG_L3_AUDIO  -> AIL_decompress_ASI(image, length, fName, &m_fileImage, 0, 0); delete image
anything else  -> m_fileImage = image                (Miles was handed it as it is)
```

and `CWave::Reset` frees a decompressed image with `AIL_mem_free_lock`, any other with `delete`.

| Function | Maps to |
|---|---|
| `AIL_file_type(data, size)` | `bf_audio_file_kind(data, size)`. The `BF_AUDIO_KIND_*` values equal `AILFILETYPE_*` (0 unknown, 1 PCM WAV, 2 ADPCM WAV, 3 other WAV, 13 MP3). This is the only call that sees the image's length, so the adapter records `image → length` here for the sound cache. |
| `AIL_WAV_info(data, &info)` | `bf_audio_query` on the image (length: the recorded one, else the RIFF size + 8), then fill `AILSOUNDINFO`: `format` = format tag, `data_ptr`/`initial_ptr` = start of the `data` chunk, `data_len`, `rate`, `bits`, `channels`, `samples` = frames, `block_size` = block align (from the fmt chunk). Record `data_ptr → image` so that the next call can find the whole file. |
| `AIL_decompress_ADPCM(&info, &out, &size)` | find the image from `info->data_ptr`, `bf_audio_decode_alloc` it, write a PCM WAV image (44-byte RIFF header + the 16-bit samples) into one `malloc` block, and return it in `*out` (and its length in `*size` when non-null). Return 1 on success, 0 on failure. |
| `AIL_decompress_ASI(data, size, name, &out, &outsize, callback)` | the same with `bf_audio_decode_alloc(data, size, …)`; `name` (the extension hint) is not needed. |
| `AIL_mem_free_lock(p)` | `free(p)`, and drop `p` from the sound cache |

**The sound cache.** `AIL_set_sample_file` gets only a pointer. Decoding and rate conversion for every play would be wasteful, so the adapter keeps a map `image pointer → (length, bf_audio_sound *)`:

- `AIL_file_type` sets the length for a pointer. Because `CWave::Load` calls it for every load, an image newly allocated at the address of a freed one replaces the stale entry: destroy the old sound there.
- `AIL_decompress_*` set the entry for the image they return. `AIL_mem_free_lock` removes it.
- `AIL_set_sample_file` creates the sound on first use with `bf_audio_sound_create_from_memory(image, length)`. For a pointer with no recorded length, it uses the RIFF size + 8.

The game's data, measured with `BF_AUDIO_TEST_ALL=1 bifrost-audio-test`:

- **`data.grf`:** 2,743 `.wav` entries, 1,826 of them DES-flagged and not readable without the GRF decryption. The 917 readable ones are all PCM WAV (tag 1): 876 × 16-bit mono 22050 Hz, 27 × 16-bit mono 48000, 3 × 16-bit stereo 48000, 2 × 16-bit stereo 22050, 2 each × 16-bit mono 11025 and 44100, and 1 each × 16-bit mono 8000, 11127 and 32000, and 8-bit mono 22050. One entry, `data\wav\worm_tail_damage.wav`, has a zlib stream cut short in the archive; the bytes that inflate decode.
- **`rdata.grf`:** 280 entries, none DES-flagged, 279 × 16-bit mono 22050 and 1 × 16-bit mono 44100.

So among the readable files, the ADPCM and MP3 branches of `CWave::Load` are never taken. They work anyway: IMA ADPCM is tested with a generated file; MS ADPCM is untested. The DES-flagged files are unknown until the GRF reader decrypts them.

### Music stream

| Function | Call site, arguments | Maps to |
|---|---|---|
| `AIL_open_stream(dig, "bgm\\NN.mp3", 0)` | `PlayStream`; then `sAssert(hDigitalStream != NULL)` | translate the path (backslashes, relative to the game folder, case-insensitive: the folder is `BGM/`, the client says `bgm\`), `bf_audio_stream_open(path)`; return a fresh non-null `HSTREAM` token, or 0 when it fails (a missing file is counted in `stream_failures`; the client asserts, which is non-fatal) |
| `AIL_set_stream_volume(s, v)` | `PlayStream`, `SetStreamVolume` (UI slider, `Session.cpp`, the fade thread) | `bf_audio_stream_set_volume(v / 127.0f)` |
| `AIL_set_stream_loop_count(s, 0)` | `PlayStream` | `bf_audio_stream_set_loop_count(0)` (0 = forever in both) |
| `AIL_start_stream(s)` | `PlayStream` | `bf_audio_stream_start()` |
| `AIL_pause_stream(s, onoff)` | `PauseStream` (the fade thread) | `bf_audio_stream_pause(onoff)` |
| `AIL_stream_volume(s)` | `PauseStream`, result unused; **called with `s == 0`** when no stream is open | `lroundf(bf_audio_stream_volume() * 127)`; accept 0 |
| `AIL_close_stream(s)` | `CloseStream`, `InitMSS` | `bf_audio_stream_close()` |

Every stream function must accept a null or stale token and then do nothing: the fade thread can call one with the handle of a stream that the main thread has just closed. Only the latest token from `AIL_open_stream` acts on the module.

### The 3D provider functions: report "none"

Never reached with `g_soundMode == 1`; implement them so that the 3D branch would fall back to 2D if it ever ran:

| Function | Returns |
|---|---|
| `AIL_enumerate_3D_providers(&next, &dest, &name)` | 0: no provider, so `provider_default` stays 0 and `Play3DDigital` calls `PlayDigital` |
| `AIL_open_3D_provider` | `M3D_INTERNAL_ERR` (non-zero = failure) |
| `AIL_3D_speaker_type`, `AIL_3D_room_type` | -1 (unsupported) |
| `AIL_open_3D_listener`, `AIL_allocate_3D_sample_handle` | 0 |
| `AIL_3D_sample_status` | `SMP_DONE` |
| `AIL_set_3D_sample_file` | 0 |
| `AIL_set_3D_speaker_type`, `AIL_set_3D_room_type`, `AIL_set_3D_position`, `AIL_set_3D_orientation` (also used for the listener in `SetListenerOrient`, with a null listener), `AIL_set_3D_sample_volume`, `AIL_set_3D_sample_distances`, `AIL_set_3D_sample_effects_level`, `AIL_start_3D_sample`, `AIL_end_3D_sample`, `AIL_release_3D_sample_handle`, `AIL_close_3D_listener`, `AIL_close_3D_provider` | nothing |

That is 46 functions: 8 for start-up, shutdown and sample handles, 6 for effects, 5 for decoding (with `AIL_mem_free_lock`), 7 for the stream, 20 for 3D. The technical plan's count of 45 leaves one out.

## Threads and the fade (for M7.3)

`FadeBgm` in `src/Mode.cpp` runs on its own thread, started with `_beginthreadex` by `CMode::ProcessFadeIn` (fade in to `g_session.m_BgmVolume`) and `CMode::RunFadeOut` (fade out, then pause). It steps the volume by 1 every 10 ms. Variables shared between it and the main thread, or between two fade threads:

| Variable | Written by | Read by |
|---|---|---|
| `g_mssVar.streamVolume` (`Sound.cpp`) | `SetStreamVolume`: fade thread, and main thread (option window slider in `UIFrameWnd2.cpp`, `/bgm` volume in `GameMode.cpp`, `Session.cpp` settings load and reset) | `GetStreamVolume` (fade thread), `PlayStream` (main) |
| `g_mssVar.hDigitalStream` | `PlayStream`, `CloseStream` (main) | `SetStreamVolume`, `PauseStream` (fade thread): a stream can be closed and replaced under the fade thread's feet |
| `g_mssVar.streamFileName` | `PlayStream`, `CloseStream` (main) | `PlayStream` (main) only |
| `FadeCnt` (`Mode.cpp`, plain `int`) | `++`/`--` by every fade thread | the fade threads: a newer fade spins (`Sleep(5)`) until it is alone, an older one sees `FadeCnt != 1` and quits. Two threads, no atomics |
| `g_session.m_BgmVolume` | main (UI, settings) | `FadeBgm` at its start (`m_BgmVolume == 0` → no fade) |
| `dwFadeStart` | main only | main only (the screen fade, not the thread) |

The module serialises its own calls (one internal lock; any thread may call it), and a stale stream token is a no-op in the adapter. So M7.3 needs the lock in the client for `streamVolume` and `hDigitalStream`: around the bodies of `SetStreamVolume`, `GetStreamVolume`, `PauseStream`, `PlayStream` and `CloseStream`. `FadeCnt` needs to become atomic, or to be guarded by the same lock.

A fade-out ends with `SetStreamVolume(1)` and a pause. The next `PlayStream` opens the new file at volume 1, and the fade-in that follows ramps it from 0 to `m_BgmVolume`. That is the original behaviour; the module needs nothing special for it.

## The M7 check

"The check is the sound module's own counters of started voices and the stream's file name": `bf_audio_get_counters` gives `voices_started`, `voices_rejected`, `stream_file` (the path passed to `bf_audio_stream_open`), `stream_status` and `stream_loops`. The same counts also go to `bf_count` as `audio.voices_started`, `audio.streams_opened` and so on, so `bf_trace_dump_counters` shows them. The control socket needs a command that returns the struct for the scripted test.
