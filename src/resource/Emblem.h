#ifndef HIGHPRIEST_RESOURCE_EMBLEM_H
#define HIGHPRIEST_RESOURCE_EMBLEM_H

#include "../base/Res.h"
#include "Bitmap.h"

class CEmblemRes;

class CEmblemRes : public CRes {
public:
	DECLARE_RESOURCE(CEmblemRes)
	CEmblemRes();
	virtual bool Load(const char* fName);
	virtual void Reset();
	CBitmapRes* GetBitmap() { return &m_bitmap; }
private:
	bool UnCompressEmblem(unsigned char* bitmap, unsigned int& bitmapSize, unsigned char* emblem, int emSize);
	virtual void OnLoadError(const char* fName) {}
private:
	CBitmapRes m_bitmap;
};

#endif
