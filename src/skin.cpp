#include "common.h"
#include "skin.h"

#include <stdio.h>
#include <string.h>
#include "bf_types.h"

#include "base/Res.h"
#include "resource/Bitmap.h"
#include "base/Util.h"


#include "3dlib/DrawUtil.h"

CSkinMgr g_skinMgr;


extern const char * UIBmp(const char * fileName);

using std::string;



extern const char * StrChrBackward(const char * strName, char c);






static string s_basicSkinName = "<Basic Skin>";
char * szPath2 = "..\\..\\skin\\";
char * szPath3 = "..\\..\\_tmpskin_\\";






CSkinMgr :: CSkinMgr()
{
	m_savedSkinName = s_basicSkinName;
}

CSkinMgr :: ~CSkinMgr()
{
	DeleteTmpSkinFolder();
}

void	CSkinMgr :: Init()
{
	m_skinFolderNameList.clear();
	m_skinNameList.clear();
	m_isZipSkinList.clear();
	m_curSkinNum = -1;
	bf_file_info wfd;
	memset(&wfd, 0, sizeof(bf_file_info));
	char * szPath = ".\\skin\\*.*";
	bf_file_find* hFile = bf_file_find_first(szPath, &wfd);
	if (hFile != NULL) {
		string cur0 = ".";
		string cur1 = "..";

		int skinIndex = 0;
		if (cur0 != wfd.name && cur1 != wfd.name && wfd.is_folder) {
			string skinFolderName = szPath2;
			skinFolderName += wfd.name;
			skinFolderName += "\\";
			m_skinFolderNameList.push_back(skinFolderName.c_str());
			m_skinNameList.push_back(wfd.name);
			m_isZipSkinList.push_back(FALSE);
			m_isZipSkinExtractedList.push_back(FALSE);
			if (m_savedSkinName == wfd.name) {
				m_curSkinNum = skinIndex;
			}
			++skinIndex;
		} else if (cur0 != wfd.name && cur1 != wfd.name) {
			string ext = StrChrBackward(wfd.name, '.');
			MakeStringLower(ext);
			if (ext == ".zip") {
				string skinFolderName = szPath3;
				skinFolderName += wfd.name;
				skinFolderName.resize(skinFolderName.size()-4);
				skinFolderName += "\\";
				m_skinFolderNameList.push_back(skinFolderName.c_str());
				string skinName = wfd.name;
				skinName.resize(skinName.size()-4);
				m_skinNameList.push_back(skinName.c_str());
				m_isZipSkinList.push_back(TRUE);
				m_isZipSkinExtractedList.push_back(FALSE);
				if (m_savedSkinName == skinName) {
					m_curSkinNum = skinIndex;
				}
				++skinIndex;
			}
		}

		while (bf_file_find_next(hFile, &wfd)) {
			if (cur0 != wfd.name && cur1 != wfd.name && wfd.is_folder) {
				string skinFolderName = szPath2;
				skinFolderName += wfd.name;
				skinFolderName += "\\";
				m_skinFolderNameList.push_back(skinFolderName.c_str());
				m_skinNameList.push_back(wfd.name);
				m_isZipSkinList.push_back(FALSE);
				m_isZipSkinExtractedList.push_back(FALSE);
				if (m_savedSkinName == wfd.name) {
					m_curSkinNum = skinIndex;
				}
				++skinIndex;
			} else if (cur0 != wfd.name && cur1 != wfd.name) {
				string ext = StrChrBackward(wfd.name, '.');
				MakeStringLower(ext);
				if (ext == ".zip") {
					string skinFolderName = szPath3;
					skinFolderName += wfd.name;
					skinFolderName.resize(skinFolderName.size()-4);
					skinFolderName += "\\";
					m_skinFolderNameList.push_back(skinFolderName.c_str());
					string skinName = wfd.name;
					skinName.resize(skinName.size()-4);
					m_skinNameList.push_back(skinName.c_str());
					m_isZipSkinList.push_back(TRUE);
					m_isZipSkinExtractedList.push_back(FALSE);
					if (m_savedSkinName == skinName) {
						m_curSkinNum = skinIndex;
					}
					++skinIndex;
				}
			}
		}
		bf_file_find_close(hFile);
	}
}

void	CSkinMgr :: SetCurrentSkinNum(int skinNum)
{
	int oldCurSkinNum = m_curSkinNum;
	m_curSkinNum = skinNum;
	if (oldCurSkinNum != m_curSkinNum) {
		g_resMgr().UnloadResByExt("bmp");
	}
}

void	CSkinMgr :: GetColorChipColor(int x, int y, DWORD &r, DWORD &g, DWORD &b)
{
	CBitmapRes * colorBitmap = (CBitmapRes *)g_resMgr().Get(UIBmp("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\colorchip.bmp"), false);
	DWORD color = colorBitmap->GetColor(x, y);
	r = (color & 0x00ff0000) >> 16;
	g = (color & 0x0000ff00) >> 8;
	b = (color & 0x000000ff);
}

const char * CSkinMgr :: GetSkinName(int skinNum)
{
	if (skinNum==-1)
		return s_basicSkinName.c_str();
	sAssert(skinNum>=0 && skinNum<m_skinFolderNameList.size());
	return m_skinNameList[skinNum].c_str();
}

const char * CSkinMgr :: GetSkinFolderName(int skinNum)
{
	if (skinNum==-1)
		return s_basicSkinName.c_str();
	sAssert(skinNum>=0 && skinNum<m_skinFolderNameList.size());
	return m_skinFolderNameList[skinNum].c_str();
}

const char * CSkinMgr :: GetSkinCurrentFolderName()
{
	if (m_curSkinNum==-1)
		return s_basicSkinName.c_str();
	sAssert(m_curSkinNum>=0 && m_curSkinNum<m_skinFolderNameList.size());
	return m_skinFolderNameList[m_curSkinNum].c_str();
}

const char * CSkinMgr :: GetCurrentSkinName()
{
	if (m_curSkinNum==-1)
		return s_basicSkinName.c_str();
	sAssert(m_curSkinNum>=0 && m_curSkinNum<m_skinFolderNameList.size());
	return m_skinNameList[m_curSkinNum].c_str();
}

void CSkinMgr :: ExtractFilesIfZipSkin()
{

	if (m_curSkinNum==-1)
		return;

	if (m_isZipSkinList[m_curSkinNum] && m_isZipSkinExtractedList[m_curSkinNum]==FALSE) {
		char	tmp[2048];
		sprintf(tmp, "%s\\_tmpskin_", g_baseDir3);
		strcat(tmp, "\\");
		strcat(tmp, m_skinNameList[m_curSkinNum].c_str());
		if (bf_file_stat(tmp, NULL)) {
			return;
		}

		sprintf(tmp, "%s\\_tmpskin_", g_baseDir3);
		if (!bf_file_stat(tmp, NULL)) {
			if (bf_file_make_folder(tmp) != 1) {
				return;
			}
		}
		sprintf(tmp, "%s\\_tmpskin_", g_baseDir3);
		strcat(tmp, "\\");
		strcat(tmp, m_skinNameList[m_curSkinNum].c_str());
		if (!bf_file_stat(tmp, NULL)) {
			if (bf_file_make_folder(tmp) != 1) {
				return;
			}
		}

		char arg[4][128];
		strcpy(arg[0], "-n");
		strcpy(arg[1], "skin/");
		strcat(arg[1], m_skinNameList[m_curSkinNum].c_str());
		strcpy(arg[2], "-d");
		strcpy(arg[3], "_tmpskin_/");
		strcat(arg[3], m_skinNameList[m_curSkinNum].c_str());

#if defined(BF_PORT)
		// runzip.exe unpacked the zipped skin into _tmpskin_; the port starts no Windows program,
		// so the skin's files stay missing and UIBmp falls back to the basic images.
		bf_port_missing(BFT_FILE, "skin.zip_not_unpacked", "%s.zip (runzip.exe)", arg[1]);
#else
		spawnl(P_WAIT, "runzip.exe", "runzip.exe", arg[0], arg[1], arg[2], arg[3], NULL);
#endif

		m_isZipSkinExtractedList[m_curSkinNum] = TRUE;
	}
}

void CSkinMgr :: DeleteTmpSkinFolder()
{
	char	tmp[2048];
	sprintf(tmp, "%s\\_tmpskin_", g_baseDir3);
	if (bf_file_stat(tmp, NULL)) {
		DeleteDirectory(tmp);
	}
}
