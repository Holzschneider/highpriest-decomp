#ifndef HIGHPRIEST_FRAMEWORK_FTWBRK_H
#define HIGHPRIEST_FRAMEWORK_FTWBRK_H

#include <string.h>

struct _TRIE;
class ThaiBreakIterator;
class ThaiFunc;
class ThaiLexicon;

struct _TRIE {
	unsigned char ch;
	bool wrap;
	void* next;
	void* child;
};

struct _DIFFBYTE {
	unsigned char b0;
	unsigned char b1;
	unsigned char b2;
	unsigned char b3;
};

union _DIFFINDEX {
	union {
		long l;
		_DIFFBYTE b;
	};
};

struct _DICTVERSION {
	unsigned char fv;
	unsigned char dv;
};

struct _DICTHEADER {
	char szDesc[256];
	char chEOF;
	_DICTVERSION version;
	unsigned int cWord;
	unsigned int cNode;
	long code;
	long nSize;
	unsigned char fValid;
};

enum {
	LT_THACH,
	LT_ENGCH,
	LT_LIBREAK,
	LT_THAPUNCT,
	LT_ENGPUNCT,
	LT_MAX,
};

class ThaiBreakIterator {
public:
	ThaiBreakIterator();
	~ThaiBreakIterator();
	bool InitializeLexicon();
	int FindThaiWordBreak(const char* szText, unsigned int nStrlen, unsigned char* fCharInfoNotMatch, unsigned int rgbBrk, unsigned int nMaxBrk);
protected:
	bool HaveType(char ch, unsigned long IsType);
	unsigned long XCharType(char ch);
	int GetWeight(unsigned char* vmOurDict, unsigned char* szText, unsigned char* pchFst, unsigned char* pchLim, unsigned long dwFlags);
	bool fCanCat(unsigned char chPrev, unsigned char ch, unsigned int uFlags);
protected:
	ThaiLexicon* m_pLexicon;
	unsigned long m_rgThaiCharTypeTable[256];
};

class ThaiLexicon {
public:
	ThaiLexicon();
	~ThaiLexicon();
	bool AddWord(unsigned char* szNew, unsigned int cb);
	void RefreshTrie();
	bool fSaveLexicon(const char* lpFileName);
	bool fLoadLexicon(const char* lpFileName);
	unsigned int uiGetCountWord() { return m_cWord; }
	long lGetCountNode() { return m_cNode; }
	long lGetCountSize() { return m_nSize; }
	_TRIE* lpGetTriePacked() { return m_trie; }
	unsigned char* pGetMemDict() { return m_MemDict; }
	bool SearchMemDict(unsigned char* pchWord);
	unsigned int GetTotalWord() { return m_cWord; }
	unsigned int GetTotalNode() { return m_cNode; }
protected:
	int Trie2Mem(_TRIE* pTrie, int iStart, bool fWrap);
	int EnumNode(_TRIE* pTrie);
	unsigned char CountUniqueStr(_TRIE* pTrie);
	int InsertSubWord(_TRIE* pTrie, unsigned char nLen, int iStart);
	void DestroyTrie(_TRIE* pTrie);
protected:
	_TRIE* m_trie;
	unsigned int m_cWord;
	long m_cNode;
	long m_nSize;
	unsigned char* m_MemDict;
	_DICTHEADER m_DictHeader;
};

class ThaiFunc {
public:
	ThaiFunc();
	~ThaiFunc();
	void InitializeTable();
	bool IsThaiChar(unsigned short ch);
	unsigned int ThaiCharType(unsigned short ch);
protected:
	unsigned int tblCharType[96];
};

#endif
