#include "common.h"
#include "../framework/Locale.h"
#include "DrawUtil.h"
#include "Renderer.h"
#include "../framework/Language.h"
#include "../window/UISys.h"

#include <windows.h>
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
	hFont = ::CreateFont(	fontHeight, 0, 0, 0, fontBold ? FW_BOLD : FW_NORMAL, 0, 0, 0,
							fontCharset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
							NONANTIALIASED_QUALITY, DEFAULT_PITCH, GetFontFace());
}

void	FontInfo :: Create_Eot(short FT_style)
{
	hFont = CreateEotFont(12,FT_style);
}

HFONT FontInfo::CreateEotFont(int fontSize,short FT_style)
{
	HFONT retFont;



	LOGFONT lgFnt;
	ZeroMemory(&lgFnt,sizeof(lgFnt));
	lgFnt.lfWidth = 0;
	lgFnt.lfWeight = FW_NORMAL;
	lgFnt.lfItalic=FALSE;
	lgFnt.lfUnderline=FALSE;
	lgFnt.lfStrikeOut=0;
	lgFnt.lfCharSet = DEFAULT_CHARSET;
	lgFnt.lfOutPrecision = OUT_RASTER_PRECIS;
	lgFnt.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lgFnt.lfQuality = PROOF_QUALITY;
	lgFnt.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;

	if (FT_style == FT_SQUIRREL)
	{
		strcpy(lgFnt.lfFaceName,LPCTSTR("RixSquirrel_10.eot"));
		lgFnt.lfHeight =  -MulDiv(13, 72, 72);
	}
	else if (FT_style == FT_LOVEANGEL)
	{
		strcpy(lgFnt.lfFaceName,LPCTSTR("RixLoveangel_10.eot"));
		lgFnt.lfHeight =  -MulDiv(13, 72, 72);
	}
	else if (FT_style == FT_GOGO)
	{
		strcpy(lgFnt.lfFaceName,LPCTSTR("RixNHCgogo_10.eot"));
		lgFnt.lfHeight =  -MulDiv(13, 72, 72);
	}

	else if (FT_style == FT_DIARY)
	{
		strcpy(lgFnt.lfFaceName,LPCTSTR("RixDiary_10.eot"));
		lgFnt.lfHeight =  -MulDiv(13, 72, 72);
	}
	else if (FT_style == FT_MINIHEART)
	{
		strcpy(lgFnt.lfFaceName,LPCTSTR("RixMiniHeart_10.eot"));
		lgFnt.lfHeight =  -MulDiv(13, 72, 72);
	}
	else if (FT_style == FT_FRESHMAN)
	{
		strcpy(lgFnt.lfFaceName,LPCTSTR("RixFreshman_10.eot"));
		lgFnt.lfHeight =  -MulDiv(13, 72, 72);
	}
	else if (FT_style == FT_KID)
	{
		strcpy(lgFnt.lfFaceName,LPCTSTR("RixKid_10.eot"));
		lgFnt.lfHeight =  -MulDiv(13, 72, 72);
	}
	else if (FT_style == FT_MAGIC)
	{
		strcpy(lgFnt.lfFaceName,LPCTSTR("RixMagic_10.eot"));
		lgFnt.lfHeight =  -MulDiv(13, 72, 72);
	}
	else if (FT_style == FT_JJANGU)
	{
		strcpy(lgFnt.lfFaceName,LPCTSTR("RixJJangu_10.eot"));
		lgFnt.lfHeight =  -MulDiv(13, 72, 72);
	}

	else
	{
		strcpy(lgFnt.lfFaceName,LPCTSTR("RixNHCgogo_10.eot"));
		lgFnt.lfHeight =  -MulDiv(13, 72, 72);
	}






	retFont = CreateFontIndirect(&lgFnt);

	return retFont;
}

bool FontInfo :: operator < (const FontInfo& fontInfo) {
	if(fontType < fontInfo.fontType) return true;
	else if(fontType > fontInfo.fontType) return false;

	if(fontHeight < fontInfo.fontHeight) return true;
	else if(fontHeight > fontInfo.fontHeight) return false;

	return fontCharset < fontInfo.fontCharset;
}

bool FontInfo :: operator == (const FontInfo& fontInfo) {
	return	fontType == fontInfo.fontType &&
			fontHeight == fontInfo.fontHeight &&
			fontCharset == fontInfo.fontCharset &&
			fontBold == fontInfo.fontBold
			;
}

int CALLBACK EnumFontFamExProc(CONST LOGFONT* plogFont, CONST TEXTMETRIC* textMetric, DWORD dwWord, LPARAM lParam)
{
	return stricmp((const char*)lParam, plogFont->lfFaceName);
}

const char*	FontInfo :: GetFontFace()
{
	LOGFONT logFont;

	memset(&logFont, 0, sizeof(logFont));

	logFont.lfCharSet = fontCharset;

	if(!EnumFontFamiliesEx(GetDC(NULL), &logFont, (FONTENUMPROC)EnumFontFamExProc, (INT_PTR)fontFace9x[fontType], 0))
	{
		return fontFace9x[fontType];
	}

	if(EnumFontFamiliesEx(GetDC(NULL), &logFont, (FONTENUMPROC)EnumFontFamExProc, (INT_PTR)fontFaceNT[fontType], 0) == 0)
	{
		return fontFaceNT[fontType];
	}

	return NULL;
}

FontMgr g_fontMgr;
FontMgr :: ~FontMgr()
{
	std::vector<FontInfo>::iterator iter = m_fontInfoList.begin();

	for(; iter != m_fontInfoList.end(); ++iter) DeleteObject(iter->hFont);

	m_fontInfoList.clear();
}

HFONT	FontMgr :: GetFont(int fontType, int fontHeight, int charset, bool bold)
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
:	m_hdc(NULL)
,	m_surface(surface)
{
	if(m_surface->GetDC(&m_hdc)) {
		m_oldFont = (HFONT)GetCurrentObject(m_hdc, OBJ_FONT);

		::SetBkMode(m_hdc, TRANSPARENT);
	}
}

DrawDC :: ~DrawDC()
{
	if(m_surface) {

		if(m_hdc) {
			SelectObject(m_hdc, m_oldFont);

			m_surface->ReleaseDC(m_hdc);
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
					case LANG_TURKISH:
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
	::SetTextColor(m_hdc, colorRef);
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
inline void	DrawDC :: _textout(int x, int y, const char* buf, int bufLen, int charset)
{

	s_wideBuffer.resize(bufLen);

	int wideBufferLen = MultiByteToWideChar(GetCodePageFromCharset(charset), 0, buf, bufLen, s_wideBuffer.begin(), s_wideBuffer.size());

	::SelectObject(m_hdc, (HGDIOBJ)g_fontMgr.GetFont(m_fontType, m_fontHeight, charset, m_bold));

	::TextOutW(m_hdc, x, y, s_wideBuffer.begin(), wideBufferLen);
}

inline void	DrawDC :: _gettextsize(const char* buf, int bufLen, LPSIZE size, int charset)
{
	if(bufLen <= 0) return;

	s_wideBuffer.resize(bufLen);

	int wideBufferLen = MultiByteToWideChar(GetCodePageFromCharset(charset), 0, buf, bufLen, s_wideBuffer.begin(), s_wideBuffer.size());

	::SelectObject(m_hdc, (HGDIOBJ)g_fontMgr.GetFont(m_fontType, m_fontHeight, charset, m_bold));

	::GetTextExtentPoint32W(m_hdc, s_wideBuffer.begin(), wideBufferLen, size);
}
