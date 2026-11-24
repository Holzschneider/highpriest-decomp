#ifndef HIGHPRIEST_RESOURCE_SKYBOX_H
#define HIGHPRIEST_RESOURCE_SKYBOX_H

#include "../common.h"
#include "../3dlib/Prim.h"
#include "../3dlib/Renderer.h"
#include "Model.h"
#include "../3dActor.h"

class CBitmapRes;
struct SprImg;
class CFlare;
class CSkyBox;
class CSkyBoxEllipse;
struct Flare;
struct PrimSegment;
struct TeiEffect;
struct ViewInfo3d;
struct objectSegment;
struct tmpActorInfo;


struct Flare {
public:
	int wType;
	float fLoc;
	float fScale;
	float r;
	float g;
	float b;
	vector3d vPositionPt;
	float fRenderSize;
};

class CFlare {
public:
	CFlare() {}
	~CFlare() {}
	void Render(matrix& view, vector3d vFrom, vector3d vAt);
	Flare SetFlare(int __$ReturnUdt, float wType, float fLocation, float fScale, float fRed, float fGreen);
	void Process(float fTimeKey, vector3d vFrom, vector3d fViewScale);
	void Init();
private:
	Flare m_Flare[12];
	unsigned int m_flareColor[12];
	char m_strTexName[260];
	vector3d m_vLightPt;
};

class CSkyBox {
public:
	CSkyBox();
	~CSkyBox();
	void Render(matrix& view, vector3d* vPos, float roty, float fLatitude, vector3d vFrom, vector3d vAt);
private:
	bool ChkVisible(float hratio, float isPartial, vector3d vratio, matrix& vPos, tlvertex3d view);
private:
	tlvertex3d m_pvWall[10];
	float m_fLoopRotY;
	CFlare m_flare;
};

class CSkyBoxEllipse {
public:
	CSkyBoxEllipse();
	~CSkyBoxEllipse();
	void Render(matrix& view, vector3d* vPos, float roty, float fLatitude, vector3d vFrom, vector3d vAt);
private:
	C3dModelRes* m_res;
	C3dActor* m_sky;
	float m_fLoopRotX;
	CFlare m_flare;
};


#endif
