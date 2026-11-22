#ifndef HIGHPRIEST_BASE_REGMGR_H
#define HIGHPRIEST_BASE_REGMGR_H


class RegKey;
struct HKEY__;

class RegKey {
public:
	RegKey();
	~RegKey();
	bool open(HKEY__* predKey, const char* subKeyName);
	bool create(HKEY__* predKey, const char* subKeyName);
	void close();
	bool setValue(const char* valueName, unsigned long regType, const unsigned char* value, unsigned long valueLen);
	bool queryValue(const char* valueName, unsigned long regType, unsigned char* value, unsigned long valueLen);
	bool queryValueSize(const char* valueName, unsigned long* valueLen);
protected:
	HKEY__* m_key;
};

#endif
