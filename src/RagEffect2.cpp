#include "common.h"
#include "View.h"
#include "GameWorld.h"
#include "GameMode.h"
#include "RagEffect.h"
#include "Session.h"
#include "GameActor.h"
#include "device/Sound.h"
#include "3dlib/Prim.h"
#include "resource/Attr.h"
#include "resource/Wave.h"
#include "3dActor.h"
#include "packet.h"
#include "pdb_enums.h"

extern BOOL g_isSkipFrame;

#include "RagEffect2.h"
#include "RagEffectPrim.h"
#include "Mode.h"
#include "window/UISys.h"
#include "3dGround.h"

extern CModeMgr g_modeMgr;
extern CSession g_session;
extern UIWindowMgr g_windowMgr;
extern CRenderer *g_renderer;
extern CEffectPrim *g_prim;

#include "resource/Ground.h"

#define E_DIVISION 21



void CRagEffect::BlueCasting()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, -100,  m_pos.z-pPos.z);

		SAINTCASTING(45,"effect\\ring_blue.tga",5);
		SAINTCASTING(25,"effect\\ring_blue.tga",5);
	}
}

void CRagEffect::DarkCasting()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		SAINTCASTING(45,"effect\\ring_black.tga",6);
		SAINTCASTING(25,"effect\\ring_black.tga",6);
	}
}

void CRagEffect::AuraBladeCasting()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		SAINTCASTING(45,"effect\\ring_white.tga",22);
		SAINTCASTING(25,"effect\\ring_yellow.tga",22);
	}
}

void CRagEffect :: BeginCasting(char *tName, char option)
{

	if (m_duration<70)
		m_duration = 70;

	{
		m_pos = m_master->m_pos;

		if (option == 33)
		{
			g_prim = LaunchEffectPrim(PP_3DCASTING_3);
			option = 0;
		}
		else
			g_prim = LaunchEffectPrim(PP_3DCASTING);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=option;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = 25.0f;
		g_prim->m_GI[ec].vecB_now.y=0;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 180;
		g_prim->m_GI[ec].distance = 4.5f;
		g_prim->m_GI[ec].rise_angle=70;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].alphaB = 180;
		g_prim->m_GI[ec].distance = 5.0f;
		g_prim->m_GI[ec].max_height = 22.0f;
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].rise_angle=57;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].rise_angle=45;
		g_prim->m_GI[ec].distance = 5.5f;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = 19.0f;
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].alphaB = 180;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].distance = 4.0f;
		g_prim->m_GI[ec].rise_angle=89;
		g_prim->m_GI[ec].vecB_now.y=0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 250.0f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 70;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}
	}
}

void CRagEffect :: BeginCasting2(char *tName, char option)
{

	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_FireWall.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		m_pos = m_master->m_pos;

		g_prim = LaunchEffectPrim(PP_ACIDDEMON);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=13;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = 25.0f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 2.4f;
		g_prim->m_GI[ec].rise_angle=70;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = 22.0f;
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 2.7f;
		g_prim->m_GI[ec].rise_angle=57;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = 19.0f;
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].rise_angle=45;
		g_prim->m_GI[ec].distance = 3.0f;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 30.0f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 2.2f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}
	}
}

void CRagEffect :: FlowerCasting(char *tName, char option, char F1)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_FLOWERCAST);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=option;



		int ec,i;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].full_display_angle = 315;
			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].vecB_now.y=0;

			if (F1 == 0)
			{
				if (ec == 0)
				{
					g_prim->m_GI[ec].distance = 4.5f;
					g_prim->m_GI[ec].rise_angle=85;
					g_prim->m_GI[ec].max_height = 27.0f;
					g_prim->m_GI[ec].RotStart = 0;
				}
				else if (ec == 1)
				{
					g_prim->m_GI[ec].distance = 5.0f;
					g_prim->m_GI[ec].rise_angle=80;
					g_prim->m_GI[ec].max_height = 28.0f;
					g_prim->m_GI[ec].RotStart = 90;
				}
				else if (ec == 2)
				{
					g_prim->m_GI[ec].distance = 5.5f;
					g_prim->m_GI[ec].rise_angle=70;
					g_prim->m_GI[ec].max_height = 29.0f;
					g_prim->m_GI[ec].RotStart = 180;
				}
				else if (ec == 3)
				{
					g_prim->m_GI[ec].distance = 5.0f;
					g_prim->m_GI[ec].rise_angle=75;
					g_prim->m_GI[ec].max_height = 30.0f;
					g_prim->m_GI[ec].RotStart = 270;
				}
			}
			else if (F1 == 1)
			{
				if (ec == 0)
				{
					g_prim->m_GI[ec].distance = 5.5f;
					g_prim->m_GI[ec].rise_angle=50;
					g_prim->m_GI[ec].max_height = 30.0f;
					g_prim->m_GI[ec].RotStart = 90;
				}
				else if (ec == 1)
				{
					g_prim->m_GI[ec].distance = 6.0f;
					g_prim->m_GI[ec].rise_angle=55;
					g_prim->m_GI[ec].max_height = 29.0f;
					g_prim->m_GI[ec].RotStart = 180;
				}
				else if (ec == 2)
				{
					g_prim->m_GI[ec].distance = 6.5f;
					g_prim->m_GI[ec].rise_angle=65;
					g_prim->m_GI[ec].max_height = 28.0f;
					g_prim->m_GI[ec].RotStart = 270;
				}
				else if (ec == 3)
				{
					g_prim->m_GI[ec].distance = 6.0f;
					g_prim->m_GI[ec].rise_angle=60;
					g_prim->m_GI[ec].max_height = 27.0f;
					g_prim->m_GI[ec].RotStart = 0;
				}
			}

			g_prim->m_GI[ec].vecT_now.x=g_prim->m_GI[ec].rise_angle;
			g_prim->m_GI[ec].vecT_now.y=g_prim->m_GI[ec].max_height;

			for (i = 0 ; i<E_DIVISION ; i++)
			{
				g_prim->m_GI[ec].height[i] = 0.0f;
				g_prim->m_GI[ec].flag1[i] = 0;
			}
		}

	}
}

void	CRagEffect :: CartBoost()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	tlvertex3d	tlvert = TransformVert(gameMode->GetView()->GetViewMatrix(), m_master->m_pos);

	m_tlvertX = tlvert.x;
	m_tlvertY = tlvert.y;

	m_pos = m_master->m_pos;

	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_IncAgility.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (m_stateCnt % 2 == 0) {
		g_prim = LaunchEffectPrim(PP_3DCROSSTEXTURE);
		g_prim->m_duration = 50;
		g_prim->m_matrix.MakeYRotation(GetRadian(random(360)));
		g_prim->m_deltaPos2.MatrixMult(vector3d(0, 0, random(7) + 2), g_prim->m_matrix);
		g_prim->m_matrix.MakeXRotation(GetRadian(90));
		g_prim->m_speed = (float)(random(50) + 20) / 100;
		g_prim->m_widthSize = (float)(random(100) + 20) / 10;
		g_prim->m_heightSize = 0.10f+(random(4)*0.1f);
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 60;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 20;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 20;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ac_center2.tga", false);
	}
}

void CRagEffect::BeginAsura(char F1)
{
	int ec;
	m_pos = m_master->m_pos;
	if (m_stateCnt == 0)
	{
		if (m_duration<350)
			m_duration = 350;
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		SAINTCASTING(45,"effect\\ring_white.tga",2);
		SAINTCASTING(25,"effect\\ring_white.tga",2);
	}

	if (m_stateCnt == 1)
	{
		g_prim = LaunchEffectPrim(PP_ASURACASTING);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture=3;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		if (F1 == 0)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\asura3.tga", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\asura2.tga", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\asura1.tga", false);
		}
		else
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\asura13.tga", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\asura12.tga", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\asura11.tga", false);
		}

		g_prim->m_size=0;




		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		for (ec=0;ec<3;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = -((2-ec)*20);
			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].flag1[0]=ec;
			if (F1 == 0)
			{
				g_prim->m_GI[ec].distance = 18.0f;
				g_prim->m_GI[ec].height[0]=-6 +(-ec*12);
				g_prim->m_GI[ec].flag1[4]=0;
			}
			else
			{
				g_prim->m_GI[ec].distance = 24.0f;
				g_prim->m_GI[ec].height[0]=-8 +(-ec*16);
				g_prim->m_GI[ec].flag1[4]=5;
			}

			for (int i=1;i<11;i++)
				g_prim->m_GI[ec].height[i]=0;

		}
	}

	if (m_stateCnt == 21)
	{
		g_prim = LaunchEffectPrim(PP_ASURACASTING);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture=3;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		if (F1 == 0)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\asura4.tga", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\asura5.tga", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\asura6.tga", false);
		}
		else
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\asura14.tga", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\asura15.tga", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\asura16.tga", false);
		}

		g_prim->m_size=0;




		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		for (ec=0;ec<3;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = -(ec*20)-40;

			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].flag1[0]=ec;
			if (F1 == 0)
			{
				g_prim->m_GI[ec].distance = 18.0f;
				g_prim->m_GI[ec].height[0]=6 +(ec*12);
				g_prim->m_GI[ec].flag1[4]=0;
			}
			else
			{
				g_prim->m_GI[ec].distance = 24.0f;
				g_prim->m_GI[ec].height[0]=8 +(ec*16);
				g_prim->m_GI[ec].flag1[4]=5;
			}

			for (int k=1;k<11;k++)
				g_prim->m_GI[ec].height[k]=0;

		}
	}
}

void CRagEffect::BeginAsura1(char* tName)
{
	int ec;
	m_pos = m_master->m_pos;
	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_ASURACASTING);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture=1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;




		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;

			g_prim->m_GI[ec].distance = 18.0f;

			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].height[0]=0;
			g_prim->m_GI[ec].flag1[0]=0;
			g_prim->m_GI[ec].flag1[4]=1;
			for (int i=1;i<21;i++)
			{
				g_prim->m_GI[ec].height[i]=0;
			}

		}
	}
}

void CRagEffect::BeginAsura2(char F1)
{
	int ec;
	m_pos = m_master->m_pos;

	if (m_stateCnt == 1)
	{
		g_prim = LaunchEffectPrim(PP_ASURACASTING);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture=4;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\soul_l.tga", false);
		g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\soul_u.tga", false);
		g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\soul_o.tga", false);
		g_prim->m_texture[3] = g_texMgr.GetTexture("effect\\soul_s.tga", false);

		g_prim->m_size=0;




		for (ec=0;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = -0-((3-ec)*20);
			g_prim->m_GI[ec].distance = 5.0f;

			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].height[0]=-4 + (-(ec)*6);
			g_prim->m_GI[ec].flag1[0]=ec;
			g_prim->m_GI[ec].flag1[4]=2;

			for (int i=1;i<11;i++)
				g_prim->m_GI[ec].height[i]=0;
		}
	}

	if (m_stateCnt == 21)
	{
		g_prim = LaunchEffectPrim(PP_ASURACASTING);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture=4;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\soul_k.tga", false);
		g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\soul_n.tga", false);
		g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\soul_i.tga", false);
		g_prim->m_texture[3] = g_texMgr.GetTexture("effect\\soul_l.tga", false);

		g_prim->m_size=0;




		for (ec=0;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = -60-((3-ec)*20);
			g_prim->m_GI[ec].distance = 5.0f;

			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].height[0]=22 + (-(ec)*6);
			g_prim->m_GI[ec].flag1[0]=ec;
			g_prim->m_GI[ec].flag1[4]=2;

			for (int i=1;i<11;i++)
				g_prim->m_GI[ec].height[i]=0;

		}
	}
}

void CRagEffect::StormKick(char *tName, char F1)
{
	int ec;
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_STORMKICK);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture=1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;




		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].max_height = 40.0f;
			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].RotStart = random(360);
			g_prim->m_GI[ec].distance = 2.0f;
			g_prim->m_GI[ec].height[0] = 50.0f;
			g_prim->m_GI[ec].height[1] = 5.0f;
			g_prim->m_GI[ec].flag1[0] = 10;
			g_prim->m_GI[ec].flag1[4] = F1;
		}
	}
}

void CRagEffect::HitLine(char F1)
{
	int ec;
	m_pos = m_master->m_pos;

	if (m_stateCnt >= 0 && m_stateCnt < 4)
	{
		g_prim = LaunchEffectPrim(PP_HITLINE);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture=1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\white02.bmp", false);

		g_prim->m_size=0;




		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = -random(26);
			g_prim->m_GI[ec].max_height = 0.10f;

			g_prim->m_GI[ec].alphaB = 125+random(51);
			g_prim->m_GI[ec].rise_angle = random(360);
			g_prim->m_GI[ec].distance = 6.0f+random(8);

			g_prim->m_GI[ec].flag1[0] = 0;
			g_prim->m_GI[ec].height[0] = 0;

			g_prim->m_GI[ec].vecB_now = m_pos;

			for (int i=20;i>=1;i--)
			{
				g_prim->m_GI[ec].height[i]=0;
				g_prim->m_GI[ec].flag1[i]=0;
			}

			g_prim->m_GI[ec].alphaT = F1;
		}
	}
}

void CRagEffect::HitLine3(char F1)
{
	int ec;
	int uNum = 0;
	int start = 0;
	m_pos = m_master->m_pos;

	if (F1 == 1)
	{
		start=0;
		uNum=2;
	}
	else if (F1 == 3)
	{
		start = 0;
		uNum = 5;
	}

	if (m_stateCnt >= start && m_stateCnt < uNum)
	{
		g_prim = LaunchEffectPrim(PP_HITLINE3);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture=1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\white02.bmp", false);

		g_prim->m_size=0;




		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			if (F1 == 1)
				g_prim->m_GI[ec].process = -random(11);
			else if (F1 == 3)
				g_prim->m_GI[ec].process = -random(41)-30;
			else
				g_prim->m_GI[ec].process = -random(26);

			if (F1 == 3)
				g_prim->m_GI[ec].max_height = 0.07f;
			else
				g_prim->m_GI[ec].max_height = 0.10f;

			if (F1>=3)
				g_prim->m_GI[ec].alphaB = 125+random(121);
			else if (F1 == 1)
				g_prim->m_GI[ec].alphaB = 125+random(51);
			else
				g_prim->m_GI[ec].alphaB = 125+random(51);
			g_prim->m_GI[ec].rise_angle = random(360);

			if (F1 == 1)
				g_prim->m_GI[ec].distance = 5.0f+random(5);
			else
				g_prim->m_GI[ec].distance = 6.0f+random(6);

			if (F1 == 3)
				g_prim->m_GI[ec].vecT_now.y=4;
			else if (F1 == 1)
				g_prim->m_GI[ec].vecT_now.y=3;
			else
				g_prim->m_GI[ec].vecT_now.y=5;

			g_prim->m_GI[ec].height[0] = 0;

			g_prim->m_GI[ec].vecT_now.x = 0.6f+(random(7)*0.1f);

			g_prim->m_GI[ec].vecB_now = m_pos;


			for (int i=20;i>=1;i--)
			{
				g_prim->m_GI[ec].height[i]=0;
				g_prim->m_GI[ec].flag1[i]=0;
			}

			g_prim->m_GI[ec].alphaT = F1;

			if (F1 == 1)
			{
				for (int j=0;j<10;j++)
				{
					g_prim->m_GI[ec].height[0]+=6;
					if (g_prim->m_GI[ec].height[0]>180)
					{
						g_prim->m_GI[ec].height[0]=180;
					}

					for (int i=g_prim->m_GI[ec].vecT_now.y;i>=1;i--)
					{
						g_prim->m_GI[ec].height[i]=g_prim->m_GI[ec].height[i-1];
					}
				}
			}
		}
	}
}

void CRagEffect::Electric(char F1,short F2)
{
	int ec;
	m_pos = m_master->m_pos;

	if (m_stateCnt >= 20 && m_stateCnt < 24)
	{
		g_prim = LaunchEffectPrim(PP_ELECTRIC);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture=1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\elec1.tga", false);

		g_prim->m_size=0;




		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].max_height = 0.1f;

			g_prim->m_GI[ec].alphaB = 0;

			g_prim->m_GI[ec].rise_angle = (ec*45)+F2;

			g_prim->m_GI[ec].distance = 0.0f;

			g_prim->m_GI[ec].height[0] = 0;
			g_prim->m_GI[ec].vecT_now.x = 0.0f+(random(11)*0.01f);

			g_prim->m_GI[ec].vecB_now = m_pos;

			g_prim->m_GI[ec].vecT_now.y=20;

			for (int i=20;i>=1;i--)
			{
				g_prim->m_GI[ec].height[i]=0;
				g_prim->m_GI[ec].flag1[i]=0;
			}
			g_prim->m_GI[ec].flag1[0]=0;

			g_prim->m_GI[ec].alphaT = F1;
		}
	}
}

void CRagEffect::Electric2(char F1,short F2)
{
	int ec;
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_ELECTRIC2);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture=1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\elec1.tga", false);

		g_prim->m_size=0;




		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].max_height = 0.1f;

			g_prim->m_GI[ec].alphaB = 0;

			{
				float Rx,Rz;
				Rx=m_deltaPos.x-m_pos.x;
				Rz=m_deltaPos.z-m_pos.z;

				g_prim->Get_Angle(Rx,Rz,&g_prim->m_GI[ec].rise_angle);
			}

			g_prim->m_GI[ec].distance = 0.0f;

			if (ec<2)
				g_prim->m_GI[ec].height[0] = ec*180;
			else
				g_prim->m_GI[ec].height[0] = (ec-2)*180;

			g_prim->m_GI[ec].vecT_now.x = 0.2f+(random(6)*0.01f);

			g_prim->m_GI[ec].vecB_now = m_pos;

			g_prim->m_GI[ec].vecT_now.y=20;

			for (int i=20;i>=1;i--)
			{
				g_prim->m_GI[ec].height[i]=0;
			}

			g_prim->m_GI[ec].alphaT = F1;
		}
	}
}

void CRagEffect::HitLine4(char F1)
{
	int ec;
	m_pos = m_master->m_pos;

	if (m_stateCnt >= 6 && m_stateCnt < 10)
	{
		g_prim = LaunchEffectPrim(PP_HITLINE3);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture=1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\white02.bmp", false);

		g_prim->m_size=0;




		for (ec=0;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = -random(31);
			if (F1 == 3)
				g_prim->m_GI[ec].max_height = 0.05f;
			else
				g_prim->m_GI[ec].max_height = 0.10f;


			g_prim->m_GI[ec].alphaB = 125+random(121);
			g_prim->m_GI[ec].rise_angle = random(360);

			g_prim->m_GI[ec].distance = 6.0f+random(6);

			g_prim->m_GI[ec].height[0] = 0;
			g_prim->m_GI[ec].vecT_now.x = 0.8f+(random(7)*0.1f);

			g_prim->m_GI[ec].vecB_now = m_pos;

			g_prim->m_GI[ec].vecT_now.y=4;

			for (int i=20;i>=1;i--)
			{
				g_prim->m_GI[ec].height[i]=0;
				g_prim->m_GI[ec].flag1[i]=0;
			}

			g_prim->m_GI[ec].alphaT = F1;
		}
	}
}

void CRagEffect::HitLine5(char F1)
{
	int ec;
	m_pos = m_master->m_pos;

	if (m_stateCnt >= 0 && m_stateCnt < 3)
	{
		g_prim = LaunchEffectPrim(PP_HITLINE3);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture=1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\white02.bmp", false);

		g_prim->m_size=0;

		g_prim->m_deltaPos = m_deltaPos;
		g_prim->m_pos = m_pos;




		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			if (F1 == 3)
			{
				g_prim->m_GI[ec].process = -random(21)-25;
				g_prim->m_GI[ec].max_height = 0.06f;
			}
			else
			{
				g_prim->m_GI[ec].process = -random(26)-15;
				g_prim->m_GI[ec].max_height = 0.07f;
			}

			g_prim->m_GI[ec].alphaB = 125+random(121);

			{
				float Rx,Rz;
				Rx=m_pos.x-m_deltaPos.x;
				Rz=m_pos.z-m_deltaPos.z;

				g_prim->Get_Angle(Rx,Rz,&g_prim->m_GI[ec].rise_angle);

				g_prim->m_GI[ec].rise_angle+=random(121)-60;
				if (g_prim->m_GI[ec].rise_angle>=360)
						g_prim->m_GI[ec].rise_angle-=360;
				if (g_prim->m_GI[ec].rise_angle<0)
						g_prim->m_GI[ec].rise_angle+=360;
			}

			g_prim->m_GI[ec].distance = 6.0f+random(7);

			g_prim->m_GI[ec].height[0] = 0;
			g_prim->m_GI[ec].vecT_now.x = 0.8f+(random(7)*0.1f);

			g_prim->m_GI[ec].vecB_now = m_pos;

			g_prim->m_GI[ec].vecT_now.y=3;

			for (int i=20;i>=1;i--)
			{
				g_prim->m_GI[ec].height[i]=0;
				g_prim->m_GI[ec].flag1[i]=0;
			}

			g_prim->m_GI[ec].alphaT = F1;
		}
	}
}

void	CRagEffect :: Sakura()
{

	m_pos = m_master->m_pos;

	if (m_stateCnt == 99999998)
	{
		m_stateCnt = 0;
		m_duration = 99999999;
	}

	if ( m_stateCnt%2 == 0 && m_stateCnt < 300 )
	{
		g_prim = LaunchEffectPrim(PP_SAKURA);

		g_prim->m_duration = m_duration;




		int ec;

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].max_height=(random(2)+2)*0.1f;
			g_prim->m_size = (float)(random(6) + 22) / 100;


			g_prim->m_GI[ec].height[0]=random(360);
			g_prim->m_GI[ec].height[1]=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[3]=random(360);

			g_prim->m_GI[ec].vecB_pre.x=m_pos.x+random(300)-150;
			g_prim->m_GI[ec].vecB_pre.y=m_pos.y-200;
			g_prim->m_GI[ec].vecB_pre.z=m_pos.z+random(300)-150;
			g_prim->m_GI[ec].flag1[0]=0;
		}

		g_prim->m_alpha = 200;
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\sakura01.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\sakura01.act");
		g_prim->SetAction(random(3), 36, MT_LOOP);
		g_prim->InitSegment();

		for (ec=1;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;
	}
}

void	CRagEffect :: Kouenka()
{

	m_pos = m_master->m_pos;

	if (m_stateCnt >= 12 && m_stateCnt <= 15)
	{
		char buf[50];
		sprintf(buf, "effect\\EF_FireArrow%d.wav", random(3) + 1);
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave(buf, m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if ( m_stateCnt < 20 )
	{
		for (int i=0;i<1;i++)
		{
			g_prim = LaunchEffectPrim(PP_KOUENKA);

			g_prim->m_duration = m_duration;

			int ec;

			for (ec=0;ec<1;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].max_height=(random(3)+2)*0.15f;

				g_prim->m_size = (float)(random(6) + 22) / 70;
				g_prim->m_GI[ec].vecB_pre.y=m_pos.y;

				short ta;
				ta=random(360);

				g_prim->m_GI[ec].vecB_pre.x=m_pos.x+(8*g_cosTable[ta]);
				g_prim->m_GI[ec].vecB_pre.z=m_pos.z+(8*g_sinTable[ta]);

				g_prim->m_GI[ec].height[0]=random(360);
				g_prim->m_GI[ec].height[1]=random(360);
				g_prim->m_GI[ec].height[2]=random(360);
				g_prim->m_GI[ec].height[3]=random(360);
			}

			for (ec=1;ec<4;ec++)
				g_prim->m_GI[ec].life = 0;

			g_prim->m_alpha = 250;
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\sakura01.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\sakura01.act");
			g_prim->SetAction(random(3), 36, MT_LOOP);
			g_prim->InitSegment();
		}

	}
}

void	CRagEffect :: Maple()
{

	m_pos = m_master->m_pos;

	if (m_stateCnt == 99999998)
	{
		m_duration = 99999999;
		m_stateCnt = 0;
	}

	if ( m_stateCnt%2 == 0 && m_stateCnt < 200 )
	{
		g_prim = LaunchEffectPrim(PP_SAKURA);

		g_prim->m_duration = m_duration;




		int ec;

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].max_height=(random(4)+2)*0.03f;
			g_prim->m_size = (float)(random(6) + 35) / 100;

			g_prim->m_GI[ec].vecB_pre.x=m_pos.x+random(300)-150;
			g_prim->m_GI[ec].height[0]=random(360);
			g_prim->m_GI[ec].height[1]=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[3]=random(360);
			g_prim->m_GI[ec].vecB_pre.y=m_pos.y-200;
			g_prim->m_GI[ec].vecB_pre.z=m_pos.z+random(300)-150;
			g_prim->m_GI[ec].flag1[0]=1;
		}

		g_prim->m_alpha = 200;

		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb4\xdc\xc7\xb3.act");
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb4\xdc\xc7\xb3.spr");

		g_prim->SetAction(0, 36, MT_LOOP);

		g_prim->InitSegment();

		for (ec=1;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;
	}
}

void	CRagEffect :: Ghost(char F1)
{

	m_pos = m_master->m_pos;

	if ( m_stateCnt < 10 )
	{
		g_prim = LaunchEffectPrim(PP_GHOST);

		g_prim->m_duration = m_duration;




		int ec;

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].life = 1;
			g_prim->m_size = 1.5f;
			g_prim->m_GI[ec].vecB_now.x=m_pos.x;
			g_prim->m_GI[ec].vecB_now.y=m_pos.y;
			g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			g_prim->m_GI[ec].RotStart=0;
			g_prim->m_GI[ec].rise_angle=0;
			if (F1 == 2)
			{
				g_prim->m_GI[ec].flag1[0]=2;
				if (m_stateCnt == 0)
					g_prim->m_GI[ec].height[1]=250;
				else
					g_prim->m_GI[ec].height[1]=30-(m_stateCnt*3);
			}
			else
			{
				g_prim->m_GI[ec].flag1[0]=1;
				g_prim->m_GI[ec].height[1]=30-(m_stateCnt*3)  ;
			}

			g_prim->m_GI[ec].flag1[2]=F1;
			g_prim->m_GI[ec].distance=16.0f;
			g_prim->m_GI[ec].max_height=4.0f;
		}

		g_prim->m_alpha = 0;

		if (F1 == 0)
		{

			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc0\xaf\xb7\xc9.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc0\xaf\xb7\xc9.act");

		}
		else if (F1 >= 1)
		{

			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb9\xda\xc1\xe3.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb9\xda\xc1\xe3.act");
		}

		g_prim->m_animSpeed=2;
		g_prim->m_renderFlag |= RF_NODEPTHCHECK;

		g_prim->SetAction(0, 16, MT_ONETIME);

		g_prim->InitSegment();

		for (ec=1;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;
	}
}

void	CRagEffect :: EnergyDrain2()
{
	if (m_stateCnt == 0) {
		for(int a = 0; a < 3; a++) {
			int i = (-90 + (a * 90));
			g_prim = LaunchEffectPrim(PP_3DPARTICLESPLINE);
			g_prim->m_duration = 60;


			g_prim->m_deltaPos2.Set(m_deltaPos.x-m_pos.x, -10, m_deltaPos.z-m_pos.z);
			g_prim->m_radius = g_prim->CalcDist(m_deltaPos.x - ((m_deltaPos.x-m_pos.x)*0.5f), m_deltaPos.z - ((m_deltaPos.z-m_pos.z)*0.5f));
			g_prim->CalcDir( (2*m_pos.x) - m_deltaPos.x , (2*m_pos.z) - m_deltaPos.z );

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
			g_prim->m_red = 160 + random(81);
			g_prim->m_green = 160 + random(81);
			g_prim->m_blue = 255;
			g_prim->m_size = 1.5f + (random(6)*0.1f);
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
			g_prim->m_numSegment = 7;
			g_prim->InitSegment();
		}
	}
}

void	CRagEffect :: EnergyDrain3()
{
	if (m_stateCnt <= 4)
	{
		{
			for(int a = 0; a < 3; a++)
			{
				int i = (-90 + (a * 90));
				g_prim = LaunchEffectPrim(PP_3DPARTICLESPLINE);
				g_prim->m_duration = 60;


				g_prim->m_deltaPos2.Set(0, -10, 0);
				g_prim->m_radius = g_prim->CalcDist(m_pos.x, m_pos.z);
				g_prim->CalcDir( m_pos.x+random(7)-3 , m_pos.z+random(7)-3 );

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
				g_prim->m_red = 160 + random(81);
				g_prim->m_green = 255;
				g_prim->m_blue = 160 + random(81);
				g_prim->m_size = 1.0f + (random(6)*0.1f);
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\particle1.act");
				g_prim->m_numSegment = 7;
				g_prim->InitSegment();
			}
		}
	}
}

void	CRagEffect :: SuperAngel(char F1, char time)
{

	m_pos = m_master->m_pos;

	if ( (m_stateCnt == 0) || (m_stateCnt <=18 && (F1 == 1 || F1 == 11) && m_stateCnt%3==0) )
	{
		g_prim = LaunchEffectPrim(PP_SUPERANGEL);

		g_prim->m_duration = m_duration;



		int ec;

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].life = 1;
			g_prim->m_size = 1.0f;
			g_prim->m_GI[ec].vecB_now.x=m_pos.x;
			g_prim->m_GI[ec].vecB_now.y=m_pos.y - 40.0f;
			g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			g_prim->m_GI[ec].flag1[0]=F1;
			g_prim->m_GI[ec].flag1[1]=m_stateCnt;
		}

		g_prim->m_alpha = 0;

		if (F1 == 0)
		{

			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc3\xb5\xbb\xe7.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc3\xb5\xbb\xe7.act");

			g_prim->m_renderFlag = RF_EFFECT;

		}
		else if (F1 == 1)
		{

			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc3\xb5\xbb\xe7\xb3\xaf\xb0\xb3.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc3\xb5\xbb\xe7\xb3\xaf\xb0\xb3.act");

			g_prim->m_renderFlag = RF_EFFECT;
		}
		else if (F1 == 2)
		{


			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb1\xea\xc5\xd0" "1.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb1\xea\xc5\xd0" "1.act");

			g_prim->m_renderFlag = RF_EFFECT;


		}
		else if (F1 == 3)
		{


			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb1\xea\xc5\xd0" "2.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb1\xea\xc5\xd0" "2.act");

			g_prim->m_renderFlag = RF_EFFECT;

		}
		else if (F1 == 4)
		{


			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb1\xea\xc5\xd0" "3.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb1\xea\xc5\xd0" "3.act");

			g_prim->m_renderFlag = RF_EFFECT;
		}
		else if (F1 == 10)
		{

			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc7\xd1\xba\xb9\xc3\xb5\xbb\xe7.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc7\xd1\xba\xb9\xc3\xb5\xbb\xe7.act");

			g_prim->m_renderFlag = RF_ALPHA;
			g_prim->m_GI[0].vecB_now.y=m_pos.y - 30.0f;
			g_prim->m_renderFlag |= RF_NODEPTHCHECK;
		}
		else if (F1 == 11)
		{

			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc7\xd1\xba\xb9\xc3\xb5\xbb\xe7(\xb3\xaf\xb0\xb3).spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc7\xd1\xba\xb9\xc3\xb5\xbb\xe7(\xb3\xaf\xb0\xb3).act");

			g_prim->m_renderFlag = RF_EFFECT;
			g_prim->m_GI[0].vecB_now.y=m_pos.y - 30.0f;
		}

		g_prim->SetAction(0, 16, MT_ONETIME);

		g_prim->InitSegment();

		g_prim->m_repeatAnim = true;
		g_prim->m_curAction = 0;

		for (ec=1;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;
	}

	if (F1 == 0 || F1 == 10)
	{
		if (m_stateCnt == 65)
		{
			g_prim = LaunchEffectPrim(PP_SLASH1);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);


			g_prim->m_size=4;



			int ec;

			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process = 0;

				g_prim->m_GI[ec].max_height = 2.0f;
				g_prim->m_GI[ec].RotStart = 0+(ec*90);
				g_prim->m_GI[ec].alphaB = 0;
				g_prim->m_GI[ec].distance = 2.0f;
				g_prim->m_GI[ec].height[0] = 1.0f;

				g_prim->m_GI[ec].flag1[0] = 0;
			}
		}

		if (m_stateCnt == 66)
		{
			g_prim = LaunchEffectPrim(PP_SLASH1);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);

			g_prim->m_size=4;



			int ec;

			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process = 0;

				g_prim->m_GI[ec].alphaB = 0;
				g_prim->m_GI[ec].distance = 2.0f;
				g_prim->m_GI[ec].max_height = 2.0f;
				g_prim->m_GI[ec].RotStart = 45+(ec*90);
				g_prim->m_GI[ec].height[0] = 1.0f;

				g_prim->m_GI[ec].flag1[0] = 0;
			}
		}
	}

}

bool	CEffectPrim :: PrimSakura()
{


	int ec;
	m_pos = m_master->m_pos;

	for (ec=0;ec<1;ec++)
	{
		int i;
		if (m_GI[ec].life == 1)
		{
			for (i=0;i<4;i+=2)
			{
				if (m_GI[ec].height[i+1] > m_GI[ec].height[i])
				{
					m_GI[ec].height[i]+=3;
					if (m_GI[ec].height[i] > 359)
						m_GI[ec].height[i] = 359;
					if (m_GI[ec].height[i] > m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = random(int(m_GI[ec].height[i]));
				}
				else
				{
					m_GI[ec].height[i]-=2;
					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
					if (m_GI[ec].height[i] <= m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = 359 - random(int(m_GI[ec].height[i]));
				}
			}

			{
				m_GI[ec].vecB_pre.y += m_GI[ec].max_height;

				if (m_GI[ec].flag1[0] == 0)
				{
					m_GI[ec].vecB_pre.x += 0.24f * g_sinTable[int(m_GI[ec].height[0])];
					m_GI[ec].vecB_pre.z += 0.30f * g_sinTable[int(m_GI[ec].height[2])];
				}
				else
				{
					m_GI[ec].vecB_pre.x += 0.12f * g_sinTable[int(m_GI[ec].height[0])];
					m_GI[ec].vecB_pre.z += 0.15f * g_sinTable[int(m_GI[ec].height[2])];
				}

				if (m_GI[ec].vecB_pre.y>m_pos.y)
				{
					m_GI[ec].vecB_pre.x=m_pos.x+random(300)-150;
					m_GI[ec].vecB_pre.y=m_pos.y-200;
					m_GI[ec].vecB_pre.z=m_pos.z+random(300)-150;
				}
			}

			m_deltaPos=m_GI[ec].vecB_pre;
		}
	}

	m_pos = m_deltaPos;


	if (m_pattern & PT_ROTATESPR) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		float longitude = gameMode->GetView()->GetCurLongitude();
		m_roll = m_roty - longitude;
	}

	ProcessAlpha();
	ShiftSegment();
	if (!AnimMotion()) return false;
	return true;
}

bool	CEffectPrim :: PrimKouenka()
{


	int ec;
	m_pos = m_master->m_pos;

	for (ec=0;ec<1;ec++)
	{
		int i;
		if (m_GI[ec].life == 1)
		{
			for (i=0;i<4;i+=2)
			{
				if (m_GI[ec].height[i+1] > m_GI[ec].height[i])
				{
					m_GI[ec].height[i]+=3;
					if (m_GI[ec].height[i] > 359)
						m_GI[ec].height[i] = 359;
					if (m_GI[ec].height[i] > m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = random(int(m_GI[ec].height[i]));
				}
				else
				{
					m_GI[ec].height[i]-=2;
					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
					if (m_GI[ec].height[i] <= m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = 359 - random(int(m_GI[ec].height[i]));
				}
			}

			{
				m_GI[ec].vecB_pre.y -= m_GI[ec].max_height;
				m_alpha-=3;
				if (m_alpha < 3)
					m_alpha = 0;

				m_GI[ec].vecB_pre.x += 0.15f * g_sinTable[int(m_GI[ec].height[0])];
				m_GI[ec].vecB_pre.z += 0.15f * g_sinTable[int(m_GI[ec].height[2])];
			}

			m_deltaPos=m_GI[ec].vecB_pre;
		}
	}

	m_pos = m_deltaPos;


	if (m_pattern & PT_ROTATESPR) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		float longitude = gameMode->GetView()->GetCurLongitude();
		m_roll = m_roty - longitude;
	}

	ProcessAlpha();
	ShiftSegment();
	if (!AnimMotion()) return false;
	return true;
}

bool	CEffectPrim :: PrimChimTo()
{


	int ec;
	m_pos = m_master->m_pos;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_deltaPos=m_GI[ec].vecB_pre;
		}
	}

	m_pos = m_deltaPos;


	if (m_pattern & PT_ROTATESPR) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		float longitude = gameMode->GetView()->GetCurLongitude();
		m_roll = m_roty - longitude;
	}

	ProcessAlpha();
	ShiftSegment();
	if (!AnimMotion()) return false;
	return true;
}

bool	CEffectPrim :: PrimBottomSpr()
{


	int ec;
	m_pos = m_master->m_pos;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].flag1[4] == 1)
				m_pos = m_master->m_pos;
			else
				m_pos=m_GI[ec].vecB_pre;
		}
	}


	if (m_pattern & PT_ROTATESPR) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		float longitude = gameMode->GetView()->GetCurLongitude();
		m_roll = m_roty - longitude;
	}

	ProcessAlpha();
	ShiftSegment();
	if (!AnimMotion()) return false;
	return true;
}

bool	CEffectPrim :: PrimGhost()
{
	int angle;
	int ec;
	m_pos = m_master->m_pos;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process<=250)
			{
				if (m_GI[ec].height[1]>m_alpha)
					m_alpha+=1;
			}

			if (m_GI[ec].process>=2600)
			{
				int a;
				a=360;
				a/=m_GI[ec].flag1[0];

				if (m_GI[ec].flag1[2] <= 1)
				{
					if ( m_GI[ec].process%a == 0 )
						m_GI[ec].flag1[0] ++;

					m_GI[ec].distance *= 1.002f;
				}

				if ( m_GI[ec].process%15 == 0 )
				{
					if (m_GI[ec].flag1[2] == 2)
						m_alpha-=5;
					else
						m_alpha-=1;

					if (m_alpha<0)
						m_alpha=0;
				}
			}

			m_GI[ec].rise_angle+=3;
			if (m_GI[ec].rise_angle>=360)
				m_GI[ec].rise_angle-=360;

			m_GI[ec].RotStart+=m_GI[ec].flag1[0];
			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			m_GI[ec].vecB_pre = m_GI[ec].vecB_now;
			if (m_GI[ec].flag1[2] == 2)
			{
				int angle;
				angle = m_GI[ec].RotStart*2;
				if (angle>=360)
					angle-=360;
				m_GI[ec].vecB_pre.x += m_GI[ec].distance * g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_pre.z += m_GI[ec].distance * g_sinTable[angle];
			}
			else
			{
				m_GI[ec].vecB_pre.x += m_GI[ec].distance * g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_pre.z += m_GI[ec].distance * g_sinTable[m_GI[ec].RotStart];
			}
			m_GI[ec].vecB_pre.y += m_GI[ec].max_height * g_sinTable[m_GI[ec].rise_angle];;

			m_deltaPos=m_GI[ec].vecB_pre;

			CGameMode * gameMode = g_modeMgr.GetGameMode();
			float longitude = gameMode->GetView()->GetCurLongitude();
			int a2;
			a2=longitude;

			angle = m_GI[ec].RotStart;
			angle=angle+a2;
			if (angle<0)
				angle+=360;
			if (angle>=360)
				angle-=360;

			if (m_GI[ec].flag1[2]==0)
			{
				if ( angle>=180 && angle<270 )
					m_curAction = 6;
				else if ( angle>=90 && angle<180 )
					m_curAction = 0;
				else if ( angle>=0 && angle<90)
					m_curAction = 0;
				else
					m_curAction = 6;
			}
			else
			{
				if (random(999)==1)
					m_curAction = 1;
				if (random(1001)==0)
					m_curAction = 0;
			}
		}
	}

	m_pos = m_deltaPos;

	ProcessAlpha();
	ShiftSegment();
	if (!AnimMotion()) return false;
	return true;
}

bool	CEffectPrim :: PrimSuperAngel()
{


	int ec;
	m_pos = m_master->m_pos;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			m_GI[ec].vecB_now.y-=0.1f;

			m_GI[ec].vecB_pre = m_GI[ec].vecB_now;
			m_deltaPos=m_GI[ec].vecB_pre;

			if (m_GI[ec].process<50)
			{
				if (m_GI[ec].flag1[1]==0)
				{
					if (m_GI[ec].flag1[0]==1)
					{
						if (m_alpha<200)
							m_alpha += 5;
					}
					else if (m_GI[ec].flag1[0]==10)
					{

						{
							if (m_alpha<255)
								m_alpha += 10;

							if (m_alpha>255)
								m_alpha=255;
						}
					}
					else if (m_GI[ec].flag1[0]==11)
						m_alpha=0;
					else if (m_GI[ec].flag1[0]>=2)
					{
						if (m_GI[ec].process<30)
						{
							if (m_alpha<150)
								m_alpha += 5;
						}
					}
					else
					{
						if (m_alpha<245)
							m_alpha += 5;
					}
				}
				else
				{
					if (m_alpha<(40-(m_GI[ec].flag1[1])))
						m_alpha += 1;
				}
			}

			if (m_GI[ec].process>30)
			{
				if (m_GI[ec].flag1[0] == 0)
				{
					if (m_GI[ec].process%3==0)
						m_curMotion++;
					if (m_curMotion>29)
						m_curMotion=29;
				}
				else if (m_GI[ec].flag1[0] == 1)
				{
					if (m_GI[ec].process%3==0)
						m_curMotion++;
					if (m_curMotion>29)
						m_curMotion=29;
				}
				else if (m_GI[ec].flag1[0] == 10 || m_GI[ec].flag1[0] == 11)
				{
					if (m_GI[ec].process%4==0)
						m_curMotion++;
					if (m_curMotion>15)
						m_curMotion=15;
				}
				else if (m_GI[ec].flag1[0] >= 2)
				{
					if (m_GI[ec].process%3==0)
						m_curMotion++;

					if (m_curMotion>29)
						m_curMotion=29;
				}
			}



			if (m_GI[ec].flag1[0] == 0)
			{
				if (m_GI[ec].process>100)
				{
					m_alpha-=4;
					if (m_alpha<0)
						m_alpha=0;
				}
			}
			else if (m_GI[ec].flag1[0] == 2)
			{
				if (m_GI[ec].process>30)
				{
					m_alpha-=3;
					if (m_alpha<0)
						m_alpha=0;
				}
			}
			else if (m_GI[ec].flag1[0] == 3)
			{
				if (m_GI[ec].process>60)
				{
					m_alpha-=3;
					if (m_alpha<0)
						m_alpha=0;
				}
			}
			else if (m_GI[ec].flag1[0] == 4)
			{
				if (m_GI[ec].process>70)
				{
					m_alpha-=3;
					if (m_alpha<0)
						m_alpha=0;
				}
			}
			else if (m_GI[ec].flag1[0] == 10)
			{
				if (m_GI[ec].process>100)
				{
					m_alpha-=6;
					if (m_alpha<0)
						m_alpha=0;
				}
			}
			else
			{
				if (m_GI[ec].flag1[1]!=0)
				{
					if (m_GI[ec].process>75)
					{
						m_alpha-=3;
						if (m_alpha<0)
							m_alpha=0;
					}
				}
				else
				{
					if (m_GI[ec].process>75)
					{
						m_alpha-=6;
						if (m_alpha<0)
							m_alpha=0;
					}
				}
			}
		}
	}

	m_pos = m_deltaPos;


	ShiftSegment();

	return true;
}

void	CRagEffect :: Cloud( char map )
{
	CWorld* world = g_modeMgr.GetGameMode()->GetWorld();
	vector3d & pPos = world->m_player->m_pos;

	m_pos = pPos;

	if (m_stateCnt == 99999998)
	{
		m_stateCnt = 0;
		m_duration = 99999999;
	}

	if (m_stateCnt == 0)
	{

		char ra;
		g_prim = LaunchEffectPrim(PP_CLOUD);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 3;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		ra=random(3);

		if (map == 3)
		{
			if (ra == 0)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\fog1.tga", false);
			else if (ra == 1)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\fog2.tga", false);
			else
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\fog3.tga", false);
		}
		else
		{
			if (ra == 0)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\cloud4.tga", false);
			else if (ra == 1)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\cloud1.tga", false);
			else
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\cloud2.tga", false);
		}

		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;
			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].height[7] = map;
			g_prim->m_GI[ec].full_display_angle=45;

			if ( map == 0 )
			{
				g_prim->m_GI[ec].height[6] = -152.0f;
				if ( m_pos.y > g_prim->m_GI[ec].height[6] )
					g_prim->m_GI[ec].process=300;
			}

			g_prim->m_GI[ec].alphaB=0;

			g_prim->m_GI[ec].flag1[0]=0;
			g_prim->m_GI[ec].RotStart=random(200)+300;




			if ( map == 0 )
			{
				g_prim->m_GI[ec].vecB_now.y=-125.0f+random(10);
				g_prim->m_GI[ec].vecB_now.x=random(300)-150;
				g_prim->m_GI[ec].vecB_now.z=random(300)-150;
			}
			else if ( map == 2 )
			{
				g_prim->m_GI[ec].vecB_now.y=0.0f+random(10);
				g_prim->m_GI[ec].vecB_now.x=random(300)-150;
				g_prim->m_GI[ec].vecB_now.z=random(300)-150;
			}
			else if ( map == 3 )
			{
				g_prim->m_GI[ec].vecB_now.x=random(300)-150;
				g_prim->m_GI[ec].vecB_now.z=random(300)-150;

				CWorld *	world = g_modeMgr.GetGameMode()->GetWorld();
				g_prim->m_GI[ec].vecB_now.y = world->m_attr->GetHeight(m_pos.x+g_prim->m_GI[ec].vecB_now.x, m_pos.z+g_prim->m_GI[ec].vecB_now.z) - 20 - random(5);
			}
			else if ( map == 5 )
			{

				g_prim->m_GI[ec].vecB_now.y=20.0f+random(10);
				g_prim->m_GI[ec].vecB_now.x=random(300)-150;
				g_prim->m_GI[ec].vecB_now.z=random(300)-150;
			}
			else
			{
				g_prim->m_GI[ec].vecB_now.y=40.0f+random(10);

				g_prim->m_GI[ec].vecB_now.x=random(200)+25;
				if (random(2)==0)
					g_prim->m_GI[ec].vecB_now.x*=-1;

				g_prim->m_GI[ec].vecB_now.z=random(200)+25;
				if (random(2)==0)
					g_prim->m_GI[ec].vecB_now.z*=-1;
			}

			g_prim->m_GI[ec].vecB_now.x+=m_pos.x;
			g_prim->m_GI[ec].vecB_now.z+=m_pos.z;

			g_prim->m_GI[ec].height[0]=random(360);
			g_prim->m_GI[ec].height[1]=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[3]=random(360);

			if ( map == 3 )
				g_prim->m_GI[ec].distance = 35.0f+random(10);
			else
				g_prim->m_GI[ec].distance = 30.0f+random(20);

			g_prim->m_GI[ec].height[8]=random(360);

			if ( map == 3 )
				g_prim->m_GI[ec].flag1[2] = 12;
			else if ( map == 5 )
				g_prim->m_GI[ec].flag1[2] = 14;
			else if ( map == 7 )
				g_prim->m_GI[ec].flag1[2] = 16;
			else if ( map == 8 )
				g_prim->m_GI[ec].flag1[2] = 17;
			else
				g_prim->m_GI[ec].flag1[2] = 0;
		}
	}
}

void	CRagEffect :: Twilight( char F1 )
{
	CWorld* world = g_modeMgr.GetGameMode()->GetWorld();
	vector3d & pPos = world->m_player->m_pos;

	m_pos = pPos;

	if (m_stateCnt == 0)
	{

		char ra;
		g_prim = LaunchEffectPrim(PP_TWILIGHT);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		if (F1 == 0)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xc7\xcf\xbe\xe1\xc7\xe3\xba\xea.bmp", false);

		}
		else if (F1 == 1)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xc8\xad\xc0\xcc\xc6\xae\xbd\xbd\xb8\xb2\xc6\xf7\xbc\xc7.bmp", false);
		}
		else if (F1 == 2)
		{
			ra=random(3);

			if (ra==0)
				g_prim->m_texture[0] = g_texMgr.GetTexture("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xc8\xad\xbf\xb0\xba\xb4.bmp", false);
			else if (ra==1)
				g_prim->m_texture[0] = g_texMgr.GetTexture("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xbf\xb0\xbb\xea\xba\xb4.bmp", false);
			else
				g_prim->m_texture[0] = g_texMgr.GetTexture("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\item\\\xbe\xcb\xc4\xda\xbf\xc3.bmp", false);
		}

		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;
			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].height[7] = 3;
			g_prim->m_GI[ec].full_display_angle=random(360);

			g_prim->m_GI[ec].alphaB=0;
			g_prim->m_GI[ec].flag1[0]=0;
			g_prim->m_GI[ec].RotStart=m_duration-50;

			{
				g_prim->m_GI[ec].vecB_now.x=random(160)-80;
				g_prim->m_GI[ec].vecB_now.z=random(160)-80;

				CWorld *	world = g_modeMgr.GetGameMode()->GetWorld();
				g_prim->m_GI[ec].vecB_now.y = world->m_attr->GetHeight(m_pos.x+g_prim->m_GI[ec].vecB_now.x, m_pos.z+g_prim->m_GI[ec].vecB_now.z) - 5 - random(66);
			}

			g_prim->m_GI[ec].vecB_now.x+=m_pos.x;
			g_prim->m_GI[ec].vecB_now.z+=m_pos.z;

			g_prim->m_GI[ec].height[0]=random(360);
			g_prim->m_GI[ec].height[1]=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[3]=random(360);

			g_prim->m_GI[ec].distance = 3.0f+(random(11)*0.1f);

			g_prim->m_GI[ec].height[8]=random(360);

			g_prim->m_GI[ec].flag1[2] = 7;
		}
	}
}

void	CRagEffect :: PokJuk()
{
	CWorld* world = g_modeMgr.GetGameMode()->GetWorld();
	vector3d & pPos = world->m_player->m_pos;
	m_pos = pPos;

	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_POKJUK);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 3;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\pok1.tga", false);
		g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\pok3.tga", false);
		g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\pok2.tga", false);

		m_pos.x-=15.0f;
		for (int ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = -300-random(50);

			g_prim->m_GI[ec].RotStart = random(360);

			g_prim->m_GI[ec].vecB_now = m_pos;
			g_prim->m_GI[ec].vecT_pre = m_pos;
			g_prim->m_GI[ec].vecT_now = m_pos;
			g_prim->m_GI[ec].vecB_pre = m_pos;
			g_prim->m_GI[ec].alphaB=0;
			g_prim->m_GI[ec].max_height=2.0f;

			g_prim->m_GI[ec].flag1[0] = random(5);
			g_prim->m_GI[ec].flag1[1] = 0;
			g_prim->m_GI[ec].flag1[3] = 0;
			g_prim->m_GI[ec].flag1[2] = random(4)*20;

			g_prim->m_GI[ec].distance=0.4f+(random(100)*0.004f);
			g_prim->m_GI[ec].full_display_angle=random(360);
			g_prim->m_GI[ec].rise_angle=random(360);
		}
	}
}

void	CRagEffect :: PokJuk_Sound()
{
	if (m_stateCnt == 300)
	{

		PlayWave("effect\\\xc6\xf8\xc1\xd7.wav");

	}

	if (m_stateCnt > 2300 )
	{
		m_stateCnt = 299;
	}
}

void	CRagEffect :: MAPPILLAR( char *tName , char PW )
{
	if (m_stateCnt == 0)
	{
		m_pos = m_master->m_pos;

		g_prim = LaunchEffectPrim(PP_MAPPILLAR);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		if (PW <=1 )
			g_prim->m_size=4;
		else if (PW == 2)
			g_prim->m_size=7;
		else if (PW == 3)
			g_prim->m_size=6;



		int ec,i;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = ec*30;
			g_prim->m_GI[ec].full_display_angle = 360;
			g_prim->m_GI[ec].max_height = 120.0f;
			g_prim->m_GI[ec].RotStart = ec*90;
			g_prim->m_GI[ec].alphaB = 50;
			g_prim->m_GI[ec].vecB_now.y=0;
			if ( PW == 0 || PW > 1 )
			{
				g_prim->m_GI[ec].alphaB = 50;
				g_prim->m_GI[ec].distance = 2.0f+(ec*0.5f);
			}
			else
			{
				g_prim->m_GI[ec].alphaB = 70;
				g_prim->m_GI[ec].distance = 11.0f+(ec*0.5f);
			}

			g_prim->m_GI[ec].alphaT = 0;
			g_prim->m_GI[ec].rise_angle=89;

			for (i = 0 ; i<E_DIVISION ; i++)
			{
				g_prim->m_GI[ec].height[i] = 0.0f;
				g_prim->m_GI[ec].flag1[i] = 0;
			}
		}

	}
}

bool	CRagEffect :: ShieldCharge()
{
	if (!ProcessEZ2STR(0, 0, 0))
		return false;
	return true;
}

bool	CRagEffect :: JobLvUp50(int time)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 5)
	{
		g_prim = LaunchEffectPrim(PP_SLASH1);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\magic_green.tga", false);

		g_prim->m_size=9;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;

			g_prim->m_GI[ec].max_height = 2.0f;
			g_prim->m_GI[ec].RotStart = time+(ec*90);
			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].distance = 2.0f;
			g_prim->m_GI[ec].height[0] = 1.0f;

			g_prim->m_GI[ec].flag1[0] = 0;
		}
	}

	if  (time == 0)
	{
		if (!ProcessEZ2STR(0, 0, 0))
			return false;
	}

	return true;
}

bool	CRagEffect :: JobLvUp50_2(int time)
{
	m_pos = m_master->m_pos;

	{
		g_prim = LaunchEffectPrim(PP_SLASH1);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\magic_blue.tga", false);

		g_prim->m_size=4;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;

			g_prim->m_GI[ec].max_height = 1.5f;
			g_prim->m_GI[ec].RotStart = time+(ec*90);
			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].distance = 2.0f;
			g_prim->m_GI[ec].height[0] = 1.0f;

			g_prim->m_GI[ec].flag1[0] = 0;
		}
	}

	return true;
}

bool	CRagEffect :: RunStop(int time)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_SLASH1);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\magic_blue.tga", false);

		g_prim->m_size=4;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;

			g_prim->m_GI[ec].distance = 2.0f;
			g_prim->m_GI[ec].height[0] = 1.0f;
			g_prim->m_GI[ec].max_height = 0.0f;
			g_prim->m_GI[ec].RotStart = time+(ec*90);
			g_prim->m_GI[ec].alphaB = 0;

			g_prim->m_GI[ec].flag1[0] = 1;
		}
	}

	return true;
}

void	CRagEffect :: ColorPaper(char *tName)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

		g_prim = LaunchEffectPrim(PP_COLORPAPER);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);



		int ec;


		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;

			g_prim->m_GI[ec].max_height = (random(31)*0.01f)+0.3f;
			g_prim->m_GI[ec].RotStart=random(360);
			g_prim->m_GI[ec].rise_angle=ec*45;
			g_prim->m_GI[ec].alphaB=0;
			g_prim->m_GI[ec].height[0] = random(360);
			g_prim->m_GI[ec].height[1] = random(360);
			g_prim->m_GI[ec].height[2] = random(360);
			g_prim->m_GI[ec].vecB_now.x=random(101)-50;
			g_prim->m_GI[ec].vecB_now.y=-random(50)-110;
			g_prim->m_GI[ec].vecB_now.z=random(101)-50;
			g_prim->m_GI[ec].distance=(random(26)*0.01f)+0.25f;
			g_prim->m_GI[ec].full_display_angle=0;
		}
	}
}

void	CRagEffect :: Providence(char *tName, char F1)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{

		g_prim = LaunchEffectPrim(PP_MAPPILLAR);


		g_prim->m_renderFlag = RF_EFFECT ;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=5;



		int ec,i;

		if ( F1 == 0 )
		{
			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process = 200;
				g_prim->m_GI[ec].full_display_angle = 360;
				g_prim->m_GI[ec].max_height = 120.0f;
				g_prim->m_GI[ec].RotStart = ec*90;

				g_prim->m_GI[ec].alphaB = 40;
				g_prim->m_GI[ec].distance = 6.0f+(ec*0.5f);

				g_prim->m_GI[ec].rise_angle=83-ec;

				g_prim->m_GI[ec].alphaT = 1;

				g_prim->m_GI[ec].vecB_now.y=0;

				for (i = 0 ; i<E_DIVISION ; i++)
				{
					g_prim->m_GI[ec].height[i] = 0.0f;
					g_prim->m_GI[ec].flag1[i] = 0;
				}
			}
		}
	}
}

void	CRagEffect :: ForestLight(char *tName , char F1)
{
	if (m_stateCnt == 0)
	{
			m_pos = m_master->m_pos;
			g_prim = LaunchEffectPrim(PP_FOREST);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_texture =	 new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

			int ec;
			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life=0;
			}

			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life=1;
				g_prim->m_GI[ec].process=0;
				if ( F1 == 2 )
					g_prim->m_GI[ec].alphaB=30;
				else if ( F1 == 10 )
					g_prim->m_GI[ec].alphaB=25;
				else if ( F1 == 11 )
					g_prim->m_GI[ec].alphaB=0;
				else
					g_prim->m_GI[ec].alphaB=40;
				g_prim->m_GI[ec].distance=0;
				if (ec==3)
				{
					g_prim->m_GI[ec].process=180;
					if (F1 == 1 || F1 == 11)
						g_prim->m_GI[ec].max_height=4.0f;
					else if (F1 == 2)
						g_prim->m_GI[ec].max_height=1.0f;
					else
						g_prim->m_GI[ec].max_height=2.0f;
				}
				else
				{
					if (F1 == 1 || F1 == 11)
						g_prim->m_GI[ec].max_height=4.0f+(ec*2);
					else if (F1 == 2)
						g_prim->m_GI[ec].max_height=1.0f+(ec*0.5f);
					else
						g_prim->m_GI[ec].max_height=2.0f+ec;
				}
				g_prim->m_GI[ec].height[0] = g_prim->m_GI[ec].max_height;
				g_prim->m_GI[ec].RotStart=25*ec;

				g_prim->m_GI[ec].vecT_now=m_pos;



				m_deltaPos = m_pos;
				if (F1 == 10)
				{
					m_deltaPos.y -= 60.0f;
					m_deltaPos.x -= 15.0f;
					m_deltaPos.z -= 15.0f;
				}
				else
				{
					m_deltaPos.y -= 300.0f;
					m_deltaPos.x -= 70.0f;
					m_deltaPos.z -= 70.0f;
				}

				g_prim->m_GI[ec].vecT_pre=m_deltaPos;
				g_prim->m_GI[ec].flag1[0]=F1;
			}
	}


}

void	CRagEffect :: WaterFall(char *tName , char F1 , char F2 , char F3)
{
	if (m_stateCnt == 0)
	{
		m_pos = m_master->m_pos;
		g_prim = LaunchEffectPrim(PP_WATERFALL);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 3;
		g_prim->m_texture =	 new CTexture* [g_prim->m_totalTexture];

		if (F3 == 0)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\waterfall11.tga", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\waterfall12.tga", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\waterfall13.tga", false);
		}
		else if (F3 == 1)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\waterfall31.tga", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\waterfall32.tga", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\waterfall33.tga", false);
		}

		int ec;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;
			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].alphaB=80;
			g_prim->m_GI[ec].flag1[0]=F1;
			if (F2 == 0)
				g_prim->m_GI[ec].distance=36+(ec*1);
			else if (F2 == 1)
				g_prim->m_GI[ec].distance=18+(ec*1);

			for (int i=0;i<20;i++)
				g_prim->m_GI[ec].height[i]=i*40;
			g_prim->m_GI[ec].vecB_now.z = (ec*1.00f) - 1.00f;
			g_prim->m_GI[ec].max_height=80-(ec*13);

			g_prim->m_GI[ec].flag1[1]=0;
		}

	}
}

void	CRagEffect :: BlueFall(char F1 , char F2 , char F3)
{
	if (m_stateCnt == 0)
	{
		m_pos = m_master->m_pos;
		g_prim = LaunchEffectPrim(PP_WATERFALL);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 3;
		g_prim->m_texture =	 new CTexture* [g_prim->m_totalTexture];

		if (F3 == 0)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\waterfall11.tga", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\waterfall12.tga", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\waterfall13.tga", false);
		}
		else if (F3 == 1)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\waterfall31.tga", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\waterfall32.tga", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\waterfall33.tga", false);
		}

		int ec;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;
			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].alphaB=120;
			g_prim->m_GI[ec].flag1[0]=F1;
			g_prim->m_GI[ec].distance=36+(ec*1);

			for (int i=0;i<20;i++)
				g_prim->m_GI[ec].height[i]=i*40;

			g_prim->m_GI[ec].vecB_now.z = (ec*0.25f) - 1.00f;

			if (F2 == 0)
				g_prim->m_GI[ec].max_height=80-(ec*13);


			else if (F2 == 1)
				g_prim->m_GI[ec].max_height=30-(ec*6);

			g_prim->m_GI[ec].flag1[1]=1;
		}

	}
}

void	CRagEffect :: Rainbow(char *tName , short time)
{
	if (m_stateCnt == 0)
	{
		CView * pView = g_modeMgr.GetGameMode()->GetView();

		m_pos = m_master->m_pos;
		g_prim = LaunchEffectPrim(PP_RAINBOW);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 1;
		g_prim->m_texture =	 new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);


		g_prim->m_GI[0].alphaT=pView->GetCurLongitude();
		g_prim->m_GI[0].alphaT=360-g_prim->m_GI[0].alphaT;

		int ec;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=0;
		}

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life=1;
			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].alphaB=0;
			g_prim->m_GI[ec].distance=50.0f;
			g_prim->m_GI[ec].full_display_angle=180;
			g_prim->m_GI[ec].RotStart=0;
			g_prim->m_GI[ec].rise_angle=g_prim->m_GI[0].alphaT;
		}

	}
}

void	CRagEffect :: LightSphere(char *tName)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		m_pos = m_master->m_pos;
		g_prim = LaunchEffectPrim(PP_LIGHTSPHERE);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 2;

		g_prim->m_texture =	 new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		int ec;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;
			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].alphaB=40;

			g_prim->m_GI[ec].distance=1;
			g_prim->m_GI[ec].height[3]=10;
			g_prim->m_GI[ec].rise_angle=random(360);
			g_prim->m_GI[ec].RotStart=random(360);
			g_prim->m_GI[ec].full_display_angle=random(360);
			g_prim->m_GI[ec].max_height=random(2)+1;

			if (random(2) == 0)
			{
				g_prim->m_GI[ec].height[0]=105;
				g_prim->m_GI[ec].height[1]=105;
				g_prim->m_GI[ec].height[2]=255;
			}
			else
			{
				g_prim->m_GI[ec].height[0]=55;
				g_prim->m_GI[ec].height[1]=55;
				g_prim->m_GI[ec].height[2]=225;
			}
		}
	}
}

void	CRagEffect :: WaterFallParticle(char F1,char F2)
{
	char choice;


	if (m_stateCnt  == 0)
	{
		m_pos = m_master->m_pos;
		g_prim = LaunchEffectPrim(PP_WATERFALLPARTICLE);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		if (random(10) > 0)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\freeze_a_small.bmp", false);
			choice=0;
		}
		else
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\waterp1.tga", false);
			choice=1;
		}

		g_prim->m_size=4;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].max_height=0.05f;
			g_prim->m_GI[ec].flag1[11]=F1;
			g_prim->m_GI[ec].flag1[12]=F2;
			if (choice == 1)
				g_prim->m_GI[ec].distance=7.0f;
			else
				g_prim->m_GI[ec].distance=4.5f;



			if (g_prim->m_GI[ec].flag1[12] == 0)
				g_prim->m_GI[ec].vecB_pre.x=random(45)-22;
			else
				g_prim->m_GI[ec].vecB_pre.x=random(23)-11;
			g_prim->m_GI[ec].vecB_pre.y=random(30);
			g_prim->m_GI[ec].vecB_pre.z=random(11)-5;
			g_prim->m_GI[ec].height[0]=random(360);
			g_prim->m_GI[ec].height[1]=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[3]=random(360);
			if (g_prim->m_GI[ec].flag1[11] == 1)
			{
				int temp;
				temp = g_prim->m_GI[ec].vecB_pre.x;
				g_prim->m_GI[ec].vecB_pre.x = g_prim->m_GI[ec].vecB_pre.z;
				g_prim->m_GI[ec].vecB_pre.z = temp;
			}


			if (g_prim->m_GI[ec].flag1[12] == 0)
				g_prim->m_GI[ec].vecB_now.x=random(45)-22;
			else
				g_prim->m_GI[ec].vecB_now.x=random(23)-11;
			g_prim->m_GI[ec].vecB_now.y=random(30);
			g_prim->m_GI[ec].vecB_now.z=random(11)-5;
			g_prim->m_GI[ec].height[4]=random(360);
			g_prim->m_GI[ec].height[5]=random(360);
			g_prim->m_GI[ec].height[6]=random(360);
			g_prim->m_GI[ec].height[7]=random(360);
			if (g_prim->m_GI[ec].flag1[11] == 1)
			{
				int temp;
				temp = g_prim->m_GI[ec].vecB_now.x;
				g_prim->m_GI[ec].vecB_now.x = g_prim->m_GI[ec].vecB_now.z;
				g_prim->m_GI[ec].vecB_now.z = temp;
			}

			if (g_prim->m_GI[ec].flag1[12] == 0)
				g_prim->m_GI[ec].vecT_pre.x=random(45)-22;
			else
				g_prim->m_GI[ec].vecT_pre.x=random(23)-11;
			g_prim->m_GI[ec].vecT_pre.y=random(30);
			g_prim->m_GI[ec].vecT_pre.z=random(11)-5;
			g_prim->m_GI[ec].height[8]=random(360);
			g_prim->m_GI[ec].height[9]=random(360);
			g_prim->m_GI[ec].height[10]=random(360);
			g_prim->m_GI[ec].height[11]=random(360);
			if (g_prim->m_GI[ec].flag1[11] == 1)
			{
				int temp;
				temp = g_prim->m_GI[ec].vecT_pre.x;
				g_prim->m_GI[ec].vecT_pre.x = g_prim->m_GI[ec].vecT_pre.z;
				g_prim->m_GI[ec].vecT_pre.z = temp;
			}

			if (g_prim->m_GI[ec].flag1[12] == 0)
				g_prim->m_GI[ec].vecT_now.x=random(45)-22;
			else
				g_prim->m_GI[ec].vecT_now.x=random(23)-11;
			g_prim->m_GI[ec].vecT_now.y=random(30);
			g_prim->m_GI[ec].vecT_now.z=random(11)-5;
			g_prim->m_GI[ec].height[12]=random(360);
			g_prim->m_GI[ec].height[13]=random(360);
			g_prim->m_GI[ec].height[14]=random(360);
			g_prim->m_GI[ec].height[15]=random(360);
			if (g_prim->m_GI[ec].flag1[11] == 1)
			{
				int temp;
				temp = g_prim->m_GI[ec].vecT_now.x;
				g_prim->m_GI[ec].vecT_now.x = g_prim->m_GI[ec].vecT_now.z;
				g_prim->m_GI[ec].vecT_now.z = temp;
			}

			g_prim->m_GI[ec].flag1[0]=2;
		}
	}
}

void	CRagEffect :: ShieldBoomerang(char *tName)
{
	int ec;
	float x,z;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\cru_shield boomerang.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		g_prim = LaunchEffectPrim(PP_SHIELDBOOMERANG);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;

		g_prim->m_deltaPos = m_deltaPos;

		x=m_deltaPos.x-m_pos.x;
		z=m_deltaPos.z-m_pos.z;




		ec=0;

		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process=0;
		g_prim->m_GI[ec].max_height=_sqrt( (x*x) + (z*z) );;

		g_prim->m_GI[ec].distance = 7.0f;
		g_prim->m_GI[ec].height[8] = 0;
		g_prim->m_GI[ec].alphaB = 255;

		g_prim->m_GI[ec].vecB_now.x=m_pos.x;
		g_prim->m_GI[ec].vecB_now.z=m_pos.z;
		g_prim->m_GI[ec].vecB_now.y = m_pos.y-8.0f;

		g_prim->m_GI[ec].RotStart=0;
		g_prim->m_GI[ec].height[1] = 2.0f;
		g_prim->m_GI[ec].height[2] = (m_deltaPos.y-m_pos.y)*(g_prim->m_GI[ec].height[1] / g_prim->m_GI[ec].max_height);

		g_prim->m_GI[ec].full_display_angle = 0;
		g_prim->m_GI[ec].flag1[0] = 0;
		g_prim->m_GI[ec].flag1[1] = 0;
		g_prim->m_GI[ec].flag1[2] = 0;

		g_prim->m_GI[1].life = 1;
		g_prim->m_GI[1].alphaB = 0;

		g_prim->m_GI[2].life = 1;
		g_prim->m_GI[2].alphaB = 0;

		g_prim->m_GI[3].life = 1;
		g_prim->m_GI[3].alphaB = 0;
	}
}

void	CRagEffect :: ShieldBoomerang3(char *tName)
{
	int ec;

	m_pos = m_master->m_pos;

	if ( m_stateCnt == 0 || m_stateCnt == 3 || m_stateCnt == 6 || m_stateCnt == 9 || m_stateCnt == 12 )
	{
		if ( m_stateCnt == 0 )
		{
			vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
			PlayWave("effect\\cru_shield boomerang.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		}

		g_prim = LaunchEffectPrim(PP_SHIELDBOOMERANG3);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;



		ec=0;

		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process=0;

		g_prim->m_GI[ec].vecB_now.x=m_pos.x;
		g_prim->m_GI[ec].vecB_now.z=m_pos.z;
		g_prim->m_GI[ec].vecB_now.y = m_pos.y-8.0f;

		g_prim->m_GI[ec].distance = 5.0f;
		g_prim->m_GI[ec].height[8] = 0;
		g_prim->m_GI[ec].alphaB = 25;

		g_prim->m_GI[ec].RotStart=random(360);
		g_prim->m_GI[ec].height[1] = 2.5f;

		g_prim->m_GI[ec].vecB_now.x-=g_prim->m_GI[ec].height[1]*15*g_cosTable[g_prim->m_GI[ec].RotStart];
		g_prim->m_GI[ec].vecB_now.z-=g_prim->m_GI[ec].height[1]*15*g_sinTable[g_prim->m_GI[ec].RotStart];

		g_prim->m_GI[ec].full_display_angle = 0;
		g_prim->m_GI[ec].flag1[0] = 0;
		g_prim->m_GI[ec].flag1[2] = 0;

		g_prim->m_GI[1].life = 1;
		g_prim->m_GI[1].alphaB = 0;

		g_prim->m_GI[2].life = 1;
		g_prim->m_GI[2].alphaB = 0;

		g_prim->m_GI[3].life = 1;
		g_prim->m_GI[3].alphaB = 0;
	}
}

void	CRagEffect :: PRESSURE(char *tName,char F1)
{
	int ec;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_PRESSURE);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;



		ec=0;

		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process=0;


		if (F1 == 0)
			g_prim->m_GI[ec].distance = 12.0f;
		else
			g_prim->m_GI[ec].distance = 8.0f;
		g_prim->m_GI[ec].alphaB = 0;


		g_prim->m_GI[ec].vecB_pre.x=0;
		g_prim->m_GI[ec].vecB_pre.z=0;
		if (F1 == 0)
			g_prim->m_GI[ec].vecB_pre.y = -115.0f;
		else
			g_prim->m_GI[ec].vecB_pre.y = -80.0f;

		g_prim->m_GI[ec].RotStart=0;
		g_prim->m_GI[ec].height[1] = 3.0f;
		g_prim->m_GI[ec].full_display_angle = random(360);
		g_prim->m_GI[ec].flag1[0] = 0;
		g_prim->m_GI[ec].flag1[2] = 7;

		if (F1 == 1)
		{
			g_prim->m_GI[ec].flag1[2] = 9;
			g_prim->m_GI[ec].flag1[3] = 1;
		}
		else if (F1 == 2)
		{
			g_prim->m_GI[ec].flag1[2] = 9;
			g_prim->m_GI[ec].flag1[3] = 3;
		}
		else if (F1 == 3)
		{
			g_prim->m_GI[ec].flag1[2] = 9;
			g_prim->m_GI[ec].flag1[3] = 4;
		}

		g_prim->m_GI[ec].flag1[4] = F1;

		g_prim->m_GI[1].life = 1;
		g_prim->m_GI[1].alphaB = 0;

		g_prim->m_GI[2].life = 1;
		g_prim->m_GI[2].alphaB = 0;

		g_prim->m_GI[3].life = 1;
		g_prim->m_GI[3].alphaB = 0;
	}
}

void	CRagEffect :: BASH3D(char *tName,char F1,char F2)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{

		g_prim = LaunchEffectPrim(PP_BASH3D);
		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		int ec;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;

			g_prim->m_GI[ec].height[4]=F2;

			if (F2 == 2 || F2 == 3)
				g_prim->m_GI[ec].process=0;
			else
				g_prim->m_GI[ec].process=-24;
			g_prim->m_GI[ec].distance=2.0f;
			if (F2 == 3)
				g_prim->m_GI[ec].rise_angle=(90*ec)+(F1*7);
			else
				g_prim->m_GI[ec].rise_angle=(90*ec)+(F1*22);
			g_prim->m_GI[ec].RotStart=random(360);
			g_prim->m_GI[ec].alphaB=0;
			g_prim->m_GI[ec].height[0]=-12.0f;

		}
	}
}

void	CRagEffect :: FirstAid()
{
	int ec;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("_heal_effect.wav", m_pos.x-pPos.x, 150,  m_pos.z-pPos.z);

		g_prim = LaunchEffectPrim(PP_FIRSTAID);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\pikapika2.bmp", false);

		g_prim->m_size=0;




		ec=0;

		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process=0;

		g_prim->m_GI[ec].distance = 10.0f;
		g_prim->m_GI[ec].height[8] = 0;
		g_prim->m_GI[ec].alphaB = 0;

		g_prim->m_GI[ec].vecB_now.x=m_pos.x;
		g_prim->m_GI[ec].vecB_now.z=m_pos.z;
		g_prim->m_GI[ec].vecB_now.y = m_pos.y-12.0f;

		g_prim->m_GI[ec].height[6] = 0.02f;
		g_prim->m_GI[ec].full_display_angle = random(360);
		g_prim->m_GI[ec].flag1[0] = 0;
		g_prim->m_GI[ec].flag1[2] = 1;
		g_prim->m_GI[ec].height[8] = random(360);

		g_prim->m_GI[1].life = 0;
		g_prim->m_GI[1].alphaB = 0;

		g_prim->m_GI[2].life = 0;
		g_prim->m_GI[2].alphaB = 0;

		g_prim->m_GI[3].life = 0;
		g_prim->m_GI[3].alphaB = 0;
	}
}

void	CRagEffect :: AirTexture(char* tName)
{
	int ec;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		float Rx,Rz;

		g_prim = LaunchEffectPrim(PP_AIRTEXTURE);


		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;



		ec=0;

		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process=0;

		g_prim->m_GI[ec].distance = 5.0f;

		g_prim->m_GI[ec].alphaB = 0;

		g_prim->m_GI[ec].vecB_now = m_deltaPos;


		g_prim->m_GI[ec].full_display_angle = random(360);




		Rz=m_deltaPos.z-m_pos.z;
		Rx=m_deltaPos.x-m_pos.x;

		g_prim->Get_Angle(Rx,Rz,&g_prim->m_GI[ec].rise_angle);
		g_prim->m_GI[ec].rise_angle+=90;
		if (g_prim->m_GI[ec].rise_angle >= 360)
			g_prim->m_GI[ec].rise_angle -= 360;


		g_prim->m_GI[1].life = 0;
		g_prim->m_GI[1].alphaB = 0;

		g_prim->m_GI[2].life = 0;
		g_prim->m_GI[2].alphaB = 0;

		g_prim->m_GI[3].life = 0;
		g_prim->m_GI[3].alphaB = 0;
	}
}

void	CRagEffect :: ThrowItem(char *tName,char F1)
{
	int ec;
	float x,z;
	char time;

	m_pos = m_master->m_pos;

	time=0;
	if (F1 == 10)
		time = 5;
	else if (F1 == 11)
		time = 10;

	if (m_stateCnt == time)
	{
		g_prim = LaunchEffectPrim(PP_THROWITEM);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;

		g_prim->m_deltaPos = m_deltaPos;

		x=m_deltaPos.x-m_pos.x;
		z=m_deltaPos.z-m_pos.z;




		ec=0;

		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process=0;
		g_prim->m_GI[ec].max_height=_sqrt( (x*x) + (z*z) );;

		if (F1 == 2)
			g_prim->m_GI[ec].distance = 2.0f;
		else if (F1 == 3)
			g_prim->m_GI[ec].distance = 4.0f;
		else
			g_prim->m_GI[ec].distance = 3.0f;

		g_prim->m_GI[ec].height[8] = 0;
		g_prim->m_GI[ec].alphaB = 255;

		g_prim->m_GI[ec].vecB_now.x=m_pos.x;
		g_prim->m_GI[ec].vecB_now.z=m_pos.z;
		g_prim->m_GI[ec].vecB_now.y = 0;
		g_prim->m_GI[ec].vecB_pre.y = m_pos.y-8.0f;

		g_prim->m_GI[ec].RotStart=0;
		if (F1 == 2)
			g_prim->m_GI[ec].height[1] = 2.0f;
		else if (F1 == 3)
			g_prim->m_GI[ec].height[1] = 3.0f;
		else
			g_prim->m_GI[ec].height[1] = g_prim->m_GI[ec].max_height / 25.0f;

		g_prim->m_GI[ec].height[2] = (m_deltaPos.y-m_pos.y)*(g_prim->m_GI[ec].height[1] / g_prim->m_GI[ec].max_height);

		if (F1 == 3)
			g_prim->m_GI[ec].full_display_angle = 330;
		else
			g_prim->m_GI[ec].full_display_angle = 0;

		g_prim->m_GI[ec].flag1[0] = 0;
		g_prim->m_GI[ec].height[7] = 0;

		if (F1 == 0)
			g_prim->m_GI[ec].flag1[2] = 8;
		else if (F1 == 1)
		{
			g_prim->m_GI[ec].flag1[2] = 9;
			g_prim->m_GI[ec].flag1[3] = m_count;
		}
		else if (F1 >= 10)
		{
			g_prim->m_GI[ec].flag1[2] = 9;
			g_prim->m_GI[ec].flag1[3] = 4;
		}
		else if (F1 == 2)
		{
			g_prim->m_GI[ec].flag1[2] = 0;
		}
		else if (F1 == 3)
		{
			g_prim->m_GI[ec].flag1[2] = 15;
		}

		for (int i=0;i<100;i++)
		{
			if (g_prim->m_GI[ec].max_height < g_prim->m_GI[ec].height[1]*i )
			{
				g_prim->m_GI[ec].flag1[1] = i;
				break;
			}
		}

		g_prim->m_GI[1].life = 1;
		g_prim->m_GI[1].alphaB = 0;

		g_prim->m_GI[2].life = 1;
		g_prim->m_GI[2].alphaB = 0;

		g_prim->m_GI[3].life = 1;
		g_prim->m_GI[3].alphaB = 0;
	}
}

void	CRagEffect :: LINELINK( void )
{
	CGameMode *		gameMode = g_modeMgr.GetGameMode();
	CGameActor *	tActor = NULL;
	CGameActor *	sActor = NULL;

	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
			g_prim = LaunchEffectPrim(PP_LINELINK);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_texture =	 new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\cloud11.tga", false);

			int ec;
			for (ec=0;ec<1;ec++)
			{
				g_prim->m_GI[ec].life=1;
				g_prim->m_GI[ec].process=0;
				g_prim->m_GI[ec].alphaB=45;
				g_prim->m_GI[ec].distance=0;
				g_prim->m_GI[ec].max_height=0.4f;

				g_prim->m_GI[ec].height[4]=0;

				g_prim->m_GI[ec].vecT_now=m_pos;

				g_prim->m_GI[ec].height[0]=m_deltaPos.y;

				if (g_prim->m_GI[ec].height[0]>0)
				{
					if (g_prim->m_GI[ec].height[0] == g_session.GetAid())
					{
						tActor = gameMode->GetWorld()->m_player;
					}
					else
					{
						tActor = gameMode->GetWorld()->GetGameActorByAID(g_prim->m_GI[ec].height[0]);
					}
				}

				if (tActor != NULL)
				{
					g_prim->m_GI[ec].vecT_pre=tActor->m_pos;
				}
				else
					g_prim->m_GI[ec].life=0;
			}

			for (ec=1;ec<4;ec++)
			{
				g_prim->m_GI[ec].life=0;
			}
	}


}

void	CRagEffect :: LINELINK2( void )
{
	CGameMode *		gameMode = g_modeMgr.GetGameMode();
	CGameActor *	tActor = NULL;
	CGameActor *	sActor = NULL;

	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
			g_prim = LaunchEffectPrim(PP_LINELINK);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_texture =	 new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_center.tga", false);

			int ec;
			for (ec=0;ec<1;ec++)
			{
				g_prim->m_GI[ec].life=1;
				g_prim->m_GI[ec].process=0;
				g_prim->m_GI[ec].alphaB=0;
				g_prim->m_GI[ec].distance=0;
				g_prim->m_GI[ec].max_height=0.2f;

				g_prim->m_GI[ec].height[4]=2;

				g_prim->m_GI[ec].vecT_now=m_pos;

				g_prim->m_GI[ec].height[0]=m_deltaPos.y;

				if (g_prim->m_GI[ec].height[0]>0)
				{
					if (g_prim->m_GI[ec].height[0] == g_session.GetAid())
					{
						tActor = gameMode->GetWorld()->m_player;
					}
					else
					{
						tActor = gameMode->GetWorld()->GetGameActorByAID(g_prim->m_GI[ec].height[0]);
					}
				}

				if (tActor != NULL)
				{
					g_prim->m_GI[ec].vecT_pre=tActor->m_pos;
				}
				else
					g_prim->m_GI[ec].life=0;
			}

			for (ec=1;ec<4;ec++)
			{
				g_prim->m_GI[ec].life=0;
			}
	}


}

void	CRagEffect :: LINELINK3( void )
{
	CGameMode *		gameMode = g_modeMgr.GetGameMode();
	CGameActor *	tActor = NULL;
	CGameActor *	sActor = NULL;

	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
			g_prim = LaunchEffectPrim(PP_LINELINK);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_texture =	 new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_center.tga", false);

			int ec;
			for (ec=0;ec<1;ec++)
			{
				g_prim->m_GI[ec].life=1;
				g_prim->m_GI[ec].process=0;
				g_prim->m_GI[ec].alphaB=0;
				g_prim->m_GI[ec].distance=0;
				g_prim->m_GI[ec].max_height=0.7f;

				g_prim->m_GI[ec].height[4]=3;

				g_prim->m_GI[ec].vecT_now=m_pos;

				g_prim->m_GI[ec].height[0]=m_deltaPos.y;

				if (g_prim->m_GI[ec].height[0]>0)
				{
					if (g_prim->m_GI[ec].height[0] == g_session.GetAid())
					{
						tActor = gameMode->GetWorld()->m_player;
					}
					else
					{
						tActor = gameMode->GetWorld()->GetGameActorByAID(g_prim->m_GI[ec].height[0]);
					}
				}

				if (tActor != NULL)
				{
					g_prim->m_GI[ec].vecT_pre=tActor->m_pos;
				}
				else
					g_prim->m_GI[ec].life=0;
			}

			for (ec=1;ec<4;ec++)
			{
				g_prim->m_GI[ec].life=0;
			}
	}


}

void	CEffectPrim :: RenderTeiRect(int *rpCnt, int TexPart, short alphaB, short alphaT,vector3d	vec1,vector3d	vec2,vector3d	vec3,vector3d	vec4,matrix &wtm, char PW , unsigned char r, unsigned char g, unsigned char b)
{
	float Tx, Tx2;
	DWORD		m_argb_B;
	int a;
	if (*rpCnt<MAX_RENDERPRIM-2)
	{
		a=alphaB;

		m_argb_B=a;
		m_argb_B<<=8;
		m_argb_B+=r;
		m_argb_B<<=8;
		m_argb_B+=g;
		m_argb_B<<=8;
		m_argb_B+=b;

		{
			Tx=TexPart;
			Tx/=E_DIVISION;
			if (TexPart == 0)
				Tx = 0;

			Tx2=TexPart+1;
			Tx2/=E_DIVISION;
		}

		vec1 += m_pos;
		vec2 += m_pos;
		vec3 += m_pos;
		vec4 += m_pos;

		m_rp[*rpCnt].srcAlphaMode = D3DBLEND_SRCALPHA;
		if (PW == 0)
			m_rp[*rpCnt].destAlphaMode = D3DBLEND_ONE;
		else
			m_rp[*rpCnt].destAlphaMode = D3DBLEND_INVSRCALPHA;



		m_rp[*rpCnt].verts[0] = TransformVert(wtm, vec1);
		m_rp[*rpCnt].verts[0].color = m_argb_B;
		m_rp[*rpCnt].verts[0].tu = 1.0f;
		m_rp[*rpCnt].verts[0].tv = Tx;
		m_rp[*rpCnt].verts[1] = TransformVert(wtm, vec2);
		m_rp[*rpCnt].verts[1].color = m_argb_B;
		m_rp[*rpCnt].verts[1].tu = 1.0f;
		m_rp[*rpCnt].verts[1].tv = Tx2;
		m_rp[*rpCnt].verts[2] = TransformVert(wtm, vec3);
		m_rp[*rpCnt].verts[2].color = m_argb_B;
		m_rp[*rpCnt].verts[2].tu = 0.0f;
		m_rp[*rpCnt].verts[2].tv = Tx2;
		m_rp[*rpCnt].tex = m_texture[0];
		g_renderer->AddRP(&m_rp[*rpCnt], m_renderFlag);

		*rpCnt = *rpCnt + 1;

		m_rp[*rpCnt].srcAlphaMode = D3DBLEND_SRCALPHA;
		if (PW == 0)
			m_rp[*rpCnt].destAlphaMode = D3DBLEND_ONE;
		else
			m_rp[*rpCnt].destAlphaMode = D3DBLEND_INVSRCALPHA;

		m_rp[*rpCnt].verts[0] = TransformVert(wtm, vec4);
		m_rp[*rpCnt].verts[0].color = m_argb_B;
		m_rp[*rpCnt].verts[0].tu = 0.0f;
		m_rp[*rpCnt].verts[0].tv = Tx;
		m_rp[*rpCnt].verts[1] = TransformVert(wtm, vec3);
		m_rp[*rpCnt].verts[1].color = m_argb_B;
		m_rp[*rpCnt].verts[1].tu = 0.0f;
		m_rp[*rpCnt].verts[1].tv = Tx2;
		m_rp[*rpCnt].verts[2] = TransformVert(wtm, vec1);
		m_rp[*rpCnt].verts[2].color = m_argb_B;
		m_rp[*rpCnt].verts[2].tu = 1.0f;
		m_rp[*rpCnt].verts[2].tv = Tx;
		m_rp[*rpCnt].tex = m_texture[0];
		g_renderer->AddRP(&m_rp[*rpCnt], m_renderFlag);

		*rpCnt = *rpCnt + 1;
	}
}

void	CEffectPrim :: RenderTeiRect(int *rpCnt, int TexPart, short alphaB, short alphaT,vector3d	vec1,vector3d	vec2,vector3d	vec3,vector3d	vec4,matrix &wtm)
{
	float Tx, Tx2;
	DWORD		m_argb_B;
	int r,g,b,a,sw,alphamode;
	float TV;
	if (*rpCnt<MAX_RENDERPRIM-2)
	{
		a=alphaB;
		TV=1.0f;
		alphamode = D3DBLEND_ONE;
		r=255;
		g=255;
		b=255;

		sw=m_size;

		switch (sw) {
			case 0:
				alphamode = D3DBLEND_INVSRCALPHA;
			case 5:
				break;
			case 1:
				r=255;
				g=175;
				b=175;
				break;
			case 2:
				r=195;
				g=195;
				b=255;
				alphamode = D3DBLEND_INVSRCALPHA;
				break;
			case 3:
				alphaB = a = 80;
				alphamode = D3DBLEND_INVSRCALPHA;
				break;
			case 4:
				r=100;
				g=100;
				b=255;
				break;
			case 6:
				r=255;
				g=100;
				b=100;
				break;
			case 7:
				r=100;
				g=255;
				b=100;
				break;
			case 8:
				r=255;
				g=255;
				b=170;
				break;
			case 9:
				r=140;
				g=210;
				b=140;
				break;
			case 10:
				r=255;
				g=89;
				b=182;
				break;
			case 11:
				r=170;
				g=170;
				b=255;
				break;
			case 12:
				r=50;
				g=50;
				b=50;
				alphamode = D3DBLEND_INVSRCALPHA;
				break;
			case 13:
				r=255;
				g=50;
				b=255;
				alphamode = D3DBLEND_INVSRCALPHA;
				break;
			case 14:
				r=120;
				g=120;
				b=255;
				alphamode = D3DBLEND_INVSRCALPHA;
				break;
			case 15:
				r=10;
				g=255;
				b=10;
				break;
			case 100:
				alphamode = D3DBLEND_INVSRCALPHA;
				r=250;
				g=250;
				b=255;
				TV=0.9f;
				break ;
		}

		m_argb_B=a;
		m_argb_B<<=8;
		m_argb_B+=r;
		m_argb_B<<=8;
		m_argb_B+=g;
		m_argb_B<<=8;
		m_argb_B+=b;

		if (TexPart >= 100)
		{
			if (TexPart == 100)
			{
				Tx=0.35f;
				Tx2=0.45f;
			}
			else if (TexPart == 101)
			{
				Tx=0.45f;
				Tx2=0.55f;
			}
			else
			{
				Tx=0.55f;
				Tx2=0.65f;
			}
		}
		else
		{
			Tx=TexPart;
			Tx/=E_DIVISION;
			if (TexPart == 0)
				Tx = 0;

			Tx2=TexPart+1;
			Tx2/=E_DIVISION;
		}

		vec1 += m_pos;
		vec2 += m_pos;
		vec3 += m_pos;
		vec4 += m_pos;

		m_rp[*rpCnt].srcAlphaMode = D3DBLEND_SRCALPHA;

		m_rp[*rpCnt].destAlphaMode = alphamode;



		m_rp[*rpCnt].verts[0] = TransformVert(wtm, vec1);
		m_rp[*rpCnt].verts[0].color = m_argb_B;
		m_rp[*rpCnt].verts[0].tu = Tx;
		m_rp[*rpCnt].verts[0].tv = TV;
		m_rp[*rpCnt].verts[1] = TransformVert(wtm, vec2);
		m_rp[*rpCnt].verts[1].color = m_argb_B;
		m_rp[*rpCnt].verts[1].tu = Tx2;
		m_rp[*rpCnt].verts[1].tv = TV;
		m_rp[*rpCnt].verts[2] = TransformVert(wtm, vec3);
		m_rp[*rpCnt].verts[2].color = m_argb_B;
		m_rp[*rpCnt].verts[2].tu = Tx2;
		m_rp[*rpCnt].verts[2].tv = 0.0f;
		m_rp[*rpCnt].tex = m_texture[0];
		g_renderer->AddRP(&m_rp[*rpCnt], m_renderFlag);

		*rpCnt = *rpCnt + 1;

		m_rp[*rpCnt].srcAlphaMode = D3DBLEND_SRCALPHA;
		m_rp[*rpCnt].destAlphaMode = alphamode;

		m_rp[*rpCnt].verts[0] = TransformVert(wtm, vec4);
		m_rp[*rpCnt].verts[0].color = m_argb_B;
		m_rp[*rpCnt].verts[0].tu = Tx;
		m_rp[*rpCnt].verts[0].tv = 0.0f;
		m_rp[*rpCnt].verts[1] = TransformVert(wtm, vec3);
		m_rp[*rpCnt].verts[1].color = m_argb_B;
		m_rp[*rpCnt].verts[1].tu = Tx2;
		m_rp[*rpCnt].verts[1].tv = 0.0f;
		m_rp[*rpCnt].verts[2] = TransformVert(wtm, vec1);
		m_rp[*rpCnt].verts[2].color = m_argb_B;
		m_rp[*rpCnt].verts[2].tu = Tx;
		m_rp[*rpCnt].verts[2].tv = TV;
		m_rp[*rpCnt].tex = m_texture[0];
		g_renderer->AddRP(&m_rp[*rpCnt], m_renderFlag);

		*rpCnt = *rpCnt + 1;
	}
}

void	CEffectPrim :: RenderTeiRect(int *rpCnt, short alpha, vector3d	vec1,vector3d	vec2,vector3d	vec3,vector3d	vec4,matrix &wtm, short PW, char TN, char F1 , unsigned char r, unsigned char g, unsigned char b , float Ty1, float Ty2, float Tx, float Tx2)




{
	DWORD		m_argb_B;
	int a;
	if (*rpCnt<MAX_RENDERPRIM-2)
	{
		a=alpha;

		m_argb_B=(b)+(g*256)+(r*256*256)+(a*256*256*256);

		if ( F1 == 0 )
		{
			vec1 += m_pos;
			vec2 += m_pos;
			vec3 += m_pos;
			vec4 += m_pos;
		}

		m_rp[*rpCnt].srcAlphaMode = D3DBLEND_SRCALPHA;
		if (PW == 0)
			m_rp[*rpCnt].destAlphaMode = D3DBLEND_ONE;
		else
			m_rp[*rpCnt].destAlphaMode = D3DBLEND_INVSRCALPHA;

		m_rp[*rpCnt].verts[0] = TransformVert(wtm, vec1);
		m_rp[*rpCnt].verts[0].color = m_argb_B;
		m_rp[*rpCnt].verts[0].tu = Tx;
		m_rp[*rpCnt].verts[0].tv = Ty2;
		m_rp[*rpCnt].verts[1] = TransformVert(wtm, vec2);
		m_rp[*rpCnt].verts[1].color = m_argb_B;
		m_rp[*rpCnt].verts[1].tu = Tx2;
		m_rp[*rpCnt].verts[1].tv = Ty2;
		m_rp[*rpCnt].verts[2] = TransformVert(wtm, vec3);
		m_rp[*rpCnt].verts[2].color = m_argb_B;
		m_rp[*rpCnt].verts[2].tu = Tx2;
		m_rp[*rpCnt].verts[2].tv = Ty1;
		m_rp[*rpCnt].tex = m_texture[TN];
		g_renderer->AddRP(&m_rp[*rpCnt], m_renderFlag);

		*rpCnt = *rpCnt + 1;

		m_rp[*rpCnt].srcAlphaMode = D3DBLEND_SRCALPHA;
		if (PW == 0)
			m_rp[*rpCnt].destAlphaMode = D3DBLEND_ONE;
		else
			m_rp[*rpCnt].destAlphaMode = D3DBLEND_INVSRCALPHA;

		m_rp[*rpCnt].verts[0] = TransformVert(wtm, vec4);
		m_rp[*rpCnt].verts[0].color = m_argb_B;
		m_rp[*rpCnt].verts[0].tu = Tx;
		m_rp[*rpCnt].verts[0].tv = Ty1;
		m_rp[*rpCnt].verts[1] = TransformVert(wtm, vec3);
		m_rp[*rpCnt].verts[1].color = m_argb_B;
		m_rp[*rpCnt].verts[1].tu = Tx2;
		m_rp[*rpCnt].verts[1].tv = Ty1;
		m_rp[*rpCnt].verts[2] = TransformVert(wtm, vec1);
		m_rp[*rpCnt].verts[2].color = m_argb_B;
		m_rp[*rpCnt].verts[2].tu = Tx;
		m_rp[*rpCnt].verts[2].tv = Ty2;
		m_rp[*rpCnt].tex = m_texture[TN];
		g_renderer->AddRP(&m_rp[*rpCnt], m_renderFlag);

		*rpCnt = *rpCnt + 1;
	}
}

void	CEffectPrim :: RenderTei2DRect(int *rpCnt, short alpha, float x, float y, float xsize, float ysize, matrix &wtm,char TN,unsigned char r,unsigned char g,unsigned char b)
{
	vector3d	vec[4];

	for (int i=0;i<4;i++)
		vec[i].y=0;

	vec[0].x = x;
	vec[0].z = y;

	vec[1].x = x+xsize;
	vec[1].z = y;

	vec[2].x = x+xsize;
	vec[2].z = y-ysize;

	vec[3].x = x;
	vec[3].z = y-ysize;

	Tei_TowardScreen(vec[0],vec[1],vec[2],vec[3],m_GI[0].alphaT,m_GI[1].alphaT);

	RenderTeiRect(rpCnt, alpha,vec[0],vec[1],vec[2],vec[3],wtm,1,TN,0,r,g,b);
}

bool	CEffectPrim :: Prim3DCasting()
{

	int i,ec;
	int middle,m2;

	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	for (ec=0;ec<4;ec++)
	{
		if ( m_size == 3.0f && ec<3 )
			ec = 3;

		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (ec<3)
				m_GI[ec].RotStart+=(ec+3);

			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			if (m_GI[ec].process>=m_duration-40)
			{
				if (ec<3)
				{
					m_GI[ec].alphaB-=5;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}
				else
				{
					m_GI[ec].alphaB-=2;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}
			}
			else if (m_GI[ec].process<20)
			{
				if (m_GI[ec].alphaT != 1)
				{
					if (ec==3)
					{
						m_GI[ec].alphaB+=8;
						if (m_GI[ec].alphaB>60)
							m_GI[ec].alphaB=60;
					}
					else
					{
						m_GI[ec].alphaB+=10;
						if (m_GI[ec].alphaB>180)
							m_GI[ec].alphaB=180;
					}
				}
			}

			for (i = 0 ; i<E_DIVISION ; i++)
			{
				int SinLimit;


				if (m_GI[ec].flag1[i]==0)
				{
					SinLimit=90+((i-middle)*m2);


					if (m_GI[ec].process<=90)
						m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[SinLimit]*g_sinTable[m_GI[ec].process];

					if (m_GI[ec].height[i] > m_GI[ec].max_height*g_sinTable[SinLimit])
					{
						m_GI[ec].height[i] = m_GI[ec].max_height*g_sinTable[SinLimit];
						m_GI[ec].flag1[i]=1;
					}

					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
				}
			}
		}
	}

	return true;
}

bool	CEffectPrim :: PrimFlowerCast()
{

	int i,ec;
	int middle,m2;

	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			float sinwave;
			m_GI[ec].process+=1;

			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			sinwave=g_sinTable[(m_GI[ec].process%90)*4]*2.5f;

			m_GI[ec].max_height=m_GI[ec].vecT_now.y + sinwave;

			if (m_GI[ec].rise_angle>=360)
				m_GI[ec].rise_angle-=360;
			if (m_GI[ec].rise_angle<0)
				m_GI[ec].rise_angle+=360;

			if (m_GI[ec].process>110)
			{
				m_GI[ec].alphaB-=1;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;

			}
			else if (m_GI[ec].process<20)
			{
					m_GI[ec].alphaB+=5;
					if (m_GI[ec].alphaB>180)
						m_GI[ec].alphaB=180;
			}

			for (i = 0 ; i<E_DIVISION ; i++)
			{
				int SinLimit;

				if (m_GI[ec].flag1[i]==0)
				{
					SinLimit=90+((i-middle)*m2);

					if (m_GI[ec].process<=90)
						m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[SinLimit]*g_sinTable[m_GI[ec].process];
					else
						m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[SinLimit];

					if (m_GI[ec].height[i] > m_GI[ec].max_height*g_sinTable[SinLimit])
					{
						m_GI[ec].height[i] = m_GI[ec].max_height*g_sinTable[SinLimit];
						m_GI[ec].flag1[i]=1;
					}

					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
				}
			}
		}
	}

	return true;
}

void	CEffectPrim :: PrimStormKick()
{

	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (m_GI[ec].process>0)
			{
				m_GI[ec].RotStart+=3+random(3);
				if (m_GI[ec].RotStart>=360)
					m_GI[ec].RotStart-=360;

				if (m_GI[ec].process>60)
					m_GI[ec].alphaB-=1;

				if (m_GI[ec].process<=40)
					m_GI[ec].alphaB+=1;

				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}
		}
	}
}

bool	CEffectPrim :: PrimAsuraCasting()
{

	int ec;
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;

	if (m_GI[0].flag1[4] == 1)
		m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if(m_GI[ec].process>0)
			{
				if (m_GI[ec].flag1[4] == 2)
				{
					if (m_GI[ec].process<20)
					{
						for (int i=1;i<11;i++)
						{
							m_GI[ec].height[i]+=13;

							if (m_GI[ec].height[i]>=10+(i*23))
									m_GI[ec].height[i]=10+(i*23);
						}
					}
					else
					{
						for (int i=1;i<9;i++)
						{
							m_GI[ec].height[i]-=2;

							if (m_GI[ec].height[i]<0)
								m_GI[ec].height[i]=0;
						}

						if (m_GI[ec].process>100)
						{
							for (int i=9;i<11;i++)
							{
								m_GI[ec].height[i]-=5;

								if (m_GI[ec].height[i]>0)
									m_GI[ec].distance*=1.005f;

								if (m_GI[ec].height[i]<0)
									m_GI[ec].height[i]=0;
							}
						}
					}
				}
				else
				{
					if (m_GI[ec].process<20)
					{
						for (int i=1;i<11;i++)
						{
							m_GI[ec].height[i]+=13;

							if (m_GI[ec].height[i]>=50+(i*20))
									m_GI[ec].height[i]=50+(i*20);
						}
					}
					else
					{
						{
							for (int i=1;i<10;i++)
							{
								m_GI[ec].height[i]-=5;

								if (m_GI[ec].height[i]<0)
									m_GI[ec].height[i]=0;
							}

							if (m_GI[ec].process>140)
							{
								m_GI[ec].height[10]-=5;

								if (m_GI[ec].height[10]>0)
									m_GI[ec].distance*=1.01f;

								if (m_GI[ec].height[10]<0)
									m_GI[ec].height[10]=0;
							}
						}
					}

					if (m_GI[ec].process<50)
						m_GI[ec].distance-=0.1f;
				}
			}
		}
	}

	return true;
}

void	CEffectPrim :: PrimHitLine()
{

	int ec,i;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (m_GI[ec].process>0)
			{



				if (m_GI[ec].process>35)
					m_GI[ec].alphaB-=5;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;


				m_GI[ec].height[0]+=8;
				if (m_GI[ec].height[0]>180)
				{
					m_GI[ec].height[0]-=180;
					m_GI[ec].flag1[0]++;
				}

				for (i=20;i>=1;i--)
				{
					m_GI[ec].height[i]=m_GI[ec].height[i-1];
					m_GI[ec].flag1[i]=m_GI[ec].flag1[i-1];
				}

			}
		}
	}
}

void	CEffectPrim :: PrimHitLine3()
{

	int ec,i;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].process>35)
					m_GI[ec].alphaB-=5;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;

				m_GI[ec].height[0]+=6;
				if (m_GI[ec].height[0]>180)
				{
					m_GI[ec].height[0]=180;
				}

				{
					for (i=m_GI[ec].vecT_now.y;i>=1;i--)
					{
						m_GI[ec].height[i]=m_GI[ec].height[i-1];
						m_GI[ec].flag1[i]=m_GI[ec].flag1[i-1];
					}
				}
				m_GI[ec].vecB_now = m_master->m_pos;
			}
		}
	}
}

void	CEffectPrim :: PrimElectric()
{

	int ec,i,j;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (m_GI[ec].process>0)
			{
				m_GI[ec].distance += 0.4f;
				if (m_GI[ec].distance > 15.0f )
					m_GI[ec].alphaB-=2;



				if (m_GI[ec].process<=25)
					m_GI[ec].alphaB+=5+random(4);

				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
				if (m_GI[ec].process==1)
				{
					for (j=0;j<21;j++)
					{
						m_GI[ec].height[0]+=9;
						if (m_GI[ec].height[0]>180)
						{
							m_GI[ec].height[0]=180;
						}
						else
						{
							for (i=m_GI[ec].vecT_now.y;i>=1;i--)
							{
								m_GI[ec].height[i]=m_GI[ec].height[i-1];
								m_GI[ec].flag1[i]=m_GI[ec].flag1[i-1];
							}
						}
					}
				}
			}
		}
	}
}

void	CEffectPrim :: PrimElectric2()
{

	int ec,i,j;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (m_GI[ec].process>0)
			{
				m_GI[ec].distance += 0.6f;

				if (m_GI[ec].process<=10)
					m_GI[ec].alphaB+=15+random(6);

				if (m_GI[ec].process>20)
					m_GI[ec].alphaB-=5;

				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;

				if (m_GI[ec].process==1)
				{
					for (j=0;j<21;j++)
					{
						m_GI[ec].height[0]+=18;

						if (m_GI[ec].height[0]>=360)
						{
							m_GI[ec].height[0]-=360;
						}
						else
						{
							for (i=m_GI[ec].vecT_now.y;i>=1;i--)
							{
								m_GI[ec].height[i]=m_GI[ec].height[i-1];
							}
						}
					}
				}
				else
				{
					for (i=0;i<=m_GI[ec].vecT_now.y;i++)
					{
						m_GI[ec].height[i]+=8;
						if (m_GI[ec].height[i]>=360)
							m_GI[ec].height[i]-=360;
						if (m_GI[ec].height[i]<0)
							m_GI[ec].height[i]+=360;
					}
				}
			}
		}
	}
}

bool	CEffectPrim :: Prim3DCasting4()
{

	int i,ec;
	int middle,m2;

	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			m_GI[ec].distance-=0.05f;
			if (m_GI[ec].distance<=0)
			{
				m_GI[ec].distance=10.0f;
				m_GI[ec].alphaB=0;
			}

			m_GI[ec].rise_angle = (90 - (m_GI[ec].distance*9));
			m_GI[ec].max_height = m_GI[ec].distance;

			if (m_GI[ec].process>=m_duration-40)
			{
				m_GI[ec].alphaB-=3;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}
			else if (m_GI[ec].alphaB<70)
			{
				m_GI[ec].alphaB+=1;
			}

			for (i = 0 ; i<E_DIVISION ; i++)
			{
				if ( m_GI[ec].flag1[i] == 0 )
				{
					m_GI[ec].height[i]=m_GI[ec].max_height;

					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
				}
			}
		}
	}

	return true;
}

bool	CEffectPrim :: Prim3DCasting5()
{

	int i,ec;
	int middle,m2;

	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			m_GI[ec].distance+=0.10f;

			if (m_GI[ec].distance>=13.0f)
			{
				m_GI[ec].distance=3.0f;
				m_GI[ec].alphaB=0;
				if (m_GI[ec].process>50)
					m_GI[ec].life = 0;
			}
			m_GI[ec].max_height = (m_GI[ec].distance-3.0f);
			if (m_GI[ec].max_height>5.0f)
			{
				m_GI[ec].max_height=10.0f-m_GI[ec].max_height;
				if (m_GI[ec].alphaB>0)
					m_GI[ec].alphaB-=2;
			}
			else
				m_GI[ec].alphaB+=2;

			m_GI[ec].max_height*=2;

			m_GI[ec].rise_angle = 90 - ((m_GI[ec].distance-3.0f)*5);

			for (i = 0 ; i<E_DIVISION ; i++)
			{
				if ( m_GI[ec].flag1[i] == 0 )
				{
					m_GI[ec].height[i]=m_GI[ec].max_height;

					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
				}
			}
		}
	}

	return true;
}

bool	CEffectPrim :: PrimHeal()
{

	int i,ec;
	int middle,m2;


	CView* pView = g_modeMgr.GetGameMode()->GetView();

	if (m_GI[0].flag1[0] == 33)
	{
		m_GI[2].alphaT=pView->GetCurLongitude();
		m_GI[2].alphaT=360-m_GI[2].alphaT;
	}


	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if ( m_GI[ec].flag1[0] == 1 )
				m_GI[ec].RotStart+=ec+8;
			else if ( m_GI[ec].flag1[0] == 2 )
			{
				if (ec<2)
					m_GI[ec].RotStart+=ec+4;
				else
					m_GI[ec].RotStart+=ec+2;
			}
			else if ( m_GI[ec].flag1[0] == 3 )
				m_GI[ec].RotStart+=ec+6;

			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			if (m_GI[ec].process>=m_GI[ec].alphaT && m_GI[ec].alphaT>=0 )
			{
				m_GI[ec].alphaB-=2;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}
			else if (m_GI[ec].process<16)
			{
				if (m_GI[ec].flag1[0] == 2)
					m_GI[ec].alphaB+=3;
				else if (m_GI[ec].flag1[0] == 3)
					m_GI[ec].alphaB+=2;
				else
					m_GI[ec].alphaB+=5;

				if (m_GI[ec].alphaB>180)
					m_GI[ec].alphaB=180;
			}

			for (i = 0 ; i<E_DIVISION ; i++)
			{
				int SinLimit;
				if ( m_GI[ec].flag1[i] == 0 || m_GI[ec].flag1[i] == 33 )
				{
					SinLimit = i*34;
					if (ec==0)
						SinLimit += m_GI[ec].process*4;
					else if (ec==1)
						SinLimit += m_GI[ec].process*3;
					else
						SinLimit += m_GI[ec].process*2;
					SinLimit %= 360;

					m_GI[ec].height[i]=((m_GI[ec].max_height*0.75f)+(m_GI[ec].max_height*0.25f*g_sinTable[SinLimit]));

					if (m_GI[ec].process<=90)
					{
						m_GI[ec].height[i] *= g_sinTable[m_GI[ec].process];
					}

					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
				}
				else if ( m_GI[ec].flag1[i] == 1 || m_GI[ec].flag1[i] == 3 )
				{
					m_GI[ec].height[i]=m_GI[ec].max_height;
					if (m_GI[ec].process<=90)
					{
						m_GI[ec].height[i] *= g_sinTable[m_GI[ec].process];
					}
				}
				else if ( m_GI[ec].flag1[i] == 2 )
				{
					m_GI[ec].height[i]=m_GI[ec].max_height;
					if (m_GI[ec].process<=45)
					{
						m_GI[ec].height[i] *= g_sinTable[m_GI[ec].process*4];
					}
					else
					{
						m_GI[ec].height[i] = 0;
					}
				}
			}
		}
	}

	return true;
}

void	CEffectPrim :: PrimAttackEnergy()
{

	int ec;
	int Angle;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (m_GI[ec].process>0)
			{
				if (ec<2)
					m_GI[ec].rise_angle += 1;
				else
					m_GI[ec].rise_angle += 0;
				if (m_GI[ec].rise_angle>=360)
					m_GI[ec].rise_angle-=360;

				m_GI[ec].RotStart = m_master->m_master->m_roty + 180;
				if (m_GI[ec].RotStart>=360)
					m_GI[ec].RotStart-=360;

				Angle = m_GI[ec].RotStart+90;
				if (Angle>=360)
					Angle-=360;

				m_GI[ec].vecT_now.x=g_cosTable[Angle]*5.0f;
				m_GI[ec].vecT_now.z=g_sinTable[Angle]*5.0f;

				if (m_GI[ec].alphaB<40)
				{
					m_GI[ec].alphaB+=5;
				}
			}
		}
	}
}

void	CEffectPrim :: PrimGuard()
{


	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;


			if (m_GI[ec].process<=1)
				m_GI[ec].RotStart = m_master->m_master->m_roty + 270;
			else
			{
				if (m_GI[ec].flag1[0] == 1)
					m_GI[ec].RotStart+=10;
				else
					m_GI[ec].RotStart = m_master->m_master->m_roty + 270;

			}


			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			if (m_GI[ec].process<10)
			{
					if (m_GI[ec].alphaB<100)
					{
						m_GI[ec].alphaB+=20;
					}
			}
			else if (m_GI[ec].process>26)
			{
					m_GI[ec].height[0]+=0.1f;

					if (m_GI[ec].alphaB>0)
							m_GI[ec].alphaB-=6;

					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
			}
		}
	}


}

void	CEffectPrim :: PrimHalfSphere()
{

	int ec;
	int Angle;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			m_GI[ec].RotStart = m_master->m_master->m_roty + 180;
			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			Angle = m_GI[ec].RotStart+90;
			if (Angle>=360)
				Angle-=360;

			m_GI[ec].vecT_now.x=g_cosTable[Angle]*5.0f;
			m_GI[ec].vecT_now.z=g_sinTable[Angle]*5.0f;

			if (m_GI[ec].process<=50)
			{
				if (m_GI[ec].alphaB<120)
				{
					m_GI[ec].alphaB+=5;
				}
			}

		}
	}

}

void	CEffectPrim :: PrimAttackEnergy2()
{

	int ec;
	int Angle;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].height[1]<90)
				{
					m_GI[ec].height[1]+=1;
					m_GI[ec].alphaB+=10;
					if (m_GI[ec].alphaB>60)
						m_GI[ec].alphaB=60;
				}
				else
				{
					m_GI[ec].alphaB=10;
					m_GI[ec].height[1]=0;
				}

				if ( m_GI[ec].process > 30)
				{
					if ( m_GI[ec].alphaB >= 60 - ((m_GI[ec].process-30)*4) )
					{
						m_GI[ec].alphaB = 60 - ((m_GI[ec].process-30)*4);
						if (m_GI[ec].alphaB<0)
							m_GI[ec].alphaB=0;
					}
				}

				m_GI[ec].rise_angle += 1;
				if (m_GI[ec].rise_angle>=360)
					m_GI[ec].rise_angle-=360;

				m_GI[ec].RotStart = m_master->m_master->m_roty + 180;
				if (m_GI[ec].RotStart>=360)
					m_GI[ec].RotStart-=360;

				Angle = m_GI[ec].RotStart+90;
				if (Angle>=360)
					Angle-=360;

				m_GI[ec].vecT_now.x=g_cosTable[Angle];
				m_GI[ec].vecT_now.z=g_sinTable[Angle];


			}
		}
	}

}

void	CEffectPrim :: PrimSphereWind()
{

	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			m_GI[ec].RotStart+=m_GI[ec].height[1];
			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;
		}
	}
}

void	CEffectPrim :: PrimSaintWing()
{


	int ec;


	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			m_GI[ec].rise_angle+=1;
			if (m_GI[ec].rise_angle>=360)
				m_GI[ec].rise_angle-=360;

			m_GI[ec].height[0] = -9.0f - ((m_GI[1].alphaT - 115)*0.17f);
			m_GI[ec].full_display_angle=20.0f*g_sinTable[m_GI[ec].rise_angle]+20.0f;

			if (ec==0 || ec==2)
				m_GI[ec].RotStart = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_roty + 135;
			else if (ec==1 || ec==3)
				m_GI[ec].RotStart = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_roty + 45;
		}
	}

}

void	CEffectPrim :: PrimParticleSpread()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;




	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		float speed;
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			speed = m_GI[ec].max_height * g_sinTable[int(m_GI[ec].height[2])];
			m_GI[ec].vecB_pre.y -= speed;
			speed = m_GI[ec].max_height * g_cosTable[int(m_GI[ec].height[2])];
			m_GI[ec].vecB_pre.x += speed * g_sinTable[int(m_GI[ec].height[0])];
			m_GI[ec].vecB_pre.z += speed * g_cosTable[int(m_GI[ec].height[0])];

			speed = m_GI[ec].max_height * g_sinTable[int(m_GI[ec].height[6])];
			m_GI[ec].vecB_now.y -= speed;
			speed = m_GI[ec].max_height * g_cosTable[int(m_GI[ec].height[6])];
			m_GI[ec].vecB_now.x += speed * g_sinTable[int(m_GI[ec].height[4])];
			m_GI[ec].vecB_now.z += speed * g_cosTable[int(m_GI[ec].height[4])];

			speed = m_GI[ec].max_height * g_sinTable[int(m_GI[ec].height[10])];
			m_GI[ec].vecT_pre.y -= speed;
			speed = m_GI[ec].max_height * g_cosTable[int(m_GI[ec].height[10])];
			m_GI[ec].vecT_pre.x += speed * g_sinTable[int(m_GI[ec].height[8])];
			m_GI[ec].vecT_pre.z += speed * g_cosTable[int(m_GI[ec].height[8])];

			if (m_GI[ec].process>25)
			{
				m_GI[ec].max_height*=0.9f;

				for (int i=12;i<15;i++)
				{
					m_GI[ec].height[i]-=6;
					if (m_GI[ec].height[i]<0)
						m_GI[ec].height[i]=0;
				}

			}
		}
	}
}

void	CEffectPrim :: PrimSphereWind2()
{

	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].process<=25)
					m_GI[ec].alphaB+=2;
				else if (m_GI[ec].process>70)
					m_GI[ec].alphaB-=2;

				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;

				m_GI[ec].distance*=1.01f;

				m_GI[ec].rise_angle++;
				if (random(2)==0)
					m_GI[ec].rise_angle++;
				if (m_GI[ec].rise_angle>=360)
					m_GI[ec].rise_angle-=360;

				m_GI[ec].RotStart+=m_GI[ec].height[1];
				if (m_GI[ec].RotStart>=360)
					m_GI[ec].RotStart-=360;
			}
		}
	}
}

void	CEffectPrim :: PrimColorPaper()
{

	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			m_GI[ec].rise_angle+=2;
			if (m_GI[ec].rise_angle>=360)
				m_GI[ec].rise_angle-=360;

			m_GI[ec].vecB_now.y+=m_GI[ec].distance;
			if (m_GI[ec].vecB_now.y>-10.0f)
			{
				m_GI[ec].alphaB-=10;
				if (m_GI[ec].alphaB<0)
				{
					m_GI[ec].alphaB=0;

					m_GI[ec].full_display_angle++;

					if (m_GI[ec].full_display_angle<3)
					{
						m_GI[ec].vecB_now.x=random(101)-50;
						m_GI[ec].vecB_now.z=random(101)-50;
						m_GI[ec].vecB_now.y=-random(50)-110;

						m_GI[ec].process=0;
					}
				}
			}
			else if (m_GI[ec].vecB_now.y>0)
				m_GI[ec].life=0;

			if (m_GI[ec].process<60)
			{
				m_GI[ec].alphaB+=5;
				if (m_GI[ec].alphaB>250)
					m_GI[ec].alphaB=250;
			}

		}
	}
}

bool	CEffectPrim :: PrimPortal()
{

	int i,ec;
	int middle,m2;

	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;
			if  (ec == 0)
			{
				m_GI[3].process+=1;
			}

			if (m_GI[ec].process>0)
			{
				if (m_GI[3].alphaT == 3)
				{

					m_GI[ec].distance-=0.25f;

					if (m_GI[ec].distance<7.0f)
					{
						{
							if (m_GI[ec].distance<0)
								m_GI[ec].distance=0;

							m_GI[ec].alphaB-=7;
							if (m_GI[ec].alphaB<0)
							{
								m_GI[ec].alphaB=0;

								if (m_GI[3].process > 1400)
								{
									m_GI[ec].life=0;
								}
								else
								{
									m_GI[ec].process=0;
									m_GI[ec].distance=14.0f;
								}
							}
						}
					}
				}
				else
				{
					m_GI[ec].distance+=0.5f;

					if (m_GI[ec].distance>7.0f)
					{
						{
							m_GI[ec].alphaB-=15;
							if (m_GI[ec].alphaB<0)
							{
								m_GI[ec].alphaB=0;

								if (m_GI[3].process > 120 && m_GI[3].alphaT == 1)
								{
									m_GI[ec].life=0;
								}
								else if (m_GI[3].process > 1400)
								{
									m_GI[ec].life=0;
								}
								else
								{
									m_GI[ec].process=0;
									m_GI[ec].distance=0;
								}
							}
						}
					}
				}

				if (m_GI[3].alphaT == 3)
				{
					if (m_GI[ec].process<20)
					{
						m_GI[ec].alphaB+=12;

						if (m_GI[ec].alphaB>240)
							m_GI[ec].alphaB=240;
					}
				}
				else
				{
					if (m_GI[ec].process<10)
					{
						m_GI[ec].alphaB+=24;

						if (m_GI[ec].alphaB>240)
							m_GI[ec].alphaB=240;
					}
				}

				for (i = 0 ; i<E_DIVISION ; i++) {
					{
						m_GI[ec].height[i]=m_GI[ec].max_height;
						if (m_GI[ec].process<=10)
						{
							m_GI[ec].height[i] *= g_sinTable[m_GI[ec].process*9];
						}
					}
				}
			}
		}
	}

	return true;
}

bool	CEffectPrim :: PrimWind()
{

	int ec;



	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (m_GI[ec].process>0)
			{
				m_GI[ec].RotStart+=5;
				if (m_GI[ec].RotStart>=360)
					m_GI[ec].RotStart-=360;

				if (m_GI[ec].alphaT == 2)
				{
					m_GI[ec].full_display_angle+=3;
					if (m_GI[ec].full_display_angle > 180)
						m_GI[ec].full_display_angle=180;
				}
				else
				{
					m_GI[ec].full_display_angle+=3;
					if (m_GI[ec].full_display_angle > 120)
						m_GI[ec].full_display_angle=120;
				}

				if (m_GI[ec].alphaT == 1)
				{

					if (m_GI[ec].process>20)
					{
						m_GI[ec].alphaB-=2;
						if (m_GI[ec].alphaB<0)
							m_GI[ec].alphaB=0;
					}

					if (m_GI[ec].process>20)
					{
						m_GI[ec].distance+=0.10f;
					}
				}
				else if (m_GI[ec].alphaT == 2)
				{
					if (m_GI[ec].process>50)
					{
						m_GI[ec].alphaB-=1;
						if (m_GI[ec].alphaB<0)
							m_GI[ec].alphaB=0;
					}

					if (m_GI[ec].process>12)
					{
						m_GI[ec].distance+=0.10f;
					}
				}

				if (m_GI[ec].process>1400)
				{
					if (m_GI[ec].alphaT != 3)
						m_GI[ec].alphaB-=3;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}

				if (m_GI[ec].alphaT == 2)
				{
					if (m_GI[ec].process<12)
					{
						m_GI[ec].alphaB+=1;
					}
				}
				else
				{
					if (m_GI[ec].process<12)
					{
						m_GI[ec].alphaB+=10;
						if (m_GI[ec].alphaB>250)
							m_GI[ec].alphaB=250;
					}
				}
			}

		}
	}

	return true;
}

void	CEffectPrim :: PrimMapPillar()
{

	int i,ec;
	int middle,m2;

	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	ec=0;

	for (;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			m_GI[ec].RotStart+=1;

			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			if (m_GI[ec].alphaT == 1 )
			{
				if (m_GI[ec].process>250)
				{
					m_GI[ec].alphaB-=1;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}
			}

			if (m_GI[ec].process>=800)
			{
				m_GI[ec].max_height-=1.0f;
				if (m_GI[ec].max_height<0)
					m_GI[ec].max_height=0;
			}

			for (i = 0 ; i<E_DIVISION ; i++)
			{
					if (m_GI[ec].process<200)
					{
						m_GI[ec].height[i] = 0;
					}
					else
					{
						if (m_GI[ec].process<=290)
							m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[m_GI[ec].process-200];
						else
						{
							m_GI[ec].height[i]=m_GI[ec].max_height;
						}
					}

					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
			}
		}
	}
}

void	CEffectPrim :: PrimRainbow()
{
	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life > 0)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process<=180)
			{
				m_GI[ec].alphaB++;
				m_GI[ec].RotStart++;
			}
			else if (m_GI[ec].process<=360)
			{
				m_GI[ec].alphaB--;
				m_GI[ec].RotStart++;
			}

			if (m_GI[ec].alphaB<0)
				m_GI[ec].alphaB=0;
			if (m_GI[ec].alphaB>=250)
				m_GI[ec].alphaB=250;

			if (m_GI[ec].rise_angle>=360)
				m_GI[ec].rise_angle-=360;
		}
	}
}

void	CEffectPrim :: PrimForest()
{

	int ec;



	for (ec=0;ec<4;ec++)
	{
		int sinp;
		if (m_GI[ec].life > 0)
		{
			m_GI[ec].process++;

			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			if (m_GI[ec].flag1[0] == 11)
			{
				if (m_GI[ec].process<=40)
					m_GI[ec].alphaB += 1;
				else if (m_GI[ec].process>140)
					m_GI[ec].alphaB -= 1;

				if (m_GI[ec].alphaB<=0)
					m_GI[ec].alphaB=0;
			}

			if (ec==1 || ec==3)
			{
				sinp = m_GI[ec].process;
				sinp %= 720;
				sinp /= 2;

				m_GI[ec].max_height = m_GI[ec].height[0];
				m_GI[ec].max_height += (g_sinTable[sinp]*0.5f);
			}
		}
	}
}

void	CEffectPrim :: PrimWaterFall()
{

	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life > 0)
		{
			m_GI[ec].process++;
		}
	}
}

void	CEffectPrim :: PrimLightSphere()
{

	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life > 0)
		{
			m_GI[ec].process++;

			if (m_GI[ec].distance<10)
			{
				m_GI[ec].distance*=1.02f;
			}

			if (m_GI[ec].process>500)
			{
				if (m_GI[ec].alphaB>0)
					m_GI[ec].alphaB--;
			}

			m_GI[ec].full_display_angle+=m_GI[ec].max_height;
			if (m_GI[ec].full_display_angle>=360)
				m_GI[ec].full_display_angle-=360;
		}
	}
}

bool	CEffectPrim :: Prim3DCasting3()
{

	int i,ec;
	int middle,m2;

	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	for (ec=0;ec<4;ec++)
	{
		if (m_size == 3.0f && ec<3 )
			ec = 3;
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (ec<3)
				m_GI[ec].RotStart+=(ec+3);

			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			if (m_GI[ec].process>=m_duration-40)
			{
				if (ec<3)
				{
					m_GI[ec].alphaB-=5;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}
				else
				{
					m_GI[ec].alphaB-=3;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}
			}
			else if (m_GI[ec].process<20)
			{
				if (ec==3)
				{
					m_GI[ec].alphaB+=8;
					if (m_GI[ec].alphaB>100)
						m_GI[ec].alphaB=100;
				}
				else
				{
					m_GI[ec].alphaB+=10;
					if (m_GI[ec].alphaB>180)
						m_GI[ec].alphaB=180;
				}
			}

			if (ec < 3)
			{
				for (i = 0 ; i<E_DIVISION ; i++) {
					int SinLimit;


					if (m_GI[ec].flag1[i]==0) {
						SinLimit=90+((i-middle)*m2);


						if (m_GI[ec].process<=90)
							m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[SinLimit]*g_sinTable[m_GI[ec].process];

						if (m_GI[ec].height[i] > m_GI[ec].max_height*g_sinTable[SinLimit]) {
							m_GI[ec].height[i] = m_GI[ec].max_height*g_sinTable[SinLimit];
							m_GI[ec].flag1[i]=1;
						}

						if (m_GI[ec].height[i] < 0)
							m_GI[ec].height[i] = 0;
					}
				}
			}
			else
			{
				for (i = 0 ; i<E_DIVISION ; i++) {
					int SinLimit;

					if (m_GI[ec].flag1[i]==0)
					{
						SinLimit=90+((i-middle)*m2);

						if (m_GI[ec].process<=90)
							m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[SinLimit]*g_sinTable[m_GI[ec].process];
						else
							m_GI[ec].flag1[i]=1;

						if (m_GI[ec].height[i] > m_GI[ec].max_height*g_sinTable[SinLimit])
							m_GI[ec].flag1[i]=1;

						if (m_GI[ec].height[i] < 0)
							m_GI[ec].height[i] = 0;
					}
					else
					{
						int pr;

						pr=m_GI[ec].process%360;

						SinLimit=90+((i-middle)*m2);

						m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[SinLimit]*0.55f;
						m_GI[ec].height[i]+=m_GI[ec].max_height*g_sinTable[SinLimit]*0.45f*g_sinTable[pr];
					}
				}
			}
		}
	}

	return true;
}

bool	CEffectPrim :: PrimAcidDemon()
{

	int i,ec;
	int middle,m2;

	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (ec==3)
				m_GI[ec].RotStart+=2+random(2);
			else
				m_GI[ec].RotStart+=4+random(3);

			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			if (m_GI[ec].process>=40)
			{
				m_GI[ec].alphaB-=15;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}
			else if (m_GI[ec].process<20)
			{
				m_GI[ec].alphaB+=5;
				if (m_GI[ec].alphaB>80)
					m_GI[ec].alphaB=80;
			}

			if (ec==3)
				m_GI[ec].max_height*=1.025f;
			else
				m_GI[ec].max_height*=1.005f;

			{
				for (i = 0 ; i<E_DIVISION ; i++)
				{
					int SinLimit;

					{
						SinLimit=90+((i-middle)*m2);

						if (m_GI[ec].process<=90)
							m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[SinLimit]*g_sinTable[m_GI[ec].process];
						else
							m_GI[ec].flag1[i]=1;

						if (m_GI[ec].height[i] > m_GI[ec].max_height*g_sinTable[SinLimit])
							m_GI[ec].flag1[i]=1;

						if (m_GI[ec].height[i] < 0)
							m_GI[ec].height[i] = 0;
					}
				}
			}
		}
	}

	return true;
}

bool	CEffectPrim :: Prim3DCasting2()
{

	int i,ec;
	int middle,m2;

	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	ec=0;

	for (;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (ec==0 || ec==2)
				m_GI[ec].RotStart+=1;
			else
				m_GI[ec].RotStart+=2;

			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			if (m_GI[ec].process>=m_duration-20)
			{
					m_GI[ec].alphaB-=5;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
			}
			else if (m_GI[ec].process<100)
			{
					m_GI[ec].alphaB+=1;
					if (m_GI[ec].alphaB>80)
						m_GI[ec].alphaB=80;
			}

			for (i = 0 ; i<E_DIVISION ; i++)
			{
					int SinLimit;
					int pr;

					if (ec<2)
						pr=(m_GI[ec].process+(ec*90))%360;
					else
						pr=((m_GI[ec].process*2)+(ec*90))%360;

					SinLimit=90+((i-middle)*m2);

					m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[SinLimit];
					m_GI[ec].height[i]+=m_GI[ec].max_height*g_sinTable[SinLimit]*0.3f*g_sinTable[pr];
			}
		}
	}

	return true;
}

void	CEffectPrim :: Render3DCasting(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int order;
	int count;
	int Angle;
	int BasicAngle;
	float Rx,Ry,Rz;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			BasicAngle = m_GI[ec].full_display_angle;
			BasicAngle /= E_DIVISION-1;

			order=0;

			for (count = 0 ; count<=m_GI[ec].full_display_angle ; count+=BasicAngle)
			{
				Angle=count+m_GI[ec].RotStart;
				Angle%=360;

				if (m_GI[ec].full_display_angle == 360)
				{
					if ( order == E_DIVISION-1 )
						Angle = m_GI[ec].RotStart;
				}

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				vec2.x=cs1*m_GI[ec].distance;
				vec2.z=sn1*m_GI[ec].distance;
				if (m_GI[ec].max_height == 6.001f)
					vec2.y=-2.0f;
				else if (m_GI[ec].max_height == 24.001f)
					vec2.y=-1.0f;
				else
					vec2.y=-0;

				Rx=g_cosTable[m_GI[ec].rise_angle] * m_GI[ec].height[order];
				Ry=g_sinTable[m_GI[ec].rise_angle] * m_GI[ec].height[order];

				Rz=sn1*Rx;
				Rx=cs1*Rx;

				vec4.x=vec2.x+Rx;
				vec4.z=vec2.z+Rz;
				if (m_GI[ec].max_height == 6.001f)
					vec4.y=-(2.0f+Ry);
				else if (m_GI[ec].max_height == 24.001f)
					vec4.y=-(1.0f+Ry);
				else
					vec4.y=-(Ry);

				if (order > 0)
				{
					RenderTeiRect(&rpCnt, order-1, m_GI[ec].alphaB, m_GI[ec].alphaB, vec1,vec2,vec4,vec3,wtm);
				}

				order++;
				if (order>=E_DIVISION)
					order=E_DIVISION-1;

				vec1=vec2;
				vec3=vec4;
			}
		}
	}
}

void	CEffectPrim :: Render3DCasting_LowPolygon(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int order;
	int count;
	int Angle;
	int BasicAngle;
	float Rx,Ry,Rz;
	int ec;
	int E_DIV;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			BasicAngle = m_GI[ec].full_display_angle;
			E_DIV = 10;
			BasicAngle /= E_DIV;

			order=0;

			for (count = 0 ; count<=m_GI[ec].full_display_angle ; count+=BasicAngle)
			{
				Angle=count+m_GI[ec].RotStart;
				Angle%=360;

				if (m_GI[ec].full_display_angle == 360)
				{
					if ( order == E_DIV )
						Angle = m_GI[ec].RotStart;
				}

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				vec2.x=cs1*m_GI[ec].distance;
				vec2.z=sn1*m_GI[ec].distance;
				vec2.y=-0;

				Rx=g_cosTable[m_GI[ec].rise_angle] * m_GI[ec].height[order];
				Ry=g_sinTable[m_GI[ec].rise_angle] * m_GI[ec].height[order];

				Rz=sn1*Rx;
				Rx=cs1*Rx;

				vec4.x=vec2.x+Rx;
				vec4.z=vec2.z+Rz;
				vec4.y=-(Ry);


				if (m_GI[ec].vecB_now.y == 1234.5f)
				{
					float lx,lz,x,z;

					lx=m_GI[ec].vecB_now.x;
					lz=m_GI[ec].vecB_now.z;

					x=(lx*g_cosTable[m_GI[2].alphaT])-(lz*g_sinTable[m_GI[2].alphaT]);
					z=(lx*g_sinTable[m_GI[2].alphaT])+(lz*g_cosTable[m_GI[2].alphaT]);

					vec2.x+=x;
					vec4.x+=x;

					vec2.z+=z;
					vec4.z+=z;

				}

				if (order > 0)
				{
					RenderTeiRect(&rpCnt, order-1, m_GI[ec].alphaB, m_GI[ec].alphaB, vec1,vec2,vec4,vec3,wtm);

				}

				order++;
				if (order>E_DIV)
					order=E_DIV;

				vec1=vec2;
				vec3=vec4;
			}
		}
	}
}

void	CEffectPrim :: RenderGuard(matrix &wtm)
{
	float		cs1, sn1 ,cs2, sn2;
	vector3d	vec1, vec2, vec3, vec4, vecM;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int ec;
	int add,add2;
	int count,count2;
	int Angle,Angle2;
	float Rx,Rz;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			for (count2=0;count2<3;count2++)
			{
				for (count=0;count<5;count++)
				{
					add=(count*45)-90;
					add2=(count2*45)-45;

					m_GI[ec].distance = (count2-1)*1.0f;

					if (m_GI[ec].distance<0)
						m_GI[ec].distance *= -1;

					m_GI[ec].distance = 1.5f - m_GI[ec].distance + 1.5f;



					Angle2=add2;
					if (Angle2>=360)
						Angle2-=360;
					if (Angle2<0)
						Angle2+=360;

					sn2 = g_sinTable[Angle2];
					cs2 = g_cosTable[Angle2];

					Rx=m_GI[ec].max_height;

					vecM.y=(Rx*sn2);
					Rz=(Rx*cs2);



					Angle=m_GI[ec].RotStart+add;
					if (Angle>=360)
						Angle-=360;
					if (Angle<0)
						Angle+=360;

					cs1 = g_cosTable[Angle];
					sn1 = g_sinTable[Angle];

					Rx=Rz;

					vecM.x=(Rx*cs1);
					vecM.z=(Rx*sn1);

					vecM.y+=-(m_GI[ec].max_height+2);


					Angle2=90+add2;
					if (Angle2>=360)
						Angle2-=360;
					if (Angle2<0)
						Angle2+=360;

					cs2 = g_cosTable[Angle2];
					sn2 = g_sinTable[Angle2];


					Angle=m_GI[ec].RotStart+add-90;
					if (Angle>=360)
						Angle-=360;
					if (Angle<0)
						Angle+=360;

					cs1 = g_cosTable[Angle];
					sn1 = g_sinTable[Angle];

					GuardPoint(m_GI[ec].distance,m_GI[ec].distance,sn2,cs2,sn1,cs1,vec1,vecM);
					GuardPoint(-m_GI[ec].distance,m_GI[ec].distance,sn2,cs2,sn1,cs1,vec2,vecM);
					GuardPoint(-m_GI[ec].distance,-m_GI[ec].distance,sn2,cs2,sn1,cs1,vec3,vecM);
					GuardPoint(m_GI[ec].distance,-m_GI[ec].distance,sn2,cs2,sn1,cs1,vec4,vecM);

					if (m_GI[ec].flag1[0] == 2)
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1, vec2, vec3, vec4,wtm,0,0,0, 255,155,0);
					else
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1, vec2, vec3, vec4,wtm,0,0,0, 205,255,205);



					Angle2=add2;
					if (Angle2>=360)
						Angle2-=360;
					if (Angle2<0)
						Angle2+=360;

					cs2 = g_cosTable[Angle2];
					sn2 = g_sinTable[Angle2];

					Rx=m_GI[ec].max_height+0.2f;

					vecM.y=(Rx*sn2);
					Rz=(Rx*cs2);



					Angle=m_GI[ec].RotStart+add;
					if (Angle>=360)
						Angle-=360;
					if (Angle<0)
						Angle+=360;

					cs1 = g_cosTable[Angle];
					sn1 = g_sinTable[Angle];


					Rx=Rz;

					vecM.x=(Rx*cs1);
					vecM.z=(Rx*sn1);

					vecM.y+=-(m_GI[ec].max_height);



					Angle2=90+add2;
					if (Angle2>=360)
						Angle2-=360;
					if (Angle2<0)
						Angle2+=360;

					cs2 = g_cosTable[Angle2];
					sn2 = g_sinTable[Angle2];


					Angle=m_GI[ec].RotStart+add-90;
					if (Angle>=360)
						Angle-=360;
					if (Angle<0)
						Angle+=360;

					cs1 = g_cosTable[Angle];
					sn1 = g_sinTable[Angle];

					GuardPoint(m_GI[ec].distance,m_GI[ec].distance,sn2,cs2,sn1,cs1,vec1,vecM);
					GuardPoint(-m_GI[ec].distance,m_GI[ec].distance,sn2,cs2,sn1,cs1,vec2,vecM);
					GuardPoint(-m_GI[ec].distance,-m_GI[ec].distance,sn2,cs2,sn1,cs1,vec3,vecM);
					GuardPoint(m_GI[ec].distance,-m_GI[ec].distance,sn2,cs2,sn1,cs1,vec4,vecM);


					if (m_GI[ec].flag1[0] == 2)
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1, vec2, vec3, vec4,wtm,0,0,0, 255,155,0);
					else
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1, vec2, vec3, vec4,wtm,0,0,0, 155,255,155);





					Angle2=add2;
					if (Angle2>=360)
						Angle2-=360;
					if (Angle2<0)
						Angle2+=360;

					cs2 = g_cosTable[Angle2];
					sn2 = g_sinTable[Angle2];

					Rx=m_GI[ec].max_height+0.4f+m_GI[ec].height[0];

					vecM.y=(Rx*sn2);
					Rz=(Rx*cs2);



					Angle=m_GI[ec].RotStart+add;
					if (Angle>=360)
						Angle-=360;
					if (Angle<0)
						Angle+=360;

					cs1 = g_cosTable[Angle];
					sn1 = g_sinTable[Angle];


					Rx=Rz;

					vecM.x=(Rx*cs1);
					vecM.z=(Rx*sn1);

					vecM.y+=-(m_GI[ec].max_height);


					Angle2=90+add2;
					if (Angle2>=360)
						Angle2-=360;
					if (Angle2<0)
						Angle2+=360;

					cs2 = g_cosTable[Angle2];
					sn2 = g_sinTable[Angle2];


					Angle=m_GI[ec].RotStart+add-90;
					if (Angle>=360)
						Angle-=360;
					if (Angle<0)
						Angle+=360;

					cs1 = g_cosTable[Angle];
					sn1 = g_sinTable[Angle];

					GuardPoint(m_GI[ec].distance,m_GI[ec].distance,sn2,cs2,sn1,cs1,vec1,vecM);
					GuardPoint(-m_GI[ec].distance,m_GI[ec].distance,sn2,cs2,sn1,cs1,vec2,vecM);
					GuardPoint(-m_GI[ec].distance,-m_GI[ec].distance,sn2,cs2,sn1,cs1,vec3,vecM);
					GuardPoint(m_GI[ec].distance,-m_GI[ec].distance,sn2,cs2,sn1,cs1,vec4,vecM);


					if (m_GI[ec].flag1[0] == 2)
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1, vec2, vec3, vec4,wtm,1,1,0, 255,155,0);
					else
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1, vec2, vec3, vec4,wtm,1,1,0, 225,255,225);

				}
			}










		}
	}
}

void	CEffectPrim :: RenderHalfSphere(matrix &wtm)
{
	float		cs1, sn1;
	float		cs2, sn2;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int Angle,Angle2;
	int BasicAngle,BasicAngle2;
	int order;
	int count;
	int ec;
	float Dist;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			BasicAngle = m_GI[ec].full_display_angle;
			BasicAngle /= 10;

			BasicAngle2 = 18;

			for (Angle2 = 0; Angle2 < 180; Angle2+=BasicAngle2)
			{
				order=0;
				for (count = 0 ; count<=m_GI[ec].full_display_angle ; count+=BasicAngle)
				{
					Angle=count+m_GI[ec].RotStart;
					if (Angle>=360)
						Angle-=360;

					if (m_GI[ec].full_display_angle == 360)
					{
						if ( count == m_GI[ec].full_display_angle )
							Angle = m_GI[ec].RotStart;
					}

					cs1 = g_cosTable[Angle];
					sn1 = g_sinTable[Angle];

					cs2 = g_cosTable[Angle2];
					sn2 = g_sinTable[Angle2];

					Dist = m_GI[ec].distance;

					vec2.y = cs2*Dist;
					Dist = sn2*Dist;

					vec2.x=cs1*Dist;
					vec2.z=sn1*Dist;
					vec2.y-=m_GI[ec].max_height;

					vec2.x+=m_GI[ec].vecT_now.x;
					vec2.z+=m_GI[ec].vecT_now.z;



					cs2 = g_cosTable[Angle2+BasicAngle2];
					sn2 = g_sinTable[Angle2+BasicAngle2];

					Dist = m_GI[ec].distance;

					vec4.y = cs2*Dist;
					Dist = sn2*Dist;

					vec4.x=cs1*Dist;
					vec4.z=sn1*Dist;
					vec4.y-=m_GI[ec].max_height;

					vec4.x+=m_GI[ec].vecT_now.x;
					vec4.z+=m_GI[ec].vecT_now.z;

					if (count > 0)
					{
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1, vec2, vec4, vec3,wtm,1,0,0, 250,250,255);
					}

					order++;

					vec1=vec2;
					vec3=vec4;
				}
			}
		}
	}
}

void	CEffectPrim :: RenderAttackEnergy(matrix &wtm)
{
	float		cs1, sn1;
	float		cs2, sn2;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int ec;
	float Dist;
	int order;
	int count;
	int Angle;
	int BasicAngle;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB>0)
		{
			BasicAngle = m_GI[ec].full_display_angle;
			BasicAngle /= E_DIVISION-1;

			{
				order=0;
				for (count = 0 ; count<=m_GI[ec].full_display_angle ; count+=BasicAngle)
				{
					Angle=count+m_GI[ec].rise_angle;
					if (Angle>=360)
						Angle-=360;

					if (m_GI[ec].full_display_angle == 360)
					{
						if ( count == m_GI[ec].full_display_angle )
							Angle = m_GI[ec].rise_angle;
					}

					cs1 = g_cosTable[Angle];
					sn1 = g_sinTable[Angle];

					cs2 = g_cosTable[m_GI[ec].RotStart];
					sn2 = g_sinTable[m_GI[ec].RotStart];

					Dist = m_GI[ec].distance;

					vec2.y = cs1*Dist;
					Dist = sn1*Dist;

					vec2.x=cs2*Dist;
					vec2.z=sn2*Dist;
					vec2.y-=m_GI[ec].max_height;



					Dist = m_GI[ec].distance+(m_GI[ec].height[0]*m_GI[ec].distance*0.1f);

					vec4.y = cs1*Dist;
					Dist = sn1*Dist;

					vec4.x=cs2*Dist;
					vec4.z=sn2*Dist;
					vec4.y-=(m_GI[ec].max_height);



					vec2.x+=m_GI[ec].vecT_now.x;
					vec2.z+=m_GI[ec].vecT_now.z;

					vec4.x+=m_GI[ec].vecT_now.x*-m_GI[ec].height[0];
					vec4.z+=m_GI[ec].vecT_now.z*-m_GI[ec].height[0];

					if (count > 0)
					{

						RenderTeiRect(&rpCnt, order-1, m_GI[ec].alphaB, m_GI[ec].alphaB, vec1,vec2,vec4,vec3,wtm);
					}

					order++;
					if (order>E_DIVISION-1)
						order=E_DIVISION-1;

					vec3=vec4;
					vec1=vec2;
				}
			}
		}
	}
}

void	CEffectPrim :: RenderAttackEnergy2(matrix &wtm)
{
	float		cs1, sn1;
	float		cs2, sn2;
	float		cs3, sn3;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int order;
	int count;
	float Dist;
	int Angle,Angle3;
	int BasicAngle;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB>0)
		{
			BasicAngle = m_GI[ec].full_display_angle;
			BasicAngle /= E_DIVISION-1;

			{
				order=0;
				for (count = 0 ; count<=m_GI[ec].full_display_angle ; count+=BasicAngle)
				{
					Angle=count+m_GI[ec].rise_angle;
					if (Angle>=360)
						Angle-=360;

					if (m_GI[ec].full_display_angle == 360)
					{
						if ( count == m_GI[ec].full_display_angle )
							Angle = m_GI[ec].rise_angle;
					}

					cs1 = g_cosTable[Angle];
					sn1 = g_sinTable[Angle];

					cs2 = g_cosTable[m_GI[ec].RotStart];
					sn2 = g_sinTable[m_GI[ec].RotStart];

					cs3 = g_cosTable[(int)m_GI[ec].height[1]];
					sn3 = g_sinTable[(int)m_GI[ec].height[1]];

					Dist = m_GI[ec].distance*sn3;

					vec2.y = cs1*Dist;
					Dist = sn1*Dist;

					vec2.x=cs2*Dist;
					vec2.z=sn2*Dist;
					vec2.y-=m_GI[ec].max_height;

					vec2.x+=m_GI[ec].vecT_now.x*3.0f;
					vec2.z+=m_GI[ec].vecT_now.z*3.0f;

					vec2.x+=m_GI[ec].vecT_now.x*cs3*m_GI[ec].max_height;
					vec2.z+=m_GI[ec].vecT_now.z*cs3*m_GI[ec].max_height;


					Angle3 = m_GI[ec].height[1] + 10;
					if (Angle3>90)
						Angle3=90;
					cs3 = g_cosTable[Angle3];
					sn3 = g_sinTable[Angle3];

					Dist = m_GI[ec].distance*sn3;

					vec4.y = cs1*Dist;
					Dist = sn1*Dist;

					vec4.z=sn2*Dist;
					vec4.x=cs2*Dist;
					vec4.y-=(m_GI[ec].max_height);

					vec4.x+=m_GI[ec].vecT_now.x*3.0f;
					vec4.z+=m_GI[ec].vecT_now.z*3.0f;

					vec4.x+=m_GI[ec].vecT_now.x*cs3*m_GI[ec].max_height;
					vec4.z+=m_GI[ec].vecT_now.z*cs3*m_GI[ec].max_height;

					if (count > 0)
					{
						RenderTeiRect(&rpCnt, order-1, m_GI[ec].alphaB, m_GI[ec].alphaB, vec1,vec2,vec4,vec3,wtm);
						RenderTeiRect(&rpCnt, order-1, m_GI[ec].alphaB, m_GI[ec].alphaB, vec1,vec2,vec4,vec3,wtm);
					}

					order++;
					if (order>E_DIVISION-1)
						order=E_DIVISION-1;

					vec1=vec2;
					vec3=vec4;
				}
			}
		}
	}
}

void	CEffectPrim :: RenderSaintWing(matrix &wtm)
{
	float		cs1, sn1 ,cs2, sn2;
	vector3d	vec1, vec2, vec3, vec4, vecM;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int Angle,Angle2;
	float Rx,Rz,Ry;
	int ec;
	int add;
	int i;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{

			for (i=0;i<10;i++)
			{
				Rx=m_GI[ec].distance;

				Angle=m_GI[ec].RotStart-65;
				if (Angle>=360)
					Angle-=360;
				if (Angle<0)
					Angle+=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				vec1.x=(Rx*cs1);
				vec1.z=(Rx*sn1);
				vec1.y=m_GI[ec].height[0];


				Angle2=m_GI[ec].RotStart+65;
				if (Angle2>=360)
					Angle2-=360;
				if (Angle2<0)
					Angle2+=360;

				cs2 = g_cosTable[Angle2];
				sn2 = g_sinTable[Angle2];

				vec4.x=(Rx*cs2);
				vec4.z=(Rx*sn2);
				vec4.y=m_GI[ec].height[0];


				if (i<5)
					Rz=m_GI[ec].max_height;
				else
					Rz=m_GI[ec].max_height+((0.5f*g_sinTable[m_GI[ec].rise_angle])+0.5f);


				Angle=m_GI[ec].RotStart;
				if (Angle>=360)
					Angle-=360;
				if (Angle<0)
					Angle+=360;

				sn1 = g_sinTable[Angle];
				cs1 = g_cosTable[Angle];

				{
					if (i<5)
						add = ((i-(i%5))*9)+((i%5)*3)+m_GI[ec].full_display_angle;
					else
						add = ((i-(i%5))*9)+((i%5)*3);
					Rx=g_cosTable[add] * Rz;
					Ry=g_sinTable[add] * Rz;
				}

				m_GI[ec].vecB_now.z=sn1*Rx;
				m_GI[ec].vecB_now.x=cs1*Rx;

				vec2.Set( vec1.x+m_GI[ec].vecB_now.x , vec1.y - Ry , vec1.z+m_GI[ec].vecB_now.z );
				vec3.Set( vec4.x+m_GI[ec].vecB_now.x , vec4.y - Ry , vec4.z+m_GI[ec].vecB_now.z );

				if (ec == 1)
				{
					int A;
					A=m_GI[ec].alphaB;
					if (i%5 < 4)
					{
						for (int B=3;B>=(i%5);B--)
						{
							A*=0.7f;
						}
						RenderTeiRect(&rpCnt, A, vec1, vec2, vec3, vec4,wtm,0,0,0, 105,105,255);
					}
					else
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1, vec2, vec3, vec4,wtm,1,0,0, 205,205,255);
				}
				else if (ec == 0)
				{
					int A;
					A=m_GI[ec].alphaB;
					if (i%5 < 4)
					{
						for (int B=3;B>=(i%5);B--)
						{
							A*=0.7f;
						}
						RenderTeiRect(&rpCnt, A, vec4, vec3, vec2, vec1,wtm,0,0,0, 105,105,255);
					}
					else
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec4, vec3, vec2, vec1,wtm,1,0,0, 205,205,255);
				}
			}








		}
	}
}

void	CEffectPrim :: RenderSphereWind(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4, vecM , vecA;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int ec;
	int count;
	int Angle;
	float Rx,Rz,Ry;
	int BasicAngle,order;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			BasicAngle = m_GI[ec].full_display_angle;
			BasicAngle /= E_DIVISION-1;

			order=0;

			for (count = 0 ; count<=m_GI[ec].full_display_angle ; count+=BasicAngle)
			{
				Angle=count+m_GI[ec].RotStart;
				if (Angle>=360)
					Angle-=360;
				else if (Angle<0)
					Angle+=360;

				if (m_GI[ec].full_display_angle == 360)
				{
					if ( order == E_DIVISION-1 )
						Angle = m_GI[ec].RotStart;
				}

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];


				Rx=g_cosTable[m_GI[ec].rise_angle] * m_GI[ec].distance;

				Ry=g_sinTable[m_GI[ec].rise_angle] * m_GI[ec].distance;

				Ry=cs1*Ry;
				Rx=cs1*Rx;
				Rz=sn1*m_GI[ec].distance;

				vecM.x=Rx;
				vecM.z=Rz;
				vecM.y=Ry;



				Rx=-(m_GI[ec].max_height*g_sinTable[m_GI[ec].rise_angle]);
				Ry= (m_GI[ec].max_height*g_cosTable[m_GI[ec].rise_angle]);
				Rz=0;

				vecA.x=Rx;
				vecA.y=Ry;
				vecA.z=Rz;


				vec2.x=vecM.x+vecA.x;
				vec2.z=vecM.z+vecA.z;
				vec2.y=vecM.y+vecA.y+m_GI[ec].height[0];

				vec4.x=vecM.x-vecA.x;
				vec4.z=vecM.z-vecA.z;
				vec4.y=vecM.y-vecA.y+m_GI[ec].height[0];

				if (order > 0)
				{
					RenderTeiRect(&rpCnt, order-1, m_GI[ec].alphaB, m_GI[ec].alphaB, vec1,vec2,vec4,vec3,wtm);
				}

				order++;
				if (order>=E_DIVISION)
					order=E_DIVISION-1;

				vec1=vec2;
				vec3=vec4;
			}
		}
	}
}

void	CEffectPrim :: RenderColorPaper(matrix &wtm)
{
	float		cs1, sn1 , cs2, sn2;
	vector3d	vec1, vec2, vec3, vec4, vecM , vecA;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int Angle;
	float Rx,Ry;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			Rx=m_GI[ec].max_height;

			cs2 = g_cosTable[m_GI[ec].rise_angle];
			sn2 = g_sinTable[m_GI[ec].rise_angle];

			Angle=m_GI[ec].RotStart;
			if (Angle>=360)
				Angle-=360;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];

			vec1.x=(Rx*cs1);
			vec1.y=(Rx*sn1);
			vec1.z=0;

			Ry=vec1.y;
			vec1.y=(Ry*cs2);
			vec1.z=(Ry*sn2);



			Angle=m_GI[ec].RotStart+90;
			if (Angle>=360)
				Angle-=360;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];

			vec2.x=(Rx*cs1);
			vec2.y=(Rx*sn1);
			vec2.z=0;

			Ry=vec2.y;
			vec2.y=(Ry*cs2);
			vec2.z=(Ry*sn2);


			Angle=m_GI[ec].RotStart+180;
			if (Angle>=360)
				Angle-=360;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];

			vec3.x=(Rx*cs1);
			vec3.y=(Rx*sn1);
			vec3.z=0;

			Ry=vec3.y;
			vec3.y=(Ry*cs2);
			vec3.z=(Ry*sn2);



			Angle=m_GI[ec].RotStart+270;
			if (Angle>=360)
				Angle-=360;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];

			vec4.x=(Rx*cs1);
			vec4.y=(Rx*sn1);
			vec4.z=0;

			Ry=vec4.y;
			vec4.y=(Ry*cs2);
			vec4.z=(Ry*sn2);



			vec3+=m_GI[ec].vecB_now;
			vec4+=m_GI[ec].vecB_now;
			vec1+=m_GI[ec].vecB_now;
			vec2+=m_GI[ec].vecB_now;

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1, vec2, vec3, vec4,wtm,0,0,0, m_GI[ec].height[0], m_GI[ec].height[1], m_GI[ec].height[2]);

		}
	}
}

void	CEffectPrim :: GuardPoint(int Rx, int Ry, float sn2, float cs2, float sn1,float cs1, vector3d	&vecA, vector3d	&vecM)
{
	float Rz;


					vecA.y=(Ry*sn2);
					vecA.z=(Ry*cs2);
					vecA.x=Rx;



					Rx=vecA.x;
					Rz=vecA.z;

					vecA.x=(Rx*cs1)-(Rz*sn1);
					vecA.z=(Rx*sn1)+(Rz*cs1);


					vecA+=vecM;
}

void	CEffectPrim :: RenderAsuraCasting(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	float Rx;
	int ec;
	int Alpha;
	int Angle;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			for (int i=10;i>=1;i--)
			{
				Angle = 45;

				if (m_GI[ec].flag1[4] == 2)
					Rx=m_GI[ec].distance + ((10-i)*1.0f);
				else
					Rx=m_GI[ec].distance + ((10-i)*3);

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				m_GI[ec].vecB_pre.x=cs1*Rx;
				m_GI[ec].vecB_pre.z=sn1*Rx;
				m_GI[ec].vecB_pre.y=0;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				m_GI[ec].vecB_now.x=cs1*Rx;
				m_GI[ec].vecB_now.z=sn1*Rx;
				m_GI[ec].vecB_now.y=0;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				m_GI[ec].vecT_now.x=cs1*Rx;
				m_GI[ec].vecT_now.z=sn1*Rx;
				m_GI[ec].vecT_now.y=0;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				m_GI[ec].vecT_pre.x=cs1*Rx;
				m_GI[ec].vecT_pre.z=sn1*Rx;
				m_GI[ec].vecT_pre.y=0;


				vec1 = m_GI[ec].vecB_pre;
				vec2 = m_GI[ec].vecB_now;
				vec3 = m_GI[ec].vecT_now;
				vec4 = m_GI[ec].vecT_pre;

				vec1.x+=m_GI[ec].height[0];
				vec2.x+=m_GI[ec].height[0];
				vec3.x+=m_GI[ec].height[0];
				vec4.x+=m_GI[ec].height[0];

				Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

				if (m_GI[ec].flag1[4] == 2)
				{
					vec1.y-=40.0f;
					vec2.y-=40.0f;
					vec3.y-=40.0f;
					vec4.y-=40.0f;
				}
				else
				{
					vec1.y-=30.0f;
					vec2.y-=30.0f;
					vec3.y-=30.0f;
					vec4.y-=30.0f;
				}

				Alpha = m_GI[ec].height[i];
				if (Alpha>0)
				{
					if (m_GI[ec].flag1[4] == 0)
						RenderTeiRect(&rpCnt, Alpha, vec2, vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],0,0,0,0);
					else if (m_GI[ec].flag1[4] == 3)
					{
						if (i == 10)
							RenderTeiRect(&rpCnt, Alpha/2, vec2, vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],0,10,10,10);
						RenderTeiRect(&rpCnt, Alpha, vec2, vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],0,10,10,10);
					}
					else if (m_GI[ec].flag1[4] == 5)
					{
						if (i == 10)
							RenderTeiRect(&rpCnt, Alpha/2, vec2, vec1,vec4,vec3,wtm,0,m_GI[ec].flag1[0],0,125,125,255);
						RenderTeiRect(&rpCnt, Alpha, vec2, vec1,vec4,vec3,wtm,0,m_GI[ec].flag1[0],0,125,125,255);
					}
					else
						RenderTeiRect(&rpCnt, Alpha, vec2, vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],0,255,255,255);

				}
			}

		}
	}
}

void	CEffectPrim :: RenderHitLine(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vecA, vecB;
	int			rpCnt = 0;
	float  radius;
	float		uInc = 0.0f;
	short Angle;
	int alpha;
	float speed;
	float Rx,Rz,Ry;
	int ec;
	float Tx;
	int i,j;

	for (ec=0 ; ec<4 ; ec++)
	{
		if ( m_GI[ec].life == 1 && m_GI[ec].alphaB > 0 && (m_GI[ec].process >0))
		{
			for  (j=0;j<20;j++)
			{

				speed=0;
				radius=1.0f;
				for (i=0;i<m_GI[ec].flag1[j];i++)
				{
					speed+=m_GI[ec].distance*2*radius;
					radius*=0.7f;
				}

				if (m_GI[ec].height[j]>0)
				{
					Rx = m_GI[ec].height[j]/90;
					Rx *= m_GI[ec].distance*radius;
					speed += Rx;
				}

				Angle = m_GI[ec].height[j];
				vecA.y = -m_GI[ec].distance * g_sinTable[Angle] * 1.5f;


				radius=1.0f;
				for (i=0;i<m_GI[ec].flag1[j];i++)
				{
					radius*=0.7f;
				}
				vecA.y*=radius;


				vecA.x = speed*g_cosTable[m_GI[ec].rise_angle];
				vecA.z = speed*g_sinTable[m_GI[ec].rise_angle];


				speed=0;
				radius=1.0f;
				for (i=0;i<m_GI[ec].flag1[j+1];i++)
				{
					speed+=m_GI[ec].distance*2*radius;
					radius*=0.7f;
				}

				if (m_GI[ec].height[j+1]>0)
				{
					Rx = m_GI[ec].height[j+1]/90;
					Rx *= m_GI[ec].distance*radius;
					speed += Rx;
				}


				Angle = m_GI[ec].height[j+1];
				vecB.y = -m_GI[ec].distance * g_sinTable[Angle] * 1.5f;

				radius=1.0f;
				for (i=0;i<m_GI[ec].flag1[j+1];i++)
				{
					radius*=0.7f;
				}
				vecB.y*=radius;

				vecB.x = speed*g_cosTable[m_GI[ec].rise_angle];
				vecB.z = speed*g_sinTable[m_GI[ec].rise_angle];


				Rx=vecA.x-vecB.x;
				Rz=vecA.z-vecB.z;

				Get_Angle(Rx,Rz,&Angle);

				vecA+=m_GI[ec].vecB_now;
				vecB+=m_GI[ec].vecB_now;


				Angle+=90;
				Angle%=360;

				for (i=0;i<=3;i++)
				{
						radius=m_GI[ec].max_height;

						Rx=radius*g_cosTable[i*120];
						Ry=radius*g_sinTable[i*120];

						Tx=Rx;

						Rx=Tx*g_cosTable[Angle];
						Rz=Tx*g_sinTable[Angle];


						vec2=vecA;
						vec2.x+=Rx;
						vec2.y+=Ry;
						vec2.z+=Rz;

						vec4=vecB;
						vec4.x+=Rx;
						vec4.y+=Ry;
						vec4.z+=Rz;

						if (i>0)
						{
							alpha=m_GI[ec].alphaB - (j*5);
							if (alpha>0)
							{
								if (m_GI[ec].alphaT == 2)
									RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,0,0,1,255,200-((20-j)*7),200-((20-j)*7));
								else
									RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,0,0,1,255,200,150-((20-j)*7));
							}
						}

						vec1=vec2;
						vec3=vec4;
				}
			}
		}
	}
}

void	CEffectPrim :: RenderHitLine3(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vecA, vecB;
	int			rpCnt = 0;
	float  radius;
	float		uInc = 0.0f;
	short Angle;
	int alpha;
	float Rx,Rz,Ry;
	float Tx;
	int ec;
	int i,j;
	float speed;

	for (ec=0 ; ec<4 ; ec++)
	{
		if ( m_GI[ec].life == 1 && m_GI[ec].alphaB > 0 && (m_GI[ec].process >0))
		{
			for  (j=0;j<m_GI[ec].vecT_now.y;j++)
			{

				speed=0;

				if (m_GI[ec].height[j]>0)
				{
					Rx = m_GI[ec].height[j]/90;
					Rx *= m_GI[ec].distance;
					speed += Rx;
				}

				Angle = m_GI[ec].height[j];
				vecA.y = -m_GI[ec].distance * g_sinTable[Angle] * m_GI[ec].vecT_now.x;

				vecA.x = speed*g_cosTable[m_GI[ec].rise_angle];
				vecA.z = speed*g_sinTable[m_GI[ec].rise_angle];


				speed=0;

				if (m_GI[ec].height[j+1]>0)
				{
					Rx = m_GI[ec].height[j+1]/90;
					Rx *= m_GI[ec].distance;
					speed += Rx;
				}


				Angle = m_GI[ec].height[j+1];
				vecB.y = -m_GI[ec].distance * g_sinTable[Angle] * m_GI[ec].vecT_now.x;

				vecB.x = speed*g_cosTable[m_GI[ec].rise_angle];
				vecB.z = speed*g_sinTable[m_GI[ec].rise_angle];


				Rx=vecA.x-vecB.x;
				Rz=vecA.z-vecB.z;

				Get_Angle(Rx,Rz,&Angle);

				vecA+=m_GI[ec].vecB_now;
				vecB+=m_GI[ec].vecB_now;


				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				for (i=0;i<=3;i++)
				{
						radius=m_GI[ec].max_height;

						Rx=radius*g_cosTable[i*120];
						Ry=radius*g_sinTable[i*120];

						Tx=Rx;

						Rx=Tx*g_cosTable[Angle];
						Rz=Tx*g_sinTable[Angle];


						vec2=vecA;
						vec2.x+=Rx;
						vec2.y+=Ry-5.0f;
						vec2.z+=Rz;

						vec4=vecB;
						vec4.x+=Rx;
						vec4.y+=Ry-5.0f;
						vec4.z+=Rz;

						if (i>0)
						{
							alpha=m_GI[ec].alphaB - (j*10);
							if (alpha>0)
							{
								if (m_GI[ec].alphaT == 0)
									RenderTeiRect(&rpCnt, alpha, vec2,vec4,vec3,vec1,wtm,0,0,1,255,200,150-((5-j)*30));
								else if (m_GI[ec].alphaT == 1)
									RenderTeiRect(&rpCnt, alpha, vec2,vec4,vec3,vec1,wtm,0,0,1,120-((5-j)*22),120-((5-j)*22),255);
								else if (m_GI[ec].alphaT == 2)
									RenderTeiRect(&rpCnt, alpha, vec2,vec4,vec3,vec1,wtm,0,0,1,255,200,150-((3-j)*50));
								else if (m_GI[ec].alphaT >= 3)
									RenderTeiRect(&rpCnt, alpha, vec2,vec4,vec3,vec1,wtm,0,0,1,255-((5-j)*30),255-((5-j)*30),255-((5-j)*30));
							}
						}

						vec1=vec2;
						vec3=vec4;
				}


				if (m_GI[ec].alphaT >= 3)
				{
					for (i=0;i<=3;i++)
					{
						radius=m_GI[ec].max_height*4;

						Rx=radius*g_cosTable[i*120];
						Ry=radius*g_sinTable[i*120];

						Tx=Rx;

						Rx=Tx*g_cosTable[Angle];
						Rz=Tx*g_sinTable[Angle];


						vec2=vecA;
						vec2.x+=Rx;
						vec2.y+=Ry-5.0f;
						vec2.z+=Rz;

						vec4=vecB;
						vec4.x+=Rx;
						vec4.y+=Ry-5.0f;
						vec4.z+=Rz;

						if (i>0)
						{
							alpha=m_GI[ec].alphaB - (j*10);
							alpha*=0.5f;
							if (alpha>0)
							{
								if (m_GI[ec].alphaT == 3)
									RenderTeiRect(&rpCnt, alpha, vec2,vec4,vec3,vec1,wtm,0,0,1,0,0,255);
							}
						}

						vec1=vec2;
						vec3=vec4;
					}
				}
			}
		}
	}
}

void	CEffectPrim :: RenderStormKick(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	short Angle;
	float Rx;
	float Height;
	int ec;
	int i;
	float cs1,sn1;

	for (ec=0 ; ec<4 ; ec++)
	{
		if ( m_GI[ec].life == 1 && m_GI[ec].alphaB > 0 )
		{
			Height = -m_GI[ec].height[0];
			Rx=m_GI[ec].max_height;

			for (i=0;i<m_GI[ec].flag1[0];i++)
			{
				if (i>0)
				{
					Height += m_GI[ec].height[1];
					Rx -= m_GI[ec].distance*(2.2f-(0.2f*i));
				}

				Angle = m_GI[ec].RotStart + (i*45);
				while (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				vec1.x=cs1*Rx;
				vec1.z=sn1*Rx;
				vec1.y=Height-1.0f;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				vec2.x=cs1*Rx;
				vec2.z=sn1*Rx;
				vec2.y=Height-1.0f;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				vec3.x=cs1*Rx;
				vec3.z=sn1*Rx;
				vec3.y=Height;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				vec4.x=cs1*Rx;
				vec4.z=sn1*Rx;
				vec4.y=Height;

				switch (m_GI[ec].flag1[4])
				{
					case 0:
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,0,50,50,255);
						break;
					case 1:
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,164,136,55);
						break;
					case 2:
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,155,255,155);
						break;
					case 3:
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,130,130,255);
						break;
					case 4:
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,255,50,50);
						break;
					case 5:
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,5,5,5);
						break;
					case 6:
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,255,255,255);
						break;
					case 7:
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,255,125,255);
						break;
				}
			}
		}
	}
}

void	CEffectPrim :: Render3DCasting2(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	int order;
	int count;
	int Angle;
	int BasicAngle;
	float Rx,Ry,Rz;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			BasicAngle = m_GI[ec].full_display_angle;

			BasicAngle /= 9;

			order=0;

			for (count = 0 ; count<m_GI[ec].full_display_angle ; count+=BasicAngle)
			{
				Angle=count+m_GI[ec].RotStart;
				Angle%=360;

				if (m_GI[ec].full_display_angle == 360)
				{
					if ( order == E_DIVISION-1 )
						Angle = m_GI[ec].RotStart;
				}

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];


				m_GI[ec].vecB_now.x=cs1*m_GI[ec].distance;
				m_GI[ec].vecB_now.z=sn1*m_GI[ec].distance;
				m_GI[ec].vecB_now.y=-0;

				Rx=g_cosTable[m_GI[ec].rise_angle] * m_GI[ec].height[order];
				Ry=g_sinTable[m_GI[ec].rise_angle] * m_GI[ec].height[order];




				if ( ec == 2 )
					Angle = 90 - Angle;

				else if ( ec == 1 )
					Angle = 270 - Angle;

				else if ( ec == 0 )
					Angle = 450 - Angle;
				else
					Angle = 630 - Angle;

				if (Angle>=360)
					Angle-=360;

				sn1 = g_sinTable[Angle];
				cs1 = g_cosTable[Angle];


				Rz=sn1*Rx;
				Rx=cs1*Rx;

				m_GI[ec].vecT_now.x=m_GI[ec].vecB_now.x+Rx;
				m_GI[ec].vecT_now.z=m_GI[ec].vecB_now.z+Rz;
				m_GI[ec].vecT_now.y=-(Ry);

				vec1 = m_GI[ec].vecT_now;
				vec1.x += m_GI[ec].flag1[0];
				vec1.z += m_GI[ec].flag1[1];

				vec2 = m_GI[ec].vecB_now;
				vec2.x += m_GI[ec].flag1[0];
				vec2.z += m_GI[ec].flag1[1];

				vec3 = m_GI[ec].vecT_pre;
				vec3.x += m_GI[ec].flag1[0];
				vec3.z += m_GI[ec].flag1[1];

				vec4 = m_GI[ec].vecB_pre;
				vec4.x += m_GI[ec].flag1[0];
				vec4.z += m_GI[ec].flag1[1];

				if (order > 0)
				{
					RenderTeiRect(&rpCnt, order-1, m_GI[ec].alphaB, m_GI[ec].alphaB, vec4,vec2,vec1,vec3,wtm);
				}

				order++;
				if (order>=E_DIVISION)
					order=E_DIVISION-1;

				m_GI[ec].vecT_pre=m_GI[ec].vecT_now;
				m_GI[ec].vecB_pre=m_GI[ec].vecB_now;
			}
		}
	}
}

void	CEffectPrim :: RenderElectric(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vecA, vecB;
	int			rpCnt = 0;
	float		radius;
	float		uInc = 0.0f;
	short		Angle;
	int			alpha;
	float		Rx,Rz,Ry;
	float		heightR[21],distXR[21],distZR[21];
	float		Tx;
	int			ec;
	int			i,j;
	float		speed;

	for (ec=0 ; ec<4 ; ec++)
	{
		for (i=0;i<m_GI[ec].vecT_now.y;i++)
		{
			heightR[i]=(random(7)*0.1f)-0.3f;
			distXR[i]=(random(7)*0.1f)-0.3f;
			distZR[i]=(random(7)*0.1f)-0.3f;
		}

		if ( m_GI[ec].life == 1 && m_GI[ec].alphaB > 0 && (m_GI[ec].process >0))
		{
			for  (j=0;j<m_GI[ec].vecT_now.y;j++)
			{

				speed=0;

				if (m_GI[ec].height[j]>0)
				{
					Rx = m_GI[ec].height[j]/90;
					Rx *= m_GI[ec].distance;
					speed += Rx;
				}

				Angle = m_GI[ec].height[j];
				vecA.y = -m_GI[ec].distance * g_sinTable[Angle] * m_GI[ec].vecT_now.x;

				vecA.x = speed*g_cosTable[m_GI[ec].rise_angle];
				vecA.z = speed*g_sinTable[m_GI[ec].rise_angle];

				vecA.y += heightR[j] - 2.0f;
				vecA.x += distXR[j];
				vecA.z += distZR[j];


				speed=0;

				if (m_GI[ec].height[j+1]>0)
				{

					Rx = m_GI[ec].height[j+1]/90;
					Rx *= m_GI[ec].distance;
					speed += Rx;
				}

				Angle = m_GI[ec].height[j+1];
				vecB.y = -m_GI[ec].distance * g_sinTable[Angle] * m_GI[ec].vecT_now.x;

				vecB.x = speed*g_cosTable[m_GI[ec].rise_angle];
				vecB.z = speed*g_sinTable[m_GI[ec].rise_angle];

				vecB.y += heightR[j+1] - 2.0f;
				vecB.x += distXR[j+1];
				vecB.z += distZR[j+1];


				Rx=vecA.x-vecB.x;
				Rz=vecA.z-vecB.z;

				Get_Angle(Rx,Rz,&Angle);

				vecA+=m_GI[ec].vecB_now;
				vecB+=m_GI[ec].vecB_now;


				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				if (j>0 && j<19)
				{
					for (i=0;i<=3;i++)
					{
						radius=m_GI[ec].max_height;

						Rx=radius*g_cosTable[i*120];
						Ry=radius*g_sinTable[i*120];

						Tx=Rx;

						Rx=Tx*g_cosTable[Angle];
						Rz=Tx*g_sinTable[Angle];

						vec2=vecA;
						vec2.x+=Rx;
						vec2.y+=Ry;
						vec2.z+=Rz;

						vec4=vecB;
						vec4.x+=Rx;
						vec4.y+=Ry;
						vec4.z+=Rz;

						if (i>0)
						{
							alpha=m_GI[ec].alphaB - (j*5);
							if (alpha>0)
							{
								RenderTeiRect(&rpCnt, alpha, vec2,vec4,vec3,vec1,wtm,0,0,1,120-((20-j)*6),120-((20-j)*6),255);
							}
						}

						vec1=vec2;
						vec3=vec4;
					}
				}

			}

		}
	}
}

void	CEffectPrim :: RenderElectric2(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vecA, vecB;
	float		radius;
	float		uInc = 0.0f;
	int			rpCnt = 0;
	short Angle;
	int alpha;
	float Rx,Rz,Ry;
	float speed;
	float Tx;
	int ec;
	int i,j;
	float heightR[21],distXR[21],distZR[21];

	for (ec=0 ; ec<4 ; ec++)
	{
		for (i=0;i<m_GI[ec].vecT_now.y;i++)
		{
			heightR[i]=(random(11)*0.1f)-0.5f;
			distXR[i]=(random(11)*0.1f)-0.5f;
			distZR[i]=(random(11)*0.1f)-0.5f;
		}

		if ( m_GI[ec].life == 1 && m_GI[ec].alphaB > 0 && (m_GI[ec].process >0))
		{
			for  (j=0;j<m_GI[ec].vecT_now.y;j++)
			{

				speed=0;

				if (j>0)
				{
					Rx = j;
					Rx /= 5;
					Rx *= m_GI[ec].distance;
					speed += Rx;
				}

				Angle = m_GI[ec].height[j];
				vecA.y = -15 * g_sinTable[Angle] * m_GI[ec].vecT_now.x;

				vecA.x = speed*g_cosTable[m_GI[ec].rise_angle];
				vecA.z = speed*g_sinTable[m_GI[ec].rise_angle];

				vecA.y += heightR[j] - 8.0f;
				vecA.x += distXR[j];
				vecA.z += distZR[j];


				speed=0;

				if (j+1>0)
				{
					Rx = j+1;
					Rx /= 5;
					Rx *= m_GI[ec].distance;
					speed += Rx;
				}

				Angle = m_GI[ec].height[j+1];
				vecB.y = -15 * g_sinTable[Angle] * m_GI[ec].vecT_now.x;

				vecB.x = speed*g_cosTable[m_GI[ec].rise_angle];
				vecB.z = speed*g_sinTable[m_GI[ec].rise_angle];

				vecB.y += heightR[j+1] - 8.0f;
				vecB.x += distXR[j+1];
				vecB.z += distZR[j+1];


				Rx=vecA.x-vecB.x;
				Rz=vecA.z-vecB.z;

				Get_Angle(Rx,Rz,&Angle);

				vecA+=m_GI[ec].vecB_now;
				vecB+=m_GI[ec].vecB_now;


				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				if (j>0 && j<19)
				{
					for (i=0;i<=3;i++)
					{
						radius=m_GI[ec].max_height;

						Rx=radius*g_cosTable[i*120];
						Ry=radius*g_sinTable[i*120];

						Tx=Rx;

						Rx=Tx*g_cosTable[Angle];
						Rz=Tx*g_sinTable[Angle];

						vec2=vecA;
						vec2.x+=Rx;
						vec2.y+=Ry;
						vec2.z+=Rz;

						vec4=vecB;
						vec4.x+=Rx;
						vec4.y+=Ry;
						vec4.z+=Rz;

						if (i>0)
						{
							if (j<10)
								alpha=m_GI[ec].alphaB - ((9-j)*15);
							else
								alpha=m_GI[ec].alphaB - ((j-10)*15);
							if (alpha>0)
							{
								RenderTeiRect(&rpCnt, alpha, vec2,vec4,vec3,vec1,wtm,0,0,1,random(121),random(121),255);
							}
						}

						vec1=vec2;
						vec3=vec4;
					}


				}

			}

		}
	}
}

void	CEffectPrim :: RenderWind(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int order;
	int count;
	int Angle;
	int BasicAngle;
	float Rx,Ry,Rz;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			BasicAngle = m_GI[ec].full_display_angle;
			BasicAngle /= E_DIVISION;

			order=0;

			for (count = 0 ; count<m_GI[ec].full_display_angle ; count+=BasicAngle)
			{
				Angle=count+m_GI[ec].RotStart;
				Angle%=360;

				if (m_GI[ec].full_display_angle == 360)
				{
					if ( order == E_DIVISION-1 )
						Angle = m_GI[ec].RotStart;
				}

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];


				m_GI[ec].vecB_now.x=cs1*m_GI[ec].distance;
				m_GI[ec].vecB_now.z=sn1*m_GI[ec].distance;
				m_GI[ec].vecB_now.y=-m_GI[ec].max_height;

				Rx=g_cosTable[m_GI[ec].rise_angle] * m_GI[ec].height[order];
				Ry=g_sinTable[m_GI[ec].rise_angle] * m_GI[ec].height[order];

				Rz=sn1*Rx;
				Rx=cs1*Rx;

				m_GI[ec].vecT_now.x=m_GI[ec].vecB_now.x+Rx;
				m_GI[ec].vecT_now.z=m_GI[ec].vecB_now.z+Rz;
				m_GI[ec].vecT_now.y=-(Ry+m_GI[ec].max_height);

				if (order > 0)
				{
					RenderTeiRect(&rpCnt, order-1, m_GI[ec].alphaB, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm);
				}

				order++;
				if (order>=E_DIVISION)
					order=E_DIVISION-1;

				m_GI[ec].vecB_pre=m_GI[ec].vecB_now;
				m_GI[ec].vecT_pre=m_GI[ec].vecT_now;
			}
		}
	}
}

void CWeather :: LaunchSakura( void )
{
	if( m_Effect ) {
		if( m_Effect->GetDuration ()-m_Effect->GetRagStateCnt()<=300 ) return;
	}
	CGameMode *		gameMode = g_modeMgr.GetGameMode();

	m_pPlayer = gameMode->GetWorld()->m_player;
	m_Effect = m_pPlayer->LaunchEffect(EF_SAKURA);
}

void CWeather :: LaunchMaple( void )
{
	if( m_Effect ) {
		if( m_Effect->GetDuration ()-m_Effect->GetRagStateCnt()<=300 ) return;
	}
	CGameMode *		gameMode = g_modeMgr.GetGameMode();

	m_pPlayer = gameMode->GetWorld()->m_player;
	m_Effect = m_pPlayer->LaunchEffect(EF_MAPLE);
}

void CWeather :: LaunchBackGroundTexture( char F1 )
{
	CGameMode *		gameMode = g_modeMgr.GetGameMode();

	m_pPlayer = gameMode->GetWorld()->m_player;
	m_pPlayer->LaunchEffect(EF_BACKTEXTURE1);
}

void CWeather :: StopSakura( void )
{
	if (m_Effect) {
		m_Effect->SetDuration(m_Effect->GetRagStateCnt() + 300);
	}
}

void CWeather :: StopMaple( void )
{
	if (m_Effect) {
		m_Effect->SetDuration(m_Effect->GetRagStateCnt() + 300);
	}
}

void CWeather :: LaunchCloud( char map )
{
	if( m_Effect ) {
		if( m_Effect->GetDuration ()-m_Effect->GetRagStateCnt()<=300 ) return;
	}
	CGameMode *		gameMode = g_modeMgr.GetGameMode();

	m_pPlayer = gameMode->GetWorld()->m_player;

	if ( map == 0 )
		m_Effect = m_pPlayer->LaunchEffect(EF_CLOUD);
	else if ( map == 1 )
		m_Effect = m_pPlayer->LaunchEffect(EF_CLOUD2);
	else if ( map == 2 )
		m_Effect = m_pPlayer->LaunchEffect(EF_CLOUD3);
	else if ( map == 3 )
		m_Effect = m_pPlayer->LaunchEffect(EF_CLOUD4);
	else if ( map == 4 )
		m_Effect = m_pPlayer->LaunchEffect(EF_CLOUD5);
	else if ( map == 5 )
		m_Effect = m_pPlayer->LaunchEffect(EF_CLOUD6);
	else if ( map == 7 )
		m_Effect = m_pPlayer->LaunchEffect(EF_CLOUD7);
	else if ( map == 8 )
		m_Effect = m_pPlayer->LaunchEffect(EF_CLOUD8);

}

void CWeather :: LaunchPokJuk( void )
{
	CGameMode *		gameMode = g_modeMgr.GetGameMode();

	m_pPlayer = gameMode->GetWorld()->m_player;

	CRagEffect *effect = m_pPlayer->LaunchEffect(EF_POKJUK_SOUND);

	for (int i=0;i<160;i++)
		CRagEffect *effect = m_pPlayer->LaunchEffect(EF_POKJUK);

}

void CWeather :: LaunchNight( void )
{
	m_bNight = true;
}

void CWeather :: StopNight( void )
{
	m_bNight = false;
}

void	CRagEffect :: Level99(char *tName,char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt == 99999998)
	{
		m_stateCnt = 0;
		m_duration = 99999999;
	}

	if (m_stateCnt  == 0)
	{
		float param_angel = 55 ;
		float param_distance = 3.9f ;
		float param_height = 15.0f ;

		g_prim = LaunchEffectPrim(PP_3DCASTING);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);


		if (F1 == 1)
			g_prim->m_size=7;
		else
			g_prim->m_size=4;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = param_height;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 120;
		g_prim->m_GI[ec].distance = param_distance;
		g_prim->m_GI[ec].rise_angle=param_angel;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = param_height-2.0f;
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].alphaB = 120;
		g_prim->m_GI[ec].distance = param_distance+0.2f;
		g_prim->m_GI[ec].rise_angle=param_angel-5;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = param_height-4.0f;
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].alphaB = 120;
		g_prim->m_GI[ec].distance = param_distance+0.4f;
		g_prim->m_GI[ec].rise_angle=param_angel-10;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=3;
		g_prim->m_GI[ec].life = 0;
	}
}

void	CRagEffect :: Map_Aura(char* tName)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_3DCASTING);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = 15.0f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 50;
		g_prim->m_GI[ec].alphaT = 1;
		g_prim->m_GI[ec].distance = 12.9f;
		g_prim->m_GI[ec].rise_angle=55;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = 13.0f;
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].alphaB = 50;
		g_prim->m_GI[ec].alphaT = 1;
		g_prim->m_GI[ec].distance = 13.1f;
		g_prim->m_GI[ec].rise_angle=50;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = 11.0f;
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].alphaB = 50;
		g_prim->m_GI[ec].alphaT = 1;
		g_prim->m_GI[ec].distance = 13.3f;
		g_prim->m_GI[ec].rise_angle=45;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=3;
		g_prim->m_GI[ec].life = 0;
	}
}

void	CRagEffect :: GumGang(char F1,char F2)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_GUMGANG);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 5;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\super1.bmp", false);
		g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\super2.bmp", false);
		g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\super3.bmp", false);
		g_prim->m_texture[3] = g_texMgr.GetTexture("effect\\super4.bmp", false);
		g_prim->m_texture[4] = g_texMgr.GetTexture("effect\\super5.bmp", false);





		int ec;

		for(ec=0;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;

		for (ec=0;ec<F1;ec++)
		{
			float range,half,he;
			int tn;
			range=8;
			half=range/2;
			he=10;
			tn=5;

			g_prim->m_GI[ec].life = 1;

			g_prim->m_GI[ec].distance=3.5f;

			g_prim->m_GI[ec].vecB_pre.y=-(random(he));
			g_prim->m_GI[ec].height[0]=10;

			g_prim->m_GI[ec].height[1]=70;
			g_prim->m_GI[ec].height[2]=random(tn);
			g_prim->m_GI[ec].height[3]=random(360);

			g_prim->m_GI[ec].vecB_now.y=-(random(he));
			g_prim->m_GI[ec].height[4]=10;
			g_prim->m_GI[ec].height[5]=130;
			g_prim->m_GI[ec].height[6]=random(tn);
			g_prim->m_GI[ec].height[7]=random(360);

			g_prim->m_GI[ec].vecT_pre.y=-(random(he));
			g_prim->m_GI[ec].height[8]=10;
			g_prim->m_GI[ec].height[9]=190;
			g_prim->m_GI[ec].height[10]=random(tn);
			g_prim->m_GI[ec].height[11]=random(360);

			g_prim->m_GI[ec].vecT_now.y=-(random(he));
			g_prim->m_GI[ec].height[12]=10;
			g_prim->m_GI[ec].height[13]=250;
			g_prim->m_GI[ec].height[14]=random(tn);
			g_prim->m_GI[ec].height[15]=random(360);

			g_prim->m_GI[ec].flag1[4]=F2;
		}

	}
}

void	CRagEffect :: DoubleGumGang(char F1,char F2)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_DOUBLEGUMGANG);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 5;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\super1.bmp", false);
		g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\super2.bmp", false);
		g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\super3.bmp", false);
		g_prim->m_texture[3] = g_texMgr.GetTexture("effect\\super4.bmp", false);
		g_prim->m_texture[4] = g_texMgr.GetTexture("effect\\super5.bmp", false);

		g_prim->m_size=0;



		int ec;

		for(ec=0;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;

		for (ec=0;ec<F1;ec++)
		{
			float range,half,he;
			int tn;
			range=8;
			half=range/2;
			he=10;
			tn=5;

			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].life = 1;

			g_prim->m_GI[ec].distance=2.0f;

			g_prim->m_GI[ec].vecB_pre.y=-(random(he));
			g_prim->m_GI[ec].height[0]=10;

			g_prim->m_GI[ec].height[1]=40;
			g_prim->m_GI[ec].height[2]=random(tn);
			g_prim->m_GI[ec].height[3]=random(360);

			g_prim->m_GI[ec].vecB_now.y=-(random(he));
			g_prim->m_GI[ec].height[4]=10;
			g_prim->m_GI[ec].height[5]=80;
			g_prim->m_GI[ec].height[6]=random(tn);
			g_prim->m_GI[ec].height[7]=random(360);

			g_prim->m_GI[ec].vecT_pre.y=-(random(he));
			g_prim->m_GI[ec].height[8]=10;
			g_prim->m_GI[ec].height[9]=120;
			g_prim->m_GI[ec].height[10]=random(tn);
			g_prim->m_GI[ec].height[11]=random(360);

			g_prim->m_GI[ec].vecT_now.y=-(random(he));
			g_prim->m_GI[ec].height[12]=10;
			g_prim->m_GI[ec].height[13]=160;
			g_prim->m_GI[ec].height[14]=random(tn);
			g_prim->m_GI[ec].height[15]=random(360);

			g_prim->m_GI[ec].flag1[4]=F2;
		}

	}
}

void	CRagEffect :: Map_MagicZone(char *tName, char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_MAPMAGICZONE);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		if (F1 == 4 || F1 == 14)
			g_prim->m_size=111;
		else if (F1 == 5)
			g_prim->m_size=112;
		else
			g_prim->m_size=110;




		g_prim->m_GI[0].life = 1;
		g_prim->m_GI[1].life = 0;
		g_prim->m_GI[2].life = 0;
		g_prim->m_GI[3].life = 0;

		if ( F1 >=2 && F1 < 4 )
			g_prim->m_GI[0].distance = 40.0f;
		else if (F1 == 4 || F1 == 14)
		{
			if (m_param[0] <= 0)
				g_prim->m_GI[0].distance = 40.0f;
			else
				g_prim->m_GI[0].distance = m_param[0];
		}
		else
			g_prim->m_GI[0].distance = 30.0f;

		if ( F1 == 4 || F1 == 5 || F1 == 14 )
		{
			g_prim->m_GI[0].RotStart = 0;
			g_prim->m_GI[0].alphaB = 250;
			g_prim->m_GI[0].height[0]=-0.5f;
		}
		else if ( F1 == 0 || F1 == 2 )
		{
			g_prim->m_GI[0].RotStart = 0;
			g_prim->m_GI[0].alphaB = 200;
			g_prim->m_GI[0].height[0]=-0.5f;
		}
		else
		{
			g_prim->m_GI[0].RotStart = 358;
			g_prim->m_GI[0].alphaB = 50;
			g_prim->m_GI[0].height[0]=-0.4f;
		}

		g_prim->m_GI[0].height[4] = F1;

	}
}

void	CRagEffect :: Map_Pika(char *tName)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_3DAURA);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=4;




		g_prim->m_GI[0].life = 1;
		g_prim->m_GI[1].life = 1;
		g_prim->m_GI[2].life = 0;
		g_prim->m_GI[3].life = 0;

		g_prim->m_GI[0].distance = 46.0f;
		g_prim->m_GI[1].distance = 46.0f;

		g_prim->m_GI[0].rise_angle = 0;
		g_prim->m_GI[1].rise_angle = 180;

		g_prim->m_GI[0].alphaB = 25;
		g_prim->m_GI[1].alphaB = 25;

		g_prim->m_GI[0].height[0]=-1.0f;
		g_prim->m_GI[1].height[0]=-1.0f;

	}
}

void	CRagEffect :: Level99_2(char *tName, char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt == 99999998)
	{
		m_stateCnt = 0;
		m_duration = 99999999;
	}

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_3DAURA);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		if (F1 == 1)
			g_prim->m_size=15;
		else
			g_prim->m_size=4;




		g_prim->m_GI[0].life = 1;
		g_prim->m_GI[1].life = 1;
		g_prim->m_GI[2].life = 0;
		g_prim->m_GI[3].life = 0;

		g_prim->m_GI[0].distance = 15.0f;
		g_prim->m_GI[1].distance = 15.0f;

		g_prim->m_GI[0].rise_angle = 0;
		g_prim->m_GI[1].rise_angle = 180;

		g_prim->m_GI[0].alphaB = 200;
		g_prim->m_GI[1].alphaB = 200;

		g_prim->m_GI[0].height[0]=-1.0f;
		g_prim->m_GI[1].height[0]=-1.0f;

	}
}

void	CRagEffect :: GroundShake(char *tName, int minus, char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_GROUNDSHAKE);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=4;
		if (F1 == 1)
			g_prim->m_size=101;
		else if (F1 == 2)
			g_prim->m_size=102;





		g_prim->m_GI[0].life = 1;
		g_prim->m_GI[0].process= minus;
		g_prim->m_GI[0].distance = 15.0f;

		g_prim->m_GI[0].rise_angle = 0;

		g_prim->m_GI[0].alphaB = 0;

		g_prim->m_GI[0].height[0]=-5.0f;

		for (int ec=1; ec<4; ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

	}
}

void	CRagEffect :: Map_Particle(char *tName, char F1, char F2)
{

	float ph;
	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_MAPPARTICLE);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;

		ph = F1 * 2;

		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].max_height=0.15f;

			g_prim->m_GI[ec].distance=1.0f;


			g_prim->m_GI[ec].height[0]=random(360);
			g_prim->m_GI[ec].height[1]=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[3]=random(360);


			g_prim->m_GI[ec].vecB_pre.x=0;
			g_prim->m_GI[ec].vecB_pre.y=-ph;
			g_prim->m_GI[ec].vecB_pre.z=0;
			g_prim->m_GI[ec].height[4]=random(360);


			g_prim->m_GI[ec].vecB_now.x=0;
			g_prim->m_GI[ec].vecB_now.y=-ph;
			g_prim->m_GI[ec].vecB_now.z=0;
			g_prim->m_GI[ec].height[5]=random(360);


			g_prim->m_GI[ec].vecT_pre.x=0;
			g_prim->m_GI[ec].vecT_pre.y=-ph;
			g_prim->m_GI[ec].vecT_pre.z=0;
			g_prim->m_GI[ec].height[6]=random(360);


			g_prim->m_GI[ec].vecT_now.x=0;
			g_prim->m_GI[ec].vecT_now.y=-ph;
			g_prim->m_GI[ec].vecT_now.z=0;
			g_prim->m_GI[ec].height[7]=random(360);

			g_prim->m_GI[ec].flag1[0]=1;

			if (F2 == 2)
				g_prim->m_GI[ec].flag1[1]=2;
			else
				g_prim->m_GI[ec].flag1[1]=0;
		}
	}
}

void	CRagEffect :: Level99_3(char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt == 99999998)
	{
		m_stateCnt = 0;
		m_duration = 99999999;
	}

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_3DAURA_2);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		if (F1 == 3)
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ghost.bmp", false);
		else if (F1 >= 1)
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\whitelight.tga", false);
		else
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\freezing_circle.bmp", false);


		g_prim->m_size=0;

		if (F1 == 2)
			g_prim->m_size=7;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			if (F1 == 3 && ec >= 1)
				g_prim->m_GI[ec].life = 0;
			else
				g_prim->m_GI[ec].life = 1;

			g_prim->m_GI[ec].max_height=0.15f;
			if (F1 == 1)
				g_prim->m_GI[ec].distance=2.4f;
			else if (F1 == 3)
			{
				g_prim->m_GI[ec].distance=3.2f;

				g_prim->m_GI[ec].max_height=0.6f;
			}
			else
				g_prim->m_GI[ec].distance=0.8f;

			if (F1 == 3)
			{
				g_prim->m_GI[ec].vecB_pre.x=random(15)-8;
				g_prim->m_GI[ec].vecB_pre.z=random(15)-8;
			}
			else
			{
				g_prim->m_GI[ec].vecB_pre.x=0;
				g_prim->m_GI[ec].vecB_pre.z=0;
			}
			g_prim->m_GI[ec].vecB_pre.y=random(100);
			g_prim->m_GI[ec].height[0]=random(360);
			g_prim->m_GI[ec].height[1]=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[3]=random(360);

			if (F1 == 3)
			{
				g_prim->m_GI[ec].vecB_now.x=random(15)-8;
				g_prim->m_GI[ec].vecB_now.z=random(15)-8;
			}
			else
			{
				g_prim->m_GI[ec].vecB_now.x=0;
				g_prim->m_GI[ec].vecB_now.z=0;
			}
			g_prim->m_GI[ec].vecB_now.y=random(100);
			g_prim->m_GI[ec].height[4]=random(360);
			g_prim->m_GI[ec].height[5]=random(360);
			g_prim->m_GI[ec].height[6]=random(360);
			g_prim->m_GI[ec].height[7]=random(360);

			if (F1 == 3)
			{
				g_prim->m_GI[ec].vecT_pre.x=random(15)-8;
				g_prim->m_GI[ec].vecT_pre.z=random(15)-8;
			}
			else
			{
				g_prim->m_GI[ec].vecT_pre.x=0;
				g_prim->m_GI[ec].vecT_pre.z=0;
			}
			g_prim->m_GI[ec].vecT_pre.y=random(100);
			g_prim->m_GI[ec].height[8]=random(360);
			g_prim->m_GI[ec].height[9]=random(360);
			g_prim->m_GI[ec].height[10]=random(360);
			g_prim->m_GI[ec].height[11]=random(360);

			if (F1 == 3)
			{
				g_prim->m_GI[ec].vecT_now.x=random(15)-8;
				g_prim->m_GI[ec].vecT_now.z=random(15)-8;
			}
			else
			{
				g_prim->m_GI[ec].vecT_now.x=0;
				g_prim->m_GI[ec].vecT_now.z=0;
			}
			g_prim->m_GI[ec].vecT_now.y=random(100);
			g_prim->m_GI[ec].height[12]=random(360);
			g_prim->m_GI[ec].height[13]=random(360);
			g_prim->m_GI[ec].height[14]=random(360);
			g_prim->m_GI[ec].height[15]=random(360);

			if (F1 == 1)
				g_prim->m_GI[ec].flag1[0]=1;
			else if (F1 == 3)
				g_prim->m_GI[ec].flag1[0]=11;
			else
				g_prim->m_GI[ec].flag1[0]=0;
		}
	}
}

void	CRagEffect :: SoulParticle(char *tName)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_SOULPARTICLE);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\whitelight.tga", false);

		g_prim->m_size=0;



		int ec;

		for (ec=0;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;

		for (ec=0;ec<2;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].max_height=0.07f;
			g_prim->m_GI[ec].distance=1.0f+(random(11)*0.1f);
			g_prim->m_GI[ec].process = 0;




			g_prim->m_GI[ec].vecB_pre.x=m_deltaPos.x;
			g_prim->m_GI[ec].vecB_pre.y=m_deltaPos.y;
			g_prim->m_GI[ec].vecB_pre.z=m_deltaPos.z;
			g_prim->m_GI[ec].height[0]=random(360);
			g_prim->m_GI[ec].height[1]=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[3]=random(360);

			g_prim->m_GI[ec].height[12]=0;

			g_prim->m_GI[ec].vecB_now.x=m_deltaPos.x;
			g_prim->m_GI[ec].vecB_now.y=m_deltaPos.y;
			g_prim->m_GI[ec].vecB_now.z=m_deltaPos.z;
			g_prim->m_GI[ec].height[4]=random(360);
			g_prim->m_GI[ec].height[5]=random(360);
			g_prim->m_GI[ec].height[6]=random(360);
			g_prim->m_GI[ec].height[7]=random(360);

			g_prim->m_GI[ec].height[13]=0;

			g_prim->m_GI[ec].vecT_pre.x=m_deltaPos.x;
			g_prim->m_GI[ec].vecT_pre.y=m_deltaPos.y;
			g_prim->m_GI[ec].vecT_pre.z=m_deltaPos.z;
			g_prim->m_GI[ec].height[8]=random(360);
			g_prim->m_GI[ec].height[9]=random(360);
			g_prim->m_GI[ec].height[10]=random(360);
			g_prim->m_GI[ec].height[11]=random(360);

			g_prim->m_GI[ec].height[14]=0;

			g_prim->m_GI[ec].height[15]=0;
		}
	}
}

void	CRagEffect :: SoulLinkLight(char *tName)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_SOULLINK);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		for (ec=0;ec<1;ec++)
		{
			int aa;

			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;

			g_prim->m_GI[ec].distance=2.5f;



			g_prim->m_GI[ec].height[10]=m_deltaPos.y;

			g_prim->m_GI[ec].height[13]=0;
			g_prim->m_GI[ec].height[14]=0;
			g_prim->m_GI[ec].height[15]=0;


			aa=ec*30;
			g_prim->m_GI[ec].height[0]=random(360);
			g_prim->m_GI[ec].height[1]=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[3]=random(360);
			g_prim->m_GI[ec].vecT_now.x = aa;
			g_prim->m_GI[ec].vecB_pre.x=-g_sinTable[(aa/2)+90]*30;
			g_prim->m_GI[ec].vecB_pre.y=-(g_sinTable[aa]*20)-40;
			g_prim->m_GI[ec].vecB_pre.z=0;

		}
	}
}

bool	CEffectPrim :: Prim3DAura()
{

	int ec;

	m_pos = m_master->m_pos;



	for (ec=0;ec<2;ec++)
	{
		m_GI[ec].rise_angle+=3;
		if (m_GI[ec].rise_angle>=360)
			m_GI[ec].rise_angle-=360;
	}

	if (!AnimTexture())
		return false;

	return true;
}

bool	CEffectPrim :: PrimMapMagicZone()
{

	int ec;

	m_pos = m_master->m_pos;



	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].height[4] == 14)
		{
			m_GI[ec].RotStart-=1;
			if (m_GI[ec].RotStart<0)
				m_GI[ec].RotStart+=360;
		}
		else
		{
			m_GI[ec].RotStart+=1;
			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;
		}
	}

	if (!AnimTexture())
		return false;

	return true;
}

void	CEffectPrim :: PrimGroundShake()
{

	int ec;

	m_pos = m_master->m_pos;



	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life > 0)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].process<=10)
				{
					m_GI[ec].alphaB+=2;
				}

				m_GI[ec].distance *= 1.07f;

				if (m_GI[ec].process>10)
				{
					m_GI[ec].alphaB--;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}
			}
		}
	}
}

bool	CEffectPrim :: PrimLandProtector()
{

	int ec;

	m_pos = m_master->m_pos;



	for (ec=0;ec<2;ec++)
	{
		if (m_GI[ec].height[4] == 2)
			m_GI[ec].rise_angle+=1;
		else
			m_GI[ec].rise_angle+=3;
		if (m_GI[ec].rise_angle>=360)
			m_GI[ec].rise_angle-=360;
	}

	return true;
}

bool	CEffectPrim :: PrimHermode()
{

	int ec;

	m_pos = m_master->m_pos;



	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].alphaB<140)
			m_GI[ec].alphaB+=2;

		m_GI[ec].rise_angle+=1;
		if (m_GI[ec].rise_angle>=360)
			m_GI[ec].rise_angle-=360;

		m_GI[ec].RotStart++;
		if (m_GI[ec].RotStart>=360)
			m_GI[ec].RotStart-=360;
	}

	return true;
}

bool	CEffectPrim :: PrimFoot()
{

	int ec;

	m_pos = m_master->m_pos;



	for (ec=0;ec<1;ec++)
	{
		m_GI[ec].process++;

		if (m_GI[ec].process>0)
		{
			if (m_GI[ec].process%2>=1)
				m_GI[ec].alphaB--;
			if (m_GI[ec].alphaB<0)
				m_GI[ec].alphaB=0;
		}
	}

	return true;
}

bool	CEffectPrim :: Prim3DAura_2()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;




	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		int i;
		if (m_GI[ec].life == 1)
		{

			for (i=0;i<16;i+=2)
			{
				if (m_GI[ec].height[i+1] > m_GI[ec].height[i])
				{
					m_GI[ec].height[i]+=3;
					if (m_GI[ec].height[i] > 359)
						m_GI[ec].height[i] = 359;
					if (m_GI[ec].height[i] > m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = random(int(m_GI[ec].height[i]));
				}
				else
				{
					m_GI[ec].height[i]-=2;
					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
					if (m_GI[ec].height[i] <= m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = 359 - random(int(m_GI[ec].height[i]));
				}
			}

			if (m_GI[ec].vecB_pre.y < 0)
			{
				if (m_GI[ec].flag1[0] == 11)
				{
					m_GI[ec].vecB_pre.x += 0.15f * g_sinTable[int(m_GI[ec].height[0])];
					m_GI[ec].vecB_pre.z += 0.15f * g_sinTable[int(m_GI[ec].height[2])];
				}
				else
				{
					m_GI[ec].vecB_pre.x += 0.15f * g_sinTable[int(m_GI[ec].height[0])];
					m_GI[ec].vecB_pre.z += 0.15f * g_sinTable[int(m_GI[ec].height[2])];
				}

				if (m_GI[ec].flag1[0] == 11)
				{
					if (m_GI[ec].vecB_pre.y<-50)
					{
						m_GI[ec].vecB_pre.x=random(15)-8;
						m_GI[ec].vecB_pre.z=random(15)-8;
						m_GI[ec].vecB_pre.y=random(100);
					}
				}
				else
				{
					if (m_GI[ec].vecB_pre.y<-30)
					{
						m_GI[ec].vecB_pre.x=0;
						m_GI[ec].vecB_pre.z=0;
						m_GI[ec].vecB_pre.y=random(100);
					}
				}
			}
			m_GI[ec].vecB_pre.y -= m_GI[ec].max_height;

			if (m_GI[ec].vecB_now.y < 0)
			{
				m_GI[ec].vecB_now.x -= 0.15f * g_sinTable[int(m_GI[ec].height[4])];
				m_GI[ec].vecB_now.z -= 0.15f * g_sinTable[int(m_GI[ec].height[6])];

				if (m_GI[ec].vecB_now.y<-30)
				{
					if (m_GI[ec].flag1[0] == 11)
					{
						m_GI[ec].vecB_now.x=random(15)-8;
						m_GI[ec].vecB_now.z=random(15)-8;
					}
					else
					{
						m_GI[ec].vecB_now.x=0;
						m_GI[ec].vecB_now.z=0;
					}
					m_GI[ec].vecB_now.y=random(100);
				}
			}
			m_GI[ec].vecB_now.y -= m_GI[ec].max_height;

			if (m_GI[ec].vecT_pre.y < 0)
			{
				m_GI[ec].vecT_pre.x += 0.15f * g_sinTable[int(m_GI[ec].height[8])];
				m_GI[ec].vecT_pre.z -= 0.15f * g_sinTable[int(m_GI[ec].height[10])];

				if (m_GI[ec].vecT_pre.y<-30)
				{
					if (m_GI[ec].flag1[0] == 11)
					{
						m_GI[ec].vecT_pre.x=random(15)-8;
						m_GI[ec].vecT_pre.z=random(15)-8;
					}
					else
					{
						m_GI[ec].vecT_pre.x=0;
						m_GI[ec].vecT_pre.z=0;
					}

					m_GI[ec].vecT_pre.y=random(100);
				}
			}
			m_GI[ec].vecT_pre.y -= m_GI[ec].max_height;

			if (m_GI[ec].vecT_now.y < 0)
			{
				m_GI[ec].vecT_now.x -= 0.15f * g_sinTable[int(m_GI[ec].height[12])];
				m_GI[ec].vecT_now.z += 0.15f * g_sinTable[int(m_GI[ec].height[14])];

				if (m_GI[ec].vecT_now.y<-30)
				{
					if (m_GI[ec].flag1[0] == 11)
					{
						m_GI[ec].vecT_now.x=random(15)-8;
						m_GI[ec].vecT_now.z=random(15)-8;
					}
					else
					{
						m_GI[ec].vecT_now.x=0;
						m_GI[ec].vecT_now.z=0;
					}

					m_GI[ec].vecT_now.y=random(100);
				}
			}
			m_GI[ec].vecT_now.y -= m_GI[ec].max_height;
		}
	}

	return true;
}

bool	CEffectPrim :: PrimMapParticle()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;


	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		int i;
		if (m_GI[ec].life == 1)
		{

			for (i=0;i<4;i+=2)
			{
				if (m_GI[ec].height[i+1] > m_GI[ec].height[i])
				{
					m_GI[ec].height[i]+=1;
					if (m_GI[ec].height[i] > 359)
						m_GI[ec].height[i] = 359;
					if (m_GI[ec].height[i] > m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = random(int(m_GI[ec].height[i]));
				}
				else
				{
					m_GI[ec].height[i]-=1;
					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
					if (m_GI[ec].height[i] <= m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = 359 - random(int(m_GI[ec].height[i]));
				}
			}

			for (i=4;i<8;i++)
			{
				m_GI[ec].height[i]-=1;
				if (m_GI[ec].height[i]<0)
					m_GI[ec].height[i]+=360;
			}

			m_GI[ec].vecB_pre.x = g_sinTable[int(m_GI[ec].height[4])] * 20.0f;
			m_GI[ec].vecB_pre.z = g_cosTable[int(m_GI[ec].height[4])] * 20.0f;

			m_GI[ec].vecB_now.x = g_sinTable[int(m_GI[ec].height[5])] * 20.0f;
			m_GI[ec].vecB_now.z = g_cosTable[int(m_GI[ec].height[5])] * 20.0f;

			m_GI[ec].vecT_pre.x = g_sinTable[int(m_GI[ec].height[6])] * 20.0f;
			m_GI[ec].vecT_pre.z = g_cosTable[int(m_GI[ec].height[6])] * 20.0f;

			m_GI[ec].vecT_now.x = g_sinTable[int(m_GI[ec].height[7])] * 20.0f;
			m_GI[ec].vecT_now.z = g_cosTable[int(m_GI[ec].height[7])] * 20.0f;

			if (m_GI[ec].vecB_pre.y < 0)
			{
				m_GI[ec].vecB_pre.x += 2.5f * g_sinTable[int(m_GI[ec].height[0])];
				m_GI[ec].vecB_pre.z += 2.5f * g_sinTable[int(m_GI[ec].height[2])];
			}

			if (m_GI[ec].vecB_now.y < 0)
			{
				m_GI[ec].vecB_now.x -= 2.5f * g_sinTable[int(m_GI[ec].height[0])];
				m_GI[ec].vecB_now.z -= 2.5f * g_sinTable[int(m_GI[ec].height[2])];
			}

			if (m_GI[ec].vecT_pre.y < 0)
			{
				m_GI[ec].vecT_pre.x += 2.5f * g_sinTable[int(m_GI[ec].height[0])];
				m_GI[ec].vecT_pre.z -= 2.5f * g_sinTable[int(m_GI[ec].height[2])];
			}

			if (m_GI[ec].vecT_now.y < 0)
			{
				m_GI[ec].vecT_now.x -= 2.5f * g_sinTable[int(m_GI[ec].height[0])];
				m_GI[ec].vecT_now.z += 2.5f * g_sinTable[int(m_GI[ec].height[2])];
			}
		}
	}

	return true;
}

bool	CEffectPrim :: PrimSoulParticle()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;




	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		int i;
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			for (i=0;i<12;i+=2)
			{
				if (m_GI[ec].height[i+1] > m_GI[ec].height[i])
				{
					m_GI[ec].height[i]+=3;
					if (m_GI[ec].height[i] > 359)
						m_GI[ec].height[i] = 359;
					if (m_GI[ec].height[i] > m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = random(int(m_GI[ec].height[i]));
				}
				else
				{
					m_GI[ec].height[i]-=2;
					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
					if (m_GI[ec].height[i] <= m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = 359 - random(int(m_GI[ec].height[i]));
				}
			}

			for (i=12;i<15;i++)
			{
				if (m_GI[ec].process<10)
					m_GI[ec].height[i]+=20;
				else if (m_GI[ec].process>30)
				{
					m_GI[ec].height[i]-=3;
					if (m_GI[ec].height[i]<0)
						m_GI[ec].height[i]=0;
				}
			}

			if (m_GI[ec].vecB_pre.y < 0)
			{
				m_GI[ec].vecB_pre.x += m_GI[ec].max_height * g_sinTable[int(m_GI[ec].height[0])];
				m_GI[ec].vecB_pre.z += m_GI[ec].max_height * g_sinTable[int(m_GI[ec].height[2])];

			}
			m_GI[ec].vecB_pre.y -= m_GI[ec].max_height;

			if (m_GI[ec].vecB_now.y < 0)
			{
				m_GI[ec].vecB_now.x -= m_GI[ec].max_height * g_sinTable[int(m_GI[ec].height[4])];
				m_GI[ec].vecB_now.z -= m_GI[ec].max_height * g_sinTable[int(m_GI[ec].height[6])];

			}
			{
				float y = m_GI[ec].vecB_now.y;
				m_GI[ec].vecB_now.y = y + m_GI[ec].max_height;
			}

			if (m_GI[ec].vecT_pre.y < 0)
			{
				m_GI[ec].vecT_pre.x += m_GI[ec].max_height * 1.2f * g_sinTable[int(m_GI[ec].height[8])];
				m_GI[ec].vecT_pre.z -= m_GI[ec].max_height * 1.2f * g_sinTable[int(m_GI[ec].height[10])];
			}

		}
	}

	return true;
}

bool	CEffectPrim :: PrimSoulLinkLight()
{

	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;




	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		int aa;

		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;
			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].process<=10)
				{
					m_GI[ec].height[13]+=20;
				}
				else if (m_GI[ec].process>170)
				{
					m_GI[ec].height[13]-=5;
					if (m_GI[ec].height[13]<0)
						m_GI[ec].height[13]=0;
				}

				{
					m_GI[ec].vecT_now.x+=(float)(m_GI[ec].process/10.0f/3.0f);


					if (m_GI[ec].vecT_now.x>=359)
						m_GI[ec].vecT_now.x=359;

					aa=m_GI[ec].vecT_now.x;

					m_GI[ec].vecB_pre.x=-30+(float)(aa/6.0f);
					m_GI[ec].vecB_pre.y=-(g_sinTable[aa]*20)-40;
					m_GI[ec].vecB_pre.z=0;
				}
			}
		}
	}

	return true;
}

bool	CEffectPrim :: PrimPeongUp()
{

	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;




	int ec;



	for (ec=0;ec<4;ec++)
	{
		int i;
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].process<30)
					m_GI[ec].alphaB+=5;
				else
					m_GI[ec].alphaB-=2;

				if (m_GI[ec].alphaB < 0)
					m_GI[ec].alphaB = 0;

				for (i=0;i<4;i+=2)
				{
					if (m_GI[ec].height[i+1] > m_GI[ec].height[i])
					{
						m_GI[ec].height[i]+=3;
						if (m_GI[ec].height[i] > 359)
							m_GI[ec].height[i] = 359;
						if (m_GI[ec].height[i] > m_GI[ec].height[i+1])
							m_GI[ec].height[i+1] = random(int(m_GI[ec].height[i]));
					}
					else
					{
						m_GI[ec].height[i]-=2;
						if (m_GI[ec].height[i] < 0)
							m_GI[ec].height[i] = 0;
						if (m_GI[ec].height[i] <= m_GI[ec].height[i+1])
							m_GI[ec].height[i+1] = 359 - random(int(m_GI[ec].height[i]));
					}
				}

				if (m_GI[ec].vecB_pre.y < 0)
				{


					m_GI[ec].vecB_pre.x += 0.03f * g_sinTable[int(m_GI[ec].height[0])];
					m_GI[ec].vecB_pre.z += 0.03f * g_sinTable[int(m_GI[ec].height[2])];
				}
				m_GI[ec].vecB_pre.y -= m_GI[ec].max_height;
			}
		}
	}

	return true;
}

bool	CEffectPrim :: PrimPeong()
{

	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;




	int ec;
	int angle;
	float cs1,sn1;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].process<=90)

				{
					cs1 = g_cosTable[m_GI[ec].rise_angle];
					sn1 = g_sinTable[m_GI[ec].rise_angle];

					m_GI[ec].vecB_pre.x+=cs1*m_GI[ec].max_height;
					m_GI[ec].vecB_pre.z+=sn1*m_GI[ec].max_height;

					angle=m_GI[ec].process*2;

					if (angle>=360)
						angle-=360;
					sn1 = g_sinTable[angle];
					m_GI[ec].vecB_pre.y=m_GI[ec].vecB_now.y-(sn1*m_GI[ec].max_height*50);
				}

				if (m_GI[ec].process<=25)
					m_GI[ec].alphaB+=2;

				if (m_GI[ec].process>70)
					m_GI[ec].alphaB-=1;

				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}
		}
	}

	return true;
}

void	CEffectPrim :: PrimWaterFallParticle()
{

	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;




	int ec;



	for (ec=0;ec<4;ec++)
	{
		int i;
		if (m_GI[ec].life == 1)
		{
			for (i=0;i<16;i+=2)
			{
				if (m_GI[ec].height[i+1] > m_GI[ec].height[i])
				{
					m_GI[ec].height[i]+=1;
					if (m_GI[ec].height[i] > 359)
						m_GI[ec].height[i] = 359;
					if (m_GI[ec].height[i] > m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = random(int(m_GI[ec].height[i]));
				}
				else
				{
					m_GI[ec].height[i]-=1;
					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
					if (m_GI[ec].height[i] <= m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = 359 - random(int(m_GI[ec].height[i]));
				}
			}

			if (m_GI[ec].vecB_pre.y < 0)
			{
				m_GI[ec].vecB_pre.x += 0.01f * g_sinTable[int(m_GI[ec].height[0])];
				m_GI[ec].vecB_pre.z += 0.01f * g_sinTable[int(m_GI[ec].height[2])];

				if (m_GI[ec].vecB_pre.y<-8)
				{
					if (m_GI[ec].flag1[12] == 0)
						m_GI[ec].vecB_pre.x=random(45)-22;
					else
						m_GI[ec].vecB_pre.x=random(25)-12;
					m_GI[ec].vecB_pre.y=random(30);
					m_GI[ec].vecB_pre.z=random(11)-5;
					if (m_GI[ec].flag1[11] == 1)
					{
						int temp;
						temp = m_GI[ec].vecB_pre.x;
						m_GI[ec].vecB_pre.x = m_GI[ec].vecB_pre.z;
						m_GI[ec].vecB_pre.z = temp;
					}
				}
			}
			m_GI[ec].vecB_pre.y -= m_GI[ec].max_height;

			if (m_GI[ec].vecB_now.y < 0)
			{
				m_GI[ec].vecB_now.x -= 0.01f * g_sinTable[int(m_GI[ec].height[4])];
				m_GI[ec].vecB_now.z -= 0.01f * g_sinTable[int(m_GI[ec].height[6])];

				if (m_GI[ec].vecB_now.y<-8)
				{
					if (m_GI[ec].flag1[12] == 0)
						m_GI[ec].vecB_now.x=random(45)-22;
					else
						m_GI[ec].vecB_now.x=random(25)-12;
					m_GI[ec].vecB_now.y=random(30);
					m_GI[ec].vecB_now.z=random(11)-5;
					if (m_GI[ec].flag1[11] == 1)
					{
						int temp;
						temp = m_GI[ec].vecB_now.x;
						m_GI[ec].vecB_now.x = m_GI[ec].vecB_now.z;
						m_GI[ec].vecB_now.z = temp;
					}
				}
			}
			m_GI[ec].vecB_now.y -= m_GI[ec].max_height;

			if (m_GI[ec].vecT_pre.y < 0)
			{
				m_GI[ec].vecT_pre.x += 0.01f * g_sinTable[int(m_GI[ec].height[8])];
				m_GI[ec].vecT_pre.z -= 0.01f * g_sinTable[int(m_GI[ec].height[10])];

				if (m_GI[ec].vecT_pre.y<-8)
				{
					if (m_GI[ec].flag1[12] == 0)
						m_GI[ec].vecT_pre.x=random(45)-22;
					else
						m_GI[ec].vecT_pre.x=random(25)-12;
					m_GI[ec].vecT_pre.y=random(30);
					m_GI[ec].vecT_pre.z=random(11)-5;
					if (m_GI[ec].flag1[11] == 1)
					{
						int temp;
						temp = m_GI[ec].vecT_pre.x;
						m_GI[ec].vecT_pre.x = m_GI[ec].vecT_pre.z;
						m_GI[ec].vecT_pre.z = temp;
					}
				}
			}
			m_GI[ec].vecT_pre.y -= m_GI[ec].max_height;

			if (m_GI[ec].vecT_now.y < 0)
			{
				m_GI[ec].vecT_now.x -= 0.01f * g_sinTable[int(m_GI[ec].height[12])];
				m_GI[ec].vecT_now.z += 0.01f * g_sinTable[int(m_GI[ec].height[14])];

				if (m_GI[ec].vecT_now.y<-8)
				{
					if (m_GI[ec].flag1[12] == 0)
						m_GI[ec].vecT_now.x=random(45)-22;
					else
						m_GI[ec].vecT_now.x=random(25)-12;
					m_GI[ec].vecT_now.y=random(30);
					m_GI[ec].vecT_now.z=random(11)-5;
					if (m_GI[ec].flag1[11] == 1)
					{
						int temp;
						temp = m_GI[ec].vecT_now.x;
						m_GI[ec].vecT_now.x = m_GI[ec].vecT_now.z;
						m_GI[ec].vecT_now.z = temp;
					}
				}
			}
			m_GI[ec].vecT_now.y -= m_GI[ec].max_height;
		}
	}
}

bool	CEffectPrim :: PrimGumGang()
{

	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;



	int ec;
	float range,half,he,tn;
	range=8;
	half=range/2;
	he=8;
	tn=5;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		int i;
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			for (i=0;i<=12;i+=4)
			{
				m_GI[ec].height[i]++;

				if (m_GI[ec].height[i]<10)
				{
					m_GI[ec].height[i+1]+=25;
				}
				else
				{
					m_GI[ec].height[i+1]-=12;

					if (m_GI[ec].height[i+1]<12)
					{
						m_GI[ec].height[i]=0;
						m_GI[ec].height[i+1]=0;
						m_GI[ec].height[i+3]=random(360);
						m_GI[ec].height[i+2]=random(tn);
						if (i==0)
						{
							m_GI[ec].vecB_pre.y=-random(he)-3;
						}
						else if (i==4)
						{
							m_GI[ec].vecB_now.y=-random(he)-3;
						}
						else if (i==8)
						{
							m_GI[ec].vecT_pre.y=-random(he)-3;
						}
						else
						{
							m_GI[ec].vecT_now.y=-random(he)-3;
						}
					}
				}
			}

		}
	}

	return true;
}

void	CEffectPrim :: Render3DAura(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;

	int Angle;
	float Rx;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
				if (m_size==100 || m_size>=110)
					Angle = m_GI[ec].RotStart;
				else if (m_size!=0)
					Angle=ec*23;
				else
					Angle = m_GI[ec].RotStart;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				Rx=m_GI[ec].distance*0.1f*(g_sinTable[m_GI[ec].rise_angle]+1);

				m_GI[ec].vecB_pre.x=cs1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecB_pre.z=sn1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecB_pre.y=m_GI[ec].height[0];

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				m_GI[ec].vecB_now.x=cs1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecB_now.z=sn1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecB_now.y=m_GI[ec].height[0];

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				m_GI[ec].vecT_now.x=cs1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecT_now.z=sn1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecT_now.y=m_GI[ec].height[0];

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				m_GI[ec].vecT_pre.x=cs1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecT_pre.z=sn1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecT_pre.y=m_GI[ec].height[0];

				if (m_size == 101)
				{
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm,0,0,0,255,0,0);
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm,0,0,0,255,0,0);
				}
				else if (m_size == 102)
				{
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm,0,0,0,255,255,0);
				}
				else if (m_size == 111)
				{
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm,0,0,0,55,55,255);
				}
				else if (m_size == 112)
				{
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm,0,0,0,105,225,105);
				}
				else if (m_size == 100)
				{
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm,0,0,0,255,255,255);
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm,0,0,0,255,255,255);
				}
				else if (m_size == 110)
				{
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm,1,0,0,255,255,255);
				}
				else if (m_size == 15)
				{
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm,0,0,0,35,255,35);
				}
				else if (m_size != 0)
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm,0);
				else
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm,1);
		}
	}
}

void	CEffectPrim :: RenderHermode(matrix &wtm)
{
	float		cs1, sn1 , snA;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;

	int Angle;
	float Rx;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
				Angle=m_GI[ec].RotStart;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				snA  = sn1*2.0f;

				Rx=m_GI[ec].distance*0.1f*(g_sinTable[m_GI[ec].rise_angle]+1);

				m_GI[ec].vecB_pre.x=cs1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecB_pre.z=sn1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecB_pre.y=m_GI[ec].height[0]+snA;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				m_GI[ec].vecB_now.x=cs1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecB_now.z=sn1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecB_now.y=m_GI[ec].height[0]+snA;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				m_GI[ec].vecT_now.x=cs1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecT_now.z=sn1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecT_now.y=m_GI[ec].height[0]+snA;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				m_GI[ec].vecT_pre.x=cs1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecT_pre.z=sn1*( m_GI[ec].distance*0.8 + Rx );
				m_GI[ec].vecT_pre.y=m_GI[ec].height[0]+snA;

				vec1 = m_GI[ec].vecB_pre;
				vec1.y -= m_GI[ec].distance;
				vec2 = m_GI[ec].vecB_now;
				vec2.y -= m_GI[ec].distance;
				vec3 = m_GI[ec].vecT_pre;
				vec3.y -= m_GI[ec].distance;
				vec4 = m_GI[ec].vecT_now;
				vec4.y -= m_GI[ec].distance;

				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm,0,0,0,m_GI[ec].flag1[0],m_GI[ec].flag1[0],250);
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,vec2,vec1,wtm,0,0,0,m_GI[ec].flag1[0],m_GI[ec].flag1[0],220);
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_now,m_GI[ec].vecT_now,vec4,vec2,wtm,0,0,0,m_GI[ec].flag1[0],m_GI[ec].flag1[0],190);
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecT_now,m_GI[ec].vecT_pre,vec3,vec4,wtm,0,0,0,m_GI[ec].flag1[0],m_GI[ec].flag1[0],130);
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecT_pre,m_GI[ec].vecB_pre,vec1,vec3,wtm,0,0,0,m_GI[ec].flag1[0],m_GI[ec].flag1[0],160);
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec4,vec3,wtm,0,0,0,m_GI[ec].flag1[0],m_GI[ec].flag1[0],250);
		}
	}
}

void	CEffectPrim :: Render3DAura_2(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vec5, vec6, vec7, vec8;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int count;
	int Angle;
	float Rx;
	int ec;

	Angle = 45;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			Rx=m_GI[ec].distance;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec1.x=cs1*Rx;
			vec1.z=sn1*Rx;
			vec1.y=0;

			cs1 = g_cosTable[Angle+90];
			sn1 = g_sinTable[Angle+90];
			vec2.x=cs1*Rx;
			vec2.z=sn1*Rx;
			vec2.y=0;

			cs1 = g_cosTable[Angle+180];
			sn1 = g_sinTable[Angle+180];
			vec3.x=cs1*Rx;
			vec3.z=sn1*Rx;
			vec3.y=0;

			cs1 = g_cosTable[Angle+270];
			sn1 = g_sinTable[Angle+270];
			vec4.x=cs1*Rx;
			vec4.z=sn1*Rx;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			vec5=vec1;
			vec6=vec2;
			vec7=vec3;
			vec8=vec4;

			vec1=vec5+m_GI[ec].vecB_pre;
			vec2=vec6+m_GI[ec].vecB_pre;
			vec3=vec7+m_GI[ec].vecB_pre;
			vec4=vec8+m_GI[ec].vecB_pre;

			if (m_GI[ec].flag1[0] <= 1)
			{
				count=250;
				if (vec1.y<-20)
					count=250+((vec1.y+20)*25);
			}
			else if (m_GI[ec].flag1[0]==11)
			{
				count=250;
				if (vec1.y<-40)
					count=250+((vec1.y+40)*25);
			}
			else
			{
				count=50;
				if (vec1.y<-4)
					count=50+((vec1.y+4)*15);
				if (vec1.y>-0.5f)
					count=vec1.y*-100;
			}

			if (count<0)
				count=0;
			if (vec1.y<=0)
			{
				if (m_GI[ec].flag1[0] == 0)
				{
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0);
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0);
				}
				else if (m_GI[ec].flag1[0] == 1)
				{
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,80,80,255);
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,80,80,255);
				}
				else if (m_GI[ec].flag1[0] == 3)
				{
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,255,55);
				}
				else if (m_GI[ec].flag1[0] == 10)
				{
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,255,255);
				}
				else if (m_GI[ec].flag1[0] == 11)
				{
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,155,155,155);
				}
				else
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,70,120,100);
			}


			{
				vec1=vec5+m_GI[ec].vecB_now;
				vec2=vec6+m_GI[ec].vecB_now;
				vec3=vec7+m_GI[ec].vecB_now;
				vec4=vec8+m_GI[ec].vecB_now;

				if (m_GI[ec].flag1[0] <= 1 || m_GI[ec].flag1[0]==11)
				{
					count=250;
					if (vec1.y<-20)
						count=250+((vec1.y+20)*25);
				}
				else
				{
					count=50;
					if (vec1.y<-4)
						count=50+((vec1.y+4)*15);
					if (vec1.y>-0.5f)
						count=vec1.y*-100;
				}

				if (count<0)
					count=0;
				if (vec1.y<=0)
				{
					if (m_GI[ec].flag1[0] == 0)
					{
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0);
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0);
					}
					else if (m_GI[ec].flag1[0] == 1)
					{
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,80,80,255);
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,80,80,255);
					}
					else if (m_GI[ec].flag1[0] == 3)
					{
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,255,55);
					}
					else if (m_GI[ec].flag1[0] == 11)
					{

					}
					else
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,70,120,100);
				}

				vec1=vec5+m_GI[ec].vecT_pre;
				vec2=vec6+m_GI[ec].vecT_pre;
				vec3=vec7+m_GI[ec].vecT_pre;
				vec4=vec8+m_GI[ec].vecT_pre;

				if (m_GI[ec].flag1[0] <= 1 || m_GI[ec].flag1[0]==11)
				{
					count=250;
					if (vec1.y<-20)
						count=250+((vec1.y+20)*25);
				}
				else
				{
					count=50;
					if (vec1.y<-4)
						count=50+((vec1.y+4)*15);
					if (vec1.y>-0.5f)
						count=vec1.y*-100;
				}

				if (count<0)
					count=0;
				if (vec1.y<=0)
				{
					if (m_GI[ec].flag1[0] == 0)
					{
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0);
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0);
					}
					else if (m_GI[ec].flag1[0] == 1)
					{
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,80,80,255);
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,80,80,255);
					}
					else if (m_GI[ec].flag1[0] == 3)
					{
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,255,55);
					}
					else if (m_GI[ec].flag1[0] == 11)
					{

					}
					else
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,70,120,100);
				}

				vec1=vec5+m_GI[ec].vecT_now;
				vec2=vec6+m_GI[ec].vecT_now;
				vec3=vec7+m_GI[ec].vecT_now;
				vec4=vec8+m_GI[ec].vecT_now;

				if (m_GI[ec].flag1[0] <= 1 || m_GI[ec].flag1[0]==11)
				{
					count=250;
					if (vec1.y<-20)
						count=250+((vec1.y+20)*25);
				}
				else
				{
					count=50;
					if (vec1.y<-4)
						count=50+((vec1.y+4)*15);
					if (vec1.y>-0.5f)
						count=vec1.y*-100;
				}

				if (count<0)
					count=0;
				if (vec1.y<=0)
				{
					if (m_GI[ec].flag1[0] == 0)
					{
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0);
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0);
					}
					else if (m_GI[ec].flag1[0] == 1)
					{
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,80,80,255);
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,80,80,255);
					}
					else if (m_GI[ec].flag1[0] == 3)
					{
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,255,55);
					}
					else if (m_GI[ec].flag1[0] == 11)
					{

					}
					else
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,70,120,100);
				}
			}
		}
	}
}

void	CEffectPrim :: RenderMapParticle(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vec5, vec6, vec7, vec8;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int count;
	int Angle;
	float Rx;
	int ec;

	Angle = 45;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			Rx=m_GI[ec].distance;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec1.x=cs1*Rx;
			vec1.z=sn1*Rx;
			vec1.y=0;

			cs1 = g_cosTable[Angle+90];
			sn1 = g_sinTable[Angle+90];
			vec2.x=cs1*Rx;
			vec2.z=sn1*Rx;
			vec2.y=0;

			cs1 = g_cosTable[Angle+180];
			sn1 = g_sinTable[Angle+180];
			vec3.x=cs1*Rx;
			vec3.z=sn1*Rx;
			vec3.y=0;

			cs1 = g_cosTable[Angle+270];
			sn1 = g_sinTable[Angle+270];
			vec4.x=cs1*Rx;
			vec4.z=sn1*Rx;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			vec5=vec1;
			vec6=vec2;
			vec7=vec3;
			vec8=vec4;

			vec1=vec5+m_GI[ec].vecB_pre;
			vec2=vec6+m_GI[ec].vecB_pre;
			vec3=vec7+m_GI[ec].vecB_pre;
			vec4=vec8+m_GI[ec].vecB_pre;

			{
				if (m_GI[ec].flag1[1] == 2)
					count=150;
				else
					count=130;

				if (vec1.y<-1)
					count=130+((vec1.y+1)*4);
			}

			if (count<0)
				count=0;
			if (vec1.y<=0)
			{
				if (m_GI[ec].flag1[1] == 2)
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,155,255,155);
				else
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,255,255);
			}

			{
				vec1=vec5+m_GI[ec].vecB_now;
				vec2=vec6+m_GI[ec].vecB_now;
				vec3=vec7+m_GI[ec].vecB_now;
				vec4=vec8+m_GI[ec].vecB_now;

				{
					if (m_GI[ec].flag1[1] == 2)
						count=150;
					else
						count=130;
					if (vec1.y<-1)
						count=130+((vec1.y+1)*4);
				}

				if (count<0)
					count=0;
				if (vec1.y<=0)
				{
					if (m_GI[ec].flag1[1] == 2)
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,155,255,155);
					else
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,255,255);
				}

				vec1=vec5+m_GI[ec].vecT_pre;
				vec2=vec6+m_GI[ec].vecT_pre;
				vec3=vec7+m_GI[ec].vecT_pre;
				vec4=vec8+m_GI[ec].vecT_pre;

				{
					if (m_GI[ec].flag1[1] == 2)
						count=150;
					else
						count=130;
					if (vec1.y<-1)
						count=130+((vec1.y+1)*4);
				}

				if (count<0)
					count=0;
				if (vec1.y<=0)
				{
					if (m_GI[ec].flag1[1] == 2)
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,155,255,155);
					else
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,255,255);
				}

				vec1=vec5+m_GI[ec].vecT_now;
				vec2=vec6+m_GI[ec].vecT_now;
				vec3=vec7+m_GI[ec].vecT_now;
				vec4=vec8+m_GI[ec].vecT_now;

				{
					if (m_GI[ec].flag1[1] == 2)
						count=150;
					else
						count=130;
					if (vec1.y<-1)
						count=130+((vec1.y+1)*4);
				}

				if (count<0)
					count=0;
				if (vec1.y<=0)
				{
					if (m_GI[ec].flag1[1] == 2)
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,155,255,155);
					else
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,255,255);
				}
			}
		}
	}
}

void	CEffectPrim :: RenderSoulParticle(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vec5, vec6, vec7, vec8;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int count;
	int Angle;
	float Rx;
	int ec;

	Angle = 45;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			Rx=m_GI[ec].distance;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec1.x=cs1*Rx;
			vec1.z=sn1*Rx;
			vec1.y=0;

			cs1 = g_cosTable[Angle+90];
			sn1 = g_sinTable[Angle+90];
			vec2.x=cs1*Rx;
			vec2.z=sn1*Rx;
			vec2.y=0;

			cs1 = g_cosTable[Angle+180];
			sn1 = g_sinTable[Angle+180];
			vec3.x=cs1*Rx;
			vec3.z=sn1*Rx;
			vec3.y=0;

			cs1 = g_cosTable[Angle+270];
			sn1 = g_sinTable[Angle+270];
			vec4.x=cs1*Rx;
			vec4.z=sn1*Rx;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			vec5=vec1;
			vec6=vec2;
			vec7=vec3;
			vec8=vec4;

			vec1=vec5+m_GI[ec].vecB_pre;
			vec2=vec6+m_GI[ec].vecB_pre;
			vec3=vec7+m_GI[ec].vecB_pre;
			vec4=vec8+m_GI[ec].vecB_pre;

			count=m_GI[ec].height[12];

			if (count<0)
				count=0;
			if (vec1.y<=0)
			{
				if (m_GI[ec].height[15]==0)
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,80,80,255);
				else if (m_GI[ec].height[15]==1)
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,200,200);
				else if (m_GI[ec].height[15]==2)
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,160,160);
				else if (m_GI[ec].height[15]==3)
					RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,120,120);
			}

			{
				vec1=vec5+m_GI[ec].vecB_now;
				vec2=vec6+m_GI[ec].vecB_now;
				vec3=vec7+m_GI[ec].vecB_now;
				vec4=vec8+m_GI[ec].vecB_now;

				count=m_GI[ec].height[13];

				if (count<0)
					count=0;
				if (vec1.y<=0)
				{
					if (m_GI[ec].height[15]==0)
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,80,80,255);
					else if (m_GI[ec].height[15]==1)
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,200,200);
					else if (m_GI[ec].height[15]==2)
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,160,160);
					else if (m_GI[ec].height[15]==3)
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,120,120);
				}

				vec1=vec5+m_GI[ec].vecT_pre;
				vec2=vec6+m_GI[ec].vecT_pre;
				vec3=vec7+m_GI[ec].vecT_pre;
				vec4=vec8+m_GI[ec].vecT_pre;

				count=m_GI[ec].height[14];

				if (count<0)
					count=0;
				if (vec1.y<=0)
				{
					if ( m_GI[ec].height[15] == 0 )
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,80,80,255);
					else if (m_GI[ec].height[15]==1)
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,200,200);
					else if (m_GI[ec].height[15]==2)
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,160,160);
					else if (m_GI[ec].height[15]==3)
						RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,0,0,255,120,120);

				}
			}
		}
	}
}

void	CEffectPrim :: RenderSoulLink(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vecA;
	vector3d	vec1, vec2, vec3, vec4;
	int Angle,alpha;
	float Rx;
	int ec;
	float		length;
	int			rpCnt = 0;
	float		uInc = 0.0f;

	Angle = 45;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			Rx=m_GI[ec].distance;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec1.x=cs1*Rx;
			vec1.z=sn1*Rx;
			vec1.y=0;

			cs1 = g_cosTable[Angle+90];
			sn1 = g_sinTable[Angle+90];
			vec2.x=cs1*Rx;
			vec2.z=sn1*Rx;
			vec2.y=0;

			cs1 = g_cosTable[Angle+180];
			sn1 = g_sinTable[Angle+180];
			vec3.x=cs1*Rx;
			vec3.z=sn1*Rx;
			vec3.y=0;

			cs1 = g_cosTable[Angle+270];
			sn1 = g_sinTable[Angle+270];
			vec4.x=cs1*Rx;
			vec4.z=sn1*Rx;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			vecA = m_GI[ec].vecB_pre;
			length = vecA.x;
			vecA.x = length * g_cosTable[m_GI[0].alphaT];
			vecA.z = length * g_sinTable[m_GI[0].alphaT];

			vec1=vec1+vecA;
			vec2=vec2+vecA;
			vec3=vec3+vecA;
			vec4=vec4+vecA;

			alpha = m_GI[ec].height[13];

			if (vec1.y<=0)
			{
				if (m_GI[ec].process<180)
				{
					if (g_session.GetAid() == m_GI[ec].height[10])
					{
						CRagEffect *	effect = g_modeMgr.GetGameMode()->GetWorld()->m_player->LaunchEffect(EF_SOULLIGHT);
						if (effect)
						{
							vector3d	pos = vecA;
							effect->SendMsg(NULL, AM_SETPOS, INT_PTR(&pos));
						}
					}
					else
					{
						CGameActor * mActor = g_modeMgr.GetGameMode()->GetWorld()->GetGameActorByAID(m_GI[ec].height[10]);
						if (mActor != NULL)
						{
							CRagEffect *	effect = mActor->LaunchEffect(EF_SOULLIGHT);
							if (effect)
							{
								vector3d	pos = vecA;
								effect->SendMsg(NULL, AM_SETPOS, INT_PTR(&pos));
							}
						}
					}
				}

				RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec3,vec4,wtm,0,0,0,125,125,255);
			}
		}
	}
}

void	CEffectPrim :: RenderPeongUp(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vec5, vec6, vec7, vec8;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int Angle;
	float Rx;
	int ec;

	Angle = 45;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			Rx=m_GI[ec].distance;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec1.x=cs1*Rx;
			vec1.z=sn1*Rx;
			vec1.y=0;

			cs1 = g_cosTable[Angle+90];
			sn1 = g_sinTable[Angle+90];
			vec2.x=cs1*Rx;
			vec2.z=sn1*Rx;
			vec2.y=0;

			cs1 = g_cosTable[Angle+180];
			sn1 = g_sinTable[Angle+180];
			vec3.x=cs1*Rx;
			vec3.z=sn1*Rx;
			vec3.y=0;

			cs1 = g_cosTable[Angle+270];
			sn1 = g_sinTable[Angle+270];
			vec4.x=cs1*Rx;
			vec4.z=sn1*Rx;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			vec5=vec1;
			vec6=vec2;
			vec7=vec3;
			vec8=vec4;

			vec1=vec5+m_GI[ec].vecB_pre;
			vec2=vec6+m_GI[ec].vecB_pre;
			vec3=vec7+m_GI[ec].vecB_pre;
			vec4=vec8+m_GI[ec].vecB_pre;

			if (vec1.y<=0)
			{
				if (m_GI[ec].flag1[0] == 2)
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,0,105,105,225);
				else if (m_GI[ec].flag1[0] == 3)
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,0,55,55,55);
				else
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,0,255,255,255);

			}
		}
	}
}

void	CEffectPrim :: RenderPeong(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vec5, vec6, vec7, vec8;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int Angle;
	float Rx;
	int ec;

	Angle = 45;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			Rx=m_GI[ec].distance;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec1.x=cs1*Rx;
			vec1.z=sn1*Rx;
			vec1.y=0;

			cs1 = g_cosTable[Angle+90];
			sn1 = g_sinTable[Angle+90];
			vec2.x=cs1*Rx;
			vec2.z=sn1*Rx;
			vec2.y=0;

			cs1 = g_cosTable[Angle+180];
			sn1 = g_sinTable[Angle+180];
			vec3.x=cs1*Rx;
			vec3.z=sn1*Rx;
			vec3.y=0;

			cs1 = g_cosTable[Angle+270];
			sn1 = g_sinTable[Angle+270];
			vec4.x=cs1*Rx;
			vec4.z=sn1*Rx;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			vec5=vec1;
			vec6=vec2;
			vec7=vec3;
			vec8=vec4;

			vec1=vec5+m_GI[ec].vecB_pre;
			vec2=vec6+m_GI[ec].vecB_pre;
			vec3=vec7+m_GI[ec].vecB_pre;
			vec4=vec8+m_GI[ec].vecB_pre;

			{

				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,255,255,255);
			}

		}
	}
}

void	CEffectPrim :: RenderGrandCross2(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	int  alpha;
	float		uInc = 0.0f;

	int Angle;
	float Rx,Rz;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			Angle = m_GI[ec].RotStart;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];

			Rx=m_GI[ec].height[0];
			Rz=m_GI[ec].height[1];

			m_GI[ec].vecB_pre.x=(cs1*Rx) - (sn1*Rz);
			m_GI[ec].vecB_pre.z=(sn1*Rx) + (cs1*Rz);
			m_GI[ec].vecB_pre.y=-1.0f;

			Rx=m_GI[ec].height[0];
			Rz=-m_GI[ec].height[1];

			m_GI[ec].vecB_now.x=(cs1*Rx) - (sn1*Rz);
			m_GI[ec].vecB_now.z=(sn1*Rx) + (cs1*Rz);
			m_GI[ec].vecB_now.y=-1.0f;

			Rx=-m_GI[ec].height[0];
			Rz=-m_GI[ec].height[1];

			m_GI[ec].vecT_now.x=(cs1*Rx) - (sn1*Rz);
			m_GI[ec].vecT_now.z=(sn1*Rx) + (cs1*Rz);
			m_GI[ec].vecT_now.y=-1.0f;

			Rx=-m_GI[ec].height[0];
			Rz=m_GI[ec].height[1];

			m_GI[ec].vecT_pre.x=(cs1*Rx) - (sn1*Rz);
			m_GI[ec].vecT_pre.z=(sn1*Rx) + (cs1*Rz);
			m_GI[ec].vecT_pre.y=-1.0f;

			vec1=m_GI[ec].vecB_pre;
			vec1.y-=m_GI[ec].height[2];

			vec2=m_GI[ec].vecB_now;
			vec2.y-=m_GI[ec].height[2];

			vec3=m_GI[ec].vecT_pre;
			vec3.y-=m_GI[ec].height[2];

			vec4=m_GI[ec].vecT_now;
			vec4.y-=m_GI[ec].height[2];

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,vec2,vec1,wtm,1);
			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_now,m_GI[ec].vecT_now,vec4,vec2,wtm,1);
			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecT_now,m_GI[ec].vecT_pre,vec3,vec4,wtm,1);
			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecT_pre,m_GI[ec].vecB_pre,vec1,vec3,wtm,1);

			alpha = m_GI[ec].alphaB/2;

			RenderTeiRect(&rpCnt, alpha, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm,0);
			RenderTeiRect(&rpCnt, alpha, m_GI[ec].vecT_pre,m_GI[ec].vecT_now,m_GI[ec].vecB_now,m_GI[ec].vecB_pre,wtm,0);
		}
	}
}

void	CEffectPrim :: RenderSlash1(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	int  alpha;
	float		uInc = 0.0f;
	int Angle;
	float Rx;
	int ec;
	int i;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			for (i=0;i<=2;i++)
			{
				Angle = m_GI[ec].RotStart-1+i;
				if (Angle<0)
					Angle+=360;
				else if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				Rx=m_GI[ec].height[0];

				vec1.x=(cs1*Rx);
				vec1.z=(sn1*Rx);
				vec1.y=-0.0f;

				vec2.x=vec1.x;
				vec2.z=vec1.z;
				if (i==1)
					vec2.y=vec1.y-(m_GI[ec].max_height*0.2f);
				else
					vec2.y=vec1.y-(m_GI[ec].max_height*0.1f);

				Rx=m_GI[ec].height[0]+m_GI[ec].distance;

				vec3.x=(cs1*Rx);
				vec3.z=(sn1*Rx);
				vec3.y=-0.0f;

				vec4.x=vec3.x;
				vec4.z=vec3.z;
				if (i==1)
					vec4.y=vec3.y-(m_GI[ec].max_height);
				else
					vec4.y=vec3.y-(m_GI[ec].max_height*0.4f);

				if (i==1)
					alpha = m_GI[ec].alphaB;
				else
					alpha = m_GI[ec].alphaB/3;

				RenderTeiRect(&rpCnt, alpha, vec1, vec3, vec4, vec2,wtm,0);
			}
		}
	}
}

void	CEffectPrim :: RenderRectUp(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;

	int Angle;
	float Rx,Rz;
	int ec;
	unsigned char r,g,b,p;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			Angle = m_GI[ec].RotStart;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];

			Rx=m_GI[ec].height[0];
			Rz=m_GI[ec].height[1];

			m_GI[ec].vecB_pre.x=(cs1*Rx) - (sn1*Rz);
			m_GI[ec].vecB_pre.z=(sn1*Rx) + (cs1*Rz);
			m_GI[ec].vecB_pre.y=-1.0f;

			Rx=m_GI[ec].height[0];
			Rz=-m_GI[ec].height[1];

			m_GI[ec].vecB_now.x=(cs1*Rx) - (sn1*Rz);
			m_GI[ec].vecB_now.z=(sn1*Rx) + (cs1*Rz);
			m_GI[ec].vecB_now.y=-1.0f;

			Rx=-m_GI[ec].height[0];
			Rz=-m_GI[ec].height[1];

			m_GI[ec].vecT_now.x=(cs1*Rx) - (sn1*Rz);
			m_GI[ec].vecT_now.z=(sn1*Rx) + (cs1*Rz);
			m_GI[ec].vecT_now.y=-1.0f;

			Rx=-m_GI[ec].height[0];
			Rz=m_GI[ec].height[1];

			m_GI[ec].vecT_pre.x=(cs1*Rx) - (sn1*Rz);
			m_GI[ec].vecT_pre.z=(sn1*Rx) + (cs1*Rz);
			m_GI[ec].vecT_pre.y=-1.0f;

			vec1=m_GI[ec].vecB_pre;
			vec1.y-=m_GI[ec].height[2];

			vec2=m_GI[ec].vecB_now;
			vec2.y-=m_GI[ec].height[2];

			vec3=m_GI[ec].vecT_pre;
			vec3.y-=m_GI[ec].height[2];

			vec4=m_GI[ec].vecT_now;
			vec4.y-=m_GI[ec].height[2];

			if (m_GI[ec].height[3] == 0)
			{
				r=255;
				g=255;
				b=255;
				p=0;
			}
			else if (m_GI[ec].height[3] == 1)
			{
				r=235;
				g=255;
				b=235;
				p=1;
			}
			else if (m_GI[ec].height[3] == 2)
			{
				r=80;
				g=80;
				b=80;
				p=1;
			}

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,vec2,vec1,wtm,p,0,0,r,g,b);
			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_now,m_GI[ec].vecT_now,vec4,vec2,wtm,p,0,0,r,g,b);
			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecT_now,m_GI[ec].vecT_pre,vec3,vec4,wtm,p,0,0,r,g,b);
			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecT_pre,m_GI[ec].vecB_pre,vec1,vec3,wtm,p,0,0,r,g,b);

			if (m_GI[ec].height[3] == 2)
			{
				int alpha;
				alpha = m_GI[ec].alphaB*2;
				if (alpha > 255)
					alpha = 255;
				RenderTeiRect(&rpCnt, alpha, m_GI[ec].vecB_pre,m_GI[ec].vecT_now,vec4,vec1,wtm,p,0,0,r,g,b);
				RenderTeiRect(&rpCnt, alpha, m_GI[ec].vecB_now,m_GI[ec].vecT_pre,vec3,vec2,wtm,p,0,0,r,g,b);
			}
		}
	}
}

void	CEffectPrim :: RenderSoulBreaker(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec[4];
	int			rpCnt = 0;
	int angle;
	float Rx;
	vector3d	vecM;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			Rx=m_GI[ec].distance;

			angle = m_GI[ec].RotStart;
			angle-=45;

			for (int i=0;i<4;i++)
			{
				angle+=90;
				if (angle>=360)
					angle-=360;

				cs1 = g_cosTable[angle];
				sn1 = g_sinTable[angle];

				vec[i].y=0;
				vec[i].x=(cs1*Rx);
				vec[i].z=(sn1*Rx);

				vec[i]+=m_GI[ec].vecB_now;
			}

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec[0],vec[1],vec[2],vec[3],wtm,1,0,1,200,200,255);
		}
	}
}

void	CEffectPrim :: RenderRectUp2(matrix &wtm)
{

	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;


	int ec;
	unsigned char r,g,b,p;

	r=200;
	g=250;
	b=200;
	p=0;

	for (ec=0 ; ec<2 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			m_GI[ec].vecB_pre.x=m_GI[ec].height[0];
			m_GI[ec].vecB_pre.z=m_GI[ec].height[1];
			m_GI[ec].vecB_pre.y=-1.0f;

			m_GI[ec].vecB_now.x=m_GI[ec].height[0];
			m_GI[ec].vecB_now.z=-m_GI[ec].height[1];
			m_GI[ec].vecB_now.y=-1.0f;

			m_GI[ec].vecT_now.x=-m_GI[ec].height[0];
			m_GI[ec].vecT_now.z=-m_GI[ec].height[1];
			m_GI[ec].vecT_now.y=-1.0f;

			m_GI[ec].vecT_pre.x=-m_GI[ec].height[0];
			m_GI[ec].vecT_pre.z=m_GI[ec].height[1];
			m_GI[ec].vecT_pre.y=-1.0f;

			vec1=m_GI[ec].vecB_pre;
			vec1.y-=m_GI[ec].height[2];

			vec2=m_GI[ec].vecB_now;
			vec2.y-=m_GI[ec].height[2];

			vec3=m_GI[ec].vecT_pre;
			vec3.y-=m_GI[ec].height[2];

			vec4=m_GI[ec].vecT_now;
			vec4.y-=m_GI[ec].height[2];

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,vec2,vec1,wtm,p,0,0,r,g,b);
			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_now,m_GI[ec].vecT_now,vec4,vec2,wtm,p,0,0,r,g,b);
			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecT_now,m_GI[ec].vecT_pre,vec3,vec4,wtm,p,0,0,r,g,b);
			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecT_pre,m_GI[ec].vecB_pre,vec1,vec3,wtm,p,0,0,r,g,b);

		}
	}

	r=150;
	g=250;
	b=150;
	p=0;

	for (ec=2 ; ec<4 ; ec++)
	{
		float left;
		left=0;
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			m_GI[ec].vecB_pre.x=m_GI[ec].height[0];
			m_GI[ec].vecB_pre.z=m_GI[ec].height[1];
			m_GI[ec].vecB_pre.y=-1.0f;

			m_GI[ec].vecB_now.x=m_GI[ec].height[0];
			m_GI[ec].vecB_now.z=-m_GI[ec].height[1];
			m_GI[ec].vecB_now.y=-1.0f;

			m_GI[ec].vecT_now.x=-m_GI[ec].height[0];
			m_GI[ec].vecT_now.z=-m_GI[ec].height[1];
			m_GI[ec].vecT_now.y=-1.0f;

			m_GI[ec].vecT_pre.x=-m_GI[ec].height[0];
			m_GI[ec].vecT_pre.z=m_GI[ec].height[1];
			m_GI[ec].vecT_pre.y=-1.0f;

			if ( m_GI[ec].distance < (m_GI[ec].height[0]*2) )
			{
				m_GI[ec].vecB_pre.z = m_GI[ec].height[1] - m_GI[ec].distance;
				m_GI[ec].vecB_now.z = m_GI[ec].vecB_pre.z - m_GI[ec].height[3];
				if ( m_GI[ec].vecB_now.z < -m_GI[ec].height[1] )
				{
					left = -m_GI[ec].height[1] - m_GI[ec].vecB_now.z;
					m_GI[ec].vecB_now.z = -m_GI[ec].height[1];

					m_GI[ec].vecT_now.x = m_GI[ec].vecB_now.x - left;
				}
			}
			else if ( m_GI[ec].distance < (m_GI[ec].height[0]*4) )
			{
				m_GI[ec].vecB_now.x = m_GI[ec].height[0] - (m_GI[ec].distance - (m_GI[ec].height[0]*2));
				m_GI[ec].vecT_now.x = m_GI[ec].vecB_now.x - m_GI[ec].height[3];
				if ( m_GI[ec].vecT_now.x < -m_GI[ec].height[0] )
				{
					left = -m_GI[ec].height[0] - m_GI[ec].vecT_now.x;
					m_GI[ec].vecT_now.x = -m_GI[ec].height[0];

					m_GI[ec].vecT_pre.z = m_GI[ec].vecT_now.z + left;
				}
			}
			else if ( m_GI[ec].distance < (m_GI[ec].height[0]*6) )
			{
				m_GI[ec].vecT_now.z = -m_GI[ec].height[1] + (m_GI[ec].distance - (m_GI[ec].height[0]*4));
				m_GI[ec].vecT_pre.z = m_GI[ec].vecT_now.z + m_GI[ec].height[3];
				if ( m_GI[ec].vecT_pre.z > m_GI[ec].height[1] )
				{
					left = m_GI[ec].vecT_pre.z - m_GI[ec].height[1];
					m_GI[ec].vecT_pre.z = m_GI[ec].height[1];

					m_GI[ec].vecB_pre.x = m_GI[ec].vecT_pre.x + left;
				}
			}
			else
			{
				m_GI[ec].vecT_pre.x = -m_GI[ec].height[0] + (m_GI[ec].distance - (m_GI[ec].height[0]*6));
				m_GI[ec].vecB_pre.x = m_GI[ec].vecT_pre.x + m_GI[ec].height[3];
				if ( m_GI[ec].vecB_pre.x > m_GI[ec].height[0] )
				{
					left = m_GI[ec].vecB_pre.x - m_GI[ec].height[0];
					m_GI[ec].vecB_pre.x = m_GI[ec].height[0];

					m_GI[ec].vecB_now.z = m_GI[ec].vecB_pre.z - left;
				}
			}

			vec1=m_GI[ec].vecB_pre;
			vec1.y-=m_GI[ec].height[2];

			vec2=m_GI[ec].vecB_now;
			vec2.y-=m_GI[ec].height[2];

			vec3=m_GI[ec].vecT_pre;
			vec3.y-=m_GI[ec].height[2];

			vec4=m_GI[ec].vecT_now;
			vec4.y-=m_GI[ec].height[2];

			if ( m_GI[ec].distance < (m_GI[ec].height[0]*2) )
			{
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,vec2,vec1,wtm,p,0,0,r,g,b);
				if (left > 0)
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_now,m_GI[ec].vecT_now,vec4,vec2,wtm,p,0,0,r,g,b);
			}
			else if ( m_GI[ec].distance < (m_GI[ec].height[0]*4) )
			{
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_now,m_GI[ec].vecT_now,vec4,vec2,wtm,p,0,0,r,g,b);
				if (left > 0)
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecT_now,m_GI[ec].vecT_pre,vec3,vec4,wtm,p,0,0,r,g,b);
			}
			else if ( m_GI[ec].distance < (m_GI[ec].height[0]*6) )
			{
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecT_now,m_GI[ec].vecT_pre,vec3,vec4,wtm,p,0,0,r,g,b);
				if (left > 0)
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecT_pre,m_GI[ec].vecB_pre,vec1,vec3,wtm,p,0,0,r,g,b);
			}
			else
			{
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecT_pre,m_GI[ec].vecB_pre,vec1,vec3,wtm,p,0,0,r,g,b);
				if (left > 0)
					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,vec2,vec1,wtm,p,0,0,r,g,b);
			}





		}
	}
}

void	CEffectPrim :: RenderIntimidate(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	int Angle,AngleXZ;
	float Rx,Rz,Ry;
	vector3d	vecM;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{

			Rx=m_GI[ec].distance;
			Ry=0;
			Rz=0;

			cs1 = g_cosTable[m_GI[ec].full_display_angle];
			sn1 = g_sinTable[m_GI[ec].full_display_angle];

			vecM.x=(cs1*Rx) - (sn1*Ry);
			vecM.y=(sn1*Rx) + (cs1*Ry);

			Rx=vecM.x;

			cs1 = g_cosTable[m_GI[ec].rise_angle];
			sn1 = g_sinTable[m_GI[ec].rise_angle];

			vecM.x=(cs1*Rx) - (sn1*Rz);
			vecM.z=(sn1*Rx) + (cs1*Rz);


			AngleXZ = int(m_GI[ec].height[2]);
			Angle = m_GI[ec].RotStart;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			Rx=cs1*m_GI[ec].height[1];

			vec1.y=sn1*m_GI[ec].height[1];

			Rz=0;

			cs1 = g_cosTable[AngleXZ];
			sn1 = g_sinTable[AngleXZ];

			vec1.x=(cs1*Rx) - (sn1*Rz);
			vec1.z=(sn1*Rx) + (cs1*Rz);



			Angle+=90;
			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			Rx=cs1*m_GI[ec].height[1];

			vec2.y=sn1*m_GI[ec].height[1];

			Rz=0;

			cs1 = g_cosTable[AngleXZ];
			sn1 = g_sinTable[AngleXZ];

			vec2.x=(cs1*Rx) - (sn1*Rz);
			vec2.z=(sn1*Rx) + (cs1*Rz);



			Angle+=90;
			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			Rx=cs1*m_GI[ec].height[1];

			vec3.y=sn1*m_GI[ec].height[1];

			Rz=0;

			cs1 = g_cosTable[AngleXZ];
			sn1 = g_sinTable[AngleXZ];

			vec3.x=(cs1*Rx) - (sn1*Rz);
			vec3.z=(sn1*Rx) + (cs1*Rz);



			Angle+=90;
			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			Rx=cs1*m_GI[ec].height[1];

			vec4.y=sn1*m_GI[ec].height[1];

			Rz=0;

			cs1 = g_cosTable[AngleXZ];
			sn1 = g_sinTable[AngleXZ];

			vec4.x=(cs1*Rx) - (sn1*Rz);
			vec4.z=(sn1*Rx) + (cs1*Rz);



			if (m_GI[ec].flag1[0] == 2)
			{
				vec1.y=0;
				vec2.y=0;
				vec3.y=0;
				vec4.y=0;


				AngleXZ = int(m_GI[ec].height[2]);
				Angle = m_GI[ec].RotStart;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				vec1.x=cs1*m_GI[ec].height[1];
				vec1.z=sn1*m_GI[ec].height[1];

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				vec2.x=cs1*m_GI[ec].height[1];
				vec2.z=sn1*m_GI[ec].height[1];

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				vec3.x=cs1*m_GI[ec].height[1];
				vec3.z=sn1*m_GI[ec].height[1];

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				vec4.x=cs1*m_GI[ec].height[1];
				vec4.z=sn1*m_GI[ec].height[1];

				Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);
			}

			vec1+=vecM;
			vec2+=vecM;
			vec3+=vecM;
			vec4+=vecM;

			vec1+=m_GI[ec].vecB_now;
			vec2+=m_GI[ec].vecB_now;
			vec3+=m_GI[ec].vecB_now;
			vec4+=m_GI[ec].vecB_now;


			if (m_GI[ec].flag1[0] == 0)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,1,100,150,255);
			else if (m_GI[ec].flag1[0] == 1)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,1,250,250,155);
			else if (m_GI[ec].flag1[0] == 2)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,1,250,100,100);
		}
	}
}

void	CEffectPrim :: RenderTeiHit1(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float Rx,Rz,Ry;
	vector3d	vecM;
	int ec;


	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{

			Rx=(m_GI[ec].height[1]*m_GI[ec].flag1[1])+m_GI[ec].distance;
			Ry=m_GI[ec].height[1];
			Rz=0;

			cs1 = g_cosTable[m_GI[ec].full_display_angle];
			sn1 = g_sinTable[m_GI[ec].full_display_angle];

			vec1.x=(cs1*Rx) - (sn1*Ry);
			vec1.y=(sn1*Rx) + (cs1*Ry);

			Rx=vec1.x;

			cs1 = g_cosTable[m_GI[ec].rise_angle];
			sn1 = g_sinTable[m_GI[ec].rise_angle];

			vec1.x=(cs1*Rx) - (sn1*Rz);
			vec1.z=(sn1*Rx) + (cs1*Rz);


			Rx=(m_GI[ec].height[1]*-m_GI[ec].flag1[1])+m_GI[ec].distance;
			Ry=m_GI[ec].height[1];
			Rz=0;

			cs1 = g_cosTable[m_GI[ec].full_display_angle];
			sn1 = g_sinTable[m_GI[ec].full_display_angle];

			vec2.x=(cs1*Rx) - (sn1*Ry);
			vec2.y=(sn1*Rx) + (cs1*Ry);

			Rx=vec2.x;

			cs1 = g_cosTable[m_GI[ec].rise_angle];
			sn1 = g_sinTable[m_GI[ec].rise_angle];

			vec2.x=(cs1*Rx) - (sn1*Rz);
			vec2.z=(sn1*Rx) + (cs1*Rz);


			Rx=(m_GI[ec].height[1]*-m_GI[ec].flag1[1])+m_GI[ec].distance;
			Ry=m_GI[ec].height[1]*-1;
			Rz=0;

			cs1 = g_cosTable[m_GI[ec].full_display_angle];
			sn1 = g_sinTable[m_GI[ec].full_display_angle];

			vec3.x=(cs1*Rx) - (sn1*Ry);
			vec3.y=(sn1*Rx) + (cs1*Ry);

			Rx=vec3.x;

			cs1 = g_cosTable[m_GI[ec].rise_angle];
			sn1 = g_sinTable[m_GI[ec].rise_angle];

			vec3.x=(cs1*Rx) - (sn1*Rz);
			vec3.z=(sn1*Rx) + (cs1*Rz);


			Rx=(m_GI[ec].height[1]*m_GI[ec].flag1[1])+m_GI[ec].distance;
			Ry=m_GI[ec].height[1]*-1;
			Rz=0;

			cs1 = g_cosTable[m_GI[ec].full_display_angle];
			sn1 = g_sinTable[m_GI[ec].full_display_angle];

			vec4.x=(cs1*Rx) - (sn1*Ry);
			vec4.y=(sn1*Rx) + (cs1*Ry);

			Rx=vec4.x;

			cs1 = g_cosTable[m_GI[ec].rise_angle];
			sn1 = g_sinTable[m_GI[ec].rise_angle];

			vec4.x=(cs1*Rx) - (sn1*Rz);
			vec4.z=(sn1*Rx) + (cs1*Rz);



			if (m_GI[ec].flag1[0] == 0)
			{
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec4,vec1,vec2,vec3,wtm,0,0,0,225,225,80);
			}
			else if (m_GI[ec].flag1[0] == 1)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec4,vec1,vec2,vec3,wtm,0,0,0,255,255,255);
			else
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec4,vec1,vec2,vec3,wtm,1,0,0,200,200,255);
		}
	}
}

void	CEffectPrim :: RenderSTIN(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec[4];
	int			rpCnt = 0;
	float Rx;
	vector3d	vecM;
	int ec;
	int angle;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			Rx=m_GI[ec].distance;

			angle = m_GI[ec].rise_angle;

			for (int i=0;i<4;i++)
			{
				angle+=90;
				if (angle>=360)
					angle-=360;

				cs1 = g_cosTable[angle];
				sn1 = g_sinTable[angle];

				vec[i].x=(cs1*Rx);
				vec[i].z=(sn1*Rx);
				vec[i].y=0;

				vec[i]+=m_GI[ec].vecB_now;
			}

			if (m_GI[ec].flag1[3] == 0)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec[0],vec[1],vec[2],vec[3],wtm,0,0,1,80,80,255);
			else if (m_GI[ec].flag1[3] == 1)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec[0],vec[1],vec[2],vec[3],wtm,0,0,1,80,255,80);
		}
	}
}

void	CEffectPrim :: RenderSTIN5(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec[4];
	int			rpCnt = 0;
	float Rx;
	vector3d	vecM;
	int ec;
	int angle;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			Rx=m_GI[ec].distance;

			angle = m_GI[ec].rise_angle;

			for (int i=0;i<4;i++)
			{
				angle+=90;
				if (angle>=360)
					angle-=360;

				cs1 = g_cosTable[angle];
				sn1 = g_sinTable[angle];



				vec[i].x=g_cosTable[m_GI[ec].RotStart]*(cs1*Rx);
				vec[i].z=g_sinTable[m_GI[ec].RotStart]*(cs1*Rx);
				vec[i].y=(sn1*Rx);

				vec[i]+=m_GI[ec].vecB_now;
			}

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec[0],vec[1],vec[2],vec[3],wtm,0,0,1,80,255,80);
		}
	}
}

void	CEffectPrim :: RenderSMA(matrix &wtm)
{
	int ec;

	for (ec=0; ec<1 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			if ( m_GI[ec].flag1[0] == 0 )
			{
				if ( m_GI[ec].process % 8 == 0 )
				{
					CRagEffect *	effect = g_modeMgr.GetGameMode()->GetWorld()->m_player->LaunchEffect(EF_SMA2);
					if (effect)
					{
						vector3d	pos = m_GI[ec].vecB_now;
						vector3d	pos2;
						pos2.x = m_GI[ec].process;
						if ( m_GI[ec].process % 16 == 0 )
							pos2.y = 0;
						else
							pos2.y = 1;
						effect->SendMsg(NULL, AM_SETPOS, INT_PTR(&pos), INT_PTR(&pos2));
					}
				}
			}
			else if ( m_GI[ec].flag1[0] == 1 )
			{
					CRagEffect *	effect = g_modeMgr.GetGameMode()->GetWorld()->m_player->LaunchEffect(EF_SMA3);
					if (effect)
					{
						vector3d	pos = m_GI[ec].vecB_now;
						effect->SendMsg(NULL, AM_SETPOS, INT_PTR(&pos));
					}
			}
		}
	}
}

void	CEffectPrim :: RenderSMA2(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int order;
	int count;
	int Angle;
	int BasicAngle;
	float Rx,Ry,Rz;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			BasicAngle = m_GI[ec].full_display_angle;
			BasicAngle /= E_DIVISION-1;

			order=0;

			for (count = 0 ; count<=m_GI[ec].full_display_angle ; count+=BasicAngle)
			{
				Angle=count+m_GI[ec].RotStart;
				Angle%=360;

				if (m_GI[ec].full_display_angle == 360)
				{
					if ( order == E_DIVISION-1 )
						Angle = m_GI[ec].RotStart;
				}

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];

				vec2.x=cs1*m_GI[ec].distance;
				vec2.z=sn1*m_GI[ec].distance;
				vec2.y=-0;

				Rx=g_cosTable[m_GI[ec].rise_angle] * m_GI[ec].height[order];
				Ry=g_sinTable[m_GI[ec].rise_angle] * m_GI[ec].height[order];

				Rz=sn1*Rx;
				Rx=cs1*Rx;

				vec4.x=vec2.x+Rx;
				vec4.z=vec2.z+Rz;
				vec4.y=-(Ry);


				vec2+=m_deltaPos;
				vec4+=m_deltaPos;
				if (order > 0)
				{
					float Tx1,Tx2;

					Tx1 = (order-1);
					Tx1 /= E_DIVISION;

					Tx2 = order;
					Tx2 /= E_DIVISION;

					RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec4,vec3,wtm,0,0,1,90,90,255,0,1,Tx1,Tx2);
				}

				order++;
				if (order>=E_DIVISION)
					order=E_DIVISION-1;

				vec1=vec2;
				vec3=vec4;
			}
		}
	}
}

void	CEffectPrim :: RenderAirTexture(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec[4];
	int			rpCnt = 0;
	float Rx,Rx2;
	vector3d	vecM;
	int ec;
	int angle;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			Rx=m_GI[ec].distance;


			angle = 0;

			for (int i=0;i<4;i++)
			{
				angle+=90;
				if (angle>=360)
					angle-=360;

				cs1 = g_cosTable[angle];
				sn1 = g_sinTable[angle];

				vec[i].x=(cs1*Rx);
				vec[i].y=(sn1*Rx);
				vec[i].z=0;

				cs1 = g_cosTable[m_GI[ec].rise_angle];
				sn1 = g_sinTable[m_GI[ec].rise_angle];

				Rx2 = vec[i].x;

				vec[i].x=(cs1*Rx2);
				vec[i].z=(sn1*Rx2);

				vec[i]+=m_GI[ec].vecB_now;
			}

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec[0],vec[1],vec[2],vec[3],wtm,0,0,1,60,60,255);



		}
	}
}

void	CEffectPrim :: RenderLineLink(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vecT;
	int			rpCnt = 0;
	float  radius;
	float		uInc = 0.0f;
	short Angle;
	int alpha;
	float Rx,Rz,Ry;
	float Tx;
	int ec;
	int i,j;

	for (ec=0 ; ec<1 ; ec++)
	{
		if ( m_GI[ec].life == 1 && m_GI[ec].alphaB > 0 )
		{



			Rx=m_GI[ec].vecT_now.x-m_GI[ec].vecT_pre.x;
			Rz=m_GI[ec].vecT_now.z-m_GI[ec].vecT_pre.z;

			Get_Angle(Rx,Rz,&Angle);

			Angle+=90;
			if (Angle>=360)
				Angle-=360;

			if (m_GI[ec].process<15)
			{
				float s;
				s=g_sinTable[m_GI[ec].process*6];

				vecT=m_GI[ec].vecT_pre-m_GI[ec].vecT_now;
				vecT*=s;

				vecT+=m_GI[ec].vecT_now;
			}
			else
			{
				vecT=m_GI[ec].vecT_pre;
			}

			for (j=0;j<2;j++)
			{
				for (i=0;i<=10;i++)
				{
					if (j==0)
						radius=m_GI[ec].max_height;
					else if (j==1)
						radius=m_GI[ec].max_height*2.0f;

					Rx=radius*g_cosTable[i*36];
					Ry=radius*g_sinTable[i*36];

					Tx=Rx;

					Rx=Tx*g_cosTable[Angle];
					Rz=Tx*g_sinTable[Angle];

					vec2=m_GI[ec].vecT_now;
					vec2.x+=Rx;
					vec2.y+=Ry;
					vec2.z+=Rz;

					vec4=vecT;
					vec4.x+=Rx;
					vec4.y+=Ry;
					vec4.z+=Rz;

					if (i>0)
					{
						if (j==0)
						{
							alpha=m_GI[ec].alphaB;

							if (m_GI[ec].height[4] == 2)
								RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,0,0,1,0,100,150);
							else if (m_GI[ec].height[4] == 3)
								RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,0,0,1,255,89,182);
							else
								RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,0,0,1,255,255,0);
						}
						else if (j==1)
						{

							if (m_GI[ec].height[4] == 2)
							{
								alpha=m_GI[ec].alphaB*2;
								RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,0,0,1,255,50,0);
							}
							else if (m_GI[ec].height[4] == 3)
							{
								alpha=m_GI[ec].alphaB*2;
								RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,0,0,1,255,89,182);
							}
							else
							{
								alpha=m_GI[ec].alphaB*2;
								RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,0,0,1,0,0,255);
							}
						}
					}

					vec1=vec2;
					vec3=vec4;
				}
			}
		}
	}
}

void	CEffectPrim :: RenderTripleAttack(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vecT;
	int			rpCnt = 0;
	float		radius;
	float		uInc = 0.0f;
	short Angle;
	int alpha;
	int i,j;
	float Rx,Rz,Ry;
	float Tx;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if ( m_GI[ec].life == 1 && m_GI[ec].alphaB > 0 )
		{
			Angle = m_GI[ec].RotStart;
			Angle+=90;
			Angle%=360;

			for (j=0;j<2;j++)
			{
				for (i=0;i<=5;i++)
				{
					if (j==0)
						radius=m_GI[ec].max_height;
					else if (j==1)
					{
						if (m_GI[ec].height[4] == 3)
							radius=m_GI[ec].max_height*2;
						else
							radius=m_GI[ec].max_height+0.25f;
					}

					Rx=radius*g_cosTable[i*72];
					Ry=radius*g_sinTable[i*72];

					Tx=Rx;

					Rx=Tx*g_cosTable[Angle];
					Rz=Tx*g_sinTable[Angle];

					vec2=m_GI[ec].vecB_now;
					vec2.x+=Rx;
					vec2.y+=Ry-m_GI[ec].height[0];
					vec2.z+=Rz;

					vec4=m_GI[ec].vecT_now;
					vec4.x+=Rx;
					vec4.y+=Ry-m_GI[ec].height[0];
					vec4.z+=Rz;

					if (i>0)
					{
						if (j==0)
						{
							alpha=m_GI[ec].alphaB;
							if (m_GI[ec].height[4] == 2)
								RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,1,0,1,255,255,255);
							else if (m_GI[ec].height[4] == 3)
								RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,1,0,1,255,89,182);
							else
								RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,1,0,1,255,255,55);
						}
						else if (j==1)
						{
							alpha=m_GI[ec].alphaB;
							if (m_GI[ec].height[4] == 2)
								RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,0,0,1,255,255,255);
							else if (m_GI[ec].height[4] == 3)
								RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,0,0,1,255,89,182);
							else
								RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,0,0,1,255,255,200);
						}
					}

					vec1=vec2;
					vec3=vec4;
				}
			}

		}
	}
}

void	CEffectPrim :: RenderForest(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vecT;
	int			rpCnt = 0;
	float  radius;
	float		uInc = 0.0f;
	short Angle;
	int alpha;
	float Rx,Rz;
	int ec;
	int i,j;

	for (ec=0 ; ec<4 ; ec++)
	{
		if ( m_GI[ec].life == 1 && m_GI[ec].alphaB > 0 )
		{


			vecT=m_GI[ec].vecT_pre;

			for (j=0;j<1;j++)
			{
				radius=m_GI[ec].max_height;

				for (i=0;i<=5;i++)
				{
					Angle = i*72;
					Angle += m_GI[ec].RotStart;
					if (Angle>=360)
						Angle-=360;

					Rx=radius*g_cosTable[Angle];
					Rz=radius*g_sinTable[Angle];

					vec2=m_GI[ec].vecT_now;
					vec2.x+=Rx;

					vec2.z+=Rz;

					vec4=vecT;
					vec4.x+=Rx;

					vec4.z+=Rz;

					if (i>0)
					{
						alpha=m_GI[ec].alphaB;
						RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec4,vec3,wtm,0,0,1,230,255,230);
					}

					vec1=vec2;
					vec3=vec4;
				}
			}
		}
	}
}

void	CEffectPrim :: RenderWaterFall(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vecT;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int alpha;
	int ec;
	int i;
	int turn;
	int speed;
	int addtn;
	int tn;
	float scroll;

	for (ec=0 ; ec<4 ; ec++)
	{
		if ( m_GI[ec].life == 1 && m_GI[ec].alphaB > 0 )
		{
			speed=m_GI[ec].max_height;

			turn=m_GI[ec].process%(speed*3);
			scroll = m_GI[ec].process%speed;
			scroll *= 40;
			scroll /= speed;

			if (m_GI[ec].flag1[1] == 1)
			{
				scroll=40-scroll;
			}

			for (i=0;i<5;i++)
			{
				vec1.x=-m_GI[ec].distance*0.5f;
				vec1.y=-m_GI[ec].height[i]+scroll;
				vec1.z=m_GI[ec].vecB_now.z;

				vec2.x=-m_GI[ec].distance*0.5f;
				vec2.y=-m_GI[ec].height[i+1]+scroll;
				vec2.z=m_GI[ec].vecB_now.z;

				vec3.x=m_GI[ec].distance*0.5f;
				vec3.y=-m_GI[ec].height[i+1]+scroll;
				vec3.z=m_GI[ec].vecB_now.z;

				vec4.x=m_GI[ec].distance*0.5f;
				vec4.y=-m_GI[ec].height[i]+scroll;
				vec4.z=m_GI[ec].vecB_now.z;

				if (m_GI[ec].flag1[0] == 1)
				{
					float temp;
					temp=vec1.x;
					vec1.x=vec1.z;
					vec1.z=temp;

					temp=vec2.x;
					vec2.x=vec2.z;
					vec2.z=temp;

					temp=vec4.x;
					vec4.x=vec4.z;
					vec4.z=temp;

					temp=vec3.x;
					vec3.x=vec3.z;
					vec3.z=temp;
				}

				alpha=m_GI[ec].alphaB;

				tn = i%3;

				addtn = turn;
				addtn /= speed;

				if ( m_GI[ec].flag1[1] == 1 )
				{
					tn -= addtn;
					while (tn<0)
						tn+=3;
				}
				else
				{
					tn += addtn;
					if (tn>2)
						tn-=3;
				}

				if (i==0)
				{
					float h,Ty1;
					Ty1 =( scroll*0.025f );

					h = vec1.y - vec2.y;
					vec1.y -= (h*Ty1);
					vec4.y = vec1.y;

					if ( m_GI[ec].flag1[1] == 1 )
						RenderTeiRect(&rpCnt, alpha, vec2,vec3,vec4,vec1,wtm,0,tn,0,55,55,255,Ty1,1.0f);
					else
						RenderTeiRect(&rpCnt, alpha, vec2,vec3,vec4,vec1,wtm,1,tn,0,255,255,255,Ty1,1.0f);
				}
				else if (i==4)
				{
					float h,Ty2;
					Ty2 = ( scroll*0.025f );

					h = vec1.y - vec2.y;
					vec2.y += (h*(1.0f-Ty2));
					vec3.y = vec2.y;
					if ( m_GI[ec].flag1[1] == 1 )
						RenderTeiRect(&rpCnt, alpha, vec2,vec3,vec4,vec1,wtm,0,tn,0,55,55,255,0.0f,Ty2);
					else
						RenderTeiRect(&rpCnt, alpha, vec2,vec3,vec4,vec1,wtm,1,tn,0,255,255,255,0.0f,Ty2);
				}
				else
				{
					if ( m_GI[ec].flag1[1] == 1 )
						RenderTeiRect(&rpCnt, alpha, vec2,vec3,vec4,vec1,wtm,0,tn,0,55,55,255);
					else
						RenderTeiRect(&rpCnt, alpha, vec2,vec3,vec4,vec1,wtm,1,tn,0,255,255,255);
				}
			}
		}
	}
}

void	CEffectPrim :: RenderRainbow(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vecT;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int count;
	int BasicAngle;
	int Angle;
	int ec;
	int order;
	float cs1,sn1;
	float cs2,sn2;
	int rcolor;
	float dist;

	for (ec=0 ; ec<4 ; ec++)
	{
		if ( m_GI[ec].life == 1 && m_GI[ec].alphaB > 0 )
		{
			BasicAngle = m_GI[ec].full_display_angle;
			BasicAngle /= E_DIVISION-1;


			for (rcolor = 0; rcolor < 7 ; rcolor++)
			{
				order = 0;
				for (count = 0; count <= m_GI[ec].full_display_angle; count+=BasicAngle)
				{
					Angle=count+m_GI[ec].RotStart;
					if (Angle>=360)
						Angle-=360;

					cs1 = g_cosTable[Angle];
					sn1 = g_sinTable[Angle];

					vec1.x=cs1*m_GI[ec].distance*(1.0-(rcolor*0.05f));
					vec1.y=sn1*m_GI[ec].distance*(1.0-(rcolor*0.05f))*2;
					vec1.z=0;

					vec4.x=cs1*m_GI[ec].distance*(1.0-((rcolor+1)*0.05f));
					vec4.y=sn1*m_GI[ec].distance*(1.0-(((rcolor+1)*0.05f)))*2;
					vec4.z=0;

					cs1 = g_cosTable[m_GI[ec].rise_angle];
					sn1 = g_sinTable[m_GI[ec].rise_angle];
					dist = vec1.x;

					vec1.x = cs1*dist;
					vec1.z = sn1*dist;

					dist = vec4.x;

					vec4.x = cs1*dist;
					vec4.z = sn1*dist;

					Angle=count+m_GI[ec].RotStart+BasicAngle;
					if (Angle>=360)
						Angle-=360;

					cs2 = g_cosTable[Angle];
					sn2 = g_sinTable[Angle];

					vec2.x=cs2*m_GI[ec].distance*(1.0-(rcolor*0.05f));
					vec2.y=sn2*m_GI[ec].distance*(1.0-(rcolor*0.05f))*2;
					vec2.z=0;

					vec3.x=cs2*m_GI[ec].distance*(1.0-((rcolor+1)*0.05f));
					vec3.y=sn2*m_GI[ec].distance*(1.0-(((rcolor+1)*0.05f)))*2;
					vec3.z=0;

					cs1 = g_cosTable[m_GI[ec].rise_angle];
					sn1 = g_sinTable[m_GI[ec].rise_angle];
					dist = vec2.x;

					vec2.x = cs1*dist;
					vec2.z = sn1*dist;

					dist = vec3.x;

					vec3.z = sn1*dist;
					vec3.x = cs1*dist;

					if (rcolor == 0)
					{
						RenderTeiRect(&rpCnt, order, m_GI[ec].alphaB, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,255,0,0);
					}
					else if (rcolor == 1)
					{
						RenderTeiRect(&rpCnt, order, m_GI[ec].alphaB, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,255,126,0);
					}
					else if (rcolor == 2)
					{
						RenderTeiRect(&rpCnt, order, m_GI[ec].alphaB, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,255,255,0);
					}
					else if (rcolor == 3)
					{
						RenderTeiRect(&rpCnt, order, m_GI[ec].alphaB, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,0,255,0);
					}
					else if (rcolor == 4)
					{
						RenderTeiRect(&rpCnt, order, m_GI[ec].alphaB, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,0,0,255);
					}
					else if (rcolor == 5)
					{
						RenderTeiRect(&rpCnt, order, m_GI[ec].alphaB, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,115,50,200);
					}
					else if (rcolor == 6)
					{
						RenderTeiRect(&rpCnt, order, m_GI[ec].alphaB, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,255,0,255);
					}

					order++;
					if (order>=E_DIVISION)
						order=E_DIVISION-1;
				}
			}
		}
	}
}

void	CEffectPrim :: RenderLightSphere(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vecT;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	short Angle;
	float Rx,Rz;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if ( m_GI[ec].life == 1 && m_GI[ec].alphaB > 0 )
		{
			Rz=0.5f;

			Rx=(m_GI[ec].distance*g_sinTable[m_GI[ec].full_display_angle])+m_GI[ec].distance;
			Angle=m_GI[ec].rise_angle;

			vec2.x=Rx*g_cosTable[Angle];
			vec2.y=Rx*g_sinTable[Angle];

			Rx=vec2.x;
			Angle=m_GI[ec].RotStart;
			vec2.x=Rx*g_cosTable[Angle];
			vec2.z=Rx*g_sinTable[Angle];



			vec2.y-=m_GI[ec].height[3];

			vec3.x=vec2.x;
			vec3.y=vec2.y;
			vec3.z=vec2.z;



			vec1.x=Rz;
			vec1.y=-m_GI[ec].height[3];
			vec1.z=0;

			vec4.x=-Rz;
			vec4.y=-m_GI[ec].height[3];
			vec4.z=0;




			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,0,m_GI[ec].height[0],m_GI[ec].height[1],m_GI[ec].height[2]);



			vec1.x=0;
			vec1.y=-m_GI[ec].height[3];
			vec1.z=Rz;

			vec4.x=0;
			vec4.y=-m_GI[ec].height[3];
			vec4.z=-Rz;


			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,0,m_GI[ec].height[0],m_GI[ec].height[1],m_GI[ec].height[2]);
		}
	}
}

void	CEffectPrim :: RenderGumGang(matrix &wtm,float Rdist,unsigned char R, unsigned char G, unsigned char B)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vec5, vec6, vec7, vec8;
	int			rpCnt = 0;
	int count;
	int Angle;
	float Rx;
	int ec;
	int dif;

	Angle = 0;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			if (m_GI[ec].flag1[4]==1)
			{
				R=255;
				G=20;
				B=20;
			}

			Rx=m_GI[ec].distance*Rdist;

			float length;
			length=4.0f;

			m_GI[ec].vecB_pre.x=g_cosTable[int(m_GI[ec].height[3])] * length;
			m_GI[ec].vecB_pre.z=g_sinTable[int(m_GI[ec].height[3])] * length;

			m_GI[ec].vecB_now.x=g_cosTable[int(m_GI[ec].height[7])] * length;
			m_GI[ec].vecB_now.z=g_sinTable[int(m_GI[ec].height[7])] * length;

			m_GI[ec].vecT_pre.x=g_cosTable[int(m_GI[ec].height[11])] * length;
			m_GI[ec].vecT_pre.z=g_sinTable[int(m_GI[ec].height[11])] * length;

			m_GI[ec].vecT_now.x=g_cosTable[int(m_GI[ec].height[15])] * length;
			m_GI[ec].vecT_now.z=g_sinTable[int(m_GI[ec].height[15])] * length;



			Angle=45;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec1.x=cs1*Rx;
			vec1.z=sn1*Rx;
			vec1.y=0;

			Angle+=90;
			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec2.x=cs1*Rx;
			vec2.z=sn1*Rx;
			vec2.y=0;

			Angle+=90;
			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec3.x=cs1*Rx;
			vec3.z=sn1*Rx;
			vec3.y=0;

			Angle+=90;
			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec4.x=cs1*Rx;
			vec4.z=sn1*Rx;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			vec5=vec1;
			vec6=vec2;
			vec7=vec3;
			vec8=vec4;

			dif = int(m_GI[ec].height[3]-m_GI[0].alphaT);

			if ( m_GI[ec].height[2] == 0 || m_GI[ec].height[2] == 2 )
			{
				if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
					dif = 0;
				else
					dif = 1;
			}
			else
			{
				if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
					dif = 1;
				else
					dif = 0;
			}

			if ( dif == 0 )
			{
				vec1=vec5+m_GI[ec].vecB_pre;
				vec2=vec6+m_GI[ec].vecB_pre;
				vec3=vec7+m_GI[ec].vecB_pre;
				vec4=vec8+m_GI[ec].vecB_pre;
			}
			else
			{
				vec1=vec6+m_GI[ec].vecB_pre;
				vec2=vec5+m_GI[ec].vecB_pre;
				vec3=vec8+m_GI[ec].vecB_pre;
				vec4=vec7+m_GI[ec].vecB_pre;
			}

			count=m_GI[ec].height[1];
			RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,char(m_GI[ec].height[2]),0,R,G,B);






			dif = int(m_GI[ec].height[7]-m_GI[0].alphaT);

			if ( m_GI[ec].height[6] == 0 || m_GI[ec].height[6] == 2 )
			{
				if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
					dif = 0;
				else
					dif = 1;
			}
			else
			{
				if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
					dif = 1;
				else
					dif = 0;
			}

			if ( dif == 0 )
			{
				vec1=vec5+m_GI[ec].vecB_now;
				vec2=vec6+m_GI[ec].vecB_now;
				vec3=vec7+m_GI[ec].vecB_now;
				vec4=vec8+m_GI[ec].vecB_now;
			}
			else
			{
				vec1=vec6+m_GI[ec].vecB_now;
				vec2=vec5+m_GI[ec].vecB_now;
				vec3=vec8+m_GI[ec].vecB_now;
				vec4=vec7+m_GI[ec].vecB_now;
			}

			count=m_GI[ec].height[5];
			RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,char(m_GI[ec].height[6]),0,R,G,B);



			dif = int(m_GI[ec].height[11]-m_GI[0].alphaT);

			if ( m_GI[ec].height[10] == 0 || m_GI[ec].height[10] == 2 )
			{
				if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
					dif = 0;
				else
					dif = 1;
			}
			else
			{
				if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
					dif = 1;
				else
					dif = 0;
			}

			if ( dif == 0 )
			{
				vec1=vec5+m_GI[ec].vecT_pre;
				vec2=vec6+m_GI[ec].vecT_pre;
				vec3=vec7+m_GI[ec].vecT_pre;
				vec4=vec8+m_GI[ec].vecT_pre;
			}
			else
			{
				vec1=vec6+m_GI[ec].vecT_pre;
				vec2=vec5+m_GI[ec].vecT_pre;
				vec3=vec8+m_GI[ec].vecT_pre;
				vec4=vec7+m_GI[ec].vecT_pre;
			}

			count=m_GI[ec].height[9];
			RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,char(m_GI[ec].height[10]),0,R,G,B);



			dif = int(m_GI[ec].height[15]-m_GI[0].alphaT);

			if ( m_GI[ec].height[14] == 0 || m_GI[ec].height[14] == 2 )
			{
				if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
					dif = 0;
				else
					dif = 1;
			}
			else
			{
				if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
					dif = 1;
				else
					dif = 0;
			}

			if ( dif == 0 )
			{
				vec1=vec5+m_GI[ec].vecT_now;
				vec2=vec6+m_GI[ec].vecT_now;
				vec3=vec7+m_GI[ec].vecT_now;
				vec4=vec8+m_GI[ec].vecT_now;
			}
			else
			{
				vec1=vec6+m_GI[ec].vecT_now;
				vec2=vec5+m_GI[ec].vecT_now;
				vec3=vec8+m_GI[ec].vecT_now;
				vec4=vec7+m_GI[ec].vecT_now;
			}

			count=m_GI[ec].height[13];
			RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,char(m_GI[ec].height[14]),0,R,G,B);
		}
	}

}

void	CEffectPrim :: RenderDoubleGumGang(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vec5, vec6, vec7, vec8;
	int			rpCnt = 0;
	int count;
	int Angle;
	float Rx;
	int ec;
	int dif;
	unsigned char R,G,B;
	float Rdist;

	Angle = 0;


	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			for (int i=0;i<3;i++)
			{

				if (m_GI[ec].flag1[4] == 0)
				{
					if (i == 0)
					{
						R=100;G=100;B=255;
						Rdist = 1.0f;
					}
					else if (i == 1)
					{
						R=0;G=0;B=120;
						Rdist = 1.2f;
					}
					else if (i == 2)
					{
						R=0;G=0;B=120;
						Rdist = 2.0f;
					}
				}
				else if (m_GI[ec].flag1[4] == 1)
				{

					if (i == 0)
					{
						R=90;G=90;B=80;
						Rdist = 1.0f;
					}
					else if (i == 1)
					{
						R=60;G=60;B=50;
						Rdist = 1.2f;
					}
					else if (i == 2)
					{
						R=30;G=30;B=20;
						Rdist = 2.0f;
					}
				}
				else
				{

					if (i == 0)
					{
						R=255;G=100;B=100;
						Rdist = 1.0f;
					}
					else if (i == 1)
					{
						R=120;G=0;B=0;
						Rdist = 1.2f;
					}
					else if (i == 2)
					{
						R=120;G=0;B=0;
						Rdist = 2.0f;
					}
				}

				Rx=m_GI[ec].distance*Rdist;

				float length;
				length=4.0f;

				m_GI[ec].vecB_pre.x=g_cosTable[int(m_GI[ec].height[3])] * length;
				m_GI[ec].vecB_pre.z=g_sinTable[int(m_GI[ec].height[3])] * length;

				m_GI[ec].vecB_now.x=g_cosTable[int(m_GI[ec].height[7])] * length;
				m_GI[ec].vecB_now.z=g_sinTable[int(m_GI[ec].height[7])] * length;

				m_GI[ec].vecT_pre.x=g_cosTable[int(m_GI[ec].height[11])] * length;
				m_GI[ec].vecT_pre.z=g_sinTable[int(m_GI[ec].height[11])] * length;

				m_GI[ec].vecT_now.x=g_cosTable[int(m_GI[ec].height[15])] * length;
				m_GI[ec].vecT_now.z=g_sinTable[int(m_GI[ec].height[15])] * length;



				Angle=45;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				vec1.x=cs1*Rx;
				vec1.z=sn1*Rx;
				vec1.y=0;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				vec2.x=cs1*Rx;
				vec2.z=sn1*Rx;
				vec2.y=0;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				vec3.x=cs1*Rx;
				vec3.z=sn1*Rx;
				vec3.y=0;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				vec4.x=cs1*Rx;
				vec4.z=sn1*Rx;
				vec4.y=0;

				Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

				vec5=vec1;
				vec6=vec2;
				vec7=vec3;
				vec8=vec4;

				dif = int(m_GI[ec].height[3]-m_GI[0].alphaT);

				if ( m_GI[ec].height[2] == 0 || m_GI[ec].height[2] == 2 )
				{
					if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
						dif = 0;
					else
						dif = 1;
				}
				else
				{
					if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
						dif = 1;
					else
						dif = 0;
				}

				if ( dif == 0 )
				{
					vec1=vec5+m_GI[ec].vecB_pre;
					vec2=vec6+m_GI[ec].vecB_pre;
					vec3=vec7+m_GI[ec].vecB_pre;
					vec4=vec8+m_GI[ec].vecB_pre;
				}
				else
				{
					vec1=vec6+m_GI[ec].vecB_pre;
					vec2=vec5+m_GI[ec].vecB_pre;
					vec3=vec8+m_GI[ec].vecB_pre;
					vec4=vec7+m_GI[ec].vecB_pre;
				}

				count=m_GI[ec].height[1];
				if (count>m_GI[ec].process)
					count = m_GI[ec].process;
				RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,char(m_GI[ec].height[2]),0,R,G,B);






				dif = int(m_GI[ec].height[7]-m_GI[0].alphaT);

				if ( m_GI[ec].height[6] == 0 || m_GI[ec].height[6] == 2 )
				{
					if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
						dif = 0;
					else
						dif = 1;
				}
				else
				{
					if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
						dif = 1;
					else
						dif = 0;
				}

				if ( dif == 0 )
				{
					vec1=vec5+m_GI[ec].vecB_now;
					vec2=vec6+m_GI[ec].vecB_now;
					vec3=vec7+m_GI[ec].vecB_now;
					vec4=vec8+m_GI[ec].vecB_now;
				}
				else
				{
					vec1=vec6+m_GI[ec].vecB_now;
					vec2=vec5+m_GI[ec].vecB_now;
					vec3=vec8+m_GI[ec].vecB_now;
					vec4=vec7+m_GI[ec].vecB_now;
				}

				count=m_GI[ec].height[5];
				if (count>m_GI[ec].process)
					count = m_GI[ec].process;
				RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,char(m_GI[ec].height[6]),0,R,G,B);



				dif = int(m_GI[ec].height[11]-m_GI[0].alphaT);

				if ( m_GI[ec].height[10] == 0 || m_GI[ec].height[10] == 2 )
				{
					if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
						dif = 0;
					else
						dif = 1;
				}
				else
				{
					if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
						dif = 1;
					else
						dif = 0;
				}

				if ( dif == 0 )
				{
					vec1=vec5+m_GI[ec].vecT_pre;
					vec2=vec6+m_GI[ec].vecT_pre;
					vec3=vec7+m_GI[ec].vecT_pre;
					vec4=vec8+m_GI[ec].vecT_pre;
				}
				else
				{
					vec1=vec6+m_GI[ec].vecT_pre;
					vec2=vec5+m_GI[ec].vecT_pre;
					vec3=vec8+m_GI[ec].vecT_pre;
					vec4=vec7+m_GI[ec].vecT_pre;
				}

				count=m_GI[ec].height[9];
				if (count>m_GI[ec].process)
					count = m_GI[ec].process;
				RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,char(m_GI[ec].height[10]),0,R,G,B);



				dif = int(m_GI[ec].height[15]-m_GI[0].alphaT);

				if ( m_GI[ec].height[14] == 0 || m_GI[ec].height[14] == 2 )
				{
					if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
						dif = 0;
					else
						dif = 1;
				}
				else
				{
					if ( (dif < 70 && dif > -80) || (dif < -270) || (dif > 270) )
						dif = 1;
					else
						dif = 0;
				}

				if ( dif == 0 )
				{
					vec1=vec5+m_GI[ec].vecT_now;
					vec2=vec6+m_GI[ec].vecT_now;
					vec3=vec7+m_GI[ec].vecT_now;
					vec4=vec8+m_GI[ec].vecT_now;
				}
				else
				{
					vec1=vec6+m_GI[ec].vecT_now;
					vec2=vec5+m_GI[ec].vecT_now;
					vec3=vec8+m_GI[ec].vecT_now;
					vec4=vec7+m_GI[ec].vecT_now;
				}

				count=m_GI[ec].height[13];
				if (count>m_GI[ec].process)
					count = m_GI[ec].process;
				RenderTeiRect(&rpCnt, count, vec1,vec2,vec3,vec4,wtm,0,char(m_GI[ec].height[14]),0,R,G,B);
			}

		}
	}
}

void	CEffectPrim :: PrimGI1()
{

	int i,ec;
	int middle,m2;

	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	ec=0;

	for (;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (m_GI[ec].process>0)
			{


				m_GI[ec].distance+=0.07f;

				m_GI[ec].rise_angle-=1;
				if (m_GI[ec].rise_angle<10)
				{
					m_GI[ec].rise_angle=10;
					m_GI[ec].alphaB=0;
				}

				{
					if (m_GI[ec].distance>=4.0f)
					{
						m_GI[ec].alphaB-=3;
						if (m_GI[ec].alphaB<=0)
						{
							m_GI[ec].alphaB = 0;
							if (m_GI[ec].process<m_duration-30)
							{
								m_GI[ec].distance = 4.0f - 0.63f;
								if (m_GI[ec].alphaT == 22)
									m_GI[ec].rise_angle=55 + 9;
								else
									m_GI[ec].rise_angle=65 + 9;
							}
						}
					}
					else
					{
						if (m_GI[ec].alphaT == 22)
							m_GI[ec].alphaB+=5;
						else
							m_GI[ec].alphaB+=10;
					}
				}



				for (i = 0 ; i<E_DIVISION ; i++)
				{
						int SinLimit;
						int pr;

						if (ec<2)
							pr=(m_GI[ec].process+(ec*90))%360;
						else
							pr=((m_GI[ec].process*2)+(ec*90))%360;

						SinLimit=90+((i-middle)*m2);

						m_GI[ec].height[i]=m_GI[ec].max_height;
						m_GI[ec].height[i]+=m_GI[ec].max_height*g_sinTable[SinLimit]*0.3f*g_sinTable[pr];
				}
			}
		}
	}
}

void	CEffectPrim :: PrimGrandCross2()
{
	int middle,m2,ec;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	ec=0;

	for (;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;


			if (m_GI[ec].process>=10)
			{

				m_GI[ec].alphaB-=1;
				if (m_GI[ec].alphaB<=0)
				{
					m_GI[ec].alphaB = 0;
					if (m_GI[ec].process>30)
						m_GI[ec].life=0;
				}
			}
			else
			{
				m_GI[ec].alphaB+=9;
			}

			m_GI[ec].height[1]+=0.005f;
			m_GI[ec].height[0]+=0.005f;

			m_GI[ec].height[2]=m_GI[ec].max_height*g_sinTable[m_GI[ec].process];

		}
	}
}

void	CEffectPrim :: PrimTeleportation2()
{

	int i,ec;
	int middle,m2;



	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	ec=0;

	for (;ec<4;ec++)
	{
		int pr;
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			m_GI[ec].RotStart+=(5-ec);
			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			if (m_GI[ec].process>0)
			{

				if (m_GI[ec].flag1[0] == 0)
				{
					pr = m_GI[ec].process * 2;

					if (pr>=180)
					{
						m_GI[ec].flag1[0] = 1;
					}
					else if (pr>=135)
					{
						m_GI[ec].alphaB--;
						if (m_GI[ec].alphaB<0)
							m_GI[ec].alphaB=0;
					}

					m_GI[ec].max_height = m_GI[ec].flag1[1] * g_sinTable[pr];
				}
				else
				{
					m_GI[ec].alphaB-=10;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}


				for (i = 0 ; i<E_DIVISION ; i++)
				{
						int SinLimit;


						if (ec<2)
							pr=(m_GI[ec].process+(ec*90))%360;
						else
							pr=((m_GI[ec].process*3)+(ec*90))%360;

						SinLimit=90+((i-middle)*m2);

						m_GI[ec].height[i]=m_GI[ec].max_height;
						m_GI[ec].height[i]+=m_GI[ec].max_height*g_sinTable[SinLimit]*0.3f*g_sinTable[pr];
				}
			}
		}
	}
}

void	CEffectPrim :: PrimGI4()
{

	int i,ec;
	int middle,m2;

	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	ec=0;

	for (;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;


			m_GI[ec].distance+=0.04f;

			m_GI[ec].rise_angle-=1;
			if (m_GI[ec].rise_angle<10)
			{
				m_GI[ec].rise_angle=10;
				m_GI[ec].alphaB=0;
			}

			if (m_GI[ec].distance>=2.5f)
			{
				m_GI[ec].alphaB-=2;
				if (m_GI[ec].alphaB<=0)
				{
					m_GI[ec].alphaB = 0;
					m_GI[ec].distance = 2.5f - 0.36f;
					m_GI[ec].rise_angle=65 + 9;
				}
			}
			else
			{
				m_GI[ec].alphaB+=20;
			}


			for (i = 0 ; i<E_DIVISION ; i++)
			{
					int SinLimit;
					int pr;

					if (ec<2)
						pr=(m_GI[ec].process+(ec*90))%360;
					else
						pr=((m_GI[ec].process*2)+(ec*90))%360;

					SinLimit=90+((i-middle)*m2);

					m_GI[ec].height[i]=m_GI[ec].max_height;
					m_GI[ec].height[i]+=m_GI[ec].max_height*g_sinTable[SinLimit]*0.3f*g_sinTable[pr];
			}
		}
	}
}

void	CEffectPrim :: PrimGI5()
{

	int i,ec;
	int middle,m2;

	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	ec=0;

	for (;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			m_GI[ec].alphaT++;
			if (m_GI[ec].alphaT>=360)
				m_GI[ec].alphaT-=360;

			m_GI[ec].distance=3.0f + (g_sinTable[m_GI[ec].alphaT]);


			m_GI[ec].RotStart+=3;
			if (m_GI[ec].RotStart>=360)
			{
				m_GI[ec].RotStart-=360;
			}


			for (i = 0 ; i<E_DIVISION ; i++)
			{
					int SinLimit;
					int pr;

					if (ec<2)
						pr=(m_GI[ec].process+(ec*90))%360;
					else
						pr=((m_GI[ec].process*2)+(ec*90))%360;

					SinLimit=90+((i-middle)*m2);

					m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[SinLimit];
					m_GI[ec].height[i]+=m_GI[ec].max_height*g_sinTable[SinLimit]*0.3f*g_sinTable[pr];
			}
		}
	}
}

void	CEffectPrim :: PrimGI2()
{

	int i,ec;
	int middle,m2;

	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	ec=0;

	for (;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (m_accel!=99)
				m_GI[ec].RotStart+=5;
			else
				m_GI[ec].RotStart+=3;

			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;


			if (m_accel!=99)
				m_GI[ec].distance+=0.3f;
			else
				m_GI[ec].distance+=0.1f;

			m_GI[ec].rise_angle-=1;
			if (m_accel!=99)
			{
				if (m_GI[ec].rise_angle<40)
				{
					m_GI[ec].rise_angle=40;
				}
			}
			else
			{
				if (m_GI[ec].rise_angle<30)
				{
					m_GI[ec].rise_angle=30;
				}
			}

			if  (m_GI[ec].process>10)
			{
				if (m_GI[ec].alphaT==2)
					m_GI[ec].alphaB-=8;
				else
					m_GI[ec].alphaB-=2;

				if (m_GI[ec].alphaB<=0)
				{
					m_GI[ec].alphaB = 0;
				}
			}
			else
			{
				if (m_GI[ec].alphaT==1)
					m_GI[ec].alphaB+=10;
				else
					m_GI[ec].alphaB+=20;
			}


			for (i = 0 ; i<E_DIVISION ; i++)
			{
					int SinLimit;
					int pr;
					pr=(m_GI[ec].process+(ec*90))%360;

					SinLimit=90+((i-middle)*m2);

					m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[SinLimit];
					m_GI[ec].height[i]+=m_GI[ec].max_height*g_sinTable[SinLimit]*0.15f*g_sinTable[pr];
			}
		}
	}
}

void	CEffectPrim :: PrimGI3()
{

	int i,ec;
	int middle,m2;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	ec=0;

	for (;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;



			m_GI[ec].distance-=0.01f;


			if (m_GI[ec].process>=10)
			{
				m_GI[ec].alphaB-=1;
				if (m_GI[ec].alphaB<=0)
				{
					m_GI[ec].alphaB = 0;
					if (m_GI[ec].process>30)
						m_GI[ec].life=0;

					m_GI[ec].distance = 20.0f;
					m_GI[ec].rise_angle=81 + 9;
				}
			}
			else
			{
				m_GI[ec].alphaB+=10;
			}


			for (i = 0 ; i<E_DIVISION ; i++)
			{
				m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[m_GI[ec].process];
			}
		}
	}
}

void	CEffectPrim :: PrimSlash1()
{


	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			m_GI[ec].height[0]+=1.0f;
			if (m_GI[ec].height[0]>10.0f)
			{
				m_GI[ec].alphaB-=2;
				if (m_GI[ec].alphaB<0)
				{
					m_GI[ec].alphaB=0;
					m_GI[ec].life=0;
				}
			}


			if (m_GI[ec].flag1[0] == 1)
			{
				m_GI[ec].distance+=1.0f;

				if (m_GI[ec].max_height<2.0f)
					m_GI[ec].max_height+=0.2f;

				if (m_GI[ec].process<=5)
					m_GI[ec].alphaB+=5;
			}
			else
			{
				m_GI[ec].distance+=2.0f;

				if (m_GI[ec].max_height<4.0f)
					m_GI[ec].max_height+=0.4f;

				if (m_GI[ec].process<=7)
					m_GI[ec].alphaB+=5;
			}

		}
	}

}

void	CEffectPrim :: PrimRectUp()
{

	int ec;
	int temp;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;




			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].height[3] == 0)
					m_GI[ec].full_display_angle+=1;
				else

					m_GI[ec].full_display_angle+=1;

				temp=m_GI[ec].full_display_angle%360;

				if (m_GI[ec].process>m_GI[ec].alphaT)
				{
					m_GI[ec].alphaB-=2;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}

				if (m_GI[ec].height[3] == 0)
				{
					m_GI[ec].height[2] = m_GI[ec].max_height*g_sinTable[temp]*0.35f;
					m_GI[ec].height[2] += m_GI[ec].max_height * 0.65f;
				}
				else
				{
					m_GI[ec].height[2] = m_GI[ec].max_height*g_sinTable[temp]*0.35f;
					m_GI[ec].height[2] += m_GI[ec].max_height * 0.65f;
				}

				if (m_GI[ec].process<90)
				{
					m_GI[ec].height[2]*=g_sinTable[m_GI[ec].process];
				}
			}
		}
	}
}

void	CEffectPrim :: PrimTeiHit1()
{
	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
				m_GI[ec].distance+=m_GI[ec].height[0];

			if (m_GI[ec].process<=20 && m_GI[ec].process>10)
			{
				if (m_GI[ec].flag1[1]==16)
					m_GI[ec].alphaB+=15;
				else
					m_GI[ec].alphaB+=15;
			}
			else
			{
				m_GI[ec].alphaB-=5;
				if (m_GI[ec].alphaB<=0)
					m_GI[ec].alphaB=0;
			}

		}
	}
}

void	CEffectPrim :: PrimRectUp2()
{

	int ec;

	for (ec=0;ec<2;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (m_GI[ec].process>0)
			{


				if (m_GI[ec].process>m_GI[ec].alphaT)
				{
					int angle;
					if (m_GI[ec].height[4] == 0)
						angle = 2505-m_GI[ec].process;
					else
						angle = 2475-m_GI[ec].process;
					if (angle>90)
						angle=90;
					if (angle<0)
						angle=0;
					m_GI[ec].height[2] = m_GI[ec].max_height * g_sinTable[angle];
				}
				else
				{
					if (m_GI[ec].process<90)
					{
						m_GI[ec].height[2] = m_GI[ec].max_height * g_sinTable[m_GI[ec].process];
					}
					else
					{
						m_GI[ec].height[2] = m_GI[ec].max_height;
					}
				}
			}
		}
	}

	for (ec=2;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			if (m_GI[ec].process>0)
			{
				m_GI[ec].distance+=0.5f;

				if ( m_GI[ec].distance > (m_GI[ec].height[0]*8) )
					m_GI[ec].distance -= (m_GI[ec].height[0]*8);

				if (m_GI[ec].process>m_GI[ec].alphaT)
				{
					int angle;
					if (m_GI[ec].height[4] == 0)
						angle = 2505-m_GI[ec].process;
					else
						angle = 2475-m_GI[ec].process;

					if (angle>90)
						angle=90;
					if (angle<0)
						angle=0;
					m_GI[ec].height[2] = m_GI[ec].max_height * g_sinTable[angle];
				}
				else
				{
					if (m_GI[ec].process<90)
					{
						m_GI[ec].height[2] = m_GI[ec].max_height * g_sinTable[m_GI[ec].process];
					}
					else
					{
						m_GI[ec].height[2] = m_GI[ec].max_height;
					}
				}
			}
		}
	}
}

void	CEffectPrim :: PrimIntimidate()
{

	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;

	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				float height = m_GI[ec].height[0];
				m_GI[ec].distance=height+m_GI[ec].distance;
			}

			if (m_GI[ec].process<=10 && m_GI[ec].process>0)
			{
				if (m_GI[ec].flag1[0] == 0)
					m_GI[ec].alphaB+=15;
				else
					m_GI[ec].alphaB+=25;
			}
			else if (m_GI[ec].process>25)
			{
				m_GI[ec].alphaB-=7;
				if (m_GI[ec].alphaB<=0)
					m_GI[ec].alphaB=0;
			}

			if (m_GI[ec].flag1[0]==2)
				m_GI[ec].RotStart+=3;

			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			if (m_GI[ec].flag1[0]==2)
				m_GI[ec].height[2]+=3;
			else
				m_GI[ec].height[2]+=5;

			if (m_GI[ec].height[2]>=360)
				m_GI[ec].height[2]-=360;
		}
	}
}

void	CEffectPrim :: PrimSoulBreaker()
{
	float Rx,Rz;


	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].process == 1)
				{
					Rx=m_deltaPos.x-m_pos.x;
					Rz=m_deltaPos.z-m_pos.z;

					Get_Angle(Rx,Rz,&m_GI[ec].RotStart);

					m_GI[ec].RotStart+=random(31)-15;
					if (m_GI[ec].RotStart>=360)
						m_GI[ec].RotStart-=360;
					if (m_GI[ec].RotStart<0)
						m_GI[ec].RotStart+=360;
				}
				else if (m_GI[ec].process<=10)
				{
					m_GI[ec].alphaB+=10;
					m_GI[ec].height[1]*=1.02f;
				}
				else
				{
					m_GI[ec].alphaB-=5;
					m_GI[ec].height[1]*=1.04f;
					if (m_GI[ec].alphaB<0)
					{
						m_GI[ec].alphaB=0;
						m_GI[ec].life=0;
					}
				}

				m_GI[ec].vecB_now.x+=m_GI[ec].height[1]*g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_now.z+=m_GI[ec].height[1]*g_sinTable[m_GI[ec].RotStart];
			}

		}
	}
}

void	CEffectPrim :: PrimSTIN()
{
	float Rx,Rz;


	int ec;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				m_GI[ec].rise_angle+=30;
				if (m_GI[ec].rise_angle>=360)
					m_GI[ec].rise_angle-=360;

				if (m_GI[ec].process == 1)
				{
					Rx=m_deltaPos.x-m_pos.x;
					Rz=m_deltaPos.z-m_pos.z;

					Get_Angle(Rx,Rz,&m_GI[ec].RotStart);
				}
				else if (m_GI[ec].process<=10)
				{
					m_GI[ec].alphaB+=25;
					if (m_GI[ec].alphaB>250)
						m_GI[ec].alphaB=250;
					m_GI[ec].height[1]*=1.02f;
				}
				else
				{
					m_GI[ec].alphaB-=5;
					m_GI[ec].height[1]*=1.04f;
					if (m_GI[ec].alphaB<0)
					{
						m_GI[ec].alphaB=0;
						m_GI[ec].life=0;
					}
				}

				m_GI[ec].vecB_now.x+=m_GI[ec].height[1]*g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_now.z+=m_GI[ec].height[1]*g_sinTable[m_GI[ec].RotStart];

				if (m_GI[ec].process%3 == 0)
				{
					for (int i=3;i>=1;i--)
					{
						m_GI[i].vecB_now = m_GI[i-1].vecB_now;
						m_GI[i].alphaB = m_GI[i-1].alphaB*0.8f;
						m_GI[i].rise_angle = m_GI[i-1].rise_angle;
					}
				}
			}

		}
	}
}

void	CEffectPrim :: PrimSTIN5()
{
	float Rx,Rz;


	int ec;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				m_GI[ec].rise_angle+=30;
				if (m_GI[ec].rise_angle>=360)
					m_GI[ec].rise_angle-=360;

				if (m_GI[ec].process == 1)
				{
					Rx=m_deltaPos.x-m_pos.x;
					Rz=m_deltaPos.z-m_pos.z;

					Get_Angle(Rx,Rz,&m_GI[ec].RotStart);
				}
				else if (m_GI[ec].process<=10)
				{
					m_GI[ec].alphaB+=6;
					if (m_GI[ec].alphaB>60)
						m_GI[ec].alphaB=60;
					m_GI[ec].height[1]*=1.02f;
				}
				else
				{
					m_GI[ec].alphaB-=3;
					m_GI[ec].height[1]*=1.04f;
					if (m_GI[ec].alphaB<0)
					{
						m_GI[ec].alphaB=0;
						m_GI[ec].life=0;
					}
				}

				m_GI[ec].vecB_now.x+=m_GI[ec].height[1]*g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_now.z+=m_GI[ec].height[1]*g_sinTable[m_GI[ec].RotStart];

				if (m_GI[ec].process%3 == 0)
				{
					for (int i=3;i>=1;i--)
					{
						m_GI[i].vecB_now = m_GI[i-1].vecB_now;
						m_GI[i].alphaB = m_GI[i-1].alphaB*0.8f;
						m_GI[i].rise_angle = m_GI[i-1].rise_angle;

					}
				}
			}

		}
	}
}

void	CEffectPrim :: PrimSTIN2()
{
	float Rx,Rz;
	CGameMode *		gameMode = g_modeMgr.GetGameMode();
	CGameActor *	tActor = NULL;


	int ec;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				if (m_deltaPos.y == g_session.GetAid())
					tActor = gameMode->GetWorld()->m_player;
				else
					tActor = gameMode->GetWorld()->GetGameActorByAID(m_deltaPos.y);

				if (tActor == NULL)
				{
					m_GI[ec].flag1[1] = 10;

					if (m_GI[ec].process == 1)
						break;
				}

				m_GI[ec].rise_angle+=30;
				if (m_GI[ec].rise_angle>=360)
					m_GI[ec].rise_angle-=360;

				if (m_GI[ec].process == 1)
				{
					Rx=tActor->m_pos.x-m_pos.x;
					Rz=tActor->m_pos.z-m_pos.z;

					Get_Angle(Rx,Rz,&m_GI[ec].RotStart);

					if (m_GI[ec].flag1[2] == 0)
					{
						m_GI[ec].RotStart+=90;
						if (m_GI[ec].RotStart>=360)
							m_GI[ec].RotStart-=360;
					}
					else
					{
						m_GI[ec].RotStart-=90;
						if (m_GI[ec].RotStart<0)
							m_GI[ec].RotStart+=360;
					}
				}
				else if (m_GI[ec].process<=20)
				{
					m_GI[ec].alphaB+=15;
					if (m_GI[ec].alphaB>250)
						m_GI[ec].alphaB=250;
				}
				else if (m_GI[ec].process>140)
				{
					m_GI[ec].flag1[1] = 10;
				}

				if (m_GI[ec].process > 5 )
				{

					if (m_GI[ec].flag1[1] == 0)
					{
						short nAngle;
						Rx=tActor->m_pos.x-m_GI[ec].vecB_now.x;
						Rz=tActor->m_pos.z-m_GI[ec].vecB_now.z;

						Get_Angle(Rx,Rz,&nAngle);

						if ( (m_GI[ec].RotStart >= nAngle - 3) && (m_GI[ec].RotStart <= nAngle + 3) )
						{

						}
						else if (m_GI[ec].flag1[2] == 0)
						{
							m_GI[ec].RotStart-=5;
							if (m_GI[ec].RotStart<0)
								m_GI[ec].RotStart+=360;
						}
						else
						{
							m_GI[ec].RotStart+=5;
							if (m_GI[ec].RotStart>=360)
								m_GI[ec].RotStart-=360;
						}
					}
					else if (m_GI[ec].flag1[1] == 10)
					{
						for (int i=0;i<4;i++)
						{
							m_GI[i].alphaB-=5;
							if (m_GI[i].alphaB<0)
							{
								m_GI[i].alphaB=0;
								m_GI[i].life=0;
							}
						}

						if (m_GI[ec].flag1[2] == 0)
						{
							m_GI[ec].RotStart-=1;
							if (m_GI[ec].RotStart<0)
								m_GI[ec].RotStart+=360;
						}
						else
						{
							m_GI[ec].RotStart+=1;
							if (m_GI[ec].RotStart>=360)
								m_GI[ec].RotStart-=360;
						}
					}
				}

				m_GI[ec].vecB_now.x+=m_GI[ec].height[1]*g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_now.z+=m_GI[ec].height[1]*g_sinTable[m_GI[ec].RotStart];

				if (m_GI[ec].process%3 == 0)
				{
					for (int i=3;i>=1;i--)
					{
						m_GI[i].vecB_now = m_GI[i-1].vecB_now;
						m_GI[i].alphaB = m_GI[i-1].alphaB*0.8f;
						m_GI[i].rise_angle = m_GI[i-1].rise_angle;
					}
				}
			}
		}
	}
}

void	CEffectPrim :: PrimSTIN4()
{
	float Rx,Rz;
	CGameMode *		gameMode = g_modeMgr.GetGameMode();
	CGameActor *	tActor = NULL;


	int ec;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				if (m_deltaPos.y == g_session.GetAid())
					tActor = gameMode->GetWorld()->m_player;
				else
					tActor = gameMode->GetWorld()->GetGameActorByAID(m_deltaPos.y);

				if (tActor == NULL)
				{
					m_GI[ec].flag1[1] = 10;

					if (m_GI[ec].process == 1)
						break;
				}

				m_GI[ec].rise_angle+=30;
				if (m_GI[ec].rise_angle>=360)
					m_GI[ec].rise_angle-=360;

				if (m_GI[ec].process == 1)
				{
					Rx=tActor->m_pos.x-m_pos.x;
					Rz=tActor->m_pos.z-m_pos.z;

					Get_Angle(Rx,Rz,&m_GI[ec].RotStart);

					if (m_GI[ec].flag1[2] == 0)
					{
						m_GI[ec].RotStart+=25+random(21);
						if (m_GI[ec].RotStart>=360)
							m_GI[ec].RotStart-=360;
					}
					else
					{
						m_GI[ec].RotStart-=25+random(21);
						if (m_GI[ec].RotStart<0)
							m_GI[ec].RotStart+=360;
					}
				}
				else if (m_GI[ec].process<=20)
				{
					m_GI[ec].alphaB+=8;
					if (m_GI[ec].alphaB>120)
						m_GI[ec].alphaB=120;
				}
				else
				{
					m_GI[ec].flag1[1] = 10;
				}

				if (m_GI[ec].process > 5 )
				{

					if (m_GI[ec].flag1[1] == 0)
					{
						short nAngle;
						Rx=tActor->m_pos.x-m_GI[ec].vecB_now.x;
						Rz=tActor->m_pos.z-m_GI[ec].vecB_now.z;

						Get_Angle(Rx,Rz,&nAngle);

						if ( (m_GI[ec].RotStart >= nAngle - 3) && (m_GI[ec].RotStart <= nAngle + 3) )
						{
							m_GI[ec].flag1[1] = 10;
						}
						else if (m_GI[ec].flag1[2] == 0)
						{
							m_GI[ec].RotStart-=5;
							if (m_GI[ec].RotStart<0)
								m_GI[ec].RotStart+=360;
						}
						else
						{
							m_GI[ec].RotStart+=5;
							if (m_GI[ec].RotStart>=360)
								m_GI[ec].RotStart-=360;
						}
					}
					else if (m_GI[ec].flag1[1] == 10)
					{
						for (int i=0;i<4;i++)
						{
							m_GI[i].alphaB-=5;
							if (m_GI[i].alphaB<0)
							{
								m_GI[i].alphaB=0;
								m_GI[i].life=0;
							}
						}

						if (m_GI[ec].flag1[2] == 0)
						{
							m_GI[ec].RotStart-=1;
							if (m_GI[ec].RotStart<0)
								m_GI[ec].RotStart+=360;
						}
						else
						{
							m_GI[ec].RotStart+=1;
							if (m_GI[ec].RotStart>=360)
								m_GI[ec].RotStart-=360;
						}
					}
				}

				m_GI[ec].vecB_now.x+=m_GI[ec].height[1]*g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_now.z+=m_GI[ec].height[1]*g_sinTable[m_GI[ec].RotStart];

				if (m_GI[ec].process%3 == 0)
				{
					for (int i=3;i>=1;i--)
					{
						m_GI[i].vecB_now = m_GI[i-1].vecB_now;
						m_GI[i].alphaB = m_GI[i-1].alphaB*0.8f;
						m_GI[i].rise_angle = m_GI[i-1].rise_angle;
					}
				}
			}
		}
	}
}

void	CEffectPrim :: PrimSMA()
{
	float Rx,Rz;


	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].process == 1)
				{
					Rx=m_deltaPos.x-m_pos.x;
					Rz=m_deltaPos.z-m_pos.z;

					Get_Angle(Rx,Rz,&m_GI[ec].RotStart);
				}

				m_GI[ec].vecB_now.x+=m_GI[ec].height[1]*g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_now.z+=m_GI[ec].height[1]*g_sinTable[m_GI[ec].RotStart];
			}

		}
	}
}

bool	CEffectPrim :: PrimSMA2()
{

	int i,ec;
	int middle,m2;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;

			m_GI[ec].RotStart+=(ec+3);

			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			if (m_GI[ec].process>=m_duration-40)
			{
				m_GI[ec].alphaB-=5;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}
			else if (m_GI[ec].process<40)
			{
					m_GI[ec].alphaB+=5;
					if (m_GI[ec].alphaB>180)
						m_GI[ec].alphaB=180;
			}

			if ( m_deltaPos2.x > 50 )
			{
				if ( m_GI[ec].alphaB > 200 - ((m_deltaPos2.x-50)*5) )
					m_GI[ec].alphaB = 200 - ((m_deltaPos2.x-50)*5);

				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}

			for (i = 0 ; i<E_DIVISION ; i++)
			{
				int SinLimit;

				if (m_GI[ec].flag1[i]==0)
				{
					SinLimit=90+((i-middle)*m2);

					if (m_GI[ec].process<=90)
						m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[SinLimit]*g_sinTable[m_GI[ec].process];

					if (m_GI[ec].height[i] > m_GI[ec].max_height*g_sinTable[SinLimit])
					{
						m_GI[ec].height[i] = m_GI[ec].max_height*g_sinTable[SinLimit];
						m_GI[ec].flag1[i]=1;
					}

					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
				}
			}
		}
	}

	return true;
}

void	CEffectPrim :: PrimSoulBreaker2()
{

	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].process == 1)
				{

				}
				else if (m_GI[ec].process<=10)
				{
					m_GI[ec].alphaB+=10;
					m_GI[ec].height[1]*=1.02f;
				}
				else
				{
					m_GI[ec].alphaB-=5;
					m_GI[ec].height[1]*=1.04f;
					if (m_GI[ec].alphaB<0)
					{
						m_GI[ec].alphaB=0;
						m_GI[ec].life=0;
					}
				}

				m_GI[ec].vecB_now.x+=m_GI[ec].height[1]*g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_now.z+=m_GI[ec].height[1]*g_sinTable[m_GI[ec].RotStart];
			}

		}
	}

}

void	CEffectPrim :: PrimTeiHit2()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();
	float Rx,Rz;

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;


	m_pos = m_master->m_pos;


	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

				if (m_GI[ec].process>0)
				{
					m_GI[ec].full_display_angle-=5;
					if (m_GI[ec].full_display_angle<0)
						m_GI[ec].full_display_angle+=360;

					if (m_GI[ec].process == 1)
					{
						Rx=m_deltaPos.x-m_pos.x;
						Rz=m_deltaPos.z-m_pos.z;

						Get_Angle(Rx,Rz,&m_GI[ec].RotStart);

						m_GI[ec].RotStart=m_GI[ec].RotStart+random(60)-30;
						if (m_GI[ec].RotStart>=360)
							m_GI[ec].RotStart-=360;
						if (m_GI[ec].RotStart<0)
							m_GI[ec].RotStart+=360;
					}
					else if (m_GI[ec].process<=10)
						m_GI[ec].alphaB+=15;
					else
					{
						m_GI[ec].alphaB-=3;
						if (m_GI[ec].alphaB<0)
						{
							m_GI[ec].alphaB=0;
							m_GI[ec].life=0;
						}
					}

				m_GI[ec].vecB_now.y += m_GI[ec].height[2];
				m_GI[ec].vecB_now.x+=m_GI[ec].height[1]*g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_now.z+=m_GI[ec].height[1]*g_sinTable[m_GI[ec].RotStart];
			}

		}
	}

}

void	CEffectPrim :: PrimParticleUp()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;


	m_pos = m_master->m_pos;


	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				m_GI[ec].full_display_angle-=5;
				if (m_GI[ec].full_display_angle<0)
					m_GI[ec].full_display_angle+=360;

				if (m_GI[ec].process == 1)
				{
					m_GI[ec].RotStart=random(360);
				}
				else if (m_GI[ec].process<=10)
					m_GI[ec].alphaB+=15;
				else
				{
					if (m_GI[ec].flag1[1] == 1)
					{
						if (m_GI[ec].process == (35 - (ec*7)) || m_GI[ec].process == (36 - (ec*7)) )
								m_GI[ec].flag1[0] = 1;
						else
								m_GI[ec].flag1[0] = 0;

					}

					m_GI[ec].alphaB-=3;
					if (m_GI[ec].alphaB<0)
					{
						m_GI[ec].alphaB=0;
						m_GI[ec].life=0;
					}
				}

				m_GI[ec].vecB_now.y -= m_GI[ec].height[2];
				m_GI[ec].vecB_now.x+=m_GI[ec].height[1]*g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_now.z+=m_GI[ec].height[1]*g_sinTable[m_GI[ec].RotStart];
			}
		}
	}
}

bool	CEffectPrim :: PrimLineLink()
{

	CGameMode *		gameMode = g_modeMgr.GetGameMode();
	CGameActor *	tActor = NULL;
	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<1;ec++)
	{
			m_GI[ec].process++;

			m_GI[ec].vecT_now=m_pos;
			m_GI[ec].vecT_now.y-=8.0f;

			if (m_GI[ec].height[4] == 2)
			{
				if (m_GI[ec].process>40)
				{
					m_GI[ec].alphaB-=3;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}
				else if (m_GI[ec].process<20)
				{
					m_GI[ec].alphaB+=6;
				}
			}
			else if (m_GI[ec].height[4] == 3)
			{
				int pro;
				pro=m_GI[ec].process%360;

				if (m_GI[ec].process<=80)
				{
					m_GI[ec].alphaB+=1;
				}

				m_GI[ec].max_height+=g_sinTable[pro]*0.005f;
			}
			else
			{
				if (m_GI[ec].process%10<5)
					m_GI[ec].alphaB+=2;
				else
					m_GI[ec].alphaB-=2;
			}

			if (m_GI[ec].height[0]>0)
			{
				if (m_GI[ec].height[0] == g_session.GetAid())
				{
					tActor = gameMode->GetWorld()->m_player;
				}
				else
				{
					tActor = gameMode->GetWorld()->GetGameActorByAID(m_GI[ec].height[0]);
				}
			}

			if (tActor == NULL)
			{
				m_GI[ec].life=0;
				m_GI[ec].alphaB=0;
			}
			else
			{
				m_GI[ec].vecT_pre=tActor->m_pos;
				m_GI[ec].vecT_pre.y-=8.0f;
			}
	}

	return true;
}

bool	CEffectPrim :: PrimTripleAttack()
{

	int ec;



	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life>0)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].process<=5)
				{
					if (m_GI[ec].height[4] == 2)
						m_GI[ec].alphaB+=2;
					else if (m_GI[ec].height[4] == 3)
						m_GI[ec].alphaB+=8;
					else
						m_GI[ec].alphaB+=8;
				}
				else
				{
					if (m_GI[ec].height[4] == 2)
						m_GI[ec].alphaB-=1;
					else if (m_GI[ec].height[4] == 3)
						m_GI[ec].alphaB-=4;
					else
						m_GI[ec].alphaB-=4;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}

				m_GI[ec].vecT_now.x = m_GI[ec].vecB_now.x + (m_GI[ec].distance*g_cosTable[m_GI[ec].RotStart]);
				m_GI[ec].vecT_now.z = m_GI[ec].vecB_now.z + (m_GI[ec].distance*g_sinTable[m_GI[ec].RotStart]);
			}
		}
	}

	return true;
}

void	CEffectPrim :: PrimShieldBoomerang()
{
	CView* pView = g_modeMgr.GetGameMode()->GetView();
	float Rx,Rz;
	short angle;
	char ra;

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;


	m_pos = m_master->m_pos;


	int ec;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].flag1[2] == 1)
				{
					ra=1;
				}
				else
				{
					ra=3;
				}

				if (m_GI[ec].flag1[2] == 1)
				{
					if (m_GI[ec].process<=10)
						m_GI[ec].alphaB+=25;
					if (m_GI[ec].alphaB>=255)
						m_GI[ec].alphaB=255;
				}

				if (m_GI[ec].process%ra == 0)
				{
					if (m_GI[ec].process > 3 )
					{
						m_GI[ec+3].full_display_angle=m_GI[ec+2].full_display_angle;
						m_GI[ec+3].distance = m_GI[ec+2].distance;
						m_GI[ec+3].height[8] = m_GI[ec+2].height[8];
						m_GI[ec+3].alphaB = m_GI[ec+2].alphaB - 25;
						if (m_GI[ec+3].alphaB < 0)
							m_GI[ec+3].alphaB = 0;

						m_GI[ec+3].vecB_now=m_GI[ec+2].vecB_now;

						m_GI[ec+3].full_display_angle = m_GI[ec+2].full_display_angle;
						m_GI[ec+3].flag1[0] = m_GI[ec+2].flag1[0];
					}

					if (m_GI[ec].process > 2 )
					{
						m_GI[ec+2].full_display_angle=m_GI[ec+1].full_display_angle;
						m_GI[ec+2].distance = m_GI[ec+1].distance;
						m_GI[ec+2].height[8] = m_GI[ec+1].height[8];
						m_GI[ec+2].alphaB = m_GI[ec+1].alphaB - 25;
						if (m_GI[ec+2].alphaB < 0)
							m_GI[ec+2].alphaB = 0;

						m_GI[ec+2].vecB_now=m_GI[ec+1].vecB_now;

						m_GI[ec+2].full_display_angle = m_GI[ec+1].full_display_angle;
						m_GI[ec+2].flag1[0] = m_GI[ec+1].flag1[0];
					}

					if (m_GI[ec].process > 1 )
					{
						m_GI[ec+1].full_display_angle=m_GI[ec].full_display_angle;
						m_GI[ec+1].distance = m_GI[ec].distance;
						m_GI[ec+1].height[8] = m_GI[ec].height[8];
						m_GI[ec+1].alphaB = m_GI[ec].alphaB - 150;
						if (m_GI[ec+1].alphaB < 0)
							m_GI[ec+1].alphaB = 0;

						m_GI[ec+1].vecB_now=m_GI[ec].vecB_now;

						m_GI[ec+1].full_display_angle = m_GI[ec].full_display_angle;
						m_GI[ec+1].flag1[0] = m_GI[ec].flag1[0];
					}
				}

				m_GI[ec].full_display_angle-=15;
				if (m_GI[ec].full_display_angle<0)
					m_GI[ec].full_display_angle+=360;

				if (m_GI[ec].process == 1)
				{
					Rx=m_deltaPos.x-m_GI[ec].vecB_now.x;
					Rz=m_deltaPos.z-m_GI[ec].vecB_now.z;

					Get_Angle(Rx,Rz,&m_GI[ec].RotStart);
				}

				if (m_GI[ec].flag1[1] == 0)
				{
					{
						if ( m_GI[ec].max_height < m_GI[ec].height[1]*m_GI[ec].process )
						{
							if (m_GI[ec].flag1[2] == 1)
							{
								m_GI[ec].flag1[1] = 2;
								m_GI[ec].height[1]*=3;
								if (random(2)==0)
								{
									m_GI[ec].RotStart+=random(45);
									if (m_GI[ec].RotStart>=360)
										m_GI[ec].RotStart-=360;
								}
								else
								{
									m_GI[ec].RotStart-=random(45);
									if (m_GI[ec].RotStart<0)
										m_GI[ec].RotStart+=360;
								}
							}
							else
								m_GI[ec].flag1[1] = 1;
						}
					}

					m_GI[ec].vecB_now.y += m_GI[ec].height[2];
				}
				else if (m_GI[ec].flag1[1] == 1 )
				{
						Rx=m_pos.x-m_GI[ec].vecB_now.x;
						Rz=m_pos.z-m_GI[ec].vecB_now.z;

						Get_Angle(Rx,Rz,&angle);

						{
							if (m_GI[ec].RotStart < angle - 4 || m_GI[ec].RotStart > angle + 4 )
							{
								m_GI[ec].RotStart+=5;
								if (m_GI[ec].RotStart>=360)
									m_GI[ec].RotStart-=360;
							}
							else
							{
								m_GI[ec].flag1[1] = 2;
							}
						}
				}
				else
				{
					m_GI[ec].alphaB-=12;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;

					m_GI[ec].vecB_now.y -= m_GI[ec].height[2]*1.0f;
				}


				m_GI[ec].vecB_now.x+=m_GI[ec].height[1]*g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_now.z+=m_GI[ec].height[1]*g_sinTable[m_GI[ec].RotStart];
			}

		}
	}

}

void	CEffectPrim :: PrimShieldBoomerang3()
{
	CView* pView = g_modeMgr.GetGameMode()->GetView();
	char ra;

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;


	m_pos = m_master->m_pos;


	int ec;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				ra=3;
				if (m_GI[ec].process%ra == 0)
				{
					if (m_GI[ec].process > 3 )
					{
						m_GI[ec+3].full_display_angle=m_GI[ec+2].full_display_angle;
						m_GI[ec+3].distance = m_GI[ec+2].distance;
						m_GI[ec+3].height[8] = m_GI[ec+2].height[8];
						m_GI[ec+3].alphaB = m_GI[ec+2].alphaB - 25;
						if (m_GI[ec+3].alphaB < 0)
							m_GI[ec+3].alphaB = 0;

						m_GI[ec+3].vecB_now=m_GI[ec+2].vecB_now;

						m_GI[ec+3].full_display_angle = m_GI[ec+2].full_display_angle;
						m_GI[ec+3].flag1[0] = m_GI[ec+2].flag1[0];
					}

					if (m_GI[ec].process > 2 )
					{
						m_GI[ec+2].full_display_angle=m_GI[ec+1].full_display_angle;
						m_GI[ec+2].distance = m_GI[ec+1].distance;
						m_GI[ec+2].height[8] = m_GI[ec+1].height[8];
						m_GI[ec+2].alphaB = m_GI[ec+1].alphaB - 25;
						if (m_GI[ec+2].alphaB < 0)
							m_GI[ec+2].alphaB = 0;

						m_GI[ec+2].vecB_now=m_GI[ec+1].vecB_now;

						m_GI[ec+2].full_display_angle = m_GI[ec+1].full_display_angle;
						m_GI[ec+2].flag1[0] = m_GI[ec+1].flag1[0];
					}

					if (m_GI[ec].process > 1 )
					{
						m_GI[ec+1].full_display_angle=m_GI[ec].full_display_angle;
						m_GI[ec+1].distance = m_GI[ec].distance;
						m_GI[ec+1].height[8] = m_GI[ec].height[8];
						m_GI[ec+1].alphaB = m_GI[ec].alphaB - 150;
						if (m_GI[ec+1].alphaB < 0)
							m_GI[ec+1].alphaB = 0;

						m_GI[ec+1].vecB_now=m_GI[ec].vecB_now;

						m_GI[ec+1].full_display_angle = m_GI[ec].full_display_angle;
						m_GI[ec+1].flag1[0] = m_GI[ec].flag1[0];
					}
				}

				m_GI[ec].full_display_angle-=15;
				if (m_GI[ec].full_display_angle<0)
					m_GI[ec].full_display_angle+=360;

				if (m_GI[ec].process<=5)
				{
					m_GI[ec].alphaB+=45;
					if (m_GI[ec].alphaB>255)
						m_GI[ec].alphaB=255;
				}

				if (m_GI[ec].process>=15)
				{
					m_GI[ec].alphaB-=10;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}

				m_GI[ec].vecB_now.x+=m_GI[ec].height[1]*g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_now.z+=m_GI[ec].height[1]*g_sinTable[m_GI[ec].RotStart];
			}
		}
	}

}

void	CEffectPrim :: PrimTanji2()
{
	CView* pView = g_modeMgr.GetGameMode()->GetView();
	float Rx,Rz;
	char ra;

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;


	m_pos = m_master->m_pos;


	int ec;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].flag1[2] == 1)
				{
					ra=1;
				}
				else
				{
					ra=3;
				}

				if (m_GI[ec].flag1[2] == 1)
				{
					if (m_GI[ec].process<=10)
						m_GI[ec].alphaB+=25;
					if (m_GI[ec].alphaB>=255)
						m_GI[ec].alphaB=255;
				}

				if (m_GI[ec].process%ra == 0)
				{
					if (m_GI[ec].process > 3 )
					{
						m_GI[ec+3].full_display_angle=m_GI[ec+2].full_display_angle;
						m_GI[ec+3].distance = m_GI[ec+2].distance;
						m_GI[ec+3].height[8] = m_GI[ec+2].height[8];
						m_GI[ec+3].alphaB = m_GI[ec+2].alphaB - 25;
						if (m_GI[ec+3].alphaB < 0)
							m_GI[ec+3].alphaB = 0;

						m_GI[ec+3].vecB_now=m_GI[ec+2].vecB_now;

						m_GI[ec+3].full_display_angle = m_GI[ec+2].full_display_angle;
						m_GI[ec+3].flag1[0] = m_GI[ec+2].flag1[0];
					}

					if (m_GI[ec].process > 2 )
					{
						m_GI[ec+2].full_display_angle=m_GI[ec+1].full_display_angle;
						m_GI[ec+2].distance = m_GI[ec+1].distance;
						m_GI[ec+2].height[8] = m_GI[ec+1].height[8];
						m_GI[ec+2].alphaB = m_GI[ec+1].alphaB - 25;
						if (m_GI[ec+2].alphaB < 0)
							m_GI[ec+2].alphaB = 0;

						m_GI[ec+2].vecB_now=m_GI[ec+1].vecB_now;

						m_GI[ec+2].full_display_angle = m_GI[ec+1].full_display_angle;
						m_GI[ec+2].flag1[0] = m_GI[ec+1].flag1[0];
					}

					if (m_GI[ec].process > 1 )
					{
						m_GI[ec+1].full_display_angle=m_GI[ec].full_display_angle;
						m_GI[ec+1].distance = m_GI[ec].distance;
						m_GI[ec+1].height[8] = m_GI[ec].height[8];
						m_GI[ec+1].alphaB = m_GI[ec].alphaB - 150;
						if (m_GI[ec+1].alphaB < 0)
							m_GI[ec+1].alphaB = 0;

						m_GI[ec+1].vecB_now=m_GI[ec].vecB_now;

						m_GI[ec+1].full_display_angle = m_GI[ec].full_display_angle;
						m_GI[ec+1].flag1[0] = m_GI[ec].flag1[0];
					}
				}

				m_GI[ec].full_display_angle-=15;
				if (m_GI[ec].full_display_angle<0)
					m_GI[ec].full_display_angle+=360;

				if (m_GI[ec].process == 1)
				{
					Rx=m_deltaPos.x-m_GI[ec].vecB_now.x;
					Rz=m_deltaPos.z-m_GI[ec].vecB_now.z;

					Get_Angle(Rx,Rz,&m_GI[ec].RotStart);
				}

				m_GI[ec].vecB_now.x+=m_GI[ec].height[1]*g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_now.z+=m_GI[ec].height[1]*g_sinTable[m_GI[ec].RotStart];

				if (m_GI[ec].process % 4 == 0)
				{
					CRagEffect *	effect = g_modeMgr.GetGameMode()->GetWorld()->m_player->LaunchEffect(EF_AIRTEXTURE);
					if (effect)
					{
						vector3d	pos = m_GI[ec].vecB_now;
						effect->SendMsg(NULL, AM_SETPOS, INT_PTR(&pos));
					}
				}
			}

		}
	}

}

void	CEffectPrim :: PrimPressure()
{
	CView* pView = g_modeMgr.GetGameMode()->GetView();
	char ra;

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;


	m_pos = m_master->m_pos;






	int ec;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				{
					ra=1;
				}

				if (m_GI[ec].process%ra == 0)
				{
					if (m_GI[ec].process > 3 )
					{
						m_GI[ec+3].full_display_angle=m_GI[ec+2].full_display_angle;
						m_GI[ec+3].distance = m_GI[ec+2].distance;
						m_GI[ec+3].height[8] = m_GI[ec+2].height[8];
						m_GI[ec+3].alphaB = m_GI[ec+2].alphaB - 25;
						if (m_GI[ec+3].alphaB < 0)
							m_GI[ec+3].alphaB = 0;

						m_GI[ec+3].vecB_now=m_GI[ec+2].vecB_now;

						m_GI[ec+3].full_display_angle = m_GI[ec+2].full_display_angle;
						m_GI[ec+3].flag1[0] = m_GI[ec+2].flag1[0];
					}

					if (m_GI[ec].process > 2 )
					{
						m_GI[ec+2].full_display_angle=m_GI[ec+1].full_display_angle;
						m_GI[ec+2].distance = m_GI[ec+1].distance;
						m_GI[ec+2].height[8] = m_GI[ec+1].height[8];
						m_GI[ec+2].alphaB = m_GI[ec+1].alphaB - 25;
						if (m_GI[ec+2].alphaB < 0)
							m_GI[ec+2].alphaB = 0;

						m_GI[ec+2].vecB_now=m_GI[ec+1].vecB_now;

						m_GI[ec+2].full_display_angle = m_GI[ec+1].full_display_angle;
						m_GI[ec+2].flag1[0] = m_GI[ec+1].flag1[0];
					}

					if (m_GI[ec].process > 1 )
					{
						m_GI[ec+1].full_display_angle=m_GI[ec].full_display_angle;
						m_GI[ec+1].distance = m_GI[ec].distance;
						m_GI[ec+1].height[8] = m_GI[ec].height[8];
						m_GI[ec+1].alphaB = m_GI[ec].alphaB - 100;
						if (m_GI[ec+1].alphaB < 0)
							m_GI[ec+1].alphaB = 0;

						m_GI[ec+1].vecB_now=m_GI[ec].vecB_now;

						m_GI[ec+1].full_display_angle = m_GI[ec].full_display_angle;
						m_GI[ec+1].flag1[0] = m_GI[ec].flag1[0];
					}
				}

				if (m_GI[ec].process<=10)
					m_GI[ec].alphaB+=20;

				if (m_GI[ec].process<=32)
				{

					if (m_GI[ec].flag1[4] == 0)
					{
						m_GI[ec].full_display_angle-=15;
						if (m_GI[ec].full_display_angle<0)
							m_GI[ec].full_display_angle+=360;

						m_GI[ec].vecB_pre.y += m_GI[ec].height[1];
					}
					else if (m_GI[ec].process<=24)
					{
						m_GI[ec].full_display_angle-=15;
						if (m_GI[ec].full_display_angle<0)
							m_GI[ec].full_display_angle+=360;

						m_GI[ec].vecB_pre.y += m_GI[ec].height[1];
					}

					m_GI[ec].vecB_now = m_pos + m_GI[ec].vecB_pre;
				}
				else
				{
					if (m_GI[ec].flag1[4] == 0)
					{
						if (m_GI[ec].process == 33)
							g_modeMgr.GetGameMode()->SendMsg(MM_QUAKE);
					}

					m_GI[ec].alphaB-=5;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}
			}
		}
	}

}

void	CEffectPrim :: PrimBubbleDrop()
{
	CView* pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;


	m_pos = m_master->m_pos;


	int ec;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				{
					if (m_GI[ec].process > 3 )
					{
						m_GI[ec+3].full_display_angle=m_GI[ec+2].full_display_angle;
						m_GI[ec+3].distance = m_GI[ec+2].distance;
						m_GI[ec+3].height[8] = m_GI[ec+2].height[8];
						m_GI[ec+3].alphaB = m_GI[ec+2].alphaB - 25;
						if (m_GI[ec+3].alphaB < 0)
							m_GI[ec+3].alphaB = 0;

						m_GI[ec+3].vecB_now=m_GI[ec+2].vecB_now;

						m_GI[ec+3].full_display_angle = m_GI[ec+2].full_display_angle;
						m_GI[ec+3].flag1[0] = m_GI[ec+2].flag1[0];
					}

					if (m_GI[ec].process > 2 )
					{
						m_GI[ec+2].full_display_angle=m_GI[ec+1].full_display_angle;
						m_GI[ec+2].distance = m_GI[ec+1].distance;
						m_GI[ec+2].height[8] = m_GI[ec+1].height[8];
						m_GI[ec+2].alphaB = m_GI[ec+1].alphaB - 25;
						if (m_GI[ec+2].alphaB < 0)
							m_GI[ec+2].alphaB = 0;

						m_GI[ec+2].vecB_now=m_GI[ec+1].vecB_now;

						m_GI[ec+2].full_display_angle = m_GI[ec+1].full_display_angle;
						m_GI[ec+2].flag1[0] = m_GI[ec+1].flag1[0];
					}

					if (m_GI[ec].process > 1 )
					{
						m_GI[ec+1].full_display_angle=m_GI[ec].full_display_angle;
						m_GI[ec+1].distance = m_GI[ec].distance;
						m_GI[ec+1].height[8] = m_GI[ec].height[8];
						m_GI[ec+1].alphaB = m_GI[ec].alphaB - 100;
						if (m_GI[ec+1].alphaB < 0)
							m_GI[ec+1].alphaB = 0;

						m_GI[ec+1].vecB_now=m_GI[ec].vecB_now;

						m_GI[ec+1].full_display_angle = m_GI[ec].full_display_angle;
						m_GI[ec+1].flag1[0] = m_GI[ec].flag1[0];
					}
				}

				if (m_GI[ec].process<=10)
					m_GI[ec].alphaB+=20;

				if (m_GI[ec].process<m_GI[ec].flag1[4])
				{
					m_GI[ec].full_display_angle-=15;
					if (m_GI[ec].full_display_angle<0)
						m_GI[ec].full_display_angle+=360;

					m_GI[ec].vecB_pre.y += m_GI[ec].height[1];
					m_GI[ec].vecB_now = m_pos + m_GI[ec].vecB_pre;
				}
				else
				{
					m_GI[ec].alphaB-=10;
					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}
			}
		}
	}

}

void	CEffectPrim :: PrimBash3D()
{

	m_pos = m_master->m_pos;


	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].height[4] == 2)
				{
					if (m_GI[ec].process<=10)
						m_GI[ec].alphaB+=10;
					else if (m_GI[ec].process>11)
					{
						m_GI[ec].alphaB-=3;
						if (m_GI[ec].alphaB<0)
							m_GI[ec].alphaB=0;
					}
					m_GI[ec].distance+=3.0f;
				}
				else if (m_GI[ec].height[4] == 3)
				{
					if (m_GI[ec].process<=10)
						m_GI[ec].alphaB+=6;
					else if (m_GI[ec].process>11)
					{
						m_GI[ec].alphaB-=1;
						if (m_GI[ec].alphaB<0)
							m_GI[ec].alphaB=0;
					}
					m_GI[ec].distance+=3.0f;
				}
				else
				{
					if (m_GI[ec].process<=10)
						m_GI[ec].alphaB+=20;
					else if (m_GI[ec].process>12)
					{
						m_GI[ec].alphaB-=15;
						if (m_GI[ec].alphaB<0)
							m_GI[ec].alphaB=0;
					}
					m_GI[ec].distance*=1.15f;
				}
			}
		}
	}
}

void	CEffectPrim :: PrimFirstAid()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;


	m_pos = m_master->m_pos;


	int ec;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			m_GI[ec].height[8]+=3;
			if (m_GI[ec].height[8]>=360)
				m_GI[ec].height[8]-=360;

			if (m_GI[ec].process<=20)
			{
				m_GI[ec].alphaB+=4;
			}

			if (m_GI[ec].process>100)
			{
				m_GI[ec].alphaB--;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}
		}
	}
}

void	CEffectPrim :: PrimAirTexture()
{



	int ec;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process<50)
				m_GI[ec].distance+=0.1f;

			if (m_GI[ec].process<=5)
			{
				m_GI[ec].alphaB+=50;
			}

			if (m_GI[ec].process>5)
			{
				m_GI[ec].alphaB-=3;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}
		}
	}

}

void	CEffectPrim :: PrimThrowItem()
{
	CView* pView = g_modeMgr.GetGameMode()->GetView();
	float Rx,Rz;

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;


	m_pos = m_master->m_pos;


	int ec;

	for (ec=0;ec<1;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			float a,b;

			m_GI[ec].process++;

			if (m_GI[ec].process%2 == 1)
			{
				if (m_GI[ec].process > 3 )
				{
					m_GI[ec+3].full_display_angle=m_GI[ec+2].full_display_angle;
					m_GI[ec+3].distance = m_GI[ec+2].distance;
					m_GI[ec+3].height[8] = m_GI[ec+2].height[8];
					m_GI[ec+3].alphaB = m_GI[ec+2].alphaB - 25;
					if (m_GI[ec+3].alphaB < 0)
						m_GI[ec+3].alphaB = 0;

					m_GI[ec+3].vecB_now=m_GI[ec+2].vecB_now;

					m_GI[ec+3].full_display_angle = m_GI[ec+2].full_display_angle;
					m_GI[ec+3].flag1[0] = m_GI[ec+2].flag1[0];
				}

				if (m_GI[ec].process > 2 )
				{
					m_GI[ec+2].full_display_angle=m_GI[ec+1].full_display_angle;
					m_GI[ec+2].distance = m_GI[ec+1].distance;
					m_GI[ec+2].height[8] = m_GI[ec+1].height[8];
					m_GI[ec+2].alphaB = m_GI[ec+1].alphaB - 25;
					if (m_GI[ec+2].alphaB < 0)
						m_GI[ec+2].alphaB = 0;

					m_GI[ec+2].vecB_now=m_GI[ec+1].vecB_now;

					m_GI[ec+2].full_display_angle = m_GI[ec+1].full_display_angle;
					m_GI[ec+2].flag1[0] = m_GI[ec+1].flag1[0];
				}

				if (m_GI[ec].process > 1 )
				{
					m_GI[ec+1].full_display_angle=m_GI[ec].full_display_angle;
					m_GI[ec+1].distance = m_GI[ec].distance;
					m_GI[ec+1].height[8] = m_GI[ec].height[8];
					m_GI[ec+1].alphaB = m_GI[ec].alphaB - 150;
					if (m_GI[ec+1].alphaB < 0)
						m_GI[ec+1].alphaB = 0;

					m_GI[ec+1].vecB_now=m_GI[ec].vecB_now;

					m_GI[ec+1].full_display_angle = m_GI[ec].full_display_angle;
					m_GI[ec+1].flag1[0] = m_GI[ec].flag1[0];
				}
			}

			if (m_GI[ec].flag1[2] == 15)
				m_GI[ec].full_display_angle+=45;
			else
				m_GI[ec].full_display_angle+=10;

			if (m_GI[ec].full_display_angle>=360)
				m_GI[ec].full_display_angle-=360;

			if (m_GI[ec].process == 1)
			{
				Rx=m_deltaPos.x-m_GI[ec].vecB_now.x;
				Rz=m_deltaPos.z-m_GI[ec].vecB_now.z;

				Get_Angle(Rx,Rz,&m_GI[ec].RotStart);
			}


			b = m_GI[ec].process;
			b /= float(m_GI[ec].flag1[1]);


			if (b > 1.05f)
			{
				m_GI[ec].alphaB-=20;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}
			else
			{
				int sinH;

				if (b>1)
					b=1;

				a = 180 * b;
				sinH = int(a);
				m_GI[ec].vecB_pre.y += m_GI[ec].height[2];

				if (m_GI[ec].flag1[2] == 0)
					m_GI[ec].vecB_now.y = m_GI[ec].vecB_pre.y - ( g_sinTable[sinH]*10.0f );
				else if (m_GI[ec].flag1[2] == 15)
					m_GI[ec].vecB_now.y = m_GI[ec].vecB_pre.y - ( g_sinTable[sinH] );
				else
					m_GI[ec].vecB_now.y = m_GI[ec].vecB_pre.y - ( g_sinTable[sinH]*30.0f );

				m_GI[ec].vecB_now.x+=m_GI[ec].height[1]*g_cosTable[m_GI[ec].RotStart];
				m_GI[ec].vecB_now.z+=m_GI[ec].height[1]*g_sinTable[m_GI[ec].RotStart];
			}

		}
	}

}

void	CEffectPrim :: PrimGroundSample()
{
	int ec;
	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].RotStart++;
			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;
			m_GI[ec].max_height = 1.5f+(g_sinTable[m_GI[ec].RotStart]*0.5f);
		}
	}
}

void	CEffectPrim :: PrimChemical()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;



	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process<=10)
				m_GI[ec].alphaB+=18;
			else if (m_GI[ec].process>20)
			{
				m_GI[ec].alphaB-=15;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}

		}
	}
}

void	CEffectPrim :: PrimChemical2()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;

	m_pos = m_master->m_pos;


	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process<=20)
				m_GI[ec].alphaB+=4;
			else if (m_GI[ec].process>50)
			{
				m_GI[ec].alphaB-=2;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}
		}
	}
}

void	CEffectPrim :: PrimGiExplosion()
{

	int i,ec;
	int middle,m2;






	m_pos = m_master->m_pos;

	middle=(E_DIVISION-1)/2;
	m2=90/middle;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			m_GI[ec].RotStart+=4;
			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			if (m_GI[ec].process<=30)
			{
				m_GI[ec].alphaB+=8;
				if (m_GI[ec].alphaB>160)
					m_GI[ec].alphaB=160;
			}
			m_GI[ec].distance-=0.2f;


			for (i = 0 ; i<E_DIVISION ; i++)
			{
				int SinLimit;

				if (m_GI[ec].flag1[i]==0)
				{
					SinLimit=90+((i-middle)*m2);

					if (m_GI[ec].process<=90)
						m_GI[ec].height[i]=m_GI[ec].max_height*g_sinTable[SinLimit]*g_sinTable[m_GI[ec].process];

					if (m_GI[ec].height[i] > m_GI[ec].max_height*g_sinTable[SinLimit])
					{
						m_GI[ec].height[i] = m_GI[ec].max_height*g_sinTable[SinLimit];
						m_GI[ec].flag1[i]=1;
					}

					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
				}
			}
		}
	}
}

void	CEffectPrim :: PrimChemical3()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;

	int ec;
	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process<=10)
			{
				if (m_GI[ec].flag1[0] < 10)
					m_GI[ec].alphaB+=12;
				else
					m_GI[ec].alphaB+=20;
			}

			if (m_GI[ec].flag1[0] >= 10)
				m_GI[ec].height[3]+=0.05f;

			m_GI[ec].height[1]+=m_GI[ec].height[3];
			if (m_GI[ec].height[1]>=200)
				m_GI[ec].height[1]-=200;

			if (m_GI[ec].process>20)
			{
				if (m_GI[ec].flag1[0] < 10)
					m_GI[ec].alphaB-=4;
				else
					m_GI[ec].alphaB-=2;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}
		}
	}
}

void	CEffectPrim :: PrimCallZone()
{
	m_pos = m_master->m_pos;


	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].process<=10)
			{
				if (m_GI[ec].alphaB<100)
					m_GI[ec].alphaB+=1;
			}

		}
	}
}

void	CEffectPrim :: PrimBlind()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;

	m_pos = m_master->m_pos;


	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;

			if (m_GI[ec].alphaB<255)
				m_GI[ec].alphaB+=1;
		}
	}
}

void	CEffectPrim :: PrimPoison()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;

	m_pos = m_master->m_pos;


	int ec;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process++;



			if (m_GI[ec].max_height>=30)
				m_GI[ec].max_height-=30.0f;

			if (m_GI[ec].flag1[10] == 1)
			{
				if (m_GI[ec].process<10)
				{
					if (m_GI[ec].alphaB<=45)
						m_GI[ec].alphaB+=10;
				}
				else if (m_GI[ec].process>65)
				{
					if (m_GI[ec].alphaB>=10)
						m_GI[ec].alphaB-=10;
				}
			}
			else
			{
				if (m_GI[ec].alphaB<=45)
					m_GI[ec].alphaB+=5;
			}
		}
	}
}

void	CRagEffect :: DEFENDER(char *tName)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_3DCASTING_2);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 40.0f;
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 8.0f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 39.0f;
		g_prim->m_GI[ec].RotStart = 270;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 7.9f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 38.0f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 7.8f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 37.0f;
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 7.7f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}
	}
}

void	CRagEffect :: SAINTCASTING(short time, char *tName, char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		if (m_duration < 56)
			m_duration = 56;

		{
			g_prim = LaunchEffectPrim(PP_GI_1);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

			if (F1 == 1)
				g_prim->m_size=8;
			else if ( F1 == 2 )
				g_prim->m_size=5;
			else if ( F1 == 3 )
				g_prim->m_size=10;
			else if ( F1 == 4 )
				g_prim->m_size=11;
			else if ( F1 == 5 )
				g_prim->m_size=4;
			else if ( F1 == 6 )
				g_prim->m_size=12;
			else
				g_prim->m_size=5;



			int ec,i;

			ec=0;
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].full_display_angle = 360;
			if (F1 == 2)
				g_prim->m_GI[ec].max_height = 25.0f;
			else if (F1 == 22)
				g_prim->m_GI[ec].max_height = 15.0f;
			else
				g_prim->m_GI[ec].max_height = 20.0f;
			g_prim->m_GI[ec].RotStart = 180;
			g_prim->m_GI[ec].alphaB = time+135;
			g_prim->m_GI[ec].distance = 4.1f;
			g_prim->m_GI[ec].rise_angle=80;
			g_prim->m_GI[ec].alphaT=F1;
			g_prim->m_GI[ec].vecB_now.y=0;

			ec=1;
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].full_display_angle = 360;
			if (F1 == 2)
				g_prim->m_GI[ec].max_height = 24.0f;
			else if (F1 == 22)
				g_prim->m_GI[ec].max_height = 14.0f;
			else
				g_prim->m_GI[ec].max_height = 19.0f;
			g_prim->m_GI[ec].RotStart = 270;
			g_prim->m_GI[ec].alphaB = time+90;
			g_prim->m_GI[ec].distance = 4.1f;
			g_prim->m_GI[ec].rise_angle=80;
			g_prim->m_GI[ec].alphaT=F1;
			g_prim->m_GI[ec].vecB_now.y=0;

			ec=2;
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].full_display_angle = 360;
			if (F1 == 2)
				g_prim->m_GI[ec].max_height = 23.0f;
			else if (F1 == 22)
				g_prim->m_GI[ec].max_height = 13.0f;
			else
				g_prim->m_GI[ec].max_height = 18.0f;
			g_prim->m_GI[ec].RotStart = 0;
			g_prim->m_GI[ec].alphaB = time+45;
			g_prim->m_GI[ec].distance = 4.1f;
			g_prim->m_GI[ec].rise_angle=80;
			g_prim->m_GI[ec].alphaT=F1;
			g_prim->m_GI[ec].vecB_now.y=0;

			ec=3;
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].full_display_angle = 360;
			if (F1 == 2)
				g_prim->m_GI[ec].max_height = 22.0f;
			else if (F1 == 22)
				g_prim->m_GI[ec].max_height = 12.0f;
			else
				g_prim->m_GI[ec].max_height = 17.0f;
			g_prim->m_GI[ec].RotStart = 90;
			g_prim->m_GI[ec].alphaB = time;
			g_prim->m_GI[ec].distance = 4.1f;
			g_prim->m_GI[ec].rise_angle=80;
			g_prim->m_GI[ec].alphaT=F1;
			g_prim->m_GI[ec].vecB_now.y=0;


			for (ec=0;ec<4;ec++)
			{
				for (i = 0 ; i<E_DIVISION ; i++)
				{
					g_prim->m_GI[ec].height[i] = 0.0f;
					g_prim->m_GI[ec].flag1[i] = 0;
				}

				if (m_duration<=60)
				{
					g_prim->m_GI[ec].alphaB=0;
					g_prim->m_GI[ec].process=-ec*5;
				}
			}
		}
	}
}

void	CRagEffect :: CoupleCasting(char *tName)
{
	m_pos = m_master->m_pos;
	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_BeginSpell.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		SAINTCASTING(45,tName,3);
		SAINTCASTING(25,tName,3);
	}
}

void	CRagEffect :: HeartCasting(char *tName , float x , float z)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{

		g_prim = LaunchEffectPrim(PP_HEAL_LOW);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=10;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 12.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;

		g_prim->m_GI[ec].distance = 2.3f;

		g_prim->m_GI[ec].rise_angle=86;
		g_prim->m_GI[ec].alphaT=m_duration-25;
		g_prim->m_GI[ec].vecB_now.x=x;
		g_prim->m_GI[ec].vecB_now.z=z;
		g_prim->m_GI[ec].vecB_now.y=1234.5f;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 33;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;

		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 25.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;

		g_prim->m_GI[ec].distance = 2.0f;

		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT=m_duration-25;
		g_prim->m_GI[ec].vecB_now.x=x;
		g_prim->m_GI[ec].vecB_now.z=z;
		g_prim->m_GI[ec].vecB_now.y=1234.5f;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 33;
		}

		g_prim->m_GI[2].life = 0;
		g_prim->m_GI[3].life = 0;
	}
}

void	CRagEffect :: WIND()
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_WIND);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\cloud11.tga", false);




		g_prim->m_size=4;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 90;
		g_prim->m_GI[ec].max_height = 8.0f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 3.0f;
		g_prim->m_GI[ec].rise_angle=90;


		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 3.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		g_prim->m_GI[ec].alphaT = 0;

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 70;
		g_prim->m_GI[ec].max_height = 8.0f;
		g_prim->m_GI[ec].RotStart = 10;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 3.2f;
		g_prim->m_GI[ec].rise_angle=90;


		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 2.5f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		g_prim->m_GI[ec].alphaT = 0;

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 50;
		g_prim->m_GI[ec].max_height = 8.0f;
		g_prim->m_GI[ec].RotStart = 20;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 3.4f;
		g_prim->m_GI[ec].rise_angle=90;


		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 2.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		g_prim->m_GI[ec].alphaT = 0;

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 30;
		g_prim->m_GI[ec].max_height = 8.0f;
		g_prim->m_GI[ec].RotStart = 30;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 3.6f;
		g_prim->m_GI[ec].rise_angle=90;


		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 1.5f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		g_prim->m_GI[ec].alphaT = 0;





		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("_heal_effect.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
}

void	CRagEffect :: VOLCANO(char *tName, char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_GI_2);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;



		int ec,i;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].full_display_angle = 360;
			if (F1 == 2)
			{
				g_prim->m_GI[ec].max_height = 15.0f;
				g_prim->m_GI[ec].rise_angle=70;
			}
			else
			{
				g_prim->m_GI[ec].max_height = 25.0f;
				g_prim->m_GI[ec].rise_angle=80;
			}
			g_prim->m_GI[ec].RotStart = 90*ec;
			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].distance = ec+1.0f;
			g_prim->m_GI[ec].alphaT=F1;
			g_prim->m_GI[ec].vecB_now.y=0;

			for (i = 0 ; i<E_DIVISION ; i++)
			{
				g_prim->m_GI[ec].height[i] = 0.0f;
				g_prim->m_GI[ec].flag1[i] = 0;
			}
		}
	}
}

void	CRagEffect :: VOLCANO2(char *tName, char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_GI_2);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;



		int ec,i;

		g_prim->m_accel=99;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].full_display_angle = 360;
			g_prim->m_GI[ec].max_height = 25.0f;
			g_prim->m_GI[ec].RotStart = 90*ec;
			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].distance = ec+1.0f;
			g_prim->m_GI[ec].rise_angle=80;
			g_prim->m_GI[ec].alphaT=F1;
			g_prim->m_GI[ec].vecB_now.y=0;

			for (i = 0 ; i<E_DIVISION ; i++)
			{
				g_prim->m_GI[ec].height[i] = 0.0f;
				g_prim->m_GI[ec].flag1[i] = 0;
			}
		}
	}
}

void CRagEffect :: Gravitation()
{
	if (m_stateCnt == 0)
	{
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		gameMode->SendMsg(MM_QUAKE, 1);
		float side = 1.0f, front = 0.24f, latitude = 0.2f;
		gameMode->SendMsg(MM_SETQUAKEINFO, (INT_PTR)&side, (INT_PTR)&front, (INT_PTR)&latitude);

		for (int i=0;i<4;i++)
		{
			int		angle;
			float	sn, cs, length;

			g_prim = LaunchEffectPrim(PP_3DQUADHORN2);
			g_prim->m_duration = m_duration;
			g_prim->m_pattern = PTQH_SPEEDLIMIT;

			angle = random(360);
			length = (float)(random(4) + 1) / 10.0f;
			sn = GetSin(angle);
			cs = GetCos(angle);

			g_prim->m_deltaPos2.Set(0, 10, 0);
			g_prim->m_size = (float)(random(50) + 300) / 100.0f;
			g_prim->m_heightSize = 18.0f;
			g_prim->m_longitude = random(360);
			g_prim->m_latitude = random(40) + 60;
			g_prim->m_matrix.MakeXRotation(GetRadian(g_prim->m_latitude));
			g_prim->m_matrix.AppendYRotation(GetRadian(g_prim->m_longitude));
			g_prim->m_rotMat = g_prim->m_matrix;
			g_prim->m_speed = 1.0f;
			g_prim->m_accel = 0.01f;
			g_prim->m_alpha = 20;

			g_prim->m_count = 15;
			g_prim->m_ChangePoint = 12;
			g_prim->m_ChangeSpeed = -1.2f;
			g_prim->m_ChangeAccel = 0.0f;
			g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

			if (i<2)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\stone.bmp", false);
			else
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ice.tga", false);
		}
	}
}

void CRagEffect :: Bottom_Volcano(char *tName, char PW , float x , float z)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{


		g_prim = LaunchEffectPrim(PP_GI_4);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=PW;



		int ec,i;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 15.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = random(100);
		g_prim->m_GI[ec].distance = 2.6f;
		g_prim->m_GI[ec].rise_angle=80;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

	}
}

void CRagEffect :: Bottom_Magnus(char *tName, char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_RECT_UP);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=1;



		int ec;

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].full_display_angle = random(360);
			if (F1 == 0)
				g_prim->m_GI[ec].max_height = 50.0f;
			else if (F1 == 1)
				g_prim->m_GI[ec].max_height = 16.0f;
			else if (F1 == 2)
				g_prim->m_GI[ec].max_height = 32.0f;

			g_prim->m_GI[ec].RotStart = 0;

			if (F1 == 0)
			{
				g_prim->m_GI[ec].alphaB = 30;
				g_prim->m_GI[ec].alphaT = 864;
			}
			else if (F1 == 1)
			{
				g_prim->m_GI[ec].alphaB = 120;
				g_prim->m_GI[ec].alphaT = 1880;
			}
			else if (F1 == 2)
			{
				g_prim->m_GI[ec].alphaB = 30;
				g_prim->m_GI[ec].alphaT = 29999;
			}

			if (F1 == 0)
			{
				g_prim->m_GI[ec].height[0] = 5.0f;
				g_prim->m_GI[ec].height[1] = 5.0f;
			}
			else
			{
				g_prim->m_GI[ec].height[0] = 2.5f;
				g_prim->m_GI[ec].height[1] = 2.5f;
			}

			g_prim->m_GI[ec].height[2] = 0.0f;

			g_prim->m_GI[ec].height[3] = F1;
		}
		ec=1;
		g_prim->m_GI[ec].life = 0;
		ec=2;
		g_prim->m_GI[ec].life = 0;
		ec=3;
		g_prim->m_GI[ec].life = 0;
	}
}

void CRagEffect :: Basilica(char *tName,char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_RECT_UP2);

		g_prim->m_renderFlag = RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;



		int ec;

		for (ec=0;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;

		for (ec=0;ec<2;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;

			if ( F1 == 0 )
				g_prim->m_GI[ec].max_height = 30.0f+ec;
			else
				g_prim->m_GI[ec].max_height = 20.0f+ec;

			if ( F1 == 0 )
			{
				g_prim->m_GI[ec].alphaB = 65;
				g_prim->m_GI[ec].alphaT = 2415;
			}
			else
			{
				g_prim->m_GI[ec].alphaB = 32;
				g_prim->m_GI[ec].alphaT = 2385;
			}

			if (F1 == 0)
				g_prim->m_GI[ec].height[0] = 12.5f+(ec*0.4f);
			else
				g_prim->m_GI[ec].height[0] = 14.0f+(ec*0.4f);

			g_prim->m_GI[ec].height[1] = g_prim->m_GI[ec].height[0];
			g_prim->m_GI[ec].height[4] = F1;

		}

		for (ec=2;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;

			if ( F1 == 0 )
				g_prim->m_GI[ec].max_height = 30.0f+ec;
			else
				g_prim->m_GI[ec].max_height = 20.0f+ec;

			if (F1 == 0)
				g_prim->m_GI[ec].distance = ((ec-2)*53.2f);
			else
				g_prim->m_GI[ec].distance = ((ec-2)*53.2f)+26.6f;;

			if (F1 == 0)
				g_prim->m_GI[ec].height[0] = 12.5f+0.8f;
			else
				g_prim->m_GI[ec].height[0] = 14.0f+0.8f;

			g_prim->m_GI[ec].height[1] = g_prim->m_GI[ec].height[0];

			g_prim->m_GI[ec].height[3] = 25.0f+1.6f;

			if ( F1 == 0 )
			{
				g_prim->m_GI[ec].alphaB = 15;
				g_prim->m_GI[ec].alphaT = 2415;
			}
			else
			{
				g_prim->m_GI[ec].alphaB = 15;
				g_prim->m_GI[ec].alphaT = 2385;
			}
			g_prim->m_GI[ec].alphaT = 2415;
			g_prim->m_GI[ec].height[4] = F1;
		}



	}
}

void CRagEffect :: Bottom_Light(char *tName, char PW)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{


		g_prim = LaunchEffectPrim(PP_GI_5);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=PW;



		int ec,i;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = 25.0f + random(10);
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 50-random(10);

		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT = random(360);
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

	}
}

void CRagEffect :: Bottom_Vertical(char *tName, char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{


		g_prim = LaunchEffectPrim(PP_BOTTOM2);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			if (F1 == 0)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process = 0;
				g_prim->m_GI[ec].full_display_angle = random(360);


				g_prim->m_GI[ec].vecB_pre.x=random(40)-20;
				g_prim->m_GI[ec].vecB_pre.z=random(40)-20;

				g_prim->m_GI[ec].vecB_now.x=0;
				g_prim->m_GI[ec].vecB_now.z=0;

				g_prim->m_GI[ec].alphaB = 15;
			}
			else if (F1 == 1 || F1 == 2)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process = 0;
				g_prim->m_GI[ec].full_display_angle = random(360);

				if (ec<2)
				{
					g_prim->m_GI[ec].vecB_pre.x=random(7)-3.0f;
					g_prim->m_GI[ec].vecB_pre.z=-3.0f;

					g_prim->m_GI[ec].vecB_now.x=-g_prim->m_GI[ec].vecB_pre.x;
					g_prim->m_GI[ec].vecB_now.z=3.0f;
				}
				else
				{
					g_prim->m_GI[ec].vecB_pre.x=-3.0f;
					g_prim->m_GI[ec].vecB_pre.z=random(7)-3.0f;

					g_prim->m_GI[ec].vecB_now.x=3.0f;
					g_prim->m_GI[ec].vecB_now.z=-g_prim->m_GI[ec].vecB_pre.z;
				}

				g_prim->m_GI[ec].alphaB = 50;

				g_prim->m_GI[ec].height[0]=F1;
			}
			else if ( F1 == 3 || F1 == 4 )
			{
				if (ec<2)
					g_prim->m_GI[ec].life = 1;
				else
					g_prim->m_GI[ec].life = 0;

				g_prim->m_GI[ec].process = 0;
				g_prim->m_GI[ec].full_display_angle = random(360);

				if (ec<1)
				{
					g_prim->m_GI[ec].vecB_pre.x=6.0f;
					g_prim->m_GI[ec].vecB_pre.z=0;

					g_prim->m_GI[ec].vecB_now.x=-6.0f;
					g_prim->m_GI[ec].vecB_now.z=0;
				}
				else
				{
					g_prim->m_GI[ec].vecB_pre.x=0;
					g_prim->m_GI[ec].vecB_pre.z=-6.0f;

					g_prim->m_GI[ec].vecB_now.x=0;
					g_prim->m_GI[ec].vecB_now.z=6.0f;
				}

				g_prim->m_GI[ec].alphaB = 100;

				g_prim->m_GI[ec].height[0]=F1;
			}
		}
	}
}

void CRagEffect :: Bottom_Music(char *tName , char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{


		g_prim = LaunchEffectPrim(PP_BOTTOM);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];


		if ( F1 == 5 )
		{
			int a;
			a=random(3);
			if (a==0)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\\xb7\xb9\xb5\xe5\xc1\xaa\xbd\xba\xc5\xe6.bmp", false);
			else if (a==1)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\\xba\xed\xb7\xe7\xc1\xaa\xbd\xba\xc5\xe6.bmp", false);
			else
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\\xbf\xbb\xb7\xce\xbf\xec\xc1\xaa\xbd\xba\xc5\xe6.bmp", false);
		}
		else if ( F1 == 2 )
		{
			int a;
			a=random(2);
			if (a==0)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\melody_a.bmp", false);
			else
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\melody_b.bmp", false);
		}
		else if ( F1 == 8 )
		{
			int a;
			a=random(8);
			if (a==0)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\spell_01.bmp", false);
			else if (a==1)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\spell_02.bmp", false);
			else if (a==2)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\spell_03.bmp", false);
			else if (a==3)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\spell_04.bmp", false);
			else if (a==4)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\spell_05.bmp", false);
			else if (a==5)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\spell_06.bmp", false);
			else if (a==6)
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\spell_07.bmp", false);
			else
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\spell_08.bmp", false);
		}
		else
			g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);


		g_prim->m_size=0;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		ec=0;

		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process=0;

		if (F1 == 1)
			g_prim->m_GI[ec].distance = 5.0f;
		else if (F1 == 4)

			g_prim->m_GI[ec].distance = 15.0f;
		else if (F1 == 6)
			g_prim->m_GI[ec].distance = 8.0f;
		else if (F1 == 9)
			g_prim->m_GI[ec].distance = 1.5f;
		else
			g_prim->m_GI[ec].distance = 3.0f;

		g_prim->m_GI[ec].height[8] = 0;
		g_prim->m_GI[ec].alphaB = 200;

		g_prim->m_GI[ec].vecB_now.x=m_pos.x;
		g_prim->m_GI[ec].vecB_now.z=m_pos.z;

		g_prim->m_GI[ec].vecB_pre.y = m_pos.y-6.0f;

		g_prim->m_GI[ec].RotStart=random(360);

		g_prim->m_GI[ec].full_display_angle = 45;
		g_prim->m_GI[ec].flag1[0] = 0;
		g_prim->m_GI[ec].flag1[2] = 2;

		g_prim->m_GI[ec].height[7] = 0;
		if (F1 == 4)
			g_prim->m_GI[ec].height[9] = 10;

		if (F1 == 2)
		{
			for (ec=1;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].alphaB = 0;
			}
		}

		if (F1 == 3)
			g_prim->m_GI[ec].flag1[2] = 3;
		else if ( F1 == 5 || F1 == 6 )
			g_prim->m_GI[ec].flag1[2] = 4;
		else if ( F1 == 7 )
			g_prim->m_GI[ec].flag1[2] = 7;
		else if ( F1 == 9 )
		{
			g_prim->m_GI[ec].flag1[2] = 9;
			g_prim->m_GI[ec].flag1[3] = 6;
		}

	}
}

void CRagEffect :: EffectTextureSet_Animation(char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		m_pos = m_master->m_pos;


		g_prim = LaunchEffectPrim(PP_EFFECTTEXTURE_ANI);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		if (F1 == 0)
		{
			g_prim->m_totalTexture = 9;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\dust01.bmp", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\dust02.bmp", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\dust03.bmp", false);
			g_prim->m_texture[3] = g_texMgr.GetTexture("effect\\dust04.bmp", false);
			g_prim->m_texture[4] = g_texMgr.GetTexture("effect\\dust05.bmp", false);
			g_prim->m_texture[5] = g_texMgr.GetTexture("effect\\dust06.bmp", false);
			g_prim->m_texture[6] = g_texMgr.GetTexture("effect\\dust07.bmp", false);
			g_prim->m_texture[7] = g_texMgr.GetTexture("effect\\dust08.bmp", false);
			g_prim->m_texture[8] = g_texMgr.GetTexture("effect\\dust09.bmp", false);
		}
		else if (F1 == 2)
		{
			g_prim->m_totalTexture = 13;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\torch_green01.bmp", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\torch_green02.bmp", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\torch_green03.bmp", false);
			g_prim->m_texture[3] = g_texMgr.GetTexture("effect\\torch_green04.bmp", false);
			g_prim->m_texture[4] = g_texMgr.GetTexture("effect\\torch_green05.bmp", false);
			g_prim->m_texture[5] = g_texMgr.GetTexture("effect\\torch_green06.bmp", false);
			g_prim->m_texture[6] = g_texMgr.GetTexture("effect\\torch_green07.bmp", false);
			g_prim->m_texture[7] = g_texMgr.GetTexture("effect\\torch_green08.bmp", false);
			g_prim->m_texture[8] = g_texMgr.GetTexture("effect\\torch_green09.bmp", false);
			g_prim->m_texture[9] = g_texMgr.GetTexture("effect\\torch_green10.bmp", false);
			g_prim->m_texture[10] = g_texMgr.GetTexture("effect\\torch_green11.bmp", false);
			g_prim->m_texture[11] = g_texMgr.GetTexture("effect\\torch_green12.bmp", false);
			g_prim->m_texture[12] = g_texMgr.GetTexture("effect\\torch_green13.bmp", false);
		}
		else if (F1 == 1)
		{
			g_prim->m_totalTexture = 13;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\torch_red01.bmp", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\torch_red02.bmp", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\torch_red03.bmp", false);
			g_prim->m_texture[3] = g_texMgr.GetTexture("effect\\torch_red04.bmp", false);
			g_prim->m_texture[4] = g_texMgr.GetTexture("effect\\torch_red05.bmp", false);
			g_prim->m_texture[5] = g_texMgr.GetTexture("effect\\torch_red06.bmp", false);
			g_prim->m_texture[6] = g_texMgr.GetTexture("effect\\torch_red07.bmp", false);
			g_prim->m_texture[7] = g_texMgr.GetTexture("effect\\torch_red08.bmp", false);
			g_prim->m_texture[8] = g_texMgr.GetTexture("effect\\torch_red09.bmp", false);
			g_prim->m_texture[9] = g_texMgr.GetTexture("effect\\torch_red10.bmp", false);
			g_prim->m_texture[10] = g_texMgr.GetTexture("effect\\torch_red11.bmp", false);
			g_prim->m_texture[11] = g_texMgr.GetTexture("effect\\torch_red12.bmp", false);
			g_prim->m_texture[12] = g_texMgr.GetTexture("effect\\torch_red13.bmp", false);
		}
		else if (F1 == 3)
		{
			g_prim->m_totalTexture = 13;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\torch_violet01.bmp", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\torch_violet02.bmp", false);
			g_prim->m_texture[2] = g_texMgr.GetTexture("effect\\torch_violet03.bmp", false);
			g_prim->m_texture[3] = g_texMgr.GetTexture("effect\\torch_violet04.bmp", false);
			g_prim->m_texture[4] = g_texMgr.GetTexture("effect\\torch_violet05.bmp", false);
			g_prim->m_texture[5] = g_texMgr.GetTexture("effect\\torch_violet06.bmp", false);
			g_prim->m_texture[6] = g_texMgr.GetTexture("effect\\torch_violet07.bmp", false);
			g_prim->m_texture[7] = g_texMgr.GetTexture("effect\\torch_violet08.bmp", false);
			g_prim->m_texture[8] = g_texMgr.GetTexture("effect\\torch_violet09.bmp", false);
			g_prim->m_texture[9] = g_texMgr.GetTexture("effect\\torch_violet10.bmp", false);
			g_prim->m_texture[10] = g_texMgr.GetTexture("effect\\torch_violet11.bmp", false);
			g_prim->m_texture[11] = g_texMgr.GetTexture("effect\\torch_violet12.bmp", false);
			g_prim->m_texture[12] = g_texMgr.GetTexture("effect\\torch_violet13.bmp", false);
		}

		g_prim->m_size=0;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		ec=0;

		g_prim->m_GI[ec].process=0;
		g_prim->m_GI[ec].life = 1;

		if (m_param[0] <= 0)
			g_prim->m_GI[ec].distance = 20.0f;
		else
			g_prim->m_GI[ec].distance = m_param[0];

		{
			g_prim->m_GI[ec].vecB_pre.y = m_pos.y-10.0f;
			g_prim->m_GI[ec].flag1[2] = 4;
			g_prim->m_GI[ec].height[10] = 0;
			g_prim->m_GI[ec].full_display_angle = 135;
		}

		g_prim->m_GI[ec].height[8] = 0;

		if (F1 == 0)
			g_prim->m_GI[ec].alphaB = 100;
		else
			g_prim->m_GI[ec].alphaB = 130;

		g_prim->m_GI[ec].vecB_now.x=m_pos.x;
		g_prim->m_GI[ec].vecB_now.z=m_pos.z;

		g_prim->m_GI[ec].RotStart=random(360);
		g_prim->m_GI[ec].flag1[0] = 0;

		g_prim->m_GI[ec].flag1[4] = F1;

	}
}

void CRagEffect :: EffectTextureSet(char *tName , char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		m_pos = m_master->m_pos;


		g_prim = LaunchEffectPrim(PP_EFFECTTEXTURE);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		ec=0;

		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process=0;
		if (F1 == 1)
		{
			g_prim->m_GI[ec].distance = 8.0f;
			g_prim->m_GI[ec].vecB_pre.y = m_pos.y-30.0f;
			g_prim->m_GI[ec].flag1[2] = 0;
			g_prim->m_GI[ec].height[10] = 1;
			g_prim->m_GI[ec].full_display_angle = 120;
		}
		else if (F1 == 2)
		{
			g_prim->m_GI[ec].distance = 10.0f;
			g_prim->m_GI[ec].vecB_pre.y = m_pos.y-10.0f;
			g_prim->m_GI[ec].flag1[2] = 2;
			g_prim->m_GI[ec].height[10] = 0;
			g_prim->m_GI[ec].full_display_angle = 135;
		}
		else if (F1 == 3)
		{
			g_prim->m_GI[ec].distance = 8.0f;
			g_prim->m_GI[ec].vecB_pre.y = m_pos.y-30.0f;
			g_prim->m_GI[ec].flag1[2] = 3;
			g_prim->m_GI[ec].height[10] = 2;
			g_prim->m_GI[ec].full_display_angle = 155;
		}
		else if (F1 == 4)
		{
			g_prim->m_GI[ec].distance = 6.0f;
			g_prim->m_GI[ec].vecB_pre.y = m_pos.y-30.0f;
			g_prim->m_GI[ec].flag1[2] = 0;
			g_prim->m_GI[ec].height[10] = 0;
			g_prim->m_GI[ec].full_display_angle = 135;
		}
		else if (F1 >= 10 && F1 <= 11)
		{
			if (m_param[0] <= 0)
				g_prim->m_GI[ec].distance = 70.0f;
			else
				g_prim->m_GI[ec].distance = m_param[0]*0.7f;

			g_prim->m_GI[ec].vecB_pre.y = m_pos.y;
			g_prim->m_GI[ec].flag1[2] = 10;
			g_prim->m_GI[ec].height[10] = 0;
			g_prim->m_GI[ec].full_display_angle = random(360);
		}
		else if (F1 == 12)
		{
			if (m_param[0] <= 0)
				g_prim->m_GI[ec].distance = 120.0f;
			else
				g_prim->m_GI[ec].distance = m_param[0]*1.2f;

			g_prim->m_GI[ec].vecB_pre.y = m_pos.y;
			g_prim->m_GI[ec].flag1[2] = 12;
			g_prim->m_GI[ec].height[10] = 0;
			g_prim->m_GI[ec].full_display_angle = random(360);
		}
		else if (F1 == 13)
		{
			if (m_param[0] <= 0)
				g_prim->m_GI[ec].distance = 300.0f;
			else
				g_prim->m_GI[ec].distance = m_param[0]*3.0f;

			g_prim->m_GI[ec].vecB_pre.y = m_pos.y;
			g_prim->m_GI[ec].flag1[2] = 12;
			g_prim->m_GI[ec].height[10] = 0;
			g_prim->m_GI[ec].full_display_angle = random(360);
		}
		else if (F1 == 14)
		{
			if (m_param[0] <= 0)
				g_prim->m_GI[ec].distance = 30.0f;
			else
				g_prim->m_GI[ec].distance = m_param[0];

			g_prim->m_GI[ec].vecB_pre.y = m_pos.y;
			g_prim->m_GI[ec].flag1[2] = 4;
			g_prim->m_GI[ec].height[10] = 0;
			g_prim->m_GI[ec].full_display_angle = random(360);
		}
		else if (F1 == 100)
		{
			g_prim->m_renderFlag = 0x101;
			g_prim->m_GI[ec].distance = 50.0f;
			g_prim->m_GI[ec].flag1[2] = 100;
			g_prim->m_GI[ec].height[10] = 0;
			g_prim->m_GI[ec].full_display_angle = 45;
		}
		else
		{
			g_prim->m_GI[ec].distance = 5.0f;
			g_prim->m_GI[ec].vecB_pre.y = m_pos.y-10.0f;
			g_prim->m_GI[ec].flag1[2] = 2;
			g_prim->m_GI[ec].height[10] = 0;
			g_prim->m_GI[ec].full_display_angle = 135;
		}

		g_prim->m_GI[ec].height[8] = 0;
		g_prim->m_GI[ec].alphaB = 0;

		if (F1 >= 100)
		{
			g_prim->m_GI[ec].vecB_now.x=m_pos.x;
			g_prim->m_GI[ec].vecB_now.z=m_pos.z+100.0f;
			g_prim->m_GI[ec].vecB_pre.y=m_pos.y+100.0f;
		}
		else
		{
			g_prim->m_GI[ec].vecB_now.x=m_pos.x;
			g_prim->m_GI[ec].vecB_now.z=m_pos.z;
		}

		g_prim->m_GI[ec].RotStart=random(360);
		g_prim->m_GI[ec].flag1[0] = 0;

		if (F1 == 4)
			g_prim->m_GI[ec].flag1[4] = 1;
		else
			g_prim->m_GI[ec].flag1[4] = F1;

	}
}

void CRagEffect :: PeongMiddle(char *tName)
{


	if (m_stateCnt  == 35)
	{
		m_pos = m_master->m_pos;


		g_prim = LaunchEffectPrim(PP_EFFECTTEXTURE2);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		ec=0;

		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process=0;
		g_prim->m_GI[ec].distance = 22.0f;

		g_prim->m_GI[ec].height[8] = 0;
		g_prim->m_GI[ec].alphaB = 0;

		g_prim->m_GI[ec].vecB_now.x=m_pos.x;
		g_prim->m_GI[ec].vecB_now.z=m_pos.z;
		g_prim->m_GI[ec].vecB_pre.y = m_pos.y-8.0f;

		g_prim->m_GI[ec].RotStart=random(360);
		g_prim->m_GI[ec].full_display_angle = 135;
		g_prim->m_GI[ec].flag1[0] = 0;
		g_prim->m_GI[ec].flag1[2] = 2;

		g_prim->m_GI[ec].height[10] = 0;
	}
}

void	CRagEffect :: PeongUp(char *tName,char F1)
{
	float cs1,sn1;
	short start = -1;

	if (F1 == 0)
		start = 35;
	else if (F1 == 1)
		start = 0;

	if (m_stateCnt == start)
	{
		g_prim = LaunchEffectPrim(PP_PEONGUP);

		g_prim->m_renderFlag = RF_EFFECT;
		g_prim->m_duration = m_duration;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);
		g_prim->m_size=0;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		if ( F1 == 0 )
		{
			for (ec=0;ec<1;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].max_height=0.10f+(random(16)*0.01f);
				g_prim->m_GI[ec].distance=3.0f+(random(11)*0.1f);
				g_prim->m_GI[ec].process = -random(26);

				g_prim->m_GI[ec].alphaB = 0;
				g_prim->m_GI[ec].vecB_pre.y=-random(8);

				int angle;
				angle=random(360);

				cs1 = g_cosTable[angle];
				sn1 = g_sinTable[angle];

				g_prim->m_GI[ec].vecB_pre.x=cs1*2.5f;
				g_prim->m_GI[ec].vecB_pre.z=sn1*2.5f;

				g_prim->m_GI[ec].height[0]=random(360);
				g_prim->m_GI[ec].height[1]=random(360);
				g_prim->m_GI[ec].height[2]=random(360);
				g_prim->m_GI[ec].height[3]=random(360);

				g_prim->m_GI[ec].flag1[0]=1;
			}
		}
		else if ( F1 == 1 )
		{
			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].max_height=0.08f+(random(11)*0.005f);
				g_prim->m_GI[ec].distance=2.0f+(random(11)*0.05f);
				g_prim->m_GI[ec].process = -random(26);

				g_prim->m_GI[ec].alphaB = 0;
				g_prim->m_GI[ec].vecB_pre.y=-random(8);

				int angle;
				angle=random(360);

				cs1 = g_cosTable[angle];
				sn1 = g_sinTable[angle];

				g_prim->m_GI[ec].vecB_pre.x=cs1*3.5f;
				g_prim->m_GI[ec].vecB_pre.z=sn1*3.5f;

				g_prim->m_GI[ec].height[0]=random(360);
				g_prim->m_GI[ec].height[1]=random(360);
				g_prim->m_GI[ec].height[2]=random(360);
				g_prim->m_GI[ec].height[3]=random(360);

				g_prim->m_GI[ec].flag1[0]=2;
			}
		}
	}
}

void CRagEffect :: Peong(char *tName , short time)
{

	if (m_stateCnt  == 35)
	{
		g_prim = LaunchEffectPrim(PP_PEONG);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			float cs1,sn1,dist;
			short angle;

			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].distance=2.5f+(random(11)*0.2f);

			g_prim->m_GI[ec].alphaB = 0;

			dist=4.0f;
			angle = random(90)+time;
			if (angle>=360)
				angle-=360;
			cs1 = g_cosTable[angle];
			sn1 = g_sinTable[angle];


			g_prim->m_GI[ec].process=-random(11);
			g_prim->m_GI[ec].rise_angle=angle;
			g_prim->m_GI[ec].max_height=0.03f;
			g_prim->m_GI[ec].vecB_pre.x=cs1*dist;
			g_prim->m_GI[ec].vecB_pre.z=sn1*dist;
			g_prim->m_GI[ec].vecB_pre.y=0;
			g_prim->m_GI[ec].vecB_now.y=-4;
			g_prim->m_GI[ec].height[0]=random(360);
			g_prim->m_GI[ec].height[1]=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[3]=random(360);
		}
	}
}

void CRagEffect :: Bottom_Out(char *tName , char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{


		g_prim = LaunchEffectPrim(PP_BOTTOMOUT);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);



		int ec,i;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		i=2;
		if (F1==1)
			i=1;

		for (ec=0;ec<i;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process=random(11);

			if (F1 == 0)
				g_prim->m_GI[ec].distance = 5.0f+random(6);
			else if (F1 == 1)
				g_prim->m_GI[ec].distance = 3.0f+(random(20)*0.1f);

			g_prim->m_GI[ec].height[8] = 0;
			g_prim->m_GI[ec].alphaB = 0;

			g_prim->m_GI[ec].vecB_now.x=m_pos.x;
			g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			if (F1 == 0)
				g_prim->m_GI[ec].vecB_pre.y = m_pos.y;
			else if (F1 == 1)
				g_prim->m_GI[ec].vecB_pre.y = m_pos.y-5.0f;

			g_prim->m_GI[ec].full_display_angle = 45;
			g_prim->m_GI[ec].flag1[0] = 0;
			g_prim->m_GI[ec].flag1[2] = 2;
			g_prim->m_GI[ec].height[7] = 0;

			g_prim->m_GI[ec].flag1[1] = F1;
		}

	}
}

void CRagEffect :: Bottom_LandProtector(char *tName, char PW)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_LANDPROTECTOR);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);




		g_prim->m_GI[0].life = 1;
		g_prim->m_GI[1].life = 0;
		g_prim->m_GI[2].life = 0;
		g_prim->m_GI[3].life = 0;

		if (PW == 2)
			g_prim->m_GI[0].distance = 12.0f;
		else if (PW == 3)
			g_prim->m_GI[0].distance = 10.0f;
		else
			g_prim->m_GI[0].distance = 7.0f;

		if (PW == 3)
		{
			g_prim->m_GI[0].RotStart = 225;
			g_prim->m_GI[0].alphaB = 250;
			g_prim->m_size=111;
		}
		else
		{
			g_prim->m_GI[0].RotStart = 0;
			g_prim->m_GI[0].alphaB = 100;
			g_prim->m_size=5;
		}

		g_prim->m_GI[0].rise_angle = random(360);
		g_prim->m_GI[0].height[0]=-2.0f;

		g_prim->m_GI[0].height[4]=PW;
	}
}

void CRagEffect :: Bottom_Hermode(char *tName)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_HERMODE);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=5;




		g_prim->m_GI[0].life = 1;
		g_prim->m_GI[1].life = 0;
		g_prim->m_GI[2].life = 0;
		g_prim->m_GI[3].life = 0;

		g_prim->m_GI[0].distance = 1.6f;
		g_prim->m_GI[0].rise_angle = random(360);
		g_prim->m_GI[0].RotStart = random(360);
		g_prim->m_GI[0].alphaB = 0;
		g_prim->m_GI[0].height[0]=-12.0f;
		g_prim->m_GI[0].flag1[0] = random(31);
	}
}

void CRagEffect :: Foot(char *tName,char F1)
{
	int ec;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_FOOT);

		g_prim->m_renderFlag = RF_ALPHA;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		if (F1 == 1)
			g_prim->m_size=100;
		else
			g_prim->m_size=0;




		g_prim->m_GI[0].life = 1;
		g_prim->m_GI[1].life = 0;
		g_prim->m_GI[2].life = 0;
		g_prim->m_GI[3].life = 0;

		ec=0;
		g_prim->m_GI[ec].process = 0;
		if (F1 == 1)
			g_prim->m_GI[ec].distance = 6.0f;
		else
			g_prim->m_GI[ec].distance = 5.0f;
		g_prim->m_GI[ec].rise_angle = 0;
		g_prim->m_GI[ec].RotStart = m_deltaPos.x;
		g_prim->m_GI[ec].alphaB = 150;
		g_prim->m_GI[ec].height[0]=-0.2f;

	}
}

void	CRagEffect :: GRANDCROSS(short time, char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\cru_grand cross.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		g_prim = LaunchEffectPrim(PP_GI_3);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		if (F1 == 0)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_white.tga", false);
			g_prim->m_size=1;
		}
		else
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_black.tga", false);
			g_prim->m_size=12;
		}



		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].full_display_angle = 90;
			g_prim->m_GI[ec].max_height = 120.0f;

			g_prim->m_GI[ec].alphaB = time;
			g_prim->m_GI[ec].distance = 19.9f;
			g_prim->m_GI[ec].rise_angle=90;

			g_prim->m_GI[ec].flag1[4] = F1;
		}

		ec=0;
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].flag1[0] = 23;
		g_prim->m_GI[ec].flag1[1] = 23;

		ec=1;
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].flag1[0] = 23;
		g_prim->m_GI[ec].flag1[1] = -23;

		ec=2;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].flag1[0] = -23;
		g_prim->m_GI[ec].flag1[1] = -23;

		ec=3;
		g_prim->m_GI[ec].RotStart = 270;
		g_prim->m_GI[ec].flag1[0] = -23;
		g_prim->m_GI[ec].flag1[1] = 23;
	}
}

void	CRagEffect :: GRANDCROSS2(char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_GRANDCROSS2);

		g_prim->m_renderFlag = 0;
		g_prim->m_renderFlag |= RF_ALPHA;
		g_prim->m_renderFlag |= RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		if (F1 == 0)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_yellow.tga", false);
		}
		else
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_black.tga", false);
		}
		g_prim->m_size=1;



		int ec;

		for (ec=0;ec<2;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;

			g_prim->m_GI[ec].max_height = 60.0f;
			g_prim->m_GI[ec].RotStart = ec*90;
			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].height[0] = 24.0f;
			g_prim->m_GI[ec].height[1] = 3.2f;
			g_prim->m_GI[ec].height[2] = 0.0f;
		}
		ec=2;
		g_prim->m_GI[ec].life = 0;
		ec=3;
		g_prim->m_GI[ec].life = 0;
	}
}

void	CRagEffect :: INTIMIDATE()
{

	if (m_stateCnt == 0)
	{
		m_pos = m_master->m_pos;

		g_prim = LaunchEffectPrim(PP_INTIMIDATE);
		g_prim->m_renderFlag = RF_EFFECT;
		g_prim->m_duration = m_duration;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\white01.bmp", false);

		m_pos.y-=9.0f;

		int ec;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;

			g_prim->m_GI[ec].vecB_now=m_deltaPos;

			g_prim->m_GI[ec].process=-random(30)-70;
			g_prim->m_GI[ec].distance=12.0f;
			g_prim->m_GI[ec].height[0]=1.5f;
			g_prim->m_GI[ec].rise_angle=random(360);
			g_prim->m_GI[ec].full_display_angle=random(360);
			g_prim->m_GI[ec].RotStart=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[1]=3.5f;
			g_prim->m_GI[ec].alphaB=0;

			g_prim->m_GI[ec].flag1[0]=0;
		}
	}
}

void	CRagEffect :: RG_COIN(char *tName)
{

	if (m_stateCnt == 0)
	{
		m_pos = m_master->m_pos;

		g_prim = LaunchEffectPrim(PP_INTIMIDATE);
		g_prim->m_renderFlag = RF_EFFECT;
		g_prim->m_duration = m_duration;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		m_pos.y-=12.0f;

		int ec;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;

			g_prim->m_GI[ec].vecB_now=m_deltaPos;

			g_prim->m_GI[ec].process=-random(30)-18;
			g_prim->m_GI[ec].distance=12.0f;
			g_prim->m_GI[ec].height[0]=1.5f;
			g_prim->m_GI[ec].rise_angle=random(360);
			g_prim->m_GI[ec].full_display_angle=random(360);
			g_prim->m_GI[ec].RotStart=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[1]=3.5f;
			g_prim->m_GI[ec].alphaB=0;

			g_prim->m_GI[ec].flag1[0]=1;
		}
	}
}

void	CRagEffect :: RG_COIN2(char *tName,float size)
{

	if (m_stateCnt == 0)
	{
		m_pos = m_master->m_pos;

		g_prim = LaunchEffectPrim(PP_INTIMIDATE);
		g_prim->m_renderFlag = RF_EFFECT;
		g_prim->m_duration = m_duration;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		m_pos.y-=12.0f;

		int ec;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;

			g_prim->m_GI[ec].vecB_now=m_deltaPos;

			g_prim->m_GI[ec].process=-random(30)-18;
			g_prim->m_GI[ec].distance=12.0f;
			g_prim->m_GI[ec].height[0]=1.0f;
			g_prim->m_GI[ec].rise_angle=random(360);
			g_prim->m_GI[ec].full_display_angle=random(360);
			g_prim->m_GI[ec].RotStart=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[1]=size;
			g_prim->m_GI[ec].alphaB=0;

			g_prim->m_GI[ec].flag1[0]=2;
		}
	}
}

void	CRagEffect :: TEIHIT1(char *tName,char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_TEIHIT1);
		g_prim->m_renderFlag = RF_EFFECT;
		g_prim->m_duration = m_duration;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		m_pos.y-=9.0f;

		int ec;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;

			g_prim->m_GI[ec].process=-10-random(25);
			g_prim->m_GI[ec].distance=-10.0f;

			g_prim->m_GI[ec].height[0]=3.0f;
			g_prim->m_GI[ec].rise_angle=random(360);
			g_prim->m_GI[ec].full_display_angle=random(360);

			g_prim->m_GI[ec].height[1]=0.7f;
			g_prim->m_GI[ec].alphaB=0;

			g_prim->m_GI[ec].flag1[0]=F1;
			g_prim->m_GI[ec].flag1[1]=24;

		}
	}
}

void	CRagEffect :: CHEMICALPROTECTION(short time)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{

		g_prim = LaunchEffectPrim(PP_CHEMICALPROTECTION);
		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\shockwave_c.bmp", false);


		m_pos.y-=9.0f;

		int ec;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;

			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].distance=100.0f;

			g_prim->m_GI[ec].rise_angle=(ec*90) + time;
			g_prim->m_GI[ec].alphaB=0;
		}
	}
}

void	CRagEffect :: CHEMICAL2(short time)
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 40)
	{
		g_prim = LaunchEffectPrim(PP_CHEMICAL2);

		g_prim->m_renderFlag |= RF_NODEPTHCHECK;

		g_prim->m_duration = m_duration;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];


		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\slash01.tga", false);


		m_pos.y-=9.0f;

		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=0;
		}

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;

			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].distance=100.0f;
			g_prim->m_GI[ec].max_height=1.0f;

			if (time == 0)
				g_prim->m_GI[ec].height[0]=-15.0f+(ec*-2.5f);
			else
				g_prim->m_GI[ec].height[0]=15.0f+(ec*2.5f);



			{
				float Rx,Rz;

				Rx=m_deltaPos.x-m_pos.x;
				Rz=m_deltaPos.z-m_pos.z;

				g_prim->Get_Angle(Rx,Rz,&g_prim->m_GI[ec].rise_angle);


				if (g_prim->m_GI[ec].rise_angle>=360)
					g_prim->m_GI[ec].rise_angle-=360;
			}


			g_prim->m_GI[ec].alphaB=0;

		}
	}
}

void	CRagEffect :: GROUNDSAMPLE(void)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_GROUNDSAMPLE);
		g_prim->m_renderFlag = RF_EFFECT;
		g_prim->m_duration = m_duration;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\magic_target.tga", false);

		int ec;
		for (ec=0;ec<4;ec++)
			g_prim->m_GI[ec].life=0;

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life=1;
			g_prim->m_GI[ec].RotStart=0;
		}
	}
}

void	CRagEffect :: GI_EXPLOSION(char *tName)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		{
			m_pos = m_master->m_pos;

			g_prim = LaunchEffectPrim(PP_GI_EXPLOSION);
			g_prim->m_renderFlag = RF_EFFECT;
			g_prim->m_duration = m_duration;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

			g_prim->m_size=4;



			int ec,i;

			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process = 0;
				g_prim->m_GI[ec].full_display_angle = 315;
				g_prim->m_GI[ec].max_height = 15.0f;
				g_prim->m_GI[ec].RotStart = random(360);
				g_prim->m_GI[ec].alphaB = 0;
				g_prim->m_GI[ec].distance = 20.0f-(ec*0.5f);
				g_prim->m_GI[ec].rise_angle=50;
				g_prim->m_GI[ec].vecB_now.y=0;

				for (i = 0 ; i<E_DIVISION ; i++)
				{
					g_prim->m_GI[ec].height[i] = 0.0f;
					g_prim->m_GI[ec].flag1[i] = 0;
				}
			}
		}
	}
}

void	CRagEffect :: CHEMICAL3(short time)
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_CHEMICAL3);

		g_prim->m_renderFlag = RF_UI;

		g_prim->m_duration = m_duration;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		if (time<10)
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\line3.tga", false);
		else
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\line3y.tga", false);

		int ec;
		for (ec=0;ec<4;ec++)
			g_prim->m_GI[ec].life=0;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;

			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].distance=100.0f;

			if (time%10 == 0)
				g_prim->m_GI[ec].height[0]=-15.0f-random(11);
			else if (time%10 == 1)
				g_prim->m_GI[ec].height[0]=15.0f+random(11);

			g_prim->m_GI[ec].height[1]=random(200);

			if (time<10)
			{
				g_prim->m_GI[ec].max_height=0.3f+(random(4)*0.1f);
				g_prim->m_GI[ec].height[2]=20.0f+random(31);
				g_prim->m_GI[ec].flag1[0] = random(3);
				g_prim->m_GI[ec].height[3]=1.5f+(random(21)*0.1f);
			}
			else if (time<20)
			{
				g_prim->m_GI[ec].max_height=0.5f+(random(6)*0.1f);
				g_prim->m_GI[ec].height[2]=40.0f+random(31);
				g_prim->m_GI[ec].flag1[0] = random(3)+10;
				g_prim->m_GI[ec].height[3]=1.0f+(random(11)*0.1f);
			}
			else
			{
				g_prim->m_GI[ec].max_height=0.4f+(random(4)*0.1f);
				g_prim->m_GI[ec].height[2]=30.0f+random(31);
				g_prim->m_GI[ec].flag1[0] = random(3)+20;
				g_prim->m_GI[ec].height[3]=1.5f+(random(21)*0.1f);
			}

			{
				float Rx,Rz;

				Rx=m_deltaPos.x-m_pos.x;
				Rz=m_deltaPos.z-m_pos.z;

				g_prim->Get_Angle(Rx,Rz,&g_prim->m_GI[ec].rise_angle);
			}

			g_prim->m_GI[ec].alphaB=0;

		}
	}
}

void	CRagEffect :: CALLZONE(char *tName)
{

	if (m_stateCnt == 100)
	{
		m_pos = m_master->m_pos;

		g_prim = LaunchEffectPrim(PP_CALLZONE);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture = 1;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		m_pos.y-=10.0f;

		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=0;
		}

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life=1;

			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].distance=8.0f;
			g_prim->m_GI[ec].alphaB=0;
			g_prim->m_GI[ec].full_display_angle=360;
		}
	}
}

void	CRagEffect :: BLIND(char level)
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_BLIND);

		g_prim->m_renderFlag |= RF_NODEPTHCHECK;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture = 2;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\fullb.tga", false);
		g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\white02.bmp", false);

		m_pos.y-=9.0f;

		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=0;
		}

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life=1;

			g_prim->m_GI[ec].process=0;
			if (level == 0)
			{
				g_prim->m_GI[ec].distance=50;
				g_prim->m_GI[ec].flag1[0]=0;
			}
			else
			{
				g_prim->m_GI[ec].distance=140.0f-(level*10);
				g_prim->m_GI[ec].flag1[0]=1;
			}
			g_prim->m_GI[ec].alphaB=0;
		}
	}
}

void	CRagEffect :: POISON(char F1)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 99999998)
	{
		m_stateCnt = 0;
		m_duration = 99999999;
	}


	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_POISON);

		g_prim->m_renderFlag |= RF_NODEPTHCHECK;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture = 1;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		if (F1 == 0)
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\poison_f.bmp", false);
		else
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\white02.bmp", false);

		m_pos.y-=9.0f;

		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=0;
		}

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life=1;

			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].distance=30.0f;
			g_prim->m_GI[ec].max_height=0;


			g_prim->m_GI[ec].alphaB=0;

			g_prim->m_GI[ec].flag1[10] = F1;
		}
	}
}

void	CRagEffect :: TELEPORTATION2(char *tName, short time)
{

	if (m_stateCnt == 0)
	{
		m_pos = m_master->m_pos;

		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_Teleportation.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		g_prim = LaunchEffectPrim(PP_TELEPORTATION2);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=4;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 100.0f+(time*60);
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].alphaB = 100;
		g_prim->m_GI[ec].distance = 1.5f - (time*2);
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].vecB_now.y=0;

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 70.0f+(time*60);
		g_prim->m_GI[ec].RotStart = 270;
		g_prim->m_GI[ec].alphaB = 80;
		g_prim->m_GI[ec].distance = 3.0f - (time*2);
		g_prim->m_GI[ec].rise_angle=89;
		g_prim->m_GI[ec].vecB_now.y=0;

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 40.0f+(time*60);
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 60;
		g_prim->m_GI[ec].distance = 4.0f - (time*2);
		g_prim->m_GI[ec].rise_angle=88;
		g_prim->m_GI[ec].vecB_now.y=0;

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 15.0f+(time*60);
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].alphaB = 40;
		g_prim->m_GI[ec].distance = 5.0f - (time*2);
		g_prim->m_GI[ec].rise_angle=87;
		g_prim->m_GI[ec].vecB_now.y=0;


		for (ec=0;ec<4;ec++)
		{
			for (i = 0 ; i<E_DIVISION ; i++)
			{
				g_prim->m_GI[ec].height[i] = 0.0f;
			}
			g_prim->m_GI[ec].flag1[0]=0;
			g_prim->m_GI[ec].flag1[1]=g_prim->m_GI[ec].max_height;
		}
	}
}

void	CRagEffect :: TEIHIT3(char *tName,char F1)
{

	if (m_stateCnt == 0)
	{
		m_pos = m_master->m_pos;

		g_prim = LaunchEffectPrim(PP_TEIHIT1);
		g_prim->m_renderFlag = RF_EFFECT;
		g_prim->m_duration = m_duration;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		m_pos.y-=9.0f;

		int ec;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life=1;

			g_prim->m_GI[ec].process=-20-random(10);
			g_prim->m_GI[ec].distance=-10.0f;

			g_prim->m_GI[ec].height[0]=2.0f;
			g_prim->m_GI[ec].rise_angle=random(360);
			g_prim->m_GI[ec].full_display_angle=random(360);
			g_prim->m_GI[ec].height[1]=1.0f;
			g_prim->m_GI[ec].alphaB=0;

			g_prim->m_GI[ec].flag1[0]=F1;
			g_prim->m_GI[ec].flag1[1]=16;
		}
	}
}

void	CRagEffect :: SoulBreaker()
{
	int ec;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_SOULBREAKER);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\purpleslash.tga", false);

		g_prim->m_size=0;

		g_prim->m_deltaPos = m_deltaPos;
		g_prim->m_pos = m_pos;



		ec=0;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;

			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].process=-28+ec;
			g_prim->m_GI[ec].distance = 20.0f;

			g_prim->m_GI[ec].vecB_now.x=m_pos.x;
			g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			g_prim->m_GI[ec].vecB_now.y = m_pos.y-10.0f+random(5);

			g_prim->m_GI[ec].RotStart=0;

			g_prim->m_GI[ec].height[1] = 0.8f;
			g_prim->m_GI[ec].flag1[0] = 0;

		}
	}
}

void	CRagEffect :: TEIHIT2()
{
	int ec;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_TEIHIT2);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\thunder_center.bmp", false);

		g_prim->m_size=0;

		g_prim->m_deltaPos = m_deltaPos;
		g_prim->m_pos = m_pos;



		ec=0;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process=-40+random(10);


			g_prim->m_GI[ec].distance = (random(16)*0.05f) + 0.1f;
			g_prim->m_GI[ec].height[8] = 0;
			g_prim->m_GI[ec].alphaB = 0;

			g_prim->m_GI[ec].vecB_now.x=m_deltaPos.x;
			g_prim->m_GI[ec].vecB_now.z=m_deltaPos.z;
			g_prim->m_GI[ec].vecB_now.y = m_deltaPos.y-8.0f;

			g_prim->m_GI[ec].RotStart=0;

			g_prim->m_GI[ec].height[1] = (random(50)*0.01f) + 0.75f;
			g_prim->m_GI[ec].height[2] = (random(200)*0.01f)-1.0f;
			g_prim->m_GI[ec].full_display_angle = 0;
			g_prim->m_GI[ec].flag1[0] = 0;
			g_prim->m_GI[ec].flag1[1] = 0;
			g_prim->m_GI[ec].flag1[2] = 2;
			g_prim->m_GI[ec].height[7] = 0;
		}
	}
}

void	CRagEffect :: SoulBreaker2(int time)
{
	int ec;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_SOULBREAKER2);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\purpleslash.tga", false);

		g_prim->m_size=0;



		int shoot;
		shoot = random(6);
		ec=0;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;

			g_prim->m_GI[ec].process=-28+ec-shoot;
			g_prim->m_GI[ec].distance = 20.0f;
			g_prim->m_GI[ec].alphaB = 0;

			g_prim->m_GI[ec].vecB_now.x=m_pos.x;
			g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			g_prim->m_GI[ec].vecB_now.y = m_pos.y-10.0f+random(5);

			g_prim->m_GI[ec].RotStart=time;

			g_prim->m_GI[ec].height[1] = 0.8f;
			g_prim->m_GI[ec].flag1[0] = 0;

		}
	}
}

void	CRagEffect :: STIN(char *tName)
{
	int ec;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_STIN);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;

		g_prim->m_deltaPos = m_deltaPos;
		g_prim->m_pos = m_pos;



		ec=0;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;

			g_prim->m_GI[ec].process= -3+ec;
			g_prim->m_GI[ec].distance = 20.0f;
			g_prim->m_GI[ec].alphaB = 0;

			g_prim->m_GI[ec].vecB_now.x=m_pos.x;
			g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			g_prim->m_GI[ec].vecB_now.y = m_pos.y-10.0f+random(5);

			g_prim->m_GI[ec].RotStart=0;
			g_prim->m_GI[ec].rise_angle=ec;

			g_prim->m_GI[ec].height[1] = 0.8f;
			g_prim->m_GI[ec].flag1[0] = 0;

			g_prim->m_GI[ec].flag1[3] = 0;
		}
	}
}

void	CRagEffect :: STIN5(char *tName)
{
	int ec;

	m_pos = m_master->m_pos;

	if (m_stateCnt < 5)
	{
		g_prim = LaunchEffectPrim(PP_STIN5);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;

		g_prim->m_deltaPos = m_deltaPos;
		g_prim->m_pos = m_pos;



		ec=0;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;

			g_prim->m_GI[ec].process= -3+ec;
			g_prim->m_GI[ec].distance = 20.0f;
			g_prim->m_GI[ec].alphaB = 0;

			if (m_stateCnt == 0)
			{
				g_prim->m_GI[ec].vecB_now.x=m_pos.x;
				g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			}
			else if (m_stateCnt == 1)
			{
				g_prim->m_GI[ec].vecB_now.x=m_pos.x+5.0f;
				g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			}
			else if (m_stateCnt == 2)
			{
				g_prim->m_GI[ec].vecB_now.x=m_pos.x-5.0f;
				g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			}

			else if (m_stateCnt == 3)
			{
				g_prim->m_GI[ec].vecB_now.x=m_pos.x;
				g_prim->m_GI[ec].vecB_now.z=m_pos.z+5.0f;
			}

			else
			{
				g_prim->m_GI[ec].vecB_now.x=m_pos.x;
				g_prim->m_GI[ec].vecB_now.z=m_pos.z-5.0f;
			}

			g_prim->m_GI[ec].vecB_now.y = m_pos.y-10.0f+random(5);

			g_prim->m_GI[ec].RotStart=random(360);
			g_prim->m_GI[ec].rise_angle=ec;

			g_prim->m_GI[ec].height[1] = 1.5f;
			g_prim->m_GI[ec].flag1[0] = 0;

			g_prim->m_GI[ec].flag1[3] = 1;
		}
	}
}

void	CRagEffect :: STIN2(char *tName,char F1)
{
	int ec;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_STIN2);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;

		g_prim->m_deltaPos = m_deltaPos;
		g_prim->m_pos = m_pos;




		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;

			g_prim->m_GI[ec].process= 0;
			g_prim->m_GI[ec].distance = 25.0f;
			g_prim->m_GI[ec].alphaB = 0;

			g_prim->m_GI[ec].vecB_now.x=m_pos.x;
			g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			g_prim->m_GI[ec].vecB_now.y = m_pos.y-10.0f;

			g_prim->m_GI[ec].RotStart=0;
			g_prim->m_GI[ec].rise_angle=ec;

			g_prim->m_GI[ec].height[1] = 2.0f;
			g_prim->m_GI[ec].flag1[0] = 0;
			g_prim->m_GI[ec].flag1[1] = 0;
			if ( F1 == 0 )
				g_prim->m_GI[ec].flag1[2] = 0;
			else if ( F1 == 1)
				g_prim->m_GI[ec].flag1[2] = 1;

			g_prim->m_GI[ec].flag1[3] = 0;
		}
	}
}

void	CRagEffect :: STIN4(char *tName,char F1)
{
	int ec;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_STIN4);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=0;

		g_prim->m_deltaPos = m_deltaPos;
		g_prim->m_pos = m_pos;




		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;

			g_prim->m_GI[ec].process= 0;
			g_prim->m_GI[ec].distance = 10.0f + (random(11)*0.5f);
			g_prim->m_GI[ec].alphaB = 0;

			g_prim->m_GI[ec].vecB_now.x=m_pos.x;
			g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			g_prim->m_GI[ec].vecB_now.y = m_pos.y-10.0f;

			g_prim->m_GI[ec].RotStart=0;
			g_prim->m_GI[ec].rise_angle=ec;

			g_prim->m_GI[ec].height[1] = 1.0f + (random(21)*0.1f);
			g_prim->m_GI[ec].flag1[0] = 0;
			g_prim->m_GI[ec].flag1[1] = 0;
			if ( F1 == 0 )
				g_prim->m_GI[ec].flag1[2] = 0;
			else if ( F1 == 1)
				g_prim->m_GI[ec].flag1[2] = 1;

			g_prim->m_GI[ec].flag1[3] = 1;
		}
	}
}

void	CRagEffect :: SMA(char F1)
{
	int ec;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_SMA);

		g_prim->m_duration = m_duration;
		g_prim->m_totalTexture = 0;

		g_prim->m_size=0;

		g_prim->m_deltaPos = m_deltaPos;
		g_prim->m_pos = m_pos;



		ec=0;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;

			g_prim->m_GI[ec].process= -3+ec;
			g_prim->m_GI[ec].alphaB = 0;

			g_prim->m_GI[ec].vecB_now.x=m_pos.x;
			g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			g_prim->m_GI[ec].vecB_now.y = m_pos.y-10.0f+random(5);
			g_prim->m_GI[ec].RotStart=0;

			g_prim->m_GI[ec].height[1] = 0.8f;

			g_prim->m_GI[ec].flag1[0] = F1;
		}
	}
}

void	CRagEffect :: SMA2(char *tName)
{
	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_SMA2);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=4;

		g_prim->m_deltaPos = m_deltaPos;
		g_prim->m_deltaPos2 = m_deltaPos2;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = 13.0f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		if ( m_deltaPos2.y == 0 )
			g_prim->m_GI[ec].distance = 3.4f;
		else
			g_prim->m_GI[ec].distance = 2.4f;
		g_prim->m_GI[ec].rise_angle=55;
		g_prim->m_GI[ec].vecB_now.y=0;

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = 11.0f;
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].alphaB = 0;
		if ( m_deltaPos2.y == 0 )
			g_prim->m_GI[ec].distance = 3.6f;
		else
			g_prim->m_GI[ec].distance = 2.6f;
		g_prim->m_GI[ec].rise_angle=50;
		g_prim->m_GI[ec].vecB_now.y=0;

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 315;
		g_prim->m_GI[ec].max_height = 9.0f;
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].alphaB = 0;
		if ( m_deltaPos2.y == 0 )
			g_prim->m_GI[ec].distance = 3.8f;
		else
			g_prim->m_GI[ec].distance = 2.8f;
		g_prim->m_GI[ec].rise_angle=45;
		g_prim->m_GI[ec].vecB_now.y=0;

		ec=3;
		g_prim->m_GI[ec].life = 0;

		for (ec=0;ec<4;ec++)
		{
			for (i = 0 ; i<E_DIVISION ; i++)
			{
				g_prim->m_GI[ec].height[i] = 0.0f;
				g_prim->m_GI[ec].flag1[i] = 0;
			}
		}
	}
}

void	CRagEffect :: PARTICLE_UP()
{
	int ec;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{
		g_prim = LaunchEffectPrim(PP_PARTICLE_UP);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\thunder_center.bmp", false);

		g_prim->m_size=0;
		g_prim->m_pos = m_pos;



		ec=0;
		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process=-10+random(10);

			g_prim->m_GI[ec].distance = (random(16)*0.03f) + 0.1f;
			g_prim->m_GI[ec].height[8] = 0;
			g_prim->m_GI[ec].alphaB = 0;

			g_prim->m_GI[ec].vecB_now.x = m_pos.x + random(9) - 4;
			g_prim->m_GI[ec].vecB_now.z = m_pos.z + random(9) - 4;
			g_prim->m_GI[ec].vecB_now.y = m_pos.y;

			g_prim->m_GI[ec].RotStart=0;

			g_prim->m_GI[ec].height[1] = (random(10)*0.01f);
			g_prim->m_GI[ec].height[2] = (random(40)*0.01f) + 0.2f;
			g_prim->m_GI[ec].full_display_angle = 0;
			g_prim->m_GI[ec].flag1[0] = 0;
			g_prim->m_GI[ec].flag1[1] = 0;
			g_prim->m_GI[ec].flag1[2] = 10;
			g_prim->m_GI[ec].height[7] = 0;
		}
	}
}

void	CRagEffect :: HealTime(char *tName, char F1)
{
	m_pos = m_master->m_pos;


	if ( m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		if (F1 == 0)
			PlayWave("_heal_effect.wav", m_pos.x-pPos.x, -40,  m_pos.z-pPos.z);
		else

			PlayWave("effect\\\xc8\xed\xb1\xe2.wav", m_pos.x-pPos.x, -80,  m_pos.z-pPos.z);

	}

	if ( m_stateCnt <= 20 && m_stateCnt %4 == 0 )
	{
		int ec;

		{
			g_prim = LaunchEffectPrim(PP_PARTICLE_UP);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_totalTexture = 2;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\pok1.tga", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\pok3.tga", false);

			g_prim->m_size=0;
			g_prim->m_pos = m_pos;



			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process=0;

				g_prim->m_GI[ec].distance = (random(10)*0.1f) + 1.5f;
				g_prim->m_GI[ec].height[8] = 0;
				g_prim->m_GI[ec].alphaB = 0;

				g_prim->m_GI[ec].vecB_now.x = m_pos.x + random(7) - 3;
				g_prim->m_GI[ec].vecB_now.z = m_pos.z + random(7) - 3;
				g_prim->m_GI[ec].vecB_now.y = m_pos.y - 6.0f;

				g_prim->m_GI[ec].RotStart=0;

				g_prim->m_GI[ec].height[1] = 0;
				g_prim->m_GI[ec].height[2] = (random(20)*0.01f) + 0.2f;
				g_prim->m_GI[ec].full_display_angle = 0;
				g_prim->m_GI[ec].flag1[0] = 0;
				g_prim->m_GI[ec].flag1[1] = 1;
				if (F1 == 0)
					g_prim->m_GI[ec].flag1[2] = 11;
				else
					g_prim->m_GI[ec].flag1[2] = 1;
				g_prim->m_GI[ec].height[7] = 0;
			}
		}

	}
}

void	CRagEffect :: ParticleTime(char *tName, char F1)
{
	short start,end;
	m_pos = m_master->m_pos;

	if ( F1 == 2 )
	{
		start = 40;
		end = 120;
	}
	else
	{
		start = 0;
		end = 80;
	}

	if ( m_stateCnt >= start && m_stateCnt <= end && m_stateCnt %4 == 0 )
	{
		int ec;

		{
			g_prim = LaunchEffectPrim(PP_PARTICLE_UP);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_totalTexture = 2;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\pok1.tga", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\pok3.tga", false);

			g_prim->m_size=0;
			g_prim->m_pos = m_pos;



			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process=0;

				g_prim->m_GI[ec].distance = (random(10)*0.1f) + 2.0f;
				g_prim->m_GI[ec].height[8] = 0;
				g_prim->m_GI[ec].alphaB = 0;

				g_prim->m_GI[ec].vecB_now.x = m_pos.x + random(11) - 5;
				g_prim->m_GI[ec].vecB_now.z = m_pos.z + random(11) - 5;
				g_prim->m_GI[ec].vecB_now.y = m_pos.y - 6.0f;

				g_prim->m_GI[ec].RotStart=0;

				g_prim->m_GI[ec].height[1] = 0;
				g_prim->m_GI[ec].height[2] = (random(20)*0.01f) + 0.1f;
				g_prim->m_GI[ec].full_display_angle = 0;
				g_prim->m_GI[ec].flag1[0] = 0;
				g_prim->m_GI[ec].flag1[1] = 1;
				if (F1 == 0)
					g_prim->m_GI[ec].flag1[2] = 11;
				else if (F1 == 1)
					g_prim->m_GI[ec].flag1[2] = 1;
				else if (F1 == 2)
					g_prim->m_GI[ec].flag1[2] = 1;
				else if (F1 == 3)
					g_prim->m_GI[ec].flag1[2] = 13;


				g_prim->m_GI[ec].height[7] = 0;
			}
		}

	}
}

void	CRagEffect :: ParticlePath(char *tName, char F1)
{
	short start,end;

	start = 0;
	end = 0;

	if ( m_stateCnt >= start && m_stateCnt <= end )
	{
		int ec;

		{
			g_prim = LaunchEffectPrim(PP_PARTICLE_UP);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_totalTexture = 2;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			if ( F1 == 1 )
			{
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\thunder_ball_0002.bmp", false);
				g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\thunder_ball_0003.bmp", false);
			}
			else if ( F1 == 2 )
			{
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\meteo_comet_b.bmp", false);
				g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\meteo_comet_c.bmp", false);
			}
			else
			{
				g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\pok1.tga", false);
				g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\pok3.tga", false);
			}

			g_prim->m_size=0;
			g_prim->m_pos = m_pos;



			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process=0;

				g_prim->m_GI[ec].distance = (random(10)*0.08f) + 1.2f;
				g_prim->m_GI[ec].height[8] = 0;
				g_prim->m_GI[ec].alphaB = 0;

				g_prim->m_GI[ec].vecB_now.x = m_deltaPos.x + random(9) - 4;
				g_prim->m_GI[ec].vecB_now.z = m_deltaPos.z + random(9) - 4;
				g_prim->m_GI[ec].vecB_now.y = m_deltaPos.y - 6.0f;

				g_prim->m_GI[ec].RotStart=0;

				g_prim->m_GI[ec].height[1] = 0;
				g_prim->m_GI[ec].height[2] = (random(20)*0.01f) + 0.1f;
				g_prim->m_GI[ec].full_display_angle = 0;
				g_prim->m_GI[ec].flag1[0] = 0;
				g_prim->m_GI[ec].flag1[1] = 1;
				if (F1 == 0)
					g_prim->m_GI[ec].flag1[2] = 11;
				else if (F1 == 1)
					g_prim->m_GI[ec].flag1[2] = 1;
				else if (F1 == 2)
					g_prim->m_GI[ec].flag1[2] = 7;


				g_prim->m_GI[ec].height[7] = 0;
			}
		}

	}
}

void	CRagEffect :: Heal(char *tName, char F1)
{
	int uTime;
	m_pos = m_master->m_pos;

	if (F1 == 0)
		uTime=50;
	else
		uTime=25;


	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_HEAL);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		if (F1==0)
			g_prim->m_size=9;
		else if (F1==1)
			g_prim->m_size=5;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].vecB_now.y=0;
		if (F1==0)
		{
			g_prim->m_GI[ec].alphaT=60;
			g_prim->m_GI[ec].max_height = 40.0f;
			g_prim->m_GI[ec].distance = 4.6f;
		}
		else if (F1==1)
		{
			g_prim->m_GI[ec].alphaT=30;
			g_prim->m_GI[ec].max_height = 40.0f + random(20);
			g_prim->m_GI[ec].distance = 4.5f;
			g_prim->m_GI[ec].distance += ( random(100) * 0.005f );
		}

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].vecB_now.y=0;
		if (F1==0)
		{
			g_prim->m_GI[ec].alphaT=60;
			g_prim->m_GI[ec].max_height = 40.0f;
			g_prim->m_GI[ec].distance = 4.8f;
		}
		else if (F1==1)
		{
			g_prim->m_GI[ec].alphaT=30;
			g_prim->m_GI[ec].max_height = g_prim->m_GI[ec-1].max_height;
			g_prim->m_GI[ec].distance = g_prim->m_GI[ec-1].distance + 0.2f;
		}

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		g_prim->m_GI[2].life = 0;
		g_prim->m_GI[3].life = 0;

		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("_heal_effect.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
	else if ( m_stateCnt <= uTime && m_stateCnt %6 == 0 )
	{
		int ec;

		if (F1 == 0)
		{
			g_prim = LaunchEffectPrim(PP_PARTICLE_UP);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\pok1.tga", false);

			g_prim->m_size=0;
			g_prim->m_pos = m_pos;



			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process=0;

				g_prim->m_GI[ec].distance = (random(10)*0.1f) + 2.0f;
				g_prim->m_GI[ec].height[8] = 0;
				g_prim->m_GI[ec].alphaB = 0;

				g_prim->m_GI[ec].vecB_now.x = m_pos.x + random(13) - 6;
				g_prim->m_GI[ec].vecB_now.z = m_pos.z + random(13) - 6;
				g_prim->m_GI[ec].vecB_now.y = m_pos.y;

				g_prim->m_GI[ec].RotStart=0;

				g_prim->m_GI[ec].height[1] = 0;
				g_prim->m_GI[ec].height[2] = (random(40)*0.01f) + 0.3f;
				g_prim->m_GI[ec].full_display_angle = 0;
				g_prim->m_GI[ec].flag1[0] = 0;
				g_prim->m_GI[ec].flag1[1] = 0;
				g_prim->m_GI[ec].flag1[2] = 11;
				g_prim->m_GI[ec].height[7] = 0;
			}
		}
		else if (F1 == 1)
		{
			int ec;

			g_prim = LaunchEffectPrim(PP_PARTICLE_UP);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_totalTexture = 2;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\pok1.tga", false);
			g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\pok3.tga", false);

			g_prim->m_size=0;
			g_prim->m_pos = m_pos;



			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process=0;

				g_prim->m_GI[ec].distance = (random(10)*0.1f) + 2.0f;
				g_prim->m_GI[ec].height[8] = 0;
				g_prim->m_GI[ec].alphaB = 0;

				g_prim->m_GI[ec].vecB_now.x = m_pos.x + random(13) - 6;
				g_prim->m_GI[ec].vecB_now.z = m_pos.z + random(13) - 6;
				g_prim->m_GI[ec].vecB_now.y = m_pos.y;

				g_prim->m_GI[ec].RotStart=0;

				g_prim->m_GI[ec].height[1] = 0;
				g_prim->m_GI[ec].height[2] = (random(40)*0.01f) + 0.4f;
				g_prim->m_GI[ec].full_display_angle = 0;
				g_prim->m_GI[ec].flag1[0] = 0;
				g_prim->m_GI[ec].flag1[1] = 1;
				g_prim->m_GI[ec].flag1[2] = 11;
				g_prim->m_GI[ec].height[7] = 0;
			}
		}

	}
}

void	CRagEffect :: Exit2(char *tName)
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_Teleportation.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		g_prim = LaunchEffectPrim(PP_HEAL);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=4;



		int ec,i;

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 20.0f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 2.4f;
		g_prim->m_GI[ec].rise_angle=84;
		g_prim->m_GI[ec].alphaT = 60;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 45.0f;
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 2.2f;
		g_prim->m_GI[ec].rise_angle=86;
		g_prim->m_GI[ec].alphaT = 60;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 70.0f;
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 2.0f;
		g_prim->m_GI[ec].rise_angle=88;
		g_prim->m_GI[ec].alphaT = 60;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 180.0f;
		g_prim->m_GI[ec].RotStart = 270;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 1.8f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT = 58;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}
	}
}

void	CRagEffect :: Guard(char *tName, char F1)
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\kyrie_guard.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);

		g_prim = LaunchEffectPrim(PP_GUARD);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 2;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);
		g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\guardK2.tga", false);

		g_prim->m_size=11;



		int ec;

		ec=3;
		g_prim->m_GI[ec].life = 0;

		ec=2;
		g_prim->m_GI[ec].life = 0;

		ec=1;
		g_prim->m_GI[ec].life = 0;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;

		g_prim->m_GI[ec].max_height = 7.8f;

		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 1.0f;
		g_prim->m_GI[ec].height[0] = 0;




		if (F1 == 2)
			g_prim->m_GI[ec].flag1[0]=2;
		else
			g_prim->m_GI[ec].flag1[0]=0;
	}
}

void	CRagEffect :: Guard2(char *tName)
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\black_maximize_power_sword_bic.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);

		g_prim = LaunchEffectPrim(PP_GUARD);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 2;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);
		g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\a01.bmp", false);

		g_prim->m_size=11;



		int ec;

		ec=3;
		g_prim->m_GI[ec].life = 0;

		ec=2;
		g_prim->m_GI[ec].life = 0;

		ec=1;
		g_prim->m_GI[ec].life = 0;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;

		g_prim->m_GI[ec].max_height = 10.0f;

		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 1.0f;
		g_prim->m_GI[ec].height[0] = 0;

		g_prim->m_GI[ec].flag1[0]=1;
	}
}

void	CRagEffect :: HalfSphere(char *tName)
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0)
	{



		g_prim = LaunchEffectPrim(PP_HALFSPHERE);


		g_prim->m_renderFlag = RF_UI;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);


		g_prim->m_size=0;



		int ec;

		ec=3;
		g_prim->m_GI[ec].life = 0;

		ec=2;
		g_prim->m_GI[ec].life = 0;

		ec=1;
		g_prim->m_GI[ec].life = 0;

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].max_height = 8.0f;
			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].distance = 8.0f-(ec*0.2f);
			g_prim->m_GI[ec].full_display_angle = 180;
		}

	}
}

void	CRagEffect :: AttackEnergy(char *tName)
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0)
	{



		g_prim = LaunchEffectPrim(PP_ATTACKENERGY);

		g_prim->m_renderFlag = RF_UI;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);



		g_prim->m_size=100;



		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;

			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].max_height = 8.0f;
			g_prim->m_GI[ec].alphaB = 0;

			g_prim->m_GI[ec].distance = 8.0f;
			g_prim->m_GI[ec].full_display_angle = 360;

			g_prim->m_GI[ec].height[0]= 20.0f;

			g_prim->m_GI[ec].rise_angle= ec*45;
		}

	}
}

void	CRagEffect :: AttackEnergy2(char *tName)
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0 || m_stateCnt == 4)
	{



		g_prim = LaunchEffectPrim(PP_ATTACKENERGY2);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);


		g_prim->m_size=4;




		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 0;
		}

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].max_height = 8.0f;
			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].distance = 8.0f;
			g_prim->m_GI[ec].full_display_angle = 360;
			g_prim->m_GI[ec].height[0]= 10.0f;
			g_prim->m_GI[ec].rise_angle= ec*45;
			g_prim->m_GI[ec].height[1]=ec*22;
		}

	}
}

void	CRagEffect :: SaintWing(char *tName)
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_SAINTWING);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=4;



		int ec;

		ec=3;
		g_prim->m_GI[ec].life = 0;
		ec=2;
		g_prim->m_GI[ec].life = 0;

		for (ec=0;ec<2;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].max_height = 10.0f;
			g_prim->m_GI[ec].distance = 2.0f;
			g_prim->m_GI[ec].height[0] = -8.0f;
			g_prim->m_GI[ec].RotStart = 0;
			g_prim->m_GI[ec].full_display_angle = 0;
			g_prim->m_GI[ec].alphaB = 200;
			g_prim->m_GI[ec].rise_angle=0;
		}
	}
}

void	CRagEffect :: SphereWind(char *tName, int time, char F1)
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_SPHEREWIND);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		if (F1 == 1)
			g_prim->m_size=10;
		else
			g_prim->m_size=4;



		int ec;

		for (ec=0;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].max_height = 4.0f+(random(11)*0.2f);
			g_prim->m_GI[ec].distance = 14.0f;
			g_prim->m_GI[ec].height[0] = -14.0f;
			g_prim->m_GI[ec].RotStart = random(360);
			g_prim->m_GI[ec].rise_angle = (ec*45)+time;
			g_prim->m_GI[ec].alphaB = 100;
			g_prim->m_GI[ec].full_display_angle=180+random(91);
			g_prim->m_GI[ec].height[1] = 3+random(4);
		}
	}
}

void	CRagEffect :: ParticleSpread(char *tName, char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt == 30)
	{
		g_prim = LaunchEffectPrim(PP_3DAURA_3);
		g_prim->m_renderFlag = RF_EFFECT;
		g_prim->m_duration = m_duration;
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);
		g_prim->m_size=0;


		int ec;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].max_height=1.0f;
			g_prim->m_GI[ec].distance=2.5f+(random(6)*0.1f);
			g_prim->m_GI[ec].process = 0;


			g_prim->m_GI[ec].vecB_pre.x=0;
			g_prim->m_GI[ec].vecB_pre.y=-0;
			g_prim->m_GI[ec].vecB_pre.z=0;
			g_prim->m_GI[ec].height[0]=random(360);

			g_prim->m_GI[ec].height[2]=random(180);


			g_prim->m_GI[ec].height[12]=230;

			g_prim->m_GI[ec].vecB_now.x=0;
			g_prim->m_GI[ec].vecB_now.y=-0;
			g_prim->m_GI[ec].vecB_now.z=0;
			g_prim->m_GI[ec].height[4]=random(360);

			g_prim->m_GI[ec].height[6]=random(180);


			g_prim->m_GI[ec].height[13]=230;

			g_prim->m_GI[ec].vecT_pre.x=0;
			g_prim->m_GI[ec].vecT_pre.y=-0;
			g_prim->m_GI[ec].vecT_pre.z=0;
			g_prim->m_GI[ec].height[8]=random(360);

			g_prim->m_GI[ec].height[10]=random(180);


			g_prim->m_GI[ec].height[14]=230;

			g_prim->m_GI[ec].height[15]=random(3)+1;
		}
	}
}

void	CRagEffect :: SphereWind2(char *tName, int time, char F1)
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_SPHEREWIND2);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 1;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=10;



		int ec;

		for (ec=0;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = -random(20);

			g_prim->m_GI[ec].max_height = 2.0f+(random(11)*0.2f);

			g_prim->m_GI[ec].distance = 5.0f;
			g_prim->m_GI[ec].height[0] = -10.0f;
			g_prim->m_GI[ec].RotStart = random(360);
			g_prim->m_GI[ec].rise_angle = (ec*45)+time;
			g_prim->m_GI[ec].alphaB = 0;
			g_prim->m_GI[ec].full_display_angle=180+random(91);
			g_prim->m_GI[ec].height[1] = 3+random(4);
		}
	}
}

void	CRagEffect :: Heal2(char *tName , char F1)
{
	m_pos = m_master->m_pos;


	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_HEAL);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		if (F1 == 2)
			g_prim->m_size = 4;
		else
			g_prim->m_size=9;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 60.0f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 4.6f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].vecB_now.y=0;
		if (F1 == 0)
			g_prim->m_GI[ec].alphaT=60;
		else
			g_prim->m_GI[ec].alphaT=40;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 60.0f;
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 5.0f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].vecB_now.y=0;
		if (F1 == 0)
			g_prim->m_GI[ec].alphaT=60;
		else
			g_prim->m_GI[ec].alphaT=40;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		if (F1 == 0)
			g_prim->m_GI[ec].max_height = 12.0f;
		else
			g_prim->m_GI[ec].max_height = 18.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 5.0f;
		g_prim->m_GI[ec].rise_angle=50;
		g_prim->m_GI[ec].vecB_now.y=0;
		if (F1 == 0)
			g_prim->m_GI[ec].alphaT=60;
		else
			g_prim->m_GI[ec].alphaT=40;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		if (F1 == 0)
			g_prim->m_GI[ec].max_height = 12.0f;
		else
			g_prim->m_GI[ec].max_height = 18.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 5.2f;
		g_prim->m_GI[ec].rise_angle=48;
		g_prim->m_GI[ec].vecB_now.y=0;
		if (F1 == 0)
			g_prim->m_GI[ec].alphaT=60;
		else
			g_prim->m_GI[ec].alphaT=40;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		if (F1 < 2)
		{
			vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
			PlayWave("_heal_effect.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		}
	}
	else if ( m_stateCnt <= 50 && m_stateCnt %6 == 0 && F1 < 2 )
	{
		int ec;

		g_prim = LaunchEffectPrim(PP_PARTICLE_UP);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 2;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\pok1.tga", false);
		g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\pok3.tga", false);

		g_prim->m_size=0;
		g_prim->m_pos = m_pos;



		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process=0;

			g_prim->m_GI[ec].distance = (random(10)*0.1f) + 2.0f;
			g_prim->m_GI[ec].height[8] = 0;
			g_prim->m_GI[ec].alphaB = 0;

			g_prim->m_GI[ec].vecB_now.x = m_pos.x + random(13) - 6;
			g_prim->m_GI[ec].vecB_now.z = m_pos.z + random(13) - 6;
			g_prim->m_GI[ec].vecB_now.y = m_pos.y;

			g_prim->m_GI[ec].RotStart=0;

			g_prim->m_GI[ec].height[1] = 0;
			if (F1 == 0)
				g_prim->m_GI[ec].height[2] = (random(40)*0.01f) + 0.4f;
			else
				g_prim->m_GI[ec].height[2] = (random(40)*0.01f) + 0.6f;
			g_prim->m_GI[ec].full_display_angle = 0;
			g_prim->m_GI[ec].flag1[0] = 0;
			g_prim->m_GI[ec].flag1[1] = 1;
			g_prim->m_GI[ec].flag1[2] = 11;
			g_prim->m_GI[ec].height[7] = 0;
		}
	}



	if (F1 == 1 && m_stateCnt == 1)
	{
		g_prim = LaunchEffectPrim(PP_HEAL);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);

		g_prim->m_size=9;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 90.0f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 4.2f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT=40;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 90.0f;
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 4.3f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT=40;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 10.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 5.3f;
		g_prim->m_GI[ec].rise_angle=46;
		g_prim->m_GI[ec].alphaT=30;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 10.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 5.4f;
		g_prim->m_GI[ec].rise_angle=44;
		g_prim->m_GI[ec].alphaT=30;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}
	}

	if ( F1 == 1 && m_stateCnt <= 50 && m_stateCnt %9 == 2 )
	{
		int ec;

		g_prim = LaunchEffectPrim(PP_PARTICLE_UP);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_totalTexture = 2;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\pok1.tga", false);
		g_prim->m_texture[1] = g_texMgr.GetTexture("effect\\pok3.tga", false);

		g_prim->m_size=0;
		g_prim->m_pos = m_pos;



		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process=0;

			g_prim->m_GI[ec].distance = (random(10)*0.1f) + 2.4f;
			g_prim->m_GI[ec].height[8] = 0;
			g_prim->m_GI[ec].alphaB = 0;

			g_prim->m_GI[ec].vecB_now.x = m_pos.x + random(13) - 6;
			g_prim->m_GI[ec].vecB_now.z = m_pos.z + random(13) - 6;
			g_prim->m_GI[ec].vecB_now.y = m_pos.y;

			g_prim->m_GI[ec].RotStart=0;

			g_prim->m_GI[ec].height[1] = 0;
			g_prim->m_GI[ec].height[2] = (random(40)*0.01f) + 0.4f;
			g_prim->m_GI[ec].full_display_angle = 0;
			g_prim->m_GI[ec].flag1[0] = 0;
			g_prim->m_GI[ec].flag1[1] = 1;
			g_prim->m_GI[ec].flag1[2] = 11;
			g_prim->m_GI[ec].height[7] = 0;
		}
	}
}

void	CRagEffect::ReadyPortal2()
{
	if (m_stateCnt%14 == 0)
	{
		m_pos = m_master->m_pos;
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_ReadyPortal.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_PORTAL);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);

		g_prim->m_size=5;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 6.001f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 0.0f;
		g_prim->m_GI[ec].rise_angle=2;
		g_prim->m_GI[ec].alphaT=100;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 1;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = -10;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 6.001f;
		g_prim->m_GI[ec].RotStart = 25;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 0.0f;
		g_prim->m_GI[ec].rise_angle=3;
		g_prim->m_GI[ec].alphaT=100;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 1;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = -20;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 6.001f;
		g_prim->m_GI[ec].RotStart = 50;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 0.0f;
		g_prim->m_GI[ec].rise_angle=4;
		g_prim->m_GI[ec].alphaT=100;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 1;
		}

		ec=3;
		g_prim->m_GI[ec].life = 0;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].alphaT = 1;

	}
}

void	CRagEffect :: PortalWind(char F1)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		g_prim = LaunchEffectPrim(PP_WIND);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\cloud11.tga", false);

		if (F1==1)
			g_prim->m_size=8;
		else
			g_prim->m_size=5;



		int ec,i;

		for (ec=0;ec<4;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process = 0;
			g_prim->m_GI[ec].full_display_angle = 30;
			if (F1 == 1)
				g_prim->m_GI[ec].max_height = 3.0f+(ec*2.0f);
			else if (F1 == 2)
				g_prim->m_GI[ec].max_height = 48.0f-(ec*5.0f);
			else if (F1 == 3)
				g_prim->m_GI[ec].max_height = 28.0f-(ec*5.0f);
			else
				g_prim->m_GI[ec].max_height = 5.0f+(ec*2.0f);
			g_prim->m_GI[ec].RotStart = 0+(90*ec);
			g_prim->m_GI[ec].alphaB = 0;
			if (F1 >= 2)
				g_prim->m_GI[ec].alphaT = 2;
			else
				g_prim->m_GI[ec].alphaT = 1;
			if (F1 == 1)
				g_prim->m_GI[ec].distance = 2.5f + (random(101)*0.01f);
			else if (F1 == 2)
				g_prim->m_GI[ec].distance = 14.0f - (ec*2.0f);
			else if (F1 == 3)
				g_prim->m_GI[ec].distance = 6.0f - (ec*1.0f);
			else
				g_prim->m_GI[ec].distance = 4.5f + (random(101)*0.03f);
			g_prim->m_GI[ec].rise_angle=80+random(21);


			for (i = 0 ; i<E_DIVISION ; i++)
			{
				g_prim->m_GI[ec].height[i] = 1.0f;
				g_prim->m_GI[ec].flag1[i] = 0;
			}
		}
	}
}

void	CRagEffect :: Portal2(char F1)
{
	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

		g_prim = LaunchEffectPrim(PP_HEAL);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		if (F1==3)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_red.tga", false);
			g_prim->m_size=10;
		}
		else
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\magic_violet.tga", false);
			g_prim->m_size=4;
		}



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 50.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 4.0f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT=1400;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 1;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 50.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 3.0f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT=1400;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 1;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 50.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 2.0f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT=1400;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 1;
		}

		g_prim->m_GI[3].life = 0;

	}




	if (m_stateCnt == 1)
	{
		m_pos = m_master->m_pos;
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

		g_prim = LaunchEffectPrim(PP_PORTAL);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		if (F1==3)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_red.tga", false);
			g_prim->m_size=10;
		}
		else
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);
			g_prim->m_size=4;
		}



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 6.001f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 0.0f;
		g_prim->m_GI[ec].rise_angle=2;
		g_prim->m_GI[ec].alphaT=3111;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 1;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;

		if (F1==3)
			g_prim->m_GI[ec].process = -20;
		else
			g_prim->m_GI[ec].process = -10;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 6.001f;
		g_prim->m_GI[ec].RotStart = 25;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 0.0f;
		g_prim->m_GI[ec].rise_angle=3;
		g_prim->m_GI[ec].alphaT=3111;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 1;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		if (F1==3)
			g_prim->m_GI[ec].process = -40;
		else
			g_prim->m_GI[ec].process = -20;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 6.001f;
		g_prim->m_GI[ec].RotStart = 50;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 0.0f;
		g_prim->m_GI[ec].rise_angle=4;
		g_prim->m_GI[ec].alphaT=3111;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 1;
		}

		ec=3;
		g_prim->m_GI[ec].life = 0;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].alphaT = F1;
	}

	if (m_stateCnt == 2)
	{
		g_prim = LaunchEffectPrim(PP_WIND);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\cloud11.tga", false);

		if (F1==3)
		{
			g_prim->m_size=10;
		}
		else
		{
			g_prim->m_size=4;
		}



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 30;
		g_prim->m_GI[ec].max_height = 1.5f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 9.0f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT = 0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 1.5f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 30;
		g_prim->m_GI[ec].max_height = 3.5f;
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 7.5f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT = 0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 1.5f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 30;
		g_prim->m_GI[ec].max_height = 5.5f;
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 6.0f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT = 0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 1.5f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 30;
		g_prim->m_GI[ec].max_height = 7.5f;
		g_prim->m_GI[ec].RotStart = 270;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 4.5f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT = 0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 1.5f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}
	}
}

void	CRagEffect :: Portal3(char F1)
{
	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

		g_prim = LaunchEffectPrim(PP_HEAL);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		if (F1==3)
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_red.tga", false);
			g_prim->m_size=10;
		}
		else
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\magic_violet.tga", false);
			g_prim->m_size=4;
		}



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 80.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 7.0f;
		g_prim->m_GI[ec].rise_angle=90;
		if (F1 == 1)
			g_prim->m_GI[ec].alphaT=-1;
		else
			g_prim->m_GI[ec].alphaT=1400;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 3;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 80.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 5.5f;
		g_prim->m_GI[ec].rise_angle=90;
		if (F1 == 1)
			g_prim->m_GI[ec].alphaT=-1;
		else
			g_prim->m_GI[ec].alphaT=1400;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 3;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 80.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 4.0f;
		g_prim->m_GI[ec].rise_angle=90;
		if (F1 == 1)
			g_prim->m_GI[ec].alphaT=-1;
		else
			g_prim->m_GI[ec].alphaT=1400;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 3;
		}

		g_prim->m_GI[3].life = 0;
	}

	if (m_stateCnt == 2)
	{
		g_prim = LaunchEffectPrim(PP_WIND);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\cloud11.tga", false);

		if (F1==3)
		{
			g_prim->m_size=10;
		}
		else
		{
			g_prim->m_size=4;
		}



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 30;
		g_prim->m_GI[ec].max_height = 2.5f;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 13.0f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT = 3;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 1.5f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 30;
		g_prim->m_GI[ec].max_height = 5.0f;
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 11.0f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT = 3;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 1.5f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 30;
		g_prim->m_GI[ec].max_height = 7.5f;
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].alphaT = 3;
		g_prim->m_GI[ec].distance = 9.0f;
		g_prim->m_GI[ec].rise_angle=90;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 1.5f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 30;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].max_height = 10.0f;
		g_prim->m_GI[ec].RotStart = 270;
		g_prim->m_GI[ec].distance = 8.0f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT = 3;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 1.5f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}
	}
}

void	CRagEffect :: Entry2()
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

		PlayWave("effect\\EF_Portal.wav", m_pos.x-pPos.x, -150,  m_pos.z-pPos.z);

		g_prim = LaunchEffectPrim(PP_HEAL);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		{
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\ring_blue.tga", false);
			g_prim->m_size=4;
		}



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 30.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;

		g_prim->m_GI[ec].distance = 3.7f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT=1400;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 2;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 30.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;

		g_prim->m_GI[ec].distance = 3.4f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT=1400;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 2;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 4.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;

		g_prim->m_GI[ec].distance = 3.6f;
		g_prim->m_GI[ec].rise_angle=10;
		g_prim->m_GI[ec].alphaT=1400;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 2;
		}

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 4.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;

		g_prim->m_GI[ec].distance = 3.7f;
		g_prim->m_GI[ec].rise_angle=5;
		g_prim->m_GI[ec].alphaT=1400;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 2;
		}
	}


}

void CRagEffect::WarpZone2(char *tName)
{
	int ec,i;
	int angle;

	if (m_stateCnt == 99999998)
		m_stateCnt = 0;


	if (m_stateCnt == 1)
	{
		m_pos = m_master->m_pos;

		g_prim = LaunchEffectPrim(PP_3DCASTING_4);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=11;

		angle = 60 ;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 270;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 2.5f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;
		g_prim->m_GI[ec].rise_angle=angle-7;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 5.0f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;
		g_prim->m_GI[ec].rise_angle=angle;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 7.5f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;

		g_prim->m_GI[ec].rise_angle=angle-5;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 10.0f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;
		g_prim->m_GI[ec].rise_angle=angle-10;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}
	}

	if (m_stateCnt == 2)
	{
		m_pos = m_master->m_pos;
		g_prim = LaunchEffectPrim(PP_3DCASTING_4);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);
		g_prim->m_size=4;




		angle = 60;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 271;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 2.7f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;
		g_prim->m_GI[ec].rise_angle=angle-8;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 1;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 5.2f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;
		g_prim->m_GI[ec].rise_angle=angle-1;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 91;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 7.7f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;
		g_prim->m_GI[ec].rise_angle=angle-6;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 181;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 10.2f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;
		g_prim->m_GI[ec].rise_angle=angle-11;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}
	}




	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		g_prim = LaunchEffectPrim(PP_3DCIRCLE);

		g_prim->m_duration = 99999999;

		g_prim->m_pattern |= PT_FILLCIRCLE;
		g_prim->m_radius = 15;
		g_prim->m_deltaPos2.y -= 1.5f;
		g_prim->m_latitude = 90;
		g_prim->m_alpha = 0;
		g_prim->m_maxAlpha = 128;
		g_prim->m_alphaSpeed = g_prim->m_maxAlpha / 10;
		g_prim->m_fadeOutCnt = g_prim->m_duration - 10;
		g_prim->InitSegment();
		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_down.tga", false);
	}


	if (m_stateCnt % 10 == 0)
	{
		float	rndNum;
		g_prim = LaunchEffectPrim(PP_3DPARTICLEORBIT);

		rndNum = random(200) / 100;
		g_prim->m_radius = 4.5f + (rndNum * 0.75f);
		g_prim->m_radiusSpeed = 0.0005f;
		g_prim->m_duration = 70;
		g_prim->m_longitude = random(360);
		if (random(2) == 0)
			g_prim->m_longSpeed = 2.5f - (random(11)*0.1f);
		else
			g_prim->m_longSpeed = -2.5f + (random(11)*0.1f);
		g_prim->m_gravSpeed = -0.3f;
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

void CRagEffect::BLACKDEVIL(char *tName)
{
	int ec,i;
	int angle;

	m_pos = m_master->m_pos;

	if (m_stateCnt == 1)
	{
		g_prim = LaunchEffectPrim(PP_3DCASTING_5);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);


		g_prim->m_size=12;




		angle = 60;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 270;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 2.5f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;
		g_prim->m_GI[ec].rise_angle=angle-7;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++) {
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 0;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 5.0f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;

		g_prim->m_GI[ec].rise_angle=angle;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++) {
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 90;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 7.5f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;

		g_prim->m_GI[ec].rise_angle=angle-5;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++) {
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 180;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 10.0f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;

		g_prim->m_GI[ec].rise_angle=angle-10;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++) {
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}
	}

	if (m_stateCnt == 2)
	{
		g_prim = LaunchEffectPrim(PP_3DCASTING_5);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);


		g_prim->m_size=12;




		angle = 60;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 271;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 2.7f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;

		g_prim->m_GI[ec].rise_angle=angle-8;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++) {
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 1;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 5.2f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;

		g_prim->m_GI[ec].rise_angle=angle-1;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++) {
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 91;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 7.7f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;

		g_prim->m_GI[ec].rise_angle=angle-6;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++) {
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=3;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].RotStart = 181;
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 10.2f;
		g_prim->m_GI[ec].max_height = g_prim->m_GI[ec].distance;

		g_prim->m_GI[ec].rise_angle=angle-11;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++) {
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

	}
}

bool	CRagEffect :: GlassWall2(char *tName)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_GlassWall.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		g_prim = LaunchEffectPrim(PP_HEAL);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
		g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

		g_prim->m_size=10;



		int ec,i;

		ec=0;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 34.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 3.6f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT=3080;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=1;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 37.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 3.3f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT=3080;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		ec=2;
		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process = 0;
		g_prim->m_GI[ec].full_display_angle = 360;
		g_prim->m_GI[ec].max_height = 40.0f;
		g_prim->m_GI[ec].RotStart = random(360);
		g_prim->m_GI[ec].alphaB = 0;
		g_prim->m_GI[ec].distance = 3.0f;
		g_prim->m_GI[ec].rise_angle=90;
		g_prim->m_GI[ec].alphaT=3080;
		g_prim->m_GI[ec].vecB_now.y=0;

		for (i = 0 ; i<E_DIVISION ; i++)
		{
			g_prim->m_GI[ec].height[i] = 0.0f;
			g_prim->m_GI[ec].flag1[i] = 0;
		}

		g_prim->m_GI[3].life = 0;
	}

	ProcessEZ2STR();
	if (m_stateCnt == m_duration)
		return false;
	else return true;
}

void	CRagEffect :: TANJI()
{
	int ec;
	float x,z;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{

		{
			vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

			PlayWave("effect\\mon_\xc5\xba\xc1\xf6\xbd\xc5\xc5\xeb.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

			g_prim = LaunchEffectPrim(PP_SHIELDBOOMERANG);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\blue_ivy.bmp", false);

			g_prim->m_size=0;

			g_prim->m_deltaPos = m_deltaPos;

			x=m_deltaPos.x-m_pos.x;
			z=m_deltaPos.z-m_pos.z;




			ec=0;

			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process=-15;
			g_prim->m_GI[ec].max_height=_sqrt( (x*x) + (z*z) );;

			g_prim->m_GI[ec].distance = 2.0f+(m_count*0.4f);
			g_prim->m_GI[ec].height[8] = 0;

			g_prim->m_GI[ec].alphaB = 5;

			g_prim->m_GI[ec].vecB_now.x=m_pos.x;
			g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			g_prim->m_GI[ec].vecB_now.y = m_pos.y-8.0f;

			g_prim->m_GI[ec].RotStart=0;
			g_prim->m_GI[ec].height[1] = 2.0f;
			g_prim->m_GI[ec].height[2] = (m_deltaPos.y-m_pos.y)*(g_prim->m_GI[ec].height[1] / g_prim->m_GI[ec].max_height);

			g_prim->m_GI[ec].full_display_angle = 0;
			g_prim->m_GI[ec].flag1[0] = 0;
			g_prim->m_GI[ec].flag1[1] = 0;
			g_prim->m_GI[ec].flag1[2] = 1;
			g_prim->m_GI[ec].height[7] = 0;

			g_prim->m_GI[1].life = 1;
			g_prim->m_GI[1].alphaB = 0;

			g_prim->m_GI[2].life = 1;
			g_prim->m_GI[2].alphaB = 0;

			g_prim->m_GI[3].life = 1;
			g_prim->m_GI[3].alphaB = 0;
		}
	}
}

void	CRagEffect :: TANJI2()
{
	int ec;
	float x,z;

	m_pos = m_master->m_pos;

	if (m_stateCnt  == 0)
	{

		{
			vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;


			g_prim = LaunchEffectPrim(PP_TANJI2);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\blue_ivy.bmp", false);

			g_prim->m_size=0;

			g_prim->m_deltaPos = m_deltaPos;

			x=m_deltaPos.x-m_pos.x;
			z=m_deltaPos.z-m_pos.z;



			ec=0;

			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process=-15;
			g_prim->m_GI[ec].max_height=_sqrt( (x*x) + (z*z) );;

			g_prim->m_GI[ec].distance = 2.0f+(m_count*0.4f);
			g_prim->m_GI[ec].height[8] = 0;
			g_prim->m_GI[ec].alphaB = 5;

			g_prim->m_GI[ec].vecB_now.x=m_pos.x;
			g_prim->m_GI[ec].vecB_now.z=m_pos.z;
			g_prim->m_GI[ec].vecB_now.y = m_pos.y-8.0f;

			g_prim->m_GI[ec].RotStart=0;
			g_prim->m_GI[ec].height[1] = 2.0f;
			g_prim->m_GI[ec].height[2] = (m_deltaPos.y-m_pos.y)*(g_prim->m_GI[ec].height[1] / g_prim->m_GI[ec].max_height);

			g_prim->m_GI[ec].full_display_angle = 0;
			g_prim->m_GI[ec].flag1[0] = 0;
			g_prim->m_GI[ec].flag1[1] = 0;
			g_prim->m_GI[ec].flag1[2] = 1;
			g_prim->m_GI[ec].height[7] = 0;

			g_prim->m_GI[1].life = 1;
			g_prim->m_GI[1].alphaB = 0;

			g_prim->m_GI[2].life = 1;
			g_prim->m_GI[2].alphaB = 0;

			g_prim->m_GI[3].life = 1;
			g_prim->m_GI[3].alphaB = 0;

		}
	}








}

void	CRagEffect :: TRIPLEATTACK()
{
	int ec;
	float x,z;

	m_pos = m_master->m_pos;

	if (m_stateCnt <= 1 )
	{
			g_prim = LaunchEffectPrim(PP_TRIPLEATTACK);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\cloud11.tga", false);

			g_prim->m_size=0;

			g_prim->m_deltaPos = m_deltaPos;

			x=m_deltaPos.x-m_pos.x;
			z=m_deltaPos.z-m_pos.z;




			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process=-random(20);

				g_prim->Get_Angle(x,z,&g_prim->m_GI[ec].RotStart);
				g_prim->m_GI[ec].alphaB = 0;

				g_prim->m_GI[ec].vecB_now.x = m_pos.x + ((random(100)*0.06f)-3.0f);
				g_prim->m_GI[ec].vecB_now.z = m_pos.z + ((random(100)*0.06f)-3.0f);;
				g_prim->m_GI[ec].vecB_now.y = m_pos.y;

				g_prim->m_GI[ec].height[1] = random(100)*0.15f;

				g_prim->m_GI[ec].vecB_now.x -= (g_prim->m_GI[ec].height[1]*g_cosTable[g_prim->m_GI[ec].RotStart]);
				g_prim->m_GI[ec].vecB_now.z -= (g_prim->m_GI[ec].height[1]*g_sinTable[g_prim->m_GI[ec].RotStart]);

				g_prim->m_GI[ec].vecT_now.x = g_prim->m_GI[ec].vecB_now.x;
				g_prim->m_GI[ec].vecT_now.z = g_prim->m_GI[ec].vecB_now.z;
				g_prim->m_GI[ec].vecT_now.y = m_pos.y;

				g_prim->m_GI[ec].max_height=0.25f;

				g_prim->m_GI[ec].height[0] = (random(100)*0.04f) + 8.0f;
				g_prim->m_GI[ec].distance = 20.0f + (random(100)*0.2f);

				g_prim->m_GI[ec].height[4] = 0;
			}
	}

	if (m_stateCnt  == 10)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_hit2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (m_stateCnt  == 20)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_hit4.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}

	if (m_stateCnt  == 30)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_hit2.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
}

void	CRagEffect :: TRIPLEATTACK2()
{
	int ec;
	float x,z;

	m_pos = m_master->m_pos;

	if ( m_stateCnt <= 3 )
	{
			g_prim = LaunchEffectPrim(PP_TRIPLEATTACK);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_center.tga", false);

			g_prim->m_size=0;

			g_prim->m_deltaPos = m_deltaPos;

			x=m_deltaPos.x-m_pos.x;
			z=m_deltaPos.z-m_pos.z;

			float	xz = _sqrt(x*x + z*z);




			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process=-random(20)-45;

				g_prim->Get_Angle(x,z,&g_prim->m_GI[ec].RotStart);
				g_prim->m_GI[ec].alphaB = 0;

				g_prim->m_GI[ec].vecB_now.x = m_pos.x + ((random(100)*0.08f)-4.0f);
				g_prim->m_GI[ec].vecB_now.z = m_pos.z + ((random(100)*0.08f)-4.0f);;
				g_prim->m_GI[ec].vecB_now.y = m_pos.y;

				g_prim->m_GI[ec].height[1] = random(100)*0.15f;

				g_prim->m_GI[ec].vecB_now.x -= (g_prim->m_GI[ec].height[1]*g_cosTable[g_prim->m_GI[ec].RotStart]);
				g_prim->m_GI[ec].vecB_now.z -= (g_prim->m_GI[ec].height[1]*g_sinTable[g_prim->m_GI[ec].RotStart]);

				g_prim->m_GI[ec].vecT_now.x = g_prim->m_GI[ec].vecB_now.x;
				g_prim->m_GI[ec].vecT_now.z = g_prim->m_GI[ec].vecB_now.z;
				g_prim->m_GI[ec].vecT_now.y = m_pos.y;

				g_prim->m_GI[ec].max_height=0.05f;

				g_prim->m_GI[ec].height[0] = (random(100)*0.04f) + 8.0f;
				g_prim->m_GI[ec].distance = xz + (random(100)*0.6f);

				g_prim->m_GI[ec].height[4] = 2;
			}
	}

}

void	CRagEffect :: TRIPLEATTACK3()
{
	int ec;
	float x,z;

	m_pos = m_master->m_pos;

	if ( m_stateCnt <= 8 )
	{
			g_prim = LaunchEffectPrim(PP_TRIPLEATTACK);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\alpha_center.tga", false);

			g_prim->m_size=0;

			g_prim->m_deltaPos = m_deltaPos;

			x=m_deltaPos.x-m_pos.x;
			z=m_deltaPos.z-m_pos.z;

			float	xz = _sqrt(x*x + z*z);




			for (ec=0;ec<4;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process=-random(110)-30;

				g_prim->Get_Angle(x,z,&g_prim->m_GI[ec].RotStart);
				g_prim->m_GI[ec].alphaB = 0;

				g_prim->m_GI[ec].vecB_now.x = m_pos.x + ((random(100)*0.08f)-4.0f);
				g_prim->m_GI[ec].vecB_now.z = m_pos.z + ((random(100)*0.08f)-4.0f);;
				g_prim->m_GI[ec].vecB_now.y = m_pos.y;

				g_prim->m_GI[ec].height[1] = random(100)*0.15f;

				g_prim->m_GI[ec].vecB_now.x -= (g_prim->m_GI[ec].height[1]*g_cosTable[g_prim->m_GI[ec].RotStart]);
				g_prim->m_GI[ec].vecB_now.z -= (g_prim->m_GI[ec].height[1]*g_sinTable[g_prim->m_GI[ec].RotStart]);

				g_prim->m_GI[ec].vecT_now.x = g_prim->m_GI[ec].vecB_now.x;
				g_prim->m_GI[ec].vecT_now.z = g_prim->m_GI[ec].vecB_now.z;
				g_prim->m_GI[ec].vecT_now.y = m_pos.y;

				g_prim->m_GI[ec].max_height=0.02f+(random(11)*0.008f);

				g_prim->m_GI[ec].height[0] = (random(100)*0.06f) + 4.0f;
				g_prim->m_GI[ec].distance = xz + (random(100)*0.3f);

				g_prim->m_GI[ec].height[4] = 3;
			}
	}

}

void	CRagEffect :: CHIMTO(char F1)
{
	float Rx,Rz;
	short angle;

	m_pos = m_master->m_pos;

	if ( m_stateCnt == 0 )
	{
		if (F1 == 0)
		{
			vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

			PlayWave("effect\\mon_\xc4\xa7\xc5\xf5\xb0\xe6.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

		}

		g_prim = LaunchEffectPrim(PP_CHIMTO);

		g_prim->m_duration = m_duration;




		int ec;

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].max_height=0;

			g_prim->m_GI[ec].vecB_pre.x=m_deltaPos.x;
			g_prim->m_GI[ec].vecB_pre.y=m_deltaPos.y-0.0f;
			g_prim->m_GI[ec].vecB_pre.z=m_deltaPos.z;
		}

		if (F1 == 0)
		{
			g_prim->m_alpha = 200;

			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc4\xa7\xc5\xf5\xb0\xe6_.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc4\xa7\xc5\xf5\xb0\xe6_.act");

			g_prim->m_animSpeed=2;
			g_prim->m_size = 2.5f;
		}
		else if (F1 == 1)
		{
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\wink.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\wink.act");
			g_prim->m_animSpeed=6;
			g_prim->m_size = 1.0f;
			g_prim->m_repeatAnim = false;
			g_prim->m_pattern |= PT_USEORGARGB;
		}
		else if (F1 == 2)
		{
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\fvoice.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\fvoice.act");
			g_prim->m_animSpeed=6;
			g_prim->m_size = 1.0f;
			g_prim->m_repeatAnim = false;
			g_prim->m_pattern |= PT_USEORGARGB;
		}

		Rx=m_pos.x-m_deltaPos.x;
		Rz=m_pos.z-m_deltaPos.z;

		g_prim->Get_Angle(Rx,Rz,&angle);

		int a2;
		a2=g_modeMgr.GetGameMode()->GetView()->GetCurLongitude();

		angle=angle+a2;
		angle%=360;
		if (angle<0)
			angle+=360;

		g_prim->m_renderFlag |= RF_NODEPTHCHECK;

		if ( angle>=180 && angle<270 )
			g_prim->SetAction(0, 16, MT_ONETIME);
		else if ( angle>=90 && angle<180 )
			g_prim->SetAction(1, 16, MT_ONETIME);
		else if ( angle>=0 && angle<90)
			g_prim->SetAction(2, 16, MT_ONETIME);
		else
			g_prim->SetAction(3, 16, MT_ONETIME);

		g_prim->InitSegment();

			for (ec=1;ec<4;ec++)
				g_prim->m_GI[ec].life = 0;
		}
}

void	CRagEffect :: Demonstration()
{

	if ( m_stateCnt == 0 )
	{
		m_pos = m_master->m_pos;

		g_prim = LaunchEffectPrim(PP_BOTTOMSPR);

		g_prim->m_duration = m_duration;




		int ec;

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].max_height=0;
			g_prim->m_size = 1.2f;

			g_prim->m_GI[ec].vecB_pre=m_pos;
			g_prim->m_GI[ec].vecB_pre.y-=1.0f;

			g_prim->m_GI[ec].flag1[4]=0;
		}

		g_prim->m_alpha = 200;

		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb5\xa5\xb8\xf3\xbd\xba\xc6\xae\xb7\xb9\xc0\xcc\xbc\xc7.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb5\xa5\xb8\xf3\xbd\xba\xc6\xae\xb7\xb9\xc0\xcc\xbc\xc7.act");

		g_prim->m_animSpeed=4;
		g_prim->SetAction(0, 16, 0);
		g_prim->InitSegment();

		for (ec=1;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;
	}
}

void	CRagEffect :: NpcStop(char F1)
{
	m_pos = m_master->m_pos;
	if ( m_stateCnt == 0 )
	{
		m_pos = m_master->m_pos;

		g_prim = LaunchEffectPrim(PP_BOTTOMSPR);

		g_prim->m_renderFlag |= RF_NODEPTHCHECK;

		g_prim->m_duration = m_duration;




		int ec;
		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].max_height=0;
			g_prim->m_size = 1.0f;

			g_prim->m_GI[ec].vecB_pre=m_pos;
			if (F1 != 2)
				g_prim->m_GI[ec].vecB_pre.y-=7.0f;

			g_prim->m_GI[ec].flag1[4] = 1;
		}

		if (F1 == 2)
		{
			g_prim->m_alpha = 100;
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\cconfine.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\cconfine.act");
			g_prim->m_animSpeed=12;
			g_prim->m_repeatAnim = false;
			g_prim->m_pattern = PT_STOP_AT_LAST;
		}
		else
		{
			g_prim->m_alpha = 200;
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbd\xba\xc5\xe9.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbd\xba\xc5\xe9.act");

			g_prim->m_animSpeed=4;
			g_prim->SetAction(0, 16, 0);
		}

		g_prim->InitSegment();

		for (ec=1;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;
	}
}

void	CRagEffect :: Bottom_Spr()
{

	if ( m_stateCnt == 0 )
	{
		m_pos = m_master->m_pos;

		g_prim = LaunchEffectPrim(PP_BOTTOMSPR);

		g_prim->m_duration = m_duration;




		int ec;

		for (ec=0;ec<1;ec++)
		{
			g_prim->m_GI[ec].life = 1;
			g_prim->m_GI[ec].process=0;
			g_prim->m_GI[ec].max_height=0;
			g_prim->m_size = 1.5f;

			g_prim->m_GI[ec].vecB_pre=m_pos;
			g_prim->m_GI[ec].vecB_pre.y-=5.0f;
			g_prim->m_GI[ec].flag1[4]=0;
		}

		g_prim->m_alpha = 250;
		g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\ring.spr");
		g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\ring.act");

		g_prim->m_animSpeed=4;
		g_prim->SetAction(0, 16, 0);
		g_prim->InitSegment();

		for (ec=1;ec<4;ec++)
			g_prim->m_GI[ec].life = 0;
	}
}

void	CRagEffect :: CHOOKGI(char F1)
{

	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		int i;
		int maxnum;
		maxnum = m_deltaPos.x;
		if (maxnum > 5)
			maxnum = 5;
		for (i=0;i<maxnum;i++)
		{
			g_prim = LaunchEffectPrim(PP_CHOOKGI);
			g_prim->m_renderFlag = RF_EFFECT;
			g_prim->m_duration = m_duration;

			int ec;
			ec=0;

			g_prim->m_GI[ec].life=1;
			g_prim->m_GI[ec].height[0]=random(360);
			g_prim->m_GI[ec].height[1]=random(360);
			g_prim->m_GI[ec].height[2]=random(360);
			g_prim->m_GI[ec].height[3]=random(360);
			g_prim->m_GI[ec].height[4]=random(360);
			g_prim->m_GI[ec].height[5]=random(360);

			if (F1 == 1)
			{
				g_prim->m_GI[ec].height[6]=i*72;

				g_prim->m_GI[ec].height[7]=-14.0f;
				g_prim->m_GI[ec].distance = 1.3f;
				g_prim->m_GI[ec].height[10]=1;
				g_prim->m_GI[ec].flag1[0]=0;
			}
			else if (F1 == 3)
			{
				if (m_deltaPos.x <= 5)
					g_prim->m_GI[ec].height[6]=i*360/m_deltaPos.x;
				else
					g_prim->m_GI[ec].height[6]=i*72;

				g_prim->m_GI[ec].height[7]=-15.0f;

				g_prim->m_GI[ec].distance = 1.0f;

				if (m_deltaPos.x > 5)
				{
					if (m_deltaPos.x - i >= 6)
						g_prim->m_GI[ec].distance = 2.0f;
				}

				g_prim->m_GI[ec].height[10]=0;
				g_prim->m_GI[ec].flag1[0]=1;
			}
			else
			{
				g_prim->m_GI[ec].height[6]=i*72;

				g_prim->m_GI[ec].height[7]=-15.0f;
				g_prim->m_GI[ec].distance = 2.0f;
				g_prim->m_GI[ec].height[10]=0;
				g_prim->m_GI[ec].flag1[0]=0;
			}

			g_prim->m_GI[ec].height[8]=random(360);

			g_prim->m_GI[ec].alphaB=0;
			g_prim->m_GI[ec].process=0;

			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\thunder_center.bmp", false);

		}
	}

}

bool	CEffectPrim :: PrimCHOOKGI()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;



	int ec;
	ec=0;

	m_pos = m_master->m_pos;

	if ( m_GI[ec].life == 1 )
	{
			int i;

			m_GI[ec].process++;

			if (m_GI[ec].process<=200)
				m_GI[ec].alphaB+=1;

			for (i=0;i<6;i+=2)
			{
				if (m_GI[ec].height[i+1] > m_GI[ec].height[i])
				{
					m_GI[ec].height[i]+=1;
					if (m_GI[ec].height[i] > 359)
						m_GI[ec].height[i] = 359;
					if (m_GI[ec].height[i] > m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = random(int(m_GI[ec].height[i]));
				}
				else
				{
					m_GI[ec].height[i]-=1;
					if (m_GI[ec].height[i] < 0)
						m_GI[ec].height[i] = 0;
					if (m_GI[ec].height[i] <= m_GI[ec].height[i+1])
						m_GI[ec].height[i+1] = 359 - random(int(m_GI[ec].height[i]));
				}
			}

			if (m_GI[ec].flag1[0] == 0)
			{
				m_GI[ec].height[6]++;
				if (m_GI[ec].height[6]>=360)
					m_GI[ec].height[6]-=360;
			}
			else
			{
				if (m_GI[ec].process%2 == 0)
				{
					m_GI[ec].height[6]++;
					if (m_GI[ec].height[6]>=360)
						m_GI[ec].height[6]-=360;
				}
			}

			if (m_GI[ec].height[10] == 1)
			{
				m_GI[ec].vecB_now.x = 0.7f * g_sinTable[int(m_GI[ec].height[0])];
				m_GI[ec].vecB_now.z = 0.7f * g_sinTable[int(m_GI[ec].height[2])];
				m_GI[ec].vecB_now.y = 0.7f * g_sinTable[int(m_GI[ec].height[4])];

				m_GI[ec].vecB_now.x += 5.5f * g_sinTable[int(m_GI[ec].height[6])];
				m_GI[ec].vecB_now.z += 5.5f * g_cosTable[int(m_GI[ec].height[6])];
			}
			else
			{
				if (m_GI[ec].flag1[0] == 0)
				{
					m_GI[ec].vecB_now.x = 1.0f * g_sinTable[int(m_GI[ec].height[0])];
					m_GI[ec].vecB_now.z = 1.0f * g_sinTable[int(m_GI[ec].height[2])];
					m_GI[ec].vecB_now.y = 1.0f * g_sinTable[int(m_GI[ec].height[4])];
				}
				else
				{
					m_GI[ec].vecB_now.x = 0.5f * g_sinTable[int(m_GI[ec].height[0])];
					m_GI[ec].vecB_now.z = 0.5f * g_sinTable[int(m_GI[ec].height[2])];
					m_GI[ec].vecB_now.y = 0.5f * g_sinTable[int(m_GI[ec].height[4])];
				}

					m_GI[ec].vecB_now.x += 7.0f * g_sinTable[int(m_GI[ec].height[6])];
					m_GI[ec].vecB_now.z += 7.0f * g_cosTable[int(m_GI[ec].height[6])];
			}

			m_GI[ec].vecB_now.y += m_GI[ec].height[7];

			m_GI[ec].height[8]+=5+random(5);
			if (m_GI[ec].height[8]>=360)
				m_GI[ec].height[8]-=360;
	}

	return true;
}

void	CEffectPrim :: RenderCHOOKGI(matrix &wtm)
{

	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vec5, vec6, vec7, vec8;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int Angle;
	float Rx;
	int ec;

	Angle = 0;

	for (ec=0 ; ec<1 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB>0 )
		{
			Rx=m_GI[ec].distance;
			Rx+=g_sinTable[int(m_GI[ec].height[8])]*m_GI[ec].distance*0.05f;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec1.x=cs1*Rx;
			vec1.z=sn1*Rx;
			vec1.y=0;

			cs1 = g_cosTable[Angle+90];
			sn1 = g_sinTable[Angle+90];
			vec2.x=cs1*Rx;
			vec2.z=sn1*Rx;
			vec2.y=0;

			cs1 = g_cosTable[Angle+180];
			sn1 = g_sinTable[Angle+180];
			vec3.x=cs1*Rx;
			vec3.z=sn1*Rx;
			vec3.y=0;

			cs1 = g_cosTable[Angle+270];
			sn1 = g_sinTable[Angle+270];
			vec4.x=cs1*Rx;
			vec4.z=sn1*Rx;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			vec1+=m_GI[ec].vecB_now;
			vec2+=m_GI[ec].vecB_now;
			vec3+=m_GI[ec].vecB_now;
			vec4+=m_GI[ec].vecB_now;

			if (m_GI[ec].flag1[0] == 1)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,0,68,42,30);
			else
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,0,0,0,255);



			Rx=m_GI[ec].distance*0.5f;
			Rx+=g_sinTable[int(m_GI[ec].height[8])]*m_GI[ec].distance*0.05f;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec1.x=cs1*Rx;
			vec1.z=sn1*Rx;
			vec1.y=0;

			cs1 = g_cosTable[Angle+90];
			sn1 = g_sinTable[Angle+90];
			vec2.x=cs1*Rx;
			vec2.z=sn1*Rx;
			vec2.y=0;

			cs1 = g_cosTable[Angle+180];
			sn1 = g_sinTable[Angle+180];
			vec3.x=cs1*Rx;
			vec3.z=sn1*Rx;
			vec3.y=0;

			cs1 = g_cosTable[Angle+270];
			sn1 = g_sinTable[Angle+270];
			vec4.x=cs1*Rx;
			vec4.z=sn1*Rx;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			vec1+=m_GI[ec].vecB_now;
			vec2+=m_GI[ec].vecB_now;
			vec3+=m_GI[ec].vecB_now;
			vec4+=m_GI[ec].vecB_now;

			if (m_GI[ec].flag1[0] == 1)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,0,255,255,255);
			else
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,0,255,255,0);
		}
	}
}

void	CEffectPrim :: RenderBottom2(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4;
	vector3d	vec5, vec6, vec7, vec8;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int Angle;
	int ec;

	Angle = 0;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB>0 )
		{
			vec1.x=m_GI[ec].vecB_now.x;
			vec1.z=m_GI[ec].vecB_now.z;
			vec1.y=-m_GI[ec].max_height;

			vec2.x=m_GI[ec].vecB_now.x;
			vec2.z=m_GI[ec].vecB_now.z;
			vec2.y=0;

			vec3.x=m_GI[ec].vecB_pre.x;
			vec3.z=m_GI[ec].vecB_pre.z;
			vec3.y=0;

			vec4.x=m_GI[ec].vecB_pre.x;
			vec4.z=m_GI[ec].vecB_pre.z;
			vec4.y=-m_GI[ec].max_height;



			if (m_GI[ec].height[0] == 1)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,1,0,0,255,155,155);
			else if (m_GI[ec].height[0] == 2)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,1,0,0,155,255,155);
			else if (m_GI[ec].height[0] == 3)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,0,0,255,255,100);
			else if (m_GI[ec].height[0] == 4)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,0,0,255,150,150);
			else
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,0,0,255,255,255);
		}
	}
}

bool	CEffectPrim :: PrimCloud()
{

	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;



	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if ( m_GI[ec].life == 1 )
		{
			if ( m_GI[ec].height[7]<99 )
			{
				int i;

				m_GI[ec].process++;

				{
					if ( m_GI[ec].height[7] == 0)
					{
						if (m_GI[ec].process < 80)
						{
							if (m_pos.y < m_GI[ec].height[6])
								m_GI[ec].alphaB+=2;
						}
					}
					else if ( m_GI[ec].height[7] == 2)
					{
						if (m_GI[ec].process < 80)
							m_GI[ec].alphaB+=2;
					}
					else if ( m_GI[ec].height[7] == 3)
					{
						if (m_GI[ec].process < 170)
							m_GI[ec].alphaB++;
					}
					else if ( m_GI[ec].height[7] == 5)
					{
						if (m_GI[ec].process < 80)
							m_GI[ec].alphaB+=2;
					}
					else
					{
						if (m_GI[ec].process < 80)
							m_GI[ec].alphaB+=3;
					}

				}

				if ( (m_GI[ec].process > m_GI[ec].RotStart) )
				{
					m_GI[ec].alphaB--;

					if (m_GI[ec].alphaB <= 0)
					{


						m_GI[ec].process = 0;
						m_GI[ec].alphaB = 0;

						if ( m_GI[ec].height[7] == 0 )
						{
							if ( m_pos.y > m_GI[ec].height[6] )
								m_GI[ec].process=300;
						}
						else if ( m_GI[ec].height[7] == 2 )
						{
							m_GI[ec].vecB_now.x=random(300)-150;
							m_GI[ec].vecB_now.z=random(300)-150;
						}
						else if ( m_GI[ec].height[7] == 5 )
						{
							m_GI[ec].vecB_now.x=random(300)-150;
							m_GI[ec].vecB_now.z=random(300)-150;
						}
						else if ( m_GI[ec].height[7] == 3 )
						{
							{
								m_GI[ec].vecB_now.x=random(300)-150;
								m_GI[ec].vecB_now.z=random(300)-150;

								CWorld *	world = g_modeMgr.GetGameMode()->GetWorld();
								m_GI[ec].vecB_now.y = world->m_attr->GetHeight(m_pos.x+m_GI[ec].vecB_now.x, m_pos.z+m_GI[ec].vecB_now.z) - 20 - random(5);
							}
						}
						else
						{
							m_GI[ec].vecB_now.x=random(200)+25;
							if (random(2)==0)
								m_GI[ec].vecB_now.x*=-1;

							m_GI[ec].vecB_now.z=random(200)+25;
							if (random(2)==0)
								m_GI[ec].vecB_now.z*=-1;
						}

						m_GI[ec].vecB_now.x+=m_pos.x;
						m_GI[ec].vecB_now.z+=m_pos.z;
					}
				}

				for (i=0;i<4;i+=2)
				{
					if (m_GI[ec].height[i+1] > m_GI[ec].height[i])
					{
						m_GI[ec].height[i]+=1;
						if (m_GI[ec].height[i] > 359)
							m_GI[ec].height[i] = 359;
						if (m_GI[ec].height[i] > m_GI[ec].height[i+1])
							m_GI[ec].height[i+1] = random(int(m_GI[ec].height[i]));
					}
					else
					{
						m_GI[ec].height[i]-=1;
						if (m_GI[ec].height[i] < 0)
							m_GI[ec].height[i] = 0;
						if (m_GI[ec].height[i] <= m_GI[ec].height[i+1])
							m_GI[ec].height[i+1] = 359 - random(int(m_GI[ec].height[i]));
					}
				}

				if ( m_GI[ec].height[7] == 3 )
				{
					m_GI[ec].vecB_now.x += 0.015f * g_sinTable[int(m_GI[ec].height[0])];
					m_GI[ec].vecB_now.z += 0.015f * g_sinTable[int(m_GI[ec].height[2])];
				}
				else if ( m_GI[ec].height[7] == 4 )
				{
					if ( g_sinTable[int(m_GI[ec].height[0])] > 0 )
						m_GI[ec].vecB_now.x += 0.20f * g_sinTable[int(m_GI[ec].height[0])];
					else
						m_GI[ec].vecB_now.x -= 0.20f * g_sinTable[int(m_GI[ec].height[0])];

					m_GI[ec].vecB_now.z += 0.05f * g_sinTable[int(m_GI[ec].height[2])];
				}
				else if ( m_GI[ec].height[7] == 5)
				{
					m_GI[ec].vecB_now.x += 0.035f * g_sinTable[int(m_GI[ec].height[0])];
					m_GI[ec].vecB_now.z += 0.035f * g_sinTable[int(m_GI[ec].height[2])];
				}
				else
				{
					m_GI[ec].vecB_now.x += 0.05f * g_sinTable[int(m_GI[ec].height[0])];
					m_GI[ec].vecB_now.z += 0.05f * g_sinTable[int(m_GI[ec].height[2])];
				}

			}

			m_GI[ec].height[8]+=1;
			if (m_GI[ec].height[8]>=360)
				m_GI[ec].height[8]-=360;
		}
	}

	return true;
}

bool	CEffectPrim :: PrimTwilight()
{

	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;



	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if ( m_GI[ec].life == 1 )
		{
			{
				int i;

				m_GI[ec].process++;

				if (m_GI[ec].process < 50)
					m_GI[ec].alphaB+=5;

				if ( (m_GI[ec].process > m_GI[ec].RotStart) )
				{
					m_GI[ec].alphaB-=5;
				}

				for (i=0;i<4;i+=2)
				{
					if (m_GI[ec].height[i+1] > m_GI[ec].height[i])
					{
						m_GI[ec].height[i]+=1;
						if (m_GI[ec].height[i] > 359)
							m_GI[ec].height[i] = 359;
						if (m_GI[ec].height[i] > m_GI[ec].height[i+1])
							m_GI[ec].height[i+1] = random(int(m_GI[ec].height[i]));
					}
					else
					{
						m_GI[ec].height[i]-=1;
						if (m_GI[ec].height[i] < 0)
							m_GI[ec].height[i] = 0;
						if (m_GI[ec].height[i] <= m_GI[ec].height[i+1])
							m_GI[ec].height[i+1] = 359 - random(int(m_GI[ec].height[i]));
					}
				}

				{
					m_GI[ec].vecB_now.x += 0.1f * g_sinTable[int(m_GI[ec].height[0])];
					m_GI[ec].vecB_now.z += 0.1f * g_sinTable[int(m_GI[ec].height[2])];
				}

			}

			m_GI[ec].height[8]+=1;
			if (m_GI[ec].height[8]>=360)
				m_GI[ec].height[8]-=360;

			m_GI[ec].full_display_angle++;
			if (m_GI[ec].full_display_angle>=360)
				m_GI[ec].full_display_angle-=360;
		}
	}

	return true;
}

void	CEffectPrim :: PrimPokJuk()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;



	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if ( m_GI[ec].life == 1 )
		{
			m_GI[ec].process++;

			if (m_GI[ec].process>0)
			{
				if (m_GI[ec].process%3==0)
				{
					m_GI[ec].vecT_pre = m_GI[ec].vecT_now;
					m_GI[ec].vecT_now = m_GI[ec].vecB_pre;
					m_GI[ec].vecB_pre = m_GI[ec].vecB_now;
				}


				if (m_GI[ec].process<=160+20+m_GI[ec].flag1[2] && m_GI[ec].process>= 160+m_GI[ec].flag1[2])
				{
					char per;

					per=m_GI[ec].process%20;

					if (per == 15 || per == 19 )
						m_GI[ec].flag1[3] = 1;
					else if (per == 17)
						m_GI[ec].flag1[3] = 2;
					else
						m_GI[ec].flag1[3] = 0;


				}
				else
					m_GI[ec].flag1[3] = 0;

				m_GI[ec].RotStart+=5;
				if (m_GI[ec].RotStart>=360)
					m_GI[ec].RotStart-=360;

				if ( m_GI[ec].flag1[1] == 0 )
				{
					m_GI[ec].vecB_now.y-=0.25f;
					if (m_GI[ec].process>154)
					{
						m_GI[ec].flag1[1] = 1;
						m_GI[ec].alphaB = 250;
					}
				}
				else if (m_GI[ec].flag1[1] == 1)
				{
					float Rx,cs1,sn1;

					sn1 = g_sinTable[m_GI[ec].full_display_angle];
					cs1 = g_cosTable[m_GI[ec].full_display_angle];


					m_GI[ec].vecB_now.y+=(sn1*m_GI[ec].distance);


					Rx=(cs1*m_GI[ec].distance);

					cs1 = g_cosTable[m_GI[ec].rise_angle];
					sn1 = g_sinTable[m_GI[ec].rise_angle];

					m_GI[ec].vecB_now.y+=0.02f;

					m_GI[ec].vecB_now.x+=(cs1*Rx);
					m_GI[ec].vecB_now.z+=(sn1*Rx);

					m_GI[ec].distance*=0.98f;
					m_GI[ec].alphaB-=1;
					if (random(2)==0)
						m_GI[ec].alphaB-=1;

					if (m_GI[ec].alphaB<0)
						m_GI[ec].alphaB=0;
				}


				if (m_GI[ec].process>2000)
				{
					m_GI[ec].process=0;
					m_GI[ec].flag1[1] = 0;
					m_GI[ec].rise_angle = random(360);
					m_GI[ec].vecB_now=m_pos;
					m_GI[ec].vecB_now.x-=15.0f;
					m_GI[ec].distance=0.4f+(random(100)*0.004f);
				}
			}
		}
	}
}

void	CEffectPrim :: PrimBottom()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;



	int ec,i;

	m_pos = m_master->m_pos;

	for (ec=0;ec<1;ec++)
	{
		if ( m_GI[ec].life == 1 )
		{
			m_GI[ec].process++;

			if (m_GI[ec].process%6==0)
			{
				for (i=3;i>0;i--)
				{
					if (m_GI[i].life>0)
					{
						m_GI[i].full_display_angle=m_GI[i-1].full_display_angle;
						m_GI[i].distance = m_GI[i-1].distance+0.5f;
						m_GI[i].height[8] = m_GI[i-1].height[8];
						m_GI[i].alphaB = m_GI[i-1].alphaB - 50;
						if (m_GI[i].alphaB < 0)
							m_GI[i].alphaB = 0;

						m_GI[i].vecB_now=m_GI[i-1].vecB_now;

						m_GI[i].full_display_angle = m_GI[i-1].full_display_angle;
						m_GI[i].flag1[0] = m_GI[i-1].flag1[0];
						m_GI[i].flag1[2] = 2;
						m_GI[i].height[7] = 0;
					}
				}
			}

			m_GI[ec].height[8]+=5;
			if (m_GI[ec].height[8]>=360)
				m_GI[ec].height[8]-=360;

			m_GI[ec].RotStart+=1;
			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			m_GI[ec].vecB_now.x=m_pos.x;
			m_GI[ec].vecB_now.z=m_pos.z;
			m_GI[ec].vecB_pre.y=m_pos.y-6.0f;

			m_GI[ec].vecB_now.y = m_GI[ec].vecB_pre.y + (3.0f * g_sinTable[m_GI[ec].RotStart]);

			if (m_GI[ec].flag1[2] == 7)
			{
				m_GI[ec].full_display_angle+=10;
				if (m_GI[ec].full_display_angle>=360)
					m_GI[ec].full_display_angle-=360;
			}

		}
	}
}

void	CEffectPrim :: PrimEffectTexture()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;



	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<1;ec++)
	{
		if ( m_GI[ec].life == 1 )
		{
			m_GI[ec].process++;

			if (m_GI[ec].process<300)
			{
				if (m_GI[ec].flag1[4] == 1 || m_GI[ec].flag1[4] == 3)
				{
					if (m_GI[ec].process<200)
					{
						m_GI[ec].alphaB+=10;
						if (m_GI[ec].alphaB>=220)
							m_GI[ec].alphaB=220;
					}
					else
					{
						m_GI[ec].alphaB-=5;
						if (m_GI[ec].alphaB<0)
							m_GI[ec].alphaB=0;
					}
				}
				else if (m_GI[ec].flag1[4] == 100)
					m_GI[ec].alphaB=255;
				else if (m_GI[ec].flag1[4] == 0)
					m_GI[ec].alphaB++;
				else if (m_GI[ec].flag1[4] >= 10 && m_GI[ec].flag1[4] <= 12)
					m_GI[ec].alphaB=150;
				else if (m_GI[ec].flag1[4] == 14)
					m_GI[ec].alphaB=50;
				else if (m_GI[ec].flag1[4] >= 10)
					m_GI[ec].alphaB=250;

			}

			if ( m_GI[ec].flag1[4] == 2 )
			{
				if (m_GI[ec].process>40)
				{
					if (m_GI[ec].process<62)
						m_GI[ec].alphaB+=8;
					else
						m_GI[ec].alphaB-=8;

					if (m_GI[ec].alphaB>0)
						m_GI[ec].distance+=0.7f;
				}
			}

			if (m_GI[ec].flag1[4] == 100)
			{
			}
			else if (m_GI[ec].flag1[4] == 10 || m_GI[ec].flag1[4] >= 13)
			{
				m_GI[ec].full_display_angle++;
				if (m_GI[ec].full_display_angle>=360)
					m_GI[ec].full_display_angle-=360;
			}
			else if (m_GI[ec].flag1[4] == 11 || m_GI[ec].flag1[4] == 12)
			{
				m_GI[ec].full_display_angle--;
				if (m_GI[ec].full_display_angle<0)
					m_GI[ec].full_display_angle+=360;
			}
			else
			{
				if (m_GI[ec].process>300)
				{
					if (m_GI[ec].alphaB>0)
						m_GI[ec].alphaB--;
				}
			}

			if (m_GI[ec].alphaB>=255)
				m_GI[ec].alphaB=255;
			if (m_GI[ec].alphaB<0)
				m_GI[ec].alphaB=0;

			if (m_GI[ec].flag1[4] != 100)
			{
				m_GI[ec].height[8]+=2;
				if (m_GI[ec].height[8]>=360)
					m_GI[ec].height[8]-=360;
			}

			m_GI[ec].vecB_now.y = m_GI[ec].vecB_pre.y;
			if (m_GI[ec].flag1[4] < 100)
			{
				m_GI[ec].vecB_now.x = m_pos.x;
				m_GI[ec].vecB_now.z = m_pos.z;
			}
		}
	}
}

void	CEffectPrim :: PrimEffectTexture_Ani()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;



	int ec,tcount;

	m_pos = m_master->m_pos;

	for (ec=0;ec<1;ec++)
	{
		if ( m_GI[ec].life == 1 )
		{
			m_GI[ec].process++;

			if (m_GI[ec].flag1[4] == 0)
				tcount=8;
			else
				tcount=6;

			if (m_GI[ec].process%tcount == 0)
			{
				m_GI[ec].flag1[0]++;
				if (m_GI[ec].flag1[0] >= m_totalTexture)
					m_GI[ec].flag1[0]=0;
			}

			if (m_GI[ec].alphaB>=255)
				m_GI[ec].alphaB=255;
			if (m_GI[ec].alphaB<0)
				m_GI[ec].alphaB=0;

			m_GI[ec].vecB_now.y = m_GI[ec].vecB_pre.y;
			m_GI[ec].vecB_now.x = m_pos.x;
			m_GI[ec].vecB_now.z = m_pos.z;
		}
	}
}

void	CEffectPrim :: PrimEffectTexture2()
{
	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;



	int ec;

	for (ec=0;ec<1;ec++)
	{
		if ( m_GI[ec].life == 1 )
		{
			m_GI[ec].process++;


			if (m_GI[ec].height[8]>=360)
				m_GI[ec].height[8]-=360;

			if (m_GI[ec].process<=30)
			{
				m_GI[ec].alphaB+=10;
				if (m_GI[ec].alphaB>255)
					m_GI[ec].alphaB=255;
			}
			else
			{
				m_GI[ec].alphaB-=3;
				if (m_GI[ec].alphaB<0)
					m_GI[ec].alphaB=0;
			}

			m_GI[ec].vecB_now.y = m_GI[ec].vecB_pre.y;
		}
	}
}

void	CEffectPrim :: PrimBottom2()
{

	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if ( m_GI[ec].life == 1 )
		{
			m_GI[ec].process++;

			if (m_GI[ec].height[0] == 1)
			{
				m_GI[ec].full_display_angle += 4;
				if (m_GI[ec].full_display_angle>=360)
					m_GI[ec].full_display_angle -= 360;

				m_GI[ec].max_height = 20.0f + (10.0f * g_sinTable[m_GI[ec].full_display_angle]);
			}
			else
			{
				m_GI[ec].full_display_angle += 2;
				if (m_GI[ec].full_display_angle>=360)
					m_GI[ec].full_display_angle -= 360;

				m_GI[ec].max_height = 20.0f + (10.0f * g_sinTable[m_GI[ec].full_display_angle]);
			}
		}
	}
}

void	CEffectPrim :: PrimBottomOut()
{

	CView * pView = g_modeMgr.GetGameMode()->GetView();

	m_GI[0].alphaT=pView->GetCurLongitude();
	m_GI[0].alphaT=360-m_GI[0].alphaT;

	m_GI[1].alphaT=pView->GetCurLatitude();
	m_GI[1].alphaT=360-m_GI[1].alphaT+90;
	if (m_GI[1].alphaT>=360)
		m_GI[1].alphaT-=360;



	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if ( m_GI[ec].life == 1 )
		{
			m_GI[ec].process++;

			if (m_GI[ec].process<=10)
			{
				if ( m_GI[ec].flag1[1] == 0)
					m_GI[ec].alphaB+=25;
				else
					m_GI[ec].alphaB+=25;
			}
			else
			{
				m_GI[ec].alphaB-=5;
				if (m_GI[ec].alphaB<=0)
				{
					m_GI[ec].alphaB=0;
					m_GI[ec].process=0;
					if (m_GI[ec].flag1[1] == 0)
						m_GI[ec].vecB_now.y=m_pos.y;
					else
						m_GI[ec].vecB_now.y=m_pos.y-5.0f;
				}

				m_GI[ec].vecB_now.y-=0.2f;
			}

			m_GI[ec].height[8]+=5;
			if (m_GI[ec].height[8]>=360)
				m_GI[ec].height[8]-=360;

			if ( m_GI[ec].flag1[1] == 1)
			{
				m_GI[ec].full_display_angle+=5;
				if (m_GI[ec].full_display_angle>=360)
					m_GI[ec].full_display_angle-=360;
			}
		}
	}
}

void	CEffectPrim :: RenderCloud(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int Angle;
	float Rx;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			Angle=m_GI[ec].full_display_angle;

			Rx=m_GI[ec].distance;

			if ( m_GI[ec].height[8]>=360 || m_GI[ec].height[8]<0 )
				m_GI[ec].height[8]=0;

			Rx+=g_sinTable[int(m_GI[ec].height[8])]*m_GI[ec].distance*0.05f;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec1.x=cs1*Rx;
			vec1.z=sn1*Rx;
			vec1.y=0;

			Angle+=90;
			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec2.x=cs1*Rx;
			vec2.z=sn1*Rx;
			vec2.y=0;

			Angle+=90;
			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec3.x=cs1*Rx;
			vec3.z=sn1*Rx;
			vec3.y=0;

			Angle+=90;
			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec4.x=cs1*Rx;
			vec4.z=sn1*Rx;
			vec4.y=0;

			if (m_GI[ec].height[9] == 10)
			{
				vec1.x+=4.0f;
				vec2.x+=4.0f;
				vec3.x+=4.0f;
				vec4.x+=4.0f;
			}

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			vec1+=m_GI[ec].vecB_now;
			vec2+=m_GI[ec].vecB_now;
			vec3+=m_GI[ec].vecB_now;
			vec4+=m_GI[ec].vecB_now;


			switch (m_GI[0].flag1[2])
			{
			case 0:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,m_GI[ec].flag1[0],1);
				break;
			case 1:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,m_GI[ec].flag1[0],1,150,150,250);
				break;
			case 2:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,0,m_GI[ec].flag1[0],1,130,130,250);
				break;
			case 3:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,0,m_GI[ec].flag1[0],1,200,200,100);
				break;
			case 4:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],1,255,255,255);
				break;
			case 7:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,0,m_GI[ec].flag1[0],1,255,255,255);
				break;
			case 8:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],1,135,255,135);
				break;
			case 9:
					if ( m_GI[0].flag1[3] == 1)
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],1,255,135,135);
					else if ( m_GI[0].flag1[3] == 2)
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],1,255,205,135);
					else if ( m_GI[0].flag1[3] == 3)
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],1,255,255,135);
					else if ( m_GI[0].flag1[3] == 4)
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],1,255,255,255);
					else if ( m_GI[0].flag1[3] == 5)
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],1,135,135,255);
					else if ( m_GI[0].flag1[3] == 6)
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,0,m_GI[ec].flag1[0],1,255,255,255);
				break;
			case 10:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,0,m_GI[ec].flag1[0],1,250,250,150);
				break;
			case 11:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,0,m_GI[ec].flag1[0],1,220,250,220);
				break;
			case 12:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],1,252,171,143);
				break;
			case 13:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,0,m_GI[ec].flag1[0],1,250,100,100);
				break;
			case 14:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],1,94,0,0);
				break;
			case 15:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,m_GI[ec].flag1[0],1,255,200,255);
				break;
			case 16:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],1,0,0,0);
				break;
			case 17:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],1,255,180,180);
				break;
			}
		}
	}
}

void	CEffectPrim :: RenderEffectTexture(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int Angle;
	float Rx;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			Angle=m_GI[ec].full_display_angle;

			Rx=m_GI[ec].distance;
			Rx+=g_sinTable[int(m_GI[ec].height[8])]*m_GI[ec].distance*0.05f;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec1.x=cs1*Rx;
			vec1.z=sn1*Rx;
			vec1.y=0;

			if (m_GI[ec].height[10] == 0)
			{
				Angle-=90;
				if (Angle<0)
					Angle+=360;
			}
			else if (m_GI[ec].height[10] == 1)
			{
				Angle-=60;
				if (Angle<0)
					Angle+=360;
			}
			else if (m_GI[ec].height[10] == 2)
			{
				Angle-=130;
				if (Angle<0)
					Angle+=360;
			}

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec2.x=cs1*Rx;
			vec2.z=sn1*Rx;
			vec2.y=0;

			if (m_GI[ec].height[10] == 0)
			{
				Angle-=90;
				if (Angle<0)
					Angle+=360;
			}
			else if (m_GI[ec].height[10] == 1)
			{
				Angle-=120;
				if (Angle<0)
					Angle+=360;
			}
			else if (m_GI[ec].height[10] == 2)
			{
				Angle-=50;
				if (Angle<0)
					Angle+=360;
			}

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec3.x=cs1*Rx;
			vec3.z=sn1*Rx;
			vec3.y=0;

			if (m_GI[ec].height[10] == 0)
			{
				Angle-=90;
				if (Angle<0)
					Angle+=360;
			}
			else if (m_GI[ec].height[10] == 1)
			{
				Angle-=60;
				if (Angle<0)
					Angle+=360;
			}
			else if (m_GI[ec].height[10] == 2)
			{
				Angle-=130;
				if (Angle<0)
					Angle+=360;
			}

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec4.x=cs1*Rx;
			vec4.z=sn1*Rx;
			vec4.y=0;

			if (m_GI[0].flag1[2] == 3)
			{
				vec1.x+=3.1f;
				vec2.x+=3.1f;
				vec3.x+=3.1f;
				vec4.x+=3.1f;

				vec1.z+=6.0f;
				vec2.z+=6.0f;
				vec3.z+=6.0f;
				vec4.z+=6.0f;
			}

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			vec1+=m_GI[ec].vecB_now;
			vec2+=m_GI[ec].vecB_now;
			vec3+=m_GI[ec].vecB_now;
			vec4+=m_GI[ec].vecB_now;


			switch (m_GI[0].flag1[2])
			{
			case 0:
			case 3:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,m_GI[ec].flag1[0],1);
				break;
			case 2:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,0,m_GI[ec].flag1[0],1,130,130,250);
			case 4:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,0,m_GI[ec].flag1[0],1);
			case 10:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,0,m_GI[ec].flag1[0],1,150,150,150);
			case 12:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,0,m_GI[ec].flag1[0],1,50,50,50);
				break;
			case 100:
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec1,vec4,vec3,wtm,1,m_GI[ec].flag1[0],1,50,50,155);
				break;
			}
		}
	}
}

void	CEffectPrim :: RenderChemical(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int Angle;
	int ec;






	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			vec4.x=0;
			vec4.z=0;
			vec4.y=0;

			vec1.x=0;
			vec1.z=0;
			vec1.y=0;

			Angle = m_GI[ec].rise_angle - 3;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec2.x=cs1*m_GI[ec].distance;
			vec2.z=sn1*m_GI[ec].distance;
			vec2.y=0;

			Angle = m_GI[ec].rise_angle + 3;
			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec3.x=cs1*m_GI[ec].distance;
			vec3.z=sn1*m_GI[ec].distance;
			vec3.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			Angle = m_GI[ec].alphaB * 0.5f;

			RenderTeiRect(&rpCnt, Angle, vec1,vec2,vec3,vec4,wtm,0,0,0,255,255,255);




			vec4.x=0;
			vec4.z=0;
			vec4.y=0;

			vec1.x=0;
			vec1.z=0;
			vec1.y=0;

			Angle = m_GI[ec].rise_angle - 5;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec2.x=cs1*m_GI[ec].distance;
			vec2.z=sn1*m_GI[ec].distance;
			vec2.y=0;

			Angle = m_GI[ec].rise_angle + 5;
			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec3.x=cs1*m_GI[ec].distance;
			vec3.z=sn1*m_GI[ec].distance;
			vec3.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,0,0,0,130,130,255);

		}
	}
}

void	CEffectPrim :: RenderBash3D(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int Angle;
	int ec;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	float		dist;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{


			vec4.x=0;
			vec4.z=0;
			vec4.y=m_GI[ec].height[0];

			vec1.x=0;
			vec1.z=0;
			vec1.y=m_GI[ec].height[0];

			if (m_GI[ec].height[4] == 2)
				Angle = m_GI[ec].rise_angle - 1;
			else
				Angle = m_GI[ec].rise_angle - 2;

			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec2.x=cs1*m_GI[ec].distance;
			vec2.z=sn1*m_GI[ec].distance;

			dist = vec2.x;
			cs1 = g_cosTable[m_GI[ec].RotStart];
			sn1 = g_sinTable[m_GI[ec].RotStart];
			vec2.x=cs1*dist;
			vec2.y=sn1*dist;
			vec2.y+=m_GI[ec].height[0];



			if (m_GI[ec].height[4] == 2)
				Angle = m_GI[ec].rise_angle + 1;
			else
				Angle = m_GI[ec].rise_angle + 2;

			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec3.x=cs1*m_GI[ec].distance;
			vec3.z=sn1*m_GI[ec].distance;

			dist = vec3.x;
			cs1 = g_cosTable[m_GI[ec].RotStart];
			sn1 = g_sinTable[m_GI[ec].RotStart];
			vec3.x=cs1*dist;
			vec3.y=sn1*dist;
			vec3.y+=m_GI[ec].height[0];

			if (m_GI[ec].height[4] == 2)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,0,0,0,0,250);
			else if (m_GI[ec].height[4] == 3)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,0,0,250,250,250);
			else if (m_GI[ec].height[4] == 4)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,0,0,0,0,250);
			else if (m_GI[ec].height[4] == 5)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,0,0,50,50,50);
			else
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,0,0,0,250,250);




			vec4.x=0;
			vec4.z=0;
			vec4.y=m_GI[ec].height[0];

			vec1.x=0;
			vec1.z=0;
			vec1.y=m_GI[ec].height[0];

			if (m_GI[ec].height[4] == 2)
				Angle = m_GI[ec].rise_angle - 2;
			else
				Angle = m_GI[ec].rise_angle - 5;

			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec2.x=cs1*m_GI[ec].distance;
			vec2.z=sn1*m_GI[ec].distance;

			dist = vec2.x;
			cs1 = g_cosTable[m_GI[ec].RotStart];
			sn1 = g_sinTable[m_GI[ec].RotStart];
			vec2.x=cs1*dist;
			vec2.y=sn1*dist;
			vec2.y+=m_GI[ec].height[0];

			if (m_GI[ec].height[4] == 2)
				Angle = m_GI[ec].rise_angle + 2;
			else
				Angle = m_GI[ec].rise_angle + 5;

			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec3.x=cs1*m_GI[ec].distance;
			vec3.z=sn1*m_GI[ec].distance;

			dist = vec3.x;
			cs1 = g_cosTable[m_GI[ec].RotStart];
			sn1 = g_sinTable[m_GI[ec].RotStart];
			vec3.x=cs1*dist;
			vec3.y=sn1*dist;
			vec3.y+=m_GI[ec].height[0];

			if (m_GI[ec].height[4] == 2)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,0,0,250,250,0);
			else if (m_GI[ec].height[4] == 3)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,1,0,0,250,250,250);
			else if (m_GI[ec].height[4] == 4)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,1,0,0,250,250,0);
			else if (m_GI[ec].height[4] == 5)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,1,0,0,250,250,250);
			else
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec2,vec3,vec4,vec1,wtm,0,0,0,250,0,0);
		}
	}
}

void	CEffectPrim :: RenderChemical2(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4, vecT , vecB;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int ec;
	int Angle;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			Angle = m_GI[ec].rise_angle - 90;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vecB.x=cs1*m_GI[ec].height[0];
			vecB.z=sn1*m_GI[ec].height[0];
			vecB.y=0;



			Angle = m_GI[ec].rise_angle - 90;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vecT.x=cs1*m_GI[ec].max_height;
			vecT.z=sn1*m_GI[ec].max_height;
			vecT.y=0;

			vecT+=vecB;


			Angle = m_GI[ec].rise_angle;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec4.x=cs1*m_GI[ec].distance;
			vec4.z=sn1*m_GI[ec].distance;
			vec4.y=0;

			vec4+=vecT;



			Angle = m_GI[ec].rise_angle - 180;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec3.x=cs1*m_GI[ec].distance;
			vec3.z=sn1*m_GI[ec].distance;
			vec3.y=0;

			vec3+=vecT;



			Angle = m_GI[ec].rise_angle + 90;
			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vecT.x=cs1*m_GI[ec].max_height;
			vecT.z=sn1*m_GI[ec].max_height;
			vecT.y=0;

			vecT+=vecB;



			Angle = m_GI[ec].rise_angle;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec1.x=cs1*m_GI[ec].distance;
			vec1.z=sn1*m_GI[ec].distance;
			vec1.y=0;

			vec1+=vecT;



			Angle = m_GI[ec].rise_angle - 180;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vec2.y=0;
			vec2.x=cs1*m_GI[ec].distance;
			vec2.z=sn1*m_GI[ec].distance;

			vec2+=vecT;



			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,255,255,255);
		}
	}
}

#include "rag_symbol_shift.h"
void	CEffectPrim :: RenderGroundSample(matrix &wtm)
{
	RAG_SYMBOL_SHIFT(8)
	int attrX;
	int attrY;

	for (int ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			CGameMode * gameMode = g_modeMgr.GetGameMode();
			C3dGround* ground = g_modeMgr.GetGameMode()->GetWorld()->m_ground;

			gameMode->GetServerCoor(m_pos.x,m_pos.z,attrX,attrY);
			attrX -= 3;
			attrY -= 3;

			vector2d uv[4];

			float half_scope_size = 3.5f;
			float sin_r = g_sinTable[m_GI[ec].RotStart];
			float cos_r = g_cosTable[m_GI[ec].RotStart];

			DWORD color;
			for(int i=0; i<7; ++i)
			{
				for(int j=0; j<7; ++j)
				{
					int x = attrX+j;
					int y = attrY+i;

					float u0 = j+0-half_scope_size;
					float v0 = i+0-half_scope_size;
					float u1 = j+1-half_scope_size;
					float v1 = i+1-half_scope_size;

					uv[0].Set(u0*cos_r + v0*sin_r, -u0*sin_r + v0*cos_r);
					uv[1].Set(u1*cos_r + v0*sin_r, -u1*sin_r + v0*cos_r);
					uv[2].Set(u0*cos_r + v1*sin_r, -u0*sin_r + v1*cos_r);
					uv[3].Set(u1*cos_r + v1*sin_r, -u1*sin_r + v1*cos_r);

				int a;
				for (a=0;a<4;a++)
					uv[a].Set(uv[a].x*m_GI[ec].max_height,uv[a].y*m_GI[ec].max_height);

					uv[0].Set((uv[0].x+half_scope_size)/7, (uv[0].y+half_scope_size)/7);
					uv[1].Set((uv[1].x+half_scope_size)/7, (uv[1].y+half_scope_size)/7);
					uv[2].Set((uv[2].x+half_scope_size)/7, (uv[2].y+half_scope_size)/7);
					uv[3].Set((uv[3].x+half_scope_size)/7, (uv[3].y+half_scope_size)/7);

					color = (DWORD)0xAA9999FF;
					ground->RenderGroundTile(wtm, x, y, color, g_texMgr.GetTexture("effect\\magic_target.tga", 0), uv);


					uv[0].Set(u0*cos_r + v0*sin_r, -u0*sin_r + v0*cos_r);
					uv[1].Set(u1*cos_r + v0*sin_r, -u1*sin_r + v0*cos_r);
					uv[2].Set(u0*cos_r + v1*sin_r, -u0*sin_r + v1*cos_r);
					uv[3].Set(u1*cos_r + v1*sin_r, -u1*sin_r + v1*cos_r);

					for (a=0;a<4;a++)
						uv[a].Set(uv[a].x*(m_GI[ec].max_height+0.1f),uv[a].y*(m_GI[ec].max_height+0.1f));

					uv[0].Set((uv[0].x+half_scope_size)/7, (uv[0].y+half_scope_size)/7);
					uv[1].Set((uv[1].x+half_scope_size)/7, (uv[1].y+half_scope_size)/7);
					uv[2].Set((uv[2].x+half_scope_size)/7, (uv[2].y+half_scope_size)/7);
					uv[3].Set((uv[3].x+half_scope_size)/7, (uv[3].y+half_scope_size)/7);

					color = (DWORD)0x559999FF;
					ground->RenderGroundTile(wtm, x, y, color, g_texMgr.GetTexture("effect\\magic_target.tga", 0), uv);
				}
			}
		}
	}
}

void	CEffectPrim :: RenderChemical3(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4, vecT , vecB;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	float		Dist;
	float		height1;
	int Angle;
	int ec;
	float		yAdd;

	yAdd = -6.0f;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{

			Angle = m_GI[ec].rise_angle - 90;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vecB.x=cs1*m_GI[ec].height[0];
			vecB.z=sn1*m_GI[ec].height[0];
			vecB.y=0;



			Angle = m_GI[ec].rise_angle - 90;
			if (Angle<0)
				Angle+=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vecT.x=cs1*m_GI[ec].max_height;
			vecT.z=sn1*m_GI[ec].max_height;
			vecT.y=yAdd;

			vecT+=vecB;


			height1 = m_GI[ec].height[1];
			if (height1 < 100)
			{
				Angle = m_GI[ec].rise_angle - 180;
				if (Angle<0)
					Angle+=360;
				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				Dist = m_GI[ec].distance-height1;
			}
			else
			{
				Angle = m_GI[ec].rise_angle;
				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				Dist = height1 - m_GI[ec].distance;
			}

			vec3.x=cs1*Dist;
			vec3.z=sn1*Dist;
			vec3.y=0;

			vec2 = vec3;

			vec3+=vecT;


			height1 = m_GI[ec].height[1]+m_GI[ec].height[2];

			if (height1 < 100)
			{
				Angle = m_GI[ec].rise_angle - 180;
				if (Angle<0)
					Angle+=360;
				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				Dist = m_GI[ec].distance-height1;
			}
			else
			{
				Angle = m_GI[ec].rise_angle;
				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				Dist = height1 - m_GI[ec].distance;
			}

			vec4.x=cs1*Dist;
			vec4.z=sn1*Dist;
			vec4.y=0;

			vec1 = vec4;

			vec4+=vecT;



			Angle = m_GI[ec].rise_angle + 90;
			if (Angle>=360)
				Angle-=360;

			cs1 = g_cosTable[Angle];
			sn1 = g_sinTable[Angle];
			vecT.x=cs1*m_GI[ec].max_height;
			vecT.z=sn1*m_GI[ec].max_height;
			vecT.y=yAdd;

			vecT+=vecB;



			vec2 += vecT;
			vec1 += vecT;



			if (m_GI[ec].flag1[0] == 0)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,225,225,255);
			else if (m_GI[ec].flag1[0] == 1)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,205,205,255);
			else if (m_GI[ec].flag1[0] == 2)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,185,185,255);
			else if (m_GI[ec].flag1[0] == 10)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,255,255,225);
			else if (m_GI[ec].flag1[0] == 11)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,255,255,205);
			else if (m_GI[ec].flag1[0] == 12)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,255,255,185);
			else if (m_GI[ec].flag1[0] == 20)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,225,255,225);
			else if (m_GI[ec].flag1[0] == 21)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,205,255,205);
			else if (m_GI[ec].flag1[0] == 22)
				RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,185,255,185);



		}
	}
}

void	CEffectPrim :: RenderCallZone(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int order;
	int count,countY;
	int Angle;
	int BasicAngle;
	int YAngle;
	float Rx,Ry,Rz;
	int ec;







	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1)
		{
			BasicAngle = m_GI[ec].full_display_angle;
			BasicAngle /= 10;

			YAngle = 360;
			YAngle /= 10;

			order=0;

			for (countY = 0; countY<360 ; countY+= YAngle)
			{
				for (count = 0 ; count<=m_GI[ec].full_display_angle ; count+=BasicAngle)
				{
					Angle=count+m_GI[ec].RotStart;
					Angle%=360;

					cs1 = g_cosTable[Angle];
					sn1 = g_sinTable[Angle];


					m_GI[ec].vecB_now.x=cs1*m_GI[ec].distance;
					m_GI[ec].vecB_now.z=sn1*m_GI[ec].distance;
					m_GI[ec].vecB_now.y=-0;

					Rx=g_cosTable[m_GI[ec].rise_angle] * m_GI[ec].height[order];
					Ry=g_sinTable[m_GI[ec].rise_angle] * m_GI[ec].height[order];

					Rz=sn1*Rx;
					Rx=cs1*Rx;

					m_GI[ec].vecT_now.x=m_GI[ec].vecB_now.x+Rx;
					m_GI[ec].vecT_now.z=m_GI[ec].vecB_now.z+Rz;
					m_GI[ec].vecT_now.y=-(Ry);

					if (order > 0)
					{
						RenderTeiRect(&rpCnt, order-1, m_GI[ec].alphaB, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm);
					}

					order++;
					if (order>=E_DIVISION)
						order=E_DIVISION-1;

					m_GI[ec].vecB_pre=m_GI[ec].vecB_now;
					m_GI[ec].vecT_pre=m_GI[ec].vecT_now;
				}
			}
		}
	}
}

void	CEffectPrim :: RenderBlind(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4;
	int r,g,b,ec;

	r=10;
	int			rpCnt = 0;
	g=10;
	b=10;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
			if (m_GI[ec].flag1[0]==1)
			{
				r=30;
				g=30;
				b=30;
			}

			vec1.x=0;
			vec1.z=0;
			vec1.y=0;

			vec2.x=0;
			vec2.z=m_GI[ec].distance;
			vec2.y=0;

			vec3.x=m_GI[ec].distance;
			vec3.z=m_GI[ec].distance;
			vec3.y=0;

			vec4.x=m_GI[ec].distance;
			vec4.z=0;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,r,g,b);


			vec1.x=0;
			vec1.z=0;
			vec1.y=0;

			vec2.x=0;
			vec2.z=-m_GI[ec].distance;
			vec2.y=0;

			vec3.x=m_GI[ec].distance;
			vec3.z=-m_GI[ec].distance;
			vec3.y=0;

			vec4.x=m_GI[ec].distance;
			vec4.z=0;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,r,g,b);



			vec1.x=0;
			vec1.z=0;
			vec1.y=0;

			vec2.x=0;
			vec2.z=m_GI[ec].distance;
			vec2.y=0;

			vec3.x=-m_GI[ec].distance;
			vec3.z=m_GI[ec].distance;
			vec3.y=0;

			vec4.x=-m_GI[ec].distance;
			vec4.z=0;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,r,g,b);



			vec1.x=0;
			vec1.z=0;
			vec1.y=0;

			vec2.x=0;
			vec2.z=-m_GI[ec].distance;
			vec2.y=0;

			vec3.x=-m_GI[ec].distance;
			vec3.z=-m_GI[ec].distance;
			vec3.y=0;

			vec4.x=-m_GI[ec].distance;
			vec4.z=0;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,r,g,b);



			float offset_distance = 50 ;

			vec1.x=m_GI[ec].distance;
			vec1.z=-m_GI[ec].distance;
			vec1.y=0;

			vec2.x=m_GI[ec].distance+offset_distance;
			vec2.z=-m_GI[ec].distance;
			vec2.y=0;

			vec3.x=m_GI[ec].distance+offset_distance;
			vec3.z=m_GI[ec].distance+offset_distance;
			vec3.y=0;

			vec4.x=m_GI[ec].distance;
			vec4.z=m_GI[ec].distance+offset_distance;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,1,0,r,g,b);



			vec1.x=m_GI[ec].distance;
			vec1.z=m_GI[ec].distance;
			vec1.y=0;

			vec2.x=m_GI[ec].distance;
			vec2.z=m_GI[ec].distance+offset_distance;
			vec2.y=0;

			vec3.x=-(m_GI[ec].distance+offset_distance);
			vec3.z=m_GI[ec].distance+offset_distance;
			vec3.y=0;

			vec4.x=-(m_GI[ec].distance+offset_distance);
			vec4.z=m_GI[ec].distance;
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,1,0,r,g,b);



			vec1.x=-m_GI[ec].distance;
			vec1.z=m_GI[ec].distance;
			vec1.y=0;

			vec2.x=-(m_GI[ec].distance+offset_distance);
			vec2.z=m_GI[ec].distance;
			vec2.y=0;

			vec3.x=-(m_GI[ec].distance+offset_distance);
			vec3.z=-(m_GI[ec].distance+offset_distance);
			vec3.y=0;

			vec4.x=-m_GI[ec].distance;
			vec4.z=-(m_GI[ec].distance+offset_distance);
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,1,0,r,g,b);



			vec1.x=-(m_GI[ec].distance);
			vec1.z=-(m_GI[ec].distance);
			vec1.y=0;

			vec2.x=-(m_GI[ec].distance);
			vec2.z=-(m_GI[ec].distance+offset_distance);
			vec2.y=0;

			vec3.x=(m_GI[ec].distance+offset_distance);
			vec3.z=-(m_GI[ec].distance+offset_distance);
			vec3.y=0;

			vec4.x=(m_GI[ec].distance+offset_distance);
			vec4.z=-(m_GI[ec].distance);
			vec4.y=0;

			Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

			RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,1,0,r,g,b);
		}
	}
}

void	CEffectPrim :: RenderPoison(matrix &wtm)
{
	vector3d	vec1, vec2, vec3, vec4, vecT , vecB;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int ec;
	int x,z;

	for (ec=0 ; ec<4 ; ec++)
	{
		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{

			for (x=-120;x<90;x+=30)
			{
				for (z=-90;z<90;z+=30)
				{
					vec1.x=x+m_GI[ec].max_height;
					vec1.z=z+m_GI[ec].max_height;
					vec1.y=-10;

					vec2.x=x+m_GI[ec].distance+m_GI[ec].max_height;
					vec2.z=z+m_GI[ec].max_height;
					vec2.y=-10;

					vec3.x=x+m_GI[ec].distance+m_GI[ec].max_height;
					vec3.z=z+m_GI[ec].distance+m_GI[ec].max_height;
					vec3.y=-10;

					vec4.x=x+m_GI[ec].max_height;
					vec4.z=z+m_GI[ec].distance+m_GI[ec].max_height;
					vec4.y=-10;

					Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

					if (m_GI[ec].flag1[10] == 0)
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,255,255,255);
					else if (m_GI[ec].flag1[10] == 1)
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,255,0,0);
					else
						RenderTeiRect(&rpCnt, m_GI[ec].alphaB, vec1,vec2,vec3,vec4,wtm,1,0,0,0,0,255);
				}
			}

		}
	}
}

void	CEffectPrim :: RenderPokJuk(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int Angle;
	float Rx;
	int four;
	int alpha;
	int ec;
	unsigned char r,g,b;
	float minus;


	for (ec=0 ; ec<4 ; ec++)
	{
		minus = 0;

		if (m_GI[ec].flag1[0] == 0)
		{
				b=255;
				r=70;
				g=70;
		}
		else if (m_GI[ec].flag1[0] == 1)
		{
				r=255;
				g=70;
				b=70;
		}
		else if (m_GI[ec].flag1[0] == 2)
		{
				r=70;
				g=255;
				b=70;
		}
		else if (m_GI[ec].flag1[0] == 3)
		{
				r=255;
				g=255;
				b=90;
		}
		else
		{
				r=255;
				g=70;
				b=255;
		}

		if (m_GI[ec].life == 1 && m_GI[ec].alphaB > 0)
		{
#if (_PACKET_VER < 0)
			for (four=0;four<4;four++)
#else
			for (four=0;four<2;four++)
#endif
			{
				Angle=m_GI[ec].RotStart;

				Rx=m_GI[ec].max_height - minus;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				vec1.x=cs1*Rx;
				vec1.z=sn1*Rx;
				vec1.y=0;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				vec2.x=cs1*Rx;
				vec2.z=sn1*Rx;
				vec2.y=0;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				vec3.x=cs1*Rx;
				vec3.z=sn1*Rx;
				vec3.y=0;

				Angle+=90;
				if (Angle>=360)
					Angle-=360;

				cs1 = g_cosTable[Angle];
				sn1 = g_sinTable[Angle];
				vec4.x=cs1*Rx;
				vec4.z=sn1*Rx;
				vec4.y=0;

				Tei_TowardScreen(vec1,vec2,vec3,vec4,m_GI[0].alphaT,m_GI[1].alphaT);

				if (four==0)
				{
					vec1+=m_GI[ec].vecB_now;
					vec2+=m_GI[ec].vecB_now;
					vec3+=m_GI[ec].vecB_now;
					vec4+=m_GI[ec].vecB_now;
				}
				else if (four==1)
				{
					vec1+=m_GI[ec].vecB_pre;
					vec2+=m_GI[ec].vecB_pre;
					vec3+=m_GI[ec].vecB_pre;
					vec4+=m_GI[ec].vecB_pre;
				}
				else if (four==2)
				{
					vec1+=m_GI[ec].vecT_now;
					vec2+=m_GI[ec].vecT_now;
					vec3+=m_GI[ec].vecT_now;
					vec4+=m_GI[ec].vecT_now;
				}
				else
				{
					vec1+=m_GI[ec].vecT_pre;
					vec2+=m_GI[ec].vecT_pre;
					vec3+=m_GI[ec].vecT_pre;
					vec4+=m_GI[ec].vecT_pre;
				}

				alpha = m_GI[ec].alphaB / (four+1);

				if (m_GI[ec].flag1[3] == 0)
					RenderTeiRect(&rpCnt, alpha, vec1,vec2,vec3,vec4,wtm,0,m_GI[ec].flag1[3],1,r,g,b);
				else
					RenderTeiRect(&rpCnt, alpha / 4, vec1,vec2,vec3,vec4,wtm,0,m_GI[ec].flag1[3],1,255,255,255);

				if ( four == 0 )
					minus+=0.5f;
				else
					minus+=0.25f;
			}
		}
	}
}

void	CEffectPrim :: Tei_TowardScreen(vector3d	&vec1, vector3d	&vec2, vector3d	&vec3, vector3d	&vec4, short Rot0 , short Rot1)
{
	float Rx, Rz;

			Rx=vec1.z;
			Rz=vec1.y;

			vec1.z=(Rx*g_cosTable[Rot1])-(Rz*g_sinTable[Rot1]);
			vec1.y=(Rx*g_sinTable[Rot1])+(Rz*g_cosTable[Rot1]);

			Rx=vec2.z;
			Rz=vec2.y;

			vec2.z=(Rx*g_cosTable[Rot1])-(Rz*g_sinTable[Rot1]);
			vec2.y=(Rx*g_sinTable[Rot1])+(Rz*g_cosTable[Rot1]);

			Rx=vec3.z;
			Rz=vec3.y;

			vec3.z=(Rx*g_cosTable[Rot1])-(Rz*g_sinTable[Rot1]);
			vec3.y=(Rx*g_sinTable[Rot1])+(Rz*g_cosTable[Rot1]);

			Rx=vec4.z;
			Rz=vec4.y;

			vec4.z=(Rx*g_cosTable[Rot1])-(Rz*g_sinTable[Rot1]);
			vec4.y=(Rx*g_sinTable[Rot1])+(Rz*g_cosTable[Rot1]);




			Rx=vec1.x;
			Rz=vec1.z;

			vec1.x=(Rx*g_cosTable[Rot0])-(Rz*g_sinTable[Rot0]);
			vec1.z=(Rx*g_sinTable[Rot0])+(Rz*g_cosTable[Rot0]);

			Rx=vec2.x;
			Rz=vec2.z;

			vec2.x=(Rx*g_cosTable[Rot0])-(Rz*g_sinTable[Rot0]);
			vec2.z=(Rx*g_sinTable[Rot0])+(Rz*g_cosTable[Rot0]);

			Rx=vec3.x;
			Rz=vec3.z;

			vec3.x=(Rx*g_cosTable[Rot0])-(Rz*g_sinTable[Rot0]);
			vec3.z=(Rx*g_sinTable[Rot0])+(Rz*g_cosTable[Rot0]);

			Rx=vec4.x;
			Rz=vec4.z;

			vec4.x=(Rx*g_cosTable[Rot0])-(Rz*g_sinTable[Rot0]);
			vec4.z=(Rx*g_sinTable[Rot0])+(Rz*g_cosTable[Rot0]);

}

void	CEffectPrim :: Tei_TowardScreen2(vector3d	&vec1, vector3d	&vec2, vector3d	&vec3, vector3d	&vec4, short Rot0 , short Rot1)
{
	float Rx, Rz;

			Rx=vec1.z;
			Rz=vec1.y;


			vec1.y=(Rx*g_sinTable[Rot1])+(Rz*g_cosTable[Rot1]);

			Rx=vec2.z;
			Rz=vec2.y;


			vec2.y=(Rx*g_sinTable[Rot1])+(Rz*g_cosTable[Rot1]);

			Rx=vec3.z;
			Rz=vec3.y;


			vec3.y=(Rx*g_sinTable[Rot1])+(Rz*g_cosTable[Rot1]);

			Rx=vec4.z;
			Rz=vec4.y;


			vec4.y=(Rx*g_sinTable[Rot1])+(Rz*g_cosTable[Rot1]);






























}

void	CRagEffect :: MagicRod()
{
	if (m_stateCnt == 0)
	{
		m_pos = m_master->m_pos;

		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\sage_magic rod.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
}

void	CRagEffect :: Magnum2(char *tName)
{
	float x,z;
	int time;

	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
		m_flag = 0;


	if (m_stateCnt == 20)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("permeter_attack.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}
	else if (m_stateCnt == 30)
	{
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\EF_MagnumBreak.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
	}


	if (m_stateCnt >= 25 && m_stateCnt <= 55 && m_stateCnt%3 == 0 )
	{
		m_flag++;

		if (m_flag < 11)
		{
			time = m_stateCnt - 25;

			g_prim = LaunchEffectPrim(PP_MAGNUM2);

			g_prim->m_renderFlag = RF_EFFECT;

			g_prim->m_duration = m_duration;

			g_prim->m_totalTexture = 1;
			g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];
			g_prim->m_texture[0] = g_texMgr.GetTexture(tName, false);

			g_prim->m_size=5;



			int ec;

			for (ec=0;ec<4;ec++)
				g_prim->m_GI[ec].life = 0;

			for (ec=0;ec<2;ec++)
			{
				g_prim->m_GI[ec].life = 1;
				g_prim->m_GI[ec].process = 0;

				g_prim->m_GI[ec].full_display_angle = 360;
				g_prim->m_GI[ec].max_height = 20.0f - (time*0.7f);


				x=m_deltaPos.x-m_pos.x;
				z=m_deltaPos.z-m_pos.z;
				g_prim->m_GI[ec].RotStart = (ec*9) + (time*6);
				g_prim->m_GI[ec].alphaB = 0;
				g_prim->m_GI[ec].distance = 5.0f+(time*0.2f);
				g_prim->m_GI[ec].rise_angle=70-(ec*8)-(time*2);

				g_prim->m_GI[ec].alphaT=ec;
			}

		}
	}
}

bool	CEffectPrim :: PrimMagnum2()
{

	int ec;

	m_pos = m_master->m_pos;

	for (ec=0;ec<4;ec++)
	{
		if (m_GI[ec].life == 1)
		{
			m_GI[ec].process+=1;




			if (m_GI[ec].rise_angle<0)
				m_GI[ec].rise_angle=0;



			if (m_GI[ec].RotStart>=360)
				m_GI[ec].RotStart-=360;

			if (m_GI[ec].process<9)
			{
				if (m_GI[ec].alphaT==0)
				{
					m_GI[ec].alphaB+=10;
					if (m_GI[ec].alphaB>80)
						m_GI[ec].alphaB=80;
				}
				else
				{
					m_GI[ec].alphaB+=5;
					if (m_GI[ec].alphaB>40)
						m_GI[ec].alphaB=40;
				}
			}
			else
			{
				m_GI[ec].alphaB-=30;

				if (m_GI[ec].alphaB<0)
				{
					m_GI[ec].alphaB=0;
					m_GI[ec].life=0;
				}

			}

		}
	}

	return true;
}

void	CEffectPrim :: RenderMagnum2(matrix &wtm)
{
	float		cs1, sn1;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;
	int order;
	int count;
	int Angle;
	int BasicAngle;
	float Rx,Ry,Rz;
	int ec;

	for (ec=0 ; ec<4 ; ec++)
	{
		if ( rpCnt < (MAX_RENDERPRIM/2) )
		{
			if (m_GI[ec].life == 1)
			{
				BasicAngle = m_GI[ec].full_display_angle;
				BasicAngle /= E_DIVISION-1;

				order=0;

				for (count = 0 ; count<=m_GI[ec].full_display_angle ; count+=BasicAngle)

				{
					Angle=count+m_GI[ec].RotStart;
					if (Angle>=360)
						Angle-=360;

					if (m_GI[ec].full_display_angle == 360)
					{
						if ( order == E_DIVISION-1 )
							Angle = m_GI[ec].RotStart;
					}

					cs1 = g_cosTable[Angle];
					sn1 = g_sinTable[Angle];


					m_GI[ec].vecB_now.x=cs1*m_GI[ec].distance;
					m_GI[ec].vecB_now.z=sn1*m_GI[ec].distance;
					m_GI[ec].vecB_now.y=-3.0f;

					Rx=g_cosTable[m_GI[ec].rise_angle] * m_GI[ec].max_height;
					Ry=g_sinTable[m_GI[ec].rise_angle] * m_GI[ec].max_height;

					Rz=sn1*Rx;
					Rx=cs1*Rx;

					m_GI[ec].vecT_now.x=m_GI[ec].vecB_now.x+Rx;
					m_GI[ec].vecT_now.z=m_GI[ec].vecB_now.z+Rz;
					m_GI[ec].vecT_now.y=-(Ry+3.0f);

					if (order > 0)
					{
						if (m_GI[ec].alphaB > 0)
							RenderTeiRect(&rpCnt, order-1, m_GI[ec].alphaB, m_GI[ec].alphaB, m_GI[ec].vecB_pre,m_GI[ec].vecB_now,m_GI[ec].vecT_now,m_GI[ec].vecT_pre,wtm);
					}

					order++;
					if (order>=E_DIVISION)
						order=E_DIVISION-1;

					m_GI[ec].vecB_pre=m_GI[ec].vecB_now;
					m_GI[ec].vecT_pre=m_GI[ec].vecT_now;
				}
			}
		}
	}
}

enum { MM_SONG_TALK_LOCAL = 0xb3 };
void	CRagEffect :: Talk_FrostJoke(char F1)
{
	if (m_stateCnt == 0)
	{
		g_modeMgr.GetCurMode()->SendMsg(MM_SONG_TALK_LOCAL,F1,0,0);
	}
}

void	CRagEffect :: Effect_SPR(char F1)
{
	int startcnt;
	startcnt = 0;
	m_pos = m_master->m_pos;

	if (F1 == 30)
	{
		startcnt = 30;
	}

	if (m_stateCnt == 99999998)
		m_stateCnt = 0;

	if (m_stateCnt == startcnt)
	{
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);
		g_prim->m_duration = m_duration;
		g_prim->m_deltaPos2.y = 0;
		g_prim->m_renderFlag |= RF_NODEPTHCHECK;
		g_prim->m_pattern |= PT_USEORGARGB;
		g_prim->m_animSpeed = 2;
		g_prim->m_repeatAnim = false;

		switch (F1)
		{
			case 0:

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc4\xb3\xbd\xbd\xb8\xb5.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc4\xb3\xbd\xbd\xb8\xb5.act");

				break;
			case 1:
			{
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\h_blood_lust.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xba\xed\xb7\xaf\xb5\xe5\xb7\xaf\xbd\xba\xc6\xae.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xba\xed\xb7\xaf\xb5\xe5\xb7\xaf\xbd\xba\xc6\xae.act");

				break;
			}
			case 2:
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\item_thunder.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\item_thunder.act");
				break;
			case 3:
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\item_cloud.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\item_cloud.act");
				break;
			case 4:
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\item_curse.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\item_curse.act");
				break;
			case 5:
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\item_zzz.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\item_zzz.act");
				break;
			case 6:
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\item_rain.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\item_rain.act");
				break;
			case 7:
			{
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\ef_firehit.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				g_prim->m_animSpeed = 3;
				g_prim->m_pattern = 0;
				g_prim->m_alpha = 220;
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef01.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef01.act");
				break;
			}
			case 8:
			{
				float Rx,Rz;
				short angle;

				g_prim->m_animSpeed = 4;
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef02.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef02.act");

				Rx=m_pos.x-m_deltaPos.x;
				Rz=m_pos.z-m_deltaPos.z;

				g_prim->Get_Angle(Rx,Rz,&angle);

				int a2;
				a2=g_modeMgr.GetGameMode()->GetView()->GetCurLongitude();

				angle=angle+a2;
				angle%=360;
				if (angle<0)
					angle+=360;

				if ( angle>=180 && angle<270 )
					g_prim->SetAction(2, 16, MT_ONETIME);
				else if ( angle>=90 && angle<180 )
					g_prim->SetAction(3, 16, MT_ONETIME);
				else if ( angle>=0 && angle<90)
					g_prim->SetAction(0, 16, MT_ONETIME);
				else
					g_prim->SetAction(1, 16, MT_ONETIME);

				break;
			}
			case 10:
				g_prim->m_animSpeed = 4;
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef04.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef04.act");
				g_prim->m_repeatAnim = true;
				break;
			case 9:
				g_prim->m_animSpeed = 4;
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef03.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef03.act");
				break;
			case 11:
				g_prim->m_animSpeed = 4;
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef05.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef05.act");
				break;
			case 12:
				g_prim->m_animSpeed = 4;
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef06.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef06.act");
				break;
			case 13:
				g_prim->m_animSpeed = 4;
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef07.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\m_ef07.act");
				break;
			case 14:

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc7\xd5\xb0\xdd_.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc7\xd5\xb0\xdd_.act");

				break;
			case 15:
			{
				g_prim->m_animSpeed = 4;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb1\xd7\xb8\xb2\xc0\xda\xba\xa3\xb1\xe2.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb1\xd7\xb8\xb2\xc0\xda\xba\xa3\xb1\xe2.act");

				if (random(2) == 0)
					g_prim->SetAction(4, 16, MT_ONETIME);
				else
					g_prim->SetAction(0, 16, MT_ONETIME);

				break;
			}
			case 16:
			{
				g_prim->m_deltaPos2.y = -6;
				g_prim->m_renderFlag = RF_ALPHA;

				g_prim->m_animSpeed = 6;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb4\xd9\xb4\xd9\xb9\xcc.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb4\xd9\xb4\xd9\xb9\xcc.act");


				g_prim->SetAction(0, 16, MT_ONETIME);

				break;
			}
			case 17:
			{
				g_prim->m_animSpeed = 4;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbe\xc8\xb0\xb3\xba\xa3\xb1\xe2.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbe\xc8\xb0\xb3\xba\xa3\xb1\xe2.act");

				if (random(2) == 0)
					g_prim->SetAction(2, 16, MT_ONETIME);
				else
					g_prim->SetAction(0, 16, MT_ONETIME);

				break;
			}
			case 18:
			{
				g_prim->m_deltaPos2.y = -6;
				g_prim->m_animSpeed = 2;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc0\xcf\xbc\xb6.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc0\xcf\xbc\xb6.act");

				g_prim->SetAction(0, 16, MT_ONETIME);

				break;
			}
			case 19:
			{
				g_prim->m_deltaPos2.y = -1;
				g_prim->m_renderFlag = RF_ALPHA;
				g_prim->m_animSpeed = random(5)+3;
				g_prim->m_repeatAnim = true;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc8\xad\xbf\xb0\xc1\xf8.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc8\xad\xbf\xb0\xc1\xf8.act");


				g_prim->SetAction(0, 16, MT_ONETIME);

				break;
			}
			case 20:
			{

				g_prim->m_animSpeed = 4;
				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb5\xa5\xbd\xba\xc6\xe4\xb6\xf3\xb5\xb5.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb5\xa5\xbd\xba\xc6\xe4\xb6\xf3\xb5\xb5.act");

				g_prim->SetAction(0, 16, MT_ONETIME);

				break;
			}
			case 21:
			{
				g_prim->m_deltaPos2.y = -1;
				g_prim->m_renderFlag = RF_ALPHA;
				g_prim->m_repeatAnim = true;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb6\xf3\xc0\xcc\xc6\xae\xb4\xd7\xbd\xba\xc7\xc7\xbe\xee.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb6\xf3\xc0\xcc\xc6\xae\xb4\xd7\xbd\xba\xc7\xc7\xbe\xee.act");


				g_prim->SetAction(0, 16, MT_ONETIME);
				break;
			}
			case 22:
			{
				g_prim->m_deltaPos2.y = -1;
				g_prim->m_renderFlag = RF_ALPHA;
				g_prim->m_repeatAnim = true;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xba\xed\xb6\xf3\xc0\xce\xb5\xe5\xbd\xba\xc7\xc7\xbe\xee.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xba\xed\xb6\xf3\xc0\xce\xb5\xe5\xbd\xba\xc7\xc7\xbe\xee.act");


				g_prim->SetAction(0, 16, MT_ONETIME);
				break;
			}
			case 23:
			{
				g_prim->m_deltaPos2.y = -1;
				g_prim->m_renderFlag = RF_ALPHA;
				g_prim->m_repeatAnim = true;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xf7\xc0\xcc\xc1\xf0\xbd\xba\xc7\xc7\xbe\xee.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xf7\xc0\xcc\xc1\xf0\xbd\xba\xc7\xc7\xbe\xee.act");


				g_prim->SetAction(0, 16, MT_ONETIME);
				break;
			}
			case 24:
			{
				g_prim->m_deltaPos2.y = -1;
				g_prim->m_renderFlag = RF_ALPHA;
				g_prim->m_repeatAnim = true;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc7\xc1\xb8\xae\xc2\xa1\xbd\xba\xc7\xc7\xbe\xee.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc7\xc1\xb8\xae\xc2\xa1\xbd\xba\xc7\xc7\xbe\xee.act");


				g_prim->SetAction(0, 16, MT_ONETIME);
				break;
			}
			case 25:
			{
				g_prim->m_deltaPos2.y = -1;
				g_prim->m_renderFlag = RF_ALPHA;
				g_prim->m_repeatAnim = true;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc7\xc3\xb7\xb9\xbe\xee\xbd\xba\xc7\xc7\xbe\xee.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc7\xc3\xb7\xb9\xbe\xee\xbd\xba\xc7\xc7\xbe\xee.act");


				g_prim->SetAction(0, 16, MT_ONETIME);
				break;
			}
			case 26:
			{
				g_prim->m_animSpeed = 4;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb7\xa1\xc7\xc7\xb5\xe5\xbb\xfe\xbf\xf6.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb7\xa1\xc7\xc7\xb5\xe5\xbb\xfe\xbf\xf6.act");

				g_prim->SetAction(0, 16, MT_ONETIME);
				break;
			}
			case 27:
			{
				g_prim->m_animSpeed = 2;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb8\xc5\xc1\xf6\xc4\xc3\xba\xd2\xb8\xb4.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xb8\xc5\xc1\xf6\xc4\xc3\xba\xd2\xb8\xb4.act");

				g_prim->SetAction(0, 16, MT_ONETIME);
				break;
			}
			case 28:
			{
				float Rx,Rz;
				short angle;

				g_prim->m_animSpeed = 4;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbd\xba\xc7\xc1\xb7\xb9\xb5\xe5.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbd\xba\xc7\xc1\xb7\xb9\xb5\xe5.act");

				Rx=m_pos.x-m_deltaPos.x;
				Rz=m_pos.z-m_deltaPos.z;

				g_prim->Get_Angle(Rx,Rz,&angle);

				int a2;
				a2=g_modeMgr.GetGameMode()->GetView()->GetCurLongitude();

				angle=angle+a2;
				angle%=360;
				if (angle<0)
					angle+=360;

				if ( angle>=180 && angle<270 )
					g_prim->SetAction(4, 16, MT_ONETIME);
				else if ( angle>=90 && angle<180 )
					g_prim->SetAction(6, 16, MT_ONETIME);
				else if ( angle>=0 && angle<90)
					g_prim->SetAction(0, 16, MT_ONETIME);
				else
					g_prim->SetAction(2, 16, MT_ONETIME);

				break;
			}
			case 29:
			{
				g_prim->m_animSpeed = 2;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xae\xb7\xa1\xc5\xb7.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xae\xb7\xa1\xc5\xb7.act");

				g_prim->SetAction(0, 16, MT_ONETIME);
				break;
			}
			case 30:
			{
				g_prim->m_animSpeed = 2;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xae\xb8\xae\xc7\xc3\xbe\xd7\xbc\xc7.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xae\xb8\xae\xc7\xc3\xbe\xd7\xbc\xc7.act");

				g_prim->SetAction(0, 16, MT_ONETIME);
				break;
			}
			case 31:
			{
				g_prim->m_animSpeed = 4;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbe\xee\xbd\xba\xc4\xf9\xc0\xcc\xc5\xa9.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbe\xee\xbd\xba\xc4\xf9\xc0\xcc\xc5\xa9.act");

				g_prim->SetAction(0, 16, MT_ONETIME);
				break;
			}
			case 32:
			{
				g_prim->m_animSpeed = 4;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xf8\xc1\xd7_\xb7\xaf\xba\xea.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xf8\xc1\xd7_\xb7\xaf\xba\xea.act");

				g_prim->SetAction(0, 16, MT_ONETIME);
				PlayWave("effect\\itempokjuk.wav", 0, 0, 0);
				break;
			}
			case 33:
			{
				g_prim->m_animSpeed = 4;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xf8\xc1\xd7_\xc8\xad\xc0\xcc\xc6\xae\xb5\xa5\xc0\xcc.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xf8\xc1\xd7_\xc8\xad\xc0\xcc\xc6\xae\xb5\xa5\xc0\xcc.act");

				g_prim->SetAction(0, 16, MT_ONETIME);
				PlayWave("effect\\itempokjuk.wav", 0, 0, 0);
				break;
			}
			case 34:
			{
				g_prim->m_animSpeed = 4;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xf8\xc1\xd7_\xb9\xdf\xb7\xbb\xc5\xb8\xc0\xce.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xf8\xc1\xd7_\xb9\xdf\xb7\xbb\xc5\xb8\xc0\xce.act");

				g_prim->SetAction(0, 16, MT_ONETIME);
				PlayWave("effect\\itempokjuk.wav", 0, 0, 0);
				break;
			}
			case 35:
			{
				g_prim->m_animSpeed = 4;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xf8\xc1\xd7_\xbb\xfd\xc0\xcf.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xf8\xc1\xd7_\xbb\xfd\xc0\xcf.act");

				g_prim->SetAction(0, 16, MT_ONETIME);
				PlayWave("effect\\itempokjuk.wav", 0, 0, 0);
				break;
			}
			case 36:
			{
				g_prim->m_animSpeed = 4;

				g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xf8\xc1\xd7_\xc5\xa9\xb8\xae\xbd\xba\xb8\xb6\xbd\xba.spr");
				g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xc6\xf8\xc1\xd7_\xc5\xa9\xb8\xae\xbd\xba\xb8\xb6\xbd\xba.act");

				g_prim->SetAction(0, 16, MT_ONETIME);
				PlayWave("effect\\itempokjuk.wav", 0, 0, 0);
				break;
			}
		}

		g_prim->InitSegment();
	}
}

void	CRagEffect :: Bubble_Drop()
{
	int ec;
	float m_pH;
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0)
	{
		if (m_param[0] <= 1)
			m_pH = 50;
		else
			m_pH = m_param[0];

		g_prim = LaunchEffectPrim(PP_BUBBLE_DROP);

		g_prim->m_renderFlag = RF_EFFECT;

		g_prim->m_duration = m_duration;

		g_prim->m_texture = new CTexture* [g_prim->m_totalTexture];

		g_prim->m_texture[0] = g_texMgr.GetTexture("effect\\\xbd\xe3\xb4\xf5\xbd\xba\xc5\xe8\xc6\xc4\xc6\xbc\xc5\xac.tga", false);


		g_prim->m_size=0;



		ec=0;

		g_prim->m_GI[ec].life = 1;
		g_prim->m_GI[ec].process=0;

		g_prim->m_GI[ec].distance = 1.0f;
		g_prim->m_GI[ec].alphaB = 0;

		g_prim->m_GI[ec].vecB_pre.x=0;
		g_prim->m_GI[ec].vecB_pre.z=0;
		g_prim->m_GI[ec].vecB_pre.y = -m_pH;

		g_prim->m_GI[ec].RotStart=0;
		g_prim->m_GI[ec].height[1] = 1.0f;
		g_prim->m_GI[ec].full_display_angle = random(360);
		g_prim->m_GI[ec].flag1[0] = 0;
		g_prim->m_GI[ec].flag1[2] = 2;

		g_prim->m_GI[ec].flag1[4] = m_pH;

		g_prim->m_GI[1].life = 1;
		g_prim->m_GI[1].alphaB = 0;

		g_prim->m_GI[2].life = 1;
		g_prim->m_GI[2].alphaB = 0;

		g_prim->m_GI[3].life = 1;
		g_prim->m_GI[3].alphaB = 0;

	}
}

void	CPc :: SetSprShoe(int shoe)
{
	m_shoe = shoe;
}

void	CRagEffect :: Vallentine(char F1)
{
	m_pos = m_master->m_pos;

	if (m_stateCnt == 0) {
		vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
		PlayWave("effect\\vallentine.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		g_prim = LaunchEffectPrim(PP_3DPARTICLE);

		g_prim->m_pattern |= PT_USEORGARGB;

		g_prim->m_duration = m_duration;
		g_prim->m_deltaPos2.y = 0;
		g_prim->m_animSpeed = 2;
		g_prim->m_repeatAnim = false;
		if (F1 == 2)
		{
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\05vallentine.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\05vallentine.act");

			g_prim->SetAction(0, 16, MT_ONETIME);
		}
		else if (F1 == 3)
		{
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\fast.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\fast.act");

			g_prim->m_animSpeed = 4;
			g_prim->SetAction(0, 16, MT_ONETIME);
		}
		else if (F1 == 4)
		{
			g_prim->m_renderFlag |= RF_NODEPTHCHECK;

			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\ro2effect.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\ro2effect.act");

			g_prim->m_animSpeed = 4;
			g_prim->SetAction(0, 16, MT_ONETIME);
		}
		else
		{
			g_prim->SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\vallentine.spr");
			g_prim->SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\vallentine.act");

			g_prim->SetAction(F1, 16, MT_ONETIME);
		}

		g_prim->InitSegment();
	}
}

void	CRagEffect :: MakeBlur2()
{
			m_pos.x = 155;
			m_pos.z = 255;
			m_pos.y = 155;
			m_master->SetArgb(-1, int(m_pos.x), int(m_pos.y), int (m_pos.z));

			if (m_stateCnt == 9998)
				m_stateCnt = 0;

			if (m_stateCnt % 20 == 0)
			{
				CBlurPC *blur = new CBlurPC((CPc *)m_master);
				blur->SendMsg(this, AM_MOVE, int(m_pos.x), int(m_pos.y), int (m_pos.z));
				g_modeMgr.GetGameMode()->GetWorld()->m_gameObjectList.push_back(blur);
			}
}

void	CPc :: RenderShoe(int shoenum)
{

	int shoechange;

	if (shoenum == 1)
		shoechange = 12;
	else
		shoechange = 7;

	if (g_isSkipFrame > 0)
	{
		m_shoe_count++;
		if (m_shoe_count>(shoechange*2))
			m_shoe_count=1;
		if (m_shoe_count<0)
			m_shoe_count=0;

		if (m_shoe_count == 0)
			shoe_pos = m_pos;

		if (m_shoe_count==shoechange || m_shoe_count==(shoechange*2))
		{
			CGameMode *		gameMode = g_modeMgr.GetGameMode();
			float move_x,move_z;
			short Angle;

			move_x=m_pos.x-shoe_pos.x;
			move_z=m_pos.z-shoe_pos.z;

			shoe_pos = m_pos;

			if ( move_x == 0 && move_z == 0 )
			{
			}
			else

			{
				float one;
				short ii;

				one=_sqrt( (move_x*move_x) + (move_z*move_z) );

				move_x/=one;
				move_z/=one;

				Angle=180;
				for (ii=0;ii<=180;ii++)
				{
					if ( (move_x) >=g_cosTable[ii])
					{
						if ( (move_z) >=0)
						{
							Angle=ii;
						}
						else
						{
							Angle=360-ii;
						}
						break;
					}
				}

				Angle+=135;
				if (Angle>=360)
					Angle-=360;

				if (shoenum == 1)
				{
						if (m_shoe_count == shoechange)
							gameMode->PlaceEffect(&shoe_pos, EF_FOOT, 333 , Angle);
						else
							gameMode->PlaceEffect(&shoe_pos, EF_FOOT2, 333 , Angle);
				}
				else if (shoenum == 2)
				{
					if (m_BodyTime3 > shoechange)
					{
						if (m_shoe_count == shoechange)
							gameMode->PlaceEffect(&shoe_pos, EF_FOOT3, 333 , Angle);
						else
							gameMode->PlaceEffect(&shoe_pos, EF_FOOT4, 333 , Angle);
					}
				}
			}
		}
	}
}

void	CEffectPrim :: Get_Angle(float move_x,float move_z,short *angle)
{
	short ii;
	float one;


	one=_sqrt( (move_x*move_x) + (move_z*move_z) );

	move_z/=one;
	move_x/=one;

	for (ii=0;ii<=180;ii++)
	{
		if ( (move_x) >=g_cosTable[ii])
		{
			if ( (move_z) >=0)
			{
				*angle=ii;
			}
			else
			{
				*angle=360-ii;
			}
			return;
		}
	}

	*angle=180;
}
