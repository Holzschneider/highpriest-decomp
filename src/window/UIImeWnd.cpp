#include "common.h"
#include "UIImeWnd.h"
#include "../framework/Language.h"
#include "../pdb_enums.h"
#include "../skin.h"

extern LanguageType g_languageType;

UICompositionWnd :: UICompositionWnd()
{
}


UICompositionWnd :: ~UICompositionWnd()
{
}

void	UICompositionWnd :: OnCreate(int cx, int cy)
{
}

int		UICompositionWnd :: SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
		case UM_IME_SET_COMPOSITION_STRING:
			{
				if (val1) {
					const char * compString = (const char *)val1;
					m_compositionString = compString;
					int nWidth = GetTextWidth(compString, 0, 0, 12, false) + 20;
					nWidth = ((nWidth/28)*28) + ((nWidth%28)?28:0);
					if (nWidth<34)
						nWidth = 34;
					if (nWidth != m_w)
						Resize(nWidth, 34);
					Invalidate();
				}
			}
			break;
		default:
			return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
			break;
	}
	return 0;
}


void	UICompositionWnd :: OnDraw()
{
	DrawLatticeFrame(0, 0, 0, 0);
	TextOutA(10, 12, m_compositionString.c_str(), 0, 0, 12, 0);
}

UICandidateWnd :: UICandidateWnd()
{
}


UICandidateWnd :: ~UICandidateWnd()
{
}

void	UICandidateWnd :: OnCreate(int cx, int cy)
{
}

int		UICandidateWnd :: SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	switch (message) {
		case UM_IME_SET_CANDIDATE:
			{
				std::string * candidate = (std::string *)val1;
				int i ;
				for (i=0; i<9; ++i)
					m_candidateList[i] = candidate[i];
				int * intArray = (int *)val2;
				m_selection = intArray[0];
				m_num_curPage = intArray[1];
				m_num_totalPage = intArray[2];
				m_mun_item = intArray[3];

				char text[64];
				int textwidthall = 0;
				for (i=0; i<m_mun_item; ++i) {
					int textnull = 0;
					sprintf(text, "%d %s", i+1, m_candidateList[i].c_str());

					if(g_languageType != LanguageJapanese)
						textwidthall += GetTextWidth(text, 0, 0, 12, false) + 8;
					else
					{
						if (GetTextWidth(text, 0, 0, 12, false) + 8 > textwidthall)
							textwidthall = GetTextWidth(text, 0, 0, 12, false) + 8;
					}
				}

				if(g_languageType != LanguageJapanese) {
					Resize(textwidthall+22, m_h);
				} else {
					Resize(textwidthall+22, 20*m_mun_item + 14);
				}

				Invalidate();
			}
			break;
		default:
			return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
			break;
	}
	return 0;
}

void	UICandidateWnd :: OnDraw()
{
	DrawLatticeFrame(0, 0, 0, 0);

	switch (g_languageType) {
		case LanguageKorean:
			{
				char text[64];
				int textout = 14;
				for (int i=0; i<m_mun_item; ++i) {
					sprintf(text, "%d %s", i+1, m_candidateList[i].c_str());
					TextOutA(textout, 12, text, 0, 0, 12, 0);
					textout += GetTextWidth(text, strlen(text), 0, 12, false) + 8;
				}

				COLORREF color;
				if (m_num_curPage > 0)
					color = RGB(112, 159, 255);
				else
					color = RGB(210, 210, 210);
				DrawTriangleDC(8, 12+6, 8+6, 10, 8+6, 26, color);

				if (m_num_curPage < m_num_totalPage)
					color = RGB(112, 159, 255);
				else
					color = RGB(210, 210, 210);
				DrawTriangleDC(m_w-8, 12+6, m_w-8-6, 10, m_w-8-6, 26, color);
			}
			break;
		case LanguageJapanese:
			{
				char text[64];
				for (int i=0; i<m_mun_item; ++i) {
					sprintf(text, "%d %s", i+1, m_candidateList[i].c_str());
					if (m_selection == i) {

						DWORD rr, gg, bb;
						g_skinMgr.GetColorChipColor(6, 2, rr, gg, bb);
						DrawBox(8, 20*i+10, GetTextWidth(text, 0, 0, 12, false)+4, 16, RGB(rr, gg, bb));
					}
					TextOutA(10, 20*i+12, text, 0, 0, 12, 0);
				}

				sprintf(text, "%d / %d" ,m_num_curPage+1, m_num_totalPage+1);
				TextOutA(120-10-GetTextWidth(text, 0, 0, 12, false), 20*9+12, text, 0, 0, 12, 0);
			}
			break;
		case LanguageSimplifiedChinese:
		case LanguageTraditionalChinese:
			{
				char text[64];
				int textout = 14;
				for (int i=0; i<m_mun_item; ++i) {
					sprintf(text, "%d %s", i+1, m_candidateList[i].c_str());
					if(i != m_selection) {
						TextOutA(textout, 12, text, 0, 0, 12, 0);
						textout += GetTextWidth(text, strlen(text), 0, 12, false) + 8;
					} else {
						TextOutA(textout, 12, text, strlen(text), 0, 12, RGB(255, 0, 0));
						textout += GetTextWidth(text, strlen(text), 0, 12, false) + 8;
					}
				}

				COLORREF color;
				if (m_num_curPage > 0)
					color = RGB(112, 159, 255);
				else
					color = RGB(210, 210, 210);
				DrawTriangleDC(8, 12+6, 8+6, 10, 8+6, 26, color);

				if (m_num_curPage < m_num_totalPage)
					color = RGB(112, 159, 255);
				else
					color = RGB(210, 210, 210);
				DrawTriangleDC(m_w-8, 12+6, m_w-8-6, 10, m_w-8-6, 26, color);
			}
			break;
		default:
			break;
	}
}

UIKeyStrokeWnd :: UIKeyStrokeWnd()
{
}


UIKeyStrokeWnd :: ~UIKeyStrokeWnd()
{
}

void	UIKeyStrokeWnd :: OnCreate(int cx, int cy)
{
}

int		UIKeyStrokeWnd :: SendMsg(UIWindow *sender, int message, int val1, int val2, int val3, int val4)
{
	return UIFrameWnd::SendMsg(sender, message, val1, val2, val3, val4);
}

void	UIKeyStrokeWnd :: OnDraw()
{
	if(g_languageType == LanguageTraditionalChinese)
	{
		DWORD rr, gg, bb;

		g_skinMgr.GetColorChipColor(2, 2, rr, gg, bb);
		DrawBox(1, 1, m_w-2, m_h-2, RGB(rr,gg,bb));

		g_skinMgr.GetColorChipColor(22, 2, rr, gg, bb);
		DrawBox(0, 0, m_w, 1, RGB(rr,gg,bb));
		DrawBox(0, 0, 1, m_h, RGB(rr,gg,bb));
		DrawBox(0, m_h-1, m_w, 1, RGB(rr,gg,bb));
		DrawBox(m_w-1, 0, 1, m_h, RGB(rr,gg,bb));

		char keystroke[3] = {0,};

		memcpy(keystroke, g_language->GetKeyStroke()+0, 2);
		TextOutA(2+14*0, 2, keystroke, 2, 0, 12, 0);
		memcpy(keystroke, g_language->GetKeyStroke()+2, 2);
		TextOutA(2+14*1, 2, keystroke, 2, 0, 12, 0);
		memcpy(keystroke, g_language->GetKeyStroke()+4, 2);
		TextOutA(2+14*2, 2, keystroke, 2, 0, 12, 0);
		memcpy(keystroke, g_language->GetKeyStroke()+6, 2);
		TextOutA(2+14*3, 2, keystroke, 2, 0, 12, 0);
	}
}
