// bifrost-control-echo: a headless stand-in client for the harness
// cross-check. It serves PROTOCOL.md through the real control module and
// keeps a fake login screen behind it, shaped like the harness's mock
// (port/harness/mock_client.py) as far as its protocol test needs: a
// LOGINWND with id_edit, pw_edit, ok and cancel; typing into the focused
// field; Tab, Backspace, Enter; login -> connecting -> charselect with mode
// and window events; resize clamping windows into view; synthetic PNG
// captures; quit. Input comes only through the app queue, as in the client:
// the module injects, this loop drains bf_app_poll_event. No window opens.
//
//   bifrost-control-echo --control <socket path>     (or BF_CONTROL)

#include "bf_app.h"
#include "bf_clock.h"
#include "bf_control.h"
#include "bf_trace.h"

#include <algorithm>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <zlib.h>

namespace {

struct Control {
	std::string id, kind, text;
	int rect[4];
	bool password = false;
};

struct Window {
	std::string name;
	int id;
	int rect[4];
	std::vector<Control> controls;
	int focus = -1;   // index of the focused edit
};

struct Game {
	std::string mode = "start";
	std::string detail = "LSM_IDENTRY";
	int width = 1024, height = 768, scale = 1;
	bool fullscreen = false;
	std::vector<Window> windows;   // front to back
	int modeTimer = 0;
	volatile sig_atomic_t running = 1;
	const char *byeReason = "quit";
	uint64_t frame = 0;
} g;

volatile sig_atomic_t g_signalled = 0;

int LogicalW() { return g.width / g.scale; }
int LogicalH() { return g.height / g.scale; }

const char *Detail(const std::string &mode)
{
	if (mode == "login") return "LSM_IDENTRY";
	if (mode == "connecting") return "LSM_WAITRESP_FROM_ACSVR";
	if (mode == "charselect") return "LSM_SELECTCHAR";
	if (mode == "quitting") return "MT_QUIT";
	return "LSM_START";
}

void SetMode(const std::string &mode, int timer = 0)
{
	std::string previous = g.mode;
	g.mode = mode;
	g.modeTimer = timer;
	BF_TRACE(BFT_MODE, "echo: mode %s -> %s", previous.c_str(), mode.c_str());
	bf_control_writer *w = bf_control_event_begin("mode");
	bf_control_write_key(w, "mode"); bf_control_write_string(w, mode.c_str());
	bf_control_write_key(w, "previous"); bf_control_write_string(w, previous.c_str());
	bf_control_write_key(w, "detail"); bf_control_write_string(w, Detail(mode));
	bf_control_event_end(w);
}

void WindowEvent(const Window &win, const char *action)
{
	bf_control_writer *w = bf_control_event_begin("window");
	bf_control_write_key(w, "action"); bf_control_write_string(w, action);
	bf_control_write_key(w, "wid"); bf_control_write_int(w, win.id);
	bf_control_write_key(w, "name"); bf_control_write_string(w, win.name.c_str());
	bf_control_event_end(w);
}

void OpenWindow(const Window &win)
{
	g.windows.insert(g.windows.begin(), win);
	WindowEvent(win, "open");
}

void CloseWindow(const std::string &name)
{
	for (size_t i = 0; i < g.windows.size(); i++) {
		if (g.windows[i].name != name) continue;
		Window win = g.windows[i];
		g.windows.erase(g.windows.begin() + i);
		WindowEvent(win, "close");
		return;
	}
}

Control Ctl(const char *id, const char *kind, int x, int y, int w, int h, const char *text = "")
{
	Control c;
	c.id = id;
	c.kind = kind;
	c.text = text;
	c.rect[0] = x; c.rect[1] = y; c.rect[2] = w; c.rect[3] = h;
	return c;
}

void OpenLogin()
{
	int cx = LogicalW() / 2, cy = LogicalH() / 2;
	Window w;
	w.name = "LOGINWND";
	w.id = 3;
	int r[4] = {cx - 140, cy - 60, 280, 120};
	memcpy(w.rect, r, sizeof r);
	w.controls.push_back(Ctl("id_edit", "edit", cx - 60, cy - 40, 160, 18));
	w.controls.push_back(Ctl("pw_edit", "edit", cx - 60, cy - 15, 160, 18));
	w.controls.back().password = true;
	w.controls.push_back(Ctl("ok", "button", cx + 20, cy + 30, 50, 20, "OK"));
	w.controls.push_back(Ctl("cancel", "button", cx + 80, cy + 30, 50, 20, "Exit"));
	w.focus = 0;
	OpenWindow(w);
}

void OpenSelectChar()
{
	int cx = LogicalW() / 2, cy = LogicalH() / 2;
	Window w;
	w.name = "SELECTCHARWND";
	w.id = 5;
	int r[4] = {cx - 300, cy - 170, 600, 340};
	memcpy(w.rect, r, sizeof r);
	w.controls.push_back(Ctl("slot:0", "item", cx - 280, cy - 150, 180, 200, ""));
	w.controls.push_back(Ctl("ok", "button", cx + 120, cy + 140, 60, 20, "OK"));
	w.controls.push_back(Ctl("make", "button", cx + 190, cy + 140, 60, 20, "Make"));
	OpenWindow(w);
}

Window *Front() { return g.windows.empty() ? nullptr : &g.windows[0]; }

void FocusNext(Window *w)
{
	std::vector<int> edits;
	for (size_t i = 0; i < w->controls.size(); i++)
		if (w->controls[i].kind == "edit") edits.push_back((int)i);
	if (edits.empty()) return;
	size_t at = 0;
	for (size_t i = 0; i < edits.size(); i++)
		if (edits[i] == w->focus) at = (i + 1) % edits.size();
	w->focus = edits[at];
}

void Submit()
{
	if (g.mode != "login") return;
	CloseWindow("LOGINWND");
	SetMode("connecting", 10);
}

void OnKey(const bf_app_event &e)
{
	if (e.type != BF_APP_EVENT_KEY_DOWN) return;
	Window *w = Front();
	switch (e.key.key) {
	case BF_KEY_Q:
		if (e.mods & BF_APP_MOD_CMD) g.running = 0;
		break;
	case BF_KEY_TAB:
		if (w) FocusNext(w);
		break;
	case BF_KEY_BACKSPACE:
		if (w && w->focus >= 0 && !w->controls[w->focus].text.empty()) {
			std::string &t = w->controls[w->focus].text;
			// remove the last UTF-8 character
			size_t n = t.size() - 1;
			while (n > 0 && ((unsigned char)t[n] & 0xc0) == 0x80) n--;
			t.erase(n);
		}
		break;
	case BF_KEY_RETURN:
	case BF_KEY_KP_ENTER:
		Submit();
		break;
	case BF_KEY_ESCAPE:
		if (g.mode == "charselect") {
			CloseWindow("SELECTCHARWND");
			SetMode("login");
			OpenLogin();
		}
		break;
	default:
		break;
	}
}

void OnText(const bf_app_event &e)
{
	Window *w = Front();
	if (!w || w->focus < 0) return;
	std::string &t = w->controls[w->focus].text;
	if (t.size() < 23) t += e.text.utf8;
}

void OnButton(const bf_app_event &e)
{
	if (e.type != BF_APP_EVENT_BUTTON_UP || e.pointer.button != BF_APP_BUTTON_LEFT) return;
	double x = e.pointer.x_px / g.scale, y = e.pointer.y_px / g.scale;
	for (Window &w : g.windows) {
		if (!(x >= w.rect[0] && x < w.rect[0] + w.rect[2] && y >= w.rect[1] && y < w.rect[1] + w.rect[3])) continue;
		for (size_t i = 0; i < w.controls.size(); i++) {
			const Control &c = w.controls[i];
			if (!(x >= c.rect[0] && x < c.rect[0] + c.rect[2] && y >= c.rect[1] && y < c.rect[1] + c.rect[3])) continue;
			if (c.kind == "edit") w.focus = (int)i;
			else if (w.name == "LOGINWND" && c.id == "ok") Submit();
			else if (w.name == "LOGINWND" && c.id == "cancel") g.running = 0;
			return;
		}
		return;
	}
}

void Tick()
{
	bf_app_event e;
	while (bf_app_poll_event(&e)) {
		switch (e.type) {
		case BF_APP_EVENT_KEY_DOWN:
		case BF_APP_EVENT_KEY_UP: OnKey(e); break;
		case BF_APP_EVENT_TEXT: OnText(e); break;
		case BF_APP_EVENT_BUTTON_DOWN:
		case BF_APP_EVENT_BUTTON_UP: OnButton(e); break;
		default: break;
		}
	}
	if (g.mode == "start") {
		SetMode("login");
		OpenLogin();
	} else if (g.mode == "connecting" && --g.modeTimer <= 0) {
		SetMode("charselect");
		OpenSelectChar();
	}
}

// ------------------------------------------------------------- providers

void WriteRect(bf_control_writer *w, const int *r)
{
	bf_control_write_begin_array(w);
	for (int i = 0; i < 4; i++) bf_control_write_int(w, r[i]);
	bf_control_write_end_array(w);
}

void PartMode(bf_control_writer *w, void *) { bf_control_write_string(w, g.mode.c_str()); }
void PartDetail(bf_control_writer *w, void *) { bf_control_write_string(w, Detail(g.mode)); }
void PartNull(bf_control_writer *w, void *) { bf_control_write_null(w); }

void PartActors(bf_control_writer *w, void *)
{
	bf_control_write_begin_array(w);
	bf_control_write_end_array(w);
}

void PartView(bf_control_writer *w, void *)
{
	bf_control_write_begin_object(w);
	bf_control_write_key(w, "width"); bf_control_write_int(w, g.width);
	bf_control_write_key(w, "height"); bf_control_write_int(w, g.height);
	bf_control_write_key(w, "scale"); bf_control_write_int(w, g.scale);
	bf_control_write_key(w, "logical");
	bf_control_write_begin_object(w);
	bf_control_write_key(w, "w"); bf_control_write_int(w, LogicalW());
	bf_control_write_key(w, "h"); bf_control_write_int(w, LogicalH());
	bf_control_write_end_object(w);
	bf_control_write_key(w, "fullscreen"); bf_control_write_bool(w, g.fullscreen);
	bf_control_write_key(w, "focused"); bf_control_write_bool(w, 1);
	bf_control_write_end_object(w);
}

void PartPointer(bf_control_writer *w, void *)
{
	bf_app_pointer p;
	bf_app_pointer_get(&p);
	bf_control_write_begin_object(w);
	bf_control_write_key(w, "x"); bf_control_write_int(w, (int64_t)lround(p.x_px / g.scale));
	bf_control_write_key(w, "y"); bf_control_write_int(w, (int64_t)lround(p.y_px / g.scale));
	bf_control_write_key(w, "buttons");
	bf_control_write_begin_array(w);
	if (p.buttons & (1u << BF_APP_BUTTON_LEFT)) bf_control_write_string(w, "left");
	if (p.buttons & (1u << BF_APP_BUTTON_MIDDLE)) bf_control_write_string(w, "middle");
	if (p.buttons & (1u << BF_APP_BUTTON_RIGHT)) bf_control_write_string(w, "right");
	bf_control_write_end_array(w);
	bf_control_write_key(w, "cell"); bf_control_write_null(w);
	bf_control_write_end_object(w);
}

void PartWindows(bf_control_writer *w, void *)
{
	bf_control_write_begin_array(w);
	for (size_t i = 0; i < g.windows.size(); i++) {
		const Window &win = g.windows[i];
		bool focused = i == 0;
		bf_control_write_begin_object(w);
		bf_control_write_key(w, "id"); bf_control_write_int(w, win.id);
		bf_control_write_key(w, "name"); bf_control_write_string(w, win.name.c_str());
		bf_control_write_key(w, "rect"); WriteRect(w, win.rect);
		bf_control_write_key(w, "focus"); bf_control_write_bool(w, focused);
		bf_control_write_key(w, "controls");
		bf_control_write_begin_array(w);
		for (size_t k = 0; k < win.controls.size(); k++) {
			const Control &c = win.controls[k];
			bf_control_write_begin_object(w);
			bf_control_write_key(w, "id"); bf_control_write_string(w, c.id.c_str());
			bf_control_write_key(w, "kind"); bf_control_write_string(w, c.kind.c_str());
			bf_control_write_key(w, "rect"); WriteRect(w, c.rect);
			bf_control_write_key(w, "text");
			if (c.password) {
				size_t chars = 0;
				for (unsigned char ch : c.text) chars += (ch & 0xc0) != 0x80;
				bf_control_write_string(w, std::string(chars, '*').c_str());
			} else {
				bf_control_write_string(w, c.text.c_str());
			}
			bf_control_write_key(w, "focus"); bf_control_write_bool(w, focused && (int)k == win.focus);
			bf_control_write_end_object(w);
		}
		bf_control_write_end_array(w);
		bf_control_write_end_object(w);
	}
	bf_control_write_end_array(w);
}

// ----------------------------------------------------------------- hooks

void PutBE(std::string &s, uint32_t v)
{
	s += (char)(v >> 24); s += (char)(v >> 16); s += (char)(v >> 8); s += (char)v;
}

void Chunk(std::string &png, const char *type, const std::string &data)
{
	PutBE(png, (uint32_t)data.size());
	std::string body = std::string(type, 4) + data;
	png += body;
	PutBE(png, (uint32_t)crc32(0, (const Bytef *)body.data(), (uInt)body.size()));
}

// A synthetic frame: a mode colour, the windows as grey boxes (no game artwork).
int Capture(void *, const char *path, bf_control_capture_info *out, char *msg, size_t size)
{
	int W = g.width, H = g.height, k = g.scale;
	uint8_t bg[4] = {40, 60, 120, 255};
	if (g.mode == "connecting") { bg[0] = bg[1] = bg[2] = 40; }
	if (g.mode == "charselect") { bg[0] = 60; bg[1] = 40; bg[2] = 100; }
	std::vector<uint8_t> raw((size_t)(W * 4 + 1) * H);
	for (int y = 0; y < H; y++) {
		uint8_t *row = &raw[(size_t)(W * 4 + 1) * y];
		row[0] = 0;
		for (int x = 0; x < W; x++) memcpy(row + 1 + x * 4, bg, 4);
	}
	for (auto it = g.windows.rbegin(); it != g.windows.rend(); ++it) {
		int x0 = std::max(0, it->rect[0] * k), y0 = std::max(0, it->rect[1] * k);
		int x1 = std::min(W, (it->rect[0] + it->rect[2]) * k), y1 = std::min(H, (it->rect[1] + it->rect[3]) * k);
		uint8_t c[4] = {200, 200, 210, 255};
		for (int y = y0; y < y1; y++)
			for (int x = x0; x < x1; x++) memcpy(&raw[(size_t)(W * 4 + 1) * y + 1 + x * 4], c, 4);
	}
	uLongf zlen = compressBound((uLong)raw.size());
	std::string z(zlen, '\0');
	if (compress2((Bytef *)&z[0], &zlen, raw.data(), (uLong)raw.size(), 1) != Z_OK) {
		snprintf(msg, size, "zlib failed");
		return BF_CONTROL_E_INTERNAL;
	}
	z.resize(zlen);
	std::string png = "\x89PNG\r\n\x1a\n";
	std::string ihdr;
	PutBE(ihdr, (uint32_t)W);
	PutBE(ihdr, (uint32_t)H);
	ihdr += std::string("\x08\x06\x00\x00\x00", 5);
	Chunk(png, "IHDR", ihdr);
	Chunk(png, "IDAT", z);
	Chunk(png, "IEND", "");
	FILE *f = fopen(path, "wb");
	if (!f || fwrite(png.data(), 1, png.size(), f) != png.size()) {
		if (f) fclose(f);
		snprintf(msg, size, "cannot write %s", path);
		return BF_CONTROL_E_IO;
	}
	fclose(f);
	out->width = W;
	out->height = H;
	out->scale = g.scale;
	out->logical_w = LogicalW();
	out->logical_h = LogicalH();
	return BF_CONTROL_E_NONE;
}

int Resize(void *, const bf_control_resize_args *a, char *msg, size_t size)
{
	if (g.fullscreen && (!a->has_fullscreen || a->fullscreen) && (a->has_width || a->has_height)) {
		snprintf(msg, size, "cannot size a fullscreen window");
		return BF_CONTROL_E_BAD_STATE;
	}
	if (a->has_width) g.width = a->width;
	if (a->has_height) g.height = a->height;
	if (a->has_scale) g.scale = a->scale;
	if (a->has_fullscreen) {
		if (a->fullscreen && !g.fullscreen) { g.width = 2560; g.height = 1600; }
		g.fullscreen = a->fullscreen != 0;
	}
	int L[2] = {LogicalW(), LogicalH()};
	for (Window &w : g.windows) {
		w.rect[0] = std::max(0, std::min(w.rect[0], L[0] - w.rect[2]));
		w.rect[1] = std::max(0, std::min(w.rect[1], L[1] - w.rect[3]));
	}
	return BF_CONTROL_E_NONE;
}

void Quit(void *) { g.running = 0; }

void ToPx(void *, double x, double y, float *px, float *py)
{
	*px = (float)(x * g.scale);
	*py = (float)(y * g.scale);
}

void FromPx(void *, float px, float py, double *x, double *y)
{
	*x = px / g.scale;
	*y = py / g.scale;
}

void OnSignal(int) { g_signalled = 1; g.running = 0; }

uint64_t ClockMs() { return bf_clock_us() / 1000; }

} // namespace

int main(int argc, char **argv)
{
	signal(SIGTERM, OnSignal);
	signal(SIGINT, OnSignal);
	bf_trace_init();
	bf_app_set_clock(ClockMs);

	const char *path = bf_control_path_from_args(argc, argv);
	if (!path) {
		fprintf(stderr, "bifrost-control-echo: no --control <path> and no BF_CONTROL\n");
		return 2;
	}
	bf_control_hooks hooks{};
	hooks.quit = Quit;
	hooks.capture = Capture;
	hooks.resize = Resize;
	hooks.logical_to_px = ToPx;
	hooks.px_to_logical = FromPx;
	bf_control_set_hooks(&hooks);
	bf_control_register_state("mode", PartMode, nullptr);
	bf_control_register_state("detail", PartDetail, nullptr);
	bf_control_register_state("view", PartView, nullptr);
	bf_control_register_state("pointer", PartPointer, nullptr);
	bf_control_register_state("map", PartNull, nullptr);
	bf_control_register_state("self", PartNull, nullptr);
	bf_control_register_state("actors", PartActors, nullptr);
	bf_control_register_state("windows", PartWindows, nullptr);
	for (const char *e : {"mode", "map", "chat", "window"}) bf_control_declare_event(e);

	bf_control_config config{path, "echo-0"};
	if (bf_control_open(&config) != BF_CONTROL_OK)
		return 1;

	const uint64_t step = 16667;   // 60 frames a second
	uint64_t next = bf_clock_us();
	while (g.running) {
		Tick();
		bf_control_poll();
		bf_control_frame_done(++g.frame);
		next += step;
		uint64_t now = bf_clock_us();
		if (next > now) bf_clock_sleep_us(next - now);
		else next = now;
	}
	bf_control_close(g_signalled ? "signal" : "quit");
	bf_tracef(BFT_MODE, "echo: exit");
	return 0;
}
