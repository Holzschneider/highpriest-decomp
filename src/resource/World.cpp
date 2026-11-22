#include "common.h"
#include "../GameWorld.h"
#include "World.h"
#include "../base/File.h"

#include "../3dlib/DrawUtil.h"
#include "../RagEffect.h"
#include "../RagEffectPrim.h"
#include "Ground.h"

extern int g_isLightMap;


using std::string;




typedef std::list<C3dWorldRes::actorInfo *>     ActorInfoList;
typedef std::list<C3dWorldRes::effectSrcInfo *> EffectSrcInfoList;
typedef std::list<C3dWorldRes::soundSrcInfo *>  SoundSrcInfoList;

#ifndef MAX_OBJNAME
#endif


static C3dWorldRes C3dWorldResInstance(Exemplar(), "rsw", "");
C3dWorldRes :: C3dWorldRes()
	: CRes()
{
	m_CalculatedNode = NULL;
}

void	C3dWorldRes :: Reset()
{
	ActorInfoList::iterator i = m_3dActors.begin();
	while (i != m_3dActors.end()) {
		delete *i;
		i++;
	}
	m_3dActors.erase(m_3dActors.begin(), m_3dActors.end());

	EffectSrcInfoList::iterator i4 = m_particles.begin();
	while (i4 != m_particles.end()) {
		delete *i4;
		i4++;
	}
	m_particles.erase(m_particles.begin(), m_particles.end());

	SoundSrcInfoList::iterator i5 = m_sounds.begin();
	while (i5 != m_sounds.end()) {
		delete *i5;
		i5++;
	}

	if (m_CalculatedNode) {
		delete m_CalculatedNode;
		m_CalculatedNode = NULL;
	}
	m_sounds.erase(m_sounds.begin(), m_sounds.end());
}

struct tmpActorInfo {
public:
	char modelName[80];
	char nodeName[80];
	vector3d pos;
	vector3d rot;
	vector3d scale;
};

enum	{
	OT_VIRTUAL=0, OT_MODEL, OT_LIGHTSRC, OT_SOUNDSRC, OT_EFFECTSRC,
};

bool	C3dWorldRes :: Load(const char *fName)
{

#ifdef _DEBUG
	OutputDebugString("C3dWorldRes :: Load");

	sprintf(g_Mapmsg,"%s",fName);
	OutputDebugString(fName);
#endif

	Reset();
	CFile	fp;
	if(!fp.Open(fName , CFile::modeRead))
	{
		return false;
	}

	char	magicId[] = "GRSW";
	char	magicBuf[5];

	fp.Read(magicBuf, 4);
	magicBuf[4]=0;
	if (strcmp(magicBuf, magicId))
	{
		ErrorMsg("Invalid Format");
		return	false;
	}



	BYTE	verMajor;
	BYTE	verMinor;
	fp.Read(&verMajor, sizeof(BYTE));

	fp.Read(&verMinor, sizeof(BYTE));
	if ((verMajor==2 && verMinor > 1) || verMajor>2)
	{
		ErrorMsg("C3dWorldRes :: Unsupported Version");
		return	false;
	}
	m_verMajor = verMajor;
	m_verMinor = verMinor;

	char	nameBuf[40];

	fp.Read(nameBuf, 40);


	fp.Read(nameBuf, 40);
	m_gndFile = string(nameBuf);
	if ((verMajor==1 && verMinor>=4) || verMajor>=2)
	{
		fp.Read(nameBuf, 40);
		m_attrFile = string(nameBuf);
	}


	fp.Read(nameBuf, 40);
	m_scrFile = string(nameBuf);

	if ((verMajor==1 && verMinor>=3) || verMajor>=2)
	{
		fp.Read(&m_waterLevel, sizeof(float));
	}
	if ((verMajor==1 && verMinor>=8) || verMajor>=2)
	{
		fp.Read(&m_waterType, sizeof(int));
		fp.Read(&m_waveHeight, sizeof(float));
		fp.Read(&m_waveSpeed, sizeof(float));
		fp.Read(&m_wavePitch, sizeof(float));
	}
	else
	{
		m_waterType = 0;
		m_waveHeight = 1;
		m_waveSpeed = 2;
		m_wavePitch = 50;
	}
	if ((verMajor==1 && verMinor>=9) || verMajor>=2)
	{
		fp.Read(&m_waterAnimSpeed, sizeof(int));
	}
	else
	{
		m_waterAnimSpeed = 3;
	}
	if ((verMajor==1 && verMinor>=5) || verMajor>=2)
	{
		fp.Read(&m_lightLongitude, sizeof(int));
		fp.Read(&m_lightLatitude, sizeof(int));
		fp.Read(&m_diffuseCol, sizeof(vector3d));
		fp.Read(&m_ambientCol, sizeof(vector3d));
	}
	else
	{
		m_lightLatitude = 45;
		m_lightLongitude = 45;

		m_diffuseCol.Set(1.0f, 1.0f, 1.0f);
		m_ambientCol.Set(0.3f, 0.3f, 0.3f);
	}

	matrix	r;
	r.MakeXRotation(DegToRad(m_lightLatitude));
	r.AppendYRotation(DegToRad(m_lightLongitude));
	m_lightDir.MatrixMult(vector3d(0, 1, 0), r);

	if ((verMajor==1 && verMinor>=7) || verMajor>=2)
	{
		float shadowOpacity;
		fp.Read(&shadowOpacity, sizeof(float));
	}

	if ((verMajor==1 && verMinor>=6) || verMajor>=2)
	{
		fp.Read(&m_groundTop, sizeof(int));
		fp.Read(&m_groundBottom, sizeof(int));
		fp.Read(&m_groundLeft, sizeof(int));
		fp.Read(&m_groundRight, sizeof(int));
	}
	else
	{
		m_groundTop = -500;
		m_groundBottom = 500;
		m_groundLeft = -500;
		m_groundRight = 500;
	}


	int		len;
	fp.Read(&len, sizeof(int));
	for (int i=0; i<len; i++)
	{
		int		type;
		fp.Read(&type, sizeof(int));
		switch (type)
		{

			case	OT_MODEL:
				{
					int		animType, blockType;
					float	animSpeed;
					if ((verMajor==1 && verMinor>=3) || verMajor>=2)
					{
						fp.Read(nameBuf, 40);
						fp.Read(&animType, sizeof(int));
						fp.Read(&animSpeed, sizeof(float));
						fp.Read(&blockType, sizeof(int));



					}
					else
					{
						animType = 0;
						animSpeed = 1.0f;
						blockType = 0;
					}

					if (animSpeed > 100 || animSpeed <= 0)
						animSpeed = 1.0f;

					tmpActorInfo	tmpInfo;
					fp.Read(&tmpInfo, sizeof(tmpActorInfo));
					actorInfo *	Info = new actorInfo;
					strcpy(Info->name, nameBuf);
					strcpy(Info->modelName, tmpInfo.modelName);
					strcpy(Info->nodeName, tmpInfo.nodeName);
					Info->animSpeed = animSpeed;
					Info->animType = animType;
					Info->blockType = blockType;
					Info->pos = tmpInfo.pos;
					Info->rot = tmpInfo.rot;
					Info->scale = tmpInfo.scale;

					m_3dActors.push_back(Info);
				}
				break;

			case	OT_LIGHTSRC:
				{
					lightSrcInfo	tmpInfo;
					fp.Read(&tmpInfo, sizeof(tmpInfo));
				}
				break;
			case	OT_EFFECTSRC:
				{
					effectSrcInfo * Info = new effectSrcInfo;
					fp.Read(Info, sizeof(effectSrcInfo));
					m_particles.push_back(Info);
				}
				break;
			case	OT_SOUNDSRC:
				{
					if (verMajor>=2)
					{
						soundSrcInfo * Info = new soundSrcInfo;
						fp.Read(Info, sizeof(soundSrcInfo));
						m_sounds.push_back(Info);
					}
					else
					{
						soundSrcInfo_V19 Info_V19;
						fp.Read(&Info_V19, sizeof(soundSrcInfo_V19));

						soundSrcInfo * Info = new soundSrcInfo;
						strncpy(Info->name, Info_V19.name, MAX_OBJNAME-1);
						strncpy(Info->waveName, Info_V19.waveName, 80-1);
						Info->pos =	Info_V19.pos;
						Info->vol =	Info_V19.vol;
						Info->width = Info_V19.width;
						Info->height = Info_V19.height;
						Info->range = Info_V19.range;
						Info->cycle = 4;
						m_sounds.push_back(Info);
					}
				}
				break;
			default:
				sAssert(false);
				Trace("not implemented type == %d (%d/%d)\n", type, i+1, len);
				break;
		}
	}

	if(verMajor == 2 && verMinor == 1)
	{
		m_CalculatedNode = new SceneGraphNode;
		m_CalculatedNode->Build(0);
		m_CalculatedNode->LoadSceneGraph(&fp, 0);
		Trace("SceneGraphNode Load.");
	}
	fp.Close();

	if (g_isLightMap != 1) {
		m_ambientCol.x *= 1.5;
		m_ambientCol.y *= 1.5;
		m_ambientCol.z *= 1.5;
		if (m_ambientCol.x > 1.0f) m_ambientCol.x = 1.0f;
		if (m_ambientCol.y > 1.0f) m_ambientCol.y = 1.0f;
		if (m_ambientCol.z > 1.0f) m_ambientCol.z = 1.0f;
	}



	g_Weather.SetAmbient( m_ambientCol );
	g_Weather.SetDiffuse( m_diffuseCol );
	g_Weather.SetMaxDiffuse( m_diffuseCol );

	return	true;
}
