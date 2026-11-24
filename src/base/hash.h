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
	unsigned int m_HashCode;
	char m_String[252];
};

struct PakPack {
public:
	unsigned int m_dataSize;
	unsigned int m_compressSize;
	unsigned int m_size;
	unsigned int m_Offset;
	char m_type;
	CHash m_fName;
};

#endif
