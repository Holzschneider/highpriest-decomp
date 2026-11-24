// The game mode as the control socket reports it (PROTOCOL.md section 3,
// parts `map`, `self`, `actors`, `pointer.cell`; section 5, events `map` and
// `chat`), between bf_client_game.cpp, which reads the client's objects with
// the client's headers, and bf_control_game.cpp, which writes the JSON.
// Plain C: no client or Windows type crosses. Strings are the client's bytes
// (Windows-1252); the writer decodes them.
#ifndef BF_CLIENT_GAME_H
#define BF_CLIENT_GAME_H

#ifdef __cplusplus
extern "C" {
#endif

// Non-zero while game mode runs on a loaded map (CGameMode in OnRun, with
// its world, ground attributes and player): `map` and `self` are reported
// only then, `null` otherwise.
int bf_client_game_ready(void);

typedef struct bf_client_game_map {
	char name[40];      // the map's name without extension ("prontera")
	int width, height;  // cells, from the ground's attributes (the .gat)
} bf_client_game_map;

// 1 and *out when ready, else 0.
int bf_client_game_map_get(bf_client_game_map *out);

typedef struct bf_client_game_self {
	unsigned int id;        // account id
	unsigned int char_id;
	const char *name;       // valid until the next call into this module
	int job, base_level, job_level;
	int x, y;               // cell of the client's own (animated) position
	int dir;                // 0-7, the server's convention (0 north, counter-clockwise)
	int moving;             // the client's walk is under way (state ST_MOVE)
	int dest_x, dest_y;     // the walk's destination cell; -1 when not moving
	int hp, max_hp, sp, max_sp;
	int zeny;
	int weight, max_weight; // as the client shows them (the server's tenths / 10)
} bf_client_game_self;

// 1 and *out when ready, else 0.
int bf_client_game_self_get(bf_client_game_self *out);

typedef struct bf_client_game_actor {
	unsigned int id;        // the server's id (GID for actors, ITAID for items, AID for skill units)
	const char *type;       // pc, npc, monster, item, portal, skill, pet, homun, merc, other
	const char *name;       // as displayed; NULL when the client does not know it
	int has_job, job;       // sprite or class id; item id for items; has_job 0 when unknown
	int x, y;               // cell
	int on_screen;          // the feet project into the view
	int screen_x, screen_y; // logical pixels, when on_screen
} bf_client_game_actor;

// The actors the client holds, other than its own character: actors, then
// ground items, then skill units. Returns their number (0 when not ready);
// *actors stays valid until the next call.
int bf_client_game_actors(const bf_client_game_actor **actors);

// 1 and the map cell under the client's pointer as its own ground picking
// finds it (CGameMode::GetGroundAttrPos), else 0 (not ready, or no cell).
int bf_client_game_pointer_cell(int *x, int *y);

typedef struct bf_client_game_chat {
	const char *channel;    // normal, party, guild, whisper, broadcast, system
	const char *from;       // sender's name, NULL when the line has none
	const char *text;       // the line without the "name : " prefix
} bf_client_game_chat;

// The lines added to the chat window since the previous call, oldest first.
// Returns their number; *lines stays valid until the next call.
int bf_client_game_take_chat(const bf_client_game_chat **lines);

// Called by UIWindowMgr::SendMsg for UIM_PUSHINTOCHATHISTORY (second 0) and
// UIM_PUSHINTOCHATHISTORY2 (second 1): the line, the client's chat type
// (val3) and the talker (val4, may be NULL).
void bf_client_chat_line(int second, const char *text, int type, const char *talker);

#ifdef __cplusplus
}
#endif

#endif
