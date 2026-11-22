#ifndef HIGHPRIEST_RESOURCE_BITMAP_H
#define HIGHPRIEST_RESOURCE_BITMAP_H

#include "../base/Res.h"

class CBitmapRes;

class CBitmapRes : public CRes {
public:
	DECLARE_RESOURCE(CBitmapRes)
	CBitmapRes();
	virtual bool Load(const char* fName);
	virtual bool LoadFromBuffer(const char* fName, const unsigned char* buffer, int size);
	virtual void Reset();
	unsigned long GetColor(int x, int y);
	bool SetAlphaWithBMP(const char* fName);
	bool ChangeToGrayScaleBMP(int blackLimit, int whiteLimit);
	void CreateNull(int x, int y);
private:
	bool LoadBMPData(const unsigned char* bitmap, int size);
	bool LoadTGAData(const unsigned char* bitmap, int size);
	bool LoadJPGData(const unsigned char* bitmap, int size);
public:

	int m_isAlpha;
	int m_width;
	int m_height;
	unsigned long* m_data;
};

#endif
