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

#ifndef _RTSP_SESSION_H
#define _RTSP_SESSION_H

#include "SDP.h"
#include "RTPPacketParser.h"
#include "RTCPPacketParser.h"
#include "FskHeaders.h"
#include "FskUtilities.h"
#include "FskTime.h"
#include "FskNetUtils.h"
#include "FskNetInterface.h"

#if SUPPORT_INSTRUMENTATION
	// implementation headers
	#if TARGET_OS_WIN32
		#include <Windows.h>
		#if TRY_WINCE_DD
			#include "ddraw.h"
		#endif
	#endif
	#include "FskInstrumentation.h"
#endif

// RTSP response -> error codes
#define kFskErrRTSPResponseCodeBase -2000

// RTSP response codes
enum {
	kRTSPResponseCodeContinue = 100,
	kRTSPResponseCodeOK = 200,
	kRTSPResponseCodeCreated = 201,
	kRTSPResponseCodeLowOnStorageSpace = 250,
	kRTSPResponseCodeMultipleChoices = 300,
	kRTSPResponseCodeMovedPermanently = 301,
	kRTSPResponseCodeMovedTemporarily = 302,
	kRTSPResponseCodeSeeOther = 303,
	kRTSPResponseCodeUseProxy = 305,
	kRTSPResponseCodeBadRequest = 400,
	kRTSPResponseCodeUnauthorized = 401,
	kRTSPResponseCodePaymentRequired = 402,
	kRTSPResponseCodeForbidden = 403,
	kRTSPResponseCodeNotFound = 404,
	kRTSPResponseCodeMethodNotAllowed = 405,
	kRTSPResponseCodeNotAcceptable = 406,
	kRTSPResponseCodeProxyAuthenticationRequired = 407,
	kRTSPResponseCodeRequestTimeout = 408,
	kRTSPResponseCodeGone = 410,
	kRTSPResponseCodeLengthRequired = 411,
	kRTSPResponseCodePreconditionFailed = 412,
	kRTSPResponseCodeEntityTooLarge = 413,
	kRTSPResponseCodeRequestURITooLong = 414,
	kRTSPResponseCodeUnsupportedMediaType = 415,
	kRTSPResponseCodeInvalidParameter = 451,
	kRTSPResponseCodeIllegalConferenceIdentifier = 452,
	kRTSPResponseCodeNotEnoughBandwidth = 453,
	kRTSPResponseCodeSessionNotFound = 454,
	kRTSPResponseCodeMethodNotValidInThisState = 455,
	kRTSPResponseCodeHeaderFieldNotValid = 456,
	kRTSPResponseCodeInvalidRange = 457,
	kRTSPResponseCodeParameterIsReadOnly = 458,
	kRTSPResponseCodeAggregateOperationNotAllowed = 459,
	kRTSPResponseCodeOnlyAggregateOperationAllowed = 460,
	kRTSPResponseCodeUnsupportedTransport = 461,
	kRTSPResponseCodeDestinationUnreachable = 462,
	kRTSPResponseCodeInternalServerError = 500,
	kRTSPResponseCodeNotImplemented = 501,
	kRTSPResponseCodeBadGateway = 502,
	kRTSPResponseCodeServiceUnavailable = 503,
	kRTSPResponseCodeGatewayTimeout = 504,
	kRTSPResponseCodeRTSPVersionNotSupported = 505,
	kRTSPResponseCodeOptionNotSupported = 551
};

enum {
	kRTSPFeatureDisableReliableUDP = 1
};

#define RTSP_DEBUG_DROP_PACKETS		0
#define RTSP_RECEIVER_REPORTS		1

// Client state machine - RFC 2326 A.1
enum {
	kRTSPClientStateError=-1,	// Error encountered in callback
	kRTSPClientStateIdle=0,		// Not connected
	kRTSPClientStateConnected,	// Connection established with host
	kRTSPClientStateStart,		// DESCRIBE not sent
	kRTSPClientStateDescribe,	// DESCRIBE sent, waiting for reply
	kRTSPClientStateInit,		// SETUP sent, waiting for reply
	kRTSPClientStateReady,		// SETUP or PLAY reply received while in playing state
	kRTSPClientStatePlaying,	// PLAY reply received
	kRTSPClientStateRecording	// RECORD reply received
};

// Network state machine
enum {
	kRTSPClientIdle,
	kRTSPClientInitialize,				// not set up yet
	kRTSPClientConnecting,

	kRTSPClientConnected,				// connected, but not reading headers or packets

	kRTSPClientWriteRequest,			// sending RTSP control commands
	kRTSPClientReadData,				// generic data read
	kRTSPClientParseData,				// determine if data is a packet or part of a response
	kRTSPClientReadResponseHeaders,		// reading response headers
	kRTSPClientProcessResponseHeaders,	// processing response headers - no response body
	kRTSPClientReadResponseBody,		// reading response body
	kRTSPClientProcessResponseBody,

	kRTSPClientReadPacketHeader,
	kRTSPClientReadPacketData,
	kRTSPClientProcessPacket
};

enum {
	kRTSPMediaStreamVideo = 0,
	kRTSPMediaStreamAudio
};

struct UDPSocketCallbackInfoRecord;
typedef struct UDPSocketCallbackInfoRecord UDPSocketCallbackInfoRecord;
typedef struct UDPSocketCallbackInfoRecord *UDPSocketCallbackInfo;

struct RTSPRequestRecord;
typedef struct RTSPRequestRecord RTSPRequestRecord;
typedef struct RTSPRequestRecord *RTSPRequest;

struct RTSPRequestListRecord;
typedef struct RTSPRequestListRecord RTSPRequestListRecord;
typedef struct RTSPRequestListRecord *RTSPRequestList;

struct RTSPSessionRecord;
typedef struct RTSPSessionRecord RTSPSessionRecord;
typedef struct RTSPSessionRecord *RTSPSession;

struct RTSPMediaStreamRecord;
typedef struct RTSPMediaStreamRecord RTSPMediaStreamRecord;
typedef struct RTSPMediaStreamRecord *RTSPMediaStream;

struct RTSPStatusRecord;
typedef struct RTSPStatusRecord RTSPStatusRecord;
typedef struct RTSPStatusRecord *RTSPStatus;

// Structure used to track reliable udp
typedef struct {
	UInt8	ackPacket[24];
	UInt32	baseSeq;
	FskTimeRecord emitTime;
	FskTimeCallBack emitCB;
	UInt32	mask;
	UInt32	nPackets;
} RTSPReliableUDPDataRecord, *RTSPReliableUDPData;

// Structure used to pass information to socket thread data callback
struct UDPSocketCallbackInfoRecord {
	RTSPSession session;
	RTSPMediaStream mediaStream;
	UInt16 nextSeq;
	FskThreadDataHandler dataHandler;
};

// A RTSP request
struct RTSPRequestRecord {
	struct RTSPRequestRecord *next;

	UInt32 cSeq;					// CSeq number of this request
	char method[32];				// Method - DESCRIBE, SETUP, PLAY, etc...

	FskHeaders *requestHeaders;		// Request headers
	FskHeaders *responseHeaders;	// Response headers
	
	char *buffer;					// Buffer containing method, url
	UInt32 bufferSize;
	UInt32 bufferPos;
	UInt32 bufferAmt;

	char *responseBody;				// Pointer to response body (do not dispose)
	UInt32 responseBodySize;

	void *param;					// User-defined parameter
};

struct RTSPRequestListRecord {
	FskList head;
};

typedef RTSPRequestList RTSPResponseList;

typedef struct {
	UInt32 length;
	char *buffer;
} RTSPReadBufferRecord, *RTSPReadBuffer;

typedef struct {
	UInt16 max_seq;        /* highest seq. number seen */
	UInt32 cycles;         /* shifted count of seq. number cycles */
	UInt32 base_seq;       /* base seq number */
	UInt32 bad_seq;        /* last 'bad' seq number + 1 */
	UInt32 probation;      /* sequ. packets till source is valid */
	UInt32 received;       /* packets received */
	UInt32 expected_prior; /* packet expected at last interval */
	UInt32 received_prior; /* packet received at last interval */
	UInt32 transit;        /* relative trans time for prev pkt */
	UInt32 jitter;         /* estimated jitter */
} RTSPStreamStatsRecord, *RTSPStreamStats;

struct RTSPMediaStreamRecord {
	UInt32 streamType;		// video or audio
	UInt32 payloadType;
	SDPMediaDescription mediaDescription;
	RTSPSession rtspSession;
	Boolean ready;			// Set TRUE when SETUP response has been received
	char *session;
	UInt32 refCon;
	UInt32 ssrc;
	Boolean ssrcIsExplicit;
#if RTSP_RECEIVER_REPORTS
	Boolean sendReceiverReport;
	UInt32 clientSSRC;
	RTCPSenderReport lastSenderReport;
	FskTimeRecord lastSenderReportTime;
	RTSPStreamStatsRecord stats;
	Boolean sequenceInitialized;
	UInt32 rtpTimeScale;	// Obtained from SDP, used to calculate jitter
#endif

	UInt32 rtpChannel;		// Used with RTP over TCP
	UInt32 rtcpChannel;		// Used with RTP over TCP
	RTPPacketParser packetParser;
	RTCPPacketParser packetParserRTCP;

	UInt32 seq;				// First RTP packet sequence number expected after seek/play
	UInt32 rtpTime;			// Time of first RTP packet after seek/play

	UInt32 sourceIPAddr;
	RTSPReadBufferRecord bufferRTP;	// UDP input buffer

	FskSocket sktRTP;		// UDP socket for RTP packets
	UInt32 clientPortRTP;	// UDP client port for RTP packets
	UInt32 serverPortRTP;	// UDP server port for *receiving* RTP packets
	UInt32 serverIPRTP;		// Server IP address of *received* UDP RTCP packets
	UDPSocketCallbackInfoRecord rtpSocketInfo;

	FskSocket sktRTCP;	// UDP socket for RTCP packets
	UInt32 clientPortRTCP;	// UDP client port for RTCP packets
	UInt32 serverPortRTCP;	// UDP server port for *receiving* RTCP packets
	UDPSocketCallbackInfoRecord rtcpSocketInfo;

	RTSPReliableUDPDataRecord reliableUDP;
};

struct RTSPStatusRecord {
	FskErr lastErr;
};

typedef FskErr (*RTSPSessionReadyCallback)(void *refCon);
typedef FskErr (*RTSPSessionResponseHeaderCallback) (char *method, FskHeaders *responseHeaders, void *refCon);
typedef FskErr (*RTSPSessionStateChangeCallback)(SInt16 newState, void *refCon);

struct RTSPSessionRecord {
	// Network
	FskSocket	skt;
	UInt32		port;
	char		*theURL;
	char		*redirect;
	FskStrParsedUrl	urlComponents;
	char		*protocol;	// don't dispose	-- points into theURL
	char		*host;		// don't dispose	-- points into theURL
	char		*path;		// don't dispose	-- points into theURL
	char		*params;	// don't dispose	-- points into theURL
	Boolean		isLocal;
	UInt32		proxyPort;
	UInt32		proxyAddr;

	UInt16		networkState;

	UInt16		readState;
	UInt16		readStateNext;
	FskThreadDataHandler	readDataHandler;
	Boolean					needsReadable;

	FskThreadDataHandler	writeDataHandler;
	Boolean					needsWritable;

	char		*readBuffer;
	UInt32		readBufferPos;
	UInt32		readBufferAmt;

	// Session
	SDPSessionDescription sdp;
	char *sessionID;			// Session identifier from server
	SInt16 sessionState;
	UInt32 cSeq;				// Sequence number
	UInt32 nextPort;			// Next port to request for RTP/RTCP
	Boolean isDarwin;
	Boolean isOrb;
	Boolean isLive;
	Boolean rtpOverTCP;
	Boolean tornDown;
	Boolean waitingForPauseResponse;
	Boolean disableReliableUDP;
	Boolean disposed;
	Boolean waitingForSocket;
	UInt32 defaultPort;
	char *userAgent;
	char *networkType;
	char *referer;
	char *wapProfile;
	char *authCode;
	double startTime;			// starting play time (in seconds)
	double preBufferTime;		// pre buffer time after seek (in seconds, QTSS only)

	Boolean useReliableUDP;		// Reliable UDP delivery, QTSS only
	Boolean useDynamicBufferingRate;		// Dynamic buffering rate, QTSS only
	double lateTolerance;
	Boolean needLateToleranceUpdate;

	FskTimeCallBack			cycleCallback;

	RTSPSessionResponseHeaderCallback clientHeaderCB;
	RTSPSessionStateChangeCallback stateChangeCB;
	void *clientRefCon;
	RTSPStatusRecord	status;
	SInt32				useCount;
	
	FskTimeCallBack		killTimer;
	UInt32				idleKillSecs;

	// Requests
	RTSPRequestList pendingRequests;	// List of client->server requests pending to be sent
	RTSPRequestList sentRequests;		// List of client->server requests that have been sent, but response not yet received
	FskHeaders *responseHeaders;		// Headers being assembled when reading from socket
	UInt32 respContentLength;

	// Client -> Server responses
	RTSPResponseList pendingResponses;	// List of client->server responses pending to be sent
	
	// Streams
	UInt32 nMediaStreams;
	RTSPMediaStreamRecord mediaStream[2];

	// Packet - used for RTP over TCP
	UInt16 packetChannelID;
	UInt32 packetLength;
	UInt32 packetPos;
	char *packetBuffer;

	FskNetInterfaceNotifier
				interfaceChangeNotifier;
	Boolean		interfaceLost;

#if RTSP_DEBUG_DROP_PACKETS
	// Packet dropping (debug)
	UInt16 *droppedPacketIndices;		// Array of indices to drop
	UInt16 droppedPacketCount;			// Number of items in Indices array
	UInt16 droppedPacketRange;			// Size of the "window" from which to drop packets
	UInt16 *droppedPacketsCurrent;		// Current packet index within range
	Boolean droppedPacketRandom;		// true iff Indices contains a random set of indices
#endif

	FskInstrumentedItemDeclaration
};

#ifdef __cplusplus
extern "C" {
#endif

// Allocate and dispose
FskErr RTSPSessionNew(RTSPSession *session, Boolean overTCP);
FskErr RTSPSessionDispose(RTSPSession session);

// Set the session media target URL
FskErr RTSPSessionSetURL(RTSPSession session, char *url);

// Connect to the host specified in the media URL
FskErr RTSPSessionConnectToHost(RTSPSession session);

// Kick off the session
FskErr RTSPSessionStart(RTSPSession session);
FskErr RTSPSessionSetSDP(RTSPSession session, void *sdp, UInt32 sdpSize);

// Specify a callback to be called when the session has completed setup
FskErr RTSPSessionSetReadyCallback(RTSPSession session, RTSPSessionReadyCallback readyCB);

FskErr RTSPSessionSetResponseHeaderCallback(RTSPSession session, RTSPSessionResponseHeaderCallback headerCB);
FskErr RTSPSessionSetStateChangeCallback(RTSPSession session, RTSPSessionStateChangeCallback stateChangeCB);

// Play and pause
FskErr RTSPSessionPlay(RTSPSession session, SInt32 playTimeInMS);
FskErr RTSPSessionPause(RTSPSession session);
FskErr RTSPSessionTeardown(RTSPSession session);
FskErr RTSPSessionOptions(RTSPSession session);

FskErr RTSPSessionSetRefCon(RTSPSession session, void *refCon);

FskErr RTSPSessionSetDefaultPort(RTSPSession session, UInt32 port);
FskErr RTSPSessionSetUserAgent(RTSPSession session, char *userAgent);
FskErr RTSPSessionSetNetworkType(RTSPSession session, char *networkType);
FskErr RTSPSessionSetReferer(RTSPSession session, char *referer);
FskErr RTSPSessionSetWapProfile(RTSPSession session, char *wapProfile);
FskErr RTSPSessionSetProxy(RTSPSession session, char *host, UInt32 addr, UInt32 port);
FskErr RTSPSessionSetAuthCode(RTSPSession session, char *authCode);
FskErr RTSPSessionSetIdleTimeout(RTSPSession session, UInt32 secs);

FskErr RTSPSessionSetFeature(RTSPSession session, UInt32 feature, void *param);

FskErr RTSPSessionSetDroppedPackets(RTSPSession session, UInt16 range, UInt16 dropped, UInt16 *droppedIndices);

FskErr RTSPSessionHandleRTPPacket(RTPPacket packet, void *refcon);
FskErr RTSPSessionHandleRTCPPacket(RTCPPacket packet, void *refcon);

FskErr RTSPSessionAckReliableUDP(RTSPSession session, RTSPMediaStream mediaStream, UInt8 *ackPacket, UInt32 ackPacketLength);

#if SUPPORT_INSTRUMENTATION

enum {
	kRTSPInstrMsgTrace = kFskInstrumentedItemFirstCustomMessage,
	kRTSPInstrMsgCycleReadTCP,
	kRTSPInstrMsgPacketStartTCP,
	kRTSPInstrMsgPacketProgressTCP,
	kRTSPInstrMsgPacketCompleteTCP,
	kRTSPInstrMsgConnectToHost
};

#endif

#ifdef __cplusplus
}
#endif

#endif
