// The control socket's game parts and events (bf_control_game.h), written
// from bf_client_game.h. The parts:
//   map      {name, width, height} on a loaded map, else null
//   self     the own character (PROTOCOL.md section 3) on a loaded map, else null
//   actors   the actors the client holds, other than self; [] outside a map
//   pointer  {x, y, buttons, cell}: the pointer as the app module has it,
//            in logical pixels, and the map cell the client's picking finds
// The events: `map` when game mode enters a map, `chat` per chat line.
#include <windows.h>

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <string>

#include "bf_app.h"
#include "bf_control.h"
#include "bf_text.h"
#include "bf_trace.h"
#include "bf_view.h"
#include "bf_client_game.h"
#include "bf_control_game.h"

namespace {

void WriteIntKey(bf_control_writer *w, const char *key, int64_t v)
{
	bf_control_write_key(w, key);
	bf_control_write_int(w, v);
}

void WriteCell(bf_control_writer *w, int x, int y)
{
	bf_control_write_begin_object(w);
	WriteIntKey(w, "x", x);
	WriteIntKey(w, "y", y);
	bf_control_write_end_object(w);
}

void WriteMap(bf_control_writer *w, void *)
{
	bf_client_game_map m;
	if (!bf_client_game_map_get(&m)) {
		bf_control_write_null(w);
		return;
	}
	bf_control_write_begin_object(w);
	bf_control_write_key(w, "name");
	bf_control_game_write_text(w, m.name);
	WriteIntKey(w, "width", m.width);
	WriteIntKey(w, "height", m.height);
	bf_control_write_end_object(w);
}

void WriteSelf(bf_control_writer *w, void *)
{
	bf_client_game_self s;
	if (!bf_client_game_self_get(&s)) {
		bf_control_write_null(w);
		return;
	}
	bf_control_write_begin_object(w);
	WriteIntKey(w, "id", s.id);
	WriteIntKey(w, "char_id", s.char_id);
	bf_control_write_key(w, "name");
	bf_control_game_write_text(w, s.name);
	WriteIntKey(w, "job", s.job);
	WriteIntKey(w, "base_level", s.base_level);
	WriteIntKey(w, "job_level", s.job_level);
	WriteIntKey(w, "x", s.x);
	WriteIntKey(w, "y", s.y);
	WriteIntKey(w, "dir", s.dir);
	bf_control_write_key(w, "moving");
	bf_control_write_bool(w, s.moving);
	bf_control_write_key(w, "dest");
	if (s.moving && s.dest_x >= 0)
		WriteCell(w, s.dest_x, s.dest_y);
	else
		bf_control_write_null(w);
	WriteIntKey(w, "hp", s.hp);
	WriteIntKey(w, "max_hp", s.max_hp);
	WriteIntKey(w, "sp", s.sp);
	WriteIntKey(w, "max_sp", s.max_sp);
	WriteIntKey(w, "zeny", s.zeny);
	WriteIntKey(w, "weight", s.weight);
	WriteIntKey(w, "max_weight", s.max_weight);
	bf_control_write_end_object(w);
}

void WriteActors(bf_control_writer *w, void *)
{
	const bf_client_game_actor *a = nullptr;
	int n = bf_client_game_actors(&a);
	bf_control_write_begin_array(w);
	for (int i = 0; i < n; ++i) {
		bf_control_write_begin_object(w);
		WriteIntKey(w, "id", a[i].id);
		bf_control_write_key(w, "type");
		bf_control_write_string(w, a[i].type);
		bf_control_write_key(w, "name");
		if (a[i].name)
			bf_control_game_write_text(w, a[i].name);
		else
			bf_control_write_string(w, "?name");
		bf_control_write_key(w, "job");
		if (a[i].has_job)
			bf_control_write_int(w, a[i].job);
		else
			bf_control_write_null(w);
		WriteIntKey(w, "x", a[i].x);
		WriteIntKey(w, "y", a[i].y);
		bf_control_write_key(w, "screen");
		if (a[i].on_screen)
			WriteCell(w, a[i].screen_x, a[i].screen_y);
		else
			bf_control_write_null(w);
		bf_control_write_key(w, "emotion");
		if (a[i].emotion >= 0)
			bf_control_write_int(w, a[i].emotion);
		else
			bf_control_write_null(w);
		bf_control_write_end_object(w);
	}
	bf_control_write_end_array(w);
}

void WritePointer(bf_control_writer *w, void *)
{
	bf_app_pointer p;
	bf_app_pointer_get(&p);
	double x = p.x_px, y = p.y_px;
	bf_view_metrics m;
	bf_view_get(&m);
	if (m.serial)
		bf_view_px_to_logical(&m, p.x_px, p.y_px, &x, &y);
	bf_control_write_begin_object(w);
	WriteIntKey(w, "x", (int64_t)lround(x));
	WriteIntKey(w, "y", (int64_t)lround(y));
	bf_control_write_key(w, "buttons");
	bf_control_write_begin_array(w);
	static const char *const kButtons[] = {"left", "right", "middle"};
	static const int kBits[] = {BF_APP_BUTTON_LEFT, BF_APP_BUTTON_RIGHT, BF_APP_BUTTON_MIDDLE};
	for (int i = 0; i < 3; ++i)
		if (p.buttons & (1u << kBits[i]))
			bf_control_write_string(w, kButtons[i]);
	bf_control_write_end_array(w);
	bf_control_write_key(w, "cell");
	int cx, cy;
	if (bf_client_game_pointer_cell(&cx, &cy))
		WriteCell(w, cx, cy);
	else
		bf_control_write_null(w);
	bf_control_write_end_object(w);
}

bool g_inMap = false;   // a `map` event has been sent for the map game mode is on

void ReportMap()
{
	bf_client_game_map m;
	bf_client_game_self s;
	if (!bf_client_game_map_get(&m) || !bf_client_game_self_get(&s)) {
		g_inMap = false;
		return;
	}
	if (g_inMap)
		return;
	g_inMap = true;
	BF_TRACE(BFT_MODE, "control: map %s %d %d (%dx%d)", m.name, s.x, s.y, m.width, m.height);
	if (bf_control_writer *w = bf_control_event_begin("map")) {
		bf_control_write_key(w, "map");
		bf_control_game_write_text(w, m.name);
		WriteIntKey(w, "x", s.x);
		WriteIntKey(w, "y", s.y);
		bf_control_event_end(w);
	}
}

void ReportChat()
{
	const bf_client_game_chat *c = nullptr;
	int n = bf_client_game_take_chat(&c);
	for (int i = 0; i < n; ++i) {
		if (bf_control_writer *w = bf_control_event_begin("chat")) {
			bf_control_write_key(w, "channel");
			bf_control_write_string(w, c[i].channel);
			bf_control_write_key(w, "from");
			bf_control_game_write_text(w, c[i].from);
			bf_control_write_key(w, "text");
			bf_control_game_write_text(w, c[i].text);
			if (c[i].window) {
				bf_control_write_key(w, "window");
				bf_control_game_write_text(w, c[i].window);
			}
			bf_control_event_end(w);
		}
	}
}

}  // namespace

void bf_control_game_write_text(bf_control_writer *w, const char *text)
{
	if (!text) {
		bf_control_write_null(w);
		return;
	}
	int n = (int)strlen(text);
	std::string utf8((size_t)n * 3 + 1, '\0');
	int m = bf_text_to_utf8(1252, text, n, &utf8[0], (int)utf8.size());
	bf_control_write_string_n(w, utf8.data(), m > 0 ? (size_t)m : 0);
}

void bf_control_game_install(void)
{
	bf_control_register_state("map", WriteMap, nullptr);
	bf_control_register_state("self", WriteSelf, nullptr);
	bf_control_register_state("actors", WriteActors, nullptr);
	bf_control_register_state("pointer", WritePointer, nullptr);
	bf_control_declare_event("map");
	bf_control_declare_event("chat");
}

void bf_control_game_frame(void)
{
	ReportMap();
	ReportChat();
}
