#ifndef HIGHPRIEST_BASE_HASH_H
#define HIGHPRIEST_BASE_HASH_H



class CHash {
public:
	CHash(const char* str);
	CHash();
	void SetString(const char* str);
	const char* c_str() const { return m_String; }
	bool operator<(const CHash& hash) const;
	bool operator==(const CHash& hash) const;
	bool operator!=(const CHash& hash) const;
protected:
	void CreateHashCode();
	unsigned long m_HashCode;
	char m_String[252];
};

struct PakPack {
public:
	unsigned long m_dataSize;
	unsigned long m_compressSize;
	unsigned long m_size;
	unsigned long m_Offset;
	char m_type;
	CHash m_fName;
};

#endif
