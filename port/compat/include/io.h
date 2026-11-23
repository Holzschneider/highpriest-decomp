#pragma once
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifndef O_BINARY
#define O_BINARY 0
#define _O_BINARY 0
#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY
#define _O_CREAT O_CREAT
#define _O_TRUNC O_TRUNC
#define _O_RDWR O_RDWR
#endif
struct _finddata_t { unsigned attrib; long time_create, time_access, time_write; unsigned size; char name[260]; };
long _findfirst(const char*, struct _finddata_t*); int _findnext(long, struct _finddata_t*); int _findclose(long);
long _filelength(int); 
#define _open open
#define _close close
#define _read read
#define _write write
#define _lseek lseek
#define _access access
#define filelength _filelength
