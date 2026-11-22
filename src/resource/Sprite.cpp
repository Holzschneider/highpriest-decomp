#include "common.h"
#include "Sprite.h"
#include "../base/File.h"
#include "../3dlib/D3dutil.h"
#include "../3dlib/Device.h"
#include "../3dlib/Texture.h"
#include "../3dlib/Renderer.h"

#include <string>
#include <vector>
#include <stdio.h>

typedef std::vector<SprImg *>::iterator spr_img_iter;








static CSprRes CSprResInstance(Exemplar(), "spr", "sprite\\");
CSprRes :: CSprRes()
: CRes()
{
	m_count = 0;
}

void    CSprRes :: Reset()
{
	for (int j = 0; j < 2; j++) {
		spr_img_iter	it = m_sprites[j].begin();

		while (it != m_sprites[j].end()) {
			if (j == 0) {
				delete [] (*it)->m_8bitImage;
			}
			else {
				SAFE_DELETE((*it)->tex);
			}
			delete	(*it);
			it++;
		}

		m_sprites[j].erase(m_sprites[j].begin(), m_sprites[j].end());
	}
}

bool	CSprRes :: Load(const char *fName)
{
	CFile		fp;
	struct
	{
		WORD	id, ver;
		WORD	count;
	}
	FileHeader;

	struct
	{
		WORD	cx, cy;
	}
	SprChunk;
	WORD	compressedSize;
	WORD	tgaCount;

	if (!fp.Open(fName, CFile::modeRead))
	{
		ErrorMsg((std::string("Spr :: Cannot find File : ") + std::string(fName)).c_str());
		return	false;
	}

	fp.Read((void *)&FileHeader, sizeof(FileHeader));
	if (FileHeader.id != (int('P') * 256 + 'S'))
	{
		ErrorMsg("Illegal File Format");
		ErrorMsg(fName);
		return false;
	}
	if (FileHeader.ver > 0x0201)
	{
		ErrorMsg("Unsupported Version. Content may be corrupted");
		return false;
	}


	m_count = FileHeader.count;

	if (FileHeader.ver >= 0x0101)
	{
		PALETTEENTRY paletteEntry[256];
		fp.Seek(-1024, CFile::end);
		fp.Read(paletteEntry, sizeof(PALETTEENTRY) * 256);
		fp.Seek(sizeof(FileHeader), CFile::begin);

		g_3dDevice.ConvertPalette(m_pal, paletteEntry, 256);
	}

	if (FileHeader.ver >= 0x0200)
		fp.Read((void *)&tgaCount, sizeof(WORD));

	DWORD *		image;
	DWORD *		dest;

	for (int i = 0; i < m_count; i++)
	{
		fp.Read((void *)&SprChunk, sizeof(SprChunk));

		SprImg *	newSpr = new SprImg;
		newSpr->tex = NULL;
		newSpr->width = SprChunk.cx;
		newSpr->height = SprChunk.cy;
		newSpr->isHalfW = 0;
		newSpr->isHalfH = 0;

		if (!g_isXHalfSpr && !g_isYHalfSpr)
		{
			if(FileHeader.ver >= 0x0201)
			{
				fp.Read(&compressedSize, sizeof(WORD));
				newSpr->m_8bitImage = new BYTE[compressedSize];
				fp.Read(newSpr->m_8bitImage, compressedSize);
			}
			else
			{
				BYTE *Image = new BYTE[newSpr->width * newSpr->height];
				fp.Read(Image, newSpr->width * newSpr->height) ;

				newSpr->m_8bitImage = ZeroCompression(Image, newSpr->width, newSpr->height, compressedSize);
				delete [] Image;
			}
		}
		else
		{
			if(FileHeader.ver >= 0x0201)
			{
				fp.Read(&compressedSize, sizeof(WORD));
				BYTE * compressedImage = new BYTE[compressedSize];
				fp.Read(compressedImage, compressedSize);
				BYTE * decompressedImage = ZeroDecompression(compressedImage, SprChunk.cx, SprChunk.cy);
				delete [] compressedImage;
				BYTE * halfImage = HalfImage(decompressedImage, SprChunk.cx, SprChunk.cy, g_isXHalfSpr, g_isYHalfSpr);
				delete [] decompressedImage;

				newSpr->m_8bitImage = ZeroCompression(halfImage, SprChunk.cx/(1+g_isXHalfSpr), SprChunk.cy/(1+g_isYHalfSpr), compressedSize);

				delete [] halfImage;
				if (g_isXHalfSpr)
				{
					newSpr->width /= 2;
					newSpr->isHalfW = 1;
				}
				if (g_isYHalfSpr)
				{
					newSpr->height /= 2;
					newSpr->isHalfH = 1;
				}
			}
			else
			{
				BYTE * decompressedImage = new BYTE[newSpr->width * newSpr->height];
				fp.Read(decompressedImage, newSpr->width * newSpr->height);
				BYTE * halfImage = HalfImage(decompressedImage, SprChunk.cx, SprChunk.cy, g_isXHalfSpr, g_isYHalfSpr);
				delete [] decompressedImage;
				newSpr->m_8bitImage = ZeroCompression(halfImage, SprChunk.cx/(1+g_isXHalfSpr), SprChunk.cy/(1+g_isYHalfSpr), compressedSize);
				delete [] halfImage;
				if (g_isXHalfSpr)
				{
					newSpr->width /= 2;
					newSpr->isHalfW = 1;
				}
				if (g_isYHalfSpr)
				{
					newSpr->height /= 2;
					newSpr->isHalfH = 1;
				}
			}
		}

		m_sprites[0].push_back(newSpr);
	}

	if (FileHeader.ver >= 0x0200)
	{
		DWORD	rgba;
		int		r, g, b, a;

		for(int m = 0; m < tgaCount; m++)
		{
			fp.Read((void *)&SprChunk, sizeof(SprChunk));
			SprImg *	newSpr = new SprImg;
			newSpr->width = SprChunk.cx;
			newSpr->height = SprChunk.cy;
			newSpr->isHalfW = 0;
			newSpr->isHalfH = 0;

			image = new DWORD[newSpr->width * newSpr->height];

			for (int y = newSpr->height - 1; y >= 0; y--)
			{
				dest = image + newSpr->width * y;

				for (int x = 0; x < newSpr->width; x++)
				{
					fp.Read(&rgba, sizeof(DWORD));

					r = rgba >> 24;
					g = (rgba & 0x00FFFFFF) >> 16;
					b = (rgba & 0x0000FFFF) >> 8;
					a = (rgba & 0x000000FF);

					*dest = PACK32BIT8888(a, r, g, b);
					dest++;
				}
			}

			newSpr->tex = g_texMgr.CreateTexture( g_isXHalfSpr ? newSpr->width / 2 : newSpr->width,
													g_isYHalfSpr ? newSpr->height / 2 : newSpr->height,
													PF_A4R4G4B4,
													NULL	);
			newSpr->tex->Update(0, 0, newSpr->width, newSpr->height, image, false, 0);
			delete [] image;

			if (g_isXHalfSpr)
			{
				newSpr->width /= 2;
				newSpr->isHalfW = 1;
			}

			if (g_isYHalfSpr)
			{
				newSpr->height /= 2;
				newSpr->isHalfH = 1;
			}

			m_sprites[1].push_back(newSpr);
	}
	}

	fp.Close();

	return  true;
}

unsigned char* CSprRes::ZeroCompression(unsigned char* Image, int x, int y, unsigned short& Size)
{
	unsigned short cnt = 0;
	WORD ZeroCnt = 0;
	BOOL StartCnt = FALSE;
	std::vector<BYTE> Data;
	BYTE *CompressedImage;

	for(int yy = 0; yy < y; yy++) {
		for(int xx = 0; xx < x; xx++) {
			if(Image[cnt] == 0 && !StartCnt) {
				StartCnt = TRUE;
				Data.push_back((BYTE)0);
				ZeroCnt++;
			} else if(Image[cnt] == 0 && StartCnt) {
				ZeroCnt++;

				if (ZeroCnt >= 255) {
					Data.push_back((BYTE)ZeroCnt);
					StartCnt = FALSE;
					ZeroCnt = 0;
				}
			} else if(Image[cnt] != 0 && StartCnt) {
				Data.push_back((BYTE)ZeroCnt);
				Data.push_back(Image[cnt]);
				StartCnt = FALSE;
				ZeroCnt = 0;
			} else {
				ZeroCnt = 0;
				Data.push_back(Image[cnt]);
				StartCnt = FALSE;
			}
			cnt++;
		}
	}
	if (ZeroCnt)
		Data.push_back(ZeroCnt);

	Size = Data.size();
	CompressedImage = new BYTE[Size];
	cnt = 0;
	while(cnt < Size) {
		CompressedImage[cnt] = Data[cnt];
		cnt++;
	}

	return CompressedImage;
}

unsigned char* CSprRes :: ZeroDecompression(unsigned char* compressedImage, int width, int height)
{
	BYTE * image = new BYTE[height * width];
	BYTE * surface;
	WORD	keyCnt = 0;
	int		color;
	BYTE *	src = compressedImage;

	for (int i = 0; i < height; i++)
	{
		surface = (image + (width*i));

		if (surface)
		{
			for (int j = 0; j < width; j++)
			{
				if (keyCnt == 0) {
					color = *src++;
					if (color == 0)
						keyCnt = *src++;
				}

				if (keyCnt) {
					*surface++ = 0;
					keyCnt--;
				} else {
					*surface++ = color;
				}
			}
		}
	}
	return image;
}

unsigned char* CSprRes :: HalfImage(unsigned char* Image, int x, int y, int isXHalf, int isYHalf)
{
	int hx = x;
	if (isXHalf)
		hx = x/2;
	int hy = y;
	if (isYHalf)
		hy = y/2;
	BYTE * halfImage = new BYTE[hx * hy];
	BYTE * surface;
	BYTE *	src = Image;
	for (int i=0; i<hy; ++i) {
		surface = (halfImage + (hx*i));
		for (int j=0; j<hx; ++j) {
			*surface++ = Image[i*(isYHalf+1)*x + j*(isXHalf+1)];
		}
	}
	return halfImage;
}
