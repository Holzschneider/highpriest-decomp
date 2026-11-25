// Packet replay: BF_PKT_REPLAY=<log> (port/client/include/bf_replay.h).
//
// Compiled with the client's flags into the client and with the probes'
// into bifrost-netprobe and bifrost-bot, which share device/Connection.cpp.
// It uses the C library only.
#include "bf_replay.h"
#include "bf_trace.h"

#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

// The log path is the user's, not a game-folder path: the C library's fopen,
// not the client's translation (bf_crt.h).
#undef fopen

namespace {

enum State { PENDING, DONE, SKIPPED };

struct Event {
	char dir;            // 'S' or 'R'
	int header;
	std::string name;
	std::vector<unsigned char> bytes;
	int line;            // in the log
	State state;
};

struct Conn {
	std::string label;   // "zone/3"; "zone/?" when the log has no seventh field
	std::string role;
	std::vector<Event> events;
};

int g_loaded = -1;             // -1: not read yet; 0: no BF_PKT_REPLAY; 1: replay
std::string g_path;
std::vector<Conn> g_conns;
size_t g_next = 0;             // the log's next connection for bf_replay_connect
int g_cur = -1;                // the connection in use, -1 none
size_t g_pos = 0;              // its next event
size_t g_offset = 0;           // bytes of the R event at g_pos already handed over
long long g_waitSince = -1;    // when the pending S at g_pos began to be waited for
long long g_endSince = -1;     // when the log's last connection ran out
bool g_endReported = false;
long long g_waitMs = 5000;
long long g_holdMs = 5000;
bool g_compareBytes = false;

long long Now()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

const char *RoleOf(const std::string &name)
{
	if (name.size() < 3 || name[2] != '_')
		return NULL;
	std::string p = name.substr(0, 2);
	if (p == "CA" || p == "AC") return "account";
	if (p == "CH" || p == "HC") return "char";
	if (p == "CZ" || p == "ZC") return "zone";
	return NULL;
}

int Hex(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

// <ms> <S|R> <header> <size> <name> <hex> [<connection>]
bool ParseLine(const char *text, Event *e, std::string *label)
{
	char dir[4], header[16], name[96], connection[64];
	long long ms;
	int size;
	connection[0] = 0;
	int offset = 0;
	if (sscanf(text, "%lld %3s %15s %d %95s %n", &ms, dir, header, &size, name, &offset) < 5 || !offset)
		return false;
	if ((dir[0] != 'S' && dir[0] != 'R') || dir[1] || size < 2)
		return false;
	const char *hex = text + offset;
	size_t hexLength = strcspn(hex, " \t\r\n");
	if (hexLength != (size_t)size * 2)
		return false;
	sscanf(hex + hexLength, "%63s", connection);
	e->dir = dir[0];
	e->name = name;
	e->bytes.resize(size);
	for (int i = 0; i < size; ++i) {
		int hi = Hex(hex[2 * i]), lo = Hex(hex[2 * i + 1]);
		if (hi < 0 || lo < 0)
			return false;
		e->bytes[i] = (unsigned char)(hi << 4 | lo);
	}
	e->header = e->bytes[0] | e->bytes[1] << 8;
	e->state = PENDING;
	*label = connection;
	return true;
}

void Load()
{
	const char *path = getenv("BF_PKT_REPLAY");
	if (!path || !*path) {
		g_loaded = 0;
		return;
	}
	g_loaded = 1;
	g_path = path;
	if (const char *wait = getenv("BF_PKT_REPLAY_WAIT"))
		g_waitMs = atoll(wait);
	if (const char *hold = getenv("BF_PKT_REPLAY_HOLD"))
		g_holdMs = atoll(hold);
	const char *bytes = getenv("BF_PKT_REPLAY_BYTES");
	g_compareBytes = bytes && !strcmp(bytes, "1");

	FILE *file = fopen(path, "r");
	if (!file) {
		bf_tracef(BFT_ERR, "replay: cannot read BF_PKT_REPLAY=%s; every connect fails", path);
		return;
	}
	std::vector<char> line(1 << 20);
	int number = 0, bad = 0, packets = 0;
	while (fgets(&line[0], (int)line.size(), file)) {
		++number;
		if (line[0] == '\n' || line[0] == '#')
			continue;
		Event e;
		std::string label;
		if (!ParseLine(&line[0], &e, &label)) {
			if (++bad <= 5)
				bf_tracef(BFT_ERR, "replay: %s line %d is not a packet line; passed over", path, number);
			continue;
		}
		e.line = number;
		const char *role = RoleOf(e.name);
		bool fresh = false;
		if (!label.empty())
			fresh = g_conns.empty() || g_conns.back().label != label;
		else if (e.dir == 'S' && role && e.name != "raw")
			fresh = g_conns.empty() || g_conns.back().role != role;
		else
			fresh = g_conns.empty();
		if (fresh) {
			Conn c;
			if (!label.empty()) {
				c.label = label;
				c.role = label.substr(0, label.find('/'));
			} else {
				c.role = role ? role : "server";
				c.label = c.role + "/?";
			}
			g_conns.push_back(c);
		}
		g_conns.back().events.push_back(e);
		++packets;
	}
	fclose(file);
	if (bad)
		bf_count("replay.log.bad_lines", bad);

	std::string summary;
	for (size_t i = 0; i < g_conns.size(); ++i) {
		char part[96];
		snprintf(part, sizeof part, "%s%s (%d)", i ? ", " : "", g_conns[i].label.c_str(), (int)g_conns[i].events.size());
		summary += part;
	}
	BF_TRACE(BFT_NET, "replay: BF_PKT_REPLAY=%s: %d packets in %d connection(s): %s; no socket is opened, wait %lld ms, hold %lld ms%s",
	         path, packets, (int)g_conns.size(), summary.c_str(), g_waitMs, g_holdMs, g_compareBytes ? ", bytes compared" : "");
}

Conn *Current()
{
	return g_cur >= 0 ? &g_conns[g_cur] : NULL;
}

void Describe(const Event &e, char *out, size_t size)
{
	snprintf(out, size, "%c 0x%04x %s %d (line %d)", e.dir, e.header, e.name.c_str(), (int)e.bytes.size(), e.line);
}

void Skip(Conn *c, size_t index, const char *why)
{
	Event &e = c->events[index];
	e.state = SKIPPED;
	bf_count("replay.sent.skipped", 1);
	char what[160];
	Describe(e, what, sizeof what);
	BF_TRACE(BFT_NET, "replay: %s: the client did not send %s %s; passed over", c->label.c_str(), what, why);
}

// Moves g_pos past finished events and passes over a sent packet that the
// client has not sent for g_waitMs.
void Settle()
{
	Conn *c = Current();
	if (!c)
		return;
	while (g_pos < c->events.size()) {
		Event &e = c->events[g_pos];
		if (e.state != PENDING) {
			++g_pos;
			g_offset = 0;
			g_waitSince = -1;
			continue;
		}
		if (e.dir != 'S')
			return;
		long long now = Now();
		if (g_waitSince < 0)
			g_waitSince = now;
		if (g_waitMs <= 0 || now - g_waitSince < g_waitMs)
			return;
		char why[64];
		snprintf(why, sizeof why, "within %lld ms", g_waitMs);
		Skip(c, g_pos, why);
	}
}

}  // namespace

extern "C" int bf_replay_active(void)
{
	if (g_loaded < 0)
		Load();
	return g_loaded == 1;
}

extern "C" int bf_replay_connect(const char *ip, int port)
{
	if (!bf_replay_active())
		return 0;
	if (g_cur >= 0)
		bf_replay_disconnect();
	if (g_next >= g_conns.size()) {
		bf_count("replay.connect.refused", 1);
		BF_TRACE(BFT_NET, "replay: the client connects to %s:%d, but the log has no connection left; refused", ip, port);
		return 0;
	}
	g_cur = (int)g_next++;
	g_pos = 0;
	g_offset = 0;
	g_waitSince = -1;
	g_endSince = -1;
	g_endReported = false;
	Conn *c = Current();
	bf_count("replay.connect", 1);
	BF_TRACE(BFT_NET, "replay: %s:%d is the log's connection %s (%d packets from line %d), no socket", ip, port,
	         c->label.c_str(), (int)c->events.size(), c->events.empty() ? 0 : c->events[0].line);
	return 1;
}

extern "C" void bf_replay_disconnect(void)
{
	Conn *c = Current();
	if (!c)
		return;
	int sent = 0, received = 0;
	for (size_t i = 0; i < c->events.size(); ++i)
		if (c->events[i].state == PENDING)
			++(c->events[i].dir == 'S' ? sent : received);
	if (sent || received)
		BF_TRACE(BFT_NET, "replay: the client closes %s with %d received and %d sent packet(s) of the log left", c->label.c_str(),
		         received, sent);
	else
		BF_TRACE(BFT_NET, "replay: the client closes %s; its log is complete", c->label.c_str());
	g_cur = -1;
}

extern "C" void bf_replay_sent(const void *data, int size)
{
	Conn *c = Current();
	if (!c || size < 2)
		return;
	const unsigned char *bytes = static_cast<const unsigned char *>(data);
	int header = bytes[0] | bytes[1] << 8;
	Settle();

	// The pending sent packets up to the next received one.
	size_t match = c->events.size();
	for (size_t i = g_pos; i < c->events.size(); ++i) {
		const Event &e = c->events[i];
		if (e.state != PENDING)
			continue;
		if (e.dir != 'S')
			break;
		if (e.header == header) {
			match = i;
			break;
		}
	}
	if (match == c->events.size()) {
		bf_count("replay.sent.unexpected", 1);
		char expected[160] = "nothing more on this connection";
		for (size_t i = g_pos; i < c->events.size(); ++i)
			if (c->events[i].state == PENDING) {
				Describe(c->events[i], expected, sizeof expected);
				break;
			}
		BF_TRACE(BFT_NET, "replay: %s: the client sent S 0x%04x (%d bytes), which the log does not have here; next in the log: %s",
		         c->label.c_str(), header, size, expected);
		return;
	}
	for (size_t i = g_pos; i < match; ++i)
		if (c->events[i].state == PENDING)
			Skip(c, i, "before the one it sent");
	Event &e = c->events[match];
	e.state = DONE;
	bf_count("replay.sent.matched", 1);
	if ((int)e.bytes.size() != size) {
		bf_count("replay.sent.size_differs", 1);
		BF_TRACE(BFT_NET, "replay: %s: S 0x%04x %s is %d bytes, %d in the log (line %d)", c->label.c_str(), header,
		         e.name.c_str(), size, (int)e.bytes.size(), e.line);
	} else if (g_compareBytes && memcmp(&e.bytes[0], bytes, size)) {
		int first = 0, differ = 0;
		for (int i = size - 1; i >= 0; --i)
			if (e.bytes[i] != bytes[i]) {
				first = i;
				++differ;
			}
		bf_count("replay.sent.bytes_differ", 1);
		BF_TRACE(BFT_NET, "replay: %s: S 0x%04x %s differs from line %d in %d byte(s), first at offset %d", c->label.c_str(),
		         header, e.name.c_str(), e.line, differ, first);
	}
	Settle();
}

extern "C" int bf_replay_receive(char *buf, int size)
{
	Conn *c = Current();
	if (!c)
		return 0;
	Settle();
	int n = 0;
	while (n < size && g_pos < c->events.size()) {
		Event &e = c->events[g_pos];
		if (e.state != PENDING) {
			++g_pos;
			g_offset = 0;
			continue;
		}
		if (e.dir != 'R')
			break;
		size_t take = e.bytes.size() - g_offset;
		if (take > (size_t)(size - n))
			take = size - n;
		memcpy(buf + n, &e.bytes[g_offset], take);
		n += (int)take;
		g_offset += take;
		if (g_offset == e.bytes.size()) {
			e.state = DONE;
			bf_count("replay.received", 1);
			++g_pos;
			g_offset = 0;
			g_waitSince = -1;
		}
	}
	if (n)
		return n;
	Settle();
	if (g_pos < c->events.size() || g_cur + 1 < (int)g_conns.size())
		return 0;   // waiting for the client, or an earlier connection that the client closes itself
	long long now = Now();
	if (g_endSince < 0) {
		g_endSince = now;
		if (g_holdMs < 0)
			BF_TRACE(BFT_NET, "replay: end of the log %s (connection %s); the connection stays open (BF_PKT_REPLAY_HOLD=%lld)",
			         g_path.c_str(), c->label.c_str(), g_holdMs);
		else
			BF_TRACE(BFT_NET, "replay: end of the log %s (connection %s); the server closes the connection in %lld ms",
			         g_path.c_str(), c->label.c_str(), g_holdMs);
	}
	if (g_holdMs < 0 || now - g_endSince < g_holdMs)
		return 0;
	if (!g_endReported) {
		g_endReported = true;
		bf_count("replay.end", 1);
		BF_TRACE(BFT_NET, "replay: end of the log: the server closes %s", c->label.c_str());
	}
	return -1;
}
