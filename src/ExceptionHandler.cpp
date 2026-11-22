#include "common.h"
#include "Session.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define _IMAGEHLP_SOURCE_

#pragma pack(push,exception)
#pragma pack(8)
#include "dbghelp.h"
#pragma pack(pop,exception)

#pragma intrinsic(memset, strcpy, strlen)

extern char exception_log[];

char exception_log[8096];

typedef BOOL(IMAGEAPI* enumerateloadedmodules)(HANDLE, PENUMLOADED_MODULES_CALLBACK, PVOID);
typedef BOOL(IMAGEAPI* stackwalk)(DWORD, HANDLE, HANDLE, LPSTACKFRAME, PVOID, PREAD_PROCESS_MEMORY_ROUTINE, PFUNCTION_TABLE_ACCESS_ROUTINE, PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE);
typedef DWORD (IMAGEAPI* gettimestamp)(HMODULE);








INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT windowRect;

	switch (uMsg)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == 2)
		{
			HGLOBAL hMem = GlobalAlloc(GMEM_DDESHARE, strlen(exception_log) + 1);
			if (hMem)
			{
				char* dst = (char*)GlobalLock(hMem);
				if (!dst)
				{
					GlobalFree(hMem);
					return TRUE;
				}

				strcpy(dst, exception_log);
				GlobalUnlock(hMem);

				if (OpenClipboard(hwndDlg))
				{
					EmptyClipboard();
					SetClipboardData(CF_TEXT, hMem);
					CloseClipboard();
				}

				SetFocus(GetDlgItem(hwndDlg, 7));
				return TRUE;
			}
		}

		if (LOWORD(wParam) == 7)
		{
			EndDialog(hwndDlg, 7);
			return TRUE;
		}
		return TRUE;

	case WM_INITDIALOG:
		HWND text = GetDlgItem(hwndDlg, 0x3EE);
		SetWindowTextA(text, exception_log);
		GetWindowRect(hwndDlg, &windowRect);
		MoveWindow(hwndDlg,
			GetSystemMetrics(SM_CXSCREEN) / 2 - (windowRect.right - windowRect.left) / 2,
			GetSystemMetrics(SM_CYSCREEN) / 2 - (windowRect.bottom - windowRect.top) / 2,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			TRUE);
		return TRUE;
	}

	return FALSE;
}




BOOL CALLBACK EnumerateLoadedModulesProc(PSTR ModuleName, ULONG ModuleBase, ULONG ModuleSize, PVOID UserContext)
{
	ULONG eip = *(ULONG*)UserContext;
	if (eip < ModuleBase || eip > ModuleBase + ModuleSize)
		return TRUE;

	sprintf(exception_log + strlen(exception_log), "%s", ModuleName);
	return FALSE;
}

LONG CALLBACK ExceptionFilter(_EXCEPTION_POINTERS* pExceptionInfo)
{
	CONTEXT* context = pExceptionInfo->ContextRecord;
	HINSTANCE hDbgHelp;

	if ((hDbgHelp = LoadLibraryA("dbghelp.dll")) != NULL)
	{
		stackwalk dbgStackWalk;
		enumerateloadedmodules dbgEnumerateLoadedModules;
		HANDLE hProcess;
		gettimestamp dbgGetTimeStamp;
		char module_name[256];
		long module_time;
		HMODULE hModule;
		HANDLE hThread;
		DWORD pc;
		DWORD sp;
		int frameCount;
		int col;
		int row;
		unsigned char* stack;
		int index;

		dbgStackWalk = (stackwalk)GetProcAddress(hDbgHelp, "StackWalk");
		dbgGetTimeStamp = (gettimestamp)GetProcAddress(hDbgHelp, "GetTimestampForLoadedLibrary");
		dbgEnumerateLoadedModules =
			(enumerateloadedmodules)GetProcAddress(hDbgHelp, "EnumerateLoadedModules");

		hModule = GetModuleHandleA(NULL);
		GetModuleFileNameA(hModule, module_name, 256);
		module_time = dbgGetTimeStamp(hModule);

		sprintf(exception_log + strlen(exception_log), "Module Name: %s\r\n", module_name);
		sprintf(exception_log + strlen(exception_log), "Time Stamp: 0x%08x - %s\r\n",
			module_time, ctime(&module_time));
		sprintf(exception_log + strlen(exception_log), "\r\n");
		sprintf(exception_log + strlen(exception_log), "Exception Type: 0x%08x\r\n",
			pExceptionInfo->ExceptionRecord->ExceptionCode);
		sprintf(exception_log + strlen(exception_log), "\r\n");

		hProcess = GetCurrentProcess();
		hThread = GetCurrentThread();
		pc = context->Eip;
		STACKFRAME stackFrame = {0};
		sp = context->Esp;
		stackFrame.AddrPC.Offset = pc;
		stackFrame.AddrPC.Mode = AddrModeFlat;
		stackFrame.AddrStack.Offset = sp;
		stackFrame.AddrStack.Mode = AddrModeFlat;
		stackFrame.AddrFrame.Offset = context->Ebp;
		stackFrame.AddrFrame.Mode = AddrModeFlat;

		for (frameCount = 0; frameCount < 512; frameCount++)
		{
			if (!stackFrame.AddrPC.Offset)
				break;

			if (!dbgStackWalk(IMAGE_FILE_MACHINE_I386, hProcess, hThread, &stackFrame,
				context, NULL, NULL, NULL, NULL))
				break;

			sprintf(exception_log + strlen(exception_log), "0x%08x\t", stackFrame.AddrPC.Offset);
			dbgEnumerateLoadedModules(GetCurrentProcess(), EnumerateLoadedModulesProc,
				&stackFrame.AddrPC.Offset);
			sprintf(exception_log + strlen(exception_log), "\r\n");
		}

		sprintf(exception_log + strlen(exception_log), "\r\n");
		sprintf(exception_log + strlen(exception_log), "eax: 0x%08x\tebx: 0x%08x\r\n",
			context->Eax, context->Ebx);
		sprintf(exception_log + strlen(exception_log), "ecx: 0x%08x\tedx: 0x%08x\r\n",
			context->Ecx, context->Edx);
		sprintf(exception_log + strlen(exception_log), "esi: 0x%08x\tedi: 0x%08x\r\n",
			context->Esi, context->Edi);
		sprintf(exception_log + strlen(exception_log), "ebp: 0x%08x\tesp: 0x%08x\r\n",
			context->Ebp, context->Esp);
		context = (CONTEXT*)context->Esp;
		sprintf(exception_log + strlen(exception_log), "\r\n");

		sprintf(exception_log + strlen(exception_log), "stack %08x - %08x\r\n",
			context, (unsigned char*)context + 0x400);
		stack = (unsigned char*)context;

		for (row = 0; row < 16; row++)
		{
			sprintf(exception_log + strlen(exception_log), "%08X : ", stack + row * 16);
			for (col = 0; col < 16; col++)
			{
				DWORD value = stack[row * 16 + col];
				sprintf(exception_log + strlen(exception_log), "%02X ", value);
			}
			sprintf(exception_log + strlen(exception_log), "\r\n");
		}

		sprintf(exception_log + strlen(exception_log), "\r\nLaunch Info \r\n");
		index = 0;
		for (row = 0; row < 4; row++)
		{
			for (col = 0; col < 8; col++)
			{
				sprintf(exception_log + strlen(exception_log), "%04X ", g_recentEffectId[index]);
				index++;
			}
			sprintf(exception_log + strlen(exception_log), "\r\n");
		}

		sprintf(exception_log + strlen(exception_log), "\r\nJob : %s\r\n",
			g_session.GetJobName(g_session.GetJob()));

		FreeLibrary(hDbgHelp);
	}
	else
	{
		sprintf(exception_log + strlen(exception_log), "Can't find DbgHelp.DLL !!!");
	}

	DialogBoxParamA(GetModuleHandleA(NULL), (LPCSTR)0x75, NULL, DialogProc, 0);
	return EXCEPTION_EXECUTE_HANDLER;
}
