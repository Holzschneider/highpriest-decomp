#ifndef HIGHPRIEST_BASE_CDEC_H
#define HIGHPRIEST_BASE_CDEC_H


#ifndef	_DEF_BIT64
#define	_DEF_BIT64
typedef	union	{
	DWORD   dw[2];
	WORD    w[4];
	BYTE	b[8];
} BIT64;
#endif

class CDec;

class CDec {
public:
	CDec(char* seed);
	CDec();
	~CDec();
	void MakeKey(BIT64);
	void MakeKey(char* seed);
	int DataEncrypt(char* pIn, int inSize, char* pOut, int* outSize, int mod);
	int DataDecrypt(char* pIn, int inSize, char* pOut, int* outSize, int mod);
	int DataDecryptHeader(char* pIn, int inSize, char* pOut, int* outSize);
protected:
	enum { CRYPT_SIZE = 8 };
	enum { RANDOM_SPAN_SIZE = 13 };
	enum { MAX_ROUND = 16 };

	int Encrypt(int, BIT64*, BIT64, BIT64*);
	int Decrypt(int, BIT64*, BIT64, BIT64*);
	void IP(BIT64* text);
	void FP(BIT64* text);
	void RoundFunction(BIT64, BIT64*);
protected:
	BIT64 m_key[MAX_ROUND];
};

#endif
