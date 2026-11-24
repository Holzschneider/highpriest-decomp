#ifndef HIGHPRIEST_SKIN_H
#define HIGHPRIEST_SKIN_H


class CSkinMgr;

class CSkinMgr {
public:
	CSkinMgr();
	virtual ~CSkinMgr();
	void Init();
	int GetNumSkin();
	int GetCurrentSkinNum();
	void SetCurrentSkinNum(int skinNum);
	const char* GetSkinFolderName(int skinNum);
	const char* GetSkinCurrentFolderName();
	const char* GetCurrentSkinName();
	const char* GetSkinName(int skinNum);
	void DeleteTmpSkinFolder();
	void GetColorChipColor(int x, int y, unsigned int& r, unsigned int& g, unsigned int& b);
	void ExtractFilesIfZipSkin();
	std::string m_savedSkinName;
private:
	int m_curSkinNum;
	std::vector<std::string> m_skinNameList;
	std::vector<std::string> m_skinFolderNameList;
	std::vector<int> m_isZipSkinExtractedList;
	std::vector<int> m_isZipSkinList;
};

inline int CSkinMgr::GetCurrentSkinNum()
{
	return m_curSkinNum;
}

inline int CSkinMgr::GetNumSkin()
{
	return m_skinFolderNameList.size();
}
extern CSkinMgr g_skinMgr;

#endif
