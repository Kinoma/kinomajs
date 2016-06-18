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

#define INSTR_PACKET_CONTENTS 0

#if TARGET_OS_WIN32
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
#elif TARGET_OS_LINUX || TARGET_OS_MAC
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <errno.h>

	#include <sys/types.h>
	#include <sys/ioctl.h>
	#include <sys/socket.h>

	#include <netinet/in.h>
	#include <netdb.h>
	#include <net/if.h>
	#include <arpa/inet.h>

	#define closesocket		close
#if TARGET_OS_LINUX
	#include <resolv.h>
	#include <net/if_arp.h>
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	#include <CoreFoundation/CoreFoundation.h>
	#include <SystemConfiguration/SystemConfiguration.h>
	#include <net/if_types.h>
	#include <net/if_dl.h>
#endif

#if TARGET_OS_ANDROID
	#include "FskHardware.h"
#endif

#elif TARGET_OS_KPL
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
#endif

#include "FskPlatformImplementation.h"
#include "FskNetUtils.h"
#include "FskResolver.h"
#include "FskNetInterface.h"
#include "FskThread.h"

#include "FskUtilities.h"

#include "FskFiles.h"

#if TARGET_OS_KPL
	#include "KplSocket.h"
#endif

#if OPEN_SSL || CLOSED_SSL
#include "FskSSL.h"
#endif

// ---------------------------------------------------------------------
#if TARGET_OS_WIN32

	static FskErr initWinSock(void);

	UINT gAsyncSelectMessage;

	#define AsyncSelect(SOCKET) WSAAsyncSelect(SOCKET, FskThreadGetCurrent()->window, gAsyncSelectMessage, FD_CONNECT | FD_ACCEPT | FD_READ | FD_WRITE | FD_CLOSE)
	#define AsyncDeselect(SOCKET) WSAAsyncSelect(SOCKET, FskThreadGetCurrent()->window, 0, 0)
#else
	#define AsyncSelect(SOCKET)
	#define AsyncDeselect(SOCKET)
#endif

#define setupSockAddr(sockaddr, addr, port) \
		FskMemSet(&sockaddr, 0, sizeof(sockaddr));	\
		sockaddr.sin_family = AF_INET;		\
		sockaddr.sin_port = htons(port);	\
		sockaddr.sin_addr.s_addr = htonl(addr)

#define KPL_INADDR_ANY		INADDR_ANY

typedef FskErr (*FskNetSocketDoCreateCallback)(FskSocket skt, int param);

static FskListMutex sSocketActiveList = NULL;

static FskListMutex sNetNotifications = NULL;
static int sSocketHighestActivePriority = kFskNetSocketLowestPriority;


extern Boolean gQuitting;

#if TARGET_OS_MAC
static void macCallBack(CFSocketRef s, CFSocketCallBackType cbType, CFDataRef addr, const void *data, void *info);
#endif

#if OPEN_SSL
static void sDoSSLTransaction(FskSocket skt);
enum {
	kSSLStateConnect = 0x13,
	kSSLStateCheckServer,
};
static void sSSLSocketConnected(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);
#endif
static void sSocketConnected(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);


// ---------------------------------------------------------------------
#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageFskSocket(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

static FskInstrumentedTypeRecord gFskSocketTypeInstrumentation = {
    NULL,
    sizeof(FskInstrumentedTypeRecord),
    "socket",
    FskInstrumentationOffset(FskSocketRecord),
    NULL,
    0,
    NULL,
    doFormatMessageFskSocket
};
#endif
// ---------------------------------------------------------------------
static int gFskNetInitialized = 0;

FskErr FskNetInitialize(void) {
	FskErr	err = kFskErrNone;
	if (gFskNetInitialized++ == 0) {

		err = FskListMutexNew(&sNetNotifications, "netNotifications");
		if (err) return err;
		err = FskListMutexNew(&sSocketActiveList, "socketActiveList");
		if (err) return err;
#if TARGET_OS_WIN32
		gAsyncSelectMessage = 0;
		err = initWinSock();
#elif TARGET_OS_KPL
		err = KplSocketInitialize();
#endif
	}

	return err;
}

// ---------------------------------------------------------------------
static void sFskNetNotificationTerminate()
{
	FskNetNotification	notify;

	notify = (FskNetNotification)FskListMutexRemoveFirst(sNetNotifications);
	while (notify) {
		FskMemPtrDispose(notify);
		notify = (FskNetNotification)FskListMutexRemoveFirst(sNetNotifications);
	}
}

// ---------------------------------------------------------------------
static void sFskNetSocketTerminate()
{
	while (true) {
		FskSocket skt = (FskSocket)FskListMutexRemoveLast(sSocketActiveList);
		if (!skt) break;
		FskNetSocketClose(skt);
	}
}

// ---------------------------------------------------------------------
FskErr FskNetTerminate(void) {

	if (--gFskNetInitialized == 0) {
		sFskNetNotificationTerminate();
		sFskNetSocketTerminate();
		FskAsyncResolverTerminate();
#if OPEN_SSL || CLOSED_SSL
		FskSSLTerminate();
#endif

#if TARGET_OS_WIN32
		WSACleanup();
#elif TARGET_OS_KPL
		KplSocketTerminate();
#endif

		FskListMutexDispose(sNetNotifications);
		FskListMutexDispose(sSocketActiveList);
		sSocketActiveList = NULL;
	}
	else if (gFskNetInitialized < 0) {
	}

	return kFskErrNone;
}

// ---------------------------------------------------------------------

#if !TARGET_OS_KPL
static FskErr sConvertErrorToFskErr(FskSocket skt, int err)
{
	FskErr outErr;
	if (0 == err)
		return kFskErrNone;

#if TARGET_OS_WIN32
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
#else
	switch (err) {
		case ENOTSOCK: outErr = kFskErrBadSocket; break;
		case EBADF: outErr = kFskErrBadSocket; break;
		case EOPNOTSUPP: outErr = kFskErrBadSocket; break;
		case ENOTCONN: outErr = kFskErrSocketNotConnected; break;
		case ECONNREFUSED: outErr = kFskErrConnectionRefused; break;
		case ECONNRESET: outErr = kFskErrConnectionClosed; break;
		case EHOSTUNREACH: outErr = kFskErrSocketNotConnected; break;
		case EHOSTDOWN: outErr = kFskErrSocketNotConnected; break;
		case EINPROGRESS: outErr = kFskErrNoData; break;
		case EINTR: outErr = kFskErrNoData; break;
		case EPIPE: outErr = kFskErrConnectionClosed; break;
		case EAGAIN: outErr = kFskErrNoData; break;
		case ENOPROTOOPT: outErr = kFskErrParameterError; break;
		case EINVAL: outErr = kFskErrParameterError; break;
		case EPERM: outErr = kFskErrConnectionRefused; break;
		case EADDRINUSE: outErr = kFskErrAddressInUse; break;
#if TARGET_OS_ANDROID
		case ETIMEDOUT: outErr = kFskErrConnectionClosed; break;
#endif
		default:
				outErr = kFskErrNetworkErr; break;
	}
#endif
#if SUPPORT_INSTRUMENTATION
	if (skt && FskInstrumentedItemHasListeners(skt)) {
		FskSocketInstrNativeErr native;
		native.skt = skt;
		native.nativeErr = err;
		native.convertedErr = outErr;
	   	FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgNativeErr, &native);
	}
#endif
	return outErr;
}
#endif

// ---------------------------------------------------------------------
// prioritization management
FskErr FskNetNotificationNew(int what, FskNetNotificationCallback callback, void *refcon)
{
	FskErr err;
	FskNetNotification notify = NULL;

	err = FskMemPtrNew(sizeof(FskNetNotificationRecord), (FskMemPtr*)(void*)&notify);
	BAIL_IF_ERR(err);

	notify->what = what;
	notify->callback = callback;
	notify->refcon = refcon;
	notify->owner = FskThreadGetCurrent();
	notify->useCount = 1;
	notify->disposed = false;
	FskInstrumentedTypePrintfDebug(&gFskSocketTypeInstrumentation, "adding a notification with callback (%p) and refcon (%p)\n", callback, refcon);

	FskListMutexPrepend(sNetNotifications, notify);

bail:
	if (kFskErrNone != err) {
		FskMemPtrDispose(notify);
	}
	return err;
}

FskErr FskNetNotificationDispose(FskNetNotificationCallback callback, void *refcon)
{
	FskNetNotification notify;

	FskMutexAcquire(sNetNotifications->mutex);

	notify = (FskNetNotification)sNetNotifications->list;
	while (notify) {
		if (notify->callback == callback && notify->refcon == refcon) {
			FskInstrumentedTypePrintfDebug(&gFskSocketTypeInstrumentation, "disposing a notification with callback (%p) and refcon (%p)\n", callback, refcon);
			FskListRemove(&sNetNotifications->list, notify);

			notify->disposed = true;
			notify->useCount -= 1;
			if (notify->useCount <= 0)
				FskMemPtrDispose(notify);
			break;
		}
		notify = notify->next;
	}
	FskMutexRelease(sNetNotifications->mutex);

	return kFskErrNone;
}

static void doNotifyCallback(void *a0, void *a1, void *a2, void *a3);

void FskNetNotificationNotify(int what, int message) {
	FskNetNotification notify;

	FskMutexAcquire(sNetNotifications->mutex);
	notify = (FskNetNotification)sNetNotifications->list;
	while (notify) {
		if ((what == notify->what) && !notify->disposed) {
			FskInstrumentedTypePrintfDebug(&gFskSocketTypeInstrumentation, " - posting callback what(%d) message(%d) to %s\n", what, message, notify->owner->name);
			notify->useCount += 1;
			FskThreadPostCallback(notify->owner, doNotifyCallback, (void *)what, (void *)message, notify, NULL);
		}
		notify = notify->next;
	}
	FskMutexRelease(sNetNotifications->mutex);
}

void doNotifyCallback(void *a0, void *a1, void *a2, void *a3)
{
	FskNetNotification notify = a2;

	if (!notify->disposed)
		(notify->callback)((int)a0, (int)a1, notify->refcon);

	notify->useCount -= 1;
	if (notify->useCount <= 0)
		FskMemPtrDispose(notify);
}

static void sMakeSocketActive(FskSocket skt) {
	FskSocket cur;

	FskMutexAcquire(sSocketActiveList->mutex);
	cur = (FskSocket)sSocketActiveList->list;
	sSocketActiveList->list = skt;
	skt->next = cur;

	if (skt->priority > sSocketHighestActivePriority) {
		FskInstrumentedTypePrintfDebug(&gFskSocketTypeInstrumentation, "new socket %s has raised network priority\n", skt->debugName);
		sSocketHighestActivePriority = skt->priority;
		FskNetNotificationNotify(kFskNetNotificationPriorityChanged, sSocketHighestActivePriority);
	}

	FskMutexRelease(sSocketActiveList->mutex);
}

static void sMakeSocketInactive(FskSocket skt) {
	FskSocket cur;
	int currentHighest = kFskNetSocketLowestPriority;

	FskMutexAcquire(sSocketActiveList->mutex);
	FskListRemove(&sSocketActiveList->list, skt);

	// find the highest remaining priority
	cur = (FskSocket)sSocketActiveList->list;
	while (cur) {
		if (cur->priority > currentHighest)
			currentHighest = cur->priority;
		cur = (FskSocket)cur->next;
	}

	// do we need to notify?
	if (currentHighest != sSocketHighestActivePriority) {
		FskInstrumentedTypePrintfDebug(&gFskSocketTypeInstrumentation, "socket priority lowered when socket %s is disposed\n", skt->debugName);
		sSocketHighestActivePriority = currentHighest;
		FskNetNotificationNotify(kFskNetNotificationPriorityChanged, sSocketHighestActivePriority);
	}

	FskMutexRelease(sSocketActiveList->mutex);
}

// ---------------------------------------------------------------------
Boolean FskNetSocketIsReadable(FskSocket skt)
{
#if !CLOSED_SSL
	if (skt->pendingReadable)
		return true;
#else
	if (skt->fssl != NULL && FskSSLGetBytesAvailable(skt->fssl) > 0) {
		skt->pendingReadable = true;
		return true;
	}
#endif

#if !TARGET_OS_KPL
	{
	fd_set  set;
	struct  timeval tv;
	int     err;
	tv.tv_sec = 0; tv.tv_usec = 0;
	FD_ZERO(&set);
	FD_SET(skt->platSkt, &set);
	err = select(skt->platSkt+1, &set, NULL, NULL, &tv);
	if ((err > 0) && (FD_ISSET(skt->platSkt, &set))) {
		skt->pendingReadable = true;
		return true;
	}
#if CLOSED_SSL
	else
		skt->pendingReadable = false;
#endif
	}
#else
	if (KplSocketIsReadable((KplSocket)skt->platSkt)) {
		skt->pendingReadable = true;
		return true;
	}
#endif
	return false;
}

// ---------------------------------------------------------------------
Boolean FskNetSocketIsWritable(FskSocket skt)
{
	if (skt->pendingWritable)
		return true;

#if !TARGET_OS_KPL
	{
	fd_set  set;
	struct  timeval tv;
	int     err;

	tv.tv_sec = 0; tv.tv_usec = 0;
	FD_ZERO(&set);
	FD_SET(skt->platSkt, &set);
	err = select(skt->platSkt+1, NULL, &set, NULL, &tv);
	if ((err > 0) && (FD_ISSET(skt->platSkt, &set))) {
		skt->pendingWritable = true;
		return true;
	}
	}
#else
	if (KplSocketIsWritable((KplSocket)skt->platSkt)) {
		skt->pendingWritable = true;
		return true;
	}
#endif
	return false;
}

// ---------------------------------------------------------------------
FskErr FskNetSocketGetRemoteAddress(FskSocket skt, UInt32 *fromIP, int *fromPort)
{
	if (skt->ipaddrRemote) {
		if (fromIP)		*fromIP = skt->ipaddrRemote;
		if (fromPort)	*fromPort = skt->portRemote;
		return kFskErrNone;
	}
	else {
		int  err;

#if !TARGET_OS_KPL
		socklen_t addrLen;
		struct sockaddr_in  remAddr;

		addrLen = sizeof(struct sockaddr_in);
		err = getpeername(skt->platSkt, (struct sockaddr*)(void*)&remAddr, &addrLen);
		if (err == -1) {
			return skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
		}
		skt->ipaddrRemote = ntohl(remAddr.sin_addr.s_addr);
		skt->portRemote = ntohs(remAddr.sin_port);
#else
		{
		UInt32 remAddr;
		int remPort;
		err = skt->lastErr = KplSocketGetRemoteAddress((KplSocket)skt->platSkt, &remAddr, &remPort);
		if (0 != err)
			return err;
		skt->ipaddrRemote = remAddr;
		skt->portRemote = remPort;
		}
#endif
	}
	if (fromIP)		*fromIP = skt->ipaddrRemote;
	if (fromPort)	*fromPort = skt->portRemote;

	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskNetSocketGetLocalAddress(FskSocket skt, UInt32 *IP, int *port)
{
	if (skt->ipaddrLocal && skt->portLocal) {
		if (IP)		*IP = skt->ipaddrLocal;
		if (port)	*port = skt->portLocal;
		return kFskErrNone;
	}
	else {
		int  err;

#if !TARGET_OS_KPL
		socklen_t addrLen = sizeof(struct sockaddr_in);
		struct sockaddr_in  remAddr;

		remAddr.sin_addr.s_addr = INADDR_ANY;
		err = getsockname(skt->platSkt, (struct sockaddr*)(void*)&remAddr, &addrLen);
		if (err == -1) {
			return skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
		}

		skt->ipaddrLocal = ntohl(remAddr.sin_addr.s_addr);
		skt->portLocal = ntohs(remAddr.sin_port);
#else
		{
		UInt32 remAddr;
		int remPort;
		err = skt->lastErr = KplSocketGetLocalAddress((KplSocket)skt->platSkt, &remAddr, &remPort);
		if (0 != err)
			return err;
		skt->ipaddrLocal = remAddr;
		skt->portLocal = remPort;
		}
#endif
	}
	if (IP)		*IP = skt->ipaddrLocal;
	if (port)	*port = skt->portLocal;

	return kFskErrNone;
}

// ---------------------------------------------------------------------
void FskSocketActivate(FskThreadDataSource source, Boolean activateIt) {
#if TARGET_OS_WIN32
	if (activateIt) {
		sMakeSocketActive((FskSocket)source);
		AsyncSelect(source->dataNode);
	}
	else {
		AsyncDeselect(source->dataNode);
		sMakeSocketInactive((FskSocket)source);
	}
#endif
}

// ---------------------------------------------------------------------
FskErr FskNetSocketMakeNonblocking(FskSocket skt)
{
	int err;

	if (skt == 0)
		return kFskErrParameterError;
	else {
#if !TARGET_OS_KPL
		unsigned long	flag;
#if TARGET_OS_WIN32
		flag = 1;
		err = AsyncSelect(skt->platSkt);
	//	err = ioctlsocket(skt->platSkt, FIONBIO, &flag);	// AsyncSelect does this for us
#else
		flag = fcntl(skt->platSkt, F_GETFL, 0);
		err = fcntl(skt->platSkt, F_SETFL, flag | O_NONBLOCK);
#endif
		if (err == -1) {
			err = skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
			goto bail;
		}
		err = kFskErrNone;
#else
		err = skt->lastErr = KplSocketMakeNonblocking((KplSocket)skt->platSkt);
		BAIL_IF_ERR(err);
#endif
	}

	skt->nonblocking = true;
bail:
	return err;
}

// ---------------------------------------------------------------------
FskErr FskNetSocketMakeBlocking(FskSocket skt)
{
	int err;

	if (skt == 0)
		return kFskErrParameterError;
	else {
#if !TARGET_OS_KPL
		int flag = 0;
#if TARGET_OS_WIN32
		err = ioctlsocket(skt->platSkt, FIONBIO, &flag);
#else
		err = fcntl(skt->platSkt, F_SETFL, flag);
#endif
		if (err == -1) {
			err = skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
			goto bail;
		}
#else
		err = skt->lastErr = KplSocketMakeBlocking((KplSocket)skt->platSkt);
		BAIL_IF_ERR(err);
#endif
	}

	skt->nonblocking = false;
bail:
	return err;
}

#if OPEN_SSL
// ---------------------------------------------------------------------
FskErr FskNetSocketDoSSL(char *host, FskSocket skt, FskNetSocketCreatedCallback callback, void *refCon)
{
	FskMemPtrDispose(skt->hostname);
	skt->hostname = FskStrDoCopy(host);
	skt->afterCreate = callback;
	skt->afterCreateRefCon = refCon;

	if (!skt->ssl) {
		skt->ssl = SSL_new(FskSSLGetContext());
		skt->bio = BIO_new_socket(skt->platSkt, BIO_NOCLOSE);
		SSL_set_bio(skt->ssl, skt->bio, skt->bio);
		skt->sslState = kSSLStateConnect;
	}
	else
		skt->sslState = kSSLStateCheckServer;

	sDoSSLTransaction(skt);
	return kFskErrNone;
}
#endif

// ---------------------------------------------------------------------
FskErr FskNetSocketNewUDP(FskSocket *newSocket, char *debugName)
{
	FskSocket	skt = NULL;
	int ret;

#if !TARGET_OS_KPL
	ret = socket(PF_INET, SOCK_DGRAM, 0);
	if (-1 == ret) {
		return sConvertErrorToFskErr(skt, FskGetErrno());
	}
#else
	KplSocket kplSocket;
	ret = KplSocketNewUDP(&kplSocket);
	if (0 != ret)
		return ret;
	ret = (int)kplSocket;
#endif
#if TARGET_OS_MAC
    {
        int set = 1;
        setsockopt(ret, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    }
#endif

	FskMemPtrNewClear(sizeof(FskSocketRecord), (FskMemPtr*)(void*)&skt);
	skt->isSocket = true;
	skt->debugName = FskStrDoCopy(debugName);
	skt->platSkt = ret;
	skt->priority = kFskNetSocketLowestPriority;

#if TARGET_OS_ANDROID
// - explicitly not setting UDP to lock the Wifi on Android
// 		trying this because on linux, we use UDP for interthread communication
//		and it triggers a wifi lock and then unlock when it binds local
//	if (gFskPhoneHWInfo->wifiAddr) {
//		skt->isWifi = true;
//		gAndroidCallbacks->addWifiSocketCB();
//	}
#endif
	FskNetSocketMakeNonblocking(skt);

	FskInstrumentedItemNew(skt, skt->debugName, &gFskSocketTypeInstrumentation);
    FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgCreateUDP, skt);

	*newSocket = skt;
#if SUPPORT_INSTRUMENTATION
	// @@ very very very ad-hoc way to avoid a crash at FskNetTerminate
	if (FskStrCompare(debugName, "instrumentation syslog") != 0)
#endif
	sMakeSocketActive(skt);

	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskNetSocketNewTCPPrioritized(FskSocket *newSocket, Boolean listener,
				int priority, char *debugName)
{
	int	ret;
	FskSocket skt = NULL;

	if (gQuitting)
		return kFskErrOutOfSequence;

#if !TARGET_OS_KPL
	ret = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ret < 0) {
		return sConvertErrorToFskErr(skt, FskGetErrno());
	}
#else
	{
	KplSocket kplSocket;
	ret = KplSocketNewTCP(&kplSocket, listener);
	if (0 != ret)
		return ret;
	ret = (int)kplSocket;
	}
#endif
#if TARGET_OS_MAC
    {
        int set = 1;
        setsockopt(ret, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    }
#endif

	FskMemPtrNewClear(sizeof(FskSocketRecord), (FskMemPtr*)(void*)&skt);
	skt->platSkt = ret;
	skt->isSocket = true;
	skt->priority = priority;
	skt->debugName = FskStrDoCopy(debugName);
#if TARGET_OS_ANDROID
	if (gFskPhoneHWInfo->wifiAddr) {
		skt->isWifi = true;
		gAndroidCallbacks->addWifiSocketCB();
	}
#endif

	*newSocket = skt;
	FskInstrumentedItemNew(skt, skt->debugName, &gFskSocketTypeInstrumentation);

    FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgCreateTCP, skt);

	sMakeSocketActive(skt);

	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskNetSocketClose(FskSocket skt)
{
	if (skt == NULL)
		return kFskErrBadSocket;
#if CLOSED_SSL
	if (skt->fssl != NULL) {
		FskSSLClose(skt->fssl);
		FskSSLDispose(skt->fssl);
		skt->fssl = NULL;
	}
#endif

    FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgClose, skt);
	sMakeSocketInactive(skt);

	if (skt->handler != NULL)
		FskThreadRemoveDataHandler(&skt->handler);

	if (skt->platSkt >= 0) {
	#if OPEN_SSL
		if (skt->ssl) {
			int ret;
			BIO_flush(skt->bio);
			ret = SSL_shutdown(skt->ssl);
			if (!ret) {
				shutdown(skt->platSkt, 1);
				ret = SSL_shutdown(skt->ssl);
			}
			SSL_free(skt->ssl);
// - SSL_free does this			BIO_free(skt->bio);
			skt->ssl = NULL;
			skt->bio = NULL;
		}
	#endif
		FskMemPtrDisposeAt((void**)(void*)&skt->hostname);

		if (!skt->owned) {
	#if TARGET_OS_MAC
			if (skt->cfRunLoopSource) {
				CFRunLoopRemoveSource(CFRunLoopGetCurrent(), skt->cfRunLoopSource, kCFRunLoopCommonModes);
				CFRelease(skt->cfRunLoopSource);
				skt->cfRunLoopSource = NULL;
			}
			if (skt->cfSkt) {
				CFSocketInvalidate(skt->cfSkt);
				CFRelease(skt->cfSkt);
			}
			else
				close(skt->platSkt);
	#elif TARGET_OS_KPL
			KplSocketClose((KplSocket)skt->platSkt);
	#elif TARGET_OS_ANDROID
			if (skt->isWifi) {
				gAndroidCallbacks->removeWifiSocketCB();
			}
			closesocket(skt->platSkt);
	#else
			closesocket(skt->platSkt);
	#endif

		}
	}
	FskInstrumentedItemDispose(skt);

	FskMemPtrDisposeAt((void**)(void*)&skt->debugName);

	FskMemPtrDispose(skt);

#if 0	// TARGET_OS_ANDROID
{
	FskSocket			skt;
	FskListMutexAcquireMutex(sSocketActiveList);
	skt = (FskSocket)sSocketActiveList->list;
	while (skt) {
		if (skt->isWifi)
			FskInstrumentedTypePrintfDebug(&gFskSocketTypeInstrumentation, " * socket %s still open\n", skt->debugName);
		skt = skt->next;
	}
	FskListMutexReleaseMutex(sSocketActiveList);
}
#endif


	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskNetSocketRecvUDP(FskSocket skt, void *buf, const int bufSize, int *amt, int *fromIP, int *fromPort)
{
	int ret;

   	FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgUDPRecv, skt);
	*amt = 0;
	if (bufSize == 0)
		return kFskErrNone;

	skt->pendingReadable = false;

#if !TARGET_OS_KPL
	{
	struct sockaddr_in	sin;
	socklen_t addrLen;
	int	bufLen;

	addrLen = sizeof(sin);
	bufLen = bufSize;
	FskMemSet(&sin, 0, sizeof(sin));
	ret = recvfrom(skt->platSkt, buf, bufLen, 0, (struct sockaddr*)(void*)&sin, &addrLen);
	if (-1 == ret) {
		return skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
	}

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListeners(skt)) {
		FskSocketInstrTrans	msg;
		msg.skt = skt;
		msg.buf = buf;
		msg.amt = ret;
   		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgUDPRecvd, &msg);
	}
#endif

	skt->ipaddrRemote = ntohl(sin.sin_addr.s_addr);
	skt->portRemote = ntohs(sin.sin_port);
	*amt = ret;

	if (fromIP)		*fromIP = skt->ipaddrRemote;
	if (fromPort)	*fromPort = skt->portRemote;

	return kFskErrNone;
	}
#else
	{
	int remAddr;
	int remPort;
	FskErr	err;

	err = skt->lastErr = KplSocketRecvUDP((KplSocket)skt->platSkt, buf, bufSize, &ret, &remAddr, &remPort);
	if (0 != err)
		return err;

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListeners(skt)) {
		FskSocketInstrTrans	msg;
		msg.skt = skt;
		msg.buf = buf;
		msg.amt = ret;
   		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgUDPRecvd, &msg);
	}
#endif

	*amt = ret;
	skt->ipaddrRemote = remAddr;
	skt->portRemote = remPort;

	if (fromIP)		*fromIP = skt->ipaddrRemote;
	if (fromPort)	*fromPort = skt->portRemote;

	return kFskErrNone;
	}
#endif
}

// ---------------------------------------------------------------------
FskErr FskNetSocketSendUDP(FskSocket skt, void *buf, const int bufSize, int *amt, int toIP, int toPort)
{
	int ret;

   	FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgUDPSend, skt);
	*amt = 0;
	if (bufSize == 0)
		return kFskErrNone;

	skt->pendingWritable = false;

#if !TARGET_OS_KPL
	{
	struct sockaddr_in sin;

	setupSockAddr(sin, toIP, toPort);
	ret = sendto(skt->platSkt, buf, bufSize, 0, (struct sockaddr*)(void*)&sin, sizeof(sin));
	if (-1 == ret) {
		return skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
	}
	}
#else
	{
	FskErr err = KplSocketSendUDP((KplSocket)skt->platSkt, buf, bufSize, &ret, toIP, toPort);
	if (0 != err)
		return skt->lastErr = err;
	}
#endif

	*amt = ret;

	skt->ipaddrRemote = toIP;
	skt->portRemote = toPort;

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListeners(skt)) {
		FskSocketInstrTrans	msg;
		msg.skt = skt;
		msg.buf = buf;
		msg.amt = ret;
   		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgUDPSent, &msg);
	}
#endif

	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskErr FskNetSocketRecvTCP(FskSocket skt, void *buf, const int bufSize, int *amt)
{
#if CLOSED_SSL
	if (skt != NULL && skt->fssl != NULL) {
		FskErr err;
		*amt = bufSize;
		err = FskSSLRead(skt->fssl, buf, amt);
		if (err != kFskErrNone) *amt = 0;
		return err;
	}
#endif
	return FskNetSocketRecvRawTCP(skt, buf, bufSize, amt);
}

FskErr FskNetSocketRecvRawTCP(FskSocket skt, void *buf, const int bufSize, int *amt)
{
	int ret;
	int bufLen;
volatile	FskErr		err;

	if (!skt)
		return kFskErrParameterError;

	FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgTCPRecv, skt);
	*amt = 0;

	if (skt->platSkt < 0)
		return kFskErrSocketNotConnected;

	if (bufSize == 0)
		return kFskErrNone;

	skt->pendingReadable = false;
	bufLen = bufSize;

#if OPEN_SSL
	if (skt->ssl) {
		ret = SSL_read(skt->ssl, buf, bufLen);
		err = SSL_get_error(skt->ssl, ret);
		switch (err) {
			case SSL_ERROR_NONE:
				*amt = ret;
#if SUPPORT_INSTRUMENTATION
				if (FskInstrumentedItemHasListeners(skt)) {
					FskSocketInstrTrans	msg;
					msg.skt = skt;
					msg.buf = buf;
					msg.amt = ret;
		   			FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgSSLRecvd, &msg);
				}
#endif
				break;
			case SSL_ERROR_ZERO_RETURN:
				SSL_clear(skt->ssl);
				SSL_free(skt->ssl);
				ERR_remove_state(0);
				skt->ssl = NULL;
	    		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgSSLClosed, skt);
				return kFskErrConnectionClosed;
			case SSL_ERROR_WANT_READ:
				return kFskErrNoData;
			case SSL_ERROR_WANT_WRITE:
				return kFskErrNoData;
			default:
	    		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgSSLRecvErr, skt);
				ERR_print_errors_fp(stderr);
				if (SSL_get_shutdown(skt->ssl) & SSL_RECEIVED_SHUTDOWN)
					SSL_shutdown(skt->ssl);
				else
					SSL_clear(skt->ssl);
				SSL_free(skt->ssl);
				ERR_remove_state(0);
				skt->ssl = NULL;
				return kFskErrConnectionClosed;
 		}
	}
	else
#endif
	{
#if !TARGET_OS_KPL
		ret = recv(skt->platSkt, buf, bufLen, 0);
		if (0 == ret)
			return kFskErrConnectionClosed;
		if (-1 == ret) {
			err = skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
			goto bail;
		}
#else
		err = KplSocketRecvTCP((KplSocket)skt->platSkt, buf, bufLen, &ret);
		if (0 != err) {
			if (kFskErrConnectionClosed == err)
				return kFskErrConnectionClosed;
			skt->lastErr = err;
			goto bail;
		}
#endif

		*amt = ret;

#if SUPPORT_INSTRUMENTATION
		if (FskInstrumentedItemHasListeners(skt)) {
			FskSocketInstrTrans	msg;
			msg.skt = skt;
			msg.buf = buf;
			msg.amt = ret;
   			FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgTCPRecvd, &msg);
		}
#endif
	}

	return kFskErrNone;

bail:
	return err;
}

// ---------------------------------------------------------------------
FskErr FskNetSocketSendTCP(FskSocket skt, void *buf, const int bufSize, int *amt)
{
#if CLOSED_SSL
	if (skt != NULL && skt->fssl != NULL) {
		FskErr err;
		*amt = bufSize;
		err = FskSSLWrite(skt->fssl, buf, amt);
		if (err != kFskErrNone) *amt = 0;
		return err;
	}
#endif
	return FskNetSocketSendRawTCP(skt, buf, bufSize, amt);
}

FskErr FskNetSocketSendRawTCP(FskSocket skt, void *buf, const int bufSize, int *amt)
{
	int ret;
	int bufLen;
	FskErr err;

	if (!skt)
		return kFskErrParameterError;

	FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgTCPSend, skt);
	*amt = 0;

	skt->pendingWritable = false;

	if (skt->platSkt < 0) {
		*amt = bufSize;
		return kFskErrSocketNotConnected;
	}

	bufLen = bufSize;

#if OPEN_SSL
	if (skt->ssl) {
		int err;
		ret = SSL_write(skt->ssl, buf, bufLen);
		err = SSL_get_error(skt->ssl, ret);
		switch (err) {
			case SSL_ERROR_NONE:
				*amt = ret;
#if SUPPORT_INSTRUMENTATION
				if (FskInstrumentedItemHasListeners(skt)) {
					FskSocketInstrTrans	msg;
					msg.skt = skt;
					msg.buf = buf;
					msg.amt = ret;
		   			FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgSSLSent, &msg);
				}
#endif
				break;
			case SSL_ERROR_ZERO_RETURN:
				SSL_clear(skt->ssl);
				SSL_free(skt->ssl);
				ERR_remove_state(0);
				skt->ssl = NULL;
	    		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgSSLClosed, skt);
				return kFskErrConnectionClosed;
			case SSL_ERROR_WANT_READ:
				return kFskErrNoData;
			case SSL_ERROR_WANT_WRITE:
				return kFskErrNoData;
			default:
	    		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgSSLSendErr, skt);
				ERR_print_errors_fp(stderr);
				if (SSL_get_shutdown(skt->ssl) & SSL_RECEIVED_SHUTDOWN)
					SSL_shutdown(skt->ssl);
				else
					SSL_clear(skt->ssl);
				SSL_free(skt->ssl);
				ERR_remove_state(0);
				skt->ssl = NULL;
				return kFskErrConnectionClosed;
		}
	}
	else
#endif
	{
#if !TARGET_OS_KPL
		ret = send(skt->platSkt, buf, bufLen, 0);
		if (ret == -1){
			err = skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
			goto bail;
		}
#else
		err = KplSocketSendTCP((KplSocket)skt->platSkt, buf, bufLen, &ret);
		if (0 != err) {
			skt->lastErr = err;
			goto bail;
		}
#endif
		*amt = ret;
#if SUPPORT_INSTRUMENTATION
		if (FskInstrumentedItemHasListeners(skt)) {
			FskSocketInstrTrans	msg;
			msg.skt = skt;
			msg.buf = buf;
			msg.amt = ret;
   			FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgTCPSent, &msg);
		}
#endif
	}

	return kFskErrNone;

bail:
	return err;
}

FskErr FskNetSocketFlushTCP(FskSocket skt)
{
#if CLOSED_SSL
	if (skt->fssl != NULL)
		return FskSSLFlush(skt->fssl);
	else
		return kFskErrNone;
#else
	return kFskErrNone;
#endif
}

// ---------------------------------------------------------------------
FskErr FskNetSocketOptions(FskSocket skt, int sktLevel, int sktOption, int val)
{
	int err;

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListeners(skt)) {
		FskSocketInstrMisc	msg;
		msg.skt = skt;
		msg.sktLevel = sktLevel;
		msg.sktOption = sktOption;
		msg.val = val;
   		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgSetOptions, &msg);
	}
#endif
#if !TARGET_OS_KPL
	err = setsockopt(skt->platSkt, sktLevel, sktOption, (char*)&val, sizeof(val));
	if (err == -1) {
		return skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
	}
#else
	{
	int kplSktOption, kplSktLevel = kKplSocketLevelSocket;
	switch(sktOption) {
		case SO_REUSEADDR:
			kplSktOption = kKplSocketOptionReuseAddr;
			break;
		case SO_DONTROUTE:
			kplSktOption = kKplSocketOptionDontRoute;
			break;
		case SO_BROADCAST:
			kplSktOption = kKplSocketOptionBroadcast;
			break;
		case SO_RCVBUF:
			kplSktOption = kKplSocketOptionRcvBuf;
			break;
		case SO_SNDBUF:
			kplSktOption = kKplSocketOptionSndBuf;
			break;
		case SO_KEEPALIVE:
			kplSktOption = kKplSocketOptionKeepAlive;
			break;
		default:
			return skt->lastErr = kFskErrUnimplemented;
	}
	err = KplSocketSetOption((KplSocket)skt->platSkt, kplSktLevel, kplSktOption, val);
	if (0 != err)
		return skt->lastErr = err;
	}
#endif

	return kFskErrNone;
}

// ---------------------------------------------------------------------

FskErr FskNetSocketReuseAddress(FskSocket skt) {
	return FskNetSocketOptions(skt, SOL_SOCKET, SO_REUSEADDR, 1);
}

FskErr FskNetSocketDontRoute(FskSocket skt) {
	return FskNetSocketOptions(skt, SOL_SOCKET, SO_DONTROUTE, 1);
}

FskErr FskNetSocketEnableBroadcast(FskSocket skt) {
	return FskNetSocketOptions(skt, SOL_SOCKET, SO_BROADCAST, 1);
}

FskErr FskNetSocketDisableBroadcast(FskSocket skt) {
	return FskNetSocketOptions(skt, SOL_SOCKET, SO_BROADCAST, 0);
}

FskErr FskNetSocketReceiveBufferSetSize(FskSocket skt, int val) {
	return FskNetSocketOptions(skt, SOL_SOCKET, SO_RCVBUF, val);
}

FskErr FskNetSocketSendBufferSetSize(FskSocket skt, int val) {
	return FskNetSocketOptions(skt, SOL_SOCKET, SO_SNDBUF, val);
}

FskErr FskNetSocketSetKeepAlive(FskSocket skt, int val) {
	return FskNetSocketOptions(skt, SOL_SOCKET, SO_KEEPALIVE, val);
}

// ---------------------------------------------------------------------
FskErr FskNetSocketBind(FskSocket skt, int addr, int port)
{
	int err;

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListeners(skt)) {
		FskSocketInstrMisc	msg;
		msg.skt = skt;
		msg.addr = addr;
		msg.port = port;
   		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgBind, &msg);
	}
#endif

#if !TARGET_OS_KPL
	{
	struct sockaddr_in sin;

	if (addr == -1)
		addr = INADDR_ANY;

	setupSockAddr(sin, addr, port);
	err = bind(skt->platSkt, (struct sockaddr*)(void*)&sin, sizeof(sin));
	if (err == -1) {
		return skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
	}
	}
#else
	if (addr == -1)
		addr = KPL_INADDR_ANY;

	err = KplSocketBind((KplSocket)skt->platSkt, addr, port);
	if (0 != err)
		return skt->lastErr = err;
#endif

	skt->ipaddrLocal = addr;
	skt->portLocal = port;

#if TARGET_OS_ANDROID
	if (addr == 0x7f000001) {
		if (skt->isWifi) {
			FskInstrumentedTypePrintfDebug(&gFskSocketTypeInstrumentation, "address is localhost, disable isWifi\n");
			gAndroidCallbacks->removeWifiSocketCB();
			skt->isWifi = false;
		}
	}
#endif

	return kFskErrNone;
}

FskErr FskNetSocketSetTTL(FskSocket skt, int ttl) {
	int err;
#if !TARGET_OS_KPL
	err = setsockopt(skt->platSkt, IPPROTO_IP, IP_TTL, (char*)&ttl, sizeof(ttl));
	if (err == 0)
		err = skt->lastErr = kFskErrNone;
	else
		err = skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
#else
	err = skt->lastErr = KplSocketSetTTL((KplSocket)skt->platSkt, ttl);
#endif
	return err;
}


// ---------------------------------------------------------------------
FskErr FskNetSocketMulticastAddMembership(FskSocket skt, int multicastAddr, int interfaceAddr) {
	int err;
#if !TARGET_OS_KPL
	{
	struct ip_mreq maddr;
	maddr.imr_multiaddr.s_addr = htonl(multicastAddr);
	maddr.imr_interface.s_addr = htonl(interfaceAddr);
	err = setsockopt(skt->platSkt, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&maddr, sizeof(struct ip_mreq));
	if (err == 0)
		err = skt->lastErr = kFskErrNone;
	else
		err = skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
	}
#else
	err = skt->lastErr = KplSocketMulticastAddMembership((KplSocket)skt->platSkt, multicastAddr, interfaceAddr);
#endif
	return err;
}

FskErr FskNetSocketMulticastDropMembership(FskSocket skt, int multicastAddr, int interfaceAddr) {
	int err;
#if !TARGET_OS_KPL
	{
	struct ip_mreq maddr;
	maddr.imr_multiaddr.s_addr = htonl(multicastAddr);
	maddr.imr_interface.s_addr = htonl(interfaceAddr);
	err = setsockopt(skt->platSkt, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&maddr, sizeof(struct ip_mreq));
	if (err == 0)
		err = skt->lastErr = kFskErrNone;
	else
		err = skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
	}
#else
	err = skt->lastErr = KplSocketMulticastDropMembership((KplSocket)skt->platSkt, multicastAddr, interfaceAddr);
#endif
	return err;
}

FskErr FskNetSocketMulticastSetTTL(FskSocket skt, int ttl) {
	int err;
#if !TARGET_OS_KPL
	err = setsockopt(skt->platSkt, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl));
	if (err == 0)
		err = skt->lastErr = kFskErrNone;
	else
		err = skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
#else
	err = skt->lastErr = KplSocketMulticastSetTTL((KplSocket)skt->platSkt, ttl);
#endif
	return err;
}

// ---------------------------------------------------------------------
FskErr FskNetSocketMulticastJoin(FskSocket skt, int multicastAddr, int interfaceAddr, int ttl)
{
	int err;

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListeners(skt)) {
		FskSocketInstrMisc	msg;
		msg.skt = skt;
		msg.multicastAddr = multicastAddr;
		msg.interfaceAddr = interfaceAddr;
		msg.ttl = ttl;
   		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgMulticastJoin, &msg);
	}
#endif

	err = FskNetSocketMulticastAddMembership(skt, multicastAddr, interfaceAddr);
	if (kFskErrNone == err)
		err = FskNetSocketMulticastSetTTL(skt, ttl);

	return err;
}

FskErr FskNetSocketMulticastLoop(FskSocket skt, char val) {
#if !TARGET_OS_KPL
	int err;
	err = setsockopt(skt->platSkt, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&val, sizeof(val));
	if (err == 0)
		skt->lastErr = kFskErrNone;
	else {
		skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
	}
#else
	skt->lastErr = KplSocketMulticastLoop((KplSocket)skt->platSkt, val);
#endif

	return kFskErrNone;
}

FskErr FskNetSocketMulticastSetOutgoingInterface(FskSocket skt, int interfaceAddr, int ttl)
{
	int err;

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListeners(skt)) {
		FskSocketInstrMisc	msg;
		msg.skt = skt;
		msg.interfaceAddr = interfaceAddr;
   		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgSetOutgoingInterface, &msg);
	}
#endif

#if TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_WIN32
	{
	struct in_addr addr;
	int				param;

	addr.s_addr = htonl(interfaceAddr);
	err = setsockopt(skt->platSkt, IPPROTO_IP, IP_MULTICAST_IF, (char*)&addr, sizeof(addr));
	if (err == -1) {
		return skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
	}
	param = ttl;
	err = setsockopt(skt->platSkt, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&param, sizeof(param));
	if (err == -1) {
		return skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
	}
	err = kFskErrNone;
	}
#elif TARGET_OS_KPL
	err = skt->lastErr = KplSocketMulticastSetOutgoingInterface((KplSocket)skt->platSkt, interfaceAddr, ttl);
#endif

	return err;
}

// ---------------------------------------------------------------------
FskErr FskNetSocketListen(FskSocket skt)
{
	int err;
   	FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgTCPListen, skt);

#if !TARGET_OS_KPL
	err = listen(skt->platSkt, 8);
	if (err == -1) {
		return skt->lastErr = sConvertErrorToFskErr(skt, FskGetErrno());
	}
	err = kFskErrNone;
#else
	err = KplSocketListen((KplSocket)skt->platSkt);
	if (0 != err)
		return skt->lastErr = err;
#endif

	return err;
}

// ---------------------------------------------------------------------
FskErr FskNetAcceptConnection(FskSocket listeningSkt, FskSocket *createdSocket, char *debugName)
{
	FskSocket skt = NULL;
	FskErr err = kFskErrNone;
	int ipAddr, port;
#if TARGET_OS_KPL
	KplSocket aSkt	= NULL;
#else
	FskSocketPlatform aSkt	= 0;
	struct sockaddr addr;
	socklen_t addrLen = sizeof(addr);
#endif

   	FskInstrumentedItemSendMessage(listeningSkt, kFskSocketInstrMsgTCPAccepting, listeningSkt);
	if (!FskNetSocketIsReadable(listeningSkt))
        BAIL(kFskErrNoData);

	listeningSkt->pendingReadable = false;

#if TARGET_OS_KPL
	err = KplSocketAcceptConnection((KplSocket)listeningSkt->platSkt, &aSkt, &ipAddr, &port);
    BAIL_IF_ERR (err);
#else
	aSkt = accept(listeningSkt->platSkt, &addr, &addrLen);
	if (aSkt < 0)
        BAIL(kFskErrSocketNotConnected);
	else {
		struct sockaddr_in *in_addr = (struct sockaddr_in*)(void*)&addr;
		ipAddr = ntohl(in_addr->sin_addr.s_addr);
		port = ntohs(in_addr->sin_port);
	}
#endif

	err = FskMemPtrNewClear(sizeof(FskSocketRecord), (FskMemPtr*)(void*)&skt);
    BAIL_IF_ERR(err);

	skt->isSocket = true;
	skt->platSkt = (FskSocketPlatform)aSkt;
	skt->ipaddrRemote = ipAddr;
	skt->portRemote = port;
	skt->debugName = FskStrDoCat("accepted from ", listeningSkt->debugName);
#if TARGET_OS_ANDROID
	if (gFskPhoneHWInfo->wifiAddr) {
		skt->isWifi = true;
		gAndroidCallbacks->addWifiSocketCB();
	}
#endif
	FskInstrumentedItemNew(skt, skt->debugName, &gFskSocketTypeInstrumentation);
   	FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgTCPAccepted, skt);
	sMakeSocketActive(skt);

bail:
	if (kFskErrNone != err) {
		if (aSkt > 0) {
#if TARGET_OS_KPL
			KplSocketClose((KplSocket)aSkt);
#else
			(void)closesocket(aSkt);
#endif
		}
	}
	
	*createdSocket = skt;
	return err;
}

// ---------------------------------------------------------------------
#if OPEN_SSL
static void sFskNetSocketSSLTransaction(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon) {
	FskSocket skt = (FskSocket)refCon;
	sDoSSLTransaction(skt);
}

static void sDoSSLTransaction(FskSocket skt) {
	int result;
	Boolean wantRead = false, wantWrite = false;

	if (skt->sslTransactionHandler)
		FskThreadRemoveDataHandler(&skt->sslTransactionHandler);

	switch (skt->sslState) {
		case kSSLStateConnect:
   			FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgSSLStateConnect, skt);
			result = SSL_connect(skt->ssl);
			if (result <= 0) {
				result = SSL_get_error(skt->ssl, result);
				switch (result) {
					case SSL_ERROR_WANT_WRITE:
						wantWrite = true;
						break;
					case SSL_ERROR_WANT_READ:
						wantRead = true;
						break;
					default:
   						FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgSSLConnectErr, ERR_error_string(result, NULL));
						break;
				}
			}
			else {
				skt->sslState = kSSLStateCheckServer;
				sDoSSLTransaction(skt);
			}

			if (wantRead || wantWrite) {
				FskThreadAddDataHandler(&skt->sslTransactionHandler, (FskThreadDataSource)skt, sFskNetSocketSSLTransaction, wantRead, wantWrite, skt);
			}
			break;
		case kSSLStateCheckServer:
			if (!FskSSLCheckServerCert(skt->ssl, skt->hostname)) {
   				FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgSSLServerCertFailed, skt);
				result = (*skt->afterCreate)(NULL, skt->afterCreateRefCon);
			}
			else {
   				FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgSSLServerCertSuccess, skt);
				result = (*skt->afterCreate)(skt, skt->afterCreateRefCon);
			}
			break;
	}
}

// ---------------------------------------------------------------------
static void doSSLConnected(FskSocket skt) {
	FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgTCPConnectedSSL, skt);

	if (!skt->ssl) {
		skt->ssl = SSL_new(FskSSLGetContext());
		skt->bio = BIO_new_socket(skt->platSkt, BIO_NOCLOSE);
		SSL_set_bio(skt->ssl, skt->bio, skt->bio);
		skt->sslState = kSSLStateConnect;
	}
	else
		skt->sslState = kSSLStateCheckServer;

	sDoSSLTransaction(skt);
}

static void sSSLSocketConnected(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon) {
	FskSocket skt = (FskSocket)refCon;
	FskThreadRemoveDataHandler(&skt->handler);
	doSSLConnected(skt);
}
#endif		// OPEN_SSL

// ---------------------------------------------------------------------
static void doConnected(FskSocket skt) {
	FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgTCPConnected, skt);
	(*skt->afterCreate)(skt, skt->afterCreateRefCon);
}

static void sSocketConnected(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon) {
	FskSocket skt = (FskSocket)refCon;

	FskThreadRemoveDataHandler(&skt->handler);
	doConnected(skt);
}

// ---------------------------------------------------------------------

static FskErr sConnectToHost(FskSocket skt)
{
	FskErr	err = kFskErrNone;

	if ((0 == skt->ipaddrRemote) && (kFskErrNone == skt->lastErr))
		skt->lastErr = kFskErrNameLookupFailed;

	if (skt->lastErr) {
		err = skt->lastErr;
		goto failed;
	}

	if (skt->nonblocking)
		FskNetSocketMakeNonblocking(skt);

	FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgTCPConnect, skt);

#if !TARGET_OS_KPL
	{
	int	ret;
	struct sockaddr_in	tcp_srv_addr;
	setupSockAddr(tcp_srv_addr, skt->ipaddrRemote, skt->portRemote);
	ret = connect(skt->platSkt, (struct sockaddr*)(void*)&tcp_srv_addr, sizeof(tcp_srv_addr));

	if (ret == -1) {
		err = FskGetErrno();
		skt->lastErr = sConvertErrorToFskErr(skt, err);
		if ((skt->lastErr == kFskErrNoData) && skt->nonblocking) {
			skt->lastErr = kFskErrNone;
#if  TARGET_OS_MAC
	#if OPEN_SSL
			if (skt->isSSL)
				FskThreadAddDataHandler(&skt->handler, (FskThreadDataSource)skt, sSSLSocketConnected, true, true, skt);
			else
	#endif
				FskThreadAddDataHandler(&skt->handler, (FskThreadDataSource)skt, sSocketConnected, true, true, skt);
#else

	#if OPEN_SSL
			if (skt->isSSL)
				FskThreadAddDataHandler(&skt->handler, (FskThreadDataSource)skt, sSSLSocketConnected, false, true, skt);
			else
	#endif
				FskThreadAddDataHandler(&skt->handler, (FskThreadDataSource)skt, sSocketConnected, false, true, skt);
#endif
			return kFskErrNone;
		}

		(*skt->afterCreate)(NULL, skt->afterCreateRefCon);

		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgTCPConnectFailed, skt);

		FskNetSocketClose(skt);

		return err;
	}
	}
#else
	err = skt->lastErr = KplSocketConnect((KplSocket)skt->platSkt, skt->ipaddrRemote, skt->portRemote);
	if (0 != err) {
		if ((skt->lastErr == kFskErrNoData) && skt->nonblocking) {
			skt->lastErr = kFskErrNone;
	#if OPEN_SSL
			if (skt->isSSL)
				FskThreadAddDataHandler(&skt->handler, (FskThreadDataSource)skt, sSSLSocketConnected, false, true, skt);
			else
	#endif
				FskThreadAddDataHandler(&skt->handler, (FskThreadDataSource)skt, sSocketConnected, false, true, skt);
			return kFskErrNone;
		}

		(*skt->afterCreate)(NULL, skt->afterCreateRefCon);

		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgTCPConnectFailed, skt);

		FskNetSocketClose(skt);

		return err;
	}
#endif

failed:
	if (err)
		FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgTCPConnectFailed, skt);

#if OPEN_SSL
	if (skt->isSSL) {
		doSSLConnected(skt);
	}
	else
#endif
	{
		doConnected(skt);
	}

	return err;
}

// ---------------------------------------------------------------------
static void sConnectResolved(FskResolver rr) {
	FskSocket skt = (FskSocket)rr->ref;

	FskInstrumentedTypePrintfDebug(&gFskSocketTypeInstrumentation, "[%s] ConnectResolved IP: %u, err: %d (skt lastErr was %d)\n", threadTag(FskThreadGetCurrent()), (unsigned)rr->resolvedIP, rr->err, skt->lastErr);
	if (rr->err)
		skt->lastErr = rr->err;
	skt->ipaddrRemote = rr->resolvedIP;
	if (rr->targetPort != 0)
		skt->portRemote = rr->targetPort;
	sConnectToHost(skt);
	return;
}

FskErr FskNetConnectToHostPrioritized(char *host, int port, Boolean blocking,
			FskNetSocketCreatedCallback callback, void *refCon, long flags,
			int priority, FskSocketCertificate cert, char *debugName)
{
	FskSocket	skt = NULL;
	FskErr		err = kFskErrNone;

#if CLOSED_SSL
	if (flags & kConnectFlagsSSLConnection) {
		void *ssl;
		err = FskSSLNew(&ssl, host, port, blocking, flags, priority);
		if (err != kFskErrUnimplemented) {
			if (err == kFskErrNone) {
				if (cert != NULL)
					FskSSLLoadCerts(ssl, cert);
				err = FskSSLHandshake(ssl, callback, refCon, true, 0);
			}
			return err;
		}
		/* else fall thru */
	}
#endif
	err = FskNetSocketNewTCPPrioritized(&skt, false, priority, debugName);
	BAIL_IF_ERR(err);
    FskAssert(skt);
	skt->portRemote = port;
	skt->hostname = FskStrDoCopy(host);

#if OPEN_SSL
	if (flags & kConnectFlagsSSLConnection) {
		skt->isSSL = true;
	}
#endif
	skt->afterCreate = callback;
	skt->afterCreateRefCon = refCon;
	skt->nonblocking = !blocking;

	FskInstrumentedItemSendMessage(skt, kFskSocketInstrMsgTCPBeginConnect, skt);

	if (0 == (flags & kConnectFlagsSynchronous)) {
		FskResolver rr = NULL;		// may want to store this, so we can cancel the connection
		FskInstrumentedTypePrintfDebug(&gFskSocketTypeInstrumentation, "About to resolve [%s] async\n", host);
		if (kFskErrNone != (err = FskNetHostnameResolveQTAsync(host, flags & kConnectFlagsQueryTypeMask, sConnectResolved, skt, &rr)))
			goto bail;
	}
	else {
		FskResolver rr;

		err = FskNetEnsureConnection(NULL);
		BAIL_IF_ERR(err);

		if (kFskErrNone == (err = FskMemPtrNewClear(sizeof(FskResolverRecord), (FskMemPtr*)(void*)&rr))) {
			UInt16 thePort;
			FskInstrumentedTypePrintfDebug(&gFskSocketTypeInstrumentation, "[%s] About to resolve [%s] sync\n", threadTag(FskThreadGetCurrent()), host);
			if (kFskErrNone == (err = FskNetHostnameResolveQT(host, flags & kConnectFlagsQueryTypeMask, &rr->resolvedIP, &thePort))) {
				if (thePort != 0) skt->portRemote = thePort;
				rr->ref = skt;
				sConnectResolved(rr);
			}
			FskMemPtrDispose(rr);
		}
	}

bail:
	if (kFskErrNone != err) {
		FskNetSocketClose(skt);
	}

	return err;
}


void FskNetCancelConnection(void *connectingRef)
{
	FskResolverCancelByRef(connectingRef);
}

// ---------------------------------------------------------------------
void FskNetStringToIPandPort(const char *buf, int *dataAddr, int *dataPort)
{
	char *foo, temp[64];
	int	ip = 0, port = 0;

	if (NULL == buf)
		goto done;

	foo = FskStrNCopyUntil(temp, buf, 63, '.');
	ip = (ip << 8) | FskStrToNum(temp);
	foo = FskStrNCopyUntil(temp, foo, 63, '.');
	ip = (ip << 8) | FskStrToNum(temp);
	foo = FskStrNCopyUntil(temp, foo, 63, '.');
	ip = (ip << 8) | FskStrToNum(temp);
	foo = FskStrNCopyUntil(temp, foo, 63, ':');
	ip = (ip << 8) | FskStrToNum(temp);
	port = FskStrToNum(foo);

done:
	if (dataAddr)
		*dataAddr = ip;
	if (dataPort)
		*dataPort = port;
}

// ---------------------------------------------------------------------
// addrString must be at least 22 characters in size.
// 3 digits for each of four numbers = 12, + 5 digits for port number
// + 4 dots and colons + termination null
void FskNetIPandPortToString(int IP, int port, char *addrString)
{
	if (port)
		snprintf(addrString, 22, "%u.%u.%u.%u:%u",
			(IP & 0xff000000) >> 24, (IP & 0x00ff0000) >> 16,
			(IP & 0x0000ff00) >> 8, (IP & 0x000000ff), port);
	else
		snprintf(addrString, 22, "%u.%u.%u.%u",
			(IP & 0xff000000) >> 24, (IP & 0x00ff0000) >> 16,
			(IP & 0x0000ff00) >> 8, (IP & 0x000000ff));
}

FskErr FskNetSocketGetLastError(FskSocket skt)
{
	return skt->lastErr;
}

FskSocketCertificate FskNetUtilCopyCertificate(FskSocketCertificate src)
{
	FskSocketCertificate dst;
	FskErr err;

	if ((err = FskMemPtrNewClear(sizeof(FskSocketCertificateRecord), &dst)) != kFskErrNone)
		return NULL;
	if (src->certificates != NULL && src->certificatesSize > 0) {
		if ((err = FskMemPtrNewFromData(src->certificatesSize, src->certificates, (FskMemPtr *)&dst->certificates)) != kFskErrNone)
			goto bail;
		dst->certificatesSize = src->certificatesSize;
	}
	if (src->policies != NULL) {
		dst->policies = FskStrDoCopy(src->policies);
		if (dst->policies == NULL) {
			err = kFskErrMemFull;
			goto bail;
		}
	}
	if (src->hostname != NULL) {
		dst->hostname = FskStrDoCopy(src->hostname);
		if (dst->hostname == NULL) {
			err = kFskErrMemFull;
			goto bail;
		}
	}
	if (src->key != NULL && src->keySize > 0) {
		if ((err = FskMemPtrNewFromData(src->keySize, src->key, (FskMemPtr *)&dst->key)) != kFskErrNone)
			goto bail;
		dst->keySize = src->keySize;
	}
bail:
	if (err != kFskErrNone) {
		FskNetUtilDisposeCertificate(dst);
		dst = NULL;
	}
	return dst;
}

void FskNetUtilDisposeCertificate(FskSocketCertificate cert)
{
	if (cert != NULL) {
		if (cert->certificates != NULL && cert->certificatesSize > 0)
			FskMemPtrDispose(cert->certificates);
		if (cert->policies != NULL)
			FskMemPtrDispose(cert->policies);
		if (cert->hostname != NULL)
			FskMemPtrDispose(cert->hostname);
		if (cert->key != NULL && cert->keySize > 0)
			FskMemPtrDispose(cert->key);
	}
	FskMemPtrDispose(cert);
}


// ---------------------------------------------------------------------
#if TARGET_OS_WIN32
static FskErr initWinSock(void)
{
	int		err;
	WORD	vers;
	WSADATA winSock;

	vers = MAKEWORD(1,1);
	err = WSAStartup(vers, &winSock);
	BAIL_IF_ERR(err);

	gAsyncSelectMessage = RegisterWindowMessage("FskNetUtilsMessage");

bail:
	return err;
}

static FskSocket win32FindFskSocketBySOCKET(SOCKET s) {
	FskSocket cur;

	FskMutexAcquire(sSocketActiveList->mutex);
	cur = (FskSocket)sSocketActiveList->list;
	while (cur) {
		if (cur->platSkt == s)
			break;
		cur = (FskSocket)cur->next;
	}

	FskMutexRelease(sSocketActiveList->mutex);
	return cur;
}

void win32SocketEvent(UINT wParam, UINT event) {
	FskSocket skt;

	if (0 == gFskNetInitialized)
		return;

	skt = win32FindFskSocketBySOCKET(wParam);
	if (!skt)
		return;
	if ((event == FD_READ) || (event == FD_ACCEPT))
		skt->pendingReadable = true;
	if ((event == FD_WRITE) || (event == FD_CONNECT))
		skt->pendingWritable = true;
	if (event == FD_CLOSE)
		skt->pendingClose = true;
}

#elif TARGET_OS_MAC

static FskSocket macFindFskSocketByCFSocket(CFSocketRef s) {
	FskSocket cur;

	FskMutexAcquire(sSocketActiveList->mutex);
	cur = sSocketActiveList->list;
	while (cur) {
		if (cur->cfSkt == s)
			break;
		cur = (FskSocket)cur->next;
	}
	FskMutexRelease(sSocketActiveList->mutex);
	return cur;
}

static FskSocket macFindFskSocketByPlatSocket(int s) {
	FskSocket cur;

	FskMutexAcquire(sSocketActiveList->mutex);
	cur = sSocketActiveList->list;
	while (cur) {
		if (cur->platSkt == s)
			break;
		cur = (FskSocket)cur->next;
	}
	FskMutexRelease(sSocketActiveList->mutex);
	return cur;
}

static void macCallBack(CFSocketRef s, CFSocketCallBackType cbType,
	CFDataRef addr, const void *data, void *info) {
	FskSocket skt;

	skt = macFindFskSocketByCFSocket(s);
	if (!skt)
		return;
	skt->pendingReadable = (cbType & kCFSocketReadCallBack) ? true : false;
	skt->pendingWritable = (cbType & kCFSocketWriteCallBack) ? true : false;
	MacThreadGotSocketData();
}

void macSocketCallbackEnable(int s, Boolean wantsReadable, Boolean wantsWritable) {
	FskSocket skt;
	CFOptionFlags enableFlags = 0L, disableFlags = 0L;

	skt = macFindFskSocketByPlatSocket(s);
	if (!skt)
		return;

	if (skt->cfSkt == NULL) {
		skt->cfSkt = CFSocketCreateWithNative(kCFAllocatorDefault, skt->platSkt,
						      kCFSocketReadCallBack | kCFSocketWriteCallBack, macCallBack, NULL);
		skt->cfRunLoopSource = CFSocketCreateRunLoopSource(NULL, skt->cfSkt, 0);
		CFRunLoopAddSource(CFRunLoopGetCurrent(), skt->cfRunLoopSource, kCFRunLoopCommonModes);
	}

	if (wantsReadable) {
		enableFlags |= kCFSocketReadCallBack;
	} else {
		disableFlags |= kCFSocketReadCallBack;
	}

	if (wantsWritable) {
		enableFlags |= kCFSocketWriteCallBack;
	} else {
		disableFlags |= kCFSocketWriteCallBack;
	}

	if (enableFlags) {
		CFSocketEnableCallBacks(skt->cfSkt, enableFlags);
	}

	if (disableFlags) {
		CFSocketDisableCallBacks(skt->cfSkt, disableFlags);
	}
}

#elif TARGET_OS_KPL
FskSocket FskNetSocketFindFskSocketByPlatformSocket(int s) {
	FskSocket cur;

	FskMutexAcquire(sSocketActiveList->mutex);
	cur = sSocketActiveList->list;
	while (cur) {
		if (cur->platSkt == s)
			break;
		cur = (FskSocket)cur->next;
	}
	FskMutexRelease(sSocketActiveList->mutex);
	return cur;
}

void FskKplSocketHostEvent(KplSocket kplSocket, UInt32 eventType)
{
	FskSocket skt;

	if (0 == gFskNetInitialized)
		return;

	skt = FskNetSocketFindFskSocketByPlatformSocket((int)kplSocket);
	if (!skt)
		return;
	if (eventType == kKplSocketHostEventRead || eventType == kKplSocketHostEventAccept)
		skt->pendingReadable = true;
	if (eventType == kKplSocketHostEventWrite || eventType == kKplSocketHostEventConnect)
		skt->pendingWritable = true;
	if (eventType == kKplSocketHostEventClose)
		skt->pendingClose = true;
}
#endif

#if SUPPORT_INSTRUMENTATION

static Boolean doFormatMessageFskSocket(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize) {
	FskSocket skt = (FskSocket)msgData;
	FskSocketInstrNativeErr *native = (FskSocketInstrNativeErr*)msgData;
	FskSocketInstrTrans *trans = (FskSocketInstrTrans*)msgData;
	FskSocketInstrMisc *misc = (FskSocketInstrMisc*)msgData;
#if INSTR_PACKET_CONTENTS
	UInt32 s;
	const UInt32 kMessageTextSize = 127;
	char tmp[128];
#endif

	switch (msg) {
    	case kFskSocketInstrMsgCreateUDP:
    		snprintf(buffer, bufferSize, "[%s] UDP socket created", skt->debugName);
			return true;
    	case kFskSocketInstrMsgUDPRecv:
    		snprintf(buffer, bufferSize, "[%s] UDP Recv", skt->debugName);
			return true;
    	case kFskSocketInstrMsgUDPRecvd:
 #if INSTR_PACKET_CONTENTS
			s = trans->amt - 1;
			if ((s + kMessageTextSize) > bufferSize)
				s = bufferSize - kMessageTextSize;
			tmp[0] = trans->buf[s];
			trans->buf[s] = '\0';
			snprintf(buffer, bufferSize, "[%s] UDP Received %d bytes\n%s%c",
				trans->skt->debugName, trans->amt, trans->buf, tmp[0]);
			trans->buf[s] = tmp[0];
#else
	   		snprintf(buffer, bufferSize, "[%s] UDP Received %d bytes", trans->skt->debugName, trans->amt);
#endif
			return true;
    	case kFskSocketInstrMsgUDPSend:
    		snprintf(buffer, bufferSize, "[%s] UDP Send", skt->debugName);
			return true;
    	case kFskSocketInstrMsgUDPSent:
#if INSTR_PACKET_CONTENTS
			s = trans->amt - 1;
			if ((s + kMessageTextSize) > bufferSize)
				s = bufferSize - kMessageTextSize;
			tmp[0] = trans->buf[s];
			trans->buf[s] = '\0';
			snprintf(buffer, bufferSize, "[%s] UDP Sent %d bytes\n%s%c",
				trans->skt->debugName, trans->amt, trans->buf, tmp[0]);
			trans->buf[s] = tmp[0];
#else
	   		snprintf(buffer, bufferSize, "[%s] UDP Sent %d bytes", trans->skt->debugName, trans->amt);
#endif
			return true;
    	case kFskSocketInstrMsgCreateTCP:
    		snprintf(buffer, bufferSize, "[%s] TCP socket created", skt->debugName);
			return true;
    	case kFskSocketInstrMsgTCPRecv:
    		snprintf(buffer, bufferSize, "[%s] TCP Recv", skt->debugName);
			return true;
    	case kFskSocketInstrMsgTCPRecvd:
#if INSTR_PACKET_CONTENTS
			s = trans->amt - 1;
			if ((s + kMessageTextSize) > bufferSize)
				s = bufferSize - kMessageTextSize;
			tmp[0] = trans->buf[s];
			trans->buf[s] = '\0';
			snprintf(buffer, bufferSize, "[%s] TCP Received %d bytes\n%s%c",
				trans->skt->debugName, trans->amt, trans->buf, tmp[0]);
			trans->buf[s] = tmp[0];
#else
	   		snprintf(buffer, bufferSize, "[%s] TCP Received %d bytes", trans->skt->debugName, trans->amt);
#endif
			return true;
    	case kFskSocketInstrMsgTCPSend:
    		snprintf(buffer, bufferSize, "[%s] TCP Send", skt->debugName);
			return true;
    	case kFskSocketInstrMsgTCPSent:
#if INSTR_PACKET_CONTENTS
			s = trans->amt - 1;
			if ((s + kMessageTextSize) > bufferSize)
				s = bufferSize - kMessageTextSize;
			tmp[0] = trans->buf[s];
			trans->buf[s] = '\0';
			snprintf(buffer, bufferSize, "[%s] TCP Sent %d bytes\n%s%c",
				trans->skt->debugName, trans->amt, trans->buf, tmp[0]);
			trans->buf[s] = tmp[0];
#else
	   		snprintf(buffer, bufferSize, "[%s] TCP Sent %d bytes", trans->skt->debugName, trans->amt);
#endif
			return true;
    	case kFskSocketInstrMsgTCPListen:
			snprintf(buffer, bufferSize, "[%s] TCP Listen to %d.%d.%d.%d:%d",
				skt->debugName, intAs4Bytes(skt->ipaddrLocal), skt->portLocal);
			return true;
    	case kFskSocketInstrMsgTCPAccepting:
			snprintf(buffer, bufferSize, "[%s] TCP Accepting on %d.%d.%d.%d:%d",
				skt->debugName, intAs4Bytes(skt->ipaddrLocal), skt->portLocal);
			return true;
    	case kFskSocketInstrMsgTCPAccepted:
			snprintf(buffer, bufferSize, "[%s] TCP Accepted from %d.%d.%d.%d:%d",
				skt->debugName, intAs4Bytes(skt->ipaddrRemote), skt->portRemote);
			return true;
    	case kFskSocketInstrMsgTCPBeginConnect:
			snprintf(buffer, bufferSize, "[%s] begin TCP Connect to %s:%d",
				skt->debugName, skt->hostname, skt->portRemote);
			return true;
    	case kFskSocketInstrMsgTCPConnect:
    		snprintf(buffer, bufferSize, "[%s] attempting connect to %d.%d.%d.%d:%d",
				skt->debugName, intAs4Bytes(skt->ipaddrRemote), skt->portRemote);
			return true;
    	case kFskSocketInstrMsgTCPConnected:
    		snprintf(buffer, bufferSize, "[%s] TCP Connected", skt->debugName);
			return true;
    	case kFskSocketInstrMsgTCPConnectFailed:
    		snprintf(buffer, bufferSize, "[%s] TCP Connect Failed (err %d)", skt->debugName, skt->lastErr);
			return true;

		case kFskSocketInstrMsgTCPConnectedSSL:
    		snprintf(buffer, bufferSize, "SSL Socket Connected. Starting handshake");
			return true;

    	case kFskSocketInstrMsgSSLRecvd:
    		snprintf(buffer, bufferSize, "SSL Received %d bytes", trans->amt);
			return true;
    	case kFskSocketInstrMsgSSLSent:
    		snprintf(buffer, bufferSize, "SSL Sent %d bytes", trans->amt);
			return true;
    	case kFskSocketInstrMsgSSLRecvErr:
			snprintf(buffer, bufferSize, "SSL Receive Error");
			return true;
    	case kFskSocketInstrMsgSSLSendErr:
    		snprintf(buffer, bufferSize, "SSL Send Error");
			return true;
    	case kFskSocketInstrMsgSSLClosed:
    		snprintf(buffer, bufferSize, "SSL Closed");
			return true;
		case kFskSocketInstrMsgSSLConnectErr:
			snprintf(buffer, bufferSize, "SSL Connect Error");
			return true;
		case kFskSocketInstrMsgSSLStateConnect:
			snprintf(buffer, bufferSize, "starting SSL Connect");
			return true;
		case kFskSocketInstrMsgSSLServerCertFailed:
			snprintf(buffer, bufferSize, "SSL Check server cert failed. Hostname: %s", skt->hostname);
			return true;
		case kFskSocketInstrMsgSSLServerCertSuccess:
			snprintf(buffer, bufferSize, "SSL Check server cert succeeded. Hostname: %s", skt->hostname);
			return true;

    	case kFskSocketInstrMsgSetOptions:
    		snprintf(buffer, bufferSize, "[%s] SetOptions: level %d, option %d, value %d",
				misc->skt->debugName,
				misc->sktLevel, misc->sktOption, misc->val);
			return true;
    	case kFskSocketInstrMsgBind:
    		snprintf(buffer, bufferSize, "[%s] bind: %d.%d.%d.%d : %d", misc->skt->debugName,
				intAs4Bytes(misc->addr), misc->port);
			return true;
    	case kFskSocketInstrMsgMulticastJoin:
    		snprintf(buffer, bufferSize, "[%s] Multicast Join: addr: %d.%d.%d.%d, interface: %d.%d.%d.%d, ttl: %d",
				misc->skt->debugName,
				intAs4Bytes(misc->multicastAddr), intAs4Bytes(misc->interfaceAddr),
				misc->ttl);
			return true;
		case kFskSocketInstrMsgSetOutgoingInterface:
			snprintf(buffer, bufferSize, "[%s] Multicast SetOutgoingInterface: %d.%d.%d.%d",
				misc->skt->debugName,
				intAs4Bytes(misc->interfaceAddr) );
			return true;
    	case kFskSocketInstrMsgClose:
			if (skt->hostname)
	    		snprintf(buffer, bufferSize, "[%s] closing socket to %s", skt->debugName, skt->hostname);
			else
	    		snprintf(buffer, bufferSize, "[%s] closing socket", skt->debugName);
			return true;
    	case kFskSocketInstrMsgNativeErr:
    		snprintf(buffer, bufferSize, "error: nativeErr %d, Fsk err %ld - %s", native->nativeErr, (long)native->convertedErr, FskInstrumentationGetErrorString(native->convertedErr));
			return true;
	}

	return false;
}
#endif

// ---------------------------------------------------------------------
static FskErr networkGetPriority(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

// property list for dispatching
static FskMediaPropertyEntryRecord gNetworkProperties[] = {
	{ kFskNetworkPropertyPriority, kFskMediaPropertyTypeInteger, networkGetPriority, NULL },
	{ kFskMediaPropertyUndefined, kFskMediaPropertyTypeUndefined, NULL, NULL }
};

FskErr FskNetworkHasProperty(void *setToNull, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType) {
	return FskMediaHasProperty(gNetworkProperties, propertyID, get, set, dataType);
}

FskErr FskNetworkSetProperty(void *setToNull, UInt32 propertyID, FskMediaPropertyValue property) {
	return FskMediaSetProperty(gNetworkProperties, NULL, NULL, propertyID, property);
}

FskErr FskNetworkGetProperty(void *setToNull, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaGetProperty(gNetworkProperties, NULL, NULL, propertyID, property);
}

FskErr networkGetPriority(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property) {
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = sSocketHighestActivePriority;
	return kFskErrNone;
}

