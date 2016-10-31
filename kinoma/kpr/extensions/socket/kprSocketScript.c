/*
 *     Copyright (C) 2002-2015 Kinoma, Inc.
 *
 *     All rights reserved.
 *
 *		Author: Basuke Suzuki
 */

#include "xs.h"

#define __FSKNETUTILS_PRIV__

#include "FskNetUtils.h"

#include "kprSocket.h"
#include "kprShell.h"

//--------------------------------------------------
// Socket
//--------------------------------------------------

typedef struct KPR_SocketRecord KPR_SocketRecord, *KPR_SocketData;

struct KPR_SocketRecord {
	xsMachine* the;
	xsSlot slot;

	KprSocket socket;
};

#define DEFER3(xxx, a, b, c) FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)xxx, a, b, c, NULL)
#define DEFER2(xxx, a, b) DEFER3(xxx, a, b, NULL)
#define DEFER1(xxx, a) DEFER3(xxx, a, NULL, NULL)
#define DEFER0(xxx) DEFER3(xxx, NULL, NULL, NULL)

static void KPR_Socket_deferredConnect(void *a, void *b, void *c, void *d);

static void KPR_Socket_onConnect(KprSocket socket, void *refcon);
static void KPR_Socket_onConnectCancel(KprSocket socket, void *refcon);
static void KPR_Socket_onClose(KprSocket socket, void *refcon);
static FskErr KPR_Socket_onDataAvailable(KprSocket socket, void *refcon);
static void KPR_Socket_onError(KprSocket socket, FskErr err, void *refcon);

static void KPR_Socket_deferredResolv(void *a, void *b, void *c, void *d);
static void KPR_Socket_onResolved(FskResolver rr);

static void KPR_Socket_setupReadBuffer(xsMachine *the, xsSlot *slot);
static FskErr KPR_Socket_loadReadableBytes(xsMachine *the);

#define xsSocket xsGet(xsGlobal, xsID("Socket"))
#define xsSocketPrototype xsGet(xsSocket, xsID("prototype"))
#define xsSocketConstructorID xsID("socketConstructor")
#define xsReadBufferID xsID("_buffer")
#define xsThisReadBuffer xsGet(xsThis, xsReadBufferID)

/* constructor */
void KPR_Socket_constructor(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KPR_SocketData socketH = NULL;
	KprSocketProto proto;
	const char *host = NULL;
	int port = 0;
	Boolean secure = false;
	KprSocketTLSProtocolVersion tlsVersion = 0;
	const char *applicationProtocols = NULL;
	KprSocketFlags flags = 0;
	KprSocket socket = NULL;
	KprSocketCallbaks callbacks = {
		KPR_Socket_onConnect,
		KPR_Socket_onConnectCancel,
		NULL,
		KPR_Socket_onClose,

		KPR_Socket_onDataAvailable,
		NULL,

		NULL,

		KPR_Socket_onError
	};

	xsVars(1);

	KPR_Socket_setupReadBuffer(the, &xsThis);

	proto = xsToInteger(xsArg(0));
	host = xsTest(xsArg(1)) ? xsToString(xsArg(1)) : NULL;
	port = xsToInteger(xsArg(2));

	if (xsTest(xsArg(3))) {
		secure = true;

		xsVar(0) = xsGet(xsArg(3), xsID("protocolVersion"));
		if (xsTest(xsVar(0))) {
			tlsVersion = xsToInteger(xsVar(0));
		}

		xsVar(0) = xsGet(xsArg(3), xsID("applicationLayerProtocolNegotiation"));
		if (xsTest(xsVar(0))) {
			applicationProtocols = xsToString(xsVar(0));
		}
	} else {
		secure = false;
	}

	if (secure) flags |= kKprSocketFlagsSecure;

	bailIfError(FskMemPtrNewClear(sizeof(KPR_SocketRecord), &socketH));
	bailIfError(KprSocketNew(proto, flags, &callbacks, socketH, &socket));

	if (tlsVersion) KprSocketSetTLSProtocolVersion(socket, tlsVersion);
	if (applicationProtocols) bailIfError(KprSocketSetTLSApplicationProtocols(socket, applicationProtocols));

	socketH->the = the;
	socketH->slot = xsThis;
	socketH->socket = socket;
	xsSetHostData(xsThis, socketH);

	if (host && port) {
		if (proto == kKprSocketProtoTCP) {
			DEFER3(KPR_Socket_deferredConnect, (void *) socketH, (void *) host, (void *) port);
			xsRemember(socketH->slot);
		} else if (proto == kKprSocketProtoUDP) {
			KprSocketSetRemotePort(socket, port);

			DEFER2(KPR_Socket_deferredResolv, (void *) socketH, (void *) host);
			xsRemember(socketH->slot);
		}
	}

bail:
	if (err) {
		KprSocketDispose(socket);
		FskMemPtrDispose(socketH);
		xsThrowIfFskErr(err);
	}
}

/* constructor */
void KPR_Socket_initWithTCPSocket(xsMachine *the, xsSlot *slot, KprSocket socket)
{
	FskErr err = kFskErrNone;
	KPR_SocketData socketH = NULL;
	KprSocketCallbaks callbacks = {
		KPR_Socket_onConnect,
		KPR_Socket_onConnectCancel,
		NULL,
		KPR_Socket_onClose,

		KPR_Socket_onDataAvailable,
		NULL,

		NULL,

		KPR_Socket_onError
	};

	bailIfError(FskMemPtrNewClear(sizeof(KPR_SocketRecord), &socketH));

	KPR_Socket_setupReadBuffer(the, slot);

	socketH->the = the;
	socketH->slot = *slot;
	socketH->socket = socket;
	xsSetHostData(*slot, socketH);
	KprSocketSetupCallbaks(socket, &callbacks, socketH);

bail:
	if (err) {
		FskMemPtrDispose(socketH);
		xsThrowIfFskErr(err);
	}
}

/* destructor */
void KPR_Socket_destructor(void *it)
{
	if (it) {
		KPR_SocketData socketH = it;

		KprSocketDispose(socketH->socket);
		FskMemPtrDispose(socketH);
	}
}

void KPR_Socket_close(xsMachine *the)
{
	KPR_SocketData socketH = xsGetHostData(xsThis);
	FskErr err = kFskErrNone;

	bailIfError(KprSocketClose(socketH->socket));

bail:
	if (err) {
		xsThrowIfFskErr(err);
	}
}

static FskErr KPR_Socket_sendData(KPR_SocketData socketH, void *data, UInt32 length, int ipaddr, int port)
{
	KprSocket socket = socketH->socket;
	KprSocketProto proto = KprSocketGetProto(socket);
	FskErr err = kFskErrNone;

	if (ipaddr && port) {
		bailIfError(KprSocketSendTo(socket, data, length, ipaddr, port));
	} else {
		if (proto == kKprSocketProtoTCP) {
			bailIfError(KprSocketSend(socket, data, length));
		} else if (proto == kKprSocketProtoUDP) {
			bailIfError(KprSocketSendTo(socket, data, length, 0L, 0L));
		} else {
			bailIfError(kFskErrBadSocket);
		}
	}

bail:
	return err;
}

static FskErr KPR_Socket_sendElement(xsMachine *the, xsSlot slot, int ipaddr, int port)
{
	KPR_SocketData socketH = xsGetHostData(xsThis);
	FskErr err = kFskErrNone;
	void *buffer = NULL;
	void *data;
	UInt32 length;
	UInt8 c;

	if (xsTypeOf(slot) == xsStringType) {
		data = xsToString(slot);
		length = FskStrLen(data);
	} else if (xsTypeOf(slot) == xsIntegerType) {
		c = xsToInteger(slot);
		data = &c;
		length = 1;
	} else if (xsIsInstanceOf(slot, xsArrayPrototype)) {
		int len = xsToInteger(xsGet(slot, xsID("length"))), i;

		for (i = 0; i < len; i++) {
			err = KPR_Socket_sendElement(the, xsGetAt(slot, xsInteger(i)), ipaddr, port);
			if (err) return err;
		}

		return err;
	} else {
		length = xsGetArrayBufferLength(slot);

		bailIfError(FskMemPtrNew(length, &buffer));
		xsGetArrayBufferData(slot, 0, buffer, length);
		data = buffer;
	}

	bailIfError(KPR_Socket_sendData(socketH, data, length, ipaddr, port));

bail:
	FskMemPtrDispose(buffer);

	return err;
}

void KPR_Socket_send(xsMachine *the)
{
	KPR_SocketData socketH = xsGetHostData(xsThis);
	KprSocket socket = socketH->socket;
	KprSocketProto proto = KprSocketGetProto(socket);
	int argc = xsToInteger(xsArgc);
	FskErr err = kFskErrNone;
	int ipaddr = 0, port = 0;

	if (argc > 1) {
		char *ipaddrStr;

		if (proto == kKprSocketProtoTCP) {
			bailIfError(kFskErrInvalidParameter);
		}

		ipaddrStr = xsToString(xsArg(1));
		if (argc > 2) {
			port = xsToInteger(xsArg(2));
		}
		if (port == 0) {
			FskNetStringToIPandPort(ipaddrStr, &ipaddr, &port);
		} else {
			FskNetStringToIPandPort(ipaddrStr, &ipaddr, NULL);
		}
	}

	bailIfError(KPR_Socket_sendElement(the, xsArg(0), ipaddr, port));

bail:
	xsThrowIfFskErr(err);
}

void KPR_Socket_recv(xsMachine *the)
{
	FskErr err = kFskErrNone;
	int argc = xsToInteger(xsArgc);
	Boolean hasBuffer = (argc > 1 && xsTest(xsArg(1)));
	int size, len;

	if (argc == 0) {
		size = 0;
	} else {
		size = xsToInteger(xsArg(0));
	}

	if (size < 0) bailIfError(kFskErrParameterError);

	len = xsGetArrayBufferLength(xsThisReadBuffer);

	if (len == 0) {
		xsResult = xsNull;
		goto bail;
	}

	if (!hasBuffer && size == 0) {
		// special case. no size, no buffer, return everything.
		xsResult = xsThisReadBuffer;
		KPR_Socket_setupReadBuffer(the, &xsThis);
		goto bail;
	}

	// prepare result buffer
	if (hasBuffer) {
		xsResult = xsArg(1);
		xsTry {
			int len2 = xsGetArrayBufferLength(xsResult);
			if (size == 0 || size > len2) size = len2;
		}
		xsCatch {
			err = kFskErrParameterError;
		}
		bailIfError(err);

		if (size > len) {
			size = len;
		}

		xsSetArrayBufferLength(xsResult, size);

		if (size == 0) goto bail;
	} else {
		if (size > len) size = len;
		xsResult = xsArrayBuffer(0, size);
	}

	{
		// copy bytes to result buffer
		void *src, *dest;

		src = xsToArrayBuffer(xsThisReadBuffer);
		dest = xsToArrayBuffer(xsResult);
		FskMemCopy(dest, src, size);
	}

	{
		// remove copied bytes
		xsSet(xsThis, xsReadBufferID, xsCall1(xsThisReadBuffer, xsID("slice"), xsInteger(size)));
	}

bail:
	xsThrowIfFskErr(err);
}

void KPR_Socket_get_connected(xsMachine *the)
{
	KPR_SocketData socketH = xsGetHostData(xsThis);
	Boolean connected = KprSocketIsConnected(socketH->socket);
	xsResult = xsBoolean(connected);
}

void KPR_Socket_get_addr(xsMachine *the)
{
	KPR_SocketData socketH = xsGetHostData(xsThis);
	UInt32 result = KprSocketGetAddress(socketH->socket);
	if (result) {
		char buffer[22];
		FskNetIPandPortToString(result, 0, buffer);
		xsResult = xsString(buffer);
	} else {
		xsResult = xsUndefined;
	}
}

void KPR_Socket_get_port(xsMachine *the)
{
	KPR_SocketData socketH = xsGetHostData(xsThis);
	int result  = KprSocketGetPort(socketH->socket);
	if (result) {
		xsResult = xsInteger(result);
	} else {
		xsResult = xsUndefined;
	}
}

void KPR_Socket_get_peerAddr(xsMachine *the)
{
	KPR_SocketData socketH = xsGetHostData(xsThis);
	UInt32 result = KprSocketGetRemoteAddress(socketH->socket);
	if (result) {
		char buffer[22];
		FskNetIPandPortToString(result, 0, buffer);
		xsResult = xsString(buffer);
	} else {
		xsResult = xsUndefined;
	}
}

void KPR_Socket_get_peerPort(xsMachine *the)
{
	KPR_SocketData socketH = xsGetHostData(xsThis);
	int result  = KprSocketGetRemotePort(socketH->socket);
	if (result) {
		xsResult = xsInteger(result);
	} else {
		xsResult = xsUndefined;
	}
}

void KPR_Socket_get_peer(xsMachine *the)
{
	KPR_SocketData socketH = xsGetHostData(xsThis);
	UInt32 ipaddr = KprSocketGetRemoteAddress(socketH->socket);
	int port  = KprSocketGetRemotePort(socketH->socket);
	if (ipaddr && port) {
		char buffer[22];
		FskNetIPandPortToString(ipaddr, port, buffer);
		xsResult = xsString(buffer);
	} else {
		xsResult = xsUndefined;
	}
}

void KPR_Socket_get_bytesAvailable(xsMachine *the)
{
	xsResult = xsInteger(xsGetArrayBufferLength(xsThisReadBuffer));
}

void KPR_Socket_get_bytesWritable(xsMachine *the)
{
	xsResult = xsInteger(32 * 1024);
}

static void KPR_Socket_deferredConnect(void *a, void *b, void *c, void *d)
{
	FskErr err;
	KPR_SocketData socketH = (KPR_SocketData) a;
	char *host = (char *) b;
	int port = (int) c;

	xsBeginHost(socketH->the);
	xsForget(socketH->slot);
	xsEndHost();

	bailIfError(KprSocketConnect(socketH->socket, host, port));

bail:
	if (err) {
		KPR_Socket_onError(socketH->socket, err, socketH);
	}
}

static void KPR_Socket_onConnect(KprSocket socket, void *refcon)
{
	KPR_SocketData socketH = refcon;

	xsBeginHost(socketH->the);

	xsThis = socketH->slot;

	xsTry {
		if (xsFindResult(xsThis, xsID("onConnect"))) {
			(void) xsCallFunction0(xsResult, xsThis);
		}
	}
	xsCatch {
	}

	xsEndHost();
}

static void KPR_Socket_onConnectCancel(KprSocket socket, void *refcon)
{
}

static void KPR_Socket_onClose(KprSocket socket, void *refcon)
{
	KPR_SocketData socketH = refcon;

	xsBeginHost(socketH->the);

	xsThis = socketH->slot;

	{
		xsTry {
			if (xsFindResult(xsThis, xsID("onClose"))) {
				(void) xsCallFunction0(xsResult, xsThis);
			}
		}
		xsCatch {
		}
	}

	xsEndHost();
}

static FskErr KPR_Socket_onDataAvailable(KprSocket socket, void *refcon)
{
	KPR_SocketData socketH = refcon;
	FskErr err = kFskErrNone;
	int len;

	xsBeginHost(socketH->the);

	xsThis = socketH->slot;
	xsVars(1);

	xsTry {
		err = KPR_Socket_loadReadableBytes(the);

		xsVar(0) = xsThisReadBuffer;

		len = xsGetArrayBufferLength(xsVar(0));

		if (len > 0) {
			xsCall1(xsThis, xsID("onMessage"), xsInteger(len));
		}
	}
	xsCatch {
		err = kFskErrScript;
	}

	xsEndHost();
	return err;
}

static void KPR_Socket_onError(KprSocket socket, FskErr err, void *refcon)
{
	KPR_SocketData socketH = refcon;

	xsBeginHost(socketH->the);

	xsThis = socketH->slot;
	xsVars(1);

	{
		xsTry {
			int callbak = xsID("onError");

			if (xsFindResult(xsThis, callbak)) {
				xsVar(0) = xsInteger(err);

				(void) xsCallFunction1(xsResult, xsThis, xsVar(0));
			}
		}
		xsCatch {
		}
	}

	xsEndHost();
}

static void KPR_Socket_deferredResolv(void *a, void *b, void *c, void *d)
{
	FskErr err = kFskErrNone;
	KPR_SocketData socketH = (KPR_SocketData) a;
	char *host = (char *) b;
	FskResolver rr = NULL;

	xsBeginHost(socketH->the);
	xsForget(socketH->slot);
	xsEndHost();

	bailIfError(FskNetHostnameResolveQTAsync(host, 0, KPR_Socket_onResolved, socketH, &rr));
	// after this call, callback has been called if host is quad dotted address.
	// don't touch resolvH because it may already disposed.

bail:
	if (err) {
		FskResolverDispose(rr);
		KPR_Socket_onError(socketH->socket, err, socketH);
	}
}

static void KPR_Socket_onResolved(FskResolver rr)
{
	KPR_SocketData socketH = (KPR_SocketData) rr->ref;

	xsBeginHost(socketH->the);

	xsThis = socketH->slot;

	{
		xsTry {
			if (rr->err) {
				KPR_Socket_onError(socketH->socket, rr->err, socketH);
			} else {
				KprSocketSetRemoteAddress(socketH->socket, rr->resolvedIP);

				if (xsFindResult(xsThis, xsID("onConnect"))) {
					(void) xsCallFunction0(xsResult, xsThis);
				}
			}
		}
		xsCatch {
		}
	}

	xsEndHost();
}

static void KPR_Socket_setupReadBuffer(xsMachine *the, xsSlot *slot)
{
	xsSet(*slot, xsReadBufferID, xsArrayBuffer(NULL, 0));
}

static FskErr KPR_Socket_loadReadableBytes(xsMachine *the)
{
	KPR_SocketData socketH = xsGetHostData(xsThis);
	FskErr err = kFskErrNone;
	int size = 1024;

	xsResult = xsThisReadBuffer;

	while (true) {
		UInt8 *p;
		int amt, pos;

		pos = xsGetArrayBufferLength(xsResult);
		xsSetArrayBufferLength(xsResult, pos + size);

		p = (UInt8 *) xsToArrayBuffer(xsResult) + pos;

		err = KprSocketRecv(socketH->socket, p, size, &amt);
		if (err) {
			xsSetArrayBufferLength(xsResult, pos);
			break;
		}

		if (amt > 0 && amt != size) {
			xsSetArrayBufferLength(xsResult, pos + amt);
		} else {
			size *= 2;
		}
	}

	xsResult = xsUndefined;

	if (err != kFskErrNoData) {
		return err;
	} else {
		return kFskErrNone;
	}
}

#define __KPRHTTPSERVER_PRIV__
#include "kprHTTPServer.h"

static void KPR_Socket_readCertificateRecord(xsMachine *the, xsSlot slot, FskSocketCertificateRecord *certs)
{
	FskMemSet(certs, 0, sizeof(FskSocketCertificateRecord));

	if (xsIsInstanceOf(slot, xsObjectPrototype)) {
		if (xsHas(slot, xsID("certificates"))) {
			certs->certificates = (void*)xsToString(xsGet(slot, xsID("certificates")));
			certs->certificatesSize = FskStrLen(certs->certificates);
		}
		if (xsHas(slot, xsID("policies"))) {
			certs->policies = xsToString(xsGet(slot, xsID("policies")));
		}
		if (xsHas(slot, xsID("hostname"))) {
			certs->hostname = xsToString(xsGet(slot, xsID("hostname")));
		}
		if (xsHas(slot, xsID("key"))) {
			certs->key = (void*)xsToString(xsGet(slot, xsID("key")));
			certs->keySize = FskStrLen(certs->key);
		}
	}

	if (certs) {
		if (!certs->certificates) {
			certs->certificates = (void*)kKprHTTPServerDefaultCretificates;
			certs->certificatesSize = FskStrLen(kKprHTTPServerDefaultCretificates);
		}
		if (!certs->key) {
			certs->key = (void*)kKprHTTPServerDefaultKey;
			certs->keySize = FskStrLen(kKprHTTPServerDefaultKey);
		}
	}
}

//--------------------------------------------------
// ListeningSocket
//--------------------------------------------------

void KPR_ListeningSocket_constructor(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KPR_SocketData socketH = NULL;
	int argc = xsToInteger(xsArgc);
	KprSocketProto proto;
	const char *protoP;
	int port;
	Boolean secure = false;
	KprSocketFlags flags = 0;
	KprSocket socket = NULL;
	KprSocketCallbaks callbacks = {
		KPR_Socket_onConnect,
		NULL,
		KPR_Socket_onClose,
		NULL,

		KPR_Socket_onDataAvailable,
		NULL,

		NULL, // KPR_ListeningSocket_onAccept,

		KPR_Socket_onError
	};


	if (argc != 1) {
		xsThrowIfFskErr(kFskErrInvalidParameter);
	}

	port = xsToInteger(xsGet(xsArg(0), xsID("port")));
	secure = xsToBoolean(xsGet(xsArg(0), xsID("secure")));
	protoP = xsTest(xsGet(xsArg(0), xsID("proto"))) ? xsToString(xsGet(xsArg(0), xsID("proto"))) : NULL;

	if (protoP == NULL) {
		proto = kKprSocketProtoTCP;
	} else if (FskStrCompareCaseInsensitive(protoP, "tcp") == 0) {
		proto = kKprSocketProtoTCP;
	} else if (FskStrCompareCaseInsensitive(protoP, "udp") == 0) {
		proto = kKprSocketProtoUDP;
	} else {
		xsThrowIfFskErr(kFskErrInvalidParameter);
	}

	if (secure) {
		flags |= kKprSocketFlagsSecure;
	}

	bailIfError(FskMemPtrNewClear(sizeof(KPR_SocketRecord), &socketH));
	bailIfError(KprSocketNew(proto, flags, &callbacks, socketH, &socket));

	if (secure) {
		FskSocketCertificateRecord certs;

		KPR_Socket_readCertificateRecord(the, xsArg(0), &certs);
		bailIfError(KprSocketSetCertificate(socket, &certs));
	}

	xsSet(xsThis, xsSocketConstructorID, xsSocket);
	KPR_Socket_setupReadBuffer(the, &xsThis);

	bailIfError(KprSocketBind(socket, port, NULL));

	if (KprSocketGetProto(socket) == kKprSocketProtoTCP) {
		bailIfError(KprSocketListen(socket));
	}

	socketH->the = the;
	socketH->slot = xsThis;
	socketH->socket = socket;
	xsSetHostData(xsThis, socketH);

bail:
	if (err) {
		KprSocketDispose(socket);
		FskMemPtrDispose(socketH);
		xsThrowIfFskErr(err);
	}
}

/*
- accept() -> Socket
	create new Socket instance and return it
- accept(socket : Socket)
	use socket and replace its host data with newly created socket
 */
void KPR_ListeningSocket_accept(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KPR_SocketData socketH = xsGetHostData(xsThis);
	KprSocket newConnection = NULL;
	int argc = xsToInteger(xsArgc);

	xsVars(1);
	// xsVar(0) -> Socket instance

	bailIfError(KprSocketAccept(socketH->socket, &newConnection));

	if (argc == 0) {
		xsVar(0) = xsNew0(xsThis, xsSocketConstructorID);
		if (!xsIsInstanceOf(xsVar(0), xsSocketPrototype)) {
			bailIfError(kFskErrInvalidParameter);
		}
	} else if (xsIsInstanceOf(xsArg(0), xsSocketPrototype)) {
		KPR_SocketData otherH;

		xsVar(0) = xsArg(0);
		otherH = xsGetHostData(xsVar(0));

		if (otherH) {
			KPR_Socket_destructor(otherH);
			xsSetHostData(xsVar(0), NULL);
		}
	} else {
		bailIfError(kFskErrParameterError);
	}

	KPR_Socket_initWithTCPSocket(the, &xsVar(0), newConnection);
	newConnection = NULL;

	xsResult = xsVar(0);

bail:
	KprSocketDispose(newConnection);
	xsThrowIfFskErr(err);
}

//--------------------------------------------------
// Resolv
//--------------------------------------------------

typedef struct KPR_SocketResolvRecord KPR_SocketResolvRecord, *KPR_SocketResolvData;

struct KPR_SocketResolvRecord {
	xsMachine* the;

	xsSlot callback;
};

static void KPR_Socket_resolv_callback(FskResolver rr);

void KPR_Socket_resolv(xsMachine *the)
{
	FskErr err = kFskErrNone;
	KPR_SocketResolvData resolvH = NULL;
	FskResolver rr = NULL;
	int argc = xsToInteger(xsArgc);
	char *host;

	if (argc != 2) {
		xsThrowIfFskErr(kFskErrInvalidParameter);
	}

	host = xsToString(xsArg(0));

	bailIfError(FskMemPtrNew(sizeof(KPR_SocketResolvRecord), &resolvH));
	resolvH->the = the;
	resolvH->callback = xsArg(1);

	xsRemember(resolvH->callback);

	bailIfError(FskNetHostnameResolveQTAsync(host, 0, KPR_Socket_resolv_callback, resolvH, &rr));
	// after this call, callback has been called if host is quad dotted address.
	// don't touch resolvH because it may already disposed.

bail:
	if (err) {
		FskResolverDispose(rr);
		xsThrowIfFskErr(err);
	}
}

static void KPR_Socket_resolv_callback(FskResolver rr)
{
	KPR_SocketResolvData resolvH = (KPR_SocketResolvData) rr->ref;

	xsBeginHost(resolvH->the);

	xsVars(1);

	{
		xsTry {
			if (rr->err) {
				xsVar(0) = xsUndefined;
			} else {
				char ipaddr[22];
				FskNetIPandPortToString(rr->resolvedIP, 0, ipaddr);
				xsVar(0) = xsString(ipaddr);
			}
			(void) xsCallFunction1(resolvH->callback, xsThis, xsVar(0));
		}
		xsCatch {
		}

		xsForget(resolvH->callback);
	}

	xsEndHost();

	FskMemPtrDispose(resolvH);
}

//--------------------------------------------------
// CoAP Function Test
//--------------------------------------------------

#if defined(RUN_UNITTEST) && RUN_UNITTEST

#include "kunit.h"

ku_main();
ku_test(CoAP_message);

void KPR_CoAP_test()
{
	ku_begin();
	ku_run(CoAP_message);
	ku_finish();
}

#endif

//--------------------------------------------------
// CoAP Library Interface
//--------------------------------------------------


FskErr kprSocket_fskLoad(FskLibrary library)
{
#if defined(RUN_UNITTEST) && RUN_UNITTEST
	KPR_CoAP_test();
#endif
	return kFskErrNone;
}

FskErr kprSocket_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

static Boolean KPR_CoAP_defineEntry(int index, int value, const char *symbol, void *refcon1, void *refcon2)
{
	xsMachine *the = (xsMachine *) refcon1;

	int sym_id = xsID((char *) symbol);
	xsNewHostProperty(xsResult, sym_id, xsInteger(value), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	return false;
}

void KPR_Socket_patch(xsMachine* the)
{
}


