#ifndef HIGHPRIEST_VNIINPUTMODE_H
#define HIGHPRIEST_VNIINPUTMODE_H

#include "telexInputMode.h"

class VNIInputMode;

class VNIInputMode : public CInputMode {
public:
	VNIInputMode();
	virtual int IsAppend(char inChar, unsigned int inputCount, char oriKey);
	virtual unsigned int ExtendOffset(char inChar, char oriChar, unsigned int oriOffset);
};

#endif
