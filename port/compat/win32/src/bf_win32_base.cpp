// kernel32 basics on port/native/sys and port/native/text: handles and the
// last error, code pages and strings, memory, module and process functions,
// system information, and the functions the client calls but the port does
// not have (DLLs, processes, CryptoAPI), which fail as the client tolerates.
#include "bf_win32_internal.h"

#include <crt_externs.h>
#include <errno.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <malloc/malloc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysctl.h>
#include <time.h>
#include <unistd.h>

#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "bf_text.h"

namespace bf_win32 {

namespace {

// Never destroyed: the client's static objects (CFileMgr's archives) close
// their handles from their destructors at exit, after this file's statics.
std::mutex &g_handleLock = *new std::mutex;
std::unordered_set<Object *> &Handles()
{
	static std::unordered_set<Object *> *handles = new std::unordered_set<Object *>;
	return *handles;
}

thread_local DWORD t_lastError = 0;

}  // namespace

HANDLE Register(Object *object)
{
	std::lock_guard<std::mutex> guard(g_handleLock);
	Handles().insert(object);
	return (HANDLE)object;
}

Object *Lookup(HANDLE handle, Kind kind)
{
	std::lock_guard<std::mutex> guard(g_handleLock);
	Object *object = (Object *)handle;
	if (Handles().count(object) && object->kind == kind)
		return object;
	t_lastError = ERROR_INVALID_HANDLE;
	return NULL;
}

bool Release(HANDLE handle)
{
	Object *object = (Object *)handle;
	{
		std::lock_guard<std::mutex> guard(g_handleLock);
		if (!Handles().erase(object))
			return false;
	}
	delete object;
	return true;
}

int Codepage(UINT codepage)
{
	switch (codepage) {
	case CP_ACP:
	case CP_OEMCP:
	case CP_MACCP:
	case CP_THREAD_ACP: return (int)bf_file_user_codepage();
	default:            return (int)codepage;
	}
}

std::string ToUtf8(const char *text, int len)
{
	if (!text)
		return std::string();
	if (len < 0)
		len = (int)strlen(text);
	int cp = Codepage(CP_ACP);
	int n = bf_text_to_utf8(cp, text, len, NULL, 0);
	if (n <= 0)
		return n == 0 ? std::string() : std::string(text, (size_t)len);
	std::string out((size_t)n, '\0');
	bf_text_to_utf8(cp, text, len, &out[0], n);
	return out;
}

std::string FromUtf8(const std::string &utf8)
{
	int cp = Codepage(CP_ACP);
	int n = bf_text_from_utf8(cp, utf8.data(), (int)utf8.size(), NULL, 0);
	if (n <= 0)
		return n == 0 ? std::string() : utf8;
	std::string out((size_t)n, '\0');
	bf_text_from_utf8(cp, utf8.data(), (int)utf8.size(), &out[0], n);
	return out;
}

uint64_t FileTimeFromUnix(int64_t seconds)
{
	return (uint64_t)(seconds + 11644473600LL) * 10000000ULL;
}

}  // namespace bf_win32

using namespace bf_win32;

// ---- Last error --------------------------------------------------------------

DWORD WINAPI GetLastError(void)
{
	return t_lastError;
}

VOID WINAPI SetLastError(DWORD error)
{
	t_lastError = error;
}

// CloseHandle for every kind of kernel object; the kinds' own clean-up is in
// their destructors (folder listings). The pseudo-handle is ignored.
BOOL WINAPI CloseHandle(HANDLE handle)
{
	if (handle == GetCurrentProcess())
		return TRUE;
	{
		std::lock_guard<std::mutex> guard(g_handleLock);
		Object *object = (Object *)handle;
		if (!Handles().count(object) || object->kind == kFind) {
			t_lastError = ERROR_INVALID_HANDLE;
			return FALSE;
		}
	}
	return Release(handle) ? TRUE : FALSE;
}

// ---- Code pages and strings ---------------------------------------------------

UINT WINAPI GetACP(void)
{
	return (UINT)Codepage(CP_ACP);
}

BOOL WINAPI IsDBCSLeadByte(BYTE byte)
{
	return bf_text_is_lead_byte(Codepage(CP_ACP), byte) ? TRUE : FALSE;
}

BOOL WINAPI IsDBCSLeadByteEx(UINT codepage, BYTE byte)
{
	return bf_text_is_lead_byte(Codepage(codepage), byte) ? TRUE : FALSE;
}

// The counts include the terminating NUL when len is -1, as on Windows; a
// too-small output buffer fails with ERROR_INSUFFICIENT_BUFFER.
INT WINAPI MultiByteToWideChar(UINT codepage, DWORD flags, LPCSTR text, INT len, LPWSTR out, INT out_max)
{
	(void)flags;
	if (!text || len == 0 || len < -1 || out_max < 0 || (out_max > 0 && !out)) {
		t_lastError = ERROR_INVALID_PARAMETER;
		return 0;
	}
	if (len == -1)
		len = (int)strlen(text) + 1;
	int cp = Codepage(codepage);
	int n = bf_text_to_utf16(cp, text, len, NULL, 0);
	if (n < 0) {
		t_lastError = ERROR_INVALID_PARAMETER;
		return 0;
	}
	if (out_max == 0)
		return n;
	if (n > out_max) {
		bf_text_to_utf16(cp, text, len, (uint16_t *)out, out_max);
		t_lastError = ERROR_INSUFFICIENT_BUFFER;
		return 0;
	}
	return bf_text_to_utf16(cp, text, len, (uint16_t *)out, out_max);
}

INT WINAPI WideCharToMultiByte(UINT codepage, DWORD flags, LPCWSTR text, INT len, LPSTR out, INT out_max,
                               LPCSTR default_char, LPBOOL used_default)
{
	(void)flags;
	(void)default_char;
	if (!text || len == 0 || len < -1 || out_max < 0 || (out_max > 0 && !out)) {
		t_lastError = ERROR_INVALID_PARAMETER;
		return 0;
	}
	const uint16_t *wide = (const uint16_t *)text;
	if (len == -1) {
		len = 0;
		while (wide[len])
			++len;
		++len;
	}
	if (used_default)
		*used_default = FALSE;
	int cp = Codepage(codepage);
	int n = bf_text_from_utf16(cp, wide, len, NULL, 0);
	if (n < 0) {
		t_lastError = ERROR_INVALID_PARAMETER;
		return 0;
	}
	if (out_max == 0)
		return n;
	if (n > out_max) {
		bf_text_from_utf16(cp, wide, len, out, out_max);
		t_lastError = ERROR_INSUFFICIENT_BUFFER;
		return 0;
	}
	return bf_text_from_utf16(cp, wide, len, out, out_max);
}

INT WINAPI MulDiv(INT a, INT b, INT c)
{
	if (c == 0)
		return -1;
	int64_t product = (int64_t)a * b;
	// Rounded half away from zero, as Windows does.
	int64_t result = ((product < 0) != (c < 0)) ? (product - c / 2) / c : (product + c / 2) / c;
	if (result > INT_MAX || result < INT_MIN)
		return -1;
	return (INT)result;
}

INT WINAPI lstrcmpiA(LPCSTR a, LPCSTR b)
{
	if (!a || !b)
		return a ? 1 : b ? -1 : 0;
	int r = strcasecmp(a, b);
	return r < 0 ? -1 : r > 0 ? 1 : 0;
}

LPSTR WINAPI lstrcatA(LPSTR dst, LPCSTR src)
{
	return dst && src ? strcat(dst, src) : NULL;
}

LPSTR WINAPI lstrcpyA(LPSTR dst, LPCSTR src)
{
	return dst && src ? strcpy(dst, src) : NULL;
}

INT WINAPI lstrlenA(LPCSTR text)
{
	return text ? (INT)strlen(text) : 0;
}

VOID WINAPI OutputDebugStringA(LPCSTR text)
{
	if (!text)
		return;
	size_t n = strlen(text);
	while (n > 0 && (text[n - 1] == '\n' || text[n - 1] == '\r'))
		--n;
	BF_TRACE(BFT_W32, "OutputDebugString: %.*s", (int)n, text);
}

// Only FROM_SYSTEM (a message for an error code) is supported, with or
// without ALLOCATE_BUFFER; the inserts of a FROM_STRING message are not.
DWORD WINAPI FormatMessageA(DWORD flags, LPCVOID source, DWORD id, DWORD lang, LPSTR buffer, DWORD size,
                            __ms_va_list *args)
{
	(void)source;
	(void)lang;
	(void)args;
	if (!(flags & FORMAT_MESSAGE_FROM_SYSTEM)) {
		BF_STUB();
		t_lastError = ERROR_NOT_SUPPORTED;
		return 0;
	}
	char text[128];
	snprintf(text, sizeof(text), "Error %u.\r\n", (unsigned)id);
	size_t n = strlen(text);
	if (flags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
		HLOCAL memory = LocalAlloc(LMEM_FIXED, n + 1 > size ? n + 1 : size);
		if (!memory)
			return 0;
		memcpy(memory, text, n + 1);
		*(LPSTR *)buffer = (LPSTR)memory;
		return (DWORD)n;
	}
	if (!buffer || size <= n) {
		t_lastError = ERROR_INSUFFICIENT_BUFFER;
		return 0;
	}
	memcpy(buffer, text, n + 1);
	return (DWORD)n;
}

// ---- Memory -----------------------------------------------------------------------
//
// GlobalAlloc and LocalAlloc: the handle is the memory itself (as for
// GMEM_FIXED), preceded by a header with the size, so GlobalSize works for
// the clipboard adapter. GlobalLock returns the handle.
namespace {
const size_t kHeader = 16;

void *BlockAlloc(UINT flags, SIZE_T size, UINT zero_flag)
{
	char *base = (char *)((flags & zero_flag) ? calloc(1, kHeader + size) : malloc(kHeader + size));
	if (!base) {
		t_lastError = ERROR_NOT_ENOUGH_MEMORY;
		return NULL;
	}
	memcpy(base, &size, sizeof(size));
	return base + kHeader;
}

void BlockFree(void *memory)
{
	if (memory)
		free((char *)memory - kHeader);
}

SIZE_T BlockSize(void *memory)
{
	SIZE_T size = 0;
	if (memory)
		memcpy(&size, (char *)memory - kHeader, sizeof(size));
	return size;
}
}  // namespace

HGLOBAL WINAPI GlobalAlloc(UINT flags, SIZE_T size)
{
	return (HGLOBAL)BlockAlloc(flags, size, GMEM_ZEROINIT);
}

LPVOID WINAPI GlobalLock(HGLOBAL memory)
{
	return (LPVOID)memory;
}

BOOL WINAPI GlobalUnlock(HGLOBAL memory)
{
	(void)memory;
	t_lastError = NO_ERROR;
	return FALSE;   // "no longer locked", as for a fixed block
}

HGLOBAL WINAPI GlobalFree(HGLOBAL memory)
{
	BlockFree((void *)memory);
	return NULL;
}

SIZE_T WINAPI GlobalSize(HGLOBAL memory)
{
	return BlockSize((void *)memory);
}

HLOCAL WINAPI LocalAlloc(UINT flags, SIZE_T size)
{
	return (HLOCAL)BlockAlloc(flags, size, LMEM_ZEROINIT);
}

LPVOID WINAPI LocalLock(HLOCAL memory)
{
	return (LPVOID)memory;
}

HLOCAL WINAPI LocalFree(HLOCAL memory)
{
	BlockFree((void *)memory);
	return NULL;
}

VOID WINAPI GlobalMemoryStatus(LPMEMORYSTATUS status)
{
	if (!status)
		return;
	uint64_t physical = 0;
	size_t length = sizeof(physical);
	sysctlbyname("hw.memsize", &physical, &length, NULL, 0);
	memset(status, 0, sizeof(*status));
	status->dwLength = sizeof(*status);
	status->dwMemoryLoad = 50;
	status->dwTotalPhys = (SIZE_T)physical;
	status->dwAvailPhys = (SIZE_T)(physical / 2);
	status->dwTotalPageFile = (SIZE_T)physical * 2;
	status->dwAvailPageFile = (SIZE_T)physical;
	status->dwTotalVirtual = (SIZE_T)1 << 47;
	status->dwAvailVirtual = (SIZE_T)1 << 46;
}

// ---- Modules and the process -------------------------------------------------------

namespace {
// What GetModuleHandle(NULL) returns; any value other than NULL and the DLLs'.
char g_mainModule;

const std::string &ExecutablePath()
{
	static std::string path = [] {
		uint32_t size = 0;
		_NSGetExecutablePath(NULL, &size);
		std::vector<char> raw(size + 1);
		if (_NSGetExecutablePath(raw.data(), &size) != 0)
			return std::string();
		char real[PATH_MAX];
		return std::string(realpath(raw.data(), real) ? real : raw.data());
	}();
	return path;
}

// Copies text with Windows' truncation rule for the "A" path functions:
// returns the length without the NUL when it fits, else the size needed
// including the NUL (GetCurrentDirectory) and nothing is copied.
UINT CopyPath(const std::string &text, LPSTR buffer, UINT size)
{
	if (!buffer || size <= text.size())
		return (UINT)text.size() + 1;
	memcpy(buffer, text.c_str(), text.size() + 1);
	return (UINT)text.size();
}
}  // namespace

// The executable itself (the client hashes it in CLoginMode::GenerateExeHash):
// an absolute POSIX path, which the file functions take as it is.
DWORD WINAPI GetModuleFileNameA(HMODULE module, LPSTR buffer, DWORD size)
{
	(void)module;   // NULL, g_hInstance or GetModuleHandle(NULL): all the executable
	const std::string &path = ExecutablePath();
	if (!buffer || size == 0) {
		t_lastError = ERROR_INSUFFICIENT_BUFFER;
		return 0;
	}
	if (path.size() >= size) {
		memcpy(buffer, path.c_str(), size - 1);
		buffer[size - 1] = 0;
		t_lastError = ERROR_INSUFFICIENT_BUFFER;
		return size;
	}
	memcpy(buffer, path.c_str(), path.size() + 1);
	t_lastError = ERROR_SUCCESS;
	return (DWORD)path.size();
}

HMODULE WINAPI GetModuleHandleA(LPCSTR name)
{
	if (!name)
		return (HMODULE)&g_mainModule;
	t_lastError = ERROR_MOD_NOT_FOUND;
	return NULL;
}

// There are no DLLs: every LoadLibrary fails as for a missing file, which the
// client's remaining callers (GameGuard and keyboard-protection loaders in
// base/FindHack.cpp) treat as "not installed".
HMODULE WINAPI LoadLibraryA(LPCSTR name)
{
	BF_TRACE(BFT_W32, "LoadLibrary(%s): no DLLs in the port", name ? name : "(null)");
	t_lastError = ERROR_MOD_NOT_FOUND;
	return NULL;
}

FARPROC WINAPI GetProcAddress(HMODULE module, LPCSTR name)
{
	(void)module;
	(void)name;
	t_lastError = ERROR_PROC_NOT_FOUND;
	return NULL;
}

BOOL WINAPI FreeLibrary(HMODULE module)
{
	(void)module;
	return TRUE;
}

// The game folder: the client's working folder, whose path it keeps in
// g_baseDir3 and puts in front of ScreenShot\..., Chat\..., _tmpskin_\...
// (bf_file sends those to the write locations).
UINT WINAPI GetCurrentDirectoryA(UINT size, LPSTR buffer)
{
	const char *game = bf_file_location(BF_LOCATION_GAME);
	return CopyPath(game ? game : "", buffer, size);
}

extern "C" char *_getcwd(char *buffer, int size)
{
	const char *game = bf_file_location(BF_LOCATION_GAME);
	std::string path = game ? game : "";
	if (!buffer) {
		size_t n = (size_t)size > path.size() + 1 ? (size_t)size : path.size() + 1;
		buffer = (char *)malloc(n);
		if (!buffer) {
			errno = ENOMEM;
			return NULL;
		}
	} else if (size <= 0 || (size_t)size <= path.size()) {
		errno = ERANGE;
		return NULL;
	}
	memcpy(buffer, path.c_str(), path.size() + 1);
	return buffer;
}

LPSTR WINAPI GetCommandLineA(void)
{
	static std::string line = [] {
		std::string out;
		int argc = *_NSGetArgc();
		char **argv = *_NSGetArgv();
		for (int i = 0; i < argc; ++i) {
			if (i)
				out += ' ';
			bool quote = !argv[i][0] || strpbrk(argv[i], " \t");
			if (quote)
				out += '"';
			out += argv[i];
			if (quote)
				out += '"';
		}
		return out;
	}();
	return (LPSTR)line.c_str();
}

VOID WINAPI ExitProcess(DWORD code)
{
	exit((int)code);
}

HANDLE WINAPI GetCurrentProcess(void)
{
	return (HANDLE)~(ULONG_PTR)0;
}

DWORD WINAPI GetCurrentProcessId(void)
{
	return (DWORD)getpid();
}

// Other processes are not inspected (base/FindHack.cpp's module scan).
HANDLE WINAPI OpenProcess(DWORD access, BOOL inherit, DWORD id)
{
	(void)access;
	(void)inherit;
	(void)id;
	BF_STUB();
	t_lastError = ERROR_ACCESS_DENIED;
	return NULL;
}

BOOL WINAPI SetPriorityClass(HANDLE process, DWORD priority)
{
	(void)process;
	(void)priority;
	return TRUE;
}

// The patcher's restart (WinMain's ExcuteProgram) and launching other
// programs: not in the port. Fails; the caller then has no process handle.
BOOL WINAPI CreateProcessA(LPCSTR application, LPSTR command, LPSECURITY_ATTRIBUTES process_attributes,
                           LPSECURITY_ATTRIBUTES thread_attributes, BOOL inherit, DWORD flags, LPVOID environment,
                           LPCSTR directory, LPSTARTUPINFOA startup, LPPROCESS_INFORMATION information)
{
	(void)process_attributes;
	(void)thread_attributes;
	(void)inherit;
	(void)flags;
	(void)environment;
	(void)directory;
	(void)startup;
	BF_STUB();
	bf_tracef(BFT_ERR, "CreateProcess(%s %s): not supported", application ? application : "",
	          command ? command : "");
	if (information)
		memset(information, 0, sizeof(*information));
	t_lastError = ERROR_NOT_SUPPORTED;
	return FALSE;
}

// The crash dialog of the Windows client; the port writes a backtrace from
// its own signal handler instead. The filter is kept and returned, not called.
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER filter)
{
	static LPTOP_LEVEL_EXCEPTION_FILTER previous = NULL;
	LPTOP_LEVEL_EXCEPTION_FILTER old = previous;
	previous = filter;
	return old;
}

// ---- System information ------------------------------------------------------------

// Windows XP SP2, what the 2008 client was written for.
BOOL WINAPI GetVersionExA(OSVERSIONINFOA *info)
{
	if (!info || (info->dwOSVersionInfoSize != sizeof(OSVERSIONINFOA)
	              && info->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXA))) {
		t_lastError = ERROR_INSUFFICIENT_BUFFER;
		return FALSE;
	}
	DWORD size = info->dwOSVersionInfoSize;
	memset(info, 0, size);
	info->dwOSVersionInfoSize = size;
	info->dwMajorVersion = 5;
	info->dwMinorVersion = 1;
	info->dwBuildNumber = 2600;
	info->dwPlatformId = VER_PLATFORM_WIN32_NT;
	strcpy(info->szCSDVersion, "Service Pack 2");
	if (size == sizeof(OSVERSIONINFOEXA)) {
		OSVERSIONINFOEXA *ex = (OSVERSIONINFOEXA *)info;
		ex->wServicePackMajor = 2;
		ex->wProductType = VER_NT_WORKSTATION;
	}
	return TRUE;
}

// A Windows path; files under it are not found (only base/FindHack.cpp asks).
UINT WINAPI GetSystemDirectoryA(LPSTR buffer, UINT size)
{
	return CopyPath("C:\\WINDOWS\\system32", buffer, size);
}

// English: the language folder of the client's scripts (CLanguage picks
// italian\, german\, ... by it; english\ is what the port's tables use).
LANGID WINAPI GetUserDefaultLangID(void)
{
	return MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
}

// ---- CryptoAPI ---------------------------------------------------------------------
// Only base/FindHack.cpp's protection code calls these; there is no provider.

BOOL WINAPI CryptAcquireContextA(HCRYPTPROV *provider, LPCSTR container, LPCSTR name, DWORD type, DWORD flags)
{
	(void)container;
	(void)name;
	(void)type;
	(void)flags;
	BF_STUB();
	if (provider)
		*provider = 0;
	t_lastError = (DWORD)NTE_PROV_TYPE_NOT_DEF;
	return FALSE;
}

BOOL WINAPI CryptReleaseContext(HCRYPTPROV provider, DWORD flags)
{
	(void)provider;
	(void)flags;
	return TRUE;
}

BOOL WINAPI CryptCreateHash(HCRYPTPROV provider, ALG_ID algorithm, HCRYPTKEY key, DWORD flags, HCRYPTHASH *hash)
{
	(void)provider;
	(void)algorithm;
	(void)key;
	(void)flags;
	BF_STUB();
	if (hash)
		*hash = 0;
	t_lastError = ERROR_INVALID_HANDLE;
	return FALSE;
}

BOOL WINAPI CryptHashData(HCRYPTHASH hash, const BYTE *data, DWORD size, DWORD flags)
{
	(void)hash;
	(void)data;
	(void)size;
	(void)flags;
	BF_STUB();
	t_lastError = ERROR_INVALID_HANDLE;
	return FALSE;
}

BOOL WINAPI CryptDeriveKey(HCRYPTPROV provider, ALG_ID algorithm, HCRYPTHASH hash, DWORD flags, HCRYPTKEY *key)
{
	(void)provider;
	(void)algorithm;
	(void)hash;
	(void)flags;
	BF_STUB();
	if (key)
		*key = 0;
	t_lastError = ERROR_INVALID_HANDLE;
	return FALSE;
}

BOOL WINAPI CryptDecrypt(HCRYPTKEY key, HCRYPTHASH hash, BOOL final, DWORD flags, BYTE *data, DWORD *size)
{
	(void)key;
	(void)hash;
	(void)final;
	(void)flags;
	(void)data;
	(void)size;
	BF_STUB();
	t_lastError = ERROR_INVALID_HANDLE;
	return FALSE;
}

BOOL WINAPI CryptDestroyKey(HCRYPTKEY key)
{
	(void)key;
	return TRUE;
}

BOOL WINAPI CryptDestroyHash(HCRYPTHASH hash)
{
	(void)hash;
	return TRUE;
}
