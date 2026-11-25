#pragma once
#include <stdint.h>
#define P_WAIT 0
#define P_NOWAIT 1
intptr_t spawnl(int mode, const char *path, const char *arg0, ...);
uintptr_t _beginthread(void (*)(void*), unsigned, void*);
void _endthread(void); void _endthreadex(unsigned);
