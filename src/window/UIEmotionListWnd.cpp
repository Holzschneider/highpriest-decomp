#include "common.h"
#include "../MsgStrings.h"
#include "UIEmotionListWnd.h"
#include "UIEmotionWnd.h"
#include "../UIRectInfo.h"
#include "../Session.h"
#include "UIFrameWnd.h"
#include "UISys.h"
#include "../base/Res.h"
#include "../pdb_enums.h"
#include "../resource/Bitmap.h"
#include "../skin.h"

extern UIWindowMgr g_windowMgr;
extern const char* UIBmp(const char* uiBmpName);
extern const char* MsgStr(MSGSTRINGID id);

void UIEmotionListWnd::OnCreate(int cx, int cy)
{
	char toolTip[6];
	char buttonName[6];
	int posX;
	int height;

	m_curIndex = -1;
	m_oldIndex = -1;

	posX = m_w - 14;
	memcpy(buttonName, "close", 6);

	std::string preBmpName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sys_";
	memcpy(toolTip, "Alt+L", 6);

	UIBitmapButton* closeButton = new UIBitmapButton;
	closeButton->SetBitmapName((preBmpName + buttonName + "_off.bmp").c_str(), BS_NORMAL);
	closeButton->SetBitmapName((preBmpName + buttonName + "_on.bmp").c_str(), BS_MOUSEON);
	closeButton->SetBitmapName((preBmpName + buttonName + "_on.bmp").c_str(), BS_PRESSED);
	closeButton->Create(closeButton->GetBitmapWidth(), closeButton->GetBitmapHeight());
	closeButton->Move(posX, 3);
	closeButton->SetId(0x87);
	closeButton->SetToolTip(toolTip);
	AddChild(closeButton);

	posX = 11;
	height = 28;
	for (int a = 0; a < 49; a++) {
		m_emotionButton[a] = new UISpriteButton;
		m_emotionButton[a]->SetSpriteName("\xc0\xcc\xc6\xd1\xc6\xae\\emotion.spr");
		m_emotionButton[a]->SetActFileName("\xc0\xcc\xc6\xd1\xc6\xae\\emotion_index3.act");
		m_emotionButton[a]->Create(40, 40);
		m_emotionButton[a]->Move(posX, height);
		m_emotionButton[a]->SetAction(a);
		m_emotionButton[a]->SetMotion(0);
		m_emotionButton[a]->SetId(0xd9);
		AddChild(m_emotionButton[a]);

		posX += 47;
		if (posX + 40 >= m_w) {
			posX = 11;
			height += 47;
		}
	}

	InitEmotionName();
}

void UIEmotionListWnd::InitEmotionName()
{
	char* newName;
	int count = 49;
	std::vector<char*>* names = &m_emotionName;

	do {
		newName = new char[32];
		memset(newName, 0, 32);
		names->push_back(newName);
	} while (--count);

	sprintf(m_emotionName[0], MsgStr(MSI_EMOTION_SURPRISE));
	sprintf(m_emotionName[1], MsgStr(MSI_EMOTION_QUESTION));
	sprintf(m_emotionName[2], MsgStr(MSI_EMOTION_DELIGHT));
	sprintf(m_emotionName[3], MsgStr(MSI_EMOTION_THROB));
	sprintf(m_emotionName[4], MsgStr(MSI_EMOTION_BIGTHROB));
	sprintf(m_emotionName[5], MsgStr(MSI_EMOTION_SWEAT));
	sprintf(m_emotionName[6], MsgStr(MSI_EMOTION_AHA));
	sprintf(m_emotionName[7], MsgStr(MSI_EMOTION_FRET));
	sprintf(m_emotionName[8], MsgStr(MSI_EMOTION_ANGER));
	sprintf(m_emotionName[9], MsgStr(MSI_EMOTION_MONEY));
	sprintf(m_emotionName[10], MsgStr(MSI_EMOTION_THINK));
	sprintf(m_emotionName[11], MsgStr(MSI_EMOTION_THANKS));
	sprintf(m_emotionName[12], MsgStr(MSI_EMOTION_KEK));
	sprintf(m_emotionName[13], MsgStr(MSI_EMOTION_SORRY));
	sprintf(m_emotionName[14], MsgStr(MSI_EMOTION_SMILE));
	sprintf(m_emotionName[15], MsgStr(MSI_EMOTION_PROFUSELY_SWEAT));
	sprintf(m_emotionName[16], MsgStr(MSI_EMOTION_SCRATCH));
	sprintf(m_emotionName[17], MsgStr(MSI_EMOTION_BEST));
	sprintf(m_emotionName[18], MsgStr(MSI_EMOTION_STARE_ABOUT));
	sprintf(m_emotionName[19], MsgStr(MSI_EMOTION_HUK));
	sprintf(m_emotionName[20], MsgStr(MSI_EMOTION_O));
	sprintf(m_emotionName[21], MsgStr(MSI_EMOTION_X));
	sprintf(m_emotionName[22], MsgStr(MSI_EMOTION_HELP));
	sprintf(m_emotionName[23], MsgStr(MSI_EMOTION_GO));
	sprintf(m_emotionName[24], MsgStr(MSI_EMOTION_CRY));
	sprintf(m_emotionName[25], MsgStr(MSI_EMOTION_KIK));
	sprintf(m_emotionName[26], MsgStr(MSI_EMOTION_CHUP));
	sprintf(m_emotionName[27], MsgStr(MSI_EMOTION_CHUPCHUP));
	sprintf(m_emotionName[28], MsgStr(MSI_EMOTION_HNG));
	sprintf(m_emotionName[29], MsgStr(MSI_EMOTION_OK));
	sprintf(m_emotionName[30], MsgStr(MSI_EMOTION_STARE));
	sprintf(m_emotionName[31], MsgStr(MSI_EMOTION_HUNGRY));
	sprintf(m_emotionName[32], MsgStr(MSI_EMOTION_COOL));
	sprintf(m_emotionName[33], MsgStr(MSI_EMOTION_MERONG));
	sprintf(m_emotionName[34], MsgStr(MSI_EMOTION_SHY));
	sprintf(m_emotionName[35], MsgStr(MSI_EMOTION_GOODBOY));
	sprintf(m_emotionName[36], MsgStr(MSI_EMOTION_SPTIME));
	sprintf(m_emotionName[37], MsgStr(MSI_EMOTION_SEXY));
	sprintf(m_emotionName[38], MsgStr(MSI_EMOTION_COMEON));
	sprintf(m_emotionName[39], MsgStr(MSI_EMOTION_SLEEPY));
	sprintf(m_emotionName[40], MsgStr(MSI_EMOTION_CONGRATULATION));
	sprintf(m_emotionName[41], MsgStr(MSI_EMOTION_HPTIME));
	sprintf(m_emotionName[42], MsgStr(MSI_EMOTION_SPARK));
	sprintf(m_emotionName[43], MsgStr(MSI_EMOTION_CONFUSE));
	sprintf(m_emotionName[44], MsgStr(MSI_EMOTION_OHNO));
	sprintf(m_emotionName[45], MsgStr(MSI_EMOTION_HUM));
	sprintf(m_emotionName[46], MsgStr(MSI_EMOTION_BLABLA));
	sprintf(m_emotionName[47], MsgStr(MSI_EMOTION_OTL));
	sprintf(m_emotionName[48], MsgStr(MSI_EMOTION_DICE));
}

void UIEmotionListWnd::OnMouseMove(int x, int y)
{
	UIFrameWnd::OnMouseMove(x, y);
	m_curIndex = (x - 11) / 47 + (m_w - 11) / 47 * ((y - 28) / 47);
	if (m_curIndex != m_oldIndex) {
		Invalidate();
		m_oldIndex = m_curIndex;
	}
}

void UIEmotionListWnd::OnLBtnDown(int x, int y)
{
	if (y < 17) {
		UIFrameWnd::OnLBtnDown(x, y);
		return;
	}
	int idx = m_curIndex;
	UIEmotionWnd* emotionWnd = g_windowMgr.GetEmotionWnd();
	if (idx >= 49 || idx < 0)
		return;
	if (!emotionWnd) {
		g_windowMgr.GetChatWnd()->m_commonChat->SetText(m_emotionName[idx]);
		g_windowMgr.GetChatWnd()->m_commonChat->Invalidate();
		return;
	}
	emotionWnd->SetUserDefineText(m_emotionName[idx], -1);
}

void UIEmotionListWnd::OnDraw()
{
	unsigned long rr;
	unsigned long gg;
	unsigned long bb;

	CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_left.bmp"), 0);
	DrawBitmap(0, 0, bmp, false);

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_right.bmp"), 0);
	DrawBitmap(m_w - bmp->m_width, 0, bmp, false);

	int loopCnt = (m_w - 24) / 12 + (((m_w - 24) % 12) ? 1 : 0);
	for (int i = 0; i < loopCnt; ++i) {
		bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_mid.bmp"), 0);
		DrawBitmap((i + 1) * 12, 0, bmp, false);
	}

	char windowName[32];
	sprintf(windowName, MsgStr(MSI_EMOTION_LIST));
	TextOutA(5, 2, windowName, 0, 0, 12, 0);

	g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
	DrawBox(0, 17, m_w, m_h - 17, RGB(rr, gg, bb));

	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	DrawBox(1, 17, m_w - 2, m_h - 17, RGB(rr, gg, bb));

	char* bmpName[3] = {
		"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysboxs_ld.bmp",
		"\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysboxs_rd.bmp"
	};

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp(bmpName[0]), false);
	DrawBitmap(0, m_h - bmp->m_height, bmp, false);

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp(bmpName[1]), false);
	DrawBitmap(m_w - bmp->m_width, m_h - bmp->m_height, bmp, false);

	if (m_curIndex >= 0 && m_curIndex < 49) {
		int y = m_curIndex / ((m_w - 11) / 47);
		int x = m_curIndex - y * ((m_w - 11) / 47);
		y = y * 47 + 28;
		x = x * 47 + 11;

		g_skinMgr.GetColorChipColor(14, 6, rr, gg, bb);
		DrawBox(x, y, 40, 40, RGB(rr, gg, bb));

		g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
		DrawBox(x + 1, y + 1, 38, 38, RGB(rr, gg, bb));
	}
}

int UIEmotionListWnd::SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_COMMAND:
		if (val1 == ID_CLOSE)
			g_windowMgr.DeleteWindow(WID_EMOTIONLISTWND);
		return 0;
	case UM_SETINFO:
		m_info = (EmotionListWndInfo*)val1;
		if (!m_info->IsValidInfo())
			m_info->MakeDefault();
		Move(m_info->x, m_info->y);
		return 0;
	}
	return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
}

void UIEmotionListWnd::OnProcess()
{
	for (int i = 0; i < 49; i++) {
		if (m_emotionButton[i]->IsAnimation())
			m_emotionButton[i]->Invalidate();
	}
}

void UIEmotionListWnd::StoreInfo()
{
	if (m_info) {
		m_info->x = m_x;
		m_info->y = m_y;
		m_info->w = m_w;
		m_info->h = m_h;
	}
}
