# audio

Sound for the port. It provides AVAudioEngine voices for effects, one looping music stream read from a file, and WAV and MP3 decoding through AudioToolbox. The contract is [`include/bf_audio.h`](include/bf_audio.h). [`MILES-USAGE.md`](MILES-USAGE.md) lists every Miles call the client makes and what the adapter maps it to, and names the variables the fade thread shares with the main thread.

## Build and test

```
cmake -S port/native/audio -B build/native/audio -G Ninja
cmake --build build/native/audio
build/native/audio/bifrost-audio-test
```

The test program needs no game data. With `BF_CLIENT_DIR` (default `~/Games/<app name>`) it also plays `BGM/01.mp3` for two seconds, with a fade out and back in from a second thread, and decodes 20 `.wav` entries from `data.grf`, skipping the DES-flagged ones. `BF_AUDIO_TEST_ALL=1` decodes every readable `.wav` in `data.grf` and `rdata.grf` and prints the format statistics. `BF_TRACE=snd` shows the module's trace lines. Sound plays on the default output device; the checks that need it say `skipped` when there is none.

What it checks:
- **Decoding:** 8-bit mono 11025 Hz, 16-bit stereo 22050 Hz, 16-bit mono 44100 Hz and IMA ADPCM, sample for sample. Also broken RIFF sizes, the built-in fallback reader, and junk data.
- **Offline rendering:** playback, stop, pan, volume (also while playing), master volumes, looping, completion, voice stealing and refusal, a rate-converted voice starting on time and at the right pitch, and the stream's loop count, pass count, volume, pause and close. These checks look at the samples rendered, not at the speaker.
- **Start volume:** on a single reused voice, a sound's first 5 ms are at the volume and pan given to `bf_audio_play`, within 1%, after a louder, quieter, differently panned, stereo or mono sound, one stopped or ended by itself, and one whose volume was changed while playing; the same for the effects volume. A change while playing ramps over 5 ms.
- **Silent mode.**
- **The device:** five voices at once, then the music stream.

## Design notes

- **Engine graph:** a pool of voice slots, each an `AVAudioSourceNode` (stereo, engine rate) that renders its sound itself, feeding an effects mixer. One `AVAudioPlayerNode` for the stream feeds a music mixer. Both mixers feed the main mixer.
- **Voices apply their own volume and pan.** The mixer (AUMultiChannelMixer) smooths every change of an input's volume and pan over about 15 ms, and of its output volume over about 95 ms, and the smoothing only advances while that input renders. A reused player therefore started at its previous sound's volume and pan, whatever was set before scheduling (also tried: setting the parameter through the audio unit's schedule block with no ramp, through its parameter tree, and rendering silence between stop and play; all smooth the same way). So each voice's mixer input stays at volume 1 and pan 0, the effects mixer's output at 1, and the voice's render block multiplies the samples: volume times the effects volume, with the mixer's own pan laws (equal power for mono: cos and sin of (pan + 1) π/4; balance for stereo). A sound starts at its gains from its first sample; a later change of volume, pan or effects volume ramps linearly over 5 ms. Cost, offline at -O2: 6 ms per 10 s of audio with 48 voices playing, 4 ms idle (the player graph took 7 and 6).
- **Handing a sound to the render thread:** `bf_audio_play` puts a small record (the buffer, its channel pointers, length, loop, generation, volume, pan) into the voice's pending slot with one atomic exchange; the render block takes it at its next frame. Finished records go onto a lock-free list that the calling thread empties and frees, so the render block neither locks nor allocates nor releases Objective-C objects. A non-looping sound marks its generation finished when its last frame is rendered.
- **Everything is wired once, at the engine rate, before the engine starts.** A player wired or re-wired at another rate while the engine runs starts late, and the delay grows with the time the engine has run. Measured offline: 0.4 s after 5 s. So each sound is converted to the engine rate on its first play (`AVAudioConverter`, kept with the sound and counted in `conversions`). The stream player converts the file's rate itself.
- **Threads:** one recursive lock serialises all calls, so the client's fade thread may call the stream functions. The stream's completion handlers only touch atomics, or take the lock with `try_lock` and otherwise defer to a private serial queue. Stopping a node while holding the lock therefore cannot deadlock.
- **Stream passes** are counted from the player's own sample clock and from the completions, whichever is further. That keeps the count exact offline, where completions arrive late on another thread.
- **No output device:** `bf_audio_start` falls back to `BF_AUDIO_MODE_SILENT` and counts a device failure. A device change restarts the engine and restarts the music from the top.
- **SDK deprecations:** the macOS 27 SDK deprecates `connect:to:…format:` and `play` in favour of `…error:` variants that exist only on macOS 27. The module targets macOS 14 and keeps the old calls, which do not warn at that target.

## Untested or open

- **MS ADPCM:** no encoder is at hand. IMA ADPCM is tested, and no readable game file uses either. The 1,826 DES-flagged `.wav` files in `data.grf` have not been looked at.
- **Mono BGM:** the music files are all stereo: 153 at 44100 Hz, 16 at 32000 and 16 at 22050. A mono file on the stereo stream player works in an offline experiment, not in the test.
- **Device changes:** the restart after `AVAudioEngineConfigurationChangeNotification` has not been exercised. On this Mac the device cannot be changed unattended.
- **Volume curve:** volumes are linear 0..1. Miles 6 had its own curve for 0..127.
