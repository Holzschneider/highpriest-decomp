#ifndef HIGHPRIEST_RESOURCE_PALETTERES_H
#define HIGHPRIEST_RESOURCE_PALETTERES_H

#include "../base/Res.h"


class CPaletteRes : public CRes {
public:
	DECLARE_RESOURCE(CPaletteRes)
	CPaletteRes();
	virtual bool Load(const char* fName);
	virtual void Reset();
	void Unload() { Reset(); }
	SYSTEMPALETTE m_pal[256];
};

#endif
