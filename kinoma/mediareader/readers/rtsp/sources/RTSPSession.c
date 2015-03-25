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
#include "RTSPSession.h"
#include "RTPPacketParser.h"
#include "FskUtilities.h"
#include "FskNetUtils.h"
#include "FskEndian.h"

#define LOG0(a)
#define LOG1(a,b)
#define LOG2(a,b,c)

#define kRequestBufferSize 2048
//#define kReadBufferSize 16384
#define kReadBufferSize 32767
#define kInvalidCSeq 0xFFFFFFFF

#define SUPPORT_RELIABLE_UDP 1
#define SUPPORT_DYNAMIC_BUFFERING_RATE 1

#if RTSP_RECEIVER_REPORTS
	// Alternative jitter calculation that avoids floating-point math
	#define JITTER_ALT_CALC		0

	#define RECEIVER_REPORTS_DISPLAY_STATS	0

	#define RTP_SEQ_MOD		(1L<<16)
	// These are default values mentioned in RFC3550 A.1
	#define MAX_DROPOUT		3000
	#define MAX_MISORDER	100
	#define MIN_SEQUENTIAL	2
#endif

static UInt32 gNextPort = 6970;		// Bump above 6970 so AT&T doesn't block
static UInt8 gNextChannel = 1;

static FskErr RTSPRequestNew(RTSPRequest *request);
static void RTSPRequestDispose(RTSPRequest request);

FskErr sPrepareRTSPClientResponse(RTSPSession session, RTSPRequest request, FskHeaders *responseHeaders, UInt32 cSeq);

static FskErr doDescribe(RTSPSession session);
static FskErr doSetup(RTSPSession session, RTSPMediaStream mediaStream);
static FskErr doPlay(RTSPSession session, SInt32 playTimeInMS);
static FskErr doPause(RTSPSession session);
static FskErr doTeardown(RTSPSession session);
static FskErr doOptions(RTSPSession session);
static FskErr doSetParameter(RTSPSession session, char *name, char *value);

static void sSessionRunCycle(FskTimeCallBack cb, FskTime when, void *param);
static void sSessionCycle(void *param);
static FskErr sSessionCycleRead(RTSPSession session);
static FskErr sSessionCycleWrite(RTSPSession session);

static FskErr sRequestComplete(RTSPSession session, RTSPRequest request);
static Boolean isRTPPacketValid(UInt8 *packet, UInt32 packetSize, RTSPMediaStream mediaStream);
static UInt32 getSessionPort(RTSPSession session);
static FskErr sDoStateChange(RTSPSession session, SInt16 newState);
static void sUpSessionUse(RTSPSession session);
static void sDownSessionUse(RTSPSession session);
static void sResetKillTimer(RTSPSession session);
static void sSessionKillMe(FskTimeCallBack cb, FskTime when, void *param);
static int clientInterfaceChangedCB(struct FskNetInterfaceRecord *iface, UInt32 status, void *param);

void sReadPacketRTP(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);
void sReadPacketRTCP(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon);

#if RTSP_DEBUG_DROP_PACKETS
static FskErr sRandomizeDroppedPackets(RTSPSession session);
#endif

#if RTSP_RECEIVER_REPORTS
static FskErr sCreateReceiverReport(RTSPMediaStream mediaStream, unsigned char **buf, UInt32 *bufSize);
static void init_seq(RTSPStreamStats s, UInt16 seq);
static FskErr update_seq(RTSPStreamStats s, UInt16 seq);
#endif

#if SUPPORT_RELIABLE_UDP
static void logPacketForReliableUDP(RTSPSession session, RTSPMediaStream mediaStream, RTPPacket packet);
static void initReliableUDP(RTSPSession session, RTSPMediaStream mediaStream);
static void reliableUDPEmitCB(FskTimeCallBack callback, const FskTime time, void *param);
#endif

#if SUPPORT_INSTRUMENTATION
	static Boolean doFormatMessageRTSP(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

	static FskInstrumentedTypeRecord gRTSPTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"rtsp",
		FskInstrumentationOffset(RTSPSessionRecord),
		NULL,
		0,
		NULL,
		doFormatMessageRTSP
	};
#endif

FskErr RTSPSessionNew(RTSPSession *session, Boolean overTCP)
{
	FskErr err = 0;
	RTSPSession sessionOut;

	err = FskMemPtrNewClear(sizeof(RTSPSessionRecord), &sessionOut);
	BAIL_IF_ERR(err);

	err = FskMemPtrNew(sizeof(RTSPRequestListRecord), &sessionOut->pendingRequests);
	BAIL_IF_ERR(err);
	sessionOut->pendingRequests->head = NULL;

	err = FskMemPtrNew(sizeof(RTSPRequestListRecord), &sessionOut->sentRequests);
	BAIL_IF_ERR(err);
	sessionOut->sentRequests->head = NULL;

	err = FskMemPtrNew(sizeof(RTSPRequestListRecord), &sessionOut->pendingResponses);
	BAIL_IF_ERR(err);
	sessionOut->pendingResponses->head = NULL;

	err = FskMemPtrNew(kReadBufferSize, &sessionOut->readBuffer);
	BAIL_IF_ERR(err);

	sessionOut->interfaceChangeNotifier = FskNetInterfaceAddNotifier(clientInterfaceChangedCB, sessionOut, "rtsp session");

	sUpSessionUse(sessionOut);
	sessionOut->rtpOverTCP = overTCP;		// Set to false to use UDP
	sessionOut->cSeq = 1;
	if (sessionOut->rtpOverTCP) {
		sessionOut->nextPort = gNextChannel;
		gNextChannel += 4;
	}
	else {
		sessionOut->nextPort = gNextPort;
		gNextPort += 4;
	}
	sessionOut->sessionState = kRTSPClientStateIdle;
	sessionOut->networkState = kRTSPClientInitialize;

	FskTimeCallbackNew(&sessionOut->cycleCallback);

	FskInstrumentedItemNew(sessionOut, NULL, &gRTSPTypeInstrumentation);

bail:
	if (err) {
		RTSPSessionDispose(sessionOut);
		sessionOut = 0;
	}
	*session = sessionOut;

	return err;
}

FskErr RTSPSessionDispose(RTSPSession session)
{
	FskErr err = 0;

	if (!session) return 0;

	if (--session->useCount > 0) {
		FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"in dispose with non-zero useCount");
		return 0;
	}

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"in dispose removing data handlers");
	
	FskNetInterfaceRemoveNotifier(session->interfaceChangeNotifier);
	session->interfaceChangeNotifier = NULL;

	FskThreadRemoveDataHandler(&session->writeDataHandler);
	FskThreadRemoveDataHandler(&session->readDataHandler);

	if (session->waitingForSocket) {
		FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"in dispose waiting for socket");
		session->disposed = true;
		goto bail;
	}

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"in dispose about to dispose everything");

	if (NULL != session->pendingRequests) {
		RTSPRequest request;
		while ((request = FskListRemoveFirst(&session->pendingRequests->head)) != NULL) {
			RTSPRequestDispose(request);
		}
		FskMemPtrDispose(session->pendingRequests);
	}

	if (NULL != session->sentRequests) {
		RTSPRequest request;
		while ((request = FskListRemoveFirst(&session->sentRequests->head)) != NULL) {
			RTSPRequestDispose(request);
		}
		FskMemPtrDispose(session->sentRequests);
	}

	if (NULL != session->pendingResponses) {
		RTSPRequest request;
		while ((request = FskListRemoveFirst(&session->pendingResponses->head)) != NULL) {
			RTSPRequestDispose(request);
		}
		FskMemPtrDispose(session->pendingResponses);
	}

	FskMemPtrDispose(session->theURL);
	FskMemPtrDispose(session->redirect);
	FskStrParsedUrlDispose(session->urlComponents);
	FskMemPtrDispose(session->sessionID);
	FskMemPtrDispose(session->readBuffer);
	FskMemPtrDispose(session->packetBuffer);
	FskMemPtrDispose(session->userAgent);
	FskMemPtrDispose(session->networkType);
	FskMemPtrDispose(session->referer);
	FskMemPtrDispose(session->wapProfile);
	FskMemPtrDispose(session->authCode);
#if RTSP_DEBUG_DROP_PACKETS
	FskMemPtrDispose(session->droppedPacketIndices);
	FskMemPtrDispose(session->droppedPacketsCurrent);
#endif
	SDPSessionDescriptionDispose(session->sdp);
	FskNetSocketClose(session->skt);

	FskTimeCallbackDispose(session->cycleCallback);
	FskTimeCallbackDispose(session->killTimer);

	if (0 != session->nMediaStreams) {
		UInt32 i;
		for (i = 0; i < session->nMediaStreams; ++i) {
			RTSPMediaStream mediaStream = &session->mediaStream[i];
			
			FskThreadRemoveDataHandler(&mediaStream->rtpSocketInfo.dataHandler);
			FskThreadRemoveDataHandler(&mediaStream->rtcpSocketInfo.dataHandler);
			FskNetSocketClose(mediaStream->sktRTP);
			FskNetSocketClose(mediaStream->sktRTCP);
			FskMemPtrDispose(mediaStream->session);
			FskMemPtrDispose(mediaStream->bufferRTP.buffer);
#if RTSP_RECEIVER_REPORTS
			FskMemPtrDispose(mediaStream->lastSenderReport);
#endif
#if SUPPORT_RELIABLE_UDP
			FskTimeCallbackDispose(mediaStream->reliableUDP.emitCB);
#endif
			RTPPacketParserDispose(mediaStream->packetParser);
			RTCPPacketParserDispose(mediaStream->packetParserRTCP);
		}
	}

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"out dispose");

	FskInstrumentedItemDispose(session);
	FskMemPtrDispose(session);

bail:
	return err;
}

/* ----------------------------------------------------------------------------------------------- */

FskErr RTSPRequestNew(RTSPRequest *request)
{
	FskErr err;

	err = FskMemPtrNewClear(sizeof(RTSPRequestRecord), request);
	BAIL_IF_ERR(err);
	err = FskHeaderStructNew(&(*request)->requestHeaders);
	BAIL_IF_ERR(err);
	err = FskMemPtrNew(kRequestBufferSize, &(*request)->buffer);
	BAIL_IF_ERR(err);

	(*request)->bufferSize = kRequestBufferSize;

bail:
	if (err) {
		RTSPRequestDispose(*request);
		*request = 0;
	}
	return err;
}

void RTSPRequestDispose(RTSPRequest request)
{
	if (!request) return;

	FskMemPtrDispose(request->buffer);
	FskHeaderStructDispose(request->requestHeaders);
	FskHeaderStructDispose(request->responseHeaders);
	FskMemPtrDispose(request);
}

FskErr RTSPSessionSetURL(RTSPSession session, char *url)
{
	FskErr	err = 0;

	FskMemPtrDispose(session->theURL);
	FskStrParsedUrlDispose(session->urlComponents);

	session->theURL = FskStrDoCopy(url);
	session->urlComponents = NULL;

	err = FskStrParseUrl(session->theURL, &session->urlComponents);
	if (0 != err) goto bail;

	session->protocol = session->urlComponents->scheme;
	session->host = session->urlComponents->host;
	session->port = session->urlComponents->port;
	session->path = session->urlComponents->path;
	session->params = session->urlComponents->params;

	if (0 == FskStrCompare(session->host, "127.0.0.1")) {
		session->isLocal = 1;
	}

bail:
	return err;
}

FskErr RTSPSessionSetDefaultPort(RTSPSession session, UInt32 port)
{
	session->defaultPort = port;
	return 0;
}

FskErr RTSPSessionSetRefCon(RTSPSession session, void *refCon)
{
	session->clientRefCon = refCon;
	return 0;
}

FskErr RTSPSessionSetResponseHeaderCallback(RTSPSession session, RTSPSessionResponseHeaderCallback headerCB)
{
	session->clientHeaderCB = headerCB;
	return 0;
}

FskErr RTSPSessionSetStateChangeCallback(RTSPSession session, RTSPSessionStateChangeCallback stateChangeCB)
{
	session->stateChangeCB = stateChangeCB;
	return 0;
}

FskErr RTSPSessionSetProxy(RTSPSession session, char *host, UInt32 addr, UInt32 port)
{
	FskErr err = kFskErrNone;

	if (host) {
		FskNetStringToIPandPort(host, (int *)&session->proxyAddr, (int *)&session->proxyPort);
	}
	else {
		session->proxyAddr = addr;
		session->proxyPort = port;
	}

	return err;
}

FskErr RTSPSessionSetAuthCode(RTSPSession session, char *authCode)
{
	if (authCode) {
		session->authCode = FskStrDoCopy(authCode);
	}
	return 0;
}

FskErr RTSPSessionPlay(RTSPSession session, SInt32 playTimeInMS)
{
	UInt32 i;
	FskErr err = 0;

	// Verify that all the media streams are ready
	for (i = 0; i < session->nMediaStreams; ++i) {
		if (!session->mediaStream[i].ready) {
			err = kFskErrRTSPSessionBadState;
			goto bail;
		}
	}
	
	err = doPlay(session, playTimeInMS);

bail:
	return err;
}

FskErr RTSPSessionPause(RTSPSession session)
{
	FskErr err = 0;

	if (session->sessionState < kRTSPClientStatePlaying) {
		err = kFskErrRTSPSessionBadState;
		goto bail;
	}

	err = doPause(session);

bail:
	return err;
}

FskErr RTSPSessionOptions(RTSPSession session)
{
	FskErr err = 0;

	err = doOptions(session);

	return err;
}

FskErr RTSPSessionTeardown(RTSPSession session)
{
	FskErr err = 0;

	if (session->sessionState < kRTSPClientStateInit) {
		err = kFskErrRTSPSessionBadState;
		goto bail;
	}

	err = doTeardown(session);

bail:
	return err;
}

FskErr RTSPSessionSetSDP(RTSPSession session, void *sdp, UInt32 sdpSize)
{
// Two ways to approach this:
// 1. fake a DESCRIBE response and pass it to sRequestComplete() for handling
// 2. parse SDP here and extract a control URL, then set URL and connect as normal
// QuickTime Player, when fed an SDP, seems to do #2, though it seems like #1
// should also work.

// Sprint only wants us to use method #1

	FskErr err = 0;

	{
		// method #1
		RTSPRequest request;

		err = RTSPRequestNew(&request);
		BAIL_IF_ERR(err);
		
		request->responseBody = (char *)sdp;
		request->responseBodySize = sdpSize;
		FskStrCopy(request->method, "DESCRIBE");
		
		err = sRequestComplete(session, request);
		if (!err) {
			err = RTSPSessionConnectToHost(session);
		}

		RTSPRequestDispose(request);
		goto bail;
	}
	
#if 0
	{
		// method #2
		SDPSessionDescription desc = 0;
		SDPAttribute ctrl;
		
		err = SDPSessionDescriptionNewFromMemory((UInt8*)sdp, sdpSize, &desc);
		BAIL_IF_ERR(err);
		
		ctrl = SDPFindSessionAttribute(desc, "control");
		err = kFskErrRTSPSessionBadURL;
		if (ctrl) {
			if (NULL != FskStrStr(ctrl->value, "://")) {
				err = RTSPSessionSetURL(session, ctrl->value);
			}
			else if (FskStrCompare(ctrl->value, "*") == 0) {
				// RTSP RFC mentions that this is an acceptable case in C.1.1
				// Normally a client would only receive an SDP that looks like this
				// as a DESCRIBE response, in which case we should use the Content-Base
				// field (session->host is already filled, so this case is already handled).
				//
				// This case should not normally occur unless the user somehow has an SDP file
				// with no control URL.  In this case, cobble together a URL from other
				// SDP fields.  Please note that this has only been tested against
				// SDP files from the Darwin Streaming Server.
				char *prefixStr = "rtsp://", *urlStr;
				
				FskMemPtrNew(FskStrLen(prefixStr) + FskStrLen(desc->origin.addressStr) + FskStrLen(desc->name) + 1, &urlStr);
				FskStrCopy(urlStr, prefixStr);
				FskStrCat(urlStr, desc->origin.addressStr);
				FskStrCat(urlStr, desc->name);
				
				err = RTSPSessionSetURL(session, urlStr);
				FskMemPtrDispose(urlStr);
			}
		}
		if (0 == err)
			err = RTSPSessionConnectToHost(session);
		SDPSessionDescriptionDispose(desc);
	}
#endif
	
bail:
	return err;
}

FskErr RTSPSessionSetUserAgent(RTSPSession session, char *userAgent)
{
	FskErr err = 0;
	
	session->userAgent = FskStrDoCopy(userAgent);
	
	return err;
}

FskErr RTSPSessionSetNetworkType(RTSPSession session, char *networkType)
{
	FskErr err = 0;
	
	session->networkType = FskStrDoCopy(networkType);
	
	return err;
}

FskErr RTSPSessionSetReferer(RTSPSession session, char *referer)
{
	FskErr err = 0;
	
	session->referer = FskStrDoCopy(referer);
	
	return err;
}

FskErr RTSPSessionSetWapProfile(RTSPSession session, char *wapProfile)
{
	FskErr err = 0;
	
	session->wapProfile = FskStrDoCopy(wapProfile);
	
	return err;
}

FskErr RTSPSessionSetIdleTimeout(RTSPSession session, UInt32 secs)
{
	if (!session) goto bail;

	session->idleKillSecs = secs;
	sResetKillTimer(session);
	
bail:
	return kFskErrNone;
}

FskErr RTSPSessionStart(RTSPSession session)
{
	FskErr err = 0;

	err = doDescribe(session);

	return err;
}

void sResetKillTimer(RTSPSession session)
{
	if (!session)
		return;

	if (session->idleKillSecs && !session->killTimer) {
		FskTimeCallbackNew(&session->killTimer);
	}
	
	if (session->killTimer) {
		if (!session->idleKillSecs)
			FskTimeCallbackRemove(session->killTimer);
		else
			FskTimeCallbackScheduleFuture(session->killTimer, session->idleKillSecs, 0, sSessionKillMe, session);
	}
}

void sSessionKillMe(FskTimeCallBack cb, FskTime when, void *param)
{
	RTSPSession session = (RTSPSession)param;
	
	if (!session)
		return;

	sUpSessionUse(session);

	session->status.lastErr = kFskErrTimedOut;
	sDoStateChange(session, kRTSPClientStateError);
	
	sDownSessionUse(session);
}

#if RTSP_RECEIVER_REPORTS

// init_seq and update_seq are copied almost exactly from RFC 3550 A.1
// They are used here for RR statistics but are also meant for packet validation

static void init_seq(RTSPStreamStats s, UInt16 seq)
{
	s->base_seq = seq;
	s->max_seq = seq;
	s->bad_seq = RTP_SEQ_MOD + 1;   /* so seq == bad_seq is false */
	s->cycles = 0;
	s->received = 0;
	s->received_prior = 0;
	s->expected_prior = 0;
	/* other initialization */
}

static FskErr update_seq(RTSPStreamStats s, UInt16 seq)
{
	UInt16 udelta = seq - s->max_seq;

	/*
	 * Source is not valid until MIN_SEQUENTIAL packets with
	 * sequential sequence numbers have been received.
	 */
	if (s->probation) {
		/* packet is in sequence */
		if (seq == s->max_seq + 1) {
			s->probation--;
			s->max_seq = seq;
			if (s->probation == 0) {
				init_seq(s, seq);
				s->received++;
				return 0;
			}
		} else {
			s->probation = MIN_SEQUENTIAL - 1;
			s->max_seq = seq;
		}
		return kFskErrRTSPBadPacket;
	} else if (udelta < MAX_DROPOUT) {
		/* in order, with permissible gap */
		if (seq < s->max_seq) {
			/*
			 * Sequence number wrapped - count another 64K cycle.
			 */
			s->cycles += RTP_SEQ_MOD;
		}
		s->max_seq = seq;
	} else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
		/* the sequence number made a very large jump */
		if (seq == s->bad_seq) {
			/*
			 * Two sequential packets -- assume that the other side
			 * restarted without telling us so just re-sync
			 * (i.e., pretend this was the first packet).
			 */
			init_seq(s, seq);
		}
		else {
			s->bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
			return kFskErrRTSPBadPacket;
		}
	} else {
		/* duplicate or reordered packet */
	}
	s->received++;
	return 0;
}
#endif

FskErr RTSPSessionHandleRTPPacket(RTPPacket packet, void *refcon)
{
	FskErr err = 0;
	RTSPMediaStream mediaStream = refcon;
	RTSPSession session = mediaStream->rtspSession;

#if RTSP_RECEIVER_REPORTS
	RTSPStreamStats s = &(mediaStream->stats);
	UInt16 seq = (UInt16)packet->sequenceNumber;
	SInt32 transit, d, arrival;
	FskTimeRecord currTime;
#endif

#if SUPPORT_RELIABLE_UDP
	if (session->useReliableUDP) {
		logPacketForReliableUDP(session, mediaStream, packet);
	}
#endif

	if (session->useDynamicBufferingRate && session->needLateToleranceUpdate && (mediaStream->packetParser->nextPacketNumber > 3)) {
		char s[64];
		session->lateTolerance = 2.0;
		snprintf(s, 64, "late-tolerance=%f", session->lateTolerance);
		doSetParameter(session, "x-transport-options", s);
		sSessionCycle(session);
		session->needLateToleranceUpdate = false;
		session->lateTolerance = 10;
	}

#if RTSP_RECEIVER_REPORTS
	// ignore packets that don't match the stated SSRC (if present)
	if (mediaStream->ssrcIsExplicit && mediaStream->ssrc != packet->SSRC) {
		LOG2("ignoring SSRC mismatch", mediaStream->ssrc, packet->SSRC);
		return kFskErrRTSPBadPacket;
	}
	// always initialize the sequence statistics the first time,
	// reinitialize for each new SSRC (if no SSRC was given over RTSP)
	if (!mediaStream->sequenceInitialized || mediaStream->ssrc != packet->SSRC) {
		// do initialization
		LOG2("InitSeq for SSRC/pType", packet->SSRC, packet->payloadType);
		init_seq(s, seq);
		s->max_seq = seq - 1;
		s->probation = MIN_SEQUENTIAL;
		mediaStream->sequenceInitialized = true;
		mediaStream->ssrc = packet->SSRC;
	}

	// It's not a show stopper if this fails
	(void)update_seq(s, seq);

	// alter the sequence number to be a 32-bit value to minimize overflow
	packet->sequenceNumber = s->cycles + seq;

	// ignore any stale packets
	// NOTE: this doesn't actually reject out-of-order packets at large,
	//       just anything prior to the PLAY response.
	if (packet->sequenceNumber < mediaStream->seq) {
		LOG2("ignoring stale packet", packet->sequenceNumber, mediaStream->seq);
		return kFskErrRTSPBadPacket;
	}

#if 0
	{
	char msg[64];
	sprintf(msg, "Got %u: seq,time", packet->payloadType);
	LOG2(msg, packet->sequenceNumber, packet->timestamp);
	}
#endif
	
	// Update jitter calculation
	if (mediaStream->lastSenderReport) {
		FskTimeGetNow(&currTime);
		arrival = (SInt32)(FskTimeInMS(&currTime) * ((double)mediaStream->rtpTimeScale / kFskTimeMsecPerSec));

		transit = arrival - packet->timestamp;
		d = transit - s->transit;
		s->transit = transit;
		if (d < 0) d = -d;
#if JITTER_ALT_CALC
		s->jitter += d - ((s->jitter + 8) >> 4);
#else
		s->jitter += (UInt32)((1./16.) * ((double)d - s->jitter));
#endif
	}
#endif
	
	return err;
}

FskErr RTSPSessionHandleRTCPPacket(RTCPPacket packet, void *refcon)
{
	FskErr err = 0;
	RTSPMediaStream mediaStream = refcon;

#if RTSP_RECEIVER_REPORTS
	// Save a copy of the most recent SR
	if (kRTCPPacketTypeSenderReport == packet->header.PT) {
		if (mediaStream->ssrc == packet->flavor.sr.SSRC) {
			UInt32 srSize = sizeof(RTCPSenderReportRecord) + (packet->header.count) * sizeof(RTCPReportBlockRecord);
			if (mediaStream->lastSenderReport)
				FskMemPtrDispose(mediaStream->lastSenderReport);
			err = FskMemPtrNew(srSize, (FskMemPtr *)&(mediaStream->lastSenderReport));
			BAIL_IF_ERR(err);
			FskMemMove(mediaStream->lastSenderReport,
				&packet->flavor.sr, srSize);			
			FskTimeGetNow(&mediaStream->lastSenderReportTime);
		}
	}
#endif

bail:
	return err;
}

FskErr RTSPSessionAckReliableUDP(RTSPSession session, RTSPMediaStream mediaStream, UInt8 *ackPacket, UInt32 ackPacketLength)
{
	FskErr err = 0;
	int packetSize;

	err = FskNetSocketSendUDP(mediaStream->sktRTCP, ackPacket, ackPacketLength, &packetSize, mediaStream->serverIPRTP, mediaStream->serverPortRTCP);

	return err;
}

static FskErr sPrepareRTSPClientRequest(RTSPSession session, RTSPRequest request, char *control)
{
	FskErr err = kFskErrNone;
	UInt32 port;

	port = getSessionPort(session);
	FskStrCopy(request->buffer, request->method);
	FskStrCat(request->buffer, " ");
	// Only use the given control as-is if it's fully qualified, or if it's the special-case of "*"
	if (control && (FskStrStr(control, "://") || FskStrCompare(control, "*") == 0)) {
		FskStrCat(request->buffer, control);
	}
	else {
		FskStrCat(request->buffer, session->protocol);
		FskStrCat(request->buffer, "://");
		FskStrCat(request->buffer, session->host);
		if (port != 554)	{
			char portNum[12];
			FskStrCat(request->buffer, ":");
			FskStrNumToStr(port, portNum, sizeof(portNum));
			FskStrCat(request->buffer, portNum);
		}
		FskStrCat(request->buffer, "/");
		FskStrCat(request->buffer, session->path);
		if (control && request->buffer[FskStrLen(request->buffer)-1] != '/') {
			FskStrCat(request->buffer, "/");
			FskStrCat(request->buffer, control);
		}
		if (NULL != session->params) {
			FskStrCat(request->buffer, "?");
			FskStrCat(request->buffer, session->params);
		}
	}
	FskStrCat(request->buffer, " RTSP/1.0\r\n");

	if (0 != session->userAgent) {
		FskHeaderAddString("User-Agent", session->userAgent, request->requestHeaders);
	}
	if (0 != session->networkType) {
		FskHeaderAddString("x-network-type", session->networkType, request->requestHeaders);
	}
	if (0 != session->referer) {
		FskHeaderAddString("Referer", session->referer, request->requestHeaders);
	}
	if (0 != session->wapProfile) {
		FskHeaderAddString("x-wap-profile", session->wapProfile, request->requestHeaders);
	}
	if (0 != session->authCode) {
		char *value;
		
		err = FskMemPtrNew(FskStrLen(session->authCode) + 32, &value);
		BAIL_IF_ERR(err);
		
		FskStrCopy(value, "Basic ");
		FskStrCat(value, session->authCode);
		FskHeaderAddString("Proxy-Authorization", value, request->requestHeaders);
		FskMemPtrDispose(value);
	}

	// Format the headers appropriately and dump into the send buffer
	request->bufferPos = FskStrLen(request->buffer);
	request->bufferPos += FskHeaderGenerateOutputBlob(&request->buffer[request->bufferPos], kRequestBufferSize - request->bufferPos,
		true, request->requestHeaders);

	request->bufferAmt = request->bufferPos;
	request->bufferPos = 0;
	
bail:
	return err;
}

FskErr sPrepareRTSPClientResponse(RTSPSession session, RTSPRequest request, FskHeaders *responseHeaders, UInt32 cSeq)
{
	char seq[32];
	char *sessionID = NULL;
	
	// Preflight the session id (BUGZID 9141)
	// We've found cases where the server sends the client OPTIONS requests even
	// before the session has been instantiated.
	// But it is okay to send a response if the server didn't provide a cSeq number
	if (kInvalidCSeq != cSeq) {
		sessionID = session->sessionID;
		if (0 == sessionID) {
			char *hdr;
			if (0 != (hdr = FskHeaderFind("Session", responseHeaders))) {
				sessionID = hdr;
			}
			if (0 == sessionID)
				goto bail;
		}
	}

	// We don't support any S->C (server to client) requests
	FskStrCopy(request->buffer, "RTSP/1.0 501 Not Implemented\r\n");
	if (kInvalidCSeq != cSeq) {
		FskStrNumToStr(cSeq, seq, sizeof(seq));
		FskHeaderAddString("CSeq", seq, request->requestHeaders);
		FskHeaderAddString("Session", sessionID, request->requestHeaders);
	}
	
	request->bufferPos = FskStrLen(request->buffer);
	request->bufferPos += FskHeaderGenerateOutputBlob(&request->buffer[request->bufferPos], kRequestBufferSize - request->bufferPos,
		true, request->requestHeaders);

	request->bufferAmt = request->bufferPos;
	request->bufferPos = 0;
	
bail:
	return 0;
}

FskErr doDescribe(RTSPSession session)
{
	char seq[80];
	RTSPRequest request = 0;
	FskErr err = 0;

	err = RTSPRequestNew(&request);
	BAIL_IF_ERR(err);

	sDoStateChange(session, kRTSPClientStateDescribe);

	// Enqueue the request
	request->cSeq = session->cSeq;
	FskStrCopy(request->method, "DESCRIBE");

	FskStrNumToStr(session->cSeq++, seq, sizeof(seq));
	FskHeaderAddString("CSeq", seq, request->requestHeaders);
	FskHeaderAddString("Accept", "application/sdp", request->requestHeaders);
	err = sPrepareRTSPClientRequest(session, request, NULL);
	BAIL_IF_ERR(err);

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)request->buffer);

	FskListAppend(&session->pendingRequests->head, request);
	
	sSessionCycle(session);

	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)"queued DESCRIBE");

bail:
	if (err) {
		RTSPRequestDispose(request);
	}
	return err;
}

FskErr doSetup(RTSPSession session, RTSPMediaStream mediaStream)
{
	FskErr err = 0;
	SDPAttribute controlAttribute;
	RTSPRequest request = NULL;
	char tmp[128], tmp1[10];

	sDoStateChange(session, kRTSPClientStateInit);

	// We require aggregate control
	controlAttribute = SDPFindMediaAttributeWithException(mediaStream->mediaDescription, "control", "*");

	err = RTSPRequestNew(&request);
	BAIL_IF_ERR(err);

	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)"***SETUP***");

	// Enqueue the request
	request->cSeq = session->cSeq++;
	FskStrCopy(request->method, "SETUP");
	request->param = mediaStream;

	FskStrNumToStr(request->cSeq, tmp, sizeof(tmp));
	FskHeaderAddString("CSeq", tmp, request->requestHeaders);
	FskStrCopy(tmp, "RTP/AVP");
	if (session->rtpOverTCP)
		FskStrCat(tmp, "/TCP");
	FskStrCat(tmp, ";unicast");
	if (session->rtpOverTCP)
		FskStrCat(tmp, ";interleaved=");
	else
		FskStrCat(tmp, ";client_port=");
	mediaStream->clientPortRTP = session->nextPort;
	mediaStream->clientPortRTCP = session->nextPort+1;
	FskStrNumToStr(session->nextPort++, tmp1, sizeof(tmp1));
	FskStrCat(tmp, tmp1);
	FskStrCat(tmp, "-");
	FskStrNumToStr(session->nextPort++, tmp1, sizeof(tmp1));
	FskStrCat(tmp, tmp1);
	FskHeaderAddString("Transport", tmp, request->requestHeaders);
	if (session->sessionID) {
		FskHeaderAddString("Session", session->sessionID, request->requestHeaders);
	}

#if SUPPORT_DYNAMIC_BUFFERING_RATE
	if (session->useDynamicBufferingRate) {
		FskHeaderAddString("x-dynamic-rate", "1", request->requestHeaders);
		FskHeaderAddString("x-transport-options", "late-tolerance=2.0", request->requestHeaders);
	}
#endif
#if SUPPORT_RELIABLE_UDP
	if (session->useReliableUDP)
		FskHeaderAddString("x-retransmit", "our-retransmit", request->requestHeaders);
#endif

	err = sPrepareRTSPClientRequest(session, request, controlAttribute ? controlAttribute->value : NULL);
	BAIL_IF_ERR(err);

	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)request->buffer);

	FskListAppend(&session->pendingRequests->head, request);

	sSessionCycle(session);

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"queued SETUP");

bail:
	if (err)
		RTSPRequestDispose(request);
		
	return err;
}

FskErr doPlay(RTSPSession session, SInt32 playTimeInMS)
{
	FskErr err = 0;
	RTSPRequest request;
	char seq[80];
	UInt32 i;
	double playTime;

	// Flush the internal packet parser handler state
	for (i = 0; i < session->nMediaStreams; ++ i) {
		RTPPacketParserFlush(session->mediaStream[i].packetParser);
		
		// Force a reset of the 'rtpTime' for this stream, since the RTSP session
		// appears to reset the 'rtpTime' when seeking to the beginning of the stream
		if (playTimeInMS <= 0) {
			session->mediaStream[i].rtpTime = -1;
		}

		// Reset the reliable udp to force a re-sync
#if SUPPORT_RELIABLE_UDP
		if (session->useReliableUDP) {
			initReliableUDP(session, &session->mediaStream[i]);
		}
#endif
	}
	
	sDoStateChange(session, kRTSPClientStateInit);

	if (-1 == playTimeInMS) {
		session->startTime = 0;
	}
	else {
		session->startTime = (double)playTimeInMS / 1000;
		LOG1("session->startTime*1000=", playTimeInMS);
	}


	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)"***PLAY***");

	err = RTSPRequestNew(&request);
	BAIL_IF_ERR(err);

	// Enqueue the request
	request->cSeq = session->cSeq++;
	FskStrCopy(request->method, "PLAY");

	FskStrNumToStr(request->cSeq, seq, sizeof(seq));
	FskHeaderAddString("CSeq", seq, request->requestHeaders);
	if (session->sessionID) {
		FskHeaderAddString("Session", session->sessionID, request->requestHeaders);
		
		// Omit the range header for a live (i.e. unseekable) stream
		if (-1 != playTimeInMS) {
			playTime = playTimeInMS / 1000.0;
			snprintf(seq, 80, "npt=%f-", playTime);
			FskHeaderAddString("Range", seq, request->requestHeaders);
		}
	}

	// The x-prebuffer header seems to instruct the DSS to return up to "maxtime"
	// samples before the request time.  When this occurs, the first sample appears
	// to be a key frame.
	FskHeaderAddString("x-prebuffer", "maxtime=1.000", request->requestHeaders);

	if (session->useDynamicBufferingRate) {
		snprintf(seq, 80, "late-tolerance=%f", session->lateTolerance);
		FskHeaderAddString("x-transport-options", seq, request->requestHeaders);
		session->needLateToleranceUpdate = true;
	}

	err = sPrepareRTSPClientRequest(session, request, NULL);
	BAIL_IF_ERR(err);

	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)request->buffer);

	FskListAppend(&session->pendingRequests->head, request);

	sSessionCycle(session);

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"queued PLAY");

bail:
	return err;
}

FskErr doPause(RTSPSession session)
{
	FskErr err = 0;
	RTSPRequest request;
	char seq[80];
	UInt16 j;

	sDoStateChange(session, kRTSPClientStateInit);

	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)"***PAUSE***");

	err = RTSPRequestNew(&request);
	BAIL_IF_ERR(err);

	// Enqueue the request
	request->cSeq = session->cSeq++;
	FskStrCopy(request->method, "PAUSE");

	FskStrNumToStr(request->cSeq, seq, sizeof(seq));
	FskHeaderAddString("CSeq", seq, request->requestHeaders);
	if (session->sessionID)
		FskHeaderAddString("Session", session->sessionID, request->requestHeaders);
	err = sPrepareRTSPClientRequest(session, request, NULL);
	BAIL_IF_ERR(err);

	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)request->buffer);

	FskListAppend(&session->pendingRequests->head, request);

	// Invalidate the media stream sequence numbers, since the 'PLAY' response will
	// return a new set.  This way, clients will reject any stale packets, since the
	// packet sequence numbers stored will be lower.
	for (j = 0; j < session->nMediaStreams; ++j) {
		RTSPMediaStream mediaStream = &session->mediaStream[j];

		mediaStream->seq = 0x7FFFFFFF;
	}

	sSessionCycle(session);

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"queued PAUSE");
	
	session->waitingForPauseResponse = true;

bail:
	return err;
}

FskErr doTeardown(RTSPSession session)
{
	FskErr err = 0;
	RTSPRequest request;
	char seq[80];

	sDoStateChange(session, kRTSPClientStateStart);

	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)"***TEARDOWN***");

	err = RTSPRequestNew(&request);
	BAIL_IF_ERR(err);

	// Enqueue the request
	request->cSeq = session->cSeq++;
	FskStrCopy(request->method, "TEARDOWN");

	FskStrNumToStr(request->cSeq, seq, sizeof(seq));
	FskHeaderAddString("CSeq", seq, request->requestHeaders);
	if (session->sessionID)
		FskHeaderAddString("Session", session->sessionID, request->requestHeaders);
	err = sPrepareRTSPClientRequest(session, request, NULL);
	BAIL_IF_ERR(err);

	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)request->buffer);

	FskListAppend(&session->pendingRequests->head, request);

	sSessionCycle(session);

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"queued TEARDOWN");

bail:
	return err;
}

FskErr doOptions(RTSPSession session)
{
	FskErr err = 0;
	RTSPRequest request = NULL;
	char seq[80];

	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)"***OPTIONS***");

	err = RTSPRequestNew(&request);
	BAIL_IF_ERR(err);

	// Enqueue the request
	request->cSeq = session->cSeq++;
	FskStrCopy(request->method, "OPTIONS");

	FskStrNumToStr(request->cSeq, seq, sizeof(seq));
	FskHeaderAddString("CSeq", seq, request->requestHeaders);
	err = sPrepareRTSPClientRequest(session, request, "*");
	BAIL_IF_ERR(err);

	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)request->buffer);

	FskListAppend(&session->pendingRequests->head, request);

	sSessionCycle(session);

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"queued OPTIONS");

bail:
	if (err)
		RTSPRequestDispose(request);
		
	return err;
}

FskErr doSetParameter(RTSPSession session, char *name, char *value)
{
	FskErr err = 0;
	RTSPRequest request;
	char seq[80];
	
	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)"***SET_PARAMETER***");

	err = RTSPRequestNew(&request);
	BAIL_IF_ERR(err);

	// Enqueue the request
	request->cSeq = session->cSeq++;
	FskStrCopy(request->method, "SET_PARAMETER");

	FskStrNumToStr(request->cSeq, seq, sizeof(seq));
	FskHeaderAddString("CSeq", seq, request->requestHeaders);
	if (session->sessionID) {
		FskHeaderAddString("Session", session->sessionID, request->requestHeaders);
	}
	FskHeaderAddString(name, value, request->requestHeaders);

	err = sPrepareRTSPClientRequest(session, request, NULL);
	BAIL_IF_ERR(err);

	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)request->buffer);

	FskListAppend(&session->pendingRequests->head, request);

	sSessionCycle(session);

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"queued SET_PARAMETER");

bail:
	return err;
}

static FskErr sRequestComplete(RTSPSession session, RTSPRequest request)
{
	FskErr err = kFskErrNone;

	// Look at the headers
#if SUPPORT_INSTRUMENTATION
	FskHeaderIterator iter;
	FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)"***** Headers back from request *****");

	iter = FskHeaderIteratorNew(request->responseHeaders);
	while (NULL != iter) {
		char line[1024];
		FskHeaderGenerateOutputLine(iter, line, 1024);
		FskInstrumentedItemSendMessageDebug(session, kRTSPInstrMsgTrace, (void *)line);
		iter = FskHeaderIteratorNext(iter);
	}
	FskHeaderIteratorDispose(iter);
#endif

	if (0 == FskStrCompare(request->method, "DESCRIBE")) {
		RTSPMediaStream mediaStream;
		SDPMediaDescription mediaDescription;
		UInt32 i;
		char *URL = NULL;

		// Parse the SDP
		err = SDPSessionDescriptionNewFromMemory((UInt8*)request->responseBody, request->responseBodySize, &session->sdp);
		BAIL_IF_ERR(err);

		// Populate the media streams
		session->nMediaStreams = 0;
		// XXX - Orb server potentially mixes up streams and ports.
		// If we happen to SETUP the second stream in the SDP first using
		// lower port numbers, it sends the first stream's data over those ports
		// and the second stream's data over the higher ports.
		
		// Symptom: All packets are rejected by UDP packet handler due to payloadType mismatch.
		// Workaround: Switch stream order here to match stream order in SDP.
		mediaDescription = SDPFindMediaDescription(session->sdp, "video");
		if (0 != mediaDescription) {
			mediaStream = &session->mediaStream[session->nMediaStreams];
			mediaStream->rtspSession = session;
			mediaStream->streamType = kRTSPMediaStreamVideo;
			mediaStream->mediaDescription = mediaDescription;
			mediaStream->seq =0x7FFFFFFF;
			mediaStream->rtpTime = -1;
#if RTSP_RECEIVER_REPORTS
			mediaStream->clientSSRC = FskRandom();
#endif
			++session->nMediaStreams;
		}
		mediaDescription = SDPFindMediaDescription(session->sdp, "audio");
		if (0 != mediaDescription) {
			mediaStream = &session->mediaStream[session->nMediaStreams];
			mediaStream->rtspSession = session;
			mediaStream->streamType = kRTSPMediaStreamAudio;
			mediaStream->mediaDescription = mediaDescription;
			mediaStream->seq =0x7FFFFFFF;
			mediaStream->rtpTime = -1;
#if RTSP_RECEIVER_REPORTS
			mediaStream->clientSSRC = FskRandom();
#endif
			++session->nMediaStreams;
		}

		if (0 == session->nMediaStreams) {
			err = kFskErrRTSPNoMediaStreams;
			goto bail;
		}

#if RTSP_DEBUG_DROP_PACKETS
		FskMemPtrNewClear(sizeof(UInt16) * session->nMediaStreams, (FskMemPtr *)&session->droppedPacketsCurrent);
#endif

		// Extract control URL from SDP if appropriate
		// Try in order listed in Appendix C.1.1 of RFC 2326
		{
		SDPAttribute ctrl = SDPFindSessionAttribute(session->sdp, "control");
		if (ctrl && FskStrCompare(ctrl->value, "*") != 0)
			URL = ctrl->value;
		}
		if (!URL && request->responseHeaders) {
			// Read URL from "Content-Base" response header
			URL = FskHeaderFind("Content-Base", request->responseHeaders);
		}
		if (!URL && request->responseHeaders) {
			// Read URL from "Content-Location" response header
			URL = FskHeaderFind("Content-Location", request->responseHeaders);
		}
		if (URL) {
			UInt32 len;
			RTSPSessionSetURL(session, URL);
			len = FskStrLen(session->path);
			if (session->path && session->path[len-1] == '/') {
				session->path[len-1] = 0;
			}
		}

		// Check for live stream
		{
		SDPAttribute attribute = SDPFindSessionAttribute(session->sdp, "range");
		if (0 != attribute) {
			char *value, *parts[4];
			UInt16 nParts;
			double range = 0;
			
			value = FskStrDoCopy(attribute->value);
			splitToken(value, &nParts, '=', &parts[0]);
			
			if (0 == FskStrCompareCaseInsensitive("npt", parts[0])) {
				splitToken(parts[1], &nParts, '-', &parts[2]);
				if (2 == nParts) {
					double start, end;
					start = FskStrToD(FskStrStripHeadSpace(parts[2]), 0);
					end = FskStrToD(FskStrStripHeadSpace(parts[3]), 0);
					range = end - start;
				}
			}
			FskMemPtrDispose(value);
			if (0 == range)
				session->isLive = true;
		}
		}
		
		// Check for Darwin and Orb servers
		if (NULL != request->responseHeaders) {
			char *server;
			server = FskHeaderFind("Server", request->responseHeaders);
			if (NULL != server) {
				char *header, *p;
				header = FskStrDoCopy(server);
				p = FskStrStripHeadSpace(header);
				if ((p[0] == 'D' && p[1] == 'S' && p[2] == 'S') || (p[0] == 'Q' && p[1] == 'T' && p[2] == 'S' && p[3] == 'S'))
					session->isDarwin = true;
//				else
//				if (0 == FskStrCompare(p, "Orbiter"))
//					session->isOrb = true;

				FskMemPtrDispose(header);
			}
		}
		
		// Check for Reliable UDP support with dynamic streaming rate
		if (0 != request->responseHeaders) {
			char *header, *p;
			Boolean supportsReliableUDP = false, supportsDynamicRate=false;
			header = FskHeaderFind("X-Accept-Retransmit", request->responseHeaders);
			if (NULL != header) {
				p = FskStrStripHeadSpace(header);
				if (0 == FskStrCompareCaseInsensitive(p, "our-retransmit"))
					supportsReliableUDP = true;
			}
			header = FskHeaderFind("X-Accept-Dynamic-Rate", request->responseHeaders);
			if (NULL != header) {
				p = FskStrStripHeadSpace(header);
				if (1 == FskStrToNum(p))
					supportsDynamicRate = true;
			}

			// Orb lies - even though it indicates that reliable udp is supported, attempts to use it result
			// in a 461 unsupported transport error
			if (session->isOrb)
				supportsReliableUDP = false;

#if SUPPORT_RELIABLE_UDP
			if (!session->rtpOverTCP)
				session->useReliableUDP = (!session->disableReliableUDP && supportsReliableUDP && supportsDynamicRate);
#endif
#if SUPPORT_DYNAMIC_BUFFERING_RATE
			session->useDynamicBufferingRate = supportsDynamicRate && session->useReliableUDP;
#endif
		}

		// Initialize the packet parsers
		for (i = 0; i < session->nMediaStreams; ++i) {
			SDPMediaFormat mediaFormat;
			mediaStream = &session->mediaStream[i];
			mediaFormat = mediaStream->mediaDescription->formatList->head;
			if (NULL != mediaFormat) {
				SDPAttribute attribute;
				char *tmp = NULL, *p[3] = {0};
				UInt16 nParts;

				// Find the matching rtpmap attribute for the media packet parser
				attribute = SDPFindMediaAttribute(mediaStream->mediaDescription, "rtpmap");
				if (NULL != attribute) {
					tmp = FskStrDoCopy(attribute->value);
					splitToken(tmp, &nParts, ' ', &p[0]);
					if (mediaFormat->payloadType == (UInt32)FskStrToNum(p[0])) {
						splitToken(p[1], &nParts, '/', &p[0]);
					}
					else {
						// maybe it's a statically defined payload type (e.g. QCELP)
						// Reset the codec name and let the packet parsers
						// decide based on mediaDescription
						p[0] = NULL;
					}
				}
				err = RTPPacketParserNew(&mediaStream->packetParser, mediaStream->mediaDescription, p[0]);
				mediaStream->payloadType = mediaFormat->payloadType;
				if (0 == err) {
					RTPPacketParserSetSessionReceivePacketCallback(mediaStream->packetParser, RTSPSessionHandleRTPPacket);
					RTPPacketParserSetSessionRefCon(mediaStream->packetParser, mediaStream);
#if RTSP_RECEIVER_REPORTS
					err = RTPPacketParserGetInfo(mediaStream->packetParser, kRTPPacketParserSelectorRTPTimeScale, &mediaStream->rtpTimeScale, NULL);
#endif
				}
				FskMemPtrDispose(tmp);
				BAIL_IF_ERR(err);
			}

			// Instantiate the RTCP packet parser
			err = RTCPPacketParserNew(&mediaStream->packetParserRTCP);
			BAIL_IF_ERR(err);
			RTCPPacketParserSetSessionReceivePacketCallback(mediaStream->packetParserRTCP, RTSPSessionHandleRTCPPacket);
			RTCPPacketParserSetSessionRefCon(mediaStream->packetParserRTCP, mediaStream);
		}

		// We've got the session description, so advance to SETUP
		doSetup(session, &session->mediaStream[0]);
	}
	else if (0 == FskStrCompare(request->method, "SETUP")) {
		char *hdr;
		UInt32 i;
		RTSPMediaStream mediaStream = (RTSPMediaStream)request->param;

		if (0 != (hdr = FskHeaderFind("Session", request->responseHeaders))) {
			mediaStream->session = FskStrDoCopy(hdr);
			FskMemPtrDispose(session->sessionID);
			session->sessionID = FskStrDoCopy(hdr);
		}
		if (0 != (hdr = FskHeaderFind("Transport", request->responseHeaders))) {
			char *tmp;
			UInt16 nParts;
			if (copyAttributeValue(hdr, "ssrc", &tmp)) {
				// Ignore SSRC values given by Darwin Streaming Server when content is live
				if (session->isDarwin && session->isLive) {
				}
				else {
					mediaStream->ssrc = FskStrHexToNum(tmp, FskStrLen(tmp));
					mediaStream->ssrcIsExplicit = true;
				}
				FskMemPtrDispose(tmp);
			}
			if (copyAttributeValue(hdr, "interleaved", &tmp)) {
				char *p[2];
				nParts = 2;
				splitToken(tmp, &nParts, '-', &p[0]);
				mediaStream->rtpChannel = FskStrToNum(p[0]);
				mediaStream->rtcpChannel = FskStrToNum(p[1]);
				FskMemPtrDispose(tmp);
			}
			if (copyAttributeValue(hdr, "client_port", &tmp)) {
				char *p[2];
				nParts = 2;
				splitToken(tmp, &nParts, '-', &p[0]);
				// ignore client_port completely and use our own in doSetup()
				FskMemPtrDispose(tmp);
			}
			if (copyAttributeValue(hdr, "server_port", &tmp)) {
				char *p[2];
				nParts = 2;
				splitToken(tmp, &nParts, '-', &p[0]);
				mediaStream->serverPortRTP = FskStrToNum(p[0]);
				mediaStream->serverPortRTCP = FskStrToNum(p[1]);
				FskMemPtrDispose(tmp);
			}
#if 0
			// The source IP address is not used by session or client
			if (copyAttributeValue(hdr, "source", &tmp)) {
				UInt32 address;
				if (0 == FskNetHostnameResolve(tmp, &address)) {
					mediaStream->sourceIPAddr = address;
				}
				FskMemPtrDispose(tmp);
			}
#endif
		}
		if (0 != (hdr = FskHeaderFind("x-dynamic-rate", request->responseHeaders))) {
			char *tmp;

			// Check the round-trip time in milliseconds
			if (copyAttributeValue(hdr, "rtt", &tmp)) {
				session->lateTolerance = 10;
				FskMemPtrDispose(tmp);
			}
		}

		// Instantiate the UDP sockets for RTP and RTCP packets
		if (!session->rtpOverTCP) {
			UInt32 localIP = 0;
			err = FskNetSocketGetLocalAddress(session->skt, &localIP, NULL);
			BAIL_IF_ERR(err);

			err = FskNetSocketNewUDP(&mediaStream->sktRTP, "RTP/UDP socket");
			if (0 != err) goto bail;
			err = FskNetSocketNewUDP(&mediaStream->sktRTCP, "RTCP/UDP socket");
			if (0 != err) goto bail;

			err = FskNetSocketReuseAddress(mediaStream->sktRTP);
			if (0 == err)
				err = FskNetSocketBind(mediaStream->sktRTP, localIP, mediaStream->clientPortRTP);
			if (0 == err)
				err = FskNetSocketReuseAddress(mediaStream->sktRTCP);
			if (0 == err)
				err = FskNetSocketBind(mediaStream->sktRTCP, localIP, mediaStream->clientPortRTCP);
			if (0 != err) {
				err = kFskErrRTSPSocketConfigFailure;
				goto bail;
			}
			
			mediaStream->rtpSocketInfo.mediaStream = mediaStream;
			mediaStream->rtpSocketInfo.session = session;
			FskNetSocketReceiveBufferSetSize(mediaStream->sktRTP, kReadBufferSize);
			FskThreadAddDataHandler(&mediaStream->rtpSocketInfo.dataHandler, (FskThreadDataSource)mediaStream->sktRTP, sReadPacketRTP, true, false, &mediaStream->rtpSocketInfo);
			
			mediaStream->rtcpSocketInfo.mediaStream = mediaStream;
			mediaStream->rtcpSocketInfo.session = session;
			FskNetSocketReceiveBufferSetSize(mediaStream->sktRTCP, kReadBufferSize);
			FskThreadAddDataHandler(&mediaStream->rtcpSocketInfo.dataHandler, (FskThreadDataSource)mediaStream->sktRTCP, sReadPacketRTCP, true, false, &mediaStream->rtcpSocketInfo);

			mediaStream->bufferRTP.length = kReadBufferSize;
			err = FskMemPtrNew(mediaStream->bufferRTP.length, &mediaStream->bufferRTP.buffer);
			if (0 != err) goto bail;
		}

		mediaStream->ready = true;

		// If all streams are ready, start playing
		for (i = 0; i < session->nMediaStreams; ++ i) {
			if (!session->mediaStream[i].ready) {
				doSetup(session, &session->mediaStream[i]);
				goto bail;
			}
		}

		// Send the magic RTP & RTCP packet required for UDP over AT&T
		if (!session->rtpOverTCP) {
			UInt32 serverIPAddr;
			UInt8 magic[128];
			FskMemSet(magic, 0, sizeof(magic));
			magic[0] = 0x80;
			serverIPAddr = mediaStream->sourceIPAddr;
			if (0 == serverIPAddr)
				FskNetSocketGetRemoteAddress(session->skt, &serverIPAddr, NULL);
			if (0 != serverIPAddr) {
				for (i = 0; i < session->nMediaStreams; ++i) {
					int amt;
					RTSPMediaStream mediaStream = &session->mediaStream[i];
					FskNetSocketSendUDP(mediaStream->sktRTP, &magic[0], sizeof(magic), &amt, serverIPAddr, mediaStream->serverPortRTP);
					FskNetSocketSendUDP(mediaStream->sktRTCP, &magic[0], sizeof(magic), &amt, serverIPAddr, mediaStream->serverPortRTCP);
				}
			}
		}

		sDoStateChange(session, kRTSPClientStateReady);
	}
	else if (0 == FskStrCompare(request->method, "PLAY")) {
		char *hdr;
		UInt16 i, j, nParts;

		// Grab the playback time range for the presentation
		if (0 != (hdr = FskHeaderFind("Range", request->responseHeaders))) {
			char *hdrCopy, *p[4], *value = 0;
			
			hdrCopy = FskStrDoCopy(hdr);
			copyAttributeValue(hdrCopy, "npt", &value);
			if (0 != value) {
				splitToken(value, &nParts, '-', &p[0]);
				if (nParts > 0) {
					char *beginVal = FskStrStripHeadSpace(p[0]);
					if (FskStrCompareCaseInsensitive(beginVal, "now") == 0) {
						session->startTime = 0;
						FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"session->startTime=0 (\"now\")");
					}
					else if (FskStrChr(beginVal, ':') != NULL) {
						// HH:MM:SS notation, currently unsupported
						err = kFskErrRTSPBadSDPParam;
						goto bail;
					}
					else {
						session->startTime = FskStrToD(beginVal, 0);
					}
					
					// Declare a session "live" if the range header looks like 
					// "npt=now-" or "npt=0-"
					if (0 == session->startTime && nParts < 2)
						session->isLive = true;
				}
				FskMemPtrDispose(value);
			}
			FskMemPtrDispose(hdrCopy);
		}
		
		// Grab the pre-buffer time.  If available subtract the pre-buffer time from the start time
		// @@ interaction between this and startTime=0 for live streams??
		if (0 != (hdr = FskHeaderFind("x-prebuffer", request->responseHeaders))) {
			char *hdrCopy, *value = 0;
			
			hdrCopy = FskStrDoCopy(hdr);
			copyAttributeValue(hdrCopy, "time", &value);
			if (0 != value) {
				session->preBufferTime = FskStrToD(FskStrStripHeadSpace(value), 0);
				session->startTime -= session->preBufferTime;
				if (session->startTime < 0)
					session->startTime = 0;
				FskMemPtrDispose(value);
			}
			FskMemPtrDispose(hdrCopy);
		}
		
		// Set some default values
		// ----
		// Live streams sometimes don't return a starting sequence number.
		// In this case, assume the sequence starts at zero (even if it doesn't)
		// this way, no packets are rejected for being stale.
		// ----
		// Live streams sometimes don't return a starting 'rtpTime'
		// In this case, allow the first incoming packet to set the
		// starting 'rtpTime'
		for (i = 0; i < session->nMediaStreams; i++) {
			session->mediaStream[i].seq = 0;
			session->mediaStream[i].rtpTime = -1;
		}
		
		// Grab the starting RTP packet sequence number and RTP time for the streams
		if (0 != (hdr = FskHeaderFind("RTP-Info", request->responseHeaders))) {
			char *tmp, *hdrCopy, *p[4];
			
			hdrCopy = FskStrDoCopy(hdr);
			splitToken(hdrCopy, &nParts, ',', &p[0]);
			for (i = 0; i < nParts; ++i) {
				if (copyAttributeValue(p[i], "url", &tmp)) {
					for (j = 0; j < session->nMediaStreams; ++j) {
						SDPAttribute controlAttribute;
						controlAttribute = SDPFindMediaAttributeWithException(session->mediaStream[j].mediaDescription, "control", "*");
						if (!controlAttribute) {
							err = kFskErrRTSPBadSDPParam;
							goto bail;
						}
						
						// Only match the tail end of the control attribute, since the url attribute could contain
						// the control attribute concatenated to the request URL.
						if (0 == (FskStrTail(tmp, controlAttribute->value))) {
							char *tmp2;
							
							if (copyAttributeValue(p[i], "seq", &tmp2)) {
								session->mediaStream[j].seq = FskStrToNum(tmp2);
								FskMemPtrDispose(tmp2);
							}
							LOG2("Play response seq # for pType", session->mediaStream[j].seq, session->mediaStream[j].payloadType);
							
							if (copyAttributeValue(p[i], "rtptime", &tmp2)) {
								session->mediaStream[j].rtpTime = (UInt32)FskStrToFskInt64(tmp2);
								FskMemPtrDispose(tmp2);
							}
							LOG2("Play response rtpTime for pType", session->mediaStream[j].rtpTime, session->mediaStream[j].payloadType);
							break;
						}
					}
					FskMemPtrDispose(tmp);
				}
			}
			FskMemPtrDispose(hdrCopy);
		}
		sDoStateChange(session, kRTSPClientStatePlaying);
	}
	else if (0 == FskStrCompare(request->method, "PAUSE")) {
		sDoStateChange(session, kRTSPClientStateReady);
		session->waitingForPauseResponse = false;
	}
	else if (0 == FskStrCompare(request->method, "TEARDOWN")) {
		sDoStateChange(session, kRTSPClientStateStart);
		session->tornDown = true;
	}
	else if (0 == FskStrCompare(request->method, "OPTIONS")) {
		// ignore it
	}

	// Give the client a look at the response headers
	if (0 != session->clientHeaderCB) {
		err = session->clientHeaderCB(request->method, request->responseHeaders, session->clientRefCon);
		BAIL_IF_ERR(err);
	}

bail:
	return err;
}

static RTSPRequest sFindRequestInList(RTSPRequestList list, UInt32 cSeq)
{
	RTSPRequest request = 0;

	RTSPRequest walker = list->head;
	while (0 != walker) {
		if (walker->cSeq == cSeq) {
			request = walker;
			break;
		}
		walker = walker->next;
	}

	return request;
}

static FskErr sProcessResponseHeaders(RTSPSession session)
{
	char	*hdr;
	int		responseCode;
	RTSPRequest request = 0;
	FskErr	err = 0, savedErr = 0;

	responseCode = FskHeaderResponseCode(session->responseHeaders);

	// Try to handle redirects
	switch(responseCode) {
		case 301:
		case 302:
		case 303:
		case 305:
			savedErr = kFskErrRTSPSessionRedirect;
			break;
			
		default:
			break;
	}

	if (responseCode == 100) {
		session->readState = kRTSPClientReadResponseHeaders;
		session->readBufferPos = 0;
		goto bail;
	}
	
	if (responseCode == 0) {
		RTSPRequest request;
		UInt32 cSeq;

		// not a response!
		// some methods are legal from S->C:
		// ANNOUNCE, GET_PARAMETER, OPTIONS, REDIRECT, SET_PARAMETER
		if ((0 == FskStrCompareCaseInsensitive(session->responseHeaders->protocol, "ANNOUNCE")) ||
			(0 == FskStrCompareCaseInsensitive(session->responseHeaders->protocol, "GET_PARAMETER")) ||
			(0 == FskStrCompareCaseInsensitive(session->responseHeaders->protocol, "OPTIONS")) ||
			(0 == FskStrCompareCaseInsensitive(session->responseHeaders->protocol, "REDIRECT")) ||
			(0 == FskStrCompareCaseInsensitive(session->responseHeaders->protocol, "SET_PARAMETER"))) {

			// Prepare a '501 Not Implemented' response
			// On OPTIONS requests, and maybe others, the server doesn't send a cSeq number.
			// If no cSeq number is available, then just send the reply without a cSeq
			hdr = FskHeaderFind("CSeq", session->responseHeaders);
			if (hdr)
				cSeq = FskStrToNum(hdr);
			else
				cSeq = kInvalidCSeq;

			err = RTSPRequestNew(&request);
			BAIL_IF_ERR(err);
			
			err = sPrepareRTSPClientResponse(session, request, session->responseHeaders, cSeq);
			BAIL_IF_ERR(err);
			
			FskListAppend(&session->pendingResponses->head, request);
			sSessionCycle(session);

			// If this C->S request has content, we need to read that before replying
			session->respContentLength = 0;
			hdr = FskHeaderFind(kFskStrContentLength, session->responseHeaders);
			if (hdr) {
				session->respContentLength = FskStrToNum(hdr);
				if (0 != session->respContentLength) {
					session->readState = kRTSPClientReadResponseBody;
				}
			}

			FskHeaderStructDispose(session->responseHeaders);
			session->responseHeaders = 0;
		}
		else {
			// XXX ignore for now
			session->readState = kRTSPClientIdle;
			session->readBufferPos = 0;
		}
		goto bail;
	}

	// Upon receiving a reponse code indicating failure, convert the code to an error and exit
	if (kFskErrRTSPSessionRedirect != savedErr && responseCode >= 300) {
		savedErr = responseCode;
	}
	
	// if the response has a content-length, then read the data
	session->respContentLength = 0;
	hdr = FskHeaderFind(kFskStrContentLength, session->responseHeaders);
	if (hdr) {
		session->respContentLength = FskStrToNum(hdr);
	}

	hdr = FskHeaderFind(kFskStrConnection, session->responseHeaders);
	if (hdr && (0 == FskStrCompareCaseInsensitiveWithLength(hdr, kFskStrClose, FskStrLen(kFskStrClose)))) {
		//session->readUntilServerCloses = true;
	}

	// Find the corresponding request
	hdr = FskHeaderFind("CSeq", session->responseHeaders);
	if (hdr) {
		UInt32 cSeq = FskStrToNum(hdr);
		request = sFindRequestInList(session->sentRequests, cSeq);
	}

	// Populate the remainder of the request
	if (0 != request) {
		request->responseHeaders = session->responseHeaders;

		// Some servers (Sprint hosted!) return error 451 (Parameter Not Understood) when
		// sent an 'OPTIONS * RTSP/1.0' request.  Detect that failure here and swallow the error.
		if (savedErr && (0 == FskStrCompare("OPTIONS", request->method)))
			savedErr = 0;
		else
		if ((kFskErrRTSPSessionRedirect == savedErr) && (0 == FskStrCompare("DESCRIBE", request->method))) {
			hdr = FskHeaderFind(kFskStrLocation, session->responseHeaders);
			if (NULL != hdr) {
				session->redirect = FskStrDoCopy(hdr);
				session->respContentLength = 0;
			}
		}

		if (savedErr) {
			// don't process any further
			FskListRemove(&session->sentRequests->head, request);
			RTSPRequestDispose(request);
			session->respContentLength = 0;
			session->responseHeaders = NULL;
			// next state doesn't matter... it's time to abort
			goto bail;
		}
		
		if (0 != session->respContentLength) {
			request->responseBody = session->readBuffer;
			request->responseBodySize = session->respContentLength;
			session->readState = kRTSPClientReadResponseBody;
		}
		else {
			FskListRemove(&session->sentRequests->head, request);

			err = sRequestComplete(session, request);
			RTSPRequestDispose(request);

			if (session->readBufferAmt)
				session->readState = kRTSPClientParseData;
			else
				session->readState = kRTSPClientIdle;

			// Clean up
			session->respContentLength = 0;
			session->responseHeaders = NULL;
		}
	}

bail:
	return ((savedErr) ? savedErr : err);
}

void sReadPacketRTP(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon)
{
	FskErr err;
	UDPSocketCallbackInfo info = (UDPSocketCallbackInfo)refCon;
	RTSPMediaStream mediaStream = info->mediaStream;
	FskSocket socket = (FskSocket)source;
	int address, port, length;
#if RTSP_DEBUG_DROP_PACKETS
	RTSPSession session = info->session;
	Boolean toDrop;
#endif

	err = FskNetSocketRecvUDP(socket, mediaStream->bufferRTP.buffer, mediaStream->bufferRTP.length, &length, &address, &port);
	if (0 != err) goto bail;
	
	sResetKillTimer(info->session);

		//if (session->sessionState < kRTSPClientStateReady)
		//	break;
	if (!mediaStream->ready || (length == 0)) goto bail;

#if RTSP_DEBUG_DROP_PACKETS
	toDrop = false;

	if (session->droppedPacketIndices) {
		UInt16 i, j;
		for (i = 0; i < session->nMediaStreams; ++i) {
			// search for the current packet number in the list of packets to drop
			for (j=0; j<session->droppedPacketCount; j++) {
				if (session->droppedPacketsCurrent[i] == session->droppedPacketIndices[j]) {
					toDrop = true;
					break;
				}
			}
			
			session->droppedPacketsCurrent[i]++;
			if (session->droppedPacketsCurrent[i] >= session->droppedPacketRange) {
				session->droppedPacketsCurrent[i] = 0;
				sRandomizeDroppedPackets(session);
			}
		}
	}
#endif
	// Store away the originating server IP address
	if (0 == mediaStream->serverIPRTP) {
		mediaStream->serverIPRTP = address;
	}

	// Validate the header, RFC-1889, A.1
	if (isRTPPacketValid((UInt8*)mediaStream->bufferRTP.buffer, length, mediaStream)
	#if RTSP_DEBUG_DROP_PACKETS
		&& !toDrop
	#endif
	) {
		UInt8 *packet;
		err = FskMemPtrNew(length, &packet);
		if (kFskErrNone == err) {
			FskMemMove(packet, mediaStream->bufferRTP.buffer, length);
			RTPPacketParserProcessBuffer(mediaStream->packetParser, packet, length);
		}
	}

bail:
	;
}

void sReadPacketRTCP(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon)
{
	FskErr err;
	UDPSocketCallbackInfo info = (UDPSocketCallbackInfo)refCon;
	RTSPMediaStream mediaStream = info->mediaStream;
	FskSocket socket = (FskSocket)source;
	int length, address, port;
	UInt8 *packet;

	while (kFskErrNone == (err = FskNetSocketRecvUDP(socket, mediaStream->bufferRTP.buffer, mediaStream->bufferRTP.length, &length, &address, &port))) {
		if (length == 0)
			continue;

		if (info->session->sessionState < kRTSPClientStateReady)
			continue;

		// The RTCP packet parser does a good job at validating the packet
		err = FskMemPtrNew(length, &packet);
		BAIL_IF_ERR(err);
		FskMemMove(packet, mediaStream->bufferRTP.buffer, length);
		RTCPPacketParserProcessBuffer(mediaStream->packetParserRTCP, packet, length);
		LOG0("got RTCP packet");

#if RTSP_RECEIVER_REPORTS
		if (mediaStream->sendReceiverReport && FskNetSocketIsWritable(mediaStream->sktRTCP)) {
			unsigned char *rr;
			UInt32 rrSize;
			FskErr localErr;

			// Not a showstopper if this fails
			localErr = sCreateReceiverReport(mediaStream, &rr, &rrSize);
			
			if (kFskErrNone == localErr) {
				FskInstrumentedItemSendMessage(mediaStream->rtspSession, kRTSPInstrMsgTrace, (void *)"sending receiver report");
				FskNetSocketSendUDP(mediaStream->sktRTCP, (char *)rr, rrSize, (int *)&length, address, mediaStream->serverPortRTCP);
				mediaStream->sendReceiverReport = false;
				
				FskMemPtrDispose(rr);
			}
		}
#endif
	}
bail:
	;
}

static FskErr sSessionCycleRead(RTSPSession session)
{
	FskErr err = 0;
	int ret;
	SInt32 amt;
	RTSPRequest request;

	sUpSessionUse(session);

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"in sSessionCycleRead");

	if (session->sessionState == kRTSPClientStateError)
		goto bail;

more:
	// Run the state machine
	switch (session->readState) {
		case kRTSPClientIdle:
			session->readState = kRTSPClientReadData;
			session->readStateNext = kRTSPClientParseData;
			// Fall through

		case kRTSPClientReadData:
			FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"kRTSPClientReadData");
			if (session->needsReadable)
				goto bail;
			amt = kReadBufferSize - session->readBufferAmt;
			err = FskNetSocketRecvTCP(session->skt, &session->readBuffer[session->readBufferAmt], amt, (int *)&ret);
			if (kFskErrNone == err) {
				sResetKillTimer(session);
				session->readBufferAmt += ret;
				session->readState = session->readStateNext;
				FskInstrumentedItemSendMessage(session, kRTSPInstrMsgCycleReadTCP, (void *)ret);
			}
			else if (kFskErrNoData == err) {
				session->needsReadable = true;
				err = kFskErrNone;
				FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"no data");
				goto bail;
			}
			else {
				FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"other error");
				goto bail;
			}
			goto more;

		case kRTSPClientParseData:
			FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"kRTSPClientParseData");
			if ('$' == session->readBuffer[session->readBufferPos]) {
				session->readState = kRTSPClientReadPacketHeader;
				++session->readBufferPos;
				session->packetChannelID = -1;
				session->packetLength = 0;
			}
			else {
				session->readState = kRTSPClientReadResponseHeaders;
			}
			goto more;


		case kRTSPClientReadPacketHeader:
			// We need the one byte channel ID and two byte packet length
			// Hence the total packet size is 3 + the packet length
			FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"kRTSPClientReadPacketHeader");
			if ((session->readBufferAmt - session->readBufferPos) > 3) {
				session->packetChannelID = session->readBuffer[session->readBufferPos];
				session->packetLength = FskMisaligned16_GetBtoN(&session->readBuffer[session->readBufferPos+1]);
				session->readBufferPos += 3;
				FskInstrumentedItemSendMessage(session, kRTSPInstrMsgPacketStartTCP, (void*)((UInt32)session->packetLength));
				
				// Allocate the packet buffer
				err = FskMemPtrNew(session->packetLength, &session->packetBuffer);
				BAIL_IF_ERR(err);
				session->packetPos = 0;

				// Slide what we can from the read buffer into the packet buffer
				amt = session->readBufferAmt - session->readBufferPos;
				if (amt > (SInt32)session->packetLength) {
					amt = session->packetLength;
				}
				FskMemMove(session->packetBuffer, &session->readBuffer[session->readBufferPos], amt);
				session->packetPos += amt;
				session->readBufferPos += amt;

				// Slide the contents of the read buffer to the head
				FskMemMove(session->readBuffer, &session->readBuffer[session->readBufferPos], session->readBufferAmt - session->readBufferPos);
				session->readBufferAmt = session->readBufferAmt - session->readBufferPos;
				session->readBufferPos = 0;

				if (session->packetLength == session->packetPos) {
					session->readState = kRTSPClientProcessPacket;
				}
				else {
					session->readState = kRTSPClientReadPacketData;
					goto bail;
				}
			}
			else {
				session->readState = kRTSPClientReadData;
				session->readStateNext = kRTSPClientReadPacketHeader;
				goto bail;
			}
			goto more;

		case kRTSPClientReadPacketData:
			FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"kRTSPClientReadPacketData");
			if (session->needsReadable || (NULL == session->packetBuffer))
				goto bail;
			amt = session->packetLength - session->packetPos;
			err = FskNetSocketRecvTCP(session->skt, &session->packetBuffer[session->packetPos], amt, (int *)&ret);
			if (kFskErrNone == err) {
				FskInstrumentedItemSendMessage(session, kRTSPInstrMsgCycleReadTCP, (void *)ret);
				sResetKillTimer(session);
				session->packetPos += ret;
				FskInstrumentedItemSendMessage(session, kRTSPInstrMsgPacketProgressTCP, (void *)ret);
				if (session->packetPos == session->packetLength) {
					session->readState = kRTSPClientProcessPacket;
				}
			}
			else if (kFskErrNoData == err) {
				session->needsReadable = true;
				FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"no data");
				goto bail;
			}
			else {
				FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"other error");
				goto bail;
			}
			goto more;

		case kRTSPClientReadResponseHeaders:
			FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"kRTSPClientReadResponseHeaders");
			if (NULL == session->responseHeaders) {
				err = FskHeaderStructNew(&session->responseHeaders);
				BAIL_IF_ERR(err);
			}

			ret = FskHeadersParseChunk(&session->readBuffer[session->readBufferPos], session->readBufferAmt, kFskHeaderTypeResponse, session->responseHeaders);
			session->readBufferAmt -= ret;
			session->readBufferPos += ret;
			if (0 != session->readBufferAmt)
				FskMemMove(session->readBuffer, &session->readBuffer[session->readBufferPos], session->readBufferAmt);
			session->readBufferPos = 0;

			if (session->responseHeaders->headersParsed) {
				session->readState = kRTSPClientProcessResponseHeaders;
			}
			else {
				session->readState = kRTSPClientReadData;
				session->readStateNext = kRTSPClientReadResponseHeaders;
			}
			goto more;

		case kRTSPClientProcessResponseHeaders:
			FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"kRTSPClientProcessResponseHeaders");
			if ((NULL != session->responseHeaders) && session->responseHeaders->headersParsed) {
				err = sProcessResponseHeaders(session);
				if (0 == err && session->readState != kRTSPClientIdle)
					goto more;
			}
			break;

		case kRTSPClientReadResponseBody:
			FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"kRTSPClientReadResponseBody");
			if (session->readBufferAmt < session->respContentLength) {
				session->readStateNext = kRTSPClientReadResponseBody;
				session->readState = kRTSPClientReadData;
				goto more;
			}
			else if (session->readBufferAmt >= session->respContentLength) {
				char *hdr;

				request = 0;
				if (NULL != session->responseHeaders) {
					hdr = FskHeaderFind("CSeq", session->responseHeaders);
					if (hdr) {
						UInt32 cSeq = FskStrToNum(hdr);
						request = sFindRequestInList(session->sentRequests, cSeq);
					}
				}

				// Clean up
				session->responseHeaders = NULL;
				session->readBufferAmt -= session->respContentLength;
				session->respContentLength = 0;
				session->readBufferPos = 0;
				session->readState = kRTSPClientIdle;

				// Call the request callback
				if (0 != request) {
					FskListRemove(&session->sentRequests->head, request);
					err = sRequestComplete(session, request);
					RTSPRequestDispose(request);
				}
			}
			FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"STATE:clientReadResponseBody");
			break;

		case kRTSPClientProcessPacket:
			FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"kRTSPClientProcessPacket");
			FskInstrumentedItemSendMessage(session, kRTSPInstrMsgPacketCompleteTCP, NULL);

			// Determine which channel this packet targets
			if (0 != session->packetLength) {
				UInt32 i;
				RTSPMediaStream mediaStream = 0;
				RTPPacketParser parser = 0;
				RTCPPacketParser parserRTCP = 0;
				for (i = 0; i < session->nMediaStreams; ++i) {
					mediaStream = &session->mediaStream[i];
					if (session->packetChannelID == mediaStream->rtpChannel) {
						parser = mediaStream->packetParser;
						break;
					}
					if (session->packetChannelID == mediaStream->rtcpChannel) {
						parserRTCP = mediaStream->packetParserRTCP;
						break;
					}
				}
				if (0 != parser) {
					if (isRTPPacketValid((UInt8*)session->packetBuffer, session->packetLength, mediaStream)) {
						err = RTPPacketParserProcessBuffer(parser, (UInt8*)session->packetBuffer, session->packetLength);
					}
				}
				if (0 != parserRTCP) {
					err = RTCPPacketParserProcessBuffer(parserRTCP, (UInt8*)session->packetBuffer, session->packetLength);
#if RTSP_RECEIVER_REPORTS
					// bugzid: 24526
					// It appears that receivers don't like receiver reports delivered over TCP, so
					// just clear the flag and move on.
					mediaStream->sendReceiverReport = false;
#if 0
					if (0 && mediaStream->sendReceiverReport && FskNetSocketIsWritable(session->skt)) {
						FskMemPtr rr;
						UInt32 rrSize;
						FskErr localErr;

						localErr = sCreateReceiverReport(mediaStream, &rr, &rrSize);

						if (kFskErrNone == localErr) {
							FskNetSocketSendTCP(session->skt, (char *)rr, rrSize, &ret);
							mediaStream->sendReceiverReport = false;
							FskMemPtrDispose(rr);
						}
					}
#endif
#endif
				}
				session->packetBuffer = 0;
				session->packetLength = 0;
			}

			if (0 == session->readBufferAmt) {
				session->readState = kRTSPClientReadData;
				session->readStateNext = kRTSPClientParseData;
				goto bail;
			}
			else {
				session->readState = kRTSPClientParseData;
			}
			goto more;
	}

bail:
	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"out sSessionCycleRead");

	if (0 != err) {
		session->status.lastErr = err;
		sDoStateChange(session, kRTSPClientStateError);
	}

	sDownSessionUse(session);

	return err;
}

FskErr sSessionCycleWrite(RTSPSession session)
{
	RTSPRequest request;
	Boolean isClientResponse = false;
	FskErr err = 0;
	SInt32 amt;
	int ret;

	sUpSessionUse(session);

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"in sSessionCycleWrite");

	if (session->sessionState == kRTSPClientStateError || NULL == session->skt)
		goto bail;

	request = session->pendingRequests->head;
	if (0 == request) {
		request = (RTSPRequest)session->pendingResponses->head;
		isClientResponse = true;
	}
	if (NULL == request)
		goto bail;

	if (session->needsWritable) {
		err = kFskErrNone;		// bail out until it's writable
		goto bail;
	}

	amt = request->bufferAmt - request->bufferPos;

	err = FskNetSocketSendTCP(session->skt, request->buffer + request->bufferPos, amt, &ret);

	if (kFskErrNone == err) {
		sResetKillTimer(session);
		request->bufferPos += ret;
		if (request->bufferAmt - request->bufferPos == 0) {
			if (!isClientResponse) {
				// Request sent - place on sent queue
				FskListRemove(&session->pendingRequests->head, request);
				FskListAppend(&session->sentRequests->head, request);
			}
			else {
				FskListRemove(&session->pendingResponses->head, request);
				RTSPRequestDispose(request);
			}
		}
		else {
			err = kFskErrNeedMoreTime;
		}
	}
	else if (kFskErrNoData == err) {
		err = kFskErrSocketFull;
	}
	else {
		session->status.lastErr = err;
		sDoStateChange(session, kRTSPClientStateError);
	}

bail:
	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"out sSessionCycleWrite");

	sDownSessionUse(session);

	return err;
}

void sSessionRunCycle(FskTimeCallBack cb, FskTime when, void *param)
{
	sSessionCycle(param);
}

static void sCanReadData(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon)
{
	RTSPSession session = (RTSPSession)refCon;
	session->needsReadable = false;
	sSessionCycleRead(session);
}

static void sCanSendData(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon)
{
	RTSPSession session = (RTSPSession)refCon;
	FskThreadRemoveDataHandler(&session->writeDataHandler);
	session->needsWritable = false;
	sSessionCycle(session);
}

void sSessionCycle(void *param)
{
	RTSPSession session = (RTSPSession)param;
	FskErr retReq;
	Boolean scheduleNextRun = false;

	sUpSessionUse(session);

	// Send pending requests
	retReq = sSessionCycleWrite(session);
	if (retReq == kFskErrSocketFull) {		// Unable to write anything
		FskThreadAddDataHandler(&session->writeDataHandler, (FskThreadDataSource)session->skt, sCanSendData, false, true, session);
	}
	else if (retReq == kFskErrNeedMoreTime)	// Unable to write everything
		scheduleNextRun = true;

	if (NULL != session->pendingRequests && NULL != session->pendingRequests->head)
		scheduleNextRun = true;
	else if (NULL != session->pendingResponses && NULL != session->pendingResponses->head)
		scheduleNextRun = true;
		
	if (scheduleNextRun)
		FskTimeCallbackScheduleNextRun(session->cycleCallback, sSessionRunCycle, session);
	
	sDownSessionUse(session);
}

static FskErr sGotClientSocket(void *_skt, void *refCon)
{
	FskErr err = 0;
	RTSPSession session = (RTSPSession)refCon;
	FskSocket skt = (FskSocket)_skt;

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"in sGotClientSocket");

	session->skt = skt;
	if (!skt) {
		FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"socket is null!");
		return kFskErrNoMoreSockets;
	}

	session->waitingForSocket = false;

	if (session->disposed) {
		FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"in sGotClientSocket disposing session");
		RTSPSessionDispose(session);
		return kFskErrNone;
	}

	sUpSessionUse(session);

	session->networkState = kRTSPClientConnected;
	session->readState = kRTSPClientIdle;

	// Register to be called back whenever the TCP socket has data to read
	FskThreadAddDataHandler(&session->readDataHandler, (FskThreadDataSource)session->skt, sCanReadData, true, false, session);
	
	// Tell the session client that we've connected to the host
	sDoStateChange(session, kRTSPClientStateConnected);

	sDownSessionUse(session);

	return err;
}

FskErr RTSPSessionConnectToHost(RTSPSession session)
{
	FskErr	err;
	UInt32 port;
	char *host, proxyHost[32];

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgTrace, (void *)"in connect to host");

	// Connect to the host (asynchronous)
	session->networkState = kRTSPClientConnecting;
	port = getSessionPort(session);
	
	if (0 != session->proxyAddr) {
		FskNetIPandPortToString(session->proxyAddr, 0, proxyHost);
		host = proxyHost;
	}
	else {
		host = session->host;
	}
	
	session->waitingForSocket = true;

	err = FskNetConnectToHost(host, port, false, (FskNetSocketCreatedCallback)sGotClientSocket, session, 0, NULL, "RTSP Client");

	if (kFskErrWaitingForSocket == err)
		err = kFskErrNone;

	FskInstrumentedItemSendMessage(session, kRTSPInstrMsgConnectToHost, (void *)err);
	
	return err;
}

FskErr RTSPSessionSetFeature(RTSPSession session, UInt32 feature, void *param)
{
	switch(feature) {
		case kRTSPFeatureDisableReliableUDP:
			session->disableReliableUDP = true;
			break;
	}

	return 0;
}

#if RTSP_DEBUG_DROP_PACKETS
// Selectively drop RTP packets as specified
// Function will cause (dropped) packets to be dropped out of every (range)
// (droppedIndices) indicates which specific indices to drop
// if (droppedIndices) is NULL, function will randomly drop (dropped) out of every (range) packets
FskErr RTSPSessionSetDroppedPackets(RTSPSession session, UInt16 range, UInt16 dropped, UInt16 *droppedIndices)
{
	FskErr err = 1;
	
	if (!session) goto bail;
	
	session->droppedPacketRange = range;
	session->droppedPacketCount = dropped;
	
	FskMemPtrDisposeAt(&session->droppedPacketIndices);
	FskMemPtrDisposeAt(&session->droppedPacketsCurrent);
	err = FskMemPtrNew(dropped * sizeof(UInt16), (FskMemPtr *)&session->droppedPacketIndices);
	BAIL_IF_ERR(err);
	if (droppedIndices) {
		session->droppedPacketRandom = false;
		FskMemCopy(session->droppedPacketIndices, droppedIndices, dropped * sizeof(UInt16));
	}
	else {
		// random
		session->droppedPacketRandom = true;
		err = sRandomizeDroppedPackets(session);
	}

bail:
	return err;
}

static FskErr sRandomizeDroppedPackets(RTSPSession session)
{
	UInt32 i, j, val;
	if (session && session->droppedPacketRandom) {
		for (i=0; i<session->droppedPacketCount; i++) {
			while (true) {
				Boolean foundDupe = false;
				val =  FskRandom() % session->droppedPacketRange;
				// search for dupes
				for (j=0; j<i; j++) {
					if (val == session->droppedPacketIndices[j]) {
						foundDupe = true;
						break;
					}
				}
				if (!foundDupe)
					break;
			}
			session->droppedPacketIndices[i] = (UInt16)val;
		}
	}
	return 0;
}
#else
FskErr RTSPSessionSetDroppedPackets(RTSPSession session, UInt16 range, UInt16 dropped, UInt16 *droppedIndices)
{
	return 0;
}
#endif


#if RTSP_RECEIVER_REPORTS
static FskErr sCreateReceiverReport(RTSPMediaStream mediaStream, unsigned char **buf, UInt32 *bufSize)
{
	FskErr err = 0;
	UInt8 *p, fractionLost = 0;
	UInt16 *lenOut, len;
	SInt32 packetsLost = 0, extended_max, expected, expected_interval,
		received_interval, lost_interval;
	UInt32 *p32, jitter = 0, LSR = 0, DLSR = 0, packetsLostClamp;
	RTSPStreamStats s = &mediaStream->stats;

	if (!(buf && bufSize) || !mediaStream->sequenceInitialized) {
		return kFskErrRTSPReceiverReportErr;
	}
	
	// @@ only support 1 report block for now
	
	*bufSize = 8 + 24;		// 8 bytes for header, 24 bytes for each report block

	err = FskMemPtrNew(*bufSize, buf);
	BAIL_IF_ERR(err);
	p = *buf;
	p[0] = 0x04 << 5;
	p[1] = 201;
	lenOut = (UInt16 *)&p[2];
	p32 = (UInt32 *)&p[4];
	*p32 = FskEndianU32_NtoB(mediaStream->clientSSRC);
	p += 8;
	len = 1;
	
	extended_max = s->cycles + s->max_seq;
	expected = extended_max - s->base_seq + 1;
	packetsLost = expected - s->received;
#if RECEIVER_REPORTS_DISPLAY_STATS
	DrawDebugMessageEx(0, "packetsLost=%ld", packetsLost);
#endif
	packetsLostClamp = (packetsLost < 0) ? 0x800000 : 0x7FFFFF;
	
	expected_interval = expected - s->expected_prior;
	s->expected_prior = expected;
	received_interval = s->received - s->received_prior;
	s->received_prior = s->received;
	lost_interval = expected_interval - received_interval;
	if (expected_interval == 0 || lost_interval <= 0)
		fractionLost = 0;
	else
		fractionLost = (UInt8)((lost_interval << 8) / expected_interval);
#if RECEIVER_REPORTS_DISPLAY_STATS
	DrawDebugMessageEx(1, "fractionLost=%hu", fractionLost);
#endif
	
#if JITTER_ALT_CALC
	jitter = s->jitter >> 4;
#else
	jitter = (UInt32)s->jitter;
#endif
#if RECEIVER_REPORTS_DISPLAY_STATS
	DrawDebugMessageEx(2, "jitter=%lu", jitter);
#endif
	
	if (mediaStream->lastSenderReport)
		LSR = (UInt32)((mediaStream->lastSenderReport->ntpTimestamp >> 16) & 0xFFFFFFFF);
#if RECEIVER_REPORTS_DISPLAY_STATS
	DrawDebugMessageEx(3, "LSR=%lu", LSR);
#endif
	
	if (mediaStream->lastSenderReport) {
		FskTimeRecord currTime;
		FskTimeGetNow(&currTime);
		FskTimeSub(&mediaStream->lastSenderReportTime, &currTime);
		DLSR = (FskTimeInMS(&currTime) << 16) / kFskTimeMsecPerSec;
	}
#if RECEIVER_REPORTS_DISPLAY_STATS
	DrawDebugMessageEx(4, "DLSR=%lu", DLSR);
	LOG2("RR:packetsLost/fractionLost", packetsLost, fractionLost);
	LOG2("RR:jitter/LSR", jitter, LSR);
	LOG2("RR:DLSR/SSRC", DLSR, mediaStream->ssrc);
#endif
	
	// report block
	{
		UInt32 *xp = (UInt32*)p;
		*xp++ = FskEndianU32_NtoB(mediaStream->ssrc);
		*xp++ = FskEndianU32_NtoB((fractionLost << 24) | (packetsLost & packetsLostClamp));
		*xp++ = FskEndianU32_NtoB(extended_max);
		*xp++ = FskEndianU32_NtoB(jitter);
		*xp++ = FskEndianU32_NtoB(LSR);
		*xp++ = FskEndianU32_NtoB(DLSR);
		len += 6;
	}
	
	// write out RC
	**buf |= 1;

	// write out length (total 32-bit words - 1)
	*lenOut = FskEndianU16_NtoB(len);

bail:
	return err;
}
#endif

Boolean isRTPPacketValid(UInt8 *packet, UInt32 packetSize, RTSPMediaStream mediaStream)
{
	UInt8 *p;
	UInt32 V, PT;
	Boolean packetValid = false;

	if (packetSize < 12) goto bail;
	
	// Validate the header, RFC-1889, A.1
	p = (UInt8*)packet;
	V = (p[0] & 0x80) >> 6;
	PT = (p[1] & 0x7F);
//	SSRC = ((UInt32)p[8]<<24)|((UInt32)p[9]<<16)|((UInt32)p[10]<<8)|p[11];
	// check the SSRC in the session's RTP packet handler
	if (2 == V && PT == mediaStream->payloadType) {
		packetValid = true;
	}
	
bail:
	return packetValid;
}

#if SUPPORT_RELIABLE_UDP

#define kReliableUDPAckMS 250

void initReliableUDP(RTSPSession session, RTSPMediaStream mediaStream)
{
	UInt8 *ack;
	UInt32 *p32;

	mediaStream->reliableUDP.baseSeq = -1;
	FskTimeCallbackDispose(mediaStream->reliableUDP.emitCB);

	FskTimeGetNow(&mediaStream->reliableUDP.emitTime);
	FskTimeAddSecs(&mediaStream->reliableUDP.emitTime, kReliableUDPAckMS);
	mediaStream->reliableUDP.emitCB = FskTimeCallbackAddNew(&mediaStream->reliableUDP.emitTime, reliableUDPEmitCB, mediaStream);
	ack = mediaStream->reliableUDP.ackPacket;
	ack[0] = 0x80;		// version, padding, subtype
	ack[1] = 0xCC;		// APP packet type
	p32 = (UInt32 *)&ack[4];
	*p32 = FskEndianU32_NtoB(mediaStream->clientSSRC);	// SSRC
	ack[8] = 'a';		// name
	ack[9] = 'c';
	ack[10] = 'k';
	ack[11] = ' ';
}

void logPacketForReliableUDP(RTSPSession session, RTSPMediaStream mediaStream, RTPPacket packet)
{
#define kPacketsPerAck 16
	UInt16 sequenceNumber;
	Boolean emit = false;
	Boolean reset = false;
	RTSPReliableUDPData reliableUDP = &mediaStream->reliableUDP;

	sequenceNumber = (UInt16)packet->sequenceNumber;

	if (kFskUInt32Max == reliableUDP->baseSeq) {
		reset = true;
	}
	else
	if (sequenceNumber > reliableUDP->baseSeq && sequenceNumber < reliableUDP->baseSeq + kPacketsPerAck) {
		if (reliableUDP->nPackets == kPacketsPerAck) {
			emit = true;
			reset = true;
		}
		else {
			reliableUDP->mask |= (1L << (32 - (packet->sequenceNumber - reliableUDP->baseSeq)));
			++reliableUDP->nPackets;
		}
	}
	else {
		emit = true;
		reset = true;
	}

	if (emit) {
		reliableUDPEmitCB(NULL, NULL, mediaStream);
	}

	if (reset) {
		reliableUDP->baseSeq = sequenceNumber;
		reliableUDP->mask = 0;
		reliableUDP->nPackets = 1;	// we count the base 'ackSeq' packet as the first packet

		FskTimeGetNow(&reliableUDP->emitTime);
		FskTimeAddMS(&reliableUDP->emitTime, kReliableUDPAckMS);
		FskTimeCallbackSet(reliableUDP->emitCB, &reliableUDP->emitTime, reliableUDPEmitCB, mediaStream);
	}
}

void reliableUDPEmitCB(FskTimeCallBack callback, const FskTime time, void *param)
{
	RTSPMediaStream mediaStream = (RTSPMediaStream)param;
	RTSPSession session = mediaStream->rtspSession;
	RTSPReliableUDPData reliableUDP = &mediaStream->reliableUDP;
	UInt32 *p32;
	UInt32 packetLength, packetWords;

	if (kFskUInt32Max == reliableUDP->baseSeq) return;

	p32 = (UInt32 *)&reliableUDP->ackPacket[16];
	*p32 = FskEndianU32_NtoB(reliableUDP->baseSeq);
	p32 = (UInt32 *)&reliableUDP->ackPacket[20];
	*p32 = FskEndianU32_NtoB(reliableUDP->mask);

	packetWords = 1 == reliableUDP->nPackets ? 4 : 5;
	packetLength = 1 == reliableUDP->nPackets ? 20 : 24;
	reliableUDP->ackPacket[3] = (UInt8)packetWords;

#if SUPPORT_INSTRUMENTATION
		if (mediaStream->streamType == kRTSPMediaStreamAudio) {
			FskInstrumentedTypePrintfDebug(&gRTSPTypeInstrumentation, "ack audio seq %ld with %ld sequential packets", reliableUDP->baseSeq, reliableUDP->nPackets);
		}
		else {
			FskInstrumentedTypePrintfDebug(&gRTSPTypeInstrumentation, "ack video seq %ld with %ld sequential packets", reliableUDP->baseSeq, reliableUDP->nPackets);
		}
#endif

	RTSPSessionAckReliableUDP(session, mediaStream, &reliableUDP->ackPacket[0], packetLength);

	reliableUDP->baseSeq = -1;	// reset

	FskTimeGetNow(&reliableUDP->emitTime);
	FskTimeAddMS(&reliableUDP->emitTime, kReliableUDPAckMS);
	FskTimeCallbackSet(reliableUDP->emitCB, &reliableUDP->emitTime, reliableUDPEmitCB, mediaStream);
}

#endif

void sUpSessionUse(RTSPSession session) {
	session->useCount++;
}
void sDownSessionUse(RTSPSession session) {
	if (NULL != session) {
		session->useCount--;
		if (session->useCount < 1) {
			RTSPSessionDispose(session);
		}
	}
}

static FskErr sDoStateChange(RTSPSession session, SInt16 newState)
{
	FskErr err = 0;

	// Guard against session being disposed from under us
	sUpSessionUse(session);

	if (NULL != session->stateChangeCB) {
		err = (*session->stateChangeCB)(newState, session->clientRefCon);
		if (0 != err) goto bail;
	}

	// If the client clears the error, then carry on and clear the error state
	if (kRTSPClientStateError == newState && session->status.lastErr == 0)
		;
	else
		session->sessionState = newState;

bail:
	sDownSessionUse(session);

	return err;
}

static UInt32 getSessionPort(RTSPSession session)
{
	UInt32 port;
	
	if (0 != session->proxyPort)
		port = session->proxyPort;
	else if (0 != session->defaultPort)
		port = session->defaultPort;
	else
		port = session->port;
		
	return port;
}

static int clientInterfaceChangedCB(struct FskNetInterfaceRecord *iface, UInt32 status, void *param)
{
	RTSPSession session = (RTSPSession)param;

	if (session->skt && (session->skt->ipaddrLocal != 0) && (iface->ip & iface->netmask) != (session->skt->ipaddrLocal & iface->netmask))
		return 0;

	switch (status) {
		case kFskNetInterfaceStatusRemoved:
			session->interfaceLost = true;
			if (session->skt)
				session->skt->pendingClose = true;
			break;
	}
	return 0;
}

#if SUPPORT_INSTRUMENTATION

Boolean doFormatMessageRTSP(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
		case kRTSPInstrMsgTrace:
			snprintf(buffer, bufferSize, "%s", (char*)msgData);
			return true;

		case kRTSPInstrMsgCycleReadTCP:
			snprintf(buffer, bufferSize, "TCP read: %ld bytes", (UInt32)msgData);
			return true;

		case kRTSPInstrMsgPacketStartTCP:
			snprintf(buffer, bufferSize, "RTP/TCP packet size: %ld bytes", (UInt32)msgData);
			return true;

		case kRTSPInstrMsgPacketProgressTCP:
			snprintf(buffer, bufferSize, "RTP/TCP packet read: %ld bytes", (UInt32)msgData);
			return true;

		case kRTSPInstrMsgPacketCompleteTCP:
			snprintf(buffer, bufferSize, "RTP/TCP packet complete");
			return true;

		case kRTSPInstrMsgConnectToHost:
			snprintf(buffer, bufferSize, "ConnectToHost returned: %ld", (UInt32)msgData);
			return true;
	}

	return false;
}

#endif
