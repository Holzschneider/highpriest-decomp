#include "common.h"
#include "CDec.h"


static const BYTE mask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
static const BYTE ip_table[64] = {
	58, 50, 42, 34, 26, 18, 10, 2,
	60, 52, 44, 36, 28, 20, 12, 4,
	62, 54, 46, 38, 30, 22, 14, 6,
	64, 56, 48, 40, 32, 24, 16, 8,
	57, 49, 41, 33, 25, 17,  9, 1,
	59, 51, 43, 35, 27, 19, 11, 3,
	61, 53, 45, 37, 29, 21, 13, 5,
	63, 55, 47, 39, 31, 23, 15, 7 };
static const BYTE fp_table[64] = {
	40,  8, 48, 16, 56, 24, 64, 32,
	39,  7, 47, 15, 55, 23, 63, 31,
	38,  6, 46, 14, 54, 22, 62, 30,
	37,  5, 45, 13, 53, 21, 61, 29,
	36,  4, 44, 12, 52, 20, 60, 28,
	35,  3, 43, 11, 51, 19, 59, 27,
	34,  2, 42, 10, 50, 18, 58, 26,
	33,  1, 41,  9, 49, 17, 57, 25 };
static const BYTE shift[16] = { 1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1 };
static const BYTE init_table[56] = {
	57, 49, 41, 33, 25, 17,  9,
	 1, 58, 50, 42, 34, 26, 18,
	10,  2, 59, 51, 43, 35, 27,
	19, 11,  3, 60, 52, 44, 36,
	63, 55, 47, 39, 31, 23, 15,
	 7, 62, 54, 46, 38, 30, 22,
	14,  6, 61, 53, 45, 37, 29,
	21, 13,  5, 28, 20, 12,  4 };
static const BYTE replace_table[48] = {
	14, 17, 11, 24,  1,  5,
	 3, 28, 15,  6, 21, 10,
	23, 19, 12,  4, 26,  8,
	16,  7, 27, 20, 13,  2,
	41, 52, 31, 37, 47, 55,
	30, 40, 51, 45, 33, 48,
	44, 49, 39, 56, 34, 53,
	46, 42, 50, 36, 29, 32 };
static const BYTE expand_table[48] = {
	32,  1,  2,  3,  4,  5,
	 4,  5,  6,  7,  8,  9,
	 8,  9, 10, 11, 12, 13,
	12, 13, 14, 15, 16, 17,
	16, 17, 18, 19, 20, 21,
	20, 21, 22, 23, 24, 25,
	24, 25, 26, 27, 28, 29,
	28, 29, 30, 31, 32,  1 };
static const BYTE tp_table[32] = {
	16,  7, 20, 21,
	29, 12, 28, 17,
	 1, 15, 23, 26,
	 5, 18, 31, 10,
	 2,  8, 24, 14,
	32, 27,  3,  9,
	19, 13, 30,  6,
	22, 11,  4, 25 };
static const BYTE s_table[8][64] = {
	{
		14,  0,  4, 15, 13,  7,  1,  4,  2, 14, 15,  2, 11, 13,  8,  1,
		 3, 10, 10,  6,  6, 12, 12, 11,  5,  9,  9,  5,  0,  3,  7,  8,
		 4, 15,  1, 12, 14,  8,  8,  2, 13,  4,  6,  9,  2,  1, 11,  7,
		15,  5, 12, 11,  9,  3,  7, 14,  3, 10, 10,  0,  5,  6,  0, 13
	}, {
		15,  3,  1, 13,  8,  4, 14,  7,  6, 15, 11,  2,  3,  8,  4, 14,
		 9, 12,  7,  0,  2,  1, 13, 10, 12,  6,  0,  9,  5, 11, 10,  5,
		 0, 13, 14,  8,  7, 10, 11,  1, 10,  3,  4, 15, 13,  4,  1,  2,
		 5, 11,  8,  6, 12,  7,  6, 12,  9,  0,  3,  5,  2, 14, 15,  9
	}, {
		10, 13,  0,  7,  9,  0, 14,  9,  6,  3,  3,  4, 15,  6,  5, 10,
		 1,  2, 13,  8, 12,  5,  7, 14, 11, 12,  4, 11,  2, 15,  8,  1,
		13,  1,  6, 10,  4, 13,  9,  0,  8,  6, 15,  9,  3,  8,  0,  7,
		11,  4,  1, 15,  2, 14, 12,  3,  5, 11, 10,  5, 14,  2,  7, 12
	}, {
		 7, 13, 13,  8, 14, 11,  3,  5,  0,  6,  6, 15,  9,  0, 10,  3,
		 1,  4,  2,  7,  8,  2,  5, 12, 11,  1, 12, 10,  4, 14, 15,  9,
		10,  3,  6, 15,  9,  0,  0,  6, 12, 10, 11,  1,  7, 13, 13,  8,
		15,  9,  1,  4,  3,  5, 14, 11,  5, 12,  2,  7,  8,  2,  4, 14
	}, {
		 2, 14, 12, 11,  4,  2,  1, 12,  7,  4, 10,  7, 11, 13,  6,  1,
		 8,  5,  5,  0,  3, 15, 15, 10, 13,  3,  0,  9, 14,  8,  9,  6,
		 4, 11,  2,  8,  1, 12, 11,  7, 10,  1, 13, 14,  7,  2,  8, 13,
		15,  6,  9, 15, 12,  0,  5,  9,  6, 10,  3,  4,  0,  5, 14,  3
	}, {
		12, 10,  1, 15, 10,  4, 15,  2,  9,  7,  2, 12,  6,  9,  8,  5,
		 0,  6, 13,  1,  3, 13,  4, 14, 14,  0,  7, 11,  5,  3, 11,  8,
		 9,  4, 14,  3, 15,  2,  5, 12,  2,  9,  8,  5, 12, 15,  3, 10,
		 7, 11,  0, 14,  4,  1, 10,  7,  1,  6, 13,  0, 11,  8,  6, 13
	}, {
		 4, 13, 11,  0,  2, 11, 14,  7, 15,  4,  0,  9,  8,  1, 13, 10,
		 3, 14, 12,  3,  9,  5,  7, 12,  5,  2, 10, 15,  6,  8,  1,  6,
		 1,  6,  4, 11, 11, 13, 13,  8, 12,  1,  3,  4,  7, 10, 14,  7,
		10,  9, 15,  5,  6,  0,  8, 15,  0, 14,  5,  2,  9,  3,  2, 12,
	}, {
		13,  1,  2, 15,  8, 13,  4,  8,  6, 10, 15,  3, 11,  7,  1,  4,
		10, 12,  9,  5,  3,  6, 14, 11,  5,  0,  0, 14, 12,  9,  7,  2,
		 7,  2, 11,  1,  4, 14,  1,  7,  9,  4, 12, 10, 14,  8,  2, 13,
		 0, 15,  6, 12, 10,  9, 13,  0, 15,  3,  3,  5,  5,  6,  8, 11,
	}
};

inline void CDec::MakeKey(BIT64 key)
{
	BYTE	i,j;
	BIT64	bit;


	bit.dw[0] = bit.dw[1] = 0;
	for(i=0;i<28;i++)
	{
		BYTE	temp = init_table[i] -1;

		if(key.b[temp>>3] & mask[temp&0x7])
			bit.b[i>>3] &= mask[i&0x7];

		temp = init_table[i+28] -1;
		if(key.b[temp>>3] & mask[temp&0x7])
			bit.b[i>>3 + 4] &= mask[i&0x7];
	}


	for(i=0;i<16;i++)
	{
		for(j=0;j<shift[i];j++)
		{
			BYTE temp = (BYTE)(bit.b[0] & 0x80);

			bit.b[0] <<= 1;
			if(bit.b[1] & 0x80)
				bit.b[0] += 1;
			bit.b[1] <<= 1;
			if(bit.b[2] & 0x80)
				bit.b[1] += 1;
			bit.b[2] <<= 1;
			if(bit.b[3] & 0x80)
				bit.b[2] += 1;
			bit.b[3] <<= 1;
			if(temp)
				bit.b[3] += 0x10;

			temp = (BYTE)(bit.b[4] & 0x80);

			bit.b[4] <<= 1;
			if(bit.b[5] & 0x80)
				bit.b[4] += 1;
			bit.b[5] <<= 1;
			if(bit.b[6] & 0x80)
				bit.b[5] += 1;
			bit.b[6] <<= 1;
			if(bit.b[7] & 0x80)
				bit.b[6] += 1;
			bit.b[7] <<= 1;
			if(temp)
				bit.b[7] += 0x10;
		}


		m_key[i].dw[0] = 0;
		m_key[i].dw[1] = 0;
		for(j=0;j<48;j++)
		{
			BYTE	temp = replace_table[j] -1;
			if(temp < 28)
			{
				if(bit.b[temp>>3] & mask[temp&0x7])
					m_key[i].b[j/6] |= mask[j%6];
			} else {
				temp = temp - 28;
				if(bit.b[temp>>3 + 4] & mask[temp&0x7])
					m_key[i].b[j/6] |= mask[j%6];
			}
		}
	}
}

inline void CDec::RoundFunction(BIT64 key, BIT64 *text)
{
	BIT64	a;
	BIT64	t;
	BYTE	i,j;


	t.dw[0] = t.dw[1] = 0;
	for(i=0;i<48;i++)
	{
		j = expand_table[i] + 31;
		if(text->b[j>>3] & mask[j&0x7])
		{
			t.b[i/6] |= mask[i%6];
		}
	}


	for(i=0;i<8;i++)
	{
		t.b[i] ^= key.b[i];
	}


	a.dw[0] = a.dw[1] = 0;
	for(i=0;i<8;i++)
	{
		if(i%2 == 0)
			a.b[i/2] = s_table[i][t.b[i]>>2] << 4;
		else
			a.b[i/2] += s_table[i][t.b[i]>>2];
	}


	t.dw[0] = t.dw[1] = 0;
	BYTE	temp;
	for(i=0;i<32;i++)
	{
		temp = tp_table[i] -1;
		if(a.b[temp>>3] & mask[temp&0x7])
			t.b[i>>3] |= mask[i&0x7];
	}

	text->dw[0] ^= t.dw[0];
}

inline int CDec::Encrypt(int round, BIT64 *pKey, BIT64 in, BIT64 *pOut)
{
	BYTE	i;
	DWORD	t;


	pOut->dw[0] = in.dw[0];
	pOut->dw[1] = in.dw[1];


	IP(pOut);

	for(i=0;i<round;i++)
	{

		RoundFunction(pKey[i],pOut);

		t = pOut->dw[0];
		pOut->dw[0] = pOut->dw[1];
		pOut->dw[1] = t;
	}


	t = pOut->dw[0];
	pOut->dw[0] = pOut->dw[1];
	pOut->dw[1] = t;


	FP(pOut);

	return true;
}

inline int CDec::Decrypt(int round, BIT64 *pKey, BIT64 in, BIT64 *pOut)
{
	BYTE	i;
	DWORD	t;


	pOut->dw[0] = in.dw[0];
	pOut->dw[1] = in.dw[1];


	IP(pOut);

	for(i=0;i<round;i++)
	{

		RoundFunction(pKey[15-i],pOut);

		t = pOut->dw[0];
		pOut->dw[0] = pOut->dw[1];
		pOut->dw[1] = t;
	}


	t = pOut->dw[0];
	pOut->dw[0] = pOut->dw[1];
	pOut->dw[1] = t;


	FP(pOut);
	return true;
}

CDec::CDec()
{
}

CDec::CDec(char* seed)
{
	MakeKey(seed);
}

CDec::~CDec()
{
}

void CDec::MakeKey(char* seed)
{
	BIT64 key;

	int i;
	for(i=0;i<8;i++)
	{
		key.b[i] = seed[i];
	}

	MakeKey(key);
}

int CDec::DataEncrypt(char *pIn,int inSize,char *pOut,int *outSize,int mod)
{
	int i, j, k;

	inSize = ((int)(inSize + 7) / 8) * 8;
	*outSize = ((int)(inSize + 7) / 8) * 8;

	if (mod < 3) {
		mod = 1;
	} else if (mod < 5) {
		mod += 1;
	} else if (mod < 7) {
		mod += 9;
	} else {
		mod += 15;
	}

	BIT64 in,out;
	i = 0;
	j = 0;
	k = 0;
	while (1) {
		if (i < 20 || (i % mod) == 0) {
			for (j = 0; j < 8; j++) {
				in.b[j] = pIn[i * 8 + j];
			}
			Encrypt(1, m_key, in, &out);
			for (j = 0; j < 8; j++) {
				pOut[i * 8 + j] = out.b[j];
			}
		} else {
			if (k != 7) {
				memcpy(pOut + i * 8, pIn + i * 8, 8);
			} else {
				BYTE out8;
				memcpy(pOut + 3 + i * 8, pIn + i * 8, 2);
				memcpy(pOut + i * 8, pIn + 3 + i * 8, 3);
				*(pOut + 5 + i * 8) = *(pIn + 6 + i * 8);
				*(pOut + 6 + i * 8) = *(pIn + 2 + i * 8);

				out8 = *(pIn + 7 + i * 8);
				switch(out8) {
				case 0:
					out8 = (char)43;
					break;
				case 43:
					out8 = (char)0;
					break;
				case 185:
					out8 = (char)192;
					break;
				case 192:
					out8 = (char)185;
					break;
				case 254:
					out8 = (char)235;
					break;
				case 235:
					out8 = (char)254;
					break;
				case 108:
					out8 = (char)128;
					break;
				case 128:
					out8 = (char)108;
					break;
				case 1:
					out8 = (char)104;
					break;
				case 104:
					out8 = (char)1;
					break;
				case 119:
					out8 = (char)72;
					break;
				case 72:
					out8 = (char)119;
					break;
				case 96:
					out8 = (char)255;
					break;
				case 255:
					out8 = (char)96;
					break;
				}

				*(pOut + 7 + i * 8) = (char)out8;
				k = 0;
			}
			k++;
		}
		if (((i + 1) * 8) >= inSize)
			break;
		i++;
	}
	return true;
}

int CDec::DataDecrypt(char *pIn,int inSize,char *pOut,int *outSize,int mod)
{

	int i, j, k;




	if (pIn == NULL)
		return false;
	if (pOut == NULL)
		return false;
	if (outSize == NULL)
		return false;

	*outSize = inSize;

	if (mod < 3) {
		mod = 1;
	} else if (mod < 5) {
		mod += 1;
	} else if (mod < 7) {
		mod += 9;
	} else {
		mod += 15;
	}

	BIT64 in, out;
	i = 0;
	j = 0;
	k = 0;
	while (1) {
		if (i < 20 || (i % mod) == 0 ) {
			for (j = 0; j < 8; j++)
			{
				in.b[j] = pIn[i * 8 + j];
			}
			Decrypt(1, m_key, in, &out);
			for (j = 0; j < 8; j++) {
				pOut[i * 8 + j] = out.b[j];
			}
		} else {
			if (k != 7) {
				memcpy(pOut + i * 8, pIn + i * 8, 8);
			} else {
				BYTE out8;
				memcpy(pOut + i * 8, pIn + 3 + i * 8, 2);
				memcpy(pOut + 3 + i * 8, pIn + i * 8, 3);
				*(pOut + 2 + i * 8) = *(pIn + 6 + i * 8);
				*(pOut + 6 + i * 8) = *(pIn + 5 + i * 8);

				out8 = (BYTE)*(pIn + 7 + i * 8);
				switch(out8) {
				case 0:
					out8 = (char)43;
					break;
				case 43:
					out8 = (char)0;
					break;
				case 185:
					out8 = (char)192;
					break;
				case 192:
					out8 = (char)185;
					break;
				case 254:
					out8 = (char)235;
					break;
				case 235:
					out8 = (char)254;
					break;
				case 108:
					out8 = (char)128;
					break;
				case 128:
					out8 = (char)108;
					break;
				case 1:
					out8 = (char)104;
					break;
				case 104:
					out8 = (char)1;
					break;
				case 119:
					out8 = (char)72;
					break;
				case 72:
					out8 = (char)119;
					break;
				case 96:
					out8 = (char)255;
					break;
				case 255:
					out8 = (char)96;
					break;
				}

				*(pOut + 7 + i * 8) = (char)out8;
				k = 0;
			}
			k++;
		}
		if (((i + 1) * 8) >= inSize)
			break;
		i++;
	}

	return true;
}

int CDec::DataDecryptHeader(char *pIn,int inSize,char *pOut,int *outSize)
{

	int i, j, k;




	*outSize = inSize;

	memcpy(pOut, pIn, *outSize);
	BIT64 in,out;
	i = 0;
	j = 0;
	k = 0;
	while(i < 20) {
		for (j = 0; j < 8; j++) {
			in.b[j] = pIn[i * 8 + j];
		}

		Decrypt(1, m_key, in, &out);
		for(j = 0; j < 8; j++) {
			pOut[i * 8 + j] = out.b[j];
		}

		if (((i + 1) * 8) >= inSize)
			break;

		i++;
	}

	return true;
}

void CDec::IP(BIT64 *text)
{
	BYTE    i;
	BIT64   t;

	t.dw[0] = t.dw[1] = 0;
	for(i=0;i<64;i++)
	{
			BYTE    temp = ip_table[i] -1;
			if((text->b[temp>>3] & mask[temp&0x7]) != 0)
					t.b[i>>3] |= mask[i&0x7];
	}
	text->dw[0] = t.dw[0];
	text->dw[1] = t.dw[1];
}

void CDec::FP(BIT64 *text)
{
	BYTE    i;
	BIT64   t;

	t.dw[0] = t.dw[1] = 0;
	for(i=0;i<64;i++)
	{
			BYTE    temp = fp_table[i] -1;
			if(text->b[temp>>3] & mask[temp&0x7])
					t.b[i>>3] |= mask[i&0x7];
	}
	text->dw[0] = t.dw[0];
	text->dw[1] = t.dw[1];
}
