#include "common.h"
#include "Ground.h"



#ifndef LMAPWIDTH
#endif
#ifndef LMAPHEIGHT
#endif
#ifndef LMAPPF
#define LMAPPF 0
#endif
#ifndef LMAPIDATASIZE
#endif
#ifndef LMAPSDATASIZE
#endif


static CGndRes CGndResInstance(Exemplar(), "gnd", "");
void	ColorChannel::GetChannel(BYTE channel[64])
{
	for(int i=0; i<64; ++i) channel[i] = GetBuffer(i);
}

void	ColorChannel::SetChannel(BYTE channel[64])
{
	for(int i=0; i<64; ++i) SetBuffer(i, channel[i]);
}

BYTE	ColorChannel::GetBuffer(int offset)
{
	int offset_bit = (offset * 5) % 8;
	int offset_byte = (offset * 5) / 8;

	if(offset_bit < 4) {

		return ( m_buffer[offset_byte] & (0xF8 >> offset_bit) ) << offset_bit;

	} else {

		return ( ( m_buffer[offset_byte] & (0xF8 >> offset_bit) ) << offset_bit ) + ( ( m_buffer[offset_byte+1] & (0xF8 << (8-offset_bit)) ) >> (8-offset_bit) );
	}
}

void	ColorChannel::SetBuffer(int offset, BYTE value)
{
	int offset_bit = (offset * 5) % 8;
	int offset_byte = (offset * 5) / 8;

	if(offset_bit < 4) {

		m_buffer[offset_byte] = (m_buffer[offset_byte] & (0xFFFFFF07 >> offset_bit) ) | ( (value & 0xF8) >> offset_bit );

	} else {

		m_buffer[offset_byte] = (m_buffer[offset_byte] & (0xFFFFFF07 >> offset_bit) ) | ( (value & 0xF8) >> offset_bit );

		m_buffer[offset_byte+1] = (m_buffer[offset_byte+1] & (0xFFFFFF07 << (8-offset_bit)) ) | ( (value & 0xF8) << (8-offset_bit) );

	}
}

CGndRes :: CGndRes()
	: CRes()
{
	m_width = 0;
	m_height = 0;
	m_zoom = 10;

	m_newVer = FALSE;
}

bool	CGndRes :: Load(const char *fName)
{
	if (m_fp.Open(fName, CFile::modeRead) == FALSE)
	{
		return	false;
	}



	char header[4];

	m_fp.Read(header, 4);

	if (memcmp(header, "GRGN", 4) == 0)
	{
		m_newVer = TRUE;

		return LoadGnd();

	}
	else
	{
		return false;
	}
}

void	CGndRes :: Reset()
{
	m_texNameTable.clear();
	m_lminfo = NULL;
	m_surface = NULL;
	m_V17cells = NULL;
}

bool	CGndRes :: LoadGnd()
{
	int		i;
	int		maxTexName;




	m_fp.Read(&m_verMajor, sizeof(BYTE));
	m_fp.Read(&m_verMinor, sizeof(BYTE));

	if(m_verMajor == 1 && m_verMinor > 6)
	{

		m_fp.Read(&m_width, sizeof(int));
		m_fp.Read(&m_height, sizeof(int));
		m_fp.Read(&m_zoom, sizeof(float));

		sAssert(m_width > 0 && m_width < 1000);
		sAssert(m_height > 0 && m_height < 1000);
		sAssert(m_zoom > 0 && m_zoom < 1000);


		m_fp.Read(&m_numTexture, sizeof(int));
		m_fp.Read(&maxTexName, sizeof(int));
		m_texNameTable.resize(m_numTexture);
		for (i = 0; i < m_numTexture; i++)
		{
			m_texNameTable[i] = (const char*)m_fp.GetCurBuf(maxTexName);
		}

		m_fp.Read(&m_numLightmap, sizeof(int));
		int		lmapWidth;
		int		lmapHeight;
		int		lmapPf;

		m_fp.Read(&lmapWidth, sizeof(int));
		m_fp.Read(&lmapHeight, sizeof(int));
		m_fp.Read(&lmapPf, sizeof(int));

		sAssert(lmapWidth == LMAPWIDTH);
		sAssert(lmapHeight == LMAPHEIGHT);
		sAssert(lmapPf == LMAPPF);

		if(m_verMinor == 7)
		{
			m_lminfo = (CLMInfo*)m_fp.GetCurBuf((LMAPIDATASIZE + LMAPSDATASIZE) * m_numLightmap);
		}
		else
		{
			m_lmindex = (LMIndex*)m_fp.GetCurBuf(sizeof(LMIndex) * m_numLightmap);

			int		colorchannel;

			m_fp.Read(&colorchannel, sizeof(int));

			m_colorchannel = (ColorChannel*)m_fp.GetCurBuf(sizeof(ColorChannel) * colorchannel);
		}

		m_fp.Read(&m_numSurface, sizeof(int));

		m_surface = (GndSurfaceFmt*)m_fp.GetCurBuf(sizeof(GndSurfaceFmt) * m_numSurface);
		m_V17cells = (GndCellFmt17*)m_fp.GetCurBuf();

		return true;
	}
	else
	{
		return false;
	}
}

void	CGndRes::GetLMInfo(int n, CLMInfo* lminfo)
{
	if(m_verMinor == 7) {
		*lminfo = m_lminfo[n];
	} else {
		BYTE a[64];
		BYTE r[64];
		BYTE g[64];
		BYTE b[64];

		m_colorchannel[m_lmindex[n].a].GetChannel(a);
		m_colorchannel[m_lmindex[n].r].GetChannel(r);
		m_colorchannel[m_lmindex[n].g].GetChannel(g);
		m_colorchannel[m_lmindex[n].b].GetChannel(b);

		for(int i=0; i<64; ++i) {

			lminfo->idata[i%8][i/8] = a[i];
			lminfo->sdata[i%8][i/8][0] = r[i];
			lminfo->sdata[i%8][i/8][1] = g[i];
			lminfo->sdata[i%8][i/8][2] = b[i];
		}
	}
}
