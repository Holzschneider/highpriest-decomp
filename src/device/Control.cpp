#include "common.h"





#include "bf_types.h"
#ifndef BF_PORT
#endif
#include "bf_input.h"

#include "Control.h"
#include "../3dlib/Device.h"


CMouse g_mouse;

void CMouse::Init()
{
#if defined(BF_PORT)
	// The port's mouse (bf_input.h) in place of DirectInput's system mouse:
	// the same state, in the logical pixels the client draws in.
	m_pMouse = 1;
	bf_input_mouse_acquire(1);
#else
	HRESULT hr;

	hr = DirectInputCreateA(g_hInstance, DIRECTINPUT_VERSION, &m_lpdi, NULL);
	if (FAILED(hr)) {
		ErrorMsg("DirectInput not available");
		return;
	}

	hr = m_lpdi->CreateDevice(GUID_SysMouse, &m_pMouse, NULL);
	if (FAILED(hr)) {
		ErrorMsg("ErrorMsg :: CreateDevice(SysMouse)");
		return;
	}

	m_pMouse->SetDataFormat(&c_dfDIMouse);
	hr = m_pMouse->SetCooperativeLevel(g_hMainWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	if (FAILED(hr)) {
		ErrorMsg("ErrorMsg :: SetCooperativeLevel(SysMouse)");
		return;
	}

	m_pMouse->Acquire();
#endif

	m_xPos = 320;
	m_yPos = 240;
	m_oldBtnState[0] = 0;
	m_oldBtnState[1] = 0;
	m_oldBtnState[2] = 0;
	m_btnState[0] = BTN_NONE;
	m_btnState[1] = BTN_NONE;
	m_btnState[2] = BTN_NONE;
	m_dblclkTime = 500;


#if defined(BF_PORT)
	m_bSwapButton = FALSE;   // macOS swaps the buttons before the app module reports them
#else
	if (GetSystemMetrics(SM_SWAPBUTTON) != 0)
		m_bSwapButton = TRUE;
	else
		m_bSwapButton = FALSE;
#endif
}

void CMouse::Exit()
{
#if defined(BF_PORT)
	if (m_pMouse) {
		bf_input_mouse_acquire(0);
		m_pMouse = 0;
	}
#else
	if (m_lpdi) {
		if (m_pMouse) {
			m_pMouse->Unacquire();
			m_pMouse->Release();
			m_pMouse = NULL;
		}
		m_lpdi->Release();
		m_lpdi = NULL;
	}
#endif
}

void CMouse::ReadState()
{
#if defined(BF_PORT)
	bf_input_mouse_state mouseState;
#else
	DIMOUSESTATE mouseState;
#endif
	int curBtnState[3];
	HRESULT hr;

	curBtnState[0] = m_oldBtnState[0];
	curBtnState[1] = m_oldBtnState[1];
	curBtnState[2] = m_oldBtnState[2];

	m_xDelta = 0;
	m_yDelta = 0;
	m_wheel = 0;

#if defined(BF_PORT)
	hr = bf_input_mouse_read(m_xPos, m_yPos, &mouseState);
	if (FAILED(hr) && g_isAppActive) {
		bf_input_mouse_acquire(1);
		hr = bf_input_mouse_read(m_xPos, m_yPos, &mouseState);
	}
#else
	hr = m_pMouse->GetDeviceState(sizeof(mouseState), &mouseState);
	if (FAILED(hr) && g_isAppActive) {
		m_pMouse->Acquire();
		hr = m_pMouse->GetDeviceState(sizeof(mouseState), &mouseState);
	}
#endif

	if (SUCCEEDED(hr)) {
		m_xPos += mouseState.lX;
		m_xDelta = mouseState.lX;
		m_yPos += mouseState.lY;
		m_yDelta = mouseState.lY;
		m_wheel = mouseState.lZ;
		if (mouseState.rgbButtons[0])
			curBtnState[0] = 1;
		else
			curBtnState[0] = 0;
		if (mouseState.rgbButtons[1])
			curBtnState[1] = 1;
		else
			curBtnState[1] = 0;

		curBtnState[2] = (mouseState.rgbButtons[2] != 0);
	}


	if (m_xPos < 0) m_xPos = 0;
	if (m_yPos < 0) m_yPos = 0;
	if (m_xPos >= g_3dDevice.GetWidth())  m_xPos = g_3dDevice.GetWidth()  - 1;
	if (m_yPos >= g_3dDevice.GetHeight()) m_yPos = g_3dDevice.GetHeight() - 1;

	DWORD curTime = bf_clock_ticks();
	for (int i = 0; i < 3; i++) {
		if (m_xDelta || m_yDelta) {
			m_dblclkCnt[i] = curTime - m_dblclkTime;
		}
		if (curBtnState[i] == 0) {
			if (m_oldBtnState[i] == 0)
				m_btnState[i] = BTN_NONE;
			else
				m_btnState[i] = BTN_UP;
		} else {
			if (m_oldBtnState[i] == 0) {
				if (curTime < m_dblclkCnt[i] + m_dblclkTime) {
					m_btnState[i] = BTN_DBLCLK;
					m_dblclkCnt[i] = curTime - m_dblclkTime;
				} else {
					m_btnState[i] = BTN_DOWN;
					m_dblclkCnt[i] = curTime;
				}
			} else {
				m_btnState[i] = BTN_PRESSED;
			}
		}
	}


	for (int j = 0; j < 3; j++) {
		m_oldBtnState[j] = curBtnState[j];
	}
}

void CMouse::ResetState()
{
	m_xPos = (int)g_3dDevice.GetWidth()  / 2;
	m_yPos = (int)g_3dDevice.GetHeight() / 2;
	m_xDelta = 0;
	m_yDelta = 0;
	for (int i = 0; i < 3; i++) {
		m_oldBtnState[i] = 0;
		m_btnState[i]    = BTN_NONE;
		m_dblclkCnt[i]   = 0;
	}
}

void CMouse::ResetButtonState()
{
	for (int i = 0; i < 3; i++) {
		m_oldBtnState[i] = 0;
		m_btnState[i]    = BTN_NONE;
		m_dblclkCnt[i]   = 0;
	}
}

HRESULT SetAcquire(BOOL acquire)
{
	if (!g_mouse.m_pMouse)
		return S_FALSE;
#if defined(BF_PORT)
	bf_input_mouse_acquire(acquire);
#else
	if (acquire)
		g_mouse.m_pMouse->Acquire();
	else
		g_mouse.m_pMouse->Unacquire();
#endif
	return S_OK;
}
