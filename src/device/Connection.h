#ifndef HIGHPRIEST_DEVICE_CONNECTION_H
#define HIGHPRIEST_DEVICE_CONNECTION_H

#include "PacketQueue.h"
#include <arpa/inet.h>
const	int QUEUE_SIZE = 0xa000;

struct ServerAddress {
	char ip[16];
	int port;
};

struct ServerAddressEx : public ServerAddress {
	DWORD dwIP;
};

class CConnection {
public:
	CConnection();
	virtual ~CConnection();
	bool Connect(const ServerAddress* addr);
	unsigned char IsConnect() { return m_socket != -1; }
	bool Poll();
	void Disconnect();
	void Send(int size, const char* buf);
	int Recv(char* buf, int size, bool mustRecv);
	void ResetQueue();
	void SetBlock(bool bBlock);
	void SetDelay(unsigned int dwTime) { m_dwTime = bf_clock_ticks() + dwTime; }
protected:
	bool OnSend();
	bool OnRecv();
	void OnConnect();
	void OnClose();
public:
	static bool __cdecl Startup();
	static void __cdecl Cleanup();
	bool CheckModulePt();
private:
#if !defined(BF_PORT)
	static WSAData s_wsaData;
	static HINSTANCE__* s_wsmodule;
#endif

	static int (__stdcall* s_wsSend)(unsigned int, const char*, int, int);
	static int (__stdcall* s_wsRecv)(unsigned int, char*, int, int);
protected:
	int m_socket;
	sockaddr_in m_addr;
	unsigned char m_bBlock;
	unsigned int m_dwTime;
	CPacketQueue m_sendQueue;
	CPacketQueue m_recvQueue;
	CPacketQueue m_blockQueue;
#if defined(BF_PORT)
	char m_bfStream[24];	// "zone/3": role and serial for BF_PKT_LOG; named by the first packet sent
	bool m_bfNamed;
#endif
};

class CRagConnection : public CConnection {
public:
	CRagConnection();
	bool SendPacket(int size, const char* buf);
	bool RecvPacket(char* buf);
	int GetPacketSize(int packetType);
	short GetPacketType(const char* buf);
	static CRagConnection& __cdecl instanceR();
protected:
	bool IsFixedPacket(int packetType);
	void InitPacketMap();
};

#define g_connection CRagConnection::instanceR()

#endif
