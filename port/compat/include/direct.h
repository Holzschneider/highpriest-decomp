#pragma once
#include <unistd.h>
#include <sys/stat.h>
#ifdef __cplusplus
extern "C" {
#endif
int _mkdir(const char*); char* _getcwd(char*, int); int _chdir(const char*);
#ifdef __cplusplus
}
#endif
