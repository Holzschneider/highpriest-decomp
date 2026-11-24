#include "common.h"
#include "Util.h"
#include "File.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <direct.h>
#include <windows.h>
#include <istream>

enum	lexMode {
	lm_quote, lm_read, lm_skip,
};


using std::string;
using std::list;
using std::vector;
using std::multimap;
using std::pair;




void	MakeStringLower(string &s)
{

	for (int j=0; j<s.size(); j++) {
		if (s[j]>0) {
			if (isupper(s[j]))
				s[j] += ('a'-'A');
		} else
			j++;
	}
}

void	CTokenizer :: MakeUpper()
{

	for (int i=0; i<m_token.size(); i++) {
		string &	s = m_token[i];

		for (int j=0; j<s.size(); j++) {
			if (s[j]>0) {
				if (islower(s[j]))
					s[j] -= ('a'-'A');
			} else
				j++;
		}
	}
}

void	CTokenizer :: MakeLower()
{

	for (int i=0; i<m_token.size(); i++) {
		string &	s = m_token[i];

		for (int j=0; j<s.size(); j++) {
			if (s[j]>0) {
				if (isupper(s[j]))
					s[j] += ('a'-'A');
			} else
				j++;
		}
	}
}

void    CTokenizer :: SetString(const string &a)
{
	int     len = a.length();
	int     curIndex = 0;
	int     curOffset = 0;
	lexMode	curMode = lm_read;
	char    buffer[1024];

	m_token.erase(m_token.begin(), m_token.end());
	int i ;
	for (i=0; i<len; i++) {
		switch (curMode) {
			case	lm_quote:
				if (a[i] == '"') {
					buffer[curOffset] = 0;
					m_token.push_back(string(buffer));
					curMode = lm_skip;
					curOffset = 0;
				} else {
					buffer[curOffset++] = a[i];
				}
				break;
			case	lm_skip:
				if (!(isspace(a[i])&&a[i]>=0)) {
					curMode = lm_read;
					i--;
				}
				break;
			case	lm_read:
				if (a[i] == '/')
					if (a[i+1] == '/')
						return;
				if (isspace(a[i])&&a[i]>=0) {
					if (i!=0) {
						buffer[curOffset] = 0;
						m_token.push_back(string(buffer));
						curOffset = 0;
					}
					curMode = lm_skip;
					break;
				}
				if (a[i] == '"') {
					curOffset = 0;
					curMode = lm_quote;
					break;
				}
				buffer[curOffset++] = a[i];
				break;
		}
	}
	if (curMode != lm_skip && i) {
		buffer[curOffset] = 0;
		m_token.push_back(string(buffer));
	}
}

void	CTokenizer :: GetLine(std::istrstream &stream)
{
	char	buf[1024];
	stream.getline(buf, 1024);
	SetString(string(buf));
}


char * GetNumberText(int number, char *buf)
{
	sprintf(buf, "%d", number);
	int size = strlen(buf);
	if (size>3) {
		string numTxt = "";
		int i ;
		for (i=0; i<(size%3); ++i) {
			numTxt += buf[i];
		}
		for (;i<size;++i) {
			if ((size-i)%3==0 && i!=0) {
				numTxt += ',';
			}
			numTxt += buf[i];
		}
		sprintf(buf, "%s", numTxt.c_str());
	}

	return buf;
}

const char * StrChrBackward(const char * strName, char c)
{
	const char* strNameEnd = strName + strlen(strName) - 1;

	while(strNameEnd >= strName) {
		if(*strNameEnd == c) return strNameEnd;
		--strNameEnd;
	}

	return NULL;
}

int GetNumberInText(char *buf)
{
	int size = strlen(buf);

	int num = 0 ;

	if (size>3)
	{
		string numTxt = "";
		string tmp = "";

		for ( int i = 0 ; i < size ; ++i )
		{
			tmp = buf[i] ;

			if ( strcmp ( tmp.c_str(), "," ) && strcmp ( tmp.c_str(), "-" ) )
				numTxt += buf[i];
		}

		int size2 = strlen ( numTxt.c_str ( ) ) ;

		string numTxt2 = numTxt ;

		if ( size2 > 9 )
		{
			char * buf = new char[size2] ;

			strcpy ( buf, numTxt2.c_str ( ) ) ;

			numTxt2 = "" ;

			for ( int i = 0 ; i < 9 ; ++i )
				numTxt2 += buf[i] ;
		}

		num = atoi ( numTxt2.c_str()) ;
	}
	else
		num = atoi (buf) ;

	return num ;
}

void	GetFileNames(list<string> &fileNameList, const char * path, const char * szFileName_wildcard)
{
	string pathWildcard = path;
	pathWildcard += szFileName_wildcard;

	WIN32_FIND_DATA wfd;
	memset(&wfd, 0, sizeof(WIN32_FIND_DATA));
	HANDLE hFile = FindFirstFile(pathWildcard.c_str(), &wfd);

	if (hFile == INVALID_HANDLE_VALUE) {
		Trace("INVALID_HANDLE_VALUE");
		FindClose(hFile);
	} else {
		string cur0 = ".";
		string cur1 = "..";
		if (cur0 != wfd.cFileName && cur1 != wfd.cFileName)
			fileNameList.push_back(wfd.cFileName);
		while (FindNextFile(hFile, &wfd)) {
			if (cur0 != wfd.cFileName && cur1 != wfd.cFileName)
				fileNameList.push_back(wfd.cFileName);
		}
		FindClose(hFile);
	}
}

DWORD	aToDW_HEX(const char *hexNumber)
{

	DWORD rValue = 0;
	int size = strlen(hexNumber);
	for (int i=size-1; i>=2; --i) {
		switch (hexNumber[i]) {
			case '0':			case '1':			case '2':			case '3':
			case '4':			case '5':			case '6':			case '7':
			case '8':			case '9':
				rValue += (hexNumber[i] - '0') << (((size-1) - i)*4);
				break;
			case 'A':			case 'B':			case 'C':			case 'D':
			case 'E':			case 'F':
				rValue += (hexNumber[i] - 'A' + 0xA) << (((size-1) - i)*4);
				break;
			case 'a':			case 'b':			case 'c':			case 'd':
			case 'e':			case 'f':
				rValue += (hexNumber[i] - 'a' + 0xA) << (((size-1) - i)*4);
				break;
		}
	}
	return rValue;
}

void	DeleteDirectory(const char * dirName)
{
	{
		list<string> fileNameList;
		list<string> dirNameList;
		WIN32_FIND_DATA wfd;
		memset(&wfd, 0, sizeof(WIN32_FIND_DATA));
		char szPath[2048];
		strcpy(szPath, dirName);
		strcat(szPath, "\\*.*");
		HANDLE hFile = FindFirstFile(szPath, &wfd);
		if (hFile == INVALID_HANDLE_VALUE) {
			FindClose(hFile);
		} else {
			string cur0 = ".";
			string cur1 = "..";
			if (cur0 != wfd.cFileName && cur1 != wfd.cFileName) {
				if (wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
					dirNameList.push_back(wfd.cFileName);
				else
					fileNameList.push_back(wfd.cFileName);
			}
			while (FindNextFile(hFile, &wfd)) {
				if (cur0 != wfd.cFileName && cur1 != wfd.cFileName) {
					if (wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
						dirNameList.push_back(wfd.cFileName);
					else
						fileNameList.push_back(wfd.cFileName);
				}
			}
			FindClose(hFile);

			strcpy(szPath, dirName);
			strcat(szPath, "\\");
			std::list<std::string>::iterator it = fileNameList.begin();
			while (it != fileNameList.end()) {
				_chmod(string(szPath + (*it)).c_str(), 0000200);
				_unlink(string(szPath + (*it)).c_str());
				++it;
			}
			it = dirNameList.begin();
			while (it != dirNameList.end()) {
				DeleteDirectory(string(szPath + (*it)).c_str());
				++it;
			}
		}
		_rmdir(dirName);
	}
}



static int RANGE_MIN = 30 ;
static int RANGE_MAX = 300 ;

CScheduler :: CScheduler ( )
{
	m_scheduleList.clear ( ) ;
}


CScheduler :: ~CScheduler ( )
{
}




bool	CScheduler :: InsertInList ( int excuteTime, int excuteType, void (*pfunc)() )
{
	POINTER_FUNC g_pfunc ;
	g_pfunc.excuteType = excuteType ;
	g_pfunc.pfunc = pfunc ;
	time(&g_pfunc.startTime) ;

	m_scheduleList.insert ( pair <time_t, POINTER_FUNC> (excuteTime, g_pfunc) ) ;

	return true ;
}

bool	CScheduler :: InsertInList ( int excuteTime, POINTER_FUNC pfunc )
{
	return InsertInList ( excuteTime, pfunc.excuteType, pfunc.pfunc ) ;
}

void	CScheduler :: OnRun ( )
{
	std::multimap<int, POINTER_FUNC>::iterator iter = m_scheduleList.begin ( ) ;

	for ( ; iter != m_scheduleList.end() ; ++iter )
	{
		time_t ltime;

		if ( ( iter->first + iter->second.startTime ) < time(&ltime) )
		{
			(iter->second.pfunc)() ;

			if ( (iter->second).excuteType != POINTER_FUNC::EXCUTE_ONCE )
			{
				DWORD excuteTime = iter->first ;


				if ( (iter->second).excuteType == POINTER_FUNC::EXCUTE_RANDOM )
					excuteTime = (DWORD)(((double) rand() / (double) RAND_MAX) * RANGE_MAX + RANGE_MIN) ;

				InsertInList ( excuteTime, iter->second ) ;
			}

			m_scheduleList.erase ( iter ) ;

			break ;
		}
		else
			break ;
	}
}
