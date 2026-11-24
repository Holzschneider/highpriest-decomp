# bifrost-bot: the headless second player

The client stays single-instance, so the features that need two players (trade, party, guild, chat, whisper) are tested against this bot. It logs a character in with the game's own `packet.cpp`, `device/PacketQueue.cpp` and `device/Connection.cpp` (like `bifrost-netprobe`), stays on the map, answers other players at packet level, and takes commands from the test harness.

```bash
port/bot/build.sh                 # build/port/bin/bifrost-bot
port/bot/selftest.py              # three bots against the test server, checked in SQL
build/port/bin/bifrost-bot --user bfbota_M --pass bfbot --char-name BfBotA [--control build/a.sock]
```

## Options

| Option | |
|---|---|
| `--user`, `--pass` | account; `BF_BOT_USER`, `BF_BOT_PASS` stand in. A userid ending in `_M`/`_F` is tried without the suffix first and registered with it when the account does not exist, so the same arguments work on every run |
| `--char-name N` | select the character of that name; if the account has none of that name, create it (Str..Luk 5) |
| `--slot N`, `--hair N` | slot (default: first free) and hair style (default 1) of a new character |
| `--control PATH` | also listen for commands on a Unix stream socket (keep the path under 104 bytes) |
| `--no-stdin` | do not read stdin; without it, end of file on stdin logs the bot out |
| `--login-retries N` | retries of a refused login, 4 s apart (default 4): the server limits registrations per interval and refuses an account still logged in |
| `--set key=value` | a policy, as the `set` command |
| `--host`, `--port` | account server, default 127.0.0.1:6900 |

## Commands

One per line, as JSON (`{"cmd":"whisper","name":"BfBotB","text":"hi","id":"7"}`) or plain (`whisper BfBotB hi`). Every command is answered with `{"event":"reply","cmd":..,"ok":..,"error":..}`, carrying `id` back when given; the outcome arrives later as events.

| Command | Plain form | What the bot sends |
|---|---|---|
| `say` | `say <text>` | public chat, `0xf3` with the "name : " prefix the server checks |
| `gm` | `gm "@zeny 100"` / `gm #zeny Name 100` | a game-master command, typed into public chat; answers come as `notice` events |
| `whisper` | `whisper <name> <text>` | `0x96` CZ_WHISPER |
| `party_say` | `party_say <text>` | `0x108` CZ_REQUEST_CHAT_PARTY |
| `guild_say` | `guild_say <text>` | `0x17e` CZ_GUILD_CHAT (in a guild only) |
| `emotion` | `emotion <type>`, or JSON `"type"` | `0xbf` CZ_REQ_EMOTION (the server takes one a second and needs Basic Skill 2) |
| `move` | `move <x> <y>` | `0xa7` CZ_REQUEST_MOVE |
| `create_party` | `create_party <name>` | `0x1e8` CZ_MAKE_GROUP2 |
| `invite_party` | `invite_party <name>` | `0x2c4` CZ_PARTY_JOIN_REQ |
| `leave_party` | | `0x100` CZ_REQ_LEAVE_GROUP |
| `invite_guild` | `invite_guild <name>`, or JSON `"aid"` | `0x168` CZ_REQ_JOIN_GUILD (by account id; a name must be in sight) |
| `leave_guild` | | `0x159` CZ_REQ_LEAVE_GUILD |
| `request_trade` | `request_trade <name>` | `0xe4` CZ_REQ_EXCHANGE_ITEM (the player must be in sight, within 2 cells) |
| `trade_accept`, `trade_decline` | | `0xe6` CZ_ACK_EXCHANGE_ITEM |
| `trade_add` | `trade_add zeny <n>` / `trade_add item <item id> <count>` / `trade_add index <inventory index> <count>` | `0xe8` CZ_ADD_EXCHANGE_ITEM |
| `trade_ok`, `trade_commit`, `trade_cancel` | | `0xeb`, `0xef`, `0xed` |
| `set` | `set <key> <value>` | changes a policy (below) |
| `state` | | answers with a `state` event: position, zeny, party, guild, trade, inventory, actors in sight |
| `raw` | `raw 8c01...` | the bytes as they are, for probing the server |
| `quit` | | `0x18a` CZ_REQ_DISCONNECT, then closes |

Policies (`set` / `--set`), defaults first: `auto_trade on` (accept trade requests), `auto_trade_confirm on` (lock when the partner locks, commit when both have), `trade_offer ""` (what to put in when a trade opens: `zeny N`, `item ID COUNT`, `index I COUNT`), `auto_party on`, `auto_guild on`, `echo_whisper on`, `echo_chat on`, `echo_prefix "echo: "`. The echo answers a whisper by whisper and public chat by public chat with the prefix in front, and never answers a line that already starts with the prefix, so two bots do not echo each other forever.

## Events

JSON lines on stdout and to every control client: `login`, `login_refused`, `created`, `entered`, `position` (`dest_x`, `eta_ms` when a walk starts; `arrived`, `stopped` or `warped`), `player` (a player in sight, by name), `vanish`, `chat`, `notice` (own chat echoed, server messages), `broadcast`, `whisper`, `whisper_result`, `trade_request`, `trade_response`, `trade_open`, `trade_item` (item 0 is zeny), `trade_add_result`, `trade_locked` (`who`: self/partner), `trade_done`, `trade_cancelled`, `party_created`, `party_invite`, `party_chat` (`from`, `id`, `text`; own lines too), `guild_chat` (`from`, `text`; own lines too), `emotion` (`id`, `name`, `type`, `self`: an emote of anyone in sight, the bot's own included), `party_invite_result`, `party_list`, `party_member`, `party_left`, `guild`, `guild_created`, `guild_invite`, `guild_invite_result`, `guild_left`, `item_added`, `zeny`, `skill_failed`, `length_mismatch`, `unknown_packet`, `warning`, `error`, `disconnected`, `exit` (with the headers that could not be framed).

A header the client's length table does not know, or a variable length shorter than its own header, is reported as `unknown_packet` with the packet before it and the first bytes; what is queued is dropped and the bot carries on from the next segment.

## Packets the bot reads

`0x7f` ZC_NOTIFY_TIME, `0xb0`/`0xb1` PAR/LONGPAR_CHANGE (speed, base level, zeny), `0x91` ZC_NPCACK_MAPMOVE (answers with CZ_NOTIFY_ACTORINIT again), `0x92` ZC_NPCACK_SERVERMOVE (reconnects), `0x87` ZC_NOTIFY_PLAYERMOVE, `0x88` ZC_STOPMOVE, `0x86` ZC_NOTIFY_MOVE, `0x80` ZC_NOTIFY_VANISH, `0x2ee`/`0x2ed`/`0x2ec` (actors with a player's look: their names are asked with `0x8c` CZ_REQNAME), `0x78`/`0x7c` (other actors), `0x95`/`0x195` names, `0x8d` ZC_NOTIFY_CHAT, `0x8e` ZC_NOTIFY_PLAYERCHAT, `0x9a` ZC_BROADCAST, `0x97` ZC_WHISPER, `0x98` ZC_ACK_WHISPER, `0x109` ZC_NOTIFY_CHAT_PARTY, `0x17f` ZC_GUILD_CHAT, `0xc0` ZC_EMOTION, `0x1f4`/`0xe5` trade request, `0x1f5`/`0xe7` trade response, `0xe9`, `0xea`, `0xec`, `0xee`, `0xf0` (trade), `0xfa`, `0x2c6`, `0x2c5`, `0xfb`, `0x1e9`/`0x104`, `0x105` (party), `0x16c`, `0x1b4`, `0x1b6`, `0x167`, `0x16a`, `0x169`, `0x15a` (guild), `0x2e8`, `0x2d0`, `0x2d4`/`0x29a`, `0xaf` (inventory), `0x110` ZC_ACK_TOUSESKILL, `0x18b` ZC_ACK_REQ_DISCONNECT. Login: `0x64` CA_LOGIN, `0x65` CH_ENTER, `0x67` CH_MAKE_CHAR, `0x66` CH_SELECT_CHAR, `0x9b` CZ_ENTER (padded), `0x7d` CZ_NOTIFY_ACTORINIT, `0x89` CZ_REQUEST_TIME every 12 s.

## What the server does that the client's table does not expect

- `0x2d4` ZC_ITEM_PICKUP_ACK3: the client's table says 33 bytes, rAthena sends 29. `PACKET_ZC_ITEM_PICKUP_ACK3` declares `time_t HireExpireDate`, 8 bytes on arm64, 4 on the wire. The bot uses 29 and reports it at start (`length_mismatch`); the client needs the field as a 32-bit integer (the same `time_t` is in ZC_ITEM_PICKUP_ACK2, ZC_ADD_QUEST, ZC_QUEST_MISSION_INFO, ZC_MAIL_REQ_OPEN and the EXTRAINFO item records; ACK2's length is set by hand in `packet.cpp`).
- Nothing else: in the self-test (about 400 packets received per run over the three bots) every other header is framed by the client's table.

Behaviour of the server, not of the protocol, that tests must allow for:

- A new character cannot trade (Basic Skill 1) or form a party (Basic Skill 7): the server answers `0x110` and nothing else. The self-test gives the skill with `#allskill`.
- The zone server sends no zeny on entry; the bot takes it from the character record.
- A zeny offer is acknowledged (`0xea`, index 0) only when the offering side locks the trade.
- On accepting a guild invitation the new member gets `0x1b4` ZC_CHANGE_GUILD and `0x1b6`, not `0x16c`. `guild_member` is written on the char server's autosave timer (minutes); `char.guild_id` at logout. A character who rejoins a guild he has left is kicked again at once ("** Guild Mismatch **" in the server log).
- `@recall` of a player standing on the GM's own cell fails ("@recall failed.").
- The zone server sends `0x91` ZC_NPCACK_MAPMOVE once right after entry; the bot loads again as the game would.
