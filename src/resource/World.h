#ifndef HIGHPRIEST_RESOURCE_WORLD_H
#define HIGHPRIEST_RESOURCE_WORLD_H

#include "../base/Res.h"
#include "../3dlib/Prim.h"
#include "../GameWorld.h"
const int MAX_OBJNAME = 80;

class C3dWorldRes;

class C3dWorldRes : public CRes {
public:
	struct actorInfo {
	public:
		char name[40];
		char modelName[80];
		char nodeName[80];
		vector3d pos;
		vector3d rot;
		vector3d scale;
		int animType;
		int blockType;
		float animSpeed;
		float posx;
		float posy;
		float posz;
	};

	struct effectSrcInfo {
	public:
		char name[80];
		vector3d pos;
		int type;
		float emitSpeed;
		float param[4];
	};

	struct lightSrcInfo {
	public:
		char name[80];
		vector3d pos;
		int red;
		int green;
		int blue;
		float range;
	};

	struct soundSrcInfo {
	public:
		char name[80];
		char waveName[80];
		vector3d pos;
		float vol;
		int width;
		int height;
		float range;
		float cycle;
	};

	struct soundSrcInfo_V19 {
	public:
		char name[80];
		char waveName[80];
		vector3d pos;
		float vol;
		int width;
		int height;
		float range;
	};

public:
	DECLARE_RESOURCE(C3dWorldRes)
	C3dWorldRes();
	virtual void Reset();
	virtual bool Load(const char* fName);
	std::list<actorInfo *> m_3dActors;
	std::list<effectSrcInfo *> m_particles;
	std::list<soundSrcInfo *> m_sounds;
	SceneGraphNode* m_CalculatedNode;
	std::string m_gndFile;
	std::string m_attrFile;
	std::string m_scrFile;
	float m_waterLevel;
	int m_waterType;
	float m_waveHeight;
	float m_waveSpeed;
	float m_wavePitch;
	int m_waterAnimSpeed;
	int m_lightLongitude;
	int m_lightLatitude;
	vector3d m_lightDir;
	vector3d m_diffuseCol;
	vector3d m_ambientCol;
	unsigned char m_verMajor;
	unsigned char m_verMinor;
	int m_groundTop;
	int m_groundBottom;
	int m_groundLeft;
	int m_groundRight;
};

#endif
