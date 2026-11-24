#include "common.h"
#include "window/UIFrameWnd.h"
#include "UIMessengerWnd.h"
#include "MsgStrings.h"
#include "skin.h"
#include "device/Control.h"
#include "framework/Locale.h"
#include "GameMode.h"
#include "GameWorld.h"
#include "GameActor.h"
#include "Mode.h"
#include "Session.h"
#include "UIRectInfo.h"
#include "window/UISys.h"
#include "base/Res.h"
#include "pdb_enums.h"
#include "3dlib/Renderer.h"
#include "resource/Bitmap.h"
#include "resource/Ground.h"





inline int CPc::GetHp() { return m_gage ? m_gage->m_hpAmount : 0; }
inline int CPc::GetMaxHp() { return m_gage ? m_gage->m_hpTotalAmount : 0; }

extern CModeMgr g_modeMgr;
extern CSession g_session;
extern UIWindowMgr g_windowMgr;
extern ServiceType g_serviceType;
extern CRenderer* g_renderer;
extern const char* UIBmp(const char* uiBmpName);
extern const char* MsgStr(MSGSTRINGID id);

UIMessengerGroupWnd::UIMessengerGroupWnd()
{
	m_info = NULL;
	m_scrollBar = NULL;
	m_viewOffset = 0;
	m_vertScrEnabled = false;
	m_itemHorzNum = 1;
	m_itemVertNum = 5;
	m_itemHeight = 23;
}

UIMessengerGroupWnd::~UIMessengerGroupWnd()
{
}

void UIMessengerGroupWnd::OnCreate(int cx, int cy)
{
	g_modeMgr.GetGameMode();

	int i;
	for (i = 0; i < 40; i++) {
		m_gageList[i] = new UIPcGage;
		m_gageList[i]->Create(60, 5);
		m_gageList[i]->Move(-100, -100);
		AddChild(m_gageList[i]);
	}

	m_scrollBar = new UIScrollBar;
	m_scrollBar->Create(m_w - 15, 17, 1, m_h - 58);
	AddChild(m_scrollBar);

	m_curRadioBtn = 1;

	{
		const char* texts[2] = { MsgStr(MSI_FRIEND), MsgStr(MSI_PARTY) };
		int createWidth[2] = { 0x30, GetTextWidth(MsgStr(MSI_PARTY), 0, 0, 12, 0) + 13 };
		int states[2] = { 0, 1 };

		for (i = 0; i < 2; ++i) {
			m_radioBtns[i] = new UIRadioBtn;
			m_radioBtns[i]->Create(createWidth[i], 12);
			m_radioBtns[i]->Move(i * 60 + 4, m_h - 16);
			m_radioBtns[i]->SetText(texts[i]);
			m_radioBtns[i]->SendMsg(this, UM_SETSTATE, states[i], 0, 0, 0);
			AddChild(m_radioBtns[i]);
		}
	}

	{
		const char* titleText[2] = { MsgStr(MSI_FRIEND), MsgStr(MSI_PARTY) };

		char buf[64];
		if (m_curRadioBtn == 1) {
			if (g_session.m_partyName.size())
				sprintf(buf, "%s(%s)", titleText[1], g_session.m_partyName.c_str());
			else
				sprintf(buf, "%s", titleText[1]);
		} else {
			sprintf(buf, "%s(%d/%d)", titleText[m_curRadioBtn], (int)m_itemList.size(), 40);
		}

		m_titleText = new UIStaticText;
		m_titleText->Create(m_w - 32, 15);
		m_titleText->SetTextWithShorten(buf);
		m_titleText->Move(16, 2);
		m_titleText->m_drawTwice = 1;
		AddChild(m_titleText);
	}

	{
		int posX[3] = { m_w - 14 };
		int ids2[3] = { ID_CLOSE };
		char* buttonName2[8] = { "close" };
		std::string preBmpName2 = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sys_";

		for (i = 0; i < 1; ++i) {
			m_sysButton = new UIBitmapButton;
			m_sysButton->SetBitmapName((preBmpName2 + buttonName2[i] + "_off.bmp").c_str(), BS_NORMAL);
			m_sysButton->SetBitmapName((preBmpName2 + buttonName2[i] + "_on.bmp").c_str(), BS_MOUSEON);
			m_sysButton->SetBitmapName((preBmpName2 + buttonName2[i] + "_on.bmp").c_str(), BS_PRESSED);
			m_sysButton->Create(m_sysButton->GetBitmapWidth(), m_sysButton->GetBitmapHeight());
			m_sysButton->Move(posX[i], 3);
			m_sysButton->SetId(ids2[i]);
			m_sysButton->SetToolTip("Alt+Z");
			AddChild(m_sysButton);
		}
	}

	{
		std::string pathName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\";

		char* buttonName[5][3] = {
			{ "mesbtn_01", "mesbtn_01_a", "mesbtn_01_b" },
			{ "mesbtn_02", "mesbtn_02_a", "mesbtn_02_b" },
			{ "mesbtn_03", "mesbtn_03_a", "mesbtn_03_b" },
			{ "mesbtn_04", "mesbtn_04_a", "mesbtn_04_b" },
			{ "mesbtn_05", "mesbtn_05_a", "mesbtn_05_b" }
		};
		int ids[5] = { ID_SENDMSG, ID_TALK, ID_FRIEND_INFO, ID_PARTY_SETTING, ID_DELETE_FRIEND };
		const char* toolTip0[5] = { MsgStr(MSI_MSG), MsgStr(MSI_1ON1), MsgStr(MSI_CHARACTER_INFO), MsgStr(MSI_FRIEND_SETTING), MsgStr(MSI_DELETE) };
		const char* toolTip[5] = { MsgStr(MSI_MSG), MsgStr(MSI_1ON1), MsgStr(MSI_CHARACTER_INFO), MsgStr(MSI_PARTY_SETTING), MsgStr(MSI_EXPEL) };

		int disabled[5] = { 1, 0, 1, 0, 0 };

		for (i = 0; i < 5; ++i) {
			m_button[i] = new UIBitmapButton;
			m_button[i]->SetBitmapName((pathName + buttonName[i][0] + ".bmp").c_str(), BS_NORMAL);
			m_button[i]->SetBitmapName((pathName + buttonName[i][1] + ".bmp").c_str(), BS_MOUSEON);
			m_button[i]->SetBitmapName((pathName + buttonName[i][2] + ".bmp").c_str(), BS_PRESSED);
			m_button[i]->Create(m_button[i]->GetBitmapWidth(), m_button[i]->GetBitmapHeight());
			m_button[i]->Move(i * 26 + 5, m_h - 40);
			m_button[i]->SetId(ids[i]);
			if (m_curRadioBtn == 0)
				m_button[i]->SetToolTip(toolTip0[i]);
			else
				m_button[i]->SetToolTip(toolTip[i]);
			m_button[i]->SetDiabled(disabled[i]);
			AddChild(m_button[i]);
		}
		m_button[0]->SetDiabled(0);
	}

	m_resizer = new UIResizer;
	m_resizer->Create(16, 16);
	m_resizer->SetType((RESIZERTYPE)7);
	m_resizer->SetBitmap("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\btn_resize.bmp");
	m_resizer->Move(m_w - 15, m_h - 15);
	m_resizer->SetColor(0xffff00ff, 0xffff00ff);
	AddChild(m_resizer);

	SendMsg(NULL, UM_REFRESHITEM, 0, 0, 0, 0);
}

FRIEND_INFO UIMessengerGroupWnd::GetItemInfoBySequence(int sequence)
{
	int count = 0;
	std::list<FRIEND_INFO>::iterator it = m_itemList.begin();
	while (it != m_itemList.end()) {
		if (sequence == count)
			return *it;
		++it;
		++count;
	}
	return FRIEND_INFO(0);
}

FRIEND_INFO UIMessengerGroupWnd::GetItemInfo(int x, int y)
{
	char text[80];

	if (x < 1 || x >= m_w - 20 || y < 17 || y >= m_itemVertNum * m_itemHeight + 17)
		return FRIEND_INFO(0);

	int yCell = (y - 17) / m_itemHeight;
	int drawCount = 0;
	int count = 0;
	std::list<FRIEND_INFO>::iterator it = m_itemList.begin();
	while (it != m_itemList.end()) {
		if (drawCount < m_itemHorzNum * m_viewOffset) {
			it++;
			++drawCount;
			continue;
		}
		if (count == m_itemHorzNum * yCell) {
			sprintf(text, "%s", (*it).characterName.c_str());
			if (x > 20 && x < GetTextWidth(text, 0, 0, 12, 0) + 26) {
				m_curItem = drawCount;
				return *it;
			}
			m_curItem = -1;
			return FRIEND_INFO(0);
		}
		it++;
		++count;
		++drawCount;
	}

	m_curItem = -1;
	return FRIEND_INFO(0);
}

void UIMessengerGroupWnd::OnRBtnUp(int x, int y)
{
	int oldCurItem = m_curItem;
	FRIEND_INFO info = GetItemInfo(x, y);
	if (oldCurItem != m_curItem)
		Invalidate();
	Trace("m_curItem == %d", m_curItem);
	if (m_curItem != -1)
		SendMsg(NULL, UM_MAKEMENU, 0, 0, 0, 0);
}

void UIMessengerGroupWnd::OnLBtnDown(int x, int y)
{
	if (y < 17) {
		UIFrameWnd::OnLBtnDown(x, y);
		return;
	}

	int oldCurItem = m_curItem;
	FRIEND_INFO info = GetItemInfo(x, y);
	if (oldCurItem != m_curItem)
		Invalidate();

	if (info.isValid && !info.state) {
		CGameMode* gameMode = g_modeMgr.GetGameMode();
		if (gameMode)
			gameMode->ProceedSkillToFriendList(info.AID);
	}
	Trace("m_curItem == %d", m_curItem);
}

void UIMessengerGroupWnd::OnWheel(int wheel)
{
	if (m_vertScrEnabled) {
		int delta = m_itemVertNum - 1;
		if (delta >= 3)
			delta = 3;
		if (wheel < 0)
			SendMsg(m_scrollBar, UM_SCROLLVERT, delta, 0, 0, 0);
		else
			SendMsg(m_scrollBar, UM_SCROLLVERT, -delta, 0, 0, 0);
	}
}

void UIMessengerGroupWnd::OnLBtnDblClk(int x, int y)
{
	SendMsg(0, UM_COMMAND, 0xa6, 0, 0, 0);
}

void UIMessengerGroupWnd::RecalcScrbarPos()
{
	if (!m_scrollBar)
		return;

	int numLine = (m_itemList.size() % m_itemHorzNum) != 0;
	int numCanDisplayItem = m_itemVertNum;
	numLine += m_itemList.size() / m_itemHorzNum;
	int range = numLine - numCanDisplayItem;
	if (range < 0)
		range = 0;
	if (range != 0)
		m_vertScrEnabled = 1;
	else
		m_vertScrEnabled = 0;

	if (m_vertScrEnabled) {
		m_scrollBar->SetRange(range, numCanDisplayItem, numLine);
		if (m_viewOffset < 0)
			m_viewOffset = 0;
		if (m_viewOffset > numLine - m_itemVertNum)
			m_viewOffset = numLine - m_itemVertNum;
		m_scrollBar->SetPos(m_viewOffset);
	} else {
		m_viewOffset = 0;
		m_scrollBar->SetRange(0, numCanDisplayItem, numLine);
	}
	m_scrollBar->Invalidate();
	ResetGageListPos();
}

void UIMessengerGroupWnd::OnDraw()
{
	char text[80];

	CBitmapRes* left = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_left.bmp"), false);
	DrawBitmap(0, 0, left, false);
	CBitmapRes* right = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_right.bmp"), false);
	DrawBitmap(GetWidth() - right->m_width, 0, right, false);

	int i;
	int titleMidCount = (GetWidth() - 24) / 12 + (((GetWidth() - 24) % 12) ? 1 : 0);
	for (i = 0; i < titleMidCount; i++) {
		CBitmapRes* mid = (CBitmapRes*)g_resMgr().Get(
			UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_mid.bmp"), false);
		DrawBitmap(12 + i * 12, 0, mid, false);
	}

	CBitmapRes* sysBase = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sys_base_off.bmp"), false);
	DrawBitmap(3, 3, sysBase, true);

	{
		DWORD rr;
		DWORD gg;
		DWORD bb;
		g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
		DrawBox(0, 17, GetWidth(), GetHeight() - 38, RGB(rr, gg, bb));
	}

	CBitmapRes* btnLeft = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_left.bmp"), false);
	DrawBitmap(0, GetHeight() - btnLeft->m_height, btnLeft, false);
	CBitmapRes* btnRight = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_right.bmp"), false);
	DrawBitmap(GetWidth() - btnRight->m_width, GetHeight() - btnRight->m_height, btnRight, false);
	int btnMidCount = (GetWidth() - 42) / 21 + (((GetWidth() - 42) % 21) ? 1 : 0);
	for (i = 0; i < btnMidCount; i++) {
		CBitmapRes* mid = (CBitmapRes*)g_resMgr().Get(
			UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_mid.bmp"), false);
		DrawBitmap(21 + i * 21, GetHeight() - mid->m_height, mid, false);
	}

	CBitmapRes* mesLeft = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\mesbtn_left.bmp"), false);
	DrawBitmap(0, GetHeight() - 40, mesLeft, false);
	CBitmapRes* mesRight = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\mesbtn_right.bmp"), false);
	DrawBitmap(GetWidth() - mesRight->m_width, GetHeight() - 40, mesRight, false);
	int mesMidCount = (GetWidth() - 14) / 7 + (((GetWidth() - 14) % 7) ? 1 : 0);
	for (i = 0; i < mesMidCount; i++) {
		CBitmapRes* mid = (CBitmapRes*)g_resMgr().Get(
			UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\mesbtn_mid.bmp"), false);
		DrawBitmap(7 + i * 7, GetHeight() - 40, mid, false);
	}

	int drawCount = 0;
	int itemCount = 0;
	std::list<FRIEND_INFO>::iterator it = m_itemList.begin();
	while (it != m_itemList.end()) {
		if (itemCount < m_itemHorzNum * m_viewOffset) {
			it++;
			itemCount++;
			continue;
		}

		int itemX = (drawCount % m_itemHorzNum) * 32 + 3;
		int itemY = (drawCount / m_itemHorzNum) * m_itemHeight + 17;
		sprintf(text, "%s", (*it).characterName.c_str());

		if (m_curItem == itemCount) {
			DWORD rr;
			DWORD gg;
			DWORD bb;
			g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
			DrawBox(itemX + 17, itemY + 3, GetTextWidth(text, 0, 0, 12, 0) + 3, 16, RGB(rr, gg, bb));
		}

		if (!(*it).state) {
			if (!(*it).role && m_curRadioBtn)
				sprintf(text, "%s", "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\grp_leader.bmp");
			else
				sprintf(text, "%s", "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\grp_online.bmp");
			CBitmapRes* icon = (CBitmapRes*)g_resMgr().Get(UIBmp(text), false);
			DrawBitmap(itemX + 1, itemY + 4, icon, true);
		}

		if ((*it).mapName.length() && !(*it).state) {
			std::string mapName = g_session.GetMapName((*it).mapName.c_str());
			sprintf(text, "%s(%s)", (*it).characterName.c_str(), mapName.c_str());
		} else {
			sprintf(text, "%s", (*it).characterName.c_str());
		}
		DWORD color = (*it).color;
		TextOutA(itemX + 19, itemY + 6, text, strlen(text), 0, 12, RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff));

		if (!(*it).state && m_curRadioBtn && m_gageList[itemCount]->GetX() != -100) {
			char text[100];
			sprintf(text, "%d/%d", m_gageList[itemCount]->m_hpAmount, m_gageList[itemCount]->m_hpTotalAmount);
			TextOutA(90, itemY + 18, text, strlen(text), 0, 10, 0);
		}

		if (drawCount + 1 == m_itemVertNum * m_itemHorzNum)
			return;

		it++;
		drawCount++;
		itemCount++;
	}
}

void UIMessengerGroupWnd::StoreInfo()
{
	if (m_info) {
		m_info->w = m_w;
		m_info->h = m_h;
		m_info->x = m_x;
		m_info->y = m_y;
	}
}

#include "rag_budget_fill.h"

INT_PTR UIMessengerGroupWnd::SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch (message) {
	case UM_SETINFO:
		m_info = (MessengerGroupWndInfo*)val1;
		if (!m_info->IsValidInfo())
			m_info->MakeDefault();
		SendMsg(NULL, UM_RESIZE, 7, m_info->w, m_info->h, 0);
		Move(m_info->x, m_info->y);
		break;

	case UM_REFRESHITEM: {
		m_itemList.erase(m_itemList.begin(), m_itemList.end());

		for (int gageIndex = 0; gageIndex < 40; ++gageIndex)
			m_gageList[gageIndex]->Move(-100, -100);

		switch (m_curRadioBtn) {
		case 0: {
			int friendNum = g_session.GetNumFriend();
			for (int friendIndex = 0; friendIndex < friendNum; ++friendIndex) {
				FRIEND_INFO itemInfo = g_session.GetFriendInfoBySequence(friendIndex);
				if (itemInfo.isValid)
					m_itemList.push_back(itemInfo);
			}
			break;
		}
		case 1: {
			int partyNum = g_session.GetNumParty();
			for (int partyIndex = 0; partyIndex < partyNum; ++partyIndex) {
				FRIEND_INFO itemInfo = g_session.GetPartyInfoBySequence(partyIndex);
				if (itemInfo.isValid)
					m_itemList.push_back(itemInfo);
			}
			break;
		}
		}

		CGameMode* gameMode = g_modeMgr.GetGameMode();
		if (gameMode && m_curRadioBtn) {
			int itemCount = 0;
			std::list<FRIEND_INFO>::iterator it = m_itemList.begin();
			while (it != m_itemList.end()) {
				if ((*it).isValid && !(*it).state) {
					CGameActor* actor;
					int hp;
					int maxHp;
					if ((*it).AID == g_session.GetAid()) {
						actor = (CGameActor*)gameMode->GetWorld()->m_player;
						hp = g_session.GetHp();
						maxHp = g_session.GetMaxhp();
					} else {
						actor = gameMode->GetWorld()->GetGameActorByAID((*it).AID);
						if (actor) {
							hp = ((CPc*)actor)->GetHp();
							maxHp = ((CPc*)actor)->GetMaxHp();
						}
					}

					if (actor)
						m_gageList[itemCount]->SetHp(hp, maxHp);
				}
				it++;
				itemCount++;
			}
		}

		m_curItem = -1;
		RecalcScrbarPos();
		Invalidate();

		const char* titleText[2] = { MsgStr(MSI_FRIEND), MsgStr(MSI_PARTY) };
		char buf[64];
		if (m_curRadioBtn == 1) {
			if (g_session.m_partyName.size())
				sprintf(buf, "%s(%s)", titleText[1], g_session.m_partyName.c_str());
			else
				sprintf(buf, "%s", titleText[1]);
		} else {
			sprintf(buf, "%s(%d/%d)", titleText[m_curRadioBtn], (int)m_itemList.size(), 40);
		}
		m_titleText->SetTextWithShorten(buf);

		UIWindow* roMapWnd = (UIWindow*)g_windowMgr.GetRoMapWnd();
		if (roMapWnd)
			roMapWnd->SendMsg(NULL, UM_REFRESH, 0, 0, 0, 0);
		break;
	}

	case UM_MAKEMENU: {
		m_menuItems.erase(m_menuItems.begin(), m_menuItems.end());
		if (g_windowMgr.GetMenu())
			g_windowMgr.DeleteWindow(WID_MENUWND);

		if (m_curItem != -1) {
			if (m_curRadioBtn == 0) {
				m_menuItems.push_back(StringId(((std::string)MsgStr(MSI_OPEN_1ON1_WINDOW)).c_str(), MENUID_TALKONLYTWO));
				m_menuItems.push_back(StringId(((std::string)MsgStr(MSI_DELETE)).c_str(), MENUID_DELETE_WHISPER_NAME));

				FRIEND_INFO info = GetItemInfoBySequence(m_curItem);
				if (info.isValid && !info.state) {
					if (g_session.m_amIPartyMaster && !g_session.GetPartyInfoByName(info.characterName.c_str()).isValid) {
						char msg[200];
						sprintf(msg, MsgStr(MSI_REQ_JOIN_PARTY3), info.characterName.c_str());
						m_menuItems.push_back(StringId(msg, MENUID_INVITE_IN_PARTY));
					}
				}
			}
			else {
				FRIEND_INFO info = GetItemInfoBySequence(m_curItem);
				if (info.isValid) {
					m_menuItems.push_back(StringId(((std::string)MsgStr(MSI_SEE_INFORMATION_OF_CHARACTER)).c_str(), MENUID_QUERYINFO));
					if (info.characterName != g_session.GetCharName())
						m_menuItems.push_back(StringId(((std::string)MsgStr(MSI_OPEN_1ON1_WINDOW)).c_str(), MENUID_TALKONLYTWO));

					if (info.characterName == g_session.GetCharName())
						m_menuItems.push_back(StringId(((std::string)MsgStr(MSI_LEAVE_PARTY)).c_str(), MENUID_LEAVE));
					else if (g_session.m_amIPartyMaster) {
						if (info.characterName != g_session.GetCharName())
							m_menuItems.push_back(StringId(((std::string)MsgStr(MSI_EXPEL)).c_str(), MENUID_EXPEL));
					}
				}
			}
		}

		if (m_menuItems.size() == 0)
			break;

		UIFrameWnd* menu = g_windowMgr.MakeWindow(WID_MENUWND);
		for (int i = 0; i < m_menuItems.size(); ++i)
			menu->SendMsg(NULL, UM_ADDITEM, (INT_PTR)m_menuItems[i].itemName.c_str(), 0, 0, 0);

		menu->SendMsg(NULL, UM_SET_CALLER_WID, WID_MESSENGERGROUPWND, 0, 0, 0);
		menu->SendMsg(NULL, UM_ADJUSTSIZE, 0, 0, 0, 0);

		int x = g_mouse.GetXPos() + 1;
		int y = g_mouse.GetYPos() + 1;
		if (x < 1)
			x = 1;
		if (x > g_renderer->m_width - menu->GetWidth())
			x = g_renderer->m_width - menu->GetWidth();
		if (y < 1)
			y = 1;
		if (y > g_renderer->m_height - menu->GetHeight())
			y = g_renderer->m_height - menu->GetHeight();
		menu->Move(x, y);
		break;
	}

	case UM_MENU_SELECTED: {
		FRIEND_INFO info = GetItemInfoBySequence(m_curItem);
		if (info.isValid) {
			switch (m_menuItems[val1].id) {
			case MENUID_SEND_MSG:
				SendMsg(NULL, UM_COMMAND, 0xa5, 0, 0, 0);
				break;
			case MENUID_TALKONLYTWO:
				SendMsg(NULL, UM_COMMAND, 0xa6, 0, 0, 0);
				break;
			case MENUID_QUERYINFO:
				SendMsg(NULL, UM_COMMAND, 0xa7, 0, 0, 0);
				break;
			case MENUID_EXPEL:
				SendMsg(NULL, UM_COMMAND, 0xa9, 0, 0, 0);
				break;
			case MENUID_LEAVE:
				SendMsg(NULL, UM_COMMAND, 0xaa, 0, 0, 0);
				break;
			case MENUID_MAKE_WHISPER_WINDOW:
				SendMsg(NULL, UM_MAKE_WHISPER_WINDOW, 0, 0, 0, 0);
				break;
			case MENUID_DELETE_WHISPER_NAME:
				SendMsg(NULL, UM_COMMAND, 0xa9, 0, 0, 0);
				break;
			case MENUID_INVITE_IN_PARTY:
				if (m_curItem != -1) {
					FRIEND_INFO info = GetItemInfoBySequence(m_curItem);
					if (info.isValid)
						g_modeMgr.GetCurMode()->SendMsg(MM_REQ_JOIN_GROUP, (INT_PTR)info.characterName.c_str(), 0, 0);
				}
				break;
			}
		}
		break;
	}

	case UM_COMMAND:
		switch (val1) {
		case ID_SENDMSG:
			if (m_curItem != -1) {
				FRIEND_INFO info = GetItemInfoBySequence(m_curItem);
				if (info.isValid) {
					RAG_BUDGET_FILL(28)
					if (!g_windowMgr.GetMailListWnd())
						g_windowMgr.MakeWindow(WID_MAILLISTWND);
					if (g_windowMgr.GetMailListWnd()) {
						g_windowMgr.GetMailListWnd()->SetType2(1);
						g_windowMgr.GetMailListWnd()->SendMsg(NULL, UM_NEWMAIL, (INT_PTR)info.characterName.c_str(), 0, 0, 0);
					}
				}
			}
			break;

		case ID_TALK:
			if (m_curItem != -1) {
				FRIEND_INFO info = GetItemInfoBySequence(m_curItem);
				if (info.isValid && info.characterName != g_session.GetCharName()) {
					if (g_windowMgr.IsFriendName(info.characterName.c_str()))
						g_windowMgr.SendMsg(UM_RESIZE, (INT_PTR)info.characterName.c_str(), (INT_PTR)MsgStr(MSI_FRIEND), 0, 0);
					else if (g_session.IsGuildMember(info.characterName.c_str()))
						g_windowMgr.SendMsg(UM_RESIZE, (INT_PTR)info.characterName.c_str(), (INT_PTR)"GuildMember", 0, 0);
					else
						g_windowMgr.SendMsg(UM_RESIZE, (INT_PTR)info.characterName.c_str(), 0, 0, 0);
				}
			}
			break;

		case ID_FRIEND_INFO:
			if (m_curItem != -1) {
				FRIEND_INFO info = GetItemInfoBySequence(m_curItem);
				if (info.isValid)
					g_windowMgr.ErrorMsg(MsgStr(MSI_NOTSUPPORTEDYET), ME_OK, 1, 0, 0);
			}
			break;

		case ID_PARTY_SETTING:
			if (m_curRadioBtn == 0) {
				if (!g_windowMgr.DeleteWindow(WID_FRIENDOPTIONWND))
					g_windowMgr.MakeWindow(WID_FRIENDOPTIONWND);
			} else {
				if (!g_windowMgr.DeleteWindow(WID_PARTYSETTINGWND)) {
					if (g_session.GetNumParty() && m_curRadioBtn == 1)
						g_windowMgr.MakeWindow(WID_PARTYSETTINGWND);
				}
			}
			break;

		case ID_DELETE_FRIEND: {
			if (m_curItem == -1)
				break;
			FRIEND_INFO info = GetItemInfoBySequence(m_curItem);
			if (info.isValid) {
				if (m_curRadioBtn == 0) {
					if (g_windowMgr.ErrorMsg(MsgStr(MSI_DO_YOU_REALLY_WANT_DELETE), ME_YESNO, 0, 0, 0) != ID_YES)
						break;
					g_modeMgr.GetCurMode()->SendMsg(MM_DELETE_FRIEND, info.AID, info.GID, 0);
				} else {
					if (info.characterName != g_session.GetCharName()) {
						if (g_windowMgr.ErrorMsg(MsgStr(MSI_DO_YOU_REALLY_WANT_EXPEL), ME_YESNO, 0, 0, 0) != ID_YES)
							break;
						g_modeMgr.GetCurMode()->SendMsg(MM_REQ_EXPEL_GROUP_MEMBER, (INT_PTR)info.characterName.c_str(), 0, 0);
					}
				}
			}
			break;
		}

		case ID_CLOSE:
			g_windowMgr.DeleteWindow(WID_MESSENGERWND);
			break;

		case ID_LEAVE:
			if (m_curItem != -1) {
				FRIEND_INFO info = GetItemInfoBySequence(m_curItem);
				if (info.isValid && info.characterName == g_session.GetCharName()) {
					if (g_windowMgr.ErrorMsg(MsgStr(MSI_DO_YOU_REALLY_WANT_LEAVE), ME_YESNO, 0, 0, 0) != ID_YES)
						break;
					g_modeMgr.GetCurMode()->SendMsg(MM_REQ_LEAVE_GROUP, 0, 0, 0);
				}
			}
			break;

		case ID_RADIOBTN: {
			if (val2)
				sender = m_radioBtns[val2 - 1];
			int oldRadioBtn = m_curRadioBtn;
			for (int i = 0; i < 2; ++i) {
				m_radioBtns[i]->Invalidate();
				if (sender == m_radioBtns[i]) {
					m_curRadioBtn = i;
					m_radioBtns[i]->SendMsg(this, UM_SETSTATE, 1, 0, 0, 0);
				} else {
					m_radioBtns[i]->SendMsg(this, UM_SETSTATE, 0, 0, 0, 0);
				}
			}

			if (m_curRadioBtn == 0) {
				m_button[3]->SetToolTip(MsgStr(MSI_FRIEND_SETTING));
				m_button[4]->SetToolTip(MsgStr(MSI_DELETE));
			} else {
				m_button[3]->SetToolTip(MsgStr(MSI_PARTY_SETTING));
				m_button[4]->SetToolTip(MsgStr(MSI_EXPEL));
			}

			if (m_curRadioBtn != oldRadioBtn)
				SendMsg(NULL, UM_REFRESHITEM, 0, 0, 0, 0);
			break;
		}

		case 0x22: {
			CGameMode* gameMode = g_modeMgr.GetGameMode();
			if (!gameMode || !m_curRadioBtn)
				break;

			int itemCount = 0;
			std::list<FRIEND_INFO>::iterator it = m_itemList.begin();
			while (it != m_itemList.end()) {
				if ((*it).isValid && !(*it).state) {
					if ((*it).AID == (unsigned int)val2) {
						CGameActor* actor;
						int hp;
						int maxHp;
						if ((*it).AID == g_session.GetAid()) {
							actor = (CGameActor*)gameMode->GetWorld()->m_player;
							hp = g_session.GetHp();
							maxHp = g_session.GetMaxhp();
						} else {
							actor = gameMode->GetWorld()->GetGameActorByAID((*it).AID);
							hp = val3;
							maxHp = val4;
						}

						if (actor)
							m_gageList[itemCount]->SetHp(hp, maxHp);

						ResetGageListPos();
						Invalidate();
						return 1;
					}
				}
				it++;
				itemCount++;
			}
			break;
		}

		case 0x7d: {
			CGameMode* gameMode = g_modeMgr.GetGameMode();
			if (!gameMode || !m_curRadioBtn)
				break;

			int itemCount = 0;
			std::list<FRIEND_INFO>::iterator it = m_itemList.begin();
			while (it != m_itemList.end()) {
				if ((*it).isValid && !(*it).state) {
					if ((*it).AID == (unsigned int)val2) {
						m_gageList[itemCount]->Move(-100, -100);
						Invalidate();
						return 1;
					}
				}
				it++;
				itemCount++;
			}
			break;
		}

		default:
			break;
		}
		break;

	case UM_SCROLLVERT:
	case UM_SCROLLVERTPAGEUP:
	case UM_SCROLLVERTPAGEDOWN: {
		int oldViewOffset = m_viewOffset;
		if (message == UM_SCROLLVERTPAGEUP)
			m_viewOffset = oldViewOffset - m_itemVertNum + 1;
		else if (message == UM_SCROLLVERTPAGEDOWN)
			m_viewOffset = oldViewOffset + m_itemVertNum - 1;
		else
			m_viewOffset = oldViewOffset + val1;

		if (oldViewOffset != m_viewOffset) {
			RecalcScrbarPos();
			Invalidate();
		}
		return m_viewOffset - oldViewOffset;
	}

	case UM_RESIZE: {
		if (val1 != 7)
			break;

		int width = val2;
		int height = val3;
		if (width > m_itemHeight * 5 + 140)
			width = m_itemHeight * 5 + 140;
		if (height > m_itemHeight * 3 + 180)
			height = m_itemHeight * 3 + 180;
		if (width < 140)
			width = 140;
		if (height < 180)
			height = 180;

		width = ((width - 140) / m_itemHeight) * m_itemHeight + 140;
		height = ((height - 180) / m_itemHeight) * m_itemHeight + 180;
		if (width != m_w || height != m_h) {
			m_viewOffset = 0;
			Resize(width, height);
			m_resizer->Move(m_w - 15, m_h - 15);
			m_scrollBar->Move(m_w - 15, 17);
			m_scrollBar->Resize(m_scrollBar->GetWidth(), m_h - 58);

			int i;
			for (i = 0; i < 2; ++i)
				m_radioBtns[i]->Move(i * 60 + 4, m_h - 16);

			m_sysButton->Move(m_w - 14, 3);

			for (i = 0; i < 5; ++i)
				m_button[i]->Move(i * 26 + 5, m_h - 40);

			m_itemVertNum = (m_h - 180) / m_itemHeight + 5;
			switch (g_serviceType) {
			case ServiceGermany:
			case ServiceBrazil:
			case ServiceVietnam:
			case ServiceFrance:
				m_itemVertNum = m_scrollBar->GetHeight() / m_itemHeight;
				break;
			}

			const char* titleText[2] = { MsgStr(MSI_FRIEND), MsgStr(MSI_PARTY) };
			char buf[64];
			if (m_curRadioBtn == 1) {
				if (g_session.m_partyName.size())
					sprintf(buf, "%s(%s)", titleText[1], g_session.m_partyName.c_str());
				else
					sprintf(buf, "%s", titleText[1]);
			} else {
				sprintf(buf, "%s(%d/%d)", titleText[m_curRadioBtn], (int)m_itemList.size(), 40);
			}

			m_titleText->Resize(m_w - 32, 12);
			m_titleText->SetTextWithShorten(buf);
			m_titleText->Move(16, 3);
			RecalcScrbarPos();
		}

		g_windowMgr.UpdateSnapInfo(this);
		break;
	}

	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

void UIMessengerGroupWnd::ResetGageListPos()
{
	CGameMode* gameMode = g_modeMgr.GetGameMode();

	if (!gameMode || !m_curRadioBtn)
		return;

	int drawCount = 0;
	int itemCount = 0;

	std::list<FRIEND_INFO>::iterator it = m_itemList.begin();

	while (it != m_itemList.end()) {
		if (itemCount < m_itemHorzNum * m_viewOffset) {
			if (m_gageList[itemCount]->GetX() != -100)
				m_gageList[itemCount]->Move(-100, -100);
			it++;
			itemCount++;
			continue;
		}

		if ((*it).isValid) {
			CGameActor* actor;
			if ((*it).AID == g_session.GetAid())
				actor = (CGameActor*)gameMode->GetWorld()->m_player;
			else
				actor = gameMode->GetWorld()->GetGameActorByAID((*it).AID);

			if (actor && !(*it).state) {
				int y = drawCount / m_itemHorzNum * m_itemHeight + 37;
				if (m_gageList[itemCount]->GetY() != y)
					m_gageList[itemCount]->Move(25, y);
			} else {
				if (m_gageList[itemCount]->GetX() != -100)
					m_gageList[itemCount]->Move(-100, -100);
			}
		}

		if (drawCount + 1 == m_itemVertNum * m_itemHorzNum)
			return;

		it++;
		drawCount++;
		itemCount++;
	}
}

UIFriendOptionWnd::UIFriendOptionWnd()
{
	m_info = NULL;
}

UIFriendOptionWnd::~UIFriendOptionWnd()
{
}

void UIFriendOptionWnd::OnCreate(int cx, int cy)
{
	int textWidth;

	textWidth = GetTextWidth(MsgStr(MSI_AUTOOPEN_1ON1_WINDOW), 0, 0, 12, 0) + 12;
	if (textWidth >= m_w - 15)
		textWidth = m_w - 15;
	m_showWhisperWndCheckBox = new UICheckBox;
	m_showWhisperWndCheckBox->Create(textWidth, 12);
	m_showWhisperWndCheckBox->SetBitmap2(
		"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_1.bmp",
		"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_0.bmp",
		MsgStr(MSI_AUTOOPEN_1ON1_WINDOW));
	m_showWhisperWndCheckBox->Move(6, 21);
	m_showWhisperWndCheckBox->SetCheck(g_session.m_isShowWhisperWnd);
	AddChild(m_showWhisperWndCheckBox);

	textWidth = GetTextWidth(MsgStr(MSI_AUTOOPENWHISPERLISTWND_FRIEND), 0, 0, 12, 0) + 12;
	if (textWidth >= m_w - 15)
		textWidth = m_w - 15;
	m_showFriendWhisperWndCheckBox = new UICheckBox;
	m_showFriendWhisperWndCheckBox->Create(textWidth, 12);
	m_showFriendWhisperWndCheckBox->SetBitmap2(
		"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_1.bmp",
		"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_0.bmp",
		MsgStr(MSI_AUTOOPENWHISPERLISTWND_FRIEND));
	m_showFriendWhisperWndCheckBox->Move(6, 37);
	m_showFriendWhisperWndCheckBox->SetCheck(g_session.m_isShowWhisperWnd_Friend);
	AddChild(m_showFriendWhisperWndCheckBox);

	textWidth = GetTextWidth(MsgStr(MSI_WHISPER_OPEN_SOUND), 0, 0, 12, 0) + 12;
	if (textWidth >= m_w - 15)
		textWidth = m_w - 15;
	m_playWhisperOpenSoundCheckBox = new UICheckBox;
	m_playWhisperOpenSoundCheckBox->Create(textWidth, 12);
	m_playWhisperOpenSoundCheckBox->SetBitmap2(
		"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_1.bmp",
		"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_0.bmp",
		MsgStr(MSI_WHISPER_OPEN_SOUND));
	m_playWhisperOpenSoundCheckBox->Move(6, 53);
	m_playWhisperOpenSoundCheckBox->SetCheck(g_session.m_isPlayWhisperOpenSound);
	AddChild(m_playWhisperOpenSoundCheckBox);

	{
		int posX[3] = { m_w - 14 };
		int ids2[3] = { ID_CLOSE };
		char* buttonName2[8] = { "close" };

		std::string preBmpName2 = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sys_";
		for (int i = 0; i < 1; ++i) {
			m_sysButton = new UIBitmapButton;
			m_sysButton->SetBitmapName((preBmpName2 + buttonName2[i] + "_off.bmp").c_str(), BS_NORMAL);
			m_sysButton->SetBitmapName((preBmpName2 + buttonName2[i] + "_on.bmp").c_str(), BS_MOUSEON);
			m_sysButton->SetBitmapName((preBmpName2 + buttonName2[i] + "_on.bmp").c_str(), BS_PRESSED);
			m_sysButton->Create(m_sysButton->GetBitmapWidth(), m_sysButton->GetBitmapHeight());
			m_sysButton->Move(posX[i], 3);
			m_sysButton->SetId(ids2[i]);
			m_sysButton->SetToolTip("Alt+I");
			AddChild(m_sysButton);
		}
	}
}

void UIFriendOptionWnd::OnDraw()
{
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

	TextOutA(5, 3, MsgStr(MSI_FRIEND_SETTING), 0, 0, 12, 0xffffff);
	TextOutA(4, 2, MsgStr(MSI_FRIEND_SETTING), 0, 0, 12, 0);

	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(0, 17, m_w, m_h - 17, RGB(rr, gg, bb));
	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	DrawBox(1, 17, m_w - 2, m_h - 17, RGB(rr, gg, bb));

	CBitmapRes* btnLeft = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_left2.bmp"), false);
	DrawBitmap(0, m_h - btnLeft->m_height, btnLeft, false);

	CBitmapRes* btnRight = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_right2.bmp"), false);
	DrawBitmap(m_w - btnRight->m_width, m_h - btnRight->m_height, btnRight, false);

	int btnMidCount = (m_w - 42) / 21 + (((m_w - 42) % 21) ? 1 : 0);
	for (i = 0; i < btnMidCount; i++) {
		CBitmapRes* mid = (CBitmapRes*)g_resMgr().Get(
			UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\btnbar_mid2.bmp"), false);
		DrawBitmap(21 + i * 21, m_h - mid->m_height, mid, false);
	}
}

INT_PTR UIFriendOptionWnd::SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch (message) {
	case UM_COMMAND:
		switch (val1) {
		case ID_CLOSE:
			g_windowMgr.DeleteWindow(WID_FRIENDOPTIONWND);
			break;
		case ID_CHECK:
			if (m_showWhisperWndCheckBox == sender) {
				g_session.m_isShowWhisperWnd = val2;
				return 0;
			}
			if (m_showFriendWhisperWndCheckBox == sender) {
				g_session.m_isShowWhisperWnd_Friend = val2;
				return 0;
			}
			if (m_playWhisperOpenSoundCheckBox == sender) {
				g_session.m_isPlayWhisperOpenSound = val2;
				return 0;
			}
			break;
		}
		break;
	case UM_SETINFO:
		m_info = (FriendOptionWndInfo*)val1;
		if (!m_info->IsValidInfo())
			m_info->MakeDefault();
		Move(m_info->x, m_info->y);
		break;
	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

void UIFriendOptionWnd::StoreInfo()
{
	if (m_info) {
		m_info->w = m_w;
		m_info->h = m_h;
		m_info->x = m_x;
		m_info->y = m_y;
	}
}
