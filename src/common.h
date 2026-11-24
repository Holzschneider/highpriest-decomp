








#ifndef	__COMMON
#define	__COMMON

#define	DIRECTINPUT_VERSION		0x700

#define		WIN32_LEAN_AND_MEAN
#include	<windows.h>
#include	<math.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdarg.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<io.h>
#include	<string>
#include	<strstream>
#include	<map>
#include	<algorithm>
#include	<vector>
#include	<list>
#include	<direct.h>
#include	<mmsystem.h>
#define		D3D_OVERLOADS
#include	<d3d.h>
#include	<dsound.h>
#include	<dinput.h>
#include	<winsock.h>
#include	<process.h>
#include	<shlwapi.h>
#include	<stack>

using namespace	std;

#include	"base/SimpleAssert.h"

typedef		DWORD		SYSTEMPALETTE;

void	CheckSystemMessage();
int    CheckDevice() ;
BOOL    InitApp(HINSTANCE hInstance, int nCmdShow);
void    ExitApp();

#if defined(BF_PORT)
	#include "bf_trace.h"
	inline	void	Trace(const char *str, ...)
	{
		if (bf_trace_mask & BFT_GAME) {
			va_list args;
			va_start(args, str);
			bf_tracev(BFT_GAME, str, args);
			va_end(args);
		}
	}
#elif defined(DEBUG) || defined(_DEBUG)
	void	Trace(const char *str, ...);
#else
	inline	void	Trace(const char *str, ...) {}
#endif

void	ErrorMsg(const char *msg);
void	ErrorMsg(int num);

extern	int	g_recentEffectId[32];

extern	HWND		g_hMainWnd;
extern	HINSTANCE	g_hInstance;
extern	BOOL		g_sysQuit;
extern	char		g_baseDir[128];
extern	char		g_baseDir3[128];
extern	BOOL		g_isAppActive;
extern	BOOL		g_activatedNow;
extern	bool		g_isAssert;
#if defined(BF_PORT)
// The system zlib's lengths are 64-bit uLongf; these take the 32-bit DWORD.
#include "bf_port.h"
#define uncompress bf_zlib_uncompress
#define compress bf_zlib_compress
#else
#define bf_tr(s) (s)
extern "C" __declspec(dllimport) int uncompress (BYTE* dest, DWORD *destLen, const BYTE *source, DWORD sourceLen);
extern "C" __declspec(dllimport) int compress(BYTE* dest, DWORD *destLen, const BYTE *source, DWORD sourceLen);
#endif

#undef	sqrt
#undef	sqrtf
#undef	sinf
#undef	cosf

float	_sqrt(float input);
float	_sin(float input);
float	_cos(float input);

#define		sqrt(a)		_sqrt(a)
#define		sin(a)		_sin(a)
#define		cos(a)		_cos(a)
#define		sqrtf(a)	_sqrt(a)
#define		sinf(a)		_sin(a)
#define		cosf(a)		_cos(a)

struct CharPrtLess : public binary_function<const char*, const char*, bool> {
	bool operator()(const char* left, const char* right) const {
		while(*left == *right && *left) { ++left; ++right; }
		if(*((const unsigned char*)left) < *((const unsigned char*)right)) return true;
		return false;
	}
};

#endif
