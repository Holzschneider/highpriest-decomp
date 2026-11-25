#ifndef HIGHPRIEST_DEVICE_CONTROL_H
#define HIGHPRIEST_DEVICE_CONTROL_H


enum EBtnState {
	BTN_NONE = 0,
	BTN_DOWN = 1,
	BTN_PRESSED = 2,
	BTN_UP = 3,
	BTN_DBLCLK = 4,
};

enum {
	MAX_BTN = 6,
};

class CMouse;
#ifndef BF_PORT
struct IDirectInputA;
struct IDirectInputDeviceA;
#endif

class CMouse {
	friend int SetAcquire(int acquire);
public:
	CMouse() { m_pMouse = NULL; m_bSwapButton = FALSE; }
	void Init();
	void Exit();
	void ReadState();
	void ResetState();
	void ResetButtonState();
	int GetWheel() { return m_wheel; }
	int GetXPos() { return m_xPos; }
	int GetYPos() { return m_yPos; }
	void SetXPos(int x) { m_xPos = x; }
	void SetYPos(int y) { m_yPos = y; }
	int GetXDelta() { return m_xDelta; }
	int GetYDelta() { return m_yDelta; }
	EBtnState GetLBtn()
	{
		if (m_bSwapButton == FALSE)
			return m_btnState[0];
		else
			return m_btnState[1];
	}
	EBtnState GetRBtn()
	{
		if (m_bSwapButton == FALSE)
			return m_btnState[1];
		else
			return m_btnState[0];
	}
	EBtnState GetWBtn() { return m_btnState[2]; }
	void SetLBtn(EBtnState state) { m_btnState[0] = state; }
	void SetRBtn(EBtnState state) { m_btnState[1] = state; }
	void SetWBtn(EBtnState);
	void SetButtonPressed(int n) { m_oldBtnState[n] = m_btnState[n] = BTN_PRESSED; }
private:
#if defined(BF_PORT)
	int m_pMouse;   // the port's mouse (bf_input.h) is set up
#else
	IDirectInputA* m_lpdi;
	IDirectInputDeviceA* m_pMouse;
#endif
	void* m_hevtMouse;
	int m_xDelta;
	int m_yDelta;
	int m_xPos;
	int m_yPos;
	int m_wheel;
	int m_oldBtnState[3];
	EBtnState m_btnState[3];
	int m_dblclkCnt[3];
	int m_dblclkTime;
	int m_bSwapButton;
};

extern CMouse g_mouse;

#endif
