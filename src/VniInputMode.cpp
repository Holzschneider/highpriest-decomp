

#include <windows.h>
#include "VniInputMode.h"

VNIInputMode::VNIInputMode() {
}

int VNIInputMode::IsAppend(char inChar, unsigned int inputCount, char oriKey)
{
	inChar -= '0';

	if (inChar > 0 && inChar < 10)
		return 1;

	ResetMode();
	return 0;
}

unsigned int VNIInputMode::ExtendOffset(char inChar, char oriChar, unsigned int oriOffset)
{
	unsigned int offset = oriOffset;

	if (inChar == m_lastInput) {
		ResetMode();
		return 1000;
	}

	switch (inChar) {
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
		if (oriChar == 'D' || oriChar == 'd') {
			offset = -1;
			ResetMode();
			break;
		}
		if (m_miniLastOffset > 0)
			offset = offset - m_miniLastOffset + 6;
		offset = offset + inChar - '6';
		m_miniLastOffset = inChar - '0';
		m_lastInput = inChar;
		break;
	case '6':
		if (oriChar == 'A' || oriChar == 'a' || oriChar == 'E' || oriChar == 'e' || oriChar == 'O' || oriChar == 'o') {
			if (m_bigLastOffset > 0)
				offset -= m_bigLastOffset;
			offset += 6;
			m_lastInput = inChar;
			m_bigLastOffset = 6;
		} else {
			offset = -1;
			ResetMode();
		}
		break;
	case '7':
		if (oriChar == 'O' || oriChar == 'o') {
			if (m_bigLastOffset > 6)
				offset -= m_bigLastOffset;
			offset += 12;
			m_lastInput = inChar;
			m_bigLastOffset = 12;
		} else if (oriChar == 'U' || oriChar == 'u') {
			offset += 6;
			m_lastInput = inChar;
			m_bigLastOffset = 6;
		} else {
			offset = -1;
			ResetMode();
		}
		break;
	case '8':
		if (oriChar == 'A' || oriChar == 'a') {
			if (m_bigLastOffset > 0)
				offset -= m_bigLastOffset;
			offset += 12;
			m_lastInput = inChar;
			m_bigLastOffset = 12;
		} else {
			offset = -1;
			ResetMode();
		}
		break;
	case '9':
		if (oriChar == 'D' || oriChar == 'd') {
			offset = 0x49;
			m_lastInput = inChar;
		}
		break;
	}
	return offset;
}
