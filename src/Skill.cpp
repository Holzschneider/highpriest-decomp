#include "common.h"
#include "packet.h"
#include "Session.h"
#include "RagEffect.h"
#include "GameMode.h"
#include "GameActor.h"
#include "GameWorld.h"
#include "resource/Wave.h"
#include "resource/World.h"
#include "3dActor.h"
#include "Picker.h"
#include "pdb_enums.h"
#include "3dGround.h"
#include "Mode.h"
#include "3dlib/Texture.h"
#include "resource/Action.h"
#include "resource/Attr.h"
#include "resource/Model.h"
#include "window/UISys.h"
#include "resource/Ground.h"

extern CModeMgr g_modeMgr;
extern CRenderer* g_renderer;
extern float g_avgPixelRatio;
extern CSession g_session;
extern int doubleAttackTerm;






inline float CRenderObject::GetStateCnt() { return float(bf_clock_ticks() - m_stateStartTick) / 24.0f; }


CSkill :: CSkill()
{
	OnInit();
}

CSkill :: ~CSkill()
{
	OnExit();
}

void	CSkill :: OnInit()
{
	m_aid = 0;
	SetJob(JT_SAFETYWALL);
	m_launchCnt = 60 * 60 * 10;
	m_shouldAddPickInfo = false;
	m_isVisibleBody = false;
	m_balloon = NULL;
	m_3dactor = NULL;
	m_LoopEffect = NULL;
	m_SkillStartTime = bf_clock_ticks();
}

void	CSkill :: OnExit()
{
	extern UIWindowMgr g_windowMgr;
	if (m_balloon) {
		g_windowMgr.PostQuit(m_balloon);
		m_balloon = NULL;
	}
	HideModel();
}

extern int durationTable[];

bool	CSkill :: OnProcess()
{
	CGameMode * gameMode = g_modeMgr.GetGameMode();
	ProcessChatBalloon();
	ProcessWillBeAttacked();

	switch (GetJob()) {
		case	JT_BENEDICTIO: {
			int		duration = durationTable[EF_BENEDICTIO] - 8;
			if (m_launchCnt >= duration) {
				m_launchCnt = 0;
				LaunchEffect(EF_BENEDICTIO);
			}
			break;
		}
		case	JT_SANCTUARY: {
			if (!m_LoopEffect) {
#ifdef SECONDJOB_SKILL2
				m_LoopEffect = LaunchEffect(EF_BOTTOM_SANC);
#else
				m_LoopEffect = LaunchEffect(EF_BOTTOM2);
#endif
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_DEMONSTRATION: {
				if (!m_LoopEffect) {
					m_LoopEffect = LaunchEffect(EF_DEMONSTRATION);
				}

				if (m_LoopEffect) {
					int		duration = m_LoopEffect->GetDuration() + 20;
					duration++;
					m_LoopEffect->SetDuration(duration);
				}
				break;







			}
			break;
		case	JT_VOLCANO: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_VO);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_FOGWALL: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_FOGWALL);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_DELUGE: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_DE);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_SUITON: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_SUITON);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_VIOLENTGALE: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_VI);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_LANDPROTECTOR: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_LA);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_DISSONANCE: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_DISSONANCE);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_LULLABY: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_LULLABY);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_DARK_RUNNER:
		{
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_RUNNER);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_DARK_TRANSFER:
		{
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_TRANSFER);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_RICHMANKIM: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_RICHMANKIM);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_BASILICA: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_BASILICA);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_SPIDERWEB: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_SPIDER);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_HERMODE:
		{
			if (!m_LoopEffect)
				m_LoopEffect = LaunchEffect(EF_BOTTOM_HERMODE);

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_GRAVITATION:
		{
			if (!m_LoopEffect)
				m_LoopEffect = LaunchEffect(EF_GRAVITATION);

			if (m_LoopEffect)
			{
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_ETERNALCHAOS: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_ETERNALCHAOS);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_DRUMBATTLEFIELD: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_DRUMBATTLEFIELD);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_RINGNIBELUNGEN: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_RINGNIBELUNGEN);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_ROKISWEIL: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_ROKISWEIL);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_INTOABYSS: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_INTOABYSS);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_SIEGFRIED: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_SIEGFRIED);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_WHISTLE: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_WHISTLE);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_ASSASSINCROSS: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_ASSASSINCROSS);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_POEMBRAGI: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_POEMBRAGI);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_APPLEIDUN: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_APPLEIDUN);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_EVILLAND: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_EVILLAND);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_GOSPEL: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_GOSPEL);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_UGLYDANCE: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_UGLYDANCE);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_HUMMING: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_HUMMING);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_DONTFORGETME: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_DONTFORGETME);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_FORTUNEKISS:
			{
			if (!m_LoopEffect)
				m_LoopEffect = LaunchEffect(EF_BOTTOM_FORTUNEKISS);

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_SERVICEFORYOU: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_BOTTOM_SERVICEFORYOU);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_VENOMDUST: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_VENOMDUST2);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_MAGNUS: {
			if (!m_LoopEffect) {
#ifdef SECONDJOB_SKILL2
				m_LoopEffect = LaunchEffect(EF_BOTTOM_MAG);
#else
				m_LoopEffect = LaunchEffect(EF_BOTTOM);
#endif
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_FIREPILLAR: {
			int		duration = durationTable[EF_FIREPILLARBOMB] - 8;
			if (m_launchCnt >= duration) {
				m_launchCnt = 0;
				LaunchEffect(EF_FIREPILLARBOMB);
			}
			break;
		}
		case	JT_PREFIREPILLAR: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_FIREPILLARON);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_ICEWALL: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_ICEWALL);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_DEATHWAVE: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_ICEWALL);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_WINDATTACK: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_FIREPILLARON);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_WATERATTACK: {
			if (!m_LoopEffect) {
				m_LoopEffect = LaunchEffect(EF_ICEWALL);
			}

			if (m_LoopEffect) {
				int		duration = m_LoopEffect->GetDuration() + 20;
				duration++;
				m_LoopEffect->SetDuration(duration);
			}
			break;
		}
		case	JT_QUAGMIRE: {
			int		duration = durationTable[EF_QUAGMIRE] - 20;
			if (m_launchCnt >= duration) {
				m_launchCnt = 0;
				LaunchEffect(EF_QUAGMIRE);
			}
			break;
		}
		case	JT_PREWARPPORTAL: {
#ifdef SECONDJOB_SKILL2
				int		duration = durationTable[EF_READYPORTAL2] - 8;
				if (m_launchCnt >= duration) {
					m_launchCnt = 0;
					LaunchEffect(EF_READYPORTAL2);
				}
#else
				int		duration = durationTable[EF_READYPORTAL] - 8;
				if (m_launchCnt >= duration) {
					m_launchCnt = 0;
					LaunchEffect(EF_READYPORTAL);
				}
#endif
			}
			break;
		case	JT_WARPPORTAL:
			{
#ifdef SECONDJOB_SKILL2
				if(g_session.m_isEffectOn)
				{
					int		duration = durationTable[EF_PORTAL2] - 8;
					if (m_launchCnt >= duration) {
						m_launchCnt = 0;
						LaunchEffect(EF_PORTAL2);
					}
				}
				else
				{
					int		duration = durationTable[EF_PORTAL] - 8;
					if (m_launchCnt >= duration) {
						m_launchCnt = 0;
						LaunchEffect(EF_PORTAL);
					}
				}
#else
				int		duration = durationTable[EF_PORTAL] - 8;
				if (m_launchCnt >= duration) {
					m_launchCnt = 0;
					LaunchEffect(EF_PORTAL);
				}
#endif
			}
			break;
		case	JT_COUPLE:
			{
					int		duration = durationTable[EF_PORTAL3] - 8;
					if (m_launchCnt >= duration) {
						m_launchCnt = 0;
						LaunchEffect(EF_PORTAL3);
					}
			}
			break;
		case	JT_SAFETYWALL: {
#ifdef SECONDJOB_SKILL2
				int		duration = durationTable[EF_GLASSWALL2] - 7;
				if (m_launchCnt >= duration) {
					m_launchCnt = 0;
					LaunchEffect(EF_GLASSWALL2);
				}
#else
				int		duration = durationTable[EF_GLASSWALL] - 7;
				if (m_launchCnt >= duration) {
					m_launchCnt = 0;
					LaunchEffect(EF_GLASSWALL);
				}
#endif
			}
			break;
		case	JT_FIREWALL: {
				int		duration = durationTable[EF_FIREWALL] - 7;
				if (m_SkillStartTime + 3000 < bf_clock_ticks())
					return false;
				if (m_launchCnt >= duration) {
					m_launchCnt = 0;
					LaunchEffect(EF_FIREWALL);
				}
			}
			break;
		case	JT_TATAMI:
			{







				if (!m_LoopEffect)
					m_LoopEffect = LaunchEffect(EF_TATAMI);

				if (m_LoopEffect) {
					int		duration = m_LoopEffect->GetDuration() + 20;
					duration++;
					m_LoopEffect->SetDuration(duration);
				}
				break;
			}
		case	JT_KAEN:
			{
				int		duration = durationTable[EF_KAEN]-7;
				if (m_launchCnt >= duration) {
					m_launchCnt = 0;
					LaunchEffect(EF_KAEN);
				}
			}
			break;
		case	JT_LIGHTNING_S:
			{
				int		duration = durationTable[EF_LIGHTNING_S]-7;
				if (m_launchCnt >= duration) {
					m_launchCnt = 0;
					LaunchEffect(EF_LIGHTNING_S);
				}
			}
			break;
		case	JT_BLIND_S:
			{
				int		duration = durationTable[EF_BLIND_S]-7;
				if (m_launchCnt >= duration) {
					m_launchCnt = 0;
					LaunchEffect(EF_BLIND_S);
				}
			}
			break;
		case	JT_POISON_S:
			{
				int		duration = durationTable[EF_POISON_S]-7;
				if (m_launchCnt >= duration) {
					m_launchCnt = 0;
					LaunchEffect(EF_POISON_S);
				}
			}
			break;
		case	JT_FREEZING_S:
			{
				int		duration = durationTable[EF_FREEZING_S]-7;
				if (m_launchCnt >= duration) {
					m_launchCnt = 0;
					LaunchEffect(EF_FREEZING_S);
				}
			}
			break;
		case	JT_FLARE_S:
			{
				int		duration = durationTable[EF_FLARE_S]-7;
				if (m_launchCnt >= duration) {
					m_launchCnt = 0;
					LaunchEffect(EF_FLARE_S);
				}
			}
			break;
		case	JT_HIDDEN_TRAP:
			break;
	}
	++m_launchCnt;
	return	true;
}

void	CSkill :: SetRenderInfo(RENDER_INFO_RECT &info, const float tlvertX, const float tlvertY)
{
	if (m_shouldAddPickInfo) {
		static int	minRectLength = 34*g_renderer->GetScreenXFactor();


		float	oowOffset = 350;
		CActorPickInfo *	actorPickInfo = new CActorPickInfo;
		actorPickInfo->m_vectors[0].x	= (info.left<=info.right) ? info.left : info.right;
		actorPickInfo->m_vectors[0].y	= info.top;
		actorPickInfo->m_vectors[0].z	= info.oow + oowOffset;
		actorPickInfo->m_vectors[1].x	= (info.left>info.right) ? info.left : info.right;
		actorPickInfo->m_vectors[1].y	= info.bottom;
		actorPickInfo->m_vectors[1].z	= info.oow + oowOffset;
		actorPickInfo->m_gid			= m_aid;
		actorPickInfo->SetJob(GetJob());
		actorPickInfo->m_isPkState		= 0;
		actorPickInfo->m_classType	= CT_CSKILL;


		int		width = actorPickInfo->m_vectors[1].x - actorPickInfo->m_vectors[0].x;
		if (width < minRectLength) {
			actorPickInfo->m_vectors[1].x += (minRectLength - width)/2;
			actorPickInfo->m_vectors[0].x -= (minRectLength - width)/2;
		}
		int		height = actorPickInfo->m_vectors[1].y - actorPickInfo->m_vectors[0].y;
		if (height < minRectLength) {
			actorPickInfo->m_vectors[1].y += (minRectLength - height)/2;
			actorPickInfo->m_vectors[0].y -= (minRectLength - height)/2;
		}

		g_actorPickNode.AddPickInfo(actorPickInfo);
	}
	SetTlvert(tlvertX, tlvertY);
}

void	CSkill :: Render(matrix &vtm)
{
	if (m_isVisibleBody) {

		float			x, y, oow;
		g_renderer->ProjectVertex(m_pos, vtm, &x, &y, &oow);

		float &		sXf = g_renderer->m_screenXFactor;
		float &		sYf = g_renderer->m_screenYFactor;
		float		pixelRatio = g_renderer->GetPixelRatio(oow);
		m_lastPixelRatio = pixelRatio / g_avgPixelRatio;


		int		xSize = 5;
		int		ySize = 5;

		RENDER_INFO_RECT	info = {	x-float(xSize/2)*sXf*m_lastPixelRatio,
										y-float(ySize/2)*sYf*m_lastPixelRatio,
										x+float(xSize/2)*sXf*m_lastPixelRatio,
										y, oow };
		SetRenderInfo(info, x, y);
	} else {
		UpdateTlvertex(vtm);
	}
}

void	CSkill :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	CGameMode *		gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);

	switch (message)
	{
		case	AM_SKILL:
		{
			SKILLSTRUCT *	ss = (SKILLSTRUCT *)arg1;
			int				damage = ss->damage;
			int				skillId = ss->skillId;
			int				level = ss->level;
			int				count = ss->count;
			int				tCellX = ss->tCellX;
			int				tCellY = ss->tCellY;
			int				attackedMT = ss->attackedMT;
			if (count==0)
				count = 1;

			damage /= count;

			DWORD			targetGid = ss->targetGID;

			CGameActor *	tActor = NULL;

			if (g_session.GetAid() == targetGid) {
				tActor = gameMode->GetWorld()->m_player;
				g_session.GetJob();
			} else {
				CGameActor *	theActor = gameMode->GetWorld()->GetGameActorByAID(targetGid);
				if (theActor) {
					tActor = theActor;
					theActor->GetJob();
				}
			}

			int		atkedEfId = EF_HIT1;
			if (skillId >= SKID_TK_MISSION)
				atkedEfId = -1;

			switch (skillId)
			{
				case	SKID_MG_FIREWALL:
					atkedEfId = EF_FIREHIT;
					break;
				case	SKID_WZ_VERMILION:
					atkedEfId = EF_WINDHIT;
					break;
				case	SKID_WZ_STORMGUST:
					atkedEfId = EF_COLDHIT;
					break;
				case	SKID_PR_MAGNUS:
					atkedEfId = EF_HOLYHIT;
					break;
				case	SKID_SG_SUN_WARM:
				case	SKID_SG_MOON_WARM:
				case	SKID_SG_STAR_WARM:
					atkedEfId = EF_M01;
					break;
			}

			if (tActor)
			{
				float	attackedSpeedFactor = float(attackedMT) / float(AVG_ATTACKED_SPEED);
				WBA		wba(AM_ATTACKED, damage, bf_clock_ticks(), m_aid, tCellX, tCellY, atkedEfId, -1, level, attackedSpeedFactor);

				int totalDamage = damage;
				tActor->SendMsg(this, AM_WILL_BE_ATTACKED, INT_PTR(&wba), totalDamage);

				for (int i=0; i<count-1; ++i)
				{
					wba.time += doubleAttackTerm;
					totalDamage += damage;
					tActor->SendMsg(this, AM_WILL_BE_ATTACKED, INT_PTR(&wba), totalDamage);
				}
			}

			if (m_3dactor)
				m_3dactor->SetAnimType(1);
		}
		break;
		case	AM_SETJOB:
		{
				SetJob(arg1);

				switch (GetJob()) {
					case	JT_WARPPORTAL:	{
						vector3d &		pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
						PlayWave("effect\\EF_Portal.wav", m_pos.x-pPos.x, 0,  m_pos.z-pPos.z);
						break;
					}
					case JT_TRAPBOOM: {
						if (m_effectId != -1) {
							CRagEffect *	effect = LaunchEffect(m_effectId);
							if (effect)
								effect->SendMsg(NULL, AM_SETPOS, INT_PTR(&m_pos));
						}
						break;
					}
				}

				m_launchCnt = 60*60*10;

				ShowModel(GetTrapModelName());
				if (m_3dactor) m_3dactor->SetAnimType(1);
				break;
			}
		case	AM_SKILLSTANDENTRY: {
			SKILLSTANDENTRYSTRUCT *		ss = (SKILLSTANDENTRYSTRUCT *)arg1;
			m_aid = ss->AID;
			m_gid = ss->AID;
			SetJob(ss->job);
#if defined(BALANCE_PATCH)
			m_isVisibleBody = ss->isVisible;
			if(ss->isVisible) {
#else
			if(m_isVisibleBody = ss->isVisible) {
#endif
				m_isVisibleBody = TRUE;
			} else if(ss->creatorAID == g_session.GetAid() || g_session.GetPartyInfoByAid(ss->creatorAID).isValid) {
				m_isVisibleBody = TRUE;
			} else {
				m_isVisibleBody = FALSE;
			}

			gameMode->GetClientCoor(ss->xPos, ss->yPos, m_pos.x, m_pos.z);
			CWorld *	world = gameMode->GetWorld();
			m_pos.y = world->m_attr->GetHeight(m_pos.x, m_pos.z);

			m_effectId = -1;
			switch (GetJob()) {
				case	JT_ICEWALL:
					m_shouldAddPickInfo = true;
					break;
				case	JT_FIREWALL: {
					vector3d &		pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
					PlayWave("effect\\EF_FireWall.wav", m_pos.x-pPos.x, 0, m_pos.z-pPos.z);
					m_shouldAddPickInfo = false;
					break;
				}
				case	JT_TATAMI:
				case	JT_LIGHTNING_S:
				case	JT_BLIND_S:
				case	JT_POISON_S:
				case	JT_FREEZING_S:
				case	JT_FLARE_S:
				{
					m_shouldAddPickInfo = false;
					break;
				}
				case	JT_KAEN: {
					if (random(8) == 0)
					{
						vector3d &		pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;

						PlayWave("effect\\\xc8\xad\xbf\xb0\xc1\xf8.wav", m_pos.x-pPos.x, 0, m_pos.z-pPos.z);

					}
					m_shouldAddPickInfo = false;
					break;
				}
				case	JT_TRAP:
					m_shouldAddPickInfo = true;
					ShowModel(GetTrapModelName());
					break;
				case	JT_TRAP_BLASTMINE:
					m_shouldAddPickInfo = true;
					m_effectId = EF_BLASTMINEBOMB;
					ShowModel(GetTrapModelName());
					break;
				case	JT_TRAP_SKIDTRAP:
				case	JT_TRAP_ANKLESNARE:
				case	JT_TRAP_LANDMINE:
				case	JT_TRAP_TALKIEBOX:
					m_shouldAddPickInfo = true;
					ShowModel(GetTrapModelName());
					break;
				case	JT_TRAP_SANDMAN:
					m_shouldAddPickInfo = true;
					m_effectId = EF_SANDMAN;
					ShowModel(GetTrapModelName());
					break;
				case	JT_TRAP_FLASHER:
					m_shouldAddPickInfo = true;
					m_effectId = EF_FLASHER;
					ShowModel(GetTrapModelName());
					break;
				case	JT_TRAP_FREEZINGTRAP:
					m_shouldAddPickInfo = true;
					m_effectId = EF_FREEZING;
					ShowModel(GetTrapModelName());
					break;
				case	JT_TRAP_CLAYMORETRAP:
					m_shouldAddPickInfo = true;
					m_effectId = EF_CLAYMORE;
					ShowModel(GetTrapModelName());
					break;
				case	JT_TRAP_SHOCKWAVE:
					m_shouldAddPickInfo = true;

					ShowModel(GetTrapModelName());

					if ( !IsEffectExist(EF_SHOCKWAVE) )
					{
						vector3d fDeltaPos = vector3d(0.0f, 0.0f, 0.0f);
						float fRot = 0.0f ;
						LaunchEffect(EF_SHOCKWAVE, fDeltaPos, fRot);
					}
					break;
			}
			break;
		}
		case	AM_SKILL_UPDATE:
			if (!m_isVisibleBody) {
				m_isVisibleBody = TRUE;
				ShowModel(GetTrapModelName());
			}
			break;
		case	AM_CHAT:
			{
				extern UIWindowMgr g_windowMgr;
				char * chatMsg = (char *)arg1;
				if (strlen(chatMsg) == 0)
					break;

				m_chatTick = bf_clock_ticks();
				if (m_balloon == NULL) {
					m_balloon = new UITransBalloonText;
					m_balloon->Create(8, 8);
					g_windowMgr.AddWindowFront(m_balloon);
				}

				m_balloon->SetText(chatMsg, 35);
				UIBalloonText *	b = m_balloon;
				b->SetFntColor(0xff8080, 0);
				m_chatWidth = m_balloon->GetWidth();
				m_chatHeight = m_balloon->GetHeight();
				m_balloon->Move(m_lastTlvertX - m_chatWidth / 2, m_lastTlvertY - g_renderer->m_height * 110 / 480);
			}
			break;
		default:
			CGameActor::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

char *	CSkill :: GetTrapModelName()
{
	switch (GetJob()) {
		case	JT_TRAP_BLASTMINE:
			return	"\xbf\xdc\xba\xce\xbc\xd2\xc7\xb0\\\xc6\xae\xb7\xa6" "03_3.rsm";
		case	JT_TRAP_SKIDTRAP:
			return	"\xbf\xdc\xba\xce\xbc\xd2\xc7\xb0\\\xc6\xae\xb7\xa6" "02.rsm";
		case	JT_TRAP_ANKLESNARE:
			return	"\xbf\xdc\xba\xce\xbc\xd2\xc7\xb0\\\xc6\xae\xb7\xa6" "01.rsm";
		case	JT_TRAP_LANDMINE:
			return	"\xbf\xdc\xba\xce\xbc\xd2\xc7\xb0\\\xc6\xae\xb7\xa6" "03.rsm";
		case	JT_TRAP_SHOCKWAVE:
			return	"\xbf\xdc\xba\xce\xbc\xd2\xc7\xb0\\\xc6\xae\xb7\xa6" "03_6.rsm";
		case	JT_TRAP_SANDMAN:
			return	"\xbf\xdc\xba\xce\xbc\xd2\xc7\xb0\\\xc6\xae\xb7\xa6" "03_4.rsm";
		case	JT_TRAP_FLASHER:
			return	"\xbf\xdc\xba\xce\xbc\xd2\xc7\xb0\\\xc6\xae\xb7\xa6" "03_5.rsm";
		case	JT_TRAP_FREEZINGTRAP:
			return	"\xbf\xdc\xba\xce\xbc\xd2\xc7\xb0\\\xc6\xae\xb7\xa6" "03_2.rsm";
		case	JT_TRAP_CLAYMORETRAP:
			return	"\xbf\xdc\xba\xce\xbc\xd2\xc7\xb0\\\xc6\xae\xb7\xa6" "04.rsm";
		case	JT_TRAP_TALKIEBOX:
			return	"\xbf\xdc\xba\xce\xbc\xd2\xc7\xb0\\\xc6\xae\xb7\xa6" "05.rsm";

		default:
			return	NULL;
	}
}

void	CSkill :: ShowModel(char* modelName)
{
	if (!modelName) return;
	if(!m_isVisibleBody) return;

	HideModel();

	vector3d 		lightDir(0.5f, 0.7f, 0.5f);
	vector3d 		diffuseCol (1, 1, 1);
	vector3d 		ambientCol(0.5f, 0.5f, 0.5f);
	C3dModelRes *	newRes = (C3dModelRes *)g_resMgr().Get(modelName, false);
	m_3dactor = new C3dActor;
	m_3dactor->SetAnimSpeed(1.0f);
	vector3d	vector = this->m_pos;
	m_3dactor->SetVolumeBoxInfo(&newRes->m_volumeBoxList);
	m_3dactor->SetPos(vector);
	m_3dactor->AssignModel(newRes);
	m_3dactor->SetName(modelName);
	m_3dactor->SetFrame(3);
	m_3dactor->SetBlockType(0);
	m_3dactor->SetIsHideCheck(FALSE);
	m_3dactor->SetAnimType(0);
	m_3dactor->UpdateVertexColor(lightDir, diffuseCol,ambientCol);
	g_modeMgr.GetGameMode()->GetWorld()->GetRootNode()->InsertObjectAfter(m_3dactor, 0);
}

void	CSkill :: HideModel()
{
	if (m_3dactor) {
		CWorld *			world = g_modeMgr.GetGameMode()->GetWorld();
		SceneGraphNode *	rootNode = world->GetRootNode();
		if (rootNode) rootNode->RemoveObject(m_3dactor, 0);
		m_3dactor = NULL;
		if (m_LoopEffect)
			m_LoopEffect->SendMsg(NULL, AM_MASTER_DEAD);
		m_LoopEffect = NULL;
	}
}

CGraffiSkill :: CGraffiSkill()
{
}

CGraffiSkill :: ~CGraffiSkill()
{
	OnExit();
}

void	CGraffiSkill :: OnInit()
{
	BYTE	c;
	int		gap, k, j;


	m_offsX = 0, m_offsY = 0;
	m_fontTex = g_texMgr.GetTexture("effect\\alpabet.bmp", false);
	m_tex = g_texMgr.CreateTexture(384UL, 384UL, PF_A1R5G5B5, NULL);
	int		len = strlen(m_strMsg);
	m_offsY += ((144-len)/24*36);

	for( int i=0;i<len;i++ ) {
		c = m_strMsg[i];

		if( c>='a' && c<='z' ) {
			c = 26+c-'a'; gap = 32;
		}
		else
		if( c>='A' && c<='Z' ) {
			c = c-'A'; gap = 32;
		}
		else
		if( c>='0' && c<='9' ) {
			c = 54+c-'0'; gap = 32;
		}
		else
		if( c=='?' ) {
			c = 53; gap = 16;
		}
		else
		if( c=='!' ) {
			c = 54; gap = 16;
		}
		else
		if( c==' ' ) {
			k = i+1;
			j = m_offsX;
			while( m_strMsg[k]!=' ' && m_strMsg[k]!=0 ) {
				j+=32;
				k++;
			}
			if( j>=512 ) m_offsX = 0, m_offsY += 36;
			c = 99;
		}
		else {
			c = 99; gap = 16;
		}
		if( m_offsX+gap>= 512 ) m_offsX = 0, m_offsY += 36;
		if( c!=99 ) m_tex->CopyTexture( m_fontTex, (c%8)*32, (c/8)*32, 32, 32, m_offsX, m_offsY, gap, 32 );
		m_offsX += gap;
	}
	m_rot = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_roty;

}

void	CGraffiSkill :: OnExit()
{
	if( m_tex ) delete m_tex;
}

bool	CGraffiSkill :: OnProcess()
{

	return	true;
}

void	CGraffiSkill :: Render(matrix &vtm)
{
	int attrX = m_ground_pos_x;
	int attrY = m_ground_pos_y;

	CGameMode * gameMode = g_modeMgr.GetGameMode();
	C3dGround* ground = gameMode->GetWorld()->m_ground;
	int	size = 18;

	attrX -= size/2;
	attrY -= size/2;

	vector2d uv[4];

	float half_size = float(size)/2;
	float sin_r = _sin(DegToRad(m_rot));
	float cos_r = _cos(DegToRad(m_rot));
	DWORD color = 0xFFffffff;

	for(int i=0; i<size; ++i) {
		for(int j=0; j<size; ++j) {

			int x = attrX+j;
			int y = attrY+i;

			float u0 = j+0-half_size;
			float v0 = i+0-half_size;
			float u1 = j+1-half_size;
			float v1 = i+1-half_size;

			uv[0].Set(u0*cos_r + v0*sin_r, -u0*sin_r + v0*cos_r);
			uv[1].Set(u1*cos_r + v0*sin_r, -u1*sin_r + v0*cos_r);
			uv[2].Set(u0*cos_r + v1*sin_r, -u0*sin_r + v1*cos_r);
			uv[3].Set(u1*cos_r + v1*sin_r, -u1*sin_r + v1*cos_r);

			uv[0].Set(1.0f-(uv[0].x+half_size)/size, (uv[0].y+half_size)/size);
			uv[1].Set(1.0f-(uv[1].x+half_size)/size, (uv[1].y+half_size)/size);
			uv[2].Set(1.0f-(uv[2].x+half_size)/size, (uv[2].y+half_size)/size);
			uv[3].Set(1.0f-(uv[3].x+half_size)/size, (uv[3].y+half_size)/size);

			ground->RenderGroundTile(vtm, x, y, color, m_tex, uv);
		}
	}
}

void	CGraffiSkill :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	switch (message) {
		case	AM_SETMASTERGID:
			SetAID(arg1);
			break;
		case	AM_SETTIME:
			m_lastingTime = arg1;
			break;
		case	AM_SET_SKILL:
			strcpy( m_strMsg, (char*)arg1 );
			m_ground_pos_x = arg2;
			m_ground_pos_y = arg3;
			OnInit();
			break;
		default:
			CGameActor::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

CMsgEffect :: CMsgEffect()
{
	OnInit();
}

CMsgEffect :: ~CMsgEffect()
{
	OnExit();
}

void	CMsgEffect :: OnInit()
{
	m_masterActor = NULL;
	m_baseAction = 0;
	m_curMotion = 0;
	m_alwaysTopLayer = 1;
	m_masterGid = 0;
	m_msgEffectType = 0;
	m_isDisappear = 0;
	m_dist = 0;
	m_orgPosY = 1;
	m_orgPosX = 1;
	m_orgZoom = 1;
	m_stateCnt = 0;
	m_oldAction = 0;
	m_offsetOow = -1;
	m_orgSprShift = 0;
	m_isAttackTarget = FALSE;
	m_isAnimation = TRUE;
	m_destPos.x = 0.0f;
	m_destPos.y = 0.0f;
	m_destPos.z = 0.0f;
	m_destPos2.Set(0.0f, 0.0f, 0.0f);
}

void	CMsgEffect :: OnExit()
{
	if (m_masterActor)
		m_masterActor->SendMsg(this, AM_MSGEFFECT_DEAD, 0, 0, 0);
}

void	CMsgEffect :: ProcessPosition()
{
	switch (m_msgEffectType) {
		case	MET_LUCKY_EFFECT:
		case	MET_GUARD:
		case	MET_MISS:
		case	MET_NUMBER:
		case	MET_NUMBER_CRITICAL:
		case	MET_CRITICAL_EFFECT:
		case	MET_BLUE_EFFECT:
		case	MET_NUMBER_TOTAL:
		case	MET_NUMBER_HALF:
			if (m_masterActor) {
				m_destPos2.x = m_pos.x = m_masterActor->m_pos.x;
				m_destPos2.z = m_pos.z = m_masterActor->m_pos.z;
			}
			break;
		case	MET_ANGELUS_EFFECT:
			if (m_masterActor) {
				m_pos = m_masterActor->m_pos;
				m_pos.y -= 15;
			}
			break;
		case	MET_CRUSIS_EFFECT:
			if (m_masterActor) {
				m_pos = m_masterActor->m_pos;
				m_pos.y -= 20;
			}
			break;
		case	MET_ICEBREAKEFFECT:
			break;
		case	MET_ICEEFFECT:
			if (m_masterActor) {
				m_pos = m_masterActor->m_pos;
			}
			break;
		case	MET_PKEFFECT:
			if (m_masterActor) {
				m_pos = m_masterActor->m_pos;
				m_pos.y -= 15;
			}
			break;
		case	MET_EMOTION:
			if (m_masterActor) {
				m_pos = m_masterActor->m_pos;
				if (m_stateId == 10000)
					m_pos.y -= 10;
				else
					m_pos.y -= 16;

			}
			break;
		case	MET_MVP:
		case	MET_SLEEP_EFFECT:
		case	MET_STUN_EFFECT:
		case	MET_SILENCE_EFFECT:
			if (m_masterActor) {
				m_pos = m_masterActor->m_pos;
				if (m_stateId == 10000)
					m_pos.y -= 10;
				else
					m_pos.y -= 20;
			}
			break;
		case	MET_CURSE_EFFECT:
			if (m_masterActor) {
				m_pos = m_masterActor->m_pos;
				m_pos.y -= 15;
				m_roty = m_masterActor->m_roty;
				int		dir = Get8Dir(m_roty);
				m_curAction = m_baseAction + dir;

				switch (dir) {
					case	2:
					case	3:
					case	4:
					case	5:
						m_offsetOow = 3;
						break;
					default:
						m_offsetOow = -3;
						break;
				}
			}
			break;
		case	MET_CART:
			if (m_masterActor)
			{
				CGameMode *		gameMode = g_modeMgr.GetGameMode();
				sAssert(gameMode);
				CWorld *		world = gameMode->GetWorld();
				float			dist = m_masterActor->CalcDist(m_pos.x, m_pos.z);
				int				dir = Get8Dir2(m_roty, TRUE);
				m_curAction = m_baseAction + dir;

				vector3d &		Mpos = m_masterActor->m_pos;
				if (dist > 5)
				{
					vector3d	lastPos = m_pos;
					vector2d	dPos(m_pos.x-Mpos.x, m_pos.z-Mpos.z);
					dPos.Normalize();
					dPos.x *= 5;
					dPos.y *= 5;
					m_pos.x = Mpos.x + dPos.x;
					m_pos.z = Mpos.z + dPos.y;

					{
						vector3d	progress;
						progress.x = m_pos.x - lastPos.x;
						progress.z = m_pos.z - lastPos.z;
						float		deltaDist = _sqrt(progress.x*progress.x + progress.z*progress.z);
						m_dist += deltaDist;
					}
					CActRes *	actRes = GetActRes();
					int			motCount = actRes->GetMotionCount(m_curAction);
					m_curMotion = int(m_dist * 0.92f / m_motionSpeed) % motCount;
				}

				CalcDir(Mpos.x, Mpos.z);
				m_pos.y = world->m_attr->GetHeight(m_pos.x, m_pos.z);

			}
			break;

		case	MET_BIRD:
			if (m_masterActor && !m_isAttackTarget) {
				CGameMode *		gameMode = g_modeMgr.GetGameMode();
				sAssert(gameMode);
				CWorld *		world = gameMode->GetWorld();
				float			dist = m_masterActor->CalcDist(m_pos.x, m_pos.z);
				int				dir = Get8Dir2(m_roty, TRUE);
				m_curAction = m_baseAction + dir;

				vector3d &	Mpos = m_masterActor->m_pos;

				if (dist > 25 && !m_isAnimation) {
					vector3d	lastPos = m_pos;
					{
						vector2d	dPos(m_pos.x-Mpos.x, m_pos.z-Mpos.z);
						dPos.Normalize();
					}
					m_pos.x += (Mpos.x - m_pos.x) / 25;
					m_pos.z += (Mpos.z - m_pos.z) / 25;
					m_curMotion = 8;

					{
						vector3d	progress;
						progress.x = m_pos.x - lastPos.x;
						progress.z = m_pos.z - lastPos.z;
						float		deltaDist = _sqrt(progress.x*progress.x + progress.z*progress.z);
						m_dist += deltaDist;
					}
				} else if (dist <= 25 && !m_isAnimation) {
					m_isAnimation = TRUE;
					vector3d	lastPos = m_pos;
					{
						vector2d	dPos(m_pos.x-Mpos.x, m_pos.z-Mpos.z);
						dPos.Normalize();
					}
					m_pos.x += (Mpos.x - m_pos.x) / 25;
					m_pos.z += (Mpos.z - m_pos.z) / 25;
					m_curMotion = 8;

					{
						vector3d	progress;
						progress.x = m_pos.x - lastPos.x;
						progress.z = m_pos.z - lastPos.z;
						float		deltaDist = _sqrt(progress.x*progress.x + progress.z*progress.z);
						m_dist += deltaDist;
					}
				} else if (dist > 12 && !m_isAttackTarget) {
					vector3d	lastPos = m_pos;
					{
						vector2d	dPos(m_pos.x-Mpos.x, m_pos.z-Mpos.z);
						dPos.Normalize();
					}
					m_pos.x += (Mpos.x - m_pos.x) / dist;
					m_pos.z += (Mpos.z - m_pos.z) / dist;

					{
						vector3d	progress;
						progress.x = m_pos.x - lastPos.x;
						progress.z = m_pos.z - lastPos.z;
						float	deltaDist = _sqrt(progress.x*progress.x + progress.z*progress.z);
						m_dist += deltaDist;
					}
				} else if (dist > 3) {
					vector3d	lastPos = m_pos;
					{
						vector2d	dPos(m_pos.x-Mpos.x, m_pos.z-Mpos.z);
						dPos.Normalize();
					}
					m_pos.x += (Mpos.x - m_pos.x) / 20;
					m_pos.z += (Mpos.z - m_pos.z) / 20;
					{
						vector3d	progress;
						progress.x = m_pos.x - lastPos.x;
						progress.z = m_pos.z - lastPos.z;
						float	deltaDist = _sqrt(progress.x*progress.x + progress.z*progress.z);
						m_dist += deltaDist;
					}
				}
				CalcDir(Mpos.x, Mpos.z);

				m_pos.y = world->m_attr->GetHeight(m_pos.x, m_pos.z) - 25;
			}
			break;
#if defined (_HOMUN)
		case	MET_CURSOR_EFFECT:
			if (m_masterActor) {
				m_pos = m_masterActor->m_pos;
				m_pos.y -= 20 ;
			}
#endif
		default:
			if (m_msgEffectType == MET_NUMBER + 100) {
				m_pos.x = m_masterActor->m_pos.x;
				m_pos.z = m_masterActor->m_pos.z;
			}
			break;
	}
}

bool	CMsgEffect :: OnProcess()
{
	if (m_isDisappear)
	{
		if (m_msgEffectType == MET_BIRD)
		{
			if (m_stateCnt>35 && m_isAttackTarget)
			{
				m_isAttackTarget = FALSE;
				m_isDisappear = FALSE;
				return	true;
			} else if (!m_isAttackTarget)
				return	false;
		} else
			return	false;
	}

	ProcessPosition();

	switch (m_msgEffectType) {
		case	MET_NUMBER_CRITICAL:
		case	MET_NUMBER:
		case	MET_NUMBER_TOTAL:
		case	MET_NUMBER_HALF:
		case	MET_MISS:
		case	MET_LUCKY_EFFECT:
		case	MET_CRITICAL_EFFECT:
		case	MET_BLUE_EFFECT:
		case	MET_BIRD:
			break;
		default:
			if (m_msgEffectType == MET_NUMBER + 100) {
				break;
			}
			ProcessSound();
			break;
	}

	switch (m_msgEffectType)
	{
		case	MET_NUMBER_CRITICAL:
			{

			float	stateCnt = GetStateCnt();
			m_curAction = m_baseAction;
			if (m_destPos.x == 0.0f && m_masterActor)
			{
				int i = m_masterActor->m_roty;
				if (i >= 0 && i < 90)
				{
					i = 0;
				} else if (i >= 90 && i < 180)
				{
					i = 2;
				} else if (i >= 180 && i < 270)
				{
					i = 4;
				} else
				{
					i = 6;
				}
				const float mov = 0.5f;
				switch(i)
				{
					case 0:
					case 1:
						m_destPos.x = -mov;
						m_destPos.z = mov;
						break;
					case 2:
					case 3:
						m_destPos.x = -mov;
						m_destPos.z = -mov;
						break;
					case 4:
					case 5:
						m_destPos.x = mov;
						m_destPos.z = -mov;
						break;
					case 6:
					case 7:
						m_destPos.x = mov;
						m_destPos.z = mov;
						break;
					default:
						m_destPos.x = 0.0f;
						m_destPos.z = 0.0f;
						break;
				}
			}
			m_pos.x = m_destPos2.x + (m_destPos.x * (stateCnt/3)) + (m_destPos.x * 3);
			m_pos.z = m_destPos2.z + (m_destPos.z * (stateCnt/3)) + (m_destPos.z * 3);
			m_pos.y = (m_orgPosY  + 8)- stateCnt * (2.0f - (stateCnt / 30)) + m_destPos.y;
			if (m_pos.y > 0)
			{
				m_destPos.y = m_pos.y / (m_pos.y * 3 / 4);
			} else
			{
				m_destPos.y = -m_pos.y / (-m_pos.y * 3 / 4);
			}
			if (m_zoom > 1.3f)
				m_zoom = m_orgZoom - float(stateCnt)*0.24f;
			else if (m_zoom<1.3f)
				m_zoom = 1.3f;
			SetArgb(250 - stateCnt*3.4f, -1, -1, -1);

			if (stateCnt > 70)
				return	false;
			return	true;
			break;
		}
		case	MET_NUMBER:
		case	MET_NUMBER_HALF:
			{
			float	stateCnt = GetStateCnt();
			m_curAction = m_baseAction;

			if (m_destPos.x == 0.0f && m_masterActor)
			{
				int i = m_masterActor->m_roty;
				if (i >= 0 && i < 90)
				{
					i = 0;
				} else if (i >= 90 && i < 180)
				{
					i = 2;
				} else if (i >= 180 && i < 270)
				{
					i = 4;
				} else
				{
					i = 6;
				}
				const float mov = 0.8f;
				switch(i) {
					case 0:
					case 1:
						m_destPos.x = -mov;
						m_destPos.z = mov;
						break;
					case 2:
					case 3:
						m_destPos.x = -mov;
						m_destPos.z = -mov;
						break;
					case 4:
					case 5:
						m_destPos.x = mov;
						m_destPos.z = -mov;
						break;
					case 6:
					case 7:
						m_destPos.x = mov;
						m_destPos.z = mov;
						break;
					default:
						m_destPos.x = 0.0f;
						m_destPos.z = 0.0f;
						break;
				}
			}
			m_pos.x = m_destPos2.x + (m_destPos.x * (stateCnt/3)) + (m_destPos.x * 3);
			m_pos.z = m_destPos2.z + (m_destPos.z * (stateCnt/3)) + (m_destPos.z * 3);
			m_pos.y = (m_orgPosY  + 8)- stateCnt * (2.0f - (stateCnt / 30)) + m_destPos.y;
			if (m_pos.y > 0) {
				m_destPos.y = m_pos.y / (m_pos.y * 3 / 4);
			} else {
				m_destPos.y = -m_pos.y / (-m_pos.y * 3 / 4);
			}
			if (m_zoom > 1.2f)
				m_zoom = m_orgZoom - float(stateCnt)*0.24f;
			else if (m_zoom<1.2f)
				m_zoom = 1.2f;
			SetArgb(250 - stateCnt*3.4f, -1, -1, -1);
			if (stateCnt > 70)
				return	false;
			return	true;
			break;
		}

		case	MET_GUARD:
			{
			float	stateCnt = GetStateCnt();
			m_curAction = m_baseAction;
			if (!m_curMotion)
			{
				vector3d & pPos = g_modeMgr.GetGameMode()->GetWorld()->m_player->m_pos;
				PlayWave("effect\\kyrie_guard.wav", m_pos.x - pPos.x, 0, m_pos.z - pPos.z);
			}
			m_curMotion++;
			if (m_curMotion >= GetActRes()->GetMotionCount(m_curAction))
			{
				m_curMotion = GetActRes()->GetMotionCount(m_curAction)-1;
			}



			SetArgb(250, -1, -1, -1);
			m_orgZoom = m_zoom = 1.2f;

			if (stateCnt > 10)
				return	false;
			return	true;
			break;
		}

		case	MET_MISS:
			{
			float	stateCnt = GetStateCnt();
			m_curAction = m_baseAction;

			m_pos.y = m_orgPosY - float(stateCnt)*0.54f;


			SetArgb(250 - stateCnt*3, -1, -1, -1);



				m_zoom = 1.0f;

			CActRes *	actRes = GetActRes();







			if (stateCnt > 80)
				return	false;
			return	true;
			break;
		}

		case	MET_NUMBER_TOTAL:
			{
			float	stateCnt = GetStateCnt();
			int alphaCnt = 0;
			m_curAction = m_baseAction;

			m_pos.y = m_orgPosY - float(stateCnt)*0.1f;
			if (stateCnt > 90)
			{
				alphaCnt = (stateCnt - 90) * 8;
			}

			SetArgb(250 - alphaCnt, -1, -1, -1);

			if (m_zoom < 3.0f)
				m_zoom += float(stateCnt)*0.18f;
			else if (m_zoom > 3.0f)
				m_zoom = 3.0f;

			if (stateCnt > 120)
				return	false;
			return	true;
			break;
		}

		case	MET_BLUE_EFFECT:
			{
			float	stateCnt = GetStateCnt();
			int alphaCnt = 0;
			m_curAction = m_baseAction;
			m_curMotion = 0;

			m_pos.y = m_orgPosY - float(stateCnt)*0.1f - 18.0f;
			if (stateCnt > 90)
			{
				alphaCnt = (stateCnt - 90) * 8;
			}

			if (alphaCnt>210)
				SetArgb(0, -1, -1, -1);
			else
				SetArgb(210 - alphaCnt, 150, 150, 255);

			if (m_zoom < 2.0f)
				m_zoom += float(stateCnt)*0.15f;
			else if (m_zoom > 2.0f)
				m_zoom = 2.0f;

			if (stateCnt > 120)
				return	false;

			return	true;
			break;
		}























		case	MET_CRITICAL_EFFECT:
			{
			float	stateCnt = GetStateCnt();
			m_curAction = m_baseAction;
			if (m_destPos.x == 0.0f && m_masterActor) {
				int i = m_masterActor->m_roty;
				if (i >= 0 && i < 90) {
					i = 0;
				} else if (i >= 90 && i < 180) {
					i = 2;
				} else if (i >= 180 && i < 270) {
					i = 4;
				} else {
					i = 6;
				}
				const float mov = 0.5f;
				switch(i) {
					case 0:
					case 1:
						m_destPos.x = -mov;
						m_destPos.z = mov;
						break;
					case 2:
					case 3:
						m_destPos.x = -mov;
						m_destPos.z = -mov;
						break;
					case 4:
					case 5:
						m_destPos.x = mov;
						m_destPos.z = -mov;
						break;
					case 6:
					case 7:
						m_destPos.x = mov;
						m_destPos.z = mov;
						break;
					default:
						m_destPos.x = 0.0f;
						m_destPos.z = 0.0f;
						break;
				}
			}
			m_pos.x = m_destPos2.x + (m_destPos.x * (stateCnt/3)) + (m_destPos.x * 3);
			m_pos.z = m_destPos2.z + (m_destPos.z * (stateCnt/3)) + (m_destPos.z * 3);
			m_pos.y = (m_orgPosY  + 8)- stateCnt * (2.0f - (stateCnt / 30)) + m_destPos.y - 3;
			if (m_pos.y > 0) {
				m_destPos.y = m_pos.y / (m_pos.y * 3 / 4);
			} else {
				m_destPos.y = -m_pos.y / (-m_pos.y * 3 / 4);
			}

			if (m_zoom > 1.0f) {
				m_zoom = m_orgZoom - float(stateCnt)*0.24f*(3.0f/5.0f);
			} else if (m_zoom < 1.0f)
				m_zoom = 1.0f;

			CActRes *	actRes = GetActRes();


			if (m_curMotion < actRes->GetMotionCount(m_curAction)-1)
				m_curMotion = int(stateCnt/(2)) % actRes->GetMotionCount(m_curAction);

			SetArgb(255 - stateCnt*3.45f, -1, -1, -1);

			if (stateCnt > 70)
				return	false;
			return	true;

			break;
		}
















		case	MET_LUCKY_EFFECT:
		case	MET_JUST_ANIMATE:
		case	MET_EMOTION:
		case	MET_MVP:
			{
			CActRes *	actRes = GetActRes();
			float		stateCnt = GetStateCnt();
			int			motionCnt = actRes->GetMotionCount(m_curAction);

			if(motionCnt == 0) break;

			m_curMotion = int(stateCnt/m_motionSpeed) % motionCnt;

			if (m_msgEffectType==MET_EMOTION && m_stateId == 10000)
			{
				m_curMotion = 15;
				actRes->GetMotion(m_curAction, m_curMotion)->sprClips[0].flags = 0;
				SetArgb(255, 250, 80, 80);
				break;
			} else if ((stateCnt/m_motionSpeed)/(motionCnt) >= 1)
			{
				if (m_msgEffectType==MET_EMOTION && m_masterGid==g_session.GetAid())
				{
					CGameMode *		gameMode = g_modeMgr.GetGameMode();
					sAssert(gameMode);
					gameMode->SendMsg(MM_PLAYER_EMOTION_END);
				}
				return	false;
			}
			break;
		}
		case	MET_ICEBREAKEFFECT:
			{
			float		stateCnt = GetStateCnt();
			CActRes *	actRes = GetActRes();
			int			motionCnt = actRes->GetMotionCount(m_curAction);
			m_curMotion = int(stateCnt*1.2f) % (motionCnt);

			if ((stateCnt*1.2f) / motionCnt >= 1) {
				return	false;
			}
			break;
		}
		case	MET_PKEFFECT:
		case	MET_ICEEFFECT:
		case	MET_SLEEP_EFFECT:
		case	MET_STUN_EFFECT:
		case	MET_CURSE_EFFECT:
		case	MET_SILENCE_EFFECT:
		case	MET_CURSOR_EFFECT:
			{
			float		stateCnt = GetStateCnt();
			CActRes *	actRes = GetActRes();
			int			motionCnt = actRes->GetMotionCount(m_curAction);
			m_curMotion = int(stateCnt/m_motionSpeed) % motionCnt;
			break;
			}
		case	MET_CRUSIS_EFFECT:
		case	MET_ANGELUS_EFFECT:
			{
				if (m_masterActor)
				{
					int		stateCnt = GetStateCnt();
					if (stateCnt%34 == 0)
						m_masterActor->SendMsg(NULL, AM_MAKE_BLUR_ACTOR, 0);
				}
				break;
			}
		case	MET_BIRD: {
			if (m_isAttackTarget && m_masterActor)
			{
				vector3d &		Mpos = m_masterActor->m_pos;
				CalcDir(Mpos.x, Mpos.z);

				m_roty -= 180;
				if (m_roty < 0)
					m_roty += 360;
				int		dir = Get8Dir2(m_roty, TRUE);
				m_curAction = 8 + dir;

				m_pos.x += m_destPos2.x;
				if (!(m_stateCnt>10 && m_stateCnt<25))
				m_pos.y += m_destPos2.y;
				m_pos.z += m_destPos2.z;
				m_moveDist = m_destPos2.x;

				if (m_stateCnt >= 25)
				{
					m_destPos2.y *= -1.9f;
					m_isDisappear = TRUE;
				}
				CActRes *	actRes = GetActRes();
				m_curMotion = 0;
				m_oldAction = m_curAction;
				m_stateCnt++;
				break;
			}
			else
			{
				CActRes *	actRes = GetActRes();
				int			motCount = actRes->GetMotionCount(m_curAction);

				if (m_stateCnt % (int)m_motionSpeed == 0) {
					if (m_isAnimation)
						m_curMotion++;
					m_stateCnt = 0;
				}

				if (m_curMotion >= motCount)
					m_curMotion = 0;

				m_oldAction = m_curAction;
				m_stateCnt++;
				break;
			}
		}
		break;
		default:
			{
				if (m_msgEffectType == MET_NUMBER + 100)
				{
					float	stateCnt = GetStateCnt();
					m_curAction = m_baseAction;

					m_pos.y = m_orgPosY - float(stateCnt)*0.18f;

					SetArgb(250 - stateCnt*2, -1, -1, -1);
					if (m_zoom > 1.0f)
						m_zoom = m_orgZoom - float(stateCnt)*0.24f;
					else if (m_zoom<1.0f)
						m_zoom = 1.0f;

					if (stateCnt > 120)
						return	false;
					return	true;
				}
			break;
		}
	}
	return	true;
}

void	CMsgEffect :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	switch (message) {
		case	AM_REQ_ATTACK:
			if (m_msgEffectType == MET_BIRD) {
				vector3d *	pos = (vector3d *)arg1;
				m_destPos2.x = (pos->x - m_pos.x) / 10;
				m_destPos2.y = (pos->y - m_pos.y) / 10;
				m_destPos2.z = (pos->z - m_pos.z) / 10;
				m_isAttackTarget = TRUE;
				m_isAnimation = FALSE;
			} else
				CAbleToMakeEffect::SendMsg(sender, message, arg1, arg2, arg3);
			break;
		case	AM_DISAPPEAR:
			m_isDisappear = TRUE;
			break;
		case	AM_MSGMASTER_DEAD: {
			m_masterActor = NULL;
			switch (m_msgEffectType) {
				case	MET_ICEBREAKEFFECT:
				case	MET_NUMBER_CRITICAL:
				case	MET_NUMBER:
				case	MET_NUMBER_TOTAL:
				case	MET_NUMBER_HALF:
				case	MET_MISS:
				case	MET_CRITICAL_EFFECT:
				case	MET_BLUE_EFFECT:
				case	MET_LUCKY_EFFECT:
					break;
				default:
					m_isDisappear = TRUE;
					break;
			}
			break;
		}
			case	AM_SET_MASTER_POINTER: {
				m_masterActor = (CGameActor *)sender;
				switch (m_msgEffectType) {
					case	MET_CART:
					case	MET_BIRD:
						if (m_masterActor) {
							m_pos.x = m_masterActor->m_pos.x - 5*GetSin(m_masterActor->m_roty);
							m_pos.z = m_masterActor->m_pos.z + 5*GetCos(m_masterActor->m_roty);
						}
						break;
					case	MET_ICEBREAKEFFECT:
					if (m_masterActor) {
						m_pos = m_masterActor->m_pos;
					}
					break;
				default:
					break;
				}
				break;
			}
			case	AM_SHAKE_ICEEFFECT:
				m_masterActor = (CGameActor *)sender;
				if (m_masterActor) {
					m_pos.x = m_masterActor->m_pos.x + (rand() % 11 - 5) * 0.1f;
					m_pos.z = m_masterActor->m_pos.z + (rand() % 11 - 5) * 0.1f;
				}
				break;
			case	AM_SETMASTERGID: {
			m_masterGid = arg1;
			m_msgEffectType = arg2;
			switch (m_msgEffectType) {
				case	MET_CART:
					m_alwaysTopLayer = false;
					m_offsetOow = 0;
					m_shadowOn = true;
					break;
				case	MET_BIRD:
					m_alwaysTopLayer = false;
					m_offsetOow = 0;
					break;
				case	MET_ICEBREAKEFFECT:
				case	MET_ICEEFFECT:
					m_alwaysTopLayer = false;
					m_offsetOow = 5;
					break;
				case	MET_PKEFFECT:
					SetArgb(127, 255, 0, 0);
					break;
				case	MET_CURSE_EFFECT:
					SetArgb(127, -1, -1, -1);
					m_alwaysTopLayer = false;
					m_offsetOow = 5;
					break;
				}
			break;
		}
		case	AM_SETPOS2:
			{
				int	posy = 0;
				switch (m_msgEffectType)
				{
				case	MET_NUMBER_TOTAL:
						posy = 15 ;
					case	MET_NUMBER_HALF:
					case	MET_NUMBER:
					case	MET_NUMBER_CRITICAL:
						{

									SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbc\xfd\xc0\xda.spr");
									SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbc\xfd\xc0\xda.act");

							m_curAction = m_baseAction = 0;

							vector3d *	sPos = (vector3d *)arg1;
							m_pos = *sPos;
							m_pos.y -= (15 + posy);

							m_orgPosY = m_pos.y;
							m_isVisible = true;
							m_curMotion = arg2;
							sAssert(m_curMotion<=9 && m_curMotion>=0);
							if (!(m_curMotion<=9 && m_curMotion>=0))
								m_curMotion = 0;
							m_orgSprShift = m_sprShift = arg3;




							m_orgZoom = m_zoom = 5;
							m_offsetOow = -2;
							m_stateStartTick = bf_clock_ticks();
							SetArgb(0, -1, -1, -1);
							break;
						}
					case	MET_MISS:
						m_orgPosX = m_pos.x;
						break;
					default:
						{
							if (m_msgEffectType == MET_NUMBER + 100)
							{

								SetSprName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbc\xfd\xc0\xda.spr");
								SetActName("\xc0\xcc\xc6\xd1\xc6\xae\\\xbc\xfd\xc0\xda.act");

								m_curAction = m_baseAction = 0;

								vector3d *	sPos = (vector3d *)arg1;
								m_pos = *sPos;
								m_pos.y -= (12 + posy);

								m_orgPosY = m_pos.y;
								m_orgPosX = m_pos.x;
								m_isVisible = true;
								m_curMotion = arg2;
								sAssert(m_curMotion<=9 && m_curMotion>=0);
								if (!(m_curMotion<=9 && m_curMotion>=0))
									m_curMotion = 0;
								m_orgSprShift = m_sprShift = arg3;




									m_orgZoom = m_zoom = 5;
								m_offsetOow = -2;
								m_stateStartTick = bf_clock_ticks();
							}
							break;
						}
					}
				break;
		}
		case	AM_SETPOS:
			{
			m_isVisible = true;
			m_stateStartTick = bf_clock_ticks();
			if (arg2)
			{
				SprActName *	sprActName = (SprActName *)arg2;
				SetSprName(sprActName->sprName.c_str());
				SetActName(sprActName->actName.c_str());
			}
			if (arg3 == 1000)
			{
					SetAction(10, 4, 0);
					m_stateId = 10000;
					m_sprArgb = 0xffff0000;
			} else {
				SetAction(arg3, 4, MT_ONETIME);
			}
			vector3d *	sPos = (vector3d *)arg1;

			switch (m_msgEffectType) {
				case	MET_ICEBREAKEFFECT:
					m_pos = *sPos;
					break;
				case	MET_CART:
					break;








				case	MET_GUARD:
					m_pos = *sPos;
					m_pos.y -= 2;
					m_orgPosY = m_pos.y;
					m_curMotion = 0;
					m_orgZoom = m_zoom = 1;
					m_offsetOow = -5;
					break;
				case	MET_MISS:
					m_pos = *sPos;
					m_pos.y -= 12;
					m_orgPosY = m_pos.y;
					m_orgPosX = m_pos.x;
					m_curMotion = 0;
					m_orgZoom = m_zoom = 5;
					m_offsetOow = -5;
					break;
				case	MET_CRITICAL_EFFECT:
					m_pos = *sPos;
					m_pos.y -= 12;
					m_orgPosY = m_pos.y;
					m_curMotion = 1;
					m_orgZoom = m_zoom = 3;
					m_offsetOow = -5;
					m_sprArgb = 0xffaaaaaa;
					break;
				case	MET_BLUE_EFFECT:
					m_pos = *sPos;
					m_pos.y -= 12;
					m_orgPosY = m_pos.y;
					m_curMotion = 1;
					m_orgZoom = m_zoom = 3;
					m_offsetOow = -5;
					m_sprArgb = 0xffaaaaaa;
					break;
				case	MET_ANGELUS_EFFECT:
				case	MET_CRUSIS_EFFECT:
					m_curMotion = 1;
					m_isVisible = false;
					break;
				case	MET_BIRDEFFECT:
					m_pos = *sPos;
					m_pos.y -= 12;
					m_orgPosY = m_pos.y;
					m_curMotion = 0;
					m_offsetOow = -5;
					break;
				default:
					m_pos = *sPos;
					m_pos.y -= 20;
					break;
			}
			}
			break;
			default:
			CAbleToMakeEffect::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

CEffectMaker :: CEffectMaker()
{
	OnInit();
}


CEffectMaker :: ~CEffectMaker()
{
}

void	CEffectMaker :: OnInit()
{
	m_type = -1;
	m_emitSpeed = 60*6;
	m_processCnt = m_emitSpeed;
	m_param[0] = 0;
	m_param[1] = 0;
	m_param[2] = 0;
	m_param[3] = 0;
}

void	CEffectMaker :: OnExit()
{
}

bool	CEffectMaker :: OnProcess()
{
	CPlayer *	player = g_modeMgr.GetGameMode()->GetWorld()->m_player;
	vector3d &	pPos = player->m_pos;
	if (CalcDist(pPos.x, pPos.z) < 5*26) {
		if (m_processCnt >= m_emitSpeed) {
			m_processCnt = 0;
			CRagEffect *	effect = LaunchEffect(m_type);
			if (effect)
				effect->SendMsg(NULL, AM_SETPARAM, INT_PTR(m_param));
		}
	}
	++m_processCnt;
	return	true;
}

void	CEffectMaker :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	switch (message)
	{
		case	AM_SETPOS:
		{
			C3dWorldRes::effectSrcInfo *	info = (C3dWorldRes::effectSrcInfo *)arg1;

			m_type = info->type;
			switch (m_type)
			{
				case	45:
					m_emitSpeed = info->emitSpeed + random(info->emitSpeed/10) - info->emitSpeed/20;
					m_processCnt = random(info->emitSpeed+1);
					break;
				default:
					m_emitSpeed = info->emitSpeed;
					m_processCnt = info->emitSpeed-random(24);
					break;
			}
			m_pos = info->pos;
			m_param[0] = info->param[0];
			m_param[1] = info->param[1];
			m_param[2] = info->param[2];
			m_param[3] = info->param[3];
			break;
		}
		default:
			CAbleToMakeEffect::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

CMandraAttackEffect :: CMandraAttackEffect()
{
	SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\mandragora_atk.spr");
	SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\mandragora_atk.act");
	m_hideTime = 0;
}

CMandraAttackEffect :: ~CMandraAttackEffect()
{
}

void	CMandraAttackEffect :: OnInit()
{
	SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\mandragora_atk.spr");
	SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\mandragora_atk.act");
	m_hideTime = 0;
}

void	CMandraAttackEffect :: OnExit()
{
}

bool	CMandraAttackEffect :: OnProcess()
{
	float	stateCnt = (bf_clock_ticks() - m_stateStartTick) / 24.0f;
	if (stateCnt < m_hideTime) {
		m_isVisible = false;
		return	true;
	}
	m_isVisible = true;
	stateCnt -= m_hideTime;

	if (!m_isMotionFinished) {
		CActRes *	actRes = GetActRes();
		int			motionCount = actRes->GetMotionCount(m_curAction);
		m_curMotion = int(stateCnt/m_motionSpeed) % motionCount;
		if ((stateCnt/m_motionSpeed)/motionCount >= 1) {
			m_curMotion = motionCount-1;
			m_isMotionFinished = true;
		}
	}

	if (m_isMotionFinished)
		return	false;
	return	true;
}

void	CMandraAttackEffect :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	switch (message) {
	case	AM_SETPOS: {
		switch (arg3)
		{
		case	JT_HYDRA:
			SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\hydra_atk.spr");
			SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\hydra_atk.act");
			break;
		case	JT_THA_ODIUM:
		case	JT_G_THA_ODIUM:
			SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\tha_odium_ef.spr");
			SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\tha_odium_ef.act");
			break;
		case	JT_DROSERA:
			SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\drosera_bullet.spr");
			SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\drosera_bullet.act");
			break;
		case	JT_MAVKA:
		case	JT_G_MAVKA:
			SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\MAVKA_BULLET.spr");
			SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\MAVKA_BULLET.act");
			break;
		case	JT_ENTWEIHEN:
		case	JT_G_ENTWEIHEN_R:
		case	JT_G_ENTWEIHEN_H:
		case	JT_G_ENTWEIHEN_M:
		case	JT_G_ENTWEIHEN_S:
			SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\ENTWEIHEN_BULLET.spr");
			SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\ENTWEIHEN_BULLET.act");
			break;
		default:
			SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\mandragora_atk.spr");
			SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\mandragora_atk.act");
			break;
		}

		vector3d	tPos;
		GetTargetPos(arg1, tPos);
		m_pos = tPos;

		m_isVisible = true;
		m_stateStartTick = bf_clock_ticks();
		CActRes *	actRes = GetActRes();
		m_motionSpeed = actRes->GetDelay(0);
		if (JT_HYDRA == arg3)
			m_motionSpeed = 2;
		if (arg2)
			m_hideTime = *((float *)arg2);
		break;
	}
	default:
		CRenderObject::SendMsg(sender, message, arg1, arg2, arg3);
		break;
	}
}

CArrowEffect :: CArrowEffect()
{
	OnInit();
}

CArrowEffect :: ~CArrowEffect()
{
}

void	CArrowEffect :: OnInit()
{


	m_baseAction = 0;
	m_sPos = m_pos;
	m_hideTime = 0;
	m_moveSpeed.Set(0, 0, 0);
	m_curMotion = 0;
	m_process = 0;
}

void	CArrowEffect :: OnExit()
{
}

bool	CArrowEffect :: OnProcess()
{
	float	stateCnt = (float)(bf_clock_ticks() - m_stateStartTick) / 24.0f;
	if (m_hideTime > stateCnt) {
		return	true;
	} else {
		if (m_isVisible == false) {
			m_isVisible = true;
			vector3d	tPos;
			if (m_targetGid) {
				if (!GetTargetPos(m_targetGid, tPos))
					tPos = m_tPos;
			} else {
				tPos = m_tPos;
			}

			tPos.y -= 10;
			m_moveSpeed = tPos-m_pos;
			m_moveSpeed.x /= 8;
			m_moveSpeed.y /= 8;
			m_moveSpeed.z /= 8;

			float	dist = CalcDist(tPos.x, tPos.y, tPos.z);
			if (dist < 40) {
				m_fastFactor = 40 / dist;
				m_moveSpeed *= m_fastFactor;
			}
			CalcDir(tPos.x, tPos.z);
			vector3d	dir = tPos - m_pos;
			dir.Normalize();
			dir *= 2.5;
			m_pos += dir;
		}

		stateCnt -= m_hideTime;
	}

	m_curAction = m_baseAction;


	m_process ++;

	if (m_process>8)
	{

		{
			m_curMotion += 1;
			if (m_curMotion >= GetActRes()->GetMotionCount(m_curAction))
			{
				m_curMotion = GetActRes()->GetMotionCount(m_curAction) - 1;
			}
		}
	}

	m_sprAngle = GetDir(m_roty);

	CGameMode *		gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);
	m_pos.x = m_sPos.x + m_moveSpeed.x*stateCnt;
	m_pos.y = m_sPos.y + m_moveSpeed.y*stateCnt;
	m_pos.z = m_sPos.z + m_moveSpeed.z*stateCnt;
	if (stateCnt > (8.0f/m_fastFactor))
		return	false;
	return	true;
}

void	CArrowEffect :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3, INT_PTR arg4)
{
	switch (message) {
		case	AM_SETPOS: {
			if (arg4 == JT_PARASITE || arg4 == JT_G_PARASITE)
			{
				SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\PARASITE_BULLET.spr");
				SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\PARASITE_BULLET.act");
			}
			else if (arg4 == JT_STONE_SHOOTER)
			{
				SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\STONE_SHOOTER_BULLET.spr");
				SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\STONE_SHOOTER_BULLET.act");
			}
			else if (arg4 == JT_WOOTAN_SHOOTER)
			{
				SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\WOOTAN_SHOOTER_BULLET.spr");
				SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\WOOTAN_SHOOTER_BULLET.act");
			}
			else if (arg4 == JT_LIVE_PEACH_TREE || arg4 == JT_G_LIVE_PEACH_TREE)
			{
				SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\PEACH_TREE_BULLET.spr");
				SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\PEACH_TREE_BULLET.act");
			}
			else if (arg4 == JT_EVIL_CLOUD_HERMIT || arg4 == JT_G_EVIL_CLOUD_HERMIT)
			{
				SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\GINSENG_BULLET.spr");
				SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\GINSENG_BULLET.act");
			}
			else if (arg4 == JT_POTON_CANON)
			{
				SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\CANON_BULLET.spr");
				SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\CANON_BULLET.act");
			}
			else if (arg4 == JT_POTON_CANON_1)
			{
				SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\CANON_BULLET_1.spr");
				SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\CANON_BULLET_1.act");
			}
			else if (arg4 == JT_POTON_CANON_2)
			{
				SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\CANON_BULLET_2.spr");
				SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\CANON_BULLET_2.act");
			}
			else if (arg4 == JT_POTON_CANON_3)
			{
				SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\CANON_BULLET_3.spr");
				SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\CANON_BULLET_3.act");
			}
			else if (arg4 == JT_LADY_TANEE)
			{
				SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\LADY_TANEE_BULLET.spr");
				SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\LADY_TANEE_BULLET.act");
			}
			else if (arg4 == JT_RAGGED_ZOMBIE)
			{
				SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\RAGGED_ZOMBIE_BULLET.spr");
				SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\RAGGED_ZOMBIE_BULLET.act");
			}
			else
			{
				SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\skel_archer_arrow.spr");
				SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\skel_archer_arrow.act");
			}

			m_fastFactor = 1;
			vector3d *	sPos = (vector3d *)arg1;
			m_pos = *sPos;
			m_pos.y -= 10;
			m_sPos = m_pos;
			m_isVisible = false;
			m_targetGid = arg2;
			GetTargetPos(m_targetGid, m_tPos);
			m_stateStartTick = bf_clock_ticks();
			if (arg3)
				m_hideTime = *((float *)arg3);
			break;
		}
		case	AM_SETPOS2: {


			SetSprName("\xb8\xf3\xbd\xba\xc5\xcd\\skel_archer_arrow.spr");
			SetActName("\xb8\xf3\xbd\xba\xc5\xcd\\skel_archer_arrow.act");

			m_fastFactor = 1;
			vector3d *	sPos = (vector3d *)arg1;
			m_pos = *sPos;
			m_pos.y -= 10;
			m_sPos = m_pos;

			vector3d *	tPos = (vector3d *)arg2;
			m_tPos = *tPos;
			m_tPos.y -= 10;

			m_isVisible = false;
			m_targetGid = 0;

			m_stateStartTick = bf_clock_ticks();
			if (arg3)
				m_hideTime = *((float *)arg3);
			break;
		}
		default:
			CRenderObject::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

bool	CDummyEffectLauncher :: OnProcess()
{
	float	stateCnt = (float)(bf_clock_ticks() - m_stateStartTick) / 24.0f;
	if (stateCnt > m_lastCnt)
		return false;
	else
		return true;
}

void	CDummyEffectLauncher :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3, INT_PTR arg4)
{
	switch (message) {
		case	AM_SETPOS: {
			vector3d *	sPos = (vector3d *)arg1;
			m_pos = *sPos;
			m_isVisible = false;
			m_stateStartTick = bf_clock_ticks();
			CRagEffect *	effect = LaunchEffect(arg2);
			if (arg3)
				m_lastCnt = arg3;
			else
				m_lastCnt = 5 * 60;

			if (arg3 == 333)
			{
				vector3d	pos;
				pos.x = arg4;
				if (effect)
					effect->SendMsg(NULL, AM_SETPOS, INT_PTR(&pos));
			}
			break;
		}
		default:
			CAbleToMakeEffect::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

CGroundScope :: CGroundScope()
{
	m_lastingTime = 0;
	m_scope_size = 1;
	m_rot = 0;
}


bool	CGroundScope :: OnProcess()
{
	if(m_lastingTime < bf_clock_ticks()) return false;

	return true;
}

void	CGroundScope :: Render(matrix &vtm)
{
	int attrX = m_ground_pos_x;
	int attrY = m_ground_pos_y;

	CGameMode * gameMode = g_modeMgr.GetGameMode();

	C3dGround* ground = g_modeMgr.GetGameMode()->GetWorld()->m_ground;

	attrX -= m_scope_size/2;
	attrY -= m_scope_size/2;

	vector2d uv[4];

	float half_scope_size = float(m_scope_size)/2;
	float sin_r = _sin(m_rot);
	float cos_r = _cos(m_rot);
	DWORD color = (DWORD)(sin_r * 40) * 0x1000000 + 0x80FFFFFF;
	for(int i=0; i<m_scope_size; ++i) {
		for(int j=0; j<m_scope_size; ++j) {

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

			uv[0].Set((uv[0].x+half_scope_size)/m_scope_size, (uv[0].y+half_scope_size)/m_scope_size);
			uv[1].Set((uv[1].x+half_scope_size)/m_scope_size, (uv[1].y+half_scope_size)/m_scope_size);
			uv[2].Set((uv[2].x+half_scope_size)/m_scope_size, (uv[2].y+half_scope_size)/m_scope_size);
			uv[3].Set((uv[3].x+half_scope_size)/m_scope_size, (uv[3].y+half_scope_size)/m_scope_size);

			ground->RenderGroundTile(vtm, x, y, color, g_texMgr.GetTexture("effect\\magic_target.tga", false), uv);
		}
	}

	m_rot += m_rot_speed;
}

void	CGroundScope :: SendMsg(CGameObject *sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	switch (message) {
		case	AM_SETMASTERGID:
			m_master_gid = arg1;
			break;
		case	AM_SETPOS:
			m_ground_pos_x = arg1;
			m_ground_pos_y = arg2;
			break;
		case	AM_SETTIME:
			m_lastingTime = arg1;
			break;
		case	AM_SET_SKILL:
			m_scope_size = GetScopeSize(arg1) + 2;
			m_rot_speed = 0.5f / m_scope_size;
			break;
		default:
			CAbleToMakeEffect::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}

int		CGroundScope :: GetScopeSize(int skill_id)
{

	switch(skill_id) {
	case	SKID_MG_NAPALMBEAT:
		return 3;
	case	SKID_MG_FIREBALL:
		return 5;
	case	SKID_MG_THUNDERSTORM:
		return 5;
	case	SKID_AC_SHOWER:
		return 3;
	case	SKID_AL_PNEUMA:
		return 3;
	case	SKID_WZ_FIREPILLAR:
		return 3;
	case	SKID_WZ_METEOR:
		return 9;
	case	SKID_WZ_VERMILION:
		return 11;
	case	SKID_WZ_WATERBALL:
		return 3;
	case	SKID_WZ_FROSTNOVA:
		return 3;
	case	SKID_WZ_STORMGUST:
		return 9;
	case	SKID_WZ_HEAVENDRIVE:
		return 5;
	case	SKID_WZ_QUAGMIRE:
		return 5;
	case	SKID_BS_HAMMERFALL:
		return 3;
	case	SKID_HT_BLITZBEAT:
		return 3;
	case	SKID_HT_DETECTING:
		return 3;
	case	SKID_PR_BENEDICTIO:
		return 3;
	case	SKID_PR_SANCTUARY:
		return 5;
	case	SKID_PR_MAGNUS:
		return 7;
	case	SKID_AS_GRIMTOOTH:
		return 3;
	case	SKID_AS_VENOMDUST:
		return 3;
	case	SKID_SN_FALCONASSAULT:
		return 3;
	case	SKID_CR_SLIMPITCHER:
		return 7;
	case	SKID_HW_GANBANTEIN:
		return 3;
	case	SKID_HW_GRAVITATION:
		return 5;
	default:
		return 0;
	}
}
