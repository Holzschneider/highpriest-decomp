#include "common.h"
#include "../base/File.h"
#include "Emblem.h"
















static CEmblemRes CEmblemResInstance(Exemplar(), "ebm", "..\\_tmpEmblem\\");
CEmblemRes :: CEmblemRes()
{
}

bool	CEmblemRes :: Load(const char *fName)
{
	CFile fp;

	if(!fp.Open(fName, CFile::modeRead)) return false;

	sAssert(fp.GetLength() < 8192);

	BYTE emblem[8192];
	BYTE bitmap[8192];
	ULONG bitmapSize = 8192;

	int emSize = fp.Read(emblem, fp.GetLength());

	if(!UnCompressEmblem(bitmap, bitmapSize, emblem, fp.GetLength()))
	{
		fp.Close();
		return false;
	}

	if(bitmapSize != 8192)
	{
		if(m_bitmap.LoadFromBuffer("_emblem.bmp", bitmap, bitmapSize) == true)
		{

			m_bitmap.m_width = min(m_bitmap.m_width, 24);
			m_bitmap.m_height = min(m_bitmap.m_height, 24);
			fp.Close();
			return true;
		}
	}

	m_bitmap.CreateNull(24, 24);
	fp.Close();
	return true;
}

void	CEmblemRes :: Reset()
{
	m_bitmap.Reset();
}

bool	CEmblemRes :: UnCompressEmblem(BYTE *bitmap, ULONG &bitmapSize, BYTE *emblem, int emSize)
{
	__try {
		uncompress(bitmap, &bitmapSize, emblem, emSize);
	} __except(1) {
		Trace("Failed - Emblem Uncompress!");
		return false;
	}

	Trace("Emblem Uncomp Success!");
	return true;
}
