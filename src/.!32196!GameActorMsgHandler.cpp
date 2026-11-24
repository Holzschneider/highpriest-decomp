#include "common.h"
#include "GameMode.h"
#include "GameWorld.h"
#include "RagEffect.h"
#include "Session.h"
#include "packet.h"
#include "resource/Wave.h"
#include "pdb_enums.h"
#include "GameActor.h"
#include "GameActorMsgHandler.h"
#include "Mode.h"
#include "resource/Action.h"
#include "resource/Attr.h"
#include "resource/Ground.h"

extern CModeMgr g_modeMgr;
extern CSession g_session;
extern int doubleAttackTerm;



RandGenerator :: RandGenerator(int callNumber, int randSeed) : m_curCallNumber(0), m_totalRandNum(0)
{
	m_maxCallNumber = callNumber;
	m_randSeed = randSeed;
	if (m_randSeed<=0)
		m_randSeed = 1;
}

int RandGenerator::RandNum() { return 0; }

extern int MAX_ATTACKMT;

int MAX_ATTACKMT = 432;
void	CGameActor :: SetModifyFactorOfmotionSpeed(int attackMT)
{
	if (attackMT > MAX_ATTACKMT)
		attackMT = MAX_ATTACKMT;

	m_modifyFactorOfmotionSpeed = (float)attackMT / 432.0f;
	m_modifyFactorOfmotionSpeed2 = m_modifyFactorOfmotionSpeed;
}

void	CGameActor :: Am_Action(CGameObject *sender, int arg1, int arg2, int arg3)
{
	CGameMode *		gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);

	ACTSTRUCT *		as = (ACTSTRUCT *)arg1;
	int				action = as->action;
	m_targetGid = as->targetGID;
	int				damage = as->damage;
	int				leftDamage = as->leftDamage;
	int				count = as->count;
	int				tCellX = as->tCellX;
	int				tCellY = as->tCellY;
	int				attackMT = as->attackMT;
	int				attackedMT = as->attackedMT;

	vector3d		tPos = m_pos;
	DWORD			tGid = m_gid;
	int				tJob = JT_WARPNPC;
	CGameActor *	tActor = NULL;

	if (g_session.GetAid() == m_targetGid)
	{
		tActor = gameMode->GetWorld()->m_player;
	}
	else if(tActor = gameMode->GetWorld()->GetGameActorByAID(m_targetGid))
	{
		NULL;
	}
	else if(tActor = gameMode->GetWorld()->GetSkillByAID(m_targetGid))
	{
		NULL;
	}

	if(tActor)
	{
		tPos = tActor->m_pos;
		tGid = tActor->GetGID();
		tJob = tActor->GetJob();
	}
	else
	{
		tPos = m_pos - (gameMode->GetWorld()->m_player->m_pos - m_pos);
	}

	if (count <= 0)		count = 1;
	if (count > 36)		count = 1;

	if (action!=ACTION_ATTACK_MULTIPLE && action!=ACTION_ATTACK_MULTIPLE_NOMOTION)
		count = 1;

	damage /= count;

	switch (action)
	{
		case	ACTION_ATTACK:
		case	ACTION_ATTACK_MULTIPLE:
		case	ACTION_ATTACK_LUCKY:
		case	ACTION_ATTACK_CRITICAL:
		case	ACTION_ATTACK_NOMOTION:
		case	ACTION_ATTACK_MULTIPLE_NOMOTION:
		{
			if (m_isCounter)
			{
				CRagEffect *	AutoCounter = LaunchEffect(EF_AUTOCOUNTER);
				if (AutoCounter)
					AutoCounter->SendMsg(this, AM_SET_MASTER_POINTER);
				m_isCounter = FALSE;
			}

			SetModifyFactorOfmotionSpeed(attackMT);
			if (as->targetGID != m_gid)
				SetState(ST_ATTACK);

			CalcDir(tPos.x, tPos.z);

			DWORD	delayTime = m_attackMotion * m_motionSpeed * 24.0f;

			if (delayTime < 0)
				delayTime = 0;

			if (GetJob() == JT_ARCHER_SKELETON || GetJob() == JT_G_ARCHER_SKELETON)
				delayTime += 192;
			else if (GetJob() == JT_ARCHER_GUARDIAN || GetJob() == JT_BOW_GUARDIAN)
				delayTime += 912;
			else if (GetJob() == JT_KNIGHT_GUARDIAN || GetJob() == JT_SWORD_GUARDIAN)
				delayTime += 408;
			else if (GetJob() == JT_SOLDIER_GUARDIAN)
				delayTime += 408;


			int		actorMsg;
			int		atkEfId = -1;
			switch (action)
			{
				case	ACTION_ATTACK_NOMOTION:
				case	ACTION_ATTACK_MULTIPLE_NOMOTION:
					actorMsg = AM_ATTACKEDNOMOTION;
					break;
				case	ACTION_ATTACK_CRITICAL:
					actorMsg = AM_ATTACKED_CRITICAL;
					atkEfId = EF_HIT2;
					break;
				case	ACTION_ATTACK_LUCKY:
					actorMsg = AM_ATTACKED_LUCKY;
					if (m_isCounter)
						actorMsg = AM_ATTACKED;
					break;
				default:
					actorMsg = AM_ATTACKED;
					break;
			}

			if (tActor)
			{
				int		sActorCellX, sActorCellY, totalDamage = 0, atkEfId2 = EF_HIT1;

				if ( GetJob() >= JT_TAEKWON && GetJob() <= JT_STAR2 )
					atkEfId2 = EF_HITLINE7;

				gameMode->GetServerCoor(gameMode->GetWorld()->m_player->m_pos.x, gameMode->GetWorld()->m_player->m_pos.y, sActorCellX, sActorCellY);
				float	attackedSpeedFactor = float(attackedMT) / float(AVG_ATTACKED_SPEED);
				RandGenerator	rGen(count, damage/6);
				if (as->targetGID == m_gid)
					atkEfId2 = -2;

				WBA		wba(actorMsg, damage + rGen.RandNum(), delayTime + timeGetTime(), m_gid,
					tCellX, tCellY, atkEfId, atkEfId2, 1, attackedSpeedFactor );


				totalDamage = wba.damage;
				if (count == 1)
					tActor->SendMsg(this, AM_WILL_BE_ATTACKED, INT_PTR(&wba));
				else
					tActor->SendMsg(this, AM_WILL_BE_ATTACKED, INT_PTR(&wba), totalDamage);

				if ( (leftDamage&&GetJob() < JT_MON_BEGIN) || (leftDamage&&GetJob() > JT_2004_JOB_BEGIN) )
				{
					for (int i=1; i<count; ++i)
					{
						wba.time += doubleAttackTerm/2;
						wba.damage = damage+rGen.RandNum();
						totalDamage += wba.damage;
						tActor->SendMsg(this, AM_WILL_BE_ATTACKED, INT_PTR(&wba), totalDamage);
						wba.time -= doubleAttackTerm/2;
					}
					wba.message = actorMsg;
					wba.time += doubleAttackTerm + doubleAttackTerm/2 + doubleAttackTerm/4;
					wba.damage = leftDamage;
					totalDamage += wba.damage;
					if (count == 1)
						tActor->SendMsg(this, AM_WILL_BE_ATTACKED, INT_PTR(&wba));
					else
						tActor->SendMsg(this, AM_WILL_BE_ATTACKED, INT_PTR(&wba), totalDamage);
				}
				else
				{
					for (int i=1; i<count; ++i)
					{
						wba.time += doubleAttackTerm;
						wba.damage = damage + rGen.RandNum();
						totalDamage += wba.damage;
						tActor->SendMsg(this, AM_WILL_BE_ATTACKED, INT_PTR(&wba), totalDamage);
					}
				}
			}
			break;
		}
		case	ACTION_ITEMPICKUP:
			SetState(ST_PICKUP);
			break;
		case	ACTION_SIT:
			SetState(ST_SIT);
			break;
		case	ACTION_STAND:
			SetState(ST_STAND);
			break;
		default:
			sAssert(false);
			Trace("Unknown Action Type == %d", action);
			break;
	}
}

void	CGameActor :: Am_Skill(CGameObject *sender, int arg1, int arg2, int arg3)
{


	CGameMode *		gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);

	SKILLSTRUCT *	ss = (SKILLSTRUCT *)arg1;
	int				action = ss->action;
	int				damage = ss->damage;
	int				skillId = ss->skillId;
	int				level = ss->level;
	int				count = ss->count;
	int				tCellX = ss->tCellX;
	int				tCellY = ss->tCellY;
	int				attackMT = ss->attackMT;
	int				attackedMT = ss->attackedMT;





	if (count <= 0)		count = 1;
	if (count > 36)		count = 1;

	sAssert(skillId>=0 && skillId<SKID_LAST);
	if (skillId == SKID_KN_PIERCE)
	{
		level = count;
	}
	damage /= count;

	DWORD	startTime = ss->startTime;
	m_targetGid = ss->targetGID;

	vector3d		tPos	= m_pos;
	int				tJob	= JT_WARPNPC;
	CGameActor *	tActor	= NULL;

	if (g_session.GetAid() == m_targetGid)
	{
		tActor = gameMode->GetWorld()->m_player;
	}
	else if(tActor = gameMode->GetWorld()->GetGameActorByAID(m_targetGid))
	{
		if (skillId == SKID_HT_BLITZBEAT || skillId == SKID_SN_FALCONASSAULT)
		{
			if (m_birdEffect != NULL)
				m_birdEffect->SendMsg(NULL, AM_REQ_ATTACK, INT_PTR(&tActor->m_pos));
		}
	}
	else if(tActor = gameMode->GetWorld()->GetSkillByAID(m_targetGid))
	{
		NULL;
	}

	if(tActor)
	{
		tPos = tActor->m_pos;
		tJob = tActor->GetJob();
	}
	else
	{
		tPos = m_pos - (gameMode->GetWorld()->m_player->m_pos - m_pos);
	}

	DWORD	arrivalDelayTime = 0;

	switch (action)
	{
		case	ACTION_ATTACK_MULTIPLE:
		case	ACTION_ATTACK_MULTIPLE_NOMOTION:
			break;
		default:
			count = 1;
			break;
	}

	switch (action)
	{
		case	ACTION_SPLASH:
			if (tActor)
			{
				if (skillId == SKID_AC_SHOWER || skillId == SKID_MG_NAPALMBEAT || skillId == SKID_TK_STORMKICK || skillId == SKID_MO_COMBOFINISH )
				{

				}
				else
				{
					float	attackedSpeedFactor = float(attackedMT) / float(AVG_ATTACKED_SPEED);

					DWORD	delayTime = m_attackMotion * m_motionSpeed * 24.0f;
					WBA wba(AM_ATTACKED_SKILL, damage, delayTime + timeGetTime(), m_gid, tCellX, tCellY, -1, -1, level, attackedSpeedFactor);
					tActor->SendMsg(this, AM_WILL_BE_ATTACKED, INT_PTR(&wba));
					break;
				}
			}
		case	ACTION_SKILL:
		case	ACTION_ATTACK_NOMOTION:
		case	ACTION_ATTACK_MULTIPLE:
		case	ACTION_ATTACK_MULTIPLE_NOMOTION:
		{

			int		beginEffectId;
			int		skillMotionType;
			g_session.GetSkillActionInfo(skillId, beginEffectId, skillMotionType);

			SetModifyFactorOfmotionSpeed(attackMT);
#if defined(BALANCE_PATCH)
			if (skillId != SKID_NV_TRICKDEAD)
			{
#endif

#ifdef	SECONDJOB_SKILL2
				if (skillId != SKID_MO_TRIPLEATTACK && skillId != SKID_MO_CHAINCOMBO && skillId != SKID_MO_COMBOFINISH
					&& skillId != SKID_MO_EXTREMITYFIST && skillId != SKID_CH_TIGERFIST )
#endif
					SetState(skillMotionType);
#if defined(BALANCE_PATCH)
			}
#endif

			if (SKILL_INFO::IsSkillNotDelayed(skillId))
			{

				CRagEffect *	effect = LaunchEffect(beginEffectId);
				if (effect)
				{
					effect->SendMsg(NULL, AM_SETCOUNT, count);
				}
			}


			g_session.LaunchSkillSpecialEffect(skillId, count, this, tActor);

			CalcDir(tPos.x, tPos.z);
			DWORD	delayTime = m_attackMotion * m_motionSpeed * 24.0f;

			if (delayTime < 0)
				delayTime = 0;

			if (GetJob() == JT_ARCHER_SKELETON || GetJob() == JT_G_ARCHER_SKELETON)
				delayTime += 192;

			if (tActor)
			{
				int				effectId1 = g_session.GetAttackEffectIdBySkillId(skillId);
				int				effectId2 = g_session.GetAttackedEffectIdBySkillId(skillId);

				if (effectId2 == EF_COMBOATTACK1)
				{
					switch(level)
					{
						case	2:
							effectId2 = EF_COMBOATTACK2;
							break;
						case	3:
							effectId2 = EF_COMBOATTACK3;
							break;
						case	4:
							effectId2 = EF_COMBOATTACK4;
							break;
						case	5:
							effectId2 = EF_COMBOATTACK5;
							break;
					}
				}

				CRagEffect *	effect = LaunchEffect(effectId1);

				if (effect)
				{
					effect->SendMsg(NULL, AM_SETCOUNT, count);
					effect->SendMsg(NULL, AM_SETPOS, INT_PTR(&tPos));
				}

				int		atkedEfId = EF_HIT1;

				if (skillId >= SKID_TK_RUN && skillId <= SKID_SL_SKA)
					atkedEfId = -1;
				else if (skillId >= SKID_TK_MISSION )
					atkedEfId = -1;

				switch (skillId)
				{
					case	SKID_MG_FIREBOLT:
					case	SKID_MG_FIREBALL:
						atkedEfId = EF_FIREHIT;
						break;
					case	SKID_MG_COLDBOLT:
					case	SKID_MG_FROSTDIVER:
						atkedEfId = EF_COLDHIT;
						break;
					case	SKID_MG_LIGHTNINGBOLT:
						atkedEfId = EF_WINDHIT;
						break;
					case	SKID_SM_MAGNUM:
						atkedEfId = EF_FIREHIT;
						break;
					case	SKID_WZ_EARTHSPIKE:
					case	SKID_WZ_HEAVENDRIVE:
						atkedEfId = EF_EARTHHIT;
						break;
					case	SKID_AS_SONICBLOW:

						atkedEfId =		EF_HIT1;
						break;
					case	SKID_KN_PIERCE:
					case	SKID_KN_BRANDISHSPEAR:
					case	SKID_KN_SPEARSTAB:
					case	SKID_KN_SPEARBOOMERANG:
#ifdef	MOB_SKILL
					case	SKID_NPC_WATERATTACK:
					case	SKID_NPC_GROUNDATTACK:
					case	SKID_NPC_FIREATTACK:
					case	SKID_NPC_FIREBREATH:
					case	SKID_NPC_WINDATTACK:
					case	SKID_NPC_POISONATTACK:
					case	SKID_NPC_HOLYATTACK:
#if defined(MOB_SKILL_PATCH)
					case	SKID_NPC_UNDEADATTACK:
#endif
#endif
						atkedEfId = -1;
						break;
					case	SKID_ALL_RESURRECTION:
						if (damage) {
							atkedEfId = EF_HOLYHIT;
							effectId2 = -1;
						}
						break;
					case	SKID_HT_BLITZBEAT:
						atkedEfId = -1;
						break;
#ifdef	MOB_SKILL
					case	SKID_NPC_DARKNESSATTACK:
						atkedEfId = EF_HITDARK;
						break;
					case	SKID_NPC_CRITICALSLASH:
						action = ACTION_ATTACK_CRITICAL;
						break;
#endif
					case	SKID_LK_SPIRALPIERCE:
						atkedEfId = -1;
						break;
					case	SKID_SL_STIN:
					case	SKID_SL_STUN:
						atkedEfId = EF_BLUE_HIT;
						break;
				}

				int		actorMsg, totalDamage = 0;
				switch (action)
				{
					case	ACTION_SKILL:
						actorMsg = AM_ATTACKEDNOMOTION_SKILL;
						break;
					case	ACTION_ATTACK_NOMOTION:
					case	ACTION_ATTACK_MULTIPLE_NOMOTION:
						actorMsg = AM_ATTACKEDNOMOTION;
						break;
					case	ACTION_ATTACK_CRITICAL:
						actorMsg = AM_ATTACKED_CRITICAL;
						break;
					default	:
						actorMsg = AM_ATTACKED_SKILL;
						break;
				}

				float			attackedSpeedFactor = float(attackedMT) / float(AVG_ATTACKED_SPEED);

				RandGenerator	rGen(count, damage/6);

				WBA				wba(actorMsg, damage + rGen.RandNum(), delayTime + timeGetTime(), m_gid, tCellX, tCellY, atkedEfId, effectId2, level, attackedSpeedFactor);

				if (skillId == SKID_PR_TURNUNDEAD)
				{
					wba.time += 500;
				}

				totalDamage = wba.damage;

				switch (skillId)
				{
					case SKID_TK_DOWNKICK :
					case SKID_TK_TURNKICK :
						wba.time += 300;
						tActor->SendMsg(this, AM_WILL_BE_ATTACKED2, INT_PTR(&wba), totalDamage);
						break;
					case SKID_TK_STORMKICK :
					case SKID_TK_COUNTER :
					case SKID_TK_JUMPKICK :
					case SKID_SM_BASH :
					case SKID_KN_BOWLINGBASH :
						tActor->SendMsg(this, AM_WILL_BE_ATTACKED2, INT_PTR(&wba), totalDamage);
						break;
					case SKID_ASC_BREAKER :
					case SKID_MO_EXTREMITYFIST :
						tActor->SendMsg(this, AM_ATTACKED_CRITICAL, INT_PTR(&wba), totalDamage);
						break;
					case SKID_MO_COMBOFINISH :
						tActor->LaunchEffect(EF_HP_DAMAGE);
						tActor->SendMsg(this, AM_WILL_BE_ATTACKED, INT_PTR(&wba), totalDamage);
						break;
					case SKID_CH_TIGERFIST :
					case SKID_NPC_BLOODDRAIN :
						tActor->SendMsg(this, AM_WILL_BE_ATTACKED, INT_PTR(&wba), totalDamage);
						break;
					default:
						if (count == 1)
						{
							tActor->SendMsg(this, AM_WILL_BE_ATTACKED, INT_PTR(&wba));
						} else {
							tActor->SendMsg(this, AM_WILL_BE_ATTACKED, INT_PTR(&wba), totalDamage);
						}
						break;
				}

				int oldefId = wba.effectId2 = -1;

#ifdef	SECONDJOB_SKILL2
				switch (skillId)
				{
					case	SKID_MO_TRIPLEATTACK:
						SetState(ST_ATTACK);
						break;
					case	SKID_MO_CHAINCOMBO:
						if (GetSex() == 0)
						{
							SetForceAnimation(96, 5, 5, 96, 5, TRUE, g_session.GetForceAnimEndTick(GetGID()), FALSE);
						}
						else
						{
							SetForceAnimation(96, 5, 5, 96, 5, TRUE, g_session.GetForceAnimEndTick(GetGID()), FALSE);
						}
						tActor->LaunchEffect(EF_TEIHIT1);
						LaunchEffect(EF_GUMGANG3);
						tActor->LaunchEffect(EF_CHAINCOMBO);
						break;
					case	SKID_MO_COMBOFINISH:
						SetForceAnimSpeed(4);
						SetForceAnimation(80, 3, 4, 80, 4, TRUE, timeGetTime() + 100, FALSE);
						g_modeMgr.GetGameMode()->SendMsg(MM_QUAKE);
