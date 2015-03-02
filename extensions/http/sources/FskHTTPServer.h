/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#ifndef __FSKHTTPSERVER__
#define __FSKHTTPSERVER__

#include "FskTime.h"
#include "FskHeaders.h"

#ifdef __FSKHTTPSERVER_PRIV__
#include "FskNetUtils.h"
#include "FskUtilities.h"
#include "FskThread.h"
#include "FskNetInterface.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define kFskHTTPServerDefaultBufferSize	32768

#define kFskTransferEncodingNone        0
#define kFskTransferEncodingChunked     1

typedef struct FskHTTPServerRecord FskHTTPServerRecord;
typedef FskHTTPServerRecord *FskHTTPServer;
typedef struct FskHTTPServerListenerRecord FskHTTPServerListenerRecord;
typedef FskHTTPServerListenerRecord *FskHTTPServerListener;
typedef struct FskHTTPServerRequestRecord FskHTTPServerRequestRecord;
typedef FskHTTPServerRequestRecord *FskHTTPServerRequest;
typedef struct FskHTTPServerStatsRecord FskHTTPServerStatsRecord;
typedef FskHTTPServerStatsRecord *FskHTTPServerStats;
typedef struct FskHTTPServerRequestStatsRecord FskHTTPServerRequestStatsRecord;
typedef FskHTTPServerRequestStatsRecord *FskHTTPServerRequestStats;

typedef FskErr (*FskHTTPServerConditionCallback)(FskHTTPServer server, UInt32 condition, void *refCon);
typedef FskErr (*FskHTTPServerRequestConditionCallback)(FskHTTPServerRequest request, UInt32 condition, void *refCon);
typedef FskErr (*FskHTTPServerReceiveDataCallback)(FskHTTPServerRequest request, char *data, UInt32 size, UInt32 *sizeConsumed, void *refCon);
typedef FskErr (*FskHTTPServerGenerateDataCallback)(FskHTTPServerRequest request, char *data, UInt32 size, UInt32 *sizeGenerated, void *refCon);

typedef struct FskHTTPServerCallbackVectors {
	FskHTTPServerConditionCallback serverCondition;
		// kFskHTTPConditionInterfaceChanged
		// kFskHTTPConditionInterfaceAdded,
		// kFskHTTPConditionInterfaceRemoved,
		// kFskHTTPConditionNoSocket

	FskHTTPServerRequestConditionCallback requestCondition;
		// kFskHTTPConditionConnectionInitialized
		// kFskHTTPConditionRequestReceivedRequestHeaders
		// kFskHTTPConditionRequestRequestFinished
		// kFskHTTPConditionRequestGenerateResponseHeaders
		// kFskHTTPConditionRequestResponseFinished
		// kFskHTTPConditionRequestErrorAbort
		// kFskHTTPConditionConnectionTerminating

	FskHTTPServerReceiveDataCallback requestReceiveRequest;
	FskHTTPServerGenerateDataCallback requestGenerateResponseBody;

} FskHTTPServerCallbackVectors, *FskHTTPServerCallbacks;


// conditions
enum {
	kFskHTTPConditionNone = 0,
	kFskHTTPConditionNoSocket,
	kFskHTTPConditionConnectionInitialized,
	kFskHTTPConditionRequestReceivedRequestHeaders,
	kFskHTTPConditionRequestRequestFinished,
	kFskHTTPConditionRequestGenerateResponseHeaders,
	kFskHTTPConditionRequestResponseFinished,
	kFskHTTPConditionRequestErrorAbort,
	kFskHTTPConditionConnectionTerminating,
	kFskHTTPConditionInterfaceAdded,
	kFskHTTPConditionInterfaceRemoved,
	kFskHTTPConditionInterfaceChanged
};

struct FskHTTPServerStatsRecord {
    FskInt64    bytesReceived;			// total bytes received
    FskInt64    bytesSent;				// total bytes sent
	UInt32		connectionsMade;		// connection attempts
	UInt32		connectionsCompleted;	// connection serviced and closed
	UInt32		connectionsAborted;		// connection closed abruptly
	UInt32		connectionsRefused;		// connection attempted, but server stopped
	UInt32		requestsStarted;		// request received
	UInt32		requestsFailed;			// request failed
	UInt32		requestsFinished;		// request serviced and completed
    FskTimeRecord serverStarted;		// last time server started
    FskTimeRecord serverStopped;		// last time server stopped
}; 

struct FskHTTPServerRequestStatsRecord {
    FskInt64    bytesReceived;				// total of all bytes for this request
    FskInt64    requestBodyReceived;		// request body bytes
    FskInt64    expectedBytesToReceive;		// content length of request body
    FskInt64    bytesSent;					// total bytes sent
    FskInt64    bodyBytesSent;				// response bytes
    FskInt64    expectedBodyToSend;			// content length of response
    FskTimeRecord requestStarted;			// when connection was accepted
    FskTimeRecord requestStopped;			// when connection was finished
}; 

#if SUPPORT_INSTRUMENTATION
enum {
	kFskHTTPInstrMsgRequestState = kFskInstrumentedItemFirstCustomMessage,
	kFskHTTPInstrMsgErrString,					// string
	kFskHTTPInstrMsgNowListening,				// listener
	kFskHTTPInstrMsgFailedListener,				// listener
	kFskHTTPInstrMsgStopListening,				// listener
	kFskHTTPInstrMsgConnectionRefusedStopped,	//
	kFskHTTPInstrMsgServerStart,				// http
	kFskHTTPInstrMsgServerStartedAlready,		// http
	kFskHTTPInstrMsgServerStop,					// http
	kFskHTTPInstrMsgServerStoppedAlready,		// http
	kFskHTTPInstrMsgRequestKillIdle,			// request
	kFskHTTPInstrMsgRequestRemainsOnClose,		// request
	kFskHTTPInstrMsgRequestRecvData,			// MsgDataRecord
	kFskHTTPInstrMsgRequestSendData,			// MsgDataRecord
/*
	kFskHTTPInstrMsg
	kFskHTTPInstrMsg
	kFskHTTPInstrMsg
	kFskHTTPInstrMsg
*/
};

typedef struct {
	char *buffer;	
	UInt32 amt;
	FskErr err;
} FskHTTPInstrMsgDataRecord;
#endif


#ifdef __FSKHTTPSERVER_PRIV__
struct FskHTTPServerListenerRecord {
	FskHTTPServerListener			next;
	FskHTTPServer			http;
	FskSocket				skt;
	int						port;
	char					*ifcName;
	FskThreadDataHandler	dataHandler;
};

struct FskHTTPServerRecord {
	FskHTTPServerListener				listeners;
	FskHTTPServerRequest 				activeRequests;

	Boolean								stopped;
	Boolean								all;
	int									defaultBufferSize;
	int									port;
	void								*refCon;

	int									keepAliveTimeout;

	FskNetInterfaceNotifier				interfaceNotifier;
	FskHTTPServerStatsRecord			stats;
	FskHTTPServerCallbacks				callbacks;

	int									useCount;
	FskThread							owner;
	Boolean								ssl;
	FskSocketCertificateRecord			*certs;

	char								name[64];

	FskInstrumentedItemDeclaration
};

#define kFskDataChunkDefaultSize    4096
typedef struct dataChunkRecord {
    int     pos;
    int     max;
    char    *buf;
    int     bufferSize;
} dataChunkRecord;

struct FskHTTPServerRequestRecord {
	FskHTTPServerRequest	next;
	FskHTTPServer	http;
	FskSocket		skt;
	int				useCount;
	int				requesterAddress;
	int				requesterPort;
	FskHeaders		*requestHeaders;

	int				responseCode;
	FskHeaders		*responseHeaders;

	FskHTTPServerRequestStatsRecord stats;

//---
	int				state;
	int				nextState;
	int				suspendedState;

	dataChunkRecord			in;
	dataChunkRecord 		out;
	FskThreadDataHandler	dataHandler;
	FskTimeCallBack			cycleCallback;
	Boolean					keepAlive;
	FskTimeCallBack			keepAliveKillCallback;
	int						keepAliveTimeout;

	int				transferEncoding;
	Boolean			requestBodyChunked;
	int 			requestBodyContentLength;

	void			*refCon;

	FskTimeCallBack			timer;
	FskInstrumentedItemDeclaration
};
#endif // __FSKHTTPSERVER_PRIV__


typedef struct {
	FskHTTPServer		server;

	xsMachine			*the;
	xsSlot				obj;
} xsNativeHTTPServerRecord, *xsNativeHTTPServer;

typedef struct {
	FskHTTPServerRequest request;

	xsMachine			*the;
	xsSlot				obj;

	UInt32				bytesRemaining;
	xsNativeHTTPServer	nhs;
} xsNativeHTTPRequestRecord, *xsNativeHTTPRequest;

/* interface is optional in HTTPServerCreate. If NULL, then all interfaces will be used */
FskExport (FskErr) FskHTTPServerCreate(int port, char *ifc, FskHTTPServer *http, void *refCon, Boolean ssl);
FskExport (FskErr) FskHTTPServerDispose(FskHTTPServer http);

FskExport (void *) FskHTTPServerGetRefcon(FskHTTPServer http);
FskExport (void) FskHTTPServerSetRefcon(FskHTTPServer http, void *refCon);

FskExport (void) FskHTTPServerSetBufferSize(FskHTTPServer http, int bufferSize);
FskExport (void) FskHTTPServerSetKeepAliveTimeout(FskHTTPServer http, int timeout);
FskExport (FskErr) FskHTTPServerGetStats(FskHTTPServer http, FskHTTPServerStats *stats);
FskExport (FskErr) FskHTTPServerSetCertificates(FskHTTPServer http, FskSocketCertificateRecord *certs);

FskExport (FskErr) FskHTTPServerListenerAdd(FskHTTPServer http, int port, char *ifcName, FskHTTPServerListener *newListener);
FskExport (FskErr) FskHTTPServerListenerDispose(FskHTTPServerListener listener);

// start all listeners
FskExport (FskErr) FskHTTPServerStart(FskHTTPServer http);
// stop accepting new connections, flush will cause this call to wait until requests are completed
FskExport (FskErr) FskHTTPServerStop(FskHTTPServer http, Boolean flush);

FskExport (FskErr) FskHTTPServerSetCallbacks(FskHTTPServer http, FskHTTPServerCallbacks callbacks);
FskExport (FskErr) FskHTTPServerGetCallbacks(FskHTTPServer http, FskHTTPServerCallbacks *callbacks);

FskExport (void) FskHTTPServerRequestSetRefcon(FskHTTPServerRequest request, void *refCon);
FskExport (void *) FskHTTPServerRequestGetRefcon(FskHTTPServerRequest request);

FskExport (void) FskHTTPServerRequestSetKeepAliveTimeout(FskHTTPServerRequest request, int timeout);
FskExport (int) FskHTTPServerRequestGetKeepAliveTimeout(FskHTTPServerRequest request);

FskExport (FskHTTPServer) FskHTTPServerRequestGetServer(FskHTTPServerRequest request);
FskExport (FskErr) FskHTTPServerRequestGetStats(FskHTTPServerRequest request, FskHTTPServerRequestStatsRecord *stats);

FskExport (FskHeaders*) FskHTTPServerRequestGetRequestHeaders(FskHTTPServerRequest request);
FskExport (FskHeaders*) FskHTTPServerRequestGetResponseHeaders(FskHTTPServerRequest request);

FskExport (FskErr) FskHTTPServerRequestGetLocalAddress(FskHTTPServerRequest request, int *ip, int *port);

FskExport (void) FskHTTPServerRequestDispose(FskHTTPServerRequest request);

FskExport (void) FskHTTPServerRequestCycle(FskHTTPServerRequest request);

FskExport(FskErr) FskHTTPServerRequestSuspend(FskHTTPServerRequest request);
FskExport(FskErr) FskHTTPServerRequestResume(FskHTTPServerRequest request);

enum {
	kHTTPNewSession = 0,
	kHTTPReadRequestHeaders,
	kHTTPProcessRequestHeaders,
	kHTTPReadRequestBody,
	kHTTPProcessRequestBody,
	kHTTPPrepareResponse,
	kHTTPProcessResponse,
	kHTTPGetDataChunk,
	kHTTPSendDataChunk,
	kHTTPSetupNextRequest,
	kHTTPDone,
	kHTTPFulfillExpectation,
	kHTTPDenyExpectation,
	kHTTPServerError,
	kHTTPSocketError,
	kHTTPClose,
	kHTTPSessionSuspend,
	kHTTPRequestComplete
};

#ifdef __cplusplus
}
#endif

#endif // __FSK_HTTPSERVER__
