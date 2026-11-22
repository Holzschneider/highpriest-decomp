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
	virtual unsigned long size() = 0;
	virtual const unsigned char* read(unsigned long, unsigned long) = 0;
};

class CMemMapFile : public CMemFile {
public:
	CMemMapFile(const char* name) { init(); open(name); }
	CMemMapFile() { init(); }
	virtual ~CMemMapFile() { close(); }
	bool open(const char* name);
	void close();
	virtual unsigned long size() { return m_dwFileSize; }
	virtual const unsigned char* read(unsigned long offset, unsigned long size);
protected:
	void init();
protected:
	void* m_hFile;
	void* m_hFileMap;
	unsigned long m_dwFileSize;
	unsigned long m_dwOpenOffset;
	unsigned long m_dwOpenSize;
	unsigned long m_dwFileMappingSize;
	unsigned long m_dwAllocationGranuarity;
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
	unsigned long m_FileVer;
	unsigned long m_FileCount;
	unsigned long m_PakInfoOffset;
	unsigned long m_PakInfoSize;
	std::vector<PakPack> m_PakPack;
	std::vector<unsigned char> m_pDecBuf;
	CMemFile* m_memFile;
};

#endif
