/*
 * bf_input.h: the client's window, event pump, keyboard, mouse, cursor and
 * clipboard on port/native/app and port/native/view (M11.7). It replaces the
 * input adapter of stage A (port/compat/input: user32, DirectInput 7, imm32)
 * and keeps its rules unchanged; port/client/src/README.md ("bf_input")
 * has them in full.
 *
 * The client's window procedure stays its own (WinMain.cpp's WindowProc).
 * bf_input_peek turns the app module's events into the messages it handled
 * on Windows, one event at a time, so that the key and pointer state it
 * reads while handling a message is the state of that message;
 * bf_input_dispatch calls the procedure with one. The message ids and the
 * virtual keys are the Windows numbers (WM_*, VK_*), which the client keeps
 * as its own (port/client/include/bf_types.h).
 *
 * Include after the client's Windows types (common.h; bf_types.h once the
 * Windows headers are gone, M11.11). C linkage; the main thread only, the
 * one that called bf_app_init.
 */
#ifndef BF_INPUT_H
#define BF_INPUT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------- window */

/* The client's window procedure. */
typedef LRESULT (*bf_input_proc)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

/*
 * The UI scale setting (0 = auto, 1..3) the window's view starts with
 * (bf_view_init); after the window exists it goes to
 * bf_view_set_scale_setting. Default 0. Called by main before WinMain.
 */
void bf_input_set_scale_setting(int setting);

/*
 * Creates the one window: content width x height points (the client's
 * logical size), title in the client's code page (CP1252), the view started
 * at the scale setting, the system pointer shown. Sends WM_NCCREATE and
 * WM_CREATE to proc, as CreateWindowEx did (lParam 0). Returns the window,
 * an opaque token the client keeps in g_hMainWnd, or NULL (traced).
 * bf_app_init must have run (main does it).
 */
HWND bf_input_window_create(int width, int height, const char *title, bf_input_proc proc);

/* Sends WM_DESTROY and WM_NCDESTROY, drops what is pending, stops the view
   and closes the window. */
void bf_input_window_destroy(void);

/* The window, or NULL. */
HWND bf_input_window(void);

/* The client area's logical size: what GetClientRect gave (left and top 0;
   "screen" coordinates are client coordinates). 640 x 480 without a window. */
void bf_input_client_size(int *width, int *height);

/* ---------------------------------------------------------------- messages */

typedef struct bf_input_msg {
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
} bf_input_msg;

/*
 * PeekMessage(PM_REMOVE) and TranslateMessage in one: the next message, 1;
 * none, 0. Pending messages first; else one event of the app queue is
 * translated; else bf_app_pump(0) runs once and the queue is looked at
 * again. A WM_KEYDOWN taken here puts the WM_CHAR that TranslateMessage
 * typed for Backspace, Tab, Return and Escape (8, 9, 13, 27; not with Ctrl)
 * right behind it.
 */
int bf_input_peek(bf_input_msg *msg);

/* DispatchMessage: calls the window procedure with msg; 0 without a window. */
LRESULT bf_input_dispatch(const bf_input_msg *msg);

/* PostMessage to the window: appended to the pending messages. */
void bf_input_post(UINT message, WPARAM wparam, LPARAM lparam);

/* DefWindowProc for the messages the client passes on: TRUE for
   WM_NCCREATE and WM_NCACTIVATE, 0 for the rest. */
LRESULT bf_input_default_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

/* ---------------------------------------------------------------- keys */

/*
 * GetKeyState: 0x8000 while the virtual key is held (as of the message
 * being handled), plus 1 for the latch of VK_CAPITAL (Caps Lock) and always
 * for VK_NUMLOCK (a Mac keypad types digits). VK_SHIFT, VK_CONTROL and
 * VK_MENU are either side; VK_CONTROL also reads down while Cmd+X or Cmd+C
 * is held (CLanguage cuts and copies on Ctrl+X, Ctrl+C); VK_LWIN/VK_RWIN
 * are Cmd; VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1/2 the pointer's
 * buttons.
 */
int bf_input_key_state(int vk);

/* GetAsyncKeyState: 0x8000 or 0, never bit 0 (the client tests the whole
   value: `if (GetAsyncKeyState(VK_SHIFT))`). */
int bf_input_async_key_state(int vk);

/*
 * The virtual key, set-1 scan code and extended flag a portable key
 * (BF_KEY_*, bf_app.h) has in WM_KEYDOWN; 0 when it has none. scan and
 * extended may be NULL.
 */
unsigned bf_input_vk_for_key(int key, unsigned *scan, int *extended);

/*
 * Alt (Option) shortcuts of the game: the virtual keys for which Alt+key
 * does something in the client (UIWindowMgr::ProcessPushButton). Text that
 * a key-down with Option (and neither Ctrl nor Cmd) types is dropped when
 * its key is one of these, and typed otherwise, but for the exception of
 * bf_input_option_types.
 */
int bf_input_is_alt_shortcut(unsigned vk);

/*
 * Whether a key-down (bf_app_event KEY_DOWN) with Option types the
 * layout's character instead of being Alt: Option held without Ctrl and
 * Cmd, a text field holds the keyboard (bf_input_client_text_focus), and
 * the keyboard layout (bf_app_key_layout_text, with Shift as held) gives
 * one printable ASCII character for it that the layout types nowhere
 * without Option ("@" for German Option+L, "[" for Option+5, "\" for
 * Option+Shift+7). Such a key-down is WM_KEYDOWN without the Alt bit, Alt
 * reads up while the key is held, and its text is typed. The layout is the
 * one in effect for AppKit's key-downs; an injected key-down (the control
 * socket) follows a layout only when one is named (bf_app_key_layout_use),
 * so that scripted tests do not depend on the Mac's layout.
 */
struct bf_app_event;
int bf_input_option_types(const struct bf_app_event *key_down);

/*
 * The app module's text filter (bf_app_set_text_filter), installed by
 * bf_input_window_create: 0 for a key-down whose text the rules above drop
 * because it is an Option shortcut, so that the text system leaves no dead
 * key pending (Option+E on a US layout would accent the next letter).
 */
int bf_input_text_filter(const struct bf_app_event *key_down, void *context);

/*
 * Provided by the client (port/client/src/bf_input_focus.cpp; the test
 * program has its own): non-zero while a text field of the client holds the
 * keyboard (UIWindowMgr::GetFocusEdit). Main thread.
 */
int bf_input_client_text_focus(void);

/* ---------------------------------------------------------------- mouse */

/* What CMouse::ReadState reads, with DIMOUSESTATE's field names. */
typedef struct bf_input_mouse_state {
	int lX, lY;                    /* movement in logical pixels */
	int lZ;                        /* wheel, 120 per line, positive away from the user */
	unsigned char rgbButtons[4];   /* left, right, middle, X1: 0x80 held */
} bf_input_mouse_state;

/* IDirectInputDevice's Acquire (1) and Unacquire (0). Acquiring forgets the
   movement and the wheel accumulated before. */
void bf_input_mouse_acquire(int acquire);

/*
 * IDirectInputDevice::GetDeviceState for the client's mouse at (x, y), the
 * logical position CMouse holds: lX/lY bring it to the logical pixel under
 * the pointer (clamped to the logical screen); while the right button is
 * held they are the device's own movement instead, fractions carried over,
 * so that the camera turns on past the window's edge. 0 (S_OK), or
 * BF_INPUT_NOT_ACQUIRED (DIERR_NOTACQUIRED's value, so that FAILED() holds)
 * when not acquired; the movement is drained either way.
 */
#define BF_INPUT_NOT_ACQUIRED ((int)0x8007000C)
int bf_input_mouse_read(int x, int y, bf_input_mouse_state *out);

/* GetCursorPos: the logical position of the pointer, not clamped. */
void bf_input_cursor_pos(int *x, int *y);

/* SetCursor(NULL), ShowCursor(FALSE): the system pointer hidden over the
   content area (the game draws its sprite), shown outside it. */
void bf_input_hide_cursor(void);

/* ---------------------------------------------------------------- clipboard */

/* The clipboard's text in CP1252 (characters it lacks dropped) with lines
   ending CR LF, as CF_TEXT held it; malloc'd, for free(). NULL when the
   clipboard holds no text that converts. */
char *bf_input_clipboard_text(void);

/* Replaces the clipboard with text (CP1252, len bytes): UTF-8, CR LF as LF.
   1, or 0 on failure. */
int bf_input_clipboard_set_text(const char *text, size_t len);

#ifdef __cplusplus
}
#endif

#endif
