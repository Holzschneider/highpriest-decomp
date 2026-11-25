#include "common.h"
#include "window/UISys.h"
#include "Mode.h"
#include "window/UIFrameWnd.h"
#include "window/UIImeWnd.h"
#include "GameMode.h"
#include "MsgStrings.h"
#include "skin.h"
#include "framework/Language.h"
#include "framework/Locale.h"
#include "View.h"
#include "GameActor.h"
#include "Session.h"
#include "base/Res.h"
#include "resource/Bitmap.h"
#include "resource/Sprite.h"
#include "resource/Action.h"
#include "resource/Emblem.h"
#include "3dlib/Renderer.h"
#include "resource/Ground.h"

extern CRenderer* g_renderer;
extern UIWindowMgr g_windowMgr;
extern CModeMgr g_modeMgr;
extern CSession g_session;
extern ServiceType g_serviceType;
extern const char* UIBmp(const char* uiBmpName);
extern const char* MsgStr(MSGSTRINGID id);
extern bool NameBalloonShorten;
extern bool NameBalloonfontBold;
extern int NameBalloonfontSize;
extern DWORD g_codePage;


void CutString(const char *msg, std::vector<std::string> &strings, int maxNumCharLine);

UIStaticText::UIStaticText() {
	m_fontHeight = 12;
	m_backR = 0xff;
	m_backG = 0xff;
	m_backB = 0xff;
	m_fontType = 0;
	m_drawBackGround = 0;
	m_isShorten = 0;
	m_drawTwice = 0;
	m_drawBold = 0;
	m_textR = 0;
	m_textG = 0;
	m_textB = 0;
}


UIStaticText::~UIStaticText() {}

void UIStaticText::SetTextColor(int r, int g, int b) {
	m_textR = r;
	m_textG = g;
	m_textB = b;
	Invalidate();
}

void UIStaticText::SetFont(int fontType, int fontHeight) {
	m_fontType = fontType;
	m_fontHeight = fontHeight;
}

void UIStaticText::SetText(char const *txt, int drawBold)
{
	m_text = txt;
	m_fullText = txt;
	m_isShorten = 0;
	m_drawBold = drawBold;
	Invalidate();
}

void UIStaticText::SetTextWithNumber(int number)
{
	char numText[40];
	sprintf(numText, "%d", number);
	m_text = numText;
	m_fullText = numText;
	m_isShorten = 0;
	Invalidate();
}

void UIStaticText::OnDraw()
{
	if (m_drawBackGround)
		ClearDC(RGB(m_backR, m_backG, m_backB));
	else
		ClearDC(0xffff00ff);

	if (m_drawTwice)
		TextOutA(1, 1, m_text.c_str(), 0, m_fontType, m_fontHeight, 0xffffff);

	if (m_drawBold) {
		TextOutA(-1, 0, m_text.c_str(), 0, m_fontType, m_fontHeight, 0x969696);
		TextOutA(0, -1, m_text.c_str(), 0, m_fontType, m_fontHeight, 0x969696);
		TextOutA(1, 0, m_text.c_str(), 0, m_fontType, m_fontHeight, 0x969696);
		TextOutA(0, 1, m_text.c_str(), 0, m_fontType, m_fontHeight, 0x969696);
	}

	TextOutA(0, 0, m_text.c_str(), 0, m_fontType, m_fontHeight, RGB(m_textR, m_textG, m_textB));
}

#include "rag_budget_fill.h"
void UIStaticText::SetTextWithShorten(char const *txt)
{
	m_fullText = txt;
	m_isShorten = 0;
	if (m_dc) {
		int txtWidth = GetTextWidth(txt, 0, m_fontType, m_fontHeight, false);
		RAG_BUDGET_FILL(4)
		GetTextWidth("...", 0, m_fontType, m_fontHeight, false);
		if (m_w >= txtWidth) {
			m_text = txt;
		} else {
			m_isShorten = 1;
			m_text = txt;
			std::string tmpTxt(txt);
			while (m_w < txtWidth) {
				if (tmpTxt.size() <= 1) {
					m_text = tmpTxt + "...";
					break;
				}
				if (tmpTxt.size() >= 2) {
					if ((tmpTxt[tmpTxt.size() - 2] & 0x80) && (tmpTxt[tmpTxt.size() - 1] & 0x80))
						tmpTxt.resize(tmpTxt.size() - 2);
					else
						tmpTxt.resize(tmpTxt.size() - 1);
				}
				m_text = tmpTxt + "...";
				txtWidth = GetTextWidth(m_text.c_str(), 0, m_fontType, m_fontHeight, false);
			}
		}
	} else {
		m_text = txt;
	}
	Invalidate();
}

void UIStaticText::OnMouseHover(int x, int y) {
	if (!m_isShorten) return;
	int gx = -6, gy = -4;
	GetGlobalCoor(gx, gy);
	CMode* m = g_modeMgr.GetCurMode();
	m->MakeHelpBalloon(m_fullText.c_str(), gx, gy, -1);
}

void UIStaticText::SetTextWithNewWidth(char const *txt)
{
	m_isShorten = 0;
	m_fullText = txt;
	m_text = txt;
	Invalidate();
	if (!m_drawBackGround) {
		if (m_parent) {
			m_parent->Invalidate();
		}
	}
	Resize(GetTextWidth(txt, 0, m_fontType, m_fontHeight, 0) + 2, m_h);
}

void UIStaticText::OnLBtnDblClk(int x, int y) {
	m_parent->OnLBtnDblClk(m_x + x, m_y + y);
}

void UIStaticText::OnMouseShape(int x, int y) {
	m_parent->OnMouseShape(m_x + x, m_y + y);
}

void UIStaticText::OnMouseMove(int x, int y) {
	OnMouseHover(x, y);
	m_parent->OnMouseMove(m_x + x, m_y + y);
}

void UIStaticText::OnWheel(int wheel) {
	m_parent->OnWheel(wheel);
}

void UIStaticText::OnLBtnDown(int x, int y) {
	m_parent->OnLBtnDown(m_x + x, m_y + y);
}

void UIStaticText::OnLBtnUp(int x, int y) {
	m_parent->OnLBtnUp(m_x + x, m_y + y);
}

void UIStaticText::OnRBtnDown(int x, int y) {
	m_parent->OnRBtnDown(m_x + x, m_y + y);
}

void UIStaticText::OnRBtnUp(int x, int y) {
	m_parent->OnRBtnUp(m_x + x, m_y + y);
}

UIStaticBitmap::UIStaticBitmap() {}


UIStaticBitmap::~UIStaticBitmap() {}

void UIStaticBitmap::OnDraw() {
	if (m_bitmapName.size() == 0) return;
	switch (m_state) {
	case 0: {
		CBitmapRes* bm = (CBitmapRes*)g_resMgr().Get(UIBmp(m_bitmapName.c_str()), false);
		DrawBitmap(0, 0, bm, false);
		break;
	}
	case 1:
		ClearDC(0xffff00ff);
		break;
	}
}

void UIStaticBitmap::SetBitmap(char const *bitmapName)
{
	m_bitmapName = bitmapName;
	CBitmapRes *bitmap;
	if (g_serviceType == ServiceKorea)
		bitmap = (CBitmapRes *)g_resMgr().Get(m_bitmapName.c_str(), false);
	else
		bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp(m_bitmapName.c_str()), false);
	m_w = bitmap->m_width;
	m_h = bitmap->m_height;
}

void UIStaticBitmap::OnLBtnDown(int x, int y) {}

void UIStaticBitmap::OnMouseMove(int x, int y) {}

void UIStaticBitmap::OnLBtnUp(int x, int y) {}

UICheckBox::UICheckBox() {
	m_isDisabled = 0;
	m_state = 0;
	m_id = 0x93;
}


UICheckBox::~UICheckBox() {}

void UICheckBox::SetCheck(int check) {
	if (m_isDisabled) return;
	m_state = check;
	Invalidate();
	if (m_parent)
		m_parent->SendMsg(this, UM_COMMAND, m_id, m_state, 0, 0);
}

void UICheckBox::OnLBtnDown(int x, int y) {
	if (m_isDisabled) return;
	m_state = 1 - m_state;
	SetCheck(m_state);
}

void UICheckBox::SetBitmap(char const *onBitmapName, char const *offBitmapName)
{
	m_onBitmapName = onBitmapName;
	m_offBitmapName = offBitmapName;
	CBitmapRes *bitmap;
	if (g_serviceType == ServiceKorea)
		bitmap = (CBitmapRes *)g_resMgr().Get(m_onBitmapName.c_str(), false);
	else
		bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp(m_onBitmapName.c_str()), false);
	m_w = bitmap->m_width;
	m_h = bitmap->m_height;
	if (g_serviceType == ServiceKorea)
		g_resMgr().Get(m_offBitmapName.c_str(), false);
	else
		g_resMgr().Get(UIBmp(m_offBitmapName.c_str()), false);
}

void UICheckBox::OnMouseShape(int x, int y) {
	g_modeMgr.GetCurMode()->SetCursorAction(2);
}

void UICheckBox::SetBitmap2(char const *onBitmapName, char const *offBitmapName, char const *checkMsg)
{
	m_onBitmapName = onBitmapName;
	m_offBitmapName = offBitmapName;
	if (g_serviceType == ServiceKorea)
		g_resMgr().Get(m_onBitmapName.c_str(), false);
	else
		g_resMgr().Get(UIBmp(m_onBitmapName.c_str()), false);
	if (g_serviceType == ServiceKorea)
		g_resMgr().Get(m_offBitmapName.c_str(), false);
	else
		g_resMgr().Get(UIBmp(m_offBitmapName.c_str()), false);
	m_checkMsg = checkMsg;
}

void UICheckBox::OnDraw()
{
	if (m_onBitmapName.size() == 0 || m_offBitmapName.size() == 0)
		return;

	if (m_isDisabled) {
		ClearDC(0xffff00ff);
		return;
	}

	switch (m_state) {
	case 0: {
		CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp(m_offBitmapName.c_str()), false);
		int y = (m_h - bmp->m_height) / 2;
		DrawBitmap(0, y, bmp, false);
		break;
	}
	case 1: {
		CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp(m_onBitmapName.c_str()), false);
		DrawBitmap(0, (m_h - bmp->m_height) / 2, bmp, false);
		break;
	}
	}

	if (m_checkMsg.size())
		TextOutA(11, 0, m_checkMsg.c_str(), 0, 0, 12, 0);
}

UISlotBitmap::UISlotBitmap(int nSlotID) {
	m_nSlotID = nSlotID;
}

UISlotBitmap::~UISlotBitmap() {}

INT_PTR UISlotBitmap::SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4) {
	if (message != UM_SETSTATE) {
		return UIWindow::SendMsg(sender, message, val1, val2, val3, val4);
	}
	m_state = val1;
	return 0;
}

void UISlotBitmap::OnLBtnDown(int x, int y) {
	if (!m_parent) return;
	m_parent->SendMsg(this, UM_SETSTATE, m_nSlotID, 0, 0, 0);
	m_parent->SendMsg(this, UM_COMMAND, m_id, 0, 0, 0);
}

void UISlotBitmap::OnLBtnDblClk(int x, int y) {
	if (m_parent)
		m_parent->SendMsg(this, UM_COMMAND, 118, 0, 0, 0);
}

UIButton::UIButton() {
	m_state = 0;
	m_isDisabled = 0;
}

UIButton::~UIButton() {}

void UIButton::OnLBtnDblClk(int x, int y) {
	OnLBtnDown(x, y);
}

void UIButton::SetToolTip(const char* toolTip) {
	m_toolTip = toolTip;
}

void UIButton::OnLBtnDown(int x, int y) {
	if (m_isDisabled) return;
	g_windowMgr.SetCapture(this);
	m_state = 2;
	Invalidate();
}

void UIButton::OnMouseHover(int x, int y) {
	if (m_isDisabled) return;
	OnMouseMove(x, y);
	if (m_toolTip.size() == 0) return;
	int gx = 0, gy = -20;
	GetGlobalCoor(gx, gy);
	g_modeMgr.GetCurMode()->MakeHelpBalloon(m_toolTip.c_str(), gx, gy, -1);
}

void UIButton::OnMouseShape(int x, int y) {
	if (m_isDisabled) {
		UIWindow::OnMouseShape(x, y);
		return;
	}
	g_modeMgr.GetCurMode()->SetCursorAction(2);
}

void UIButton::OnLBtnUp(int x, int y) {
	if (m_isDisabled) return;
	if (g_windowMgr.GetCapture() != this) return;
	if (x >= 0 && x < m_w && y >= 0 && y < m_h) {
		m_state = 1;
		if (m_parent) {
			m_parent->SendMsg(this, UM_COMMAND, m_id, 0, 0, 0);
		} else {
			g_modeMgr.GetCurMode()->SendMsg(MM_COMMAND, m_id, 0, 0);
		}
	} else {
		m_state = 0;
	}
	Invalidate();
	g_windowMgr.ReleaseCapture();
}

void UIButton::OnMouseMove(int x, int y) {
	if (m_isDisabled) return;
	if (g_windowMgr.GetCapture() == this) {
		if (x >= 0 && x < m_w && y >= 0 && y < m_h) {
			if (m_state != 2) {
				m_state = 2;
				Invalidate();
			}
		} else {
			if (m_state) {
				m_state = 0;
				Invalidate();
			}
		}
	} else {
		if (x >= 0 && x < m_w && y >= 0 && y < m_h) {
			if (m_state != 1) {
				m_state = 1;
				Invalidate();
			}
		} else {
			if (m_state) {
				m_state = 0;
				Invalidate();
			}
		}
	}
}

void UIButton::OnDraw() {
	CBitmapRes* m_bitmap;
	switch (m_state) {
	case 0:
		m_bitmap = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sml_button_up.bmp"), 0);
		break;
	case 1:
		m_bitmap = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sml_button_act.bmp"), 0);
		break;
	case 2:
		m_bitmap = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sml_button_down.bmp"), 0);
		break;
	}
	DrawBitmap(0, 0, m_bitmap, false);
	TextOutA(0, 0, m_text.c_str(), 0, 0, 12, 0);
}

UINoneVisibleButton::UINoneVisibleButton() {
	m_state = 0;
	m_isDisabled = 0;
}

UINoneVisibleButton::~UINoneVisibleButton() {}

void UINoneVisibleButton::OnDraw() {
	ClearDC(0xffff00ff);
}

UIColorButton::UIColorButton() {
	unsigned int rr, gg, bb;
	g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
	m_r = rr;
	m_g = gg;
	m_b = bb;
	g_skinMgr.GetColorChipColor(0x12, 2, rr, gg, bb);
	m_br = rr;
	m_bg = gg;
	m_bb = bb;
}

UIColorButton::~UIColorButton() {}

void UIColorButton::OnDraw()
{
	unsigned int rr;
	unsigned int gg;
	unsigned int bb;
	int textWidth = GetTextWidth(m_text.c_str(), 0, 0, 12, false);

	g_skinMgr.GetColorChipColor(22, 2, rr, gg, bb);
	ClearDC(RGB(rr, gg, bb));

	switch (m_state) {
	case 0:
		DrawBox(1, 1, m_w - 2, m_h - 2, RGB(m_br, m_bg, m_bb));
		TextOutA((m_w - textWidth) / 2 + 1, (m_h - 12) / 2 + 1, m_text.c_str(), 0, 0, 12, 0xffffff);
		TextOutA((m_w - textWidth) / 2, (m_h - 12) / 2, m_text.c_str(), 0, 0, 12, 0);
		break;
	case 1:
		DrawBox(1, 1, m_w - 2, m_h - 2, RGB(m_r, m_g, m_b));
		TextOutA((m_w - textWidth) / 2 + 1, (m_h - 12) / 2 + 1, m_text.c_str(), 0, 0, 12, 0xffffff);
		TextOutA((m_w - textWidth) / 2, (m_h - 12) / 2, m_text.c_str(), 0, 0, 12, 0);
		break;
	case 2:
		DrawBox(1, 1, m_w - 2, m_h - 2, RGB(m_r, m_g, m_b));
		TextOutA((m_w - textWidth) / 2 + 2, (m_h - 12) / 2 + 2, m_text.c_str(), 0, 0, 12, 0xffffff);
		TextOutA((m_w - textWidth) / 2 + 1, (m_h - 12) / 2 + 1, m_text.c_str(), 0, 0, 12, 0);
		break;
	}
}

UIBitmapButton::UIBitmapButton() {
	m_bitmapWidth = 0;
	m_bitmapHeight = 0;
}

#include "GameWorld.h"

UIBitmapButton::~UIBitmapButton()
{}

void UIBitmapButton::OnDraw() {
	CBitmapRes* bitmap;
	switch (m_state) {
	case 0:
		bitmap = (CBitmapRes*)g_resMgr().Get(UIBmp(m_normalBitmapName.c_str()), 0);
		break;
	case 1:
		bitmap = (CBitmapRes*)g_resMgr().Get(UIBmp(m_mouseonBitmapName.c_str()), 0);
		break;
	case 2:
		bitmap = (CBitmapRes*)g_resMgr().Get(UIBmp(m_pressedBitmapName.c_str()), 0);
		break;
	}
	DrawBitmap(0, 0, bitmap, false);
}

void UIBitmapButton::SetBitmapName(char const *bitmapName, int buttonState)
{
	CBitmapRes *bitmap;
	if (g_serviceType == ServiceKorea)
		bitmap = (CBitmapRes *)g_resMgr().Get(bitmapName, false);
	else
		bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp(bitmapName), false);
	if (!bitmap)
		return;

	if (bitmap->m_width > m_bitmapWidth)
		m_bitmapWidth = bitmap->m_width;
	if (bitmap->m_height > m_bitmapHeight)
		m_bitmapHeight = bitmap->m_height;

	switch (buttonState) {
	case 0:
		m_normalBitmapName = bitmapName;
		break;
	case 1:
		m_mouseonBitmapName = bitmapName;
		break;
	case 2:
		m_pressedBitmapName = bitmapName;
		break;
	}
}

UISpriteButton::UISpriteButton() {
	m_curAct = 0;
	m_curMot = 0;
	m_maxAct = 0;
	m_maxMot = 0;
	m_isAnimation = 0;
	m_isFitToSize = 1;
}

UISpriteButton::~UISpriteButton()
{}

void UISpriteButton::SetActFileName(const char* actName) {
	m_strActName = actName;
}

void UISpriteButton::SetSpriteName(char const *sprName)
{
	char fileName[256];
	strcpy(fileName, sprName);
	fileName[strlen(sprName) - 3] = 0;
	m_strSprName = fileName;
	m_strSprName += "spr";
	m_strActName = fileName;
	m_strActName += "act";
}

int UISpriteButton::GetSprHeight(int act, int mot) {
	CSprRes* spr = (CSprRes*)g_resMgr().Get(m_strSprName.c_str(), 0);
	CActRes* actRes = (CActRes*)g_resMgr().Get(m_strActName.c_str(), 0);
	CMotion* m = actRes->GetMotion(act, mot);
	return spr->m_sprites[0][m->sprClips[0].sprIndex]->height;
}

int UISpriteButton::GetSprWidth(int act, int mot) {
	CSprRes* spr = (CSprRes*)g_resMgr().Get(m_strSprName.c_str(), 0);
	CActRes* actRes = (CActRes*)g_resMgr().Get(m_strActName.c_str(), 0);
	CMotion* m = actRes->GetMotion(act, mot);
	return spr->m_sprites[0][m->sprClips[0].sprIndex]->width;
}

void UISpriteButton::OnDraw()
{
	CActRes *actRes = (CActRes *)g_resMgr().Get(m_strActName.c_str(), false);
	CSprRes *sprRes = (CSprRes *)g_resMgr().Get(m_strSprName.c_str(), false);

	if (!actRes || !sprRes)
		return;

	{
		CMotion *curMotion = actRes->GetMotion(m_curAct, m_curMot);

		int w = 0, h = 0, ox = 0, oy = 0;
		for (int a = 0; a < curMotion->sprClips.size(); a++) {
			CacheInfo info;
			unsigned int *palette = sprRes->m_pal;
			CSprClip *clip = curMotion->GetClip(a);
			if (clip->sprIndex == -1)
				return;

			SprImg *img = sprRes->m_sprites[clip->clipType][clip->sprIndex];
			CTexture *tex;
			if (clip->clipType == 0) {
				tex = g_renderer->GetSpriteIndex(*img, (INT_PTR)palette, &info);
				if (!tex)
					tex = g_renderer->AddSpriteIndex(*img, palette, &info);
			} else {
				tex = img->tex;
			}

			int x, y;
			if (a == 0) {
				x = m_w / 2 - img->width / 2;
				y = m_h / 2 - img->height / 2;
				w = img->width / 2;
				h = img->height / 2;
				ox = clip->x;
				oy = clip->y;
			} else {
				x = m_w / 2 - ox - w + clip->x;
				y = m_h / 2 + clip->y - oy - h;
			}

			int imgWidth = img->width, imgHeight = img->height, srcx = 0, srcy = 0;
			if (x + imgWidth > m_w) {
				imgWidth += m_w - img->width - x;
			}
			if (x < 0) {
				srcx = -x;
				x = 0;
			}

			if (y + imgHeight > m_h) {
				imgHeight += m_h - img->height - y;
			}
			if (y < 0) {
				srcy = -y;
				y = 0;
			}

			int dx = imgWidth - srcx, dy = imgHeight - srcy;
			if (imgWidth - srcx < 0) {
				dx = 0;
			}
			if (imgHeight - srcy < 0) {
				dy = 0;
			}

			int ix = (int)(info.tu * 256.0f + srcx), iy = (int)(info.tv * 256.0f + srcy);
			if (ix < 0) {
				ix = 0;
			}
			if (iy < 0) {
				iy = 0;
			}

			if (m_dc && clip && img) {
				m_dc->BltTexture2(x, y, tex, ix, iy, dx, dy,
					clip->flags, img->isHalfW + 1, img->isHalfH + 1);
			}
		}
	}

	if (m_isAnimation) {
		m_curMot++;
		if (m_curAct >= actRes->GetActionCount()) {
			m_curAct = 0;
			m_curMot = 0;
		}
		if (m_curMot >= actRes->GetMotionCount(m_curAct)) {
			m_curMot = 0;
		}
	}
}

void UISpriteButton::OnMouseMove(int x, int y) {
	GetAncestor()->OnMouseMove(m_x + x, m_y + y);
}

void UISpriteButton::OnLBtnDown(int x, int y) {
	UIButton::OnLBtnDown(x, y);
	GetAncestor()->OnLBtnDown(m_x + x, m_y + y);
}

UIBalloonText::UIBalloonText() {
	m_isBold = 0;
	m_fontSize = 12;
	m_fontColor = 0xffffff;
	m_stateCnt = 0;
	m_bgColor = 0;
}


UIBalloonText::~UIBalloonText() {}

void UIBalloonText::OnProcess() {
	m_stateCnt++;
	if (m_stateCnt == 2)
		SetShow(1);
}

void UIBalloonText::SetText(char const *msg, const int maxNumCharLine)
{
	if (strlen(msg)) {
		m_strings.clear();
		CutString(msg, m_strings, maxNumCharLine);
		if (m_strings.size() > 0) {
			AdjustSizeByText();
		}
	}
}

void UIBalloonText::AddText(const char* msg) {
	m_strings.push_back(msg);
	AdjustSizeByText();
}

void UIBalloonText::SetFntSize(int fontSize) {
	m_fontSize = fontSize;
	AdjustSizeByText();
}

void UIBalloonText::OnDraw() {
	ClearDC(0xffffff);
	int y = 4;
	for (int i = 0; (unsigned int)i < m_strings.size(); ++i) {
		TextOutA(6, y, m_strings[i].c_str(), 0, NULL, m_fontSize, m_fontColor);
		y += GetTextHeight(m_strings[i].c_str(), 0, 0, m_fontSize, 0) + 4;
	}
}

void UIBalloonText::AdjustSizeByText()
{
	int width = 0;
	int y = 4;
	for (int i = 0; (unsigned int)i < m_strings.size(); ++i) {
		width = max(width, GetTextWidth(m_strings[i].c_str(), 0, 0, m_fontSize, 0));
		y += GetTextHeight(m_strings[i].c_str(), 0, 0, m_fontSize, 0) + 4;
	}
	Resize(width + 12, y);
	Invalidate();
}

UITransBalloonText::UITransBalloonText() {}

UITransBalloonText::~UITransBalloonText() {}

void UITransBalloonText::OnCreate(int cx, int cy) {
	int z = 0;
	m_transBoxInfo.drawEdge = z;
	m_transBoxInfo.x = m_x;
	m_transBoxInfo.y = m_y;
	m_transBoxInfo.cx = m_w;
	m_transBoxInfo.cy = m_h;
	m_transBoxInfo.color = 0x66000000;
	m_transBoxInfo.color2 = z;
	m_charfont = z;
}

void UITransBalloonText::Resize(int cx, int cy) {
	UIBalloonText::Resize(cx, cy);
	m_transBoxInfo.cx = m_w;
	m_transBoxInfo.cy = m_h;
}

void UITransBalloonText::Move(int x, int y) {
	UIBalloonText::Move(x, y);
	m_transBoxInfo.x = m_x;
	m_transBoxInfo.y = m_y;
}

void UITransBalloonText::OnDraw()
{
	ClearDC(0xffff00ff);
	int y = 4;
	for (int i = 0; (unsigned int)i < m_strings.size(); ++i) {
		TextOutWithShadow(6, y, m_strings[i].c_str(), 0, m_fontColor, m_bgColor, 0, m_fontSize);
		y += GetTextHeight(m_strings[i].c_str(), 0, 0, m_fontSize, 0) + 4;
	}
}

void UIColorTransBalloonText::OnDraw() {
	ClearDC(0xffff00ff);
	unsigned int colorRef = 0xffffff;
	TextOutWithDecoration(6, 4, m_strings[0].c_str(), 0, &colorRef, 0, 0xc);
}

void UITransBalloonText::SetTransBoxColor(unsigned int transboxColor) {
	m_transBoxInfo.color = transboxColor;
}

bool UITransBalloonText::GetTransBoxInfo(BOXINFO* boxInfo) {
	*boxInfo = m_transBoxInfo;
	return true;
}

UINameBalloonText::UINameBalloonText() {
	m_bgColor = 0;
	m_textWidth = 0;
	m_isBack = 0;
	m_fontColor = 0xffffff;
}

UINameBalloonText::~UINameBalloonText()
{}





#include "rag_budget_sites.h"


void UINameBalloonText::SetNameText(char const *cName)
{
	if (m_strings.size() > 0 && m_strings[0].compare(cName) == 0)
		return;

	m_strings.clear();
	m_strings.push_back(std::string(cName));

	m_textWidth = max(GetTextWidth((*m_strings.rbegin()).c_str(), (*m_strings.rbegin()).size(), 0, NameBalloonfontSize, NameBalloonfontBold) + 8, 32);
	Resize(m_textWidth, NameBalloonfontSize + 8);
	Invalidate();
}

void UINameBalloonText::SetNameText(NamePair const &namePair, bool cutSharp)
{
	m_strings.clear();

	std::string cName(namePair.cName);

	if (cutSharp) {
		int pos = cName.find('#');
		if (pos != std::string::npos) cName.resize(pos);
	}

	if (NameBalloonShorten) {
		if (namePair.rName.c_str()[0] == '\0') {
			m_textWidth = 0;
		} else {
			m_strings.push_back(namePair.rName.c_str());
			m_textWidth = max(GetTextWidth(m_strings.back().c_str(), m_strings.back().size(), 0, NameBalloonfontSize, NameBalloonfontBold) + 8, 32);
		}
		m_strings.push_back(cName.c_str());
		m_textWidth = max(GetTextWidth(m_strings.back().c_str(), 0, 0, NameBalloonfontSize, NameBalloonfontBold) + 8, m_textWidth);
	} else {
		if (namePair.pName.c_str()[0] == '\0') {
			m_strings.push_back(cName.c_str());
		} else {
			char temp[256];
			if (g_serviceType == ServiceKorea)
				sprintf(temp, bf_tr("%s\xa1\xba%s\xa1\xbb"), cName.c_str(), namePair.pName.c_str());
			else
				sprintf(temp, "%s (%s)", cName.c_str(), namePair.pName.c_str());
			m_strings.push_back(temp);
		}
		m_textWidth = max(GetTextWidth(m_strings.back().c_str(), (*m_strings.rbegin()).size(), 0, NameBalloonfontSize, NameBalloonfontBold) + 8, 32);

		if (namePair.gName.c_str()[0] != '\0') {
			char temp[256];
			if (g_serviceType == ServiceKorea)
				sprintf(temp, bf_tr("%s\xa1\xba%s\xa1\xbb"), namePair.gName.c_str(), namePair.rName.c_str());
			else
				sprintf(temp, "%s [%s]", namePair.gName.c_str(), namePair.rName.c_str());
			m_strings.push_back(temp); RAG_BUDGET_SITES(1)
			m_textWidth = max(GetTextWidth((*m_strings.rbegin()).c_str(), 0, 0, NameBalloonfontSize, NameBalloonfontBold) + 8, m_textWidth);
		}
	}

	Resize(m_textWidth, (NameBalloonfontSize + 2) * m_strings.size() + 6);
	Invalidate();
}

void UINameBalloonText::SetEmblemResName(char const *EmblemResName)
{
	if (EmblemResName) {
		m_ebmResName = EmblemResName;
		if (m_ebmResName.size()) {
			Resize(m_textWidth + 25, m_h > 24 ? m_h : 24);
		}
		Invalidate();
	}
}

void UINameBalloonText::OnDraw()
{
	ClearDC(0xffff00ff);
	int y = 4;
	int offX = y;
	if (m_ebmResName.size() && g_resMgr().IsExist(m_ebmResName.c_str())) {
		CEmblemRes *emblem = (CEmblemRes *)g_resMgr().Get(m_ebmResName.c_str(), false);
		if (emblem) {
			g_modeMgr.GetGameMode();
			DrawBitmap(4, (m_h - 24) / 2, emblem->GetBitmap(), true);
			offX = 32;
		}
	}

	for (std::vector<std::string>::iterator it = m_strings.begin(); it != m_strings.end(); ++it) {
		TextOutWithOutline(offX, y, it->c_str(), it->size(), m_fontColor, m_bgColor, 0, NameBalloonfontSize, NameBalloonfontBold);
		y += NameBalloonfontSize + 2;
	}
}

void UIVerticalNameBalloonText::SetNameText(NamePair const &namePair, bool cutSharp)
{
	m_strings.clear();

	std::string cName(namePair.cName);

	if (cutSharp) {
		int pos = cName.find('#');
		if (pos != std::string::npos) cName.resize(pos);
	}

	if (NameBalloonShorten) {
		if (namePair.rName.c_str()[0] == '\0') {
			m_textHeight = 0;
		} else {
			m_strings.push_back(namePair.rName.c_str());
			m_textHeight = max(GetTextHeightVertical(m_strings.back().c_str(), m_strings.back().size(), 0, NameBalloonfontSize, NameBalloonfontBold) + 8, 32);
		}
		m_strings.push_back(cName.c_str());
		m_textHeight = max(GetTextHeightVertical(m_strings.back().c_str(), 0, 0, NameBalloonfontSize, NameBalloonfontBold) + 8, m_textHeight);
	} else {
		if (namePair.pName.c_str()[0] == '\0') {
			m_strings.push_back(cName.c_str());
		} else {
			char temp[256];
			sprintf(temp, "%s (%s)", cName.c_str(), namePair.pName.c_str());
			m_strings.push_back(temp);
		}
		m_textHeight = max(GetTextHeightVertical(m_strings.back().c_str(), (*m_strings.rbegin()).size(), 0, NameBalloonfontSize, NameBalloonfontBold) + 8, 32);

		if (namePair.gName.c_str()[0] != '\0') {
			char temp[256];
			sprintf(temp, "%s [%s]", namePair.gName.c_str(), namePair.rName.c_str());
			m_strings.push_back(temp);
			m_textHeight = max(GetTextHeightVertical((*m_strings.rbegin()).c_str(), 0, 0, NameBalloonfontSize, NameBalloonfontBold) + 8, m_textHeight);
		}
	}

	Resize(m_textHeight, (NameBalloonfontSize + 2) * m_strings.size() + 6);
	Invalidate();
}

void UIVerticalNameBalloonText::SetNameText(char const *cName)
{
	if (m_strings.size() > 0 && m_strings[0].compare(cName) == 0)
		return;

	m_strings.clear();
	m_strings.push_back(std::string(cName));

	m_textHeight = max(GetTextHeightVertical(m_strings.back().c_str(), 0, 0, NameBalloonfontSize, NameBalloonfontBold) + 8, 32);
	Resize(NameBalloonfontSize + 8, m_textHeight);
	Invalidate();
}

void UIVerticalNameBalloonText::SetEmblemResName(char const *EmblemResName)
{
	if (EmblemResName && strlen(EmblemResName)) {
		m_ebmResName = EmblemResName;
		if (m_ebmResName.size()) Resize(m_w > 24 ? m_w : 24, m_textHeight + 25);
		Invalidate();
	}
}

void UIVerticalNameBalloonText::OnDraw()
{
	ClearDC(0xffff00ff);
	int offY = 4;
	if (m_ebmResName.size() && g_resMgr().IsExist(m_ebmResName.c_str())) {
		CEmblemRes *emblem = (CEmblemRes *)g_resMgr().Get(m_ebmResName.c_str(), false);
		if (emblem) {
			DrawBitmap((m_w - 24) / 2, 0, emblem->GetBitmap(), true);
			offY = 28;
		}
	}

	int x = 4;
	for (std::vector<std::string>::iterator it = m_strings.begin(); it != m_strings.end(); ++it) {
		TextOutWithOutlineVertical(x, offY, it->c_str(), it->size(), m_fontColor, m_bgColor, 0, NameBalloonfontSize, NameBalloonfontBold);
		x += NameBalloonfontSize + 2;
	}
}

UIEditCtrl::UIEditCtrl()
{
	m_maxchar = 0xff;
	m_text.reserve(m_maxchar);
	m_selectionCursor = 0;
	m_selectionOrigin = 0;
	m_maskchar = 0;
	m_isSingColorFrame = 1;
	m_r = 0xff;
	m_g = 0xff;
	m_b = 0xff;
	m_textR = 0;
	m_textG = 0;
	m_textB = 0;
	m_xOffset = 0;
	m_yOffset = 0;
	m_type = 0;
}


UIEditCtrl::~UIEditCtrl() {}

void UIEditCtrl::DisableSelect(int cursorPos) {
	if (cursorPos > m_text.size()) cursorPos = m_text.size();

	m_selectionOrigin = m_selectionCursor = cursorPos;
}

void UIEditCtrl::OnLBtnDown(int x, int y) {
	g_windowMgr.SetFocusEdit(this);
}

void UIEditCtrl::SetText(char const *txt)
{
	m_text.assign(txt, txt + strlen(txt));
	SetTextByMaxChar();

	if (g_windowMgr.GetFocusEdit() == this) {
		g_language->ResetInput();
		g_language->SetInput(m_text.c_str());
	}
	MakeSelect();

	if (m_parent)
		m_parent->Invalidate();
}

void UIEditCtrl::MakeSelect() {
	if (g_windowMgr.GetFocusEdit() == this) {
		g_language->SetSelection(0, g_language->GetInputSize());
	} else {
		m_selectionOrigin = 0;
		m_selectionCursor = m_text.size();
	}
}

void UIEditCtrl::OnFinishEdit() {
	RefreshText();
	Invalidate();
}

void UIEditCtrl::OnBeginEdit()
{
	g_language->ResetInput();
	g_language->SetInput(m_text.c_str());
	g_language->HideText(m_maskchar != 0);
	MakeSelect();
}

void UIEditCtrl::SetTextColor(int r, int g, int b) {
	m_textR = r;
	m_textG = g;
	m_textB = b;
	Invalidate();
}

void UIEditCtrl::SetFrameColor(int r, int g, int b) {
	m_r = r;
	m_g = g;
	m_b = b;
	Invalidate();
}

int UIEditCtrl::GetTextSize() {
	if (g_windowMgr.GetFocusEdit() == this) {
		return g_language->GetInputSize();
	}
	return m_text.size();
}

const char* UIEditCtrl::GetText() {
	if (g_windowMgr.GetFocusEdit() == this) RefreshText();
	return m_text.c_str();
}

void UIEditCtrl::OnDraw()
{
	m_yOffset = 0;
	if (!m_isSingColorFrame) {
		int count = (m_w - 20) / 24;
		CBitmapRes* bitmap = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\type_dialog_l.bmp"), false);
		DrawBitmap(0, 0, bitmap, false);

		bitmap = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\type_dialog_m.bmp"), false);
		for (int i = 0; i < count; ++i)
			DrawBitmap(i * 24 + 10, 0, bitmap, false);

		bitmap = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\type_dialog_r.bmp"), false);
		DrawBitmap(count * 24 + 10, 0, bitmap, false);
		m_yOffset = 5;
	} else {
		ClearDC(RGB(m_r, m_g, m_b));
		m_yOffset = 2;
	}
	DrawEditText();
}

void UIEditCtrl::RefreshText()
{
	int prevCursor = m_selectionCursor;
	g_language->GetLanguageText(&m_text);
	g_language->GetSelection(&m_selectionOrigin, &m_selectionCursor);
	SetTextByMaxChar();

	bool changedSelection = false;
	if (m_selectionCursor > m_text.size() || m_selectionOrigin > m_text.size()) {
		m_selectionOrigin = m_selectionOrigin < m_text.size() ? m_selectionOrigin : m_text.size();
		m_selectionCursor = m_selectionCursor < m_text.size() ? m_selectionCursor : m_text.size();
		g_language->SetSelection(m_selectionOrigin, m_selectionCursor);
		changedSelection = true;
	}

	if (m_selectionCursor == 0) {
		m_xOffset = 0;
		return;
	}

	std::string text;

	if (g_language->GetSelectionSize() >= m_text.size())
		m_xOffset = 0;

	if (m_selectionCursor > prevCursor || changedSelection) {
		int left = 0;
		if (m_xOffset)
			left = GetTextWidth(m_text.c_str(), m_xOffset, 0, 12, 0);
		if (GetTextWidth(m_text.c_str(), m_selectionCursor, 0, 12, 0) + 3 - left > m_w - 12) {
			const char *textIter = CharNextExA(g_codePage, m_text.c_str(), 0);
			while (GetTextWidth(textIter, 0, 0, 12, 0) + 3 > m_w - 12)
				textIter = CharNextExA(g_codePage, textIter, 0);
			m_xOffset = (m_xOffset + m_selectionCursor - prevCursor < m_selectionCursor - strlen(textIter)) ? m_xOffset + m_selectionCursor - prevCursor : m_selectionCursor - strlen(textIter);
		}
	} else if (m_selectionCursor < prevCursor) {
		if (GetTextWidth(m_text.c_str(), m_selectionCursor, 0, 12, 0) + 3 < m_w - 12 && m_xOffset > m_selectionCursor)
			m_xOffset = m_xOffset - prevCursor + m_selectionCursor;
		m_xOffset = m_xOffset > 0 ? m_xOffset : 0;
	}
}

bool IsDrawCursor()
{
	if (g_windowMgr.isFocusChatWnd() && g_session.m_isBattle && !g_session.m_isBmChat) {
		return false;
	}
	return true;
}

void UIEditCtrl::SetTextByMaxChar()
{
	int size = m_text.size();
	if (size > m_maxchar) {
		char *textIter = CharNextExA(g_codePage, m_text.c_str(), 0);
		while ((unsigned)size - strlen(textIter) <= (unsigned)m_maxchar)
			textIter = CharNextExA(g_codePage, textIter, 0);
		textIter = CharPrevExA(g_codePage, m_text.c_str(), textIter, 0);
		m_text.resize(size - strlen(textIter));
		while (m_text.size() < m_maxchar)
			m_text += " ";
	}
}

void UIEditCtrl::UpdateCandidate(int pos) {
	UICandidateWnd* wnd = g_windowMgr.GetCandidateWnd();
	if (!wnd) return;
	int candx = pos, candy = m_h + 6;
	GetGlobalCoor(candx, candy);
	if (candx + wnd->GetWidth() > g_renderer->m_width)
		candx = g_renderer->m_width - wnd->GetWidth();
	if (candy + wnd->GetHeight() > g_renderer->m_height) {
		int dy = -12 - wnd->GetHeight();
		candy += dy;
		candy -= m_h;
	}
	wnd->Move(candx, candy);
}

void UIEditCtrl::UpdateKeyStroke(int pos) {
	UIKeyStrokeWnd* wnd = g_windowMgr.GetKeyStrokeWnd();
	if (!wnd) return;
	int candx = pos, candy = m_h + 6;
	GetGlobalCoor(candx, candy);
	if (candx + wnd->GetWidth() > g_renderer->m_width)
		candx = g_renderer->m_width - wnd->GetWidth();
	if (candy + wnd->GetHeight() > g_renderer->m_height) {
		int dy = -12 - wnd->GetHeight();
		candy += dy;
		candy -= m_h;
	}
	wnd->Move(candx, candy);
}

void UIEditCtrl::DrawEditText()
{
	int xOffset = 3;

	if (m_type == 0)
	{
		if (m_xOffset)
			xOffset = 3 - GetTextWidth(m_text.c_str(), m_xOffset, 0, 12, 0);
	}

	std::string text;
	unsigned int textColor = RGB(m_textR, m_textG, m_textB);

	if (g_windowMgr.GetFocusEdit() == this) {
		RefreshText();

		int start;
		int end;
		if (m_maskchar) {
			text.assign(m_text.size(), (char)m_maskchar);
			start = m_selectionOrigin < m_selectionCursor ? m_selectionOrigin : m_selectionCursor;
			end = m_selectionOrigin > m_selectionCursor ? m_selectionOrigin : m_selectionCursor;
		} else {
			const char *charsetString = g_language->GetLanguageCharset(false);
			text = charsetString + m_text;
			start = (m_selectionOrigin < m_selectionCursor ? m_selectionOrigin : m_selectionCursor) + strlen(charsetString);
			end = (m_selectionOrigin > m_selectionCursor ? m_selectionOrigin : m_selectionCursor) +
				strlen(charsetString) + g_language->GetCompositionStringSize();
		}

		int left = start ? GetTextWidth(text.c_str(), start, 0, 12, 0) : 0;
		int right = end ? GetTextWidth(text.c_str(), end, 0, 12, 0) : 0;

		if (g_language->GetSelectionSize()) {
			DrawSelectionBox(xOffset + left, right - left);
			TextOutA(xOffset, m_yOffset, text.c_str(), 0, 0, 12, textColor);
		} else if (g_language->GetCompositionStringSize()) {
			UpdateCandidate(left);
			UpdateKeyStroke(right);
			if ((timeGetTime() / 500 & 1) == 0) {
				DrawSelectionBox(left + xOffset, right - left > 2 ? right - left : 2);
			}
			TextOutA(xOffset, m_yOffset, text.c_str(), 0, 0, 12, textColor);
		} else {
			TextOutA(xOffset, m_yOffset, text.c_str(), 0, 0, 12, textColor);
			if ((timeGetTime() / 500 & 1) == 0 && IsDrawCursor()) TextOutA(left + xOffset - 2, m_yOffset, "|", 0, 0, 12, textColor);
			if (m_type) {
				if (m_parent)
					m_parent->Invalidate();
			}
		}
	} else {
		if (m_maskchar) text.assign(m_text.size(), (char)m_maskchar);
		else text.assign(m_text);
		TextOutA(xOffset, m_yOffset, text.c_str(), 0, 0, 12, textColor);
		if (m_type) {
			if (m_parent)
				m_parent->Invalidate();
		}
	}
}

void UIEditCtrl::DrawSelectionBox(int xOffset, int width) {
	if (xOffset < 3) {
		width = width + xOffset - 3;
		xOffset = 3;
	}
	if (xOffset >= m_w || width <= 0) return;
	if (xOffset + width > m_w) width = m_w - xOffset;
	unsigned int rr, gg, bb;
	g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
	DrawBox(xOffset, 0, width, m_h,
			((unsigned int)(unsigned char)bb << 16) |
			((unsigned int)(unsigned char)gg << 8) |
			(unsigned char)rr);
}

INT_PTR UIEditCtrl::SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4) {
	if (message) {
		UIWindow::SendMsg(sender, message, val1, val2, val3, val4);
		return 0;
	}
	UIWindow::SendMsg(sender, UM_DEFPUSH, val1, val2, val3, val4);
	g_language->ResetInput();
	return 0;
}

UIEditCtrl2::UIEditCtrl2() {}

UIEditCtrl2::~UIEditCtrl2() {}

INT_PTR UIEditCtrl2::SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4) {
	if (message) {
		UIWindow::SendMsg(sender, message, val1, val2, val3, val4);
	} else {
		if (g_windowMgr.IsLastFocusEdit(g_windowMgr.GetFocusEdit())) {
			g_windowMgr.SetFocusEditFirst();
		} else {
			g_windowMgr.SetFocusEditNext();
		}
		UIWindow::SendMsg(sender, UM_DEFPUSH, val1, val2, val3, val4);
	}
	return 0;
}

UIEditCtrl3::UIEditCtrl3()
{
	m_scrollBar = 0;
	m_scrEnabled = 0;
	m_itemSize = 0;
	m_lineLength = 0;
	m_curPos = 0;
	m_curPos2 = 0;
	m_curWidth = 0;
	m_prePos = 0;
	m_nextPos = 0;
	m_scrollOffset = 0;
	m_moveScroll = 0;
	m_moveUpCursor = 0;
	m_moveDownCursor = 0;
	m_cursorMove = 0;
	m_onShiftEnter = 0;
	m_itemSpacing = 14;
}

UIEditCtrl3::~UIEditCtrl3()
{}

void UIEditCtrl3::OnCreate(int cx, int cy) {
	m_scrollBar = new UIScrollBar3();
	m_scrollBar->Create(cx - 10, 0, 1, cy);
	AddChild(m_scrollBar);
}

void UIEditCtrl3::OnWheel(int wheel) {
	if (m_scrEnabled) {
		if (wheel < 0)
			SendMsg(m_scrollBar, UM_SCROLLVERT, 3, 0, 0, 0);
		else
			SendMsg(m_scrollBar, UM_SCROLLVERT, -3, 0, 0, 0);
	}
}

void UIEditCtrl3::Resize(int cx, int cy)
{
	UIWindow::Resize(cx, cy);
	m_scrollBar->Move(m_w - 10, 0);
	m_scrollBar->Resize(m_scrollBar->GetWidth(), cy);
	RecalcScrbarPos();
}

void UIEditCtrl3::OnKeyDown(unsigned int wParam, int lParam)
{
	m_prePos = 0;
	m_nextPos = 0;

	switch (wParam) {
	case VK_LEFT:
		if (m_yOffset > 0) {
			g_language->GetSelection(&m_selectionOrigin, &m_selectionCursor);
			int thisLineSize = 0;
			for (int i = 0; i < m_yOffset; ++i)
				thisLineSize += strlen(strings[i].c_str());
			if (thisLineSize == m_selectionCursor && enters[m_yOffset - 1])
				g_language->OnKeyDown(wParam, lParam);
		}
		g_language->OnKeyDown(wParam, lParam);
		return;

	case VK_RIGHT:
		g_language->GetSelection(&m_selectionOrigin, &m_selectionCursor);
		if ((unsigned int)m_yOffset < strings.size()) {
			int thisLineSize = 0;
			for (int i = 0; i <= m_yOffset; ++i)
				thisLineSize += strlen(strings[i].c_str());
			if (thisLineSize - 2 == m_selectionCursor && enters[m_yOffset])
				g_language->OnKeyDown(wParam, lParam);
		}
		g_language->OnKeyDown(wParam, lParam);
		return;

	case VK_INSERT:
	case VK_DELETE:
	case 'C':
	case 'V':
	case 'X':
		m_prePos = 0;
		m_nextPos = 0;
		g_language->OnKeyDown(wParam, lParam);
		return;

	case VK_HOME:
		m_prePos = 0;
		m_nextPos = 0;
		if (strings.size() > 0) {
			g_language->GetSelection(&m_selectionOrigin, &m_selectionCursor);
			int n = m_selectionCursor;
			for (int i = 0; i < m_yOffset; ++i)
				n -= strlen(strings[i].c_str());
			g_language->MoveUpSelection(n);
			g_language->GetSelection(&m_selectionOrigin, &m_selectionCursor);
			return;
		}
		break;

	case VK_END:
		m_prePos = 0;
		m_nextPos = 0;
		if (strings.size() > 0) {
			g_language->GetSelection(&m_selectionOrigin, &m_selectionCursor);
			int n = m_selectionCursor;
			for (int i = 0; i < m_yOffset; ++i)
				n -= strlen(strings[i].c_str());
			n = strlen(strings[m_yOffset].c_str()) - n;
			if (enters[m_yOffset])
				n -= 2;
			g_language->MoveDownSelection(n);
			g_language->GetSelection(&m_selectionOrigin, &m_selectionCursor);
		}
		m_cursorMove = 1;
		return;

	case VK_BACK:
		if (strings.size() > 0) {
			g_language->GetSelection(&m_selectionOrigin, &m_selectionCursor);
			int thisLineSize = 0;
			for (int i = 0; i < m_yOffset; ++i)
				thisLineSize += strlen(strings[i].c_str());
			if (thisLineSize == m_selectionCursor && enters[m_yOffset - 1])
				g_language->OnChar(VK_BACK, lParam);
		}
		break;

	}
}

void UIEditCtrl3::OnDraw() {
	ClearDC(((unsigned int)(unsigned char)m_b << 16) | ((unsigned int)(unsigned char)m_g << 8) | (unsigned char)m_r);
	DrawEditText();
	if (m_parent)
		m_parent->Invalidate();
}

INT_PTR UIEditCtrl3::SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch (message) {
	case WM_KEYDOWN:
		m_moveScroll = 0;
		OnKeyDown(val1, val2);
		return 3;

	case UM_UPPUSH:
		m_cursorMove = 0;
		if (strings.size() > 1 || (strings.size() == 1 || (strings.size() > 1 && m_yOffset == 0)) && enters[0]) {
			g_language->GetSelection(&m_selectionOrigin, &m_selectionCursor);
			int n;
			if (m_prePos > 0 && m_nextPos == m_selectionCursor && !m_moveUpCursor)
				n = m_prePos;
			else {
				int line = 0;
				int lineW = 0;
				m_moveDownCursor = 0;
				const char *msg = m_upLineText;
				const char *msg_end = msg + strlen(msg);
				if (m_curPos) {
					while (msg < msg_end) {
						const char *msg_old = msg;
						msg = g_language->GetWordBreak(msg, msg_end - msg);
						int len = msg - msg_old;
						int tempW = lineW;
						lineW += GetTextWidth(msg_old, len, 0, 12, false);
						line += len;
						if (len == 1) {
							if (*msg_old == '\n') {
								--line;
								break;
							}
						}
						if (lineW > m_curWidth) {
							if (lineW - m_curWidth > m_curWidth - tempW)
								line -= len;
							break;
						}
					}
				}
				m_prePos = m_curPos - line + strlen(m_upLineText);
				n = m_prePos;
			}
			g_language->MoveUpSelection(n);
			m_moveUpCursor = 1;
			m_cursorMove = 1;
			g_language->GetSelection(&m_selectionOrigin, &m_selectionCursor);
			m_nextPos = m_selectionCursor;
		}
		break;

	case UM_DOWNPUSH:
		m_cursorMove = 0;
		if ((strings.size() > 1 && (unsigned int)m_yOffset < strings.size() - 1) ||
			(strings.size() == 1 && enters[0] && m_curPos2 > 0)) {
			g_language->GetSelection(&m_selectionOrigin, &m_selectionCursor);
			int n;
			if (m_prePos > 0 && m_nextPos == m_selectionCursor && !m_moveDownCursor)
				n = m_prePos;
			else {
				m_moveUpCursor = 0;
				int line = 0;
				int lineW = 0;
				if (strings.size() > 1 && (unsigned int)m_yOffset < strings.size() - 1) {
					const char *msg = m_downLineText;
					const char *msg_end = msg + strlen(msg);
					while (msg < msg_end) {
						const char *msg_old = msg;
						msg = g_language->GetWordBreak(msg, msg_end - msg);
						int len = msg - msg_old;
						int tempW = lineW;
						lineW += GetTextWidth(msg_old, len, 0, 12, false);
						line += len;
						if (len == 1) {
							if (*msg_old == '\n') {
								--line;
								break;
							}
						}
						if (lineW > m_curWidth) {
							if (lineW - m_curWidth > m_curWidth - tempW)
								line -= len;
							break;
						}
					}
				}
				m_prePos = m_curPos2 + line;
				n = m_prePos;
			}
			g_language->MoveDownSelection(n);
			m_moveDownCursor = 1;
			g_language->GetSelection(&m_selectionOrigin, &m_selectionCursor);
			m_nextPos = m_selectionCursor;
		}
		break;

	case UM_SCROLLVERT:
	case UM_SCROLLVERTPAGEUP:
	case UM_SCROLLVERTPAGEDOWN:
		{
		int oldOffset = m_scrollOffset;
		m_moveScroll = 1;
		if (message == 9)
			m_scrollOffset = oldOffset - m_h / m_itemSpacing + 1;
		else if (message == 10)
			m_scrollOffset = oldOffset + m_h / m_itemSpacing - 1;
		else
			m_scrollOffset = oldOffset + val1;

		if (m_scrollOffset < 0)
			m_scrollOffset = 0;
		if (m_scrollOffset > m_itemSize - m_h / m_itemSpacing)
			m_scrollOffset = m_itemSize - m_h / m_itemSpacing;
		m_scrollBar->SetPos(m_scrollOffset);
		m_scrollBar->Invalidate();
		Invalidate();
		if (m_parent)
			m_parent->Invalidate();
		return m_scrollOffset - oldOffset;
		}

	case UM_DEFPUSH:
		if (m_onShiftEnter && HIBYTE(GetKeyState(VK_SHIFT)) || !m_onShiftEnter) {
			char tmp[3];
			tmp[0] = '\r';
			tmp[1] = '\n';
			tmp[2] = 0;
			g_language->AddInput2(tmp);
		} else {
			UIWindow::SendMsg(sender, UM_DEFPUSH, val1, val2, val3, val4);
		}
		break;

	default:
		return UIEditCtrl::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

void UIEditCtrl3::SetTextByMaxChar()
{
	int size = m_text.size();
	if (size > m_maxchar) {
		char *textIter = CharNextExA(g_codePage, m_text.c_str(), 0);
		while ((unsigned)size - strlen(textIter) <= (unsigned)m_maxchar)
			textIter = CharNextExA(g_codePage, textIter, 0);
		textIter = CharPrevExA(g_codePage, m_text.c_str(), textIter, 0);
		m_text.resize(size - strlen(textIter));

		g_language->ResetInput();
		g_language->SetInput(m_text.c_str());

		while (m_text.size() < m_maxchar)
			m_text += " ";
	}
}

void UIEditCtrl3::RecalcScrbarPos()
{
	int parentnumCanDisplayItem = m_h / m_itemSpacing;
	int range = m_itemSize - parentnumCanDisplayItem;
	if (range == 1 && !m_scrollOffset)
		range = 0;
	if (range < 0)
		range = 0;
	if (range != 0)
		m_scrEnabled = 1;
	else
		m_scrEnabled = 0;

	if (m_scrEnabled) {
		m_scrollBar->SetRange(range, parentnumCanDisplayItem, m_itemSize);
		m_scrollBar->SetPos(m_scrollOffset);
	} else {
		m_scrollBar->SetRange(0, parentnumCanDisplayItem, m_itemSize);
		m_scrollOffset = 0;
	}
	m_scrollBar->Invalidate();
}

void UIEditCtrl3::DrawEditText()
{
	m_x = m_x;
	int xOffset = 3;
	if (!m_type) {
		if (m_xOffset)
			xOffset = 3 - GetTextWidth(m_text.c_str(), m_xOffset, 0, 12, false);
	}

	std::string text;
	unsigned int textColor = RGB(m_textR, m_textG, m_textB);
	if (g_windowMgr.GetFocusEdit() == this)
		RefreshText();

	const char *charsetString = g_language->GetLanguageCharset(false);
	text = charsetString + m_text;

	int w = m_w - 6;
	if (m_scrEnabled)
		w += 3 - m_scrollBar->GetWidth();

	strings.clear();
	enters.clear();
	PolishText3(text.c_str(), strings, enters, w);

	m_itemSize = strings.size();
	if (m_onShiftEnter)
		++m_itemSize;

	if (strings.size() <= 0) {
		if ((timeGetTime() / 500 & 1) == 0 && IsDrawCursor())
			TextOutA(1, 0, "|", 0, 0, 12, textColor);
		return;
	}

	int start;
	int yOffset;
	int nItem = m_h / m_itemSpacing;
	int firstItem = m_scrollOffset;
	int lastItem = (unsigned int)(m_scrollOffset + nItem) < strings.size() ? m_scrollOffset + nItem : strings.size();

	int thisLineSize = 0;
	int i;
	for (i = 0; i < firstItem; ++i)
		thisLineSize += strlen(strings[i].c_str());

	for (i = firstItem; i < lastItem; ++i) {
		yOffset = (i - firstItem) * m_itemSpacing;
		thisLineSize += strlen(strings[i].c_str());

		if (g_windowMgr.GetFocusEdit() == this && g_language->GetSelectionSize()) {
			start = m_selectionOrigin < m_selectionCursor ? m_selectionOrigin : m_selectionCursor;
			int e = m_selectionOrigin > m_selectionCursor ? m_selectionOrigin : m_selectionCursor;
			int left = 0;
			int right = 0;

			if (start < thisLineSize) {
				int n = start - (thisLineSize - strlen(strings[i].c_str()));
				if (n < 0)
					n = 0;
				left = n ? GetTextWidth(strings[i].c_str(), n, 0, 12, false) : 0;
				int e = strlen(strings[i].c_str());
				if (enters[i])
					e -= 2;
				right = GetTextWidth(strings[i].c_str(), e, 0, 12, false);
			}
			if (e <= thisLineSize) {
				int n = e - (thisLineSize - strlen(strings[i].c_str()));
				if (n < 0)
					n = 0;
				if (strlen(strings[i].c_str()) == n && enters[i])
					n -= 2;
				right = n ? GetTextWidth(strings[i].c_str(), n, 0, 12, false) : 0;
			}

			DrawSelectionBox(left + xOffset, yOffset, right - left);
		}

		int len = strlen(strings[i].c_str());
		char *buf = new char[len + 1];
		strcpy(buf, strings[i].c_str());
		buf[len] = 0;

		if (len >= 2 && buf[len - 2] == '\r')	// a line of one character has no "\r\n" to cut
			buf[len - 2] = 0;

		TextOutA(xOffset, yOffset, buf, 0, 0, 12, textColor);
#if defined(BF_PORT)
		delete[] buf;	// never freed: a line's copy leaked on every draw of a multi-line field (M8.6 soak)
#endif
	}

	start = strlen(charsetString) + m_selectionCursor;
	int end = strlen(charsetString) + g_language->GetCompositionStringSize() +
		(m_selectionOrigin > m_selectionCursor ? m_selectionOrigin : m_selectionCursor);
	int n = start;
	for (int line = 0; line < strings.size(); ++line) {
		n -= strlen(strings[line].c_str());

		m_yOffset = line;

		if (n < 0)
			break;

		if (m_cursorMove && n == 0)
			break;
	}

	if (g_windowMgr.GetFocusEdit() == this) {
		if (m_lineLength != strlen(strings[m_yOffset].c_str())) {
			m_moveScroll = 0;
			m_lineLength = strlen(strings[m_yOffset].c_str());
		}

		thisLineSize = 0;
		for (int i = 0; i < m_yOffset; ++i)
			thisLineSize += strlen(strings[i].c_str());

		int cursorInLine = start - thisLineSize;
		int n = 0;
		if (cursorInLine < 0)
			cursorInLine = start;

		if (strlen(strings[m_yOffset].c_str()) == cursorInLine && enters[m_yOffset]) {
			cursorInLine = 0;
			n = 1;
		}

		if (m_curPos2 != strlen(strings[m_yOffset].c_str()) - cursorInLine)
			m_curPos2 = strlen(strings[m_yOffset].c_str()) - cursorInLine;
		if (m_curPos != cursorInLine)
			m_curPos = cursorInLine;

		int cursorWidth = cursorInLine ? GetTextWidth(strings[m_yOffset].c_str(), cursorInLine, 0, 12, false) : 0;
		int endInLine = end - thisLineSize;
		if (endInLine < 0)
			endInLine = end;
		if (strlen(strings[m_yOffset].c_str()) == endInLine && enters[m_yOffset])
			endInLine -= 2;
		int endWidth = endInLine ? GetTextWidth(strings[m_yOffset].c_str(), endInLine, 0, 12, false) : 0;

		if (g_language->GetCompositionStringSize()) {
			int width = __max(endWidth - cursorWidth, 2);
			cursorWidth += 2 + width;
			m_curPos += 2;
			UpdateCandidate(cursorWidth);
			UpdateKeyStroke(endWidth);
		}

		if (m_curWidth != cursorWidth)
			m_curWidth = cursorWidth;

		if ((timeGetTime() / 500 & 1) == 0 && IsDrawCursor()) {
			TextOutA(cursorWidth - 2 + xOffset, (m_yOffset - firstItem + n) * m_itemSpacing,
				"|", 0, 0, 12, textColor);

			if (!m_moveScroll) {
				if (m_moveUpCursor) {
					if (m_yOffset - n < m_scrollOffset) {
						if (m_yOffset >= m_itemSize - nItem)
							m_scrollOffset = m_itemSize - nItem;
						else
							m_scrollOffset = m_yOffset + n;
					}
				} else if (m_moveDownCursor) {
					if (m_yOffset > m_scrollOffset + nItem - 1) {
						if (m_yOffset < nItem)
							m_scrollOffset = 0;
						else
							m_scrollOffset = m_yOffset - nItem + 1;
					}
				} else {
					if (m_itemSize <= nItem)
						m_scrollOffset = m_yOffset;
					else
						m_scrollOffset = m_itemSize - nItem + n;
					if (m_onShiftEnter && m_scrollOffset > 0)
						--m_scrollOffset;
				}
			}
		}

		if (n)
			++m_yOffset;
	}

	if (strings.size() > 1 && m_yOffset > 0) {
		int len = strlen(strings[m_yOffset - 1].c_str());
		m_upLineText = new char[len + 1];
		strcpy(m_upLineText, strings[m_yOffset - 1].c_str());
	} else if ((strings.size() == 1 || (strings.size() > 1 && m_yOffset == 0)) && enters[0]) {
		int len = strlen(strings[0].c_str());
		m_upLineText = new char[len + 1];
		strcpy(m_upLineText, strings[0].c_str());
	}

	if (strings.size() > 1 && (unsigned int)m_yOffset < strings.size() - 1) {
		int len = strlen(strings[m_yOffset + 1].c_str());
		m_downLineText = new char[len + 1];
		strcpy(m_downLineText, strings[m_yOffset + 1].c_str());
	} else if (strings.size() == 1 && enters[0]) {
		int len = strlen(strings[0].c_str());
		m_downLineText = new char[len + 1];
		strcpy(m_downLineText, strings[0].c_str());
	}

	RecalcScrbarPos();
}

void UIEditCtrl3::DrawSelectionBox(int xOffset, int yOffset, int width) {
	if (xOffset < 3) {
		width = width + xOffset - 3;
		xOffset = 3;
	}
	if (xOffset >= m_w || width <= 0) return;
	if (xOffset + width > m_w) width = m_w - xOffset;
	unsigned int rr, gg, bb;
	g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
	DrawBox(xOffset, yOffset, width, 14,
			((unsigned int)(unsigned char)bb << 16) |
			((unsigned int)(unsigned char)gg << 8) |
			(unsigned char)rr);
}

void UIBitmapEditCtrl::SetBitmap(char const *bitmapName)
{
	m_bitmapName = bitmapName;
	CBitmapRes* bitmap = (CBitmapRes*)g_resMgr().Get(UIBmp(m_bitmapName.c_str()), false);
	m_w = bitmap->m_width;
	m_h = bitmap->m_height;
}

void UIBitmapEditCtrl::OnDraw() {
	ClearDC(-1);
	CBitmapRes* bm = (CBitmapRes*)g_resMgr().Get(m_bitmapName.c_str(), false);
	DrawBitmap(0, 0, bm, false);
	m_yOffset = 3;
	DrawEditText();
}

UIWhisperEditCtrl::UIWhisperEditCtrl() {
	m_curHis = 0;
}

void UIWhisperEditCtrl::OnBeginEdit()
{
	UIEditCtrl::OnBeginEdit();
	MakeSelect();
}

void UIWhisperEditCtrl::SetText(char const *txt)
{
	UIEditCtrl::SetText(txt);
	if (strlen(txt) == 0) m_curHis = m_hisChat.size();
}

void UIWhisperEditCtrl::StoreInHistory()
{
	if (GetTextSize() == 0) {
		m_curHis = m_hisChat.size();
		return;
	}

	for (int i = 0; i < m_hisChat.size(); ++i) {
		if (m_hisChat[i] == GetText())
			return;
	}

	if (m_curHis == m_hisChat.size()) {
		if (m_hisChat.size() >= 30) {
			--m_curHis;
			m_hisChat.erase(m_hisChat.begin());
		}
		m_hisChat.push_back(GetText());
	} else {
		bool found = false;
		int i;
		for (i = 0; i < m_hisChat.size(); ++i) {
			if (m_hisChat[i] == GetText()) {
				found = true;
				break;
			}
		}
		if (!found) {
			m_hisChat.push_back(GetText());
			m_curHis = m_hisChat.size() - 1;
		} else
			m_curHis = i;
	}

	if (m_curHis == m_hisChat.size())
		SetText("");
	else
		SetText(m_hisChat[m_curHis].c_str());
}

void UIWhisperEditCtrl::OnFinishEdit() {
	StoreInHistory();
	UIEditCtrl::OnFinishEdit();
}

INT_PTR UIWhisperEditCtrl::SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch (message) {
	case UM_UPPUSH:
		if (m_hisChat.size() == 0)
			return 0;
		if (m_curHis != 0)
			--m_curHis;
		SetText(m_hisChat[m_curHis].c_str());
		OnBeginEdit();
		break;
	case UM_DOWNPUSH:
		if (m_hisChat.size() == 0) return 0;
		if (m_curHis >= m_hisChat.size()) return 0;
		++m_curHis;
		if (m_curHis == m_hisChat.size())
			SetText("");
		else
			SetText(m_hisChat[m_curHis].c_str());
		OnBeginEdit();
		break;
	default:
		UIEditCtrl::SendMsg(sender, message, val1, val2, val3, val4);
		break;
	}

	return 0;
}

void UIEditCtrlVarId::SetText(char const *txt)
{
	UIEditCtrl::SetText(txt);
	Invalidate();
}

void UIEditCtrlVarId::OnWheel(int wheel) {
	if (m_parent)
		m_parent->OnWheel(wheel);
}

void UIEditCtrlVarId::OnFinishEdit() {
	UIEditCtrl::OnFinishEdit();
	SendMsg(this, UM_DEFPUSH, 0, 0, 0, 0);
}

INT_PTR UIEditCtrlVarId::SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4) {
	if (message) {
		UIWindow::SendMsg(sender, message, val1, val2, val3, val4);
		return 0;
	}
	if (m_parent) {
		m_parent->SendMsg(this, UM_COMMAND, m_id, 0, 0, 0);
	}
	return 0;
}


UIChatEditCtrl::UIChatEditCtrl()
{
	m_curHis = 0;
	m_hisChat.reserve(30);
}

void UIChatEditCtrl::OnLBtnDown(int x, int y) {
	g_windowMgr.SetFocusEdit(this);
	m_parent->OnLBtnDown(m_x + x, m_y + y);
}

void UIChatEditCtrl::StoreInHistory()
{
	if (GetTextSize()) {
		if (m_curHis == m_hisChat.size()) {
			if (m_hisChat.size() >= 30) {
				--m_curHis;
				m_hisChat.erase(m_hisChat.begin());
			}
			m_hisChat.push_back(GetText());
		} else {
			m_hisChat[m_curHis] = GetText();
		}

		++m_curHis;
		if (m_curHis == m_hisChat.size())
			SetText("");
		else
			SetText(m_hisChat[m_curHis].c_str());
	}
}

INT_PTR UIChatEditCtrl::SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch (message) {
	case UM_UPPUSH:
		if (m_hisChat.size() == 0)
			break;
		if (m_curHis == m_hisChat.size())
			m_lastEditText = GetText();
		if (m_curHis != 0)
			--m_curHis;
		SetText(m_hisChat[m_curHis].c_str());
		OnBeginEdit();
		break;
	case UM_DOWNPUSH:
		if (m_hisChat.size() == 0)
			break;
		if (m_curHis < m_hisChat.size()) {
			++m_curHis;
			if (m_curHis == m_hisChat.size())
				SetText(m_lastEditText.c_str());
			else
				SetText(m_hisChat[m_curHis].c_str());
			OnBeginEdit();
		}
		break;
	default:
		return UIEditCtrl::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

UIResizer::UIResizer() {
	m_type = (RESIZERTYPE)0;
	m_parentWidth = 0;
	m_parentHeight = 0;
	m_color = 0xffff00ff;
	m_backColor = 0xffff00ff;
}

void UIResizer::OnDraw() {
	if (m_bitmapName.size() != 0) {
		CBitmapRes* bm = (CBitmapRes*)g_resMgr().Get(UIBmp(m_bitmapName.c_str()), false);
		DrawBitmap(0, 0, bm, false);
	} else {
		ClearDC(m_backColor);
		DrawBox(0, 0, m_w, 2, m_color);
	}
}

void UIResizer::SetType(RESIZERTYPE type) { m_type = type; }

void UIResizer::SetBitmap(char const *bitmapName)
{
	m_bitmapName = bitmapName;
	CBitmapRes *bitmap;
	if (g_serviceType == ServiceKorea)
		bitmap = (CBitmapRes *)g_resMgr().Get(m_bitmapName.c_str(), false);
	else
		bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp(m_bitmapName.c_str()), false);
	m_w = bitmap->m_width;
	m_h = bitmap->m_height;
}

void UIResizer::SetColor(unsigned int color, unsigned int backColor) {
	Invalidate();
	m_color = color;
	m_backColor = backColor;
}

void UIResizer::OnLBtnUp(int x, int y) {
	if (g_windowMgr.GetCapture() == this) {
		if (m_parent)
			m_parent->RefreshSnap();
		g_windowMgr.ReleaseCapture();
	}
}

void UIResizer::OnLBtnDown(int x, int y) {
	g_windowMgr.SetCapture(this);
	m_startX = x;
	m_startY = y;
	if (m_parent) {
		m_parentWidth = m_parent->GetWidth();
		m_parentHeight = m_parent->GetHeight();
	}
	GetGlobalCoor(m_startX, m_startY);
}

void UIResizer::OnMouseHover(int x, int y) {}

void UIResizer::OnMouseMove(int x, int y)
{
	if (g_windowMgr.GetCapture() == this) {
		int gx = x, gy = y;
		GetGlobalCoor(gx, gy);
		if (m_parent) {
			int dx = gx - m_startX;
			int dy = gy - m_startY;
			int width = m_parentWidth;
			int height = m_parentHeight;
			switch (m_type) {
			case RT_LEFT:
				width -= dx;
				height = m_parentHeight;
				break;
			case RT_RIGHT:
				width += dx;
				height = m_parentHeight;
				break;
			case RT_UP:
				width = m_parentWidth;
				height -= dy;
				break;
			case RT_DOWN:
				width = m_parentWidth;
				height += dy;
				break;
			case RT_UPLEFT:
				width -= dx;
				height -= dy;
				break;
			case RT_UPRIGHT:
				width += dx;
				height -= dy;
				break;
			case RT_DOWNLEFT:
				width -= dx;
				height += dy;
				break;
			case RT_DOWNRIGHT:
				width += dx;
				height += dy;
				break;
			}

			m_parent->SendMsg(this, UM_RESIZE, m_type, width, height, 0);
		}
	}
}

void UIResizer::OnMouseShape(int x, int y) {
	if (g_windowMgr.GetCapture() == this) {
		g_modeMgr.GetCurMode()->SetCursorAction(2);
		return;
	}
	if (x >= 0 && x < m_w && y >= 0 && y <= m_h) {
		g_modeMgr.GetCurMode()->SetCursorAction(2);
		return;
	}
	if (g_windowMgr.GetCapture() != this) {
		CMode* mode = g_modeMgr.GetCurMode();
		mode->SetCursorAction(0);
	}
}

UITab::UITab() {
	m_state = 0;
	m_typeTab = 0;
	m_numTab = 3;
}

void UITab::OnLBtnDown(int x, int y) {
	if (m_parent) {
		int idx = y / (m_h / m_numTab);
		if (idx < 0) idx = 0;
		if (idx > m_numTab - 1)
			idx = m_numTab - 1;
		if (m_state != idx) {
			m_state = idx;
			Invalidate();
			m_parent->SendMsg(this, UM_TAB, m_state, 0, 0, 0);
		}
	}
}

INT_PTR UITab::SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4) {
	if (message == 13) {
		m_state = val1;
		Invalidate();
	}
	return 0;
}


void UITab::OnDraw()
{
	if (m_typeTab == 0) {
		char bitmapName[128];
		sprintf(bitmapName, "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\tab_itm_%02d.bmp", m_state + 1);
		CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp(bitmapName), false);
		DrawBitmap(0, 0, bmp, false);
	} else if (m_typeTab == 1) {
		char bitmapName[128];
		sprintf(bitmapName, "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\tab_cmd_%02d.bmp", m_state + 1);
		CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp(bitmapName), false);
		DrawBitmap(0, 0, bmp, false);
	} else if (m_typeTab == 2) {
		char bitmapName[128];
		sprintf(bitmapName, "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\tab_que_%02d.bmp", m_state + 1);
		CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp(bitmapName), false);
		DrawBitmap(0, 0, bmp, false);
	}
}

UIRadioBtn::UIRadioBtn() {
	m_state = 0;
	m_type = 0;
	m_isEnabled = 1;
	m_id = 0x95;
}

UIRadioBtn::~UIRadioBtn() {}

void UIRadioBtn::SetText(char const *text)
{
	m_text = text;
	Invalidate();
}

// The auction window (UIAuctionWnd::OnCreate, 0x49B298) writes m_type = 1
// in place, an inlined setter.
void UIRadioBtn::SetType(int type)
{
	m_type = type;
}

void UIRadioBtn::OnLBtnDown(int x, int y) {
	if (m_parent && m_isEnabled) {
		m_parent->SendMsg(this, UM_COMMAND, m_id, 0, 0, 0);
	}
}

void UIRadioBtn::OnDraw()
{
	ClearDC(0xffff00ff);
	if (m_state == 0) {
		CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\radiobtn_off.bmp"), false);
		DrawBitmap(0, 0, bmp, false);
	} else {
		if (m_type) {
			CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\radiobtn_on1.bmp"), false);
			DrawBitmap(0, 0, bmp, false);
		} else {
			CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\radiobtn_on.bmp"), false);
			DrawBitmap(0, 0, bmp, false);
		}
	}
	if (m_isEnabled) {
		TextOutA(13, 0, m_text.c_str(), 0, 0, 12, 0);
	} else {
		TextOutA(13, 0, m_text.c_str(), 0, 0, 12, 0xb5b5b5);
	}
}

INT_PTR UIRadioBtn::SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4) {
	if (message == 13) {
		m_state = val1;
		Invalidate();
	}
	return 0;
}

UIChatRoomTitle::UIChatRoomTitle()
{}

UIChatRoomTitle::~UIChatRoomTitle()
{}

void UIChatRoomTitle::OnCreate(int cx, int cy)
{
	m_title = new UIStaticText;
	m_title->Create(m_w - 34, 15);
	m_title->Move(31, 8);
	m_title->SetTextWithShorten("");
	m_title->SetFont(0, 12);
	AddChild(m_title);
}

void UIChatRoomTitle::OnMouseShape(int x, int y)
{
	if (x >= 0 && x < m_w && y >= 0 && y <= m_h) {
		g_modeMgr.GetCurMode()->SetCursorAction(2);
		return;
	}
	CMode* mode = g_modeMgr.GetCurMode();
	mode->SetCursorAction(0);
}

void UIChatRoomTitle::OnLBtnDblClk(int x, int y) {
	CGameMode* gm = g_modeMgr.GetGameMode();
	switch (m_roomInfo.roomType) {
	case 0: {
		UIFrameWnd* w = g_windowMgr.MakeWindow(WID_PASSWORDWND);
		w->SendMsg(0, UM_SETROOMID, m_roomInfo.roomId, 0, 0, 0);
		break;
	}
	case 3:
		break;
	default:
		gm->SendMsg(MM_REQ_ENTER_ROOM, m_roomInfo.roomId, 0, 0);
		break;
	}
}

void UIChatRoomTitle::OnDraw()
{
	ClearDC(0xffff00ff);

	CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_lu.bmp"), false);
	DrawBitmap(0, 0, bmp, false);

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_ru.bmp"), false);
	DrawBitmap(m_w - bmp->m_width, 0, bmp, false);

	int count = (m_w - 24) / 12 + ((m_w - 24) % 12 != 0);
	int i;
	for (i = 0; i < count; i++) {
		bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_mu.bmp"), false);
		DrawBitmap(i * 12 + 12, 0, bmp, false);
	}

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_ld.bmp"), false);
	DrawBitmap(0, m_h - bmp->m_height - 6, bmp, false);

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_rd.bmp"), false);
	DrawBitmap(m_w - bmp->m_width, m_h - bmp->m_height - 6, bmp, false);

	count = (m_w - 24) / 12 + ((m_w - 24) % 12 != 0);
	for (i = 0; i < count; i++) {
		bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_md.bmp"), false);
		DrawBitmap(i * 12 + 12, m_h - bmp->m_height - 6, bmp, false);
	}

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_arr_l.bmp"), false);
	DrawBitmap(m_w / 2 - bmp->m_width / 2, m_h - bmp->m_height, bmp, false);

	if (!m_roomInfo.roomType) {
		bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\chat_close.bmp"), false);
	} else {
		bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\chat_open.bmp"), false);
	}
	DrawBitmap(3, 3, bmp, true);
}

INT_PTR UIChatRoomTitle::SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch (message) {
	case 6:
		break;
	case 0x22: {
		m_roomInfo = *(ChatRoomInfo *)val1;

		std::string typeString;
		char title[384];
		if (m_roomInfo.roomType == 0) {
			typeString = MsgStr(MSI_ROOM_PRIVATE);
			sprintf(title, "%s (%d/%d)", m_roomInfo.title.c_str(), m_roomInfo.numPeople, m_roomInfo.maxNumPeople);
		} else if (m_roomInfo.roomType == 3) {
			typeString = "";
			sprintf(title, "%s", m_roomInfo.title.c_str());
		} else {
			typeString = MsgStr(MSI_ROOM_PUBLIC);
			sprintf(title, "%s (%d/%d)", m_roomInfo.title.c_str(), m_roomInfo.numPeople, m_roomInfo.maxNumPeople);
		}

		m_shopfont = (short)val2;
		m_title->SetFont(0, 12);
		m_title->SetTextWithShorten(title);
		Invalidate();
		break;
	}
	default:
		return UIWindow::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

UIMerchantShopTitle::UIMerchantShopTitle() {}

UIMerchantShopTitle::~UIMerchantShopTitle() {}

void UIMerchantShopTitle::OnCreate(int cx, int cy)
{
	m_title = new UIStaticText;
	m_title->Create(m_w - 34, 15);
	m_title->Move(31, 8);
	m_title->SetTextWithShorten("");
	m_title->SetFont(0, 12);
	AddChild(m_title);
}

void UIMerchantShopTitle::OnLBtnDblClk(int x, int y) {
	CGameMode* gameMode = g_modeMgr.GetGameMode();
	gameMode->SendMsg(MM_REQ_BUY_FROMMC, m_shopOwnerAid, 0, 0);
}

void UIMerchantShopTitle::OnLBtnDown(int x, int y) {
	if (g_session.m_isShopping == 1) {
		CGameMode* gameMode = g_modeMgr.GetGameMode();
		gameMode->SendMsg(MM_REQ_BUY_FROMMC, m_shopOwnerAid, 0, 0);
	}
}

void UIMerchantShopTitle::OnDraw()
{
	ClearDC(0xffff00ff);

	CBitmapRes* bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_lu.bmp"), false);
	DrawBitmap(0, 0, bmp, false);

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_ru.bmp"), false);
	DrawBitmap(m_w - bmp->m_width, 0, bmp, false);

	int count = (m_w - 24) / 12 + ((m_w - 24) % 12 != 0);
	int i;
	for (i = 0; i < count; i++) {
		bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_mu.bmp"), false);
		DrawBitmap(i * 12 + 12, 0, bmp, false);
	}

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_ld.bmp"), false);
	DrawBitmap(0, m_h - bmp->m_height - 6, bmp, false);

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_rd.bmp"), false);
	DrawBitmap(m_w - bmp->m_width, m_h - bmp->m_height - 6, bmp, false);

	count = (m_w - 24) / 12 + ((m_w - 24) % 12 != 0);
	for (i = 0; i < count; i++) {
		bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_md.bmp"), false);
		DrawBitmap(i * 12 + 12, m_h - bmp->m_height - 6, bmp, false);
	}

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\sysbox_arr_l.bmp"), false);
	DrawBitmap(m_w / 2 - bmp->m_width / 2, m_h - bmp->m_height, bmp, false);

	bmp = (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\shop.bmp"), false);
	DrawBitmap(3, 3, bmp, true);
}

void UIMerchantShopTitle::OnMouseShape(int x, int y)
{
	if (x >= 0 && x < m_w && y >= 0 && y <= m_h) {
		g_modeMgr.GetCurMode()->SetCursorAction(2);
		return;
	}
	CMode* mode = g_modeMgr.GetCurMode();
	mode->SetCursorAction(0);
}

void UIMerchantShopTitle::OnRBtnDown(int x, int y) {
	if (g_session.m_isShopping == 1) {
		CMode* curMode = g_modeMgr.GetCurMode();
		curMode->SendMsg(MM_RELEASE_QUEST_SH, 0, 0, 0);
		g_session.ClearMerchantDealItems();
	}
}

INT_PTR UIMerchantShopTitle::SendMsg(UIWindow *sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch (message) {
	case UM_COMMAND:
		break;
	case UM_SETNAID:
		m_shopOwnerAid = val1;
		break;
	case UM_SETINFO:
		{
			std::string shopTitle = (char *)val1;
			char title[384];
			sprintf(title, "%s", shopTitle.c_str());

			m_shopfont = (short)val2;
			m_title->SetFont(0, 12);
			m_title->SetTextWithShorten(title);

			short x = 116 - (short)(GetTextWidth(title, 0, 0, m_title->GetHeight(), false) / 2);
			if (x < 31)
				x = 31;
			m_title->Move(x, 8);
			Invalidate();
		}
		break;
	default:
		return UIWindow::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}

UIEmblemWnd::UIEmblemWnd()
{
	m_masterAid = 0;
	m_fileName = "";
	Create(24, 24);
}


UIEmblemWnd::~UIEmblemWnd() {}

INT_PTR UIEmblemWnd::SendMsg(UIWindow * sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	char buf[256];
	if (message == 0x22) {
		m_masterAid = val1;
		g_session.GetEmblemFileName(buf, val2, val3);
		m_fileName = buf;
	}
	return 0;
}


void UIEmblemWnd::OnProcess()
{
	CGameActor* actor = g_session.GetGameActorByAid(m_masterAid);
	if (!actor)
		return;

	int nameX = actor->m_lastTlvertX;
	int nameY = actor->m_lastTlvertY;
	if (m_posx == nameX && m_posx == nameY)
		return;

	m_posx = nameX;
	m_posy = nameY;
	Move(
		(int)(nameX - (g_renderer->m_width * 5 / 640) * actor->m_lastPixelRatio),
		(int)(nameY - (g_renderer->m_height * 100 / 480) * actor->m_lastPixelRatio));
}

void UIEmblemWnd::OnDraw() {
	if (!m_fileName.length() || !g_resMgr().IsExist(m_fileName.c_str())) return;
	CEmblemRes* emblem = (CEmblemRes*)g_resMgr().Get(m_fileName.c_str(), 0);
	if (!emblem) return;
	if (!g_session.IsSiegeMode()) return;
	DrawBitmap(0, 0, emblem->GetBitmap(), true);
}

UIWordDisplayWnd::UIWordDisplayWnd()
{
	m_masterAid = 0;
	m_fileName = "";
	Create(24, 24);
}



UIWordDisplayWnd::~UIWordDisplayWnd() {}

INT_PTR UIWordDisplayWnd::SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4) {
	if (message == 0x22)
	{
		m_masterAid = val1;
	}
	return 0;
}

void UIWordDisplayWnd::OnProcess()
{
	CGameActor* actor = g_session.GetGameActorByAid(m_masterAid);
	if (m_masterAid == g_session.GetAid())
		actor = g_modeMgr.GetGameMode()->GetWorld()->m_player;
	if (!actor)
		return;

	int nameX = actor->m_lastTlvertX;
	int nameY = actor->m_lastTlvertY;
	if (m_posx == nameX && m_posx == nameY)
		return;

	m_posx = nameX;
	m_posy = nameY;
	Move(
		(int)(nameX - (g_renderer->m_width * 5 / 640) * actor->m_lastPixelRatio),
		(int)(nameY - (g_renderer->m_height * 100 / 480) * actor->m_lastPixelRatio));
}

void UIWordDisplayWnd::OnDraw() {
	DrawBitmap(0, 0, (CBitmapRes*)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\\xc0\xfc\xc0\xe5\xc0\xfb\xc7\xa5\xbd\xc3.bmp"), false), true);
}
