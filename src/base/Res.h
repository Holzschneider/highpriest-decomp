#ifndef HIGHPRIEST_BASE_RES_H
#define HIGHPRIEST_BASE_RES_H

#include "hash.h"


class CRes {
public:
	CRes();

	virtual ~CRes() {}
	virtual bool LoadFromBuffer(const char* fName, const unsigned char* buffer, int size) { return false; }
	int Lock();
	int Unlock();
	const char* GetName();
	void UpdateInfo(const char* fName, int extIndex);
	void UpdateTimeStamp();
protected:
	virtual CRes* Clone() const = 0;
	virtual bool Load(const char*) = 0;
	virtual void Reset() {}
	virtual void OnLoadError(const char* fName);
protected:
	int m_lockCnt;
	unsigned int m_timeStamp;
	int m_extIndex;
	CHash m_fName;
public:

	friend class CResMgr;
};

struct ResPtrLess : public std::binary_function<CHash*, CHash*, bool> {
	bool operator()(CHash* left, CHash* right) const {
		return *left < *right;
	}
};

class CResMgr {
public:
	CResMgr();
	~CResMgr();
	void Reset();
	CRes* Get(const char* fNameInput, bool bRefresh);
	CRes* GetAlways(const char* fNameInput);
	int IsExist(const char* fNameInput);
	void Unload(CRes* res);
	void UnloadAll();
	void UnloadRarelyUsedRes();
	void UnloadUnlockedRes();
	void UnloadResByExt(const char* ext);
	void RegisterType(const char* resId, const char* baseDir, CRes* t);
	void ReadResNameTable(const char* ext);
	void PrintRes(const char* ext);
private:
	CRes* FindRes(int ext, const char* name);
	void MakeOpenName(char* openName, const char* fName, const char* typeDir);
	const char* ExtractExt(const char* fName);
	int FindExtIndex(const char* fNameInput);
	const char* GetRealResName(const char* fName);
	char* ToLower(char* text);
	typedef std::map<const char*, int, CharPrtLess> ext_map;
	typedef ext_map::iterator ext_map_iter;
	typedef ext_map::value_type ext_map_value;
	typedef std::map<CHash*, CRes*, ResPtrLess> res_map;
	typedef res_map::iterator res_map_iter;
	typedef res_map::value_type res_map_value;
private:
	std::map<char const *, int, CharPrtLess > m_resExt;
	std::vector<char const *> m_typeDir;
	std::vector<CRes *> m_objTypes;
	std::vector<std::map<CHash *, CRes *, ResPtrLess > > m_fileList;
	_RTL_CRITICAL_SECTION m_GetResSection;
	std::map<std::string, std::string> m_realResName;
public:
	unsigned int m_usedForSprTexture;
	unsigned int m_usedForModelTexture;
	unsigned int m_usedForGNDTexture;
	unsigned int m_usedForSprite;
	unsigned int m_usedForSprAction;
	unsigned int m_usedForGAT;
	unsigned int m_usedForGND;
	unsigned int m_usedForIMF;
	unsigned int m_usedForModel;
	unsigned int m_ResMemAmount;
	unsigned int m_ResSprAmount;
	unsigned int m_ResTexAmount;
	unsigned int m_ResGatAmount;
	unsigned int m_ResGndAmount;
	unsigned int m_ResRswAmount;
	unsigned int m_ResModAmount;
	unsigned int m_ResWavAmount;
};
CResMgr& g_resMgr();

class Exemplar { public: Exemplar() {} };



#define DECLARE_RESOURCE(class_name) \
	virtual CRes* Clone() const { return new class_name; } \
	class_name(Exemplar, const char* resid, const char* baseDir) { g_resMgr().RegisterType(resid, baseDir, this); }


#endif
