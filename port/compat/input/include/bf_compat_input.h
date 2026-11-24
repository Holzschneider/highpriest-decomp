/*
 * bf_compat_input: the client's user32 window, message, keyboard, cursor and
 * clipboard names, DirectInput 7 and imm32, on port/native/app and
 * port/native/view (stage A; M11 deletes it). The Win32 functions themselves
 * are declared by <windows.h>, <dinput.h> and <imm.h>; this header holds the
 * port-side functions that start-up code, other adapters and the control
 * socket call. Include <windows.h> first.
 *
 * Threads: the main thread (the one that calls bf_app_init), as user32 is.
 */
#ifndef BF_COMPAT_INPUT_H
#define BF_COMPAT_INPUT_H

#include "bf_app.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The single-instance slot bf_app_init is started with when the adapter
 * starts the application (first FindWindowA, RegisterClassA or
 * CreateWindowExA). Call before any of them. Default: instance "main",
 * single instance on. instance_id is copied.
 */
void bf_compat_input_configure(const char *instance_id, int single_instance);

/*
 * The UI scale setting (0 = auto, 1..3) CreateWindowExA passes to
 * bf_view_init. After the window exists it is passed on to
 * bf_view_set_scale_setting. Default 0 (auto).
 */
void bf_compat_input_set_scale_setting(int setting);

/*
 * Called for every event the message pump takes out of the app queue,
 * before it is translated. Returning non-zero consumes the event: no message
 * is made from it (the key and pointer state of bf_app still follow it).
 * For the control socket's USER events and for tests. NULL removes it.
 */
typedef int (*bf_compat_input_event_hook)(const bf_app_event *event, void *context);
void bf_compat_input_set_event_hook(bf_compat_input_event_hook hook, void *context);

/* The window CreateWindowExA made, or NULL. */
HWND bf_compat_input_main_window(void);

/*
 * The DirectInput mouse reports, per GetDeviceState, the difference between
 * the pointer's logical position and the position it believes the client's
 * CMouse holds (m_xPos, m_yPos), and then believes the client has moved by
 * that much and clamped to the logical screen, as CMouse::ReadState does.
 * The belief starts at (320, 240), CMouse::Init's values, and follows
 * GetCursorPos, whose one caller hands the result to CMouse::SetXPos/SetYPos.
 * These two let other code read or correct it.
 */
void bf_compat_input_mouse_get_client_pos(int *x, int *y);
void bf_compat_input_mouse_set_client_pos(int x, int y);

/*
 * The Windows virtual key, set-1 scan code and extended-key flag the adapter
 * gives a portable key (BF_KEY_*) in WM_KEYDOWN. 0 when it has none. For
 * tests and the control socket. scan and extended may be NULL.
 */
unsigned bf_compat_input_vk_for_key(int key, unsigned *scan, int *extended);

/*
 * Alt (Option) shortcuts of the game: the virtual keys for which Alt+key
 * does something in the client (UIWindowMgr::ProcessPushButton). Text that
 * a key-down with Option (and neither Ctrl nor Cmd) types is dropped when
 * its key is one of these, and delivered as WM_CHAR otherwise.
 */
int bf_compat_input_is_alt_shortcut(unsigned vk);

#ifdef __cplusplus
}
#endif

#endif
