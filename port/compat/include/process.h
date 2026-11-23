#pragma once
#include <stdint.h>
uintptr_t _beginthread(void (*)(void*), unsigned, void*);
uintptr_t _beginthreadex(void*, unsigned, unsigned (*)(void*), void*, unsigned, unsigned*);
void _endthread(void); void _endthreadex(unsigned);
