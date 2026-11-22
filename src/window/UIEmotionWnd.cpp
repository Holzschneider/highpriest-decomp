#include "common.h"
#include "../MsgStrings.h"
#include <windows.h>
#include "UIEmotionWnd.h"
#include "../UIControl.h"
#include "../UIRectInfo.h"
#include "UIFrameWnd.h"
#include "UISys.h"
#include "../Session.h"
#include "../base/Res.h"
#include "../pdb_enums.h"
#include "../resource/Bitmap.h"
#include "../skin.h"

extern UIWindowMgr g_windowMgr;
extern CSession g_session;
extern const char* UIBmp(const char* uiBmpName);
extern const char* MsgStr(MSGSTRINGID id);
extern const char* g_regPath;
extern char* g_emotionPath;

char* g_emotionPath = "\\ShortCutList";

void UIEmotionWnd::OnCreate(int cx, int cy)
{
	char text[16];
	for (int a = 0; a < 10; a++) {
		m_UserDefineText[a] = new UIEditCtrl2;
		m_UserDefineText[a]->Create(302, 16);
		m_UserDefineText[a]->Move(53, a * 21 + 20);
		m_UserDefineText[a]->SetTextLimit(50);
		m_UserDefineText[a]->SetFrameColor(232, 232, 232);
		m_UserDefineText[a]->SetText(g_session.m_shortcutText[a].c_str());
		AddChild(m_UserDefineText[a]);

		sprintf(text, "Alt + %d", a + 1);
		m_staticText[a] = new UIStaticText;
		m_staticText[a]->Create(40, 16);
		m_staticText[a]->Move(5, a * 21 + 23);
		m_staticText[a]->SetText(text, 0);
		AddChild(m_staticText[a]);
	}

	m_staticText[9]->SetText("Alt + 0", 0);

	int posX = m_w - 14;

	char buttonName[] = "close";
	std::string preBmpName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sys_";
	char toolTip[] = "Alt+M";
	UIBitmapButton* closeButton = new UIBitmapButton;
	closeButton->SetBitmapName((preBmpName + buttonName + "_off.bmp").c_str(), BS_NORMAL);
	closeButton->SetBitmapName((preBmpName + buttonName + "_on.bmp").c_str(), BS_MOUSEON);
	closeButton->SetBitmapName((preBmpName + buttonName + "_on.bmp").c_str(), BS_PRESSED);
	closeButton->Create(closeButton->GetBitmapWidth(), closeButton->GetBitmapHeight());
	closeButton->Move(posX, 3);
	closeButton->SetId(0x87);
	closeButton->SetToolTip(toolTip);
	AddChild(closeButton);

	{
		std::string pathName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\";
		m_emotionListButton = new UIBitmapButton;
		m_emotionListButton->SetBitmapName((pathName + "btn_view" + ".bmp").c_str(), BS_NORMAL);
		m_emotionListButton->SetBitmapName((pathName + "btn_view_a" + ".bmp").c_str(), BS_MOUSEON);
		m_emotionListButton->SetBitmapName((pathName + "btn_view_b" + ".bmp").c_str(), BS_PRESSED);
		m_emotionListButton->Create(m_emotionListButton->GetBitmapWidth(), m_emotionListButton->GetBitmapHeight());
		m_emotionListButton->Move(m_w - 50, m_h - 25);
		m_emotionListButton->SetId(0xd9);
		AddChild(m_emotionListButton);
	}

	g_windowMgr.SetFocusEdit(m_UserDefineText[0]);
	m_UserDefineText[0]->DisableSelect(255);
	m_curIndex = 0;
}

void UIEmotionWnd::OnLBtnDown(int x, int y)
{
	if (y < 17)
		UIFrameWnd::OnLBtnDown(x, y);
}

void UIEmotionWnd::StoreInfo()
{
	if (m_info) {
		m_info->x = m_x;
		m_info->y = m_y;
		m_info->w = m_w;
		m_info->h = m_h;
	}
	WriteToReg();
}

void UIEmotionWnd::InitFromReg()
{
	HKEY hResultKey;
	HKEY hKey;
	DWORD DATALENGTH;
	DWORD REG_TYPE;
	char EmotionPath[512];
	char Text[256];
	char KeyNumber[5];

	strcpy(EmotionPath, g_regPath);
	strcat(EmotionPath, g_emotionPath);
	strcat(EmotionPath, "\\");

	LONG openResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, EmotionPath, 0, KEY_QUERY_VALUE, &hResultKey);
	DATALENGTH = 50;
	if (openResult == ERROR_SUCCESS) {
		RegCloseKey(hResultKey);
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, EmotionPath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
			for (int a = 0; a < 10; a++) {
				REG_TYPE = REG_SZ;
				itoa(a, KeyNumber, 10);
				memset(Text, 0, sizeof(Text));
				DATALENGTH = 53;
				RegQueryValueEx(hKey, KeyNumber, 0, &REG_TYPE, (LPBYTE)Text, &DATALENGTH);
				if (strcmp(Text, "") != 0)
					m_UserDefineText[a]->SetText(Text);
			}
			RegCloseKey(hKey);
		}
	}
}

void UIEmotionWnd::WriteToReg()
{
	HKEY hKey;
	DWORD dwDisposition = 0;
	char EmotionPath[512];
	char KeyNumber[5];

	strcpy(EmotionPath, g_regPath);
	strcat(EmotionPath, g_emotionPath);
	strcat(EmotionPath, "\\");

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, EmotionPath, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey,
					   &dwDisposition) == ERROR_SUCCESS) {
		for (int a = 0; a < 10; a++) {
			itoa(a, KeyNumber, 10);
			const char* text = m_UserDefineText[a]->GetText();
			RegSetValueEx(hKey, KeyNumber, 0, REG_SZ, (const BYTE*)text, strlen(text) + 1);
			if (strcmp(text, "") != 0)
				g_session.m_shortcutText[a] = text;
		}
		RegCloseKey(hKey);
	}
}

void UIEmotionWnd::OnDraw()
{
	char windowName[32];
	DWORD rr;
	DWORD gg;
	DWORD bb;

	CBitmapRes* left = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_left.bmp"), false);
	DrawBitmap(0, 0, left, false);

	CBitmapRes* right = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_right.bmp"), false);
	DrawBitmap(m_w - right->m_width, 0, right, false);

	int titleMidCount = (m_w - 24) / 12 + (((m_w - 24) % 12) ? 1 : 0);
	int i;
	for (i = 0; i < titleMidCount; i++) {
		CBitmapRes* mid = (CBitmapRes*)g_resMgr().Get(
			UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_mid.bmp"), false);
		DrawBitmap(12 + i * 12, 0, mid, false);
	}

	sprintf(windowName, MsgStr(MSI_SHORTCUT_LIST));
	TextOutA(5, 2, windowName, 0, 0, 12, 0);

	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(0, 17, m_w, m_h - 17, RGB(rr, gg, bb));
	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	DrawBox(1, 17, m_w - 2, m_h - 17, RGB(rr, gg, bb));

	int btnMidCount = (m_w - 42) / 21 + (((m_w - 42) % 21) ? 1 : 0);
	for (i = 0; i < btnMidCount; i++) {
		CBitmapRes* mid = (CBitmapRes*)g_resMgr().Get(
			UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_mid2.bmp"), false);
		DrawBitmap(21 + i * 21, m_h - mid->m_height, mid, false);
	}

	CBitmapRes* btnLeft = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_left2.bmp"), false);
	DrawBitmap(0, m_h - btnLeft->m_height, btnLeft, false);

	CBitmapRes* btnRight = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_right2.bmp"), false);
	DrawBitmap(m_w - btnRight->m_width, m_h - btnRight->m_height, btnRight, false);
}

void UIEmotionWnd::SetUserDefineText(char* text, int Index)
{
	if (Index == -1) {
		UIWindow* focused = g_windowMgr.GetFocusEdit();
		for (int i = 0; i < 10; i++) {
			if (focused == (UIWindow*)m_UserDefineText[i]) {
				Index = i;
			}
		}
	}
	if (Index == -1) return;
	m_UserDefineText[Index]->SetText(text);
}

int UIEmotionWnd::SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case 0:
		WriteToReg();
		break;
	case UM_COMMAND:
		switch (val1) {
		case ID_CLOSE:
			g_windowMgr.DeleteWindow(WID_EMOTIONWND);
			break;
		case ID_OPEN_EMOTIONLIST:
			if (!g_windowMgr.GetEmotionListWnd())
				g_windowMgr.MakeWindow(WID_EMOTIONLISTWND);
			else
				g_windowMgr.DeleteWindow(WID_EMOTIONLISTWND);
			break;
		}
		break;
	case UM_SETINFO:
		m_info = (EmotionWndInfo*)val1;
		if (!m_info->IsValidInfo())
			m_info->MakeDefault();
		Move(m_info->x, m_info->y);
		break;
	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}
