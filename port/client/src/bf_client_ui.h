// The client's open UI windows as the control socket reports them (PROTOCOL.md
// section 3, parts `windows` and `controls`), between bf_client_state.cpp,
// which reads the window manager with the client's headers, and bf_main.cpp,
// which writes the JSON. Plain C: no client or Windows type crosses.
#ifndef BF_CLIENT_UI_H
#define BF_CLIENT_UI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bf_client_ui_control {
	const char *id;     // stable within the window: "ok", "id_edit", "slot:0", ...
	const char *kind;   // button, edit, list, item, check, text
	int x, y, w, h;     // logical pixels, window content origin
	const char *text;   // the client's bytes (Windows-1252); a masked edit gives '*' per character
	int focus;          // holds the keyboard
	const char *const *items;   // a list's lines (client bytes), in order; null for other kinds
	int item_count;
} bf_client_ui_control;

typedef struct bf_client_ui_window {
	const void *key;    // the window object, for telling open and close apart
	int wid;            // WID_* value; -1 for a window not made by UIWindowMgr::MakeWindow
	const char *name;   // WID_* name without the prefix, or the class name without "UI", upper case
	int x, y, w, h;
	int focus;          // a control of the window holds the keyboard
	int first_control;  // index into the controls array
	int control_count;
} bf_client_ui_window;

// The windows shown, front to back. Returns their number; *windows and
// *controls stay valid until the next call. 0 before the window manager has
// windows (and in headless runs).
int bf_client_ui_windows(const bf_client_ui_window **windows, const bf_client_ui_control **controls);

#ifdef __cplusplus
}
#endif

#endif
