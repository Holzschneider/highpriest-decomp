#include <windows.h>
#include "common.h"
#include "MsgStrings.h"








#include "cSecureGamePack.h"
#include "Mode.h"
#include "device/Connection.h"

#include <stdio.h>

extern UIWindowMgr g_windowMgr;
extern CModeMgr    g_modeMgr;

const char* MsgStr(MSGSTRINGID id);




#define SGP_DLL      "IMESCHLP"
#define SGP_ENTRY    "QueryIMEHelper"
#define SGP_VERSION  "5,01,2600,1832\0"

cSecureGamePack* cSecureGamePack::m_pInst = 0;

cSecureGamePack::cSecureGamePack()
{
	if (m_pInst == 0)
		m_pInst = this;
}





cSecureGamePack::~cSecureGamePack()
{
	HMODULE hLib = ::LoadLibraryA(SGP_DLL);
	if (hLib)
	{
		FARPROC (__stdcall *pfn)(unsigned int, const char*) =
			(FARPROC (__stdcall *)(unsigned int, const char*))::GetProcAddress(hLib, SGP_ENTRY);
		if (pfn)
		{
			void (__cdecl *cleanup)(const char*) =
				(void (__cdecl *)(const char*))pfn(0x9000F004, SGP_VERSION);
			if (cleanup)
				cleanup(SGP_VERSION);
			::FreeLibrary(::GetModuleHandleA(SGP_DLL));
			::OutputDebugStringA("Secure Game Pack\xc0\xcc \xc1\xa4\xbb\xf3\xc0\xfb\xc0\xb8\xb7\xce \xc7\xd8\xc1\xa6\xc7\xcf\xbf\xb4\xbd\xc0\xb4\xcf\xb4\xd9");
		}
	}
}

void cSecureGamePack::Destroy()
{



	if (m_pInst)
	{
		delete m_pInst;
		m_pInst = 0;
	}
}

bool cSecureGamePack::InitSecureGamePack()
{



	char strMsg[256];
	DWORD rc;
	unsigned int (__cdecl *pError)();

	FARPROC (__stdcall *pfn)(unsigned int, const char*) =
		(FARPROC (__stdcall *)(unsigned int, const char*))::GetProcAddress(::LoadLibraryA(SGP_DLL), SGP_ENTRY);
	if (pfn == 0)
	{
		::MessageBoxA(g_hMainWnd,
					  "IMESCHLP.dll\xc0\xbb \xb7\xce\xb5\xe5\xc7\xcf\xb4\xc2\xb5\xa5 \xbd\xc7\xc6\xd0",
					  ":::Warning:::",
					  MB_ICONINFORMATION);
		return 0;
	}

	char szBuffer[128] = {0};


	int (__cdecl *pInit)(HWND, char*, void (__cdecl*)(void*), const char*) =
		(int (__cdecl *)(HWND, char*, void (__cdecl*)(void*), const char*))pfn(0x9000F002, SGP_VERSION);
	if (pInit)
	{
		rc = pInit(g_hMainWnd, szBuffer, OnNotifyFromSGP, SGP_VERSION);
		switch (rc)
		{
		case 0x8000E001:
			{
				g_windowMgr.ErrorMsg(MsgStr(MSI_SGP_CODE_ALREADY_RUNNING), ME_OK, 1, 0, 0);

				LeaveSecureGamePack();
			}
			break;
		case 1:
			{

				int (__cdecl *pVerify)(HWND, char*) = (int (__cdecl *)(HWND, char*))pfn(0x9000F001, SGP_VERSION);
				if (pVerify)
				{
					if (pVerify(g_hMainWnd, szBuffer) == 0)
					{
						::MessageBoxA(g_hMainWnd, MsgStr(MSI_SGP_CODE_NOT_EQUALENCRYPTEDDATA), "::: Warning :::", 1);
						return 0;
					}
				}
				::OutputDebugStringA("Secure Game Pack\xc0\xcc \xc1\xa4\xbb\xf3\xc0\xfb\xc0\xb8\xb7\xce \xbd\xc7\xc7\xe0\xb5\xc7\xbe\xfa\xbd\xc0\xb4\xcf\xb4\xd9");
			}
			break;
		}


		if (rc != 1)
		{
			pError = (unsigned int (__cdecl *)())pfn(0x9000F005, SGP_VERSION);
			if (pError)
			{
				sprintf(strMsg,
						"\xb3\xbb\xba\xce\xc0\xfb\xc0\xce \xbf\xc0\xb7\xf9\xb0\xa1 \xb9\xdf\xbb\xfd\xc7\xcf\xbf\xa9 \xb4\xf5 \xc0\xcc\xbb\xf3 \xc1\xf8\xc7\xe0\xc7\xd2 \xbc\xf6 \xbe\xf8\xbd\xc0\xb4\xcf\xb4\xd9     \n\n\xbf\xa1\xb7\xaf \xc4\xda\xb5\xe5: 0x%08X, 0x%08X",
						rc,
						pError());
				::MessageBoxA(g_hMainWnd, strMsg, ":::Warning:::", MB_OK);
			}
		}
	}

	return 1;
}

void cSecureGamePack::OnNotifyFromSGP(void* lpNotifyData)
{




	unsigned int event = *((unsigned int*)lpNotifyData);

	switch (event)
	{
	case 0x8000E001:
		g_windowMgr.ErrorMsg(MsgStr(MSI_SGP_CODE_ALREADY_RUNNING),     ME_OK, 1, 0, 0);
		break;
	case 0x8000E003:
		g_windowMgr.ErrorMsg(MsgStr(MSI_SGP_CODE_DETECTED_MACRO),      ME_OK, 1, 0, 0);
		break;
	case 0x8000E004:
		g_windowMgr.ErrorMsg(MsgStr(MSI_SGP_CODE_DETECTED_SPEEDHACK),  ME_OK, 1, 0, 0);
		break;
	case 0x8000E005:
		g_windowMgr.ErrorMsg(MsgStr(MSI_SGP_CODE_DETECTED_APIHOOK),    ME_OK, 1, 0, 0);
		break;
	case 0x8000E006:
		g_windowMgr.ErrorMsg(MsgStr(MSI_SGP_CODE_DETECTED_MSGHOOK),    ME_OK, 1, 0, 0);
		break;
	default:
		break;
	}

	FARPROC (__stdcall *pfn)(unsigned int, const char*) =
		(FARPROC (__stdcall *)(unsigned int, const char*))::GetProcAddress(::LoadLibraryA(SGP_DLL), SGP_ENTRY);
	if (pfn)
	{
		int (__cdecl *leave)(HWND, const char*) =
			(int (__cdecl *)(HWND, const char*))pfn(0x9000F003, SGP_VERSION);
		if (leave)
			leave(g_hMainWnd, SGP_VERSION);
	}

	CRagConnection::instanceR().Disconnect();
	g_modeMgr.Quit();
}

void cSecureGamePack::DestroySecureGamePack()
{
	HMODULE hLib = ::LoadLibraryA(SGP_DLL);
	if (hLib)
	{
		FARPROC (__stdcall *pfn)(unsigned int, const char*) =
			(FARPROC (__stdcall *)(unsigned int, const char*))::GetProcAddress(hLib, SGP_ENTRY);
		if (pfn)
		{
			void (__cdecl *cleanup)(const char*) =
				(void (__cdecl *)(const char*))pfn(0x9000F004, SGP_VERSION);
			if (cleanup)
				cleanup(SGP_VERSION);
			::FreeLibrary(::GetModuleHandleA(SGP_DLL));
			::OutputDebugStringA("Secure Game Pack\xc0\xcc \xc1\xa4\xbb\xf3\xc0\xfb\xc0\xb8\xb7\xce \xc7\xd8\xc1\xa6\xc7\xcf\xbf\xb4\xbd\xc0\xb4\xcf\xb4\xd9");
		}
	}
}

void cSecureGamePack::LeaveSecureGamePack()
{
	FARPROC (__stdcall *pfn)(unsigned int, const char*) =
		(FARPROC (__stdcall *)(unsigned int, const char*))::GetProcAddress(::LoadLibraryA(SGP_DLL), SGP_ENTRY);
	if (pfn)
	{
		int (__cdecl *leave)(HWND, const char*) =
			(int (__cdecl *)(HWND, const char*))pfn(0x9000F003, SGP_VERSION);
		if (leave)
			leave(g_hMainWnd, SGP_VERSION);
	}
}
