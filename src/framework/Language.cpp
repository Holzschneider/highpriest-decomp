#include "common.h"
#include "Locale.h"
#include "Language.h"
#include "ftwbrk.h"
#include "../base/File.h"
#include "../base/Res.h"

#include <windows.h>
#include <imm.h>
#include <minmax.h>
#include <string>
#include <list>

using std::string;

#define MAX_CANDIDATE_PAGE_SIZE 9




extern DWORD       g_codePage;
extern ServiceType g_serviceType;


int	GetCharsetFromLang( LANGID langid )
{
	switch( PRIMARYLANGID(langid) )
	{
	case LANG_JAPANESE:
		return SHIFTJIS_CHARSET;
	case LANG_KOREAN:
		return HANGEUL_CHARSET;
	case LANG_CHINESE:
		{
			switch( SUBLANGID(langid) )
			{
			case SUBLANG_CHINESE_SIMPLIFIED:
				return GB2312_CHARSET;
			case SUBLANG_CHINESE_TRADITIONAL:
				return CHINESEBIG5_CHARSET;
			}
		}
	case LANG_GREEK:
		return GREEK_CHARSET;

	case LANG_TURKISH:
		return TURKISH_CHARSET;

	case LANG_HEBREW:
		return HEBREW_CHARSET;

	case LANG_ARABIC:
		return ARABIC_CHARSET;

	case LANG_ESTONIAN:
	case LANG_LATVIAN:
	case LANG_LITHUANIAN:
		return BALTIC_CHARSET;

	case LANG_RUSSIAN:
	case LANG_BELARUSIAN:
	case LANG_BULGARIAN:
	case LANG_UKRAINIAN:
		return RUSSIAN_CHARSET;

	case LANG_THAI:
		return THAI_CHARSET;

	case LANG_CZECH:
	case LANG_HUNGARIAN:
	case LANG_POLISH:
	case LANG_CROATIAN:
	case LANG_MACEDONIAN:
	case LANG_ROMANIAN:
	case LANG_SLOVAK:
	case LANG_SLOVENIAN:
		return EASTEUROPE_CHARSET;

	case LANG_VIETNAMESE:
		return VIETNAMESE_CHARSET;

	case LANG_PORTUGUESE:
		return ANSI_CHARSET;

	default:
		return ANSI_CHARSET;
	}
}

int	GetCodePageFromCharset( int charset )
{
	switch( charset )
	{
	case SHIFTJIS_CHARSET:
		return 932;

	case HANGUL_CHARSET:
		return 949;

	case GB2312_CHARSET:
		return 936;

	case CHINESEBIG5_CHARSET:
		return 950;

	case GREEK_CHARSET:
		return 1253;

	case TURKISH_CHARSET:
		return 1254;

	case RUSSIAN_CHARSET:
		return 1251;

	case VIETNAMESE_CHARSET:
		return 1258;

	case HEBREW_CHARSET:
		return 1255;

	case ARABIC_CHARSET:
		return 1256;

	case BALTIC_CHARSET:
		return 1257;

	case THAI_CHARSET:
		return 874;

	case EASTEUROPE_CHARSET:
		return 1250;

	default:
		return 1252;
	}
}

int	GetCodePageFromLang( LANGID langid )
{
	return GetCodePageFromCharset(GetCharsetFromLang(langid));
}

CLanguage :: CLanguage()
:	m_candidateList(NULL),
	m_selectionCursor(0),
	m_selectionOrigin(0),
	m_candidateCount(0),
	m_candidatePageSize(0),
	m_candidateSelection(0),
	m_languageFolder(""),
	m_underLineStart(0),
	m_underLineEnd(0),
	m_langId(LOWORD(GetKeyboardLayout(0)))
{
	m_codePage = g_multiLang ? (GetCodePageFromLang(m_langId)) : g_codePage;

	int langId = PRIMARYLANGID(GetUserDefaultLangID());

	switch( langId )
	{
	case LANG_ITALIAN:
		m_scriptLangId = LANG_ITALIAN;
		m_languageFolder = "italian\\";
		break;
	case LANG_PORTUGUESE:
		m_scriptLangId = LANG_PORTUGUESE;
		m_languageFolder = "portuguese\\";
		break;
	case LANG_TURKISH:
		m_scriptLangId = LANG_TURKISH;
		m_languageFolder = "turkish\\";
		break;
	case LANG_GERMAN:
		m_scriptLangId = LANG_GERMAN;
		m_languageFolder = "german\\";
		break;
	default:
		m_scriptLangId = LANG_ENGLISH;
		m_languageFolder = "english\\";
		break;
	}

	if (ServiceVietnam == g_serviceType)
		m_codePage = 1258;
}





CLanguage :: ~CLanguage()
{
}

void CLanguage :: GetLanguageText(string* text)
{
	*text = m_input;

	if(GetCompositionStringSize()) text->insert(m_selectionCursor, GetCompositionString());
}

const char * CLanguage :: LanguageFile(const char * fName)
{
	if ( ServiceKorea != g_serviceType )
	{
		static char result[MAX_PATH];
		strcpy(result,m_languageFolder.c_str());
		strcat(result,fName);

		bool bTxt = strstr(fName,".txt") || strstr(fName,".xml");

		if (
			( bTxt && CFile::IsFileExist(result) ) ||
			( !bTxt && g_resMgr().IsExist(result) )
			)
			return result;
		else
			return fName;
	}
	else
		return fName;
}

const char* CLanguage :: GetLanguageCharset(bool bScript)
{
	static char charsetString[16];

	if(g_multiLang) {

		sprintf(charsetString, "|%02x", GetCharsetFromLang(bScript ? m_scriptLangId : m_langId));

	} else {
		charsetString[0] = 0;

	}

	return charsetString;
}

void CLanguage :: ResetInput()
{
	CancelComposition();
	m_input.resize(0);
	m_selectionOrigin = m_selectionCursor = m_input.size();
	m_keyStroke.resize(0);
	m_compStr.resize(0);
	m_candidateCount = 0;
}

void CLanguage :: GetSelection(int* origin, int* cursor)
{
	*origin = m_selectionOrigin;
	*cursor = m_selectionCursor;
}

void CLanguage :: GetSelectionRange(int* start, int* end)
{
	if(m_selectionOrigin < m_selectionCursor) {
		*start = m_selectionOrigin;
		*end = m_selectionCursor;
	} else {
		*start = m_selectionCursor;
		*end = m_selectionOrigin;
	}
}

void CLanguage :: SetSelection(int origin, int cursor)
{
	m_selectionOrigin = origin;
	m_selectionCursor = cursor;

	if(m_selectionOrigin < 0) m_selectionOrigin = 0;
	if(m_selectionOrigin > m_input.size()) m_selectionOrigin = m_input.size();
	if(m_selectionCursor < 0) m_selectionCursor = 0;
	if(m_selectionCursor > m_input.size()) m_selectionCursor = m_input.size();
}

const char* CLanguage :: GetCandidateString(int nCandidate)
{
	if(nCandidate < m_candidateCount) {

		wchar_t*	candidateStrW = (wchar_t*)(&m_candidateBuffer[m_candidateList->dwOffset[nCandidate]]);

		static char	candidateStr[256];
		int			candidateStrLen = WideCharToMultiByte(m_codePage, 0, candidateStrW, -1, candidateStr, sizeof(candidateStr), NULL, NULL);

		candidateStr[min(candidateStrLen, sizeof(candidateStr)-1)] = 0;

		return candidateStr;
	} else {
		return "";
	}
}

LRESULT CLanguage :: ProcessImeMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, bool* trapped)
{
	switch (message) {
		case WM_INPUTLANGCHANGE:
			m_langId = LOWORD(lParam);
			m_codePage = g_multiLang ? (GetCodePageFromLang(m_langId)) : g_codePage;
			break;

		case WM_IME_STARTCOMPOSITION:
			BeginKeyStroke();
			DeleteSelection();
			break;

		case WM_IME_COMPOSITION:
			if(lParam&GCS_COMPSTR) {
				GetImeCompStr();
				ClearKeyStroke();
			}
			if(lParam&GCS_CURSORPOS) {
				GetImeCursorPos();
			}
			if(lParam&GCS_RESULTSTR)
			{
				GetImeResultStr();
			}
			if(lParam&GCS_COMPATTR) {
				GetImeCompAttr();
			}
			break;

		case WM_IME_ENDCOMPOSITION:
			EndKeyStroke();
			m_compStr = "";
			break;

		case WM_IME_NOTIFY:
			switch (wParam) {
				case IMN_OPENCANDIDATE:
				case IMN_CHANGECANDIDATE:
					GetImeCandidateList();
					break;

				case IMN_CLOSECANDIDATE:
					m_candidateCount = 0;
					break;
			}
			break;
		default:
		*trapped = 0;
		return 0L;
	}

	*trapped = 1;
	return 0L;
}

void CLanguage :: OnChar(WPARAM wParam, LPARAM lParam)
{
	switch(wParam) {
	case '\r':
	case '\n':
		break;
	case '\t':
	case 27:
		break;
	case '\b':
		if(!GetSelectionSize()) DecreaseSelection();
		DeleteSelection();
		break;
	default:
		if(wParam > 31) {
			AddInput((const char*)&wParam);
		}
		break;
	}
}

void CLanguage :: OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	if(HIBYTE(GetKeyState(VK_SHIFT))) {
		switch(wParam) {
		case VK_LEFT:
			DecreaseSelection();
			break;
		case VK_RIGHT:
			IncreaseSelection();
			break;
		case VK_HOME:
			m_selectionCursor = 0;
			break;
		case VK_END:
			m_selectionCursor = m_input.size();
			break;
		case VK_DELETE:
			CutTextToClipBoard();
			break;
		case VK_INSERT:
			CopyTextFromClipBoard();
			break;
		}
	} else if(HIBYTE(GetKeyState(VK_CONTROL))) {
		switch(wParam) {
		case 'X':
			CutTextToClipBoard();
			break;
		case 'C':
			CopyTextToClipBoard();
			break;
		case 'V':
			CopyTextFromClipBoard();
			break;
		}
	} else {
		switch(wParam) {
		case VK_DELETE:
			if(!GetSelectionSize()) IncreaseSelection();
			DeleteSelection();
			break;
		case VK_LEFT:
			DecreaseSelection();
			CancelSelection();
			break;
		case VK_RIGHT:
			IncreaseSelection();
			CancelSelection();
			break;
		case VK_HOME:
			m_selectionCursor = 0;
			CancelSelection();
			break;
		case VK_END:
			m_selectionCursor = m_input.size();
			CancelSelection();
			break;
		}
	}
}

void CLanguage :: HideText(bool bHideText)
{
	m_bHideText = bHideText;
}

const char* CLanguage :: GetWordBreak(const char* text, unsigned int size)
{
	return CharNextExA(g_codePage, text, 0);
}

void CLanguage :: GetImeCompStr()
{
	HIMC himc = ImmGetContext(g_hMainWnd);
	if(!himc) { m_compStr.resize(0); return; }

	wchar_t	compStrW[256];
	int		compStrWLen = ImmGetCompositionStringW(himc, GCS_COMPSTR, compStrW, sizeof(compStrW))/sizeof(wchar_t);

	char	compStr[256];
	int		compStrLen = WideCharToMultiByte(m_codePage, 0, compStrW, compStrWLen, compStr, sizeof(compStr), NULL, NULL);

	ImmReleaseContext(g_hMainWnd, himc);

	m_compStr.assign(compStr, compStr+compStrLen);
}



void CLanguage :: GetImeResultStr()
{
	HIMC himc = ImmGetContext(g_hMainWnd);
	if(!himc) { m_compStr.resize(0); return; }

	wchar_t	resultStrW[256];

	int resultStrWLen = ImmGetCompositionStringW(
		himc,
		GCS_RESULTSTR,
		resultStrW,
		sizeof( resultStrW )
		) / sizeof(wchar_t);

	char resultStr[256];
	int resultStrLen = WideCharToMultiByte(
		m_codePage,
		0,
		resultStrW,
		resultStrWLen,
		resultStr,
		sizeof(resultStr),
		NULL, NULL );
	resultStr[resultStrLen] = 0;

	ImmReleaseContext(g_hMainWnd, himc);

	AddInput((const char*)resultStr);
}

void CLanguage :: GetImeCompAttr()
{

}

void CLanguage :: GetImeCursorPos()
{
	HIMC himc = ImmGetContext(g_hMainWnd);
	if(!himc) { m_compStr.resize(0); return; }

	ImmGetCompositionString(himc, GCS_CURSORPOS, &m_compCursorPos, sizeof(m_compCursorPos));

	ImmReleaseContext(g_hMainWnd, himc);
}

void CLanguage :: GetImeCandidateList()
{
	HIMC himc = ImmGetContext(g_hMainWnd);

	if(himc) {

		int candidateListSize = ImmGetCandidateListW( himc, 0, NULL, 0 );

		if(candidateListSize > 0) {

			m_candidateBuffer.resize(candidateListSize);
			m_candidateList = (CANDIDATELIST*)(m_candidateBuffer.begin());

			ImmGetCandidateListW(himc, 0, m_candidateList, candidateListSize);

			m_candidateCount = m_candidateList->dwCount;
			m_candidateSelection = m_candidateList->dwSelection;
			m_candidatePageSize = min(m_candidateList->dwPageSize, MAX_CANDIDATE_PAGE_SIZE);
		} else {
			m_candidateCount = 0;
		}

		ImmReleaseContext(g_hMainWnd, himc);

	} else {
		m_candidateCount = 0;
	}
}

void CLanguage :: CancelComposition()
{
	HIMC himc = ImmGetContext(g_hMainWnd);

	if(!himc) return;

	ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);

	ImmReleaseContext(g_hMainWnd, himc);
}

void CLanguage :: AddInput(const char* input)
{
	DeleteSelection();
	m_input.insert(m_selectionCursor, input);
	m_selectionCursor = m_selectionOrigin = m_selectionCursor + strlen(input);
}

void CLanguage :: CopyTextToClipBoard()
{
	if(!GetSelectionSize()) return;

	if (!OpenClipboard(g_hMainWnd)) return;

	HANDLE hText = GlobalAlloc(GMEM_DDESHARE, GetSelectionSize()+1);

	if(hText) {
		LPSTR lpText = (LPSTR)GlobalLock(hText);

		if(lpText) {
			int start, end;
			if(m_selectionOrigin < m_selectionCursor) {
				start = m_selectionOrigin;
				end = m_selectionCursor;
			} else {
				start = m_selectionCursor;
				end = m_selectionOrigin;
			}
			if(!m_bHideText)
				memcpy(lpText, m_input.c_str()+start, end);
			else
				memset(lpText, '*', m_input.size()-start);
			lpText[GetSelectionSize()] = 0;
			GlobalUnlock(hText);

			EmptyClipboard();
			SetClipboardData(CF_TEXT, hText);
		}
	}

	CloseClipboard();
}

void CLanguage :: CopyTextFromClipBoard()
{
	if(!OpenClipboard(g_hMainWnd)) return;

	HANDLE hText = GetClipboardData(CF_TEXT);

	if(hText) {

		LPSTR lpText = (LPSTR)GlobalLock(hText);

		if(lpText) {
			AddInput(lpText);
			GlobalUnlock(hText);
		}
	}

	CloseClipboard();
}

void CLanguage :: CutTextToClipBoard()
{
	CopyTextToClipBoard();
	DeleteSelection();
}

void CLanguage :: IncreaseSelection()
{
	if(m_selectionCursor == m_input.size())
		return;
	else if(m_selectionCursor > m_input.size())
		m_selectionCursor = m_input.size();
	else
		m_selectionCursor = CharNextExA(m_codePage, m_input.c_str() + m_selectionCursor, 0) - m_input.c_str();
}

void CLanguage :: DecreaseSelection()
{
	if(m_selectionCursor == 0)
		return;
	else if(m_selectionCursor < 0)
		m_selectionCursor = 0;
	else
		m_selectionCursor = CharPrevExA(m_codePage, m_input.c_str(), m_input.c_str() + m_selectionCursor, 0) - m_input.c_str();
}

void CLanguage :: DeleteSelection()
{
	if(m_selectionCursor == m_selectionOrigin) return;

	int start = min(m_selectionOrigin, m_selectionCursor);
	m_input.erase(start, GetSelectionSize());
	m_selectionCursor = m_selectionOrigin = start;
}

void CLanguage :: MoveUpSelection( int newPos )
{
	if(m_selectionCursor == 0)
		return;
	else if(m_selectionCursor < 0)
		m_selectionCursor = 0;
	else
	{
		if ( m_selectionCursor - newPos >= 0 )
			m_selectionCursor -= newPos;
	}

	if ( !HIBYTE(GetKeyState(VK_SHIFT)) )
		m_selectionOrigin = m_selectionCursor;
}

void CLanguage :: MoveDownSelection( int newPos )
{
	if(m_selectionCursor == m_input.size())
		return;
	else if(m_selectionCursor > m_input.size())
		m_selectionCursor = m_input.size();
	else
	{
		if ( m_selectionCursor + newPos <= m_input.size() )
			m_selectionCursor += newPos;
	}

	if ( !HIBYTE(GetKeyState(VK_SHIFT)) )
		m_selectionOrigin = m_selectionCursor;
}



CLanguageTC :: CLanguageTC()
:	m_tcKeyStroke(0)
{
	BYTE text[] = "\xa3\x74\xa3\x78\xa3\xbe\xa3\xbf\xa3\xa4\xa3\xbd\xa3\xbb\xa3\xab\xa3\xaf\xa3\xb3\xa3\x76\xa3\xa7\x3f\x3f\x3f\xa3\xa2\xa3\xa6\xa3\xa9\xa3\xac\xa3\xb9\xa3\xad\xa3\xb1\xa3\xaa\xa3\xba\xa3\xb0\xa3\xb4\xa3\x75\xa3\xa1\xa3\x7a\xa3\xa5\x3f\xa3\xa3\xa3\x79\x3f\xa3\xa8\xa3\x77\xa3\xb7\xa3\xb5\xa3\xae\xa3\xb2\xa3\xb6";
	BYTE key[] = {	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
					'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
					'K', 'L', 'N', 'M', 'O', 'P', 'Q', 'R', 'S', 'T',
					'U', 'V', 'W', 'X', 'Y', 'Z', 0xbd, 0xba, 0xbc, 0xbe, 0xbf };
	BYTE offset[] = "11441443331111111132331233111121111133333";
	int i;
	for(i=0; i<256; ++i) {
		m_tcKeyStrokeTable[i].offset = 4;
	}

	for(i=0; i<sizeof(key); ++i) {
		m_tcKeyStrokeTable[key[i]].text[0] = text[i*2];
		m_tcKeyStrokeTable[key[i]].text[1] = text[i*2+1];
		m_tcKeyStrokeTable[key[i]].text[2] = 0;
		m_tcKeyStrokeTable[key[i]].offset = (offset[i] - '1');
	}
}


const char*	CLanguageTC :: GetKeyStroke()
{
	char keyStroke[] = "????";

	for(std::list<BYTE>::iterator iter = m_tcKeyStrokeList.begin(); iter != m_tcKeyStrokeList.end(); ++iter)
	{
		int a = *iter;
		TCINPUTKEY& tcInputKey = m_tcKeyStrokeTable[a];
		if(tcInputKey.offset < 4)
			memcpy(keyStroke+tcInputKey.offset*2, tcInputKey.text, 2);
	}
	m_keyStroke.assign(keyStroke, 8);

	return CLanguage::GetKeyStroke();
}

int CLanguageTC :: GetKeyStrokeSize()
{
	if(m_tcKeyStrokeList.size() == 1 && m_tcKeyStrokeTable[*m_tcKeyStrokeList.rbegin()].offset == 4) return 0;

	return m_tcKeyStrokeList.size();
}




void CLanguageTC :: GetImeCompStr()
{
	CLanguage::GetImeCompStr();

	if(m_compStr.size() > 2)
		if(!strcmp(m_compStr.c_str()+m_compStr.size()-2, "?")) m_compStr.resize(m_compStr.size()-2);
}

void CLanguageTC :: SetKeyStroke(BYTE key)
{
	switch(key)
	{
	case VK_BACK:
		if(m_tcKeyStrokeList.size()) m_tcKeyStrokeList.resize(m_tcKeyStrokeList.size()-1);
		break;
	default:
		for(std::list<BYTE>::iterator iter = m_tcKeyStrokeList.begin(); iter != m_tcKeyStrokeList.end(); ++iter)
		{
			if(m_tcKeyStrokeTable[*iter].offset == m_tcKeyStrokeTable[key].offset)
			{
				m_tcKeyStrokeList.erase(iter);
				break;
			}
		}
		m_tcKeyStrokeList.push_back(key);
		break;
	}
}

void CLanguageTC :: ClearKeyStroke()
{
	if(!m_tcKeyStrokeList.size()) return;

	BYTE key = *m_tcKeyStrokeList.rbegin();

	m_tcKeyStrokeList.clear();

	if(m_tcKeyStrokeTable[key].offset < 4) m_tcKeyStrokeList.push_back(key);
}


static ThaiBreakIterator s_TBreaker;
CLanguageTH :: CLanguageTH()
{
	m_useTWordBreaker = s_TBreaker.InitializeLexicon();
}


const char* CLanguageTH :: GetWordBreak(const char* text, unsigned int size)
{
	return CLanguage::GetWordBreak(text, size);
}
