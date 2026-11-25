#include "common.h"
#include "GPak.h"
#include "File.h"
#include "hash.h"

#include "bf_types.h"
#include <stdlib.h>
#include <string.h>
#include <list>
#include <utility>



bool            g_readFolderFirst = false;



CFileMgr g_fileMgr;






CFileMgr::~CFileMgr()
{
	for (GPakListIter iter = m_pakList.begin(); iter != m_pakList.end(); ++iter) {
		delete iter->first;
		delete iter->second;
	}
}

bool CFileMgr::AddPak(const char* name)
{
	typedef std::pair<CMemFile*, CGPak*> GDataPair;
	m_pakList.push_front(GDataPair(new CMemMapFile(name), new CGPak));
#if defined(BF_PORT)
	bool opened = m_pakList.front().second->Open(m_pakList.front().first);
	BF_TRACE(BFT_FILE, "archive %s: %s", name, opened ? "opened" : "NOT OPENED");
	if (!opened)
		bf_count("file.archive.not_opened", 1);
	return opened;
#else
	return m_pakList.front().second->Open(m_pakList.front().first);
#endif
}

#if defined(BF_PORT)
// Traces one lookup and passes its result through.
static void* TraceLookup(const char* name, const char* source, void* data)
{
	if (data) {
		BF_TRACE(BFT_FILE, "%-7s %s", source, name);
	} else if (!source[0]) {
		BF_TRACE(BFT_FILE, "MISSING %s", name);
		bf_count("file.missing", 1);
	}
	return data;
}
#else
#define TraceLookup(name, source, data) (data)
#endif

void* CFileMgr::GetData(const char* name, unsigned int* size)
{
	if (g_readFolderFirst) {
		void* data;
		if (data = TraceLookup(name, "folder", GetFile(name, size))) {
			return data;
		} else if (data = TraceLookup(name, "archive", GetPak(name, size))) {
			return data;
		} else {
			return TraceLookup(name, "", NULL);
		}
	} else {
		void* data;
		if (data = TraceLookup(name, "archive", GetPak(name, size))) {
			return data;
		} else if (data = TraceLookup(name, "folder", GetFile(name, size))) {
			return data;
		} else {
			return TraceLookup(name, "", NULL);
		}
	}
}

void CFileMgr::ReleaseData(void* data)
{
	free(data);
}

bool CFileMgr::IsDataExist(const char* name)
{
	for (GPakListIter iter = m_pakList.begin(); iter != m_pakList.end(); ++iter) {
		CHash tmp(name);
		if (iter->second->GetInfo(tmp, NULL)) {
			return true;
		}
	}
	return IsFileExist(name);
}

void* CFileMgr::GetPak(const char* name, unsigned int* size)
{
	for (GPakListIter iter = m_pakList.begin(); iter != m_pakList.end(); ++iter) {
		PakPack pakPack;
		CHash tmp(name);
		if (iter->second->GetInfo(tmp, &pakPack)) {
			*size = pakPack.m_size;
			void* data = calloc(1, pakPack.m_size + 8);
			if (iter->second->GetData(pakPack, data)) {
				return data;
			}
			free(data);
		}
	}
	return NULL;
}

void* CFileMgr::GetFile(const char* name, unsigned int* size)
{
	bf_file* hFile = bf_file_open(name, BF_FILE_READ);
	if (hFile != NULL) {
		*size = (unsigned int)bf_file_size(hFile);
		if (size != NULL) {
			void* data = calloc(1, *size ? *size : 1);
			if (data) {
				bf_file_read(hFile, data, *size);
				bf_file_close(hFile);
				return data;
			}
			free(data);
		}
		bf_file_close(hFile);
	}
	return NULL;
}

bool CFileMgr::IsFileExist(const char* name)
{
	return bf_file_stat(name, NULL) ? true : false;
}






CFile::CFile()
{
	m_hFile = NULL;
	m_buf = NULL;
}

CFile::~CFile()
{
	Close();
}

bool CFile::Open(const char* fName, unsigned int nOpenFlags)
{
	if (nOpenFlags == modeRead) {
		MakeFileName(m_fileName, fName);
		m_cursor = 0;

		m_buf = (char*)g_fileMgr.GetData(m_fileName, &m_size);

		if (m_buf != NULL) {
			return true;
		} else {
			return false;
		}
	} else if (nOpenFlags & modeWrite) {
		strcpy(m_fileName, fName);
		m_hFile = bf_file_open(fName, BF_FILE_READ | BF_FILE_WRITE | BF_FILE_CREATE | BF_FILE_TRUNCATE);
		if (m_hFile != NULL) {
			m_size = (unsigned int)bf_file_size(m_hFile);
			return true;
		} else {
#if defined(BF_PORT)
			bf_port_missing(BFT_FILE, "file.write_failed", "%s", m_fileName);
#else
			ErrorMsg(m_fileName);
#endif
			return false;
		}
	} else {
		return false;
	}
}

void CFile::Close()
{
	if (m_hFile) {
		bf_file_close(m_hFile);
		m_hFile = NULL;
	} else if (m_buf) {
		free(m_buf);
		m_buf = NULL;
	}
}

bool CFile::Read(void* lpBuf, unsigned int nCount)
{
	if (m_hFile) {
		return bf_file_read(m_hFile, lpBuf, nCount) >= 0 ? true : false;
	} else if (m_buf) {
		if (m_cursor + nCount > m_size) {
			return false;
		}
		memcpy(lpBuf, m_buf + m_cursor, nCount);
		m_cursor += nCount;
		return true;
	} else {
		return false;
	}
}

bool CFile::Write(const void* lpBuf, unsigned int nCount)
{
	if (m_hFile) {
		return bf_file_write(m_hFile, lpBuf, nCount) >= 0 ? true : false;
	} else {
		return false;
	}
}

bool CFile::Seek(int lOff, unsigned int nFrom)
{
	if (m_hFile) {
		bf_file_seek(m_hFile, lOff, nFrom);
		return true;
	} else if (m_buf) {
		switch (nFrom) {
		case begin:   m_cursor = lOff;          break;
		case current: m_cursor += lOff;         break;
		case end:     m_cursor = m_size + lOff; break;
		}
		if (m_cursor > m_size) {
			m_cursor = m_size;
			return false;
		} else {
			return true;
		}
	} else {
		return false;
	}
}

bool CFile::IsFileExist(const char* fName)
{
	char fullName[256];
	MakeFileName(fullName, fName);

	return g_fileMgr.IsDataExist(fullName);
}

void CFile::MakeFileName(char* output, const char* input)
{
#if defined(BF_PORT)
	// An absolute path (one built on g_baseDir3, the game folder's POSIX
	// path) is not below g_baseDir; on Windows the drive letter's ':' did this.
	if (input[0] == '/') {
		strcpy(output, input);
		return;
	}
#endif
	strcpy(output, g_baseDir);
	strcat(output, input);

	if (strstr(output, ":")) {
		char temp[1024];
		int pos = strstr(output, ":") - output - 1;
		memcpy(temp, output + pos, strlen(output) - pos + 1);
		strcpy(output, temp);
	}

	while (strstr(output, "//")) {
		int pos = strstr(output, "//") - output;
		char temp[1024];
		memcpy(temp, output, pos);
		strcpy(temp + pos, output + pos + 1);
		strcpy(output, temp);
	}
}
