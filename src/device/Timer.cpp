#include "common.h"

#include <windows.h>
#include <mmsystem.h>

extern BOOL g_frameskip;
static DWORD frameTime;
static DWORD frameFreq;



#pragma comment(lib, "winmm.lib")

void	ResetTimer()
{
	frameTime = timeGetTime();
}

void	InitTimer(DWORD freq)
{
	frameTime = timeGetTime();
	frameFreq = 1000/freq;
}

BOOL SkipSceneRendering()
{
	return TRUE;
}

int		GetSkipFrameCount()
{
	int skipCount = (timeGetTime() - frameTime) / frameFreq;

	frameTime += frameFreq * skipCount;

	if (g_frameskip)
	{
		return skipCount;
	}
	else
	{
		return 1;
	}
}
