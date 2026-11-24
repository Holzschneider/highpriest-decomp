// The game mode as the control socket reports it (PROTOCOL.md section 3,
// parts `map`, `self`, `actors`, `pointer.cell`; section 5, `chat`), read
// from the client's objects (bf_client_game.h). Compiled with the client's
// flags and headers (BF_CLIENT_PORT_SOURCES in CMakeLists.txt), like
// bf_client_state.cpp. Read-only: nothing here changes the client or sends a
// packet (an actor's name is read from the names the client already has; it
// asks the server for one only when the pointer rests on the actor).
#include "common.h"
#include "packet.h"
#include "Mode.h"
#include "GameMode.h"
#include "GameActor.h"
#include "GameWorld.h"
#include "View.h"
#include "Session.h"
#include "3dlib/Renderer.h"
#include "device/Control.h"
#include "resource/Attr.h"
#include "base/Util.h"

#include <string.h>
#include <string>
#include <vector>

#include "bf_client_game.h"

extern int g_bfModeRunning;   // Mode.cpp: a mode is in OnRun

// Friend of CGameMode (GameMode.h): the members the protocol needs that the
// class keeps private.
struct BfClientGame {
	static const char *RswName(CGameMode *mode) { return mode->m_rswName; }
	static const char *ActorName(CGameMode *mode, unsigned int aid)
	{
		std::map<unsigned int, NamePair>::const_iterator it = mode->m_actorNameList.find(aid);
		// ZC_ACK_REQNAME assigns the packet's 24 bytes, NULs included.
		if (it == mode->m_actorNameList.end() || !it->second.cName.c_str()[0])
			return NULL;
		return it->second.cName.c_str();
	}
	// Friend of CGameActor too (GameActor.h): the action of emotion.act its
	// newest emote balloon shows, -1 when none. An emote is a CMsgEffect of
	// type MET_EMOTION in m_msgEffectList (AM_EMOTION, CGameMode::MakeEmotion)
	// until AM_MSGEFFECT_DEAD takes it out.
	static int Emotion(CGameActor *a)
	{
		int action = -1;
		for (std::list<CMsgEffect *>::iterator m = a->m_msgEffectList.begin(); m != a->m_msgEffectList.end(); ++m)
			if (*m && (*m)->GetMsgEffectType() == MET_EMOTION)
				action = (*m)->m_curAction;
		return action;
	}
};

namespace {

CGameMode *ReadyMode()
{
	CGameMode *mode = g_modeMgr.GetGameMode();
	if (!mode || !g_bfModeRunning || g_sysQuit)
		return NULL;
	CWorld *world = mode->GetWorld();
	if (!world || !world->m_attr || !world->m_player || !mode->GetView())
		return NULL;
	return mode;
}

void Cell(CGameMode *mode, const vector3d &pos, int *x, int *y)
{
	mode->GetServerCoor(pos.x, pos.z, *x, *y);
}

// The client's own classification where it has one (CSession::IsPc, the
// pet's head type, the homunculus and mercenary ids it keeps), else the job
// ranges of packet.h.
const char *ActorType(CGameActor *a)
{
	int job = a->GetJob();
	unsigned int gid = a->GetGID();
	if (gid && gid == g_session.m_homunGID)
		return "homun";
	if ((gid && gid == g_session.m_merGID) || a->m_objectType == NPC_MERSOL_TYPE)
		return "merc";
	if (g_session.IsPc(job))
		return "pc";
	if (job == JT_WARPNPC || job == JT_HIDDEN_WARP_NPC)
		return "portal";
	if (job > JT_MON_BEGIN && job < JT_2004_JOB_BEGIN)
		return a->GetHeadType() == HEADTYPE_LAST || (gid && gid == g_session.m_petGID) ? "pet" : "monster";
	if (job >= JT_EVENT_MON_BEGIN && job <= JT_EVENT_MON_END)
		return "monster";
	if (job > JT_MER_BEGIN && job < JT_MER_ARCHER01)
		return "homun";
	if (job >= JT_MER_ARCHER01 && job < JT_MER_ARCHER01 + 30)
		return "merc";
	if (job > JT_WARPNPC && job < JT_MON_BEGIN)
		return "npc";
	if ((job >= 10000 && job < 20000) || (job >= JT_EVENT_NPC_BEGIN && job <= JT_EVENT_NPC_END))
		return "npc";
	return "other";
}

std::vector<bf_client_game_actor> g_actors;
std::vector<std::string> g_names;   // the actors' names, kept for g_actors
std::string g_selfName;

void Screen(CGameMode *mode, const vector3d &pos, bf_client_game_actor *out)
{
	out->on_screen = 0;
	if (!g_renderer)
		return;
	matrix vtm = mode->GetView()->GetViewMatrix();
	float x, y, oow;
	float w = pos.x * vtm.v13 + pos.y * vtm.v23 + pos.z * vtm.v33 + vtm.v43;
	if (w <= 0.0f)
		return;   // behind the camera
	g_renderer->ProjectVertex(pos, vtm, &x, &y, &oow);
	if (x < 0 || y < 0 || x >= g_renderer->m_width || y >= g_renderer->m_height)
		return;
	out->on_screen = 1;
	out->screen_x = (int)x;
	out->screen_y = (int)y;
}

// ---- chat lines

struct ChatLine {
	std::string channel, from, text;
	bool hasFrom;
	std::string window;   // partner of the whisper window, empty for the chat window
};
std::vector<ChatLine> g_chatPending;   // since the last take
std::vector<ChatLine> g_chatTaken;
std::vector<bf_client_game_chat> g_chatOut;

}  // namespace

extern "C" int bf_client_game_ready(void)
{
	return ReadyMode() != NULL;
}

extern "C" int bf_client_game_map_get(bf_client_game_map *out)
{
	CGameMode *mode = ReadyMode();
	if (!mode)
		return 0;
	const char *rsw = BfClientGame::RswName(mode);
	size_t n = strlen(rsw);
	const char *dot = strrchr(rsw, '.');
	if (dot)
		n = (size_t)(dot - rsw);
	if (n >= sizeof(out->name))
		n = sizeof(out->name) - 1;
	memcpy(out->name, rsw, n);
	out->name[n] = 0;
	C3dAttr *attr = mode->GetWorld()->m_attr;
	out->width = attr->m_width;
	out->height = attr->m_height;
	return 1;
}

extern "C" int bf_client_game_self_get(bf_client_game_self *out)
{
	CGameMode *mode = ReadyMode();
	if (!mode)
		return 0;
	CPlayer *p = mode->GetWorld()->m_player;
	out->id = g_session.GetAid();
	out->char_id = g_session.GetGid();
	g_selfName = g_session.GetCharName();
	out->name = g_selfName.c_str();
	out->job = g_session.GetJob();
	out->base_level = g_session.GetLevel();
	out->job_level = g_session.GetJoblevel();
	Cell(mode, p->m_pos, &out->x, &out->y);
	out->dir = DegToDir(p->m_roty);
	out->moving = p->m_stateId == ST_MOVE;
	out->dest_x = out->moving ? p->m_moveDestX : -1;
	out->dest_y = out->moving ? p->m_moveDestY : -1;
	out->hp = g_session.GetHp();
	out->max_hp = g_session.GetMaxhp();
	out->sp = g_session.GetSp();
	out->max_sp = g_session.GetMaxsp();
	out->zeny = g_session.GetGold();
	out->weight = g_session.GetWeight() / 10;
	out->max_weight = g_session.GetMaxWeight() / 10;
	return 1;
}

extern "C" int bf_client_game_actors(const bf_client_game_actor **actors)
{
	g_actors.clear();
	g_names.clear();
	*actors = NULL;
	CGameMode *mode = ReadyMode();
	if (!mode)
		return 0;
	CWorld *world = mode->GetWorld();
	struct Entry {
		bf_client_game_actor a;
		const char *name;
		bool cutSharp;   // shown without the part from '#' on, as the name balloon does
	};
	std::vector<Entry> entries;
	std::vector<const void *> seen;
	for (std::list<CGameActor *>::iterator it = world->m_actorList.begin(); it != world->m_actorList.end(); ++it) {
		CGameActor *a = *it;
		if (!a || a == world->m_player)
			continue;
		Entry e = {};
		e.a.id = a->GetGID();
		e.a.type = ActorType(a);
		e.a.has_job = 1;
		e.a.job = a->GetJob();
		Cell(mode, a->m_pos, &e.a.x, &e.a.y);
		Screen(mode, a->m_pos, &e.a);
		e.name = BfClientGame::ActorName(mode, a->GetGID());
		e.a.emotion = BfClientGame::Emotion(a);
		// CGameMode's name balloons cut the name at '#' for all but players
		// (UINameBalloonText::SetNameText, cutSharp = !IsThisPC).
		e.cutSharp = a->IsThisPC(a->GetJob()) == FALSE;
		entries.push_back(e);
		seen.push_back(a);
	}
	for (std::list<CItem *>::iterator it = world->m_itemList.begin(); it != world->m_itemList.end(); ++it) {
		CItem *i = *it;
		if (!i)
			continue;
		Entry e = {};
		e.a.id = i->GetAID();
		e.a.type = "item";
		e.a.emotion = -1;
		e.a.has_job = i->m_bfItemId >= 0;
		e.a.job = i->m_bfItemId;
		Cell(mode, i->m_pos, &e.a.x, &e.a.y);
		Screen(mode, i->m_pos, &e.a);
		e.name = i->m_itemName.empty() ? NULL : i->m_itemName.c_str();
		entries.push_back(e);
	}
	for (std::list<CSkill *>::iterator it = world->m_skillList.begin(); it != world->m_skillList.end(); ++it) {
		CSkill *s = *it;
		bool dup = false;
		for (size_t k = 0; k < seen.size(); ++k)
			dup = dup || seen[k] == s;
		if (!s || dup)
			continue;
		Entry e = {};
		e.a.id = s->GetAID();
		e.a.type = "skill";
		e.a.emotion = -1;
		e.a.has_job = 1;
		e.a.job = s->GetJob();
		Cell(mode, s->m_pos, &e.a.x, &e.a.y);
		Screen(mode, s->m_pos, &e.a);
		entries.push_back(e);
	}
	// The names are copied, so that they outlive a change of the client's lists.
	g_names.reserve(entries.size());
	for (size_t k = 0; k < entries.size(); ++k) {
		g_names.push_back(entries[k].name ? entries[k].name : "");
		std::string::size_type sharp = g_names.back().find('#');
		if (entries[k].cutSharp && sharp != std::string::npos)
			g_names.back().resize(sharp);
	}
	for (size_t k = 0; k < entries.size(); ++k) {
		entries[k].a.name = entries[k].name ? g_names[k].c_str() : NULL;
		g_actors.push_back(entries[k].a);
	}
	*actors = g_actors.empty() ? NULL : &g_actors[0];
	return (int)g_actors.size();
}

extern "C" int bf_client_game_pointer_cell(int *x, int *y)
{
	CGameMode *mode = ReadyMode();
	if (!mode || !g_renderer)
		return 0;
	int dx = -1, dy = -1;
	if (!mode->GetGroundAttrPos(dx, dy))
		return 0;
	C3dAttr *attr = mode->GetWorld()->m_attr;
	if (dx < 0 || dy < 0 || dx >= attr->m_width || dy >= attr->m_height)
		return 0;
	*x = dx;
	*y = dy;
	return 1;
}

// UIWindowMgr::SendMsg's chat types (val3): 1 a line someone said around
// ("name : text"), 2 a whisper (talker in val4), 3 party, 4 guild, 13 a
// siege broadcast; 0 and the rest are the client's own messages. A broadcast
// is the one line pushed twice, into the history (UIM_PUSHINTOCHATHISTORY)
// and into the broadcast list (UIM_PUSHINTOCHATHISTORY2). Types 8 and 9 are
// not shown in the chat window (UINewChatWnd's allShow) and are left out.
extern "C" void bf_client_chat_line(int second, const char *text, int type, const char *talker)
{
	if (!text || strcmp(text, "No Msg") == 0 || strcmp(text, "NO MSG") == 0)
		return;   // what UIWindowMgr::SendMsg does not show either
	if (second) {
		if (!g_chatPending.empty() && g_chatPending.back().text.size() <= strlen(text) &&
		    g_chatPending.back().channel == "system" && !g_chatPending.back().hasFrom) {
			ChatLine &last = g_chatPending.back();
			if (strstr(text, last.text.c_str()))
				last.channel = "broadcast";
		}
		return;
	}
	if (type == 8 || type == 9)
		return;
	ChatLine line;
	line.hasFrom = false;
	line.text = text;
	switch (type) {
	case 1: line.channel = "normal"; break;
	case 2: line.channel = "whisper"; break;
	case 3: line.channel = "party"; break;
	case 4: line.channel = "guild"; break;
	case 13: line.channel = "broadcast"; break;
	default: line.channel = "system"; break;
	}
	if (type >= 1 && type <= 4) {
		std::string::size_type sep = line.text.find(" : ");
		if (sep != std::string::npos && sep > 0) {
			line.from = line.text.substr(0, sep);
			line.text = line.text.substr(sep + 3);
			line.hasFrom = true;
		}
	}
	if (type == 2) {
		line.hasFrom = talker && *talker;
		line.from = line.hasFrom ? talker : "";
	}
	if (g_chatPending.size() >= 256)   // nobody takes them (no control socket)
		g_chatPending.erase(g_chatPending.begin());
	g_chatPending.push_back(line);
}

extern "C" void bf_client_whisper_window_line(const char *partner, const char *text)
{
	if (!text || !partner)
		return;
	ChatLine line;
	line.channel = "whisper";
	line.window = partner;
	line.text = text;
	line.hasFrom = false;
	std::string::size_type sep = line.text.find(" : ");
	if (sep != std::string::npos && sep > 0) {
		line.from = line.text.substr(0, sep);
		line.text = line.text.substr(sep + 3);
		line.hasFrom = true;
	}
	if (g_chatPending.size() >= 256)
		g_chatPending.erase(g_chatPending.begin());
	g_chatPending.push_back(line);
}

extern "C" int bf_client_game_take_chat(const bf_client_game_chat **lines)
{
	g_chatTaken.swap(g_chatPending);
	g_chatPending.clear();
	g_chatOut.clear();
	for (size_t k = 0; k < g_chatTaken.size(); ++k) {
		bf_client_game_chat c;
		c.channel = g_chatTaken[k].channel.c_str();
		c.from = g_chatTaken[k].hasFrom ? g_chatTaken[k].from.c_str() : NULL;
		c.text = g_chatTaken[k].text.c_str();
		c.window = g_chatTaken[k].window.empty() ? NULL : g_chatTaken[k].window.c_str();
		g_chatOut.push_back(c);
	}
	*lines = g_chatOut.empty() ? NULL : &g_chatOut[0];
	return (int)g_chatOut.size();
}

// ---- damage numbers

namespace {
std::vector<bf_client_game_damage> g_damagePending;
std::vector<bf_client_game_damage> g_damageTaken;
}

// MSGEFFECTTYPE (GameMode.h) of the numbers CGameActor::Am_Make_Number draws;
// a recovered amount (healing) is MET_NUMBER + 100 (CGameActor::SendMsg,
// AM_ATTACKED).
extern "C" void bf_client_damage_number(unsigned int gid, int number, int type)
{
	bf_client_game_damage d;
	d.target = gid;
	d.number = number;
	switch (type) {
	case MET_NUMBER: d.kind = "normal"; break;
	case MET_NUMBER_CRITICAL: d.kind = "critical"; break;
	case MET_NUMBER + 100: d.kind = "heal"; break;
	case MET_NUMBER_HALF: d.kind = "half"; break;
	case MET_NUMBER_TOTAL: d.kind = "total"; break;
	default: d.kind = "other"; break;
	}
	if (g_damagePending.size() >= 256)   // nobody takes them (no control socket)
		g_damagePending.erase(g_damagePending.begin());
	g_damagePending.push_back(d);
}

extern "C" int bf_client_game_take_damage(const bf_client_game_damage **numbers)
{
	g_damageTaken.swap(g_damagePending);
	g_damagePending.clear();
	*numbers = g_damageTaken.empty() ? NULL : &g_damageTaken[0];
	return (int)g_damageTaken.size();
}
