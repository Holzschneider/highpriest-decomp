#include "common.h"
#include "window/UIFrameWnd.h"
#include "window/UISys.h"
#include "Session.h"
#include "Mode.h"
#include "base/Res.h"
#include "resource/Bitmap.h"
#include "skin.h"
#include "device/Control.h"

extern UIWindowMgr g_windowMgr;
extern CModeMgr g_modeMgr;
extern CSession g_session;
extern const char* UIBmp(const char* uiBmpName);

void CutString(const char *msg, std::vector<std::string> &strings, int maxNumCharLine);
void CutStringWithColor(const char *msg, std::vector<std::string> &strings, int maxNumCharLine, bool readCR);

UIBarGraph::UIBarGraph() { m_percent = 0.0f; }

UIBarGraph::~UIBarGraph() {}

void UIBarGraph::SetAmount(float percent) {
	m_percent = percent;
	Invalidate();
}

void UIBarGraph::OnDraw()
{
	CBitmapRes* bg = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\gze_bg.bmp"), false);
	DrawBitmap(0, 0, bg, false);
	if (m_percent == 0.0f)
		return;

	std::string colorName;
	if (m_percent < 25.0f)
		colorName = "red";
	else
		colorName = "blue";

	int drawCount = (m_w - 8) * (int)m_percent / 100;
	CBitmapRes* left = (CBitmapRes*)g_resMgr().Get(
		UIBmp(("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\gze" + colorName + "_left.bmp").c_str()), false);
	DrawBitmap(0, 0, left, true);

	CBitmapRes* mid = (CBitmapRes*)g_resMgr().Get(
		UIBmp(("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\gze" + colorName + "_mid.bmp").c_str()), false);
	for (int i = 0; i < drawCount; i++)
		DrawBitmap(i + 4, 0, mid, true);

	CBitmapRes* right = (CBitmapRes*)g_resMgr().Get(
		UIBmp(("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\gze" + colorName + "_right.bmp").c_str()), false);
	DrawBitmap(drawCount + 4, 0, right, true);
}

UIBarGraph2::UIBarGraph2() {
	m_percent = 0;
	m_isVertical = 0;
	m_r = 42;
	m_backR = 142;
	m_backG = 142;
	m_backB = 142;
	m_g = 75;
	m_b = 143;
}

UIBarGraph2::~UIBarGraph2() {}

void UIBarGraph2::OnDraw()
{
	if (m_isVertical) {
		ClearDC(((unsigned int)(unsigned char)m_backB << 16) |
				((unsigned int)(unsigned char)m_backG << 8) |
				(unsigned char)m_backR);
		if (m_percent == 0.0f)
			return;

		DrawBox(0, m_h - (int)(((float)m_h * m_percent) / 100.0f), m_w, (int)(((float)m_h * m_percent) / 100.0f),
				((unsigned int)(unsigned char)m_b << 16) |
				((unsigned int)(unsigned char)m_g << 8) |
				(unsigned char)m_r);
		return;
	}

	DrawBox(0, 0, m_w, m_h, 0xffff00);
	DrawBox(1, 1, m_w - 2, m_h - 2,
			((unsigned int)(unsigned char)m_backB << 16) |
			((unsigned int)(unsigned char)m_backG << 8) |
			(unsigned char)m_backR);
	if (m_percent != 0.0f) {
		DrawBox(1, 1, (int)(((float)(m_w - 2) * m_percent) / 100.0f), m_h - 2,
				((unsigned int)(unsigned char)m_b << 16) |
				((unsigned int)(unsigned char)m_g << 8) |
				(unsigned char)m_r);
	}

	char tmpStr[10];
	sprintf(tmpStr, "%3d%%", (int)m_percent);
	TextOutA((m_w >> 1) - 20, (m_h >> 1) - 7, tmpStr, 0, 1, 16, 0xffff);
}

UIBarGraph3::UIBarGraph3() {
	m_pAmount = NULL;
	m_pTotalAmount = NULL;
}

UIBarGraph3::~UIBarGraph3() {}

void UIBarGraph3::SetAmount(const int* pAmount, const int* pTotalAmount) {
	m_pAmount = pAmount;
	m_pTotalAmount = pTotalAmount;
}

UIBarGraphPlayerHp::UIBarGraphPlayerHp() { m_isPlayerType = 0; }

UIBarGraphPlayerHp::~UIBarGraphPlayerHp() {}

void UIBarGraphPlayerHp::OnDraw()
{
	int hp = 0;
	int maxhp = 0;
	if (m_isPlayerType == 0) {
		hp = g_session.GetHp();
		maxhp = g_session.GetMaxhp();
	} else if (m_isPlayerType == 1) {
		hp = g_session.GetHomunHP();
		maxhp = g_session.GetHomunMaxHP();
	} else if (m_isPlayerType == 2) {
		hp = g_session.GetMerHP();
		maxhp = g_session.GetMerMaxHP();
	}

	if (maxhp < hp)
		return;

	CBitmapRes* bg = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\gze_bg.bmp"), false);
	DrawBitmap(0, 0, bg, false);
	if (maxhp == 0)
		return;

	float percent = ((float)hp * 100.0f) / ((float)maxhp);
	if (percent == 0.0f)
		return;

	std::string colorName;
	if (percent < 25.0f)
		colorName = "red";
	else
		colorName = "blue";

	int drawCount = (m_w - 8) * (int)percent / 100;
	CBitmapRes* left = (CBitmapRes*)g_resMgr().Get(
		UIBmp(("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\gze" + colorName + "_left.bmp").c_str()), false);
	DrawBitmap(0, 0, left, true);

	CBitmapRes* mid = (CBitmapRes*)g_resMgr().Get(
		UIBmp(("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\gze" + colorName + "_mid.bmp").c_str()), false);
	for (int i = 0; i < drawCount; i++)
		DrawBitmap(i + 4, 0, mid, true);

	CBitmapRes* right = (CBitmapRes*)g_resMgr().Get(
		UIBmp(("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\gze" + colorName + "_right.bmp").c_str()), false);
	DrawBitmap(drawCount + 4, 0, right, true);
}

UIBarGraphPlayerSp::UIBarGraphPlayerSp() { m_isPlayerType = 0; }

UIBarGraphPlayerSp::~UIBarGraphPlayerSp() {}

void UIBarGraphPlayerSp::OnDraw()
{
	int sp = 0;
	int maxsp = 0;
	if (m_isPlayerType == 0) {
		sp = g_session.GetSp();
		maxsp = g_session.GetMaxsp();
	} else if (m_isPlayerType == 1) {
		sp = g_session.GetHomunSP();
		maxsp = g_session.GetHomunMaxSP();
	} else if (m_isPlayerType == 2) {
		sp = g_session.GetMerSP();
		maxsp = g_session.GetMerMaxSP();
	}

	if (maxsp < sp)
		return;

	CBitmapRes* bg = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\gze_bg.bmp"), false);
	DrawBitmap(0, 0, bg, false);
	if (maxsp == 0)
		return;

	float percent = ((float)sp * 100.0f) / ((float)maxsp);
	if (percent == 0.0f)
		return;

	std::string colorName;
	if (percent < 25.0f)
		colorName = "red";
	else
		colorName = "blue";

	int drawCount = (m_w - 8) * (int)percent / 100;
	CBitmapRes* left = (CBitmapRes*)g_resMgr().Get(
		UIBmp(("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\gze" + colorName + "_left.bmp").c_str()), false);
	DrawBitmap(0, 0, left, true);

	CBitmapRes* mid = (CBitmapRes*)g_resMgr().Get(
		UIBmp(("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\gze" + colorName + "_mid.bmp").c_str()), false);
	for (int i = 0; i < drawCount; i++)
		DrawBitmap(i + 4, 0, mid, true);

	CBitmapRes* right = (CBitmapRes*)g_resMgr().Get(
		UIBmp(("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\gze" + colorName + "_right.bmp").c_str()), false);
	DrawBitmap(drawCount + 4, 0, right, true);
}

void UIBarGraph4::OnMouseHover(int x, int y)
{
	if (m_pAmount == NULL || m_pTotalAmount == NULL)
		return;
	if (*m_pTotalAmount < *m_pAmount)
		return;
	if (*m_pTotalAmount == 0)
		return;

	int gx = 0;
	int gy = -16;
	GetGlobalCoor(gx, gy);

	double percent = (double)*m_pAmount / *m_pTotalAmount * 100.0;
	int j = percent;
	char itemHelp[128];
	sprintf(itemHelp, "%d.%d %%", j, (int)((float)(percent - j) * 10.0f));
	g_modeMgr.GetCurMode()->MakeHelpBalloon(itemHelp, gx, gy, -1);
}

void UIBarGraph4::OnDraw()
{
	if (m_pAmount == NULL || m_pTotalAmount == NULL)
		return;
	if (*m_pTotalAmount < *m_pAmount)
		return;

	unsigned int rr, gg, bb;
	g_skinMgr.GetColorChipColor(0x1a, 2, rr, gg, bb);
	ClearDC(((gg & 0xff) << 16) | ((gg & 0xff) << 8) | (rr & 0xff));

	g_skinMgr.GetColorChipColor(0x1e, 2, rr, gg, bb);
	DrawBox(1, 1, m_w - 2, m_h - 2,
			((bb & 0xff) << 16) | ((gg & 0xff) << 8) | (rr & 0xff));

	if (*m_pTotalAmount == 0)
		return;

	float percent = (float)*m_pAmount / *m_pTotalAmount;
	if (percent == 0.0f)
		return;

	g_skinMgr.GetColorChipColor(2, 6, rr, gg, bb);
	if (m_bType == 0 && percent * 100.0f < 25.0f) {
		rr = 255;
		gg = 0;
		bb = 0;
	}

	DrawBox(1, 1, (int)((float)(m_w - 2) * percent), m_h - 2,
			((bb & 0xff) << 16) | ((gg & 0xff) << 8) | (rr & 0xff));
}

UIGage::UIGage() {
	m_backR = 68;
	m_backG = 68;
	m_backB = 68;
	m_backR2 = 68;
	m_backG2 = 68;
	m_backB2 = 68;
	m_edgeR = 20;
	m_edgeG = 25;
	m_edgeB = 155;
	m_r = 22;
	m_g = 234;
	m_b = 37;
	m_r2 = 29;
	m_g2 = 101;
	m_b2 = 219;
}


UIGage::~UIGage() {}

UIRechargeGage::UIRechargeGage()
{
	m_amount = 0;
	m_totalAmount = 100;
}

UIRechargeGage::~UIRechargeGage() {}

void UIRechargeGage::SetAmount(int amount, int totalAmount) {
	m_amount = amount;
	m_totalAmount = totalAmount;
	Invalidate();
}

void UIRechargeGage::OnDraw()
{
	ClearDC(((unsigned int)(unsigned char)m_edgeB << 16) |
			((unsigned int)(unsigned char)m_edgeG << 8) |
			(unsigned char)m_edgeR);
	DrawBox(1, 1, m_w - 2, m_h - 2,
			((unsigned int)(unsigned char)m_backB << 16) |
			((unsigned int)(unsigned char)m_backG << 8) |
			(unsigned char)m_backR);
	if (m_totalAmount) {
		int rate = m_amount * 100 / m_totalAmount;
		if ((float)rate != 0.0f) {
			DrawBox(1, 1, (int)((float)m_w * (float)rate / 100.0f - 2.0f), m_h - 2,
					((unsigned int)(unsigned char)m_b << 16) |
					((unsigned int)(unsigned char)m_g << 8) |
					(unsigned char)m_r);
		}
	}
}

UIPcGage::UIPcGage()
{
	m_hpAmount = 0;
	m_hpTotalAmount = 0;
	m_spAmount = 0;
	m_spTotalAmount = 0;
	m_mode = 0;
	m_dangerR = 234;
	m_dangerG = 22;
	m_dangerB = 22;
}

UIPcGage::~UIPcGage() {}

void UIPcGage::SetMode(int mode) {
	m_mode = mode;
}

void UIPcGage::SetHp(int amount, int totalAmount) {
	m_hpAmount = amount;
	m_hpTotalAmount = totalAmount;
	Invalidate();
}

void UIPcGage::SetSp(int amount, int totalAmount) {
	m_spAmount = amount;
	m_spTotalAmount = totalAmount;
	Invalidate();
}

void UIPcGage::OnDraw()
{
	float percent2;

	ClearDC(RGB(m_edgeR, m_edgeG, m_edgeB));

	if (m_mode == 0) {
		DrawBox(1, 1, m_w - 2, m_h - 2,
				RGB(m_backR, m_backG, m_backB));
		if (m_hpTotalAmount == 0)
			return;

		percent2 = m_hpAmount * 100 / m_hpTotalAmount;
		if (percent2 == 0.0f)
			return;

		if (percent2 < 25.0f) {
			DrawBox(1, 1, (int)((float)m_w * percent2 / 100.0f - 2.0f), m_h - 2,
					RGB(m_dangerR, m_dangerG, m_dangerB));
		} else {
			DrawBox(1, 1, (int)((float)m_w * percent2 / 100.0f - 2.0f), m_h - 2,
					RGB(m_r, m_g, m_b));
		}
		return;
	}

	DrawBox(1, 1, m_w - 2, (m_h - 3) / 2,
			RGB(m_backR, m_backG, m_backB));
	DrawBox(1, m_h / 2 + 1, m_w - 2, (m_h - 3) / 2,
			RGB(m_backR2, m_backG2, m_backB2));

	if (m_hpTotalAmount == 0 || m_spTotalAmount == 0)
		return;

	float percent1 = m_hpAmount * 100 / m_hpTotalAmount;
	percent2 = m_spAmount * 100 / m_spTotalAmount;

	if (percent1 != 0.0f) {
		if (percent1 < 25.0f) {
			DrawBox(1, 1, (int)((float)m_w * percent1 / 100.0f - 2.0f), (m_h - 3) / 2,
					RGB(m_dangerR, m_dangerG, m_dangerB));
		} else {
			DrawBox(1, 1, (int)((float)m_w * percent1 / 100.0f - 2.0f), (m_h - 3) / 2,
					RGB(m_r, m_g, m_b));
		}
	}

	if (percent2 != 0.0f)
		DrawBox(1, m_h / 2 + 1, (int)((float)m_w * percent2 / 100.0f - 2.0f), (m_h - 3) / 2, RGB(m_r2, m_g2, m_b2));
}

UIPlayerGage::UIPlayerGage()
{
	m_isPlayerType = 0;
	m_backB2 = 68;
	m_dangerR = 234;
	m_dangerG = 22;
	m_dangerB = 22;
}


UIPlayerGage::~UIPlayerGage() {}

void UIPlayerGage::OnDraw()
{
	int sp = 0;
	int maxsp = 0;
	int hp = 0;
	int maxhp = 0;

	if (m_isPlayerType == 0) {
		sp = g_session.GetSp();
		maxsp = g_session.GetMaxsp();
		hp = g_session.GetHp();
		maxhp = g_session.GetMaxhp();
	} else if (m_isPlayerType == 1) {
		maxsp = g_session.m_homunMaxSP;
		sp = g_session.m_homunSP;
		hp = g_session.m_homunHP;
		maxhp = g_session.m_homunMaxHP;
	} else if (m_isPlayerType == 2) {
		maxsp = g_session.m_merMaxSP;
		sp = g_session.m_merSP;
		hp = g_session.m_merHP;
		maxhp = g_session.m_merMaxHP;
	}

	if (maxsp < 1)
		maxsp = 1;
	if (maxhp < 1)
		maxhp = 1;

	float percent1 = hp * 100 / maxhp;
	float percent2 = sp * 100 / maxsp;

	ClearDC(((unsigned int)(unsigned char)m_edgeB << 16) |
			((unsigned int)(unsigned char)m_edgeG << 8) |
			(unsigned char)m_edgeR);
	DrawBox(1, 1, m_w - 2, (m_h - 3) / 2,
			((unsigned int)(unsigned char)m_backB << 16) |
			((unsigned int)(unsigned char)m_backG << 8) |
			(unsigned char)m_backR);
	DrawBox(1, m_h / 2 + 1, m_w - 2, (m_h - 3) / 2,
			((unsigned int)(unsigned char)m_backB2 << 16) |
			((unsigned int)(unsigned char)m_backG2 << 8) |
			(unsigned char)m_backR2);

	if (percent1 != 0.0f) {
		if (percent1 < 25.0f) {
			DrawBox(1, 1, (int)(((float)m_w * percent1) / 100.0f - 2.0f), (m_h - 3) / 2,
					((unsigned int)(unsigned char)m_dangerB << 16) |
					((unsigned int)(unsigned char)m_dangerG << 8) |
					(unsigned char)m_dangerR);
		} else {
			DrawBox(1, 1, (int)(((float)m_w * percent1) / 100.0f - 2.0f), (m_h - 3) / 2,
					((unsigned int)(unsigned char)m_b << 16) |
					((unsigned int)(unsigned char)m_g << 8) |
					(unsigned char)m_r);
		}
	}

	if (percent2 != 0.0f) {
		DrawBox(1, m_h / 2 + 1, (int)(((float)m_w * percent2) / 100.0f - 2.0f), (m_h - 3) / 2,
				((unsigned int)(unsigned char)m_b2 << 16) |
				((unsigned int)(unsigned char)m_g2 << 8) |
				(unsigned char)m_r2);
	}
}

UIListBox::UIListBox()
{
	m_curItem = 0;
	m_vertViewOffset = 0;
	m_horzViewOffset = 0;
	m_maxTextWidth = 0;
	m_vertScrollBar = NULL;
	m_horzScrollBar = NULL;
	m_vertScrEnabled = 0;
	m_horzScrEnabled = 0;
	m_isTransParent = 0;
	m_bR = 255;
	m_bG = 255;
	m_bB = 255;
	m_itemSpacing = 16;
}

UIListBox::~UIListBox() {}

void UIListBox::OnCreate(int cx, int cy)
{
	m_vertScrollBar = new UIScrollBar;
	m_vertScrollBar->Create(cx - 13, 0, 1, cy);
	AddChild(m_vertScrollBar);
	m_horzScrollBar = new UIScrollBar;
	m_horzScrollBar->Create(0, cy - 13, 0, cx - 13);
	AddChild(m_horzScrollBar);
}

void UIListBox::Create2(int x, int y, int cx, int cy, bool trans) {
	m_isTransParent = trans;
	UIWindow::Resize(cx, cy);
	Move(x, y);
	OnCreate(cx, cy);
	OnSize(cx, cy);
}

void UIListBox::OnLBtnDblClk(int x, int y) {
	if (m_curItem != -1)
		m_parent->SendMsg(this, UM_COMMAND, m_id, m_curItem, 0, 0);
}

void UIListBox::OnLBtnDown(int x, int y) {
	m_curItem = HitTest(x, y);
	Invalidate();
}

void UIListBox::OnWheel(int wheel) {
	if (m_vertScrEnabled) {
		if (wheel < 0)
			SendMsg(m_vertScrollBar, UM_SCROLLVERT, 3, 0, 0, 0);
		else
			SendMsg(m_vertScrollBar, UM_SCROLLVERT, -3, 0, 0, 0);
	}
}

void UIListBox::Resize(int cx, int cy) {
	UIWindow::Resize(cx, cy);
	m_vertScrollBar->Resize(12, cy);
	m_vertScrollBar->Move(cx - 13, 0);
	m_horzScrollBar->Resize(cx - 13, 12);
	m_horzScrollBar->Move(0, cy - 13);
	RecalcScrbarPos();
}

void UIListBox::OnDraw()
{
	if (m_isTransParent) {
		ClearDC(0xffff00ff);
		if (m_parent)
			m_parent->Invalidate();
	} else {
		ClearDC(RGB(m_bR, m_bG, m_bB));
	}
	int firstItem = m_vertViewOffset;
	int lastItem = min(m_h / m_itemSpacing - m_horzScrEnabled + firstItem,
					   m_items.size());
	unsigned int colorRef = 0;
	for (int i = firstItem; i < lastItem; i++) {
		if (i == m_curItem)
			DrawBox(0, (i - firstItem) * m_itemSpacing, m_w, m_itemSpacing - 1, 0xfbdfca);
		int x = 3 - m_horzViewOffset;
		int y = (i - firstItem) * m_itemSpacing;
		TextOutWithDecoration(x, y, m_items[i].c_str(), m_items[i].size(), &colorRef, 0, 12);
	}
}

INT_PTR UIListBox::SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch (message) {
	case 40:
		m_curItem = val1;
		if (m_curItem < 0)
			m_curItem = 0;
		if ((unsigned int)m_curItem > m_items.size() - 1)
			m_curItem = m_items.size() - 1;
		if (m_curItem < m_vertViewOffset)
			SendMsg(NULL, UM_SCROLLVERT, -1, 0, 0, 0);
		if (m_curItem > m_vertViewOffset + m_h / m_itemSpacing - 1)
			SendMsg(NULL, UM_SCROLLVERT, 1, 0, 0, 0);
		Invalidate();
		RecalcScrbarPos();
		break;

	case 18:
		--m_curItem;
		if (m_curItem < 0)
			m_curItem = 0;
		if (m_curItem < m_vertViewOffset)
			SendMsg(NULL, UM_SCROLLVERT, -1, 0, 0, 0);
		Invalidate();
		RecalcScrbarPos();
		break;

	case 19:
		++m_curItem;
		if ((unsigned int)m_curItem > m_items.size() - 1)
			m_curItem = m_items.size() - 1;
		if (m_curItem > m_vertViewOffset + m_h / m_itemSpacing - 1)
			SendMsg(NULL, UM_SCROLLVERT, 1, 0, 0, 0);
		Invalidate();
		RecalcScrbarPos();
		break;

	case 7:
	case 9:
	case 10: {
		int oldOffset = m_vertViewOffset;
		if (message == 9)
			m_vertViewOffset = oldOffset - m_h / m_itemSpacing + 1;
		else if (message == 10)
			m_vertViewOffset = oldOffset + m_h / m_itemSpacing - 1;
		else
			m_vertViewOffset = oldOffset + val1;
		if (m_vertViewOffset < 0)
			m_vertViewOffset = 0;
		if ((unsigned int)m_vertViewOffset > (unsigned int)(m_items.size() + m_horzScrEnabled - m_h / m_itemSpacing))
			m_vertViewOffset = m_horzScrEnabled - m_h / m_itemSpacing + m_items.size();
		m_vertScrollBar->SetPos(m_vertViewOffset);
		m_vertScrollBar->Invalidate();
		Invalidate();
		if (m_isTransParent)
			if (m_parent)
				m_parent->Invalidate();
		return m_vertViewOffset - oldOffset;
	}

	case 8:
	case 11:
	case 12: {
		int oldOffset = m_horzViewOffset;
		if (message == 11)
			m_horzViewOffset = oldOffset - 15;
		else if (message == 12)
			m_horzViewOffset = oldOffset + 15;
		else
			m_horzViewOffset = oldOffset + val1;
		if (m_horzViewOffset < 0)
			m_horzViewOffset = 0;
		if (m_horzViewOffset > m_maxTextWidth - m_w + m_vertScrEnabled * 16)
			m_horzViewOffset = m_maxTextWidth - m_w + m_vertScrEnabled * 16;
		m_horzScrollBar->SetPos(m_horzViewOffset);
		m_horzScrollBar->Invalidate();
		Invalidate();
		if (m_isTransParent)
			if (m_parent)
				m_parent->Invalidate();
		return m_horzViewOffset - oldOffset;
	}

	default:
		return UIWindow::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

void UIListBox::RecalcScrbarPos()
{
	int numCanDisplayItem = m_h / m_itemSpacing;
	int maxPos = m_items.size() - numCanDisplayItem;
	if (maxPos < 0)
		maxPos = 0;
	if (maxPos)
		m_vertScrEnabled = 1;
	else
		m_vertScrEnabled = 0;

	if (m_vertScrEnabled)
		m_vertScrollBar->SetRange(m_horzScrEnabled + maxPos, numCanDisplayItem, m_items.size());
	else
		m_vertScrollBar->SetRange(0, numCanDisplayItem, m_items.size());
	m_vertScrollBar->Invalidate();
}

void UIListBox::AddItem(char const *txt)
{
	m_items.push_back(std::string(txt));
	m_maxTextWidth = max(m_maxTextWidth, GetTextWidth(txt, 0, 0, 12, false));
	RecalcScrbarPos();
}

void UIListBox::ClearAllItems()
{
	m_curItem = -1;
	m_maxTextWidth = 0;
	m_horzViewOffset = 0;
	m_vertViewOffset = 0;
	m_vertScrEnabled = 0;
	m_horzScrEnabled = 0;
	m_items.clear();
}

int UIListBox::HitTest(int x, int y) {
	if (x < 0)
		return -1;
	if (y < 0)
		return -1;
	if (x >= m_w)
		return -1;
	if (y >= m_h)
		return -1;

	int item = y / m_itemSpacing + m_vertViewOffset;
	if (item < 0 || (unsigned int)item >= m_items.size())
		item = -1;
	return item;
}

UIComboBox::UIComboBox()
{
	m_callerWid = WID_LAST;
}

UIComboBox::~UIComboBox() {}

void UIComboBox::OnCreate(int cx, int cy) {
	UIListBox::OnCreate(cx, cy);
}

void UIComboBox::Create2(int x, int y, int cx, int cy, bool trans) {
	UIListBox::Create2(x, y, cx, cy, trans);
}

void UIComboBox::RecalcScrbarPos()
{
	int numCanDisplayItem = m_h / m_itemSpacing;
	int vertRange = m_items.size() - numCanDisplayItem;

	if (vertRange < 0)
		vertRange = 0;
	if (vertRange != 0)
		m_vertScrEnabled = 1;
	else
		m_vertScrEnabled = 0;

	if (m_vertScrEnabled) {
		m_vertScrollBar->SetRange(m_horzScrEnabled + vertRange, numCanDisplayItem, m_items.size());
		if (m_curItem >= m_vertViewOffset) {
			if (m_vertViewOffset + numCanDisplayItem <= m_curItem)
				m_vertViewOffset = m_curItem - numCanDisplayItem + 1;
		} else {
			m_vertViewOffset = m_curItem;
		}
		if (m_vertViewOffset < 0)
			m_vertViewOffset = 0;
		if (m_vertViewOffset > vertRange)
			m_vertViewOffset = vertRange;
		m_vertScrollBar->SetPos(m_vertViewOffset);
	} else {
		m_vertScrollBar->SetRange(0, numCanDisplayItem, m_items.size());
	}
	m_vertScrollBar->Invalidate();
}

void UIComboBox::OnLBtnUp(int x, int y) {
	m_curItem = HitTest(x, y);
	UIWindow* caller = g_windowMgr.QueryWindow(m_callerWid);
	if (caller != NULL)
		caller->SendMsg(NULL, UM_COMBOBOX_SELECTED, m_curItem, 0, 0, 0);
	g_windowMgr.DeleteWindow(WID_COMBOBOXWND);
}

void UIComboBox::OnMouseMove(int x, int y) {
	int prev = m_curItem;
	m_curItem = HitTest(x, y);
	if (prev != m_curItem)
		Invalidate();
}

void UIComboBox::OnLBtnDown(int x, int y) {}

void UIComboBox::OnLBtnDblClk(int x, int y) {}


void UIComboBox::Resize(int cx, int cy)
{
	UIWindow::Resize(cx, cy);
	m_vertScrollBar->Resize(12, cy);
	m_vertScrollBar->Move(cx - 13, 0);
	m_horzScrollBar->Resize(cx - 13, 12);
	m_horzScrollBar->Move(0, cy - 13);
	RecalcScrbarPos();
	if (m_vertScrEnabled) {
		m_vertViewOffset = 0;
		m_vertScrollBar->SetPos(0);
		m_vertScrollBar->Move(cx - 13, 0);
		m_vertScrollBar->Invalidate();
	} else {
		m_vertViewOffset = 0;
		m_vertScrollBar->SetPos(0);
		m_vertScrollBar->Move(cx - 13, 0);
	}
}

void UIComboBox::OnDraw()
{
	unsigned int rr;
	unsigned int gg;
	unsigned int bb;

	g_skinMgr.GetColorChipColor(6, 6, rr, gg, bb);
	ClearDC(((bb & 0xff) << 16) | ((gg & 0xff) << 8) | (rr & 0xff));

	g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
	DrawBox(1, 1, m_w - 2, m_h - 2,
			((bb & 0xff) << 16) | ((gg & 0xff) << 8) | (rr & 0xff));

	CBitmapRes* leftTop = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sysboxs_lu.bmp"), false);
	DrawBitmap(0, 0, leftTop, false);

	CBitmapRes* leftBottom = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sysboxs_ld.bmp"), false);
	DrawBitmap(0, m_h - leftBottom->m_height, leftBottom, false);

	CBitmapRes* rightTop = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sysboxs_ru.bmp"), false);
	DrawBitmap(m_w - rightTop->m_width, 0, rightTop, false);

	CBitmapRes* rightBottom = (CBitmapRes*)g_resMgr().Get(
		UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sysboxs_rd.bmp"), false);
	DrawBitmap(m_w - rightBottom->m_width, m_h - rightBottom->m_height, rightBottom, false);

	int firstItem = m_vertViewOffset;
	int lastItem = min(m_h / m_itemSpacing - m_horzScrEnabled + firstItem,
					   m_items.size());
	for (int i = firstItem; i < lastItem; i++) {
		if (i == m_curItem)
		{
			g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
			DrawBox(3, (i - firstItem) * m_itemSpacing + 1, m_w - 6, m_itemSpacing - 2, ((bb & 0xff) << 16) | ((gg & 0xff) << 8) | (rr & 0xff));
		}
		TextOutA(3 - m_horzViewOffset, (i - firstItem) * m_itemSpacing + 2, m_items[i].c_str(), 0, 0, 12, 0);
	}

	if (m_vertScrEnabled) {
		g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
		DrawBox(m_w - 16, 0, 16, m_h, ((bb & 0xff) << 16) | ((gg & 0xff) << 8) | (rr & 0xff));
	}
}

INT_PTR UIComboBox::SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch (message) {
	case 0:
		{
			UIWindow* caller = g_windowMgr.QueryWindow(m_callerWid);
			if (caller)
				caller->SendMsg(NULL, UM_COMBOBOX_SELECTED, m_curItem, 0, 0, 0);
			if (g_windowMgr.GetComboBoxWnd())
				g_windowMgr.DeleteWindow(WID_COMBOBOXWND);
		}
		break;

	case 18:
		if (!m_items.size())
			return 0;
		m_curItem--;
		if (m_curItem < 0)
			m_curItem = 0;
		if (m_curItem < m_vertViewOffset)
			m_vertViewOffset = m_curItem;
		RecalcScrbarPos();
		Invalidate();
		break;
	case 19:
		if (m_items.size() == 0)
			return 0;
		m_curItem++;
		if (m_curItem >= m_items.size())
			m_curItem = m_items.size() - 1;
		{
			int numCanDisplayItem = m_h / m_itemSpacing - m_horzScrEnabled;
			if (m_vertViewOffset <= m_curItem - numCanDisplayItem)
				m_vertViewOffset = m_curItem - numCanDisplayItem + 1;
		}
		RecalcScrbarPos();
		Invalidate();
		break;
	case 31:
		AddItem((const char*)val1);
		Invalidate();
		break;

	case 83:
		m_callerWid = (WINDOWID)val1;
		break;

	case 40:
		m_curItem = val1;
		if (m_curItem < 0)
			m_curItem = 0;
		if (m_curItem > m_items.size() - 1)
			m_curItem = m_items.size() - 1;
		RecalcScrbarPos();
		Invalidate();
		break;

	default:
		return UIListBox::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

UIChatHisBox::UIChatHisBox()
{
	m_maxItem = 240;
	m_itemSpacing = 14;
	m_items.reserve(240);
	m_itemColors.reserve(240);
	m_talkerNameList.reserve(240);
}

UIChatHisBox::~UIChatHisBox()
{}

void UIChatHisBox::OnCreate(int cx, int cy)
{
	m_vertScrollBar = new UIScrollBar2;
	m_vertScrollBar->Create(cx - 13, 0, 1, cy);
	AddChild(m_vertScrollBar);
	m_horzScrollBar = new UIScrollBar2;
	m_horzScrollBar->Create(0, cy - 13, 0, cx - 13);
	AddChild(m_horzScrollBar);
}

void UIChatHisBox::Create2(int x, int y, int cx, int cy, bool trans)
{
	UIListBox::Create2(x, y, cx, cy, true);
}

void UIChatHisBox::Resize(int cx, int cy)
{
	UIListBox::Resize(cx, cy);
	if (m_vertScrEnabled) {
		m_vertViewOffset = m_items.size() + m_horzScrEnabled - m_h / m_itemSpacing;
		m_vertScrollBar->SetPos(m_vertViewOffset);
		m_vertScrollBar->Move(cx - 13, 0);
		m_vertScrollBar->Invalidate();
	} else {
		m_vertViewOffset = 0;
		m_vertScrollBar->SetPos(0);
		m_vertScrollBar->Move(cx - 13, 0);
	}
}

void UIChatHisBox::SetShow(int visible) {
	m_show = visible;
	if (visible) Invalidate();
}

void UIChatHisBox::DeleteFirstHalfItem()
{
	int half = m_items.size() / 2;
	int i;
	for (i = 0; i < half; i++)
		m_items[i] = m_items[i + half];

	m_items.resize(half);

	half = m_itemColors.size() / 2;
	for (i = 0; i < half; i++)
		m_itemColors[i] = m_itemColors[i + half];

	m_itemColors.resize(half);

	half = m_talkerNameList.size() / 2;
	for (i = 0; i < half; i++)
		m_talkerNameList[i] = m_talkerNameList[i + half];

	m_talkerNameList.resize(half);

	RecalcScrbarPos();
}

void UIChatHisBox::ClearAllItems()
{
	UIListBox::ClearAllItems();
	m_itemColors.erase(m_itemColors.begin(), m_itemColors.end());
	m_talkerNameList.erase(m_talkerNameList.begin(), m_talkerNameList.end());
}

void UIChatHisBox::AddItem(char const *txt, unsigned int itemRGB, char const *name)
{
	bool isBottomLine = false;
	if (m_vertScrEnabled) {
		if (m_vertViewOffset == m_items.size() + m_horzScrEnabled - m_h / m_itemSpacing)
			isBottomLine = true;
	} else
		isBottomLine = true;

	if (m_items.size() >= (unsigned int)m_maxItem)
		DeleteFirstHalfItem();

	if (name)
		m_talkerNameList.push_back(name);
	else
		m_talkerNameList.push_back("");
	m_itemColors.push_back(itemRGB);
	UIListBox::AddItem(txt);
	if (m_vertScrEnabled) {
		if (isBottomLine) {
			m_vertViewOffset = m_items.size() + m_horzScrEnabled - m_h / m_itemSpacing;
			m_vertScrollBar->SetPos(m_vertViewOffset);
			m_vertScrollBar->Invalidate();
		}
	}
	Invalidate();
}

void UIChatHisBox::AddItem2(char const *txt, unsigned int itemRGB, char const *name)
{
	std::vector<std::string> strings;
	int width = m_w;
	if (m_vertScrollBar)
		width -= 16;
	PolishText4(txt, strings, width);

	for (int i = 0; i < strings.size(); i++) {
		int len = strlen(strings[i].c_str());
		char* line = new char[len + 1];
		strcpy(line, strings[i].c_str());
		line[len] = 0;
		AddItem(line, itemRGB, name);
#if defined(BF_PORT)
		delete[] line;	// the copy was never freed; one per line drawn into the chat (M8.6 soak, leaks)
#endif
	}
}

const char* UIChatHisBox::GetTalkerName(int item) {
	if (m_talkerNameList[item].length() > 0)
		return m_talkerNameList[item].c_str();
	return NULL;
}

void UIChatHisBox::OnLBtnUp(int x, int y) {}

void UIChatHisBox::OnLBtnDblClk(int x, int y) {
	if (m_curItem != -1) {
		if (GetTalkerName(m_curItem) != NULL) {
			m_parent->SendMsg(this, UM_COMMAND, 0x119, (INT_PTR)GetTalkerName(m_curItem), 0, 0);
		}
	}
}

void UIChatHisBox::OnLBtnDown(int x, int y) {
	m_curItem = HitTest(x, y);
}

void UIChatHisBox::OnDraw()
{
	ClearDC(0xffff00ff);
	if (m_parent && m_x >= 0 && m_y >= 0)
		m_parent->OnDraw2();

	int firstItem = m_vertViewOffset;
	int lastItem = min(m_h / m_itemSpacing - m_horzScrEnabled + m_vertViewOffset,
					   m_items.size());

	for (int i = firstItem; i < lastItem; i++) {
		TextOutA(3, (i - firstItem) * m_itemSpacing + 4, m_items[i].c_str(), 0, 0, 12, 0);
		TextOutA(2, (i - firstItem) * m_itemSpacing + 3, m_items[i].c_str(), 0, 0, 12, m_itemColors[i]);
	}
}

int UIChatHisBox::WriteChatToFile(char const *fileName)
{
	FILE* fp = fopen(fileName, "w");
	if (fp == NULL)
		return 0;

	char nextLine = '\n';
	int itemCount = m_items.size();
	for (int i = 0; i < itemCount; i++) {
		fwrite(m_items[i].c_str(), m_items[i].size(), 1, fp);
		fwrite(&nextLine, 1, 1, fp);
	}
	fclose(fp);
	return 1;
}

INT_PTR UIChatHisBox::SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4) {
	return UIListBox::SendMsg(sender, message, val1, val2, val3, val4);
}

int UIChatHisBox::WriteChatToReportFile(const char* fileName) {
	return 1;
}

UIChatHisBox2::UIChatHisBox2()
{
	m_r = 255;
	m_g = 255;
	m_b = 255;
}

UIChatHisBox2::~UIChatHisBox2()
{}


void UIChatHisBox2::Create2(int x, int y, int cx, int cy, bool trans)
{
	UIListBox::Create2(x, y, cx, cy, false);
}

void UIChatHisBox2::OnCreate(int cx, int cy)
{
	m_vertScrollBar = new UIScrollBar;
	m_vertScrollBar->Create(cx - 13, 0, 1, cy);
	AddChild(m_vertScrollBar);
	m_horzScrollBar = new UIScrollBar2;
	m_horzScrollBar->Create(0, cy - 13, 0, cx - 13);
	AddChild(m_horzScrollBar);
}

void UIChatHisBox2::OnRBtnUp(int x, int y) {
	m_parent->OnRBtnUp(m_x + x, m_y + y);
}

void UIChatHisBox2::OnDraw()
{
	ClearDC(RGB(m_r, m_g, m_b));

	int firstItem = m_vertViewOffset;
	int lastItem = min(m_h / m_itemSpacing - m_horzScrEnabled + firstItem,
					   m_items.size());

	for (int i = firstItem; i < lastItem; i++) {
		TextOutA(2, (i - firstItem) * m_itemSpacing + 3, m_items[i].c_str(),
				 0, 0, 12, m_itemColors[i]);
	}
}

UIMemberListBox::UIMemberListBox()
{}

UIMemberListBox::~UIMemberListBox()
{}

void UIMemberListBox::OnRBtnUp(int x, int y) {
	if (m_parent != NULL)
		m_parent->SendMsg(NULL, UM_MAKEMENU, 0, 0, 0, 0);
}

void UIMemberListBox::OnLBtnDown(int x, int y) {
	UIListBox::OnLBtnDown(x, y);
}

void UIMemberListBox::OnRBtnDown(int x, int y) {
	UIListBox::OnLBtnDown(x, y);
}

void UIMemberListBox::OnMouseHover(int x, int y)
{
	int firstItem = m_vertViewOffset;
	int lastItem = min(m_h / m_itemSpacing - m_horzScrEnabled + firstItem, m_items.size());

	int item = y / m_itemSpacing + firstItem;

	if (item < firstItem || item >= lastItem)
		return;

	if (GetTextWidth(m_items[item].c_str(), 0, 0, 12, false) > m_w - m_vertScrEnabled * 15) {
		int gx = -4, gy = y / m_itemSpacing * m_itemSpacing;
		GetGlobalCoor(gx, gy);
		CMode* mode = g_modeMgr.GetCurMode();
		mode->MakeHelpBalloon(m_items[item].c_str(), gx, gy, -1);
	}
}

void UIMemberListBox::OnDraw()
{
	ClearDC(RGB(m_r, m_g, m_b));

	int firstItem = m_vertViewOffset;
	int lastItem = min(m_h / m_itemSpacing - m_horzScrEnabled + firstItem,
					   m_items.size());
	for (int i = firstItem; i < lastItem; i++) {
		if (i == m_curItem)
			DrawBox(0, (i - firstItem) * m_itemSpacing + 1, m_w, m_itemSpacing, 0xc8c832);
		TextOutA(2, (i - firstItem) * m_itemSpacing + 3, m_items[i].c_str(), 0, 0, 12, m_itemColors[i]);
	}
}

UITextViewer::UITextViewer()
{
	DWORD rr, gg, bb;

	m_itemSpacing = 18;
	g_skinMgr.GetColorChipColor(0x12, 2, rr, gg, bb);
	m_bgR = rr;
	m_bgG = gg;
	m_bgB = bb;
	m_noColor = 0;
}


UITextViewer::~UITextViewer() {}

void UITextViewer::OnCreate(int cx, int cy) {
	UIListBox::OnCreate(cx, cy);
}

void UITextViewer::Create2(int x, int y, int cx, int cy, bool trans) {
	UIListBox::Create2(x, y, cx, cy, trans);
}

void UITextViewer::OnDraw()
{
	if (m_isTransParent) {
		ClearDC(0xffff00ff);
		if (m_parent)
			m_parent->Invalidate();
	} else {
		ClearDC(RGB(m_bgR, m_bgG, m_bgB));
	}

	int firstItem = m_vertViewOffset;
	int lineSpacing = m_itemSpacing - m_horzScrEnabled;
	int d;
	if (lineSpacing == 0)
		d = 1;
	else
		d = lineSpacing;
	int lastItem = min(m_h / d + firstItem, m_items.size());
	unsigned int colorRef = 0;
	int i;
	for (i = 0; i < firstItem; i++) {
		const char* text = m_items[i].c_str();
		const char* textEnd = text + m_items[i].size();
		while (text < textEnd) text = InterpretColor(FindColorMark(text, textEnd), &colorRef);
	}

	if (!m_noColor) {
		for (i = firstItem; i < lastItem; i++)
			TextOutWithDecoration(3, (i - firstItem) * m_itemSpacing + 4, m_items[i].c_str(), m_items[i].size(), &colorRef, 0, 12);
	} else {
		for (i = firstItem; i < lastItem; i++)
			TextOutWithOutline(3, (i - firstItem) * m_itemSpacing + 4, m_items[i].c_str(), m_items[i].size(), 0, 0xffffff, 0, 12, false);
	}
}

INT_PTR UITextViewer::SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch (message) {
	case 18:
		if (m_vertScrEnabled)
			SendMsg(m_vertScrollBar, UM_SCROLLVERT, -1, 0, 0, 0);
		break;

	case 19:
		if (m_vertScrEnabled)
			SendMsg(m_vertScrollBar, UM_SCROLLVERT, 1, 0, 0, 0);
		break;

	case 29:
		{
			int maxNumCharLine = 35;
			if (val2)
				maxNumCharLine = val2;
			int i;
			if (val2 == 0x8a) {
				std::vector<std::string> strings;
				CutString((const char*)val1, strings, 0x26);
				for (i = 0; i < strings.size(); i++)
					AddItem(strings[i].c_str());
				m_noColor = 1;
			} else {
				std::vector<std::string> strings;
				CutStringWithColor((const char*)val1, strings, maxNumCharLine, val3 != 0);
				for (i = 0; i < strings.size(); i++)
					AddItem(strings[i].c_str());
				m_noColor = 0;
			}
			Invalidate();
		}
		break;

	default:
		return UIListBox::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

UIServerListBox::UIServerListBox()
{
	m_serverBalloon = NULL;
}

UIServerListBox::~UIServerListBox()
{}

void UIServerListBox::OnMouseHover(int x, int y)
{
	int index = HitTest(x, y);

	int p_y = g_mouse.GetYPos();

	if (m_strings.size() != 0) {
		if (index != -1 && m_strings[index].size() != 0) {
			int x = 0, y = 0;
			if (m_serverBalloon) {
				g_windowMgr.PostQuit(m_serverBalloon);
				m_serverBalloon = NULL;
			}
			m_serverBalloon = new UIColorTransBalloonText;
			m_serverBalloon->Create(8, 8);
			GetGlobalCoor(x, y);
			m_serverBalloon->Move(x + 80, p_y - 6);
			g_windowMgr.AddWindow(m_serverBalloon);
			m_serverBalloon->SetText(m_strings[index].c_str(), 50);
		} else {
			g_windowMgr.PostQuit(m_serverBalloon);
			m_serverBalloon = NULL;
		}
	}
}

UIScrollBar::UIScrollBar() {
	m_scrollBtnSize = 13;
	m_maxPos = 0;
	m_curPos = 0;
	m_isVert = 1;
	m_drawMode = 0;
	m_startDragX = 0;
	m_startDragY = 0;
	m_deltaDrag = 0;
	m_parentNumItem = 0;
	m_parentnumCanDisplayItem = 0;
	m_fixedLength = 13;
	m_type = 1;
}

UIScrollBar::~UIScrollBar() {}

bool UIScrollBar::IsTransmitMouseInput() {
	if (m_maxPos == 0 && m_parent != NULL)
		return m_parent->IsTransmitMouseInput();
	return false;
}

void UIScrollBar::Create(int x, int y, int type, int size)
{
	m_isVert = type;

	int cx, cy;

	switch (type) {
	case 0:
		cx = size;
		cy = m_fixedLength;
		break;

	case 1:
		cx = m_fixedLength;
		cy = size;
		break;
	}
	UIWindow::Resize(cx, cy);
	Move(x, y);
	OnCreate(cx, cy);
	OnSize(cx, cy);
}
void UIScrollBar::OnMouseShape(int x, int y) {
	if (m_maxPos == 0) {
		UIWindow::OnMouseShape(x, y);
		return;
	}
	g_modeMgr.GetCurMode()->SetCursorAction(2);
}

void UIScrollBar::OnWheel(int wheel) {
	m_parent->OnWheel(wheel);
}

void UIScrollBar::OnLBtnDblClk(int x, int y) {
	OnLBtnDown(x, y);
	if (m_maxPos == 0)
		m_parent->OnLBtnDblClk(m_x + x, m_y + y);
}

void UIScrollBar::OnLBtnDown(int x, int y)
{
	if (m_maxPos == 0) {
		m_parent->OnLBtnDown(m_x + x, m_y + y);
		return;
	}

	int hit = HitTest(x, y);
	m_startDragX = x;
	m_startDragY = y;
	m_deltaDrag = 0;

	switch (m_isVert) {
	case 0:
		switch (hit) {
		case 0:
			m_parent->SendMsg(this, UM_SCROLLHORZ, -1, 0, 0, 0);
			return;
		case 1:
			m_parent->SendMsg(this, UM_SCROLLHORZPAGEUP, 0, 0, 0, 0);
			return;
		case 2:
			g_windowMgr.SetCapture(this);
			return;
		case 3:
			m_parent->SendMsg(this, UM_SCROLLHORZPAGEDOWN, 0, 0, 0, 0);
			return;
		case 4:
			m_parent->SendMsg(this, UM_SCROLLHORZ, 1, 0, 0, 0);
			return;
		}
		break;
	case 1:
		switch (hit) {
		case 0:
			m_parent->SendMsg(this, UM_SCROLLVERT, -1, 0, 0, 0);
			return;
		case 1:
			m_parent->SendMsg(this, UM_SCROLLVERTPAGEUP, 0, 0, 0, 0);
			return;
		case 2:
			g_windowMgr.SetCapture(this);
			return;
		case 3:
			m_parent->SendMsg(this, UM_SCROLLVERTPAGEDOWN, 0, 0, 0, 0);
			return;
		case 4:
			m_parent->SendMsg(this, UM_SCROLLVERT, 1, 0, 0, 0);
			return;
		}
		break;
	}
}

void UIScrollBar::Resize(int cx, int cy) {
	Invalidate();
	switch (m_isVert) {
	case 0:
		cy = m_fixedLength;
		UIWindow::Resize(cx, cy);
		OnSize(cx, cy);
		break;
	case 1:
		cx = m_fixedLength;
		break;
	}
	UIWindow::Resize(cx, cy);
	OnSize(cx, cy);
}

void UIScrollBar::OnMouseMove(int x, int y)
{
	if (m_maxPos == 0) {
		m_parent->OnMouseMove(m_x + x, m_y + y);
		return;
	}

	if (GetScrollRange() == 0)
		return;

	int oldDrawMode = m_drawMode;
	int hit = HitTest(x, y);

	switch (hit) {
	case -1:
	case 1:
	case 3:
		m_drawMode = 0;
		break;
	case 0:
		m_drawMode = 1;
		break;
	case 2:
		m_drawMode = 3;
		break;
	case 4:
		m_drawMode = 2;
		break;
	}

	if (oldDrawMode != m_drawMode)
		Invalidate();

	if (g_windowMgr.GetCapture() != this)
		return;
	if (m_parent == NULL)
		return;

	switch (m_isVert) {
	case 0: {
		int length = m_w;
		int range = length - (m_scrollBtnSize * 2 + GetThumbSize());
		int delta = (x - m_startDragX) * (m_maxPos + 1) / range - m_deltaDrag;
		if (delta != 0) {
			int ret = m_parent->SendMsg(this, UM_SCROLLHORZ, delta, 0, 0, 0);
			m_deltaDrag += ret;
		}
		break;
	}
	case 1: {
		int length = m_h;
		int range = length - (m_scrollBtnSize * 2 + GetThumbSize());
		int delta = (y - m_startDragY) * (m_maxPos + 1) / range - m_deltaDrag;
		if (delta != 0) {
			m_deltaDrag += m_parent->SendMsg(this, UM_SCROLLVERT, delta, 0, 0, 0);
		}
		break;
	}
	}
}

void UIScrollBar::OnRBtnUp(int x, int y) {
	if (m_maxPos == 0)
		m_parent->OnRBtnUp(m_x + x, m_y + y);
}

void UIScrollBar::OnLBtnUp(int x, int y) {
	if (m_maxPos == 0) {
		m_parent->OnLBtnUp(m_x + x, m_y + y);
		return;
	}
	if (g_windowMgr.GetCapture() == this)
		g_windowMgr.ReleaseCapture();
}

void UIScrollBar::OnDraw()
{
	ClearDC(0xffff00ff);
	if (m_maxPos == 0)
		return;

	switch (m_isVert) {
	case 0:
		if (m_drawMode == 1)
			DrawBitmap(1, 0, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll1left.bmp"), false), false);
		else if (m_drawMode == 4)
			DrawBitmap(1, 0, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll1left.bmp"), false), false);
		else
			DrawBitmap(1, 0, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll1left.bmp"), false), false);
		{
			CBitmapRes* midBmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll1mid.bmp"), false);
			int repeat = (m_w - m_scrollBtnSize - 14) / 13 + ((m_w - m_scrollBtnSize - 14) % 13 != 0);
			for (int i = 0; i < repeat; i++)
				DrawBitmap(m_scrollBtnSize + i * 13, 0, midBmp, false);
			DrawBitmap(m_w - m_scrollBtnSize - 13, 0, midBmp, false);
		}
		DrawBox(m_w - m_scrollBtnSize, 0, 13, m_h, 0xffff00ff);
		if (m_drawMode == 2)
			DrawBitmap(m_w - m_scrollBtnSize - 1, 0, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll1right.bmp"), false), true);
		else if (m_drawMode == 5)
			DrawBitmap(m_w - m_scrollBtnSize - 1, 0, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll1right.bmp"), false), true);
		else
			DrawBitmap(m_w - m_scrollBtnSize - 1, 0, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll1right.bmp"), false), true);
		{
			int thumbPos = m_scrollBtnSize + GetThumbOffset();
			DrawBitmap(thumbPos, 0, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll1bar_left.bmp"), false), true);
			int thumbSize = GetThumbSize();
			for (int p = thumbPos + 4; p < thumbPos + thumbSize - 4; p += 4)
				DrawBitmap(p, 0, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll1bar_mid.bmp"), false), true);
			DrawBitmap(thumbPos + thumbSize - 4, 0, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll1bar_right.bmp"), false), true);
		}
		break;

	case 1:
		if (m_drawMode == 1)
			DrawBitmap(0, 1, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll0up.bmp"), false), false);
		else if (m_drawMode == 4)
			DrawBitmap(0, 1, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll0up.bmp"), false), false);
		else
			DrawBitmap(0, 1, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll0up.bmp"), false), false);
		{
			CBitmapRes* midBmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll0mid.bmp"), false);
			int repeat = (m_h - m_scrollBtnSize - 14) / 13 + ((m_h - m_scrollBtnSize - 14) % 13 != 0);
			for (int i = 0; i < repeat; i++)
				DrawBitmap(0, m_scrollBtnSize + i * 13, midBmp, false);
			DrawBitmap(0, m_h - m_scrollBtnSize - 13, midBmp, false);
		}
		DrawBox(0, m_h - m_scrollBtnSize, m_w, 13, 0xffff00ff);
		if (m_drawMode == 2)
			DrawBitmap(0, m_h - m_scrollBtnSize - 1, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll0down.bmp"), false), true);
		else if (m_drawMode == 5)
			DrawBitmap(0, m_h - m_scrollBtnSize - 1, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll0down.bmp"), false), true);
		else
			DrawBitmap(0, m_h - m_scrollBtnSize - 1, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll0down.bmp"), false), true);
		{
			int thumbPos = m_scrollBtnSize + GetThumbOffset();
			DrawBitmap(0, thumbPos, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll0bar_up.bmp"), false), true);
			int thumbSize = GetThumbSize();
			for (int p = thumbPos + 4; p < thumbPos + thumbSize - 4; p += 4)
				DrawBitmap(0, p, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll0bar_mid.bmp"), false), true);
			DrawBitmap(0, thumbPos + thumbSize - 4, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\scroll0bar_down.bmp"), false), true);
		}
		break;
	}
}

void UIScrollBar::SetPos(int pos) {
	m_curPos = pos;
}

void UIScrollBar::SetRange(int range, int parentnumCanDisplayItem, int parentNumItem) {
	m_maxPos = range;
	m_parentnumCanDisplayItem = parentnumCanDisplayItem;
	m_parentNumItem = parentNumItem;
	Invalidate();
	if (m_curPos > m_maxPos)
		m_curPos = m_maxPos;
}

int UIScrollBar::HitTest(int x, int y)
{
	if (m_maxPos == 0)
		return -1;
	if (x < 0)
		return -1;
	if (x >= m_w)
		return -1;
	if (y < 0)
		return -1;
	if (y >= m_h)
		return -1;

	switch (m_isVert) {
	case 0:
		if (x < m_scrollBtnSize)
			return 0;
		if (x < m_scrollBtnSize + GetThumbOffset())
			return 1;
		if (x < m_scrollBtnSize + GetThumbOffset() + GetThumbSize())
			return 2;
		return x >= m_w - m_scrollBtnSize ? 4 : 3;

	case 1:
		if (y < m_scrollBtnSize)
			return 0;
		if (y < m_scrollBtnSize + GetThumbOffset())
			return 1;
		if (y < m_scrollBtnSize + GetThumbOffset() + GetThumbSize())
			return 2;
		if (y >= m_h - m_scrollBtnSize)
			return 4;
		return 3;

	}

	return -1;
}

int UIScrollBar::GetPos() { return m_curPos; }

int UIScrollBar::GetThumbSize() {
	int length = m_h;
	switch (m_isVert) {
	case 0:
		length = m_w;
		break;
	case 1:
		length = m_h;
		break;
	}
	int size = length;
	size -= m_scrollBtnSize * 2;
	if (m_parentNumItem != 0)
		size = m_parentnumCanDisplayItem * size / m_parentNumItem;
	if (m_type && size < 8)
		size = 8;
	return size;
}

int UIScrollBar::GetThumbOffset() {
	return m_curPos * GetScrollRange() / m_maxPos;
}

int UIScrollBar::GetScrollRange() {
	int length = m_h;
	switch (m_isVert) {
	case 0:
		length = m_w;
		break;
	case 1:
		length = m_h;
		break;
	}
	return length - (m_scrollBtnSize * 2 + GetThumbSize());
}

UIScrollBar2::UIScrollBar2()
{
	m_scrollBtnSize = 10;
}

void UIScrollBar2::OnDraw()
{
	ClearDC(0xffff00ff);
	if (m_maxPos == 0)
		return;

	std::string pName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\");
	switch (m_isVert) {
	case 1: {
		DrawBox(0, 11, 10, m_h - 21, 0);
		if (m_drawMode == 1)
			DrawBitmap(0, 0, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "dialscr_up.bmp").c_str()), false), false);
		else if (m_drawMode == 4)
			DrawBitmap(0, 0, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "dialscr_up.bmp").c_str()), false), false);
		else
			DrawBitmap(0, 0, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "dialscr_up.bmp").c_str()), false), false);
		int thumbPos = m_scrollBtnSize + GetThumbOffset();
		DrawBox(0, thumbPos + 1, 10, GetThumbSize() - 1, 0xafaaa5);
		if (m_drawMode == 2)
			DrawBitmap(0, m_h - m_scrollBtnSize - 1, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "dialscr_down.bmp").c_str()), false), false);
		else if (m_drawMode == 5)
			DrawBitmap(0, m_h - m_scrollBtnSize - 1, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "dialscr_down.bmp").c_str()), false), false);
		else
			DrawBitmap(0, m_h - m_scrollBtnSize - 1, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "dialscr_down.bmp").c_str()), false), false);
		break;
	}
	}
}

UIScrollBar3::UIScrollBar3()
{
	m_scrollBtnSize = 9;
}

void UIScrollBar3::OnDraw()
{
	ClearDC(0xffff00ff);
	if (m_maxPos == 0)
		return;

	std::string pName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\");
	switch (m_isVert) {
	case 1: {
		DrawBox(0, 10, 9, m_h - 20, 0xf1f1f1);
		if (m_drawMode == 1)
			DrawBitmap(0, 0, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "dialscr_up1.bmp").c_str()), false), false);
		else if (m_drawMode == 4)
			DrawBitmap(0, 0, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "dialscr_up1.bmp").c_str()), false), false);
		else
			DrawBitmap(0, 0, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "dialscr_up1.bmp").c_str()), false), false);
		int thumbPos = m_scrollBtnSize + GetThumbOffset();
		if (thumbPos < 9)
			thumbPos = 9;
		int thumbSize = GetThumbSize();
		if (thumbSize > m_h - 20)
			thumbSize = 1;
		DrawBox(0, thumbPos + 1, 9, thumbSize - 1, 0xe9e9e9);
		if (m_drawMode == 2)
			DrawBitmap(0, m_h - m_scrollBtnSize, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "dialscr_down1.bmp").c_str()), false), false);
		else if (m_drawMode == 5)
			DrawBitmap(0, m_h - m_scrollBtnSize, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "dialscr_down1.bmp").c_str()), false), false);
		else
			DrawBitmap(0, m_h - m_scrollBtnSize, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "dialscr_down1.bmp").c_str()), false), false);
		break;
	}
	}

	m_type = 0;
}

UIScrollBar4::UIScrollBar4()
{
	m_scrollBtnSize = 0;
	m_fixedLength = 6;
	m_thumbSize = 12;
}

void UIScrollBar4::OnDraw()
{
	ClearDC(0xffff00ff);
	if (m_maxPos == 0)
		return;

	std::string pName("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\");
	switch (m_isVert) {
	case 0:
		DrawBitmap(0, 0, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "scrollbar.bmp").c_str()), false), true);
		{
			int thumbPos = GetThumbOffset();
			DrawBitmap(thumbPos, 0, (CBitmapRes*)g_resMgr().Get(UIBmp((pName + "scroll_bar.bmp").c_str()), false), true);
		}
		break;
	}

	m_type = 0;
}


UIImgMenu::UIImgMenu() {
	sel_w = 0;
	sel_h = 0;
	m_callerWid = WID_LAST;
	m_curMenuIndex = -1;
}

UIImgMenu::~UIImgMenu() {}

void UIImgMenu::Create(int cx, int cy, int type, int size, std::string bgImg)
{
	m_type = type;
	m_size = size;
	m_bgImg = bgImg + ".bmp";
	m_bgImgOver = bgImg;

	cx += 3;
	cy += 3;
	m_bar = new unsigned char[size];
	m_disable = new unsigned char[size];
	for (int i = 0; i < size; ++i) {
		m_bar[i] = 0;
		m_disable[i] = 0;
	}

	UIWindow::Resize(cx, cy);
	OnCreate(cx, cy);
	OnSize(cx, cy);
}

void UIImgMenu::OnMouseMove(int x, int y) {
	OnMouseHover(x, y);
}

void UIImgMenu::OnLBtnUp(int x, int y)
{
	m_curMenuIndex = GetMenuIndex(x, y);

	if (m_curMenuIndex == -1)
		return;

	if (m_disable[m_curMenuIndex])
		return;
	UIWindow* caller = g_windowMgr.QueryWindow(m_callerWid);
	if (caller)
		caller->SendMsg(NULL, UM_MENU_SELECTED, m_curMenuIndex, m_id, 0, 0);

	g_windowMgr.DeleteWindow(WID_IMGMENUWND);
}

int UIImgMenu::GetMenuIndex(int x, int y)
{
	if (x < 1 || x >= m_w - 3 || y < 1 || y >= m_h - 3)
		return -1;

	int result;

	if (m_type != 0)
	{
		sel_w = (m_w - 3) / m_size;
		result = x / sel_w;
	}
	else
	{
		sel_h = (m_h - 3) / m_size;
		result = y / sel_h;
	}

	if (result > m_size - 1)
		result = m_size - 1;

	return result;
}

void UIImgMenu::OnDraw()
{
	DrawBox(0, 0, m_w, m_h, 0x99a8ac);

	CBitmapRes* bg = (CBitmapRes*)g_resMgr().Get(UIBmp(m_bgImg.c_str()), false);
	DrawBitmap(1, 1, bg, false);

	if (m_curMenuIndex > -1) {
		char tmp[10];
		sprintf(tmp, "_%d.bmp", m_curMenuIndex);
		std::string buf = m_bgImgOver + tmp;
		CBitmapRes* bitmap = (CBitmapRes*)g_resMgr().Get(UIBmp(buf.c_str()), false);

		int offset = 0;
		if (m_bar[m_curMenuIndex])
			offset = 3;

		if (!m_disable[m_curMenuIndex]) {
			if (m_type) {
				DrawBitmap(m_curMenuIndex * sel_w + offset + 1, 1, bitmap, false);
			} else {
				DrawBitmap(1, m_curMenuIndex * sel_h + offset + 1, bitmap, false);
			}
		}
	}
}

void UIImgMenu::OnMouseShape(int x, int y) {
	g_modeMgr.GetCurMode()->SetCursorAction(2);
}

void UIImgMenu::Move(int x, int y) {
	UIWindow::Move(x, y);
}

void UIImgMenu::OnMouseHover(int x, int y) {
	int oldMenuIndex = m_curMenuIndex;
	m_curMenuIndex = GetMenuIndex(x, y);
	if (oldMenuIndex != m_curMenuIndex)
		Invalidate();
}

INT_PTR UIImgMenu::SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch (message) {
	case UM_SETINFO:
		{
			m_type = val1;
			m_size = val2;
			std::string bgImg((char*)val3);
			m_bgImg = bgImg + ".bmp";
			m_bgImgOver = bgImg;

			m_bar = new unsigned char[m_size];
			m_disable = new unsigned char[m_size];
			for (int i = 0; i < m_size; ++i) {
				m_bar[i] = 0;
				m_disable[i] = 0;
			}

			Invalidate();
		}
		break;

	case UM_SETID:
		m_id = val1;
		break;

	case UM_SET_CALLER_WID:
		m_callerWid = (WINDOWID)val1;
		break;

	case UM_SET_BAR:
		if (val2)
			m_bar[val1 - 1] = 1;
		else
			m_bar[val1 - 1] = 0;
		break;

	case UM_SET_DISABLE:
		if (val2)
			m_disable[val1 - 1] = 1;
		else
			m_disable[val1 - 1] = 0;
		break;

	default:
		return UIWindow::SendMsg(sender, message, val1, val2, val3, val4);
	}

	return 0;
}

UITextButton::UITextButton()
{
	m_id2 = 0;
	m_fontBgColor = 0x00ffffff;
}

UITextButton::~UITextButton() {}

void UITextButton::Create(char const* text, unsigned int fontColor, int fontFace, int fontSize, bool isFontBold)
{
	UIButton::SetText(std::string(text));
	m_text2 = m_text;
	m_fontColor = fontColor;
	m_selectColor = fontColor;
	m_btnColor = fontColor;
	m_fontFace = fontFace;
	m_fontSize = fontSize;
	m_isFontBold = isFontBold;

	int width = strlen(text) * fontSize / 2 + 2;
	int height = fontSize + 2;
	if (isFontBold)
		width += 4;

	UIWindow::Resize(width, height);
	OnCreate(width, height);
	OnSize(width, height);
}

void UITextButton::OnRBtnDown(int x, int y) {
	if (m_isDisabled)
		return;
	if (m_parent != NULL)
		m_parent->SendMsg(this, UM_TEXT_RBTN_UP, m_id, m_id2, 0, 0);
	Invalidate();
}

void UITextButton::OnLBtnUp(int x, int y) {
	if (m_isDisabled)
		return;
	if (g_windowMgr.GetCapture() != this)
		return;
	if (x >= 0 && x < m_w && y >= 0 && y < m_h) {
		m_state = 1;
		if (m_parent != NULL)
			m_parent->SendMsg(this, UM_TEXT_LBTN_UP, m_id, m_id2, 0, 0);
	} else {
		m_state = 0;
	}
	Invalidate();
	g_windowMgr.ReleaseCapture();
}

void UITextButton::OnRBtnUp(int x, int y) {
	if (m_isDisabled)
		return;
	if (g_windowMgr.GetCapture() != this)
		return;
	if (x >= 0 && x < m_w && y >= 0 && y < m_h) {
		m_state = 1;
		if (m_parent != NULL)
			m_parent->SendMsg(this, UM_TEXT_RBTN_UP, m_id, m_id2, 0, 0);
	} else {
		m_state = 0;
	}
	Invalidate();
	g_windowMgr.ReleaseCapture();
}

void UITextButton::OnMouseHover(int x, int y) {
	if (m_parent)
		m_parent->OnMouseHover(m_x + x, m_y + y);
	UIButton::OnMouseHover(x, y);
}

void UITextButton::OnMouseMove(int x, int y)
{
	if (m_isDisabled)
		return;

	if (g_windowMgr.GetCapture() == this) {
		if (x >= 0 && x < m_w && y >= 0 && y < m_h) {
			m_btnColor = m_selectColor;
			if (m_state != 2) {
				m_state = 2;
				Invalidate();
			}
		} else {
			m_btnColor = m_fontColor;
			if (m_state != 0) {
				m_state = 0;
				Invalidate();
			}
		}
	} else {
		if (x >= 0 && x < m_w && y >= 0 && y < m_h) {
			m_btnColor = m_selectColor;
			if (m_state != 1) {
				m_state = 1;
				Invalidate();
			}
		} else {
			m_btnColor = m_fontColor;
			if (m_state != 0) {
				m_state = 0;
				Invalidate();
			}
		}
	}
}

void UITextButton::Resize()
{
	if (strcmp(m_text.c_str(), "") != 0) {
		m_text2 = m_text;
		int width = GetTextWidth(m_text.c_str(), 0, m_fontFace, m_fontSize, m_isFontBold) + 2;
		int height = m_fontSize + 2;

		UIWindow::Resize(width, height);
	}
}

void UITextButton::Resize2(int cx, int cy)
{
	if (strcmp(m_text.c_str(), "") != 0) {
		Resize();
		if (m_w > cx) {
			m_text2 = PolishText(m_text.c_str(), cx, m_fontFace, m_fontSize, m_isFontBold);
			UIWindow::Resize(cx, cy);
		}
	}
}

void UITextButton::OnDraw() {
	ClearDC(0xffff00ff);
	if (m_parent != NULL)
		m_parent->Invalidate();
	if (m_isFontBold) {
		TextOutWithOutline(1, 1, m_text2.c_str(), 0, m_btnColor, m_fontBgColor, m_fontFace, m_fontSize, 1);
	} else {
		TextOutA(1, 1, m_text2.c_str(), 0, m_fontFace, m_fontSize, m_btnColor);
	}
}

UIBitmapButton2::UIBitmapButton2() {}

UIBitmapButton2::~UIBitmapButton2() {}

void UIBitmapButton2::DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo) {
	if (g_windowMgr.GetExchangeWnd()) {
		UIWindow::DragAndDrop(x, y, dragInfo);
	} else if (m_parent != NULL) {
		m_parent->DragAndDrop(x, y, dragInfo);
	}
}

void UIBitmapButton2::OnLBtnUp(int x, int y) {
	if (m_isDisabled)
		return;
	if (g_windowMgr.GetCapture() != this)
		return;
	if (x >= 0 && x < m_w && y >= 0 && y < m_h) {
		m_state = 1;
		if (m_parent != NULL)
			m_parent->SendMsg(this, UM_BMP_LBTN_UP, m_id, 0, 0, 0);
	} else {
		m_state = 0;
	}
	Invalidate();
	g_windowMgr.ReleaseCapture();
}

void UIBitmapButton2::OnRBtnUp(int x, int y) {
	if (m_isDisabled)
		return;
	if (g_windowMgr.GetCapture() != this)
		return;
	if (x >= 0 && x < m_w && y >= 0 && y < m_h) {
		m_state = 1;
		if (m_parent != NULL)
			m_parent->SendMsg(this, UM_BMP_RBTN_UP, m_id, 0, 0, 0);
	} else {
		m_state = 0;
	}
	Invalidate();
	g_windowMgr.ReleaseCapture();
}

void UIBitmapButton2::OnRBtnDown(int x, int y) {
	if (m_isDisabled)
		return;
	if (m_parent != NULL)
		m_parent->SendMsg(this, UM_BMP_RBTN_UP, m_id, 0, 0, 0);
	Invalidate();
}




UIChatHisBox3::UIChatHisBox3()
{
	m_type = 1;
	m_fontSize = 12;
}

UIChatHisBox3::~UIChatHisBox3()
{}

void UIChatHisBox3::OnCreate(int cx, int cy)
{
	m_vertScrollBar = new UIScrollBar3;
	m_vertScrollBar->Create(cx - 10, 0, 1, cy);
	AddChild(m_vertScrollBar);
	m_horzScrollBar = new UIScrollBar3;
	m_horzScrollBar->Create(0, cy - 10, 0, cx - 10);
	AddChild(m_horzScrollBar);
}

void UIChatHisBox3::Resize(int cx, int cy) {
	UIWindow::Resize(cx, cy);
	m_vertScrollBar->Resize(9, cy);
	m_vertScrollBar->Move(cx - 10, 0);
	m_horzScrollBar->Resize(cx - 10, 9);
	m_horzScrollBar->Move(0, cy - 10);
	RecalcScrbarPos();
}

void UIChatHisBox3::AddItem(char const *txt, unsigned int itemRGB)
{
	bool isBottomLine = false;
	if (m_vertScrEnabled) {
		if (m_vertViewOffset == m_items.size() + m_horzScrEnabled - m_h / m_itemSpacing)
			isBottomLine = true;
	} else
		isBottomLine = true;

	if (m_items.size() >= (unsigned int)m_maxItem)
		DeleteFirstHalfItem();
	m_itemColors.push_back(itemRGB);
	UIListBox::AddItem(txt);
	if (m_vertScrEnabled) {
		if (isBottomLine && m_type) {
			m_vertViewOffset = m_items.size() + m_horzScrEnabled - m_h / m_itemSpacing;
			m_vertScrollBar->SetPos(m_vertViewOffset);
			m_vertScrollBar->Invalidate();
		}
	}
	Invalidate();
}

void UIChatHisBox3::AddItem2(char const *txt, unsigned int itemRGB, char const *tab)
{
	int tabPx = strlen(tab) * m_fontSize;
	int width = m_w - (tabPx + tabPx);
	std::vector<std::string> strings;
	std::vector<bool> enters;
	int len;

	PolishText3(txt, strings, enters, width);

	for (int i = 0; i < strings.size(); i++) {
		len = strlen(strings[i].c_str());
		char* line = new char[len + 1];
		strcpy(line, strings[i].c_str());
		line[len] = 0;
		if (line[len - 2] == '\r')
			line[len - 2] = 0;

		char* item = new char[strlen(line) + strlen(tab) + 1];
		sprintf(item, "%s%s", tab, line);
		AddItem(item, itemRGB);
#if defined(BF_PORT)
		delete[] item;	// the copy was never freed; one per line drawn into the chat (M8.6 soak, leaks)
		delete[] line;
#endif
	}
}

void UIChatHisBox3::SetScrollPos(int pos) {
	if (m_vertScrEnabled)
		m_vertScrollBar->SetPos(pos);
	RecalcScrbarPos();
}

void UIChatHisBox3::OnDraw()
{
	if (m_isTransParent) {
		ClearDC(0xffff00ff);
		if (m_parent)
			m_parent->Invalidate();
	} else {
		ClearDC(RGB(m_bR, m_bG, m_bB));
	}

	int firstItem = m_vertViewOffset;
	int lastItem = min(m_h / m_itemSpacing - m_horzScrEnabled + firstItem,
					   m_items.size());
	for (int i = firstItem; i < lastItem; i++) {
		TextOutA(2, (i - firstItem) * m_itemSpacing + 3, m_items[i].c_str(), 0, 0, m_fontSize, m_itemColors[i]);
	}
}

UITabControl::UITabControl() {
	m_tab = -1;
}
UITabControl::~UITabControl()
{}

void UITabControl::Create(bool bVerticalType, int width, int height, unsigned int onColor, unsigned int offColor, unsigned int bgColor, unsigned int btmColor) {
	OnCreate(0, 0);
	m_bVerticalType = bVerticalType;
	m_tabWidth = width;
	m_tabHeight = height;
	m_colorOnTab = onColor;
	m_colorOffTab = offColor;
	m_colorBgTab = bgColor;
	m_colorBtmTab = btmColor;
}

void UITabControl::SetTabString(char const *tabString)
{
	m_TabStrings.push_back(std::string(tabString));

	m_maxTab = m_TabStrings.size();
	m_tab = 0;

	int width;
	if (m_bVerticalType) {
		width = strlen(tabString) / 2 + strlen(tabString) % 2;
	} else {
		width = GetTextWidth(tabString, 0, 0, 12, false);
	}

	if (m_tabWidth < width * 12)
		m_tabWidth = width * 12 + 8;

	if (m_bVerticalType) {
		Resize(m_tabHeight + 1, (m_tabWidth - 1) * m_maxTab + 1);
	} else {
		Resize((m_tabWidth - 1) * m_maxTab + 1, m_tabHeight + 1);
	}
}

void UITabControl::OnDraw()
{
	ClearDC(0xffff00ff);

	int i = 0;
	std::list<std::string>::iterator it = m_TabStrings.begin();

	for (; it != m_TabStrings.end(); ++it, ++i) {
		unsigned int colorTab = m_colorOffTab;
		int offset = 1;
		if (m_tab == i) {
			colorTab = m_colorOnTab;
			offset = 0;
		}

		int tabPos = (m_tabWidth - 1) * i;
		int ty = (m_tabHeight - 12) / 2 + 1;
		if (m_bVerticalType) {
			unsigned int colorBgTab = m_colorBgTab;
			DrawBox(offset + 1, tabPos + 1, m_tabHeight - offset, m_tabWidth - 2, colorBgTab);
			DrawBorderLine(offset, tabPos, m_tabHeight + 5 - offset, m_tabWidth, colorTab);

			int width = strlen(it->c_str()) / 2 + strlen(it->c_str()) % 2;
			int tx = (m_tabWidth - width * 12) / 2 + tabPos;
			if (m_tab == i) {
				TextOutVertical(ty + 1, tx + 1, it->c_str(), 0, 0, 12, 0xe0e0e0);
				TextOutVertical(ty, tx, it->c_str(), 0, 0, 12, 0);
			} else {
				TextOutVertical(ty, tx, it->c_str(), 0, 0, 12, colorTab);
			}
		} else {
			unsigned int colorBgTab = m_colorBgTab;
			DrawBox(tabPos + 1, offset + 1, m_tabWidth - 2, m_tabHeight - offset, colorBgTab);
			DrawBorderLine(tabPos, offset, m_tabWidth, m_tabHeight + 5 - offset, colorTab);

			int tx = (m_tabWidth - GetTextWidth(it->c_str(), 0, 0, 12, false)) / 2 + tabPos;
			if (m_tab == i) {
				TextOutWithShadow(tx, ty, it->c_str(), 0, colorTab, 0xe0e0e0, 0, 12);
			} else {
				TextOutWithDecoration(tx, ty, it->c_str(), 0, &colorTab, 0, 12);
			}
		}
	}

	if (m_bVerticalType) {
		DrawBox(m_tabHeight, 0, 1, m_h, m_colorBtmTab);
		DrawBox(m_tabHeight, (m_tabWidth - 1) * m_tab + 1, 1, m_tabWidth - 2, m_colorBgTab);
	} else {
		DrawBox(0, m_tabHeight, m_w, 1, m_colorBtmTab);
		DrawBox((m_tabWidth - 1) * m_tab + 1, m_tabHeight, m_tabWidth - 2, 1, m_colorBgTab);
	}
}


void UITabControl::OnLBtnDown(int x, int y) {
	int xx = x, yy = y;
	if (m_bVerticalType) {
		xx = y;
		yy = x;
	}
	if (yy > 0 && yy < m_tabHeight) {
		int oldTab = m_tab;
		int newTab = xx / m_tabWidth;
		if (xx > 0 && newTab < m_maxTab && oldTab != newTab) {
			m_tab = newTab;
		}
		if (oldTab != m_tab) {
			ChangeTab();
		}
	}
}

void UITabControl::ChangeTab() {
	if (m_parent != NULL)
		m_parent->SendMsg(this, UM_TAB, m_tab, 0, 0, 0);
	Invalidate();
}
