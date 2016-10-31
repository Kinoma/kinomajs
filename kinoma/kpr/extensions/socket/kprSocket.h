/*
 *     Copyright (C) 2002-2015 Kinoma, Inc.
 *
 *     All rights reserved.
 *
 *		Author: Basuke Suzuki
 */

#ifndef __KPRSOCKET__
#define __KPRSOCKET__

#include "kpr.h"
#include "FskHeaders.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	//--------------------------------------------------
	// KprSocket
	//--------------------------------------------------

	typedef struct KprSocketRecord KprSocketRecord, *KprSocket;
	typedef struct KprSocketCallbaks KprSocketCallbaks;

	typedef void (KprSocketGeneralCallback)(KprSocket socket, void *refcon);
	typedef void (KprSocketGeneralErrorCallback)(KprSocket socket, FskErr err, void *refcon);
	typedef FskErr (KprSocketDataAvailableCallback)(KprSocket socket, void *refcon);
	typedef void (KprSocketDataCallback)(KprSocket socket, FskMemPtr buffer, UInt32 length, void *refcon);

	typedef void (KprSocketAcceptCallback)(KprSocket socket, KprSocket newConnection, void *refcon);

	/*
	 * note for callbacks:
	 *
	 * For TCP Socket:
	 *   1. Optional: `connect`, `connectCancelled`, `close`, `error`
	 *   2a. If `dataAvailable` exists, it will be called.
	 *   2b. Else`data` will be called if it exists.
	 *
	 * For TCP Listening Socket:
	 *   1. Optional: `close`, `error`
	 *   2a. If `connect` exists, it will be called.
	 *   2b. Else `accept` will be called, if it exists, with automatically
	 *       accepted KprSocket instance.
	 *
	 * For UDP Socket:
	 *   1. Optional: `close`, `error`
	 *   2a. If `dataAvailable` exists, it will be called.
	 *   2b. Else`data` will be called if it exists.
	 */
	struct KprSocketCallbaks {
		KprSocketGeneralCallback *connect;
		KprSocketGeneralCallback *connectCancelled;
		KprSocketGeneralCallback *close;
		KprSocketGeneralCallback *disconnect;

		KprSocketDataAvailableCallback *dataAvailable;
		KprSocketDataCallback *data;

		KprSocketAcceptCallback *accept;

		KprSocketGeneralErrorCallback *error;
	};

	typedef enum {
		kKprSocketProtoTCP = 1,
		kKprSocketProtoUDP,
	} KprSocketProto;

	typedef enum {
		kKprSocketFlagsSecure = 1,
	} KprSocketFlags;

	typedef enum {
		kKprSocketSSLStateInsecure = 0,
		kKprSocketSSLStateHandshaking,
		kKprSocketSSLStateEstablished,
	} KprSocketSSLState;

	typedef enum {
		kKprTLSProtocolVersionTLS_1_0 = kFskSSLProtocolVersionTLS_1_0,
		kKprTLSProtocolVersionTLS_1_1 = kFskSSLProtocolVersionTLS_1_1,
		kKprTLSProtocolVersionTLS_1_2 = kFskSSLProtocolVersionTLS_1_2,
	} KprSocketTLSProtocolVersion;

	FskAPI(FskErr) KprSocketNew(KprSocketProto proto, KprSocketFlags flags, KprSocketCallbaks *callbacks, void *refcon, KprSocket *it);
	FskAPI(FskErr) KprSocketDispose(KprSocket self);

	FskAPI(void) KprSocketSetupCallbaks(KprSocket self, KprSocketCallbaks *callbacks, void *refcon);

	FskAPI(FskErr) KprSocketConnect(KprSocket self, const char *host, int port);
	FskAPI(FskErr) KprSocketClose(KprSocket self);
	Boolean KprSocketIsConnected(KprSocket self);

	FskAPI(FskErr) KprSocketSend(KprSocket self, FskMemPtr data, UInt32 length);

	FskAPI(FskErr) KprSocketSendTo(KprSocket self, FskMemPtr data, UInt32 length, int ipaddr, int port);
	FskAPI(FskErr) KprSocketRecv(KprSocket self, FskMemPtr buffer, int bufferSize, int *amt);

	FskAPI(FskErr) KprSocketBind(KprSocket self, int port, const char *iface);
	FskAPI(FskErr) KprSocketListen(KprSocket self);
	Boolean KprSocketIsListening(KprSocket self);
	FskErr KprSocketAccept(KprSocket self, KprSocket *it);

	FskAPI(FskErr) KprSocketSetCertificate(KprSocket self, FskSocketCertificateRecord *certs);
	FskAPI(FskErr) KprSocketSetTLSProtocolVersion(KprSocket self, KprSocketTLSProtocolVersion version);
	FskAPI(FskErr) KprSocketSetTLSApplicationProtocols(KprSocket self, const char *protocols);

	FskAPI(KprSocketProto) KprSocketGetProto(KprSocket self);
	FskAPI(UInt32) KprSocketGetAddress(KprSocket self);
	FskAPI(int) KprSocketGetPort(KprSocket self);
	FskAPI(UInt32) KprSocketGetRemoteAddress(KprSocket self);
	FskAPI(int) KprSocketGetRemotePort(KprSocket self);
	FskAPI(void) KprSocketSetRemoteAddress(KprSocket self, UInt32 addr);
	FskAPI(void) KprSocketSetRemotePort(KprSocket self, int port);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
