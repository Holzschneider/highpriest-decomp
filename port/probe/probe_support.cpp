/*
 * What the probes need from the rest of the game and from Windows, in the
 * smallest form that lets device/Connection.cpp link on its own.
 */
#include "common.h"
#include <time.h>

DWORD WINAPI timeGetTime(void)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (DWORD)(now.tv_sec * 1000 + now.tv_nsec / 1000000);
}

void ErrorMsg(const char *msg)
{
	BF_TRACE(BFT_ERR, "ErrorMsg: %s", msg);
}
