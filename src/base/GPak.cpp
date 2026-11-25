#include "common.h"
#include "File.h"
#include "GPak.h"
#include "CDec.h"

#include "bf_types.h"
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <vector>



#define HEADOFFSET             46
#define MAX_EXT_STRING         4
#define DATA_COMPRESSED        0x00000001
#define DATA_ENCRYPTED         0x00000002
#define DATA_HEADER_ENCRYPTED  0x00000004

#pragma pack(push, 1)
struct pak_file_header {
	DWORD Offset;
	DWORD DummyCnt;
	DWORD FileCnt;
	DWORD Version;
};
struct pak_data_header {
	DWORD m_compressSize;
	DWORD m_dataSize;
	DWORD m_size;
	char  m_type;
	DWORD m_Offset;
};
struct pak_info_header {
	DWORD m_compressedSize;
	DWORD m_size;
};
#pragma pack(pop)

char EncExtString[MAX_EXT_STRING][5] = {
	".gnd",
	".gat",
	".act",
	".str"
};





// The whole archive is mapped at once (bf_file_map_open_file); a view at an
// offset is a pointer into that mapping, valid until close().
static const BYTE*	MapView(bf_file_mapping* fileMap, DWORD offset)
{
	return (const BYTE*)bf_file_mapping_data(fileMap) + offset;
}

bool	CMemMapFile :: open(const char* name)
{
	m_hFile = bf_file_open(name, BF_FILE_READ);

	if(m_hFile == NULL)
	{
		return false;
	}

	m_dwFileSize = (DWORD)bf_file_size(m_hFile);
	m_dwOpenOffset = 0;
	m_dwOpenSize = 0;

	m_hFileMap = bf_file_map_open_file(m_hFile);

	if(m_hFileMap != NULL && bf_file_mapping_size(m_hFileMap) != 0)	// an empty file has no mapping, as on Windows
	{
		return true;
	}
	else
	{
		m_dwFileMappingSize = 0;
		return false;
	}
}

void	CMemMapFile :: close()
{
	if(m_dwFileMappingSize && m_pFile)
	{
		m_pFile = NULL;
	}

	if(m_hFileMap)
	{
		bf_file_unmap(m_hFileMap);
		m_hFileMap = NULL;
	}
	if(m_hFile)
	{
		bf_file_close(m_hFile);
		m_hFile = NULL;
	}
}

const BYTE*	CMemMapFile :: read(DWORD offset, DWORD size)
{
	sAssert(offset < m_dwFileSize);
	sAssert(offset+size <= m_dwFileSize);

	if(m_dwFileMappingSize)
	{
		if(offset < m_dwOpenOffset || m_dwOpenOffset+m_dwOpenSize < offset+size)
		{

			if(m_pFile)
			{
				m_pFile = NULL;
			}

			while(m_dwFileMappingSize)
			{
				if(m_dwFileMappingSize < size)
					break;

				m_dwOpenOffset = offset & m_dwAllocationGranuarity;
				m_dwOpenSize = m_dwFileMappingSize;
				if(m_dwOpenOffset+m_dwOpenSize > m_dwFileSize)
				{
					m_pFile = MapView(m_hFileMap, m_dwOpenOffset);
					m_dwOpenSize = m_dwFileSize - m_dwOpenOffset;
				}
				else if(m_dwOpenOffset+m_dwOpenSize > offset+size)
				{
					m_pFile = MapView(m_hFileMap, m_dwOpenOffset);
				}
				else
				{
					m_pFile = MapView(m_hFileMap, m_dwOpenOffset);
				}

				if(m_pFile)
					return m_pFile + (offset - m_dwOpenOffset);

				m_dwFileMappingSize /= 2;
			}

			bf_file_unmap(m_hFileMap);
			m_hFileMap = NULL;
			m_pFile = 0;
			m_dwOpenSize = 0;
			m_dwFileMappingSize = 0;
			sAssert(m_pFile != (BYTE*)"MemoryMapped Failed!! Future work in file mode!!");
		}
		else
		{
			return m_pFile + (offset - m_dwOpenOffset);
		}
	}

	if(m_dwOpenSize < size)
	{

		m_dwOpenSize = size;

		m_pFileBuf.resize(m_dwOpenSize);

		m_pFile = m_pFileBuf.data();
	}

	DWORD dwRead;
	bf_file_seek(m_hFile, offset, 0);
	dwRead = (DWORD)bf_file_read(m_hFile, m_pFileBuf.data() , size);
	sAssert(dwRead == size);

	return m_pFile;
}

void	CMemMapFile :: init()
{
	DWORD allocationGranularity = 0x10000;	// GetSystemInfo's on Windows: views start at multiples of 64 KB

	m_dwFileMappingSize = 0x20000000;
	m_dwAllocationGranuarity =  0xFFFFFFFF - (allocationGranularity-1);

	m_hFile = NULL;
	m_hFileMap = NULL;
	m_pFile = NULL;
	m_dwFileSize = 0;
	m_dwOpenOffset = 0;
	m_dwOpenSize = 0;
}

bool	CGPak :: Open(CMemFile* memFile)
{
	if(memFile == NULL) return false;

	m_memFile = memFile;

	const BYTE* pFileIter = m_memFile->read(0, HEADOFFSET);

	if (memcmp(pFileIter, "Master of Magic", sizeof("Master of Magic")) != 0) {
		sAssert(false);
		return false;
	}

	pFileIter += 30;

	pak_file_header* pakFileHeader = (pak_file_header*)pFileIter;

	m_FileVer = pakFileHeader->Version;
	m_FileCount = pakFileHeader->FileCnt - (pakFileHeader->DummyCnt + 7);

	m_PakInfoOffset = pakFileHeader->Offset + HEADOFFSET;

	if((m_FileVer & 0xFF00) == 0x0100)
	{

		if(OpenPak01() == true) return true;

	}
	else if((m_FileVer & 0xFF00) == 0x0200)
	{

		if(OpenPak02() == true) return true;

	}

	Init();

	return false;
}

bool	CGPak :: GetInfo(const CHash& fName, PakPack* pakPack)
{
	pak_map_iter pakIter = lower_bound(m_PakPack.begin(), m_PakPack.end(), fName, PakPrtLess());

	if (pakIter == m_PakPack.end() || pakIter->m_fName != fName)
	{
		return false;
	}
	else
	{
		if(pakPack) *pakPack = *pakIter;
		return true;
	}
}

bool	CGPak :: GetData(const PakPack& pakPack, void* buffer)
{
	const BYTE* pFile = m_memFile->read(HEADOFFSET + pakPack.m_Offset, pakPack.m_dataSize);

	if(pFile)
	{
		const BYTE* pCompressedData;
		DWORD DecodedSize = 0;

		char seed[8];
		CDec dec(MakeSeed(pakPack.m_fName.c_str(), seed));

		if(pakPack.m_type & DATA_ENCRYPTED)
		{

			int mod = 0;
			for(DWORD temp = pakPack.m_compressSize; temp; temp/=10) ++mod;
			if(mod <= 0) mod = 1;

			sAssert(m_pDecBuf.size() >= pakPack.m_dataSize);

			dec.DataDecrypt((char*)pFile, pakPack.m_dataSize, (char*)m_pDecBuf.data() , (int*)&DecodedSize, mod);
			pCompressedData  = m_pDecBuf.data() ;

		}
		else if(pakPack.m_type & DATA_HEADER_ENCRYPTED)
		{

			dec.DataDecryptHeader((char*)pFile, pakPack.m_dataSize, (char*)m_pDecBuf.data() , (int*)&DecodedSize);
			pCompressedData  = m_pDecBuf.data();

		}
		else
		{
			DecodedSize = pakPack.m_compressSize;
			pCompressedData = pFile;
		}

		if(pakPack.m_type & DATA_COMPRESSED)
		{
			DWORD size = pakPack.m_size;
			uncompress((BYTE*)buffer, &size, pCompressedData, DecodedSize);
		}
		else
		{
			memcpy(buffer, m_pDecBuf.data(), pakPack.m_size);
		}

		return true;
	}

	return false;
}

void CGPak :: Init()
{
	m_FileVer		= 0;
	m_FileCount		= 0;
	m_PakInfoOffset	= 0;
	m_PakInfoSize	= 0;

	m_PakPack.clear();
	m_pDecBuf.clear();

	m_memFile = NULL;
}

BOOL	CGPak :: IsNeverEncrypt(const char *fileName)
{
	fileName = strrchr(fileName, '.');

	if(fileName)
	{
		for(int i = 0; i < MAX_EXT_STRING; i++)
		{
			if(strcmpi(fileName, EncExtString[i]) == 0)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

char*	CGPak :: MakeSeed(const char *fileName, char *seed)
{
	const char* fileNameEnd = fileName + strlen(fileName);

	for(const char* fileNameIter = fileName; fileNameIter<fileNameEnd; ++fileNameIter)
	{
		if(*fileNameIter == '\\' || *fileNameIter == '/') fileName = fileNameIter + 1;
	}

	int len = fileNameEnd - fileName;

	if (len < 6)
	{
		seed[0] = 'n';
		seed[7] = 'k';
		memcpy(seed + 1, fileName, len);
		memcpy(seed + 1 + len, fileName, (6 - len) );
	}
	else
	{
		seed[0] = 'n';
		seed[7] = 'k';
		memcpy(seed + 1, fileName, 6);
	}

	return seed;
}

bool	CGPak :: OpenPak01()
{

	m_PakInfoSize = m_memFile->size() - m_PakInfoOffset;

	const BYTE* pFileIter = m_memFile->read(m_PakInfoOffset, m_PakInfoSize);

	m_PakPack.resize(m_FileCount);

	DWORD decBufSize = 0;

	for (unsigned a = 0; a < m_FileCount; a++)
	{
		int len = *(DWORD*)(pFileIter);
		pFileIter += sizeof(DWORD);

		char fName[256];
		if (m_FileVer < 0x0101)
		{
			ModifyString(fName, (char*)(pFileIter), strlen((char*)(pFileIter))+1);
		}
		else if (m_FileVer <= 0x0103)
		{
			int KeyCnt, DecSize;
			char validfName[256], seed[8], strbuf[20];
			pFileIter += 2;
			ModifyString(fName, (char*)pFileIter, len - 6);

			if (m_FileVer == 0x0101)
			{
				KeyCnt = 95000 - (a * 2) + 1;
			}
			else
			{
				KeyCnt = 95000 - (((a * 5) + 1) / 2) * 3 + 7;
			}

			if (KeyCnt < 10000)
				KeyCnt += 85000;

			itoa(KeyCnt, strbuf, 10);

			memcpy(seed + 2, strbuf, 5);
			seed[0] = 'P';
			seed[1] = 'r';
			seed[7] = 'e';

			sAssert(len - 6 < sizeof(validfName));

			CDec Dec(seed);
			Dec.DataDecrypt(fName, (len - 6), validfName, &DecSize, 1);
			strcpy(fName, validfName);
			len -= 2;
		}
		else
		{
			return false;
		}

		pFileIter += len;

		pak_data_header* pakDataHeader = (pak_data_header*)pFileIter;
		pFileIter += sizeof(pak_data_header);

		PakPack& pakPack = m_PakPack[a];

		pakPack.m_compressSize	= pakDataHeader->m_compressSize-(pakDataHeader->m_size+715);
		pakPack.m_dataSize		= pakDataHeader->m_dataSize-37579;
		pakPack.m_size			= pakDataHeader->m_size;
		pakPack.m_type			= pakDataHeader->m_type;
		pakPack.m_Offset		= pakDataHeader->m_Offset;
		pakPack.m_fName.SetString(fName);

		if (IsNeverEncrypt(fName))
		{
			pakPack.m_type |= DATA_HEADER_ENCRYPTED;
		}
		else
		{
			pakPack.m_type |= DATA_ENCRYPTED;
		}

		decBufSize = max(decBufSize, pakPack.m_dataSize);
	}

	sort(m_PakPack.begin(), m_PakPack.end(), PakPrtLess());

	m_pDecBuf.resize(decBufSize);

	return true;
}

bool	CGPak :: OpenPak02()
{

	const BYTE* pFileIter = m_memFile->read(m_PakInfoOffset, sizeof(pak_info_header));

	pak_info_header pakInfoHeader = *((pak_info_header*)pFileIter);

	m_PakInfoSize = sizeof(pak_info_header) + pakInfoHeader.m_compressedSize;


	pFileIter = m_memFile->read(m_PakInfoOffset+sizeof(pak_info_header), pakInfoHeader.m_compressedSize);

	DWORD size = pakInfoHeader.m_size + 8;
	std::vector<BYTE> pakInfoBuffer(size);

	if(uncompress(&pakInfoBuffer[0], &size, pFileIter, pakInfoHeader.m_compressedSize) != 0)
		return false;


	pFileIter = &pakInfoBuffer[0];

	m_PakPack.resize(m_FileCount);

	DWORD decBufSize = 0;

	for (unsigned a = 0; a < m_FileCount; a++)
	{
		const char* fName = (const char*)pFileIter;

		pFileIter += strlen(fName) + 1;

		pak_data_header* pakDataHeader = (pak_data_header*)pFileIter;
		pFileIter += sizeof(pak_data_header);

		PakPack& pakPack = m_PakPack[a];

		pakPack.m_compressSize	= pakDataHeader->m_compressSize;
		pakPack.m_dataSize	= pakDataHeader->m_dataSize;
		pakPack.m_size			= pakDataHeader->m_size;
		pakPack.m_type			= pakDataHeader->m_type;
		pakPack.m_Offset		= pakDataHeader->m_Offset;
		pakPack.m_fName.SetString(fName);

		decBufSize = max(decBufSize, pakPack.m_dataSize);
	}

	sort(m_PakPack.begin(), m_PakPack.end(), PakPrtLess());

	m_pDecBuf.resize(decBufSize);

	return true;
}

char	CGPak :: ChangeLHBit_BYTE(char ch)
{
	char tmpChar;

	tmpChar = (ch >> 4) & 0x0f;
	tmpChar |= (ch << 4) & 0xf0;

	return tmpChar;
}

void	CGPak :: ModifyString(char* dest, const char *src)
{
	int len = strlen(src) + 1;

	for(int i=0; i<len; ++i) dest[i] = ChangeLHBit_BYTE(src[i]);
}

void	CGPak :: ModifyString(char* dest, const char *src, int bufferSize)
{
	for(int i=0; i<bufferSize; ++i) dest[i] = ChangeLHBit_BYTE(src[i]);
}
