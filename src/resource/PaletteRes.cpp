#include "common.h"
#include "PaletteRes.h"
#include "../base/File.h"
#include "../3dlib/Device.h"

using std::string;


static CPaletteRes CPaletteResInstance(Exemplar(), "pal", "palette\\");

CPaletteRes :: CPaletteRes()
	: CRes()
{
}

void	CPaletteRes :: Reset()
{
}

bool	CPaletteRes :: Load(const char *fName)
{
	CFile		fp;


	if (!fp.Open(fName, CFile::modeRead)) {
#if !defined(BF_PORT)	// missing: counted by CRes::OnLoadError after both names
		ErrorMsg((string("CPaletteRes :: Cannot find File : ") + string(fName)).c_str());
#endif
		return	false;
	}

	PALETTEENTRY paletteEntry[256];
	fp.Read(paletteEntry, sizeof(PALETTEENTRY) * 256);

	g_3dDevice.ConvertPalette(m_pal, paletteEntry, 256);

	fp.Close();
	return true;
}
