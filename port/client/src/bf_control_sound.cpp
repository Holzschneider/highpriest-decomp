// The `sound` part and event of the control socket (bf_control_sound.h).
//   sound   {mode, max_voices, voices_started, voices_playing, voices_stolen,
//            voices_rejected, decodes, decode_failures, effects_volume,
//            music_volume, stream {file, status, loops, duration, position,
//            volume, opened, failures}, recent [voice ...]}
//   voice   {serial, file, volume, pan, distance, loop, started, duration}
// The event `sound` carries one voice. The file names and distances are the
// ones PlayWave (src/resource/Wave.cpp) tags each voice with; a voice that
// nothing tagged has file null and distance null.
#include <string.h>
#include <string>

#include "bf_audio.h"
#include "bf_control.h"
#include "bf_text.h"
#include "bf_control_sound.h"

namespace {

const int kRecentInState = 16;
long long g_sent = 0;   // serial of the last voice sent as an event

void Key(bf_control_writer *w, const char *key) { bf_control_write_key(w, key); }
void Int(bf_control_writer *w, const char *key, long long v) { Key(w, key); bf_control_write_int(w, v); }
void Num(bf_control_writer *w, const char *key, double v) { Key(w, key); bf_control_write_number(w, v); }
void Str(bf_control_writer *w, const char *key, const char *v) { Key(w, key); bf_control_write_string(w, v); }

// The game's file names are Korean (code page 949) bytes.
void WriteName(bf_control_writer *w, const char *name)
{
	if (!name || !*name) {
		bf_control_write_null(w);
		return;
	}
	int n = (int)strlen(name);
	std::string utf8((size_t)n * 3 + 1, '\0');
	int m = bf_text_to_utf8(949, name, n, &utf8[0], (int)utf8.size());
	if (m < 0)
		m = bf_text_to_utf8(1252, name, n, &utf8[0], (int)utf8.size());
	bf_control_write_string_n(w, utf8.data(), m > 0 ? (size_t)m : 0);
}

const char *ModeName(int mode)
{
	switch (mode) {
	case BF_AUDIO_MODE_DEVICE: return "device";
	case BF_AUDIO_MODE_OFFLINE: return "offline";
	case BF_AUDIO_MODE_SILENT: return "silent";
	default: return "off";
	}
}

const char *StreamStatus(int status)
{
	switch (status) {
	case BF_AUDIO_STREAM_OPEN: return "open";
	case BF_AUDIO_STREAM_PLAYING: return "playing";
	case BF_AUDIO_STREAM_PAUSED: return "paused";
	case BF_AUDIO_STREAM_DONE: return "done";
	default: return "none";
	}
}

void WriteVoiceFields(bf_control_writer *w, const bf_audio_voice_record &r)
{
	Int(w, "serial", r.serial);
	Key(w, "file");
	WriteName(w, r.label);
	Num(w, "volume", r.volume);
	Num(w, "pan", r.pan);
	Key(w, "distance");
	if (r.distance < 0)
		bf_control_write_null(w);
	else
		bf_control_write_number(w, r.distance);
	Key(w, "loop");
	bf_control_write_bool(w, r.loop);
	Key(w, "started");
	bf_control_write_bool(w, r.started);
	Num(w, "duration", r.duration);
}

void WriteSound(bf_control_writer *w, void *)
{
	bf_audio_counters c;
	bf_audio_get_counters(&c);
	bf_control_write_begin_object(w);
	Str(w, "mode", ModeName(c.mode));
	Int(w, "max_voices", c.max_voices);
	Int(w, "voices_started", c.voices_started);
	Int(w, "voices_playing", c.voices_playing);
	Int(w, "voices_stolen", c.voices_stolen);
	Int(w, "voices_rejected", c.voices_rejected);
	Int(w, "decodes", c.decodes);
	Int(w, "decode_failures", c.decode_failures);
	Num(w, "effects_volume", bf_audio_effects_volume());
	Num(w, "music_volume", bf_audio_music_volume());
	Key(w, "muted");
	bf_control_write_bool(w, bf_audio_muted());
	Key(w, "stream");
	bf_control_write_begin_object(w);
	Key(w, "file");
	WriteName(w, c.stream_file);
	Str(w, "status", StreamStatus(c.stream_status));
	Int(w, "loops", c.stream_loops);
	Num(w, "duration", c.stream_duration);
	Num(w, "position", bf_audio_stream_position());
	Num(w, "volume", bf_audio_stream_volume());
	Int(w, "opened", c.streams_opened);
	Int(w, "failures", c.stream_failures);
	bf_control_write_end_object(w);
	Key(w, "recent");
	bf_audio_voice_record r[kRecentInState];
	int n = bf_audio_recent_voices(0, r, kRecentInState);
	bf_control_write_begin_array(w);
	for (int i = 0; i < n; ++i) {
		bf_control_write_begin_object(w);
		WriteVoiceFields(w, r[i]);
		bf_control_write_end_object(w);
	}
	bf_control_write_end_array(w);
	bf_control_write_end_object(w);
}

}  // namespace

void bf_control_sound_install(void)
{
	bf_control_register_state("sound", WriteSound, nullptr);
	bf_control_declare_event("sound");
	bf_audio_voice_record last[1];
	g_sent = bf_audio_recent_voices(0, last, 1) ? last[0].serial : 0;
}

void bf_control_sound_frame(void)
{
	static bf_audio_voice_record r[BF_AUDIO_RECENT_VOICES];
	int n = bf_audio_recent_voices(g_sent, r, BF_AUDIO_RECENT_VOICES);
	for (int i = 0; i < n; ++i) {
		if (bf_control_writer *w = bf_control_event_begin("sound")) {
			WriteVoiceFields(w, r[i]);
			bf_control_event_end(w);
		}
		g_sent = r[i].serial;
	}
}
