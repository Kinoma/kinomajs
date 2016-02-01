/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
#define __FSKNETUTILS_PRIV__
#define __FSKTHREAD_PRIV__
#include "Fsk.h"
#include "FskEndian.h"
#include "FskThread.h"
#include "KplSocket.h"

#include <WinSock2.h>
#include <ws2tcpip.h>

#include <MSWSock.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static FskErr initWinSock(void);
static FskErr sConvertErrorToFskErr(int err);

UINT gAsyncSelectMessage = -1;

#define setupSockAddr(sockaddr, addr, port) \
		memset(&sockaddr, 0, sizeof(sockaddr));	\
		sockaddr.sin_family = AF_INET;		\
		sockaddr.sin_port = htons(port);	\
		sockaddr.sin_addr.s_addr = htonl(addr)

extern HWND KplThreadGetCurrentHWND(void);

#define AsyncSelect(SOCKET) WSAAsyncSelect(SOCKET, (HWND)KplThreadGetCurrentHWND(), gAsyncSelectMessage, FD_CONNECT | FD_ACCEPT | FD_READ | FD_WRITE | FD_CLOSE);

FskErr KplSocketInitialize(void)
{
	FskErr err;
	
	err = initWinSock();
	if (err)
		return sConvertErrorToFskErr(err);
		
	return kFskErrNone;
}

FskErr KplSocketTerminate(void)
{
	WSACleanup();
	return kFskErrNone;
}

FskErr KplSocketConnect(KplSocket kplSocket, int toIP, int port)
{
	int ret, skt = (int)kplSocket;
	struct sockaddr_in	tcp_srv_addr;
	
	setupSockAddr(tcp_srv_addr, toIP, port);
	
	ret = connect(skt, (struct sockaddr *)&tcp_srv_addr, sizeof(tcp_srv_addr));
	if (-1 == ret)
		return sConvertErrorToFskErr(WSAGetLastError());
		
	return kFskErrNone;
}

Boolean KplSocketIsReadable(KplSocket kplSocket)
{
	int		skt = (int)kplSocket;
	fd_set  set;
	struct  timeval tv;
	int     err;
	
	tv.tv_sec = 0; tv.tv_usec = 0;
	FD_ZERO(&set);
	FD_SET(skt, &set);
	err = select(skt+1, &set, NULL, NULL, &tv);
	if ((err > 0) && (FD_ISSET(skt, &set))) {
		return true;
	}
	return false;
}

Boolean KplSocketIsWritable(KplSocket kplSocket)
{
	int		skt = (int)kplSocket;
	fd_set  set;
	struct  timeval tv;
	int     err;

	tv.tv_sec = 0; tv.tv_usec = 0;
	FD_ZERO(&set);
	FD_SET(skt, &set);
	err = select(skt+1, NULL, &set, NULL, &tv);
	if ((err > 0) && (FD_ISSET(skt, &set))) {
		return true;
	}
	return false;
}

FskErr KplSocketGetRemoteAddress(KplSocket kplSocket, UInt32 *fromIP, int *fromPort)
{
	int ret, skt = (int)kplSocket;
	socklen_t addrLen;
	struct sockaddr_in  remAddr;

	addrLen = sizeof(struct sockaddr_in);
	ret = getpeername(skt, (struct sockaddr*)&remAddr, &addrLen);
	if (-1 == ret)
		return sConvertErrorToFskErr(WSAGetLastError());

	*fromIP = ntohl(remAddr.sin_addr.s_addr);
	*fromPort = ntohs(remAddr.sin_port);
	
	return kFskErrNone;
}

FskErr KplSocketGetLocalAddress(KplSocket kplSocket, UInt32 *fromIP, int *fromPort)
{
	int ret, skt = (int)kplSocket;
	socklen_t addrLen = sizeof(struct sockaddr_in);
	struct sockaddr_in  remAddr;

	remAddr.sin_addr.s_addr = INADDR_ANY;
	ret = getsockname(skt, (struct sockaddr*)&remAddr, &addrLen);
	if (ret == -1) {
		return sConvertErrorToFskErr(WSAGetLastError());
	}
	*fromIP = ntohl(remAddr.sin_addr.s_addr);
	*fromPort = ntohs(remAddr.sin_port);
	
	return kFskErrNone;
}

FskErr KplSocketMakeNonblocking(KplSocket kplSocket)
{
	int ret, skt = (int)kplSocket;
	ret = AsyncSelect(skt);
	if (ret == -1) {
		return sConvertErrorToFskErr(WSAGetLastError());
	}
	return kFskErrNone;
}

FskErr KplSocketMakeBlocking(KplSocket kplSocket)
{
	int skt = (int)kplSocket;
	int flag = 0;
	
	if (0 != ioctlsocket(skt->platSkt, FIONBIO, &flag))
		return sConvertErrorToFskErr(WSAGetLastError());

	return kFskErrNone;
}

FskErr KplSocketNewUDP(KplSocket *newSocket)
{
	int ret = socket(PF_INET, SOCK_DGRAM, 0);
	if (-1 == ret) {
		return sConvertErrorToFskErr(WSAGetLastError());
	}
	*newSocket = (KplSocket)ret;
	
	return kFskErrNone;
}

FskErr KplSocketNewTCP(KplSocket *newSocket, Boolean listener)
{
	int ret = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ret <= 0) {
		return sConvertErrorToFskErr(WSAGetLastError());
	}
	*newSocket = (KplSocket)ret;
	
	return kFskErrNone;
}

FskErr KplSocketClose(KplSocket kplSocket)
{
	int skt = (int)kplSocket;
	
	closesocket(skt);
	
	return kFskErrNone;
}

FskErr KplSocketRecvTCP(KplSocket kplSocket, void *buf, const int bufSize, int *amt)
{
	int ret, skt = (int)kplSocket;
	
	ret = recv(skt, buf, bufSize, 0);
	if (0 == ret)
		return kFskErrConnectionClosed;
	if (-1 == ret)
		return sConvertErrorToFskErr(WSAGetLastError());
		
	*amt = ret;
	
	return kFskErrNone;
}

FskErr KplSocketSendTCP(KplSocket kplSocket, void *buf, const int bufSize, int *amt)
{
	int ret, skt = (int)kplSocket;
	
	ret = send(skt, buf, bufSize, 0);
	if (-1 == ret)
		return sConvertErrorToFskErr(WSAGetLastError());
		
	*amt = ret;
	
	return kFskErrNone;
}

FskErr KplSocketRecvUDP(KplSocket kplSocket, void *buf, const int bufSize, int *amt, int *fromIP, int *fromPort)
{
	int ret, skt = (int)kplSocket;
	struct sockaddr_in	sin;
	socklen_t addrLen;
	int	bufLen;

	addrLen = sizeof(sin);
	bufLen = bufSize;
 	memset(&sin, 0, sizeof(sin));
 	
	ret = recvfrom(skt, buf, bufLen, 0, (struct sockaddr*)&sin, &addrLen);
	if (-1 == ret)
		return sConvertErrorToFskErr(WSAGetLastError());
		
	*amt = ret;

	if (fromIP)		*fromIP = ntohl(sin.sin_addr.s_addr);
	if (fromPort)	*fromPort = ntohs(sin.sin_port);

	return kFskErrNone;
}

FskErr KplSocketSendUDP(KplSocket kplSocket, void *buf, const int bufSize, int *amt, int toIP, int toPort)
{
	int ret, skt = (int)kplSocket;
	struct sockaddr_in sin;

	setupSockAddr(sin, toIP, toPort);
	ret = sendto(skt, buf, bufSize, 0, (struct sockaddr*)&sin, sizeof(sin));
	if (-1 == ret)
		return sConvertErrorToFskErr(WSAGetLastError());
		
	return kFskErrNone;
}

FskErr KplSocketSetOption(KplSocket kplSocket, int kplSktLevel, int kplSktOption, int val)
{
	int ret, sktOption, skt = (int)kplSocket;
	
	if (kplSktLevel != kKplSocketLevelSocket)
		return kFskErrUnimplemented;
	
	switch(kplSktOption) {
		case kKplSocketOptionReuseAddr:
			sktOption = SO_REUSEADDR;
			break;
		case kKplSocketOptionDontRoute:
			sktOption = SO_DONTROUTE;
			break;
		case kKplSocketOptionBroadcast:
			sktOption = SO_BROADCAST;
			break;
		case kKplSocketOptionRcvBuf:
			sktOption = SO_RCVBUF;
			break;
		case kKplSocketOptionSndBuf:
			sktOption = SO_SNDBUF;
			break;
		case kKplSocketOptionKeepAlive:
			sktOption = SO_KEEPALIVE;
			break;
		default:
			return kFskErrUnimplemented;
	}
	
	ret = setsockopt(skt, SOL_SOCKET, sktOption, (char*)&val, sizeof(val));
	if (-1 == ret)
		return sConvertErrorToFskErr(WSAGetLastError());
		
	return kFskErrNone;
}

FskErr KplSocketBind(KplSocket kplSocket, int addr, int port)
{
	int ret, skt = (int)kplSocket;
	struct sockaddr_in sin;
	
	setupSockAddr(sin, addr, port);
	
	ret = bind(skt, (struct sockaddr*)&sin, sizeof(sin));
	if (-1 == ret)
		return sConvertErrorToFskErr(WSAGetLastError());
		
	return kFskErrNone;
}

FskErr KplSocketListen(KplSocket kplSocket)
{
	int ret, skt = (int)kplSocket;
	
	ret = listen(skt, 8);
	if (-1 == ret)
		return sConvertErrorToFskErr(WSAGetLastError());
		
	return kFskErrNone;
}

FskErr KplSocketAcceptConnection(KplSocket listeningSkt, KplSocket *createdSocket, int *addr, int *port)
{
	struct sockaddr_in sin;
	socklen_t addrLen = sizeof(sin);
	int skt = (int)listeningSkt, ret;

 	memset(&sin, 0, sizeof(sin));

	ret = accept(skt, (struct sockaddr*)&sin, &addrLen);
	if (ret < 0)
		return kFskErrSocketNotConnected;
		
	*createdSocket = (KplSocket)ret;
	if (addr)
		*addr = ntohl(sin.sin_addr.s_addr);
	if (port)
		*port = ntohs(sin.sin_port);	
		
	return kFskErrNone;
}

FskErr KplSocketMulticastJoin(KplSocket kplSocket, int multicastAddr, int interfaceAddr, int ttl)
{
	int ret, skt = (int)kplSocket;
	struct ip_mreq maddr;
	
	maddr.imr_multiaddr.s_addr = htonl(multicastAddr);
	maddr.imr_interface.s_addr = htonl(interfaceAddr);
	
	ret = setsockopt(skt, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&maddr, sizeof(struct ip_mreq));
	if (ret != -1)
		ret = setsockopt(skt, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl));
	if (ret == 0)
		return kFskErrNone;
		
	return sConvertErrorToFskErr(WSAGetLastError());
}

FskErr KplSocketMulticastLoop(KplSocket kplSocket, char val)
{
	int ret, skt = (int)kplSocket;
	
	ret = setsockopt(skt, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&val, sizeof(val));
	if (ret == 0)
		return kFskErrNone;
		
	return sConvertErrorToFskErr(WSAGetLastError());
}

FskErr KplSocketMulticastSetOutgoingInterface(KplSocket kplSocket, int interfaceAddr, int ttl)
{
	int ret, skt = (int)kplSocket;
	struct sockaddr_in addr;
	int param;
	
 	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = htonl(interfaceAddr);
	ret = setsockopt(skt, IPPROTO_IP, IP_MULTICAST_IF, (char*)&addr, sizeof(addr));
	if (ret == -1)
		return sConvertErrorToFskErr(WSAGetLastError());
	param = ttl;
	ret = setsockopt(skt, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&param, sizeof(param));
	if (ret == -1)
		return sConvertErrorToFskErr(WSAGetLastError());

	return kFskErrNone;
}

FskErr initWinSock(void)
{
	int		err;
	WORD	vers;
	WSADATA winSock;

	vers = MAKEWORD(1,1);
	err = WSAStartup(vers, &winSock);
	if (err) goto bail;

	gAsyncSelectMessage = RegisterWindowMessage("FskNetUtilsMessage");

bail:
	return err;
}

FskErr sConvertErrorToFskErr(int err)
{
	FskErr outErr;
	
	if (0 == err)
		return kFskErrNone;

	switch (err) {
		case WSAENOTSOCK: outErr = kFskErrBadSocket; break;
		case WSAEBADF: outErr = kFskErrBadSocket; break;
		case WSAENOTCONN: outErr = kFskErrSocketNotConnected; break;
		case WSAECONNREFUSED: outErr = kFskErrConnectionRefused; break;
		case WSAEINTR: outErr = kFskErrNoData; break;
		case WSAENOPROTOOPT: outErr = kFskErrParameterError; break;
		case WSAEADDRINUSE: outErr = kFskErrAddressInUse; break;
		case WSAEWOULDBLOCK: outErr = kFskErrNoData; break;
		case WSAECONNABORTED: outErr = kFskErrSocketNotConnected; break;
		case WSAECONNRESET: outErr = kFskErrSocketNotConnected; break;
		default:
			outErr = kFskErrNetworkErr; break;
	}
	
	return outErr;
}
