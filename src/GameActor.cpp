#include "common.h"
#include "framework/Locale.h"
#include <math.h>
#include <algorithm>
#include "resource/Wave.h"
#include "pdb_enums.h"
#include "GameActor.h"
#include "Picker.h"



#ifndef PACK555TO32
#define PACK555TO32(argb)       ( 0xFF000000 + ((argb&0x7C00)<<9) + ((argb&0x03E0)<<6) + ((argb&0x0001F)<<3) )
#endif


float g_zOffset = 0.000009f;
float g_zHover = 0.000036f;
float g_avgPixelRatio = 1.34f;




namespace {
const int BL_LIGHT_BODY  = 0x01;
const int BL_LIGHT_SWORD = 0x02;
const int BL_BABY_BODY    = 0x10;
const int BL_DOUBLE_BODY  = 0x08;
const int BL_PRESSED      = 0x20;
const int BL_KICKED       = 0x80;
const int BL_HIT          = 0x100;
const int BL_REFLECT_BODY = 0x200;
const int BL_GIANT        = 0x400;
const int BL_ASURA        = 0x800;
const int BL_4WAY         = 0x1000;
const int BL_QUAKE        = 0x2000;
const int BL_UNDEAD       = 0x80000;
const int BL_HIT_BLUE     = 0x4000;
const int BL_HIT_RED      = 0x40000;

const int Layer_Body       = 0;
const int Layer_Head       = 1;
const int Layer_Accessory1 = 2;
const int NEVERSEE_DAMAGEVALUE = -29999;
const int MAKETYPE_NEWCART_START = 36;

inline bool IsPetHeadType(CGameActor* actor) { return actor->GetHeadType() == HEADTYPE_LAST; }
}

#ifndef PATH_HUMAN
#define PATH_HUMAN "\xc0\xce\xb0\xa3\xc1\xb7\\"
#endif

static inline float GetShadowOow(float sprY, float tlvertY, float tvz)
{
	return 1.0f / (-g_shadowSlope*(sprY-tlvertY) + tvz);
}

static inline float GetSprOow(float sprY, float tlvertY, float tvz)
{
	return 1.0f / (g_slope*(sprY-tlvertY) + tvz);
}
#include "MsgStrings.h"
#include "window/UIFrameWnd.h"
#include "GameMode.h"
const MAKE_MSG_EFFECT_TYPE MAKETYPE_TOTAL_CRITICAL_EFFECT = (MAKE_MSG_EFFECT_TYPE)13;
#include "View.h"
#include "PathFinder.h"
#include "RagEffect.h"
#include "RagEffectPrim.h"
#include "MercenaryAI.h"
#include "Session.h"

#include "Mode.h"
#include "resource/Sprite.h"
#include "resource/Action.h"
#include "resource/Attr.h"
#include "resource/PaletteRes.h"
#include "3dlib/Device.h"
#include "base/Res.h"
#include "GameWorld.h"
#include "resource/Ground.h"
#include "3dGround.h"
#include "device/Connection.h"

extern UIWindowMgr g_windowMgr;
extern CModeMgr g_modeMgr;
extern CSession g_session;
extern CRenderer* g_renderer;

extern BOOL g_isSkipFrame;
extern int durationTable[];

bool IsEffectStateChicken(DWORD effectState);
bool IsEffectStateNothing(DWORD effectState);
bool IsEffectStateSight(DWORD effectState);
bool IsEffectStateRuwach(DWORD effectState);
bool IsEffectStateStar2(DWORD effectState);
bool IsEffectStateHiding(DWORD effectState);
bool IsEffectStateBurrow(DWORD effectState);
bool IsEffectStatePushCart(DWORD effectState);
bool IsEffectStateBird(DWORD effectState);
bool IsEffectStateSpecialHiding(DWORD effectState);
bool IsEffectStateSightChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateRuwachChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateBurrowChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateHidingChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStatePushCartChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateBirdChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateChickenChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateSpecialHidingChanged(DWORD effectState, DWORD oldEffectState);
bool IsEffectStateStar2Changed(DWORD effectState, DWORD oldEffectState);
extern int g_se_go;
extern char g_se_type;
extern int g_isBlackWorld;
extern int g_isPoison;
extern char g_FirstEffect;


void RENDER_INFO_RECT :: SetInfo(float fleft, float ftop, float fright, float fbottom, float foow) {

	RENDER_INFO_RECT::left = fleft;
	RENDER_INFO_RECT::right = fright;
	RENDER_INFO_RECT::top = ftop;
	RENDER_INFO_RECT::bottom = fbottom;
	RENDER_INFO_RECT::oow = foow;
}

void RENDER_INFO_RECT :: Update(const RPSprite& spr) {

	float sprLeft = (spr.left<=spr.right)?spr.left:spr.right;

	float sprRight = (spr.left>spr.right)?spr.left:spr.right;

	if (sprLeft < left) left = sprLeft;

	if (spr.top < top) top = spr.top;

	if (sprRight > right) right = sprRight;

	if (spr.bottom > bottom) bottom = spr.bottom;
}

CGameObject :: CGameObject()
{
	OnInit();
}


CGameObject :: ~CGameObject()
{
	OnExit();
}

void	CGameObject :: OnInit()
{


}

void	CGameObject :: OnExit()
{
}

bool	CGameObject :: OnProcess()
{
	return true;
}

int		CGameObject :: Get8Dir(float rot)
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	int		dir = 0;
	if (gameMode) {
		float	longitude =  -gameMode->GetView()->GetCurLongitude() - rot;
		while (longitude < 0)
			longitude += 360;
		while (longitude >= 360)
			longitude -= 360;

		dir = (longitude)/45;
	}
	return	dir;
}

int		CGameObject :: Get8Dir2(float rot, int isReal8Dir)
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	int		dir = 0;
	if (gameMode) {
		float	longitude =  -gameMode->GetView()->GetCurLongitude() - rot;
		while (longitude < 0)
			longitude += 360;
		while (longitude >= 360)
			longitude -= 360;

		if (isReal8Dir) {
			if (longitude < 22.5 || longitude > 360-22.5)
				dir = 0;
			else
				dir = (longitude+22.5)/45;
		} else {
			dir = (longitude)/45;
		}
	}
	return	dir;
}

int		CGameObject :: GetDir(float rot)
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	int	longitude = 0;
	if (gameMode) {
		longitude =  -gameMode->GetView()->GetCurLongitude() - rot;
		while (longitude < 0)
			longitude += 360;
		while (longitude >= 360)
			longitude -= 360;
	}
	return longitude;
}

void	CGameObject :: SendMsg(CGameObject *sender, int message, int arg1, int arg2, int arg3)
{
	switch (message) {
		default:
			break;
	}
}

CRenderObject :: CRenderObject()
{
	m_sprRes = NULL;
	m_actRes = NULL;
	OnInit();
}


CRenderObject :: ~CRenderObject()
{
	if(m_sprRes) m_sprRes->Unlock();
	if(m_actRes) m_actRes->Unlock();

	OnExit();
}

void	CRenderObject :: OnInit()
{
	m_pos.Set(0,0,0);
	m_baseAction = 0;
	m_oldBaseAction = m_curAction = 0;
	m_oldMotion = m_curMotion = 0;
	m_roty = 0;
	m_zoom = 1.0f;
	m_shadowZoom = 1.0f;
	m_motionType = 0;
	m_motionSpeed = 1.0f;
	m_stateId = ST_STAND;
	m_isVisible = true;
	m_isVisibleBody = true;
	m_sprColor.color = 0xFFFFFFFF;
	m_alwaysTopLayer = false;
	m_shadowOn = false;
	m_isMotionFinished = false;
	m_isMotionFreezed = false;
	m_shouldAddPickInfo = false;
	m_sprShift = 0;
	m_sprAngle = 0;
	m_singleColorStartTick = timeGetTime();
	m_isSingleColor = false;
	m_isLieOnGround = false;
	m_isSprArgbFixed = false;
	m_colorOfSingleColor = 0xffff0000;
	m_offsetOow = 0;
	m_bodyPalette = 0;
	m_lastPixelRatio = 1;
	m_loopCountOfmotionFinish = 1;
	m_modifyFactorOfmotionSpeed = 1;

	m_isPc = FALSE;

	char sprName[128];
	char actName[128];

	sprintf(sprName, "NPC\\%s.spr", g_session.m_jobNameTable[JT_1_ETC_01]);
	sprintf(actName, "NPC\\%s.act", g_session.m_jobNameTable[JT_1_ETC_01]);

	SetSprName(sprName);
	SetActName(actName);

	m_oldColor.a = 0;
	m_oldColor.r = 0;
	m_oldColor.g = 0;
	m_oldColor.b = 0;

	m_isForceState = FALSE;
	m_isForceState2 = FALSE;
	m_isForceState3 = FALSE;
	m_forceAnimSpeed = 10;

	m_BeZero = 1;
	m_BodyLight = 0;
	m_BodyFlag = 0;
	m_BodySin = 0;
	m_BodySin2 = 0;
	m_BodySin3 = 0;
	m_BodySin4 = 0;
	m_BodySin5 = 0;
	m_BodyTime = 0;
	m_BodyTime2 = 0;

	m_FlyMove = 0;
	m_FlyNow = 0;

	m_BodyAni = 0;
	m_BodyAct = 0;
	m_BodyAniFrame = 0;

	m_charfont = 0;
}

void	CRenderObject :: OnExit()
{
}

float	CRenderObject :: CalcDist(float x, float z)
{
	float	dx = x-m_pos.x;
	float	dz = z-m_pos.z;

	return	_sqrt(dx*dx + dz*dz);
}

float	CRenderObject :: CalcDist(float x, float y, float z)
{
	float	dx = x-m_pos.x;
	float	dz = z-m_pos.z;
	float	dy = y-m_pos.y;
	float	dxz = _sqrt(dx*dx + dz*dz);

	return	_sqrt(dxz*dxz + dy*dy);
}

void	CRenderObject :: CalcDir(float x, float z)
{
	float	dx = x - m_pos.x;
	float	dz = z - m_pos.z;
	m_roty = RadToDeg(atan2(dx, -dz));
	if (m_roty >= 360)
		m_roty -= 360;
	if (m_roty < 0)
		m_roty += 360;
}

float	CRenderObject :: CalcDir(float sx, float sz, float x, float z)
{
	float	dx = x - sx;
	float	dz = z - sz;
	float	roty = RadToDeg(atan2(dx, -dz));
	if (roty >= 360)
		roty -= 360;
	if (roty < 0)
		roty += 360;
	return roty;
}

void	CRenderObject :: ModifyArgb(int a, int r, int g, int b)
{
	if (!m_isSprArgbFixed) {

		int	m_A=m_sprColor.a;
		int	m_R=m_sprColor.r;
		int	m_G=m_sprColor.g;
		int	m_B=m_sprColor.b;

		m_A+=a;	m_R+=r;	m_G+=g;	m_B+=b;

		m_sprColor.a = (m_A > 0 ? m_A : 0) < 255 ? (m_A > 0 ? m_A : 0) : 255;
		m_sprColor.r = (m_R > 0 ? m_R : 0) < 255 ? (m_R > 0 ? m_R : 0) : 255;
		m_sprColor.g = (m_G > 0 ? m_G : 0) < 255 ? (m_G > 0 ? m_G : 0) : 255;
		m_sprColor.b = (m_B > 0 ? m_B : 0) < 255 ? (m_B > 0 ? m_B : 0) : 255;
	}
}

void	CRenderObject :: SetArgb(int a, int r, int g, int b)
{
	if (a != -1) m_sprColor.a = a;
	if (r != -1) m_sprColor.r = r;
	if (g != -1) m_sprColor.g = g;
	if (b != -1) m_sprColor.b = b;
}

bool	CRenderObject :: GetTargetPos(DWORD targetGid, vector3d &tPos)
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	bool tPosExist = true;
	if (targetGid == g_session.GetAid()) {
		tPos = gameMode->GetWorld()->m_player->m_pos;
	} else {
		CGameActor * theActor = gameMode->GetWorld()->GetGameActorByAID(targetGid);
		if (theActor)
			tPos = theActor->m_pos;
		else {
			tPosExist = false;
			tPos = m_pos - (gameMode->GetWorld()->m_player->m_pos - m_pos);
		}
	}
	return tPosExist;
}

void	CRenderObject :: ProcessMotion()
{
	int dir = Get8Dir(m_roty);
	m_curAction = m_baseAction + dir;
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);

	float stateCnt = float(timeGetTime() - m_stateStartTick) / 24.0f;
	CActRes *	actRes = GetActRes();

	if(!actRes)
	{
		return;
	}

#ifdef	SECONDJOB_SKILL2
	if (m_isForceState2)
	{

		m_forceAct = m_forceAct2[m_forceStartMot];
		m_forceMot = m_forceMot2[m_forceStartMot];

		if (m_isForceAnimFinish)
		{
			m_baseAction = m_forceAct2[4];
			m_curAction = m_forceAct2[4] + Get8Dir(m_roty);
			m_curMotion = m_forceMot2[4];
			if (!m_forceStateEndTick) {
				m_isForceState2 = FALSE;
				m_motionType = MT_ONETIME;
			}
		}
		else
		{
			m_baseAction = m_forceAct;
			m_curAction = m_forceAct + Get8Dir(m_roty);
			m_curMotion = m_forceMot;
		}

		if (!m_isForceAnimation)
			return;
		DWORD debugTime = timeGetTime();
		if (timeGetTime() >= m_forceStateEndTick)
		{
			m_isForceState2 = FALSE;
			m_forceStateEndTick = 0;
			m_motionType = MT_ONETIME;
			m_isMotionFinished = TRUE;
		}

		if (!m_isForceAnimFinish && m_forceStateCnt % m_forceAnimSpeed == 0)
		{
			m_forceStartMot++;
		}

		if (m_forceStartMot > m_forceMaxMot)
		{
			if (m_isForceAnimLoop)
				m_forceStartMot = 0;
			else
			{
				m_forceStartMot = m_forceMaxMot;
				m_isForceAnimFinish = TRUE;
			}
		}

		m_forceStateCnt++;
	}
	else if (m_isForceState3)
	{
		if (m_isForceAnimFinish)
		{
			m_baseAction = m_forceFinishedAct;
			m_curAction = m_forceFinishedAct + Get8Dir(m_roty);
			m_curMotion = m_forceFinishedMot;
			if (!m_forceStateEndTick)
			{
				m_isForceState = FALSE;
				m_motionType = MT_ONETIME;
			}
		}
		else
		{
			m_baseAction = m_forceAct;
			m_curAction = m_forceAct + Get8Dir(m_roty);
			m_curMotion = m_forceMot;
		}

		if (!m_isForceAnimation)
			return;
		DWORD debugTime = timeGetTime();

		if (timeGetTime() >= m_forceStateEndTick)
		{
			m_isForceState = FALSE;
			m_forceStateEndTick = 0;
			m_motionType = MT_ONETIME;
			m_isMotionFinished = TRUE;
		}

		if (!m_isForceAnimFinish)
		{
			if (m_BodyAni == 3)
				m_forceMot = int(stateCnt*2/m_motionSpeed)-(m_forceMaxMot+1);
			else
				m_forceMot = int(stateCnt*2/m_motionSpeed);
		}

		if (m_forceMot > m_forceMaxMot)
		{
			if (m_BodyAni == 2)
			{
				m_BodyAni = 3;
				m_forceMot = 0;
			}
			else
			{
				m_BodyAni = 0;
				m_forceMot = m_forceMaxMot;
				m_isForceAnimFinish = TRUE;
			}
		}
		m_forceStateCnt++;
	}
	else if (m_isForceState)
	{
		if (m_isForceAnimFinish) {
			m_baseAction = m_forceFinishedAct;
			m_curAction = m_forceFinishedAct + Get8Dir(m_roty);
			m_curMotion = m_forceFinishedMot;
			if (!m_forceStateEndTick) {
				m_isForceState = FALSE;
				m_motionType = MT_ONETIME;
			}
		} else {
			m_baseAction = m_forceAct;
			m_curAction = m_forceAct + Get8Dir(m_roty);
			m_curMotion = m_forceMot;
		}

		if (!m_isForceAnimation)
			return;
		DWORD debugTime = timeGetTime();
		if (timeGetTime() >= m_forceStateEndTick) {
			m_isForceState = FALSE;
			m_forceStateEndTick = 0;
			m_motionType = MT_ONETIME;
			m_isMotionFinished = TRUE;
		}

		if (!m_isForceAnimFinish && m_forceStateCnt % m_forceAnimSpeed == 0)
		{
			if ( m_isForceAnimLoop && m_forceFinishedAct == 0 && m_forceFinishedMot != 0)
			{
				m_forceMot--;
				m_forceFinishedMot = 0;
			}
			else
				m_forceMot++;
		}

		if (m_forceMot > m_forceMaxMot) {
			if (m_isForceAnimLoop) {
				if ( m_forceFinishedAct == 0 )
				{
					m_forceMot = m_forceStartMot + 1;
					m_forceFinishedMot = 1;
				}
				else
					m_forceMot = m_forceStartMot;
			} else {
				m_forceMot = m_forceMaxMot;
				m_isForceAnimFinish = TRUE;
			}
		}
		m_forceStateCnt++;
	}
	else
#endif
	{
		if (m_isMotionFinished || m_isMotionFreezed)
		{
			int motionCount = actRes->GetMotionCount(m_curAction);
			if (m_curMotion > motionCount-1)
				m_curMotion = motionCount-1;
			return;
		}
		else
		{
			switch (m_motionType)
			{
			case MT_ONETIME:
				{
					int motionCount = actRes->GetMotionCount(m_curAction);
					m_curMotion = int(stateCnt/m_motionSpeed)%motionCount;
					float loopCount = (stateCnt/m_motionSpeed)/float(motionCount);

					if (loopCount >= 1)
					{
						m_curMotion = motionCount-1;
						if (loopCount >= m_loopCountOfmotionFinish)
							m_isMotionFinished = true;
					}
				}
				break;
			case 0:
				{
					m_curMotion = int(stateCnt/m_motionSpeed)%(actRes->GetMotionCount(m_curAction));
				}
				break;
			}
		}

	}
}

#ifndef BOOL_RANDOM
#define BOOL_RANDOM( num ) random(100)%num==0
#endif
void	CRenderObject :: ProcessSound()
{
	if (m_oldMotion != m_curMotion || m_oldBaseAction != m_baseAction) {
		if (m_oldBaseAction != m_baseAction)
			m_oldMotion = 0;
		CActRes *	actRes = m_actRes;
		int motCnt;
		if (m_oldMotion < m_curMotion)
		{
			for (motCnt=m_oldMotion+1; motCnt<=m_curMotion; motCnt++)
			{
				int eventId = actRes->GetMotion(m_curAction, motCnt)->m_eventId;
				if (eventId != -1)
				{
					std::string eventName = actRes->GetEventName(eventId);
					if (eventName.find(".wav") != -1)
					{
						CGameMode * gameMode = g_modeMgr.GetGameMode();
						sAssert(gameMode);
						vector3d &pPos = gameMode->GetWorld()->m_player->m_pos;
						PlayWave(eventName.c_str(), m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
				}
			}
		} else if (m_curMotion < m_oldMotion )
		{
			int numMotion = actRes->GetMotionCount(m_curAction);
			if (m_oldMotion >= numMotion)
				m_oldMotion = numMotion-1;
			for (motCnt=m_oldMotion+1; motCnt<numMotion; motCnt++)
			{
				int eventId = actRes->GetMotion(m_curAction, motCnt)->m_eventId;
				if (eventId != -1)
				{
					std::string eventName = actRes->GetEventName(eventId);
					if (eventName.find(".wav") != -1 )
					{
						CGameMode * gameMode = g_modeMgr.GetGameMode();
						sAssert(gameMode);
						vector3d &pPos = gameMode->GetWorld()->m_player->m_pos;
						PlayWave(eventName.c_str(), m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
				}
			}

			for (motCnt=0; motCnt<=m_curMotion; motCnt++)
			{
				int eventId = actRes->GetMotion(m_curAction, motCnt)->m_eventId;
				if (eventId != -1)
				{
					std::string eventName = actRes->GetEventName(eventId);
					if (eventName.find(".wav") != -1 )
					{
						CGameMode * gameMode = g_modeMgr.GetGameMode();
						sAssert(gameMode);
						vector3d &pPos = gameMode->GetWorld()->m_player->m_pos;
						PlayWave(eventName.c_str(), m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					}
				}
			}

		}
	}
	m_oldMotion = m_curMotion;
	m_oldBaseAction = m_baseAction;
}

void	CRenderObject :: SetAction(int action, int speed, int type)
{
	m_motionSpeed = m_actRes->GetDelay(action);
	if (m_motionSpeed<1)
		m_motionSpeed=1;
	m_motionSpeed *= m_modifyFactorOfmotionSpeed;
	m_curAction = m_baseAction = action;
	m_curMotion = 0;
	m_motionType = type;
	m_isMotionFinished = false;
	m_loopCountOfmotionFinish = 1;
	m_modifyFactorOfmotionSpeed = 1;
	m_isForceState = FALSE;
	m_isForceState2 = FALSE;
	m_isForceState3 = FALSE;
}

void	CRenderObject :: SendMsg(CGameObject *sender, int message, int arg1, int arg2, int arg3)
{
	switch (message) {
		default:
			CGameObject::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

void	CRenderObject :: SetLmIntensity(float &lmRIntensity, float &lmGIntensity, float &lmBIntensity)
{
	extern int g_isLightMap;
	if (g_isLightMap == FALSE)
		return;

	CGameMode * gameMode = g_modeMgr.GetGameMode();
	C3dGround &ground = *gameMode->GetWorld()->m_ground;

	if (ground.IsNewVer())
	{
		int sx, sy;
		gameMode->GetServerCoor(m_pos.x, m_pos.z, sx, sy);
		ground.GetLmIntensityFloatRGBByGatCoor(sx, sy, lmRIntensity, lmGIntensity, lmBIntensity);
	}
}

void	CRenderObject :: UpdateTlvertex(matrix &vtm)
{
	float		x, y, oow;

	g_renderer->ProjectVertex(m_pos, vtm, &x, &y, &oow);

	SetTlvert(x, y);
}

void	CRenderObject :: RenderMercenary(matrix &vtm,int MerJob)
{
	char ML;
	CActRes *	actRes;
	CSprRes *	sprRes;
	CMotion *	mot;
	CMotion *	headMotion;
	CMotion *	bodyMotion;
	CMotion *	accMotion;

	int	head_imfOffX;
	int	head_imfOffY;
	int	acc_imfOffX;
	int	acc_imfOffY;
	int	total_imfOffX;
	int	total_imfOffY;
	char	temp[256], tmpName[256];

	vector3d	tv;
	tv.MatrixMult(m_pos, vtm);

	float		x, y, oow;
	g_renderer->ProjectVertex(m_pos, vtm, &x, &y, &oow);


	float	pixelRatio = g_renderer->GetPixelRatio(oow);
	m_lastPixelRatio = pixelRatio / g_avgPixelRatio;
	float	fSprShift = m_sprShift*m_zoom*g_renderer->GetScreenXFactor()*m_lastPixelRatio;
	float	zHover = g_zHover * m_lastPixelRatio;

	for (ML=0;ML<8;ML++)
	{
		total_imfOffX = 0;
		total_imfOffY = 0;

		if (ML >= 5)
		{
			if (MerJob == JT_MER_ARCHER10 || MerJob == JT_MER_LANCER10 || MerJob == JT_MER_SWORDMAN10)
			{
			}
			else
				continue;
		}

		if (ML == 0)
		{
			actRes = GetActRes();
			sprRes = GetSprRes();

			mot = actRes->GetMotion(m_curAction, m_curMotion);

			if (m_curAction < 8)
			{
				mot = actRes->GetMotion(m_curAction, 0);
			}

			bodyMotion = mot;
		}
		else if (ML == 1)
		{
			if (MerJob >= JT_MER_ARCHER01 && MerJob <= JT_MER_ARCHER10)
			{

				sprintf(tmpName, "\300\316\260\243\301\267\\\270\323\270\256\305\353\\\277\251\\%d_\277\251.act", (MerJob%23) + 1);
				actRes = (CActRes *)g_resMgr().Get(tmpName, 0);

				sprintf(tmpName, "\300\316\260\243\301\267\\\270\323\270\256\305\353\\\277\251\\%d_\277\251.spr", (MerJob%23) + 1);
				sprRes = (CSprRes *)g_resMgr().Get(tmpName, 0);
			}
			else
			{

				sprintf(tmpName, "\300\316\260\243\301\267\\\270\323\270\256\305\353\\\263\262\\%d_\263\262.act", (MerJob%23) + 1);

				actRes = (CActRes *)g_resMgr().Get(tmpName, 0);

				sprintf(tmpName, "\300\316\260\243\301\267\\\270\323\270\256\305\353\\\263\262\\%d_\263\262.spr", (MerJob%23) + 1);

				sprRes = (CSprRes *)g_resMgr().Get(tmpName, 0);
			}

			mot = actRes->GetMotion(m_curAction, m_curMotion);

			if (m_curAction < 8)
			{
				mot = actRes->GetMotion(m_curAction, 0);
			}

			headMotion = mot;

			if (headMotion->attachCnt && bodyMotion->attachCnt)
			{
				if (headMotion->attachInfo[0].m_attr == bodyMotion->attachInfo[0].m_attr)
				{
					head_imfOffX = bodyMotion->attachInfo[0].x - headMotion->attachInfo[0].x;
					head_imfOffY = bodyMotion->attachInfo[0].y - headMotion->attachInfo[0].y;
				}
			}

			total_imfOffX = head_imfOffX;
			total_imfOffY = head_imfOffY;
		}
		else if (ML == 2)
		{

			if (MerJob >= JT_MER_ARCHER01 && MerJob <= JT_MER_ARCHER10)
			{
				sprintf(tmpName, "\300\316\260\243\301\267\\\277\353\272\264\\\310\260\277\353\272\264_\310\260.act");
				actRes = (CActRes *)g_resMgr().Get(tmpName, 0);

				sprintf(tmpName, "\300\316\260\243\301\267\\\277\353\272\264\\\310\260\277\353\272\264_\310\260.spr");
				sprRes = (CSprRes *)g_resMgr().Get(tmpName, 0);

			}
			else if (MerJob >= JT_MER_LANCER01 && MerJob <= JT_MER_LANCER10)
			{
				sprintf(tmpName, "\300\316\260\243\301\267\\\277\353\272\264\\\303\242\277\353\272\264_\303\242.act");
				actRes = (CActRes *)g_resMgr().Get(tmpName, 0);

				sprintf(tmpName, "\300\316\260\243\301\267\\\277\353\272\264\\\303\242\277\353\272\264_\303\242.spr");
				sprRes = (CSprRes *)g_resMgr().Get(tmpName, 0);
			}
			else
			{
				sprintf(tmpName, "\300\316\260\243\301\267\\\277\353\272\264\\\260\313\277\353\272\264_\260\313.act");
				actRes = (CActRes *)g_resMgr().Get(tmpName, 0);

				sprintf(tmpName, "\300\316\260\243\301\267\\\277\353\272\264\\\260\313\277\353\272\264_\260\313.spr");
				sprRes = (CSprRes *)g_resMgr().Get(tmpName, 0);
			}

			mot = actRes->GetMotion(m_curAction, m_curMotion);
		}
		else if (ML == 3)
		{

			if (MerJob >= JT_MER_ARCHER01 && MerJob <= JT_MER_ARCHER10)
			{
				continue;
			}
			else if (MerJob >= JT_MER_LANCER01 && MerJob <= JT_MER_LANCER10)
			{
				sprintf(tmpName, "\300\316\260\243\301\267\\\277\353\272\264\\\303\242\277\353\272\264_\303\242_\260\313\261\244.act");
				actRes = (CActRes *)g_resMgr().Get(tmpName, 0);
				sprintf(tmpName, "\300\316\260\243\301\267\\\277\353\272\264\\\303\242\277\353\272\264_\303\242_\260\313\261\244.spr");
				sprRes = (CSprRes *)g_resMgr().Get(tmpName, 0);
			}
			else
			{
				sprintf(tmpName, "\300\316\260\243\301\267\\\277\353\272\264\\\260\313\277\353\272\264_\260\313_\260\313\261\244.act");
				actRes = (CActRes *)g_resMgr().Get(tmpName, 0);

				sprintf(tmpName, "\300\316\260\243\301\267\\\277\353\272\264\\\260\313\277\353\272\264_\260\313_\260\313\261\244.spr");
				sprRes = (CSprRes *)g_resMgr().Get(tmpName, 0);
			}

			mot = actRes->GetMotion(m_curAction, m_curMotion);
		}
		else if (ML == 4)
		{
			switch (MerJob)
			{
					case JT_MER_ARCHER01:
						sprintf(temp, "\277\251\\\277\251_\305\320\270\360\300\332"); break;
					case JT_MER_ARCHER02:
						sprintf(temp, "\277\251\\\277\251_\260\361\265\347\307\354\265\345\261\342\276\356"); break;
					case JT_MER_ARCHER03:
						sprintf(temp, "\277\251\\\277\251_\264\353\307\374\261\335\271\346\277\357"); break;
					case JT_MER_ARCHER04:
						sprintf(temp, "\277\251\\\277\251_\277\244\270\256\275\272\270\360\300\332"); break;
					case JT_MER_ARCHER05:
						sprintf(temp, "\277\251\\\277\251_\260\241\272\316\305\260\260\241\270\351"); break;
					case JT_MER_ARCHER06:
						sprintf(temp, "\277\251\\\277\251_\277\251\277\325\300\307\305\365\261\270"); break;
					case JT_MER_ARCHER07:
						sprintf(temp, "\277\251\\\277\251_\301\366\270\243\305\270\275\272\260\241\270\351"); break;
					case JT_MER_ARCHER08:
						sprintf(temp, "\277\251\\\277\251_\275\307\271\366\306\274\276\306\266\363"); break;
					case JT_MER_ARCHER09:
						sprintf(temp, "\277\251\\\277\251_\260\355\276\347\300\314\270\360\300\332"); break;
					case JT_MER_ARCHER10:
						sprintf(temp, "\277\251\\\277\251_\277\271\273\333\270\256\272\273"); break;
					case JT_MER_LANCER01:
						sprintf(temp, "\263\262\\\263\262_\260\243\310\243\270\360"); break;
					case JT_MER_LANCER02:
						sprintf(temp, "\263\262\\\263\262_\260\313\300\272\264\253\260\241\270\256\260\263"); break;
					case JT_MER_LANCER03:
						sprintf(temp, "\263\262\\\263\262_\260\355\272\355\270\260\260\241\270\3514\310\243"); break;
					case JT_MER_LANCER04:
						sprintf(temp, "\263\262\\\263\262_\276\347\270\360\300\332"); break;
					case JT_MER_LANCER05:
						sprintf(temp, "\263\262\\\263\262_\261\315\302\372\300\272\263\312\261\270\270\256"); break;
					case JT_MER_LANCER06:
						sprintf(temp, "\263\262\\\263\262_\265\267\300\322\300\272\300\332\300\307\270\266\300\275"); break;
					case JT_MER_LANCER07:
						sprintf(temp, "\263\262\\\263\262_\270\266\270\256\277\300\263\335\270\360\300\332"); break;
					case JT_MER_LANCER08:
						sprintf(temp, "\263\262\\\263\262_\266\363\300\314\277\302\270\266\275\272\305\251"); break;
					case JT_MER_LANCER09:
						sprintf(temp, "\263\262\\\263\262_\305\254\267\316\275\272\307\357\270\344"); break;
					case JT_MER_LANCER10:
						sprintf(temp, "\263\262\\\263\262_\267\316\265\345\304\253\310\243\300\307\273\324"); break;
					case JT_MER_SWORDMAN01:
						sprintf(temp, "\263\262\\\263\262_\271\335\303\242\260\355"); break;
					case JT_MER_SWORDMAN02:
						sprintf(temp, "\263\262\\\263\262_\272\320\263\353\300\324"); break;
					case JT_MER_SWORDMAN03:
						sprintf(temp, "\263\262\\\263\262_\304\332\263\242\270\256\270\360\300\332"); break;
					case JT_MER_SWORDMAN04:
						sprintf(temp, "\263\262\\\263\262_\276\337\261\270\270\360\300\332"); break;
					case JT_MER_SWORDMAN05:
						sprintf(temp, "\263\262\\\263\262_\300\345\267\312\260\307"); break;
					case JT_MER_SWORDMAN06:
						sprintf(temp, "\263\262\\\263\262_\272\316\277\353"); break;
					case JT_MER_SWORDMAN07:
						sprintf(temp, "\263\262\\\263\262_\305\270\300\314\260\305\270\266\275\272\305\251"); break;
					case JT_MER_SWORDMAN08:
						sprintf(temp, "\263\262\\\263\262_\303\312\275\302\264\336\305\365\261\270"); break;
					case JT_MER_SWORDMAN09:
						sprintf(temp, "\263\262\\\263\262_\277\243\301\251\270\265\270\360\300\332"); break;
					case JT_MER_SWORDMAN10:
						sprintf(temp, "\263\262\\\263\262_\305\302\276\347\275\305\300\307\270\360\300\332"); break;
				default:
					break ;
			}
		}
		else if (ML == 5)
		{
			switch (MerJob)
			{
					case JT_MER_SWORDMAN10:
						sprintf(temp, "\263\262\\\263\262_\264\343\271\350"); break;
					case JT_MER_LANCER10:
						sprintf(temp, "\263\262\\\263\262_\305\344\275\272\306\256"); break;
					case JT_MER_ARCHER10:
						sprintf(temp, "\277\251\\\277\251_\262\311\300\331"); break;
				default:
					break ;
			}
		}
		else if (ML == 6)
		{
			switch (MerJob)
			{
					case JT_MER_LANCER10:
					case JT_MER_SWORDMAN10:
						sprintf(temp, "\263\262\\\263\262_\277\344\301\244\300\307\261\315"); break;
					case JT_MER_ARCHER10:
						sprintf(temp, "\277\251\\\277\251_\277\344\301\244\300\307\261\315"); break;
				default:
					break ;
			}
		}
		else if (ML == 7)
		{
			switch (MerJob)
			{
					case JT_MER_SWORDMAN10:
						sprintf(temp, "\263\262\\\263\262_\273\347\300\314\305\254\267\323\275\272\276\306\300\314"); break;
					case JT_MER_LANCER10:
						sprintf(temp, "\263\262\\\263\262_\305\270\305\260\277\354\275\272\264\253\260\241\270\256\260\263"); break;
					case JT_MER_ARCHER10:
						sprintf(temp, "\277\251\\\277\251_\271\314\275\272\306\275\267\316\301\356"); break;
				default:
					break ;
			}
		}
		if (ML >= 4)
		{
			sprintf(tmpName, "\276\307\274\274\273\347\270\256\\%s.act",temp);
			actRes = (CActRes *)g_resMgr().Get(tmpName, 0);

			sprintf(tmpName, "\276\307\274\274\273\347\270\256\\%s.spr",temp);
			sprRes = (CSprRes *)g_resMgr().Get(tmpName, 0);

			mot = actRes->GetMotion(m_curAction, m_curMotion);

			if (m_curAction < 8)
			{
				mot = actRes->GetMotion(m_curAction, 0);
			}

			accMotion = mot;

			if (accMotion->attachCnt && headMotion->attachCnt)
			{
				if (accMotion->attachInfo[0].m_attr == headMotion->attachInfo[0].m_attr)
				{
					acc_imfOffX = headMotion->attachInfo[0].x - accMotion->attachInfo[0].x;
					acc_imfOffY = headMotion->attachInfo[0].y - accMotion->attachInfo[0].y;
				}
			}

			total_imfOffX = head_imfOffX + acc_imfOffX;
			total_imfOffY = head_imfOffY + acc_imfOffY;
		}

		float lmRIntensity = 1.0f;
		float lmGIntensity = 1.0f;
		float lmBIntensity = 1.0f;

		SetLmIntensity(lmRIntensity, lmGIntensity, lmBIntensity);

		float	m_fA = m_sprColor.a;
		float	m_fR = lmRIntensity * m_sprColor.r;
		float	m_fG = lmGIntensity * m_sprColor.g;
		float	m_fB = lmBIntensity * m_sprColor.b;

		RENDER_INFO_RECT info = { g_renderer->m_width, g_renderer->m_height, 0, 0, oow };

		bool isInsideScreen = false;

		for (int i=0; i<mot->numClips; i++)
		{
			float zoomX = 1.0f * m_zoom;
			float zoomY = 1.0f * m_zoom;
			float	top, btm, left, right;

			CSprClip *	sprClip = mot->GetClip(i);
			if ((unsigned int)sprClip->sprIndex == 0xffffffff)
				continue;
			SprImg *	img = sprRes->m_sprites[sprClip->clipType][sprClip->sprIndex];

			top = y + double(sprClip->y + total_imfOffY) * pixelRatio * sprClip->zoomy * zoomY;
			btm = y + double(sprClip->y + img->height*(img->isHalfH+1) + total_imfOffY) * pixelRatio * sprClip->zoomy * zoomY + 1;

			if (sprClip->flags & 1)
			{
				left = x + double(sprClip->x + img->width*(img->isHalfW+1) + total_imfOffX) * pixelRatio * sprClip->zoomx * zoomX;
				right = x + double(sprClip->x + total_imfOffX) * pixelRatio * sprClip->zoomx * zoomX + 1;

				if (top >= g_renderer->m_height || btm <= 0 || right >= g_renderer->m_width || left <= 0) continue;

			} else
			{
				left = x + double(sprClip->x + total_imfOffX) * pixelRatio * sprClip->zoomx * zoomX;
				right = x + double(sprClip->x + img->width*(img->isHalfW+1) + total_imfOffX) * pixelRatio * sprClip->zoomx * zoomX + 1;

				if (top >= g_renderer->m_height || btm <= 0 || left >= g_renderer->m_width || right <= 0) continue;
			}



			isInsideScreen = true;

			float sprOow;
			float sprOowup;
			if (m_alwaysTopLayer)
			{
				sprOow =	1.0f/NEARPLANE+g_zOffset*float(i) - g_zOffset*20 + g_zOffset*m_offsetOow;
				sprOowup =	1.0f/NEARPLANE+g_zOffset*float(i) - g_zOffset*20 + g_zOffset*m_offsetOow;
			} else if (m_isLieOnGround)
			{
				sprOow =	GetShadowOow(btm, y, tv.z)+g_zOffset*float(i+3+m_offsetOow)+zHover;
				sprOowup =	GetShadowOow(top, y, tv.z)+g_zOffset*float(i+3+m_offsetOow)+zHover;
			} else
			{
				sprOow =	GetSprOow(btm, y, tv.z)+g_zOffset*float(i+3+m_offsetOow)+zHover;
				sprOowup =	GetSprOow(top, y, tv.z)+g_zOffset*float(i+3+m_offsetOow)+zHover;
			}

			COLOR	argb;

			if (m_isSingleColor && m_colorOfSingleColor != 0xffff0000)
			{
				argb.color = 0xffff0000;
			} else {
				argb.a = float(sprClip->a) * m_fA * 1/255.0f;
				argb.r = float(sprClip->r) * m_fR * 1/255.0f;
				argb.g = float(sprClip->g) * m_fG * 1/255.0f;
				argb.b = float(sprClip->b) * m_fB * 1/255.0f;
			}

			int renderFlag = RF_SPRITE;

			if (argb.a < 255) renderFlag = RF_ALPHA;

			if ( ML >= 4 )
			{
				sprOow += 0.0001f + (ML-4)*0.00004f;
				sprOowup += 0.0001f + (ML-4)*0.00004f;

				renderFlag = RF_ALPHA;
			}

			RPSprite	spr;
			spr.top = top;
			spr.bottom = btm;
			spr.left = left + fSprShift;
			spr.right = right + fSprShift;
			spr.oow = sprOow;
			spr.oowup = sprOowup;
			spr.spr = img;
			spr.rgba = argb.color;
			spr.angle = sprClip->angle+m_sprAngle;

			SetCharacterAngle(&spr.angle);

			if (m_shouldAddPickInfo) info.Update(spr);

			SYSTEMPALETTE * palette = sprRes->m_pal;
			if (m_bodyPalette) {
				CPaletteRes * palRes = (CPaletteRes *) g_resMgr().Get(m_bodyPaletteName.c_str(), 0);
				palette = palRes->m_pal;
			}

			if ( (m_BodyLight & BL_DOUBLE_BODY) == BL_DOUBLE_BODY && (ML <= 1) )
			{
				RPSprite	spr2;
				float add;

				m_BodySin ++;
				if (m_BodySin>180)
					m_BodySin=0;
				if (m_BodySin < 0)
					m_BodySin=0;

				add=(g_sinTable[m_BodySin]*1.5f)+5.0f;

				add *= 400.0f / g_modeMgr.GetGameMode()->GetView()->GetDestDistance();

				spr2.top		= top -add;
				spr2.bottom		= btm +add;
				spr2.left		= left + fSprShift;
				spr2.right		= right + fSprShift;
				if (spr2.left > spr2.right)
				{
					spr2.left+=add;
					spr2.right-=add;
				}
				else
				{
					spr2.left-=add;
					spr2.right+=add;
				}
				spr2.oow			= sprOow - 0.00005f;
				spr2.oowup		= sprOowup - 0.00005f;
				spr2.spr			= img;
				spr2.rgba		= argb.color;
				spr2.angle		= sprClip->angle+m_sprAngle;

				RenderSprite(spr2, palette, RF_EFFECT);

			}
			RenderSprite(spr, palette, renderFlag);

		}

		if (isInsideScreen)
			SetRenderInfo(info, x, y);
		else
			SetTlvert(x, y);
	}
	m_BeZero = 1;
}

void	CRenderObject :: RenderBody(matrix &vtm, char FLAG1)
{
	vector3d	tv;

	tv.MatrixMult(m_pos, vtm);

	float		x, y, oow;
	g_renderer->ProjectVertex(m_pos, vtm, &x, &y, &oow);


	float	pixelRatio = g_renderer->GetPixelRatio(oow);
	m_lastPixelRatio = pixelRatio / g_avgPixelRatio;
	float	fSprShift = m_sprShift*m_zoom*g_renderer->GetScreenXFactor()*m_lastPixelRatio;
	float	zHover = g_zHover * m_lastPixelRatio;


	CMotion *	mot;
	CActRes *	actRes = GetActRes();
	CSprRes *	sprRes = GetSprRes();

	if (g_session.m_isDA_illusionON == 1 && FLAG1 == 1)
	{
		SYSTEMTIME sTime;
		GetLocalTime(&sTime);

		int DA_job;
		if ((sTime.wDay % 3) == 0)
			DA_job = 0x520;
		else if ((sTime.wDay % 3) == 1)
			DA_job = 0x56d;
		else
			DA_job = 0x4f8;

		char DA_Name[256];
		sprintf(DA_Name, "\270\363\275\272\305\315\\%s.act", g_session.m_jobNameTable[DA_job]);
		actRes = (CActRes *)g_resMgr().Get(DA_Name, 0);

		sprintf(DA_Name, "\270\363\275\272\305\315\\%s.spr", g_session.m_jobNameTable[DA_job]);
		sprRes = (CSprRes *)g_resMgr().Get(DA_Name, 0);
	}

	mot = actRes->GetMotion(m_curAction, m_curMotion);

	#ifdef	_DEBUG
	if (m_curAction >= actRes->GetActionCount())
	{
 		sAssert(false);
		Trace("Error : CRenderObject::Render - Illegal action,%s",actRes->GetName());
		return;
	}
	if (m_curMotion >= actRes->GetMotionCount(m_curAction)) {
		sAssert(false);
		Trace("Error : CRenderObject::Render - Illegal motion,%s",actRes->GetName());
		return;
	}
	#endif

	float lmRIntensity = 1.0f;
	float lmGIntensity = 1.0f;
	float lmBIntensity = 1.0f;

	SetLmIntensity(lmRIntensity, lmGIntensity, lmBIntensity);

	float	m_fA = m_sprColor.a;
	float	m_fR = lmRIntensity * m_sprColor.r;
	float	m_fG = lmGIntensity * m_sprColor.g;
	float	m_fB = lmBIntensity * m_sprColor.b;

	RENDER_INFO_RECT info = { g_renderer->m_width, g_renderer->m_height, 0, 0, oow };

	bool isInsideScreen = false;

	SetCharacterPixelRatio(&pixelRatio);

	for (int i=0; i<mot->numClips; i++)
	{
		float zoomX = 1.0f * m_zoom;
		float zoomY = 1.0f * m_zoom;
		float	top, btm, left, right;

		CSprClip *	sprClip = mot->GetClip(i);
		if ((unsigned int)sprClip->sprIndex == 0xffffffff)
			continue;

		SprImg *	img = sprRes->m_sprites[sprClip->clipType][sprClip->sprIndex];

		top = y + double(sprClip->y) * pixelRatio * sprClip->zoomy * zoomY;
		btm = y + double(sprClip->y + img->height*(img->isHalfH+1)) * pixelRatio * sprClip->zoomy * zoomY + 1;

		if (sprClip->flags & 1) {
			left = x + double(sprClip->x + img->width*(img->isHalfW+1)) * pixelRatio * sprClip->zoomx * zoomX;
			right = x + double(sprClip->x) * pixelRatio * sprClip->zoomx * zoomX + 1;

			if (top >= g_renderer->m_height || btm <= 0 || right >= g_renderer->m_width || left <= 0) continue;

		} else {
			left = x + double(sprClip->x) * pixelRatio * sprClip->zoomx * zoomX;
			right = x + double(sprClip->x + img->width*(img->isHalfW+1)) * pixelRatio * sprClip->zoomx * zoomX + 1;

			if (top >= g_renderer->m_height || btm <= 0 || left >= g_renderer->m_width || right <= 0) continue;
		}

		SetCharacterSquare(&top, &btm, &left, &right);

		isInsideScreen = true;

		float sprOow;
		float sprOowup;
		if (m_alwaysTopLayer)
		{
			sprOow =	1.0f/NEARPLANE+g_zOffset*float(i) - g_zOffset*20 + g_zOffset*m_offsetOow;
			sprOowup =	1.0f/NEARPLANE+g_zOffset*float(i) - g_zOffset*20 + g_zOffset*m_offsetOow;
		}
		else if (m_isLieOnGround)
		{
			sprOow =	GetShadowOow(btm, y, tv.z)+g_zOffset*float(i+3+m_offsetOow)+zHover;
			sprOowup =	GetShadowOow(top, y, tv.z)+g_zOffset*float(i+3+m_offsetOow)+zHover;
		}
		else
		{
			sprOow =	GetSprOow(btm, y, tv.z)+g_zOffset*float(i+3+m_offsetOow)+zHover;
			sprOowup =	GetSprOow(top, y, tv.z)+g_zOffset*float(i+3+m_offsetOow)+zHover;
		}

		COLOR	argb;

		if (m_isSingleColor && m_colorOfSingleColor != 0xffff0000)
		{
			argb.color = 0xffff0000;
		}
		else
		{
			argb.a = float(sprClip->a) * m_fA * 1/255.0f;
			argb.r = float(sprClip->r) * m_fR * 1/255.0f;
			argb.g = float(sprClip->g) * m_fG * 1/255.0f;
			argb.b = float(sprClip->b) * m_fB * 1/255.0f;
		}

#ifdef RENDER_FIX2

		vector3d ambColor = g_Weather.GetAmbient() ;
		vector3d envColor = g_Weather.GetEnvDiff() ;

		argb.r *= envColor.x ;
		argb.g *= envColor.y ;
		argb.b *= envColor.z ;

		argb.r += ambColor.x ;
		argb.g += ambColor.y ;
		argb.b += ambColor.z ;

#endif

		RPSprite	spr;
		spr.top = top;
		spr.bottom = btm;
		spr.left = left + fSprShift;
		spr.right = right + fSprShift;
		spr.oow = sprOow;
		spr.oowup = sprOowup;
		spr.spr = img;
		spr.rgba = argb.color;
		spr.angle = sprClip->angle+m_sprAngle;

		SetCharacterAngle(&spr.angle);

		if (m_shouldAddPickInfo) info.Update(spr);

		SYSTEMPALETTE * palette = sprRes->m_pal;
		if (m_bodyPalette)
		{
			CPaletteRes * palRes = (CPaletteRes *) g_resMgr().Get(m_bodyPaletteName.c_str(), 0);
			palette = palRes->m_pal;
		}

		int renderFlag = RF_SPRITE;

		if (argb.a < 255) renderFlag = RF_ALPHA;

		if ( (m_BodyLight & BL_LIGHT_BODY) == BL_LIGHT_BODY )
			renderFlag = RF_EFFECT;

		if ( (m_BodyLight & BL_DOUBLE_BODY) == BL_DOUBLE_BODY )
		{
			RPSprite	spr2;
			float add;

			m_BodySin ++;
			if (m_BodySin>180)
				m_BodySin=0;
			if (m_BodySin < 0)
				m_BodySin=0;

			add=(g_sinTable[m_BodySin]*1.5f)+5.0f;

			add *= 400.0f / g_modeMgr.GetGameMode()->GetView()->GetDestDistance();

			spr2.top		= top -add;
			spr2.bottom		= btm +add;
			spr2.left		= left + fSprShift;
			spr2.right		= right + fSprShift;
			if (spr2.left > spr2.right)
			{
				spr2.left+=add;
				spr2.right-=add;
			}
			else
			{
				spr2.left-=add;
				spr2.right+=add;
			}
			spr2.oow			= sprOow - 0.00005f;
			spr2.oowup		= sprOowup - 0.00005f;
			spr2.spr			= img;
			spr2.rgba		= argb.color;
			spr2.angle		= sprClip->angle+m_sprAngle;

			RenderSprite(spr2, palette, RF_EFFECT);

		}

		if ( (m_BodyLight & BL_ASURA) == BL_ASURA )

		{
			RPSprite	spr2;
			float add;
			short alpha;

			for (int i=1;i<=10;i++)
			{
				if (m_BodyTime <= 130)
				{
					alpha = m_BodyTime - 50;
					alpha -= (i)*5;
				}
				else
				{
					alpha = 145 - m_BodyTime;
					alpha += (10-i)*10;
				}

				if (alpha>200)
					alpha = 200;
				if (alpha<0)
					alpha = 0;

				add = i*3;

				add *= 400.0f / g_modeMgr.GetGameMode()->GetView()->GetDestDistance();

				spr2.top		= top - add;
				spr2.bottom		= btm + add;
				spr2.left		= left + fSprShift;
				spr2.right		= right + fSprShift;
				if (spr2.left > spr2.right)
				{
					spr2.left+=add;
					spr2.right-=add;
				}
				else
				{
					spr2.left-=add;
					spr2.right+=add;
				}

				spr2.oow		= sprOow + (0.000005f*add) + 0.00001f;
				spr2.oowup		= sprOowup + (0.000005f*add) + 0.00001f;
				spr2.spr		= img;
				spr2.rgba		= (alpha<<24) + (255<<16) + (255<<8) + 255;;
				spr2.angle		= sprClip->angle+m_sprAngle;

				RenderSprite(spr2, palette, RF_EFFECT);

			}
		}

		if ( (m_BodyLight & BL_UNDEAD) == BL_UNDEAD || (m_BodyLight & BL_LIGHT_SWORD) == BL_LIGHT_SWORD )
		{
			RPSprite	spr2;
			float add;
			short alpha;

			for (int i=1;i<=2;i++)
			{
				alpha = m_BodySin5;

				add = i;

				add *= 400.0f / g_modeMgr.GetGameMode()->GetView()->GetDestDistance();

				spr2.top		= top - add;
				spr2.bottom		= btm + add;
				spr2.left		= left + fSprShift;
				spr2.right		= right + fSprShift;
				if (spr2.left > spr2.right)
				{
					spr2.left+=add;
					spr2.right-=add;
				}
				else
				{
					spr2.left-=add;
					spr2.right+=add;
				}

				spr2.oow		= sprOow + (0.000005f*add) + 0.00001f;
				spr2.oowup		= sprOowup + (0.000005f*add) + 0.00001f;
				spr2.spr		= img;
				if ( (m_BodyLight & BL_LIGHT_SWORD) == BL_LIGHT_SWORD )
					spr2.rgba		= (alpha<<24) + (55<<16) + (55<<8) + 255;
				else
					spr2.rgba		= (alpha<<24) + (5<<16) + (155<<8) + 5;
				spr2.angle		= sprClip->angle+m_sprAngle;

				RenderSprite(spr2, palette, RF_EFFECT);

			}
		}

		if ( (m_BodyLight & BL_4WAY) == BL_4WAY )

		{
			RPSprite	spr2;
			float add;
			short alpha;

			for (int i=1;i<=4;i++)
			{
				add = m_BodyTime*(btm-top)*0.05f;
				add *= 400.0f / g_modeMgr.GetGameMode()->GetView()->GetDestDistance();
				alpha = 150-(m_BodyTime*5);
				if (alpha<0)
					alpha=0;

				if (i==1)
				{
					spr2.top		= top;
					spr2.bottom		= btm;
					spr2.left		= left + fSprShift + add;
					spr2.right		= right + fSprShift + add;
				}
				else if (i==2)
				{
					spr2.top		= top;
					spr2.bottom		= btm;
					spr2.left		= left + fSprShift - add;
					spr2.right		= right + fSprShift - add;
				}
				else if (i==3)
				{
					spr2.top		= top + add;
					spr2.bottom		= btm + add;
					spr2.left		= left + fSprShift;
					spr2.right		= right + fSprShift;
				}
				else if (i==4)
				{
					spr2.top		= top - add;
					spr2.bottom		= btm - add;
					spr2.left		= left + fSprShift;
					spr2.right		= right + fSprShift;
				}

				spr2.oow		= sprOow + 0.1f;
				spr2.oowup		= sprOowup + 0.1f;
				spr2.spr		= img;
				spr2.rgba		= (alpha<<24) + (255<<16) + (255<<8) + 255;
				spr2.angle		= sprClip->angle+m_sprAngle;

				RenderSprite(spr2, palette, renderFlag);

			}
		}

		if ( (m_BodyLight & BL_REFLECT_BODY) == BL_REFLECT_BODY )
		{
			RPSprite	spr2;
			float add;
			short alpha;

			m_BodySin2 ++;
			if (m_BodySin2>=200)
				m_BodySin2-=200;

			for (int i=1;i<5;i++)
			{
				add=(i*5)+(m_BodySin2*0.1f);
				if (add>=20)
					add-=20;
				alpha = 100 - add*5;

				add *= 400.0f / g_modeMgr.GetGameMode()->GetView()->GetDestDistance();

				spr2.top		= top - add;
				spr2.bottom		= btm + add;
				spr2.left		= left + fSprShift;
				spr2.right		= right + fSprShift;
				if (spr2.left > spr2.right)
				{
					spr2.left+=add;
					spr2.right-=add;
				}
				else
				{
					spr2.left-=add;
					spr2.right+=add;
				}

				spr2.oow		= sprOow - (0.000005f*add) - 0.00001f;
				spr2.oowup		= sprOowup - (0.000005f*add) - 0.00001f;
				spr2.spr		= img;
				spr2.rgba		= (alpha<<24) + (255<<16) + (255<<8) + 255;;
				spr2.angle		= sprClip->angle+m_sprAngle;

				if (renderFlag != RF_EFFECT)
					RenderSprite(spr2, palette, renderFlag);

			}
			spr.rgba		= (150<<24) + (255<<16) + (255<<8) + 255;;
		}

		if ( (m_BodyLight & BL_QUAKE) == BL_QUAKE )
		{
			float ran[4];

			for (int i=0;i<4;i++)
			{
				ran[i] = random(21) - 10;
				ran[i] *= 400.0f / g_modeMgr.GetGameMode()->GetView()->GetDestDistance();
			}

			spr.top			= top + ran[0];
			spr.bottom		= btm + ran[1];
			spr.left		= left + fSprShift + ran[2];
			spr.right		= right + fSprShift + ran[3];
		}

		if ( (m_BodyLight & BL_LIGHT_SWORD) == BL_LIGHT_SWORD )
			spr.rgba		= (100<<24) + (55<<16) + (55<<8) + 255;

		RenderSprite(spr, palette, renderFlag);

		SetCharacterLight(spr, palette);
	}

	if (isInsideScreen)
		SetRenderInfo(info, x, y);
	else
		SetTlvert(x, y);
	m_BeZero = 1;
}

void	CRenderObject :: RenderShadow(matrix &vtm, float zoom)
{
	if (m_shadowOn && m_shadowZoom!=0 && zoom!=0) {

		sAssert(g_modeMgr.GetGameMode() != NULL);

		vector3d	pos(m_pos.x, g_modeMgr.GetGameMode()->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z), m_pos.z);

		vector3d	tv;
		tv.MatrixMult(pos, vtm);

		float		x, y, oow;
		g_renderer->ProjectVertex(pos, vtm, &x, &y, &oow);


		float	pixelRatio = g_renderer->GetPixelRatio(oow);

		CSprRes *	shadow_spr = (CSprRes*)g_resMgr().Get("shadow.spr", 0);
		CActRes *	shadow_act = (CActRes*)g_resMgr().Get("shadow.act", 0);

		CSprClip *	sprClip = shadow_act->GetMotion(0, 0)->GetClip(0);
		SprImg *	img = shadow_spr->m_sprites[sprClip->clipType][0];

		float zoomX = zoom * m_shadowZoom;
		float zoomY = zoom * m_shadowZoom;
		float	top, btm, left, right;

		top = y + double(sprClip->y) * pixelRatio * sprClip->zoomy * zoomY;
		btm = y + double(sprClip->y + img->height*(img->isHalfH+1)) * pixelRatio * sprClip->zoomy * zoomY + 1;
		left = x + double(sprClip->x) * pixelRatio * sprClip->zoomx * zoomX;
		right = x + double(sprClip->x + img->width*(img->isHalfW+1)) * pixelRatio * sprClip->zoomx * zoomX + 1;

		if (top >= g_renderer->m_height || btm <= 0 || left >= g_renderer->m_width || right <= 0) return;


		DWORD	argb = (sprClip->a<<24) + (sprClip->r<<16) + (sprClip->g<< 8) + sprClip->b;

		RPSprite	spr;
		spr.top = top;
		spr.bottom = btm;
		spr.left = left + m_sprShift;
		spr.right = right + m_sprShift;
		spr.oow = GetShadowOow(btm, y, tv.z)+0.00001f;
		spr.oowup = GetShadowOow(top, y, tv.z)+0.00001f;
		spr.spr = img;
		spr.rgba = argb;
		spr.angle = sprClip->angle;

		RenderSprite(spr, shadow_spr->m_pal, 0x101);

	}
}

void	CRenderObject :: RenderSprite(const RPSprite &spr, unsigned long* const pal, int renderFlag)
{
		SprImg *	img = spr.spr;
		CTexture *	tex;
		CacheInfo	info;

		if (img->tex == NULL)
		{
			tex = g_renderer->GetSpriteIndex(*img, (DWORD)pal, &info);
			if (tex == 0 )
			{
				tex = g_renderer->AddSpriteIndex(*img, pal, &info);
			}


		}
		else
		{
			tex = img->tex;
			info.tu = 0;
			info.tv = 0;
			info.tu1 = img->tex->GetUAdjust();
			info.tv1 = img->tex->GetVAdjust();
		}

		float		z = GetZFromOow(spr.oow);
		float		z1 = GetZFromOow(spr.oowup);

		RPQuadFace& rp = g_renderer->BorrowQuadRP();

		rp.tex = tex;
		tlvertex3d*	verts = rp.verts;

		float x1cs, x1sn, x2cs, x2sn, y1sn, y1cs, y2sn, y2cs;

		if (spr.angle)
		{
			float	cs = GetCos(spr.angle);
			float	sn = GetSin(spr.angle);

			float	w = (spr.right - spr.left) / 2;
			float	h = (spr.bottom - spr.top) / 2;
			float	x = spr.left + w;
			float	y = spr.top + h;

			x1cs = -w * cs;
			x1sn = -w * sn;
			x2cs = w * cs;
			x2sn = w * sn;
			y1sn = -h * sn;
			y1cs = -h * cs;
			y2sn = h * sn;
			y2cs = h * cs;

			verts[0].x = x + (x1cs - y1sn) + 0.5f;
			verts[0].y = y + (x1sn + y1cs) + 0.5f;
			verts[1].x = x + (x2cs - y1sn) + 0.5f;
			verts[1].y = y + (x2sn + y1cs) + 0.5f;
			verts[2].x = x + (x1cs - y2sn) + 0.5f;
			verts[2].y = y + (x1sn + y2cs) + 0.5f;
			verts[3].x = x + (x2cs - y2sn) + 0.5f;
			verts[3].y = y + (x2sn + y2cs) + 0.5f;

		}
		else
		{
			verts[0].x = spr.left + 0.5f;
			verts[0].y = spr.top + 0.5f;
			verts[1].x = spr.right + 0.5f;
			verts[1].y = spr.top + 0.5f;
			verts[2].x = spr.left + 0.5f;
			verts[2].y = spr.bottom + 0.5f;
			verts[3].x = spr.right + 0.5f;
			verts[3].y = spr.bottom + 0.5f;
		}

		verts[0].oow = spr.oowup;
		verts[0].z = z1;
		verts[0].tu = info.tu;
		verts[0].tv = info.tv;
		verts[0].color = spr.rgba;

		verts[1].oow = spr.oowup;
		verts[1].z = z1;
		verts[1].tu = info.tu1;
		verts[1].tv = info.tv;
		verts[1].color = spr.rgba;

		verts[2].oow = spr.oow;
		verts[2].z = z;
		verts[2].tu = info.tu;
		verts[2].tv = info.tv1;
		verts[2].color = spr.rgba;

		verts[3].oow = spr.oow;
		verts[3].z = z;
		verts[3].tu = info.tu1;
		verts[3].tv = info.tv1;
		verts[3].color = spr.rgba;

		if (renderFlag == 5)
		{
			rp.srcAlphaMode = 5;
			rp.destAlphaMode = 2;
		}
		else
		{
			rp.srcAlphaMode = 5;
			rp.destAlphaMode = 6;
		}

		g_renderer->AddRP(&rp, renderFlag);
}


void	CRenderObject :: SetRenderInfo(RENDER_INFO_RECT &info, const float tlvertX, const float tlvertY)
{

}

void	CRenderObject :: SetTlvert(float tlvertX, float tlvertY)
{
	m_lastTlvertX = tlvertX;
	m_lastTlvertY = tlvertY;
}

void	CRenderObject :: Render(matrix &vtm)
{
	if (m_isVisible)
	{
		RenderShadow(vtm, m_zoom);
		RenderBody(vtm, 0);
	}
}

void CRenderObject :: SetSprName(const char* sprName)
{
	if (m_sprRes) m_sprRes->Unlock();
	m_sprRes = (CSprRes*)g_resMgr().Get(sprName, 0);
	if (m_sprRes) m_sprRes->Lock();
}

const char* CRenderObject::GetSprName() {
	if (!m_sprRes) return NULL;
	return m_sprRes->GetName();
}

CSprRes* CRenderObject::GetSprRes() {
	return m_sprRes;
}

void CRenderObject :: SetActName(const char* actName)
{
	if (m_actRes) m_actRes->Unlock();
	m_actRes = (CActRes*)g_resMgr().Get(actName, 0);
	m_actRes->Lock();
}

CActRes* CRenderObject::GetActRes() {
	return m_actRes;
}

const char* CRenderObject::GetActName() {
	if (!m_actRes) return NULL;
	return m_actRes->GetName();
}

void CRenderObject :: SetCharacterPixelRatio(float	*pixelratio)
{
	if ( (m_BodyLight & BL_BABY_BODY) == BL_BABY_BODY )
	{
		int sinangle;
		float sn;

		sinangle = (m_BodySin3*4) + 90;
		if (sinangle<90)
			sinangle=90;
		if (sinangle>270)
			sinangle=270;
		if (sinangle<270)
		{
			sn = g_sinTable[sinangle];
			sn += 1.0f;
			sn *= 0.25f;
			sn+= 0.5f;
			*pixelratio *= sn;
		}
		else
			*pixelratio *= 0.5f;
	}

	if ( (m_BodyLight & BL_GIANT) == BL_GIANT )
	{
		int sinangle;
		float sn;

		sinangle = (m_BodySin4*4) + 90;
		if (sinangle<90)
			sinangle=90;
		if (sinangle>270)
			sinangle=270;
		if (sinangle<270)
		{
			sn = g_sinTable[sinangle];
			sn += 1.0f;
			sn *= 0.25f;
			sn = 1.5f - sn;
			*pixelratio *= sn;
		}
		else
			*pixelratio *= 1.5f;
	}
}

void CRenderObject :: SetCharacterSquare(float	*top, float *btm, float *left, float *right)
{
	if ( (m_BodyLight & BL_KICKED) == BL_KICKED )
	{
		int sinangle;
		float sn;
		float minus,size;

		sinangle = m_BodyTime*5;
		sn = g_sinTable[sinangle];

		size=0.0f;
		if (m_BodyTime<=18)
		{
			size = sn*0.3f;
		}
		else if (m_BodyTime<=20)
			size = 0.3f;
		else if (m_BodyTime<=23)
			size = (23-m_BodyTime)*0.1f;

		minus=*btm-*top;
		minus*=size;
		*top-=minus;
		*btm-=minus;
	}

	if ( (m_BodyLight & BL_PRESSED) == BL_PRESSED )
	{

		float minus,size;

		size=1.0f;
		if (m_BodyTime <= 3)
		{
			size = ( 100 - (m_BodyTime*20.0f) ) * 0.01f;
		}
		else if (m_BodyTime <= 33)
		{
			size = 0.4f-(random(6)*0.01f);
		}

		minus=*btm-*top;
		size = 1.0f - size;
		minus*=size;
		*top+=minus;
	}
}

void CRenderObject :: SetCharacterAngle(float	*angle)
{
	const int BL_SPINED = 0x40;
	if ( (m_BodyLight & BL_SPINED) == BL_SPINED )
	{
		int sinangle;
		float sn;

		sinangle = m_BodyTime*5;
		sn = g_sinTable[sinangle+90];

		if (m_BodyTime<=36)
		{
			*angle = 180-(sn*180);

			if (*angle >= 360)
				*angle -= 360;
		}
		else
			*angle = 0;
	}
}

short CRenderObject :: SetCharacterFlying(void)
{
	short fpos;
	char IsWarm;

	{
		m_FlyMove+=2;
		if (m_FlyMove>=360)
			m_FlyMove-=360;

		IsWarm = 0;
		if ( (m_BodyLight & BL_HIT_RED) == BL_HIT_RED )
			IsWarm = 1;

		if ( m_stateId == ST_STAND || m_stateId == ST_MOVE || IsWarm == 1 )
		{
			if (m_FlyNow < 20)
			{
				if (m_FlyMove%30 == 0)
					m_FlyNow++;
			}
			else if (m_FlyNow > 20)
			{
				if (m_FlyMove%30 == 0)
					m_FlyNow--;
			}
		}
		else if ( m_stateId == ST_SIT )
		{
			if (m_FlyNow < 100)
			{
				if (m_FlyMove%30 == 0)
					m_FlyNow++;
			}
			else if (m_FlyNow > 100)
			{
				if (m_FlyMove%30 == 0)
					m_FlyNow--;
			}
		}
		else if (m_FlyNow > 0)
		{
			m_FlyNow--;
		}
	}

	fpos = m_FlyNow;
	fpos += (g_sinTable[m_FlyMove]*4.0f);

	fpos *= 400.0f / g_modeMgr.GetGameMode()->GetView()->GetDestDistance();

	if (fpos < 0)
		fpos = 0;

	return fpos;
}

void CRenderObject :: Palette_BW(CPaletteRes *palRes)
{
	PALETTEENTRY palEntry[256];

	for (int i=0;i<256;i++)
	{
		int a,r,g,b,rgb;
		int paldata;

		paldata = PACK555TO32(palRes->m_pal[i]);

		a=paldata>>24;
		r=(paldata>>16)%256;
		g=(paldata>>8)%256;
		b=paldata%256;
		rgb=(r+g+b)/3;

		palEntry[i].peFlags=a;
		palEntry[i].peRed=rgb;
		palEntry[i].peGreen=rgb;
		palEntry[i].peBlue=rgb;
	}

	g_3dDevice.ConvertPalette(palRes->m_pal, palEntry, 256);
}



CActRes* CRenderObject :: SetCharacterActRes(int m_accessory2, int layer)
{
	if (layer == Layer_Body)
	{
		if (m_BodyAni == 1)
		{
			m_BodyAniFrame++;
			if (m_BodyAniFrame > 20)
			{
				m_BodyAniFrame = 0;
				m_BodyAct++;
				if (m_BodyAct > 2)
					m_BodyAct = 0;
			}

			if (m_BodyAct == 1)
				return (CActRes *)g_resMgr().Get("sprite\\\xc0\xce\xb0\xa3\xc1\xb7\\\xb8\xf6\xc5\xeb\\\xb3\xb2\\\xb8\xf9\xc5\xa9_\xb3\xb2.act", 0);
			else if (m_BodyAct == 2)
				return (CActRes *)g_resMgr().Get("sprite\\\xc0\xce\xb0\xa3\xc1\xb7\\\xb8\xf6\xc5\xeb\\\xb3\xb2\\\xb7\xce\xb1\xd7_\xb3\xb2.act", 0);
		}
		else if (m_BodyAni == 3)
		{
			return (CActRes *)g_resMgr().Get("sprite\\\xc0\xce\xb0\xa3\xc1\xb7\\\xb8\xf6\xc5\xeb\\\xb3\xb2\\\xb8\xf9\xc5\xa9_\xb3\xb2.act", 0);
		}
	}
	if (m_accessory2 == 15 && layer == 3)
	{
		m_BodyTime3++;
		if (m_BodyTime3 >= 0x28)
			m_BodyTime3 = 0;

		if (m_BodyTime3 < 0x14)
			return (CActRes *)g_resMgr().Get("sprite\\\xbe\xc7\xbc\xbc\xbb\xe7\xb8\xae\\\xb3\xb2\\\xb3\xb2_\xc3\xb5\xbb\xe7\xc0\xc7\xb8\xd3\xb8\xae\xb6\xec.act", 0);
		if (m_BodyTime3 < 0x28)
			return (CActRes *)g_resMgr().Get("sprite\\\xbe\xc7\xbc\xbc\xbb\xe7\xb8\xae\\\xb3\xb2\\\xb3\xb2_\xbe\xc7\xb8\xb6\xc0\xc7\xb8\xd3\xb8\xae\xb6\xec.act", 0);
	}

	return 0;
}



CSprRes* CRenderObject :: SetCharacterSprRes(int m_accessory2, int layer)
{
	if (layer == Layer_Body)
	{
		if (m_BodyAni == 1)
		{
			m_BodyAniFrame++;
			if (m_BodyAniFrame > 20)
			{
				m_BodyAniFrame = 0;
				m_BodyAct++;
				if (m_BodyAct > 2)
					m_BodyAct = 0;
			}

			if (m_BodyAct == 1)
				return (CSprRes *)g_resMgr().Get("sprite\\\xc0\xce\xb0\xa3\xc1\xb7\\\xb8\xf6\xc5\xeb\\\xb3\xb2\\\xb8\xf9\xc5\xa9_\xb3\xb2.spr", 0);
			else if (m_BodyAct == 2)
				return (CSprRes *)g_resMgr().Get("sprite\\\xc0\xce\xb0\xa3\xc1\xb7\\\xb8\xf6\xc5\xeb\\\xb3\xb2\\\xb7\xce\xb1\xd7_\xb3\xb2.spr", 0);
		}
		else if (m_BodyAni == 3)
		{
			return (CSprRes *)g_resMgr().Get("sprite\\\xc0\xce\xb0\xa3\xc1\xb7\\\xb8\xf6\xc5\xeb\\\xb3\xb2\\\xb8\xf9\xc5\xa9_\xb3\xb2.spr", 0);
		}
	}
	if (m_accessory2 == 15 && layer == 3)
	{
		if (m_BodyTime3 < 0x14)
			return (CSprRes *)g_resMgr().Get("sprite\\\xbe\xc7\xbc\xbc\xbb\xe7\xb8\xae\\\xb3\xb2\\\xb3\xb2_\xc3\xb5\xbb\xe7\xc0\xc7\xb8\xd3\xb8\xae\xb6\xec.spr", 0);
		if (m_BodyTime3 < 0x28)
			return (CSprRes *)g_resMgr().Get("sprite\\\xbe\xc7\xbc\xbc\xbb\xe7\xb8\xae\\\xb3\xb2\\\xb3\xb2_\xbe\xc7\xb8\xb6\xc0\xc7\xb8\xd3\xb8\xae\xb6\xec.spr", 0);
	}

	return 0;
}




void CRenderObject :: SetCharacterPreAngle(int layer, int *imfOffX, int *imfOffY, int *head_imfOffX, int *head_imfOffY)
{
	const int BL_SPINED = 0x40;
	if ( (m_BodyLight & BL_SPINED) == BL_SPINED )
	{
		int sinangle = m_BodyTime*5;
		float sn = g_sinTable[sinangle+90];

		int angle = 180 - (sn * 180);
		if (angle >= 360)
			angle -= 360;

		if (layer == 1)
		{
			*imfOffX += (g_sinTable[angle]*40);
			*imfOffY -= (g_cosTable[angle]*40)-40;

			*head_imfOffX = *imfOffX;
			*head_imfOffY = *imfOffY;
			return;
		}
		if (layer >= 2)
		{
			*imfOffX += (g_sinTable[angle]*10);
			*imfOffY -= (g_cosTable[angle]*10)-10;
		}
	}
}

void CRenderObject :: SetCharacterLight(RPSprite spr, unsigned long* const pal)
{
	if ( (m_BodyLight & BL_HIT) == BL_HIT )
	{
		short R,G,B;
		short alpha;

		R=255;G=255;B=255;

		if (m_BodyTime<=5)
		{
			alpha = m_BodyTime*50;
			spr.rgba = (alpha<<24) + (R<<16) + (G<<8) + B;
		}
		else if (m_BodyTime<=8)
		{
			spr.rgba = (255<<24) + (R<<16) + (G<<8) + B;
		}
		else if (m_BodyTime<=11)
		{
			alpha = 255-((m_BodyTime-8)*80);
			spr.rgba = (alpha<<24) + (R<<16) + (G<<8) + B;
		}

		if (m_BodyTime<=11)
		{
			RenderSprite(spr, pal, 5);
		}
	}

	if ( (m_BodyLight & BL_HIT_BLUE) == BL_HIT_BLUE )
	{
		short R,G,B;
		short alpha;

		R=5;G=5;B=255;

		if (m_BodyTime2<=10)
		{
			alpha = m_BodyTime2*15;
			spr.rgba = (alpha<<24) + (R<<16) + (G<<8) + B;
		}
		else if (m_BodyTime2<=20)
		{
			spr.rgba = (160<<24) + (R<<16) + (G<<8) + B;
		}
		else if (m_BodyTime2<=50)
		{
			alpha = 155-((m_BodyTime2-20)*5);
			spr.rgba = (alpha<<24) + (R<<16) + (G<<8) + B;
		}

		if (m_BodyTime2<=50)
		{
			RenderSprite(spr, pal, 5);
			RenderSprite(spr, pal, 5);

		}
	}

	if ( (m_BodyLight & BL_HIT_RED) == BL_HIT_RED )
	{
		short R,G,B;
		short alpha;

		R=255;G=5;B=5;

		if (m_BodyTime2<=10)
		{
			alpha = m_BodyTime2*15;
			spr.rgba = (alpha<<24) + (R<<16) + (G<<8) + B;
		}
		else if (m_BodyTime2<=20)
		{
			spr.rgba = (160<<24) + (R<<16) + (G<<8) + B;
		}
		else if (m_BodyTime2<=50)
		{
			alpha = 155-((m_BodyTime2-20)*5);
			spr.rgba = (alpha<<24) + (R<<16) + (G<<8) + B;
		}

		if (m_BodyTime2<=50)
		{
			RenderSprite(spr, pal, 5);
			RenderSprite(spr, pal, 5);

		}
	}
}

CMousePointer :: CMousePointer()
{
	OnInit();
}

CMousePointer :: ~CMousePointer()
{
	OnExit();
}

void	CMousePointer :: OnInit()
{
	m_visibleTime = 0;
	m_alwaysTopLayer = true;
	m_isVisible = false;
	m_stateStartTick = timeGetTime();
	SetSprName("cursors.spr");
	SetActName("cursors.act");
	SetAction(3, 2, MT_LOOP);
}

void	CMousePointer :: OnExit()
{
}

bool	CMousePointer :: OnProcess()
{
	float stateCnt = (timeGetTime() - m_stateStartTick) / 24.0f;
	m_curAction = m_baseAction;
	CActRes *	actRes = GetActRes();
	m_curMotion = int(stateCnt/m_motionSpeed)%(actRes->GetMotionCount(m_curAction));

	if (m_visibleTime) {
		m_visibleTime--;
		if (m_visibleTime==0)
			m_isVisible = false;
	}

	return true;
}


void	CMousePointer :: SendMsg(CGameObject *sender, int message, int arg1, int arg2, int arg3)
{
	switch (message) {
		case AM_SETPOSMOUSE:
			{
				m_pos.x = ((vector2d *)arg1)->x;
				m_pos.z = ((vector2d *)arg1)->y;
				CGameMode * gameMode = g_modeMgr.GetGameMode();
				CWorld * world = gameMode->GetWorld();
				m_pos.y = world->m_attr->GetHeight(m_pos.x, m_pos.z);
				m_isVisible = true;
				m_visibleTime = 0;
				if (arg2)
					m_sprColor.color = 0xffff0000;
				else
					m_sprColor.color = 0xffffffff;
			}
			break;
		case AM_HIDEMOUSE:
			{
				if (arg1)
					m_visibleTime = arg1;
				else
					m_isVisible = false;
			}
			break;
	}
}

CAbleToMakeEffect :: CAbleToMakeEffect()
{
	OnInit();
}

CAbleToMakeEffect :: ~CAbleToMakeEffect()
{
	OnExit();
}

void	CAbleToMakeEffect :: OnInit()
{
	m_efId = -1;
	m_Sk_Level = 0;
	m_isLoop = FALSE;

	m_beginSpellEffect = NULL;
	m_magicTargetEffect = NULL;
}

void	CAbleToMakeEffect :: OnExit()
{
	if (m_effectList.size())
	{
		std::list<CRagEffect *>::iterator it = m_effectList.begin();
		while (it != m_effectList.end())
		{
			if (*it)
			{
				(*it)->SendMsg(this, AM_MASTER_DEAD);
			}
			++it;
		}
		m_effectList.erase(m_effectList.begin(), m_effectList.end());
	}
	m_beginSpellEffect = NULL;
	m_magicTargetEffect = NULL;
}

int g_recentEffectId[32] = { 0, };

CRagEffect *	CAbleToMakeEffect :: LaunchEffect(int effectId, vector3d fDeltaPos, float fRot)
{




	BOOL forceEffect = FALSE;
	switch (effectId) {
		case	EF_LOCKON:
		case	EF_PORTAL:
		case	EF_READYPORTAL:
		case	EF_BOTTOM:
		case	EF_BOTTOM2:
		case	EF_REFINEFAIL:
		case	EF_REFINEOK:
		case	EF_TOPRANK:
		case	EF_FIREPILLARON:
		case	EF_STATUS_STATE:
		case	EF_PARTY:
		case	EF_BLIND:
		case	EF_TALK_FROSTJOKE:
		case	EF_TALK_SCREAM:
		case	EF_ANGEL:
		case	EF_ANGEL2:
		case	EF_MINI_TETRIS:
		case	EF_DEVIL1:
		case	EF_DEVIL2:
		case	EF_DEVIL3:
		case	EF_DEVIL4:
		case	EF_DEVIL5:
		case	EF_DEVIL6:
		case	EF_DEVIL7:
		case	EF_DEVIL8:
		case	EF_DEVIL9:
		case	EF_DEVIL10:
		case	EF_BLUEBODY:
		case	EF_REDLIGHTBODY:
		case	EF_ASURABODY:
		case	EF_UNDEADBODY:
		case	EF_UNDEADBODY_DEL:
		case	EF_TAE_READY:
		case	EF_RED_HIT:
		case	EF_BLUE_HIT:
		case	EF_JUMPBODY:
		case	EF_LANDBODY:
		case	EF_QUAKEBODY:
		case	EF_ANGEL3:
		case	EF_REFLECTBODY:
			forceEffect = TRUE;
			break;
		case	EF_POISONREACT2:
			m_isLoop = TRUE;
			break;
	}

	if ((!g_session.m_isEffectOn || effectId < 0 ||  effectId > 60000) && (forceEffect==FALSE))
		return NULL;

	int buf[32];
	memcpy(buf + 1, g_recentEffectId, 31);
	buf[0] = effectId;
	memcpy(g_recentEffectId, buf, 32);

	CRagEffect *newEffect = new CRagEffect;

	float sinDeg = GetSin((int)fRot);
	float cosDeg = GetCos((int)fRot);
	float dx = (fDeltaPos.x * cosDeg) - (fDeltaPos.z * sinDeg);
	float dz = (fDeltaPos.x * sinDeg) + (fDeltaPos.z * cosDeg);

	fDeltaPos.x = dx;
	fDeltaPos.z = dz;

	if (newEffect == NULL || this == NULL)
		return NULL;




	newEffect->Init(this, effectId, fDeltaPos);

	CGameMode * gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);
	gameMode->GetWorld()->m_gameObjectList.push_back(newEffect);
	m_effectList.push_back(newEffect);

	return	newEffect;
}

bool CAbleToMakeEffect :: IsExistEffect( CRagEffect* pEffect )
{
	if (m_effectList.size()) {
		std::list<CRagEffect *>::iterator it = m_effectList.begin();
		while (it != m_effectList.end()) {
			if( (*it)==pEffect ) return true;
			++it;
		}
	}
	return false;
}

void	CAbleToMakeEffect :: SendMsg(CGameObject * sender, int msgid, int val1, int val2, int val3)
{

	switch (msgid) {
		case	AM_MAKE_MAGIC_TARGET:
			if (m_magicTargetEffect == NULL) {
				m_magicTargetEffect = LaunchEffect(val1);
			}
			if (m_magicTargetEffect)
				m_magicTargetEffect->SendMsg(NULL, AM_SET_DURATION, val2);
			break;
		case	AM_DELETE_MAGIC_TARGET:
			if (m_magicTargetEffect) {
				std::list<CRagEffect *>::iterator it = std::find(m_effectList.begin(), m_effectList.end(), m_magicTargetEffect);
				if (it != m_effectList.end()) {
					(*it)->SendMsg(this, AM_MASTER_DEAD);
					m_effectList.erase(it);
				}
				m_magicTargetEffect = NULL;
			}
			break;

		case	AM_MAKE_BEGIN_SPELL:
			if (m_beginSpellEffect == NULL)
			{
				m_beginSpellEffect = LaunchEffect(val1);
			}

			if (m_beginSpellEffect)
			{
				if (val1 > 0)
				{
					SendMsg(NULL, AM_DELETE_EFFECT, val1);
					m_beginSpellEffect = LaunchEffect(val1);
				}

				if (m_beginSpellEffect)
					m_beginSpellEffect->SendMsg(NULL, AM_SET_DURATION, val2);
			}
			break;
		case	AM_DELETE_BEGIN_SPELL:
			if (m_beginSpellEffect) {
				std::list<CRagEffect *>::iterator it = std::find(m_effectList.begin(), m_effectList.end(), m_beginSpellEffect);
				if (it != m_effectList.end()) {
					(*it)->SendMsg(this, AM_MASTER_DEAD);
					m_effectList.erase(it);
				}
				m_beginSpellEffect = NULL;
			}
			break;
		case	AM_EFFECT_DEAD:
			{
				if (sender == NULL)
					break;

				std::list<CRagEffect *>::iterator it = m_effectList.begin();
				while (it != m_effectList.end()) {
					if ((*it) == sender) {
						m_effectList.erase(it);

						if (m_beginSpellEffect==sender) {
							m_beginSpellEffect = NULL;
						} else if (m_magicTargetEffect==sender) {
							m_magicTargetEffect = NULL;
						}
						break;
					}
					++it;
				}
			}
			break;
		case	AM_DELETE_EFFECT:
			{
				std::list<CRagEffect *>::iterator it = m_effectList.begin();
				for(;it != m_effectList.end(); ++it) {
					if(!(*it)) {
						m_effectList.erase(it);
					} else if((*it)->GetType() == val1) {
						(*it)->SendMsg(this, AM_MASTER_DEAD);
						m_effectList.erase(it);
						return ;
					}
				}
			}
			break;
		case	AM_IS_HAVE_EFFECT:
			{
				int *efId = (int*)val1;
				std::list<CRagEffect *>::iterator it = m_effectList.begin();
				for(;it != m_effectList.end(); ++it) {
					if(!(*it)) {
						m_effectList.erase(it);
					} else if((*it)->GetType() == *efId) {
						*efId = -1;
						return;
					}
				}
			}
			break;
		default:
			CRenderObject::SendMsg(sender, msgid, val1, val2, val3);
			break;
	}
	}

int	CAbleToMakeEffect :: IsEffectExist(int EffectId)
{
	std::list<CRagEffect *>::iterator it = m_effectList.begin();
	for (; it != m_effectList.end(); ++it) {
		if (*it)
			if ((*it)->GetType() == EffectId)
				return 1;
	}
	return 0;
}

CGameActor :: CGameActor()
{
	OnInit();
}




CGameActor :: ~CGameActor()
{
	OnExit();
}

void	CGameActor :: OnInit()
{
	m_cursorEffect = NULL ;

	m_pkEffect = NULL;
	m_iceEffect = NULL;
	m_cartEffect = NULL;
	m_birdEffect = NULL;
	m_stunEffect = NULL;
	m_curseEffect = NULL;
	m_sleepEffect = NULL;
	m_crusisEffect = NULL;
	m_silenceEffect = NULL;
	m_angelusEffect = NULL;
	m_chatProhibitEffect = NULL;
	m_pos.Set(0,0,0);
	m_roty = 0;
	m_baseAction = 0;
	m_curAction = 0;
	m_curMotion = 0;
	m_stateId = ST_STAND;
	m_motionType = 0;
	m_motionSpeed = 1;
	m_shadowOn = true;
	m_accel.Set(0,0,0);
	m_dist = 0;
	m_gid = -1;
	m_lastServerTime = g_session.GetServerTime();
	m_lastProcessStateTime = timeGetTime();
	m_lastTlvertX = 0;
	m_lastTlvertY = 0;
	m_balloon = NULL;
	m_chatTitle = NULL;
	m_merchantShopTitle = NULL;
	m_skillRechargeGage = NULL;
	m_chatWidth = 0;
	m_nameWidth = 0;
	m_isMotionFinished = false;
	m_targetGid = 0;
	m_attackMotion = -1;
	m_pathStartCell = -1;
	m_shouldAddPickInfo = true;
	m_stateStartTick = timeGetTime();
	m_petEmotionStartTick = 0;
	m_skillRechargeEndTick = timeGetTime()-5000;
	m_skillRechargeStartTick = timeGetTime()-10000;
	m_effectLaunchCnt = 60*60;
	m_sprColor.color = 0xFFFFFFFF;
	m_isVisible = true;
	m_alwaysTopLayer = false;
	m_willBeDead = false;
	m_bodyState = 0;
	m_effectState = 0;
	m_healthState = 0;
	m_pkState = 0;
	m_vanishTime = 0;
	m_99 = 0;
	m_is99 = FALSE;

	m_isCounter = FALSE;
	m_isTrickDead = FALSE;
	m_isPlayHitWave = FALSE;
	memset(m_hitWaveName, 0, 128);
	m_gdid = 0;
	m_freezeEndTick = 0;
	m_actorType = 0;
	m_emblemWnd = NULL;
	m_WordDisplayWnd = NULL;
	m_isNeverAnimation = false;

	m_colorEndTick = 0;
	m_isAsuraAttack = FALSE;

	m_isBladeStop = FALSE;

	m_homunAI = NULL ;
	m_merAI = NULL ;

	m_objectType = UNKNOWN_TYPE ;
}

void	CGameActor :: OnExit()
{
	if (m_homunAI && m_homunAI->IsOpened())
	{
		m_homunAI->Close ( ) ;
		m_homunAI = NULL ;
	}
	if (m_merAI && m_merAI->IsOpened())
	{
		m_merAI->Close ( ) ;
		m_merAI = NULL ;
	}
	if (m_balloon) {
		g_windowMgr.PostQuit(m_balloon);
		m_balloon = NULL;
	}

	if (m_chatTitle) {
		g_windowMgr.PostQuit(m_chatTitle);
		m_chatTitle = NULL;
	}
	if (m_merchantShopTitle) {
		g_windowMgr.PostQuit(m_merchantShopTitle);
		m_merchantShopTitle = NULL;
	}
	if (m_skillRechargeGage) {
		g_windowMgr.PostQuit(m_skillRechargeGage);
		m_skillRechargeGage = NULL;
	}

	std::list<CMsgEffect *>::iterator it = m_msgEffectList.begin();
	while (it != m_msgEffectList.end()) {
		(*it)->SendMsg(NULL, AM_MSGMASTER_DEAD);
		++it;
	}
	m_msgEffectList.erase(m_msgEffectList.begin(), m_msgEffectList.end());

	DeleteEmblemWnd();
	DeleteWordDisplayWnd();
}

int		CGameActor :: Get8Dir(float rot)
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	int		dir = 0;
	if (gameMode)
	{
		float	longitude =  -gameMode->GetView()->GetCurLongitude() - rot;
		while (longitude < 0)
			longitude += 360;
		while (longitude >= 360)
			longitude -= 360;
		bool isReal8Dir = (IsThisPC(GetJob()) && (m_baseAction == 0 || m_baseAction == 8 || m_baseAction == 16))
			|| ((GetJob() > JT_WARPNPC && GetJob() < JT_MON_BEGIN) && m_baseAction == 0);

		isReal8Dir = isReal8Dir || (g_session.IsMer(GetJob()) && (m_baseAction == 0 || m_baseAction == 8 || m_baseAction == 16));
		if (isReal8Dir) {
			if (longitude < 22.5 || longitude > 360-22.5)
				dir = 0;
			else
				dir = int((longitude+22.5)/45);
		} else {
			dir = int(longitude/45);
		}
	}
	return	dir;
}

void	CGameActor :: CalcAccel(float speed)
{
	m_accel.x = speed * GetSin(m_roty);
	m_accel.z = -speed * GetCos(m_roty);
}

vector3d	CGameActor :: CalcAccel3d(vector3d to, float accel)
{
	float dist = CalcDist(to.x, to.y, to.z);
	float factor = accel / dist;
	vector3d accel3d;
	accel3d.x = (to.x - m_pos.x) * factor;
	accel3d.y = (to.y - m_pos.y) * factor;
	accel3d.z = (to.z - m_pos.z) * factor;
	return accel3d;
}



extern BOOL g_shiftKeyPressed;
extern BOOL g_isGoodSkillHanging;
void	CGameActor :: SetRenderInfo(RENDER_INFO_RECT &info, const float tlvertX, const float tlvertY)
{

	if (m_shouldAddPickInfo) {
		static int minRectLength = 40*g_renderer->GetScreenXFactor();

		float oowOffset = 0;
		if ((!g_shiftKeyPressed && g_isGoodSkillHanging) || (g_shiftKeyPressed && !g_isGoodSkillHanging)) {
			if (g_session.IsSiegeMode()) {
				if (IsThisPC(GetJob())) {
					if (g_session.IsMyGuild(GetGdid()))
						oowOffset = 400;
					else oowOffset = 100;
				} else if (GetJob() >= JT_WARPNPC && GetJob() < JT_MON_BEGIN)
					oowOffset = 100;
				else if (GetJob() > JT_MON_BEGIN && GetJob() < JT_2004_JOB_BEGIN) {
					if (IsPetHeadType(this)) {
						oowOffset = 50;
					} else
						oowOffset = 200;
				}
			#if defined (_HOMUN)
				else if ( g_session.IsMerOrHomun(GetJob()) )
					oowOffset = 200;
			#endif
			} else {
				if (IsThisPC(GetJob()))
					oowOffset = 400;
				else if (GetJob() >= JT_WARPNPC && GetJob() < JT_MON_BEGIN)
					oowOffset = 100;
				else if (GetJob() > JT_MON_BEGIN && GetJob() < JT_2004_JOB_BEGIN) {
					if (IsPetHeadType(this)) {
						oowOffset = 50;
					} else
						oowOffset = 200;
				}
			#if defined (_HOMUN)
				else if ( g_session.IsMerOrHomun(GetJob()) )
					oowOffset = 200;
			#endif
			}
		} else {
			if (g_session.IsSiegeMode()) {
				if (IsThisPC(GetJob())) {
					if (g_session.IsMyGuild(GetGdid()))
						oowOffset = 100;
					else oowOffset = 400;
				} else if (GetJob() >= JT_WARPNPC && GetJob() < JT_MON_BEGIN)
					oowOffset = 200;
				else if (GetJob() > JT_MON_BEGIN && GetJob() < JT_2004_JOB_BEGIN)
					if (IsPetHeadType(this)) {
						oowOffset = 50;
					} else
						oowOffset = 400;
			#if defined (_HOMUN)
				else if ( g_session.IsMerOrHomun(GetJob()) )
					oowOffset = 200;
			#endif
			} else {
				if (IsThisPC(GetJob()))
					oowOffset = 100;
				else if (GetJob() >= JT_WARPNPC && GetJob() < JT_MON_BEGIN)
					oowOffset = 200;
				else if (GetJob() > JT_MON_BEGIN && GetJob() < JT_2004_JOB_BEGIN)
					if (IsPetHeadType(this)) {
						oowOffset = 50;
					} else
						oowOffset = 400;
			#if defined (_HOMUN)
				else if ( g_session.IsMerOrHomun(GetJob()) )
					oowOffset = 200;
			#endif
			}
		}
		CActorPickInfo * actorPickInfo = new CActorPickInfo;
		actorPickInfo->m_vectors[0].x = (info.left<=info.right)?info.left:info.right;
		actorPickInfo->m_vectors[0].y = info.top;
		actorPickInfo->m_vectors[0].z = info.oow + oowOffset;
		actorPickInfo->m_vectors[1].x = (info.left>info.right)?info.left:info.right;
		actorPickInfo->m_vectors[1].y = info.bottom;
		actorPickInfo->m_vectors[1].z = info.oow + oowOffset;
		actorPickInfo->m_gid = m_gid;
		actorPickInfo->SetJob(GetJob());
		actorPickInfo->m_isPkState = m_pkState;
		if (GetJob() > JT_MON_BEGIN && GetJob() < JT_2004_JOB_BEGIN && IsPetHeadType(this)) {
			actorPickInfo->m_classType = CT_PET;
		}
#if defined (_HOMUN)
		else if ( g_session.IsMerOrHomun(GetJob()) )
			actorPickInfo->m_classType = CT_MER;
#endif
		else
			actorPickInfo->m_classType = CT_CGAMEACTOR;

#if defined (_MER_R2)
		if ( m_objectType == NPC_MERSOL_TYPE )
			actorPickInfo->m_classType = CT_MER ;
#endif

		int width = actorPickInfo->m_vectors[1].x - actorPickInfo->m_vectors[0].x;
		if (width < minRectLength) {
			actorPickInfo->m_vectors[1].x += (minRectLength - width)/2;
			actorPickInfo->m_vectors[0].x -= (minRectLength - width)/2;
		}
		int height = actorPickInfo->m_vectors[1].y - actorPickInfo->m_vectors[0].y;
		if (height < minRectLength) {
			actorPickInfo->m_vectors[1].y += (minRectLength - height)/2;
			actorPickInfo->m_vectors[0].y -= (minRectLength - height)/2;
		}

		g_actorPickNode.AddPickInfo(actorPickInfo);
	}
	SetTlvert(tlvertX, tlvertY);
}

void	CGameActor :: SetAction(int action, int speed, int type)
{
	CRenderObject::SetAction(action, speed, type);
	m_isNeverAnimation = false;
	m_attackMotion = -1;
}

void	CGameActor :: SetSex(int sex)
{
	 m_sex = sex & 0x01;
}


void	CGameActor :: ProcessActionWithDir()
{
	m_curAction = m_baseAction + Get8Dir(m_roty);
}


int		CGameActor :: GetAttackMotion()
{
	CActRes * actRes = GetActRes();
	int motionCount = actRes->GetMotionCount(m_curAction);
	for (int curMot=0; curMot<motionCount; curMot++) {
		CMotion * motion = actRes->GetMotion(m_curAction, curMot);
		if (motion->m_eventId == -1)
			continue;
		const char * eventName = actRes->GetEventName(motion->m_eventId);
		if (strcmp(eventName, "atk")==0)
			return curMot;
	}
	int r = motionCount-1;
	if (r)
		r -= 1;
	return r;
}

void	CGameActor :: RegisterPos()
{
	int sx, sy;
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	gameMode->GetServerCoor(m_pos.x, m_pos.z, sx, sy);
	gameMode->SendMsg(MM_ADDREGISTERPOS, m_gid, sx, sy);
}

void	CGameActor :: UnRegisterPos()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	gameMode->SendMsg(MM_DELETEREGISTERPOS, m_gid, 0, 0);
}

void	CGameActor :: AddDist(vector3d &lastPos, vector3d &pos)
{
	vector3d	progress;
	progress.x = m_pos.x - lastPos.x;
	progress.z = m_pos.z - lastPos.z;
	float	deltaDist = _sqrt(progress.x*progress.x + progress.z*progress.z);
	m_dist += deltaDist;
}

void	CGameActor :: ProcessMotionWithDist()
{
	int motCount = GetActRes()->GetMotionCount(m_curAction);
	m_curMotion = int(m_dist * 1.48f / float(m_motionSpeed)) % motCount;
}

void	CGameActor :: ProcessChatBalloon()
{
	static float hHover = float(81*g_renderer->m_height/480);
#ifndef		DEL_OLD_UI
	if (m_balloon) {
		if (timeGetTime() < m_chatTick+5000) {
			int nameX = m_lastTlvertX-m_chatWidth/2;
			int nameY = m_lastTlvertY-hHover*m_lastPixelRatio - m_chatHeight;

			if (nameX < -3)
				nameX = -3;
			if (nameX > g_renderer->m_width-m_balloon->GetWidth()+3)
				nameX = g_renderer->m_width-m_balloon->GetWidth()+3;
			if (nameY < -3)
				nameY = -3;
			if (nameY > g_renderer->m_height-m_balloon->GetHeight()+3)
				nameY = g_renderer->m_height-m_balloon->GetHeight()+3;
			m_balloon->Move(nameX, nameY);
		} else {
			g_windowMgr.PostQuit(m_balloon);
			m_balloon = NULL;
		}
	}
	if (m_chatTitle) {
		int nameX = m_lastTlvertX-100;
		int nameY = m_lastTlvertY-hHover*m_lastPixelRatio - 17;
		m_chatTitle->Move(nameX, nameY);
	}
	if (m_merchantShopTitle) {
		int nameX = m_lastTlvertX-96;
		int nameY = m_lastTlvertY-hHover*m_lastPixelRatio - 17 - 17;
		m_merchantShopTitle->Move(nameX, nameY - 20);
	}

	if (m_skillRechargeGage)
	{
		DWORD curTime = timeGetTime();
		if (timeGetTime() > m_skillRechargeEndTick)
		{
			SendMsg(NULL, AM_DELETE_SKILL_RECHARGEGAGE);
			if (m_stateId == 8)
			{
				SetState(ST_STAND);
			}
		}
		else
		{
			m_skillRechargeGage->Move(m_lastTlvertX-30, m_lastTlvertY-hHover*m_lastPixelRatio);
			int total = m_skillRechargeEndTick - m_skillRechargeStartTick;
			if (total < 0)
				total = 1;
			int amount = total - (m_skillRechargeEndTick - timeGetTime());
			if (amount > total)
				amount = total;
			m_skillRechargeGage->SetAmount(amount, total);
		}
	}
#endif
}

void CGameActor::DeleteChatBalloon() {
	if (m_balloon) {
		g_windowMgr.PostQuit(m_balloon);
		m_balloon = 0;
	}
}

void	CGameActor :: SetJob(int job)
{
	m_job = job;
	if (m_job == JT_CHICKEN)
		m_job = JT_KNIGHT;
	else if (m_job == JT_CHICKEN2)
		m_job = JT_CRUSADER;
	else if (m_job == JT_CHICKEN_H)
		m_job = JT_KNIGHT_H;
	else if (m_job == JT_CHICKEN_B)
		m_job = JT_KNIGHT_B;
	else if (m_job == JT_CHICKEN2_H)
		m_job = JT_CRUSADER_H;
	else if (m_job == JT_CHICKEN2_B)
		m_job = JT_CRUSADER_B;
	else if (m_job == JT_STAR2)
		m_job = JT_STAR;

	SetSprJob(GetJob());
}

int		CGameActor :: GetJob()
{
	if(m_job == JT_KNIGHT && IsEffectStateChicken(GetEffectState())) {
		return JT_CHICKEN;
	} else if (m_job == JT_CRUSADER && IsEffectStateChicken(GetEffectState())) {
		return JT_CHICKEN2;
	} else if (m_job == JT_KNIGHT_H && IsEffectStateChicken(GetEffectState())) {
		return JT_CHICKEN_H;
	} else if (m_job == JT_KNIGHT_B && IsEffectStateChicken(GetEffectState())) {
		return JT_CHICKEN_B;
	} else if (m_job == JT_CRUSADER_H && IsEffectStateChicken(GetEffectState())) {
		return JT_CHICKEN2_H;
	} else if (m_job == JT_CRUSADER_B && IsEffectStateChicken(GetEffectState())) {
		return JT_CHICKEN2_B;
	} else if (m_job == JT_STAR && IsEffectStateStar2(GetEffectState())) {
		return JT_STAR2;
	} else {
		return m_job;
	}
}

int CGameActor::IsThisPC(int job)
{
	return g_session.IsThisPC(job);
}

extern bool IsEventAID(DWORD aid);
void	CGameActor :: SetState(int stateId)
{
	if (stateId == -1)
		return;

	CGameMode * gameMode = g_modeMgr.GetGameMode();
	int rideActOffset = 0;
	if (m_stateId == ST_DAMAGE) {
		m_pos.x = m_damageDestX;
		m_pos.z = m_damageDestZ;
		m_pos.y = gameMode->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z);
	}
	if (stateId == ST_DAMAGE) {
		if (m_stateId == ST_MOVE) {
			m_stateId = ST_STAND;
		}
	}

	switch (stateId)
	{
	case ST_AUTOCOUNTER:
		{
#ifdef ACTION_V2






			if (g_session.IsPc(GetJob()))
			{
				int actIndex = g_session.GetSpriteAction( ST_ATTACK, GetWeapon(), m_bride, GetShield() ) ;
				SetAction(actIndex, 4, MT_ONETIME);
			}
			else
			{
				SetAction(rideActOffset+16, 4, MT_ONETIME);
			}
#else
			if (g_session.IsPc(GetJob()))
			{
				if (g_session.IsSecondAttack(GetJob(), GetSex(), GetWeapon()))
					SetAction(rideActOffset+88, 4, MT_ONETIME);
				else
					SetAction(rideActOffset+80, 4, MT_ONETIME);
			}
			else
			{
				SetAction(rideActOffset+16, 4, MT_ONETIME);
			}
#endif
				m_isMotionFreezed = true;
			}
			break;
		case ST_DAMAGE:
			{
#ifdef		SECONDJOB_SKILL2
				if (m_isForceState)
					return;
				if (m_isForceState2)
					return;
				if (m_isForceState3)
					return;
#endif
				CGameActor * tActor = NULL;
				if (m_targetGid == g_session.GetAid())
					tActor = gameMode->GetWorld()->m_player;
				else
					tActor = gameMode->GetWorld()->GetGameActorByAID(m_targetGid);

				if (tActor)
					tActor->SendMsg(NULL, AM_DELETE_MAGIC_TARGET);
			}
#ifndef	MOB_SKILL
			SendMsg(NULL, AM_DELETE_BEGIN_SPELL);
			SendMsg(NULL, AM_DELETE_SKILL_RECHARGEGAGE);
#endif

#ifdef ACTION_V2
			{
				int actIndex = g_session.GetSpriteAction( ST_DAMAGE, GetWeapon(), m_bride, GetShield() ) ;
				SetAction(actIndex, 5, MT_ONETIME);
			}
#else
			SetAction(rideActOffset+48, 5, MT_ONETIME);
#endif


			break;
		case ST_SKILL:
			if (m_stateId == ST_SIT)
				return;
		case ST_ATTACK:
		case ST_ATTACK2:
#ifdef ACTION_V2
			{
				int actIndex = g_session.GetSpriteAction( ST_ATTACK, GetWeapon() , m_bride, GetShield() ) ;
				SetAction(actIndex, 3, MT_ONETIME);
			}
#else
			if (ST_ATTACK2==stateId)
				SetAction(rideActOffset+88, 3, MT_ONETIME);
			else
				SetAction(rideActOffset+80, 3, MT_ONETIME);
#endif
			if (m_targetGid == g_session.GetAid()) {
				vector3d &pPos = gameMode->GetWorld()->m_player->m_pos;
				CalcDir(pPos.x, pPos.z);
			} else {
				CGameActor * targetActor = gameMode->GetWorld()->GetGameActorByAID(m_targetGid);
				if (targetActor)
					CalcDir(targetActor->m_pos.x, targetActor->m_pos.z);
			}
			if (GetJob() == JT_ARCHER_GUARDIAN) {
				MakeSpecialAttackEffect(m_targetGid, 28.0f);
			}
			m_attackMotion = GetAttackMotion();
			break;
		case	ST_DEAD:
#ifdef		SECONDJOB_SKILL2
			m_isForceState = false;
			m_isForceState2 = false;
			m_isForceState3 = false;
#endif
			m_isMotionFreezed = false;
			if (m_bodyState == BODYSTATE_FREEZING) {
				SendMsg(NULL, AM_DELETE_ICEEFFECT);
				SendMsg(NULL, AM_MAKE_ICEBREAKE_FFECT);
			}
			m_isLieOnGround = true;
			m_isMotionFinished = false;
			m_isMotionFreezed = false;
#ifdef ACTION_V2
			{
				int actIndex = g_session.GetSpriteAction( ST_DEAD, GetWeapon(), m_bride, GetShield() ) ;
				SetAction(actIndex, 4, MT_ONETIME);
			}
#else
			SetAction(rideActOffset+64, 4, MT_ONETIME);
#endif
			break;
		case	ST_STAND:

#ifdef ACTION_V2
			{
				int actIndex = g_session.GetSpriteAction( ST_STAND, GetWeapon(), m_bride, GetShield() ) ;
				SetAction(actIndex, 4, MT_STATIC);
			}
#else
		SetAction(rideActOffset+0, 4, MT_LOOP);
			if (!IsEventAID(m_gid))
				m_isMotionFinished = true;
#endif
			break;
		case	ST_MOVE:
			if (!m_isAsuraAttack)
			{
#ifdef SEQ_ANIM_ZL
				if (false != m_isMotionFinished)
				{
					if (m_baseAction == rideActOffset+8)
					{
						break;
					}
				}
#endif
#ifdef ACTION_V2
				int actIndex = g_session.GetSpriteAction( ST_MOVE, GetWeapon(), m_bride, GetShield() ) ;
				SetAction(actIndex, 6, MT_STATIC);
#else
				SetAction(rideActOffset+8, 6, 0);
#endif
			}
			break;
		case	ST_PICKUP:
			m_isCounter = FALSE;
			m_isMotionFreezed = false;
#ifdef ACTION_V2
			{
				int actIndex = g_session.GetSpriteAction( ST_PICKUP, GetWeapon(), m_bride, GetShield() ) ;
				SetAction(actIndex, 6, MT_ONETIME);
			}
#else
			SetAction(rideActOffset+24, 6, MT_ONETIME);
#endif
			break;
		case	ST_ATTACKWAIT:
#ifdef ACTION_V2
			{
				int actIndex = g_session.GetSpriteAction( ST_ATTACKWAIT, GetWeapon(), m_bride, GetShield() ) ;
				SetAction(actIndex, 6, MT_STATIC);
			}
#else
			SetAction(rideActOffset+32, 6, MT_ONETIME);
#endif
			break;
		case	ST_THROW:
#ifdef ACTION_V2
			{
				int actIndex = g_session.GetSpriteAction( ST_THROW, GetWeapon(), m_bride, GetShield() ) ;
				SetAction(actIndex, 6, MT_ONETIME);
			}
#else
			SetAction(rideActOffset+40, 3, MT_ONETIME);
#endif
			break;
#ifdef	SECONDJOB_SKILL2
		case	ST_DANCE:
			SetForceAnimSpeed(10);
			SetForceAnimation(rideActOffset+96, 1 , 3, rideActOffset+0, 0, TRUE, 99999999, TRUE);
			break;
		case	ST_SING:
			SetForceAnimSpeed(10);
			if (FALSE)
			{
				SetForceAnimation(rideActOffset+128, 1 , 3,rideActOffset+128, 1, TRUE, 99999999, TRUE);
			}
			else
				SetForceAnimation(96, 1 , 3, 96, 1, TRUE, 99999999, TRUE);
			break;
		case	ST_READYSHOT:
			SetForceAnimSpeed(10);
			SetForceAnimation(rideActOffset+88, 0 , 0,rideActOffset+88, 0, TRUE, 3000, FALSE);
			break;
		case	ST_THROW2:
			SetForceAnimSpeed(10);
			SetForceAnimation2(rideActOffset+40, 0, rideActOffset+40, 1, rideActOffset+40, 2, rideActOffset+40, 3, rideActOffset+40, 4, TRUE, 1000, FALSE);
			break;
		case	ST_READYJUMP:
			SetForceAnimSpeed(10);
			SetForceAnimation2(rideActOffset+40, 0, rideActOffset+40, 0, rideActOffset+40, 0, rideActOffset+40, 1, rideActOffset+40, 1, TRUE, 3000, FALSE);
			while (IsEffectExist(EF_LANDBODY))
				SendMsg(NULL, AM_DELETE_EFFECT, EF_LANDBODY);
			while (IsEffectExist(EF_JUMPBODY))
				SendMsg(NULL, AM_DELETE_EFFECT, EF_JUMPBODY);
			LaunchEffect(EF_JUMPBODY);
			LaunchEffect(EF_PEONG);
			break;
		case	ST_LANDJUMP:
			SetForceAnimSpeed(10);
			SetForceAnimation2(rideActOffset+24, 1, rideActOffset+24, 1, rideActOffset+24, 1, rideActOffset+24, 1, 8, 5, TRUE, 3000, FALSE);
			break;
		case	ST_R_STORM:
			SetForceAnimSpeed(10);
			SetForceAnimation(rideActOffset+96, 0 , 0, rideActOffset+96, 0, TRUE, 2000, FALSE);
			LaunchEffect(EF_TAE_READY);
			break;
		case	ST_A_STORM:
			SetForceAnimSpeed(10);
			SetForceAnimation(rideActOffset+96, 1 , 1, rideActOffset+96, 1, TRUE, 2000, FALSE);
			break;
		case	ST_R_DOWN:
			SetForceAnimSpeed(10);
			SetForceAnimation(rideActOffset+96, 2 , 2, rideActOffset+96, 2, TRUE, 2000, FALSE);
			LaunchEffect(EF_TAE_READY);
			break;
		case	ST_SG_DOWN:
			SetForceAnimSpeed(10);
			SetForceAnimation(rideActOffset+96, 2 , 2, rideActOffset+96, 2, TRUE, 1000, FALSE);
			break;
		case	ST_A_DOWN:
			SetForceAnimSpeed(5);
			SetForceAnimation2(rideActOffset+80, 3,rideActOffset+80, 5,rideActOffset+80, 7,rideActOffset+80, 8,rideActOffset+80, 2, TRUE, 500, FALSE);
			break;
		case	ST_R_TURN:
			SetForceAnimSpeed(10);
			SetForceAnimation(rideActOffset+96, 3 , 3, rideActOffset+96, 3, TRUE, 2000, FALSE);
			LaunchEffect(EF_TAE_READY);
			break;
		case	ST_SG_TURN:
			SetForceAnimSpeed(10);
			SetForceAnimation(rideActOffset+96, 3 , 3, rideActOffset+96, 3, TRUE, 1000, FALSE);
			break;
		case	ST_A_TURN:
			SetForceAnimSpeed(5);
			SetForceAnimation2(rideActOffset+88, 4, rideActOffset+88, 5, rideActOffset+88, 6, rideActOffset+88, 7, rideActOffset+88, 2, TRUE, 500, FALSE);
			break;
		case	ST_R_COUNTER:
			SetForceAnimSpeed(10);
			SetForceAnimation(rideActOffset+96, 4 , 4, rideActOffset+96, 4, TRUE, 2000, FALSE);
			LaunchEffect(EF_TAE_READY);
			break;
		case	ST_A_COUNTER:
			SetForceAnimSpeed(4);
			if ( GetJob() == JT_LINKER )
				SetForceAnimation2(rideActOffset+88, 4, rideActOffset+88, 5,rideActOffset+ 88, 6,rideActOffset+ 88, 7,rideActOffset+ 88, 2, TRUE, 500, FALSE);
			else
				SetForceAnimation2(rideActOffset+40, 2,rideActOffset+ 40, 3, rideActOffset+40, 4, rideActOffset+40, 4, rideActOffset+40, 4, TRUE, 500, FALSE);
			break;
		case	ST_R_DODGE:
			SetForceAnimSpeed(10);
			SetForceAnimation(rideActOffset+24, 1 , 1, rideActOffset+24, 1, TRUE, 2000, FALSE);
			LaunchEffect(EF_TAE_READY);
			break;
		case	ST_A_JUMPKICK:
			SetForceAnimSpeed(10);
			SetForceAnimation(rideActOffset+96,5,5,rideActOffset+96,5, TRUE, 3000, FALSE);
			break;
		case	ST_NINJASKILL1:
			SetForceAnimSpeed(6);
			SetForceAnimation(rideActOffset+96, 0 , 1, rideActOffset+96, 1, TRUE, 500, FALSE);
			break;
		case	ST_NINJASKILL2:
			SetForceAnimSpeed(6);
			SetForceAnimation(rideActOffset+96, 2 , 5, rideActOffset+96, 5, TRUE, 1000, FALSE);
			break;
		case	ST_NINJAREADY:
			SetForceAnimSpeed(10);
			if (random(2) == 0)
				SetForceAnimation(rideActOffset+96, 2 , 2, rideActOffset+96, 2, TRUE, 500, FALSE);
			else
				SetForceAnimation(rideActOffset+96, 3 , 3, rideActOffset+96, 3, TRUE, 500, FALSE);
			break;
		case	ST_GUNSKILL1:
			SetForceAnimSpeed(6);
			if (random(2) == 0)
				SetForceAnimation(rideActOffset+96, 0 , 3, rideActOffset+96, 3, TRUE, 1000, FALSE);
			else
				SetForceAnimation2(rideActOffset+96, 3, rideActOffset+96, 2, rideActOffset+96, 1, rideActOffset+96, 0, rideActOffset+96, 0, TRUE, 1000, FALSE);
			break;
		case	ST_GUNSKILL2:
			SetForceAnimSpeed(6);
			SetForceAnimation(rideActOffset+96, 4 , 5, rideActOffset+96, 5, TRUE, 500, FALSE);
			break;
		case	ST_GUNSKILL3:
			SetForceAnimSpeed(6);
			SetForceAnimation(rideActOffset+40, 0 , 2,rideActOffset+40, 2, TRUE, 500, FALSE);
			break;
		case	ST_GUNREADY:
			SetForceAnimSpeed(10);
			if (random(2) == 0)
				SetForceAnimation(rideActOffset+96, 1 , 1, rideActOffset+96, 1, TRUE, 500, FALSE);
			else
				SetForceAnimation(rideActOffset+96, 2 , 2, rideActOffset+96, 2, TRUE, 500, FALSE);
			break;
		case	ST_GUNREADY2:
			SetForceAnimSpeed(10);
			SetForceAnimation(rideActOffset+40, 0 , 0,rideActOffset+40, 0, TRUE, 500, FALSE);
			break;
#endif
	}

	m_curAction = m_baseAction + Get8Dir(m_roty);
	if ( m_stateId == ST_STAND || m_stateId == ST_SIT )
		m_oldstateId = m_stateId ;

	if (stateId != ST_DAMAGE)
		m_stateId = stateId;
	m_stateStartTick = timeGetTime();
}
bool	CGameActor :: ProcessVanish()
{
	if (m_vanishTime) {
		DWORD curTime = timeGetTime();
		if (curTime > m_vanishTime) {
			return false;
		}
		int alpha = (m_vanishTime - curTime) / 2;
		if (alpha > 255) {
			alpha = 255;
		}
		SetArgb(alpha, -1, -1, -1);
	}
	return true;
}

void	CGameActor :: SetVanish()
{
	m_gid = -1;
	m_99 = 0;
	m_shadowOn = FALSE;
	m_vanishTime = timeGetTime() + 6120/12;
	if (m_vanishTime == 0) {
		m_vanishTime = 1;
	}
}


bool	CGameActor :: ProcessState()
{
	if (m_freezeEndTick)
	{
		if (timeGetTime() > m_freezeEndTick)
		{
			m_isMotionFreezed = false;
			m_freezeEndTick = 0;
		}
	}

	if (!ProcessVanish())
	{
		return false;
	}

	CGameMode * gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);
	CWorld * world = gameMode->GetWorld();
	DWORD serverTime = g_session.GetServerTime();
	BOOL posChanged = FALSE;

	switch (m_stateId) {
		case	ST_AUTOCOUNTER:
			{
				if (!m_isCounter) {
					m_isCounter = TRUE;
					if (g_session.IsPc(GetJob())) {
						int sex = m_sex;
						if (g_session.IsSecondAttack(GetJob(), sex, GetWeapon()))
							SetAction(88, 4, MT_ONETIME);
						else
							SetAction(80, 4, MT_ONETIME);
					} else {
						SetAction(16, 4, MT_ONETIME);
					}
					m_isMotionFreezed = true;
				}
				DWORD Time = timeGetTime();
				if (timeGetTime() >= m_skillRechargeEndTick) {
					m_isCounter = FALSE;
					m_isMotionFreezed = false;
					SetState(ST_STAND);
					SendMsg(NULL, AM_DELETE_SKILL_RECHARGEGAGE);
				}
				ProcessMotion();
			}
			break;
		case	ST_STAND:

			if ((m_motionType == MT_ONETIME && m_isMotionFinished))
			{
				SetState(ST_STAND);
			}
			else if (GetJob() >= JT_MON_BEGIN && GetJob() < JT_MONSTER_LAST && GetHeadType() == HEADTYPE_LAST) {
				if (m_gid == g_session.m_petGID && m_petEmotionStartTick && timeGetTime() - m_petEmotionStartTick > 3000) {
					int friendly, hunger, emotion, talk;

					if ( rand() % 100 < 25 )
					{
						hunger = g_session.GetPetHungryState(g_session.m_petHungry);
						friendly = g_session.GetPetFriendlyState(g_session.m_petFriendly);

						emotion = g_session.GetPetEmotion(hunger, friendly, PM_NORMAL);
						talk = g_session.GetPetTalkNumber(g_session.m_petJob, PM_NORMAL, hunger);
						gameMode->SendMsg(MM_PET_ACT, talk);
						gameMode->SendMsg(MM_PET_ACT, emotion);
					}
					m_petEmotionStartTick = 0;
				}
			}
			ProcessMotion();
			break;
		case	ST_MOVE:
			{

				DWORD nextCellTime;
				float nCY;
				float nCX;
				float nBY;
				float nBX;
				int blockDeltaTime;
				vector3d	lastPos = m_pos;
				int deltaTime;

				m_pathStartCell = m_path.GetNextCellInfo(serverTime, nextCellTime, nCX, nCY);

				gameMode->GetClientCoor(nCX, nCY, nBX, nBY);

				blockDeltaTime = nextCellTime - m_lastServerTime;
				deltaTime = serverTime - m_lastServerTime;
				if (blockDeltaTime<0)
					blockDeltaTime = 0;
				if (deltaTime<0)
					deltaTime = 0;
				if (blockDeltaTime > 0 && deltaTime > 0) {
					vector3d deltaPos;
					deltaPos.x = nBX - lastPos.x;
					deltaPos.z = nBY - lastPos.z;
					deltaPos.x *= deltaTime;
					deltaPos.x /= blockDeltaTime;
					deltaPos.z *= deltaTime;
					deltaPos.z /= blockDeltaTime;

					m_pos.x = lastPos.x + deltaPos.x;
					m_pos.z = lastPos.z + deltaPos.z;









					posChanged = TRUE;

				}

				if (!m_isNeverAnimation)
				{
					if ((lastPos.x != m_pos.x) || (lastPos.z != m_pos.z))
						m_roty = CalcDir(lastPos.x, lastPos.z, m_pos.x, m_pos.z);
				}

				if (!m_isNeverAnimation)
				{
					ProcessActionWithDir();
				}

				AddDist(lastPos, m_pos);

				if (!m_isNeverAnimation)
				{
					ProcessMotionWithDist();
				}
				if (m_pathStartCell == -1 && serverTime>=nextCellTime)
				{
					{
						m_pos.x = nBX;
						m_pos.z = nBY;

						posChanged = TRUE;
						SetState(ST_STAND);
						m_isNeverAnimation = false;
						m_isMotionFreezed = false;
	#ifdef	SECONDJOB_SKILL2

						if (m_isAsuraAttack)
						{
							m_isAsuraAttack = FALSE;
							SetForceAnimation(96, 2, 2, 96, 2, TRUE, 3000, FALSE);
						}
	#endif
					}
				}

			}
			break;
		case	ST_ATTACKWAIT:
			{


				ProcessMotion();
				if (m_isMotionFinished)
				{
					SetState(ST_ATTACKWAIT);
				}
				else
				{
				}
			}
			break;
		case	ST_BEGIN_SKILL:
			{
				m_isCounter = FALSE;
				ProcessMotion();
				float stateCnt = float(timeGetTime() - m_stateStartTick) / 24.0f;
				if (m_targetGid && (int(stateCnt) % 34 == 0)) {
					CGameMode * gameMode = g_modeMgr.GetGameMode();
					CGameActor * tActor = gameMode->GetWorld()->GetGameActorByAID(m_targetGid);
					if (tActor) {
						CalcDir(tActor->m_pos.x, tActor->m_pos.z);
					}
				}
			}
			break;
		case	ST_SKILL:
			{
				m_isCounter = FALSE;
				ProcessMotion();
				if (m_isMotionFinished)
					SetState(ST_STAND);
			}
			break;
		case	ST_PICKUP:
			m_isCounter = FALSE;
			ProcessMotion();
			if (m_isMotionFinished)
				SetState(ST_STAND);
			break;
		case	ST_DAMAGE:
			{
				m_isCounter = FALSE;
				ProcessMotion();
				float tick = m_stateStartTick;
				vector3d	prevPos = m_pos;
				vector3d	deltaPos;
				deltaPos.x = m_damageDestX - m_pos.x;
				deltaPos.z = m_damageDestZ - m_pos.z;
				DWORD deltaTime = m_stateStartTick + 200 - timeGetTime();

				int dmgDeltaTime = timeGetTime() - m_lastProcessStateTime;
				if (dmgDeltaTime < 0)
					dmgDeltaTime = 0;

				if (deltaTime > 0)
				{
					m_pos.x = prevPos.x + (deltaPos.x * dmgDeltaTime / deltaTime);
					m_pos.z = prevPos.z + (deltaPos.z * dmgDeltaTime / deltaTime);
					posChanged = TRUE;
				}

				if (m_isMotionFinished)
				{
					m_pos.x = m_damageDestX;
					m_pos.z = m_damageDestZ;
					posChanged = TRUE;
					SetState(ST_STAND);
				}
			}
			break;
		case	ST_DEAD:
#ifdef		SECONDJOB_SKILL2
			m_isForceState = false;
			m_isForceState2 = false;
			m_isForceState3 = false;
#endif
			m_isMotionFreezed = false;
			m_isCounter = FALSE;
			ProcessMotion();
			if (!m_isMotionFinished || m_willBeAttackedList.size())
			{
				break;
			}
			gameMode->SendMsg(MM_DELETEREGISTERPOS, m_gid);

			if ((GetJob()>JT_USER_LAST && GetJob()<JT_2004_JOB_BEGIN)
#if defined (_MER_R2)
				|| m_objectType == NPC_MERSOL_TYPE
#endif
				|| g_session.IsMerOrHomun(GetJob()) )
			{
				MakeCorpse();
				return 0;
			}

			if ( GetActRes() == 0 )
			{
				break;
			}

			break;
		case	ST_ATTACK:
		case	ST_ATTACK2:
		case	ST_THROW:
			ProcessMotion();
			if (m_isMotionFinished) {
				SetState(ST_STAND);
			}
			break;
		case	ST_DANCE:
		case	ST_SING:
		case	ST_READYSHOT:
		case	ST_THROW2:
		case	ST_READYJUMP:
		case	ST_LANDJUMP:
		case	ST_R_STORM:
		case	ST_A_STORM:
		case	ST_R_DOWN:
		case	ST_SG_DOWN:
		case	ST_A_DOWN:
		case	ST_R_TURN:
		case	ST_SG_TURN:
		case	ST_A_TURN:
		case	ST_R_COUNTER:
		case	ST_A_COUNTER:
		case	ST_R_DODGE:
		case	ST_A_JUMPKICK:
		case	ST_NINJASKILL1:
		case	ST_NINJASKILL2:
		case	ST_NINJAREADY:
		case	ST_GUNSKILL1:
		case	ST_GUNSKILL2:
		case	ST_GUNSKILL3:
		case	ST_GUNREADY:
		case	ST_GUNREADY2:
			m_isCounter = FALSE;
			ProcessMotion();
			break;
		case	ST_SLEEP:
			m_isCounter = FALSE;
			break;
	}
	m_lastProcessStateTime = timeGetTime();
	m_lastServerTime = serverTime;
	if (posChanged)
		m_pos.y = world->m_attr->GetHeight(m_pos.x, m_pos.z);
	return true;
}
void	CGameActor :: MakeCorpse()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);
	gameMode->MakeCorpse(m_pos.x, m_pos.y, m_pos.z, GetActName(), GetSprName(), m_baseAction, m_curMotion, m_roty);
}

void	CGameActor :: ProcessWillBeAttacked()
{
	DWORD curTime = timeGetTime();
	std::list<WBA>::iterator it = m_willBeAttackedList.begin();
	while (it != m_willBeAttackedList.end())
	{
		WBA &wba = *it;
		if (curTime >= wba.time)
		{
			SendMsg(NULL, wba.message, int(&wba), 0, 0);
			it = m_willBeAttackedList.erase(it);
		}
		else
			it++;
	}
}

void	CGameActor :: MakeSpecialAttackEffect(DWORD targetGid, float hideTime)
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);

	switch (GetJob()) {
		case	JT_ARCHER_GUARDIAN:
		case	JT_ARCHER_SKELETON:
		case	JT_ORC_ARCHER:
		case	JT_GOBLIN_ARCHER:
		case	JT_RAYDRIC_ARCHER:
		case	JT_KOBOLD_ARCHER:
		case	JT_GARGOYLE:

		case	JT_G_ARCHER_SKELETON:
		case	JT_G_ORC_ARCHER:
		case	JT_G_RAYDRIC_ARCHER:
		case	JT_G_KOBOLD_ARCHER:

		case	JT_PARASITE:
		case	JT_STONE_SHOOTER:
		case	JT_WOOTAN_SHOOTER:
		case	JT_LIVE_PEACH_TREE:
		case	JT_EVIL_CLOUD_HERMIT:
		case	JT_G_PARASITE:
		case	JT_G_LIVE_PEACH_TREE:
		case	JT_G_EVIL_CLOUD_HERMIT:
		case	JT_RAGGED_ZOMBIE:

		case	JT_POTON_CANON:
		case	JT_POTON_CANON_1:
		case	JT_POTON_CANON_2:
		case	JT_POTON_CANON_3:
		case	JT_LADY_TANEE:

		case	JT_MER_ARCHER01:
		case	JT_MER_ARCHER02:
		case	JT_MER_ARCHER03:
		case	JT_MER_ARCHER04:
		case	JT_MER_ARCHER05:
		case	JT_MER_ARCHER06:
		case	JT_MER_ARCHER07:
		case	JT_MER_ARCHER08:
		case	JT_MER_ARCHER09:
		case	JT_MER_ARCHER10:
			gameMode->MakeArrowEffect(m_pos, targetGid, hideTime, GetJob());
			break;
		case	JT_MANDRAGORA:
		case	JT_HYDRA:
		case	JT_THA_ODIUM:
		case	JT_G_THA_ODIUM:
		case	JT_DROSERA:
		case	JT_MAVKA:
		case	JT_G_MAVKA:

		case	JT_ENTWEIHEN:
		case	JT_G_ENTWEIHEN_R:
		case	JT_G_ENTWEIHEN_H:
		case	JT_G_ENTWEIHEN_M:
		case	JT_G_ENTWEIHEN_S:
			gameMode->MakeMandraAttackEffect(targetGid, GetJob(), 0);
			break;
	}
}

void	CGameActor :: DeleteMatchingEffect(CMsgEffect *msgEffect)
{
	if (m_cartEffect == msgEffect) {
		m_cartEffect = NULL;
	} else if (m_birdEffect == msgEffect) {
		m_birdEffect = NULL;
	} else if (m_pkEffect == msgEffect) {
		m_pkEffect = NULL;
	} else if (m_iceEffect == msgEffect) {
		m_iceEffect = NULL;
	} else if (m_curseEffect == msgEffect) {
		m_curseEffect = NULL;
	} else if (m_sleepEffect == msgEffect) {
		m_sleepEffect = NULL;
	} else if (m_stunEffect == msgEffect) {
		m_stunEffect = NULL;
	} else if (m_angelusEffect == msgEffect) {
		m_angelusEffect = NULL;
	} else if (m_crusisEffect == msgEffect) {
		m_crusisEffect = NULL;
	} else if (m_silenceEffect == msgEffect) {
		m_silenceEffect = NULL;
	} else if (m_chatProhibitEffect == msgEffect) {
		m_chatProhibitEffect = NULL;
	} else if (m_cursorEffect == msgEffect) {
		m_cursorEffect = NULL;
	}
}

int	CGameActor :: GetProperJob(int job, int effectState)
{
	if (job == 7 && IsEffectStateChicken(effectState))
		return 13;
	if (job == 14 && IsEffectStateChicken(effectState))
		return 21;
	if (job == 0xfa8 && IsEffectStateChicken(effectState))
		return 0xfae;
	if (job == 0xfbe && IsEffectStateChicken(effectState))
		return 0xfc4;
	if (job == 0xfaf && IsEffectStateChicken(effectState))
		return 0xfb6;
	if (job == 0xfc5 && IsEffectStateChicken(effectState))
		return 0xfcc;
	if (job == 0xfcf && IsEffectStateStar2(effectState))
		return 0xfd0;
	return job;
}

void CGameActor :: MakePartyEffect(int AID)
{
	CGameMode *curMode = g_modeMgr.GetGameMode();
	if (curMode->GetWorld()->IsPKZone() || curMode->GetWorld()->IsEventPVPMode()) {
		for (int a = 0; a < g_session.GetNumParty(); a++) {
			FRIEND_INFO fi = g_session.GetPartyInfoBySequence(a);
			if (fi.AID == AID) {
				if (!IsEffectExist(EF_PARTY))
					LaunchEffect(EF_PARTY);
				break;
			}
		}
	}
}

void	CGameActor :: SetGuildInfo(int gdid, int emblemVersion)
{
	if (gdid < 1) return;
	if (emblemVersion < 1 && GetJob() != 0x2d2) return;
	if (!g_session.IsPc(GetJob()) && !g_session.IsGuardian(GetJob()) && GetJob() != 0x2d2)
		return;
	m_gdid = gdid;
	m_emblemVersion = emblemVersion;
	char emblemResName[256];
	g_session.GetEmblemFileName(emblemResName, m_gdid, m_emblemVersion);
	if (!g_resMgr().IsExist(emblemResName))
		g_modeMgr.GetCurMode()->SendMsg(0x8b, m_gdid, 0, 0);
}
void	CGameActor :: SetGroupMemberAndVisible(BOOL bIsMemberAndVisible)
{
	m_bIsMemberAndVisible = bIsMemberAndVisible;
}

int doubleAttackTerm = 200;

void	CGameActor :: SendMsg(CGameObject *sender, int message, int arg1, int arg2, int arg3)
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);

	switch (message) {
		case	AM_DELETE_PROHIBITEFFECT:
			{
				if (m_chatProhibitEffect) {
					g_modeMgr.GetGameMode()->GetWorld()->RemoveGameObject(m_chatProhibitEffect);
				}
			}
			break;
		case	AM_MSGEFFECT_DEAD:
			{
				std::list<CMsgEffect *>::iterator it = std::find(m_msgEffectList.begin(), m_msgEffectList.end(), (CMsgEffect *)sender);
				if (it != m_msgEffectList.end()) {
					CMsgEffect * msgEffect = *it;
					DeleteMatchingEffect(msgEffect);
					m_msgEffectList.erase(it);
				}
			}
			break;
		case	AM_SET_TARGET_AID:
			{
				m_targetGid = arg1;
			}
			break;
		case	AM_MAKE_NUMBER:
			{
				Am_Make_Number(sender, arg1, arg2, arg3);
			}
			break;
		case	AM_MVP:
			{
				CRagEffect *mvpEffect = LaunchEffect(EF_MVP);
				if (mvpEffect)
					mvpEffect->SendMsg(this, AM_SET_MASTER_POINTER);
			}
			break;
		case	AM_MAKE_MISS:
			{
				if (g_session.m_bMakeMissEffect) {
					int sprArgb = arg1;
					CMsgEffect * missEffect = gameMode->MakeMissEffect(m_pos, sprArgb, m_gid);
					m_msgEffectList.push_back(missEffect);
					missEffect->SendMsg(this, AM_SET_MASTER_POINTER);
				}
			}
			break;
		case	AM_MAKE_GUARD:
			{
				LaunchEffect(EF_GUARD);
			}
			break;
		case	AM_MAKE_LUCKY:
			{
				CMsgEffect * luckyEffect = gameMode->MakeMsgEffect(MAKETYPE_LUCKY_EFFECT, m_pos, m_gid);
				m_msgEffectList.push_back(luckyEffect);
				luckyEffect->SendMsg(this, AM_SET_MASTER_POINTER);
			}
			break;
		case	AM_EMOTION:
			{
				if (arg1 != ET_CHAT_PROHIBIT) {
					CMsgEffect * emotionEffect = gameMode->MakeEmotion(m_pos, arg1, m_gid);
					m_msgEffectList.push_back(emotionEffect);
					emotionEffect->SendMsg(this, AM_SET_MASTER_POINTER);
				} else if(!m_chatProhibitEffect) {
					m_chatProhibitEffect = gameMode->MakeEmotion(m_pos, arg1, m_gid);
					m_msgEffectList.push_back(m_chatProhibitEffect);
					m_chatProhibitEffect->SendMsg(this, AM_SET_MASTER_POINTER);
				}
			}
			break;
		case	AM_MAKE_CART:
			{
				if (IsEffectStateBurrow(m_effectState) || IsEffectStateHiding(m_effectState)) break;

				CMsgEffect * cartEffect;
				if (m_cartEffect == NULL) {
					switch (m_effectState)
					{
						case EFFECTSTATE_PUSHCART:
							if (GetJob() == JT_SUPERNOVICE)
								cartEffect = gameMode->MakeMsgEffect(MAKETYPE_CART_EFFECT6, m_pos, m_gid);
							else if (GetJob() == JT_SUPERNOVICE_B)
								cartEffect = gameMode->MakeMsgEffect(MAKETYPE_CART_EFFECT16, m_pos, m_gid);
							else if (GetJob() == JT_MERCHANT_B || GetJob() == JT_BLACKSMITH_B || GetJob() == JT_ALCHEMIST_B)
								cartEffect = gameMode->MakeMsgEffect(MAKETYPE_CART_EFFECT11, m_pos, m_gid);
							else
								cartEffect = gameMode->MakeMsgEffect(MAKETYPE_CART_EFFECT, m_pos, m_gid);
							break;

						case EFFECTSTATE_PUSHCART2:
							if (GetJob() == JT_MERCHANT_B || GetJob() == JT_BLACKSMITH_B || GetJob() == JT_ALCHEMIST_B)
								cartEffect = gameMode->MakeMsgEffect(MAKETYPE_CART_EFFECT12, m_pos, m_gid);
							else
								cartEffect = gameMode->MakeMsgEffect(MAKETYPE_CART_EFFECT2, m_pos, m_gid);
							break;

						case EFFECTSTATE_PUSHCART3:
							if (GetJob() == JT_MERCHANT_B || GetJob() == JT_BLACKSMITH_B || GetJob() == JT_ALCHEMIST_B)
								cartEffect = gameMode->MakeMsgEffect(MAKETYPE_CART_EFFECT13, m_pos, m_gid);
							else
								cartEffect = gameMode->MakeMsgEffect(MAKETYPE_CART_EFFECT3, m_pos, m_gid);
							break;

						case EFFECTSTATE_PUSHCART4:
							if (GetJob() == JT_MERCHANT_B || GetJob() == JT_BLACKSMITH_B || GetJob() == JT_ALCHEMIST_B)
								cartEffect = gameMode->MakeMsgEffect(MAKETYPE_CART_EFFECT14, m_pos, m_gid);
							else
								cartEffect = gameMode->MakeMsgEffect(MAKETYPE_CART_EFFECT4, m_pos, m_gid);
							break;

						case EFFECTSTATE_PUSHCART5:
							if (GetJob() == JT_MERCHANT_B || GetJob() == JT_BLACKSMITH_B || GetJob() == JT_ALCHEMIST_B)
								cartEffect = gameMode->MakeMsgEffect(MAKETYPE_CART_EFFECT15, m_pos, m_gid);
							else
								cartEffect = gameMode->MakeMsgEffect(MAKETYPE_CART_EFFECT5, m_pos, m_gid);
							break;

						default:
							cartEffect = gameMode->MakeMsgEffect(MAKETYPE_CART_EFFECT, m_pos, m_gid);
							break;
					}
					m_msgEffectList.push_back(cartEffect);
					cartEffect->SendMsg(this, AM_SET_MASTER_POINTER);
					m_cartEffect = cartEffect;
				}
			}
			break;
		case	AM_DELETE_CART:
			{
				if (m_cartEffect) {
					m_cartEffect->SendMsg(NULL, AM_DISAPPEAR);
					m_cartEffect = NULL;
				}
			}
			break;
		case	AM_MAKE_BIRD:
			{
				if (m_birdEffect == NULL)
				{
					CMsgEffect * birdEffect;
#if defined (_UPDATE2004B)
					if ( GetJob() == JT_HUNTER_H )
						birdEffect = gameMode->MakeMsgEffect(MAKETYPE_BIRD_EFFECT2, m_pos, m_gid);
					else if ( GetJob() == JT_HUNTER_B )
						birdEffect = gameMode->MakeMsgEffect(MAKETYPE_BIRD_EFFECT3, m_pos, m_gid);
					else if ( GetJob() == JT_GANGSI )
						birdEffect = gameMode->MakeMsgEffect(MAKETYPE_BIRD_EFFECT4, m_pos, m_gid);
					else if ( GetJob() == JT_DEATHKNIGHT )
						birdEffect = gameMode->MakeMsgEffect(MAKETYPE_BIRD_EFFECT5, m_pos, m_gid);
					else if ( GetJob() == JT_COLLECTOR )
						birdEffect = gameMode->MakeMsgEffect(MAKETYPE_BIRD_EFFECT6, m_pos, m_gid);
					else
#endif
						birdEffect = gameMode->MakeMsgEffect(MAKETYPE_BIRD_EFFECT, m_pos, m_gid);
					m_msgEffectList.push_back(birdEffect);
					birdEffect->SendMsg(this, AM_SET_MASTER_POINTER);
					birdEffect->m_motionSpeed = 6;
					m_birdEffect = birdEffect;
				}
			}
			break;
		case	AM_DELETE_BIRD:
			{
				if (m_birdEffect) {
					m_birdEffect->SendMsg(NULL, AM_DISAPPEAR);
					m_birdEffect = NULL;
				}
			}
			break;
		case	AM_MAKE_PKEFFECT:
			{
				if (m_pkEffect == NULL) {
					CMsgEffect * pkEffect = gameMode->MakeMsgEffect(MAKETYPE_PK_EFFECT, m_pos, m_gid);
					m_msgEffectList.push_back(pkEffect);
					pkEffect->SendMsg(this, AM_SET_MASTER_POINTER);
					m_pkEffect = pkEffect;
				}
			}
			break;
		case	AM_DELETE_PKEFFECT:
			{
				if (m_pkEffect) {
					m_pkEffect->SendMsg(NULL, AM_DISAPPEAR);
					m_pkEffect = NULL;
				}
			}
			break;

		case	AM_MAKE_ICEEFFECT:
			{
				if (m_iceEffect == NULL) {
					CMsgEffect * iceEffect = gameMode->MakeMsgEffect(MAKETYPE_ICE_EFFECT, m_pos, m_gid);
					m_msgEffectList.push_back(iceEffect);
					iceEffect->SendMsg(this, AM_SET_MASTER_POINTER);
					m_iceEffect = iceEffect;
				}
			}
			break;
		case	AM_SHAKE_ICEEFFECT:
			{
				if (m_iceEffect) {
					m_iceEffect->SendMsg(this, AM_SHAKE_ICEEFFECT);
				}
			}
			break;
		case	AM_MAKE_FREEZINGEFFECT:
			{
			}
		case	AM_DELETE_ICEEFFECT:
			{
				if (m_iceEffect) {
					m_iceEffect->SendMsg(NULL, AM_DISAPPEAR);
					m_iceEffect = NULL;
				}
			}
			break;
		case	AM_MAKE_CURSE_EFFECT:
			{
				if (m_curseEffect == NULL) {
					CMsgEffect * curseEffect = gameMode->MakeMsgEffect(MAKETYPE_CURSE_EFFECT, m_pos, m_gid);
					m_msgEffectList.push_back(curseEffect);
					curseEffect->SendMsg(this, AM_SET_MASTER_POINTER);
					m_curseEffect = curseEffect;
				}
			}
			break;
		case	AM_DELETE_CURSE_EFFECT:
			{
				if (m_curseEffect) {
					m_curseEffect->SendMsg(NULL, AM_DISAPPEAR);
					m_curseEffect = NULL;
				}
			}
			break;
		case	AM_MAKE_SLEEP_EFFECT:
			{
				if (m_sleepEffect == NULL) {
					CMsgEffect * sleepEffect = gameMode->MakeMsgEffect(MAKETYPE_SLEEP_EFFECT, m_pos, m_gid);
					m_msgEffectList.push_back(sleepEffect);
					sleepEffect->SendMsg(this, AM_SET_MASTER_POINTER);
					m_sleepEffect = sleepEffect;
					SetState(ST_SLEEP);
				}
			}
			break;
		case	AM_DELETE_SLEEP_EFFECT:
			{
				if (m_sleepEffect) {
					m_sleepEffect->SendMsg(NULL, AM_DISAPPEAR);
					m_sleepEffect = NULL;
					SetState ( m_oldstateId ) ;

				}
			}
			break;
		case	AM_MAKE_STUN_EFFECT:
			{
				if (m_stunEffect == NULL) {
					CMsgEffect * stunEffect = gameMode->MakeMsgEffect(MAKETYPE_STUN_EFFECT, m_pos, m_gid);
					m_msgEffectList.push_back(stunEffect);
					stunEffect->SendMsg(this, AM_SET_MASTER_POINTER);
					m_stunEffect = stunEffect;
				}
			}
			break;
		case	AM_DELETE_STUN_EFFECT:
			{
				if (m_stunEffect) {
					m_stunEffect->SendMsg(NULL, AM_DISAPPEAR);
					m_stunEffect = NULL;
				}
			}
			break;
		case	AM_MAKE_SILENCE_EFFECT:
			{
				if (m_silenceEffect == NULL) {
					CMsgEffect * silenceEffect = gameMode->MakeMsgEffect(MAKETYPE_SILENCE_EFFECT, m_pos, m_gid);
					m_msgEffectList.push_back(silenceEffect);
					silenceEffect->SendMsg(this, AM_SET_MASTER_POINTER);
					m_silenceEffect = silenceEffect;
				}
			}
			break;
		case	AM_DELETE_SILENCE_EFFECT:
			{
				if (m_silenceEffect) {
					m_silenceEffect->SendMsg(NULL, AM_DISAPPEAR);
					m_silenceEffect = NULL;
				}
			}
			break;
		case	AM_MAKE_ANGELUS_EFFECT:
			{
				if (m_angelusEffect == NULL) {
					CMsgEffect * angelusEffect = gameMode->MakeMsgEffect(MAKETYPE_ANGELUS_EFFECT, m_pos, m_gid);
					m_msgEffectList.push_back(angelusEffect);
					angelusEffect->SendMsg(this, AM_SET_MASTER_POINTER);
					m_angelusEffect = angelusEffect;
				}
			}
			break;
		case	AM_DELETE_ANGELUS_EFFECT:
			{
				if (m_angelusEffect) {
					m_angelusEffect->SendMsg(NULL, AM_DISAPPEAR);
					m_angelusEffect = NULL;
				}
			}
			break;
		case	AM_MAKE_CRUCIS_EFFECT:
			{
				if (m_crusisEffect == NULL) {
					CMsgEffect * crusisEffect = gameMode->MakeMsgEffect(MAKETYPE_CRUSIS_EFFECT, m_pos, m_gid);
					m_msgEffectList.push_back(crusisEffect);
					crusisEffect->SendMsg(this, AM_SET_MASTER_POINTER);
					m_crusisEffect = crusisEffect;
				}
			}
			break;
		case	AM_DELETE_CRUCIS_EFFECT:
			{
				if (m_crusisEffect) {
					m_crusisEffect->SendMsg(NULL, AM_DISAPPEAR);
					m_crusisEffect = NULL;
				}
			}
			break;
		case	AM_MAKE_SMOKE_FFECT:
			{
				CMsgEffect * smokeEffect = gameMode->MakeMsgEffect(MAKETYPE_SMOKE_EFFECT, m_pos, m_gid);
				m_msgEffectList.push_back(smokeEffect);
				smokeEffect->SendMsg(this, AM_SET_MASTER_POINTER);
			}
			break;
		case	AM_MAKE_ICEBREAKE_FFECT:
			{
				CMsgEffect * iBreakEffect = gameMode->MakeMsgEffect(MAKETYPE_ICEBREAK_EFFECT, m_pos, m_gid);
				m_msgEffectList.push_back(iBreakEffect);
				iBreakEffect->SendMsg(this, AM_SET_MASTER_POINTER);
			}
			break;
		case	AM_MAKE_CRITICAL_EFFECT:
			{
				CMsgEffect * criticalEffect = gameMode->MakeMsgEffect(MAKETYPE_CRITICAL_EFFECT, m_pos, m_gid);
				m_msgEffectList.push_back(criticalEffect);
				criticalEffect->SendMsg(this, AM_SET_MASTER_POINTER);
			}
			break;
		case	AM_MAKE_BLUE_EFFECT:
			{
				CMsgEffect * blueEffect = gameMode->MakeMsgEffect(MAKETYPE_BLUE_EFFECT, m_pos, m_gid);
				m_msgEffectList.push_back(blueEffect);
				blueEffect->SendMsg(this, AM_SET_MASTER_POINTER);
			}
			break;
		case	AM_USEITEM_EFFECT:
			{
				LaunchEffect(EF_EXIT);
			}
			break;
		case	AM_WILL_BE_DEAD:
			{
		#if defined (_HOMUN)
				if ( m_gid == g_session.m_homunEnemyGID )
				{
					g_session.m_homunEnemyGID = 0 ;
					g_session.m_selectHomunEnemy = false ;
					g_session.m_attackHomunEnemy = false ;
				}

				if ( m_gid == g_session.m_homunGID )
				{
					if (g_windowMgr.GetHomunInfoWnd()) {
						g_windowMgr.DeleteWindow(WID_HOMUNINFOWND);
					}
					if (g_windowMgr.GetHomunSkillListWnd()) {
						g_windowMgr.DeleteWindow(WID_HOMUNSKILLLISTWND);
					}
					g_session.InitHomunTargetInfo ( ) ;
					g_session.m_homunAlive = false ;
				}
#endif

#if defined (_MER)
				if ( m_gid == g_session.m_merEnemyGID )
				{
					g_session.m_merEnemyGID = 0 ;
					g_session.m_selectMerEnemy = false ;
					g_session.m_attackMerEnemy = false ;
				}

				if ( m_gid == g_session.m_merEnemyGID )
				{
					if (g_windowMgr.GetMerInfoWnd()) {
						g_windowMgr.DeleteWindow(WID_MERINFOWND);
					}
					if (g_windowMgr.GetMerSkillListWnd()) {
						g_windowMgr.DeleteWindow(WID_MERSKILLLISTWND);
					}
					g_session.InitMerTargetInfo ( ) ;
					g_session.m_merAlive = false ;
				}
#endif

				if ( (GetJob() > JT_MON_BEGIN && GetJob() < JT_MONSTER_LAST)
#if defined (_MER_R2)
					|| m_objectType == NPC_MERSOL_TYPE
#endif
					|| g_session.IsMerOrHomun(GetJob()) )
					m_gid = 100000 - m_gid;

				if (m_willBeAttackedList.size()) {
					int job = GetJob();
					sAssert(job >= JT_NOVICE);
					if (IsThisPC(job)) {
						SetState(ST_DEAD);
					} else {
						m_willBeDead = true;
					}
				} else
					SetState(ST_DEAD);
			}
			break;

#if defined (_HOMUN)
		case	AM_MAKE_CURSOR_EFFECT:
			{
				if (m_cursorEffect == NULL) {
					CMsgEffect * cursorEffect = gameMode->MakeMsgEffect(MAKETYPE_CURSOR_EFFECT, m_pos, m_gid);
					m_msgEffectList.push_back(cursorEffect);
					cursorEffect->SendMsg(this, AM_SET_MASTER_POINTER);
					m_cursorEffect = cursorEffect;
				}
			}
			break;
		case	AM_DELETE_CURSOR_EFFECT:
			{
				if (m_cursorEffect) {
					m_cursorEffect->SendMsg(NULL, AM_DISAPPEAR);
					m_cursorEffect = NULL;
				}
			}
			break;
#endif
		case	AM_DEAD:
			{
				if (m_stateId == ST_DEAD)
					break;
				vector3d & pPos = gameMode->GetWorld()->m_player->m_pos;
				SetState(ST_DEAD);
			}
			break;
		case	AM_ATTACKED_SKILL:
		case	AM_ATTACKEDNOMOTION_SKILL:
#ifdef	MOB_SKILL
			{
				WBA * wba = (WBA *)arg1;
				m_colorOfSingleColor = 0xffff0000;
				if (wba->effectId2 == EF_TELEKHIT) {
					m_colorOfSingleColor = 0xff042ba4;
				} else if (wba->effectId2 == EF_DARKATTACK) {
					m_colorOfSingleColor = 0xff505050;
				}
			}
#endif
		case	AM_ATTACKED_MANYTIMES:
		case	AM_ATTACKED_MANYTIMES2:
		case	AM_ATTACKED_MANYTIMES2DASH:
		case	AM_ATTACKED:
		case	AM_ATTACKEDNOMOTION:
		case	AM_ATTACKED_CRITICAL:
		case	AM_ATTACKED_LUCKY:
			{
				WBA * wba = (WBA *)arg1;

				int damage = wba->damage;
				DWORD tGid = wba->targetGid;
				int effectId1 = wba->effectId1;
				int effectId2 = wba->effectId2;
				int level = wba->effectLevel;
				float attackedSpeedFactor = wba->attackedSpeedFactor;

				if (wba->tCellX == -1 || wba->tCellY == -1)
				{
					m_damageDestX = m_pos.x;
					m_damageDestZ = m_pos.z;
				}
				else
				{
					gameMode->GetClientCoor(wba->tCellX, wba->tCellY, m_damageDestX, m_damageDestZ);
				}

				int tJob = GetJob();
				vector3d tPos = m_pos;

				unsigned long textColor = 0xFFFFFFFF ;

				CGameActor * tActor = NULL;
				int tActorWeapon = -1;
				if (g_session.GetAid() == tGid)
				{
					tActor = gameMode->GetWorld()->m_player;

				}
				else
				{
					tActor = gameMode->GetWorld()->GetGameActorByAID(tGid);
				}

				if (tActor)
				{
					tPos = tActor->m_pos;
					tJob = tActor->GetJob();
					tActorWeapon = tActor->GetWeapon();
				}

				if (AM_ATTACKED_LUCKY == message)
					damage = 0;

				bool recoverDamage = false;
				if (damage < 0)
				{
					if ( damage != NEVERSEE_DAMAGEVALUE )
					{
						damage = -damage;
						recoverDamage = true;
					}
				}

				if (damage)
				{
					m_singleColorStartTick = timeGetTime();
					if (wba->effectId2 == -2)
					{
						m_isSingleColor = false;
					}
					else
					{
						m_isSingleColor = true;
						vector3d & pPos = gameMode->GetWorld()->m_player->m_pos;
						switch (message)
						{
							case AM_ATTACKED_SKILL:
							case AM_ATTACKEDNOMOTION_SKILL:
							{
								const char * waveName = g_session.GetWeaponHitWaveName(-1);
								PlayWave(waveName, m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
							}
							break;
							default:
							{
								if (IsThisPC(GetJob()))
								{
									const char * waveName = g_session.GetJobHitWaveName(GetJob());
									PlayWave(waveName, m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
								}
								else
								{
									if ( tActor!= NULL )
									{
										if ( tActor->GetJob()==JT_ASSASSIN || tActor->GetJob()==JT_ASSASSIN_H || tActor->GetJob()==JT_ASSASSIN_B )
										{
											int	left, right;
											left = tActorWeapon & 0xFFFF;
											right = (tActorWeapon >> 16) & 0xFFFF;

											if (left > 1000)
											{
												left = g_session.GetWeaponType(left);
											}

											const char * waveName = g_session.GetWeaponHitWaveName(left);
											PlayWave(waveName, m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
										}
										else
										{
											if (tActorWeapon > 1000)
											{
												tActorWeapon = g_session.GetWeaponType(tActorWeapon);
											}
											const char * waveName = g_session.GetWeaponHitWaveName(tActorWeapon);
											PlayWave(waveName, m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
										}
									}
									else
									{
										if (tActorWeapon > 1000)
										{
											tActorWeapon = g_session.GetWeaponType(tActorWeapon);
										}
										const char * waveName = g_session.GetWeaponHitWaveName(tActorWeapon);
										PlayWave(waveName, m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
									}
								}
							}
							break;
						}
					}

					CRagEffect * effect = LaunchEffect(effectId1);

					if (effect)
						effect->SendMsg(NULL, AM_SETCOUNT, 1);

					if (effectId2 == EF_SONICBLOWHIT)
					{
						effectId2 = -1;
						m_roty += 90;
						if (m_roty > 360)
							m_roty -= 360;
					}
					else
					{
						effect = LaunchEffect(effectId2);
						if (effect)
						{
							effect->SendMsg(NULL, AM_SETCOUNT, 1);
							if (effectId2 == EF_BLOODDRAIN || effectId2 == EF_ENERGYDRAIN)
							{
								if (tActor)
									effect->SendMsg(NULL, AM_SETPOS, int(&tActor->m_pos));
							}
						}
					}

					{
						if(recoverDamage)
						{
							SendMsg(NULL, AM_MAKE_NUMBER, damage, MET_NUMBER + 100);
						}
						else if(AM_ATTACKED_CRITICAL == message)
						{
							SendMsg(NULL, AM_MAKE_NUMBER, damage, 0xffffff00, MET_NUMBER_CRITICAL);
							SendMsg(NULL, AM_MAKE_CRITICAL_EFFECT);
						}
						else
						{
							if (message == AM_ATTACKED_MANYTIMES2)
							{
								SendMsg(NULL, AM_MAKE_NUMBER, level, 0xffffffff, MET_NUMBER_TOTAL);
								SendMsg(NULL, AM_MAKE_BLUE_EFFECT);
							}
							else if (message == AM_ATTACKED_MANYTIMES2DASH)
							{
								SendMsg(NULL, AM_MAKE_NUMBER, level, 0xffffffff, MET_NUMBER_TOTAL);
							}
							else if (message == AM_ATTACKED_MANYTIMES)
							{
								SendMsg(NULL, AM_MAKE_NUMBER, damage, 0xffffffff, MET_NUMBER_HALF);
								SendMsg(NULL, AM_MAKE_NUMBER, level, 0xffffff55, MET_NUMBER_TOTAL);
							}
							else
							{
								SendMsg(NULL, AM_MAKE_NUMBER, damage, 0xffffffff, MET_NUMBER);
							}
						}
					}

					if (!recoverDamage)
					{
						if (IsThisPC(GetJob()))
						{

							if (m_stateId != ST_DEAD && m_stateId != ST_ATTACK && message != AM_ATTACKEDNOMOTION)
							{
								SetState(ST_DAMAGE);
								if ( attackedSpeedFactor > 1 )
								{
									m_loopCountOfmotionFinish = attackedSpeedFactor;
								}
								else
								{
									m_motionSpeed *= attackedSpeedFactor;
								}
							}
						}
						else
						{
							if (m_stateId != ST_DEAD && message != AM_ATTACKEDNOMOTION)
							{
								SetState(ST_DAMAGE);
								if ( attackedSpeedFactor > 1 )
								{
									m_loopCountOfmotionFinish = attackedSpeedFactor;
								}
								else
								{
									m_motionSpeed = attackedSpeedFactor;
								}
							}
						}
					}
				}
				else
				{
					if (AM_ATTACKED_LUCKY == message)
					{
						SendMsg(NULL, AM_MAKE_LUCKY);
					}
					else
						{
							if (tActor)
							{
								if (tJob >= JT_WARPNPC && tJob < JT_2004_JOB_BEGIN)
								{
									if (GetGID() == g_session.GetAid() && g_session.IsExistEffectState(EFST_KYRIE))
									{
										SendMsg(NULL, AM_MAKE_GUARD);
									}
									else if (GetGID() == g_session.GetAid() && g_session.IsExistEffectState(EFST_PARRYING))
									{
										SendMsg(NULL, AM_MAKE_GUARD);
									}
									else
										tActor->SendMsg(NULL, AM_MAKE_MISS, 0xffffffff);
								}
								else
								{
									tActor->SendMsg(NULL, AM_MAKE_MISS, 0xffff0000);
								}
							}
						}
				}
				if (m_willBeDead && m_willBeAttackedList.size() <= 1)
					SendMsg(NULL, AM_DEAD);
			}
			break;
		case	AM_WILL_BE_ATTACK:
			{
				SetState(ST_ATTACK);
				m_attackMotion = GetAttackMotion();
				RegisterPos();
				m_stateStartTick = timeGetTime();
				m_curMotion = 4;
				m_motionSpeed = 1;
				m_curAction += Get8Dir(m_roty);
				if (m_willBeDead && m_willBeAttackedList.size() <= 1)
				{
					SendMsg(NULL, AM_DEAD);
				}
			}
			break;
		case	AM_WILL_BE_ATTACKED:
			{
				WBA * w = (WBA *)arg1;
				if (arg2 == 0)
				{
					m_willBeAttackedList.push_back(*w);
				}
				else
				{
					w->effectLevel = arg2;
					w->message = AM_ATTACKED_MANYTIMES;
					m_willBeAttackedList.push_back(*w);
				}
			}
			break;
		case	AM_WILL_BE_ATTACKED2:
			{
				WBA * w = (WBA *)arg1;
				if (arg2 == 0) {
					m_willBeAttackedList.push_back(*w);
				} else {
					w->effectLevel = arg2;
					w->message = AM_ATTACKED_MANYTIMES2;
					m_willBeAttackedList.push_back(*w);
				}
			}
			break;
		case	AM_WILL_BE_ATTACKED2DASH:
			{
				WBA * w = (WBA *)arg1;
				if (arg2 == 0) {
					m_willBeAttackedList.push_back(*w);
				} else {
					w->effectLevel = arg2;
					w->message = AM_ATTACKED_MANYTIMES2DASH;
					m_willBeAttackedList.push_back(*w);
				}
			}
			break;
		case	AM_SKILL:
			Am_Skill(sender, arg1, arg2, arg3);
			break;

		case	AM_GROUNDSKILL:
			Am_Groundskill(sender, arg1, arg2, arg3);
			break;

		case	AM_ACTION:
			Am_Action(sender, arg1, arg2, arg3);
			break;

		case	AM_ATTR_STATE_CHANGE:
			{
				ATTRSTATESTRUCT * s = (ATTRSTATESTRUCT *)arg1;
				SetAttrState(s->bodyState, s->effectState, s->healthState, true);
				SetPKState(s->isPKModeON);
			}
			break;
		case	AM_STANDENTRY:
			{
				STANDENTRYSTRUCT * s = (STANDENTRYSTRUCT *)arg1;
				gameMode->GetClientCoor(s->x, s->y, m_pos.x, m_pos.z);
				m_pos.y = gameMode->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z);
				m_roty = DirToDeg(s->dir);
				m_xSize = s->xSize;
				m_ySize = s->ySize;
				m_moveDestX = s->x;
				m_moveDestY = s->y;
				SetHeadType(s->head);

#if defined (_HOMUN)
				if ( s->headpalette == NPC_PET_TYPE )
					SetHeadType(HEADTYPE_LAST);
#endif
				SetAttrState(s->bodyState, s->effectState, s->healthState, false);
				SetState(ST_STAND);
				SetGuildInfo(s->guildId, s->guildEmblemVersion);
			}
			break;
		case	AM_INITSTANDENTRY:
			{
				STANDENTRYSTRUCT * s = (STANDENTRYSTRUCT *)arg1;

				m_gid = s->GID;
				s->job = GetProperJob(s->job, s->effectState);
				SetSprAct(s->job, s->sex);
				gameMode->GetClientCoor(s->x, s->y, m_pos.x, m_pos.z);
				m_pos.y = gameMode->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z);
				m_roty = DirToDeg(s->dir);
				m_xSize = s->xSize;
				m_ySize = s->ySize;
				m_moveDestX = s->x;
				m_moveDestY = s->y;
				m_speed = s->speed;
				SetHeadType(s->head);

#if defined (_HOMUN)
				if ( s->headpalette == NPC_PET_TYPE )
					SetHeadType(HEADTYPE_LAST);
#endif
				SetAttrState(s->bodyState, s->effectState, s->healthState, false);
				SetState(ST_STAND);
				SetPKState(s->isPKModeON);

				if( s->job==JT_GUILD_FLAG )
				{
					s->guildId = (DWORD)((unsigned short)s->accessory2 << 16) ;
					s->guildId |= (unsigned short)s->accessory3;
					s->guildEmblemVersion = (int)(s->shield);
					SetGuildInfo(s->guildId, s->guildEmblemVersion);
				}
				SetPropensityInfo(s->honor, s->virtue);

				if (arg2 == 1)
				{
					m_sprColor.color = 0x44ffffff;
				}
				else if (arg2 == 2)
				{
					if (!IsEffectStateSpecialHiding(s->effectState))
#ifdef SECONDJOB_SKILL2
						LaunchEffect(EF_ENTRY2);
#else
						LaunchEffect(EF_ENTRY);
#endif

				}
			}
			break;
		case	AM_INITMOVEENTRY:
		case	AM_INITMOVEENTRY2:
			{
				MOVEENTRYSTRUCT * ms = (MOVEENTRYSTRUCT *)arg1;

				m_gid = ms->GID;

				ms->job = GetProperJob(ms->job, ms->effectState);
				SetHeadType(ms->head);

#if defined (_HOMUN)
				if ( ms->headpalette == NPC_PET_TYPE )
					SetHeadType(HEADTYPE_LAST);
#endif
				SetSprAct(ms->job, ms->sex);
				SetAttrState(ms->bodyState, ms->effectState, ms->healthState, false);
				SetPKState(ms->isPKModeON);
				SetGuildInfo(ms->guildId, ms->guildEmblemVersion);
				SetPropensityInfo(ms->honor, ms->virtue);

				m_speed = ms->speed;
				m_moveStartTime = ms->moveStartTime;
				CGameMode * gameMode = g_modeMgr.GetGameMode();

				sAssert(gameMode);
				gameMode->GetClientCoor(ms->sx, ms->sy, m_pos.x, m_pos.z);
				m_pos.x += (ms->cellX-8) * (float(gameMode->GetWorld()->m_attr->m_zoom)/16.0f);
				m_pos.z += (ms->cellY-8) * (float(gameMode->GetWorld()->m_attr->m_zoom)/16.0f);
				CWorld * world = gameMode->GetWorld();
				m_pos.y = world->m_attr->GetHeight(m_pos.x, m_pos.z);

				bool result = FindPath(ms->sx, ms->sy, ms->cellX, ms->cellY, ms->dx, ms->dy, ms->speed);
				if (result) {
					m_moveDestX = ms->dx;
					m_moveDestY = ms->dy;
					SetState(ST_MOVE);
					m_pathStartCell = 0;
				} else
					SetState(ST_STAND);
			}
			break;
		case	AM_MOVEENTRY:
		case	AM_MOVEENTRY2:
			{

				MOVEENTRYSTRUCT * ms = (MOVEENTRYSTRUCT *)arg1;

				m_speed = ms->speed;
				m_moveStartTime = ms->moveStartTime;

				ms->job = GetProperJob(ms->job, ms->effectState);
				SetHeadType(ms->head);
#if defined (_HOMUN)
				if ( ms->headpalette == NPC_PET_TYPE )
					SetHeadType(HEADTYPE_LAST);
#endif
				SetSprAct(ms->job, ms->sex);
				SetAttrState(ms->bodyState, ms->effectState, ms->healthState, false);
				SetPKState(ms->isPKModeON);
				SetGuildInfo(ms->guildId, ms->guildEmblemVersion);
				SetPropensityInfo(ms->honor, ms->virtue);

				bool result = FindPath(ms->sx, ms->sy, ms->cellX, ms->cellY, ms->dx, ms->dy, ms->speed);

				if (result)
				{
					m_moveDestX = ms->dx;
					m_moveDestY = ms->dy;
					SetState(ST_MOVE);
					m_pathStartCell = 0;
				}
			}
			break;
		case	AM_MOVE:
			{
				MOVESTRUCT *ms = (MOVESTRUCT *)arg1;
				m_moveStartTime = timeGetTime();
				bool result = FindPath(ms->sx, ms->sy, ms->cellX, ms->cellY, ms->dx, ms->dy, m_speed);
				if (result) {
					m_moveDestX = ms->dx;
					m_moveDestY = ms->dy;
					SetState(ST_MOVE);
					m_pathStartCell = 0;
				} else
					SetState(ST_STAND);
			}
			break;
		case	AM_DELETE_SHOP_TITLE:
			{

				if (m_merchantShopTitle) {
					g_windowMgr.PostQuit(m_merchantShopTitle);
					m_merchantShopTitle = NULL;
				}
			}
			break;
		case	AM_DELETE_CHAT_TITLE:
			{
				if (m_chatTitle) {
					g_windowMgr.PostQuit(m_chatTitle);
					m_chatTitle = NULL;
				}
			}
			break;
		case	AM_MAKE_SHOP_TITLE:
			{
				if (m_merchantShopTitle == NULL) {
					m_merchantShopTitle = new UIMerchantShopTitle;
					m_merchantShopTitle->Create(200, 34);
					g_windowMgr.AddWindowFront(m_merchantShopTitle);
				}
				m_merchantShopTitle->SendMsg(NULL, UM_SETINFO, arg1, m_charfont, 0, 0);
				m_merchantShopTitle->SendMsg(NULL, UM_SETNAID, m_gid, 0, 0, 0);
				m_merchantShopTitle->Move(m_lastTlvertX - 100, m_lastTlvertY - 110 * g_renderer->m_height / 480);
			}
			break;
		case	AM_MAKE_CHAT_TITLE:
			{
				if (m_chatTitle == NULL) {
					m_chatTitle = new UIChatRoomTitle;
					m_chatTitle->Create(200, 34);
					g_windowMgr.AddWindowFront(m_chatTitle);
				}
				m_chatTitle->SendMsg(NULL, UM_SETINFO, arg1, m_charfont, 0, 0);
				m_chatTitle->Move(m_lastTlvertX - 100, m_lastTlvertY - 110 * g_renderer->m_height / 480);
			}
			break;
		case	AM_MAKE_SKILL_RECHARGEGAGE:
			{
				if (m_skillRechargeGage == NULL) {
					static float hHover = float(81 * g_renderer->m_height / 480);
					m_skillRechargeGage = new UIRechargeGage;
					m_skillRechargeGage->Create(60, 6);
					m_skillRechargeGage->Move(m_lastTlvertX - 30, m_lastTlvertY - hHover * m_lastPixelRatio);
					g_windowMgr.AddWindowFront(m_skillRechargeGage);
				}

				DWORD curTime = timeGetTime();
				m_skillRechargeEndTick = arg1 + curTime;
				m_skillRechargeStartTick = curTime ;

				if ( g_session.GetAid() == m_gid )
				{


				}
				else
				{
				}

			}
			break;
		case	AM_DELETE_SKILL_RECHARGEGAGE:
			{
				if (m_skillRechargeGage == NULL)
					break;

				g_windowMgr.PostQuit(m_skillRechargeGage);
				m_skillRechargeGage = NULL;
				if (m_gid == g_session.GetAid())
				{
					if (g_session.m_GaugePacket == 1)
					{
						PACKET_CZ_PROGRESS p;
						p.PacketType	= HEADER_CZ_PROGRESS;
						g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (char *)(&p));
					}
					g_session.m_GaugePacket = 0;
				}
			}
			break;
		case	AM_CHAT:
			{
				char * chatMsg = (char *)arg1;
				if (strlen(chatMsg) == 0)
					break;
				m_chatTick = timeGetTime();
				DeleteChatBalloon();
				if (m_balloon == NULL) {
					m_balloon = new UITransBalloonText;
					m_balloon->Create(8, 8);
					m_balloon->SetShow(0);
					g_windowMgr.AddWindowFront(m_balloon);
				}
				if (m_balloon == NULL)
					break;
				if (m_gid == g_session.GetAid())
					m_balloon->SetCharFont(g_session.m_charfont);
				else
					m_balloon->SetCharFont(m_charfont);
				m_balloon->SetText(chatMsg, 35);
				if (arg2)
					m_balloon->SetFntColor(arg2, 0);
				m_chatWidth = m_balloon->GetWidth();
				m_chatHeight = m_balloon->GetHeight();
				m_balloon->Move(m_lastTlvertX - m_chatWidth / 2, m_lastTlvertY - 110 * g_renderer->m_height / 480);
			}
			break;
		case	AM_STOPMOVE:
			{
				int dx = arg1, dy = arg2, speed = arg3;
				int sx, sy, cellX, cellY;
				m_moveStartTime = g_session.GetServerTime();
				gameMode->GetServerCoor(m_pos.x, m_pos.z, sx, sy, cellX, cellY);

				bool result;
				if (!speed) {
					if (m_isAsuraAttack)
					{
						result = FindPath(sx, sy, cellX, cellY, dx, dy, 55);
					}
					else
					{
						result = FindPath(sx, sy, cellX, cellY, dx, dy, 2);
					}
				} else {
					result = FindPath(sx, sy, cellX, cellY, dx, dy, speed);
				}

				if (result)
				{
					m_moveDestX = dx;
					m_moveDestY = dy;
					SetState(ST_MOVE);
					m_pathStartCell = 0;

					if (!speed)
						m_isNeverAnimation = true;
					else
					{
						m_isNeverAnimation = false;
						m_isMotionFreezed = true;
					}
				}
				else
				{
					SetState(ST_STAND);
				}
			}
			break;
		case	AM_CORRECT_POS:
			{
				int dx = arg1, dy = arg2;
				m_moveDestX = dx;
				m_moveDestY = dy;
				gameMode->GetClientCoor(dx, dy, m_pos.x, m_pos.z);
			}
			break;
		case	AM_RESURRECTION:
			{
				Trace("AM_RESURRECTION");
				m_stateId = ST_STAND;
				m_isLieOnGround = false;
				m_shouldAddPickInfo = true;
				g_modeMgr.GetGameMode()->SetWaitUseItemAck(FALSE);
				SetState(ST_STAND);
			}
			break;
		case	AM_OUTOFSIGHT:
			{
				SetVanish();
			}
			break;
		case	AM_99:
			{
				if (m_99!=99)
				{
					if (!IsEffectStateSpecialHiding(m_effectState) && !IsEffectStateBurrow(m_effectState) && !IsEffectStateHiding(m_effectState))
					{
						m_99=99;

						if (g_session.m_isAura == TRUE)
						{
							if (GetJob() > JT_2004_JOB_BEGIN || GetJob() == JT_SUPERNOVICE)
							{
								for (int i=0;i<2;i++)
									LaunchEffect(EF_LEVEL99_4);
								LaunchEffect(EF_LEVEL99_5);
								LaunchEffect(EF_LEVEL99_6);
							}
							else if ((GetJob() < JT_TREASURE_BOX1 || GetJob() > JT_TREASURE_BOX40) && GetJob() != JT_G_TREASURE_BOX)
							{
								if (GetJob() >= JT_TREASURE_BOX41 && GetJob() <= JT_TREASURE_BOX_I)
								{
								}
								else if (GetJob() >= JT_SHECIL && GetJob() <= JT_G_RAWREL)
								{
									for (int i=0;i<2;i++)
										LaunchEffect(EF_LEVEL99_4);
								}
								else
								{
									for (int i=0;i<2;i++)
										LaunchEffect(EF_LEVEL99_4);
									LaunchEffect(EF_LEVEL99);
									LaunchEffect(EF_LEVEL99_2);
								}
							}
						}
						else
						{
							if (GetJob() > JT_2004_JOB_BEGIN || GetJob() == JT_SUPERNOVICE)
							{
								for (int i=0;i<2;i++)
									LaunchEffect(EF_LEVEL99_4);
							}
							else if ((GetJob() < JT_TREASURE_BOX1 || GetJob() > JT_TREASURE_BOX40) && GetJob() != JT_G_TREASURE_BOX)
							{
								if (GetJob() >= JT_TREASURE_BOX41 && GetJob() <= JT_TREASURE_BOX_I)
								{
								}
								else if (GetJob() >= JT_SHECIL)
								{
									for (int i=0;i<2;i++)
										LaunchEffect(EF_LEVEL99_4);
								}
								else
								{
									for (int i=0;i<2;i++)
										LaunchEffect(EF_LEVEL99_3);
								}
							}
						}
					}
				}
				m_is99 = TRUE;
			}
			break;
		case	AM_STATUS_CHANGE:
			{
#ifndef BALANCE_PATCH
				switch(arg1)
				{
					case	STMSG_INCATTACKSPEED:
						break;
					case	STMSG_DECATTACKSPEED:
						SendMsg(this, AM_DELETE_EFFECT, EF_TWOHANDQUICKEN);
						SendMsg(this, AM_DELETE_EFFECT, EF_SPEARQUICKEN);
						break;
					case	STMSG_INCWEAPONATTACK:
						break;
					case	STMSG_DECWEAPONATTACK:
						break;
					case	STMSG_FASTPREDELAY:
						break;
					case	STMSG_NORMALPREDELAY:
						break;
					case	STMSG_WEAPONPROPERTYPOISON:
						break;
					case	STMSG_WEAPONPROPERTYSAINT:
						break;
					case	STMSG_WEAPONPROPERTYORIGINAL:
						break;
					case	STMSG_ARMORPROPERTYSAINT:
						break;
					case	STMSG_ARMORPROPERTYORIGINAL:
						break;
					case	STMSG_BARRIOR:
						break;
					case	STMSG_DISAPPEARBARRIOR:
						break;
					case	STMSG_PERFECTDAMAGE:
						break;
					case	STMSG_DISAPPEARPERFECTDAMAGE:
						break;
					case	STMSG_OVERTHRUSTING:
						break;
					case	STMSG_DISAPPEAROVERTHRUSTING:
						SendMsg(this, AM_DELETE_EFFECT, EF_OVERTHRUST);
						break;
					case	STMSG_MAXIMIZE:
						break;
					case	STMSG_DISAPPEARMAXIMIZE:
						break;
				}
#else

				switch(arg1)
				{
					case	EFST_TWOHANDQUICKEN:
					case	EFST_ONEHANDQUICKEN:
					{
						while (IsEffectExist(EF_MAKEBLUR))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_MAKEBLUR);

						if (arg2) {
							CRagEffect *effect = LaunchEffect(EF_MAKEBLUR);
							if (effect) {
								effect->SendMsg(NULL, AM_SETCOUNT, 65535, EF_TWOHANDQUICKEN);
							}
						}
						break;
					}
					case	EFST_SPEARQUICKEN: {
						while (IsEffectExist(EF_MAKEBLUR))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_MAKEBLUR);

						if (arg2) {
							CRagEffect *effect = LaunchEffect(EF_MAKEBLUR);
							if (effect) {
								effect->SendMsg(NULL, AM_SETCOUNT, 65535, EF_SPEARQUICKEN);
							}
						}
						break;
					}
					case	EFST_NJ_BUNSINJYUTSU:
					{
						while (IsEffectExist(EF_BUNSINJYUTSU))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_BUNSINJYUTSU);

						if (arg2) {
							CRagEffect *effect = LaunchEffect(EF_BUNSINJYUTSU);
						}

						break;
					}
					case	EFST_DEVIL1:
					{
						if (m_gid == g_session.GetAid())
						{
							SKILL_INFO skillInfo = g_session.GetSkillItemInfoBySkillId(SKID_SG_DEVIL);
							if ( skillInfo.level >= 1 && skillInfo.level <= 10 )
							{
								if (!IsEffectExist(EF_DEVIL1+(skillInfo.level-1)))
								{
									LaunchEffect(EF_DEVIL1+(skillInfo.level-1));
									LaunchEffect(EF_BLACKDEVIL);
									PlayWave("effect\\_blind.wav", 0, -100, 0);
								}
							}
						}
						break;
					}
					case	EFST_SOULLINK:
					{
						if (m_gid == g_session.GetAid())
						{
							if (arg2)
								g_Weather.LaunchNight();
							else
								g_Weather.StopNight();
						}

						if (!arg2)
						{
							while (IsEffectExist(EF_ASURABODY))
								SendMsg(NULL, AM_DELETE_EFFECT, EF_ASURABODY);
						}
						break;
					}
					case	EFST_CRYSTAL:
					{
						if (m_gid == g_session.GetAid())
						{
							if (arg2)
							{
							}
						}
						break;
					}
					case	EFST_DA_CONTRACT:
					{
						while (IsEffectExist(EF_M04))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_M04);
						if (arg2) {
							LaunchEffect(EF_M04);
						}
						break;
					}
					case	EFST_PROPERTYUNDEAD:
					{
						if (!arg2)
						{
							while (IsEffectExist(EF_UNDEADBODY))
							{
								SendMsg(NULL, AM_DELETE_EFFECT, EF_UNDEADBODY);
									LaunchEffect(EF_UNDEADBODY_DEL);
							}
						}
						else if (!IsEffectExist(EF_UNDEADBODY))
							LaunchEffect(EF_UNDEADBODY);

						break;
					}
					case	EFST_SKE:
					{
						if (m_gid == g_session.GetAid())
						{
							if (arg2)
								g_Weather.LaunchNight();
							else
							{
								g_Weather.StopNight();
							}
						}
						break;
					}
					case	EFST_SMA_READY:
					{
						if (arg2) {
							LaunchEffect(EF_SMA_READY);
						}
						break;
					}
					case	EFST_EXPLOSIONSPIRITS: {
						while (IsEffectExist(EF_GUMGANG))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_GUMGANG);

						if (arg2) {
							LaunchEffect(EF_GUMGANG);
						}

						while (IsEffectExist(EF_MAKEBLUR))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_MAKEBLUR);

						if (arg2) {
							CRagEffect *effect = LaunchEffect(EF_MAKEBLUR);
							if (effect) {
								effect->SendMsg(NULL, AM_SETCOUNT, 65534, EF_OVERTHRUST);
							}
						}
						break;
					}
					case	EFST_DBARRIER:
					case	EFST_STEELBODY:
					{
						while (IsEffectExist(EF_STEELBODY))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_STEELBODY);
						if (arg2) {
							LaunchEffect(EF_STEELBODY);
						}
						break;
					}
					case	EFST_SG_SUN_WARM:
					case	EFST_SG_MOON_WARM:
					case	EFST_SG_STAR_WARM:
					{
						while (IsEffectExist(EF_DOUBLEGUMGANG))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_DOUBLEGUMGANG);
						while (IsEffectExist(EF_REDLIGHTBODY))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_REDLIGHTBODY);
						if (arg2) {
							LaunchEffect(EF_DOUBLEGUMGANG);
							LaunchEffect(EF_REDLIGHTBODY);
						}
						break;
					}
					case	EFST_DAURA:
					{
						while (IsEffectExist(EF_DOUBLEGUMGANG))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_DOUBLEGUMGANG);
						while (IsEffectExist(EF_REDLIGHTBODY))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_REDLIGHTBODY);
						if (arg2) {
							LaunchEffect(EF_DOUBLEGUMGANG);
							LaunchEffect(EF_REDLIGHTBODY);
						}
						break;
					}
					case	EFST_KAITE:
					{
						while (IsEffectExist(EF_REFLECTBODY))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_REFLECTBODY);
						if (arg2) {
							LaunchEffect(EF_REFLECTBODY);
						}
						break;
					}
					case	EFST_AURABLADE:	{
						while (IsEffectExist(EF_AURABLADE))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_AURABLADE);
						while (IsEffectExist(EF_AURABLADE2))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_AURABLADE2);
						if (arg2) {
							LaunchEffect(EF_AURABLADE);
							LaunchEffect(EF_AURABLADE2);
						}
						break;
					}
					case	EFST_STOP:	{
						while (IsEffectExist(EF_NPC_STOP))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_NPC_STOP);
						break;
					}
					case	EFST_MOON:	{
						while (IsEffectExist(EF_SPHEREWIND2))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_SPHEREWIND2);

						if (arg2) {
							LaunchEffect(EF_SPHEREWIND2);
						}
						break;
					}
					case	EFST_MARIONETTE_MASTER :	{
						SendMsg(NULL, AM_DELETE_EFFECT, EF_LINELINK3);
						while (IsEffectExist(EF_PINKBODY))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_PINKBODY);

						if (arg2) {
							LaunchEffect(EF_PINKBODY);
						}
						break;
					}
					case	EFST_MARIONETTE :	{
						while (IsEffectExist(EF_PINKBODY))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_PINKBODY);
						if (arg2) {
							LaunchEffect(EF_PINKBODY);
						}
						break;
					}
					case	EFST_LKCONCENTRATION:	{
						SendMsg(NULL, AM_DELETE_EFFECT, EF_LKCONCENTRATION);
						if (arg2) {
							LaunchEffect(EF_LKCONCENTRATION);
						}
						break;
					}

					case	EFST_BERSERK:
					{
						while (IsEffectExist(EF_REDBODY))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_REDBODY);

						if (arg2) {
							LaunchEffect(EF_REDBODY);
						}
						break;
					}
					case	EFST_ASSUMPTIO:
					{
						while (IsEffectExist(EF_ASSUMPTIO))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_ASSUMPTIO);
						break;
					}
					case	EFST_RG_CCONFINE_M:
					case	EFST_RG_CCONFINE_S:
					{
						while (IsEffectExist(EF_NPC_STOP2))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_NPC_STOP2);

						if (arg2) {
							LaunchEffect(EF_NPC_STOP2);
						}
						break;
					}
					case	EFST_WZ_SIGHTBLASTER:
					{
						while (IsEffectExist(EF_SIGHT2))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_SIGHT2);

						if (arg2) {
							LaunchEffect(EF_SIGHT2);
						}
						break;
					}
					case	EFST_RUN:
					{
						if (IsEffectExist(EF_RUN))
							LaunchEffect(EF_STOPRUN);
						SendMsg(NULL, AM_DELETE_EFFECT, EF_RUN);
						break;
					}
					case	EFST_TING:
					{
						{
							vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

							PlayWave("effect\\T_\xba\xae\xc6\xa8\xb1\xe8.wav", m_pos.x-pPos.x, 0, m_pos.z-pPos.z);
						}
						LaunchEffect(EF_QUAKEBODY);
						if (IsEffectExist(EF_RUN))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_RUN);
						break;
					}
					case	EFST_MAGICPOWER:
					{
						while (IsEffectExist(EF_LIGHTBLADE))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_LIGHTBLADE);

						if (arg2) {
							LaunchEffect(EF_LIGHTBLADE);
						}
						break;
					}
					case	EFST_OVERTHRUST:
					case	EFST_OVERTHRUSTMAX:
					{
						while (IsEffectExist(EF_MAKEBLUR))
							SendMsg(NULL, AM_DELETE_EFFECT, EF_MAKEBLUR);

						if (arg2) {
							CRagEffect *effect = LaunchEffect(EF_MAKEBLUR);
							if (effect) {
								effect->SendMsg(NULL, AM_SETCOUNT, 65535, EF_OVERTHRUST);
							}
						}
						break;
					}
					case	EFST_ENERGYCOAT:
						{
							while (IsEffectExist(EF_MAKEBLUR))
								SendMsg(NULL, AM_DELETE_EFFECT, EF_MAKEBLUR);

							if (arg2) {
								CRagEffect *effect = LaunchEffect(EF_MAKEBLUR);
								if (effect) {
									effect->SendMsg(NULL, AM_SETCOUNT, 65535, EF_ENERGYCOAT);
								}
							}
						}
					break;
					case	EFST_TRICKDEAD:
					{
						if (arg2)
						{
							int rideActOffset = 0;
							m_isTrickDead = TRUE;
#ifdef ACTION_V2
							int actIndex = g_session.GetSpriteAction( ST_DEAD, GetWeapon(), m_bride, GetShield() ) ;
							SetAction(actIndex, 4, MT_ONETIME);
#else
							SetAction(rideActOffset+64, 4, MT_ONETIME);
#endif
							m_isMotionFreezed = true;
						} else {
							m_isTrickDead = FALSE;
							m_stateId = ST_STAND;
							m_isLieOnGround = false;
							m_shouldAddPickInfo = true;
							SetState(ST_STAND);
							m_isMotionFreezed = false;
						}
						break;
					}
					case	EFST_ILLUSION:
	#if defined (_UPDATE2005A)
						if (m_gid == g_session.GetAid())
						{
							if (arg2)
							{
								g_se_go = 10000;
								g_se_type = 2;
							}
							else
								g_se_go = 0;
						}
	#endif
						break;
					case	EFST_DA_BLACK:
						if (m_gid == g_session.GetAid())
						{
							g_se_go = 1;
							g_se_type = 1;
						}
						break;

					case	EFST_MER_FLEE:
						{
							SendMsg(NULL, AM_CHAT, int("FLEE UP!"));
							LaunchEffect(EF_AGIUP);
						}
						break;

					case	EFST_MER_ATK:
						{
							SendMsg(NULL, AM_CHAT, int("ATK UP!"));
							LaunchEffect(EF_AGIUP);
						}
						break;

					case	EFST_MER_HIT:
						{
							SendMsg(NULL, AM_CHAT, int("DEX UP!"));
							LaunchEffect(EF_AGIUP);
						}
						break;

					case	EFST_MER_HP:
						{
							SendMsg(NULL, AM_CHAT, int("HP UP!"));
							LaunchEffect(EF_AGIUP);
						}
						break;

					case	EFST_MER_SP:
						{
							SendMsg(NULL, AM_CHAT, int("SP UP!"));
							LaunchEffect(EF_AGIUP);
						}
						break;




				}
#endif
			}
			break;
		case	AM_CHANGE_COLOR:
			m_isSprArgbFixed = TRUE;
			if (m_oldColor.a == 0 && m_oldColor.r == 0 && m_oldColor.g == 0 && m_oldColor.b == 0) {
				m_oldColor.a = m_sprArgb >> 24 & 0xff;
				m_oldColor.r = m_sprArgb >> 16 & 0xff;
				m_oldColor.g = m_sprArgb >> 8 & 0xff;
				m_oldColor.b = m_sprArgb & 0xff;
			}
			m_curColor.a = 255;
			m_curColor.r = arg1;
			m_curColor.g = arg2;
			m_curColor.b = arg3;

			m_sprArgb = 255 << 24;
			m_sprArgb |= arg1 << 16;
			m_sprArgb |= arg2 << 8;
			m_sprArgb |= arg3;
			break;
		case	AM_RETURN_COLOR:
			m_oldColor.a = 0;
			m_oldColor.r = 0;
			m_oldColor.g = 0;
			m_oldColor.b = 0;
			m_isSprArgbFixed = FALSE;
			break;
		default:
			CAbleToMakeEffect::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}


bool CGameActor :: FindPath(int mssx, int mssy, int mscellX, int mscellY, int msdx, int msdy, int msspeed)
{
	bool result = g_pathFinder.FindPath(m_moveStartTime, mssx, mssy, mscellX, mscellY, msdx, msdy, msspeed, &m_path);
	if (result) {
		DWORD lastCellTime;
		m_path.GetLastCellTime(lastCellTime);
		DWORD serverTime = g_session.GetServerTime();
		if (lastCellTime <= serverTime) {
			msspeed = msspeed * 10 / 13;
			m_moveStartTime = serverTime;
			result = g_pathFinder.FindPath(m_moveStartTime, mssx, mssy, mscellX, mscellY, msdx, msdy, msspeed, &m_path);
		}
	}
	return result;
}

void	CGameActor :: SetPKState(int isPKModeON)
{
	int oldPkState = m_pkState;
	m_pkState = isPKModeON;
}

void	CGameActor :: Delete99LevelEffects()
{
	m_99 = 0;
	SendMsg(NULL, 0x6d, 0xc8, 0, 0);
	SendMsg(NULL, AM_DELETE_EFFECT, 0x18d, 0, 0);
	SendMsg(NULL, AM_DELETE_EFFECT, 0x18e, 0, 0);
	SendMsg(NULL, AM_DELETE_EFFECT, 0x16a, 0, 0);
	SendMsg(NULL, AM_DELETE_EFFECT, 0x16a, 0, 0);
	SendMsg(NULL, AM_DELETE_EFFECT, 0xc9, 0, 0);
	SendMsg(NULL, AM_DELETE_EFFECT, 0xca, 0, 0);
	SendMsg(NULL, AM_DELETE_EFFECT, 0xca, 0, 0);
}


BOOL g_isPoison = FALSE;

void	CGameActor :: SetAttrState(int bodyState, int effectState, int healthState, bool changedNow)
{










	int oldBodyState = m_bodyState;
	int oldEffectState = m_effectState;
	m_bodyState = bodyState;
	m_effectState = effectState;
	m_healthState = healthState;

	CWorld* world = g_modeMgr.GetGameMode()->GetWorld();

	if(IsEffectStateSightChanged(m_effectState, oldEffectState))
	{
		if(IsEffectStateSight(m_effectState))
		{
			if(world && world->m_player)
			{
				vector3d & pPos = world->m_player->m_pos;
				PlayWave("effect\\EF_Sight.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
		}
	}

	if(IsEffectStateRuwachChanged(m_effectState, oldEffectState))
	{
		if(IsEffectStateRuwach(m_effectState))
		{
			if(world && world->m_player)
			{
				vector3d & pPos = world->m_player->m_pos;
				PlayWave("effect\\EF_Sight.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
		}
	}


	if(IsEffectStateBurrowChanged(m_effectState, oldEffectState))
	{
		Delete99LevelEffects();
		if(IsEffectStateBurrow(m_effectState))
		{
			if (changedNow)
			{
				SendMsg(NULL, AM_MAKE_SMOKE_FFECT);
				if ( GetJob() != JT_NINJA )
					LaunchEffect(EF_BASH);
			}
			m_isVisibleBody = false;
			if (m_gid != g_session.GetAid() && !g_session.IsMasterAid(7))
			{
				m_shadowOn = false;
				SendMsg(NULL, AM_DELETE_EFFECT, EF_TOPRANK);
				SendMsg(NULL, AM_REMOVEGAGE);
			}
			if(IsEffectStatePushCart(m_effectState)) SendMsg(NULL, AM_DELETE_CART);
			if(IsEffectStateBird(m_effectState)) SendMsg(NULL, AM_DELETE_BIRD);
			DeleteEmblemWnd();
			DeleteWordDisplayWnd();
		}
		else
		{
			if (changedNow)
			{
				SendMsg(NULL, AM_MAKE_SMOKE_FFECT);
				g_modeMgr.GetGameMode()->SetWaitUseItemAck(FALSE);
			}

			m_isVisibleBody = true;
			if (m_gid != g_session.GetAid())
			{
				m_shadowOn = true;
			}
			if (IsEffectStatePushCart(m_effectState)) SendMsg(NULL, AM_MAKE_CART);
			if (IsEffectStateBird(m_effectState)) SendMsg(NULL, AM_MAKE_BIRD);
			MakeEmblemWnd();
			if (m_is99)
			{
				SendMsg(NULL, AM_99);
			}
		}
	}

	if(IsEffectStateHidingChanged(m_effectState, oldEffectState))
	{
		Delete99LevelEffects();
		if(IsEffectStateHiding(m_effectState))
		{
			m_isVisibleBody = false;
			m_isVisible = false;

			if (m_gid != g_session.GetAid() && !g_session.IsMasterAid(7))
			{
				m_shadowOn = false;
				SendMsg(NULL, AM_REMOVEGAGE);
				SendMsg(NULL, AM_DELETE_EFFECT, EF_TOPRANK);
			}
			if(IsEffectStatePushCart(m_effectState)) SendMsg(NULL, AM_DELETE_CART);
			if(IsEffectStateBird(m_effectState)) SendMsg(NULL, AM_DELETE_BIRD);
			DeleteEmblemWnd();
			DeleteWordDisplayWnd();

		}
		else
		{
			m_isVisibleBody = true;
			m_isVisible = true;

			if (m_gid != g_session.GetAid()) {
				m_shadowOn = true;
			}
			if (IsEffectStatePushCart(m_effectState)) SendMsg(NULL, AM_MAKE_CART);

			if(IsEffectStateBird(m_effectState)) SendMsg(NULL, AM_MAKE_BIRD);
			MakeEmblemWnd();
			if (m_is99) {
				SendMsg(NULL, AM_99);
			}
		}
	}

	if(IsEffectStatePushCartChanged(m_effectState, oldEffectState))
	{
		if(IsEffectStatePushCart(m_effectState)) {
			SendMsg(NULL, AM_DELETE_CART);
			SendMsg(NULL, AM_MAKE_CART, m_effectState);
		} else {
			SendMsg(NULL, AM_DELETE_CART);
		}
	}

	if(IsEffectStateBirdChanged(m_effectState, oldEffectState)) {
		if(IsEffectStateBird(m_effectState))
			SendMsg(NULL, AM_MAKE_BIRD);
		else
			SendMsg(NULL, AM_DELETE_BIRD);
	}

	if(IsEffectStateChickenChanged(m_effectState, oldEffectState)) {
		if(IsEffectStateChicken(m_effectState))
			SendMsg(NULL, AM_MAKE_CHICKEN);
		else
			SendMsg(NULL, AM_DELETE_CHICKEN);
	}

	if(IsEffectStateStar2Changed(m_effectState, oldEffectState)) {
		if(IsEffectStateStar2(m_effectState))
			SendMsg(NULL, AM_MAKE_CHICKEN);
		else
			SendMsg(NULL, AM_DELETE_CHICKEN);
	}

	if(IsEffectStateSpecialHidingChanged(m_effectState, oldEffectState)) {
		Delete99LevelEffects();
		if(IsEffectStateSpecialHiding(m_effectState))
		{
			if (g_session.IsMasterAid(8)) {
				m_isVisibleBody = true;
				m_isVisible = true;
				m_shadowOn = false;
			} else {
				m_isVisibleBody = false;
				m_isVisible = false;
				m_shadowOn = false;
				SendMsg(NULL, AM_DELETE_EFFECT, EF_TOPRANK);
			}
			if(IsEffectStatePushCart(m_effectState)) SendMsg(NULL, AM_DELETE_CART);
			if(IsEffectStateBird(m_effectState)) SendMsg(NULL, AM_DELETE_BIRD);
			DeleteEmblemWnd();
			DeleteWordDisplayWnd();
		} else {
			m_isVisibleBody = true;
			m_isVisible = true;
			m_shadowOn = true;
			if(IsEffectStatePushCart(m_effectState)) SendMsg(NULL, AM_MAKE_CART);
			if(IsEffectStateBird(m_effectState)) SendMsg(NULL, AM_MAKE_BIRD);
			MakeEmblemWnd();
			if (m_is99) {
				SendMsg(NULL, AM_99);
			}
		}
	}


	if (oldBodyState != m_bodyState) {
		switch (oldBodyState) {
			case	BODYSTATE_FREEZING:
				SendMsg(NULL, AM_DELETE_ICEEFFECT);
				SendMsg(NULL, AM_MAKE_ICEBREAKE_FFECT);
				{
					CGameMode * gameMode = g_modeMgr.GetGameMode();
					vector3d & pPos = gameMode->GetWorld()->m_player->m_pos;
					PlayWave("_frozen_explosion.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					m_isMotionFreezed = false;
					m_isSprArgbFixed = false;
					m_isMotionFinished = true;
					SetArgb(-1,255,255,255);
					g_modeMgr.GetGameMode()->SetWaitUseItemAck(FALSE);
				}
				break;
			case	BODYSTATE_STONECURSE:
				{
					CGameMode * gameMode = g_modeMgr.GetGameMode();
					vector3d & pPos = gameMode->GetWorld()->m_player->m_pos;
					PlayWave("_stone_explosion.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
					g_modeMgr.GetGameMode()->SetWaitUseItemAck(FALSE);
				}
				break;
			case	BODYSTATE_STUN:
				SendMsg(NULL, AM_DELETE_STUN_EFFECT);
				g_modeMgr.GetGameMode()->SetWaitUseItemAck(FALSE);
				break;
			case	BODYSTATE_SLEEP:
				SendMsg(NULL, AM_DELETE_SLEEP_EFFECT);
				g_modeMgr.GetGameMode()->SetWaitUseItemAck(FALSE);
				break;
		}

		switch (m_bodyState) {

			case	BODYSTATE_FREEZING:
				SendMsg(NULL, AM_MAKE_ICEEFFECT);
				m_isMotionFreezed = true;
				m_isSprArgbFixed = true;
				m_isMotionFinished = true;
				SetArgb(-1,0,128,255);
				if (changedNow) {
					CGameMode * gameMode = g_modeMgr.GetGameMode();
					vector3d & pPos = gameMode->GetWorld()->m_player->m_pos;
					PlayWave("_stonecurse.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				break;
			case	BODYSTATE_STONECURSE:
				m_isMotionFreezed = true;
				m_isSprArgbFixed = true;
				m_isMotionFinished = true;
				SetArgb(-1,64,64,64);
				break;
			case	BODYSTATE_STONECURSE_ING:
				m_isSprArgbFixed = true;
				SetArgb(-1,128,128,128);
				break;
			case	BODYSTATE_STUN:
				SendMsg(NULL, AM_MAKE_STUN_EFFECT);
				if (changedNow) {
					CGameMode * gameMode = g_modeMgr.GetGameMode();
					vector3d & pPos = gameMode->GetWorld()->m_player->m_pos;
					PlayWave("_stun.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
				}
				break;
			case	BODYSTATE_SLEEP:
				SendMsg(NULL, AM_MAKE_SLEEP_EFFECT);
				break;
			default:
				break;
		}
	}


	{
		if (m_healthState & HEALTHSTATE_HEAVYPOISON)
		{
			if (m_gid == g_session.GetAid())
			{
				g_isPoison = TRUE;
			}
			m_isSprArgbFixed = true;
			SetArgb(-1,255,50,255);
			if (changedNow) {
				CGameMode * gameMode = g_modeMgr.GetGameMode();
				vector3d & pPos = gameMode->GetWorld()->m_player->m_pos;
				PlayWave("_poison.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

				if (m_gid == g_session.GetAid())
				{
					if (!gameMode->GetWorld()->m_player->IsEffectExist(EF_POISON))
						CRagEffect *effect = LaunchEffect(EF_POISON);
				}
			}
		}
		else if (m_healthState & HEALTHSTATE_POISON)
		{
			if (m_gid == g_session.GetAid())
			{
				g_isPoison = TRUE;
			}
			m_isSprArgbFixed = true;
			SetArgb(-1,255,128,255);
			if (changedNow) {
				CGameMode * gameMode = g_modeMgr.GetGameMode();
				vector3d & pPos = gameMode->GetWorld()->m_player->m_pos;
				PlayWave("_poison.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

#ifdef SECONDJOB_SKILL2
				if (m_gid == g_session.GetAid())
				{
					if (!gameMode->GetWorld()->m_player->IsEffectExist(EF_POISON))
						CRagEffect *effect = LaunchEffect(EF_POISON);
				}
#endif
			}
		} else {
			if (m_gid == g_session.GetAid())
			{
#ifdef SECONDJOB_SKILL2
				if (g_isPoison == TRUE)
					SendMsg(NULL, AM_DELETE_EFFECT, EF_POISON);
#endif
				g_isPoison = FALSE;
			}
		}

		if (m_healthState & HEALTHSTATE_CURSE) {
			SendMsg(NULL, AM_MAKE_CURSE_EFFECT);
			m_isSprArgbFixed = true;
			SetArgb(-1, 200, 50, 50);
			if (changedNow) {
				CGameMode * gameMode = g_modeMgr.GetGameMode();
				vector3d & pPos = gameMode->GetWorld()->m_player->m_pos;
				PlayWave("_curse.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
		} else
		{
			SendMsg(NULL, AM_DELETE_CURSE_EFFECT);
		}

		if (m_healthState & HEALTHSTATE_SILENCE) {
			if (changedNow) {
				CGameMode * gameMode = g_modeMgr.GetGameMode();
				vector3d & pPos = gameMode->GetWorld()->m_player->m_pos;
				PlayWave("_silence.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
			SendMsg(NULL, AM_MAKE_SILENCE_EFFECT);
		} else {
			SendMsg(NULL, AM_DELETE_SILENCE_EFFECT);
		}

		if (m_healthState & HEALTHSTATE_ANGELUS) {
			SendMsg(NULL, AM_MAKE_ANGELUS_EFFECT);
		} else {
			SendMsg(NULL, AM_DELETE_ANGELUS_EFFECT);
		}

		if (m_healthState & HEALTHSTATE_CONFUSION)
		{
			if (changedNow)
			{
				CGameMode * gameMode = g_modeMgr.GetGameMode();
				vector3d & pPos = gameMode->GetWorld()->m_player->m_pos;
				PlayWave("_confusion.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
			}
		}
		else
		{
			if (m_gid == g_session.GetAid())
			{
			}
		}

		if (m_healthState & HEALTHSTATE_BLIND)
		{
			if (m_gid == g_session.GetAid())
			{
				g_isBlackWorld = TRUE;
			}
			if (changedNow)
			{
				CGameMode * gameMode = g_modeMgr.GetGameMode();
				vector3d & pPos = gameMode->GetWorld()->m_player->m_pos;
				PlayWave("_blind.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);

				if (m_gid == g_session.GetAid())
				{
#ifdef SECONDJOB_SKILL2
					if (!gameMode->GetWorld()->m_player->IsEffectExist(EF_BLIND))
						CRagEffect *effect = LaunchEffect(EF_BLIND);
#endif
				}
			}
		}
		else
		{
			if (m_gid == g_session.GetAid())
			{
#ifdef SECONDJOB_SKILL2
				if (g_isBlackWorld == TRUE)
					SendMsg(NULL, AM_DELETE_EFFECT, EF_BLIND);
#endif
				g_isBlackWorld = FALSE;
			}
		}

	}

	if (m_bodyState==BODYSTATE_NORMAL) {
		m_isMotionFreezed = false;
	}

	if (m_healthState & HEALTHSTATE_POISON){}
	else if (m_healthState & HEALTHSTATE_HEAVYPOISON){}
	else if (m_healthState & HEALTHSTATE_CURSE){}
	else if ( (m_bodyState == BODYSTATE_FREEZING) || (m_bodyState == BODYSTATE_STONECURSE) || (m_bodyState == BODYSTATE_STONECURSE_ING) ){}
	else
	{
		m_isSprArgbFixed = false;

		SetArgb(-1,255,255,255);
	}

	if (m_gid == g_session.GetAid())
	{
		m_isVisibleBody = false;
		m_isVisible = false;
		m_shadowOn = false;
		if (g_FirstEffect <= 2)
		{
			if (g_FirstEffect == 2)
				g_modeMgr.GetGameMode()->GetWorld()->m_player->ReLaunchBlurEffects();
			g_FirstEffect++;
		}
	}
}

void	CGameActor :: ProcessEffectState()
{
	if (g_isSkipFrame > 0)
	{
		if (m_BeZero == 1)
		{
			m_BodyLight = 0;
			m_BeZero = 0;
		}
	}

	if (IsEffectStateNothing(m_effectState)) {
		m_effectLaunchCnt = 60*60*10;
		return;
	}

	if(IsEffectStateSight(m_effectState)) {
		int effectId = EF_SIGHT;

		int effectDuration = durationTable[effectId];
		if (m_effectLaunchCnt >= effectDuration) {
			m_effectLaunchCnt = 0;
			CRagEffect * effect = LaunchEffect(effectId);
			if (effect) {
				effect->SendMsg(NULL, AM_SETCOUNT, (timeGetTime()>>4)%10 + 1);
			}
		}
	}

	if(IsEffectStateRuwach(m_effectState)) {
		int effectId = EF_RUWACH;

		int effectDuration = durationTable[effectId];
		if (m_effectLaunchCnt >= effectDuration) {
			m_effectLaunchCnt = 0;
			CRagEffect * effect = LaunchEffect(effectId);
			if (effect) {
				effect->SendMsg(NULL, AM_SETCOUNT, (timeGetTime()>>4)%10 + 1);
			}
		}
	}

	++m_effectLaunchCnt;
}

void CGameActor::ProcessBodyState() {
	if (m_bodyState > 0 && m_bodyState <= 2) {
		m_isMotionFreezed = 1;
	}
}

BOOL CGameActor::IsDarkEffect(int effectId) {
	return effectId == 180;
}

void	CGameActor :: DeleteEmblemWnd()
{
	if (g_session.IsSiegeMode() && m_emblemWnd) {
		g_windowMgr.PostQuit((UIWindow*)m_emblemWnd);
		m_emblemWnd = NULL;
	}
}

void	CGameActor :: MakeEmblemWnd()
{
	if (g_session.IsSiegeMode())
	{
		extern ServiceType g_serviceType;
		if (g_serviceType != 3 && g_serviceType != 8 && g_serviceType != 9)
			g_modeMgr.GetGameMode()->MakeEmblemWnd(this, GetGdid(), GetEmblemVersion());
	}
}

void	CGameActor :: DeleteWordDisplayWnd()
{
	if (m_WordDisplayWnd) {
		g_windowMgr.PostQuit((UIWindow *)m_WordDisplayWnd);
		m_WordDisplayWnd = 0;
	}
}

void CGameActor::MakeWordDisplayWnd() {
	g_modeMgr.GetGameMode()->MakeWordDisplayWnd(this);
}
void	CRenderObject :: SetForceAnimation(int act, int mot, int maxMot, int finishAct, int finishMot, BOOL isAnim, DWORD endTick, BOOL isLoop)
{
	m_forceAct = act;
	m_forceMot = mot;
	m_forceStartMot = mot;
	m_forceMaxMot = maxMot;
	m_forceFinishedAct = finishAct;
	m_forceFinishedMot = finishMot;

	m_isForceAnimLoop = isLoop;
	m_isForceAnimation = isAnim;
	m_isForceAnimFinish = FALSE;

	m_isForceState = TRUE;
	m_isForceState3 = FALSE;
	m_isForceState2 = FALSE;

	if (isAnim) {
		m_forceStateEndTick = timeGetTime() + endTick;
	} else m_forceStateEndTick = 0;

	m_forceStateCnt = 1;
}

void	CRenderObject :: SetForceAnimation2(int act, int mot, int act2, int mot2, int act3, int mot3,int act4, int mot4,int act5, int mot5, BOOL isAnim, DWORD endTick, BOOL isLoop)
{
	m_forceAct2[0] = act;
	m_forceMot2[0] = mot;
	m_forceAct2[1] = act2;
	m_forceMot2[1] = mot2;
	m_forceAct2[2] = act3;
	m_forceMot2[2] = mot3;
	m_forceAct2[3] = act4;
	m_forceMot2[3] = mot4;
	m_forceAct2[4] = act5;
	m_forceMot2[4] = mot5;

	m_forceAct = act;
	m_forceMot = mot;

	m_forceStartMot = 0;
	m_forceMaxMot = 4;

	m_isForceAnimLoop = isLoop;
	m_isForceAnimation = isAnim;
	m_isForceAnimFinish = FALSE;

	m_isForceState2 = TRUE;
	m_isForceState = FALSE;
	m_isForceState3 = FALSE;

	if (isAnim) {
		m_forceStateEndTick = timeGetTime() + endTick;
	} else m_forceStateEndTick = 0;

	m_forceStateCnt = 1;
}

void	CRenderObject :: SetForceAnimation3(int act, int mot, int maxMot, BOOL isAnim, DWORD endTick, BOOL isLoop)
{
	m_forceAct = act;
	m_forceMot = mot;
	m_forceStartMot = mot;
	m_forceMaxMot = maxMot;
	m_forceFinishedAct = act;
	m_forceFinishedMot = maxMot;

	m_isForceAnimLoop = isLoop;
	m_isForceAnimation = isAnim;
	m_isForceAnimFinish = FALSE;

	m_isForceState3 = TRUE;
	m_isForceState2 = FALSE;
	m_isForceState = FALSE;
	m_BodyAni = 2;

	if (isAnim)
		m_forceStateEndTick = timeGetTime() + endTick;
	else
		m_forceStateEndTick = 0;

	m_forceStateCnt = 1;
}

void	CRenderObject :: SetForceAnimEndTick(DWORD tick)
{
	m_forceStateEndTick = timeGetTime() + tick;
}

void	CRenderObject :: SetForceAnimSpeed(BYTE speed)
{
	m_forceAnimSpeed = speed;
}

BOOL	CGameActor :: IsFreezeState()
{
	if (m_isBladeStop)
		return TRUE;
	return FALSE;
}

_MSG2AI&	CGameActor :: GetMsg2AI ( int merType )
{
	_MSG2AI * msg = NULL ;

	if ( merType == 0 )
		msg = &m_homunMsg;
	else if ( merType == 1 )
		msg = &m_merMsg;

	return *msg ;
}

void	CGameActor :: SetMsg2AI ( _MSG2AI& msg, int merType )
{
	if ( merType == 0 )
		m_homunMsg = msg;
	else if ( merType == 1 )
		m_merMsg = msg;
}

_MSG2AI&	CGameActor :: GetResMsg2AI ( int merType )
{
	_MSG2AI * resMsg = NULL ;

	if ( merType == 0 )
		resMsg = &m_homunResMsg ;
	else if ( merType == 1 )
		resMsg = &m_merResMsg ;

	return *resMsg ;
}

void	CGameActor :: SetResMsg2AI ( _MSG2AI& msg, int merType )
{
	if ( merType == 0 )
		m_homunResMsg = msg;
	else if ( merType == 1 )
		m_merResMsg = msg;
}
