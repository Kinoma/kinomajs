/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#include "kprMQTTEndpoint.h"


//--------------------------------------------------
// MQTT Interface
//--------------------------------------------------

#define kSocketBufferSize (128 * 1024)

#define CALLBACK(x) if (self->x) self->x


static FskErr KprMQTTEndpointPackMessage(KprMQTTEndpoint self, KprMQTTMessage message, void **buffer, UInt32 *length);

enum {
	kKprMQTTEndpoint_fixedHeaderState,
	kKprMQTTEndpoint_remainingLengthState,
	kKprMQTTEndpoint_variableHeaderState,

	kKprMQTTEndpoint_connectSignatureState,
	kKprMQTTEndpoint_connectVersionState,
	kKprMQTTEndpoint_connectFlagState,
	kKprMQTTEndpoint_connectKeepAliveState,
	kKprMQTTEndpoint_connectClientIdState,
	kKprMQTTEndpoint_connectWillTopicState,
	kKprMQTTEndpoint_connectWillMessageState,
	kKprMQTTEndpoint_connectUserNameState,
	kKprMQTTEndpoint_connectPasswordState,

	kKprMQTTEndpoint_connackState,

	kKprMQTTEndpoint_publishTopicState,
	kKprMQTTEndpoint_publishMessageIdState,
	kKprMQTTEndpoint_publishDataState,

	kKprMQTTEndpoint_subscribeMessageIdState,
	kKprMQTTEndpoint_subscribeTopicState,
	kKprMQTTEndpoint_subscribeQoSState,

	kKprMQTTEndpoint_subackMessageIdState,
	kKprMQTTEndpoint_subackQoSState,

	kKprMQTTEndpoint_unsubscribeMessageIdState,
	kKprMQTTEndpoint_unsubscribeTopicState,

	kKprMQTTEndpoint_messageIdState,

	kKprMQTTEndpoint_completeState,
};

static FskErr KprMQTTEndpoint_readFixedHeader(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readRemainingLength(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_dispatchByType(KprSocketReader reader, void *refcon);

static FskErr KprMQTTEndpoint_readConnectSignature(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readConnectVersion(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readConnectFlag(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readConnectKeepAlive(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readConnectClientId(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readConnectWillTopic(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readConnectWillMessage(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readConnectUserName(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readConnectPassword(KprSocketReader reader, void *refcon);

static FskErr KprMQTTEndpoint_readConnAck(KprSocketReader reader, void *refcon);

static FskErr KprMQTTEndpoint_readPublishTopic(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readPublishMessageId(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readPublishData(KprSocketReader reader, void *refcon);

static FskErr KprMQTTEndpoint_readSubscribeMessageId(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readSubscribeTopic(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readSubscribeQoS(KprSocketReader reader, void *refcon);

static FskErr KprMQTTEndpoint_readSubackMessageId(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readSubackQoS(KprSocketReader reader, void *refcon);

static FskErr KprMQTTEndpoint_readUnsubscribeMessageId(KprSocketReader reader, void *refcon);
static FskErr KprMQTTEndpoint_readUnsubscribeTopic(KprSocketReader reader, void *refcon);

static FskErr KprMQTTEndpoint_readMessageId(KprSocketReader reader, void *refcon);

static FskErr KprMQTTEndpoint_handleMessage(KprSocketReader reader, void *refcon);

static KprSocketReaderState states[] = {
	{ kKprMQTTEndpoint_fixedHeaderState, KprMQTTEndpoint_readFixedHeader },
	{ kKprMQTTEndpoint_remainingLengthState, KprMQTTEndpoint_readRemainingLength },
	{ kKprMQTTEndpoint_variableHeaderState, KprMQTTEndpoint_dispatchByType },

	{ kKprMQTTEndpoint_connectSignatureState, KprMQTTEndpoint_readConnectSignature },
	{ kKprMQTTEndpoint_connectVersionState, KprMQTTEndpoint_readConnectVersion },
	{ kKprMQTTEndpoint_connectFlagState, KprMQTTEndpoint_readConnectFlag },
	{ kKprMQTTEndpoint_connectKeepAliveState, KprMQTTEndpoint_readConnectKeepAlive },
	{ kKprMQTTEndpoint_connectClientIdState, KprMQTTEndpoint_readConnectClientId },
	{ kKprMQTTEndpoint_connectWillTopicState, KprMQTTEndpoint_readConnectWillTopic },
	{ kKprMQTTEndpoint_connectWillMessageState, KprMQTTEndpoint_readConnectWillMessage },
	{ kKprMQTTEndpoint_connectUserNameState, KprMQTTEndpoint_readConnectUserName },
	{ kKprMQTTEndpoint_connectPasswordState, KprMQTTEndpoint_readConnectPassword },

	{ kKprMQTTEndpoint_connackState, KprMQTTEndpoint_readConnAck },

	{ kKprMQTTEndpoint_publishTopicState, KprMQTTEndpoint_readPublishTopic },
	{ kKprMQTTEndpoint_publishMessageIdState, KprMQTTEndpoint_readPublishMessageId },
	{ kKprMQTTEndpoint_publishDataState, KprMQTTEndpoint_readPublishData },

	{ kKprMQTTEndpoint_subscribeMessageIdState, KprMQTTEndpoint_readSubscribeMessageId },
	{ kKprMQTTEndpoint_subscribeTopicState, KprMQTTEndpoint_readSubscribeTopic },
	{ kKprMQTTEndpoint_subscribeQoSState, KprMQTTEndpoint_readSubscribeQoS },

	{ kKprMQTTEndpoint_subackMessageIdState, KprMQTTEndpoint_readSubackMessageId },
	{ kKprMQTTEndpoint_subackQoSState, KprMQTTEndpoint_readSubackQoS },

	{ kKprMQTTEndpoint_unsubscribeMessageIdState, KprMQTTEndpoint_readUnsubscribeMessageId },
	{ kKprMQTTEndpoint_unsubscribeTopicState, KprMQTTEndpoint_readUnsubscribeTopic },

	{ kKprMQTTEndpoint_messageIdState, KprMQTTEndpoint_readMessageId },
	
	{ kKprMQTTEndpoint_completeState, KprMQTTEndpoint_handleMessage }
};

static FskErr KprMQTTEndpoint_readBytes(KprMQTTEndpoint self, void *buffer, UInt32 length);
static FskErr KprMQTTEndpoint_readString(KprMQTTEndpoint self, char **result);
static FskErr KprMQTTEndpoint_readChunk(KprMQTTEndpoint self, KprMemoryBuffer *result);
static FskErr KprMQTTEndpoint_readUInt8(KprMQTTEndpoint self, UInt8 *data);
static FskErr KprMQTTEndpoint_readUInt16(KprMQTTEndpoint self, UInt16 *data);

static void KprMQTTEndpoint_readError(KprSocketErrorContext context, FskErr err, void *refcon);
static void KprMQTTEndpoint_writeError(KprSocketErrorContext context , FskErr err, void *refcon);

//--------------------------------------------------
// MQTT Implementation
//--------------------------------------------------

FskErr KprMQTTEndpointNew(KprMQTTEndpoint* it, FskSocket skt, void *refcon)
{
	FskErr err = kFskErrNone;
	KprMQTTEndpoint self = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KprMQTTEndpointRecord), &self));

	self->socket = skt;
	FskNetSocketReceiveBufferSetSize(self->socket, kSocketBufferSize);
	self->protocolVersion = kKprMQTTProtocol311;

	bailIfError(KprSocketReaderNew(&self->reader, self->socket, states, sizeof(states) / sizeof(KprSocketReaderState), self));
	self->reader->errorCallback = KprMQTTEndpoint_readError;

	bailIfError(KprSocketWriterNew(&self->writer, self->socket, self));
	self->writer->errorCallback = KprMQTTEndpoint_writeError;

	KprSocketReaderSetState(self->reader, kKprMQTTEndpoint_fixedHeaderState);

	self->refcon = refcon;

	*it = self;
bail:
	if (err) KprMQTTEndpointDispose(self);
	return err;
}

FskErr KprMQTTEndpointDispose(KprMQTTEndpoint self)
{
	if (self) {
		KprSocketWriterDispose(self->writer);
		self->writer = NULL;

		KprSocketReaderDispose(self->reader);
		self->reader = NULL;

		FskNetSocketClose(self->socket);
		self->socket = NULL;

		FskMemPtrDispose(self->reading.string);
		FskMemPtrDispose(self->reading.buffer);

		FskMemPtrDispose(self);
	}
	return kFskErrNone;
}

// --- SEND MESSAGES --------------------------------

FskErr KprMQTTEndpointSendMessage(KprMQTTEndpoint self, KprMQTTMessage message)
{
	FskErr err = kFskErrNone;
	void *buffer = NULL;
	UInt32 size;

	bailIfError(KprMQTTEndpointPackMessage(self, message, &buffer, &size));
	KprSocketWriterSendBytes(self->writer, buffer, size);

bail:
	FskMemPtrDispose(buffer);
	return err;
}

static UInt32 KprMQTTEndpointWriteUInt16(UInt16 value, UInt8 *p)
{
	*(UInt16 *)p = FskEndianU16_NtoB(value);
	return sizeof(UInt16);
}

static UInt32 KprMQTTEndpointPackedStringSize(const char *str)
{
	return str != NULL ? FskStrLen(str) + sizeof(UInt16) : 0;
}

static UInt32 KprMQTTEndpointWriteString(const char *src, UInt8 *p)
{
	UInt16 len;

	if (src == NULL) return 0;

	len = FskStrLen(src);

	p += KprMQTTEndpointWriteUInt16(len, p);

	FskMemCopy(p, src, len);

	return len + sizeof(UInt16);
}

static UInt32 kprMQTTEndpointRemainingLengthBytes(UInt32 length, UInt8 *p)
{
	UInt32 bytes = 0;
	while (length > 0) {
		UInt32 digit = length % 128;
		length /= 128;

		if (p) {
			if (length > 0) digit |= 0x80u;
			*p++ = digit;
		}
		bytes += 1;
	}
	return (bytes > 0 ? bytes : 1);
}

static FskErr KprMQTTEndpointPackMessage(KprMQTTEndpoint self, KprMQTTMessage message, void **buffer, UInt32 *length)
{
	FskErr err;
	UInt32 remainingLength, size;
	KprMQTTSubscribeTopic topic;
	UInt8 *p, flag;
	const char *protocolSignature;

	switch (message->type) {
		case kKprMQTTMessageTypeCONNECT: {
			struct KprMQTTConnectMessageRecord *p = &message->t.connect;
			remainingLength = 4; // version (1 byte) + flag (1 byte) + keep alive (2 bytes)

			if (self->protocolVersion == kKprMQTTProtocol31) {
				protocolSignature = kKprMQTTProtocolSignature31;
			} else {
				protocolSignature = kKprMQTTProtocolSignature311;
			}
			remainingLength += KprMQTTEndpointPackedStringSize(protocolSignature);

			remainingLength += KprMQTTEndpointPackedStringSize(p->clientIdentifier);
			if (p->willTopic) {
				remainingLength += KprMQTTEndpointPackedStringSize(p->willTopic);

				remainingLength += sizeof(UInt16);
				if (p->willPayload) {
					remainingLength += p->willPayload->size;
				}
			}
			remainingLength += KprMQTTEndpointPackedStringSize(p->username);
			remainingLength += KprMQTTEndpointPackedStringSize(p->password);
			break;
		}

		case kKprMQTTMessageTypeCONNACK:
			remainingLength = 2;
			break;

		case kKprMQTTMessageTypePUBLISH:
			remainingLength = message->t.publish.payload ? message->t.publish.payload->size : 0;
			remainingLength += KprMQTTEndpointPackedStringSize(message->t.publish.topic);
			if (message->qualityOfService > 0) remainingLength += sizeof(UInt16);
			break;

		case kKprMQTTMessageTypeSUBSCRIBE:
		case kKprMQTTMessageTypeSUBACK:
		case kKprMQTTMessageTypeUNSUBSCRIBE:
			remainingLength = sizeof(UInt16);
			topic = message->t.other.topics;
			while (topic) {
				if (message->type != kKprMQTTMessageTypeSUBACK) {
					remainingLength += KprMQTTEndpointPackedStringSize(topic->topic);
				}
				if (message->type != kKprMQTTMessageTypeUNSUBSCRIBE) {
					remainingLength += sizeof(UInt8);
				}
				topic = topic->next;
			}
			break;

		case kKprMQTTMessageTypePUBACK:
		case kKprMQTTMessageTypePUBREC:
		case kKprMQTTMessageTypePUBREL:
		case kKprMQTTMessageTypePUBCOMP:
		case kKprMQTTMessageTypeUNSUBACK:
			remainingLength = sizeof(UInt16);
			break;

		default:
			remainingLength = 0;
			break;
	}

	*length = sizeof(UInt8) + remainingLength + kprMQTTEndpointRemainingLengthBytes(remainingLength, NULL);

	err = FskMemPtrNewClear(*length, buffer);
	if (err) return err;

	p = (UInt8 *) *buffer;

	// Fixed header BYTE 1
	flag = (message->type << 4) | (message->qualityOfService << 1);
	if (message->duplicateDelivery) flag |= 0x08;
	if (message->isRetained) flag |= 0x01;

	*p++ = flag;

	// Remaining Length
	p += kprMQTTEndpointRemainingLengthBytes(remainingLength, p);

	// Variable Header and Payload
	switch (message->type) {
		case kKprMQTTMessageTypeCONNECT: {
			struct KprMQTTConnectMessageRecord *cp = &message->t.connect;

			// Protocol Signature
			p += KprMQTTEndpointWriteString(protocolSignature, p);

			// Protocol Version
			*p++ = self->protocolVersion;

			// Connect Flags
			flag = 0;
			if (cp->cleanSession) flag |= 0x02;
			if (cp->willTopic) {
				flag |= 0x04 | (cp->willQualityOfService << 3);
				if (cp->willIsRetained) flag |= 0x020;
			}

			if (cp->username) flag |= 0x40;
			if (cp->password) flag |= 0x80;
			*p++ = flag;

			// Keep Alive
			p += KprMQTTEndpointWriteUInt16(cp->keepAlive, p);

			// Payload
			p += KprMQTTEndpointWriteString(cp->clientIdentifier, p);
			if (cp->willTopic) {
				UInt32 len = (cp->willPayload ? cp->willPayload->size : 0);

				p += KprMQTTEndpointWriteString(cp->willTopic, p);
				p += KprMQTTEndpointWriteUInt16(len, p);
				if (len > 0) {
					FskMemCopy(p, cp->willPayload->buffer, len);
					p += len;
				}
			}
			p += KprMQTTEndpointWriteString(cp->username, p);
			p += KprMQTTEndpointWriteString(cp->password, p);
			break;
		}

		case kKprMQTTMessageTypeCONNACK: {
			UInt8 flags = 0;
			if (message->t.connack.sesstionPresent) flags |= 0x01;

			*p++ = flags;
			*p++ = message->t.connack.returnCode;
			break;
		}

		case kKprMQTTMessageTypePUBLISH:
			p += KprMQTTEndpointWriteString(message->t.publish.topic, p);

			if (message->qualityOfService > 0) {
				p += KprMQTTEndpointWriteUInt16(message->messageId, p);
			}

			if (message->t.publish.payload) {
				p += KprMemoryBufferCopyTo(message->t.publish.payload, p);
			}
			break;

		case kKprMQTTMessageTypeSUBSCRIBE:
		case kKprMQTTMessageTypeSUBACK:
		case kKprMQTTMessageTypeUNSUBSCRIBE:
			p += KprMQTTEndpointWriteUInt16(message->messageId, p);

			topic = message->t.other.topics;
			while (topic) {
				if (message->type != kKprMQTTMessageTypeSUBACK) {
					p += KprMQTTEndpointWriteString(topic->topic, p);
				}
				if (message->type != kKprMQTTMessageTypeUNSUBSCRIBE) {
					*p++ = topic->qualityOfService;
				}
				topic = topic->next;
			}
			break;

		case kKprMQTTMessageTypePUBACK:
		case kKprMQTTMessageTypePUBREC:
		case kKprMQTTMessageTypePUBREL:
		case kKprMQTTMessageTypePUBCOMP:
		case kKprMQTTMessageTypeUNSUBACK:
			p += KprMQTTEndpointWriteUInt16(message->messageId, p);
			break;

		default:
			break;
	}

	size = p - (UInt8 *) *buffer;

	return kFskErrNone;
}

// --- RECEIVE MESSAGES -----------------------------

static FskErr KprMQTTEndpoint_readFixedHeader(KprSocketReader reader, void *refcon)
{
	FskErr err = kFskErrNone;
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	UInt8 header;

	err = KprMQTTEndpoint_readUInt8(self, &header);
	if (err != kFskErrNone) return err;

	self->reading.remainingLength = 0;
	self->reading.lengthMultiplier = 1;
	bailIfError(KprMQTTMessageNew(&self->message));

	self->message->type = header >> 4;
	self->message->qualityOfService = (header >> 1) & 0x03;
	self->message->duplicateDelivery = ((header & 0x04) != 0);
	self->message->isRetained = ((header & 0x01) != 0);

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_remainingLengthState);

bail:
	return err;
}

static FskErr KprMQTTEndpoint_readRemainingLength(KprSocketReader reader, void *refcon)
{
	FskErr err = kFskErrNone;
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	UInt8 length;

	err = KprMQTTEndpoint_readUInt8(self, &length);
	if (err != kFskErrNone) return err;

	self->reading.remainingLength += (length & 0x7f) * self->reading.lengthMultiplier;

	if (length & 0x80) {
		self->reading.lengthMultiplier *= 0x80;
	} else {
		KprSocketReaderSetState(reader, kKprMQTTEndpoint_variableHeaderState);
		self->reading.headerBegan = true;
	}

	return err;
}

static FskErr KprMQTTEndpoint_dispatchByType(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	int nextState;

	switch (self->message->type) {
		case kKprMQTTMessageTypeCONNECT:
			nextState = kKprMQTTEndpoint_connectSignatureState;
			break;

		case kKprMQTTMessageTypeCONNACK:
			nextState = kKprMQTTEndpoint_connackState;
			break;

		case kKprMQTTMessageTypePUBLISH:
			nextState = kKprMQTTEndpoint_publishTopicState;
			break;

		case kKprMQTTMessageTypeSUBSCRIBE:
			nextState = kKprMQTTEndpoint_subscribeMessageIdState;
			break;

		case kKprMQTTMessageTypeSUBACK:
			nextState = kKprMQTTEndpoint_subackMessageIdState;
			break;

		case kKprMQTTMessageTypeUNSUBSCRIBE:
			nextState = kKprMQTTEndpoint_unsubscribeMessageIdState;
			break;

		case kKprMQTTMessageTypePUBACK:
		case kKprMQTTMessageTypePUBREC:
		case kKprMQTTMessageTypePUBREL:
		case kKprMQTTMessageTypePUBCOMP:
		case kKprMQTTMessageTypeUNSUBACK:
			nextState = kKprMQTTEndpoint_messageIdState;
			break;

		default:
			nextState = kKprMQTTEndpoint_completeState;
			break;
	}

	KprSocketReaderSetState(reader, nextState);
	return kFskErrNone;
}

static FskErr KprMQTTEndpoint_readConnectSignature(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;
	char *signature = NULL;

	err = KprMQTTEndpoint_readString(self, &signature);
	if (err != kFskErrNone) return err;

	if (FskStrCompare(signature, kKprMQTTProtocolSignature311) == 0) {
		self->protocolVersion = kKprMQTTProtocol311;
	} else if (FskStrCompare(signature, kKprMQTTProtocolSignature31) == 0) {
		self->protocolVersion = kKprMQTTProtocol31;
	} else {
		err = kFskErrBadData;
	}

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_connectVersionState);
	FskMemPtrDispose(signature);

	return err;
}

static FskErr KprMQTTEndpoint_readConnectVersion(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;
	UInt8 version;

	err = KprMQTTEndpoint_readUInt8(self, &version);
	if (err != kFskErrNone) return err;

	if (version != self->protocolVersion) {
		err = kFskErrBadData;
	}

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_connectFlagState);

	return err;
}

static FskErr KprMQTTEndpoint_readConnectFlag(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;
	UInt8 flag;

	err = KprMQTTEndpoint_readUInt8(self, &flag);
	if (err != kFskErrNone) return err;

	if (flag & 0x01) {
		err = kFskErrBadData;
	}

	self->reading.hasUserName = ((flag & 0x80) != 0);
	self->reading.hasPassword = ((flag & 0x40) != 0);
	self->reading.hasWill = ((flag & 0x04) != 0);
	self->message->t.connect.cleanSession = ((flag & 0x02) != 0);
	self->message->t.connect.willIsRetained = ((flag & 0x20) != 0);
	self->message->t.connect.willQualityOfService = ((flag >> 3) & 0x3);

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_connectKeepAliveState);

	return err;
}

static FskErr KprMQTTEndpoint_readConnectKeepAlive(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;

	err = KprMQTTEndpoint_readUInt16(self, &self->message->t.connect.keepAlive);
	if (err != kFskErrNone) return err;

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_connectClientIdState);

	return err;
}

static FskErr KprMQTTEndpoint_readConnectClientId(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;

	err = KprMQTTEndpoint_readString(self, &self->message->t.connect.clientIdentifier);
	if (err != kFskErrNone) return err;

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_connectWillTopicState);

	return err;
}

static FskErr KprMQTTEndpoint_readConnectWillTopic(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;

	if (self->reading.hasWill) {
		err = KprMQTTEndpoint_readString(self, &self->message->t.connect.willTopic);
		if (err != kFskErrNone) return err;
	}

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_connectWillMessageState);

	return err;
}

static FskErr KprMQTTEndpoint_readConnectWillMessage(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;

	if (self->reading.hasWill) {
		struct KprMQTTConnectMessageRecord *cp = &self->message->t.connect;
		err = KprMQTTEndpoint_readChunk(self, &cp->willPayload);
		if (err != kFskErrNone) return err;
	}

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_connectUserNameState);

	return err;
}

static FskErr KprMQTTEndpoint_readConnectUserName(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;

	if (self->reading.hasUserName) {
		err = KprMQTTEndpoint_readString(self, &self->message->t.connect.username);
		if (err != kFskErrNone) return err;
	}

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_connectPasswordState);

	return err;
}

static FskErr KprMQTTEndpoint_readConnectPassword(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;

	if (self->reading.hasPassword) {
		err = KprMQTTEndpoint_readString(self, &self->message->t.connect.password);
		if (err != kFskErrNone) return err;
	}

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_completeState);

	return err;
}

static FskErr KprMQTTEndpoint_readConnAck(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;
	UInt8 values[2];

	err = KprMQTTEndpoint_readBytes(self, values, sizeof(values));
	if (err != kFskErrNone) return err;

	self->message->t.connack.sesstionPresent = values[0] * 0x01;
	self->message->t.connack.returnCode = values[1];

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_completeState);

	return err;
}

static FskErr KprMQTTEndpoint_readPublishTopic(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;

	err = KprMQTTEndpoint_readString(self, &self->message->t.publish.topic);
	if (err != kFskErrNone) return err;

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_publishMessageIdState);

	return err;
}

static FskErr KprMQTTEndpoint_readPublishMessageId(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;

	if (self->message->qualityOfService > kKprMQTTMessageQoSAtMostOnce) {
		err = KprMQTTEndpoint_readUInt16(self, &self->message->messageId);
		if (err != kFskErrNone) return err;
	}

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_publishDataState);

	return err;
}

static FskErr KprMQTTEndpoint_readPublishData(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;
	UInt32 size;

	if (self->reading.remainingLength < self->reading.readLength) {
		err = kFskErrBadData;
	} else {
		size = self->reading.remainingLength - self->reading.readLength;

		if (self->message->t.publish.payload == NULL) {
			err = KprMemoryBufferNew(size, &self->message->t.publish.payload);
			if (err != kFskErrNone) return err;
		}

		err = KprMQTTEndpoint_readBytes(self, self->message->t.publish.payload->buffer, size);
		if (err != kFskErrNone) return err;
	}

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_completeState);

	return err;
}

static FskErr KprMQTTEndpoint_readSubscribeMessageId(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;

	err = KprMQTTEndpoint_readUInt16(self, &self->message->messageId);
	if (err != kFskErrNone) return err;

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_subscribeTopicState);

	return err;
}

static FskErr KprMQTTEndpoint_readSubscribeTopic(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;
	char *topic = NULL;
	KprMQTTSubscribeTopic subscriptTopic;

	err = KprMQTTEndpoint_readString(self, &topic);
	if (err != kFskErrNone) return err;

	bailIfError(KprMQTTMessageAddSubscribeTopic(self->message, &subscriptTopic));

	subscriptTopic->topic = topic;
	topic = NULL;

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_subscribeQoSState);

bail:
	FskMemPtrDispose(topic);
	return err;
}

static FskErr KprMQTTEndpoint_readSubscribeQoS(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;
	KprMQTTSubscribeTopic subscriptTopic;
	UInt8 qos;

	err = KprMQTTEndpoint_readUInt8(self, &qos);
	if (err != kFskErrNone) return err;

	subscriptTopic = KprMQTTMessageLastSubscribeTopic(self->message);
	subscriptTopic->qualityOfService = qos;

	if (self->reading.readLength >= self->reading.remainingLength) {
		KprSocketReaderSetState(reader, kKprMQTTEndpoint_completeState);
	} else {
		KprSocketReaderSetState(reader, kKprMQTTEndpoint_subscribeTopicState);
	}

	return err;
}

static FskErr KprMQTTEndpoint_readSubackMessageId(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;

	err = KprMQTTEndpoint_readUInt16(self, &self->message->messageId);
	if (err != kFskErrNone) return err;

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_subackQoSState);

	return err;
}

static FskErr KprMQTTEndpoint_readSubackQoS(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;
	KprMQTTSubscribeTopic subscriptTopic;
	UInt8 qos;

	err = KprMQTTEndpoint_readUInt8(self, &qos);
	if (err != kFskErrNone) return err;

	bailIfError(KprMQTTMessageAddSubscribeTopic(self->message, &subscriptTopic));

	subscriptTopic->qualityOfService = qos;

	if (self->reading.readLength >= self->reading.remainingLength) {
		KprSocketReaderSetState(reader, kKprMQTTEndpoint_completeState);
	}

bail:
	return err;
}

static FskErr KprMQTTEndpoint_readUnsubscribeMessageId(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;

	err = KprMQTTEndpoint_readUInt16(self, &self->message->messageId);
	if (err != kFskErrNone) return err;

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_unsubscribeTopicState);

	return err;
}

static FskErr KprMQTTEndpoint_readUnsubscribeTopic(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;
	char *topic = NULL;
	KprMQTTSubscribeTopic subscriptTopic;

	err = KprMQTTEndpoint_readString(self, &topic);
	if (err != kFskErrNone) return err;

	bailIfError(KprMQTTMessageAddSubscribeTopic(self->message, &subscriptTopic));

	subscriptTopic->topic = topic;
	topic = NULL;

	if (self->reading.readLength >= self->reading.remainingLength) {
		KprSocketReaderSetState(reader, kKprMQTTEndpoint_completeState);
	}

bail:
	FskMemPtrDispose(topic);
	return err;
}

static FskErr KprMQTTEndpoint_readMessageId(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;

	err = KprMQTTEndpoint_readUInt16(self, &self->message->messageId);
	if (err != kFskErrNone) return err;

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_completeState);

	return err;
}

static FskErr KprMQTTEndpoint_handleMessage(KprSocketReader reader, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	FskErr err = kFskErrNone;

	if (self->reading.readLength != self->reading.remainingLength) {
		err = kFskErrBadData;
	} else if (self->messageCallback) {
		CALLBACK(messageCallback)(self, self->message, self->refcon);
	} else {
		KprMQTTMessageDispose(self->message);
	}

	self->message = NULL;
	
	self->reading.headerBegan = false;
	self->reading.remainingLength = 0;
	self->reading.readLength = 0;

	self->reading.hasUserName = false;
	self->reading.hasPassword = false;
	self->reading.hasWill = false;

	KprMemoryBufferDispose(self->reading.buffer);
	self->reading.buffer = NULL;

	FskMemPtrDisposeAt(&self->reading.string);

	KprSocketReaderSetState(reader, kKprMQTTEndpoint_fixedHeaderState);
	return err;
}

static void KprMQTTEndpoint_readError(KprSocketErrorContext context UNUSED, FskErr err, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	CALLBACK(errorCallback)(self, err, "read error", self->refcon);
}

static void KprMQTTEndpoint_writeError(KprSocketErrorContext context UNUSED, FskErr err, void *refcon)
{
	KprMQTTEndpoint self = (KprMQTTEndpoint)refcon;
	CALLBACK(errorCallback)(self, err, "write error", self->refcon);
}

static FskErr KprMQTTEndpoint_readBytes(KprMQTTEndpoint self, void *buffer, UInt32 length)
{
	FskErr err;

	err = KprSocketReaderReadBytes(self->reader, buffer, length);
	if (err != kFskErrNone) return err;

	if (self->reading.headerBegan) {
		self->reading.readLength += length;
	}

	return kFskErrNone;
}

static FskErr KprMQTTEndpoint_readString(KprMQTTEndpoint self, char **result)
{
	FskErr err;
	int length;

	if (self->reading.string == NULL) {
		err = KprMQTTEndpoint_readUInt16(self, &self->reading.value16);
		if (err != kFskErrNone) return err;

		err = FskMemPtrNew(self->reading.value16 + 1, &self->reading.string);
		if (err != kFskErrNone) return err;
	}

	length = self->reading.value16;

	err = KprMQTTEndpoint_readBytes(self, self->reading.string, length);
	if (err != kFskErrNone) return err;

	((char *) self->reading.string)[length] = 0;

	*result = self->reading.string;

	self->reading.string = NULL;
	self->reading.value16 = 0;

	return err;
}

static FskErr KprMQTTEndpoint_readChunk(KprMQTTEndpoint self, KprMemoryBuffer *result)
{
	FskErr err;
	int length;

	if (self->reading.buffer == NULL) {
		err = KprMQTTEndpoint_readUInt16(self, &self->reading.value16);
		if (err != kFskErrNone) return err;

		err = KprMemoryBufferNew(self->reading.value16, &self->reading.buffer);
		if (err != kFskErrNone) return err;
	}

	length = self->reading.buffer->size;

	err = KprMQTTEndpoint_readBytes(self, self->reading.buffer->buffer, length);
	if (err != kFskErrNone) return err;

	*result = self->reading.buffer;

	self->reading.buffer = NULL;
	self->reading.value16 = 0;

	return err;
}

static FskErr KprMQTTEndpoint_readUInt8(KprMQTTEndpoint self, UInt8 *data)
{
	FskErr err;
	err = KprMQTTEndpoint_readBytes(self, data, sizeof(UInt8));
	if (err != kFskErrNone) return err;

	return err;
}

static FskErr KprMQTTEndpoint_readUInt16(KprMQTTEndpoint self, UInt16 *data)
{
	FskErr err;
	err = KprMQTTEndpoint_readBytes(self, &self->reading.value16, sizeof(UInt16));
	if (err != kFskErrNone) return err;

	*data = FskEndianU16_BtoN(self->reading.value16);

	return err;
}

