#include "common.h"

#include "Connection.h"
#include "../packet.h"
#include "../base/Util.h"


#include "../3dlib/DrawUtil.h"
#include "../Session.h"


#if defined(BF_PORT)
#include "bf_replay.h"
extern "C" const char *bf_packet_name(int header);
#define DUMP_ADDRESS(a)    BF_TRACE(BFT_NET, "%s %s:%d", (a) + ((a)[0] == '\n'), inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port))
#define DUMP_DATA(a, b)    bf_count("net.bytes.received", (b))
#define DUMP_PACKET(a,b,c) bf_trace_packet_ex((a)[0], (b), (c), NULL, m_bfStream)
#define DUMP_RAW(a,b,c)    bf_trace_packet_ex((a)[0], (b), (c), "raw", m_bfStream)
// Stands for the socket of a replayed connection (BF_PKT_REPLAY): none is opened.
#define BF_REPLAY_SOCKET   0x7ffffffe
static int s_bfSerial;
#else
#define DUMP_ADDRESS(a)    ((void)0)
#define DUMP_DATA(a, b)    ((void)0)
#define DUMP_PACKET(a,b,c) ((void)0)
#define DUMP_RAW(a,b,c)    ((void)0)
#endif


extern void InitPacketLenWithClient();


CConnection::CConnection()
:	m_socket(INVALID_SOCKET)
,	m_bBlock(false)
,	m_dwTime(0)
{
#if defined(BF_PORT)
	m_bfStream[0] = 0;
	m_bfNamed = false;
#endif
	ResetQueue();
}


CConnection::~CConnection()
{
}

void CConnection::SetBlock(bool bBlock)
{
	m_bBlock = bBlock;

	if (m_bBlock == false && m_blockQueue.GetSize()) {

		int len;

		while (len = m_blockQueue.GetSafeDataLen())
		{
			m_sendQueue.InsertData(len, m_blockQueue.GetDataPtr());
			m_blockQueue.RemoveData(len);
			OnSend();
		}
	}
}

bool CConnection::Connect(const ServerAddress *addr)
{
	sAssert(m_socket == INVALID_SOCKET);

	SetBlock(false);
	ResetQueue();

#if defined(BF_PORT)
	snprintf(m_bfStream, sizeof(m_bfStream), "server/%d", ++s_bfSerial);
	m_bfNamed = false;
	if (bf_replay_active()) {
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin_addr.s_addr = inet_addr(addr->ip);
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(addr->port);
		if (!bf_replay_connect(addr->ip, addr->port))
			return false;
		m_socket = BF_REPLAY_SOCKET;
		DUMP_ADDRESS("\nConnecting (replay)");
		return true;
	}
#endif

	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket != INVALID_SOCKET) {

		unsigned int ul = 1;

		if (ioctlsocket(m_socket, FIONBIO, (unsigned int*)&ul) != SOCKET_ERROR) {

			memset(&m_addr, 0, sizeof(SOCKADDR));
			m_addr.sin_addr.s_addr = inet_addr(addr->ip);
			m_addr.sin_family = AF_INET;
			m_addr.sin_port = htons(addr->port);


			int result = connect(m_socket, (LPSOCKADDR)&m_addr, sizeof(SOCKADDR));

			if (result != SOCKET_ERROR || WSAGetLastError() == WSAEWOULDBLOCK) {

				DUMP_ADDRESS("\nConnecting");

				return true;

			} else {

				Trace("connect error %d", WSAGetLastError());

				Disconnect();

				return false;
			}

		} else {
			ErrorMsg("Failed to setup select mode");
			return false;
		}

	} else {
		return false;
	}
}

bool CConnection::Poll()
{
#if defined(BF_PORT)
	if (m_socket != INVALID_SOCKET && bf_replay_active()) {
		char tempBuf[2048];
		int ret = bf_replay_receive(tempBuf, sizeof(tempBuf));
		if (ret > 0) {
			m_recvQueue.InsertData(ret, tempBuf);
			DUMP_DATA(tempBuf, ret);
		} else if (ret < 0) {
			Disconnect();
			return false;
		}
		return true;
	}
#endif
	if (m_socket != INVALID_SOCKET)
		return OnRecv() && OnSend();
	else
		return true;
}



void CConnection::Disconnect()
{
	if (m_socket != INVALID_SOCKET) {
#if defined(BF_PORT)
		if (bf_replay_active())
			bf_replay_disconnect();
		else
#endif
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		m_sendQueue.Init(QUEUE_SIZE);
		m_blockQueue.Init(QUEUE_SIZE);
		DUMP_ADDRESS("Disconnecting");
	}
}

void CConnection::Send(int size, const char *buf)
{
	if (INVALID_SOCKET != m_socket) {

		if (m_bBlock) {
			m_blockQueue.InsertData(size, buf);
		} else {
			m_sendQueue.InsertData(size, buf);
			OnSend();
		}
	}
}

int CConnection::Recv(char *buf, int size, bool mustRecv)
{
	if (mustRecv) {

		if (m_recvQueue.GetData(size, buf)) {
			DUMP_RAW("R", buf, size);
			return size;
		} else
			return 0;

	} else {

		size = min(size, m_recvQueue.GetSize());

		m_recvQueue.GetData(size, buf);

		return size;
	}
}

void CConnection::ResetQueue()
{
	m_recvQueue.Init(QUEUE_SIZE);
	m_sendQueue.Init(QUEUE_SIZE);
	m_blockQueue.Init(QUEUE_SIZE);
}

bool CConnection::OnSend()
{
#if defined(BF_PORT)
	if (bf_replay_active()) {
		m_sendQueue.RemoveData(m_sendQueue.GetSize());	// compared in SendPacket
		return true;
	}
#endif
	while (m_dwTime < bf_clock_ticks() && m_sendQueue.GetSize()) {
		int ret;
		fd_set fdwrite;
		timeval tval = { 0, 0 };
		FD_ZERO(&fdwrite);
		FD_SET(m_socket, &fdwrite);

		if ((ret = select(0, NULL, &fdwrite, NULL, &tval)) != SOCKET_ERROR) {

			if (ret > 0) {
				int result = s_wsSend(m_socket, m_sendQueue.GetDataPtr(), m_sendQueue.GetSafeDataLen(), 0);

				if (result != SOCKET_ERROR) {
					m_sendQueue.RemoveData(result);

				} else {
					int errorCode = WSAGetLastError();
					if (errorCode != WSAEWOULDBLOCK && errorCode != WSAENOTCONN) {
						Disconnect();
						return false;
					}
				}
			} else {
				break;
			}
		}
	}

	return true;
}

bool CConnection::OnRecv()
{
	char tempBuf[2048];

	int ret;
	fd_set fdread;
	timeval tval = { 0, 0 };
	FD_ZERO(&fdread);
	FD_SET(m_socket, &fdread);
	if ((ret = select(0, &fdread, NULL, NULL, &tval)) != SOCKET_ERROR)
	{

		if (ret > 0)
		{

			ret = s_wsRecv(m_socket, tempBuf, sizeof(tempBuf), 0);
			if (ret != SOCKET_ERROR)
			{

				if (ret > 0)
				{
					m_recvQueue.InsertData(ret, tempBuf);

					DUMP_DATA(tempBuf, ret);
				}
				else
				{
					Disconnect();
					return false;
				}

			}
			else
			{
				int errorCode = WSAGetLastError();
				if (errorCode != WSAEWOULDBLOCK && errorCode != WSAENOTCONN) {
					Disconnect();
					return false;
				}
			}
		}
	}

	return true;
}

void CConnection::OnConnect()
{
	DUMP_ADDRESS("Connected");
}

void CConnection::OnClose()
{
	DUMP_ADDRESS("Closing");
}

WSAData CConnection::s_wsaData;
HINSTANCE__* CConnection::s_wsmodule;
int (__stdcall* CConnection::s_wsSend)(unsigned int, const char*, int, int);
int (__stdcall* CConnection::s_wsRecv)(unsigned int, char*, int, int);

#if defined(BF_PORT)
static int __stdcall PortSend(unsigned int s, const char* buf, int len, int flags) { return (int)send(s, buf, len, flags); }
static int __stdcall PortRecv(unsigned int s, char* buf, int len, int flags) { return (int)recv(s, buf, len, flags); }

bool CConnection::Startup()
{
	WSAStartup(MAKEWORD(1, 1), &s_wsaData);
	s_wsmodule = NULL;
	s_wsSend = PortSend;
	s_wsRecv = PortRecv;
	return true;
}

void CConnection::Cleanup()
{
	WSACleanup();
}

bool CConnection::CheckModulePt()
{
	return true;
}
#else
bool CConnection::Startup()
{
	if (WSAStartup(MAKEWORD(1, 1), &s_wsaData) != 0) {
		ErrorMsg("Failed to load Winsock library!");
		WSACleanup();
		return false;
	}

	s_wsmodule = LoadLibrary("ws2_32.dll");
	s_wsSend = (int(PASCAL FAR*)(SOCKET, const char FAR *, int, int))GetProcAddress(s_wsmodule, "send");
	s_wsRecv = (int(PASCAL FAR*)(SOCKET, char FAR *, int, int))GetProcAddress(s_wsmodule, "recv");

	if (s_wsSend == NULL) {
		s_wsSend = send;
		MessageBox(NULL, "GetProcAddress(\"send\") Failed.", "Module Hooking Error", MB_OK);
	}

	if (s_wsRecv == NULL) {
		s_wsRecv = recv;
		MessageBox(NULL, "GetProcAddress(\"recv\") Failed.", "Module Hooking Error", MB_OK);
	}

	return true;
}

void CConnection::Cleanup()
{
	WSACleanup();
	FreeLibrary(s_wsmodule);
}

bool CConnection::CheckModulePt()
{
	HMODULE hMod = LoadLibrary("ws2_32.dll");

	FARPROC pf1 = GetProcAddress(hMod, "connect");
	FARPROC pf2 = GetProcAddress(s_wsmodule, "connect");

	if (pf1 != pf2)
		return false;

	pf1 = GetProcAddress(hMod, "closesocket");
	pf2 = GetProcAddress(s_wsmodule, "closesocket");

	if (pf1 != pf2)
		return false;

	pf1 = GetProcAddress(hMod, "send");
	pf2 = GetProcAddress(s_wsmodule, "send");

	if (pf1 != pf2)
		return false;

	pf1 = GetProcAddress(hMod, "select");
	pf2 = GetProcAddress(s_wsmodule, "select");

	if (pf1 != pf2)
		return false;

	return true;
}

#endif

CRagConnection::CRagConnection()
{
#if defined(BF_PORT)
	bf_trace_set_packet_namer(bf_packet_name);
#endif
	InitPacketMap();
}




bool CRagConnection::SendPacket(int size, const char *buf)
{
#if defined(BF_PORT)
	if (!m_bfNamed && size >= 2) {
		// The first packet tells which server this is: CA_ account, CH_ char,
		// anything else zone (the zone's entry packet varies with the version).
		const char *name = bf_packet_name(GetPacketType(buf) & 0xffff);
		const char *role = name && !strncmp(name, "CA_", 3) ? "account" : name && !strncmp(name, "CH_", 3) ? "char" : "zone";
		const char *serial = strchr(m_bfStream, '/');
		if (serial) {
			char label[sizeof(m_bfStream)];
			snprintf(label, sizeof(label), "%s%s", role, serial);
			strcpy(m_bfStream, label);
		}
		m_bfNamed = true;
	}
	if (m_socket != INVALID_SOCKET && bf_replay_active())
		bf_replay_sent(buf, size);
#endif
	DUMP_PACKET("S", buf, size);
	Send(size, buf);

	return true;
}

bool CRagConnection::RecvPacket(char *buf)
{
	WORD packetHeader, packetSize;
	DWORD headerWithSize;

	if (m_recvQueue.PeekData(2, (char *)&packetHeader)) {
		if (IsFixedPacket(packetHeader)) {
#if defined(BF_PORT)
			// packet.cpp knows where a received length differs from the
			// table's (the auction answer of M8.4).
			packetSize = max(::GetRecvPacketSize(packetHeader), 2);
#else
			packetSize = GetPacketSize(packetHeader);
#endif
		} else if (m_recvQueue.PeekData(4, (char *)&headerWithSize)) {
			packetSize = HIWORD(headerWithSize);
		} else {
			return false;
		}

		if (m_recvQueue.GetData(packetSize, buf)) {
			DUMP_PACKET("R", buf, packetSize);
			return true;
		}
	}

	return false;
}

int CRagConnection::GetPacketSize(int packetType)
{
#if defined(BF_PORT)
	// The lengths, the SRPACKETR2 pair among them, are packet.cpp's (M10.1).
	return max(::GetPacketSize(packetType), 2);
#else
	switch (packetType) {
		case HEADER_ZC_SRPACKETR2_INIT:
			return sizeof(struct PACKET_ZC_SRPACKETR2_INIT);
		case HEADER_CZ_SRPACKETR2_START:
			return sizeof(struct PACKET_CZ_SRPACKETR2_START);
	}

	return max(g_packetLenMap[packetType], 2);
#endif
}

short CRagConnection::GetPacketType(const char *buf)
{
	return *((WORD*)buf);
}

bool CRagConnection::IsFixedPacket(int packetType)
{
#if defined(BF_PORT)
	return ::IsFixedPacket(packetType);
#else
	if (g_packetLenMap[packetType] == -1)
	{
		return false;
	}
	return true;
#endif
}

void CRagConnection::InitPacketMap()
{
	InitPacketLenWithClient();
}



CRagConnection& CRagConnection::instanceR()
{
	static CRagConnection _instance;
	return _instance;
}
