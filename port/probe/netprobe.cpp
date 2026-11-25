/*
 * bifrost-netprobe: logs in to a Ragnarok server with the game's own connection
 * and packet code (device/Connection.cpp, device/PacketQueue.cpp,
 * packet.cpp), compiled for macOS. It follows the steps of CLoginMode:
 * account server, character server, zone server, and stops once the zone
 * server has sent its first burst of packets.
 *
 * It answers two questions before any of the game's rendering exists: does
 * the networking code work as a native 64-bit build, and does the server
 * speak the packet set this client expects. A packet whose header the
 * client's length table does not know ends the run with a report, because
 * the client could not find the next packet boundary after it either.
 *
 *   bifrost-netprobe [--host 127.0.0.1] [--port 6900] [--user bftest] [--pass bftest]
 *               [--char-name BFProbe] [--listen-ms 2000]
 *
 * BF_TRACE=net,pkt prints every packet; BF_PKT_LOG=file records them.
 */
#include "common.h"
#include "device/Connection.h"
#include "packet.h"
#include "PaddingPacket.h"
#include "bf_trace.h"

#include <map>
#include <string>

extern "C" const char *bf_packet_name(int header);

namespace {

class ProbeConnection : public CRagConnection {
public:
	enum Result { NOTHING, PACKET, UNKNOWN_HEADER };

	// Takes the next complete packet off the receive queue, or reports that
	// its header has no entry in the client's length table.
	Result Next(char *buf, int *header)
	{
		WORD type;
		if (!m_recvQueue.PeekData(2, (char *)&type))
			return NOTHING;
		*header = type;
		if (g_packetLenMap.find(type) == g_packetLenMap.end())
			return UNKNOWN_HEADER;
		return RecvPacket(buf) ? PACKET : NOTHING;
	}

	int Pending() const { return m_recvQueue.GetSize(); }

	int PeekRaw(char *buf, int size) const
	{
		if (size > m_recvQueue.GetSize())
			size = m_recvQueue.GetSize();
		m_recvQueue.PeekData(size, buf);
		return size;
	}
};

struct Options {
	std::string host, user, pass, charName;
	int port, listenMs;
};

const char *Name(int header)
{
	const char *name = bf_packet_name(header);
	return name ? name : "(no name)";
}

int Fail(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "bifrost-netprobe: FAILED: ");
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
	return 1;
}

bool Connect(ProbeConnection &conn, const char *ip, int port)
{
	ServerAddress addr;
	memset(&addr, 0, sizeof(addr));
	strncpy(addr.ip, ip, sizeof(addr.ip) - 1);
	addr.port = port;
	return conn.Connect(&addr);
}

// Polls until a packet arrives. Returns its header, 0 on timeout or a closed
// connection, -1 on a header the client does not know.
int Wait(ProbeConnection &conn, char *buf, int timeoutMs)
{
	DWORD deadline = bf_clock_ticks() + timeoutMs;
	while (bf_clock_ticks() < deadline) {
		bool alive = conn.Poll();
		int header = 0;
		switch (conn.Next(buf, &header)) {
		case ProbeConnection::PACKET:
			return header;
		case ProbeConnection::UNKNOWN_HEADER: {
			char raw[32];
			int size = conn.PeekRaw(raw, sizeof(raw));
			fprintf(stderr, "bifrost-netprobe: the server sent header 0x%04x, which the client's length table does not contain\n", header);
			bf_trace_hex(BFT_ERR, "bytes at the unknown header", raw, size);
			return -1;
		}
		case ProbeConnection::NOTHING:
			break;
		}
		if (!alive)
			return 0;
		usleep(2000);
	}
	return 0;
}

// Waits for one of two headers and passes over any other packet, as the
// game's dispatch does with packets it has no use for in that state.
int WaitFor(ProbeConnection &conn, char *buf, int timeoutMs, int first, int second)
{
	DWORD deadline = bf_clock_ticks() + timeoutMs;
	for (;;) {
		int remaining = (int)(deadline - bf_clock_ticks());
		int header = remaining > 0 ? Wait(conn, buf, remaining) : 0;
		if (header <= 0 || header == first || header == second)
			return header;
		BF_TRACE(BFT_NET, "passing over 0x%04x %s", header, Name(header));
	}
}

// The character and zone server answer a connection with four bare bytes,
// the account id, ahead of the first packet.
bool WaitRaw(ProbeConnection &conn, char *buf, int size, int timeoutMs)
{
	DWORD deadline = bf_clock_ticks() + timeoutMs;
	while (bf_clock_ticks() < deadline) {
		bool alive = conn.Poll();
		if (conn.Recv(buf, size, true))
			return true;
		if (!alive)
			return false;
		usleep(2000);
	}
	return false;
}

std::string Ip(DWORD address)
{
	in_addr in;
	in.s_addr = address;
	return inet_ntoa(in);
}

int Run(const Options &o)
{
	char buf[20480];
	ProbeConnection conn;

	// Account server.
	if (!Connect(conn, o.host.c_str(), o.port))
		return Fail("cannot connect to the account server %s:%d", o.host.c_str(), o.port);

	PACKET_CA_LOGIN login;
	memset(&login, 0, sizeof(login));
	login.PacketType = HEADER_CA_LOGIN;
	login.Version = 20;
	strncpy((char *)login.ID, o.user.c_str(), sizeof(login.ID) - 1);
	strncpy((char *)login.Passwd, o.pass.c_str(), sizeof(login.Passwd) - 1);
	login.clienttype = 0;
	conn.SendPacket(conn.GetPacketSize(login.PacketType), (char *)&login);

	int header = Wait(conn, buf, 5000);
	if (header == HEADER_AC_REFUSE_LOGIN)
		return Fail("the account server refused the login, error code %d", ((PACKET_AC_REFUSE_LOGIN *)buf)->ErrorCode);
	if (header != HEADER_AC_ACCEPT_LOGIN)
		return Fail("expected AC_ACCEPT_LOGIN, got 0x%04x %s", header, Name(header));

	PACKET_AC_ACCEPT_LOGIN accept = *(PACKET_AC_ACCEPT_LOGIN *)buf;
	int servers = (accept.PacketLength - sizeof(accept)) / sizeof(SERVER_ADDR);
	if (servers < 1)
		return Fail("the account server listed no character server");
	SERVER_ADDR charServer = *(SERVER_ADDR *)(buf + sizeof(accept));
	printf("account server: accepted, account %u, %d character server(s), first \"%.20s\" at %s:%d\n",
		(unsigned)accept.AID, servers, charServer.name, Ip(charServer.ip).c_str(), charServer.port);
	conn.Disconnect();

	// Character server.
	std::string charIp = Ip(charServer.ip);
	if (!Connect(conn, charIp.c_str(), charServer.port))
		return Fail("cannot connect to the character server %s:%d", charIp.c_str(), charServer.port);

	PACKET_CH_ENTER enter;
	memset(&enter, 0, sizeof(enter));
	enter.PacketType = HEADER_CH_ENTER;
	enter.AID = accept.AID;
	enter.AuthCode = accept.AuthCode;
	enter.userLevel = accept.userLevel;
	enter.clientType = 0;
	enter.Sex = accept.Sex;
	conn.SendPacket(conn.GetPacketSize(enter.PacketType), (char *)&enter);

	DWORD echoedAid = 0;
	if (!WaitRaw(conn, (char *)&echoedAid, 4, 5000) || echoedAid != accept.AID)
		return Fail("the character server did not echo the account id");

	header = Wait(conn, buf, 5000);
	if (header == HEADER_HC_REFUSE_ENTER)
		return Fail("the character server refused, error code %d", ((PACKET_HC_REFUSE_ENTER *)buf)->ErrorCode);
	if (header != HEADER_HC_ACCEPT_ENTER)
		return Fail("expected HC_ACCEPT_ENTER, got 0x%04x %s", header, Name(header));

	// Header, length and 20 reserved bytes come ahead of the characters.
	int listBytes = *(WORD *)(buf + 2) - 24;
	int characters = listBytes / (int)sizeof(CHARACTER_INFO);
	printf("character server: accepted, %d character(s), %d bytes each", characters, (int)sizeof(CHARACTER_INFO));
	if (listBytes % (int)sizeof(CHARACTER_INFO))
		printf("  ** %d bytes left over: the server's character record has another size **", listBytes % (int)sizeof(CHARACTER_INFO));
	printf("\n");

	int slot = 0;
	if (characters == 0) {
		PACKET_CH_MAKE_CHAR make;
		memset(&make, 0, sizeof(make));
		make.PacketType = HEADER_CH_MAKE_CHAR;
		strncpy((char *)make.name, o.charName.c_str(), sizeof(make.name) - 1);
		make.Str = make.Agi = make.Vit = make.Int = make.Dex = make.Luk = 5;
		make.CharNum = 0;
		make.headPal = 0;
		make.head = 1;
		conn.SendPacket(conn.GetPacketSize(make.PacketType), (char *)&make);
		header = WaitFor(conn, buf, 5000, HEADER_HC_ACCEPT_MAKECHAR, HEADER_HC_REFUSE_MAKECHAR);
		if (header == HEADER_HC_REFUSE_MAKECHAR)
			return Fail("the character server refused to create \"%s\", error code %d", o.charName.c_str(), ((PACKET_HC_REFUSE_MAKECHAR *)buf)->ErrorCode);
		if (header != HEADER_HC_ACCEPT_MAKECHAR)
			return Fail("character creation: expected HC_ACCEPT_MAKECHAR, got 0x%04x %s", header, Name(header));
		printf("character server: created \"%.24s\"\n", ((PACKET_HC_ACCEPT_MAKECHAR *)buf)->charinfo.name);
	} else {
		CHARACTER_INFO *first = (CHARACTER_INFO *)(buf + 24);
		slot = first->CharNum;
		printf("character server: using \"%.24s\" (slot %d, base level %d, job %d)\n", first->name, slot, first->level, first->job);
	}

	PACKET_CH_SELECT_CHAR select;
	select.PacketType = HEADER_CH_SELECT_CHAR;
	select.CharNum = slot;
	conn.SendPacket(conn.GetPacketSize(select.PacketType), (char *)&select);
	header = WaitFor(conn, buf, 5000, HEADER_HC_NOTIFY_ZONESVR, HEADER_HC_REFUSE_SELECTCHAR);
	if (header != HEADER_HC_NOTIFY_ZONESVR)
		return Fail("expected HC_NOTIFY_ZONESVR, got 0x%04x %s", header, Name(header));
	PACKET_HC_NOTIFY_ZONESVR zone = *(PACKET_HC_NOTIFY_ZONESVR *)buf;
	printf("character server: zone server %s:%d, map \"%.16s\"\n", Ip(zone.addr.ip).c_str(), zone.addr.port, zone.mapName);
	conn.Disconnect();

	// Zone server.
	std::string zoneIp = Ip(zone.addr.ip);
	if (!Connect(conn, zoneIp.c_str(), zone.addr.port))
		return Fail("cannot connect to the zone server %s:%d", zoneIp.c_str(), zone.addr.port);

	PACKET_CZ_ENTER cz;
	memset(&cz, 0, sizeof(cz));
	cz.PacketType = 0x9b;
	cz.AID = accept.AID;
	cz.GID = zone.GID;
	cz.AuthCode = accept.AuthCode;
	cz.clientTime = bf_clock_ticks();
	cz.Sex = accept.Sex;
	conn.SendPacket(conn.GetPacketSize(cz.PacketType), (char *)&cz);

	bool entered = false;
	std::map<int, int> seen;
	int total = 0;
	DWORD until = bf_clock_ticks() + 5000;
	while (bf_clock_ticks() < until) {
		header = Wait(conn, buf, 250);
		if (header < 0)
			return Fail("the packet stream from the zone server cannot be followed past this point (%d packets read before it)", total);
		if (header == 0) {
			if (!conn.IsConnect())
				break;
			continue;
		}
		++seen[header];
		++total;
		if (header == HEADER_ZC_REFUSE_ENTER)
			return Fail("the zone server refused, error code %d", ((PACKET_ZC_REFUSE_ENTER *)buf)->ErrorCode);
		// ZC_ACCEPT_ENTER2 adds a font field after the same leading fields.
		if ((header == HEADER_ZC_ACCEPT_ENTER || header == HEADER_ZC_ACCEPT_ENTER2) && !entered) {
			entered = true;
			PACKET_ZC_ACCEPT_ENTER *a = (PACKET_ZC_ACCEPT_ENTER *)buf;
			int x = (a->PosDir[0] << 2) | (a->PosDir[1] >> 6);
			int y = ((a->PosDir[1] & 0x3f) << 4) | (a->PosDir[2] >> 4);
			printf("zone server: accepted, position (%d, %d)\n", x, y);
			// What the game sends when the map has loaded.
			PACKET_CZ_NOTIFY_ACTORINIT loaded;
			loaded.PacketType = HEADER_CZ_NOTIFY_ACTORINIT;
			conn.SendPacket(conn.GetPacketSize(loaded.PacketType), (char *)&loaded);
			until = bf_clock_ticks() + o.listenMs;
		}
	}
	if (!entered)
		return Fail("the zone server sent neither ZC_ACCEPT_ENTER nor ZC_ACCEPT_ENTER2 (%d packets read)", total);

	printf("zone server: %d packets of %d kinds in the first %d ms after loading, all with known headers:\n",
		total, (int)seen.size(), o.listenMs);
	for (std::map<int, int>::const_iterator it = seen.begin(); it != seen.end(); ++it)
		printf("  0x%04x %-36s x%d\n", it->first, Name(it->first), it->second);
	if (conn.Pending())
		printf("  %d bytes of an incomplete packet were left in the queue\n", conn.Pending());

	conn.Disconnect();
	printf("bifrost-netprobe: OK\n");
	return 0;
}

} // namespace

int main(int argc, char **argv)
{
	Options o;
	o.host = "127.0.0.1"; o.port = 6900; o.user = "bftest"; o.pass = "bftest";
	o.charName = "BFProbe"; o.listenMs = 2000;
	for (int i = 1; i + 1 < argc; i += 2) {
		std::string key = argv[i];
		if (key == "--host") o.host = argv[i + 1];
		else if (key == "--port") o.port = atoi(argv[i + 1]);
		else if (key == "--user") o.user = argv[i + 1];
		else if (key == "--pass") o.pass = argv[i + 1];
		else if (key == "--char-name") o.charName = argv[i + 1];
		else if (key == "--listen-ms") o.listenMs = atoi(argv[i + 1]);
		else { fprintf(stderr, "unknown option %s\n", key.c_str()); return 2; }
	}

	bf_trace_init();
	bf_trace_set_packet_namer(bf_packet_name);
	if (!CConnection::Startup())
		return Fail("socket startup");
	int result = Run(o);
	CConnection::Cleanup();
	bf_trace_dump_counters();
	return result;
}
