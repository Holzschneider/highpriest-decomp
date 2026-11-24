#ifndef HIGHPRIEST_BASE_GPAK_H
#define HIGHPRIEST_BASE_GPAK_H

#include "hash.h"

class CMemFile;
class CGPak;

typedef std::vector<PakPack>::iterator pak_map_iter;

struct PakPrtLess : public std::binary_function<const PakPack&, const PakPack&, bool> {

	unsigned char operator()(const PakPack& leftPak, const PakPack& rightPak) const {
		return leftPak.m_fName < rightPak.m_fName;
	}

	unsigned char operator()(const PakPack& leftPak, const CHash& right) const {
		return leftPak.m_fName < right;
	}

};

class CMemFile {
public:
	virtual ~CMemFile() {}
	virtual unsigned int size() = 0;
	virtual const unsigned char* read(unsigned int, unsigned int) = 0;
};

class CMemMapFile : public CMemFile {
public:
	CMemMapFile(const char* name) { init(); open(name); }
	CMemMapFile() { init(); }
	virtual ~CMemMapFile() { close(); }
	bool open(const char* name);
	void close();
	virtual unsigned int size() { return m_dwFileSize; }
	virtual const unsigned char* read(unsigned int offset, unsigned int size);
protected:
	void init();
protected:
	void* m_hFile;
	void* m_hFileMap;
	unsigned int m_dwFileSize;
	unsigned int m_dwOpenOffset;
	unsigned int m_dwOpenSize;
	unsigned int m_dwFileMappingSize;
	unsigned int m_dwAllocationGranuarity;
	const unsigned char* m_pFile;
	std::vector<unsigned char> m_pFileBuf;
};

class CGPak {
public:
	CGPak() { Init(); }
	virtual ~CGPak() {}
	bool Open(CMemFile* memFile);
	bool GetInfo(const CHash& fName, PakPack* pakPack);
	bool GetData(const PakPack& pakPack, void* buffer);
protected:
	void Init();
	char ChangeLHBit_BYTE(char ch);
	void ModifyString(char* dest, const char* src, int bufferSize);
	void ModifyString(char* dest, const char* src);
	int IsNeverEncrypt(const char* fileName);
	char* MakeSeed(const char* fileName, char* seed);
	bool OpenPak01();
	bool OpenPak02();
protected:
	unsigned int m_FileVer;
	unsigned int m_FileCount;
	unsigned int m_PakInfoOffset;
	unsigned int m_PakInfoSize;
	std::vector<PakPack> m_PakPack;
	std::vector<unsigned char> m_pDecBuf;
	CMemFile* m_memFile;
};

#endif
