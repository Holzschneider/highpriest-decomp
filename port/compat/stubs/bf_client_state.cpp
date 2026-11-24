// The client's state as the control socket names it (PROTOCOL.md section 3,
// part `mode`), read from the mode manager. Compiled with the client's flags
// and headers (BF_CLIENT_PORT_SOURCES in CMakeLists.txt), so that the start-up
// glue in bf_main.cpp needs no client header.
#include "common.h"
#include "Mode.h"
#include "LoginMode.h"
#include "window/UISys.h"
#include "window/UIFrameWnd.h"
#include "ItemInfo.h"
#include "Session.h"

#include <cxxabi.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <typeinfo>
#include <vector>

#include "bf_client_ui.h"

extern UIWindowMgr g_windowMgr;
extern int g_bfModeRunning;   // Mode.cpp: a mode is in OnRun

extern "C" const char *bf_port_mode(const char **detail);

namespace {

struct SubMode {
	int id;
	const char *mode;
	const char *detail;
};

const SubMode kLogin[] = {
	{ LSM_WARNING,                        "login",      "LSM_WARNING" },
	{ LSM_LICENCE,                        "login",      "LSM_LICENCE" },
	{ LSM_SELECTACCOUNT,                  "login",      "LSM_SELECTACCOUNT" },
	{ LSM_IDENTRY,                        "login",      "LSM_IDENTRY" },
	{ LSM_WAITRESP_FROM_ACSVR,            "connecting", "LSM_WAITRESP_FROM_ACSVR" },
	{ LSM_WAITRESP_FROM_CHSVR,            "connecting", "LSM_WAITRESP_FROM_CHSVR" },
	{ LSM_SELECTSVR,                      "servers",    "LSM_SELECTSVR" },
	{ LSM_SELECTCHAR,                     "charselect", "LSM_SELECTCHAR" },
	{ LSM_MAKECHAR,                       "makechar",   "LSM_MAKECHAR" },
	{ LSM_WAITRESP_FROM_CHSVR_SELECTCHAR, "charselect", "LSM_WAITRESP_FROM_CHSVR_SELECTCHAR" },
	{ LSM_WAITRESP_FROM_CHSVR_MAKECHAR,   "makechar",   "LSM_WAITRESP_FROM_CHSVR_MAKECHAR" },
	{ LSM_WAITRESP_FROM_CHSVR_DELETECHAR, "charselect", "LSM_WAITRESP_FROM_CHSVR_DELETECHAR" },
	{ LSM_READY_FOR_ZONESVR,              "loading",    "LSM_READY_FOR_ZONESVR" },
	{ LSM_SELECTACCOUNT2,                 "login",      "LSM_SELECTACCOUNT2" },
	{ LSM_REQ_GAME_GUARD_CHECK,           "connecting", "LSM_REQ_GAME_GUARD_CHECK" },
	{ LSM_ACK_GAME_GUARD,                 "connecting", "LSM_ACK_GAME_GUARD" },
	{ LSM_REQ_GAME_GUARD_CHECK2,          "connecting", "LSM_REQ_GAME_GUARD_CHECK2" },
	{ LSM_REQ_PTNUM,                      "connecting", "LSM_REQ_PTNUM" },
	{ LSM_REQ_PTACINFO,                   "connecting", "LSM_REQ_PTACINFO" },
	{ LSM_SELECTCHAR2,                    "charselect", "LSM_SELECTCHAR2" },
};

}  // namespace

// One of start, login, connecting, servers, charselect, makechar, loading,
// game, quitting; *detail the client's own name for it.
extern "C" const char *bf_port_mode(const char **detail)
{
	const char *unused;
	if (!detail)
		detail = &unused;
	if (g_sysQuit || !g_modeMgr.GetLoopCond()) {
		*detail = "g_sysQuit";
		return "quitting";
	}
	if (g_modeMgr.GetGameMode()) {
		// OnInit loads the map; the game is played once OnRun runs, and
		// `map` and `self` are reported from then on (bf_client_game.cpp).
		if (!g_bfModeRunning) {
			*detail = "MT_GAME (loading)";
			return "loading";
		}
		*detail = "MT_GAME";
		return "game";
	}
	CLoginMode *login = g_modeMgr.GetLoginMode();
	if (!login || !g_modeMgr.GetCurMode()) {
		*detail = "none";
		return "start";
	}
	int sub = login->GetSubMode();
	for (size_t i = 0; i < sizeof(kLogin) / sizeof(kLogin[0]); ++i) {
		if (kLogin[i].id == sub) {
			*detail = kLogin[i].detail;
			return kLogin[i].mode;
		}
	}
	*detail = "LSM_?";
	return "login";
}

// ---- the open UI windows (PROTOCOL.md section 3, `windows` and `controls`)

namespace {

const char *const kWindowNames[] = {
	"BASICINFOWND",
	"CHATWND",
	"SELECTSERVERWND",
	"LOGINWND",
	"MAKECHARWND",
	"SELECTCHARWND",
	"WAITWND",
	"LOADINGWND",
	"ITEMWND",
	"TOOLTIPWND",
	"EQUIPWND",
	"STATUSWND",
	"ITEMCOLLECTIONWND",
	"OPTIONWND",
	"MINIMAPZOOMWND",
	"ITEMDROPCNTWND",
	"SAYDIALOGWND",
	"CHOOSEWND",
	"MENUWND",
	"RESTARTWND",
	"NOTICECONFIRMWND",
	"NOTIFYLEVELUPWND",
	"ITEMSHOPWND",
	"ITEMPURCHASEWND",
	"ITEMSELLWND",
	"CHOOSESELLBUYWND",
	"COMBOBOXWND",
	"CHATROOMMAKEWND",
	"CHATROOMWND",
	"PASSWORDWND",
	"CHATROOMCHANGEWND",
	"EXCHANGEWND",
	"EXCHANGEACCEPTWND",
	"ITEMSTOREWND",
	"MESSENGERGROUPWND",
	"JOINPARTYACCEPTWND",
	"SHORTCUTWND",
	"SKILLLISTWND",
	"TIPOFTHEDAYWND",
	"CHOOSEWARPWND",
	"MERCHANTITEMWND",
	"MERCHANTSHOPMAKEWND",
	"MERCHANTMIRRORITEMWND",
	"MERCHANTITEMSHOPWND",
	"MERCHANTITEMPURCHASEWND",
	"MERCHANTITEMMYSHOPWND",
	"SKILLDESCRIBEWND",
	"CARDITEMILLUSTWND",
	"QUITWND",
	"NOTIFYJOBLEVELUPWND",
	"ITEMPARAMCHANGEDISPLAYWND",
	"CANDIDATEWND",
	"COMPOSITIONWND",
	"PARTYSETTINGWND",
	"EMAILADDRESSWND",
	"SKILLNAMECHANGEWND",
	"NPCEDITDIALOGWND",
	"DETAILLEVELWND",
	"NOTIFYITEMOBTAINWND",
	"GUILDWND",
	"GUILDINFOMANAGEWND",
	"GUILDMEMBERMANAGEWND",
	"GUILDPOSITIONMANAGEWND",
	"GUILDSKILLWND",
	"GUILDBANISHEDMEMBERWND",
	"GUILDNOTICEWND",
	"GUILDTOTALINFOWND",
	"WHISPERLISTWND",
	"FRIENDOPTIONWND",
	"MESSENGERWND",
	"JOINGUILDACCEPTWND",
	"WEBBROWSERWND",
	"ALLYGUILDACCEPTWND",
	"ITEMIDENTIFYWND",
	"ITEMCOMPOSITIONWND",
	"GUILDLEAVEREASONDESCWND",
	"GUILDBANREASONDESCWND",
	"MONSTERINFOWND",
	"ILLUSTWND",
	"MAKETARGETLISTWND",
	"MAKETARGETPROCESSWND",
	"MAKETARGETRESULTWND",
	"COMBINEDCARDITEMCOLLECTIONWND",
	"CALCULATORWND",
	"TALKBOXTRAPINPUTWND",
	"KEYSTROKEWND",
	"EMOTIONWND",
	"EMOTIONLISTWND",
	"PETINFOWND",
	"TOPRANKWND",
	"SELECTPETEGGWND",
	"PETTAMINGDECEIVEWND",
	"CNTWND",
	"NOTICEWND",
	"MAKINGARROWLISTWND",
	"SELECTCARTWND",
	"MINIGAMEWND",
	"SELECTMINIGAMEWND",
	"MINIGAMESCOREWND",
	"SPELLLISTWND",
	"NPCTEXTEDITDIALOGWND",
	"GRAFFISTRBOXWND",
	"SHOWNEEDITEMLISTWND",
	"PROHIBITLISTWND",
	"COUPLEACCEPTWND",
	"BABYACCEPTWND",
	"BOOKWND",
	"ITEMREPAIRWND",
	"SYSCHECKWND",
	"JOINFRIENDACCEPTWND",
	"BABYACCEPTWND2",
	"WEAPONREFINEWND",
	"COMMANDLISTWND",
	"HOMUNINFOWND",
	"HOMUNSKILLLISTWND",
	"STOREPASSWORDINPUTWND",
	"AUTOMESSAGEWND",
	"MAILLISTWND",
	"MAILVIEWWND",
	"STARPLACE_ACCEPTWND",
	"AUCTIONWND",
	"AUCTIONMSGWND",
	"IMGMENUWND",
	"GRON_MESSENGERWND",
	"GRON_OPTIONWND",
	"GRON_ALERTWND",
	"GRON_MESSAGEWND",
	"GRON_SETTINGWND",
	"GRON_MEMOWND",
	"GRON_BARWND",
	"PTNUMWND",
	"PTACINFOWND",
	"MERINFOWND",
	"MERSKILLLISTWND",
	"SELCHARFORUSERVERWND",
	"CHANGENAMEWND",
	"REBIRTHWND",
	"SUBCHATWND_ST",
	"SUBCHATWND_BT",
	"BATTLEMSG_OPTIONWND",
	"SUBCHAT_MINIWND",
	"QUESTWND",
	"QUEST_DETAIL_WND",
	"ROSHOPWND",
	"MEMORIAL_DUN_WND",
	"YOURITEMWND",
	"OTHERUSER_EQUIPWND",
	"ROMAPWND",
	"ADMINTOOLWND",
	"ADMINTOOL_MINIWND",
	"NOTIFYQUESTWND",
};

struct IdName {
	int id;
	const char *name;
};

// The control ids of UISys.h, lower case without "ID_": a button is named by
// what it does (ok, cancel, make, delete, buy, sell, ...).
const IdName kControlNames[] = {
	{ ID_NOTHING, "nothing" },
	{ ID_OK, "ok" },
	{ ID_CANCEL, "cancel" },
	{ ID_LOGIN, "login" },
	{ ID_YES, "yes" },
	{ ID_NO, "no" },
	{ ID_BUTTON1, "button1" },
	{ ID_BUTTON2, "button2" },
	{ ID_BUTTON3, "button3" },
	{ ID_STATUS, "status" },
	{ ID_OPTION, "option" },
	{ ID_ITEMS, "items" },
	{ ID_EQUIP, "equip" },
	{ ID_SKILL, "skill" },
	{ ID_DIALOG, "dialog" },
	{ ID_JOB, "job" },
	{ ID_FRIEND, "friend" },
	{ ID_BASE, "base" },
	{ ID_CLOSE, "close" },
	{ ID_MINI, "mini" },
	{ ID_MAKE, "make" },
	{ ID_SLOT, "slot" },
	{ ID_STR, "str" },
	{ ID_AGI, "agi" },
	{ ID_VIT, "vit" },
	{ ID_INT, "int" },
	{ ID_DEX, "dex" },
	{ ID_LUK, "luk" },
	{ ID_DELETE, "delete" },
	{ ID_HIS, "his" },
	{ ID_CHECK, "check" },
	{ ID_COMM, "comm" },
	{ ID_RADIOBTN, "radiobtn" },
	{ ID_PLUS, "plus" },
	{ ID_MINUS, "minus" },
	{ ID_NEXT, "next" },
	{ ID_MAP, "map" },
	{ ID_RESTART, "restart" },
	{ ID_EXIT, "exit" },
	{ ID_CLOSEALL, "closeall" },
	{ ID_SELL, "sell" },
	{ ID_BUY, "buy" },
	{ ID_WHISPERCOMBO, "whispercombo" },
	{ ID_RIGHT, "right" },
	{ ID_LEFT, "left" },
	{ ID_NUMCOMBO, "numcombo" },
	{ ID_CLASSCOMBO, "classcombo" },
	{ ID_EXCHANGE, "exchange" },
	{ ID_SENDMSG, "sendmsg" },
	{ ID_TALK, "talk" },
	{ ID_FRIEND_INFO, "friend_info" },
	{ ID_PARTY_SETTING, "party_setting" },
	{ ID_DELETE_FRIEND, "delete_friend" },
	{ ID_LEAVE, "leave" },
	{ ID_DELETE_FRIEND_NAME, "delete_friend_name" },
	{ ID_TALKTOWHOCOMBO, "talktowhocombo" },
	{ ID_LVLUP1, "lvlup1" },
	{ ID_LVLUP2, "lvlup2" },
	{ ID_LVLUP3, "lvlup3" },
	{ ID_LVLUP4, "lvlup4" },
	{ ID_LVLUP5, "lvlup5" },
	{ ID_LVLUP6, "lvlup6" },
	{ ID_LVLUP7, "lvlup7" },
	{ ID_PREV, "prev" },
	{ ID_MERCHANTITEMS, "merchantitems" },
	{ ID_EQUIP_OPTION_OFF, "equip_option_off" },
	{ ID_EQUIP_OPTION_OPEN, "equip_option_open" },
	{ ID_CARDITEM, "carditem" },
	{ ID_QUIT, "quit" },
	{ ID_TO_CHARACTER_SELECT, "to_character_select" },
	{ ID_LVLSELECTDN1, "lvlselectdn1" },
	{ ID_LVLSELECTDN2, "lvlselectdn2" },
	{ ID_LVLSELECTDN3, "lvlselectdn3" },
	{ ID_LVLSELECTDN4, "lvlselectdn4" },
	{ ID_LVLSELECTDN5, "lvlselectdn5" },
	{ ID_LVLSELECTDN6, "lvlselectdn6" },
	{ ID_LVLSELECTDN7, "lvlselectdn7" },
	{ ID_LVLSELECTUP1, "lvlselectup1" },
	{ ID_LVLSELECTUP2, "lvlselectup2" },
	{ ID_LVLSELECTUP3, "lvlselectup3" },
	{ ID_LVLSELECTUP4, "lvlselectup4" },
	{ ID_LVLSELECTUP5, "lvlselectup5" },
	{ ID_LVLSELECTUP6, "lvlselectup6" },
	{ ID_LVLSELECTUP7, "lvlselectup7" },
	{ ID_MAKEACCOUNT, "makeaccount" },
	{ ID_SKINSELECTCOMBO, "skinselectcombo" },
	{ ID_MODAL_PASSED, "modal_passed" },
	{ ID_DETAILLEVEL, "detaillevel" },
	{ ID_APPLY, "apply" },
	{ ID_ACCU_EXP, "accu_exp" },
	{ ID_OPEN_MEMBER_POS_COMBO, "open_member_pos_combo" },
	{ ID_ADD_GUILD_POSITION, "add_guild_position" },
	{ ID_POSNAME_EDIT, "posname_edit" },
	{ ID_PAYRATE_EDIT, "payrate_edit" },
	{ ID_SEND_GUILD_NOTICE, "send_guild_notice" },
	{ ID_CHANGE_GUILE_EMBLEM, "change_guile_emblem" },
	{ ID_UP, "up" },
	{ ID_DOWN, "down" },
	{ ID_NOTICE_TITLE_EDITED, "notice_title_edited" },
	{ ID_NOTICE_EDITED, "notice_edited" },
	{ ID_OPEN_EMOTIONLIST, "open_emotionlist" },
	{ ID_CHARGE, "charge" },
	{ ID_INTRO, "intro" },
	{ ID_NOTICE, "notice" },
	{ ID_BOOKITEM, "bookitem" },
	{ ID_AUTOBOOKITEM, "autobookitem" },
	{ ID_OPEN_HOMUNSKILLLIST, "open_homunskilllist" },
	{ ID_FEED_TO_HOMUN, "feed_to_homun" },
	{ ID_DELETE_HOMUN, "delete_homun" },
	{ ID_CHANGE_STORE_PASSWORD, "change_store_password" },
	{ ID_STORE_PASSWORD_NUM0, "store_password_num0" },
	{ ID_STORE_PASSWORD_NUM1, "store_password_num1" },
	{ ID_STORE_PASSWORD_NUM2, "store_password_num2" },
	{ ID_STORE_PASSWORD_NUM3, "store_password_num3" },
	{ ID_STORE_PASSWORD_NUM4, "store_password_num4" },
	{ ID_STORE_PASSWORD_NUM5, "store_password_num5" },
	{ ID_STORE_PASSWORD_NUM6, "store_password_num6" },
	{ ID_STORE_PASSWORD_NUM7, "store_password_num7" },
	{ ID_STORE_PASSWORD_NUM8, "store_password_num8" },
	{ ID_STORE_PASSWORD_NUM9, "store_password_num9" },
	{ ID_STORE_PASSWORD_NUM_RESET, "store_password_num_reset" },
	{ ID_STORE_RETURN, "store_return" },
	{ ID_SEND_MAIL, "send_mail" },
	{ ID_DELETE_MAIL, "delete_mail" },
	{ ID_CANCEL_MAIL, "cancel_mail" },
	{ ID_INPUT_ZENY, "input_zeny" },
	{ ID_SET_ZENY, "set_zeny" },
	{ ID_RETURN_MAIL, "return_mail" },
	{ ID_AUCTION_VIEW, "auction_view" },
	{ ID_AUCTION_ADD, "auction_add" },
	{ ID_AUCTION_SELL, "auction_sell" },
	{ ID_AUCTION_BUY, "auction_buy" },
	{ ID_ITEM_NAME, "item_name" },
	{ ID_PREV_PAGE, "prev_page" },
	{ ID_NEXT_PAGE, "next_page" },
	{ ID_ADD_AUCTION, "add_auction" },
	{ ID_FINISH_AUCTION, "finish_auction" },
	{ ID_BUY_AUCTION, "buy_auction" },
	{ ID_SEARCH, "search" },
	{ ID_FROM_NAME, "from_name" },
	{ ID_HEADER, "header" },
	{ ID_SET_STATUS, "set_status" },
	{ ID_VIEW_LETTER, "view_letter" },
	{ ID_ADD_FRIEND, "add_friend" },
	{ ID_ADD_GROUP, "add_group" },
	{ ID_SEND_MSG, "send_msg" },
	{ ID_STATUS_MENU, "status_menu" },
	{ ID_GROUP_MENU, "group_menu" },
	{ ID_FRIEND_MENU, "friend_menu" },
	{ ID_FRIEND_MENU1, "friend_menu1" },
	{ ID_GROUP, "group" },
	{ ID_INVITE, "invite" },
	{ ID_VIEW_CHAT_LIST, "view_chat_list" },
	{ ID_SELECT_ITEM, "select_item" },
	{ ID_INSERT_FRIEND, "insert_friend" },
	{ ID_SET_NICK, "set_nick" },
	{ ID_DELETE_SOCKET, "delete_socket" },
	{ ID_SELECT_WND, "select_wnd" },
	{ ID_SEARCH_FRIEND, "search_friend" },
	{ ID_SETTING, "setting" },
	{ ID_HELP, "help" },
	{ ID_LOGOUT, "logout" },
	{ ID_SEND, "send" },
	{ ID_REPLY, "reply" },
	{ ID_NAME, "name" },
	{ ID_TYPING_MSG, "typing_msg" },
	{ ID_OPEN_MERSKILLLIST, "open_merskilllist" },
	{ ID_FIRE_MER, "fire_mer" },
	{ ID_CHANGE_NAME, "change_name" },
	{ ID_CHANGE_NAME_SUCCESS, "change_name_success" },
	{ ID_REBIRTH, "rebirth" },
	{ ID_PREVIEW, "preview" },
	{ ID_NEXTVIEW, "nextview" },
	{ ID_BOOKMARK, "bookmark" },
	{ ID_STICK_ON, "stick_on" },
	{ ID_STICK_OFF, "stick_off" },
	{ ID_B_GRAVITY, "b_gravity" },
	{ ID_B_HANGAME, "b_hangame" },
	{ ID_QUEST, "quest" },
	{ ID_PREMIUMSLOT, "premiumslot" },
	{ ID_RESET, "reset" },
};

const char *ControlName(int id)
{
	for (size_t i = 0; i < sizeof(kControlNames) / sizeof(kControlNames[0]); ++i)
		if (kControlNames[i].id == id)
			return kControlNames[i].name;
	return nullptr;
}

struct Made {
	int wid;
	const std::type_info *type;
};

// What UIWindowMgr::MakeWindow made, by object. A record whose object has
// since been freed and reused by another class is told apart by its type.
std::map<UIWindow *, Made> g_made;

struct Control {
	UIWindow *wnd;
	std::string id;
	const char *kind;
	int x, y, w, h;
	std::string text;
	int focus;
	std::vector<std::string> items;   // a list's lines
};

struct Window {
	UIWindow *wnd;
	int wid;
	std::string name;
	int x, y, w, h;
	int focus;
	std::vector<Control> controls;
};

std::vector<Window> g_windows;
std::vector<bf_client_ui_window> g_out_windows;
std::vector<bf_client_ui_control> g_out_controls;
std::vector<const char *> g_out_items;

std::string ClassName(UIWindow *w)
{
	int status = 0;
	char *name = abi::__cxa_demangle(typeid(*w).name(), nullptr, nullptr, &status);
	std::string s = status == 0 && name ? name : typeid(*w).name();
	free(name);
	if (s.compare(0, 2, "UI") == 0)
		s.erase(0, 2);
	for (size_t i = 0; i < s.size(); ++i)
		s[i] = (char)toupper((unsigned char)s[i]);
	return s;
}

}  // namespace

// Reads what the window classes keep to themselves; a friend of each class
// it looks into (UISys.h, UIFrameWnd.h, under BF_PORT).
struct BfUiProbe {
	static void Rect(UIWindow *w, Control &c)
	{
		int x = 0, y = 0;
		w->GetGlobalCoor(x, y);
		c.x = x;
		c.y = y;
		c.w = w->m_w;
		c.h = w->m_h;
	}

	// Every control below w that can be clicked, typed into or read, with
	// a name from its control id or its kind and position in the walk.
	static void Collect(UIWindow *w, Window &out, int depth)
	{
		if (depth > 6)
			return;
		for (std::list<UIWindow *>::iterator it = w->m_children.begin(); it != w->m_children.end(); ++it) {
			UIWindow *c = *it;
			if (!c->m_show)
				continue;
			Control k;
			k.wnd = c;
			k.kind = nullptr;
			k.focus = c == g_windowMgr.m_editWindow;
			if (UIEditCtrl *e = dynamic_cast<UIEditCtrl *>(c)) {
				k.kind = "edit";
				k.text = e->m_maskchar ? std::string(e->m_text.size(), '*') : e->m_text;
			} else if (UIButton *b = dynamic_cast<UIButton *>(c)) {
				k.kind = "button";
				k.text = b->m_text;
			} else if (UICheckBox *x = dynamic_cast<UICheckBox *>(c)) {
				k.kind = "check";
				k.text = x->GetCheck() ? "on" : "off";
			} else if (dynamic_cast<UISlotBitmap *>(c)) {
				k.kind = "item";
			} else if (UIListBox *l = dynamic_cast<UIListBox *>(c)) {
				k.kind = "list";
				int sel = l->GetSelected();
				if (sel >= 0 && sel < l->GetNumItem())
					k.text = l->GetItem(sel);
				for (int i = 0; i < l->GetNumItem(); ++i)
					k.items.push_back(l->GetItem(i));
			} else if (UIStaticText *t = dynamic_cast<UIStaticText *>(c)) {
				k.kind = "text";
				k.text = t->m_fullText.empty() ? t->m_text : t->m_fullText;
			}
			if (k.kind) {
				const char *name = strcmp(k.kind, "button") == 0 ? ControlName(c->m_id) : nullptr;
				int n = 0;
				for (size_t i = 0; i < out.controls.size(); ++i)
					if (strcmp(out.controls[i].kind, k.kind) == 0)
						++n;
				char id[64];
				if (name)
					snprintf(id, sizeof id, "%s", name);
				else
					snprintf(id, sizeof id, "%s:%d", k.kind, n);
				for (size_t i = 0; i < out.controls.size(); ++i)
					if (out.controls[i].id == id) {   // a second button with the same id
						snprintf(id, sizeof id, "%s:%d", name ? name : k.kind, n);
						break;
					}
				k.id = id;
				Rect(c, k);
				out.controls.push_back(k);
			}
			if (!dynamic_cast<UIListBox *>(c))   // a list's scroll bars are part of it
				Collect(c, out, depth + 1);
		}
	}

	static Control *Find(Window &w, UIWindow *c)
	{
		for (size_t i = 0; i < w.controls.size(); ++i)
			if (w.controls[i].wnd == c)
				return &w.controls[i];
		return nullptr;
	}

	static void Rename(Window &w, UIWindow *c, const char *id)
	{
		if (Control *k = c ? Find(w, c) : nullptr)
			k->id = id;
	}

	static void RenameId(Window &w, const char *from, const char *to)
	{
		for (size_t i = 0; i < w.controls.size(); ++i)
			if (w.controls[i].id == from)
				w.controls[i].id = to;
	}


	// Drops the control whose window is c.
	static void Remove(Window &w, UIWindow *c)
	{
		for (size_t i = 0; i < w.controls.size(); ++i)
			if (w.controls[i].wnd == c) {
				w.controls.erase(w.controls.begin() + i);
				return;
			}
	}

	static void Add(Window &w, UIWindow *wnd, const char *id, const char *kind, int x, int y, int cw, int ch,
	                const std::string &text)
	{
		Control k;
		k.wnd = wnd;
		k.id = id;
		k.kind = kind;
		wnd->GetGlobalCoor(x, y);
		k.x = x;
		k.y = y;
		k.w = cw;
		k.h = ch;
		k.text = text;
		k.focus = 0;
		w.controls.push_back(k);
	}

	static std::string ListText(UIListBox *l)
	{
		std::string s;
		for (int i = 0; i < l->GetNumItem(); ++i) {
			if (i)
				s += "\n";
			s += l->GetItem(i);
		}
		return s;
	}

	// The entries a list box shows, as `<prefix><text>` at their rows (M8.1e:
	// CHOOSEWND's menu entries, `choice:<label>`).
	static void ListEntries(Window &w, UIListBox *l, const char *prefix)
	{
		int spacing = l->m_itemSpacing > 0 ? l->m_itemSpacing : 1;
		int first = l->m_vertViewOffset;
		int last = l->m_h / spacing - l->m_horzScrEnabled + first;
		if (last > l->GetNumItem())
			last = l->GetNumItem();
		for (int i = first; i < last; ++i) {
			std::string id = std::string(prefix) + l->GetItem(i);
			Add(w, l, id.c_str(), "item", 0, (i - first) * spacing,
			    l->m_w - (l->m_vertScrEnabled ? 13 : 0), spacing, l->GetItem(i));
		}
	}

	// The items a deal or storage window shows (shop list, purchase, sell,
	// storage), as `item:<item id>` at their cells (the shop's items have no
	// inventory index and the storage's index is the storage's own), the walk of
	// UIItemBaseWnd::GetItemInfo (UIItemWnd.cpp): cells of 32 from (40, 17),
	// m_viewOffset rows scrolled away, damaged pet eggs left out. Text: the
	// name as the client shows it and the count.
	static void ItemCells(Window &w, UIItemBaseWnd *b)
	{
		int horz = b->m_itemHorzNum > 0 ? b->m_itemHorzNum : 1;
		int drawCount = 0;
		int itemCount = 0;
		for (std::list<ITEM_INFO>::iterator it = b->m_itemList.begin(); it != b->m_itemList.end(); ++it, ++itemCount) {
			if (itemCount < b->m_viewOffset * horz)
				continue;
			if (g_session.IsPetEgg(atoi(it->m_itemName.c_str())) && it->m_isDamaged)
				continue;
			if (drawCount >= b->m_itemVertNum * horz)
				break;
			char name[256];
			name[0] = 0;
			ITEM_INFO info = *it;
			info.GetItemName(name);
			char text[300];
			snprintf(text, sizeof text, "%s %d", name, it->m_num);
			char id[32];
			snprintf(id, sizeof id, "item:%d", atoi(it->m_itemName.c_str()));
			for (size_t i = 0; i < w.controls.size(); ++i)
				if (w.controls[i].id == id) {   // the same item twice (unequal refines, cards)
					snprintf(id, sizeof id, "item:%d:%d", atoi(it->m_itemName.c_str()), drawCount);
					break;
				}
			Add(w, b, id, "item", (drawCount % horz) * 32 + 40, (drawCount / horz) * 32 + 17, 32, 32, text);
			++drawCount;
		}
	}

	// The NPC windows (M8.1e).
	static void Npc(Window &w)
	{
		if (UISayDialogWnd *d = dynamic_cast<UISayDialogWnd *>(w.wnd)) {
			// UM_DELETE_BUTTON moves a used button to (m_w, m_h) and keeps it
			// as a child; only m_buttonList holds the live ones. Next and Close
			// share ID_NEXT and differ in m_target.
			std::vector<UIWindow *> stale;
			for (size_t i = 0; i < w.controls.size(); ++i)
				if (dynamic_cast<UIButton *>(w.controls[i].wnd))
					stale.push_back(w.controls[i].wnd);
			for (size_t i = 0; i < stale.size(); ++i)
				Remove(w, stale[i]);
			for (std::list<UIBitmapButton *>::iterator it = d->m_buttonList.begin(); it != d->m_buttonList.end(); ++it) {
				Control k;
				k.wnd = *it;
				k.id = d->m_target == MM_CLOSE_DIALOG ? "close" : "next";
				k.kind = "button";
				k.focus = 0;
				Rect(*it, k);
				w.controls.push_back(k);
			}
			if (Control *k = Find(w, d->m_textViewer)) {
				k->id = "text";
				k->text = ListText(d->m_textViewer);
			}
		} else if (UIChooseWnd *c = dynamic_cast<UIChooseWnd *>(w.wnd)) {
			Rename(w, c->m_choostList, "list");
			ListEntries(w, c->m_choostList, "choice:");
		} else if (UINpcEditDialogWnd *e = dynamic_cast<UINpcEditDialogWnd *>(w.wnd)) {
			Rename(w, e->m_editCtrl, "edit");
		} else if (UIItemBaseWnd *b = dynamic_cast<UIItemBaseWnd *>(w.wnd)) {
			if (dynamic_cast<UIItemWnd *>(b))
				return;   // the item window: Items() (M8.1d), item:<inventory index>
			if (b->m_h == 17)
				return;   // minimised to its title bar
			ItemCells(w, b);
			// The storage's vertical tabs, named as the item window's
			// (`tab:item`, `tab:equip`, `tab:etc`, text "on" for the open
			// one), in UITab::OnLBtnDown's rows.
			for (std::list<UIWindow *>::iterator it = b->m_children.begin(); it != b->m_children.end(); ++it) {
				UITab *tab = dynamic_cast<UITab *>(*it);
				if (!tab || !tab->m_show || tab->m_numTab != 3)
					continue;
				static const char *const kTabs[3] = {"tab:item", "tab:equip", "tab:etc"};
				int rowH = tab->m_h / tab->m_numTab;
				for (int i = 0; i < 3; ++i)
					Add(w, tab, kTabs[i], "button", 0, i * rowH, tab->m_w, rowH, tab->GetState() == i ? "on" : "");
			}
		} else if (dynamic_cast<UIItemDropCntWnd *>(w.wnd)) {
			RenameId(w, "edit:0", "edit");
		}
	}

	// The names port/harness/README.md lists for the windows of the login.
	static void Specific(Window &w)
	{
		static const char *const kStats[12] = {"name", "job", "level", "exp", "hp", "sp",
		                                       "str", "agi", "vit", "int", "dex", "luk"};
		if (UILoginWnd *l = dynamic_cast<UILoginWnd *>(w.wnd)) {
			Rename(w, l->m_login, "id_edit");
			Rename(w, l->m_password, "pw_edit");
			RenameId(w, "login", "ok");
			RenameId(w, "check:0", "save");
		} else if (UISelectCharWnd *s = dynamic_cast<UISelectCharWnd *>(w.wnd)) {
			for (int i = 0; i < 3; ++i) {
				int slot = s->m_curPage * 3 + i;
				Control *k = s->m_slots[i] ? Find(w, s->m_slots[i]) : nullptr;
				if (!k)
					continue;
				char id[16];
				snprintf(id, sizeof id, "slot:%d", slot);
				k->id = id;
				k->focus = i == s->m_curSlot;
				CMode *mode = g_modeMgr.GetCurMode();
				CHARACTER_INFO *info = mode ? (CHARACTER_INFO *)mode->SendMsg(MM_QUERYCHARICTORINFO, slot, 0, 0) : nullptr;
				if (info && !s->m_isEmpty[slot]) {
					char name[25];
					memcpy(name, info->name, 24);
					name[24] = 0;
					k->text = name;
				} else {
					k->text = "";
				}
			}
			for (int t = 0; t < 12; ++t)
				Rename(w, s->m_text[t], kStats[t]);
		} else if (UIMakeCharWnd *m = dynamic_cast<UIMakeCharWnd *>(w.wnd)) {
			Rename(w, m->m_nameEditCtrl, "name_edit");
			for (int t = 0; t < 6; ++t)
				Rename(w, m->m_text[t], kStats[6 + t]);
		} else if (UINewChatWnd *c = dynamic_cast<UINewChatWnd *>(w.wnd)) {
			// The chat line and the whisper name field left of it (M8.1b).
			Rename(w, c->m_commonChat, "chat_edit");
			Rename(w, c->m_whisperChat, "whisper_edit");
		} else if (UIMenuWnd *m = dynamic_cast<UIMenuWnd *>(w.wnd)) {
			// The entries are drawn, not child windows (UIMenuWnd::OnDraw):
			// each UM_ADDITEM adds its text twice, one line of half the
			// spacing each, a boundary one; "menu:<n>" is the n-th entry,
			// the index MM_MENUSELECTED receives.
			int half = m->m_menuItemSpacing / 2, n = 0;
			for (size_t i = 0; i < m->m_menuItems.size(); ++i) {
				if (m->IsBoundary((int)i))
					continue;
				int seq = (int)i - m->GetNumBoundary((int)i);
				if (seq % 2)   // the second line of an entry
					continue;
				Control k;
				k.wnd = nullptr;
				k.kind = "button";
				char id[32];
				snprintf(id, sizeof id, "menu:%d", n++);
				k.id = id;
				k.text = m->m_menuItems[i];
				k.focus = 0;
				int x = 0, y = 0;
				m->GetGlobalCoor(x, y);
				k.x = x + 3;
				k.y = y + half * (int)i + 1;
				k.w = m->m_w - 6;
				k.h = m->m_menuItemSpacing - 2;
				w.controls.push_back(k);
			}
		}
	}

	static void AddControl(Window &w, UIWindow *wnd, const char *id, const char *kind, int x, int y,
	                       int cw, int ch, const std::string &text)
	{
		Control k;
		k.wnd = nullptr;
		k.id = id;
		k.kind = kind;
		int gx = x, gy = y;
		wnd->GetGlobalCoor(gx, gy);
		k.x = gx;
		k.y = gy;
		k.w = cw;
		k.h = ch;
		k.text = text;
		k.focus = 0;
		w.controls.push_back(k);
	}

	static std::string ItemName(ITEM_INFO &item)
	{
		char name[128];
		name[0] = 0;
		item.GetItemName(name);
		return name;
	}

	// What the item, equipment and status windows draw themselves, as
	// controls (M8.1d): the item window's tabs (`tab:item`, `tab:equip`,
	// `tab:etc`, text "on" for the open one) and the items it shows
	// (`item:<index>`, the server's inventory index, as UIItemWnd::OnDraw
	// lays them out); the equipment window's worn items (`item:<index>`, at
	// the slot UIEquipWnd::GetItemInfo hit-tests); the status window's
	// values (`stat:str` ... with the bonus as "5 + 1", `need:str` ..., the
	// points a raise costs, and `atk`, `matk`, `hit`, `critical`, `def`,
	// `mdef`, `flee`, `aspd`, `status_point`, formatted as
	// UIStatusWnd::OnDraw prints them).
	static void Items(Window &w)
	{
		if (UIItemWnd *iw = dynamic_cast<UIItemWnd *>(w.wnd)) {
			if (iw->m_h == 17)
				return;
			for (std::list<UIWindow *>::iterator it = iw->m_children.begin(); it != iw->m_children.end(); ++it) {
				UITab *tab = dynamic_cast<UITab *>(*it);
				if (!tab || !tab->m_show || tab->m_numTab != 3)
					continue;
				static const char *const kTabs[3] = {"tab:item", "tab:equip", "tab:etc"};
				int th = tab->m_h / tab->m_numTab;
				for (int i = 0; i < 3; ++i)
					AddControl(w, tab, kTabs[i], "button", 0, i * th, tab->m_w, th,
					           tab->GetState() == i ? "on" : "");
			}
			int drawCount = 0, itemCount = 0;
			for (std::list<ITEM_INFO>::iterator it = iw->m_itemList.begin(); it != iw->m_itemList.end(); ++it, ++itemCount) {
				if (itemCount < iw->m_viewOffset * iw->m_itemHorzNum)
					continue;
				if (g_session.IsPetEgg(atoi(it->m_itemName.c_str())) && it->m_isDamaged)
					continue;
				if (iw->m_itemHorzNum <= 0 || drawCount >= iw->m_itemVertNum * iw->m_itemHorzNum)
					break;
				char id[32];
				snprintf(id, sizeof id, "item:%u", it->m_itemIndex);
				AddControl(w, iw, id, "item", (drawCount % iw->m_itemHorzNum) * 32 + 40,
				           (drawCount / iw->m_itemHorzNum) * 32 + 17, 32, 32, ItemName(*it));
				++drawCount;
			}
		} else if (UIEquipWnd *ew = dynamic_cast<UIEquipWnd *>(w.wnd)) {
			if (ew->m_h == 17 || ew->m_typeWnd != 0)
				return;
			// slot -> the cell UIEquipWnd::GetItemInfo maps to it
			static const int kSlotX[10] = {6, 6, 6, 6, 251, 251, 251, 251, 6, 251};
			static const int kSlotY[10] = {43, 69, 95, 121, 43, 69, 95, 121, 17, 17};
			std::vector<unsigned int> seen;
			for (int slot = 0; slot < 10; ++slot) {
				ITEM_INFO item = g_session.GetEquipedItems(slot);
				if (!item.m_itemIndex || item.m_num <= 0)
					continue;
				bool dup = false;
				for (size_t i = 0; i < seen.size(); ++i)
					dup = dup || seen[i] == item.m_itemIndex;
				if (dup)
					continue;   // a two-slot item answers at its first slot
				seen.push_back(item.m_itemIndex);
				char id[32];
				snprintf(id, sizeof id, "item:%u", item.m_itemIndex);
				AddControl(w, ew, id, "item", kSlotX[slot], kSlotY[slot], 24, 24, ItemName(item));
			}
		} else if (UIStatusWnd *sw = dynamic_cast<UIStatusWnd *>(w.wnd)) {
			if (sw->m_h == 17 || sw->m_state)
				return;
			static const char *const kStat[6] = {"str", "agi", "vit", "int", "dex", "luk"};
			const int base[6] = {g_session.GetStr(), g_session.GetAgi(), g_session.GetVit(),
			                     g_session.GetInt(), g_session.GetDex(), g_session.GetLuk()};
			const int plus[6] = {g_session.GetPlusStr(), g_session.GetPlusAgi(), g_session.GetPlusVit(),
			                     g_session.GetPlusInt(), g_session.GetPlusDex(), g_session.GetPlusLuk()};
			const int *need[6] = {g_session.GetStandardStrPointer(), g_session.GetStandardAgiPointer(),
			                      g_session.GetStandardVitPointer(), g_session.GetStandardIntPointer(),
			                      g_session.GetStandardDexPointer(), g_session.GetStandardLukPointer()};
			static const int kPosY[6] = {6, 22, 38, 54, 70, 86};
			char text[64], id[32];
			for (int i = 0; i < 6; ++i) {
				if (plus[i])
					snprintf(text, sizeof text, "%d + %d", base[i], plus[i]);
				else
					snprintf(text, sizeof text, "%d", base[i]);
				snprintf(id, sizeof id, "stat:%s", kStat[i]);
				AddControl(w, sw, id, "text", 54, kPosY[i] + 16, 34, 14, text);
				snprintf(text, sizeof text, "%d", *need[i]);
				snprintf(id, sizeof id, "need:%s", kStat[i]);
				AddControl(w, sw, id, "text", 100, kPosY[i] + 16, 14, 14, text);
			}
			struct Value { const char *id; int x, y; };
			static const Value kValues[9] = {
				{"atk", 192, 21}, {"matk", 192, 37}, {"hit", 192, 53}, {"critical", 192, 69},
				{"def", 273, 21}, {"mdef", 273, 37}, {"flee", 273, 53}, {"aspd", 273, 69},
				{"status_point", 273, 85},
			};
			for (int i = 0; i < 9; ++i) {
				switch (i) {
				case 0: snprintf(text, sizeof text, "%d + %d", g_session.GetAttPower(), g_session.GetRefiningPower()); break;
				case 1: snprintf(text, sizeof text, "%d ~ %d", g_session.GetMin_mattPower(), g_session.GetMax_mattPower()); break;
				case 2: snprintf(text, sizeof text, "%d", g_session.GetHitSuccessValue()); break;
				case 3: snprintf(text, sizeof text, "%d", g_session.GetCriticalSuccessValue()); break;
				case 4: snprintf(text, sizeof text, "%d + %d", g_session.GetItemDefPower(), g_session.GetPlusdefPower()); break;
				case 5: snprintf(text, sizeof text, "%d + %d", g_session.GetMdefPower(), g_session.GetPlusmdefPower()); break;
				case 6: snprintf(text, sizeof text, "%d + %d", g_session.GetAvoidSuccessValue(), g_session.GetPlusAvoidSuccessValue()); break;
				case 7: snprintf(text, sizeof text, "%d", (2000 - g_session.GetASPD()) / 10); break;
				default: snprintf(text, sizeof text, "%d", g_session.GetPoint()); break;
				}
				// right-aligned at x, as TextOutWithRightAlign draws them
				AddControl(w, sw, kValues[i].id, "text", kValues[i].x - 60, kValues[i].y, 60, 14, text);
			}
		}
	}

	static void Describe()
	{
		g_windows.clear();
		UIWindowMgr &mgr = g_windowMgr;
		for (std::list<UIWindow *>::reverse_iterator it = mgr.m_children.rbegin(); it != mgr.m_children.rend(); ++it) {
			UIWindow *wnd = *it;
			if (!wnd->m_show)
				continue;
			bool quitting = false;
			for (std::list<UIWindow *>::iterator q = mgr.m_quitWindow.begin(); q != mgr.m_quitWindow.end(); ++q)
				if (*q == wnd)
					quitting = true;
			if (quitting)
				continue;
			Window w;
			w.wnd = wnd;
			w.wid = -1;
			std::map<UIWindow *, Made>::iterator made = g_made.find(wnd);
			if (made != g_made.end() && *made->second.type == typeid(*wnd))
				w.wid = made->second.wid;
			if (w.wid >= 0 && w.wid < (int)(sizeof(kWindowNames) / sizeof(kWindowNames[0])))
				w.name = kWindowNames[w.wid];
			else
				w.name = ClassName(wnd);
			w.x = wnd->m_x;
			w.y = wnd->m_y;
			w.w = wnd->m_w;
			w.h = wnd->m_h;
			w.focus = 0;
			for (UIWindow *e = mgr.m_editWindow; e; e = e->m_parent)
				if (e == wnd)
					w.focus = 1;
			Collect(wnd, w, 0);
			Specific(w);
			Items(w);
			Npc(w);
			g_windows.push_back(w);
		}
	}
};

extern "C" void bf_client_window_made(UIWindow *wnd, int wid)
{
	Made m = {wid, &typeid(*wnd)};
	g_made[wnd] = m;
}

extern "C" int bf_client_ui_windows(const bf_client_ui_window **windows, const bf_client_ui_control **controls)
{
	if (bf_port_headless()) {
		*windows = nullptr;
		*controls = nullptr;
		return 0;
	}
	BfUiProbe::Describe();
	g_out_windows.clear();
	g_out_controls.clear();
	g_out_items.clear();
	for (size_t i = 0; i < g_windows.size(); ++i)
		for (size_t j = 0; j < g_windows[i].controls.size(); ++j)
			for (size_t k = 0; k < g_windows[i].controls[j].items.size(); ++k)
				g_out_items.push_back(g_windows[i].controls[j].items[k].c_str());
	size_t item = 0;
	for (size_t i = 0; i < g_windows.size(); ++i) {
		const Window &w = g_windows[i];
		bf_client_ui_window o = {w.wnd, w.wid, w.name.c_str(), w.x, w.y, w.w, w.h, w.focus,
		                         (int)g_out_controls.size(), (int)w.controls.size()};
		g_out_windows.push_back(o);
		for (size_t j = 0; j < w.controls.size(); ++j) {
			const Control &c = w.controls[j];
			bf_client_ui_control k = {c.id.c_str(), c.kind, c.x, c.y, c.w, c.h, c.text.c_str(), c.focus,
			                          c.items.empty() ? nullptr : &g_out_items[item], (int)c.items.size()};
			item += c.items.size();
			g_out_controls.push_back(k);
		}
	}
	*windows = g_out_windows.empty() ? nullptr : &g_out_windows[0];
	*controls = g_out_controls.empty() ? nullptr : &g_out_controls[0];
	return (int)g_out_windows.size();
}
