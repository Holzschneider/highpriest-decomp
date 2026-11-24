// DirectInput 7 for CMouse: one system mouse, immediate data in DIMOUSESTATE.
//
// lX / lY are the difference between the pointer's logical position and the
// position the client's CMouse holds, so that CMouse::ReadState's own
// integration (m_xPos += lX, then clamped to the screen) lands on the
// system pointer (technical plan 3.7). While the right button is held the
// device's own movement is reported instead, so that camera rotation goes on
// at the window's edge and beyond; the client's position then follows that
// movement, and snaps back to the pointer when the button is let go.
// lZ is the wheel in WHEEL_DELTA units (120 per line). The pointer is never
// captured or hidden outside the window: SetCooperativeLevel's exclusive
// mode is accepted and means nothing.
#define DIRECTINPUT_VERSION 0x0700
#include "bf_compat_input_internal.h"

#include <initguid.h>
#include <dinput.h>

#include <math.h>
#include <string.h>

#include "bf_trace.h"

// ------------------------------------------------------------ data formats

extern "C" {

// The objects of c_dfDIMouse as dinput.lib has them.
static DIOBJECTDATAFORMAT g_mouse_objects[] = {
	{&GUID_XAxis, DIMOFS_X, DIDFT_AXIS | DIDFT_ANYINSTANCE, 0},
	{&GUID_YAxis, DIMOFS_Y, DIDFT_AXIS | DIDFT_ANYINSTANCE, 0},
	{&GUID_ZAxis, DIMOFS_Z, DIDFT_AXIS | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0},
	{nullptr, DIMOFS_BUTTON0, DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
	{nullptr, DIMOFS_BUTTON1, DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
	{nullptr, DIMOFS_BUTTON2, DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0},
	{nullptr, DIMOFS_BUTTON3, DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0},
};
static DIOBJECTDATAFORMAT g_mouse2_objects[] = {
	{&GUID_XAxis, 0, DIDFT_AXIS | DIDFT_ANYINSTANCE, 0},
	{&GUID_YAxis, 4, DIDFT_AXIS | DIDFT_ANYINSTANCE, 0},
	{&GUID_ZAxis, 8, DIDFT_AXIS | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0},
	{nullptr, 12, DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
	{nullptr, 13, DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0},
	{nullptr, 14, DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0},
	{nullptr, 15, DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0},
	{nullptr, 16, DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0},
	{nullptr, 17, DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0},
	{nullptr, 18, DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0},
	{nullptr, 19, DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0},
};

const DIDATAFORMAT c_dfDIMouse = {sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT), DIDF_RELAXIS,
                                  sizeof(DIMOUSESTATE), 7, g_mouse_objects};
const DIDATAFORMAT c_dfDIMouse2 = {sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT), DIDF_RELAXIS,
                                   sizeof(DIMOUSESTATE2), 11, g_mouse2_objects};

}  // extern "C"

// ------------------------------------------------------------ the client's position

namespace {

int g_client_x = 320, g_client_y = 240;   // CMouse::Init's m_xPos, m_yPos
double g_rest_x = 0, g_rest_y = 0;        // fractions of a logical pixel not yet reported (right-drag)
double g_rest_wheel = 0;                  // fractions of WHEEL_DELTA not yet reported

int Clamp(int v, int lo, int hi)
{
	return v < lo ? lo : v > hi ? hi : v;
}

// {00000000-0000-0000-C000-000000000046}; compared here so that IID_IUnknown
// (whoever defines it) is not needed.
const GUID kIUnknown = {0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

bool Same(REFGUID a, const GUID &b)
{
	return memcmp(&a, &b, sizeof(GUID)) == 0;
}

// ------------------------------------------------------------ the mouse

class Mouse final : public IDirectInputDevice7A {
public:
	ULONG refs = 1;
	bool acquired = false;
	DWORD format_size = 0;   // sizeof(DIMOUSESTATE) or sizeof(DIMOUSESTATE2) once SetDataFormat ran

	STDMETHOD_(HRESULT, QueryInterface)(REFIID riid, void **out) override
	{
		if (!out)
			return E_POINTER;
		if (Same(riid, kIUnknown) || Same(riid, IID_IDirectInputDeviceA) || Same(riid, IID_IDirectInputDevice2A)
		    || Same(riid, IID_IDirectInputDevice7A)) {
			AddRef();
			*out = this;
			return S_OK;
		}
		*out = nullptr;
		return E_NOINTERFACE;
	}
	STDMETHOD_(ULONG, AddRef)() override { return ++refs; }
	STDMETHOD_(ULONG, Release)() override
	{
		ULONG n = --refs;
		if (n == 0)
			delete this;
		return n;
	}

	STDMETHOD(GetCapabilities)(LPDIDEVCAPS caps) override
	{
		if (!caps || caps->dwSize < sizeof(DIDEVCAPS_DX3))
			return DIERR_INVALIDPARAM;
		DWORD size = caps->dwSize;
		memset(caps, 0, size);
		caps->dwSize = size;
		caps->dwFlags = DIDC_ATTACHED;
		caps->dwDevType = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_UNKNOWN << 8);
		caps->dwAxes = 3;
		caps->dwButtons = 3;
		return DI_OK;
	}
	STDMETHOD(EnumObjects)(LPDIENUMDEVICEOBJECTSCALLBACKA, LPVOID, DWORD) override { BF_STUB(); return DIERR_UNSUPPORTED; }
	STDMETHOD(GetProperty)(REFGUID, LPDIPROPHEADER) override { BF_STUB(); return DIERR_UNSUPPORTED; }
	STDMETHOD(SetProperty)(REFGUID, LPCDIPROPHEADER) override { BF_STUB(); return DIERR_UNSUPPORTED; }

	STDMETHOD(Acquire)() override
	{
		if (!format_size)
			return DIERR_INVALIDPARAM;
		if (acquired)
			return S_FALSE;
		acquired = true;
		bf_app_pointer_take_delta(nullptr, nullptr, nullptr);   // what happened before is not reported
		g_rest_x = g_rest_y = g_rest_wheel = 0;
		return DI_OK;
	}
	STDMETHOD(Unacquire)() override
	{
		if (!acquired)
			return DI_NOEFFECT;
		acquired = false;
		return DI_OK;
	}

	STDMETHOD(GetDeviceState)(DWORD size, LPVOID data) override
	{
		float dx = 0, dy = 0, wheel = 0;
		bf_app_pointer_take_delta(&dx, &dy, &wheel);   // always drained, so nothing piles up
		if (!acquired)
			return DIERR_NOTACQUIRED;
		if (!data || (size != sizeof(DIMOUSESTATE) && size != sizeof(DIMOUSESTATE2)))
			return DIERR_INVALIDPARAM;
		memset(data, 0, size);
		DIMOUSESTATE2 *s = (DIMOUSESTATE2 *)data;

		bf_view_metrics m;
		bfin::Metrics(&m);
		int scale = m.ui_scale > 0 ? m.ui_scale : 1;
		bf_app_pointer p;
		bf_app_pointer_get(&p);
		int nx = g_client_x, ny = g_client_y;
		if (p.buttons & (1u << BF_APP_BUTTON_RIGHT)) {
			// Right-drag: the device's movement, in logical pixels.
			g_rest_x += dx / scale;
			g_rest_y += dy / scale;
			s->lX = (LONG)trunc(g_rest_x);
			s->lY = (LONG)trunc(g_rest_y);
			g_rest_x -= s->lX;
			g_rest_y -= s->lY;
			nx = g_client_x + s->lX;
			ny = g_client_y + s->lY;
		} else {
			int lx, ly;
			bf_view_px_to_logical_pixel(&m, p.x_px, p.y_px, &lx, &ly);
			s->lX = lx - g_client_x;
			s->lY = ly - g_client_y;
			g_rest_x = g_rest_y = 0;
			nx = lx;
			ny = ly;
		}
		// As CMouse::ReadState clamps.
		g_client_x = Clamp(nx, 0, m.logical_w - 1);
		g_client_y = Clamp(ny, 0, m.logical_h - 1);

		g_rest_wheel += (double)wheel * WHEEL_DELTA;
		s->lZ = (LONG)trunc(g_rest_wheel);
		g_rest_wheel -= s->lZ;

		int buttons = size == sizeof(DIMOUSESTATE2) ? 8 : 4;
		static const int order[5] = {BF_APP_BUTTON_LEFT, BF_APP_BUTTON_RIGHT, BF_APP_BUTTON_MIDDLE,
		                             BF_APP_BUTTON_X1, BF_APP_BUTTON_X2};
		for (int i = 0; i < buttons && i < 5; i++)
			s->rgbButtons[i] = ((p.buttons >> order[i]) & 1) ? 0x80 : 0;
		return DI_OK;
	}

	STDMETHOD(GetDeviceData)(DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD) override { BF_STUB(); return DIERR_NOTBUFFERED; }

	STDMETHOD(SetDataFormat)(LPCDIDATAFORMAT format) override
	{
		if (!format)
			return DIERR_INVALIDPARAM;
		if (acquired)
			return DIERR_ACQUIRED;
		if (format->dwDataSize != sizeof(DIMOUSESTATE) && format->dwDataSize != sizeof(DIMOUSESTATE2)) {
			BF_STUB();
			return DIERR_INVALIDPARAM;
		}
		format_size = format->dwDataSize;
		return DI_OK;
	}
	STDMETHOD(SetEventNotification)(HANDLE) override { BF_STUB(); return DIERR_UNSUPPORTED; }
	STDMETHOD(SetCooperativeLevel)(HWND hwnd, DWORD flags) override
	{
		// Exclusive or not, the pointer is never captured (spec section 4).
		return DI_OK;
	}
	STDMETHOD(GetObjectInfo)(LPDIDEVICEOBJECTINSTANCEA, DWORD, DWORD) override { BF_STUB(); return DIERR_UNSUPPORTED; }
	STDMETHOD(GetDeviceInfo)(LPDIDEVICEINSTANCEA) override { BF_STUB(); return DIERR_UNSUPPORTED; }
	STDMETHOD(RunControlPanel)(HWND, DWORD) override { return DIERR_UNSUPPORTED; }
	STDMETHOD(Initialize)(HINSTANCE, DWORD, REFGUID) override { return DI_OK; }
	STDMETHOD(CreateEffect)(REFGUID, LPCDIEFFECT, LPDIRECTINPUTEFFECT *, LPUNKNOWN) override { return DIERR_UNSUPPORTED; }
	STDMETHOD(EnumEffects)(LPDIENUMEFFECTSCALLBACKA, LPVOID, DWORD) override { return DI_OK; }
	STDMETHOD(GetEffectInfo)(LPDIEFFECTINFOA, REFGUID) override { return DIERR_UNSUPPORTED; }
	STDMETHOD(GetForceFeedbackState)(LPDWORD) override { return DIERR_UNSUPPORTED; }
	STDMETHOD(SendForceFeedbackCommand)(DWORD) override { return DIERR_UNSUPPORTED; }
	STDMETHOD(EnumCreatedEffectObjects)(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK, LPVOID, DWORD) override { return DI_OK; }
	STDMETHOD(Escape)(LPDIEFFESCAPE) override { return DIERR_UNSUPPORTED; }
	STDMETHOD(Poll)() override { return acquired ? DI_NOEFFECT : DIERR_NOTACQUIRED; }
	STDMETHOD(SendDeviceData)(DWORD, LPCDIDEVICEOBJECTDATA, LPDWORD, DWORD) override { return DIERR_UNSUPPORTED; }
	STDMETHOD(EnumEffectsInFile)(LPCSTR, LPDIENUMEFFECTSINFILECALLBACK, LPVOID, DWORD) override { return DIERR_UNSUPPORTED; }
	STDMETHOD(WriteEffectToFile)(LPCSTR, DWORD, LPDIFILEEFFECT, DWORD) override { return DIERR_UNSUPPORTED; }
};

// ------------------------------------------------------------ DirectInput

class Input final : public IDirectInput7A {
public:
	ULONG refs = 1;

	STDMETHOD_(HRESULT, QueryInterface)(REFIID riid, void **out) override
	{
		if (!out)
			return E_POINTER;
		if (Same(riid, kIUnknown) || Same(riid, IID_IDirectInputA) || Same(riid, IID_IDirectInput2A)
		    || Same(riid, IID_IDirectInput7A)) {
			AddRef();
			*out = this;
			return S_OK;
		}
		*out = nullptr;
		return E_NOINTERFACE;
	}
	STDMETHOD_(ULONG, AddRef)() override { return ++refs; }
	STDMETHOD_(ULONG, Release)() override
	{
		ULONG n = --refs;
		if (n == 0)
			delete this;
		return n;
	}

	STDMETHOD(CreateDevice)(REFGUID guid, LPDIRECTINPUTDEVICEA *out, LPUNKNOWN outer) override
	{
		if (!out)
			return DIERR_INVALIDPARAM;
		*out = nullptr;
		if (!Same(guid, GUID_SysMouse)) {
			// The keyboard comes through window messages.
			BF_TRACE(BFT_INPUT, "input: DirectInput: only the system mouse is offered\n");
			return DIERR_DEVICENOTREG;
		}
		*out = new Mouse();
		return DI_OK;
	}
	STDMETHOD(EnumDevices)(DWORD, LPDIENUMDEVICESCALLBACKA, LPVOID, DWORD) override { BF_STUB(); return DIERR_UNSUPPORTED; }
	STDMETHOD(GetDeviceStatus)(REFGUID guid) override { return Same(guid, GUID_SysMouse) ? DI_OK : DI_NOTATTACHED; }
	STDMETHOD(RunControlPanel)(HWND, DWORD) override { return DIERR_UNSUPPORTED; }
	STDMETHOD(Initialize)(HINSTANCE, DWORD) override { return DI_OK; }
	STDMETHOD(FindDevice)(REFGUID, LPCSTR, LPGUID) override { return DIERR_DEVICENOTREG; }
	STDMETHOD(CreateDeviceEx)(REFGUID guid, REFIID riid, LPVOID *out, LPUNKNOWN outer) override
	{
		if (!out)
			return DIERR_INVALIDPARAM;
		*out = nullptr;
		LPDIRECTINPUTDEVICEA dev = nullptr;
		HRESULT hr = CreateDevice(guid, &dev, outer);
		if (FAILED(hr))
			return hr;
		hr = dev->QueryInterface(riid, out);
		dev->Release();
		return hr;
	}
};

}  // namespace

extern "C" {

HRESULT WINAPI DirectInputCreateA(HINSTANCE instance, DWORD version, LPDIRECTINPUTA *out, LPUNKNOWN outer)
{
	if (!out)
		return DIERR_INVALIDPARAM;
	*out = new Input();
	return DI_OK;
}

HRESULT WINAPI DirectInputCreateEx(HINSTANCE instance, DWORD version, REFIID riid, LPVOID *out, LPUNKNOWN outer)
{
	if (!out)
		return DIERR_INVALIDPARAM;
	*out = nullptr;
	Input *in = new Input();
	HRESULT hr = in->QueryInterface(riid, out);
	in->Release();
	return hr;
}

void bf_compat_input_mouse_get_client_pos(int *x, int *y)
{
	if (x) *x = g_client_x;
	if (y) *y = g_client_y;
}

void bf_compat_input_mouse_set_client_pos(int x, int y)
{
	g_client_x = x;
	g_client_y = y;
	g_rest_x = g_rest_y = 0;
}

}  // extern "C"
