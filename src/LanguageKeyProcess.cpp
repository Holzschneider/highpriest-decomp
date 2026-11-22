#include "common.h"







#include "LanguageKeyProcess.h"
#include "VietnamLanguage.h"


#include "3dlib/Prim.h"

namespace {
const UINT WM_KEYPROCESS = 0x464;
}

CLanguageKeyProcess::CLanguageKeyProcess()
{
	m_hWnd = NULL;
	m_pForeignLang = NULL;
	m_lastOutput = 0;
	m_useLanguageMode = 0;
}

CLanguageKeyProcess::CLanguageKeyProcess(HWND__* hWnd)
{
	m_hWnd = hWnd;
	m_pForeignLang = NULL;
	m_lastOutput = 0;
	m_useLanguageMode = 0;
}

CLanguageKeyProcess::~CLanguageKeyProcess()
{
}

int CLanguageKeyProcess::InitKeyProcess(HWND__* hWnd, ECountry name, EInputMode type)
{
	m_hWnd = hWnd;

	if (name == KEYPROCESS_LANG_VIETNAM) {
		if (m_pForeignLang) {
			delete m_pForeignLang;
		}
		m_pForeignLang = new CVietLang;
	}

	if (m_pForeignLang) {
		return m_pForeignLang->InitNativeLanguage(type);
	}
	return 0;
}

void CLanguageKeyProcess::DestroyKeyProcess()
{
	if (m_pForeignLang) {
		m_pForeignLang->DestroyForeignLanguage();
		delete m_pForeignLang;
	}
}

int CLanguageKeyProcess::ChangeInputMode(EInputMode type)
{
	if (m_pForeignLang) {
		return m_pForeignLang->ChangeInputMode(type);
	}
	return 0;
}

int CLanguageKeyProcess::IsExpend(char inChar)
{
	if (m_useLanguageMode && m_pForeignLang->IsExpend(inChar)) {
		return 1;
	}
	int code = inChar;
	SendMessageA((HWND)m_hWnd, WM_KEYPROCESS, code, 1);
	m_lastOutput = code;
	return 0;
}

unsigned short CLanguageKeyProcess::GetMBCSCode(char inChar, unsigned int* pVietIndex)
{
	unsigned short resultChar = 0xffff;
	int isStart = 1;

	ECurInputStatus status;
	if ((status = m_pForeignLang->FindOffsetMBCS(inChar, &resultChar, &isStart)) == KEYPROCESS_INPUTSTATUS_CONTINUE) {
		if (isStart == 1)
			NewChar(resultChar);
		else
			ContinueChar(resultChar);
	} else if (status == KEYPROCESS_INPUTSTATUS_START) {

	} else if (status == KEYPROCESS_INPUTSTATUS_WAIT) {
		CompleteChar(resultChar);
		NewChar(inChar);
	}

	return m_completeChar;
}

void CLanguageKeyProcess::ResetInput()
{
	if (m_pForeignLang) {
		m_pForeignLang->ResetInput(1);
	}
}

void CLanguageKeyProcess::ProcessKeyDown(unsigned int wParam)
{
	switch (wParam) {
	case VK_CONTROL:
		m_bChangeKeyCtrl = 1;
		break;
	case VK_MENU:
		m_bChangeKeyShift = 1;
		break;
	}
}

void CLanguageKeyProcess::ProcessKeyUp(unsigned int wParam)
{
	if (m_bChangeKeyCtrl != 0 && m_bChangeKeyShift != 0) {
		m_useLanguageMode = (m_useLanguageMode == 0);
	}

	switch (wParam) {
	case VK_CONTROL:
		m_bChangeKeyCtrl = 0;
		break;
	case VK_MENU:
		m_bChangeKeyShift = 0;
		break;
	}
}

void CLanguageKeyProcess::CompleteChar(unsigned int inChar)
{
	if (((unsigned char*)&m_lastOutput)[1]) {
		SendMessageA((HWND)m_hWnd, WM_KEYPROCESS, VK_BACK, 1);
	}
	SendMessageA((HWND)m_hWnd, WM_KEYPROCESS, VK_BACK, 1);
	SendMessageA((HWND)m_hWnd, WM_KEYPROCESS, inChar, 1);
	m_lastOutput = (unsigned short)inChar;
}

void CLanguageKeyProcess::ContinueChar(unsigned int inChar)
{
	if (((unsigned char*)&m_lastOutput)[1]) {
		SendMessageA((HWND)m_hWnd, WM_KEYPROCESS, VK_BACK, 1);
	}
	SendMessageA((HWND)m_hWnd, WM_KEYPROCESS, VK_BACK, 1);
	SendMessageA((HWND)m_hWnd, WM_KEYPROCESS, inChar, 1);
	m_lastOutput = (unsigned short)inChar;
}

void CLanguageKeyProcess::NewChar(unsigned int inChar)
{
	SendMessageA((HWND)m_hWnd, WM_KEYPROCESS, inChar, 1);
	m_lastOutput = (unsigned short)inChar;
}
