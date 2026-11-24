/*
 * bf_types.h: the Windows types, structures, macros and constants the client
 * keeps once every call it makes is native (M11.0b). It replaces, in M11.11,
 * the Windows and DirectX headers that src/common.h includes today.
 *
 * What is here, and why it is the same as today:
 *   - the Win32 integer and pointer typedefs, the opaque handle types and the
 *     structures the client's own code reads and writes (RECT, POINT, SIZE,
 *     PALETTEENTRY, RGBQUAD, the bitmap headers, FILETIME, SYSTEMTIME,
 *     LOGFONTA, D3DMATRIX);
 *   - the macros the client's arithmetic uses (RGB, LOWORD, MAKELONG,
 *     ZeroMemory, MAX_PATH, the calling-convention words, min and max);
 *   - the constants the client uses as its own numbers: the virtual-key codes
 *     (VK_*), the window-message ids it dispatches internally (WM_*), the
 *     character sets and font weights of LOGFONTA, and the Direct3D 7 render,
 *     blend, texture-stage, primitive and vertex-format values in their 2008
 *     numbering, which the renderer keeps (technical plan 3.3, progress log
 *     "M11 planned").
 * Every type has exactly the underlying type, size, alignment and field
 * offsets the vendored Wine headers give it in this build (arm64, the flags
 * of port/tools/flags.sh): INT_PTR, LONG_PTR, LPARAM and LRESULT are long
 * long, UINT_PTR and WPARAM unsigned long long, LONG, BOOL and HRESULT int,
 * DWORD unsigned int, WCHAR char16_t in C++ (unsigned short in C), handles
 * pointers to distinct one-int structures; NULL is 0LL in C++, as Wine
 * redefines it. Every constant has the same value and type. The layout test
 * port/client/test (bifrost-types-test) checks all of it against Wine.
 * Names and values were enumerated from the built source (port/tools/
 * winnames.py, group "keep") and read from the Wine headers by a compiled
 * program; the full families of VK_*, D3DTA_*, D3DFVF_* and of the D3D enums
 * are kept so that the input and graphics modules can map them.
 *
 * Valid C (gnu89, the Lua units get the prefix too: hence min and max only
 * for C++, lgc.c has a local "StkId max") and C++14. Not for Objective-C or
 * Objective-C++: BOOL here is int, the Objective-C runtime's BOOL is bool.
 * Port code in .mm files talks to the client through its own headers.
 */
#ifndef BF_TYPES_H
#define BF_TYPES_H

#if defined(__OBJC__)
#error "bf_types.h is the client's Windows-shaped vocabulary; Objective-C(++) units must not include it (BOOL differs)"
#endif

#include <stddef.h>
#include <string.h>

/* ---------------------------------------------------------------- words */

#define WINAPI
#define CALLBACK
#define APIENTRY
#define APIPRIVATE
#define PASCAL
#define CDECL
#define WINAPIV
#define FAR
#define NEAR
#define _far
#define _near
#define CONST const
#define VOID void
#define __declspec(x)
#define _declspec(x)

#undef NULL
#ifdef __cplusplus
#define NULL 0LL
#else
#define NULL ((void*)0)
#endif

#undef FALSE
#define FALSE 0
#undef TRUE
#define TRUE 1

/* ---------------------------------------------------------- integer types */

typedef char CHAR;
typedef short SHORT;
typedef int LONG;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int BOOL;
typedef BYTE BOOLEAN;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned int ULONG;
typedef float FLOAT;
typedef double DOUBLE;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef ULONGLONG DWORDLONG;
typedef unsigned char byte;

typedef long long INT_PTR;
typedef long long LONG_PTR;
typedef unsigned long long UINT_PTR;
typedef unsigned long long ULONG_PTR;
typedef ULONG_PTR DWORD_PTR;
typedef ULONG_PTR SIZE_T;
typedef LONG_PTR SSIZE_T;

typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
typedef LONG HRESULT;
typedef DWORD COLORREF;
typedef WORD ATOM;

#if defined(__cplusplus) && __cpp_unicode_literals >= 200710
typedef char16_t WCHAR;
#else
typedef unsigned short WCHAR;
#endif
typedef CHAR TCHAR;

typedef void *PVOID;
typedef void *LPVOID;
typedef const void *LPCVOID;
typedef BYTE *PBYTE;
typedef BYTE *LPBYTE;
typedef WORD *LPWORD;
typedef DWORD *PDWORD;
typedef DWORD *LPDWORD;
typedef LONG *LPLONG;
typedef INT *LPINT;
typedef UINT *PUINT;
typedef UINT *LPUINT;
typedef BOOL *LPBOOL;
typedef CHAR *PSTR;
typedef CHAR *LPSTR;
typedef const CHAR *LPCSTR;
typedef WCHAR *LPWSTR;
typedef const WCHAR *LPCWSTR;
typedef LPSTR PTSTR;
typedef LPSTR LPTSTR;
typedef LPCSTR LPCTSTR;

/* ---------------------------------------------------------------- handles */

typedef void *HANDLE;
typedef HANDLE *PHANDLE;
typedef HANDLE *LPHANDLE;
typedef HANDLE HGDIOBJ;
typedef HANDLE HGLOBAL;
typedef HANDLE HLOCAL;
typedef struct HWND__ { int unused; } *HWND;
typedef struct HINSTANCE__ { int unused; } *HINSTANCE;
typedef HINSTANCE HMODULE;
typedef struct HDC__ { int unused; } *HDC;
typedef struct HFONT__ { int unused; } *HFONT;
typedef struct HBITMAP__ { int unused; } *HBITMAP;
typedef struct HBRUSH__ { int unused; } *HBRUSH;
typedef struct HPEN__ { int unused; } *HPEN;
typedef struct HRGN__ { int unused; } *HRGN;
typedef struct HPALETTE__ { int unused; } *HPALETTE;
typedef struct HICON__ { int unused; } *HICON;
typedef HICON HCURSOR;
typedef struct HMENU__ { int unused; } *HMENU;

/* ------------------------------------------------------------- structures */

typedef struct tagRECT {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT, *PRECT, *LPRECT;
typedef const RECT *LPCRECT;

typedef struct tagPOINT {
	LONG x;
	LONG y;
} POINT, *PPOINT, *LPPOINT;

typedef struct tagSIZE {
	LONG cx;
	LONG cy;
} SIZE, *PSIZE, *LPSIZE;

typedef struct tagPALETTEENTRY {
	BYTE peRed;
	BYTE peGreen;
	BYTE peBlue;
	BYTE peFlags;
} PALETTEENTRY, *PPALETTEENTRY, *LPPALETTEENTRY;

typedef struct tagRGBQUAD {
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
} RGBQUAD, *LPRGBQUAD;

/* 14 bytes, as in the file: Windows packs it to 2. */
#pragma pack(push, 2)
typedef struct {
	WORD bfType;
	DWORD bfSize;
	WORD bfReserved1;
	WORD bfReserved2;
	DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER, *LPBITMAPFILEHEADER;
#pragma pack(pop)

typedef struct {
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER, *LPBITMAPINFOHEADER;

typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[1];
} BITMAPINFO, *PBITMAPINFO, *LPBITMAPINFO;

typedef struct _FILETIME {
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

#define LF_FACESIZE 32
typedef struct tagLOGFONTA {
	LONG lfHeight;
	LONG lfWidth;
	LONG lfEscapement;
	LONG lfOrientation;
	LONG lfWeight;
	BYTE lfItalic;
	BYTE lfUnderline;
	BYTE lfStrikeOut;
	BYTE lfCharSet;
	BYTE lfOutPrecision;
	BYTE lfClipPrecision;
	BYTE lfQuality;
	BYTE lfPitchAndFamily;
	CHAR lfFaceName[LF_FACESIZE];
} LOGFONTA, *PLOGFONTA, *LPLOGFONTA;
typedef LOGFONTA LOGFONT;
typedef PLOGFONTA PLOGFONT;
typedef LPLOGFONTA LPLOGFONT;

/* ----------------------------------------------------------------- macros */

#define MAX_PATH 260

#define LOBYTE(w) ((BYTE)((DWORD_PTR)(w) & 0xFF))
#define HIBYTE(w) ((BYTE)((DWORD_PTR)(w) >> 8))
#define LOWORD(l) ((WORD)((DWORD_PTR)(l) & 0xFFFF))
#define HIWORD(l) ((WORD)((DWORD_PTR)(l) >> 16))
#define MAKEWORD(low, high) ((WORD)(((BYTE)((DWORD_PTR)(low) & 0xFF)) | ((WORD)((BYTE)((DWORD_PTR)(high) & 0xFF))) << 8))
#define MAKELONG(low, high) ((LONG)(((WORD)((DWORD_PTR)(low) & 0xFFFF)) | ((DWORD)((WORD)((DWORD_PTR)(high) & 0xFFFF))) << 16))
#define MAKEWPARAM(low, high) ((WPARAM)(DWORD)MAKELONG(low, high))
#define MAKELPARAM(low, high) ((LPARAM)(DWORD)MAKELONG(low, high))
#define MAKELRESULT(low, high) ((LRESULT)(DWORD)MAKELONG(low, high))

#define RGB(r, g, b) ((COLORREF)((BYTE)(r) | ((BYTE)(g) << 8) | ((BYTE)(b) << 16)))
#define PALETTERGB(r, g, b) (0x02000000 | RGB(r, g, b))
#define GetRValue(rgb) ((BYTE)(rgb))
#define GetGValue(rgb) ((BYTE)((rgb) >> 8))
#define GetBValue(rgb) ((BYTE)((rgb) >> 16))

#define ZeroMemory(d, n) memset((d), 0, (n))
#define FillMemory(d, n, c) memset((d), (c), (n))
#define CopyMemory(d, s, n) memcpy((d), (s), (n))
#define MoveMemory(d, s, n) memmove((d), (s), (n))

#ifndef __max
#define __max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef __min
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#endif
/* C++ only: the Lua units (C) get this header through the prefix too, and
   lgc.c declares a local named max. */
#if defined(__cplusplus) && !defined(NOMINMAX)
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#endif

#define S_OK ((HRESULT)0)
#define S_FALSE ((HRESULT)1)
#define E_FAIL ((HRESULT)0x80004005)
#define E_OUTOFMEMORY ((HRESULT)0x8007000E)
#define E_INVALIDARG ((HRESULT)0x80070057)
#define SUCCEEDED(stat) ((HRESULT)(stat) >= 0)
#define FAILED(stat) ((HRESULT)(stat) < 0)

/* ------------------------------------------- fonts and bitmaps (LOGFONTA) */

#define ANSI_CHARSET        ((BYTE)0)
#define DEFAULT_CHARSET     ((BYTE)1)
#define SYMBOL_CHARSET      ((BYTE)2)
#define SHIFTJIS_CHARSET    ((BYTE)128)
#define HANGEUL_CHARSET     ((BYTE)129)
#define HANGUL_CHARSET      ((BYTE)129)
#define GB2312_CHARSET      ((BYTE)134)
#define CHINESEBIG5_CHARSET ((BYTE)136)
#define GREEK_CHARSET       ((BYTE)161)
#define TURKISH_CHARSET     ((BYTE)162)
#define HEBREW_CHARSET      ((BYTE)177)
#define ARABIC_CHARSET      ((BYTE)178)
#define BALTIC_CHARSET      ((BYTE)186)
#define VIETNAMESE_CHARSET  ((BYTE)163)
#define RUSSIAN_CHARSET     ((BYTE)204)
#define EE_CHARSET          ((BYTE)238)
#define EASTEUROPE_CHARSET  ((BYTE)238)
#define THAI_CHARSET        ((BYTE)222)
#define JOHAB_CHARSET       ((BYTE)130)
#define MAC_CHARSET         ((BYTE)77)
#define OEM_CHARSET         ((BYTE)255)
#define VISCII_CHARSET      ((BYTE)240)
#define TCVN_CHARSET        ((BYTE)241)
#define KOI8_CHARSET        ((BYTE)242)
#define ISO3_CHARSET        ((BYTE)243)
#define ISO4_CHARSET        ((BYTE)244)
#define ISO10_CHARSET       ((BYTE)245)
#define CELTIC_CHARSET      ((BYTE)246)

#define FW_DONTCARE   0
#define FW_THIN       100
#define FW_EXTRALIGHT 200
#define FW_ULTRALIGHT 200
#define FW_LIGHT      300
#define FW_NORMAL     400
#define FW_REGULAR    400
#define FW_MEDIUM     500
#define FW_SEMIBOLD   600
#define FW_DEMIBOLD   600
#define FW_BOLD       700
#define FW_EXTRABOLD  800
#define FW_ULTRABOLD  800
#define FW_HEAVY      900
#define FW_BLACK      900

#define BI_RGB       0
#define BI_RLE8      1
#define BI_RLE4      2
#define BI_BITFIELDS 3

/* ----------------------------------------------------- virtual-key codes */

#define VK_LBUTTON                         0x1
#define VK_RBUTTON                         0x2
#define VK_CANCEL                          0x3
#define VK_MBUTTON                         0x4
#define VK_XBUTTON1                        0x5
#define VK_XBUTTON2                        0x6
#define VK_BACK                            0x8
#define VK_TAB                             0x9
#define VK_CLEAR                           0xc
#define VK_RETURN                          0xd
#define VK_SHIFT                           0x10
#define VK_CONTROL                         0x11
#define VK_MENU                            0x12
#define VK_PAUSE                           0x13
#define VK_CAPITAL                         0x14
#define VK_KANA                            0x15
#define VK_HANGEUL                         0x15
#define VK_HANGUL                          0x15
#define VK_JUNJA                           0x17
#define VK_FINAL                           0x18
#define VK_HANJA                           0x19
#define VK_KANJI                           0x19
#define VK_ESCAPE                          0x1b
#define VK_CONVERT                         0x1c
#define VK_NONCONVERT                      0x1d
#define VK_ACCEPT                          0x1e
#define VK_MODECHANGE                      0x1f
#define VK_SPACE                           0x20
#define VK_PRIOR                           0x21
#define VK_NEXT                            0x22
#define VK_END                             0x23
#define VK_HOME                            0x24
#define VK_LEFT                            0x25
#define VK_UP                              0x26
#define VK_RIGHT                           0x27
#define VK_DOWN                            0x28
#define VK_SELECT                          0x29
#define VK_PRINT                           0x2a
#define VK_EXECUTE                         0x2b
#define VK_SNAPSHOT                        0x2c
#define VK_INSERT                          0x2d
#define VK_DELETE                          0x2e
#define VK_HELP                            0x2f
#define VK_LWIN                            0x5b
#define VK_RWIN                            0x5c
#define VK_APPS                            0x5d
#define VK_SLEEP                           0x5f
#define VK_NUMPAD0                         0x60
#define VK_NUMPAD1                         0x61
#define VK_NUMPAD2                         0x62
#define VK_NUMPAD3                         0x63
#define VK_NUMPAD4                         0x64
#define VK_NUMPAD5                         0x65
#define VK_NUMPAD6                         0x66
#define VK_NUMPAD7                         0x67
#define VK_NUMPAD8                         0x68
#define VK_NUMPAD9                         0x69
#define VK_MULTIPLY                        0x6a
#define VK_ADD                             0x6b
#define VK_SEPARATOR                       0x6c
#define VK_SUBTRACT                        0x6d
#define VK_DECIMAL                         0x6e
#define VK_DIVIDE                          0x6f
#define VK_F1                              0x70
#define VK_F2                              0x71
#define VK_F3                              0x72
#define VK_F4                              0x73
#define VK_F5                              0x74
#define VK_F6                              0x75
#define VK_F7                              0x76
#define VK_F8                              0x77
#define VK_F9                              0x78
#define VK_F10                             0x79
#define VK_F11                             0x7a
#define VK_F12                             0x7b
#define VK_F13                             0x7c
#define VK_F14                             0x7d
#define VK_F15                             0x7e
#define VK_F16                             0x7f
#define VK_F17                             0x80
#define VK_F18                             0x81
#define VK_F19                             0x82
#define VK_F20                             0x83
#define VK_F21                             0x84
#define VK_F22                             0x85
#define VK_F23                             0x86
#define VK_F24                             0x87
#define VK_NAVIGATION_VIEW                 0x88
#define VK_NAVIGATION_MENU                 0x89
#define VK_NAVIGATION_UP                   0x8a
#define VK_NAVIGATION_DOWN                 0x8b
#define VK_NAVIGATION_LEFT                 0x8c
#define VK_NAVIGATION_RIGHT                0x8d
#define VK_NAVIGATION_ACCEPT               0x8e
#define VK_NAVIGATION_CANCEL               0x8f
#define VK_NUMLOCK                         0x90
#define VK_SCROLL                          0x91
#define VK_OEM_NEC_EQUAL                   0x92
#define VK_OEM_FJ_JISHO                    0x92
#define VK_OEM_FJ_MASSHOU                  0x93
#define VK_OEM_FJ_TOUROKU                  0x94
#define VK_OEM_FJ_LOYA                     0x95
#define VK_OEM_FJ_ROYA                     0x96
#define VK_LSHIFT                          0xa0
#define VK_RSHIFT                          0xa1
#define VK_LCONTROL                        0xa2
#define VK_RCONTROL                        0xa3
#define VK_LMENU                           0xa4
#define VK_RMENU                           0xa5
#define VK_BROWSER_BACK                    0xa6
#define VK_BROWSER_FORWARD                 0xa7
#define VK_BROWSER_REFRESH                 0xa8
#define VK_BROWSER_STOP                    0xa9
#define VK_BROWSER_SEARCH                  0xaa
#define VK_BROWSER_FAVORITES               0xab
#define VK_BROWSER_HOME                    0xac
#define VK_VOLUME_MUTE                     0xad
#define VK_VOLUME_DOWN                     0xae
#define VK_VOLUME_UP                       0xaf
#define VK_MEDIA_NEXT_TRACK                0xb0
#define VK_MEDIA_PREV_TRACK                0xb1
#define VK_MEDIA_STOP                      0xb2
#define VK_MEDIA_PLAY_PAUSE                0xb3
#define VK_LAUNCH_MAIL                     0xb4
#define VK_LAUNCH_MEDIA_SELECT             0xb5
#define VK_LAUNCH_APP1                     0xb6
#define VK_LAUNCH_APP2                     0xb7
#define VK_OEM_1                           0xba
#define VK_OEM_PLUS                        0xbb
#define VK_OEM_COMMA                       0xbc
#define VK_OEM_MINUS                       0xbd
#define VK_OEM_PERIOD                      0xbe
#define VK_OEM_2                           0xbf
#define VK_OEM_3                           0xc0
#define VK_GAMEPAD_A                       0xc3
#define VK_GAMEPAD_B                       0xc4
#define VK_GAMEPAD_X                       0xc5
#define VK_GAMEPAD_Y                       0xc6
#define VK_GAMEPAD_RIGHT_SHOULDER          0xc7
#define VK_GAMEPAD_LEFT_SHOULDER           0xc8
#define VK_GAMEPAD_LEFT_TRIGGER            0xc9
#define VK_GAMEPAD_RIGHT_TRIGGER           0xca
#define VK_GAMEPAD_DPAD_UP                 0xcb
#define VK_GAMEPAD_DPAD_DOWN               0xcc
#define VK_GAMEPAD_DPAD_LEFT               0xcd
#define VK_GAMEPAD_DPAD_RIGHT              0xce
#define VK_GAMEPAD_MENU                    0xcf
#define VK_GAMEPAD_VIEW                    0xd0
#define VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON  0xd1
#define VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON 0xd2
#define VK_GAMEPAD_LEFT_THUMBSTICK_UP      0xd3
#define VK_GAMEPAD_LEFT_THUMBSTICK_DOWN    0xd4
#define VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT   0xd5
#define VK_GAMEPAD_LEFT_THUMBSTICK_LEFT    0xd6
#define VK_GAMEPAD_RIGHT_THUMBSTICK_UP     0xd7
#define VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN   0xd8
#define VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT  0xd9
#define VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT   0xda
#define VK_OEM_4                           0xdb
#define VK_OEM_5                           0xdc
#define VK_OEM_6                           0xdd
#define VK_OEM_7                           0xde
#define VK_OEM_8                           0xdf
#define VK_OEM_AX                          0xe1
#define VK_OEM_102                         0xe2
#define VK_ICO_HELP                        0xe3
#define VK_ICO_00                          0xe4
#define VK_PROCESSKEY                      0xe5
#define VK_ICO_CLEAR                       0xe6
#define VK_PACKET                          0xe7
#define VK_OEM_RESET                       0xe9
#define VK_OEM_JUMP                        0xea
#define VK_OEM_PA1                         0xeb
#define VK_OEM_PA2                         0xec
#define VK_OEM_PA3                         0xed
#define VK_OEM_WSCTRL                      0xee
#define VK_OEM_CUSEL                       0xef
#define VK_OEM_ATTN                        0xf0
#define VK_OEM_FINISH                      0xf1
#define VK_OEM_COPY                        0xf2
#define VK_OEM_AUTO                        0xf3
#define VK_OEM_ENLW                        0xf4
#define VK_OEM_BACKTAB                     0xf5
#define VK_ATTN                            0xf6
#define VK_CRSEL                           0xf7
#define VK_EXSEL                           0xf8
#define VK_EREOF                           0xf9
#define VK_PLAY                            0xfa
#define VK_ZOOM                            0xfb
#define VK_NONAME                          0xfc
#define VK_PA1                             0xfd
#define VK_OEM_CLEAR                       0xfe

/* ------------------------------------------------------- window messages */

/* The ids the client dispatches through its own procedures (the window
   procedure in WinMain.cpp, the UI windows' SendMsg); the input and window
   module (M11.7) produces them. */
#define WM_ACTIVATE             0x6
#define WM_APP                  0x8000
#define WM_CHAR                 0x102
#define WM_CLOSE                0x10
#define WM_CREATE               0x1
#define WM_DESTROY              0x2
#define WM_ENTERMENULOOP        0x211
#define WM_ENTERSIZEMOVE        0x231
#define WM_ERASEBKGND           0x14
#define WM_EXITMENULOOP         0x212
#define WM_EXITSIZEMOVE         0x232
#define WM_IME_COMPOSITION      0x10f
#define WM_IME_ENDCOMPOSITION   0x10e
#define WM_IME_NOTIFY           0x282
#define WM_IME_SETCONTEXT       0x281
#define WM_IME_STARTCOMPOSITION 0x10d
#define WM_INPUTLANGCHANGE      0x51
#define WM_KEYDOWN              0x100
#define WM_KEYUP                0x101
#define WM_KILLFOCUS            0x8
#define WM_MOUSEMOVE            0x200
#define WM_MOVE                 0x3
#define WM_NCACTIVATE           0x86
#define WM_PAINT                0xf
#define WM_QUIT                 0x12
#define WM_SETCURSOR            0x20
#define WM_SETFOCUS             0x7
#define WM_SYSCOMMAND           0x112
#define WM_SYSKEYDOWN           0x104
#define WM_USER                 0x400

/* --------------------------------------- Direct3D 7 values the renderer keeps */

typedef float D3DVALUE;
typedef DWORD D3DCOLOR;

typedef struct _D3DMATRIX {
	D3DVALUE _11, _12, _13, _14;
	D3DVALUE _21, _22, _23, _24;
	D3DVALUE _31, _32, _33, _34;
	D3DVALUE _41, _42, _43, _44;
#ifdef __cplusplus
	_D3DMATRIX() { }
	D3DVALUE &operator () (int r, int c) { return (&_11)[r * 4 + c]; }
	const D3DVALUE &operator () (int r, int c) const { return (&_11)[r * 4 + c]; }
#endif
} D3DMATRIX, *LPD3DMATRIX;

typedef enum {
	D3DSHADE_FLAT        = 1,
	D3DSHADE_GOURAUD     = 2,
	D3DSHADE_PHONG       = 3,
	D3DSHADE_FORCE_DWORD = 0x7fffffff
} D3DSHADEMODE;

typedef enum {
	D3DFILL_POINT       = 1,
	D3DFILL_WIREFRAME   = 2,
	D3DFILL_SOLID       = 3,
	D3DFILL_FORCE_DWORD = 0x7fffffff
} D3DFILLMODE;

typedef enum {
	D3DBLEND_ZERO            = 1,
	D3DBLEND_ONE             = 2,
	D3DBLEND_SRCCOLOR        = 3,
	D3DBLEND_INVSRCCOLOR     = 4,
	D3DBLEND_SRCALPHA        = 5,
	D3DBLEND_INVSRCALPHA     = 6,
	D3DBLEND_DESTALPHA       = 7,
	D3DBLEND_INVDESTALPHA    = 8,
	D3DBLEND_DESTCOLOR       = 9,
	D3DBLEND_INVDESTCOLOR    = 10,
	D3DBLEND_SRCALPHASAT     = 11,
	D3DBLEND_BOTHSRCALPHA    = 12,
	D3DBLEND_BOTHINVSRCALPHA = 13,
	D3DBLEND_FORCE_DWORD     = 0x7fffffff
} D3DBLEND;

typedef enum _D3DTEXTUREADDRESS {
	D3DTADDRESS_WRAP        = 1,
	D3DTADDRESS_MIRROR      = 2,
	D3DTADDRESS_CLAMP       = 3,
	D3DTADDRESS_BORDER      = 4,
	D3DTADDRESS_FORCE_DWORD = 0x7fffffff
} D3DTEXTUREADDRESS;

typedef enum {
	D3DCULL_NONE        = 1,
	D3DCULL_CW          = 2,
	D3DCULL_CCW         = 3,
	D3DCULL_FORCE_DWORD = 0x7fffffff
} D3DCULL;

typedef enum {
	D3DCMP_NEVER        = 1,
	D3DCMP_LESS         = 2,
	D3DCMP_EQUAL        = 3,
	D3DCMP_LESSEQUAL    = 4,
	D3DCMP_GREATER      = 5,
	D3DCMP_NOTEQUAL     = 6,
	D3DCMP_GREATEREQUAL = 7,
	D3DCMP_ALWAYS       = 8,
	D3DCMP_FORCE_DWORD  = 0x7fffffff
} D3DCMPFUNC;

typedef enum _D3DFOGMODE {
	D3DFOG_NONE        = 0,
	D3DFOG_EXP         = 1,
	D3DFOG_EXP2        = 2,
	D3DFOG_LINEAR      = 3,
	D3DFOG_FORCE_DWORD = 0x7fffffff
} D3DFOGMODE;

typedef enum _D3DZBUFFERTYPE {
	D3DZB_FALSE       = 0,
	D3DZB_TRUE        = 1,
	D3DZB_USEW        = 2,
	D3DZB_FORCE_DWORD = 0x7fffffff
} D3DZBUFFERTYPE;

typedef enum {
	D3DPT_POINTLIST     = 1,
	D3DPT_LINELIST      = 2,
	D3DPT_LINESTRIP     = 3,
	D3DPT_TRIANGLELIST  = 4,
	D3DPT_TRIANGLESTRIP = 5,
	D3DPT_TRIANGLEFAN   = 6,
	D3DPT_FORCE_DWORD   = 0x7fffffff
} D3DPRIMITIVETYPE;

typedef enum {
	D3DRENDERSTATE_TEXTUREHANDLE              = 1,
	D3DRENDERSTATE_ANTIALIAS                  = 2,
	D3DRENDERSTATE_TEXTUREADDRESS             = 3,
	D3DRENDERSTATE_TEXTUREPERSPECTIVE         = 4,
	D3DRENDERSTATE_WRAPU                      = 5,
	D3DRENDERSTATE_WRAPV                      = 6,
	D3DRENDERSTATE_ZENABLE                    = 7,
	D3DRENDERSTATE_FILLMODE                   = 8,
	D3DRENDERSTATE_SHADEMODE                  = 9,
	D3DRENDERSTATE_LINEPATTERN                = 10,
	D3DRENDERSTATE_MONOENABLE                 = 11,
	D3DRENDERSTATE_ROP2                       = 12,
	D3DRENDERSTATE_PLANEMASK                  = 13,
	D3DRENDERSTATE_ZWRITEENABLE               = 14,
	D3DRENDERSTATE_ALPHATESTENABLE            = 15,
	D3DRENDERSTATE_LASTPIXEL                  = 16,
	D3DRENDERSTATE_TEXTUREMAG                 = 17,
	D3DRENDERSTATE_TEXTUREMIN                 = 18,
	D3DRENDERSTATE_SRCBLEND                   = 19,
	D3DRENDERSTATE_DESTBLEND                  = 20,
	D3DRENDERSTATE_TEXTUREMAPBLEND            = 21,
	D3DRENDERSTATE_CULLMODE                   = 22,
	D3DRENDERSTATE_ZFUNC                      = 23,
	D3DRENDERSTATE_ALPHAREF                   = 24,
	D3DRENDERSTATE_ALPHAFUNC                  = 25,
	D3DRENDERSTATE_DITHERENABLE               = 26,
	D3DRENDERSTATE_ALPHABLENDENABLE           = 27,
	D3DRENDERSTATE_FOGENABLE                  = 28,
	D3DRENDERSTATE_SPECULARENABLE             = 29,
	D3DRENDERSTATE_ZVISIBLE                   = 30,
	D3DRENDERSTATE_SUBPIXEL                   = 31,
	D3DRENDERSTATE_SUBPIXELX                  = 32,
	D3DRENDERSTATE_STIPPLEDALPHA              = 33,
	D3DRENDERSTATE_FOGCOLOR                   = 34,
	D3DRENDERSTATE_FOGTABLEMODE               = 35,
	D3DRENDERSTATE_FOGTABLESTART              = 36,
	D3DRENDERSTATE_FOGTABLEEND                = 37,
	D3DRENDERSTATE_FOGTABLEDENSITY            = 38,
	D3DRENDERSTATE_FOGSTART                   = 36,
	D3DRENDERSTATE_FOGEND                     = 37,
	D3DRENDERSTATE_FOGDENSITY                 = 38,
	D3DRENDERSTATE_STIPPLEENABLE              = 39,
	D3DRENDERSTATE_EDGEANTIALIAS              = 40,
	D3DRENDERSTATE_COLORKEYENABLE             = 41,
	D3DRENDERSTATE_BORDERCOLOR                = 43,
	D3DRENDERSTATE_TEXTUREADDRESSU            = 44,
	D3DRENDERSTATE_TEXTUREADDRESSV            = 45,
	D3DRENDERSTATE_MIPMAPLODBIAS              = 46,
	D3DRENDERSTATE_ZBIAS                      = 47,
	D3DRENDERSTATE_RANGEFOGENABLE             = 48,
	D3DRENDERSTATE_ANISOTROPY                 = 49,
	D3DRENDERSTATE_FLUSHBATCH                 = 50,
	D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT = 51,
	D3DRENDERSTATE_STENCILENABLE              = 52,
	D3DRENDERSTATE_STENCILFAIL                = 53,
	D3DRENDERSTATE_STENCILZFAIL               = 54,
	D3DRENDERSTATE_STENCILPASS                = 55,
	D3DRENDERSTATE_STENCILFUNC                = 56,
	D3DRENDERSTATE_STENCILREF                 = 57,
	D3DRENDERSTATE_STENCILMASK                = 58,
	D3DRENDERSTATE_STENCILWRITEMASK           = 59,
	D3DRENDERSTATE_TEXTUREFACTOR              = 60,
	D3DRENDERSTATE_STIPPLEPATTERN00           = 64,
	D3DRENDERSTATE_STIPPLEPATTERN01           = 65,
	D3DRENDERSTATE_STIPPLEPATTERN02           = 66,
	D3DRENDERSTATE_STIPPLEPATTERN03           = 67,
	D3DRENDERSTATE_STIPPLEPATTERN04           = 68,
	D3DRENDERSTATE_STIPPLEPATTERN05           = 69,
	D3DRENDERSTATE_STIPPLEPATTERN06           = 70,
	D3DRENDERSTATE_STIPPLEPATTERN07           = 71,
	D3DRENDERSTATE_STIPPLEPATTERN08           = 72,
	D3DRENDERSTATE_STIPPLEPATTERN09           = 73,
	D3DRENDERSTATE_STIPPLEPATTERN10           = 74,
	D3DRENDERSTATE_STIPPLEPATTERN11           = 75,
	D3DRENDERSTATE_STIPPLEPATTERN12           = 76,
	D3DRENDERSTATE_STIPPLEPATTERN13           = 77,
	D3DRENDERSTATE_STIPPLEPATTERN14           = 78,
	D3DRENDERSTATE_STIPPLEPATTERN15           = 79,
	D3DRENDERSTATE_STIPPLEPATTERN16           = 80,
	D3DRENDERSTATE_STIPPLEPATTERN17           = 81,
	D3DRENDERSTATE_STIPPLEPATTERN18           = 82,
	D3DRENDERSTATE_STIPPLEPATTERN19           = 83,
	D3DRENDERSTATE_STIPPLEPATTERN20           = 84,
	D3DRENDERSTATE_STIPPLEPATTERN21           = 85,
	D3DRENDERSTATE_STIPPLEPATTERN22           = 86,
	D3DRENDERSTATE_STIPPLEPATTERN23           = 87,
	D3DRENDERSTATE_STIPPLEPATTERN24           = 88,
	D3DRENDERSTATE_STIPPLEPATTERN25           = 89,
	D3DRENDERSTATE_STIPPLEPATTERN26           = 90,
	D3DRENDERSTATE_STIPPLEPATTERN27           = 91,
	D3DRENDERSTATE_STIPPLEPATTERN28           = 92,
	D3DRENDERSTATE_STIPPLEPATTERN29           = 93,
	D3DRENDERSTATE_STIPPLEPATTERN30           = 94,
	D3DRENDERSTATE_STIPPLEPATTERN31           = 95,
	D3DRENDERSTATE_WRAP0                      = 128,
	D3DRENDERSTATE_WRAP1                      = 129,
	D3DRENDERSTATE_WRAP2                      = 130,
	D3DRENDERSTATE_WRAP3                      = 131,
	D3DRENDERSTATE_WRAP4                      = 132,
	D3DRENDERSTATE_WRAP5                      = 133,
	D3DRENDERSTATE_WRAP6                      = 134,
	D3DRENDERSTATE_WRAP7                      = 135,
	D3DRENDERSTATE_CLIPPING                   = 136,
	D3DRENDERSTATE_LIGHTING                   = 137,
	D3DRENDERSTATE_EXTENTS                    = 138,
	D3DRENDERSTATE_AMBIENT                    = 139,
	D3DRENDERSTATE_FOGVERTEXMODE              = 140,
	D3DRENDERSTATE_COLORVERTEX                = 141,
	D3DRENDERSTATE_LOCALVIEWER                = 142,
	D3DRENDERSTATE_NORMALIZENORMALS           = 143,
	D3DRENDERSTATE_COLORKEYBLENDENABLE        = 144,
	D3DRENDERSTATE_DIFFUSEMATERIALSOURCE      = 145,
	D3DRENDERSTATE_SPECULARMATERIALSOURCE     = 146,
	D3DRENDERSTATE_AMBIENTMATERIALSOURCE      = 147,
	D3DRENDERSTATE_EMISSIVEMATERIALSOURCE     = 148,
	D3DRENDERSTATE_VERTEXBLEND                = 151,
	D3DRENDERSTATE_CLIPPLANEENABLE            = 152,
	D3DRENDERSTATE_FORCE_DWORD                = 0x7fffffff
} D3DRENDERSTATETYPE;

typedef enum _D3DTEXTURESTAGESTATETYPE {
	D3DTSS_COLOROP               = 1,
	D3DTSS_COLORARG1             = 2,
	D3DTSS_COLORARG2             = 3,
	D3DTSS_ALPHAOP               = 4,
	D3DTSS_ALPHAARG1             = 5,
	D3DTSS_ALPHAARG2             = 6,
	D3DTSS_BUMPENVMAT00          = 7,
	D3DTSS_BUMPENVMAT01          = 8,
	D3DTSS_BUMPENVMAT10          = 9,
	D3DTSS_BUMPENVMAT11          = 10,
	D3DTSS_TEXCOORDINDEX         = 11,
	D3DTSS_ADDRESS               = 12,
	D3DTSS_ADDRESSU              = 13,
	D3DTSS_ADDRESSV              = 14,
	D3DTSS_BORDERCOLOR           = 15,
	D3DTSS_MAGFILTER             = 16,
	D3DTSS_MINFILTER             = 17,
	D3DTSS_MIPFILTER             = 18,
	D3DTSS_MIPMAPLODBIAS         = 19,
	D3DTSS_MAXMIPLEVEL           = 20,
	D3DTSS_MAXANISOTROPY         = 21,
	D3DTSS_BUMPENVLSCALE         = 22,
	D3DTSS_BUMPENVLOFFSET        = 23,
	D3DTSS_TEXTURETRANSFORMFLAGS = 24,
	D3DTSS_FORCE_DWORD           = 0x7fffffff
} D3DTEXTURESTAGESTATETYPE;

typedef enum _D3DTEXTUREOP {
	D3DTOP_DISABLE                   = 1,
	D3DTOP_SELECTARG1                = 2,
	D3DTOP_SELECTARG2                = 3,
	D3DTOP_MODULATE                  = 4,
	D3DTOP_MODULATE2X                = 5,
	D3DTOP_MODULATE4X                = 6,
	D3DTOP_ADD                       = 7,
	D3DTOP_ADDSIGNED                 = 8,
	D3DTOP_ADDSIGNED2X               = 9,
	D3DTOP_SUBTRACT                  = 10,
	D3DTOP_ADDSMOOTH                 = 11,
	D3DTOP_BLENDDIFFUSEALPHA         = 12,
	D3DTOP_BLENDTEXTUREALPHA         = 13,
	D3DTOP_BLENDFACTORALPHA          = 14,
	D3DTOP_BLENDTEXTUREALPHAPM       = 15,
	D3DTOP_BLENDCURRENTALPHA         = 16,
	D3DTOP_PREMODULATE               = 17,
	D3DTOP_MODULATEALPHA_ADDCOLOR    = 18,
	D3DTOP_MODULATECOLOR_ADDALPHA    = 19,
	D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20,
	D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21,
	D3DTOP_BUMPENVMAP                = 22,
	D3DTOP_BUMPENVMAPLUMINANCE       = 23,
	D3DTOP_DOTPRODUCT3               = 24,
	D3DTOP_FORCE_DWORD               = 0x7fffffff
} D3DTEXTUREOP;

typedef enum _D3DTEXTUREMAGFILTER {
	D3DTFG_POINT         = 1,
	D3DTFG_LINEAR        = 2,
	D3DTFG_FLATCUBIC     = 3,
	D3DTFG_GAUSSIANCUBIC = 4,
	D3DTFG_ANISOTROPIC   = 5,
	D3DTFG_FORCE_DWORD   = 0x7fffffff
} D3DTEXTUREMAGFILTER;

typedef enum _D3DTEXTUREMINFILTER {
	D3DTFN_POINT       = 1,
	D3DTFN_LINEAR      = 2,
	D3DTFN_ANISOTROPIC = 3,
	D3DTFN_FORCE_DWORD = 0x7fffffff
} D3DTEXTUREMINFILTER;

typedef enum _D3DTEXTUREMIPFILTER {
	D3DTFP_NONE        = 1,
	D3DTFP_POINT       = 2,
	D3DTFP_LINEAR      = 3,
	D3DTFP_FORCE_DWORD = 0x7fffffff
} D3DTEXTUREMIPFILTER;

#define D3DTA_SELECTMASK     0xf
#define D3DTA_DIFFUSE        0x0
#define D3DTA_CURRENT        0x1
#define D3DTA_TEXTURE        0x2
#define D3DTA_TFACTOR        0x3
#define D3DTA_SPECULAR       0x4
#define D3DTA_COMPLEMENT     0x10
#define D3DTA_ALPHAREPLICATE 0x20


#define D3DFVF_RESERVED0      0x1
#define D3DFVF_POSITION_MASK  0xe
#define D3DFVF_XYZ            0x2
#define D3DFVF_XYZRHW         0x4
#define D3DFVF_XYZB1          0x6
#define D3DFVF_XYZB2          0x8
#define D3DFVF_XYZB3          0xa
#define D3DFVF_XYZB4          0xc
#define D3DFVF_XYZB5          0xe
#define D3DFVF_NORMAL         0x10
#define D3DFVF_RESERVED1      0x20
#define D3DFVF_DIFFUSE        0x40
#define D3DFVF_SPECULAR       0x80
#define D3DFVF_TEXCOUNT_MASK  0xf00
#define D3DFVF_TEXCOUNT_SHIFT 0x8
#define D3DFVF_TEX0           0x0
#define D3DFVF_TEX1           0x100
#define D3DFVF_TEX2           0x200
#define D3DFVF_TEX3           0x300
#define D3DFVF_TEX4           0x400
#define D3DFVF_TEX5           0x500
#define D3DFVF_TEX6           0x600
#define D3DFVF_TEX7           0x700
#define D3DFVF_TEX8           0x800
#define D3DFVF_RESERVED2      0xf000
#define D3DFVF_VERTEX         0x112
#define D3DFVF_LVERTEX        0x1e2
#define D3DFVF_TLVERTEX       0x1c4
#define D3DFVF_TEXTUREFORMAT2 0x0
#define D3DFVF_TEXTUREFORMAT1 0x3
#define D3DFVF_TEXTUREFORMAT3 0x1
#define D3DFVF_TEXTUREFORMAT4 0x2

#define D3DFVF_TEXCOORDSIZE1(CoordIndex) (D3DFVF_TEXTUREFORMAT1 << (CoordIndex * 2 + 16))
#define D3DFVF_TEXCOORDSIZE2(CoordIndex) (D3DFVF_TEXTUREFORMAT2)
#define D3DFVF_TEXCOORDSIZE3(CoordIndex) (D3DFVF_TEXTUREFORMAT3 << (CoordIndex * 2 + 16))
#define D3DFVF_TEXCOORDSIZE4(CoordIndex) (D3DFVF_TEXTUREFORMAT4 << (CoordIndex * 2 + 16))

#endif
