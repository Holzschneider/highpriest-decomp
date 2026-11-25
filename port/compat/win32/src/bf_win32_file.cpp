// kernel32's folder listing on bf_file: what is left of the file adapter
// after M11.3, for base/FindHack.cpp (FindFirstFile + FindClose), whose
// call sites move with its own M11 step. The client's other file calls go to
// port/native/sys (bf_file.h) directly; the C library's file names
// (bf_crt.h) are port/client/src/bf_crt.cpp's.
#include "bf_win32_internal.h"

#include <string.h>

using namespace bf_win32;

namespace {

struct Find : Object {
	Find() : Object(kFind) {}
	~Find() override { bf_file_find_close(find); }
	bf_file_find *find = nullptr;
};

void FillFindData(const bf_file_info &info, WIN32_FIND_DATAA *out)
{
	memset(out, 0, sizeof(*out));
	DWORD attributes = info.is_folder ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE;
	if (info.read_only)
		attributes |= FILE_ATTRIBUTE_READONLY;
	out->dwFileAttributes = attributes;
	uint64_t t = FileTimeFromUnix(info.modified);
	out->ftCreationTime.dwLowDateTime = out->ftLastAccessTime.dwLowDateTime = out->ftLastWriteTime.dwLowDateTime =
		(DWORD)t;
	out->ftCreationTime.dwHighDateTime = out->ftLastAccessTime.dwHighDateTime = out->ftLastWriteTime.dwHighDateTime =
		(DWORD)(t >> 32);
	out->nFileSizeHigh = (DWORD)(info.size >> 32);
	out->nFileSizeLow = (DWORD)info.size;
	strlcpy(out->cFileName, info.name, sizeof(out->cFileName));
}

}  // namespace

// ---- Folder listings -------------------------------------------------------------
// "." and ".." are never listed (the client skips them anyway).

HANDLE WINAPI FindFirstFileA(LPCSTR pattern, LPWIN32_FIND_DATAA data)
{
	if (!pattern || !data) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return INVALID_HANDLE_VALUE;
	}
	bf_file_info info;
	bf_file_find *find = bf_file_find_first(pattern, &info);
	if (!find) {
		SetLastError(ERROR_FILE_NOT_FOUND);
		return INVALID_HANDLE_VALUE;
	}
	FillFindData(info, data);
	Find *object = new Find;
	object->find = find;
	return Register(object);
}

BOOL WINAPI FindClose(HANDLE handle)
{
	if (!Lookup(handle, kFind))
		return FALSE;   // also FindClose(INVALID_HANDLE_VALUE)
	Release(handle);
	return TRUE;
}
