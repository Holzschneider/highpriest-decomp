#ifndef HIGHPRIEST_BASE_FINDHACK_H
#define HIGHPRIEST_BASE_FINDHACK_H

// The process and module scan of the keyboard protection's error report
// (base/FindHack.cpp): not in the port.
#ifndef BF_PORT


typedef VOID (WINAPI *FEnumModuleCallback)(HMODULE hMod, PVOID pUserData);

typedef BOOL (WINAPI *FLookupPrivilegeValue)(LPCTSTR, LPCTSTR, PLUID);
typedef BOOL (WINAPI *FAdjustTokenPrivileges)(HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);
typedef BOOL (WINAPI *FOpenProcessToken)(HANDLE, DWORD, PHANDLE);

typedef BOOL (WINAPI *FEnumProcessModules)(HANDLE, HMODULE *, DWORD, LPDWORD);
typedef DWORD (WINAPI *FGetModuleFileNameEx)(HANDLE, HMODULE, LPTSTR, DWORD);

typedef HANDLE (WINAPI *FCreateToolhelp32Snapshot)(DWORD, DWORD);
typedef BOOL (WINAPI *FModule32First)(HANDLE, LPMODULEENTRY32);
typedef BOOL (WINAPI *FModule32Next)(HANDLE, LPMODULEENTRY32);
typedef BOOL (WINAPI *FProcess32First)(HANDLE, LPPROCESSENTRY32);
typedef BOOL (WINAPI *FProcess32Next)(HANDLE, LPPROCESSENTRY32);

class IProcess
{
private:
	FEnumProcessModules			fnEnumProcessModules;
	FGetModuleFileNameEx		fnGetModuleFileNameEx;

	FCreateToolhelp32Snapshot	fnCreateToolhelp32Snapshot;
	FProcess32First				fnProcess32First;
	FProcess32Next				fnProcess32Next;
	FModule32First				fnModule32First;
	FModule32Next				fnModule32Next;

	HINSTANCE					m_hDll;
	BOOL						m_bErr;
	BOOL						m_bNT;

protected:
	IProcess();

	VOID Unload();
	VOID Load();
	VOID Load9X();
	VOID LoadNT();

	static BOOL GetProcAddresses( HINSTANCE *hLibrary, LPCSTR lpszLibrary, INT nCount, ... );
	static BOOL IsWin9X();

	BOOL GetProcessModule(DWORD dwPID, DWORD dwModuleID, LPMODULEENTRY32 lpMe32, DWORD cbMe32);
	BOOL EnableDebugPrivilege(BOOL bEnable);

	BOOL EnumModuleNT(DWORD dwPid, FEnumModuleCallback FEnum, PVOID pUserData);
	BOOL EnumModule9X(DWORD dwPid, FEnumModuleCallback FEnum, PVOID pUserData);

public:
	~IProcess();

	BOOL Bad();
	static IProcess *Inst();

	BOOL EnumModule(DWORD dwPid, FEnumModuleCallback FEnum, PVOID pUserData);
};

#endif // BF_PORT

#endif
