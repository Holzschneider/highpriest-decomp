#ifndef HIGHPRIEST_PDB_SHARED_H
#define HIGHPRIEST_PDB_SHARED_H







struct HP_Damage_Moment {
public:
	unsigned long DamageTime;
	int DamageChar;
	int DamageNum;
	char DamageEnd;
};


struct HP_TRAVEL_AVAILABLE {
public:
	short X;
	short Y;
	char Map;
};

struct Registry {
public:
	_GUID guidDevice;
	_GUID guidDriver;
	char deviceName[40];
	unsigned long isFullScreenMode;
	unsigned long isLowResSprite;
	unsigned long useCompressedTexture;
	unsigned long isFogRender;
	unsigned long Width;
	unsigned long Height;
	unsigned long bitPerPixel;
	unsigned long deviceCnt;
	unsigned long modeCnt;
	unsigned long frameSkip;
	unsigned long filtering;
	unsigned long m_bUse3dSound;
	unsigned long m_soundOutPut;
	unsigned long m_soundDriver;
};

struct Registry2 {
public:
	_GUID m_guidDevice;
	_GUID m_guidDriver;
	char m_deviceName[40];
	unsigned long m_isFullScreenMode;
	unsigned long m_width;
	unsigned long m_height;
	unsigned long m_bitPerPixel;
	unsigned long m_deviceCnt;
	unsigned long m_modeCnt;
	unsigned long m_isVoodoo;
	unsigned long m_spriteMode;
	unsigned long m_textureMode;
	unsigned long m_trilinearFilter;
	int m_soundMode;
	int m_speakerType;
	int m_digitalrateType;
	int m_numSampleType;
	int m_digitalbitsType;
	int m_useWaveout;
	char m_providerName[1024];
};

#endif
