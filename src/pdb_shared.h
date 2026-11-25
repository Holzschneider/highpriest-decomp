#ifndef HIGHPRIEST_PDB_SHARED_H
#define HIGHPRIEST_PDB_SHARED_H







struct Registry {
public:
	unsigned int guidDevice[4];   // a GUID, as the setup program stored it (M11.9: 16 bytes, read only)
	unsigned int guidDriver[4];
	char deviceName[40];
	unsigned int isFullScreenMode;
	unsigned int isLowResSprite;
	unsigned int useCompressedTexture;
	unsigned int isFogRender;
	unsigned int Width;
	unsigned int Height;
	unsigned int bitPerPixel;
	unsigned int deviceCnt;
	unsigned int modeCnt;
	unsigned int frameSkip;
	unsigned int filtering;
	unsigned int m_bUse3dSound;
	unsigned int m_soundOutPut;
	unsigned int m_soundDriver;
};

struct Registry2 {
public:
	unsigned int m_guidDevice[4];   // a GUID, as the setup program stored it (M11.9: 16 bytes, read only)
	unsigned int m_guidDriver[4];
	char m_deviceName[40];
	unsigned int m_isFullScreenMode;
	unsigned int m_width;
	unsigned int m_height;
	unsigned int m_bitPerPixel;
	unsigned int m_deviceCnt;
	unsigned int m_modeCnt;
	unsigned int m_isVoodoo;
	unsigned int m_spriteMode;
	unsigned int m_textureMode;
	unsigned int m_trilinearFilter;
	int m_soundMode;
	int m_speakerType;
	int m_digitalrateType;
	int m_numSampleType;
	int m_digitalbitsType;
	int m_useWaveout;
	char m_providerName[1024];
};

#endif
