// The control socket's `sound` part and `sound` event (PROTOCOL.md sections
// 3 and 5), read from the sound module's counters and its ring of recent
// voices (port/native/audio, bf_audio.h). Port code (bf_compat_flags, in
// bf_port_main); bf_main.cpp calls it.
#ifndef BF_CONTROL_SOUND_H
#define BF_CONTROL_SOUND_H

// Registers the part and declares the event; before bf_control_open.
void bf_control_sound_install(void);

// Once per frame, before bf_control_poll: one `sound` event per call of
// bf_audio_play since the last frame.
void bf_control_sound_frame(void);

#endif
