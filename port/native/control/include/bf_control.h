/*
 * bf_control: the control socket (PROTOCOL.md, version 0).
 *
 * The client can be driven and observed from outside through a Unix-domain
 * socket speaking JSON, one object per line. This module serves that
 * protocol: an I/O thread reads and writes the socket, and the client's main
 * thread carries out every request inside bf_control_poll(), which the
 * client calls once per frame. Input is injected into the application
 * module's one event queue (bf_app_post_event); nothing reaches the client
 * behind that queue.
 *
 * What the module does by itself: the transport (one connection at a time,
 * framing, the 1 MiB line limit, seq numbering, the `busy` refusal), the
 * request registry, JSON reading and writing, and the commands that need
 * nothing from the client or only a hook: hello, key, pointer, wait,
 * capture, state, resize, quit. The client supplies state providers
 * (bf_control_register_state), hooks (bf_control_set_hooks), its events
 * (bf_control_event_begin) and, if it wants, commands of its own
 * (bf_control_register).
 *
 * The client's loop, per frame:
 *
 *     bf_app_pump(0); while (bf_app_poll_event(&e)) handle(e);
 *     logic step(s);
 *     bf_control_poll();            // after the events have been handled
 *     draw and present frame n;
 *     bf_control_frame_done(n);     // n = frames drawn so far
 *
 * Threads: every function runs on the main thread (the one that calls
 * bf_control_poll) unless its comment says otherwise. Callbacks (handlers,
 * providers, checks, hooks) are called on the main thread, from inside
 * bf_control_poll or bf_control_frame_done or the handler that triggers
 * them; they must not call bf_control_poll or bf_control_frame_done.
 *
 * C header, usable from C and C++14.
 */
#ifndef BF_CONTROL_H
#define BF_CONTROL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BF_CONTROL_PROTOCOL  0           /* the version this module speaks */
#define BF_CONTROL_LINE_MAX  (1u << 20)  /* bytes in one line, without the \n */
#define BF_CONTROL_PATH_MAX  103         /* bytes in the socket path (macOS) */

enum {
	BF_CONTROL_OK    = 0,
	BF_CONTROL_ERROR = -1   /* see the trace (BFT_ERR) */
};

/* The error codes of PROTOCOL.md section 2. */
typedef enum bf_control_code {
	BF_CONTROL_E_NONE = 0,
	BF_CONTROL_E_BAD_REQUEST,
	BF_CONTROL_E_UNKNOWN_COMMAND,
	BF_CONTROL_E_UNSUPPORTED,
	BF_CONTROL_E_BAD_KEY,
	BF_CONTROL_E_BAD_TEXT,
	BF_CONTROL_E_BAD_STATE,
	BF_CONTROL_E_TIMEOUT,
	BF_CONTROL_E_IO,
	BF_CONTROL_E_INTERNAL
} bf_control_code;

/* "bad_request", ...; "internal" for an unknown value. Any thread. */
const char *bf_control_code_name(int code);

/* -------------------------------------------------------------- lifecycle */

/*
 * The socket path the client was asked for: the argument after --control
 * (or of --control=<path>) in argv, else the environment variable
 * BF_CONTROL, else NULL (no socket). The flag wins. The string returned
 * points into argv or the environment. Any thread.
 */
const char *bf_control_path_from_args(int argc, char *const *argv);

typedef struct bf_control_config {
	const char *path;      /* the socket path; at most BF_CONTROL_PATH_MAX bytes */
	const char *version;   /* the client's build string, for hello; NULL: "unknown" */
} bf_control_config;

/*
 * Removes a stale file at the path (not a directory), binds and listens,
 * and starts the I/O thread. Never waits for a connection. Returns
 * BF_CONTROL_OK, or BF_CONTROL_ERROR with a trace line when the path is too
 * long, cannot be bound, or the module is already open; the client then runs
 * without the socket. The registry, the providers and the hooks are kept
 * from before, across open and close.
 */
int bf_control_open(const bf_control_config *config);

/*
 * Sends the event `bye` with the reason (`quit`, `signal`, `error`; NULL:
 * `quit`) when a connection is open, writes what is still unsent (waiting
 * at most a second), closes the connection and the socket, removes the
 * socket file and stops the I/O thread. Deferred requests are cancelled:
 * those with a check function are freed; one deferred without a check
 * stays valid until its owner replies, and that reply is discarded.
 * Nothing happens when the module is not open.
 */
void bf_control_close(const char *reason);

int bf_control_is_open(void);
/* Non-zero while a connection is open, as the main thread knows it. */
int bf_control_is_connected(void);

/*
 * Once per frame, after the client has taken the injected events from the
 * app queue and carried them out (so that a reply sent here sees their
 * effect). Takes the lines the I/O thread has read, posts the next logic
 * step of an injection, checks deferred requests and carries out requests
 * in order until one is deferred. Never blocks.
 */
void bf_control_poll(void);

/*
 * After frame `frames_drawn` (1 for the first) has been completely drawn,
 * before the next one starts, so that the target still holds it: captures
 * are taken here, `resize` and `wait {"frames": n}` replies are sent from
 * here, and the built-in state part `frame` reports this number.
 */
void bf_control_frame_done(uint64_t frames_drawn);

/* The last number given to bf_control_frame_done; 0 before. */
uint64_t bf_control_frame(void);

/* ------------------------------------------------------------ JSON values */

/*
 * A parsed JSON value, read-only, owned by the module (a request's
 * arguments). Every function accepts NULL and then reports "absent".
 */
typedef struct bf_control_value bf_control_value;

typedef enum bf_control_type {
	BF_CONTROL_T_ABSENT = 0,  /* NULL: the key is not there */
	BF_CONTROL_T_NULL,
	BF_CONTROL_T_BOOL,
	BF_CONTROL_T_NUMBER,
	BF_CONTROL_T_STRING,
	BF_CONTROL_T_ARRAY,
	BF_CONTROL_T_OBJECT
} bf_control_type;

int bf_control_value_type(const bf_control_value *v);
/* The member of an object, NULL if v is no object or has no such key (the last one wins). */
const bf_control_value *bf_control_value_get(const bf_control_value *object, const char *key);
/* 1 and *out when v is a number written as an integer that fits int64 ("5", not "5.0"), else 0. */
int bf_control_value_int(const bf_control_value *v, int64_t *out);
/* 1 and *out when v is a number, else 0. */
int bf_control_value_number(const bf_control_value *v, double *out);
/* 1 and *out (0 or 1) when v is true or false, else 0. */
int bf_control_value_bool(const bf_control_value *v, int *out);
/* The UTF-8 string, NUL-terminated (*length without the NUL may be NULL), or NULL if v is no string. */
const char *bf_control_value_string(const bf_control_value *v, size_t *length);
/* Elements of an array or members of an object; 0 for anything else. */
size_t bf_control_value_count(const bf_control_value *v);
const bf_control_value *bf_control_value_at(const bf_control_value *v, size_t index);
/* The key of member `index` of an object, NULL if none. */
const char *bf_control_value_key_at(const bf_control_value *v, size_t index);

/* ------------------------------------------------------------ JSON writer */

/*
 * Writes JSON into a reply, an event or a state part. Every function accepts
 * NULL and then does nothing, so that an event nobody receives costs nothing
 * (bf_control_event_begin returns NULL). Inside an object a key comes before
 * each value. Strings are UTF-8; invalid bytes are written as U+FFFD.
 * A misuse (a value without a key, an unclosed object) is traced, and the
 * message becomes an `internal` error or, for a state part, `null`.
 */
typedef struct bf_control_writer bf_control_writer;

void bf_control_write_key(bf_control_writer *w, const char *key);
void bf_control_write_null(bf_control_writer *w);
void bf_control_write_bool(bf_control_writer *w, int value);
void bf_control_write_int(bf_control_writer *w, int64_t value);
void bf_control_write_number(bf_control_writer *w, double value);   /* NaN and infinities as null */
void bf_control_write_string(bf_control_writer *w, const char *utf8);  /* NULL as null */
void bf_control_write_string_n(bf_control_writer *w, const char *utf8, size_t length);
void bf_control_write_begin_object(bf_control_writer *w);
void bf_control_write_end_object(bf_control_writer *w);
void bf_control_write_begin_array(bf_control_writer *w);
void bf_control_write_end_array(bf_control_writer *w);
/* A copy of a parsed value (an argument echoed back). */
void bf_control_write_value(bf_control_writer *w, const bf_control_value *v);

/* ------------------------------------------------------------- requests */

typedef struct bf_control_request bf_control_request;

/*
 * Carries out a request. It replies (bf_control_reply, _reply_error) before
 * it returns, or defers the reply (bf_control_defer). A handler that does
 * neither gets an `internal` error sent for it.
 */
typedef void (*bf_control_handler)(bf_control_request *request, void *context);

/*
 * Registers a command, or replaces one of the same name (also a built-in
 * one); a NULL handler removes it. The names of PROTOCOL.md section 6 are
 * refused (BF_CONTROL_ERROR): they answer `unsupported`. The name appears
 * in hello's `commands`.
 */
int bf_control_register(const char *name, bf_control_handler handler, void *context);

const char *bf_control_request_command(const bf_control_request *request);
/* The whole request object; bf_control_value_get(args, "x") reads an argument. */
const bf_control_value *bf_control_request_args(const bf_control_request *request);
/* Shorthand for bf_control_value_get(bf_control_request_args(request), key). */
const bf_control_value *bf_control_request_arg(const bf_control_request *request, const char *key);

/*
 * The writer for the reply's result fields (keys and values written into
 * the reply object after "id", "ok" and "seq"; for an error reply, the
 * extra fields beside "error"). The same writer on every call.
 */
bf_control_writer *bf_control_reply_writer(bf_control_request *request);

/*
 * Sends the reply, `ok` true with the fields written, or `ok` false with
 * {"code", "message"} and the fields written. Its `seq` is that of the last
 * event sent before it. Afterwards the request must not be used again. A
 * reply to a cancelled request is discarded. A second reply is ignored.
 */
void bf_control_reply(bf_control_request *request);
void bf_control_reply_error(bf_control_request *request, int code, const char *message);

/*
 * Keeps the request open after the handler returns: the requests behind it
 * wait until it is answered (PROTOCOL.md: replies in order). With a check
 * function, the module calls check(request, context) from every
 * bf_control_poll and bf_control_frame_done until it replies; without one,
 * the owner keeps the pointer and replies later on the main thread.
 */
typedef void (*bf_control_check)(bf_control_request *request, void *context);
void bf_control_defer(bf_control_request *request, bf_control_check check, void *context);

/* Non-zero once the request's connection has closed: its reply will be discarded. */
int bf_control_request_cancelled(const bf_control_request *request);

/* ----------------------------------------------------------------- events */

/*
 * Adds a name to hello's `events` (bye and error are built in). Emitting
 * does not require it.
 */
void bf_control_declare_event(const char *name);

/*
 * Starts an event: returns a writer for its fields (after "event", "seq",
 * "t"), or NULL when no connection is open. bf_control_event_end numbers it
 * and sends it. One event at a time. Events are kept per connection for
 * `wait {"event": ...}`.
 */
bf_control_writer *bf_control_event_begin(const char *name);
void bf_control_event_end(bf_control_writer *w);

/* ------------------------------------------------------------------ state */

/*
 * Writes one state part as exactly one JSON value (an object, a list, a
 * string or null). Called for `state` and to evaluate `wait` conditions,
 * which read `mode`, `map`, `windows` and `self` (the fields of PROTOCOL.md
 * section 3). A part with no provider is left out of `state` replies, and a
 * `wait` that needs it answers `unsupported`. The module provides `frame`
 * (bf_control_frame()) unless the client registers its own.
 */
typedef void (*bf_control_state_fn)(bf_control_writer *w, void *context);

/* Registers or replaces a part; NULL removes it. Reserved part names are refused. */
int bf_control_register_state(const char *part, bf_control_state_fn fn, void *context);

/* ------------------------------------------------------------------ hooks */

typedef struct bf_control_capture_info {
	int width, height;          /* the PNG's size, device pixels */
	double scale;               /* UI scale */
	int logical_w, logical_h;   /* logical size */
} bf_control_capture_info;

typedef struct bf_control_resize_args {
	int has_width, width;       /* window content, device pixels */
	int has_height, height;
	int has_scale, scale;       /* 1, 2 or 3 */
	int has_fullscreen, fullscreen;
} bf_control_resize_args;

/*
 * What the client supplies. Any member may be NULL: the command that needs
 * it then answers `unsupported` and is left out of hello's `commands`.
 * Functions returning int return a bf_control_code (BF_CONTROL_E_NONE on
 * success) and may put a message for people into `message`.
 */
typedef struct bf_control_hooks {
	void *context;              /* passed to every hook */

	/* `quit`: start the shutdown as Cmd+Q does. Called after the reply is sent. */
	void (*quit)(void *context);

	/*
	 * `capture`: write the frame just completed as an 8-bit RGBA PNG at the
	 * absolute path and fill *out. Called from bf_control_frame_done of the
	 * first frame completed after the request. The module reads the file
	 * back for its SHA-256. bf_gfx_capture_png does the writing.
	 */
	int (*capture)(void *context, const char *path, bf_control_capture_info *out,
	               char *message, size_t message_size);

	/*
	 * `resize`: start the change (arguments checked for range already).
	 * The reply is sent from bf_control_frame_done of the first frame after
	 * the call during which resize_busy (if set) returns 0, with the part
	 * `view` as its result.
	 */
	int (*resize)(void *context, const bf_control_resize_args *args,
	              char *message, size_t message_size);
	int (*resize_busy)(void *context);   /* non-zero while a fullscreen animation runs */

	/*
	 * Logical coordinates to the pixels of bf_app pointer events (content
	 * area, drawable pixels: logical x UI scale x backing scale), and back.
	 * Without them a logical pixel is one pixel.
	 */
	void (*logical_to_px)(void *context, double x, double y, float *x_px, float *y_px);
	void (*px_to_logical)(void *context, float x_px, float y_px, double *x, double *y);
} bf_control_hooks;

/* Copies the hooks; NULL clears them. */
void bf_control_set_hooks(const bf_control_hooks *hooks);

#ifdef __cplusplus
}
#endif

#endif
