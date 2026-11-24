# miles: the Miles adapter (stage A)

This adapter defines the 46 `AIL_*` functions of `mss32.dll` that the client calls, on top of [`port/native/audio`](../../native/audio/README.md). The callers are `src/device/Sound.cpp` and `src/resource/Wave.cpp`. The specification is [`port/native/audio/MILES-USAGE.md`](../../native/audio/MILES-USAGE.md). The adapter gets its declarations from the client's `src/device/Mss.h`, so a definition whose signature differs from RAD's fails to compile. M11 deletes this directory and moves the two callers onto `bf_audio.h`.

## Build and test

```
cmake --preset debug && cmake --build --preset debug --target bf_compat_miles bifrost-compat-miles-test
build/cmake/debug/compat/miles/bifrost-compat-miles-test
```

The test program replays the client's call sequences through the `AIL_*` names: `InitMSS` (2D), `CWave::Load` and `CWave::Reset`, `PlayDigital`, `EndDigitalSound`, `PlayStream`, `PauseStream`, `CloseStream` and `UnInitMSS`. Before `AIL_open_digital_driver` it starts the engine in OFFLINE mode, and the driver adopts that engine. The test can then render the mix and measure it: level, volume ratios, centring, completion, stream loops, and silence while the stream is paused. Its WAV, IMA ADPCM and 8-bit images are generated. It also writes a short WAV into `$BF_TEST_OUT`, or the current directory when that is unset, and plays it as a looping stream. When the game folder holds `BGM/01.mp3` and `BGM/08.mp3`, the test opens them by the client's names (`bgm\01.mp3`) and runs `AIL_decompress_ASI` on the first 256 KB of `01.mp3`. Otherwise it prints `skipped`. Last, it opens the real device the way the client does and plays a 0.3 s tone, so sound may be heard. The ctest label is `sound`, so `fastcheck.sh` builds the test but does not run it.

## The names

Volumes are 0..127 in Miles and 0..1 in the module (`v / 127`).

| Function | Status | Maps to |
|---|---|---|
| `AIL_set_redist_directory` | no-op | keeps a copy of the string and returns it |
| `AIL_startup` | no-op | returns 1 |
| `AIL_set_preference` | no-op | stores the value, returns the previous one |
| `AIL_open_digital_driver` | implemented | `bf_audio_start(DEVICE, 48 voices)`. A running engine is adopted. A SILENT fallback still returns a driver. Returns 0 only when the engine cannot start. Rate, bits and channels are ignored. |
| `AIL_close_digital_driver` | implemented | stops every voice, closes the stream, invalidates all handles, `bf_audio_stop` |
| `AIL_shutdown` | implemented | closes the driver if it is still open; safe to call twice (atexit) |
| `AIL_allocate_sample_handle` | implemented | an adapter slot (64 at most); the token is `generation << 8 \| slot + 1` |
| `AIL_release_sample_handle` | implemented | `bf_audio_voice_stop`; frees the slot; the old token goes stale |
| `AIL_sample_status` | implemented | `bf_audio_voice_playing` → `SMP_PLAYING`, otherwise `SMP_DONE`; a stale token gives `SMP_FREE` |
| `AIL_init_sample` | implemented | stops the voice, forgets the image, sets the volume back to 127 |
| `AIL_set_sample_file` | implemented | finds or creates the image's `bf_audio_sound` in the sound cache; returns 0 when it cannot be decoded |
| `AIL_set_sample_volume` | implemented | stores the volume; `bf_audio_voice_set_volume` if the sample is playing |
| `AIL_start_sample` | implemented | `bf_audio_play(sound, volume, pan 0, no loop)` |
| `AIL_end_sample` | implemented | `bf_audio_voice_stop` |
| `AIL_file_type` | implemented | `bf_audio_file_kind`; also records image → length, which drops a cached sound left at that address |
| `AIL_WAV_info` | implemented | walks the RIFF chunks for `format`, `rate`, `channels`, `bits`, `block_size`, `data_ptr`/`initial_ptr`, `data_len`; `samples` comes from `bf_audio_query`; records `data_ptr` → image |
| `AIL_decompress_ADPCM` | implemented | finds the image from `info->data_ptr`, then `bf_audio_decode_alloc`, then one `malloc` block: a 44-byte PCM WAV header and 16-bit samples. 1, or 0 with `*out = NULL`. |
| `AIL_decompress_ASI` | implemented | the same on `(data, size)`; the extension hint is not used |
| `AIL_mem_free_lock` | implemented | drops the image from the sound cache, then `free` |
| `AIL_open_stream` | implemented | `bf_file_resolve` (backslashes, any letter case, relative to the game folder: `bgm\` finds `BGM/`), then `bf_audio_stream_open`. Returns a new serial token, or 0. A missing file is still handed to the module so that it counts in `stream_failures`. |
| `AIL_close_stream` | implemented | `bf_audio_stream_close` |
| `AIL_start_stream` | implemented | `bf_audio_stream_start` |
| `AIL_pause_stream` | implemented | `bf_audio_stream_pause` |
| `AIL_set_stream_volume` | implemented | `bf_audio_stream_set_volume(v / 127)` |
| `AIL_set_stream_loop_count` | implemented | `bf_audio_stream_set_loop_count` (0 = forever in both) |
| `AIL_stream_volume` | implemented | `lroundf(bf_audio_stream_volume() * 127)`, for any token, including 0 |
| `AIL_enumerate_3D_providers` | 3D: none | 0, so `provider_default` stays 0 and `Play3DDigital` falls back to `PlayDigital` |
| `AIL_open_3D_provider` | 3D: none | `M3D_INTERNAL_ERR` |
| `AIL_3D_speaker_type`, `AIL_3D_room_type` | 3D: none | -1 |
| `AIL_open_3D_listener`, `AIL_allocate_3D_sample_handle` | 3D: none | 0 |
| `AIL_3D_sample_status` | 3D: none | `SMP_DONE` |
| `AIL_set_3D_sample_file` | 3D: none | 0 |
| `AIL_set_3D_speaker_type`, `AIL_set_3D_room_type`, `AIL_set_3D_position`, `AIL_set_3D_orientation`, `AIL_set_3D_sample_volume`, `AIL_set_3D_sample_distances`, `AIL_set_3D_sample_effects_level`, `AIL_start_3D_sample`, `AIL_end_3D_sample`, `AIL_release_3D_sample_handle`, `AIL_close_3D_listener`, `AIL_close_3D_provider` | 3D: none | nothing |

No name is a `BF_STUB`. A 3D function is never reached, because `WinMain` forces `m_soundMode = 1`.

`MSS_auto_cleanup` is not defined. `Mss.h` calls it from `AIL_startup()` only under `_MSC_VER`, and `src/library/mss/cleanup.c` is not one of the game's units.

## Behaviour worth knowing

- **Stale handles:** every stream function checks its token against the stream that is open now, under the adapter's lock. A null, closed or replaced token does nothing and is counted as `miles.stale_stream_calls`. The fade thread may therefore race the main thread's `CloseStream` and `PlayStream`. The test runs 40 reopenings under such a thread. The client's own shared variables still need M7.3's lock.
- **The sound cache** follows MILES-USAGE.md. An image is decoded on its first play and kept with it. `AIL_file_type`, `AIL_decompress_*` and `AIL_mem_free_lock` invalidate the cached sound at an address. An image that cannot be decoded is remembered, so that it is not decoded again on every play, and is counted as `miles.undecodable_images`. PCM images that `CWave::Reset` frees with `delete` are not seen. Their entries stay until a new image at the same address passes through `AIL_file_type`, which every `CWave::Load` calls.
- **Adopting a running engine:** `AIL_open_digital_driver` does not start the engine when one is already running. That lets a test choose OFFLINE or SILENT. `AIL_close_digital_driver` stops the engine whoever started it.

## Open

- **Volume ramp at the start of a sound:** the module reuses `AVAudioPlayerNode`s, and the mixer ramps a reused player from its previous volume to the new one over about 15 ms. Measured offline at volume 32 after a sound at 127, in 5 ms windows: peaks 0.31, 0.20, 0.11, then 0.089. A quiet distant sound that follows a loud one therefore starts loud for about 15 ms. The fix belongs in `port/native/audio`, for example by setting a player's volume to 0 when its voice ends. The test measures levels after 20 ms.
- **Loudness:** a mono sound at volume 127 peaks at 0.707 of its amplitude on each side, the module's centred pan. Miles mixed mono to a mono device at full level. The owner's play test judges.
- **Module fix in this package:** `bf_audio_voice_playing` called right after `bf_audio_play`, before the first offline render, threw an Objective-C exception (`playerTimeForNodeTime:` on a node time that is neither sample- nor host-valid). `port/native/audio/src/bf_audio_engine.mm` now checks the node time first, in the voice and in the stream code. `bifrost-audio-test` still passes.
- **MS ADPCM** is untested here too; the IMA path is tested.

## For port/README.md

> `port/compat/miles`: the 46 `AIL_*` names of Miles on `port/native/audio` (step M7.2a). `bifrost-compat-miles-test` replays `Sound.cpp`'s and `Wave.cpp`'s call sequences on an offline engine and measures the samples. It plays the game's `BGM/01.mp3` when the game folder is there and ends with one tone on the device. Label `sound`, so not part of the fast check's test run.
