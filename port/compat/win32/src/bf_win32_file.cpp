// kernel32's files, folder listings and file mappings on bf_file, and the
// C library's file functions on client paths (bf_crt.h).
//
// Every path goes through bf_file: reads find the file in the game folder
// (backslashes, any case, CP949) or in a write location first; writes go to
// the write locations and are refused in the game folder with
// ERROR_ACCESS_DENIED (errno EACCES for the C functions).
#include "bf_win32_internal.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <mutex>
#include <string>
#include <vector>

using namespace bf_win32;

namespace {

struct File : Object {
	File() : Object(kFile) {}
	~File() override { bf_file_close(file); }
	bf_file *file = nullptr;
};

// The whole file is mapped once; every view is a pointer into it. The
// mapping lives until its handle is closed and its last view unmapped, as a
// Win32 section object does.
struct Mapping : Object {
	Mapping() : Object(kMapping) {}
	~Mapping() override;
	bf_file_mapping *mapping = nullptr;
	const char *data = nullptr;
	uint64_t size = 0;
};

struct Find : Object {
	Find() : Object(kFind) {}
	~Find() override { bf_file_find_close(find); }
	bf_file_find *find = nullptr;
};

// Mappings whose views are still in use, also after CloseHandle.
// Never destroyed: the client's static objects (CFileMgr's archives) close
// their handles from their destructors at exit, after this file's statics.
std::mutex &g_mappingLock = *new std::mutex;
struct Section {
	bf_file_mapping *mapping;
	const char *data;
	uint64_t size;
	int views;
	bool closed;
};
std::vector<Section> &Sections()
{
	static std::vector<Section> *sections = new std::vector<Section>;
	return *sections;
}

void DropSection(std::vector<Section>::iterator it)
{
	bf_file_unmap(it->mapping);
	Sections().erase(it);
}

Mapping::~Mapping()
{
	std::lock_guard<std::mutex> guard(g_mappingLock);
	for (std::vector<Section>::iterator it = Sections().begin(); it != Sections().end(); ++it) {
		if (it->mapping == mapping) {
			it->closed = true;
			if (it->views == 0)
				DropSection(it);
			return;
		}
	}
}

const uint64_t kGranularity = 0x10000;   // GetSystemInfo's dwAllocationGranularity

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

// ---- Files ------------------------------------------------------------------------

HANDLE WINAPI CreateFileA(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES attributes,
                          DWORD disposition, DWORD flags, HANDLE template_file)
{
	(void)share;
	(void)attributes;
	(void)flags;
	(void)template_file;
	if (!name || disposition < CREATE_NEW || disposition > TRUNCATE_EXISTING) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return INVALID_HANDLE_VALUE;
	}
	bool read = (access & (GENERIC_READ | GENERIC_ALL | FILE_READ_DATA)) != 0;
	bool write = (access & (GENERIC_WRITE | GENERIC_ALL | FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0
	             || disposition != OPEN_EXISTING;
	unsigned open = (read ? BF_FILE_READ : 0) | (write ? BF_FILE_WRITE : 0);
	bool existed = false;
	if (disposition == CREATE_ALWAYS || disposition == OPEN_ALWAYS)
		existed = bf_file_write_location(name) != BF_LOCATION_GAME && bf_file_stat(name, NULL);
	switch (disposition) {
	case CREATE_NEW:        open |= BF_FILE_CREATE | BF_FILE_EXCLUSIVE; break;
	case CREATE_ALWAYS:     open |= BF_FILE_CREATE | BF_FILE_TRUNCATE; break;
	case OPEN_EXISTING:     break;
	case OPEN_ALWAYS:       open |= BF_FILE_CREATE; break;
	case TRUNCATE_EXISTING: open |= BF_FILE_TRUNCATE; break;
	}
	errno = 0;
	bf_file *file = bf_file_open(name, open);
	if (!file) {
		DWORD error = ErrorFromErrno(errno ? errno : ENOENT);
		if (error == ERROR_ALREADY_EXISTS)
			error = ERROR_FILE_EXISTS;   // CREATE_NEW on an existing file
		SetLastError(error);
		BF_TRACE(BFT_W32, "CreateFile(%s, %s): error %u", name, write ? "write" : "read", (unsigned)error);
		return INVALID_HANDLE_VALUE;
	}
	File *object = new File;
	object->file = file;
	SetLastError(existed ? ERROR_ALREADY_EXISTS : ERROR_SUCCESS);
	return Register(object);
}

BOOL WINAPI ReadFile(HANDLE handle, LPVOID buffer, DWORD size, LPDWORD done, LPOVERLAPPED overlapped)
{
	if (done)
		*done = 0;
	File *file = (File *)Lookup(handle, kFile);
	if (!file)
		return FALSE;
	if (overlapped) {
		BF_STUB();
		SetLastError(ERROR_NOT_SUPPORTED);
		return FALSE;
	}
	int64_t n = bf_file_read(file->file, buffer, size);
	if (n < 0) {
		SetErrorFromErrno();
		return FALSE;
	}
	if (done)
		*done = (DWORD)n;
	return TRUE;
}

BOOL WINAPI WriteFile(HANDLE handle, LPCVOID buffer, DWORD size, LPDWORD done, LPOVERLAPPED overlapped)
{
	if (done)
		*done = 0;
	File *file = (File *)Lookup(handle, kFile);
	if (!file)
		return FALSE;
	if (overlapped) {
		BF_STUB();
		SetLastError(ERROR_NOT_SUPPORTED);
		return FALSE;
	}
	int64_t n = bf_file_write(file->file, buffer, size);
	if (n < 0) {
		SetErrorFromErrno();
		return FALSE;
	}
	if (done)
		*done = (DWORD)n;
	return TRUE;
}

DWORD WINAPI SetFilePointer(HANDLE handle, LONG distance, LPLONG high, DWORD method)
{
	File *file = (File *)Lookup(handle, kFile);
	if (!file)
		return INVALID_SET_FILE_POINTER;
	int64_t offset = high ? (int64_t)(((uint64_t)(uint32_t)*high << 32) | (uint32_t)distance) : (int64_t)distance;
	int64_t base = method == FILE_BEGIN ? 0 : method == FILE_CURRENT ? bf_file_tell(file->file)
	             : method == FILE_END   ? bf_file_size(file->file) : -1;
	if (base < 0) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return INVALID_SET_FILE_POINTER;
	}
	if (base + offset < 0) {
		SetLastError(ERROR_NEGATIVE_SEEK);
		return INVALID_SET_FILE_POINTER;
	}
	int64_t at = bf_file_seek(file->file, base + offset, 0);
	if (at < 0) {
		SetErrorFromErrno();
		return INVALID_SET_FILE_POINTER;
	}
	if (high)
		*high = (LONG)(at >> 32);
	SetLastError(ERROR_SUCCESS);
	return (DWORD)at;
}

DWORD WINAPI GetFileSize(HANDLE handle, LPDWORD high)
{
	File *file = (File *)Lookup(handle, kFile);
	if (!file)
		return INVALID_FILE_SIZE;
	int64_t size = bf_file_size(file->file);
	if (size < 0) {
		SetErrorFromErrno();
		return INVALID_FILE_SIZE;
	}
	if (high)
		*high = (DWORD)((uint64_t)size >> 32);
	SetLastError(ERROR_SUCCESS);
	return (DWORD)size;
}

BOOL WINAPI SetEndOfFile(HANDLE handle)
{
	File *file = (File *)Lookup(handle, kFile);
	if (!file)
		return FALSE;
	if (bf_file_truncate_here(file->file) != 0) {
		SetErrorFromErrno();
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI FlushFileBuffers(HANDLE handle)
{
	return Lookup(handle, kFile) ? TRUE : FALSE;
}

BOOL WINAPI DeleteFileA(LPCSTR name)
{
	if (bf_file_remove(name) != 0) {
		SetErrorFromErrno();
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI MoveFileA(LPCSTR from, LPCSTR to)
{
	if (bf_file_stat(to, NULL)) {
		SetLastError(ERROR_ALREADY_EXISTS);
		return FALSE;
	}
	if (bf_file_rename(from, to) != 0) {
		SetErrorFromErrno();
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI CreateDirectoryA(LPCSTR name, LPSECURITY_ATTRIBUTES attributes)
{
	(void)attributes;
	int r = bf_file_make_folder(name);
	if (r == 1)
		return TRUE;
	if (r == 0)
		SetLastError(ERROR_ALREADY_EXISTS);
	else
		SetErrorFromErrno();
	return FALSE;
}

DWORD WINAPI GetFileAttributesA(LPCSTR name)
{
	bf_file_info info;
	if (!bf_file_stat(name, &info)) {
		SetLastError(ERROR_FILE_NOT_FOUND);
		return INVALID_FILE_ATTRIBUTES;
	}
	DWORD attributes = info.is_folder ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE;
	if (info.read_only)
		attributes |= FILE_ATTRIBUTE_READONLY;
	return attributes;
}

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

BOOL WINAPI FindNextFileA(HANDLE handle, LPWIN32_FIND_DATAA data)
{
	Find *find = (Find *)Lookup(handle, kFind);
	if (!find || !data)
		return FALSE;
	bf_file_info info;
	if (!bf_file_find_next(find->find, &info)) {
		SetLastError(ERROR_NO_MORE_FILES);
		return FALSE;
	}
	FillFindData(info, data);
	return TRUE;
}

BOOL WINAPI FindClose(HANDLE handle)
{
	if (!Lookup(handle, kFind))
		return FALSE;   // also FindClose(INVALID_HANDLE_VALUE), which base/Util.cpp and skin.cpp do
	Release(handle);
	return TRUE;
}

// ---- File mappings ------------------------------------------------------------------
// Read-only mappings of a whole file (the GRF archives, base/GPak.cpp).

HANDLE WINAPI CreateFileMappingA(HANDLE file_handle, LPSECURITY_ATTRIBUTES attributes, DWORD protect,
                                 DWORD size_high, DWORD size_low, LPCSTR name)
{
	(void)attributes;
	(void)size_high;
	(void)size_low;
	(void)name;   // named sections are not shared between processes here
	if ((protect & 0xFF) != PAGE_READONLY && (protect & 0xFF) != PAGE_WRITECOPY) {
		BF_STUB();
		SetLastError(ERROR_NOT_SUPPORTED);
		return NULL;
	}
	File *file = (File *)Lookup(file_handle, kFile);
	if (!file) {
		if (file_handle == INVALID_HANDLE_VALUE)
			BF_STUB();   // a section backed by the paging file
		return NULL;
	}
	bf_file_mapping *mapping = bf_file_map_open_file(file->file);
	if (!mapping) {
		SetErrorFromErrno();
		return NULL;
	}
	if (bf_file_mapping_size(mapping) == 0) {
		bf_file_unmap(mapping);
		SetLastError(ERROR_FILE_INVALID);
		return NULL;
	}
	Mapping *object = new Mapping;
	object->mapping = mapping;
	object->data = (const char *)bf_file_mapping_data(mapping);
	object->size = bf_file_mapping_size(mapping);
	{
		std::lock_guard<std::mutex> guard(g_mappingLock);
		Sections().push_back(Section{mapping, object->data, object->size, 0, false});
	}
	BF_TRACE(BFT_W32, "CreateFileMapping(%s): %llu bytes", bf_file_path(file->file),
	         (unsigned long long)object->size);
	SetLastError(ERROR_SUCCESS);
	return Register(object);
}

LPVOID WINAPI MapViewOfFile(HANDLE handle, DWORD access, DWORD offset_high, DWORD offset_low, SIZE_T bytes)
{
	Mapping *mapping = (Mapping *)Lookup(handle, kMapping);
	if (!mapping)
		return NULL;
	if (access & (FILE_MAP_WRITE & ~FILE_MAP_READ)) {
		SetLastError(ERROR_ACCESS_DENIED);
		return NULL;
	}
	uint64_t offset = ((uint64_t)offset_high << 32) | offset_low;
	if (offset % kGranularity) {
		SetLastError(ERROR_MAPPED_ALIGNMENT);
		return NULL;
	}
	if (offset >= mapping->size || (bytes && bytes > mapping->size - offset)) {
		SetLastError(ERROR_ACCESS_DENIED);
		return NULL;
	}
	std::lock_guard<std::mutex> guard(g_mappingLock);
	for (Section &section : Sections()) {
		if (section.mapping == mapping->mapping) {
			++section.views;
			break;
		}
	}
	return (LPVOID)(mapping->data + offset);
}

BOOL WINAPI UnmapViewOfFile(LPCVOID view)
{
	const char *p = (const char *)view;
	std::lock_guard<std::mutex> guard(g_mappingLock);
	for (std::vector<Section>::iterator it = Sections().begin(); it != Sections().end(); ++it) {
		if (p >= it->data && p < it->data + it->size && it->views > 0) {
			if (--it->views == 0 && it->closed)
				DropSection(it);
			return TRUE;
		}
	}
	SetLastError(ERROR_INVALID_ADDRESS);
	return FALSE;
}

// ---- The C library on client paths (bf_crt.h) ----------------------------------

extern "C" FILE *bf_crt_fopen(const char *path, const char *mode)
{
	if (!path || !mode || !*mode) {
		errno = EINVAL;
		return NULL;
	}
	bool write = mode[0] != 'r' || strchr(mode, '+') != NULL;
	char resolved[2048];
	if (bf_file_resolve(path, write ? BF_RESOLVE_WRITE : BF_RESOLVE_READ, resolved, sizeof(resolved)) < 0)
		return NULL;   // errno from bf_file: ENOENT, EACCES (a write into the game folder), ...
	// The C library has no text mode: "rt", "wt", "at" are "r", "w", "a".
	char native[8];
	size_t n = 0;
	for (const char *m = mode; *m && n + 1 < sizeof(native); ++m)
		if (*m != 't')
			native[n++] = *m;
	native[n] = 0;
	FILE *file = fopen(resolved, native);
	BF_TRACE(BFT_FILE, "fopen(%s, %s) = %s%s", path, mode, resolved, file ? "" : " (failed)");
	return file;
}

extern "C" int bf_crt_access(const char *path, int mode)
{
	bf_file_info info;
	if (!path || !bf_file_stat(path, &info)) {
		errno = ENOENT;
		return -1;
	}
	if ((mode & W_OK) && info.read_only) {
		errno = EACCES;
		return -1;
	}
	return 0;
}

extern "C" int bf_crt_remove(const char *path)
{
	bf_file_info info;
	if (path && bf_file_stat(path, &info) && info.is_folder)
		return bf_file_remove_folder(path);
	return bf_file_remove(path);
}

extern "C" int bf_crt_unlink(const char *path)
{
	return bf_file_remove(path);
}

extern "C" int bf_crt_rmdir(const char *path)
{
	return bf_file_remove_folder(path);
}

extern "C" int bf_crt_rename(const char *from, const char *to)
{
	return bf_file_rename(from, to);
}

// _chmod(path, _S_IWRITE) before deleting a cached file (base/Util.cpp,
// Session.cpp): applies in the write locations; the game folder is refused.
extern "C" int bf_crt_chmod(const char *path, int mode)
{
	if (!path) {
		errno = EINVAL;
		return -1;
	}
	char resolved[2048];
	if (bf_file_resolve(path, BF_RESOLVE_READ, resolved, sizeof(resolved)) < 0)
		return -1;
	if (bf_file_write_location(path) == BF_LOCATION_GAME) {
		errno = EACCES;
		return -1;
	}
	return chmod(resolved, (mode & _S_IWRITE) ? 0644 : 0444);
}

// 0 created; -1 with EEXIST if it is there already, as the Visual C++ one.
extern "C" int _mkdir(const char *path)
{
	int r = bf_file_make_folder(path);
	if (r == 1)
		return 0;
	if (r == 0)
		errno = EEXIST;
	return -1;
}
