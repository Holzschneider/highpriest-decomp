#ifndef HIGHPRIEST_INPUTMODE_H
#define HIGHPRIEST_INPUTMODE_H





class CInputMode;

class CInputMode {
public:
	CInputMode() {
		m_lastInput      = 0x7F;
		m_miniLastOffset = -1;
		m_bigLastOffset  = -1;
	}
	virtual int IsAppend(char, unsigned int, char) = 0;
	virtual unsigned int ExtendOffset(char, char, unsigned int) = 0;
	virtual void ResetMode() {
		m_lastInput      = 0x7F;
		m_miniLastOffset = -1;
		m_bigLastOffset  = -1;
	}
protected:
	char m_lastInput;
	int m_miniLastOffset;
	int m_bigLastOffset;
};

#endif
