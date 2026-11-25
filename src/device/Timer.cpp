#include "common.h"

#include "bf_types.h"

extern BOOL g_frameskip;
static DWORD frameTime;
static DWORD frameFreq;



#pragma comment(lib, "winmm.lib")

void	ResetTimer()
{
	frameTime = bf_clock_ticks();
}

void	InitTimer(DWORD freq)
{
	frameTime = bf_clock_ticks();
	frameFreq = 1000/freq;
}

BOOL SkipSceneRendering()
{
	return TRUE;
}

int		GetSkipFrameCount()
{
	int skipCount = (bf_clock_ticks() - frameTime) / frameFreq;

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
