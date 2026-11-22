#include "common.h"




#include "cKeyProtector.h"

cKeyProtector* cKeyProtector::m_pInst = NULL;

cKeyProtector::cKeyProtector(ServiceType eSType)
{
	if (m_pInst == NULL && InitKeyProtector(eSType))
		m_pInst = this;
}

cKeyProtector::~cKeyProtector()
{
	if (m_eServiceType != ServiceVietnam)
		return;
	if (m_bStartSCSK == 0)
		return;
	if (m_bSuccessSCSK == 0)
		return;

	int ret = m_lpStopSCSK();
	if (ret != 0) {
		OutputDebugStringA("\xC5\xB0\xBA\xB8\xB5\xE5 \xBA\xB8\xBE\xC8 \xC1\xA4\xC1\xF6");
		m_bStartSCSK = 0;
	} else {
		OutputDebugStringA("\xC5\xB0\xBA\xB8\xB5\xE5 \xBA\xB8\xBE\xC8 \xC1\xA4\xC1\xF6 \xBD\xC7\xC6\xD0");
		m_bStartSCSK = 1;
	}
}

bool cKeyProtector::InitKeyProtector(ServiceType eSType)
{
	if (eSType != ServiceVietnam) return false;

	bool bOk = false;

	m_bSuccessSCSK = 0;
	m_bStartSCSK = 0;
	m_hSCSK = NULL;
	m_lpLoadSCSK2 = NULL;
	m_lpStartSCSK = NULL;
	m_lpStopSCSK = NULL;
	m_lpGetRealAscii = NULL;
	m_eServiceType = eSType;

	m_hSCSK = LoadLibraryA("SCSKAppLink.dll");
	if (m_hSCSK != NULL) {
		m_lpLoadSCSK2    = (int (*)())GetProcAddress(m_hSCSK, "LoadSCSK");
		m_lpStartSCSK    = (int (*)(HWND__*))GetProcAddress(m_hSCSK, "StartSCSK");
		m_lpStopSCSK     = (int (*)())GetProcAddress(m_hSCSK, "StopSCSK");
		m_lpGetRealAscii = (unsigned int (*)())GetProcAddress(m_hSCSK, "GetRealAscii");

		m_bSuccessSCSK = m_lpLoadSCSK2();
		if (m_bSuccessSCSK != 0) {
			OutputDebugStringA("\xC5\xB0\xBA\xB8\xB5\xE5 \xBA\xB8\xBE\xC8 DLL\xC0\xBB \xC0\xBB \xBC\xBA\xB0\xF8\xC0\xFB\xC0\xB8\xB7\xCE Load \xC7\xCF\xBF\xB4\xBD\xC0\xB4\xCF\xB4\xD9.\n");
			bOk = true;
		} else {
			OutputDebugStringA("\xC5\xB0\xBA\xB8\xB5\xE5 \xBA\xB8\xBE\xC8\xC0\xBB Load\xC7\xCF\xB4\xC2\xB5\xA5 \xBD\xC7\xC6\xD0\xC7\xCF\xBF\xB4\xBD\xC0\xB4\xCF\xB4\xD9.\n");
			bOk = false;
		}
	} else {
		OutputDebugStringA("\xC5\xB0\xBA\xB8\xB5\xE5 \xBA\xB8\xBE\xC8 DLL\xC0\xBB \xB7\xCE\xB5\xE5\xC7\xCF\xB4\xC2\xB5\xA5 \xBD\xC7\xC6\xD0\xC7\xCF\xBF\xB4\xBD\xC0\xB4\xCF\xB4\xD9.\n");
	}

	return bOk;
}

int cKeyProtector::Start(HWND__ *hWnd)
{
	if (m_eServiceType != ServiceVietnam)
		return 0;

	int ret = 0;
	if (m_bStartSCSK == 0 && m_bSuccessSCSK != 0) {
		ret = m_lpStartSCSK(hWnd);
		if (ret != 0)
			OutputDebugStringA("\xC5\xB0\xBA\xB8\xB5\xE5 \xBA\xB8\xBE\xC8 \xBD\xBA\xC5\xB8\xC6\xAE");
		else
			OutputDebugStringA("\xC5\xB0\xBA\xB8\xB5\xE5 \xBA\xB8\xBE\xC8 \xBD\xBA\xC5\xB8\xC6\xAE \xBD\xC7\xC6\xD0");
		m_bStartSCSK = (ret != 0);
	}
	return ret;
}

int cKeyProtector::Stop()
{
	if (m_eServiceType != ServiceVietnam)
		return 0;

	int ret = 0;
	if (m_bStartSCSK != 0 && m_bSuccessSCSK != 0) {
		ret = m_lpStopSCSK();
		if (ret != 0)
			OutputDebugStringA("\xC5\xB0\xBA\xB8\xB5\xE5 \xBA\xB8\xBE\xC8 \xC1\xA4\xC1\xF6");
		else
			OutputDebugStringA("\xC5\xB0\xBA\xB8\xB5\xE5 \xBA\xB8\xBE\xC8 \xC1\xA4\xC1\xF6 \xBD\xC7\xC6\xD0");
		m_bStartSCSK = (ret == 0);
	}
	return ret;
}

bool cKeyProtector::GetRealAsciiCode(unsigned int &wParam)
{
	if (m_eServiceType != ServiceVietnam)
		return false;

	bool bOk = false;
	if (m_bStartSCSK != 0 && wParam == 0x2A) {
		unsigned int code = m_lpGetRealAscii();
		if (code != 0) {
			wParam = code;
			bOk = true;
		}
	}
	return bOk;
}
