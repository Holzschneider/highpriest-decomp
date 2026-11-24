#ifndef HIGHPRIEST_FRAMEWORK_LANGUAGE_H
#define HIGHPRIEST_FRAMEWORK_LANGUAGE_H


class CLanguage;
class CLanguageTC;
class CLanguageTH;
struct HWND__;
struct tagCANDIDATELIST;

typedef enum {
	LanguageKorean = 0,
	LanguageEnglish = 1,
	LanguageJapanese = 2,
	LanguageSimplifiedChinese = 3,
	LanguageTraditionalChinese = 4,
	LanguageThai = 5,
	LanguagePortuguese = 6,
	LanguageVietnam = 7,
} LanguageType;

extern LanguageType g_languageType;

class CLanguage {
public:
	CLanguage();
	virtual ~CLanguage();
	void SetHWnd(HWND__* hWnd) { }
	void GetLanguageText(std::string* text);
	const char* GetLanguageCharset(bool bScript);
	const char* GetInput() { return m_input.c_str(); }
	void SetInput(const char* inputStr) { ResetInput(); AddInput(inputStr); }
	void ResetInput();
	int GetInputSize() { return m_input.size(); }
	virtual const char* GetKeyStroke() { return m_keyStroke.c_str(); }
	virtual int GetKeyStrokeSize() { return m_keyStroke.size(); }
	virtual void SetKeyStroke(unsigned char key) {}
	void GetSelection(int* origin, int* cursor);
	void SetSelection(int origin, int cursor);
	void GetSelectionRange(int* origin, int* cursor);
	int GetSelectionSize() { return abs(m_selectionCursor - m_selectionOrigin); }
protected:
	void CancelSelection() { m_selectionOrigin = m_selectionCursor; }
public:
	int GetSelectionCursor() { return m_selectionCursor; }
	const char* GetCompositionString() { return m_compStr.c_str(); }
	int GetCompositionStringSize() { return m_compStr.size(); }
	int GetCompositionCursorPos() { return m_compCursorPos; }
	virtual LRESULT ProcessImeMessage(HWND__* hWnd, unsigned int message, WPARAM wParam, LPARAM lParam, bool* trapped);
	void OnKeyDown(WPARAM wParam, LPARAM lParam);
	void OnChar(WPARAM wParam, LPARAM lParam);
	const char* GetCandidateString(int nCandidate);
	int GetCandidateCount() { return m_candidateCount; }
	int GetCandidateSelection() { return m_candidateSelection; }
	int GetCandidatePageSize() { return m_candidatePageSize; }
	void HideText(bool key);
	virtual const char* GetWordBreak(const char* text, unsigned int size);
	const char* LanguageFile(const char* fName);
	void MoveUpSelection(int newPos);
	void MoveDownSelection(int newPos);
	void AddInput2(const char* input) { AddInput(input); }
protected:
	virtual void GetImeCompStr();
	void GetImeResultStr();
	void GetImeCompAttr();
	void GetImeCursorPos();
	void GetImeCandidateList();
	void CancelComposition();
	void AddInput(const char* input);
	void CopyTextToClipBoard();
	void CopyTextFromClipBoard();
	void CutTextToClipBoard();
	virtual void BeginKeyStroke() {}
	virtual void EndKeyStroke() {}
	virtual void ClearKeyStroke() {}
	void IncreaseSelection();
	void DecreaseSelection();
	void DeleteSelection();
	void ClearSelection() { m_selectionCursor = m_input.size(); CancelSelection(); }
	void ClearComposition() { m_compStr = ""; }
	void ReadCandidate();
	void ClearCandidate();
protected:
	std::deque<unsigned int> m_fakeInput;
	tagCANDIDATELIST* m_candidateList;
	int m_candidateCount;
	int m_candidatePageSize;
	int m_candidateSelection;
	std::vector<unsigned char> m_candidateBuffer;
	std::string m_input;
	int m_inputCursor;
	int m_selectionCursor;
	int m_selectionOrigin;
	std::string m_keyStroke;
	std::string m_compStr;
	int m_compCursorPos;
	unsigned char m_bHideText;
	unsigned short m_langId;
	unsigned int m_codePage;
	int m_underLineStart;
	int m_underLineEnd;
public:
	std::string m_languageFolder;
	unsigned short m_scriptLangId;
};

struct TCINPUTKEY {
	BYTE offset;
	char text[3];
};

class CLanguageTC : public CLanguage {
public:
	CLanguageTC();
	virtual const char* GetKeyStroke();
	virtual int GetKeyStrokeSize();
	virtual void SetKeyStroke(unsigned char key);
protected:
	virtual void EndKeyStroke() { m_tcKeyStrokeList.clear(); }
	void FormatInputKey();
	virtual void ClearKeyStroke();
	virtual void GetImeCompStr();
protected:
	std::list<unsigned char> m_tcKeyStrokeList;
	TCINPUTKEY m_tcKeyStrokeTable[256];
	unsigned char m_tcKeyStrokeEnabled;
	unsigned char m_tcKeyStroke;
};

class CLanguageTH : public CLanguage {
public:
	CLanguageTH();
	virtual const char* GetWordBreak(const char* text, unsigned int size);
private:
	unsigned char m_useTWordBreaker;
};
int GetCodePageFromCharset(int charset);
extern bool g_multiLang;
extern CLanguage* g_language;

#endif
