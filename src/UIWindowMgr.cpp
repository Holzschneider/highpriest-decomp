#include "common.h"
#include "bf_setting_key.h"
#include "bf_sys.h"
#include "bf_input.h"
#include "device/Control.h"
#include "GameMode.h"
#include "3dlib/DrawUtil.h"
#include "device/Connection.h"
#include "MsgStrings.h"
#include "framework/Locale.h"
#include "framework/Language.h"
#include "SnapMgr.h"
#include "device/Sound.h"
#include "resource/Wave.h"
#include "window/UISys.h"
#include "window/UIFrameWnd.h"
#include "window/UIIllustWnd.h"
#include "window/UIImeWnd.h"
#include "window/UIEmotionWnd.h"
#include "window/UIEmotionListWnd.h"
#include "UIGuildWnd.h"
#include "UIMessengerWnd.h"
#include "GameActor.h"
#include "RagEffect.h"
#include "3dActor.h"
#include "Session.h"
#include "3dlib/Renderer.h"
#include "3dlib/Device.h"
#include "3dlib/Texture.h"
#include "resource/Bitmap.h"
#include "window/UIMetalWorkWnd.h"
#include "resource/Skybox.h"
#include "Mode.h"
#include "LoginMode.h"
#include "GameWorld.h"
#include "MercenaryAI.h"
#include "GuildInfo.h"
#include "base/Res.h"
#include "pdb_enums.h"
#include "resource/Ground.h"
#include "pdb_shared.h"

extern CRenderer* g_renderer;
extern CSession g_session;
extern Registry2 g_registry;
extern CModeMgr g_modeMgr;
extern CMouse g_mouse;
extern const char* g_regPath;
extern char* g_uiPath;
extern char* g_shortItemPath;
extern char* g_suPath;
extern LanguageType g_languageType;
extern ServiceType g_serviceType;
extern CSnapMgr g_snapMgr;
extern UIWindowMgr g_windowMgr;
extern short g_mouse_L;
extern bool g_mouseMoved;
extern const char* MsgStr(MSGSTRINGID id);
extern void CutString(const char* msg, std::vector<std::string>& strings, int maxNumCharLine);
extern const char* whisperWindowOpenSound;




static void ClearChatHistory(std::list<StringAndColor>& history)
{
	history.erase(history.begin(), history.end());
}

char* g_uiPath = "\\UIRectInfo";
char* g_shortItemPath = "\\ShortcutItem";
char* g_suPath = "\\SkillUseLevelInfo";
int guildWndWidth = 400;
int guildWndHeight = 317;



const char* whisperWindowOpenSound = "..\\..\\BGM\\Msg_NewMsg.wav";

UIWindowMgr g_windowMgr;
std::vector<int> stateId;


short g_mouse_L = 0;
bool g_mouseMoved = 0;







void UIWindowMgr::InitUIRectInfoFromReg()
{
	DWORD DATALENGTH;
	bf_setting_key hResultKey;
	bf_setting_key hKey;
	char UIPath[512];

	strcpy(UIPath, g_regPath);
	strcat(UIPath, g_uiPath);

	bool lResult = bf_setting_open(hResultKey, UIPath);
	if (lResult)
	{
	bf_setting_close(hResultKey);
	bool lResult1 = bf_setting_open(hKey, UIPath);
	if (lResult1)
	{

	DATALENGTH = 4;

	bf_setting_get(hKey, "m_minimapZoomWnd", m_miniMapZoomFactor);
	if (!bf_setting_get(hKey, "M_ISDRAGALL", m_isDragAll))
		m_isDragAll = 1;
	if (m_isDragAll != 0)
		m_isDragAll = 1;
	if (!bf_setting_get(hKey, "M_ISDRAWCOMPASS", m_isDrawCompass))
		m_isDrawCompass = 0;
	if (!bf_setting_get(hKey, "M_MINIMAPARGB", m_miniMapArgb))
		m_miniMapArgb = 0xaaffffff;
	if (m_miniMapArgb != 0xffffffff)
		m_miniMapArgb = 0xaaffffff;
	bf_setting_get(hKey, "M_CHATWNDHEIGHT", m_chatWndHeight);
	bf_setting_get(hKey, "M_CHATWNDX", m_chatWndX);
	bf_setting_get(hKey, "M_CHATWNDY", m_chatWndY);
	bf_setting_get(hKey, "M_CHATWNDSHOW", m_chatWndShow);

	bf_setting_get(hKey, "SAYDIALOGWNDINFO.X", sayDialogWndInfo.x);
	bf_setting_get(hKey, "SAYDIALOGWNDINFO.Y", sayDialogWndInfo.y);
	bf_setting_get(hKey, "SAYDIALOGWNDINFO.W", sayDialogWndInfo.w);
	bf_setting_get(hKey, "SAYDIALOGWNDINFO.H", sayDialogWndInfo.h);

	bf_setting_get(hKey, "CHOOSEWNDINFO.X", chooseWndInfo.x);
	bf_setting_get(hKey, "CHOOSEWNDINFO.Y", chooseWndInfo.y);
	bf_setting_get(hKey, "CHOOSEWNDINFO.W", chooseWndInfo.w);
	bf_setting_get(hKey, "CHOOSEWNDINFO.H", chooseWndInfo.h);

	bf_setting_get(hKey, "BASICINFOWNDINFO.X", basicInfoWndInfo.x);
	bf_setting_get(hKey, "BASICINFOWNDINFO.Y", basicInfoWndInfo.y);
	bf_setting_get(hKey, "BASICINFOWNDINFO.W", basicInfoWndInfo.w);
	bf_setting_get(hKey, "BASICINFOWNDINFO.H", basicInfoWndInfo.h);
	basicInfoWndInfo.x = 740;
	basicInfoWndInfo.y = 645;
	basicInfoWndInfo.h = 120;

	bf_setting_get(hKey, "ITEMWNDINFO.X", itemWndInfo.x);
	bf_setting_get(hKey, "ITEMWNDINFO.Y", itemWndInfo.y);
	bf_setting_get(hKey, "ITEMWNDINFO.W", itemWndInfo.w);
	bf_setting_get(hKey, "ITEMWNDINFO.H", itemWndInfo.h);
	bf_setting_get(hKey, "ITEMWNDINFO.ORGHEIGHT", itemWndInfo.orgHeight);
	bf_setting_get(hKey, "ITEMWNDINFO.CURTAB", itemWndInfo.curTab);
	bf_setting_get(hKey, "ITEMWNDINFO.CURRADIO1", itemWndInfo.curRadio1);
	bf_setting_get(hKey, "ITEMWNDINFO.CURRADIO2", itemWndInfo.curRadio2);
	bf_setting_get(hKey, "ITEMWNDINFO.CURRADIO3", itemWndInfo.curRadio3);
	itemWndInfo.x = 744;
	itemWndInfo.y = 514;
	itemWndInfo.w = 280;
	itemWndInfo.h = 134;
	itemWndInfo.orgHeight = 134;
	itemWndInfo.curTab = 1;
	itemWndInfo.show = 0;

	bf_setting_get(hKey, "QUESTWNDINFO.X", questWndInfo.x);
	bf_setting_get(hKey, "QUESTWNDINFO.Y", questWndInfo.y);
	bf_setting_get(hKey, "QUESTWNDINFO.W", questWndInfo.w);
	bf_setting_get(hKey, "QUESTWNDINFO.H", questWndInfo.h);
	bf_setting_get(hKey, "QUESTWNDINFO.ORGHEIGHT", questWndInfo.orgHeight);
	bf_setting_get(hKey, "QUESTWNDINFO.CURTAB", questWndInfo.curTab);
	bf_setting_get(hKey, "QUESTWNDINFO.SHOW", questWndInfo.show);

	bf_setting_get(hKey, "STATUSWNDINFO.X", statusWndInfo.x);
	bf_setting_get(hKey, "STATUSWNDINFO.Y", statusWndInfo.y);
	bf_setting_get(hKey, "STATUSWNDINFO.W", statusWndInfo.w);
	bf_setting_get(hKey, "STATUSWNDINFO.H", statusWndInfo.h);
	bf_setting_get(hKey, "STATUSWNDINFO.ORGHEIGHT", statusWndInfo.orgHeight);
	statusWndInfo.show = 0;

	bf_setting_get(hKey, "EQUIPWNDINFO.X", equipWndInfo.x);
	bf_setting_get(hKey, "EQUIPWNDINFO.Y", equipWndInfo.y);
	bf_setting_get(hKey, "EQUIPWNDINFO.W", equipWndInfo.w);
	bf_setting_get(hKey, "EQUIPWNDINFO.H", equipWndInfo.h);
	bf_setting_get(hKey, "EQUIPWNDINFO.ORGHEIGHT", equipWndInfo.orgHeight);
	equipWndInfo.show = 0;

	bf_setting_get(hKey, "OPTIONWNDINFO.X", optionWndInfo.x);
	bf_setting_get(hKey, "OPTIONWNDINFO.Y", optionWndInfo.y);
	bf_setting_get(hKey, "OPTIONWNDINFO.W", optionWndInfo.w);
	bf_setting_get(hKey, "OPTIONWNDINFO.H", optionWndInfo.h);
	bf_setting_get(hKey, "OPTIONWNDINFO.ORGHEIGHT", optionWndInfo.orgHeight);
	optionWndInfo.show = 0;

	bf_setting_get(hKey, "SHORTENITEMWNDINFO.X", shortenItemWndInfo.x);
	bf_setting_get(hKey, "SHORTENITEMWNDINFO.Y", shortenItemWndInfo.y);
	bf_setting_get(hKey, "SHORTENITEMWNDINFO.W", shortenItemWndInfo.w);
	bf_setting_get(hKey, "SHORTENITEMWNDINFO.H", shortenItemWndInfo.h);
	shortenItemWndInfo.show = 0;

	bf_setting_get(hKey, "SHORTCUTWNDINFO.X", shortCutWndInfo.x);
	bf_setting_get(hKey, "SHORTCUTWNDINFO.Y", shortCutWndInfo.y);
	bf_setting_get(hKey, "SHORTCUTWNDINFO.W", shortCutWndInfo.w);
	bf_setting_get(hKey, "SHORTCUTWNDINFO.H", shortCutWndInfo.h);
	shortCutWndInfo.show = 0;

	bf_setting_get(hKey, "CHATROOMWNDINFO.X", chatRoomWndInfo.x);
	bf_setting_get(hKey, "CHATROOMWNDINFO.Y", chatRoomWndInfo.y);
	bf_setting_get(hKey, "CHATROOMWNDINFO.W", chatRoomWndInfo.w);
	bf_setting_get(hKey, "CHATROOMWNDINFO.H", chatRoomWndInfo.h);

	bf_setting_get(hKey, "ITEMSTOREWNDINFO.X", itemStoreWndInfo.x);
	bf_setting_get(hKey, "ITEMSTOREWNDINFO.Y", itemStoreWndInfo.y);
	bf_setting_get(hKey, "ITEMSTOREWNDINFO.W", itemStoreWndInfo.w);
	bf_setting_get(hKey, "ITEMSTOREWNDINFO.H", itemStoreWndInfo.h);

	bf_setting_get(hKey, "ITEMSHOPWNDINFO.X", itemShopWndInfo.x);
	bf_setting_get(hKey, "ITEMSHOPWNDINFO.Y", itemShopWndInfo.y);
	bf_setting_get(hKey, "ITEMSHOPWNDINFO.W", itemShopWndInfo.w);
	bf_setting_get(hKey, "ITEMSHOPWNDINFO.H", itemShopWndInfo.h);

	bf_setting_get(hKey, "ITEMSELLWNDINFO.X", itemSellWndInfo.x);
	bf_setting_get(hKey, "ITEMSELLWNDINFO.Y", itemSellWndInfo.y);
	bf_setting_get(hKey, "ITEMSELLWNDINFO.W", itemSellWndInfo.w);
	bf_setting_get(hKey, "ITEMSELLWNDINFO.H", itemSellWndInfo.h);

	bf_setting_get(hKey, "ITEMPURCHASEWNDINFO.X", itemPurchaseWndInfo.x);
	bf_setting_get(hKey, "ITEMPURCHASEWNDINFO.Y", itemPurchaseWndInfo.y);
	bf_setting_get(hKey, "ITEMPURCHASEWNDINFO.W", itemPurchaseWndInfo.w);
	bf_setting_get(hKey, "ITEMPURCHASEWNDINFO.H", itemPurchaseWndInfo.h);

	bf_setting_get(hKey, "MESSENGERGROUPWNDINFO.X", messengerGroupWndInfo.x);
	bf_setting_get(hKey, "MESSENGERGROUPWNDINFO.Y", messengerGroupWndInfo.y);
	bf_setting_get(hKey, "MESSENGERGROUPWNDINFO.W", messengerGroupWndInfo.w);
	bf_setting_get(hKey, "MESSENGERGROUPWNDINFO.H", messengerGroupWndInfo.h);
	if (!bf_setting_get(hKey, "MESSENGERGROUPWNDINFO.SHOW", messengerGroupWndInfo.show))
		messengerGroupWndInfo.show = 0;
	if (!bf_setting_get(hKey, "MESSENGERGROUPWNDINFO.RADIO", messengerGroupWndInfo.radio))
		messengerGroupWndInfo.radio = 1;

	bf_setting_get(hKey, "EXCHANGEACCEPTWNDINFO.X", exchangeAcceptWndInfo.x);
	bf_setting_get(hKey, "EXCHANGEACCEPTWNDINFO.Y", exchangeAcceptWndInfo.y);
	bf_setting_get(hKey, "EXCHANGEACCEPTWNDINFO.W", exchangeAcceptWndInfo.w);
	bf_setting_get(hKey, "EXCHANGEACCEPTWNDINFO.H", exchangeAcceptWndInfo.h);

	bf_setting_get(hKey, "SKILLLISTWNDINFO.X", skillListWndInfo.x);
	bf_setting_get(hKey, "SKILLLISTWNDINFO.Y", skillListWndInfo.y);
	bf_setting_get(hKey, "SKILLLISTWNDINFO.W", skillListWndInfo.w);
	bf_setting_get(hKey, "SKILLLISTWNDINFO.H", skillListWndInfo.h);
	skillListWndInfo.show = 0;

	bf_setting_get(hKey, "MERCHANTITEMWNDINFO.X", merchantItemWndInfo.x);
	bf_setting_get(hKey, "MERCHANTITEMWNDINFO.Y", merchantItemWndInfo.y);
	bf_setting_get(hKey, "MERCHANTITEMWNDINFO.W", merchantItemWndInfo.w);
	bf_setting_get(hKey, "MERCHANTITEMWNDINFO.H", merchantItemWndInfo.h);
	merchantItemWndInfo.show = 0;

	bf_setting_get(hKey, "MERCHANTMIRRORITEMWNDINFO.X", merchantMirrorItemWndInfo.x);
	bf_setting_get(hKey, "MERCHANTMIRRORITEMWNDINFO.Y", merchantMirrorItemWndInfo.y);
	bf_setting_get(hKey, "MERCHANTMIRRORITEMWNDINFO.W", merchantMirrorItemWndInfo.w);
	bf_setting_get(hKey, "MERCHANTMIRRORITEMWNDINFO.H", merchantMirrorItemWndInfo.h);

	bf_setting_get(hKey, "MERCHANTSHOPMAKEWNDINFO.X", merchantShopMakeWndInfo.x);
	bf_setting_get(hKey, "MERCHANTSHOPMAKEWNDINFO.Y", merchantShopMakeWndInfo.y);
	bf_setting_get(hKey, "MERCHANTSHOPMAKEWNDINFO.W", merchantShopMakeWndInfo.w);
	bf_setting_get(hKey, "MERCHANTSHOPMAKEWNDINFO.H", merchantShopMakeWndInfo.h);

	bf_setting_get(hKey, "MERCHANTITEMSHOPWNDINFO.X", merchantItemShopWndInfo.x);
	bf_setting_get(hKey, "MERCHANTITEMSHOPWNDINFO.Y", merchantItemShopWndInfo.y);
	bf_setting_get(hKey, "MERCHANTITEMSHOPWNDINFO.W", merchantItemShopWndInfo.w);
	bf_setting_get(hKey, "MERCHANTITEMSHOPWNDINFO.H", merchantItemShopWndInfo.h);

	bf_setting_get(hKey, "MERCHANTITEMMYSHOPWNDINFO.X", merchantItemMyShopWndInfo.x);
	bf_setting_get(hKey, "MERCHANTITEMMYSHOPWNDINFO.Y", merchantItemMyShopWndInfo.y);
	bf_setting_get(hKey, "MERCHANTITEMMYSHOPWNDINFO.W", merchantItemMyShopWndInfo.w);
	bf_setting_get(hKey, "MERCHANTITEMMYSHOPWNDINFO.H", merchantItemMyShopWndInfo.h);

	bf_setting_get(hKey, "MERCHANTITEMPURCHASEWNDINFO.X", merchantItemPurchaseWndInfo.x);
	bf_setting_get(hKey, "MERCHANTITEMPURCHASEWNDINFO.Y", merchantItemPurchaseWndInfo.y);
	bf_setting_get(hKey, "MERCHANTITEMPURCHASEWNDINFO.W", merchantItemPurchaseWndInfo.w);
	bf_setting_get(hKey, "MERCHANTITEMPURCHASEWNDINFO.H", merchantItemPurchaseWndInfo.h);

	bf_setting_get(hKey, "ITEMCOLLECTIONWNDINFO.X", itemCollectionWndInfo.x);
	bf_setting_get(hKey, "ITEMCOLLECTIONWNDINFO.Y", itemCollectionWndInfo.y);
	bf_setting_get(hKey, "ITEMCOLLECTIONWNDINFO.W", itemCollectionWndInfo.w);
	bf_setting_get(hKey, "ITEMCOLLECTIONWNDINFO.H", itemCollectionWndInfo.h);

	bf_setting_get(hKey, "COMBINEDCARDITEMCOLLECTIONWNDINFO.X", combinedCardItemCollectionWndInfo.x);
	bf_setting_get(hKey, "COMBINEDCARDITEMCOLLECTIONWNDINFO.Y", combinedCardItemCollectionWndInfo.y);
	bf_setting_get(hKey, "COMBINEDCARDITEMCOLLECTIONWNDINFO.W", combinedCardItemCollectionWndInfo.w);
	bf_setting_get(hKey, "COMBINEDCARDITEMCOLLECTIONWNDINFO.H", combinedCardItemCollectionWndInfo.h);

	bf_setting_get(hKey, "ITEMPARAMCHANGEDISPLAYWNDINFO.X", itemParamChangeDisplayWndInfo.x);
	bf_setting_get(hKey, "ITEMPARAMCHANGEDISPLAYWNDINFO.Y", itemParamChangeDisplayWndInfo.y);
	bf_setting_get(hKey, "ITEMPARAMCHANGEDISPLAYWNDINFO.W", itemParamChangeDisplayWndInfo.w);
	bf_setting_get(hKey, "ITEMPARAMCHANGEDISPLAYWNDINFO.H", itemParamChangeDisplayWndInfo.h);

	bf_setting_get(hKey, "PARTYSETTINGWNDINFO.X", partySettingWndInfo.x);
	bf_setting_get(hKey, "PARTYSETTINGWNDINFO.Y", partySettingWndInfo.y);
	bf_setting_get(hKey, "PARTYSETTINGWNDINFO.W", partySettingWndInfo.w);
	bf_setting_get(hKey, "PARTYSETTINGWNDINFO.H", partySettingWndInfo.h);

	bf_setting_get(hKey, "DETAILLEVELWNDINFO.X", detailLevelWndInfo.x);
	bf_setting_get(hKey, "DETAILLEVELWNDINFO.Y", detailLevelWndInfo.y);
	bf_setting_get(hKey, "DETAILLEVELWNDINFO.W", detailLevelWndInfo.w);
	bf_setting_get(hKey, "DETAILLEVELWNDINFO.H", detailLevelWndInfo.h);

	bf_setting_get(hKey, "WHISPERWNDINFO.X", whisperWndInfo.x);
	bf_setting_get(hKey, "WHISPERWNDINFO.Y", whisperWndInfo.y);
	bf_setting_get(hKey, "WHISPERWNDINFO.W", whisperWndInfo.w);
	bf_setting_get(hKey, "WHISPERWNDINFO.H", whisperWndInfo.h);

	bf_setting_get(hKey, "FRIENDOPTIONWNDINFO.X", friendOptionWndInfo.x);
	bf_setting_get(hKey, "FRIENDOPTIONWNDINFO.Y", friendOptionWndInfo.y);
	bf_setting_get(hKey, "FRIENDOPTIONWNDINFO.W", friendOptionWndInfo.w);
	bf_setting_get(hKey, "FRIENDOPTIONWNDINFO.H", friendOptionWndInfo.h);

	bf_setting_get(hKey, "ITEMCOMPOSITIONWNDINFO.X", itemCompositionWndInfo.x);
	bf_setting_get(hKey, "ITEMCOMPOSITIONWNDINFO.Y", itemCompositionWndInfo.y);
	bf_setting_get(hKey, "ITEMCOMPOSITIONWNDINFO.W", itemCompositionWndInfo.w);
	bf_setting_get(hKey, "ITEMCOMPOSITIONWNDINFO.H", itemCompositionWndInfo.h);

	bf_setting_get(hKey, "ITEMIDENTIFYWNDINFO.X", itemIdentifyWndInfo.x);
	bf_setting_get(hKey, "ITEMIDENTIFYWNDINFO.Y", itemIdentifyWndInfo.y);
	bf_setting_get(hKey, "ITEMIDENTIFYWNDINFO.W", itemIdentifyWndInfo.w);
	bf_setting_get(hKey, "ITEMIDENTIFYWNDINFO.H", itemIdentifyWndInfo.h);

	bf_setting_get(hKey, "SUBCHATWNDST.X", subChatWndStInfo.x);
	bf_setting_get(hKey, "SUBCHATWNDST.Y", subChatWndStInfo.y);
	bf_setting_get(hKey, "SUBCHATWNDST.W", subChatWndStInfo.w);
	bf_setting_get(hKey, "SUBCHATWNDST.H", subChatWndStInfo.h);

	bf_setting_get(hKey, "SUBCHATWNDBT.X", subChatWndBtInfo.x);
	bf_setting_get(hKey, "SUBCHATWNDBT.Y", subChatWndBtInfo.y);
	bf_setting_get(hKey, "SUBCHATWNDBT.W", subChatWndBtInfo.w);
	bf_setting_get(hKey, "SUBCHATWNDBT.H", subChatWndBtInfo.h);

	bf_setting_read(hKey, "CHATWNDSTICKON", &m_chatWndStickOn, &DATALENGTH);
	bf_setting_read(hKey, "ONSTSUBCHAT", &m_onStSubChat, &DATALENGTH);
	bf_setting_read(hKey, "ONBTSUBCHAT", &m_onBtSubChat, &DATALENGTH);
	bf_setting_read(hKey, "MEMORIALDUNWND.X", &g_session.dunInfoWndX, &DATALENGTH);
	bf_setting_read(hKey, "MEMORIALDUNWND.Y", &g_session.dunInfoWndY, &DATALENGTH);


	bf_setting_close(hKey);
	}

	return;
	}

	MakeDefaultUIRectInfo();
}

void UIWindowMgr::InitShortcutItemInfoFromReg()
{
	int shortenSkillUseLevel;
	bf_setting_key hKey;
	DWORD DATALENGTH;
	bf_setting_key hResultKey;
	char itemName[33];
	char sItemRegName[33];
	char UIPath[512];

	strcpy(UIPath, g_regPath);
	strcat(UIPath, g_shortItemPath);
	strcat(UIPath, "\\");
	if (strstr(g_session.m_lastServerName.c_str(), "\xbb\xe7\xc5\xa9\xb6\xf3\xc0\xcc"))
		strcat(UIPath, "\xbb\xe7\xc5\xa9\xb6\xf3\xc0\xcc");
	else
		strcat(UIPath, g_session.m_lastServerName.c_str());

	bool lResult = bf_setting_open(hResultKey, UIPath);
	if (lResult)
	{
		bf_setting_close(hResultKey);
		bool lResult1 = bf_setting_open(hKey, UIPath);
		if (lResult1)
		{
			g_session.ClearShortenItemName();
			g_session.ClearShortenSkillUseLevel();

			for (int i = 0; i < 27; ++i)
			{
				DATALENGTH = 0x20;
				sprintf(sItemRegName, "SITEM%d%d%d", i, g_serviceType, g_session.m_charSlot);
				memset(itemName, 0, sizeof(itemName));
				bool res = bf_setting_read(hKey, sItemRegName, itemName, &DATALENGTH);
				int flag = 0;
				g_session.m_shortcutSlotCnt = i / 9;
				if (res) {
					if (itemName[0] != '#')
						g_session.SetShortenItemName(itemName, i);
					else
						flag = 1;
				}
				else
					g_session.SetShortenItemName("nothing", i);

				DATALENGTH = 4;
				sprintf(sItemRegName, "S_SKILL_USE_LEVEL%d%d%d", i, g_serviceType, g_session.m_charSlot);
				shortenSkillUseLevel = 0;
				res = bf_setting_get(hKey, sItemRegName, shortenSkillUseLevel);

				if (res)
					g_session.SetShortenSkillUseLevel(shortenSkillUseLevel, i);
				else
				{
					g_session.SetShortenSkillUseLevel(0, i);
					shortenSkillUseLevel = 0;
				}

				if (flag)
					g_session.SetShortenSkillName(itemName, shortenSkillUseLevel, i);
			}

			bf_setting_close(hKey);
		}
	}
	g_session.m_shortcutSlotCnt = 0;
}

void UIWindowMgr::WriteShorcutItemInfoToReg()
{
	bf_setting_key hKey;
	int shortenSkillUseLevel;
	char sItemRegName[33];
	char itemName[33];
	char UIPath[512];
	int i;

	strcpy(UIPath, g_regPath);
	strcat(UIPath, g_shortItemPath);
	strcat(UIPath, "\\");
	if (strstr(g_session.m_lastServerName.c_str(), "\xbb\xe7\xc5\xa9\xb6\xf3\xc0\xcc"))
		strcat(UIPath, "\xbb\xe7\xc5\xa9\xb6\xf3\xc0\xcc");
	else
		strcat(UIPath, g_session.m_lastServerName.c_str());

	bool lResult = bf_setting_create(hKey, UIPath);
	if (!lResult)
		return;

	for (i = 0; i < 27; ++i)
	{
		memset(itemName, 0, sizeof(itemName));
		strcpy(itemName, g_session.GetShortenItemName(i));
		sprintf(sItemRegName, "SITEM%d%d%d", i, g_serviceType, g_session.m_charSlot);
		bf_setting_set_string(hKey, sItemRegName, itemName, 0x20);

		shortenSkillUseLevel = g_session.GetShortenSkillUseLevel(i);
		sprintf(sItemRegName, "S_SKILL_USE_LEVEL%d%d%d", i, g_serviceType, g_session.m_charSlot);
		bf_setting_set(hKey, sItemRegName, shortenSkillUseLevel);
	}

	bf_setting_close(hKey);
}

void UIWindowMgr::WriteUIRectInfoToReg()
{
	bf_setting_key hKey;
	char UIPath[512];

	strcpy(UIPath, g_regPath);
	strcat(UIPath, g_uiPath);

	bool lResult = bf_setting_create(hKey, UIPath);
	if (!lResult)
		return;

	bf_setting_set(hKey, "m_minimapZoomWnd", m_miniMapZoomFactor);
	bf_setting_set(hKey, "M_ISDRAGALL", m_isDragAll);
	bf_setting_set(hKey, "M_ISDRAWCOMPASS", m_isDrawCompass);
	bf_setting_set(hKey, "M_MINIMAPARGB", m_miniMapArgb);
	bf_setting_set(hKey, "M_CHATWNDSTATUS", m_chatWndStatus);
	bf_setting_set(hKey, "M_CHATWNDHEIGHT", m_chatWndHeight);
	bf_setting_set(hKey, "M_CHATWNDX", m_chatWndX);
	bf_setting_set(hKey, "M_CHATWNDY", m_chatWndY);
	bf_setting_set(hKey, "M_CHATWNDSHOW", m_chatWndShow);

	bf_setting_set(hKey, "SAYDIALOGWNDINFO.X", sayDialogWndInfo.x);
	bf_setting_set(hKey, "SAYDIALOGWNDINFO.Y", sayDialogWndInfo.y);
	bf_setting_set(hKey, "SAYDIALOGWNDINFO.W", sayDialogWndInfo.w);
	bf_setting_set(hKey, "SAYDIALOGWNDINFO.H", sayDialogWndInfo.h);

	bf_setting_set(hKey, "CHOOSEWNDINFO.X", chooseWndInfo.x);
	bf_setting_set(hKey, "CHOOSEWNDINFO.Y", chooseWndInfo.y);
	bf_setting_set(hKey, "CHOOSEWNDINFO.W", chooseWndInfo.w);
	bf_setting_set(hKey, "CHOOSEWNDINFO.H", chooseWndInfo.h);

	bf_setting_set(hKey, "BASICINFOWNDINFO.X", basicInfoWndInfo.x);
	bf_setting_set(hKey, "BASICINFOWNDINFO.Y", basicInfoWndInfo.y);
	bf_setting_set(hKey, "BASICINFOWNDINFO.W", basicInfoWndInfo.w);
	bf_setting_set(hKey, "BASICINFOWNDINFO.H", basicInfoWndInfo.h);

	bf_setting_set(hKey, "ITEMWNDINFO.X", itemWndInfo.x);
	bf_setting_set(hKey, "ITEMWNDINFO.Y", itemWndInfo.y);
	bf_setting_set(hKey, "ITEMWNDINFO.W", itemWndInfo.w);
	bf_setting_set(hKey, "ITEMWNDINFO.H", itemWndInfo.h);
	bf_setting_set(hKey, "ITEMWNDINFO.ORGHEIGHT", itemWndInfo.orgHeight);
	bf_setting_set(hKey, "ITEMWNDINFO.CURTAB", itemWndInfo.curTab);
	bf_setting_set(hKey, "ITEMWNDINFO.CURRADIO1", itemWndInfo.curRadio1);
	bf_setting_set(hKey, "ITEMWNDINFO.CURRADIO2", itemWndInfo.curRadio2);
	bf_setting_set(hKey, "ITEMWNDINFO.CURRADIO3", itemWndInfo.curRadio3);
	bf_setting_set(hKey, "ITEMWNDINFO.SHOW", itemWndInfo.show);

	bf_setting_set(hKey, "QUESTWNDINFO.X", questWndInfo.x);
	bf_setting_set(hKey, "QUESTWNDINFO.Y", questWndInfo.y);
	bf_setting_set(hKey, "QUESTWNDINFO.W", questWndInfo.w);
	bf_setting_set(hKey, "QUESTWNDINFO.H", questWndInfo.h);
	bf_setting_set(hKey, "QUESTWNDINFO.ORGHEIGHT", questWndInfo.orgHeight);
	bf_setting_set(hKey, "QUESTWNDINFO.CURTAB", questWndInfo.curTab);
	bf_setting_set(hKey, "QUESTWNDINFO.SHOW", questWndInfo.show);

	bf_setting_set(hKey, "STATUSWNDINFO.X", statusWndInfo.x);
	bf_setting_set(hKey, "STATUSWNDINFO.Y", statusWndInfo.y);
	bf_setting_set(hKey, "STATUSWNDINFO.W", statusWndInfo.w);
	bf_setting_set(hKey, "STATUSWNDINFO.H", statusWndInfo.h);
	bf_setting_set(hKey, "STATUSWNDINFO.ORGHEIGHT", statusWndInfo.orgHeight);
	bf_setting_set(hKey, "STATUSWNDINFO.SHOW", statusWndInfo.show);

	bf_setting_set(hKey, "EQUIPWNDINFO.X", equipWndInfo.x);
	bf_setting_set(hKey, "EQUIPWNDINFO.Y", equipWndInfo.y);
	bf_setting_set(hKey, "EQUIPWNDINFO.W", equipWndInfo.w);
	bf_setting_set(hKey, "EQUIPWNDINFO.H", equipWndInfo.h);
	bf_setting_set(hKey, "EQUIPWNDINFO.ORGHEIGHT", equipWndInfo.orgHeight);
	bf_setting_set(hKey, "EQUIPWNDINFO.SHOW", equipWndInfo.show);

	bf_setting_set(hKey, "OPTIONWNDINFO.X", optionWndInfo.x);
	bf_setting_set(hKey, "OPTIONWNDINFO.Y", optionWndInfo.y);
	bf_setting_set(hKey, "OPTIONWNDINFO.W", optionWndInfo.w);
	bf_setting_set(hKey, "OPTIONWNDINFO.H", optionWndInfo.h);
	bf_setting_set(hKey, "OPTIONWNDINFO.ORGHEIGHT", optionWndInfo.orgHeight);
	bf_setting_set(hKey, "OPTIONWNDINFO.SHOW", optionWndInfo.show);

	bf_setting_set(hKey, "SHORTENITEMWNDINFO.X", shortenItemWndInfo.x);
	bf_setting_set(hKey, "SHORTENITEMWNDINFO.Y", shortenItemWndInfo.y);
	bf_setting_set(hKey, "SHORTENITEMWNDINFO.W", shortenItemWndInfo.w);
	bf_setting_set(hKey, "SHORTENITEMWNDINFO.H", shortenItemWndInfo.h);
	bf_setting_set(hKey, "SHORTENITEMWNDINFO.SHOW", shortenItemWndInfo.show);

	bf_setting_set(hKey, "SHORTCUTWNDINFO.X", shortCutWndInfo.x);
	bf_setting_set(hKey, "SHORTCUTWNDINFO.Y", shortCutWndInfo.y);
	bf_setting_set(hKey, "SHORTCUTWNDINFO.W", shortCutWndInfo.w);
	bf_setting_set(hKey, "SHORTCUTWNDINFO.H", shortCutWndInfo.h);
	bf_setting_set(hKey, "SHORTCUTWNDINFO.SHOW", shortCutWndInfo.show);

	bf_setting_set(hKey, "CHATROOMWNDINFO.X", chatRoomWndInfo.x);
	bf_setting_set(hKey, "CHATROOMWNDINFO.Y", chatRoomWndInfo.y);
	bf_setting_set(hKey, "CHATROOMWNDINFO.W", chatRoomWndInfo.w);
	bf_setting_set(hKey, "CHATROOMWNDINFO.H", chatRoomWndInfo.h);

	bf_setting_set(hKey, "ITEMSTOREWNDINFO.X", itemStoreWndInfo.x);
	bf_setting_set(hKey, "ITEMSTOREWNDINFO.Y", itemStoreWndInfo.y);
	bf_setting_set(hKey, "ITEMSTOREWNDINFO.W", itemStoreWndInfo.w);
	bf_setting_set(hKey, "ITEMSTOREWNDINFO.H", itemStoreWndInfo.h);

	bf_setting_set(hKey, "ITEMSHOPWNDINFO.X", itemShopWndInfo.x);
	bf_setting_set(hKey, "ITEMSHOPWNDINFO.Y", itemShopWndInfo.y);
	bf_setting_set(hKey, "ITEMSHOPWNDINFO.W", itemShopWndInfo.w);
	bf_setting_set(hKey, "ITEMSHOPWNDINFO.H", itemShopWndInfo.h);

	bf_setting_set(hKey, "ITEMSELLWNDINFO.X", itemSellWndInfo.x);
	bf_setting_set(hKey, "ITEMSELLWNDINFO.Y", itemSellWndInfo.y);
	bf_setting_set(hKey, "ITEMSELLWNDINFO.W", itemSellWndInfo.w);
	bf_setting_set(hKey, "ITEMSELLWNDINFO.H", itemSellWndInfo.h);

	bf_setting_set(hKey, "ITEMPURCHASEWNDINFO.X", itemPurchaseWndInfo.x);
	bf_setting_set(hKey, "ITEMPURCHASEWNDINFO.Y", itemPurchaseWndInfo.y);
	bf_setting_set(hKey, "ITEMPURCHASEWNDINFO.W", itemPurchaseWndInfo.w);
	bf_setting_set(hKey, "ITEMPURCHASEWNDINFO.H", itemPurchaseWndInfo.h);

	bf_setting_set(hKey, "MESSENGERGROUPWNDINFO.X", messengerGroupWndInfo.x);
	bf_setting_set(hKey, "MESSENGERGROUPWNDINFO.Y", messengerGroupWndInfo.y);
	bf_setting_set(hKey, "MESSENGERGROUPWNDINFO.W", messengerGroupWndInfo.w);
	bf_setting_set(hKey, "MESSENGERGROUPWNDINFO.H", messengerGroupWndInfo.h);
	bf_setting_set(hKey, "MESSENGERGROUPWNDINFO.SHOW", messengerGroupWndInfo.show);
	bf_setting_set(hKey, "MESSENGERGROUPWNDINFO.RADIO", messengerGroupWndInfo.radio);

	bf_setting_set(hKey, "EXCHANGEACCEPTWNDINFO.X", exchangeAcceptWndInfo.x);
	bf_setting_set(hKey, "EXCHANGEACCEPTWNDINFO.Y", exchangeAcceptWndInfo.y);
	bf_setting_set(hKey, "EXCHANGEACCEPTWNDINFO.W", exchangeAcceptWndInfo.w);
	bf_setting_set(hKey, "EXCHANGEACCEPTWNDINFO.H", exchangeAcceptWndInfo.h);

	bf_setting_set(hKey, "SKILLLISTWNDINFO.X", skillListWndInfo.x);
	bf_setting_set(hKey, "SKILLLISTWNDINFO.Y", skillListWndInfo.y);
	bf_setting_set(hKey, "SKILLLISTWNDINFO.W", skillListWndInfo.w);
	bf_setting_set(hKey, "SKILLLISTWNDINFO.H", skillListWndInfo.h);
	bf_setting_set(hKey, "SKILLLISTWNDINFO.SHOW", skillListWndInfo.show);

	bf_setting_set(hKey, "MERCHANTITEMWNDINFO.X", merchantItemWndInfo.x);
	bf_setting_set(hKey, "MERCHANTITEMWNDINFO.Y", merchantItemWndInfo.y);
	bf_setting_set(hKey, "MERCHANTITEMWNDINFO.W", merchantItemWndInfo.w);
	bf_setting_set(hKey, "MERCHANTITEMWNDINFO.H", merchantItemWndInfo.h);
	bf_setting_set(hKey, "MERCHANTITEMWNDINFO.SHOW", merchantItemWndInfo.show);

	bf_setting_set(hKey, "MERCHANTMIRRORITEMWNDINFO.X", merchantMirrorItemWndInfo.x);
	bf_setting_set(hKey, "MERCHANTMIRRORITEMWNDINFO.Y", merchantMirrorItemWndInfo.y);
	bf_setting_set(hKey, "MERCHANTMIRRORITEMWNDINFO.W", merchantMirrorItemWndInfo.w);
	bf_setting_set(hKey, "MERCHANTMIRRORITEMWNDINFO.H", merchantMirrorItemWndInfo.h);

	bf_setting_set(hKey, "MERCHANTSHOPMAKEWNDINFO.X", merchantShopMakeWndInfo.x);
	bf_setting_set(hKey, "MERCHANTSHOPMAKEWNDINFO.Y", merchantShopMakeWndInfo.y);
	bf_setting_set(hKey, "MERCHANTSHOPMAKEWNDINFO.W", merchantShopMakeWndInfo.w);
	bf_setting_set(hKey, "MERCHANTSHOPMAKEWNDINFO.H", merchantShopMakeWndInfo.h);

	bf_setting_set(hKey, "MERCHANTITEMSHOPWNDINFO.X", merchantItemShopWndInfo.x);
	bf_setting_set(hKey, "MERCHANTITEMSHOPWNDINFO.Y", merchantItemShopWndInfo.y);
	bf_setting_set(hKey, "MERCHANTITEMSHOPWNDINFO.W", merchantItemShopWndInfo.w);
	bf_setting_set(hKey, "MERCHANTITEMSHOPWNDINFO.H", merchantItemShopWndInfo.h);

	bf_setting_set(hKey, "MERCHANTITEMMYSHOPWNDINFO.X", merchantItemMyShopWndInfo.x);
	bf_setting_set(hKey, "MERCHANTITEMMYSHOPWNDINFO.Y", merchantItemMyShopWndInfo.y);
	bf_setting_set(hKey, "MERCHANTITEMMYSHOPWNDINFO.W", merchantItemMyShopWndInfo.w);
	bf_setting_set(hKey, "MERCHANTITEMMYSHOPWNDINFO.H", merchantItemMyShopWndInfo.h);

	bf_setting_set(hKey, "MERCHANTITEMPURCHASEWNDINFO.X", merchantItemPurchaseWndInfo.x);
	bf_setting_set(hKey, "MERCHANTITEMPURCHASEWNDINFO.Y", merchantItemPurchaseWndInfo.y);
	bf_setting_set(hKey, "MERCHANTITEMPURCHASEWNDINFO.W", merchantItemPurchaseWndInfo.w);
	bf_setting_set(hKey, "MERCHANTITEMPURCHASEWNDINFO.H", merchantItemPurchaseWndInfo.h);

	bf_setting_set(hKey, "ITEMCOLLECTIONWNDINFO.X", itemCollectionWndInfo.x);
	bf_setting_set(hKey, "ITEMCOLLECTIONWNDINFO.Y", itemCollectionWndInfo.y);
	bf_setting_set(hKey, "ITEMCOLLECTIONWNDINFO.W", itemCollectionWndInfo.w);
	bf_setting_set(hKey, "ITEMCOLLECTIONWNDINFO.H", itemCollectionWndInfo.h);

	bf_setting_set(hKey, "COMBINEDCARDITEMCOLLECTIONWNDINFO.X", combinedCardItemCollectionWndInfo.x);
	bf_setting_set(hKey, "COMBINEDCARDITEMCOLLECTIONWNDINFO.Y", combinedCardItemCollectionWndInfo.y);
	bf_setting_set(hKey, "COMBINEDCARDITEMCOLLECTIONWNDINFO.W", combinedCardItemCollectionWndInfo.w);
	bf_setting_set(hKey, "COMBINEDCARDITEMCOLLECTIONWNDINFO.H", combinedCardItemCollectionWndInfo.h);

	bf_setting_set(hKey, "ITEMPARAMCHANGEDISPLAYWNDINFO.X", itemParamChangeDisplayWndInfo.x);
	bf_setting_set(hKey, "ITEMPARAMCHANGEDISPLAYWNDINFO.Y", itemParamChangeDisplayWndInfo.y);
	bf_setting_set(hKey, "ITEMPARAMCHANGEDISPLAYWNDINFO.W", itemParamChangeDisplayWndInfo.w);
	bf_setting_set(hKey, "ITEMPARAMCHANGEDISPLAYWNDINFO.H", itemParamChangeDisplayWndInfo.h);

	bf_setting_set(hKey, "PARTYSETTINGWNDINFO.X", partySettingWndInfo.x);
	bf_setting_set(hKey, "PARTYSETTINGWNDINFO.Y", partySettingWndInfo.y);
	bf_setting_set(hKey, "PARTYSETTINGWNDINFO.W", partySettingWndInfo.w);
	bf_setting_set(hKey, "PARTYSETTINGWNDINFO.H", partySettingWndInfo.h);

	bf_setting_set(hKey, "DETAILLEVELWNDINFO.X", detailLevelWndInfo.x);
	bf_setting_set(hKey, "DETAILLEVELWNDINFO.Y", detailLevelWndInfo.y);
	bf_setting_set(hKey, "DETAILLEVELWNDINFO.W", detailLevelWndInfo.w);
	bf_setting_set(hKey, "DETAILLEVELWNDINFO.H", detailLevelWndInfo.h);

	bf_setting_set(hKey, "WHISPERWNDINFO.X", whisperWndInfo.x);
	bf_setting_set(hKey, "WHISPERWNDINFO.Y", whisperWndInfo.y);
	bf_setting_set(hKey, "WHISPERWNDINFO.W", whisperWndInfo.w);
	bf_setting_set(hKey, "WHISPERWNDINFO.H", whisperWndInfo.h);

	bf_setting_set(hKey, "FRIENDOPTIONWNDINFO.X", friendOptionWndInfo.x);
	bf_setting_set(hKey, "FRIENDOPTIONWNDINFO.Y", friendOptionWndInfo.y);
	bf_setting_set(hKey, "FRIENDOPTIONWNDINFO.W", friendOptionWndInfo.w);
	bf_setting_set(hKey, "FRIENDOPTIONWNDINFO.H", friendOptionWndInfo.h);

	bf_setting_set(hKey, "ITEMCOMPOSITIONWNDINFO.X", itemCompositionWndInfo.x);
	bf_setting_set(hKey, "ITEMCOMPOSITIONWNDINFO.Y", itemCompositionWndInfo.y);
	bf_setting_set(hKey, "ITEMCOMPOSITIONWNDINFO.W", itemCompositionWndInfo.w);
	bf_setting_set(hKey, "ITEMCOMPOSITIONWNDINFO.H", itemCompositionWndInfo.h);

	bf_setting_set(hKey, "ITEMIDENTIFYWNDINFO.X", itemIdentifyWndInfo.x);
	bf_setting_set(hKey, "ITEMIDENTIFYWNDINFO.Y", itemIdentifyWndInfo.y);
	bf_setting_set(hKey, "ITEMIDENTIFYWNDINFO.W", itemIdentifyWndInfo.w);
	bf_setting_set(hKey, "ITEMIDENTIFYWNDINFO.H", itemIdentifyWndInfo.h);

	bf_setting_set(hKey, "SUBCHATWNDST.X", subChatWndStInfo.x);
	bf_setting_set(hKey, "SUBCHATWNDST.Y", subChatWndStInfo.y);
	bf_setting_set(hKey, "SUBCHATWNDST.W", subChatWndStInfo.w);
	bf_setting_set(hKey, "SUBCHATWNDST.H", subChatWndStInfo.h);

	bf_setting_set(hKey, "SUBCHATWNDBT.X", subChatWndBtInfo.x);
	bf_setting_set(hKey, "SUBCHATWNDBT.Y", subChatWndBtInfo.y);
	bf_setting_set(hKey, "SUBCHATWNDBT.W", subChatWndBtInfo.w);
	bf_setting_set(hKey, "SUBCHATWNDBT.H", subChatWndBtInfo.h);

	bf_setting_write(hKey, "CHATWNDSTICKON", BF_SETTING_INT, &m_chatWndStickOn, 4);
	bf_setting_write(hKey, "ONSTSUBCHAT", BF_SETTING_INT, &m_onStSubChat, 4);
	bf_setting_write(hKey, "ONBTSUBCHAT", BF_SETTING_INT, &m_onBtSubChat, 4);
	bf_setting_set(hKey, "MEMORIALDUNWND.X", g_session.dunInfoWndX);
	bf_setting_set(hKey, "MEMORIALDUNWND.Y", g_session.dunInfoWndY);


	bf_setting_close(hKey);
}

void UIWindowMgr::InitSkillUseLevelInfoFromReg()
{
	int skillUseLevel;
	bf_setting_key hKey;
	bf_setting_key hResultKey;
	char sItemRegName[24];
	char UIPath[512];

	strcpy(UIPath, g_regPath);
	strcat(UIPath, g_suPath);

	bool lResult = bf_setting_open(hResultKey, UIPath);
	if (!lResult)
		return;
	bf_setting_close(hResultKey);
	lResult = bf_setting_open(hKey, UIPath);
	if (!lResult)
		return;

	int i;
	for (i = 1; i < 0x40f; ++i)
	{
		sprintf(sItemRegName, "%s%d", SKILL_INFO::GetSkillIdNameBySkillId(i), g_session.m_charSlot);
		lResult = bf_setting_get(hKey, sItemRegName, skillUseLevel);
		if (lResult) {
			if (SKILL_INFO::IsLevelUseSkill(i))
				g_session.SetSkillUseLevel(i, skillUseLevel);
			else
				g_session.SetSkillUseLevel(i, 0);
		} else
			g_session.SetSkillUseLevel(i, 0);
	}
	for (i = 0x1f40; i < 0x1f51; ++i)
	{
		sprintf(sItemRegName, "%s%d", SKILL_INFO::GetSkillIdNameBySkillId(i), g_session.m_charSlot);
		lResult = bf_setting_get(hKey, sItemRegName, skillUseLevel);
		if (lResult) {
			if (SKILL_INFO::IsLevelUseSkill(i))
				g_session.SetHomunSkillUseLevel(i, skillUseLevel);
			else
				g_session.SetHomunSkillUseLevel(i, 0);
		} else
			g_session.SetHomunSkillUseLevel(i, 0);
	}
	for (i = 0x2008; i < 0x202e; ++i)
	{
		sprintf(sItemRegName, "%s%d", SKILL_INFO::GetSkillIdNameBySkillId(i), g_session.m_charSlot);
		lResult = bf_setting_get(hKey, sItemRegName, skillUseLevel);
		if (lResult) {
			if (SKILL_INFO::IsLevelUseSkill(i))
				g_session.SetMerSkillUseLevel(i, skillUseLevel);
			else
				g_session.SetMerSkillUseLevel(i, 0);
		} else
			g_session.SetMerSkillUseLevel(i, 0);
	}
	bf_setting_close(hKey);
}

void UIWindowMgr::WriteSkillUseLevelInfoToReg()
{
	bf_setting_key hKey;
	char UIPath[512];
	char sItemRegName[24];
	int skillUseLevel;
	int i;

	strcpy(UIPath, g_regPath);
	strcat(UIPath, g_suPath);

	bool lResult = bf_setting_create(hKey, UIPath);
	if (!lResult)
		return;

	for (i = 1; i < 0x40f; ++i)
	{
		skillUseLevel = g_session.GetSkillUseLevel(i);
		sprintf(sItemRegName, "%s%d", SKILL_INFO::GetSkillIdNameBySkillId(i), g_session.m_charSlot);
		bf_setting_set(hKey, sItemRegName, skillUseLevel);
	}

	for (i = 0x1f40; i < 0x1f51; ++i)
	{
		skillUseLevel = g_session.GetHomunSkillUseLevel(i);
		sprintf(sItemRegName, "%s%d", SKILL_INFO::GetSkillIdNameBySkillId(i), g_session.m_charSlot);
		bf_setting_set(hKey, sItemRegName, skillUseLevel);
	}

	for (i = 0x2008; i < 0x202e; ++i)
	{
		skillUseLevel = g_session.GetMerSkillUseLevel(i);
		sprintf(sItemRegName, "%s%d", SKILL_INFO::GetSkillIdNameBySkillId(i), g_session.m_charSlot);
		bf_setting_set(hKey, sItemRegName, skillUseLevel);
	}

	bf_setting_close(hKey);
}

void UIWindowMgr::GetDeltaSnap(UIWindow *snapWindow, int &rdx, int &rdy, int mode)
{
	rdx = 0;
	rdy = 0;

	int mdx = 0;
	int mdy = 0;
	bool xNotAdded = true;
	bool yNotAdded = true;
	CSnapInfo snapInfo(snapWindow->m_x, snapWindow->m_y, snapWindow->m_w, snapWindow->m_h);
	CSnapInfo mgrSnapInfo(0, 0, m_w, m_h);
	mgrSnapInfo.Reverse();
	mgrSnapInfo.GetDeltaSnap(&snapInfo, mdx, mdy);

	if (mdx) {
		xNotAdded = false;
		rdx = mdx;
	}
	if (mdy) {
		yNotAdded = false;
		rdy = mdy;
	}

	if (!xNotAdded && !yNotAdded)
		return;

	int dx = 0;
	int dy = 0;
	bool AmIMaster = g_snapMgr.IsMasterSnap(snapWindow);

	std::map<UIWindow *, CSnapInfo>::iterator it = m_snapInfo.begin();
	while (it != m_snapInfo.end()) {
		bool isConnected = g_snapMgr.IsConnected(snapWindow, it->first);
		bool windowIsMaster = g_snapMgr.IsMasterSnap(it->first);
		bool notSameWindow = (snapWindow != it->first);
		bool windowSnapedToMaster = g_snapMgr.IsWindowSnapedToMaster(it->first);

		bool doSnap;
		if (mode)
			doSnap = notSameWindow && !windowIsMaster && !isConnected && !windowSnapedToMaster;
		else
			doSnap = notSameWindow && (!AmIMaster || (!windowSnapedToMaster && !isConnected));

		if (doSnap) {
			it->second.GetDeltaSnap(&snapInfo, dx, dy);
			if (dx && xNotAdded) {
				xNotAdded = false;
				rdx = dx;
			}
			if (dy && yNotAdded) {
				yNotAdded = false;
				rdy = dy;
			}
			if (!xNotAdded && !yNotAdded)
				return;
		}
		++it;
	}
}

void UIWindowMgr::UpdateSnapInfo(UIWindow *snapWindow)
{
	if (!g_session.m_isWindow)
		return;
	if (snapWindow == NULL)
		return;

	m_snapInfo[snapWindow].SetInfo(snapWindow->m_x, snapWindow->m_y, snapWindow->m_w, snapWindow->m_h);
}

void UIWindowMgr::DeleteSnapInfo(UIWindow *window)
{
	if (g_session.m_isWindow) {
		std::map<UIWindow *, CSnapInfo>::iterator it = m_snapInfo.find(window);
		if (it != m_snapInfo.end())
			m_snapInfo.erase(it);
	}
}

void UIWindowMgr::MakeWndPointerNull()
{
	m_lastHitWindow = NULL;
	m_captureWindow = NULL;
	m_editWindow = NULL;
	m_modalWindow = NULL;
	m_chatWnd = NULL;
	m_loadingWnd = NULL;
	m_basicInfoWnd = NULL;
	m_itemWnd = NULL;
	m_equipWnd = NULL;
	m_statusWnd = NULL;
	m_optionWnd = NULL;
	m_minimapZoomWnd = NULL;
	m_itemDropCntWnd = NULL;
	m_sayDialogWnd = NULL;
	m_chooseWnd = NULL;
	m_itemIdentifyWnd = NULL;
	m_itemCompositionWnd = NULL;
	m_chooseWarpWnd = NULL;
	m_menu = NULL;
	m_comboBoxWnd = NULL;
	m_itemCollectionWnd = NULL;
	m_combinedCardItemCollectionWnd = NULL;
	m_itemParamChangeDisplayWnd = NULL;
	m_skillDescribeWnd = NULL;
	m_questDetailWnd = NULL;
	m_quitWnd = NULL;
	m_restartWnd = NULL;
	m_cardItemIllustWnd = NULL;
	m_notifyLevelUpWnd = NULL;
	m_notifyJobLevelUpWnd = NULL;
	m_notifyQuestWnd = NULL;
	m_itemShopWnd = NULL;
	m_itemPurchaseWnd = NULL;
	m_itemSellWnd = NULL;
	m_chooseSellBuyWnd = NULL;
	m_chatRoomMakeWnd = NULL;
	m_chatRoomChangeWnd = NULL;
	m_chatRoomWnd = NULL;
	m_passwordWnd = NULL;
	m_exchangeWnd = NULL;
	m_exchangeAcceptWnd = NULL;
	m_itemStoreWnd = NULL;
	m_skillListWnd = NULL;
	m_messengerGroupWnd = NULL;
	m_joinPartyAcceptWnd = NULL;
	m_CoupleAcceptWnd = NULL;
	m_StarPlaceAcceptWnd = NULL;
	m_YourItemWnd = NULL;
	m_BabyAcceptWnd = NULL;
	m_BabyAcceptWnd2 = NULL;
	m_joinGuildAcceptWnd = NULL;
	m_allyGuildAcceptWnd = NULL;
	m_shortCutWnd = NULL;
	m_tipOfTheDayWnd = NULL;
	m_merchantItemWnd = NULL;
	m_merchantMirrorItemWnd = NULL;
	m_merchantShopMakeWnd = NULL;
	m_merchantItemShopWnd = NULL;
	m_merchantItemPurchaseWnd = NULL;
	m_merchantItemMyShopWnd = NULL;
	m_candidateWnd = NULL;
	m_compositionWnd = NULL;
	m_keyStrokeWnd = NULL;
	m_partySettingWnd = NULL;
	m_skillNameChangeWnd = NULL;
	m_npcEditDialogWnd = NULL;
	m_npcTextEditDialogWnd = NULL;
	m_detailLevelWnd = NULL;
	m_notifyItemObtainWnd = NULL;
	m_guildInfoManageWnd = NULL;
	m_guildMemberManageWnd = NULL;
	m_guildPositionManageWnd = NULL;
	m_guildSkillWnd = NULL;
	m_guildBanishedMemberWnd = NULL;
	m_guildNoticeWnd = NULL;
	m_guildTotalInfoWnd = NULL;
	m_friendOptionWnd = NULL;
	m_guildLeaveReasonDescWnd = NULL;
	m_guildBanReasonDescWnd = NULL;
	m_illustWnd = NULL;
	m_petInfoWnd = NULL;
	m_selectPetEggWnd = NULL;
	m_petTamingDeceiveWnd = NULL;
	m_itemRepairWnd = NULL;
	m_makingArrowListWnd = NULL;
	m_selectCartWnd = NULL;
	m_emotionWnd = NULL;
	m_emotionListWnd = NULL;
	m_bookWnd = NULL;
	m_joinFriendAcceptWnd = NULL;
	m_commandListWnd = NULL;
	m_homunInfoWnd = NULL;
	m_homunSkillListWnd = NULL;
	m_merInfoWnd = NULL;
	m_merSkillListWnd = NULL;
	m_autoMessageWnd = NULL;
	m_mailListWnd = NULL;
	m_mailViewWnd = NULL;
	m_auctionWnd = NULL;
	m_auctionMsgWnd = NULL;
	m_imgMenuWnd = NULL;
	m_selectCharWnd = NULL;
	m_selCharForUServerWnd = NULL;
	m_rebirthWnd = NULL;
	m_subChatWndST = NULL;
	m_subChatWndBT = NULL;
	m_subChatMiniWnd = NULL;
	m_battleMsgOpionWnd = NULL;
	m_memorialDunWnd = NULL;
	m_otherUserEquipWnd = NULL;
	m_roMapWnd = NULL;
}















UIWindowMgr::UIWindowMgr()
{
	MakeWndPointerNull();
	m_lastMouseX = -1;
	m_lastMouseY = -1;
	m_wallpaperSurface = NULL;
	m_w = 0;
	m_h = 0;
	m_chatMsg = "";
	m_chatWndStatus = 7;
	m_chatWndHeight = 98;
	m_chatWndX = 0;
	m_chatWndY = 0;
	m_chatWndShow = 1;
	RestoreChatWndStatus();
	m_miniMapZoomFactor = 4.0f;
	m_miniMapArgb = 0xaaffffff;
	m_isDrawCompass = 1;
	m_chatWndWhisperCurHis = 0;
	m_chatWndCurHis = 0;
	m_isInvalidatedByForce = 0;
	m_conversionMode = (g_languageType == LanguageJapanese);
	ChatWndStickOnOff(1);
	m_onBtSubChat = 0;
	m_onStSubChat = 0;

	if (g_serviceType == ServiceKorea) {
		T_Logo_X = 0;
		T_Logo_Y = -25;
		T_Char_X = 0;
		T_Char_Y = 0;
		T_Tape_Y = 0;
		T_12Age_X = 0;
		T_12Age_Y = 0;
		T_Grav_X = 0;
		T_Grav_Y = 0;
		T_Intel_X = 0;
		T_Intel_Y = 0;
		T_CLICK = 0;
	}
	else {
		T_Logo_X = 0;
		T_Logo_Y = 0;
		T_Char_X = 0;
		T_Char_Y = 0;
		T_Tape_Y = 0;
		T_12Age_X = 0;
		T_12Age_Y = 0;
		T_Grav_X = 0;
		T_Grav_Y = 0;
		T_Intel_X = 0;
		T_Intel_Y = 0;
		T_CLICK = 0;
	}
}

UIWindowMgr::~UIWindowMgr()
{
	RemoveAllWindows();
	DestroyDC();
}

void UIWindowMgr::SetSize(int cx, int cy)
{
	m_w = cx;
	m_h = cy;
	m_chatWndX = 0;
	m_chatWndY = cy - m_chatWndHeight;
}

#if defined(BF_PORT)
// The port's resize path (M4.3a). MakeWindow records which window it made,
// so that a resize can move the windows it placed by the screen's size by
// as much as that placement moves. The size is kept to tell a reused
// address from the window that was recorded.
struct BfMadeWindow { int id; int w; int h; };
// Never destroyed: ~UIWindowMgr deletes its windows during static destruction.
static std::map<UIWindow *, BfMadeWindow> &g_bfMadeWindows = *new std::map<UIWindow *, BfMadeWindow>;

// A deleted window leaves the record (it kept every window ever made, M8.6).
void UIWindowMgr::BfForgetWindow(UIWindow *window)
{
	g_bfMadeWindows.erase(window);
}

// Where MakeWindow puts a window on a screen of cx x cy, for the windows of
// the login mode (the UIX, UIY and UICY of UISys.h at that size).
static bool BfPlacement(int id, int cx, int cy, int *x, int *y)
{
	int uix = (cx - 640) / 2, uiy = (cy - 480) / 2;
	switch (id) {
	case WID_LOGINWND:
	case WID_WAITWND:
		*x = uix + 185; *y = cy * 300 / 480; return true;
	case WID_SELECTSERVERWND:
		*x = uix + 185; *y = cy * 300 / 480 - 80; return true;
	case WID_NOTICEWND:
		*x = uix + 33; *y = uiy + 40; return true;
	case WID_MAKECHARWND:
	case WID_SELECTCHARWND:
		*x = uix + 33; *y = uiy + 65; return true;
	case WID_LOADINGWND:
		*x = cx / 2 - 100; *y = cy / 2 + 160; return true;
	}
	return false;
}

// The screen is now cx x cy logical pixels. A window of the login mode moves
// with its placement (so the login window stays where the client puts it at
// that size, and a window the player dragged keeps its offset); any other
// window keeps the screen edge it touches, else its offset from the
// centre horizontally and its share of the height vertically (as UIX and
// UICY place most of them). Then every window is clamped back into view and
// the UI is redrawn. The original fixed the size at start-up.
void UIWindowMgr::BfOnResize(int cx, int cy)
{
	int oldW = m_w, oldH = m_h;
	SetSize(cx, cy);
	if (oldW <= 0 || oldH <= 0 || (oldW == cx && oldH == cy))
		return;
	for (std::list<UIWindow *>::iterator i = m_children.begin(); i != m_children.end(); ++i) {
		UIWindow *window = *i;
		int x = window->m_x, y = window->m_y, x0, y0, x1, y1;
		std::map<UIWindow *, BfMadeWindow>::iterator made = g_bfMadeWindows.find(window);
		if (made != g_bfMadeWindows.end() && made->second.w == window->m_w && made->second.h == window->m_h
			&& BfPlacement(made->second.id, oldW, oldH, &x0, &y0) && BfPlacement(made->second.id, cx, cy, &x1, &y1)) {
			x += x1 - x0;
			y += y1 - y0;
		} else {
			if (x + window->m_w >= oldW && x > 0)
				x += cx - oldW;
			else if (x > 0)
				x += (cx - oldW) / 2;
			if (y + window->m_h >= oldH && y > 0)
				y += cy - oldH;
			else if (y > 0)
				y = y * cy / oldH;
		}
		if (x > cx - window->m_w) x = cx - window->m_w;
		if (y > cy - window->m_h) y = cy - window->m_h;
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x != window->m_x || y != window->m_y)
			window->Move(x, y);
	}
	// The minimap's zoom buttons belong to the minimap, which
	// CGameMode::DrawMiniMap draws at the right edge at every size; they keep
	// MakeWindow's place beside it (17 px from the edge, not touching it).
	if (m_minimapZoomWnd)
		m_minimapZoomWnd->Move(cx - 29, m_minimapZoomWnd->m_y);
	// The status icons (CRagEffect::StatusState) are laid out from the
	// screen's width and height when the state list changes; lay them out
	// again, as a state change does (Zc_Msg_State_Change: AM_SETCOUNT 1).
	if (CGameMode *game = g_modeMgr.GetGameMode()) {   // NULL outside the game
		CWorld *world = game->GetWorld();
		if (world && world->m_player && world->m_player->m_statusEffect)
			world->m_player->m_statusEffect->SendMsg(NULL, AM_SETCOUNT, 1, 0, 0);
	}
	InvalidateAll();
}
#endif

void UIWindowMgr::CreateDCA(int cx, int cy) {}

void UIWindowMgr::DestroyDC()
{
	if (m_wallpaperSurface) {
		delete m_wallpaperSurface;
		m_wallpaperSurface = NULL;
	}
}

void UIWindowMgr::ClearDC(unsigned int color)
{
	if (m_wallpaperSurface) { delete m_wallpaperSurface; m_wallpaperSurface = NULL; }
}

void UIWindowMgr::AddWindow(UIWindow* window)
{
	m_children.push_back(window);
}

void UIWindowMgr::AddWindowFront(UIWindow* window)
{
	m_children.push_front(window);
}

void UIWindowMgr::RemoveWindow(UIWindow *window)
{
	if (window == NULL)
		return;

	std::list<UIWindow *>::iterator it = m_children.begin();
	while (it != m_children.end())
	{
		if (*it == window)
		{
			if (g_session.m_isWindow)
				DeleteSnapInfo(window);

			g_snapMgr.RemoveSnapObject(*it);
			(*it)->StoreInfo();
			delete *it;
			m_children.erase(it);
			return;
		}
		++it;
	}
}

void UIWindowMgr::HideChatWnd()
{
	if (m_chatWnd)
		m_chatWnd->SetShow(0);
}





void UIWindowMgr::RemoveAllWindowsExceptChatWnd()
{
	if (m_chatWnd != NULL)
	{
		std::list<UIWindow *>::iterator it = m_children.begin();
		while (it != m_children.end())
		{
			if ((UIWindow *)m_chatWnd != (*it) &&
				(UIWindow *)m_subChatWndST != (*it) &&
				(UIWindow *)m_subChatWndBT != (*it) &&
				(UIWindow *)m_subChatMiniWnd != (*it))
			{
				if ((*it) == (UIWindow *)m_memorialDunWnd && g_session.m_onDunInfoWnd)
				{
					g_session.dunInfoWndX = (*it)->m_x;
					g_session.dunInfoWndY = (*it)->m_y;
				}
				(*it)->StoreInfo();
				delete (*it);
				it = m_children.erase(it);
			}
			else
				++it;
		}

		it = m_quitWindow.begin();
		while (it != m_quitWindow.end())
		{
			if ((UIWindow *)m_chatWnd != (*it) &&
				(UIWindow *)m_subChatWndST != (*it) &&
				(UIWindow *)m_subChatWndBT != (*it) &&
				(UIWindow *)m_subChatMiniWnd != (*it))
			{
				it = m_quitWindow.erase(it);
			}
			else
				++it;
		}

		std::map<UIWindow *, CSnapInfo>::iterator snapIt = m_snapInfo.begin();
		while (snapIt != m_snapInfo.end())
		{

			if ((UIWindow *)m_chatWnd != snapIt->first &&
				(UIWindow *)m_subChatWndST != *it &&
				(UIWindow *)m_subChatWndBT != *it &&
				(UIWindow *)m_subChatMiniWnd != *it)
			{
				snapIt = m_snapInfo.erase(snapIt);
			}
			else
				++snapIt;
		}

		UISubChatHisWnd *subChatWndST = NULL;
		UISubChatHisWnd *subChatWndBT = NULL;
		UISubChatMiniWnd *subChatMiniWnd = NULL;
		UINewChatWnd *chatWnd = NULL;
		if (m_subChatWndST != NULL)
			subChatWndST = m_subChatWndST;
		if (m_subChatWndBT != NULL)
			subChatWndBT = m_subChatWndBT;
		if (m_subChatMiniWnd != NULL)
			subChatMiniWnd = m_subChatMiniWnd;
		if (m_chatWnd != NULL)
			chatWnd = m_chatWnd;

		MakeWndPointerNull();
		m_chatWnd = chatWnd;
		m_subChatWndST = subChatWndST;
		m_subChatWndBT = subChatWndBT;
		m_subChatMiniWnd = subChatMiniWnd;
	}
	else
		RemoveAllWindows();
}

void UIWindowMgr::RemoveAllWindows()
{
	std::list<UIWindow *>::iterator it = m_children.begin();
	while (it != m_children.end())
	{
		(*it)->StoreInfo();
		delete *it;
		++it;
	}

	m_children.erase(m_children.begin(), m_children.end());
	m_quitWindow.erase(m_quitWindow.begin(), m_quitWindow.end());
	m_snapInfo.erase(m_snapInfo.begin(), m_snapInfo.end());
	MakeWndPointerNull();
}

void UIWindowMgr::MakeTopLayer(UIWindow* window)
{
	if (m_roMapWnd)
		return;

	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		if (*i == window) {
			m_children.erase(i);
			m_children.push_back(window);
			return;
		}
		i++;
	}
}

void UIWindowMgr::InvalidateAll()
{
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		(*i)->InvalidateChildren();
		++i;
	}
}

void UIWindowMgr::InvalidateUpdateNeededUI()
{
	if (m_isInvalidatedByForce)
		return;
	m_isInvalidatedByForce = 1;
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		if ((*i)->IsUpdateNeed())
			(*i)->InvalidateChildren();
		++i;
	}
}

bool UIWindowMgr::Drop(int dropX, int dropY, const DRAG_INFO* const dragInfo)
{
	UIWindow* hit = HitTest(dropX, dropY);
	if (hit) {
		int px, py;
		GetWindowPos(hit, &px, &py);
		hit->DragAndDrop(dropX - px, dropY - py, dragInfo);
		return true;
	}
	return false;
}
void UIWindowMgr::EditFocusToChatWnd()
{
	if (m_chatWnd)
		SetFocusEdit((UIWindow*)m_chatWnd->m_commonChat);
}



int UIWindowMgr::ProcessInput()
{
	int rValue = 0;
	int x = g_mouse.GetXPos();
	int y = g_mouse.GetYPos();

	if (m_modalWindow == NULL) {
		std::list<UIWindow *>::iterator it = m_quitWindow.begin();
		while (it != m_quitWindow.end()) {
			UIWindow *window = *it;

			if (m_captureWindow)
			{
				if (m_captureWindow == window || m_captureWindow->IsChildOf(window))
					m_captureWindow = NULL;
			}
			if (m_editWindow)
			{
				if (m_editWindow == window || m_editWindow->IsChildOf(window))
					m_editWindow = NULL;
			}
			if (m_modalWindow)
			{
				if (m_modalWindow == window || m_modalWindow->IsChildOf(window))
					m_modalWindow = NULL;
			}
			if (m_lastHitWindow)
			{
				if (m_lastHitWindow == window || m_lastHitWindow->IsChildOf(window))
					m_lastHitWindow = NULL;
			}

			RemoveWindow(window);
			it++;
		}
		m_quitWindow.erase(m_quitWindow.begin(), m_quitWindow.end());
	}

	if (m_captureWindow) {
		rValue = 1;
		int gx, gy;
		GetWindowPos(m_captureWindow, &gx, &gy);

		if (g_mouse.GetLBtn() == BTN_DOWN) {
			UIWindow *ancestor = m_captureWindow->GetAncestor();
			if (ancestor->IsFrameWnd())
				MakeTopLayer(ancestor);
			m_captureWindow->OnLBtnDown(x - gx, y - gy);
		}

		if (g_mouse.GetLBtn() == BTN_DBLCLK) {
			m_captureWindow->OnLBtnDown(x - gx, y - gy);
			m_captureWindow->OnLBtnDblClk(x - gx, y - gy);
		}

		m_captureWindow->OnMouseShape(x - gx, y - gy);

		if (m_lastMouseX != x || m_lastMouseY != y)
			m_captureWindow->OnMouseMove(x - gx, y - gy);
		else
			m_captureWindow->OnMouseHover(x - gx, y - gy);

		if (g_mouse.GetWheel())
			this->GetCapture()->OnWheel(g_mouse.GetWheel());

		if (g_mouse.GetLBtn() == BTN_UP)
			m_captureWindow->OnLBtnUp(x - gx, y - gy);
	}
	else {
		UIWindow *hit = HitTest(x, y);

		if (m_modalWindow && hit) {
			if (hit != m_modalWindow && !hit->IsChildOf(m_modalWindow))
				hit = NULL;
		}

		if (hit) {
			rValue = hit->IsTransmitMouseInput() ? 2 : 1;

			int gx, gy;
			GetWindowPos(hit, &gx, &gy);

			if (g_mouse.GetLBtn() == BTN_DBLCLK)
				hit->OnLBtnDblClk(x - gx, y - gy);

			if (g_mouse.GetRBtn() == BTN_DBLCLK)
				hit->OnRBtnDblClk(x - gx, y - gy);

			if (g_mouse.GetLBtn() == BTN_DOWN) {
				UIWindow *ancestor = hit->GetAncestor();
				if (ancestor->IsFrameWnd())
					MakeTopLayer(ancestor);
				hit->OnLBtnDown(x - gx, y - gy);
			}

			if (g_mouse.GetRBtn() == BTN_DOWN) {
				UIWindow *ancestor = hit->GetAncestor();
				if (ancestor->IsFrameWnd())
					MakeTopLayer(ancestor);
				hit->OnRBtnDown(x - gx, y - gy);
			}

			hit->OnMouseShape(x - gx, y - gy);

			if (m_lastMouseX != x || m_lastMouseY != y)
				hit->OnMouseMove(x - gx, y - gy);
			else
				hit->OnMouseHover(x - gx, y - gy);

			if (g_mouse.GetWheel())
				hit->OnWheel(g_mouse.GetWheel());

			if (g_mouse.GetRBtn() == BTN_UP)
				hit->OnRBtnUp(x - gx, y - gy);

			if (g_mouse.GetLBtn() == BTN_UP)
				hit->OnLBtnUp(x - gx, y - gy);
		}

		g_mouse_L = g_mouse.GetLBtn();

		if (g_mouse_L == BTN_DOWN) {
			if (m_comboBoxWnd) {
				if (hit) {
					if (hit != (UIWindow *)m_comboBoxWnd && !hit->IsChildOf((UIWindow *)m_comboBoxWnd))
						DeleteWindow(WID_COMBOBOXWND);
				}
				else
					DeleteWindow(WID_COMBOBOXWND);
			}
		}

		if (m_selectCharWnd)
			m_selectCharWnd->Invalidate();

		if (m_equipWnd)
			m_equipWnd->Invalidate();

		if (hit) {
			if (m_lastHitWindow) {
				if (hit != m_lastHitWindow) {
					int gx = 0, gy = 0;
					m_lastHitWindow->GetGlobalCoor(gx, gy);
					m_lastHitWindow->OnMouseMove(x - gx, y - gy);
					m_lastHitWindow->OnMouseShape(x - gx, y - gy);
				}
			}
			m_lastHitWindow = hit;
		}
	}

	if (GetFocusEdit())
		GetFocusEdit()->Invalidate();

	if (x == m_lastMouseX && y == m_lastMouseY)
		g_mouseMoved = false;
	else
		g_mouseMoved = true;

	m_lastMouseX = x;
	m_lastMouseY = y;

	if (m_modalWindow)
		return 1;
	return rValue;
}

void UIWindowMgr::ClearAllWindowState()
{
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		(*i)->ClearAllChildrenState();
		++i;
	}
}

void UIWindowMgr::OnProcess()
{
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		(*i)->OnProcess();
		++i;
	}
}

void UIWindowMgr::Render()
{
	BOXINFO boxInfo;
	std::list<UIWindow *>::iterator i = m_children.begin();

	for (; i != m_children.end(); ++i) {
		UIWindow* window = *i;
		if (window->m_show) {
			window->DoDraw(0);
			if (window->GetTransBoxInfo(&boxInfo)) {
				DrawBoxScreen(boxInfo.x, boxInfo.y, boxInfo.cx, boxInfo.cy, boxInfo.color);
			} else if (window->GetTransBoxInfo2(&boxInfo)) {
				window->DrawBoxScreen2();
			}

			if (window == m_roMapWnd)
				m_roMapWnd->DrawRoMap();

			window->m_dc->DrawSurface(window->m_x, window->m_y, window->m_w, window->m_h, window->GetColor());
		}
	}
}

#if defined(BF_PORT)
// The size of the wallpaper's bitmap (SetWallpaper), for the cover rule.
static int s_bfWallpaperW;
static int s_bfWallpaperH;

// The "cover" rule for the full-screen pictures (owner's request F2): a
// picture of iw x ih is scaled uniformly until it fills w x h, centred, the
// overflow cropped (it is drawn past the screen's edges). The 2008 client
// stretched it to the screen instead, which distorts it at any aspect but
// the picture's own. At a whole factor (1 to 4) the UI pass samples it
// unfiltered (Renderer.cpp BfTexelExact), each texel a clean block of logical
// pixels and so of device pixels at UI scale 1, 2 and 3; at any other factor
// it is filtered (see RenderTitleGraphic for why).
static void BfCoverRect(int iw, int ih, int w, int h, int* x, int* y, int* cw, int* ch)
{
	float s = (float)w / iw > (float)h / ih ? (float)w / iw : (float)h / ih;
	*cw = (int)ceilf(iw * s - 0.001f);
	*ch = (int)ceilf(ih * s - 0.001f);
	*x = (w - *cw) / 2;
	*y = (h - *ch) / 2;
}
#endif

void UIWindowMgr::RenderWallPaper()
{
	m_isInvalidatedByForce = 0;

#if defined(BF_PORT)
	if (m_wallpaperSurface && s_bfWallpaperW > 0 && s_bfWallpaperH > 0) {
		int x, y, w, h;
		BfCoverRect(s_bfWallpaperW, s_bfWallpaperH, m_w, m_h, &x, &y, &w, &h);
		m_wallpaperSurface->DrawSurfaceStretch(((int)g_3dDevice.GetWidth() - m_w) / 2 + x, ((int)g_3dDevice.GetHeight() - m_h) / 2 + y, w, h);
		return;
	}
#endif
	if (m_wallpaperSurface)
		m_wallpaperSurface->DrawSurfaceStretch(((int)g_3dDevice.GetWidth() - m_w) / 2, ((int)g_3dDevice.GetHeight() - m_h) / 2, m_w, m_h);
}

// The title screen of the login mode for the service types that have one
// (CLoginMode::OnUpdate): a background of 4 x 3 tiles, four tape strips, the
// age rating, the developer's mark and the logo, each stretched to fractions
// of the screen, as the reference does (0x4EB2A0, called from
// CLoginMode::OnUpdate 0x5D54A0). A click picks a piece and a drag moves it.
// The file names are CP949: "T_<background><row>-<column>.bmp",
// "T_<tape><n>.tga", "T_12<years>.tga", "T_<developer>.tga", "T_R<n>.tga"
// in the interface folder.
//
// Under BF_PORT (owner's request F2) nothing is stretched. The background,
// 4 x 3 tiles of 256 x 256 (a 1024 x 768 picture), follows the "cover" rule:
// scaled uniformly until it fills the screen, centred, the overflow cropped
// (see BfCoverRect). The pieces drawn over it (tapes, age rating, developer's
// mark, logo) are not part of that picture: they keep the 2008 layout of a
// 1024 x 768 screen at the largest uniform scale at which all of it fits, so
// none is cropped, and each stays against the edges it sits on there (logo top
// left, age rating top right, developer's mark bottom right, tapes along the
// bottom, centred, their black band carried on to both edges). At 1024 x 768
// every piece is where and as large as the 2008 client draws it, texel for
// pixel. Filtering is the UI pass's rule (Renderer.cpp BfTexelExact): a piece
// drawn at a whole factor is sampled unfiltered (clean blocks), at any other
// factor linearly: the title is a painted, photo-like picture, which a
// filter scales smoothly, where unfiltered sampling at a factor like 1.17 would
// double some texel rows and columns and not others and show as a ragged
// pattern across the faces and the logo's edges.
void UIWindowMgr::RenderTitleGraphic()
{
	extern char title_random;
	RECT rect = { 0, 0, 255, 255 };
	float oow = 1.0f / NEARPLANE - 0.0001f;
	short cw = (short)(g_renderer->m_width / 4);   // a tile: a quarter of the width, a third of the height
	short ch = (short)(g_renderer->m_height / 3);
#if defined(BF_PORT)
	float bfW = (float)g_renderer->m_width, bfH = (float)g_renderer->m_height;
	float bfCover = bfW / 1024.0f > bfH / 768.0f ? bfW / 1024.0f : bfH / 768.0f;
	float bfFit = bfW / 1024.0f < bfH / 768.0f ? bfW / 1024.0f : bfH / 768.0f;
	cw = ch = (short)(256.0f * bfFit + 0.5f);   // a tile of the pieces: square, as the art is
	if (cw < 1)
		cw = ch = 1;
	int bfRight = g_renderer->m_width - cw * 4;    // where the pieces against the right edge move
	int bfBottom = g_renderer->m_height - ch * 3;  // and those against the bottom edge
	int bfMiddle = bfRight / 2;                    // and the tapes, centred
#else
	const int bfRight = 0, bfBottom = 0, bfMiddle = 0;
#endif
	float fcw = (float)cw;
	float fch = (float)ch;
	char name[256];

	if (g_mouse.GetLBtn() == BTN_DOWN) {
		int mx = g_mouse.GetXPos();
		int my = g_mouse.GetYPos();
		T_CLICK = 0;
		if (fcw * 3.4f + T_12Age_X + bfRight < mx && mx < fcw * 3.9f + T_12Age_X + bfRight
			&& fch * 0.2f + T_12Age_Y < my && my < fch * 0.6f + T_12Age_Y)
			T_CLICK = 3;
		if (fcw * 3.3f + T_Grav_X + bfRight < mx && mx < fcw * 3.9f + T_Grav_X + bfRight
			&& fch * 2.4f + T_Grav_Y + bfBottom < my && my < fch * 2.8f + T_Grav_Y + bfBottom)
			T_CLICK = 2;
		if (fcw * 0.1f + T_Logo_X < mx && mx < fcw * 1.8f + T_Logo_X
			&& fch * 0.3f + T_Logo_Y < my && my < fch * 1.2f + T_Logo_Y)
			T_CLICK = 1;
		if (g_serviceType != ServiceKorea && title_random > 0
			&& fcw * 2.2f + T_Char_X < mx && mx < fcw * 3.5f + T_Char_X
			&& T_Char_Y < my && my < T_Char_Y + ch * 3)
			T_CLICK = 5;
		if (fch * 2.7f + T_Tape_Y + bfBottom < my && my < fch * 3.0f + T_Tape_Y + bfBottom)
			T_CLICK = 4;
	}

	if (g_mouse.GetLBtn() == BTN_PRESSED) {
		int dx = g_mouse.GetXDelta();
		int dy = g_mouse.GetYDelta();
		switch (T_CLICK) {
		case 1: T_Logo_X += dx; T_Logo_Y += dy; break;
		case 2: T_Grav_X += dx; T_Grav_Y += dy; break;
		case 3: T_12Age_X += dx; T_12Age_Y += dy; break;
		case 6: T_Intel_X += dx; T_Intel_Y += dy; break;
		case 4:
			if (T_Tape_Y + dy >= 0)
				T_Tape_Y += dy;
			break;
		case 5:
			if (g_serviceType != ServiceKorea || T_Char_X + dx >= 0)
				T_Char_X += dx;
			break;
		}
	}

	CMode *mode = g_modeMgr.GetLoginMode();
	if (!mode)
		return;

	// The background; services other than Korea take the T2_ set for the first title.
	char col = 0;
	for (int x = 0; x < cw * 4; x += cw) {
		++col;
		char row = 0;
		for (int y = 0; y < ch * 3; y += ch) {
			++row;
#if defined(BF_PORT)
			{
				float tile = 256.0f * bfCover;
				float x0 = (bfW - tile * 4.0f) / 2.0f, y0 = (bfH - tile * 3.0f) / 2.0f;
				rect.left = (int)floorf(x0 + tile * (col - 1) + 0.5f);
				rect.top = (int)floorf(y0 + tile * (row - 1) + 0.5f);
				rect.right = (int)floorf(x0 + tile * col + 0.5f);
				rect.bottom = (int)floorf(y0 + tile * row + 0.5f);
			}
#else
			rect.left = x;
			rect.top = y;
			rect.right = x + cw;
			rect.bottom = y + ch;
#endif
			if (g_serviceType != ServiceKorea && title_random == 0)
				sprintf(name, "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\T2_\xb9\xe8\xb0\xe6%d-%d.bmp", row, col);
			else
				sprintf(name, "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\T_\xb9\xe8\xb0\xe6%d-%d.bmp", row, col);
			mode->DrawTexture(&rect, 0, oow, 0xffffffff, g_texMgr.GetTexture(name, false));
		}
	}

	// The tapes along the bottom.
	char tape = 0;
	for (int x = 0; x < cw * 4; x += cw) {
		++tape;
		rect.left = x + bfMiddle;
		rect.top = (int)(fch * 2.5f + T_Tape_Y) + bfBottom;
		rect.right = x + cw + bfMiddle;
		rect.bottom = (int)(fch * 3.0f + T_Tape_Y) + bfBottom;
		sprintf(name, "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\T_\xc5\xd7\xc0\xd4%d.tga", tape);
		mode->DrawTexture(&rect, 0, oow, 0xffffffff, g_texMgr.GetTexture(name, false));
#if defined(BF_PORT)
		// The band carried on to the screen's edge: the outer tapes' outermost
		// texel column (black under the text), drawn across the gap.
		if (bfMiddle > 0 && (tape == 1 || tape == 4)) {
			CTexture* tex = g_texMgr.GetTexture(name, false);
			float u = tape == 1 ? 0.5f / 256.0f : 255.5f / 256.0f;
			rect.left = tape == 1 ? 0 : rect.right;
			rect.right = tape == 1 ? bfMiddle : g_renderer->m_width;
			mode->DrawTexture(&rect, 0, oow, 0xffffffff, tex, UVRECT(u, 0.0f, u, 1.0f));
		}
#endif
	}

	// The age rating, top right.
	rect.left = (int)(T_12Age_X + fcw * 3.0f) + bfRight;
	rect.top = T_12Age_Y;
	rect.right = rect.left + cw;
	rect.bottom = T_12Age_Y + ch;
	mode->DrawTexture(&rect, 0, oow, 0xffffffff, g_texMgr.GetTexture("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\T_12\xbc\xbc.tga", false));

	// The developer's mark, bottom right, half a tile high.
	rect.left = (int)(T_Grav_X + fcw * 3.1f) + bfRight;
	rect.top = (int)(T_Grav_Y + fch * 2.4f) + bfBottom;
	rect.right = rect.left + cw;
	rect.bottom = (int)(rect.top + fch * 0.5f);
	mode->DrawTexture(&rect, 0, oow, 0xffffffff, g_texMgr.GetTexture("\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\T_\xc1\xdf\xb7\xc2\xbc\xba\xc0\xce.tga", false));

	// The logo in two columns: R1 over R3, R2 over R4; R1 and R2 a tile high, R3 and R4 half.
	char piece = -2;
	for (int x = T_Logo_X; x < T_Logo_X + cw * 2; x += cw) {
		++piece;
		int y = (int)(fch * 0.1f + T_Logo_Y);
		if (!(y < fch * 2.0f + T_Logo_Y))
			continue;
		int left = (int)(fcw * 0.2f + x);
		char n = piece;
		do {
			n += 2;
			rect.left = left;
			rect.top = y;
			rect.right = left + cw;
			rect.bottom = n <= 2 ? y + ch : (int)(y + fch * 0.5f);
			sprintf(name, "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\T_R%d.tga", n);
			mode->DrawTexture(&rect, 0, oow, 0xffffffff, g_texMgr.GetTexture(name, false));
			y += ch;
		} while (y < fch * 2.0f + T_Logo_Y);
	}
}

void UIWindowMgr::DrawBitmapToFrame(int x, int y, CBitmapRes* bitmap)
{
	if (!bitmap)
		return;
	CDC* dc = g_3dDevice.CreateDCBuffer(bitmap->m_width, bitmap->m_height);
	dc->Update(0, 0, bitmap->m_width, bitmap->m_height, bitmap->m_data, true);
	dc->DrawSurface(x, y, bitmap->m_width, bitmap->m_height, 0xffffffff);
	delete dc;
}

void UIWindowMgr::SetWallpaper(CBitmapRes* bitmap)
{
	if (!bitmap) {
		if (m_wallpaperSurface) { delete m_wallpaperSurface; m_wallpaperSurface = NULL; }
		return;
	}
	if (!m_wallpaperSurface)
		m_wallpaperSurface = g_3dDevice.CreateWallPaper(bitmap->m_width, bitmap->m_height);
	if (m_wallpaperSurface)
		m_wallpaperSurface->Update(0, 0, bitmap->m_width, bitmap->m_height, bitmap->m_data, 0, 0);
#if defined(BF_PORT)
	s_bfWallpaperW = bitmap->m_width;
	s_bfWallpaperH = bitmap->m_height;
#endif
}

void UIWindowMgr::FindFocusEditAndSetFocus()
{
	std::list<UIWindow *>::iterator it = m_children.begin();
	while (it != m_children.end()) {
		UIWindow* child = *it;
		if (child->m_show) {
			std::list<UIWindow *>::iterator j = child->m_children.begin();
			while (j != child->m_children.end()) {
				UIWindow* w = *j;
				if (w->CanGetFocus()) {
					SetFocusEdit(w);
					return;
				}
				j++;
			}
		}
		it++;
	}
}

bool UIWindowMgr::IsFirstFocusEdit(UIWindow* wnd)
{
	if (!wnd)
		return false;
	if (wnd->m_parent->GetPrevSiblingOf(wnd))
		return false;
	return true;
}

bool UIWindowMgr::IsLastFocusEdit(UIWindow* wnd)
{
	if (!wnd)
		return false;
	if (wnd->m_parent->GetNextSiblingOf(wnd))
		return false;
	return true;
}

void UIWindowMgr::SetFocusEditNext()
{
	if (!m_editWindow)
		return;
	UIWindow* next = m_editWindow->m_parent->GetNextSiblingOf(m_editWindow);
	if (next) {
		if (!next->CanGetFocus())
			return;
		SetFocusEdit(next);
	} else {
		SetFocusEdit(NULL);
	}
}

void UIWindowMgr::SetFocusEditFirst()
{
	if (m_editWindow) {
		UIWindow* parent = m_editWindow->m_parent;
		std::list<UIWindow *>::iterator i = parent->m_children.begin();
		while (i != parent->m_children.end()) {
			UIWindow* w = *i;
			if (w->CanGetFocus()) {
				SetFocusEdit(w);
				return;
			}
			++i;
		}
	}
	SetFocusEdit(NULL);
}

void UIWindowMgr::SetFocusEditPrev()
{
	if (!m_editWindow)
		return;
	UIWindow* prev = m_editWindow->m_parent->GetPrevSiblingOf(m_editWindow);
	if (prev) {
		if (!prev->CanGetFocus())
			return;
		SetFocusEdit(prev);
	} else {
		SetFocusEdit(NULL);
	}
}

void UIWindowMgr::SetFocusEditLast()
{
	if (m_editWindow) {
		UIWindow* parent = m_editWindow->m_parent;
		std::list<UIWindow *>::reverse_iterator i = parent->m_children.rbegin();
		while (i != parent->m_children.rend()) {
			UIWindow* w = *i;
			if (w->CanGetFocus()) {
				SetFocusEdit(w);
				return;
			}
			i++;
		}
	}
	SetFocusEdit(NULL);
}

void UIWindowMgr::FinishEdit()
{
	if (m_editWindow)
		m_editWindow->OnFinishEdit();
}

void UIWindowMgr::SetFocusEdit(UIWindow* wnd)
{
	// The 2008 client (0x4EBFC0) has no condition here; the reconstruction
	// skipped the chat window's common line, which then never got the focus.
	if (m_editWindow)
		m_editWindow->OnFinishEdit();
	m_editWindow = wnd;
	if (wnd)
		wnd->OnBeginEdit();
}

UIWindow* UIWindowMgr::GetFocusEdit() { return m_editWindow; }

void UIWindowMgr::SetCapture(UIWindow* window)
{
	m_captureWindow = window;
}

UIWindow* UIWindowMgr::GetCapture() { return m_captureWindow; }

void UIWindowMgr::ReleaseCapture()
{
	m_captureWindow = NULL;
}

void UIWindowMgr::DeleteButton()
{
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		if ((*i)->IsFrameWnd())
			(*i)->SendMsg(NULL, UM_DELETE, 0, 0, 0, 0);
		++i;
	}
}

void UIWindowMgr::PageUpButton()
{
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		if ((*i)->IsFrameWnd())
			(*i)->SendMsg(NULL, UM_PAGEUPPUSH, 0, 0, 0, 0);
		++i;
	}
}

void UIWindowMgr::LeftButton()
{
	if (m_changeNameWnd) {
		m_changeNameWnd->SendMsg(NULL, UM_LEFTPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_bookWnd) {
		m_bookWnd->SendMsg(NULL, UM_LEFTPUSH, 0, 0, 0, 0);
		return;
	}
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		if ((*i)->IsFrameWnd())
			(*i)->SendMsg(NULL, UM_LEFTPUSH, 0, 0, 0, 0);
		++i;
	}
}

void UIWindowMgr::RightButton()
{
	if (m_changeNameWnd) {
		m_changeNameWnd->SendMsg(NULL, UM_RIGHTPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_bookWnd) {
		m_bookWnd->SendMsg(NULL, UM_RIGHTPUSH, 0, 0, 0, 0);
		return;
	}
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		if ((*i)->IsFrameWnd())
			(*i)->SendMsg(NULL, UM_RIGHTPUSH, 0, 0, 0, 0);
		++i;
	}
}

void UIWindowMgr::UpButton()
{
	if (m_chooseWnd) {
		m_chooseWnd->SendMsg(NULL, UM_UPPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_sayDialogWnd) {
		m_sayDialogWnd->SendMsg(NULL, UM_UPPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_comboBoxWnd) {
		m_comboBoxWnd->SendMsg(NULL, UM_UPPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_chooseWarpWnd) {
		m_chooseWarpWnd->SendMsg(NULL, UM_UPPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_metalListWnd) {
		((UIWindow*)m_metalListWnd)->SendMsg(NULL, UM_UPPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_editWindow) {
		m_editWindow->SendMsg(NULL, UM_UPPUSH, 0, 0, 0, 0);
		return;
	}
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		if ((*i)->IsFrameWnd())
			(*i)->SendMsg(NULL, UM_UPPUSH, 0, 0, 0, 0);
		++i;
	}
}

void UIWindowMgr::DownButton()
{
	if (m_chooseWnd) {
		m_chooseWnd->SendMsg(NULL, UM_DOWNPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_sayDialogWnd) {
		m_sayDialogWnd->SendMsg(NULL, UM_DOWNPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_comboBoxWnd) {
		m_comboBoxWnd->SendMsg(NULL, UM_DOWNPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_chooseWarpWnd) {
		m_chooseWarpWnd->SendMsg(NULL, UM_DOWNPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_metalListWnd) {
		((UIWindow*)m_metalListWnd)->SendMsg(NULL, UM_DOWNPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_editWindow) {
		m_editWindow->SendMsg(NULL, UM_DOWNPUSH, 0, 0, 0, 0);
		return;
	}
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		if ((*i)->IsFrameWnd())
			(*i)->SendMsg(NULL, UM_DOWNPUSH, 0, 0, 0, 0);
		++i;
	}
}

void UIWindowMgr::PageDownButton()
{
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		if ((*i)->IsFrameWnd())
			(*i)->SendMsg(NULL, UM_PAGEDOWNPUSH, 0, 0, 0, 0);
		++i;
	}
}

void UIWindowMgr::ToggleButton(int toggleDir)
{
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		if ((*i)->IsFrameWnd())
			(*i)->SendMsg(NULL, UM_TOGGLEPUSH, toggleDir, 0, 0, 0);
		++i;
	}
}

void UIWindowMgr::CancelPushButton()
{
	if (m_editWindow) {
		m_editWindow->SendMsg(NULL, UM_CANCELPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_modalWindow) {
		m_modalWindow->SendMsg(NULL, UM_CANCELPUSH, 0, 0, 0, 0);
		return;
	}
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		if ((*i)->IsFrameWnd())
			(*i)->SendMsg(NULL, UM_CANCELPUSH, 0, 0, 0, 0);
		++i;
	}
}

void UIWindowMgr::RefreshItem()
{
	if (m_itemWnd)
		m_itemWnd->SendMsg(NULL, UM_REFRESHITEM, 0, 0, 0, 0);
	if (m_shortCutWnd)
		m_shortCutWnd->SendMsg(NULL, UM_REFRESHITEM, 0, 0, 0, 0);
}

void UIWindowMgr::DefPushButton()
{
	if (m_modalWindow) {
		m_modalWindow->SendMsg(NULL, UM_DEFPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_quitWnd) {
		m_quitWnd->SendMsg(NULL, UM_DEFPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_npcEditDialogWnd) {
		m_npcEditDialogWnd->SendMsg(NULL, UM_DEFPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_npcTextEditDialogWnd) {
		m_npcTextEditDialogWnd->SendMsg(NULL, UM_DEFPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_chooseWnd) {
		m_chooseWnd->SendMsg(NULL, UM_DEFPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_sayDialogWnd) {
		m_sayDialogWnd->SendMsg(NULL, UM_DEFPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_comboBoxWnd) {
		m_comboBoxWnd->SendMsg(NULL, UM_DEFPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_chooseWarpWnd) {
		m_chooseWarpWnd->SendMsg(NULL, UM_DEFPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_metalListWnd) {
		((UIWindow*)m_metalListWnd)->SendMsg(NULL, UM_DEFPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_metalProcessWnd) {
		((UIWindow*)m_metalProcessWnd)->SendMsg(NULL, UM_DEFPUSH, 0, 0, 0, 0);
		return;
	}
	if (m_editWindow) {
		m_editWindow->SendMsg(NULL, UM_DEFPUSH, 0, 0, 0, 0);
		return;
	}
	std::list<UIWindow *>::iterator i = m_children.begin();
	while (i != m_children.end()) {
		if ((*i)->IsFrameWnd())
			(*i)->SendMsg(NULL, UM_DEFPUSH, 0, 0, 0, 0);
		++i;
	}
}

UIWindow* UIWindowMgr::HitTest(int x, int y)
{
	std::list<UIWindow *>::reverse_iterator i = m_children.rbegin();
	while (i != m_children.rend()) {
		UIWindow* result = (*i)->HitTest(x, y);
		if (result)
			if (result->m_show)
				return result;
		i++;
	}
	return NULL;
}

void UIWindowMgr::GetWindowPos(UIWindow* window, int* x, int* y)
{
	int sx = 0;
	int sy = 0;
	while (window) {
		sx += window->m_x;
		sy += window->m_y;
		window = window->m_parent;
	}
	*x = sx;
	*y = sy;
}

void UIWindowMgr::PostQuit(UIWindow* wnd)
{
	if (wnd == (UIWindow*)m_selectCharWnd)
		m_selectCharWnd = NULL;
	if (wnd == (UIWindow*)m_selCharForUServerWnd)
		m_selCharForUServerWnd = NULL;
	if (wnd == (UIWindow*)m_changeNameWnd)
		m_changeNameWnd = NULL;
	m_quitWindow.push_back(wnd);
}



int UIWindowMgr::DoModal(UIWindow *wnd)
{
	m_modalResult = 0;
	while (!g_sysQuit) {
		if (m_modalResult) {
			PostQuit(wnd);
			return m_modalResult;
		}

		if (g_modeMgr.GetCurMode())
			g_modeMgr.GetCurMode()->OnUpdate();
	}

	return ID_OK;
}

void UIWindowMgr::AddFriendCharName(const char* friendCharName)
{
	PACKET_CZ_ADD_FRIENDS p;
	p.PacketType = HEADER_CZ_ADD_FRIENDS;
	memcpy(p.name, friendCharName, 24);
	CRagConnection::instanceR().SendPacket(
		CRagConnection::instanceR().GetPacketSize(p.PacketType),
		(const char*)&p);
}






void UIWindowMgr::AddWhisperCharName(char const *whisperCharName)
{
	if (m_chatWnd) {
		std::vector<std::string> &hisChat = m_chatWnd->m_whisperChat->m_hisChat;

		std::vector<std::string>::iterator it = hisChat.begin();
		while (it != hisChat.end()) {
			if (*it == whisperCharName)
				return;

			it++;
		}

		hisChat.push_back(whisperCharName);
		if (m_chatWnd->m_whisperChat->m_curHis >= hisChat.size() - 1)
			m_chatWnd->m_whisperChat->m_curHis = hisChat.size();
	}
}

void UIWindowMgr::AddWhisperCharName2(char const *whisperCharName)
{
	int curHis = 0;
	if (m_chatWnd) {
		std::vector<std::string> &whisperHisChat = m_chatWnd->m_whisperChat->m_hisChat;

		std::vector<std::string>::iterator it = whisperHisChat.begin();
		while (it != whisperHisChat.end()) {
			if (*it == whisperCharName)
				goto found;

			it++;
			curHis++;
		}

		whisperHisChat.push_back(whisperCharName);

found:
		m_chatWnd->m_whisperChat->m_curHis = curHis;
		m_chatWnd->SendMsg(m_chatWnd->m_whisperChat, UM_COMBOBOX_SELECTED, curHis, 0, 0, 0);
	}
}

void UIWindowMgr::StoreChatWndStatus() {}

void UIWindowMgr::RestoreChatWndStatus() {}

int UIWindowMgr::ErrorMsg(char const *msg, int type, int isDefYes, int changeMsg, unsigned int autoReturnTime)
{
	Trace("windowMgr.ErrorMsg==%s", msg);
	if (m_modalWindow) {
		if (changeMsg)
			m_modalWindow->SendMsg(NULL, UM_CHANGEMSG, (INT_PTR)msg, 0, 0, 0);
		return ID_CANCEL;
	}

	UIMessageBox *messageBox = new UIMessageBox;
	messageBox->Create(msg, type, isDefYes, autoReturnTime);
	AddWindow(messageBox);
	this->ReleaseCapture();
	SetFocusEdit(NULL);
	m_modalWindow = messageBox;
	int result = DoModal(messageBox);

	m_modalWindow = NULL;
	FindFocusEditAndSetFocus();
	return result;
}

int UIWindowMgr::ErrorMsgAutoreturn(char const *msg, unsigned int autoReturnTime)
{
	if (m_modalWindow)
		return ID_CANCEL;

	UIMessageBoxAutoreturn *messageBox = new UIMessageBoxAutoreturn;
	messageBox->Create(msg, autoReturnTime);
	AddWindow(messageBox);
	this->ReleaseCapture();
	SetFocusEdit(NULL);
	m_modalWindow = messageBox;
	int result = DoModal(messageBox);

	m_modalWindow = NULL;
	FindFocusEditAndSetFocus();
	return result;
}

int UIWindowMgr::EmailAddress()
{
	if (m_modalWindow)
		return ID_CANCEL;

	UIEmailAddressWnd *emailWnd = new UIEmailAddressWnd;
	emailWnd->Create(280, 48);
	emailWnd->Move((g_renderer->m_width - 640) / 2 + 185, (g_renderer->m_height - 480) / 2 + 180);
	AddWindow(emailWnd);
	m_captureWindow = NULL;
	m_modalWindow = emailWnd;
	int result = DoModal(emailWnd);

	m_modalWindow = NULL;
	FindFocusEditAndSetFocus();
	return result;
}

UIWindow * UIWindowMgr::QueryWindow(WINDOWID windowId)
{
	switch (windowId)
	{
	case WID_LOGINWND:
		return (UIWindow *)m_loginWnd;
	case WID_CHATWND:
		return (UIWindow *)m_chatWnd;
	case WID_CHATROOMMAKEWND:
		return (UIWindow *)m_chatRoomMakeWnd;
	case WID_CHATROOMCHANGEWND:
		return (UIWindow *)m_chatRoomChangeWnd;
	case WID_CHATROOMWND:
		return (UIWindow *)m_chatRoomWnd;
	case WID_MESSENGERGROUPWND:
		return (UIWindow *)m_messengerGroupWnd;
	case WID_OPTIONWND:
		return (UIWindow *)m_optionWnd;
	case WID_SKILLLISTWND:
		return (UIWindow *)m_skillListWnd;
	case WID_QUESTWND:
		return (UIWindow *)m_questWnd;
	case WID_QUEST_DETAIL_WND:
		return (UIWindow *)m_questDetailWnd;
	case WID_GUILDMEMBERMANAGEWND:
		return (UIWindow *)m_guildMemberManageWnd;
	case WID_GUILDINFOMANAGEWND:
		return (UIWindow *)m_guildInfoManageWnd;
	case WID_PETINFOWND:
		return (UIWindow *)m_petInfoWnd;
	case WID_GUILDSKILLWND:
		return (UIWindow *)m_guildSkillWnd;
	case WID_PROHIBITLISTWND:
		return (UIWindow *)m_prohibitListWnd;
	case WID_HOMUNINFOWND:
		return (UIWindow *)m_homunInfoWnd;
	case WID_HOMUNSKILLLISTWND:
		return (UIWindow *)m_homunSkillListWnd;
	case WID_MERINFOWND:
		return (UIWindow *)m_merInfoWnd;
	case WID_MERSKILLLISTWND:
		return (UIWindow *)m_merSkillListWnd;
	case WID_AUCTIONWND:
		return (UIWindow *)m_auctionWnd;
	default:
		return NULL;
	}
}

#include "rag_budget_fill.h"
bool UIWindowMgr::DeleteWindow(WINDOWID windowId)
{
	if (m_modalWindow)
		return false;
	switch (windowId) {
	case WID_CHATWND:
		DeleteWindow(WID_COMBOBOXWND);
		if (!m_chatWnd) return false;
		PostQuit(m_chatWnd);
		m_chatWnd = NULL;
		m_chatWndShow = 0;
		return true;
	case WID_ITEMWND:
		if (!m_itemWnd) return false;
		PostQuit(m_itemWnd);
		m_itemWnd = NULL;
		itemWndInfo.show = 0;
		return true;
	case WID_QUESTWND:
		if (!m_questWnd) return false;
		PostQuit(m_questWnd);
		m_questWnd = NULL;
		questWndInfo.show = 0;
		return true;
	case WID_EQUIPWND:
		if (!m_equipWnd) return false;
		PostQuit(m_equipWnd);
		m_equipWnd = NULL;
		equipWndInfo.show = 0;
		return true;
	case WID_STATUSWND:
		if (!m_statusWnd) return false;
		PostQuit(m_statusWnd);
		m_statusWnd = NULL;
		statusWndInfo.show = 0;
		return true;
	case WID_ITEMDROPCNTWND:
		if (!m_itemDropCntWnd) return false;
		PostQuit(m_itemDropCntWnd);
		m_itemDropCntWnd = NULL;
		return true;
	case WID_SAYDIALOGWND:
		if (!m_sayDialogWnd) return false;
		PostQuit(m_sayDialogWnd);
		m_sayDialogWnd = NULL;
		return true;
	case WID_CHATROOMWND:
		if (!m_chatRoomWnd) return false;
		PostQuit(m_chatRoomWnd);
		m_chatRoomWnd = NULL;
		return true;
	case WID_CHOOSEWARPWND:
		if (!m_chooseWarpWnd) return false;
		PostQuit(m_chooseWarpWnd);
		m_chooseWarpWnd = NULL;
		return true;
	case WID_CHOOSEWND:
		if (!m_chooseWnd) return false;
		PostQuit(m_chooseWnd);
		m_chooseWnd = NULL;
		return true;
	case WID_ITEMCOMPOSITIONWND:
		if (!m_itemCompositionWnd) return false;
		PostQuit(m_itemCompositionWnd);
		m_itemCompositionWnd = NULL;
		return true;
	case WID_ITEMIDENTIFYWND:
		if (!m_itemIdentifyWnd) return false;
		PostQuit(m_itemIdentifyWnd);
		m_itemIdentifyWnd = NULL;
		return true;
	case WID_COMBOBOXWND:
		if (!m_comboBoxWnd) return false;
		PostQuit(m_comboBoxWnd);
		m_comboBoxWnd = NULL;
		return true;
	case WID_MENUWND:
		if (!m_menu) return false;
		PostQuit(m_menu);
		m_menu = NULL;
		return true;
	case WID_OPTIONWND:
		if (!m_optionWnd) return false;
		PostQuit(m_optionWnd);
		m_optionWnd = NULL;
		optionWndInfo.show = 0;
		return true;
	case WID_CHATROOMCHANGEWND:
		if (!m_chatRoomChangeWnd) return false;
		PostQuit(m_chatRoomChangeWnd);
		m_chatRoomChangeWnd = NULL;
		return true;
	case WID_CHATROOMMAKEWND:
		if (!m_chatRoomMakeWnd) return false;
		PostQuit(m_chatRoomMakeWnd);
		m_chatRoomMakeWnd = NULL;
		return true;
	case WID_CHOOSESELLBUYWND:
		if (!m_chooseSellBuyWnd) return false;
		PostQuit(m_chooseSellBuyWnd);
		m_chooseSellBuyWnd = NULL;
		return true;
	case WID_ITEMSELLWND:
		if (!m_itemSellWnd) return false;
		PostQuit(m_itemSellWnd);
		m_itemSellWnd = NULL;
		return true;
	case WID_MERCHANTITEMPURCHASEWND:
		if (!m_merchantItemPurchaseWnd) return false;
		PostQuit(m_merchantItemPurchaseWnd);
		m_merchantItemPurchaseWnd = NULL;
		return true;
	case WID_ITEMPURCHASEWND:
		if (!m_itemPurchaseWnd) return false;
		PostQuit(m_itemPurchaseWnd);
		m_itemPurchaseWnd = NULL;
		return true;
	case WID_MERCHANTITEMMYSHOPWND:
		if (!m_merchantItemMyShopWnd) return false;
		PostQuit(m_merchantItemMyShopWnd);
		m_merchantItemMyShopWnd = NULL;
		return true;
	case WID_MERCHANTITEMSHOPWND:
		if (!m_merchantItemShopWnd) return false;
		PostQuit(m_merchantItemShopWnd);
		m_merchantItemShopWnd = NULL;
		return true;
	case WID_ITEMSHOPWND:
		if (!m_itemShopWnd) return false;
		PostQuit(m_itemShopWnd);
		m_itemShopWnd = NULL;
		return true;
	case WID_NOTIFYJOBLEVELUPWND:
		if (!m_notifyJobLevelUpWnd) return false;
		PostQuit(m_notifyJobLevelUpWnd);
		m_notifyJobLevelUpWnd = NULL;
		return true;
	case WID_NOTIFYLEVELUPWND:
		if (!m_notifyLevelUpWnd) return false;
		PostQuit(m_notifyLevelUpWnd);
		m_notifyLevelUpWnd = NULL;
		return true;
	case WID_NOTIFYQUESTWND:
		if (!m_notifyQuestWnd) return false;
		PostQuit(m_notifyQuestWnd);
		m_notifyQuestWnd = NULL;
		return true;
	case WID_CARDITEMILLUSTWND:
		if (!m_cardItemIllustWnd) return false;
		PostQuit(m_cardItemIllustWnd);
		m_cardItemIllustWnd = NULL;
		return true;
	case WID_RESTARTWND:
		if (!m_restartWnd) return false;
		PostQuit(m_restartWnd);
		m_restartWnd = NULL;
		return true;
	case WID_QUITWND:
		if (!m_quitWnd) return false;
		PostQuit(m_quitWnd);
		m_quitWnd = NULL;
		return true;
	case WID_SKILLDESCRIBEWND:
		if (!m_skillDescribeWnd) return false;
		PostQuit(m_skillDescribeWnd);
		m_skillDescribeWnd = NULL;
		return true;
	case WID_QUEST_DETAIL_WND:
		if (!m_questDetailWnd) return false;
		PostQuit(m_questDetailWnd);
		m_questDetailWnd = NULL;
		return true;
	case WID_ITEMPARAMCHANGEDISPLAYWND:
		if (!m_itemParamChangeDisplayWnd) return false;
		PostQuit(m_itemParamChangeDisplayWnd);
		m_itemParamChangeDisplayWnd = NULL;
		return true;
	case WID_COMBINEDCARDITEMCOLLECTIONWND:
		if (!m_combinedCardItemCollectionWnd) return false;
		PostQuit(m_combinedCardItemCollectionWnd);
		m_combinedCardItemCollectionWnd = NULL;
		return true;
	case WID_ITEMCOLLECTIONWND:
		if (!m_itemCollectionWnd) return false;
		PostQuit(m_itemCollectionWnd);
		m_itemCollectionWnd = NULL;
		return true;
	case WID_MINIMAPZOOMWND:
		if (!m_minimapZoomWnd) return false;
		PostQuit(m_minimapZoomWnd);
		m_minimapZoomWnd = NULL;
		return true;
	case WID_SKILLLISTWND:
		if (!m_skillListWnd) return false;
		PostQuit(m_skillListWnd);
		m_skillListWnd = NULL;
		skillListWndInfo.show = 0;
		return true;
	case WID_ITEMSTOREWND:
		if (!m_itemStoreWnd) return false;
		PostQuit(m_itemStoreWnd);
		m_itemStoreWnd = NULL;
		return true;
	case WID_MESSENGERWND:
		if (m_messengerGroupWnd)
			((UIWindow*)m_messengerGroupWnd)->StoreInfo();
		if (DeleteWindow(WID_MESSENGERGROUPWND))
			return true;
		break;
	case WID_MESSENGERGROUPWND:
		if (!m_messengerGroupWnd) return false;
		PostQuit(m_messengerGroupWnd);
		m_messengerGroupWnd = NULL;
		messengerGroupWndInfo.show = 0;
		return true;
	case WID_MERCHANTSHOPMAKEWND:
		DeleteWindow(WID_MERCHANTMIRRORITEMWND);
		if (!m_merchantShopMakeWnd) return false;
		PostQuit(m_merchantShopMakeWnd);
		m_merchantShopMakeWnd = NULL;
		return true;
	case WID_MERCHANTMIRRORITEMWND:
		if (!m_merchantMirrorItemWnd) return false;
		PostQuit(m_merchantMirrorItemWnd);
		m_merchantMirrorItemWnd = NULL;
		return true;
	case WID_MERCHANTITEMWND:
		if (!m_merchantItemWnd) return false;
		PostQuit(m_merchantItemWnd);
		m_merchantItemWnd = NULL;
		merchantItemWndInfo.show = 0;
		return true;
	case WID_CANDIDATEWND:
		if (!m_candidateWnd) return false;
		PostQuit(m_candidateWnd);
		m_candidateWnd = NULL;
		return true;
	case WID_COMPOSITIONWND:
		if (!m_compositionWnd) return false;
		PostQuit(m_compositionWnd);
		m_compositionWnd = NULL;
		return true;
	case WID_DETAILLEVELWND:
		if (!m_detailLevelWnd) return false;
		PostQuit(m_detailLevelWnd);
		m_detailLevelWnd = NULL;
		return true;
	case WID_NOTIFYITEMOBTAINWND:
		if (!m_notifyItemObtainWnd) return false;
		PostQuit(m_notifyItemObtainWnd);
		m_notifyItemObtainWnd = NULL;
		return true;
	case WID_NPCEDITDIALOGWND:
		if (!m_npcEditDialogWnd) return false;
		PostQuit(m_npcEditDialogWnd);
		m_npcEditDialogWnd = NULL;
		return true;
	case WID_NPCTEXTEDITDIALOGWND:
		if (!m_npcTextEditDialogWnd) return false;
		PostQuit(m_npcTextEditDialogWnd);
		m_npcTextEditDialogWnd = NULL;
		return true;
	case WID_SKILLNAMECHANGEWND:
		if (!m_skillNameChangeWnd) return false;
		PostQuit(m_skillNameChangeWnd);
		m_skillNameChangeWnd = NULL;
		return true;
	case WID_GUILDWND:
		{
			int allFalse = 1;
			if (g_windowMgr.DeleteWindow(WID_GUILDINFOMANAGEWND)) allFalse = 0;
			if (g_windowMgr.DeleteWindow(WID_GUILDMEMBERMANAGEWND)) allFalse = 0;
			if (g_windowMgr.DeleteWindow(WID_GUILDPOSITIONMANAGEWND)) allFalse = 0;
			if (g_windowMgr.DeleteWindow(WID_GUILDSKILLWND)) allFalse = 0;
			if (g_windowMgr.DeleteWindow(WID_GUILDBANISHEDMEMBERWND)) allFalse = 0;
			if (g_windowMgr.DeleteWindow(WID_GUILDNOTICEWND)) allFalse = 0;
			if (!g_windowMgr.DeleteWindow(WID_GUILDTOTALINFOWND) && allFalse) return false;
			return true;
		}
	case WID_GUILDMEMBERMANAGEWND:
		if (!m_guildMemberManageWnd) return false;
		m_guildMemberManageWnd->StoreInfo();
		PostQuit(m_guildMemberManageWnd);
		m_guildMemberManageWnd = NULL;
		return true;
	case WID_GUILDPOSITIONMANAGEWND:
		if (!m_guildPositionManageWnd) return false;
		m_guildPositionManageWnd->StoreInfo();
		PostQuit(m_guildPositionManageWnd);
		m_guildPositionManageWnd = NULL;
		return true;
	case WID_GUILDSKILLWND:
		if (!m_guildSkillWnd) return false;
		m_guildSkillWnd->StoreInfo();
		PostQuit(m_guildSkillWnd);
		m_guildSkillWnd = NULL;
		return true;
	case WID_GUILDBANISHEDMEMBERWND:
		if (!m_guildBanishedMemberWnd) return false;
		m_guildBanishedMemberWnd->StoreInfo();
		PostQuit(m_guildBanishedMemberWnd);
		m_guildBanishedMemberWnd = NULL;
		return true;
	case WID_GUILDNOTICEWND:
		if (!m_guildNoticeWnd) return false;
		m_guildNoticeWnd->StoreInfo();
		PostQuit(m_guildNoticeWnd);
		m_guildNoticeWnd = NULL;
		return true;
	case WID_GUILDTOTALINFOWND:
		if (!m_guildTotalInfoWnd) return false;
		m_guildTotalInfoWnd->StoreInfo();
		PostQuit(m_guildTotalInfoWnd);
		m_guildTotalInfoWnd = NULL;
		return true;
	case WID_GUILDINFOMANAGEWND:
		if (!m_guildInfoManageWnd) return false;
		m_guildInfoManageWnd->StoreInfo();
		PostQuit(m_guildInfoManageWnd);
		m_guildInfoManageWnd = NULL;
		return true;
	case WID_ILLUSTWND:
		if (!m_illustWnd) return false;
		PostQuit(m_illustWnd);
		m_illustWnd = NULL;
		return true;
	case WID_GUILDBANREASONDESCWND:
		if (!m_guildBanReasonDescWnd) return false;
		PostQuit(m_guildBanReasonDescWnd);
		m_guildBanReasonDescWnd = NULL;
		return true;
	case WID_GUILDLEAVEREASONDESCWND:
		if (!m_guildLeaveReasonDescWnd) return false;
		PostQuit(m_guildLeaveReasonDescWnd);
		m_guildLeaveReasonDescWnd = NULL;
		return true;
	case WID_FRIENDOPTIONWND:
		if (!m_friendOptionWnd) return false;
		PostQuit(m_friendOptionWnd);
		m_friendOptionWnd = NULL;
		return true;
	case WID_PARTYSETTINGWND:
		if (!m_partySettingWnd) return false;
		PostQuit(m_partySettingWnd);
		m_partySettingWnd = NULL;
		return true;
	case WID_TIPOFTHEDAYWND:
		if (!m_tipOfTheDayWnd) return false;
		PostQuit(m_tipOfTheDayWnd);
		m_tipOfTheDayWnd = NULL;
		return true;
	case WID_JOINGUILDACCEPTWND:
		if (!m_joinGuildAcceptWnd) return false;
		PostQuit(m_joinGuildAcceptWnd);
		m_joinGuildAcceptWnd = NULL;
		return true;
	case WID_ALLYGUILDACCEPTWND:
		if (!m_allyGuildAcceptWnd) return false;
		PostQuit(m_allyGuildAcceptWnd);
		m_allyGuildAcceptWnd = NULL;
		return true;
	case WID_JOINPARTYACCEPTWND:
		if (!m_joinPartyAcceptWnd) return false;
		PostQuit(m_joinPartyAcceptWnd);
		m_joinPartyAcceptWnd = NULL;
		return true;
	case WID_COUPLEACCEPTWND:
		if (!m_CoupleAcceptWnd) return false;
		PostQuit(m_CoupleAcceptWnd);
		m_CoupleAcceptWnd = NULL;
		return true;
	case WID_STARPLACE_ACCEPTWND:
		if (!m_StarPlaceAcceptWnd) return false;
		PostQuit(m_StarPlaceAcceptWnd);
		m_StarPlaceAcceptWnd = NULL;
		return true;
	case WID_YOURITEMWND:
		if (!m_YourItemWnd) return false;
		PostQuit(m_YourItemWnd);
		m_YourItemWnd = NULL;
		return true;
	case WID_BABYACCEPTWND:
		if (!m_BabyAcceptWnd) return false;
		PostQuit(m_BabyAcceptWnd);
		m_BabyAcceptWnd = NULL;
		return true;
	case WID_BABYACCEPTWND2:
		if (!m_BabyAcceptWnd2) return false;
		PostQuit(m_BabyAcceptWnd2);
		m_BabyAcceptWnd2 = NULL;
		return true;
	case WID_EXCHANGEACCEPTWND:
		if (!m_exchangeAcceptWnd) return false;
		PostQuit(m_exchangeAcceptWnd);
		m_exchangeAcceptWnd = NULL;
		return true;
	case WID_EXCHANGEWND:
		if (!m_exchangeWnd) return false;
		PostQuit(m_exchangeWnd);
		m_exchangeWnd = NULL;
		return true;
	case WID_PASSWORDWND:
		if (!m_passwordWnd) return false;
		PostQuit(m_passwordWnd);
		m_passwordWnd = NULL;
		return true;
	case WID_MAKETARGETLISTWND:
		if (!m_metalListWnd) return false;
		PostQuit(m_metalListWnd);
		m_metalListWnd = NULL;
		return false;
	case WID_ITEMREPAIRWND:
		if (!m_itemRepairWnd) return false;
		PostQuit(m_itemRepairWnd);
		m_itemRepairWnd = NULL;
		return false;
	case WID_WEAPONREFINEWND:
		if (!m_weaponRefineWnd) return false;
		PostQuit(m_weaponRefineWnd);
		m_weaponRefineWnd = NULL;
		return false;
	case WID_MAKETARGETPROCESSWND:
		if (!m_metalProcessWnd) return false;
		PostQuit(m_metalProcessWnd);
		m_metalProcessWnd = NULL;
		return false;
	case WID_MAKETARGETRESULTWND:
		if (!m_metalResultWnd) return false;
		PostQuit(m_metalResultWnd);
		m_metalResultWnd = NULL;
		return false;
	case WID_KEYSTROKEWND:
		if (!m_keyStrokeWnd) return false;
		PostQuit(m_keyStrokeWnd);
		m_keyStrokeWnd = NULL;
		return false;
	case WID_EMOTIONWND:
		if (!m_emotionWnd) return false;
		PostQuit(m_emotionWnd);
		m_emotionWnd = NULL;
		return true;
	case WID_EMOTIONLISTWND:
		if (!m_emotionListWnd) return false;
		PostQuit(m_emotionListWnd);
		m_emotionListWnd = NULL;
		return true;
	case WID_PETINFOWND:
		if (!m_petInfoWnd) return false;
		PostQuit(m_petInfoWnd);
		m_petInfoWnd = NULL;
		return true;
	case WID_SELECTPETEGGWND:
		if (!m_selectPetEggWnd) return false;
		PostQuit(m_selectPetEggWnd);
		m_selectPetEggWnd = NULL;
		return true;
	case WID_PETTAMINGDECEIVEWND:
		if (!m_petTamingDeceiveWnd) return false;
		PostQuit(m_petTamingDeceiveWnd);
		m_petTamingDeceiveWnd = NULL;
		return true;
	case WID_MAKINGARROWLISTWND:
		if (!m_makingArrowListWnd) return false;
		PostQuit(m_makingArrowListWnd);
		m_makingArrowListWnd = NULL;
		return true;
	case WID_SELECTCARTWND:
		if (!m_selectCartWnd) return false;
		PostQuit(m_selectCartWnd);
		m_selectCartWnd = NULL;
		return true;
	case WID_SPELLLISTWND:
		if (!m_spellListWnd) return false;
		PostQuit(m_spellListWnd);
		m_spellListWnd = NULL;
		return true;
	case WID_PROHIBITLISTWND:
		if (!m_prohibitListWnd) return false;
		PostQuit(m_prohibitListWnd);
		m_prohibitListWnd = NULL;
		return true;
	case WID_BOOKWND:
		if (!m_bookWnd) return false;
		PostQuit(m_bookWnd);
		m_bookWnd = NULL;
		return true;
	case WID_JOINFRIENDACCEPTWND:
		if (!m_joinFriendAcceptWnd) return false;
		PostQuit(m_joinFriendAcceptWnd);
		m_joinFriendAcceptWnd = NULL;
		return true;
	case WID_COMMANDLISTWND:
		if (!m_commandListWnd) return false;
		PostQuit(m_commandListWnd);
		m_commandListWnd = NULL;
		return true;
	case WID_HOMUNINFOWND:
		if (!m_homunInfoWnd) return false;
		PostQuit(m_homunInfoWnd);
		m_homunInfoWnd = NULL;
		return true;
	case WID_HOMUNSKILLLISTWND:
		if (!m_homunSkillListWnd) return false;
		PostQuit(m_homunSkillListWnd);
		m_homunSkillListWnd = NULL;
		homunSkillListWndInfo.show = 0;
		return true;
	case WID_MERINFOWND:
		if (!m_merInfoWnd) return false;
		PostQuit(m_merInfoWnd);
		m_merInfoWnd = NULL;
		return true;
	case WID_MERSKILLLISTWND:
		if (!m_merSkillListWnd) return false;
		PostQuit(m_merSkillListWnd);
		m_merSkillListWnd = NULL;
		merSkillListWndInfo.show = 0;
		return true;
	case WID_AUTOMESSAGEWND:
		if (!m_autoMessageWnd) return false;
		PostQuit(m_autoMessageWnd);
		m_autoMessageWnd = NULL;
		return true;
	case WID_MAILLISTWND:
		if (!m_mailListWnd) return false;
		PostQuit(m_mailListWnd);
		m_mailListWnd = NULL;
		return true;
	case WID_MAILVIEWWND:
		if (!m_mailViewWnd) return false;
		PostQuit(m_mailViewWnd);
		m_mailViewWnd = NULL;
		return true;
	case WID_AUCTIONWND:
		if (!m_auctionWnd) return false;
		PostQuit(m_auctionWnd);
		m_auctionWnd = NULL;
		return true;
	case WID_AUCTIONMSGWND:
		if (!m_auctionMsgWnd) return false;
		PostQuit(m_auctionMsgWnd);
		m_auctionMsgWnd = NULL;
		return true;
	case WID_IMGMENUWND:
		if (!m_imgMenuWnd) return false;
		PostQuit(m_imgMenuWnd);
		m_imgMenuWnd = NULL;
		return true;
	case WID_CHANGENAMEWND:
		RAG_BUDGET_FILL(8)
		if (!m_changeNameWnd) return false;
		PostQuit(m_changeNameWnd);
		m_changeNameWnd = NULL;
		return true;
	case WID_REBIRTHWND:
		if (!m_rebirthWnd) return false;
		PostQuit(m_rebirthWnd);
		m_rebirthWnd = NULL;
		return true;
	case WID_SUBCHATWND_ST:
		if (!m_subChatWndST) return false;
		m_subChatWndST->StoreInfo();
		PostQuit(m_subChatWndST);
		m_subChatWndST = NULL;
		return true;
	case WID_SUBCHATWND_BT:
		if (!m_subChatWndBT) return false;
		m_subChatWndBT->StoreInfo();
		PostQuit(m_subChatWndBT);
		m_subChatWndBT = NULL;
		return true;
	case WID_BATTLEMSG_OPTIONWND:
		if (!m_battleMsgOpionWnd) return false;
		PostQuit(m_battleMsgOpionWnd);
		m_battleMsgOpionWnd = NULL;
		return true;
	case WID_SUBCHAT_MINIWND:
		if (!m_subChatMiniWnd) return false;
		PostQuit(m_subChatMiniWnd);
		m_subChatMiniWnd = NULL;
		return true;
	case WID_MEMORIAL_DUN_WND:
		if (!m_memorialDunWnd) return false;
		m_memorialDunWnd->InitInfo();
		PostQuit(m_memorialDunWnd);
		m_memorialDunWnd = NULL;
		g_session.m_onDunInfoWnd = 0;
		return true;
	case WID_OTHERUSER_EQUIPWND:
		if (!m_otherUserEquipWnd) return false;
		PostQuit(m_otherUserEquipWnd);
		m_otherUserEquipWnd = NULL;
		return true;
	case WID_ROMAPWND:
		if (!m_roMapWnd) return false;
		PostQuit(m_roMapWnd);
		m_roMapWnd = NULL;
		return true;
	default:
		break;
	}
	return false;
}


bool UIWindowMgr::ShowWindow(WINDOWID windowId, int show)
{
	switch (windowId) {
	case WID_CHATWND:
		m_chatWndShow = show;
		if (m_chatWnd == NULL)
			g_windowMgr.MakeWindow(WID_CHATWND);

		m_chatWnd->SetShow(show);
		if (show)
			g_windowMgr.MakeTopLayer((UIWindow *)m_chatWnd);
		else
			m_chatWnd->StoreInfo();
		return true;

	case WID_ITEMDROPCNTWND:
		g_windowMgr.MakeWindow(WID_ITEMDROPCNTWND);
		if (m_itemDropCntWnd->IsShow() != show)
			m_itemDropCntWnd->SetShow(show);

		if (show)
			g_windowMgr.MakeTopLayer((UIWindow *)m_itemDropCntWnd);
		break;

	case WID_SHORTCUTWND:
		shortCutWndInfo.show = show;
		g_windowMgr.MakeWindow(WID_SHORTCUTWND);
		m_shortCutWnd->SetShow(show);
		if (show)
			g_windowMgr.MakeTopLayer((UIWindow *)m_shortCutWnd);
		return true;
	}

	return false;
}


#if defined(BF_PORT)
// The control socket names a window by its WID (bf_client_state.cpp): every
// window made here is recorded with the id it was made for.
extern "C" void bf_client_window_made(UIWindow *wnd, int windowId);
// The control socket's `chat` event (bf_client_game.cpp): every line pushed
// into the chat history.
extern "C" void bf_client_chat_line(int second, const char *text, int type, const char *talker);

UIFrameWnd * UIWindowMgr::MakeWindow(WINDOWID windowId)
{
	UIFrameWnd *wnd = BfMakeWindow(windowId);
	if (wnd)
		bf_client_window_made(wnd, windowId);
	return wnd;
}

UIFrameWnd * UIWindowMgr::BfMakeWindow(WINDOWID windowId)
#else
UIFrameWnd * UIWindowMgr::MakeWindow(WINDOWID windowId)
#endif
{
	UIFrameWnd *wnd = NULL;
	UIFrameWnd *window;
	int width;
	int height;

	switch (windowId) {
	case WID_BASICINFOWND:
		if (!(wnd = m_basicInfoWnd)) {
			wnd = m_basicInfoWnd = new UIBasicInfoWnd;
			wnd->Create(280, 34);
			wnd->Move(0, 0);
			AddWindow(wnd);
			m_basicInfoWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&basicInfoWndInfo, 0, 0, 0);
			g_snapMgr.AddSnapMasterObject(wnd);
		}
		wnd->SetShow(1);
		if (itemWndInfo.show)
			MakeWindow(WID_ITEMWND);
		if (equipWndInfo.show)
			MakeWindow(WID_EQUIPWND);
		if (statusWndInfo.show)
			MakeWindow(WID_STATUSWND);
		if (optionWndInfo.show)
			MakeWindow(WID_OPTIONWND);
		if (skillListWndInfo.show)
			MakeWindow(WID_SKILLLISTWND);
		if (questWndInfo.show)
			DeleteWindow(WID_QUESTWND);
		// Restored from the reference (MakeWindow 0x4EFB20, at 0x4EFC74): on an
		// outdoor map (CGameMode +0x270, m_canRotateView) the minimap opens
		// with the basic info window.
		if (g_modeMgr.GetGameMode()->CanRotateView())
			MakeWindow(WID_MINIMAPZOOMWND);
		this->ShowWindow(WID_CHATWND, m_chatWndShow);
		this->ShowWindow(WID_SHORTCUTWND, shortCutWndInfo.show);
		this->ShowWindow(WID_ITEMDROPCNTWND, 0);
		if (messengerGroupWndInfo.show)
			MakeWindow(WID_MESSENGERWND);
		if (g_session.m_isLowSystemMemory && g_session.m_bAutoOpenDetailWindowIfLowMemory) {
			if (g_registry.m_spriteMode == 2 && g_registry.m_textureMode == 2)
				MakeWindow(WID_DETAILLEVELWND);
		}
		break;
	case WID_CHATWND:
		if (!(wnd = m_chatWnd)) {
			wnd = m_chatWnd = new UINewChatWnd;
			wnd->Create(600, m_chatWndHeight);
			AddWindow(wnd);
		}
		m_chatWnd->SendMsg(NULL, UM_SETINFO, 0, 0, 0, 0);
		break;
	case WID_SELECTSERVERWND:
		wnd = new UISelectServerWnd;
		wnd->Create(280, 200);
		wnd->Move(UIX(185), UICY(300) - 80);
		AddWindow(wnd);
		break;
	case WID_LOGINWND:
		wnd = new UILoginWnd;
		m_loginWnd = (UILoginWnd *)wnd;
		wnd->Create(280, 120);
		wnd->Move(UIX(185), UICY(300));
		AddWindow(wnd);
		break;
	case WID_NOTICEWND:
		wnd = new UINoticeWnd;
		wnd->Create(600, 400);
		wnd->Move(UIX(33), UIY(40));
		AddWindow(wnd);
		break;
	case WID_MAKECHARWND:
		wnd = new UIMakeCharWnd;
		wnd->Create(576, 342);
		wnd->Move(UIX(33), UIY(65));
		AddWindow(wnd);
		break;
	case WID_SELECTCHARWND:
		wnd = new UISelectCharWnd;
		m_selectCharWnd = (UISelectCharWnd *)wnd;
		wnd->Create(576, 342);
		wnd->Move(UIX(33), UIY(65));
		AddWindow(wnd);
		stateId.erase(stateId.begin(), stateId.end());
		break;
	case WID_ITEMWND:
		if (!(wnd = (UIFrameWnd *)m_itemWnd)) {
			wnd = m_itemWnd = new UIItemWnd;
			wnd->Create(280, 120);
			wnd->Move(0, 120);
			AddWindow(wnd);
			m_itemWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&itemWndInfo, 0, 0, 0);
			g_snapMgr.AddSnapObject(wnd);
		}
		wnd->SetShow(1);
		itemWndInfo.show = 1;
		break;
	case WID_QUESTWND:
		if (!(wnd = m_questWnd)) {
			wnd = m_questWnd = new UIQuestWnd;
			wnd->Create(280, 220);
			wnd->Move(0, 120);
			AddWindow(wnd);
			m_questWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&questWndInfo, 0, 0, 0);
			g_snapMgr.AddSnapObject(wnd);
		}
		wnd->SetShow(1);
		DeleteWindow(WID_NOTIFYQUESTWND);
		questWndInfo.show = 1;
		break;
	case WID_WAITWND:
		wnd = new UIWaitWnd;
		wnd->Create(280, 120);
		wnd->Move(UIX(185), UICY(300));
		AddWindow(wnd);
		break;
	case WID_LOADINGWND:
		if (!(wnd = (UIFrameWnd *)m_loadingWnd)) {
			wnd = m_loadingWnd = new UILoadingWnd;
			wnd->Create(240, 15);
			wnd->Move(g_renderer->m_width / 2 - 100, g_renderer->m_height / 2 + 160);
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_TOOLTIPWND:
		wnd = new UIToolTipWnd;
		wnd->Create(200, 120);
		wnd->Move(UIX(185), UICY(300));
		AddWindow(wnd);
		break;
	case WID_SHORTCUTWND:
		wnd = m_shortCutWnd;
		if (wnd) break;
		wnd = new UIShortCutWnd;
		m_shortCutWnd = (UIShortCutWnd *)wnd;
		wnd->Create(280, 34);
		wnd->Move(UIX(200), UICY(200));
		AddWindow(wnd);
		m_shortCutWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&shortCutWndInfo, 0, 0, 0);
		g_snapMgr.AddSnapObject(wnd);
		break;
	case WID_EQUIPWND:
		if (!(wnd = m_equipWnd)) {
			wnd = m_equipWnd = new UIEquipWnd;
			m_equipWnd->m_typeWnd = 0;
			wnd->Create(280, 147);
			wnd->Move(UIX(185), UICY(300));
			AddWindow(wnd);
			m_equipWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&equipWndInfo, 0, 0, 0);
			g_snapMgr.AddSnapObject(wnd);
		}
		wnd->SetShow(1);
		equipWndInfo.show = 1;
		break;
	case WID_STATUSWND:
		if (!(wnd = m_statusWnd)) {
			wnd = m_statusWnd = new UIStatusWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(185), UICY(300));
			AddWindow(wnd);
			m_statusWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&statusWndInfo, 0, 0, 0);
			g_snapMgr.AddSnapObject(wnd);
		}
		wnd->SetShow(1);
		statusWndInfo.show = 1;
		DeleteWindow(WID_NOTIFYLEVELUPWND);
		break;
	case WID_SKILLLISTWND:
		if (!(wnd = m_skillListWnd)) {
			wnd = m_skillListWnd = new UISkillListWnd;
			m_skillListWnd->m_typeWnd = 0;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
			m_skillListWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&skillListWndInfo, 0, 0, 0);
		}
		m_skillListWnd->m_typeWnd = 0;
		wnd->SetShow(1);
		skillListWndInfo.show = 1;
		DeleteWindow(WID_NOTIFYJOBLEVELUPWND);
		break;
	case WID_ITEMSTOREWND:
		if (!(wnd = (UIFrameWnd *)m_itemStoreWnd)) {
			wnd = m_itemStoreWnd = new UIItemStoreWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
			m_itemStoreWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&itemStoreWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_MESSENGERWND:
		if (messengerGroupWndInfo.radio)
			MakeWindow(WID_MESSENGERGROUPWND)->SendMsg(NULL, UM_COMMAND, 0x95, 2, 0, 0);
		else
			MakeWindow(WID_MESSENGERGROUPWND)->SendMsg(NULL, UM_COMMAND, 0x95, 1, 0, 0);
		break;
	case WID_MESSENGERGROUPWND:
		DeleteWindow(WID_MESSENGERWND);
		if (!(wnd = (UIFrameWnd *)m_messengerGroupWnd)) {
			wnd = m_messengerGroupWnd = new UIMessengerGroupWnd;
			wnd->Create(140, 155);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
			m_messengerGroupWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&messengerGroupWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		messengerGroupWndInfo.show = 1;
		break;
	case WID_MERCHANTSHOPMAKEWND:
		if (!(wnd = m_merchantShopMakeWnd)) {
			wnd = m_merchantShopMakeWnd = new UIMerchantShopMakeWnd;
			wnd->Create(280, 220);
			wnd->Move(UIX(185), UICY(200));
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&merchantShopMakeWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		MakeWindow(WID_MERCHANTMIRRORITEMWND);
		break;
	case WID_MERCHANTMIRRORITEMWND:
		if (!(wnd = (UIFrameWnd *)m_merchantMirrorItemWnd)) {
			wnd = m_merchantMirrorItemWnd = new UIMerchantMirrorItemWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(0), UICY(120));
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&merchantMirrorItemWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_MERCHANTITEMWND:
		if (m_mailListWnd || m_mailViewWnd) {
			g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_CLOSE_MAILWND), 0xff, 0, 0);
		} else {
			if (!(wnd = (UIFrameWnd *)m_merchantItemWnd)) {
				wnd = m_merchantItemWnd = new UIMerchantItemWnd;
				wnd->Create(280, 120);
				wnd->Move(UIX(200), UICY(120));
				AddWindow(wnd);
				wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&merchantItemWndInfo, 0, 0, 0);
			}
			wnd->SetShow(1);
			merchantItemWndInfo.show = 1;
		}
		break;
	case WID_DETAILLEVELWND:
		if (!(wnd = (UIFrameWnd *)m_detailLevelWnd)) {
			wnd = m_detailLevelWnd = new UIDetailLevelWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&detailLevelWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_NOTIFYITEMOBTAINWND:
		if (!(wnd = (UIFrameWnd *)m_notifyItemObtainWnd)) {
			wnd = m_notifyItemObtainWnd = new UINotifyItemObtainWnd;
			wnd->Create(180, 32);
			wnd->Move(UIX(220), 51);
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_NPCEDITDIALOGWND:
		if (!(wnd = (UIFrameWnd *)m_npcEditDialogWnd)) {
			wnd = m_npcEditDialogWnd = new UINpcEditDialogWnd;
			wnd->Create(176, 46);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_NPCTEXTEDITDIALOGWND:
		if (!(wnd = (UIFrameWnd *)m_npcTextEditDialogWnd)) {
			wnd = m_npcTextEditDialogWnd = new UINpcTextEditDialogWnd;
			wnd->Create(346, 46);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_SKILLNAMECHANGEWND:
		if (!(wnd = (UIFrameWnd *)m_skillNameChangeWnd)) {
			wnd = m_skillNameChangeWnd = new UISkillNameChangeWnd;
			wnd->Create(280, 48);
			wnd->Move(UIX(185), UIY(180));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_GUILDWND:
		if (!guildWndInfo.IsValidInfo())
			guildWndInfo.MakeDefault();
		switch (guildWndInfo.curTab) {
		case 0: wnd = g_windowMgr.MakeWindow(WID_GUILDINFOMANAGEWND); break;
		case 1: wnd = g_windowMgr.MakeWindow(WID_GUILDMEMBERMANAGEWND); break;
		case 2: wnd = g_windowMgr.MakeWindow(WID_GUILDPOSITIONMANAGEWND); break;
		case 3: wnd = g_windowMgr.MakeWindow(WID_GUILDSKILLWND); break;
		case 4: wnd = g_windowMgr.MakeWindow(WID_GUILDBANISHEDMEMBERWND); break;
		case 5: wnd = g_windowMgr.MakeWindow(WID_GUILDNOTICEWND); break;
		case 6: wnd = g_windowMgr.MakeWindow(WID_GUILDTOTALINFOWND); break;
		default: break;
		}
		break;
	case WID_GUILDMEMBERMANAGEWND:
		if (!(wnd = m_guildMemberManageWnd)) {
			wnd = m_guildMemberManageWnd = new UIGuildMemberManageWnd;
			width = guildWndWidth;
			height = guildWndHeight;
			wnd->Create(width, height);
			wnd->Move(0, 147);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&guildWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_GUILDPOSITIONMANAGEWND:
		if (!(wnd = m_guildPositionManageWnd)) {
			wnd = m_guildPositionManageWnd = new UIGuildPositionManageWnd;
			width = guildWndWidth;
			height = guildWndHeight;
			wnd->Create(width, height);
			wnd->Move(0, 147);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&guildWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_GUILDSKILLWND:
		if (!(wnd = m_guildSkillWnd)) {
			wnd = m_guildSkillWnd = new UIGuildSkillWnd;
			width = guildWndWidth;
			height = guildWndHeight;
			wnd->Create(width, height);
			wnd->Move(0, 147);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&guildWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_GUILDBANISHEDMEMBERWND:
		if (!(wnd = m_guildBanishedMemberWnd)) {
			wnd = m_guildBanishedMemberWnd = new UIGuildBanishedMemberWnd;
			width = guildWndWidth;
			height = guildWndHeight;
			wnd->Create(width, height);
			wnd->Move(0, 147);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&guildWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_GUILDNOTICEWND:
		if (!(wnd = m_guildNoticeWnd)) {
			wnd = m_guildNoticeWnd = new UIGuildNoticeWnd;
			width = guildWndWidth;
			height = guildWndHeight;
			wnd->Create(width, height);
			wnd->Move(0, 147);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&guildWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_GUILDTOTALINFOWND:
		if (!(wnd = m_guildTotalInfoWnd)) {
			wnd = m_guildTotalInfoWnd = new UIGuildTotalInfoWnd;
			width = guildWndWidth;
			height = guildWndHeight;
			wnd->Create(width, height);
			wnd->Move(0, 147);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&guildWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_GUILDINFOMANAGEWND:
		if (!(wnd = m_guildInfoManageWnd)) {
			wnd = m_guildInfoManageWnd = new UIGuildInfoManageWnd;
			width = guildWndWidth;
			height = guildWndHeight;
			wnd->Create(width, height);
			wnd->Move(0, 147);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&guildWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_GUILDLEAVEREASONDESCWND:
		if (!(wnd = (UIFrameWnd *)m_guildLeaveReasonDescWnd)) {
			wnd = m_guildLeaveReasonDescWnd = new UIGuildLeaveReasonDescWnd;
			wnd->Create(280, 80);
			wnd->Move(UIX(200), UICY(120));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_ILLUSTWND:
		if (!(wnd = m_illustWnd)) {
			wnd = m_illustWnd = new UIIllustWnd;
			wnd->Create(4, 4);
			wnd->Move(UIX(200), UICY(120));
			AddWindowFront(wnd);
		} else {
			wnd->SendMsg(NULL, UM_SETINFO, -1, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_GUILDBANREASONDESCWND:
		if (!(wnd = (UIFrameWnd *)m_guildBanReasonDescWnd)) {
			wnd = m_guildBanReasonDescWnd = new UIGuildBanReasonDescWnd;
			wnd->Create(280, 80);
			wnd->Move(UIX(200), UICY(120));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_FRIENDOPTIONWND:
		if (!(wnd = (UIFrameWnd *)m_friendOptionWnd)) {
			wnd = m_friendOptionWnd = new UIFriendOptionWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(120));
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&friendOptionWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_PARTYSETTINGWND:
		if (!(wnd = m_partySettingWnd)) {
			wnd = m_partySettingWnd = new UIPartySettingWnd;
			wnd->Create(280, g_session.GetNumParty() ? 245 : 180);
			wnd->Move(UIX(200), UICY(120));
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&partySettingWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_COMPOSITIONWND:
		if (!(wnd = (UIFrameWnd *)m_compositionWnd)) {
			wnd = m_compositionWnd = new UICompositionWnd;
			wnd->Create(34, 34);
			wnd->Move(0, 0);
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_CANDIDATEWND:
		if (!(wnd = m_candidateWnd)) {
			wnd = m_candidateWnd = new UICandidateWnd;

			width = 292;
			height = 34;
			if (m_conversionMode) {
				width = 120;
				height = 212;
			}
			wnd->Create(width, height);
			wnd->Move(g_renderer->m_width - width, g_renderer->m_height - height);
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_TIPOFTHEDAYWND:
		if (!(wnd = (UIFrameWnd *)m_tipOfTheDayWnd)) {
			wnd = m_tipOfTheDayWnd = new UITipOfTheDayWnd;
			wnd->Create(280, 180);
			wnd->Move(UIX(200), UICY(120));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_JOINGUILDACCEPTWND:
		if (!(wnd = (UIFrameWnd *)m_joinGuildAcceptWnd)) {
			wnd = m_joinGuildAcceptWnd = new UIJoinGuildAcceptWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_ALLYGUILDACCEPTWND:
		if (!(wnd = (UIFrameWnd *)m_allyGuildAcceptWnd)) {
			wnd = m_allyGuildAcceptWnd = new UIAllyGuildAcceptWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_JOINPARTYACCEPTWND:
		if (!(wnd = (UIFrameWnd *)m_joinPartyAcceptWnd)) {
			wnd = m_joinPartyAcceptWnd = new UIJoinPartyAcceptWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_COUPLEACCEPTWND:
		if (!(wnd = (UIFrameWnd *)m_CoupleAcceptWnd)) {
			wnd = m_CoupleAcceptWnd = new UICoupleAcceptWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_STARPLACE_ACCEPTWND:
		if (!(wnd = (UIFrameWnd *)m_StarPlaceAcceptWnd)) {
			wnd = m_StarPlaceAcceptWnd = new UIStarPlaceAcceptWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_YOURITEMWND:
		if (!(wnd = (UIFrameWnd *)m_YourItemWnd)) {
			wnd = m_YourItemWnd = new UIYourItemWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_BABYACCEPTWND:
		if (!(wnd = (UIFrameWnd *)m_BabyAcceptWnd)) {
			wnd = m_BabyAcceptWnd = new UIBabyAcceptWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_BABYACCEPTWND2:
		if (!(wnd = (UIFrameWnd *)m_BabyAcceptWnd2)) {
			wnd = m_BabyAcceptWnd2 = new UIBabyAcceptWnd2;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_EXCHANGEACCEPTWND:
		if (!(wnd = (UIFrameWnd *)m_exchangeAcceptWnd)) {
			wnd = m_exchangeAcceptWnd = new UIExchangeAcceptWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
			m_exchangeAcceptWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&exchangeAcceptWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_EXCHANGEWND:
		if (!(wnd = (UIFrameWnd *)m_exchangeWnd)) {
			wnd = m_exchangeWnd = new UIExchangeWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_PASSWORDWND:
		if (!(wnd = (UIFrameWnd *)m_passwordWnd)) {
			wnd = m_passwordWnd = new UIPasswordWnd;
			wnd->Create(176, 46);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_ITEMDROPCNTWND:
		if (!(wnd = m_itemDropCntWnd)) {
			wnd = m_itemDropCntWnd = new UIItemDropCntWnd;
			wnd->Create(182, 46);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
		}
		break;
	case WID_SAYDIALOGWND:
		if (!(wnd = m_sayDialogWnd)) {
			wnd = m_sayDialogWnd = new UISayDialogWnd;
			wnd->Create(280, 180);
			wnd->Move(UIX(200), UICY(100));
			AddWindow(wnd);
			m_sayDialogWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&sayDialogWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_CHATROOMWND:
		if (!(wnd = m_chatRoomWnd)) {
			wnd = m_chatRoomWnd = new UIChatRoomWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(300));
			AddWindow(wnd);
			m_chatRoomWnd->SendMsg(NULL, UM_SETINFO2, (INT_PTR)&chatRoomWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_CHOOSEWARPWND:
		{
			if (!(window = m_chooseWarpWnd)) {
				window = m_chooseWarpWnd = new UIChooseWarpWnd;
				window->Create(280, 120);
				wnd = window;
				wnd->Move(UIX(200), UICY(300));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_CHOOSEWND:
		if (!(wnd = (UIFrameWnd *)m_chooseWnd)) {
			wnd = m_chooseWnd = new UIChooseWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(300));
			AddWindow(wnd);
			m_chooseWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&chooseWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_ITEMCOMPOSITIONWND:
		if (!(wnd = (UIFrameWnd *)m_itemCompositionWnd)) {
			wnd = m_itemCompositionWnd = new UIItemCompositionWnd;
			wnd->Create(200, 200);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
			m_itemCompositionWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&itemCompositionWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_ITEMIDENTIFYWND:
		if (!(wnd = (UIFrameWnd *)m_itemIdentifyWnd)) {
			wnd = m_itemIdentifyWnd = new UIItemIdentifyWnd;
			wnd->Create(200, 200);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
			m_itemIdentifyWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&itemIdentifyWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_COMBOBOXWND:
		{
			if (!(window = m_comboBoxWnd)) {
				window = m_comboBoxWnd = new UIComboBoxWnd;
				window->Create(16, 16);
				wnd = window;
				wnd->Move(UIX(200), UICY(300));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_MENUWND:
		{
			if (!(window = m_menu)) {
				window = m_menu = new UIMenuWnd;
				window->Create(4, 4);
				wnd = window;
				wnd->Move(UIX(200), UICY(300));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_OPTIONWND:
		if (!(wnd = (UIFrameWnd *)m_optionWnd)) {
			wnd = m_optionWnd = new UIOptionWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(185), UICY(300));
			AddWindow(wnd);
			m_optionWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&optionWndInfo, 0, 0, 0);
			g_snapMgr.AddSnapObject(wnd);
		}
		wnd->SetShow(1);
		optionWndInfo.show = 1;
		break;
	case WID_MINIMAPZOOMWND:
		if (!(wnd = m_minimapZoomWnd)) {
			wnd = m_minimapZoomWnd = new UIMinimapZoomWnd;
			wnd->Create(12, 24);
			width = g_renderer->m_width - 29;
			wnd->Move(width, 17);
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_CHATROOMCHANGEWND:
		{
			if (!(window = m_chatRoomChangeWnd)) {
				window = m_chatRoomChangeWnd = new UIChatRoomChangeWnd;
				window->Create(280, 120);
				wnd = window;
				wnd->Move(UIX(185), UICY(300));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_CHATROOMMAKEWND:
		{
			if (!(window = m_chatRoomMakeWnd)) {
				window = m_chatRoomMakeWnd = new UIChatRoomMakeWnd;
				window->Create(280, 120);
				wnd = window;
				wnd->Move(UIX(185), UICY(300));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_CHOOSESELLBUYWND:
		{
			if (!(window = m_chooseSellBuyWnd)) {
				window = m_chooseSellBuyWnd = new UIChooseSellBuyWnd;
				window->Create(280, 120);
				wnd = window;
				wnd->Move(UIX(185), UICY(300));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_ITEMSELLWND:
		if (!(wnd = (UIFrameWnd *)m_itemSellWnd)) {
			wnd = m_itemSellWnd = new UIItemSellWnd;
			wnd->Create(280, 120);
			wnd->Move(300, 200);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&itemSellWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_MERCHANTITEMPURCHASEWND:
		if (!(wnd = (UIFrameWnd *)m_merchantItemPurchaseWnd)) {
			wnd = m_merchantItemPurchaseWnd = new UIMerchantItemPurchaseWnd;
			wnd->Create(280, 120);
			wnd->Move(300, 200);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&merchantItemPurchaseWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_ITEMPURCHASEWND:
		if (!(wnd = m_itemPurchaseWnd)) {
			wnd = m_itemPurchaseWnd = new UIItemPurchaseWnd;
			wnd->Create(280, 120);
			wnd->Move(300, 200);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&itemPurchaseWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		MakeWindow(WID_ITEMPARAMCHANGEDISPLAYWND);
		break;
	case WID_MERCHANTITEMMYSHOPWND:
		if (!(wnd = (UIFrameWnd *)m_merchantItemMyShopWnd)) {
			wnd = m_merchantItemMyShopWnd = new UIMerchantItemMyShopWnd;
			wnd->Create(280, 120);
			wnd->Move(0, 200);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&merchantItemMyShopWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_MERCHANTITEMSHOPWND:
		if (!(wnd = (UIFrameWnd *)m_merchantItemShopWnd)) {
			wnd = m_merchantItemShopWnd = new UIMerchantItemShopWnd;
			wnd->Create(280, 120);
			wnd->Move(0, 200);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&merchantItemShopWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_ITEMSHOPWND:
		if (!(wnd = (UIFrameWnd *)m_itemShopWnd)) {
			wnd = m_itemShopWnd = new UIItemShopWnd;
			wnd->Create(280, 120);
			wnd->Move(0, 200);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&itemShopWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_NOTIFYJOBLEVELUPWND:
		{
			if (!(window = m_notifyJobLevelUpWnd)) {
				window = m_notifyJobLevelUpWnd = new UINotifyJobLevelUpWnd;
				window->Create(43, 43);
				wnd = window;
				wnd->Move(0, g_renderer->m_height - 43);
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_NOTIFYLEVELUPWND:
		{
			if (!(window = m_notifyLevelUpWnd)) {
				window = m_notifyLevelUpWnd = new UINotifyLevelUpWnd;
				window->Create(43, 43);
				wnd = window;
				wnd->Move(g_renderer->m_width - 43, g_renderer->m_height - 43);
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_NOTIFYQUESTWND:
		{
			if (!(window = m_notifyQuestWnd)) {
				window = m_notifyQuestWnd = new UINotifyQuestWnd;
				window->Create(43, 43);
				wnd = window;
				wnd->Move(g_renderer->m_width - 43, g_renderer->m_height - 43);
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_CARDITEMILLUSTWND:
		{
			if (!(window = m_cardItemIllustWnd)) {
				window = m_cardItemIllustWnd = new UICardItemIllustWnd;
				window->Create(300, 417);
				wnd = window;
				wnd->Move(UIX(0), UICY(0));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_RESTARTWND:
		{
			if (!(window = m_restartWnd)) {
				window = m_restartWnd = new UIRestartWnd;
				window->Create(280, 120);
				wnd = window;
				wnd->Move(UIX(185), UICY(300));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_QUITWND:
		{
			if (!(window = m_quitWnd)) {
				window = m_quitWnd = new UIQuitWnd;
				window->Create(280, 120);
				wnd = window;
				wnd->Move(UIX(185), UICY(300));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_SKILLDESCRIBEWND:
		{
			if (!(window = m_skillDescribeWnd)) {
				window = m_skillDescribeWnd = new UISkillDescribeWnd;
				window->Create(280, 120);
				wnd = window;
				wnd->Move(UIX(185), UICY(300) - 120);
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_QUEST_DETAIL_WND:
		{
			if (!(window = m_questDetailWnd)) {
				window = m_questDetailWnd = new UIQuestDetailWnd;
				window->Create(350, 375);
				wnd = window;
				wnd->Move(UIX(10), UICY(50));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_ITEMPARAMCHANGEDISPLAYWND:
		if (g_serviceType == ServiceVietnam) break;
		if (!(wnd = (UIFrameWnd *)m_itemParamChangeDisplayWnd)) {
			wnd = m_itemParamChangeDisplayWnd = new UIItemParamChangeDisplayWnd;
			wnd->Create(280, 34);
			wnd->Move(UIX(185), UICY(300) - 34);
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&itemParamChangeDisplayWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_COMBINEDCARDITEMCOLLECTIONWND:
		if (!(wnd = (UIFrameWnd *)m_combinedCardItemCollectionWnd)) {
			wnd = m_combinedCardItemCollectionWnd = new UICombinedCardItemCollectionWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(185), UICY(300));
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&combinedCardItemCollectionWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_ITEMCOLLECTIONWND:
		if (!(wnd = (UIFrameWnd *)m_itemCollectionWnd)) {
			wnd = m_itemCollectionWnd = new UIItemCollectionWnd;
			wnd->Create(280, 120);
			wnd->Move(UIX(185), UICY(300));
			AddWindow(wnd);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&itemCollectionWndInfo, 0, 0, 0);
		}
		wnd->SetShow(1);
		break;
	case WID_NOTICECONFIRMWND:
		wnd = new UINoticeConfirmWnd;
		wnd->Create(280, 120);
		wnd->Move(UIX(185), UICY(300));
		AddWindow(wnd);
		break;
	case WID_MONSTERINFOWND:
		wnd = new UIMonsterInfoWnd;
		wnd->Create(400, 200);
		wnd->Move(0, 0);
		AddWindow(wnd);
		break;
	case WID_MAKETARGETLISTWND:
		{
			if (!(window = m_metalListWnd)) {
				window = m_metalListWnd = new UIMakeTargetListWnd;
				window->Create(280, 150);
				wnd = window;
				wnd->Move(UIX(185), UICY(300) - 60);
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_ITEMREPAIRWND:
		{
			if (!(window = m_itemRepairWnd)) {
				window = m_itemRepairWnd = new UIItemRepairWnd;
				window->Create(280, 150);
				wnd = window;
				wnd->Move(UIX(185), UICY(300) - 60);
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_WEAPONREFINEWND:
		{
			if (!(window = m_weaponRefineWnd)) {
				window = m_weaponRefineWnd = new UIWeaponRefineWnd;
				window->Create(280, 150);
				wnd = window;
				wnd->Move(UIX(185), UICY(300) - 60);
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_MAKETARGETPROCESSWND:
		{
			if (!(window = m_metalProcessWnd)) {
				window = m_metalProcessWnd = new UIMakeTargetProcessWnd;
				window->Create(280, 150);
				wnd = window;
				wnd->Move(UIX(185), UICY(300) - 60);
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_MAKETARGETRESULTWND:
		{
			if (!(window = m_metalResultWnd)) {
				window = m_metalResultWnd = new UIMakeTargetResultWnd;
				window->Create(280, 120);
				wnd = window;
				wnd->Move(UIX(185), UICY(300) - 60);
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_TALKBOXTRAPINPUTWND:
		wnd = new UITalkboxTrapInputWnd;
		wnd->Create(280, 120);
		wnd->Move(UIX(185), UICY(300) - 60);
		AddWindow(wnd);
		break;
	case WID_KEYSTROKEWND:
		{
			if (!(window = m_keyStrokeWnd)) {
				window = m_keyStrokeWnd = new UIKeyStrokeWnd;
				window->Create(60, 18);
				wnd = window;
				wnd->Move(UIX(185), UICY(300) - 60);
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_EMOTIONWND:
		if (!(wnd = m_emotionWnd)) {
			wnd = m_emotionWnd = new UIEmotionWnd;
			wnd->Create(375, 280);
			wnd->Move(UIX(185), UICY(300) - 60);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&emotionWndInfo, 0, 0, 0);
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_EMOTIONLISTWND:
		if (!(wnd = m_emotionListWnd)) {
			wnd = m_emotionListWnd = new UIEmotionListWnd;
			wnd->Create(445, 310);
			wnd->Move(UIX(185), UICY(400) - 60);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&emotionListWndInfo, 0, 0, 0);
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_PETINFOWND:
		if (g_session.m_petJob == -1) break;
		if (!(wnd = m_petInfoWnd)) {
			wnd = m_petInfoWnd = new UIPetInfoWnd;
			wnd->Create(280, 160);
			wnd->Move(UIX(185), UICY(400) - 60);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&petInfoWndInfo, 0, 0, 0);
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_SELECTPETEGGWND:
		{
			if (!(window = m_selectPetEggWnd)) {
				window = m_selectPetEggWnd = new UIPetEggListWnd;
				window->Create(200, 200);
				wnd = window;
				wnd->Move(UIX(200), UICY(200));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_PETTAMINGDECEIVEWND:
		{
			if (!(window = m_petTamingDeceiveWnd)) {
				window = m_petTamingDeceiveWnd = new UIPetTamingDeceiveWnd;
				window->Create(196, 256);
				wnd = window;
				wnd->Move(g_renderer->m_width / 2 - 98, g_renderer->m_height / 2 - 128);
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_MAKINGARROWLISTWND:
		{
			if (!(window = m_makingArrowListWnd)) {
				window = m_makingArrowListWnd = new UIMakingArrowListWnd;
				window->Create(200, 200);
				wnd = window;
				wnd->Move(UIX(200), UICY(200));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_SELECTCARTWND:
		{
			if (!(window = m_selectCartWnd)) {
				window = m_selectCartWnd = new UISelectCartWnd;
				window->Create(372, 98);
				wnd = window;
				wnd->Move(UIX(200), UICY(200));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_SPELLLISTWND:
		if (m_spellListWnd) {
			PostQuit(m_spellListWnd);
			m_spellListWnd = NULL;
		}
		wnd = new UISpellListWnd;
		m_spellListWnd = (UISpellListWnd *)wnd;
		wnd->Create(200, 200);
		wnd->Move(UIX(200), UICY(200));
		AddWindow(wnd);
		wnd->SetShow(1);
		break;
	case WID_GRAFFISTRBOXWND:
		wnd = new UIGraffiStrboxWnd;
		wnd->Create(280, 120);
		wnd->Move(UIX(185), UICY(300) - 60);
		AddWindow(wnd);
		break;
	case WID_PROHIBITLISTWND:
		{
			if (!(window = m_prohibitListWnd)) {
				window = m_prohibitListWnd = new UIProhibitListWnd;
				window->Create(500, 400);
				wnd = window;
				wnd->Move(UIX(100), UICY(100));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_BOOKWND:
		{
			if (!(window = m_bookWnd)) {
				window = m_bookWnd = new UIBookWnd;
				window->Create(600, 420);
				wnd = window;
				wnd->Move(UIX(0), UICY(0));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_JOINFRIENDACCEPTWND:
		{
			if (!(window = m_joinFriendAcceptWnd)) {
				window = m_joinFriendAcceptWnd = new UIJoinFriendAcceptWnd;
				window->Create(280, 120);
				wnd = window;
				wnd->Move(UIX(200), UICY(200));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_COMMANDLISTWND:
		{
			if (!(window = m_commandListWnd)) {
				window = m_commandListWnd = new UICommandListWnd;
				window->Create(250, 200);
				wnd = window;
				wnd->Move(UIX(400), UICY(200));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_HOMUNINFOWND:
		if (g_session.m_homunJob == -1) break;
		if (!(wnd = m_homunInfoWnd)) {
			wnd = m_homunInfoWnd = new UIHomunInfoWnd;
			wnd->Create(280, 180);
			wnd->Move(UIX(185), UICY(400) - 60);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&homunInfoWndInfo, 0, 0, 0);
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_HOMUNSKILLLISTWND:
		if (!(wnd = m_homunSkillListWnd)) {
			wnd = m_homunSkillListWnd = new UISkillListWnd;
			m_homunSkillListWnd->m_typeWnd = 1;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
			m_homunSkillListWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&homunSkillListWndInfo, 0, 0, 0);
		}
		m_homunSkillListWnd->m_typeWnd = 1;
		wnd->SetShow(1);
		homunSkillListWndInfo.show = 1;
		break;
	case WID_MERINFOWND:
		if (g_session.m_merJob == -1) break;
		if (!(wnd = m_merInfoWnd)) {
			wnd = m_merInfoWnd = new UIMerInfoWnd;
			wnd->Create(280, 180);
			wnd->Move(UIX(185), UICY(400) - 60);
			wnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&merInfoWndInfo, 0, 0, 0);
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_MERSKILLLISTWND:
		if (!(wnd = m_merSkillListWnd)) {
			wnd = m_merSkillListWnd = new UISkillListWnd;
			m_merSkillListWnd->m_typeWnd = 2;
			wnd->Create(280, 120);
			wnd->Move(UIX(200), UICY(200));
			AddWindow(wnd);
			m_merSkillListWnd->SendMsg(NULL, UM_SETINFO, (INT_PTR)&merSkillListWndInfo, 0, 0, 0);
		}
		m_merSkillListWnd->m_typeWnd = 2;
		wnd->SetShow(1);
		merSkillListWndInfo.show = 1;
		break;
	case WID_AUTOMESSAGEWND:
		{
			if (!(window = m_autoMessageWnd)) {
				window = m_autoMessageWnd = new UIAutoMessageWnd;
				window->Create(280, 48);
				wnd = window;
				wnd->Move(UIX(185), UIY(180));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_MAILLISTWND:
		if (m_merchantItemWnd) {
			g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_CLOSE_MERCHANTITEMWND), 0xff, 0, 0);
			break;
		}
		{
			if (!(window = m_mailListWnd)) {
				window = m_mailListWnd = new UIMailListWnd;
				window->Create(300, 400);
				wnd = window;
				wnd->Move(UIX(100), UICY(100));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_MAILVIEWWND:
		if (m_merchantItemWnd) {
			g_windowMgr.SendMsg(UIM_OPENCHATWND, 0, 0, 0, 0);
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_CLOSE_MERCHANTITEMWND), 0xff, 0, 0);
			break;
		}
		{
			if (!(window = m_mailViewWnd)) {
				window = m_mailViewWnd = new UIMailViewWnd;
				window->Create(300, 400);
				wnd = window;
				wnd->Move(UIX(400), UICY(100));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_AUCTIONWND:
		{
			if (!(window = m_auctionWnd)) {
				window = m_auctionWnd = new UIAuctionWnd;
				window->Create(550, 300);
				wnd = window;
				wnd->Move(UIX(200), UICY(100));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_AUCTIONMSGWND:
		{
			if (!(window = m_auctionMsgWnd)) {
				window = m_auctionMsgWnd = new UIAuctionMsgWnd;
				window->Create(236, 161);
				wnd = window;
				wnd->Move(UIX(300), UICY(130));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_IMGMENUWND:
		if (!m_imgMenuWnd) {
			wnd = new UIImgMenuWnd;
			m_imgMenuWnd = (UIImgMenuWnd *)wnd;
			wnd->Create(16, 16);
			wnd->Move(UIX(200), UICY(300));
			AddWindow(wnd);
		} else {

			wnd = m_comboBoxWnd;
		}
		wnd->SetShow(1);
		break;
	case WID_SELCHARFORUSERVERWND:
		wnd = new UISelCharForUServerWnd;
		m_selCharForUServerWnd = (UISelCharForUServerWnd *)wnd;
		wnd->Create(540, 400);
		wnd->Move(UIX(33), UIY(65));
		AddWindow(wnd);
		break;
	case WID_CHANGENAMEWND:
		{
			if (!(window = m_changeNameWnd)) {
				window = m_changeNameWnd = new UIChangeNameWnd;
				window->Create(184, 105);
				wnd = window;
				wnd->Move(UIX(260), UIY(180));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_REBIRTHWND:
		{
			if (!(window = m_rebirthWnd)) {
				window = m_rebirthWnd = new UIRebirthWnd;
				window->Create(280, 143);
				wnd = window;
				wnd->Move(UIX(185), UICY(300));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_SUBCHATWND_ST:
		if (!(wnd = (UIFrameWnd *)m_subChatWndST)) {
			wnd = m_subChatWndST = new UISubChatHisWnd;
			wnd->Create(300, 95);
			wnd->SetId(0);
			AddWindow(wnd);
			((UIWindow *)m_subChatWndST)->SendMsg(NULL, UM_SETINFO, (INT_PTR)&subChatWndStInfo, 0, 0, 0);
		}
		wnd->SetShow(0);
		break;
	case WID_SUBCHATWND_BT:
		if (!(wnd = (UIFrameWnd *)m_subChatWndBT)) {
			wnd = m_subChatWndBT = new UISubChatHisWnd;
			wnd->Create(300, 95);
			wnd->SetId(1);
			AddWindow(wnd);
			((UIWindow *)m_subChatWndBT)->SendMsg(NULL, UM_SETINFO, (INT_PTR)&subChatWndBtInfo, 0, 0, 0);
		}
		wnd->SetShow(0);
		break;
	case WID_BATTLEMSG_OPTIONWND:
		{
			if (!(window = m_battleMsgOpionWnd)) {
				window = m_battleMsgOpionWnd = new UIBattleMsgOptionWnd;
				window->Create(280, 200);
				wnd = window;
				wnd->Move(UIX(400), UICY(200));
				AddWindow(wnd);
			}
			wnd = window;
			wnd->SetShow(1);
		}
		break;
	case WID_SUBCHAT_MINIWND:
		if (!(wnd = (UIFrameWnd *)m_subChatMiniWnd)) {
			wnd = m_subChatMiniWnd = new UISubChatMiniWnd;
			wnd->Create(5, 5);
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_MEMORIAL_DUN_WND:
		if (g_session.m_dunName == "") return NULL;
		if (!(wnd = m_memorialDunWnd)) {
			wnd = m_memorialDunWnd = new UIMemorialDunWnd;
			wnd->Create(200, 150);
			if (g_session.dunInfoWndX >= 0 && g_session.dunInfoWndY >= 0)
				wnd->Move(g_session.dunInfoWndX, g_session.dunInfoWndY);
			else
				wnd->Move(0, 310);
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		g_session.m_onDunInfoWnd = 1;
		break;
	case WID_OTHERUSER_EQUIPWND:
		if (!(wnd = m_otherUserEquipWnd)) {
			wnd = m_otherUserEquipWnd = new UIEquipWnd;
			m_otherUserEquipWnd->m_typeWnd = 1;
			wnd->Create(280, 147);
			wnd->Move(UIX(185), UICY(300));
			AddWindow(wnd);
			g_snapMgr.AddSnapObject(wnd);
		}
		wnd->SetShow(1);
		break;
	case WID_ROMAPWND:
		if (!(wnd = m_roMapWnd)) {
			wnd = m_roMapWnd = new UIRoMapWnd;
			wnd->Create(g_renderer->m_width, g_renderer->m_height);
			wnd->Move(0, 0);
			AddWindow(wnd);
		}
		wnd->SetShow(1);
		g_windowMgr.MakeTopLayer((UIWindow *)m_roMapWnd);
		break;
	default:
		break;
	}

#if defined(BF_PORT)
	if (wnd) {
		BfMadeWindow made = { (int)windowId, wnd->m_w, wnd->m_h };
		g_bfMadeWindows[(UIWindow *)wnd] = made;
	}
#endif
	return wnd;
}

unsigned int __stdcall FinalStrike(void *pvoid)
{
	g_modeMgr.GetGameMode()->GetWorld()->m_player->SendMsg(NULL, AM_DELETE_EFFECT, 0x9f, 0, 0);
	g_modeMgr.GetGameMode()->GetWorld()->m_player->SendMsg(NULL, AM_DELETE_EFFECT, 0xa6, 0, 0);

	int sleepAdd = 0;
	int slowStep = 0;
	for (int i = 0; i < 160; ++i)
	{
		switch (i)
		{
		case 19:
		case 61:
		case 113:
		case 145:
		case 156:
		case 157:
			continue;
		}

		vector3d deltaPos = vector3d(0.0f, 0.0f, 0.0f);
		g_modeMgr.GetGameMode()->GetWorld()->m_player->LaunchEffect(i, deltaPos, 0.0f);
		bf_clock_sleep_ms(sleepAdd + 200);
		sleepAdd += 5 - slowStep;
		if ((i % 20) == 1)
			++slowStep;
	}

	return 0;
}

unsigned int __stdcall TestStrike(void * pvoid)
{
	{
		vector3d deltaPos = vector3d(0.0f, 0.0f, 0.0f);
		g_modeMgr.GetGameMode()->GetWorld()->m_player->LaunchEffect(0x1fe, deltaPos, 0.0f);
	}

	bf_clock_sleep_ms(1000);

	for (int i = 0; i < 7; ++i)
	{
		vector3d deltaPos = vector3d(0.0f, 0.0f, 0.0f);
		g_modeMgr.GetGameMode()->GetWorld()->m_player->LaunchEffect(0x1d2 + i, deltaPos, 0.0f);
		bf_clock_sleep_ms(1000);
	}

	return 0;
}














void MakeEffectList() {}

bool UIWindowMgr::ExcuteMsgInBattleMode(unsigned int virtualKey, int newKeydown)
{
	if (g_session.m_isBattle && !g_session.m_isBmChat) {
		if (m_shortCutWnd) {
			switch (newKeydown) {
			case 0x100001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 0, 1, 0, 0);
				break;
			case 0x110001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 1, 1, 0, 0);
				break;
			case 0x120001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 2, 1, 0, 0);
				break;
			case 0x130001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 3, 1, 0, 0);
				break;
			case 0x140001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 4, 1, 0, 0);
				break;
			case 0x150001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 5, 1, 0, 0);
				break;
			case 0x160001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 6, 1, 0, 0);
				break;
			case 0x170001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 7, 1, 0, 0);
				break;
			case 0x180001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 8, 1, 0, 0);
				break;
			case 0x1e0001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 0, 2, 0, 0);
				break;
			case 0x1f0001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 1, 2, 0, 0);
				break;
			case 0x200001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 2, 2, 0, 0);
				break;
			case 0x210001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 3, 2, 0, 0);
				break;
			case 0x220001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 4, 2, 0, 0);
				break;
			case 0x230001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 5, 2, 0, 0);
				break;
			case 0x240001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 6, 2, 0, 0);
				break;
			case 0x250001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 7, 2, 0, 0);
				break;
			case 0x260001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 8, 2, 0, 0);
				break;
			case 0x2c0001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 0, 0, 0, 0);
				break;
			case 0x2d0001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 1, 0, 0, 0);
				break;
			case 0x2e0001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 2, 0, 0, 0);
				break;
			case 0x2f0001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 3, 0, 0, 0);
				break;
			case 0x300001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 4, 0, 0, 0);
				break;
			case 0x310001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 5, 0, 0, 0);
				break;
			case 0x320001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 6, 0, 0, 0);
				break;
			case 0x330001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 7, 0, 0, 0);
				break;
			case 0x340001:
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 8, 0, 0, 0);
				break;
			}
		}
	}

	return false;
}

bool UIWindowMgr::ExcuteMsgInBattleModeVietnam(int lnKey)
{
	if (g_session.m_isBattle && !g_session.m_isBmChat) {
		if (m_shortCutWnd) {
			switch (lnKey) {
			case 'q':
			case 'Q':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 0, 1, 0, 0);
				break;
			case 'w':
			case 'W':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 1, 1, 0, 0);
				break;
			case 'e':
			case 'E':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 2, 1, 0, 0);
				break;
			case 'r':
			case 'R':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 3, 1, 0, 0);
				break;
			case 't':
			case 'T':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 4, 1, 0, 0);
				break;
			case 'y':
			case 'Y':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 5, 1, 0, 0);
				break;
			case 'u':
			case 'U':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 6, 1, 0, 0);
				break;
			case 'i':
			case 'I':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 7, 1, 0, 0);
				break;
			case 'o':
			case 'O':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 8, 1, 0, 0);
				break;
			case 'a':
			case 'A':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 0, 2, 0, 0);
				break;
			case 's':
			case 'S':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 1, 2, 0, 0);
				break;
			case 'd':
			case 'D':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 2, 2, 0, 0);
				break;
			case 'f':
			case 'F':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 3, 2, 0, 0);
				break;
			case 'g':
			case 'G':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 4, 2, 0, 0);
				break;
			case 'h':
			case 'H':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 5, 2, 0, 0);
				break;
			case 'j':
			case 'J':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 6, 2, 0, 0);
				break;
			case 'k':
			case 'K':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 7, 2, 0, 0);
				break;
			case 'l':
			case 'L':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 8, 2, 0, 0);
				break;
			case 'z':
			case 'Z':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 0, 0, 0, 0);
				break;
			case 'x':
			case 'X':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 1, 0, 0, 0);
				break;
			case 'c':
			case 'C':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 2, 0, 0, 0);
				break;
			case 'v':
			case 'V':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 3, 0, 0, 0);
				break;
			case 'b':
			case 'B':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 4, 0, 0, 0);
				break;
			case 'n':
			case 'N':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 5, 0, 0, 0);
				break;
			case 'm':
			case 'M':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 6, 0, 0, 0);
				break;
			case ',':
			case '<':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 7, 0, 0, 0);
				break;
			case '.':
			case '>':
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 8, 0, 0, 0);
				break;
			}
		}
	}

	return false;
}

inline BOOL CheckKeyPressed(int vkcode)
{
#if defined(BF_PORT)
	return HIBYTE(bf_input_async_key_state(vkcode));
#else
	return HIBYTE(GetAsyncKeyState(vkcode));
#endif
}

void UIWindowMgr::SetTextAtBattleMode()
{
	if (!m_chatWnd)
		return;
	if (m_editWindow == (UIWindow*)m_chatWnd->m_commonChat)
		m_chatWnd->m_commonChat->SetText("");
	if (m_editWindow == (UIWindow*)m_chatWnd->m_whisperChat)
		m_chatWnd->m_whisperChat->SetText(m_chatWnd->m_whisperChat->GetText());
}

bool UIWindowMgr::isFocusChatWnd()
{
	if (m_chatWnd) {
		if (GetFocusEdit() == (UIWindow*)m_chatWnd->m_commonChat || GetFocusEdit() == (UIWindow*)m_chatWnd->m_whisperChat)
			return true;
	}
	return false;
}

bool UIWindowMgr::ProcessPushButton(unsigned int virtualKey, int newKeydown, int accurateKey)
{
	switch (virtualKey) {
	case VK_ESCAPE:
		if (newKeydown) {
			if (m_itemCollectionWnd)
				DeleteWindow(WID_ITEMCOLLECTIONWND);
			else if (m_skillDescribeWnd)
				DeleteWindow(WID_SKILLDESCRIBEWND);
			else if (m_questDetailWnd)
				DeleteWindow(WID_QUEST_DETAIL_WND);
			else if (m_combinedCardItemCollectionWnd)
				DeleteWindow(WID_COMBINEDCARDITEMCOLLECTIONWND);
			else if (m_autoMessageWnd) {
				g_session.m_onAutoMSG = 0;
				DeleteWindow(WID_AUTOMESSAGEWND);
			}
			else if (m_battleMsgOpionWnd)
				m_battleMsgOpionWnd->SendMsg(NULL, UM_COMMAND, 0x87, 0, 0, 0);
			else if (!m_modalWindow && !g_sysQuit && g_modeMgr.GetLoopCond()) {
				CGameMode *gameMode = g_modeMgr.GetGameMode();
				if (gameMode) {
					if (!DeleteWindow(WID_RESTARTWND) && !DeleteWindow(WID_REBIRTHWND) && gameMode->IsPlayerDead()) {
						if (gameMode->HaveSiegfriedItem())
							MakeWindow(WID_REBIRTHWND);
						else
							MakeWindow(WID_RESTARTWND);
					}
					if (!DeleteWindow(WID_QUITWND) && !gameMode->IsPlayerDead())
						MakeWindow(WID_QUITWND);
				}
				else if (ErrorMsg(MsgStr(MSI_DO_YOU_REALLY_WANT_TO_QUIT), ME_YESNO, 1, 0, 0) == ID_YES) {
					CRagConnection::instanceR().Disconnect();
					g_modeMgr.GetCurMode()->SendMsg(MM_QUIT, 0, 0, 0);
				}
			}
			else
				m_modalResult = -1;
		}
		break;

	case VK_TAB:
		if (CheckKeyPressed(VK_CONTROL)) {
			if (m_basicInfoWnd) {
				m_basicInfoWnd->SendMsg(NULL, UM_COMMAND, 0x99, 0, 0, 0);
				return true;
			}
			break;
		}
		if (m_editWindow) {
#if defined(BF_PORT)
			if (bf_input_async_key_state(VK_SHIFT)) {
#else
			if (GetAsyncKeyState(VK_SHIFT)) {
#endif
				if (IsFirstFocusEdit(m_editWindow))
					SetFocusEditLast();
				else
					SetFocusEditPrev();
			}
			else {
				if (IsLastFocusEdit(m_editWindow))
					SetFocusEditFirst();
				else
					SetFocusEditNext();
			}
		}
		else
			FindFocusEditAndSetFocus();
		break;

	case 'X':
		if (CheckKeyPressed(VK_MENU)) {
			if (newKeydown) {
				if (!m_modalWindow && !g_sysQuit && g_modeMgr.GetLoopCond()) {
					CGameMode *gameMode = g_modeMgr.GetGameMode();
					if (gameMode) {
						if (!DeleteWindow(WID_RESTARTWND) && !DeleteWindow(WID_REBIRTHWND)) {
							if (gameMode->IsPlayerDead()) {
								if (gameMode->HaveSiegfriedItem())
									MakeWindow(WID_REBIRTHWND);
								else
									MakeWindow(WID_RESTARTWND);
							}
						}
						if (!DeleteWindow(WID_QUITWND))
							if (!gameMode->IsPlayerDead())
								MakeWindow(WID_QUITWND);
					}
					else if (ErrorMsg(MsgStr(MSI_DO_YOU_REALLY_WANT_TO_QUIT), ME_YESNO, 1, 0, 0) == ID_YES) {
						CRagConnection::instanceR().Disconnect();
						g_modeMgr.GetCurMode()->SendMsg(MM_QUIT, 0, 0, 0);
					}
				}
				else
					m_modalResult = -1;
			}
		}
		break;

	case VK_DELETE:
		if (newKeydown)
			DeleteButton();
		break;

	case VK_SPACE:
		if (g_windowMgr.isFocusChatWnd()) {
			if (g_session.m_isBattle && !g_session.m_isBmChat) {
				g_session.m_isBmChat = TRUE;
				g_session.m_isStartBmChat = TRUE;
			}
		}
		break;

	case VK_RETURN:
		if (g_session.m_isBattle) {
			if (g_serviceType == ServiceKorea || g_serviceType == ServiceVietnam || g_serviceType == ServiceThai)
				g_session.m_isBmChat = FALSE;
			else
				g_session.m_isBmChat = !g_session.m_isBmChat;
		}
		if (newKeydown)
			DefPushButton();
		break;

	case VK_PRIOR: {
		CMode *curMode = g_modeMgr.GetCurMode();
		if (curMode) {
			int useType = curMode->SendMsg(MM_GET_SKILL_USE_TYPE, 0, 0, 0);
			if (useType) {
				int useLevel = curMode->SendMsg(MM_GET_SKILL_USE_LEVEL, 0, 0, 0);
				int nextLevel = 5;
				if (useLevel == 1)
					nextLevel = 4;
				curMode->SendMsg(MM_USESKILL_LEVEL_PLUS, nextLevel, 0, 0);
			}
			else
				PageUpButton();
		}
		break;
	}

	case VK_NEXT: {
		CMode *curMode = g_modeMgr.GetCurMode();
		if (curMode) {
			int useType = curMode->SendMsg(MM_GET_SKILL_USE_TYPE, 0, 0, 0);
			if (useType) {
				int useLevel = curMode->SendMsg(MM_GET_SKILL_USE_LEVEL, 0, 0, 0);
				int nextLevel = 5;
				if (useLevel == 5)
					nextLevel = 4;
				curMode->SendMsg(MM_USESKILL_LEVEL_MINUS, nextLevel, 0, 0);
			}
			else
				PageDownButton();
		}
		break;
	}

	case VK_UP: {
		CMode *curMode = g_modeMgr.GetCurMode();
		if (curMode) {
			int useType = curMode->SendMsg(MM_GET_SKILL_USE_TYPE, 0, 0, 0);
			if (useType) {
				curMode->SendMsg(MM_USESKILL_LEVEL_PLUS, 1, 0, 0);
				return true;
			}
			UpButton();
		}
		break;
	}

	case VK_DOWN: {
		CMode *curMode = g_modeMgr.GetCurMode();
		if (curMode) {
			int useType = curMode->SendMsg(MM_GET_SKILL_USE_TYPE, 0, 0, 0);
			if (useType) {
				curMode->SendMsg(MM_USESKILL_LEVEL_MINUS, 1, 0, 0);
				return true;
			}
			DownButton();
		}
		break;
	}

	case VK_LEFT:
		LeftButton();
		break;

	case VK_RIGHT:
		RightButton();
		break;

	case VK_INSERT:
		if (newKeydown) {
			if (CheckKeyPressed(VK_MENU)) {
				m_isDrawCompass = 1 - m_isDrawCompass;
				return true;
			}
			if (!CheckKeyPressed(VK_SHIFT)) {
				CMode *curMode = g_modeMgr.GetCurMode();
				if (curMode)
					curMode->SendMsg(MM_PROCESS_TALK_TYPE, 0x16, 0, 0);
				return true;
			}
		}
		break;

	case VK_HOME:
		if (CheckKeyPressed(VK_MENU)) {
			if (newKeydown) {
				g_session.m_isAttrIndicator = 1 - g_session.m_isAttrIndicator;
				return true;
			}
		}
		else {
			CMode *curMode = g_modeMgr.GetCurMode();
			if (curMode) {
				int useType = curMode->SendMsg(MM_GET_SKILL_USE_TYPE, 0, 0, 0);
				if (useType) {
					curMode->SendMsg(MM_USESKILL_LEVEL_PLUS, 5000, 0, 0);
					return true;
				}
			}
		}
		break;

	// Restored from the reference (ProcessPushButton 0x4f58c0, case 0x23):
	// End turns the book's pages to its end, or sets the skill level to
	// the lowest; Alt+End toggles the player's HP bar.
	case VK_END:
		if (!CheckKeyPressed(VK_MENU)) {
			if (m_bookWnd) {
				m_bookWnd->SendMsg(NULL, UM_ENDPUSH, 0, 0, 0, 0);
				return true;
			}
			CMode *curMode = g_modeMgr.GetCurMode();
			if (curMode && curMode->SendMsg(MM_GET_SKILL_USE_TYPE, 0, 0, 0)) {
				curMode->SendMsg(MM_USESKILL_LEVEL_MINUS, 5000, 0, 0);
				return true;
			}
		}
		else if (newKeydown) {
			CMode *curMode = g_modeMgr.GetCurMode();
			if (curMode) {
				curMode->SendMsg(MM_TOGGLE_PLAYERGAGE, 0, 0, 0);
				return true;
			}
		}
		break;

	case 'A':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd) {
				m_basicInfoWnd->SendMsg(NULL, UM_COMMAND, 0x7e, 0, 0, 0);
				return true;
			}
		}
		else if (CheckKeyPressed(VK_CONTROL)) {
			if (m_statusWnd)
				m_statusWnd->SendMsg(NULL, UM_COMMAND, 0x88, 0, 0, 0);
		}
		break;

	case 'S':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd) {
				m_basicInfoWnd->SendMsg(NULL, UM_COMMAND, 0x82, 0, 0, 0);
				return true;
			}
		}
		break;

	case 'O':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd) {
				m_basicInfoWnd->SendMsg(NULL, UM_COMMAND, 0x7f, 0, 0, 0);
				return true;
			}
		}
		else if (CheckKeyPressed(VK_CONTROL)) {
			if (m_optionWnd)
				m_optionWnd->SendMsg(NULL, UM_COMMAND, 0x88, 0, 0, 0);
		}
		break;

	case 'E':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd) {
				m_basicInfoWnd->SendMsg(NULL, UM_COMMAND, 0x80, 0, 0, 0);
				return true;
			}
		}
		else if (CheckKeyPressed(VK_CONTROL)) {
			if (m_itemWnd)
				m_itemWnd->SendMsg(NULL, UM_COMMAND, 0x88, 0, 0, 0);
		}
		break;

	case 'W':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd) {
				m_basicInfoWnd->SendMsg(NULL, UM_COMMAND, 0xb5, 0, 0, 0);
				return true;
			}
		}
		break;

	case 'Q':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd) {
				m_basicInfoWnd->SendMsg(NULL, UM_COMMAND, 0x81, 0, 0, 0);
				return true;
			}
		}
		else if (CheckKeyPressed(VK_CONTROL)) {
			if (m_equipWnd)
				m_equipWnd->SendMsg(NULL, UM_COMMAND, 0x88, 0, 0, 0);
		}
		break;

	case 'V':
		if (CheckKeyPressed(VK_MENU) || CheckKeyPressed(VK_CONTROL) || newKeydown == 10000) {
			if (m_basicInfoWnd) {
				m_basicInfoWnd->SendMsg(NULL, UM_COMMAND, 0x88, 0, 0, 0);
				return true;
			}
		}
		break;

	case 'G':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd)
				if (g_guildInfo.m_gdid) {
					if (!g_windowMgr.DeleteWindow(WID_GUILDWND))
						g_windowMgr.MakeWindow(WID_GUILDWND);
				}
			return true;
		}
		break;

	case 'P':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd)
				if (!g_windowMgr.DeleteWindow(WID_PARTYSETTINGWND)) {
					if (g_session.GetNumParty())
						g_windowMgr.MakeWindow(WID_PARTYSETTINGWND);
				}
			return true;
		}
		else if (g_session.IsMasterAid(8) && CheckKeyPressed(VK_CONTROL)) {
			bf_sys_thread_create(FinalStrike, this, NULL);
			return true;
		}
		break;

	case 'J':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd)
				if (!g_windowMgr.DeleteWindow(WID_PETINFOWND))
					g_windowMgr.MakeWindow(WID_PETINFOWND);
			return true;
		}
		break;

	case 'T':
		if (CheckKeyPressed(VK_MENU) && g_session.m_homunAlive) {
			g_session.InitHomunTargetInfo();
			CGameMode *gameMode = g_modeMgr.GetGameMode();
			CGameActor *actor = gameMode->GetWorld()->GetGameActorByAID(g_session.m_homunGID);
			if (actor && actor->m_homunAI && actor->m_homunAI->IsOpened()) {
				_MSG2AI msg;
				msg.command = FOLLOW_CMD;
				actor->SetMsg2AI(msg, 0);
			}
		}
		if (CheckKeyPressed(VK_CONTROL) && g_session.m_merAlive) {
			g_session.InitMerTargetInfo();
			CGameMode *gameMode = g_modeMgr.GetGameMode();
			CGameActor *actor = gameMode->GetWorld()->GetGameActorByAID(g_session.m_merGID);
			if (actor && actor->m_merAI && actor->m_merAI->IsOpened()) {
				_MSG2AI msg;
				msg.command = FOLLOW_CMD;
				actor->SetMsg2AI(msg, 1);
			}
		}
		break;

	case 'R':
		if (CheckKeyPressed(VK_MENU)) {
			if (m_basicInfoWnd && g_session.m_homunAlive)
				if (!g_windowMgr.DeleteWindow(WID_HOMUNINFOWND))
					g_windowMgr.MakeWindow(WID_HOMUNINFOWND);
		}
		if (CheckKeyPressed(VK_CONTROL)) {
			if (m_basicInfoWnd && g_session.m_merAlive)
				if (!g_windowMgr.DeleteWindow(WID_MERINFOWND))
					g_windowMgr.MakeWindow(WID_MERINFOWND);
		}
		break;

	case 'B':
		if (CheckKeyPressed(VK_MENU)) {
			if (m_memorialDunWnd)
				m_memorialDunWnd->SetShow(!m_memorialDunWnd->m_show);
			else
				g_windowMgr.MakeWindow(WID_MEMORIAL_DUN_WND);
		}
		break;

	// Restored from the reference (ProcessPushButton 0x4f58c0, cases 0x43,
	// 0x44, 0x48, 0x49, 0x4c, 0x4d, 0x55, 0x59, 0x5a): Alt+C chat room,
	// Alt+D detail level, Alt+H friends and Alt+Z party (the messenger
	// window's two tabs), Alt+I friend options, Alt+L emotion list, Alt+M
	// emotions, Alt+U quests (Korea only), Alt+Y command list.
	case 'C':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd) {
				m_basicInfoWnd->SendMsg(NULL, UM_COMMAND, ID_COMM, 0, 0, 0);
				return true;
			}
		}
		break;

	case 'D':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd) {
				m_basicInfoWnd->SendMsg(NULL, UM_COMMAND, ID_DETAILLEVEL, 0, 0, 0);
				return true;
			}
		}
		break;

	case 'H':
	case 'Z':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd) {
				int oldRadio = messengerGroupWndInfo.radio;
				messengerGroupWndInfo.radio = virtualKey == 'Z';
				if (!DeleteWindow(WID_MESSENGERWND) || oldRadio != messengerGroupWndInfo.radio)
					MakeWindow(WID_MESSENGERWND);
			}
		}
		break;

	case 'I':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd)
				if (!DeleteWindow(WID_FRIENDOPTIONWND))
					MakeWindow(WID_FRIENDOPTIONWND);
			return true;
		}
		break;

	case 'L':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd)
				if (!DeleteWindow(WID_EMOTIONLISTWND))
					MakeWindow(WID_EMOTIONLISTWND);
			return true;
		}
		break;

	case 'M':
		if (CheckKeyPressed(VK_MENU) || newKeydown == 10000) {
			if (m_basicInfoWnd)
				if (!DeleteWindow(WID_EMOTIONWND))
					MakeWindow(WID_EMOTIONWND);
			return true;
		}
		break;

	case 'U':
		if (g_serviceType == ServiceKorea && (CheckKeyPressed(VK_MENU) || newKeydown == 10000)) {
			if (m_basicInfoWnd) {
				m_basicInfoWnd->SendMsg(NULL, UM_COMMAND, ID_QUEST, 0, 0, 0);
				return true;
			}
		}
		break;

	case 'Y':
		if (CheckKeyPressed(VK_MENU)) {
			if (m_basicInfoWnd)
				if (!DeleteWindow(WID_COMMANDLISTWND))
					MakeWindow(WID_COMMANDLISTWND);
		}
		break;

	case '1':
		if (newKeydown) {
			if (CheckKeyPressed(VK_MENU)) {
				ProcessShortcutMessage(0);
				return true;
			}
			if (CheckKeyPressed(VK_CONTROL)) {
				if (g_serviceType == ServiceKorea || g_serviceType == ServiceIndonesia) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x0d, 0, 0);
				} else if (g_serviceType == ServiceBrazil) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x33, 0, 0);
				} else if (g_serviceType == ServiceRussia) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x42, 0, 0);
				} else if (g_serviceType == ServiceIndia) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x40, 0, 0);
				}
				return true;
			}
		}
		break;

	case '2':
		if (newKeydown) {
			if (CheckKeyPressed(VK_MENU)) {
				ProcessShortcutMessage(1);
				return true;
			}
			if (CheckKeyPressed(VK_CONTROL)) {
				if (g_serviceType == ServiceKorea || g_serviceType == ServiceIndonesia) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x23, 0, 0);
				} else if (g_serviceType == ServiceRussia) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x43, 0, 0);
				}
				return true;
			}
		}
		break;

	case '3':
		if (newKeydown) {
			if (CheckKeyPressed(VK_MENU)) {
				ProcessShortcutMessage(2);
				return true;
			}
			if (CheckKeyPressed(VK_CONTROL)) {
				if (g_serviceType == ServiceKorea || g_serviceType == ServicePhilippine) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x30, 0, 0);
				} else if (g_serviceType == ServiceRussia) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x0d, 0, 0);
				}
				return true;
			}
		}
		break;

	case '4':
		if (newKeydown) {
			if (CheckKeyPressed(VK_MENU)) {
				ProcessShortcutMessage(3);
				return true;
			}
			if (CheckKeyPressed(VK_CONTROL)) {
				if (g_serviceType == ServiceKorea || g_serviceType == ServiceMalaysia || g_serviceType == ServiceSingapore) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x31, 0, 0);
				} else if (g_serviceType == ServiceRussia) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x23, 0, 0);
				}
				return true;
			}
		}
		break;

	case '5':
		if (newKeydown) {
			if (CheckKeyPressed(VK_MENU)) {
				ProcessShortcutMessage(4);
				return true;
			}
			if (CheckKeyPressed(VK_CONTROL)) {
				if (g_serviceType == ServiceKorea || g_serviceType == ServiceMalaysia || g_serviceType == ServiceSingapore) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x32, 0, 0);
				} else if (g_serviceType == ServiceRussia) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x30, 0, 0);
				}
				return true;
			}
		}
		break;

	case '6':
		if (newKeydown) {
			if (CheckKeyPressed(VK_MENU)) {
				ProcessShortcutMessage(5);
				return true;
			}
			if (CheckKeyPressed(VK_CONTROL)) {
				if (g_serviceType == ServiceKorea) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x33, 0, 0);
				} else if (g_serviceType == ServiceRussia) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x31, 0, 0);
				}
				return true;
			}
		}
		break;

	case '7':
		if (newKeydown) {
			if (CheckKeyPressed(VK_MENU)) {
				ProcessShortcutMessage(6);
				return true;
			}
			if (CheckKeyPressed(VK_CONTROL)) {
				if (g_serviceType == ServiceKorea || g_serviceType == ServiceTaiwan) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x40, 0, 0);
				} else if (g_serviceType == ServiceRussia) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x32, 0, 0);
				}
				return true;
			}
		}
		break;

	case '8':
		if (newKeydown) {
			if (CheckKeyPressed(VK_MENU)) {
				ProcessShortcutMessage(7);
				return true;
			}
			if (CheckKeyPressed(VK_CONTROL)) {
				if (g_serviceType == ServiceKorea) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x42, 0, 0);
				} else if (g_serviceType == ServiceRussia) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x33, 0, 0);
				}
				return true;
			}
		}
		break;

	case '9':
		if (newKeydown) {
			if (CheckKeyPressed(VK_MENU)) {
				ProcessShortcutMessage(8);
				return true;
			}
			if (CheckKeyPressed(VK_CONTROL)) {
				if (g_serviceType == ServiceKorea) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x43, 0, 0);
				} else if (g_serviceType == ServiceRussia) {
					CMode *curMode = g_modeMgr.GetCurMode();
					curMode->SendMsg(MM_REQ_EMOTION, 0x40, 0, 0);
				}
				return true;
			}
		}
		break;

	case '0':
		if (newKeydown) {
			if (CheckKeyPressed(VK_MENU)) {
				ProcessShortcutMessage(9);
				return true;
			}
		}
		break;

	case 189:
		if (newKeydown) {
			if (CheckKeyPressed(VK_CONTROL)) {
				CMode *curMode = g_modeMgr.GetCurMode();
				curMode->SendMsg(MM_REQ_EMOTION, 0x0a, 0, 0);
				return true;
			}
		}
		break;

	case 187:
		if (newKeydown) {
			if (CheckKeyPressed(VK_CONTROL)) {
				CMode *curMode = g_modeMgr.GetCurMode();
				curMode->SendMsg(MM_REQ_EMOTION, 0x0b, 0, 0);
				return true;
			}
		}
		break;

	case 220:
		if (newKeydown) {
			if (CheckKeyPressed(VK_CONTROL)) {
				CMode *curMode = g_modeMgr.GetCurMode();
				curMode->SendMsg(MM_REQ_EMOTION, 0x0c, 0, 0);
				return true;
			}
		}
		break;

	// Restored from the reference (ProcessPushButton 0x4f58c0, case 0xc0):
	// Ctrl+` opens the world map.
	case 192:
		if (CheckKeyPressed(VK_CONTROL)) {
			if (m_basicInfoWnd)
				if (!DeleteWindow(WID_ROMAPWND))
					MakeWindow(WID_ROMAPWND);
		}
		break;

	case VK_MULTIPLY:
		if (CheckKeyPressed(VK_CONTROL)) {
			if (m_miniMapArgb == 0xffffffff)
				m_miniMapArgb = 0xaaffffff;
			else
				m_miniMapArgb = 0xffffffff;
			return true;
		}
		break;

	case VK_ADD:
		if (CheckKeyPressed(VK_CONTROL)) {
			if (m_minimapZoomWnd) {
				m_minimapZoomWnd->SendMsg(NULL, UM_COMMAND, 0x96, 0, 0, 0);
				return true;
			}
		}
		break;

	case VK_SUBTRACT:
		if (CheckKeyPressed(VK_CONTROL)) {
			if (m_minimapZoomWnd) {
				m_minimapZoomWnd->SendMsg(NULL, UM_COMMAND, 0x97, 0, 0, 0);
				return true;
			}
		}
		break;

#if defined(BF_PORT)
	// Restored from the reference (ProcessPushButton 0x4f58c0, cases 0x70
	// and 0x71): F1 and F2 use the shortcut bar's first two slots, as F3-F9
	// use the others.
	case VK_F1:
		if (newKeydown) {
			if (m_shortCutWnd)
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 0, 0, 0, 0);
		}
		break;

	case VK_F2:
		if (newKeydown) {
			if (m_shortCutWnd)
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 1, 0, 0, 0);
		}
		break;
#else
	case VK_F1:
	case VK_F2:
		break;
#endif

	case VK_F3:
		if (newKeydown) {
			if (m_shortCutWnd)
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 2, 0, 0, 0);
		}
		break;

	case VK_F4:
		if (newKeydown) {
			if (m_shortCutWnd)
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 3, 0, 0, 0);
		}
		break;

	case VK_F5:
		if (newKeydown) {
			if (m_shortCutWnd)
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 4, 0, 0, 0);
		}
		break;

	case VK_F6:
		if (newKeydown) {
			if (m_shortCutWnd)
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 5, 0, 0, 0);
		}
		break;

	case VK_F7:
		if (newKeydown) {
			if (m_shortCutWnd)
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 6, 0, 0, 0);
		}
		break;

	case VK_F8:
		if (newKeydown) {
			if (m_shortCutWnd)
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 7, 0, 0, 0);
		}
		break;

	case VK_F9:
		if (newKeydown) {
			if (m_shortCutWnd)
				m_shortCutWnd->SendMsg(NULL, UM_USE_SHORTEN_ITEM, 8, 0, 0, 0);
		}
		break;

	case VK_F10:
		if (m_basicInfoWnd) {
			if (CheckKeyPressed(VK_MENU)) {
				this->ShowWindow(WID_CHATWND, 1 - m_chatWndShow);
				return true;
			}
			if (m_chatWnd) {
				if (m_chatWnd->m_show) {
					m_chatWnd->SendMsg(NULL, UM_TOGGLEPUSH, 0, 0, 0, 0);
					if (!m_chatWnd->m_state)
						this->ShowWindow(WID_CHATWND, 0);
				}
				else
					this->ShowWindow(WID_CHATWND, 1);
			}
		}
		break;

#if defined(BF_PORT)
	// Restored from the reference (ProcessPushButton 0x4f58c0, cases 0x7a
	// and 0x7b): F11 closes the windows (the basic window's ID_CLOSEALL);
	// F12 turns the shortcut bar to its next page of three and shows it.
	case VK_F11:
		if (m_basicInfoWnd)
			m_basicInfoWnd->SendMsg(NULL, UM_COMMAND, ID_CLOSEALL, 0, 0, 0);
		break;

	case VK_F12:
		if (m_basicInfoWnd) {
			if (++g_session.m_shortcutSlotCnt > 2)
				g_session.m_shortcutSlotCnt = 0;
			this->ShowWindow(WID_SHORTCUTWND, 0);
			this->ShowWindow(WID_SHORTCUTWND, 1);
		}
		break;
#else
	case VK_F11:
	case VK_F12:
		break;
#endif

	case VK_SNAPSHOT:
	case VK_SCROLL:
		if (newKeydown)
			g_modeMgr.GetCurMode()->SendMsg(MM_SCREENSHOT, 0, 0, 0);
		break;

	default:
		return false;
	}

	return false;
}

INT_PTR UIWindowMgr::SendMsg(int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4)
{
	RAG_BUDGET_FILL(11)
	switch (message) {
	case UIM_MAKE_WHISPER_WINDOW: {
		std::string wName((const char*)val1);
		std::map<std::string, UIWindow *>::iterator it = m_whisperNameWindow.find(wName);
		if (it == m_whisperNameWindow.end()) {
			UIWhisperWnd *whisperWindow = new UIWhisperWnd;
			whisperWindow->Create(280, 120);
			AddWindow(whisperWindow);
			whisperWindow->SendMsg(NULL, UM_SETINFO, (INT_PTR)&whisperWndInfo, 0, 0, 0);
			whisperWindow->SetShow(1);
			whisperWindow->m_whisperName = wName;
			if (val2 == 0)
				whisperWindow->m_whisperTitle = wName;
			else
				whisperWindow->m_whisperTitle = wName + "  (" + (const char*)val2 + ")  *^_^*";

			int count = m_whisperNameWindow.size();
			if (count) {
				int x = whisperWndInfo.x + count * 17;
				int y = whisperWndInfo.y + count * 17;
				int ww = whisperWndInfo.w;
				int wh = whisperWndInfo.h;
				if (x > 0 && x + ww < g_renderer->m_width - 17 && y > 0 && y + wh < g_renderer->m_height - 17) {
					whisperWindow->Move(whisperWndInfo.x + count * 17, whisperWndInfo.y + count * 17);
				}
				else {
					int x2 = count * 17;
					int y2 = count * 17;
					if (x2 > 0 && x2 + ww < g_renderer->m_width - 17 && y2 > 0 && y2 + wh < g_renderer->m_height - 17)
						whisperWindow->Move(x2, y2);
				}
			}

			AddWhisperWindow(wName, whisperWindow);
			AddWhisperCharName(wName.c_str());
			return 1;
		}
		break;
	}

	case UIM_SAVE_CHAT_TO_FILE_FROM_CHATROOM:
		if (m_chatRoomWnd)
			m_chatRoomWnd->WriteChatToFile();
		break;

	case UIM_SAVE_CHAT_TO_FILE:
		if (m_chatWnd)
			m_chatWnd->WriteChatToFile();
		break;

	case UIM_REPORT_CHAT_TO_FILE_FROM_CHATROOM:
		if (m_chatRoomWnd)
			m_chatRoomWnd->WriteChatToReportFile();
		break;

	case UIM_PUSH_INTO_CHATROOM:
		if (m_chatRoomWnd)
			m_chatRoomWnd->SendMsg(NULL, UM_ADDCHAT, val1, val2, 0, 0);
		break;

	case UIM_PUSHINTOCHATHISTORY:
	case UIM_PUSHINTOCHATHISTORY2:
#if defined(BF_PORT)
		bf_client_chat_line(message == UIM_PUSHINTOCHATHISTORY2, (const char*)val1, (int)val3, (const char*)val4);
#endif
		if (m_chatWnd) {
			unsigned int colorRef = val2;
			if (colorRef == 0)
				colorRef = 0xffffff;
			if (strcmp((const char*)val1, "No Msg") != 0 && strcmp((const char*)val1, "NO MSG") != 0) {
				if (message == UIM_PUSHINTOCHATHISTORY2)
					m_chatWnd->SendMsg(NULL, UM_ADDCHAT2, val1, colorRef, val3, val4);
				else
					m_chatWnd->SendMsg(NULL, UM_ADDCHAT, val1, colorRef, val3, val4);
			}
		}
		else {
			unsigned int colorRef = val2;
			if (colorRef == 0)
				colorRef = 0xffffff;
			std::vector<std::string> strings;
			CutString((const char*)val1, strings, 0x5e);
			for (int i = 0; i < strings.size(); i++) {
				StringAndColor sc;
				sc.String = strings[i];
				sc.Color = colorRef;
				m_chatWndHistory.push_back(sc);
			}
		}
		break;

	case UIM_LOADINGPERCENT:
		if (m_loadingWnd)
			m_loadingWnd->SendMsg(NULL, UM_LOADINGPERCENT, val1, 0, 0, 0);
		break;

	case UIM_OPENCHATWND:
		this->ShowWindow(WID_CHATWND, 1);
		if (m_chatWnd)
			m_chatWnd->SendMsg(NULL, UM_OPENCHATWND, 0, 0, 0, 0);
		break;

	case UIM_CHANGECHATWNDSTATUS:
		if (m_chatWnd)
			m_chatWnd->SendMsg(NULL, UM_CHANGESTATUS, val1, 0, 0, 0);
		break;

	case UIM_IME_OPENCANDIDATE:
	case UIM_IME_CHANGECANDIDATE: {
		UIFrameWnd *wnd = MakeWindow(WID_CANDIDATEWND);
		wnd->SendMsg(NULL, UM_IME_SET_CANDIDATE, val1, val2, 0, 0);
		break;
	}

	case UIM_IME_CLOSECANDIDATE:
		DeleteWindow(WID_CANDIDATEWND);
		break;
	}
	return 0;
}


void UIWindowMgr::AddWhisperWindow(std::string wName, UIWindow *whisperWindow)
{
	std::map<std::string, UIWindow *>::iterator it = m_whisperNameWindow.find(wName);
	if (it == m_whisperNameWindow.end())
		m_whisperNameWindow[wName] = whisperWindow;
}

void UIWindowMgr::DeleteWhisperWindow(std::string wName)
{
	std::map<std::string, UIWindow *>::iterator it = m_whisperNameWindow.find(wName);
	if (it != m_whisperNameWindow.end())
		m_whisperNameWindow.erase(it);
}

void UIWindowMgr::AddToNameWaitingWindowList(UIWindow* wnd)
{
	std::list<UIWindow *>::iterator i = std::find(m_nameWaitingList.begin(), m_nameWaitingList.end(), wnd);
	if (i == m_nameWaitingList.end()) m_nameWaitingList.push_back(wnd);
}

void UIWindowMgr::RemoveFromNameWaitingWindowList(UIWindow* wnd)
{
	std::list<UIWindow *>::iterator i = std::find(m_nameWaitingList.begin(), m_nameWaitingList.end(), wnd);
	if (i != m_nameWaitingList.end()) m_nameWaitingList.erase(i);
}

void UIWindowMgr::RefreshNameWaitingWindowList()
{
	std::list<UIWindow *>::iterator i = m_nameWaitingList.begin();
	for (; i != m_nameWaitingList.end(); ++i) (*i)->Invalidate();
	m_nameWaitingList.clear();
}

int UIWindowMgr::IsFriendName(const char* cName)
{
	return g_session.IsFriendName(cName) != 0;
}

int UIWindowMgr::AddWhisperChatToWhisperWnd(char const *cName, char const *chatMsg, unsigned int color)
{
	int isFriendName = g_session.IsFriendName(cName) ? 1 : 0;
	int madeWindow = 0;

	bool friendOnly = !g_session.m_isShowWhisperWnd || isFriendName;
	if (friendOnly) {
		if (g_session.m_isShowWhisperWnd_Friend && isFriendName)
			madeWindow = SendMsg(UIM_MAKE_WHISPER_WINDOW, (INT_PTR)cName, (INT_PTR)MsgStr(MSI_FRIEND), 0, 0);
	}
	else if (g_session.IsGuildMember(cName)) {
		madeWindow = SendMsg(UIM_MAKE_WHISPER_WINDOW, (INT_PTR)cName, (INT_PTR)"GuildMember", 0, 0);
	}
	else {
		if (g_session.IsCheatName(cName))
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_ID_WARNING), 0xffff, 0, 0);
		else if (g_session.IsCheatGuildName(cName))
			g_windowMgr.SendMsg(UIM_PUSHINTOCHATHISTORY, (INT_PTR)MsgStr(MSI_ID_WARNING_GUILD), 0xffff, 0, 0);
		madeWindow = SendMsg(UIM_MAKE_WHISPER_WINDOW, (INT_PTR)cName, 0, 0, 0);
	}

	if (madeWindow) {
		if (g_resMgr().IsExist(whisperWindowOpenSound)) {
			PlayWave(whisperWindowOpenSound, 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
		}
		else {
			if (g_session.m_isPlayWhisperOpenSound)
				PlayWave("\xb9\xf6\xc6\xb0\xbc\xd2\xb8\xae.wav", 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
		}
	}

	if (strcmp(cName, g_session.GetCharName()) != 0) {
		if (g_session.m_isShowWhisperWnd && madeWindow)
			g_windowMgr.AddWhisperCharName(cName);

		std::map<std::string, UIWindow *>::iterator it = m_whisperNameWindow.find(cName);
		if (it != m_whisperNameWindow.end()) {
			UIWindow *whisperWnd = it->second;
			whisperWnd->SendMsg(NULL, UM_ADDCHAT, (INT_PTR)chatMsg, color, 2, 0);
			MakeTopLayer(whisperWnd);
			return 1;
		}
	}
	return 0;
}

void UIWindowMgr::WriteWhisperChatToFile(char const *cName)
{
	std::map<std::string, UIWindow *>::iterator it = m_whisperNameWindow.find(cName);
	if (it != m_whisperNameWindow.end())
		m_whisperNameWindow[cName]->SendMsg(NULL, UM_MENU_SELECTED, 0, 0, 0, 0);
}

void UIWindowMgr::InvalidateGuildWnd()
{
	if (m_guildInfoManageWnd)
		((UIWindow*)m_guildInfoManageWnd)->SendMsg(NULL, UM_REFRESH, 0, 0, 0, 0);
	if (m_guildMemberManageWnd)
		((UIWindow*)m_guildMemberManageWnd)->SendMsg(NULL, UM_REFRESH, 0, 0, 0, 0);
	if (m_guildPositionManageWnd)
		((UIWindow*)m_guildPositionManageWnd)->SendMsg(NULL, UM_REFRESH, 0, 0, 0, 0);
	if (m_guildSkillWnd)
		((UIWindow*)m_guildSkillWnd)->SendMsg(NULL, UM_REFRESH, 0, 0, 0, 0);
	if (m_guildBanishedMemberWnd)
		((UIWindow*)m_guildBanishedMemberWnd)->SendMsg(NULL, UM_REFRESH, 0, 0, 0, 0);
	if (m_guildNoticeWnd)
		((UIWindow*)m_guildNoticeWnd)->SendMsg(NULL, UM_REFRESH, 0, 0, 0, 0);
	if (m_guildTotalInfoWnd)
		((UIWindow*)m_guildTotalInfoWnd)->SendMsg(NULL, UM_REFRESH, 0, 0, 0, 0);
}

void UIWindowMgr::ProcessShortcutMessage(int Index)
{
	char chatBuf[256];
	strcpy(chatBuf, g_session.m_shortcutText[Index].c_str());

	CGameMode *curMode = (CGameMode *)g_modeMgr.GetCurMode();
	if (chatBuf[0] != '/') {
		g_windowMgr.SetChatMsg(chatBuf);
		if (g_session.m_talkType == 2 && g_guildInfo.m_gdid)
			curMode->SendMsg(MM_CHAT_GUILD, 0, 0, 0);
		else if (g_session.m_talkType == 1 && g_session.GetNumParty())
			curMode->SendMsg(MM_CHAT_PARTY, 0, 0, 0);
		else
			curMode->SendMsg(MM_CHATMSG, 0, 0, 0);
	}

	else {
		std::string param[3];
		TALKTYPE talkType;
		int pos = g_session.GetTalkType(chatBuf, &talkType, param);
		if (pos != -1)
			g_windowMgr.SetChatMsg(chatBuf + pos);

		g_modeMgr.GetCurMode()->SendMsg(MM_PROCESS_TALK_TYPE, talkType, (INT_PTR)param, 0);
	}
}

bool UIWindowMgr::IsAniAccessory(int accessory)
{
	switch (accessory) {
	case 264:
		return true;
	case 311:
		return true;
	case 368:
		return true;
	}
	return true;
}

void UIWindowMgr::ReadToReg()
{
	bf_setting_key hKey;
	char OptionPath[512];
	DWORD DATALENGTH;
	char buf[10];

	strcpy(OptionPath, g_regPath);
	strcat(OptionPath, "\\ChatOption");

	DATALENGTH = sizeof(DWORD);
	bf_setting_open(hKey, OptionPath);

	for (int i = 0; i < 13; ++i)
	{
		itoa(i, buf, 10);
		BYTE* option = &g_session.m_battleMsgOnOffSet[i];
		if (!bf_setting_read(hKey, buf, option, &DATALENGTH))
			*option = 1;
	}

	bf_setting_close(hKey);
}
