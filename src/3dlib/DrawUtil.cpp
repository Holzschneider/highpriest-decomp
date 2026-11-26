#include "common.h"
#include "../framework/Locale.h"
#include "DrawUtil.h"
#include "Renderer.h"
#include "../framework/Language.h"
#include "../window/UISys.h"
#include "bf_coverage.h"

#include "bf_types.h"
#include <algorithm>



extern CRenderer*g_renderer;
extern FontMgr   g_fontMgr;




extern LanguageType g_languageType;
extern ServiceType  g_serviceType;






char* fontFaceNT[] =  {
						"Gulim",
						"Arial",
						"MS Gothic",
						"SimSun",
						"MingLiU",
						"Tahoma",
						"Arial",
						"Verdana",
						"Verdana",
						"Arial",
						"Arial",
								};











char* fontFace9x[] =  {
						"\xB1\xBC\xB8\xB2",
						"Arial",
						"\x82\x6C\x82\x72\x20\x82\x6F\x83\x53\x83\x56\x83\x62\x83\x4E",
						"\xCB\xCE\xCC\xE5",
						"\xB2\xD3\xA9\xFA\xC5\xE9",
						"Tahoma",
						"Arial",
						"Verdana",
						"Verdana",
						"Arial",
						"Arial",
								};

DWORD g_fontCharSet[] =  {
							HANGUL_CHARSET,
							ANSI_CHARSET,
							SHIFTJIS_CHARSET,
							GB2312_CHARSET,
							CHINESEBIG5_CHARSET,
							THAI_CHARSET,
							TURKISH_CHARSET,
							ANSI_CHARSET,
							ANSI_CHARSET,
							RUSSIAN_CHARSET,
							VIETNAMESE_CHARSET,
											};

bool    g_isHex[]   = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,

	0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

char    g_hexToDec[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

int isHex(char c)
{
	return g_isHex[(BYTE)c];
}

int	FindCharSet(const char* buf, int bufLen)
{
	if(g_multiLang) {
		int len = min(bufLen,strlen(buf));
		for(int i=0; i<len-2; ++i) {
			if(buf[i] == '|' && g_isHex[(BYTE)buf[i+1]] && g_isHex[(BYTE)buf[i+2]]) {
				return i;
			}
		}
	}

	return bufLen;
}

int		ReadCharSet(const char* buf)
{
	return g_hexToDec[(BYTE)buf[1]]*0x10 + g_hexToDec[(BYTE)buf[2]];
}

void	DrawBoxScreen(int x, int y, int cx, int cy, DWORD color)
{
	if (color & 0xff000000)
	{
		RPQuadFace& rpFace = g_renderer->BorrowQuadRP();

		rpFace.tex = g_texMgr.GetDummyTexture();

		tlvertex3d*	sprVert = rpFace.m_verts;


		sprVert[0].x = x;
		sprVert[0].y = y;
		sprVert[0].z = 0.000001f;
		sprVert[0].oow = 0.999999f;
		sprVert[0].tu = 0;
		sprVert[0].tv = 0;
		sprVert[0].color = color;
		sprVert[1].x = x+cx;
		sprVert[1].y = y;
		sprVert[1].z = 0.000001f;
		sprVert[1].oow = 0.999999f;
		sprVert[1].tu = 0;
		sprVert[1].tv = 0;
		sprVert[1].color = color;
		sprVert[2].x = x;
		sprVert[2].y = y+cy;
		sprVert[2].z = 0.000001f;
		sprVert[2].oow = 0.999999f;
		sprVert[2].tu = 0;
		sprVert[2].tv = 0;
		sprVert[2].color = color;
		sprVert[3].x = x+cx;
		sprVert[3].y = y+cy;
		sprVert[3].z = 0.000001f;
		sprVert[3].oow = 0.999999f;
		sprVert[3].tu = 0;
		sprVert[3].tv = 0;
		sprVert[3].color = color;

		g_renderer->AddRP(&rpFace, RF_UI);
	}
}

FontInfo :: FontInfo(int type, int height, int charset, bool bold)
: fontType(type), fontHeight(height), fontCharset(charset), fontBold(bold), hFont(NULL)
{
}

void	FontInfo :: Create()
{
	// The font CreateFont made of (fontHeight, 0, 0, 0, bold ? FW_BOLD :
	// FW_NORMAL, 0, 0, 0, fontCharset, ..., NONANTIALIASED_QUALITY, ..., face):
	// a cell height, never smoothed; the face name is in the code page of the
	// character set (the 9x names), and no face lets bf_text pick one by it.
	const char* face = GetFontFace();
	char faceUtf8[128] = { 0 };
	if(face && face[0]) {
		if(bf_text_to_utf8(bf_text_codepage_for_charset(fontCharset), face, -1, faceUtf8, sizeof(faceUtf8) - 1) < 0)
			bf_text_to_utf8(BF_TEXT_CP_WESTERN, face, -1, faceUtf8, sizeof(faceUtf8) - 1);
	}
	bf_text_font_desc desc = { faceUtf8[0] ? faceUtf8 : NULL, fontHeight, fontBold ? FW_BOLD : FW_NORMAL, 0, 0, 0, fontCharset };
	hFont = bf_text_font_create(&desc);
}

void	FontInfo :: Create_Eot(short FT_style)
{
	hFont = CreateEotFont(12,FT_style);
}

bf_text_font* FontInfo::CreateEotFont(int fontSize,short FT_style)
{
	const char* face;

	if (FT_style == FT_SQUIRREL)
		face = "RixSquirrel_10.eot";
	else if (FT_style == FT_LOVEANGEL)
		face = "RixLoveangel_10.eot";
	else if (FT_style == FT_GOGO)
		face = "RixNHCgogo_10.eot";
	else if (FT_style == FT_DIARY)
		face = "RixDiary_10.eot";
	else if (FT_style == FT_MINIHEART)
		face = "RixMiniHeart_10.eot";
	else if (FT_style == FT_FRESHMAN)
		face = "RixFreshman_10.eot";
	else if (FT_style == FT_KID)
		face = "RixKid_10.eot";
	else if (FT_style == FT_MAGIC)
		face = "RixMagic_10.eot";
	else if (FT_style == FT_JJANGU)
		face = "RixJJangu_10.eot";
	else
		face = "RixNHCgogo_10.eot";

	// CreateFontIndirect's LOGFONT: height -MulDiv(13, 72, 72), FW_NORMAL,
	// DEFAULT_CHARSET. No .eot face is installed: bf_text falls back.
	bf_text_font_desc desc = { face, -13, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET };

	return bf_text_font_create(&desc);
}

bool FontInfo :: operator < (const FontInfo& fontInfo) const {
	if(fontType < fontInfo.fontType) return true;
	else if(fontType > fontInfo.fontType) return false;

	if(fontHeight < fontInfo.fontHeight) return true;
	else if(fontHeight > fontInfo.fontHeight) return false;

	return fontCharset < fontInfo.fontCharset;
}

bool FontInfo :: operator == (const FontInfo& fontInfo) const {
	return	fontType == fontInfo.fontType &&
			fontHeight == fontInfo.fontHeight &&
			fontCharset == fontInfo.fontCharset &&
			fontBold == fontInfo.fontBold
			;
}

const char*	FontInfo :: GetFontFace()
{
	// The 9x name if Windows lists it under the character set, else the NT
	// name, else none (EnumFontFamiliesEx with a callback that matched the
	// name). The font types past the tables (FT_SQUIRREL...) have no face.
	if(fontType < 0 || fontType >= (int)(sizeof(fontFaceNT) / sizeof(fontFaceNT[0])))
		return NULL;

	if(bf_text_face_listed(fontFace9x[fontType], fontCharset))
	{
		return fontFace9x[fontType];
	}

	if(bf_text_face_listed(fontFaceNT[fontType], fontCharset))
	{
		return fontFaceNT[fontType];
	}

	return NULL;
}

FontMgr g_fontMgr;
FontMgr :: ~FontMgr()
{
	std::vector<FontInfo>::iterator iter = m_fontInfoList.begin();

	for(; iter != m_fontInfoList.end(); ++iter) bf_text_font_destroy(iter->hFont);

	m_fontInfoList.clear();
}

bf_text_font*	FontMgr :: GetFont(int fontType, int fontHeight, int charset, bool bold)
{
	FontInfo fontInfo(fontType, fontHeight, charset, bold);

	std::vector<FontInfo>::iterator iter = std::find(m_fontInfoList.begin(), m_fontInfoList.end(), fontInfo);

	if(iter != m_fontInfoList.end())
	{
		return iter->hFont;
	}
	else
	{
		fontInfo.Create();

		m_fontInfoList.push_back(fontInfo);
		std::sort(m_fontInfoList.begin(), m_fontInfoList.end());

		return fontInfo.hFont;
	}
}

DrawDC :: DrawDC(CDC* surface)
:	m_canvas(NULL)
,	m_surface(surface)
{
	if(!m_surface->GetDC(&m_canvas))
		m_canvas = NULL;
}

DrawDC :: ~DrawDC()
{
	if(m_surface) {

		if(m_canvas) {
			m_surface->ReleaseDC(m_canvas);
		}
	}
}

void	DrawDC :: SetFont(int fontType, int fontHeight, bool bold)
{
	if(fontType==0) {
		switch(g_languageType) {
		case LanguageEnglish:

			if ( ServiceIndonesia == g_serviceType )
			{
				fontType = 8;
				fontHeight += 2;
			}
			else if( g_serviceType == ServiceRussia )
			{
				fontType = 9;
				fontHeight += 3;
			}
			else
			{
				fontType = 1;
				fontHeight += 3;
				if ( g_language )
				{
					switch(g_language->m_scriptLangId)
					{
					case 0x1f:	// LANG_TURKISH
						fontType = 6;
						break;
					}
				}
			}
			break;
		case LanguageJapanese:
			fontType = 2;
			break;
		case LanguageSimplifiedChinese:
			fontType = 3;
			break;
		case LanguageTraditionalChinese:
			fontType = 4;
			break;
		case LanguageThai:
			fontType = 5;
			fontHeight += 3;
			break;
		case LanguagePortuguese:
			fontType = 7;
			fontHeight += 3;
			if ((ServiceBrazil == g_serviceType) || (ServiceFrance == g_serviceType))
				fontHeight -= 2;

			break;
		case LanguageVietnam:
			fontType	= 10;
			fontHeight += 3;
			break;
#if defined(BF_PORT)
		case LanguageKorean:
			// Font 0 is the Korean font: nothing to choose. The original had
			// no case, so the Korean service (sclientinfo.xml absent or
			// naming it) asserted on every text drawn (about 4,400 in 25 s).
			break;
#endif
		default:
			sAssertM(false, "Unknown Language Type !!");
			break;
		};
	}

	if (LanguageKorean == g_languageType && fontType >= FT_SQUIRREL)
	{
		m_fontType		= fontType;
		m_fontHeight	= fontHeight;
		m_charset		= g_fontCharSet[0];
		m_bold			= bold;
	}
	else
	{
		m_fontType		= fontType;
		m_fontHeight	= fontHeight;
		m_charset		= g_fontCharSet[fontType];
		m_bold			= bold;
	}
}

void	DrawDC :: SetTextColor(DWORD colorRef)
{
	if(m_canvas)
		m_canvas->color = bf_text_color_from_colorref(colorRef);
}

void	DrawDC :: SingleLangTextOut(int x, int y, const char* buf, int bufLen)
{
	int charSetOffset = FindCharSet(buf, bufLen);

	_textout(x, y, buf, charSetOffset, m_charset);

	if(charSetOffset < bufLen) {

		SIZE size;

		_gettextsize(buf, charSetOffset, &size, m_charset);

		int charset = ReadCharSet(buf+charSetOffset);

		_textout(x+size.cx, y, buf+(charSetOffset+3), bufLen-(charSetOffset+3), charset);
	}
}

void	DrawDC :: TextOutA(int x, int y, const char* buf, int bufLen)
{
	if ( g_multiLang )
	{
		const char * out = buf;
		int charSetOffset = 0;
		int posX = x;
		int posY = y;
		SIZE size;
		int outLen = bufLen;

		while(charSetOffset < bufLen)
		{
			charSetOffset += FindCharSet(buf+charSetOffset, outLen);
			if ( charSetOffset >= bufLen )
				SingleLangTextOut(posX, posY, out, charSetOffset);
			else
			{
				if( outLen <= 3 )
				{
					sAssert(outLen >= 3);
					break;
				}
				int len = 0;
				len = FindCharSet(buf+charSetOffset+3, outLen-3) + 3;
				SingleLangTextOut(posX, posY, out, len);
				GetTextExtentPoint32A(out,len,&size);
				posX += size.cx;
				charSetOffset += len;
				out += len;
				outLen -= len;
			}
		}
	}
	else
		SingleLangTextOut(x,y,buf,bufLen);
}

void	DrawDC :: GetTextExtentPoint32A(const char* buf, int bufLen, LPSIZE size)
{
	int charSetOffset = FindCharSet(buf, bufLen);

	_gettextsize(buf, charSetOffset, size, m_charset);

	if(charSetOffset < bufLen) {

		SIZE right;

		int charset = ReadCharSet(buf+charSetOffset);

		_gettextsize(buf+(charSetOffset+3), bufLen-(charSetOffset+3), &right, charset);

		size->cx += right.cx;
		size->cy = max(size->cy, right.cy);

	}
}


std::vector<unsigned short> DrawDC::s_wideBuffer;
// The text as UTF-16 in s_wideBuffer, as MultiByteToWideChar converted it:
// 0 units for no text, an unknown code page or a result over bufLen units.
static int WideText(std::vector<unsigned short>& wide, int codePage, const char* buf, int bufLen)
{
	if(!buf || bufLen <= 0) return 0;
	int n = bf_text_to_utf16(codePage, buf, bufLen, NULL, 0);
	if(n <= 0 || n > (int)wide.size()) return 0;
	return bf_text_to_utf16(codePage, buf, bufLen, wide.data(), wide.size());
}

inline void	DrawDC :: _textout(int x, int y, const char* buf, int bufLen, int charset)
{

	s_wideBuffer.resize(bufLen);

	int wideBufferLen = WideText(s_wideBuffer, GetCodePageFromCharset(charset), buf, bufLen);

	bf_text_font* font = g_fontMgr.GetFont(m_fontType, m_fontHeight, charset, m_bold);

	if(!m_canvas || !font || wideBufferLen <= 0) return;

	// The coverage report (M9.5): Korean or other non-Windows-1252 text drawn.
	if(m_canvas->bitmap.pixels)
		bf_coverage_drawn(BF_COVERAGE_UTF16, s_wideBuffer.data(), wideBufferLen);

	bf_text_canvas_draw(m_canvas, font, x, y, BF_TEXT_CP_UTF16, s_wideBuffer.data(), wideBufferLen);
}

inline void	DrawDC :: _gettextsize(const char* buf, int bufLen, LPSIZE size, int charset)
{
	if(bufLen <= 0) return;

	s_wideBuffer.resize(bufLen);

	int wideBufferLen = WideText(s_wideBuffer, GetCodePageFromCharset(charset), buf, bufLen);

	bf_text_font* font = g_fontMgr.GetFont(m_fontType, m_fontHeight, charset, m_bold);

	if(!m_canvas || !font) return;

	int cx = 0, cy = 0;
	if(bf_text_extent(font, BF_TEXT_CP_UTF16, wideBufferLen ? (const void*)s_wideBuffer.data() : (const void*)"", wideBufferLen, &cx, &cy) < 0) return;
	size->cx = cx;
	size->cy = cy;
}
