#include "common.h"
#include "MsgStrings.h"
#include "framework/Locale.h"
#include "window/UIFrameWnd.h"
#include "UIGuildWnd.h"
#include "skin.h"
#include "device/Control.h"
#include "base/Util.h"
#include "GameMode.h"


extern UIWindowMgr g_windowMgr;
extern CModeMgr    g_modeMgr;
extern CSession    g_session;
extern ServiceType g_serviceType;
extern CRenderer*  g_renderer;
extern const char* MsgStr(MSGSTRINGID id);
extern const char* UIBmp(const char* uiBmpName);
extern int FIELD_W[];
extern int BANFIELD_W[];
extern int GUILDFIELD_W[];
extern int PFIELD_W[];


int FIELD_W[7] = { 85, 68, 43, 30, 40, 42, 62 };
int PFIELD_W[5] = { 50, 142, 60, 60, 60 };
int posLineHeight = 19;
int BANFIELD_W[2] = { 82, 292 };
int banishLineHeight = 19;
int GUILDFIELD_W[4] = { 151, 82, 70, 70 };
int guildLineHeight = 19;
extern int posLineHeight;
extern int banishLineHeight;
extern int guildLineHeight;

#include "rag_budget_fill.h"
void UIGuildWnd::InitTabText()
{
	if (m_tabText[0])
		return;

	if (g_guildInfo.m_guildTabBitmask == -1)
		g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENUINTERFACE, 0, 0, 0);

	{
		const char* tabName[6] = {
			MsgStr(MSI_GUILDINFOMANAGE),
			MsgStr(MSI_GUILDMEMBERMANAGE),
			MsgStr(MSI_GUILDPOSITIONMANAGE),
			MsgStr(MSI_GUILDSKILL),
			MsgStr(MSI_GUILDBANISHEDMEMBER),
			MsgStr(MSI_GUILDNOTICE),
		};

		for (int i = 0; i < 6; ++i) {
			RAG_BUDGET_FILL(12)
			m_tabText[i] = new UIStaticText;
			m_tabText[i]->Create(62, 15);
			m_tabText[i]->SetTextWithShorten(tabName[i]);
			m_tabText[i]->Move(i * 66 + 3, 22);
			AddChild(m_tabText[i]);
		}
	}

	{
		int posX[3] = { m_w - 14, };
		int ids2[3] = { ID_CLOSE, };
		std::string preBmpName2 = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sys_";

		for (int i = 0; i < 1; ++i) {
			UIBitmapButton* sysButton = new UIBitmapButton;
			sysButton->SetBitmapName((preBmpName2 + "close" + "_off.bmp").c_str(), BS_NORMAL);
			sysButton->SetBitmapName((preBmpName2 + "close" + "_on.bmp").c_str(), BS_MOUSEON);
			sysButton->SetBitmapName((preBmpName2 + "close" + "_on.bmp").c_str(), BS_PRESSED);
			sysButton->Create(sysButton->GetBitmapWidth(), sysButton->GetBitmapHeight());
			sysButton->Move(posX[i], 3);
			sysButton->SetId(ids2[i]);
			sysButton->SetToolTip("Alt+G");
			AddChild(sysButton);
		}
	}
}

void UIGuildWnd::OnDraw()
{
	unsigned int rr;
	unsigned int gg;
	unsigned int bb;

	DrawTab(GetTab());

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

	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(0, 41, m_w, m_h - 41, RGB(rr, gg, bb));
	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	DrawBox(1, 41, m_w - 2, m_h - 41, RGB(rr, gg, bb));

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

void UIGuildWnd::DrawTab(int curTab)
{
	unsigned int rr;
	unsigned int gg;
	unsigned int bb;

	g_skinMgr.GetColorChipColor(10, 2, rr, gg, bb);
	DrawBox(0, 17, m_w, 24, RGB(rr, gg, bb));

	for (int i = 0; i < 6; ++i) {
		if (curTab == i)
			g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
		else
			g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
		DrawBox(i * 66 + 1, 18, 64, 23, RGB(rr, gg, bb));
	}
}

void UIGuildWnd::OnLBtnDown(int x, int y)
{
	if (y < 17) {
		UIFrameWnd::OnLBtnDown(x, y);
		return;
	}
	if (y <= 41) {
		int curTab = GetTab();
		GetTab();
		if (x < 66)
			x = 0;
		else if (x < 132)
			x = 1;
		else if (x < 198)
			x = 2;
		else if (x < 264)
			x = 3;
		else if (x < 330)
			x = 4;
		else if (x < 396)
			x = 5;
		else if (x < 462)
			x = 6;
		else
			x = 6;
		if (curTab != x)
			SendMsg(NULL, UM_CHANGE_GUILD_TAB, x, 0, 0, 0);
	}
}

void UIGuildWnd::StoreInfo()
{
	if (m_info) {
		m_info->x = m_x;
		m_info->y = m_y;
		m_info->w = m_w;
		m_info->h = m_h;
		m_info->curTab = GetTab();
	}
}

int UIGuildWnd::SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_SETINFO:
		m_info = (GuildWndInfo *)val1;
		if (!m_info->IsValidInfo())
			m_info->MakeDefault();
		Move(m_info->x, m_info->y);
		break;

	case UM_CHANGE_GUILD_TAB: {
		int tab = val1;
		int bitmask[7] = { 0, 1, 2, 4, 0x10, 0x80, 0x40 };

		switch (tab) {
		case 1:
		case 2:
		case 4:
		case 5:
			if ((g_guildInfo.m_guildTabBitmask & bitmask[tab]) == 0)
				return 0;
			break;
		case 3:
			break;
		case 6:
			return 0;
		}

		g_windowMgr.DeleteWindow(WID_GUILDWND);
		switch (tab) {
		case 0:
			g_windowMgr.MakeWindow(WID_GUILDINFOMANAGEWND);
			break;
		case 1:
			g_windowMgr.MakeWindow(WID_GUILDMEMBERMANAGEWND);
			break;
		case 2:
			g_windowMgr.MakeWindow(WID_GUILDPOSITIONMANAGEWND);
			break;
		case 3:
			g_windowMgr.MakeWindow(WID_GUILDSKILLWND);
			break;
		case 4:
			g_windowMgr.MakeWindow(WID_GUILDBANISHEDMEMBERWND);
			break;
		case 5:
			g_windowMgr.MakeWindow(WID_GUILDNOTICEWND);
			break;
		case 6:
			g_windowMgr.MakeWindow(WID_GUILDTOTALINFOWND);
			break;
		}
		break;
	}

	case UM_REFRESH:
		InvalidateChildren();
		break;

	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

UIGuildInfoManageWnd::UIGuildInfoManageWnd()
{
	m_compressedEmblemBitmapFileImage = 0;
	m_compressedEmblemBitmapFileImageSize = 0;
	m_gdid = g_guildInfo.m_gdid;
	m_emblemVersion = g_guildInfo.m_emblemVersion;
}

UIGuildInfoManageWnd::~UIGuildInfoManageWnd()
{
	DeleteCompressedEmblemBitmapFileImage();
}

void UIGuildInfoManageWnd::OnCreate(int cx, int cy)
{
	InitTabText();

	m_curAllyGuild = -1;
	m_curHostileGuild = -1;

	if (strcmp(g_guildInfo.m_masterName.c_str(), "") == 0)
		g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_GUILDINFO, 0, 0);

	g_guildInfo.m_isRequestInfo[GM_GUILDINFO] = TRUE;
	g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_GUILDINFO, 0, 0);
	g_guildInfo.m_isRequestInfo[GM_MEMBERMGR] = TRUE;
	g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_MEMBERMGR, 0, 0);

	char emblemResName[256];
	g_session.GetEmblemFileName(emblemResName, m_gdid, m_emblemVersion);
	m_emblemResName = emblemResName;
	if (!g_resMgr().IsExist(m_emblemResName.c_str()))
		g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_EMBLEM_IMG, m_gdid, 0, 0);

	std::string pathName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\");

	m_emblemChangeButton = new UIBitmapButton;
	m_emblemChangeButton->SetBitmapName((pathName + "btn_edit" + ".bmp").c_str(), BS_NORMAL);
	m_emblemChangeButton->SetBitmapName((pathName + "btn_edit_a" + ".bmp").c_str(), BS_MOUSEON);
	m_emblemChangeButton->SetBitmapName((pathName + "btn_edit_b" + ".bmp").c_str(), BS_PRESSED);
	m_emblemChangeButton->Create(m_emblemChangeButton->GetBitmapWidth(), m_emblemChangeButton->GetBitmapHeight());
	if (g_guildInfo.m_amIMaster)
		m_emblemChangeButton->Move(m_w - m_emblemChangeButton->GetWidth() - 8, 69);
	else
		m_emblemChangeButton->Move(-200, -200);
	m_emblemChangeButton->SetId(ID_CHANGE_GUILE_EMBLEM);
	AddChild(m_emblemChangeButton);
}

void UIGuildInfoManageWnd::OnDraw()
{
	char text[1024];
	unsigned int gg;
	unsigned int bb;
	unsigned int rr;

	UIGuildWnd::OnDraw();

	sprintf(text, "%s : %s", MsgStr(MSI_GUILDNAME), g_guildInfo.m_guildName.c_str());
	TextOutA(8, 49, text, 0, 0, 12, 0);

	sprintf(text, "%s : %d", MsgStr(MSI_GUILDLEVEL), g_guildInfo.m_guildLevel);
	TextOutA(8, 65, text, 0, 0, 12, 0);

	sprintf(text, "%s : %s", MsgStr(MSI_GUILD_MASTER_NAME), g_guildInfo.m_masterName.c_str());
	TextOutA(8, 81, text, 0, 0, 12, 0);

	sprintf(text, "%s : %d / %d", MsgStr(MSI_GUILD_NUM_MEMBER),
		g_guildInfo.m_numMember, g_guildInfo.m_numMaxMember);
	TextOutA(8, 97, text, 0, 0, 12, 0);
	int memberTextWidth = GetTextWidth(text, 0, 0, 12, false);

	sprintf(text, "%s",
		"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\grp_online.bmp");
	CBitmapRes* onlineIcon = (CBitmapRes*)g_resMgr().Get(UIBmp(text), false);
	if (g_serviceType == ServiceKorea) {
		DrawBitmap(128, 95, onlineIcon, true);
		sprintf(text, "   %d", g_guildInfo.m_numOnMember);
		TextOutA(138, 97, text, 0, 0, 12, 0);
	} else {
		DrawBitmap(memberTextWidth + 16, 95, onlineIcon, true);
		sprintf(text, " %d", g_guildInfo.m_numOnMember);
		GetTextWidth(MsgStr(MSI_GUILD_MASTER_NAME), 0, 0, 12, false);
		TextOutA(onlineIcon->m_width + memberTextWidth + 16, 97, text, 0, 0, 12, 0);
	}

	sprintf(text, "%s : %d", MsgStr(MSI_GUILD_AVG_MEMBER_LEVEL), g_guildInfo.m_memberAvgLevel);
	TextOutA(8, 113, text, 0, 0, 12, 0);

	sprintf(text, "%s : %s", MsgStr(MSI_GUILD_MANAGE_LAND), g_guildInfo.m_manageLand.c_str());
	TextOutA(8, 129, text, 0, 0, 12, 0);

	sprintf(text, "%s", MsgStr(MSI_GUILD_FAIR));
	TextOutA(8, 161, text, 0, 0, 12, 0);

	sprintf(text, "%s : %d", MsgStr(MSI_GUILD_EXP), g_guildInfo.m_exp);
	TextOutA(200, 49, text, 0, 0, 12, 0);

	sprintf(text, "%s", MsgStr(MSI_GUILD_EMBLEM));
	TextOutA(200, 73, text, 0, 0, 12, 0);

	sprintf(text, "%s : %d", MsgStr(MSI_GUILD_POINT), g_guildInfo.m_point);
	TextOutA(200, 97, text, 0, 0, 12, 0);

	sprintf(text, "%s", MsgStr(MSI_ALLY_GUILD));
	TextOutA(200, 139, text, 0, 0, 12, 0);

	sprintf(text, "%s", MsgStr(MSI_HOSTILITY_GUILD));
	TextOutA(200, 213, text, 0, 0, 12, 0);

	{
		unsigned int bb;
		unsigned int rr;
		unsigned int gg;
		g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
		DrawBox(299, 64, 26, 26, RGB(rr, gg, bb));
	}

	if (g_resMgr().IsExist(m_emblemResName.c_str())) {
		CEmblemRes* emblem = (CEmblemRes*)g_resMgr().Get(m_emblemResName.c_str(), false);
		if (emblem) DrawBitmap(300, 65, emblem->GetBitmap(), true);
	}

	{
		g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
		DrawBox(23, 188, 90, 90, RGB(rr, gg, bb));

		g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
		DrawBox(24, 189, 88, 88, RGB(rr, gg, bb));

		g_skinMgr.GetColorChipColor(22, 2, rr, gg, bb);
		DrawBox(67, 189, 2, 88, RGB(rr, gg, bb));

		g_skinMgr.GetColorChipColor(22, 2, rr, gg, bb);
		DrawBox(24, 232, 88, 2, RGB(rr, gg, bb));

		g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
		DrawBox(67 - int(float(g_guildInfo.m_honor) * -0.42f),
			232 - int(float(g_guildInfo.m_virtue) * 0.42f), 2, 2, RGB(rr, gg, bb));
	}

	TextOutA(63, 176, "R", 0, 0, 12, 0);
	TextOutA(11, 227, "V", 0, 0, 12, 0);
	TextOutA(64, 281, "W", 0, 0, 12, 0);
	TextOutA(116, 227, "F", 0, 0, 12, 0);

	{
		g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
		DrawBox(200, 155, 168, 48, RGB(rr, gg, bb));
	}

	std::list<RELATED_GUILD_INFORMATION>::iterator it =
		g_guildInfo.m_relatedGuildInfoList.begin();
	int count = 0;
	if (it != g_guildInfo.m_relatedGuildInfoList.end()) {
		int y = 157;
		while (it != g_guildInfo.m_relatedGuildInfoList.end() && y < 205) {
			if (it->relation == 0) {
				if (count == m_curAllyGuild) {
					unsigned int bb;
					unsigned int gg;
					unsigned int rr;
					g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
					DrawBox(204, y, 80, 16, RGB(rr, gg, bb));
				}
				TextOutA(204, y, it->GuildName.c_str(), 0, 0, 12, 0);
				++count;
				y += 16;
			}
			++it;
		}
	}

	{
		g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
		DrawBox(200, 229, 168, 48, RGB(rr, gg, bb));
	}

	it = g_guildInfo.m_relatedGuildInfoList.begin();
	count = 0;
	if (it != g_guildInfo.m_relatedGuildInfoList.end()) {
		int y = 231;
		while (it != g_guildInfo.m_relatedGuildInfoList.end() && y < 279) {
			if (it->relation == 1) {
				if (count == m_curHostileGuild) {
					unsigned int gg;
					unsigned int rr;
					unsigned int bb;
					g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
					DrawBox(204, y, 80, 16, RGB(rr, gg, bb));
				}
				TextOutA(204, y, it->GuildName.c_str(), 0, 0, 12, 0);
				++count;
				y += 16;
			}
			++it;
		}
	}
}

void UIGuildInfoManageWnd::SetEmblemResName()
{
	char emblemResName[256];
	g_session.GetEmblemFileName(emblemResName, m_gdid, m_emblemVersion);
	m_emblemResName = emblemResName;
}

void UIGuildInfoManageWnd::OnRBtnUp(int x, int y)
{
	if (x <= 200 || x >= 288)
		return;

	if (y > 157 && y < 205) {
		int num = g_guildInfo.GetNumRelatedGuild(0);
		m_curAllyGuild = -1;
		m_curHostileGuild = -1;
		if (num) {
			m_curAllyGuild = (y - 157) / 16;
			if (num > m_curAllyGuild)
				SendMsg(NULL, UM_MAKEMENU, 0, 0, 0, 0);
		}
		Invalidate();
	} else if (y > 231 && y < 279) {
		int num = g_guildInfo.GetNumRelatedGuild(1);
		m_curAllyGuild = -1;
		m_curHostileGuild = -1;
		if (num) {
			m_curHostileGuild = (y - 231) / 16;
			if (num > m_curHostileGuild)
				SendMsg(NULL, UM_MAKEMENU, 0, 0, 0, 0);
		}
		Invalidate();
	}
}

void UIGuildInfoManageWnd::DeleteCompressedEmblemBitmapFileImage()
{
	if (m_compressedEmblemBitmapFileImage) {
		delete m_compressedEmblemBitmapFileImage;
		m_compressedEmblemBitmapFileImage = NULL;
		m_compressedEmblemBitmapFileImageSize = 0;
	}
}

void UIGuildInfoManageWnd::RefreshEmblemList()
{
	m_emblemList.clear();

	char path[256];
	sprintf(path, "%s\\emblem\\", g_baseDir3);
	GetFileNames(m_emblemList, path, "*.bmp");
}

void UIGuildInfoManageWnd::OnLBtnDown(int x, int y)
{
	UIGuildWnd::OnLBtnDown(x, y);
}

void UIGuildInfoManageWnd::RefreshCompressedEmblemFileImage(const char* emblemBitmapFileName)
{
	DeleteCompressedEmblemBitmapFileImage();

	char emblemBitmapName[128];
	sprintf(emblemBitmapName, "..\\emblem\\%s", emblemBitmapFileName);

	CFile file;
	if (file.Open(emblemBitmapName, CFile::modeRead)) {
		CBitmapRes emblemBitmap;
		unsigned int fileLength = file.GetLength();
		if (emblemBitmap.LoadFromBuffer(emblemBitmapFileName, file.GetBuf(), fileLength)) {
			if (emblemBitmap.m_width == 24 && emblemBitmap.m_height == 24) {
				m_compressedEmblemBitmapFileImageSize = fileLength;
				m_compressedEmblemBitmapFileImage =
					(char*)operator new(m_compressedEmblemBitmapFileImageSize);
				compress((BYTE*)m_compressedEmblemBitmapFileImage,
						 &m_compressedEmblemBitmapFileImageSize,
						 file.GetBuf(),
						 fileLength);
			}
		}
	}
}

#include "rag_budget_sites.h"
int UIGuildInfoManageWnd::SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	m_x = m_x;
	switch (message) {
	case UM_MENU_SELECTED: {
		CMode* gameMode = (CMode*)g_modeMgr.GetGameMode();
		if (m_menuItems[val1].id != 0x1b)
			break;

		if (m_curAllyGuild != -1) {
			std::list<RELATED_GUILD_INFORMATION>::iterator it =
				g_guildInfo.m_relatedGuildInfoList.begin();
			int count = 0;
			int GDID = 0;
			while (it != g_guildInfo.m_relatedGuildInfoList.end() && count < 3) {
				if (it->relation == 0) {
					if (count == m_curAllyGuild) {
						GDID = it->GDID;
						break;
					}
					++count;
				}
				++it;
			}
			if (GDID)
				gameMode->SendMsg(MM_REQ_DELETE_RELATED_GUILD, GDID, 0, 0);
		} else if (m_curHostileGuild != -1) {
			std::list<RELATED_GUILD_INFORMATION>::iterator it =
				g_guildInfo.m_relatedGuildInfoList.begin();
			int count = 0;
			int GDID = 0;
			while (it != g_guildInfo.m_relatedGuildInfoList.end() && count < 3) {
				if (it->relation == 1) {
					if (count == m_curHostileGuild) {
						GDID = it->GDID;
						break;
					}
					++count;
				}
				++it;
			}
			if (GDID)
				gameMode->SendMsg(MM_REQ_DELETE_RELATED_GUILD, GDID, 1, 0);
		}
		break;
	}

	case UM_MAKEMENU: {
		m_menuItems.erase(m_menuItems.begin(), m_menuItems.end());

		if (g_windowMgr.GetMenu())
			g_windowMgr.DeleteWindow(WID_MENUWND);

		if (g_guildInfo.m_amIMaster) {
			m_menuItems.push_back(StringId(MsgStr(MSI_DELETE), 0x1b));
		}

		if (m_menuItems.size() == 0)
			break;

		UIFrameWnd* menu = g_windowMgr.MakeWindow(WID_MENUWND);
		for (int i = 0; i < m_menuItems.size(); ++i)
			menu->SendMsg(NULL, UM_ADDITEM, (int)m_menuItems[i].itemName.c_str(), 0, 0, 0);

		menu->SendMsg(NULL, UM_SET_CALLER_WID, WID_GUILDINFOMANAGEWND, 0, 0, 0);
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

	case UM_COMMAND:
		switch (val1) {
		case ID_CLOSE:
			g_windowMgr.DeleteWindow(WID_GUILDWND);
			break;

		case ID_CHANGE_GUILE_EMBLEM: {
			if (g_windowMgr.GetComboBoxWnd())
				g_windowMgr.DeleteWindow(WID_COMBOBOXWND);

			RefreshEmblemList();

			int numEmblem = m_emblemList.size();
			if (numEmblem == 0)
				break;

			UIComboBoxWnd* combo = (UIComboBoxWnd*)g_windowMgr.MakeWindow(WID_COMBOBOXWND);
			std::list<std::string>::iterator it = m_emblemList.begin();
			while (it != m_emblemList.end()) {
				combo->SendMsg(NULL, UM_ADDITEM, (int)it->c_str(), 0, 0, 0);
				++it;
			}

			int h = numEmblem * 16 > 160 ? 160 : numEmblem * 16;

			combo->SendMsg(NULL, UM_SETCURITEM, 0, 0, 0, 0);
			combo->Resize(160, h);
			combo->SendMsg(NULL, UM_SET_CALLER_WID, WID_GUILDINFOMANAGEWND, 0, 0, 0);

			int gx = m_w - 163;
			int gy = 86;
			GetGlobalCoor(gx, gy);
			combo->Move(gx + 1, gy + 1);
			break;
		}
		}
		break;

	case UM_COMBOBOX_SELECTED:
		if (m_emblemList.size()) {
			std::string bmpName;
			std::list<std::string>::iterator it = m_emblemList.begin();
			int i = 0; RAG_BUDGET_SITES(1)
			while (it != m_emblemList.end()) {
				if (val1 == i) {
					bmpName = it->c_str();
					break;
				}
				++it;
				++i;
			}

			if (bmpName.size() != 0) {
				char emblemBitmapName[128];
				sprintf(emblemBitmapName, "..\\..\\emblem\\%s", bmpName.c_str());
				CBitmapRes* bitmap = (CBitmapRes*)g_resMgr().Get(emblemBitmapName, false);
				if (bitmap && bitmap->m_width == 24 && bitmap->m_height == 24) {
					RefreshCompressedEmblemFileImage(bmpName.c_str());
					g_modeMgr.GetCurMode()->SendMsg(MM_SEND_GUILD_EMBLEM,
						(int)m_compressedEmblemBitmapFileImage,
						m_compressedEmblemBitmapFileImageSize,
						m_gdid);
					g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_EMBLEM_IMG, m_gdid, 0, 0);
				}
			}
		}
		break;

	case UM_REFRESH: {
		m_gdid = g_guildInfo.m_gdid;
		m_emblemVersion = g_guildInfo.m_emblemVersion;

		SetEmblemResName();

		if (g_guildInfo.m_amIMaster)
			m_emblemChangeButton->Move(m_w - m_emblemChangeButton->GetWidth() - 8, 69);
		else
			m_emblemChangeButton->Move(-200, -200);
		InvalidateChildren();
		break;
	}

	default:
		return UIGuildWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}

UIGuildMemberManageWnd::UIGuildMemberManageWnd()
{
	m_scrollBar = NULL;
	for (int i = 0; i < 5; ++i)
		m_posComboButton[i] = NULL;
}

UIGuildMemberManageWnd::~UIGuildMemberManageWnd()
{}

void UIGuildMemberManageWnd::OnCreate(int cx, int cy)
{
	InitTabText();

	if (strcmp(g_guildInfo.m_masterName.c_str(), "") == 0)
		g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_GUILDINFO, 0, 0);

	g_guildInfo.m_isRequestInfo[GM_MEMBERMGR] = TRUE;
	g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_MEMBERMGR, 0, 0);

	m_memberInfoList = g_guildInfo.GetMemberInfoList();

	m_curItem = 10;
	m_viewOffset = 0;
	m_itemHorzNum = 1;
	m_itemVertNum = 5;
	m_vertScrEnabled = 0;
	m_posComboSeq = 0;

	for (int i = 0; i < 5; ++i) {
		int y = i * 35 + 63;

		m_memberName[i] = new UIStaticText;
		m_memberName[i]->Create(FIELD_W[0] / 2 + 2, 15);
		m_memberName[i]->SetTextWithShorten("");
		m_memberName[i]->Move(8, y);
		AddChild(m_memberName[i]);

		m_positionName[i] = new UIStaticText;
		m_positionName[i]->Create(FIELD_W[1] - 16, 15);
		m_positionName[i]->SetTextWithShorten("");
		m_positionName[i]->Move(FIELD_W[0] + 8, y);
		AddChild(m_positionName[i]);

		m_jobName[i] = new UIStaticText;
		m_jobName[i]->Create(39, 15);
		m_jobName[i]->SetTextWithShorten("");
		m_jobName[i]->Move(FIELD_W[0] + 58, y);
		AddChild(m_jobName[i]);

		m_memo[i] = new UIStaticText;
		m_memo[i]->Create(39, 15);
		m_memo[i]->SetTextWithShorten("");
		m_memo[i]->Move(FIELD_W[0] + 101, y);
		AddChild(m_memo[i]);
	}

	RAG_BUDGET_FILL(16)
	if (g_guildInfo.m_amIMaster) {
		std::string pathName =
			"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\";

		for (int i = 0; i < 5; ++i) {
			m_posComboButton[i] = new UIBitmapButton;
			m_posComboButton[i]->SetBitmapName((pathName + "txtbox_btn_a" + ".bmp").c_str(), BS_NORMAL);
			m_posComboButton[i]->SetBitmapName((pathName + "txtbox_btn_b" + ".bmp").c_str(), BS_MOUSEON);
			m_posComboButton[i]->SetBitmapName((pathName + "txtbox_btn_c" + ".bmp").c_str(), BS_PRESSED);
			m_posComboButton[i]->Create(m_posComboButton[i]->GetBitmapWidth(), m_posComboButton[i]->GetBitmapHeight());
			m_posComboButton[i]->Move(142, i * 35 + 81);
			m_posComboButton[i]->SetId(ID_OPEN_MEMBER_POS_COMBO);
			AddChild(m_posComboButton[i]);
		}
	}

	m_scrollBar = new UIScrollBar;
	m_scrollBar->Create(m_w - 15, 63, 1, m_h - 87);
	AddChild(m_scrollBar);

	{
		std::string pathName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\";
		char* buttonName[][3] = {
			{ "btn_ok", "btn_ok_a", "btn_ok_b", },
		};
		int ids[] = { ID_OK };
		int pos[][2] = {
			{ m_w - 61, m_h - 23 },
		};

		for (int index = 0; index < 1; index++) {
			m_applyButton = new UIBitmapButton;
			m_applyButton->SetBitmapName((pathName + buttonName[index][0] + ".bmp").c_str(), BS_NORMAL);
			m_applyButton->SetBitmapName((pathName + buttonName[index][1] + ".bmp").c_str(), BS_MOUSEON);
			m_applyButton->SetBitmapName((pathName + buttonName[index][2] + ".bmp").c_str(), BS_PRESSED);
			m_applyButton->Create(m_applyButton->GetBitmapWidth(), m_applyButton->GetBitmapHeight());

			if (g_guildInfo.m_amIMaster && m_memberPositionInfoListForChange.size())
				m_applyButton->Move(pos[index][0], pos[index][1]);
			else
				m_applyButton->Move(-200, -200);

			m_applyButton->SetId(ids[index]);
			AddChild(m_applyButton);
		}
		m_defPushId = 0x76;
	}

	RecalcScrbarPos();
}

void UIGuildMemberManageWnd::OnDraw()
{
	char text[512];

	UIGuildWnd::OnDraw();

	for (int hideIndex = 0; hideIndex < 5; ++hideIndex) {
		m_memberName[hideIndex]->Move(-200, -200);
		m_positionName[hideIndex]->Move(-200, -200);
		m_jobName[hideIndex]->Move(-200, -200);
		m_memo[hideIndex]->Move(-200, -200);
		if (m_posComboButton[hideIndex])
			m_posComboButton[hideIndex]->Move(-200, -200);
	}

	unsigned int rr;
	unsigned int gg;
	unsigned int bb;
	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(4, 45, m_w - 22, m_h - 106, RGB(rr, gg, bb));

	int boxW[7] = { FIELD_W[0], FIELD_W[1], FIELD_W[2], FIELD_W[3], FIELD_W[4], FIELD_W[5], FIELD_W[6] };
	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);

	for (int row = 0; row < 6; ++row) {
		int x = 0;
		for (int col = 0; col < 7; ++col) {
			DrawBox(x + 5, row * 35 + 46, boxW[col], 34, RGB(rr, gg, bb));
			x += boxW[col] + 1;
		}
	}

	const char* rowName[7] = {
		MsgStr(MSI_NAME),
		MsgStr(MSI_POSITION),
		MsgStr(MSI_JOB),
		MsgStr(MSI_LEVEL),
		MsgStr(MSI_MEMO),
		MsgStr(MSI_CONTRIBUTION),
		MsgStr(MSI_EXP_CONTIRIBUTION),
	};
	int x = 0;
	for (int title = 0; title < 7; ++title) {
		TextOutA(x + 8, 49, rowName[title], 0, 0, 12, 0);
		x += boxW[title] + 1;
	}

	std::list<GUILD_MEMBER_INFORMATION>::iterator it = m_memberInfoList.begin();
	int drawCount = 0;
	int itemCount = 0;
	for (; it != m_memberInfoList.end(); ) {
		if (itemCount < m_itemHorzNum * m_viewOffset) {
			++it;
			++itemCount;
		} else {
			int itemY = drawCount / m_itemHorzNum * 35 + 84;

			if (it->CurrentState == 1)
				DrawBox(5, itemY - 3, m_w - 24, 34, 0xe4ffe2);

			if (m_curItem == itemCount) {
				unsigned int rr;
				unsigned int gg;
				unsigned int bb;
				g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
				DrawBox(5, itemY - 3, m_w - 24, 34, RGB(rr, gg, bb));
			}

			if (m_posComboButton[drawCount])
				m_posComboButton[drawCount]->Move(142, drawCount * 35 + 81);

			m_memberName[drawCount]->SetTextWithShorten(it->CharName.c_str());
			m_memberName[drawCount]->Move(FIELD_W[0] / 2 + 2, itemY);

			m_positionName[drawCount]->SetTextWithShorten(it->GPositionName.c_str());
			m_positionName[drawCount]->Move(FIELD_W[0] + 9, itemY);

			if (it->CurrentState == 1) {
				char tmpName[256];
				std::string actName = g_session.GetHeadActName(it->Job, it->HeadType, it->Sex, tmpName);

				std::string sprName = g_session.GetHeadSprName(it->Job, it->HeadType, it->Sex, tmpName);

				CActRes* actRes = (CActRes*)g_resMgr().Get(actName.c_str(), false);
				CSprRes* sprRes = (CSprRes*)g_resMgr().Get(sprName.c_str(), false);
				CMotion* motion = actRes->GetMotion(0, 0);
				unsigned int* palette = sprRes->m_pal;
				CSprClip* clip = motion->GetClip(1);
				SprImg* sprImg = sprRes->m_sprites[clip->clipType][clip->sprIndex];

				int headPalette = it->HeadPalette;
				if (headPalette) {
					char palName[64];
					std::string headPaletteName = g_session.GetHeadPaletteName(it->HeadType, it->Job, it->Sex, headPalette, palName);
					CPaletteRes* palRes = (CPaletteRes*)g_resMgr().Get(headPaletteName.c_str(), false);
					palette = palRes->m_pal;
				}

				CacheInfo info;

				CTexture* tex;
				if (clip->clipType == 0) {
					tex = g_renderer->GetSpriteIndex(*sprImg, (DWORD)palette, &info);
					if (!tex)
						tex = g_renderer->AddSpriteIndex(*sprImg, palette, &info);
				} else {
					tex = sprImg->tex;
				}

				m_dc->BltTexture2(8, itemY, tex,
					(int)(info.tu * 256.0f), (int)(info.tv * 256.0f),
					sprImg->width, sprImg->height,
					clip->flags, sprImg->isHalfW + 1, sprImg->isHalfH + 1);
			}

			std::string jobName = g_session.GetJobName(it->Job);

			if (g_serviceType == 12 || g_serviceType == 5 || g_serviceType == 6) {
				if (it->Sex) {
					if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x35d]))
						jobName = MsgStr(MSI_NOVICE);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x1]))
						jobName = MsgStr(MSI_SWORDMAN);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x2]))
						jobName = MsgStr(MSI_MAGICIAN);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x3]))
						jobName = MsgStr(MSI_ARCHER);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x4]))
						jobName = MsgStr(MSI_ACOLYTE);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x5]))
						jobName = MsgStr(MSI_MERCHANT);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x6]))
						jobName = MsgStr(MSI_THIEF);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x7]))
						jobName = MsgStr(MSI_KNIGHT);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x8]))
						jobName = MsgStr(MSI_PRIEST);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x9]))
						jobName = MsgStr(MSI_WIZARD);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xa]))
						jobName = MsgStr(MSI_BLACKSMITH);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xb]))
						jobName = MsgStr(MSI_HUNTER);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xc]))
						jobName = MsgStr(MSI_ASSASSIN);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xe]))
						jobName = MsgStr(MSI_CRUSADER);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xf]))
						jobName = MsgStr(MSI_MONK);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x10]))
						jobName = MsgStr(MSI_SAGE);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x11]))
						jobName = MsgStr(MSI_ROGUE);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x12]))
						jobName = MsgStr(MSI_ALCHEMIST);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x13]))
						jobName = MsgStr(MSI_BARD);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x17]) == 0)
						jobName = MsgStr(MSI_JOB_SUPERNOVICE_M);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x3ca]) == 0)
						jobName = MsgStr(MSI_NOVICE_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa2]) == 0)
						jobName = MsgStr(MSI_SWORDMAN_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa3]) == 0)
						jobName = MsgStr(MSI_MAGICIAN_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa4]) == 0)
						jobName = MsgStr(MSI_ARCHER_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa5]) == 0)
						jobName = MsgStr(MSI_ACOLYTE_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa6]) == 0)
						jobName = MsgStr(MSI_MERCHANT_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa7]) == 0)
						jobName = MsgStr(MSI_THIEF_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa8]) == 0)
						jobName = MsgStr(MSI_KNIGHT_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa9]) == 0)
						jobName = MsgStr(MSI_PRIEST_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfaa]) == 0)
						jobName = MsgStr(MSI_WIZARD_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfab]) == 0)
						jobName = MsgStr(MSI_BLACKSMITH_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfac]) == 0)
						jobName = MsgStr(MSI_HUNTER_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfad]) == 0)
						jobName = MsgStr(MSI_ASSASSIN_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfaf]) == 0)
						jobName = MsgStr(MSI_CRUSADER_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfb0]) == 0)
						jobName = MsgStr(MSI_MONK_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfb1]) == 0)
						jobName = MsgStr(MSI_SAGE_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfb2]) == 0)
						jobName = MsgStr(MSI_ROGUE_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfb3]) == 0)
						jobName = MsgStr(MSI_ALCHEMIST_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfb4]) == 0)
						jobName = MsgStr(MSI_BARD_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfce]) == 0)
						jobName = MsgStr(MSI_JOB_TAEKWON_M);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfcf]) == 0)
						jobName = MsgStr(MSI_KWONSUNG_M);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfd0]) == 0)
						jobName = MsgStr(MSI_KWONSUNG_M);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfd1]) == 0)
						jobName = MsgStr(MSI_SOULLINGKER_M);
				} else {
					if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x35d]))
						jobName = MsgStr(MSI_NOVICE_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x1]))
						jobName = MsgStr(MSI_SWORDMAN_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x2]))
						jobName = MsgStr(MSI_MAGICIAN_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x3]))
						jobName = MsgStr(MSI_ARCHER_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x4]))
						jobName = MsgStr(MSI_ACOLYTE_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x5]))
						jobName = MsgStr(MSI_MERCHANT_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x6]))
						jobName = MsgStr(MSI_THIEF_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x7]))
						jobName = MsgStr(MSI_KNIGHT_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x8]))
						jobName = MsgStr(MSI_PRIEST_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x9]))
						jobName = MsgStr(MSI_WIZARD_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xa]))
						jobName = MsgStr(MSI_BLACKSMITH_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xb]))
						jobName = MsgStr(MSI_HUNTER_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xc]))
						jobName = MsgStr(MSI_ASSASSIN_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xe]))
						jobName = MsgStr(MSI_CRUSADER_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xf]))
						jobName = MsgStr(MSI_MONK_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x10]))
						jobName = MsgStr(MSI_SAGE_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x11]))
						jobName = MsgStr(MSI_ROGUE_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x12]))
						jobName = MsgStr(MSI_ALCHEMIST_W);
					else if (!strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x14]))
						jobName = MsgStr(MSI_DANCER);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x17]) == 0)
						jobName = MsgStr(MSI_JOB_SUPERNOVICE_F);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0x3ca]) == 0)
						jobName = MsgStr(MSI_NOVICE_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa2]) == 0)
						jobName = MsgStr(MSI_SWORDMAN_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa3]) == 0)
						jobName = MsgStr(MSI_MAGICIAN_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa4]) == 0)
						jobName = MsgStr(MSI_ARCHER_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa5]) == 0)
						jobName = MsgStr(MSI_ACOLYTE_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa6]) == 0)
						jobName = MsgStr(MSI_MERCHANT_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa7]) == 0)
						jobName = MsgStr(MSI_THIEF_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa8]) == 0)
						jobName = MsgStr(MSI_KNIGHT_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfa9]) == 0)
						jobName = MsgStr(MSI_PRIEST_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfaa]) == 0)
						jobName = MsgStr(MSI_WIZARD_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfab]) == 0)
						jobName = MsgStr(MSI_BLACKSMITH_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfac]) == 0)
						jobName = MsgStr(MSI_HUNTER_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfad]) == 0)
						jobName = MsgStr(MSI_ASSASSIN_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfaf]) == 0)
						jobName = MsgStr(MSI_CRUSADER_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfb0]) == 0)
						jobName = MsgStr(MSI_MONK_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfb1]) == 0)
						jobName = MsgStr(MSI_SAGE_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfb2]) == 0)
						jobName = MsgStr(MSI_ROGUE_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfb3]) == 0)
						jobName = MsgStr(MSI_ALCHEMIST_H_W);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfb5]) == 0)
						jobName = MsgStr(MSI_DANCER_H);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfce]) == 0)
						jobName = MsgStr(MSI_JOB_TAEKWON_F);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfcf]) == 0)
						jobName = MsgStr(MSI_KWONSUNG_F);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfd0]) == 0)
						jobName = MsgStr(MSI_KWONSUNG_F);
					else if (strcmp(g_session.GetJobName(it->Job), g_session.m_jobNameTable[0xfd1]) == 0)
						jobName = MsgStr(MSI_SOULLINGKER_F);
				}
			}

			m_jobName[drawCount]->SetTextWithShorten(jobName.c_str());
			m_jobName[drawCount]->Move(FIELD_W[0] + FIELD_W[1] + 10, itemY);

			sprintf(text, "%d", it->Level);
			TextOutA(FIELD_W[0] + FIELD_W[1] + FIELD_W[2] + 11, itemY, text, 0, 0, 12, 0);

			m_memo[drawCount]->SetTextWithShorten(it->Memo.c_str());
			m_memo[drawCount]->Move(FIELD_W[0] + FIELD_W[1] + FIELD_W[2] + FIELD_W[3] + 12, itemY);

			sprintf(text, "%0.0f %s", (double)it->Service, "%");
			TextOutA(FIELD_W[0] + FIELD_W[1] + FIELD_W[2] + FIELD_W[3] + FIELD_W[4] + 13, itemY, text, 0, 0, 12, 0);

			sprintf(text, "%d", it->MemberExp);
			TextOutA(FIELD_W[0] + FIELD_W[1] + FIELD_W[2] + FIELD_W[3] + FIELD_W[4] + FIELD_W[5] + 14, itemY, text, 0, 0, 12, 0);

			if (drawCount + 1 == m_itemVertNum * m_itemHorzNum)
				break;
			++it;
			++drawCount;
			++itemCount;
		}
	}
}

int UIGuildMemberManageWnd::GetCurItem(int x, int y)
{
	if (x > m_w - 14 || x < 4 || y < 81 || y >= m_itemVertNum * 35 + 81)
		return -1;

	int yCell = (y - 81) / 35;
	int j = 0;
	int i = 0;
	std::list<GUILD_MEMBER_INFORMATION>::iterator it = m_memberInfoList.begin();
	while (it != m_memberInfoList.end()) {
		if (i < m_viewOffset * m_itemHorzNum) {
			it++;
			++i;
		} else {
			if (j == m_itemHorzNum * yCell)
				return i;
			++it;
			++j;
			++i;
		}
	}
	return -1;
}

void UIGuildMemberManageWnd::OnLBtnDown(int x, int y)
{
	UIGuildWnd::OnLBtnDown(x, y);
	int oldCurItem = m_curItem;
	m_curItem = GetCurItem(x, y);
	if (oldCurItem != m_curItem)
		Invalidate();
}

bool UIGuildMemberManageWnd::GetMemberInfo(int curItem, GUILD_MEMBER_INFORMATION *guildMemberInfo)
{
	int i = 0;
	std::list<GUILD_MEMBER_INFORMATION>::iterator it = m_memberInfoList.begin();
	while (it != m_memberInfoList.end()) {
		if (curItem == i) {
			*guildMemberInfo = *it;
			return true;
		}
		++it;
		++i;
	}
	return false;
}


void UIGuildMemberManageWnd::OnRBtnUp(int x, int y)
{
	SendMsg(NULL, UM_MAKEMENU, 0, 0, 0, 0);
}

void UIGuildMemberManageWnd::OnRBtnDown(int x, int y)
{
	int prev = m_curItem;
	m_curItem = GetCurItem(x, y);
	if (prev != m_curItem)
		Invalidate();
}

int UIGuildMemberManageWnd::SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_COMBOBOX_SELECTED: {
		std::list<GUILD_MEMBER_INFORMATION>::iterator it = m_memberInfoList.begin();
		int i = 0;
		while (it != m_memberInfoList.end()) {
			if (m_curItem == i) {
				int isChange = val1 && it->GPositionID;
				if (it->GPositionID != val1 && isChange) {
					it->GPositionID = val1;
					it->GPositionName = g_guildInfo.m_positionIdNameMap[it->GPositionID];
					m_memberPositionInfoListForChange[it->GID].AID = it->AID;
					m_memberPositionInfoListForChange[it->GID].GID = it->GID;
					m_memberPositionInfoListForChange[it->GID].positionID = it->GPositionID;
				}
				break;
			}
			++it;
			++i;
		}

		int pos[1][2] = { { m_w - 61, m_h - 23 } };
		if (g_guildInfo.m_amIMaster && m_memberPositionInfoListForChange.size())
			m_applyButton->Move(pos[0][0], pos[0][1]);
		else
			m_applyButton->Move(-200, -200);

		Invalidate();
		break;
	}

	case UM_COMMAND:
		switch (val1) {
		case ID_OK:
			if (m_memberPositionInfoListForChange.size()) {
				CMode* curMode = g_modeMgr.GetCurMode();
				g_guildInfo.m_memberPositionInfoListForChange = m_memberPositionInfoListForChange;
				curMode->SendMsg(MM_REQ_CHANGE_MEMBERPOS, 0, 0, 0);
				m_memberPositionInfoListForChange.clear();
			}
			break;

		case ID_OPEN_MEMBER_POS_COMBO:
			if (m_posComboButton[0]) {
				char text[64];
				int i;
				int btnId = 0;
				for (i = 0; i < 5; ++i) {
					if (m_posComboButton[i] == sender) {
						btnId = i;
						break;
					}
				}

				m_posComboSeq = m_viewOffset + btnId;
				m_curItem = m_posComboSeq;
				Invalidate();

				if (g_windowMgr.GetComboBoxWnd())
					g_windowMgr.DeleteWindow(WID_COMBOBOXWND);

				UIComboBoxWnd* combo = (UIComboBoxWnd*)g_windowMgr.MakeWindow(WID_COMBOBOXWND);

				int numPosition = g_guildInfo.m_positionIdNameMap.size();
				for (i = 0; i < numPosition; ++i) {
					sprintf(text, "%s", g_guildInfo.m_positionIdNameMap[i].c_str());
					combo->SendMsg(NULL, UM_ADDITEM, (int)text, 0, 0, 0);
				}

				int h = i * 16 < 80 ? i * 16 : 80;
				combo->SendMsg(NULL, UM_SETCURITEM, 0, 0, 0, 0);
				combo->Resize(150, h);
				combo->SendMsg(NULL, UM_SET_CALLER_WID, WID_GUILDMEMBERMANAGEWND, 0, 0, 0);

				int gx = 88;
				int gy = btnId * 35 + 100;
				GetGlobalCoor(gx, gy);
				combo->Move(gx + 1, gy + 1);
			}
			break;

		case ID_CLOSE:
			g_windowMgr.DeleteWindow(WID_GUILDWND);
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

	case UM_MAKEMENU: {
		m_menuItems.erase(m_menuItems.begin(), m_menuItems.end());

		if (g_windowMgr.GetMenu())
			g_windowMgr.DeleteWindow(WID_MENUWND);

		if (m_curItem != -1) {
			GUILD_MEMBER_INFORMATION info;
			if (GetMemberInfo(m_curItem, &info)) {
				m_menuItems.push_back(StringId(MsgStr(MSI_SEE_INFORMATION_OF_CHARACTER), MENUID_VIEW_GUILD_MEMBER_INFO));

				if (info.GID == g_session.GetGid()) {
					if (info.CharName != g_guildInfo.m_masterName)
						m_menuItems.push_back(StringId(MsgStr(MSI_LEAVE_GUILD), MENUID_LEAVE_GUILD));
				} else {
					if (g_guildInfo.m_masterName == g_session.GetCharName())
						m_menuItems.push_back(StringId(MsgStr(MSI_EXPEL_GUILD), MENUID_REMOVE_GUILE_MEMBER));
					else if (g_guildInfo.m_rightPunish
						&& info.CharName != g_guildInfo.m_masterName)
						m_menuItems.push_back(StringId(MsgStr(MSI_EXPEL_GUILD), MENUID_REMOVE_GUILE_MEMBER));

					if (info.CurrentState == 1 && g_session.m_amIPartyMaster) {
						if (!g_session.GetPartyInfoByName(info.CharName.c_str()).isValid) {
							char msg[200];
							sprintf(msg, MsgStr(MSI_REQ_JOIN_PARTY3), info.CharName.c_str());
							m_menuItems.push_back(StringId(msg, MENUID_INVITE_IN_PARTY));
						}
					}
				}
			}
		}

		if (m_menuItems.size() == 0)
			break;

		UIFrameWnd* menu = g_windowMgr.MakeWindow(WID_MENUWND);
		for (int i = 0; i < m_menuItems.size(); ++i)
			menu->SendMsg(NULL, UM_ADDITEM, (int)m_menuItems[i].itemName.c_str(), 0, 0, 0);

		menu->SendMsg(NULL, UM_SET_CALLER_WID, WID_GUILDMEMBERMANAGEWND, 0, 0, 0);
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

	case UM_MENU_SELECTED:
		switch (m_menuItems[val1].id) {
		case MENUID_REMOVE_GUILE_MEMBER: {
			GUILD_MEMBER_INFORMATION info;
			int vars[3];
			if (GetMemberInfo(m_curItem, &info)) {
				vars[0] = g_guildInfo.m_gdid;
				vars[1] = info.AID;
				vars[2] = info.GID;
				UIWindow* wnd = g_windowMgr.MakeWindow(WID_GUILDBANREASONDESCWND);
				wnd->SendMsg(NULL, UM_SETINFO, (int)vars, 0, 0, 0);
			}
			break;
		}

		case MENUID_LEAVE_GUILD: {
			GUILD_MEMBER_INFORMATION info;
			if (GetMemberInfo(m_curItem, &info) && info.AID == g_session.GetAid()) {
				g_session.m_LoadingTime = info.GID;
				g_windowMgr.MakeWindow(WID_GUILDLEAVEREASONDESCWND);
			}
			break;
		}

		case MENUID_VIEW_GUILD_MEMBER_INFO: {
			GUILD_MEMBER_INFORMATION info;
			if (GetMemberInfo(m_curItem, &info))
				g_modeMgr.GetCurMode()->SendMsg(MM_REQ_OPEN_MEMBER_INFO, info.AID, 0, 0);
			break;
		}

		case MENUID_INVITE_IN_PARTY: {
			GUILD_MEMBER_INFORMATION info;
			if (GetMemberInfo(m_curItem, &info))
				g_modeMgr.GetCurMode()->SendMsg(MM_REQ_JOIN_GROUP, (int)info.CharName.c_str(), 0, 0);
			break;
		}
		}
		break;

	case UM_REFRESH: {
		m_memberPositionInfoListForChange.clear();
		m_memberInfoList = g_guildInfo.GetMemberInfoList();
		InvalidateChildren();
		int pos[1][2] = { { m_w - 61, m_h - 23 } };
		if (g_guildInfo.m_amIMaster && m_memberPositionInfoListForChange.size())
			m_applyButton->Move(pos[0][0], pos[0][1]);
		else
			m_applyButton->Move(-200, -200);
		RecalcScrbarPos();
		break;
	}

	default:
		return UIGuildWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}


void UIGuildMemberManageWnd::RecalcScrbarPos()
{
	if (!m_scrollBar)
		return;

	int numCanDisplayItem = m_itemVertNum;
	int numLine = m_memberInfoList.size() / m_itemHorzNum + ((m_memberInfoList.size() % m_itemHorzNum) != 0);

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
}

void UIGuildMemberManageWnd::OnWheel(int wheel)
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

UIGuildPositionManageWnd::UIGuildPositionManageWnd()
{
	for (int i = 0; i < 11; ++i) {
		m_joinCheckBox[i] = NULL;
		m_punishCheckBox[i] = NULL;
		m_posNameEdit[i] = NULL;
		m_payRateEdit[i] = NULL;
		m_posName[i] = NULL;
		m_payRate[i] = NULL;
	}
}

UIGuildPositionManageWnd::~UIGuildPositionManageWnd()
{}

void UIGuildPositionManageWnd::OnCreate(int cx, int cy)
{
	InitTabText();

	m_posInfoList = g_guildInfo.GetPosInfoList();

	if (strcmp(g_guildInfo.m_masterName.c_str(), "") == 0)
		g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_GUILDINFO, 0, 0);

	g_guildInfo.m_isRequestInfo[GM_POSITION] = TRUE;
	g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_POSITION, 0, 0);

	m_viewOffset = 0;
	m_curItem = 0;
	m_itemHorzNum = 1;
	m_itemVertNum = 11;
	m_vertScrEnabled = 0;

	m_scrollBar = new UIScrollBar;
	m_scrollBar->Create(m_w - 15, 63, 1, m_h - 87);
	AddChild(m_scrollBar);

	int i;
	if (g_guildInfo.m_amIMaster) {
		for (i = 0; i < 11; ++i) {
			m_joinCheckBox[i] = new UICheckBox;
			m_joinCheckBox[i]->Create(12, 12);
			m_joinCheckBox[i]->SetBitmap2(
				"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_1.bmp",
				"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_0.bmp",
				"");
			m_joinCheckBox[i]->Move(m_w - 148, posLineHeight * i + 66);
			m_joinCheckBox[i]->SetCheck2(0);
			AddChild(m_joinCheckBox[i]);
		}

		for (i = 0; i < 11; ++i) {
			m_punishCheckBox[i] = new UICheckBox;
			m_punishCheckBox[i]->Create(12, 12);
			m_punishCheckBox[i]->SetBitmap2(
				"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_1.bmp",
				"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_0.bmp",
				"");
			m_punishCheckBox[i]->Move(m_w - 48, posLineHeight * i + 66);
			m_punishCheckBox[i]->SetCheck2(0);
			AddChild(m_punishCheckBox[i]);
		}
	}

	if (g_guildInfo.m_amIMaster) {
		for (i = 0; i < 11; ++i) {
			m_posNameEdit[i] = new UIEditCtrlVarId;
			m_posNameEdit[i]->Create(PFIELD_W[1] - 4, 16);
			m_posNameEdit[i]->Move(PFIELD_W[0] + 9, posLineHeight * i + 66);
			m_posNameEdit[i]->SetTextLimit(30);
			m_posNameEdit[i]->SetFrameColor(242, 242, 242);
			m_posNameEdit[i]->SetId(ID_POSNAME_EDIT);
			AddChild(m_posNameEdit[i]);
		}

		for (i = 0; i < 11; ++i) {
			m_payRateEdit[i] = new UIEditCtrlVarId;
			m_payRateEdit[i]->Create(PFIELD_W[4] / 2 - 4, 16);
			m_payRateEdit[i]->Move(PFIELD_W[0] + PFIELD_W[1] + PFIELD_W[2] + PFIELD_W[3] + 12, posLineHeight * i + 66);
			m_payRateEdit[i]->SetTextLimit(2);
			m_payRateEdit[i]->SetFrameColor(242, 242, 242);
			m_payRateEdit[i]->SetId(ID_PAYRATE_EDIT);
			AddChild(m_payRateEdit[i]);
		}
	} else {
		for (i = 0; i < 11; ++i) {
			m_posName[i] = new UIStaticText;
			m_posName[i]->Create(PFIELD_W[1] - 4, 15);
			m_posName[i]->SetTextWithShorten("");
			m_posName[i]->Move(PFIELD_W[0] + 9, posLineHeight * i + 66);
			AddChild(m_posName[i]);
		}

		for (i = 0; i < 11; ++i) {
			m_payRate[i] = new UIStaticText;
			m_payRate[i]->Create(PFIELD_W[4] / 2 - 4, 15);
			m_payRate[i]->SetTextWithShorten("");
			m_payRate[i]->Move(PFIELD_W[0] + PFIELD_W[1] + PFIELD_W[2] + PFIELD_W[3] + 12, posLineHeight * i + 66);
			AddChild(m_payRate[i]);
		}
	}

	{
		std::string pathName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\";
		int pos[1][2];
		pos[0][0] = m_w - 61;
		pos[0][1] = m_h - 23;

		m_applyButton = new UIBitmapButton;
		m_applyButton->SetBitmapName((pathName + "btn_ok" + ".bmp").c_str(), BS_NORMAL);
		m_applyButton->SetBitmapName((pathName + "btn_ok_a" + ".bmp").c_str(), BS_MOUSEON);
		m_applyButton->SetBitmapName((pathName + "btn_ok_b" + ".bmp").c_str(), BS_PRESSED);
		m_applyButton->Create(m_applyButton->GetBitmapWidth(), m_applyButton->GetBitmapHeight());
		if (g_guildInfo.m_amIMaster && m_changedPositionInfoList.size())
			m_applyButton->Move(pos[0][0], pos[0][1]);
		else
			m_applyButton->Move(-200, -200);
		m_applyButton->SetId(ID_OK);
		AddChild(m_applyButton);
		m_defPushId = 0x76;
	}

	RecalcScrbarPos();
}

int UIGuildPositionManageWnd::SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
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

	case UM_COMMAND:
		switch (val1) {
		case ID_OK: {
			if (!m_changedPositionInfoList.size())
				break;
			CMode* mode = g_modeMgr.GetCurMode();
			g_guildInfo.m_changedPositionInfoList = m_changedPositionInfoList;
			mode->SendMsg(MM_REQ_CHANGE_GUILD_POSITIONINFO, 0, 0, 0);
			m_changedPositionInfoList.clear();
			m_applyButton->Move(-200, -200);
			InvalidateChildren();
			return 0;
		}

		case ID_CHECK: {
			if (!m_joinCheckBox[0] || !m_punishCheckBox[0])
				break;
			for (int i = 0; i < 11; ++i) {
				if (m_joinCheckBox[i] == sender) {
					int isValid;
					GUILD_POSITION_INFORMATION& posInfo = GetPosInfo(m_viewOffset + i, isValid);
					if (isValid) {
						posInfo.rightOfJoin = val2;
						m_changedPositionInfoList[posInfo.positionID] = posInfo;
					}
				}
				if (m_punishCheckBox[i] == sender) {
					int isValid;
					GUILD_POSITION_INFORMATION& posInfo = GetPosInfo(m_viewOffset + i, isValid);
					if (isValid) {
						posInfo.rightOfPunish = val2;
						m_changedPositionInfoList[posInfo.positionID] = posInfo;
					}
				}
			}
			int pos[1][2] = { { m_w - 61, m_h - 23 } };
			if (g_guildInfo.m_amIMaster && m_changedPositionInfoList.size()) {
				m_applyButton->Move(pos[0][0], pos[0][1]);
				InvalidateChildren();
				return 0;
			}
			m_applyButton->Move(-200, -200);
			InvalidateChildren();
			return 0;
		}

		case ID_CLOSE:
			g_windowMgr.DeleteWindow(WID_GUILDWND);
			return 0;

		case ID_POSNAME_EDIT: {
			if (!m_posNameEdit[0])
				break;
			int btnId = val1 - ID_POSNAME_EDIT;
			for (int i = 0; i < 11; ++i) {
				if (m_posNameEdit[i] == sender) {
					btnId = i;
					break;
				}
			}
			int isValid;
			GUILD_POSITION_INFORMATION& posInfo = GetPosInfo(m_viewOffset + btnId, isValid);
			if (isValid) {
				posInfo.posName = m_posNameEdit[btnId]->GetText();
				m_changedPositionInfoList[posInfo.positionID] = posInfo;
			}
			int pos[1][2] = { { m_w - 61, m_h - 23 } };
			if (g_guildInfo.m_amIMaster && m_changedPositionInfoList.size()) {
				m_applyButton->Move(pos[0][0], pos[0][1]);
				InvalidateChildren();
				return 0;
			}
			m_applyButton->Move(-200, -200);
			InvalidateChildren();
			return 0;
		}

		case ID_PAYRATE_EDIT: {
			if (!m_payRateEdit[0])
				break;
			int btnId = val1 - ID_PAYRATE_EDIT;
			for (int i = 0; i < 11; ++i) {
				if (m_payRateEdit[i] == sender) {
					btnId = i;
					break;
				}
			}
			int isValid;
			GUILD_POSITION_INFORMATION& posInfo = GetPosInfo(m_viewOffset + btnId, isValid);
			if (isValid) {
				posInfo.payRate = atoi(m_payRateEdit[btnId]->GetText());
				m_changedPositionInfoList[posInfo.positionID] = posInfo;
			}
			int pos[1][2] = { { m_w - 61, m_h - 23 } };
			if (g_guildInfo.m_amIMaster && m_changedPositionInfoList.size()) {
				m_applyButton->Move(pos[0][0], pos[0][1]);
				InvalidateChildren();
				return 0;
			}
			m_applyButton->Move(-200, -200);
			InvalidateChildren();
			return 0;
		}
		}
		break;

	case UM_REFRESH: {
		m_posInfoList = g_guildInfo.GetPosInfoList();
		InvalidateChildren();
		int pos[1][2] = { { m_w - 61, m_h - 23 } };
		if (g_guildInfo.m_amIMaster && m_changedPositionInfoList.size()) {
			m_applyButton->Move(pos[0][0], pos[0][1]);
			RecalcScrbarPos();
			return 0;
		}
		m_applyButton->Move(-200, -200);
		RecalcScrbarPos();
		return 0;
	}

	default:
		return UIGuildWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

GUILD_POSITION_INFORMATION& UIGuildPositionManageWnd::GetPosInfo(int curItem, int& isValid)
{
	isValid = 1;
	int i = 0;
	static GUILD_POSITION_INFORMATION sDummy;
	std::list<GUILD_POSITION_INFORMATION>::iterator it = m_posInfoList.begin();
	while (it != m_posInfoList.end()) {
		if (curItem == i)
			return *it;
		++it;
		++i;
	}
	isValid = 0;
	return sDummy;
}

void UIGuildPositionManageWnd::OnDraw()
{
	char text[512];

	UIGuildWnd::OnDraw();

	for (int hideIndex = 0; hideIndex < 11; ++hideIndex) {
		if (m_joinCheckBox[hideIndex])
			m_joinCheckBox[hideIndex]->Move(-200, -200);
		if (m_punishCheckBox[hideIndex])
			m_punishCheckBox[hideIndex]->Move(-200, -200);
		if (m_posNameEdit[hideIndex])
			m_posNameEdit[hideIndex]->Move(-200, -200);
		if (m_payRateEdit[hideIndex])
			m_payRateEdit[hideIndex]->Move(-200, -200);
		if (m_posName[hideIndex])
			m_posName[hideIndex]->Move(-200, -200);
		if (m_payRate[hideIndex])
			m_payRate[hideIndex]->Move(-200, -200);
	}

	unsigned int rr;
	unsigned int gg;
	unsigned int bb;
	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(4, 45, m_w - 22, m_h - posLineHeight - 69, RGB(rr, gg, bb));

	int boxH = posLineHeight - 1;
	int boxW[5] = { PFIELD_W[0], PFIELD_W[1], PFIELD_W[2], PFIELD_W[3], PFIELD_W[4] };
	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	for (int j = 0; j < 12; ++j) {
		int x = 0;
		for (int col = 0; col < 5; ++col) {
			DrawBox(x + 5, posLineHeight * j + 46, boxW[col], boxH, RGB(rr, gg, bb));
			x += boxW[col] + 1;
		}
	}

	const char* rowName[5] = { MsgStr(MSI_GRADE), MsgStr(MSI_POSITION_NAME), MsgStr(MSI_JOIN_AUTHORITY), MsgStr(MSI_PENALTY_AUTORITY), MsgStr(MSI_CONTRIBUTION_PERCENT) };
	int x = 0;
	for (int title = 0; title < 5; ++title) {
		TextOutA(x + 8, 49, rowName[title], 0, 0, 12, 0);
		x += boxW[title] + 1;
	}

	int drawCount = 0;
	int itemCount = 0;
	std::list<GUILD_POSITION_INFORMATION>::iterator it = m_posInfoList.begin();
	if (it != m_posInfoList.end()) {
		do {
			if (itemCount < m_viewOffset * m_itemHorzNum) {
				++it;
				++itemCount;
			} else {
				int y = drawCount / m_itemHorzNum * posLineHeight + 68;
				if (m_curItem == itemCount) {
					unsigned int rr;
					unsigned int gg;
					unsigned int bb;
					g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
					DrawBox(5, y - 3, m_w - 24, 18, RGB(rr, gg, bb));
				}

				sprintf(text, "%d", it->ranking);
				TextOutA(8, y, text, 0, 0, 12, 0);

				if (m_posNameEdit[drawCount]) {
					m_posNameEdit[drawCount]->SetText(it->posName.c_str());
					m_posNameEdit[drawCount]->Move(PFIELD_W[0] + 9, y - 2);
				} else if (m_posName[drawCount]) {
					m_posName[drawCount]->SetTextWithShorten(it->posName.c_str());
					m_posName[drawCount]->Move(PFIELD_W[0] + 9, y - 2);
				}

				if (m_joinCheckBox[drawCount]) {
					m_joinCheckBox[drawCount]->SetCheck2(it->rightOfJoin);
					m_joinCheckBox[drawCount]->Move(PFIELD_W[0] + PFIELD_W[1] + 10, y);
				} else {
					char bmpName[128];
					sprintf(bmpName, "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_%d.bmp", it->rightOfJoin != 0);
					CBitmapRes* bitmap = (CBitmapRes*)g_resMgr().Get(bmpName, false);
					DrawBitmap(PFIELD_W[0] + PFIELD_W[1] + 10, y, bitmap, false);
				}

				if (m_punishCheckBox[drawCount]) {
					m_punishCheckBox[drawCount]->SetCheck2(it->rightOfPunish);
					m_punishCheckBox[drawCount]->Move(PFIELD_W[0] + PFIELD_W[1] + PFIELD_W[2] + 11, y);
				} else {
					char bmpName[128];
					sprintf(bmpName, "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\checkbox_%d.bmp", it->rightOfPunish != 0);
					CBitmapRes* bitmap = (CBitmapRes*)g_resMgr().Get(bmpName, false);
					DrawBitmap(PFIELD_W[0] + PFIELD_W[1] + PFIELD_W[2] + 11, y, bitmap, false);
				}

				if (m_payRateEdit[drawCount]) {
					sprintf(text, "%d", it->payRate);
					m_payRateEdit[drawCount]->SetText(text);
					m_payRateEdit[drawCount]->Move(PFIELD_W[0] + PFIELD_W[1] + PFIELD_W[2] + PFIELD_W[3] + 12, y - 2);
					TextOutA(PFIELD_W[0] + PFIELD_W[1] + PFIELD_W[2] + PFIELD_W[3] + PFIELD_W[4] / 2 + 12, y, "%", 0, 0, 12, 0);
				} else if (m_payRate[drawCount]) {
					sprintf(text, "%d %s", it->payRate, "%");
					m_payRate[drawCount]->SetText(text, 0);
					m_payRate[drawCount]->Move(PFIELD_W[0] + PFIELD_W[1] + PFIELD_W[2] + PFIELD_W[3] + 12, y - 2);
				}

				if (drawCount + 1 == m_itemVertNum * m_itemHorzNum)
					break;
				++it;
				++drawCount;
				++itemCount;
			}
		} while (it != m_posInfoList.end());
	}
}

int UIGuildPositionManageWnd::GetCurItem(int x, int y)
{
	if (x > m_w - 14 || x < 4 || y < posLineHeight + 46 || y >= (m_itemVertNum + 1) * posLineHeight + 46)
		return -1;

	int yCell = (y - posLineHeight - 46) / posLineHeight;
	int j = 0;
	int i = 0;
	std::list<GUILD_POSITION_INFORMATION>::iterator it = m_posInfoList.begin();
	while (it != m_posInfoList.end()) {
		if (i < m_viewOffset * m_itemHorzNum) {
			++it;
			++i;
		} else {
			if (j == m_itemHorzNum * yCell)
				return i;
			++it;
			++j;
			++i;
		}
	}
	return -1;
}

void UIGuildPositionManageWnd::OnRBtnDown(int x, int y)
{
	int prev = m_curItem;
	m_curItem = GetCurItem(x, y);
	if (prev != m_curItem)
		Invalidate();
}

void UIGuildPositionManageWnd::OnLBtnDown(int x, int y)
{
	UIGuildWnd::OnLBtnDown(x, y);
	int oldCurItem = m_curItem;
	m_curItem = GetCurItem(x, y);
	if (oldCurItem != m_curItem)
		Invalidate();
}

void UIGuildPositionManageWnd::Invalidate()
{
	UIWindow::Invalidate();
	g_windowMgr.SetFocusEdit(NULL);
}

void UIGuildPositionManageWnd::RecalcScrbarPos()
{
	if (m_scrollBar) {
		int numCanDisplayItem = m_itemVertNum;

		int numLine = m_posInfoList.size() / m_itemHorzNum + ((m_posInfoList.size() % m_itemHorzNum) != 0);

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
	}
}

void UIGuildPositionManageWnd::OnWheel(int wheel)
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

UIGuildSkillWnd::UIGuildSkillWnd()
{
	m_button[0] = 0;
	m_button[1] = 0;
	m_curItem = -1;
	m_itemHorzNum = 1;
	m_info = 0;
	m_viewOffset = 0;
}

UIGuildSkillWnd::~UIGuildSkillWnd()
{}

void UIGuildSkillWnd::OnCreate(int cx, int cy)
{
	InitTabText();

	int pos[2][2];
	int ids[2];
	char* buttonName[2][3];

	if (strcmp(g_guildInfo.m_masterName.c_str(), "") == 0)
		g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_GUILDINFO, 0, 0);

	g_guildInfo.m_isRequestInfo[GM_SKILL] = TRUE;
	g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_SKILL, 0, 0);

	m_scrollBar = new UIScrollBar;
	m_scrollBar->Create(m_w - 15, 63, 1, m_h - 87);
	AddChild(m_scrollBar);

	m_itemHorzNum = 1;
	m_itemVertNum = (cy - 62) / 32;

	{
		int ids[7] = { ID_LVLUP1, ID_LVLUP2, ID_LVLUP3, ID_LVLUP4, ID_LVLUP5, ID_LVLUP6, ID_LVLUP7 };
		std::string preBmpName1 = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\skill_up_a.bmp";
		std::string preBmpName2 = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\skill_up_b.bmp";
		std::string preBmpName3 = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\skill_up_c.bmp";

		for (int i = 0; i < 7; ++i) {
			m_incArrowButton[i] = new UIBitmapButton;
			m_incArrowButton[i]->SetBitmapName(preBmpName1.c_str(), BS_NORMAL);
			m_incArrowButton[i]->SetBitmapName(preBmpName2.c_str(), BS_MOUSEON);
			m_incArrowButton[i]->SetBitmapName(preBmpName3.c_str(), BS_PRESSED);
			m_incArrowButton[i]->Create(m_incArrowButton[i]->GetBitmapWidth(), m_incArrowButton[i]->GetBitmapHeight());
			m_incArrowButton[i]->Move(78, 45 + i * 32);
			m_incArrowButton[i]->SetId(ids[i]);
			AddChild(m_incArrowButton[i]);
		}
	}

	{
		int ids[7] = { ID_LVLSELECTDN1, ID_LVLSELECTDN2, ID_LVLSELECTDN3, ID_LVLSELECTDN4, ID_LVLSELECTDN5, ID_LVLSELECTDN6, ID_LVLSELECTDN7 };
		std::string preBmpName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\arw_left.bmp";

		for (int i = 0; i < 7; ++i) {
			m_lvlSelectDnButton[i] = new UIBitmapButton;
			m_lvlSelectDnButton[i]->SetBitmapName(preBmpName.c_str(), BS_NORMAL);
			m_lvlSelectDnButton[i]->SetBitmapName(preBmpName.c_str(), BS_MOUSEON);
			m_lvlSelectDnButton[i]->SetBitmapName(preBmpName.c_str(), BS_PRESSED);
			m_lvlSelectDnButton[i]->Create(m_lvlSelectDnButton[i]->GetBitmapWidth(), m_lvlSelectDnButton[i]->GetBitmapHeight());
			m_lvlSelectDnButton[i]->Move(112, 58 + i * 32);
			m_lvlSelectDnButton[i]->SetId(ids[i]);
			AddChild(m_lvlSelectDnButton[i]);
		}
	}

	{
		int ids[7] = { ID_LVLSELECTUP1, ID_LVLSELECTUP2, ID_LVLSELECTUP3, ID_LVLSELECTUP4, ID_LVLSELECTUP5, ID_LVLSELECTUP6, ID_LVLSELECTUP7 };
		std::string preBmpName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\arw_right.bmp";

		for (int i = 0; i < 7; ++i) {
			m_lvlSelectUpButton[i] = new UIBitmapButton;
			m_lvlSelectUpButton[i]->SetBitmapName(preBmpName.c_str(), BS_NORMAL);
			m_lvlSelectUpButton[i]->SetBitmapName(preBmpName.c_str(), BS_MOUSEON);
			m_lvlSelectUpButton[i]->SetBitmapName(preBmpName.c_str(), BS_PRESSED);
			m_lvlSelectUpButton[i]->Create(m_lvlSelectUpButton[i]->GetBitmapWidth(), m_lvlSelectUpButton[i]->GetBitmapHeight());
			m_lvlSelectUpButton[i]->Move(192, 58 + i * 32);
			m_lvlSelectUpButton[i]->SetId(ids[i]);
			AddChild(m_lvlSelectUpButton[i]);
		}
	}

	std::string pathName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\";

	buttonName[0][0] = "btn_use";
	buttonName[0][1] = "btn_use_a";
	buttonName[0][2] = "btn_use_b";
	buttonName[1][0] = "btn_close";
	buttonName[1][1] = "btn_close_a";
	buttonName[1][2] = "btn_close_b";

	ids[0] = 0x76;
	ids[1] = 0x87;

	pos[0][0] = 46;
	pos[0][1] = m_h - 24;
	pos[1][0] = 92;
	pos[1][1] = m_h - 24;

	for (int i = 0; i < 2; ++i) {
		m_button[i] = new UIBitmapButton;
		m_button[i]->SetBitmapName((pathName + buttonName[i][0] + ".bmp").c_str(), BS_NORMAL);
		m_button[i]->SetBitmapName((pathName + buttonName[i][1] + ".bmp").c_str(), BS_MOUSEON);
		m_button[i]->SetBitmapName((pathName + buttonName[i][2] + ".bmp").c_str(), BS_PRESSED);
		m_button[i]->Create(m_button[i]->GetBitmapWidth(), m_button[i]->GetBitmapHeight());
		m_button[i]->Move(pos[i][0], pos[i][1]);
		m_button[i]->SetId(ids[i]);
		AddChild(m_button[i]);
	}

	SendMsg(NULL, UM_REFRESHITEM, 0, 0, 0, 0);
}

void UIGuildSkillWnd::OnDraw()
{
	char text[80];
	char point[64];

	UIGuildWnd::OnDraw();

	int drawCount = 0;
	int itemCount = 0;
	std::list<SKILL_INFO>::iterator it = m_itemList.begin();
	if (it != m_itemList.end()) {
		do {
			if (itemCount < m_viewOffset * m_itemHorzNum) {
				++it;
				++itemCount;
			} else {
				int x = (drawCount % m_itemHorzNum) * 32 + 72;
				int y = (drawCount / m_itemHorzNum) * 32 + 41;
				if (m_curItem == itemCount) {
					unsigned int rr;
					unsigned int gg;
					unsigned int bb;
					g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
					if (it->level == 0)
						g_skinMgr.GetColorChipColor(10, 2, rr, gg, bb);
					else if (it->spcost == 0)
						g_skinMgr.GetColorChipColor(14, 2, rr, gg, bb);
					DrawBox(x + 32, y + 2, 164, 28, RGB(rr, gg, bb));
				}

				g_session.GetSkillBitmapFileName(it->skillName, text);
				CBitmapRes* bitmap = (CBitmapRes*)g_resMgr().Get(text, false);
				DrawBitmap(x - 28, y + 4, bitmap, true);

				int r = 0;
				int g = 0;
				int b = 0;
				if (it->level != 0) {
					if (g_serviceType == (ServiceType)12)
						sprintf(text, "Nv : %d", it->level);
					else
						sprintf(text, "Lv : %d", it->level);
					TextOutA(x + 41, y + 17, text, 0, 0, 12, 0);

					if (it->type == 0)
						sprintf(text, MsgStr(MSI_PASSIVE));
					else
						sprintf(text, "Sp : %d", it->spcost);
					TextOutA(x + 140, y + 17, text, 0, 0, 12, 0);
				} else {
					r = 120;
					g = 120;
					b = 120;
				}

				TextOutA(x + 40, y + 5, it->GetName(), 0, 0, 12, RGB(r, g, b));
				if (drawCount == m_itemVertNum * m_itemHorzNum)
					break;
				++it;
				++drawCount;
				++itemCount;
			}
		} while (it != m_itemList.end());
	}

	sprintf(point, "SkillPoint : %d", g_session.GetGuildSkillPoint());
	TextOutA(m_w - 100, m_h - 20, point, 0, 0, 12, 0);
}

void UIGuildSkillWnd::OnLBtnDown(int x, int y)
{
	int prev = m_curItem;
	m_curItem = GetCurItem(x, y);
	if (prev != m_curItem)
		Invalidate();
	UIGuildWnd::OnLBtnDown(x, y);
}

void UIGuildSkillWnd::OnRBtnDown(int x, int y)
{
	int prev = m_curItem;
	m_curItem = GetCurItem(x, y);
	if (prev != m_curItem)
		Invalidate();
}

void UIGuildSkillWnd::OnRBtnUp(int x, int y)
{
	SendMsg(NULL, UM_MAKEMENU, 0, 0, 0, 0);
}

int UIGuildSkillWnd::GetCurItem(int x, int y)
{
	if (x < 40 || y < 41 || y >= m_itemVertNum * 32 + 41)
		return -1;

	int yCell = (y - 41) / 32;
	int drawCount = 0;
	int itemCount = 0;
	std::list<SKILL_INFO>::iterator it = m_itemList.begin();
	while (it != m_itemList.end()) {
		if (itemCount < m_viewOffset * m_itemHorzNum) {
			++it;
			++itemCount;
		} else {
			if (drawCount == m_itemHorzNum * yCell)
				return itemCount;
			++it;
			++drawCount;
			++itemCount;
		}
	}
	return -1;
}

int UIGuildSkillWnd::SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_COMMAND:
		switch (val1) {
		case ID_CLOSE:
			g_windowMgr.DeleteWindow(WID_GUILDWND);
			return 0;

		case ID_OK:
			if (m_curItem == -1)
				break;
			{
				SKILL_INFO skillInfo = GetSkillInfo(m_curItem);
				if (skillInfo.level)
					SendMsg(NULL, UM_USESKILL, (int)&skillInfo, 0, 0, 0);
				break;
			}

		case ID_LVLUP1:
		case ID_LVLUP2:
		case ID_LVLUP3:
		case ID_LVLUP4:
		case ID_LVLUP5:
		case ID_LVLUP6:
		case ID_LVLUP7:
			if (!g_session.GetGuildSkillPoint())
				break;
			{
				int viewCnt = val1 - ID_LVLUP1;
				int index = m_viewOffset + viewCnt;
				SKILL_INFO skillInfo = GetSkillInfo(index);
				if (skillInfo.m_isValid) {
					CMode* mode = g_modeMgr.GetCurMode();
					mode->SendMsg(MM_UPGRADE_SKILLLEVEL, skillInfo.SKID, 0, 0);
				}
				break;
			}

		case ID_LVLSELECTDN1:
		case ID_LVLSELECTDN2:
		case ID_LVLSELECTDN3:
		case ID_LVLSELECTDN4:
		case ID_LVLSELECTDN5:
		case ID_LVLSELECTDN6:
		case ID_LVLSELECTDN7: {
			int viewCnt = val1 - ID_LVLSELECTDN1;
			int index = m_viewOffset + viewCnt;
			SKILL_INFO skillInfo = GetSkillInfo(index);
			int skillId = skillInfo.SKID;
			if (skillInfo.m_isValid) {
				int useLevel = g_session.GetSkillUseLevel(skillId);
				--useLevel;
				if (useLevel <= 0)
					useLevel = 1;
				if (useLevel > skillInfo.level)
					useLevel = skillInfo.level;
				g_session.SetSkillUseLevel(skillId, useLevel);
				Invalidate();
			}
			break;
		}

		case ID_LVLSELECTUP1:
		case ID_LVLSELECTUP2:
		case ID_LVLSELECTUP3:
		case ID_LVLSELECTUP4:
		case ID_LVLSELECTUP5:
		case ID_LVLSELECTUP6:
		case ID_LVLSELECTUP7: {
			int viewCnt = val1 - ID_LVLSELECTUP1;
			int index = m_viewOffset + viewCnt;
			SKILL_INFO skillInfo = GetSkillInfo(index);
			int skillId = skillInfo.SKID;
			if (skillInfo.m_isValid) {
				int useLevel = g_session.GetSkillUseLevel(skillId);
				++useLevel;
				if (useLevel <= 0)
					useLevel = 1;
				if (useLevel > skillInfo.level)
					useLevel = skillInfo.level;
				g_session.SetSkillUseLevel(skillId, useLevel);
				Invalidate();
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
			RefreshButton();
			Invalidate();
		}
		return m_viewOffset - oldViewOffset;
	}

	case UM_REFRESHITEM: {
		m_itemList.erase(m_itemList.begin(), m_itemList.end());

		int skillCount = g_session.m_guildSkillList.size();
		for (int i = 0; i < skillCount; ++i) {
			SKILL_INFO skillInfo = g_session.GetGuildSkillItemInfoBySequence(i);
			if (skillInfo.m_isValid) {
				if (skillInfo.upgradable || skillInfo.level)
					m_itemList.push_back(skillInfo);
			}
		}

		Invalidate();
		RecalcScrbarPos();
		RefreshButton();
		return 0;
	}

	case UM_MAKEMENU: {
		m_menuItems.erase(m_menuItems.begin(), m_menuItems.end());

		if (g_windowMgr.GetMenu())
			g_windowMgr.DeleteWindow(WID_MENUWND);

		if (m_curItem != -1) {
			SKILL_INFO skillInfo = GetSkillInfo(m_curItem);
			if (skillInfo.m_isValid) {
				m_menuItems.push_back(StringId(((std::string)MsgStr(MSI_SEE_INFORMATION_OF_SKILL)).c_str(), 3));
			}
		}

		if (m_menuItems.size() == 0)
			break;

		UIFrameWnd* menu = g_windowMgr.MakeWindow(WID_MENUWND);
		for (int i = 0; i < m_menuItems.size(); ++i)
			menu->SendMsg(NULL, UM_ADDITEM, (int)m_menuItems[i].itemName.c_str(), 0, 0, 0);

		menu->SendMsg(NULL, UM_SET_CALLER_WID, WID_GUILDSKILLWND, 0, 0, 0);
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
		return 0;
	}

	case UM_MENU_SELECTED:
		if (m_curItem == -1)
			break;
		{
			SKILL_INFO skillInfo = GetSkillInfo(m_curItem);
			if (skillInfo.m_isValid) {
				switch (m_menuItems[val1].id) {
				case 3: {
					UIWindow* wnd = g_windowMgr.MakeWindow(WID_SKILLDESCRIBEWND);
					wnd->SendMsg(NULL, UM_SKILLDESCRIBE, skillInfo.SKID, 0, 0, 0);
					break;
				}
				case 14: {
					UIWindow* wnd = g_windowMgr.MakeWindow(WID_SKILLNAMECHANGEWND);
					wnd->SendMsg(NULL, 0x44, skillInfo.SKID, 0, 0, 0);
					break;
				}
				}
			}
			break;
		}

	case UM_USESKILL: {
		SKILL_INFO* skillInfo = (SKILL_INFO*)val1;
		if (skillInfo->level == 0)
			break;
		int skillId = skillInfo->SKID;
		CMode* mode = g_modeMgr.GetCurMode();
		int useLevel = g_session.GetSkillUseLevel(skillId);
		mode->SendMsg(MM_USING_SKILL, val1, useLevel, 0);
		break;
	}

	default:
		return UIGuildWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}

SKILL_INFO UIGuildSkillWnd::GetSkillInfo(int curItem)
{
	int count = 0;
	SKILL_INFO skillInfo(0);
	std::list<SKILL_INFO>::iterator it = m_itemList.begin();

	while (it != m_itemList.end()) {
		if (curItem == count) {
			skillInfo = *it;
			break;
		}
		++it;
		++count;
	}

	return skillInfo;
}

void UIGuildSkillWnd::OnWheel(int wheel)
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

void UIGuildSkillWnd::RecalcScrbarPos()
{
	if (m_scrollBar) {
		int numCanDisplayItem = m_itemVertNum;

		int numLine = m_itemList.size() / m_itemHorzNum + ((m_itemList.size() % m_itemHorzNum) != 0);

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
	}
}

SKILL_INFO UIGuildSkillWnd::GetSkillInfoByViewCnt(int viewCnt)
{
	std::list<SKILL_INFO>::iterator it = m_itemList.begin();
	int count = 0;
	while (it != m_itemList.end()) {
		if (m_viewOffset + viewCnt == count)
			return *it;
		++it;
		++count;
	}
	return SKILL_INFO(0);
}
void UIGuildSkillWnd::RefreshButton()
{
	for (int i = 0, y = 34; y < 258; ++i, y += 32) {
		SKILL_INFO sInfo = GetSkillInfoByViewCnt(i);

		if (g_guildInfo.m_amIMaster
			&& i < m_itemVertNum
			&& m_viewOffset + i < m_itemList.size()
			&& g_session.GetGuildSkillPoint()
			&& sInfo.m_isValid
			&& sInfo.upgradable) {
			m_incArrowButton[i]->Move(78, y + 11);
		} else {
			m_incArrowButton[i]->Move(-200, y - 17);
		}

		m_lvlSelectDnButton[i]->Move(-200, y);
		m_lvlSelectUpButton[i]->Move(-200, y);
	}
}








UIGuildBanishedMemberWnd::UIGuildBanishedMemberWnd()
{}


UIGuildBanishedMemberWnd::~UIGuildBanishedMemberWnd() {}

void UIGuildBanishedMemberWnd::OnCreate(int cx, int cy)
{
	InitTabText();

	if (strcmp(g_guildInfo.m_masterName.c_str(), "") == 0)
		g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_GUILDINFO, 0, 0);

	g_guildInfo.m_isRequestInfo[GM_BANLIST] = TRUE;
	g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_BANLIST, 0, 0);

	m_viewOffset = 0;
	m_curItem = 0;
	m_itemHorzNum = 1;
	m_itemVertNum = 11;
	m_vertScrEnabled = 0;

	m_scrollBar = new UIScrollBar;
	m_scrollBar->Create(m_w - 15, 63, 1, m_h - 87);
	AddChild(m_scrollBar);

	for (int i = 0; i < 11; ++i) {
		m_memberName[i] = new UIStaticText;
		m_memberName[i]->Create(BANFIELD_W[0] - 4, 15);
		m_memberName[i]->SetTextWithShorten("");
		m_memberName[i]->Move(8, banishLineHeight * i + 63);
		AddChild(m_memberName[i]);

		m_reason[i] = new UIStaticText;
		m_reason[i]->Create(BANFIELD_W[1] - 4, 15);
		m_reason[i]->SetTextWithShorten("");
		m_reason[i]->Move(FIELD_W[0] + 58, banishLineHeight * i + 63);
		AddChild(m_reason[i]);
	}

	RecalcScrbarPos();
}

void UIGuildBanishedMemberWnd::OnDraw()
{
	UIGuildWnd::OnDraw();

	for (int hideIndex = 0; hideIndex < 11; ++hideIndex) {
		m_memberName[hideIndex]->Move(-200, -200);
		m_reason[hideIndex]->Move(-200, -200);
	}

	unsigned int rr;
	unsigned int gg;
	unsigned int bb;
	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(4, 45, m_w - 22, m_h - banishLineHeight - 69, RGB(rr, gg, bb));

	int boxH = banishLineHeight - 1;
	int boxW[2] = { BANFIELD_W[0], BANFIELD_W[1] };
	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	for (int j = 0; j < 12; ++j) {
		int x = 0;
		for (int col = 0; col < 2; ++col) {
			DrawBox(x + 5, banishLineHeight * j + 46, boxW[col], boxH, RGB(rr, gg, bb));
			x += boxW[col] + 1;
		}
	}

	const char* rowName[2] = { MsgStr(MSI_NAME), MsgStr(MSI_GUILD_KICK_REASON) };
	int x = 0;
	for (int title = 0; title < 2; ++title) {
		TextOutA(x + 8, 49, rowName[title], 0, 0, 12, 0);
		x += boxW[title] + 1;
	}

	int drawCount = 0;
	int itemCount = 0;
	std::list<SGUILD_BANISH_INFO>::iterator it = g_guildInfo.m_banishList.begin();
	if (it != g_guildInfo.m_banishList.end()) {
		do {
			if (itemCount < m_itemHorzNum * m_viewOffset) {
				++it;
				++itemCount;
			} else {
				int y = drawCount / m_itemHorzNum * banishLineHeight + 68;
				if (m_curItem == itemCount) {
					unsigned int rr;
					unsigned int gg;
					unsigned int bb;
					g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
					DrawBox(5, y - 3, m_w - 24, 18, RGB(rr, gg, bb));
				}
				m_memberName[drawCount]->SetTextWithShorten(it->MemberName.c_str());
				m_memberName[drawCount]->Move(8, y);
				m_reason[drawCount]->SetTextWithShorten(it->Reason.c_str());
				m_reason[drawCount]->Move(BANFIELD_W[0] + 9, y);
				if (drawCount + 1 == m_itemVertNum * m_itemHorzNum)
					break;
				++it;
				++drawCount;
				++itemCount;
			}
		} while (it != g_guildInfo.m_banishList.end());
	}
}

int UIGuildBanishedMemberWnd::GetCurItem(int x, int y)
{
	if (x > m_w - 14 || x < 4 || y < banishLineHeight + 46 || y >= (m_itemVertNum + 1) * banishLineHeight + 46)
		return -1;

	int yCell = (y - banishLineHeight - 46) / banishLineHeight;
	int j = 0;
	int i = 0;
	std::list<SGUILD_BANISH_INFO>::iterator it = g_guildInfo.m_banishList.begin();
	while (it != g_guildInfo.m_banishList.end()) {
		if (i < m_viewOffset * m_itemHorzNum) {
			++it;
			++i;
		} else {
			if (j == m_itemHorzNum * yCell)
				return i;
			++it;
			++j;
			++i;
		}
	}
	return -1;
}

void UIGuildBanishedMemberWnd::OnLBtnDown(int x, int y)
{
	UIGuildWnd::OnLBtnDown(x, y);
	int oldCurItem = m_curItem;
	m_curItem = GetCurItem(x, y);
	if (oldCurItem != m_curItem)
		Invalidate();
}

int UIGuildBanishedMemberWnd::SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_COMMAND:
		if (val1 == 0x87)
			g_windowMgr.DeleteWindow(WID_GUILDWND);
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

	case UM_REFRESH:
		InvalidateChildren();
		RecalcScrbarPos();
		break;

	default:
		return UIGuildWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}

SGUILD_BANISH_INFO UIGuildBanishedMemberWnd::GetBanishInfo(int curItem, int &isValid)
{
	int i = 0;
	isValid = 1;
	SGUILD_BANISH_INFO banishInfo;

	std::list<SGUILD_BANISH_INFO>::iterator it = g_guildInfo.m_banishList.begin();
	while (it != g_guildInfo.m_banishList.end()) {
		if (curItem == i)
			return *it;
		++it;
		++i;
	}

	isValid = 0;
	return banishInfo;
}


void UIGuildBanishedMemberWnd::OnWheel(int wheel)
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

void UIGuildBanishedMemberWnd::RecalcScrbarPos()
{
	if (m_scrollBar) {
		int numCanDisplayItem = m_itemVertNum;

		int numLine = g_guildInfo.m_banishList.size() / m_itemHorzNum + ((g_guildInfo.m_banishList.size() % m_itemHorzNum) != 0);

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
	}
}

UIGuildNoticeWnd::UIGuildNoticeWnd()
{}


UIGuildNoticeWnd::~UIGuildNoticeWnd() {}
void UIGuildNoticeWnd::OnCreate(int cx, int cy)
{
	InitTabText();

	m_noticeTitleEdited = 0;
	m_noticeEdited = 0;

	if (strcmp(g_guildInfo.m_masterName.c_str(), "") == 0)
		g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_GUILDINFO, 0, 0);

	m_noticeTitleEdit = new UIEditCtrlVarId;
	m_noticeTitleEdit->Create(m_w - 60, 16);
	m_noticeTitleEdit->Move(50, 66);
	m_noticeTitleEdit->SetTextLimit(60);
	m_noticeTitleEdit->SetFrameColor(242, 242, 242);
	m_noticeTitleEdit->SetId(ID_NOTICE_TITLE_EDITED);
	AddChild(m_noticeTitleEdit);
	g_windowMgr.SetFocusEdit(m_noticeTitleEdit);

	int y = 104;
	UIEditCtrlVarId** noticeEdit = m_noticeEdit;
	while (y < 275) {
		*noticeEdit = new UIEditCtrlVarId;
		(*noticeEdit)->Create(m_w - 10, 16);
		(*noticeEdit)->Move(5, y);
		(*noticeEdit)->SetTextLimit(120);
		(*noticeEdit)->SetFrameColor(242, 242, 242);
		(*noticeEdit)->SetId(ID_NOTICE_EDITED);
		AddChild(*noticeEdit);
		y += 19;
		++noticeEdit;
	}

	m_noticeTitleEdit->SetText(g_guildInfo.m_subjectName.c_str());
	m_noticeEdit[0]->SetText(g_guildInfo.m_noticeName.c_str());

	std::string pathName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\";

	m_sendButton = new UIBitmapButton;
	m_sendButton->SetBitmapName((pathName + "btn_send" + ".bmp").c_str(), BS_NORMAL);
	m_sendButton->SetBitmapName((pathName + "btn_send_a" + ".bmp").c_str(), BS_MOUSEON);
	m_sendButton->SetBitmapName((pathName + "btn_send_b" + ".bmp").c_str(), BS_PRESSED);
	m_sendButton->Create(m_sendButton->GetBitmapWidth(), m_sendButton->GetBitmapHeight());
	if (g_guildInfo.m_amIMaster && m_noticeTitleEdited && m_noticeEdited)
		m_sendButton->Move(m_w - 46, m_h - 24);
	else
		m_sendButton->Move(-200, -200);
	m_sendButton->SetId(ID_SEND_GUILD_NOTICE);
	AddChild(m_sendButton);
}

void UIGuildNoticeWnd::OnDraw()
{
	UIGuildWnd::OnDraw();
	char temp[64];
	sprintf(temp, "%s:", MsgStr(MSI_TITLE));
	TextOutA(5, 67, temp, 0, 0, 12, 0);
	TextOutA(5, 86, MsgStr(MSI_CONTENTS), 0, 0, 12, 0);
}

void UIGuildNoticeWnd::OnLBtnDown(int x, int y)
{
	UIGuildWnd::OnLBtnDown(x, y);
}

int UIGuildNoticeWnd::SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_COMMAND:
		switch (val1) {
		case ID_CLOSE:
			g_windowMgr.DeleteWindow(WID_GUILDWND);
			break;
		case ID_SEND_GUILD_NOTICE: {
			std::string notice;
			for (int i = 0; i < 9; ++i) {
				if (m_noticeEdit[i]->GetTextSize()) {
					notice += m_noticeEdit[i]->GetText();
					notice += " ";
				}
			}
			if (g_guildInfo.m_amIMaster) {
				g_modeMgr.GetCurMode()->SendMsg(MM_GUILD_NOTICE, g_guildInfo.m_gdid,
					(int)m_noticeTitleEdit->GetText(), (int)notice.c_str());
			}
			m_sendButton->Move(-200, -200);
			m_noticeTitleEdited = 0;
			m_noticeEdited = 0;
			InvalidateChildren();
			break;
		}
		case ID_NOTICE_TITLE_EDITED:
			m_noticeTitleEdited = 1;
			if (m_noticeEdited) {
				if (g_guildInfo.m_amIMaster) {
					m_sendButton->Move(m_w - 46, m_h - 24);
					InvalidateChildren();
				}
			}
			break;
		case ID_NOTICE_EDITED:
			m_noticeEdited = 1;
			if (m_noticeTitleEdited) {
				if (g_guildInfo.m_amIMaster) {
					m_sendButton->Move(m_w - 46, m_h - 24);
					InvalidateChildren();
				}
			}
			break;
		}
		break;
	case UM_REFRESH:
		if (g_guildInfo.m_amIMaster && m_noticeTitleEdited && m_noticeEdited)
			m_sendButton->Move(m_w - 46, m_h - 24);
		else
			m_sendButton->Move(-200, -200);
		InvalidateChildren();
		break;
	default:
		return UIGuildWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

UIGuildTotalInfoWnd::UIGuildTotalInfoWnd()
{}


UIGuildTotalInfoWnd::~UIGuildTotalInfoWnd() {}

void UIGuildTotalInfoWnd::OnCreate(int cx, int cy)
{
	InitTabText();

	if (strcmp(g_guildInfo.m_masterName.c_str(), "") == 0)
		g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_GUILDINFO, 0, 0);

	g_guildInfo.m_isRequestInfo[GM_ALLGUILDLIST] = TRUE;
	g_modeMgr.GetCurMode()->SendMsg(MM_REQ_GUILD_MENU, GM_ALLGUILDLIST, 0, 0);

	m_viewOffset = 0;
	m_curItem = 0;
	m_itemHorzNum = 1;
	m_itemVertNum = 12;
	m_vertScrEnabled = 0;

	m_scrollBar = new UIScrollBar;
	m_scrollBar->Create(m_w - 15, 63, 1, m_h - 87);
	AddChild(m_scrollBar);

	for (int i = 0; i < 12; ++i) {
		m_guildName[i] = new UIStaticText;
		m_guildName[i]->Create(BANFIELD_W[0] - 4, 15);
		m_guildName[i]->SetTextWithShorten("");
		m_guildName[i]->Move(8, guildLineHeight * i + 63);
		AddChild(m_guildName[i]);
	}

	RecalcScrbarPos();
}

void UIGuildTotalInfoWnd::OnDraw()
{
	char text[512];

	UIGuildWnd::OnDraw();

	for (int hideIndex = 0; hideIndex < 12; ++hideIndex)
		m_guildName[hideIndex]->Move(-200, -200);

	unsigned int rr;
	unsigned int gg;
	unsigned int bb;
	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(4, 45, m_w - 22, m_h - 69, RGB(rr, gg, bb));

	int boxH = guildLineHeight - 1;
	int boxW[4] = { GUILDFIELD_W[0], GUILDFIELD_W[1], GUILDFIELD_W[2], GUILDFIELD_W[3] };
	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	for (int j = 0; j < 12; ++j) {
		int x = 0;
		for (int col = 0; col < 4; ++col) {
			DrawBox(x + 5, guildLineHeight * j + 46, boxW[col], boxH, RGB(rr, gg, bb));
			x += boxW[col] + 1;
		}
	}

	const char* rowName[4] = { MsgStr(MSI_GUILD_NAME), MsgStr(MSI_GUILD_LEVEL), MsgStr(MSI_GUILD_PEOPLE_COUNT), MsgStr(MSI_RANKING) };
	int x = 0;
	for (int title = 0; title < 4; ++title) {
		TextOutA(x + 8, 49, rowName[title], 0, 0, 12, 0);
		x += boxW[title] + 1;
	}

	int drawCount = 0;
	int itemCount = 0;
	std::list<SGUILD_INFO>::iterator it = g_guildInfo.m_guildList.begin();
	while (it != g_guildInfo.m_guildList.end()) {
		int skipCount = m_itemHorzNum;
		if (itemCount < skipCount * m_viewOffset) {
			++it;
			++itemCount;
			continue;
		}

		int y = drawCount / m_itemHorzNum * banishLineHeight + 68;
		if (m_curItem == itemCount) {
			unsigned int rr;
			unsigned int gg;
			unsigned int bb;
			g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
			DrawBox(5, y - 3, m_w - 24, 18, RGB(rr, gg, bb));
		}
		m_guildName[drawCount]->SetTextWithShorten(it->GuildName.c_str());
		m_guildName[drawCount]->Move(8, y);
		sprintf(text, "%d", it->Level);
		TextOutA(GUILDFIELD_W[0] + 9, y, text, 0, 0, 12, 0);
		sprintf(text, "%d/%d", it->UserNum, it->MaxUserNum);
		TextOutA(GUILDFIELD_W[0] + GUILDFIELD_W[1] + 10, y, text, 0, 0, 12, 0);
		sprintf(text, "%d", it->Ranking);
		TextOutA(GUILDFIELD_W[0] + GUILDFIELD_W[1] + GUILDFIELD_W[2] + 11, y, text, 0, 0, 12, 0);
		if (drawCount + 1 == m_itemVertNum * m_itemHorzNum)
			break;
		++it;
		++drawCount;
		++itemCount;
	}
}

int UIGuildTotalInfoWnd::GetCurItem(int x, int y)
{
	if (x > m_w - 14 || x < 4 || y < guildLineHeight + 46 || y >= (m_itemVertNum + 1) * guildLineHeight + 46)
		return -1;

	int yCell = (y - guildLineHeight - 46) / guildLineHeight;
	int j = 0;
	int i = 0;
	std::list<SGUILD_INFO>::iterator it = g_guildInfo.m_guildList.begin();
	while (it != g_guildInfo.m_guildList.end()) {
		if (i < m_viewOffset * m_itemHorzNum) {
			++it;
			++i;
		} else {
			if (j == m_itemHorzNum * yCell)
				return i;
			++it;
			++j;
			++i;
		}
	}
	return -1;
}

void UIGuildTotalInfoWnd::OnLBtnDown(int x, int y)
{
	UIGuildWnd::OnLBtnDown(x, y);
	int oldCurItem = m_curItem;
	m_curItem = GetCurItem(x, y);
	if (oldCurItem != m_curItem)
		Invalidate();
}

int UIGuildTotalInfoWnd::SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_COMMAND:
		if (val1 == 0x87)
			g_windowMgr.DeleteWindow(WID_GUILDWND);
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

	default:
		return UIGuildWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

SGUILD_INFO UIGuildTotalInfoWnd::GetBanishInfo(int curItem, int &isValid)
{
	int i = 0;
	isValid = 1;
	SGUILD_INFO gInfo;

	std::list<SGUILD_INFO>::iterator it = g_guildInfo.m_guildList.begin();
	while (it != g_guildInfo.m_guildList.end()) {
		if (curItem == i)
			return *it;
		++it;
		++i;
	}

	isValid = 0;
	return gInfo;
}

void UIGuildTotalInfoWnd::OnWheel(int wheel)
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

void UIGuildTotalInfoWnd::RecalcScrbarPos()
{
	if (m_scrollBar) {
		int numCanDisplayItem = m_itemVertNum;

		int numLine = g_guildInfo.m_guildList.size() / m_itemHorzNum + ((g_guildInfo.m_guildList.size() % m_itemHorzNum) != 0);

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
	}
}

UIGuildLeaveReasonDescWnd::UIGuildLeaveReasonDescWnd()
{
	m_editCtrl = 0;
	m_titleText = 0;
}


UIGuildLeaveReasonDescWnd::~UIGuildLeaveReasonDescWnd() {}
void UIGuildLeaveReasonDescWnd::OnCreate(int cx, int cy)
{
	m_editCtrl = new UIEditCtrl;
	m_editCtrl->Create(m_w - 32, 16);
	m_editCtrl->Move(15, 22);
	m_editCtrl->SetTextLimit(39);
	m_editCtrl->SetFrameColor(232, 232, 232);
	m_editCtrl->SetText("");
	AddChild(m_editCtrl);
	g_windowMgr.SetFocusEdit(m_editCtrl);

	m_titleText = new UIStaticText;
	m_titleText->Create(m_w - 30, 15);
	m_titleText->Move(15, 6);
	m_titleText->SetText(MsgStr(MSI_PLEASE_INPUT_WHY_LEAVING), 0);
	AddChild(m_titleText);

	{
		std::string pathName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\";
		char* buttonName[2][3] = {
			{ "btn_ok", "btn_ok_a", "btn_ok_b" },
			{ "btn_cancel", "btn_cancel_a", "btn_cancel_b" },
		};
		int ids[2] = { 0x76, 0x77 };
		int pos[2][2] = {
			{ m_w - 101, m_h - 32 },
			{ m_w - 55, m_h - 32 },
		};

		for (int i = 0; i < 2; ++i) {
			UIBitmapButton* button = new UIBitmapButton;
			button->SetBitmapName((pathName + buttonName[i][0] + ".bmp").c_str(), 0);
			button->SetBitmapName((pathName + buttonName[i][1] + ".bmp").c_str(), 1);
			button->SetBitmapName((pathName + buttonName[i][2] + ".bmp").c_str(), 2);
			button->Create(button->GetBitmapWidth(), button->GetBitmapHeight());
			button->Move(pos[i][0], pos[i][1]);
			button->SetId(ids[i]);
			AddChild(button);
		}
	}

	m_defPushId = 0x76;
}


int UIGuildLeaveReasonDescWnd::SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4)
{
	if (message != UM_COMMAND)
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	switch (val1) {
	case ID_OK: {
		CMode* mode = g_modeMgr.GetCurMode();
		mode->SendMsg(MM_REQ_LEAVE_GUILD, (int)m_editCtrl->GetText(), 0, 0);
		g_windowMgr.DeleteWindow(WID_GUILDLEAVEREASONDESCWND);
		break;
	}
	case ID_CANCEL:
		g_windowMgr.DeleteWindow(WID_GUILDLEAVEREASONDESCWND);
		break;
	}
	return 0;
}

UIGuildBanReasonDescWnd::UIGuildBanReasonDescWnd()
{
}

UIGuildBanReasonDescWnd::~UIGuildBanReasonDescWnd() {}

void UIGuildBanReasonDescWnd::OnCreate(int cx, int cy)
{
	UIGuildLeaveReasonDescWnd::OnCreate(cx, cy);
	if (m_titleText) {
		m_titleText->SetText(MsgStr(MSI_PELASE_INPUT_WHY_EXPEL), 0);
	}
}

int UIGuildBanReasonDescWnd::SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_SETINFO: {
		int* src = (int*)val1;
		for (int i = 0; i < 3; ++i)
			m_vars[i] = src[i];
		break;
	}
	case UM_COMMAND:
		switch (val1) {
		case ID_OK:
			g_windowMgr.DeleteWindow(WID_GUILDBANREASONDESCWND);
			g_modeMgr.GetCurMode()->SendMsg(MM_REQ_BAN_GUILD, (int)m_vars, (int)m_editCtrl->GetText(), 0);
			break;
		case ID_CANCEL:
			g_windowMgr.DeleteWindow(WID_GUILDBANREASONDESCWND);
			break;
		}
		break;
	default:
		return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}
