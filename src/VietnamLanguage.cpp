#include <windows.h>
#include "VietnamLanguage.h"
#include "VniInputMode.h"
#include "telexInputMode.h"

#include <string.h>

unsigned short lowerCodeMap[] = {
	60513, 52321, 53857, 56929, 62049,    97,
	60642, 52450, 53986, 57058, 62178,   226,
	60643, 52451, 53987, 57059, 62179,   227,
	60517, 52325, 53861, 56933, 62053,   101,
	60650, 52458, 53994, 57066, 62186,   234,
	60521, 52329, 53865, 56937, 62057,   105,
	60527, 52335, 53871, 56943, 62063,   111,
	60660, 52468, 54004, 57076, 62196,   244,
	60661, 52469, 54005, 57077, 62197,   245,
	60533, 52341, 53877, 56949, 62069,   117,
	60669, 52477, 54013, 57085, 62205,   253,
	60537, 52345, 53881, 56953, 62073,   121,
	  100,   240,
};

unsigned short upperCodeMap[] = {
	60481, 52289, 53825, 56897, 62017,    65,
	60610, 52418, 53954, 57026, 62146,   194,
	60611, 52419, 53955, 57027, 62147,   195,
	60485, 52293, 53829, 56901, 62021,    69,
	60618, 52426, 53962, 57034, 62154,   202,
	60489, 52297, 53833, 56905, 62025,    73,
	60495, 52303, 53839, 56911, 62031,    79,
	60628, 52436, 53972, 57044, 62164,   212,
	60629, 52437, 53973, 57045, 62165,   213,
	60501, 52309, 53845, 56917, 62037,    85,
	60637, 52445, 53981, 57053, 62173,   221,
	60505, 52313, 53849, 56921, 62041,    89,
	   68,   208,
};

CVietLang::CVietLang()
{
	memset(m_inputKeyBuf, 0, sizeof(m_inputKeyBuf));
	memset(m_outputKeyBuf, 0, sizeof(m_outputKeyBuf));
	m_curStatus = KEYPROCESS_INPUTSTATUS_WAIT;
	m_keyCount = 0;
	m_pKeyMode = NULL;
}

int CVietLang::InitNativeLanguage(EInputMode mode)
{
	m_inputMode = mode;
	if (m_pKeyMode) {
		delete m_pKeyMode;
	}
	if (m_inputMode == KEYPROCESS_INPUTMODE_VIET_VNI) {
		m_pKeyMode = new VNIInputMode;
	} else if (m_inputMode == KEYPROCESS_INPUTMODE_VIET_TELEX) {
		m_pKeyMode = new TELEXInputMode;
	}
	return m_pKeyMode != NULL;
}

int CVietLang::DestroyForeignLanguage()
{
	if (m_pKeyMode) {
		delete m_pKeyMode;
	}
	return 1;
}

void CVietLang::ResetInput(int outer)
{
	if (outer) {
		if (m_pKeyMode)
			m_pKeyMode->ResetMode();
	}
	memset(m_inputKeyBuf, 0, sizeof(m_inputKeyBuf));
	memset(m_outputKeyBuf, 0, sizeof(m_outputKeyBuf));
	m_curStatus = KEYPROCESS_INPUTSTATUS_WAIT;
	m_keyCount = 0;
	m_oriKey = 0;
}

int CVietLang::ChangeInputMode(EInputMode mode)
{
	if (mode == m_inputMode) {
		return 1;
	}
	if (m_pKeyMode) {
		delete m_pKeyMode;
	}
	m_inputMode = mode;
	if (mode == KEYPROCESS_INPUTMODE_VIET_VNI) {
		m_pKeyMode = new VNIInputMode;
	} else if (mode == KEYPROCESS_INPUTMODE_VIET_TELEX) {
		m_pKeyMode = new TELEXInputMode;
	}
	return m_pKeyMode != NULL;
}

int CVietLang::IsExpend(char inChar)
{
	if (m_curStatus == KEYPROCESS_INPUTSTATUS_CONTINUE) {
		if (m_pKeyMode->IsAppend(inChar, m_keyCount, m_oriKey) != 0) {
			return 1;
		}
		ResetInput(0);
	}
	if (inChar == 'A' || inChar == 'a') {
		if (m_curStatus != KEYPROCESS_INPUTSTATUS_START) {
			m_curStatus = KEYPROCESS_INPUTSTATUS_START;
		}
		return 1;
	}
	if (inChar == 'E' || inChar == 'e') {
		if (m_curStatus != KEYPROCESS_INPUTSTATUS_START) {
			m_curStatus = KEYPROCESS_INPUTSTATUS_START;
		}
		return 1;
	}
	if (inChar == 'I' || inChar == 'i') {
		if (m_curStatus != KEYPROCESS_INPUTSTATUS_START) {
			m_curStatus = KEYPROCESS_INPUTSTATUS_START;
		}
		return 1;
	}
	if (inChar == 'O' || inChar == 'o') {
		if (m_curStatus != KEYPROCESS_INPUTSTATUS_START) {
			m_curStatus = KEYPROCESS_INPUTSTATUS_START;
		}
		return 1;
	}
	if (inChar == 'U' || inChar == 'u') {
		if (m_curStatus != KEYPROCESS_INPUTSTATUS_START) {
			m_curStatus = KEYPROCESS_INPUTSTATUS_START;
		}
		return 1;
	}
	if (inChar == 'Y' || inChar == 'y') {
		if (m_curStatus != KEYPROCESS_INPUTSTATUS_START) {
			m_curStatus = KEYPROCESS_INPUTSTATUS_START;
		}
		return 1;
	}
	if (inChar == 'D' || inChar == 'd') {
		if (m_curStatus != KEYPROCESS_INPUTSTATUS_START) {
			m_curStatus = KEYPROCESS_INPUTSTATUS_START;
		}
		return 1;
	}
	return 0;
}

ECurInputStatus CVietLang::FindOffsetMBCS(char inChar, unsigned short* result, int* start)
{
	int offset;
	int ext;
	unsigned short out;

	switch (m_curStatus) {
	case KEYPROCESS_INPUTSTATUS_CONTINUE:
		ext = m_pKeyMode->ExtendOffset(inChar, m_oriKey, m_offset);
		if (ext == 1000) {
			*result = m_outputKeyBuf[m_keyCount - 2];
			ResetInput(0);
			return m_curStatus;
		}
		if (ext < 0) {
			*result = m_outputKeyBuf[m_keyCount - 1];
			ResetInput(0);
			return m_curStatus;
		}
		*start = 0;
		offset = ext;
		break;

	case KEYPROCESS_INPUTSTATUS_START:
		m_oriKey = inChar;
		m_curStatus = KEYPROCESS_INPUTSTATUS_CONTINUE;

		switch (inChar) {
		case 'A': case 'a':
			offset = 5;
			m_offset = offset;
			break;
		case 'E': case 'e':
			offset = 23;
			m_offset = offset;
			break;
		case 'I': case 'i':
			offset = 35;
			m_offset = offset;
			break;
		case 'O': case 'o':
			offset = 41;
			m_offset = offset;
			break;
		case 'U': case 'u':
			offset = 59;
			m_offset = offset;
			break;
		case 'Y': case 'y':
			offset = 71;
			m_offset = offset;
			break;
		case 'D': case 'd':
			offset = 72;
			m_offset = offset;
			break;
		default:
			m_curStatus = KEYPROCESS_INPUTSTATUS_WAIT;
			offset = -1;
			break;
		}
		break;

	}

	m_offset = offset;
	if (m_oriKey >= 'a') {
		out = lowerCodeMap[offset];
	} else {
		out = upperCodeMap[offset];
	}
	*result = out;
	if (m_keyCount > 9) {
		m_inputKeyBuf[1] = m_inputKeyBuf[8];
		m_inputKeyBuf[2] = m_inputKeyBuf[9];
		m_outputKeyBuf[1] = m_outputKeyBuf[8];
		m_outputKeyBuf[2] = m_outputKeyBuf[9];
		m_keyCount = 3;
	}
	m_inputKeyBuf[m_keyCount] = inChar;
	m_outputKeyBuf[m_keyCount++] = *result;
	return m_curStatus;
}

void CVietLang::ChangeCurStatus(ECurInputStatus Status)
{
	if (m_curStatus != Status) {
		m_curStatus = Status;
	}
}
