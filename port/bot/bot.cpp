/*
 * bifrost-bot: a headless second player for the tests that need two.
 *
 * The real client stays single-instance, so trade, party, guild, chat and
 * whisper are tested against this bot: it logs a second character in with
 * the game's own connection and packet code (device/Connection.cpp,
 * device/PacketQueue.cpp, packet.cpp, as bifrost-netprobe does), stays on
 * the map, answers other players at packet level and takes commands from the
 * test harness as JSON lines (stdin, or a Unix socket with --control).
 *
 *   bifrost-bot [--host 127.0.0.1] [--port 6900] [--user U] [--pass P]
 *               [--char-name Name] [--slot N] [--hair N] [--control PATH]
 *               [--no-stdin] [--login-retries N] [--set key=value ...]
 *
 * BF_BOT_USER and BF_BOT_PASS stand in for --user and --pass. A userid
 * ending in _M or _F is tried without the suffix first and registered with
 * it when the account does not exist yet, so the same arguments work on
 * every run. Commands and events are listed in port/bot/README.md.
 *
 * Offsets of the packets the server sends are those of rAthena at
 * PACKETVER 20080401; the length of every packet comes from the client's
 * own table, as it would for the game.
 */
#include "common.h"
#include "device/Connection.h"
#include "packet.h"
#include "PaddingPacket.h"
#include "bf_trace.h"

#include "bot_io.h"

#include <map>
#include <set>
#include <string>
#include <vector>

extern "C" const char *bf_packet_name(int header);

namespace {

// ------------------------------------------------------------------ packets

// Little-endian readers over a received packet.
inline unsigned U8(const char *p, int o) { return (unsigned char)p[o]; }
inline unsigned U16(const char *p, int o) { return U8(p, o) | (U8(p, o + 1) << 8); }
inline int S16(const char *p, int o) { return (short)U16(p, o); }
inline unsigned U32(const char *p, int o) { return U16(p, o) | (U16(p, o + 2) << 16); }
inline int S32(const char *p, int o) { return (int)U32(p, o); }

// A fixed-width, possibly unterminated string field.
std::string Str(const char *p, int o, int width)
{
	int n = 0;
	while (n < width && p[o + n])
		++n;
	return std::string(p + o, n);
}

// The text at the end of a variable-length packet, up to its terminator.
std::string Tail(const char *p, int o, int size)
{
	return o < size ? Str(p, o, size - o) : std::string();
}

void PutU16(char *p, int o, unsigned v) { p[o] = (char)v; p[o + 1] = (char)(v >> 8); }
void PutU32(char *p, int o, unsigned v) { PutU16(p, o, v & 0xffff); PutU16(p, o + 2, v >> 16); }

// The 3-byte position of ZC_ACCEPT_ENTER, ZC_NOTIFY_STANDENTRY ...
void DecodePos(const char *p, int o, int *x, int *y)
{
	const unsigned char *b = (const unsigned char *)p + o;
	*x = (b[0] << 2) | (b[1] >> 6);
	*y = ((b[1] & 0x3f) << 4) | (b[2] >> 4);
}

// The 6-byte source and destination of ZC_NOTIFY_MOVE and friends.
void DecodeMove(const char *p, int o, int *x0, int *y0, int *x1, int *y1)
{
	const unsigned char *b = (const unsigned char *)p + o;
	*x0 = (b[0] << 2) | (b[1] >> 6);
	*y0 = ((b[1] & 0x3f) << 4) | (b[2] >> 4);
	*x1 = ((b[2] & 0x0f) << 6) | (b[3] >> 2);
	*y1 = ((b[3] & 0x03) << 8) | b[4];
}

void EncodePos(int x, int y, int dir, BYTE *out)
{
	out[0] = (BYTE)(x >> 2);
	out[1] = (BYTE)((x << 6) | ((y >> 4) & 0x3f));
	out[2] = (BYTE)((y << 4) | (dir & 0x0f));
}

const char *Name(int header)
{
	const char *name = bf_packet_name(header);
	return name ? name : "(no name)";
}

std::string Hex(const char *data, int size)
{
	std::string out;
	char b[4];
	for (int i = 0; i < size; ++i) {
		snprintf(b, sizeof(b), "%02x", (unsigned char)data[i]);
		out += b;
	}
	return out;
}

std::string HeaderText(int header)
{
	char b[8];
	snprintf(b, sizeof(b), "0x%04x", header);
	return b;
}

// Packet headers the bot uses that packet.h knows only by number, because
// this client's request ids are shuffled (see packet.cpp and rAthena's
// 2007-02-12aSakexe block).
enum {
	BOT_CZ_ENTER = 0x9b,
	BOT_CZ_REQUEST_MOVE = 0xa7,
	BOT_CZ_REQUEST_TIME = 0x89,
	BOT_CZ_REQUEST_CHAT = 0xf3,
	BOT_CZ_REQNAME = 0x8c,
};

// Server packets whose length in the client's table differs from what
// rAthena sends at PACKETVER 20080401. The bot uses the server's value and
// reports the difference at start: these are defects for the port to fix in
// packet.h, not for the bot to hide.
struct LengthFix { int header; int serverLength; const char *why; };
const LengthFix kLengthFixes[] = {
	// PACKET_ZC_ITEM_PICKUP_ACK3 declares HireExpireDate as time_t, which is
	// 8 bytes on arm64; the server sends a 4-byte field.
	{ 0x2d4, 29, "ZC_ITEM_PICKUP_ACK3: time_t HireExpireDate is 8 bytes in this build, 4 on the wire" },
};

// ------------------------------------------------------------------ connection

class BotConnection : public CRagConnection {
public:
	enum Result { NOTHING, PACKET, UNKNOWN_HEADER, BAD_LENGTH };

	// Takes the next complete packet off the receive queue. A header the
	// client's table does not know, or a variable length shorter than its own
	// header, is reported instead: the packet boundary after it is lost.
	Result Next(char *buf, int capacity, int *header, int *size)
	{
		WORD type;
		if (!m_recvQueue.PeekData(2, (char *)&type))
			return NOTHING;
		*header = type;
		PacketLenMap::const_iterator it = g_packetLenMap.find(type);
		if (it == g_packetLenMap.end() || it->second == 0)
			return UNKNOWN_HEADER;
		int len = it->second;
		if (len == -1) {
			char head[4];
			if (!m_recvQueue.PeekData(4, head))
				return NOTHING;
			len = U16(head, 2);
			if (len < 4 || len > capacity) {
				*size = len;
				return BAD_LENGTH;
			}
		} else if (len < 2 || len > capacity) {
			*size = len;
			return BAD_LENGTH;
		}
		if (!m_recvQueue.GetData(len, buf))
			return NOTHING;
		bf_trace_packet('R', buf, len);
		*size = len;
		return PACKET;
	}

	int Pending() const { return m_recvQueue.GetSize(); }

	int PeekRaw(char *buf, int size) const
	{
		if (size > m_recvQueue.GetSize())
			size = m_recvQueue.GetSize();
		m_recvQueue.PeekData(size, buf);
		return size;
	}

	// Drops everything received so far; the stream resumes at the next
	// segment, which in practice starts with a packet.
	int Drop()
	{
		int n = m_recvQueue.GetSize();
		m_recvQueue.RemoveData(n);
		return n;
	}

	void SendRaw(const void *data, int size) { SendPacket(size, (const char *)data); }

	// A fixed-size request, sized by the client's table as the game does.
	void SendFixed(const void *data)
	{
		WORD type = *(const WORD *)data;
		SendPacket(GetPacketSize(type), (const char *)data);
	}
};

bool Connect(BotConnection &conn, const char *ip, int port)
{
	ServerAddress addr;
	memset(&addr, 0, sizeof(addr));
	strncpy(addr.ip, ip, sizeof(addr.ip) - 1);
	addr.port = port;
	return conn.Connect(&addr);
}

std::string Ip(DWORD address)
{
	in_addr in;
	in.s_addr = address;
	return inet_ntoa(in);
}

// ------------------------------------------------------------------ events

// One output line: {"event":"name", ...}.
class Event {
public:
	explicit Event(const char *name) : m_text("{\"event\":" + bf_bot_json_string(name)) {}
	Event &S(const char *key, const std::string &value) { Key(key); m_text += bf_bot_json_string(value); return *this; }
	Event &I(const char *key, long long value) { Key(key); m_text += std::to_string(value); return *this; }
	Event &B(const char *key, bool value) { Key(key); m_text += value ? "true" : "false"; return *this; }
	Event &Raw(const char *key, const std::string &json) { Key(key); m_text += json; return *this; }
	void Emit() { bf_bot_io_emit(m_text + "}"); }
	std::string Text() const { return m_text + "}"; }
private:
	void Key(const char *key) { m_text += ","; m_text += bf_bot_json_string(key); m_text += ":"; }
	std::string m_text;
};

// ------------------------------------------------------------------ the bot

struct Options {
	std::string host, user, pass, charName, control;
	int port, slot, hair, loginRetries;
	bool useStdin;
	std::vector<std::string> sets;
};

struct Actor {
	DWORD id;
	int x, y;
	int job;
	bool pcLook;     // sent with the player-look packets (0x2ec..0x2ee)
	std::string name;
	bool nameAsked;
};

struct Item {
	int index, itid, count;
	bool equip;
};

struct PartyMember {
	DWORD aid;
	std::string name, map;
	bool leader, online;
};

class Bot {
public:
	explicit Bot(const Options &o);
	int Run();

private:
	// Login
	bool Login();
	bool EnterZone(const std::string &ip, int port);
	int Wait(char *buf, int timeoutMs);
	int WaitFor(char *buf, int timeoutMs, int first, int second);
	bool WaitRaw(char *buf, int size, int timeoutMs);
	bool Fail(const char *stage, const char *format, ...);

	// Play
	void Dispatch(const char *p, int size);
	void Command(const std::string &line);
	void Reply(const BfBotFields &f, bool ok, const std::string &error = std::string(), const std::string &extraJson = std::string());
	bool SetPolicy(const std::string &key, const std::string &value, std::string *error);
	void Tick(bool now);
	void UpdateMovement();
	void SendChat(const std::string &text);
	void SendWhisper(const std::string &to, const std::string &text);
	void SendMove(int x, int y);
	void RequestName(DWORD id);
	void SendTradeOffer();
	void TradeReset();
	void OnTradeLocked(int who);
	void Quit();
	std::string StateJson();
	std::string PositionJson();
	Actor *ActorByName(const std::string &name);
	void SeeActor(DWORD id, int job, int x, int y, bool pcLook);

	Options o;
	BotConnection conn;
	char buf[65536];

	// Session
	DWORD aid, gid, authCode, userLevel;
	BYTE sex;
	std::string charName, map;
	bool inZone, running;
	int exitCode;
	DWORD lastTimeSync, lastServerTime;

	// Position: last known cell, and the walk in progress.
	int x, y, destX, destY, speed;
	bool moving;
	DWORD moveStart, moveEnd;

	long long zeny;
	int baseLevel;

	std::map<DWORD, Actor> actors;
	std::map<int, Item> items;

	// Party and guild
	std::string partyName;
	std::vector<PartyMember> members;
	DWORD guildId;
	std::string guildName;
	bool guildMaster;

	// Trade
	enum TradeState { TRADE_NONE, TRADE_ASKED, TRADE_INVITED, TRADE_OPEN };
	TradeState trade;
	std::string tradePartner;
	DWORD tradePartnerId;
	bool tradeSelfLocked, tradePartnerLocked, tradeCommitted, tradeOffered;

	// Policies, settable with --set and the "set" command
	bool autoTrade, autoTradeConfirm, autoParty, autoGuild, echoWhisper, echoChat;
	std::string echoPrefix, tradeOffer;

	// Diagnostics
	std::map<int, int> unknownHeaders;
	int lastHeader, lastSize;
	long long packetsIn;
};

Bot::Bot(const Options &opt)
	: o(opt), aid(0), gid(0), authCode(0), userLevel(0), sex(0), inZone(false), running(true), exitCode(0),
	  lastTimeSync(0), lastServerTime(0), x(0), y(0), destX(0), destY(0), speed(150), moving(false),
	  moveStart(0), moveEnd(0), zeny(-1), baseLevel(0), guildId(0), guildMaster(false), trade(TRADE_NONE),
	  tradePartnerId(0), tradeSelfLocked(false), tradePartnerLocked(false), tradeCommitted(false), tradeOffered(false),
	  autoTrade(true), autoTradeConfirm(true), autoParty(true), autoGuild(true), echoWhisper(true), echoChat(true),
	  echoPrefix("echo: "), lastHeader(0), lastSize(0), packetsIn(0)
{
}

bool Bot::Fail(const char *stage, const char *format, ...)
{
	char msg[512];
	va_list args;
	va_start(args, format);
	vsnprintf(msg, sizeof(msg), format, args);
	va_end(args);
	fprintf(stderr, "bifrost-bot: FAILED (%s): %s\n", stage, msg);
	Event("error").S("stage", stage).S("message", msg).Emit();
	return false;
}

// Polls until a packet arrives. Returns its header, 0 on timeout or a closed
// connection, -1 on a packet that cannot be framed.
int Bot::Wait(char *b, int timeoutMs)
{
	DWORD deadline = timeGetTime() + timeoutMs;
	while (timeGetTime() < deadline) {
		bool alive = conn.Poll();
		int header = 0, size = 0;
		switch (conn.Next(b, sizeof(buf), &header, &size)) {
		case BotConnection::PACKET:
			lastSize = size;
			return header;
		case BotConnection::UNKNOWN_HEADER:
		case BotConnection::BAD_LENGTH: {
			char raw[32];
			int n = conn.PeekRaw(raw, sizeof(raw));
			fprintf(stderr, "bifrost-bot: cannot frame header 0x%04x during login (bytes %s)\n", header, Hex(raw, n).c_str());
			return -1;
		}
		case BotConnection::NOTHING:
			break;
		}
		if (!alive)
			return 0;
		usleep(2000);
	}
	return 0;
}

int Bot::WaitFor(char *b, int timeoutMs, int first, int second)
{
	DWORD deadline = timeGetTime() + timeoutMs;
	for (;;) {
		int remaining = (int)(deadline - timeGetTime());
		int header = remaining > 0 ? Wait(b, remaining) : 0;
		if (header <= 0 || header == first || header == second)
			return header;
		BF_TRACE(BFT_NET, "passing over 0x%04x %s", header, Name(header));
	}
}

bool Bot::WaitRaw(char *b, int size, int timeoutMs)
{
	DWORD deadline = timeGetTime() + timeoutMs;
	while (timeGetTime() < deadline) {
		bool alive = conn.Poll();
		if (conn.Recv(b, size, true))
			return true;
		if (!alive)
			return false;
		usleep(2000);
	}
	return false;
}

// Account server, character server (creating the character if needed),
// zone server: the steps of the game's login mode.
bool Bot::Login()
{
	// A userid with an _M / _F suffix registers a new account; once the
	// account exists, only the plain userid logs in.
	std::vector<std::string> ids;
	size_t len = o.user.size();
	bool registering = len > 2 && o.user[len - 2] == '_' && (o.user[len - 1] == 'M' || o.user[len - 1] == 'F'
		|| o.user[len - 1] == 'm' || o.user[len - 1] == 'f');
	if (registering)
		ids.push_back(o.user.substr(0, len - 2));
	ids.push_back(o.user);

	PACKET_AC_ACCEPT_LOGIN accept;
	SERVER_ADDR charServer;
	bool accepted = false;
	for (int attempt = 0; !accepted && attempt <= o.loginRetries; ++attempt) {
		if (attempt > 0)
			usleep(4000 * 1000);
		for (size_t k = 0; k < ids.size() && !accepted; ++k) {
			if (!Connect(conn, o.host.c_str(), o.port))
				return Fail("account", "cannot connect to the account server %s:%d", o.host.c_str(), o.port);
			PACKET_CA_LOGIN login;
			memset(&login, 0, sizeof(login));
			login.PacketType = HEADER_CA_LOGIN;
			login.Version = 20;
			strncpy((char *)login.ID, ids[k].c_str(), sizeof(login.ID) - 1);
			strncpy((char *)login.Passwd, o.pass.c_str(), sizeof(login.Passwd) - 1);
			login.clienttype = 0;
			conn.SendFixed(&login);

			int header = Wait(buf, 5000);
			if (header == HEADER_AC_ACCEPT_LOGIN) {
				accept = *(PACKET_AC_ACCEPT_LOGIN *)buf;
				int servers = (accept.PacketLength - sizeof(accept)) / sizeof(SERVER_ADDR);
				if (servers < 1)
					return Fail("account", "the account server listed no character server");
				charServer = *(SERVER_ADDR *)(buf + sizeof(accept));
				accepted = true;
				Event("login").S("stage", "account").S("userid", ids[k]).I("aid", accept.AID).Emit();
			} else if (header == HEADER_AC_REFUSE_LOGIN) {
				int code = ((PACKET_AC_REFUSE_LOGIN *)buf)->ErrorCode;
				Event("login_refused").S("userid", ids[k]).I("code", code).Emit();
				// 0: no such account, try the registering userid next. 3: the
				// server limits registrations per interval; 8: still logged
				// in elsewhere: both clear after a few seconds.
			} else {
				conn.Disconnect();
				return Fail("account", "expected AC_ACCEPT_LOGIN, got 0x%04x %s", header, Name(header));
			}
			conn.Disconnect();
		}
	}
	if (!accepted)
		return Fail("account", "the account server refused %s", o.user.c_str());
	aid = accept.AID;
	authCode = accept.AuthCode;
	userLevel = accept.userLevel;
	sex = accept.Sex;

	// Character server.
	std::string charIp = Ip(charServer.ip);
	if (!Connect(conn, charIp.c_str(), charServer.port))
		return Fail("char", "cannot connect to the character server %s:%d", charIp.c_str(), charServer.port);
	PACKET_CH_ENTER enter;
	memset(&enter, 0, sizeof(enter));
	enter.PacketType = HEADER_CH_ENTER;
	enter.AID = accept.AID;
	enter.AuthCode = accept.AuthCode;
	enter.userLevel = accept.userLevel;
	enter.clientType = 0;
	enter.Sex = accept.Sex;
	conn.SendFixed(&enter);

	DWORD echoedAid = 0;
	if (!WaitRaw((char *)&echoedAid, 4, 5000) || echoedAid != accept.AID)
		return Fail("char", "the character server did not echo the account id");
	int header = Wait(buf, 5000);
	if (header == HEADER_HC_REFUSE_ENTER)
		return Fail("char", "the character server refused, error code %d", ((PACKET_HC_REFUSE_ENTER *)buf)->ErrorCode);
	if (header != HEADER_HC_ACCEPT_ENTER)
		return Fail("char", "expected HC_ACCEPT_ENTER, got 0x%04x %s", header, Name(header));

	// Header, length and 20 reserved bytes, then 108-byte character records
	// (CHARACTER_INFO): zeny at 8, walk speed at 50, base level at 58, name
	// at 74, slot at 104. The zone server does not send the zeny on entry.
	const int kRecord = 108;
	int count = (U16(buf, 2) - 24) / kRecord;
	int slot = -1;
	std::set<int> used;
	std::string names = "[";
	for (int i = 0; i < count; ++i) {
		const char *c = buf + 24 + i * kRecord;
		std::string name = Str(c, 74, 24);
		used.insert(U8(c, 104));
		names += (i ? "," : "") + bf_bot_json_string(name);
		if (slot < 0 && (o.charName.empty() || name == o.charName)) {
			slot = U8(c, 104);
			charName = name;
			zeny = S32(c, 8);
			speed = S16(c, 50) > 0 ? S16(c, 50) : speed;
			baseLevel = S16(c, 58);
		}
	}
	Event("login").S("stage", "char").I("characters", count).Raw("names", names + "]").Emit();

	if (slot < 0) {
		if (o.charName.empty())
			o.charName = "BfBot";
		int make = o.slot;
		if (make < 0)
			for (make = 0; used.count(make); ++make) {}
		PACKET_CH_MAKE_CHAR mk;
		memset(&mk, 0, sizeof(mk));
		mk.PacketType = HEADER_CH_MAKE_CHAR;
		strncpy((char *)mk.name, o.charName.c_str(), sizeof(mk.name) - 1);
		mk.Str = mk.Agi = mk.Vit = mk.Int = mk.Dex = mk.Luk = 5;
		mk.CharNum = (BYTE)make;
		mk.headPal = 0;
		mk.head = (WORD)o.hair;
		conn.SendFixed(&mk);
		header = WaitFor(buf, 5000, HEADER_HC_ACCEPT_MAKECHAR, HEADER_HC_REFUSE_MAKECHAR);
		if (header == HEADER_HC_REFUSE_MAKECHAR)
			return Fail("char", "the character server refused to create \"%s\" in slot %d, error code %d",
				o.charName.c_str(), make, ((PACKET_HC_REFUSE_MAKECHAR *)buf)->ErrorCode);
		if (header != HEADER_HC_ACCEPT_MAKECHAR)
			return Fail("char", "character creation: expected HC_ACCEPT_MAKECHAR, got 0x%04x %s", header, Name(header));
		charName = Str(buf, 2 + 74, 24);
		slot = U8(buf, 2 + 104);
		zeny = S32(buf, 2 + 8);
		speed = S16(buf, 2 + 50) > 0 ? S16(buf, 2 + 50) : speed;
		baseLevel = S16(buf, 2 + 58);
		Event("created").S("name", charName).I("slot", slot).I("hair", o.hair).Emit();
	}

	PACKET_CH_SELECT_CHAR select;
	select.PacketType = HEADER_CH_SELECT_CHAR;
	select.CharNum = (BYTE)slot;
	conn.SendFixed(&select);
	header = WaitFor(buf, 5000, HEADER_HC_NOTIFY_ZONESVR, HEADER_HC_REFUSE_SELECTCHAR);
	if (header != HEADER_HC_NOTIFY_ZONESVR)
		return Fail("char", "expected HC_NOTIFY_ZONESVR, got 0x%04x %s", header, Name(header));
	PACKET_HC_NOTIFY_ZONESVR zone = *(PACKET_HC_NOTIFY_ZONESVR *)buf;
	gid = zone.GID;
	map = Str((const char *)zone.mapName, 0, 16);
	conn.Disconnect();
	Event("login").S("stage", "select").S("name", charName).I("slot", slot).I("gid", gid).S("map", map).Emit();

	return EnterZone(Ip(zone.addr.ip), zone.addr.port);
}

bool Bot::EnterZone(const std::string &ip, int port)
{
	if (!Connect(conn, ip.c_str(), port))
		return Fail("zone", "cannot connect to the zone server %s:%d", ip.c_str(), port);
	PACKET_CZ_ENTER cz;
	memset(&cz, 0, sizeof(cz));
	cz.PacketType = BOT_CZ_ENTER;
	cz.AID = aid;
	cz.GID = gid;
	cz.AuthCode = authCode;
	cz.clientTime = timeGetTime();
	cz.Sex = sex;
	conn.SendFixed(&cz);

	// The zone server answers with the account id as four bare bytes, then
	// ZC_ACCEPT_ENTER2; anything ahead of that is dispatched as usual.
	DWORD deadline = timeGetTime() + 8000;
	while (timeGetTime() < deadline) {
		int header = Wait(buf, (int)(deadline - timeGetTime()));
		if (header <= 0)
			break;
		if (header == HEADER_ZC_REFUSE_ENTER)
			return Fail("zone", "the zone server refused, error code %d", U8(buf, 2));
		if (header == HEADER_ZC_ACCEPT_ENTER || header == HEADER_ZC_ACCEPT_ENTER2) {
			DecodePos(buf, 6, &x, &y);
			destX = x;
			destY = y;
			moving = false;
			inZone = true;
			actors.clear();
			// What the game sends once the map has loaded.
			PACKET_CZ_NOTIFY_ACTORINIT loaded;
			loaded.PacketType = HEADER_CZ_NOTIFY_ACTORINIT;
			conn.SendFixed(&loaded);
			Tick(true);
			Event("entered").S("name", charName).I("aid", aid).I("gid", gid).S("map", map).I("x", x).I("y", y).Emit();
			return true;
		}
		Dispatch(buf, lastSize);
	}
	return Fail("zone", "the zone server did not accept the character");
}

void Bot::Tick(bool now)
{
	DWORD t = timeGetTime();
	if (!now && t - lastTimeSync < 12000)
		return;
	lastTimeSync = t;
	PACKET_CZ_REQUEST_TIME p;
	memset(&p, 0, sizeof(p));
	p.PacketType = BOT_CZ_REQUEST_TIME;
	p.clientTime = t;
	conn.SendFixed(&p);
}

// ------------------------------------------------------------------ requests

void Bot::SendChat(const std::string &text)
{
	// The server checks the "name : " prefix the game puts in front.
	std::string line = charName + " : " + text;
	if (line.size() > 255)
		line.resize(255);
	char p[300];
	int size = 4 + (int)line.size() + 1;
	PutU16(p, 0, BOT_CZ_REQUEST_CHAT);
	PutU16(p, 2, size);
	memcpy(p + 4, line.c_str(), line.size() + 1);
	conn.SendRaw(p, size);
}

void Bot::SendWhisper(const std::string &to, const std::string &text)
{
	char p[300];
	std::string t = text.substr(0, 200);
	int size = 28 + (int)t.size() + 1;
	memset(p, 0, sizeof(p));
	PutU16(p, 0, HEADER_CZ_WHISPER);
	PutU16(p, 2, size);
	strncpy(p + 4, to.c_str(), NAME_LENGTH - 1);
	memcpy(p + 28, t.c_str(), t.size() + 1);
	conn.SendRaw(p, size);
}

void Bot::SendMove(int tx, int ty)
{
	PACKET_CZ_REQUEST_MOVE p;
	memset(&p, 0, sizeof(p));
	p.PacketType = BOT_CZ_REQUEST_MOVE;
	EncodePos(tx, ty, 0, p.dest);
	conn.SendFixed(&p);
}

void Bot::RequestName(DWORD id)
{
	PACKET_CZ_REQNAME p;
	memset(&p, 0, sizeof(p));
	p.PacketType = BOT_CZ_REQNAME;
	p.AID = id;
	conn.SendFixed(&p);
}

void Bot::Quit()
{
	if (conn.IsConnect()) {
		PACKET_CZ_REQ_DISCONNECT q;
		memset(&q, 0, sizeof(q));
		q.PacketType = HEADER_CZ_REQ_DISCONNECT;
		conn.SendFixed(&q);
		// The server answers ZC_ACK_REQ_DISCONNECT and closes; it refuses
		// for a while after login, and closing the socket logs out anyway.
		DWORD deadline = timeGetTime() + 1500;
		while (timeGetTime() < deadline && conn.IsConnect()) {
			int header = Wait(buf, (int)(deadline - timeGetTime()));
			if (header <= 0 || header == HEADER_ZC_ACK_REQ_DISCONNECT)
				break;
		}
		conn.Disconnect();
	}
	running = false;
}

// ------------------------------------------------------------------ state

Actor *Bot::ActorByName(const std::string &name)
{
	for (std::map<DWORD, Actor>::iterator it = actors.begin(); it != actors.end(); ++it)
		if (it->second.name == name)
			return &it->second;
	return NULL;
}

void Bot::SeeActor(DWORD id, int job, int ax, int ay, bool pcLook)
{
	if (id == aid || id == gid)
		return;
	Actor &a = actors[id];
	bool fresh = a.id == 0;
	a.id = id;
	a.job = job;
	a.x = ax;
	a.y = ay;
	a.pcLook = pcLook;
	if (fresh) {
		a.nameAsked = false;
	}
	// Names are asked for every actor with a player's look, so that commands
	// can address players by name.
	if (pcLook && !a.nameAsked) {
		a.nameAsked = true;
		RequestName(id);
	}
}

void Bot::UpdateMovement()
{
	if (moving && (int)(timeGetTime() - moveEnd) >= 0) {
		moving = false;
		x = destX;
		y = destY;
		Event("position").S("map", map).I("x", x).I("y", y).B("arrived", true).Emit();
	}
}

std::string Bot::PositionJson()
{
	char b[160];
	snprintf(b, sizeof(b), "{\"map\":%s,\"x\":%d,\"y\":%d,\"dest_x\":%d,\"dest_y\":%d,\"moving\":%s}",
		bf_bot_json_string(map).c_str(), x, y, destX, destY, moving ? "true" : "false");
	return b;
}

std::string Bot::StateJson()
{
	std::string a = "[";
	bool first = true;
	for (std::map<DWORD, Actor>::iterator it = actors.begin(); it != actors.end(); ++it) {
		const Actor &ac = it->second;
		char b[200];
		snprintf(b, sizeof(b), "%s{\"id\":%u,\"name\":%s,\"job\":%d,\"x\":%d,\"y\":%d,\"player_look\":%s}",
			first ? "" : ",", (unsigned)ac.id, bf_bot_json_string(ac.name).c_str(), ac.job, ac.x, ac.y, ac.pcLook ? "true" : "false");
		a += b;
		first = false;
	}
	a += "]";

	std::string m = "[";
	for (size_t i = 0; i < members.size(); ++i) {
		char b[200];
		snprintf(b, sizeof(b), "%s{\"aid\":%u,\"name\":%s,\"leader\":%s,\"online\":%s}", i ? "," : "",
			(unsigned)members[i].aid, bf_bot_json_string(members[i].name).c_str(),
			members[i].leader ? "true" : "false", members[i].online ? "true" : "false");
		m += b;
	}
	m += "]";

	std::string inv = "[";
	first = true;
	for (std::map<int, Item>::iterator it = items.begin(); it != items.end(); ++it) {
		char b[120];
		snprintf(b, sizeof(b), "%s{\"index\":%d,\"item\":%d,\"count\":%d,\"equip\":%s}", first ? "" : ",",
			it->second.index, it->second.itid, it->second.count, it->second.equip ? "true" : "false");
		inv += b;
		first = false;
	}
	inv += "]";

	static const char *kTrade[] = { "none", "asked", "invited", "open" };
	std::string t = "{\"state\":" + bf_bot_json_string(kTrade[trade]) + ",\"partner\":" + bf_bot_json_string(tradePartner)
		+ ",\"self_locked\":" + (tradeSelfLocked ? "true" : "false")
		+ ",\"partner_locked\":" + (tradePartnerLocked ? "true" : "false") + "}";

	return Event("state").S("name", charName).I("aid", aid).I("gid", gid).B("in_zone", inZone)
		.Raw("position", PositionJson()).I("zeny", zeny).I("base_level", baseLevel)
		.S("party", partyName).Raw("party_members", m).I("guild_id", guildId).S("guild", guildName)
		.Raw("trade", t).Raw("inventory", inv).Raw("actors", a).Text();
}

// ------------------------------------------------------------------ trade

void Bot::TradeReset()
{
	trade = TRADE_NONE;
	tradePartner.clear();
	tradePartnerId = 0;
	tradeSelfLocked = tradePartnerLocked = tradeCommitted = tradeOffered = false;
}

// trade_offer policy: "" (nothing), "zeny N", "item ITEMID COUNT" or
// "index INDEX COUNT".
void Bot::SendTradeOffer()
{
	if (tradeOffered || tradeOffer.empty())
		return;
	tradeOffered = true;
	char kind[16] = "";
	int a = 0, b = 1;
	if (sscanf(tradeOffer.c_str(), "%15s %d %d", kind, &a, &b) < 2)
		return;
	PACKET_CZ_ADD_EXCHANGE_ITEM p;
	memset(&p, 0, sizeof(p));
	p.PacketType = HEADER_CZ_ADD_EXCHANGE_ITEM;
	if (!strcmp(kind, "zeny")) {
		p.index = 0;
		p.count = a;
	} else if (!strcmp(kind, "index")) {
		p.index = (short)a;
		p.count = b;
	} else {
		int index = -1;
		for (std::map<int, Item>::iterator it = items.begin(); it != items.end(); ++it)
			if (it->second.itid == a && !it->second.equip)
				index = it->first;
		if (index < 0)
			for (std::map<int, Item>::iterator it = items.begin(); it != items.end(); ++it)
				if (it->second.itid == a)
					index = it->first;
		if (index < 0) {
			Event("trade_offer_failed").S("offer", tradeOffer).S("reason", "item not in inventory").Emit();
			return;
		}
		p.index = (short)index;
		p.count = b;
	}
	conn.SendFixed(&p);
}

void Bot::OnTradeLocked(int who)
{
	if (who == 0)
		tradeSelfLocked = true;
	else
		tradePartnerLocked = true;
	Event("trade_locked").S("who", who == 0 ? "self" : "partner").S("partner", tradePartner).Emit();
	if (!autoTradeConfirm)
		return;
	if (tradePartnerLocked && !tradeSelfLocked) {
		SendTradeOffer();
		PACKET_CZ_CONCLUDE_EXCHANGE_ITEM p;
		p.PacketType = HEADER_CZ_CONCLUDE_EXCHANGE_ITEM;
		conn.SendFixed(&p);
	}
	if (tradePartnerLocked && tradeSelfLocked && !tradeCommitted) {
		tradeCommitted = true;
		PACKET_CZ_EXEC_EXCHANGE_ITEM p;
		p.PacketType = HEADER_CZ_EXEC_EXCHANGE_ITEM;
		conn.SendFixed(&p);
	}
}

// ------------------------------------------------------------------ dispatch

void Bot::Dispatch(const char *p, int size)
{
	int header = U16(p, 0);
	++packetsIn;
	int ax, ay, bx, by;

	switch (header) {
	// --- time, self
	case HEADER_ZC_NOTIFY_TIME:
		lastServerTime = U32(p, 2);
		break;
	case HEADER_ZC_AID:
		break;
	case HEADER_ZC_PAR_CHANGE:
		if (U16(p, 2) == 0)       // SP_SPEED
			speed = S32(p, 4) > 0 ? S32(p, 4) : speed;
		else if (U16(p, 2) == 11) // SP_BASELEVEL
			baseLevel = S32(p, 4);
		break;
	case HEADER_ZC_LONGPAR_CHANGE:
		if (U16(p, 2) == 20) {    // SP_ZENY
			long long before = zeny;
			zeny = S32(p, 4);
			if (before >= 0 && before != zeny)
				Event("zeny").I("zeny", zeny).I("change", zeny - before).Emit();
		}
		break;
	case HEADER_ZC_NPCACK_MAPMOVE: {
		map = Str(p, 2, 16);
		x = destX = S16(p, 18);
		y = destY = S16(p, 20);
		moving = false;
		actors.clear();
		PACKET_CZ_NOTIFY_ACTORINIT loaded;
		loaded.PacketType = HEADER_CZ_NOTIFY_ACTORINIT;
		conn.SendFixed(&loaded);
		Event("position").S("map", map).I("x", x).I("y", y).B("warped", true).Emit();
		break;
	}
	case HEADER_ZC_NPCACK_SERVERMOVE: {
		map = Str(p, 2, 16);
		x = destX = S16(p, 18);
		y = destY = S16(p, 20);
		std::string ip = Ip(U32(p, 22));
		int port = U16(p, 26);
		Event("server_move").S("map", map).S("ip", ip).I("port", port).Emit();
		conn.Disconnect();
		if (!EnterZone(ip, port))
			running = false, exitCode = 1;
		break;
	}

	// --- movement
	case HEADER_ZC_NOTIFY_PLAYERMOVE: {
		DecodeMove(p, 6, &ax, &ay, &bx, &by);
		x = ax;
		y = ay;
		destX = bx;
		destY = by;
		int dx = abs(bx - ax), dy = abs(by - ay);
		int diag = dx < dy ? dx : dy, straight = (dx > dy ? dx : dy) - diag;
		moveStart = timeGetTime();
		moveEnd = moveStart + (DWORD)((diag * 14 + straight * 10) * speed / 10);
		moving = true;
		Event("position").S("map", map).I("x", x).I("y", y).I("dest_x", destX).I("dest_y", destY)
			.I("eta_ms", moveEnd - moveStart).Emit();
		break;
	}
	case HEADER_ZC_STOPMOVE:
		if (U32(p, 2) == aid || U32(p, 2) == gid) {
			x = destX = S16(p, 6);
			y = destY = S16(p, 8);
			moving = false;
			Event("position").S("map", map).I("x", x).I("y", y).B("stopped", true).Emit();
		} else if (actors.count(U32(p, 2))) {
			actors[U32(p, 2)].x = S16(p, 6);
			actors[U32(p, 2)].y = S16(p, 8);
		}
		break;
	case HEADER_ZC_NOTIFY_MOVE:
		DecodeMove(p, 6, &ax, &ay, &bx, &by);
		if (actors.count(U32(p, 2))) {
			actors[U32(p, 2)].x = bx;
			actors[U32(p, 2)].y = by;
		}
		break;
	case HEADER_ZC_NOTIFY_VANISH: {
		DWORD id = U32(p, 2);
		std::map<DWORD, Actor>::iterator it = actors.find(id);
		if (it != actors.end()) {
			if (it->second.pcLook)
				Event("vanish").I("id", id).S("name", it->second.name).I("type", U8(p, 6)).Emit();
			actors.erase(it);
		}
		break;
	}

	// --- actors (offsets of rAthena's idle_unit / spawn_unit / unit_walking
	// at this version: 0x2ee and 0x2ed without, 0x2ec with an object type)
	case 0x2ee:
	case 0x2ed:
		DecodePos(p, 50, &ax, &ay);
		SeeActor(U32(p, 2), S16(p, 16), ax, ay, true);
		break;
	case 0x2ec:
		DecodeMove(p, 55, &ax, &ay, &bx, &by);
		SeeActor(U32(p, 3), S16(p, 17), bx, by, true);
		break;
	case HEADER_ZC_NOTIFY_STANDENTRY:   // 0x78, with an object type
		DecodePos(p, 47, &ax, &ay);
		SeeActor(U32(p, 3), S16(p, 15), ax, ay, false);
		break;
	case HEADER_ZC_NOTIFY_STANDENTRY_NPC: // 0x7c
		DecodePos(p, 37, &ax, &ay);
		SeeActor(U32(p, 3), S16(p, 21), ax, ay, false);
		break;
	case HEADER_ZC_ACK_REQNAME:
	case HEADER_ZC_ACK_REQNAMEALL: {
		DWORD id = U32(p, 2);
		std::string name = Str(p, 6, 24);
		std::map<DWORD, Actor>::iterator it = actors.find(id);
		if (it != actors.end()) {
			bool fresh = it->second.name.empty();
			it->second.name = name;
			if (fresh && it->second.pcLook)
				Event("player").I("id", id).S("name", name).I("x", it->second.x).I("y", it->second.y).Emit();
		}
		break;
	}

	// --- chat
	case HEADER_ZC_NOTIFY_CHAT: {
		DWORD id = U32(p, 4);
		std::string line = Tail(p, 8, size);
		std::string from, text = line;
		size_t sep = line.find(" : ");
		if (sep != std::string::npos) {
			from = line.substr(0, sep);
			text = line.substr(sep + 3);
		}
		Event("chat").S("from", from).I("id", id).S("text", text).Emit();
		if (echoChat && id != aid && text.compare(0, echoPrefix.size(), echoPrefix) != 0)
			SendChat(echoPrefix + text);
		break;
	}
	case HEADER_ZC_NOTIFY_PLAYERCHAT:
		// Our own chat coming back, and the server's messages to us (the
		// answers to game-master commands among them).
		Event("notice").S("text", Tail(p, 4, size)).Emit();
		break;
	case HEADER_ZC_BROADCAST:
		Event("broadcast").S("text", Tail(p, 4, size)).Emit();
		break;
	case HEADER_ZC_WHISPER: {
		std::string from = Str(p, 4, 24);
		std::string text = Tail(p, 28, size);
		Event("whisper").S("from", from).S("text", text).Emit();
		if (echoWhisper && !from.empty() && from != charName && text.compare(0, echoPrefix.size(), echoPrefix) != 0)
			SendWhisper(from, echoPrefix + text);
		break;
	}
	case HEADER_ZC_NOTIFY_CHAT_PARTY: {
		// Party chat, the sender's own line included: "name : text".
		std::string line = Tail(p, 8, size);
		std::string from, text = line;
		size_t sep = line.find(" : ");
		if (sep != std::string::npos) {
			from = line.substr(0, sep);
			text = line.substr(sep + 3);
		}
		Event("party_chat").S("from", from).I("id", U32(p, 4)).S("text", text).Emit();
		break;
	}
	case HEADER_ZC_GUILD_CHAT: {
		std::string line = Tail(p, 4, size);
		std::string from, text = line;
		size_t sep = line.find(" : ");
		if (sep != std::string::npos) {
			from = line.substr(0, sep);
			text = line.substr(sep + 3);
		}
		Event("guild_chat").S("from", from).S("text", text).Emit();
		break;
	}
	case HEADER_ZC_EMOTION: {
		// Any actor's emote in sight, the bot's own included.
		DWORD id = U32(p, 2);
		std::map<DWORD, Actor>::iterator it = actors.find(id);
		Event("emotion").I("id", id).S("name", id == aid || id == gid ? charName : it != actors.end() ? it->second.name : "")
			.I("type", U8(p, 6)).B("self", id == aid || id == gid).Emit();
		break;
	}
	case HEADER_ZC_ACK_WHISPER:
		// 0 sent, 1 not online, 2 ignored, 3 ignoring everyone
		Event("whisper_result").I("result", U8(p, 2)).Emit();
		break;

	// --- trade
	case HEADER_ZC_REQ_EXCHANGE_ITEM2:
	case HEADER_ZC_REQ_EXCHANGE_ITEM: {
		tradePartner = Str(p, 2, 24);
		tradePartnerId = header == HEADER_ZC_REQ_EXCHANGE_ITEM2 ? U32(p, 26) : 0;
		trade = TRADE_INVITED;
		Event("trade_request").S("from", tradePartner).I("level", header == HEADER_ZC_REQ_EXCHANGE_ITEM2 ? S16(p, 30) : 0).Emit();
		if (autoTrade) {
			PACKET_CZ_ACK_EXCHANGE_ITEM a;
			a.PacketType = HEADER_CZ_ACK_EXCHANGE_ITEM;
			a.result = 3;   // accept
			conn.SendFixed(&a);
		}
		break;
	}
	case HEADER_ZC_ACK_EXCHANGE_ITEM2:
	case HEADER_ZC_ACK_EXCHANGE_ITEM: {
		// 0 too far, 1 no such character, 2 failed, 3 accepted, 4 cancelled, 5 busy
		int result = U8(p, 2);
		Event("trade_response").I("result", result).S("partner", tradePartner).Emit();
		if (result == 3) {
			trade = TRADE_OPEN;
			tradeSelfLocked = tradePartnerLocked = tradeCommitted = tradeOffered = false;
			Event("trade_open").S("partner", tradePartner).Emit();
			SendTradeOffer();
		} else {
			TradeReset();
		}
		break;
	}
	case HEADER_ZC_ADD_EXCHANGE_ITEM:
		// Item id 0 is zeny.
		Event("trade_item").I("item", U16(p, 6)).I("count", S32(p, 2)).S("partner", tradePartner).Emit();
		break;
	case HEADER_ZC_ACK_ADD_EXCHANGE_ITEM:
		// 0 added, 1 too heavy for the partner, 2 no more room
		Event("trade_add_result").I("index", S16(p, 2)).I("result", U8(p, 4)).Emit();
		break;
	case HEADER_ZC_CONCLUDE_EXCHANGE_ITEM:
		OnTradeLocked(U8(p, 2));
		break;
	case HEADER_ZC_CANCEL_EXCHANGE_ITEM:
		Event("trade_cancelled").S("partner", tradePartner).Emit();
		TradeReset();
		break;
	case HEADER_ZC_EXEC_EXCHANGE_ITEM:
		// 0 done, 1 failed
		Event("trade_done").I("result", U8(p, 2)).S("partner", tradePartner).Emit();
		TradeReset();
		break;

	// --- party
	case HEADER_ZC_ACK_MAKE_GROUP:
		// 0 created, 1 name taken, 2 already in a party
		Event("party_created").I("result", U8(p, 2)).Emit();
		break;
	case HEADER_ZC_PARTY_JOIN_REQ: {
		DWORD grid = U32(p, 2);
		std::string party = Str(p, 6, 24);
		Event("party_invite").S("party", party).I("party_id", grid).Emit();
		if (autoParty) {
			PACKET_CZ_PARTY_JOIN_REQ_ACK a;
			memset(&a, 0, sizeof(a));
			a.PacketType = HEADER_CZ_PARTY_JOIN_REQ_ACK;
			a.GRID = grid;
			a.bAccept = 1;
			conn.SendFixed(&a);
		}
		break;
	}
	case HEADER_ZC_PARTY_JOIN_REQ_ACK:
		// 0 already in a party, 1 rejected, 2 accepted, 3 full, 7 offline ...
		Event("party_invite_result").S("name", Str(p, 2, 24)).I("answer", S32(p, 26)).Emit();
		break;
	case HEADER_ZC_GROUP_LIST: {
		partyName = Str(p, 4, 24);
		members.clear();
		std::string m = "[";
		for (int o2 = 28; o2 + 46 <= size; o2 += 46) {
			PartyMember pm;
			pm.aid = U32(p, o2);
			pm.name = Str(p, o2 + 4, 24);
			pm.map = Str(p, o2 + 28, 16);
			pm.leader = U8(p, o2 + 44) == 0;
			pm.online = U8(p, o2 + 45) == 0;
			members.push_back(pm);
			m += std::string(members.size() > 1 ? "," : "") + bf_bot_json_string(pm.name);
		}
		Event("party_list").S("party", partyName).Raw("members", m + "]").Emit();
		break;
	}
	case HEADER_ZC_ADD_MEMBER_TO_GROUP2:
	case HEADER_ZC_ADD_MEMBER_TO_GROUP: {
		DWORD maid = U32(p, 2);
		std::string name = Str(p, 39, 24);
		partyName = Str(p, 15, 24);
		bool known = false;
		for (size_t i = 0; i < members.size(); ++i)
			if (members[i].aid == maid) {
				known = true;
				members[i].online = U8(p, 14) == 0;
			}
		if (!known) {
			PartyMember pm;
			pm.aid = maid;
			pm.name = name;
			pm.map = Str(p, 63, 16);
			pm.leader = U32(p, 6) == 0;
			pm.online = U8(p, 14) == 0;
			members.push_back(pm);
			Event("party_member").S("party", partyName).S("name", name).I("aid", maid).Emit();
		}
		break;
	}
	case HEADER_ZC_DELETE_MEMBER_FROM_GROUP: {
		DWORD maid = U32(p, 2);
		std::string name = Str(p, 6, 24);
		Event("party_left").S("party", partyName).S("name", name).I("result", U8(p, 30)).Emit();
		if (maid == aid) {
			partyName.clear();
			members.clear();
		} else {
			for (size_t i = 0; i < members.size(); ++i)
				if (members[i].aid == maid) {
					members.erase(members.begin() + i);
					break;
				}
		}
		break;
	}

	// --- guild
	case HEADER_ZC_UPDATE_GDID:
		guildId = U32(p, 2);
		guildMaster = U8(p, 14) != 0;
		guildName = Str(p, 19, 24);
		Event("guild").I("guild_id", guildId).S("guild", guildName).B("master", guildMaster).Emit();
		break;
	case HEADER_ZC_CHANGE_GUILD:
		// Sent to the area when a character joins a guild; rAthena sends it,
		// not ZC_UPDATE_GDID, to the one who has just accepted an invitation.
		if (U32(p, 2) == aid || U32(p, 2) == gid) {
			guildId = U32(p, 6);
			guildMaster = false;
			Event("guild").I("guild_id", guildId).S("guild", guildName).B("master", false).B("joined", true).Emit();
		}
		break;
	case HEADER_ZC_GUILD_INFO2:
		if (U32(p, 2) == guildId)
			guildName = Str(p, 46, 24);
		break;
	case HEADER_ZC_RESULT_MAKE_GUILD:
		// 0 created, 1 already in a guild, 2 name taken, 3 no Emperium
		Event("guild_created").I("result", U8(p, 2)).Emit();
		break;
	case HEADER_ZC_REQ_JOIN_GUILD: {
		DWORD gdid = U32(p, 2);
		Event("guild_invite").S("guild", Str(p, 6, 24)).I("guild_id", gdid).Emit();
		if (autoGuild) {
			PACKET_CZ_JOIN_GUILD a;
			memset(&a, 0, sizeof(a));
			a.PacketType = HEADER_CZ_JOIN_GUILD;
			a.GDID = gdid;
			a.answer = 1;
			conn.SendFixed(&a);
		}
		break;
	}
	case HEADER_ZC_ACK_REQ_JOIN_GUILD:
		// 0 already in a guild, 1 rejected, 2 accepted, 3 full
		Event("guild_invite_result").I("answer", U8(p, 2)).Emit();
		break;
	case HEADER_ZC_ACK_LEAVE_GUILD:
		Event("guild_left").S("name", Str(p, 2, 24)).S("reason", Str(p, 26, 40)).Emit();
		if (Str(p, 2, 24) == charName) {
			guildId = 0;
			guildName.clear();
		}
		break;

	// --- inventory (rAthena at this version: 22-byte normal entries,
	// 26-byte equipment entries)
	case HEADER_ZC_NORMAL_ITEMLIST3:
	case HEADER_ZC_EQUIPMENT_ITEMLIST3: {
		bool equip = header == HEADER_ZC_EQUIPMENT_ITEMLIST3;
		int entry = equip ? 26 : 22;
		if ((size - 4) % entry)
			Event("warning").S("packet", Name(header)).S("message", "item entries do not divide the packet").I("size", size).Emit();
		for (int o2 = 4; o2 + entry <= size; o2 += entry) {
			Item it;
			it.index = S16(p, o2);
			it.itid = U16(p, o2 + 2);
			it.count = equip ? 1 : S16(p, o2 + 6);
			it.equip = equip;
			items[it.index] = it;
		}
		break;
	}
	case HEADER_ZC_ITEM_PICKUP_ACK3:
	case HEADER_ZC_ITEM_PICKUP_ACK2:
		if (U8(p, 22) == 0) {   // result
			int index = S16(p, 2);
			Item &it = items[index];
			it.index = index;
			it.itid = U16(p, 6);
			it.count += U16(p, 4);
			it.equip = U16(p, 19) != 0;   // equip location
			Event("item_added").I("index", index).I("item", it.itid).I("count", U16(p, 4)).Emit();
		}
		break;
	case HEADER_ZC_ITEM_THROW_ACK: {
		int index = S16(p, 2);
		if (items.count(index)) {
			items[index].count -= S16(p, 4);
			if (items[index].count <= 0)
				items.erase(index);
		}
		break;
	}

	case HEADER_ZC_ACK_TOUSESKILL:
		// Also how the server refuses an action the character's Basic Skill
		// level does not allow yet (skill 1: trade needs level 1, creating a
		// party level 7).
		Event("skill_failed").I("skill", U16(p, 2)).I("cause", U8(p, size - 1)).Emit();
		break;

	case HEADER_ZC_ACK_REQ_DISCONNECT:
		Event("quit_ack").I("result", S16(p, 2)).Emit();
		break;

	default:
		break;
	}
}

// ------------------------------------------------------------------ commands

void Bot::Reply(const BfBotFields &f, bool ok, const std::string &error, const std::string &extraJson)
{
	BfBotFields::const_iterator cmd = f.find("cmd"), id = f.find("id");
	Event e("reply");
	e.S("cmd", cmd == f.end() ? "" : cmd->second);
	if (id != f.end())
		e.S("id", id->second);
	e.B("ok", ok);
	if (!error.empty())
		e.S("error", error);
	if (!extraJson.empty())
		e.Raw("result", extraJson);
	e.Emit();
}

bool ParseBool(const std::string &v, bool *out)
{
	if (v == "1" || v == "on" || v == "true" || v == "yes") { *out = true; return true; }
	if (v == "0" || v == "off" || v == "false" || v == "no") { *out = false; return true; }
	return false;
}

bool Bot::SetPolicy(const std::string &key, const std::string &value, std::string *error)
{
	bool *flag = NULL;
	if (key == "auto_trade") flag = &autoTrade;
	else if (key == "auto_trade_confirm") flag = &autoTradeConfirm;
	else if (key == "auto_party") flag = &autoParty;
	else if (key == "auto_guild") flag = &autoGuild;
	else if (key == "echo_whisper") flag = &echoWhisper;
	else if (key == "echo_chat") flag = &echoChat;
	if (flag) {
		if (!ParseBool(value, flag)) {
			*error = "expected on or off";
			return false;
		}
		return true;
	}
	if (key == "echo_prefix") { echoPrefix = value; return true; }
	if (key == "trade_offer") { tradeOffer = value; return true; }
	*error = "unknown setting " + key;
	return false;
}

void Bot::Command(const std::string &line)
{
	BfBotFields f;
	std::string error;
	if (!bf_bot_parse_command(line, &f, &error)) {
		Event("reply").S("line", line).B("ok", false).S("error", error).Emit();
		return;
	}
	const std::string cmd = f["cmd"];
	std::string name = f.count("name") ? f["name"] : (f.count("to") ? f["to"] : "");
	std::string text = f["text"];

	if (cmd == "state") {
		bf_bot_io_emit(StateJson());
		Reply(f, true);
	} else if (cmd == "quit") {
		Reply(f, true);
		Quit();
	} else if (!inZone) {
		Reply(f, false, "not on a map");
	} else if (cmd == "say") {
		SendChat(text);
		Reply(f, true);
	} else if (cmd == "gm") {
		// A game-master command is typed into public chat; the server runs it
		// for a GM account and answers with notices.
		if (text.empty() || (text[0] != '@' && text[0] != '#')) {
			Reply(f, false, "a GM command starts with @ or #");
			return;
		}
		SendChat(text);
		Reply(f, true);
	} else if (cmd == "whisper") {
		if (name.empty() || text.empty()) {
			Reply(f, false, "usage: whisper <name> <text>");
			return;
		}
		SendWhisper(name, text);
		Reply(f, true);
	} else if (cmd == "party_say") {
		char p[300];
		std::string l = charName + " : " + text;
		l.resize(l.size() > 200 ? 200 : l.size());
		int sz = 4 + (int)l.size() + 1;
		PutU16(p, 0, HEADER_CZ_REQUEST_CHAT_PARTY);
		PutU16(p, 2, sz);
		memcpy(p + 4, l.c_str(), l.size() + 1);
		conn.SendRaw(p, sz);
		Reply(f, true);
	} else if (cmd == "guild_say") {
		if (!guildId) {
			Reply(f, false, "not in a guild");
			return;
		}
		char p[300];
		std::string l = charName + " : " + text;
		l.resize(l.size() > 200 ? 200 : l.size());
		int sz = 4 + (int)l.size() + 1;
		PutU16(p, 0, HEADER_CZ_GUILD_CHAT);
		PutU16(p, 2, sz);
		memcpy(p + 4, l.c_str(), l.size() + 1);
		conn.SendRaw(p, sz);
		Reply(f, true);
	} else if (cmd == "emotion") {
		std::string v = f.count("type") ? f["type"] : text;
		if (v.empty() || strspn(v.c_str(), "0123456789") != v.size()) {
			Reply(f, false, "usage: emotion <type>");
			return;
		}
		PACKET_CZ_REQ_EMOTION p;
		p.PacketType = HEADER_CZ_REQ_EMOTION;
		p.type = (BYTE)atoi(v.c_str());
		conn.SendFixed(&p);
		Reply(f, true);
	} else if (cmd == "move") {
		int tx = atoi(f["x"].c_str()), ty = atoi(f["y"].c_str());
		if (f["x"].empty() || f["y"].empty()) {
			Reply(f, false, "usage: move <x> <y>");
			return;
		}
		SendMove(tx, ty);
		Reply(f, true);
	} else if (cmd == "create_party") {
		if (name.empty() || name.size() >= NAME_LENGTH) {
			Reply(f, false, "a party name of 1 to 23 characters");
			return;
		}
		PACKET_CZ_MAKE_GROUP2 p;
		memset(&p, 0, sizeof(p));
		p.PacketType = HEADER_CZ_MAKE_GROUP2;
		strncpy(p.groupName, name.c_str(), NAME_LENGTH - 1);
		p.ItemPickupRule = 0;
		p.ItemDivisionRule = 0;
		conn.SendFixed(&p);
		Reply(f, true);
	} else if (cmd == "invite_party") {
		if (name.empty()) {
			Reply(f, false, "usage: invite_party <name>");
			return;
		}
		PACKET_CZ_PARTY_JOIN_REQ p;
		memset(&p, 0, sizeof(p));
		p.PacketType = HEADER_CZ_PARTY_JOIN_REQ;
		strncpy(p.characterName, name.c_str(), NAME_LENGTH - 1);
		conn.SendFixed(&p);
		Reply(f, true);
	} else if (cmd == "leave_party") {
		PACKET_CZ_REQ_LEAVE_GROUP p;
		p.PacketType = HEADER_CZ_REQ_LEAVE_GROUP;
		conn.SendFixed(&p);
		Reply(f, true);
	} else if (cmd == "invite_guild") {
		DWORD target = f.count("aid") ? (DWORD)strtoul(f["aid"].c_str(), NULL, 10) : 0;
		if (!target) {
			Actor *a = ActorByName(name);
			if (!a) {
				Reply(f, false, "no player of that name in sight; give \"aid\"");
				return;
			}
			target = a->id;
		}
		PACKET_CZ_REQ_JOIN_GUILD p;
		memset(&p, 0, sizeof(p));
		p.PacketType = HEADER_CZ_REQ_JOIN_GUILD;
		p.AID = target;
		p.MyAID = aid;
		p.MyGID = gid;
		conn.SendFixed(&p);
		Reply(f, true);
	} else if (cmd == "leave_guild") {
		if (!guildId) {
			Reply(f, false, "not in a guild");
			return;
		}
		PACKET_CZ_REQ_LEAVE_GUILD p;
		memset(&p, 0, sizeof(p));
		p.PacketType = HEADER_CZ_REQ_LEAVE_GUILD;
		p.GDID = guildId;
		p.AID = aid;
		p.GID = gid;
		strncpy(p.reasonDesc, "bifrost-bot", sizeof(p.reasonDesc) - 1);
		conn.SendFixed(&p);
		Reply(f, true);
	} else if (cmd == "request_trade") {
		Actor *a = ActorByName(name);
		if (!a) {
			Reply(f, false, "no player of that name in sight");
			return;
		}
		tradePartner = name;
		tradePartnerId = a->id;
		trade = TRADE_ASKED;
		PACKET_CZ_REQ_EXCHANGE_ITEM p;
		p.PacketType = HEADER_CZ_REQ_EXCHANGE_ITEM;
		p.AID = a->id;
		conn.SendFixed(&p);
		Reply(f, true);
	} else if (cmd == "trade_accept" || cmd == "trade_decline") {
		PACKET_CZ_ACK_EXCHANGE_ITEM a;
		a.PacketType = HEADER_CZ_ACK_EXCHANGE_ITEM;
		a.result = cmd == "trade_accept" ? 3 : 4;
		conn.SendFixed(&a);
		Reply(f, true);
	} else if (cmd == "trade_add") {
		std::string saved = tradeOffer;
		tradeOffer = f["kind"] + " " + f["amount"] + " " + (f.count("count") ? f["count"] : "1");
		tradeOffered = false;
		SendTradeOffer();
		tradeOffer = saved;
		Reply(f, true);
	} else if (cmd == "trade_ok") {
		PACKET_CZ_CONCLUDE_EXCHANGE_ITEM p;
		p.PacketType = HEADER_CZ_CONCLUDE_EXCHANGE_ITEM;
		conn.SendFixed(&p);
		Reply(f, true);
	} else if (cmd == "trade_commit") {
		tradeCommitted = true;
		PACKET_CZ_EXEC_EXCHANGE_ITEM p;
		p.PacketType = HEADER_CZ_EXEC_EXCHANGE_ITEM;
		conn.SendFixed(&p);
		Reply(f, true);
	} else if (cmd == "trade_cancel") {
		PACKET_CZ_CANCEL_EXCHANGE_ITEM p;
		p.PacketType = HEADER_CZ_CANCEL_EXCHANGE_ITEM;
		conn.SendFixed(&p);
		Reply(f, true);
	} else if (cmd == "set") {
		if (!SetPolicy(f["key"], f["value"], &error))
			Reply(f, false, error);
		else
			Reply(f, true);
	} else if (cmd == "raw") {
		// Hex bytes sent as they are, for probing the server.
		std::string hex, bytes;
		for (size_t i = 0; i < f["hex"].size(); ++i)
			if (isxdigit((unsigned char)f["hex"][i]))
				hex.push_back(f["hex"][i]);
		for (size_t i = 0; i + 1 < hex.size(); i += 2)
			bytes.push_back((char)strtoul(hex.substr(i, 2).c_str(), NULL, 16));
		if (bytes.size() < 2) {
			Reply(f, false, "usage: raw <hex bytes>");
			return;
		}
		conn.SendRaw(bytes.data(), (int)bytes.size());
		Reply(f, true);
	} else {
		Reply(f, false, "unknown command " + cmd);
	}
}

// ------------------------------------------------------------------ main loop

int Bot::Run()
{
	for (size_t i = 0; i < o.sets.size(); ++i) {
		size_t eq = o.sets[i].find('=');
		std::string error;
		if (eq == std::string::npos || !SetPolicy(o.sets[i].substr(0, eq), o.sets[i].substr(eq + 1), &error)) {
			fprintf(stderr, "bifrost-bot: bad --set %s %s\n", o.sets[i].c_str(), error.c_str());
			return 2;
		}
	}

	// Length differences between the client's table and the server.
	for (size_t i = 0; i < sizeof(kLengthFixes) / sizeof(kLengthFixes[0]); ++i) {
		int h = kLengthFixes[i].header;
		int client = g_packetLenMap.count(h) ? g_packetLenMap[h] : 0;
		if (client != kLengthFixes[i].serverLength) {
			Event("length_mismatch").S("header", HeaderText(h)).I("client", client).I("server", kLengthFixes[i].serverLength)
				.S("why", kLengthFixes[i].why).Emit();
			g_packetLenMap[h] = kLengthFixes[i].serverLength;
		}
	}

	if (!Login())
		return 1;

	while (running) {
		std::vector<std::string> lines;
		bf_bot_io_poll(4, &lines);
		for (size_t i = 0; i < lines.size() && running; ++i)
			Command(lines[i]);
		if (!running)
			break;
		if (o.useStdin && o.control.empty() && bf_bot_io_stdin_closed()) {
			Quit();
			break;
		}

		bool alive = conn.Poll();
		for (;;) {
			int header = 0, size = 0;
			BotConnection::Result r = conn.Next(buf, sizeof(buf), &header, &size);
			if (r == BotConnection::NOTHING)
				break;
			if (r == BotConnection::PACKET) {
				Dispatch(buf, size);
				lastHeader = header;
				continue;
			}
			// A packet that cannot be framed: report it with the packet before
			// it (whose length may be the real fault), drop what is queued and
			// carry on from the next segment.
			char raw[48];
			int n = conn.PeekRaw(raw, sizeof(raw));
			++unknownHeaders[header];
			bf_count("bot.unframed_packets", 1);
			Event("unknown_packet").S("header", HeaderText(header))
				.S("reason", r == BotConnection::UNKNOWN_HEADER ? "header not in the client's length table" : "length field out of range")
				.I("length", r == BotConnection::BAD_LENGTH ? size : -1)
				.S("previous", HeaderText(lastHeader)).S("previous_name", Name(lastHeader))
				.S("bytes", Hex(raw, n)).I("dropped", conn.Drop()).Emit();
			break;
		}
		if (!alive || !conn.IsConnect()) {
			Event("disconnected").S("name", charName).Emit();
			exitCode = running ? 3 : exitCode;
			running = false;
			break;
		}
		Tick(false);
		UpdateMovement();
	}

	std::string unknown = "[";
	for (std::map<int, int>::iterator it = unknownHeaders.begin(); it != unknownHeaders.end(); ++it)
		unknown += (it == unknownHeaders.begin() ? "" : ",") + bf_bot_json_string(HeaderText(it->first));
	Event("exit").S("name", charName).I("packets", packetsIn).Raw("unknown_headers", unknown + "]").I("code", exitCode).Emit();
	return exitCode;
}

} // namespace

int main(int argc, char **argv)
{
	Options o;
	o.host = "127.0.0.1";
	o.port = 6900;
	o.user = getenv("BF_BOT_USER") ? getenv("BF_BOT_USER") : "";
	o.pass = getenv("BF_BOT_PASS") ? getenv("BF_BOT_PASS") : "";
	o.slot = -1;
	o.hair = 1;
	o.loginRetries = 4;
	o.useStdin = true;
	for (int i = 1; i < argc; ++i) {
		std::string key = argv[i];
		if (key == "--no-stdin") { o.useStdin = false; continue; }
		if (i + 1 >= argc) { fprintf(stderr, "bifrost-bot: %s needs a value\n", key.c_str()); return 2; }
		std::string v = argv[++i];
		if (key == "--host") o.host = v;
		else if (key == "--port") o.port = atoi(v.c_str());
		else if (key == "--user") o.user = v;
		else if (key == "--pass") o.pass = v;
		else if (key == "--char-name") o.charName = v;
		else if (key == "--slot") o.slot = atoi(v.c_str());
		else if (key == "--hair") o.hair = atoi(v.c_str());
		else if (key == "--control") o.control = v;
		else if (key == "--login-retries") o.loginRetries = atoi(v.c_str());
		else if (key == "--set") o.sets.push_back(v);
		else { fprintf(stderr, "bifrost-bot: unknown option %s\n", key.c_str()); return 2; }
	}
	if (o.user.empty() || o.pass.empty()) {
		fprintf(stderr, "bifrost-bot: give --user and --pass (or BF_BOT_USER, BF_BOT_PASS)\n");
		return 2;
	}

	bf_trace_init();
	bf_trace_set_packet_namer(bf_packet_name);
	if (!bf_bot_io_open(o.useStdin, o.control.empty() ? NULL : o.control.c_str()))
		return 2;
	if (!CConnection::Startup()) {
		fprintf(stderr, "bifrost-bot: socket startup failed\n");
		return 1;
	}
	Bot *bot = new Bot(o);
	int result = bot->Run();
	delete bot;
	CConnection::Cleanup();
	bf_bot_io_close();
	bf_trace_dump_counters();
	return result;
}
