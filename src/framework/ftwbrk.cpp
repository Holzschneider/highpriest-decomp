#include "common.h"





#include "ftwbrk.h"

#include "../base/File.h"
#include <stdio.h>
#include <string.h>


typedef unsigned char       byte;
typedef _TRIE               TRIE;
typedef _TRIE*              LPTRIE;
typedef _DIFFINDEX          DIFFINDEX;
typedef _DICTHEADER         DICTHEADER;






#define _lptrie(lpv)  ((LPTRIE)(lpv))

#ifdef max
#undef max
#endif
#define max(a, b)  (((a) > (b)) ? (a) : (b))

#define KARAN              236


#define ENDWORD            0x80
#define ENDCHILD           0x40


#define MAX_DICT_CTRL_CHAR 0x20
#define SUBSTRENDWORD      0x02
#define SUBSTRHAVECHILD    0x04


#define FTWB_CARETMOVEMENT  0
#define FTWB_WORDWRAP       1
#define FTWB_SEPARATESYMBOL 2
#define FTWB_SPELLER        4

#define CWL_MAX             100


#define XT_THAI            0x00000001
#define XT_ENGLISH         0x00000002
#define XT_BOTH            0x00000004
#define XT_WORDBEGIN       0x00000010
#define XT_WORDEND         0x00000020
#define XT_SENTENCEEND     0x00000040
#define XT_ZEROWIDTH       0x00000100
#define XT_SYMBOL          0x00000200
#define XT_PUNCTUATION     0x00000400
#define XT_LEADINGVOWEL    0x00001000
#define XT_FOLLOWINGVOWEL  0x00002000
#define XT_DIACRITIC       0x00004000
#define XT_TONEMARK        0x00008000
#define XT_DIGIT           0x00010000
#define XT_CONSONANCE      0x00020000
#define XT_UPPERTAIL       0x00040000
#define XT_LOWERTAIL       0x00080000
#define XT_UPPERCASE       0x00100000
#define XT_LOWERCASE       0x00200000
#define XT_WHITESPACE      0x01000000
#define XT_HARDSPACE       0x02000000
#define XT_SOFTBREAK       0x04000000
#define XT_NONBREAK        0x08000000
#define XT_CONTROL         0x10000000

#define XT_THA      XT_THAI
#define XT_ENG      XT_ENGLISH
#define XT_WRDBEG   XT_WORDBEGIN
#define XT_WRDEND   XT_WORDEND
#define XT_SNTEND   XT_SENTENCEEND
#define XT_SYM      XT_SYMBOL
#define XT_PUNCT    XT_PUNCTUATION
#define XT_LVOW     XT_LEADINGVOWEL
#define XT_FVOW     XT_FOLLOWINGVOWEL
#define XT_DIAC     XT_DIACRITIC
#define XT_TONE     XT_TONEMARK
#define XT_CONS     XT_CONSONANCE
#define XT_LTAIL    XT_LOWERTAIL
#define XT_UTAIL    XT_UPPERTAIL
#define XT_UCASE    XT_UPPERCASE
#define XT_LCASE    XT_LOWERCASE
#define XT_ZWIDTH   XT_ZEROWIDTH
#define XT_ALLSYMBOL (XT_PUNCT | XT_WRDEND | XT_WRDBEG | XT_SNTEND)

#define FNonbreakChar(ch)  ( (ch) == 221 )



#define TCT_CONSONANCE       0x0001
#define TCT_DIGIT            0x0002
#define TCT_PUNCT            0x0004
#define TCT_CAPITAL          0x0008
#define TCT_THAI             0x0010
#define TCT_ENGLISH          0x0020
#define TCT_UPPER            0x0080
#define TCT_LOWER            0x0100
#define TCT_VOWEL            0x0200
#define TCT_TONEMARK         0x0400
#define TCT_BREAK            0x1000
#define TCT_DIACRITIC        0x2000
#define TCT_LEADINGVOWEL     0x4000
#define TCT_CONTROL          0x0000

#define TCT_THAIVOWEL        (TCT_THAI | TCT_VOWEL)
#define TCT_THAIUPPERVOWEL   (TCT_THAI | TCT_VOWEL | TCT_UPPER)
#define TCT_THAILOWERVOWEL   (TCT_THAI | TCT_VOWEL | TCT_LOWER)
#define TCT_THAIDIGIT        (TCT_THAI | TCT_DIGIT)
#define TCT_THAICONS         (TCT_THAI | TCT_CONSONANCE)
#define TCT_THAIDIAC         (TCT_THAI | TCT_DIACRITIC)
#define TCT_THAICONSLOWER    (TCT_THAICONS | TCT_LOWER)
#define TCT_THAICONSUPPER    (TCT_THAICONS | TCT_UPPER)
#define TCT_THAITONE         (TCT_THAI | TCT_TONEMARK)
#define TCT_THAIBREAK        (TCT_THAI | TCT_BREAK)


unsigned int TBL_CHTYPE_WITHLEADING[96] =
{
	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,
	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,
	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,
	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONSLOWER,	TCT_THAICONSLOWER,

	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,
	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,
	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONSUPPER,
	TCT_THAICONS,	TCT_THAICONSUPPER,	TCT_THAICONS,	TCT_THAICONSUPPER,

	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,
	TCT_THAICONSLOWER,	TCT_THAICONS,	TCT_THAICONSLOWER,	TCT_THAICONS,
	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,
	TCT_THAICONS,	TCT_THAICONS,	TCT_THAICONS,	TCT_THAIBREAK,

	TCT_THAI,		TCT_THAIUPPERVOWEL,	TCT_THAI,	TCT_THAIVOWEL,
	TCT_THAIUPPERVOWEL,	TCT_THAIUPPERVOWEL,	TCT_THAIUPPERVOWEL,	TCT_THAIUPPERVOWEL,
	TCT_THAILOWERVOWEL,	TCT_THAILOWERVOWEL,	TCT_THAILOWERVOWEL,	TCT_BREAK,
	TCT_BREAK,		TCT_CONTROL	,	TCT_CONTROL,	TCT_THAI,

	TCT_THAI|TCT_LEADINGVOWEL,		TCT_THAI|TCT_LEADINGVOWEL,		TCT_THAI|TCT_LEADINGVOWEL,		TCT_THAI|TCT_LEADINGVOWEL,
	TCT_THAI|TCT_LEADINGVOWEL,		TCT_THAI,		TCT_THAI,		TCT_THAIDIAC,
	TCT_THAITONE,	TCT_THAITONE,	TCT_THAITONE,	TCT_THAITONE,
	TCT_THAITONE,	TCT_THAI,		TCT_THAI,		TCT_THAI,

	TCT_THAIDIGIT,	TCT_THAIDIGIT,	TCT_THAIDIGIT,	TCT_THAIDIGIT,
	TCT_THAIDIGIT,	TCT_THAIDIGIT,	TCT_THAIDIGIT,	TCT_THAIDIGIT,
	TCT_THAIDIGIT,	TCT_THAIDIGIT,	TCT_THAIBREAK,	TCT_THAIBREAK,
	TCT_THAILOWERVOWEL,	TCT_THAILOWERVOWEL	,TCT_THAILOWERVOWEL,	TCT_ENGLISH,
};

unsigned int TBL_CHTYPE[96] =
{
	TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,
	TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,
	TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,
	TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONSLOWER,  TCT_THAICONSLOWER,

	TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,
	TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,
	TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONSUPPER,
	TCT_THAICONS,       TCT_THAICONSUPPER,  TCT_THAICONS,       TCT_THAICONSUPPER,

	TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,
	TCT_THAICONSLOWER,  TCT_THAICONS,       TCT_THAICONSLOWER,  TCT_THAICONS,
	TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,
	TCT_THAICONS,       TCT_THAICONS,       TCT_THAICONS,       TCT_THAIBREAK,

	TCT_THAI,           TCT_THAIUPPERVOWEL, TCT_THAI,           TCT_THAI,
	TCT_THAIUPPERVOWEL, TCT_THAIUPPERVOWEL, TCT_THAIUPPERVOWEL, TCT_THAIUPPERVOWEL,
	TCT_THAILOWERVOWEL, TCT_THAILOWERVOWEL, TCT_THAILOWERVOWEL, TCT_BREAK,
	TCT_BREAK,          TCT_CONTROL,        TCT_CONTROL,        TCT_THAI,

	TCT_THAI,           TCT_THAI,           TCT_THAI,           TCT_THAI,
	TCT_THAI,           TCT_THAI,           TCT_THAI,           TCT_THAIDIAC,
	TCT_THAITONE,       TCT_THAITONE,       TCT_THAITONE,       TCT_THAITONE,
	TCT_THAITONE,       TCT_THAIDIAC,       TCT_THAI,           TCT_THAI,

	TCT_THAIDIGIT,      TCT_THAIDIGIT,      TCT_THAIDIGIT,      TCT_THAIDIGIT,
	TCT_THAIDIGIT,      TCT_THAIDIGIT,      TCT_THAIDIGIT,      TCT_THAIDIGIT,
	TCT_THAIDIGIT,      TCT_THAIDIGIT,      TCT_THAIBREAK,      TCT_THAIBREAK,
	TCT_THAILOWERVOWEL, TCT_THAILOWERVOWEL, TCT_THAILOWERVOWEL, TCT_ENGLISH,
};


unsigned int THAICHARTYPETABLE[256] =
{
	0,0,0,0,    0,0,0,0,    0,0,0,0,    0,0,0,0,
		   0,0,0,0,    0,0,0,0,    0,0,0,0,    0,0,0,0,

	XT_WHITESPACE, XT_PUNCT, XT_PUNCT, XT_PUNCT,
		   XT_PUNCT,      XT_PUNCT, XT_PUNCT, XT_PUNCT,
	XT_WRDBEG,     XT_WRDEND, XT_PUNCT, XT_PUNCT,
		   XT_PUNCT,      XT_PUNCT, XT_PUNCT | XT_BOTH, XT_PUNCT,
	XT_DIGIT | XT_ENG, XT_DIGIT | XT_ENG, XT_DIGIT | XT_ENG, XT_DIGIT | XT_ENG,
		   XT_DIGIT | XT_ENG, XT_DIGIT | XT_ENG, XT_DIGIT | XT_ENG, XT_DIGIT | XT_ENG,
	XT_DIGIT | XT_ENG, XT_DIGIT | XT_ENG, XT_PUNCT, XT_PUNCT,
		   XT_WRDBEG,     XT_PUNCT, XT_WRDEND, XT_SNTEND,
	XT_PUNCT,
		   XT_ENG | XT_UCASE, XT_ENG | XT_UCASE, XT_ENG | XT_UCASE,
		   XT_ENG | XT_UCASE, XT_ENG | XT_UCASE, XT_ENG | XT_UCASE,
		   XT_ENG | XT_UCASE,
	XT_ENG | XT_UCASE, XT_ENG | XT_UCASE, XT_ENG | XT_UCASE,
		   XT_ENG | XT_UCASE, XT_ENG | XT_UCASE, XT_ENG | XT_UCASE,
		   XT_ENG | XT_UCASE, XT_ENG | XT_UCASE,
	XT_ENG | XT_UCASE, XT_ENG | XT_UCASE, XT_ENG | XT_UCASE,
		   XT_ENG | XT_UCASE, XT_ENG | XT_UCASE, XT_ENG | XT_UCASE,
		   XT_ENG | XT_UCASE, XT_ENG | XT_UCASE,
	XT_ENG | XT_UCASE, XT_ENG | XT_UCASE, XT_ENG | XT_UCASE,
		   XT_WRDBEG, XT_PUNCT, XT_WRDEND, XT_PUNCT, XT_PUNCT,
	XT_PUNCT,
		   XT_ENG | XT_LCASE, XT_ENG | XT_LCASE, XT_ENG | XT_LCASE,
		   XT_ENG | XT_LCASE, XT_ENG | XT_LCASE, XT_ENG | XT_LCASE,
		   XT_ENG | XT_LCASE,
	XT_ENG | XT_LCASE, XT_ENG | XT_LCASE, XT_ENG | XT_LCASE,
		   XT_ENG | XT_LCASE, XT_ENG | XT_LCASE, XT_ENG | XT_LCASE,
		   XT_ENG | XT_LCASE, XT_ENG | XT_LCASE,
	XT_ENG | XT_LCASE, XT_ENG | XT_LCASE, XT_ENG | XT_LCASE,
		   XT_ENG | XT_LCASE, XT_ENG | XT_LCASE, XT_ENG | XT_LCASE,
		   XT_ENG | XT_LCASE, XT_ENG | XT_LCASE,
	XT_ENG | XT_LCASE, XT_ENG | XT_LCASE, XT_ENG | XT_LCASE,
		   XT_WRDBEG, XT_PUNCT, XT_WRDEND, XT_PUNCT, 0,
	XT_THA | XT_CONS,
		   XT_THA | XT_ZWIDTH,
		   XT_THA | XT_ZWIDTH,
		   XT_THA | XT_ZWIDTH,
		   XT_THA | XT_ZWIDTH,
		   XT_PUNCT,
		   XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_TONE,
	XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_DIAC,
		   XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_DIAC,
	XT_THA | XT_CONS,
		   XT_PUNCT | XT_BOTH | XT_WRDBEG,
		   XT_PUNCT | XT_BOTH | XT_WRDEND,
		   XT_PUNCT | XT_BOTH | XT_WRDBEG,
		   XT_PUNCT | XT_BOTH | XT_WRDEND,
		   XT_PUNCT,
		   XT_PUNCT,
		   XT_PUNCT,
	XT_THA | XT_ZWIDTH | XT_DIAC,
		   XT_THA | XT_ZWIDTH | XT_DIAC,
		   XT_THA | XT_ZWIDTH | XT_DIAC,
		   XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_DIAC,
	XT_WHITESPACE,
		   XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS,
		   XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS,
	XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS,
		   XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS,
	XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS,
		   XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS,
	XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS,
		   XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS,
	XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS,
		   XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS,
	XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS,
		   XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_CONS, XT_THA | XT_SYMBOL,
	XT_THA | XT_FVOW,
		   XT_THA | XT_ZWIDTH | XT_DIAC,
		   XT_THA | XT_FVOW,
		   XT_THA | XT_FVOW,
		   XT_THA | XT_ZWIDTH,
		   XT_THA | XT_ZWIDTH,
		   XT_THA | XT_ZWIDTH,
		   XT_THA | XT_ZWIDTH,
	XT_THA | XT_ZWIDTH,
		   XT_THA | XT_ZWIDTH,
		   XT_THA | XT_ZWIDTH | XT_DIAC,
		   XT_HARDSPACE,
		   XT_PUNCT,
		   0,
		   0,
		   XT_PUNCT,
	XT_THA | XT_LVOW,
		   XT_THA | XT_LVOW,
		   XT_THA | XT_LVOW,
		   XT_THA | XT_LVOW,
		   XT_THA | XT_LVOW,
		   XT_THA | XT_FVOW,
		   XT_THA,
		   XT_THA | XT_ZWIDTH | XT_DIAC,
	XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_TONE,
		   XT_THA | XT_ZWIDTH | XT_DIAC,
		   XT_THA | XT_ZWIDTH | XT_DIAC,
		   XT_THA | XT_ZWIDTH | XT_DIAC,
		   XT_PUNCT,
	XT_DIGIT | XT_THA, XT_DIGIT | XT_THA, XT_DIGIT | XT_THA,
		   XT_DIGIT | XT_THA, XT_DIGIT | XT_THA, XT_DIGIT | XT_THA,
		   XT_DIGIT | XT_THA,
	XT_DIGIT | XT_THA, XT_DIGIT | XT_THA,
		   XT_PUNCT,
		   XT_PUNCT,
		   XT_THA | XT_ZWIDTH,
		   XT_THA | XT_ZWIDTH,
		   XT_THA | XT_ZWIDTH,
		   0,
};





ThaiFunc::ThaiFunc()
{
}

ThaiFunc::~ThaiFunc()
{
}

bool ThaiFunc::IsThaiChar(unsigned short ch)
{
	return ((unsigned int(ch) >> 8) & 0x00ff) == 0x000e;
}

unsigned int ThaiFunc::ThaiCharType(unsigned short ch)
{
	return TBL_CHTYPE[(byte)ch];
}

void ThaiFunc::InitializeTable()
{
	memcpy(tblCharType, TBL_CHTYPE, 96 * sizeof(unsigned int));
}





ThaiLexicon::ThaiLexicon()
{
	m_trie    = NULL;
	m_cWord   = 0;
	m_cNode   =
	m_nSize   = 0;
	m_MemDict = NULL;
	memset((void*)&m_DictHeader, 0, sizeof(DICTHEADER));
}

ThaiLexicon::~ThaiLexicon()
{
	if (m_MemDict)
		delete m_MemDict;
	DestroyTrie(m_trie);
}

void ThaiLexicon::DestroyTrie(LPTRIE pTrie)
{
	if (pTrie)
	{
		DestroyTrie(_lptrie(pTrie->child));
		DestroyTrie(_lptrie(pTrie->next));
		delete pTrie;
	}
}

bool ThaiLexicon::AddWord(byte* szNew, unsigned int cb)
{
	unsigned int iNext = 0;
	LPTRIE pTrie;

	if (cb <= 0) return false;

	if (m_cWord == 0)
	{
		m_trie = new TRIE;
		m_trie->next = m_trie->child = NULL;
		m_trie->ch = szNew[iNext];
		m_trie->wrap = 0;
		m_cNode++;
	}

	pTrie = m_trie;
	while (iNext < cb)
	{
LNewLoop:
		if (pTrie->ch == szNew[iNext])
		{
LCharMatched:
			if (++iNext == cb)
				break;

			if ((!pTrie->child) || (_lptrie(pTrie->child)->ch > szNew[iNext]))
			{
				LPTRIE pTmp = new TRIE;
				m_cNode++;
				pTmp->ch = szNew[iNext];
				pTmp->next = pTrie->child;
				pTmp->child = NULL;
				pTmp->wrap = false;
				pTrie->child = pTmp;
				pTrie = _lptrie(pTrie->child);
				goto LCharMatched;
			}
			pTrie = _lptrie(pTrie->child);
			goto LNewLoop;
		}
		else
		if ((!pTrie->next) || (_lptrie(pTrie->next)->ch > szNew[iNext]))
		{
			LPTRIE pTmp = new TRIE;
			m_cNode++;
			pTmp->ch = szNew[iNext];
			pTmp->next = pTrie->next;
			pTmp->child = NULL;
			pTmp->wrap = false;
			pTrie->next = pTmp;
			pTrie = pTmp;
			goto LCharMatched;
		}
		pTrie = _lptrie(pTrie->next);
		goto LNewLoop;
	}

	if (pTrie->wrap)
		return false;
	else
	{
		pTrie->wrap = true;
		m_cWord++;
	}
	return true;
}

void ThaiLexicon::RefreshTrie()
{
	bool fMemValid = false;
	if (!m_MemDict)
	{
		if ((m_MemDict = new byte[m_cNode * 10]) != NULL)
			fMemValid = true;
	}



	if (fMemValid)
		m_nSize = Trie2Mem(m_trie, 0, false);
}

int ThaiLexicon::Trie2Mem(LPTRIE pTrie, int iStart, bool fWrap)
{
	int iNext, iCur;
	DIFFINDEX cbDiff;
	int iNode, cNode = EnumNode(pTrie);
	LPTRIE pTrieWalk = pTrie;

	iCur  = iStart;
	iNext = iStart + 1 + 3 * cNode;

	for (iNode = 0; iNode < cNode; iNode++)
	{
		cbDiff.l = iNext;
		m_MemDict[iCur++] = (cbDiff.b.b2 | (fWrap ? ENDWORD : 0));
		m_MemDict[iCur++] = cbDiff.b.b1;
		m_MemDict[iCur++] = cbDiff.b.b0;
		byte nSubWordLen = CountUniqueStr(_lptrie(pTrieWalk));
		if (nSubWordLen > 1)
			iNext = InsertSubWord(_lptrie(pTrieWalk), nSubWordLen, iNext);
		else
		{
			m_MemDict[iNext++] = pTrieWalk->ch;

			if (pTrieWalk->child)
				iNext = Trie2Mem(_lptrie(pTrieWalk->child), iNext, pTrieWalk->wrap);
			else
				m_MemDict[iNext++] = (ENDCHILD | ENDWORD);
		}

		pTrieWalk = _lptrie(pTrieWalk->next);
	}

	m_MemDict[iCur++] = (ENDCHILD | (fWrap ? ENDWORD : 0));
	return iNext;
}

int ThaiLexicon::EnumNode(LPTRIE pTrie)
{
	int cNode = 0;
	while (pTrie)
	{
		pTrie = _lptrie(pTrie->next);
		cNode++;
	}
	return cNode;
}

byte ThaiLexicon::CountUniqueStr(LPTRIE pTrie)
{
	LPTRIE pTrieWalk = pTrie;
	byte nLevel = 0;

	while (pTrieWalk && !(pTrieWalk->next))
	{
		pTrieWalk = _lptrie(pTrieWalk->child);
		nLevel++;
	}

	return nLevel;
}

int ThaiLexicon::InsertSubWord(LPTRIE pTrie, byte nLen, int iStart)
{
	LPTRIE pTrieWalk = pTrie;
	byte iLoop;
	int iNext = iStart;
	bool fEndWord = false;

	m_MemDict[iNext++] = nLen;

	for (iLoop = 0; iLoop < nLen; iLoop++)
	{
		m_MemDict[iNext++] = pTrieWalk->ch;
		if ((fEndWord = pTrieWalk->wrap) == true)
			m_MemDict[iNext++] = SUBSTRENDWORD;
		pTrieWalk = _lptrie(pTrieWalk->child);
	}
	if (pTrieWalk)
	{
		if ((unsigned int)m_MemDict[iNext - 1] < MAX_DICT_CTRL_CHAR)
			m_MemDict[iNext - 1] |= SUBSTRHAVECHILD;
		else
			m_MemDict[iNext++] = SUBSTRHAVECHILD;
		iNext = Trie2Mem(_lptrie(pTrieWalk), iNext, fEndWord);
	}
	return iNext;
}

bool ThaiLexicon::SearchMemDict(BYTE* pchWord)
{
	byte* pchDictWalk = m_MemDict;
	byte* pchWordWalk = pchWord;
	DIFFINDEX cbDiff;
	cbDiff.l = 0;
	byte iLoop, nSubStrLen;

	while (*pchWordWalk)
	{
LSrcLoopStart:
		if (*pchDictWalk & ENDCHILD)
			break;
		cbDiff.b.b2 = ((*(pchDictWalk++)) & 0x3F);
		cbDiff.b.b1 = *(pchDictWalk++);
		cbDiff.b.b0 = *(pchDictWalk++);
		if (((nSubStrLen = m_MemDict[cbDiff.l]) < MAX_DICT_CTRL_CHAR) &&
			(m_MemDict[cbDiff.l + 1] == *pchWordWalk))
		{
			cbDiff.l++;
			for (iLoop = 0; iLoop < nSubStrLen; iLoop++)
			{
				if (m_MemDict[cbDiff.l++] != *pchWordWalk)
					return false;

				pchWordWalk++;

				if (!(*pchWordWalk))
					return ((m_MemDict[cbDiff.l] < MAX_DICT_CTRL_CHAR) &&
							(m_MemDict[cbDiff.l] & SUBSTRENDWORD));
				else
				if (m_MemDict[cbDiff.l] < MAX_DICT_CTRL_CHAR)
				{
					if (m_MemDict[cbDiff.l] & SUBSTRHAVECHILD)
					{
						pchDictWalk = m_MemDict + cbDiff.l + 1;
						goto LSrcLoopStart;
					}
					else
						cbDiff.l++;
				}
			}
			return false;
		}
		else
		{
			if (m_MemDict[cbDiff.l] == *pchWordWalk)
			{
				pchWordWalk++;
				if (!(*pchWordWalk) && (m_MemDict[cbDiff.l + 1] & ENDWORD))
					return true;
				pchDictWalk = m_MemDict + cbDiff.l + 1;
			}
		}
	}
	return false;
}

bool ThaiLexicon::fSaveLexicon(const char* lpFileName)
{
	bool fSuccess = false;



	m_DictHeader.chEOF      = -1;
	m_DictHeader.version.fv = 2;
	m_DictHeader.version.dv = 2;
	m_DictHeader.cWord      = m_cWord;
	m_DictHeader.cNode      = (unsigned int)m_cNode;
	m_DictHeader.code       = ((int)m_cWord << 1) + m_cNode;
	m_DictHeader.nSize      = m_nSize;
	m_DictHeader.fValid     = true;

	FILE* fn = 0;
	fn = fopen((const char*)lpFileName, "a+b");
	if (!fn)
		return fSuccess;
	if (fwrite(&m_DictHeader, sizeof(byte), sizeof(DICTHEADER), fn) != sizeof(DICTHEADER))
	{
		fclose(fn);
		return fSuccess;
	}
	if (fwrite(m_MemDict, sizeof(byte), m_nSize, fn) != (size_t)m_nSize)
	{
		fclose(fn);
		return fSuccess;
	}
	fclose(fn);
	fSuccess = true;

	return fSuccess;
}

bool ThaiLexicon::fLoadLexicon(const char* lpFileName)
{
	bool fSuccess = false;
	CFile file;

	if (file.Open(lpFileName, CFile::modeRead))
	{

		if (!file.Read(&m_DictHeader, sizeof(DICTHEADER)))
			;
		else

			if (((int)m_DictHeader.code) != (int)(((int)m_DictHeader.cWord << 1) + (int)m_DictHeader.cNode))
				;
			else

				if (!(m_MemDict = new byte[m_DictHeader.nSize]))
					;
				else
				{

					if (!file.Read(m_MemDict, m_DictHeader.nSize))
						;
					else
					{
						m_cWord = m_DictHeader.cWord;
						m_cNode = m_DictHeader.cNode;
						m_nSize = m_DictHeader.nSize;
						fSuccess = true;
					}
				}
	}
	else
	{
		return false;
	}

	if (!fSuccess && (m_MemDict))
	{
		delete m_MemDict;
		m_MemDict = NULL;
	}
	m_DictHeader.fValid = fSuccess;
	return fSuccess;
}





ThaiBreakIterator::ThaiBreakIterator() : m_pLexicon(0)
{
}

ThaiBreakIterator::~ThaiBreakIterator()
{
	if (m_pLexicon)
		delete m_pLexicon;
}

bool ThaiBreakIterator::InitializeLexicon()
{
	if (m_pLexicon)
		delete m_pLexicon;
	m_pLexicon = new ThaiLexicon;
	m_pLexicon->fLoadLexicon("..\\ftwrk.lex");
	memcpy(m_rgThaiCharTypeTable, THAICHARTYPETABLE, 256 * sizeof(unsigned int));
	if (!m_pLexicon)
		return false;
	return true;
}

int ThaiBreakIterator::FindThaiWordBreak(const char* szText, unsigned int nStrlen, byte* rgbBrk, unsigned int nMaxBrk, unsigned int uFlags)
{
	byte* vmOurDict = m_pLexicon->pGetMemDict();
	byte* pchFst = (byte*)szText;
	byte* pchLim;
	byte rgWL[CWL_MAX];
	byte iWL, iBrk, cch;
	int iLoop;
	byte* pchDictWalk;
	byte* pchWordWalk;
	DIFFINDEX cbDiff;
	byte chDict;
	bool fCharInfoNotMatch;
	bool fCanBreakNext;
	int nSubStrLen;
	int iSubLoop;
	unsigned int dwTypeCur;
	byte chCur;
	unsigned int THAGROUP, SYMGROUP, ENGGROUP;

	int cchMaxMatch;
	bool fSeparateSymbol = (uFlags & FTWB_SEPARATESYMBOL) ? true : false;




	if (!m_pLexicon->pGetMemDict())
	{
		InitializeLexicon();
		vmOurDict = m_pLexicon->pGetMemDict();
		if (!vmOurDict)
			return 0;
	}




	if ((nMaxBrk == 0) || (!pchFst) || (!rgbBrk))
		return 0;



	if (nStrlen)
		pchLim = pchFst + nStrlen;
	else
		pchLim = (byte*)(szText + strlen(szText));



	rgbBrk[0] = iBrk = 0;
	cbDiff.l = 0;



	if (fSeparateSymbol)
	{
		ENGGROUP = XT_ENG;
		SYMGROUP = XT_PUNCT | XT_WRDBEG | XT_WRDEND | XT_SNTEND;
		THAGROUP = XT_THA;
	}
	else
	{
		ENGGROUP = XT_ENG | XT_PUNCT;
		SYMGROUP = XT_WRDBEG | XT_WRDEND | XT_SNTEND;
		THAGROUP = XT_THA;
	}



	pchWordWalk = pchFst;
	while ((*pchFst) &&
		   (*pchWordWalk) &&
		   (pchFst < pchLim) &&
		   (iBrk < nMaxBrk))
	{
		rgWL[0] = iWL = cch = cbDiff.l = cchMaxMatch = 0;
		fCharInfoNotMatch = fCanBreakNext = false;
		pchDictWalk = vmOurDict;
		pchWordWalk = pchFst;




LStartLoop:
		while ((chCur = *pchWordWalk) && (!fCharInfoNotMatch))
		{
			dwTypeCur = XCharType(chCur);




			if ((dwTypeCur & THAGROUP) ||
				((*pchWordWalk == 0x2E) &&
				 (cch != 0) &&
				 (*(pchWordWalk - 1) > 0x7F)))
			{
				goto LParseThaiChar;
			}
			else if (dwTypeCur & ENGGROUP)
			{
				if (cch) { goto LStoreWordLen; }
				if (rgbBrk[iBrk]) { rgbBrk[++iBrk] = 0; }
				do {
					++pchWordWalk;
					++cch;
				} while ((pchWordWalk < pchLim) &&
						 HaveType(*pchWordWalk, ENGGROUP));
				goto LCollectSpace;
			}
			else if (dwTypeCur & SYMGROUP)
			{
				unsigned int dwSav = dwTypeCur;
				if (cch) { goto LStoreWordLen; }
				if (rgbBrk[iBrk]) { rgbBrk[++iBrk] = 0; }
				do {
					++pchWordWalk;
					++cch;
				} while ((pchWordWalk < pchLim) &&
						 HaveType(*pchWordWalk, dwSav));
				goto LCollectSpace;
			}
			else if (dwTypeCur & XT_WRDBEG)
			{
				if (cch) { goto LStoreWordLen; }
				if (rgbBrk[iBrk]) { rgbBrk[++iBrk] = 0; }
				do {
					++pchWordWalk;
					++cch;
				} while ((pchWordWalk < pchLim) &&
						 HaveType(*pchWordWalk, XT_WRDBEG));
				goto LStartLoop;
			}
			else if (dwTypeCur & XT_WRDEND)
			{
				while ((pchWordWalk < pchLim) && (XT_WRDEND & XCharType(*pchWordWalk)))
				{
					++pchWordWalk;
					++cch;
				}
				goto LCollectSpace;
			}
			else if (chCur == 0x20)
			{
				++pchWordWalk;
				++cch;
				if ((0xE6 == *pchWordWalk) &&
					(((unsigned int)pchWordWalk > (unsigned int)(szText + 1)) &&
					 (XCharType(*(pchWordWalk - 2)) & XT_THAI)))
				{
					++pchWordWalk;
					++cch;
				}
LCollectSpace:
				while (*pchWordWalk == 0x20)
				{
					++pchWordWalk;
					++cch;
				}
				if (cch) { goto LStoreWordLen; }
			}
			else if (chCur == 0xE6)
			{
				++pchWordWalk;
				++cch;
				goto LCollectSpace;
			}
			else if (chCur < 0x20)
			{
				if (cch) { goto LStoreWordLen; }
				if (rgbBrk[iBrk]) { rgbBrk[++iBrk] = 0; }
				if ((*pchWordWalk == 0x0D) &&
					(pchWordWalk + 1 < pchLim) &&
					(*(pchWordWalk + 1) == 0x0A))
				{
					pchWordWalk += 2;
					cch += 2;
				}
				else
				{
					pchWordWalk++;
					cch++;
				}
				if (cch) { goto LStoreWordLen; }
			}
			else
			{



				pchWordWalk++;
				cch++;
				goto LStartLoop;
			}

LParseThaiChar:

			if (*pchDictWalk & ENDCHILD)
				break;

			cbDiff.b.b2 = ((*(pchDictWalk++)) & 0x3F);
			cbDiff.b.b1 = *(pchDictWalk++);
			cbDiff.b.b0 = *(pchDictWalk++);




			if (((nSubStrLen = vmOurDict[cbDiff.l]) < MAX_DICT_CTRL_CHAR) &&
				(vmOurDict[cbDiff.l + 1] == *pchWordWalk))
			{
				++cbDiff.l;
				for (iSubLoop = 0; iSubLoop < nSubStrLen; iSubLoop++)
				{
					if (vmOurDict[cbDiff.l++] != *pchWordWalk)
					{
						fCharInfoNotMatch = true;
						break;
					}
					else
					{
						cch++;
						pchWordWalk++;
						if (vmOurDict[cbDiff.l] < MAX_DICT_CTRL_CHAR)
						{
							if ((vmOurDict[cbDiff.l] & SUBSTRENDWORD) &&
								(!HaveType(*pchWordWalk, XT_ZWIDTH)))
							{
								rgWL[iWL++] = cch;
								rgWL[iWL] = 0;
							}
							if (vmOurDict[cbDiff.l] & SUBSTRHAVECHILD)
							{
								pchDictWalk = vmOurDict + cbDiff.l + 1;
								break;
							}
							cbDiff.l++;
						}
					}
				}
			}
			else



			if ((chDict = vmOurDict[cbDiff.l]) < *pchWordWalk)
			{

			}
			else if (chDict == *pchWordWalk)
			{
				++cch;
				++pchWordWalk;



				if ((vmOurDict[cbDiff.l + 1] & ENDWORD) &&
					(!HaveType(*pchWordWalk, XT_ZWIDTH)))
				{
					rgWL[iWL++] = cch;
					rgWL[iWL] = 0;
				}



				pchDictWalk = vmOurDict + cbDiff.l + 1;
			}
			else
				break;
		}




		if (iWL)
		{
			int w, i = iWL;
			int wMaxPri, wMaxSnd, wMaxSum, iMax;

			wMaxPri = wMaxSnd = wMaxSum = iMax = 0;

			if (iWL == 1 &&
				0 == GetWeight(vmOurDict, pchFst, pchFst + rgWL[0], pchLim, 0))
			{
				wMaxSum = rgWL[0];

				for (i = 1; i < rgWL[iWL - 1]; i++)
				{
					w = GetWeight(vmOurDict, pchFst, pchFst + i, pchLim, 0);
					if (w + i > wMaxSum)
					{
						rgWL[0] = i;
						iMax = 0;
						break;
					}
				}
			}
			else
			for (i = iWL - 1; i >= 0; i--)
			{
				w = GetWeight(vmOurDict, pchFst, pchFst + rgWL[i], pchLim, 0);



				if ((w + rgWL[i] > wMaxSum) ||
					((w + rgWL[i] == wMaxSum) && (max(w, rgWL[i]) <= wMaxPri)))
				{



					wMaxSum = max(w, 1) + rgWL[i];
					wMaxPri = max(w, rgWL[i]);
					wMaxSnd = wMaxSum - wMaxPri;
					iMax = i;
				}
			}

			iLoop = iMax;

			cch = rgWL[iLoop];
		}
		else
		{
			cch = 1;
		}



		if (cch == 1)
		{
			if (HaveType(*pchFst, XT_THA) &&
				((pchFst + 1) < pchLim) &&
				(*(pchFst + 1) != KARAN))
			{
				pchWordWalk = pchFst + 1;
				while ((pchWordWalk < pchLim) && HaveType(*pchWordWalk, XT_ZWIDTH | XT_FVOW))
				{
					++pchWordWalk;
					++cch;
				}
			}
			else if ((*pchFst == 0x0D) &&
					 ((pchFst + 1) < pchLim) &&
					 (*(pchFst + 1) == 0x0A))
			{
				cch = 2;
			}
		}
LStoreWordLen:
		if (((unsigned int)pchFst > (unsigned int)szText) &&
			fCanCat(*(pchFst - 1), *pchFst, uFlags))
		{
			if (iBrk && rgbBrk[iBrk] == 0)
				rgbBrk[iBrk - 1] += cch;
			else
				rgbBrk[iBrk] += cch;
		}
		else
		if (((unsigned int)pchFst > (unsigned int)szText) &&
			(XCharType(*(pchFst - 1)) & XCharType(*pchFst)) &&
			(
			  (
				(cch == 1) &&
				(!(HaveType(*pchFst, XT_WRDBEG))) &&
				(!(HaveType(*pchFst, XT_LVOW) && ((pchFst + 1) < pchLim) && HaveType(*(pchFst + 1), XT_THA)))
			  )
			  ||
			  (
				(rgbBrk[iBrk] == 1) ||
				(iBrk && (rgbBrk[iBrk] == 0) && (rgbBrk[iBrk - 1] == 1))
			  )
			)
		   )
		{
			if (rgbBrk[iBrk])
				rgbBrk[iBrk] += cch;
			else
				rgbBrk[iBrk - 1] += cch;
		}
		else
		{
			if (rgbBrk[iBrk])
				++iBrk;
			rgbBrk[iBrk] = cch;
		}

		pchFst += cch;
		pchWordWalk = pchFst;

		if (iBrk == nMaxBrk)
		{
			rgbBrk[iBrk] = 0;
			break;
		}
	}

	if (rgbBrk[iBrk])
		iBrk++;
	rgbBrk[iBrk] = 0;

	return iBrk;
}

int ThaiBreakIterator::GetWeight(byte* vmOurDict, byte* szText, byte* pchFst, byte* pchLim, unsigned int dwFlags)
{
	byte* pchWordWalk;
	byte* pchDictWalk;
	bool fCharInfoNotMatch = false;
	DIFFINDEX cbDiff;
	byte chDict;
	int weight = 0;
	int nSubStrLen;
	int cch = 0;



	if (!(*pchFst)) return 1000;



	pchWordWalk = pchFst;
	pchDictWalk = vmOurDict;
	cbDiff.l = 0;




	while ((*pchWordWalk) &&
		   (pchWordWalk < pchLim) &&
		   (!fCharInfoNotMatch))
	{



		if (*pchDictWalk & ENDCHILD)
		{
			weight = cch;
			break;
		}




		cbDiff.b.b2 = ((*(pchDictWalk++)) & 0x3F);
		cbDiff.b.b1 = *(pchDictWalk++);
		cbDiff.b.b0 = *(pchDictWalk++);




		if (((nSubStrLen = vmOurDict[cbDiff.l]) < MAX_DICT_CTRL_CHAR) &&
			(vmOurDict[cbDiff.l + 1] == *pchWordWalk))
		{
			int iSubLoop;

			++cbDiff.l;

			for (iSubLoop = 0; iSubLoop < nSubStrLen; iSubLoop++)
			{
				if (vmOurDict[cbDiff.l++] != *pchWordWalk)
				{
					fCharInfoNotMatch = true;
					break;
				}
				else
				{
					cch++;
					pchWordWalk++;
					if (vmOurDict[cbDiff.l] < MAX_DICT_CTRL_CHAR)
					{
						if (vmOurDict[cbDiff.l] & SUBSTRENDWORD)
							weight = cch;

						if (vmOurDict[cbDiff.l] & SUBSTRHAVECHILD)
						{
							pchDictWalk = vmOurDict + cbDiff.l + 1;
							break;
						}
						cbDiff.l++;
					}
				}
			}
		}
		else



		if ((chDict = vmOurDict[cbDiff.l]) < *pchWordWalk)
		{
		}
		else if (chDict == *pchWordWalk)
		{
			++cch;
			++pchWordWalk;



			if (vmOurDict[cbDiff.l + 1] & ENDWORD)
				weight = cch;



			pchDictWalk = vmOurDict + cbDiff.l + 1;
		}
		else
		{
			fCharInfoNotMatch = true;
			break;
		}
	}

	return (weight);
}

bool ThaiBreakIterator::fCanCat(byte chPrev, byte ch, unsigned int uFlags)
{








	if (FNonbreakChar(chPrev))
		return true;




	if (HaveType(chPrev, XT_THA) &&
		HaveType(ch, XT_FVOW | XT_ZWIDTH))
		return true;

	if (ch == ' ')
		return true;
	else if (chPrev == ' ')
		return false;







	if (uFlags & FTWB_SEPARATESYMBOL)
	{

		if ((HaveType(chPrev, XT_ALLSYMBOL) ? 1 : 0) != (HaveType(ch, XT_ALLSYMBOL) ? 1 : 0))
			return false;

		return (
				(HaveType(chPrev, XT_LVOW)      && HaveType(ch, XT_THA)) ||
				(HaveType(chPrev, XT_ALLSYMBOL) && HaveType(ch, XT_ALLSYMBOL)) ||
				(HaveType(chPrev, XT_ENG)       && HaveType(ch, XT_ENG))
			   );
	}




	else
	{
		if (HaveType(ch, XT_WRDBEG) && !HaveType(chPrev, XT_WRDBEG))
			return false;

		return (
				HaveType(ch, XT_WRDEND | XT_SNTEND) ||
				HaveType(ch, XT_SNTEND) ||
				HaveType(chPrev, XT_WRDBEG) ||
				(HaveType(chPrev, XT_LVOW)  && HaveType(ch, XT_THA)) ||
				(HaveType(chPrev, XT_PUNCT) && HaveType(ch, XT_ENG | XT_DIGIT)) ||
				HaveType(ch, XT_PUNCT)
			   );
	}



	return false;
}

bool ThaiBreakIterator::HaveType(char ch, unsigned int IsType)
{
	return m_rgThaiCharTypeTable[(byte)ch] & IsType ? true : false;
}

unsigned int ThaiBreakIterator::XCharType(char ch)
{
	return m_rgThaiCharTypeTable[(byte)ch];
}
