#ifndef HIGHPRIEST_BASE_FILE_H
#define HIGHPRIEST_BASE_FILE_H


class CMemFile;
class CGPak;

class CFile {
public:
	enum OpenFlags {
		modeRead = 0,
		modeWrite = 1,
		modeReadWrite = 2,
		shareCompat = 0,
		shareExclusive = 16,
		shareDenyWrite = 32,
		shareDenyRead = 48,
		shareDenyNone = 64,
		modeNoInherit = 128,
		modeCreate = 4096,
		modeNoTruncate = 8192,
		typeText = 16384,
		typeBinary = 32768,
	};

	enum SeekPosition {
		begin = 0,
		current = 1,
		end = 2,
	};

	CFile();
	~CFile();
	bool Open(const char* fName, unsigned int nOpenFlags);
	bool Read(void* lpBuf, unsigned int nCount);
	bool Write(const void* lpBuf, unsigned int nCount);
	bool Seek(int lOff, unsigned int nFrom);
	void Close();
	char* GetFileName() { return m_fileName; }
	unsigned int GetLength() { return m_size; }
	const unsigned char* GetBuf() { return (const unsigned char*)m_buf; }
	const void* GetCurBuf(int lOff = 0) { void* temp = m_buf + m_cursor; m_cursor += lOff; return temp; }
	static bool __cdecl IsFileExist(const char* fName);
protected:
	static void __cdecl MakeFileName(char* output, const char* input);
	bf_file* m_hFile;
	char* m_buf;
	unsigned int m_cursor;
	unsigned int m_size;
	char m_fileName[128];
};


typedef std::pair<CMemFile*, CGPak*>   GDataPair;
typedef std::list<GDataPair>           GPakList;
typedef GPakList::iterator             GPakListIter;

class CFileMgr {
public:
	~CFileMgr();
	bool AddPak(const char* name);
	void* GetData(const char* name, unsigned int* size);
	void ReleaseData(void* data);
	bool IsDataExist(const char* name);



protected:
	void* GetPak(const char* name, unsigned int* size);
	void* GetFile(const char* name, unsigned int* size);
	bool IsFileExist(const char* name);
	friend class CFile;
protected:
	GPakList m_pakList;
};
extern CFileMgr g_fileMgr;

#endif
