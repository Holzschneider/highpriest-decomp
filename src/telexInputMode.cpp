








#include <windows.h>
#include "telexInputMode.h"






unsigned char TelexTones[]    = "sfrxj";
unsigned char TelexBreves[]   = "wW";
unsigned char TelexShortcuts[6] = {'[', ']', 'w', 'W', '{', '}'};





TELEXInputMode::TELEXInputMode()
{
}





int TELEXInputMode::IsAppend(char inChar, unsigned int inputCount, char oriKey)
{
	if (inChar == oriKey || inChar == 'w' || inChar == 'W')
		return 1;

	int i = 0;
	while (TelexTones[i] != inChar && i < 5) i++;
	if (i < 5)
		return 1;

	ResetMode();
	return 0;
}







unsigned int TELEXInputMode::ExtendOffset(char inChar, char oriChar, unsigned int oriOffset)
{
	unsigned int offset = oriOffset;

	if (inChar == m_lastInput) {
		ResetMode();
		return 1000;
	}

	int i = 0;
	while (TelexTones[i] != inChar && i < 5) i++;
	if (i < 5) {

		if (oriChar == 'D' || oriChar == 'd') {
			ResetMode();
			return (unsigned int)-1;
		}
		if (m_miniLastOffset > 0) {
			offset = oriOffset - m_miniLastOffset + 6;
		}
		m_lastInput      = inChar;
		m_miniLastOffset = i + 1;
		return offset + i - 5;
	}

	if (inChar == oriChar) {
		if (oriChar == 'U' || oriChar == 'u') {
			ResetMode();
			return (unsigned int)-1;
		}
		if (oriChar == 'Y' || oriChar == 'y') {
			ResetMode();
			return (unsigned int)-1;
		}
		if (inChar == 'd' || inChar == 'D') {
			m_bigLastOffset = inChar;
			m_lastInput     = inChar;
			return 0x49;
		}
		if (m_bigLastOffset > 0 &&
			(m_bigLastOffset == 'w' || m_bigLastOffset == 'W')) {
			offset = oriOffset - 0xc;
		}
		m_bigLastOffset = inChar;
		m_lastInput     = inChar;
		return offset + 6;
	}

	if (inChar == 'w' || inChar == 'W') {
		if (oriChar == 'E' || oriChar == 'e') {
			ResetMode();
			return (unsigned int)-1;
		}
		if (oriChar == 'Y' || oriChar == 'y') {
			ResetMode();
			return (unsigned int)-1;
		}
		if (oriChar == 'D' || oriChar == 'd') {
			ResetMode();
			return (unsigned int)-1;
		}
		if (m_bigLastOffset > 0 && m_bigLastOffset == oriChar) {
			offset = oriOffset - 6;
		}
		m_bigLastOffset = inChar;
		m_lastInput     = inChar;
		return offset + 0xc;
	}

	ResetMode();
	return (unsigned int)-1;
}
