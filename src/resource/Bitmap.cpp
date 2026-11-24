#include <string.h>
#include "common.h"
#include "Bitmap.h"
#include "../base/File.h"
#include "../3dlib/Device.h"
#include "../ijl.h"
#if defined(BF_PORT)
#include "bf_port.h"
#endif













static CBitmapRes CBitmapResInstance(Exemplar(), "bmp", "texture\\");
static CBitmapRes CBitmapResInstance2(Exemplar(), "tga", "texture\\");
static CBitmapRes CBitmapResInstance3(Exemplar(), "jpg", "texture\\");
CBitmapRes :: CBitmapRes()
{
	m_isAlpha = FALSE;
	m_data = NULL;
}

bool	CBitmapRes :: Load(const char *fName)
{
	CFile fp;

	if (fName == NULL)
		return false;

	if(!fp.Open(fName, CFile::modeRead)) return false;

	if(LoadFromBuffer(fName, fp.GetBuf(), fp.GetLength()))
	{
		fp.Close();
		return true;
	}
	else
	{
		fp.Close();
		return false;
	}
}

bool	CBitmapRes :: LoadFromBuffer(const char *fName, const BYTE* buffer, int size)
{
	TCHAR *		strExtension;


	if ((strExtension = (TCHAR*)strrchr(fName, '.')) == NULL) return false;

	if (!lstrcmpi(strExtension, ".bmp"))
		return LoadBMPData(buffer, size);
	else if (!lstrcmpi(strExtension, ".tga"))
		return LoadTGAData(buffer, size);
	else if (!lstrcmpi(strExtension, ".jpg"))
		return LoadJPGData(buffer, size);
	else {
		ErrorMsg("Unsupport fileformat!");
		return false;
	}
}

void    CBitmapRes :: Reset()
{
	if (m_data) {
		delete [] m_data;
	}
	m_width = 0;
	m_height = 0;
	m_data = NULL;
}

DWORD	CBitmapRes :: GetColor(int x, int y)
{
	sAssert(x>=0 && x<m_width);
	sAssert(y>=0 && y<m_height);
	if (x<0 || x>=m_width || y<0 || y>=m_height)
		return 0x00ff0000;
	return m_data[x+y*m_width];
}

bool	CBitmapRes :: SetAlphaWithBMP(const char *fName)
{
	CBitmapRes alphaBitmap;
	if (!alphaBitmap.Load(fName)) {
		ErrorMsg("Error reading alpha bitmap");
		alphaBitmap.Reset();
		return false;
	}
	sAssert(m_data);
	if (m_data==NULL) {
		return false;
	}
	sAssert(m_height==alphaBitmap.m_height);
	sAssert(m_width==alphaBitmap.m_width);

	for (int i=0; i<m_height; ++i) {
		for (int j=0; j<m_width; ++j) {
			DWORD color = m_data[j+i*m_width];
			DWORD aColor = alphaBitmap.GetColor(j, i);
			int R = (aColor|0x00ff0000) >> 16;
			int G = (aColor|0x0000ff00) >> 8;
			int B = (aColor|0x000000ff);
			int alpha = (R+G+B)/3;
			m_data[j+i*m_width] = (alpha<<24) + (color&0x00ffffff);
		}
	}

	alphaBitmap.Reset();
	return true;
}

bool	CBitmapRes :: ChangeToGrayScaleBMP ( int blackLimit, int whiteLimit )
{
	if (m_data==NULL)
		return false ;

	DWORD mdata ;
	int a, r, g, b, rgb ;

	for ( int i = 0 ; i < m_height ; ++i )
	{
		for ( int j = 0 ; j < m_width ; ++j )
		{
			mdata = *(m_data + m_width * i + j) ;

			a = mdata>>24 ;
			r = (mdata>>16)%256 ;
			g = (mdata>>8)%256 ;
			b = mdata%256 ;

			if ( r == 255 && b == 255 && g == 0 )
			{
			}
			else
			{
				rgb = (r+g+b) / 3 ;

				if ( rgb < blackLimit )
					rgb = blackLimit ;

				if ( rgb > whiteLimit )
					rgb = 255 ;

				r = rgb ;
				g = rgb ;
				b = rgb ;
			}

			*(m_data + m_width * i + j) = (a<<24) + (r<<16) + (g<<8) + b ;
		}
	}

	return true ;
}

void	CBitmapRes :: CreateNull(int x, int y)
{
	sAssert(m_data == NULL);

	m_isAlpha = FALSE;
	m_width = x;
	m_height = y;
	m_data = new DWORD[x * y];
	memset(m_data, 0xFF, sizeof(DWORD) * x * y);
}







bool	CBitmapRes :: LoadBMPData(const BYTE* bitmap, int size)
{
#define _EXTEND_BMP
#ifdef	_EXTEND_BMP
	BITMAPFILEHEADER	FileHdr;
	BITMAPINFO			InfoHdr;
	BYTE *				pColorMap = NULL;
	int					i, j, readType = 0;
	const BYTE*			bitmapiter = bitmap;
	BYTE	headText[13];

	Reset();

	memcpy(&FileHdr, bitmapiter, sizeof(BITMAPFILEHEADER));
	memcpy(headText, bitmapiter, 12);
	headText[12] = NULL;

	if (!strcmp((char*)headText, "\xC3\xD1\xBE\xCB\xC0\xCC\xBB\xA1\xC5\xE4\xB4\xCF"))
		readType = 1;
	else if (FileHdr.bfType != 'B' + 'M' * 256) return false;

	if (readType)
	{
		DWORD tmp;
		BYTE tmpByte;
		bitmapiter += 12;

		memcpy(&tmp, bitmapiter, sizeof(DWORD));
		bitmapiter += sizeof(DWORD);
		tmp = tmp >> 1;
		i = 0x1000000;
		j = 0x9293;
		tmp -= i + (j << 8);
		InfoHdr.bmiHeader.biWidth = (tmp >> 16) & 0xffff;
		InfoHdr.bmiHeader.biHeight = tmp & 0xffff;
		InfoHdr.bmiHeader.biHeight--;
		memcpy(&tmpByte, bitmapiter, sizeof(BYTE));
		bitmapiter += sizeof(BYTE);
		switch (tmpByte) {
			case	0:
				InfoHdr.bmiHeader.biBitCount = 24;
				break;
			case	1:
				InfoHdr.bmiHeader.biBitCount = 8;
				break;
			default:
				InfoHdr.bmiHeader.biBitCount = 1;
				break;
		}

		if (InfoHdr.bmiHeader.biBitCount != 24 && InfoHdr.bmiHeader.biBitCount != 8)
		{
			ErrorMsg("Only 16M or 8bit color format supported");
			return	false;
		}
		DWORD	pitch = (InfoHdr.bmiHeader.biWidth + 3) & ~3;
		m_width = InfoHdr.bmiHeader.biWidth;
		m_height = InfoHdr.bmiHeader.biHeight;

		pColorMap = new BYTE[m_width * m_height * 3];
		m_data = new DWORD[m_width * m_height];

		if (InfoHdr.bmiHeader.biBitCount == 8) {
			RGBQUAD	colorMap[256];
			BYTE	*bufPtr, gbColor[512], rColor[512];
			BYTE	tmpRow[4096];
			int		k;

			memcpy(&tmp, bitmapiter, sizeof(DWORD));
			bitmapiter += sizeof(DWORD);
			InfoHdr.bmiHeader.biClrUsed = tmp;
			if (InfoHdr.bmiHeader.biClrUsed)
			{
				memcpy(gbColor, bitmapiter, InfoHdr.bmiHeader.biClrUsed * 2);
				bitmapiter += InfoHdr.bmiHeader.biClrUsed * 2;
				memcpy(rColor, bitmapiter, InfoHdr.bmiHeader.biClrUsed * 2);
				bitmapiter += InfoHdr.bmiHeader.biClrUsed * 2;
				i = 0;j = 0;
				while (i < InfoHdr.bmiHeader.biClrUsed) {
					colorMap[i].rgbGreen = gbColor[j];
					colorMap[i].rgbReserved = rColor[j];
					j++;
					colorMap[i].rgbBlue = gbColor[j];
					colorMap[i].rgbRed = rColor[j];
					j++;
					i++;
				}
			}
			else
			{
				memcpy(gbColor, bitmapiter, 512);
				bitmapiter += 512;
				memcpy(rColor, bitmapiter, 512);
				bitmapiter += 512;

				i = 0;j = 0;
				while (i < 256) {
					colorMap[i].rgbGreen = gbColor[j];
					colorMap[i].rgbReserved = rColor[j];
					j++;
					colorMap[i].rgbBlue = gbColor[j];
					colorMap[i].rgbRed = rColor[j];
					j++;
					i++;
				}
			}

			for (i = m_height - 1; i >= 0; i--) {
				int	padded = int(m_width + 3) & (~3);

				memcpy(tmpRow, bitmapiter, padded);
				bitmapiter += padded;

				bufPtr = pColorMap + m_width * i * 3;

				for (j = 0; j < m_width; j++) {
					k = tmpRow[j];
					*bufPtr		  = colorMap[k].rgbBlue;
					*(bufPtr + 1) = colorMap[k].rgbGreen;
					*(bufPtr + 2) = colorMap[k].rgbRed;
					bufPtr += 3;
				}
			}
		}
		else if (InfoHdr.bmiHeader.biBitCount == 24) {
			BYTE *tmpData1 = new BYTE[m_width * m_height * 3];
			BYTE *tmpData2 = new BYTE[m_width * m_height * 2];

			i = m_height - 1; j = 0;
			BYTE	*bufPtr = pColorMap, *tmpbuf1 = tmpData1, *tmpbuf2 = tmpData2;
			while (i >= 0) {
				memcpy(tmpbuf1, bitmapiter, m_width * 2);
				tmpbuf1 += m_width * 2;
				bitmapiter += m_width * 2;
				i--;
			}

			i = m_height - 1;
			while (i >= 0) {
				memcpy(tmpbuf2, bitmapiter, m_width);
				tmpbuf2 += m_width;
				bitmapiter += m_width;
				i--;
			}

			readType = 0;
			i = 0;
			while (i < m_height) {
				j = 0;
				while (j < m_width) {
					*bufPtr = tmpData2[j + i * m_width];
					*(bufPtr + 1) = tmpData1[readType++];
					*(bufPtr + 2) = tmpData1[readType++];
					bufPtr += 3;
			 		j++;
				}
				i++;
			}

			delete [] tmpData1;
			delete [] tmpData2;
		}
	} else {
		bitmapiter += sizeof(BITMAPFILEHEADER);

		memcpy(&InfoHdr, bitmapiter, sizeof(BITMAPINFOHEADER));
		bitmapiter += sizeof(BITMAPINFOHEADER);

		if (InfoHdr.bmiHeader.biBitCount != 24 && InfoHdr.bmiHeader.biBitCount != 8)
		{
			ErrorMsg("Only 16M or 8bit color format supported");
			return	false;
		}
		DWORD	pitch = (InfoHdr.bmiHeader.biWidth + 3) & ~3;
		m_width = InfoHdr.bmiHeader.biWidth;
		m_height = InfoHdr.bmiHeader.biHeight;

		pColorMap = new BYTE[m_width * m_height * 3];
		m_data = new DWORD[m_width * m_height];

		if (InfoHdr.bmiHeader.biBitCount == 8) {
			RGBQUAD	colorMap[256];
			BYTE	*bufPtr;
			BYTE	tmpRow[4096];
			int		k;

			if (InfoHdr.bmiHeader.biClrUsed)
			{
				memcpy(colorMap, bitmapiter, InfoHdr.bmiHeader.biClrUsed * sizeof(RGBQUAD));
				bitmapiter += InfoHdr.bmiHeader.biClrUsed * sizeof(RGBQUAD);
			}
			else
			{
				memcpy(colorMap, bitmapiter, 256 * sizeof(RGBQUAD));
				bitmapiter += 256 * sizeof(RGBQUAD);
			}

			bitmapiter = bitmap + FileHdr.bfOffBits;

			for (i = m_height - 1; i >= 0; i--) {
				int	padded = int(m_width + 3) & (~3);

				memcpy(tmpRow, bitmapiter, padded);
				bitmapiter += padded;

				bufPtr = pColorMap + m_width * i * 3;

				for (j = 0; j < m_width; j++) {
					k = tmpRow[j];
					*bufPtr		  = colorMap[k].rgbBlue;
					*(bufPtr + 1) = colorMap[k].rgbGreen;
					*(bufPtr + 2) = colorMap[k].rgbRed;
					bufPtr += 3;
				}
			}
		}
		else if (InfoHdr.bmiHeader.biBitCount == 24) {
			bitmapiter = bitmap + FileHdr.bfOffBits;

			for (i = m_height - 1; i >= 0; i--) {

				memcpy(pColorMap + i * m_width * 3, bitmapiter, m_width * 3);
				bitmapiter += m_width * 3;

				if (pitch - m_width) {
					char	tmp[10];
					memcpy(tmp, bitmapiter, 4 - (pitch - m_width));
					bitmapiter += 4 - (pitch - m_width);
				}
			}
		}
	}

	DWORD *	ptr;
	int		srcr, srcg, srcb;

	for (i = 0; i < m_height; i++)
	{
		BYTE *	colorPtr = pColorMap + i * m_width * 3;
		ptr = m_data + m_width * i;

		if (colorPtr != NULL && ptr != NULL)
		{
			srcb = *colorPtr;
			srcg = *(colorPtr + 1);
			srcr = *(colorPtr + 2);
			*ptr++ = PACK32BIT8888(0xff, srcr, srcg, srcb);

			for (j = 1; j < m_width; j++)
			{
				colorPtr += 3;
				srcb = *colorPtr;
				srcg = *(colorPtr + 1);
				srcr = *(colorPtr + 2);
				*ptr++ = PACK32BIT8888(0xff, srcr, srcg, srcb);
			}
		}
	}
	if (pColorMap)
		delete [] pColorMap;
	m_isAlpha = FALSE;
	return true;
#else
	BITMAPFILEHEADER	FileHdr;
	BITMAPINFO			InfoHdr;
	BYTE *				pColorMap;
	int					i, j;
	const BYTE*			bitmapiter = bitmap;

	Reset();

	memcpy(&FileHdr, bitmapiter, sizeof(BITMAPFILEHEADER));
	bitmapiter += sizeof(BITMAPFILEHEADER);

	if (FileHdr.bfType != 'B' + 'M' * 256) return false;

	memcpy(&InfoHdr, bitmapiter, sizeof(BITMAPINFOHEADER));
	bitmapiter += sizeof(BITMAPINFOHEADER);

	if (InfoHdr.bmiHeader.biBitCount != 24 && InfoHdr.bmiHeader.biBitCount != 8)
	{
		ErrorMsg("Only 16M or 8bit color format supported");
		return	false;
	}
	DWORD	pitch = (InfoHdr.bmiHeader.biWidth + 3) & ~3;
	m_width = InfoHdr.bmiHeader.biWidth;
	m_height = InfoHdr.bmiHeader.biHeight;

	pColorMap = new BYTE[m_width * m_height * 3];
	m_data = new DWORD[m_width * m_height];

	if (InfoHdr.bmiHeader.biBitCount == 8) {
		RGBQUAD	colorMap[256];
		BYTE	*bufPtr;
		BYTE	tmpRow[4096];
		int		k;

		if (InfoHdr.bmiHeader.biClrUsed)
		{
			memcpy(colorMap, bitmapiter, InfoHdr.bmiHeader.biClrUsed * sizeof(RGBQUAD));
			bitmapiter += InfoHdr.bmiHeader.biClrUsed * sizeof(RGBQUAD);
		}
		else
		{
			memcpy(colorMap, bitmapiter, 256 * sizeof(RGBQUAD));
			bitmapiter += 256 * sizeof(RGBQUAD);
		}

		bitmapiter = bitmap + FileHdr.bfOffBits;

		for (i = m_height - 1; i >= 0; i--) {
			int	padded = int(m_width + 3) & (~3);

			memcpy(tmpRow, bitmapiter, padded);
			bitmapiter += padded;

			bufPtr = pColorMap + m_width * i * 3;

			for (j = 0; j < m_width; j++) {
				k = tmpRow[j];
				*bufPtr		  = colorMap[k].rgbBlue;
				*(bufPtr + 1) = colorMap[k].rgbGreen;
				*(bufPtr + 2) = colorMap[k].rgbRed;
				bufPtr += 3;
			}
		}
	}
	else if (InfoHdr.bmiHeader.biBitCount == 24) {
		bitmapiter = bitmap + FileHdr.bfOffBits;

		for (i = m_height - 1; i >= 0; i--) {

			memcpy(pColorMap + i * m_width * 3, bitmapiter, m_width * 3);
			bitmapiter += m_width * 3;

			if (pitch - m_width) {
				char	tmp[10];
				memcpy(tmp, bitmapiter, 4 - (pitch - m_width));
				bitmapiter += 4 - (pitch - m_width);
			}
		}
	}

	DWORD *	ptr;
	int		srcr, srcg, srcb;

	for (i = 0; i < m_height; i++) {
		BYTE *	colorPtr = pColorMap + i * m_width * 3;
		ptr = m_data + m_width * i;

		for (j = 0; j < m_width; j++) {
			srcb = *colorPtr;
			srcg = *(colorPtr + 1);
			srcr = *(colorPtr + 2);
			*ptr++ = PACK32BIT8888(0xff, srcr, srcg, srcb);
			colorPtr += 3;
		}
	}
	delete [] pColorMap;
	m_isAlpha = FALSE;
	return true;
#endif
}

bool	CBitmapRes :: LoadTGAData(const BYTE* bitmap, int size)
{
	struct TargaHeader {
		BYTE IDLength;
		BYTE ColormapType;
		BYTE ImageType;
		BYTE ColormapSpecification[5];
		WORD XOrigin;
		WORD YOrigin;
		WORD ImageWidth;
		WORD ImageHeight;
		BYTE PixelDepth;
		BYTE ImageDescriptor;
	} tga;
	const BYTE* bitmapiter = bitmap;

	memcpy(&tga, bitmapiter, sizeof(TargaHeader));
	bitmapiter += sizeof(TargaHeader);


	if (0 != tga.ColormapType || (tga.ImageType != 10 && tga.ImageType != 2)) return false;


	if (tga.IDLength) bitmapiter += tga.IDLength;

	m_width = tga.ImageWidth;
	m_height = tga.ImageHeight;
	m_data = new DWORD[m_width * m_height];

	DWORD *	tmpBuf = m_data + m_width * (m_height - 1);

	DWORD	r = 0, g = 0, b = 0;
	BYTE a = 0xff;

	for (int y = 0; y < m_height; y++) {
		for (int x = 0; x < m_width; ) {

			if (tga.ImageType == 10) {
				BYTE PacketInfo;

				memcpy(&PacketInfo, bitmapiter, 1);
				bitmapiter += 1;

				WORD PacketType = 0x80 & PacketInfo;
				WORD PixelCount = (0x007f & PacketInfo) + 1;

				if (PacketType) {
					memcpy(&b, bitmapiter, 1);
					bitmapiter += 1;
					memcpy(&g, bitmapiter, 1);
					bitmapiter += 1;
					memcpy(&r, bitmapiter, 1);
					bitmapiter += 1;

					if (tga.PixelDepth == 32)
					{
						memcpy(&a, bitmapiter, 1);
						bitmapiter += 1;
					}

					while (PixelCount--) {
						*(tmpBuf + x) = PACK32BIT8888(a, r, g, b);
						x++;
					}
				}
				else {
					while (PixelCount--) {
						memcpy(&b, bitmapiter, 1);
						bitmapiter += 1;
						memcpy(&g, bitmapiter, 1);
						bitmapiter += 1;
						memcpy(&r, bitmapiter, 1);
						bitmapiter += 1;

						if (tga.PixelDepth == 32)
						{
							memcpy(&a, bitmapiter, 1);
							bitmapiter += 1;
						}

						*(tmpBuf + x) = PACK32BIT8888(a, r, g, b);
						x++;
					}
				}
			}
			else {
				memcpy(&b, bitmapiter, 1);
				bitmapiter += 1;
				memcpy(&g, bitmapiter, 1);
				bitmapiter += 1;
				memcpy(&r, bitmapiter, 1);
				bitmapiter += 1;

				if (tga.PixelDepth == 32)
				{
					memcpy(&a, bitmapiter, 1);
					bitmapiter += 1;
				}

				*(tmpBuf + x) = PACK32BIT8888(a, r, g, b);
				x++;
			}
		}

		tmpBuf -= m_width;
	}
	m_isAlpha = TRUE;
	return true;
}

bool	CBitmapRes :: LoadJPGData(const BYTE* bitmap, int size)
{
	const BYTE *		temp = bitmap;
	BYTE *				pColorMap;
	JPEG_CORE_PROPERTIES	JPGimg;
	int					len = size;
	int					i, j;

#if defined(BF_PORT)
	// The Intel JPEG Library is replaced by ImageIO (technical plan 3.3). Rows
	// are decoded unpadded, as the conversion below reads them.
	if (bf_codec_jpeg_info(temp, len, &m_width, &m_height) != BF_CODEC_OK) {
		m_width = m_height = 0;
		return false;
	}
	pColorMap = new BYTE[m_width * m_height * 3];
	m_data = new DWORD[m_width * m_height];
	if (bf_codec_jpeg_decode(temp, len, pColorMap, m_width, m_height, m_width * 3, BF_CODEC_BGR24) != BF_CODEC_OK) {
		delete [] pColorMap;
		delete [] m_data;
		m_data = NULL;
		return false;
	}
#else
	memset(&JPGimg, 0, sizeof(JPEG_CORE_PROPERTIES));

	if (ijlInit(&JPGimg) != IJL_OK) return false;

	JPGimg.JPGBytes = (BYTE*)temp;
	JPGimg.JPGSizeBytes = len;

	if (ijlRead(&JPGimg, IJL_JBUFF_READPARAMS) == IJL_OK) {
		m_width = JPGimg.JPGWidth;
		m_height = JPGimg.JPGHeight;

		pColorMap = new BYTE[m_width * m_height * 3];
		m_data = new DWORD[m_width * m_height];

		JPGimg.DIBBytes = pColorMap;
		JPGimg.DIBColor = IJL_BGR;
		JPGimg.DIBWidth = m_width;
		JPGimg.DIBHeight = m_height;
		JPGimg.DIBChannels = 3;
		JPGimg.DIBPadBytes = IJL_DIB_PAD_BYTES(m_width, 3);

#ifdef EPISODE_VER
		int result = ijlRead(&JPGimg, IJL_JBUFF_READWHOLEIMAGE);
#ifdef _DEBUG
		if ( result != IJL_OK )
			ErrorMsg("ijlRead failed - check loaded patch quality");
#endif

#else
		if (ijlRead(&JPGimg, IJL_JBUFF_READWHOLEIMAGE) != IJL_OK) {
			delete [] pColorMap;
			pColorMap = NULL;

			delete [] m_data;
			m_data = NULL;

			return false;
		}
#endif
	}
	ijlFree(&JPGimg);
#endif

	DWORD *	ptr;
	int		srcr, srcg, srcb;

	for (i = 0; i < m_height; i++) {
		BYTE *	colorPtr = pColorMap + i * m_width * 3;
		ptr = m_data + m_width * i;

		if (!colorPtr) return false;
		for (j = 0; j < m_width; j++) {
			srcb = *colorPtr;
			srcg = *(colorPtr + 1);
			srcr = *(colorPtr + 2);

			if ((srcr >= 0xfe && srcr <= 0xff) && (srcg >= 0 && srcg <= 3) && (srcb >= 0xfe && srcb <= 0xff)) {
				*ptr++ = PACK32BIT8888(0, COLORKEY_R, COLORKEY_G, COLORKEY_B);
			} else {
				*ptr++ = PACK32BIT8888(0xff, srcr, srcg, srcb);
			}

			colorPtr += 3;
		}
	}
	delete [] pColorMap;
	m_isAlpha = FALSE;
	return true;
}
