#include "common.h"
#include "View.h"
#include "GameWorld.h"
#include "GameMode.h"
#include "RagEffect.h"
#include "Session.h"
#include "GameActor.h"
#include "3dActor.h"
#include "packet.h"
#include "3dlib/Prim.h"
#include "resource/Attr.h"
#include "resource/Wave.h"
#include "device/Sound.h"
#include <map>
#include "3dlib/Device.h"
#include "RagEffect2.h"
#include "Mode.h"
#include "pdb_enums.h"
#include "resource/EZeffect.h"
#include "resource/Skybox.h"
#include "resource/Ground.h"

extern CModeMgr g_modeMgr;
extern CSession g_session;
extern CEffectPrim *g_prim;
extern CRenderer *g_renderer;

int durationTable[1024] = {
	50, 50, 50, 50, 50, 50, 100, 50, 80, 80, 100, 80,
	40, 9999, 50, 100, 40, 30, 50, 50, 100, 100, 270, 999,
	100, 40, 160, 150, 100, 200, 104, 160, 100, 200, 100, 100,
	100, 100, 100, 100, 999, 999, 150, 100, 50, 100, 180, 250,
	130, 50, 50, 50, 40, 50, 40, 40, 40, 40, 40, 40,
	200, 250, 250, 250, 999, 999, 999, 999, 999, 9999, 999, 250,
	500, 250, 250, 999, 999, 999, 999, 250, 250, 250, 250, 999,
	999, 999, 999, 999, 999, 9999, 9999, 9999, 9999, 250, 250, 150,
	9999, 9999, 250, 250, 70, 9999, 9999, 999, 999, 250, 9999, 9999,
	9999, 9999, 9999, 9999, 999, 9999, 350, 250, 250, 100, 250, 250,
	250, 40, 250, 250, 9999, 250, 9999, 9999, 250, 9999, 9999, 9999,
	250, 9999, 9999, 9999, 80, 350, 8000, 9999, 250, 9999, 250, 999,
	999, 32797, 999, 999, 999, 999, 999, 999, 100, 999, 999, 999,
	999, 999, 999, 9999, 9999, 0, 99999999, 99999999, 9999, 1000, 9999, 999,
	999, 9999, 999, 9999, 100, 100, 100, 100, 100, 100, 100, 100,
	50, 9999, 9999, 9999, 0, 9999, 9999, 9999, 9999, 9999, 9999, 9999,
	9999, 9999, 9999, 9999, 150, 9999, 0, 9999, 99999999, 99999999, 99999999, 99999,
	100, 100, 100, 100, 100, 100, 100, 100, 500, 9999, 9999, 500,
	200, 200, 199, 199, 199, 150, 200, 300, 300, 300, 999, 999,
	999999, 99999999, 99999999, 999, 99999, 99999999, 999, 999, 999, 999, 999, 29999,
	29999, 29999, 29999, 9999, 499, 499, 499, 999, 200, 499, 9999, 150,
	200, 100, 99999999, 999, 999, 999, 999, 999, 999, 300, 300, 300,
	300, 300, 300, 16, 300, 300, 300, 300, 300, 300, 300, 300,
	300, 29999, 29999, 29999, 29999, 29999, 29999, 29999, 29999, 29999, 29999, 29999,
	29999, 29999, 29999, 29999, 29999, 29999, 29999, 999, 999, 99999999, 300, 300,
	300, 99999999, 99999, 300, 200, 90, 90, 36000, 200, 299, 300, 300,
	100, 100, 200, 9999, 200, 9999, 9999, 9999, 100, 99999999, 36000, 36000,
	36000, 100, 340, 340, 1000, 100, 200, 300, 300, 99999999, 99999999, 99999999,
	200, 200, 200, 100, 3000, 9999, 1000, 1000, 100, 99999999, 99999999, 9999,
	1000, 99999999, 99999999, 99999999, 99999999, 99999999, 99999999, 99999999, 99999999, 99999999, 4000, 4000,
	4000, 300, 99999999, 100, 100, 300, 200, 60, 99999, 99999, 19999, 200,
	200, 50, 29999, 99999, 200, 200, 200, 200, 100, 99999999, 99999999, 200,
	200, 200, 250, 200, 200, 200, 250, 150, 200, 200, 99999, 99999999,
	99999999, 99999999, 99999999, 200, 200, 200, 200, 100, 29999, 29999, 200, 200,
	200, 300, 300, 300, 300, 50, 50, 100, 100, 100, 99999999, 99999999,
	99999, 99999, 99999, 99999, 99999999, 100, 14, 99999999, 200, 200, 200, 200,
	300, 3000, 50, 100, 200, 3000, 100, 300, 300, 60, 9999, 30,
	100, 500, 35, 340, 340, 60, 999, 100, 250, 99999, 60, 1500,
	100, 100, 35, 100, 100, 100, 200, 300, 100, 100, 50, 300,
	300, 300, 300, 300, 300, 300, 99999999, 99999999, 99999999, 99999999, 99999999, 99999999,
	99999999, 99999999, 99999999, 99999999, 99999999, 99999, 99999, 200, 400, 400, 400, 100,
	100, 120, 499, 300, 200, 250, 250, 250, 120, 100, 200, 500,
	999999, 200, 100, 100, 100, 100, 400, 10, 300, 3000, 300, 99999999,
	99999999, 29999, 200, 100, 499, 120, 2000, 300, 300, 300, 300, 300,
	300, 300, 300, 300, 300, 300, 300, 300, 300, 200, 120, 300,
	50, 300, 200, 500, 99999999, 200, 200, 200, 60, 60, 60, 100,
	100, 200, 200, 200, 50, 99999999, 99999999, 99999999, 99999999, 2000, 99999999, 200,
	60, 100, 100, 100, 100, 60, 100, 100, 300, 300, 300, 300,
	100, 100, 100, 100, 100, 180, 200, 50, 100, 100, 99999999, 100,
	100, 100, 200, 200, 99999999, 100, 100, 100, 100, 100, 100, 200,
	200, 999999, 60, 50, 99999, 2, 100, 100, 100, 100, 100, 100,
	100, 200, 200, 200, 200, 9999, 300, 250, 29999, 200, 200, 300,
	200, 60, 60, 300, 200, 999999, 100, 100, 100, 100, 23, 100,
	100, 200, 23, 23, 23, 23, 23, 100, 100, 100, 100, 100,
	100, 100, 99999999, 99999999, 50, 50, 50, 99999999, 40, 50, 50, 50,
	50, 50, 50, 50, 50, 200, 100, 99999999, 28, 70, 28, 29999,
	29999, 99999999, 29999, 200, 300, 50, 99999999, 99999999, 99999999, 36000, 100, 100,
	100, 100, 100, 99999999, 99999999, 99999999, 99999999, 99999999, 99999999, 99999999, 99999999, 99999999,
	99999999, 99999999, 99999999, 100, 36000, 99999999, 99999999, 99999999, 40, 1200, 1500, 1800,
	3600, 9999, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
};
CEffectPrim* g_prim;

extern BOOL g_isSkipFrame;




const int BM_RGB           = 0;
const int BL_LIGHT_BODY    = 0x01;
const int BL_LIGHT_SWORD   = 0x02;
const int BL_SPARK_SWORD   = 0x04;
const int BL_BABY_BODY     = 0x10;
const int BL_DOUBLE_BODY   = 0x08;
const int BL_PRESSED       = 0x20;
const int BL_SPINED        = 0x40;
const int BL_KICKED        = 0x80;
const int BL_HIT           = 0x100;
const int BL_REFLECT_BODY  = 0x200;
const int BL_GIANT         = 0x400;
const int BL_ASURA         = 0x800;
const int BL_4WAY          = 0x1000;
const int BL_QUAKE         = 0x2000;
const int BL_HIT_BLUE      = 0x4000;
const int BL_RUNNING       = 0x8000;
const int BL_HIGHJUMP      = 0x10000;
const int BL_LANDJUMP      = 0x20000;
const int BL_HIT_RED       = 0x40000;
const int BL_UNDEAD        = 0x80000;

enum {
	EAKT_POS = 0,
};

enum {
	ANIT_STOP   = 0,
	ANIT_INTP   = 1,
	ANIT_ONCE   = 2,
	ANIT_LOOP   = 3,
	ANIT_RLOOP  = 4,
	ANIT_BILOOP = 5,
};

CRagEffect :: CRagEffect()
{
	m_master = NULL;
	m_count = 0;
	m_stateCnt = 0;
	m_type = 0;
	m_level = 1;
	m_flag = 0;

	m_defZ = FALSE;
	m_isDirection = FALSE;
	m_deltaPos.Set(0, 0, 0);
	m_deltaPos2.Set(0, 0, 0);
	m_deltaPos3.Set(0, 0, 0);

	m_ezEffectRes = NULL;
	m_aniClips = NULL;
	m_iCurLayer = 0;
	ZeroMemory(m_aiCurAniKey, sizeof(int) * 128);
	ZeroMemory(m_rp, sizeof(m_rp));
}

CRagEffect :: ~CRagEffect()
{
	if (m_primList.size() != 0) {
		std::list<CEffectPrim *>::iterator it = m_primList.begin();
		while (it != m_primList.end()) {
			(*it)->SendMsg(NULL, AM_MASTER_DEAD, 0, 0, 0);
			it++;
		}
		m_primList.erase(m_primList.begin(), m_primList.end());
	}

	if (m_master)
		m_master->SendMsg(this, AM_EFFECT_DEAD, 0, 0, 0);

	if (m_ezEffectRes)
		m_ezEffectRes->Unlock();
}

bool	CRagEffect :: OnProcess2()
{
	switch (m_type)
	{
		case	EF_DEMONSTRATION:				Demonstration();						break;
		case	EF_TELEPORTATION2:
				TELEPORTATION2("effect\\Magic_Violet.tga",0);
				break;
		case	EF_PHARMACY_OK:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\p_success.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				if (!EZ2STROnlyEffect())	return false;
				break;
		case	EF_PHARMACY_FAIL:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\p_failed.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				if (!EZ2STROnlyEffect())	return false;
				break;

		case	EF_WATERFALL:
				{
					WaterFall("",0,0,0);
					for (int i=0;i<40;i++)
						WaterFallParticle(0,0);
				}
				break;
		case	EF_WATERFALL_90:
				{
					WaterFall("",1,0,0);
					for (int i=0;i<40;i++)
						WaterFallParticle(1,0);
				}
				break;
		case	EF_WATERFALL_SMALL:
				{
					WaterFall("",0,1,0);
					for (int i=0;i<20;i++)
						WaterFallParticle(0,1);
				}
				break;
		case	EF_WATERFALL_SMALL_90:
				{
					WaterFall("",1,1,0);
					for (int i=0;i<20;i++)
						WaterFallParticle(1,1);
				}
				break;
		case	EF_WATERFALL_T2:
				{
					WaterFall("",0,0,1);
					for (int i=0;i<40;i++)
						WaterFallParticle(0,0);
				}
				break;
		case	EF_WATERFALL_T2_90:
				{
					WaterFall("",1,0,1);
					for (int i=0;i<40;i++)
						WaterFallParticle(1,0);
				}
				break;
		case	EF_WATERFALL_SMALL_T2:
				{
					WaterFall("",0,1,1);
					for (int i=0;i<20;i++)
						WaterFallParticle(0,1);
				}
				break;
		case	EF_WATERFALL_SMALL_T2_90:
				{
					WaterFall("",1,1,1);
					for (int i=0;i<20;i++)
						WaterFallParticle(1,1);
				}
				break;

		case	EF_FORESTLIGHT:
				ForestLight("effect\\cloud11.tga",0);
				break;
		case	EF_FORESTLIGHT2:
				ForestLight("effect\\cloud11.tga",1);
				break;
		case	EF_FORESTLIGHT3:
				ForestLight("effect\\cloud11.tga",2);
				break;
		case	EF_FORESTLIGHT4:
				ForestLight("effect\\cloud11.tga",10);
				break;

		case	EF_THROWITEM3:				ThrowItem("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xb5\xb9.bmp", 2);		break;

		case	EF_FIRSTAID:				FirstAid();			break;
		case	EF_SPRINKLESAND:
				{
					for (int i=0;i<20;i++)
						PARTICLE_UP();
				}
				break;
		case	EF_LOUD:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xb0\xed\xbc\xba\xb9\xe6\xb0\xa1.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}

					if (!EZ2STROnlyEffect())	return false;
					break;
				}
		case	EF_HEAL:					Heal("effect\\alpha_down.tga",0);				break;
		case	EF_HEAL2:					Heal2("effect\\ring_white.tga",0);				break;
		case	EF_EXIT2:					Exit2("effect\\ring_purple.tga");				break;
		case	EF_GLASSWALL2:			if (!GlassWall2("effect\\alpha_down.tga"))	return false;		break;
		case	EF_READYPORTAL2:			ReadyPortal2();		break;
		case	EF_PORTAL2:					Portal2(0);			break;
		case	EF_BOTTOM_MAG:			Bottom_Magnus("effect\\ring_red.tga",0);				break;
		case	EF_BOTTOM_SANC:			Bottom_Magnus("effect\\alpha_down.tga",1);			break;
		case	EF_HEAL3:					Heal("effect\\alpha_down.tga",1);				break;
		case	EF_WARPZONE2:			WarpZone2("effect\\ring_blue.tga");			break;
		case	EF_HEAL4:					Heal2("effect\\ring_white.tga",1);				break;
		case	EF_FOOT:					Foot("effect\\foot_l_b.tga",0);				break;
		case	EF_FOOT2:					Foot("effect\\foot_r_b.tga",0);				break;
		case	EF_BEGINASURA:		BeginAsura(0);		break;
		case	EF_TRIPLEATTACK:
				TRIPLEATTACK();
				break;
		case	EF_HITLINE:				HitLine(0);				break;
		case	EF_HPTIME:					HealTime("effect\\white02.bmp",0);				break;
		case	EF_SPTIME:					HealTime("effect\\white02.bmp",1);				break;
		case	EF_MAPLE:					Maple();			break;
		case	EF_BLIND:
				BLIND(0);
				break;
		case	EF_POISON:
				POISON(0);
				break;
		case	EF_GUARD:					Guard("effect\\guardK.tga",0);					break;
		case	EF_JOBLVUP50:
				{
					JobLvUp50(0);
					JobLvUp50(45);
				}
				break;
		case	EF_ANGEL2:
				{
					if (m_stateCnt == 0)
						PlayWave("levelup.wav");
					SuperAngel(0,0);
					SuperAngel(1,0);
					SuperAngel(2,0);
					SuperAngel(3,0);

				}
				break;
		case	EF_MAGNUM2:					Magnum2("effect\\ring_yellow.tga");		break;
		case	EF_CALLZONE:
				CALLZONE("effect\\white02.bmp");
				break;
		case	EF_PORTAL3:					Portal2(3);			break;
		case	EF_COUPLECASTING:		CoupleCasting("effect\\ring_red.tga");		break;
		case	EF_HEARTCASTING:
				if (m_stateCnt == 0)
				{
					float cell;

					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					cell = 0.27f;

					HeartCasting("effect\\ring_red.tga",0*cell,37*cell);
					HeartCasting("effect\\ring_red.tga",-13*cell,44*cell);
					HeartCasting("effect\\ring_red.tga",-29*cell,48*cell);
					HeartCasting("effect\\ring_red.tga",-46*cell,42*cell);
					HeartCasting("effect\\ring_red.tga",-56*cell,26*cell);

					HeartCasting("effect\\ring_red.tga",-57*cell,6*cell);
					HeartCasting("effect\\ring_red.tga",-51*cell,-12*cell);
					HeartCasting("effect\\ring_red.tga",-40*cell,-26*cell);
					HeartCasting("effect\\ring_red.tga",-25*cell,-38*cell);
					HeartCasting("effect\\ring_red.tga",-10*cell,-47*cell);

					HeartCasting("effect\\ring_red.tga",0*cell,-60*cell);
					HeartCasting("effect\\ring_red.tga",11*cell,-47*cell);
					HeartCasting("effect\\ring_red.tga",26*cell,-38*cell);
					HeartCasting("effect\\ring_red.tga",41*cell,-26*cell);
					HeartCasting("effect\\ring_red.tga",52*cell,-12*cell);

					HeartCasting("effect\\ring_red.tga",58*cell,6*cell);
					HeartCasting("effect\\ring_red.tga",57*cell,26*cell);
					HeartCasting("effect\\ring_red.tga",47*cell,42*cell);
					HeartCasting("effect\\ring_red.tga",30*cell,48*cell);
					HeartCasting("effect\\ring_red.tga",14*cell,44*cell);
				}
				break;
		case	EF_ENTRY2:					Entry2();			break;
		case	EF_SAINTWING:
				{
					SaintWing("effect\\wing003.bmp");

				}
				break;
		case	EF_SPHEREWIND:
				{
					for (int i=0;i<45;i+=9)
						SphereWind("effect\\blue_ivy.bmp",i,0);
				}
				break;
		case	EF_COLORPAPER:
				{
					for (int i=0;i<50;i++)
						ColorPaper("effect\\white02.bmp");
				}
				break;
		case	EF_LIGHTSPHERE:
				{
					EffectTextureSet("effect\\thunder_center.bmp",0);
					for (int i=0;i<80;i++)
						LightSphere("effect\\white02.bmp");
				}
				break;
		case	EF_GHOST:					Ghost(0);			break;
		case	EF_BAT:						Ghost(1);			break;
		case	EF_BAT2:					Ghost(2);			break;
		case	EF_SOULBREAKER:
				{
					if (m_stateCnt == 20)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xb1\xe2\xb0\xf8\xc6\xf7.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}

					if (m_stateCnt >= 0 && m_stateCnt <= 10)
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;
						m_master->SetArgb(-1, 255, 0, 255);
					}

					SoulBreaker();
				}
				break;
		case	EF_LEVEL99_4:			Level99_3(1);						break;
		case	EF_VALLENTINE:				Vallentine(0);				break;
		case	EF_VALLENTINE2:				Vallentine(1);				break;
		case	EF_PRESSURE:
				{
					if (m_stateCnt == 3)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xc7\xc1\xb7\xb9\xbc\xc5.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					PRESSURE("effect\\cross_old.bmp",0);
					GroundShake("effect\\explosive_1_128.bmp",-28,0);
				}
				break;
		case	EF_BASH3D:
				{
					if (m_stateCnt == 22)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xbc\xbc\xc5\xa9\xb8\xae\xc6\xc4\xc0\xcc\xbd\xba.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}

					for (int i=0;i<5;i++)
						BASH3D("effect\\alpha_center.tga",i,0);

					if (m_stateCnt >= 20 && m_stateCnt <= 40)
					{
						if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
							m_master->m_BodyLight += BL_DOUBLE_BODY;
						m_master->SetArgb(-1, 255, 200, 200);
					}
				}
				break;
		case	EF_POKJUK_SOUND:
				PokJuk_Sound();
				break;
		case	EF_AURABLADE:
#if defined (_UPDATE2004B)
				{
					AuraBladeCasting();
					break;
				}
#else
				{
					ProcessEZ2STR();
					break;
				}
#endif
		case	EF_REDBODY:
				if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
					m_master->m_BodyLight += BL_LIGHT_BODY;
				m_master->SetArgb(-1, 255, 100, 100);
				break;
		case	EF_LKCONCENTRATION:
				{
					if (m_stateCnt == 0)
					{
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\knight_twohandquicken.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					m_master->SetArgb(-1, 255, 255, 160);
					ProcessEZ2STR();
					break;
				}
		case	EF_BOTTOM_GOSPEL:				Bottom_Music("effect\\cross_old.bmp",9);		break;
		case	EF_ANGEL:				if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_DEVIL:				if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_DRAGONSMOKE:				DragonSmoke();		break;
		case	EF_CHEMICALPROTECTION:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("apocalips_attack.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					CHEMICALPROTECTION(30);
					CHEMICALPROTECTION(60);
				}
				break;
		case	EF_CHEMICAL2:
				{
					CHEMICAL2(0);
					CHEMICAL2(1);

					if (m_stateCnt == 44)
					{
						g_modeMgr.GetGameMode()->SendMsg(MM_QUAKE);

						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xbf\xac\xc1\xd6\xba\xd8\xb0\xdd.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}

					if (m_stateCnt >= 20 && m_stateCnt <= 120)
					{
						if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
							m_master->m_BodyLight += BL_DOUBLE_BODY;
						m_master->SetArgb(-1, 255, 255, 150);
					}
				}
				break;
	}

	return true;
}

void	CRagEffect :: SendMsg(CGameObject * sender, int msgid, INT_PTR val1, INT_PTR val2, INT_PTR val3)
{
	switch (msgid) {
		case	AM_CHILD_DEAD:
			{
				if (val1 == 0)
				{
					if (sender == NULL)
						break;
					std::list<CEffectPrim *>::iterator it = m_primList.begin();
					while (it != m_primList.end()) {
						if ((*it) == sender) {
							m_primList.erase(it);
							break;
						}
						it++;
					}
				}
#if defined(BALANCE_PATCH)
				else {
					std::list<CEffectPrim *>::iterator it = m_primList.begin();
					while (it != m_primList.end()) {
						(*it)->SendMsg(NULL, AM_MASTER_DEAD, 0, 0, 0);
						it++;
					}
					m_primList.erase(m_primList.begin(), m_primList.end());
				}
#endif
			}
			break;

		case	AM_SETPARAM:
			{
				float * param = ((float *)val1);
				m_param[0] = param[0];
				m_param[1] = param[1];
				m_param[2] = param[2];
				m_param[3] = param[3];
			}
			break;

		case	AM_MASTER_DEAD:
			m_master = NULL;
			break;

		case	AM_SETCOUNT:
			if (val1 == 65535) {
				if (val2 == EF_TWOHANDQUICKEN) {
					m_pos.x = 200.0f;
					m_pos.y = 200.0f;
					m_pos.z = 0.0f;
				} else if (val2 == EF_OVERTHRUST) {
					m_pos.x = 250.0f;
					m_pos.y = 150.0f;
					m_pos.z = 150.0f;
				} else if (val2 == EF_SPEARQUICKEN) {
					m_pos.x = 200.0f;
					m_pos.y = 200.0f;
					m_pos.z = 0.0f;
				}
#if defined(BALANCE_PATCH)
				else if (val2 == EF_ENERGYCOAT) {
					m_pos.x = 180.0f;
					m_pos.y = 180.0f;
					m_pos.z = 250.0f;
				}
#endif
#ifdef	SECONDJOB_SKILL2
				else if (val2 == EF_FASTMOVE) {
					m_pos.x = 250.0f;
					m_pos.y = 250.0f;
					m_pos.z = 250.0f;
					m_duration = val3;
				}
#endif
				break;
			} else if (val1 == 65534) {
				if (val2 == EF_OVERTHRUST) {
					m_pos.x = -1.0f;
					m_pos.y = -1.0f;
					m_pos.z = -1.0f;
				}
				else if (val2 == EF_TWOHANDQUICKEN)
				{
					m_pos.x = -2.0f;
					m_pos.y = -2.0f;
					m_pos.z = -2.0f;
				}
				break;
			}
			m_count = val1;
			if (m_count > 50)
				m_count = 50;
			break;

		case	AM_SETPOS:
			if (val1)
				m_deltaPos = *((vector3d *)val1);
			if (val2)
				m_deltaPos2 = *((vector3d *)val2);
			break;

		case AM_SET_DURATION:
			{
				SetDuration(val1);
			}
			break;
	}
}

bool	CRagEffect :: OnProcess3()
{
#if 1
	switch (m_type)
	{
		case	EF_HP_DAMAGE:
				if (m_stateCnt >= 10 && m_stateCnt < 34)
				{
					m_master->m_roty += 30.0f;
					if (m_master->m_roty > 360.0f)
						m_master->m_roty -= 360.0f;
				}
				break;
		case	EF_FLOWERLEAF:
				ProcessEZ2STR();
				break;
		case	EF_CLOUD7:
				{
					for (int i=0;i<80;i++)
						Cloud(7);
				}
				break;
		case	EF_CLOUD8:
				{
					for (int i=0;i<80;i++)
						Cloud(8);
				}
				break;
		case	EF_GLOW1:
				EffectTextureSet("effect\\glow01.bmp",14);
				break;
		case	EF_GLOW2:
				EffectTextureSet("effect\\glow02.bmp",14);
				break;
		case	EF_GLOW11:
				EffectTextureSet("effect\\glow11.bmp",14);
				break;
		case	EF_GLOW12:
				EffectTextureSet("effect\\glow12.bmp",14);
				break;
		case	EF_GLOW4:
				{
					Map_MagicZone("effect\\glow04.bmp",5);
					for (int i=2;i<21;i+=2)
						Map_Particle("effect\\mpa3.tga",i,2);
				}
				break;
		case	EF_MAP_GHOST:
				Level99_3(3);
				break;
		case	EF_DUST:
				EffectTextureSet_Animation(0);
				break;
		case	EF_TORCH_RED:
				EffectTextureSet_Animation(1);
				break;
		case	EF_TORCH_GREEN:
				EffectTextureSet_Animation(2);
				break;
		case	EF_TORCH_PURPLE:
				EffectTextureSet_Animation(3);
				break;
		case	EF_MAP_MAGICZONE:
				Map_MagicZone("effect\\mjin2.tga",0);
				Map_MagicZone("effect\\mjin.tga",1);
				Map_Pika("effect\\pikapika2.bmp");
				Map_Aura("effect\\ring_blue.tga");
				break;
		case	EF_MAP_MAGICZONE2:
				{
					Map_MagicZone("effect\\mjin2.tga",2);
					Map_MagicZone("effect\\mjin.tga",3);
					for (int i=2;i<21;i+=2)
						Map_Particle("effect\\mpa3.tga",i);
				}
				break;

		case	EF_MAP_MAGICZONE3:
				Map_MagicZone("effect\\\xb8\xb6\xb9\xfd\xc1\xf8" "1-1.bmp",4);
				Map_MagicZone("effect\\\xb8\xb6\xb9\xfd\xc1\xf8" "1-2.bmp",14);
				Map_MagicZone("effect\\\xb8\xb6\xb9\xfd\xc1\xf8" "1-3.bmp",4);
				break;

		case	EF_MAP_MAGICZONE4:
				Map_MagicZone("effect\\\xb8\xb6\xb9\xfd\xc1\xf8" "2-1.bmp",4);
				Map_MagicZone("effect\\\xb8\xb6\xb9\xfd\xc1\xf8" "2-2.bmp",14);
				Map_MagicZone("effect\\\xb8\xb6\xb9\xfd\xc1\xf8" "2-3.bmp",4);
				break;
		case	EF_POK_LOVE:
				Effect_SPR(32);
				break;
		case	EF_POK_WHITE:
				Effect_SPR(33);
				break;
		case	EF_POK_VALEN:
				Effect_SPR(34);
				break;
		case	EF_POK_BIRTH:
				Effect_SPR(35);
				break;
		case	EF_POK_CHRISTMAS:
				Effect_SPR(36);
				break;
		case	EF_MAPSPHERE:
				EffectTextureSet("effect\\\xb1\xb8\xc3\xbc" "2.bmp",12);
				EffectTextureSet("effect\\\xb1\xb8\xc3\xbc" "3.bmp",13);
				EffectTextureSet("effect\\\xb1\xb8\xc3\xbc" "4.bmp",11);
				EffectTextureSet("effect\\\xb1\xb8\xc3\xbc" "5.bmp",10);
				break;
		case	EF_MAPSPHERE2:
				EffectTextureSet("effect\\\xb1\xb8\xc3\xbc" "12.bmp",12);
				EffectTextureSet("effect\\\xb1\xb8\xc3\xbc" "3.bmp",13);
				EffectTextureSet("effect\\\xb1\xb8\xc3\xbc" "14.bmp",11);
				EffectTextureSet("effect\\\xb1\xb8\xc3\xbc" "15.bmp",10);
				break;
		case	EF_BACKTEXTURE1:
				EffectTextureSet("effect\\white02.bmp",100);
				break;
		case	EF_GREEN99_3:			Level99_3(2);						break;
		case	EF_GREEN99_5:			Level99("effect\\ring_white.tga",1);							break;
		case	EF_GREEN99_6:			Level99_2("effect\\pikapika2.bmp",1);						break;
		case	EF_CRITICALWOUND:
				if (m_stateCnt == 1)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\wideb.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				ProcessEZ2STR();
				break;
		case	EF_NPC_SLOWCAST:
				EffectTextureSet("effect\\blast_mine##clock.bmp",4);
				if (m_stateCnt == 10)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\EF_DecAgility.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				break;
		case	EF_GUARD3:					Guard("effect\\guardK.tga",2);					break;
		case	EF_BOTTOM_EVILLAND:			Bottom_Music("effect\\curse.bmp",6);		break;
		case	EF_BOTTOM_RUNNER:			Bottom_LandProtector("effect\\hanmoon1.tga",3);		break;
		case	EF_BOTTOM_TRANSFER:			Bottom_LandProtector("effect\\hanmoon2.tga",3);		break;
		case	EF_DA_SPACE:
				{
					if ((m_master->m_BodyLight & BL_LIGHT_SWORD) != BL_LIGHT_SWORD)
					{
						m_master->m_BodyLight += BL_LIGHT_SWORD;
						if (m_stateCnt<25)
							m_master->m_BodySin5 = m_stateCnt*8;
						else
							m_master->m_BodySin5 = 200;
					}
					break;
				}
		case	EF_NPC_EARTHQUAKE:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\earth_quake.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				Effect_SPR(31);
				if (m_stateCnt == 0 || m_stateCnt == 35 || m_stateCnt == 70)
					g_modeMgr.GetGameMode()->SendMsg(MM_QUAKE);
				break;
		case	EF_BUBBLE_DROP:
				Bubble_Drop();
				break;
		case	EF_UNDEADBODY:
				{
					{
						if ((m_master->m_BodyLight & BL_UNDEAD) != BL_UNDEAD)
						{
							m_master->m_BodyLight += BL_UNDEAD;
							if (m_stateCnt<200)
								m_master->m_BodySin5 = m_stateCnt;
							else
								m_master->m_BodySin5 = 200;
						}
					}
					break;
				}
		case	EF_UNDEADBODY_DEL:
				{
					if (m_stateCnt < 40)
					{
						if ((m_master->m_BodyLight & BL_UNDEAD) != BL_UNDEAD)
						{
							m_master->m_BodyLight += BL_UNDEAD;
							m_master->m_BodySin5 = 200 - (m_stateCnt*5);
						}
					}
					break;
				}

		case	EF_ISSEN:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xc0\xcf\xbc\xb6.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					Effect_SPR(18);
					break;
				}
		case	EF_KASUMIKIRI:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xbe\xc8\xb0\xb3\xba\xa3\xb1\xe2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					Effect_SPR(17);
					break;
				}
		case	EF_KIRIKAGE:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xb1\xd7\xb8\xb2\xc0\xda\xba\xa3\xb1\xe2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					Effect_SPR(15);
					break;
				}

		case	EF_DAMAGE1:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\EF_hit2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					m_master->SendMsg(NULL, AM_MAKE_NUMBER, 1, 0xffff0000, MET_NUMBER + 100);
				}
				break;
		case	EF_DAMAGE1_2:
				if (m_stateCnt == 0)
				{
					m_master->SendMsg(NULL, AM_MAKE_NUMBER, 1, 0xffff0000, MET_NUMBER + 100);
				}
				break;
		case	EF_DAMAGE1_3:
				if (m_stateCnt == 0)
				{
					m_master->SendMsg(NULL, AM_MAKE_NUMBER, 1, 0xffff00ff, MET_NUMBER + 100);
				}
				break;
		case	EF_SPREADATTACK:
				Effect_SPR(28);
				break;
		case	EF_RAPIDSHOWER:
				{
					if (m_stateCnt == 20)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xb7\xa1\xc7\xc7\xb5\xe5\xbb\xfe\xbf\xf6.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					Effect_SPR(26);
					break;
				}
		case	EF_MAGICALBULLET:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xb8\xc5\xc1\xf6\xc4\xc3\xba\xed\xb8\xb4.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					Effect_SPR(27);
					break;
				}
		case	EF_TRIPLEACTION:
				{
					if (m_stateCnt == 25)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xc6\xae\xb8\xae\xc7\xc3\xbe\xd7\xbc\xc7.wav", 0, 0,  0);
					}
					Effect_SPR(30);
					break;
				}
		case	EF_TRACKING:
				{
					Effect_SPR(29);
					break;
				}
		case	EF_LIGHTNING_S:
				{
					Effect_SPR(21);
					break;
				}
		case	EF_BLIND_S:
				{
					Effect_SPR(22);
					break;
				}
		case	EF_POISON_S:
				{
					Effect_SPR(23);
					break;
				}
		case	EF_FREEZING_S:
				{
					Effect_SPR(24);
					break;
				}
		case	EF_FLARE_S:
				{
					Effect_SPR(25);
					break;
				}
		case	EF_TATAMI:
				{
					Effect_SPR(16);
					break;
				}
		case	EF_TRACKCASTING:
				{
					if (m_stateCnt == 0)
					{



					}
					ProcessEZ2STR();
					break;
				}
		case	EF_BULLSEYE:
				{
					ProcessEZ2STR();
					break;
				}
		case	EF_BAKU:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xc6\xf8\xbf\xb0\xb7\xe6.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					ProcessEZ2STR();
					break;
				}
		case	EF_HYOUSYOURAKU:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xba\xf9\xc1\xa4\xb6\xf4.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					ProcessEZ2STR();
					break;
				}
		case	EF_KAEN:
				{
					Effect_SPR(19);
					break;
				}
		case	EF_DESPERADO:
				{
					if (m_stateCnt == 10)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xb5\xa5\xbd\xba\xc6\xe4\xb6\xf3\xb5\xb5.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					Effect_SPR(20);
					break;
				}
		case	EF_CHOOKGI3:		CHOOKGI(3);			break;
		case	EF_BASH3D5:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xb4\xf5\xbd\xba\xc6\xae\xbc\xa6.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}

					for (int i=0;i<6;i++)
						BASH3D("effect\\alpha_center.tga",i,5);
				}
				break;
		case	EF_RG_COIN3:
				{
					if (m_stateCnt == 21)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xb5\xf0\xbd\xba\xbe\xcf.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}

					RG_COIN2("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xb0\xf1\xb5\xe5\xb7\xe8\xbd\xba.bmp",5.0f);
					RG_COIN2("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xb4\xf5\xb0\xb3\xb8\xae\xbd\xbc.bmp",5.0f);
					RG_COIN2("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xb4\xf5\xba\xed\xbc\xa6\xb0\xc7.bmp",5.0f);
					RG_COIN2("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xb4\xf5\xbd\xba\xc5\xa9.bmp",5.0f);

					break;
				}
		case	EF_STIN5:
				{
					if (m_stateCnt == 1)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\t_\xb9\xd9\xb6\xf7\xb9\xe6\xc3\xe2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}

					STIN5("effect\\270\xb9\xd9\xb6\xf7.tga");

				}
				break;
		case	EF_CHEMICAL4:
				{
					if (m_stateCnt == 28)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xc7\xc7\xbe\xee\xbd\xcc\xbc\xa6.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}

					for (int i=0;i<4;i++)
					{
						CHEMICAL3(20);
						CHEMICAL3(21);
					}
				}
				break;
		case	EF_THUNDERSTORM2:
				ThunderStorm2();
				break;
		case	EF_STIN4:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xc7\xb3\xc0\xce.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}

					for (int i=0;i<m_count;i++)

						STIN4("effect\\270\xb9\xd9\xb6\xf7.tga",i%2);

				}
				break;

		case	EF_THROWITEM6:				ThrowItem("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xba\xa3\xb3\xd1\xb3\xaa\xc0\xcc\xc7\xc1.bmp", 3);		break;
		case	EF_THROWITEM7:				ThrowItem("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xbc\xf6\xb8\xae\xb0\xcb.bmp", 3);		break;
		case	EF_THROWITEM8:				ThrowItem("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xc4\xed\xb3\xaa\xc0\xcc_\xb5\xb6.bmp", 3);		break;
		case	EF_THROWITEM9:				ThrowItem("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xc7\xb3\xb8\xb6_\xbf\xad\xc8\xad.bmp", 3);		break;

		case	EF_THROWITEM10:				ThrowItem("effect\\coin_a.bmp", 3);		break;
		case	EF_BOTTOM_SUITON:		Bottom_Volcano("effect\\ring_blue.tga",14);			break;
		case	EF_HYOUSENSOU:			EarthSpike(2);		break;
		case	EF_KOUENKA:
				{
					Kouenka();
					for (int i=0;i<2;i++)
						BASH3D("effect\\alpha_center.tga",i,2);
				}
				break;
		case	EF_BUNSINJYUTSU:
				MakeBlur2();
				break;
		case	EF_COOKING_OK:
				if (m_stateCnt == 45)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("_heal_effect.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				if (!EZ2STROnlyEffect())
					return false;
				break;
		case	EF_COOKING_FAIL:
				if (m_stateCnt == 50)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("caramel_die.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				if (!EZ2STROnlyEffect())
					return false;
				break;
		case	EF_TEMP_OK:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("_heal_effect.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				EffectTextureSet("effect\\success.bmp",3);
				break;
		case	EF_TEMP_FAIL:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("caramel_die.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				EffectTextureSet("effect\\failed.bmp",3);
				break;
		case	EF_FVOICE:
				CHIMTO(2);
				break;
		case	EF_WINK:
				CHIMTO(1);
				break;
		case	EF_NPC_STOP2:
				NpcStop(2);
				break;
		case	EF_NPC_STOP2_DEL:
				if (m_stateCnt==0)
				{
					m_master->SendMsg(NULL, AM_DELETE_EFFECT, EF_NPC_STOP2);
				}
				break;
		case	EF_FIREHIT2:
			if (!EZ2STROnlyEffect())
				return false;
			break;
		case	EF_SIGHT2:					Sight2();			break;
		case	EF_KAIZEL:

			if (m_stateCnt==15)
			{
				JobLvUp50_2(0);
				JobLvUp50_2(45);
			}
			break;
		case	EF_KAAHI:

			break;
		case	EF_ANGEL3:
				{
					SuperAngel(11);
					SuperAngel(10);
				}
				break;
		case	EF_TWILIGHT1:
				{
					for (int k=0;k<80;k++)
						Twilight(0);
				}
				break;
		case	EF_TWILIGHT2:
				{
					for (int i=0;i<80;i++)
						Twilight(1);
				}
				break;
		case	EF_TWILIGHT3:
				{
					for (int k=0;k<80;k++)
						Twilight(2);
				}
				break;
		case	EF_BLUEBODY:
				{
					if ((m_master->m_BodyLight & BL_HIT_BLUE) != BL_HIT_BLUE)
					{
						if (m_stateCnt<=150)
						{
							m_master->m_BodyLight += BL_HIT_BLUE;
							m_master->m_BodyTime2 = m_stateCnt/3;
						}
					}
					break;
				}
		case	EF_MADNESS_BLUE:
				{
					if (m_stateCnt%4 == 0)
					{
						if ((m_master->m_BodyLight & BL_HIT_BLUE) != BL_HIT_BLUE)
						{
							m_master->m_BodyLight += BL_HIT_BLUE;
							m_master->m_BodyTime2 = 20;
						}
					}
					break;
				}
		case	EF_MADNESS_RED:
				{
					if (m_stateCnt%4 == 0)
					{
						if ((m_master->m_BodyLight & BL_HIT_RED) != BL_HIT_RED)
						{
							m_master->m_BodyLight += BL_HIT_RED;
							m_master->m_BodyTime2 = 20;
						}
					}
					break;
				}
		case	EF_HATED:
				if ( m_stateCnt < 200 )
				{
					if ((m_master->m_BodyLight & BL_HIT_BLUE) != BL_HIT_BLUE)
					{
						m_master->m_BodyLight += BL_HIT_BLUE;
						if (m_stateCnt<=25)
							m_master->m_BodyTime2 = m_stateCnt;
						if (m_stateCnt>150 && m_stateCnt<=175)
							m_master->m_BodyTime2 = 175 - m_stateCnt;
					}
				}
				ParticleTime("effect\\white02.bmp",1);
				break;
		case	EF_HATED2:
				if (m_stateCnt == 10)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

					PlayWave("effect\\T_\xb5\xfb\xb5\xed\xc7\xd1\xb8\xb6\xb9\xfd.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

				}
				ParticleTime("effect\\white02.bmp",3);
				break;
		case	EF_BABYBODY_BACK:
				{
					if (m_stateCnt<=15)
					{
						if ((m_master->m_BodyLight & BL_BABY_BODY) != BL_BABY_BODY)
						{
							m_master->m_BodyLight += BL_BABY_BODY;
							m_master->m_BodySin3 = (45-(m_stateCnt*3));
						}
					}
					break;
				}
		case	EF_THROWITEM4:
				{

					ThrowItem("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xc8\xad\xbf\xb0\xba\xb4.bmp", 10);
					ThrowItem("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xbf\xb0\xbb\xea\xba\xb4.bmp", 11);

				}
				break;

		case	EF_THROWITEM5:				ThrowItem("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xb9\xf6\xbc\xad\xc5\xa9\xc6\xf7\xbc\xc7.bmp",1);	break;

		case	EF_GREENBODY:
			{
				if (m_stateCnt<31)
				{
					if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
					{
						m_master->m_BodyLight += BL_DOUBLE_BODY;
					}

					if (m_stateCnt%2 == 0)
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;
						m_master->SetArgb(-1, 0, 255, 0);
					}
					else
					{
						m_master->SetArgb(-1, 255, 255, 255);
					}
				}
				break;
			}
		case	EF_ACIDDEMON:
				{
					if (m_stateCnt == 5)
						g_modeMgr.GetGameMode()->SendMsg(MM_QUAKE);

					BeginCasting2("effect\\ring_black.tga",33);
				}
				break;

		case	EF_CLOUD4:
				{
					for (int i=0;i<80;i++)
						Cloud(3);
				}
				break;
		case	EF_CLOUD5:
				{
					for (int k=0;k<80;k++)
						Cloud(4);
				}
				break;
		case	EF_CLOUD6:
				{
					for (int i=0;i<80;i++)
						Cloud(5);
				}
				break;

		case	EF_TAROTCARD1:
				EffectTextureSet("effect\\Tarot01.tga",1);
				break;
		case	EF_TAROTCARD2:
				EffectTextureSet("effect\\Tarot02.tga",1);
				break;
		case	EF_TAROTCARD3:
				EffectTextureSet("effect\\Tarot03.tga",1);
				break;
		case	EF_TAROTCARD4:
				EffectTextureSet("effect\\Tarot04.tga",1);
				break;
		case	EF_TAROTCARD5:
				EffectTextureSet("effect\\Tarot05.tga",1);
				break;
		case	EF_TAROTCARD6:
				EffectTextureSet("effect\\Tarot06.tga",1);
				break;
		case	EF_TAROTCARD7:
				EffectTextureSet("effect\\Tarot07.tga",1);
				break;
		case	EF_TAROTCARD8:
				EffectTextureSet("effect\\Tarot08.tga",1);
				break;
		case	EF_TAROTCARD9:
				EffectTextureSet("effect\\Tarot09.tga",1);
				break;
		case	EF_TAROTCARD10:
				EffectTextureSet("effect\\Tarot10.tga",1);
				break;
		case	EF_TAROTCARD11:
				EffectTextureSet("effect\\Tarot11.tga",1);
				break;
		case	EF_TAROTCARD12:
				EffectTextureSet("effect\\Tarot12.tga",1);
				break;
		case	EF_TAROTCARD13:
				EffectTextureSet("effect\\Tarot13.tga",1);
				break;
		case	EF_TAROTCARD14:
				EffectTextureSet("effect\\Tarot14.tga",1);
				break;
		case	EF_GRAVITATION:			Gravitation();		break;
		case	EF_DOUBLECASTBODY:
			{
				if (m_stateCnt>=10 && m_stateCnt<51)
				{
					if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
					{
						m_master->m_BodyLight += BL_DOUBLE_BODY;
					}

					if (m_stateCnt%2 == 0)
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;
						m_master->SetArgb(-1, 255, 0, 0);
					}
					else
					{
						m_master->SetArgb(-1, 255, 255, 255);
					}
				}
				break;
			}
		case	EF_CARTTER:
				{
					if (m_stateCnt == 30)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\wizard_fire_pillar_b.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					for (int i=0;i<4;i++)
						ParticleSpread("effect\\whitelight.tga");
					break;
				}
		case	EF_BOTTOM_HERMODE:			Bottom_Hermode("effect\\white02.bmp");	break;
		case	EF_CHEMICAL2DASH:
				{
					for (int i=0;i<4;i++)
					{
						CHEMICAL3(10);
						CHEMICAL3(11);
					}

					if (m_stateCnt == 44)
					{
						g_modeMgr.GetGameMode()->SendMsg(MM_QUAKE);

						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xbf\xac\xc1\xd6\xba\xd8\xb0\xdd.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}

					if (m_stateCnt >= 20 && m_stateCnt <= 100)
					{
						if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
							m_master->m_BodyLight += BL_DOUBLE_BODY;
						m_master->SetArgb(-1, 255, 255, 150);
					}
				}
				break;
		case	EF_GROUNDSAMPLE:
				GROUNDSAMPLE();
				break;
		case	EF_GI_EXPLOSION:
			GI_EXPLOSION("effect\\ring_blue.tga");
			break;
		case	EF_BEGINASURA11:			BeginAsura(1);		break;
		case	EF_05VAL:					Vallentine(2);
			break;
		case	EF_ITEMFAST:				Vallentine(3);				break;
		case	EF_MEMORIZE:
				{
					if (m_stateCnt == 0)
					{
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\priest_suffragium.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}

					if (m_stateCnt>=10 && m_stateCnt<36)
					{
						if (m_stateCnt%2 == 0)
						{
							if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
								m_master->m_BodyLight += BL_LIGHT_BODY;
							m_master->SetArgb(-1, 250, 250, 100);
						}
						else
						{
							m_master->SetArgb(-1, 255, 255, 255);
						}
					}
					break;
				}
		case	EF_SHRINK:
				{
					if (m_stateCnt == 5)
					{
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}

					if (m_stateCnt>=5 && m_stateCnt<26)
					{
						if (m_stateCnt%2 == 0)
						{
							if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
								m_master->m_BodyLight += BL_LIGHT_BODY;
							m_master->SetArgb(-1, 250, 250, 100);
						}
						else
						{
							m_master->SetArgb(-1, 255, 255, 255);
						}
					}
					break;
				}
		case	EF_CHOOKGI2:		CHOOKGI(1);			break;

		case	EF_SLIM:
				{

					PRESSURE("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\\xb7\xb9\xb5\xe5\xbd\xbd\xb8\xb2\xc6\xf7\xbc\xc7.bmp",1);
					GroundShake("effect\\bbbb.bmp",-16,1);
				}
				break;
		case	EF_SLIM2:
				{

					PRESSURE("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\\xbf\xbb\xb7\xce\xbf\xec\xbd\xbd\xb8\xb2\xc6\xf7\xbc\xc7.bmp",2);

					GroundShake("effect\\bbbb.bmp",-16,2);
				}
				break;
		case	EF_SLIM3:
				{

					PRESSURE("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\\xc8\xad\xc0\xcc\xc6\xae\xbd\xbd\xb8\xb2\xc6\xf7\xbc\xc7.bmp",3);

					GroundShake("effect\\bbbb.bmp",-16);
				}
				break;

		case	EF_SOULLINK:
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

					if (m_stateCnt == 0)
						PlayWave("effect\\T_\xc4\xb3\xbd\xba\xc6\xc3.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					if (m_stateCnt == 145)
						PlayWave("effect\\T_\xbf\xb5\xc8\xa5.wav", m_pos.x-pPos.x, -50,  m_pos.z-pPos.z);

					SoulLinkLight("effect\\whitelight.tga");
					BeginAsura2(0);
				}
				break;
		case	EF_SOULLIGHT:
				{
					SoulParticle("effect\\whitelight.tga");
				}
				break;
		case	EF_CASTSPIN:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}

					if (m_master->m_stateId == ST_STAND)
					{
						if (m_stateCnt <= 15)
						{
							m_master->m_roty += 12;
							if (m_master->m_roty > 360)
								m_master->m_roty -= 360;
						}
						else if (m_stateCnt <= 30)
						{
							m_master->m_roty += 18;
							if (m_master->m_roty > 360)
								m_master->m_roty -= 360;
						}
						else if (m_stateCnt <= 40)
						{
							m_master->m_roty += 24;
							if (m_master->m_roty > 360)
								m_master->m_roty -= 360;
						}
						else
						{
							m_master->m_roty += 48;
							if (m_master->m_roty > 360)
								m_master->m_roty -= 360;
						}
					}
				}
				break;
		case	EF_PIERCEBODY:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}

					if (m_stateCnt>=0 && m_stateCnt<141)
					{
						if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
						{
							m_master->m_BodyLight += BL_DOUBLE_BODY;
						}

						if (m_stateCnt%2 == 0)
						{
							if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
								m_master->m_BodyLight += BL_LIGHT_BODY;
							m_master->SetArgb(-1, 250, 250, 100);
						}
						else
						{
							m_master->SetArgb(-1, 255, 255, 255);
						}
					}
				}
				break;
		case	EF_GUARD2:
				{
					Guard2("effect\\a01.bmp");

					if (m_stateCnt>=10 && m_stateCnt<31)
					{
						if (m_stateCnt%2 == 0)
						{
							if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
								m_master->m_BodyLight += BL_LIGHT_BODY;
							m_master->SetArgb(-1, 250, 250, 250);
						}
						else
						{
							m_master->SetArgb(-1, 255, 255, 255);
						}
					}
				}
				break;
		case	EF_RG_COIN2 :
				{
					if (m_stateCnt == 1 || m_stateCnt == 11 || m_stateCnt == 21 || m_stateCnt == 31)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xba\xb9\xc8\xa3\xb0\xdd.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					for (int i=0;i<4;i++)
					{
						RG_COIN2("effect\\shield.bmp",5.0f);
						RG_COIN2("effect\\sword.bmp",9.0f);
					}
					break;
				}

		case	EF_SHIELDBOOMERANG2:		ShieldBoomerang("effect\\\xc5\xe4\xb8\xb6.bmp");	break;

		case	EF_SHIELDBOOMERANG3:
				{
					ShieldBoomerang3("effect\\shield_boomerang.bmp");
				}
				break;
		case	EF_EDP:
			{
				EnchantPoison2();

				if (m_stateCnt<81)
				{
					if (m_stateCnt%2 == 0)
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;
						m_master->SetArgb(-1, 255, 0, 255);
					}
					else
					{
						m_master->SetArgb(-1, 255, 255, 255);
					}
				}

				break;
			}
		case	EF_CHEMICALBODY:
			{
				if (m_stateCnt<31)
				{
					if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
					{
						m_master->m_BodyLight += BL_DOUBLE_BODY;
					}

					if (m_stateCnt%2 == 0)
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;
						m_master->SetArgb(-1, 0, 0, 255);
					}
					else
					{
						m_master->SetArgb(-1, 255, 255, 255);
					}
				}
				break;
			}
		case	EF_MOCHI:
		case	EF_LAMADAN:
		case	EF_MAPAE:
		case	EF_ITEMPOKJUK:
				ProcessEZ2STR();
				break;
		case	EF_BLUEFALL:
				BlueFall(0,0,0);
				break;
		case	EF_BLUEFALL_90:
				BlueFall(1,0,0);
				break;
		case	EF_FASTBLUEFALL:
				BlueFall(0,1,0);
				break;
		case	EF_FASTBLUEFALL_90:
				BlueFall(1,1,0);
				break;
		case	EF_BIG_PORTAL:
				Portal3();
				break;
		case	EF_BIG_PORTAL2:
				Portal3(1);
				break;
		case	EF_SCREEN_QUAKE:
				if ( m_stateCnt < 100 && m_stateCnt%5 == 0 )
					g_modeMgr.GetGameMode()->SendMsg(MM_QUAKE);
				break;
		case	EF_HITTEXTURE:
				EffectTextureSet("effect\\freeze_a.bmp",2);
				break;
		case	EF_STIN3:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

					PlayWave("effect\\T_\xbf\xa1\xb3\xca\xc1\xf6\xb9\xe6\xc3\xe2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

				}
				SMA(1);
				break;
		case	EF_SMA:
				SMA();
				break;
		case	EF_SMA2:
				SMA2("effect\\ring_blue.tga");
				break;
		case	EF_SMA3:

				ParticlePath("effect\\thunder_ball_0001.bmp",1);
				break;
		case	EF_QUAKEBODY3:
				{
					if ( m_stateCnt > 30 && m_stateCnt <= 50 )
					{
						m_master->m_BodyLight |= BL_QUAKE;
						m_master->m_BodyTime = m_stateCnt;
					}
					break;
				}
		case	EF_QUAKEBODY4:
				{
					if ( m_stateCnt == 20 )
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\ef_hit6.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}

					if ( m_stateCnt > 20 )
					{
						m_master->m_BodyLight |= BL_QUAKE;
						m_master->m_BodyTime = m_stateCnt;

						if (m_stateCnt%2 == 0)
							m_master->SetArgb(-1, 250, 50, 50);
						else
							m_master->SetArgb(-1, 255, 255, 255);
					}
					break;
				}
		case	EF_RED_HIT:
				{
					if (m_stateCnt<=16)
					{
						m_master->m_BodyLight |= BL_HIT_RED;
						m_master->m_BodyTime2 = m_stateCnt*3;
					}
					break;
				}
		case	EF_BLUE_HIT:
				{
					if (m_stateCnt<=16)
					{
						m_master->m_BodyLight |= BL_HIT_BLUE;
						m_master->m_BodyTime2 = m_stateCnt*3;
					}
					break;
				}
		case	EF_REDLIGHTBODY:
				{
					{
						if ((m_master->m_BodyLight & BL_HIT_RED) != BL_HIT_RED)
						{
							m_master->m_BodyLight += BL_HIT_RED;
							if (m_stateCnt<=200)
								m_master->m_BodyTime2 = m_stateCnt/8.0f;
							else
								m_master->m_BodyTime2 = 25;
						}
					}
					break;
				}
		case	EF_RO2YEAR:					Vallentine(4);				break;
		case	EF_SMA_READY:
				ParticleTime("effect\\white02.bmp",2);
				break;
		case	EF_STIN:
				{
					if (m_stateCnt == 10)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\T_\xb9\xd9\xb6\xf7\xb8\xb6\xb9\xfd.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}



					STIN("effect\\270\xb9\xd9\xb6\xf7.tga");

				}
				break;
		case	EF_STIN2:
				if (m_stateCnt == 5 || m_stateCnt == 20 || m_stateCnt == 35 || m_stateCnt == 50 || m_stateCnt == 65)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

					if (m_stateCnt<=50)
						PlayWave("effect\\T_\xb3\xaf\xb6\xf3\xc2\xf7\xb1\xe2.wav", 0, -m_stateCnt+5, 0);
					else
						PlayWave("effect\\T_\xb3\xaf\xb6\xf3\xc2\xf7\xb1\xe2.wav", 0, -50, 0);

				}

				STIN2("effect\\270\xb9\xd9\xb6\xf7.tga",0);
				STIN2("effect\\270\xb9\xd9\xb6\xf7.tga",1);

				break;
		case	EF_HOMUNCASTING:		CoupleCasting("effect\\ring_red.tga");		break;
		case	EF_HFLIMOON1:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\h_moonlight_1.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				if (!EZ2STROnlyEffect())
					return false;
				break;
		case	EF_HFLIMOON2:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\h_moonlight_2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				if (!EZ2STROnlyEffect())
					return false;
				break;
		case	EF_HFLIMOON3:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\h_moonlight_3.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				if (!EZ2STROnlyEffect())
					return false;
				break;
		case	EF_HO_UP:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("levelup.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				if (!EZ2STROnlyEffect())
					return false;
				break;

		case	EF_CRYSTAL_BLUE:
				POISON(2);
				break;

		case	EF_FOOD01:
		case	EF_FOOD02:
		case	EF_FOOD03:
		case	EF_FOOD04:
		case	EF_FOOD05:
		case	EF_FOOD06:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("_heal_effect.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				if (!EZ2STROnlyEffect())
					return false;
				break;
		case	EF_HAMIDEFENCE:
				if (!EZ2STROnlyEffect())
					return false;
				break;
		case	EF_HAMICASTLE:
				Effect_SPR(0);
				break;
		case	EF_HAMIBLOOD:
				Effect_SPR(1);
				break;

		case	EF_ITEM_THUNDER:
				Effect_SPR(2);
				break;
		case	EF_ITEM_CLOUD:
				Effect_SPR(3);
				break;
		case	EF_ITEM_CURSE:
				Effect_SPR(4);
				break;
		case	EF_ITEM_ZZZ:
				Effect_SPR(5);
				break;
		case	EF_ITEM_RAIN:
				Effect_SPR(6);
				break;
		case	EF_ITEM_LIGHT:
				ForestLight("effect\\cloud11.tga",11);
				break;

		case	EF_M01:
				Effect_SPR(7);
				break;
		case	EF_M02:
				if (m_stateCnt == 5)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

					PlayWave("effect\\\xc7\xae\xb9\xf6\xbd\xba\xc5\xcd.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

				}
				Effect_SPR(8);
				break;
		case	EF_M03:
				Effect_SPR(9);
				break;
		case	EF_M04:
				Effect_SPR(10);
				break;
		case	EF_M05:
				Effect_SPR(11);
				break;
		case	EF_M06:
				Effect_SPR(12);
				break;
		case	EF_M07:
				Effect_SPR(13);
				break;
		case	EF_HAPGYEOK:
				Effect_SPR(14);
				ProcessEZ2STR();
				break;
		case	EF_DRAGONFEAR:
				if (m_stateCnt == 1)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\dragonfear.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					CGameMode * gameMode = g_modeMgr.GetGameMode();
					gameMode->SendMsg(MM_QUAKE);
				}
				ProcessEZ2STR();
				break;
		case	EF_WIDECONFUSE:
				ProcessEZ2STR();
				break;
		case	EF_BLEEDING:
				if (m_stateCnt == 1)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\wideb.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				POISON(1);
				ProcessEZ2STR();
				break;
	}

	return true;
#endif
	return 0;
}


bool	CRagEffect :: OnProcess()
{
	if (m_master == NULL)
		return false;

	switch (m_type)
	{
		case	EF_CHOOKGI:
		case	EF_CHOOKGI2:
		case	EF_CHOOKGI3:
		case	EF_LEVEL99:
		case	EF_LEVEL99_2:
		case	EF_LEVEL99_3:
		case	EF_LEVEL99_4:
		case	EF_LEVEL99_5:
		case	EF_FIREWALL:
		case	EF_FIREHIT:
		case	EF_FIREHIT2:
		case	EF_HALFSPHERE:
		case	EF_COLDHIT:
		case	EF_WINDHIT:
		case	EF_POISONHIT:
		case	EF_HEAL:

		case	EF_TAROTCARD1:
		case	EF_TAROTCARD2:
		case	EF_TAROTCARD3:
		case	EF_TAROTCARD4:
		case	EF_TAROTCARD5:
		case	EF_TAROTCARD6:
		case	EF_TAROTCARD7:
		case	EF_TAROTCARD8:
		case	EF_TAROTCARD9:
		case	EF_TAROTCARD10:
		case	EF_TAROTCARD11:
		case	EF_TAROTCARD12:
		case	EF_TAROTCARD13:
		case	EF_TAROTCARD14:

		case	EF_DOUBLEGUMGANG:
		case	EF_ATTACKENERGY:
		case	EF_ATTACKENERGY2:
		case	EF_HEAL2:
		case	EF_HEAL3:
		case	EF_HEAL4:
		case	EF_EARTHHIT:
		case	EF_HOLYHIT:
		case	EF_LEVEL99_6:
		case	EF_GUMGANG:
		case	EF_STEELBODY:
		case	EF_BEGINASURA:
		case	EF_BEGINASURA1:
		case	EF_BEGINASURA2:
		case	EF_BEGINASURA3:
		case	EF_BEGINASURA4:
		case	EF_BEGINASURA5:
		case	EF_BEGINASURA6:
		case	EF_BEGINASURA7:
		case	EF_BLUECASTING:
		case	EF_DARKCASTING:
		case	EF_FLOWERCAST:
		case	EF_FLOWERCAST2:
		case	EF_FLOWERCAST3:
		case	EF_SOULLINK:
		case	EF_SOULLIGHT:
		case	EF_INCAGILITY:
		case	EF_KAIZEL:
		case	EF_GREEN99_3:
		case	EF_GREEN99_5:
		case	EF_GREEN99_6:
			break;
		default:
			if ( g_isSkipFrame == 0 )
				return true;
			break;
		case	EF_STATUS_STATE:
			if ( g_isSkipFrame == 0 )
				return true;
			break;
	}

	if ( g_isSkipFrame > 0 )
	{
		if (m_master->m_BeZero == 1)
		{
			m_master->m_BodyLight = 0;
			m_master->m_BeZero = 0;
		}
	}

	if (m_type >= EF_CHEMICALPROTECTION && m_type <= EF_DRAGONSMOKE)
		OnProcess2();
	else if (m_type >= EF_MOCHI)
		OnProcess3();
	else
	{
		switch (m_type)
		{
		case	EF_HIT1:					Hit1();				break;
		case	EF_HIT2:					Hit2(0);				break;
		case	EF_HIT3:					Hit3();				break;
		case	EF_HIT4:					Hit4();				break;
		case	EF_HIT5:					Hit5();				break;
		case	EF_HIT6:					Hit6();				break;
		case	EF_ENTRY:					Entry();			break;
		case	EF_EXIT:					Exit();				break;
		case	EF_ABSORBSPIRITS:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xc8\xed\xb1\xe2.wav", m_pos.x-pPos.x, -80,  m_pos.z-pPos.z);
					}
					Heal2("effect\\ring_blue.tga",2);
				}
				break;
		case	EF_HALFSPHERE:
				HalfSphere("effect\\white02.bmp");
				break;
		case	EF_ATTACKENERGY:
				HalfSphere("effect\\white02.bmp");
				AttackEnergy("effect\\ring_blue.tga");
				break;
		case	EF_ATTACKENERGY2:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

					PlayWave("effect\\T_\xb8\xb6\xb9\xfd\xb9\xdd\xbb\xe7.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}

				AttackEnergy2("effect\\cloud11.tga");

				break;
		case	EF_SPHEREWIND2:
				{
					for (int i=0;i<45;i+=9)
						SphereWind("effect\\fire_ivy.bmp",i,1);
				}
				break;
		case	EF_WARP:					Warp();				break;
		case	EF_ENHANCE:					Enhance();			break;
		case	EF_ENDURE:					Endure();			break;
		case	EF_BEGINSPELL:				BeginSpell();		break;
		case	EF_HEALSP:					HealSP();			break;
		case	EF_SOULSTRIKE:				SoulStrike(0);		break;
		case	EF_SOULSTRIKE2:				SoulStrike(1);		break;
		case	EF_BASH:					Bash();				break;
		case	EF_MAGNUMBREAK:				MagnumBreak();		break;
		case	EF_STEAL:					Steal();			break;

		case	EF_PATTACK:					EnchantPoison();	break;
		case	EF_DETOXICATION:			Detoxication();		break;

		case	EF_RUWACH:					Ruwach();			break;
		case	EF_TELEPORTATION:			Teleportation();	break;
		case	EF_READYPORTAL:				ReadyPortal();		break;
		case	EF_PORTAL:					Portal();			break;

		case	EF_SIGHT:					Sight();			break;
		case	EF_FIREBALL:				FireBall();			break;
		case	EF_ICEARROW:				IceArrow();			break;
		case	EF_FROSTDIVER:				FrostDiver();		break;
		case	EF_FROSTDIVER2:				FrostDiver2();		break;
		case	EF_FIREARROW:				FireArrow();		break;
		case	EF_NAPALMBEAT:				NapalmBeat();		break;
		case	EF_NAPALMVALCAN:
				{
					if (m_stateCnt == 20 || m_stateCnt == 30 || m_stateCnt == 40 || m_stateCnt == 50 || m_stateCnt == 60)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\EF_NapalmBeat.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

						Hit2(0);
					}
				}
				break;
		case	EF_PORTAL4:
				{
					PortalWind(0);

					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xc0\xa9\xb5\xe5\xbf\xf6\xc5\xa9.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
					}

					if (m_stateCnt >= 5 && m_stateCnt <= 25)
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;
						m_master->SetArgb(-1, 220, 250, 220);
					}
				}
				break;
		case	EF_PORTAL5:
				{
					PortalWind(1);

					if (m_stateCnt >= 5 && m_stateCnt <= 65)
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;
						m_master->SetArgb(-1, 250, 250, 200);
					}
				}
				break;
		case	EF_INCAGILITY:				IncAgility();		break;
		case	EF_BABY:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\EF_Blessing.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
						for (int i=0;i<45;i+=9)
							SphereWind2("effect\\fire_ivy.bmp",i,0);
				}
				break;
		case	EF_CARTBOOST:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\EF_IncAgility.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					ProcessEZ2STR();
					break;
				}
		case	EF_AGIUP:
				{
					CartBoost();
					break;
				}
		case	EF_SOULBURN:
				{
					ProcessEZ2STR();
					break;
				}
		case	EF_SOULCHANGE:
				{
					ProcessEZ2STR();
					break;
				}
		case	EF_REJECTSWORD:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\kyrie_guard.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
					}

					if (m_stateCnt >= 0 && m_stateCnt < 30 )
					{
						if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
							m_master->m_BodyLight += BL_DOUBLE_BODY;

						if (m_stateCnt%6<3)
							m_master->SetArgb(-1, 100, 100, 100);
						else
							m_master->SetArgb(-1, 250, 250, 250);
					}

					ProcessEZ2STR();
					break;
				}
		case	EF_DECAGILITY:				DecAgility();		break;
		case	EF_AQUA:					Aqua();				break;

		case	EF_BLESSING:				Blessing();			break;

		case	EF_INCAGIDEX:				IncAGIDEX();		break;

		case	EF_SMOKE:					Smoke();			break;
		case	EF_FIREFLY:					FireFly();			break;
		case	EF_SANDWIND:				SandWind();			break;
		case	EF_TORCH:					Torch();			break;
		case	EF_SPRAYPOND:				SprayPond();		break;
		case	EF_BANJJAKII:				Banjjakii();		break;
		case	EF_RAIN:
				break;
		case	EF_SNOW:
#if !defined(INSTANT_EVENT_SERVER)
			Snow();
#endif
			break;
		case	EF_SAKURA:					Sakura();			break;
		case	EF_CLOUD:
				{
					for (int i=0;i<40;i++)
						Cloud(0);
				}
				break;
		case	EF_CLOUD2:
				{
					for (int i=0;i<60;i++)
						Cloud(1);
				}
				break;
		case	EF_CLOUD3:
				{
					for (int i=0;i<40;i++)
						Cloud(2);
				}
				break;
		case	EF_POKJUK:
				PokJuk();
				break;
		case	EF_MAPPILLAR:			MAPPILLAR("effect\\ring_blue.tga",0);			break;
		case	EF_MAPPILLAR3:			MAPPILLAR("effect\\magic_green.tga",2);		break;
		case	EF_MAPPILLAR4:			MAPPILLAR("effect\\ring_red.tga",3);		break;
		case	EF_MAPPILLAR2:			MAPPILLAR("effect\\ring_blue.tga",1);		break;
		case	EF_VOLCANO:			VOLCANO("effect\\ring_red.tga",0);				break;
		case	EF_DELUGE:				VOLCANO("effect\\ring_blue.tga",0);			break;
		case	EF_VIOLENTGALE:		VOLCANO("effect\\magic_green.tga",0);				break;
		case	EF_LANDPROTECTOR:		VOLCANO("effect\\ring_white.tga",0);			break;

		case	EF_GANBANTEIN:			VOLCANO("effect\\ring_white.tga",2);	break;


		case	EF_GUMGANG2:			VOLCANO2("effect\\ring_yellow.tga",0);		break;
		case	EF_GUMGANG3:			VOLCANO("effect\\ring_yellow.tga",1);		break;

		case	EF_FIRESPLASHHIT:		FireSplashHit();							break;
		case	EF_COLDHIT:				ColdHit();									break;

		case	EF_POISONHIT:			PoisonHit();								break;

		case	EF_BEGINSPELL2:		BeginSpell2();		break;
		case	EF_BEGINSPELL3:		BeginSpell3();		break;
		case	EF_BEGINSPELL4:		BeginSpell4();		break;
		case	EF_BEGINSPELL5:		BeginSpell5();		break;
		case	EF_BEGINSPELL6:		BeginSpell6();		break;
		case	EF_BEGINSPELL7:		BeginSpell7();		break;
		case	EF_BLUECASTING:		BlueCasting();		break;
		case	EF_DARKCASTING:		DarkCasting();		break;
		case	EF_FLOWERCAST:
				if (m_stateCnt == 5)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

					PlayWave("effect\\T_\xbe\xc8\xb6\xf4\xc7\xd1\xb8\xb6\xb9\xfd.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);

				}
				if ((m_master->m_BodyLight & BL_ASURA) != BL_ASURA)
				{
					m_master->m_BodyLight += BL_ASURA;
					if (m_stateCnt<200)
						m_master->m_BodySin5 = m_stateCnt;
					else if (m_stateCnt<400)
						m_master->m_BodySin5 = 400-m_stateCnt;
				}
				FlowerCasting("effect\\ring_blue.tga",4,0);
				FlowerCasting("effect\\ring_blue.tga",4,1);
				break;
		case	EF_FLOWERCAST2:
				break;
		case	EF_FLOWERCAST3:

				break;
		case	EF_STOPRUN	:
				break;
		case	EF_RUN:
				if ((m_master->m_BodyLight & BL_RUNNING) != BL_RUNNING)
				{
					m_master->m_BodyLight += BL_RUNNING;
					m_master->m_BodyTime3 = m_stateCnt;
				}

				break;
		case	EF_BEGINASURA1:
			BeginAsura1("effect\\hanmoon1.tga");
			break;
		case	EF_BEGINASURA2:
			BeginAsura1("effect\\hanmoon2.tga");
			break;
		case	EF_BEGINASURA3:
			BeginAsura1("effect\\hanmoon3.tga");
			break;
		case	EF_BEGINASURA4:
			BeginAsura1("effect\\hanmoon4.tga");
			break;
		case	EF_BEGINASURA5:
			BeginAsura1("effect\\hanmoon7.tga");
			break;
		case	EF_BEGINASURA6:
			BeginAsura1("effect\\hanmoon5.tga");
			break;
		case	EF_BEGINASURA7:
			BeginAsura1("effect\\hanmoon6.tga");
			break;
		case	EF_HITLINE3:
				{
					HitLine3(0);
					break;
				}
		case	EF_HITLINE4:
				{
					HitLine4(3);
					break;
				}
		case	EF_HITLINE5:
				{
					HitLine5(3);
					break;
				}
		case	EF_HITLINE6:
				{
					HitLine3(3);
					break;
				}
		case	EF_HITLINE7:
				{
					HitLine3(1);
					break;
				}
		case	EF_STORMKICK:
				{
					if (m_stateCnt == 5)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\T_\xc8\xb8\xbf\xc0\xb8\xae\xc2\xf7\xb1\xe2.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
					}
					StormKick("effect\\storm2.tga",0);
					PortalWind(2);
					PortalWind(3);

					if (m_stateCnt <= 47)
					{
						m_master->m_roty += 30;

						if (m_master->m_roty > 360)
							m_master->m_roty -= 360;
					}
					break;
				}
		case	EF_STORMKICK1:
				{
					StormKick("effect\\storm2.tga",1);
					PortalWind(2);
					PortalWind(3);

					if (m_stateCnt <= 50)
					{
						m_master->m_roty += 14;
						if (m_master->m_roty > 360)
							m_master->m_roty -= 360;
					}
					break;
				}
		case	EF_STORMKICK2:
				{
					StormKick("effect\\storm2.tga",2);
					PortalWind(2);
					PortalWind(3);

					break;
				}
		case	EF_STORMKICK3:
				{
					StormKick("effect\\storm2.tga",3);
					PortalWind(2);
					PortalWind(3);
					break;
				}
		case	EF_STORMKICK4:
				{
					if (m_stateCnt == 15)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\T_\xc8\xb8\xc7\xc7.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);

					}
					for (int i=0;i<30;i++)
					{
						PeongUp("effect\\whitelight.tga",1);
					}
					break;
				}
		case	EF_STORMKICK5:
				{
					if (m_stateCnt == 15)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\T_\xc8\xb8\xc7\xc7.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);

						m_master->m_stateId = ST_NINJAREADY;
						m_master->SetForceAnimation(96,2 , 2, 96, 2, TRUE, 25, FALSE);
					}
					for (int i=0;i<30;i++)
					{
						PeongUp("effect\\whitelight.tga",1);
					}

					break;
				}
		case	EF_STORMKICK6:
				{
					StormKick("effect\\storm2.tga",6);
					PortalWind(2);
					PortalWind(3);

					break;
				}
		case	EF_STORMKICK7:
				{
					StormKick("effect\\storm2.tga",7);
					PortalWind(2);
					PortalWind(3);

					break;
				}
		case	EF_ELECTRIC:
				{
					if (m_stateCnt == 25)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\T_\xc0\xfc\xb1\xe2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					Electric(0,0);
					Electric(0,180);
					break;
				}
		case	EF_ELECTRIC2:
				{
					Electric2(0,0);
					Electric2(0,0);
					break;
				}
		case	EF_HITLINE2:
				{
					HitLine(2);

					if (m_stateCnt >= 10 && m_stateCnt <= 20)
					{
						if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
							m_master->m_BodyLight += BL_DOUBLE_BODY;
						m_master->SetArgb(-1, 255, 120, 50);
					}
				}
				break;
		case	EF_MAGICCRASHER:
				{
					if (m_stateCnt == 25)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xb8\xc5\xc1\xf7 \xc5\xa9\xb7\xa1\xbd\xac.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}

					if (m_stateCnt == 30)
						g_modeMgr.GetGameMode()->SendMsg(MM_QUAKE);

					if (m_stateCnt >= 30 && m_stateCnt <= 60)
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;

						m_master->SetArgb(-1, random(255), random(255), random(255));
					}
				}
				break;
			case	EF_MAGICCRASHER2:
					{
						if (m_stateCnt == 25)
						{
							m_pos = m_master->m_pos;
							vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
							PlayWave("effect\\swordman_provoke.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
						}

					if (m_stateCnt >= 0 && m_stateCnt <= 60)
					{
						m_master->SetArgb(-1, random(255), random(255), random(255));
					}
				}
				break;
		case	EF_FALCONASSAULT:
				{
					if (m_stateCnt == 30)
						g_modeMgr.GetGameMode()->SendMsg(MM_QUAKE);

					if (m_stateCnt >= 30 && m_stateCnt <= 54)
					{
						m_master->m_roty += 30;
						if (m_master->m_roty > 360)
							m_master->m_roty -= 360;
					}

					if (m_stateCnt >= 30 && m_stateCnt <= 50 && (m_stateCnt%10 < 5) )
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;
					}
				}
				break;

		case	EF_CLOAKING:			Cloaking();			break;
		case	EF_SONICBLOW:			SonicBlow();		break;
		case	EF_SONICBLOWHIT:		SonicBlowHit();		break;
		case	EF_GRIMTOOTH:			GrimTooth();		break;
		case	EF_ENCHANTPOISON:		EnchantPoison();	break;
		case	EF_OVERTHRUST:			OverThrust();		break;

		case	EF_LOCKON:				LockOn();			break;
		case	EF_WARPZONE:			WarpZone();			break;
		case	EF_BARRIER:				Barrier();			break;
		case	EF_SIGHTRASHER:			Sightrasher();		break;
		case	EF_CONE:				Cone();				break;
		case	EF_SPHERE:				Sphere();			break;
		case	EF_BOWLINGBASH:			BowlingBash();		break;
		case	EF_ICEWALL:				IceWall();			break;
		case	EF_EARTHSPIKE:			EarthSpike(0);		break;

		case	EF_SPEARBMR:			SpearBmr();			break;
		case	EF_PIERCE:				Pierce();			break;
		case	EF_TURNUNDEAD:			TurnUndead();		break;
		case	EF_YUFITEL:				Yufitel(0);			break;
		case	EF_YUFITELHIT:			YufitelHit(0);		break;
		case	EF_YUFITEL2:			YufitelHit(1);			break;
		case	EF_HASTEUP:				HasteUp();			break;
		case	EF_FLASHER:				Flasher();			break;
		case	EF_REMOVETRAP:			RemoveTrap();		break;
		case	EF_BOTTOM:				Bottom();			break;
		case	EF_BLITZBEAT:			BlitzBeat();		break;
		case	EF_WATERBALL:			WaterBall();		break;
		case	EF_WATERBALL2:			WaterBall2();		break;
		case	EF_FIREIVY:				FireIvy();			break;
		case	EF_DETECTING:			Detecting();		break;

		case	EF_MELTDOWN:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\black_overthrust.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					ProcessEZ2STR();
					break;
				}
		case	EF_TWOHANDQUICKEN:		TwoHandQuicken();	break;
		case	EF_SPEARQUICKEN:		TwoHandQuicken();	break;
		case	EF_GRIMTOOTHATK:		GrimToothAtk();		break;
		case	EF_SLOWPOISON:
				ProcessEZ2STR();
				break;
		case	EF_BOTTOM2:				Bottom2();			break;

		case	EF_BOTTOM_FOGWALL:				Bottom_Magnus("effect\\ring_white.tga",2);			break;
		case	EF_BOTTOM_VO:					Bottom_Volcano("effect\\ring_red.tga");				break;

		case	EF_BOTTOM_DE:					Bottom_Volcano("effect\\ring_blue.tga");			break;
		case	EF_BOTTOM_VI:					Bottom_Volcano("effect\\magic_green.tga");			break;
		case	EF_BOTTOM_LA:					Bottom_LandProtector("effect\\aaa copy.bmp",1);		break;
		case	EF_BOTTOM_DONTFORGETME:			Bottom_Vertical("effect\\magic_green.tga",2);	break;
		case	EF_BOTTOM_FORTUNEKISS:			Bottom_Music("effect\\kiss.bmp",4);				break;
		case	EF_BOTTOM_SERVICEFORYOU:		Bottom_Vertical("effect\\safeline.bmp",4);		break;
		case	EF_BOTTOM_DISSONANCE:			Bottom_Vertical("effect\\ring_blue.tga",0);		break;
		case	EF_BOTTOM_LULLABY:				Bottom_Music("effect\\zz.bmp",1);				break;
		case	EF_BOTTOM_RICHMANKIM:			Bottom_Music("effect\\pocket.bmp",3);			break;
		case	EF_BOTTOM_ETERNALCHAOS:			Bottom_Light("effect\\twirl.bmp",8);			break;
		case	EF_BOTTOM_DRUMBATTLEFIELD:		Bottom_Music("",2);								break;
		case	EF_BOTTOM_RINGNIBELUNGEN:		Bottom_Music("effect\\twirl.bmp",7);			break;
		case	EF_BOTTOM_ROKISWEIL:			Bottom_Out("effect\\safeline.bmp",0);				break;
		case	EF_BOTTOM_INTOABYSS:			Bottom_Music("",5);								break;
		case	EF_BOTTOM_SIEGFRIED:			Bottom_Light("effect\\twirl.bmp",4);			break;
		case	EF_BOTTOM_WHISTLE:				Bottom_Music("effect\\melody_b.bmp",0);			break;
		case	EF_BOTTOM_ASSASSINCROSS:		Bottom_Vertical("effect\\ring_red.tga",1);		break;
		case	EF_BOTTOM_POEMBRAGI:			Bottom_Music("",8);								break;
		case	EF_BOTTOM_APPLEIDUN:			Bottom_Music("effect\\idun_apple.bmp",6);		break;
		case	EF_BOTTOM_UGLYDANCE:			Bottom_Vertical("effect\\ring_red.tga",0);		break;
		case	EF_BOTTOM_HUMMING:				Bottom_Music("effect\\melody_a.bmp",0);			break;
		case	EF_BOTTOM_SPIDER:
				{
					Bottom_LandProtector("effect\\spiderweb.tga",2);
					break;
				}
		case	EF_BOTTOM_BASILICA:
				{
					Basilica("effect\\alpha_down.tga",0);
					Basilica("effect\\alpha_down.tga",1);
					break;
				}
		case	EF_NPC_STOP:
				NpcStop(0);
				break;
		case	EF_TALK_FROSTJOKE:				Talk_FrostJoke(1);				break;
		case	EF_TALK_SCREAM:					Talk_FrostJoke(0);				break;

		case	EF_BLASTMINE:			BlastMine();						break;
		case	EF_FIREPILLARON:		FirePillarOn();						break;
		case	EF_REVIVE:				Revive();							break;
		case	EF_HEAVENSDRIVE:		HeavensDrive();						break;
		case	EF_TEST:				Test();								break;
		case	EF_TOPRANK:				TopRank();							break;
		case	EF_PARTY:				Party();							break;
		case	EF_STATUS_STATE:		StatusState();						break;
		case	EF_MAKEBLUR:			MakeBlur();							break;
		case	EF_LEVEL99:				Level99("effect\\ring_blue.tga",0);							break;
		case	EF_LEVEL99_2:			Level99_2("effect\\pikapika2.bmp",0);						break;
		case	EF_LEVEL99_3:			Level99_3(0);						break;
		case	EF_LEVEL99_5:			Level99("effect\\ring_blue.tga",0);							break;
		case	EF_LEVEL99_6:			Level99_2("effect\\pikapika2.bmp",0);						break;
		case	EF_GUMGANG:				GumGang(1,0);							break;
		case	EF_GUMGANGNPC:			GumGang(2,1);							break;

		case	EF_DOUBLEGUMGANG:
				DoubleGumGang(2,2);
				break;
		case	EF_DOUBLEGUMGANG2:
				DoubleGumGang(2,1);
				break;
		case	EF_DOUBLEGUMGANG3:
				DoubleGumGang(2,0);
				break;
		case	EF_STEELBODY:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

					PlayWave("effect\\mon_\xb1\xdd\xb0\xad\xba\xd2\xb1\xab.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

				}
				GumGang(4,0);
				break;
		case	EF_AURABLADE2:
				if ((m_master->m_BodyLight & BL_LIGHT_SWORD) != BL_LIGHT_SWORD)
					m_master->m_BodyLight += BL_LIGHT_SWORD;
				break;
		case	EF_LIGHTBLADE:
				if ((m_master->m_BodyLight & BL_SPARK_SWORD) != BL_SPARK_SWORD)
					m_master->m_BodyLight += BL_SPARK_SWORD;
				break;
		case	EF_DEFENDER:				DEFENDER("effect\\ring_black.tga");				break;
		case	EF_REFLECTSHIELD:			DEFENDER("effect\\ring_yellow.tga");			break;
		case	EF_WIND:					WIND();											break;
		case	EF_GRANDCROSS:
				{
					GRANDCROSS(0,0);
					GRANDCROSS2(0);
				}
				break;
		case	EF_GRANDCROSS2:
				{
					GRANDCROSS(0,1);
					GRANDCROSS2(1);
				}
				break;
		case	EF_INTIMIDATE:
				{
					if (m_stateCnt  == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\EF_Bash.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					if (m_stateCnt == 35)
					{
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\rog_intimidate.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}

					for (int k=0;k<20;k++)
						INTIMIDATE();
				}
				break;
		case	EF_TEIHIT1:
				{
					for (int i=0;i<20;i++)
						TEIHIT1("effect\\alpha_center.tga",0);
				}
				break;
		case	EF_TEIHIT1X:
				{
					for (int i=0;i<24;i++)
						TEIHIT1("effect\\lens1.tga",1);
				}
				break;
		case	EF_TEIHIT2:
				{
					for (int i=0;i<5;i++)
						TEIHIT2();
				}
				break;

		case	EF_CHIMTO:
				CHIMTO(0);
				break;
		case	EF_TANJI:
				TANJI();
				break;
		case	EF_TANJI2:
				TANJI2();
				break;
		case	EF_BACKSTAP:
				{
					if (m_stateCnt == 20)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\rog_back stap.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					for (int i=0;i<20;i++)
						TEIHIT2();
				}
				break;
		case	EF_BASH3D3:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xc7\xec\xb5\xe5 \xc5\xa9\xb7\xaf\xbd\xac.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}

					for (int i=0;i<6;i++)
						BASH3D("effect\\alpha_center.tga",i,4);

					if (m_stateCnt >= 20 && m_stateCnt <= 50)
					{
						if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
							m_master->m_BodyLight += BL_DOUBLE_BODY;
						m_master->SetArgb(-1, 255, 255, 200);
					}
				}
				break;
		case	EF_BASH3D4:
				{
					if (m_stateCnt == 22)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xba\xf1\xc6\xae \xc1\xb6\xc0\xce\xc6\xae.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}

					for (int i=0;i<6;i++)
						BASH3D("effect\\alpha_center.tga",i,5);

					if (m_stateCnt >= 20 && m_stateCnt <= 50)
					{
						if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
							m_master->m_BodyLight += BL_DOUBLE_BODY;
						m_master->SetArgb(-1, 255, 255, 255);
					}
				}
				break;
		case	EF_TRUESIGHT:
				{
					if (m_stateCnt == 0) {
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\hunter_detecting.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					for (int i=0;i<12;i++)
						BASH3D("effect\\alpha_center.tga",i,3);
				}
				break;
		case	EF_PEONG:
				{
					int i;

					for (i=0;i<30;i++)
					{
						PeongUp("effect\\whitelight.tga",0);
					}

					for (i=0;i<4;i++)
					{
						Peong("effect\\peong1.tga",i*90);
						Peong("effect\\peong2.tga",i*90);
						Peong("effect\\peong3.tga",i*90);
						Peong("effect\\peong2.tga",i*90);
					}
				}
				break;
		case	EF_BASH3D2:
				{
					if (m_stateCnt == 5)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\mon_\xb8\xcd\xb7\xe6\xb0\xfa\xb0\xad.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}

					for (int i=0;i<8;i++)
						BASH3D("effect\\alpha_center.tga",i,2);

					if (m_stateCnt >= 5 && m_stateCnt <= 35)
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;
					}
				}
				break;

		case	EF_ASSUMPTIO:
				{
					if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
					{
						m_master->m_BodyLight += BL_DOUBLE_BODY;
					}
				}
				break;
		case	EF_ASSUMPTIO2:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xbe\xc6\xbc\xfb\xc7\xc1\xc6\xbc\xbf\xc0.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}

					ProcessEZ2STR();
				}
				break;
		case	EF_SOULBREAKER2:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xb8\xde\xc5\xd7\xbf\xc0 \xbe\xee\xbd\xe4\xc6\xae.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}

					if (m_stateCnt >= 0 && m_stateCnt <= 10)
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;
						m_master->SetArgb(-1, 255, 0, 255);
					}

					for (int i=0;i<360;i+=45)
						SoulBreaker2(i);
				}
				break;
		case	EF_TEIHIT3:
				{
					for (int i=0;i<20;i++)
						TEIHIT3("effect\\lens2.tga",2);
				}
				break;
		case	EF_TRIPLEATTACK2:
				{
					if (m_stateCnt == 30)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xbb\xfe\xc7\xc1\xbd\xb4\xc6\xc3.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					TRIPLEATTACK2();
				}
				break;
		case	EF_TRIPLEATTACK3:
				{
					if (m_stateCnt == 30)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xbe\xd6\xb7\xce\xbf\xec \xb9\xdf\xc4\xad.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					TRIPLEATTACK3();
				}
				break;

		case	EF_CHEMICAL3:
				{
					for (int i=0;i<4;i++)
					{
						CHEMICAL3(0);
						CHEMICAL3(1);
					}
				}
				break;
		case	EF_DEVIL1:
				BLIND(1);
				break;
		case	EF_DEVIL2:
				BLIND(2);
				break;
		case	EF_DEVIL3:
				BLIND(3);
				break;
		case	EF_DEVIL4:
				BLIND(4);
				break;
		case	EF_DEVIL5:
				BLIND(5);
				break;
		case	EF_DEVIL6:
				BLIND(6);
				break;
		case	EF_DEVIL7:
				BLIND(7);
				break;
		case	EF_DEVIL8:
				BLIND(8);
				break;
		case	EF_DEVIL9:
				BLIND(9);
				break;
		case	EF_DEVIL10:
				BLIND(10);
				break;
		case	EF_BLACKDEVIL:
				BLACKDEVIL("effect\\ring_black.tga");
				break;
		case	EF_CHAINCOMBO:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

					PlayWave("effect\\mon_\xbf\xac\xc8\xaf.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

				}

				if (!EZ2STROnlyEffect())
					return false;
				break;
		case	EF_CHOOKGI:			CHOOKGI(0);			break;
		case	EF_LINELINK:		LINELINK();			break;
		case	EF_LINELINK2:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xbc\xd2\xbf\xef \xc3\xbc\xc0\xce\xc1\xf6.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					LINELINK2();
					break;
				}
		case	EF_LINELINK3:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xbc\xd2\xbf\xef \xc3\xbc\xc0\xce\xc1\xf6.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					LINELINK3();
					break;
				}
		case	EF_LINKLIGHT:
				{
					if (m_stateCnt >= 40 && m_stateCnt <= 70)
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;
						m_master->SetArgb(-1, 200, 150, 50);
					}
					break;
				}
		case	EF_PINKBODY:
				{
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;
						if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
							m_master->m_BodyLight += BL_DOUBLE_BODY;
						m_master->SetArgb(-1, 255,89,182);
					}
					break;
				}
		case	EF_PRESSEDBODY:
				{
					if (m_stateCnt == 5)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\T_\xb3\xbb\xb7\xc1\xc2\xef\xb1\xe2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					{
						if ((m_master->m_BodyLight & BL_PRESSED) != BL_PRESSED)
						{
							if (m_stateCnt>=17 && m_stateCnt<=50)
							{
								m_master->m_BodyLight += BL_PRESSED;
								m_master->m_BodyTime = m_stateCnt-17;
							}
						}
					}
					break;
				}
		case	EF_HITBODY:
				{
					{
						if ((m_master->m_BodyLight & BL_HIT) != BL_HIT)
						{
							m_master->m_BodyLight += BL_HIT;
							if (m_stateCnt<15)
								m_master->m_BodyTime = m_stateCnt;
						}
					}
					break;
				}
		case	EF_TAE_READY:
				{
					{
						if ((m_master->m_BodyLight & BL_HIT_BLUE) != BL_HIT_BLUE)
						{
							m_master->m_BodyLight += BL_HIT_BLUE;
							if (m_stateCnt<=50)
								m_master->m_BodyTime2 = m_stateCnt;
						}
					}
					break;
				}
		case	EF_BABYBODY:
				{
					{
						if ((m_master->m_BodyLight & BL_BABY_BODY) != BL_BABY_BODY)
						{
							m_master->m_BodyLight += BL_BABY_BODY;
							if (m_stateCnt<=45)
							{
								m_master->m_BodySin3 = m_stateCnt;
							}
						}
					}
					break;
				}
		case	EF_BABYBODY2:
				{
					{
						if ((m_master->m_BodyLight & BL_BABY_BODY) != BL_BABY_BODY)
						{
							m_master->m_BodyLight += BL_BABY_BODY;
							m_master->m_BodySin3 = 45;
						}
					}
					break;
				}
		case	EF_GIANTBODY:
				{
					{
						if ((m_master->m_BodyLight & BL_GIANT) != BL_GIANT)
						{
							m_master->m_BodyLight += BL_GIANT;
							if (m_stateCnt<=45)
							{
								m_master->m_BodySin4 = m_stateCnt;
							}
						}
					}
					break;
				}
		case	EF_GIANTBODY2:
				{
					{
						if ((m_master->m_BodyLight & BL_GIANT) != BL_GIANT)
						{
							m_master->m_BodyLight += BL_GIANT;
							m_master->m_BodySin4 = 45;
						}
					}
					break;
				}
		case	EF_REFLECTBODY:
				{
					{
						if ((m_master->m_BodyLight & BL_REFLECT_BODY) != BL_REFLECT_BODY)
						{
							m_master->m_BodyLight += BL_REFLECT_BODY;
						}
					}
					break;
				}
		case	EF_ASURABODY:
				{
					{
						if ((m_master->m_BodyLight & BL_ASURA) != BL_ASURA)
						{
							m_master->m_BodyLight += BL_ASURA;
							if (m_stateCnt<200)
								m_master->m_BodySin5 = m_stateCnt;
							else
								m_master->m_BodySin5 = 200;
						}
					}
					break;
				}
		case	EF_ASURABODY_MONSTER:
				{
					{
						if ((m_master->m_BodyLight & BL_ASURA) != BL_ASURA)
						{
							m_master->m_BodyLight += BL_ASURA;
							if (m_stateCnt<150)
								m_master->m_BodyTime = m_stateCnt+50;
						}
					}
					break;
				}
		case	EF_4WAYBODY:
				{
					if ( m_stateCnt >= 30 )
					{
						m_master->m_BodyLight |= BL_4WAY;
						m_master->m_BodyTime = m_stateCnt-30;
					}
					break;
				}
		case	EF_QUAKEBODY:
		case	EF_QUAKEBODY2:
				{
					{
						if ((m_master->m_BodyLight & BL_QUAKE) != BL_QUAKE)
						{
							m_master->m_BodyLight += BL_QUAKE;
							m_master->m_BodyTime = m_stateCnt;
						}
					}
					break;
				}
		case	EF_JUMPBODY:
				{
					if (m_stateCnt == 30)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\T_\xc8\xb8\xc7\xc7" "2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					{
						if ((m_master->m_BodyLight & BL_HIGHJUMP) != BL_HIGHJUMP)
						{
							if (m_stateCnt>35)
							{
								m_master->m_BodyLight += BL_HIGHJUMP;
								m_master->m_BodyTime = m_stateCnt-35;
							}
						}
					}
					break;
				}
		case	EF_LANDBODY:
				{
					{
						if ((m_master->m_BodyLight & BL_LANDJUMP) != BL_LANDJUMP)
						{
							{
								m_master->m_BodyLight += BL_LANDJUMP;
								m_master->m_BodyTime2 = m_stateCnt;
							}
						}

						if ((m_master->m_BodyLight & BL_SPINED) != BL_SPINED)
						{
							if (m_stateCnt<25)
							{
								m_master->m_BodyLight += BL_SPINED;
								m_master->m_BodyTime = m_stateCnt+11;
							}
						}
					}
					break;
				}
		case	EF_SPINEDBODY2:
				{
					if (m_stateCnt == 5)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\T_\xb5\xb9\xb7\xc1\xc2\xf7\xb1\xe2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					{
						if ((m_master->m_BodyLight & BL_SPINED) != BL_SPINED)
						{
							if (m_stateCnt>=14 && m_stateCnt<36+14)
							{
								m_master->m_BodyLight += BL_SPINED;
								m_master->m_BodyTime = m_stateCnt-14;
							}
						}
					}
					break;
				}
		case	EF_SPINEDBODY:
				{
					{
						if ((m_master->m_BodyLight & BL_SPINED) != BL_SPINED)
						{
							if (m_stateCnt<18)
							{
								m_master->m_BodyLight += BL_SPINED;
								m_master->m_BodyTime = m_stateCnt*2;
							}
						}
					}
					break;
				}
		case	EF_KICKEDBODY:
				{
					if (m_stateCnt == 17)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\EF_hit2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					if (m_stateCnt == 25)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\EF_hit4.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					{
						if ((m_master->m_BodyLight & BL_KICKED) != BL_KICKED)
						{
							if (m_stateCnt>=30)
							{
								m_master->m_BodyLight += BL_KICKED;
								m_master->m_BodyTime = m_stateCnt-30;
							}
						}
					}
					break;
				}
		case	EF_JUMPKICK:
				{
#ifdef	SECONDJOB_SKILL2
					if (m_stateCnt == 5)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\T_\xb3\xaf\xb6\xf3\xc2\xf7\xb1\xe2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

					}
					if (m_stateCnt == 5)
					{
						m_master->m_stateId = ST_A_JUMPKICK;
						m_master->SetForceAnimation(96,5 , 5, 96, 5, TRUE, 25, FALSE);
					}
					if (m_stateCnt == 35)
					{
						if (m_master->m_stateId == ST_A_JUMPKICK)
							m_master->m_stateId = ST_ATTACK;
					}
#endif
					break;
				}
		case	EF_DEVOTION:		if (!ProcessEZ2STR(0, -20, 0))	return false;		break;
		case	EF_PROVIDENCE:
				{
				Providence("effect\\ring_yellow.tga",0);
					if (!ProcessEZ2STR(0, -80, 0))	return false;
					break;
				}
		case	EF_LIGHTSPHERE2:
				{
					EffectTextureSet("effect\\thunder_center.bmp",0);
					for (int i=0;i<80;i++)
						LightSphere("effect\\white02.bmp");
				}
				break;
		case	EF_RAINBOW:
				{

					Rainbow("effect\\alpha_center.tga",0);


				}
				break;
		case	EF_SHIELDBOOMERANG:		ShieldBoomerang("effect\\shield_boomerang.bmp");	break;
		case	EF_AIRTEXTURE:
				{

					AirTexture("effect\\emp shock.tga");
				}
				break;
		case	EF_THROWITEM:				ThrowItem("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xba\xf3\xba\xb4.bmp", 0);		break;
		case	EF_THROWITEM2:				ThrowItem("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xba\xf3\xc6\xf7\xbc\xc7\xba\xb4.bmp",1);	break;

		case	EF_VENOMDUST2:				VenomDust2();		break;
#ifdef	MOB_SKILL
		case	EF_CHANGEDARK:				ChangeDark();		break;
		case	EF_CHANGEFIRE:				ChangeFire();		break;
		case	EF_CHANGECOLD:				ChangeCold();		break;
		case	EF_CHANGEWIND:				ChangeWind();		break;
		case	EF_CHANGEFLAME:				ChangeFlame();		break;
		case	EF_CHANGEEARTH:				ChangeEarth();		break;
		case	EF_CHAINGEHOLY:				ChangeHoly();		break;
		case	EF_CHANGEPOISON:			ChangePoison();		break;
		case	EF_HITDARK:					HitDark();			break;
		case	EF_DARKATTACK:				HitDark();			break;
		case	EF_CURSEATTACK:				CurseAttack();		break;
		case	EF_TELEKHIT:									break;
#endif
		case	EF_DARKBREATH:				DarkBreath();		break;
		case	EF_SUMMONSLAVE:				SummonSlave();		break;
		case	EF_BLOODDRAIN:				BloodDrain();		break;
		case	EF_ENERGYDRAIN:				EnergyDrain();		break;
		case	EF_ENERGYDRAIN2:
				{
					EnergyDrain2();

					if (m_stateCnt >= 55 && m_stateCnt <= 65)
					{
						if ((m_master->m_BodyLight & BL_LIGHT_BODY) != BL_LIGHT_BODY)
							m_master->m_BodyLight += BL_LIGHT_BODY;
						m_master->SetArgb(-1, 100, 100, 255);
					}
				}
				break;
		case	EF_ENERGYDRAIN3:
				{
					EnergyDrain3();

					if (m_stateCnt >= 50 && m_stateCnt <= 80)
					{
						if ((m_master->m_BodyLight & BL_DOUBLE_BODY) != BL_DOUBLE_BODY)
							m_master->m_BodyLight += BL_DOUBLE_BODY;
						m_master->SetArgb(-1, 250-(m_stateCnt*2), 250-(m_stateCnt*2), 250);
					}
				}
				break;
		case	EF_TRANSBLUEBODY:
				{
					if (m_stateCnt >= 0 && m_stateCnt <= 200)
					{
						m_master->SetArgb(-1, 255-(m_stateCnt+50), 255-(m_stateCnt+50), 255);
					}
				}
				break;

		case	EF_JOBCHANGE:			if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_LVUP:				if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_JOBLVUP:				if (!EZ2STROnlyEffect())					return false;		break;

		case	EF_POTIONPILLAR:			PotionPillar();		break;
		case	EF_ANGELUS:				if (!Angelus())				return false;		break;
		case	EF_SIGNUM:				if (!Signum())				return false;		break;
		case	EF_INVENOM:				if (!Invenom())				return false;		break;
		case	EF_ARROWSHOT:			if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_CURE:				if (!Cure())				return false;		break;
		case	EF_PROVOKE:				if (!Provoke())				return false;		break;
		case	EF_MVP:					if (!Mvp())					return false;		break;
		case	EF_SKIDTRAP:			if (!SkidTrap())			return false;		break;
		case	EF_GLORIA:				if (!Gloria())				return false;		break;
		case	EF_MAGNIFICAT:			if (!Magnificat())			return false;		break;
		case	EF_RESURRECTION:		if (!Resurrection())		return false;		break;
		case	EF_RECOVERY:			if (!Recovery())			return false;		break;
		case	EF_SANCTUARY:			if (!Sanctuary())			return false;		break;
		case	EF_IMPOSITIO:			if (!Impositio())			return false;		break;
		case	EF_LEXAETERNA:			if (!LexAeterna())			return false;		break;
		case	EF_ASPERSIO:			if (!Aspersio())			return false;		break;
		case	EF_LEXDIVINA:			if (!LexDivina())			return false;		break;
		case	EF_SUFFRAGIUM:			if (!Suffragium())			return false;		break;
		case	EF_STORMGUST:			if (!StormGust())			return false;		break;
		case	EF_LORD:				if (!Lord())				return false;		break;
		case	EF_BENEDICTIO:			if (!Benedictio())			return false;		break;
		case	EF_METEORSTORM:			if (!MeteorStorm())			return false;		break;
		case	EF_QUAGMIRE:			if (!Quagmire())			return false;		break;
		case	EF_FIREPILLAR:			if (!FirePillar())			return false;		break;
		case	EF_FIREPILLARBOMB:		if (!FirePillarBomb())		return false;		break;
		case	EF_REPAIRWEAPON:		if (!RepairWeapon())		return false;		break;
		case	EF_CRASHEARTH:			if (!CrashEarth())			return false;		break;
		case	EF_PERFECTION:			if (!Perfection())			return false;		break;
		case	EF_MAXPOWER:			if (!MaxPower())			return false;		break;
		case	EF_BLASTMINEBOMB:		if (!BlastMineBomb())		return false;		break;
		case	EF_CLAYMORE:			if (!Claymore())			return false;		break;
		case	EF_FREEZING:			if (!Freezing())			return false;		break;
		case	EF_SPRINGTRAP:			if (!SpringTrap())			return false;		break;
		case	EF_BUBBLE:				if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_GASPUSH:				if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_KYRIE:				if (!Kyrie())				return false;		break;
		case	EF_MAGNUS:				if (!Magnus())				return false;		break;
		case	EF_VENOMDUST:			if (!VenomDust())			return false;		break;
		case	EF_AUTOCOUNTER:			if (!AutoCounter())			return false;		break;
		case	EF_POISONREACT:			if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_POISONREACT2:		if (!PoisonReact2())		return false;		break;
		case	EF_SPLASHER:			if (!Splasher())			return false;		break;
		case	EF_FREEZE:				if (!Freeze())				return false;		break;
		case	EF_FREEZED:				if (!Freezed())				return false;		break;
		case	EF_ICECRASH:			if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_SANDMAN:				if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_PNEUMA:				if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_SONICBLOW2:			if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_BRANDISHSPEAR:		if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_BRANDISH2:			if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_FIREHIT:				if (!FireHit())				return false;		break;
		case	EF_WINDHIT:				if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_SHOCKWAVE:			if (!ShockWave())			return false;		break;
		case	EF_SHOCKWAVEHIT:		if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_EARTHHIT:			if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_PIERCESELF:			if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_BOWLINGSELF:			if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_SPEARSTABSELF:		if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_SPEARBMRSELF:		if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_HOLYHIT:				if (!EZ2STROnlyEffect())	return false;		break;
		case	EF_CONCENTRATION:		if (!Concentration())		return false;		break;
		case	EF_REFINEOK:			if (!RefineOk())			return false;		break;
		case	EF_REFINEFAIL:			if (!RefineFail())			return false;		break;

		case	EF_STOPEFFECT:
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\T_\xc8\xbf\xb0\xfa\xc0\xbd" "1.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					RunStop(0);
					RunStop(45);
				}
				break;
		case	EF_COIN:				if (!Coin())								return false;		break;
		case	EF_FIREWALL:			if (!FireWall())							return false;		break;
		case	EF_GLASSWALL:			if (!GlassWall())							return false;		break;
		case	EF_LIGHTBOLT:			if (!LightBolt())							return false;		break;
		case	EF_STONECURSE:			if (!StoneCurse())							return false;		break;
		case	EF_THUNDERSTORM:		if (!ThunderStorm())						return false;		break;
		case	EF_TAMINGSUCCESS:		if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_TAMINGFAILED:		if (!EZ2STROnlyEffect())					return false;		break;
#if defined(BALANCE_PATCH)
		case	EF_ENERGYCOAT:			if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_CARTREVOLUTION:		if (!CartRevolution())						return false;		break;
#endif
#ifdef	MOB_SKILL
		case	EF_MENTALBREAK:			if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_SUI_EXPLOSION:		if (!SuiExplosion())						return false;		break;
		case	EF_SUICIDE:				if (!Suicide())								return false;		break;
		case	EF_COMBOATTACK1:		if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_COMBOATTACK2:		if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_COMBOATTACK3:		if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_COMBOATTACK4:		if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_COMBOATTACK5:		if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_GUIDEDATTACK:		if (!EZ2STROnlyEffect())					return false;		break;

		case	EF_POISONATTACK:		if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_SILENCEATTACK:		if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_STUNATTACK:			if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_PETRIFYATTACK:		if (!StoneCurse())							return false;		break;
		case	EF_SLEEPATTACK:			if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_MAGICALATTHIT:		if (!EZ2STROnlyEffect())					return false;		break;

#endif
		case	EF_PONG:				if (!EZ2STROnlyEffect())					return false;		break;
		case	EF_POTION1:
		case	EF_POTION2:
		case	EF_POTION3:
		case	EF_POTION4:
		case	EF_POTION7:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("_heal_effect.wav", m_pos.x-pPos.x, -80,  m_pos.z-pPos.z);
				}
				if (!EZ2STROnlyEffect())											return false;		break;
		case	EF_POTION5:
		case	EF_POTION6:
		case	EF_POTION8:
#ifdef SECONDJOB_SKILL2
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

					PlayWave("effect\\\xc8\xed\xb1\xe2.wav", m_pos.x-pPos.x, -80,  m_pos.z-pPos.z);

				}
#endif
				if (!EZ2STROnlyEffect())
					return false;
				break;
		case	EF_DEFFENDER:
			if (!EZ2STROnlyEffect())
				return false;
			break;
		case	EF_KEEPING:
			if (!EZ2STROnlyEffect())
				return false;
			break;
		case	EF_POTION_CON:			if (!Potion_Con())			return false;		break;
		case	EF_POTION_:				if (!Potion_())				return false;		break;
		case	EF_POTION_BERSERK:		if (!Potion_Berserk())		return false;		break;
#ifdef SECONDJOB_SKILL2
		case	EF_SPELLBREAKER:
				if (m_stateCnt == 0)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\sage_spell breake.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				if (!EZ2STROnlyEffect())
					return false;
				break;
		case	EF_DISPELL:
			if (!EZ2STROnlyEffect())
				return false;
			break;
		case	EF_HOLYCROSS :
				if (m_stateCnt == 0)
					PlayWave("effect\\cru_holy cross.wav");
				if (!EZ2STROnlyEffect())
					return false;
				break;

		case	EF_FOOT3:					Foot("effect\\foot_l2.tga",1);				break;
		case	EF_FOOT4:					Foot("effect\\foot_r2.tga",1);				break;
		case	EF_STEALCOIN :
			if (!EZ2STROnlyEffect())
				return false;
			break;
			case	EF_STRIPWEAPON :
			case	EF_STRIPSHIELD :
			case	EF_STRIPARMOR :
			case	EF_STRIPHELM :
					{
						if (m_stateCnt == 6)
						{
							m_pos = m_master->m_pos;
							vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

							PlayWave("effect\\\xba\xb9\xc8\xa3\xb0\xdd.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
						}

					if (!EZ2STROnlyEffect())
					return false;
					break;
				}
		case	EF_RG_COIN :
				{
					if (m_stateCnt == 0)
					{
						m_pos = m_master->m_pos;
						vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\rog_steal coin.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
					for (int i=0;i<30;i++)
						RG_COIN("effect\\coin_a.bmp");
					break;
				}
		case	EF_SHIELDCHARGE :		if (!ShieldCharge())		return false;		break;

			case	EF_FLAMELAUNCHER:
#if defined (_UPDATE2005C)
				if (m_stateCnt == 10)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("_enemy_hit_wind1.wav", m_pos.x-pPos.x, -150, m_pos.z-pPos.z);
				}
#endif
				if (!EZ2STROnlyEffect())
					return false;
				break;
			case	EF_FROSTWEAPON:
#if defined (_UPDATE2005C)
				if (m_stateCnt == 10)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("_enemy_hit_wind1.wav", m_pos.x-pPos.x, -150, m_pos.z-pPos.z);
				}
#endif
				if (!EZ2STROnlyEffect())
					return false;
				break;
			case	EF_LIGHTNINGLOADER:
#if defined (_UPDATE2005C)
				if (m_stateCnt == 10)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("_enemy_hit_wind1.wav", m_pos.x-pPos.x, -150, m_pos.z-pPos.z);
				}
#endif
				if (!EZ2STROnlyEffect())
					return false;
				break;
			case	EF_SEISMICWEAPON:
#if defined (_UPDATE2005C)
				if (m_stateCnt == 10)
				{
					m_pos = m_master->m_pos;
					vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("_enemy_hit_wind1.wav", m_pos.x-pPos.x, -150, m_pos.z-pPos.z);
				}
#endif
				if (!EZ2STROnlyEffect())
					return false;
				break;

			case	EF_MAGICROD:
				{
					EZ2STROnlyEffect();
					MagicRod();
				}
				break;
#endif
		default:
				{
					char	buf[128];
					sprintf(buf, "Invalid Effect Id %d", m_type);
					ErrorMsg(buf);
					return false;
				}
				break;
		}
	}

	m_stateCnt++;
	if (m_stateCnt >= m_duration)
	{
#ifdef _LOOP_FIX1
		if( m_UseEZProcess == true )
		{
			if( m_cLoopCount == -1 )
	  {
		return true ;
	  }
		}
		else
#endif
		{
			return false ;
		}
	}


	return true;
}

void	CRagEffect :: Render(matrix &wtm)
{
	if (!m_aniClips) return;

	int rpCnt = 0;

	for (int iLayer = 1; iLayer < m_aniClips->cLayer; iLayer++) {

		if (m_isLayerDrawn[iLayer])
		{
			RenderAniClip(m_aniClips->aLayer[iLayer], &m_actXformData[iLayer], rpCnt, wtm);

			rpCnt += 2;
		}
	}
}

void	CRagEffect :: Init(CRenderObject * master, int effectId, vector3d fDeltaPos )
{
	m_type = effectId;
	m_master = master;
	m_pos = master->m_pos;
	m_longitude = master->m_roty;
	m_duration = durationTable[m_type];
	m_deltaPos = fDeltaPos;

	char StrName[64];
	sprintf(StrName, "");

	switch (m_type)
	{
		case	EF_ANGEL:
			sprintf(StrName, "angel.str");
			break;
		case	EF_DEVIL:
			sprintf(StrName, "devil.str");
			break;
		case	EF_ANGELUS:
#if defined (_UPDATE2004B)
			if (g_session.m_isMinEffect)
			{
				sprintf(StrName, "jong_mini.str");
			}
			else
#endif
			{
				sprintf(StrName, "Angelus.str");
			}
			break;
		case EF_ARROWSHOT:
			sprintf(StrName, "ArrowShot.str");
			break;
		case EF_INVENOM:
			sprintf(StrName, "Invenom.str");
			break;
		case EF_CURE:
			if (g_session.m_isMinEffect) {
				sprintf(StrName, "cure_min.str");
			} else {
				sprintf(StrName, "Cure.str");
			}
			break;
		case EF_PROVOKE:
			sprintf(StrName, "Provoke.str");
			break;
		case EF_MVP:
			sprintf(StrName, "Mvp.str");
			break;
		case EF_SKIDTRAP:
			sprintf(StrName, "SkidTrap.str");
			break;
		case EF_GLORIA:
			if (g_session.m_isMinEffect) {
				sprintf(StrName, "gloria_min.str");
			} else {
				sprintf(StrName, "Gloria.str");
			}
			break;
		case EF_MAGNIFICAT:
			if (g_session.m_isMinEffect) {
				sprintf(StrName, "magnificat_min.str");
			} else {
				sprintf(StrName, "Magnificat.str");
			}
			break;
		case EF_RESURRECTION:
			if (g_session.m_isMinEffect) {
				sprintf(StrName, "resurrection_min.str");
			} else {
				sprintf(StrName, "Resurrection.str");
			}
			break;
		case EF_RECOVERY:
			sprintf(StrName, "Recovery.str");
			break;
		case EF_SANCTUARY:
			sprintf(StrName, "Sanctuary.str");
			break;
		case EF_IMPOSITIO:
			sprintf(StrName, "Impositio.str");
			break;
		case EF_LEXAETERNA:
			if (g_session.m_isMinEffect) {
				sprintf(StrName, "lexaeterna_min.str");
			} else {
				sprintf(StrName, "LexAeterna.str");
			}
			break;
		case EF_ASPERSIO:
			sprintf(StrName, "Aspersio.str");
			break;
		case EF_LEXDIVINA:
			sprintf(StrName, "LexDivina.str");
			break;
		case EF_SUFFRAGIUM:
			if (g_session.m_isMinEffect) {
				sprintf(StrName, "suffragium_min.str");
			} else {
				sprintf(StrName, "Suffragium.str");
			}
			break;
		case EF_STORMGUST:
			sprintf(StrName, "StormGust.str");
			break;
		case EF_LORD:
			sprintf(StrName, "Lord.str");
			break;
		case EF_BENEDICTIO:
			sprintf(StrName, "Benedictio.str");
			break;
		case EF_METEORSTORM:
			sprintf(StrName, "Meteor%d.str", random(3) + 1);
			break;
		case EF_QUAGMIRE:
			sprintf(StrName, "Quagmire.str");
			break;
		case EF_FIREPILLAR:
			sprintf(StrName, "FirePillar.str");
			break;
		case EF_FIREPILLARBOMB:
			sprintf(StrName, "FirePillarBomb.str");
			break;
		case EF_REPAIRWEAPON:
			sprintf(StrName, "RepairWeapon.str");
			break;
		case EF_CRASHEARTH:
			sprintf(StrName, "CrashEarth.str");
			break;
		case EF_PERFECTION:
			if (g_session.m_isMinEffect) {
				sprintf(StrName, "WeaponPerfection_min.str");
			} else {
				sprintf(StrName, "WeaponPerfection.str");
			}
			break;
		case EF_MAXPOWER:
			if (g_session.m_isMinEffect) {
				sprintf(StrName, "maximize_min.str");
			} else {
				sprintf(StrName, "MaximizePower.str");
			}
			break;
		case EF_BLASTMINEBOMB:
			sprintf(StrName, "BlastMine.str");
			break;
		case EF_CLAYMORE:
			sprintf(StrName, "Claymore.str");
			break;
		case EF_FREEZING:
			sprintf(StrName, "Freezing.str");
			break;
		case EF_SPRINGTRAP:
			sprintf(StrName, "Spring.str");
			break;
		case EF_BUBBLE:
			sprintf(StrName, "Bubble%d.str", random(4) + 1);
			break;
		case EF_GASPUSH:
			sprintf(StrName, "GasPush.str");
			break;
		case EF_KYRIE:
			if (g_session.m_isMinEffect) {
				sprintf(StrName, "kyrie_min.str");
			} else {
				sprintf(StrName, "Kyrie.str");
			}
			break;
		case EF_MAGNUS:
			sprintf(StrName, "Magnus.str");
			break;
		case EF_VENOMDUST:
			sprintf(StrName, "VenomDust.str");
			break;
		case EF_POISONREACT:
			sprintf(StrName, "PoisonReact_1st.str");
			break;
		case EF_POISONREACT2:
			sprintf(StrName, "PoisonReact.str");
			break;
		case EF_AUTOCOUNTER:
			sprintf(StrName, "AutoCounter.str");
			break;
		case EF_SPLASHER:
			sprintf(StrName, "VenomSplasher.str");
			break;
		case EF_FREEZE:
			sprintf(StrName, "Freeze.str");
			break;
		case EF_FREEZED:
			sprintf(StrName, "Freezed.str");
			break;
		case EF_ICECRASH:
			sprintf(StrName, "IceCrash.str");
			break;
		case	EF_SANDMAN:
			sprintf(StrName, "SandMan.str");
			break;
		case	EF_PNEUMA:
			sprintf(StrName, "Pneuma%d.str", random(3) + 1);
			break;
		case	EF_SONICBLOW2:
			sprintf(StrName, "SonicBlow.str");
			break;
		case	EF_BRANDISHSPEAR:
			sprintf(StrName, "Brandish.str");
			break;
		case	EF_BRANDISH2:
			sprintf(StrName, "Brandish2.str");
			break;
		case	EF_FIREHIT:
		case	EF_FIREHIT2:
			sprintf(StrName, "FireHit%d.str", random(3) + 1);
			break;
		case	EF_WINDHIT:
			sprintf(StrName, "WindHit%d.str", random(3) + 1);
			break;
		case	EF_SHOCKWAVE:
			sprintf(StrName, "ShockWave.str");
			break;
		case	EF_SHOCKWAVEHIT:
			sprintf(StrName, "ShockWaveHit.str");
			break;
		case	EF_EARTHHIT:
			sprintf(StrName, "EarthHit.str");
			break;
		case	EF_PIERCESELF:
			sprintf(StrName, "Pierce.str");
			break;
		case	EF_BOWLINGSELF:
			sprintf(StrName, "Bowling.str");
			break;
		case	EF_SPEARSTABSELF:
			sprintf(StrName, "SpearStab.str");
			break;
		case	EF_SPEARBMRSELF:
			sprintf(StrName, "SpearBoomerang.str");
			break;
		case	EF_HOLYHIT:
			sprintf(StrName, "HolyHit.str");
			break;
		case	EF_CONCENTRATION:
			sprintf(StrName, "Concentration.str");
			break;
		case	EF_REFINEOK:
			sprintf(StrName, "bs_RefineSuccess.str");
			break;
		case	EF_REFINEFAIL:
			sprintf(StrName, "bs_RefineFailed.str");
			break;
		case	EF_PHARMACY_OK:
			sprintf(StrName, "p_success.str");
			break;
		case	EF_PHARMACY_FAIL:
			sprintf(StrName, "p_failed.str");
			break;
		case	EF_JOBCHANGE:
			sprintf(StrName, "JobChange.str");
			break;
		case	EF_LVUP:
			sprintf(StrName, "LevelUP.str");
			break;
		case	EF_JOBLVUP:
			sprintf(StrName, "JobLvUP.str");
			break;
		case	EF_JOBLVUP50:
			sprintf(StrName, "JobLvUP.str");
			break;
		case	EF_TWOHANDQUICKEN:
			sprintf(StrName, "TwoHand.str");
			break;
		case	EF_SPEARQUICKEN:
			sprintf(StrName, "TwoHand.str");
			break;
		case EF_COIN:
			if (g_session.m_isMinEffect) {
				sprintf(StrName, "memor_min.str");
			} else {
				sprintf(StrName, "Maemor.str");
			}
			break;
		case EF_FIREWALL:
			sprintf(StrName, "Firewall%d.str", random(2) + 1);
			break;
		case	EF_GLASSWALL:
			sprintf(StrName, "SafetyWall.str");
			break;
		case	EF_GLASSWALL2:
			sprintf(StrName, "SafetyWall.str");
			break;
		case EF_THUNDERSTORM:
			sprintf(StrName, "ThunderStorm.str");
			break;
		case EF_LIGHTBOLT:
			sprintf(StrName, "Lightning.str");
			break;
		case EF_STONECURSE:
			sprintf(StrName, "StoneCurse.str");
			break;
		case	EF_SIGNUM:
			sprintf(StrName, "Cross.str");
			break;
		case	EF_TAMINGSUCCESS:
			sprintf(StrName, "TamingSuccess.str");
			break;
		case	EF_TAMINGFAILED:
			sprintf(StrName, "TamingFailed.str");
			break;
#if defined(BALANCE_PATCH)
		case	EF_ENERGYCOAT:
			sprintf(StrName, "EnergyCoat.str");
			break;
		case	EF_CARTREVOLUTION:
			sprintf(StrName, "CartRevolution.str");
			break;
#endif
		case	EF_SUI_EXPLOSION:
			sprintf(StrName, "sui_explosion.str");
			break;
		case	EF_MENTALBREAK:
			sprintf(StrName, "MentalBreak.str");
			break;
		case	EF_MAGICALATTHIT:
			sprintf(StrName, "magical.str");
			break;
		case	EF_SUICIDE:
			sprintf(StrName, "suicide.str");
			break;
		case	EF_COMBOATTACK1:
			sprintf(StrName, "yunta_1.str");
			break;
		case	EF_COMBOATTACK2:
			sprintf(StrName, "yunta_2.str");
			break;
		case	EF_COMBOATTACK3:
			sprintf(StrName, "yunta_3.str");
			break;
		case	EF_COMBOATTACK4:
			sprintf(StrName, "yunta_4.str");
			break;
		case	EF_COMBOATTACK5:
			sprintf(StrName, "yunta_5.str");
			break;
		case	EF_GUIDEDATTACK:
			sprintf(StrName, "homing.str");
			break;
		case	EF_POISONATTACK:
			sprintf(StrName, "poison.str");
			break;
		case	EF_SILENCEATTACK:
			sprintf(StrName, "silence.str");
			break;
		case	EF_STUNATTACK:
			sprintf(StrName, "stun.str");
			break;
		case	EF_SLEEPATTACK:
			sprintf(StrName, "sleep.str");
			break;
		case	EF_PETRIFYATTACK:
			sprintf(StrName, "StoneCurse.str");
			break;
		case	EF_PONG:
			sprintf(StrName, "Pong%d.str", random(3) + 1);
			break;

		case EF_HFLIMOON1:
			sprintf(StrName, "moonlight_1.str");
			break;
		case EF_HFLIMOON2:
			sprintf(StrName, "moonlight_2.str");
			break;
		case EF_HFLIMOON3:
			sprintf(StrName, "moonlight_3.str");
			break;

		case EF_HO_UP:
			sprintf(StrName, "h_levelup.str");
			break;

		case	EF_POTION1:
			sprintf(StrName, "\xBB\xA1\xB0\xA3\xC6\xF7\xBC\xC7.str");
			break;
		case	EF_POTION2:
			sprintf(StrName, "\xC1\xD6\xC8\xAB\xC6\xF7\xBC\xC7.str");
			break;
		case	EF_POTION3:
			sprintf(StrName, "\xB3\xEB\xB6\xF5\xC6\xF7\xBC\xC7.str");
			break;
		case	EF_POTION4:
			sprintf(StrName, "\xC7\xCF\xBE\xE1\xC6\xF7\xBC\xC7.str");
			break;
		case	EF_POTION5:
			sprintf(StrName, "\xC6\xC4\xB6\xF5\xC6\xF7\xBC\xC7.str");
			break;
		case	EF_POTION6:
			sprintf(StrName, "\xC3\xCA\xB7\xCF\xC6\xF7\xBC\xC7.str");
			break;
		case	EF_POTION7:
			sprintf(StrName, "fruit.str");
			break;
		case	EF_POTION8:
			sprintf(StrName, "fruit_.str");
			break;
		case	EF_DEFFENDER:
			sprintf(StrName, "Deffender.str");
			break;
		case	EF_KEEPING:
			sprintf(StrName, "Keeping.str");
			break;
		case	EF_POTION_CON:
			sprintf(StrName, "\xC1\xFD\xC1\xDF.str");
			break;
		case	EF_POTION_:
			sprintf(StrName, "\xB0\xA2\xBC\xBA.str");
			break;
		case	EF_POTION_BERSERK:
			sprintf(StrName, "\xB9\xF6\xBC\xAD\xC5\xA9.str");
			break;
#ifdef SECONDJOB_SKILL2
		case	EF_SPELLBREAKER:
			sprintf(StrName, "spell.str");
			break;
#endif
		case	EF_DISPELL:
			sprintf(StrName, "\xB5\xF0\xBD\xBA\xC6\xE7.str");
			break;
		case	EF_MAGICROD:
			sprintf(StrName, "\xB8\xC5\xC1\xF7\xB7\xCE\xB5\xE5.str");
			break;
#ifdef SECONDJOB_SKILL2
		case	EF_HOLYCROSS:
			sprintf(StrName, "holy_cross.str");
			break;
#endif
		case	EF_CHAINCOMBO:
			sprintf(StrName, "\xBF\xAC\xC8\xAF.str");
			break;
#ifdef SECONDJOB_SKILL2
		case	EF_ASSUMPTIO2:
			sprintf(StrName, "asum.str");
			break;
		case	EF_MELTDOWN:
			sprintf(StrName, "melt.str");
			break;
#endif
		case	EF_MOCHI:
			sprintf(StrName, "\xC2\xFD\xBD\xD2\xB6\xB1.str");
			break;
#ifdef SECONDJOB_SKILL2
		case	EF_CARTBOOST:
			sprintf(StrName, "cart.str");
			break;
		case	EF_REJECTSWORD:
			sprintf(StrName, "sword.str");
			break;
#endif
		case	EF_SOULBURN:
			sprintf(StrName, "\xBC\xD2\xBF\xEF\xB9\xF8.str");
			break;
		case	EF_SOULCHANGE:
			sprintf(StrName, "\xBB\xE7\xB6\xF7\xC8\xBF\xB0\xFA.str");
			break;
#ifdef SECONDJOB_SKILL2
		case	EF_AURABLADE:
#if defined (_UPDATE2004B)
			break;
#else
			sprintf(StrName, "AURA.str");
			break;
#endif
		case	EF_STEALCOIN:
			sprintf(StrName, "steal_coin.str");
			break;
		case	EF_STRIPWEAPON:
			sprintf(StrName, "strip_weapon.str");
			break;
		case	EF_STRIPSHIELD:
			sprintf(StrName, "strip_shield.str");
			break;
		case	EF_STRIPARMOR:
			sprintf(StrName, "strip_armor.str");
			break;
		case	EF_STRIPHELM:
			sprintf(StrName, "strip_helm.str");
			break;
		case	EF_SHIELDCHARGE:
			sprintf(StrName, "shield_charge.str");
			break;
		case	EF_DEVOTION:
			sprintf(StrName, "devotion.str");
			break;
		case	EF_PROVIDENCE:
			sprintf(StrName, "providence.str");
			break;
		case	EF_FLAMELAUNCHER:
			sprintf(StrName, "enc_fire.str");
			break;
		case	EF_FROSTWEAPON:
			sprintf(StrName, "enc_ice.str");
			break;
		case	EF_LIGHTNINGLOADER:
			sprintf(StrName, "enc_wind.str");
			break;
		case	EF_SEISMICWEAPON:
			sprintf(StrName, "enc_earth.str");
			break;
		case	EF_LOUD:
			sprintf(StrName, "loud.str");
			break;
		case	EF_LKCONCENTRATION:
			sprintf(StrName, "TwoHand.str");
			break;
		case	EF_LAMADAN:
			sprintf(StrName, "ramadan.str");
			break;
		case	EF_MAPAE:
			sprintf(StrName, "mapae.str");
			break;
		case	EF_ITEMPOKJUK:
			sprintf(StrName, "itempokjuk.str");
			break;
		case	EF_BAKU:
			sprintf(StrName, "fire dragon.str");
			break;
		case	EF_TRACKCASTING:
			sprintf(StrName, "\xC6\xAE\xB7\xA2\xC5\xB7.str");
			break;
		case	EF_BULLSEYE:
			sprintf(StrName, "\xBA\xD2\xBD\xBA\xBE\xC6\xC0\xCC.str");
			break;
		case	EF_HYOUSYOURAKU:
			sprintf(StrName, "icy.str");
			break;
		case EF_HAMIDEFENCE:
			sprintf(StrName, "defense.str");
			break;
		case EF_FOOD01:
			sprintf(StrName, "food_str.str");
			break;
		case EF_FOOD02:
			sprintf(StrName, "food_int.str");
			break;
		case EF_FOOD03:
			sprintf(StrName, "food_vit.str");
			break;
		case EF_FOOD04:
			sprintf(StrName, "food_agi.str");
			break;
		case EF_FOOD05:
			sprintf(StrName, "food_dex.str");
			break;
		case EF_FOOD06:
			sprintf(StrName, "food_luk.str");
			break;
		case EF_COOKING_OK:
			sprintf(StrName, "cook_suc.str");
			break;
		case EF_COOKING_FAIL:
			sprintf(StrName, "cook_fail.str");
			break;
		case EF_HAPGYEOK:
			sprintf(StrName, "itempokjuk.str");
			break;
		case EF_DRAGONFEAR:
			sprintf(StrName, "dfear.str");
			break;
		case EF_BLEEDING:
			sprintf(StrName, "wideb.str");
			break;
		case EF_WIDECONFUSE:
			sprintf(StrName, "dfear.str");
			break;
		case EF_CRITICALWOUND:
			sprintf(StrName, "cwound.str");
			break;
		case EF_SLOWPOISON:
			sprintf(StrName, "slowp.str");
			break;
		case EF_FLOWERLEAF:
			sprintf(StrName, "flower_leaf.str");
			break;
#endif
	}
	if (strlen(StrName) > 0)
	{
		LoadEzEffect(StrName);
	}
}

CEffectPrim *	CRagEffect :: LaunchEffectPrim(EFFECTPRIMID effectPrimId, vector3d fDeltaPos)
{
	CEffectPrim * prim = new CEffectPrim;
	if (!prim) {
		sAssert(false);
		return NULL;
	}
	prim->Init(this, effectPrimId, fDeltaPos);
	m_primList.push_back(prim);

	CGameMode * gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);
	gameMode->GetWorld()->m_gameObjectList.push_back(prim);
	return	prim;
}

void	CRagEffect :: LoadEzEffect(char *fName)
{
	sAssert(m_ezEffectRes == NULL);
	sAssert(m_aniClips == NULL);

	m_ezEffectRes = (CEZeffectRes *)g_resMgr().Get(fName, false);

	if(m_ezEffectRes)
	{
		m_ezEffectRes->Lock();

		m_aniClips =  &m_ezEffectRes->m_aniClips;
		m_cEndLayer = m_aniClips->cLayer;

		if (m_aniClips->aLayer[0].cAniKey == 0) m_cEndLayer--;
	}
}

void	CRagEffect :: InitEZ2STRFrame()
{
	m_iCurLayer = 0;
	m_stateCnt = 0;
	m_cEndLayer = m_aniClips->cLayer;
	if (m_aniClips->aLayer[0].cAniKey == 0)
		m_cEndLayer--;

	for (int iLayer = 0; iLayer < m_aniClips->cLayer; iLayer++) {
		m_aiCurAniKey[iLayer] = 0;
	}
}

bool	CRagEffect :: ProcessEZ2STR(float x, float y, float z)
{
	float			r;


	if (m_ezEffectRes == NULL) return false;


	if (m_iCurLayer >= m_cEndLayer) return false;

	tlvertex3d	tlvert = TransformVert(g_modeMgr.GetGameMode()->GetView()->GetViewMatrix(), m_master->m_pos);

		m_tlvertX = tlvert.x + x;

		m_tlvertY = tlvert.y + y;
		m_tlvertZ = tlvert.z + z;

		if (x == 0 && y == -80 && z == 0)
			m_defZ = FALSE;
		else
			m_defZ = TRUE;


		for (int iLayer = 0; iLayer < m_aniClips->cLayer; iLayer++)
		{
			m_isLayerDrawn[iLayer] = false;

			const KAC_LAYER* pLayer = &m_aniClips->aLayer[iLayer];

			if (pLayer == NULL) continue;

			if (pLayer->cAniKey == 0) continue;

			if (m_aiCurAniKey[iLayer] == -1) continue;

			int nAniKey = m_aiCurAniKey[iLayer] ;

			KAC_KEYFRAME * pAK = &pLayer->aAniKey[ nAniKey ];

			int dFrame = m_stateCnt - pAK->iFrame;

			if (dFrame < 0)	continue;

			KAC_XFORMDATA *	pCurXformData = &m_actXformData[iLayer];

			if (dFrame && (pAK + 1)->iFrame == m_stateCnt) {
				m_aiCurAniKey[iLayer]++;

				if (m_aiCurAniKey[iLayer] >= (pLayer->cAniKey - 1)) {
					m_aiCurAniKey[iLayer] = -1;
					m_iCurLayer++;
				}

				pAK++;
			}

			if (pAK->dwType == EAKT_POS) {
				memcpy(pCurXformData, &pAK->XformData, sizeof(KAC_XFORMDATA));

				if(m_aiCurAniKey[iLayer] != -1)
					if ((pAK + 1)->iFrame == pAK->iFrame) m_aiCurAniKey[iLayer]++;
			}
			else {
				pCurXformData->x += pAK->XformData.x;
				pCurXformData->y += pAK->XformData.y;
				pCurXformData->u += pAK->XformData.u;
				pCurXformData->v += pAK->XformData.v;
				pCurXformData->us += pAK->XformData.us;
				pCurXformData->vs += pAK->XformData.vs;
				pCurXformData->u2 += pAK->XformData.u2;
				pCurXformData->v2 += pAK->XformData.v2;
				pCurXformData->us2 += pAK->XformData.us2;
				pCurXformData->vs2 += pAK->XformData.vs2;
				pCurXformData->ax[0] += pAK->XformData.ax[0];
				pCurXformData->ax[1] += pAK->XformData.ax[1];
				pCurXformData->ax[2] += pAK->XformData.ax[2];
				pCurXformData->ax[3] += pAK->XformData.ax[3];
				pCurXformData->ay[0] += pAK->XformData.ay[0];
				pCurXformData->ay[1] += pAK->XformData.ay[1];
				pCurXformData->ay[2] += pAK->XformData.ay[2];
				pCurXformData->ay[3] += pAK->XformData.ay[3];
				pCurXformData->crR += pAK->XformData.crR;
				pCurXformData->crG += pAK->XformData.crG;
				pCurXformData->crB += pAK->XformData.crB;

				pCurXformData->crA += pAK->XformData.crA;
				pCurXformData->rz += pAK->XformData.rz;

				switch (pAK->XformData.anitype) {
				case ANIT_STOP:
					break;

				case ANIT_INTP:
					pCurXformData->aniframe += pAK->XformData.aniframe;
					break;

				case ANIT_ONCE:
					pCurXformData->aniframe += pAK->XformData.anidelta;
					if (pCurXformData->aniframe >= (float)pLayer->cTex)
						pCurXformData->aniframe = (float)pLayer->cTex - 1;

					break;

				case ANIT_BILOOP:
					{
						int iFrame = (int)(pCurXformData->aniframe + pAK->XformData.anidelta * (float)dFrame);
						int iCnt = pLayer->cTex - 1;
						int iR = iFrame / iCnt;

						if (iR & 1)
							iFrame = iCnt - (iFrame - iR * iCnt);
						else
							iFrame = iFrame - iR * iCnt;

						pCurXformData->aniframe = (float)iFrame;
					}
					break;

				case ANIT_LOOP:
					{
						float f = pCurXformData->aniframe += pAK->XformData.anidelta;

						if (pCurXformData->aniframe >= (float)pLayer->cTex) {
							r = (float)(int)(f / (float)pLayer->cTex);
							f -= (r * (float)pLayer->cTex);

							pCurXformData->aniframe = f;
						}
					}
					break;

				case ANIT_RLOOP:
					{
						float f = pCurXformData->aniframe -= pAK->XformData.anidelta;

						if (f < 0.0f) {
							f -= (r * (float)pLayer->cTex);
							r = (float)(int)(f / (float)pLayer->cTex);

							if (f < 0.0f)
								f += (float)pLayer->cTex;

							pCurXformData->aniframe = f;
						}
					}
					break;

				}
			}

			m_isLayerDrawn[iLayer] = true;
		}

		return true;
}

void CRagEffect :: RenderAniClip(KAC_LAYER& pLayer, const KAC_XFORMDATA* pXformData, int rpCnt, matrix &wtm)
{
	DWORD		color;
	int			iVert;
	float		tx, ty, rx, ry;
	float		sinDeg;
	float		cosDeg;

	float		x, y, oow;
	g_renderer->ProjectVertex(m_pos, wtm, &x, &y, &oow);

	float	pixelRatio = g_renderer->GetPixelRatio(oow);

	RPFace& rpFace = g_renderer->BorrowQuadRP();

	tlvertex3d*	tlvert = rpFace.verts;

	if (pXformData->ax[0] < pXformData->ax[1])
	{
		tlvert[1].x = pXformData->ax[0]  ;
		tlvert[1].tu = pXformData->u;
		tlvert[3].x = pXformData->ax[1]  ;
		tlvert[3].tu = pXformData->u + pXformData->us;

		tlvert[0].x = pXformData->ax[3]  ;
		tlvert[0].tu = pXformData->u;
		tlvert[2].x = pXformData->ax[2]  ;
		tlvert[2].tu = pXformData->u + pXformData->us;
	}
	else
	{
		tlvert[1].x = pXformData->ax[1]  ;
		tlvert[1].tu = pXformData->u + pXformData->us;
		tlvert[3].x = pXformData->ax[0]  ;
		tlvert[3].tu = pXformData->u;

		tlvert[0].x = pXformData->ax[2]  ;
		tlvert[0].tu = pXformData->u + pXformData->us;
		tlvert[2].x = pXformData->ax[3]  ;
		tlvert[2].tu = pXformData->u;
	}

	if (pXformData->ay[1] < pXformData->ay[2])
	{
		tlvert[3].y = pXformData->ay[1] ;
		tlvert[3].tv = pXformData->v;
		tlvert[2].y = pXformData->ay[2]  ;
		tlvert[2].tv = pXformData->v + pXformData->vs;

		tlvert[1].y = pXformData->ay[0]  ;
		tlvert[1].tv = pXformData->v;
		tlvert[0].y = pXformData->ay[3]  ;
		tlvert[0].tv = pXformData->v + pXformData->vs;
	}
	else
	{
		tlvert[3].y = pXformData->ay[2] ;
		tlvert[3].tv = pXformData->v + pXformData->vs;
		tlvert[2].y = pXformData->ay[1] ;
		tlvert[2].tv = pXformData->v;

		tlvert[1].y = pXformData->ay[3]  ;
		tlvert[1].tv = pXformData->v + pXformData->vs;
		tlvert[0].y = pXformData->ay[0]  ;
		tlvert[0].tv = pXformData->v;
	}


	CTexture* tex = pLayer.GetTexture( max((int)pXformData->aniframe, 0) );

	float uAdjust = tex->GetUAdjust();
	float vAdjust = tex->GetVAdjust();

	tlvert[0].tu *= uAdjust;
	tlvert[0].tv *= vAdjust;
	tlvert[1].tu *= uAdjust;
	tlvert[1].tv *= vAdjust;
	tlvert[2].tu *= uAdjust;
	tlvert[2].tv *= vAdjust;
	tlvert[3].tu *= uAdjust;
	tlvert[3].tv *= vAdjust;

	sinDeg = _sin(DegToRad(pXformData->rz / 2.8444444444444));
	cosDeg = _cos(DegToRad(pXformData->rz / 2.8444444444444));

	for (iVert = 0; iVert < 4; iVert++)
	{
		tx = tlvert[iVert].x;
		ty = tlvert[iVert].y;
		rx = tx * cosDeg - ty * sinDeg;
		ry = tx * sinDeg + ty * cosDeg;

		tlvert[iVert].x = m_tlvertX + ((pXformData->x - 320) + rx) * pixelRatio;
		tlvert[iVert].y = m_tlvertY + ((pXformData->y - 240) + ry) * pixelRatio;
	}

	color = PACK32BIT8888((int)pXformData->crA, (int)pXformData->crR, (int)pXformData->crG, (int)pXformData->crB);

	tlvert[0].color = color;
	tlvert[1].color = color;
	tlvert[2].color = color;
	tlvert[3].color = color;

	tlvert[0].specular = 0xffffffff;
	tlvert[1].specular = 0xffffffff;
	tlvert[2].specular = 0xffffffff;
	tlvert[3].specular = 0xffffffff;

	tlvert[0].z = (1.0f/32236.0f);
	tlvert[1].z = (1.0f/32236.0f);
	tlvert[2].z = (1.0f/32236.0f);
	tlvert[3].z = (1.0f/32236.0f);

	tlvert[0].oow = oow;
	tlvert[1].oow = oow;
	tlvert[2].oow = oow;
	tlvert[3].oow = oow;



	rpFace.tex = tex;
	rpFace.srcAlphaMode = pXformData->srcalpha;
	rpFace.destAlphaMode = pXformData->destalpha;
	rpFace.mtPreset = pXformData->mtpreset;

	g_renderer->AddRP(&rpFace, RF_EFFECT);
}

void	CRagEffect :: Entry()
{
	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = 55;
		g_prim->m_matrix.MakeXRotation(GetRadian((int)g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_accel = -0.00007f;
		g_prim->m_longSpeed = -10.0f;
		g_prim->m_outerSize = 6.0f;
		g_prim->m_outerSpeed = 0.08f;
		g_prim->m_innerSize = 5;
		g_prim->m_heightSize = 6.5f;
		g_prim->m_heightSpeed = -0.1f;
		g_prim->m_alpha = 5;
		g_prim->m_maxAlpha = 245;
		g_prim->m_alphaSpeed = 10;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 5);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);
	}

	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = 55;
		g_prim->m_arcAngle = 45;
		g_prim->m_longSpeed = 10.0f;
		g_prim->m_outerSize = 4.5f;
		g_prim->m_innerSize = 4.5f;
		g_prim->m_heightSpeed = 2.5f;
		g_prim->m_heightAccel = -(g_prim->m_heightSpeed / g_prim->m_duration) * 2.0f;
		g_prim->m_alpha = 160;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);
	}
}

void	CRagEffect :: Hit1()
{
	if (m_stateCnt == 0)
	{
		int		angle = 180 - m_master->m_roty;

		if (angle < 0)
			angle += 360;

		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = 10;
		g_prim->m_longitude = angle;
		g_prim->m_latitude = -90;
		g_prim->m_matrix.MakeXRotation(GetRadian((int)g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos2.Set(0, -10, 0);
		g_prim->m_speed = 0.7f;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
		g_prim->m_outerSize = 10;
		g_prim->m_innerSize = 5;
		g_prim->m_heightSize = 3.5f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 2);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);

		if (!g_session.m_isMinEffect)
		{
			int i ;
			for (i = 0; i < 2; i++)
			{
				g_prim = LaunchEffectPrim(PP_3DPARTICLE);
				g_prim->m_longitude = random(80) + angle - 40;
				g_prim->m_latitude = -50 + random(100);
				g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
				g_prim->m_matrix.AppendXRotation(GetRadian((int)g_prim->m_latitude));
				g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, random(4) + 4), g_prim->m_matrix);
				g_prim->m_deltaPos2 = vector3d(0, -10, 0) + g_prim->m_deltaPos3;
				g_prim->m_speed = (float)(random(90) + 60) / 100;
				g_prim->m_duration = random(24) + 6;
				g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
				g_prim->m_size = (float)(random(100) + 60) / 100;

				g_prim->m_fadeOutCnt = 0;
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
				g_prim->m_numSegment = 3;
				g_prim->InitSegment();
			}

			for (i = 0; i < 2; i++) {
				g_prim = LaunchEffectPrim(PP_3DPARTICLEGRAVITY);
				g_prim->m_longitude = random(80) + angle + 140;
				g_prim->m_latitude = -50 + random(100);
				g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
				g_prim->m_matrix.AppendXRotation(GetRadian((int)g_prim->m_latitude));
				g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, random(4) + 4), g_prim->m_matrix);
				g_prim->m_deltaPos2 = vector3d(0, -10, 0) + g_prim->m_deltaPos3;
				g_prim->m_matrix.MakeYRotation(GetRadian(g_prim->m_longitude));
				g_prim->m_duration = random(24) + 6;
				g_prim->m_speed = (float)(random(90) + 60) / 100;
				g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
				g_prim->m_gravSpeed = -(float)(random(90) + 30) / 100;
				g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration * 2.0f);
				g_prim->m_size = (float)(random(100) + 60) / 100;

				g_prim->m_fadeOutCnt = 0;
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
				g_prim->m_numSegment = 3;
				g_prim->InitSegment();
			}
		}
	}
}

void	CRagEffect :: Hit2(int option)
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_hit2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		float	length;
		int		tmpCnt = 0;

		for (float i = 0; i < 360; i += 45, tmpCnt++)
		{
			g_prim = LaunchEffectPrim(PP_2DTEXTURE);
			g_prim->m_pattern = PT2DTEX_SIZEUP;
			g_prim->m_duration = random(20) + 10;
			g_prim->m_longitude = g_prim->m_roll = (i - 15) + random(30);
			g_prim->m_speed = (float)(random(45) + 5) / 10.0f;
			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
			g_prim->m_widthSize = random(15) + 5;
			g_prim->m_heightSize = random(20) + 20;
			g_prim->m_widthSpeed = -(g_prim->m_widthSize / g_prim->m_duration);
			g_prim->m_heightSpeed = 1.5f;
			g_prim->m_heightAccel = 0.25f;
			length = random(5);
			g_prim->m_deltaPos2.x = length * GetSin(i);
			g_prim->m_deltaPos2.y = -length * GetCos(i);
			g_prim->m_deltaPos2.y -= 20;
			if (option) {









				g_prim->m_red = 120;
				g_prim->m_green = 120;
			}
			g_prim->m_alpha = 0;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 8;
			g_prim->m_GI[0].life=0;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			if (tmpCnt % 2 == 0)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\lens1.tga", false);
			else
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\lens2.tga", false);
		}
	}
}

void	CRagEffect :: Hit3()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_hit3.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		int		angle = 180 - m_master->m_roty;

		if (angle < 0)
			angle += 360;

		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = 15;
		g_prim->m_longitude = angle;
		g_prim->m_latitude = -90;
		g_prim->m_matrix.MakeXRotation(GetRadian((int)g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos2.Set(0, -10, 0);
		g_prim->m_speed = 0.7f;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
		g_prim->m_outerSize = 1.5f;
		g_prim->m_innerSize = 1.5f;
		g_prim->m_heightSpeed = 0.5f;
		g_prim->m_heightAccel = 0.2f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\lens2.tga", false);

		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = 15;
		g_prim->m_longitude = angle;
		g_prim->m_latitude = -90;
		g_prim->m_matrix.MakeXRotation(GetRadian((int)g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos2 = vector3d(0, -10, 0);
		g_prim->m_speed = 0.7f;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
		g_prim->m_outerSize = 4.0f;
		g_prim->m_innerSize = 1.5f;
		g_prim->m_heightSpeed = 0.25f;
		g_prim->m_heightAccel = 0.2f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\lens2.tga", false);

		for (int i = 0; i < 8; i++) {
			g_prim = LaunchEffectPrim(PP_3DPARTICLE);
			g_prim->m_longitude = (angle - 40) + random(80);
			g_prim->m_latitude = -90 + 40 + random(100);
			g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
			g_prim->m_matrix.AppendXRotation(GetRadian((int)g_prim->m_latitude));
			g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, random(4) + 4), g_prim->m_matrix);
			g_prim->m_deltaPos2 = vector3d(0, -10, 0) + g_prim->m_deltaPos3;
			g_prim->m_speed = (float)(random(120) + 80) / 100;
			g_prim->m_duration = random(24) + 6;
			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
			g_prim->m_size = (float)(random(100) + 60) / 100;
			g_prim->m_sizeSpeed = -(g_prim->m_size / g_prim->m_duration);
			g_prim->m_fadeOutCnt = 0;
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
			g_prim->m_numSegment = 3;
			g_prim->InitSegment();
		}
	}
}

void	CRagEffect :: Hit4()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_hit4.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		int		angle = 180 - m_master->m_roty;

		if (angle < 0)
			angle += 360;

		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = 15;
		g_prim->m_longitude = angle;
		g_prim->m_latitude = -90;
		g_prim->m_matrix.MakeXRotation(GetRadian((int)g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos2 = vector3d(0, -10, 0);
		g_prim->m_speed = 0.7f;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
		g_prim->m_outerSize = 4.0f;
		g_prim->m_innerSize = 0.5f;
		g_prim->m_heightSpeed = 0.25f;
		g_prim->m_heightAccel = 0.15f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\lens2.tga", false);

		for (int i = 0; i < 5; i++) {
			g_prim = LaunchEffectPrim(PP_3DPARTICLE);
			g_prim->m_longitude = angle + random(80) - 40;
			g_prim->m_latitude = random(100) + (-90 + 40);
			g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
			g_prim->m_matrix.AppendXRotation(GetRadian((int)g_prim->m_latitude));
			g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, random(4) + 4), g_prim->m_matrix);
			g_prim->m_deltaPos2 = vector3d(0, -10, 0) + g_prim->m_deltaPos3;
			g_prim->m_speed = (float)(random(90) + 60) / 100;
			g_prim->m_duration = random(24) + 6;
			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
			g_prim->m_size = (float)(random(100) + 60) / 100;
			g_prim->m_sizeSpeed = -(g_prim->m_size / g_prim->m_duration);
			g_prim->m_fadeOutCnt = 0;
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
			g_prim->m_numSegment = 3;
			g_prim->InitSegment();
		}
	}
}

void	CRagEffect :: Hit5()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_hit5.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		int		angle[2];

		angle[0] = random(360);
		angle[1] = angle[0] + 90;

		for (int i = 0; i < 2; i++) {
			g_prim = LaunchEffectPrim(PP_2DTEXTURE);
			g_prim->m_duration = 17;
			g_prim->m_deltaPos2.y = -20;
			g_prim->m_roll = angle[i];
			g_prim->m_rollSpeed = -5.0f;
			g_prim->m_rollAccel = -(g_prim->m_rollSpeed / g_prim->m_duration) / 1.5f;
			g_prim->m_widthSize = random(5) + 20;
			g_prim->m_heightSize = random(10) + 30;
			g_prim->m_widthSpeed = -(g_prim->m_widthSize / g_prim->m_duration);
			g_prim->m_heightSpeed = 2.5f;
			g_prim->m_heightAccel = 0.5f;
			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 2);
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\lens2.tga", false);
			g_prim->m_GI[0].life=0;
		}
	}
}

void	CRagEffect :: Hit6()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_hit6.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		int		angle[2];

		angle[0] = random(360);
		angle[1] = angle[0] + 90;

		for (int i = 0; i < 2; i++) {
			g_prim = LaunchEffectPrim(PP_2DTEXTURE);
			g_prim->m_duration = 17;
			g_prim->m_deltaPos2.y = -20;
			g_prim->m_roll = angle[i];
			g_prim->m_rollSpeed = -5.0f;
			g_prim->m_rollAccel = -(g_prim->m_rollSpeed / g_prim->m_duration) / 1.5f;
			g_prim->m_widthSize = random(5) + 10;
			g_prim->m_heightSize = random(5) + 15;
			g_prim->m_widthSpeed = -(g_prim->m_widthSize / g_prim->m_duration);
			g_prim->m_heightSpeed = 1.7f;
			g_prim->m_heightAccel = 0.5f;
			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 2);
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\lens2.tga", false);
			g_prim->m_GI[0].life=0;
		}
	}
}

void	CRagEffect :: Exit()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = m_duration = 100;

		g_prim->m_outerSize = 4.5f;
		g_prim->m_innerSize = 4.5f;
		g_prim->m_heightSize = 35.0f;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 90;
		g_prim->m_alphaSpeed = (g_prim->m_maxAlpha / 30 );
		g_prim->m_fadeOutCnt = g_prim->m_duration - 20;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);

		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("_heal_effect.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!g_session.m_isMinEffect)
	{
		if (m_stateCnt % 9 == 0)
		{
			float	rndNum;

			g_prim = LaunchEffectPrim(PP_3DPARTICLEORBIT);
			rndNum = random(200) / 100;
			g_prim->m_radius = 3.0f;

			g_prim->m_duration = 50;
			g_prim->m_longitude = random(360);

			g_prim->m_gravSpeed = -1.2f;
			g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration) / 1.5f;
			g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
			g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, g_prim->m_radius), g_prim->m_matrix);
			g_prim->m_size = 0.55f;
			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
			g_prim->InitSegment();
		}
	}
}

void	CRagEffect :: Warp()
{
	if (m_stateCnt % 20 == 0)
	{
		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = m_duration;
		g_prim->m_renderFlag |= RF_NODEPTHCHECK;
		g_prim->m_radius = 2;
		g_prim->m_arcAngle = 22.5f;


		g_prim->m_innerSize = 10.0f;

		g_prim->m_latitude = 90;
		g_prim->m_matrix.MakeXRotation(DegToRad(g_prim->m_latitude));
		g_prim->m_radiusSpeed = 1.25f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / g_prim->m_duration) / 2.0f;
		g_prim->m_alpha = 200;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 2);
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("EFFECT\\RING_YELLOW.TGA", false);
	}

	if (m_stateCnt % 2 == 0)
	{
		float	rndNum;

		g_prim = LaunchEffectPrim(PP_3DPARTICLEORBIT);
		rndNum = random(200) / 100;
		g_prim->m_radius = 1.0f + (rndNum * 0.7f);
		g_prim->m_duration = 40;
		g_prim->m_longitude = random(360);
		g_prim->m_longSpeed = ((rndNum * 3) + 3.0f);
		g_prim->m_gravSpeed = (float)(-random(240) + 60) / 100;
		g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration) / 2.0f;
		g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, g_prim->m_radius), g_prim->m_matrix);
		g_prim->m_size = 0.2f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
		g_prim->InitSegment();
	}
}

void	CRagEffect :: BeginSpell()
{
	m_pos = m_master->m_pos;
	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		SAINTCASTING(45,"effect\\ring_yellow.tga",1);
		SAINTCASTING(25,"effect\\ring_yellow.tga",1);
	}
}

void CRagEffect::BeginSpell2()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		BeginCasting("effect\\ring_blue.tga",2);
	}
}

void CRagEffect::BeginSpell3()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		BeginCasting("effect\\ring_yellow.tga",1);
	}
}

void CRagEffect::BeginSpell4()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		BeginCasting("effect\\Magic_Green.tga",33);
	}
}

void CRagEffect::BeginSpell5()
{
	m_pos = m_master->m_pos;
	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		BeginCasting("effect\\ring_yellow.tga", 0);
	}
}

void CRagEffect::BeginSpell6()
{
	m_pos = m_master->m_pos;
	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		SAINTCASTING(45,"effect\\ring_white.tga",0);
		SAINTCASTING(25,"effect\\ring_white.tga",0);
	}
}

void CRagEffect::BeginSpell7()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		BeginCasting("effect\\Magic_Violet.tga",33);
	}
}

bool	CRagEffect :: GlassWall()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_GlassWall.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		int		angle = 180 - m_master->m_roty;
		matrix	tmpMat;

		if (angle < 0)
			angle += 360;

		tmpMat.MakeYRotation(GetRadian(angle));

		for (int i = 1; i >= -1; i -= 2) {
			g_prim = LaunchEffectPrim(PP_3DTEXTURE);
			g_prim->m_duration = m_duration;
			g_prim->m_pattern = PT3DTEX_SIZEUP | PT3DTEX_WAVERINGLY;
			g_prim->m_any = 50;
			g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, 2.6f * i), tmpMat);
			g_prim->m_longitude = angle + 90;

			g_prim->m_widthSize = 3.0f;

			g_prim->m_heightSize = 40.0f;
			g_prim->m_heightSpeed = 0.25f;

			g_prim->m_alpha = 0;
			g_prim->m_maxAlpha = 180;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 6;
			g_prim->m_fadeOutCnt = g_prim->m_duration - 6;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);

			g_prim = LaunchEffectPrim(PP_3DTEXTURE);
			g_prim->m_duration = m_duration;
			g_prim->m_pattern = PT3DTEX_SIZEUP | PT3DTEX_WAVERINGLY;
			g_prim->m_any = 50;
			g_prim->m_deltaPos2.MatrixMult(vector3d(3.0f * i, 0, 0), tmpMat);
			g_prim->m_longitude = angle;
			g_prim->m_widthSize = 2.6f;
			g_prim->m_heightSize = 40.0f;
			g_prim->m_heightSpeed = 0.25f;
			g_prim->m_alpha = 0;
			g_prim->m_maxAlpha = 180;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 6;
			g_prim->m_fadeOutCnt = g_prim->m_duration - 6;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);
		}
	}

	ProcessEZ2STR();
	if (m_stateCnt == m_duration)
		return false;
	else return true;
}

void	CRagEffect :: Blessing()
{
	m_pos = m_master->m_pos;
	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_Blessing.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = m_duration;
		g_prim->m_pattern |= PT_FILLCIRCLE;
		g_prim->m_renderFlag |= RF_NODEPTHCHECK;
		g_prim->m_radius = 10;
		g_prim->m_latitude = 90;
		g_prim->m_red = 0x20;
		g_prim->m_green = 0xb0;
		g_prim->m_blue = 0xe8;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 100;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 30;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 30;
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);
	}

	if (m_stateCnt % 3 == 0 && m_stateCnt < 10) {
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = m_duration;
		g_prim->m_animSpeed = 2;
		g_prim->m_alpha = 200 - m_stateCnt * 6;
		g_prim->m_deltaPos2.Set(0, -25, 0);

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc3\xe0\xba\xb9.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc3\xe0\xba\xb9.act");

		g_prim->InitSegment();
	}

	if (m_stateCnt % 4 == 0) {
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = 80;
		g_prim->m_pattern |= PT_TWINKLE;
		g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, -25, random(7) + 2), g_prim->m_matrix);
		g_prim->m_matrix.MakeXRotation(GetRadian(-90));
		g_prim->m_speed = (float)(random(20) + 20) / 100;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
		g_prim->m_alpha = 0;
		g_prim->m_size = 0.5f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 5);
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle6.act");
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle6.spr");
		g_prim->InitSegment();
	}
}

void	CRagEffect :: IncAGIDEX()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	m_pos = m_master->m_pos;

	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_IncAgiDex.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_2DTEXTURE);
		g_prim->m_duration = m_duration;
		g_prim->m_longitude = 0;
		g_prim->m_speed = 1.5f;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) * 1.2f;
		g_prim->m_widthSize = 40;
		g_prim->m_heightSize = 20;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 200;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 15;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 15;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\dex_agi_up.bmp", false);
		g_prim->m_GI[0].life = 0;
	}

	if (m_stateCnt % 2 == 0) {
		g_prim = LaunchEffectPrim(PP_3DCROSSTEXTURE);
		g_prim->m_duration = 50;
		g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, random(7) + 2), g_prim->m_matrix);
		g_prim->m_matrix.MakeXRotation(GetRadian(90));
		g_prim->m_speed = (float)(random(50) + 20) / 100;
		g_prim->m_widthSize = (float)(random(60) + 30) / 10;
		g_prim->m_heightSize = 0.18f;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 200;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 20;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 20;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ac_center2.tga", false);
		g_prim->m_GI[0].life = 0;
	}
}

void	CRagEffect :: HealSP()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = m_duration = 100;
		g_prim->m_longSpeed = 5.0f;
		g_prim->m_outerSize = 2.5f;
		g_prim->m_innerSize = 2.5f;
		g_prim->m_heightSize = 15.0f;
		g_prim->m_red = 0x20;
		g_prim->m_green = 0xb0;
		g_prim->m_blue = 0xe8;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 16;
		g_prim->m_alphaSpeed = (g_prim->m_maxAlpha / 30);
		g_prim->m_fadeOutCnt = g_prim->m_duration - 20;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);

		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = m_duration;
		g_prim->m_longSpeed = 5.0f;
		g_prim->m_outerSize = 3.5f;
		g_prim->m_innerSize = 3.5f;
		g_prim->m_heightSize = 25.0f;
		g_prim->m_red = 0x20;
		g_prim->m_green = 0xb0;
		g_prim->m_blue = 0xe8;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 32;
		g_prim->m_alphaSpeed = (g_prim->m_maxAlpha / 30);
		g_prim->m_fadeOutCnt = g_prim->m_duration - 20;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);

		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = m_duration;
		g_prim->m_longSpeed = 5.0f;
		g_prim->m_outerSize = 4.5f;
		g_prim->m_innerSize = 4.5f;
		g_prim->m_heightSize = 35.0f;
		g_prim->m_red = 0x20;
		g_prim->m_green = 0xb0;
		g_prim->m_blue = 0xe8;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 64;
		g_prim->m_alphaSpeed = (g_prim->m_maxAlpha / 30);
		g_prim->m_fadeOutCnt = g_prim->m_duration - 20;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);

		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("_heal_effect.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (m_stateCnt % 2 == 0) {
		float	rndNum;

		g_prim = LaunchEffectPrim(PP_3DPARTICLEORBIT);
		rndNum = random(200) / 100;
		g_prim->m_radius = 3.0f;
		g_prim->m_duration = 30;
		g_prim->m_longitude = random(360);
		g_prim->m_gravSpeed = -1.2f;
		g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration) / 1.5f;
		g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, g_prim->m_radius), g_prim->m_matrix);
		g_prim->m_size = 0.55f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle2.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle2.act");
		g_prim->InitSegment();
	}
}

 void	CRagEffect :: SoulStrike(char F1)
{
	int start, step, end;

	switch (m_count) {
		case 1: {
			start = 0;
			end = 1;
			step = 1;
			break;
		}
		case 2: {
			start = -90;
			end = 2;
			step = 180;
			break;
		}
		case 3: {
			start = -90;
			end = 3;
			step = 90;
			break;
		}
		case 4: {
			start = -90;
			end = 4;
			step = 60;
			break;
		}
		case 5: {
			start = -90;
			end = 5;
			step = 45;
			break;
		}
	}

	int i = start, dist = 11, delay = 5;
	if (((m_stateCnt + delay) % dist == 0) && ((m_stateCnt + delay) / dist) <= end) {
		i  = start + (((m_stateCnt + delay) / dist) * step);
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_SoulStrike.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_3DPARTICLESPLINE);
		g_prim->m_duration = 40;
		g_prim->m_deltaPos2.Set(0, -10, 0);
		g_prim->m_radius = g_prim->CalcDist(m_deltaPos.x, m_deltaPos.z);
		g_prim->CalcDir(m_deltaPos.x, m_deltaPos.z);
		g_prim->m_longitude = -g_prim->m_roty;
		g_prim->m_speed = g_prim->m_radius / g_prim->m_duration;
		g_prim->m_gravSpeed = 3.5f;
		g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration) * 2.0f;
		g_prim->m_latiSpeed = -2.0f;
		g_prim->m_latiAccel = -(g_prim->m_latiSpeed / (g_prim->m_duration + 0)) * 2.0f;
		g_prim->m_roll = i;
		g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, 1), g_prim->m_matrix);
		g_prim->m_rotAxis[0] = g_prim->m_deltaPos3.x;
		g_prim->m_rotAxis[1] = g_prim->m_deltaPos3.y;
		g_prim->m_rotAxis[2] = g_prim->m_deltaPos3.z;
		g_prim->m_size = 2.75f;
		if (F1 == 0)
		{
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
		}
		else
		{
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle5.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle5.act");
		}

		g_prim->m_numSegment = 12;
		g_prim->InitSegment();
	}
}

void	CRagEffect :: Bash()
{
	if (!g_session.m_isMinEffect)
	{
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		tlvertex3d tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

		m_tlvertX = tlvert.x;
		m_tlvertY = tlvert.y;

		if (m_stateCnt == 0)
		{
			vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
			PlayWave("effect\\EF_Bash.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			g_prim = LaunchEffectPrim(PP_2DCIRCLE);
			g_prim->m_duration = 40;
			g_prim->m_pattern |= PT_FILLCIRCLE;
			g_prim->m_deltaPos2.y = -40;
			g_prim->m_radius = 100;
			g_prim->m_alpha = 0;
			g_prim->m_maxAlpha = 170;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 6;
			g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);

			int loopCnt = 20;
			if (g_session.m_isMinEffect)
				loopCnt = 10;
			for (int i = 0; i < loopCnt; i++)
			{
				g_prim = LaunchEffectPrim(PP_2DFLASH);
				g_prim->m_duration = 40;
				g_prim->m_deltaPos2.y = -40;
				g_prim->m_longitude = random(360);
				g_prim->m_longSpeed = (float)(random(60) + 10) / 10.0f;
				g_prim->m_longAccel = -(g_prim->m_longSpeed / g_prim->m_duration) / 1.5f;
				g_prim->m_arcAngle = (float)(random(25) + 5) / 10.0f;
				g_prim->m_heightSize = random(40) + 20;
				g_prim->m_heightSpeed = (float)(random(30) + 20) / 10.0f;
				g_prim->m_alpha = 0;
				g_prim->m_maxAlpha = 200;
				g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;
				g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
				g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
				g_prim->m_texture[0] = g_texMgr.GetTexture("EFFECT\\alpha_center.tga", false);
			}
		}
	}
}

void	CRagEffect :: Detoxication()
{
	if (!g_session.m_isMinEffect)
	{
		if (m_stateCnt % 5 == 0) {
			if (m_stateCnt == 0) {
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\EF_Detoxication.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
			g_prim = LaunchEffectPrim(PP_3DPARTICLE);
			g_prim->m_alpha = 0;
			g_prim->m_duration = 40;
			g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
			g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, random(6) + 2), g_prim->m_matrix);
			g_prim->m_matrix.MakeXRotation(GetRadian(90));
			g_prim->m_speed = (float)(random(30) + 30) / 100;
			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
			g_prim->m_size = (float)(random(50) + 40) / 100;
			g_prim->m_rollSpeed = 3.0f;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;
			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 5);
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle2.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle2.act");
			g_prim->InitSegment();
		}
	} else {
		if (m_stateCnt % 15 == 0) {
			if (m_stateCnt == 0) {
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\EF_Detoxication.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
			g_prim = LaunchEffectPrim(PP_3DPARTICLE);
			g_prim->m_duration = 40;
			g_prim->m_alpha = 0;
			g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
			g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, random(6) + 2), g_prim->m_matrix);
			g_prim->m_matrix.MakeXRotation(GetRadian(90));
			g_prim->m_speed = (float)(random(30) + 30) / 100;
			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
			g_prim->m_size = (float)(random(50) + 40) / 100;
			g_prim->m_rollSpeed = 3.0f;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;
			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 5);
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle2.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle2.act");
			g_prim->InitSegment();
		}
	}
}

void	CRagEffect :: MagnumBreak()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_MagnumBreak.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = m_duration;
		g_prim->m_renderFlag |= RF_NODEPTHCHECK;
		g_prim->m_latitude = 90;
		g_prim->m_radiusSpeed = 1.75f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / g_prim->m_duration) / 2.0f;
		g_prim->m_innerSize = 12.0f;
		g_prim->m_alpha = 0;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 15;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 15;
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_yellow.tga", false);

		g_prim = LaunchEffectPrim(PP_3DSPHERE);
		g_prim->m_duration = m_duration;
		g_prim->m_radiusSpeed = 1.15f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / g_prim->m_duration) / 2.0f;
		g_prim->m_longSpeed = 3.0f;
		g_prim->m_alpha = 0 ;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 15;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 15;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\\xb4\xeb\xc6\xf8\xb9\xdf.tga", false);

		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = 30;
		g_prim->m_renderFlag |= RF_NODEPTHCHECK;
		g_prim->m_latitude = 90;
		g_prim->m_radiusSpeed = 1.75f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / g_prim->m_duration) / 2.0f;
		g_prim->m_innerSize = 12.0f;
		g_prim->m_alpha = 0;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 15;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 15;
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_yellow.tga", false);
	}
}

void	CRagEffect :: Steal()
{
#ifdef _2ND_EFFECT
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_Steal.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		for (int i = 0; i < 10; i++) {
			g_prim = LaunchEffectPrim(PP_3DPARTICLEGRAVITY);
			g_prim->m_longitude = random(360);
			g_prim->m_latitude = -90 + (40 + random(100));
			g_prim->m_matrix.MakeYRotation(GetRadian(g_prim->m_longitude));
			g_prim->m_matrix.AppendXRotation(GetRadian(g_prim->m_latitude));
			g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, random(4) + 4), g_prim->m_matrix);
			g_prim->m_deltaPos2 = vector3d(0, -10, 0) + g_prim->m_deltaPos3;
			g_prim->m_matrix.MakeYRotation(GetRadian(g_prim->m_longitude));
			g_prim->m_duration = m_duration;
			g_prim->m_speed = (float)(random(50) + 50) / 100;
			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
			g_prim->m_gravSpeed = -(float)(random(90) + 60) / 100;
			g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration) * 2.0f;

			g_prim->m_size = (float)(random(100) + 100) / 100;
			g_prim->m_sizeSpeed = -(g_prim->m_size / g_prim->m_duration);
			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle7.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle7.act");
			g_prim->InitSegment();
		}
	}
#else
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_Steal.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		for (int i = 0; i < 10; i++) {
			g_prim = LaunchEffectPrim(PP_3DPARTICLEGRAVITY);
			g_prim->m_longitude = random(360);
			g_prim->m_latitude = -90 + (40 + random(100));
			g_prim->m_matrix.MakeYRotation(GetRadian(g_prim->m_longitude));
			g_prim->m_matrix.AppendXRotation(GetRadian(g_prim->m_latitude));
			g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, random(4) + 4), g_prim->m_matrix);
			g_prim->m_deltaPos2 = vector3d(0, -10, 0) + g_prim->m_deltaPos3;
			g_prim->m_matrix.MakeYRotation(GetRadian(g_prim->m_longitude));
			g_prim->m_duration = m_duration;
			g_prim->m_speed = (float)(random(50) + 50) / 100;
			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
			g_prim->m_gravSpeed = -(float)(random(90) + 60) / 100;
			g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration) * 2.0f;

			g_prim->m_size = (float)(random(100) + 100) / 100;
			g_prim->m_sizeSpeed = -(g_prim->m_size / g_prim->m_duration);
			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle7.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle7.act");
			g_prim->InitSegment();
		}
	}
#endif
}

void	CRagEffect :: Smoke()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0) {
		int	num = random(4) + 1;
		for (int i = 0; i < num; i++) {
			g_prim = LaunchEffectPrim(PP_3DPARTICLE);
			g_prim->m_duration = m_duration;
			g_prim->m_matrix.MakeXRotation(GetRadian(75 + random(30)));
			g_prim->m_matrix.AppendYRotation(GetRadian(random(360)));
			g_prim->m_deltaPos2.y = -9.0f;
			g_prim->m_rollSpeed = 0.75f;
			g_prim->m_speed = (float)(random(3) + 3) / 10.0f;
			g_prim->m_size = 1.5f;

			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);

			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb1\xbc\xb6\xd2\xbf\xac\xb1\xe2.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb1\xbc\xb6\xd2\xbf\xac\xb1\xe2.act");

			g_prim->InitSegment();
		}
	}
}

void	CRagEffect :: FireFly()
{
	if (m_stateCnt == 0) {
		vector3d	deltaPos;

		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_pattern |= PT_CURVE;
		g_prim->m_pattern |= PT_TWINKLE;
		m_duration = g_prim->m_duration = 140;
		g_prim->m_speed = (float)(random(35) + 15) / 100;
		g_prim->m_longitude = random(360);
		g_prim->m_latitude = random(360);
		g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_matrix.AppendXRotation(GetRadian((int)g_prim->m_latitude));
		deltaPos.MatrixMult(vector3d(0, 0, random(13) + 2), g_prim->m_matrix);
		g_prim->m_deltaPos2 = vector3d(0, -10, 0) + deltaPos;
		g_prim->m_count = random(25) + 5;
		g_prim->m_size = (float)(random(70) + 30) / 100;

		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 80;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 1.5f;
		g_prim->m_AlphaShow = TRUE;

		g_prim->m_chCnt = 0;
		g_prim->m_chPoint.push_back(40);
		g_prim->m_chVal1.push_back(120);
		g_prim->m_chVal2.push_back(200);
		g_prim->m_chPoint.push_back(100);
		g_prim->m_chVal1.push_back(0);
		g_prim->m_chVal2.push_back(80);

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
		g_prim->InitSegment();
	}
}

void	CRagEffect :: SandWind()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt < 60 && m_stateCnt % 2 == 0) {
		g_prim = LaunchEffectPrim(PP_2DTEXTURE);
		g_prim->m_pattern = PT_BLOW;
		g_prim->m_duration = m_duration - random(100);
		g_prim->m_speed = (float)(random(300) + 260) / 100;

		g_prim->m_longitude = 130 + random(15);
		g_prim->m_widthSize = 30;
		g_prim->m_heightSize = 30;
		g_prim->m_deltaPos2.x = random(640) - 500;
		g_prim->m_deltaPos2.y = random(50) - 240;
		g_prim->m_count = random(10) + 10;
		g_prim->m_alpha = 128;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->m_totalTexture = 1;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\sand1.bmp", false);
		g_prim->m_GI[0].life=0;
	}
}

void	CRagEffect :: Torch()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_pattern |= PT_USEORGARGB;
		g_prim->m_duration = m_duration;
		g_prim->m_deltaPos2.y = -10;
		int par = m_param[0];

		if (m_param[0] <= 0)
			m_param[0] = 1.0f;
		if (m_param[1] < 1)
			m_param[1] = 1;

		g_prim->m_size = m_param[0];
		g_prim->m_animSpeed = m_param[1];

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\torch_01.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\torch_01.act");
		g_prim->InitSegment();
	}
}

void	CRagEffect :: SprayPond()
{
	if (m_stateCnt == 0) {
		vector3d	deltaPos;

		for (int i = 0; i < 360; i += 45) {
			g_prim = LaunchEffectPrim(PP_3DTEXTURE);
			g_prim->m_pattern = PT3DTEX_MOVETEXTUREV;
			g_prim->m_duration = 130;
			g_prim->m_matrix.MakeYRotation(GetRadian((int)(i + 22.5f)));
			deltaPos.MatrixMult(vector3d(0, 0, 12.2f), g_prim->m_matrix);
			g_prim->m_deltaPos2 = vector3d(0, -2, 0) + deltaPos;
			g_prim->m_rotMat.MakeYRotation(GetRadian((int)(90 + i + 22.5f)));
			g_prim->m_alpha = 120;
			g_prim->m_widthSize = 5.0f;
			g_prim->m_heightSize = 2.7f;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\pond_water.bmp", false);
		}
	}

	if (m_stateCnt % 10 == 0) {
		for (int i = 0; i < 360; i += 45) {
			g_prim = LaunchEffectPrim(PP_3DTEXTURE);
			g_prim->m_duration = 30;
			g_prim->m_matrix.MakeYRotation(GetRadian((int)(i + 22.5f)));
			g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, 13.8f), g_prim->m_matrix);
			g_prim->m_rotMat.MakeZRotation(GetRadian(90));
			g_prim->m_rotMat.AppendYRotation(GetRadian((int)(90 + i + 22.5f)));
			g_prim->m_alpha = 100;
			g_prim->m_fadeOutCnt = 0;
			g_prim->m_widthSize = 5.5f;
			g_prim->m_heightSize = 1.0f;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);
		}
	}

	if (m_stateCnt % 20 == 0) {
		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = 25;
		g_prim->m_radius = 13;
		g_prim->m_arcAngle = 10;
		g_prim->m_size = 1.25f;
		g_prim->m_latitude = 90;
		g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
		g_prim->m_radiusSpeed = 0.25f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / g_prim->m_duration);
		g_prim->m_alpha = 100;
		g_prim->m_fadeOutCnt = 0;
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_center.tga", false);
	}
}

void	CRagEffect :: Banjjakii()
{
	m_pos = m_master->m_pos;
	int	i;

	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_pattern |= PT_USEORGARGB;
		g_prim->m_duration = 125;
		g_prim->m_deltaPos2.y = -10;
		int par = m_param[0];

		if (m_param[0] <= 0)
			m_param[0] = 1.0f;
		if (m_param[1] < 1)
			m_param[1] = 1;

		g_prim->m_animSpeed = m_param[1];

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc5\xa9\xb8\xae\xbd\xba\xb8\xb6\xbd\xba.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc5\xa9\xb8\xae\xbd\xba\xb8\xb6\xbd\xba.act");

		i = rand()%10;
		if( i%3==1 ) {
			g_prim->m_size = 0.01f;
		} else {
			g_prim->m_size = 1.0f;
		}
		g_prim->SetAction(i/2, 1, MT_ONETIME);
		g_prim->InitSegment();
	}
}

void	CRagEffect :: Enhance()
{
	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = m_duration;
		g_prim->m_arcAngle = 22.5f;
		g_prim->m_size = 7.0f;
		g_prim->m_latitude = 90;
		g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
		g_prim->m_alpha = 128;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_yellow.tga", false);

		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = m_duration;
		g_prim->m_outerSize = 6;
		g_prim->m_innerSize = 6;
		g_prim->m_heightSpeed = 2.0f;
		g_prim->m_heightAccel = -(g_prim->m_heightSpeed / g_prim->m_duration) / 1.5f;
		g_prim->m_maxHeightSize = 50;
		g_prim->m_alpha = 128;

		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);
	}

	if (!g_session.m_isMinEffect) {
		if (m_stateCnt % 5 == 0) {
			g_prim = LaunchEffectPrim(PP_3DCROSSTEXTURE);
			g_prim->m_duration = 50;
			g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
			g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, random(5) + 2), g_prim->m_matrix);
			g_prim->m_matrix.MakeXRotation(GetRadian(90));
			g_prim->m_speed = 2.0f;
			g_prim->m_widthSize = 7.0f;
			g_prim->m_heightSize = 0.25f;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);
			g_prim->m_GI[0].life=0;
		}
	} else {
		if (m_stateCnt % 15 == 0) {
			g_prim = LaunchEffectPrim(PP_3DCROSSTEXTURE);
			g_prim->m_duration = 50;
			g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
			g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, random(5) + 2), g_prim->m_matrix);
			g_prim->m_matrix.MakeXRotation(GetRadian(90));
			g_prim->m_speed = 2.0f;
			g_prim->m_widthSize = 7.0f;
			g_prim->m_heightSize = 0.25f;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);
			g_prim->m_GI[0].life=0;
		}
	}
}

void	CRagEffect :: Snow()
{
	CWorld* world = g_modeMgr.GetGameMode()->GetWorld();
	vector3d & pPos = world->m_player->m_pos;

	if (m_stateCnt == 0) {

	}
	m_pos = pPos;

	if (m_duration - m_stateCnt > 160) {
		for( int i=0; i<2 ; i++ ) {
		g_prim = LaunchEffectPrim(PP_3DPARTICLE_NOMASTER);
		g_prim->m_orgPos = m_pos;
		g_prim->m_duration = 320;
		g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
		g_prim->m_deltaPos2.MatrixMult
			(vector3d(0, -100.0f+world->m_attr->GetHeight(m_pos.x, m_pos.z), random(300) - 300), g_prim->m_matrix);
		g_prim->m_matrix.MakeXRotation(GetRadian(90));
		g_prim->m_speed = -0.5f;
		g_prim->m_accel = 0.0f;
		g_prim->m_size = (float)(random(15) + 25) / 50;
		g_prim->m_rollSpeed = 0.0f;
		g_prim->m_alpha = 0;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 5);
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\ef_snow.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\ef_snow.act");
		g_prim->InitSegment();
		}
	}
}

#ifndef _2ND_EFFECT
bool	CRagEffect :: Coin()
{
	if (!g_session.m_isMinEffect) {
		if (m_stateCnt == 20) {
			vector3d	deltaPos;

			int loopCnt = 5;
			if (g_session.m_isMinEffect)
				loopCnt = 2;
			for (int i = 0; i < loopCnt; i++) {
				g_prim = LaunchEffectPrim(PP_3DPARTICLEGRAVITY);
				g_prim->m_duration = random(30) + 40;

				g_prim->m_longitude = random(12) * 30;
				g_prim->m_matrix.MakeYRotation(GetRadian(g_prim->m_longitude));

				deltaPos.MatrixMult(vector3d(0, 0, random(2) + 2), g_prim->m_matrix);
				g_prim->m_deltaPos2 = vector3d(0, -10, 0) + deltaPos;
				g_prim->m_matrix.MakeYRotation(GetRadian(g_prim->m_longitude));
				g_prim->m_speed = (float)(random(50) + 50) / 100;
				g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 3.5f;
				g_prim->m_gravSpeed = -(float)(random(100) + 50) / 25;

				g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration * 4.0f);
				g_prim->m_size = 2.0f;
				g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
				g_prim->m_playWaveType = 1;
				g_prim->m_animSpeed = 1;
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb5\xbf\xc0\xfc.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb5\xbf\xc0\xfc.act");
				g_prim->m_numSegment = 2;
				g_prim->InitSegment();
			}
		}
	}

	ProcessEZ2STR();

	if (100 <= m_stateCnt)
		return false;
	return true;
}
#endif

void	CRagEffect :: Endure()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_Endure.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_2DTEXTURE);
		g_prim->m_duration = m_duration;
		g_prim->m_pattern |= PT2DTEX_SIZEACCELLIMIT;
		g_prim->m_deltaPos2.y = -20;
		g_prim->m_widthSize = 150;
		g_prim->m_heightSize = 150;
		g_prim->m_widthSpeed = -5;
		g_prim->m_heightSpeed = -5;
		g_prim->m_count = g_prim->m_duration - 35;
		g_prim->m_widthAccel = -g_prim->m_widthSpeed / g_prim->m_count;
		g_prim->m_heightAccel = -g_prim->m_heightSpeed / g_prim->m_count;
		g_prim->m_alpha = 0;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 20;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\endure.tga", false);
		g_prim->m_GI[0].life=0;
	}

	if (m_stateCnt < 50) {
		float	length;

		g_prim = LaunchEffectPrim(PP_2DTEXTURE);
		g_prim->m_duration = 40;
		g_prim->m_longitude = g_prim->m_roll = random(360);
		g_prim->m_speed = -4.0f;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration);
		g_prim->m_widthSize = (float)(random(100) + 60) / 100;
		g_prim->m_heightSize = random(20) + 30;
		g_prim->m_alpha = 0;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 20;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		length = random(40) + 100;
		g_prim->m_deltaPos2.x = length * GetSin(g_prim->m_longitude);
		g_prim->m_deltaPos2.y = -length * GetCos(g_prim->m_longitude);
		g_prim->m_deltaPos2.y -= 20;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);
		g_prim->m_GI[0].life=0;
	}
}

void	CRagEffect :: Sight()
{
	m_pos = m_master->m_pos;

	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt % 2 == 0)
	{
		float	degree = (m_stateCnt) * -5.0f;
		float	sn = GetSin(degree);
		float	cs = GetCos(degree);
		float ts = GetRadian(degree);
		float		radius = 15;

		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = 20;
		g_prim->m_deltaPos2.Set(radius * sn, -20, -radius * cs);
		g_prim->m_deltaPosAccel.y = 0.1f;

		g_prim->m_animSpeed = 3;
		g_prim->m_size = 2.5f;
		g_prim->m_sizeSpeed = -0.1f;
		g_prim->m_alpha = 150;
		g_prim->m_alphaSpeed = -3.0f;

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\Sight.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\Sight.act");
		g_prim->InitSegment();

		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = 20;
		g_prim->m_deltaPos2.Set(radius * sn, 0, -radius * cs);
		g_prim->m_animSpeed = 2;
		g_prim->m_size = 1.5f;
		g_prim->m_sizeSpeed = -0.06f;
		g_prim->m_alpha = 120;
		g_prim->m_alphaSpeed = -1.0f;

		g_prim->SetSprName("Shadow.spr");
		g_prim->SetActName("Shadow.act");

		g_prim->InitSegment();
	}
}

void	CRagEffect :: Sight2()
{
	m_pos = m_master->m_pos;

	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt % 2 == 0)
	{
		float	degree = (m_stateCnt) * -5.0f;
		float	sn = GetSin(degree);
		float	cs = GetCos(degree);
		float ts = GetRadian(degree);
		float		radius = 15;

		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = 20;
		g_prim->m_deltaPos2.Set(radius * sn, -20, -radius * cs);
		g_prim->m_deltaPosAccel.y = 0.1f;
		g_prim->m_animSpeed = 3;
		g_prim->m_size = 2.5f;
		g_prim->m_sizeSpeed = -0.1f;
		g_prim->m_alpha = 50;
		g_prim->m_alphaSpeed = -3.0f;

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\Sight.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\Sight.act");
		g_prim->InitSegment();
	}

}

bool	CRagEffect :: StoneCurse()
{
	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

void	CRagEffect :: FireBall()
{
	if (m_stateCnt == 20 || m_stateCnt == 24 || m_stateCnt == 28 || m_stateCnt == 32) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_FireBall.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = 20;
		g_prim->m_pattern |= PT_CHKTARGETPOS | PT_ROTATESPR;
		g_prim->m_deltaPos2.Set(0, -15, 0);
		g_prim->CalcDir(m_deltaPos.x, m_deltaPos.z);
		g_prim->m_longitude = -g_prim->m_roty;
		g_prim->m_roty = 180 - g_prim->m_roty;
		if (g_prim->m_roty < 0)
			g_prim->m_roty += 360;

		g_prim->m_speed = -(g_prim->CalcDist(m_deltaPos.x, m_deltaPos.z) / g_prim->m_duration);
		g_prim->m_matrix.MakeYRotation(GetRadian(g_prim->m_longitude));
		g_prim->m_size = 1.3f;
		g_prim->m_animSpeed = 1;
		if (m_stateCnt != 20) {
			g_prim->m_red = 246;
			g_prim->m_green = 199;
			g_prim->m_blue = 76;
			if (m_stateCnt == 24)
				g_prim->m_alpha = 180;
			else if (m_stateCnt == 28)
				g_prim->m_alpha = 130;
			else
				g_prim->m_alpha = 80;
		}
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\fireball.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\fireball.act");

		g_prim->InitSegment();
	}
}

bool	CRagEffect :: FireWall()
{
	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

void	CRagEffect :: IceArrow()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 12) {
		char buf[50];
		sprintf(buf, "effect\\EF_IceArrow%d.wav", random(3) + 1);
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave(buf, m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		m_count = m_count * 10;
	}

	if (m_stateCnt >= 12 && (m_stateCnt - 12) % 10 == 0 && (m_stateCnt - 12) < m_count) {
		float		tempVal, angle;

		g_prim = LaunchEffectPrim(PP_3DCROSSTEXTURE);
		g_prim->m_duration = 70;
		g_prim->m_pattern = PT3DTEX_CHECKGROUND | PT3DTEX_MOVEDUMMY;
		g_prim->m_deltaPos2.Set(30 + random(10) - 5, -60, 20 + random(10) - 5);

		angle = g_prim->CalcDir(g_prim->m_deltaPos2.x, g_prim->m_deltaPos2.z, random(4) - 2, random(4) - 2);
		m_longitude = g_prim->m_longitude = -angle;

		tempVal = _sqrt((fabs(g_prim->m_deltaPos2.x) * fabs(g_prim->m_deltaPos2.x)) +
					   (fabs(g_prim->m_deltaPos2.z) * fabs(g_prim->m_deltaPos2.z)));

		angle = g_prim->CalcDir(-fabs(g_prim->m_deltaPos2.y), tempVal, 0, 0);
		m_latitude = g_prim->m_latitude = angle;

		g_prim->m_matrix.MakeXRotation(GetRadian((int)g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_speed = -2.75f;
		g_prim->m_widthSize = 11.5f;
		g_prim->m_heightSize = 3.8f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 20;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\icearrow.tga", false);
		g_prim->m_GI[0].life=0;
	}

	if (m_flag) {
		m_flag = 0;
		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = 30;

		g_prim->m_latitude = 90;
		g_prim->m_radiusSpeed = 1.2f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / (g_prim->m_duration + 40)) * 2.0f;
		g_prim->m_innerSize = 10.0f;
		g_prim->m_alpha = 0;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);
	}
}

void	CRagEffect :: FrostDiver()
{
	if (!g_session.m_isMinEffect)
	{
		if (m_stateCnt == 0)
		{
			matrix	tmpMat;
			CalcDir(m_deltaPos.x, m_deltaPos.z);
			tmpMat.MakeYRotation(GetRadian(-m_roty));
			m_deltaPos2.MatrixMult(vector3d(0, 0, 5.0f), tmpMat);
			m_deltaPos3 -= m_deltaPos2;
			m_deltaPos2.MatrixMult(vector3d(0, 0, 2.0f), tmpMat);
		}

		if (!m_flag)
		{
			vector3d	tmpVec = m_pos;

			m_pos = m_pos + m_deltaPos3;
			if (CalcDist(m_deltaPos.x, m_deltaPos.z) <= 2.5f)
				m_flag = 1;

			m_pos = tmpVec;

			int		angle;
			float	sn, cs, length;

			g_prim = LaunchEffectPrim(PP_3DQUADHORN);
			g_prim->m_duration = 40;
			g_prim->m_pattern |= PTQH_SPEEDLIMIT;

			angle = random(360);
			length = (float)(random(10) + 5) / 10.0f;
			sn = GetSin(angle);
			cs = GetCos(angle);

			g_prim->m_deltaPos2.Set(length * sn, 30, -length * cs);
			g_prim->m_deltaPos2 += m_deltaPos3;
			g_prim->m_size = (float)(random(40) + 60) / 100.0f;
			g_prim->m_heightSize = (float)(random(30) + 150) / 10.0f;

			g_prim->m_longitude = random(360);
			g_prim->m_latitude = random(30) + 75;
			g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
			g_prim->m_matrix.AppendYRotation(GetRadian(g_prim->m_longitude));
			g_prim->m_rotMat = g_prim->m_matrix;
			g_prim->m_speed = 3.0f;
			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) * 2.0f;
			g_prim->m_count = 20;

			g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ice.tga", false);

			m_deltaPos3 -= m_deltaPos2;
		}
	}
}

void	CRagEffect :: FrostDiver2()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_FrostDiver2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		int		angle;
		float	sn, cs, length;

		for (int i = 0; i < 8; i++) {
			g_prim = LaunchEffectPrim(PP_3DQUADHORN);
			g_prim->m_duration = 40;
			g_prim->m_pattern |= PTQH_SPEEDLIMIT;

			angle = random(360);
			length = (float)(random(4) + 1) / 10.0f;
			sn = GetSin(angle);
			cs = GetCos(angle);

			g_prim->m_deltaPos2.Set(length * sn, 30, -length * cs);
			g_prim->m_size = (float)(random(250) + 100) / 100.0f;
			g_prim->m_heightSize = (float)(random(100) + 200) / 10.0f;
			g_prim->m_longitude = random(360);
			g_prim->m_latitude = random(20) + 80;
			g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
			g_prim->m_matrix.AppendYRotation(GetRadian(g_prim->m_longitude));
			g_prim->m_rotMat = g_prim->m_matrix;
			g_prim->m_speed = 3.0f;
			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) * 2.0f;
			g_prim->m_count = 20;
			g_prim->m_alpha = 200;
			g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ice.tga", false);
		}
	}
}

void	CRagEffect :: Ruwach()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt % 3 == 0) {
		float	degree = (m_stateCnt / 1.3) * -5.0f;
		float		radius = 15;
		float	sn = GetSin(degree);
		float	cs = GetCos(degree);
		float ts = GetRadian(degree);

		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = 25;
		g_prim->m_deltaPos2.Set(radius * sn, -10, -radius * cs);

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle2.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle2.act");

		g_prim->m_size = 3.0f;
		g_prim->m_sizeSpeed = -0.1f;
		g_prim->m_alpha = 250;
		g_prim->m_alphaSpeed = -3;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 6;
		g_prim->InitSegment();

		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = 25;
		g_prim->m_deltaPos2.Set(radius * sn, 0, -radius * cs);

		g_prim->SetSprName("Shadow.spr");
		g_prim->SetActName("Shadow.act");

		g_prim->m_animSpeed = 2;
		g_prim->m_size = 1.5f;
		g_prim->m_sizeSpeed = -0.06f;
		g_prim->m_alpha = 150;
		g_prim->m_alphaSpeed = -2.5;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 6;
		g_prim->InitSegment();
	}
}

bool	CRagEffect :: LightBolt()
{
	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

#ifndef _2ND_EFFECT
bool	CRagEffect :: ThunderStorm()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\magician_thunderstorm.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}
#endif

void	CRagEffect :: ThunderStorm2()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_ThunderStorm.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = m_duration;
		g_prim->m_renderFlag |= RF_NODEPTHCHECK;
		g_prim->m_pattern |= PT_USEORGARGB;
		g_prim->m_deltaPos2.y = -4.5f;
		g_prim->m_animSpeed = 2;
		g_prim->m_size = 2.5f;

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbd\xe3\xb4\xf5\xbd\xba\xc5\xe8.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbd\xe3\xb4\xf5\xbd\xba\xc5\xe8.act");

		g_prim->InitSegment();
	}
}

void	CRagEffect :: FireArrow()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 12) {
		char buf[50];
		sprintf(buf, "effect\\EF_FireArrow%d.wav", random(3) + 1);
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave(buf, m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		m_count = m_count * 10;
	}

	if (m_stateCnt >= 12 && (m_stateCnt - 12) % 10 == 0 && (m_stateCnt - 12) < m_count) {
		float		tempVal, angle;

		g_prim = LaunchEffectPrim(PP_3DCROSSTEXTURE);
		g_prim->m_duration = 70;
		g_prim->m_pattern = PT3DTEX_CHECKGROUND | PT3DTEX_MOVEDUMMY;
		g_prim->m_deltaPos2.Set(30 + random(10) - 5, -60, 20 + random(10) - 5);

		angle = g_prim->CalcDir(g_prim->m_deltaPos2.x, g_prim->m_deltaPos2.z, random(4) - 2, random(4) - 2);
		m_longitude = g_prim->m_longitude = -angle;

		tempVal = _sqrt((fabs(g_prim->m_deltaPos2.x) * fabs(g_prim->m_deltaPos2.x)) +
					   (fabs(g_prim->m_deltaPos2.z) * fabs(g_prim->m_deltaPos2.z)));

		angle = g_prim->CalcDir(-fabs(g_prim->m_deltaPos2.y), tempVal, 0, 0);
		m_latitude = g_prim->m_latitude = angle;

		g_prim->m_matrix.MakeXRotation(GetRadian((int)g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_speed = -2.75f;
		g_prim->m_widthSize = 14.0f;
		g_prim->m_heightSize = 3.5f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 20;
		g_prim->m_totalTexture = 6;
		g_prim->m_animSpeed = 1;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\\xba\xd2\xc8\xad\xbb\xec" "1.tga", false);
		g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\\xba\xd2\xc8\xad\xbb\xec" "2.tga", false);
		g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\\xba\xd2\xc8\xad\xbb\xec" "3.tga", false);
		g_prim->m_texture[3] = g_texMgr.GetTexture("effect\\\xba\xd2\xc8\xad\xbb\xec" "4.tga", false);
		g_prim->m_texture[4] = g_texMgr.GetTexture("effect\\\xba\xd2\xc8\xad\xbb\xec" "5.tga", false);
		g_prim->m_texture[5] = g_texMgr.GetTexture("effect\\\xba\xd2\xc8\xad\xbb\xec" "6.tga", false);

		g_prim->m_GI[0].life=0;
	}

	if (m_stateCnt > 0 && m_stateCnt % 4 == 0) {
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = random(24) + 6;
		g_prim->m_longitude = m_longitude - 20 + random(40);
		g_prim->m_latitude = m_latitude - 10 + random(20);
		g_prim->m_matrix.MakeXRotation(GetRadian((int)g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos2 = m_deltaPos;
		g_prim->m_speed = (float)(random(90) + 60) / 100;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 1.5f;

		g_prim->m_size = (float)(random(30) + 20) / 100;
		g_prim->m_sizeSpeed = -(g_prim->m_size / g_prim->m_duration);
		g_prim->m_fadeOutCnt = 0;
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle4.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle4.act");
		g_prim->InitSegment();
	}

	if (m_flag) {
		m_flag = 0;
		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = 30;

		g_prim->m_latitude = 90;
		g_prim->m_radiusSpeed = 1.2f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / (g_prim->m_duration + 40)) * 2.0f;
		g_prim->m_innerSize = 10.0f;
		g_prim->m_alpha = 0;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_yellow.tga", false);
	}
}

void	CRagEffect :: NapalmBeat()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt < 15 && m_stateCnt >= m_stateCnt % 5) {
		if (m_stateCnt == 0) {
			vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
			PlayWave("effect\\EF_NapalmBeat.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		}
		char	buf[128];
		float	length, angle;

		g_prim = LaunchEffectPrim(PP_2DTEXTURE);
		g_prim->m_duration = 30;
		length = random(50) + 10;
		angle = random(360);
		g_prim->m_deltaPos2.x = length * GetSin((int)angle);
		g_prim->m_deltaPos2.y = -length * GetCos((int)angle);
		g_prim->m_deltaPos2.y -= 20;
		g_prim->m_widthSize = g_prim->m_heightSize = random(40) + 20;
		g_prim->m_alpha = 0;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->m_repeatAnim = false;
		g_prim->m_animSpeed = 2;
		g_prim->m_totalTexture = 8;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		for (int i = 0; i < g_prim->m_totalTexture; i++)
		{

			sprintf(buf, "effect\\\xc6\xf8\xb9\xdf%d.TGA", i + 1);

			g_prim->m_texture[i] = g_texMgr.GetTexture(buf, false);
		}
		g_prim->m_GI[0].life=0;
	}
}

void	CRagEffect::Teleportation()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_Teleportation.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = 60;
		g_prim->m_pattern |= PT_DISAPPEAR;
		g_prim->m_outerSize = 4.0f;
		g_prim->m_innerSize = 4.0f;
		g_prim->m_heightSpeed = 5.0f;
		g_prim->m_heightAccel = 0.25f;
		g_prim->m_longSpeed = 2.0f;
		g_prim->m_fadeOutCnt = 0;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);
	}
}

void	CRagEffect::ReadyPortal()
{
	if (m_stateCnt % 14 == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_ReadyPortal.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = 30;
		g_prim->m_deltaPos2.y = -1.0f;

		g_prim->m_innerSize = 8;
		g_prim->m_latitude = 90;
		g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
		g_prim->m_radiusSpeed = 1.0f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / g_prim->m_duration) / 2.0f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 2);
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);
	}
}

void	CRagEffect :: Portal()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = m_duration;
		g_prim->m_arcAngle = 36.0f;
		g_prim->m_outerSize = 3.5f;
		g_prim->m_innerSize = 3.5f;
		g_prim->m_longSpeed = 4.0f;
		g_prim->m_heightSize = 40.0f;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 128;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 6;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 6;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);

		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = m_duration;
		g_prim->m_arcAngle = 36.0f;
		g_prim->m_outerSize = 4.0f;
		g_prim->m_innerSize = 4.0f;
		g_prim->m_longSpeed = 5.0f;
		g_prim->m_heightSize = 50.0f;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 128;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 6;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 6;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);
	}

	if (m_stateCnt % 14 == 0) {
		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = 30;
		g_prim->m_deltaPos2.y = -1.0f;

		g_prim->m_innerSize = 8;
		g_prim->m_latitude = 90;
		g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
		g_prim->m_radiusSpeed = 1.0f;

		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / g_prim->m_duration) / 2.0f;

		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 2);
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);
	}
}

void	CRagEffect :: IncAgility()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	m_pos = m_master->m_pos;

	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_IncAgility.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_2DTEXTURE);
		g_prim->m_duration = m_duration;
		g_prim->m_longitude = 0;
		g_prim->m_speed = 1.5f;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) * 1.2f;
		g_prim->m_widthSize = 40;
		g_prim->m_heightSize = 20;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 200;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 15;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 15;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\agi_up.bmp", false);

		g_prim->m_GI[0].life = 99;
	}

	if (m_stateCnt % 2 == 0) {
		g_prim = LaunchEffectPrim(PP_3DCROSSTEXTURE);
		g_prim->m_duration = 50;
		g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, random(7) + 2), g_prim->m_matrix);
		g_prim->m_matrix.MakeXRotation(GetRadian(90));
		g_prim->m_speed = (float)(random(50) + 20) / 100;
		g_prim->m_widthSize = (float)(random(60) + 30) / 10;
		g_prim->m_heightSize = 0.18f;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 200;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 20;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 20;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ac_center2.tga", false);

		g_prim->m_GI[0].life = 99;
	}
}

void	CRagEffect :: DecAgility()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	m_pos = m_master->m_pos;

	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_DecAgility.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_2DTEXTURE);
		g_prim->m_duration = m_duration;
		g_prim->m_deltaPos2.y -= 80;
		g_prim->m_longitude = 180;
		g_prim->m_speed = 1.0f;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration);
		g_prim->m_widthSize = 40;
		g_prim->m_heightSize = 10;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 200;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 15;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 15;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\slow.bmp", false);
		g_prim->m_GI[0].life=0;
	}

	if (m_stateCnt % 2 == 0) {
		g_prim = LaunchEffectPrim(PP_3DCROSSTEXTURE);
		g_prim->m_duration = 50;
		g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, random(7) + 2), g_prim->m_matrix);
		g_prim->m_deltaPos2.y -= 20;
		g_prim->m_matrix.MakeXRotation(GetRadian(-90));

		g_prim->m_accel = 0.015f;
		g_prim->m_widthSize = (float)(random(60) + 30) / 10;
		g_prim->m_heightSize = 0.18f;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 200;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 20;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 20;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ac_center2.tga", false);
		g_prim->m_GI[0].life=0;
	}
}

void	CRagEffect :: Aqua()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_Aqua.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = m_duration;
		g_prim->m_deltaPos2.y = -20;
		g_prim->m_animSpeed = 2;
		g_prim->m_repeatAnim = false;

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbc\xba\xbc\xf6\xb6\xdf\xb1\xe2.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbc\xba\xbc\xf6\xb6\xdf\xb1\xe2.act");

		g_prim->InitSegment();
	}
}

void CRagEffect::FireSplashHit()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	int Angle = random(360);
	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_2DTEXTURE);
		g_prim->m_duration = 30;
		g_prim->m_deltaPos2.y = -20;
		g_prim->m_widthSize = 10.0f;
		g_prim->m_heightSize = 10.0f;
		g_prim->m_widthSpeed = 5.7f;
		g_prim->m_heightSpeed = 5.7f;
		g_prim->m_roll = Angle;
		g_prim->m_rollSpeed = -18.0f;
		g_prim->m_rollAccel = -(g_prim->m_rollSpeed / g_prim->m_duration) / 2.5f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 2);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\FireRing.tga", false);
		g_prim->m_GI[0].life=0;
	}
}

void CRagEffect::ColdHit()
{
	if (m_stateCnt == 0) {
		float length;

		int	tmpCnt = 0;
		for (float i = 0; i < 360; i += 40, tmpCnt++) {
			g_prim = LaunchEffectPrim(PP_2DQUAD);
			g_prim->m_duration = 15;

			g_prim->m_longitude = g_prim->m_roll = (i - 15) + random(60);

			if (tmpCnt % 3 == 0) {
				g_prim->m_heightSize = (float)(random(10) + 100);
				g_prim->m_widthSize = 10.0f;
				g_prim->m_speed = (float)(random(45) / 10) + 12.8;
			} else {
				g_prim->m_heightSize = (float)(random(5) + 45);
				g_prim->m_widthSize = 4.0f;
				g_prim->m_speed = (float)(random(45) / 10) + 4.8;
			}

			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 1.5;

			length = random(5);
			g_prim->m_deltaPos2.x = length * GetSin(i);
			g_prim->m_deltaPos2.y = -length * GetCos(i);
			g_prim->m_deltaPos2.y -= 20;

			g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
			g_prim->m_matrix.AppendYRotation(GetRadian(g_prim->m_longitude));
			g_prim->m_rotMat = g_prim->m_matrix;
			g_prim->m_count = 20;
			g_prim->m_alpha = 200;
			g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ice.tga", false);
		}
	}

	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt == 0 || m_stateCnt == 7) {
		g_prim = LaunchEffectPrim(PP_2DTEXTURE);
		g_prim->m_duration = 25;
		g_prim->m_speed = 0.85f;
		g_prim->m_longitude = -20 + random(40);
		g_prim->m_widthSize = 1;
		g_prim->m_heightSize = 1;
		g_prim->m_ChangeWidthSpeed = 0.5f;
		g_prim->m_ChangeHeightSpeed = 0.5f;
		g_prim->m_heightSpeed = 6.0f;
		g_prim->m_widthSpeed = 6.0f;
		g_prim->m_ChangePoint = 10;

		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\smoke.tga", false);
		g_prim->m_GI[0].life=0;
	}

}

void CRagEffect::PoisonHit()
{
	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = m_duration;
		g_prim->m_size = 1.5f;
		g_prim->m_animSpeed = 2;
		g_prim->m_pattern |= PT_USEORGARGB;
		g_prim->m_repeatAnim = false;
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\PoisonHit.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\PoisonHit.act");
		g_prim->InitSegment();
	}
}

void CRagEffect::LockOn()
{
	m_pos = m_master->m_pos;
	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = m_duration;
		g_prim->m_pattern |= PT3DTEX_RGBCYCLE;
		g_prim->m_longitude = 0;
		g_prim->m_longSpeed = 4.5f;
		g_prim->m_roll = 90;

		g_prim->m_ChangeWidthAccel = 0.0f;
		g_prim->m_ChangeHeightAccel = 0.0f;
		g_prim->m_widthSize = 30.0f;
		g_prim->m_heightSize = 30.0f;
		g_prim->m_widthSpeed = -1.0f;
		g_prim->m_heightSpeed = -1.0f;
		g_prim->m_ChangePoint = 20;
		g_prim->m_ChangeWidthSpeed = 0;
		g_prim->m_ChangeHeightSpeed = 0;
		g_prim->m_RGBCycleDelay = 20;
		g_prim->m_red = 250;
		g_prim->m_blue = 150;
		g_prim->m_blueSpeed = -15;
		g_prim->m_green = 150;
		g_prim->m_greenSpeed = -15;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\LockOn128.tga", false);

		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = m_duration;
		g_prim->m_pattern |= PT3DTEX_RGBCYCLE;
		g_prim->m_longitude = 45;
		g_prim->m_longSpeed = 4.5f;
		g_prim->m_roll = 90;
		g_prim->m_widthSize = 30.0f;
		g_prim->m_heightSize = 30.0f;
		g_prim->m_widthSpeed = -1.0f;
		g_prim->m_heightSpeed = -1.0f;
		g_prim->m_ChangePoint = 20;
		g_prim->m_ChangeWidthSpeed = 0;
		g_prim->m_ChangeHeightSpeed = 0;
		g_prim->m_ChangeWidthAccel = 0.0f;
		g_prim->m_ChangeHeightAccel = 0.0f;
		g_prim->m_RGBCycleDelay = 20;
		g_prim->m_red = 250;
		g_prim->m_blue = 150;
		g_prim->m_blueSpeed = -15;
		g_prim->m_green = 150;
		g_prim->m_greenSpeed = -15;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\LockOn128.tga", false);
	}
	m_pos.y = g_modeMgr.GetGameMode()->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z) - 2.0f;
}

void CRagEffect::WarpZone()
{
	if (m_stateCnt == 145)
		m_stateCnt = 0;

	if (m_stateCnt == 0)
	{

		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = 158;
		g_prim->m_pattern |= PT_FILLCIRCLE;

		g_prim->m_radius = 15 ;
		g_prim->m_deltaPos2.y -= 1.0f;
		g_prim->m_latitude = 90;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 128;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);

	}

	if (m_stateCnt % 28 == 0 && m_stateCnt < 117)
	{

		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = 80;
		g_prim->m_innerSize = 4.0f;
		g_prim->m_latitude = 90;
		g_prim->m_deltaPos2.y -= 1.0f;

		g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
		g_prim->m_radius = 14.0f;
		g_prim->m_radiusSpeed = -0.15f;

		g_prim->m_alpha = 5;
		g_prim->m_maxAlpha = 200;
		g_prim->m_alphaSpeed = 10 ;

		g_prim->m_chPoint.push_back(20);
		g_prim->m_chVal1.push_back(0);

		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 5);
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);
	}

	if (m_stateCnt % 10 == 0)
	{
		float	rndNum;
		g_prim = LaunchEffectPrim(PP_3DPARTICLEORBIT);
		rndNum = random(200) / 100;
		g_prim->m_radius = 5.0f + (rndNum * 0.75f);
		g_prim->m_radiusSpeed = 0.0005f;
		g_prim->m_duration = 130;
		g_prim->m_longitude = random(360);
		if (random(1) == 0)
			g_prim->m_longSpeed = 2.0f;
		else g_prim->m_longSpeed = -2.0f;
		g_prim->m_gravSpeed = -0.2f;
		g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration) / 1.5f;
		g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, g_prim->m_radius), g_prim->m_matrix);
		g_prim->m_size = 0.7f + (random(10) / 10);
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
		g_prim->InitSegment();
	}
}

void CRagEffect::Sightrasher()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt == 0) {
		m_master->SendMsg(NULL, AM_DELETE_EFFECT, EF_SIGHT, 0, 0);
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\wizard_sightrasher.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (m_stateCnt % 5 == 0 && m_stateCnt < 20) {
		int a ;
		for (a = 0; a < 8; a++) {
			float	degree = (a) * -45.0f;
			float	sn = GetSin(degree);
			float	cs = GetCos(degree);
			float ts = GetRadian(degree);
			float		radius = 5;

			g_prim = LaunchEffectPrim(PP_3DPARTICLE);
			g_prim->m_duration = 85 - m_stateCnt;
			g_prim->m_pattern |= PT_DELTASPEED;
			radius += m_stateCnt / 5;
			g_prim->m_deltaPos2.Set(radius * sn, -20 + (m_stateCnt / 3), -radius * cs);
			g_prim->m_speed3d.Set(1.0 * sn, 0, -1.0 * cs);
			g_prim->m_animSpeed = 3 + random(2);
			g_prim->m_size = 2.5f - float(m_stateCnt / 10.0f);
			g_prim->m_alpha = 150 - (m_stateCnt * 2);


			g_prim->m_deltaPosAccel.x = 0;
			g_prim->m_deltaPosAccel.y = 0;
			g_prim->m_deltaPosAccel.z = 0;

			g_prim->m_ChangeSpeedx = 0.5 * sn;
			g_prim->m_ChangeSpeedy = 0;
			g_prim->m_ChangeSpeedz = -0.5 * cs;
			g_prim->m_ChangePoint = 35 - m_stateCnt;
			g_prim->m_ChangeAccelx = -0.004f;
			g_prim->m_ChangeAccely = 0;
			g_prim->m_ChangeAccelz = 0.004f;
			g_prim->m_fadeOutCnt = 65 - m_stateCnt;
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\Sight.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\Sight.act");
			g_prim->InitSegment();
		}

		for (a = 0; a < 8; a++) {
			float	degree = (a) * -45.0f;
			float	sn = GetSin(degree);
			float	cs = GetCos(degree);
			float ts = GetRadian(degree);
			float		radius = 5;

			g_prim = LaunchEffectPrim(PP_3DPARTICLE);
			g_prim->m_duration = 85 - m_stateCnt;
			g_prim->m_pattern |= PT_DELTASPEED;
			radius += m_stateCnt / 5;
			g_prim->m_deltaPos2.Set(radius * sn, 0, -radius * cs);
			g_prim->m_speed3d.Set(1.0 * sn, 0, -1.0 * cs);
			g_prim->m_animSpeed = 3;
			g_prim->m_size = 1.0f - float(m_stateCnt / 30.0f);
			g_prim->m_alpha = 150 - (m_stateCnt * 2);



			g_prim->m_ChangeSpeedx = 0.5 * sn;
			g_prim->m_ChangeSpeedz = -0.5 * cs;
			g_prim->m_ChangePoint = 35 - m_stateCnt;
			g_prim->m_ChangeAccelx = -0.004f;
			g_prim->m_ChangeAccelz = 0.004f;
			g_prim->m_fadeOutCnt = 65 - m_stateCnt;
			g_prim->SetSprName("Shadow.spr");
			g_prim->SetActName("Shadow.act");
			g_prim->InitSegment();
		}
	}

	if (m_stateCnt == 10) {
		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = 30;
		g_prim->m_latitude = 90;
		g_prim->m_radiusSpeed = 2.5f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / g_prim->m_duration) / 1.5f;
		g_prim->m_innerSize = -10.0f;
		g_prim->m_alpha = 150;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_yellow.tga", false);
	}

}

void CRagEffect :: Barrier()
{
	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DSPHERE);
		g_prim->m_duration = 250;
		g_prim->m_radius = 13.5f;
		g_prim->m_deltaPos2.y = -10.0f;
		g_prim->m_longitude = 2.0f;
		g_prim->m_alpha = 100;
		g_prim->m_texture = new CTexture*[g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\\xb4\xeb\xc6\xf8\xb9\xdf.tga", false);
	}
}

void CRagEffect :: Cone()
{
	if (m_stateCnt % 130 == 0) {
		g_prim = LaunchEffectPrim(PP_3DPARTICLEORBIT);
		m_duration = 200;
		g_prim->m_duration = 300;
		g_prim->m_radius = 0.1f;
		g_prim->m_radiusSpeed = 0.2f;
		g_prim->m_gravSpeed = 0.1f;
		g_prim->m_gravAccel = 0.005f;
		g_prim->m_longitude = 2;
		g_prim->m_longSpeed = 15.0f;
		g_prim->m_longAccel = -0.01f;
		g_prim->m_size = 1.2f;
		g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, -30, g_prim->m_radius), g_prim->m_matrix);

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
		g_prim->InitSegment();
	}
}

void CRagEffect :: Sphere()
{
	m_pos = m_master->m_pos;
	if (m_stateCnt % 10 == 0) {
		g_prim = LaunchEffectPrim(PP_3DPARTICLEORBIT);
		m_duration = 500 - m_stateCnt;
		g_prim->m_duration = 300;
		g_prim->m_radius = 0.1f;
		g_prim->m_radiusSpeed = 0.15f;
		g_prim->m_gravSpeed = 0.15f;

		g_prim->m_longitude = 2;
		g_prim->m_longSpeed = 8.0f;
		g_prim->m_longAccel = -0.01f;
		g_prim->m_size = 1.2f;
		g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, -30, g_prim->m_radius), g_prim->m_matrix);

		g_prim->m_chRadiSpeed = TRUE;
		g_prim->m_chCnt = 0;
		g_prim->m_chPoint.push_back(95);
		g_prim->m_chVal1.push_back((float)-0.15);
		g_prim->m_chVal2.push_back(g_prim->m_radiusAccel);

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
		g_prim->InitSegment();
	}
}

void CRagEffect :: BowlingBash()
{
	if (m_stateCnt == 0 || m_stateCnt == 5) {
		int angle = 180 - m_master->m_roty;
		if (angle < 0)
			angle += 360;

		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = 20 - m_stateCnt;
		g_prim->m_longitude = angle + m_stateCnt * 20;
		g_prim->m_matrix.MakeXRotation(GetRadian((int)g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_outerSize = 8;
		g_prim->m_outerSpeed = 0.5f;
		g_prim->m_outerAccel = -0.03f;
		g_prim->m_innerSize = 3;
		g_prim->m_innerSpeed = 0.5f;
		g_prim->m_innerAccel = -0.03f;
		g_prim->m_heightSize = 3.5f;


		g_prim->m_alpha = 240 - m_stateCnt * 7;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 2);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);
	}

	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = 50;
		g_prim->m_renderFlag |= RF_NODEPTHCHECK;
		g_prim->m_latitude = 90;
		g_prim->m_radius = 8.0f;
		g_prim->m_radiusSpeed = 0.7f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / 30);
		g_prim->m_innerSize = 5.0f;
		g_prim->m_ChangePoint = 20;
		g_prim->m_ChangeAccel = 0;
		g_prim->m_ChangeSpeed = 0.08f;


		g_prim->m_alpha = 45;
		g_prim->m_fadeOutCnt = 35;
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_yellow.tga", false);
	}
}

void CRagEffect :: IceWall()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\wizard_icewall.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		int		angle;
		float	sn, cs, length;

		int duration = 9999;
		m_duration = duration;

		for (int i = 0; i < 3; i++)
		{
			g_prim = LaunchEffectPrim(PP_3DQUADHORN);
			g_prim->m_duration = m_duration;
			g_prim->m_pattern |= PTQH_SPEEDLIMIT;

			angle = random(360);

			length = -2 + (i * 2);
			sn = GetSin(angle);
			cs = GetCos(angle);

			g_prim->m_deltaPos2.Set(0 * sn, 30, -length * cs);
			g_prim->m_size = 2.5f + (float)(random(40) / 100);
			g_prim->m_heightSize = (float)(random(100) + 300) / 10.0f;
			g_prim->m_longitude = random(360);
			g_prim->m_latitude = 87 + random(6);

			g_prim->m_count = 20;
			g_prim->m_alpha = 200;

			g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
			g_prim->m_matrix.AppendYRotation(GetRadian(g_prim->m_longitude));
			g_prim->m_rotMat = g_prim->m_matrix;
			g_prim->m_speed = 3.0f;
			g_prim->m_accel = -(g_prim->m_speed / 40) * 2.0f;
			g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ice.tga", false);
		}
	}
}

void CRagEffect :: EarthSpike(char F1)
{
#if 1
	if (m_stateCnt == 0)
	{
		char **tName = (char**)&F1;

		if (F1 == 0)
		{
			vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
			PlayWave("effect\\wizard_earthspike.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);

			*tName = "effect\\stone.bmp";
		}
		else if (F1 == 2)
		{
			char buf[50];
			sprintf(buf, "effect\\EF_IceArrow%d.wav", random(3) + 1);
			vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
			PlayWave(buf, m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

			*tName = "effect\\ice.tga";
		}

		CGameMode * gameMode = g_modeMgr.GetGameMode();
		gameMode->SendMsg(MM_QUAKE, 1);
		float side = 1.0f, front = 0.24f, latitude = 0.2f;
		gameMode->SendMsg(MM_SETQUAKEINFO, (INT_PTR)&side, (INT_PTR)&front, (INT_PTR)&latitude);

		int		angle;
		float	sn, cs, length;

		g_prim = LaunchEffectPrim(PP_3DQUADHORN);
		g_prim->m_duration = 240;
		g_prim->m_pattern = PTQH_SPEEDLIMIT;
		g_prim->m_pattern |= PTQH_RETURNDOWN;

		angle = random(360);
		length = (float)(random(4) + 1) / 10.0f;
		sn = GetSin(angle);
		cs = GetCos(angle);

		g_prim->m_deltaPos2.Set(0, 10, 0);
		g_prim->m_size = (float)(random(50) + 300) / 100.0f;
		g_prim->m_heightSize = 18.0f;
		g_prim->m_longitude = random(360);
		g_prim->m_latitude = random(20) + 80;
		g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian(g_prim->m_longitude));
		g_prim->m_rotMat = g_prim->m_matrix;
		g_prim->m_speed = 1.0f;
		g_prim->m_accel = 0.01f;
		g_prim->m_alpha = 250;

		g_prim->m_count = 15;
		g_prim->m_ChangePoint = 12;
		g_prim->m_ChangeSpeed = -1.2f;
		g_prim->m_ChangeAccel = 0.0f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(*tName, false);

		for (int a = 0; a < 6; a++) {
			float degree = random((a + 1) * 60) + (a * 60);
			if (degree > 360) degree = 360;
			float		radius = 3.5f;

			g_prim = LaunchEffectPrim(PP_3DQUADHORN);
			g_prim->m_duration = 240;
			g_prim->m_pattern |= PTQH_SPEEDLIMIT;

			angle = random(360);
			length = (float)(random(4) + 1) / 10.0f;
			sn = GetSin(angle);
			cs = GetCos(angle);

			g_prim->m_deltaPos2.Set(radius * sn, 20, -radius * cs);
			g_prim->m_size = (float)(random(100) + 400) / 100.0f;
			g_prim->m_heightSize = 20.0f;
			g_prim->m_longitude = degree;
 			g_prim->m_latitude = 100;
			g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
			g_prim->m_matrix.AppendYRotation(GetRadian(g_prim->m_longitude));
			g_prim->m_rotMat = g_prim->m_matrix;
			g_prim->m_speed = 1.5f;
			g_prim->m_accel = 0.0f;
			g_prim->m_alpha = 250;

			g_prim->m_count = 2;
			g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture(*tName, false);
		}
	}

#endif
}

void CRagEffect :: TurnUndead()
{
#if 1
	if (m_stateCnt == 37) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_Bash.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;
	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_2DCIRCLE);
		g_prim->m_radius = 370;
		g_prim->m_radiusSpeed = -(g_prim->m_radius / 20);
		g_prim->m_duration = 20;
		g_prim->m_pattern |= PT_FILLCIRCLE;
		g_prim->m_deltaPos2.y = -40;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 130;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 6;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);

		int Loop = 20;
		if(m_stateCnt == 0)
			Loop = 5;
		for (int i = 0; i < Loop; i++) {
			g_prim = LaunchEffectPrim(PP_2DFLASH);
			g_prim->m_duration = 20;
			g_prim->m_count = -1;
			g_prim->m_speed3d.x = m_master->m_pos.x;
			g_prim->m_speed3d.y = m_master->m_pos.z;



			g_prim->m_deltaPos2.y = -40;
			g_prim->m_longitude = random(360);
			g_prim->m_longSpeed = (float)(random(60) + 10) / 10.0f;
			g_prim->m_longAccel = -(g_prim->m_longSpeed / g_prim->m_duration) / 1.5f;
			g_prim->m_arcAngle = (float)(random(25) + 5) / 10.0f;
			if(m_stateCnt == 0) {
				g_prim->m_heightSize = random(30) + 350;
				g_prim->m_heightSpeed = -(float)(random(30) + 150) / 10.0f;
			} else {
				g_prim->m_heightSize = 0;
				g_prim->m_heightSpeed = (float)(random(20) + 50) / 10.0f;

				g_prim->m_ChangePoint = 40;
				g_prim->m_ChangeSpeed = (float)(random(10) + 15) / 10.0f;
				g_prim->m_ChangeAccel = 0;
				g_prim->m_ChangeHeightSpeed = 0;
			}
			g_prim->m_alpha = 0;
			g_prim->m_maxAlpha = 150;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;

			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("EFFECT\\alpha_center.tga", false);
		}
	}

	m_pos = m_master->m_pos;
	if (m_stateCnt == 50) {
		g_prim = LaunchEffectPrim(PP_3DTEXTURE);

		g_prim->m_duration = 30;
		g_prim->m_roll = 90;
		g_prim->m_widthSpeed = 1.5f;
		g_prim->m_heightSpeed = 1.5f;
		g_prim->m_deltaPos2.y = -7.0f;
		g_prim->m_widthAccel = (g_prim->m_widthSpeed / 10);
		g_prim->m_heightAccel = (g_prim->m_heightSpeed / 10);
		g_prim->m_alpha = 150;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 10;

		g_prim->m_ChangePoint = 15;
		g_prim->m_ChangeWidthSpeed = 0.5f;
		g_prim->m_ChangeWidthAccel = -0.5f / (float)15;
		g_prim->m_ChangeHeightSpeed = 0.5f;
		g_prim->m_ChangeHeightAccel = -0.5f / (float)15;
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_liner.tga", false);
	}
#endif
}

void CRagEffect :: SpearBmr()
{
	if (m_stateCnt == 0 || m_stateCnt == 4 || m_stateCnt == 8 || m_stateCnt == 12) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_FireBall.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = 15;
		g_prim->m_pattern |= PT_CHKTARGETPOS | PT_ROTATESPR;
		g_prim->m_deltaPos2.Set(0, -15, 0);
		g_prim->CalcDir(m_deltaPos.x, m_deltaPos.z);
		g_prim->m_longitude = -g_prim->m_roty;
		g_prim->m_roty = 180 - g_prim->m_roty;
		if (g_prim->m_roty < 0)
			g_prim->m_roty += 360;

		g_prim->m_speed = -(g_prim->CalcDist(m_deltaPos.x, m_deltaPos.z) / g_prim->m_duration);
		g_prim->m_matrix.MakeYRotation(GetRadian(g_prim->m_longitude));
		g_prim->m_size = 1.0f;
		g_prim->m_animSpeed = 1;
		if (m_stateCnt != 0) {
			if (m_stateCnt == 4)
				g_prim->m_alpha = 180;
			else if (m_stateCnt == 8)
				g_prim->m_alpha = 130;
			else
				g_prim->m_alpha = 80;
		}

		int dir = Get8Dir2(m_master->m_roty, TRUE);
		SetAction(dir, 1, MT_ONETIME);

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc3\xa2.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc3\xa2.act");


		g_prim->InitSegment();
	}
}

void CRagEffect :: Pierce()
{
	if (m_stateCnt % 20 == 0 && m_stateCnt < m_level * 20) {
		int		angle = 180 - m_master->m_roty;

		if (angle < 0)
			angle += 360;

		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = 15;
		g_prim->m_longitude = angle;
		g_prim->m_latitude = -90;
		g_prim->m_matrix.MakeXRotation(GetRadian((int)g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos2.Set(0, -10, 0);
		g_prim->m_speed = 0.7f;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
		g_prim->m_outerSize = 11 + (m_stateCnt * 0.1);
		g_prim->m_innerSize = 6 + (m_stateCnt * 0.1);
		g_prim->m_heightSize = 2.5f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 2);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_yellow.tga", false);

		if (m_stateCnt > 0) {
			int i ;
			for (i = 0; i < 4; i++) {
				g_prim = LaunchEffectPrim(PP_3DPARTICLE);
				g_prim->m_longitude = random(80) + angle - 40;
				g_prim->m_latitude = -50 + random(100);
				g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
				g_prim->m_matrix.AppendXRotation(GetRadian((int)g_prim->m_latitude));
				g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, random(4) + 4), g_prim->m_matrix);
				g_prim->m_deltaPos2 = vector3d(0, -10, 0) + g_prim->m_deltaPos3;
				g_prim->m_speed = (float)(random(90) + 60) / 100;
				g_prim->m_duration = random(24) + 6;
				g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
				g_prim->m_size = (float)(random(100) + 60) / 100;
				g_prim->m_sizeSpeed = -(g_prim->m_size / g_prim->m_duration);
				g_prim->m_fadeOutCnt = 0;
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
				g_prim->m_numSegment = 3;
				g_prim->InitSegment();
			}

			for (i = 0; i < 4; i++) {
				g_prim = LaunchEffectPrim(PP_3DPARTICLEGRAVITY);
				g_prim->m_longitude = random(80) + angle + 140;
				g_prim->m_latitude = -50 + random(100);
				g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
				g_prim->m_matrix.AppendXRotation(GetRadian((int)g_prim->m_latitude));
				g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, random(4) + 4), g_prim->m_matrix);
				g_prim->m_deltaPos2 = vector3d(0, -10, 0) + g_prim->m_deltaPos3;
				g_prim->m_matrix.MakeYRotation(GetRadian(g_prim->m_longitude));
				g_prim->m_duration = random(24) + 6;
				g_prim->m_speed = (float)(random(90) + 60) / 100;
				g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
				g_prim->m_gravSpeed = -(float)(random(90) + 30) / 100;
				g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration * 2.0f);
				g_prim->m_size = (float)(random(100) + 60) / 100;
				g_prim->m_sizeSpeed = -(g_prim->m_size / g_prim->m_duration);
				g_prim->m_fadeOutCnt = 0;
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
				g_prim->m_numSegment = 3;
				g_prim->InitSegment();
			}
		}
	}
}

void CRagEffect :: RemoveTrap()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\hunter_removetrap.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (m_stateCnt == 0 || m_stateCnt == 7) {
		float rand;
		for(int a = 0; a < 12; a ++) {
			g_prim = LaunchEffectPrim(PP_3DPARTICLEORBIT);
			g_prim->m_duration = 50;
			g_prim->m_radiusSpeed = 0.2f;
			rand = random(10);
			g_prim->m_gravSpeed = -2.0f;
			rand = random(10);
			g_prim->m_gravAccel = 0.08f + (rand / 1000);
			g_prim->m_longitude = random(360);
			g_prim->m_size = 0.8f;

			g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
			g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, g_prim->m_radius), g_prim->m_matrix);

			g_prim->m_fadeOutCnt = 35;
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
			g_prim->InitSegment();
		}
	}
}

void CRagEffect :: Yufitel(char F1)
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\hunter_shockwavetrap.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if(m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = 20;
		g_prim->m_pattern |= PT3DTEX_SHOWFRONT;
		g_prim->m_pattern |= PT3DTEX_CHKTARGETPOS;
		g_prim->m_renderFlag |= RF_EMISSIVE;
		g_prim->m_widthSize = 3.5f;
		g_prim->m_heightSize = 3.5f;
		g_prim->CalcDir(m_deltaPos.x, m_deltaPos.z);
		g_prim->m_longitude = -g_prim->m_roty;
		g_prim->m_roty = 180 - g_prim->m_roty;
		if (g_prim->m_roty < 0)
			g_prim->m_roty += 360;
		g_prim->m_speed = -(g_prim->CalcDist(m_deltaPos.x, m_deltaPos.z) / g_prim->m_duration);
		g_prim->m_matrix.MakeYRotation(GetRadian(g_prim->m_longitude));
		g_prim->m_deltaPos2.y = -5.0f;
		g_prim->m_alpha = 170;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		if (F1 == 0)
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\thunder_center.bmp", false);
		else
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\twirl_soft.bmp", false);

		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = 20;
		g_prim->m_pattern = PT3DTEX_SHOWFRONT;
		g_prim->m_pattern |= PT3DTEX_CHKTARGETPOS;
		g_prim->m_renderFlag |= RF_EMISSIVE;
		g_prim->m_repeatAnim = true;
		g_prim->m_widthSize = 4.5f;
		g_prim->m_heightSize = 4.5f;
		g_prim->CalcDir(m_deltaPos.x, m_deltaPos.z);
		g_prim->m_longitude = -g_prim->m_roty;
		g_prim->m_roty = 180 - g_prim->m_roty;
		if (g_prim->m_roty < 0)
			g_prim->m_roty += 360;
		g_prim->m_speed = -(g_prim->CalcDist(m_deltaPos.x, m_deltaPos.z) / g_prim->m_duration);
		g_prim->m_matrix.MakeYRotation(GetRadian(g_prim->m_longitude));

		g_prim->m_deltaPos2.y = -5.0f;
		g_prim->m_animSpeed = 1;
		g_prim->m_totalTexture = 6;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		if (F1 == 0)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\thunder_ball_a.bmp", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\thunder_ball_b.bmp", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\thunder_ball_c.bmp", false);
			g_prim->m_texture[3] = g_texMgr.GetTexture("effect\\thunder_ball_d.bmp", false);
			g_prim->m_texture[4] = g_texMgr.GetTexture("effect\\thunder_ball_e.bmp", false);
			g_prim->m_texture[5] = g_texMgr.GetTexture("effect\\thunder_ball_f.bmp", false);
		}
		else
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\twirl_soft.bmp", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\thunder_ball_b.bmp", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\twirl_soft.bmp", false);
			g_prim->m_texture[3] = g_texMgr.GetTexture("effect\\thunder_ball_d.bmp", false);
			g_prim->m_texture[4] = g_texMgr.GetTexture("effect\\twirl_soft.bmp", false);
			g_prim->m_texture[5] = g_texMgr.GetTexture("effect\\thunder_ball_f.bmp", false);

		}
	}
}

void CRagEffect :: YufitelHit(char F1)
{
	m_pos = m_master->m_pos;
	if (m_stateCnt % 20 == 0)
	{
		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = 10;
		g_prim->m_pattern |= PT3DTEX_SHOWFRONT;
		g_prim->m_renderFlag |= RF_EMISSIVE;
		g_prim->m_renderFlag |= RF_NODEPTHCHECK;

		g_prim->m_deltaPos2.y = -5.0f;
		g_prim->m_widthSpeed = 2.5f;
		g_prim->m_heightSpeed = 2.5f;

		g_prim->m_ChangePoint = 10;
		g_prim->m_ChangeWidthSpeed = 0.0f;
		g_prim->m_ChangeHeightSpeed = 0.0f;

		g_prim->m_ChangeWidthAccel = -g_prim->m_ChangeWidthSpeed / 50;

		g_prim->m_ChangeHeightAccel = -g_prim->m_ChangeHeightSpeed / 50;




		g_prim->m_fadeOutCnt = 1;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		if (F1 == 0)
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\thunder_pang.bmp", false);
		else
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\pokjuk_d.bmp", false);
	}

	if (m_stateCnt == 10) {
		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = 300;
		g_prim->m_pattern |= PT3DTEX_SHOWFRONT;
		g_prim->m_renderFlag |= RF_NODEPTHCHECK;
		g_prim->m_renderFlag |= RF_EMISSIVE;

		g_prim->m_deltaPos2.y = -5.0f;
		g_prim->m_repeatAnim = true;
		g_prim->m_widthSize = 7.5f;
		g_prim->m_heightSize = 7.5f;
		g_prim->m_animSpeed = 1;
		g_prim->m_totalTexture = 5;
		g_prim->m_texture = new CTexture *[g_prim->m_totalTexture];

		if (F1 == 0)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\thunder_plazma_blast_a.bmp", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\thunder_plazma_blast_b.bmp", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\thunder_ball_d.bmp", false);
			g_prim->m_texture[3] = g_texMgr.GetTexture("effect\\thunder_ball_e.bmp", false);
			g_prim->m_texture[4] = g_texMgr.GetTexture("effect\\thunder_ball_f.bmp", false);
		}
		else
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\twirl_soft.bmp", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\thunder_ball_b.bmp", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\twirl_soft.bmp", false);
			g_prim->m_texture[3] = g_texMgr.GetTexture("effect\\thunder_ball_c.bmp", false);
			g_prim->m_texture[4] = g_texMgr.GetTexture("effect\\twirl_soft.bmp", false);
		}
	}
}

void CRagEffect :: HasteUp()
{
	if (m_stateCnt == 0) {
		m_duration = 300;
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\black_adrenalinerush_a.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (m_stateCnt == 30) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\black_adrenalinerush_b.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt == 0) {
		for (int i = 0; i < 20; i++) {
			g_prim = LaunchEffectPrim(PP_2DFLASH);
			g_prim->m_duration = 80;




			g_prim->m_deltaPos2.y = -40;
			g_prim->m_longitude = random(360);
			g_prim->m_longSpeed = (float)(random(60) + 10) / 10.0f;
			g_prim->m_longAccel = -(g_prim->m_longSpeed / g_prim->m_duration) / 1.5f;
			g_prim->m_arcAngle = (float)(random(25) + 5) / 10.0f;
			g_prim->m_heightSize = 0;
			g_prim->m_heightSpeed = (float)(random(20) + 50) / 13.0f;

			g_prim->m_ChangePoint = 40;
			g_prim->m_ChangeSpeed = (float)(random(10) + 15) / 10.0f;
			g_prim->m_ChangeAccel = 0;
			g_prim->m_ChangeHeightSpeed = 0;




			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("EFFECT\\alpha_center.tga", false);
		}
	}

	if (m_stateCnt == 0)
	{
		for (int i = 0; i < 4; i++)
		{
			g_prim = LaunchEffectPrim(PP_3DPARTICLEORBIT);
			g_prim->m_duration = 100;
			g_prim->m_radius = 7.0f;
			g_prim->m_gravSpeed = -0.1f;
			g_prim->m_gravAccel = -0.002f;
			g_prim->m_longitude = i * 90;
			g_prim->m_longAccel = 0.2f;
			g_prim->m_longSpeed = 0.3f + m_stateCnt * (g_prim->m_longAccel);

			g_prim->m_size = 1.2f;
			g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));


			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 10);
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
			g_prim->InitSegment();
		}
	}
}

void CRagEffect :: Flasher()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\hunter_flasher.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = 70;
		g_prim->m_pattern = PT_FILLCIRCLE | PT3DTEX_SHOWFRONT;
		g_prim->m_renderFlag |= RF_EMISSIVE;
		g_prim->m_deltaPos2.y = -10;
		g_prim->m_widthSize = 20.0f;

		g_prim->m_heightSize = 20.0f;

		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 200;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 6;

		g_prim->m_AlphaShow = TRUE;
		g_prim->m_chPoint.push_back(40);
		g_prim->m_chVal1.push_back(120);
		g_prim->m_chVal2.push_back(200);
		g_prim->m_chPoint.push_back(100);
		g_prim->m_chVal1.push_back(0);
		g_prim->m_chVal2.push_back(80);

		g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\thunder_center.bmp", false);

		for (int i = 0; i < 20; i++) {
			g_prim = LaunchEffectPrim(PP_2DFLASH);
			g_prim->m_duration = 70;
			g_prim->m_deltaPos2.y = -40;
			g_prim->m_longitude = random(360);
			g_prim->m_longSpeed = (float)(random(60) + 10) / 10.0f;
			g_prim->m_longAccel = -(g_prim->m_longSpeed / g_prim->m_duration) / 1.5f;
			g_prim->m_arcAngle = (float)(random(25) + 5) / 10.0f;
			g_prim->m_heightSize = random(40) + 20;
			g_prim->m_heightSpeed = (float)(random(30) + 50) / 10.0f;
			g_prim->m_alpha = 0;
			g_prim->m_maxAlpha = 200;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;
			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("EFFECT\\alpha_center.tga", false);
		}
	}
}

void CRagEffect :: BlitzBeat()
{
	if (m_stateCnt == 5) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\hunter_blitzbeat_1st.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (m_stateCnt == 30) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\hunter_blitzbeat.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	static int hitCnt = 0;
	m_pos = m_master->m_pos;
	if (m_stateCnt == 0) {
		hitCnt = 0;
		float angle, sn, cs;
		float		radius = 3.5f;
		for (int a = 0; a < 10; a++) {
			g_prim = LaunchEffectPrim(PP_3DCROSSTEXTURE);
			g_prim->m_duration = 20;
			g_prim->m_renderFlag |= RF_NODEPTHCHECK;

			angle = m_master->m_roty;


			g_prim->m_longitude = angle;
			g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));

			angle = random(360);
			sn = GetSin(angle);
			cs = GetCos(angle);

			g_prim->m_speed3d.MatrixMult(vector3d(0, 0, 15.0f), g_prim->m_matrix);
			g_prim->m_deltaPos += g_prim->m_speed3d;

			g_prim->m_deltaPos2.Set(radius * sn, -7.0f, -radius * cs);
			g_prim->m_speed = -1.2f;
			g_prim->m_accel = -0.1f;
			g_prim->m_widthSize = (float)(random(40) + 20) / 10;
			g_prim->m_heightSize = 0.2f;
			g_prim->m_widthSpeed = 0.1f;
			g_prim->m_widthAccel = -0.01f;
			g_prim->m_alpha = 0;
			g_prim->m_maxAlpha = 250;

			g_prim->m_ChangePoint = 10;
			g_prim->m_ChangeSpeed = -0.3f;
			g_prim->m_ChangeAccel = 0.001f;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 2;
			g_prim->m_fadeOutCnt = g_prim->m_duration - 2;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ac_center2.tga", false);
			g_prim->m_GI[0].life=0;
		}
	}

	if (m_stateCnt > 35 && m_stateCnt % 3 == 0 && hitCnt < m_count) {
		int		angle = m_master->m_roty;
		int		randPosx = random(10);
		int		randPosy = random(20);
		int		randPosz = random(10);




		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = 12;
		g_prim->m_longitude = angle;
		g_prim->m_latitude = -90;
		g_prim->m_matrix.MakeXRotation(GetRadian((int)g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian((int)g_prim->m_longitude));

		g_prim->m_speed3d.MatrixMult(vector3d(0, -2.0f, 1.0f), g_prim->m_matrix);
		g_prim->m_deltaPos += g_prim->m_speed3d;

		g_prim->m_deltaPos2.Set((randPosx + 1) / 5, -12 + (randPosy + 1) / 2, 0);
		g_prim->m_speed = 0.4f;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
		g_prim->m_outerSize = 1.5f;
		g_prim->m_innerSize = 0.5f;
		g_prim->m_heightSize = 3.5f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 2);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\magic_green.tga", false);
		hitCnt++;
	}
}

void CRagEffect :: Bottom()
{
	if (m_stateCnt == 0) {
		int		angle = 180 - m_master->m_roty;
		matrix	tmpMat;

		if (angle < 0)
			angle += 360;

		tmpMat.MakeYRotation(GetRadian(angle));

		m_duration = 9999;
		float WidthSize = 5.0f;
		for (int i = 1; i >= -1; i -= 2) {
			g_prim = LaunchEffectPrim(PP_3DTEXTURE);
 			g_prim->m_duration = 9999;
			g_prim->m_commonPattern = PT_UPDATEFADEOUT;
			g_prim->m_pattern = PT3DTEX_SIZEUP | PT3DTEX_WAVERINGLY;
			g_prim->m_any = 50;
			g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, WidthSize * i), tmpMat);
			g_prim->m_longitude = angle + 90;

			g_prim->m_widthSize = WidthSize;

			g_prim->m_heightSize = 15.0f;
			g_prim->m_heightSpeed = 0.25f;

			g_prim->m_alpha = 0;
			g_prim->m_maxAlpha = 180;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 6;
			g_prim->m_fadeOutCnt = g_prim->m_duration - 35;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\magic_violet.tga", false);

			g_prim = LaunchEffectPrim(PP_3DTEXTURE);
			g_prim->m_duration = 9999;
			g_prim->m_commonPattern = PT_UPDATEFADEOUT;
			g_prim->m_pattern = PT3DTEX_SIZEUP | PT3DTEX_WAVERINGLY;
			g_prim->m_any = 50;
			g_prim->m_deltaPos2.MatrixMult(vector3d(WidthSize * i, 0, 0), tmpMat);
			g_prim->m_longitude = angle;
			g_prim->m_widthSize = WidthSize;
			g_prim->m_heightSize = 15.0f;
			g_prim->m_heightSpeed = 0.25f;
			g_prim->m_alpha = 0;
			g_prim->m_maxAlpha = 180;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 6;
			g_prim->m_fadeOutCnt = g_prim->m_duration - 35;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\magic_violet.tga", false);
		}
	}
}

void CRagEffect :: Bottom2()
{
	if (m_stateCnt == 0) {
		int		angle = 180 - m_master->m_roty;
		matrix	tmpMat;

		if (angle < 0)
			angle += 360;

		tmpMat.MakeYRotation(GetRadian(angle));

		m_duration = 9999;
		float WidthSize = 2.5f;

		for (int i = 1; i >= -1; i -= 2) {

			g_prim = LaunchEffectPrim(PP_3DTEXTURE);

			g_prim->m_duration = 9999;
			g_prim->m_commonPattern = PT_UPDATEFADEOUT;
			g_prim->m_pattern = PT3DTEX_SIZEUP | PT3DTEX_WAVERINGLY;
			g_prim->m_any = 50;
			g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, WidthSize * i), tmpMat);
			g_prim->m_longitude = angle + 90;
			g_prim->m_widthSize = WidthSize;

			g_prim->m_heightSize = 15.0f;
			g_prim->m_heightSpeed = 0.25f;
			g_prim->m_alpha = 180;
			g_prim->m_fadeOutCnt = 9999 - 35;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\magic_green.tga", false);

			g_prim = LaunchEffectPrim(PP_3DTEXTURE);
			g_prim->m_duration = 9999;
			g_prim->m_commonPattern = PT_UPDATEFADEOUT;
			g_prim->m_pattern = PT3DTEX_SIZEUP | PT3DTEX_WAVERINGLY;
			g_prim->m_any = 50;
			g_prim->m_deltaPos2.MatrixMult(vector3d(WidthSize * i, 0, 0), tmpMat);
			g_prim->m_longitude = angle;
			g_prim->m_widthSize = WidthSize;
			g_prim->m_heightSize = 15.0f;
			g_prim->m_heightSpeed = 0.25f;
			g_prim->m_alpha = 180;
			g_prim->m_fadeOutCnt = 9999 - 35;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\magic_green.tga", false);
		}
	}
}

void CRagEffect :: WaterBall()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\wizard_waterball_chulung.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if(m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = m_duration;
		g_prim->m_pattern = PT3DTEX_SHOWFRONT;
		g_prim->m_pattern |= PT3DTEX_ACCELARATION;
		g_prim->m_renderFlag |= RF_EMISSIVE;
		g_prim->m_repeatAnim = true;
		g_prim->m_widthSize = 3.5f;
		g_prim->m_heightSize = 3.5f;
		g_prim->m_speed3d.y = -(25.0f / g_prim->m_duration);

		g_prim->m_deltaPos2.y = -5.0f;
		g_prim->m_animSpeed = 5;
		g_prim->m_totalTexture = 3;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\water_out_a.bmp", false);
		g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\water_out_b.bmp", false);
		g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\water_out_c.bmp", false);
	}
}

void CRagEffect :: WaterBall2()
{
	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\wizard_waterball_chulung.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		g_prim = LaunchEffectPrim(PP_3DPARTICLESPLINE);

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\Waterball.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\Waterball.act");

		g_prim->m_duration = 40;
		g_prim->m_deltaPos2.Set(0, -10, 0);
		g_prim->m_radius = g_prim->CalcDist(m_deltaPos.x, m_deltaPos.z);
		g_prim->CalcDir(m_deltaPos.x, m_deltaPos.z);
		g_prim->m_longitude = -g_prim->m_roty;
		g_prim->m_speed = g_prim->m_radius / g_prim->m_duration;
		g_prim->m_gravSpeed = 3.5f;
		g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration) * 2.0f;
		g_prim->m_latiSpeed = -2.0f;
		g_prim->m_latiAccel = -(g_prim->m_latiSpeed / (g_prim->m_duration + 0)) * 2.0f;
		g_prim->m_roll = 0;
		g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, 1), g_prim->m_matrix);
		g_prim->m_rotAxis[0] = g_prim->m_deltaPos3.x;
		g_prim->m_rotAxis[1] = g_prim->m_deltaPos3.y;
		g_prim->m_rotAxis[2] = g_prim->m_deltaPos3.z;
		g_prim->m_size = 0.75f;
		g_prim->m_numSegment = 12;
		g_prim->InitSegment();
	}
}

void CRagEffect :: FireIvy()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\wizard_fire_ivy.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if(m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = m_duration;
		g_prim->m_pattern = PT3DTEX_SHOWFRONT;
		g_prim->m_pattern |= PT3DTEX_ACCELARATION;
		g_prim->m_renderFlag |= RF_EMISSIVE;
		g_prim->m_widthSize = 3.5f;
		g_prim->m_heightSize = 3.5f;
		g_prim->m_speed3d.y = -(25.0f / g_prim->m_duration);

		g_prim->m_deltaPos2.y = -5.0f;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\fire_ivy.bmp", false);
	}
}

void CRagEffect :: Detecting()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\hunter_detecting.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if(m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = 57;
		g_prim->m_widthSpeed = 1.5f;
		g_prim->m_heightSpeed = 1.5f;
		g_prim->m_widthAccel = g_prim->m_widthSpeed / 14;
		g_prim->m_heightAccel = g_prim->m_heightSpeed / 14;
		g_prim->m_deltaPos2.y = -7.0f;
		g_prim->m_speed3d.y = -0.5f;
		g_prim->m_roll = 90;

		g_prim->m_ChangePoint = 17;
		g_prim->m_ChangeWidthSpeed = 0.1f;
		g_prim->m_ChangeHeightSpeed = 0.1f;
		g_prim->m_fadeOutCnt = 40;
		g_prim->m_ChangeWidthAccel = g_prim->m_ChangeWidthSpeed / 30;
		g_prim->m_ChangeHeightAccel = g_prim->m_ChangeHeightSpeed / 30;
		g_prim->m_deltaPos2.y = -5.0f;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\fashasha.tga", false);
	}
}

void CRagEffect :: Cloaking()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\assasin_cloaking.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	m_master->SetArgb(50, -1, -1, -1);
	m_shadowOn = false;
}

void CRagEffect :: SonicBlow()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_StoneCurse.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_2DCIRCLE);
		g_prim->m_duration = m_duration;
		g_prim->m_fadeOutCnt = 0;
		g_prim->m_deltaPos2.y = -20;
		g_prim->m_arcAngle = 10.0f;
		g_prim->m_innerSize = 10.0f;
		g_prim->m_radiusSpeed = 7.0f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / (g_prim->m_duration + 40)) * 2.0f;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_center.tga", false);
	}
}

void CRagEffect :: SonicBlowHit()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\assasin_sonicblow.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}


	if (m_stateCnt == 0) {
		int		angle = 180 - m_master->m_roty;
		int		randPosx = random(10);
		int		randPosy = random(20);
		int		randPosz = random(10);

		randPosx = (randPosx + 1) / 4;
		randPosy = (randPosy + 1) / 2;
		randPosz /= 4;

		if (angle < 0)
			angle += 360;

		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = 12;
		g_prim->m_longitude = angle;
		g_prim->m_latitude = -90;
		g_prim->m_matrix.MakeXRotation(GetRadian((int)g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian((int)g_prim->m_longitude));

		g_prim->m_speed3d.MatrixMult(vector3d(0, -2.0f, 0), g_prim->m_matrix);
		g_prim->m_deltaPos += g_prim->m_speed3d;

		g_prim->m_deltaPos2.Set(randPosx, -11 + randPosy, 0);
		g_prim->m_speed = 0.4f;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
		g_prim->m_outerSize = 7;
		g_prim->m_innerSize = 4;
		g_prim->m_heightSize = 3.5f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 2);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\magic_red.tga", false);
	}
}

void CRagEffect :: GrimTooth()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_FrostDiver.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		matrix	tmpMat;
		CalcDir(m_deltaPos.x, m_deltaPos.z);
		tmpMat.MakeYRotation(GetRadian(-m_roty));
		m_deltaPos2.MatrixMult(vector3d(0, 0, 2.0f), tmpMat);
		m_deltaPos3 -= m_deltaPos2;
	}

	if (!m_flag && m_stateCnt % 3 == 0) {
		vector3d	tmpVec = m_pos;


		if (CalcDist(m_deltaPos.x, m_deltaPos.z) <= 2.5f)
			m_flag = 1;

		m_pos = tmpVec;

		int		angle;
		float	sn, cs, length;

		g_prim = LaunchEffectPrim(PP_3DQUADHORN);
		g_prim->m_pattern |= PTQH_SPEEDLIMIT;
		g_prim->m_duration = 40;

		angle = random(360);
		length = (float)(random(10) + 5) / 10.0f;
		sn = GetSin(angle);
		cs = GetCos(angle);


		g_prim->m_deltaPos2.Set(0, 20, 0);
		{
			vector3d* deltaPos2 = &g_prim->m_deltaPos2;
			deltaPos2->x = m_deltaPos3.x + deltaPos2->x;
			deltaPos2->y = m_deltaPos3.y + deltaPos2->y;
			deltaPos2->z = m_deltaPos3.z + deltaPos2->z;
		}
		g_prim->m_size = (float)(random(40) + 60) / 100.0f;
		g_prim->m_heightSize = (float)(100) / 10.0f;

		g_prim->m_longitude = random(360);
		g_prim->m_latitude = random(30) + 75;
		g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian(g_prim->m_longitude));
		g_prim->m_rotMat = g_prim->m_matrix;
		g_prim->m_speed = 3.0f;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) * 2.0f;
		g_prim->m_count = 6;

		g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\stone.bmp", false);

		m_deltaPos3 -= m_deltaPos2;
	} else if(!m_flag) {
		m_deltaPos3 -= m_deltaPos2;
	}
}

void CRagEffect :: GrimToothAtk()
{
	if (m_stateCnt == 0) {
		int		angle;
		float	sn, cs, length;

		for(int a = 0; a < 3; a++) {
			g_prim = LaunchEffectPrim(PP_3DQUADHORN);
			g_prim->m_pattern |= PTQH_SPEEDLIMIT;
			g_prim->m_renderFlag |= RF_NODEPTHCHECK;
			g_prim->m_duration = m_duration = 1000;

			angle = a * 120;
			length = (float)(30) / 10.0f;
			sn = GetSin(angle);
			cs = GetCos(angle);

			switch(a) {
			case	0:
				g_prim->m_deltaPos2.Set(0, 40, -12);
				break;
			case	1:
				g_prim->m_deltaPos2.Set(12, 40, 6);
				break;
			case	2:
				g_prim->m_deltaPos2.Set(-12, 40, 6);
				break;
			}
			g_prim->m_size = (float)(90) / 100.0f;
			g_prim->m_heightSize = (float)(250) / 10.0f;

			g_prim->m_longitude = (6 - a) * 120;
			g_prim->m_latitude = 75;
			g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
			g_prim->m_matrix.AppendYRotation(GetRadian(g_prim->m_longitude));
			g_prim->m_rotMat = g_prim->m_matrix;
			g_prim->m_speed = 3.5f;
			g_prim->m_accel = 0.001f;
			g_prim->m_count = 10;
			g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\stone.bmp", false);

			m_deltaPos3 -= m_deltaPos2;
		}
	}
}

void CRagEffect :: EnchantPoison()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\assasin_enchantpoison.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		PlayWave("effect\\EF_PoisonAttack.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (m_stateCnt % 5 == 0) {

		g_prim = LaunchEffectPrim(PP_3DPARTICLE);

		g_prim->m_duration = 40;
		g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, random(6) + 2), g_prim->m_matrix);
		g_prim->m_matrix.MakeXRotation(GetRadian(90));
		g_prim->m_speed = (float)(random(30) + 30) / 100;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
		g_prim->m_size = (float)(random(50) + 40) / 100;
		g_prim->m_rollSpeed = 3.0f;
		g_prim->m_alpha = 0;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 5);
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle3.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle3.act");
		g_prim->InitSegment();
	}
}

void CRagEffect :: EnchantPoison2()
{
	if (m_stateCnt == 0 || m_stateCnt == 5 || m_stateCnt == 11 || m_stateCnt == 18 || m_stateCnt == 26 || m_stateCnt == 40)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\assasin_enchantpoison.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (m_stateCnt % 3 == 0) {
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = 40;
		g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, random(6) + 2), g_prim->m_matrix);
		g_prim->m_matrix.MakeXRotation(GetRadian(90));
		g_prim->m_speed = (float)(random(50) + 30) / 100;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
		g_prim->m_size = (float)(random(20) + 20) / 100;
		g_prim->m_rollSpeed = 3.0f;
		g_prim->m_alpha = 0;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 5);
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle3.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle3.act");
		g_prim->InitSegment();
	}
}

void CRagEffect :: Revive()
{
	if (m_stateCnt % 25 == 0)
	{
		g_prim = LaunchEffectPrim(PP_3DCYLINDER);

		g_prim->m_duration = 60;
		g_prim->m_longSpeed = 1.5f;
		g_prim->m_longAccel = 0.03f;
		g_prim->m_outerSize = 10.5f;
		g_prim->m_innerSpeed = -0.12f;
		g_prim->m_innerAccel = 0.003f;
		g_prim->m_outerSpeed = -0.12f;
		g_prim->m_outerAccel = 0.003f;
		g_prim->m_innerSize = 7.5f;

		g_prim->m_heightSize = 7.5f;
		g_prim->m_alpha = 240;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 2);

		if (m_stateCnt == 0)
		{
			g_prim->m_duration = m_duration;
			g_prim->m_innerSpeed = 0;
			g_prim->m_innerAccel = 0;
			g_prim->m_longSpeed = 1.5f;
			g_prim->m_outerSize = 11.0f;
			g_prim->m_innerSize = 7.5f;
			g_prim->m_outerSpeed = 0;
			g_prim->m_outerAccel = 0;
			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 10);
		}

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_yellow.tga", false);
	}
}

void CRagEffect :: OverThrust()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\black_overthrust.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt == 0) {
		m_duration = 9999;
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_StoneCurse.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_2DCIRCLE);
		g_prim->m_duration = 40;
		g_prim->m_deltaPos2.y = -20;
		g_prim->m_arcAngle = 10.0f;
		g_prim->m_innerSize = 10.0f;
		g_prim->m_radiusSpeed = 7.0f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / (g_prim->m_duration + 40)) * 2.0f;
		g_prim->m_fadeOutCnt = 0;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_center.tga", false);
	}

#if !defined(BALANCE_PATCH)
	if (m_stateCnt == 9998) {
		m_stateCnt = 5;
	}

	m_master->SetArgb(-1, 250, 150, 150);

	if (m_stateCnt % 3 == 0) {
		CBlurPC *blur = new CBlurPC((CPc *)m_master);
		blur->SendMsg(this, BM_RGB, 250, 150, 150);
		g_modeMgr.GetGameMode()->GetWorld()->m_gameObjectList.push_back(blur);
	}
#endif
}

void CRagEffect :: SlowPoison()
{
	if (m_stateCnt % 5 == 0) {
		if (m_stateCnt == 0) {
			vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
			PlayWave("effect\\EF_PoisonAttack.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		}
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = 40;
		g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, random(6) + 2), g_prim->m_matrix);
		g_prim->m_matrix.MakeXRotation(GetRadian(90));
		g_prim->m_deltaPos2.y = -20.0f;
		g_prim->m_speed = (float)(random(30) + 30) / 100 * -1;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
		g_prim->m_size = (float)(random(50) + 40) / 100;
		g_prim->m_rollSpeed = 3.0f;
		g_prim->m_alpha = 0;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 5);
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle3.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle3.act");
		g_prim->InitSegment();
	}
}

void CRagEffect :: FirePillarOn()
{
	if (m_stateCnt == 0) {
		for(int a = 0; a < 3; a++) {
			int speed = 1;

			speed *= -1;

			g_prim = LaunchEffectPrim(PP_3DCYLINDER);
			g_prim->m_duration = m_duration;
			g_prim->m_longSpeed = 3 * speed;
			g_prim->m_outerSize = 4 + a;
			g_prim->m_innerSize = 1.5 + (a + 1);
			g_prim->m_heightSize = 2.5f + ((3 - a) * (8 - (a * 2)) + 1);
			g_prim->m_rollSpeed = 0.5f * speed;
			g_prim->m_alpha = 240 - m_stateCnt * 7;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\magic_red.tga", false);
		}
	}
}

void CRagEffect :: HeavensDrive()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\wizard_earthspike.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);

		CGameMode * gameMode = g_modeMgr.GetGameMode();
		gameMode->SendMsg(MM_QUAKE);

		int		angle;
		float	sn, cs, length;

		m_duration += 50;
		for (int i = 0; i < 5; i++) {
			for(int j = 0; j < 5; j++) {
				g_prim = LaunchEffectPrim(PP_3DQUADHORN);
				g_prim->m_duration = m_duration - 10 + random(30);
				g_prim->m_pattern = PTQH_SPEEDLIMIT;
				g_prim->m_pattern |= PTQH_RETURNDOWN;

				angle = random(360);
				length = (float)(random(4) + 1) / 10.0f;
				sn = GetSin(angle);
				cs = GetCos(angle);

				float posX = -10 + j * 5;
				float posZ = -10 + i * 5;
				int rand = random(60);
				g_prim->m_deltaPos2.Set(posX, 10, posZ);
				g_prim->m_size = (float)(random(50) + 300) / 100.0f;
				g_prim->m_heightSize = 9.0f + (float)(rand + 1) / 10.0f;
				g_prim->m_longitude = random(360);
				g_prim->m_latitude = 80 + random(20);
				g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
				g_prim->m_matrix.AppendYRotation(GetRadian(g_prim->m_longitude));
				g_prim->m_rotMat = g_prim->m_matrix;
				g_prim->m_ChangeSpeedy = -1.0f;
				g_prim->m_ChangeAccely = -0.01f;
				g_prim->m_speed = 1.0f;
				g_prim->m_accel = 0.01f;

				g_prim->m_ChangePoint = 11;
				g_prim->m_ChangeSpeed = -1.2f;
				g_prim->m_ChangeAccel = 0.0f;
				g_prim->m_count = 14;
				g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\stone.bmp", false);
			}
		}
	}
}

void CRagEffect :: TopRank()
{
	m_pos = m_master->m_pos;
	if (m_stateCnt == 9998) {
		m_stateCnt = 0;
	}

	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = 9999;
		g_prim->m_longitude = 0;
		g_prim->m_longSpeed = 4.5f;

		g_prim->m_roll = 90;
		g_prim->m_widthSize = 4.0f;
		g_prim->m_heightSize = 4.0f;
		if (m_count != 11) {
			g_prim->m_red = 250;
			g_prim->m_blue = 0 + (10 - m_count) * 25;
			g_prim->m_green = 0 + (10 - m_count) * 25;
		} else {
			g_prim->m_red = 0;
			g_prim->m_blue = 0;
			g_prim->m_green = 250;
		}
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\LockOn128.tga", false);
	}

	m_pos.y = g_modeMgr.GetGameMode()->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z) - 2.0f;
}

void CRagEffect :: Party()
{
	m_pos = m_master->m_pos;
	if (m_stateCnt == 9998) {
		m_stateCnt = 0;
	}

	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = 9999;
		g_prim->m_longitude = 0;

		g_prim->m_roll = 90;
		g_prim->m_widthSize = 4.0f;
		g_prim->m_heightSize = 4.0f;
		g_prim->m_red = 50;
		g_prim->m_blue = 253;
		g_prim->m_green = 50;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\party.tga", false);
	}
	m_pos.y = g_modeMgr.GetGameMode()->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z) - 2.0f;
}

void	CRagEffect :: StatusState()
{
	if (m_stateCnt == 9998)
	{
		m_stateCnt = 0;
	}

	if (m_count != 0)
	{
		m_count = 0;
		m_stateCnt = 0;
		SendMsg(NULL, AM_CHILD_DEAD, 100);
	}

	if (m_stateCnt == 0)
	{
	int height;
	int y;
	int x;
	int DrawList[50];
	int cnt;
	int b;
	int i;
	int shown = 0;

	x = 0;
	y = 0;
	height = g_renderer->m_height - 50;
	cnt = 0;
	b = -1;
	while (b < 4) {
		++b;
		i = 0;
		int k = -1;
		while (i < (int)stateId.size()) {
			k++;
			i++;
			int drawId = stateId[k];
			if (drawId == 42)
				drawId = 12;

			char* fileName = g_session.GetStateFileName(stateId[k], b);
			if (fileName == NULL)
				continue;

			bool found = false;
			int n = -1;
			int j = 0;
			while (j < cnt) {
				n++;
				j++;
				if (drawId == DrawList[n]) {
					found = true;
					break;
				}
			}
			if (found)
				continue;

			g_prim = LaunchEffectPrim(PP_2DTEXTURE);
			g_prim->m_pattern = 4;
			g_prim->m_duration = 9999;
			g_prim->m_widthSize = 16.0f;
			g_prim->m_heightSize = 16.0f;

			if (shown * 35 + 150 >= height) {
				x -= 35;
				y = 35;
				shown = 0;
			} else {
				y += 35;
			}

			g_prim->m_deltaPos2.x = (float)(g_renderer->m_width + x - 32);
			g_prim->m_deltaPos2.y = (float)(y + 150);
			g_prim->m_GI[0].life = 0;
			g_prim->m_texture = new CTexture*[g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture(fileName, false);

			if (cnt < 99) {
				DrawList[cnt++] = drawId;
				++shown;
			}
		}
	}
	}
}

void	CRagEffect :: MakeBlur()
{
	if (m_master->m_bodyPalette == 0)
	{
		if (m_master->m_oldColor.a != 0) {
			m_master->SetArgb(-1, int(m_pos.x + m_master->m_curColor.r) >> 1, int(m_pos.y + m_master->m_curColor.g) >> 1, int(m_pos.z + m_master->m_curColor.b) >> 1);
			if (m_stateCnt == 9998) {
				m_stateCnt = 0;
			}
			if (m_stateCnt % 5 == 0) {
				CBlurPC *blur = new CBlurPC((CPc *)m_master);
				blur->SetArgb(-1, int(m_pos.x + m_master->m_curColor.r) >> 1, int(m_pos.y + m_master->m_curColor.g) >> 1, int(m_pos.z + m_master->m_curColor.b) >> 1);
				g_modeMgr.GetGameMode()->GetWorld()->m_gameObjectList.push_back(blur);
			}
		} else {
			m_master->SetArgb(-1, int(m_pos.x), int(m_pos.y), int (m_pos.z));

			if (m_stateCnt == 9998) {
				m_stateCnt = 0;
			}

			if (m_pos.x == -1.0f)
			{
				if (rand()%14>0)
					m_master->SetArgb(-1, 250, 200, 200);
				else
					m_master->SetArgb(-1, 250, 250, 250);
			}
			else if (m_pos.x == -2.0f)
			{
				m_master->SetArgb(-1, 250, 250, 200);
			}
			else if (m_duration == 30) {
				CBlurPC *blur = new CBlurPC((CPc *)m_master);
				blur->SendMsg(this, BM_RGB, int(m_pos.x), int(m_pos.y), int (m_pos.z));
				g_modeMgr.GetGameMode()->GetWorld()->m_gameObjectList.push_back(blur);
			} else {
				if (m_stateCnt % 5 == 0) {
					CBlurPC *blur = new CBlurPC((CPc *)m_master);
					blur->SendMsg(this, BM_RGB, int(m_pos.x), int(m_pos.y), int (m_pos.z));
					g_modeMgr.GetGameMode()->GetWorld()->m_gameObjectList.push_back(blur);
				}
			}
		}
	}
	else
	{
		if (m_master->m_oldColor.a != 0)
		{
			m_master->SetArgb(-1, int(m_pos.x + m_master->m_curColor.r) >> 1, int(m_pos.y + m_master->m_curColor.g) >> 1, int(m_pos.z + m_master->m_curColor.b) >> 1);
			if (m_stateCnt == 9998) {
				m_stateCnt = 0;
			}
		}
		else
		{
			m_master->SetArgb(-1, int(m_pos.x), int(m_pos.y), int (m_pos.z));

			if (m_stateCnt == 9998)
			{
				m_stateCnt = 0;
			}

			if (m_pos.x == -1.0f)
			{
					if (rand()%14>0)
						m_master->SetArgb(-1, 250, 200, 200);
					else
						m_master->SetArgb(-1, 250, 250, 250);
			}
			else if (m_pos.x == -2.0f)
			{
				m_master->SetArgb(-1, 250, 250, 200);
			}
			else if (m_duration == 30)
			{
					CBlurPC *blur = new CBlurPC((CPc *)m_master);
					blur->SendMsg(this, BM_RGB, int(m_pos.x), int(m_pos.y), int (m_pos.z));
					g_modeMgr.GetGameMode()->GetWorld()->m_gameObjectList.push_back(blur);
			}
		}

	}
}

void	CRagEffect :: TetraVoltex()
{
	m_pos = m_master->m_pos;
	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = m_duration;
		g_prim->m_longitude = 0;
		g_prim->m_longSpeed = 0;
		g_prim->m_roll = 90;
		g_prim->m_widthSize = 30.0f;
		g_prim->m_heightSize = 30.0f;
		g_prim->m_widthSpeed = -1.0f;
		g_prim->m_heightSpeed = -1.0f;
		g_prim->m_ChangePoint = 22;
		g_prim->m_ChangeWidthSpeed = 0;
		g_prim->m_ChangeHeightSpeed = 0;
		g_prim->m_ChangeWidthAccel = 0.0f;
		g_prim->m_ChangeHeightAccel = 0.0f;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\MagicCircle.bmp", false);
	} else {
		std::list<CEffectPrim*>::iterator i = m_primList.begin();
		CEffectPrim *p = (CEffectPrim *) *i;
		if (p->m_isChangeProcessed) {
			if (m_stateCnt >= 2000) {
				p->m_ChangePoint = 1;
				p->m_longSpeed = 4.5f;
				p->m_isChangeProcessed = FALSE;
			} else if (m_stateCnt >= 1500) {
				p->m_ChangePoint = 1998;
				p->m_longSpeed = -4.5f;
				p->m_isChangeProcessed = FALSE;
			} else if (m_stateCnt >= 1000) {
				p->m_ChangePoint = 1498;
				p->m_longSpeed = 4.5f;
				p->m_isChangeProcessed = FALSE;
			} else if (m_stateCnt >= 500) {
				p->m_ChangePoint = 998;
				p->m_longSpeed = -4.5f;
				p->m_isChangeProcessed = FALSE;
			} else if (m_stateCnt >= 300) {
				p->m_ChangePoint = 498;
				p->m_longSpeed = 4.5f;
				p->m_isChangeProcessed = FALSE;
			}
		}
	}

	m_pos.y = g_modeMgr.GetGameMode()->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z) - 2.0f;
}

void	CRagEffect :: VenomDust2()
{
	if (m_stateCnt % 5 == 0) {
		int a, r = 1;
		for(a = 0; a < r; a++) {
			g_prim = LaunchEffectPrim(PP_3DPARTICLE);
			g_prim->m_duration = 10;
			g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));

			g_prim->m_matrix.MakeXRotation(GetRadian(90));
			g_prim->m_speed = (float)(random(20) + 20) / 80;
			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
			g_prim->m_size = (float)(80) / 100;
			g_prim->m_rollSpeed = 3.0f;
			g_prim->m_alpha = 70;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;
			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 5);
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle3.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle3.act");
			g_prim->InitSegment();
		}
	}
}

void	CRagEffect :: ChangeFire()
{
	if (m_stateCnt == 0)
	{
		BeginCasting("effect\\ring_red.tga",3);
	}
}

void	CRagEffect :: ChangeCold()
{
	if (m_stateCnt == 0)
	{
		BeginCasting("effect\\ring_blue.tga",3);
	}
}

void	CRagEffect :: ChangeDark()
{
	if (m_stateCnt == 0)
	{
		BeginCasting("effect\\ring_black.tga",3);
	}
}

void	CRagEffect :: ChangeWind()
{
	if (m_stateCnt == 0)
	{
		BeginCasting("effect\\ring_yellow.tga", 3);
	}
}

void	CRagEffect :: ChangeFlame()
{
	if (m_stateCnt == 0)
	{
		BeginCasting("effect\\ring_jadu.tga",3);
	}
}

void	CRagEffect :: ChangeEarth()
{
	if (m_stateCnt == 0)
	{
		BeginCasting("effect\\ring_brown.tga",3);
	}
}

void	CRagEffect :: ChangeHoly()
{
	if (m_stateCnt == 0)
	{
		BeginCasting("effect\\ring_white.tga",3);
	}
}

void	CRagEffect :: ChangePoison()
{
	if (m_stateCnt == 0)
	{
		BeginCasting("effect\\ring_purple.tga",3);
	}
}

void	CRagEffect :: HitDark()
{
	if (m_stateCnt == 0) {
		int		angle = 180 - m_master->m_roty;

		if (angle < 0)
			angle += 360;

		g_prim = LaunchEffectPrim(PP_3DCYLINDER);
		g_prim->m_duration = 10;
		g_prim->m_longitude = angle;
		g_prim->m_latitude = -90;
		g_prim->m_matrix.MakeXRotation(GetRadian((int)g_prim->m_latitude));
		g_prim->m_matrix.AppendYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos2.Set(0, -10, 0);
		g_prim->m_speed = 0.7f;
		g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
		g_prim->m_outerSize = 10;
		g_prim->m_innerSize = 5;
		g_prim->m_heightSize = 3.5f;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 2);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);

		int i ;
		for (i = 0; i < 2; i++) {
			g_prim = LaunchEffectPrim(PP_3DPARTICLE);
			g_prim->m_longitude = (angle - 40) + random(80);
			g_prim->m_latitude = -90 + 40 + random(100);
			g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
			g_prim->m_matrix.AppendXRotation(GetRadian((int)g_prim->m_latitude));
			g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, random(4) + 4), g_prim->m_matrix);
			g_prim->m_deltaPos2 = vector3d(0, -10, 0) + g_prim->m_deltaPos3;
			g_prim->m_speed = (float)(random(90) + 60) / 100;
			g_prim->m_duration = random(24) + 6;
			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
			g_prim->m_size = (float)(random(100) + 60) / 100;

			g_prim->m_red = 20;
			g_prim->m_green = 20;
			g_prim->m_blue = 20;
			g_prim->m_fadeOutCnt = 0;
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
			g_prim->m_numSegment = 3;
			g_prim->InitSegment();
		}

		for (i = 0; i < 2; i++) {
			g_prim = LaunchEffectPrim(PP_3DPARTICLEGRAVITY);
			g_prim->m_longitude = random(80) + angle + 140;
			g_prim->m_latitude = -50 + random(100);
			g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
			g_prim->m_matrix.AppendXRotation(GetRadian((int)g_prim->m_latitude));
			g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, random(4) + 4), g_prim->m_matrix);
			g_prim->m_deltaPos2 = vector3d(0, -10, 0) + g_prim->m_deltaPos3;
			g_prim->m_matrix.MakeYRotation(GetRadian(g_prim->m_longitude));
			g_prim->m_duration = random(24) + 6;
			g_prim->m_speed = (float)(random(90) + 60) / 100;
			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
			g_prim->m_gravSpeed = -(float)(random(90) + 30) / 100;
			g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration * 2.0f);
			g_prim->m_size = (float)(random(100) + 60) / 100;

			g_prim->m_red = 20;
			g_prim->m_green = 20;
			g_prim->m_blue = 20;
			g_prim->m_fadeOutCnt = 0;
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
			g_prim->m_numSegment = 3;
			g_prim->InitSegment();
		}
	}
}

void	CRagEffect :: CurseAttack()
{
	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DTEXTURE);
		g_prim->m_duration = 9999;
		g_prim->m_longitude = 0;
		g_prim->m_longSpeed = 4.5f;


		g_prim->m_deltaPos2.y -= 30.0f;
		g_prim->m_widthSize = 4.0f;
		g_prim->m_heightSize = 4.0f;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\curse.bmp", false);


	}
}

void	CRagEffect :: DragonSmoke()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_3DPARTICLE, m_deltaPos);
		g_prim->m_duration = m_duration;
		g_prim->m_rollSpeed = 0.75f;
		g_prim->m_matrix.MakeXRotation(GetRadian(75 + random(30)));
		g_prim->m_matrix.AppendYRotation(GetRadian(random(360)));
		g_prim->m_deltaPos2.y = -9.0f;
		g_prim->m_speed = (float)(random(3) + 3) / 10.0f;
		g_prim->m_size = 1.5f;

		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb1\xbc\xb6\xd2\xbf\xac\xb1\xe2.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb1\xbc\xb6\xd2\xbf\xac\xb1\xe2.act");

		g_prim->InitSegment();
	}
}

void	CRagEffect :: DarkBreath()
{
	if (m_stateCnt == 0) {
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = 65;
		g_prim->m_renderFlag |= RF_NODEPTHCHECK;
		g_prim->m_deltaPos2.y -= 20;
		g_prim->m_size = (float)(80) / 100;
		g_prim->m_animSpeed = 1;
		g_prim->m_green = 0;
		g_prim->m_blue = 0;
		g_prim->m_fadeOutCnt = 60;
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\DarkBreath.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\DarkBreath.act");
		g_prim->InitSegment();
	}
}

void	CRagEffect :: SummonSlave()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;
	if (m_stateCnt == 0) {
		for (int i = 0; i < 20; i++) {
			g_prim = LaunchEffectPrim(PP_2DFLASH);
			g_prim->m_duration = 40;
			g_prim->m_deltaPos2.y = -40;
			g_prim->m_longitude = random(360);
			g_prim->m_longSpeed = (float)(random(60) + 10) / 10.0f;
			g_prim->m_longAccel = -(g_prim->m_longSpeed / g_prim->m_duration) / 1.5f;
			g_prim->m_arcAngle = (float)(random(25) + 5) / 10.0f;
			g_prim->m_heightSize = random(40) + 20;
			g_prim->m_heightSpeed = (float)(random(30) + 20) / 10.0f;
			g_prim->m_alpha = 0;
			g_prim->m_maxAlpha = 200;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;
			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("EFFECT\\alpha_center.tga", false);
		}
	} else if (m_stateCnt == 35) {
		if (m_master)
			m_master->SendMsg(NULL, AM_MAKE_SMOKE_FFECT, 0, 0, 0);
	}
}

void	CRagEffect :: BloodDrain()
{

	if (m_stateCnt == 0) {
		for(int a = 0; a < 3; a++) {



			int i = (-90 + (a * 90));
			g_prim = LaunchEffectPrim(PP_3DPARTICLESPLINE);
			g_prim->m_duration = 60;
			g_prim->m_deltaPos2.Set(0, -10, 0);
			g_prim->m_radius = g_prim->CalcDist(m_deltaPos.x, m_deltaPos.z);
			g_prim->CalcDir(m_deltaPos.x, m_deltaPos.z);
			g_prim->m_longitude = -g_prim->m_roty;
			g_prim->m_speed = (g_prim->m_radius / g_prim->m_duration) * 2.0f;
			g_prim->m_gravSpeed = 2.0f;
			g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration) * 2.0f;
			g_prim->m_latiSpeed = -2.0f;
			g_prim->m_latiAccel = -(g_prim->m_latiSpeed / g_prim->m_duration) * 2.0f;
			g_prim->m_roll = i;
			g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
			g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, 1), g_prim->m_matrix);
			g_prim->m_rotAxis[0] = g_prim->m_deltaPos3.x;
			g_prim->m_rotAxis[1] = g_prim->m_deltaPos3.y;
			g_prim->m_rotAxis[2] = g_prim->m_deltaPos3.z;
			g_prim->m_red = 250;
			g_prim->m_green = 100;
			g_prim->m_blue = 100;
			g_prim->m_size = 1.7f;
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
			g_prim->m_numSegment = 7;
			g_prim->InitSegment();
		}
	}
}

void	CRagEffect :: EnergyDrain()
{

	if (m_stateCnt == 0) {
		for(int a = 0; a < 3; a++) {



			int i = (-90 + (a * 90));
			g_prim = LaunchEffectPrim(PP_3DPARTICLESPLINE);
			g_prim->m_duration = 60;
			g_prim->m_deltaPos2.Set(0, -10, 0);
			g_prim->m_radius = g_prim->CalcDist(m_deltaPos.x, m_deltaPos.z);
			g_prim->CalcDir(m_deltaPos.x, m_deltaPos.z);
			g_prim->m_longitude = -g_prim->m_roty;
			g_prim->m_speed = (g_prim->m_radius / g_prim->m_duration) * 2.0f;
			g_prim->m_gravSpeed = 2.0f;
			g_prim->m_gravAccel = -(g_prim->m_gravSpeed / g_prim->m_duration) * 2.0f;
			g_prim->m_latiSpeed = -2.0f;
			g_prim->m_latiAccel = -(g_prim->m_latiSpeed / g_prim->m_duration) * 2.0f;
			g_prim->m_roll = i;
			g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
			g_prim->m_deltaPos3.MatrixMult(vector3d(0, 0, 1), g_prim->m_matrix);
			g_prim->m_rotAxis[0] = g_prim->m_deltaPos3.x;
			g_prim->m_rotAxis[1] = g_prim->m_deltaPos3.y;
			g_prim->m_rotAxis[2] = g_prim->m_deltaPos3.z;
			g_prim->m_red = 100;
			g_prim->m_green = 100;
			g_prim->m_blue = 250;
			g_prim->m_size = 1.7f;
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
			g_prim->m_numSegment = 7;
			g_prim->InitSegment();
		}
	}
}

void	CRagEffect :: PotionPillar(float speed, int duration)
{
	g_prim = LaunchEffectPrim(PP_3DCYLINDER);
	g_prim->m_duration = duration;
	g_prim->m_outerSize = 4.5f;
	g_prim->m_innerSize = 4.5f;
	g_prim->m_heightSize = 10.0f;
	g_prim->m_heightSpeed = speed;
	g_prim->m_heightAccel = 0.01f;
	g_prim->m_alpha = 0;
	g_prim->m_maxAlpha = 90;
	g_prim->m_alphaSpeed = (g_prim->m_maxAlpha / 20 );
	g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
	g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
	g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);

}

void	CRagEffect :: Test()
{
	if (m_stateCnt % 5 == 0) {
		int a, r = 1;
		for(a = 0; a < r; a++) {
			g_prim = LaunchEffectPrim(PP_3DPARTICLE);
			g_prim->m_duration = 10;
			g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
			g_prim->m_matrix.MakeXRotation(GetRadian(90));
			g_prim->m_speed = (float)(random(20) + 20) / 80;
			g_prim->m_accel = -(g_prim->m_speed / g_prim->m_duration) / 2.0f;
			g_prim->m_size = (float)(80) / 100;
			g_prim->m_rollSpeed = 3.0f;
			g_prim->m_alpha = 70;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;
			g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 5);
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle3.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle3.act");
			g_prim->InitSegment();
		}
	}
	m_pos.y = g_modeMgr.GetGameMode()->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z) - 4.5f;

}

bool CRagEffect::Angelus()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_Angelus.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect::Signum()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	if (m_stateCnt == 30) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_Signum.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		PlayWave("effect\\EF_Bash.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		g_prim = LaunchEffectPrim(PP_2DCIRCLE);
		g_prim->m_duration = 40;
		g_prim->m_pattern |= PT_FILLCIRCLE;
		g_prim->m_deltaPos2.y = -40;
		g_prim->m_radius = 100;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 170;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 6;
		g_prim->m_fadeOutCnt = g_prim->m_duration - (g_prim->m_duration / 3);
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);

		for (int i = 0; i < 20; i++) {
			g_prim = LaunchEffectPrim(PP_2DFLASH);
			g_prim->m_duration = 35;
			g_prim->m_deltaPos2.y = -40;
			g_prim->m_alpha = 0;
			g_prim->m_longitude = random(360);
			g_prim->m_longSpeed = (float)(random(60) + 10) / 10.0f;
			g_prim->m_longAccel = -(g_prim->m_longSpeed / g_prim->m_duration) / 1.5f;
			g_prim->m_arcAngle = (float)(random(25) + 15) / 10.0f;
			g_prim->m_heightSize = random(60) + 30;
			g_prim->m_heightSpeed = (float)(random(35) + 95) / 10.0f;
			g_prim->m_maxAlpha = 200;
			g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10.0f;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("EFFECT\\alpha_center.tga", false);
		}
	}
	if (!ProcessEZ2STR())
		return false;
	return true;
}

bool CRagEffect::Gloria()
{
	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_gloria.wav", m_pos.x-pPos.x, 1,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect::Mvp()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\st_mvp.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect::Cure()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\Acolyte_cure.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect::Invenom()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\thief_invenom.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect::Provoke()
{
	if (m_stateCnt == 5) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\swordman_provoke.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
	if (m_stateCnt == 30) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\swordman_provoke.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect::SkidTrap()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\hunter_skidtrap.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect::Magnificat()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_magnificat.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect::Resurrection()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_resurrection.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect::Recovery()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_recovery.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: Sanctuary()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_sanctuary.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: Lord()
{
	switch (m_stateCnt) {
		case 0:
			{
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\wizard_meteo.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				PlayWave("effect\\hunter_blastmine.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
			break;
		case 10:
			{
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\hunter_blastmine.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
			break;
		case 20:
			{
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\wizard_meteo.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
			break;
		case 50:
			{
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\wizard_meteo.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
			break;
		case 80:
			{
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\wizard_meteo.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
			break;
		case 100:
			{
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\wizard_meteo.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
			break;
		case 130:
			{
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\wizard_meteo.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
			break;
		case 140:
			{
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\wizard_meteo.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
			break;
		case 180:
			{
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\wizard_meteo.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
			break;
		case 200:
			{
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\wizard_meteo.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
			break;
		}

	if (m_stateCnt >= 50 && m_stateCnt % 50 == 0 && m_stateCnt < 300) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		gameMode->SendMsg(MM_QUAKE);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: StormGust()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\wizard_stormgust.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

 	if (!g_session.m_isMinEffect && m_stateCnt % 29 == 0 && m_stateCnt > 30 && m_stateCnt <= 150) {
		for (int a = 0; a < 2; a++) {
			float degree = random(360), angle, cs, sn;
			float		radius = random(25) + 3;

			g_prim = LaunchEffectPrim(PP_3DQUADHORN);
			g_prim->m_duration = 215 - m_stateCnt;
			g_prim->m_pattern |= PTQH_SPEEDLIMIT;

			angle = random(360);
			sn = GetSin(angle);
			cs = GetCos(angle);

			g_prim->m_deltaPos2.Set(radius * sn, 20, -radius * cs);
			g_prim->m_size = (float)(random(100) + 400) / 100.0f;
			g_prim->m_heightSize = 20.0f;
			g_prim->m_longitude = degree;
 			g_prim->m_latitude = 100;
			g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
			g_prim->m_matrix.AppendYRotation(GetRadian(g_prim->m_longitude));
			g_prim->m_rotMat = g_prim->m_matrix;
			g_prim->m_speed = 1.5f;
			g_prim->m_accel = 0.0f;
			g_prim->m_alpha = 250;

			g_prim->m_count = 3;
			g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ice.tga", false);
		}
	}
	if (!ProcessEZ2STR())
		return false;
	return true;
}

bool CRagEffect :: Impositio()
{
	if (m_stateCnt == 60) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_impositio.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: Suffragium()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_suffragium.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: LexDivina()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_lexdivina.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
	if (m_stateCnt == 20) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_lexdivina.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
	if (m_stateCnt == 35) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_lexdivina.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
	if (m_stateCnt == 60) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_lexdivina.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
	if (m_stateCnt == 70) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_lexdivina.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: LexAeterna()
{
	if (m_stateCnt == 15) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_lexaeterna.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: Aspersio()
{
	if (m_stateCnt == 70) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_aspersio.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (m_stateCnt % 5 == 0 && m_stateCnt > 60 && m_stateCnt < 140) {
		g_prim = LaunchEffectPrim(PP_3DPARTICLEORBIT);
		m_duration = 400;
		g_prim->m_duration = 40;
		g_prim->m_radius = 1.5f;
		g_prim->m_gravSpeed = 0.19f;
		g_prim->m_gravAccel = 0.01f;
		g_prim->m_longitude = random(360);
		g_prim->m_size = 1.2f;
		g_prim->m_matrix.MakeYRotation(GetRadian((int)g_prim->m_longitude));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, -30, g_prim->m_radius), g_prim->m_matrix);

		g_prim->m_fadeOutCnt = 30;
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
		g_prim->InitSegment();
	}
	if( m_stateCnt == m_duration)
		return false;
	ProcessEZ2STR();
	return true;
}

bool CRagEffect :: Benedictio()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_benedictio.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: Quagmire()
{
	if (m_stateCnt == 0) {
		if (random(12)==0)
		{
			vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
			PlayWave("effect\\wizard_quagmire.wav", m_pos.x-pPos.x, -100,  m_pos.z-pPos.z);
		}
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: MeteorStorm()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\wizard_meteor.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (m_stateCnt == 40) {
		g_modeMgr.GetGameMode()->SendMsg(MM_QUAKE);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: FirePillar()
{
	if (m_stateCnt == 75) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\wizard_fire_pillar_a.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: FirePillarBomb()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\wizard_fire_pillar_b.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: RepairWeapon()
{
	if (m_stateCnt == 20) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\black_weapon_repair_a.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (m_stateCnt == 55) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\black_weapon_repair_a.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: CrashEarth()
{
	if (m_stateCnt == 30) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\wizard_fire_pillar_b.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (m_stateCnt == 35) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		gameMode->SendMsg(MM_QUAKE);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: Perfection()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\black_weapon_perfection.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: MaxPower()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\black_maximize_power_circle.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (m_stateCnt == 40) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\black_maximize_power_sword.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}
	if (m_stateCnt == 47) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\black_maximize_power_sword.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (m_stateCnt == 65) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\black_maximize_power_sword_bic.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

void CRagEffect :: BlastMine()
{
}

bool CRagEffect :: Magnus()
{
	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_magnus.wav", m_pos.x - pPos.x, 1, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;

	return true;
}

bool CRagEffect :: BlastMineBomb()
{
	if (m_stateCnt == 20) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\hunter_blastmine.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: Claymore()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\hunter_claymoretrap.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: Freezing()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\hunter_freezingtrap.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: SpringTrap()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\hunter_springtrap.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: Kyrie()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_kyrie_eleison_b.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (m_stateCnt == 15) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\priest_kyrie_eleison_a.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: VenomDust()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\assasin_poisonreact.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: AutoCounter()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\knight_autocounter.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: PoisonReact2()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\assasin_poisonreact.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: Splasher()
{
	if (m_stateCnt == 10) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\assasin_venomsplasher.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: Freeze()
{
	if (!ProcessEZ2STR(0, -80.0f, 0)) {
		m_stateCnt = 9999;
		return false;
	}
	return true;
}

bool CRagEffect :: Freezed()
{
	if (!ProcessEZ2STR(0, -80.0f, 0)) {
		m_stateCnt = 9999;
		return false;
	}
	return true;
}

bool CRagEffect :: FireHit()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave(g_session.GetAttrWaveName(ATTR_FIRE), m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: ShockWave()
{
	if (!ProcessEZ2STR(0, -80.0f, 0)) {
		InitEZ2STRFrame();
	}
	return true;
}

bool CRagEffect :: TwoHandQuicken()
{
	if (m_stateCnt == 0) {
		m_duration = 9999;
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\knight_twohandquicken.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

#if !defined(BALANCE_PATCH)
	if (m_stateCnt == 9998) {
		m_stateCnt = 5000;
	}

	m_master->SetArgb(-1, 200, 200, 0);
	if (m_stateCnt % 5 == 0) {
		CBlurPC *blur = new CBlurPC((CPc *)m_master);
		blur->SendMsg(this, BM_RGB, 200, 200, 0);
		g_modeMgr.GetGameMode()->GetWorld()->m_gameObjectList.push_back(blur);
	}

	if (m_stateCnt < 5000) {
		if (!ProcessEZ2STR(0, -80.0f, 0))
			return false;
	}
#else
	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
#endif
	return true;
}

bool CRagEffect :: Concentration()
{
	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\ac_concentration.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: RefineOk()
{
	if (m_stateCnt == 15) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\bs_refinesuccess.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool CRagEffect :: RefineFail()
{
	if (m_stateCnt == 5) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\bs_refinefailed.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

#if defined(BALANCE_PATCH)
bool	CRagEffect :: CartRevolution()
{
 	if (m_stateCnt == 7 || m_stateCnt == 20)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_MagnumBreak.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_3DCIRCLE);
		g_prim->m_duration = 20;
		g_prim->m_renderFlag |= RF_NODEPTHCHECK;
		g_prim->m_maxAlpha = 180;
		g_prim->m_latitude = 90;
		g_prim->m_radiusSpeed = 1.75f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / g_prim->m_duration) / 2.0f;
		g_prim->m_innerSize = 5.0f;
		g_prim->m_alpha = 0;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 15;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 15;
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_yellow.tga", false);

		g_prim = LaunchEffectPrim(PP_3DSPHERE);
		g_prim->m_duration = 20;
		g_prim->m_radiusSpeed = 1.35f;
		g_prim->m_radiusAccel = -(g_prim->m_radiusSpeed / g_prim->m_duration) / 2.0f;
		g_prim->m_longSpeed = 3.0f;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 240;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 7;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 15;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\\xb4\xeb\xc6\xf8\xb9\xdf.tga", false);

	}

	if (m_stateCnt >= 300) {
		return false;
	}
	ProcessEZ2STR(0, -80.0f, 0);
	return true;
}
#endif

bool	CRagEffect :: Suicide()
{
	m_deltaPos2 = m_master->m_pos;
	m_master->m_pos = m_deltaPos;

	if (!ProcessEZ2STR(0, -80.0f, 0))
	{
		m_master->m_pos = m_deltaPos2;
		return false;
	}

	m_master->m_pos = m_deltaPos2;
	return true;
}

bool	CRagEffect :: SuiExplosion()
{
	if (m_stateCnt == 0) {
		Hit2(1);
	} else if (m_stateCnt == 3 || m_stateCnt == 7 || m_stateCnt == 9) {
		((CMode *)g_modeMgr.GetGameMode())->SendMsg(MM_QUAKE, 1, 0, 0);
	}

	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool	CRagEffect :: Potion_Con()
{
	m_pos = m_master->m_pos;
	if (m_stateCnt == 0)
	{
		PlayWave("effect\\ac_concentration.wav");
		PotionPillar(2.0f);
	}
	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool	CRagEffect :: Potion_()
{
	m_pos = m_master->m_pos;
	if (m_stateCnt == 30)
	{
		PlayWave("effect\\ac_concentration.wav", 0, 0, 0, 250, 40, 1.0f);
		PotionPillar(1.9f, 30);
	}
	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}

bool	CRagEffect :: Potion_Berserk()
{
	if (m_stateCnt == 0) {
		m_master->SendMsg(NULL, AM_CHANGE_COLOR, 250, 120, 120);
		PotionPillar(1.0f, 50);
	}

 	if (m_stateCnt == 18) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		gameMode->SendMsg(MM_QUAKE, 1);
		PlayWave("effect\\ac_concentration.wav", 0, 0, 0, 250, 40, 1.0f);
	}

	m_pos = m_master->m_pos;
	if (m_stateCnt > 12 && m_stateCnt < 110 && m_stateCnt % 4 == 0) {
		g_prim = LaunchEffectPrim(PP_3DCROSSTEXTURE);
		g_prim->m_duration = 40;
		g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, random(7) + 2), g_prim->m_matrix);
		g_prim->m_matrix.MakeXRotation(GetRadian(90));
		g_prim->m_speed = (float)(random(50) + 20) / 60;
		g_prim->m_widthSize = (float)(random(60) + 30) / 10;
		g_prim->m_heightSize = 0.2f;
		g_prim->m_red = 240;
		g_prim->m_green = 0;
		g_prim->m_blue = 0;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 180;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 20;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 20;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ac_center2.tga", false);
		g_prim->m_GI[0].life=0;
	}

	ProcessEZ2STR(0, -80.0f, 0);
	if (m_stateCnt > 150) {
		m_master->SendMsg(NULL, AM_RETURN_COLOR, 0, 0, 0);
		return false;
	}
	return true;
}

bool	CRagEffect :: EZ2STROnlyEffect()
{
	if (!ProcessEZ2STR(0, -80.0f, 0))
		return false;
	return true;
}
