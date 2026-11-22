#ifndef HIGHPRIEST_VIETNAMLANGUAGE_H
#define HIGHPRIEST_VIETNAMLANGUAGE_H


enum EInputMode {
	KEYPROCESS_INPUTMODE_VIET_VNI = 0,
	KEYPROCESS_INPUTMODE_VIET_TELEX = 1,
};

enum ECurInputStatus {
	KEYPROCESS_INPUTSTATUS_WAIT = 1001,
	KEYPROCESS_INPUTSTATUS_START = 1002,
	KEYPROCESS_INPUTSTATUS_CONTINUE = 1003,
	KEYPROCESS_INPUTSTATUS_END = 1004,
};

class CInputMode;
class CVietLang;

class CVietLang {
public:
	CVietLang();
	int IsExpend(char inChar);
	ECurInputStatus FindOffsetMBCS(char inChar, unsigned short* result, int* start);
	int InitNativeLanguage(EInputMode mode);
	int DestroyForeignLanguage();
	int ChangeInputMode(EInputMode mode);
	unsigned int GetCount();
	ECurInputStatus GetCurStatus();
	void ResetInput(int outer);
	EInputMode GetInputMode();
private:
	void ChangeCurStatus(ECurInputStatus Status);
private:
	CInputMode* m_pKeyMode;
	EInputMode m_inputMode;
	unsigned int m_keyCount;
	char m_inputKeyBuf[10];
	unsigned short m_outputKeyBuf[10];
	char m_oriKey;
	ECurInputStatus m_curStatus;
	unsigned int m_offset;
};

#endif
