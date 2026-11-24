#include "common.h"
#include "MsgStrings.h"
#include "framework/Locale.h"
#include "device/Sound.h"

#include <windows.h>
#include <ctype.h>
#include <ddraw.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <string>

#include "3dlib/Device.h"
#include "Mode.h"
#include "Session.h"
#include "window/UIFrameWnd.h"
#include "base/File.h"
#include "base/Res.h"
#include "device/Connection.h"
#include "device/Control.h"
#include "framework/Language.h"
#include "movie/CBink.h"
#include "pdb_enums.h"
#include "pdb_shared.h"
#include "RagEffect.h"
#include "RagEffectPrim.h"
#include "resource/Model.h"
#include "skin.h"
#include <shellapi.h>
#include <tlhelp32.h>
#include "GameMode.h"
#if defined(BF_PORT)
#include "LoginMode.h"
#endif


extern ServiceType   g_serviceType;
extern Registry2     g_registry;

extern const char*   WINDOW_NAME;
extern const char*   g_regPath;
extern char*         g_optionPath;
extern int           WINDOW_WIDTH;
extern int           WINDOW_HEIGHT;
extern char          g_multiSTOP;
extern const char*   MsgStr(MSGSTRINGID id);

extern bool          IsEnableOldKeycrypt();
extern int           NpKeyMyLoadLibrary();

extern CSession      g_session;
extern CMouse        g_mouse;
extern CModeMgr      g_modeMgr;
extern ServerType    g_serverType;
extern int           g_loginStartMode;
extern char          g_EXE_way;
extern const char*   PATCH_CLIENT;
extern const char*   PATCH_UP;
extern bool          g_bUseCommand;
extern void          InitClientInfo(const char* clientInfo);
extern void          ResetTimer();
extern void          DeleteHack();
extern void          SetHwnd();
extern LONG CALLBACK ExceptionFilter(struct _EXCEPTION_POINTERS*);

extern int  (__cdecl *MyInitialize)();
extern int  (__cdecl *MyStartEncryption)();
extern int  (__cdecl *MyGetInitError)();
extern int  (__cdecl *MyGetAthKbdObject)(char*);
extern void (__cdecl *MyRegisterHWNDForEdit)(unsigned int, void*);
extern void (__cdecl *MyInstallReg)();

HANDLE ExcuteProgram(const char* excutiveFile);

char mutexKey[] = "Surface";
char g_baseDir[128] = "data\\";
int WINDOW_WIDTH = 640;
int WINDOW_HEIGHT = 480;
int g_texLenDivFactor = 1;
int g_isAppActive = 1;
int g_frameskip = 1;
int g_showTipsAtStartup = 1;
int g_isLightMap = 1;


Registry RStr;
char g_baseDir3[128];
Registry2 g_registry;
WNDPROC lpfnNewEditWndProc;
CBink g_Bink;
HWND g_hMainWnd = NULL;
HWND g_hActiveXWnd = NULL;
int g_sysQuit = 0;
HINSTANCE g_hInstance = NULL;
int g_activatedNow = 0;
int g_isVoodoo = 0;
int g_disableFiltering = 0;
int g_isXHalfSpr = 0;
int g_isYHalfSpr = 0;
int g_isSkipFrame = 0;
char g_MessageHi = 0;
bool g_enableKeyCrypt = false;
char g_FirstEffect = 0;
char g_checkBOT = 0;
short g_BOT_x = 0;
short g_BOT_y = 0;
char g_multiSTOP = 0;
char g_EXE_way = 0;

#if defined(BF_PORT)
// The port's resize path (M4.3a). bf_main.cpp calls these from the view's
// listener and from the modal frame (port/native/view).
static int g_bfModalDepth = 0;
extern int g_bfModeRunning;   // Mode.cpp: a mode is in OnRun
extern UIWindowMgr g_windowMgr;

// The window's logical size has changed and the graphics device has been
// resized: the device's buffers, the renderer's size and projection (the
// picking uses both) and the window manager's bounds follow, and the windows
// are put back into view. 0 while the client has no renderer yet.
extern "C" int bf_client_view_changed(int w, int h)
{
	if (!g_renderer)
		return 0;
	if (w == g_renderer->m_width && h == g_renderer->m_height)
		return 1;
	int oldW = g_renderer->m_width, oldH = g_renderer->m_height;
	HRESULT hr = g_3dDevice.BfResize(w, h);
	if (FAILED(hr)) {
		bf_tracef(BFT_ERR, "resize: C3dDevice::BfResize(%d, %d) failed (0x%08x)", w, h, (unsigned)hr);
		return 1;
	}
	g_renderer->SetSize(w, h);
	if (g_renderer->m_guardBandLeft == -640) {   // CreateRenderer's defaults follow the size
		g_renderer->m_guardBandRight = g_renderer->m_width + 640;
		g_renderer->m_guardBandBottom = g_renderer->m_height + 480;
	}
	g_windowMgr.BfOnResize(w, h);
	BF_TRACE(BFT_GFX, "resize: client %dx%d -> %dx%d", oldW, oldH, w, h);
	return 1;
}

// One frame while AppKit runs a loop of its own (a live resize, an open
// menu), from inside the message pump of the current mode's OnUpdate: the
// mode's OnUpdate again, with the pump left out. Not during start-up or a
// mode's OnInit (map loading pumps too), and not nested.
extern "C" int bf_client_modal_frame(void)
{
	CMode *mode = g_modeMgr.GetCurMode();
	if (g_bfModalDepth || !g_bfModeRunning || !mode || g_sysQuit)
		return 0;
	++g_bfModalDepth;
	mode->OnUpdate();
	--g_bfModalDepth;
	return 1;
}
#endif

void CheckSystemMessage()
{
#if defined(BF_PORT)
	bf_port_frame();
	if (g_bfModalDepth)
		return;   // inside bf_client_modal_frame: AppKit holds the event loop
#endif
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_KEYDOWN && msg.wParam == VK_PROCESSKEY)
		{
			g_language->SetKeyStroke((unsigned char)ImmGetVirtualKey(msg.hwnd));
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

bool GetWindowActiveMode()
{
	if (g_isAppActive == TRUE) return true;
	return false;
}

extern HRESULT SetAcquire(BOOL acquire);

void SetWindowActiveMode(BOOL active)
{
	g_isAppActive = active;
	if (active)
	{
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
		SetAcquire(TRUE);
	}
	else
	{
		SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
		SetAcquire(FALSE);
	}
}















extern void          ExitFindHack();
extern int          SetAcquire(int active);
extern int           (__cdecl *MyCloseDriver)();
extern int           (__cdecl *MyStopEncryption)();
extern UIWindowMgr   g_windowMgr;


LRESULT FAR PASCAL WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	bool hooked;
	LRESULT lResult = g_language->ProcessImeMessage(hWnd, message, wParam, lParam, &hooked);

	if (hooked)
	{
		if (g_language->GetCandidateCount())
		{
			const int MAX_CANDLIST = 9;
			std::string strCandidate[MAX_CANDLIST];
			int intArray[4];
			int candidateStart = g_language->GetCandidateSelection() - g_language->GetCandidateSelection() % MAX_CANDLIST;
			int candidateEnd = min(candidateStart + MAX_CANDLIST, g_language->GetCandidateCount());
			intArray[0] = g_language->GetCandidateSelection() - candidateStart;
			intArray[1] = (g_language->GetCandidateSelection() + MAX_CANDLIST - 1) / MAX_CANDLIST;
			intArray[2] = (g_language->GetCandidateCount() + MAX_CANDLIST - 1) / MAX_CANDLIST;
			intArray[3] = candidateEnd - candidateStart;

			for (int i = 0; i < candidateEnd - candidateStart; ++i)
				strCandidate[i] = g_language->GetCandidateString(candidateStart + i);

			UIFrameWnd* wnd = g_windowMgr.MakeWindow(WID_CANDIDATEWND);
			wnd->SendMsg(NULL, UM_IME_SET_CANDIDATE, (INT_PTR)strCandidate, (INT_PTR)intArray, 0, 0);
		} else {
			g_windowMgr.DeleteWindow(WID_CANDIDATEWND);
		}



		if (g_language->GetKeyStrokeSize())
		{
			g_windowMgr.MakeWindow(WID_KEYSTROKEWND)->Invalidate();
		}
		else
		{
			g_windowMgr.DeleteWindow(WID_KEYSTROKEWND);
		}
		return lResult;
	}

	switch (message)
	{
	case WM_CREATE:
		g_language->SetHWnd(hWnd);
		return 0;

	case WM_MOVE:
		if (GetWindowActiveMode() == FALSE && g_3dDevice.IsFullscreen() == FALSE)
			g_3dDevice.ShowFrame(1);
		return 0;

	case WM_NCACTIVATE:
		SetWindowActiveMode(LOWORD(wParam));
		if (wParam) ResetTimer();
		break;

	case WM_ACTIVATE:
		if (g_3dDevice.IsFullscreen() == TRUE)
		{
			if (GetWindowActiveMode())
			{
				g_activatedNow = TRUE;
				ShowWindow(hWnd, SW_MAXIMIZE);
			}
			else
			{
				ShowWindow(hWnd, SW_MINIMIZE);
			}
		}
		return TRUE;

	case WM_ENTERMENULOOP:
	case WM_ENTERSIZEMOVE:
		SetWindowActiveMode(FALSE);
		break;

	case WM_EXITMENULOOP:
	case WM_EXITSIZEMOVE:
		SetWindowActiveMode(GetActiveWindow() == hWnd || !IsIconic(hWnd));
		break;

	case WM_CLOSE:
		g_sysQuit = TRUE;
		if (IsEnableOldKeycrypt())
			MyCloseDriver();
		if (g_serviceType == 0xb)
			ShellExecuteA(g_hMainWnd, "open", "http://www.ragnarok.co.in/index.php", NULL, NULL, SW_SHOWNORMAL);
		ExitFindHack();
		return 0;

	case WM_SYSKEYDOWN:
		if (g_windowMgr.ProcessPushButton(wParam, (~(UINT)lParam >> 30) & 1, 0)) return 0;
		break;

	case WM_SYSCOMMAND:
		switch (wParam & 0xFFF0)
		{
		case SC_MOVE:
		case SC_RESTORE:
		case SC_MINIMIZE:
		case SC_MOUSEMENU:
		case SC_MONITORPOWER:
			if (g_3dDevice.IsFullscreen()) return 0;
			break;
		case SC_CLOSE:
			break;
		case SC_KEYMENU:
			if (lParam == ' ') break;
		default:
			return 0;
		}
		break;

	case WM_MOUSEMOVE:
		if (GetWindowActiveMode() == FALSE && g_3dDevice.IsFullscreen() == FALSE)
		{
			POINT ptMouse;
			GetCursorPos(&ptMouse);
			ScreenToClient(g_hMainWnd, &ptMouse);
			g_mouse.SetXPos(ptMouse.x);
			g_mouse.SetYPos(ptMouse.y);
		}
		break;

	case WM_SETCURSOR:
		if (g_3dDevice.IsFullscreen() == FALSE)
			SetCursor(NULL);
		return 0;

	case WM_IME_SETCONTEXT:
		wParam = TRUE;
		lParam = 0;
		break;

	case WM_KEYDOWN:
		if (!g_Bink.Break(wParam)) return 0;
		if (g_windowMgr.ProcessPushButton(wParam, (~(UINT)lParam >> 30) & 1, lParam)) return 0;
		if (g_windowMgr.isFocusChatWnd())
		{
			if (g_windowMgr.ExcuteMsgInBattleMode(wParam, lParam)) return 0;
		}
		return 0;

	case WM_CHAR:
		if (g_session.m_isBattle)
		{
			if (g_serviceType == 0xd)
			{
				if (g_session.m_isBmChat)
				{
					if (wParam == VK_SPACE && g_session.m_isStartBmChat)
					{
						g_windowMgr.SetTextAtBattleMode();
						g_session.m_isStartBmChat = FALSE;
					}
					else
						g_language->OnChar(wParam, lParam);
				}
				else
				{
					if (!g_windowMgr.isFocusChatWnd())
						g_language->OnChar(wParam, lParam);
				}
			}
			else
			{
				if (!g_windowMgr.isFocusChatWnd())
					g_language->OnChar(wParam, lParam);

				if (g_session.m_isBmChat)
				{
					if (wParam == VK_SPACE && g_session.m_isStartBmChat)
					{
						g_windowMgr.SetTextAtBattleMode();
						g_session.m_isStartBmChat = FALSE;
					}
					else
						g_language->OnChar(wParam, lParam);
				}
			}
		}
		else
		{
			g_language->OnChar(wParam, lParam);
		}

		return 0;

	case WM_KILLFOCUS:
		if (g_Bink.IsBinkPlaying())
			g_Bink.Pause(1);
		if (IsEnableOldKeycrypt()) MyStopEncryption();
		break;

	case WM_SETFOCUS:
		if (g_Bink.IsBinkPlaying())
			g_Bink.Pause(0);
		break;

	case WM_KEYUP:
		if (wParam == VK_SNAPSHOT && g_windowMgr.ProcessPushButton(wParam, 1, 0)) return 0;
	case WM_DESTROY:
		return 0;

	case WM_ERASEBKGND:
		return TRUE;

	case WM_PAINT:
		ValidateRect(hWnd, NULL);
		return 0;
	}

	return DefWindowProcA(hWnd, message, wParam, lParam);
}

int InitApp(HINSTANCE hInstance, int nCmdShow)
{
	g_hInstance = hInstance;

	if (FindWindowA(WINDOW_NAME, WINDOW_NAME))
		g_multiSTOP = 1;

	WNDCLASSA wc = { 0, WindowProc, 0, 0, hInstance,
					 LoadIconA(hInstance, MAKEINTRESOURCEA(0x77)),
					 LoadCursorA(NULL, IDC_ARROW),
					 (HBRUSH)GetStockObject(BLACK_BRUSH), NULL, WINDOW_NAME };
	RegisterClassA(&wc);

	RECT windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	AdjustWindowRect(&windowRect, 0x02CA0000, FALSE);

	char OptionPath[0x200];
	strcpy(OptionPath, g_regPath);
	strcat(OptionPath, g_optionPath);

	DWORD REG_TYPE = REG_DWORD;
	DWORD DATALENGTH = sizeof(DWORD);
	short win_left = (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2;
	short win_top = 0;

	HKEY hKey;
	LONG lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, OptionPath, 0, KEY_QUERY_VALUE, &hKey);
	if (lResult == ERROR_SUCCESS)
	{
		if (RegQueryValueExA(hKey, "window_left", 0, &REG_TYPE, (LPBYTE)&win_left, &DATALENGTH) != ERROR_SUCCESS)
			win_left = (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2;
		if (RegQueryValueExA(hKey, "window_top", 0, &REG_TYPE, (LPBYTE)&win_top, &DATALENGTH) != ERROR_SUCCESS)
			win_top = 0;
		if ((short)win_left > GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH)
			win_left = (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2;
		if ((short)win_left < 0)
			win_left = 0;
		if ((short)win_top > GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT)
			win_top = (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2;
		if ((short)win_top < 0)
			win_top = 0;
	}
	RegCloseKey(hKey);

	g_hMainWnd = CreateWindowExA(0, wc.lpszClassName, WINDOW_NAME, 0x02CA0000,
								 (short)win_left, (short)win_top,
								 windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
								 NULL, NULL, hInstance, NULL);
	if (!g_hMainWnd)
	{
		MessageBoxA(NULL, "Window Creation Failed", "ERROR", 0);
		return 0;
	}

	ShowWindow(g_hMainWnd, SW_SHOW);
	UpdateWindow(g_hMainWnd);

	if (IsEnableOldKeycrypt())
	{
		if (NpKeyMyLoadLibrary())
		{
			if (MyInitialize() != 0)
			{
				int err = MyGetInitError();
				if (err == 3)
				{
					char temp[30] = {0};
					if (MyGetAthKbdObject(temp))
						MessageBoxA(NULL, temp, MsgStr(MSI_KEYCRYPT), 0x1040);
					int chose = MessageBoxA(NULL, MsgStr(MSI_KEYCRYPT_INSTALL_KEYDRIVER), MsgStr(MSI_KEYCRYPT), 0x1044);
					if (chose == IDYES)
					{
						if (MyInstallReg)
						{
							MyInstallReg();
							MessageBoxA(NULL, MsgStr(MSI_KEYCRYPT_REBOOT),
										MsgStr(MSI_KEYCRYPT), 0x1040);
							return 0;
						}
						MessageBoxA(NULL, MsgStr(MSI_KEYCRYPT_INSTALLFAIL),
									MsgStr(MSI_KEYCRYPT), 0x1040);
						return 0;
					}
					MessageBoxA(NULL, MsgStr(MSI_KEYCRYPT_SKIPKEYCRYPT),
								MsgStr(MSI_KEYCRYPT), 0x1040);
				}
				MyRegisterHWNDForEdit(0x10, g_hMainWnd);
				MyStartEncryption();
			}
			else
			{
				int err = MyGetInitError();
				if (err == 1)
				{
					MessageBoxA(NULL, MsgStr(MSI_KEYCRYPT_NOKEYCRYPTFILE),
								MsgStr(MSI_KEYCRYPT), 0x1040);
				}
				else if (err == 2 && GetKeyboardType(0) == 0x51)
				{
					int chose = MessageBoxA(NULL, MsgStr(MSI_KEYCRYPT_USBKEYBOARD), MsgStr(MSI_KEYCRYPT), 0x1044);
					if (chose == IDYES)
					{
						if (MyInstallReg)
						{
							MyInstallReg();
							MessageBoxA(NULL, MsgStr(MSI_KEYCRYPT_REBOOT),
										MsgStr(MSI_KEYCRYPT), 0x1040);
							return 0;
						}
						MessageBoxA(NULL, MsgStr(MSI_KEYCRYPT_INSTALLFAIL),
									MsgStr(MSI_KEYCRYPT), 0x1040);
						return 0;
					}
					MessageBoxA(NULL, MsgStr(MSI_KEYCRYPT_SKIPKEYCRYPT),
								MsgStr(MSI_KEYCRYPT), 0x1040);
				}
			}
		}
	}

	GetCurrentDirectoryA(0x80, g_baseDir3);
	return 1;
}

void ExitApp()
{
	CheckSystemMessage();
	CoUninitialize();
	DestroyWindow(g_hMainWnd);
}

extern const char* g_regPath;
extern int WINDOW_WIDTH, WINDOW_HEIGHT;
extern int g_showTipsAtStartup, g_isLightMap, g_isVoodoo;
extern int g_isXHalfSpr, g_isYHalfSpr, g_texLenDivFactor;
int ReadRegistry()
{
	HKEY hKey;
	DWORD reg_Type;
	HKEY hResultKey;
	DWORD Datalength;

	LONG lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, g_regPath, 0, KEY_QUERY_VALUE, &hResultKey);
	RegCloseKey(hResultKey);
	if (lResult != ERROR_SUCCESS) {
		// No settings written yet: built-in defaults instead of a setup program.
		g_registry.m_isFullScreenMode = 0;
		g_registry.m_width = WINDOW_WIDTH = 1024;
		g_registry.m_height = WINDOW_HEIGHT = 768;
		g_registry.m_bitPerPixel = 32;
		g_registry.m_spriteMode = 2;
		g_registry.m_textureMode = 2;
		g_registry.m_soundMode = 1;
		g_registry.m_numSampleType = 2;
		g_soundMode = g_registry.m_soundMode;
		return 0;
	}

	LONG lResult1 = RegOpenKeyExA(HKEY_LOCAL_MACHINE, g_regPath, 0, KEY_QUERY_VALUE, &hKey);
	if (lResult1 == ERROR_SUCCESS)
	{
		reg_Type = REG_DWORD;
		Datalength = sizeof(DWORD);
		RegQueryValueExA(hKey, "ISFULLSCREENMODE", 0, &reg_Type, (LPBYTE)&g_registry.m_isFullScreenMode, &Datalength);
		RegQueryValueExA(hKey, "WIDTH",            0, &reg_Type, (LPBYTE)&g_registry.m_width,            &Datalength);
		RegQueryValueExA(hKey, "HEIGHT",           0, &reg_Type, (LPBYTE)&g_registry.m_height,           &Datalength);
		if (!g_registry.m_isFullScreenMode) {
			WINDOW_WIDTH = g_registry.m_width;
			WINDOW_HEIGHT = g_registry.m_height;
		}
		RegQueryValueExA(hKey, "BITPERPIXEL",      0, &reg_Type, (LPBYTE)&g_registry.m_bitPerPixel,      &Datalength);
		RegQueryValueExA(hKey, "DEVICECNT",        0, &reg_Type, (LPBYTE)&g_registry.m_deviceCnt,        &Datalength);
		RegQueryValueExA(hKey, "MODECNT",          0, &reg_Type, (LPBYTE)&g_registry.m_modeCnt,          &Datalength);
		RegQueryValueExA(hKey, "ISVOODOO",         0, &reg_Type, (LPBYTE)&g_registry.m_isVoodoo,         &Datalength);
		RegQueryValueExA(hKey, "SHOWTIPSATSTARTUP",0, &reg_Type, (LPBYTE)&g_showTipsAtStartup,           &Datalength);
		if (RegQueryValueExA(hKey, "ISLIGHTMAP", 0, &reg_Type, (LPBYTE)&g_isLightMap, &Datalength) != ERROR_SUCCESS)
			g_isLightMap = 1;
		if (g_isLightMap == 1)
			g_Weather.SetObjLight(true);
		else
			g_Weather.SetObjLight(false);

		if (RegQueryValueExA(hKey, "TRILINEARFILTER", 0, &reg_Type, (LPBYTE)&g_registry.m_trilinearFilter, &Datalength) != ERROR_SUCCESS)
			g_registry.m_trilinearFilter = 0;

		if (RegQueryValueExA(hKey, "SPRITEMODE", 0, &reg_Type, (LPBYTE)&g_registry.m_spriteMode, &Datalength) != ERROR_SUCCESS)
			g_registry.m_spriteMode = 2;
		switch (g_registry.m_spriteMode)
		{
			case 0:
				g_isXHalfSpr = 1;
			case 1:
				g_isYHalfSpr = 1;
				break;
			default:
				g_isXHalfSpr = 0;
				g_isYHalfSpr = 0;
				break;
		}

		if (RegQueryValueExA(hKey, "TEXTUREMODE", 0, &reg_Type, (LPBYTE)&g_registry.m_textureMode, &Datalength) != ERROR_SUCCESS)
			g_registry.m_textureMode = 2;
		switch (g_registry.m_textureMode)
		{
			case 0:
				g_texLenDivFactor = 4;
				break;
			case 1:
				g_texLenDivFactor = 2;
				break;
			default:
				g_texLenDivFactor = 1;
				break;
		}

		if (g_showTipsAtStartup != 1 && g_showTipsAtStartup != 0)
			g_showTipsAtStartup = 1;

		reg_Type = REG_SZ;
		Datalength = 40;
		RegQueryValueExA(hKey, "DEVICENAME", 0, &reg_Type, (LPBYTE)g_registry.m_deviceName, &Datalength);
		reg_Type = REG_BINARY;
		Datalength = 16;
		RegQueryValueExA(hKey, "GUIDDEVICE", 0, &reg_Type, (LPBYTE)&g_registry.m_guidDevice, &Datalength);
		reg_Type = REG_BINARY;
		Datalength = 16;
		RegQueryValueExA(hKey, "GUIDDRIVER", 0, &reg_Type, (LPBYTE)&g_registry.m_guidDriver, &Datalength);
		RegCloseKey(hKey);
	}

	g_isVoodoo = g_registry.m_isVoodoo;
	lResult1 = RegOpenKeyExA(HKEY_LOCAL_MACHINE, g_regPath, 0, KEY_QUERY_VALUE, &hKey);
	if (lResult1 == ERROR_SUCCESS)
	{
		reg_Type = REG_DWORD;
		Datalength = sizeof(DWORD);
		RegQueryValueExA(hKey, "SOUNDMODE",       0, &reg_Type, (LPBYTE)&g_registry.m_soundMode,       &Datalength);
		RegQueryValueExA(hKey, "SPEAKERTYPE",     0, &reg_Type, (LPBYTE)&g_registry.m_speakerType,     &Datalength);
		RegQueryValueExA(hKey, "DIGITALRATETYPE", 0, &reg_Type, (LPBYTE)&g_registry.m_digitalrateType, &Datalength);
		RegQueryValueExA(hKey, "DIGITALBITSTYPE", 0, &reg_Type, (LPBYTE)&g_registry.m_digitalbitsType, &Datalength);
		if (RegQueryValueExA(hKey, "NUMSAMPLETYPE", 0, &reg_Type, (LPBYTE)&g_registry.m_numSampleType, &Datalength) != ERROR_SUCCESS)
			g_registry.m_numSampleType = 2;
		reg_Type = REG_SZ;
		Datalength = 0x400;
		RegQueryValueExA(hKey, "PROVIDERNAME", 0, &reg_Type, (LPBYTE)g_registry.m_providerName, &Datalength);
		RegCloseKey(hKey);
	}

	g_soundMode = g_registry.m_soundMode;
	return 1;
}

int SetupDriver2()
{
	if (g_registry.m_soundMode)
	{
		if (!InitMSS())
		{
			ErrorMsg("Audio System Init Failed");
			g_soundMode = 0;
			return 0;
		}
	}

	DDSURFACEDESC2 ddsd;
	DWORD dwFlags = 6;
	if (g_registry.m_isFullScreenMode) dwFlags = 7;

	ddsd.dwWidth  = g_registry.m_width;
	ddsd.dwHeight = g_registry.m_height;
	ddsd.ddpfPixelFormat.dwRGBBitCount = g_registry.m_bitPerPixel;
	ddsd.dwMipMapCount = 0;

	if (g_3dDevice.Init(g_hMainWnd,
						(_GUID*)&g_registry.m_guidDriver,
						(_GUID*)&g_registry.m_guidDevice,
						(_DDSURFACEDESC2*)&ddsd,
						dwFlags) < 0)
	{
		ErrorMsg("Cannot init d3d OR grf file has problem.");
		return 0;
	}
	return 1;
}

bool UpdatePatch(const char* patch, const char* pactchup)
{
	if (access(pactchup, 0)) return false;

	char buf[256];
	if (ServiceBrazil == g_serviceType)
		sprintf(buf, "%s foi modificada. Clique em Ok para continuar.", patch);
	else
		sprintf(buf, "%s is changed. Press ok to restart", patch);

	if (MessageBox(NULL, buf, "Alert", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
	{
		_chmod(patch, _S_IWRITE);
		_unlink(patch);
		if (rename(pactchup, patch) == 0)
		{
			ExcuteProgram(patch);
			return true;
		}
	}
	return false;
}

HANDLE ExcuteProgram(const char* excutiveFile)
{
	char excutiveFileName[256];

	strcpy(excutiveFileName, excutiveFile);

	STARTUPINFO si = {0, };
	PROCESS_INFORMATION pi = { 0, };

	si.cb=sizeof(STARTUPINFO);
	si.dwFlags=STARTF_USESHOWWINDOW;
	si.wShowWindow=SW_SHOWNORMAL;

	CreateProcess(NULL, excutiveFileName, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

	return pi.hProcess;
}

// The archives, in search order, as later clients list them in DATA.INI in
// the game folder:
//   [Data]
//   1=rdata.grf
//   2=data.grf
// The lowest number is searched first. Without the file, data.grf alone.
void AddPaksFromDataIni()
{
	int keys[16];
	char names[16][MAX_PATH];
	int numPak = 0;

	FILE* fp = fopen("DATA.INI", "r");
	if (fp) {
		char line[MAX_PATH + 16];
		bool inData = false;
		while (numPak < 16 && fgets(line, sizeof(line), fp)) {
			char* p = line + strspn(line, " \t");
			p[strcspn(p, "\r\n")] = 0;
			char* value = strchr(p, '=');
			if (*p == '[') {
				inData = _strnicmp(p, "[Data]", 6) == 0;
			} else if (inData && isdigit((unsigned char)*p) && value && value[1]) {
				int key = atoi(p);
				int i = numPak++;
				for (; i > 0 && keys[i - 1] > key; --i) {
					keys[i] = keys[i - 1];
					strcpy(names[i], names[i - 1]);
				}
				keys[i] = key;
				strncpy(names[i], value + 1, MAX_PATH - 1);
				names[i][MAX_PATH - 1] = 0;
			}
		}
		fclose(fp);
	}
	if (numPak == 0)
		strcpy(names[numPak++], "data.grf");

	// CFileMgr searches the archive added last first.
	while (numPak > 0)
		g_fileMgr.AddPak(names[--numPak]);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					 LPSTR lpCmdLine, int nCmdShow)
{
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ExceptionFilter);


	if (strstr(lpCmdLine, "1rag1")  || strstr(lpCmdLine, "1sak1") ||
		strstr(lpCmdLine, "1gld1")  || strstr(lpCmdLine, "1rag12") ||
		strstr(lpCmdLine, "1rag23") || strstr(lpCmdLine, "1rag34"))
	{
	}
	if (strstr(lpCmdLine, "2rag2"))
		g_EXE_way = 2;


	AddPaksFromDataIni();
	g_session.m_isCheckToPcbang = (strstr(lpCmdLine, "pc") != NULL);



	char clientinfofile[64] = "sclientinfo.xml";
	if (strstr(lpCmdLine, "1pvpsak1"))
		strcpy(clientinfofile, "psakclientinfo.xml");

	char* pos;
	if ((pos = strstr(lpCmdLine, "/account:")) != NULL)
	{
		pos += strlen("/account:");
		char* dst = clientinfofile;
		while (*pos && *pos != '\t' && *pos != ' ')
		{
			*dst = *pos;
			dst++;
			pos++;
		}
		*dst = 0;
	}
	InitClientInfo(clientinfofile);

	if (g_serviceType == 3 && strstr(clientinfofile, "clientinfo"))
		g_bUseCommand = false;


	if (UpdatePatch(PATCH_CLIENT, PATCH_UP))
		return 0;

	g_session.Init();


	ReadRegistry();

#ifndef BF_PORT
	_controlfp(0x20000, 0x30000);
#endif
	// _PC_24 under _MCW_PC: x87 precision control, which arm64 does not
	// have. Floats are computed as floats there; doubles stay doubles where
	// the x87 rounded them to 24 bits (only sprite extents and one line
	// test use doubles).

	UINT wTimerRes = 0;
	TIMECAPS tc;
	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR)
	{
		wTimerRes = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);
		timeBeginPeriod(wTimerRes);
	}

	ResetTimer();
	CoInitialize(NULL);
	g_session.Create();

	g_resMgr().ReadResNameTable("resNameTable.txt");

#if defined(BF_PORT)
	if (bf_port_headless()) {
		// --headless (M3.7): the start-up up to here, the login mode
		// constructed, and no window, device or frame.
		CMode* login = new CLoginMode;
		// Under `file`, beside the lookups it sums up (the M3.7 check reads that category).
		BF_TRACE(BFT_FILE, "headless: login mode constructed; stopping before the window and the first frame");
		delete login;
		return 0;
	}
#endif
	if (!InitApp(hInstance, nCmdShow)) return 1;
#if defined(BF_PORT)
	if (!bf_port_start_graphics()) return 1;
#endif

	if (!SetupDriver2()) return 1;

	extern CRenderer* g_renderer;
	g_renderer = g_3dDevice.CreateRenderer();
	if (!CConnection::Startup()) return 1;

	InitTimer(60);
	SetFocus(g_hMainWnd);
	if (g_3dDevice.IsFullscreen())
		ShowCursor(FALSE);
	SetHwnd();
	g_mouse.Init();

	extern C3dGrannyBoneRes g_GrannyBoneRes;
	extern CSkinMgr g_skinMgr;
	extern CTexMgr g_texMgr;

	g_windowMgr.SetSize(g_renderer->m_width, g_renderer->m_height);
	g_windowMgr.InitUIRectInfoFromReg();
	g_GrannyBoneRes.Init();
	g_GrannyBoneRes.AddBone("model\\3dmob\\guildflag90_1.gr2", 5, NULL);
	g_skinMgr.Init();
	CreateTrigonometricTable();

	if (g_serviceType == ServiceTaiwan || g_serviceType == ServiceMalaysia || g_serviceType == ServiceSingapore
		|| (g_serviceType == ServiceIndonesia && g_serverType == ServerPrimary)
		|| g_serviceType == ServiceBrazil || g_serviceType == ServiceRussia)
		g_loginStartMode = 1;
	else if (g_serverType != ServerPrimary && g_serviceType != ServiceJapan
		&& g_serverType != ServerPK && g_serviceType != ServiceChina)
		g_loginStartMode = 0;
	else
		g_loginStartMode = g_serviceType ? 2 : 0;

	g_modeMgr.Run(MT_LOGIN, "login.rsw");

	UnInitMSS();
	g_windowMgr.DestroyDC();
	CRagConnection::instanceR().Disconnect();
	CConnection::Cleanup();
	g_windowMgr.RemoveAllWindows();
	g_windowMgr.WriteUIRectInfoToReg();
	g_session.WriteOptionToRegistry();
	g_resMgr().Reset();
	g_texMgr.DestroyAllTexture();
	g_3dDevice.DestroyRenderer(g_renderer);
	g_3dDevice.DestroyObjects();
	g_mouse.Exit();

	CheckSystemMessage();
	CoUninitialize();
	DestroyWindow(g_hMainWnd);
	DeleteHack();
	CoUninitialize();
	if (wTimerRes) timeEndPeriod(wTimerRes);
	return 0;
}
