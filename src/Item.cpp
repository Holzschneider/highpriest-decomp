#include "common.h"
#include "packet.h"
#include "Picker.h"
#include "Session.h"
#include "GameMode.h"
#include "GameActor.h"
#include "MsgStrings.h"
#include "resource/World.h"
#include "GameWorld.h"
#include "ItemInfo.h"
#include "resource/Attr.h"
#include "resource/Ground.h"

extern CModeMgr g_modeMgr;
extern CRenderer* g_renderer;
extern CSession g_session;
const char* MsgStr(MSGSTRINGID id);

CItem::CItem()
{
	OnInit();
}

CItem::~CItem()
{
	OnExit();
}

void CItem::OnInit()
{
	m_shouldAddPickInfo = true;
	m_zoom = 1.0f;
	m_shadowOn = true;
	m_colorOfSingleColor = 0xffffff00;
}

void CItem::OnExit()
{
}

void CItem::Render(matrix& vtm)
{
	if (m_isVisible) {
		RenderShadow(vtm, m_zoom * 0.4f);
		RenderBody(vtm, 0);
	}
}

bool	CItem :: OnProcess()
{
	float	stateCnt = (timeGetTime() - m_stateStartTick) / 24.f;
	if ((int(stateCnt)%92) >= 90)
		m_isSingleColor = 1;
	else
		m_isSingleColor = 0;

	if (m_isJumping) {
		m_pos.y = m_sPosY + (m_sfallingSpeed + 0.083f*stateCnt) * stateCnt;
		CGameMode *		gameMode = g_modeMgr.GetGameMode();
		sAssert(gameMode);
		if (m_pos.y > gameMode->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z)) {
			m_pos.y = gameMode->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z);
			m_isJumping = false;
		}
	}

	return	true;
}

void	CItem :: SetRenderInfo(RENDER_INFO_RECT &info, const float tlvertX, const float tlvertY)
{
	if (m_shouldAddPickInfo) {
		static int	minRectLength = 34 * g_renderer->GetScreenXFactor();
		float		oowOffset = 300;

		CActorPickInfo *	actorPickInfo = new CActorPickInfo;
		actorPickInfo->m_vectors[0].x	= (info.left <= info.right) ? info.left : info.right;
		actorPickInfo->m_vectors[0].y	= info.top;
		actorPickInfo->m_vectors[0].z	= info.oow + oowOffset;
		actorPickInfo->m_vectors[1].x	= (info.left > info.right) ? info.left : info.right;
		actorPickInfo->m_vectors[1].y	= info.bottom;
		actorPickInfo->m_vectors[1].z	= info.oow + oowOffset;
		actorPickInfo->m_gid			= m_aid;
		actorPickInfo->SetJob(JT_LAST);
		actorPickInfo->m_isPkState		= 0;
		actorPickInfo->m_classType		= CT_CITEM;

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

void CItem::SendMsg(CGameObject* sender, int message, INT_PTR arg1, INT_PTR arg2, INT_PTR arg3)
{
	CGameMode* gameMode = g_modeMgr.GetGameMode();
	sAssert(gameMode);
	switch (message) {
		case AM_INITSTANDENTRY: {
			ITEMSTANDENTRYSTRUCT* s = (ITEMSTANDENTRYSTRUCT*)arg1;
			m_aid = s->ITAID;
			m_itemName = s->name;
			std::string orgItemName = m_itemName;

			gameMode->GetClientCoor(s->x, s->y, s->subX, s->subY, m_pos.x, m_pos.z);

			m_pos.y = gameMode->GetWorld()->m_attr->GetHeight(m_pos.x, m_pos.z);
			if (s->isJumpEntry) {
				m_pos.y -= 15;
				m_isJumping = true;
				m_sfallingSpeed = -0.6f;
			} else {
				m_isJumping = false;
			}
			m_sPosY = m_pos.y;

			std::string sprName = g_session.GetItemSpriteFileName(m_itemName.c_str(), s->isIdentified);
			sprName += ".spr";
			std::string actName = g_session.GetItemSpriteFileName(m_itemName.c_str(), s->isIdentified);
			actName += ".act";

			SetSprName(sprName.c_str());
			SetActName(actName.c_str());

			char itemHelp[128];
			char itemName[64];
			ITEM_INFO dummyItemInfo;

			dummyItemInfo.m_itemName = orgItemName;
			dummyItemInfo.m_isIdentified = s->isIdentified;
			dummyItemInfo.m_refiningLevel = 0;
			dummyItemInfo.GetItemName(itemName);

			sprintf(itemHelp, MsgStr(MSI_EA2), itemName, s->count);

			m_itemName = itemHelp;

			GetActRes();
			m_stateStartTick = timeGetTime();
			break;
		}
		default:
			CRenderObject::SendMsg(sender, message, arg1, arg2, arg3);
			break;
	}
}
