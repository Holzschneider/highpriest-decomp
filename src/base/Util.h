#ifndef HIGHPRIEST_BASE_UTIL_H
#define HIGHPRIEST_BASE_UTIL_H

#include "../3dlib/DrawUtil.h"


struct POINTER_FUNC {
	enum enumConfig { EXCUTE_ONCE = 0, EXCUTE_REPEAT, EXCUTE_RANDOM };
	int excuteType;
	void (*pfunc)();
	int startTime;
};

class CScheduler {
public:
	CScheduler();
	virtual ~CScheduler();
	void OnRun();
	bool InsertInList(int excuteTime, int excuteType, void (*pfunc)());
	bool InsertInList(int excuteTime, POINTER_FUNC excuteType);
private:
	std::multimap<int, POINTER_FUNC, std::less<int> > m_scheduleList;
};

class CTokenizer {
public:
	CTokenizer(const std::string& a);
	CTokenizer();
	void operator<<(const std::string& a);
	std::string& operator[](int a);
	void SetString(const std::string& a);
	void MakeUpper();
	void MakeLower();
	int NumTokens() { return m_token.size(); }
	int GetInt(int a) { return atoi(m_token[a].c_str()); }
	int GetFloat(int a) { return atof(m_token[a].c_str()); }
	void GetLine(std::istrstream& stream);
private:
	std::vector<std::string> m_token;
};

inline CTokenizer::CTokenizer()
	: m_token(std::allocator<std::string>())
{
}

inline CTokenizer::CTokenizer(const std::string& a)
	: m_token(std::allocator<std::string>())
{
	SetString(a);
}

inline void CTokenizer::operator<<(const std::string& a)
{
	SetString(a);
}


inline std::string& CTokenizer::operator[](int a)
{
	int numToken = m_token.size();
	if (a < 0 || a >= numToken) {
		ErrorMsg("Error in CTokenizer :: operator []");
		for (int i = 0; i < numToken; i++)
			ErrorMsg(m_token[i].c_str());
	}
	return m_token[a];
}

inline int random(int range)
{
	if(range==0)
		return 0;
	return rand()%range;
}

inline float DirToDeg(int dir)
{
	return dir * 45+180;
}


inline void FixInvalidNameForFile(std::string &name)
{
	int len = name.size();
	for (int cnt = 0; cnt < len; ++cnt) {
		char c = name[cnt];
		if ((c=='\\')||(c=='/')||(c==':')||(c=='*')||(c=='?')||(c=='"')||(c=='<')||(c=='>')||(c=='|')) {
			name[cnt] = '_';
		}
	}
}

inline float Dir24ToDeg(int dir)
{
	return dir * 15+180;
}

inline int DegToDir24(float roty)
{
	return ((int(roty) + 180) / 15)%24;
}

inline int DegToDir(float roty)
{
	return ((int(roty) + 180) / 45)%8;
}
void MakeStringLower(std::string &s);
char* GetNumberText(int number, char* buf);
int GetNumberInText(char* buf);
DWORD aToDW_HEX(const char *hexNumber);
void GetFileNames(std::list<std::string>& fileNameList, const char* path, const char* pattern);
void DeleteDirectory(const char * dirName);

#endif
