#ifndef HIGHPRIEST_WINDOW_UISYS_H
#define HIGHPRIEST_WINDOW_UISYS_H



#include "../3dlib/Device.h"
#include "../3dlib/Renderer.h"
#include "../resource/Bitmap.h"
#include "../SnapMgr.h"
#include "../packet.h"
#include "../resource/Emblem.h"
#include "../UIRectInfo.h"
const int maxChatSize = 246;


enum {
	UM_DEFPUSH,
	UM_CANCELPUSH,
	UM_TOGGLEPUSH,
	UM_PAGEUPPUSH,
	UM_PAGEDOWNPUSH,
	UM_CLOSE,
	UM_COMMAND,
	UM_SCROLLVERT,
	UM_SCROLLHORZ,
	UM_SCROLLVERTPAGEUP,
	UM_SCROLLVERTPAGEDOWN,
	UM_SCROLLHORZPAGEUP,
	UM_SCROLLHORZPAGEDOWN,
	UM_SETSTATE,
	UM_RESIZE,
	UM_CHANGESTATUS,
	UM_OPENCHATWND,
	UM_LOADINGPERCENT,
	UM_UPPUSH,
	UM_DOWNPUSH,
	UM_LEFTPUSH,
	UM_RIGHTPUSH,
	UM_TAB,
	UM_REFRESHITEM,
	UM_ITEMEXPLANATION,
	UM_ENABLENEXTBUTTON,
	UM_SETSAYDIALOG,
	UM_SETCHOOSE,
	UM_SETNAID,
	UM_SETTEXT,
	UM_ADJUSTSIZE,
	UM_ADDITEM,
	UM_DISABLENEXTBUTTON,
	UM_DELETE,
	UM_SETINFO,
	UM_REFRESHBUTTON,
	UM_SETTITLE,
	UM_ADDCHAT,
	UM_DROP,
	UM_COMBOBOX_SELECTED,
	UM_SETCURITEM,
	UM_USE_SHORTEN_ITEM,
	UM_SET_COLOR,
	UM_CLEARMEMBER,
	UM_ADDMEMBER,
	UM_MEMBER_EXIT,
	UM_ROLE_CHANGE,
	UM_SETROOMID,
	UM_MENU_SELECTED,
	UM_MAKEMENU,
	UM_SET_EXCHANGE_CHAR_NAME,
	UM_MYDRAGCLOSED,
	UM_OTHERDRAGCLOSED,
	UM_SETINFO2,
	UM_GET_EDIT_FOCUS,
	UM_SETCOUNTINFO,
	UM_SET_JOIN_PARTY_MSG,
	UM_CHANGEMSG,
	UM_USESKILL,
	UM_SETSKID,
	UM_REFRESH,
	UM_SKILLDESCRIBE,
	UM_CARDITEMILLUSTRATE,
	UM_ENABLECLOSEBUTTON,
	UM_PARAM,
	UM_IME_SET_CANDIDATE,
	UM_IME_SET_COMPOSITION_STRING,
	UM_SETCOUNTMSG,
	UM_SET_SKILL_INFO,
	UM_CHANGE_GUILD_TAB,
	UM_DELETE_WHISPER_NAME,
	UM_MAKE_WHISPER_WINDOW,
	UM_ADDBOUNDARY,
	UM_SET_JOIN_GUILD_MSG,
	UM_SET_ALLY_GUILD_MSG,
	UM_CLEAR_ITEM,
	UM_SET_IDENTIFIY_INFO,
	UM_SET_CARD_ITEM_INDEX,
	UM_SET_BITMAP,
	UM_SET_MONSTER_INFO,
	UM_SET_TARGET,
	UM_SET_COUNT,
	UM_SET_BUTTON_FACE,
	UM_SET_CALLER_WID,
	UM_FIRSTSCREEN,
	UM_ADD_BUTTON,
	UM_DELETE_BUTTON,
	UM_SETPOS,
	UM_ENABLE_CANCEL,
	UM_ENABLE_EXIT,
	UM_SET_COUPLE_MSG,
	UM_SET_BABY_MSG,
	UM_READBOOK,
	UM_AUTOREAD,
	UM_GETBOOKCONTENTS,
	UM_SET_JOIN_FRIEND_MSG,
	UM_SET_BABY_MSG2,
	UM_TEXT_LBTN_DblClk,
	UM_TEXT_LBTN_UP,
	UM_TEXT_RBTN_UP,
	UM_BMP_LBTN_UP,
	UM_BMP_RBTN_UP,
	UM_SETID,
	UM_NEWMAIL,
	UM_DECITEM,
	UM_SET_BAR,
	UM_SET_DISABLE,
	UM_BEGIN_DRAG_BTN,
	UM_MOUSE_HOVER_BTN,
	UM_END_DRAG_BTN,
	UM_CHANGE_TVITEM_TEXT,
	UM_RECEIVE_MSG,
	UM_SAVE_CHAT,
	UM_NOT_SAVE_CHAT,
	UM_ENDPUSH,
	UM_ADDCHAT2,
	UM_LAST,
};
enum {
	ID_NOTHING = 117,
	ID_OK = 118,
	ID_CANCEL = 119,
	ID_LOGIN = 120,
	ID_YES = 121,
	ID_NO = 122,
	ID_BUTTON1 = 123,
	ID_BUTTON2 = 124,
	ID_BUTTON3 = 125,
	ID_STATUS = 126,
	ID_OPTION = 127,
	ID_ITEMS = 128,
	ID_EQUIP = 129,
	ID_SKILL = 130,
	ID_DIALOG = 131,
	ID_JOB = 132,
	ID_FRIEND = 133,
	ID_BASE = 134,
	ID_CLOSE = 135,
	ID_MINI = 136,
	ID_MAKE = 137,
	ID_SLOT = 138,
	ID_STR = 139,
	ID_AGI = 140,
	ID_VIT = 141,
	ID_INT = 142,
	ID_DEX = 143,
	ID_LUK = 144,
	ID_DELETE = 145,
	ID_HIS = 146,
	ID_CHECK = 147,
	ID_COMM = 148,
	ID_RADIOBTN = 149,
	ID_PLUS = 150,
	ID_MINUS = 151,
	ID_NEXT = 152,
	ID_MAP = 153,
	ID_RESTART = 154,
	ID_EXIT = 155,
	ID_CLOSEALL = 156,
	ID_SELL = 157,
	ID_BUY = 158,
	ID_WHISPERCOMBO = 159,
	ID_RIGHT = 160,
	ID_LEFT = 161,
	ID_NUMCOMBO = 162,
	ID_CLASSCOMBO = 163,
	ID_EXCHANGE = 164,
	ID_SENDMSG = 165,
	ID_TALK = 166,
	ID_FRIEND_INFO = 167,
	ID_PARTY_SETTING = 168,
	ID_DELETE_FRIEND = 169,
	ID_LEAVE = 170,
	ID_DELETE_FRIEND_NAME = 171,
	ID_TALKTOWHOCOMBO = 172,
	ID_LVLUP1 = 173,
	ID_LVLUP2 = 174,
	ID_LVLUP3 = 175,
	ID_LVLUP4 = 176,
	ID_LVLUP5 = 177,
	ID_LVLUP6 = 178,
	ID_LVLUP7 = 179,
	ID_PREV = 180,
	ID_MERCHANTITEMS = 181,
	ID_EQUIP_OPTION_OFF = 182,
	ID_EQUIP_OPTION_OPEN = 183,
	ID_CARDITEM = 184,
	ID_QUIT = 185,
	ID_TO_CHARACTER_SELECT = 186,
	ID_LVLSELECTDN1 = 187,
	ID_LVLSELECTDN2 = 188,
	ID_LVLSELECTDN3 = 189,
	ID_LVLSELECTDN4 = 190,
	ID_LVLSELECTDN5 = 191,
	ID_LVLSELECTDN6 = 192,
	ID_LVLSELECTDN7 = 193,
	ID_LVLSELECTUP1 = 194,
	ID_LVLSELECTUP2 = 195,
	ID_LVLSELECTUP3 = 196,
	ID_LVLSELECTUP4 = 197,
	ID_LVLSELECTUP5 = 198,
	ID_LVLSELECTUP6 = 199,
	ID_LVLSELECTUP7 = 200,
	ID_MAKEACCOUNT = 201,
	ID_SKINSELECTCOMBO = 202,
	ID_MODAL_PASSED = 203,
	ID_DETAILLEVEL = 204,
	ID_APPLY = 205,
	ID_ACCU_EXP = 206,
	ID_OPEN_MEMBER_POS_COMBO = 207,
	ID_ADD_GUILD_POSITION = 208,
	ID_POSNAME_EDIT = 209,
	ID_PAYRATE_EDIT = 210,
	ID_SEND_GUILD_NOTICE = 211,
	ID_CHANGE_GUILE_EMBLEM = 212,
	ID_UP = 213,
	ID_DOWN = 214,
	ID_NOTICE_TITLE_EDITED = 215,
	ID_NOTICE_EDITED = 216,
	ID_OPEN_EMOTIONLIST = 217,
	ID_CHARGE = 218,
	ID_INTRO = 219,
	ID_NOTICE = 220,
	ID_BOOKITEM = 221,
	ID_AUTOBOOKITEM = 222,
	ID_OPEN_HOMUNSKILLLIST = 223,
	ID_FEED_TO_HOMUN = 224,
	ID_DELETE_HOMUN = 225,
	ID_CHANGE_STORE_PASSWORD = 226,
	ID_STORE_PASSWORD_NUM0 = 227,
	ID_STORE_PASSWORD_NUM1 = 228,
	ID_STORE_PASSWORD_NUM2 = 229,
	ID_STORE_PASSWORD_NUM3 = 230,
	ID_STORE_PASSWORD_NUM4 = 231,
	ID_STORE_PASSWORD_NUM5 = 232,
	ID_STORE_PASSWORD_NUM6 = 233,
	ID_STORE_PASSWORD_NUM7 = 234,
	ID_STORE_PASSWORD_NUM8 = 235,
	ID_STORE_PASSWORD_NUM9 = 236,
	ID_STORE_PASSWORD_NUM_RESET = 237,
	ID_STORE_RETURN = 238,
	ID_SEND_MAIL = 239,
	ID_DELETE_MAIL = 240,
	ID_CANCEL_MAIL = 241,
	ID_INPUT_ZENY = 242,
	ID_SET_ZENY = 243,
	ID_RETURN_MAIL = 244,
	ID_AUCTION_VIEW = 245,
	ID_AUCTION_ADD = 246,
	ID_AUCTION_SELL = 247,
	ID_AUCTION_BUY = 248,
	ID_ITEM_NAME = 249,
	ID_PREV_PAGE = 250,
	ID_NEXT_PAGE = 251,
	ID_ADD_AUCTION = 252,
	ID_FINISH_AUCTION = 253,
	ID_BUY_AUCTION = 254,
	ID_SEARCH = 255,
	ID_FROM_NAME = 256,
	ID_HEADER = 257,
	ID_SET_STATUS = 258,
	ID_VIEW_LETTER = 259,
	ID_ADD_FRIEND = 260,
	ID_ADD_GROUP = 261,
	ID_SEND_MSG = 262,
	ID_STATUS_MENU = 263,
	ID_GROUP_MENU = 264,
	ID_FRIEND_MENU = 265,
	ID_FRIEND_MENU1 = 266,
	ID_GROUP = 267,
	ID_INVITE = 268,
	ID_VIEW_CHAT_LIST = 269,
	ID_SELECT_ITEM = 270,
	ID_INSERT_FRIEND = 271,
	ID_SET_NICK = 272,
	ID_DELETE_SOCKET = 273,
	ID_SELECT_WND = 274,
	ID_SEARCH_FRIEND = 275,
	ID_SETTING = 276,
	ID_HELP = 277,
	ID_LOGOUT = 278,
	ID_SEND = 279,
	ID_REPLY = 280,
	ID_NAME = 281,
	ID_TYPING_MSG = 282,
	ID_OPEN_MERSKILLLIST = 283,
	ID_FIRE_MER = 284,
	ID_CHANGE_NAME = 285,
	ID_CHANGE_NAME_SUCCESS = 286,
	ID_REBIRTH = 287,
	ID_PREVIEW = 288,
	ID_NEXTVIEW = 289,
	ID_BOOKMARK = 290,
	ID_STICK_ON = 291,
	ID_STICK_OFF = 292,
	ID_B_GRAVITY = 293,
	ID_B_HANGAME = 294,
	ID_QUEST = 295,
	ID_PREMIUMSLOT = 296,
	ID_RESET = 297,
};
enum {
	ME_OK,
	ME_OKCANCEL,
	ME_YESNO,
	ME_YESNOCANCEL,
	ME_CANCEL,
};
enum {
	UIM_CREATESTATUSWND,
	UIM_PUSHINTOCHATHISTORY,
	UIM_CHANGECHATWNDSTATUS,
	UIM_OPENCHATWND,
	UIM_LOADINGPERCENT,
	UIM_PUSH_INTO_CHATROOM,
	UIM_SAVE_CHAT_TO_FILE,
	UIM_SAVE_CHAT_TO_FILE_FROM_CHATROOM,
	UIM_IME_OPENCANDIDATE,
	UIM_IME_CHANGECANDIDATE,
	UIM_IME_CLOSECANDIDATE,
	UIM_IME_STARTCOMPOSITION,
	UIM_IME_COMPSTR,
	UIM_IME_ENDCOMPOSITION,
	UIM_MAKE_WHISPER_WINDOW,
	UIM_CURSOR_CHANGE_ACTION,
	UIM_CURSOR_CHANGE_MOTION,
	UIM_REPORT_CHAT_TO_FILE,
	UIM_REPORT_CHAT_TO_FILE_FROM_CHATROOM,
	UIM_CLOSE_GRON_MESSENGER,
	UIM_PUSHINTOCHATHISTORY2,
};
enum {
	DROP_TO_GROUND,
	DROP_TO_ITEMPURCHASEWND,
	DROP_TO_ITEMSELLWND,
	DROP_TO_ITEMSHOPWND_0,
	DROP_TO_ITEMSHOPWND_1,
	DROP_TO_EXCHANGEWND,
	DROP_TO_ITEMWND,
	DROP_TO_ITEMSTOREWND,
	DROP_TO_MERCHANTITEMWND,
	DROP_TO_MERCHANTSHOPMAKEWND,
	DROP_TO_MERCHANTMIRRORITEMWND,
	DROP_TO_MERCHANTITEMSHOPWND,
	DROP_TO_MERCHANTITEMPURCHASEWND,
	DROP_TO_MAILWND,
};
enum {
	NMSG_CHAT = 1,
	NMSG_WHISPER = 2,
	NMSG_PARTY = 3,
	NMSG_GUILD = 4,
	NMSG_GET_ITEM = 5,
	NMSG_EQUIP = 6,
	NMSG_CHANGE_STATUS = 7,
	NMSG_GET_ITEM_PARTY = 8,
	NMSG_CHANGE_STATUS_PARTY = 9,
	NMSG_FAIL_SKILL = 10,
	NEW_PARTY_SETUP = 11,
	NMSG_DAMAGED_EQUIP = 12,
	NMSG_SEIGE_INFO = 13,
};
enum {
	MODE_ST_CHAT,
	MODE_BT_CHAT,
};


enum WINDOWID : int;

enum {
	SB_NULL = -1,
	SB_UPBTN = 0,
	SB_UPPAGE = 1,
	SB_THUMB = 2,
	SB_DNPAGE = 3,
	SB_DNBTN = 4,
};
enum {
	SDM_NORMAL,
	SDM_UPBTNON,
	SDM_DNBTNON,
	SDM_THUMBON,
	SDM_UPBTNPUSH,
	SDM_DNBTNPUSH,
	SDM_THUMBPUSH,
};
enum {
	FT_GULIM = 0,
	FT_ARIAL = 1,
	FT_SQUIRREL = 20,
	FT_LOVEANGEL = 21,
	FT_GOGO = 22,
	FT_DIARY = 23,
	FT_MINIHEART = 24,
	FT_FRESHMAN = 25,
	FT_KID = 26,
	FT_MAGIC = 27,
	FT_JJANGU = 28,
};

struct BOXINFO {
public:
	int x;
	int y;
	int cx;
	int cy;
	int drawEdge;
	int color;
	int color2;
};
struct DRAG_INFO;



enum {
	SNT_VERTLEFT,
	SNT_VERTRIGHT,
	SNT_HORZTOP,
	SNT_HORZBOTTOM,
};

struct SnapInfo {
public:
	int points[3];
};

class CSnapInfo {
public:
	CSnapInfo(int x, int y, int w, int h) { SetInfo(x, y, w, h); }
	CSnapInfo() {}
	void SetInfo(int x, int y, int w, int h);
	void Reverse();
	void GetDeltaSnap(CSnapInfo* snapInfo, int& dx, int& dy);
	bool IsOverLapped(int s1, int d1, int s2, int d2);
	SnapInfo m_info[4];
};

class UIWindow {
public:
	UIWindow();
	virtual ~UIWindow();
	void Create(int deltaDragX, int deltaDragY);
	void Destroy();
	void AddChild(UIWindow* window);
	void RemoveChild(UIWindow* window);
	void RemoveAllChildren();
	void ClearAllChildrenState();
	void InvalidateChildren();
	void SetId(int id) { m_id = id; }
	int GetId() { return m_id; }
	int IsFamily(UIWindow* window);
	virtual void Invalidate();
	virtual void InvalidateWallPaper() {}
	UIWindow* HitTest(int x, int y);
	void DoDraw(int visible);
	virtual void Resize(int deltaDragX, int deltaDragY);
	virtual bool IsFrameWnd() { return false; }
	virtual bool IsUpdateNeed() { return false; }
	virtual void Move(int deltaDragX, int deltaDragY);
	virtual bool CanGetFocus() { return false; }
	virtual bool GetTransBoxInfo(BOXINFO* __formal) { return false; }
	virtual bool IsTransmitMouseInput() { return false; }
	virtual bool ShouldDoHitTest() { return true; }
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
	virtual void StoreInfo() {}
	virtual void SaveOriginalPos() {}
	virtual void MoveDelta(int deltaDragX, int deltaDragY) {}
	bool IsChildOf(UIWindow* win);
	UIWindow* GetNextSiblingOf(UIWindow* wnd);
	UIWindow* GetPrevSiblingOf(UIWindow* wnd);
	UIWindow* GetAncestor();
	void GetGlobalCoor(int& x, int& y);
	int IsShow() { return m_show; }
	virtual unsigned int GetColor();
	virtual void SetShow(int visible) { m_show = visible; }
	virtual void OnCreate(int cx, int cy) {}
	virtual void OnDestroy() {}
	virtual void OnProcess() {}
	virtual void OnDraw() {}
	virtual void OnDraw2() {}
	virtual void OnRun() {}
	virtual void OnSize(int cx, int cy) {}
	virtual void OnBeginEdit() {}
	virtual void OnFinishEdit() {}
	virtual void OnLBtnDown(int x, int y) {}
	virtual void OnLBtnDblClk(int x, int y) {}
	virtual void OnMouseMove(int x, int y) {}
	virtual void OnMouseHover(int x, int y) {}
	virtual void OnMouseShape(int deltaDragX, int deltaDragY);
	virtual void OnLBtnUp(int x, int y) {}
	virtual void OnRBtnDown(int x, int y) {}
	virtual void OnRBtnUp(int x, int y) {}
	virtual void OnRBtnDblClk(int x, int y) {}
	virtual void OnWheel(int wheel) {}
	virtual void RefreshSnap() {}
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual bool GetTransBoxInfo2(BOXINFO* __formal) { return false; }
	virtual void DrawBoxScreen2() {}
	int GetX() { return m_x; }
	int GetY() { return m_y; }
	int GetHeight() { return m_h; }
	int GetWidth() { return m_w; }
protected:
	void DrawBitmap(int x, int y, CBitmapRes* bitmap, bool drawOnlyNoTrans);
	void TextOutA(int x, int y, const char* text, int textLen, int fontType, int fontHeight, unsigned int colorRef);
	void TextOutWithRightAlign(int x, int y, const char* text, int textLen, int fontType, int fontHeight, unsigned int colorRef);
	void TextOutWithDecoration(int x, int y, const char* text, int textLen, unsigned int* colorRef, int fontType, int fontHeight);
	void TextOutWithShadow(int x, int y, const char* text, int textLen, unsigned int colorText = 0, unsigned int colorShadow = 0xffffff, int fontType = 0, int fontHeight = 12);
	void TextOutWithOutline(int x, int y, const char* text, int textLen, unsigned int colorText, unsigned int colorOutline, int fontType, int fontHeight, bool bold);
	void TextOutWithDecoOutline(int x, int y, const char* text, int textLen, unsigned int* colorText, unsigned int colorOutline, int fontType, int fontHeight, bool bold);
	void TextOutVertical(int x, int y, const char* text, int textLen, int fontType, int fontHeight, unsigned int colorRef);
	void TextOutWithOutlineVertical(int x, int y, const char* text, int textLen, unsigned int colorText, unsigned int colorOutline, int fontType, int fontHeight, bool bold);
	void TextOutWithTwoColors(int x, int y, const char* text, unsigned int colorRef, unsigned int colorRef2, int textLen, int fontType, int fontHeight);
	int GetTextWidth(const char* text, int textLen, int fontType, int fontHeight, bool bold);
	int GetTextHeight(const char* text, int textLen, int fontType, int fontHeight, bool bold);
	int GetTextHeightVertical(const char* text, int textLen, int fontType, int fontHeight, bool bold);
	int GetColorTextWidth(const char* text, int textLen, int fontType, int fontHeight, bool bold);
	char* PolishText(const char* text, int drawWidth, int fontType, int fontHeight, bool bold);
	char* PolishText2(const char* text, int maxNumCharLine);
	void PolishText3(const char* text, std::vector<std::string>& strings, std::vector<bool>& enters, int width);
	void PolishText4(const char* text, std::vector<std::string>& strings, int width);
	void ClearDC(unsigned int color);
	void ClearDCRect(unsigned int color, tagRECT& rect);
	void DrawBox(int x, int y, int cx, int cy, unsigned int color);
	void DrawBorderLine(int x, int y, int cx, int cy, unsigned int color);
	void DrawTriangleDC(int x1, int y1, int x2, int y2, int x3, int y3, unsigned int color);
	const char* InterpretColor(const char* color_text, unsigned int* colorRef);
	const char* FindColorMark(const char* text, const char* text_end);
public:


	friend class UIWindowMgr;
	friend class SnapObject;
	friend class CSnapMgr;
protected:
	UIWindow* m_parent;
	std::list<UIWindow *> m_children;
	int m_x;
	int m_y;
	int m_w;
	int m_h;
	int m_isDirty;
	CDC* m_dc;
	int m_id;
	int m_state;
	int m_stateCnt;
	int m_show;
	unsigned int m_trans;
	unsigned int m_transTarget;
	unsigned int m_transTime;
};

struct BasicInfoWndInfo;
struct ChatRoomWndInfo;
struct ChooseWndInfo;
struct CombinedCardItemCollectionWndInfo;
struct DetailLevelWndInfo;
struct EquipWndInfo;
struct ExchangeAcceptWndInfo;
struct HomunInfoWndInfo;
struct ItemCollectionWndInfo;
struct ItemCompositionWndInfo;
struct ItemIdentifyWndInfo;
struct ItemParamChangeDisplayWndInfo;
struct ItemPurchaseWndInfo;
struct ItemSellWndInfo;
struct ItemShopWndInfo;
struct ItemStoreWndInfo;
struct ItemWndInfo;
struct MerInfoWndInfo;
struct MerchantItemMyShopWndInfo;
struct MerchantItemPurchaseWndInfo;
struct MerchantItemShopWndInfo;
struct MerchantItemWndInfo;
struct MerchantMirrorItemWndInfo;
struct MerchantShopMakeWndInfo;
struct NamePair;
struct OptionWndInfo;
struct PartySettingWndInfo;
struct PetInfoWndInfo;
struct QuestWndInfo;
struct SayDialogWndInfo;
struct SkillListWndInfo;
struct StatusWndInfo;
struct WhisperWndInfo;
class UIAllyGuildAcceptWnd;
class UIAuctionMsgWnd;
class UIAuctionWnd;
class UIAutoMessageWnd;
class UIBabyAcceptWnd;
class UIBabyAcceptWnd2;
class UIBasicInfoWnd;
class UIBookWnd;
class UICardItemIllustWnd;
class UIChatRoomChangeWnd;
class UIChatRoomMakeWnd;
class UIChatRoomWnd;
class UIChooseSellBuyWnd;
class UIChooseWarpWnd;
class UIChooseWnd;
class UICombinedCardItemCollectionWnd;
class UIComboBoxWnd;
class UICommandListWnd;
class UICoupleAcceptWnd;
class UIDetailLevelWnd;
class UIEquipWnd;
class UIExchangeAcceptWnd;
class UIExchangeWnd;
class UIFrameWnd;
class UIHomunInfoWnd;
class UIImgMenuWnd;
class UIItemCollectionWnd;
class UIItemCompositionWnd;
class UIItemDropCntWnd;
class UIItemIdentifyWnd;
class UIItemParamChangeDisplayWnd;
class UIItemPurchaseWnd;
class UIItemSellWnd;
class UIItemShopWnd;
class UIItemStoreWnd;
class UIItemWnd;
class UIJoinFriendAcceptWnd;
class UIJoinGuildAcceptWnd;
class UIJoinPartyAcceptWnd;
class UILoadingWnd;
class UILoginWnd;
class UIMailListWnd;
class UIMailViewWnd;
class UIMakingArrowListWnd;
class UIMenuWnd;
class UIMerInfoWnd;
class UIMerchantItemMyShopWnd;
class UIMerchantItemPurchaseWnd;
class UIMerchantItemShopWnd;
class UIMerchantItemWnd;
class UIMerchantMirrorItemWnd;
class UIMerchantShopMakeWnd;
class UINotifyItemObtainWnd;
class UINpcEditDialogWnd;
class UINpcTextEditDialogWnd;
class UIOptionWnd;
class UIPartySettingWnd;
class UIPasswordWnd;
class UIPetEggListWnd;
class UIPetInfoWnd;
class UIPetTamingDeceiveWnd;
class UIProhibitListWnd;
class UIQuestDetailWnd;
class UIQuestWnd;
class UIQuitWnd;
class UIRestartWnd;
class UISayDialogWnd;
class UISelectCartWnd;
class UISelectCharWnd;
class UISkillDescribeWnd;
class UISkillListWnd;
class UISkillNameChangeWnd;
class UISpellListWnd;
class UIStarPlaceAcceptWnd;
class UIStatusWnd;
class UITipOfTheDayWnd;
class UIYourItemWnd;
struct ShortCutWndInfo;
struct SubChatWndInfo;
class UIBattleMsgOptionWnd;
class UIChangeNameWnd;
class UIMemorialDunWnd;
class UINewChatWnd;
class UIRebirthWnd;
class UIRoMapWnd;
class UISelCharForUServerWnd;
class UIShortCutWnd;
class UISubChatHisWnd;
class UISubChatMiniWnd;
class UICandidateWnd;
class UICompositionWnd;
class UIEmotionListWnd;
class UIEmotionWnd;
class UIFriendOptionWnd;
class UIGuildBanReasonDescWnd;
class UIGuildBanishedMemberWnd;
class UIGuildInfoManageWnd;
class UIGuildLeaveReasonDescWnd;
class UIGuildMemberManageWnd;
class UIGuildNoticeWnd;
class UIGuildPositionManageWnd;
class UIGuildSkillWnd;
class UIGuildTotalInfoWnd;
class UIIllustWnd;
class UIItemRepairWnd;
class UIKeyStrokeWnd;
class UIMakeTargetListWnd;
class UIMakeTargetProcessWnd;
class UIMakeTargetResultWnd;
class UIMessengerGroupWnd;
class UIWeaponRefineWnd;
class UIMinimapZoomWnd;
class UINotifyJobLevelUpWnd;
class UINotifyLevelUpWnd;
class UINotifyQuestWnd;

class UIBarGraph : public UIWindow {
public:
	UIBarGraph();
	virtual ~UIBarGraph();
	virtual void OnDraw();
	void SetAmount(float percent);
	float m_percent;
};

class UIBarGraph2 : public UIBarGraph {
public:
	UIBarGraph2();
	virtual ~UIBarGraph2();
	virtual void OnDraw();
	int m_isVertical;
	int m_backR;
	int m_backG;
	int m_backB;
	int m_r;
	int m_g;
	int m_b;
};

class UIBarGraphPlayerHp : public UIWindow {
public:
	UIBarGraphPlayerHp();
	virtual ~UIBarGraphPlayerHp();
	virtual void OnDraw();
	int m_isPlayerType;
};

class UIBarGraphPlayerSp : public UIWindow {
public:
	UIBarGraphPlayerSp();
	virtual ~UIBarGraphPlayerSp();
	virtual void OnDraw();
	int m_isPlayerType;
};

class UIScrollBar : public UIWindow {
public:
	UIScrollBar();
	virtual ~UIScrollBar();
	void Create(int x, int y, int type, int size);
	virtual void OnLBtnDown(int x, int y);
	virtual void OnLBtnDblClk(int x, int y);
	virtual void OnMouseMove(int x, int y);
	virtual void OnMouseShape(int x, int y);
	virtual void OnLBtnUp(int x, int y);
	virtual void OnRBtnUp(int x, int y);
	virtual void OnWheel(int wheel);
	virtual void OnDraw();
	virtual void Resize(int x, int y);
	virtual bool IsTransmitMouseInput();
	virtual int GetThumbSize();
	void SetRange(int range, int parentnumCanDisplayItem, int parentNumItem);
	void SetPos(int wheel);
	void SetType(int wheel);
	int GetPos();
protected:
	int HitTest(int x, int y);
	int GetScrollRange();
	int GetThumbOffset();
protected:
	int m_drawMode;
	int m_maxPos;
	int m_curPos;
	int m_isVert;
	int m_startDragX;
	int m_startDragY;
	int m_deltaDrag;
	int m_parentNumItem;
	int m_parentnumCanDisplayItem;
	int m_scrollBtnSize;
	int m_fixedLength;
	int m_type;
};

class UIScrollBar2 : public UIScrollBar {
public:
	UIScrollBar2();
	virtual void OnDraw();
};

class UIScrollBar3 : public UIScrollBar {
public:
	UIScrollBar3();
	virtual void OnDraw();
};

class UIScrollBar4 : public UIScrollBar {
public:
	UIScrollBar4();
	virtual void OnDraw();
	virtual int GetThumbSize() { return m_thumbSize; }
protected:
	int m_thumbSize;
};

class UIImgMenu : public UIWindow {
public:
	UIImgMenu();
	virtual ~UIImgMenu();
	void Create(int cx, int cy, int type, int size, std::string bgImg);
	virtual void OnDraw();
	virtual void OnLBtnUp(int x, int y);
	virtual void OnMouseMove(int x, int y);
	virtual void OnMouseHover(int x, int y);
	virtual void OnMouseShape(int x, int y);
	virtual void Move(int x, int y);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	int GetMenuIndex(int x, int y);
protected:
	int m_curMenuIndex;
	int m_size;
	int sel_w;
	int sel_h;
	int m_type;
	unsigned char* m_bar;
	unsigned char* m_disable;
	std::string m_bgImg;
	std::string m_bgImgOver;
	WINDOWID m_callerWid;
};

class UIEditCtrl : public UIWindow {
public:
	UIEditCtrl();
	virtual ~UIEditCtrl();
	virtual void OnLBtnDown(int x, int y);
	virtual void OnDraw();
	virtual void OnBeginEdit();
	virtual void OnFinishEdit();
	virtual void SetText(const char* txt);
	virtual bool CanGetFocus() { return true; }
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	void SetMaskChar(char maskchar) { m_maskchar = maskchar; }
	void SetTextLimit(int maxchar) { if (maxchar < 0) maxchar = 0; m_maxchar = maxchar; }
	void SetFrameColor(int r, int g, int b);
	void SetTextColor(int r, int g, int b);
	void MakeSelect();
	void DisableSelect(int cursorPos);
	const char* GetText();
	int GetTextSize();
	void SetType(int type) { m_type = type; }
protected:
	virtual void DrawEditText();
	void RefreshText();
	void DrawSelectionBox(int x, int y);
	void UpdateKeyStroke(int cursorPos);
	void UpdateCandidate(int cursorPos);
	virtual void SetTextByMaxChar();
protected:
	int m_selectionOrigin;
	int m_selectionCursor;
	int m_maskchar;
	int m_maxchar;
	int m_isSingColorFrame;
	int m_r;
	int m_g;
	int m_b;
	int m_textR;
	int m_textG;
	int m_textB;
	int m_xOffset;
	int m_yOffset;
	int m_type;
public:
	std::string m_text;
};

class UIBitmapEditCtrl : public UIEditCtrl {
public:
	virtual void OnDraw();
	void SetBitmap(const char* bitmapName);
protected:
	std::string m_bitmapName;
};

class UIEditCtrl2 : public UIEditCtrl {
public:
	UIEditCtrl2();
	virtual ~UIEditCtrl2();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
};

class UIEditCtrl3 : public UIEditCtrl2 {
public:
	UIEditCtrl3();
	virtual ~UIEditCtrl3();
	virtual void OnCreate(int cx, int cy);
	virtual void Resize(int cx, int cy);
	virtual void OnDraw();
	virtual void OnWheel(int wheel);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual void RecalcScrbarPos();
	void OnShiftEnter();
protected:
	virtual void DrawEditText();
	void DrawSelectionBox(int xOffset, int yOffset, int width);
	virtual void SetTextByMaxChar();
	void OnKeyDown(unsigned int wParam, int lParam);
protected:
	UIScrollBar* m_scrollBar;
	int m_scrEnabled;
	int m_itemSpacing;
	int m_itemSize;
	int m_lineLength;
	int m_curPos;
	int m_curPos2;
	int m_curWidth;
	int m_prePos;
	int m_nextPos;
	int m_scrollOffset;
	unsigned char m_moveScroll;
	unsigned char m_moveUpCursor;
	unsigned char m_moveDownCursor;
	char* m_upLineText;
	char* m_downLineText;
	std::vector<std::string> strings;
	std::vector<bool> enters;
	unsigned char m_cursorMove;
	unsigned char m_onShiftEnter;
};

class UIWhisperEditCtrl : public UIEditCtrl {
public:
	UIWhisperEditCtrl();
	virtual void OnBeginEdit();
	virtual void OnFinishEdit();
	virtual void SetText(const char* txt);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	void StoreInHistory();
	std::vector<std::string> m_hisChat;
	int m_curHis;
};

class UIChatEditCtrl : public UIEditCtrl {
public:
	UIChatEditCtrl();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual void OnLBtnDown(int x, int y);
	void StoreInHistory();
	std::vector<std::string> m_hisChat;
	std::string m_lastEditText;
	int m_curHis;
};

class UIEditCtrlWheel : public UIEditCtrl {
public:
	UIEditCtrlWheel() {}
	virtual ~UIEditCtrlWheel() {}
	virtual void OnWheel(int wheel) { if (m_parent) m_parent->OnWheel(wheel); }
};

class UIEditCtrlVarId : public UIEditCtrl {
public:
	UIEditCtrlVarId() {}
	virtual ~UIEditCtrlVarId() {}
	virtual void SetText(const char* txt);
	virtual void OnFinishEdit();
	virtual void OnWheel(int wheel);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
};

class UIStaticText : public UIWindow {
public:
	UIStaticText();
	virtual ~UIStaticText();
	virtual void OnDraw();
	virtual void OnLBtnDown(int fontType, int fontHeight);
	virtual void OnLBtnDblClk(int fontType, int fontHeight);
	virtual void OnMouseMove(int fontType, int fontHeight);
	virtual void OnLBtnUp(int fontType, int fontHeight);
	virtual void OnMouseHover(int fontType, int fontHeight);
	virtual void OnMouseShape(int fontType, int fontHeight);
	virtual void OnWheel(int number);
	virtual void OnRBtnDown(int fontType, int fontHeight);
	virtual void OnRBtnUp(int fontType, int fontHeight);
	void SetText(const char* txt, int drawBold);
	void SetTextWithNumber(int number);
	void SetTextWithShorten(const char* txt);
	void SetTextWithNewWidth(const char* txt);
	void SetFont(int fontType, int fontHeight);
	void SetTextColor(int r, int g, int b);
	const char* GetFullText() { return m_fullText.c_str(); }
	int m_drawBackGround;
	int m_backR;
	int m_backG;
	int m_backB;
	int m_textR;
	int m_textG;
	int m_textB;
	int m_drawTwice;
	int m_drawBold;
protected:
	int m_fontHeight;
	int m_fontType;
	int m_isShorten;
	std::string m_text;
	std::string m_fullText;
};

class UIStaticBitmap : public UIWindow {
public:
	UIStaticBitmap();
	virtual ~UIStaticBitmap();
	virtual void OnLBtnDown(int x, int y);
	virtual void OnMouseMove(int x, int y);
	virtual void OnLBtnUp(int x, int y);
	virtual void OnDraw();
	void SetBitmap(const char* bitmapName);
private:
	std::string m_bitmapName;
};

class UICheckBox : public UIWindow {
public:
	UICheckBox();
	virtual ~UICheckBox();
	virtual void OnLBtnDown(int x, int y);
	virtual void OnDraw();
	virtual void OnMouseShape(int x, int y);
	void SetBitmap(const char* onBitmapName, const char* offBitmapName);
	void SetBitmap2(const char* onBitmapName, const char* offBitmapName, const char* checkMsg);
	void SetCheck(int check);
	void SetCheck2(int check) { m_state = check; Invalidate(); }
	int GetCheck() { return m_state; }
	int m_isDisabled;
private:
	std::string m_onBitmapName;
	std::string m_offBitmapName;
	std::string m_checkMsg;
};

class UISlotBitmap : public UIStaticBitmap {
public:
	UISlotBitmap(int nSlotID);
	virtual ~UISlotBitmap();
	virtual void OnLBtnDown(int x, int y);
	virtual void OnLBtnDblClk(int x, int y);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
protected:
	int m_nSlotID;
};

enum RESIZERTYPE {
	RT_LEFT = 0,
	RT_RIGHT = 1,
	RT_UP = 2,
	RT_DOWN = 3,
	RT_UPLEFT = 4,
	RT_UPRIGHT = 5,
	RT_DOWNLEFT = 6,
	RT_DOWNRIGHT = 7,
};

class UIResizer : public UIWindow {
public:
	UIResizer();
	virtual void OnDraw();
	virtual void OnLBtnDown(int x, int y);
	virtual void OnMouseMove(int x, int y);
	virtual void OnLBtnUp(int x, int y);
	virtual void OnMouseHover(int x, int y);
	virtual void OnMouseShape(int x, int y);
	void SetBitmap(const char* bitmapName);
	void SetType(RESIZERTYPE type);
	void SetColor(unsigned int color, unsigned int backColor);
	void SetColor(unsigned int);
protected:
	std::string m_bitmapName;
	int m_startX;
	int m_startY;
	RESIZERTYPE m_type;
	int m_parentWidth;
	int m_parentHeight;
	unsigned int m_color;
	unsigned int m_backColor;
};

class UITab : public UIWindow {
public:
	UITab();
	virtual void OnDraw();
	virtual void OnLBtnDown(int x, int y);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	int GetState() { return m_state; }
	int m_numTab;
	int m_typeTab;
};

class UIRadioBtn : public UIWindow {
public:
	UIRadioBtn();
	virtual ~UIRadioBtn();
	virtual void OnDraw();
	virtual void OnLBtnDown(int x, int y);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	void SetText(const char* text);
	void SetType(int);
	std::string m_text;
	int m_isEnabled;
	int m_type;
};

class UIMerchantShopTitle : public UIWindow {
public:
	UIMerchantShopTitle();
	virtual ~UIMerchantShopTitle();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnRBtnDown(int cx, int cy);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual void OnMouseShape(int cx, int cy);
	int GetShopOwnerAid();
	UIStaticText* m_title;
	unsigned int m_shopOwnerAid;
	short m_shopfont;
};

struct ChatRoomInfo {
public:
	std::string title;
	std::string pass;
	int roomType;
	int numPeople;
	int maxNumPeople;
	int roomId;
};

class UIChatRoomTitle : public UIWindow {
public:
	UIChatRoomTitle();
	virtual ~UIChatRoomTitle();
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual void OnMouseShape(int cx, int cy);
	int GetRoomId();
	UIStaticText* m_title;
	ChatRoomInfo m_roomInfo;
	short m_shopfont;
};

class UIBarGraph3 : public UIWindow {
public:
	UIBarGraph3();
	virtual ~UIBarGraph3();
	virtual void OnDraw() = 0;
	void SetAmount(const int* pAmount, const int* pTotalAmount);
	const int* m_pAmount;
	const int* m_pTotalAmount;
};

class UIBarGraph4 : public UIBarGraph3 {
public:
	UIBarGraph4() {}
	virtual ~UIBarGraph4() {}
	virtual void OnDraw();
	virtual void OnMouseHover(int x, int y);
	unsigned char m_bType;
};

class UIGage : public UIWindow {
public:
	UIGage();
	virtual ~UIGage();
	virtual bool ShouldDoHitTest() { return false; }
	int m_edgeR;
	int m_edgeG;
	int m_edgeB;
	int m_backR;
	int m_backG;
	int m_backB;
	int m_r;
	int m_g;
	int m_b;
	int m_backR2;
	int m_backG2;
	int m_backB2;
	int m_r2;
	int m_g2;
	int m_b2;
};

class UIRechargeGage : public UIGage {
public:
	UIRechargeGage();
	virtual ~UIRechargeGage();
	virtual void OnDraw();
	void SetAmount(int amount, int totalAmount);
	int m_amount;
	int m_totalAmount;
};

class UIPcGage : public UIGage {
public:
	UIPcGage();
	virtual ~UIPcGage();
	virtual void OnDraw();
	void SetHp(int amount, int totalAmount);
	void SetSp(int amount, int totalAmount);
	void SetMode(int mode);
	int m_dangerR;
	int m_dangerG;
	int m_dangerB;
	int m_mode;
	int m_hpAmount;
	int m_hpTotalAmount;
	int m_spAmount;
	int m_spTotalAmount;
};

class UIPlayerGage : public UIGage {
public:
	UIPlayerGage();
	virtual ~UIPlayerGage();
	virtual bool IsUpdateNeed() { return true; }
	virtual void OnDraw();
	int m_dangerR;
	int m_dangerG;
	int m_dangerB;
	int m_isPlayerType;
};

class UIBalloonText : public UIWindow {
public:
	UIBalloonText();
	virtual ~UIBalloonText();
	virtual void OnProcess();
	virtual void OnDraw();
	virtual bool ShouldDoHitTest() { return false; }
	void SetText(const char* msg, const int maxNumCharLine);
	void AddText(const char* msg);
	void SetFntColor(unsigned int fntColor, unsigned int bgColor) { m_fontColor = fntColor; m_bgColor = bgColor; }
	void SetFntSize(int fontSize);
	void SetBackTrans(unsigned char);
	void SetCharFont(short charfont) { m_charfont = charfont; }
protected:
	void AdjustSizeByText();
protected:
	unsigned char m_isBold;
	int m_fontSize;
	std::vector<std::string> m_strings;
	unsigned int m_fontColor;
	unsigned int m_bgColor;
	unsigned char m_isBack;
	short m_charfont;
};

class UITransBalloonText : public UIBalloonText {
public:
	UITransBalloonText();
	virtual ~UITransBalloonText();
	virtual void OnDraw();
	virtual bool GetTransBoxInfo(BOXINFO* boxInfo);
	virtual void OnCreate(int cx, int cy);
	virtual void Move(int cx, int cy);
	virtual void Resize(int cx, int cy);
	void SetTransBoxColor(unsigned int transboxColor);
	BOXINFO m_transBoxInfo;
};

class UIColorTransBalloonText : public UITransBalloonText {
private:
	virtual void OnDraw();
};

class UINameBalloonText : public UIBalloonText {
public:
	UINameBalloonText();
	virtual ~UINameBalloonText();
	virtual void OnDraw();
	virtual void SetEmblemResName(const char* cName);
	virtual void SetNameText(const char* cName);
	virtual void SetNameText(const NamePair& cName, bool cutSharp);
	unsigned char HasEmblem();
	std::string m_ebmResName;
	int m_textWidth;
};

class UIVerticalNameBalloonText : public UINameBalloonText {
public:
	virtual void OnDraw();
	virtual void SetEmblemResName(const char* namePair);
	virtual void SetNameText(const char* namePair);
	virtual void SetNameText(const NamePair& namePair, bool cutSharp);
protected:
	int m_textHeight;
};




class UICharInfoBalloonText : public UITransBalloonText {
public:
	UICharInfoBalloonText();
	virtual ~UICharInfoBalloonText();
	virtual void OnDraw();
	void SetText(std::vector<std::string>& strings) { m_strings = strings; }
};

class UIButton : public UIWindow {
public:
	UIButton();
	virtual ~UIButton();
	virtual void OnLBtnDblClk(int x, int y);
	virtual void OnLBtnDown(int x, int y);
	virtual void OnMouseMove(int x, int y);
	virtual void OnLBtnUp(int x, int y);
	virtual void OnMouseShape(int x, int y);
	virtual void OnDraw();
	virtual void OnMouseHover(int x, int y);
	void SetToolTip(const char* toolTip);
	void SetText(std::string txt) { m_text = txt; }
	void SetDiabled(int disabled) { m_isDisabled = disabled; }
protected:
	std::string m_text;
	std::string m_toolTip;
	int m_isDisabled;
};

class UITextButton : public UIButton {
public:
	UITextButton();
	virtual ~UITextButton();
	void Create(const char* text, unsigned int fontColor, int fontFace, int fontSize, bool isFontBold);
	virtual void OnDraw();
	virtual void OnLBtnUp(int x, int y);
	virtual void OnRBtnDown(int x, int y);
	virtual void OnRBtnUp(int x, int y);
	virtual void OnMouseMove(int x, int y);
	virtual void OnMouseHover(int x, int y);
	void Resize();
	void Resize2(int x, int y);
	void SetFontBold(unsigned char);
	void SetFontColor(unsigned int);
	void SetSelectColor(unsigned int selectColor) { m_selectColor = selectColor; }
	void SetId2(int id) { m_id2 = id; }
protected:
	unsigned int m_btnColor;
	unsigned int m_fontColor;
	unsigned int m_selectColor;
	unsigned int m_fontBgColor;
	bool m_isFontBold;
	int m_fontSize;
	int m_fontFace;
	int m_id2;
	std::string m_text2;
};



enum {
	BS_NORMAL,
	BS_MOUSEON,
	BS_PRESSED,
};

class UIBitmapButton : public UIButton {
public:
	UIBitmapButton();
	virtual ~UIBitmapButton();
	virtual void OnDraw();
	void SetBitmapName(const char* bitmapName, int buttonState);
	int GetBitmapWidth() { return m_bitmapWidth; }
	int GetBitmapHeight() { return m_bitmapHeight; }
private:
	int m_bitmapWidth;
	int m_bitmapHeight;
	std::string m_normalBitmapName;
	std::string m_mouseonBitmapName;
	std::string m_pressedBitmapName;
};

class UIBitmapButton2 : public UIBitmapButton {
public:
	UIBitmapButton2();
	virtual ~UIBitmapButton2();
	virtual void OnLBtnUp(int x, int y);
	virtual void OnRBtnDown(int x, int y);
	virtual void OnRBtnUp(int x, int y);
	virtual void DragAndDrop(int x, int y, const DRAG_INFO* const dragInfo);
};

class UITabControl : public UIWindow {
public:
	UITabControl();
	virtual ~UITabControl();
	virtual void OnDraw();
	virtual void OnLBtnDown(int x, int y);
	void Create(bool bVerticalType, int width, int height, unsigned int onColor, unsigned int offColor, unsigned int bgColor, unsigned int btmColor);
	int GetTabWidth();
	int GetTabHeight();
	unsigned int GetColorOnTab();
	unsigned int GetColorOffTab();
	void SetTabNum(int);
	void SetTabString(const char* tabString);
protected:
	void ChangeTab();
protected:
	unsigned char m_bVerticalType;
	int m_tab;
	int m_maxTab;
	int m_tabHeight;
	int m_tabWidth;
	unsigned int m_colorOnTab;
	unsigned int m_colorOffTab;
	unsigned int m_colorBgTab;
	unsigned int m_colorBtmTab;
	std::list<std::string> m_TabStrings;
};

class UINoneVisibleButton : public UIButton {
public:
	UINoneVisibleButton();
	virtual ~UINoneVisibleButton();
	virtual void OnDraw();
};

class UIColorButton : public UIButton {
public:
	UIColorButton();
	virtual ~UIColorButton();
	virtual void OnDraw();
protected:
	int m_r;
	int m_g;
	int m_b;
	int m_br;
	int m_bg;
	int m_bb;
};

class UISpriteButton : public UIButton {
public:
	UISpriteButton();
	virtual ~UISpriteButton();
	virtual void OnDraw();
	virtual void OnLBtnDown(int x, int y);
	virtual void OnMouseMove(int x, int y);
	void SetAction(int action) { m_curAct = action; }
	void SetMotion(int motion) { m_curMot = motion; }
	void SetAnimation(int);
	void SetFitToSize(int);
	void SetSpriteName(const char* actName);
	void SetActFileName(const char* actName);
	int GetSprWidth(int act, int mot);
	int GetSprHeight(int act, int mot);
	int GetCurAction();
	int GetCurMotion();
	int GetMaxAction();
	int GetMaxMotion();
	int IsAnimation() { return m_isAnimation; }
	int IsFitToSize();
private:
	int m_maxAct;
	int m_maxMot;
	int m_curAct;
	int m_curMot;
	int m_isAnimation;
	int m_isFitToSize;
	std::string m_strSprName;
	std::string m_strActName;
};

class UIListBox : public UIWindow {
public:
	UIListBox();
	virtual ~UIListBox();
	virtual void Create2(int x, int y, int cx, int cy, bool trans);
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnWheel(int wheel);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual void AddItem(const char* txt);
	virtual void ClearAllItems();
	virtual void Resize(int cx, int cy);
	virtual void RecalcScrbarPos();
	int IsVertScrEnabled() { return m_vertScrEnabled; }
	int GetSelected() { return m_curItem; }
	int GetNumItem() { return m_items.size(); }
	std::string& GetItem(int item) { return m_items[item]; }
protected:
	int HitTest(int x, int y);
public:
	int m_bR;
	int m_bG;
	int m_bB;
protected:
	std::vector<std::string> m_items;
	int m_curItem;
	int m_vertViewOffset;
	int m_horzViewOffset;
	int m_maxTextWidth;
	int m_vertScrEnabled;
	int m_horzScrEnabled;
	UIScrollBar* m_vertScrollBar;
	UIScrollBar* m_horzScrollBar;
	int m_isTransParent;
	int m_itemSpacing;
	char m_noColor;
};

class UIComboBox : public UIListBox {
public:
	UIComboBox();
	virtual ~UIComboBox();
	virtual void Create2(int x, int y, int cx, int cy, bool trans);
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnLBtnUp(int cx, int cy);
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnMouseMove(int cx, int cy);
	virtual void OnDraw();
	virtual void Resize(int cx, int cy);
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual void RecalcScrbarPos();
protected:
	WINDOWID m_callerWid;
};

class UIChatHisBox : public UIListBox {
public:
	UIChatHisBox();
	virtual ~UIChatHisBox();
	virtual void Create2(int x, int y, int cx, int cy, bool trans);
	virtual void OnCreate(int cx, int cy);
	virtual void OnLBtnDown(int cx, int cy);
	virtual void OnLBtnDblClk(int cx, int cy);
	virtual void OnLBtnUp(int cx, int cy);
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	virtual bool IsTransmitMouseInput() { return true; }
	virtual void AddItem(const char* txt, unsigned int itemRGB, const char* name);
	virtual void ClearAllItems();
	virtual void Resize(int cx, int cy);
	virtual void SetShow(int visible);
	void DeleteFirstHalfItem();
	unsigned int GetItemColor(int);
	int WriteChatToReportFile(const char* fileName);
	int WriteChatToFile(const char* fileName);
	void AddItem2(const char* txt, unsigned int itemRGB, const char* name);
protected:
	const char* GetTalkerName(int item);
protected:
	std::vector<std::string> m_talkerNameList;
	std::vector<unsigned int> m_itemColors;
	int m_maxItem;
};

class UIServerListBox : public UIListBox {
public:
	UIServerListBox();
	virtual ~UIServerListBox();
	virtual void OnMouseHover(int x, int y);
	std::vector<std::string> m_strings;
	UIBalloonText* m_serverBalloon;
};

class UIChatHisBox2 : public UIChatHisBox {
public:
	UIChatHisBox2();
	virtual ~UIChatHisBox2();
	virtual void Create2(int x, int y, int cx, int cy, bool trans);
	virtual void OnCreate(int cx, int cy);
	virtual void OnDraw();
	virtual void OnRBtnUp(int cx, int cy);
	virtual bool IsTransmitMouseInput() { return false; }
	int m_r;
	int m_g;
	int m_b;
};

class UIMemberListBox : public UIChatHisBox2 {
public:
	UIMemberListBox();
	virtual ~UIMemberListBox();
	virtual void OnDraw();
	virtual void OnMouseHover(int x, int y);
	virtual void OnLBtnDown(int x, int y);
	virtual void OnRBtnDown(int x, int y);
	virtual void OnRBtnUp(int x, int y);
};

class UIChatHisBox3 : public UIChatHisBox2 {
public:
	UIChatHisBox3();
	virtual ~UIChatHisBox3();
	virtual void OnCreate(int cx, int cy);
	virtual void Resize(int cx, int cy);
	virtual void OnDraw();
	virtual void AddItem(const char* txt, unsigned int itemRGB);
	void SetItemSpacing(int pos) { m_itemSpacing = pos; }
	void SetBgColor(int r, int g, int b) { m_bR = r; m_bG = g; m_bB = b; }
	void SetFontSize(int pos);
	void SetType(int pos);
	int GetFontSize();
	void AddItem2(const char* txt, unsigned int itemRGB, const char* tab);
	void SetScrollPos(int pos);
protected:
	int m_type;
	int m_fontSize;
};

class UITextViewer : public UIListBox {
public:
	UITextViewer();
	virtual ~UITextViewer();
	virtual void Create2(int x, int y, int cx, int cy, bool trans);
	virtual void OnCreate(int x, int y);
	virtual void OnLBtnDown(int x, int y) {}
	virtual void OnLBtnDblClk(int x, int y) {}
	virtual void OnLBtnUp(int x, int y) {}
	virtual void OnDraw();
	virtual int SendMsg(UIWindow* sender, int message, int val1, int val2, int val3, int val4);
	int m_bgR;
	int m_bgG;
	int m_bgB;
};






struct StringAndColor {
public:
	std::string String;
	unsigned int Color;
};

struct WhisperListWndInfo {
public:
	void MakeDefault();
	int IsValidInfo();
	int x;
	int y;
	int w;
	int h;
	int show;
};

class UIWindowMgr {
public:
	UIWindowMgr();
	virtual ~UIWindowMgr();
	void ClearDC(unsigned int color);
	void DestroyDC();
	void OnProcess();
	void Render();
	void AddWindow(UIWindow* snapWindow);
	void AddWindowFront(UIWindow* snapWindow);
	void RemoveAllWindows();
	void RemoveAllWindowsExceptChatWnd();
	void ClearAllWindowState();
	void HideChatWnd();
	void MakeTopLayer(UIWindow* snapWindow);
	void InvalidateUpdateNeededUI();
	void InvalidateAll();
	void InvalidateGuildWnd();
	void RenderWallPaper();
	void RenderTitleGraphic();
	void SetCapture(UIWindow* snapWindow);
	void ReleaseCapture();
	void FinishEdit();
	void SetFocusEdit(UIWindow* snapWindow);
	void SetFocusEditPrev();
	void SetFocusEditFirst();
	void SetFocusEditNext();
	void SetFocusEditLast();
	void PostQuit(UIWindow* snapWindow);
	void DeleteWhisperWindow(std::string wName);
	void AddToNameWaitingWindowList(UIWindow* snapWindow);
	void RemoveFromNameWaitingWindowList(UIWindow* snapWindow);
	void RefreshNameWaitingWindowList();
	void SetModalResult(int result) { m_modalResult = result; }
	void DefPushButton();
	void CancelPushButton();
	void ToggleButton(int toggleDir);
	void PageUpButton();
	void PageDownButton();
	void UpButton();
	void DownButton();
	void LeftButton();
	void RightButton();
	void DeleteButton();
	void RefreshItem();
	void MakeWndPointerNull();
	void AddWhisperCharName(const char* friendCharName);
	void AddWhisperCharName2(const char* friendCharName);
	void AddFriendCharName(const char* friendCharName);
	void DeleteFriendName(std::string wName);
	void WriteWhisperChatToFile(const char* friendCharName);
	void SetTextAtBattleMode();
	void SetWallpaper(CBitmapRes* bitmap);
	void DrawBitmap(int x, int y, CBitmapRes* bitmap);
	void DrawBitmapToFrame(int x, int y, CBitmapRes* bitmap);
	void SetChatMsg(const char* msg) { m_chatMsg = msg; }
	void ClearChatMsg() { m_chatMsg = ""; }
	void ProcessShortcutMessage(int toggleDir);
	void StoreChatWndStatus();
	void RestoreChatWndStatus();
	void InitShortcutItemInfoFromReg();
	void InitUIRectInfoFromReg();
	void WriteUIRectInfoToReg();
	void WriteShorcutItemInfoToReg();
	void InitSkillUseLevelInfoFromReg();
	void WriteSkillUseLevelInfoToReg();
	void MakeDefaultUIRectInfo();
	void EditFocusToChatWnd();
	void FindFocusEditAndSetFocus();
	void SetSize(int cx, int cy);
	void UpdateSnapInfo(UIWindow* snapWindow);
	void GetDeltaSnap(UIWindow* snapWindow, int& isConnected, int& rdx, int rdy);
	void DeleteSnapInfo(UIWindow* snapWindow);
	int IsFriendName(const char* cName);
	int AddWhisperChatToWhisperWnd(const char* cName, const char* chatMsg, unsigned int color);
	bool ProcessPushButton(unsigned int virtualKey, int newKeydown, int PASS);
	bool IsFirstFocusEdit(UIWindow* wnd);
	bool IsLastFocusEdit(UIWindow* wnd);
	bool ExcuteMsgInBattleMode(unsigned int virtualKey, int newKeydown);
	bool ExcuteMsgInBattleModeVietnam(int lnKey);
	bool isFocusChatWnd();
	bool DeleteWindow(WINDOWID windowId);
	bool ShowWindow(WINDOWID windowId, int show);
	unsigned char IsModalStatus() { return m_modalWindow != NULL; }
	bool Drop(int dropX, int dropY, const DRAG_INFO* const dragInfo);
	unsigned char IsMinimapVisible();
	int ProcessInput();
	int DoModal(UIWindow* wnd);
	int ErrorMsg(const char* msg, int type, int isDefYes, int changeMsg, unsigned int autoReturnTime);
	int MGScoreMsg(unsigned int, int, int, int, unsigned int);
	int ErrorMsgAutoreturn(const char* msg, unsigned int autoReturnTime);
	int EmailAddress();
	int GetWallpaperLeft();
	int GetWallpaperTop();
	int SendMsg(int message, int val1, int val2, int val3, int val4);
	int GetHeight();
	int GetWidth();
	UIWindow* GetCapture();
	UIWindow* GetFocusEdit();
	UIWindow* HitTest(int x, int y);
	UIWindow* QueryWindow(WINDOWID windowId);
	UIFrameWnd* MakeWindow(WINDOWID windowId);
	const char* GetChatMsg() { return m_chatMsg.c_str(); }
	std::vector<std::string>& GetFriendCharNameList();
	UISayDialogWnd* GetSayDialogWnd() { return m_sayDialogWnd; }
	UIChooseWnd* GetChooseWnd() { return m_chooseWnd; }
	UIItemIdentifyWnd* GetItemIdentifyWnd();
	UIItemRepairWnd* GetItemRepairWnd();
	UIWeaponRefineWnd* GetWeaponRefineWnd();
	UIItemCompositionWnd* GetItemCompositionWnd();
	UIStatusWnd* GetStatusWnd() { return m_statusWnd; }
	UIBasicInfoWnd* GetBasicInfoWnd() { return m_basicInfoWnd; }
	UIItemShopWnd* GetItemShopWnd() { return m_itemShopWnd; }
	UIItemPurchaseWnd* GetItemPurchaseWnd() { return m_itemPurchaseWnd; }
	UIItemSellWnd* GetItemSellWnd() { return m_itemSellWnd; }
	UIComboBoxWnd* GetComboBoxWnd() { return m_comboBoxWnd; }
	UIChatRoomWnd* GetChatRoom() { return m_chatRoomWnd; }
	UIExchangeWnd* GetExchangeWnd() { return m_exchangeWnd; }
	UIExchangeAcceptWnd* GetExchangeAcceptWnd() { return m_exchangeAcceptWnd; }
	UIEquipWnd* GetEquipWnd() { return m_equipWnd; }
	UINewChatWnd* GetChatWnd() { return m_chatWnd; }
	UIItemStoreWnd* GetItemStoreWnd() { return m_itemStoreWnd; }
	UISkillListWnd* GetSkillListWnd() { return m_skillListWnd; }
	UIItemWnd* GetItemWnd() { return m_itemWnd; }
	UIQuestWnd* GetQuestWnd() { return m_questWnd; }
	UIQuestDetailWnd* GetQuestDetailWnd() { return m_questDetailWnd; }
	UIMessengerGroupWnd* GetMessengerGroupWnd() { return m_messengerGroupWnd; }
	UIMenuWnd* GetMenu() { return m_menu; }
	UIItemDropCntWnd* GetItemDropCntWnd() { return m_itemDropCntWnd; }
	UIMerchantItemWnd* GetMerchantItemWnd() { return m_merchantItemWnd; }
	UIMerchantShopMakeWnd* GetMerchantShopMakeWnd() { return m_merchantShopMakeWnd; }
	UIMerchantMirrorItemWnd* GetMerchantMirrorItemWnd() { return m_merchantMirrorItemWnd; }
	UIMerchantItemShopWnd* GetMerchantItemShopWnd() { return m_merchantItemShopWnd; }
	UIMerchantItemMyShopWnd* GetMerchantItemMyShopWnd() { return m_merchantItemMyShopWnd; }
	UIMerchantItemPurchaseWnd* GetMerchantItemPurchaseWnd() { return m_merchantItemPurchaseWnd; }
	UIShortCutWnd* GetShortCutWnd() { return m_shortCutWnd; }
	UIRestartWnd* GetRestartWnd();
	UIOptionWnd* GetOptionWnd() { return m_optionWnd; }
	UIItemParamChangeDisplayWnd* GetItemParamChangeDisplayWnd() { return m_itemParamChangeDisplayWnd; }
	UICandidateWnd* GetCandidateWnd() { return m_candidateWnd; }
	UICompositionWnd* GetCompositionWnd();
	UIKeyStrokeWnd* GetKeyStrokeWnd() { return m_keyStrokeWnd; }
	UIPartySettingWnd* GetPartySettingWnd() { return m_partySettingWnd; }
	UIFriendOptionWnd* GetFriendOptionWnd();
	UIMakeTargetProcessWnd* GetMakeTargetProcessWnd();
	UIGuildInfoManageWnd* GetGuildInfoManageWnd() { return m_guildInfoManageWnd; }
	UIGuildMemberManageWnd* GetGuildMemberManageWnd();
	UIGuildPositionManageWnd* GetGuildPositionManageWnd() { return m_guildPositionManageWnd; }
	UIGuildSkillWnd* GetGuildSkillWnd() { return m_guildSkillWnd; }
	UIGuildBanishedMemberWnd* GetGuildBanishedMemberWnd();
	UIGuildNoticeWnd* GetGuildNoticeWnd();
	UIGuildTotalInfoWnd* GetGuildTotalInfoWnd();
	UIEmotionWnd* GetEmotionWnd() { return m_emotionWnd; }
	UITipOfTheDayWnd* GetTipOfDayWnd() { return m_tipOfTheDayWnd; }
	UIPetInfoWnd* GetPetInfoWnd() { return m_petInfoWnd; }
	UIPetTamingDeceiveWnd* GetPetTamingDeceiveWnd() { return m_petTamingDeceiveWnd; }
	UIMakingArrowListWnd* GetMakingArrowListWnd();
	UISelectCartWnd* GetSelectCartWnd();
	UIChatRoomWnd* GetChatRoomWnd() { return m_chatRoomWnd; }
	UIEmotionListWnd* GetEmotionListWnd() { return m_emotionListWnd; }
	UIChatRoomMakeWnd* GetChatRoomMakeWnd() { return m_chatRoomMakeWnd; }
	UIIllustWnd* GetIllustWnd() { return m_illustWnd; }
	UIWindow** GetChatRoomWndPointer();
	UIWindow** GetMessengerGroupWndPointer();
	UIWindow** GetSkillListWndPointer();
	UIWindow** GetGuildMemberManageWndPointer();
	UIWindow** GetGuildInfoManageWndPointer();
	UICommandListWnd* GetUICommandListWnd() { return m_commandListWnd; }
	UIHomunInfoWnd* GetHomunInfoWnd() { return m_homunInfoWnd; }
	UISkillListWnd* GetHomunSkillListWnd() { return m_homunSkillListWnd; }
	UIMerInfoWnd* GetMerInfoWnd() { return m_merInfoWnd; }
	UISkillListWnd* GetMerSkillListWnd() { return m_merSkillListWnd; }
	UIMailListWnd* GetMailListWnd() { return m_mailListWnd; }
	UIMailViewWnd* GetMailViewWnd() { return m_mailViewWnd; }
	UIAuctionWnd* GetAuctionWnd() { return m_auctionWnd; }
	UIAuctionMsgWnd* GetAuctionMsgWnd() { return m_auctionMsgWnd; }
	UIImgMenuWnd* GetImgMenuWnd() { return m_imgMenuWnd; }
	UIChangeNameWnd* GetChangeNameWnd() { return m_changeNameWnd; }
	UISelectCharWnd* GetSelectCharWnd() { return m_selectCharWnd; }
	UISubChatHisWnd* GetSubChatWndST() { return m_subChatWndST; }
	UISubChatHisWnd* GetSubChatWndBT() { return m_subChatWndBT; }
	UISubChatMiniWnd* GetSubChatMiniWnd() { return m_subChatMiniWnd; }
	bool IsChatWndStickOn() { return m_chatWndStickOn; }
	void ChatWndStickOnOff(bool onOff) { m_chatWndStickOn = onOff; }
	unsigned char IsStSubChatOn() { return m_onStSubChat; }
	unsigned char IsBtSubChatOn() { return m_onBtSubChat; }
	void StSubChatOnOff() { m_onStSubChat = !m_onStSubChat; }
	void BtSubChatOnOff() { m_onBtSubChat = !m_onBtSubChat; }
	UIMemorialDunWnd* GetMemorialDunWnd() { return m_memorialDunWnd; }
	UIRoMapWnd* GetRoMapWnd() { return m_roMapWnd; }
	bool IsAniAccessory(int accessory);
	void ReadToReg();
private:
	void CreateDCA(int cx, int cy);
	void RemoveWindow(UIWindow* snapWindow);
	void GetWindowPos(UIWindow* window, int* x, int* y);
	void AddWhisperWindow(std::string wName, UIWindow* whisperWindow);
public:


	friend class CGameMode;
	friend class UIItemWnd;
	int m_chatWndX;
	int m_chatWndY;
	int m_chatWndHeight;
	int m_chatWndShow;
	int m_gronMsnWndShow;
	int m_gronMsgWndShow;
	int m_chatWndStatus;
	float m_miniMapZoomFactor;
	unsigned int m_miniMapArgb;
	int m_isDrawCompass;
	int m_isDragAll;
	int m_conversionMode;
private:
	std::list<UIWindow *> m_children;
	std::list<UIWindow *> m_quitWindow;
	std::list<UIWindow *> m_nameWaitingList;
	std::map<UIWindow *, CSnapInfo> m_snapInfo;
	UIWindow* m_captureWindow;
	UIWindow* m_editWindow;
	UIWindow* m_modalWindow;
	UIWindow* m_lastHitWindow;
	UILoadingWnd* m_loadingWnd;
	UIMinimapZoomWnd* m_minimapZoomWnd;
	UIStatusWnd* m_statusWnd;
	UINewChatWnd* m_chatWnd;
	UILoginWnd* m_loginWnd;
	UIItemWnd* m_itemWnd;
	UIQuestWnd* m_questWnd;
	UIBasicInfoWnd* m_basicInfoWnd;
	UIEquipWnd* m_equipWnd;
	UIOptionWnd* m_optionWnd;
	UIShortCutWnd* m_shortCutWnd;
	UIItemDropCntWnd* m_itemDropCntWnd;
	UISayDialogWnd* m_sayDialogWnd;
	UIChooseWnd* m_chooseWnd;
	UIItemIdentifyWnd* m_itemIdentifyWnd;
	UIItemRepairWnd* m_itemRepairWnd;
	UIWeaponRefineWnd* m_weaponRefineWnd;
	UIItemCompositionWnd* m_itemCompositionWnd;
	UIChooseWarpWnd* m_chooseWarpWnd;
	UIMenuWnd* m_menu;
	UIComboBoxWnd* m_comboBoxWnd;
	UIItemCollectionWnd* m_itemCollectionWnd;
	UICombinedCardItemCollectionWnd* m_combinedCardItemCollectionWnd;
	UIItemParamChangeDisplayWnd* m_itemParamChangeDisplayWnd;
	UISkillDescribeWnd* m_skillDescribeWnd;
	UIQuestDetailWnd* m_questDetailWnd;
	UIQuitWnd* m_quitWnd;
	UIRestartWnd* m_restartWnd;
	UICardItemIllustWnd* m_cardItemIllustWnd;
	UINotifyLevelUpWnd* m_notifyLevelUpWnd;
	UINotifyJobLevelUpWnd* m_notifyJobLevelUpWnd;
	UINotifyQuestWnd* m_notifyQuestWnd;
	UIItemShopWnd* m_itemShopWnd;
	UIItemPurchaseWnd* m_itemPurchaseWnd;
	UIItemSellWnd* m_itemSellWnd;
	UIChooseSellBuyWnd* m_chooseSellBuyWnd;
	UIChatRoomMakeWnd* m_chatRoomMakeWnd;
	UIChatRoomChangeWnd* m_chatRoomChangeWnd;
	UIChatRoomWnd* m_chatRoomWnd;
	UIPasswordWnd* m_passwordWnd;
	UIExchangeWnd* m_exchangeWnd;
	UIExchangeAcceptWnd* m_exchangeAcceptWnd;
	UIItemStoreWnd* m_itemStoreWnd;
	UISkillListWnd* m_skillListWnd;
	UIMessengerGroupWnd* m_messengerGroupWnd;
	UIJoinPartyAcceptWnd* m_joinPartyAcceptWnd;
	UICoupleAcceptWnd* m_CoupleAcceptWnd;
	UIStarPlaceAcceptWnd* m_StarPlaceAcceptWnd;
	UIYourItemWnd* m_YourItemWnd;
	UIBabyAcceptWnd* m_BabyAcceptWnd;
	UIBabyAcceptWnd2* m_BabyAcceptWnd2;
	UIJoinGuildAcceptWnd* m_joinGuildAcceptWnd;
	UIAllyGuildAcceptWnd* m_allyGuildAcceptWnd;
	UITipOfTheDayWnd* m_tipOfTheDayWnd;
	UIMerchantItemWnd* m_merchantItemWnd;
	UIMerchantMirrorItemWnd* m_merchantMirrorItemWnd;
	UIMerchantShopMakeWnd* m_merchantShopMakeWnd;
	UIMerchantItemShopWnd* m_merchantItemShopWnd;
	UIMerchantItemMyShopWnd* m_merchantItemMyShopWnd;
	UIMerchantItemPurchaseWnd* m_merchantItemPurchaseWnd;
	UIPartySettingWnd* m_partySettingWnd;
	UISkillNameChangeWnd* m_skillNameChangeWnd;
	UINpcEditDialogWnd* m_npcEditDialogWnd;
	UINpcTextEditDialogWnd* m_npcTextEditDialogWnd;
	UIDetailLevelWnd* m_detailLevelWnd;
	UINotifyItemObtainWnd* m_notifyItemObtainWnd;
	UIGuildInfoManageWnd* m_guildInfoManageWnd;
	UIGuildMemberManageWnd* m_guildMemberManageWnd;
	UIGuildPositionManageWnd* m_guildPositionManageWnd;
	UIGuildSkillWnd* m_guildSkillWnd;
	UIGuildBanishedMemberWnd* m_guildBanishedMemberWnd;
	UIGuildNoticeWnd* m_guildNoticeWnd;
	UIGuildTotalInfoWnd* m_guildTotalInfoWnd;
	UIFriendOptionWnd* m_friendOptionWnd;
	UIGuildLeaveReasonDescWnd* m_guildLeaveReasonDescWnd;
	UIGuildBanReasonDescWnd* m_guildBanReasonDescWnd;
	UIIllustWnd* m_illustWnd;
	UIMakeTargetListWnd* m_metalListWnd;
	UIMakeTargetProcessWnd* m_metalProcessWnd;
	UIMakeTargetResultWnd* m_metalResultWnd;
	UIEmotionWnd* m_emotionWnd;
	UIEmotionListWnd* m_emotionListWnd;
	UIProhibitListWnd* m_prohibitListWnd;
	UIPetInfoWnd* m_petInfoWnd;
	UIPetEggListWnd* m_selectPetEggWnd;
	UIPetTamingDeceiveWnd* m_petTamingDeceiveWnd;
	UIMakingArrowListWnd* m_makingArrowListWnd;
	UISelectCartWnd* m_selectCartWnd;
	UISpellListWnd* m_spellListWnd;
	UICandidateWnd* m_candidateWnd;
	UICompositionWnd* m_compositionWnd;
	UIKeyStrokeWnd* m_keyStrokeWnd;
	UIBookWnd* m_bookWnd;
	UIJoinFriendAcceptWnd* m_joinFriendAcceptWnd;
	UICommandListWnd* m_commandListWnd;
	UIHomunInfoWnd* m_homunInfoWnd;
	UISkillListWnd* m_homunSkillListWnd;
	UIMerInfoWnd* m_merInfoWnd;
	UISkillListWnd* m_merSkillListWnd;
	UIAutoMessageWnd* m_autoMessageWnd;
	UIMailListWnd* m_mailListWnd;
	UIMailViewWnd* m_mailViewWnd;
	UIAuctionWnd* m_auctionWnd;
	UIAuctionMsgWnd* m_auctionMsgWnd;
	UIImgMenuWnd* m_imgMenuWnd;
	UISelectCharWnd* m_selectCharWnd;
	UISelCharForUServerWnd* m_selCharForUServerWnd;
	UIChangeNameWnd* m_changeNameWnd;
	UIRebirthWnd* m_rebirthWnd;
	UISubChatHisWnd* m_subChatWndST;
	UISubChatHisWnd* m_subChatWndBT;
	UIBattleMsgOptionWnd* m_battleMsgOpionWnd;
	UISubChatMiniWnd* m_subChatMiniWnd;
	bool m_chatWndStickOn;
	unsigned char m_onStSubChat;
	unsigned char m_onBtSubChat;
	UIMemorialDunWnd* m_memorialDunWnd;
	UIEquipWnd* m_otherUserEquipWnd;
	UIRoMapWnd* m_roMapWnd;
	int m_lastMouseX;
	int m_lastMouseY;
	int m_modalResult;
	CSurface* m_wallpaperSurface;
	int m_w;
	int m_h;
	std::string m_wallPaperBmpName;
	short T_Logo_X;
	short T_Logo_Y;
	short T_Char_X;
	short T_Char_Y;
	short T_Tape_Y;
	short T_12Age_X;
	short T_12Age_Y;
	short T_Grav_X;
	short T_Grav_Y;
	short T_Intel_X;
	short T_Intel_Y;
	char T_CLICK;
	std::list<StringAndColor> m_chatWndHistory;
	std::list<std::string> m_chatWndMyTalk;
	std::list<std::string> m_chatWndWhisper;
	std::string m_chatMsg;
	int m_chatWndCurHis;
	int m_chatWndWhisperCurHis;
	int m_isInvalidatedByForce;
	std::map<std::string, UIWindow *> m_whisperNameWindow;
	BasicInfoWndInfo basicInfoWndInfo;
	ItemWndInfo itemWndInfo;
	QuestWndInfo questWndInfo;
	StatusWndInfo statusWndInfo;
	EquipWndInfo equipWndInfo;
	OptionWndInfo optionWndInfo;
	ShortenItemWndInfo shortenItemWndInfo;
	ShortenSkillWndInfo shortenSkillWndInfo;
	SayDialogWndInfo sayDialogWndInfo;
	ChooseWndInfo chooseWndInfo;
	ChatRoomWndInfo chatRoomWndInfo;
	ItemStoreWndInfo itemStoreWndInfo;
	ItemShopWndInfo itemShopWndInfo;
	ItemSellWndInfo itemSellWndInfo;
	ItemPurchaseWndInfo itemPurchaseWndInfo;
	MessengerGroupWndInfo messengerGroupWndInfo;
	ShortCutWndInfo shortCutWndInfo;
	ExchangeAcceptWndInfo exchangeAcceptWndInfo;
	SkillListWndInfo skillListWndInfo;
	MerchantItemWndInfo merchantItemWndInfo;
	MerchantMirrorItemWndInfo merchantMirrorItemWndInfo;
	MerchantShopMakeWndInfo merchantShopMakeWndInfo;
	MerchantItemShopWndInfo merchantItemShopWndInfo;
	MerchantItemMyShopWndInfo merchantItemMyShopWndInfo;
	MerchantItemPurchaseWndInfo merchantItemPurchaseWndInfo;
	ItemCollectionWndInfo itemCollectionWndInfo;
	CombinedCardItemCollectionWndInfo combinedCardItemCollectionWndInfo;
	ItemParamChangeDisplayWndInfo itemParamChangeDisplayWndInfo;
	PartySettingWndInfo partySettingWndInfo;
	DetailLevelWndInfo detailLevelWndInfo;
	GuildWndInfo guildWndInfo;
	WhisperWndInfo whisperWndInfo;
	WhisperListWndInfo whisperListWndInfo;
	FriendOptionWndInfo friendOptionWndInfo;
	ItemCompositionWndInfo itemCompositionWndInfo;
	ItemIdentifyWndInfo itemIdentifyWndInfo;
	EmotionWndInfo emotionWndInfo;
	EmotionListWndInfo emotionListWndInfo;
	PetInfoWndInfo petInfoWndInfo;
	HomunInfoWndInfo homunInfoWndInfo;
	SkillListWndInfo homunSkillListWndInfo;
	MerInfoWndInfo merInfoWndInfo;
	SkillListWndInfo merSkillListWndInfo;
	SubChatWndInfo subChatWndStInfo;
	SubChatWndInfo subChatWndBtInfo;
};

extern CRenderer* g_renderer;
inline int UIX(int x)
{
	return (g_renderer->m_width - 640) / 2 + x;
}
inline int UIY(int y)
{
	return (g_renderer->m_height - 480) / 2 + y;
}
inline int UICY(int y)
{
	return g_renderer->m_height * y / 480;
}

#endif
