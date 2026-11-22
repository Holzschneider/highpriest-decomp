#include "common.h"
#include "RegMgr.h"

bool	RegKey :: open(HKEY predKey, const char* subKeyName)
{
	if(RegOpenKeyEx(predKey, subKeyName, 0, KEY_ALL_ACCESS, &m_key) == ERROR_SUCCESS) {
		return true;
	}

	return false;
}

bool	RegKey :: create(HKEY predKey, const char* subKeyName)
{
	DWORD dwDisposition;

	if(RegCreateKeyEx(predKey, subKeyName, NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &m_key, &dwDisposition) == ERROR_SUCCESS) {
		return true;
	}

	return false;
}

bool	RegKey :: setValue(const char* valueName, DWORD regType, const BYTE* value, DWORD valueLen)
{
	if(m_key) {
		return RegSetValueEx(m_key, valueName, NULL, regType, value, valueLen)  == ERROR_SUCCESS;
	}

	return false;
}

bool	RegKey :: queryValueSize(const char* valueName, DWORD* valueLen)
{
	if(m_key) {
		if(RegQueryValueEx(m_key, valueName, NULL, NULL, NULL, valueLen)  == ERROR_SUCCESS) {
			return true;
		}
	}

	return false;
}

bool	RegKey :: queryValue(const char* valueName, DWORD regType, BYTE* value, DWORD valueLen)
{
	if(m_key) {
		DWORD type;
		if(RegQueryValueEx(m_key, valueName, NULL, &type, value, &valueLen)  == ERROR_SUCCESS) {
			return type == regType;
		}
	}

	return false;
}
