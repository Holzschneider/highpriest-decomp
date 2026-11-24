#include "common.h"
#include "GameMode.h"
#include "MsgStrings.h"
#include "framework/Locale.h"
#include "device/Sound.h"
#include "resource/Wave.h"
#include "GameActor.h"
#include "GameWorld.h"
#include "ItemInfo.h"
#include "Mode.h"
#include "Session.h"
#include "SkillInfo.h"
#include "window/UISys.h"
#include "View.h"
#include "base/File.h"
#include "device/Connection.h"
#include "window/UIFrameWnd.h"
#include "framework/Language.h"
#include "GuildInfo.h"
#include "Insult.h"
#include <string>
#include "resource/Ground.h"
using std::string;

#include "pdb_enums.h"


extern CSession   g_session;
extern CModeMgr   g_modeMgr;
extern UIWindowMgr g_windowMgr;
extern ServiceType g_serviceType;
extern ServerType  g_serverType;
#include "packet.h"
#include "PaddingPacket.h"
extern int        g_mustPumpOutReceiveQueue;
extern int        g_loginStartMode;
extern int        g_isGoodSkillHanging;

int g_isGoodSkillHanging;
extern std::vector<int> stateId;
extern const char* MsgStr(MSGSTRINGID id);
extern const char* MsgStrToCompare(MSGSTRINGID id);
extern char g_MessageHi;
extern char*      DataTxtDecode(char* str);
extern bool       IsEffectStateBurrow(unsigned int state);
extern bool       IsEffectStatePushCart(unsigned int state);
extern bool       IsNameYellow(DWORD aid);
extern bool       IsGravityAid(DWORD aid);
extern CRenderer* g_renderer;
bool IsSameSentence(int nRepeat);
#define g_connection (CRagConnection::instanceR())


#ifndef NAME_LENGTH
#define NAME_LENGTH 24
#endif
#ifndef MAPNAME_LENGTH
#define MAPNAME_LENGTH 16
#endif
inline void SetMustPumpOutReceiveQueue(int v) { g_mustPumpOutReceiveQueue = v; }

#pragma pack(push, 2)
struct PACKET_CZ_INPUT_EDITDLGSTR {
	short PacketType;
	unsigned short PacketLength;
	unsigned int NAID;
};
struct PRODUCTINFO_INSTORE {
	short index;
	short count;
	int price;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct PACKET_CZ_REQ_OPENSTORE2 {
	short PacketType;
	short PacketLength;
	char storeName[80];
	bool result;
};
#pragma pack(pop)




#pragma pack(1)

bool IsSameSentence(int nRepeat)
{
	if (g_serverType != ServerInstantEvent && g_serviceType != ServiceGermany && nRepeat >= 2)
	{
		g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
		g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_NO_SAME_SENTANCE), 0xff, 0, 0);
		g_windowMgr.ClearChatMsg();
		return true;
	}
	return false;
}

#include "rag_budget_fill.h"
#include "rag_budget_sites.h"
INT_PTR CGameMode::SendMsg(int messageId, INT_PTR val1, INT_PTR val2, INT_PTR val3)
{ RAG_BUDGET_FILL(122)
	switch (messageId)
	{
	case MM_QUEUE_REQ_GUILD_EMBLEM_IMG: {
		m_emblemReqGdidQueue.push_back(val1);
		break;
	}
	case MM_QUAKE:
		if (val1 == 1)
			m_view->SetQuake(TRUE, 1, 1.0f, 0.24f, 0.2f);
		else
			m_view->SetQuake(TRUE, 0, 1.0f, 0.24f, 0.2f);
		break;
	case MM_SETQUAKEINFO: {
		float *v = (float *)val1;
		float *v1 = (float *)val2;
		float *v2 = (float *)val3;
		m_view->SetQuakeInfo(*v, *v1, *v2);
		break;
	}
	case MM_SHOW_IMAGE: {
		string imageName = (char *)val1;
		int		type = val2;
		m_showImageInfo.type = type;
		m_showImageInfo.imageName = imageName;
		if (m_showImageInfo.imageName.find(".bmp") == string::npos &&
			m_showImageInfo.imageName.find(".BMP") == string::npos)
			m_showImageInfo.imageName += ".bmp";
		break;
	}
	case MM_GET_SKILL_USE_LEVEL:           return m_skillUseInfo.m_useLevel;
	case MM_GET_SKILL_USE_TYPE:            return m_skillUseInfo.m_skillUseType;
	case MM_USESKILL_LEVEL_MINUS: {
		if (SKILL_INFO::IsLevelUseSkill(m_skillUseInfo.m_skillId)) {
			SKILL_INFO skillInfo = g_session.GetSkillItemInfoBySkillId(m_skillUseInfo.m_skillId);
			if (skillInfo.m_isValid) {
				(m_skillUseInfo.m_useLevel) -= val1;
				if (m_skillUseInfo.m_useLevel > skillInfo.level)
					m_skillUseInfo.m_useLevel = skillInfo.level;
				if (m_skillUseInfo.m_useLevel <= 0)
					m_skillUseInfo.m_useLevel = 1;
			}
		}
		break;
	}
	case MM_USESKILL_LEVEL_PLUS: {
		if (SKILL_INFO::IsLevelUseSkill(m_skillUseInfo.m_skillId)) {
			SKILL_INFO skillInfo = g_session.GetSkillItemInfoBySkillId(m_skillUseInfo.m_skillId);
			if (skillInfo.m_isValid) {
				(m_skillUseInfo.m_useLevel) += val1;
				if (m_skillUseInfo.m_useLevel > skillInfo.level)
					m_skillUseInfo.m_useLevel = skillInfo.level;
				if (m_skillUseInfo.m_useLevel <= 0)
					m_skillUseInfo.m_useLevel = 1;
			}
		}
		break;
	}
	case MM_MAKE_SKILL_USED_MSG: {
		m_skillUsedTick = timeGetTime();

		int skillId = val1;
		BOOL isDrawName = TRUE;
		if (val3 < 0 || skillId < 0)
			isDrawName = FALSE;
		if (val1 == 389)
			isDrawName = FALSE;

			CGameActor *sActor = (CGameActor *)val2;
			if (!sActor || !isDrawName)
				break;

			sAssert(skillId >= 0 && skillId < SKID_COMM_END);

			char skillName[64];
			NamePair name;
			name.Clear(); RAG_BUDGET_SITES(7)
			char charName[64];

			if (val1 == 334 || val1 == 335) {
				name = g_modeMgr.GetGameMode()->GetActorName(val3);
				name.cName.c_str();
				strcpy(charName, name.cName.c_str());

				strcpy(skillName, SKILL_INFO::GetSkillNameBySkillId(skillId));
				if (g_serviceType == ServiceKorea)
					strcat(skillName, " \xa2\xbd");
				if (charName != NULL)
					strcat(charName, " !!  ");
				strcat(charName, MsgStr(MSI_LOVE_SKILL));

			sActor->SendMsg(NULL, 7, INT_PTR(charName), 0, 0);
		} else if (val1 == 336) {
			wsprintf(charName, "%s", m_CoupleName);

			strcpy(skillName, SKILL_INFO::GetSkillNameBySkillId(skillId));
			if (g_serviceType == ServiceKorea)
				strcat(skillName, " \xa2\xbd");

			strcat(charName, " !!  ");
			strcat(charName, skillName);

			sActor->SendMsg(NULL, 7, INT_PTR(charName), 0, 0);
		} else {
			if (val1 == 326 || val1 == 318 || val1 == 492) {
			} else {
					strcpy(skillName, SKILL_INFO::GetSkillNameBySkillId(skillId));
					strcat(skillName, " !!");
					sActor->SendMsg(NULL, 7, INT_PTR(skillName), 0, 0);
				}
		}
		break;
	}
	case MM_TOGGLE_PLAYERGAGE:
		{
			if (m_playerGage) {
				g_windowMgr.PostQuit(m_playerGage);
				m_playerGage = NULL;
				break;
			}
			m_playerGage = new UIPlayerGage();
			m_playerGage->m_isPlayerType = 0;
			m_playerGage->Create(60, 9);
			CPlayer *player = m_world->m_player;
			m_playerGage->Move(
				player->m_lastTlvertX - 30,
				(int)((float)(g_renderer->m_height * 12 / 480) * player->m_lastPixelRatio + player->m_lastTlvertY));
			g_windowMgr.AddWindowFront(m_playerGage);
		}
		break;
	case MM_REQ_SELL: {
		g_windowMgr.DeleteWindow(WID_ITEMSHOPWND);
		g_windowMgr.DeleteWindow(WID_ITEMSELLWND);
		{
			int		itemSize = g_session.GetNumSellItem();
			int		packetStrLen = sizeof(PACKET_CZ_PC_SELL_ITEMLIST);
			int		packetLength = packetStrLen+sizeof(CZ_SELL_ITEM)*itemSize;
			char	packet[2048];
			sAssert(packetLength<=sizeof(packet));
			if (!(packetLength<=sizeof(packet)))
				break;

			PACKET_CZ_PC_SELL_ITEMLIST p;
			p.PacketType	= HEADER_CZ_PC_SELL_ITEMLIST;
			p.PacketLength	= packetLength;
			memcpy(packet, &p, packetStrLen);

			for (int i=0; i<itemSize; i++) {
				ITEM_INFO itemInfo = g_session.GetSellItemInfoBySequence(i);
				CZ_SELL_ITEM pitem;
				pitem.count = itemInfo.m_num;
				pitem.index = itemInfo.m_itemIndex;
				memcpy(packet + packetStrLen + i*sizeof(CZ_SELL_ITEM), &pitem, sizeof(CZ_SELL_ITEM));
			}
			if (itemSize) {
				if (!m_isPlayerDead)
					g_connection.SendPacket(p.PacketLength, (const char *)(&packet[0]));
			}
		}
		g_session.ClearDealItems();
		break;
	}
	case MM_REQ_PURCHASE: {
		g_windowMgr.DeleteWindow(WID_ITEMSHOPWND);
		g_windowMgr.DeleteWindow(WID_ITEMPURCHASEWND);
		{
			int		itemSize = g_session.GetNumPurchaseItem();
			int		packetStrLen = sizeof(PACKET_CZ_PC_PURCHASE_ITEMLIST);
			int		packetLength = packetStrLen+sizeof(CZ_PURCHASE_ITEM)*itemSize;
			char	packet[2048];
			sAssert(packetLength<=sizeof(packet));
			if (!(packetLength<=sizeof(packet)))
				break;

			PACKET_CZ_PC_PURCHASE_ITEMLIST p;
			p.PacketType	= HEADER_CZ_PC_PURCHASE_ITEMLIST;
			p.PacketLength	= packetLength;
			memcpy(packet, &p, packetStrLen);

			for (int i=0; i<itemSize; i++) {
				ITEM_INFO itemInfo = g_session.GetPurchaseItemInfoBySequence(i);

				CZ_PURCHASE_ITEM pitem;
				pitem.count = itemInfo.m_num;
				pitem.ITID	= atoi(itemInfo.m_itemName.c_str());
				memcpy(packet + packetStrLen + i*sizeof(CZ_PURCHASE_ITEM), &pitem, sizeof(CZ_PURCHASE_ITEM));
			}
			if (itemSize) {
				if (!m_isPlayerDead)
					g_connection.SendPacket(p.PacketLength, (const char *)(&packet[0]));
			}
		}
		g_session.ClearDealItems();
		break;
	}
	case MM_RELEASE_QUEST:
		m_isOnQuest = false;
		g_windowMgr.DeleteWindow(WID_SAYDIALOGWND);
		g_windowMgr.DeleteWindow(WID_CHOOSEWND);
		g_windowMgr.DeleteWindow(WID_CHOOSESELLBUYWND);
		g_windowMgr.DeleteWindow(WID_ITEMSHOPWND);
		g_windowMgr.DeleteWindow(WID_ITEMPURCHASEWND);
		g_windowMgr.DeleteWindow(WID_ITEMSELLWND);
		g_windowMgr.DeleteWindow(WID_MERCHANTITEMSHOPWND);
		g_windowMgr.DeleteWindow(WID_MERCHANTITEMPURCHASEWND);
		g_windowMgr.DeleteWindow(WID_ITEMCOLLECTIONWND);
		g_windowMgr.DeleteWindow(WID_ITEMPARAMCHANGEDISPLAYWND);
		SendMsg(MM_SHOW_IMAGE, (INT_PTR)"", -1, 0);
		break;
	case MM_RELEASE_QUEST_SH:
		m_isOnQuest = false;
		g_windowMgr.DeleteWindow(WID_MERCHANTITEMSHOPWND);
		g_windowMgr.DeleteWindow(WID_MERCHANTITEMPURCHASEWND);
		g_windowMgr.DeleteWindow(WID_ITEMCOLLECTIONWND);
		break;
	case MM_ACK_SELL: {
		PACKET_CZ_ACK_SELECT_DEALTYPE p; p.PacketType = HEADER_CZ_ACK_SELECT_DEALTYPE;
		p.NAID = val1;
		p.type = 1;
		if (m_isPlayerDead) break;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_ACK_SELECT_DEALTYPE), (const char *)&p);
		break;
	}
	case MM_ACK_BUY: {
		PACKET_CZ_ACK_SELECT_DEALTYPE p; p.PacketType = HEADER_CZ_ACK_SELECT_DEALTYPE;
		p.NAID = val1;
		p.type = 0;
		if (m_isPlayerDead) break;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_ACK_SELECT_DEALTYPE), (const char *)&p);
		break;
	}
	case MM_PLAYER_EMOTION_END:
		m_isReqEmotion = false;
		break;
	case MM_REFRESH_MUSIC: {
		if (!g_session.m_bgmIsPaused)
			PlayStream(m_streamFileName.c_str());
		else
			CloseStream();
		break;
	}
	case MM_REQ_NUM_USER: {
		if (g_serviceType == ServiceChina || g_serviceType == ServiceMalaysia || g_serviceType == ServiceSingapore)
			break;
		PACKET_CZ_REQ_USER_COUNT p;
		p.PacketType = HEADER_CZ_REQ_USER_COUNT;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_USER_COUNT), (const char *)&p);
		break;
	}
	case MM_HOMUN_WAIT: {
		CGameActor * actor = GetWorld()->GetGameActorByAID(g_session.m_homunGID);
		if (actor && actor->HomunAIIsOpened())
		{
			_MSG2AI	msg;
			msg.command = FOLLOW_CMD;
			actor->SetMsg2AI(msg, 0);
		}
		break;
	}
	case MM_REQ_BLACKSMITH: {
		PACKET_CZ_BLACKSMITH_RANK p; p.PacketType = HEADER_CZ_BLACKSMITH_RANK;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_BLACKSMITH_RANK), (const char *)&p);
		break;
	}
	case MM_REQ_ALCHEMIST: {
		PACKET_CZ_ALCHEMIST_RANK p; p.PacketType = HEADER_CZ_ALCHEMIST_RANK;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_ALCHEMIST_RANK), (const char *)&p);
		break;
	}
	case MM_REQ_KILLER_RANK: {
		PACKET_CZ_BLACKSMITH_RANK p;
		p.PacketType = HEADER_CZ_KILLER_RANK;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_KILLER_RANK), (const char *)&p);
		break;
	}
	case MM_REQ_TAEKWON: {
		PACKET_CZ_TAEKWON_RANK p; p.PacketType = HEADER_CZ_TAEKWON_RANK;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_TAEKWON_RANK), (const char *)&p);
		break;
	}
	case MM_REQ_MUNAK: {
		PACKET_CZ_GANGSI_RANK p; p.PacketType = HEADER_CZ_GANGSI_RANK;
		p.PacketSwitch = JT_GANGSI;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_GANGSI_RANK), (const char *)&p);
		break;
	}
	case MM_REQ_DEATHKNIGHT: {
		PACKET_CZ_GANGSI_RANK p; p.PacketType = HEADER_CZ_GANGSI_RANK;
		p.PacketSwitch = JT_DEATHKNIGHT;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_GANGSI_RANK), (const char *)&p);
		break;
	}
	case MM_REQ_COLLECTOR: {
		PACKET_CZ_GANGSI_RANK p; p.PacketType = HEADER_CZ_GANGSI_RANK;
		p.PacketSwitch = JT_COLLECTOR;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_GANGSI_RANK), (const char *)&p);
		break;
	}
	case MM_REQ_EMOTION: {
		if (!m_isReqEmotion && timeGetTime() > m_reqEmotionTick + 2500) {
			m_reqEmotionTick = timeGetTime();
			m_isReqEmotion = true;
			PACKET_CZ_REQ_EMOTION p;
			p.PacketType = HEADER_CZ_REQ_EMOTION;
			p.type = val1;
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_EMOTION), (const char *)&p);
		}
		break;
	}
	case MM_STATUS_CHANGE: {
		if (!m_waitingReqStatusAck) {
			m_waitingReqStatusAck = true;
			PACKET_CZ_STATUS_CHANGE p;
			p.PacketType = HEADER_CZ_STATUS_CHANGE;
			p.statusID = val1;
			p.changeAmount = val2;
			if (!m_isPlayerDead)
				g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_STATUS_CHANGE), (const char *)&p);
		}
		break;
	}
	case MM_REQ_STATUS: {
		if (!m_waitingReqStatusAck) {
			m_waitingReqStatusAck = true;
			PACKET_CZ_REQ_STATUS p;
			p.PacketType = HEADER_CZ_REQ_STATUS;
			if (!m_isPlayerDead)
				g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_STATUS), (const char *)&p);
		}
		break;
	}
	case MM_SETLOCKONMOUSE:
		m_isMouseLockOn = val1;
		if (val1 == 0) {
			m_lastPcGid = 0;
			m_lastMonGid = 0;
		}
		break;
	case MM_QUIT_GAME:
		SendMsg(MM_REQ_DISCONNECT, 0, 0, 0);
		break;
	case MM_RESTART: {
		PACKET_CZ_RESTART p;
		p.PacketType = HEADER_CZ_RESTART;
		p.type = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_RESTART), (const char *)&p);
		break;
	}
			case MM_INITWHISPERMENU: {
				m_lastWhisperMenuCharacterName = (const char *)val1;
				m_menuIdList.clear();
				m_menuIdList.push_back(MENUID_WRITE_CHAT_TO_FILE);
				m_menuIdList.push_back(MENUID_ADD_TO_FRIEND_CHAR_LIST_FROM_WINDOW);
				break;
		}
	case MM_MENUSELECTED: {
		if (g_windowMgr.m_menu != NULL) {
			g_windowMgr.DeleteWindow(WID_MENUWND);
		}
		if (g_windowMgr.m_itemStoreWnd != NULL)
			break;
		switch (m_menuIdList[val1]) {
		case MENUID_DISCONNECT_CHARACTER:
			SendMsg(MM_DISCONNECT_CHARACTER, m_menuTargetAID, 0, 0);
			break;
		case MENUID_DEAL: {
			CGameMode *gameMode = g_modeMgr.GetGameMode();
			CGameActor *actor = gameMode->GetWorld()->m_player;
			if (actor->GetEffectState() != 2) {
				SendMsg(MM_REQ_EXCHANGE_ITEM, m_menuTargetAID, 0, 0);
			}
			break;
		}
		case MENUID_INVITE_IN_GUILD:
			SendMsg(MM_REQ_JOIN_GUILD, m_menuTargetAID, 0, 0);
			break;
		case MENUID_ALLY_GUILD:
			SendMsg(MM_REQ_ALLY_GUILD, m_menuTargetAID, 0, 0);
			break;
		case MENUID_HOSTILE_GUILD:
			SendMsg(MM_REQ_HOSTILE_GUILD, m_menuTargetAID, 0, 0);
			break;
		case MENUID_INVITE_IN_PARTY:
			SendMsg(MM_REQ_JOIN_GROUP, INT_PTR(m_actorNameList[m_menuTargetAID].cName.c_str()), 0, 0);
			break;
		case MENUID_INVITE_IN_BABY:
			SendMsg(MM_REQ_JOIN_BABY, m_menuTargetAID, 0, 0);
			break;
		case MENUID_ADD_TO_FRIEND_CHAR_LIST:
			g_windowMgr.AddFriendCharName(m_actorNameList[m_menuTargetAID].cName.c_str());
			break;
		case MENUID_MAKE_WHISPER_WINDOW:
			if (g_windowMgr.IsFriendName(m_actorNameList[m_menuTargetAID].cName.c_str()))
				g_windowMgr.SendMsg(UIM_MAKE_WHISPER_WINDOW, (INT_PTR)m_actorNameList[m_menuTargetAID].cName.c_str(), (INT_PTR)MsgStr(MSI_FRIEND), 0, 0);
			else if (g_session.IsGuildMember(m_actorNameList[m_menuTargetAID].cName.c_str()))
				g_windowMgr.SendMsg(UIM_MAKE_WHISPER_WINDOW, (INT_PTR)m_actorNameList[m_menuTargetAID].cName.c_str(), (INT_PTR)"GuildMember", 0, 0);
			else
				g_windowMgr.SendMsg(UIM_MAKE_WHISPER_WINDOW, (INT_PTR)m_actorNameList[m_menuTargetAID].cName.c_str(), 0, 0, 0);
			break;
			case MENUID_REQ_EX_LIST: {
				ProcessTalkType(TT_REQ_WHISPER_PC_EX, (INT_PTR)&m_actorNameList[m_menuTargetAID]);
				g_session.AddToExNameList(m_actorNameList[m_menuTargetAID].cName.c_str());
				g_session.AddToExAidList(m_menuTargetAID);
				break;
			}
			case MENUID_REQ_IN:
				ProcessTalkType(TT_REQ_WHISPER_PC_IN, (INT_PTR)&m_actorNameList[m_menuTargetAID]);
				g_session.DeleteFromExNameList(m_actorNameList[m_menuTargetAID].cName.c_str());
				g_session.DeleteFromExAidList(m_menuTargetAID);
				break;
			case MENUID_WRITE_CHAT_TO_FILE: {
				const string& name = m_lastWhisperMenuCharacterName;
				g_windowMgr.WriteWhisperChatToFile(name.c_str());
				break;
			}
			case MENUID_ADD_TO_FRIEND_CHAR_LIST_FROM_WINDOW: {
				const string& name = m_lastWhisperMenuCharacterName;
				g_windowMgr.AddFriendCharName(name.c_str());
				break;
			}
		case MENUID_REQ_MANNER_POINT_PLUS: {
			if (!IsNameYellow(g_session.GetAid()))
				break;
			UICntWnd *w = new UICntWnd();
			w->Create(182, 46);
				w->Move(UIX(200), UICY(200));
				g_windowMgr.AddWindow(w);
				w->SendMsg(0, UM_SET_TARGET, MM_REQ_GIVE_MANNER_POINT_PLUS, 0, 0, 0);
				w->SendMsg(0, UM_SETTEXT, (INT_PTR)MsgStr(MSI_GIVE_PLUS_MANNER_POINT), 0, 0, 0);
				w->SendMsg(0, UM_SET_COUNT, 1, 0, 0, 0);
			break;
		}
		case MENUID_REQ_MANNER_POINT_MINUS: {
			if (!IsGravityAid(g_session.GetAid()))
				break;
			UICntWnd *w = new UICntWnd();
			w->Create(182, 46);
				w->Move(UIX(200), UICY(200));
				g_windowMgr.AddWindow(w);
				w->SendMsg(0, UM_SET_TARGET, MM_REQ_GIVE_MANNER_POINT_MINUS, 0, 0, 0);
				w->SendMsg(0, UM_SETTEXT, (INT_PTR)MsgStr(MSI_GIVE_MINUS_MANNER_POINT), 0, 0, 0);
				w->SendMsg(0, UM_SET_COUNT, 1, 0, 0, 0);
			break;
		}
			case MENUID_SHOW_CHAT_PROHIBIT_LOG: {
				UIFrameWnd *w = g_windowMgr.MakeWindow(WID_PROHIBITLISTWND);
				w->SendMsg(0, UM_SETINFO, m_menuTargetAID, 0, 0, 0);
				return 0;
			}
		case MENUID_PET_FEEDING:
			if (g_windowMgr.ErrorMsg(MsgStr(MSI_SURE_TO_FEED_PET), ME_YESNO, 0, 0, 0) == ID_YES) {
				SendMsg(MM_COMMAND_PET, 1, 0, 0);
			}
			break;
		case MENUID_PET_PERFORMANCE:
			SendMsg(MM_COMMAND_PET, 2, 0, 0);
			break;
		case MENUID_PET_ACC_OFF:
			SendMsg(MM_COMMAND_PET, 4, 0, 0);
			break;
		case MENUID_PET_RETURN_EGG: {
			SendMsg(MM_COMMAND_PET, 3, 0, 0);
			ITEM_INFO info = g_session.GetItemInfoByIndex(g_session.m_petEggIndex);
			info.m_isDamaged = false;
			g_session.SetItem(&info);
			g_session.m_petEggIndex = -1;
			g_session.m_petJob = -1;
				UIItemWnd *itemWnd = g_windowMgr.GetItemWnd();
				if (itemWnd) {
					itemWnd->Invalidate();
				}
			g_windowMgr.DeleteWindow(WID_PETINFOWND);
			break;
		}
		case MENUID_PET_SHOWINFO:
			SendMsg(MM_COMMAND_PET, 0, 0, 0);
			if (!g_windowMgr.DeleteWindow(WID_PETINFOWND)) {
				g_windowMgr.MakeWindow(WID_PETINFOWND);
			}
			break;
			case MENUID_COPY_GID: {
				char msg[128];
				sprintf(msg, "%s : %d (GID)", g_session.m_lastServerName.c_str(), m_menuTargetAID);
				g_language->SetInput(msg);
				break;
			}
		case MENUID_HOMUN_SHOWINFO:
			SendMsg(MM_COMMAND_HOMUN, 0, 0, 0);
			if (!g_windowMgr.DeleteWindow(WID_HOMUNINFOWND)) {
				g_windowMgr.MakeWindow(WID_HOMUNINFOWND);
			}
			break;
		case MENUID_HOMUN_FEEDING:
			if (g_windowMgr.ErrorMsg(MsgStr(MSI_SURE_TO_FEED_PET), ME_YESNO, 0, 0, 0) == ID_YES) {
				SendMsg(MM_COMMAND_HOMUN, 1, 0, 0);
			}
			break;
			case MENUID_HOMUN_FOLLOW: {
				CGameActor *actor = GetWorld()->GetGameActorByAID(g_session.m_homunGID);
				if (actor && actor->HomunAIIsOpened()) {
					_MSG2AI msg;
					msg.command = FOLLOW_CMD;
					actor->SetMsg2AI(msg, 0);
				}
				break;
			}
		case MENUID_MER_SHOWINFO:
			SendMsg(MM_COMMAND_MER, 1, 0, 0);
			if (!g_windowMgr.DeleteWindow(WID_MERINFOWND)) {
				g_windowMgr.MakeWindow(WID_MERINFOWND);
			}
			break;
			case MENUID_MER_FOLLOW: {
				CGameActor *actor = GetWorld()->GetGameActorByAID(g_session.m_merGID);
				if (actor && actor->MerAIIsOpened()) {
					_MSG2AI msg;
					msg.command = FOLLOW_CMD;
					actor->SetMsg2AI(msg, 1);
				}
				break;
			}
		case MENUID_VIEW_OTHERUSER_EQUIPWND: {
			PACKET_CZ_EQUIPWIN_MICROSCOPE p;
			p.PacketType = HEADER_CZ_EQUIPWIN_MICROSCOPE;
			p.AID = m_menuTargetAID;
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_EQUIPWIN_MICROSCOPE), (const char *)&p);
			break;
		}
		}
		break;
	}
	case MM_REQ_NEXT_SCRIPT: {
		PACKET_CZ_REQ_NEXT_SCRIPT p;
		p.PacketType = HEADER_CZ_REQ_NEXT_SCRIPT;
		p.NAID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_NEXT_SCRIPT), (const char *)&p);
		break;
	}
	case MM_TAKE_OFF_EQUIP: {
		if (m_waitingTakeoffEquipAck + 2000 > timeGetTime() || m_isPlayerDead) {
			break;
		}
		int		slotNum = val1;
		sAssert(slotNum != -1);

			ITEM_INFO	itemInfo = g_session.GetEquipedItems(slotNum);
			if (itemInfo.IsValid()) {
				if (itemInfo.m_num > 0) {
					m_waitingTakeoffEquipAck = timeGetTime();
					PACKET_CZ_REQ_TAKEOFF_EQUIP	 packet;
					packet.PacketType	= HEADER_CZ_REQ_TAKEOFF_EQUIP;
					packet.index		= itemInfo.m_itemIndex;
					g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_TAKEOFF_EQUIP), (const char *)(&packet));
					return	1;
				} else {
				return	0;
			}
		}
		break;
	}
	case MM_CHOOSE_MENU: {
		PACKET_CZ_CHOOSE_MENU p;
		p.PacketType = HEADER_CZ_CHOOSE_MENU;
		p.num = val1;
		p.NAID = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_CHOOSE_MENU), (const char *)&p);
		break;
	}
	case MM_CLOSE_DIALOG: {
		m_isOnQuest = false;
		g_windowMgr.DeleteWindow(WID_SAYDIALOGWND);
		g_windowMgr.DeleteWindow(WID_CHOOSEWND);
		PACKET_CZ_CLOSE_DIALOG p;
		p.PacketType = HEADER_CZ_CLOSE_DIALOG;
		p.NAID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_CLOSE_DIALOG), (const char *)&p);
		break;
	}
	case MM_WEAR_EQUIP_ARROW: {
		Trace("MM_WEAR_EQUIP_ARROW1");
		if (m_waitingWearEquipAck + 2000 > timeGetTime() || m_isPlayerDead)
			break;
		Trace("MM_WEAR_EQUIP_ARROW2");
		int		itemIndex = val1;
		int		location = val2;
		m_waitingWearEquipAck = timeGetTime();
		PACKET_CZ_REQ_WEAR_EQUIP	 packet;
		packet.PacketType	= HEADER_CZ_REQ_WEAR_EQUIP;
		packet.index		= itemIndex;
		packet.wearLocation	= location;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_WEAR_EQUIP), (const char *)(&packet));
		break;
	}
	case MM_BEGIN_DRAG_FROM_ITEMSELLWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ResetDrag();
		ITEM_INFO itemInfo = g_session.GetSellItemInfoByIndex(val3);
		if (itemInfo.IsValid()) {
			m_dragInfo.m_dragType			= DT_FROM_ITEMSELLWND;
			m_dragInfo.m_dragItemIndex		= val3;
			m_dragInfo.m_isIdentified		= itemInfo.m_isIdentified;
			m_dragInfo.m_numDragItem		= itemInfo.m_num;
			m_dragInfo.m_dragSprName		= g_session.GetItemSpriteFileName((const char *)val1, itemInfo.m_isIdentified);
			m_dragInfo.m_dragItemName		= ((const char *)val1);
			m_dragInfo.m_dragItemPrice		= val2;
			m_dragInfo.m_dragItemRealPrice	= itemInfo.m_realPrice;
			m_dragInfo.m_numDragItem		= itemInfo.m_num;
			m_dragInfo.m_slotNum			= -1;
			m_dragInfo.m_refiningLevel = itemInfo.m_refiningLevel;
			memcpy(m_dragInfo.m_slot, itemInfo.m_slot, sizeof(itemInfo.m_slot));
		}
		break;
	}
	case MM_BEGIN_DRAG_FROM_MERCHANTITEMPURCHASEWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ResetDrag();
		ITEM_INFO itemInfo = g_session.GetMerchantPurchaseItemInfoByName(((const char *)val1));
		if (itemInfo.IsValid()) {
			m_dragInfo.m_dragType			= DT_FROM_MERCHANTITEMPURCHASEWND;
			m_dragInfo.m_dragItemIndex		= itemInfo.m_itemIndex;
			m_dragInfo.m_isIdentified		= itemInfo.m_isIdentified;
			m_dragInfo.m_numDragItem		= itemInfo.m_num;
			m_dragInfo.m_dragSprName		= g_session.GetItemSpriteFileName((const char *)val1, itemInfo.m_isIdentified);
			m_dragInfo.m_dragItemName		= ((const char *)val1);
			m_dragInfo.m_dragItemPrice		= itemInfo.m_price;
			m_dragInfo.m_dragItemRealPrice	= itemInfo.m_realPrice;
			m_dragInfo.m_numDragItem		= itemInfo.m_num;
			m_dragInfo.m_slotNum			= -1;
			m_dragInfo.m_refiningLevel = itemInfo.m_refiningLevel;
			memcpy(m_dragInfo.m_slot, itemInfo.m_slot, sizeof(itemInfo.m_slot));
		}
		break;
	}
	case MM_BEGIN_DRAG_FROM_ITEMPURCHASEWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ResetDrag();
		ITEM_INFO	itemInfo = g_session.GetPurchaseItemInfoByName(((const char *)val1));
		if (itemInfo.IsValid()) {
			m_dragInfo.m_dragType			= DT_FROM_ITEMPURCHASEWND;
			m_dragInfo.m_dragItemIndex		= val3;
			m_dragInfo.m_isIdentified		= itemInfo.m_isIdentified;
			m_dragInfo.m_numDragItem		= itemInfo.m_num;
			m_dragInfo.m_dragSprName		= g_session.GetItemSpriteFileName((const char *)val1, itemInfo.m_isIdentified);
			m_dragInfo.m_dragItemName		= ((const char *)val1);
			m_dragInfo.m_dragItemPrice		= val2;
			m_dragInfo.m_dragItemRealPrice	= itemInfo.m_realPrice;
			m_dragInfo.m_numDragItem		= itemInfo.m_num;
			m_dragInfo.m_slotNum			= -1;
			m_dragInfo.m_refiningLevel = itemInfo.m_refiningLevel;
			memcpy(m_dragInfo.m_slot, itemInfo.m_slot, sizeof(itemInfo.m_slot));
		}
		break;
	}
		case MM_WEAR_EQUIP: {
			if (m_waitingWearEquipAck + 2000 > timeGetTime() || m_isPlayerDead) break;

		int slotNum[3];
		int slotNum2[3];

		int itemIndex = val1;
		int location = val2;
		g_session.BitMaskToSlotNum(location, slotNum);

		ITEM_INFO equipedItemInfo = g_session.GetEquipedItems(slotNum[0]);

		if (equipedItemInfo.m_isDamaged) {
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_DAMAGED_ITEM), 0xff, 0, 0);
			break;
		}

		if (equipedItemInfo.IsValid()) {
			if (equipedItemInfo.m_num == 1) {
				g_session.BitMaskToSlotNum(equipedItemInfo.m_wearLocation, slotNum2);
			}
		}

		ITEM_INFO itemInfo = g_session.GetItemInfoByIndex(itemIndex);

		if (itemInfo.m_isYours == 1) {
			UIFrameWnd* w = g_windowMgr.MakeWindow(WID_YOURITEMWND);
			m_waitingWearEquipAck = timeGetTime();
			w->SendMsg(NULL, UM_SET_BABY_MSG, itemIndex, val2, 0, 0);
			break;
		}

		m_waitingWearEquipAck = timeGetTime();
		PACKET_CZ_REQ_WEAR_EQUIP	 packet;
		packet.PacketType	= HEADER_CZ_REQ_WEAR_EQUIP;
		packet.index		= itemIndex;
		packet.wearLocation	= location;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_WEAR_EQUIP), (const char *)(&packet));

			break;
		}
		case MM_BEGIN_DRAG_FROM_SKILLLISTWND: {
			g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
			ResetDrag();
		const char *	skillName = (const char *)val2;
		m_dragInfo.m_dragType		= DT_FROM_SKILLLISTWND;
		m_dragInfo.m_dragItemIndex	= val1;
		m_dragInfo.m_isIdentified	= 0;
		m_dragInfo.m_dragSprName	= g_session.GetItemSpriteFileName_Skill(skillName);
		m_dragInfo.m_dragItemName	= skillName;
		m_dragInfo.m_skillName		= (const char *)val2;
		m_dragInfo.m_skillUseLevel	= val3;
		break;
	}
	case MM_BEGIN_DRAG_FROM_HOSKILLLISTWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ResetDrag();
		const char *	skillName = (const char *)val2;
		m_dragInfo.m_dragType		= DT_FROM_HOSKILLLISTWND;
		m_dragInfo.m_dragItemIndex	= val1;
		m_dragInfo.m_isIdentified	= 0;
		m_dragInfo.m_dragSprName	= g_session.GetItemSpriteFileName_Skill(skillName);
		m_dragInfo.m_dragItemName	= skillName;
		m_dragInfo.m_skillName		= (const char *)val2;
		m_dragInfo.m_skillUseLevel	= val3;
		break;
	}
	case MM_BEGIN_DRAG_FROM_MERSKILLLISTWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ResetDrag();
		const char *skillName = (const char *)val2;
		m_dragInfo.m_dragType       = DT_FROM_MERSKILLLISTWND;
		m_dragInfo.m_dragItemIndex  = val1;
		m_dragInfo.m_isIdentified   = 0;
		m_dragInfo.m_dragSprName    = g_session.GetItemSpriteFileName_Skill(skillName);
		m_dragInfo.m_dragItemName   = skillName;
		m_dragInfo.m_skillName      = skillName;
		m_dragInfo.m_skillUseLevel  = val3;
		break;
	}
	case MM_BEGIN_DRAG_FROM_ITEMSHOPWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ResetDrag();

		ITEM_INFO *		pItemInfo = (ITEM_INFO *)val1;
		ITEM_INFO		itemInfo = g_session.GetItemInfoByIndex(pItemInfo->m_itemIndex);

		if (itemInfo.IsValid()) {
			m_dragInfo.m_dragType			= DT_FROM_ITEMSHOPWND;
			m_dragInfo.m_dragItemIndex		= pItemInfo->m_itemIndex;
			m_dragInfo.m_isIdentified		= pItemInfo->m_isIdentified;
			m_dragInfo.m_numDragItem		= pItemInfo->m_num;
			m_dragInfo.m_dragSprName		= g_session.GetItemSpriteFileName(pItemInfo->m_itemName.c_str(), pItemInfo->m_isIdentified);
			m_dragInfo.m_dragItemName		= pItemInfo->m_itemName.c_str();
			m_dragInfo.m_dragItemType		= pItemInfo->m_itemType;
			m_dragInfo.m_dragItemPrice		= pItemInfo->m_price;
			m_dragInfo.m_dragItemRealPrice	= pItemInfo->m_realPrice;
			m_dragInfo.m_refiningLevel = pItemInfo->m_refiningLevel;
			memcpy(m_dragInfo.m_slot, pItemInfo->m_slot, sizeof(pItemInfo->m_slot));
			m_dragInfo.m_slotNum			= -1;
		} else {
			m_dragInfo.m_dragType			= DT_FROM_ITEMSHOPWND;
			m_dragInfo.m_dragItemIndex		= pItemInfo->m_itemIndex;
			m_dragInfo.m_numDragItem		= 1;
			m_dragInfo.m_dragSprName		= g_session.GetItemSpriteFileName(pItemInfo->m_itemName.c_str(), pItemInfo->m_isIdentified);
			m_dragInfo.m_dragItemName		= pItemInfo->m_itemName.c_str();
			m_dragInfo.m_dragItemPrice		= pItemInfo->m_price;
			m_dragInfo.m_dragItemRealPrice	= pItemInfo->m_realPrice;
			m_dragInfo.m_dragItemType		= pItemInfo->m_itemType;
			m_dragInfo.m_isIdentified		= pItemInfo->m_isIdentified;
			m_dragInfo.m_slotNum			= -1;
			m_dragInfo.m_refiningLevel = pItemInfo->m_refiningLevel;
			memcpy(m_dragInfo.m_slot, pItemInfo->m_slot, sizeof(pItemInfo->m_slot));
		}
		break;
	}
	case MM_BEGIN_DRAG_FROM_ITEMWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ITEM_INFO	itemInfo = g_session.GetItemInfoByIndex(val1);
		ResetDrag();
		if (itemInfo.IsValid()) {
			m_dragInfo.m_dragType		= DT_FROM_ITEMWND;
			m_dragInfo.m_dragItemIndex	= val1;
			m_dragInfo.m_isIdentified	= itemInfo.m_isIdentified;
			m_dragInfo.m_numDragItem	= val2;
			m_dragInfo.m_dragSprName	= g_session.GetItemSpriteFileName(itemInfo.m_itemName.c_str(), itemInfo.m_isIdentified);
			m_dragInfo.m_dragItemName	= g_session.GetItemInfoByIndex(val1).m_itemName;
			m_dragInfo.m_slotNum		= -1;
			m_dragInfo.m_refiningLevel = itemInfo.m_refiningLevel;
			m_dragInfo.m_dragItemType = itemInfo.m_itemType;
			memcpy(m_dragInfo.m_slot, itemInfo.m_slot, sizeof(itemInfo.m_slot));
		}
		break;
	}
	case MM_BEGIN_DRAG_FROM_MERCHANTSHOPMAKEWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ITEM_INFO itemInfo = g_session.GetMerchantHopeToSellItemInfoByIndex(val1);
		ResetDrag();
		if (itemInfo.IsValid()) {
			m_dragInfo.m_dragType		= DT_FROM_MERCHANTSHOPMAKEWND;
			m_dragInfo.m_dragItemIndex	= val1;
			m_dragInfo.m_isIdentified	= itemInfo.m_isIdentified;
			m_dragInfo.m_numDragItem	= itemInfo.m_num;
			m_dragInfo.m_dragSprName	= g_session.GetItemSpriteFileName(itemInfo.m_itemName.c_str(), itemInfo.m_isIdentified);
			m_dragInfo.m_dragItemName	= itemInfo.m_itemName;
			m_dragInfo.m_slotNum		= -1;
			m_dragInfo.m_refiningLevel = itemInfo.m_refiningLevel;
			memcpy(m_dragInfo.m_slot, itemInfo.m_slot, sizeof(itemInfo.m_slot));
		}
		break;
	}
	case MM_BEGIN_DRAG_FROM_MERCHANTMIRRORITEMWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ITEM_INFO itemInfo = g_session.GetMerchantMirrorItemInfoByIndex(val1);
		ResetDrag();
		if (itemInfo.IsValid()) {
			m_dragInfo.m_dragType		= DT_FROM_MERCHANTMIRRORITEMWND;
			m_dragInfo.m_dragItemIndex	= val1;
			m_dragInfo.m_isIdentified	= itemInfo.m_isIdentified;
			m_dragInfo.m_numDragItem	= itemInfo.m_num;
			m_dragInfo.m_dragSprName	= g_session.GetItemSpriteFileName(itemInfo.m_itemName.c_str(), itemInfo.m_isIdentified);
			m_dragInfo.m_dragItemName	= itemInfo.m_itemName;
			m_dragInfo.m_slotNum		= -1;
			m_dragInfo.m_refiningLevel = itemInfo.m_refiningLevel;
			memcpy(m_dragInfo.m_slot, itemInfo.m_slot, sizeof(itemInfo.m_slot));
		}
		break;
	}
	case MM_BEGIN_DRAG_FROM_MERCHANTITEMWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ITEM_INFO itemInfo = g_session.GetMerchantItemInfoByIndex(val1);
		ResetDrag();
		if (itemInfo.IsValid()) {
			m_dragInfo.m_dragType		= DT_FROM_MERCHANTITEMWND;
			m_dragInfo.m_dragItemIndex	= val1;
			m_dragInfo.m_isIdentified	= itemInfo.m_isIdentified;
			m_dragInfo.m_numDragItem	= itemInfo.m_num;
			m_dragInfo.m_dragSprName	= g_session.GetItemSpriteFileName(itemInfo.m_itemName.c_str(), itemInfo.m_isIdentified);
			m_dragInfo.m_dragItemName	= itemInfo.m_itemName;
			m_dragInfo.m_slotNum		= -1;
			m_dragInfo.m_refiningLevel = itemInfo.m_refiningLevel;
			memcpy(m_dragInfo.m_slot, itemInfo.m_slot, sizeof(itemInfo.m_slot));
		}
		break;
	}
	case MM_BEGIN_DRAG_FROM_EQUIPMWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ResetDrag();

		int		slotNum[3];

		g_session.BitMaskToSlotNum(val1, slotNum);
		ITEM_INFO	itemInfo = g_session.GetEquipedItems(slotNum[0]);
		if (itemInfo.IsValid()) {
			m_dragInfo.m_dragType		= DT_FROM_EQUIPWND;
			m_dragInfo.m_dragItemIndex	= itemInfo.m_itemIndex;
			m_dragInfo.m_isIdentified	= itemInfo.m_isIdentified;
			m_dragInfo.m_numDragItem	= itemInfo.m_num;
			m_dragInfo.m_dragSprName	= g_session.GetItemSpriteFileName(itemInfo.m_itemName.c_str(), itemInfo.m_isIdentified);
			m_dragInfo.m_slotNum		= slotNum[0];
			m_dragInfo.m_refiningLevel = itemInfo.m_refiningLevel;
			memcpy(m_dragInfo.m_slot, itemInfo.m_slot, sizeof(itemInfo.m_slot));
		}
		break;
	}
	case MM_BEGIN_DRAG_FROM_ITEMSTOREWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ITEM_INFO	itemInfo = g_session.GetStoreItemInfoByIndex(val1);
		ResetDrag();
		if (itemInfo.IsValid()) {
			m_dragInfo.m_dragType		= DT_FROM_ITEMSTOREWND;
			m_dragInfo.m_dragItemIndex	= val1;
			m_dragInfo.m_isIdentified	= itemInfo.m_isIdentified;
			m_dragInfo.m_numDragItem	= val2;
			m_dragInfo.m_dragSprName	= g_session.GetItemSpriteFileName(itemInfo.m_itemName.c_str(), itemInfo.m_isIdentified);
			m_dragInfo.m_dragItemName	= itemInfo.m_itemName;
			m_dragInfo.m_slotNum		= -1;
			m_dragInfo.m_refiningLevel = itemInfo.m_refiningLevel;
			memcpy(m_dragInfo.m_slot, itemInfo.m_slot, sizeof(itemInfo.m_slot));
		}
		break;
	}
	case MM_BEGIN_DRAG_FROM_SHORTCUTWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ITEM_INFO	itemInfo = g_session.GetItemInfoByIndex(val1);
		ResetDrag();
		if (itemInfo.IsValid()) {
			m_dragInfo.m_dragType		= DT_FROM_SHORTCUTWND;
			m_dragInfo.m_dragItemIndex	= val1;
			m_dragInfo.m_isIdentified	= itemInfo.m_isIdentified;
			m_dragInfo.m_numDragItem	= itemInfo.m_num;
			m_dragInfo.m_dragSprName	= g_session.GetItemSpriteFileName(itemInfo.m_itemName.c_str(), itemInfo.m_isIdentified);
			m_dragInfo.m_dragItemName	= itemInfo.m_itemName;
			m_dragInfo.m_slotNum		= -1;
			m_dragInfo.m_refiningLevel = itemInfo.m_refiningLevel;
			memcpy(m_dragInfo.m_slot, itemInfo.m_slot, sizeof(itemInfo.m_slot));
		}
		break;
	}
	case MM_BEGIN_DRAG_FROM_MERCHANTITEMSHOPWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ResetDrag();
		ITEM_INFO *		pItemInfo = (ITEM_INFO *)val1;

		if (pItemInfo->IsValid()) {
			m_dragInfo.m_dragType			= DT_FROM_MERCHANTITEMSHOPWND;
			m_dragInfo.m_dragItemIndex		= pItemInfo->m_itemIndex;
			m_dragInfo.m_numDragItem		= 1;
			m_dragInfo.m_dragSprName		= g_session.GetItemSpriteFileName(pItemInfo->m_itemName.c_str(), pItemInfo->m_isIdentified);
			m_dragInfo.m_dragItemName		= pItemInfo->m_itemName.c_str();
			m_dragInfo.m_dragItemPrice		= pItemInfo->m_price;
			m_dragInfo.m_dragItemRealPrice	= pItemInfo->m_realPrice;
			m_dragInfo.m_dragItemType		= pItemInfo->m_itemType;
			m_dragInfo.m_isIdentified		= pItemInfo->m_isIdentified;
			m_dragInfo.m_slotNum			= -1;
			m_dragInfo.m_refiningLevel = pItemInfo->m_refiningLevel;
			memcpy(m_dragInfo.m_slot, pItemInfo->m_slot, sizeof(pItemInfo->m_slot));
		}
		break;
	}
	case MM_DROP: {
		if (m_dragInfo.m_dragType == DT_NODRAG)
			return m_dragInfo.m_dragType;
		if (m_dragInfo.m_dragType == DT_FROM_ITEMWND) {
			if (!g_session.IsValidItemIndexInUnequiped(m_dragInfo.m_dragItemIndex)
				|| m_waitingWearEquipAck + 2000 > timeGetTime()) {
				m_dragInfo.m_dragType = DT_NODRAG;
				return 0;
			}
		} else if (m_dragInfo.m_dragType == DT_FROM_EQUIPWND) {
			if (!g_session.IsValidItemIndexInEquiped(m_dragInfo.m_dragItemIndex)) {
				m_dragInfo.m_dragType = DT_NODRAG;
				return 0;
			}
		}

		if (g_windowMgr.Drop(val1, val2, &m_dragInfo))
			break;

		if (m_dragInfo.m_dragType == DT_FROM_ITEMWND) {
			if (g_windowMgr.GetItemWnd() == NULL) {
				g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_OPEN_ITEMWND_TO_THROW), 0xff, 0, 0);
				SendMsg(MM_CANCEL_DRAG, 0, 0, 0);
				return 0;
			}
		}

		void *exchangeWnd = g_windowMgr.GetExchangeWnd();
		void *equipWnd = g_windowMgr.GetEquipWnd();
		UIItemStoreWnd *itemStoreWnd = g_windowMgr.GetItemStoreWnd();
		UIMailListWnd *mailListWnd = g_windowMgr.GetMailListWnd();
		UIMailViewWnd *mailViewWnd = g_windowMgr.GetMailViewWnd();
		UIAuctionWnd *auctionWnd = g_windowMgr.GetAuctionWnd();
		if (!exchangeWnd && !equipWnd && !itemStoreWnd && !m_isOnQuest && !mailListWnd && !mailViewWnd && !auctionWnd) {
			switch (m_dragInfo.m_dragType) {
			case DT_FROM_ITEMWND:
				if (g_session.GetEquipArrowIndex() == m_dragInfo.m_dragItemIndex) {
					ITEM_INFO it = g_session.GetItemInfoByIndex(m_dragInfo.m_dragItemIndex);
					if (it.IsValid()) {
						SendMsg(MM_CANCEL_DRAG, 0, 0, 0);
						g_windowMgr.ErrorMsg(MsgStr(MSI_UNEQUIP_ARROW), ME_OK, 1, 0, 0);
						return 0;
					}
				}
				if (m_dragInfo.m_numDragItem == 1)
					SendMsg(MM_ITEMTHROW, m_dragInfo.m_dragItemIndex, 1, 0);
				else {
					g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 1);
					UIItemDropCntWnd *w = g_windowMgr.GetItemDropCntWnd();
					w->SendMsg(0, UM_SETSTATE, 0, 0, 0, 0);
				}
				break;
			default:
				SendMsg(MM_CANCEL_DRAG, 0, 0, 0);
				break;
			}
		} else {
			SendMsg(MM_CANCEL_DRAG, 0, 0, 0);
			if (!exchangeWnd && equipWnd && !itemStoreWnd && !mailListWnd && !mailViewWnd && !auctionWnd) {
				g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
				g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_CLOSEEQUIPWND), 0xff, 0, 0);
			}
		}
		break;
	}
	case MM_CANCEL_USE_SKILL: {
		m_skillUseInfo.m_skillUseType	= SUT_NOSKILL;
		m_skillUseInfo.m_skillId		= 0;
		m_skillUseInfo.m_useLevel		= 0;
		g_isGoodSkillHanging			= false;
		break;
	}
	case MM_CANCEL_DRAG:
		m_dragInfo.m_dragType = DT_NODRAG;
		m_dragInfo.m_dragItemIndex = 0;
		m_dragInfo.m_numDragItem = 0;
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		break;
	case MM_ITEMTHROW_THROUGH_DROPCNTWND:
		SendMsg(MM_ITEMTHROW, m_dragInfo.m_dragItemIndex, val1, 0);
		break;
	case MM_ITEMTHROW: {
		sAssert(val1);
		if (!(m_waitingWearEquipAck + 2000 > timeGetTime()) && !m_waitingUseItemAck && !m_waitingItemThrowAck && val1 && !m_isPlayerDead) {
			m_waitingItemThrowAck = true;
			int		itemIndex = val1;
			int		numItem = val2;

			PACKET_CZ_ITEM_THROW	packet;
			packet.PacketType	= 0x116;
			SetPaddingValue(packet.pad, 3);
			SetPaddingValue(packet.pad2, 1);
			packet.Index		= itemIndex;
			packet.count		= numItem;
			g_connection.SendPacket(g_connection.GetPacketSize(packet.PacketType), (const char *)(&packet));
		}
		SendMsg(MM_CANCEL_DRAG, 0, 0, 0);
		break;
	}
	case MM_USEITEM: {
		if (!m_waitingUseItemAck && !m_waitingItemThrowAck && val1 && !m_isPlayerDead) {
			int effectState = m_world->m_player->GetEffectState(), bodyState = m_world->m_player->GetBodyState();
			if (IsEffectStateBurrow(effectState) || bodyState == BODYSTATE_FREEZING || bodyState == BODYSTATE_STUN ||
				bodyState == BODYSTATE_SLEEP || bodyState == BODYSTATE_STONECURSE) {
				break;
			}

			ITEM_INFO itemInfo = g_session.GetItemInfoByIndex(val1);
			if (m_world->m_player->m_stateId == 6) {
				if (itemInfo.GetItemId() == 601 || itemInfo.GetItemId() == 602 || itemInfo.GetItemId() == 611 || itemInfo.GetItemId() == 605 || itemInfo.GetItemId() == 606) {
					g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_CANT_USE_WHEN_SITDOWN), (int)RGB(255, 0, 0), 0, 0);
					break;
				}
			}

			m_waitingUseItemAck = true;

			PACKET_CZ_USE_ITEM p;
			p.PacketType = 0x9f;
			SetPaddingValue(p.pad, 2);
			SetPaddingValue(p.pad2, 4);
			p.AID = g_session.GetAid();
			p.index = val1;
			g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)&p);
		}
		break;
	}
	case MM_SOCKETERROR:
		m_noMove = 1;
		m_noMoveStartTick = timeGetTime();
		CRagConnection::instanceR().Disconnect();
		if (g_windowMgr.ErrorMsg(MsgStr(MSI_UNABLE_TO_CONNECT_SERVER), ME_OK, 1, 1, 0) == ID_MODAL_PASSED)
			break;
		SendMsg(MM_QUIT_TO_IDENTRY, 0, 0, 0);
		return 0;
	case MM_SHOWMOUSEPOINTER:
		ShowMousePointer(val1, val2, val3);
		break;
	case MM_HIDEMOUSEPOINTER:
		HideMousePointer(val1);
		break;
	case MM_BROADCASTMSG: {
		const int maxChatSize = 246;
		char	chat2[maxChatSize+1];

		int msgType = val1;
		const char* lpMsg = g_windowMgr.GetChatMsg();
		strcpy(chat2, lpMsg);

		BOOL	isGood = g_insultFilter.IsGoodSentence(chat2);
		char	chat[maxChatSize+1];
		string	chatMsg;
		if (msgType == 0 || msgType == 1) {
			chatMsg = string(g_session.GetCharName()) + ": " + g_windowMgr.GetChatMsg();
		} else if (msgType == 4 || msgType == 5) {
			chatMsg = string("blue") + g_windowMgr.GetChatMsg();
		} else {
			chatMsg = g_windowMgr.GetChatMsg();
		}

		strcpy(chat, chatMsg.c_str());

		chat[maxChatSize] = 0;
		unsigned int	chatSize = strlen(chat);
		sAssert(chatSize <= maxChatSize);
		if (strlen(g_windowMgr.GetChatMsg())) {
			chatSize++;
			PACKET_CZ_BROADCAST		p;
			if (msgType == 0) {
				p.PacketType = HEADER_CZ_BROADCAST;
			} else if (msgType == 1) {
				p.PacketType = HEADER_CZ_LOCALBROADCAST;
			} else if (msgType == 2) {
				p.PacketType = HEADER_CZ_BROADCAST;
			} else if (msgType == 3) {
				p.PacketType = HEADER_CZ_LOCALBROADCAST;
			} else if (msgType == 4) {
				p.PacketType = HEADER_CZ_BROADCAST;
			} else if (msgType == 5) {
				p.PacketType = HEADER_CZ_LOCALBROADCAST;
			}
			sAssert(sizeof(p) == 4);
			p.PacketLength = sizeof(p) + chatSize;
			char	packet[1024];
			sAssert(p.PacketLength <= sizeof(packet));
			memcpy(packet, (const char *)(&p), sizeof(p));
			memcpy(packet + sizeof(p), chat, chatSize);
			g_connection.SendPacket(p.PacketLength, (const char *)&packet[0]);
		}

		g_windowMgr.ClearChatMsg();
		break;
	}
	case MM_CHAT_BATTLEFIELD: {
		const int maxChatSize = 246;
		if (strlen(g_windowMgr.GetChatMsg()) == 0)
			break;
		if (g_session.IsEFST_Berserk() == TRUE)
			break;

		char chat2[maxChatSize + 1];
		strcpy(chat2, g_windowMgr.GetChatMsg());
		BOOL isBad = g_insultFilter.IsBadSentence(chat2);
		if (isBad) {
			g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_NO_SEND_BECAUSE_INSULT), 0xff, 0, 0);
			g_windowMgr.ClearChatMsg();
			break;
		}

		if (m_lastChat == chat2)
			m_sameChatRepeatCnt++;
		else
			m_sameChatRepeatCnt = 0;
		if (IsSameSentence(m_sameChatRepeatCnt))
			break;
		m_lastChat = chat2;

		char chat[maxChatSize + 1];
		sprintf(chat, "%s : %s%s", g_session.GetCharName(), g_language->GetLanguageCharset(0), g_windowMgr.GetChatMsg());
		unsigned int chatSize = strlen(chat);
		g_windowMgr.ClearChatMsg();

		PACKET_CZ_BATTLEFIELD_CHAT p;
		p.PacketType = HEADER_CZ_BATTLEFIELD_CHAT;
		p.PacketLength = sizeof(p) + chatSize + 1;
		char packet[1024];
		memcpy(packet, &p, sizeof(p));
		memcpy(packet + sizeof(p), chat, chatSize + 1);
		g_connection.SendPacket(p.PacketLength, (const char *)&packet[0]);
		break;
	}
	case MM_CHATMSG: {
		const int maxChatSize = 246;
		if (strlen(g_windowMgr.GetChatMsg()) == 0)
			break;
		if (g_session.IsEFST_Berserk() == TRUE)
			break;

			char chat2[maxChatSize + 1];
			char chat_p2[maxChatSize + 1];
			char chat_p1[maxChatSize + 1];
			char chat_m1[maxChatSize + 1];
			char chat_m2[maxChatSize + 1];
		int  len;
		strcpy(chat2, g_windowMgr.GetChatMsg());
		BOOL isBad = g_insultFilter.IsBadSentence(chat2);
		if (isBad) {
			g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_NO_SEND_BECAUSE_INSULT), 0xff, 0, 0);
			g_windowMgr.ClearChatMsg();
			break;
		}

		if (m_lastChat == chat2)
			m_sameChatRepeatCnt++;
		else
			m_sameChatRepeatCnt = 0;
		if (IsSameSentence(m_sameChatRepeatCnt))
			break;
		m_lastChat = chat2;

				memcpy(chat_p2, chat2, maxChatSize + 1);
		len = strlen(chat_p2);
		if (len > 6) {
			if (len + 3 < maxChatSize + 1) {
				chat_p2[len + 2] = chat_p2[len];
				chat_p2[len + 3] = chat_p2[len + 1];
				chat_p2[len]     = chat_p2[len - 2];
				chat_p2[len + 1] = chat_p2[len - 1];
			}

					memcpy(chat_p1, chat2, maxChatSize + 1);
			len = strlen(chat_p1);
			if (len + 1 < maxChatSize + 1) {
				chat_p1[len + 1] = chat_p1[len];
				chat_p1[len]     = chat_p1[len - 1];
			}

					memcpy(chat_m1, chat2, maxChatSize + 1);
			len = strlen(chat_m1);
			if (len + 1 < maxChatSize + 1) {
				chat_m1[len - 1] = chat_m1[len];
				chat_m1[len]     = chat_m1[len + 1];
			}

					memcpy(chat_m2, chat2, maxChatSize + 1);
			len = strlen(chat_m2);
			if (len + 1 < maxChatSize + 1) {
				chat_m2[len - 2] = chat_m2[len];
				chat_m2[len - 1] = chat_m2[len + 1];
			}
		}

		if (m_recordChatNum < 0 && m_recordChatNum > 10)
			m_recordChatNum = 0;
		if (m_recordChatNum == 10) {
			for (int i = 0; i < 10; i++) {
				m_recordChat[i]     = m_recordChat[i + 1];
				m_recordChatTime[i] = m_recordChatTime[i + 1];
			}
		}
		m_recordChat[m_recordChatNum]     = chat2;
		m_recordChatTime[m_recordChatNum] = timeGetTime();
		int recordChatNum = m_recordChatNum;
		recordChatNum++;
		int maxRecordChatNum = 10;
		m_recordChatNum = recordChatNum;
		if (m_recordChatNum > maxRecordChatNum)
			m_recordChatNum = maxRecordChatNum;

		{
			int count = 0;
			for (int i = 0; i <= m_recordChatNum; i++) {
				if (i == 10) {
					if ((timeGetTime() - m_recordChatTime[0] < 10000) && (timeGetTime() - m_recordChatTime[0] > 0)) {
						ChatDori();
						m_strikeNum = 0;
						break;
					} else if ((timeGetTime() - m_recordChatTime[0] < 30000) && (timeGetTime() - m_recordChatTime[0] > 0)) {
						for (int j = 0; j < m_recordChatNum; j++) {
							if (m_recordChat[j].length() < 20)
								break;
							if (j == m_recordChatNum - 1) {
								g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)"TOO FAST CHATTING!", (int)RGB(255, 150, 110), 0, 0);
								ChatDori();
								m_strikeNum = 0;
								break;
							}
						}
					}

					if (g_session.GetJob() == JT_SUPERNOVICE || g_session.GetJob() == JT_SUPERNOVICE_B) {
						char temp[256], temp2[256], temp3[256];
							if (g_serviceType == ServiceKorea) {
							sprintf(temp, MsgStr(MSI_SUPERNOVICE1));
							sprintf(temp2, "%s %s%s", MsgStr(MSI_SUPERNOVICE2), g_session.GetCharName(), MsgStr(MSI_SUPERNOVICE3));
							sprintf(temp3, MsgStr(MSI_SUPERNOVICE4));
						} else {
							sprintf(temp, MsgStrToCompare(MSI_SUPERNOVICE1));
							sprintf(temp2, "%s %s%s", MsgStrToCompare(MSI_SUPERNOVICE2), g_session.GetCharName(), MsgStrToCompare(MSI_SUPERNOVICE3));
							sprintf(temp3, MsgStrToCompare(MSI_SUPERNOVICE4));
						}

						if (m_recordChat[9] == temp3) {
							if (m_recordChat[7] == temp) {
								if (m_world->m_player->m_stateId != 3) {
									PACKET_CZ_CHOPOKGI p;
									p.PacketType = HEADER_CZ_CHOPOKGI;
									g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_CHOPOKGI), (const char *)&p);
								}
							}
						}
					}
				}

				if (strlen(chat2) > 6) {
					if (chat2 == m_recordChat[i] || chat_p2 == m_recordChat[i] || chat_p1 == m_recordChat[i] || chat_m1 == m_recordChat[i] || chat_m2 == m_recordChat[i]) {
						if ((timeGetTime() - m_recordChatTime[i] < 30000) && (timeGetTime() - m_recordChatTime[0] > 0)) {
							count++;
							if (count >= 3) {
								if (m_strikeNum == 3) {
									for (int k = 0; k < 2; k++)
										m_strikeTime[k] = m_strikeTime[k + 1];
									m_strikeNum = 2;
								}
								m_strikeTime[m_strikeNum] = timeGetTime();
								if (m_strikeNum >= 2) {
									if (m_strikeTime[2] - m_strikeTime[0] < 90000) {
										ChatDori();
										m_strikeNum = 0;
										break;
									}
								}
								if (m_strikeNum < 3)
									m_strikeNum++;
								break;
							}
						}
					}
				}
			}
			}

			for (int a = 0; a < stateId.size(); a++) {
				if (stateId[a] == EFST_RANKING) {
					char msg[256];
					sprintf(msg, "%s", g_windowMgr.GetChatMsg());
					int num = atoi(msg);

					PACKET_CZ_DEATH_QUESTION p;
					p.PacketType = HEADER_CZ_DEATH_QUESTION;
					p.Qanswer = num;
					g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_DEATH_QUESTION), (const char *)(&p));
				}
			}

			{
				char chat[maxChatSize + 1];
			sprintf(chat, "%s : %s%s", g_session.GetCharName(), g_language->GetLanguageCharset(0), g_windowMgr.GetChatMsg());
			chat[maxChatSize] = 0;
			unsigned int chatSize = strlen(chat);
			g_windowMgr.ClearChatMsg();
			PACKET_CZ_REQUEST_CHAT p;
			p.PacketType   = 0xf3;
			p.PacketLength = sizeof(p) + chatSize + 1;
			char packet[1024];
			memcpy(packet, &p, sizeof(p));
			memcpy(packet + sizeof(p), chat, chatSize + 1);
			g_connection.SendPacket(p.PacketLength, (const char *)packet);
		}
		break;
	}
	case MM_CHAT_GUILD: {
		const int maxChatSize = 246;
		if (strlen(g_windowMgr.GetChatMsg()) == 0)
			break;
		if (g_guildInfo.m_gdid == 0)
			break;
		if (g_session.IsEFST_Berserk() == TRUE)
			break;

		char chat2[maxChatSize + 1];
		strcpy(chat2, g_windowMgr.GetChatMsg());
		BOOL isBad = g_insultFilter.IsBadSentence(chat2);
		if (isBad) {
			g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_NO_SEND_BECAUSE_INSULT), 0xff, 0, 0);
			g_windowMgr.ClearChatMsg();
			break;
		}

		if (m_lastChat == chat2)
			m_sameChatRepeatCnt++;
		else
			m_sameChatRepeatCnt = 0;
		if (IsSameSentence(m_sameChatRepeatCnt))
			break;
		m_lastChat = chat2;

		char chat[maxChatSize + 1];
		sprintf(chat, "%s : %s%s", g_session.GetCharName(), g_language->GetLanguageCharset(0), g_windowMgr.GetChatMsg());
		unsigned int chatSize = strlen(chat);
		g_windowMgr.ClearChatMsg();
		if (chatSize >= 1024 - sizeof(PACKET_CZ_GUILD_CHAT))
			break;

		PACKET_CZ_GUILD_CHAT p;
		p.PacketType = HEADER_CZ_GUILD_CHAT;
		p.PacketLength = sizeof(p) + chatSize + 1;
		char packet[1024];
		memcpy(packet, &p, sizeof(p));
		memcpy(packet + sizeof(p), chat, chatSize + 1);
		g_connection.SendPacket(p.PacketLength, (const char *)&packet[0]);
		break;
	}
	case MM_CHAT_PARTY: {
		const int maxChatSize = 246;
		if (strlen(g_windowMgr.GetChatMsg()) == 0)
			break;
		if (g_session.IsEFST_Berserk() == TRUE)
			break;

		char chat2[maxChatSize + 1];
		strcpy(chat2, g_windowMgr.GetChatMsg());
		BOOL isBad = g_insultFilter.IsBadSentence(chat2);
		if (isBad) {
			g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_NO_SEND_BECAUSE_INSULT), 0xff, 0, 0);
			g_windowMgr.ClearChatMsg();
			break;
		}

		if (m_lastChat == chat2)
			m_sameChatRepeatCnt++;
		else
			m_sameChatRepeatCnt = 0;
		if (IsSameSentence(m_sameChatRepeatCnt))
			break;
		m_lastChat = chat2;

		char chat[maxChatSize + 1];
		sprintf(chat, "%s : %s%s", g_session.GetCharName(), g_language->GetLanguageCharset(0), g_windowMgr.GetChatMsg());
		unsigned int chatSize = strlen(chat);
		g_windowMgr.ClearChatMsg();

		PACKET_CZ_REQUEST_CHAT_PARTY p;
		p.PacketType = HEADER_CZ_REQUEST_CHAT_PARTY;
		p.PacketLength = sizeof(p) + chatSize + 1;
		char packet[1024];
		memcpy(packet, &p, sizeof(p));
		memcpy(packet + sizeof(p), chat, chatSize + 1);
		g_connection.SendPacket(p.PacketLength, (const char *)&packet[0]);
		break;
	}
	case MM_WHISPERMSG: {
		const int maxChatSize = 246;
		const int maxWhisperSize = maxChatSize - NAME_LENGTH;
		if (strlen(g_windowMgr.GetChatMsg()) == 0)
			break;
		if (g_session.IsEFST_Berserk() == TRUE)
			break;

		char whisperChat2[maxWhisperSize + 1];
		strncpy(whisperChat2, g_windowMgr.GetChatMsg(), maxWhisperSize);
		whisperChat2[maxWhisperSize] = 0;
		BOOL isBad = g_insultFilter.IsBadSentence(whisperChat2);
		if (isBad) {
			g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_NO_SEND_BECAUSE_INSULT), 0xff, 0, 0);
			g_windowMgr.ClearChatMsg();
			break;
		}

		if (g_MessageHi == 0) {
			if (m_lastChat == whisperChat2)
				m_sameChatRepeatCnt++;
			else
				m_sameChatRepeatCnt = 0;
		}
		if (!g_session.m_onAutoMSG) {
			if (IsSameSentence(m_sameChatRepeatCnt))
				break;
		}

		m_lastChat = whisperChat2;

			char cName[NAME_LENGTH + 1];
			memset(cName, 0, NAME_LENGTH + 1);
			strcpy(cName, (char *)val1);
			m_lastWhisperName = cName;
			BOOL isToOther = strcmp(m_lastWhisperName.c_str(), g_session.GetCharName());
			if (!isToOther) {
				g_windowMgr.ClearChatMsg();
				break;
			}

		char whisperChat[maxWhisperSize + 1];
		if (g_multiLang) {
			strcpy(whisperChat, g_language->GetLanguageCharset(0));
			strncpy(whisperChat + 3, g_windowMgr.GetChatMsg(), maxWhisperSize - 3);
		} else {
			strncpy(whisperChat, g_windowMgr.GetChatMsg(), maxWhisperSize);
			}
			whisperChat[maxWhisperSize] = 0;
			m_lastWhisper = whisperChat;
			unsigned int whisperChatSize = strlen(whisperChat);
			if (whisperChatSize <= 0 || whisperChatSize >= maxWhisperSize) {
				g_windowMgr.ClearChatMsg();
				break;
			}
			g_windowMgr.ClearChatMsg();

			PACKET_CZ_WHISPER p;
		p.PacketType = HEADER_CZ_WHISPER;
		memcpy(p.receiver, cName, NAME_LENGTH);
		p.PacketLength = sizeof(p) + whisperChatSize + 1;
		char packet[1024];
		memcpy(packet, &p, sizeof(p));
		memcpy(packet + sizeof(p), whisperChat, whisperChatSize + 1);
		g_connection.SendPacket(p.PacketLength, (const char *)&packet[0]);
		break;
	}
	case MM_DISCONNECT_CHARACTER: {
		PACKET_CZ_DISCONNECT_CHARACTER p;
		p.PacketType = HEADER_CZ_DISCONNECT_CHARACTER;
		p.AID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_DISCONNECT_CHARACTER), (const char *)&p);
		break;
	}
	case MM_DISCONNECT_ALL_CHARACTER: {
		PACKET_CZ_DISCONNECT_ALL_CHARACTER p; p.PacketType = HEADER_CZ_DISCONNECT_ALL_CHARACTER;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_DISCONNECT_ALL_CHARACTER), (const char *)&p);
		break;
	}
	case MM_QUERYRSWNAME:                  return (INT_PTR)m_rswName;
	case MM_CONTACTNPC: {
		Trace("MM_CONTACTNPC");
		if (m_world->m_player)
			m_world->m_player->SendMsg(NULL, 8, 0, 0, 0);
		CONTACTNPC *	s = (CONTACTNPC *)val1;
		if (s->job == JT_WARPNPC) {
			if (m_noMove == TRUE)
				break;
			m_noMove = TRUE;
			m_noMoveStartTick = timeGetTime();
		}
		PACKET_CZ_CONTACTNPC	packet;
		packet.PacketType = HEADER_CZ_CONTACTNPC;
		packet.NAID = s->nid;
		packet.type = s->type;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_CONTACTNPC), (const char *)(&packet));
		break;
	}
	case MM_ADDGUILDPOS: {
		DWORD	aid = val1;
		m_guildPosList[aid].x		= val2;
		m_guildPosList[aid].y		= val3;
		m_guildPosList[aid].color	= 0xffffa6d2;
		break;
	}
	case MM_DELETEGUILDPOS: {
		DWORD	aid = val1;
		std::map<DWORD, ColorCellPos>::iterator		it = m_guildPosList.find(aid);
		if (it != m_guildPosList.end())
			m_guildPosList.erase(it);
		break;
	}
	case MM_DELETEGUILDPOSALL: {
		m_guildPosList.erase(m_guildPosList.begin(), m_guildPosList.end());
		break;
	}
	case MM_ADDREGISTERPOS: {
		DWORD	gid = val1;
		m_actorPosList[gid].x = val2;
		m_actorPosList[gid].y = val3;
		break;
	}
	case MM_DELETEREGISTERPOS: {
		DWORD	gid = val1;
		std::map<DWORD, CellPos>::iterator	it = m_actorPosList.find(gid);
		if (it != m_actorPosList.end())
			m_actorPosList.erase(it);
		break;
	}
	case MM_PROCESS_TALK_TYPE:
		ProcessTalkType(val1, val2);
		break;
	case MM_ADDPARTYPOS: {
		DWORD	aid = val1;
		m_partyPosList[aid].x		= val2;
		m_partyPosList[aid].y		= val3;
		m_partyPosList[aid].color	= g_session.GetPartyInfoByAid(aid).color;
		break;
	}
	case MM_DELETEPARTYPOSALL: {
		m_partyPosList.erase(m_partyPosList.begin(), m_partyPosList.end());
		break;
	}
	case MM_DELETEPARTYPOS: {
		DWORD	gid = val1;
		std::map<DWORD, ColorCellPos>::iterator		it = m_partyPosList.find(gid);
		if (it != m_partyPosList.end())
			m_partyPosList.erase(it);
		break;
	}
	case MM_REQ_ROLE_CHANGE: {
		PACKET_CZ_REQ_ROLE_CHANGE p;
		p.PacketType = HEADER_CZ_REQ_ROLE_CHANGE;
		char name[25];
		strcpy(name, (const char *)val1);
		memcpy(p.name, name, 24);
		p.role = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_ROLE_CHANGE), (const char *)&p);
		break;
	}
	case MM_REQ_EXPEL_MEMBER: {
		PACKET_CZ_REQ_EXPEL_MEMBER p;
		p.PacketType = HEADER_CZ_REQ_EXPEL_MEMBER;
		char name[25];
		strcpy(name, (const char *)val1);
		memcpy(p.name, name, 24);
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_EXPEL_MEMBER), (const char *)&p);
		break;
	}
	case MM_REQ_EXIT_ROOM:
		if (g_windowMgr.GetChatRoom()) {
			PACKET_CZ_EXIT_ROOM p;
			p.PacketType = HEADER_CZ_EXIT_ROOM;
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_EXIT_ROOM), (const char *)&p);
		}
		break;
	case MM_REQ_EXCHANGE_ITEM: {
		if (g_serviceType == ServiceIndonesia && g_windowMgr.GetExchangeAcceptWnd()) {
			g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_CHARACTER_IS_DEALING), 0xff, 0, 0);
			break;
		}
		SendMsg(MM_RELEASE_QUEST, 0, 0, 0);
		g_session.ClearDealItems();

		if (m_actorNameList.find(val1) != m_actorNameList.end()) {
			m_lastExchangeCharacterName = m_actorNameList[val1].cName;
			PACKET_CZ_REQ_EXCHANGE_ITEM		p;
			p.PacketType	= HEADER_CZ_REQ_EXCHANGE_ITEM;
			p.AID			= val1;
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_EXCHANGE_ITEM), (const char *)(&p));
		}
		break;
	}
	case MM_ACK_EXCHANGE_ITEM: {
		SendMsg(MM_RELEASE_QUEST, 0, 0, 0);
		g_session.ClearDealItems();
		PACKET_CZ_ACK_EXCHANGE_ITEM p;
		p.PacketType = HEADER_CZ_ACK_EXCHANGE_ITEM;
		p.result = (unsigned char)val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_ACK_EXCHANGE_ITEM), (const char *)&p);
		break;
	}
	case MM_ADD_EXCHANGE_ITEM: {
		if (g_session.IsExistIndexInMyChangeList(val1)) {
			g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_CAN_DRAG_ITEM_OLNY_ONETIME), 0xff, 0, 0);
			break;
		}

		if (g_session.IsExchangeItemListFull() && val1) {
			g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_MAXEXCHANGE_IS_10_KIND), 0xff, 0, 0);
			break;
		}

		if (!m_isWaitingAddExchangeItem) {
			m_isWaitingAddExchangeItem = TRUE;
			PACKET_CZ_ADD_EXCHANGE_ITEM		p;
			p.PacketType	= HEADER_CZ_ADD_EXCHANGE_ITEM;
			p.index			= val1;
			m_exchangeItemCnt = p.count = val2;
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_ADD_EXCHANGE_ITEM), (const char *)(&p));
		}
		break;
	}
	case MM_CREATE_CHATROOM: {
		ChatRoomInfo *info = (ChatRoomInfo *)val1;
		m_lastChatroomInfo.title        = info->title;
		m_lastChatroomInfo.roomType     = info->roomType;
		m_lastChatroomInfo.numPeople    = info->numPeople;
		m_lastChatroomInfo.maxNumPeople = info->maxNumPeople;
		m_lastChatroomInfo.roomId       = info->roomId;
		m_lastChatroomInfo.pass         = info->pass;
		PACKET_CZ_CREATE_CHATROOM p;
		p.PacketType   = HEADER_CZ_CREATE_CHATROOM;
		p.PacketLength = info->title.size() + sizeof(p);
		memset(p.passwd, 0, 8);
		memcpy(p.passwd, info->pass.c_str(), info->pass.size());
		p.size = (short)info->maxNumPeople;
		p.type = (unsigned char)info->roomType;
		char packet[1024];
		memcpy(packet, &p, sizeof(p));
		memcpy(packet + sizeof(p), info->title.c_str(), info->title.size());
		g_connection.SendPacket(p.PacketLength, packet);
		break;
	}
	case MM_CHANGE_CHATROOM: {
		if (g_windowMgr.GetChatRoom()) {
			ChatRoomInfo *info = (ChatRoomInfo *)val1;
			m_lastChatroomInfo.title        = info->title;
			m_lastChatroomInfo.roomType     = info->roomType;
			m_lastChatroomInfo.numPeople    = info->numPeople;
			m_lastChatroomInfo.maxNumPeople = info->maxNumPeople;
			m_lastChatroomInfo.roomId       = info->roomId;
			m_lastChatroomInfo.pass         = info->pass;
			PACKET_CZ_CHANGE_CHATROOM p;
			p.PacketType   = HEADER_CZ_CHANGE_CHATROOM;
			p.PacketLength = info->title.size() + sizeof(p);
			memset(p.passwd, 0, 8);
			memcpy(p.passwd, info->pass.c_str(), info->pass.size());
			p.size = (short)info->maxNumPeople;
			p.type = (unsigned char)info->roomType;
			char packet[1024];
			memcpy(packet, &p, sizeof(p));
			memcpy(packet + sizeof(p), info->title.c_str(), info->title.size());
			g_connection.SendPacket(p.PacketLength, packet);
		}
		break;
	}
	case MM_REQ_ENTER_ROOM: {
		UIChatRoomWnd *chatRoom = g_windowMgr.GetChatRoom();
		if (timeGetTime() > m_reqTickChatRoom + 1000)
			m_isWaitingEnterRoom = 0;
		if (chatRoom)
			break;
		if (m_isWaitingEnterRoom)
			break;
		m_isWaitingEnterRoom = 1;
		m_reqTickChatRoom = timeGetTime();
		PACKET_CZ_REQ_ENTER_ROOM p;
		p.PacketType = HEADER_CZ_REQ_ENTER_ROOM;
		p.roomID = val1;
		if ((const char *)val2) {
			char passwd[9];
			strcpy(passwd, (const char *)val2);
			memcpy(p.passwd, passwd, 8);
		} else {
			memset(p.passwd, 0, 8);
		}
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_ENTER_ROOM), (const char *)&p);
		break;
	}
	case MM_ADD_ITEM_TO_STORE: {
		PACKET_CZ_MOVE_ITEM_FROM_BODY_TO_STORE p;
		p.PacketType = 0x94;
		SetPaddingValue(p.pad, 5);
		SetPaddingValue(p.pad2, 1);
		p.index = (short)val1;
		p.count = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(0x94), (const char *)&p);
		break;
	}
	case MM_MOVE_ITEM_FROM_STORE_TO_BODY: {
		PACKET_CZ_MOVE_ITEM_FROM_STORE_TO_BODY p;
		p.PacketType = 0xf7;
		SetPaddingValue(p.pad, 12);
		SetPaddingValue(p.pad2, 2);
		p.index = (short)val1;
		p.count = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(0xf7), (const char *)&p);
		break;
	}
	case MM_CLOSE_STORE: {
		PACKET_CZ_CLOSE_STORE p; p.PacketType = 0x193;
		g_connection.SendPacket(g_connection.GetPacketSize(0x193), (const char *)&p);
		break;
	}
	case MM_MOVE_ITEM_FROM_BODY_TO_CART: {
		PACKET_CZ_MOVE_ITEM_FROM_BODY_TO_CART p;
		p.PacketType = HEADER_CZ_MOVE_ITEM_FROM_BODY_TO_CART;
		p.index = val1;
		p.count = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_MOVE_ITEM_FROM_BODY_TO_CART), (const char *)&p);
		break;
	}
	case MM_MOVE_ITEM_FROM_CART_TO_BODY: {
		PACKET_CZ_MOVE_ITEM_FROM_CART_TO_BODY p;
		p.PacketType = HEADER_CZ_MOVE_ITEM_FROM_CART_TO_BODY;
		p.index = val1;
		p.count = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_MOVE_ITEM_FROM_CART_TO_BODY), (const char *)&p);
		break;
	}
	case MM_MOVE_ITEM_FROM_STORE_TO_CART: {
		PACKET_CZ_MOVE_ITEM_FROM_STORE_TO_CART p;
		p.PacketType = HEADER_CZ_MOVE_ITEM_FROM_STORE_TO_CART;
		p.index = val1;
		p.count = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_MOVE_ITEM_FROM_STORE_TO_CART), (const char *)&p);
		break;
	}
	case MM_MOVE_ITEM_FROM_CART_TO_STORE: {
		PACKET_CZ_MOVE_ITEM_FROM_CART_TO_STORE p;
		p.PacketType = HEADER_CZ_MOVE_ITEM_FROM_CART_TO_STORE;
		p.index = val1;
		p.count = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_MOVE_ITEM_FROM_CART_TO_STORE), (const char *)&p);
		break;
	}
	case MM_CONCLUDE_EXCHANGE_ITEM: {
		PACKET_CZ_CONCLUDE_EXCHANGE_ITEM p; p.PacketType = HEADER_CZ_CONCLUDE_EXCHANGE_ITEM;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_CONCLUDE_EXCHANGE_ITEM), (const char *)&p);
		break;
	}
	case MM_CANCEL_EXCHANGE_ITEM: {
		PACKET_CZ_CANCEL_EXCHANGE_ITEM p; p.PacketType = HEADER_CZ_CANCEL_EXCHANGE_ITEM;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_CANCEL_EXCHANGE_ITEM), (const char *)&p);
		break;
	}
	case MM_EXEC_EXCHANGE_ITEM: {
		PACKET_CZ_EXEC_EXCHANGE_ITEM p; p.PacketType = HEADER_CZ_EXEC_EXCHANGE_ITEM;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_EXEC_EXCHANGE_ITEM), (const char *)&p);
		break;
	}
	case MM_MAKE_GROUP: {
		if (g_session.GetNumParty())
			break;
		const char *groupName = (const char *)val1;
		if (strlen(groupName) >= NAME_LENGTH) {
			g_windowMgr.ErrorMsg(MsgStr(MSI_NAMELENGTH_TOO_LONG), ME_OK, 1, 0, 0);
			break;
		}
		g_windowMgr.DeleteWindow(WID_PARTYSETTINGWND);
		g_windowMgr.MakeWindow(WID_PARTYSETTINGWND);
		g_session.m_partyRequestName = groupName;
		break;
	}
	case MM_CONFORM_MAKEGROUP: {
		PACKET_CZ_MAKE_GROUP2	pnew;
		pnew.PacketType = HEADER_CZ_MAKE_GROUP2;
		strncpy(pnew.groupName, g_session.m_partyRequestName.c_str(), NAME_LENGTH);
		pnew.ItemPickupRule = static_cast<BYTE>(val2);
		pnew.ItemDivisionRule = static_cast<BYTE>(val3);
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_MAKE_GROUP2), (const char *)(&pnew));
		break;
	}
	case MM_REQ_JOIN_GROUP: {
		PACKET_CZ_PARTY_JOIN_REQ p;
		p.PacketType = HEADER_CZ_PARTY_JOIN_REQ;
		const char *characterName = (const char *)val1;
		if (strlen(characterName) >= 24) {
			g_windowMgr.ErrorMsg(MsgStr(MSI_NAMELENGTH_TOO_LONG), ME_OK, 1, 0, 0);
			break;
		}
		strncpy(p.characterName, characterName, 24);
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)&p);
		break;
	}
	case MM_STARPLACE_ACCEPT: {
		PACKET_CZ_AGREE_STARPLACE p; p.PacketType = HEADER_CZ_AGREE_STARPLACE;
		p.which = (char)val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_AGREE_STARPLACE), (const char *)&p);
		break;
	}
	case MM_REQ_JOIN_BABY: {
		PACKET_CZ_REQ_JOIN_BABY p;
		p.PacketType = HEADER_CZ_REQ_JOIN_BABY;
		p.AID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_JOIN_BABY), (const char *)&p);
		break;
	}
	case MM_JOIN_GROUP: {
		if (val2 == 0)
			g_session.m_partyName = "";
		else
			g_windowMgr.MakeWindow(WID_MESSENGERGROUPWND);

		PACKET_CZ_PARTY_JOIN_REQ_ACK	p;
		p.PacketType	= HEADER_CZ_PARTY_JOIN_REQ_ACK;
		p.bAccept		= (val2) ? true : false;
		p.GRID			= val1;

		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_PARTY_JOIN_REQ_ACK), (const char *)(&p));
		break;
	}
	case MM_JOIN_COUPLE: {
		PACKET_CZ_JOIN_COUPLE p;
		p.PacketType = HEADER_CZ_JOIN_COUPLE;
		p.GID = m_lastCouplePacketGid;
		p.AID = m_lastCouplePacketAid;
		p.answer = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_JOIN_COUPLE), (const char *)(&p));
		break;
	}
	case MM_JOIN_BABY: {
		PACKET_CZ_JOIN_BABY p;
		p.PacketType = HEADER_CZ_JOIN_BABY;
		p.GID = m_lastCouplePacketGid;
		p.AID = m_lastCouplePacketAid;
		p.answer = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_JOIN_BABY), (const char *)&p);
		break;
	}
	case MM_DORIDORI: {
		PACKET_CZ_DORIDORI p; p.PacketType = HEADER_CZ_DORIDORI;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_DORIDORI), (const char *)&p);
		break;
	}
	case MM_REQ_LEAVE_GROUP: {
		PACKET_CZ_REQ_LEAVE_GROUP p; p.PacketType = HEADER_CZ_REQ_LEAVE_GROUP;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_LEAVE_GROUP), (const char *)&p);
		break;
	}
	case MM_REQ_EXPEL_GROUP_MEMBER: {
		if (g_session.m_amIPartyMaster) {
			PACKET_CZ_REQ_EXPEL_GROUP_MEMBER	p;
			p.PacketType = HEADER_CZ_REQ_EXPEL_GROUP_MEMBER;
			const char *	name = (const char *)val1;
			strncpy(p.characterName, name, NAME_LENGTH);
			FRIEND_INFO		fInfo = g_session.GetPartyInfoByName(name);
			sAssert(fInfo.isValid);
			p.AID = fInfo.AID;
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_EXPEL_GROUP_MEMBER), (const char *)(&p));
		}
		break;
	}
	case MM_USING_SKILL: {
		SKILL_INFO *	skillInfo = (SKILL_INFO *)val1;
		int		skillUseLevel = val2;

		switch (skillInfo->type)
		{
			case	SKILLTYPE_PASSIVE:
				break;
			case	SKILLTYPE_TOCHARACTER:
				SendMsg(MM_HANG_USE_SKILL, SUT_TO_CHARACTER, INT_PTR(skillInfo), skillUseLevel);
				break;
			case	SKILLTYPE_TOGROUND:
				SendMsg(MM_HANG_USE_SKILL, SUT_TO_GROUND, INT_PTR(skillInfo), skillUseLevel);
				break;
			case	SKILLTYPE_TOALL:
				SendMsg(MM_HANG_USE_SKILL, SUT_TO_ALL, INT_PTR(skillInfo), skillUseLevel);
				break;
			case	SKILLTYPE_TOME: {
				DWORD Aid = 0;
				if (skillInfo->SKID >= 8000 && skillInfo->SKID < 8017)
					Aid = g_session.m_homunGID;
				else if (skillInfo->SKID >= 8200 && skillInfo->SKID < 8238)
					Aid = g_session.m_merGID;
				else
					Aid = g_session.GetAid();
				SendMsg(MM_USE_SKILL, skillInfo->SKID, Aid, skillUseLevel);
				SendMsg(MM_HANG_USE_SKILL, SUT_NOSKILL, 0, 0);
				break;
			}
			case	SKILLTYPE_TOITEM:
				SendMsg(MM_HANG_USE_SKILL, SUT_TO_ITEM, INT_PTR(skillInfo), skillUseLevel);
				break;
			case	SKILLTYPE_TOSKILL:
				SendMsg(MM_HANG_USE_SKILL, SUT_TO_SKILL, INT_PTR(skillInfo), skillUseLevel);
				break;
		}
		break;
	}
	case MM_UPGRADE_SKILLLEVEL: {
		if (m_isReqUpgradeSkillLevel == FALSE) {
			m_isReqUpgradeSkillLevel = TRUE;
			int skillId = val1;
			PACKET_CZ_UPGRADE_SKILLLEVEL p;
			p.PacketType = HEADER_CZ_UPGRADE_SKILLLEVEL;
			p.SKID = skillId;
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_UPGRADE_SKILLLEVEL), (const char *)(&p));
		}
		break;
	}
	case MM_HANG_USE_SKILL: {
		if (g_windowMgr.GetMerchantItemMyShopWnd())
			break;
		m_skillUseInfo.m_skillUseType = (ENUM_SKILL_USE_TYPE)val1;
		if (val2) {
			SKILL_INFO *	pSkillInfo = (SKILL_INFO *)val2;
			m_skillUseInfo.m_skillId		= pSkillInfo->SKID;
			m_skillUseInfo.m_attackRange	= pSkillInfo->attackRange;
			if (SKILL_INFO::IsLevelUseSkill(pSkillInfo->SKID))
				m_skillUseInfo.m_useLevel = val3;
			else {
				SKILL_INFO skillInfo;
				if (m_skillUseInfo.m_skillId >= 8000 && m_skillUseInfo.m_skillId < 8017)
					skillInfo = g_session.GetHomunSkillItemInfoBySkillId(m_skillUseInfo.m_skillId);
				else if (m_skillUseInfo.m_skillId >= 8200 && m_skillUseInfo.m_skillId < 8238)
					skillInfo = g_session.GetMerSkillItemInfoBySkillId(m_skillUseInfo.m_skillId);
				else
					skillInfo = g_session.GetSkillItemInfoBySkillId(m_skillUseInfo.m_skillId);

				if (skillInfo.m_isValid) {
					m_skillUseInfo.m_useLevel = skillInfo.level;
				} else {
					m_skillUseInfo.m_useLevel = 0;
				}
			}
			g_isGoodSkillHanging = SKILL_INFO::IsGoodSkillForActor(m_skillUseInfo.m_skillId);
		}
		break;
	}
	case MM_USE_SKILL: {
		int SKID = val1;
		if (SKID == 10000) {
			if (val2 == g_session.m_petGID)
				return 0;

			UIPetTamingDeceiveWnd *pWnd = (UIPetTamingDeceiveWnd *)g_windowMgr.MakeWindow(WID_PETTAMINGDECEIVEWND);
			pWnd->SendMsg(0, UM_SET_TARGET, 0, val2, 0, 0);
			return 0;
		}
		if (SKID == 20000) {
			PACKET_CZ_REQ_JOIN_COUPLE p;
			p.PacketType = HEADER_CZ_REQ_JOIN_COUPLE;
			p.AID = val2;
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_JOIN_COUPLE), (const char *)&p);
			return 0;
		}
		if (SKID == 20001) {
			char msgBuf[256];
			sprintf(msgBuf, "[ (val1 == 20001) ]");
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)msgBuf, (int)RGB(255, 150, 110), 0, 0);
			PACKET_CZ_REQ_JOIN_BABY p;
			p.PacketType = HEADER_CZ_REQ_JOIN_BABY;
			p.AID = val2;
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_JOIN_BABY), (const char *)&p);
			return 0;
		}
		if (g_windowMgr.GetMerchantItemMyShopWnd())
			break;
		int skillUseLevel = val3;
		if (skillUseLevel == 0) {
			SKILL_INFO skillInfo;
			if (SKID >= 8000 && SKID < 8017)
				skillInfo = g_session.GetHomunSkillItemInfoBySkillId(SKID);
			else if (SKID >= 8200 && SKID < 8238)
				skillInfo = g_session.GetMerSkillItemInfoBySkillId(SKID);
			else
				skillInfo = g_session.GetSkillItemInfoBySkillId(SKID);
			if (skillInfo.m_isValid) {
				skillUseLevel = skillInfo.level;
			} else {
				SKILL_INFO tmpSkillInfo = g_session.GetTempSkillItemInfoBySkillId(SKID);
				skillUseLevel = tmpSkillInfo.m_isValid ? tmpSkillInfo.level : 0;
			}
		}
		if (!((SKID >= 8000 && SKID < 8017) || (SKID >= 8200 && SKID < 8238))) {
			if (m_world->m_player->m_stateId == 6)
				return 0;
		}
		if (g_windowMgr.GetChatRoomWnd())
			return 0;
		if (SKID == 154) {
			if (g_session.GetSp() < 40) {
				char msgBuf[256];
				sprintf(msgBuf, MsgStr(MSI_USESKILL_FAIL_SP_INSUFFICIENT));
				g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)msgBuf, 0xff, 10, 0);
				return 0;
			} else if (!IsEffectStatePushCart(g_session.m_effectState)) {
				char msgBuf[256];
				sprintf(msgBuf, MsgStr(MSI_MUST_EQUIP_CART));
				g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)msgBuf, 0xff, 10, 0);
				return 0;
			}
			g_windowMgr.MakeWindow(WID_SELECTCARTWND);
		}
		PACKET_CZ_USE_SKILL p;
		p.PacketType = 0x72;
		SetPaddingValue(p.pad, 4);
		SetPaddingValue(p.pad2, 2);
		SetPaddingValue(p.pad3, 9);
		p.selectedLevel = (short)skillUseLevel;
		p.SKID = (unsigned short)SKID;
		p.targetID = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(0x72), (const char *)&p);
		break;
	}
	case MM_USE_SKILL_TOGROUND: {
		int *	values = (int *)val1;
		int		SKID = values[0];
		int		xPos = values[1];
		int		yPos = values[2];
		int		skillUseLevel = values[3];
		if (skillUseLevel == 0)
			skillUseLevel = g_session.GetSkillItemInfoBySkillId(SKID).level;

		PACKET_CZ_USE_SKILL_TOGROUND	p;
		p.PacketType	= 275;
		SetPaddingValue(p.pad, 3);
		SetPaddingValue(p.pad2, 2);
		SetPaddingValue(p.pad3, 1);
		p.selectedLevel = skillUseLevel;
		p.SKID			= SKID;
		p.xPos			= xPos;
		p.yPos			= yPos;
		g_connection.SendPacket(g_connection.GetPacketSize(275), (const char *)(&p));
		break;
	}
	case MM_USE_SKILL_TOGROUND_WITH_TALKBOX: {
		INT_PTR *values = (INT_PTR *)val1;
		PACKET_CZ_USE_SKILL_TOGROUND_WITHTALKBOX p;
		p.PacketType    = 126;
		p.selectedLevel = (short)values[3];
		p.SKID          = (unsigned short)values[0];
		p.xPos          = (short)values[1];
		p.yPos          = (short)values[2];
		strcpy(p.contents, (const char *)values[4]);
		if (g_insultFilter.IsBadSentence(p.contents)) {
			g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_NO_SEND_BECAUSE_INSULT), 0xff, 0, 0);
			break;
		}
		if (p.selectedLevel == 0)
			p.selectedLevel = g_session.GetSkillItemInfoBySkillId(p.SKID).level;
		g_connection.SendPacket(g_connection.GetPacketSize(126), (const char *)&p);
		break;
	}
	case MM_CANCEL_LOCKON: {
		PACKET_CZ_CANCEL_LOCKON p; p.PacketType = HEADER_CZ_CANCEL_LOCKON;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_CANCEL_LOCKON), (const char *)&p);
		break;
	}
	case MM_SELECT_WARPPOINT: {
		PACKET_CZ_SELECT_WARPPOINT p;
		p.PacketType = HEADER_CZ_SELECT_WARPPOINT;
		strncpy(p.mapName, (const char *)val1, 16);
		p.SKID = (unsigned short)val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_SELECT_WARPPOINT), (const char *)&p);
		break;
	}
	case MM_REMEMBER_WARPPOINT: {
		PACKET_CZ_REMEMBER_WARPPOINT p; p.PacketType = HEADER_CZ_REMEMBER_WARPPOINT;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REMEMBER_WARPPOINT), (const char *)&p);
		break;
	}
		case MM_REQ_BIRDOFF:
		case MM_REQ_CHIKENOFF:
		case MM_REQ_CARTOFF: {
			PACKET_CZ_REQ_CARTOFF p; p.PacketType = HEADER_CZ_REQ_CARTOFF;
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_CARTOFF), (const char *)&p);
			break;
		}
	case MM_REQ_CLOSESTORE: {
		PACKET_CZ_REQ_CLOSESTORE p; p.PacketType = HEADER_CZ_REQ_CLOSESTORE;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_CLOSESTORE), (const char *)&p);
		break;
	}
	case MM_REQ_OPENSTORE: {
		char packet[2048];
		int numItem = g_session.GetNumMerchantHopeToSellItem();
		PACKET_CZ_REQ_OPENSTORE2 *p = (PACKET_CZ_REQ_OPENSTORE2 *)packet;
		p->PacketType = HEADER_CZ_REQ_OPENSTORE2;
		p->PacketLength = sizeof(PACKET_CZ_REQ_OPENSTORE2) + sizeof(PRODUCTINFO_INSTORE) * numItem;
		if (val2 == TRUE) {
			strncpy(p->storeName, (char *)val1, 79);
			p->storeName[79] = 0;
			p->result = true;
		} else {
			p->storeName[0] = 0;
			p->result = false;
		}
		g_session.m_myShopName = p->storeName;
		PRODUCTINFO_INSTORE *sItem = (PRODUCTINFO_INSTORE *)(packet + sizeof(PACKET_CZ_REQ_OPENSTORE2));
		for (int i = 0; i < numItem; i++) {
			ITEM_INFO itemInfo = g_session.GetMerchantHopeToSellItemInfoBySequence(i);
			if (itemInfo.IsValid()) {
				sItem->index = itemInfo.m_itemIndex;
				Trace("MM_REQ_OPENSTORE itemInfo.m_itemIndex == %d", itemInfo.m_itemIndex);
				sItem->count = itemInfo.m_num;
				sItem->price = itemInfo.m_realPrice;
				++sItem;
			}
		}
		if (numItem) {
			g_connection.SendPacket(p->PacketLength, (const char *)packet);
		} else {
			p->result = false;
			g_connection.SendPacket(p->PacketLength, (const char *)packet);
		}
		g_session.ClearMerchantHopeToSellItem();
		break;
	}
	case MM_PC_PURCHASE_ITEMLIST_FROMMC: {
		g_session.CloneMerchantPurchaseItemToMerchantPurchaseItemSaved();

		int		numItem = g_session.GetNumMerchantPurchaseItem();

		PACKET_CZ_PC_PURCHASE_ITEMLIST_FROMMC	p;
		p.PacketType	= HEADER_CZ_PC_PURCHASE_ITEMLIST_FROMMC;
		int		packetStrLen = sizeof(PACKET_CZ_PC_PURCHASE_ITEMLIST_FROMMC);
		int		packetLength = packetStrLen+sizeof(CZ_PURCHASE_ITEM_FROMMC) * numItem;
		p.PacketLength	= packetLength;
		p.AID			= val1;

		char	packet[2048];
		sAssert(packetLength <= sizeof(packet));
		if (!(packetLength <= sizeof(packet)))
			break;
		memcpy(packet, &p, packetStrLen);

		for (int i=0; i<numItem; i++) {
			ITEM_INFO	itemInfo = g_session.GetMerchantPurchaseItemInfoBySequence(i);

			if (itemInfo.IsValid()) {
				CZ_PURCHASE_ITEM_FROMMC		pi;
				pi.count = itemInfo.m_num;
				pi.index = itemInfo.m_itemIndex;
				memcpy(packet + packetStrLen + i*sizeof(CZ_PURCHASE_ITEM_FROMMC), &pi, sizeof(CZ_PURCHASE_ITEM_FROMMC));
			}
		}
		if (numItem)
			g_connection.SendPacket(p.PacketLength, (const char *)(&packet[0]));
		break;
	}
	case MM_REQ_BUY_FROMMC: {
		UIMerchantItemShopWnd *	pWnd = g_windowMgr.GetMerchantItemShopWnd();
		if (val1 == g_session.GetAid()) break;
		if (pWnd) break;
		if (timeGetTime() <= m_reqTickMerchantShop + 1000) break;
		m_reqTickMerchantShop = timeGetTime();
		PACKET_CZ_REQ_BUY_FROMMC	p;
		p.PacketType	= HEADER_CZ_REQ_BUY_FROMMC;
		p.AID			= val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_BUY_FROMMC), (const char *)(&p));
		break;
	}
	case MM_MOVETO_MAP: {
		PACKET_CZ_MOVETO_MAP p;
		p.PacketType = HEADER_CZ_MOVETO_MAP;
		strncpy(p.mapName, (const char *)val1, 16);
		p.xPos = (short)val2;
		p.yPos = (short)val3;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_MOVETO_MAP), (const char *)&p);
		break;
	}
	case MM_CHANGE_GROUPEXPOPTION: {
		PACKET_CZ_CHANGE_GROUPEXPOPTION p;
		p.PacketType = HEADER_CZ_CHANGE_GROUPEXPOPTION;
		p.expOption = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_CHANGE_GROUPEXPOPTION), (const char *)&p);
		break;
	}
	case MM_SEND_NUMBER_TO_NPC: {
		int naid = val1;
		int value = val2;
		CGameActor *actor = m_world->GetGameActorByAID(naid);
		if (actor) {
			PACKET_CZ_INPUT_EDITDLG p;
			p.PacketType = HEADER_CZ_INPUT_EDITDLG;
			p.NAID = naid;
			p.value = value;
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_INPUT_EDITDLG), (const char *)&p);
		}
		break;
	}
	case MM_SEND_TEXT_TO_NPC: {
		int naid = val1;
		char value[256], packet[1024];
		strcpy(value, (char *)val2);
		CGameActor *actor = m_world->GetGameActorByAID(naid);
		if (actor || naid == 0) {
			PACKET_CZ_INPUT_EDITDLGSTR *p = (PACKET_CZ_INPUT_EDITDLGSTR *)packet;
			p->PacketType = HEADER_CZ_INPUT_EDITDLGSTR;
			p->NAID = naid;
			p->PacketLength = sizeof(PACKET_CZ_INPUT_EDITDLGSTR) + strlen(value) + 1;
			memcpy(packet + sizeof(PACKET_CZ_INPUT_EDITDLGSTR), value, strlen(value) + 1);
			g_connection.SendPacket(p->PacketLength, (const char *)&packet);
		}
		break;
	}
	case MM_SEND_GUILD_EMBLEM: {
		if (!g_guildInfo.m_amIMaster)
			break;
			if (val2 + (int)sizeof(PACKET_CZ_REGISTER_GUILD_EMBLEM_IMG) > 2048)
				break;
			PACKET_CZ_REGISTER_GUILD_EMBLEM_IMG p;
			p.PacketType = HEADER_CZ_REGISTER_GUILD_EMBLEM_IMG;
			p.PacketLength = (short)(val2 + sizeof(PACKET_CZ_REGISTER_GUILD_EMBLEM_IMG));
			char packet[2048];
			memcpy(packet, (const char *)&p, sizeof(p));
			memcpy(packet + sizeof(PACKET_CZ_REGISTER_GUILD_EMBLEM_IMG), (const char *)val1, val2);
			g_connection.SendPacket(p.PacketLength, (const char *)&packet[0]);
			break;
		}
	case MM_REQ_CHANGE_MEMBERPOS: {
		if (!g_guildInfo.m_amIMaster)
			break;
		int size = g_guildInfo.m_memberPositionInfoListForChange.size();
		int packetStrLen = sizeof(PACKET_CZ_REQ_CHANGE_MEMBERPOS);
		PACKET_CZ_REQ_CHANGE_MEMBERPOS p;
		p.PacketType = HEADER_CZ_REQ_CHANGE_MEMBERPOS;
		p.PacketLength = sizeof(PACKET_CZ_REQ_CHANGE_MEMBERPOS) + size*sizeof(MEMBER_POSITION_INFORMATION);

		char packet[2048];
		sAssert(2048 >= p.PacketLength);
		if (2048 < p.PacketLength) {
			return 0;
		}
		memcpy(packet, &p, packetStrLen);
		std::map<int, MEMBER_POSITION_INFORMATION>::iterator it = g_guildInfo.m_memberPositionInfoListForChange.begin();
		int i = 0;
		while (it != g_guildInfo.m_memberPositionInfoListForChange.end()) {
			MEMBER_POSITION_INFORMATION mp_info;
			mp_info.AID = it->second.AID;
			mp_info.GID = it->second.GID;
			mp_info.positionID = it->second.positionID;

			memcpy(packet + packetStrLen + i*sizeof(MEMBER_POSITION_INFORMATION), &mp_info, sizeof(MEMBER_POSITION_INFORMATION));
			++i;
			++it;
		}

		g_guildInfo.m_memberPositionInfoListForChange.clear();
		g_connection.SendPacket(p.PacketLength, (const char *)(packet));
		break;
	}
	case MM_REQ_CHANGE_GUILD_POSITIONINFO: {
		if (!g_guildInfo.m_amIMaster)
			break;
		int size = g_guildInfo.m_changedPositionInfoList.size();
		int packetStrLen = sizeof(PACKET_CZ_REG_CHANGE_GUILD_POSITIONINFO);

		PACKET_CZ_REG_CHANGE_GUILD_POSITIONINFO p;
		p.PacketType = HEADER_CZ_REG_CHANGE_GUILD_POSITIONINFO;
		p.PacketLength = sizeof(packetStrLen) + size*sizeof(GUILD_REG_POSITION_INFO);

		char packet[2048];
		sAssert(2048 >= p.PacketLength);
		if (2048 < p.PacketLength) {
			return 0;
		}
		memcpy(packet, &p, packetStrLen);
		std::map<int, GUILD_POSITION_INFORMATION>::iterator it = g_guildInfo.m_changedPositionInfoList.begin();
		int i = 0;
		while (it != g_guildInfo.m_changedPositionInfoList.end()) {
			GUILD_REG_POSITION_INFO info;
			info.positionID = it->second.positionID;
			int right = 0;
			if (it->second.rightOfJoin) {
				right |= GUILD_RIGHT_JOIN;
			}
			if (it->second.rightOfPunish) {
				right |= GUILD_RIGHT_PUNISHMENT;
			}
			info.right = right;
			info.ranking = it->second.ranking;
			info.payRate = it->second.payRate;

			strncpy(info.posName, it->second.posName.c_str(), 24-1);
			info.posName[24-1] = 0;

			memcpy(packet + packetStrLen + i*sizeof(GUILD_REG_POSITION_INFO), &info, sizeof(GUILD_REG_POSITION_INFO));
			++i;
			++it;
		}

		g_guildInfo.m_changedPositionInfoList.clear();
		g_connection.SendPacket(p.PacketLength, (const char *)(packet));
		break;
	}
	case MM_REQ_GUILD_MENUINTERFACE: {
		PACKET_CZ_REQ_GUILD_MENUINTERFACE p; p.PacketType = HEADER_CZ_REQ_GUILD_MENUINTERFACE;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_GUILD_MENUINTERFACE), (const char *)&p);
		break;
	}
	case MM_REQ_DISORGANIZE_GUILD: {
		if (!g_guildInfo.m_amIMaster) {
			break;
		}
		const char *key = (const char *)val1;

		PACKET_CZ_REQ_DISORGANIZE_GUILD p;
		p.PacketType = HEADER_CZ_REQ_DISORGANIZE_GUILD;
		strncpy(p.key, key, 40-1);
		p.key[40-1] = 0;

		if (strcmp(key, g_guildInfo.m_guildName.c_str()))
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_DISORGANIZE_GUILD_FAILURE), 0xff, 4, 0);
		else
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_DISORGANIZE_GUILD), (const char *)(&p));
		break;
	}
	case MM_REQ_GUILD_MENU: {
		PACKET_CZ_REQ_GUILD_MENU p;
		p.PacketType = HEADER_CZ_REQ_GUILD_MENU;
		p.Type = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_GUILD_MENU), (const char *)&p);
		break;
	}
	case MM_REQ_GUILD_EMBLEM_IMG: {
		if (val1 == 0)
			break;
		std::list<DWORD>::iterator it = m_emblemReqGdidQueue.begin();
		while (it != m_emblemReqGdidQueue.end()) {
			if (*it != val1)
				it++;
			else {
				it = m_emblemReqGdidQueue.erase(it);
			}
		}

		if (g_session.GetEmblemState() == false) break;

		PACKET_CZ_REQ_GUILD_EMBLEM_IMG	p;
		p.PacketType	= HEADER_CZ_REQ_GUILD_EMBLEM_IMG;
		p.GDID			= val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_GUILD_EMBLEM_IMG), (const char *)(&p));
		break;
	}
	case MM_REQ_OPEN_MEMBER_INFO: {
		PACKET_CZ_REQ_OPEN_MEMBER_INFO p;
		p.PacketType = HEADER_CZ_REQ_OPEN_MEMBER_INFO;
		p.AID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_OPEN_MEMBER_INFO), (const char *)&p);
		break;
	}
	case MM_REQ_LEAVE_GUILD: {
		const char *reasonDesc = (const char *)val1;
		PACKET_CZ_REQ_LEAVE_GUILD p;
		p.PacketType = HEADER_CZ_REQ_LEAVE_GUILD;
		p.GDID = g_guildInfo.m_gdid;
		p.AID = g_session.GetAid();
		p.GID = g_session.GetGid();
		if (g_multiLang) {
			strcpy(p.reasonDesc, g_language->GetLanguageCharset(0));
			strncpy(p.reasonDesc + 3, reasonDesc, 40 - 1 - 3);
		}
		else
			strncpy(p.reasonDesc, reasonDesc, 40 - 1);
		p.reasonDesc[40 - 1] = 0;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_LEAVE_GUILD), (const char *)(&p));
		break;
	}
	case MM_REQ_BAN_GUILD: {
		if (!g_guildInfo.m_amIMaster && !g_guildInfo.m_rightPunish)
			break;

		int *vars = (int *)val1;
		const char *reasonDesc = (const char *)val2;
		PACKET_CZ_REQ_BAN_GUILD p;
		p.PacketType = HEADER_CZ_REQ_BAN_GUILD;
		p.GDID = vars[0];
		p.AID = vars[1];
		p.GID = vars[2];

		if (g_multiLang) {
			strcpy(p.reasonDesc, g_language->GetLanguageCharset(0));
			strncpy(p.reasonDesc + 3, reasonDesc, 40 - 1 - 3);
		}
		else
			strncpy(p.reasonDesc, reasonDesc, 40 - 1);
		p.reasonDesc[40 - 1] = 0;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_BAN_GUILD), (const char *)(&p));
		break;
	}
	case MM_REQ_MAKE_GUILD: {
		const char *gName = (const char *)val1;

		char compName[2];
		char msg[256];

		memcpy(compName, gName, 2);
		if (strcmp(compName, "\xa1\xa1") == 0) {
			sprintf(msg, "Can NOT use space in Guild Name !!");
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)msg, (int)RGB(255, 150, 110), 0, 0);
			break;
		}

		memcpy(compName, gName + strlen(gName) - 2, 2);
		if (strcmp(compName, "\xa1\xa1") == 0) {
			sprintf(msg, "Can NOT use space in Guild Name !!");
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)msg, (int)RGB(255, 150, 110), 0, 0);
			break;
		}

		if (g_serviceType == ServiceBrazil && strchr(gName, '##') != NULL) {
			g_windowMgr.ErrorMsg(MsgStr(MSI_BILLING_1), ME_OK, 1, 0, 0);
			break;
		}
		if (strlen(gName) >= 24) {
			g_windowMgr.ErrorMsg(MsgStr(MSI_NAMELENGTH_TOO_LONG), ME_OK, 1, 0, 0);
			break;
		}

		PACKET_CZ_REQ_MAKE_GUILD p;
		p.PacketType = HEADER_CZ_REQ_MAKE_GUILD;
		p.GID = g_session.GetGid();
		strncpy(p.GName, gName, 24 - 1);
		p.GName[24 - 1] = 0;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_MAKE_GUILD), (const char *)(&p));
		break;
	}
	case MM_JOIN_GUILD: {
		PACKET_CZ_JOIN_GUILD p;
		p.PacketType = HEADER_CZ_JOIN_GUILD;
		p.GDID = val1;
		p.answer = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_JOIN_GUILD), (const char *)&p);
		break;
	}
	case MM_REQ_JOIN_GUILD: {
		PACKET_CZ_REQ_JOIN_GUILD p;
		p.PacketType = HEADER_CZ_REQ_JOIN_GUILD;
		p.AID = val1;
		p.MyGID = g_session.GetGid();
		p.MyAID = g_session.GetAid();
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_JOIN_GUILD), (const char *)&p);
		break;
	}
	case MM_TESTPACKET: {
		struct {
			short PacketType;
			short PacketLength;
		} packet;
		packet.PacketType = 0xf3;
		packet.PacketLength = 2;
		g_connection.SendPacket(sizeof(DWORD), (const char *)&packet);
		break;
	}
	case MM_REQ_GIVE_MANNER_POINT: {
		if (g_serverType == ServerInstantEvent)
			break;
		PACKET_CZ_REQ_GIVE_MANNER_POINT p;
		p.PacketType = HEADER_CZ_REQ_GIVE_MANNER_POINT;
		p.otherAID = val1;
		p.type = (char)val2;
		int point = (val3 > 0) ? val3 : 0;
		p.point = (short)point;
		if (point >= 30000)
			p.point = 30000;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_GIVE_MANNER_POINT), (const char *)&p);
		break;
	}
	case MM_GUILD_NOTICE: {
		const char *subject = (const char *)val2;
		const char *notice = (const char *)val3;
		PACKET_CZ_GUILD_NOTICE p;
		p.PacketType = HEADER_CZ_GUILD_NOTICE;
		p.GDID = val1;
		if (g_multiLang) {
			strcpy(p.subject, g_language->GetLanguageCharset(0));
			strncpy(p.subject + 3, subject, 60 - 1 - 3);
			strcpy(p.notice, g_language->GetLanguageCharset(0));
			strncpy(p.notice + 3, notice, 120 - 1 - 3);
		} else {
			strncpy(p.subject, subject, 60 - 1);
			strncpy(p.notice, notice, 120 - 1);
		}
		p.subject[60 - 1] = 0;
		p.notice[120 - 1] = 0;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_GUILD_NOTICE), (const char *)&p);
		break;
	}
	case MM_REQ_ALLY_GUILD: {
		PACKET_CZ_REQ_ALLY_GUILD p;
		p.PacketType = HEADER_CZ_REQ_ALLY_GUILD;
		p.AID = val1;
		p.MyAID = g_session.GetAid();
		p.MyGID = g_session.GetGid();
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_ALLY_GUILD), (const char *)&p);
		break;
	}
	case MM_ALLY_GUILD: {
		PACKET_CZ_ALLY_GUILD p;
		p.PacketType = HEADER_CZ_ALLY_GUILD;
		p.otherAID = val1;
		p.answer = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_ALLY_GUILD), (const char *)&p);
		break;
	}
	case MM_REQ_ITEMIDENTIFY: {
		PACKET_CZ_REQ_ITEMIDENTIFY p;
		p.PacketType = HEADER_CZ_REQ_ITEMIDENTIFY;
		p.index = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_ITEMIDENTIFY), (const char *)&p);
		break;
	}
	case MM_PETEGG_SELECT_FROM_LIST: {
		PACKET_CZ_SELECT_PETEGG p;
		p.PacketType = HEADER_CZ_SELECT_PETEGG;
		p.index = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_SELECT_PETEGG), (const char *)&p);
		break;
	}
	case MM_REPAIRITEM_SELECT_FROM_LIST: {
		PACKET_CZ_REQ_ITEMREPAIR p;
		p.PacketType = HEADER_CZ_REQ_ITEMREPAIR;
		if (val1 == -1)
			p.TargetItemInfo.index = (short)val1;
		else
			memcpy(&p.TargetItemInfo, (REPAIRITEM_INFO *)val1, sizeof(REPAIRITEM_INFO));
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_ITEMREPAIR), (const char *)&p);
		break;
	}
	case MM_REFINEITEM_SELECT_FROM_LIST: {
		PACKET_CZ_REQ_WEAPONREFINE p; p.PacketType = HEADER_CZ_REQ_WEAPONREFINE;
		p.Index = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_WEAPONREFINE), (const char *)&p);
		break;
	}
	case MM_REQ_ITEMCOMPOSITION_LIST: {
		m_lastCardItemIndex = val1;
		PACKET_CZ_REQ_ITEMCOMPOSITION_LIST p; p.PacketType = HEADER_CZ_REQ_ITEMCOMPOSITION_LIST;
		p.cardIndex = (short)val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_ITEMCOMPOSITION_LIST), (const char *)&p);
		break;
	}
	case MM_REQ_ITEMCOMPOSITION: {
		PACKET_CZ_REQ_ITEMCOMPOSITION p;
		p.PacketType = HEADER_CZ_REQ_ITEMCOMPOSITION;
		p.equipIndex = val1;
		p.cardIndex = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_ITEMCOMPOSITION), (const char *)&p);
		break;
	}
	case MM_REQ_HOSTILE_GUILD: {
		PACKET_CZ_REQ_HOSTILE_GUILD p;
		p.PacketType = HEADER_CZ_REQ_HOSTILE_GUILD;
		p.AID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_HOSTILE_GUILD), (const char *)&p);
		break;
	}
	case MM_REQ_DELETE_RELATED_GUILD: {
		PACKET_CZ_REQ_DELETE_RELATED_GUILD p;
		p.PacketType = HEADER_CZ_REQ_DELETE_RELATED_GUILD;
		p.OpponentGDID = val1;
		p.Relation = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_DELETE_RELATED_GUILD), (const char *)&p);
		break;
	}
	case MM_REQ_DISCONNECT: {
		PACKET_CZ_REQ_DISCONNECT p; p.PacketType = HEADER_CZ_REQ_DISCONNECT;
		p.type = 0;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_DISCONNECT), (const char *)&p);
		break;
	}
	case MM_REQ_MAKING_ITEM: {
		PACKET_CZ_REQMAKINGITEM		p;
		ITEM_INFO *		pMaterialiteminfo = (ITEM_INFO *)val2;

		p.PacketType			= HEADER_CZ_REQMAKINGITEM;
		p.info.ITID				= val1;
		p.info.material_ID[0]	= atoi(pMaterialiteminfo[0].m_itemName.c_str());
		p.info.material_ID[1]	= atoi(pMaterialiteminfo[1].m_itemName.c_str());
		p.info.material_ID[2]	= atoi(pMaterialiteminfo[2].m_itemName.c_str());
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQMAKINGITEM), (const char *)(&p));
		break;
	}
	case MM_BEGIN_DRAG_FROM_METALPROCESSWND: {
		ResetDrag();
		char *		itemName = (char *)val1;
		m_dragInfo.m_dragType		= DT_FROM_METALPROCESSWND;
		m_dragInfo.m_dragItemName	= itemName;
		m_dragInfo.m_dragSprName	= g_session.GetItemSpriteFileName(itemName, true);
		break;
	}
	case MM_ITEMPICKUP: {
		PACKET_CZ_ITEM_PICKUP p;
		p.PacketType = 0xf5;
		SetPaddingValue(p.pad, 2);
		p.ITAID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)&p);
		break;
	}
	case MM_REQUEST_ACT: {
		PACKET_CZ_REQUEST_ACT p;
		p.PacketType = 0x190;
		SetPaddingValue(p.pad, 3);
		SetPaddingValue(p.pad2, 9);
		p.action = (unsigned char)val1;
		p.targetGID = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)&p);
		break;
	}
	case MM_REQUEST_MOVE: {
		PACKET_CZ_REQUEST_MOVE p;
		p.PacketType = 0xa7;
		SetPaddingValue(p.pad, 3);
		EncodePosDir(val1, val2, 0, p.dest);
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)&p);
		break;
	}
	case MM_CHANGE_DIRECTION: {
		PACKET_CZ_CHANGE_DIRECTION p;
		p.PacketType = 0x85;
		SetPaddingValue(p.pad, 5);
		SetPaddingValue(p.pad2, 1);
		p.dir = (unsigned char)val1;
		p.headDir = (short)val2;
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)&p);
		break;
	}
	case MM_GUILD_MEMBER_STATUS_CHANGE: {
		if (g_session.m_isLogInOut != TRUE)
			break;
		char msg[128];
		switch (val2) {
		case 0:
			sprintf(msg, MsgStr(MSI_GUILD_MEMBER_STATUS_OFFLINE), (const char *)val1);
			break;
		case 1:
			sprintf(msg, MsgStr(MSI_GUILD_MEMBER_STATUS_ONLINE), (const char *)val1);
			break;
		default:
			return 0;
		}
		g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)msg, 0xffff00, NMSG_GUILD, 0);
		break;
	}
	case MM_QUIT_TO_IDENTRY:
		g_mustPumpOutReceiveQueue = 0;
		g_loginStartMode = LSM_IDENTRY;
		g_modeMgr.Switch(MT_LOGIN, "login.rsw");
		break;
	case MM_RESET_PARAMETER: {
		PACKET_CZ_RESET p;
		p.PacketType = HEADER_CZ_RESET;
		switch (val1) {
		case 1:  p.type = RESET_SKILL; break;
		default: p.type = RESET_STAT;  break;
		}
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)&p);
		break;
	}
	case MM_REQ_CHANGE_MAPTYPE: {
		PACKET_CZ_CHANGE_MAPTYPE p;
		p.PacketType = HEADER_CZ_CHANGE_MAPTYPE;
		p.xPos = (short)val1;
		p.yPos = (short)val2;
		p.type = val3 ? 1 : 0;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_CHANGE_MAPTYPE), (const char *)&p);
		break;
	}
	case MM_CHANGE_EFFECTSTATE: {
		PACKET_CZ_CHANGE_EFFECTSTATE p;
		p.PacketType = HEADER_CZ_CHANGE_EFFECTSTATE;
		p.EffectState = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_CHANGE_EFFECTSTATE), (const char *)&p);
		break;
	}
	case MM_PET_RENAME: {
		PACKET_CZ_RENAME_PET p;
		p.PacketType = HEADER_CZ_RENAME_PET;
		int size = strlen((char *)val1);
		if (size < 24) {
			size++;
			memcpy(p.szName, (char *)val1, size);
		} else {
			memcpy(p.szName, (char *)val1, 24);
			p.szName[23] = 0;
		}
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)&p);
		break;
	}
	case MM_REQ_PETEGG_INFO: {
		PACKET_CZ_PETEGG_INFO p;
		p.PacketType = HEADER_CZ_PETEGG_INFO;
		p.index = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_PETEGG_INFO), (const char *)&p);
		break;
	}
	case MM_PET_ACT: {
		if (val1 == -1)
			return 0;
		if (val1 >= 50000 && (g_session.m_petFriendly / 10) < 90 && !g_session.IsMasterAid(3))
			return 0;

		PACKET_CZ_PET_ACT p;
		p.PacketType = HEADER_CZ_PET_ACT;
		p.data = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)&p);
		break;
	}
	case MM_COMMAND_PET: {
		PACKET_CZ_COMMAND_PET p;
		p.PacketType = HEADER_CZ_COMMAND_PET;
		p.cSub = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_COMMAND_PET), (const char *)&p);
		break;
	}
	case MM_PETDECEIVE_SHOOT: {
		PACKET_CZ_TRYCAPTURE_MONSTER p;
		p.PacketType = HEADER_CZ_TRYCAPTURE_MONSTER;
		p.targetAID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_TRYCAPTURE_MONSTER), (const char *)&p);
		break;
	}
	case MM_REQ_GIVE_MANNER_POINT_PLUS:
		SendMsg(MM_REQ_GIVE_MANNER_POINT, m_menuTargetAID, MP_PLUS, val1);
		g_session.EraseProhibit(m_menuTargetAID, val1);
		break;
	case MM_REQ_GIVE_MANNER_POINT_MINUS:
		SendMsg(MM_REQ_GIVE_MANNER_POINT, m_menuTargetAID, MP_MINUS, val1);
		g_session.AppendProhibit(m_menuTargetAID, val1);
		break;
	case MM_ARROW_FROM_LIST: {
		PACKET_CZ_REQ_MAKINGARROW p; p.PacketType = HEADER_CZ_REQ_MAKINGARROW;
		p.id = (unsigned short)val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_MAKINGARROW), (const char *)&p);
		break;
	}
	case MM_ITEM_FROM_LIST: {
		PACKET_CZ_REQ_MAKINGITEM p; p.PacketType = HEADER_CZ_REQ_MAKINGITEM;
		p.id = (unsigned short)val1;
		p.mkType = (short)val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_MAKINGITEM), (const char *)&p);
		break;
	}
	case MM_SELECT_CART: {
		PACKET_CZ_REQ_CHANGECART p; p.PacketType = HEADER_CZ_REQ_CHANGECART;
		p.num = (short)val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_CHANGECART), (const char *)&p);
		break;
	}
	case MM_ITEM_CREATE: {
		const char *itemName = (const char *)val1;
		if (itemName == NULL) break;
		PACKET_CZ_ITEM_CREATE p;
		p.PacketType = HEADER_CZ_ITEM_CREATE;
		strncpy(p.itemName, itemName, 24);
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_ITEM_CREATE), (const char *)&p);
		break;
	}
	case MM_REMOVE_TO_AID: {
		PACKET_CZ_REMOVE_AID p;
		p.PacketType = HEADER_CZ_REMOVE_AID;
		memset(p.AccountName, 0, 24);
		strcpy(p.AccountName, (char *)val1);
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REMOVE_AID), (const char *)&p);
		break;
	}
	case MM_SHIFT_TO_GID: {
		PACKET_CZ_SHIFT p;
		p.PacketType = HEADER_CZ_SHIFT;
		memset(p.CharacterName, 0, 24);
		strcpy(p.CharacterName, (char *)val1);
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_SHIFT), (const char *)&p);
		break;
	}
	case MM_RECALL_AID: {
		PACKET_CZ_RECALL p;
		p.PacketType = HEADER_CZ_RECALL;
		memset(p.AccountName, 0, 24);
		strcpy(p.AccountName, (char *)val1);
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_RECALL), (const char *)&p);
		break;
	}
		case MM_RECALL_GID: {
			PACKET_CZ_RECALL_GID p;
			p.PacketType = HEADER_CZ_RECALL_GID;
			memset(p.CharacterName, 0, 24);
			strcpy(p.CharacterName, (char *)val1);
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_RECALL_GID), (const char *)&p);
			break;
		}
		case MM_FORCING_MOVE_AID:
		case MM_FORCING_MOVE_GID:
			break;
		case MM_CHECK_ID: {
			PACKET_CZ_REQ_STATUS_GM p;
			p.PacketType = HEADER_CZ_REQ_STATUS_GM;
			memset(p.CharName, 0, NAME_LENGTH);
			memcpy(p.CharName, (char *)val1, NAME_LENGTH);
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_STATUS_GM), (const char *)(&p));

			char msg[256];
			sprintf(msg, "[ %s ]", p.CharName);
				g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)msg, RGB(155,250,110), 0, 0);
				break;
			}
	case MM_RC_ID: {
		PACKET_CZ_REQ_GIVE_MANNER_BYNAME p;
		p.PacketType = HEADER_CZ_REQ_GIVE_MANNER_BYNAME;
		memset(p.CharName, 0, 24);
		memcpy(p.CharName, (char *)val1, 24);
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_GIVE_MANNER_BYNAME), (const char *)&p);
		break;
	}
	case MM_BILLING_INFO: {
		PACKET_CZ_REQ_REMAINTIME p; p.PacketType = HEADER_CZ_REQ_REMAINTIME;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_REMAINTIME), (const char *)&p);
		break;
	}
	case MM_SELECT_SPELL: {
		PACKET_CZ_SELECTAUTOSPELL p; p.PacketType = HEADER_CZ_SELECTAUTOSPELL;
		p.SKID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_SELECTAUTOSPELL), (const char *)&p);
		break;
	}
		case MM_REQ_ACCOUNT_NAME:
			PACKET_CZ_REQ_ACCOUNTNAME p;
			p.PacketType = HEADER_CZ_REQ_ACCOUNTNAME;
			p.AID = val1;
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_ACCOUNTNAME), (const char *)&p);
			break;
		case MM_SONG_TALK: {
		CFile fp;
		char *fName = "";
		if (g_serviceType != ServiceGermany) {
			if (val1 == 0)
				fName = "DC_scream.txt";
			else if (val1 == 1)
				fName = "BA_frostjoke.txt";
		} else {
			if (val1 == 0)
				fName = "english\\DC_scream.txt";
			else if (val1 == 1)
				fName = "english\\BA_frostjoke.txt";
		}
		if (!fp.Open(fName, CFile::modeRead))
			return 0;

		int size = fp.GetLength();
		char *buf = new char[size + 1];
		fp.Read(buf, size);
		buf[size] = 0;
		fp.Close();

		std::string a;
		a = "";
		std::vector<std::string> tmpList;

			char st = 0;
			char msg[256];
			int randomline;
			if (val1 == 0)
				randomline = (rand() % 114) + 1;
			else if (val1 == 1)
				randomline = (rand() % 117) + 1;

			int i = 0;
			if (i < size) {
				char *msgp = msg;
				do {
					if (buf[i] == 13) {
						st++;
					}
					if (st == randomline) {
						if (buf[i] != 13 && buf[i] != 10 && buf[i] != 9) {
							*msgp = buf[i];
							msgp++;
							*msgp = 0;
						}
					} else if (st == randomline + 1) {
						break;
					}
					i++;
				} while (i < size);
			}

		if (buf)
			delete[] buf;

		char chat[247];
		sprintf(chat, "%s : %s", g_session.GetCharName(), DataTxtDecode(msg));
		chat[246] = 0;
		unsigned int chatSize = strlen(chat);
		g_windowMgr.ClearChatMsg();

		PACKET_CZ_REQUEST_CHAT p;
		p.PacketType   = 0xf3;
		p.PacketLength = (short)(sizeof(p) + chatSize + 1);
		char packet[1024];
		memcpy(packet, &p, sizeof(p));
		memcpy(packet + sizeof(p), chat, chatSize + 1);
		g_connection.SendPacket(p.PacketLength, (const char *)packet);
		break;
	}
	case MM_JOIN_FRIEND: {
		if (val3) {
			UIPetTamingDeceiveWnd *pWnd = (UIPetTamingDeceiveWnd *)g_windowMgr.MakeWindow(WID_MESSENGERGROUPWND);
			pWnd->SendMsg(0, UM_COMMAND, 149, 1, 0, 0);
		}

		PACKET_CZ_ACK_REQ_ADD_FRIENDS	p;
		p.PacketType	= HEADER_CZ_ACK_REQ_ADD_FRIENDS;
		p.ReqAID		= val1;
		p.ReqGID		= val2;
		p.Result		= val3;
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)(&p));
		break;
	}
		case MM_DELETE_FRIEND: {
			PACKET_CZ_DELETE_FRIENDS p;
			p.PacketType = HEADER_CZ_DELETE_FRIENDS;
			p.AID = val1;
			p.GID = val2;
			g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_DELETE_FRIENDS), (const char *)&p);
			break;
		}
#if defined (STORE_PASSWORD)
	case MM_ACK_STORE_PASSWORD:
		{
#if defined (CASH_PASSWORD)
			if(g_session.m_isCashPassword)
			{
				g_session.m_isCashPassword = false;
				PACKET_CZ_ACK_CASH_PASSWORD p;
				p.PacketType = HEADER_CZ_ACK_CASH_PASSWORD;

				EncryptStorePassword (val1, p.Password);
				EncryptStorePassword (val3, p.NewPassword);
				p.Type = val2;
				g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)(&p));
				break;
			}
			else
			{
#endif
				PACKET_CZ_ACK_STORE_PASSWORD p;
				p.PacketType = HEADER_CZ_ACK_STORE_PASSWORD;

				EncryptStorePassword (val1, p.Password);
				EncryptStorePassword (val3, p.NewPassword);
				p.Type = val2;
				g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)(&p));
#if defined (CASH_PASSWORD)
			}
#endif
		}
		break;
#endif

	case MM_COMMAND_HOMUN: {
		PACKET_CZ_COMMAND_MER p; p.PacketType = HEADER_CZ_COMMAND_MER;
		p.type = 0;
		p.command = (char)val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_COMMAND_MER), (const char *)&p);
		break;
	}
	case MM_MER_RENAME: {
		PACKET_CZ_RENAME_PET p;
		p.PacketType = HEADER_CZ_RENAME_MER;
		int size = strlen((char *)val1);
		if (size < 24) {
			size++;
			memcpy(p.szName, (char *)val1, size);
		} else {
			memcpy(p.szName, (char *)val1, 24);
			p.szName[23] = 0;
		}
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)&p);
		break;
	}
	case MM_COMMAND_MER: {
		PACKET_CZ_MER_COMMAND p;
		p.PacketType = HEADER_CZ_MER_COMMAND;
		p.command = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_MER_COMMAND), (const char *)&p);
		break;
	}
		case MM_GET_MAIL_LIST: {
		PACKET_CZ_MAIL_GET_LIST p; p.PacketType = HEADER_CZ_MAIL_GET_LIST;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_MAIL_GET_LIST), (const char *)&p);
		break;
	}
	case MM_SEND_MAIL: {
		int txtLen = strlen((char *)val3) + 1;

		PACKET_CZ_MAIL_SEND	p;
		p.PacketType	= HEADER_CZ_MAIL_SEND;
		memcpy(p.ReceiveName, (char *)val1, NAME_LENGTH);
		p.ReceiveName[strlen((char *)val1)] = 0;

		int	len = strlen((char *)val2);
			memcpy(p.Header, (char *)val2, len);
			p.Header[len] = 0;

			p.PacketLength = sizeof(p) + txtLen;

			char	packet[1024];
			sAssert(p.PacketLength <= sizeof(packet));
			memcpy(packet, (const char *)(&p), sizeof(p));

		memcpy(packet + sizeof(p), (char *)val3, txtLen);

		g_connection.SendPacket(p.PacketLength, (const char *)&packet[0]);
		break;
	}
	case MM_RESET_MAIL_ITEM: {
		PACKET_CZ_MAIL_RESET_ITEM p;
		p.PacketType = HEADER_CZ_MAIL_RESET_ITEM;
		p.Type = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_MAIL_RESET_ITEM), (const char *)&p);
		break;
	}
	case MM_OPEN_MAIL: {
		PACKET_CZ_MAIL_OPEN p;
		p.PacketType = HEADER_CZ_MAIL_OPEN;
		p.MailID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_MAIL_OPEN), (const char *)&p);
		break;
	}
	case MM_ADD_MAIL_ITEM: {
		PACKET_CZ_MAIL_ADD_ITEM		p;
		p.PacketType	= HEADER_CZ_MAIL_ADD_ITEM;
		p.index	= val1;
		g_session.m_mailItemCnt = p.count = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)(&p));
		break;
	}
	case MM_RETURN_MAIL: {
		PACKET_CZ_REQ_MAIL_RETURN p;
		p.PacketType = HEADER_CZ_REQ_MAIL_RETURN;
		p.MailID = val1;
		memcpy(p.ReceiveName, (char *)val2, sizeof(p.ReceiveName));
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)&p);
		break;
	}
	case MM_DELETE_MAIL: {
		PACKET_CZ_MAIL_DELETE p;
		p.PacketType = HEADER_CZ_MAIL_DELETE;
		p.MailID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_MAIL_DELETE), (const char *)&p);
		break;
	}
	case MM_GET_MAIL_ITEM: {
		PACKET_CZ_MAIL_GET_ITEM p;
		p.PacketType = HEADER_CZ_MAIL_GET_ITEM;
		p.MailID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_MAIL_GET_ITEM), (const char *)&p);
		break;
	}
	case MM_BEGIN_DRAG_FROM_MAILWND: {
		g_windowMgr.ShowWindow(WID_ITEMDROPCNTWND, 0);
		ITEM_INFO	itemInfo = g_session.m_mailItem;
		ResetDrag();
		if (itemInfo.IsValid()) {
			m_dragInfo.m_dragType		= DT_FROM_MAILWND;
			m_dragInfo.m_dragItemIndex	= itemInfo.m_itemIndex;
			m_dragInfo.m_isIdentified	= itemInfo.m_isIdentified;
			m_dragInfo.m_numDragItem	= itemInfo.m_num;
			m_dragInfo.m_dragSprName	= g_session.GetItemSpriteFileName(itemInfo.m_itemName.c_str(), itemInfo.m_isIdentified);
			m_dragInfo.m_dragItemName	= itemInfo.m_itemName;
			m_dragInfo.m_slotNum		= -1;
			m_dragInfo.m_refiningLevel = itemInfo.m_refiningLevel;
			memcpy(m_dragInfo.m_slot, itemInfo.m_slot, sizeof(itemInfo.m_slot));
		}
		break;
	}
	case MM_RESET_AUCTION_ITEM: {
		PACKET_CZ_AUCTION_CREATE p;
		p.PacketType = HEADER_CZ_AUCTION_CREATE;
		p.Type = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_AUCTION_CREATE), (const char *)&p);
		break;
	}
	case MM_ADD_AUCTION_ITEM: {
		PACKET_CZ_AUCTION_ADD_ITEM p;
		p.PacketType = HEADER_CZ_AUCTION_ADD_ITEM;
		p.index = val1;
		p.count = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_AUCTION_ADD_ITEM), (const char *)&p);
		break;
	}
	case MM_ADD_AUCTION: {
		PACKET_CZ_AUCTION_ADD p;
		p.PacketType = HEADER_CZ_AUCTION_ADD;
		p.NowMoney = val1;
		p.MaxMoney = val2;
		p.DeleteHour = val3;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_AUCTION_ADD), (const char *)&p);
		break;
	}
	case MM_CANCEL_ADD_AUCTION: {
		PACKET_CZ_AUCTION_ADD_CANCEL p;
		p.PacketType = HEADER_CZ_AUCTION_ADD_CANCEL;
		p.AuctionID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_AUCTION_ADD_CANCEL), (const char *)&p);
		break;
	}
	case MM_BUY_AUCTION: {
		PACKET_CZ_AUCTION_BUY p;
		p.PacketType = HEADER_CZ_AUCTION_BUY;
		p.AuctionID = val1;
		p.Money = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_AUCTION_BUY), (const char *)&p);
		break;
	}
	case MM_SEARCH_AUCTION_ITEM: {
		PACKET_CZ_AUCTION_ITEM_SEARCH p;
		p.PacketType = HEADER_CZ_AUCTION_ITEM_SEARCH;
		p.Type = (short)val1;
		p.Page = (unsigned short)val3;
		if (val1 == 4)
			memcpy((char *)p.Name, (char *)val2, sizeof(p.Name));
		else if (val1 == 5)
			p.AuctionID = val2;
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)&p);
		break;
	}
	case MM_AUCTION_REQ_MY_INFO: {
		PACKET_CZ_AUCTION_REQ_MY_INFO p;
		p.PacketType = HEADER_CZ_AUCTION_REQ_MY_INFO;
		p.Type = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_AUCTION_REQ_MY_INFO), (const char *)&p);
		break;
	}
	case MM_AUCTION_MY_SELL_STOP: {
		PACKET_CZ_AUCTION_REQ_MY_SELL_STOP p;
		p.PacketType = HEADER_CZ_AUCTION_REQ_MY_SELL_STOP;
		p.AuctionID = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_AUCTION_REQ_MY_SELL_STOP), (const char *)&p);
		break;
	}
	case MM_REQ_HUNTING: {
		PACKET_CZ_HUNTINGLIST p; p.PacketType = HEADER_CZ_HUNTINGLIST;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_HUNTINGLIST), (const char *)&p);
		break;
	}
	case MM_REQ_BUY_CASH_POINT: {
		int		itemSize = g_session.GetNumPurchaseItem();

		if (itemSize && !m_isPlayerDead)
		{
			PACKET_CZ_PC_BUY_CASH_POINT_ITEM p;
			p.PacketType = HEADER_CZ_PC_BUY_CASH_POINT_ITEM;

			ITEM_INFO itemInfo = g_session.GetPurchaseItemInfoBySequence(0);
			p.count = itemInfo.m_num;
			p.ITID = atoi(itemInfo.m_itemName.c_str());
			p.FreePoint = val1;

			g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)(&p));
		}

		g_session.ClearDealItems();
		break;
	}
	case MM_REBIRTH: {
		PACKET_CZ_STANDING_RESURRECTION p; p.PacketType = HEADER_CZ_STANDING_RESURRECTION;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_STANDING_RESURRECTION), (const char *)&p);
		break;
	}
	case MM_SHORTCUT_KEY_CHANGE: {
		PACKET_CZ_SHORTCUT_KEY_CHANGE	p;
		p.PacketType = HEADER_CZ_SHORTCUT_KEY_CHANGE;
		p.ShortCutKey = g_session.m_shortCutList[val1];
		p.Index = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(p.PacketType), (const char *)(&p));
		break;
	}
	case MM_REQ_ITEM_EXPLANATION_BYNAME:
		break;
	case MM_PKMODE_CHANGE:
		break;
	case MM_CHANGE_SKILL_NAME:
		break;
	case MM_OPEN_EQUIPEDITEM_CHANGE: {
		PACKET_CZ_CONFIG p;
		p.PacketType = HEADER_CZ_CONFIG;
		p.Config = PACKET_CZ_CONFIG::CONFIG_OPEN_EQUIPMENT_WINDOW;
		p.Value = val1;
		g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_CONFIG), (const char *)&p);
	}

	default:
		return CMode::SendMsg(messageId, val1, val2, val3);
	}
	return 0;
}


#include "MercenaryAI.h"


void CGameMode::ChatDori()
{
	unsigned int aid = g_session.GetAid();
	if (IsGravityAid(aid))
		return;

	if (g_serverType == ServerInstantEvent)
		return;

	PACKET_CZ_REQ_GIVE_MANNER_POINT p;
	p.PacketType = HEADER_CZ_REQ_GIVE_MANNER_POINT;
	p.otherAID = g_session.GetAid();
	p.type = MP_SELF;
	p.point = 10;
	g_connection.SendPacket(g_connection.GetPacketSize(HEADER_CZ_REQ_GIVE_MANNER_POINT), (const char *)&p);
}






#pragma pack()
