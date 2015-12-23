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
#ifndef __FSK_HTTPCLIENT_H__
#define __FSK_HTTPCLIENT_H__

#include "FskMemory.h"
#include "FskString.h"
#include "FskHeaders.h"
#include "FskNetUtils.h"
#include "FskNetInterface.h"
#include "HTTP.h"
#include "FskThread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FskHTTPClientRecord *FskHTTPClient;
typedef struct FskHTTPClientRequestRecord *FskHTTPClientRequest;
typedef struct FskHTTPAuthRecord *FskHTTPAuth;

// callbacks
// Client returns kFskErrEndOfFile to indicate that this is the final buffer
typedef FskErr (*FskHTTPClientRequestSendDataCallback) (FskHTTPClientRequest request, char **buffer, int *bufferSize, FskInt64 position, void *refCon);
typedef FskErr (*FskHTTPClientRequestReceivedResponseHeadersCallback) (FskHTTPClientRequest request, FskHeaders *responseHeaders, void *refCon);
typedef FskErr (*FskHTTPClientRequestReceivedDataCallback) (FskHTTPClientRequest request, char *buffer, int bufferSize, void *refCon);
typedef FskErr (*FskHTTPClientRequestFinishedCallback) (FskHTTPClient client, FskHTTPClientRequest request, void *refCon);

typedef FskErr (*FskHTTPClientAuthCallback) (FskHTTPClient client, FskHTTPClientRequest request, char *url, char *realm, FskHTTPAuth auth, void *refCon);
typedef FskErr (*FskHTTPClientFinishedCallback) (FskHTTPClient client, void *refCon);

// Client Status
typedef struct FskHTTPClientRequestStatus {
	FskInt64	bytesSent;
	FskInt64	headerSize;
	FskInt64	bytesReceived;
	FskInt64	dataReceived;
	Boolean		requestFinished;
	int			responseCode;
	FskErr		lastErr;
} FskHTTPClientRequestStatus;

typedef struct FskHTTPClientStatus {
	FskInt64	bytesSent;
	FskInt64	bytesReceived;
	FskErr		lastErr;
} FskHTTPClientStatus;

#define kFskHTTPClientTransferBufferSize	8192

#if SUPPORT_INSTRUMENTATION
enum {
	kFskHTTPClientInstrMsgKillIdle = kFskInstrumentedItemFirstCustomMessage, // client
	kFskHTTPClientInstrMsgNew,							// FskHTTPClientSystem
	kFskHTTPClientInstrMsgBegin,						// client
	kFskHTTPClientInstrMsgBeginCurrentlyProcessing,		// client
	kFskHTTPClientInstrMsgBeginSSLThruProxy,			// client
	kFskHTTPClientInstrMsgBeginClientAlreadyConnected,	// client
	kFskHTTPClientInstrMsgBeginConnectToSecureProxy,	// client
	kFskHTTPClientInstrMsgBeginConnectToSecure,			// client
	kFskHTTPClientInstrMsgBeginConnectToProxy,			// client
	kFskHTTPClientInstrMsgBeginConnect,					// client
	kFskHTTPClientInstrMsgSecureProxyConnected,			// client
	kFskHTTPClientInstrMsgSSLConnectFailed,				// client
	kFskHTTPClientInstrMsgSuspend,						// client
	kFskHTTPClientInstrMsgResume,						// client
	kFskHTTPClientInstrMsgCancel,						// client
	kFskHTTPClientInstrMsgFlushRequests,				// client
	kFskHTTPClientInstrMsgRequestState,					// client
	kFskHTTPClientInstrMsgResponseState,				// client
	kFskHTTPClientInstrMsgTransitionRequest,			// req
	kFskHTTPClientInstrMsgConnectionClosed,				// req
	kFskHTTPClientInstrMsgFinished,						// client
	kFskHTTPClientInstrMsgRemainder,						// RequestInstrMsgDataRecord
	kFskHTTPClientInstrMsgCycleBegin,					// client
	kFskHTTPClientInstrMsgCycleEnd,						// client
	kFskHTTPClientInstrMsgFinishedDoneCB,
};

enum {
	kFskHTTPClientInstrMsgSystemHTTPProxy = kFskInstrumentedItemFirstCustomMessage + 512,	// FskHTTPClientSystem
	kFskHTTPClientInstrMsgSystemHTTPSProxy,		// FskHTTPClientSystem
	kFskHTTPClientInstrMsgHTTPProxy,			// FskHTTPClientSystem
	kFskHTTPClientInstrMsgHTTPSProxy,			// FskHTTPClientSystem
};

enum {
	kFskHTTPClientRequestInstrMsgAdd = kFskInstrumentedItemFirstCustomMessage + 1024,	// request
	kFskHTTPClientRequestInstrMsgRemove,		// request
	kFskHTTPClientRequestInstrMsgChangeURL,		// char *
	kFskHTTPClientRequestInstrMsgServerResponse,	// int
	kFskHTTPClientRequestInstrMsgSentRequest,	// RequestInstrMsgDataRecord
	kFskHTTPClientRequestInstrMsgSentBody,		// RequestInstrMsgDataRecord
	kFskHTTPClientRequestInstrMsgRecvHeaders,	// RequestInstrMsgDataRecord
	kFskHTTPClientRequestInstrMsgRecvData,		// RequestInstrMsgDataRecord
	kFskHTTPClientRequestInstrMsgReadRemainder,	// RequestInstrMsgDataRecord
	kFskHTTPClientRequestInstrMsgFinished,		// request
	kFskHTTPClientRequestInstrMsgBadTE,			// char *
	kFskHTTPClientRequestInstrMsgErrString,		// char *
	kFskHTTPClientRequestInstrMsgNotConnect,	// (char *) - method
	kFskHTTPClientRequestInstrMsgReissue,		// request
	kFskHTTPClientRequestInstrMsgCloseOnDone,	// request
};

enum {
	kFskHTTPClientAuthInstrMsgAddToReq = kFskInstrumentedItemFirstCustomMessage + 2048,	// auth
	kFskHTTPClientAuthInstrMsgCredFromURL,		// req
	kFskHTTPClientAuthInstrMsgCredFromAPI,		// client
	kFskHTTPClientAuthInstrMsgCredFromCallback,	// req
	kFskHTTPClientAuthInstrMsgTooManyFailures,	// req
	kFskHTTPClientAuthInstrMsgChallenge,		// req
	kFskHTTPClientAuthInstrMsgGeneratingSessionKey,		// req
	kFskHTTPClientAuthInstrMsgSessionKeyAccepted,		// req
};

typedef struct {
	char *buffer;
	UInt32		amt;
	FskErr		err;
} FskHTTPClientRequestInstrMsgDataRecord;
#endif

typedef enum {
	kFskHTTPClientReadAllData = 0xffffffff,
	kFskHTTPClientReadAnyData = 0
} FskHTTPClientReadEnum;	// constants for FskHTTPClientSetReceiveDataCallback

//typedef struct FskHTTPClientRequestRecord;

enum {
	kFskHTTPAuthCredentialsTypeNone = 0,
	kFskHTTPAuthCredentialsTypeString,
	kFskHTTPAuthCredentialsTypeDigest
};

#define kHTTPAuthAttemptsMax	5
typedef struct FskHTTPAuthRecord {
	struct	FskHTTPAuthRecord *next;
	int		authType;
	Boolean	waitingForPassword;
	Boolean tryPassword;
	char	*realm;
	char	*username;
	char	*credentials;
	int		credentialsType;
	int		credentialsSize;
	char	*nonce;
	char	*qop;
	char	*alg;
	int		authAttempts;
	Boolean	sessionKeyValid;
	char	sessionKey[33];
	Boolean	challenged;
	Boolean useCnonce;
	char	cnonce[9];
	int		nc;
	char	*opaque;

	FskInstrumentedItemDeclaration
} FskHTTPAuthRecord;

typedef struct FskHTTPClientRecord {
	FskSocket	skt;
	int			useCount;
	int			seqNum;
	Boolean		isLocal;

	int			proxyPort;
	int			proxyAddr;
	int			proxyPortS;
	int			proxyAddrS;
	char		*proxyAuth;
	int			protocolNum;
	Boolean		sslProxied;

	char		*host;
	int			hostPort;
	int			hostIP;

	FskHTTPAuth	auth;
	FskSocketCertificate	certs;

	struct FskHTTPClientRequestRecord *httpRequests;
	int			requestState;
	int			requestNextState;
	int			requestSuspendedState;

	Boolean		flushUnsatisfiedResponses;
	Boolean		readUntilServerCloses;
	Boolean		waitingForSocket;
	Boolean		disposed;
	Boolean		dontPipeline;
	Boolean		eligibleForReconnect;
	Boolean		callingFinish;
	Boolean		calledFinish;

	struct FskHTTPClientRequestRecord *httpResponses;
	int			responseState;
	int			responseNextState;
	int			responseSuspendedState;
	Boolean		pendingSuspend;
	Boolean		pendingCancel;
	
	FskHTTPClientStatus	status;

	Boolean		inputBufNeedsDisposal;	// if locally malloc'd
	char		*inputBuf;
	int			inputBufAmt;
	int			inputBufPos;

	Boolean		outputBufNeedsDisposal;	// if locally malloc'd
	char		*outputBuf;
	int			outputBufAmt;
	int			outputBufPos;

	void						*refCon;
	FskHTTPClientAuthCallback			authCB;
	FskHTTPClientFinishedCallback		finishedCB;

	FskTimeCallBack			cycleCallback;
	FskThreadDataHandler	writeDataHandler;
	Boolean					needsWritable;
	FskThreadDataHandler	readDataHandler;
	Boolean					needsReadable;

	int						idleKillSecs;
	FskTimeCallBack			killTimer;
	FskNetInterfaceNotifier interfaceChangeNotifier;
	Boolean					interfaceLost;
	int						interfaceSeed;
	int						interfaceSeedPreConnect;

	int						priority;

	int						cycleNum;		// for debugging
	char					*name;
	FskInstrumentedItemDeclaration
} FskHTTPClientRecord;


typedef struct FskHTTPClientRequestRecord {
	struct FskHTTPClientRequestRecord	*next;
	FskHTTPClient						owner;

	int			useCount;
	int			redirectTimes;

	char		*method;
	Boolean		methodNeedsDispose;	// if user changed the method (like post)
	Boolean		beenRequested;		// this request has been serialized into a request buffer
	Boolean		closeOnDone;	// close socket on completion
	Boolean		flushAndReissue;	// flush data, don't call callbacks
	Boolean			suspend;		// after flushing, suspend this client - ie. wait for UI to provide Auth.

	FskStrParsedUrl parsedUrl;
	int		protocolNum;
	int		hostPort;

	FskAssociativeArray	requestParameters;
	FskHeaders			*requestHeaders;
	FskHeaders			*responseHeaders;

	int				transferEncoding;
	int				currentChunkSize;	// for chunked encoding - expected size
	Boolean			chunksFinished;
	Boolean			expectContinue;
	Boolean			eligibleForReconnect;

	FskHTTPClientRequestStatus	status;

	void										*refCon;
	FskHTTPClientRequestSendDataCallback				sendDataCB;
	FskHTTPClientRequestReceivedResponseHeadersCallback	responseHeadersCB;
		UInt32									responseHeadersFlags;
	FskHTTPClientRequestReceivedDataCallback			receivedDataCB;
	FskHTTPClientReadEnum                               receivedDataCBMode;
	FskHTTPClientRequestFinishedCallback				reqFinishedCB;

	Boolean		staticRequestBuffer;	// if set by the user
	char		*requestBodyBuffer;			// set by user 
	int			requestBodySize;		// managed by the user of the lib
	int			requestBodyPos;
	int			requestBodyBufferAmt;
	int			requestBodyBufferPos;

	FskInt64	respContentLength;		// if specified in a header
	FskInt64	respContentRead;		// how much we've read of the response

	Boolean		userBuffer;		// if false, then buffer needs disposal
	char		*buffer;
	int			bufferSize;	
	int			bufferAmt;
	int			bufferPos;

	FskInstrumentedItemDeclaration
} FskHTTPClientRequestRecord;

// using slash as separate between name and version to comply with UPnP requirements
#define kFskHTTPClientIdentifier "Kinoma HTTP Client/2.5"

// initialize module
FskAPI(FskErr) FskHTTPClientInitialize(void);
FskAPI(FskErr) FskHTTPClientTerminate(void);

// Create an HTTP client object
FskAPI(FskErr)	FskHTTPClientNewPrioritized(FskHTTPClient *client,	\
			int priority, char *name);
#define	FskHTTPClientNew(client, name)	\
			FskHTTPClientNewPrioritized(client, kFskNetSocketDefaultPriority, name)

FskAPI(FskErr)	FskHTTPClientDispose(FskHTTPClient client);

// Set a proxy (will override system proxy if any)
// host can contain a "host:port", or alternately, use addr and port
enum {
	kFskHTTPClientProxy	= 1,
	kFskHTTPSClientProxy
};

FskAPI(FskErr) FskHTTPSetProxyOverride(Boolean proxyOverride);

FskAPI(void) FskHTTPSyncSystemProxy(void);
FskAPI(FskErr) FskHTTPSetDefaultProxy(int type, int addr, int port);
FskAPI(FskErr) FskHTTPGetProxy(int type, int *addr, int *port);

FskAPI(FskErr) FskHTTPClientGetProxy(FskHTTPClient client, int type, int *addr, int *port);
FskAPI(FskErr) FskHTTPClientSetProxy(FskHTTPClient client, int type, int addr, int port);

FskAPI(void) FskHTTPSetDefaultProxyAuth(const char* authString);

FskAPI(void) FskHTTPClientSetRefCon(FskHTTPClient client, void *refCon);

// if a HCFinishedCallback is set, then the user is responsible for disposing
// of the client
FskAPI(void) FskHTTPClientSetFinishedCallback(FskHTTPClient client,
							  	FskHTTPClientFinishedCallback callback);

FskAPI(void)	FskHTTPClientSetAuthCallback(FskHTTPClient client,
								FskHTTPClientAuthCallback callback);
FskAPI(FskErr)	FskHTTPClientSetCredentials(FskHTTPClient client,
								char *username, char *credentials,
								int credentialsSize, int credentialsType);
FskAPI(FskErr)	FskHTTPClientSetCertificates(FskHTTPClient client, FskSocketCertificate certs);

FskAPI(FskErr)	FskHTTPClientBegin(FskHTTPClient client);
FskAPI(FskErr)  FskHTTPClientAddRequest(FskHTTPClient client, FskHTTPClientRequest request);
FskAPI(FskErr)	FskHTTPClientRemoveRequest(FskHTTPClientRequest request);
//FskAPI(FskHTTPClientRequest) FskHTTPClientGetActiveRequest(FskHTTPClient client);
FskAPI(FskErr)	FskHTTPClientFlushRequests(FskHTTPClient client);

// BeginRequest is a shortcut that combines AddRequest and Begin
FskAPI(FskErr)	FskHTTPClientBeginRequest(FskHTTPClient client, FskHTTPClientRequest request);

FskAPI(void) 	FskHTTPClientSuspend(FskHTTPClient client);
FskAPI(void) 	FskHTTPClientResume(FskHTTPClient client);
FskAPI(void)	FskHTTPClientCancel(FskHTTPClient client);
FskAPI(Boolean)	FskHTTPClientIsIdle(FskHTTPClient client);


// for requests

FskAPI(FskErr) FskHTTPClientRequestNew(FskHTTPClientRequest *reqOut, char *url);
FskAPI(FskErr) FskHTTPClientRequestDispose(FskHTTPClientRequest request);

FskAPI(void) FskHTTPClientRequestSetRefCon(FskHTTPClientRequest request, void *refCon);

FskAPI(FskErr) FskHTTPClientRequestSetURL(FskHTTPClientRequest request, char *url);
FskAPI(FskErr) FskHTTPClientRequestSetURLParts(FskHTTPClientRequest request, char *protocol, char *host, int port, char *target);

// Add request line parameters, e.g. "abc=100"
FskAPI(FskErr) FskHTTPClientRequestAddParameter(FskHTTPClientRequest request, char *paramName, char *paramValue);

// Add header "name: value" pair
FskAPI(FskErr) FskHTTPClientRequestAddHeader(FskHTTPClientRequest request, char *headerName, char *headerValue);
FskAPI(FskErr) FskHTTPClientRequestRemoveHeader(FskHTTPClientRequest request, char *headerName);

// Specify method - get/post
FskAPI(void) FskHTTPClientRequestSetMethod(FskHTTPClientRequest request, char *method);

// set the post method and a buffer
FskAPI(FskErr) FskHTTPClientRequestPost(FskHTTPClientRequest request, char *messageBody, int messageBodyLen);

// Set a reference to some data that will be used as body data.
// If set, this data will be sent, otherwise SendData callback will be used
FskAPI(void) FskHTTPClientRequestSetRequestBody(FskHTTPClientRequest clientReq, char *buffer, int bufferSize);

// Specify a callback for sending message body data
// if SetRequestBody was used, it will override this.
FskAPI(void) FskHTTPClientRequestSetSendDataCallback(FskHTTPClientRequest clientReq,
							  	FskHTTPClientRequestSendDataCallback callback);

// Specify a callback for browsing response headers
enum {
	kHTTPClientResponseHeadersOnRedirect = 1L << 0
};

FskAPI(void) FskHTTPClientRequestSetReceivedResponseHeadersCallback(FskHTTPClientRequest request,
					FskHTTPClientRequestReceivedResponseHeadersCallback callback,
					UInt32 flags);

// Specify a callback for receiving response data
// mode specifies:
//		kFskHTTPClientReadAllData	-- library manages buffer
//		kFskHTTPClientReadAnyData	-- user manages buffer size
//									-- kFskErrParameterErr if buffersize is 0
FskAPI(FskErr) FskHTTPClientRequestSetReceivedDataCallback(FskHTTPClientRequest request,   
								 FskHTTPClientRequestReceivedDataCallback callback,
								 char *buffer, int bufferSize, FskHTTPClientReadEnum mode);

// Specify a callback for when the request is complete
// if a HCFinishedCallback is set, then the user is responsible for disposing
// of the request
FskAPI(void) FskHTTPClientRequestSetFinishedCallback(FskHTTPClientRequest request,
									  FskHTTPClientRequestFinishedCallback callback);



FskAPI(void) FskHTTPClientSetIdleTimeout(FskHTTPClient client, int secs);

#ifdef __cplusplus
}
#endif


#endif // __FSK_HTTPCLIENT_H__

