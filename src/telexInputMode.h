#ifndef HIGHPRIEST_TELEXINPUTMODE_H
#define HIGHPRIEST_TELEXINPUTMODE_H

#include "InputMode.h"

class TELEXInputMode;

class TELEXInputMode : public CInputMode {
public:
	TELEXInputMode();
	virtual int IsAppend(char inChar, unsigned int inputCount, char oriKey);
	virtual unsigned int ExtendOffset(char inChar, char oriChar, unsigned int oriOffset);
};

#endif
