// Shared pieces of the win32 adapter: the kernel object handles, the last
// error, errno translation and the code pages.
#ifndef BF_WIN32_INTERNAL_H
#define BF_WIN32_INTERNAL_H

#include <windows.h>

#include <stdint.h>
#include <string>

#include "bf_sys.h"
#include "bf_trace.h"

namespace bf_win32 {

// A kernel object behind a HANDLE. Every live object is registered, so that
// a handle the client passes is checked before use: the client closes
// handles it never opened (FindClose(INVALID_HANDLE_VALUE), RegCloseKey of a
// key whose open failed).
enum Kind { kFind = 1 };

struct Object {
	explicit Object(Kind k) : kind(k) {}
	virtual ~Object() {}
	Kind kind;
};

// Registers a new object and returns it as a HANDLE.
HANDLE Register(Object *object);
// The live object of that kind behind a handle, or NULL (ERROR_INVALID_HANDLE set).
Object *Lookup(HANDLE handle, Kind kind);
// Unregisters and deletes; false if the handle is not live.
bool Release(HANDLE handle);

// CP_ACP and the other symbolic code pages as a number (CP_ACP follows
// bf_file_user_codepage, which the client sets per service: 949 or 1252).
int Codepage(UINT codepage);

// The client's bytes (CP_ACP) to UTF-8 and back. len < 0: NUL-terminated.
std::string ToUtf8(const char *text, int len = -1);
std::string FromUtf8(const std::string &utf8);

// 100-ns intervals since 1601 for seconds since 1970, and back.
uint64_t FileTimeFromUnix(int64_t seconds);

}  // namespace bf_win32

#endif
