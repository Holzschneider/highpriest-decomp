#ifndef HIGHPRIEST_WINDOW_UIFRAMEWND_H
#define HIGHPRIEST_WINDOW_UIFRAMEWND_H

#include "UISys.h"

#include "../LoginMode.h"
#include "../Session.h"
#include <shellapi.h>

enum WINDOWID : int {
	WID_BASICINFOWND = 0,
	WID_CHATWND = 1,
	WID_SELECTSERVERWND = 2,
	WID_LOGINWND = 3,
	WID_MAKECHARWND = 4,
	WID_SELECTCHARWND = 5,
	WID_WAITWND = 6,
	WID_LOADINGWND = 7,
	WID_ITEMWND = 8,
	WID_TOOLTIPWND = 9,
	WID_EQUIPWND = 10,
	WID_STATUSWND = 11,
	WID_ITEMCOLLECTIONWND = 12,
	WID_OPTIONWND = 13,
	WID_MINIMAPZOOMWND = 14,
	WID_ITEMDROPCNTWND = 15,
	WID_SAYDIALOGWND = 16,
	WID_CHOOSEWND = 17,
	WID_MENUWND = 18,
	WID_RESTARTWND = 19,
	WID_NOTICECONFIRMWND = 20,
	WID_NOTIFYLEVELUPWND = 21,
	WID_ITEMSHOPWND = 22,
	WID_ITEMPURCHASEWND = 23,
	WID_ITEMSELLWND = 24,
	WID_CHOOSESELLBUYWND = 25,
	WID_COMBOBOXWND = 26,
	WID_CHATROOMMAKEWND = 27,
	WID_CHATROOMWND = 28,
	WID_PASSWORDWND = 29,
	WID_CHATROOMCHANGEWND = 30,
	WID_EXCHANGEWND = 31,
	WID_EXCHANGEACCEPTWND = 32,
	WID_ITEMSTOREWND = 33,
	WID_MESSENGERGROUPWND = 34,
	WID_JOINPARTYACCEPTWND = 35,
	WID_SHORTCUTWND = 36,
	WID_SKILLLISTWND = 37,
	WID_TIPOFTHEDAYWND = 38,
	WID_CHOOSEWARPWND = 39,
	WID_MERCHANTITEMWND = 40,
	WID_MERCHANTSHOPMAKEWND = 41,
	WID_MERCHANTMIRRORITEMWND = 42,
	WID_MERCHANTITEMSHOPWND = 43,
	WID_MERCHANTITEMPURCHASEWND = 44,
	WID_MERCHANTITEMMYSHOPWND = 45,
	WID_SKILLDESCRIBEWND = 46,
	WID_CARDITEMILLUSTWND = 47,
	WID_QUITWND = 48,
	WID_NOTIFYJOBLEVELUPWND = 49,
	WID_ITEMPARAMCHANGEDISPLAYWND = 50,
	WID_CANDIDATEWND = 51,
	WID_COMPOSITIONWND = 52,
	WID_PARTYSETTINGWND = 53,
	WID_EMAILADDRESSWND = 54,
	WID_SKILLNAMECHANGEWND = 55,
	WID_NPCEDITDIALOGWND = 56,
	WID_DETAILLEVELWND = 57,
	WID_NOTIFYITEMOBTAINWND = 58,
	WID_GUILDWND = 59,
	WID_GUILDINFOMANAGEWND = 60,
	WID_GUILDMEMBERMANAGEWND = 61,
	WID_GUILDPOSITIONMANAGEWND = 62,
	WID_GUILDSKILLWND = 63,
	WID_GUILDBANISHEDMEMBERWND = 64,
	WID_GUILDNOTICEWND = 65,
	WID_GUILDTOTALINFOWND = 66,
	WID_WHISPERLISTWND = 67,
	WID_FRIENDOPTIONWND = 68,
	WID_MESSENGERWND = 69,
	WID_JOINGUILDACCEPTWND = 70,
	WID_WEBBROWSERWND = 71,
	WID_ALLYGUILDACCEPTWND = 72,
	WID_ITEMIDENTIFYWND = 73,
	WID_ITEMCOMPOSITIONWND = 74,
	WID_GUILDLEAVEREASONDESCWND = 75,
	WID_GUILDBANREASONDESCWND = 76,
	WID_MONSTERINFOWND = 77,
	WID_ILLUSTWND = 78,
	WID_MAKETARGETLISTWND = 79,
	WID_MAKETARGETPROCESSWND = 80,
	WID_MAKETARGETRESULTWND = 81,
	WID_COMBINEDCARDITEMCOLLECTIONWND = 82,
	WID_CALCULATORWND = 83,
	WID_TALKBOXTRAPINPUTWND = 84,
	WID_KEYSTROKEWND = 85,
	WID_EMOTIONWND = 86,
	WID_EMOTIONLISTWND = 87,
	WID_PETINFOWND = 88,
	WID_TOPRANKWND = 89,
	WID_SELECTPETEGGWND = 90,
	WID_PETTAMINGDECEIVEWND = 91,
	WID_CNTWND = 92,
	WID_NOTICEWND = 93,
	WID_MAKINGARROWLISTWND = 94,
	WID_SELECTCARTWND = 95,
	WID_MINIGAMEWND = 96,
	WID_SELECTMINIGAMEWND = 97,
	WID_MINIGAMESCOREWND = 98,
	WID_SPELLLISTWND = 99,
	WID_NPCTEXTEDITDIALOGWND = 100,
	WID_GRAFFISTRBOXWND = 101,
	WID_SHOWNEEDITEMLISTWND = 102,
	WID_PROHIBITLISTWND = 103,
	WID_COUPLEACCEPTWND = 104,
	WID_BABYACCEPTWND = 105,
	WID_BOOKWND = 106,
	WID_ITEMREPAIRWND = 107,
	WID_SYSCHECKWND = 108,
	WID_JOINFRIENDACCEPTWND = 109,
	WID_BABYACCEPTWND2 = 110,
	WID_WEAPONREFINEWND = 111,
	WID_COMMANDLISTWND = 112,
	WID_HOMUNINFOWND = 113,
	WID_HOMUNSKILLLISTWND = 114,
	WID_STOREPASSWORDINPUTWND = 115,
	WID_AUTOMESSAGEWND = 116,
	WID_MAILLISTWND = 117,
	WID_MAILVIEWWND = 118,
	WID_STARPLACE_ACCEPTWND = 119,
	WID_AUCTIONWND = 120,
	WID_AUCTIONMSGWND = 121,
	WID_IMGMENUWND = 122,
	WID_GRON_MESSENGERWND = 123,
	WID_GRON_OPTIONWND = 124,
	WID_GRON_ALERTWND = 125,
	WID_GRON_MESSAGEWND = 126,
	WID_GRON_SETTINGWND = 127,
	WID_GRON_MEMOWND = 128,
	WID_GRON_BARWND = 129,
	WID_PTNUMWND = 130,
	WID_PTACINFOWND = 131,
	WID_MERINFOWND = 132,
	WID_MERSKILLLISTWND = 133,
	WID_SELCHARFORUSERVERWND = 134,
	WID_CHANGENAMEWND = 135,
	WID_REBIRTHWND = 136,
	WID_SUBCHATWND_ST = 137,
	WID_SUBCHATWND_BT = 138,
	WID_BATTLEMSG_OPTIONWND = 139,
	WID_SUBCHAT_MINIWND = 140,
	WID_QUESTWND = 141,
	WID_QUEST_DETAIL_WND = 142,
	WID_ROSHOPWND = 143,
	WID_MEMORIAL_DUN_WND = 144,
	WID_YOURITEMWND = 145,
	WID_OTHERUSER_EQUIPWND = 146,
	WID_ROMAPWND = 147,
	WID_ADMINTOOLWND = 148,
	WID_ADMINTOOL_MINIWND = 149,
	WID_NOTIFYQUESTWND = 150,
	WID_LAST = 151,
};


enum {
	MENUID_EXPEL,
	MENUID_GIVE_OWNER,
	MENUID_CHANGE_ROOM_SETTING,
	MENUID_QUERYINFO,
	MENUID_DEAL,
	MENUID_INVITE_IN_PARTY,
	MENUID_SEND_MSG,
	MENUID_TALKONLYTWO,
	MENUID_QUERYPOS,
	MENUID_LEAVE,
	MENUID_ADD_TO_FRIEND_CHAR_LIST,
	MENUID_WRITE_CHAT_TO_FILE,
	MENUID_REQ_EX_LIST,
	MENUID_REQ_IN,
	MENUID_CHANGE_SKILL_NAME,
	MENUID_BREAK_GUILD,
	MENUID_REMOVE_GUILE_MEMBER,
	MENUID_LEAVE_GUILD,
	MENUID_VIEW_GUILD_MEMBER_INFO,
	MENUID_DELETE_WHISPER_NAME,
	MENUID_MAKE_WHISPER_WINDOW,
	MENUID_ADD_TO_FRIEND_CHAR_LIST_FROM_WINDOW,
	MENUID_INVITE_IN_GUILD,
	MENUID_REQ_MANNER_POINT_PLUS,
	MENUID_REQ_MANNER_POINT_MINUS,
	MENUID_ALLY_GUILD,
	MENUID_HOSTILE_GUILD,
	MENUID_DELETE_RELATED_GUILD,
	MENUID_DISCONNECT_CHARACTER,
	MENUID_PET_FEEDING,
	MENUID_PET_PERFORMANCE,
	MENUID_PET_RETURN_EGG,
	MENUID_PET_ACC_OFF,
	MENUID_PET_SHOWINFO,
	MENUID_COPY_GID,
	MENUID_SHOW_CHAT_PROHIBIT_LOG,
	MENUID_INVITE_IN_BABY,
	MENUID_HOMUN_SHOWINFO,
	MENUID_HOMUN_FEEDING,
	MENUID_HOMUN_FOLLOW,
	MENUID_MER_SHOWINFO,
	MENUID_MER_FOLLOW,
	MENUID_VIEW_OTHERUSER_EQUIPWND,
};
enum {
	WAITING,
	READY,
	DUN_IN,
	DUN_OUT,
};

class UIFrameWnd : public UIWindow {
public:
	UIFrameWnd();
	virtual ~UIFrameWnd();
	virtual void OnLBtnDown(int x, int y);
	virtual void OnMouseMove(int x, int y);
	virtual void OnMouseHover(int x, int y);
	virtual void OnLBtnUp(int x, int y);
	virtual void Move(int x, int y);
	virtual bool IsFrameWnd() { return true; }
	virtual void SaveOriginalPos();
	virtual void MoveDelta(int x, int y);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void RefreshSnap();
protected:
	void DrawItem(int x, int y, const char* iteminfo, bool forceidentified);
	void DrawItem(int x, int y, const ITEM_INFO& iteminfo, bool forceidentified);
	void DrawItemWithCount(int x, int y, const ITEM_INFO& iteminfo, bool forceidentified, bool boldtext);
	void DrawItemFrame(int x, int y);
	void DrawLatticeFrame(int x, int y, int width, int height);
	int MakeItemText(const ITEM_INFO& iteminfo, unsigned int& manufacturerColor, std::vector<char const *>& itemNameList, char*& itemNameIter, char*& itemNameColored, bool bIsColorList);
	int TextOutWithItemRect(int x, int y, int width, std::vector<char const *>& itemNameList, char* itemName, char* isColored, unsigned int itemNameColored, unsigned int manufacturerColor, int color, int fontType);
	int DrawItemText(int x, int y, const ITEM_INFO& iteminfo, unsigned int color, int fontType, int fontHeight);
	int DrawItemTextRect(int x, int y, int width, const ITEM_INFO& iteminfo, unsigned int color, int fontType, int fontHeight);
	int DrawItemObtainTextRect(int x, int y, int width, const ITEM_INFO& iteminfo, unsigned int color, int fontType, int fontHeight);
protected:
	int m_startGlobalX;
	int m_startGlobalY;
	int m_orgX;
	int m_orgY;
	int m_defPushId;
	int m_defCancelPushId;
};

class UIWaitWnd : public UIFrameWnd {
public:
	UIWaitWnd();
	virtual ~UIWaitWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	void SetMsg(const char* waitMsg, int fontHeight, int fontType);
protected:
	UITextViewer* m_textViewer;
	UIBitmapButton* m_pctrCancel;
};

class UIEmailAddressWnd : public UIFrameWnd {
public:
	UIEmailAddressWnd();
	virtual ~UIEmailAddressWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UIEditCtrl* m_editCtrl;
};

class UINoticeConfirmWnd : public UIFrameWnd {
public:
	UINoticeConfirmWnd();
	virtual ~UINoticeConfirmWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UITextViewer* m_textViewer;
	int m_target;
};

struct StringId {
public:
	StringId() {}
	StringId(const char* name, int value) : itemName(name), id(value) {}
	std::string itemName;
	int id;
};

class UIWhisperWnd : public UIFrameWnd {
public:
	UIWhisperWnd();
	virtual ~UIWhisperWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnRBtnUp(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
	virtual void Move(int cx, int cy);
	void WriteChatToFile();
	void WriteChatToReportFile();
	std::string m_whisperName;
	std::string m_whisperTitle;
protected:
	UIResizer* m_resizer;
	UIChatEditCtrl* m_chatEdit;
	UIChatHisBox2* m_listBox;
	UIBitmapButton* m_sysButton;
	std::vector<StringId> m_menuItems;
	WhisperWndInfo* m_info;
};

class UIGraffiStrboxWnd : public UIFrameWnd {
public:
	UIGraffiStrboxWnd();
	virtual ~UIGraffiStrboxWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	int m_skillId;
	int m_gSkillDx;
	int m_gSkillDy;
	int m_skillUseLevel;
	UIEditCtrl* m_trapText;
};

class UIItemIdentifyWnd : public UIFrameWnd {
public:
	UIItemIdentifyWnd();
	virtual ~UIItemIdentifyWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnUp(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void OnWheel(int wheel);
	virtual void StoreInfo();
	void RecalcScrbarPos();
	int GetCurItem(int x, int y);
	ITEM_INFO GetItemInfo(int __$ReturnUdt, int& curItem);
protected:
	UIScrollBar* m_scrollBar;
	int m_viewOffset;
	int m_curItem;
	int m_itemHorzNum;
	int m_itemVertNum;
	int m_vertScrEnabled;
	std::list<ITEM_INFO> m_itemInfoList;
	std::string m_title;
private:
	ItemIdentifyWndInfo* m_itemIdentifyWndInfo;
};

class UIItemCompositionWnd : public UIItemIdentifyWnd {
public:
	UIItemCompositionWnd();
	virtual ~UIItemCompositionWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
protected:
	ItemCompositionWndInfo* m_itemCompositionWndInfo;
	int m_cardItemIndex;
};

class UIComboBoxWnd : public UIFrameWnd {
public:
	UIComboBoxWnd();
	virtual ~UIComboBoxWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void Resize(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UIComboBox* m_comboBox;
};

class UIExchangeWnd : public UIFrameWnd {
public:
	UIExchangeWnd();
	virtual ~UIExchangeWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void OnMouseHover(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	ITEM_INFO GetItemInfo(int __$ReturnUdt, int x);
	void InvalidateButton();
	void RemoveMoneyCtrls();
protected:
	UIBitmapButton* m_buttons[3];
	UIEditCtrl* m_editCtrl;
	int m_IsMyDragClosed;
	int m_IsOtherDragClosed;
	int m_okPushed;
	int m_exchangePushed;
	int m_cancelPushed;
	std::list<ITEM_INFO> m_myItemList;
	std::list<ITEM_INFO> m_otherItemList;
	std::string m_exchangeCharacterName;
	int m_exchangeCharacterID;
	short m_exchangeCharacterLV;
};

class UIChooseSellBuyWnd : public UIFrameWnd {
public:
	UIChooseSellBuyWnd();
	virtual ~UIChooseSellBuyWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	unsigned int m_naid;
	UITextViewer* m_textViewer;
};

class UIExchangeAcceptWnd : public UIChooseSellBuyWnd {
public:
	UIExchangeAcceptWnd();
	virtual ~UIExchangeAcceptWnd();
	virtual void OnCreate(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
protected:
	ExchangeAcceptWndInfo* m_info;
};

class UIQuestDetailWnd : public UIFrameWnd {
public:
	UIQuestDetailWnd();
	virtual ~UIQuestDetailWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UITextViewer* m_textViewer;
	UITextViewer* m_textViewer2;
	std::string m_skillBmp;
	std::string m_skillName;
	std::string m_title;
	int m_questID;
	char m_mobCount;
	char m_mobCurrent;
	int m_mobID[3];
	short m_mobHunt[3];
	short m_mobTime_Year;
	short m_mobTime_Month;
	short m_mobTime_Day;
	short m_mobTime_Hour;
	short m_mobTime_Minute;
	short m_Server_Hour;
	short m_Server_Minute;
	UIStaticText* m_classText;
	std::vector<std::string> m_classStringList;
	UIStaticText* m_classText2;
	UIStaticText* m_classText3;
};

class UIMenuWnd : public UIFrameWnd {
public:
	UIMenuWnd();
	virtual ~UIMenuWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnUp(int cx, int cy);
	virtual void OnDraw();
	virtual void OnMouseMove(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void Move(int cx, int cy);
	void AddMenu(const char* menuString);
	int GetMenuIndex(int x, int y);
	int GetNumBoundary(int seq);
	int IsBoundary(int seq);
protected:
	WINDOWID m_callerWid;
	int m_curMenuIndex;
	int m_menuItemSpacing;
	std::vector<std::string> m_menuItems;
#if defined(BF_PORT)
	friend struct BfUiProbe;   // port/compat/stubs/bf_client_state.cpp: the control socket's `windows`
#endif
};

class UIRestartWnd : public UIFrameWnd {
public:
	UIRestartWnd();
	virtual ~UIRestartWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UITextViewer* m_textViewer;
};

class UIOptionWnd : public UIFrameWnd {
public:
	UIOptionWnd();
	virtual ~UIOptionWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
protected:
	int m_orgHeight;
	OptionWndInfo* m_info;
	UIScrollBar* m_bgmVolumescrBar;
	UIScrollBar* m_soundVolumescrBar;
	int m_soundVolume;
	int m_bgmVolume;
	UICheckBox* m_soundOncheckBox;
	UICheckBox* m_bgmOncheckBox;
	UICheckBox* m_attackSnapCheckBox;
	UICheckBox* m_skillSnapCheckBox;
	UICheckBox* m_itemSnapCheckBox;
	UICheckBox* m_noCtrlCheckBox;
	UINoneVisibleButton* m_skinSelectButton;
#if defined(BF_PORT)
	friend struct BfUiProbe;   // port/compat/stubs/bf_client_state.cpp: the control socket's `windows`
#endif
};

class UIItemParamChangeDisplayWnd : public UIFrameWnd {
public:
	UIItemParamChangeDisplayWnd();
	virtual ~UIItemParamChangeDisplayWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
	void RecalcEquipedParam();
	ITEM_INFO GetSlotItem(int __$ReturnUdt);
protected:
	int m_atk;
	int m_newAtk;
	int m_def;
	int m_newDef;
	int m_selectedAtk;
	int m_selectedDef;
	int m_sameSlotAtk;
	int m_sameSlotDef;
	std::string m_itemName;
	int m_isIdentified;
	ItemParamChangeDisplayWndInfo* m_info;
};

class UIPartySettingWnd : public UIFrameWnd {
public:
	UIPartySettingWnd();
	virtual ~UIPartySettingWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnLBtnDown(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
private:
	static const bool s_bUseNewPacket;
protected:
	UIRadioBtn* m_radioExpDiv[2];
	int m_curExpDivBtn;
	UIRadioBtn* m_radioItemCollect[2];
	int m_curItemCollectBtn;
	UIRadioBtn* m_radioItemDiv[2];
	int m_curItemDivBtn;
	PartySettingWndInfo* m_info;
private:
	int m_nYOffset;
};

class UITipOfTheDayWnd : public UIFrameWnd {
public:
	UITipOfTheDayWnd();
	virtual ~UITipOfTheDayWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	void WriteToReg();
protected:
	UITextViewer* m_textViewer;
	int m_indexOfTip;
	int m_showTipsAtStartup;
};

class UIPetTamingDeceiveWnd : public UIFrameWnd {
public:
	UIPetTamingDeceiveWnd();
	virtual ~UIPetTamingDeceiveWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnProcess();
	virtual void OnLBtnDown(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
private:
	int m_delay;
	int m_curAct;
	int m_curMot;
	int m_monGID;
	int m_destPos;
	int m_maxDelay;
	unsigned int m_startTick;
	int m_isSelected;
};

class UIMakingArrowListWnd : public UIItemIdentifyWnd {
public:
	UIMakingArrowListWnd();
	virtual ~UIMakingArrowListWnd();
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	short m_mkType;
};

class UISelectCartWnd : public UIFrameWnd {
public:
	UISelectCartWnd();
	virtual ~UISelectCartWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnProcess();
	virtual void OnLBtnDown(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
private:
	UISpriteButton* m_sprButton[5];
};

class UIChatRoomMakeWnd : public UIFrameWnd {
public:
	UIChatRoomMakeWnd();
	virtual ~UIChatRoomMakeWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	std::string m_title;
	UIEditCtrl* m_roomTitle;
	UIEditCtrl* m_passWord;
	UIRadioBtn* m_radioBtns[2];
	UIStaticText* m_numText;
	UIStaticText* m_classText;
	UIBitmapButton* m_sysButton;
	int m_curRadioBtn;
	int m_comboMode;
	int m_numPeople;
	int m_classMode;
	std::vector<std::string> m_numStringList;
	std::vector<std::string> m_classStringList;
	std::vector<int> m_numList;
	std::vector<int> m_classModeList;
};

class UICommandListWnd : public UIFrameWnd {
public:
	UICommandListWnd();
	virtual ~UICommandListWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual void OnWheel(int wheel);
	virtual void RecalcScrbarPos();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	int GetCurItem(int x, int y);
protected:
	UITab* m_tab;
	UIScrollBar* m_scrollBar;
	int m_curItem;
	int m_itemHorzNum;
	int m_itemVertNum;
	int m_vertScrEnabled;
	int m_viewOffset;
};

class UIMailListWnd : public UIFrameWnd {
public:
	UIMailListWnd();
	virtual ~UIMailListWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR tmp, INT_PTR val2, INT_PTR val3);
	void ResetMailWrite(bool allInit);
	void SetType(int type) { m_type = type; }
	void SetType2(int type2) { m_type2 = type2; }
	int GetType2() { return m_type2; }
protected:
	void InitAllBtnPos();
protected:
	UITextButton* m_btnFromNameList[7];
	UITextButton* m_btnMailHeaderList[7];
	UITextButton* m_prevBtn;
	UITextButton* m_nextBtn;
	UIEditCtrl* m_receiveName;
	UIEditCtrl* m_MailHeader;
	UIEditCtrl* m_Zeny;
	UIEditCtrl3* m_MailContent;
	UIBitmapButton* m_inputZenyBtn;
	UIBitmapButton* m_setZenyBtn;
	UIBitmapButton* m_sendBtn;
	UIBitmapButton* m_cancelBtn;
	UIStaticText* m_Zeny2;
	int m_type;
	int m_type2;
	int m_page;
	int m_maxPage;
	int m_inputZeny;
	ITEM_INFO m_item;
};

class UIMailViewWnd : public UIFrameWnd {
public:
	UIMailViewWnd();
	virtual ~UIMailViewWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnMouseShape(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UIChatHisBox3* m_MailContent;
	UIBitmapButton* m_returnMailBtn;
	UIBitmapButton* m_reMailBtn;
	UIBitmapButton* m_delBtn;
	ITEM_INFO m_item;
	int m_zeny;
	int r;
	int g;
	int b;
};

class UIAuctionWnd : public UIFrameWnd {
public:
	UIAuctionWnd();
	virtual ~UIAuctionWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnLBtnUp(int cx, int cy);
	virtual void OnRBtnUp(int cx, int cy);
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	void InitItem() { ITEM_INFO itemInfo; m_item = itemInfo; }
protected:
	ITEM_INFO* GetCurAuctionItem(int index);
	ITEM_INFO* GetCurAuctionItem2(AUCTION_ITEM_SEARCH_INFO* auction);
	AUCTION_ITEM_SEARCH_INFO* GetCurAuction(int index);
	void InitAllBtnPos();
protected:
	UIBitmapButton2* m_itemBmpList[5];
	UITextButton* m_btnItemNameList[5];
	UITextButton* m_sellerNameList[5];
	UITextButton* m_buyerNameList[5];
	UIBitmapButton2* m_itemBmpBtn;
	UITextButton* m_itemNameBtn;
	UITextButton* m_userNameBtn;
	UIEditCtrl* m_nowMoney;
	UIEditCtrl* m_maxMoney;
	UIEditCtrl* m_deleteHour;
	UIEditCtrl* m_search;
	UIBitmapButton* m_addBtn;
	UIBitmapButton* m_cancelBtn;
	UIBitmapButton* m_finishBtn;
	UIBitmapButton* m_buyBtn;
	UIBitmapButton* m_searchBtn;
	UITextButton* m_prevBtn;
	UITextButton* m_nextBtn;
	UIRadioBtn* m_btnOrder[4];
	UIRadioBtn* m_btnSearch[2];
	int m_type;
	int m_page;
	int m_maxPage;
	unsigned char m_searchAuction;
	int m_curItem;
	AUCTION_ITEM_SEARCH_INFO* m_curAuctionItem;
	int m_curOrderBtn;
	int m_curSearchBtn;
	ITEM_INFO m_item;
#if defined(BF_PORT)
	friend struct BfUiProbe;   // port/compat/stubs/bf_client_state.cpp: the control socket's `windows`
#endif
};

class UIAuctionMsgWnd : public UIFrameWnd {
public:
	UIAuctionMsgWnd();
	virtual ~UIAuctionMsgWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UIBitmapButton* m_button;
	UIEditCtrl* m_Money;
	int m_type;
	int m_price;
	int m_auctionID;
	char itemName[25];
#if defined(BF_PORT)
	friend struct BfUiProbe;   // port/compat/stubs/bf_client_state.cpp: the control socket's `windows`
#endif
};

class UIImgMenuWnd : public UIFrameWnd {
public:
	UIImgMenuWnd();
	virtual ~UIImgMenuWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void Resize(int cx, int cy);
	virtual void OnDraw();
	virtual void Move(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UIImgMenu* m_imgMenu;
#if defined(BF_PORT)
	friend struct BfUiProbe;   // port/compat/stubs/bf_client_state.cpp: the control socket's `windows`
#endif
};

class UILoadingWnd : public UIFrameWnd {
public:
	UILoadingWnd();
	virtual ~UILoadingWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	void SetMsg(const char* waitMsg, int fontHeight, int fontType);
protected:
	UIStaticText* m_waitMsg;
	UIBarGraph2* m_barGraph;
};

class UIItemCollectionWnd : public UIFrameWnd {
public:
	UIItemCollectionWnd();
	virtual ~UIItemCollectionWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnMouseHover(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnProcess();
	int GetCardSlotNum(int x, int y, int& gx, int& gy);
protected:
	bool IsSameCollectionDisplayParameters(const ITEM_INFO& item_info);
protected:
	ITEM_INFO m_itemInfo;
	UITextViewer* m_textViewer;
	UIBitmapButton* m_cardItemButton;
	std::string m_itemBmp;
	std::string m_itemName;
	unsigned char m_isIdentified;
	unsigned char m_hasSlot;
	WINDOWID m_windowId;
private:
	ItemCollectionWndInfo* m_info;
public:
	short m_petLevel;
	short m_petFullness;
	short m_petRelationship;
	char m_petName[24];
	unsigned short m_petITID;
	int m_isPetNameModified;
};

class UICombinedCardItemCollectionWnd : public UIItemCollectionWnd {
public:
	UICombinedCardItemCollectionWnd();
	virtual ~UICombinedCardItemCollectionWnd();
	virtual void OnCreate(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
	CombinedCardItemCollectionWndInfo* m_combinedItemInfo;
};

class UISkillDescribeWnd : public UIFrameWnd {
public:
	UISkillDescribeWnd();
	virtual ~UISkillDescribeWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UIBitmapButton* sysButton;
	UITextViewer* m_textViewer;
	std::string m_skillBmp;
	std::string m_skillName;
	std::string m_title;
};

class UIQuitWnd : public UIFrameWnd {
public:
	UIQuitWnd();
	virtual ~UIQuitWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UICardItemIllustWnd : public UIFrameWnd {
public:
	UICardItemIllustWnd();
	virtual ~UICardItemIllustWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	std::string m_itemBmp;
	std::string m_itemName;
	unsigned char m_isIdentified;
};

class UIChatRoomChangeWnd : public UIChatRoomMakeWnd {
public:
	UIChatRoomChangeWnd();
	virtual ~UIChatRoomChangeWnd();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UIItemDropCntWnd : public UIFrameWnd {
public:
	UIItemDropCntWnd();
	virtual ~UIItemDropCntWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void SetShow(int visible);
protected:
	UIEditCtrl* m_editCtrl;
	UIStaticText* m_newText;
};

class UIPasswordWnd : public UIItemDropCntWnd {
public:
	UIPasswordWnd();
	virtual ~UIPasswordWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void SetShow(int visible);
protected:
	unsigned int m_roomId;
};

class UIJoinPartyAcceptWnd : public UIExchangeAcceptWnd {
public:
	UIJoinPartyAcceptWnd();
	virtual ~UIJoinPartyAcceptWnd();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UICoupleAcceptWnd : public UIExchangeAcceptWnd {
public:
	UICoupleAcceptWnd();
	virtual ~UICoupleAcceptWnd();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UIStarPlaceAcceptWnd : public UIExchangeAcceptWnd {
public:
	UIStarPlaceAcceptWnd();
	virtual ~UIStarPlaceAcceptWnd();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	char which;
};

class UIYourItemWnd : public UIExchangeAcceptWnd {
public:
	UIYourItemWnd();
	virtual ~UIYourItemWnd();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	int val_index;
	int val_wear;
};

class UIBabyAcceptWnd : public UIExchangeAcceptWnd {
public:
	UIBabyAcceptWnd();
	virtual ~UIBabyAcceptWnd();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UIBabyAcceptWnd2 : public UIExchangeAcceptWnd {
public:
	UIBabyAcceptWnd2();
	virtual ~UIBabyAcceptWnd2();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UIJoinGuildAcceptWnd : public UIExchangeAcceptWnd {
public:
	UIJoinGuildAcceptWnd();
	virtual ~UIJoinGuildAcceptWnd();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UISkillNameChangeWnd : public UIFrameWnd {
public:
	UISkillNameChangeWnd();
	virtual ~UISkillNameChangeWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UIEditCtrl* m_editCtrl;
	int m_skillId;
	std::string m_skillName;
};

class UINpcEditDialogWnd : public UIFrameWnd {
public:
	UINpcEditDialogWnd();
	virtual ~UINpcEditDialogWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UIEditCtrl* m_editCtrl;
	unsigned int m_naid;
#if defined(BF_PORT)
	friend struct BfUiProbe;   // port/compat/stubs/bf_client_state.cpp: the control socket's `windows`
#endif
};

class UINpcTextEditDialogWnd : public UINpcEditDialogWnd {
public:
	UINpcTextEditDialogWnd();
	virtual ~UINpcTextEditDialogWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UIDetailLevelWnd : public UIFrameWnd {
public:
	UIDetailLevelWnd();
	virtual ~UIDetailLevelWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
protected:
	int m_textureDetailLevel;
	int m_spriteDetailLevel;
	int m_trilinearFilter;
	UIScrollBar* m_spriteDetailLevelScrbar;
	UIScrollBar* m_textureDetailLevelScrbar;
	DetailLevelWndInfo* m_info;
	UICheckBox* m_openDetailWindowCheckBox;
	UICheckBox* m_trilinearCheckBox;
};

class UIProhibitListWnd : public UIFrameWnd {
public:
	UIProhibitListWnd();
	virtual ~UIProhibitListWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnProcess();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	int m_oldLogCnt;
	unsigned int m_AID;
};

class UIPetEggListWnd : public UIItemIdentifyWnd {
public:
	UIPetEggListWnd();
	virtual ~UIPetEggListWnd();
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UISpellListWnd : public UIItemIdentifyWnd {
public:
	UISpellListWnd();
	virtual ~UISpellListWnd();
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	std::vector<int> m_spellList;
};

class UIBookWnd : public UIFrameWnd {
public:
	UIBookWnd();
	virtual ~UIBookWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnProcess();
	virtual void OnLBtnDown(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	void WriteBookMarkToRegistry(const char* bookName, int page);
	int ReadBookMarkFromRegistry(const char* bookName);
	char* GetContents(const char* text);
protected:
	UITextViewer* m_textViewer;
	UITransBalloonText* m_bookcastBalloon;
	char m_BookTitle[100];
	int m_BookTotalLine;
	int m_BookTotalPage;
	int m_PresentPage;
	int m_PresentLine;
	int m_BookCastTick;
	unsigned char m_bAutoReadBook;
	unsigned int m_bgR;
	unsigned int m_bgG;
	unsigned int m_bgB;
	std::vector<std::string> m_bookContents;
	UISpriteButton* m_BookButton[4];
};

class UIJoinFriendAcceptWnd : public UIExchangeAcceptWnd {
public:
	UIJoinFriendAcceptWnd();
	virtual ~UIJoinFriendAcceptWnd();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	unsigned int m_nReqAID;
	unsigned int m_nReqGID;
};

class UIAutoMessageWnd : public UIFrameWnd {
public:
	UIAutoMessageWnd();
	virtual ~UIAutoMessageWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UIEditCtrl* m_editCtrl;
};

class UIMessageBoxAutoreturn : public UIFrameWnd {
public:
	UIMessageBoxAutoreturn();
	virtual ~UIMessageBoxAutoreturn();
	void Create(const char* msg, unsigned int autoReturnTime);
	virtual void OnProcess();
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
private:
	UITextViewer* m_textViewer;
	UIBarGraph2* m_barGraph;
	int m_isAutoReturn;
	unsigned int m_openTime;
	unsigned int m_autoReturnTime;
	int m_maxSecond;
	int m_remainSecond;
	std::string m_countMsg;
	std::string m_msg;
};

class UIMessageBox : public UIFrameWnd {
public:
	UIMessageBox();
	virtual ~UIMessageBox();
	void Create(const char* msg, int type, int isDefYes, unsigned int autoReturnTime);
	virtual void OnProcess();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void OnDestroy();
	virtual void OnDraw();
private:
	UITextViewer* m_textViewer;
	UIRechargeGage* m_barGraph;
	int m_isAutoReturn;
	unsigned int m_openTime;
	unsigned int m_autoReturnTime;
	int m_maxSecond;
	int m_remainSecond;
	std::string m_countMsg;
	std::string m_msg;
};

class UINoticeWnd : public UIFrameWnd {
public:
	UINoticeWnd();
	virtual ~UINoticeWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDestroy();
	virtual void OnDraw();
	virtual void OnWheel(int wheel);
	virtual void OnMouseHover(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	void PrintNotice();
	UIStaticText* m_NoticeMessage[21];
	int m_NoticeTotalLine;
	int m_NoticeDisplayLine;
	char* m_NoticeFileName;
	int m_TotalPage;
	int m_PresentPage;
	int m_PageStartLine[10];
	int m_PageLastLine[10];
	int m_PagePresentLine;
protected:
	UIScrollBar* m_scrollBar;
};

struct viewSprite {
public:
	int x;
	int y;
	std::string actName[5];
	std::string sprName[5];
	std::string imfName;
	int headPalette;
	int bodyPalette;
	std::string headPaletteName;
	std::string bodyPaletteName;
	int baseAction;
	int curAction;
	int curMotion;
};

class UIMakeCharWnd : public UIFrameWnd {
public:
	UIMakeCharWnd();
	virtual ~UIMakeCharWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnProcess();
	virtual void OnDestroy();
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	void MakeButton();
	void InitTextControls();
	void DrawHexagon();
	UIBitmapEditCtrl* m_nameEditCtrl;
	CHARACTER_INFO m_charInfo;
private:
	viewSprite m_viewChar;
	UIStaticText* m_text[6];
#if defined(BF_PORT)
	friend struct BfUiProbe;   // port/compat/stubs/bf_client_state.cpp: the control socket's `windows`
#endif
};

class UIChatWnd : public UIFrameWnd {
public:
	UIChatWnd();
	virtual ~UIChatWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnMouseMove(int cx, int cy);
	virtual void OnLBtnUp(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual bool GetTransBoxInfo(BOXINFO* boxInfo);
	virtual void Move(int cx, int cy);
	virtual void Resize(int cx, int cy);
	virtual void StoreInfo();
	virtual void SetShow(int state);
	virtual void OnDraw();
	virtual void OnDraw2();
	void WriteChatToFile();
	void WriteChatToReportFile();
	void SetState(int state);
	int GetState();
	UIResizer* m_resizer;
	UIEditCtrl* m_lastFocusChat;
	UIWhisperEditCtrl* m_whisperChat;
	UIChatEditCtrl* m_commonChat;
	UIBitmapButton* m_buttons[3];
	UIChatHisBox* m_listBox;
	BOXINFO m_boxInfo;
	int m_comboMode;
};

class UIMonsterInfoWnd : public UIFrameWnd {
public:
	UIMonsterInfoWnd();
	virtual ~UIMonsterInfoWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	void DrawPropertyBox(int x, int y, const char* propertyString, int power, unsigned int borderColor, unsigned int surfaceColor);
protected:
	PACKET_ZC_MONSTER_INFO m_zc_monster_info;
};

class UITalkboxTrapInputWnd : public UIFrameWnd {
public:
	UITalkboxTrapInputWnd();
	virtual ~UITalkboxTrapInputWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	int m_skillId;
	int m_gSkillDx;
	int m_gSkillDy;
	int m_skillUseLevel;
	UIEditCtrl* m_trapText;
};

class UICntWnd : public UIFrameWnd {
public:
	UICntWnd();
	virtual ~UICntWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	int m_target;
	UIEditCtrl* m_editCtrl;
	UIStaticText* m_newText;
};

class UIChatRoomWnd : public UIFrameWnd {
public:
	UIChatRoomWnd();
	virtual ~UIChatRoomWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR isMyName, INT_PTR val2, INT_PTR val3);
	virtual void StoreInfo();
	virtual void Move(int cx, int cy);
	int GetRoomId() { return m_roomInfo.roomId; }
	void WriteChatToFile();
	void WriteChatToReportFile();
	ChatRoomInfo m_roomInfo;
protected:
	UIResizer* m_resizer;
	UIChatEditCtrl* m_chatEdit;
	UIChatHisBox2* m_listBox;
	UIChatHisBox2* m_memberListBox;
	UIBitmapButton* m_sysButton;
	UIStaticText* m_title;
	std::map<std::string, unsigned int> m_memberList;
	std::vector<StringId> m_menuItems;
	ChatRoomWndInfo* m_info;
};

class UIPetInfoWnd : public UIFrameWnd {
public:
	UIPetInfoWnd();
	virtual ~UIPetInfoWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void StoreInfo();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	UIEditCtrl2* m_newName;
	PetInfoWndInfo* m_info;
	UIBitmapButton* m_button;
	UINoneVisibleButton* m_skinSelectButton;
};

class UIHomunInfoWnd : public UIFrameWnd {
public:
	UIHomunInfoWnd();
	virtual ~UIHomunInfoWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void StoreInfo();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	UIEditCtrl2* m_newName;
	UIBitmapButton* m_button;
	UIBarGraphPlayerHp* m_hpGraph;
	UIBarGraphPlayerSp* m_spGraph;
	UIBarGraph4* m_expGraph;
	UIBarGraph4* m_hungryGraph;
	HomunInfoWndInfo* m_info;
};

class UIMerInfoWnd : public UIFrameWnd {
public:
	UIMerInfoWnd();
	virtual ~UIMerInfoWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void StoreInfo();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	UIEditCtrl2* m_newName;
	UIBitmapButton* m_button;
	UIBarGraphPlayerHp* m_hpGraph;
	UIBarGraphPlayerSp* m_spGraph;
	MerInfoWndInfo* m_info;
};

class UISelectCharWnd : public UIFrameWnd {
public:
	UISelectCharWnd();
	virtual ~UISelectCharWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnProcess();
	virtual void OnMouseMove(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void InvalidateWallPaper();
	void MakeButton(int id);
	void DeleteButton(int id);
	void WriteToReg();
	void InitTextControls();
	void InitFromReg();
private:
	viewSprite m_viewChar[12];
	int m_isEmpty[12];
	unsigned char m_isAvailable[12];
	short m_accessory_top[12];
	unsigned int m_stateStartTick;
	char m_dontmove;
	int m_curPage;
	int m_curSlot;
	int m_pageCount;
	UIBitmapButton* m_okButton;
	UIBitmapButton* m_makeButton;
	UIBitmapButton* m_cancelButton;
	UIBitmapButton* m_deleteButton;
	UIBitmapButton* m_noticeButton;
	UIBitmapButton* m_chargeButton;
	UIStaticText* m_text[22];
	UISlotBitmap* m_slots[13];
	UIBitmapButton* m_changeNameBtn;
#if defined(BF_PORT)
	friend struct BfUiProbe;   // port/compat/stubs/bf_client_state.cpp: the control socket's `windows`
#endif
};

class UILoginWnd : public UIFrameWnd {
public:
	UILoginWnd();
	virtual ~UILoginWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnDestroy();
	virtual void StoreInfo();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	void InitFromReg();
	void WriteToReg();
	void OpenRegistPage();
	UIEditCtrl* m_login;
	UIEditCtrl* m_password;
	UIBitmapButton* m_cancelButton;
	int m_isCheckOn;
	int m_isGravity;
	int m_isHangame;
	UIStaticText* m_classText;
	std::vector<std::string> m_classStringList;
	int m_comboMode;
};

class UIAllyGuildAcceptWnd : public UIExchangeAcceptWnd {
public:
	UIAllyGuildAcceptWnd();
	virtual ~UIAllyGuildAcceptWnd();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UISelectServerWnd : public UIFrameWnd {
public:
	UISelectServerWnd();
	virtual ~UISelectServerWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void OnDestroy();
	void InitDefaultCurItemFromReg(const char* regkey);
	void SaveCurItemToReg(const char* regkey);
	UIServerListBox* m_serverList;
private:
	UIBitmapButton* m_exitButton;
	std::string m_regKey;
	int m_defaultCurItem;
	int m_target;
};

class UIItemBaseWnd : public UIFrameWnd {
public:
	UIItemBaseWnd();
	virtual ~UIItemBaseWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void StoreInfo();
	virtual void OnDraw();
	virtual void OnWheel(int wheel);
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	void RecalcScrbarPos();
protected:
	ITEM_INFO GetItemInfo(int __$ReturnUdt);
	ITEM_INFO GetItemInfo(int __$ReturnUdt, int x);
	int GetCurItem(int x, int y);
protected:
	int m_drawNumItem;
	int m_isBasket;
	UIResizer* m_resizer;
	UIScrollBar* m_scrollBar;
	UIBitmapButton* m_button[2];
	int m_viewOffset;
	int m_itemVertNum;
	int m_itemHorzNum;
	int m_vertScrEnabled;
	std::list<ITEM_INFO> m_itemList;
#if defined(BF_PORT)
	friend struct BfUiProbe;   // port/compat/stubs/bf_client_state.cpp: the control socket's `windows`
#endif
};

class UIItemShopWnd : public UIItemBaseWnd {
public:
	UIItemShopWnd();
	virtual ~UIItemShopWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void StoreInfo();
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual void OnDraw();
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	bool IsExistItem(const char* itemName);
protected:
	UIStaticText* m_titleText;
	UICheckBox* m_dragAllCheckBox;
	int m_isSellMode;
	ItemShopWndInfo* m_info;
	int m_curItem;
	int m_barHeight;
	int m_barHeight2;
	int m_isPointShop;
};

class UIItemPurchaseWnd : public UIItemBaseWnd {
public:
	UIItemPurchaseWnd();
	virtual ~UIItemPurchaseWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual void OnDraw();
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR enoughMoney, INT_PTR val2, INT_PTR val3);
	virtual void StoreInfo();
protected:
	UIStaticText* m_priceText;
	ItemPurchaseWndInfo* m_info;
	int m_curItem;
	int m_isPointShop;
	UIEditCtrl* m_freePoint;
};

class UIItemSellWnd : public UIItemBaseWnd {
public:
	UIItemSellWnd();
	virtual ~UIItemSellWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual void OnDraw();
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
protected:
	UIStaticText* m_priceText;
	ItemSellWndInfo* m_info;
	int m_curItem;
};

class UIItemStoreWnd : public UIItemBaseWnd {
public:
	UIItemStoreWnd();
	virtual ~UIItemStoreWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void StoreInfo();
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	ITEM_INFO GetItemInfo(int __$ReturnUdt, int x);
public:
	bool Sort(const ITEM_INFO* const itemInfo);
	void RefreshRadioBtn();
protected:
	std::string m_countInfoText;
	UIStaticText* m_titleText;
	ItemStoreWndInfo* m_info;
	UITab* m_tab;
	int m_curRadioBtn[3];
	int m_curTab;
};

class UIMerchantItemWnd : public UIItemBaseWnd {
public:
	UIMerchantItemWnd();
	virtual ~UIMerchantItemWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
protected:
	MerchantItemWndInfo* m_info;
	UIBitmapButton* m_sysButtons[1];
	UIStaticText* m_supplyText;
};

class UIMerchantShopMakeWnd : public UIFrameWnd {
public:
	UIMerchantShopMakeWnd();
	virtual ~UIMerchantShopMakeWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void StoreInfo();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR isFull, INT_PTR val2, INT_PTR val3);
	virtual void OnWheel(int wheel);
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual void OnLBtnDown(int cx, int cy);
	void RecalcScrbarPos();
	ITEM_INFO GetItemInfo(int __$ReturnUdt, int x);
protected:
	std::string m_title;
	UIEditCtrl* m_shopTitle;
	UIBitmapButton* m_upButton;
	UIBitmapButton* m_downButton;
	UIEditCtrl* m_itemCountEdit[13];
	int m_numItemCountEdit;
	UIScrollBar* m_scrollBar;
	int m_viewOffset;
	int m_itemHorzNum;
	int m_itemVertNum;
	int m_vertScrEnabled;
	std::list<ITEM_INFO> m_itemList;
	MerchantShopMakeWndInfo* m_info;
};

class UIMerchantMirrorItemWnd : public UIItemBaseWnd {
public:
	UIMerchantMirrorItemWnd();
	virtual ~UIMerchantMirrorItemWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
protected:
	MerchantMirrorItemWndInfo* m_info;
};

class UIMerchantItemShopWnd : public UIItemBaseWnd {
public:
	UIMerchantItemShopWnd();
	virtual ~UIMerchantItemShopWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual void OnDraw();
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
protected:
	UIStaticText* m_titleText;
	MerchantItemShopWndInfo* m_info;
	int m_curItem;
	int m_aid;
};

class UIMerchantItemMyShopWnd : public UIItemBaseWnd {
public:
	UIMerchantItemMyShopWnd();
	virtual ~UIMerchantItemMyShopWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
protected:
	UIStaticText* m_titleText;
	MerchantItemMyShopWndInfo* m_info;
};

class UIMerchantItemPurchaseWnd : public UIItemBaseWnd {
public:
	UIMerchantItemPurchaseWnd();
	virtual ~UIMerchantItemPurchaseWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual void OnDraw();
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
protected:
	UIStaticText* m_priceText;
	unsigned int m_aid;
	MerchantItemPurchaseWndInfo* m_info;
	int m_curItem;
};

class UIMiniBtnWnd : public UIFrameWnd {
public:
	UIMiniBtnWnd();
	virtual ~UIMiniBtnWnd();
	virtual void OnDraw();
	virtual void OnLBtnDown(int x, int y);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	void SetBtnImg(CBitmapRes*, CBitmapRes*);
	void SetWndID(WINDOWID);
	CBitmapRes* m_OnImgRes;
	CBitmapRes* m_OffImgRes;
	WINDOWID m_wndID;
};

class UINewChatWnd : public UIFrameWnd {
public:
	UINewChatWnd();
	virtual ~UINewChatWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnDraw2();
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnMouseMove(int cx, int cy);
	virtual void OnLBtnUp(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual bool GetTransBoxInfo2(BOXINFO* boxInfo);
	virtual void DrawBoxScreen2();
	virtual void Move(int cx, int cy);
	virtual void Resize(int cx, int cy);
	virtual void StoreInfo();
	virtual void SetShow(int state);
	void SetState(int state);
	int GetState();
	void WriteChatToFile();
	void WriteChatToReportFile();
	void ChangeTab();
	void StickOnOff(bool stickOn);
	UIEditCtrl* m_lastFocusChat;
	UIResizer* m_resizer;
	UIChatEditCtrl* m_commonChat;
	UIWhisperEditCtrl* m_whisperChat;
	BOXINFO m_boxInfo;
	UIBitmapButton* m_buttons[4];
	int m_comboMode;
	UIChatHisBox* m_listBox;
	UIChatHisBox* m_cmdListBox;
	int m_tab;
	int m_saveState;
	int m_titleHeight;
	int m_titleWidth;
	int m_chatEditHeight;
	UIBitmapButton* m_btnStickOff;
	UIBitmapButton* m_btnOption;
	unsigned int m_colorTab1;
	unsigned int m_colorTab2;
};

class UIChangeNameWnd : public UIFrameWnd {
public:
	UIChangeNameWnd();
	virtual ~UIChangeNameWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
private:
	UIBitmapButton* m_okButton;
	UIBitmapButton* m_cancelButton;
	UIBitmapEditCtrl* m_nameEditCtrl;
	CHARACTER_INFO* m_charInfo;
	CHARACTER_INFO g_charInfo;
};

class UISubChatHisWnd : public UIFrameWnd {
public:
	UISubChatHisWnd();
	virtual ~UISubChatHisWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnDraw2();
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnMouseMove(int cx, int cy);
	virtual void OnLBtnUp(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual bool GetTransBoxInfo2(BOXINFO* boxInfo);
	virtual void DrawBoxScreen2();
	virtual void Move(int cx, int cy);
	virtual void Resize(int cx, int cy);
	virtual void StoreInfo();
	void SetMsgListBox();
	void DeleteFirstHalfItem();
	SubChatWndInfo* m_info;
	UIResizer* m_resizer;
	BOXINFO m_boxInfo;
	UIChatHisBox* m_listBox;
	UIBitmapButton* m_btnStickOn;
	UIBitmapButton* m_btnOption;
	UIBitmapButton* m_btnMini;
	std::vector<std::string> m_chatHistory;
	std::vector<unsigned int> m_colors;
	std::vector<std::string> m_talkerNameList;
	int m_titleHeight;
	int m_titleWidth;
	int m_barHeight;
};

class UISubChatMiniWnd : public UIFrameWnd {
public:
	UISubChatMiniWnd();
	virtual ~UISubChatMiniWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnLBtnDown(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	int m_num;
	int m_miniWidth;
	int m_miniHeight;
};

class UIMemorialDunWnd : public UIFrameWnd {
public:
	UIMemorialDunWnd();
	virtual ~UIMemorialDunWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void Move(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	void RefreshInfo();
	void InitInfo();
protected:
	UIBitmapButton* m_buttons[2];
};

class UISelCharForUServerWnd : public UIFrameWnd {
public:
	UISelCharForUServerWnd();
	virtual ~UISelCharForUServerWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnMouseMove(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	CHARACTER_INFO* GetCharInfo(int x, int y);
	void ShowCharInfoBalloon(int x, int y, unsigned int fntColor);
private:
	viewSprite m_viewChar[12];
	int m_isEmpty[12];
	short m_accessory_top[12];
	unsigned int m_stateStartTick;
	UICheckBox* m_checkBox[12];
	UIBitmapButton* m_okButton;
	UICharInfoBalloonText* m_charInfoBalloon;
	std::vector<std::string> m_strings;
	int m_selCharNum;
};

class UIRebirthWnd : public UIFrameWnd {
public:
	UIRebirthWnd();
	virtual ~UIRebirthWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UIBattleMsgOptionWnd : public UIFrameWnd {
public:
	UIBattleMsgOptionWnd();
	virtual ~UIBattleMsgOptionWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual void OnWheel(int wheel);
	virtual void RecalcScrbarPos();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	int GetCurItem(int x, int y);
	void WriteToReg();
protected:
	UIScrollBar* m_scrollBar;
	int m_curItem;
	int m_itemVertNum;
	int m_vertScrEnabled;
	int m_viewOffset;
	std::vector<char *> m_battleMsgOptionTable;
};

class UIBasicInfoWnd : public UIFrameWnd {
public:
	enum HEIGHT_TYPE {
		MINI_SIZE = 34,
		FULL_SIZE = 120,
	};

	UIBasicInfoWnd();
	virtual ~UIBasicInfoWnd();
	virtual bool IsUpdateNeed() { return true; }
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
	void NewHeight(int newHeight);
protected:
	UIStaticText* m_name;
	BasicInfoWndInfo* m_info;
	UIBitmapButton* m_buttons[8];
	UIBarGraphPlayerHp* m_hpGraph;
	UIBarGraphPlayerSp* m_spGraph;
	UIBarGraph4* m_expGraph;
	UIBarGraph4* m_jobexpGraph;
};

class UISkillListWnd : public UIFrameWnd {
public:
	UISkillListWnd();
	virtual ~UISkillListWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnRBtnUp(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR count, INT_PTR val2, INT_PTR val3);
	virtual void StoreInfo();
	virtual void OnWheel(int wheel);
	virtual bool IsUpdateNeed() { return true; }
	virtual void OnDraw();
	void RecalcScrbarPos();
	void RefreshButton();
	SKILL_INFO GetSkillInfoByViewCnt(int __$ReturnUdt);
protected:
	SKILL_INFO GetSkillInfo(int __$ReturnUdt);
	SKILL_INFO GetSkillInfo(int __$ReturnUdt, int x);
	int GetCurItem(int x, int y);
protected:
	UIResizer* m_resizer;
	UIScrollBar* m_scrollBar;
	UIBitmapButton* m_button[2];
	UIBitmapButton* m_incArrowButton[7];
	UIBitmapButton* m_lvlSelectDnButton[7];
	UIBitmapButton* m_lvlSelectUpButton[7];
	int m_curItem;
	int m_viewOffset;
	int m_itemHorzNum;
	int m_itemVertNum;
	int m_vertScrEnabled;
	std::list<SKILL_INFO> m_itemList;
	SkillListWndInfo* m_info;
	std::vector<StringId> m_menuItems;
public:
	int m_typeWnd;
#if defined(BF_PORT)
	friend struct BfUiProbe;   // port/compat/stubs/bf_client_state.cpp: the control socket's `windows`
#endif
};

class UIItemWnd : public UIItemBaseWnd {
public:
	UIItemWnd();
	virtual ~UIItemWnd();
	virtual bool IsUpdateNeed() { return true; }
	virtual void OnCreate(int cx, int cy);
	virtual void OnDestroy();
	virtual void OnDraw();
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual void StoreInfo();
	bool Sort(const ITEM_INFO* const itemInfo);
	void RefreshRadioBtn();
protected:
	ITEM_INFO GetItemInfo(int __$ReturnUdt, int x);
protected:
	ItemWndInfo* m_info;
	UITab* m_tab;
	UIBitmapButton* m_sysButtons[3];
	int m_curRadioBtn[3];
	int m_curTab;
	int m_orgHeight;
};

class UIQuestWnd : public UIItemBaseWnd {
public:
	UIQuestWnd();
	virtual ~UIQuestWnd();
	virtual bool IsUpdateNeed() { return true; }
	virtual void OnCreate(int cx, int cy);
	virtual void OnDestroy();
	virtual void OnDraw();
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
	void RefreshRadioBtn();
	void RecalcScrbarPos();
protected:
	ITEM_INFO GetItemInfo(int, int);
protected:
	QuestWndInfo* m_info;
	UITab* m_tab;
	UIBitmapButton* m_sysButtons[3];
	UIBitmapButton* m_button[2];
	int m_curTab;
	int m_orgHeight;
	int m_curQuest;
};

class UIToolTipWnd : public UIFrameWnd {
public:
	UIToolTipWnd();
	virtual ~UIToolTipWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw() { DrawLatticeFrame(0, 0, 0, 0); }
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UIListBox* m_toolTipList;
};

class UIShortCutWnd : public UIFrameWnd {
public:
	UIShortCutWnd();
	virtual ~UIShortCutWnd();
	virtual bool IsUpdateNeed() { return true; }
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnProcess();
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual void StoreInfo();
	virtual void SetShow(int visible);
	int GetSlot(int x, int y);
protected:
	ShortCutWndInfo* m_info;
	ShortCutKey m_shortCutList[27];
	int m_maxSlot;
	int m_slot;
#if defined(BF_PORT)
	friend struct BfUiProbe;   // port/compat/stubs/bf_client_state.cpp: the control socket's `windows`
#endif
};

class UIEquipWnd : public UIFrameWnd {
public:
	UIEquipWnd();
	virtual ~UIEquipWnd();
	virtual bool IsUpdateNeed() { return true; }
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual void OnMouseHover(int cx, int cy);
	virtual void OnMouseMove(int cx, int cy);
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual void StoreInfo();
	ITEM_INFO GetItemInfo(int __$ReturnUdt, int x);
protected:
	bool HasOptionOffButton();
	bool HasOptionOpenButton();
	const char* GetOptionOffToolTipText();
	const char* GetOptionOpenToolTipText();
	void SendOptionOffToMode();
public:
	int m_typeWnd;
protected:
	int m_highlightMode[3];
	int m_orgHeight;
	EquipWndInfo* m_info;
	UIBitmapButton* m_optionOffButton;
	UIBitmapButton* m_optionOpenButton;
	unsigned int m_stateStartTick;
	int m_barHeight;
	UICheckBox* m_openEquipWndCheckBox;
};

class UINotifyItemObtainWnd : public UIFrameWnd {
public:
	UINotifyItemObtainWnd();
	virtual ~UINotifyItemObtainWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void OnProcess();
	virtual bool ShouldDoHitTest() { return false; }
protected:
	std::string m_itemName;
	std::string m_message;
	unsigned int m_madeTick;
	int m_isMoney;
	ITEM_INFO m_itemInfo;
	bool m_isIdentified;
};

class UIStatusWnd : public UIFrameWnd {
public:
	UIStatusWnd();
	virtual ~UIStatusWnd();
	virtual bool IsUpdateNeed() { return true; }
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnMouseHover(int cx, int cy);
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
	void ChangeState(int state);
protected:
	UIBitmapButton* m_incArrowButton[6];
	int m_orgHeight;
	StatusWndInfo* m_info;
};

class UISayDialogWnd : public UIFrameWnd {
public:
	UISayDialogWnd();
	virtual ~UISayDialogWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw() { DrawLatticeFrame(0, 0, 0, 0); }
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
	virtual void Resize(int cx, int cy);
protected:
	int GetButtonOffset(int buttonSize);
protected:
	UITextViewer* m_textViewer;
	std::list<UIBitmapButton *> m_buttonList;
	std::list<UIBitmapButton *> m_buttonListDeleted;
	int m_target;
	unsigned int m_naid;
	SayDialogWndInfo* m_info;
	int m_bClearTextViewer;
#if defined(BF_PORT)
	friend struct BfUiProbe;   // port/compat/stubs/bf_client_state.cpp: the control socket's `windows`
#endif
};

class UIChooseWnd : public UIFrameWnd {
public:
	UIChooseWnd();
	virtual ~UIChooseWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw() { DrawLatticeFrame(0, 0, 0, 0); }
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void StoreInfo();
protected:
	int m_index;
	std::vector<int> m_itemIndex;
	UIListBox* m_choostList;
	unsigned int m_naid;
	ChooseWndInfo* m_info;
#if defined(BF_PORT)
	friend struct BfUiProbe;   // port/compat/stubs/bf_client_state.cpp: the control socket's `windows`
#endif
};

class UIChooseWarpWnd : public UIFrameWnd {
public:
	UIChooseWarpWnd();
	virtual ~UIChooseWarpWnd();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw() { DrawLatticeFrame(0, 0, 0, 0); }
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
protected:
	UIListBox* m_choostList;
	unsigned int m_skid;
	std::vector<std::string> m_mapNameList;
};

class UIMinimapZoomWnd : public UIFrameWnd {
public:
	UIMinimapZoomWnd() {}
	virtual ~UIMinimapZoomWnd() {}
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UINotifyLevelUpWnd : public UIFrameWnd {
public:
	UINotifyLevelUpWnd() {}
	virtual ~UINotifyLevelUpWnd() {}
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UINotifyJobLevelUpWnd : public UINotifyLevelUpWnd {
public:
	UINotifyJobLevelUpWnd() {}
	virtual ~UINotifyJobLevelUpWnd() {}
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UINotifyQuestWnd : public UIFrameWnd {
public:
	UINotifyQuestWnd() {}
	virtual ~UINotifyQuestWnd() {}
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
};

class UIEmblemWnd : public UIFrameWnd {
public:
	UIEmblemWnd();
	virtual ~UIEmblemWnd();
	virtual void OnCreate(int cx, int cy) {}
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void OnProcess();
	int m_posx;
	int m_posy;
	std::string m_fileName;
	unsigned int m_masterAid;
};

class UIWordDisplayWnd : public UIFrameWnd {
public:
	UIWordDisplayWnd();
	virtual ~UIWordDisplayWnd();
	virtual void OnCreate(int cx, int cy) {}
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	virtual void OnProcess();
	int m_posx;
	int m_posy;
	std::string m_fileName;
	unsigned int m_masterAid;
};

struct PartyMapInfo {
public:
	std::string mapName;
	MapPosInfo mapPosInfo;
	std::list<std::string> cNameList;
};

class UIRoMapWnd : public UIFrameWnd {
public:
	UIRoMapWnd();
	virtual ~UIRoMapWnd();
	virtual void OnCreate(int x, int y);
	virtual void OnMouseMove(int x, int y);
	virtual void OnLBtnDown(int x, int y) {}
	virtual void OnDraw();
	virtual INT_PTR SendMsg(UIWindow* sender, int message, INT_PTR val1, INT_PTR val2, INT_PTR val3, INT_PTR val4);
	void DrawRoMap();
protected:
	void DrawMiniMap();
	void DrawPartyInfos();
	void DrawThisMapInfo();
	void ShowAllMapName();
	void ShowThisMapName(int x, int y);
	void ShowThisMapPartyInfo(int x, int y);
	int GetThisDivisionInMap(int mx, int my);
	bool IsInSameMap(std::string mapName, std::string cName);
	void GetPartyFriendMapInfo();
	bool GetPartyFriendPos(PartyMapInfo* partyMapInfo);
	void InitInfo();
protected:
	CTexture* m_titleBarTexture;
	CTexture* m_roMapTexture;
	CTexture* m_miniMapTexture;
	std::list<PartyMapInfo> m_partyPosList;
	PartyMapInfo* m_showThisPosInfo;
	PartyMapInfo* m_showMiniMap;
	MapPosInfo* m_thisPosInfo;
	MapPosInfo* m_miniMapInfo;
	unsigned char m_showAllMapName;
};

#endif
