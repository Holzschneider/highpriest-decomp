#ifndef HIGHPRIEST_LANGUAGEKEYPROCESS_H
#define HIGHPRIEST_LANGUAGEKEYPROCESS_H

#include "VietnamLanguage.h"

enum ECountry {
	KEYPROCESS_LANG_ENGLISH = 101,
	KEYPROCESS_LANG_VIETNAM = 102,
};

class CLanguageKeyProcess;
struct HWND__;

class CLanguageKeyProcess {
public:
	CLanguageKeyProcess(HWND__* hWnd);
	CLanguageKeyProcess();
	~CLanguageKeyProcess();
	int InitKeyProcess(HWND__* hWnd, ECountry name, EInputMode type);
	void DestroyKeyProcess();
	int ChangeInputMode(EInputMode type);
	void ProcessKeyDown(unsigned int wParam);
	void ProcessKeyUp(unsigned int wParam);
	HWND__* GetWndHandle();
	int IsExpend(char inChar);
	unsigned short GetMBCSCode(char inChar, unsigned int* pVietIndex);
	void UseNationalLanguage();
	void NotUseNationalLanguage();
	int IsUseNationalLanguage();
	void ResetInput();
	EInputMode GetInputMode();
private:
	void CompleteChar(unsigned int wParam);
	void ContinueChar(unsigned int wParam);
	void NewChar(unsigned int wParam);
private:
	HWND__* m_hWnd;
	CVietLang* m_pForeignLang;
	unsigned short m_completeChar;
	int m_useLanguageMode;
	unsigned short m_lastOutput;
	int m_bChangeKeyCtrl;
	int m_bChangeKeyShift;
};

#endif
