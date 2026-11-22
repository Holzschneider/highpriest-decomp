#include "common.h"
#include "UIIllustWnd.h"
#include "../GameMode.h"
#include "../Mode.h"
#include "../base/Res.h"
#include "../pdb_enums.h"
#include "../resource/Bitmap.h"

extern CModeMgr g_modeMgr;
extern const char * UIBmp(const char * uiBmpName);

UIIllustWnd :: UIIllustWnd()
{
	m_isDrawTitle   = 0;
	m_isMoveableWnd = 0;
}


UIIllustWnd :: ~UIIllustWnd()
{
}

void	UIIllustWnd :: OnCreate(int cx, int cy)
{
}

void	UIIllustWnd :: OnDestroy()
{
	if (m_bitmapName.length())
		g_resMgr().Unload(g_resMgr().Get(UIBmp(m_bitmapName.c_str()), false));
}

bool	UIIllustWnd :: ShouldDoHitTest()
{
	if (m_isDrawTitle || m_isMoveableWnd) return true;
	return false;
}

void	UIIllustWnd :: OnMouseMove(int x, int y)
{
	if (m_isMoveableWnd)
		UIFrameWnd::OnMouseMove(x, y);
}

void UIIllustWnd::OnDraw()
{
	ClearDC(0xffff00ff);

	if (m_isDrawTitle) {
		{
			CBitmapRes* leftBmp = (CBitmapRes*)g_resMgr().Get(
				UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_left.bmp"), 0);
			DrawBitmap(0, 0, leftBmp, 0);
		}

		CBitmapRes* rightBmp = (CBitmapRes*)g_resMgr().Get(
			UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_right.bmp"), 0);
		DrawBitmap(m_w - rightBmp->m_width, 0, rightBmp, 0);

		int n = (m_w - 24) / 12 + (((m_w - 24) % 12) ? 1 : 0);

		CBitmapRes* bitmap = (CBitmapRes*)g_resMgr().Get(
			UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_mid.bmp"), 0);
		for (int i = 0; i < n; ++i)
			DrawBitmap(12 + i * 12, 0, bitmap, 0);

		CBitmapRes* userBitmap = (CBitmapRes*)g_resMgr().Get(UIBmp(m_bitmapName.c_str()), 0);
		DrawBitmap(0, bitmap->m_height, userBitmap, 1);
	} else {
		CBitmapRes* bitmap = (CBitmapRes*)g_resMgr().Get(UIBmp(m_bitmapName.c_str()), 0);
		DrawBitmap(0, 0, bitmap, 1);
	}
}

#include "rag_budget_fill.h"
int	UIIllustWnd :: SendMsg(UIWindow * sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
	case UM_COMMAND:
		if (val1 == ID_CLOSE)
			g_modeMgr.GetGameMode()->SendMsg(MM_SHOW_IMAGE, (int)"", -1, 0);
		break;
	case UM_SET_BITMAP: {
		RAG_BUDGET_FILL(12)
		m_bitmapName ="\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\Illust\\";
		m_bitmapName += (char *)val1;

		CBitmapRes * bitmap = 0;
		if (g_resMgr().IsExist(m_bitmapName.c_str()))
			bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp(m_bitmapName.c_str()), 0);

		if (!bitmap) {
			m_bitmapName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\cardbmp\\";
			m_bitmapName += (char *)val1;
			bitmap = (CBitmapRes *)g_resMgr().Get(UIBmp(m_bitmapName.c_str()), 0);
		}

		if (bitmap) {
			if (m_h != bitmap->m_height && m_w != bitmap->m_width) {
				if (m_isDrawTitle) {
					CBitmapRes * title = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\titlebar_left.bmp"), 0);
					Resize(bitmap->m_width, bitmap->m_height + title->m_height);
				} else {
					Resize(bitmap->m_width, bitmap->m_height);
				}
			}
		}
		Invalidate();
		break;
	}
	case UM_SETINFO:
		if (val1 == -1) {
			m_isDrawTitle = 0;
			m_isMoveableWnd = 0;
			RemoveAllChildren();
		} else if (val1 == -2) {
			m_isDrawTitle = 1;
		} else {
			int posX = m_w - 16;
			char buttonName[6] = "close";

			std::string preBmpName = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\basic_interface\\sys_";

			UIBitmapButton * button = new UIBitmapButton;
			button->SetBitmapName((preBmpName + buttonName + "_off.bmp").c_str(), BS_NORMAL);
			button->SetBitmapName((preBmpName + buttonName + "_on.bmp").c_str(), BS_MOUSEON);
			button->SetBitmapName((preBmpName + buttonName + "_on.bmp").c_str(), BS_PRESSED);
			button->Create(button->GetBitmapWidth(), button->GetBitmapHeight());
			button->Move(posX, 3);
			button->SetId(0x87);
			AddChild(button);

			m_isMoveableWnd = 1;
			Invalidate();
		}
		break;
	default:
		if (m_isMoveableWnd)
			return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
		return UIWindow::SendMsg(sender, message, val1, val2, val3, val4);
	}
	return 0;
}
