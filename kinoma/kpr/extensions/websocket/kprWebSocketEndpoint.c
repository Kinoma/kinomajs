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
#include "FskNetUtils.h"
#include "FskSSL.h"
#include "FskEndian.h"

#include "kpr.h"
#include "kprShell.h"
#include "kprUtilities.h"
#include "kprWebSocketEndpoint.h"


//--------------------------------------------------
// INSTRUMENTATION
//--------------------------------------------------

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord gKprWebSocketEndpointInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprWebSocketEndpoint", FskInstrumentationOffset(KprWebSocketEndpointRecord), NULL, 0, NULL, NULL, NULL, 0 };
#endif

//--------------------------------------------------
// WebSocket Interface
//--------------------------------------------------

#define kSocketBufferSize (128 * 1024)

#define CALLBACK(x) if (self->x) self->x


enum {
	kKprWebSocketEndpoint_onHeader = 1,
	kKprWebSocketEndpoint_onFrameOpcode,
	kKprWebSocketEndpoint_onFrameLength,
	kKprWebSocketEndpoint_onFrameLength16,
	kKprWebSocketEndpoint_onFrameLength64,
	kKprWebSocketEndpoint_onFrameMaskData,
	kKprWebSocketEndpoint_onFrameMessage,
};


static Boolean KprWebSocketEndpointCheckURL(KprWebSocketEndpoint self);

static FskErr KprWebSocketEndpointConnectCallback(FskSocket skt, void *refCon);
static void KprWebSocketEndpointDisconnect(KprWebSocketEndpoint self);

static FskErr KprWebSocketEndpointSetupSocketReader(KprWebSocketEndpoint self, FskSocket skt, int initialState);
static void KprWebSocket_onReadError(KprSocketErrorContext context, FskErr err, void *refcon);
static void KprWebSocket_onWriteError(KprSocketErrorContext context, FskErr err, void *refcon);

static void KprWebSocketEndpointStartConnect(KprWebSocketEndpoint self);

static FskErr KprWebSocketEndpointCancelConnection(KprWebSocketEndpoint self);
static FskErr KprWebSocketEndpointStartClosingHandshake(KprWebSocketEndpoint self, UInt16 code, char *reason);

static FskErr KprWebSocketEndpointUpgradeConnection(KprWebSocketEndpoint self);

static FskErr KprWebSocketEndpointReadHeader(KprSocketReader reader, KprWebSocketEndpoint self);
static FskErr KprWebSocketEndpointReadFrameOpcode(KprSocketReader reader, KprWebSocketEndpoint self);
static FskErr KprWebSocketEndpointReadFrameLength(KprSocketReader reader, KprWebSocketEndpoint self);
static FskErr KprWebSocketEndpointReadFrameLength16(KprSocketReader reader, KprWebSocketEndpoint self);
static FskErr KprWebSocketEndpointReadFrameLength64(KprSocketReader reader, KprWebSocketEndpoint self);
static FskErr KprWebSocketEndpointReadFrameMaskData(KprSocketReader reader, KprWebSocketEndpoint self);
static FskErr KprWebSocketEndpointReadFrameMessage(KprSocketReader reader, KprWebSocketEndpoint self);

static FskErr KprWebSocketEndpointHandleTextMessage(KprWebSocketEndpoint self, void *message, UInt32 length);
static FskErr KprWebSocketEndpointHandleBinaryMessage(KprWebSocketEndpoint self, void *message, UInt32 length);
static FskErr KprWebSocketEndpointHandleCloseFrame(KprWebSocketEndpoint self, void *message, UInt32 length);
static Boolean KprWebSocketEndpointValidateResponse(KprWebSocketEndpoint self, FskHeaders *response);

static FskErr KprWebSocketEndpointSendRawFrame(KprWebSocketEndpoint self, UInt8 opcode, void *payload, UInt32 length);

//--------------------------------------------------
// WebSocket Implementation
//--------------------------------------------------

FskErr KprWebSocketEndpointNew(KprWebSocketEndpoint* it, void *refcon)
{
	FskErr err = kFskErrNone;
	KprWebSocketEndpoint self = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprWebSocketEndpointRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &gKprWebSocketEndpointInstrumentation);
	
	self->refcon = refcon;
	self->state = kKprWebSocketStateConnecting;

	FskDebugStr("CREATE: KprWebSocketEndpoint\n");
	return err;
bail:
	if (self) KprWebSocketEndpointDispose(self);
	return err;
}

void KprWebSocketEndpointDispose(KprWebSocketEndpoint self)
{
	if (self) {
		FskDebugStr("DISPOSE: KprWebSocketEndpoint\n");
		if (self->socket) KprWebSocketEndpointDisconnect(self);

		if (self->url) FskMemPtrDispose(self->url);
		if (self->key) FskMemPtrDispose(self->key);
		if (self->origin) FskMemPtrDispose(self->origin);
		if (self->closeReason) FskMemPtrDispose(self->closeReason);
		if (self->parts) FskStrParsedUrlDispose(self->parts);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

static Boolean KprWebSocketEndpointCheckURL(KprWebSocketEndpoint self)
{
	if (FskStrCompare(self->parts->scheme, "ws") == 0) return true;
	if (FskStrCompare(self->parts->scheme, "wss") == 0) return true;
	return false;
}

FskErr KprWebSocketEndpointConnect(KprWebSocketEndpoint self, char *url, char *origin)
{
	FskErr err = kFskErrNone;
	
	self->url = FskStrDoCopy(url);
	bailIfNULL(self->url);
	bailIfError(FskStrParseUrl(self->url, &self->parts));
	if (!KprWebSocketEndpointCheckURL(self)) {
		bailIfError(kFskErrInvalidParameter);
	}
	
	self->isSecure = (FskStrCompare(self->parts->scheme, "wss") == 0);
	
	if (origin) {
		self->origin = FskStrDoCopy(origin);
		bailIfNULL(self->origin);
	}
	
	self->state = kKprWebSocketStateConnecting;
	
	INVOKE_AFTER1(KprWebSocketEndpointStartConnect, self);
	
bail:
	return err;
}

static void KprWebSocketEndpointStartConnect(KprWebSocketEndpoint self)
{
	FskErr err = kFskErrNone;
	int port = 80;
	long flags = 0;
	
	if (self->parts->port) port = self->parts->port;
	
	if (self->isSecure) {
		if (port == 80) port = 443;
		flags |= kConnectFlagsSSLConnection;
	}
	
	bailIfError(FskNetConnectToHost(self->parts->host, port, false, KprWebSocketEndpointConnectCallback, self, flags, NULL, "WebSocket"));
	
bail:
	if (err) {
		FskDebugStr("ERROR: %d\n", err);
		CALLBACK(errorCallback)(self, err, "cannot start connection", self->refcon);
	}
}

static FskErr KprWebSocketEndpointConnectCallback(FskSocket skt, void *refCon)
{
	FskErr err = kFskErrNone;
	KprWebSocketEndpoint self = refCon;
	
	FskDebugStr("CONNECT: callback was called\n");
	if (!skt || 0 == skt->ipaddrRemote) {
		bailIfError(kFskErrSocketNotConnected);
	}
	
	bailIfError(KprWebSocketEndpointSetupSocketReader(self, skt, kKprWebSocketEndpoint_onHeader));
	self->doMask = true;
	self->needNonMaskedFrame = true;

	if (self->cancelConnection) {
		KprWebSocketEndpointDisconnect(self);
	} else {
		bailIfError(KprWebSocketEndpointUpgradeConnection(self));
	}
	
bail:
	if (err) {
		CALLBACK(errorCallback)(self, err, "cannot start handshake", self->refcon);
	}
	return err;
}

FskErr KprWebSocketEndpointOpenWithSocket(KprWebSocketEndpoint self, FskSocket skt)
{
	FskErr err = kFskErrNone;

	if (!skt || 0 == skt->ipaddrRemote) {
		bailIfError(kFskErrSocketNotConnected);
	}

	bailIfError(KprWebSocketEndpointSetupSocketReader(self, skt, kKprWebSocketEndpoint_onFrameOpcode));
	self->doMask = false;
	self->needMaskedFrame = true;

	self->state = kKprWebSocketStateOpen;

	CALLBACK(openCallback)(self, self->refcon);

bail:
	if (err) {
		CALLBACK(errorCallback)(self, err, "cannot start handshake", self->refcon);
	}
	return err;
}

static FskErr KprWebSocketEndpointSetupSocketReader(KprWebSocketEndpoint self, FskSocket skt, int initialState)
{
	FskErr err = kFskErrNone;
	KprSocketReaderState states[] = {
		{
			kKprWebSocketEndpoint_onHeader,
			(KprSocketReaderCallback) KprWebSocketEndpointReadHeader
		},
		{
			kKprWebSocketEndpoint_onFrameOpcode,
			(KprSocketReaderCallback) KprWebSocketEndpointReadFrameOpcode
		},
		{
			kKprWebSocketEndpoint_onFrameLength,
			(KprSocketReaderCallback) KprWebSocketEndpointReadFrameLength
		},
		{
			kKprWebSocketEndpoint_onFrameLength16,
			(KprSocketReaderCallback) KprWebSocketEndpointReadFrameLength16
		},
		{
			kKprWebSocketEndpoint_onFrameLength64,
			(KprSocketReaderCallback) KprWebSocketEndpointReadFrameLength64
		},
		{
			kKprWebSocketEndpoint_onFrameMaskData,
			(KprSocketReaderCallback) KprWebSocketEndpointReadFrameMaskData
		},
		{
			kKprWebSocketEndpoint_onFrameMessage,
			(KprSocketReaderCallback) KprWebSocketEndpointReadFrameMessage
		},
	};
	UInt32 stateCount = sizeof(states) / sizeof(KprSocketReaderState);
	KprSocketReader reader;
	KprSocketWriter writer;

	bailIfError(KprSocketReaderNew(&reader, skt, states, stateCount, self));
	bailIfError(KprSocketWriterNew(&writer, skt, self));

	FskNetSocketReceiveBufferSetSize(skt, kSocketBufferSize);

	reader->errorCallback = KprWebSocket_onReadError;
	KprSocketReaderSetState(reader, initialState);

	writer->errorCallback = KprWebSocket_onWriteError;

	self->socket = skt;
	self->reader = reader;
	self->writer = writer;

bail:
	if (err) {
		KprSocketReaderDispose(reader);
		KprSocketWriterDispose(writer);
	}
	return err;
}

static void KprWebSocketHandleError(KprWebSocketEndpoint self, FskErr err, UInt16 code, char *reason)
{
	switch (err) {
		case kFskErrConnectionClosed:
			code = 1001;
			reason = "endpoint closed";
			CALLBACK(errorCallback)(self, err, reason, self->refcon);
			KprWebSocketEndpointDisconnect(self);
			return;

		case kFskErrBadData:
			code = 1002;
			reason = "protocol error";
			break;

		default:
			break;
	}

	// report error.

	if (self->state <= kKprWebSocketStateOpen) {
		CALLBACK(errorCallback)(self, err, reason, self->refcon);

		KprWebSocketEndpointClose(self, code, reason);
	}
}

static void KprWebSocket_onReadError(KprSocketErrorContext context UNUSED, FskErr err, void *refcon)
{
	KprWebSocketEndpoint self = refcon;

	KprWebSocketHandleError(self, err, 1008, "read error");
}

static void KprWebSocket_onWriteError(KprSocketErrorContext context UNUSED, FskErr err, void *refcon)
{
	KprWebSocketEndpoint self = refcon;

	KprWebSocketHandleError(self, err, 1008, "write error");
}


static void KprWebSocketEndpointDisconnect(KprWebSocketEndpoint self)
{
	if (self->socket) {
		KprSocketReaderDispose(self->reader);
		self->reader = NULL;

		KprSocketWriterDispose(self->writer);
		self->writer = NULL;

		FskNetSocketClose(self->socket);
		self->socket = NULL;
		
		self->state = kKprWebSocketStateClosed;
	}
	
	if (self->read.headers) {
		FskHeaderStructDispose(self->read.headers);
		self->read.headers = NULL;
	}
	
	if (self->read.message) {
		FskMemPtrDispose(self->read.message);
		self->read.message = NULL;
	}

	CALLBACK(closeCallback)(self, self->closeCode, self->closeReason, self->cleanClose, self->refcon);
}

FskErr KprWebSocketEndpointClose(KprWebSocketEndpoint self, UInt16 code, char *reason)
{
	FskErr err = kFskErrNone;
	FskDebugStr("CLOSE: (%d), %s\n", code, reason);
	self->closeCode = code;
	if (reason) {
		FskMemPtrDispose(self->closeReason);
		self->closeReason = FskStrDoCopy(reason);
	}
	
	switch (self->state) {
		case kKprWebSocketStateConnecting:
			bailIfError(KprWebSocketEndpointCancelConnection(self));
			break;
			
		case kKprWebSocketStateOpen:
			bailIfError(KprWebSocketEndpointStartClosingHandshake(self, code, reason));
			break;
			
		case kKprWebSocketStateClosing:
		case kKprWebSocketStateClosed:
		default:
			break;
	}
	
bail:
	return err;
}

static FskErr KprWebSocketEndpointCancelConnection(KprWebSocketEndpoint self)
{
	self->state = kKprWebSocketStateClosing;
	CALLBACK(closingCallback)(self, self->closeCode, self->closeReason, self->cleanClose, self->refcon);
	self->cancelConnection = true;
	return kFskErrNone;
}

static FskErr KprWebSocketEndpointStartClosingHandshake(KprWebSocketEndpoint self, UInt16 code, char *reason)
{
	FskErr err = kFskErrNone;
	char *payload = NULL;
	UInt32 length = sizeof(UInt16);
	
	self->state = kKprWebSocketStateClosing;
	CALLBACK(closingCallback)(self, self->closeCode, self->closeReason, self->cleanClose, self->refcon);

	length += FskStrLen(reason);
	bailIfError(FskMemPtrNew(length, &payload));
	
	*((UInt16 *) payload) = FskEndianU16_NtoB(code);
	FskMemCopy(&payload[2], reason, length - sizeof(UInt16));
	
	bailIfError(KprWebSocketEndpointSendRawFrame(self, kKprWebSocketOpcodeClose, payload, length));
	self->closeWasSent = true;
	
bail:
	if (payload) FskMemPtrDispose(payload);
	
	if (err || (self->closeWasSent && self->closeWasReceived)) {
		self->cleanClose = (err == kFskErrNone);
		KprWebSocketEndpointDisconnect(self);
	}
	
	return err;
}

static FskErr KprWebSocketEndpointUpgradeConnection(KprWebSocketEndpoint self)
{
	FskErr err = kFskErrNone;
	FskHeaders *request;
	char buffer[1024], tmp[1024], portStr[10];
	int len, port;
	
	bailIfError(FskHeaderStructNew(&request));
	
	port = (self->parts->port ? port = self->parts->port : 80);
	
	if (port == 80) {
		FskHeaderAddString("Host", self->parts->host, request);
	} else {
		FskStrCopy(tmp, self->parts->host);
		FskStrCat(tmp, ":");
		FskStrNumToStr(port, portStr, 10);
		FskStrCat(tmp, portStr);
		FskHeaderAddString("Host", tmp, request);
	}
	
	if (self->origin) {
		FskHeaderAddString("Origin", self->origin, request);
	} else {
		FskStrCopy(tmp, "http://");
		FskStrCat(tmp, self->parts->host);
		FskHeaderAddString("Origin", tmp, request);
	}
	
	FskHeaderAddString("Upgrade", "websocket", request);
	FskHeaderAddString("Connection", "Upgrade", request);
	
	KprWebSocketCreateKey(&self->key);
	FskHeaderAddString("Sec-WebSocket-Key", self->key, request);
	FskHeaderAddInteger("Sec-WebSocket-Version", 13, request);
	
	FskStrCopy(buffer, "GET ");
	if (self->parts->path[0] != '/') FskStrCat(buffer, "/");
	FskStrCat(buffer, self->parts->path);
	FskStrCat(buffer, " HTTP/1.1\r\n");
	
	len = FskStrLen(buffer);
	FskHeaderGenerateOutputBlob(&buffer[len], 1024 - len, true, request);
	
	KprSocketWriterSendBytes(self->writer, buffer, FskStrLen(buffer));
	
bail:
	
	return err;
}

static FskErr KprWebSocketEndpointHandleResponse(KprWebSocketEndpoint self, FskHeaders *response)
{
	FskErr err = kFskErrNone;
	
	if (self->cancelConnection) {
		KprWebSocketEndpointDisconnect(self);
		return err;
	}
	
	if (!KprWebSocketEndpointValidateResponse(self, response)) {
		KprWebSocketEndpointDisconnect(self);
		return -1;
	}
	
	FskDebugStr("HANDSHAKE: done. upgraded to websocket\n");
	self->state = kKprWebSocketStateOpen;

	CALLBACK(openCallback)(self, self->refcon);

	return kFskErrNone;
}

static FskErr KprWebSocketEndpointHandleFrame(KprWebSocketEndpoint self, UInt8 opcode, UInt8 *message, UInt32 length)
{
	FskErr err = kFskErrNone;
	
	if (self->closeWasReceived) return kFskErrConnectionClosed;

	switch (opcode) {
		case kKprWebSocketOpcodeTextFrame:
			if (!self->ignoreFurtherFrame) {
				FskDebugStr("FRAME: Text! (%ld bytes)\n", length);
				err = KprWebSocketEndpointHandleTextMessage(self, message, length);
			}
			break;

		case kKprWebSocketOpcodeBinaryFrame:
			if (!self->ignoreFurtherFrame) {
				FskDebugStr("FRAME: Binary! (%ld bytes)\n", length);
				err = KprWebSocketEndpointHandleBinaryMessage(self, message, length);
			}
			break;

		case kKprWebSocketOpcodeClose:
			FskDebugStr("FRAME: Close!\n");
			err = KprWebSocketEndpointHandleCloseFrame(self, message, length);
			break;
			
		case kKprWebSocketOpcodePing:
			if (!self->ignoreFurtherFrame) {
				FskDebugStr("FRAME: Ping!\n");
				bailIfError(KprWebSocketEndpointSendRawFrame(self, kKprWebSocketOpcodePong, message, length));
			}
			break;
			
		case kKprWebSocketOpcodePong:
			FskDebugStr("FRAME: Pong!\n");
			break;
			
		default:
			FskDebugStr("FRAME: Unknown opcode %d! (%ld bytes)\n", (int) opcode, length);
			break;
	}
	
bail:
	return err;
}

static FskErr KprWebSocketEndpointHandleTextMessage(KprWebSocketEndpoint self, void *message, UInt32 length)
{
	FskErr err = kFskErrNone;
	char *text = NULL;

	bailIfError(FskMemPtrNew(length + 1, &text));
	FskMemCopy(text, message, length);
	text[length] = 0;

	CALLBACK(textCallback)(self, text, self->refcon);

bail:
	if (text) FskMemPtrDispose(text);
	return err;
}

static FskErr KprWebSocketEndpointHandleBinaryMessage(KprWebSocketEndpoint self, void *message, UInt32 length)
{
	FskErr err = kFskErrNone;

	CALLBACK(binaryCallback)(self, message, length, self->refcon);

	return err;
}

static FskErr KprWebSocketEndpointHandleCloseFrame(KprWebSocketEndpoint self, void *message, UInt32 length)
{
	FskErr err = kFskErrNone;
	UInt16 code;

	if (length >= sizeof(UInt16)) {
		code = *(UInt16 *)message;
		code = FskEndianU16_BtoN(code);
	} else {
		code = 1005;
	}

	self->closeWasReceived = true;
	if (self->closeWasSent) {
		KprWebSocketEndpointDisconnect(self);
	} else {
		KprWebSocketEndpointClose(self, code, "Another endpoint closed");
	}

	return err;
}

static Boolean KprWebSocketEndpointCheckHeaderValue(FskHeaders *response, char *name, char *bingo)
{
	char *value;
	
	value = FskHeaderFind(name, response);
	if (!value) return false;
	
	return (FskStrCompareCaseInsensitive(value, bingo) == 0);
}

static Boolean KprWebSocketEndpointValidateResponse(KprWebSocketEndpoint self, FskHeaders *response)
{
	FskErr err;
	char *value;
	char *encoded = NULL;
	Boolean result = false;

	{
		FskHeaderIterator iter = FskHeaderIteratorNew(response);
		FskDebugStr("HANDSHAKE: response headers\n");
		while (iter) {
			FskDebugStr(">  %s: %s\n", iter->name, iter->value);
			iter = FskHeaderIteratorNext(iter);
		}
		FskHeaderIteratorDispose(iter);
	}
	
	if (response->responseCode != 101) return false;
	if (!KprWebSocketEndpointCheckHeaderValue(response, "Upgrade", "websocket")) return false;
	if (!KprWebSocketEndpointCheckHeaderValue(response, "Connection", "Upgrade")) return false;
	
	value = FskHeaderFind("Sec-WebSocket-Accept", response);
	if (!value) return false;

	bailIfError(KprWebSocketCalculateHash(self->key, &encoded));

	result = (FskStrCompare(value, encoded) == 0);
bail:
	if (encoded) FskMemPtrDispose(encoded);
	return result;
}

static FskErr KprWebSocketEndpointCreateFrame(Boolean fin, UInt8 opcode, Boolean mask, void *payload, UInt32 length, void **frame, UInt32 *frameLength)
{
	FskErr err = kFskErrNone;
	UInt8 headers[14], maskData[4];
	UInt32 headerLen = 1 + 1;
	
	headers[0] = opcode;
	if (fin) {
		headers[0] |= 0x80u;
	}
	
	if (length < 126) {
		headers[1] = (UInt8) length;
	} else if (length < 0x10000l) {
		headers[1] = 126;
		
		*((UInt16 *)&headers[2]) = FskEndianU16_NtoB(length);
		headerLen += sizeof(UInt16);
	} else {
		headers[1] = 127;
		
		*((FskInt64 *)&headers[2]) = FskEndianU64_NtoB(length);
		headerLen += sizeof(FskInt64);
	}
	
	if (mask) {
		headers[1] |= 0x80u;
		
		*((SInt32 *)maskData) = FskRandom();
		FskMemCopy(&headers[headerLen], maskData, sizeof(maskData));
		headerLen += sizeof(UInt32);
	}
	
	bailIfError(FskMemPtrNew(headerLen + length, frame));
	FskMemCopy(*frame, headers, headerLen);
	if (length > 0) {
		FskMemCopy((UInt8 *) *frame + headerLen, payload, length);
	}
	*frameLength = headerLen + length;
	
	if (mask) {
		KprWebSocketMask((UInt8 *) *frame + headerLen, length, maskData);
	}
	
bail:
	return err;
}

UInt32 KprWebSocketEndpointGetPendingData(KprWebSocketEndpoint self)
{
	if (self->reader == NULL) return 0;
	return self->writer->pendingLength;
}

FskErr KprWebSocketEndpointSendString(KprWebSocketEndpoint self, char *message)
{
	return KprWebSocketEndpointSendRawFrame(self, kKprWebSocketOpcodeTextFrame, message, FskStrLen(message));
}

FskErr KprWebSocketEndpointSendBinary(KprWebSocketEndpoint self, void *data, UInt32 length)
{
	return KprWebSocketEndpointSendRawFrame(self, kKprWebSocketOpcodeBinaryFrame, data, length);
}

FskErr KprWebSocketEndpointSendPing(KprWebSocketEndpoint self)
{
	if (self->state != kKprWebSocketStateOpen) return kFskErrBadState;

	FskTimeGetNow(&self->pingSent);
	self->pongReceived = false;
	return KprWebSocketEndpointSendRawFrame(self, kKprWebSocketOpcodePing, NULL, 0);
}

static FskErr KprWebSocketEndpointSendRawFrame(KprWebSocketEndpoint self, UInt8 opcode, void *payload, UInt32 length)
{
	FskErr err = kFskErrNone;
	void *frame = NULL;
	UInt32 frameLength;
	
	if (self->closeWasSent) return kFskErrBadState;
	if (self->writer == NULL) return kFskErrBadState;

	bailIfError(KprWebSocketEndpointCreateFrame(true, opcode, self->doMask, payload, length, &frame, &frameLength));
	KprSocketWriterSendBytes(self->writer, frame, frameLength);
	
bail:
	if (frame) FskMemPtrDispose(frame);
	return err;
}

// Socket Read

static FskErr KprWebSocketEndpointReadHeader(KprSocketReader reader, KprWebSocketEndpoint self)
{
	FskErr err = kFskErrNone;

	if (self->read.headers == NULL) {
		bailIfError(FskHeaderStructNew(&self->read.headers));
	}

	err = KprSocketReaderReadHTTPHeaders(reader, self->read.headers);
	if (err != kFskErrNone || !self->read.headers->headersParsed) return err;

	err = KprWebSocketEndpointHandleResponse(self, self->read.headers);
	
	if (err == kFskErrNone) {
		KprSocketReaderSetState(reader, kKprWebSocketEndpoint_onFrameOpcode);
	}
	
	FskHeaderStructDispose(self->read.headers);
	self->read.headers = NULL;

bail:
	return err;
}

static FskErr KprWebSocketEndpointReadFrameOpcode(KprSocketReader reader, KprWebSocketEndpoint self)
{
	FskErr err = kFskErrNone;
	UInt8 data;
	
	err = KprSocketReaderReadBytes(reader, &data, sizeof(UInt8));
	if (err != kFskErrNone) return err;
	
	self->read.opcode = data & 0x7fu;
	self->read.fin = ((data & 0x80u) != 0);
	
	KprSocketReaderSetState(reader, kKprWebSocketEndpoint_onFrameLength);
	return err;
}

static FskErr KprWebSocketEndpointReadFrameLength(KprSocketReader reader, KprWebSocketEndpoint self)
{
	FskErr err = kFskErrNone;
	UInt8 data, nextPhase;
	
	err = KprSocketReaderReadBytes(reader, &data, sizeof(UInt8));
	if (err != kFskErrNone) return err;
	
	self->read.length = data & 0x7fu;
	self->read.mask = ((data & 0x80u) != 0);

	switch (self->read.length) {
		case 126:
			nextPhase = kKprWebSocketEndpoint_onFrameLength16;
			break;
			
		case 127:
			nextPhase = kKprWebSocketEndpoint_onFrameLength64;
			break;
			
		default:
			nextPhase = kKprWebSocketEndpoint_onFrameMaskData;
			break;
	}
	
	KprSocketReaderSetState(reader, nextPhase);
	return err;
}

static FskErr KprWebSocketEndpointReadFrameLength16(KprSocketReader reader, KprWebSocketEndpoint self)
{
	FskErr err = kFskErrNone;
	UInt16 value;
	
	err = KprSocketReaderReadBytes(reader, self->read.buffer, sizeof(UInt16));
	if (err != kFskErrNone) return err;
	
	value = *((UInt16 *) self->read.buffer);
	self->read.length = FskEndianU16_BtoN(value);
	
	KprSocketReaderSetState(reader, kKprWebSocketEndpoint_onFrameMaskData);
	return err;
}

static FskErr KprWebSocketEndpointReadFrameLength64(KprSocketReader reader, KprWebSocketEndpoint self)
{
	FskErr err = kFskErrNone;
	FskInt64 value;
	
	err = KprSocketReaderReadBytes(reader, self->read.buffer, sizeof(FskInt64));
	if (err != kFskErrNone) return err;
	
	value = *((FskInt64 *) self->read.buffer);
	self->read.length = FskEndianU64_BtoN(value);
	
	KprSocketReaderSetState(reader, kKprWebSocketEndpoint_onFrameMaskData);
	return err;
}

static FskErr KprWebSocketEndpointReadFrameMaskData(KprSocketReader reader, KprWebSocketEndpoint self)
{
	FskErr err = kFskErrNone;
	
	if (self->read.mask) {
		err = KprSocketReaderReadBytes(reader, self->read.maskData, 4);
		if (err != kFskErrNone) return err;
	}
	
	KprSocketReaderSetState(reader, kKprWebSocketEndpoint_onFrameMessage);
	return err;
}

static FskErr KprWebSocketEndpointReadFrameMessage(KprSocketReader reader, KprWebSocketEndpoint self)
{
	FskErr err = kFskErrNone;
	UInt32 length = self->read.length;
	
	if (length > 0) {
		if (self->read.message == NULL) {
			err = FskMemPtrNew(length, &self->read.message);
			if (err != kFskErrNone) return err;
		}
		
		err = KprSocketReaderReadBytes(reader, self->read.message, length);
		if (err != kFskErrNone) return err;
		
		if (self->read.mask) {
			KprWebSocketMask((UInt8 *) self->read.message, length, self->read.maskData);
		}
	}

	KprSocketReaderSetState(reader, kKprWebSocketEndpoint_onFrameOpcode);

	if ((self->needMaskedFrame && !self->read.mask)
		|| (self->needNonMaskedFrame && self->read.mask)) {
		self->ignoreFurtherFrame = true;
		bailIfError(kFskErrBadData);
	}

	err = KprWebSocketEndpointHandleFrame(self, self->read.opcode, self->read.message, length);

bail:
	FskMemPtrDisposeAt(&self->read.message);

	return err;
}

// Deferred Utilities

void KprWebSocketEndpointInvokeAfter(void *func, void *param1, void *param2, void *param3, void *param4)
{
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)func, param1, param2, param3, param4);
}

