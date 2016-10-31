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
#include "FskSSL.h"
#include "FskNetInterface.h"
#include "kprSocket.h"
#include "kprUtilities.h"

//--------------------------------------------------
// definitions
//--------------------------------------------------

#define kSocketReadBufferSize (32u * 1024)

typedef enum {
	kKprSocketModeUndefined = 0,
	kKprSocketModeTCP,
	kKprSocketModeTCPListen,
	kKprSocketModeUDP,

	kKprSocketModeFinished = -1,
} KprSocketMode;

typedef struct KprSocketListenerRecord KprSocketListenerRecord, *KprSocketListener;

struct KprSocketRecord {
	KprSocket next;

	KprSocketMode mode;
	KprSocketProto proto;
	Boolean secure;
	KprSocketCallbaks callbacks;
	void *refcon;
	KprSocket server; // @weak

	const char *host;
	int port;

	FskSocket socket;
	Boolean connected;
	Boolean cancelled;
	KprSocketWriter writer;

	// for TCP Listen
	KprSocketListener listeners;
	KprSocketListener currentListener;

	// Working with thread
	FskThreadDataHandler handler;
	UInt32 preventDispose;
	Boolean disposeRequested;
	UInt8 buffer[kSocketReadBufferSize];

	// SSL
	FskSocketCertificateRecord *certs;
	KprSocketSSLState sslState;
	KprSocketTLSProtocolVersion tlsProtocolVersion;
	const char *applicationProtocols;

	FskTimeRecord lastDataArrived;
};

static FskErr KprSocketOnConnect(FskSocket skt, void *refcon);
static void KprSocketOnWriteError(KprSocketErrorContext context , FskErr err, void *refcon);

static FskErr KprSocketSetupDataReader(KprSocket self);
static void KprSocketDataReader(FskThreadDataHandler handler, FskThreadDataSource source, void *refcon);

static FskErr KprSocketListenerNew(KprSocket socket, int ip, int port, KprSocketListener *it);
static FskErr KprSocketListenerDispose(KprSocketListener self);
static int KprSocketListenerGetPort(KprSocketListener self);
static FskErr KprSocketListenerListen(KprSocketListener self);
static FskErr KprSocketListenerAccept(KprSocketListener self, FskSocket *it);

static KprSocket gSockets = NULL;

//--------------------------------------------------
// KprSocket life cycle
//--------------------------------------------------

FskErr KprSocketNew(KprSocketProto proto, KprSocketFlags flags, KprSocketCallbaks *callbacks, void *refcon, KprSocket *it)
{
	FskErr err = kFskErrNone;
	KprSocket self = NULL;

	bailIfError(FskMemPtrNewClear(sizeof(KprSocketRecord), &self));

	switch (proto) {
		case kKprSocketProtoTCP: {

			break;
		}
		case kKprSocketProtoUDP: {
			bailIfError(FskNetSocketNewUDP(&self->socket, "KprSocketUDP"));
			self->mode = kKprSocketModeUDP;

			KprSocketSetupDataReader(self);
			break;
		}
		default:
			bailIfError(kFskErrParameterError);
			break;
	}

	self->proto = proto;
	self->secure = (Boolean)(flags & kKprSocketFlagsSecure);
	self->callbacks = *callbacks;
	self->refcon = refcon;
	*it = self;

	FskListAppend(&gSockets, self);

bail:
	if (err) {
		KprSocketDispose(self);
	}

	return err;
}

static FskErr KprSocketNewFromFskSocket(FskSocket skt, KprSocketFlags flags, KprSocket *it)
{
	FskErr err = kFskErrNone;
	KprSocket self = NULL;
	UInt32 ipaddr;
	char ipaddrStr[22];

	bailIfError(FskMemPtrNewClear(sizeof(KprSocketRecord), &self));

	bailIfError(FskNetSocketGetLocalAddress(skt, &ipaddr, &self->port));
	FskNetIPandPortToString(ipaddr, 0, ipaddrStr);
	self->host = FskStrDoCopy(ipaddrStr);
	bailIfNULL(self->host);

	self->mode = kKprSocketModeTCP;
	self->proto = kKprSocketProtoTCP;
	self->secure = (Boolean)(flags & kKprSocketFlagsSecure);

	bailIfError(KprSocketOnConnect(skt, self));

	*it = self;

	FskListAppend(&gSockets, self);

bail:
	if (err) {
		KprSocketDispose(self);
	}

	return err;
}

static void KprSocketCleanup(KprSocket self)
{
	FskThreadRemoveDataHandler(&self->handler);

	KprSocketWriterDispose(self->writer);
	self->writer = NULL;

	if (self->socket) {
		FskNetSocketClose(self->socket);
		self->socket = NULL;
	}

	while (self->listeners) {
		KprSocketListenerDispose(self->listeners);
	}

	self->connected = false;

	FskMemPtrDispose((char *) self->host);
	self->host = NULL;
	self->port = 0;
	self->mode = kKprSocketModeFinished;

	FskNetUtilDisposeCertificate(self->certs);

	FskMemPtrDispose((void *) self->applicationProtocols);
	self->applicationProtocols = NULL;

	memset(&self->callbacks, 0, sizeof(KprSocketCallbaks));
}

FskErr KprSocketDispose(KprSocket self)
{
	FskErr err = kFskErrNone;

	if (self) {
		memset(&self->callbacks, 0, sizeof(KprSocketCallbaks));

		if (self->preventDispose > 0) {
			self->disposeRequested = true;
		} else {
			FskListRemove(&gSockets, self);

			KprSocketCleanup(self);
			FskMemPtrDispose(self);
		}
	}
	return err;
}

void KprSocketSetupCallbaks(KprSocket self, KprSocketCallbaks *callbacks, void *refcon)
{
	self->callbacks = *callbacks;
	self->refcon = refcon;
}

static void KprSocketOnWriteError(KprSocketErrorContext context , FskErr err, void *refcon)
{
	KprSocket self = refcon;

	if (self->callbacks.error) {
		self->callbacks.error(self, err, self->refcon);
	}
}

//--------------------------------------------------
// Connect
//--------------------------------------------------

FskErr KprSocketConnect(KprSocket self, const char *host, int port)
{
	FskErr err = kFskErrNone;
	long flags = 0;

	if (self->mode != kKprSocketModeUndefined) return kFskErrBadState;

	self->host = FskStrDoCopy(host);
	bailIfNULL(self->host);

	self->port = port;
	self->mode = kKprSocketModeTCP;
	if (self->secure) {
		FskSSLOption ssl;

		flags |= kConnectFlagsSSLConnection;

		FskNetSSLOptionInitialize(self->host, self->port, flags, kFskNetSocketDefaultPriority, &ssl);

		ssl.protocolVersion = self->tlsProtocolVersion;
		ssl.applicationProtocols = self->applicationProtocols;
		bailIfError(FskNetConnectToSecureHost(&ssl, self->certs, KprSocketOnConnect, self));
	} else {
		bailIfError(FskNetConnectToHost((char *) self->host, self->port, false, KprSocketOnConnect, self, flags, NULL, "KprSocketTCP"));
	}

bail:
	if (err) {
		KprSocketCleanup(self);
	}
	return err;
}

static FskErr KprSocketOnConnect(FskSocket skt, void *refcon)
{
	FskErr err = kFskErrNone;
	KprSocket self = (KprSocket) refcon;

	if (!skt || 0 == skt->ipaddrRemote) {
		bailIfError(kFskErrNameLookupFailed);
	}

	bailIfError(skt->lastErr);

	if (self->cancelled) {
		FskNetSocketClose(skt);

		if (self->callbacks.connectCancelled) {
			self->callbacks.connectCancelled(self, self->refcon);
		}

		KprSocketCleanup(self);
		return kFskErrNone;
	}

	bailIfError(KprSocketWriterNew(&self->writer, skt, self));
	self->writer->errorCallback = KprSocketOnWriteError;

	self->socket = skt;
	skt = NULL;
	self->connected = true;

	KprSocketSetupDataReader(self);

	if (!(self->server && self->secure) && self->callbacks.connect) {
		self->callbacks.connect(self, self->refcon);
	}

bail:
	FskNetSocketClose(skt);

	if (err) {
		if (self->callbacks.error) {
			self->callbacks.error(self, err, self->refcon);
		}

		KprSocketCleanup(self);
	}
	return err;
}

FskErr KprSocketClose(KprSocket self)
{
	FskErr err = kFskErrNone;

	switch (self->mode) {
		case kKprSocketModeTCP:
		case kKprSocketModeTCPListen:
		case kKprSocketModeUDP:
			break;

		default:
			return kFskErrBadSocket;
	}

	if (self->socket == NULL) {
		self->cancelled = true;
		self->mode = kKprSocketModeFinished;
		return kFskErrNone;
	}

	if (self->callbacks.close) {
		self->callbacks.close(self, self->refcon);
		// @TODO close will only called when conncection already established.
	}

	KprSocketCleanup(self);

	return err;
}

Boolean KprSocketIsConnected(KprSocket self)
{
	return self->connected;
}

//--------------------------------------------------
// Bind and Listen
//--------------------------------------------------

struct KprSocketListenerRecord {
	KprSocketListener next; // @weak
	KprSocket owner; // @weak
	FskSocket socket;
	int ip;
	int port;
	FskThreadDataHandler dataHandler;
};

FskErr KprSocketBind(KprSocket self, int port, const char *iface)
{
	FskErr err = kFskErrNone;
	KprSocketListener listener = NULL;
	FskNetInterfaceRecord *ifc = NULL;

	if (self->port != 0) return kFskErrBadState;

	if (iface) {
		ifc = FskNetInterfaceFindByName((char *) iface);
		if (ifc == NULL) return kFskErrNotFound;

		bailIfError(KprSocketListenerNew(self, ifc->ip, port, &listener));
		if (port == 0) {
			port = KprSocketListenerGetPort(listener);
		}
	} else {
		int i, numI;

		numI = FskNetInterfaceEnumerate();
		for (i = 0; i < numI; i++) {
			int ip, status;

			err = FskNetInterfaceDescribe(i, &ifc);
			if (err) continue;
			ip = ifc->ip;
			status = ifc->status;
			FskNetInterfaceDescriptionDispose(ifc);
			ifc = NULL;

			if (status) {
				bailIfError(KprSocketListenerNew(self, ip, port, &listener));
				if (port == 0) {
					port = KprSocketListenerGetPort(listener);
				}
			}
		}
	}

	self->port = port;

bail:
	FskNetInterfaceDescriptionDispose(ifc);
	if (err) {
		KprSocketCleanup(self);
	}
	return err;
}

FskErr KprSocketListen(KprSocket self)
{
	FskErr err = kFskErrNone;
	KprSocketListener listener;

	if (self->proto != kKprSocketProtoTCP) return kFskErrBadSocket;
	if (self->handler) return kFskErrBadState;
	if (self->port == 0) return kFskErrBadState;

	listener = self->listeners;
	while (listener) {
		bailIfError(KprSocketListenerListen(listener));
		listener = listener->next;
	}

	self->mode = kKprSocketModeTCPListen;

bail:
	if (err) {
		KprSocketCleanup(self);
	}
	return err;
}

Boolean KprSocketIsListening(KprSocket self)
{
	return self->mode == kKprSocketModeTCPListen;
}

static FskErr KprSocketSSLHandshakeFinished(struct FskSocketRecord *skt, void *refCon) {
	KprSocket self = (KprSocket)refCon;
	KprSocket server = self->server;
	Boolean bornToBeSecure = (self->secure == true);

	if (skt == NULL || FskNetSocketGetLastError(skt)) {
		if (self->callbacks.error) {
			self->callbacks.error(self, kFskErrSSLHandshakeFailed, self->refcon);
		}

		return kFskErrOperationFailed;
	}

	self->secure = true;
	self->sslState = kKprSocketSSLStateEstablished;

	if (bornToBeSecure && self->callbacks.connect) {
		self->callbacks.connect(self, self->refcon);
	}

	if (server && server->callbacks.accept) {
		server->callbacks.accept(server, self, server->refcon);
	}
	return kFskErrNone;
}

static FskErr KprSocketStartTSL(KprSocket self)
{
	FskErr err;
	void *ssl = NULL;

	if (self->sslState != kKprSocketSSLStateInsecure) return kFskErrBadState;

	bailIfError(FskSSLAttach(&ssl, self->socket));

	if (self->server && self->server->certs != NULL)
		FskSSLLoadCerts(ssl, self->server->certs);

	self->sslState = kKprSocketSSLStateHandshaking;
	err = FskSSLHandshake(ssl, KprSocketSSLHandshakeFinished, self, (self->server == NULL), 0);

bail:
	if (err != kFskErrNone) {
		FskSSLDispose(ssl);
	}
	return err;
}

FskErr KprSocketAccept(KprSocket self, KprSocket *it)
{
	FskErr err = kFskErrNone;
	FskSocket skt = NULL;
	KprSocket socket = NULL;
	KprSocketFlags flags = 0;

	if (self->currentListener == NULL) return kFskErrBadSocket;

	bailIfError(KprSocketListenerAccept(self->currentListener, &skt));

	if (self->secure) flags |= kKprSocketFlagsSecure;

	bailIfError(KprSocketNewFromFskSocket(skt, flags, &socket));
	skt = NULL;
	socket->server = self;

	if (self->secure) {
		bailIfError(KprSocketStartTSL(socket));
	} else {
		if (self->callbacks.accept) {
			self->callbacks.accept(self, socket, self->refcon);
		}
	}

	*it = socket;
	socket = NULL;

bail:
	FskNetSocketClose(skt);
	KprSocketDispose(socket);

	return err;
}

FskErr KprSocketSetCertificate(KprSocket self, FskSocketCertificateRecord *certs)
{
	if (certs) {
		self->certs = FskNetUtilCopyCertificate(certs);
		if (self->certs == NULL) return kFskErrMemFull;
	}
	return kFskErrNone;
}

FskErr KprSocketSetTLSProtocolVersion(KprSocket self, KprSocketTLSProtocolVersion version)
{
	self->tlsProtocolVersion = version;
	return kFskErrNone;
}

FskErr KprSocketSetTLSApplicationProtocols(KprSocket self, const char *protocols)
{
	FskErr err = kFskErrNone;

	if (self->applicationProtocols) FskMemPtrDispose((void *) self->applicationProtocols);
	self->applicationProtocols = FskStrDoCopy(protocols);
	bailIfNULL(self->applicationProtocols);

bail:
	return err;
}


//--------------------------------------------------
// Sending and receiving data
//--------------------------------------------------

FskErr KprSocketSend(KprSocket self, FskMemPtr data, UInt32 length)
{
	FskErr err = kFskErrNone;

	if (self->writer == NULL) return kFskErrConnectionClosed;

	KprSocketWriterSendBytes(self->writer, data, length);
	return err;
}

FskErr KprSocketSendTo(KprSocket self, FskMemPtr data, UInt32 length, int ipaddr, int port)
{
	FskErr err = kFskErrNone;
	int amt = 0;

	if (self->proto != kKprSocketProtoUDP) return kFskErrBadSocket;

	if (ipaddr == 0) ipaddr = KprSocketGetRemoteAddress(self);
	if (port == 0) port = KprSocketGetRemotePort(self);

	err = FskNetSocketSendUDP(self->socket, data, length, &amt, ipaddr, port);
	return err;
}

FskErr KprSocketRecv(KprSocket self, FskMemPtr buffer, int bufferSize, int *amt)
{
	FskErr err;
	FskTimeGetNow(&self->lastDataArrived);

	switch (self->mode) {
		case kKprSocketModeTCP: {
			err = FskNetSocketRecvTCP(self->socket, buffer, bufferSize, amt);
			break;
		}

		case kKprSocketModeUDP: {
			err = FskNetSocketRecvUDP(self->socket, buffer, bufferSize, amt, NULL, NULL);
			break;
		}

		default:
			err = kFskErrBadSocket;
			break;
	}
	return err;
}

static FskErr KprSocketSetupDataReader(KprSocket self)
{
	if (self->handler) return kFskErrBadState;

	FskThreadAddDataHandler(&self->handler, (FskThreadDataSource)self->socket, KprSocketDataReader, true, false, self);

	return kFskErrNone;
}

static void KprSocketDataReader(FskThreadDataHandler handler UNUSED, FskThreadDataSource source UNUSED, void *refcon)
{
	FskErr err = kFskErrNone;
	KprSocket self = refcon;

	self->preventDispose++;

	if (self->callbacks.dataAvailable) {
		err = self->callbacks.dataAvailable(self, self->refcon);
	} else {
		while (err == kFskErrNone && !self->disposeRequested) {
			int length = 0;

			err = KprSocketRecv(self, self->buffer, kSocketReadBufferSize, &length);
			if (err == kFskErrNone && length > 0 && self->callbacks.data) {
				self->callbacks.data(self, (FskMemPtr) self->buffer, length, self->refcon);
			}
		}
	}

	switch (err) {
		case kFskErrNone:
		case kFskErrNoData:
			break;

		case kFskErrConnectionClosed:
			if (self->callbacks.disconnect && self->mode == kKprSocketModeTCP) {
				self->callbacks.disconnect(self, self->refcon);
			}
			KprSocketCleanup(self);
			break;

		default:
			// report error.
			if (self->callbacks.error) {
				self->callbacks.error(self, err, self->refcon);
			}
			break;
	}

	self->preventDispose--;

	if (self->disposeRequested) {
		KprSocketDispose(self);
	}
}

//--------------------------------------------------
// Socket information
//--------------------------------------------------

KprSocketProto KprSocketGetProto(KprSocket self)
{
	return self->proto;
}

UInt32 KprSocketGetAddress(KprSocket self)
{
	FskErr err;
	UInt32 ipaddr;

	if (self->socket == NULL) return 0;

	err = FskNetSocketGetLocalAddress(self->socket, &ipaddr, NULL);
	if (err != kFskErrNone) return 0L;
	return ipaddr;
}

int KprSocketGetPort(KprSocket self)
{
	FskErr err;
	int port;

	if (self->listeners) {
		return KprSocketListenerGetPort(self->listeners);
	}

	if (self->socket == NULL) return 0;

	err = FskNetSocketGetLocalAddress(self->socket, NULL, &port);
	if (err != kFskErrNone) return 0L;
	return port;
}

UInt32 KprSocketGetRemoteAddress(KprSocket self)
{
	FskErr err;
	UInt32 ipaddr;

	if (self->socket == NULL) return 0;

	err = FskNetSocketGetRemoteAddress(self->socket, &ipaddr, NULL);
	if (err != kFskErrNone) return 0L;
	return ipaddr;
}

int KprSocketGetRemotePort(KprSocket self)
{
	FskErr err;
	int port;

	if (self->socket == NULL) return 0;

	err = FskNetSocketGetRemoteAddress(self->socket, NULL, &port);
	if (err != kFskErrNone) return 0L;
	return port;
}

void KprSocketSetRemoteAddress(KprSocket self, UInt32 addr)
{
	if (self->socket) {
		self->socket->ipaddrRemote = addr;
	}
}

void KprSocketSetRemotePort(KprSocket self, int port)
{
	if (self->socket) {
		self->socket->portRemote = port;
	}
}

//--------------------------------------------------
// Listener
//--------------------------------------------------

static void KprSocketListenerNewConnection(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);

static FskErr KprSocketListenerNew(KprSocket owner, int ip, int port, KprSocketListener *it)
{
	KprSocketListener self = NULL;
	FskErr err;
	FskSocket skt = NULL;

	bailIfError(KprMemPtrNewClear(sizeof(KprSocketListenerRecord), &self));

	bailIfError(FskNetSocketNewTCP(&skt, true, "KprSocketListener"));

	FskNetSocketReuseAddress(skt);
	FskNetSocketMakeNonblocking(skt);

	bailIfError(FskNetSocketBind(skt, ip, port));
	bailIfError(FskNetSocketGetLocalAddress(skt, NULL, &port));

	self->owner = owner;
	self->ip = ip;
	self->port = port;
	self->socket = skt;
	skt = NULL;

	FskListAppend(&owner->listeners, self);
	if (it) *it = self;

bail:
	FskNetSocketClose(skt);

	if (err) {
		KprSocketListenerDispose(self);
	}

	return err;
}

static FskErr KprSocketListenerDispose(KprSocketListener self)
{
	if (self) {
		FskThreadRemoveDataHandler(&self->dataHandler);
		FskNetSocketClose(self->socket);

		if (self->owner) {
			FskListRemove(&self->owner->listeners, self);
		}

		KprMemPtrDispose(self);
	}
	return kFskErrNone;
}

static int KprSocketListenerGetPort(KprSocketListener self)
{
	return self->port;
}

static FskErr KprSocketListenerListen(KprSocketListener self)
{
	FskErr err;

	bailIfError(FskNetSocketListen(self->socket));

	FskThreadAddDataHandler(&self->dataHandler, (FskThreadDataSource)self->socket, (FskThreadDataReadyCallback)KprSocketListenerNewConnection, true, false, self);

bail:
	return err;
}

static FskErr KprSocketListenerAccept(KprSocketListener self, FskSocket *it)
{
	return FskNetAcceptConnection(self->socket, it, "KprSocketListener");
}

static void KprSocketListenerNewConnection(FskThreadDataHandler handler UNUSED, FskThreadDataSource source UNUSED, void *refcon) {
	KprSocketListener self = refcon;
	FskErr err = kFskErrNone;
	KprSocket socket = self->owner;

	socket->preventDispose++;
	socket->currentListener = self;

	if (socket->callbacks.connect) {
		socket->callbacks.connect(socket, socket->refcon);
	} else if (socket->callbacks.accept) {
		KprSocket client = NULL;

		bailIfError(KprSocketAccept(socket, &client));
		socket->callbacks.accept(socket, client, socket->refcon);

		if (client->callbacks.connect) {
			client->callbacks.connect(client, client->refcon);
		}
	} else {
		err = kFskErrConnectionRefused;
	}

bail:
	if (err) {
		if (socket->callbacks.error) {
			socket->callbacks.error(socket, err, socket->refcon);
		}
	}

	socket->currentListener = NULL;
	socket->preventDispose--;

	if (socket->disposeRequested) {
		KprSocketDispose(socket);
	}
}

