// The client's entry point on macOS (M3.2, M3.7): main() builds WinMain's
// command line from argv, does the start-up the adapters ask for (their
// READMEs, "What the client must call at start-up"), opens the control socket
// when asked, and calls the client's own WinMain.
//
// Port flags, taken out of the command line WinMain sees:
//   --headless            start-up only: WinMain stops before the window and
//                         the first frame and returns 0 (bf_port_headless)
//   --control <path>      the control socket (or --control=<path>, or the
//                         environment variable BF_CONTROL)
//   --server <host[:port]>, --account <name>, --password <text>,
//   --character <name or slot>
//                         launch arguments (or --flag=value, or BF_SERVER,
//                         BF_ACCOUNT, BF_PASSWORD, BF_CHARACTER): the login
//                         mode logs in, takes the first character server and
//                         enters with the character, without input, once
//                         (bf_port_launch; option LaunchArguments)
// Every other argument is passed on, joined by spaces, as the Windows
// command line was (WinMain parses it with strstr).
//
// The graphics device is created by bf_port_start_graphics, which WinMain
// calls right after InitApp has made the window (bf_input_window_create
// creates the window, its Metal layer and the view): one line in WinMain.
//
// The control socket is served from bf_port_frame, which CheckSystemMessage
// calls at the top of every frame of both modes: the frame presented last is
// reported first (bf_control_frame_done; a capture reads the target, which
// still holds it), then the requests are carried out (bf_control_poll; the
// events injected earlier have been handled during the previous frame).
//
// Compiled with bf_port_flags (the prefix and bf_types.h, C++17,
// the C library's own file names) into the library bf_port_main.
#include "bf_types.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <vector>

#include "bf_app.h"
#include "bf_clock.h"
#include "bf_render.h"
#include "bf_coverage.h"
#include "bf_control.h"
#include "bf_file.h"
#include "bf_gfx.h"
#include "bf_input.h"
#include "bf_port.h"
#include "bf_prefs.h"
#include "bf_settings.h"
#include "bf_text.h"
#include "bf_trace.h"
#include "bf_view.h"
#include "bf_client_ui.h"
#include "bf_control_coverage.h"
#include "bf_control_game.h"
#include "bf_control_movie.h"
#include "bf_control_sound.h"

extern "C" const char *bf_port_mode(const char **detail);   // bf_client_state.cpp
extern "C" int bf_client_view_changed(int w, int h);         // WinMain.cpp, the client's resize path; 0: no renderer yet
extern "C" int bf_client_modal_frame(void);                  // WinMain.cpp
extern "C" int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);   // WinMain.cpp, C linkage as common.h declares it
extern "C" void *objc_autoreleasePoolPush(void);            // libobjc: the frame's autorelease pool
extern "C" void objc_autoreleasePoolPop(void *pool);

namespace {

bool g_headless = false;
bf_gfx_device *g_gfx = nullptr;
long long g_frames_reported = 0;
std::string g_mode;   // the last `mode` reported as an event

uint64_t ClockMs()
{
	return bf_clock_us() / 1000;
}

// ---- control hooks

void Quit(void *)
{
	// What Cmd+Q does: WM_CLOSE to the window, whose procedure sets g_sysQuit.
	if (bf_input_window())
		bf_input_post(WM_CLOSE, 0, 0);
	else
		exit(0);
}

int Capture(void *, const char *path, bf_control_capture_info *out, char *message, size_t message_size)
{
	if (!g_gfx) {
		snprintf(message, message_size, "no graphics device (headless or not started yet)");
		return BF_CONTROL_E_UNSUPPORTED;
	}
	if (bf_gfx_capture_png(g_gfx, path) != BF_GFX_OK) {
		snprintf(message, message_size, "bf_gfx_capture_png failed for %s", path);
		return BF_CONTROL_E_IO;
	}
	int lw = 0, lh = 0, scale = 1, nw = 0, nh = 0;
	bf_gfx_get_size(g_gfx, &lw, &lh, &scale, &nw, &nh);
	out->width = nw;
	out->height = nh;
	out->scale = scale;
	out->logical_w = lw;
	out->logical_h = lh;
	return BF_CONTROL_E_NONE;
}

void LogicalToPx(void *, double x, double y, float *x_px, float *y_px)
{
	bf_view_metrics m;
	bf_view_get(&m);
	double px = x, py = y;
	if (m.serial)
		bf_view_logical_to_px(&m, x, y, &px, &py);
	*x_px = (float)px;
	*y_px = (float)py;
}

void PxToLogical(void *, float x_px, float y_px, double *x, double *y)
{
	bf_view_metrics m;
	bf_view_get(&m);
	*x = x_px;
	*y = y_px;
	if (m.serial)
		bf_view_px_to_logical(&m, x_px, y_px, x, y);
}

// `resize`: what a resize by the user does. The scale first (it can raise
// the window's minimum size), then the content size in pixels, then one sync
// so that the client follows at once; the reply waits for the next frame.
int g_fullscreen_target = -1;   // while a fullscreen change asked for is under way

int Resize(void *, const bf_control_resize_args *a, char *message, size_t message_size)
{
	if (!g_gfx) {
		snprintf(message, message_size, "no window (headless or not started yet)");
		return BF_CONTROL_E_UNSUPPORTED;
	}
	bf_view_metrics m;
	bf_view_get(&m);
	int fullscreen = bf_app_window_is_fullscreen();
	bool to_fullscreen = a->has_fullscreen ? a->fullscreen != 0 : fullscreen != 0;
	if ((a->has_width || a->has_height) && to_fullscreen) {
		snprintf(message, message_size, "width and height cannot be set in fullscreen");
		return BF_CONTROL_E_BAD_STATE;
	}
	if (a->has_fullscreen && (a->fullscreen != 0) != (fullscreen != 0)) {
		g_fullscreen_target = a->fullscreen != 0;
		bf_view_toggle_fullscreen();
	}
	if (a->has_scale && bf_view_set_scale_setting(a->scale) < 0) {   // 1: changed
		snprintf(message, message_size, "bf_view_set_scale_setting(%d) failed", a->scale);
		return BF_CONTROL_E_INTERNAL;
	}
	if (a->has_width || a->has_height) {
		bf_view_get(&m);
		double backing = m.backing_scale > 0 ? m.backing_scale : 1.0;
		int w = a->has_width ? a->width : m.drawable_w;
		int h = a->has_height ? a->height : m.drawable_h;
		bf_app_window_set_content_size(w / backing, h / backing);
	}
	bf_view_sync();
	bf_view_get(&m);
	BF_TRACE(BFT_GFX, "control: resize to %dx%d px, UI scale %d (setting %d), logical %dx%d", m.drawable_w, m.drawable_h,
	         m.ui_scale, m.scale_setting, m.logical_w, m.logical_h);
	return BF_CONTROL_E_NONE;
}

int ResizeBusy(void *)
{
	if (g_fullscreen_target < 0)
		return 0;
	if ((bf_app_window_is_fullscreen() != 0) == (g_fullscreen_target != 0)) {
		g_fullscreen_target = -1;
		return 0;
	}
	return 1;
}

void WriteView(bf_control_writer *w, void *)
{
	bf_view_metrics m;
	bf_view_get(&m);
	bf_control_write_begin_object(w);
	bf_control_write_key(w, "width");
	bf_control_write_int(w, m.drawable_w);
	bf_control_write_key(w, "height");
	bf_control_write_int(w, m.drawable_h);
	bf_control_write_key(w, "scale");
	bf_control_write_int(w, m.ui_scale);
	bf_control_write_key(w, "logical");
	bf_control_write_begin_object(w);
	bf_control_write_key(w, "w");
	bf_control_write_int(w, m.logical_w);
	bf_control_write_key(w, "h");
	bf_control_write_int(w, m.logical_h);
	bf_control_write_end_object(w);
	bf_control_write_key(w, "fullscreen");
	bf_control_write_bool(w, m.fullscreen);
	bf_control_write_key(w, "focused");
	bf_control_write_bool(w, bf_app_window_is_focused());
	bf_control_write_end_object(w);
}

// ---- the client's resize path (M4.3a)

void ViewChanged(const bf_view_metrics *now, const bf_view_metrics *, void *)
{
	bf_client_view_changed(now->logical_w, now->logical_h);
}

int g_modal_frames;          // frames run from inside AppKit's live-resize loop
void *g_frame_pool;          // the autorelease pool of the frame in progress

void ModalFrame(void *)
{
	++g_modal_frames;
	bf_client_modal_frame();
	--g_modal_frames;
}

// Installed at the first frame, once the client's device, renderer and
// window manager exist; the view may have changed since the device was made.
void InstallResizePath()
{
	static bool installed = false;
	if (installed || !g_gfx)
		return;
	bf_view_metrics m;
	bf_view_get(&m);
	if (!bf_client_view_changed(m.logical_w, m.logical_h))
		return;   // no renderer yet: the start-up's own frames
	installed = true;
	bf_view_set_listener(ViewChanged, nullptr);
	bf_view_set_modal_frame(ModalFrame, nullptr);
}

void WriteMode(bf_control_writer *w, void *)
{
	bf_control_write_string(w, bf_port_mode(nullptr));
}

void WriteDetail(bf_control_writer *w, void *)
{
	const char *detail = nullptr;
	bf_port_mode(&detail);
	bf_control_write_string(w, detail);
}

// ---- open windows (PROTOCOL.md section 3, `windows`; section 5, `window`)

// The client's bytes are Windows-1252 (spec section 5).
void WriteText(bf_control_writer *w, const char *text)
{
	bf_control_game_write_text(w, text);
}

void WriteRect(bf_control_writer *w, int x, int y, int cx, int cy)
{
	bf_control_write_begin_array(w);
	bf_control_write_int(w, x);
	bf_control_write_int(w, y);
	bf_control_write_int(w, cx);
	bf_control_write_int(w, cy);
	bf_control_write_end_array(w);
}

void WriteWindows(bf_control_writer *w, void *)
{
	const bf_client_ui_window *wins = nullptr;
	const bf_client_ui_control *ctls = nullptr;
	int n = bf_client_ui_windows(&wins, &ctls);
	bf_control_write_begin_array(w);
	for (int i = 0; i < n; ++i) {
		const bf_client_ui_window &win = wins[i];
		bf_control_write_begin_object(w);
		bf_control_write_key(w, "id");
		bf_control_write_int(w, win.wid);
		bf_control_write_key(w, "name");
		bf_control_write_string(w, win.name);
		bf_control_write_key(w, "rect");
		WriteRect(w, win.x, win.y, win.w, win.h);
		bf_control_write_key(w, "focus");
		bf_control_write_bool(w, win.focus);
		bf_control_write_key(w, "controls");
		bf_control_write_begin_array(w);
		for (int j = 0; j < win.control_count; ++j) {
			const bf_client_ui_control &c = ctls[win.first_control + j];
			bf_control_write_begin_object(w);
			bf_control_write_key(w, "id");
			bf_control_write_string(w, c.id);
			bf_control_write_key(w, "kind");
			bf_control_write_string(w, c.kind);
			bf_control_write_key(w, "rect");
			WriteRect(w, c.x, c.y, c.w, c.h);
			bf_control_write_key(w, "text");
			WriteText(w, c.text);
			bf_control_write_key(w, "focus");
			bf_control_write_bool(w, c.focus);
			if (strcmp(c.kind, "list") == 0) {
				bf_control_write_key(w, "items");
				bf_control_write_begin_array(w);
				for (int k = 0; k < c.item_count; ++k)
					WriteText(w, c.items[k]);
				bf_control_write_end_array(w);
			}
			bf_control_write_end_object(w);
		}
		bf_control_write_end_array(w);
		bf_control_write_end_object(w);
	}
	bf_control_write_end_array(w);
}

struct OpenWindow {
	const void *key;
	int wid;
	std::string name;
};
std::vector<OpenWindow> g_open;   // as last reported

void WindowEvent(const char *action, const OpenWindow &o)
{
	BF_TRACE(BFT_UI, "control: window %s %s (%d)", action, o.name.c_str(), o.wid);
	if (bf_control_writer *w = bf_control_event_begin("window")) {
		bf_control_write_key(w, "action");
		bf_control_write_string(w, action);
		bf_control_write_key(w, "wid");
		bf_control_write_int(w, o.wid);
		bf_control_write_key(w, "name");
		bf_control_write_string(w, o.name.c_str());
		bf_control_event_end(w);
	}
}

// Once per frame: the windows closed since the last frame, then those opened.
void ReportWindowChanges()
{
	const bf_client_ui_window *wins = nullptr;
	const bf_client_ui_control *ctls = nullptr;
	int n = bf_client_ui_windows(&wins, &ctls);
	std::vector<OpenWindow> now;
	for (int i = 0; i < n; ++i)
		now.push_back(OpenWindow{wins[i].key, wins[i].wid, wins[i].name});
	auto same = [](const OpenWindow &a, const OpenWindow &b) { return a.key == b.key && a.name == b.name; };
	for (const OpenWindow &o : g_open) {
		bool still = false;
		for (const OpenWindow &k : now)
			still = still || same(o, k);
		if (!still)
			WindowEvent("close", o);
	}
	for (const OpenWindow &k : now) {
		bool before = false;
		for (const OpenWindow &o : g_open)
			before = before || same(o, k);
		if (!before)
			WindowEvent("open", k);
	}
	g_open.swap(now);
}

void ReportModeChange()
{
	const char *detail = nullptr;
	const char *mode = bf_port_mode(&detail);
	if (g_mode == mode)
		return;
	std::string previous = g_mode.empty() ? "start" : g_mode;
	g_mode = mode;
	BF_TRACE(BFT_MODE, "control: mode %s (%s)", mode, detail);
	if (bf_control_writer *w = bf_control_event_begin("mode")) {
		bf_control_write_key(w, "mode");
		bf_control_write_string(w, mode);
		bf_control_write_key(w, "previous");
		bf_control_write_string(w, previous.c_str());
		bf_control_write_key(w, "detail");
		bf_control_write_string(w, detail);
		bf_control_event_end(w);
	}
}

void OpenControl(const char *path)
{
	bf_control_hooks hooks = {};
	hooks.quit = Quit;
	hooks.capture = Capture;
	hooks.logical_to_px = LogicalToPx;
	hooks.px_to_logical = PxToLogical;
	hooks.resize = Resize;
	hooks.resize_busy = ResizeBusy;
	bf_control_set_hooks(&hooks);
	bf_control_register_state("mode", WriteMode, nullptr);
	bf_control_register_state("detail", WriteDetail, nullptr);
	bf_control_register_state("view", WriteView, nullptr);
	bf_control_register_state("windows", WriteWindows, nullptr);
	bf_control_declare_event("mode");
	bf_control_declare_event("window");
	bf_control_game_install();   // map, self, actors, pointer; events map, chat
	bf_control_sound_install();  // sound; event sound
	bf_control_movie_install();  // movie; event movie
	bf_control_coverage_install();  // coverage (M9.5)
	bf_control_config config = {path, BF_APP_NAME " " __DATE__};
	if (bf_control_open(&config) != BF_CONTROL_OK)
		bf_tracef(BFT_ERR, "control: cannot open %s; running without the socket", path);
}

// The end of the process, from exit() (main's return, or any exit(0) of the
// client and the adapters), registered first in main so that it runs before
// the destructors of every static object made before main: it saves the
// settings, writes the counters and ends with _exit. The client's static
// destructors (CFileMgr closing its archives, CSkinMgr removing its temporary
// folder) would otherwise run after the port's own statics, in no defined
// order across units, and call into the file and handle tables after those
// are gone. They release only what the system reclaims anyway.
int g_exit_status = 0;

void EndProcess()
{
	bf_control_close("quit");
	bf_control_coverage_write();   // BF_COVERAGE (M9.5)
	bf_settings_save(bf_settings_shared());
	if (bf_trace_mask & ~(unsigned)(BFT_ERR | BFT_STUB))   // any category asked for: the counts too
		bf_trace_dump_counters();
	fflush(NULL);
	_exit(g_exit_status);
}

// ---- launch arguments (spec section 2, option LaunchArguments, default on)

struct LaunchArg {
	const char *flag;   // --server, --account, ...
	const char *env;    // BF_SERVER, BF_ACCOUNT, ...
	const char *key;    // what bf_port_launch is asked for
	std::string value;
	bool given;
};

LaunchArg g_launch[] = {
	{"--server", "BF_SERVER", "server", "", false},          // host[:port] of the account server
	{"--account", "BF_ACCOUNT", "account", "", false},
	{"--password", "BF_PASSWORD", "password", "", false},
	{"--character", "BF_CHARACTER", "character", "", false},  // a name, or a slot number
};

// The value of a flag that takes one (--flag value or --flag=value), or NULL.
LaunchArg *LaunchFlag(const char *arg, const char **inline_value)
{
	for (LaunchArg &a : g_launch) {
		size_t n = strlen(a.flag);
		if (strncmp(arg, a.flag, n) == 0 && (arg[n] == 0 || arg[n] == '=')) {
			*inline_value = arg[n] == '=' ? arg + n + 1 : nullptr;
			return &a;
		}
	}
	return nullptr;
}

void LaunchFromEnvironment()
{
	for (LaunchArg &a : g_launch) {
		const char *v = getenv(a.env);
		if (!a.given && v && *v) {
			a.value = v;
			a.given = true;
		}
	}
}

bool IsPortFlag(const char *arg)
{
	const char *unused;
	return strcmp(arg, "--headless") == 0 || strcmp(arg, "--control") == 0 || strncmp(arg, "--control=", 10) == 0
	       || strncmp(arg, "-psn_", 5) == 0   // Finder's process serial number
	       || LaunchFlag(arg, &unused);
}

}  // namespace

// ---- what the client calls (bf_port.h)

extern "C" int bf_port_headless(void)
{
	return g_headless;
}

extern "C" void bf_port_missing(unsigned category, const char *counter, const char *format, ...)
{
	bf_count(counter, 1);
	// A table lookup that fell back to a placeholder (table.missing.<table>)
	// belongs to the coverage report of M9.5, traced or not.
	static const char kTable[] = "table.missing.";
	bool table = strncmp(counter, kTable, sizeof kTable - 1) == 0 && counter[sizeof kTable - 1];
	if (!table && !(bf_trace_mask & category))
		return;
	char text[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof text, format, args);
	va_end(args);
	if (table)
		bf_coverage_placeholder(counter + sizeof kTable - 1, text);
	if (bf_trace_mask & category)
		bf_tracef(category, "%s: %s", counter, text);
}

extern "C" int bf_port_option(const char *option)
{
	return bf_settings_option(bf_settings_shared(), option);
}

// A launch argument (`server`, `account`, `password`, `character`) given on
// the command line or in the environment, or NULL: not given, or the option
// LaunchArguments is off. Called by the login mode (LoginMode.cpp).
extern "C" const char *bf_port_launch(const char *key)
{
	if (!bf_settings_option(bf_settings_shared(), BF_OPT_LAUNCH_ARGUMENTS))
		return nullptr;
	for (const LaunchArg &a : g_launch)
		if (a.given && strcmp(a.key, key) == 0)
			return a.value.c_str();
	return nullptr;
}

extern "C" int bf_port_start_graphics(void)
{
	if (g_gfx)
		return 1;
	void *layer = bf_app_window_metal_layer();
	bf_view_metrics m;
	bf_view_get(&m);
	if (!layer || !m.serial) {
		bf_tracef(BFT_ERR, "gfx: no window layer or view to draw into (layer %p)", layer);
		return 0;
	}
	bf_gfx_desc desc = {layer, m.logical_w, m.logical_h, m.ui_scale};
	g_gfx = bf_gfx_create(&desc);
	if (!g_gfx) {
		bf_tracef(BFT_ERR, "gfx: bf_gfx_create(%dx%d, scale %d) failed", m.logical_w, m.logical_h, m.ui_scale);
		return 0;
	}
	bf_view_attach_gfx(g_gfx);
	bf_render_set_device(g_gfx);
	BF_TRACE(BFT_GFX, "gfx: device on the window layer, logical %dx%d, UI scale %d, drawable %dx%d", m.logical_w,
	         m.logical_h, m.ui_scale, m.drawable_w, m.drawable_h);
	return 1;
}

extern "C" void bf_port_frame(void)
{
	// One autorelease pool per frame. The client's loop never returns to an
	// event loop that would drain one, so whatever Cocoa autoreleased during
	// a frame (outside the modules' own pools) piled up for the whole
	// session (the soak's NSMutableArray growth). Not inside the live-resize
	// loop, whose pools AppKit owns.
	if (!g_modal_frames) {
		if (g_frame_pool)
			objc_autoreleasePoolPop(g_frame_pool);
		g_frame_pool = objc_autoreleasePoolPush();
	}
	bool control = bf_control_is_open();
	if (g_gfx && control) {
		long long frames = 0;
		bf_gfx_get_stats(g_gfx, nullptr, nullptr, nullptr, &frames);
		if (frames > g_frames_reported) {
			g_frames_reported = frames;
			bf_control_frame_done((uint64_t)frames);   // before the sync: a capture reads the frame just drawn
		}
	}
	if (g_gfx) {
		// The view follows the window at the start of every frame; its
		// listener resizes the client (InstallResizePath).
		InstallResizePath();
		bf_view_sync();
	}
	if (!control)
		return;
	ReportModeChange();
	ReportWindowChanges();
	bf_control_game_frame();
	bf_control_sound_frame();
	bf_control_movie_frame();
	bf_control_poll();
}

// ---- the entry point

// A start that ends before WinMain: EndProcess (atexit) exits with
// g_exit_status, so the status is set here, and the line stands in for
// "exit: WinMain returned N", which the harness reads for a bundle launch.
static int RefuseStart(int status, const char *why)
{
	BF_TRACE(BFT_MODE, "exit: start refused, status %d (%s)", status, why);
	g_exit_status = status;
	return status;
}

int main(int argc, char **argv)
{
	bf_trace_init();   // BF_TRACE reads the mask before any first use would
	atexit(EndProcess);
	std::string command_line;
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--headless") == 0)
			g_headless = true;
		if (IsPortFlag(argv[i])) {
			const char *value = nullptr;
			if (LaunchArg *a = LaunchFlag(argv[i], &value)) {
				if (!value && i + 1 < argc)
					value = argv[++i];
				a->value = value ? value : "";
				a->given = value != nullptr;
			} else if (strcmp(argv[i], "--control") == 0 && i + 1 < argc)
				++i;
			continue;
		}
		if (!command_line.empty())
			command_line += ' ';
		command_line += argv[i];
	}
	BF_TRACE(BFT_MODE, "start: %s%s, command line \"%s\", game folder %s", argv[0], g_headless ? " --headless" : "",
	         command_line.c_str(), bf_file_location(BF_LOCATION_GAME));
	LaunchFromEnvironment();
	for (const LaunchArg &a : g_launch)
		if (a.given)
			BF_TRACE(BFT_MODE, "start: launch argument %s %s", a.key,
			         strcmp(a.key, "password") == 0 ? "(given)" : a.value.c_str());

	if (g_headless) {
		// No chooser to ask: a game folder whose path is over the limit
		// stops the start here (bf_file traced the refusal), and no other
		// folder is used in its place.
		char why[256];
		if (!bf_file_game_folder_fits(bf_file_location(BF_LOCATION_GAME), why, sizeof why)) {
			bf_tracef(BFT_ERR, "start: game folder not usable: %s", why);
			return RefuseStart(1, "game folder");
		}
	} else {
		// The application first: the game-folder chooser and the window need NSApp.
		bf_app_config app = {};
		app.single_instance = 1;
		int r = bf_app_init(&app);
		if (r == BF_APP_ALREADY_RUNNING)
			return 0;
		if (r != BF_APP_OK) {
			bf_tracef(BFT_ERR, "start: bf_app_init failed (%d)", r);
			return RefuseStart(1, "bf_app_init");
		}
		bf_app_set_clock(ClockMs);
		// The chooser refuses a folder over the limit (and says why), so the
		// longest it returns fits.
		char folder[BF_FILE_FOLDER_MAX + 1];
		switch (bf_prefs_choose_game_folder(folder, sizeof folder)) {
		case 1:
			break;
		case 0:
			return RefuseStart(0, "the game-folder chooser was cancelled");
		default:
			return RefuseStart(1, "no usable game folder");   // and nobody to ask; traced
		}
		bf_prefs_install_menu_item();
		// Display/UIScale, as the settings window writes it; absent: auto (0),
		// the view's rule.
		bf_input_set_scale_setting(bf_settings_int(bf_settings_shared(), BF_PREFS_DISPLAY, BF_PREFS_UI_SCALE, 0));
	}

	const char *control = bf_control_path_from_args(argc, argv);
	if (control)
		OpenControl(control);

	std::string cmd = command_line;   // WinMain may write into it
	// The instance handle is only a token here: nothing asks Windows about
	// it. 1 is SW_SHOWNORMAL, which InitApp does not read under BF_PORT.
	static HINSTANCE__ module;
	int result = WinMain(&module, NULL, &cmd[0], 1);

	BF_TRACE(BFT_MODE, "exit: WinMain returned %d", result);
	bf_control_close("quit");
	if (g_gfx) {
		bf_render_set_device(NULL);
		bf_view_attach_gfx(NULL);
		bf_gfx_destroy(g_gfx);
		g_gfx = nullptr;
	}
	g_exit_status = result;
	return result;   // exit(), then EndProcess
}
