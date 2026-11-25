/*
 * bf_sys: the operating system for the client, designed for macOS.
 *
 * This header holds threads, the Keychain and the shell functions, and
 * includes the module's other parts:
 *
 *   bf_clock.h      the one clock (time, sleep, calendar)
 *   bf_file.h       the game folder, the write locations, file access
 *   bf_settings.h   the settings object and its property list
 *
 * No Windows types. The adapters in port/compat give these functions the
 * Win32 names the client calls today (CreateThread, critical sections,
 * MessageBox, ShellExecute, ...); in M11 the client calls them directly.
 *
 * Atomics: the client's InterlockedIncrement/Decrement are served by the
 * compiler's atomics (std::atomic or __atomic_add_fetch on the LONG) in the
 * adapter; this module offers no atomics of its own.
 */
#ifndef BF_SYS_H
#define BF_SYS_H

#include <stddef.h>
#include <stdint.h>

#include "bf_clock.h"
#include "bf_file.h"
#include "bf_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The application name the module was built with (BF_APP_NAME). */
const char *bf_sys_app_name(void);

/* ---- Threads --------------------------------------------------------------
 *
 * A thread handle behaves like a Win32 thread handle: the thread runs
 * detached, the handle can be waited on any number of times from any thread,
 * and releasing the handle does not stop the thread. Every function here may
 * be called from any thread.
 */

typedef struct bf_sys_thread bf_sys_thread;
typedef uint32_t (*bf_sys_thread_fn)(void *argument);

/* Starts fn(argument) on a new thread. name (may be NULL) is shown in the debugger. NULL on failure. */
bf_sys_thread *bf_sys_thread_create(bf_sys_thread_fn fn, void *argument, const char *name);

/*
 * Waits up to timeout_ms (BF_SYS_WAIT_FOREVER for no limit) for the thread
 * to return. 1 if it has returned (*result, may be NULL, gets its value),
 * 0 on timeout.
 */
#define BF_SYS_WAIT_FOREVER 0xFFFFFFFFu
int bf_sys_thread_wait(bf_sys_thread *thread, uint32_t timeout_ms, uint32_t *result);

/* Drops the handle (CloseHandle); the thread keeps running if it still is. */
void bf_sys_thread_release(bf_sys_thread *thread);

/* Waits for the thread and releases the handle. Returns the thread's value. */
uint32_t bf_sys_thread_join(bf_sys_thread *thread);

/* A number identifying the calling thread, never 0 (GetCurrentThreadId). */
uint64_t bf_sys_thread_id(void);

/* 1 on the process's main thread. */
int bf_sys_is_main_thread(void);

/* Priority of the calling thread: -2 (lowest) .. 2 (highest), as THREAD_PRIORITY_*. Mapped to QoS classes. */
void bf_sys_thread_set_priority(int priority);

/* ---- Mutex ---------------------------------------------------------------
 * Recursive, like a critical section: the owning thread may lock it again
 * and must unlock it as often. Heap-allocated; the critical-section adapter
 * keeps the pointer inside the CRITICAL_SECTION it was given.
 */

typedef struct bf_sys_mutex bf_sys_mutex;

bf_sys_mutex *bf_sys_mutex_create(void);
void bf_sys_mutex_lock(bf_sys_mutex *mutex);
int  bf_sys_mutex_try_lock(bf_sys_mutex *mutex);   /* 1 if locked */
void bf_sys_mutex_unlock(bf_sys_mutex *mutex);
void bf_sys_mutex_destroy(bf_sys_mutex *mutex);

/* ---- Keychain ------------------------------------------------------------
 * The saved account password: a generic password item, service = the
 * application name, account = the account name (UTF-8). Thread: any.
 */

/*
 * 1 if the Keychain can be used from this session without asking anyone:
 * 0 when it is locked or unreachable (a background session without the
 * window server, errSecInteractionNotAllowed). Thread: any.
 */
int bf_sys_keychain_available(void);
/* Stores or replaces the password. 1 or 0 (trace line with the OSStatus). */
int bf_sys_keychain_store(const char *account, const char *password);
/* 1 and the password copied NUL-terminated; 0 if none is stored; -1 on error or if out_size is too small. */
int bf_sys_keychain_read(const char *account, char *out, size_t out_size);
/* 1 if an item was deleted, 0 if there was none, -1 on error. */
int bf_sys_keychain_delete(const char *account);

/* ---- Shell ----------------------------------------------------------------
 * Alerts are modal NSAlerts on the main thread. They are not shown, but
 * logged as a BFT_ERR trace line and answered with the first button, when
 * $BF_NO_ALERTS is set, when there is no window-server session, or when the
 * call is not on the main thread. Opening is done by NSWorkspace; with
 * $BF_NO_OPEN set it is logged and reported as done instead.
 */

typedef enum bf_sys_alert_style {
	BF_ALERT_INFO     = 0,
	BF_ALERT_WARNING  = 1,
	BF_ALERT_CRITICAL = 2
} bf_sys_alert_style;

typedef enum bf_sys_alert_buttons {
	BF_ALERT_BUTTONS_OK        = 0,
	BF_ALERT_BUTTONS_OK_CANCEL = 1,
	BF_ALERT_BUTTONS_YES_NO    = 2
} bf_sys_alert_buttons;

typedef enum bf_sys_alert_answer {
	BF_ALERT_OK     = 1,
	BF_ALERT_CANCEL = 2,
	BF_ALERT_YES    = 6,
	BF_ALERT_NO     = 7
} bf_sys_alert_answer;   /* the values of IDOK, IDCANCEL, IDYES, IDNO */

/* Title and message are UTF-8 (the adapter converts the client's code page). */
bf_sys_alert_answer bf_sys_alert(const char *title, const char *message,
                                 bf_sys_alert_style style, bf_sys_alert_buttons buttons);

/*
 * bf_sys_alert for the client's own text: title and message are bytes in a
 * Windows code page (949, 1252, ...; 0 means the user code page,
 * bf_file_user_codepage) and are converted to UTF-8 by Foundation. Text the
 * code page does not decode is read as Windows-1252, and failing that as Mac
 * Roman, so it is never dropped. NULL is "". Same answers as bf_sys_alert.
 */
bf_sys_alert_answer bf_sys_alert_codepage(unsigned codepage, const char *title, const char *message,
                                          bf_sys_alert_style style, bf_sys_alert_buttons buttons);

/* 1 if alerts are shown, 0 if they are logged (see above). */
int bf_sys_alerts_enabled(void);

/* Opens a URL (http, https, mailto, ...) in its application. 1 or 0. Thread: any. */
int bf_sys_open_url(const char *url);
/* Opens a file or folder (absolute UTF-8 path) in its application. 1 or 0. Thread: any. */
int bf_sys_open_path(const char *path);

#ifdef __cplusplus
}
#endif

#endif
