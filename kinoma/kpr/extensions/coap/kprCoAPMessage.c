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
#include "kprCoAPMessage.h"
#include "FskEndian.h"
#include "kprURL.h"

#if defined(RUN_UNITTEST) && RUN_UNITTEST

#include "kunit.h"

ku_test(CoAP_message)
{
}

#endif



static void KprCoAPMessageDisposeOption(KprCoAPMessageOptionRecord *optRec);

FskErr KprCoAPMessageNew(KprCoAPMessage *it)
{
	FskErr err = kFskErrNone;
	KprCoAPMessage self = NULL;

	bailIfError(KprMemPtrNewClear(sizeof(KprCoAPMessageRecord), &self));
	bailIfError(KprRetainableNew(&self->retainable));

	*it = self;

bail:
	if (err) {
		KprCoAPMessageDispose(self);
	}

	return err;
}

FskErr KprCoAPMessageDispose(KprCoAPMessage self)
{
	if (self && KprRetainableRelease(self->retainable)) {
		KprCoAPMessageOptionRecord *optRec;

		KprMemoryChunkDispose(self->token);
		KprMemoryChunkDispose(self->payload);

		optRec = self->options;
		while (optRec != NULL) {
			KprCoAPMessageOptionRecord *next = optRec->next;
			KprCoAPMessageDisposeOption(optRec);
			optRec = next;
		}

		KprMemPtrDispose((void *) self->uri);
		KprMemPtrDispose((void *) self->host);
		KprMemPtrDispose((void *) self->path);
		KprMemPtrDispose((void *) self->query);

		KprRetainableDispose(self->retainable);
		KprMemPtrDispose(self);
	}
	return kFskErrNone;
}

FskErr KprCoAPMessageDisposeAt(KprCoAPMessage *it)
{
	FskErr err = kFskErrNone;

	if (it) {
		err = KprCoAPMessageDispose(*it);
		*it = NULL;
	}

	return err;
}

FskErr KprCoAPMessageCopy(KprCoAPMessage original, KprCoAPMessage *it)
{
	FskErr err = kFskErrNone;
	KprCoAPMessage self = NULL;

	bailIfError(KprCoAPMessageNew(&self));

	self->messageId = original->messageId;

	if (original->token) {
		int size;

		size = original->token->size;
		if (size > 0) {
			bailIfError(KprCoAPMessageSetToken(self, KprMemoryChunkStart(original->token), size));
		}
	}

	*it = self;

bail:
	if (err) {
		KprCoAPMessageDispose(self);
	}

	return err;
}

KprCoAPMessage KprCoAPMessageRetain(KprCoAPMessage self)
{
	KprRetainableRetain(self->retainable);
	return self;
}

Boolean KprCoAPMessageIsConfirmable(KprCoAPMessage self)
{
	return (self->type == kKprCoAPMessageTypeConfirmable);
}


static int KprCoAPMessageCompareOption(const void *a, const void *b)
{
	int o1 = ((KprCoAPMessageOptionRecord *)a)->option;
	int o2 = ((KprCoAPMessageOptionRecord *)b)->option;

	return o1 - o2;
}

static FskErr KprCoAPMessageAppendOption(KprCoAPMessage self, KprCoAPMessageOption option, KprCoAPMessageOptionFormat format, KprCoAPMessageOptionRecord **it)
{
	FskErr err = kFskErrNone;
	KprCoAPMessageOptionRecord *optRec;

	bailIfError(KprMemPtrNewClear(sizeof(KprCoAPMessageOptionRecord), &optRec));

	optRec->option = option;
	optRec->format = format;

	FskListInsertSorted(&self->options, optRec, KprCoAPMessageCompareOption);
	*it = optRec;

bail:
	return err;
}

static void KprCoAPMessageDisposeOption(KprCoAPMessageOptionRecord *optRec)
{
	switch (optRec->format) {
		case kKprCoAPMessageOptionFormatOpaque:
			KprMemPtrDispose((void *) optRec->value.opaque.data);
			break;

		case kKprCoAPMessageOptionFormatString:
			KprMemPtrDispose((void *) optRec->value.string);
			break;

		default:
			break;
	}
	KprMemPtrDispose(optRec);
}

FskErr KprCoAPMessageAppendEmptyOption(KprCoAPMessage self, KprCoAPMessageOption option)
{
	FskErr err = kFskErrNone;
	KprCoAPMessageOptionRecord *optRec;

	bailIfError(KprCoAPMessageAppendOption(self, option, kKprCoAPMessageOptionFormatEmpty, &optRec));

bail:
	return err;
}

FskErr KprCoAPMessageAppendOpaqueOption(KprCoAPMessage self, KprCoAPMessageOption option, const void *data, UInt32 length)
{
	FskErr err = kFskErrNone;
	KprCoAPMessageOptionRecord *optRec;

	if (length > kKprCoAPMessageOptionLengthMax) {
		bailIfError(kFskErrBadData);
	}

	bailIfError(KprCoAPMessageAppendOption(self, option, kKprCoAPMessageOptionFormatOpaque, &optRec));

	if (length > 0) {
		bailIfError(KprMemPtrNewFromData(length, data, &optRec->value.opaque.data));
	}
	optRec->value.opaque.length = length;

bail:
	return err;
}

FskErr KprCoAPMessageAppendUintOption(KprCoAPMessage self, KprCoAPMessageOption option, UInt32 uint)
{
	FskErr err = kFskErrNone;
	KprCoAPMessageOptionRecord *optRec;

	bailIfError(KprCoAPMessageAppendOption(self, option, kKprCoAPMessageOptionFormatUint, &optRec));

	optRec->value.uint = uint;

bail:
	return err;
}

FskErr KprCoAPMessageAppendStringOption(KprCoAPMessage self, KprCoAPMessageOption option, const char *string)
{
	FskErr err = kFskErrNone;
	KprCoAPMessageOptionRecord *optRec;

	if (FskStrLen(string) > kKprCoAPMessageOptionLengthMax) {
		bailIfError(kFskErrBadData);
	}

	bailIfError(KprCoAPMessageAppendOption(self, option, kKprCoAPMessageOptionFormatString, &optRec));

	optRec->value.string = FskStrDoCopy(string);
	bailIfNULL(optRec->value.string);

bail:
	return err;
}

KprCoAPMessageOptionRecord *KprCoAPMessageFindOption(KprCoAPMessage self, KprCoAPMessageOption option)
{
	KprCoAPMessageOptionRecord *optRec = self->options;
	while (optRec) {
		if (optRec->option == option) return optRec;
		if (optRec->option > option) break;
		optRec = optRec->next;
	}
	return NULL;
}

FskErr KprCoAPMessageRemoveOptions(KprCoAPMessage self, KprCoAPMessageOption option)
{
	FskErr err = kFskErrNone;
	KprCoAPMessageOptionRecord *optRec;

	while ((optRec = KprCoAPMessageFindOption(self, option)) != NULL) {
		FskListRemove(&self->options, optRec);
		KprCoAPMessageDisposeOption(optRec);
	}
	return err;

}

FskErr KprCoAPMessageSetToken(KprCoAPMessage self, const void *token, UInt32 length)
{
	FskErr err = kFskErrNone;

	if (length > kKprCoAPMessageTokenLengthMax) {
		bailIfError(kFskErrBadData);
	}

	KprMemoryChunkDisposeAt(&self->token);

	bailIfError(KprMemoryChunkNew(length, token, &self->token));

bail:
	return err;
}

FskErr KprCoAPMessageSetPayload(KprCoAPMessage self, const void *payload, UInt32 length)
{
	FskErr err = kFskErrNone;

	KprMemoryChunkDisposeAt(&self->payload);
	bailIfError(KprMemoryChunkNew(length, payload, &self->payload));

bail:
	return err;
}

KprCoAPContentFormat KprCoAPMessageGetContentFormat(KprCoAPMessage self)
{
	KprCoAPMessageOptionRecord *optRec = KprCoAPMessageFindOption(self, kKprCoAPMessageOptionContentFormat);
	if (optRec == NULL) return kKprCoAPContentFormatNone;

	return optRec->value.uint;
}

FskErr KprCoAPMessageSetContentFormat(KprCoAPMessage self, KprCoAPContentFormat format)
{
	FskErr err = kFskErrNone;

	bailIfError(KprCoAPMessageRemoveOptions(self, kKprCoAPMessageOptionContentFormat));
	bailIfError(KprCoAPMessageAppendUintOption(self, kKprCoAPMessageOptionContentFormat, format));

bail:
	return err;
}

typedef struct {
	UInt8 value;
	UInt8 extraBytes;
	UInt8 extraValue[2];
} KprCoAPMessagePackedOptionValue;

static KprCoAPMessagePackedOptionValue KprCoAPMessagePackOptValue(UInt32 value)
{
	KprCoAPMessagePackedOptionValue packed;

	/*
	 0 - 12
	 13: 0 - 255 => 13 - 268
	 14: 0 - 65535 => 269 - 65804
	 */

	if (value <= 12) {
		packed.value = value;
		packed.extraBytes = 0;
	} else if (value <= 268) {
		packed.value = 13;
		packed.extraBytes = 1;
		packed.extraValue[0] = value - 13;
	} else {
		packed.value = 14;
		packed.extraBytes = 2;

		value = value - 269;
		packed.extraValue[0] = (value >> 8) & 0xff;
		packed.extraValue[1] = (value >> 0) & 0xff;
	}
	return packed;
}

static UInt32 KprCoAPMessageOptionLength(KprCoAPMessageOptionRecord *optRec)
{
	switch (optRec->format) {
		case kKprCoAPMessageOptionFormatEmpty:
			return 0;

		case kKprCoAPMessageOptionFormatOpaque:
			return optRec->value.opaque.length;

		case kKprCoAPMessageOptionFormatUint:
			if (optRec->value.uint == 0) return 0;
			if (optRec->value.uint <= 0xff) return 1;
			if (optRec->value.uint <= 0xffff) return 2;
			if (optRec->value.uint <= 0xffffff) return 3;
			return 4;

		case kKprCoAPMessageOptionFormatString:
			return FskStrLen(optRec->value.string);
	}

	return 0;
}

static UInt32 KprCoAPMessageCalculateSerializedSize(KprCoAPMessage self)
{
	UInt32 size;
	KprCoAPMessageOptionRecord *optRec;
	KprCoAPMessageOption prev;

	size = sizeof(UInt32);
	if (self->token) {
		size += self->token->size;
	}

	if (self->payload) {
		size += self->payload->size + 1;
	}

	optRec = self->options;
	prev = 0;
	while (optRec) {
		UInt32 optLen, optDelta;
		KprCoAPMessagePackedOptionValue packed;

		size += 1;

		optLen = KprCoAPMessageOptionLength(optRec);
		packed = KprCoAPMessagePackOptValue(optLen);
		size += packed.extraBytes;

		optDelta = optRec->option - prev;
		prev = optRec->option;
		packed = KprCoAPMessagePackOptValue(optDelta);
		size += packed.extraBytes;

		size += optLen;

		optRec = optRec->next;
	}

	return size;
}

static void KprCoAPMessageSerializeTo(KprCoAPMessage self, FskMemPtr data)
{
	UInt32 header, version, type, tokenLength, code, messageId;
	KprCoAPMessageOptionRecord *optRec;
	KprCoAPMessageOption prev;

	/*
	 0                   1                   2                   3
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 |Ver| T |  TKL  |      Code     |          Message ID           |
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 |   Token (if any, TKL bytes) ...
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 |   Options (if any) ...
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 |1 1 1 1 1 1 1 1|    Payload (if any) ...
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 */

	// fixed header
	version = 0x01 << 30;
	type = self->type << 28;
	tokenLength = self->token ? self->token->size << 24 : 0;
	code = self->code << 16;
	messageId = self->messageId;
	header = version | type | tokenLength | code | messageId;

	header = FskEndianU32_NtoB(header);
	FskMemCopy(data, &header, sizeof(UInt32));
	data += sizeof(UInt32);

	// token
	if (self->token) {
		data = KprMemoryChunkCopyTo(self->token, data);
	}

	// options
	optRec = self->options;
	prev = 0;
	while (optRec) {
		UInt32 opt, len, val32;
		KprCoAPMessagePackedOptionValue optDelta, optLen;
		UInt8 byte, *p;

		/*
		 0   1   2   3   4   5   6   7
		 +---------------+---------------+
		 |  Option Delta | Option Length |   1 byte
		 +---------------+---------------+
		 /         Option Delta          /   0-2 bytes
		 \          (extended)           \
		 +-------------------------------+
		 /         Option Length         /   0-2 bytes
		 \          (extended)           \
		 +-------------------------------+
		 /         Option Value          /   0 or more bytes
		 +-------------------------------+
		 */

		opt = optRec->option - prev;
		prev = optRec->option;
		optDelta = KprCoAPMessagePackOptValue(opt);

		len = KprCoAPMessageOptionLength(optRec);
		optLen = KprCoAPMessagePackOptValue(len);

		byte = optDelta.value << 4 | optLen.value;
		*(UInt8 *)data++ = byte;

		if (optDelta.extraBytes > 0) {
			FskMemCopy(data, optDelta.extraValue, optDelta.extraBytes);
			data += optDelta.extraBytes;
		}

		if (optLen.extraBytes > 0) {
			FskMemCopy(data, optLen.extraValue, optLen.extraBytes);
			data += optLen.extraBytes;
		}

		if (len > 0) {
			switch (optRec->format) {
				case kKprCoAPMessageOptionFormatEmpty:
					break;

				case kKprCoAPMessageOptionFormatOpaque:
					FskMemCopy(data, optRec->value.opaque.data, len);
					break;

				case kKprCoAPMessageOptionFormatUint:
					val32 = optRec->value.uint;
					val32 = FskEndianU32_NtoB(val32);
					p = (UInt8 *)&val32;

					FskMemCopy(data, p + (sizeof(UInt32) - len), len);
					break;

				case kKprCoAPMessageOptionFormatString:
					FskMemCopy(data, optRec->value.string, len);
			}
			data += len;
		}

		optRec = optRec->next;
	}

	// payload
	if (self->payload) {
		*(UInt8 *)data++ = 0xff;
		KprMemoryChunkCopyTo(self->payload, data);
	}
}

FskErr KprCoAPMessageSerialize(KprCoAPMessage self, KprMemoryChunk *it)
{
	FskErr err = kFskErrNone;
	KprMemoryChunk chunk = NULL;
	UInt32 size;

	size = KprCoAPMessageCalculateSerializedSize(self);
	bailIfError(KprMemoryChunkNew(size, NULL, &chunk));

	KprCoAPMessageSerializeTo(self, KprMemoryChunkStart(chunk));

	*it = chunk;

bail:
	if (err) {
		KprMemoryChunkDispose(chunk);
	}
	return err;
}

#define PTR_ADD(p, v) (const void *)((FskMemPtr) p + v)

typedef struct KprCoAPMessageDataReaderRecord {
	const void *start;
	const void *end;
	UInt32 index;
} KprCoAPMessageDataReaderRecord, *KprCoAPMessageDataReader;

static void KprCoAPMessageDataReaderInit(KprCoAPMessageDataReader reader, const void *buffer, UInt32 size)
{
	reader->start = buffer;
	reader->end = PTR_ADD(buffer, size);
	reader->index = 0;
}

static Boolean KprCoAPMessageDataReaderIsEOF(KprCoAPMessageDataReader reader)
{
	return PTR_ADD(reader->start, reader->index) >= reader->end;
}

static Boolean KprCoAPMessageDataReaderGetPtr(KprCoAPMessageDataReader reader, UInt32 size, const void **p)
{
	*p = PTR_ADD(reader->start, reader->index);
	reader->index += size;

	if (PTR_ADD(*p, size) > reader->end) return false;
	return true;
}

static Boolean KprCoAPMessageDataReaderRead(KprCoAPMessageDataReader reader, void *buffer, UInt32 size)
{
	const void *p;
	if (!KprCoAPMessageDataReaderGetPtr(reader, size, &p)) return false;
	FskMemCopy(buffer, p, size);
	return true;
}

#define CHECK(test) if (!(test)) { err = kFskErrBadData; goto bail; }
#define READ(p, s) CHECK(KprCoAPMessageDataReaderRead(&reader, p, s))
#define READ_INTO(val) READ(&val, sizeof(val))
#define HAS_DATA() !KprCoAPMessageDataReaderIsEOF(&reader)

static Boolean KprCoAPMessageValidateMessage(KprCoAPMessage self)
{
	if (self->code == 0) {
		if (self->token && self->token->size > 0) return false;
		if (self->payload && self->payload->size > 0) return false;
		return true;
	}

	return true;
}

FskErr KprCoAPMessageDeserialize(const void *buffer, UInt32 size, KprCoAPMessage *it)
{
	FskErr err = kFskErrNone;
	KprCoAPMessage self = NULL;
	KprCoAPMessageDataReaderRecord reader;
	UInt32 header, version, len;
	Boolean hasPayloadMarker;
	KprCoAPMessageOption option;
	const void *p;
	char *str = NULL;

	bailIfError(KprCoAPMessageNew(&self));

	KprCoAPMessageDataReaderInit(&reader, buffer, size);

	/*
	 0                   1                   2                   3
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 |Ver| T |  TKL  |      Code     |          Message ID           |
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 |   Token (if any, TKL bytes) ...
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 |   Options (if any) ...
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 |1 1 1 1 1 1 1 1|    Payload (if any) ...
	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 */

	// fixed header
	READ_INTO(header);
	header = FskEndianU32_BtoN(header);

	version = (header >> 30) & 0x03;
	CHECK(version == 1);

	self->type = (header >> 28) & 0x03;
	len = (header >> 24) & 0xf;
	self->code = (header >> 16) & 0xff;
	self->messageId = (header) & 0xffff;

	// token
	if (len > 0) {
		const void *p;
		CHECK(KprCoAPMessageDataReaderGetPtr(&reader, len, &p));
		bailIfError(KprCoAPMessageSetToken(self, p, len));
	}

	// options
	hasPayloadMarker = false;
	option = 0;

	while (HAS_DATA()) {
		UInt32 opt;
		UInt8 byte, *byteP;
		UInt16 val16;
		UInt32 val32;
		KprCoAPMessageOptionFormat format;

		/*
		 0   1   2   3   4   5   6   7
		 +---------------+---------------+
		 |  Option Delta | Option Length |   1 byte
		 +---------------+---------------+
		 /         Option Delta          /   0-2 bytes
		 \          (extended)           \
		 +-------------------------------+
		 /         Option Length         /   0-2 bytes
		 \          (extended)           \
		 +-------------------------------+
		 /         Option Value          /   0 or more bytes
		 +-------------------------------+
		 */

		READ_INTO(byte);
		if (byte == 0xff) {
			hasPayloadMarker = true;
			break;
		}

		opt = (byte >> 4) & 0xf;
		len = (byte) & 0xf;

		if (opt == 13) {
			READ_INTO(byte);
			opt = 13 + byte;
		} else if (opt == 14) {
			READ_INTO(val16);
			opt = 13 + 256 + val16;
		} else {
			CHECK(opt != 15);
		}

		if (len == 13) {
			READ_INTO(byte);
			len = 13 + byte;
		} else if (len == 14) {
			READ_INTO(val16);
			len = 13 + 256 + val16;
		} else {
			CHECK(opt != 15);
		}

		option += opt;
		CHECK(KprCoAPMessageDataReaderGetPtr(&reader, len, &p));

		format = KprCoAPMessageOptionGetFormat(option);
		switch (format) {
			case kKprCoAPMessageOptionFormatEmpty:
				CHECK(len == 0);
				bailIfError(KprCoAPMessageAppendEmptyOption(self, option));
				break;

			case kKprCoAPMessageOptionFormatOpaque:
				bailIfError(KprCoAPMessageAppendOpaqueOption(self, option, p, len));
				break;

			case kKprCoAPMessageOptionFormatUint:
				byteP = (UInt8 *)p;
				val32 = 0;
				while (len-- > 0) {
					val32 = val32 * 256 + *byteP++;
				}
				bailIfError(KprCoAPMessageAppendUintOption(self, option, val32));
				break;

			case kKprCoAPMessageOptionFormatString:
				bailIfError(KprMemPtrNew(len + 1, &str));
				FskMemCopy(str, p, len);
				str[len] = 0;
				bailIfError(KprCoAPMessageAppendStringOption(self, option, str));
				KprMemPtrDispose(str);
				str = NULL;
				break;
		}
	}

	// payload
	if (HAS_DATA()) {
		CHECK(hasPayloadMarker == true);

		CHECK(KprCoAPMessageDataReaderGetPtr(&reader, 0, &p));
		len = size - reader.index;
		bailIfError(KprCoAPMessageSetPayload(self, p, len));
	} else {
		CHECK(hasPayloadMarker == false);
	}

	CHECK(KprCoAPMessageValidateMessage(self));

	*it = self;

bail:
	KprMemPtrDispose(str);

	if (err) {
		KprCoAPMessageDispose(self);
	}
	return err;
}

KprCoAPMessageOptionFormat KprCoAPMessageOptionGetFormat(KprCoAPMessageOption option)
{
	switch (option) {
		case kKprCoAPMessageOptionIfNoneMatch:
			return kKprCoAPMessageOptionFormatEmpty;

		case kKprCoAPMessageOptionIfMatch:
		case kKprCoAPMessageOptionETag:
			return kKprCoAPMessageOptionFormatOpaque;

		case kKprCoAPMessageOptionUriPort:
		case kKprCoAPMessageOptionContentFormat:
		case kKprCoAPMessageOptionMaxAge:
		case kKprCoAPMessageOptionAccept:
		case kKprCoAPMessageOptionSize1:
		case kKprCoAPMessageOptionBlock2:
		case kKprCoAPMessageOptionBlock1:
		case kKprCoAPMessageOptionSize2:
		case kKprCoAPMessageOptionObserve:
			return kKprCoAPMessageOptionFormatUint;

		case kKprCoAPMessageOptionUriHost:
		case kKprCoAPMessageOptionLocationPath:
		case kKprCoAPMessageOptionUriPath:
		case kKprCoAPMessageOptionUriQuery:
		case kKprCoAPMessageOptionLocationQuery:
		case kKprCoAPMessageOptionProxyUri:
		case kKprCoAPMessageOptionProxyScheme:
			return kKprCoAPMessageOptionFormatString;
	}

	return kKprCoAPMessageOptionFormatEmpty;
}

static Boolean KprCoAPMessageIsValidURL(KprURLParts parts);
static FskErr KprCoAPMessageParseAndAddOptionComponets(KprCoAPMessage request, char *str, char delimiter, KprCoAPMessageOption option);

FskErr KprCoAPMessageParseUri(KprCoAPMessage self, const char *uri)
{
	FskErr err = kFskErrNone;
	KprURLPartsRecord parts;
	char *host = NULL;
	char *path = NULL;
	char *query = NULL;
	int ipaddr, len;

	KprURLSplit((char *) uri, &parts);
	if (!KprCoAPMessageIsValidURL(&parts)) bailIfError(kFskErrInvalidParameter);

	bailIfError(KprCoAPStrNewWithLength(parts.host, parts.hostLength, &host));
	bailIfError(KprCoAPStrNewWithLength(parts.path, parts.pathLength, &path));
	bailIfError(KprCoAPStrNewWithLength(parts.query, parts.queryLength, &query));

	if (query) {
		bailIfError(KprCoAPMessageParseAndAddOptionComponets(self, query, '&', kKprCoAPMessageOptionUriQuery));
	}

	if (path) {
		len = FskStrLen(path);
		if (!(len == 0 || (len == 1 && *path == '/'))) {
			bailIfError(KprCoAPMessageParseAndAddOptionComponets(self, path + 1, '/', kKprCoAPMessageOptionUriPath));
		}
	}

	if (!FskStrIsDottedQuad(host, &ipaddr)) {
		bailIfError(KprCoAPMessageAppendStringOption(self, kKprCoAPMessageOptionUriHost, host));
	}

	self->uri = FskStrDoCopy(uri);
	bailIfNULL(self->uri);

	self->host = host;
	self->path = path;
	self->query	= query;
	self->port = parts.port;

bail:

	if (err) {
		KprMemPtrDispose(host);
		KprMemPtrDispose(path);
		KprMemPtrDispose(query);
	}

	return err;
}

FskErr KprCoAPMessageBuildUri(KprCoAPMessage request)
{
	FskErr err = kFskErrNone;
	KprCoAPMessageOptionRecord *optRec;
	int hostLen = 0, portLen = 0, pathLen = 0, queryLen = 0;
	char *uri = NULL, *host = NULL, portStr[10], *path = NULL, *query = NULL;
	UInt32 len = 0;

	// coap://[hostname](:[port])/acd/dfe/gih?a=b&b=c
	//   7 +  host.len +
	//                 port.len + 1
	//

	optRec = request->options;
	while (optRec) {
		switch (optRec->option) {
			case kKprCoAPMessageOptionUriHost:
				hostLen = FskStrLen(optRec->value.string);
				break;

			case kKprCoAPMessageOptionUriPort:
				FskStrNumToStr(optRec->value.uint, portStr + 1, sizeof(portStr) - 1);
				portStr[0] = ':';
				portLen = FskStrLen(portStr);
				break;

			case kKprCoAPMessageOptionUriPath:
				pathLen += 1 + FskStrCountEscapedCharsToEncode(optRec->value.string);
				break;

			case kKprCoAPMessageOptionUriQuery:
				queryLen += 1 + FskStrCountEscapedCharsToEncode(optRec->value.string);
				break;

			default:
				break;
		}

		optRec = optRec->next;
	}

	len = 7 + hostLen + portLen + pathLen + queryLen;

	bailIfError(KprMemPtrNew(len + 1, &request->uri));
	uri = (char *)request->uri;

	if (hostLen > 0) {
		bailIfError(KprMemPtrNew(hostLen + 1, &request->host));
		host = (char *)request->host;
		*host = 0;
	}

	if (pathLen > 0) {
		bailIfError(KprMemPtrNew(pathLen + 1, &request->path));
		path = (char *)request->path;
		*path = 0;
	}

	if (queryLen > 0) {
		bailIfError(KprMemPtrNew((queryLen - 1) + 1, &request->query));
		query = (char *)request->query;
		*query = 0;
	}

	optRec = request->options;
	while (optRec) {
		switch (optRec->option) {
			case kKprCoAPMessageOptionUriHost:
				FskStrCopy(host, optRec->value.string);
				break;

			case kKprCoAPMessageOptionUriPort:
				request->port = optRec->value.uint;
				break;

			case kKprCoAPMessageOptionUriPath:
				FskStrCat(path, "/");
				FskStrEncodeEscapedChars(optRec->value.string, path + FskStrLen(path));
				break;

			case kKprCoAPMessageOptionUriQuery:
				if (*query) FskStrCat(query, "&");
				FskStrEncodeEscapedChars(optRec->value.string, query + FskStrLen(query));
				break;

			default:
				break;
		}

		optRec = optRec->next;
	}

	FskStrCopy(uri, "coap://");

	if (host) {
		FskStrCat(uri, host);
	}

	if (portLen > 0) {
		FskStrCat(uri, portStr);
	}

	if (path) {
		FskStrCat(uri, path);
	}

	if (query) {
		FskStrCat(uri, "?");
		FskStrCat(uri, query);
	}

bail:
	return err;
}

static Boolean KprCoAPMessageIsValidURL(KprURLParts parts)
{
	if (parts->schemeLength != 4) return false;
	if (FskStrCompareCaseInsensitiveWithLength(parts->scheme, "coap", 4) != 0) return false;

	if (parts->hostLength == 0) return false;
	if (parts->fragmentLength > 0) return false;

	if (parts->port == 0 || parts->port == 80) {
		parts->port = 5683;
	}

	return true;
}

static FskErr KprCoAPMessageParseAndAddOptionComponets(KprCoAPMessage request, char *str, char delimiter, KprCoAPMessageOption option)
{
	FskErr err = kFskErrNone;
	int len = FskStrLen(str);

	char *p = str, *end = p + len;
	while (p < end) {
		if (*p == delimiter) *p = 0;
		p += 1;
	}
	p = str;

	while (p < end) {
		len = FskStrLen(p);
		FskStrDecodeEscapedChars(p, p);

		bailIfError(KprCoAPMessageAppendStringOption(request, option, p));

		p += len + 1;
	}

bail:
	return err;
}

