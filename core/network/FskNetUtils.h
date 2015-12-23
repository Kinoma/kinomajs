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
#ifndef __NET_UTILS_H__
#define __NET_UTILS_H__

#include "Fsk.h"
#include "FskTime.h"
#include "FskThread.h"
#include "FskResolver.h"
#include "FskMedia.h"		// for property stuff

#if OPEN_SSL
	#include <openssl/ssl.h>
#endif

#ifdef __FSKNETUTILS_PRIV__
	#if TARGET_OS_MAC
			#import <CoreFoundation/CFSocket.h>
			#define FskGetErrno()	errno

	#elif TARGET_OS_WIN32
		#include <iptypes.h>
		#include <iphlpapi.h>

		#define kEAGAINErr		WSAEWOULDBLOCK
		#define FskGetErrno()	WSAGetLastError()


	#elif TARGET_OS_LINUX
		#include <errno.h>
		#include <sys/types.h>
		#include <sys/socket.h>

		#define SOCKET_ERROR	-1
		#define kEAGAINErr		EAGAIN

		#define FskGetErrno()	errno
	#elif TARGET_OS_KPL
		typedef unsigned int	SOCKET;

		#ifndef INADDR_ANY
			#define INADDR_ANY		(unsigned long)0x00000000
		#endif

		#ifndef AF_INET
			#define AF_INET			2
		#endif

		#ifndef SOL_SOCKET
			#define SOL_SOCKET		0xffff
			#define SO_REUSEADDR	0x0004
			#define SO_KEEPALIVE	0x0008
			#define SO_DONTROUTE	0x0010
			#define SO_BROADCAST	0x0020
			#define SO_RCVBUF		0x1002
			#define SO_SNDBUF		0x1001
		#endif
	#else

		#error whats your platform?

	#endif
#endif

#if TARGET_OS_MAC
	#import <CoreFoundation/CoreFoundation.h>
#endif

typedef int FskSocketPlatform;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __FSKNETUTILS_PRIV__
	FskDeclarePrivateType(FskSocket)
	FskDeclarePrivateType(FskNetNotification)
#else
	typedef struct FskSocketRecord *FskSocket;
	typedef struct FskNetNotificationRecord *FskNetNotification;
#endif

typedef struct FskSocketCertificateRecord *FskSocketCertificate;

typedef FskErr (*FskNetSocketCreatedCallback)(FskSocket skt, void *refCon);

typedef void (*FskNetNotificationCallback)(int what, int message, void *refCon);

#ifdef __FSKNETUTILS_PRIV__
	typedef struct FskSocketRecord {
		struct FskSocketRecord *next;
		FskSocketPlatform platSkt;
		Boolean	isSocket;			
		Boolean pendingReadable;
		Boolean pendingWritable;
		Boolean pendingClose;
#if USE_POLL
		Boolean pendingError;		// to match ThreadDataSource
		int		err;				// to match ThreadDataSource
#endif

// -- end of FskThreadDataSourceRecord head
		int		ipaddrLocal;
		int		portLocal;
		int 	ipaddrRemote;
		int		portRemote;

		Boolean nonblocking;
		Boolean listener;

		Boolean owned;		// is the platform socket owned by someone else?

#if TARGET_OS_MAC
		CFSocketRef		cfSkt;
		CFRunLoopSourceRef  cfRunLoopSource;
#endif
		FskResolver	rr;

		Boolean	isSSL;
#if OPEN_SSL
		SSL_CTX	*context;
		SSL		*ssl;
		BIO		*bio;
		struct FskThreadDataHandlerRecord *sslTransactionHandler;
		int		sslState;
#endif
#if CLOSED_SSL
		void	*fssl;
#endif
		FskNetSocketCreatedCallback afterCreate;
		void	*afterCreateRefCon;

		char	*hostname;
		int		lastErr;
		char	*debugName;		// a string representation of the creator

		int		priority;
#if TARGET_OS_ANDROID
		int		isWifi;
#endif
		Boolean		isEof;
		FskThreadDataHandler	handler;

		FskInstrumentedItemDeclaration
	} FskSocketRecord;

	typedef struct FskNetNotificationRecord {
		struct FskNetNotificationRecord	*next;
		void						*refcon;
		int							what;
		FskThread					owner;
		FskNetNotificationCallback	callback;
		SInt16						useCount;
		Boolean						disposed;
	} FskNetNotificationRecord;
#endif

#if SUPPORT_INSTRUMENTATION
enum {
    kFskSocketInstrMsgCreateUDP = kFskInstrumentedItemFirstCustomMessage,
    kFskSocketInstrMsgUDPRecv,
    kFskSocketInstrMsgUDPRecvd,		//  FskSocketInstrTrans
    kFskSocketInstrMsgUDPSend,
    kFskSocketInstrMsgUDPSent,		//	FskSocketInstrTrans
    kFskSocketInstrMsgCreateTCP,
    kFskSocketInstrMsgTCPRecv,
    kFskSocketInstrMsgTCPRecvd,		//	FskSocketInstrTrans
    kFskSocketInstrMsgTCPSend,
    kFskSocketInstrMsgTCPSent,		//	FskSocketInstrTrans
    kFskSocketInstrMsgTCPListen,
    kFskSocketInstrMsgTCPAccepting,
    kFskSocketInstrMsgTCPAccepted,
    kFskSocketInstrMsgTCPConnect,
    kFskSocketInstrMsgTCPBeginConnect,
    kFskSocketInstrMsgTCPConnected,
    kFskSocketInstrMsgTCPConnectedSSL,
    kFskSocketInstrMsgTCPConnectFailed,

    kFskSocketInstrMsgSSLRecvd,		//	FskSocketInstrTrans
    kFskSocketInstrMsgSSLSent,		//	FskSocketInstrTrans
    kFskSocketInstrMsgSSLRecvErr,
    kFskSocketInstrMsgSSLSendErr,
    kFskSocketInstrMsgSSLClosed,
	kFskSocketInstrMsgSSLConnectErr,	// char*
	kFskSocketInstrMsgSSLStateConnect,
	kFskSocketInstrMsgSSLServerCertFailed,
	kFskSocketInstrMsgSSLServerCertSuccess,

    kFskSocketInstrMsgSetOptions,		// FskSocketInstrMisc
    kFskSocketInstrMsgBind,				// FskSocketInstrMisc
    kFskSocketInstrMsgMulticastJoin,	// FskSocketInstrMisc
	kFskSocketInstrMsgSetOutgoingInterface,
    kFskSocketInstrMsgClose,
    kFskSocketInstrMsgNativeErr,				// FskSocketInstrNativeErr
};

typedef struct {
	FskSocket skt;
	int		nativeErr;
	FskErr	convertedErr;
} FskSocketInstrNativeErr;

typedef struct {
	FskSocket skt;
	char	*buf;
	int		amt;
} FskSocketInstrTrans;

typedef struct {
	FskSocket skt;
	// Options
		int sktLevel;
		int sktOption;
		int	val;
	// Bind
		int addr;
		int port;
	// MulticastJoin
		int multicastAddr;
		int interfaceAddr;
		int ttl;
		
} FskSocketInstrMisc;
#endif

#if TARGET_OS_WIN32
	extern UINT gAsyncSelectMessage;
	void win32SocketEvent(UINT wParam, UINT event);
#elif TARGET_OS_MAC
	void macSocketCallbackEnable(int s, Boolean wantsReadable, Boolean wantsWritable);
#endif

#define kFskNetLocalhost	(0x7f000001)


enum {
	kFskNetworkPropertyPriority = 1
};

FskAPI(FskErr) FskNetworkHasProperty(void *setToNull, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);
FskAPI(FskErr) FskNetworkSetProperty(void *setToNull, UInt32 propertyID, FskMediaPropertyValue property);
FskAPI(FskErr) FskNetworkGetProperty(void *setToNull, UInt32 propertyID, FskMediaPropertyValue property);



enum {
	kFskNetSocketLowestPriority		= 0,
	kFskNetSocketLowPriority		= 1024,
	kFskNetSocketMediumPriority		= 2048,
	kFskNetSocketHighPriority		= 3072,
	kFskNetSocketHighestPriority	= 4096
};
#define kFskNetSocketDefaultPriority		kFskNetSocketMediumPriority


enum {
	kFskNetNotificationPriorityChanged = 1,
	kFskNetNotificationNetworkAvailableChanged,
	kFskNetNotificationNetworkInterfaceChanged,
};

FskAPI(FskErr) FskNetNotificationNew(int what, FskNetNotificationCallback callback, void *refcon);
FskAPI(FskErr) FskNetNotificationDispose(FskNetNotificationCallback callback, void *refcon);
#ifdef __FSKNETUTILS_PRIV__
	void FskNetNotificationNotify(int what, int message);
#endif

FskErr FskNetInitialize(void);
FskErr FskNetTerminate(void);
FskAPI(Boolean) FskNetIsLocalAddress(int addr);
FskAPI(Boolean) FskNetIsLocalNetwork(int addr);

FskAPI(FskErr) FskNetSocketNewUDP(FskSocket *newSocket, char *debugName);
FskAPI(FskErr) FskNetSocketNewTCPPrioritized(FskSocket *newSocket, Boolean listener, \
				int priority, char *debugName);
#define FskNetSocketNewTCP(newSocket, listener, debugName)	\
	FskNetSocketNewTCPPrioritized(newSocket, listener, kFskNetSocketDefaultPriority, debugName)

FskAPI(FskErr) FskNetSocketClose(FskSocket skt);

FskAPI(void) FskSocketActivate(FskThreadDataSource source, Boolean activateIt);
FskAPI(FskErr) FskNetSocketMakeNonblocking(FskSocket skt);
FskAPI(FskErr) FskNetSocketMakeBlocking(FskSocket skt);
FskAPI(FskErr) FskNetSocketOptions(FskSocket skt, int sktLevel, int sktOption, int val);
FskAPI(FskErr) FskNetSocketReuseAddress(FskSocket skt);
FskAPI(FskErr) FskNetSocketDontRoute(FskSocket skt);
FskAPI(FskErr) FskNetSocketEnableBroadcast(FskSocket skt);
FskAPI(FskErr) FskNetSocketDisableBroadcast(FskSocket skt);
FskAPI(FskErr) FskNetSocketReceiveBufferSetSize(FskSocket skt, int val);
FskAPI(FskErr) FskNetSocketSendBufferSetSize(FskSocket skt, int val);
FskAPI(FskErr) FskNetSocketSetKeepAlive(FskSocket skt, int val);

FskAPI(FskErr) FskNetSocketBind(FskSocket skt, int addr, int port);
FskAPI(FskErr) FskNetSocketSetTTL(FskSocket skt, int ttl);

FskAPI(Boolean) FskNetSocketIsReadable(FskSocket skt);
FskAPI(Boolean) FskNetSocketIsWritable(FskSocket skt);

FskAPI(FskErr) FskNetSocketRecvTCP(FskSocket skt, void *buf, const int bufSize, int *amt);
FskAPI(FskErr) FskNetSocketRecvRawTCP(FskSocket skt, void *buf, const int bufSize, int *amt);
FskAPI(FskErr) FskNetSocketSendTCP(FskSocket skt, void *buf, const int bufSize, int *amt);
FskAPI(FskErr) FskNetSocketSendRawTCP(FskSocket skt, void *buf, const int bufSize, int *amt);
FskAPI(FskErr) FskNetSocketRecvUDP(FskSocket skt, void *buf, const int bufSize, int *amt, int *fromIP, int *fromPort);
FskAPI(FskErr) FskNetSocketSendUDP(FskSocket skt, void *buf, const int bufSize, int *amt, int toIP, int toPort);
FskAPI(FskErr) FskNetSocketFlushTCP(FskSocket skt);

enum {
	kConnectFlagsSSLConnection	= 0x00000100,
	kConnectFlagsSynchronous	= 0x00000200,
	kConnectFlagsQueryTypeMask	= 0x0000f000,
	kConnectFlagsServerSelection	= 0x00001000,
};


#if OPEN_SSL || CLOSED_SSL
FskAPI(FskErr) FskNetSocketDoSSL(char *host, FskSocket skt, FskNetSocketCreatedCallback callback, void *refCon);
#endif

typedef struct FskSocketCertificateRecord {
	void *certificates;
	int certificatesSize;
	char *policies;
	char *hostname;
	void *key;
	int keySize;
} FskSocketCertificateRecord;

FskAPI(FskErr) FskNetConnectToHostPrioritized(char *host, int port, Boolean blocking,
				FskNetSocketCreatedCallback callback, void *refCon, long flags,
				int priority, FskSocketCertificate cert, char *debugName);
#define FskNetConnectToHost(host, port, blocking, callback, refCon, flags, cert, debugName) \
			FskNetConnectToHostPrioritized(host, port, blocking, callback, refCon, flags, \
					kFskNetSocketDefaultPriority, cert, debugName)


FskAPI(FskErr) FskNetSocketListen(FskSocket skt);
FskAPI(FskErr) FskNetAcceptConnection(FskSocket listeningSkt, FskSocket *createdSocket, char *debugName);

FskAPI(FskErr) FskNetSocketGetLocalAddress(FskSocket skt, UInt32 *fromIP, int *fromPort);
FskAPI(FskErr) FskNetSocketGetRemoteAddress(FskSocket skt, UInt32 *fromIP, int *fromPort);

FskAPI(FskErr) FskNetSocketMulticastJoin(FskSocket skt, int multicastAddr, int interfaceAddr, int ttl);
FskAPI(FskErr) FskNetSocketMulticastLoop(FskSocket skt, char val);
FskAPI(FskErr) FskNetSocketMulticastSetOutgoingInterface(FskSocket skt, int interfaceAddr, int ttl);
FskAPI(FskErr) FskNetSocketMulticastAddMembership(FskSocket skt, int multicastAddr, int interfaceAddr);
FskAPI(FskErr) FskNetSocketMulticastDropMembership(FskSocket skt, int multicastAddr, int interfaceAddr);
FskAPI(FskErr) FskNetSocketMulticastSetTTL(FskSocket skt, int ttl);



FskAPI(void) FskNetCancelConnection(void *connectingRef);

FskAPI(void) FskNetStringToIPandPort(const char *buf, int *IP, int *port);
FskAPI(void) FskNetIPandPortToString(int IP, int port, char *addrString);
FskAPI(FskErr) FskNetSocketGetLastError(FskSocket skt);

FskAPI(FskSocketCertificate) FskNetUtilCopyCertificate(FskSocketCertificate src);
FskAPI(void) FskNetUtilDisposeCertificate(FskSocketCertificate cert);


#if TARGET_OS_KPL
FskSocket FskNetSocketFindFskSocketByPlatformSocket(int skt);
#endif

#define FskNetMakeIP(a, b, c, d)	(((int)a & 0xff) << 24 | ((int)b & 0xff) << 16 | ((int)c & 0xff) << 8 | ((int)d & 0xff))
#define intAs4Bytes(a)	(int)(((UInt32)a & 0xff000000) >> 24), (int)((a & 0xff0000) >> 16), (int)((a & 0xff00) >> 8), (int)(a & 0xff)

#define FskNetEnsureConnection(rr) (kFskErrNone)

#ifdef __cplusplus
}
#endif

#endif // __NET_UTILS_H__

