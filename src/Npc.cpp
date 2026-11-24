#include "common.h"
#include "framework/Locale.h"
#include "resource/Wave.h"
#include "GameActor.h"
#include "Mode.h"
#include "GameMode.h"
#include "GameWorld.h"
#include "3dGround.h"
#include "resource/Action.h"
#include "resource/World.h"
#include "window/UISys.h"
#include "Session.h"
#include "RagEffect.h"
#include "resource/Skybox.h"
#include "MercenaryAI.h"
#include "pdb_enums.h"
#include <stdio.h>
#include "resource/Ground.h"

extern UIWindowMgr g_windowMgr;
extern CModeMgr g_modeMgr;
extern CSession g_session;
extern CRenderer* g_renderer;
extern float g_avgPixelRatio;
extern bool IsEventAID(DWORD aid);
bool IsEffectStateHiding(DWORD effectState);
bool IsEffectStateBurrow(DWORD effectState);
bool IsEffectStateSpecialHiding(DWORD effectState);


CBlowEffect :: CBlowEffect()
{
	OnInit();
}

CBlowEffect :: ~CBlowEffect()
{
	OnExit();
}

void	CBlowEffect :: OnInit()
{

	SetSprName("\xC0\xCC\xC6\xD1\xC6\xAE\\\xC0\xCC\xC6\xD1\xC6\xAE.spr");
	SetActName("\xC0\xCC\xC6\xD1\xC6\xAE\\\xC0\xCC\xC6\xD1\xC6\xAE.act");
	m_baseAction = 0;
}

void	CBlowEffect :: OnExit() {}

bool	CBlowEffect :: OnProcess()
{
	float		stateCnt = float(timeGetTime() - m_stateStartTick) / 24.0f;
	m_curAction = m_baseAction;
	CActRes *	actRes = GetActRes();
	int			motionCnt = actRes->GetMotionCount(m_curAction);
	m_curMotion = int(stateCnt/m_motionSpeed) % (motionCnt);

	ProcessSound();
	if ((stateCnt/m_motionSpeed)/motionCnt >= 1)
		return	false;

	return	true;
}

void	CBlowEffect :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	switch (message) {
		case	AM_SETPOS: {
			vector3d *	pos = (vector3d *)arg1;
			m_pos.x = pos->x;
			m_pos.y = pos->y;
			m_pos.z = pos->z;
			m_isVisible = true;
			if (arg2)
				m_sprArgb = 0xffff0000;
			else
				m_sprArgb = 0xffffffff;
			m_stateStartTick = timeGetTime();
		}
		break;
		default:
			CRenderObject::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

CCorpse :: CCorpse()
{
	OnInit();
}

CCorpse :: ~CCorpse()
{
	OnExit();
}

void CCorpse::OnInit()
{
	m_isVisible = true;
	m_isLieOnGround = true;
	m_stateStartTick = timeGetTime();
}

void	CCorpse :: OnExit() {}

bool	CCorpse :: OnProcess()
{
	float		stateCnt = float(timeGetTime() - m_stateStartTick) / 24.0f;
	m_curAction = m_baseAction + Get8Dir(m_roty);

	CActRes *	actRes = GetActRes();
	int			motionCnt = actRes->GetMotionCount(m_curAction);
	m_curMotion = motionCnt-1;

	SetArgb(255-stateCnt, -1, -1, -1);

	if (stateCnt >= 255)
		return	false;
	return	true;
}

void CCorpse::SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	switch (message) {
		default:
			CRenderObject::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

CBlurPC :: CBlurPC(CPc* pc)
{
	OnInit();

	m_pos				= pc->m_pos;
	m_lastPixelRatio	= pc->m_lastPixelRatio;
	m_sprShift			= pc->m_sprShift;
	m_zoom				= pc->m_zoom;
	m_baseAction		= pc->m_baseAction;
	m_curAction			= pc->m_curAction;
	m_curMotion			= pc->m_curMotion;
	SetActName(pc->GetActName());
	m_alwaysTopLayer	= pc->m_alwaysTopLayer;
	m_offsetOow			= pc->m_offsetOow;
	m_isLieOnGround		= pc->m_isLieOnGround;
	m_isSingleColor		= pc->m_isSingleColor;
	m_sprAngle			= pc->m_sprAngle;
	m_bodyPalette		= pc->m_bodyPalette;
	m_headPalette		= pc->GetHeadPalette();
	m_effectState		= pc->GetEffectState();
	m_headPaletteName	= pc->GetHeadPaletteName();
	SetSprNames(pc->GetSprNames());
	SetActNames(pc->GetActNames());
	m_imfName			= pc->GetImfNames();
	m_gdid				= pc->GetGdid();
	m_gid				= pc->GetGID();
	SetSex(pc->GetSex());
	SetJob(pc->GetJob());

	m_motionY			= 0;
	m_alphaDelta		= 0x10000000;
}


CBlurPC :: ~CBlurPC()
{
	OnExit();
}

void CBlurPC::OnInit()
{
	m_isVisible = true;
	m_shadowOn	= FALSE;
	m_stateStartTick = timeGetTime();
}

void	CBlurPC :: OnExit() {}

bool	CBlurPC :: OnProcess()
{
	if (m_sprArgb >= m_alphaDelta) {
		m_sprArgb -= m_alphaDelta;
		return true;
	}
	return false;
}

void	CBlurPC :: Render(matrix &ptm)
{
	if (IsEffectStateHiding(m_effectState)
		|| IsEffectStateSpecialHiding(m_effectState)
		|| IsEffectStateBurrow(m_effectState))
		return;

	float	x, y;
	x = (rand() % 2 + 1) / 10.0f;
	y = (rand() % 2 + 1) / 10.0f;
	if (rand() % 2 == 0)
		x *= -1;
	if (rand() % 2 == 0)
		y *= -1;

	if (m_isVisible) {
		extern ServiceType g_serviceType;
		extern bool IsGravityAid(DWORD aid);
		if (g_serviceType == 4) {
			if (IsGravityAid(m_gid) && m_gid != g_session.GetAid())
				return;
		}

		if (m_stateId != ST_SIT && m_stateId != ST_DEAD)
			RenderShadow(ptm, m_zoom);

		if (m_isVisibleBody) {
			if (IsEventAID(m_gid))
				CPc::RenderBody2(ptm, TRUE);
			else if (m_renderWithoutLayer)
				RenderBody(ptm, 0);
			else
				CPc::RenderBodyLayer(ptm, TRUE);
		} else
			UpdateTlvertex(ptm);
	} else
		UpdateTlvertex(ptm);
}

void	CBlurPC :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	switch (message) {
		case 0:
			SetArgb(180, arg1, arg2, arg3);
			m_alphaDelta = 0x8000000;
			break;
		case 1:
			m_sprArgb = m_sprArgb & 0x00FFFFFF + (DWORD)arg1 * 0x01000000;
			m_alphaDelta = (DWORD)arg2 * 0x01000000;
			break;
		case 2:
			m_motionY = *((float *)&arg1);
			break;
		case 3:
			m_isMotionFreezed = TRUE;
			break;
		default:
			CPc::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

CMasterChaser :: CMasterChaser()
{
	OnInit();
}

CMasterChaser :: ~CMasterChaser()
{
	OnExit();
}

void	CMasterChaser :: OnInit()
{
	m_pursuedMaster = NULL;
}

void	CMasterChaser :: OnExit() {}

CNumEffect :: CNumEffect()
{
	OnInit();
}

CNumEffect :: ~CNumEffect()
{
	OnExit();
}

void	CNumEffect :: OnInit()
{
	SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbc\xfd\xc0\xda.spr");
	SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbc\xfd\xc0\xda.act");
	m_baseAction = 0;
	m_alwaysTopLayer = true;
}

void	CNumEffect :: OnExit() {}

bool	CNumEffect :: OnProcess()
{
	float	stateCnt = float(timeGetTime() - m_stateStartTick) / 24.0f;
	m_curAction = m_baseAction;

	m_pos.y = m_orgPosY - float(stateCnt)*0.18f;
	SetArgb(255 - stateCnt*2, -1, -1, -1);
	if (m_zoom > 1.0f)
		m_zoom = m_orgZoom - float(stateCnt)*0.24f;
	else if (m_zoom < 1.0f)
		m_zoom = 1.0f;

	if (stateCnt > 120)
		return	false;
	return	true;
}

void	CNumEffect :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	switch (message) {
		case	AM_SETMASTERGID: {
			}
			break;
		case	AM_SETPOS: {
			vector3d *		sPos = (vector3d *)arg1;
			m_pos = *sPos;
			m_pos.y -= 12;
			m_orgPosY = m_pos.y;
			m_isVisible = true;
			m_curMotion = arg2;
			if (!(m_curMotion<=9 && m_curMotion>=0))
				m_curMotion = 0;
			m_sprShift = arg3;
			m_orgZoom = m_zoom = 5;
			m_stateStartTick = timeGetTime();
			break;
		}
		default:
			CRenderObject::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

CSoundMaker :: CSoundMaker()
{
	OnInit();
}


CSoundMaker :: ~CSoundMaker()
{
	OnExit();
}

void	CSoundMaker :: OnInit()
{
	m_waveName = "";
	m_vol = 100;
	m_range = 1;
	m_width = 1 + 2 * m_range;
	m_height = 1 + 2 * m_range;
	{
		m_diagonal = _sqrt(float(m_height*m_height + m_width*m_width));
	}
	m_lastPlayTime = timeGetTime()-4000;
}

void	CSoundMaker :: OnExit() {}

bool	CSoundMaker :: OnProcess()
{
	CPlayer *		player = g_modeMgr.GetGameMode()->GetWorld()->m_player;
	vector3d &		pPos = player->m_pos;
	float			distToPlayer = CalcDist(pPos.x, pPos.z);
	if (distToPlayer-(m_diagonal+m_range) > 5*26)
		return	true;

	float	cX;
	float	cZ;

	float	dx = pPos.x - m_pos.x;
	float	dz = pPos.z - m_pos.z;

	int		sabun = 0;
	if (dx >=0 && dz >=0) {
		sabun = 0;
	} else if (dx<0 && dz>=0) {
		sabun = 1;
	} else if (dx<0 && dz<0) {
		sabun = 2;
	} else if (dx>=0 && dz<0) {
		sabun = 3;
	}

	float	rectSlope = m_height / m_width;
	float	pPosSlope = dz / dx;
	bool	isVertLine = rectSlope >= abs(pPosSlope);

	float	xPole=1, yPole=1;
	switch (sabun) {
		case	0:
			xPole = 1; yPole = 1;
			break;
		case	1:
			xPole = -1; yPole = 1;
			break;
		case	2:
			xPole = -1; yPole = -1;
			break;
		default:
			xPole = 1; yPole = -1;
			break;
	}

	if (isVertLine) {
		cX = xPole * m_width / 2;
		cZ = pPosSlope * cX;
	} else {
		cZ = yPole * m_height / 2;
		cX = cZ / pPosSlope;
	}
	cX += m_pos.x;
	cZ += m_pos.z;

	float	closestDistToPlayer = player->CalcDist(cX, cZ);
	if (closestDistToPlayer-m_range > 5*15)
		return	true;

	if (m_waveName.size())
	{
		if (timeGetTime() > m_lastPlayTime+m_cycle)
		{
			m_lastPlayTime = timeGetTime();
			PlayWave(m_waveName.c_str(), cX-pPos.x, 0, cZ-pPos.z, m_range, m_range/6, m_vol);
		}
	}

	return	true;
}

void	CSoundMaker :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	switch (message) {
		case	AM_SETPOS: {
			C3dWorldRes::soundSrcInfo *	info = (C3dWorldRes::soundSrcInfo *)arg1;

			m_waveName = info->waveName;
			m_vol = info->vol;
			m_width = info->width;
			m_height = info->height;
			m_range = info->range;
			m_pos = info->pos;
			m_diagonal = _sqrt(float(m_height*m_height + m_width*m_width));
			m_cycle = info->cycle * 1000;
			m_lastPlayTime = timeGetTime()-m_cycle;
			break;
		}
		default:
			CRenderObject::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

CGndRiser :: CGndRiser()
{
	OnInit();
}


CGndRiser :: ~CGndRiser()
{
	OnExit();
}

void	CGndRiser :: OnInit() {}

void	CGndRiser :: OnExit()
{
	CGameMode *		gameMode = g_modeMgr.GetGameMode();

	if (gameMode) {
		gameMode->GetWorld()->m_ground->SetGndRiseAmount(0, m_sx, m_sy);
	}
}

bool	CGndRiser :: OnProcess()
{
	float	stateCnt = float(timeGetTime() - m_stateStartTick) / 24.0f;
	if (stateCnt > m_lastCnt)
		return	false;

	CGameMode *		gameMode = g_modeMgr.GetGameMode();
	if (gameMode) {
		if (stateCnt > m_lastCnt/2) {
			gameMode->GetWorld()->m_ground->SetGndRiseAmount((-(m_lastCnt/2.0f)*(m_lastCnt/2.0f) + (stateCnt-m_lastCnt/2)*(stateCnt-m_lastCnt/2))/25.0f, m_sx, m_sy);
		} else {
			gameMode->GetWorld()->m_ground->SetGndRiseAmount(-stateCnt * stateCnt / 25.0f, m_sx, m_sy);
		}
	}

	return	true;
}

void	CGndRiser :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	switch (message) {
		case	AM_SETPOS: {
			vector3d *	sPos = (vector3d *)arg1;
			m_pos = *sPos;
			m_isVisible = false;
			m_stateStartTick = timeGetTime();
			m_lastCnt = 30;

			CGameMode *		gameMode = g_modeMgr.GetGameMode();
			if (gameMode) {
				gameMode->GetServerCoor(m_pos.x, m_pos.z, m_sx, m_sy);
				m_sx /= 2;
				m_sy /= 2;
			}
			break;
		}
		default:
			CAbleToMakeEffect::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

CNpc :: CNpc() : CGameActor()
{
	OnInit();
}


CNpc :: ~CNpc()
{
	OnExit();
}

void	CNpc :: OnInit()
{
	m_gage = NULL;
	m_gage2 = NULL;
	m_playerIswithinBox = false;
	m_lastEffectLaunchTick = timeGetTime();
	m_doesLaunchEffect = FALSE;
}

void	CNpc :: OnExit()
{
	if (m_gage) {
		g_windowMgr.PostQuit(m_gage);
		m_gage = NULL;
	}
	if (m_gage2) {
		g_windowMgr.PostQuit(m_gage2);
		m_gage2 = NULL;
	}
}

void	CNpc :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	switch (message) {
		case	AM_INITSTANDENTRY: {
			CGameActor::SendMsg(sender, message, arg1, arg2, arg3);
			STANDENTRYSTRUCT *	s = (STANDENTRYSTRUCT *)arg1;
			m_isVisible = g_session.IsVisibleNPC((JOBTYPE)s->job);
			SetGuildInfo(s->guildId, s->guildEmblemVersion);
			break;
		}
		case	AM_REMOVEGAGE: {
			if (m_gage2) {
				g_windowMgr.PostQuit(m_gage2);
				m_gage2 = NULL;
			}
			break;
		}
		case	AM_UPDATEGAGE: {
			if (!m_gage2) {
				m_gage2 = new UIPcGage();
				m_gage2->Create(60, 5);
				g_windowMgr.AddWindowFront(m_gage2);
			}
			m_gage2->SetHp(arg1, arg2);
			m_gage2->SetSp(0, 0);
			break;
		}
		case	AM_UPDATEGAGE_HP: {
			if (m_gage2)
				m_gage2->SetHp(arg1, arg2);
			break;
		}
		case	AM_UPDATEGAGE2: {
			if (!m_gage) {
				m_gage = new UIPlayerGage();
				if (arg1 == 0)
					m_gage->m_isPlayerType = 1;
				else if (arg1 == 1)
					m_gage->m_isPlayerType = 2;
				m_gage->Create(60, 9);
				g_windowMgr.AddWindowFront(m_gage);
			}
			m_gage->Move(m_lastTlvertX-30, g_renderer->m_height*12/480 + m_lastTlvertY);
			break;
		}
		default:
			CGameActor::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

void	CNpc :: SetSprJob(int job)
{
	char	sprName[128];
	char	actName[128];

	m_shadowZoom = g_session.GetShadowFactorWithJob(job);
	if (job < JT_MON_BEGIN) {
		sprintf(sprName, "NPC\\%s.spr", g_session.m_jobNameTable[job]);
		sprintf(actName, "NPC\\%s.act", g_session.m_jobNameTable[job]);
	} else if (job >= JT_MER_ARCHER01 && job <= JT_MER_SWORDMAN10) {
		sprintf(sprName, "\xc0\xce\xb0\xa3\xc1\xb7\\\xb8\xf6\xc5\xeb\\%s.spr", g_session.m_jobNameTable[job]);
		sprintf(actName, "\xc0\xce\xb0\xa3\xc1\xb7\\\xb8\xf6\xc5\xeb\\%s.act", g_session.m_jobNameTable[job]);
	} else if (job > JT_MER_BEGIN && job < JT_MER_LAST) {
		sprintf(sprName, "homun\\%s.spr", g_session.m_jobNameTable[job]);
		sprintf(actName, "homun\\%s.act", g_session.m_jobNameTable[job]);
	} else {
		sprintf(sprName, "\xb8\xf3\xbd\xba\xc5\xcd\\%s.spr", g_session.m_jobNameTable[job]);
		sprintf(actName, "\xb8\xf3\xbd\xba\xc5\xcd\\%s.act", g_session.m_jobNameTable[job]);
	}

	SetSprName(sprName);
	SetActName(actName);
}

void	CNpc :: SetSprAct(int job, int sex)
{
	SetJob(job);
	SetSex(sex);

	char	palName[64];
	m_bodyPalette = 0;
	if (g_session.IsHaveAnotherPalette(GetJob(), palName)) {
		m_bodyPalette = 1;
		m_bodyPaletteName = palName;
	}
}

void  CNpc :: SetSprJobDye(int job) {}

void	CNpc :: SetState(int stateId)
{
	if (stateId == ST_DAMAGE)
	{
		if (m_stateId == ST_MOVE)
			m_stateId = ST_STAND;
	}
	else if (stateId == -1)
	{
		return;
	}

	switch (GetJob())
	{
		case	JT_HIDDEN_NPC:
			{
			m_xSize *= 2;
			m_ySize *= 2;
			break;
		}
		case	JT_EFFECTLAUNCHER:
			{
			m_pos.y -= m_ySize * 5;
			break;
		}
		case	JT_HIDDEN_WARP_NPC:
			break;
		case	JT_WARPNPC:
			{
			if (m_ySize <= 10) {
				m_ySize = 10;
			}
			if (m_xSize <= 10) {
				m_xSize = 10;
			}
			m_x1 = m_pos.x - m_xSize/2;
			m_z1 = m_pos.z - m_ySize/2;
			m_x2 = m_pos.x + m_xSize/2;
			m_z2 = m_pos.z + m_ySize/2;
			if (!m_doesLaunchEffect && m_isVisibleBody)
			{
				m_doesLaunchEffect = TRUE;
#ifdef SECONDJOB_SKILL2
				LaunchEffect(EF_WARPZONE2);
#else
				LaunchEffect(EF_WARPZONE);
#endif
			}
			break;
		}
		default:
			if (GetJob()<JT_MON_BEGIN)
			{
				SetAction(0, 4, MT_LOOP);
				RegisterPos();
			} else {
				if (m_stateId == ST_DAMAGE) {
					m_pos.x = m_damageDestX;
					m_pos.z = m_damageDestZ;
				}

				switch (stateId) {
					case	ST_STAND:
						m_isLieOnGround = false;
						m_shouldAddPickInfo = true;
						SetAction(0, 4, MT_LOOP);

						if (m_pkState) {
							int talk, hunger;
							hunger = g_session.GetPetHungryState(g_session.m_petHungry);
							switch(m_pkState) {
								case	1:
									talk = g_session.GetPetTalkNumber(GetJob(), PM_PERFORMANCE1, hunger);
									SetAction(40, 4, MT_ONETIME);
									break;
								case	2:
									talk = g_session.GetPetTalkNumber(GetJob(), PM_PERFORMANCE2, hunger);
									SetAction(48, 4, MT_ONETIME);
									break;
								case	3:
									talk = g_session.GetPetTalkNumber(GetJob(), PM_PERFORMANCE3, hunger);
									SetAction(56, 4, MT_ONETIME);
									break;
								case	4:
									talk = g_session.GetPetTalkNumber(GetJob(), PM_PERFORMANCE_S, hunger);
									SetAction(64, 4, MT_ONETIME);
									break;
								default:
									SetAction(40, 4, MT_ONETIME);
							}

							if (m_gid == g_session.m_petGID)
								g_modeMgr.GetGameMode()->SendMsg(MM_PET_ACT, talk);
							m_pkState = 0;
							m_isMotionFinished = false;
						} else {
							if (m_stateId != ST_STAND)
								m_petEmotionStartTick = timeGetTime();
						}

						RegisterPos();
						break;
					case	ST_MOVE:
						if (m_bodyState != BODYSTATE_STONECURSE && m_bodyState != BODYSTATE_FREEZING)
							m_isMotionFreezed = false;
						if ( GetJob() >= JT_MER_ARCHER01 && GetJob() <= JT_MER_SWORDMAN10 )
							SetAction(8, 6, MT_LOOP);
						else
							SetAction(8, 4, MT_LOOP);
						UnRegisterPos();
						break;

					case	ST_SKILL:
					#if defined (_MER_R2)
						if ( m_objectType == NPC_MERSOL_TYPE )
							SetAction(16, 4, MT_ONETIME);
					#endif

					#if defined (_HOMUN)
						if ( GetJob() >= JT_MER_LIF && GetJob() <= JT_MER_VANILMIRTH_H2 )
							SetAction(40, 4, MT_ONETIME);
						else if ( GetJob() >= JT_MER_ARCHER01 && GetJob() <= JT_MER_SWORDMAN10 )
							SetAction(96, 6, MT_ONETIME);
					#endif
						break;
					case	ST_ATTACK:{
						if (m_bodyState != BODYSTATE_STONECURSE && m_bodyState != BODYSTATE_FREEZING)
							m_isMotionFreezed = false;

			#if defined (_HOMUN)
						if ( GetJob() >= JT_MER_LIF && GetJob() <= JT_MER_VANILMIRTH_H2 )
						{
							if ( rand ( ) % 2 )
								SetAction(16, 4, MT_ONETIME);
							else
								SetAction(48, 4, MT_ONETIME);
						}
						else if ( GetJob() >= JT_MER_ARCHER01 && GetJob() <= JT_MER_SWORDMAN10 )
						{
							SetAction(80, 3, MT_ONETIME);
						}
						else
			#endif
							SetAction(16, 4, MT_ONETIME);

						{
							CGameMode *		gameMode = g_modeMgr.GetGameMode();
							if (m_targetGid == g_session.GetAid()) {
								vector3d &pPos = gameMode->GetWorld()->m_player->m_pos;
								CalcDir(pPos.x, pPos.z);
							} else {
								CGameActor * targetActor = gameMode->GetWorld()->GetGameActorByAID(m_targetGid);
								if (targetActor)
									CalcDir(targetActor->m_pos.x, targetActor->m_pos.z);
							}
						}

						m_attackMotion = GetAttackMotion();
						RegisterPos();
						MakeSpecialAttackEffect(m_targetGid, m_attackMotion * m_motionSpeed);

						break;
					}
					case	ST_DAMAGE:
#ifdef		SECONDJOB_SKILL2
						if (m_isForceState)
							return;
						if (m_isForceState2)
							return;
						if (m_isForceState3)
							return;
#endif
#ifndef	MOB_SKILL
						SendMsg(NULL, AM_DELETE_SKILL_RECHARGEGAGE);
#endif

						if ( GetJob() >= JT_MER_ARCHER01 && GetJob() <= JT_MER_SWORDMAN10 )
							SetAction(48, 5, MT_ONETIME);
						else if (m_curAction < 40)
						{
							SetAction(24, 4, MT_ONETIME);
						}
						else
						{
							ProcessMotion();
							return;
						}
						break;
					case	ST_DEAD:
#ifdef		SECONDJOB_SKILL2
						m_isForceState = false;
						m_isForceState2 = false;
						m_isForceState3 = false;
#endif
						m_isMotionFreezed = false;
						if (m_bodyState == BODYSTATE_FREEZING) {
							SendMsg(NULL, AM_DELETE_ICEEFFECT, 0, 0, 0);
							SendMsg(NULL, AM_MAKE_ICEBREAKE_FFECT, 0, 0, 0);
						}
						m_isLieOnGround = true;
						m_isMotionFreezed = false;
						m_shouldAddPickInfo = false;
						if ( GetJob() >= JT_MER_ARCHER01 && GetJob() <= JT_MER_SWORDMAN10 )
							SetAction(64, 4, MT_ONETIME);
						else
							SetAction(32, 8, MT_ONETIME);
						UnRegisterPos();
						break;
					case	ST_SLEEP:
						if (m_bodyState == BODYSTATE_FREEZING) {
							SendMsg(NULL, AM_DELETE_ICEEFFECT, 0, 0, 0);
							SendMsg(NULL, AM_MAKE_ICEBREAKE_FFECT, 0, 0, 0);
						}
						m_isLieOnGround = false;
						m_isMotionFinished = true;
						m_isMotionFreezed = true;
						SetAction(0, 8, MT_ONETIME);
						break;
				}
			}
			break;
	}
	m_curAction = m_baseAction + Get8Dir(m_roty);
	if (stateId != ST_DAMAGE)
		m_stateId = stateId;
	m_stateStartTick = timeGetTime();
}

void	CNpc :: RegisterPos()
{
	switch (GetJob()) {
		case	JT_WARPNPC:
		case	JT_EFFECTLAUNCHER:
		case	JT_HIDDEN_NPC:
		case	JT_HIDDEN_WARP_NPC:
			break;
		default: {
			int		sx, sy;
			CGameMode *		gameMode = g_modeMgr.GetGameMode();
			gameMode->GetServerCoor(m_pos.x, m_pos.z, sx, sy);
			gameMode->SendMsg(MM_ADDREGISTERPOS, m_gid, sx, sy);
		}
			break;
	}
}

void	CNpc :: UnRegisterPos()
{
	switch (GetJob()) {
		case	JT_WARPNPC:
		case	JT_EFFECTLAUNCHER:
		case	JT_HIDDEN_NPC:
		case	JT_HIDDEN_WARP_NPC:
			break;
		default: {
			CGameMode *		gameMode = g_modeMgr.GetGameMode();
			gameMode->SendMsg(MM_DELETEREGISTERPOS, m_gid);
		}
		break;
	}
}

void	CNpc :: ProcessGage()
{
	if (m_gage) {
		m_gage->Move(m_lastTlvertX - 30,
			(int)((float)(g_renderer->m_height * 12 / 480) * m_lastPixelRatio + m_lastTlvertY));
		m_gage->Invalidate();
	}
	if (m_gage2) {
		m_gage2->Move(m_lastTlvertX - 30,
			(int)((float)(g_renderer->m_height * 12 / 480) * m_lastPixelRatio + m_lastTlvertY + 10.0f));
		m_gage2->Invalidate();
	}
}

void	CNpc :: ProcessLaunchEffect()
{
	DWORD	&effectType = m_speed;
	int		tickOfcycle = m_xSize*1000;

	if (timeGetTime() > m_lastEffectLaunchTick+tickOfcycle) {
		m_lastEffectLaunchTick = timeGetTime();
		LaunchEffect(effectType);
	}
}

void	CNpc :: ProcessContact()
{
	CGameMode *		gameMode = g_modeMgr.GetGameMode();

	CPlayer *		player = gameMode->GetWorld()->m_player;
	float &			px = player->m_pos.x;
	float &			pz = player->m_pos.z;
	if (px>m_x1 && px<m_x2 && pz>m_z1 && pz<m_z2)
	{
		if (!m_playerIswithinBox)
		{
			CONTACTNPC	s;
			s.nid	= m_gid;
			s.job	= GetJob();
			s.type	= 1;
			gameMode->SendMsg(10, int(&s), 0, 0);
		}
		m_playerIswithinBox = true;
	} else {
		m_playerIswithinBox = false;
	}
}

bool	CNpc :: OnProcess()
{
	if (!ProcessVanish())
		return	false;
	if (m_isSingleColor) {
		if (timeGetTime() > m_singleColorStartTick+100) {
			m_colorOfSingleColor = 0xffff0000;
			m_isSingleColor = false;
		}
	}
	bool	result = true;
	switch (GetJob()) {
		case	JT_HIDDEN_NPC: {
			ProcessWillBeAttacked();
			ProcessChatBalloon();
			result = ProcessState();
			break;
		}
		case	JT_EFFECTLAUNCHER: {
			ProcessLaunchEffect();
			break;
		}
		case	JT_HIDDEN_WARP_NPC:
		case	JT_WARPNPC:
			break;
		default: {
			ProcessBodyState();
			ProcessEffectState();
			ProcessWillBeAttacked();
			if (m_sprArgb != 0xffffffff)
				ModifyArgb(8, 8, 8, 8);
			ProcessChatBalloon();
			result = ProcessState();
			break;
		}
	}
	if (GetHeadType() != HEADTYPE_LAST)
		ProcessSound();

	if (g_session.m_homunGID == m_gid) {
		if (m_homunAI) {
			if (m_homunAI->IsOpened())
				m_homunAI->Proc(m_gid);
			else {
				if (g_session.m_onHoUserAI)
					m_homunAI->Open("./AI/USER_AI/AI.lua");
				else
					m_homunAI->Open("./AI/AI.lua");
				SendMsg(NULL, AM_UPDATEGAGE2, MT_HOMUN, 0, 0);
			}
		}
		else
			m_homunAI = new CMercenaryAI(MT_HOMUN);
	}

	if (g_session.m_merGID == m_gid) {
		if (m_merAI) {
			if (m_merAI->IsOpened())
				m_merAI->Proc(m_gid);
			else {
				if (g_session.m_onMerUserAI)
					m_merAI->Open("./AI/USER_AI/AI_M.lua");
				else
					m_merAI->Open("./AI/AI_M.lua");
				SendMsg(NULL, AM_UPDATEGAGE2, MT_MER, 0, 0);
			}
		}
		else
			m_merAI = new CMercenaryAI(MT_MER);
	}

	ProcessGage();

	if (g_session.m_homunAlive && g_session.m_homunEnemyGID == m_gid)
		SendMsg(NULL, AM_MAKE_CURSOR_EFFECT, 0, 0, 0);
	else if (g_session.m_merAlive && g_session.m_merEnemyGID == m_gid)
		SendMsg(NULL, AM_MAKE_CURSOR_EFFECT, 0, 0, 0);
	else
		SendMsg(NULL, AM_DELETE_CURSOR_EFFECT, 0, 0, 0);

	return	result;
}

void	CNpc :: Render(matrix &vtm)
{
	switch (GetJob())
	{
		case	JT_HIDDEN_NPC:
			{
			float		x, y, oow;
			g_renderer->ProjectVertex(m_pos, vtm, &x, &y, &oow);

			float &sXf = g_renderer->m_screenXFactor;
			float &sYf = g_renderer->m_screenYFactor;
			float	pixelRatio = g_renderer->GetPixelRatio(oow);
			m_lastPixelRatio = pixelRatio / g_avgPixelRatio;

			RENDER_INFO_RECT	info = {	x - float(m_xSize/2)*sXf*m_lastPixelRatio,
											y - float(m_ySize/2)*sYf*m_lastPixelRatio,
											x + float(m_xSize/2)*sXf*m_lastPixelRatio,
											y,
											oow };

			SetRenderInfo(info, x, y);

			#ifdef _DEBUG
				RenderShadow(vtm, m_zoom*3);
			#endif
			break;
		}
		case	JT_EFFECTLAUNCHER:
		{
			break;
		}
		case	JT_HIDDEN_WARP_NPC:
			#ifdef LOCAL_SERVER
			#endif
			break;
		case	JT_WARPNPC: {
			if (m_isVisibleBody)
			{
				float			x, y, oow;
				g_renderer->ProjectVertex(m_pos, vtm, &x, &y, &oow);

				float &		sXf = g_renderer->m_screenXFactor;
				float &		sYf = g_renderer->m_screenYFactor;

				float		pixelRatio = g_renderer->GetPixelRatio(oow);
				m_lastPixelRatio = pixelRatio / g_avgPixelRatio;
				RENDER_INFO_RECT info = {	x - 50*sXf*m_lastPixelRatio,
											y - 50*sYf*m_lastPixelRatio,
											x + 50*sXf*m_lastPixelRatio,
											y + 50*sYf*m_lastPixelRatio,
											oow };
				SetRenderInfo(info, x, y);
				if (!g_session.m_isEffectOn)
					RenderShadow(vtm, m_zoom * 3);
			} else {
				UpdateTlvertex(vtm);
			}
			break;
		}
		default:
		{
			char PASS;
			PASS=0;

			if (GetJob() >= JT_MER_ARCHER01 && GetJob() <= JT_MER_SWORDMAN10)
			{
				if (m_isVisible)
				{
					if ((m_sprArgb&0xff000000) == 0xff000000)
						RenderShadow(vtm, m_zoom);

					if (m_isVisibleBody)
					{
						RenderMercenary(vtm, GetJob()) ;
					}
					else
					{
						UpdateTlvertex(vtm);
					}
				}
				break;
			}

			for (int a=0; a<stateId.size(); a++)
			{
				if (stateId[a] == EFST_CLAIRVOYANCE)
					PASS = 1;
			}

			if ( PASS == 1 && ( m_isVisible == 0 || m_isVisibleBody == 0 ) )
			{
				SetArgb(-1, 0, 0, 0);
				if (GetJob() >= JT_MON_BEGIN && GetJob() < JT_MONSTER_LAST)
					RenderBody(vtm,1);
				else
					RenderBody(vtm,0);
			}
			else if (m_isVisible)
			{
				if ((m_sprArgb&0xff000000) == 0xff000000)
					RenderShadow(vtm, m_zoom);

				if (m_isVisibleBody)
				{
					if (GetJob() >= JT_MON_BEGIN && GetJob() < JT_MONSTER_LAST)
						RenderBody(vtm,1);
					else
						RenderBody(vtm,0);
				}
				else
					UpdateTlvertex(vtm);
			}
			break;
		}
	}

}
