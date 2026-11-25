#ifndef HIGHPRIEST_3DLIB_DRAWUTIL_H
#define HIGHPRIEST_3DLIB_DRAWUTIL_H

#include "Renderer.h"
void DrawBoxScreen(int x, int y, int cx, int cy, unsigned int color);
int FindCharSet(const char* buf, int bufLen);
int		ReadCharSet(const char* buf);
int		isHex(char c);


class DrawDC {
public:
	DrawDC(CDC* surface);
	~DrawDC();
	void SetFont(int fontType, int fontHeight, bool bold);
	void SetTextColor(unsigned int colorRef);
	void TextOutA(int x, int y, const char* buf, int bufLen);
	void SingleLangTextOut(int x, int y, const char* buf, int bufLen);
	void GetTextExtentPoint32A(const char* buf, int bufLen, tagSIZE* size);
protected:
	void _textout(int x, int y, const char* buf, int bufLen, int charset);
	void _gettextsize(const char* buf, int bufLen, tagSIZE* size, int charset);
	static std::vector<unsigned short> s_wideBuffer;
protected:
	bf_text_canvas* m_canvas;
	CDC* m_surface;
	int m_fontType;
	int m_fontHeight;
	int m_charset;
	int m_fontWeight;
	bool m_vertical;
	bool m_bold;
};

struct FontInfo {
public:
	FontInfo(int type, int height, int charset, bool bold);
	void Create();
	void Create_Eot(short FT_style);
	bf_text_font* CreateEotFont(int fontSize, short FT_style);
	const char* GetFontFace();
	bool operator<(const FontInfo& fontInfo) const;
	bool operator==(const FontInfo& fontInfo) const;
	int fontType;
	int fontHeight;
	int fontCharset;
	unsigned char fontBold;
	bf_text_font* hFont;
};

class FontMgr {
public:
	~FontMgr();
	bf_text_font* GetFont(int fontType, int fontHeight, int charset, bool bold);
protected:
	std::vector<FontInfo> m_fontInfoList;
};

#endif
