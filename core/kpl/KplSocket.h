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
#ifndef __KPL_SOCKET_H__
#define __KPL_SOCKET_H__

#include "FskErrors.h"

#define kKplSocketLevelSocket 0xFFFF
enum {
	kKplSocketOptionReuseAddr = 1,
	kKplSocketOptionDontRoute,
	kKplSocketOptionBroadcast,
	kKplSocketOptionRcvBuf,
	kKplSocketOptionSndBuf,
	kKplSocketOptionKeepAlive
};

#ifdef __cplusplus
extern "C" {
#endif

KplDeclarePrivateType(KplSocket)

FskErr KplSocketInitialize(void);
FskErr KplSocketTerminate(void);

Boolean KplSocketIsReadable(KplSocket skt);
Boolean KplSocketIsWritable(KplSocket skt);

FskErr KplSocketConnect(KplSocket skt, int toIP, int port);

FskErr KplSocketGetRemoteAddress(KplSocket skt, UInt32 *fromIP, int *fromPort);
FskErr KplSocketGetLocalAddress(KplSocket skt, UInt32 *fromIP, int *fromPort);

FskErr KplSocketMakeNonblocking(KplSocket skt);
FskErr KplSocketMakeBlocking(KplSocket skt);

FskErr KplSocketNewUDP(KplSocket *newSocket);
FskErr KplSocketNewTCP(KplSocket *newSocket, Boolean listener);

FskErr KplSocketClose(KplSocket skt);

FskErr KplSocketRecvTCP(KplSocket skt, void *buf, const int bufSize, int *amt);
FskErr KplSocketSendTCP(KplSocket skt, void *buf, const int bufSize, int *amt);

FskErr KplSocketRecvUDP(KplSocket skt, void *buf, const int bufSize, int *amt, int *fromIP, int *fromPort);
FskErr KplSocketSendUDP(KplSocket skt, void *buf, const int bufSize, int *amt, int toIP, int toPort);

FskErr KplSocketSetOption(KplSocket skt, int sktLevel, int sktOption, int val);

FskErr KplSocketBind(KplSocket skt, int addr, int port);
FskErr KplSocketSetTTL(KplSocket kplSocket, int ttl);

FskErr KplSocketMulticastJoin(KplSocket skt, int multicastAddr, int interfaceAddr, int ttl);
FskErr KplSocketMulticastLoop(KplSocket skt, char val);
FskErr KplSocketMulticastSetOutgoingInterface(KplSocket skt, int interfaceAddr, int ttl);
FskErr KplSocketMulticastAddMembership(KplSocket kplSocket, int multicastAddr, int interfaceAddr);
FskErr KplSocketMulticastDropMembership(KplSocket kplSocket, int multicastAddr, int interfaceAddr);
FskErr KplSocketMulticastSetTTL(KplSocket kplSocket, int ttl);


FskErr KplSocketListen(KplSocket skt);
FskErr KplSocketAcceptConnection(KplSocket listeningSkt, KplSocket *createdSocket, int *addr, int *port);

enum {
	kKplSocketHostEventRead = 0,
	kKplSocketHostEventAccept,
	kKplSocketHostEventWrite,
	kKplSocketHostEventConnect,
	kKplSocketHostEventClose
};
	
void FskKplSocketHostEvent(KplSocket skt, UInt32 eventType);

#ifdef __cplusplus
}
#endif

#endif // __KPL_SOCKET_H__
