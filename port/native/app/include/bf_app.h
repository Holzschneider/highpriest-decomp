/*
 * bf_app: the application module. The AppKit side of the client: the
 * process as a regular application, one window with a Metal layer, the one
 * event queue that all input passes through, the key and pointer state kept
 * from that queue, and the clipboard.
 *
 * The client keeps its own loop on the main thread. Each frame it calls
 * bf_app_pump(), which processes pending AppKit events without blocking and
 * turns them into bf_app_event records, and then drains the queue with
 * bf_app_poll_event(). The control socket injects into the same queue with
 * bf_app_post_event(); to the consumer an injected event differs from an
 * AppKit one only by its `source` field.
 *
 * Key and pointer state (bf_app_key_is_down, bf_app_pointer_get, ...) is
 * updated when an event is taken out of the queue by bf_app_poll_event, not
 * when AppKit produces it. The state therefore always matches the events the
 * client has seen, as GetKeyState does on Windows, and injected events move
 * it exactly as real ones do.
 *
 * Coordinates are relative to the window's content area, origin top left,
 * y downwards. "pt" values are AppKit points, "px" values are pixels of the
 * Metal layer's drawable (points times the backing scale).
 *
 * Threads: every function runs on the main thread unless its comment says
 * otherwise. The main thread is the one that called bf_app_init.
 *
 * C header, usable from C and C++14. No AppKit types cross it; the window
 * and the layer cross as void *.
 */
#ifndef BF_APP_H
#define BF_APP_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------- results */

enum {
	BF_APP_OK              = 0,
	BF_APP_ERROR           = -1,  /* see the BFT_ERR trace line */
	BF_APP_ALREADY_RUNNING = 1    /* bf_app_init: another instance has been brought to the front; exit */
};

/* ------------------------------------------------------------------ clock */

/*
 * The time stamped on every event, in milliseconds. The clock belongs to the
 * sys module (bf_clock_*); the application installs it here once, before or
 * after bf_app_init. Until it is installed, the module uses a monotonic
 * placeholder clock starting at 0 and says so once in the BFT_INPUT trace.
 * Any thread may call the installed function.
 */
typedef uint64_t (*bf_app_clock_fn)(void);
void bf_app_set_clock(bf_app_clock_fn now_ms);
uint64_t bf_app_now_ms(void);   /* any thread */

/* ------------------------------------------------------------------- init */

typedef struct bf_app_config {
	/*
	 * Name of the single-instance slot. NULL means "main", the client's
	 * own. Tests use their own name so that they can run beside the client.
	 */
	const char *instance_id;
	/*
	 * Non-zero: a second process with the same instance_id brings the first
	 * one to the front and bf_app_init returns BF_APP_ALREADY_RUNNING.
	 * The environment variable BF_MULTI_INSTANCE=1 turns this off.
	 */
	int single_instance;
} bf_app_config;

/*
 * Makes the process a regular application (Dock icon, menu bar with Hide,
 * Full Screen and Quit) without entering [NSApp run], whether or not it was
 * started from a bundle, and opts out of App Nap for the life of the
 * process. Call once, first, on the main thread. config may be NULL
 * (instance "main", single instance on).
 */
int bf_app_init(const bf_app_config *config);

/* Closes the window and releases the activity and the instance lock. */
void bf_app_shutdown(void);

/*
 * Processes the AppKit events that are pending and turns them into queue
 * entries. timeout_ms = 0 never blocks; a positive value waits at most that
 * long for the first event (an event posted from another thread ends the
 * wait). Returns the number of AppKit events processed. A nested call (from
 * the modal tick below) returns 0 at once.
 */
int bf_app_pump(int timeout_ms);

/*
 * While AppKit runs a modal loop of its own on the main thread (a live
 * window resize, an open menu), bf_app_pump does not return. If a tick is
 * set, it is called about 60 times a second during such a loop so that the
 * client can keep its frame going. The tick must not call bf_app_pump.
 */
void bf_app_set_modal_tick(void (*tick)(void *context), void *context);

/* Non-zero while the activity that disables App Nap is held. Any thread. */
int bf_app_activity_held(void);

/* ----------------------------------------------------------------- window */

typedef struct bf_app_size {
	double width_pt, height_pt;   /* content area in points */
	int width_px, height_px;      /* the same in pixels: the drawable size */
	double scale;                 /* backing scale factor, 1.0 or 2.0 usually */
} bf_app_size;

/*
 * Creates the one window: titled, resizable, able to go fullscreen, content
 * area width_pt x height_pt points, centred, shown and made key. title is
 * UTF-8. The content view is backed by a CAMetalLayer whose drawableSize the
 * module keeps equal to the content size in pixels (and its contentsScale
 * equal to the backing scale); its device is the system default device until
 * the graphics module sets another. Returns BF_APP_OK or BF_APP_ERROR.
 */
int bf_app_window_create(double width_pt, double height_pt, const char *title);
void bf_app_window_destroy(void);

void *bf_app_window_nswindow(void);     /* NSWindow *, or NULL */
void *bf_app_window_metal_layer(void);  /* CAMetalLayer *, or NULL */

void bf_app_window_get_size(bf_app_size *out);
/* Resizes the content area. A resize event follows. */
void bf_app_window_set_content_size(double width_pt, double height_pt);
void bf_app_window_set_title(const char *title);

/*
 * The smallest content size the user can drag the window to, in points
 * (default 320 x 240). Does not resize the window by itself. The view module
 * (port/native/view) sets it from the UI scale.
 */
void bf_app_window_set_min_content_size(double width_pt, double height_pt);
void bf_app_window_get_min_content_size(double *width_pt, double *height_pt);

typedef struct bf_app_screen {
	double width_pt, height_pt;                  /* the whole screen: the content size in fullscreen */
	double visible_width_pt, visible_height_pt;  /* the largest content area of a titled window on it
	                                                (without menu bar, Dock and title bar) */
	double scale;                                /* the screen's backing scale */
} bf_app_screen;

/* The screen the window is on (the main screen without a window). BF_APP_OK or BF_APP_ERROR. */
int bf_app_window_get_screen(bf_app_screen *out);

/*
 * Native fullscreen (its own Space). Cmd+F and the View menu toggle it too.
 * The change is animated: a BF_APP_EVENT_FULLSCREEN event and a resize
 * event arrive when it is complete, some hundred milliseconds later.
 */
void bf_app_window_toggle_fullscreen(void);
int bf_app_window_is_fullscreen(void);

/* Non-zero when some part of the window is visible on a screen. */
int bf_app_window_is_visible(void);
/* Non-zero when the window is the key window of the active application. */
int bf_app_window_is_focused(void);

/*
 * Hides the system pointer while it is over the content area, where the
 * game draws its own cursor sprite. Outside the window the pointer is shown
 * as usual. The pointer is never captured, warped or confined.
 */
void bf_app_set_cursor_hidden_in_window(int hidden);

/* ------------------------------------------------------------------- keys */

/*
 * Portable key codes: the USB HID keyboard usage IDs (usage page 7), so
 * that the values are stable and documented elsewhere. Only the keys a
 * keyboard on a Mac can produce are named. Fn has no HID usage and gets a
 * private value. Keys macOS reports that have no entry arrive as
 * BF_KEY_NONE with their mac_keycode.
 */
typedef enum bf_key {
	BF_KEY_NONE = 0,
	BF_KEY_A = 0x04, BF_KEY_B, BF_KEY_C, BF_KEY_D, BF_KEY_E, BF_KEY_F, BF_KEY_G,
	BF_KEY_H, BF_KEY_I, BF_KEY_J, BF_KEY_K, BF_KEY_L, BF_KEY_M, BF_KEY_N,
	BF_KEY_O, BF_KEY_P, BF_KEY_Q, BF_KEY_R, BF_KEY_S, BF_KEY_T, BF_KEY_U,
	BF_KEY_V, BF_KEY_W, BF_KEY_X, BF_KEY_Y, BF_KEY_Z,                  /* 0x1d */
	BF_KEY_1 = 0x1e, BF_KEY_2, BF_KEY_3, BF_KEY_4, BF_KEY_5,
	BF_KEY_6, BF_KEY_7, BF_KEY_8, BF_KEY_9, BF_KEY_0,                  /* 0x27 */
	BF_KEY_RETURN = 0x28, BF_KEY_ESCAPE, BF_KEY_BACKSPACE, BF_KEY_TAB, BF_KEY_SPACE,
	BF_KEY_MINUS = 0x2d, BF_KEY_EQUAL, BF_KEY_LEFTBRACKET, BF_KEY_RIGHTBRACKET,
	BF_KEY_BACKSLASH = 0x31,
	BF_KEY_SEMICOLON = 0x33, BF_KEY_QUOTE, BF_KEY_GRAVE, BF_KEY_COMMA,
	BF_KEY_PERIOD, BF_KEY_SLASH, BF_KEY_CAPSLOCK,                       /* 0x39 */
	BF_KEY_F1 = 0x3a, BF_KEY_F2, BF_KEY_F3, BF_KEY_F4, BF_KEY_F5, BF_KEY_F6,
	BF_KEY_F7, BF_KEY_F8, BF_KEY_F9, BF_KEY_F10, BF_KEY_F11, BF_KEY_F12, /* 0x45 */
	BF_KEY_PRINTSCREEN = 0x46, BF_KEY_SCROLLLOCK, BF_KEY_PAUSE,
	BF_KEY_INSERT = 0x49, BF_KEY_HOME, BF_KEY_PAGEUP, BF_KEY_DELETE, BF_KEY_END,
	BF_KEY_PAGEDOWN, BF_KEY_RIGHT, BF_KEY_LEFT, BF_KEY_DOWN, BF_KEY_UP,  /* 0x52 */
	BF_KEY_NUMLOCK = 0x53,          /* the keypad's Clear key on a Mac */
	BF_KEY_KP_DIVIDE, BF_KEY_KP_MULTIPLY, BF_KEY_KP_MINUS, BF_KEY_KP_PLUS,
	BF_KEY_KP_ENTER, BF_KEY_KP_1, BF_KEY_KP_2, BF_KEY_KP_3, BF_KEY_KP_4,
	BF_KEY_KP_5, BF_KEY_KP_6, BF_KEY_KP_7, BF_KEY_KP_8, BF_KEY_KP_9,
	BF_KEY_KP_0, BF_KEY_KP_PERIOD,                                      /* 0x63 */
	BF_KEY_NONUSBACKSLASH = 0x64,   /* the ISO key left of Z (kVK_ISO_Section) */
	BF_KEY_MENU = 0x65,
	BF_KEY_KP_EQUAL = 0x67,
	BF_KEY_F13 = 0x68, BF_KEY_F14, BF_KEY_F15, BF_KEY_F16, BF_KEY_F17,
	BF_KEY_F18, BF_KEY_F19, BF_KEY_F20,                                 /* 0x6f */
	BF_KEY_MUTE = 0x7f, BF_KEY_VOLUMEUP, BF_KEY_VOLUMEDOWN,
	BF_KEY_LCTRL = 0xe0, BF_KEY_LSHIFT, BF_KEY_LALT, BF_KEY_LCMD,
	BF_KEY_RCTRL, BF_KEY_RSHIFT, BF_KEY_RALT, BF_KEY_RCMD,              /* 0xe7 */
	BF_KEY_FN = 0xf0,               /* not a HID usage */
	BF_KEY_COUNT = 0x100
} bf_key;

/* The portable code of a macOS virtual key code (kVK_*), BF_KEY_NONE if none. Any thread. */
int bf_app_key_from_mac(unsigned mac_keycode);
/* The macOS virtual key code of a portable code, or -1. Any thread. */
int bf_app_key_to_mac(int key);
/* Lower-case names ("a", "1", "f1", "return", "left", "lshift", "kp_plus"); "?" if none. Any thread. */
const char *bf_app_key_name(int key);
/* The reverse of bf_app_key_name, for the control socket; BF_KEY_NONE if unknown. Any thread. */
int bf_app_key_from_name(const char *name);

/* --------------------------------------------------------- keyboard layout */

/*
 * What the keyboard layout types, through the system (UCKeyTranslate), so
 * that a consumer can tell an Option combination that types a character the
 * layout has nowhere else ("@" on German Option+L) from one that only
 * decorates a letter. The layout is the one in effect for typing (the
 * current keyboard layout, also while an input method is active), or the one
 * bf_app_key_layout_use named. Main thread.
 */

/*
 * The characters one press of the key types with the modifiers
 * (BF_APP_MOD_SHIFT, BF_APP_MOD_ALT and BF_APP_MOD_CAPSLOCK count; the
 * others are ignored), with no dead key pending, as UTF-8 into buf
 * (NUL-terminated, truncated to fit size). A dead key gives the accent it
 * leaves pending (Option+E on a US layout: "´") and sets *dead to 1 (dead
 * may be NULL). Returns the length in bytes without the NUL; 0 when the key
 * types nothing, is unknown, or no layout data is available.
 */
size_t bf_app_key_layout_text(unsigned mac_keycode, uint32_t mods, char *buf, size_t size, int *dead);

/*
 * Non-zero when the layout types utf8 (the whole string) with one key alone
 * or with Shift: without Option. A dead key's accent counts (it is typed by
 * the dead key and Space).
 */
int bf_app_key_layout_types_plain(const char *utf8);

/*
 * Uses the installed keyboard layout with this input source id (for example
 * "com.apple.keylayout.German") for the two functions above, without
 * selecting it in the system; NULL goes back to the layout in effect. For
 * tests. Returns BF_APP_OK, or BF_APP_ERROR (traced) when no such layout is
 * installed; the previous choice then stands.
 */
int bf_app_key_layout_use(const char *source_id);

/* The id bf_app_key_layout_use named, or NULL while the layout in effect is used. */
const char *bf_app_key_layout_named(void);

/*
 * A test applied to every key-down from AppKit (not to injected ones) after
 * it is queued and before the text system sees it: returning 0 keeps the key
 * from the text system, so that it types nothing and leaves no dead key or
 * input-method composition pending. key_down is the queued event, its seq
 * set. Cmd combinations never reach the text system and are not passed.
 * NULL (the default) passes every key on. Main thread.
 */
struct bf_app_event;
typedef int (*bf_app_text_filter)(const struct bf_app_event *key_down, void *context);
void bf_app_set_text_filter(bf_app_text_filter filter, void *context);

/* Modifier flags. Option is Alt. */
enum {
	BF_APP_MOD_SHIFT    = 1 << 0,
	BF_APP_MOD_CTRL     = 1 << 1,
	BF_APP_MOD_ALT      = 1 << 2,   /* Option */
	BF_APP_MOD_CMD      = 1 << 3,
	BF_APP_MOD_CAPSLOCK = 1 << 4,
	BF_APP_MOD_FN       = 1 << 5
};

/* --------------------------------------------------------------- pointer */

enum {
	BF_APP_BUTTON_LEFT   = 0,
	BF_APP_BUTTON_RIGHT  = 1,
	BF_APP_BUTTON_MIDDLE = 2,
	BF_APP_BUTTON_X1     = 3,
	BF_APP_BUTTON_X2     = 4,
	BF_APP_BUTTON_COUNT  = 5
};

/* --------------------------------------------------------------- events */

typedef enum bf_app_event_type {
	BF_APP_EVENT_NONE = 0,
	BF_APP_EVENT_KEY_DOWN,      /* key: also for modifiers; key.repeat on auto-repeat */
	BF_APP_EVENT_KEY_UP,        /* key */
	BF_APP_EVENT_TEXT,          /* text: characters typed, after the KEY_DOWN that typed them */
	BF_APP_EVENT_POINTER_MOVE,  /* pointer */
	BF_APP_EVENT_BUTTON_DOWN,   /* pointer, pointer.button */
	BF_APP_EVENT_BUTTON_UP,     /* pointer, pointer.button */
	BF_APP_EVENT_WHEEL,         /* pointer, pointer.wheel_x / wheel_y */
	BF_APP_EVENT_POINTER_ENTER, /* pointer: entered the content area */
	BF_APP_EVENT_POINTER_LEAVE, /* pointer: left it */
	BF_APP_EVENT_RESIZE,        /* size: content size or backing scale changed */
	BF_APP_EVENT_FOCUS,         /* value: 1 gained, 0 lost (keys and buttons were released before) */
	BF_APP_EVENT_VISIBILITY,    /* value: 1 some part visible, 0 occluded or minimised */
	BF_APP_EVENT_FULLSCREEN,    /* value: 1 entered, 0 left */
	BF_APP_EVENT_QUIT,          /* value: BF_APP_QUIT_* ; nothing has been closed yet */
	BF_APP_EVENT_REOPEN,        /* value: 1 a second launch, 0 Dock icon clicked; the window is already in front */
	BF_APP_EVENT_USER           /* user: free for injectors (the control socket) */
} bf_app_event_type;

enum {
	BF_APP_QUIT_CLOSE_BUTTON = 0,  /* the window's close button */
	BF_APP_QUIT_COMMAND      = 1,  /* Cmd+Q or the Quit menu item */
	BF_APP_QUIT_SYSTEM       = 2   /* a terminate request from outside: the Dock's Quit, logout, AppleScript */
};

enum {
	BF_APP_SOURCE_APPKIT   = 0,
	BF_APP_SOURCE_INJECTED = 1     /* anything that came through bf_app_post_event */
};

/* Event flags. */
enum {
	/*
	 * POINTER_MOVE, BUTTON_*, WHEEL: dx_px/dy_px hold the device's own
	 * movement. Without it (an injected event that leaves it clear), the
	 * delta is filled in when the event is dequeued as the difference to the
	 * previous pointer position.
	 */
	BF_APP_EVF_DELTA_VALID = 1 << 0,
	/* WHEEL: from a trackpad or Magic Mouse (continuous, with momentum), not a wheel with notches. */
	BF_APP_EVF_PRECISE     = 1 << 1
};

#define BF_APP_TEXT_MAX 32   /* bytes in a text event including the NUL; longer text is split */

typedef struct bf_app_event {
	uint32_t type;      /* bf_app_event_type */
	uint32_t source;    /* BF_APP_SOURCE_* */
	uint32_t flags;     /* BF_APP_EVF_* */
	uint32_t mods;      /* BF_APP_MOD_* at the time of the event */
	uint64_t seq;       /* queue order, set by the queue, starts at 1 */
	uint64_t time_ms;   /* from the installed clock; set by the queue if 0 */
	union {
		struct {
			uint16_t mac_keycode;   /* kVK_*, 0xffff if none; injected: derived from `key` when key != 0 */
			uint16_t key;           /* bf_key; injected with 0: derived from mac_keycode */
			uint8_t repeat;
		} key;
		struct {
			char utf8[BF_APP_TEXT_MAX];  /* NUL-terminated, printable characters only */
			uint64_t key_seq;            /* seq of the KEY_DOWN that typed it; 0 if none (IME, injected) */
		} text;
		struct {
			float x_px, y_px;       /* position in the content area, pixels */
			float x_pt, y_pt;       /* the same in points */
			float dx_px, dy_px;     /* movement since the previous pointer event, pixels */
			float wheel_x, wheel_y; /* WHEEL: lines, fractional from a trackpad; y > 0 is up/away, as the user set scrolling */
			uint32_t button;        /* BUTTON_*: BF_APP_BUTTON_* */
			uint32_t buttons;       /* buttons held after the event, bit per BF_APP_BUTTON_* */
			uint32_t clicks;        /* BUTTON_DOWN: AppKit's click count (2 = double click) */
		} pointer;
		struct {
			bf_app_size size;
		} size;
		int32_t value;              /* FOCUS, VISIBILITY, FULLSCREEN, QUIT, REOPEN */
		struct {
			uint32_t code;
			int64_t a, b;
		} user;
	};
} bf_app_event;

/*
 * Takes the oldest event out of the queue into *out and applies it to the
 * key and pointer state. Returns 1, or 0 when the queue is empty.
 */
int bf_app_poll_event(bf_app_event *out);

/*
 * Puts an event at the end of the queue: how the control socket injects.
 * Any thread. The queue sets seq, source (BF_APP_SOURCE_INJECTED) and, if 0,
 * time_ms. For injected key, text and pointer events, the modifiers of the
 * modifier keys held in the state are added to mods when dequeued, so that
 * "lshift down, a down" reads as Shift+A. For injected pointer events the
 * pixel position is authoritative: the point position is derived from it,
 * and `buttons` is always set from the state. Wakes a waiting bf_app_pump.
 * Returns BF_APP_OK, or BF_APP_ERROR when the queue is full (4096 events).
 */
int bf_app_post_event(const bf_app_event *event);

/*
 * As bf_app_post_event, and returns the seq the queue gave the event, or 0
 * when the queue is full. Any thread.
 */
uint64_t bf_app_post_event_seq(const bf_app_event *event);

/*
 * The seq of the last event bf_app_poll_event has taken out of the queue,
 * 0 before the first. Since the queue is first in, first out, an event
 * posted with seq s has been taken once this is >= s: how the control
 * socket knows the client has consumed what it injected. Any thread.
 */
uint64_t bf_app_consumed_seq(void);

/* Number of events waiting. Any thread. */
int bf_app_queue_size(void);

/* ------------------------------------------------------- state from queue */

/* Non-zero while the key is held, as of the last dequeued event. */
int bf_app_key_is_down(int key);
int bf_app_mac_key_is_down(unsigned mac_keycode);
/* BF_APP_MOD_* derived from the modifier keys held. */
uint32_t bf_app_modifiers(void);

typedef struct bf_app_pointer {
	float x_px, y_px;     /* last position, content area, pixels (may be outside while dragging) */
	float x_pt, y_pt;
	uint32_t buttons;     /* bit per BF_APP_BUTTON_* */
	int inside;           /* over the content area */
} bf_app_pointer;
void bf_app_pointer_get(bf_app_pointer *out);

/*
 * Movement and wheel accumulated from dequeued events since the previous
 * call, then reset: what a DirectInput mouse reports per read. Any pointer
 * may be NULL.
 */
void bf_app_pointer_take_delta(float *dx_px, float *dy_px, float *wheel_y);

/* ------------------------------------------------------------- clipboard */

/*
 * Copies the clipboard's plain text, UTF-8 and NUL-terminated, into buf
 * (truncated at a character boundary to fit size). Returns the full length
 * in bytes without the NUL, 0 when the clipboard holds no text.
 */
size_t bf_app_clipboard_get_text(char *buf, size_t size);
/* Replaces the clipboard with plain text. Returns BF_APP_OK or BF_APP_ERROR. */
int bf_app_clipboard_set_text(const char *utf8);

#ifdef __cplusplus
}
#endif

#endif
