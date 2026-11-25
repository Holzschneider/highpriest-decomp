#include "common.h"
#include "UISys.h"
#include "../3dlib/Renderer.h"
#include "../3dlib/Device.h"
#include "../3dlib/DrawUtil.h"
#include "../framework/Language.h"
#include "../resource/Bitmap.h"
#include "../Mode.h"
#include "../pdb_enums.h"
#include "../ItemInfo.h"

#include <windows.h>

extern CModeMgr	g_modeMgr;
extern UIWindowMgr g_windowMgr;

#include <algorithm>


#define CURSOR_ARROW 0




extern bool          g_isHex[];
extern unsigned char hex_table[];

unsigned char hex_table[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

extern DWORD g_codePage;

BOOL	IsValidColorIndicator(LPCSTR pszMsg, LPCSTR pszEnd)
{
	if(!(*pszMsg == '^' && pszEnd-pszMsg>6))
		return FALSE;

	for(int nPos = 1; nPos <= 6; ++nPos)
	{
		if(!g_isHex[pszMsg[nPos]])
			return FALSE;
	}

	return TRUE;
}

BOOL IsValidItemIndicator(LPCSTR pszMsg, LPCSTR pszEnd, int * psizeChr, int * pitemId)
{
	char * indicator = "^nItemID^";
	LPCSTR item = pszMsg;
	int len = strlen(indicator);
	char buf[10];

	if ( strncmp(item,indicator,len) == 0 )
	{
		int nPos ;
		for(nPos = 0; item + nPos < pszEnd; ++nPos)
		{
			if(!g_isHex[item[nPos+len]])
				break;
		}

		if(nPos != 0)
		{
			strncpy(buf,item+len,nPos);
			buf[nPos] = NULL;

			*psizeChr = nPos+len;
			*pitemId = atoi(buf);
			return TRUE;
		}
	}

	return FALSE;
}

void	CutString(const char *msg, std::vector<std::string> &strings, int maxNumCharLine)
{
	const char* msg_iter = msg;
	const char* msg_end = msg + strlen(msg);
	const char* msg_limit = msg + maxNumCharLine;
	const char* msg_break = msg;
	const char* msg_charset = msg + FindCharSet(msg, msg_end - msg);

	while (msg_iter < msg_end)
	{
		msg_iter = g_language->GetWordBreak(msg_iter, msg_end - msg_iter);

		while(*msg_iter == ' ')
			msg_break = ++msg_iter;

		if(msg_iter >= msg_limit)
		{
			if(msg_break > msg)
			{
				strings.push_back(std::string(msg, msg_break));
				msg = msg_break;
			}
			else
			{
				strings.push_back(std::string(msg, msg_iter));
				msg = msg_iter;
			}
			msg_limit = msg + maxNumCharLine;
		}
	}

	if(msg < msg_iter)
		strings.push_back(std::string(msg, msg_iter));

	if ( g_multiLang && msg_charset != NULL && *msg_charset != NULL)
	{
		if(msg_charset < msg_end)
		{
			for(int i=1; i<strings.size(); ++i)
				strings[i].insert(strings[i].begin(), msg_charset, msg_charset+3);
		}
	}
}

void	CutStringWithColor(const char *msg, std::vector<std::string> &strings, int maxNumCharLine, bool readCR)
{
	const char* msg_iter = msg;
	const char* msg_end = msg + strlen(msg);
	const char* msg_limit = msg + maxNumCharLine;
	const char* msg_break = msg;
	const char* msg_charset = msg + FindCharSet(msg, msg_end - msg);
	int colorgap = 0;

	while (msg_iter < msg_end)
	{
		if(readCR && (*msg_iter == 0x0d))
		{
			strings.push_back(std::string(msg, msg_iter++));

			while(msg_iter < msg_end && *msg_iter==0x0a)
				++msg_iter;

			msg = msg_iter;
			msg_limit = msg + maxNumCharLine;
		}
		else
		{
			int sizeChr = 0;
			int itemId = 0;

			if(IsValidColorIndicator(msg_iter, msg_end))
			{
				msg_iter += 7;
				msg_limit += 7;
			}
			else if (IsValidItemIndicator(msg_iter,msg_end,&sizeChr,&itemId))
			{
				strings.push_back(std::string(msg, msg_iter));
				msg = msg_iter;
				msg_iter += sizeChr;
				msg = msg_iter;
				msg_limit += sizeChr;
				if ( itemId )
				{
					ITEM_INFO item;
					item.SetItemId(itemId);
					char buf[MAX_PATH];
					item.GetIdDisplayName(buf);
					strings.push_back(buf);
				}
			}
			else
				msg_iter = g_language->GetWordBreak(msg_iter, msg_end - msg_iter);

			while(*msg_iter == ' ')
				msg_break = ++msg_iter;

			if(msg_iter >= msg_limit)
			{
				if(msg_break > msg)
				{
					strings.push_back(std::string(msg, msg_break));
					msg = msg_break;
				}
				else
				{
					strings.push_back(std::string(msg, msg_iter));
					msg = msg_iter;
				}
				msg_limit = msg + maxNumCharLine;
			}
		}
	}

	if(msg < msg_iter)
		strings.push_back(std::string(msg, msg_iter));

	if ( g_multiLang && msg_charset != NULL && *msg_charset != NULL)
	{
		if(msg_charset < msg_end)
		{
			for(int i=1; i<strings.size(); ++i)
			{
				int pos = 0;
				if ( IsValidColorIndicator(strings[i].c_str(),strings[i].c_str()+strings[i].size()) )
					pos = 7;
				strings[i].insert(strings[i].begin()+pos, msg_charset, msg_charset+3);
			}
		}
	}
}

UIWindow :: UIWindow()
{
	m_x = 0;
	m_y = 0;
	m_w = 0;
	m_h = 0;
	m_parent = NULL;
	m_isDirty = TRUE;
	m_state = 0;
	m_stateCnt = 0;
	m_dc = NULL;
	m_id = ID_NOTHING;
	m_show = TRUE;
	m_trans = 0xFF;
	m_transTarget = 0xFF;
	m_transTime = bf_clock_ticks();
}


UIWindow :: ~UIWindow()
{
	Destroy();

	g_windowMgr.RemoveFromNameWaitingWindowList(this);
#if defined(BF_PORT)
	g_windowMgr.BfForgetWindow(this);
#endif
}


void	UIWindow :: DragAndDrop(int x, int y, const DRAG_INFO * const dragInfo)
{
	CMode *	curMode = g_modeMgr.GetCurMode();
	curMode->SendMsg(MM_CANCEL_DRAG, 0, 0, 0);
}

UIWindow *	UIWindow :: GetAncestor()
{
	UIWindow * ancestor = this;
	while (ancestor->m_parent)
		ancestor = ancestor->m_parent;
	sAssert(ancestor);
	return ancestor;
}

void	UIWindow :: AddChild(UIWindow *window)
{
	window->m_parent = this;
	m_children.push_back(window);
}

void	UIWindow :: RemoveChild(UIWindow * window)
{
	std::list<UIWindow *>::iterator i = std::find(m_children.begin(), m_children.end(), window);

	if (i != m_children.end())
		m_children.erase(i);

	delete window;
}

void	UIWindow :: RemoveAllChildren()
{
	std::list<UIWindow *>::iterator i = m_children.begin();

	for (; i != m_children.end(); ++i)
		delete *i;

	m_children.clear();
}

void	UIWindow :: ClearAllChildrenState()
{
	m_state = 0;
	Invalidate();

	std::list<UIWindow *>::iterator i = m_children.begin();

	for (; i != m_children.end(); ++i)
		(*i)->ClearAllChildrenState();
}

void	UIWindow :: Create(int cx, int cy)
{
	UIWindow::Resize(cx, cy);
	OnCreate(cx, cy);
	OnSize(cx, cy);
}

void	UIWindow :: Resize(int cx, int cy)
{
	sAssert(cx>0);
	sAssert(cy>0);
	if(m_dc)
		delete m_dc;
	m_dc = g_3dDevice.CreateDCBuffer(cx, cy);
	m_w = cx;
	m_h = cy;
	Invalidate();
}

void	UIWindow :: Destroy()
{
	OnDestroy();
	if (m_dc) {
		delete m_dc;
		m_dc = NULL;
	}
	RemoveAllChildren();
}

int	UIWindow :: IsFamily(UIWindow *window)
{
	if (window == NULL)
		return FALSE;
	if (window == this)
		return TRUE;

	std::list<UIWindow *>::iterator it = m_children.begin();

	for (; it != m_children.end(); ++it)
		if ((*it)->IsFamily(window))
			return TRUE;

	return FALSE;
}

UIWindow *	UIWindow :: GetNextSiblingOf(UIWindow *wnd)
{
	std::list<UIWindow *>::iterator i = m_children.begin();

	for (; i != m_children.end(); ++i) {
		if (*i == wnd && (*i)->CanGetFocus()) {
			++i;

			for (; i != m_children.end(); ++i)
				if ((*i)->CanGetFocus())
					return *i;

			return NULL;
		}
	}
	return NULL;
}

UIWindow *	UIWindow :: GetPrevSiblingOf(UIWindow *wnd)
{
	std::list<UIWindow *>::reverse_iterator i = m_children.rbegin();

	for (; i != m_children.rend(); ++i) {
		if (*i == wnd && (*i)->CanGetFocus()) {
			++i;

			for (; i != m_children.rend(); ++i)
				if ((*i)->CanGetFocus())
					return *i;

			return NULL;
		}
	}
	return NULL;
}

void	UIWindow :: Move(int x, int y)
{
	m_x = x;
	m_y = y;
}

void	UIWindow :: OnMouseShape(int x, int y)
{
	g_modeMgr.GetCurMode()->SetCursorAction(CURSOR_ARROW);
}

void	UIWindow :: GetGlobalCoor(int &x, int &y)
{
	x += m_x;
	y += m_y;
	UIWindow * parent = m_parent;
	while (parent) {
		x += parent->m_x;
		y += parent->m_y;
		parent = parent->m_parent;
	}
}

void	UIWindow :: ClearDC(DWORD color)
{
	if (m_dc)
		m_dc->ClearSurface(NULL, color);
}

void	UIWindow :: ClearDCRect(DWORD color, RECT &rect)
{
	if (m_dc)
		m_dc->ClearSurface(&rect, color);
}

void	UIWindow :: DrawTriangleDC(int x1, int y1, int x2, int y2, int x3, int y3, COLORREF color)
{
	POINT point[3];
	point[0].x = x1;
	point[0].y = y1;
	point[1].x = x2;
	point[1].y = y2;
	point[2].x = x3;
	point[2].y = y3;

	sAssert(m_dc);

	HDC dc;

	if(m_dc->GetDC(&dc)) {
		HRGN aRgn = CreatePolygonRgn(point, 3, WINDING);
		HBRUSH colorBrush = CreateSolidBrush(color);
		FillRgn(dc, aRgn, colorBrush);
		DeleteObject(aRgn);
		DeleteObject(colorBrush);
		m_dc->ReleaseDC(dc);
	}
}

void	UIWindow :: DrawBox(int x, int y, int cx, int cy, DWORD color)
{
	RECT r = { max(x,0), max(y,0), min(x+cx,m_w), min(y+cy,m_h) };

	m_dc->ClearSurface(&r, color);
}

void	UIWindow :: DrawBorderLine(int x, int y, int w, int h, DWORD color)
{
	DrawBox(x + 2, y, w - 4, 1, color);
	DrawBox(x + 1, y + 1, 1, 1, color);
	DrawBox(x, y + 2, 1, h - 4, color);
	DrawBox(x + 1, y + h - 2, 1, 1, color);

	DrawBox(x + w - 2, y + 1, 1, 1, color);
	DrawBox(x + w - 1, y + 2, 1, h - 4, color);
	DrawBox(x + 2, y + h - 1, w - 4, 1, color);
	DrawBox(x + w - 2, y + h - 2, 1, 1, color);
}

void	UIWindow :: DoDraw(int blitToParent)
{
	if (m_isDirty) {
		OnDraw();
		m_isDirty = FALSE;
		blitToParent = TRUE;
	}

	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		UIWindow * childWnd = *i;
		childWnd->DoDraw(blitToParent);
		++i;
	}

	if (blitToParent) {
		if (m_parent) {
			m_parent->m_dc->CopyRect(m_x, m_y, m_w, m_h, m_dc);
		}
	}
}

void	UIWindow :: Invalidate()
{
	m_isDirty = TRUE;
}

void	UIWindow :: InvalidateChildren()
{
	m_isDirty = TRUE;
	InvalidateWallPaper();

	std::list<UIWindow *>::iterator i = m_children.begin();

	for (; i != m_children.end(); ++i)
		(*i)->InvalidateChildren();
}

bool UIWindow :: IsChildOf(UIWindow *win)
{
	for (UIWindow * parent = m_parent; parent; parent = parent->m_parent)
		if (parent == win)
			return true;

	return false;
}

void	UIWindow :: DrawBitmap(int x, int y, CBitmapRes *bitmap, bool drawOnlyNoTrans)
{
	if(!m_dc)
		return;
	if (bitmap == NULL)
		return;

	m_dc->Update(x, y, bitmap->m_width, bitmap->m_height, bitmap->m_data, drawOnlyNoTrans);
}

const char* UIWindow :: InterpretColor(const char* color_text, DWORD* colorRef)
{
	if(!color_text)
		return color_text;

	if(*color_text == '^')
	{


		int r = hex_table[color_text[1]] * 0x10 + hex_table[color_text[2]];
		int g = hex_table[color_text[3]] * 0x10 + hex_table[color_text[4]];
		int b = hex_table[color_text[5]] * 0x10 + hex_table[color_text[6]];

		*colorRef = RGB(r,g,b);

		return color_text + 7;
	}

	return color_text;
}

const char*	UIWindow :: FindColorMark(const char* text, const char* text_end)
{
	if(!text)
		return text;



	for(;text < text_end; text = CharNextExA(g_codePage, text, 0))
	{
		if('^' != *text || text_end-text < 6)
			continue;
		BOOL	bIsColorMark = TRUE;
		for(int nPos = 1; nPos <= 6; ++nPos)
		{
			if(!g_isHex[text[nPos]])
			{
				bIsColorMark = FALSE;
				break;
			}
		}
		if(bIsColorMark)
			return text;
	}

	return text;
}

void	UIWindow :: TextOutA(int x, int y, const char *text, int textLen, int fontType, int fontHeight, DWORD colorRef)
{

	if(!textLen)
		textLen = strlen(text);

	DrawDC dc(m_dc);
	dc.SetFont(fontType, fontHeight, false);

	dc.SetTextColor(colorRef);
	dc.TextOutA(x, y, text, textLen);
}

void	UIWindow :: TextOutWithRightAlign(int x, int y, const char *text, int textLen, int fontType, int fontHeight, COLORREF colorRef)
{
	sAssert(m_dc);

	if(!textLen)
		textLen = strlen(text);

	DrawDC dc(m_dc);
	dc.SetFont(fontType, fontHeight, false);

	SIZE	size;
	dc.GetTextExtentPoint32A(text, strlen(text), &size);

	dc.SetTextColor(colorRef);
	dc.TextOutA(x-size.cx, y, text, textLen);
}

void	UIWindow :: TextOutWithDecoration(int x, int y, const char *text, int textLen, COLORREF* colorRef, int fontType, int fontHeight)
{
	sAssert(m_dc);

	if(!textLen) textLen = strlen(text);

	SIZE	size;
	const char* text_iter;
	const char* text_end = text + textLen;
	const char* msg_charset = text + FindCharSet(text, text_end - text);

	DrawDC dc(m_dc);
	dc.SetFont(fontType, fontHeight, false);

	while(text < text_end)
	{
		text = InterpretColor(text, colorRef);
		text_iter = FindColorMark(text, text_end);
		std::string str = std::string(text,text_iter);

		if ( msg_charset != NULL && *msg_charset != NULL)
			str.insert(str.begin(), msg_charset, msg_charset+3);

		if(text_iter != text)
		{
			dc.SetTextColor(*colorRef);
			dc.TextOutA(x, y, str.c_str(), str.size());

			dc.GetTextExtentPoint32A(text, text_iter - text, &size);

			x += size.cx;
		}

		text = text_iter;
	}
}

void	UIWindow :: TextOutWithShadow(int x, int y, const char *text, int textLen, COLORREF colorText, COLORREF colorShadow, int fontType, int fontHeight)
{
	sAssert(m_dc);

	if(!textLen)
		textLen = strlen(text);

	DrawDC dc(m_dc);
	dc.SetFont(fontType, fontHeight, false);

	dc.SetTextColor(colorShadow);
	dc.TextOutA(x+1, y+1, text, textLen);

	dc.SetTextColor(colorText);
	dc.TextOutA(x, y, text, textLen);
}

void	UIWindow :: TextOutWithOutline(int x, int y, const char *text, int textLen, COLORREF colorText, COLORREF colorOutline, int fontType, int fontHeight, bool bold)
{
	sAssert(m_dc);

	if(!textLen)
		textLen = strlen(text);

	DrawDC dc(m_dc);
	dc.SetFont(fontType, fontHeight, bold);

	dc.SetTextColor(colorOutline);
	dc.TextOutA(x-1, y, text, textLen);
	dc.TextOutA(x+1, y, text, textLen);
	dc.TextOutA(x, y-1, text, textLen);
	dc.TextOutA(x, y+1, text, textLen);

	dc.SetTextColor(colorText);
	dc.TextOutA(x, y, text, textLen);
}

void	UIWindow :: TextOutWithDecoOutline(int x, int y, const char *text, int textLen, COLORREF* colorText, COLORREF colorOutline, int fontType, int fontHeight, bool bold)
{


	if(!textLen)
		textLen = strlen(text);

	SIZE	size;
	const char* text_iter;
	const char* text_end = text + textLen;
	const char* msg_charset = text + FindCharSet(text, text_end - text);

	DrawDC dc(m_dc);
	dc.SetFont(fontType, fontHeight, bold);

	while(text < text_end)
	{
		text = InterpretColor(text, colorText);

		text_iter = FindColorMark(text, text_end);

		std::string str = std::string(text,text_iter);

		if ( msg_charset != NULL && *msg_charset != NULL)
			str.insert(str.begin(), msg_charset, msg_charset+3);

		if(text_iter != text)
		{
			dc.SetTextColor(colorOutline);
			dc.TextOutA(x-1, y, str.c_str(), str.size());
			dc.TextOutA(x+1, y, str.c_str(), str.size());
			dc.TextOutA(x, y-1, str.c_str(), str.size());
			dc.TextOutA(x, y+1, str.c_str(), str.size());

			dc.SetTextColor(*colorText);
			dc.TextOutA(x, y, str.c_str(), str.size());

			dc.GetTextExtentPoint32A(text, text_iter - text, &size);

			x += size.cx;
		}

		text = text_iter;
	}
}

void	UIWindow :: TextOutVertical(int x, int y, const char *text, int textLen, int fontType, int fontHeight, COLORREF colorRef)
{
	sAssert(m_dc);

	if(!textLen) textLen = strlen(text);

	SIZE size;
	int maxWidth = 0;
	const char* textNext;
	const char* textIter = text;
	const char* textEnd = textIter + textLen;

	DrawDC dc(m_dc);
	dc.SetFont(fontType, fontHeight, false);

	dc.SetTextColor(colorRef);

	while(textIter < textEnd)
	{
		textNext = CharNextExA(g_codePage, textIter, 0);
		dc.GetTextExtentPoint32A(textIter, textNext - textIter, &size);
		maxWidth = max(maxWidth, size.cx);
		textIter = textNext;
	}

	textIter = text;
	while(textIter < textEnd)
	{
		textNext = CharNextExA(g_codePage, textIter, 0);
		dc.GetTextExtentPoint32A(textIter, textNext - textIter, &size);
		dc.TextOutA(x+(maxWidth-size.cx)/2, y, textIter, textNext - textIter);
		textIter = textNext;
		y += size.cy;
	}
}

void	UIWindow :: TextOutWithOutlineVertical(int x, int y, const char *text, int textLen, COLORREF colorText, COLORREF colorOutline, int fontType, int fontHeight, bool bold)
{
	sAssert(m_dc);

	if(!textLen) textLen = strlen(text);

	SIZE size;

	DrawDC dc(m_dc);
	dc.SetFont(fontType, fontHeight, bold);

	dc.GetTextExtentPoint32A(text, textLen, &size);

	dc.SetTextColor(colorOutline);
	dc.TextOutA(x+size.cy-1, y, text, textLen);
	dc.TextOutA(x+size.cy+1, y, text, textLen);
	dc.TextOutA(x+size.cy, y-1, text, textLen);
	dc.TextOutA(x+size.cy, y+1, text, textLen);

	dc.SetTextColor(colorText);
	dc.TextOutA(x+size.cy, y, text, textLen);
}

void	UIWindow :: TextOutWithTwoColors(int x, int y, const char *text, COLORREF colorRef, COLORREF colorRef2,
													int textLen, int fontType, int fontHeight)
{
	sAssert(m_dc);

	if(!textLen)
		textLen = strlen(text);

	DrawDC dc(m_dc);
	dc.SetFont(fontType, fontHeight, false);

	dc.SetTextColor(colorRef);
	dc.TextOutA(x, y, text, textLen);

	dc.SetTextColor(colorRef2);
	dc.TextOutA(x - 1, y, text, textLen);
}

int		UIWindow :: GetTextWidth(const char * text, int textLen, int fontType, int fontHeight, bool bold)
{

	if(!text) return 0;
	if(!textLen)
	{
		textLen = strlen(text);
		if(!textLen) return 0;
	}



	DrawDC dc(m_dc);
	dc.SetFont(fontType, fontHeight, bold);

	SIZE size;

	dc.GetTextExtentPoint32A(text, textLen, &size);

	return size.cx;
}

int		UIWindow :: GetTextHeight(const char * text, int textLen, int fontType, int fontHeight, bool bold)
{

	if(!text) return 0;
	if(!textLen)
	{
		textLen = strlen(text);
		if(!textLen) return 0;
	}



	DrawDC dc(m_dc);
	dc.SetFont(fontType, fontHeight,bold);

	SIZE size;

	dc.GetTextExtentPoint32A(text, textLen, &size);

	return size.cy;
}

int		UIWindow :: GetTextHeightVertical(const char * text, int textLen, int fontType, int fontHeight, bool bold)
{

	if(!text) return 0;
	if(!textLen)
	{
		textLen = strlen(text);
		if(!textLen) return 0;
	}



	SIZE size;
	int height = 0;
	const char* textNext;
	const char* textEnd = text + textLen;

	DrawDC dc(m_dc);
	dc.SetFont(fontType, fontHeight,bold);

	while(text < textEnd)
	{
		textNext = CharNextExA(g_codePage, text, 0);
		dc.GetTextExtentPoint32A(text, textNext - text, &size);
		height += size.cy;
		text = textNext;
	}

	return height;
}

int		UIWindow :: GetColorTextWidth(const char * text, int textLen, int fontType, int fontHeight, bool bold)
{

	if(!text) return 0;
	if(!textLen)
	{
		textLen = strlen(text);
		if(!textLen) return 0;
	}



	int tx = GetTextWidth ( text, textLen, fontType, fontHeight, bold ) ;
	{
		DWORD colorRef = RGB(0,0,0);


		if ( InterpretColor ( FindColorMark ( text, text + textLen ), &colorRef ) && colorRef )
			tx -= ( GetTextWidth ( "^000000", 0, 0, 12, false ) * 2 ) - 6 ;
	}

	return tx;
}

char * UIWindow :: PolishText ( const char * text, int drawWidth, int fontType, int fontHeight, bool bold )
{
	if ( !text )
		return (char *) text ;

	int len = strlen ( text ) ;
	int textW = GetTextWidth ( text, len, fontType, fontHeight, bold ) ;
	int tabW = GetTextWidth ( "...", 0, fontType, fontHeight, bold ) ;

	if ( textW > drawWidth )
	{
		int		line = 0 ;
		int		lineW = 0 ;

		const char* msg_src = text ;
		const char* msg_iter = text ;
		const char* msg_end = text + strlen(text) ;

		while (msg_iter < msg_end)
		{
			const char* msg_old = msg_iter ;
			msg_iter = g_language->GetWordBreak(msg_iter, msg_end - msg_iter);

			int k = msg_iter - msg_old ;

			lineW += GetTextWidth ( msg_old, k, fontType, fontHeight, bold ) ;

			line += k ;

			if ( lineW > drawWidth - tabW )
			{
				line -= k ;
				break;
			}
		}

		if ( line > 0 )
		{
			char * pText = new char[line + 4] ;
			char * buf = new char[line + 1] ;

			memcpy ( buf, msg_src, line ) ;
			buf[line] = 0 ;

			strcpy ( pText, buf ) ;
			strcat ( pText, "..." ) ;

			return pText ;
		}
	}

	return (char *) text ;
}

char * UIWindow :: PolishText2 ( const char * text, int maxNumCharLine )
{
	if ( !text )
		return (char *) text ;

	int textLen = strlen ( text ) ;

	if ( textLen > maxNumCharLine )
	{
		int		line = 0 ;

		const char* msg_src = text ;
		const char* msg_iter = text ;
		const char* msg_end = text + strlen(text) ;

		while (msg_iter < msg_end)
		{
			const char* msg_old = msg_iter ;
			msg_iter = g_language->GetWordBreak(msg_iter, msg_end - msg_iter);

			int k = msg_iter - msg_old ;

			line += k ;

			if ( line > maxNumCharLine )
			{
				line -= k ;
				break;
			}
		}

		if ( line > 0 )
		{
			char * pText = new char[line + 1] ;

			memcpy ( pText, msg_src, line ) ;
			pText[line] = 0 ;

			return pText ;
		}
	}

	return (char *) text ;
}


void	UIWindow :: PolishText3 ( const char * text, std::vector<std::string> &strings, std::vector<bool> &enters, int width )
{
	if ( !text )
		return ;

	int textW = GetTextWidth ( text, 0, 0, 12, false ) ;

	int len = strlen ( text ) ;

	if ( textW > 0 )
	{
		int		index = 0 ;

		while ( index < len )
		{
			int		line = 0 ;
			int		lineW = 0 ;

			bool	enter = false ;

			const char* msg_src = text + index ;
			const char* msg_iter = text + index ;
			const char* msg_end = text + index + strlen(text + index) ;

			while (msg_iter < msg_end)
			{
				enter = false ;

				const char* msg_old = msg_iter ;
				msg_iter = g_language->GetWordBreak(msg_iter, msg_end - msg_iter);

				int k = msg_iter - msg_old ;

				lineW += GetTextWidth ( msg_old, k, 0, 12, false ) ;

				line += k ;

				if ( k == 1 )
				{
					if ( msg_old[0] == 13 )
					{
						enter = true ;
						k++ ;
						line++ ;
						break;
					}
				}

				if ( lineW > width )
				{
					line -= k ;
					break;
				}
			}

			if ( line > 0 )
			{
				char * buf = new char[line + 1] ;

				memcpy ( buf, msg_src, line ) ;
				buf[line] = 0 ;

				strings.push_back ( buf ) ;
#if defined(BF_PORT)
				delete[] buf ;	// never freed: one per line of every wrapped text (M8.6 soak, leaks)
#endif

				if ( enter )
					enters.push_back ( true ) ;
				else
					enters.push_back ( false ) ;

				index += line ;
			}
		}
	}
}

void	UIWindow :: PolishText4 ( const char * text, std::vector<std::string> &strings, int width )
{
	if ( !text )
		return ;

	int textW = GetTextWidth ( text, 0, 0, 12, false ) ;

	int len = strlen ( text ) ;

	if ( textW > 0 )
	{
		int		index = 0 ;

		while ( index < len )
		{
			int		line = 0 ;
			int		lineW = 0 ;

			bool	enter = false ;

			const char* msg_src = text + index ;
			const char* msg_iter = text + index ;
			const char* msg_end = text + index + strlen(text + index) ;

			while (msg_iter < msg_end)
			{
				enter = false ;

				const char* msg_old = msg_iter ;
				msg_iter = g_language->GetWordBreak(msg_iter, msg_end - msg_iter);

				int k = msg_iter - msg_old ;

				lineW += GetTextWidth ( msg_old, k, 0, 12, false ) ;

				line += k ;

				if ( k == 1 )
				{
					if ( msg_old[0] == 13 )
					{
						lineW -= ( GetTextWidth ( "\n", 0, 0, 12, false ) * 2 ) ;

						enter = true ;
						k++ ;
						line++ ;
						break;
					}
				}

				if ( msg_old[0] == 94 )
				{
					DWORD colorRef = RGB(0,0,0);
					InterpretColor ( msg_old, &colorRef ) ;

					if ( colorRef )
						lineW -= ( GetTextWidth ( "^000000", 0, 0, 12, false ) ) ;
					else
					{
						char * buf = new char[8] ;
						memcpy ( buf, msg_old, 7 ) ;
						buf[7] = 0 ;

						if ( !strcmp ( buf, "^000000" ) )
							lineW -= ( GetTextWidth ( "^000000", 0, 0, 12, false ) ) ;
#if defined(BF_PORT)
						delete[] buf ;	// never freed (M8.6 soak, leaks)
#endif
					}
				}

				if ( lineW > width )
				{
					line -= k ;
					break;
				}
			}

			if ( line > 0 )
			{
				char * buf = new char[line + 1] ;

				memcpy ( buf, msg_src, line ) ;

				if ( enter )
					buf[line-2] = 0 ;
				else
					buf[line] = 0 ;

				if ( enter && strlen(buf) == 0 )
					strings.push_back ( " " ) ;
				else
					strings.push_back ( buf ) ;
#if defined(BF_PORT)
				delete[] buf ;	// never freed: one per line of every chat message (M8.6 soak, leaks)
#endif

				index += line ;
			}
		}
	}
}

UIWindow *	UIWindow :: HitTest(int x, int y)
{
	if (!ShouldDoHitTest() || !m_show)
		return NULL;
	if (x < m_x || x >= m_x+m_w || y < m_y || y >= m_y+m_h)
		return NULL;

	std::list<UIWindow *>::reverse_iterator i = m_children.rbegin();

	while (i != m_children.rend()) {
		UIWindow * result;

		result = (*i)->HitTest(x-m_x, y-m_y);
		if (result)
			return result;
		++i;
	}
	return this;
}

INT_PTR		UIWindow :: SendMsg(UIWindow * sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	switch (message) {
		case	UM_DEFPUSH:
			if (m_parent)
				m_parent->SendMsg(this, UM_DEFPUSH, 0, 0, 0, 0);
			break;

		case	UM_CANCELPUSH:
			if (m_parent)
				m_parent->SendMsg(this, UM_CANCELPUSH, 0, 0, 0, 0);
			break;

		default:
			return 0;
	}
	return	0;
}

DWORD	UIWindow :: GetColor()
{
	if(m_trans < m_transTarget)
		m_trans = min(m_transTarget, m_trans + (bf_clock_ticks() - m_transTime) / 16);
	else if(m_trans > m_transTarget)
		m_trans = max(m_transTarget, m_trans - (bf_clock_ticks() - m_transTime) / 16);

	return ((DWORD)m_trans) * 0x01000000 + 0xFFFFFF;
}

void	CSnapInfo :: SetInfo(int x, int y, int w, int h)
{
	m_info[SNT_VERTLEFT].points[0] = x;
	m_info[SNT_VERTLEFT].points[1] = y;
	m_info[SNT_VERTLEFT].points[2] = y+h;

	m_info[SNT_VERTRIGHT].points[0] = x+w;
	m_info[SNT_VERTRIGHT].points[1] = y;
	m_info[SNT_VERTRIGHT].points[2] = y+h;

	m_info[SNT_HORZTOP].points[0] = y;
	m_info[SNT_HORZTOP].points[1] = x;
	m_info[SNT_HORZTOP].points[2] = x+w;

	m_info[SNT_HORZBOTTOM].points[0] = y+h;
	m_info[SNT_HORZBOTTOM].points[1] = x;
	m_info[SNT_HORZBOTTOM].points[2] = x+w;
}

void	CSnapInfo :: Reverse()
{
	SnapInfo tempInfo;
	tempInfo = m_info[SNT_VERTLEFT];
	m_info[SNT_VERTLEFT] = m_info[SNT_VERTRIGHT];
	m_info[SNT_VERTRIGHT] = tempInfo;

	tempInfo = m_info[SNT_HORZTOP];
	m_info[SNT_HORZTOP] = m_info[SNT_HORZBOTTOM];
	m_info[SNT_HORZBOTTOM] = tempInfo;
}

bool CSnapInfo :: IsOverLapped(int s1, int d1, int s2, int d2)
{
	if (s2 >= s1 && s2 <= d1)
		return true;
	if (d2 >= s1 && d2 <= d1)
		return true;
	if (s1 >= s2 && s1 <= d2)
		return true;
	if (d1 >= s2 && d1 <= d2)
		return true;

	return false;
}

void	CSnapInfo :: GetDeltaSnap(CSnapInfo * snapInfo, int &dx, int &dy)
{
	dx = 0;
	dy = 0;
	const int gravityLength = 10;
	bool  isOverLapped = false;


	isOverLapped = IsOverLapped(m_info[SNT_VERTLEFT].points[1], m_info[SNT_VERTLEFT].points[2], snapInfo->m_info[SNT_VERTRIGHT].points[1], snapInfo->m_info[SNT_VERTRIGHT].points[2]);
	if (isOverLapped)
	{
		bool dxIsModified = false;
		int sx = m_info[SNT_VERTLEFT].points[0];
		int x = snapInfo->m_info[SNT_VERTRIGHT].points[0];
		if (x >= sx-gravityLength && x <= sx+gravityLength)
		{
			dx = sx-x;
			dxIsModified = true;
		}
		else
		{
			int sx = m_info[SNT_VERTRIGHT].points[0];
			int x = snapInfo->m_info[SNT_VERTLEFT].points[0];

			if (x <= sx+gravityLength && x >= sx-gravityLength)
			{
				dx = sx-x;
				dxIsModified = true;
			}
		}

		if (dxIsModified)
		{
			int sy = m_info[SNT_HORZTOP].points[0];
			int y = snapInfo->m_info[SNT_HORZTOP].points[0];
			if (y >= sy-gravityLength && y <= sy+gravityLength)
				dy = sy-y;
			else
			{
				int sy = m_info[SNT_HORZBOTTOM].points[0];
				int y = snapInfo->m_info[SNT_HORZBOTTOM].points[0];
				if (y <= sy+gravityLength && y >= sy-gravityLength)
					dy = sy-y;
			}
		}
	}



	isOverLapped = IsOverLapped(m_info[SNT_HORZTOP].points[1], m_info[SNT_HORZTOP].points[2], snapInfo->m_info[SNT_HORZBOTTOM].points[1], snapInfo->m_info[SNT_HORZBOTTOM].points[2]);
	if (isOverLapped)
	{
		bool dyIsModified = false;
		int sy = m_info[SNT_HORZTOP].points[0];
		int y = snapInfo->m_info[SNT_HORZBOTTOM].points[0];

		if (y >= sy-gravityLength && y <= sy+gravityLength)
		{
			dy = sy-y;
			dyIsModified = true;
		}
		else
		{
			int sy = m_info[SNT_HORZBOTTOM].points[0];
			int y = snapInfo->m_info[SNT_HORZTOP].points[0];

			if (y <= sy+gravityLength && y >= sy-gravityLength)
			{
				dy = sy-y;
				dyIsModified = true;
			}
		}

		if (dyIsModified)
		{
			int sx = m_info[SNT_VERTLEFT].points[0];
			int x = snapInfo->m_info[SNT_VERTLEFT].points[0];

			if (x >= sx-gravityLength && x <= sx+gravityLength)
				dx = sx-x;
			else
			{
				int sx = m_info[SNT_VERTRIGHT].points[0];
				int x = snapInfo->m_info[SNT_VERTRIGHT].points[0];

				if (x <= sx+gravityLength && x >= sx-gravityLength)
					dx = sx-x;
			}
		}
	}
}
