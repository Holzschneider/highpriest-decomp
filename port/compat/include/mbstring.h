#pragma once
#include <stddef.h>
unsigned char* _mbschr(const unsigned char*, unsigned); unsigned char* _mbsrchr(const unsigned char*, unsigned);
unsigned char* _mbsstr(const unsigned char*, const unsigned char*); int _mbsicmp(const unsigned char*, const unsigned char*);
unsigned char* _mbsinc(const unsigned char*); int _ismbblead(unsigned); int _mbsbtype(const unsigned char*, size_t);
unsigned char* _mbsnbcpy(unsigned char*, const unsigned char*, size_t); unsigned char* _mbsnbcat(unsigned char*, const unsigned char*, size_t);
int _mbsnbicmp(const unsigned char*, const unsigned char*, size_t);
