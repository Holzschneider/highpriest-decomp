#include "common.h"

#include <string.h>
#include "Res.h"



#include "Util.h"
#include "../3dlib/DrawUtil.h"
#include "File.h"
#include "../window/UISys.h"
#include "bf_types.h"
#include <stdio.h>




#ifndef sprintf_s
inline int sprintf_s(char* buf, size_t, const char* fmt, const char* a) { return sprintf(buf, fmt, a); }
#endif


extern DWORD         EXPIRETIME;
extern unsigned char lower_table[];

DWORD EXPIRETIME = 15000;
unsigned char lower_table[256] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	64, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 91, 92, 93, 94, 95,
	96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
};
extern const char*   StrChrBackward(const char* str, char c);


using std::map;
using std::string;
using std::vector;


CRes :: CRes()
:	m_lockCnt(0)
,	m_fName("")
{
}

int		CRes :: Lock()
{
	return ++m_lockCnt;
}

int		CRes :: Unlock()
{
	sAssert(m_lockCnt > 0);

	UpdateTimeStamp();

	return --m_lockCnt;
}

const char*	CRes :: GetName()
{
	return m_fName.c_str();
}

void	CRes :: UpdateInfo(const char* fName, int extIndex)
{
	m_fName.SetString(fName);
	m_extIndex = extIndex;
	UpdateTimeStamp();
}

void	CRes :: UpdateTimeStamp()
{
	m_timeStamp = bf_clock_ticks();
}

void	CRes :: OnLoadError(const char* fName)
{
	char	msg[255];
	sprintf_s(msg,256,"ResourceError : Can't find file %s\n", fName);
#if defined(BF_PORT)
	// Missing data (spec section 5): counted and traced, no box. CResMgr::Get
	// then hands out a placeholder or NULL, which the callers handled after
	// the original box too.
	bf_port_missing(BFT_RES, "res.missing", "%s", fName);
#elif defined(EPISODE_VER)
	sAssertM(false,msg);
#else
	ErrorMsg(msg);
#endif

}

CResMgr &	g_resMgr()
{
	static	CResMgr		resMgr;
	return	resMgr;
}


CResMgr :: CResMgr()
{
	m_usedForSprTexture = 0;
	m_usedForModelTexture = 0;
	m_usedForGNDTexture = 0;
	m_usedForSprite = 0;
	m_usedForSprAction = 0;
	m_usedForGAT = 0;
	m_usedForGND = 0;
	m_usedForIMF = 0;
	m_usedForModel = 0;

	m_ResMemAmount = 0;
	m_ResSprAmount = 0;
	m_ResTexAmount = 0;
	m_ResGatAmount = 0;
	m_ResGndAmount = 0;
	m_ResRswAmount = 0;
	m_ResModAmount = 0;
	m_ResWavAmount = 0;

	m_GetResSection = bf_sys_mutex_create();
}


CResMgr :: ~CResMgr()
{
	Reset();
	bf_sys_mutex_destroy(m_GetResSection);
}


void	CResMgr :: Reset()
{
	UnloadAll();
}

CRes *	CResMgr :: Get(const char *fNameInput, bool bRefresh)
{
	if(!fNameInput) return NULL;

	bf_sys_mutex_lock(m_GetResSection);

	char	fName[128];
	strcpy(fName, fNameInput);

	int extIndex = FindExtIndex(ExtractExt(ToLower(fName)));

	if(extIndex < 0)
	{
		bf_sys_mutex_unlock(m_GetResSection);
		return NULL;
	}

	char openFileName[256] = {0,};

	MakeOpenName(openFileName, fName, m_typeDir[extIndex]);
	CRes * newRes = FindRes(extIndex, openFileName);

	if(newRes != NULL && bRefresh == false)
	{
		newRes->UpdateTimeStamp();
		bf_sys_mutex_unlock(m_GetResSection);
		return newRes;
	}

	newRes = m_objTypes[extIndex]->Clone();

	if(newRes == NULL) return NULL;

	if(!newRes->Load(openFileName))
	{
		MakeOpenName(openFileName, GetRealResName(fName), m_typeDir[extIndex]);

		if(!newRes->Load(openFileName))
		{
			newRes->OnLoadError(fName);
#if defined(BF_PORT)
			// A missing sprite or action (a job, monster or headgear newer
			// than the client knows, or one the 2021 data renamed) is drawn
			// as the poring: the default sprite, loaded under the missing
			// name so that it is looked for once.
			const char* placeholder = NULL;
			const char* ext = ExtractExt(fName);
			if (!strcmp(ext, "spr"))
				placeholder = "sprite\\\xb8\xf3\xbd\xba\xc5\xcd\\poring.spr";
			else if (!strcmp(ext, "act"))
				placeholder = "sprite\\\xb8\xf3\xbd\xba\xc5\xcd\\poring.act";
			if (placeholder && newRes->Load(placeholder))
			{
				MakeOpenName(openFileName, fName, m_typeDir[extIndex]);
				newRes->UpdateInfo(openFileName, extIndex);
				m_fileList[extIndex].insert(res_map_value(&newRes->m_fName, newRes));
				bf_sys_mutex_unlock(m_GetResSection);
				return newRes;
			}
#endif
			delete newRes;
			bf_sys_mutex_unlock(m_GetResSection);
			return NULL;
		}
	}

	newRes->UpdateInfo(openFileName, extIndex);
	m_fileList[extIndex].insert(res_map_value(&newRes->m_fName, newRes));

	bf_sys_mutex_unlock(m_GetResSection);

	return newRes;
}

CRes *	CResMgr :: GetAlways(const char *fNameInput)
{
	return NULL;
}

BOOL	CResMgr :: IsExist(const char *fNameInput)
{
	if(!fNameInput) return FALSE;

	char	fName[128];

	strcpy(fName, fNameInput);

	int extIndex = FindExtIndex(ExtractExt(ToLower(fName)));

	if(extIndex < 0) {
		return FALSE;
	}

	char openFileName[256] = {0,};

	MakeOpenName(openFileName, fName, m_typeDir[extIndex]);

	CRes * newRes = FindRes(extIndex, openFileName);

	if(newRes != NULL) {
		newRes->UpdateTimeStamp();
		return TRUE;
	}

	return CFile::IsFileExist(openFileName);
}

void	CResMgr :: Unload(CRes *res)
{
	int extIndex = res->m_extIndex;

	sAssert(extIndex > 0 && extIndex < m_fileList.size());

	res_map_iter i = m_fileList[extIndex].find(&res->m_fName);

	i->second->Reset();
	delete i->second;

	m_fileList[extIndex].erase(i);
}

void	CResMgr :: UnloadAll()
{
	for(int a=0; a<m_fileList.size(); ++a)
	{
		res_map& res_file_list = m_fileList[a];

		res_map_iter i = res_file_list.begin();

		while (i != res_file_list.end()) {
			i->second->Reset();
			delete i->second;
			++i;
		}
		res_file_list.clear();
	}
}

void	CResMgr :: UnloadRarelyUsedRes()
{
	DWORD	expireTime = bf_clock_ticks() - EXPIRETIME;

	for(int a=0; a<m_fileList.size(); ++a)
	{
		res_map& res_file_list = m_fileList[a];

		res_map_iter i = res_file_list.begin();

		while (i != res_file_list.end()) {
			if (!i->second->m_lockCnt && i->second->m_timeStamp < expireTime) {
				i->second->Reset();
				delete i->second;
				i = res_file_list.erase(i);
			} else {
				++i;
			}
		}
	}
}

void	CResMgr :: UnloadUnlockedRes()
{
	DWORD	curTime = bf_clock_ticks();
	for(int a=0; a<m_fileList.size(); ++a)
	{
		res_map& res_file_list = m_fileList[a];

		res_map_iter i = res_file_list.begin();

		while (i != res_file_list.end()) {
			if (!i->second->m_lockCnt) {
				i->second->Reset();
				delete i->second;
				i = res_file_list.erase(i);
			} else {
				++i;
			}
		}
	}
}

void	CResMgr :: UnloadResByExt(const char* ext)
{
	int extIndex = FindExtIndex(ext);

	if(extIndex < 0)
	{
		return;
	}

	sAssert(extIndex >= 0 && extIndex < m_fileList.size());

	res_map& res_file_list = m_fileList[extIndex];

	res_map_iter i = res_file_list.begin();

	while (i != res_file_list.end()) {
		if(i->second->m_lockCnt == 0) {
			i->second->Reset();
			delete	i->second;
			i = res_file_list.erase(i);
		} else {
			++i;
		}
	}
}

void    CResMgr :: RegisterType(const char* resId, const char* baseDir, CRes *t)
{
	int extIndex = m_resExt.size();

	m_resExt.insert(ext_map_value(resId, extIndex));

	m_objTypes.push_back(t);
	m_typeDir.push_back(baseDir);
	m_fileList.resize(extIndex+1);
}


void	CResMgr :: ReadResNameTable(const char* resNameTable)
{
	m_realResName.clear();

	CFile fp;
	char * fName;
	fName = "ResNameTable.txt";
	if (!fp.Open(fName, CFile::modeRead))
		return;

	int size = fp.GetLength();
	char * buf = new char[size+1];

	fp.Read(buf, size);
	buf[size]=0;
	fp.Close();

	char * base = buf;
	string a, b;
	a.resize(0);
	b.resize(0);
	for (int i=0; i<size; i++) {
		switch (buf[i]) {
			case '#':
				buf[i] = 0;
				if (a.size()==0) {
					a = base;
				} else {
					b = base;
					m_realResName.insert(map <string, string> :: value_type(a, b));
					a.resize(0);
					b.resize(0);
				}
				base = buf+i+1;
				break;
			case 10:
			case 13:
				buf[i] = 0;
				base = buf+i+1;
				break;
		}
	}

	if (buf)
		delete [] buf;
}


void	CResMgr :: PrintRes(const char* fileName)
{
	FILE* file = fopen(fileName, "wt");

	if(!file) return;

	std::vector<res_map>::iterator iter = m_fileList.begin();
	for(; iter != m_fileList.end(); ++iter)
	{
		res_map& res_file_list = *iter;

		res_map_iter i = res_file_list.begin();

		for(; i!=res_file_list.end(); ++i) {
			fprintf(file, "%s\n", i->second->m_fName.c_str());
		}
	}

	fclose(file);
}

CRes *	CResMgr :: FindRes(int ext, const char* name)
{
	CHash fName(name);

	res_map_iter resPtr = m_fileList[ext].find(&fName);

	if (resPtr != m_fileList[ext].end()) return resPtr->second;

	return NULL;
}

void	CResMgr :: MakeOpenName(char* openName, const char* fName, const char* typeDir)
{
	int typeLen = strlen(typeDir);

	if(strncmp(fName, typeDir, typeLen))
	{
		strcpy(openName, typeDir);
		openName += typeLen;
	}

	strcpy(openName, fName);
}

const char* CResMgr :: ExtractExt(const char* fName)
{
	const char* ext = StrChrBackward(fName, '.');

	if(ext) return ext+1;

	return NULL;
}

int		CResMgr :: FindExtIndex(const char* ext_str)
{
	if(!ext_str) return -1;

	ext_map_iter i = m_resExt.find(ext_str);

	if(i == m_resExt.end())
	{
		char msg[255];
		sprintf(msg, "ResourceError : %s is Undefined Ext \n", ext_str);
		sAssertM(false, msg);
		return -1;
	}



	return i->second;
}

const char*	CResMgr :: GetRealResName(const char* resName)
{
	map <string, string> :: iterator iter = m_realResName.find(resName);

	if(iter == m_realResName.end()){
		string modifyResName(resName);
		string::size_type markPos = modifyResName.rfind("@");
		if((markPos != string::npos) && (markPos >= 4)){
			modifyResName.erase(markPos-4,3);
			iter = m_realResName.find(modifyResName);
			if(iter != m_realResName.end()) return iter->second.c_str();
		}
		return resName;
	}

	return iter->second.c_str();
}

char* CResMgr :: ToLower(char* text) {

	unsigned char* temp = (unsigned char*)text;

	while(*temp) {
		*temp = lower_table[*temp];
		++temp;
	}

	return text;
}
