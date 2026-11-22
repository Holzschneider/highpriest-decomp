#ifndef HIGHPRIEST_RESOURCE_SPRITE_H
#define HIGHPRIEST_RESOURCE_SPRITE_H

#include "../common.h"

#include "../base/Res.h"
#include "Action.h"
#include "PaletteRes.h"


class CTexture;

class CSprRes;

struct SprImg {
	short width, height;
	short isHalfW, isHalfH;

	CTexture* tex;
	BYTE* m_8bitImage;
};

class CSprRes : public CRes {
public:
	DECLARE_RESOURCE(CSprRes)
	CSprRes();
	virtual bool Load(const char* fName);
	virtual void Reset();
	void Unload() { Reset(); }
	unsigned char* ZeroCompression(unsigned char* Image, int x, int y, unsigned short& Size);
	unsigned char* ZeroDecompression(unsigned char* compressedImage, int width, int height);
	unsigned char* HalfImage(unsigned char* Image, int x, int y, int isXHalf, int isYHalf);
	SYSTEMPALETTE m_pal[256];
	std::vector<SprImg *> m_sprites[2];
	int m_count;
};
extern int g_isXHalfSpr;
extern int g_isYHalfSpr;

#endif
