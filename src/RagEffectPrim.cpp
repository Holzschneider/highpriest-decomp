#include "common.h"
#include "View.h"
#include "GameWorld.h"
#include "GameMode.h"
#include "RagEffect.h"
#include "device/Sound.h"
#include "Session.h"
#include "GameActor.h"
#include "3dlib/Prim.h"
#include "resource/Attr.h"
#include "resource/Wave.h"
#include "3dActor.h"
#include "packet.h"
#include "3dlib/Device.h"
#include "RagEffect2.h"
#include "RagEffectPrim.h"
#include "resource/Action.h"
#include "resource/Sprite.h"
#include "Mode.h"
#include "pdb_enums.h"
#include "resource/Ground.h"


CWeather g_Weather;

extern CModeMgr g_modeMgr;
extern CSession g_session;
extern CRenderer *g_renderer;
extern BOOL g_isSkipFrame;

typedef CSprClip* PSprClip;


inline float GetSprOow(float sprY, float tlvertY, float tvz)
{
	return 1.0f / (g_slope * (sprY - tlvertY) + tvz);
}

CEffectPrim :: CEffectPrim()
{
#if 1
	m_playWaveType = 0;
	m_playWaveName = NULL;

	m_any = 0;
	m_type = 0;
	m_pattern = 0;
	m_renderFlag = RF_ALPHA;

	m_isHit = FALSE;
	m_chSpeed = FALSE;
	m_AlphaShow = FALSE;
	m_chRadiSpeed = FALSE;
	m_isDisappear = FALSE;
	m_isChangeProcessed = FALSE;

	m_master = NULL;
	m_count = 0;
	m_chCnt = 0;
	m_stateCnt = 0;
	m_radius = 0;
	m_minRadius = 0;
	m_arcAngle = 36.0f;
	m_roty = 0;
	m_validAngle = 180;

	m_size = 1;
	m_innerSize = 0;
	m_outerSize = 0;
	m_widthSize = 0;
	m_heightSize = 0;
	m_maxHeightSize = 100;

	m_speed = 0;
	m_sizeSpeed = 0;
	m_radiusSpeed = 0;
	m_outerSpeed = 0;
	m_innerSpeed = 0;
	m_longSpeed = 0;
	m_latiSpeed = 0;
	m_rollSpeed = 0;
	m_widthSpeed = 0;
	m_heightSpeed = 0;
	m_alphaSpeed = 0;
	m_gravSpeed = 0;

	m_accel = 0;
	m_sizeAccel = 0;
	m_radiusAccel = 0;
	m_gravAccel = 0;
	m_longAccel = 0;
	m_latiAccel = 0;
	m_rollAccel = 0;
	m_outerAccel = 0;
	m_innerAccel = 0;
	m_widthAccel = 0;
	m_heightAccel = 0;

	m_red = 0xff;
	m_green = 0xff;
	m_blue = 0xff;
	m_redSpeed = 0;
	m_greenSpeed = 0;
	m_blueSpeed = 0;

	m_ChangePoint = -1;
	m_ChangeWidthSpeed = 0.0001f;
	m_ChangeHeightSpeed = 0.0001f;
	m_ChangeSpeed = 0.0001f;
	m_ChangeSpeedx = 0;
	m_ChangeSpeedy = 0;
	m_ChangeSpeedz = 0;
	m_ChangeAccelx = 0;
	m_ChangeAccely = 0;
	m_ChangeAccelz = 0;
	m_ChangeWidthAccel = 0.0001f;
	m_ChangeHeightAccel = 0.0001f;
	m_ChangeAccel = 0.0001f;
	m_RGBCycleCnt = 0;
	m_RGBCycleDelay = 0;
	m_commonPattern = 0;

	m_speed3d.Set(0, 0, 0);
	m_deltaPos.Set(0, 0, 0);
	m_deltaPos2.Set(0, 0, 0);
	m_deltaPos3.Set(0, 0, 0);
	m_deltaPosAccel.Set(0, 0, 0);
	m_longitude = 0;
	m_latitude = 0;
	m_roll = 0;
	m_duration = 100;
	m_fadeInCnt = -1;
	m_fadeOutCnt = -1;
	m_argb = 0xffffffff;
	m_alpha = 0xfe;

	m_alphaDelta = 1.0f;
	m_maxAlpha = 0xfe;
	m_minAlpha = 0;
	m_repeatAnim = true;
	m_numSegment = 1;
	m_animSpeed = 4;
	m_curAction = 0;
	m_curMotion = 0;
	m_totalTexture = 1;
	m_texture = NULL;
	m_u1 = m_v1 = 0;
	m_u2 = m_v2 = 1;

	m_matrix.MakeIdentity();
	m_rotMat.MakeIdentity();
#endif
}

CEffectPrim :: ~CEffectPrim()
{

	if (m_texture) {
		delete [] m_texture;
	}

	if (m_master)
		m_master->SendMsg(this, AM_CHILD_DEAD);
}

bool	CEffectPrim :: AnimTexture()
{
	if (m_stateCnt % m_animSpeed == 0)
	{
		m_curMotion++;

		if (m_repeatAnim == false) {
			if (m_curMotion == m_totalTexture)
				return false;
		}
		else {
			m_curMotion %= m_totalTexture;
		}
	}

	return true;
}

void	CEffectPrim :: InitSegment()
{
	for (int i = 0; i < m_numSegment; i++) {
		m_segment[i].pos = m_pos;
		m_segment[i].segPos[0] = m_segPos[0];
		m_segment[i].segPos[1] = m_segPos[1];
		m_segment[i].segPos[2] = m_segPos[2];
		m_segment[i].segPos[3] = m_segPos[3];
		m_segment[i].radius = m_radius;
		m_segment[i].size = m_size;
		m_segment[i].longitude = m_longitude;
		m_segment[i].mat = m_matrix;
		m_segment[i].alpha = m_alpha;
		m_segment[i].red = m_red;
		m_segment[i].green = m_green;
		m_segment[i].blue = m_blue;
		m_segment[i].argb = PACK32BIT8888((int)m_alpha, m_red, m_green, m_blue);
	}
}

bool	CEffectPrim :: AnimMotion()
{
	CActRes *	actRes = GetActRes();

	if (m_stateCnt % m_animSpeed == 0)
	{
		m_curMotion++;

		if (m_repeatAnim == false)
		{
			if (m_pattern & PT_STOP_AT_LAST)
			{
				if (m_curMotion == actRes->GetMotionCount(m_curAction))
				{
					m_curMotion--;
				}
			}
			else
			{
				if (m_curMotion == actRes->GetMotionCount(m_curAction))
					return false;
			}
		}
		else
			m_curMotion %= actRes->GetMotionCount(m_curAction);
	}

	return true;
}

void	CEffectPrim :: ProcessAlpha()
{
	if (m_AlphaShow)
	{
		if (m_chCnt < 0 || m_chCnt > m_chPoint.size()-1)
			m_chCnt = 0;
		if (m_stateCnt == m_chPoint[m_chCnt])
		{
			m_alpha = m_chVal1[m_chCnt];
			m_maxAlpha = m_chVal2[m_chCnt];
			m_chCnt++;
		}
		if (m_pattern & PT_TWINKLE)
		{
			if (m_alphaSpeed < 0)
			{
				if (m_alpha <= m_minAlpha)
					m_alphaSpeed *= -1;
			}
			else
			{
				if (m_alpha >= m_maxAlpha)
					m_alphaSpeed *= -1;
			}
		}

		if (m_isDisappear == FALSE &&
			(m_duration - m_stateCnt) > (m_master->m_duration - m_master->m_stateCnt + 1))
		{
			m_isDisappear = TRUE;
			m_pattern &= ~PT_TWINKLE;
			m_alphaSpeed = -(m_alpha / (m_master->m_duration - m_master->m_stateCnt));
		}

		if (m_stateCnt == m_fadeOutCnt) {
			m_pattern &= ~PT_TWINKLE;
			m_alphaSpeed = -(m_alpha / (m_duration - m_stateCnt)) * m_alphaDelta;
		}

		m_alpha += m_alphaSpeed;
		m_alpha = (m_alpha < m_minAlpha) ? m_minAlpha : m_alpha;
		m_alpha = (m_alpha > m_maxAlpha) ? m_maxAlpha : m_alpha;
		m_argb = PACK32BIT8888((int)m_alpha, m_red, m_green, m_blue);
		return;
	}
	else if (m_chSpeed)
	{
		if (m_stateCnt == m_chPoint[m_chCnt])
		{
			m_alphaSpeed = m_chVal1[m_chCnt];
			m_chCnt++;
		}
	}

	if (m_pattern & PT_TWINKLE)
	{
		if (m_alphaSpeed < 0)
		{
			if (m_alpha <= m_minAlpha)
				m_alphaSpeed *= -1;
		}
		else {
			if (m_alpha >= m_maxAlpha)
				m_alphaSpeed *= -1;
		}
	}

	if (m_isDisappear == false &&
		(m_duration - m_stateCnt) > (m_master->m_duration - m_master->m_stateCnt + 1))
	{
		m_isDisappear = true;
		m_pattern &= ~PT_TWINKLE;
		m_alphaSpeed = -(m_alpha / (m_master->m_duration - m_master->m_stateCnt));
	}

	if (m_stateCnt == m_fadeOutCnt)
	{
		m_pattern &= ~PT_TWINKLE;
		m_alphaSpeed = -(m_alpha / (m_duration - m_stateCnt)) * m_alphaDelta;
	}

	m_alpha += m_alphaSpeed;
	m_alpha = (m_alpha < m_minAlpha) ? m_minAlpha : m_alpha;
	m_alpha = (m_alpha > m_maxAlpha) ? m_maxAlpha : m_alpha;
	m_argb = PACK32BIT8888((int)m_alpha, m_red, m_green, m_blue);

}

void	CEffectPrim :: ShiftSegment()
{
	float	level;

	for (int i = m_numSegment - 1; i > 0; i--)
	{
		m_segment[i].pos = m_segment[i - 1].pos;
		m_segment[i].segPos[0] = m_segment[i - 1].segPos[0];
		m_segment[i].segPos[1] = m_segment[i - 1].segPos[1];
		m_segment[i].segPos[2] = m_segment[i - 1].segPos[2];
		m_segment[i].segPos[3] = m_segment[i - 1].segPos[3];
		m_segment[i].radius = m_segment[i - 1].radius;
		m_segment[i].longitude = m_segment[i - 1].longitude;
		m_segment[i].mat = m_segment[i - 1].mat;

		level = m_alpha / (float)m_numSegment;
		m_segment[i].alpha = m_alpha - ((float)i * level);
		m_segment[i].argb = PACK32BIT8888((int)m_segment[i].alpha, m_red, m_green, m_blue);

		level = m_size / (float)m_numSegment;
		m_segment[i].size = m_size - (((float)i * level) / 2.0f);
	}

	m_segment[0].pos = m_pos;
	m_segment[0].segPos[0] = m_segPos[0];
	m_segment[0].segPos[1] = m_segPos[1];
	m_segment[0].segPos[2] = m_segPos[2];
	m_segment[0].segPos[3] = m_segPos[3];
	m_segment[0].radius = m_radius;
	m_segment[0].size = m_size;
	m_segment[0].longitude = m_longitude;
	m_segment[0].mat = m_matrix;
	m_segment[0].alpha = m_alpha;
	m_segment[0].argb = PACK32BIT8888((int)m_segment[0].alpha, m_red, m_green, m_blue);
}

bool	CEffectPrim :: OnProcess()
{
#if 1
	if (m_master == NULL)
		return false;

	switch (m_type)
	{
		case	PP_2DTEXTURE:
		case	PP_3DCROSSTEXTURE:
			if (m_GI[0].life != 99)
			{
				if ( g_isSkipFrame == 0 )
					return true;
			}
			break;
		case	PP_CHOOKGI:
		case	PP_3DCASTING:
		case	PP_3DAURA:
		case	PP_DOUBLEGUMGANG:
		case	PP_ATTACKENERGY:
		case	PP_ATTACKENERGY2:
		case	PP_HALFSPHERE:
		case	PP_3DAURA_2:
		case	PP_SOULLINK:
		case	PP_GUMGANG:
		case	PP_HEAL:
		case	PP_ASURACASTING:
		case	PP_GI_1:
		case	PP_FLOWERCAST:
		case	PP_SOULPARTICLE:
			m_pos = m_master->m_pos;
		default:
			if ( g_isSkipFrame == 0 )
				return true;
			break;
	}

	switch (m_type) {
		case	PP_2DTEXTURE:
		case	PP_2DQUAD:
			if (!Prim2DTexture())
				return false;
			break;

		case	PP_2DTEXTUREORBIT:
			if (!Prim2DTextureOrbit())
				return false;
			break;

		case	PP_2DFLASH:
			Prim2DFlash();
			break;

		case	PP_2DCIRCLE:
			Prim2DCircle();
			break;

		case	PP_3DPARTICLE:
			if (!Prim3DParticle())
				return false;
			break;

		case	PP_3DPARTICLEGRAVITY:
			if (!Prim3DParticleGravity())
				return false;
			break;

		case	PP_3DPARTICLEORBIT:
			if (!Prim3DParticleOrbit())
				return false;
			break;

		case	PP_3DPARTICLESPLINE:
			Prim3DParticleSpline();
			break;

		case	PP_3DTEXTURE:
		case	PP_3DCROSSTEXTURE:
				Prim3DTexture();
				break;
		case	PP_3DQUADHORN:			Prim3DQuadHorn();			break;
		case	PP_3DQUADHORN2:			Prim3DQuadHorn2();			break;
		case	PP_3DCIRCLE:			Prim3DCircle();			break;
		case	PP_3DCYLINDER:			Prim3DCylinder();			break;
		case	PP_3DSPHERE:			Prim3DSphere();			break;
		case	PP_3DSHINELIGHT: break;
		case	PP_3DCYLINDERHALF:	Prim3DCylinderHalf();	break;
		case	PP_3DFLASH:				Prim3DFlash();				break;

		case	PP_3DSPLINE:
		case	PP_3DSPIRAL:
				Prim3DSpiral();
				break;
		case  PP_3DLINE:
				Prim3DLine();
				break;
		case  PP_3DPARTICLE_NOMASTER:
				if( !Prim3DParticleNoMaster() ) {
					return false;
				}
				break;
		case  PP_3DTEXTURE_NOMASTER:
				if( !Prim3DTextureNoMaster() ) {
					return false;
				}
				break;
		case	PP_3DCASTING:			Prim3DCasting();			break;
		case	PP_3DCASTING_2:			Prim3DCasting2();			break;
		case	PP_3DCASTING_3:			Prim3DCasting3();			break;
		case	PP_3DCASTING_4:			Prim3DCasting4();			break;
		case	PP_3DCASTING_5:			Prim3DCasting5();			break;
		case	PP_FLOWERCAST:			PrimFlowerCast();			break;
		case	PP_ACIDDEMON:			PrimAcidDemon();			break;
		case	PP_MAGNUM2:			PrimMagnum2();			break;
		case	PP_ASURACASTING:			PrimAsuraCasting();			break;
		case	PP_HITLINE:			PrimHitLine();			break;
		case	PP_HITLINE3:		PrimHitLine3();			break;
		case	PP_STORMKICK:		PrimStormKick();			break;
		case	PP_ELECTRIC:		PrimElectric();			break;
		case	PP_ELECTRIC2:		PrimElectric2();			break;
		case	PP_HEAL:			PrimHeal();			break;
		case	PP_HEAL_LOW:		PrimHeal();			break;
		case	PP_GUARD:			PrimGuard();			break;
		case	PP_HALFSPHERE:			PrimHalfSphere();			break;
		case	PP_ATTACKENERGY:			PrimAttackEnergy();			break;
		case	PP_ATTACKENERGY2:			PrimAttackEnergy2();			break;
		case	PP_SAINTWING:			PrimSaintWing();			break;
		case	PP_SPHEREWIND:			PrimSphereWind();			break;
		case	PP_SPHEREWIND2:			PrimSphereWind2();			break;
		case	PP_COLORPAPER:			PrimColorPaper();			break;
		case	PP_WIND:			PrimWind();			break;
		case	PP_PORTAL:			PrimPortal();			break;
		case	PP_3DAURA:			Prim3DAura();			break;
		case	PP_MAPMAGICZONE:			PrimMapMagicZone();			break;
		case	PP_GROUNDSHAKE:			PrimGroundShake();			break;
		case	PP_3DAURA_2:			Prim3DAura_2();			break;
		case	PP_MAPPARTICLE:			PrimMapParticle();			break;
		case	PP_3DAURA_3:			PrimParticleSpread();			break;
		case	PP_SOULPARTICLE:		PrimSoulParticle();			break;
		case	PP_SOULLINK:			PrimSoulLinkLight();			break;
		case	PP_PEONGUP:			PrimPeongUp();			break;
		case	PP_PEONG:			PrimPeong();			break;
		case	PP_WATERFALLPARTICLE:			PrimWaterFallParticle();			break;
		case	PP_GI_1:			PrimGI1();			break;
		case	PP_GI_2:			PrimGI2();			break;
		case	PP_GI_3:			PrimGI3();			break;
		case	PP_GI_4:			PrimGI4();			break;
		case	PP_GI_5:			PrimGI5();			break;

		case	PP_MAPPILLAR:			PrimMapPillar();			break;
		case	PP_FOREST:			PrimForest();			break;
		case	PP_WATERFALL:			PrimWaterFall();			break;
		case	PP_RAINBOW:			PrimRainbow();			break;

		case	PP_TELEPORTATION2:			PrimTeleportation2();			break;
		case	PP_GRANDCROSS2:			PrimGrandCross2();			break;
		case	PP_SLASH1:			PrimSlash1();			break;
		case	PP_RECT_UP:			PrimRectUp();			break;
		case	PP_RECT_UP2:			PrimRectUp2();			break;
		case	PP_INTIMIDATE:			PrimIntimidate();			break;
		case	PP_TEIHIT1:			PrimTeiHit1();			break;
		case	PP_SOULBREAKER:			PrimSoulBreaker();			break;
		case	PP_SOULBREAKER2:		PrimSoulBreaker2();			break;
		case	PP_STIN:			PrimSTIN();			break;
		case	PP_STIN2:			PrimSTIN2();			break;
		case	PP_STIN4:			PrimSTIN4();			break;
		case	PP_STIN5:			PrimSTIN5();			break;
		case	PP_SMA:				PrimSMA();			break;
		case	PP_SMA2:			PrimSMA2();			break;
		case	PP_CHOOKGI:
				if (!PrimCHOOKGI())
					return false;
				break;
		case	PP_LINELINK:			PrimLineLink();			break;
		case	PP_TRIPLEATTACK:			PrimTripleAttack();			break;
		case	PP_CLOUD:			PrimCloud();			break;
		case	PP_TWILIGHT:		PrimTwilight();			break;
		case	PP_POKJUK:			PrimPokJuk();			break;

		case	PP_GUMGANG:
		case	PP_DOUBLEGUMGANG:
				PrimGumGang();
				break;
		case	PP_LANDPROTECTOR:			PrimLandProtector();			break;
		case	PP_HERMODE:			PrimHermode();			break;
		case	PP_FOOT:			PrimFoot();			break;
		case	PP_SAKURA:
				PrimSakura();
				break;
		case	PP_KOUENKA:
				PrimKouenka();
				break;
		case	PP_CHIMTO:
				PrimChimTo();
				break;
		case	PP_GHOST:
				PrimGhost();
				break;
		case	PP_SUPERANGEL:
				PrimSuperAngel();
				break;
		case	PP_LIGHTSPHERE:			PrimLightSphere();			break;
		case	PP_SHIELDBOOMERANG:			PrimShieldBoomerang();			break;
		case	PP_SHIELDBOOMERANG3:		PrimShieldBoomerang3();			break;
		case	PP_TANJI2:			PrimTanji2();			break;
		case	PP_PRESSURE:			PrimPressure();			break;
		case	PP_BUBBLE_DROP:			PrimBubbleDrop();			break;
		case	PP_BASH3D:			PrimBash3D();			break;
		case	PP_FIRSTAID:			PrimFirstAid();			break;
		case	PP_AIRTEXTURE:			PrimAirTexture();			break;
		case	PP_THROWITEM:			PrimThrowItem();			break;
		case	PP_CHEMICALPROTECTION:			PrimChemical();			break;
		case	PP_CHEMICAL2:			PrimChemical2();			break;
		case	PP_CHEMICAL3:			PrimChemical3();			break;
		case	PP_GROUNDSAMPLE:		PrimGroundSample();			break;
		case	PP_GI_EXPLOSION:		PrimGiExplosion();			break;
		case	PP_CALLZONE:			PrimCallZone();			break;
		case	PP_BLIND:			PrimBlind();			break;
		case	PP_POISON:			PrimPoison();			break;
		case	PP_TEIHIT2:			PrimTeiHit2();			break;
		case	PP_PARTICLE_UP:			PrimParticleUp();			break;
		case	PP_BOTTOM:			PrimBottom();			break;
		case	PP_EFFECTTEXTURE:			PrimEffectTexture();			break;
		case	PP_EFFECTTEXTURE2:			PrimEffectTexture2();			break;
		case	PP_EFFECTTEXTURE_ANI:			PrimEffectTexture_Ani();			break;
		case	PP_BOTTOM2:			PrimBottom2();			break;
		case	PP_BOTTOMOUT:			PrimBottomOut();			break;
		case	PP_BOTTOMSPR:			PrimBottomSpr();			break;
		default:
				{
					char	buf[128];
					sprintf(buf, "Illegal Effect PrimitiveId %d", m_type);
					ErrorMsg(buf);
				}
			break;
	}

	m_stateCnt++;
	if (m_stateCnt > m_duration)
		return	false;
	else
		return	true;
#endif
	return 0;
}

bool	CEffectPrim :: Prim2DTexture()
{
#if 1
	if (m_ChangePoint == m_stateCnt) {
		if (m_ChangeWidthSpeed != 0.0001f)
			m_widthSpeed = m_ChangeWidthSpeed;
		if (m_ChangeHeightSpeed != 0.0001f)
			m_heightSpeed = m_ChangeHeightSpeed;
		if (m_ChangeSpeed != 0.0001f)
			m_speed = m_ChangeSpeed;
		if (m_ChangeWidthAccel != 0.0001f)
			m_widthAccel = m_ChangeWidthAccel;
		if (m_ChangeHeightAccel != 0.0001f)
			m_heightAccel = m_ChangeHeightAccel;
		if (m_ChangeAccel != 0.0001f)
			m_accel = m_ChangeAccel;
	}

	int pattern = m_pattern;
	if (pattern & PT2DTEX_FIXPOSITION) {
		m_pos.x = m_deltaPos2.x;
		m_pos.y = m_deltaPos2.y;
	} else {
		m_pos.x = m_master->m_tlvertX + m_deltaPos2.x;
		m_pos.y = m_master->m_tlvertY + m_deltaPos2.y;
	}

	m_speed += m_accel;
	m_rollSpeed += m_rollAccel;
	m_widthSpeed += m_widthAccel;
	m_heightSpeed += m_heightAccel;

	if ((pattern & PT2DTEX_SIZEACCELLIMIT) && m_stateCnt >= m_count) {
		m_widthSpeed = m_widthAccel = 0;
		m_heightSpeed = m_heightAccel = 0;
	}

	if (pattern & PT_TWINKLE) {
		if (m_alphaSpeed < 0) {
			if (m_alpha <= m_minAlpha)
				m_alphaSpeed *= -1;
		}
		else {
			if (m_alpha >= m_maxAlpha)
				m_alphaSpeed *= -1;
		}
	}

	m_roll += m_rollSpeed;
	m_widthSize += m_widthSpeed;
	m_heightSize += m_heightSpeed;

	m_deltaPos.x += m_speed * GetSin(m_longitude);
	m_deltaPos.y -= m_speed * GetCos(m_longitude);
	m_pos.x += m_deltaPos.x;
	m_pos.y += m_deltaPos.y;

	m_red += m_redSpeed;
	m_green += m_greenSpeed;
	m_blue += m_blueSpeed;

	if (m_red > 255)
		m_red = 0;
	else if (m_red < 0)
		m_red = 255;

	if (m_green > 255)
		m_green = 255;
	else if (m_green < 0)
		m_green = 0;

	if (m_blue > 255)
		m_blue = 255;
	else if (m_blue < 0)
		m_blue = 0;

	if (m_pattern & PT_BLOW) {
		if (m_stateCnt % m_count == 0) {
			m_longitude = (m_longitude - 8) + random(16);
			m_count = random(10) + 10;
		}
	}

	ProcessAlpha();
	if (!AnimTexture())
		return false;

	return true;
#endif
	return 0;
}

bool	CEffectPrim :: Prim2DTextureOrbit()
{
	m_orgPos.x = m_master->m_tlvertX + m_deltaPos2.x;
	m_orgPos.y = m_master->m_tlvertY + m_deltaPos2.y;

	m_radiusSpeed += m_radiusAccel;
	m_longSpeed += m_longAccel;
	m_widthSpeed += m_widthAccel;
	m_heightSpeed += m_heightAccel;
	m_rollSpeed += m_rollAccel;

	m_radius += m_radiusSpeed;
	m_longitude += m_longSpeed;
	m_roll += m_rollSpeed;
	m_widthSize += m_widthSpeed;
	m_heightSize += m_heightSpeed;

	m_pos.x = m_orgPos.x + m_radius * GetSin(m_longitude);
	m_pos.y = m_orgPos.y - m_radius * GetCos(m_longitude);

	ProcessAlpha();
	if (!AnimTexture())
		return false;

	return true;
	return 0;
}

void	CEffectPrim :: Prim3DFlash()
{
	m_pos.y = m_master->m_tlvertY + m_deltaPos2.y;
	m_pos.x = m_master->m_tlvertX + m_deltaPos2.x;

	if (m_pattern & PT2D_STOP)	{
		if (m_heightSize <= 0) {
			m_heightSpeed = 0.0f;
			m_heightAccel = 0.0f;
			Trace("m_heightSize  = %d", m_heightSize);
		}
	}

	if (m_stateCnt == m_ChangePoint) {
		m_heightSpeed = m_ChangeHeightSpeed;
		m_longSpeed = m_ChangeSpeed;
		m_longAccel = m_ChangeAccel;
	}

	if (m_heightSize < 0) {
		m_heightSize = 0;
		m_heightSpeed *= -1;
	}
	m_longSpeed += m_longAccel;
	m_heightSpeed += m_heightAccel;

	m_arcAngle += m_widthSpeed;
	m_longitude += m_longSpeed;
	m_heightSize += m_heightSpeed;

	ProcessAlpha();
}

void	CEffectPrim :: Prim2DCircle()
{
	m_pos.x = m_master->m_tlvertX + m_deltaPos2.x;
	m_pos.y = m_master->m_tlvertY + m_deltaPos2.y;

	m_radiusSpeed += m_radiusAccel;

	m_radius += m_radiusSpeed;


	if (m_pattern & PT_FILLCIRCLE || m_radius < m_innerSize)
		m_size = m_radius;
	else
		m_size = m_innerSize;

	ProcessAlpha();
}

void	CEffectPrim :: Prim2DFlash()
{
	m_pos.x = m_master->m_tlvertX + m_deltaPos2.x;
	m_pos.y = m_master->m_tlvertY + m_deltaPos2.y;

	if (m_pattern & PT2D_STOP)	{
		if (m_heightSize <= 0) {
			m_heightSpeed = 0.0f;
			m_heightAccel = 0.0f;
			Trace("m_heightSize  = %d", m_heightSize);
		}
	}

	if (m_stateCnt == m_ChangePoint) {
		m_heightSpeed = m_ChangeHeightSpeed;
		m_longSpeed = m_ChangeSpeed;
		m_longAccel = m_ChangeAccel;
	}

	if (m_heightSize < 0) {
		m_heightSize = 0;
		m_heightSpeed *= -1;
	}
	m_longSpeed += m_longAccel;
	m_heightSpeed += m_heightAccel;

	m_longitude += m_longSpeed;
	m_heightSize += m_heightSpeed;
	m_arcAngle += m_widthSpeed;

	ProcessAlpha();
}

bool	CEffectPrim :: Prim3DParticle()
{
#if 1
	m_orgPos = m_master->m_pos + m_deltaPos2;

	if (m_stateCnt == m_ChangePoint) {
		m_speed3d.x = m_ChangeSpeedx;
		m_speed3d.y = m_ChangeSpeedy;
		m_speed3d.z = m_ChangeSpeedz;
		m_deltaPosAccel.x = m_ChangeAccelx;
		m_deltaPosAccel.y = m_ChangeAccely;
		m_deltaPosAccel.z = m_ChangeAccelz;
	}

	m_speed += m_accel;
	m_sizeSpeed += m_sizeAccel;
	if (m_pattern & PT_DELTASPEED) {
		m_speed3d.x += m_deltaPosAccel.x;
		m_speed3d.y += m_deltaPosAccel.y;
		m_speed3d.z += m_deltaPosAccel.z;

		m_deltaPos += m_speed3d;
		m_roll += m_rollSpeed;
		m_size += m_sizeSpeed;
	} else if (((m_pattern & PT_NOWAIT) || m_stateCnt > m_fadeInCnt) ) {

		m_speed3d.MatrixMult(vector3d(0.0f, 0.0f, m_speed), m_matrix);


		m_deltaPos += m_speed3d;
		m_roll += m_rollSpeed;
		m_size += m_sizeSpeed;
	}


	if (m_pattern & PT_SPRINKLE) {

		if (m_stateCnt % m_count == 0) {
			m_count = random(8) + 12;
			m_deltaPos3.x = (float)(random(5) + 5) / 100.0f;
			m_deltaPos3.z = (float)(random(5) + 5) / 100.0f;

			if (random(2) == 0)
				m_deltaPos3.x *= -1;

			if (random(2) == 0)
				m_deltaPos3.z *= -1;
		}

		m_deltaPos.x += m_deltaPos3.x;
	}


	if (m_pattern & PT_CURVE) {
		if (m_stateCnt % m_count == 0) {
			m_speed = (float)(random(35) + 15) / 100;
			m_longitude = (m_longitude - 40) + random(80);
			m_latitude = (m_latitude - 40) + random(80);
			m_matrix.MakeYRotation(GetRadian((int)m_longitude));
			m_matrix.AppendXRotation(GetRadian((int)m_latitude));
			m_count = random(10) + 5;
		}
	}

	if (m_pattern & PT_BLOW) {
		if (m_stateCnt % m_count == 0) {
			m_longitude = (m_longitude - 5) + random(10);
			m_latitude = (m_latitude - 5) + random(10);
			m_matrix.MakeXRotation(GetRadian((int)m_latitude));
			m_matrix.AppendYRotation(GetRadian((int)m_longitude));
			m_count = random(15) + 5;
		}
	}

	m_pos = m_orgPos + m_deltaPos;

	if (m_pattern & PT_CHKTARGETPOS) {
		if (CalcDist(m_master->m_deltaPos.x, m_master->m_deltaPos.z) <= 3.0f)
			return false;
	}


	if (m_pattern & PT_ROTATESPR) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		m_roll = m_roty - gameMode->GetView()->GetCurLongitude();
	}

	ProcessAlpha();
	ShiftSegment();
	if (!AnimMotion())
		return false;

	return true;
#endif
	return 0;
}


bool	CEffectPrim :: Prim3DParticleNoMaster()
{
#if 1
	vector3d orgPos = m_orgPos + m_deltaPos2;

	if (m_stateCnt == m_ChangePoint) {
		m_speed3d.x = m_ChangeSpeedx;
		m_speed3d.y = m_ChangeSpeedy;
		m_speed3d.z = m_ChangeSpeedz;
		m_deltaPosAccel.x = m_ChangeAccelx;
		m_deltaPosAccel.y = m_ChangeAccely;
		m_deltaPosAccel.z = m_ChangeAccelz;
	}

	m_speed += m_accel;
	m_sizeSpeed += m_sizeAccel;
	if (m_pattern & PT_DELTASPEED) {
		m_speed3d.x += m_deltaPosAccel.x;
		m_speed3d.y += m_deltaPosAccel.y;
		m_speed3d.z += m_deltaPosAccel.z;

		m_deltaPos += m_speed3d;
		m_roll += m_rollSpeed;
		m_size += m_sizeSpeed;
	} else if (((m_pattern & PT_NOWAIT) || m_stateCnt > m_fadeInCnt) ) {
		m_speed3d.MatrixMult(vector3d(0.0f, 0.0f, m_speed), m_matrix);

		m_deltaPos += m_speed3d;
		m_roll += m_rollSpeed;
		m_size += m_sizeSpeed;
	}





	if (m_pattern & PT_SPRINKLE) {

		if (m_stateCnt % m_count == 0) {
			m_count = random(8) + 12;
			m_deltaPos3.x = (float)(random(5) + 5) / 100.0f;
			m_deltaPos3.z = (float)(random(5) + 5) / 100.0f;

			if (random(2) == 0)
				m_deltaPos3.x *= -1;

			if (random(2) == 0)
				m_deltaPos3.z *= -1;
		}

		m_deltaPos.x += m_deltaPos3.x;
	}


	if (m_pattern & PT_CURVE) {
		if (m_stateCnt % m_count == 0) {
			m_speed = (float)(random(35) + 15) / 100;
			m_longitude = (m_longitude - 40) + random(80);
			m_latitude = (m_latitude - 40) + random(80);
			m_matrix.MakeYRotation(GetRadian((int)m_longitude));
			m_matrix.AppendXRotation(GetRadian((int)m_latitude));
			m_count = random(10) + 5;
		}
	}

	if (m_pattern & PT_BLOW) {
		if (m_stateCnt % m_count == 0) {
			m_longitude = (m_longitude - 5) + random(10);
			m_latitude = (m_latitude - 5) + random(10);
			m_matrix.MakeXRotation(GetRadian((int)m_latitude));
			m_matrix.AppendYRotation(GetRadian((int)m_longitude));
			m_count = random(15) + 5;
		}
	}

	m_pos = orgPos + m_deltaPos;







	if (m_pattern & PT_ROTATESPR) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		m_roll = m_roty - gameMode->GetView()->GetCurLongitude();
	}

	ProcessAlpha();
	ShiftSegment();
	if (!AnimMotion()) return false;

	return true;
#endif
	return 0;
}

bool	CEffectPrim :: Prim3DParticleSpline()
{
	m_orgPos = m_master->m_pos + m_deltaPos2;

	m_speed += m_accel;
	m_latitude += m_latiSpeed;
	m_latiSpeed += m_latiAccel;
	m_gravSpeed += m_gravAccel;

	m_speed3d.MatrixMult(vector3d(0.0f, 0.0f, m_speed - m_gravSpeed), m_matrix);
	m_rotMat.MakeTransform(m_rotAxis, GetRadian(m_roll));
	m_deltaPos3.MatrixMult(vector3d(0, m_latitude, 0), m_rotMat);

	m_size += m_sizeSpeed;
	m_deltaPos -= m_speed3d;
	m_pos = m_orgPos + m_deltaPos + m_deltaPos3;

	ProcessAlpha();
	ShiftSegment();
	if (!AnimMotion())
		return false;

	return true;
	return 0;
}

bool	CEffectPrim :: Prim3DParticleGravity()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();

	m_orgPos = m_master->m_pos + m_deltaPos2;

	m_speed += m_accel;
	m_gravSpeed += m_gravAccel;
	m_speed = (m_speed < 0) ? 0 : m_speed;

	m_speed3d.MatrixMult(vector3d(0.0f, 0.0f, m_speed), m_matrix);
	m_deltaPos.x += m_speed3d.x;
	m_deltaPos.y += m_gravSpeed;
	m_deltaPos.z += m_speed3d.z;

	m_pos = m_orgPos + m_deltaPos;
	m_size += m_sizeSpeed;

	if (m_gravSpeed > 0 && m_pos.y > gameMode->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z)) {
		m_gravSpeed /= -1.5f;
		if (m_playWaveType == 1) {
			char buf[50];
			sprintf(buf, "effect\\EF_Coin%d.wav", random(3) + 1);
			vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
			PlayWave(buf, m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		}
	}

	ProcessAlpha();
	ShiftSegment();
	if (!AnimMotion())
		return false;

	return true;
	return 0;
}

bool	CEffectPrim :: Prim3DParticleOrbit()
{
	m_orgPos = m_master->m_pos;

	m_radiusSpeed += m_radiusAccel;
	m_gravSpeed += m_gravAccel;
	m_longSpeed += m_longAccel;

	m_radius += m_radiusSpeed;
	m_longitude += m_longSpeed;
	m_deltaPos2.y += m_gravSpeed;
	m_orgPos += m_deltaPos2;
	m_roll += m_rollSpeed;

	if (m_stateCnt == m_ChangePoint) {
		m_gravSpeed = m_ChangeSpeed;
		m_gravAccel = m_ChangeAccel;
	}

	if (m_chRadiSpeed) {
		if (m_stateCnt == m_chPoint[m_chCnt]) {
			m_radiusSpeed = m_chVal1[m_chCnt];
			m_radiusAccel = m_chVal2[m_chCnt];
			m_chCnt++;
		}
	}




	m_matrix.MakeYRotation(GetRadian(m_longitude));
	m_deltaPos.MatrixMult(vector3d(0, 0, m_radius), m_matrix);
	m_pos = m_orgPos + m_deltaPos;
	m_size += m_sizeSpeed;


	m_latitude += m_latiSpeed;
	m_matrix.AppendZRotation(m_latitude);


	ProcessAlpha();
	ShiftSegment();
	if (!AnimMotion())
		return false;

	return true;
	return 0;
}

void	CEffectPrim :: Prim3DQuadHorn()
{
	m_orgPos = m_master->m_pos + m_deltaPos2;

	if (m_stateCnt == m_ChangePoint) {
		m_speed = m_ChangeSpeed;
		m_accel = m_ChangeAccel;
	}

	m_speed += m_accel;
	m_speed3d.MatrixMult(vector3d(0, 0, m_speed), m_matrix);
	m_deltaPos += m_speed3d;
	m_pos = m_orgPos + m_deltaPos;


	if ((m_pattern & PTQH_SPEEDLIMIT) && m_stateCnt == m_count)
		m_speed = m_accel = 0;

	if (m_pattern & PTQH_RETURNDOWN) {
		if (m_stateCnt == int((m_duration - 30))) {
			m_speed = m_ChangeSpeedy;
			m_accel = m_ChangeAccely;
		}
		return;
	}

	ProcessAlpha();
}

void	CEffectPrim :: Prim3DQuadHorn2()
{
	m_orgPos = m_master->m_pos + m_deltaPos2;

	if (m_stateCnt >= 550)
	{
		if (m_stateCnt == 550)
		{
			m_speed = m_ChangeSpeed;
			m_accel = m_ChangeAccel;
		}
	}
	else if (m_stateCnt % 6 == 2)
	{
		m_speed = m_ChangeSpeed;
		m_accel = m_ChangeAccel;
	}
	else if (m_stateCnt % 6 == 5)
	{
		m_speed = 1.18f;
		m_accel = 0.01f;
	}

	m_speed += m_accel;
	m_speed3d.MatrixMult(vector3d(0, 0, m_speed), m_matrix);
	m_deltaPos += m_speed3d;
	m_pos = m_orgPos + m_deltaPos;

	ProcessAlpha();
}

void	CEffectPrim :: Prim3DQuadSquare()
{
	m_orgPos = m_master->m_pos + m_deltaPos2;

	m_speed += m_accel;
	m_speed3d.MatrixMult(vector3d(0, 0, m_speed), m_matrix);
	m_deltaPos += m_speed3d;
	m_pos = m_orgPos + m_deltaPos;

	if ((m_pattern & PTQH_SPEEDLIMIT) && m_stateCnt >= m_count)
		m_speed = m_accel = 0;

	ProcessAlpha();
}

bool	CEffectPrim :: Prim3DTexture()
{
	m_orgPos = m_master->m_pos + m_deltaPos2;

	if (m_ChangePoint == m_stateCnt) {
		m_isChangeProcessed = TRUE;
		if (m_ChangeWidthSpeed != 0.0001f)
			m_widthSpeed = m_ChangeWidthSpeed;
		if (m_ChangeHeightSpeed != 0.0001f)
			m_heightSpeed = m_ChangeHeightSpeed;
		if (m_ChangeSpeed != 0.0001f)
			m_speed = m_ChangeSpeed;
		if (m_ChangeWidthAccel != 0.0001f)
			m_widthAccel = m_ChangeWidthAccel;
		if (m_ChangeHeightAccel != 0.0001f)
			m_heightAccel = m_ChangeHeightAccel;
		if (m_ChangeAccel != 0.0001f)
			m_accel = m_ChangeAccel;

		if (m_ChangeSpeedx != 0.0001f)
			m_speed3d.x = m_ChangeSpeedx;

		if (m_ChangeSpeedy != 0.0001f)
			m_speed3d.y = m_ChangeSpeedy;

		if (m_ChangeSpeedz != 0.0001f)
			m_speed3d.z = m_ChangeSpeedz;

		m_ChangeAccelx = 0.0001f;
		m_ChangeAccely = 0.0001f;
		m_ChangeAccelz = 0.0001f;
		m_ChangeWidthSpeed = 0.0001f;
		m_ChangeHeightSpeed = 0.0001f;
		m_ChangeSpeed = 0.0001f;
		m_ChangeWidthAccel = 0.0001f;
		m_ChangeHeightAccel = 0.0001f;
		m_ChangeAccel = 0.0001f;
		m_ChangeSpeedx = 0.0001f;
		m_ChangeSpeedy = 0.0001f;
		m_ChangeSpeedz = 0.0001f;
		m_ChangeAccelx = 0.0001f;
		m_ChangeAccely = 0.0001f;
		m_ChangeAccelz = 0.0001f;
	}

	m_speed3d += m_deltaPosAccel;

	m_widthSpeed += m_widthAccel;
	m_heightSpeed += m_heightAccel;
	m_longSpeed += m_longAccel;
	m_latiSpeed += m_latiAccel;

	m_speed += m_accel;

	if (m_pattern & PT3DTEX_CHKTARGETPOS) {

		m_speed3d.Set(0, 0, 0);
		m_speed3d.MatrixMult(vector3d(0, 0, m_speed), m_matrix);
		m_deltaPos += m_speed3d;
	} else if (m_pattern & PT3DTEX_ACCELARATION) {
		m_deltaPos += m_speed3d;
	} else {
		m_speed3d.MatrixMult(vector3d(0, 0, m_speed), m_matrix);
		m_deltaPos += m_speed3d;
	}

	if (m_pattern & PT3DTEX_CHKTARGETPOS) {
		if (CalcDist(m_master->m_deltaPos.x, m_master->m_deltaPos.z) <= 3.0f) {
			m_isHit = TRUE;
			Trace("Hit!!");
			return false;
		}
		m_longitude = m_roty;
	}

	m_pos = m_orgPos + m_deltaPos;

	m_widthSize += m_widthSpeed;
	m_heightSize += m_heightSpeed;
	m_longitude += m_longSpeed;
	m_latitude += m_latiSpeed;

	if (m_heightSize > m_maxHeightSize)
		m_heightSpeed = m_heightAccel = 0;

	if (m_pattern & PT3DTEX_SHOWSIDEFACE) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		float roll = gameMode->GetView()->GetCurLatitude();

		m_rotMat.MakeZRotation(roll);
		m_rotMat.AppendYRotation(GetRadian(m_roty));

	} else if (m_pattern & PT3DTEX_SHOWFRONT) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		m_longitude = gameMode->GetView()->GetCurLongitude() + 90;
		m_roll = gameMode->GetView()->GetCurLatitude();

		m_rotMat.MakeXRotation(GetRadian(m_latitude));
		m_rotMat.AppendZRotation(GetRadian(m_roll));
		m_rotMat.AppendYRotation(GetRadian(m_longitude));
	} else {
		m_rotMat.MakeZRotation(GetRadian(m_roll));
		m_rotMat.AppendXRotation(GetRadian(m_latitude));
		m_rotMat.AppendYRotation(GetRadian(m_longitude));
	}

	if (m_pattern & PT3DTEX_MOVETEXTUREV) {
		m_v1 = (float)(m_count) / 10;
		m_v2 = m_v1 + 1.0f;

		m_count++;
		if (m_count == 10)
			m_count = 0;
	}

	if (m_pattern & PT3DTEX_MOVEDUMMY) {
		m_master->m_deltaPos = m_deltaPos2 + m_deltaPos;
	}

	if (m_pattern & PT3DTEX_CHECKGROUND) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();

		if (m_pos.y > gameMode->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z)) {
			m_pattern &= ~PT3DTEX_CHECKGROUND;
			m_speed = 0;
			m_stateCnt = m_duration - 25;
			m_master->m_flag = 1;
		}
	}

	if (m_pattern & PT3DTEX_WAVERINGLY) {
		if (m_stateCnt % m_any == 0)
			m_heightSpeed *= -1.0f;
	}

	if ((m_pattern & PT3DTEX_RGBCYCLE) && (m_red == 254 || m_blue == 254 || m_green == 254)) {
		if (m_RGBCycleDelay)
			m_RGBCycleCnt++;
		if (m_RGBCycleDelay == m_RGBCycleCnt) {
			m_red += m_redSpeed;
			m_green += m_greenSpeed;
			m_blue += m_blueSpeed;
			m_RGBCycleCnt = 0;
		}

	} else {
		m_red += m_redSpeed;
		m_green += m_greenSpeed;
		m_blue += m_blueSpeed;
	}

	if (m_pattern & PT3DTEX_RGBCYCLE) {
		if (m_red > 254)
			m_red = 0;
		else if (m_red < 0)
			m_red = 254;

		if (m_green > 254)
			m_green = 0;
		else if (m_green < 0)
			m_green = 254;

		if (m_blue > 254)
			m_blue = 0;
		else if (m_blue < 0)
			m_blue = 254;
	} else {
		if (m_red > 255)
			m_red = 0;
		else if (m_red < 0)
			m_red = 255;

		if (m_green > 255)
			m_green = 255;
		else if (m_green < 0)
			m_green = 0;

		if (m_blue > 255)
			m_blue = 255;
		else if (m_blue < 0)
			m_blue = 0;
	}

	ProcessAlpha();
	if (!AnimTexture())
		return false;

	return true;
	return 0;
}

bool	CEffectPrim :: Prim3DTextureNoMaster()
{
		vector3d orgPos = m_orgPos + m_deltaPos2;

	if (m_ChangePoint == m_stateCnt) {
		if (m_ChangeWidthSpeed != 0.0001f)
			m_widthSpeed = m_ChangeWidthSpeed;
		if (m_ChangeHeightSpeed != 0.0001f)
			m_heightSpeed = m_ChangeHeightSpeed;
		if (m_ChangeSpeed != 0.0001f)
			m_speed = m_ChangeSpeed;
		if (m_ChangeWidthAccel != 0.0001f)
			m_widthAccel = m_ChangeWidthAccel;
		if (m_ChangeHeightAccel != 0.0001f)
			m_heightAccel = m_ChangeHeightAccel;
		if (m_ChangeAccel != 0.0001f)
			m_accel = m_ChangeAccel;

		if (m_ChangeSpeedx != 0.0001f)
			m_speed3d.x = m_ChangeSpeedx;

		if (m_ChangeSpeedy != 0.0001f)
			m_speed3d.y = m_ChangeSpeedy;

		if (m_ChangeSpeedz != 0.0001f)
			m_speed3d.z = m_ChangeSpeedz;

		if (m_ChangeAccelx != 0.0001f)
			m_deltaPosAccel.x = m_ChangeAccelx;

		if (m_ChangeAccely != 0.0001f)
			m_deltaPosAccel.y = m_ChangeAccely;

		if (m_ChangeAccelz != 0.0001f)
			m_deltaPosAccel.z = m_ChangeAccelz;
	}

	m_speed3d += m_deltaPosAccel;

	m_widthSpeed += m_widthAccel;
	m_heightSpeed += m_heightAccel;
	m_longSpeed += m_longAccel;
	m_latiSpeed += m_latiAccel;

	m_speed += m_accel;

	if (m_pattern & PT3DTEX_CHKTARGETPOS) {

		m_speed3d.Set(0, 0, 0);
		m_speed3d.MatrixMult(vector3d(0, 0, m_speed), m_matrix);
		m_deltaPos += m_speed3d;
	} else if (m_pattern & PT3DTEX_ACCELARATION) {
		m_deltaPos += m_speed3d;
	} else {
		m_speed3d.MatrixMult(vector3d(0, 0, m_speed), m_matrix);
		m_deltaPos += m_speed3d;
	}

	if (m_pattern & PT3DTEX_CHKTARGETPOS) {
		if (CalcDist(m_master->m_deltaPos.x, m_master->m_deltaPos.z) <= 3.0f) {
			m_isHit = TRUE;
			Trace("Hit!!");
			return false;
		}
		m_longitude = m_roty;
	}

	m_pos = orgPos + m_deltaPos;

	m_widthSize += m_widthSpeed;
	m_heightSize += m_heightSpeed;
	m_longitude += m_longSpeed;
	m_latitude += m_latiSpeed;

	if (m_heightSize > m_maxHeightSize)
		m_heightSpeed = m_heightAccel = 0;

	if (m_pattern & PT3DTEX_SHOWSIDEFACE) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		float roll = gameMode->GetView()->GetCurLatitude();

		m_rotMat.MakeZRotation(roll);
		m_rotMat.AppendYRotation(GetRadian(m_roty));

	} else if (m_pattern & PT3DTEX_SHOWFRONT) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		m_longitude = gameMode->GetView()->GetCurLongitude() + 90;
		m_roll = gameMode->GetView()->GetCurLatitude();

		m_rotMat.MakeXRotation(GetRadian(m_latitude));
		m_rotMat.AppendZRotation(GetRadian(m_roll));
		m_rotMat.AppendYRotation(GetRadian(m_longitude));
	} else {
		m_rotMat.MakeZRotation(GetRadian(m_roll));
		m_rotMat.AppendXRotation(GetRadian(m_latitude));
		m_rotMat.AppendYRotation(GetRadian(m_longitude));
	}

	if (m_pattern & PT3DTEX_MOVETEXTUREV) {
		m_v1 = (float)(m_count) / 10;
		m_v2 = m_v1 + 1.0f;

		m_count++;
		if (m_count == 10)
			m_count = 0;
	}

	if (m_pattern & PT3DTEX_MOVEDUMMY) {
		m_master->m_deltaPos = m_deltaPos2 + m_deltaPos;
	}

	if (m_pattern & PT3DTEX_CHECKGROUND) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();

		if (m_pos.y > gameMode->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z)) {
			m_pattern &= ~PT3DTEX_CHECKGROUND;
			m_speed = 0;
			m_stateCnt = m_duration - 25;
			m_master->m_flag = 1;
		}
	}

	if (m_pattern & PT3DTEX_WAVERINGLY) {
		if (m_stateCnt % m_any == 0)
			m_heightSpeed *= -1.0f;
	}

	if ((m_pattern & PT3DTEX_RGBCYCLE) && (m_red == 254 || m_blue == 254 || m_green == 254)) {
		if (m_RGBCycleDelay)
			m_RGBCycleCnt++;
		if (m_RGBCycleDelay == m_RGBCycleCnt) {
			m_red += m_redSpeed;
			m_green += m_greenSpeed;
			m_blue += m_blueSpeed;
			m_RGBCycleCnt = 0;
		}

	} else {
		m_red += m_redSpeed;
		m_green += m_greenSpeed;
		m_blue += m_blueSpeed;
	}

	if (m_pattern & PT3DTEX_RGBCYCLE) {
		if (m_red > 254)
			m_red = 0;
		else if (m_red < 0)
			m_red = 254;

		if (m_green > 254)
			m_green = 0;
		else if (m_green < 0)
			m_green = 254;

		if (m_blue > 254)
			m_blue = 0;
		else if (m_blue < 0)
			m_blue = 254;
	} else {
		if (m_red > 255)
			m_red = 0;
		else if (m_red < 0)
			m_red = 255;

		if (m_green > 255)
			m_green = 255;
		else if (m_green < 0)
			m_green = 0;

		if (m_blue > 255)
			m_blue = 255;
		else if (m_blue < 0)
			m_blue = 0;
	}

	ProcessAlpha();
	if (!AnimTexture())
		return false;

	return true;
	return 0;
}

void	CEffectPrim :: Prim3DCircle()
{
	m_orgPos = m_master->m_pos + m_deltaPos2;

	if (m_ChangePoint == m_stateCnt)
	{
		m_radiusAccel = m_ChangeAccel;
		m_radiusSpeed = m_ChangeSpeed;
	}

	m_speed += m_accel;
	m_longSpeed += m_longAccel;
	m_radiusSpeed += m_radiusAccel;
	m_innerSpeed += m_innerAccel;
	m_longitude += m_longSpeed;
	m_radius += m_radiusSpeed;
	m_innerSize += m_innerSpeed;

	m_matrix.MakeXRotation(GetRadian(m_latitude));
	m_matrix.AppendYRotation(GetRadian(m_longitude));

	m_speed3d.MatrixMult(vector3d(0.0f, 0.0f, m_speed), m_matrix);
	m_deltaPos += m_speed3d;
	m_pos = m_orgPos + m_deltaPos;

	if (m_pattern & PT_FILLCIRCLE || m_radius < m_innerSize)
		m_size = m_radius;
	else
		m_size = m_innerSize;

	ProcessAlpha();
	ShiftSegment();
}

bool	CEffectPrim :: Prim3DCylinderHalf()
{
	m_orgPos = m_master->m_pos + m_deltaPos2;

	m_speed += m_accel;
	m_outerSpeed += m_outerAccel;
	m_innerSpeed += m_innerAccel;
	m_heightSpeed += m_heightAccel;

	m_matrix.MakeXRotation(GetRadian(m_latitude));
	m_matrix.AppendYRotation(GetRadian(m_longitude));

	m_speed3d.MatrixMult(vector3d(0, -m_speed, 0), m_matrix);
	m_deltaPos += m_speed3d;

	m_pos = m_orgPos + m_deltaPos;

	m_longitude += m_longSpeed;
	m_outerSize += m_outerSpeed;
	m_innerSize += m_innerSpeed;
	m_heightSize += m_heightSpeed;

	if (m_heightSize > m_maxHeightSize)
		m_heightSize = m_maxHeightSize;

	if (m_pattern & PT_DISAPPEAR) {
		if (m_stateCnt == m_fadeOutCnt)
			m_outerSpeed = -(m_outerSize / (m_duration - m_stateCnt + 1));
	}













	ProcessAlpha();
	if (!AnimTexture())
		return false;
	return true;
	return 0;
}

bool	CEffectPrim :: Prim3DCylinder()
{
	m_orgPos = m_master->m_pos + m_deltaPos2;

	m_speed += m_accel;
	m_heightSpeed += m_heightAccel;
	m_outerSpeed += m_outerAccel;
	m_innerSpeed += m_innerAccel;
	m_longSpeed += m_longAccel;
	m_latiSpeed += m_latiAccel;
	m_longitude += m_longSpeed;
	m_latitude += m_latiSpeed;

	m_matrix.MakeXRotation(GetRadian(m_latitude));
	m_matrix.AppendYRotation(GetRadian(m_longitude));

	m_speed3d.MatrixMult(vector3d(0, -m_speed, 0), m_matrix);
	m_deltaPos += m_speed3d;

	m_pos = m_orgPos + m_deltaPos;

	m_longitude += m_longSpeed;
	m_outerSize += m_outerSpeed;
	m_innerSize += m_innerSpeed;
	m_heightSize += m_heightSpeed;

	if (m_heightSize > m_maxHeightSize)
		m_heightSize = m_maxHeightSize;

	if (m_pattern & PT_DISAPPEAR) {
		if (m_stateCnt == m_fadeOutCnt)
			m_outerSpeed = -(m_outerSize / (m_duration - m_stateCnt + 1));
	}













	ProcessAlpha();
	if (!AnimTexture())
		return false;

	return true;
	return 0;
}

void	CEffectPrim :: Prim3DSpiral()
{
	m_orgPos = m_master->m_pos + m_deltaPos2;

	vector3d	vec1, vec2;

	vec1.x = m_orgPos.x + (m_radius * GetSin(m_longitude));
	vec1.y = m_orgPos.y;
	vec1.z = m_orgPos.z - (m_radius * GetCos(m_longitude));

	vec2.x = m_orgPos.x + (m_radius * GetSin(m_longitude + m_longSpeed));
	vec2.y = m_orgPos.y;
	vec2.z = m_orgPos.z - (m_radius * GetCos(m_longitude + m_longSpeed));

	m_segPos[0].Set(vec1.x, vec1.y, vec1.z);
	m_segPos[1].Set(vec2.x, vec2.y + m_heightSpeed, vec2.z);
	m_segPos[2].Set(vec1.x, vec1.y - m_size, vec1.z);
	m_segPos[3].Set(vec2.x, vec2.y + m_heightSpeed - m_size, vec2.z);

	ProcessAlpha();
	ShiftSegment();

	m_longitude += m_longSpeed;
	m_deltaPos2.y += m_heightSpeed;
}

void	CEffectPrim :: Prim3DSphere()
{
	m_orgPos = m_master->m_pos + m_deltaPos2;
	m_pos = m_orgPos;

	m_radiusSpeed += m_radiusAccel;
	m_radius += m_radiusSpeed;
	m_longitude += m_longSpeed;

	ProcessAlpha();
}

void	CEffectPrim :: Render2DQuad()
{
	float	sn = GetSin(m_roll);
	float	cs = GetCos(m_roll);
	int		i;
	float	x, y;
	float	halfWidth, halfHeight, halfHeight1;

	halfWidth = m_widthSize / 2.0f;
	halfHeight = m_heightSize / 2.0f;
	halfHeight1 = halfHeight / 1.2f;

	m_rp[0].verts[0].x = -halfWidth;
	m_rp[0].verts[0].y = -halfHeight + halfHeight1;
	m_rp[0].verts[0].z = 0.000001f;
	m_rp[0].verts[0].oow = 0.999999f;
	m_rp[0].verts[0].color = m_argb;
	m_rp[0].verts[0].tu = 0;
	m_rp[0].verts[0].tv = 0.5f;

	m_rp[0].verts[1].x = 0;
	m_rp[0].verts[1].z = 0.000001f;
	m_rp[0].verts[1].y = halfHeight;
	m_rp[0].verts[1].oow = 0.999999f;
	m_rp[0].verts[1].color = 0xffffffff;
	m_rp[0].verts[1].tu = 0.5f;
	m_rp[0].verts[1].tv = 1;

	m_rp[0].verts[2].x = 0;
	m_rp[0].verts[2].y = -halfHeight;
	m_rp[0].verts[2].z = 0.000001f;
	m_rp[0].verts[2].oow = 0.999999f;
	m_rp[0].verts[2].color = m_argb;
	m_rp[0].verts[2].tu = 0.5f;
	m_rp[0].verts[2].tv = 0;
	m_rp[0].tex = m_texture[m_curMotion];

	for (i = 0; i < 3; i++) {
		x = m_rp[0].verts[i].x;
		y = m_rp[0].verts[i].y;

		m_rp[0].verts[i].x = m_pos.x + (x * cs - y * sn);
		m_rp[0].verts[i].y = m_pos.y + (x * sn + y * cs);
	}







	g_renderer->AddRP(&m_rp[0], RF_ALPHA);

	m_rp[1].verts[0].x = halfWidth;
	m_rp[1].verts[0].y = halfHeight - halfHeight1;
	m_rp[1].verts[0].z = 0.000001f;
	m_rp[1].verts[0].oow = 0.999999f;
	m_rp[1].verts[0].color = m_argb;
	m_rp[1].verts[0].tu = 1;
	m_rp[1].verts[0].tv = 0.5f;

	m_rp[1].verts[1].x = 0;
	m_rp[1].verts[1].y = halfHeight;
	m_rp[1].verts[1].z = 0.000001f;
	m_rp[1].verts[1].oow = 0.999999f;
	m_rp[1].verts[1].color = 0xffffffff;
	m_rp[1].verts[1].tu = 0.5f;
	m_rp[1].verts[1].tv = 1;

	m_rp[1].verts[2].x = 0;
	m_rp[1].verts[2].y = -halfHeight;
	m_rp[1].verts[2].z = 0.000001f;
	m_rp[1].verts[2].oow = 0.999999f;
	m_rp[1].verts[2].color = m_argb;
	m_rp[1].verts[2].tu = 0.5;
	m_rp[1].verts[2].tv = 0;
	m_rp[1].tex = m_texture[m_curMotion];

	for (i = 0; i < 3; i++) {
		x = m_rp[1].verts[i].x;
		y = m_rp[1].verts[i].y;

		m_rp[1].verts[i].x = m_pos.x + (x * cs - y * sn);
		m_rp[1].verts[i].y = m_pos.y + (x * sn + y * cs);
	}






	g_renderer->AddRP(&m_rp[1], RF_ALPHA);
}

void	CEffectPrim :: Prim3DLine()
{
	m_pos.y += 5.0f;
}

void	CEffectPrim :: Render2DTexture()
{
	float	x, y, x1, y1;
	float	rx[4], ry[4];
	float	sinDeg, cosDeg;
	sinDeg = GetSin((int)m_roll);
	cosDeg = GetCos((int)m_roll);

	if (m_pattern & PT2DTEX_SIZEUP) {
		x = -m_widthSize;
		y = -m_heightSize * 2;
		x1 = m_widthSize;
		y1 = 0;
	}
	else {
		x = -m_widthSize;
		y = -m_heightSize;
		x1 = m_widthSize;
		y1 = m_heightSize;
	}

	rx[0] = (x * cosDeg) - (y * sinDeg);
	ry[0] = (x * sinDeg) + (y * cosDeg);
	rx[1] = (x1 * cosDeg) - (y * sinDeg);
	ry[1] = (x1 * sinDeg) + (y * cosDeg);
	rx[2] = (x * cosDeg) - (y1 * sinDeg);
	ry[2] = (x * sinDeg) + (y1 * cosDeg);
	rx[3] = (x1 * cosDeg) - (y1 * sinDeg);
	ry[3] = (x1 * sinDeg) + (y1 * cosDeg);

	m_rp[0].verts[0].x = m_pos.x + rx[2] - 0.5f;
	m_rp[0].verts[0].y = m_pos.y + ry[2] - 0.5f;
	m_rp[0].verts[0].z = 0.000001f;
	m_rp[0].verts[0].oow = 0.999999f;
	m_rp[0].verts[0].color = m_argb;
	m_rp[0].verts[0].tu = 0.0f;
	m_rp[0].verts[0].tv = 1.0f;

	m_rp[0].verts[1].x = m_pos.x + rx[0] - 0.5f;
	m_rp[0].verts[1].y = m_pos.y + ry[0] - 0.5f;
	m_rp[0].verts[1].z = 0.000001f;
	m_rp[0].verts[1].oow = 0.999999f;
	m_rp[0].verts[1].color = m_argb;
	m_rp[0].verts[1].tu = 0.0f;
	m_rp[0].verts[1].tv = 0.0f;

	m_rp[0].verts[2].x = m_pos.x + rx[3] - 0.5f;
	m_rp[0].verts[2].y = m_pos.y + ry[3] - 0.5f;
	m_rp[0].verts[2].z = 0.000001f;
	m_rp[0].verts[2].oow = 0.999999f;
	m_rp[0].verts[2].color = m_argb;
	m_rp[0].verts[2].tu = 1.0f;
	m_rp[0].verts[2].tv = 1.0f;
	m_rp[0].tex = m_texture[m_curMotion];







	g_renderer->AddRP(&m_rp[0], RF_ALPHA);

	m_rp[1].verts[0].x = m_pos.x + rx[3] - 0.5f;
	m_rp[1].verts[0].y = m_pos.y + ry[3] - 0.5f;
	m_rp[1].verts[0].z = 0.000001f;
	m_rp[1].verts[0].oow = 0.999999f;
	m_rp[1].verts[0].color = m_argb;
	m_rp[1].verts[0].tu = 1.0f;
	m_rp[1].verts[0].tv = 1.0f;

	m_rp[1].verts[1].x = m_pos.x + rx[0] - 0.5f;
	m_rp[1].verts[1].y = m_pos.y + ry[0] - 0.5f;
	m_rp[1].verts[1].z = 0.000001f;
	m_rp[1].verts[1].oow = 0.999999f;
	m_rp[1].verts[1].color = m_argb;
	m_rp[1].verts[1].tu = 0.0f;
	m_rp[1].verts[1].tv = 0.0f;

	m_rp[1].verts[2].x = m_pos.x + rx[1] - 0.5f;
	m_rp[1].verts[2].y = m_pos.y + ry[1] - 0.5f;
	m_rp[1].verts[2].z = 0.000001f;
	m_rp[1].verts[2].oow = 0.999999f;
	m_rp[1].verts[2].color = m_argb;
	m_rp[1].verts[2].tu = 1.0f;
	m_rp[1].verts[2].tv = 0.0f;
	m_rp[1].tex = m_texture[m_curMotion];







	g_renderer->AddRP(&m_rp[1], RF_ALPHA);
}

void	CEffectPrim :: Render2DFlash()
{
	float	cs1, sn1, cs2, sn2;
	cs1 = GetCos((int)(m_longitude - m_arcAngle));
	sn1 = GetSin((int)(m_longitude - m_arcAngle));
	cs2 = GetCos((int)(m_longitude + m_arcAngle));
	sn2 = GetSin((int)(m_longitude + m_arcAngle));

	m_rp[0].verts[0].x = m_pos.x;
	m_rp[0].verts[0].y = m_pos.y;
	m_rp[0].verts[0].z = 0.000001f;
	m_rp[0].verts[0].oow = 0.999999f;
	m_rp[0].verts[0].color = m_argb;
	m_rp[0].verts[0].tu = 0.5f;
	m_rp[0].verts[0].tv = 0.0f;

	m_rp[0].verts[1].x = m_pos.x + (m_heightSize * cs1);
	m_rp[0].verts[1].y = m_pos.y - (m_heightSize * sn1);
	m_rp[0].verts[1].z = 0.000001f;
	m_rp[0].verts[1].oow = 0.999999f;
	m_rp[0].verts[1].color = m_argb;
	m_rp[0].verts[1].tu = 1.0f;
	m_rp[0].verts[1].tv = 1.0f;

	m_rp[0].verts[2].x = m_pos.x + (m_heightSize * cs2);
	m_rp[0].verts[2].y = m_pos.y - (m_heightSize * sn2);
	m_rp[0].verts[2].z = 0.000001f;
	m_rp[0].verts[2].oow = 0.999999f;
	m_rp[0].verts[2].color = m_argb;
	m_rp[0].verts[2].tu = 0.0f;
	m_rp[0].verts[2].tv = 1.0f;
	m_rp[0].tex = m_texture[0];







	g_renderer->AddRP(&m_rp[0], RF_ALPHA);
}

void	CEffectPrim :: Render3DFlash(matrix &wtm)
{
	float	cs1, sn1, cs2, sn2;
	cs1 = GetCos((int)(m_longitude - m_arcAngle));
	sn1 = GetSin((int)(m_longitude - m_arcAngle));
	cs2 = GetCos((int)(m_longitude + m_arcAngle));
	sn2 = GetSin((int)(m_longitude + m_arcAngle));

	m_rp[0].verts[0].x = m_pos.x;
	m_rp[0].verts[0].y = m_pos.y;
	m_rp[0].verts[0].z = 0.000001f;
	m_rp[0].verts[0].oow = 0.999999f;
	m_rp[0].verts[0].color = m_argb;
	m_rp[0].verts[0].tu = 0.5f;
	m_rp[0].verts[0].tv = 0.0f;

	m_rp[0].verts[1].x = m_pos.x + (m_heightSize * cs1);
	m_rp[0].verts[1].y = m_pos.y - (m_heightSize * sn1);
	m_rp[0].verts[1].z = 0.000001f;
	m_rp[0].verts[1].oow = 0.999999f;
	m_rp[0].verts[1].color = m_argb;
	m_rp[0].verts[1].tu = 1.0f;
	m_rp[0].verts[1].tv = 1.0f;

	m_rp[0].verts[2].x = m_pos.x + (m_heightSize * cs2);
	m_rp[0].verts[2].y = m_pos.y - (m_heightSize * sn2);
	m_rp[0].verts[2].z = 0.000001f;
	m_rp[0].verts[2].oow = 0.999999f;
	m_rp[0].verts[2].color = m_argb;
	m_rp[0].verts[2].tu = 0.0f;
	m_rp[0].verts[2].tv = 1.0f;
	m_rp[0].tex = m_texture[0];







	g_renderer->AddRP(&m_rp[0], RF_ALPHA);
}

void	CEffectPrim :: Render3DLine(matrix &wtm)
{
	tlvertex3d	dpos;
	vector3d		v = m_pos;

	tlvertex3d	spos = TransformVert(wtm, v);
	v.y -= 10.0f;
	dpos = TransformVert(wtm, v);

	m_rp[0].verts[0].x = spos.x;
	m_rp[0].verts[0].y = spos.y;
	m_rp[0].verts[0].z = spos.z;
	m_rp[0].verts[0].oow = 0.999999f;
	m_rp[0].verts[0].color = PACK32BIT8888(156, 255, 255, 255);
	m_rp[0].verts[0].tu = 0.0f;
	m_rp[0].verts[0].tv = 0.0f;

	m_rp[0].verts[1].x = spos.x+1.0f;
	m_rp[0].verts[1].y = spos.y;
	m_rp[0].verts[1].z = spos.z;
	m_rp[0].verts[1].oow = 0.999999f;
	m_rp[0].verts[1].color = PACK32BIT8888(155, 255, 255, 255);
	m_rp[0].verts[1].tu = 0.0f;
	m_rp[0].verts[1].tv = 0.0f;

	m_rp[0].verts[2].x = dpos.x;
	m_rp[0].verts[2].y = dpos.y;
	m_rp[0].verts[2].z = dpos.z;
	m_rp[0].verts[2].oow = 0.999999f;
	m_rp[0].verts[2].color = PACK32BIT8888(155, 255, 255, 255);
	m_rp[0].verts[2].tu = 0.0f;
	m_rp[0].verts[2].tv = 0.0f;
	m_rp[0].tex = NULL;

	g_renderer->AddRP(&m_rp[0], RF_ALPHA | RF_EFFECT);
}

void	CEffectPrim :: Render2DCircle()
{
#if 1
	float	uInc = 0.0f;
	int		rpCnt = 0;
	float	i, innerSize, temp;
	float	cs1, sn1, cs2, sn2;

	innerSize = m_radius - m_size;
	temp = 360 - m_arcAngle;

	for (i = 0; i <= temp; i += m_arcAngle) {
		cs1 = g_cosTable[(int)i];
		sn1 = g_sinTable[(int)i];
		cs2 = g_cosTable[(int)(i + m_arcAngle)];
		sn2 = g_sinTable[(int)(i + m_arcAngle)];

		m_rp[rpCnt].verts[0].x = m_pos.x + (innerSize * cs1);
		m_rp[rpCnt].verts[0].y = m_pos.y - (innerSize * sn1);
		m_rp[rpCnt].verts[0].z = 0.000001f;
		m_rp[rpCnt].verts[0].oow = 0.999999f;
		m_rp[rpCnt].verts[0].color = m_argb;
		m_rp[rpCnt].verts[0].tu = uInc;
		m_rp[rpCnt].verts[0].tv = 1;

		m_rp[rpCnt].verts[1].x = m_pos.x + (m_radius * cs2);
		m_rp[rpCnt].verts[1].y = m_pos.y - (m_radius * sn2);
		m_rp[rpCnt].verts[1].z = 0.000001f;
		m_rp[rpCnt].verts[1].oow = 0.999999f;
		m_rp[rpCnt].verts[1].color = m_argb;
		m_rp[rpCnt].verts[1].tu = uInc + 0.25f;
		m_rp[rpCnt].verts[1].tv = 0;

		m_rp[rpCnt].verts[2].x = m_pos.x + (m_radius * cs1);
		m_rp[rpCnt].verts[2].y = m_pos.y - (m_radius * sn1);
		m_rp[rpCnt].verts[2].z = 0.000001f;
		m_rp[rpCnt].verts[2].oow = 0.999999f;
		m_rp[rpCnt].verts[2].color = m_argb;
		m_rp[rpCnt].verts[2].tu = uInc;
		m_rp[rpCnt].verts[2].tv = 0;
		m_rp[rpCnt].tex = m_texture[0];







		g_renderer->AddRP(&m_rp[rpCnt++], RF_ALPHA);

		m_rp[rpCnt].verts[0].x = m_pos.x + (innerSize * cs1);
		m_rp[rpCnt].verts[0].y = m_pos.y - (innerSize * sn1);
		m_rp[rpCnt].verts[0].z = 0.000001f;
		m_rp[rpCnt].verts[0].oow = 0.999999f;
		m_rp[rpCnt].verts[0].color = m_argb;
		m_rp[rpCnt].verts[0].tu = uInc;
		m_rp[rpCnt].verts[0].tv = 1;

		m_rp[rpCnt].verts[1].x = m_pos.x + (innerSize * cs2);
		m_rp[rpCnt].verts[1].y = m_pos.y - (innerSize * sn2);
		m_rp[rpCnt].verts[1].z = 0.000001f;
		m_rp[rpCnt].verts[1].oow = 0.999999f;
		m_rp[rpCnt].verts[1].color = m_argb;
		m_rp[rpCnt].verts[1].tu = uInc + 0.25f;
		m_rp[rpCnt].verts[1].tv = 1;

		m_rp[rpCnt].verts[2].x = m_pos.x + (m_radius * cs2);
		m_rp[rpCnt].verts[2].y = m_pos.y - (m_radius * sn2);
		m_rp[rpCnt].verts[2].z = 0.000001f;
		m_rp[rpCnt].verts[2].oow = 0.999999f;
		m_rp[rpCnt].verts[2].color = m_argb;
		m_rp[rpCnt].verts[2].tu = uInc + 0.25f;
		m_rp[rpCnt].verts[2].tv = 0;
		m_rp[rpCnt].tex = m_texture[0];








		g_renderer->AddRP(&m_rp[rpCnt++], RF_ALPHA);

		uInc += 0.25f;
		if (uInc >= 1.0f)
			uInc = 0.0f;
	}
#endif
}

void	CEffectPrim :: Render3DParticle(matrix &wtm)
{
#if 1
	CSprRes *	sprRes = GetSprRes();
	CActRes *	actRes = GetActRes();

	float		top, btm, left, right;
	vector3d	tvup;


	for (int j = 0; j < m_numSegment; j++)
	{

		vector3d	tv;
		tv.MatrixMult(m_segment[j].pos, wtm);

		float		x, y, oow;
		g_renderer->ProjectVertex(m_segment[j].pos, wtm, &x, &y, &oow);


		float	pixelRatio = g_renderer->GetPixelRatio(oow);


		CMotion *	mot = actRes->GetMotion(m_curAction, m_curMotion);

		for (int i = 0; i < mot->numClips; i++)
		{
			PSprClip	sprClip = mot->GetClip(i);
			SprImg *	img = sprRes->m_sprites[sprClip->clipType][sprClip->sprIndex];

			top = y + double(sprClip->y) * m_segment[j].size * sprClip->zoomy * pixelRatio;
			btm = y + double(sprClip->y + img->height*(img->isHalfH+1)) * m_segment[j].size * sprClip->zoomy * pixelRatio;

			if (top >= g_renderer->m_height || btm <= 0)
				continue;
			if (sprClip->flags & 1)
			{
				left = x + double(sprClip->x + img->width*(img->isHalfW+1)) * m_segment[j].size * sprClip->zoomx * pixelRatio;
				right = x + double(sprClip->x) * m_segment[j].size * sprClip->zoomx * pixelRatio;

				if (right >= g_renderer->m_width || left <= 0)
					continue;
			}
			else
			{
				left = x + double(sprClip->x) * m_segment[j].size * sprClip->zoomx * pixelRatio;
				right = x + double(sprClip->x + img->width*(img->isHalfW+1)) * m_segment[j].size * sprClip->zoomx * pixelRatio;

				if (left >= g_renderer->m_width || right <= 0)
					continue;
			}


			RPSprite	spr;
			spr.top = top;
			spr.bottom = btm;
			spr.left = left;
			spr.right = right;
			spr.oow = GetSprOow(btm, y, tv.z) + 0.00001f;
			spr.oowup = GetSprOow(top, y, tv.z) + 0.00001f;
			spr.spr = img;

			if (m_pattern & PT_USEORGARGB)
				spr.rgba = (sprClip->a<<24) + (sprClip->r<<16) + (sprClip->g<< 8) + sprClip->b;
			else
				spr.rgba = m_segment[j].argb;

			spr.angle = sprClip->angle + m_roll;

			RenderSprite(spr, sprRes->m_pal, m_renderFlag);


		}
	}
#endif
}

void	CEffectPrim :: Render3DQuadSquare(matrix &wtm)
{
	vector3d		vec[4][3], rotVec[4][3];
	float			uVal = 0.0f;
	int				i, rpCnt = 0;
	vec[0][0].x = -m_size;
	vec[0][0].y = -m_size;
	vec[0][0].z = 0;
	vec[0][1].x = 0;
	vec[0][1].y = 0;
	vec[0][1].z = m_heightSize;
	vec[0][2].x = m_size;
	vec[0][2].y = -m_size;
	vec[0][2].z = 0;

	vec[1][0].x = -m_size;
	vec[1][0].y = -m_size;
	vec[1][0].z = 0;
	vec[1][1].x = 0;
	vec[1][1].y = 0;
	vec[1][1].z = m_heightSize;
	vec[1][2].x = -m_size;
	vec[1][2].y = m_size;
	vec[1][2].z = 0;

	vec[2][0].x = -m_size;
	vec[2][0].y = m_size;
	vec[2][0].z = 0;
	vec[2][1].x = 0;
	vec[2][1].y = 0;
	vec[2][1].z = m_heightSize;
	vec[2][2].x = m_size;
	vec[2][2].y = m_size;
	vec[2][2].z = 0;

	vec[3][0].x = m_size;
	vec[3][0].y = m_size;
	vec[3][0].z = 0;
	vec[3][1].x = 0;
	vec[3][1].y = 0;
	vec[3][1].z = m_heightSize;
	vec[3][2].x = m_size;
	vec[3][2].y = -m_size;
	vec[3][2].z = 0;

	for (i = 0; i < 4; i++)
	{
		rotVec[i][0].MatrixMult(vec[i][0], m_rotMat);
		rotVec[i][1].MatrixMult(vec[i][1], m_rotMat);
		rotVec[i][2].MatrixMult(vec[i][2], m_rotMat);

		m_rp[rpCnt].verts[0] = TransformVert(wtm, m_pos + rotVec[i][0]);
		m_rp[rpCnt].verts[0].color = m_argb;
		m_rp[rpCnt].verts[0].tu = uVal;
		m_rp[rpCnt].verts[0].tv = 1;

		m_rp[rpCnt].verts[1] = TransformVert(wtm, m_pos + rotVec[i][1]);
		m_rp[rpCnt].verts[1].color = m_argb;
		m_rp[rpCnt].verts[1].tu = uVal;
		m_rp[rpCnt].verts[1].tv = 0;

		m_rp[rpCnt].verts[2] = TransformVert(wtm, m_pos + rotVec[i][2]);
		m_rp[rpCnt].verts[2].color = m_argb;
		m_rp[rpCnt].verts[2].tu = uVal + 0.2f;
		m_rp[rpCnt].verts[2].tv = 1;
		m_rp[rpCnt].tex = m_texture[0];







		g_renderer->AddRP(&m_rp[rpCnt++], RF_ALPHA);

		uVal += 0.2f;
	}
}

void	CEffectPrim :: Render3DQuadHorn(matrix &wtm)
{
	vector3d		vec[4][3], rotVec[4][3];
	int				i, rpCnt = 0;
	float			uVal = 0.0f;

	vec[0][0].x = -m_size;
	vec[0][0].y = -m_size;
	vec[0][0].z = 0;
	vec[0][1].x = 0;
	vec[0][1].y = 0;
	vec[0][1].z = m_heightSize;
	vec[0][2].x = m_size;
	vec[0][2].y = -m_size;
	vec[0][2].z = 0;

	vec[1][0].x = -m_size;
	vec[1][0].y = -m_size;
	vec[1][0].z = 0;
	vec[1][1].x = 0;
	vec[1][1].y = 0;
	vec[1][1].z = m_heightSize;
	vec[1][2].x = -m_size;
	vec[1][2].y = m_size;
	vec[1][2].z = 0;

	vec[2][0].x = -m_size;
	vec[2][0].y = m_size;
	vec[2][0].z = 0;
	vec[2][1].x = 0;
	vec[2][1].y = 0;
	vec[2][1].z = m_heightSize;
	vec[2][2].x = m_size;
	vec[2][2].y = m_size;
	vec[2][2].z = 0;

	vec[3][0].x = m_size;
	vec[3][0].y = m_size;
	vec[3][0].z = 0;
	vec[3][1].x = 0;
	vec[3][1].y = 0;
	vec[3][1].z = m_heightSize;
	vec[3][2].x = m_size;
	vec[3][2].y = -m_size;
	vec[3][2].z = 0;

	for (i = 0; i < 4; i++) {

		rotVec[i][0].MatrixMult(vec[i][0], m_rotMat);
		rotVec[i][1].MatrixMult(vec[i][1], m_rotMat);
		rotVec[i][2].MatrixMult(vec[i][2], m_rotMat);

		m_rp[rpCnt].verts[0] = TransformVert(wtm, m_pos + rotVec[i][0]);
		m_rp[rpCnt].verts[0].color = m_argb;
		m_rp[rpCnt].verts[0].tu = uVal;
		m_rp[rpCnt].verts[0].tv = 1;

		m_rp[rpCnt].verts[1] = TransformVert(wtm, m_pos + rotVec[i][1]);
		m_rp[rpCnt].verts[1].color = m_argb;
		m_rp[rpCnt].verts[1].tu = uVal;
		m_rp[rpCnt].verts[1].tv = 0;

		m_rp[rpCnt].verts[2] = TransformVert(wtm, m_pos + rotVec[i][2]);
		m_rp[rpCnt].verts[2].color = m_argb;
		m_rp[rpCnt].verts[2].tu = uVal + 0.2f;
		m_rp[rpCnt].verts[2].tv = 1;
		m_rp[rpCnt].tex = m_texture[0];







		g_renderer->AddRP(&m_rp[rpCnt++], RF_ALPHA);

		uVal += 0.2f;
	}
}

void	CEffectPrim :: Render3DTexture(matrix &wtm)
{
#if 1
	vector3d		vec[4], rotVec[4];
	float scale = 2.0f ;
	if (m_pattern & PT3DTEX_GROWUPPER)
	{
		vec[0].x = 0;
		vec[0].y = -m_heightSize * 2 ;
		vec[0].z = m_widthSize ;

		vec[1].x = 0;
		vec[1].y = -m_heightSize * 2 ;
		vec[1].z = -m_widthSize ;

		vec[2].x = 0;
		vec[2].y = 0;
		vec[2].z = m_widthSize ;

		vec[3].x = 0;
		vec[3].y = 0;
		vec[3].z = -m_widthSize ;
	}
	else
	{
		vec[0].x = 0;
		vec[0].y = -m_heightSize  ;
		vec[0].z = m_widthSize  ;

		vec[1].x = 0;
		vec[1].y = -m_heightSize  ;
		vec[1].z = -m_widthSize  ;

		vec[2].x = 0;
		vec[2].y = m_heightSize  ;
		vec[2].z = m_widthSize  ;

		vec[3].x = 0;
		vec[3].y = m_heightSize  ;
		vec[3].z = -m_widthSize  ;
	}

	if (m_pattern & PT3DTEX_SIZEUP)
	{
		vec[2].y = 0;
		vec[3].y = 0;
	}




	rotVec[0].MatrixMult(vec[0], m_rotMat);
	rotVec[1].MatrixMult(vec[1], m_rotMat);
	rotVec[2].MatrixMult(vec[2], m_rotMat);
	rotVec[3].MatrixMult(vec[3], m_rotMat);

	m_rp[0].verts[0] = TransformVert(wtm, m_pos + rotVec[0]);
	m_rp[0].verts[0].color = m_argb;
	m_rp[0].verts[0].tu = 0.0f;
	m_rp[0].verts[0].tv = m_v1;

	m_rp[0].verts[1] = TransformVert(wtm, m_pos + rotVec[1]);
	m_rp[0].verts[1].color = m_argb;
	m_rp[0].verts[1].tu = 1.0f;
	m_rp[0].verts[1].tv = m_v1;

	m_rp[0].verts[2] = TransformVert(wtm, m_pos + rotVec[2]);
	m_rp[0].verts[2].color = m_argb;
	m_rp[0].verts[2].tu = 0.0f;
	m_rp[0].verts[2].tv = m_v2;







	m_rp[0].tex = m_texture[m_curMotion];


	m_rp[1].verts[0] = TransformVert(wtm, m_pos + rotVec[3]);
	m_rp[1].verts[0].color = m_argb;
	m_rp[1].verts[0].tu = 1.0f;
	m_rp[1].verts[0].tv = m_v2;

	m_rp[1].verts[1] = TransformVert(wtm, m_pos + rotVec[2]);
	m_rp[1].verts[1].color = m_argb;
	m_rp[1].verts[1].tu = 0.0f;
	m_rp[1].verts[1].tv = m_v2;

	m_rp[1].verts[2] = TransformVert(wtm, m_pos + rotVec[1]);
	m_rp[1].verts[2].color = m_argb;
	m_rp[1].verts[2].tu = 1.0f;
	m_rp[1].verts[2].tv = m_v1;
	m_rp[1].tex = m_texture[m_curMotion];







	g_renderer->AddRP(&m_rp[0], m_renderFlag);
	g_renderer->AddRP(&m_rp[1], m_renderFlag);
#endif
}

void	CEffectPrim :: Render3DTextureBillboard(matrix &wtm)
{
	float x, y, oow;
	g_renderer->ProjectVertex(m_pos, wtm, &x, &y, &oow);


	float	pixelRatio = g_renderer->GetPixelRatio(oow);
	float fWidth = m_widthSize*pixelRatio, fHeight = m_heightSize*pixelRatio;

	m_rp[0].verts[0].x = x;
	m_rp[0].verts[0].y = y;
	m_rp[0].verts[0].z = 0.8f;
	m_rp[0].verts[0].color = m_argb;
	m_rp[0].verts[0].tu = 0.0f;
	m_rp[0].verts[0].tv = 0.0f;
	m_rp[0].verts[0].oow = 1.0f;

	m_rp[0].verts[1].x = x + fWidth;
	m_rp[0].verts[1].y = y;
	m_rp[0].verts[1].z = 0.8f;
	m_rp[0].verts[1].color = m_argb;
	m_rp[0].verts[1].tu = 1.0f;
	m_rp[0].verts[1].tv = 0.0f;
	m_rp[0].verts[1].oow = 1.0f;

	m_rp[0].verts[2].x = x;
	m_rp[0].verts[2].y = y - fHeight;
	m_rp[0].verts[2].z = 0.8f;
	m_rp[0].verts[2].color = m_argb;
	m_rp[0].verts[2].tu = 0.0f;
	m_rp[0].verts[2].tv = 1.0f;
	m_rp[0].verts[2].oow = 1.0f;
	m_rp[0].tex = m_texture[m_curMotion];

	m_rp[1].verts[0].x = x + fWidth;
	m_rp[1].verts[0].y = y;
	m_rp[1].verts[0].z = 0.8f;
	m_rp[1].verts[0].color = m_argb;
	m_rp[1].verts[0].tu = 1.0f;
	m_rp[1].verts[0].tv = 0.0f;
	m_rp[1].verts[0].oow = 1.0f;

	m_rp[1].verts[1].x = x;
	m_rp[1].verts[1].y = y - fHeight;
	m_rp[1].verts[1].z = 0.8f;
	m_rp[1].verts[1].color = m_argb;
	m_rp[1].verts[1].tu = 0.0f;
	m_rp[1].verts[1].tv = 1.0f;
	m_rp[1].verts[1].oow = 1.0f;

	m_rp[1].verts[2].x = x + fWidth;
	m_rp[1].verts[2].y = y - fHeight;
	m_rp[1].verts[2].z = 0.8f;
	m_rp[1].verts[2].color = m_argb;
	m_rp[1].verts[2].tu = 1.0f;
	m_rp[1].verts[2].tv = 1.0f;
	m_rp[1].verts[2].oow = 1.0f;
	m_rp[1].tex = m_texture[m_curMotion];






	g_renderer->AddRP(&m_rp[0], m_renderFlag);
	g_renderer->AddRP(&m_rp[1], m_renderFlag);
}

void	CEffectPrim :: Render3DCrossTexture(matrix &wtm)
{
	vector3d	vec[2][4], rotVec[4];
	int			rpCnt = 0;

	vec[0][0].x = -m_heightSize;
	vec[0][0].y = 0;
	vec[0][0].z = m_widthSize;

	vec[0][1].x = -m_heightSize;
	vec[0][1].y = 0;
	vec[0][1].z = -m_widthSize;

	vec[0][2].x = m_heightSize;
	vec[0][2].y = 0;
	vec[0][2].z = m_widthSize;

	vec[0][3].x = m_heightSize;
	vec[0][3].y = 0;
	vec[0][3].z = -m_widthSize;

	vec[1][0].x = 0;
	vec[1][0].y = -m_heightSize;
	vec[1][0].z = m_widthSize;

	vec[1][1].x = 0;
	vec[1][1].y = -m_heightSize;
	vec[1][1].z = -m_widthSize;

	vec[1][2].x = 0;
	vec[1][2].y = m_heightSize;
	vec[1][2].z = m_widthSize;

	vec[1][3].x = 0;
	vec[1][3].y = m_heightSize;
	vec[1][3].z = -m_widthSize;

	for (int i = 0; i < 2; i++) {
		rotVec[0].MatrixMult(vec[i][0], m_matrix);
		rotVec[1].MatrixMult(vec[i][1], m_matrix);
		rotVec[2].MatrixMult(vec[i][2], m_matrix);
		rotVec[3].MatrixMult(vec[i][3], m_matrix);

		m_rp[rpCnt].verts[0] = TransformVert(wtm, m_pos + rotVec[0]);
		m_rp[rpCnt].verts[0].color = m_argb;
		m_rp[rpCnt].verts[0].tu = 0.0f;
		m_rp[rpCnt].verts[0].tv = 1.0f;

		m_rp[rpCnt].verts[1] = TransformVert(wtm, m_pos + rotVec[1]);
		m_rp[rpCnt].verts[1].color = m_argb;
		m_rp[rpCnt].verts[1].tu = 1.0f;
		m_rp[rpCnt].verts[1].tv = 1.0f;

		m_rp[rpCnt].verts[2] = TransformVert(wtm, m_pos + rotVec[2]);
		m_rp[rpCnt].verts[2].color = m_argb;
		m_rp[rpCnt].verts[2].tu = 0.0f;
		m_rp[rpCnt].verts[2].tv = 0.0f;
		m_rp[rpCnt].tex = m_texture[m_curMotion];






		g_renderer->AddRP(&m_rp[rpCnt++], RF_ALPHA);

		m_rp[rpCnt].verts[0] = TransformVert(wtm, m_pos + rotVec[3]);
		m_rp[rpCnt].verts[0].color = m_argb;
		m_rp[rpCnt].verts[0].tu = 1.0f;
		m_rp[rpCnt].verts[0].tv = 0.0f;
		m_rp[rpCnt].verts[1] = TransformVert(wtm, m_pos + rotVec[2]);
		m_rp[rpCnt].verts[1].color = m_argb;
		m_rp[rpCnt].verts[1].tu = 0.0f;
		m_rp[rpCnt].verts[1].tv = 0.0f;
		m_rp[rpCnt].verts[2] = TransformVert(wtm, m_pos + rotVec[1]);
		m_rp[rpCnt].verts[2].color = m_argb;
		m_rp[rpCnt].verts[2].tu = 1.0f;
		m_rp[rpCnt].verts[2].tv = 1.0f;
		m_rp[rpCnt].tex = m_texture[m_curMotion];






		g_renderer->AddRP(&m_rp[rpCnt++], RF_ALPHA);
	}
}

void	CEffectPrim :: Render3DCircle(matrix &wtm)
{
	int			i;
	float		innerSize, temp;
	float		uInc = 0.0f;
	int			rpCnt = 0;
	float		cs1, sn1, cs2, sn2;
	vector3d	vec1, vec2, vec3, vec4;

	temp = 360 - m_arcAngle;

	for (int j = 0; j < m_numSegment; j++)
	{
		innerSize = m_segment[j].radius - m_size;

		for (i = 0; i <= temp; i += m_arcAngle)
		{
			cs1 = g_cosTable[(int)i];
			sn1 = g_sinTable[(int)i];
			cs2 = g_cosTable[(int)(i + m_arcAngle)];
			sn2 = g_sinTable[(int)(i + m_arcAngle)];

			vec1.MatrixMult(vector3d(innerSize * cs1, innerSize * sn1, 0.0f), m_matrix);
			vec2.MatrixMult(vector3d(innerSize * cs2, innerSize * sn2, 0.0f), m_matrix);
			vec3.MatrixMult(vector3d(m_segment[j].radius * cs1, m_segment[j].radius * sn1, 0.0f), m_matrix);
			vec4.MatrixMult(vector3d(m_segment[j].radius * cs2, m_segment[j].radius * sn2, 0.0f), m_matrix);
			vec1 += m_segment[j].pos;
			vec2 += m_segment[j].pos;
			vec3 += m_segment[j].pos;
			vec4 += m_segment[j].pos;

			m_rp[rpCnt].verts[0] = TransformVert(wtm, vec1);
			m_rp[rpCnt].verts[0].color = m_segment[j].argb;
			m_rp[rpCnt].verts[0].tu = uInc;
			m_rp[rpCnt].verts[0].tv = 1.0f;

			m_rp[rpCnt].verts[1] = TransformVert(wtm, vec2);
			m_rp[rpCnt].verts[1].color = m_segment[j].argb;
			m_rp[rpCnt].verts[1].tu = uInc + 0.25f;
			m_rp[rpCnt].verts[1].tv = 1.0f;

			m_rp[rpCnt].verts[2] = TransformVert(wtm, vec3);
			m_rp[rpCnt].verts[2].color = m_segment[j].argb;
			m_rp[rpCnt].verts[2].tu = uInc;
			m_rp[rpCnt].verts[2].tv = 0.0f;
			m_rp[rpCnt].tex = m_texture[0];






			g_renderer->AddRP(&m_rp[rpCnt++], m_renderFlag);

			m_rp[rpCnt].verts[0] = TransformVert(wtm, vec4);
			m_rp[rpCnt].verts[0].color = m_segment[j].argb;
			m_rp[rpCnt].verts[0].tu = uInc + 0.25f;
			m_rp[rpCnt].verts[0].tv = 0.0f;

			m_rp[rpCnt].verts[1] = TransformVert(wtm, vec3);
			m_rp[rpCnt].verts[1].color = m_segment[j].argb;
			m_rp[rpCnt].verts[1].tu = uInc;
			m_rp[rpCnt].verts[1].tv = 0.0f;

			m_rp[rpCnt].verts[2] = TransformVert(wtm, vec2);
			m_rp[rpCnt].verts[2].color = m_segment[j].argb;
			m_rp[rpCnt].verts[2].tu = uInc + 0.25f;
			m_rp[rpCnt].verts[2].tv = 1.0f;
			m_rp[rpCnt].tex = m_texture[0];







			g_renderer->AddRP(&m_rp[rpCnt++], m_renderFlag);

			uInc += 0.25f;
			if (uInc >= 1.0f)
				uInc = 0.0f;
		}
	}
}

void	CEffectPrim :: Render3DCylinder(matrix &wtm)
{
	float		i, temp;
	float		cs1, sn1, cs2, sn2;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		uInc = 0.0f;

	temp = 360 - m_arcAngle;

	for (i = 0; i <= temp; i += m_arcAngle) {
		cs1 = g_cosTable[(int)i];
		sn1 = g_sinTable[(int)i];
		cs2 = g_cosTable[(int)(i + m_arcAngle)];
		sn2 = g_sinTable[(int)(i + m_arcAngle)];

		vec1.MatrixMult(vector3d(m_innerSize * cs1, 0, m_innerSize * sn1), m_matrix);
		vec2.MatrixMult(vector3d(m_innerSize * cs2, 0, m_innerSize * sn2), m_matrix);
		vec3.MatrixMult(vector3d(m_outerSize * cs1, -m_heightSize, m_outerSize * sn1), m_matrix);
		vec4.MatrixMult(vector3d(m_outerSize * cs2, -m_heightSize, m_outerSize * sn2), m_matrix);
		vec1 += m_pos;
		vec2 += m_pos;
		vec3 += m_pos;
		vec4 += m_pos;

		m_rp[rpCnt].verts[0] = TransformVert(wtm, vec1);
		m_rp[rpCnt].verts[0].color = m_argb;
		m_rp[rpCnt].verts[0].tu = uInc;
		m_rp[rpCnt].verts[0].tv = 1.0f;

		m_rp[rpCnt].verts[2] = TransformVert(wtm, vec2);
		m_rp[rpCnt].verts[2].color = m_argb;
		m_rp[rpCnt].verts[2].tu = uInc + 0.25f;
		m_rp[rpCnt].verts[2].tv = 1.0f;

		m_rp[rpCnt].verts[1] = TransformVert(wtm, vec3);
		m_rp[rpCnt].verts[1].color = m_argb;
		m_rp[rpCnt].verts[1].tu = uInc;
		m_rp[rpCnt].verts[1].tv = 0.0f;
		m_rp[rpCnt].tex = m_texture[m_curMotion];








		g_renderer->AddRP(&m_rp[rpCnt++], RF_ALPHA);

		m_rp[rpCnt].verts[2] = TransformVert(wtm, vec4);
		m_rp[rpCnt].verts[2].color = m_argb;
		m_rp[rpCnt].verts[2].tu = uInc + 0.25f;
		m_rp[rpCnt].verts[2].tv = 0.0f;

		m_rp[rpCnt].verts[1] = TransformVert(wtm, vec3);
		m_rp[rpCnt].verts[1].color = m_argb;
		m_rp[rpCnt].verts[1].tu = uInc;
		m_rp[rpCnt].verts[1].tv = 0.0f;

		m_rp[rpCnt].verts[0] = TransformVert(wtm, vec2);
		m_rp[rpCnt].verts[0].color = m_argb;
		m_rp[rpCnt].verts[0].tu = uInc + 0.25f;
		m_rp[rpCnt].verts[0].tv = 1.0f;
		m_rp[rpCnt].tex = m_texture[m_curMotion];








		g_renderer->AddRP(&m_rp[rpCnt++], RF_ALPHA);

		uInc += 0.25f;
		if (uInc >= 1.0f)
			uInc = 0.0f;
	}
}

void	CEffectPrim :: Render3DSegment(matrix &wtm)
{
	int		rpCnt = 0;
	for (int i = 0; i < m_numSegment; i++) {
		m_rp[rpCnt].verts[0] = TransformVert(wtm, m_segment[i].segPos[0]);
		m_rp[rpCnt].verts[0].color = m_segment[i].argb;
		m_rp[rpCnt].verts[0].tu = 0;
		m_rp[rpCnt].verts[0].tv = 1;
		m_rp[rpCnt].verts[1] = TransformVert(wtm, m_segment[i].segPos[1]);
		m_rp[rpCnt].verts[1].color = m_segment[i].argb;
		m_rp[rpCnt].verts[1].tu = 1;
		m_rp[rpCnt].verts[1].tv = 1;
		m_rp[rpCnt].verts[2] = TransformVert(wtm, m_segment[i].segPos[2]);
		m_rp[rpCnt].verts[2].color = m_segment[i].argb;
		m_rp[rpCnt].verts[2].tu = 0;
		m_rp[rpCnt].verts[2].tv = 0;
		m_rp[rpCnt].tex = m_texture[0];






		g_renderer->AddRP(&m_rp[rpCnt++], RF_ALPHA);

		m_rp[rpCnt].verts[0] = TransformVert(wtm, m_segment[i].segPos[3]);
		m_rp[rpCnt].verts[0].color = m_segment[i].argb;
		m_rp[rpCnt].verts[0].tu = 1;
		m_rp[rpCnt].verts[0].tv = 0;
		m_rp[rpCnt].verts[1] = TransformVert(wtm, m_segment[i].segPos[2]);
		m_rp[rpCnt].verts[1].color = m_segment[i].argb;
		m_rp[rpCnt].verts[1].tu = 0;
		m_rp[rpCnt].verts[1].tv = 0;
		m_rp[rpCnt].verts[2] = TransformVert(wtm, m_segment[i].segPos[1]);
		m_rp[rpCnt].verts[2].color = m_segment[i].argb;
		m_rp[rpCnt].verts[2].tu = 1;
		m_rp[rpCnt].verts[2].tv = 1;
		m_rp[rpCnt].tex = m_texture[0];






		g_renderer->AddRP(&m_rp[rpCnt++], RF_ALPHA);
	}
}

void	CEffectPrim :: Render3DSphere(matrix &wtm)
{
	float		i, j, temp, temp1;
	float		radian[4];
	matrix		mat1, mat2, mat3, mat4;
	vector3d	vec1, vec2, vec3, vec4;
	int			rpCnt = 0;
	float		incU = 1.0f / (360.0f / m_arcAngle);
	float		incV = 1.0f / (180.0f / m_arcAngle);
	float		uVal, vVal = 0.0f;

	temp = 360 - m_arcAngle;
	temp1 = 90 - m_arcAngle;

	for (i = -90; i <= temp1; i += m_arcAngle)
	{
		uVal = 0.0f;

		for (j = 0; j <= temp; j += m_arcAngle)
		{
			radian[0] = GetRadian(j + m_longitude);
			radian[1] = GetRadian(i);
			radian[2] = GetRadian((j + m_arcAngle + 0.25f) + m_longitude);
			radian[3] = GetRadian(i + m_arcAngle);

			mat1.MakeXRotation(radian[1]);
			mat1.AppendYRotation(radian[0]);
			vec1.MatrixMult(vector3d(0.0f, 0.0f, m_radius), mat1);

			mat2.MakeXRotation(radian[3]);
			mat2.AppendYRotation(radian[0]);
			vec2.MatrixMult(vector3d(0.0f, 0.0f, m_radius), mat2);

			mat3.MakeXRotation(radian[1]);
			mat3.AppendYRotation(radian[2]);
			vec3.MatrixMult(vector3d(0.0f, 0.0f, m_radius), mat3);

			mat4.MakeXRotation(radian[3]);
			mat4.AppendYRotation(radian[2]);
			vec4.MatrixMult(vector3d(0.0f, 0.0f, m_radius), mat4);
			vec1 += m_pos;
			vec2 += m_pos;
			vec3 += m_pos;
			vec4 += m_pos;

			m_rp[rpCnt].verts[0] = TransformVert(wtm, vec1);
			m_rp[rpCnt].verts[0].color = m_argb;
			m_rp[rpCnt].verts[0].tu = uVal;
			m_rp[rpCnt].verts[0].tv = vVal;

			m_rp[rpCnt].verts[1] = TransformVert(wtm, vec2);
			m_rp[rpCnt].verts[1].color = m_argb;
			m_rp[rpCnt].verts[1].tu = uVal;
			m_rp[rpCnt].verts[1].tv = vVal + incV;

			m_rp[rpCnt].verts[2] = TransformVert(wtm, vec3);
			m_rp[rpCnt].verts[2].color = m_argb;
			m_rp[rpCnt].verts[2].tu = uVal + incU;
			m_rp[rpCnt].verts[2].tv = vVal;
			m_rp[rpCnt].tex = m_texture[0];








			g_renderer->AddRP(&m_rp[rpCnt++], RF_ALPHA);

			m_rp[rpCnt].verts[0] = TransformVert(wtm, vec4);
			m_rp[rpCnt].verts[0].color = m_argb;
			m_rp[rpCnt].verts[0].tu = uVal + incU;
			m_rp[rpCnt].verts[0].tv = vVal + incV;

			m_rp[rpCnt].verts[1] = TransformVert(wtm, vec3);
			m_rp[rpCnt].verts[1].color = m_argb;
			m_rp[rpCnt].verts[1].tu = uVal + incU;
			m_rp[rpCnt].verts[1].tv = vVal;

			m_rp[rpCnt].verts[2] = TransformVert(wtm, vec2);
			m_rp[rpCnt].verts[2].color = m_argb;
			m_rp[rpCnt].verts[2].tu = uVal;
			m_rp[rpCnt].verts[2].tv = vVal + incV;
			m_rp[rpCnt].tex = m_texture[0];









			g_renderer->AddRP(&m_rp[rpCnt++], RF_ALPHA );

			uVal += incU;
		}

		vVal += incV;
	}
}

void	CEffectPrim :: SendMsg(CGameObject *sender, int msgid, INT_PTR val1, INT_PTR val2, INT_PTR val3)
{
	if (msgid == 0x18) m_master = NULL;
}

void	CEffectPrim :: Init(CRagEffect * master, EFFECTPRIMID effectPrimId, vector3d fDeltaPos )
{
	m_master = master;
	m_deltaPos += fDeltaPos;
	m_type = effectPrimId;
	m_pos = m_orgPos = master->m_pos;
}

void	CEffectPrim :: Render(matrix &wtm)
{
#if 1
	if (!m_isVisible)
		return;

	switch (m_type) {
		case	PP_2DTEXTURE:
		case	PP_2DTEXTUREORBIT:
			Render2DTexture();
			break;

		case	PP_2DQUAD:
			Render2DQuad();
			break;

		case	PP_2DFLASH:
			Render2DFlash();
			break;

		case	PP_2DCIRCLE:
			Render2DCircle();
			break;

		case	PP_3DPARTICLE:
		case	PP_3DPARTICLEGRAVITY:
		case	PP_3DPARTICLEORBIT:
		case	PP_3DPARTICLESPLINE:
		case  PP_3DPARTICLE_NOMASTER:
		case  PP_SAKURA:
		case  PP_KOUENKA:
		case  PP_CHIMTO:
		case  PP_GHOST:
		case  PP_SUPERANGEL:
		case  PP_BOTTOMSPR:
			Render3DParticle(wtm);
			break;
		case	PP_3DQUADHORN:
		case	PP_3DQUADHORN2:
			Render3DQuadHorn(wtm);		break;
		case	PP_3DTEXTURE:
			Render3DTexture(wtm);
			break;
		case  PP_3DTEXTURE_NOMASTER:
		  Render3DTextureBillboard(wtm);
			break;
		case	PP_3DCROSSTEXTURE:	Render3DCrossTexture(wtm);	break;

		 case	PP_3DCIRCLE:
			Render3DCircle(wtm);
			break;
		case	PP_3DCYLINDER:
			Render3DCylinder(wtm);
			break;
		case	PP_HALFSPHERE:
			RenderHalfSphere(wtm);
			break;
		case	PP_3DCASTING:
		case	PP_3DCASTING_2:
		case	PP_3DCASTING_3:
		case	PP_3DCASTING_4:
		case	PP_3DCASTING_5:
		case	PP_MAPPILLAR:
		case	PP_HEAL:
		case	PP_PORTAL:
		case	PP_FLOWERCAST:
		case	PP_GI_EXPLOSION:
		case	PP_ACIDDEMON:
			Render3DCasting(wtm);
			break;
		case	PP_SMA2:
			RenderSMA2(wtm);
			break;
		case	PP_MAGNUM2:
			RenderMagnum2(wtm);
			break;
		case	PP_GUARD:
			RenderGuard(wtm);
			break;
		case	PP_ATTACKENERGY:
			RenderAttackEnergy(wtm);
			break;
		case	PP_ATTACKENERGY2:
			RenderAttackEnergy2(wtm);
			break;
		case	PP_SAINTWING:
			RenderSaintWing(wtm);
			break;
		case	PP_SPHEREWIND:
		case	PP_SPHEREWIND2:
			RenderSphereWind(wtm);
			break;
		case	PP_COLORPAPER:
			RenderColorPaper(wtm);
			break;
		case	PP_WIND:
			RenderWind(wtm);
			break;
		case	PP_3DAURA:
		case	PP_MAPMAGICZONE:
		case	PP_GROUNDSHAKE:
		case	PP_LANDPROTECTOR:
		case	PP_FOOT:
			Render3DAura(wtm);
			break;
		case	PP_HERMODE:
			RenderHermode(wtm);
			break;
		case	PP_ASURACASTING:
			RenderAsuraCasting(wtm);
			break;
		case	PP_HITLINE:
			RenderHitLine(wtm);
			break;
		case	PP_HITLINE3:
			RenderHitLine3(wtm);
			break;
		case	PP_STORMKICK:
			RenderStormKick(wtm);
			break;
		case	PP_ELECTRIC:
			RenderElectric(wtm);
			break;
		case	PP_ELECTRIC2:
			RenderElectric2(wtm);
			break;
		case	PP_MAPPARTICLE:
			RenderMapParticle(wtm);
			break;
		case	PP_3DAURA_2:
		case	PP_WATERFALLPARTICLE:
			Render3DAura_2(wtm);
			break;
		case	PP_SOULPARTICLE:
		case	PP_3DAURA_3:
			RenderSoulParticle(wtm);
			break;
		case	PP_SOULLINK:
			RenderSoulLink(wtm);
			break;
		case	PP_PEONGUP:
			RenderPeongUp(wtm);
			break;
		case	PP_PEONG:
			RenderPeong(wtm);
			break;
		case	PP_GUMGANG:
			RenderGumGang(wtm, 1.0f, 100, 100, 255);
			break;
		case	PP_DOUBLEGUMGANG:
			RenderDoubleGumGang(wtm);
			break;
		case	PP_GI_1:
		case	PP_GI_2:
		case	PP_GI_5:
		case	PP_TELEPORTATION2:
			Render3DCasting(wtm);
			break;
		case	PP_GI_4:
		case	PP_HEAL_LOW:
			Render3DCasting_LowPolygon(wtm);
			break;
		case	PP_GI_3:
			Render3DCasting2(wtm);
			break;
		case	PP_GRANDCROSS2:
			RenderGrandCross2(wtm);
			break;
		case	PP_SLASH1:
			RenderSlash1(wtm);
			break;
		case	PP_RECT_UP:
			RenderRectUp(wtm);
			break;
		case	PP_RECT_UP2:
			RenderRectUp2(wtm);
			break;
		case	PP_INTIMIDATE:
			RenderIntimidate(wtm);
			break;
		case	PP_TEIHIT1:
			RenderTeiHit1(wtm);
			break;
		case	PP_SOULBREAKER:
		case	PP_SOULBREAKER2:
			RenderSoulBreaker(wtm);
			break;
		case	PP_STIN:
		case	PP_STIN2:
		case	PP_STIN4:
			RenderSTIN(wtm);
			break;
		case	PP_STIN5:
			RenderSTIN5(wtm);
			break;
		case	PP_SMA:
			RenderSMA(wtm);
			break;
		case	PP_AIRTEXTURE:
			RenderAirTexture(wtm);
			break;
		case	PP_CHOOKGI:
			RenderCHOOKGI(wtm);
			break;
		case	PP_CLOUD:
		case	PP_SHIELDBOOMERANG:
		case	PP_SHIELDBOOMERANG3:
		case	PP_TANJI2:
		case	PP_PRESSURE:
		case	PP_BUBBLE_DROP:
		case	PP_FIRSTAID:
		case	PP_THROWITEM:
		case	PP_TEIHIT2:
		case	PP_PARTICLE_UP:
		case	PP_BOTTOM:
		case	PP_BOTTOMOUT:
		case	PP_TWILIGHT:
			RenderCloud(wtm);
			break;
		case	PP_EFFECTTEXTURE:
		case	PP_EFFECTTEXTURE2:
		case	PP_EFFECTTEXTURE_ANI:
			RenderEffectTexture(wtm);
			break;
		case	PP_CHEMICALPROTECTION:
			RenderChemical(wtm);
			break;
		case	PP_BASH3D:
			RenderBash3D(wtm);
			break;
		case	PP_CHEMICAL2:
			RenderChemical2(wtm);
			break;
		case	PP_CHEMICAL3:
			RenderChemical3(wtm);
			break;
		case	PP_GROUNDSAMPLE:
			RenderGroundSample(wtm);
			break;
		case	PP_CALLZONE:
			RenderCallZone(wtm);
			break;
		case	PP_BLIND:
			RenderBlind(wtm);
			break;
		case	PP_POISON:
			RenderPoison(wtm);
			break;
		case	PP_POKJUK:
			RenderPokJuk(wtm);
			break;
		case	PP_LINELINK:
			RenderLineLink(wtm);
			break;
		case	PP_TRIPLEATTACK:
			RenderTripleAttack(wtm);
			break;
		case	PP_FOREST:
			RenderForest(wtm);
			break;
		case	PP_WATERFALL:
			RenderWaterFall(wtm);
			break;
		case	PP_RAINBOW:
			RenderRainbow(wtm);
			break;
		case	PP_LIGHTSPHERE:
			RenderLightSphere(wtm);
			break;
		case	PP_BOTTOM2:
			RenderBottom2(wtm);
			break;
		case	PP_3DSPLINE:
		case	PP_3DSPIRAL:
			Render3DSegment(wtm);
			break;

		case	PP_3DSPHERE:
			Render3DSphere(wtm);
			break;

		case PP_3DFLASH:
			Render3DFlash(wtm);
			break;

		case PP_3DLINE:
			Render3DLine(wtm);
			break;
	}
#endif
}

CWeather :: CWeather()
{
	m_bNight = false;
	m_bObjLight = true;
	m_Effect = NULL;
	m_pPlayer = NULL;
	m_nDayCount = 0;
	m_Diffuse = vector3d(1.0f, 1.0f, 1.0f);
	m_Ambient = vector3d(0.0f, 0.0f, 0.0f);
	m_EnvDiff = vector3d(1.0f, 1.0f, 1.0f);
}

CWeather :: ~CWeather()
{
}

void CWeather :: Process(void)
{

	if( m_bNight==true )
	{
		if( m_Diffuse.x>0.5f )
			m_Diffuse.x-=0.005f;
		if( m_Diffuse.y>0.5f )
			m_Diffuse.y-=0.005f;
	}
	else
	{
		if( m_Diffuse.x<1.0f && m_Diffuse.x<m_MaxDiffuse.x )
			m_Diffuse.x+=0.005f;
		if( m_Diffuse.y<1.0f && m_Diffuse.y<m_MaxDiffuse.y )
			m_Diffuse.y+=0.005f;
	}

	if( m_Effect )
	{
		if( m_pPlayer->IsExistEffect(m_Effect)==false )
			m_Effect = NULL;
	}

	m_EnvDiff.x = 1.0f - ((1.0f-m_Diffuse.x) * (1.0f-m_Ambient.x));
	m_EnvDiff.y = 1.0f - ((1.0f-m_Diffuse.y) * (1.0f-m_Ambient.y));
	m_EnvDiff.z = 1.0f - ((1.0f-m_Diffuse.z) * (1.0f-m_Ambient.z));

	if( m_nDayCount++==2880 ) m_nDayCount = 0;

}

void CWeather :: SetNightDegree( void )
{
}

void CWeather :: LaunchSnow( void )
{
	if( m_Effect ) {
		if( m_Effect->GetDuration ()-m_Effect->GetRagStateCnt()<=300 ) return;
	}
	CGameMode *		gameMode = g_modeMgr.GetGameMode();

	m_pPlayer = gameMode->GetWorld()->m_player;
	m_Effect = m_pPlayer->LaunchEffect(EF_SNOW);
}

void CWeather :: StopSnow( void )
{
	if (m_Effect) {
		m_Effect->SetDuration (m_Effect->GetRagStateCnt()+300);
	}
}

void CWeather :: Reset( void )
{
	m_Effect = 0;
}

void CWeather :: AddScript( std::string map, std::string weather )
{
	WeatherInfo info;
	info.weather = weather;
	info.map = map;
	m_weatherInfo.push_back(info);
}

vector3d CWeather :: GetSunWorldLoc( void )
{
	vector3d  resultVec;
	float sinDeg = GetSin((m_nDayCount >> 3)+90);
	float cosDeg = GetCos((m_nDayCount >> 3)+90);
	vector3d	v = vector3d( 0.0f, 1000.0f, 0.0f );

	resultVec.z = 0.0f;
	resultVec.x = (v.x * sinDeg) + (v.y * cosDeg);
	resultVec.y = (v.x * cosDeg) - (v.y * sinDeg);

	return resultVec;
}

void CWeather :: ScriptProcess( std::string map )
{
#if !defined(INSTANT_EVENT_SERVER)
	if(g_session.m_isEffectOn)
	{
		for ( std::vector<WeatherInfo>::iterator it = m_weatherInfo.begin(); it != m_weatherInfo.end(); ++ it )
		{
			if ( it->map == map )
			{
				if ( it->weather == "snow" )
					LaunchSnow();
				else if ( it->weather == "sakura" )
					LaunchSakura();
				else if ( it->weather == "pokjuk" )
					LaunchPokJuk();
			}
		}
	}
#endif
}
