/*
 * <winsock.h> for the macOS build: the Winsock 1.1 names the game uses, on
 * BSD sockets. Sockets are plain descriptors; nothing here is emulated
 * except the error numbers of a non-blocking connect.
 */
#ifndef HP_WINSOCK_H
#define HP_WINSOCK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

typedef int SOCKET;
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr *LPSOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct hostent HOSTENT;
typedef struct hostent *LPHOSTENT;

#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)

struct WSAData {
	unsigned short wVersion;
	unsigned short wHighVersion;
	char szDescription[257];
	char szSystemStatus[129];
	unsigned short iMaxSockets;
	unsigned short iMaxUdpDg;
	char *lpVendorInfo;
};
typedef struct WSAData WSADATA;
typedef struct WSAData *LPWSADATA;

#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAENOTCONN    ENOTCONN
#define WSAECONNRESET  ECONNRESET
#define WSAEINPROGRESS EINPROGRESS

static inline int WSAStartup(unsigned short version, WSADATA *data)
{
	memset(data, 0, sizeof(*data));
	data->wVersion = data->wHighVersion = version;
	/* Writing to a socket the server has closed must fail with an error,
	   as it does on Windows, not end the process. */
	signal(SIGPIPE, SIG_IGN);
	return 0;
}
static inline int WSACleanup(void) { return 0; }

/* A non-blocking connect that has started reports EINPROGRESS here and
   WSAEWOULDBLOCK on Windows; the game tests for the latter. */
static inline int WSAGetLastError(void)
{
	return errno == EINPROGRESS ? EWOULDBLOCK : errno;
}

static inline int closesocket(SOCKET s) { return close(s); }

/* Winsock ignores the first argument of select and the game passes 0; BSD
   select examines only descriptors below it. */
static inline int hp_winsock_select(fd_set *readable, fd_set *writable, fd_set *failed, struct timeval *timeout)
{
	return select(FD_SETSIZE, readable, writable, failed, timeout);
}
#define select(count, readable, writable, failed, timeout) \
	hp_winsock_select((readable), (writable), (failed), (timeout))

static inline int ioctlsocket(SOCKET s, unsigned long request, void *argument)
{
	int value = *(unsigned int *)argument != 0;
	return ioctl(s, request, &value);
}

#endif
