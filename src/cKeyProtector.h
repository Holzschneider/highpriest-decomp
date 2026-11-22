#ifndef HIGHPRIEST_CKEYPROTECTOR_H
#define HIGHPRIEST_CKEYPROTECTOR_H

#include "framework/Locale.h"

class cKeyProtector;
struct HINSTANCE__;
struct HWND__;

class cKeyProtector {
	typedef int (*pfLoadSCSK2)();
	typedef int (*pfStartSCSK)(HWND__*);
	typedef int (*pfStopSCSK)();
	typedef unsigned int (*pfGetRealAscii)();
private:
	bool InitKeyProtector(ServiceType eSType);
protected:
	static cKeyProtector* m_pInst;
	cKeyProtector(ServiceType eSType);
public:
	~cKeyProtector();
	static cKeyProtector* __cdecl GetInstance();
	static void __cdecl Create(ServiceType);
	static void __cdecl Destroy();
	static unsigned char __cdecl IsAvailable();
	int Start(HWND__* hWnd);
	int Stop();
	bool GetRealAsciiCode(unsigned int& wParam);
private:
	int m_bSuccessSCSK;
	int m_bStartSCSK;
	HINSTANCE__* m_hSCSK;
	int (*m_lpLoadSCSK2)();
	int (*m_lpStartSCSK)(HWND__*);
	int (*m_lpStopSCSK)();
	unsigned int (*m_lpGetRealAscii)();
	ServiceType m_eServiceType;
};

#endif
