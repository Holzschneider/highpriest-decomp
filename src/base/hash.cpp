#include "common.h"





#include "hash.h"

#include <string.h>

#pragma intrinsic(strcpy, strcmp, strlen, memcpy, memset)


CHash::CHash()
{
	SetString("");
}

CHash::CHash(const char* str)
{
	SetString(str);
}
void CHash::SetString(const char* str)
{
	strcpy(m_String, str);
	strlwr(m_String);
	CreateHashCode();
}

bool CHash::operator<(const CHash& hash) const
{
	if (m_HashCode != hash.m_HashCode)
	{
		return m_HashCode < hash.m_HashCode;
	}
	return strcmp(m_String, hash.m_String) < 0;
}

bool CHash::operator==(const CHash& hash) const
{
	return !operator!=(hash);
}

bool CHash::operator!=(const CHash& hash) const
{
	if (m_HashCode != hash.m_HashCode)
	{
		return true;
	}
	return strcmp(m_String, hash.m_String) != 0;
}

void CHash::CreateHashCode()
{
	unsigned int len = strlen(m_String);
	unsigned int result = 5381;

	for (unsigned int i = 0; i < len; i++)
	{
		result = ((result << 5) + result) + (unsigned int)(int)(signed char)m_String[i];
	}

	m_HashCode = result;
}
