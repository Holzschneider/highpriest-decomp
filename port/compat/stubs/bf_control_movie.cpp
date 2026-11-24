// The `movie` part and event of the control socket (bf_control_movie.h).
//   movie   {playing, paused, file, width, height, frames, frame, fps, tracks,
//            sound, voice, sound_seconds, opened, closed, ended, last_frame}
//   event   {what: "start" | "end", file, frames, frame, ended}
// frame is the frame last decoded (1-based, 0 before the first); ended and
// last_frame describe the last movie closed (ended: shown to its last frame).
#include "bf_compat_bink.h"
#include "bf_control.h"
#include "bf_control_movie.h"

namespace {

long long g_sent_opened = 0, g_sent_closed = 0;

void Key(bf_control_writer *w, const char *key) { bf_control_write_key(w, key); }
void Int(bf_control_writer *w, const char *key, long long v) { Key(w, key); bf_control_write_int(w, v); }
void Num(bf_control_writer *w, const char *key, double v) { Key(w, key); bf_control_write_number(w, v); }
void Bool(bf_control_writer *w, const char *key, int v) { Key(w, key); bf_control_write_bool(w, v != 0); }

void File(bf_control_writer *w, const bf_compat_bink_state &s)
{
	Key(w, "file");
	if (s.file[0])
		bf_control_write_string(w, s.file);
	else
		bf_control_write_null(w);
}

void WriteMovie(bf_control_writer *w, void *)
{
	bf_compat_bink_state s;
	bf_compat_bink_get_state(&s);
	bf_control_write_begin_object(w);
	Bool(w, "playing", s.playing);
	Bool(w, "paused", s.paused);
	File(w, s);
	Int(w, "width", s.width);
	Int(w, "height", s.height);
	Int(w, "frames", s.frames);
	Int(w, "frame", s.frame);
	Num(w, "fps", s.fps);
	Int(w, "tracks", s.tracks);
	Bool(w, "sound", s.sound);
	Int(w, "voice", s.voice);
	Num(w, "sound_seconds", s.sound_seconds);
	Int(w, "opened", s.opened);
	Int(w, "closed", s.closed);
	Bool(w, "ended", s.ended);
	Int(w, "last_frame", s.last_frame);
	bf_control_write_end_object(w);
}

void Event(const char *what, const bf_compat_bink_state &s)
{
	if (bf_control_writer *w = bf_control_event_begin("movie")) {
		Key(w, "what");
		bf_control_write_string(w, what);
		File(w, s);
		Int(w, "frames", s.frames);
		Int(w, "frame", s.frame);
		Bool(w, "ended", s.ended);
		bf_control_event_end(w);
	}
}

}  // namespace

void bf_control_movie_install(void)
{
	bf_control_register_state("movie", WriteMovie, nullptr);
	bf_control_declare_event("movie");
	bf_compat_bink_state s;
	bf_compat_bink_get_state(&s);
	g_sent_opened = s.opened;
	g_sent_closed = s.closed;
}

void bf_control_movie_frame(void)
{
	bf_compat_bink_state s;
	bf_compat_bink_get_state(&s);
	if (s.opened > g_sent_opened) {
		g_sent_opened = s.opened;
		Event("start", s);
	}
	if (s.closed > g_sent_closed) {
		g_sent_closed = s.closed;
		Event("end", s);
	}
}
