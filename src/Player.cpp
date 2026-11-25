#include "common.h"
#include "GameActor.h"
#include "GameMode.h"
#include "GameWorld.h"
#include "resource/Attr.h"
#include "base/Util.h"
#include "device/Sound.h"
#include "device/Control.h"
#include "RagEffect.h"
#include "window/UIFrameWnd.h"
#include "MsgStrings.h"
#include "GuildInfo.h"
#include "resource/Wave.h"
#include "device/Connection.h"
#include <math.h>
#include "pdb_enums.h"
#include "Mode.h"
#include "RagEffectPrim.h"
#include "Session.h"
#include "window/UISys.h"
#include "resource/Ground.h"

extern CRenderer* g_renderer;
extern CModeMgr g_modeMgr;
extern CSession g_session;
extern UIWindowMgr g_windowMgr;
extern char g_FirstEffect;
extern char g_checkBOT;
extern int g_se_go;
extern bool IsEffectStateBurrow(unsigned int state);

#define RadToDeg(rad)  (float((rad) / pi) * 180.0f)

enum ATTACKMODE { ATTM_REPEAT, ATTM_REPEAT_TILL_BUTTONUP, ATTM_ONCE };


CPlayer::CPlayer()
{
	OnInit();
}


CPlayer::~CPlayer()
{
	OnExit();
}

void	CPlayer :: OnInit()
{
	g_modeMgr.GetGameMode()->GetClientCoor(g_session.m_posX, g_session.m_posY, m_pos.x, m_pos.z);
	m_pos.y = g_modeMgr.GetGameMode()->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z);
	g_modeMgr.GetGameMode()->GetServerCoor(m_pos.x, m_pos.z, m_destCellX, m_destCellZ);
	m_gSkillDx = 0;
	m_gSkillDy = 0;
	m_roty = DirToDeg(g_session.m_dir);
	m_gid = g_session.GetAid();
	m_gdid = g_guildInfo.m_gdid;
	m_emblemVersion = g_guildInfo.m_emblemVersion;
	m_pathStartCell = -1;
	m_preMoveStartTick = 0;
	m_preMoveOn = false;
	m_proceedType = PT_NOTHING;
	m_attackMode = ATTM_REPEAT;
	m_totalAttackReqCnt = 0;
	m_isAttackRequest = false;
	m_proceedTargetGid = 0;
	m_shouldAddPickInfo = true;
	m_skillId = 0;
	m_skillAttackRange = 1;
	m_skillUseLevel = 0;
	m_tickOfMoveForAttack = bf_clock_ticks()-1200;
	m_attackReqTime = bf_clock_ticks()-1200;
	m_lastTlvertX = g_renderer->m_halfWidth;
	m_lastTlvertY = g_renderer->m_halfHeight;
	m_isPreengageStateOfMove = FALSE;
	m_preengageXOfMove = -1;
	m_preengageYOfMove = -1;
	m_isWaitingMoveAck = FALSE;
	m_moveReqTick = bf_clock_ticks()-1200;
	SetState(ST_STAND);
	m_speed = g_session.GetSpeed();
	m_isPc = TRUE;

	SetSex(g_session.GetSex());

	int oldbodyState = m_bodyState;
	int oldeffectState = m_effectState;
	int oldhealthState = m_healthState;

	m_bodyState = g_session.m_bodyState;
	m_effectState = g_session.m_effectState;
	m_healthState = g_session.m_healthState;

	SetSprAct2(g_session.GetJob(), g_session.GetSex(), g_session.m_head, g_session.m_weapon,
				g_session.m_accessory, g_session.m_accessory2, g_session.m_accessory3, g_session.m_shield, g_session.m_headPalette, g_session.m_bodyPalette , g_session.m_effectState);

	m_bodyState = oldbodyState;
	m_effectState = oldeffectState;
	m_healthState = oldhealthState;

	SetAttrState(g_session.m_bodyState, g_session.m_effectState, g_session.m_healthState, false);
#if defined(BALANCE_PATCH)
	SendMsg(NULL, AM_DELETE_EFFECT, EF_STATUS_STATE, 0, 0);
	m_statusEffect = LaunchEffect(EF_STATUS_STATE);
	if (g_FirstEffect >= 2)
		ReLaunchBlurEffects();
#endif
















#ifdef _ARTICLE_BLOOD_DIS

#endif

}

void	CPlayer :: OnExit()
{
}

static char *effectStr[512] = {
	"EF_TEST",
};

bool	CPlayer :: OnProcess()
{
#ifdef LOAD_RES_THREAD
	LoadBackgroundUpdate() ;
#endif

	ProcessBodyState();
	ProcessEffectState();
	ProcessWillBeAttacked();
	ProcessChatBalloon();

	if (m_isSingleColor) {
		if (bf_clock_ticks() > m_singleColorStartTick + 100) {
			m_colorOfSingleColor = 0xffff0000;
			m_isSingleColor = false;
		}
	}

	if (m_sprArgb != 0xffffffff)
		ModifyArgb(8,8,8,8);
	CGameMode *		gameMode = g_modeMgr.GetGameMode();

	if (m_preMoveOn) {
		if (bf_clock_ticks() > m_preMoveStartTick + g_session.m_averagePingTime / 2) {
			MOVESTRUCT ms;
			gameMode->GetServerCoor(m_pos.x, m_pos.z, ms.sx, ms.sy, ms.cellX, ms.cellY);
			ms.startTime = g_session.GetServerTime();
			ms.dx = m_destCellX;
			ms.dy = m_destCellZ;
			Trace("PreMove");
			SendMsg(NULL, AM_MOVE, (INT_PTR)&ms, 0, 0);
		}
	}

	ProcessProceed();
	bool	result = ProcessState();
	ProcessSound();
#ifdef	SECONDJOB_SKILL2
	if (m_isPlayHitWave) {
		CGameMode * gameMode = g_modeMgr.GetGameMode();
		sAssert(gameMode);
		vector3d &pPos = gameMode->GetWorld()->m_player->m_pos;
		PlayWave(m_hitWaveName, m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
		m_isPlayHitWave = FALSE;
	}
#endif
	return	result;
}

void CPlayer::ProcessPreengageMove()
{
	if (m_isPreengageStateOfMove && (ST_ATTACK==m_stateId || ST_PICKUP==m_stateId || ST_SIT == m_stateId ))
	{
		int		dx = m_preengageXOfMove;
		int		dy = m_preengageYOfMove;

		m_isPreengageStateOfMove = FALSE;
		m_preengageXOfMove = -1;
		m_preengageYOfMove = -1;

		SendMsg(NULL, AM_REQ_MOVE_PRIMITIVE, dx, dy, 0);
	}
}


void	CPlayer :: ProcessProceed()
{
	CGameMode *		gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);
	switch (m_proceedType)
	{
		case	PT_ATTACK:
		case	PT_ATTACK_2:
			{
			CGameActor *	theActor = gameMode->GetWorld()->GetGameActorByAID(m_proceedTargetGid);








			if (theActor)
			{
				if (theActor->m_stateId != ST_DEAD)
				{
					int		sx, sy;
					gameMode->GetServerCoor(m_pos.x, m_pos.z, sx, sy);
					bool	properState;
						if (GetJob() == JT_SAGE || GetJob() == JT_SAGE_H || GetJob() == JT_SAGE_B) {
							properState = (m_stateId==1 || m_stateId==0 || m_stateId==4 ||
								m_stateId==7 || m_stateId==8);
						}
						else if ( GetJob() >= JT_TAEKWON && GetJob() < JT_2004_JOB_LAST ||
							GetJob() == JT_GUNSLINGER || GetJob() == JT_NINJA )
						{
							properState = (m_stateId==1 || m_stateId==0 || m_stateId==4 ||
								m_stateId==7 || m_stateId==8 || m_stateId>=16 );

						}
						else
						{
							properState = (m_stateId==1 || m_stateId==0 || m_stateId==4);
						}

						if (properState)
						{
							float	targetDist = theActor->CalcDist(m_pos.x, m_pos.y, m_pos.z);
							if (theActor->GetActorType() == 1) {
								targetDist -= 20.0f;
								if (targetDist < 0.0f) targetDist = 0.0f;
							}

							bool	closeEnough = targetDist <= (g_session.m_attackRange*5 + 5.5f);
							bool	atkMoveArrived = (m_destCellX==sx && m_destCellZ==sy);

						if (atkMoveArrived || closeEnough) {
							if (ATTM_ONCE==m_attackMode)
								SendMsg(NULL, AM_CANCELATTACK, 0, 0, 0);
							if (m_isAttackRequest==0) {
								if (m_attackReqTime+1200 < bf_clock_ticks()) {
									m_attackReqTime = bf_clock_ticks();
									m_isAttackRequest = 1;
									m_totalAttackReqCnt++;

									if (ATTM_ONCE == m_attackMode)
										SendMsg(NULL, AM_REQ_ATTACK, 0, 0, 0);
									else
										SendMsg(NULL, AM_REQ_ATTACK_REPEAT, 0, 0, 0);
								}
							}
						} else {
								int		delayTime = 450;
								if (PT_ATTACK_2 == m_proceedType) delayTime = 1200;
								bool	enoughDelayed = bf_clock_ticks() > m_tickOfMoveForAttack+delayTime;
								bool	notAtkState = m_stateId != ST_ATTACK;

							int		dx, dy;
							dx = theActor->m_moveDestX;
							dy = theActor->m_moveDestY;

								float	tActorPosX;
							float	tActorPosZ;

							if (PT_ATTACK_2 == m_proceedType) {
								tActorPosX = theActor->m_pos.x;
								tActorPosZ = theActor->m_pos.z;
							} else {
								gameMode->GetClientCoor(dx, dy, tActorPosX, tActorPosZ);
							}
								if (theActor->GetActorType() == 1)
								{
									float	fDeg = RadToDeg(atan2(tActorPosX - m_pos.x, -(tActorPosZ - m_pos.z)));

									if (fDeg >= 360.0f) fDeg -= 360.0f;
									if (fDeg < 0.0f) fDeg += 360.0f;
									if (fDeg >= 0.0f && fDeg < 25.0f) dy += 5;
									else
									if (fDeg >= 25.0f && fDeg < 70.0f) { dx += 5; dy += 5; }
									else
									if (fDeg >= 70.0f && fDeg < 115.0f) dx += 5;
									else
									if (fDeg >= 115.0f && fDeg < 160.0f) { dx += 5; dy -= 5; }
									else
									if (fDeg >= 160.0f && fDeg < 205.0f) dy -= 5;
									else
									if (fDeg >= 205.0f && fDeg < 250.0f) { dx -= 5; dy += 5; }
									else
									if (fDeg >= 250.0f && fDeg < 295.0f) dx -= 5;
									else
									if (fDeg >= 295.0f && fDeg < 340.0f) { dx -= 5; dy -= 5; }
									else dy += 5;
								}

								gameMode->ConvertToNumCellCloseAndFindNoRedCell(m_pos.x, m_pos.z, tActorPosX, tActorPosZ, dx, dy, g_session.m_attackRange);

								if ((m_destCellX!=dx || m_destCellZ!=dy) && notAtkState && m_isAttackRequest==0 && enoughDelayed) {
								m_destCellX = dx;
								m_destCellZ = dy;
								m_tickOfMoveForAttack = bf_clock_ticks();

								SendMsg(NULL, AM_REQ_MOVE_PRIMITIVE, dx, dy, 0);

							}
						}
					}
				}
				} else {
						SendMsg(NULL, AM_CANCELATTACK, 0, 0, 0);
					}
					break;
				}

			case	PT_SKILL:
					{
					CGameActor *	theActor = NULL;


				if ( m_proceedTargetGid == g_session.GetAid() )
					theActor = this;
				else
					theActor = gameMode->GetWorld()->GetGameActorByAID(m_proceedTargetGid);


			if (theActor)
			{
				if (true)
				{
					int		sx, sy;
					gameMode->GetServerCoor(m_pos.x, m_pos.z, sx, sy);

					int		dx, dy;
					dx = theActor->m_moveDestX;
					dy = theActor->m_moveDestY;

					int		dx2 = dx;
					int		dy2 = dy;

					float tActorPosX;
					float tActorPosZ;

						tActorPosX = theActor->m_pos.x;
						tActorPosZ = theActor->m_pos.z;

							gameMode->ConvertToNumCellCloseAndFindNoRedCell(m_pos.x, m_pos.z, tActorPosX, tActorPosZ, dx, dy, m_skillAttackRange );

							if ((m_stateId==1 || m_stateId==0 || m_stateId==4 || m_stateId>=16))
							{
								float	targetDist = theActor->CalcDist(m_pos.x, m_pos.y, m_pos.z);

								if (theActor->GetActorType() == 1) {
									targetDist -= 20.0f;
									if (targetDist < 0.0f) targetDist = 0.0f;
								}
								bool	closeEnough = targetDist <= (m_skillAttackRange*5 + 5.5f);
								bool	atkMoveArrived = (m_destCellX==sx && m_destCellZ==sy);

							if (closeEnough || atkMoveArrived ) {
								SendMsg(NULL, AM_REQ_SKILL, 0, 0, 0);
								SendMsg(NULL, AM_CANCELATTACK, 0, 0, 0);
							} else if ((m_destCellX!=dx || m_destCellZ!=dy) && m_stateId != ST_ATTACK) {
								m_destCellX = dx;
								m_destCellZ = dy;

								if (theActor->GetActorType() == 1)
								{
									float	fDeg = RadToDeg(atan2(tActorPosX - m_pos.x, -(tActorPosZ - m_pos.z)));

									if (fDeg >= 360.0f) fDeg -= 360.0f;
									if (fDeg < 0.0f) fDeg += 360.0f;
									if (fDeg >= 0.0f && fDeg < 25.0f) dy += 5;
									else
									if (fDeg >= 25.0f && fDeg < 70.0f) { dx += 5; dy += 5; }
									else
									if (fDeg >= 70.0f && fDeg < 115.0f) dx += 5;
									else
									if (fDeg >= 115.0f && fDeg < 160.0f) { dx += 5; dy -= 5; }
									else
									if (fDeg >= 160.0f && fDeg < 205.0f) dy -= 5;
									else
									if (fDeg >= 205.0f && fDeg < 250.0f) { dx -= 5; dy += 5; }
									else
									if (fDeg >= 250.0f && fDeg < 295.0f) dx -= 5;
									else
									if (fDeg >= 295.0f && fDeg < 340.0f) { dx -= 5; dy -= 5; }
									else dy += 5;
								}


								SendMsg(NULL, AM_REQ_MOVE_PRIMITIVE, dx, dy, 0);
							}
						}
				}
			}
			else
			{
				SendMsg(NULL, AM_CANCELATTACK, 0, 0, 0);
			}
			break;
		}
		case	PT_GROUNDSKILL: {
			int		dx = m_gSkillDx, dy = m_gSkillDy;
			int		sx, sy;
			gameMode->GetServerCoor(m_pos.x, m_pos.z, sx, sy);

			int		dx2 = dx;
			int		dy2 = dy;

			float	fdx, fdz;
			gameMode->GetClientCoor(dx, dy, fdx, fdz);
			gameMode->ConvertToNumCellCloseAndFindNoRedCell(m_pos.x, m_pos.z, fdx, fdz, dx, dy, m_skillAttackRange);

					if ((m_stateId==1 || m_stateId==0 || m_stateId==4 || m_stateId>=16)) {
						float	targetDist = CalcDist(fdx, m_pos.y, fdz);
						bool	closeEnough = targetDist <= (m_skillAttackRange*5 + 5.5f);
				bool	atkMoveArrived = (m_destCellX==sx && m_destCellZ==sy);

				if (closeEnough || atkMoveArrived) {
					SendMsg(NULL, AM_REQ_GROUND_SKILL, 0, 0, 0);
					SendMsg(NULL, AM_CANCELATTACK, 0, 0, 0);
				} else if ((m_destCellX!=dx || m_destCellZ!=dy) && m_stateId != ST_ATTACK) {
					m_destCellX = dx;
					m_destCellZ = dy;

					SendMsg(NULL, AM_REQ_MOVE_PRIMITIVE, dx, dy, 0);
				}
			}
			break;
		}
		case	PT_PICKUPITEM: {
			CItem *		theItem = gameMode->GetWorld()->GetItemByAID(m_proceedTargetGid);
			if (theItem) {
				int		dx, dy;
				int		sx, sy;
				gameMode->GetServerCoor(m_pos.x, m_pos.z, sx, sy);
				gameMode->GetServerCoor(theItem->m_pos.x, theItem->m_pos.z, dx, dy);

				int		dx2 = dx;
				int		dy2 = dy;

				gameMode->ConvertToNoRedCell(sx, sy, dx, dy);
					if (m_stateId == 0 || m_stateId>=16)
					{
					if (gameMode->IsInsideCell(sx, sy, dx2, dy2, 1)) {
						SendMsg(NULL, AM_REQ_CHANGE_DIR, dx2, dy2, 0);
						if (!gameMode->IsReqPickupItemNaid(m_proceedTargetGid)) {
							gameMode->AddPickupItemNaid(m_proceedTargetGid);
							gameMode->SendMsg(MM_ITEMPICKUP, m_proceedTargetGid);
						}
						SendMsg(NULL, AM_CANCELATTACK, 0, 0, 0);
					} else if (m_destCellX!=dx || m_destCellZ!=dy) {
						m_destCellX = dx;
						m_destCellZ = dy;
						SendMsg(NULL, AM_REQ_MOVE_PRIMITIVE, dx, dy, 0);
					}
				}
			} else
				SendMsg(NULL, AM_CANCELATTACK, 0, 0, 0);
			break;
		}
		case	PT_TOUCH_SKILL: {
			CSkill *	theSkill = gameMode->GetWorld()->GetSkillByAID(m_proceedTargetGid);
			if (theSkill) {
				int		dx, dy;
				int		sx, sy;
				gameMode->GetServerCoor(m_pos.x, m_pos.z, sx, sy);
				gameMode->GetServerCoor(theSkill->m_pos.x, theSkill->m_pos.z, dx, dy);

				int		dx2 = dx;
				int		dy2 = dy;

				float	fdx, fdz;
				gameMode->GetClientCoor(dx, dy, fdx, fdz);
				gameMode->ConvertToNumCellCloseAndFindNoRedCell(m_pos.x, m_pos.z, fdx, fdz, dx, dy, m_skillAttackRange);

					if ((m_stateId==1 || m_stateId==0 || m_stateId==4 || m_stateId>=16)) {
					float	targetDist = CalcDist(fdx, m_pos.y, fdz);
					bool	closeEnough = targetDist <= (m_skillAttackRange*5 + 5.5f);
					bool	atkMoveArrived = (m_destCellX==sx && m_destCellZ==sy);
					if (closeEnough || atkMoveArrived) {
						switch (theSkill->GetJob()) {
							case	JT_ICEWALL:
							case	JT_TRAP_BLASTMINE:
							case	JT_TRAP_CLAYMORETRAP:
								if (m_skillId) {
									SendMsg(NULL, AM_REQ_SKILL, 0, 0, 0);
									SendMsg(NULL, AM_CANCELATTACK, 0, 0, 0);
								} else {
									SendMsg(NULL, AM_REQ_ATTACK, 0, 0, 0);
								}
								break;
							default:
							SendMsg(NULL, AM_REQ_SKILL, 0, 0, 0);
							SendMsg(NULL, AM_CANCELATTACK, 0, 0, 0);
								break;
						}
					} else if ((m_destCellX!=dx || m_destCellZ!=dy) && (m_stateId!=ST_ATTACK || theSkill->GetJob()==JT_ICEWALL || theSkill->GetJob()==JT_TRAP_BLASTMINE || theSkill->GetJob()==JT_TRAP_CLAYMORETRAP)) {
						m_destCellX = dx;
						m_destCellZ = dy;
						SendMsg(NULL, AM_REQ_MOVE_PRIMITIVE, dx, dy, 0);
					}
				}
			} else {
				SendMsg(NULL, AM_CANCELATTACK, 0, 0, 0);
			}
		}
		break;
	}

	}

bool CPlayer::ProcessState()
{
	switch (m_stateId) {
		default:
			return	CPc::ProcessState();
	}
	return true;
}

void	CPlayer :: SetState(int stateId)
{
	if (ST_DEAD == m_stateId) return;

	if (stateId != ST_MOVE) {
		m_destCellX = -1;
		m_destCellZ = -1;
	}

	ProcessPreengageMove();

	if(stateId == ST_DEAD) {
		CGameMode* gameMode = g_modeMgr.GetGameMode();
		gameMode->SendMsg(MM_HIDEMOUSEPOINTER);
		gameMode->PlaceEffect(&m_pos, EF_DEVIL, 5*60);
		g_se_go = 0;
	}

	CPc::SetState(stateId);
}

void	CPlayer :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	CGameMode *		gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);
	switch (message) {
		case	AM_REQ_GROUND_SKILL: {
			int		values[4];
			values[0] = m_skillId;
			values[1] = m_gSkillDx;
			values[2] = m_gSkillDy;
			values[3] = m_skillUseLevel;

			if(m_skillId == SKID_HT_TALKIEBOX) {
				UIWindow* wnd = g_windowMgr.MakeWindow(WID_TALKBOXTRAPINPUTWND);
				wnd->SendMsg(NULL, UM_SET_SKILL_INFO, INT_PTR(values), 0, 0, 0);
			}
			else
#ifdef		SECONDJOB_SKILL2
				if(m_skillId == SKID_RG_GRAFFITI) {
					UIWindow* wnd = g_windowMgr.MakeWindow(WID_GRAFFISTRBOXWND);
					wnd->SendMsg(NULL, UM_SET_SKILL_INFO, INT_PTR(values), 0, 0, 0);
				}
				else
#endif
			{
				gameMode->SendMsg(MM_USE_SKILL_TOGROUND, INT_PTR(values));
			}
			break;
			}
		case	AM_REQ_SKILL:
			{





				gameMode->SendMsg(MM_USE_SKILL, m_skillId, m_proceedTargetGid, m_skillUseLevel);
			}
			break ;
		case	AM_REQ_ATTACK_REPEAT: {
			switch (arg2)
			{
			case	JT_ICEWALL:
			case	JT_TRAP_BLASTMINE:
			case	JT_TRAP_CLAYMORETRAP:
				gameMode->SendMsg(MM_REQUEST_ACT, ACTION_ATTACK_REPEAT, arg1);
				break;
			default:
				CGameActor *actor = NULL;
				actor = gameMode->GetWorld()->GetGameActorByAID(m_proceedTargetGid);
				if (actor) {
					if (actor->GetHeadType() == HEADTYPE_LAST)
						return;
				}
				gameMode->SendMsg(MM_REQUEST_ACT, ACTION_ATTACK_REPEAT, m_proceedTargetGid);
				break;
			}
			break;
		}
		case	AM_REQ_ATTACK: {
			switch (arg2) {
				case	JT_ICEWALL:
				case	JT_TRAP_BLASTMINE:
				case	JT_TRAP_CLAYMORETRAP:
					gameMode->SendMsg(MM_REQUEST_ACT, ACTION_ATTACK, arg1);
					break;
				default:
					gameMode->SendMsg(MM_REQUEST_ACT, ACTION_ATTACK, m_proceedTargetGid);
					break;
			}
			break;
		}
		case	AM_REQ_STAND:
		case	AM_REQ_SIT: {
			if(IsEffectStateBurrow(m_effectState)) break;

			if(m_stateId == ST_STAND)
				gameMode->SendMsg(MM_REQUEST_ACT, ACTION_SIT, m_proceedTargetGid);
			else if(m_stateId == 6)
				gameMode->SendMsg(MM_REQUEST_ACT, ACTION_STAND, m_proceedTargetGid);
			break;
		}
		case	AM_REQ_MOVE:
			{


			int		dx = arg1, dy = arg2;

#ifdef WAIT_DAMAGED_MOVE
			if ( this->m_stateId == ST_DAMAGE )
			{
				m_waitDamagedMoveX = dx ;
				m_waitDamagedMoveY = dy ;
				return ;
			}
#endif

			SendMsg(NULL, AM_CANCELATTACK, 0, 0, 0);
			if (m_stateId==2 || m_stateId==5 || m_stateId==6)
			{
				m_isPreengageStateOfMove = TRUE;
				m_preengageXOfMove = dx;
				m_preengageYOfMove = dy;
				break;
			}
			else
			{
				m_isPreengageStateOfMove = FALSE;
				m_preengageXOfMove = -1;
				m_preengageYOfMove = -1;
			}


				SendMsg(NULL, AM_REQ_MOVE_PRIMITIVE, dx, dy, 0);
				break;
		}
		case	AM_REQ_MOVE_PRIMITIVE:
		{
			g_checkBOT = 0;






			if (m_isWaitingMoveAck)
			{
				if (bf_clock_ticks() <= m_moveReqTick + 1000)
				{
					break;
				}
			}

			{
				m_isWaitingMoveAck = TRUE;
				m_moveReqTick = bf_clock_ticks();





					gameMode->SendMsg(MM_REQUEST_MOVE, arg1, arg2);




			}

			{


			}
			break;
		}
		case	AM_REQ_CHANGE_DIR: {
			if (m_stateId!=0 && m_stateId!=4 && m_stateId!=6)
				return;
			int		dx = arg1;
			int		dy = arg2;
			float	cx, cy;
			gameMode->GetClientCoor(dx, dy, cx, cy);


			float	nRoty = CalcDir(m_pos.x, m_pos.z, cx, cy);

			int		rotDiff = int(m_roty - nRoty);
			if (rotDiff<0) rotDiff+=360;
			rotDiff = (rotDiff+22.5) / 45;
			if (rotDiff < 0)
				rotDiff += 8;
			if (rotDiff >= 8)
				rotDiff -= 8;



			if (rotDiff == 0) {

				m_headDir = HEADERDIR_DEFAULT;
				m_roty = nRoty;
			}

			if (rotDiff == 4) {

				m_headDir = HEADERDIR_DEFAULT;
				m_roty = nRoty;
			}


			if (rotDiff == 1) {
				if (m_headDir != HEADERDIR_RIGHT)
					m_headDir = HEADERDIR_RIGHT;
				else {
					m_headDir = HEADERDIR_DEFAULT;
					m_roty = nRoty;
				}
			}
			if (rotDiff==2 || rotDiff==3) {

				m_headDir = HEADERDIR_RIGHT;
				m_roty = nRoty + 45;
				if (m_roty >= 360)
					m_roty -= 360;
			}
			if (rotDiff == 7) {
				if (m_headDir != HEADERDIR_LEFT)
					m_headDir = HEADERDIR_LEFT;
				else {
					m_headDir = HEADERDIR_DEFAULT;
					m_roty = nRoty;
				}
			}
			if (rotDiff == 5 || rotDiff == 6) {


				m_headDir = HEADERDIR_LEFT;
				m_roty = nRoty - 45;
				if (m_roty < 0)
					m_roty += 360;
			}
			m_curMotion = m_headDir;

			gameMode->SendMsg(MM_CHANGE_DIRECTION, DegToDir(m_roty), m_headDir);
			break;
		}
		case	AM_PREMOVE:
			m_preMoveOn = true;
			m_preMoveStartTick = bf_clock_ticks();
			m_destCellX = arg1;
			m_destCellZ = arg2;
			break;
		case	AM_MOVE:
			{
				m_isWaitingMoveAck = FALSE;
				bool		result;
				m_preMoveOn = false;
				MOVESTRUCT *	ms = (MOVESTRUCT *)arg1;
				m_moveStartTime = ms->startTime;

				int speed = g_session.GetSpeed() ;




				result = FindPath(ms->sx, ms->sy, ms->cellX, ms->cellY, ms->dx, ms->dy, speed );

				if (result)
				{


					m_moveDestX = ms->dx;
					m_moveDestY = ms->dy;
					SetState(ST_MOVE);
					m_pathStartCell = 0;
				}
				else
				{
					int sx, sy, cellX, cellY ;
					gameMode->GetServerCoor(m_pos.x, m_pos.z, sx, sy, cellX, cellY);
					bool	result = FindPath(sx, sy, cellX, cellY, ms->dx, ms->dy, g_session.GetSpeed());
					if (result)
					{
						m_moveDestX = ms->dx;
						m_moveDestY = ms->dy;
						SetState(ST_MOVE);
					}
				}



			}
			break ;
		case	AM_FREEZE:
			SetState(ST_STAND);
			break;
		case	AM_CANCELATTACK:
			m_isAttackRequest = 0;
			m_proceedType = PT_NOTHING;
			gameMode->SendMsg(MM_SETLOCKONMOUSE, 0);
			break;
		case	AM_ATTACKCLICKUP: {
			if (m_proceedType==PT_ATTACK || m_proceedType==PT_ATTACK_2) {
				if (m_attackMode) {
					m_attackMode = ATTM_ONCE;
					if (m_totalAttackReqCnt) {
							SendMsg(NULL, AM_CANCELATTACK, 0, 0, 0);
						gameMode->SendMsg(MM_CANCEL_LOCKON);
					}
				}
			} else if (m_proceedType == PT_SKILL) {
				gameMode->SendMsg(MM_SETLOCKONMOUSE, 0);
			}
			break;
		}
		case	AM_SET_SKILL_USE_LEVEL: {
			m_skillUseLevel = arg1;
			break;
		}
		case	AM_PROCEEDGROUNDSKILL: {

			if (m_stateId==6)
				break;
			if (IsEffectStateBurrow(m_effectState) && arg1!=SKID_AS_GRIMTOOTH && arg1!=SKID_RG_BACKSTAP
				&& arg1!=SKID_NJ_SHADOWJUMP && arg1!=SKID_NJ_KIRIKAGE) break;

			m_proceedType = PT_GROUNDSKILL;
			m_skillId = arg1;
			m_gSkillDx = arg2;
			m_gSkillDy = arg3;
			m_skillAttackRange = gameMode->GetSkillAttackRange();
			m_proceedTargetGid = 0;

			break;
		}

		case	AM_PROCEEDSKILL:
			{
			Trace("AM_PROCEEDSKILL");

			if (m_stateId == 6)
				break;

			if (IsEffectStateBurrow(m_effectState) && arg2!=SKID_AS_GRIMTOOTH && arg2!=SKID_RG_BACKSTAP
				&& arg2!=SKID_NJ_SHADOWJUMP && arg2!=SKID_NJ_KIRIKAGE && arg2!=SKID_DE_SLASH) break;

			m_proceedType = PT_SKILL;
			m_proceedTargetGid = arg1;
			m_skillId = arg2;
			m_skillAttackRange = arg3;
			break;
		}
		case	AM_RESUMEPROCEEDATTACK: {
			if (m_stateId==6 || IsEffectStateBurrow(m_effectState))
				break;
			if (m_proceedType==PT_ATTACK) {
				m_tickOfMoveForAttack = bf_clock_ticks()-1200;
				m_proceedType = PT_ATTACK_2;
			} else {
				m_tickOfMoveForAttack = bf_clock_ticks()-1200;
				m_proceedType = PT_ATTACK;
			}

			m_proceedTargetGid = arg1;
			m_isAttackRequest = 0;
			m_totalAttackReqCnt = 0;
			m_destCellX = -1;
			m_destCellZ = -1;
			break;
		}
		case	AM_PROCEEDATTACK: {
			if (m_stateId==6 || IsEffectStateBurrow(m_effectState))
				break;
			CGameActor *	theActor = gameMode->GetWorld()->GetGameActorByAID(arg1);
			if (theActor==NULL)
				return;
			if (theActor->m_speed > g_session.GetSpeed()*4/3)
				m_proceedType = PT_ATTACK_2;
			else
				m_proceedType = PT_ATTACK;
			m_proceedTargetGid = arg1;
			m_isAttackRequest = 0;
			m_attackMode = arg2;
			m_totalAttackReqCnt = 0;
			break;
		}
		case	AM_PROCEEDPICKUPITEM: {
			if (m_stateId==6 || IsEffectStateBurrow(m_effectState))
				break;
			m_proceedType = PT_PICKUPITEM;
			m_proceedTargetGid = arg1;
			break;
		}

		case	AM_PROCEEDTOUCHSKILL: {
			if (m_stateId==6 || IsEffectStateBurrow(m_effectState))
				break;
			m_proceedType = PT_TOUCH_SKILL;
			SKILL_USE_INFO * skillUseInfo = (SKILL_USE_INFO *)arg2;

			m_skillId = skillUseInfo->m_skillId;
			m_skillUseLevel = skillUseInfo->m_useLevel;
			m_skillAttackRange = skillUseInfo->m_attackRange;
			m_proceedTargetGid = arg1;
			break;
		}
		default:
			CPc::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

void	CPlayer :: ReLaunchBlurEffects()
{
	int a, id;
	g_Weather.StopNight();

	for(a = 0; a < stateId.size(); a++)
	{
		id = stateId[a];
		if (id == EFST_TWOHANDQUICKEN)
		{
			CRagEffect *effect = LaunchEffect(EF_MAKEBLUR);
			if (effect)
				effect->SendMsg(NULL, AM_SETCOUNT, 65535, EF_TWOHANDQUICKEN);
		}
		else if (id == EFST_ONEHANDQUICKEN)
		{
			CRagEffect *effect = LaunchEffect(EF_MAKEBLUR);
			if (effect)
				effect->SendMsg(NULL, AM_SETCOUNT, 65535, EF_TWOHANDQUICKEN);
		}
		else if (id == EFST_NJ_BUNSINJYUTSU)
		{
			CRagEffect *effect = LaunchEffect(EF_BUNSINJYUTSU);
			if (effect)
				effect->SendMsg(NULL, AM_SETCOUNT, 65535, EF_BUNSINJYUTSU);
		}
		else if (id == EFST_OVERTHRUST)
		{
			CRagEffect *effect = LaunchEffect(EF_MAKEBLUR);
			if (effect)
				effect->SendMsg(NULL, AM_SETCOUNT, 65535, EF_OVERTHRUST);
		}
		else if (id == EFST_OVERTHRUSTMAX)
		{
			CRagEffect *effect = LaunchEffect(EF_MAKEBLUR);
			if (effect)
				effect->SendMsg(NULL, AM_SETCOUNT, 65535, EF_OVERTHRUST);
		}
		else if (id == EFST_ENERGYCOAT) {
			CRagEffect *effect = LaunchEffect(EF_MAKEBLUR);
			if (effect)
				effect->SendMsg(NULL, AM_SETCOUNT, 65535, EF_ENERGYCOAT);
		} else if (id == EFST_SPEARQUICKEN) {
			CRagEffect *effect = LaunchEffect(EF_MAKEBLUR);
			if (effect)
				effect->SendMsg(NULL, AM_SETCOUNT, 65535, EF_SPEARQUICKEN);
		} else if (id == EFST_EXPLOSIONSPIRITS)
		{
			LaunchEffect(EF_GUMGANG);
		}
		else if (id == EFST_STEELBODY)
		{
			LaunchEffect(EF_STEELBODY);
		}
		else if (id == EFST_SG_SUN_WARM || id == EFST_SG_MOON_WARM || id == EFST_SG_STAR_WARM )
		{
			LaunchEffect(EF_DOUBLEGUMGANG);
			LaunchEffect(EF_REDLIGHTBODY);
		}
		else if (id == EFST_KAITE)
		{
			LaunchEffect(EF_REFLECTBODY);
		}
		else if (id == EFST_AURABLADE)
		{
			LaunchEffect(EF_AURABLADE2);
		}
		else if (id == EFST_BERSERK)
		{
			LaunchEffect(EF_REDBODY);
		}
		else if (id == EFST_LKCONCENTRATION)
		{
			LaunchEffect(EF_LKCONCENTRATION);
		}
		else if (id == EFST_ASSUMPTIO)
		{
			LaunchEffect(EF_ASSUMPTIO);
		}
		else if (id == EFST_MAGICPOWER)
		{
			LaunchEffect(EF_LIGHTBLADE);
		}
		else if (id == EFST_MOON)
		{
			LaunchEffect(EF_SPHEREWIND2);
		}
		else if ( id == EFST_DEVIL1 )
		{
			SKILL_INFO skillInfo = g_session.GetSkillItemInfoBySkillId(SKID_SG_DEVIL);
			if ( skillInfo.level >= 1 && skillInfo.level <= 10 )
			{
				if (!IsEffectExist(EF_DEVIL1+(skillInfo.level-1)))
				{
					if (strcmp("\xa1\xba\xcf\xeb\xe1\xa1\xe1\xb4\xd2\xb3\xa1\xbb", g_session.GetCharName()) != 0)
					{
						LaunchEffect(EF_DEVIL1+(skillInfo.level-1));
						LaunchEffect(0x1e7);
					}
				}
			}
		}
		else if ( id == EFST_SOULLINK )
		{
			g_Weather.LaunchNight();
			LaunchEffect(EF_ASURABODY);
		}
		else if ( id == EFST_PROPERTYUNDEAD )
		{
			LaunchEffect(EF_UNDEADBODY);
		}
		else if ( id == EFST_SKE )
		{
			g_Weather.LaunchNight();
		}
		else if (id == EFST_DAURA )
		{
			LaunchEffect(EF_DOUBLEGUMGANG);
			LaunchEffect(EF_REDLIGHTBODY);
		}
		else if (id == EFST_DBARRIER)
		{
			LaunchEffect(EF_STEELBODY);
		}
		else if ( id == EFST_DA_CONTRACT )
		{
			LaunchEffect(EF_M04);
		}
		else if ( id == EFST_CRYSTAL )
		{
			LaunchEffect(EF_CRYSTAL_BLUE);
		}
	}

	if ( g_session.GetLevel()==99 )
	{
		SendMsg(NULL, AM_99, 0, 0, 0);
	}
}
