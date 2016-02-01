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
#ifndef __KPRCOAPMESSAGE__
#define __KPRCOAPMESSAGE__

#include "kpr.h"
#include "kprCoAPCommon.h"
#include "kprUtilities.h"

#define kKprCoAPMessageTokenLengthMax 8L
#define kKprCoAPMessageOptionLengthMax 65804L

//--------------------------------------------------
// Typedefs
//--------------------------------------------------

typedef struct KprCoAPMessageOptionRecord KprCoAPMessageOptionRecord;

struct KprCoAPMessageRecord {
	KprRetainable retainable;
	KprCoAPMessageType type;
	KprCoAPMessageCode code;
	UInt16 messageId;
	Boolean frozen;

	KprMemoryBlock token;

	KprCoAPMessageOptionRecord *options;

	KprMemoryBlock payload;

	const char *uri;
	const char *host;
	UInt16 port;
	const char *path;
	const char *query;
};

struct KprCoAPMessageOptionRecord {
	KprCoAPMessageOptionRecord *next;
	KprCoAPMessageOption option;
	KprCoAPMessageOptionFormat format;
	union {
		struct {
			UInt32 length;
			const void *data;
		} opaque;
		UInt32 uint;
		const char *string;
	} value;
};

//--------------------------------------------------
// Methods
//--------------------------------------------------

FskErr KprCoAPMessageNew(KprCoAPMessage *it);
FskErr KprCoAPMessageDispose(KprCoAPMessage self);
FskErr KprCoAPMessageDisposeAt(KprCoAPMessage *it);

FskErr KprCoAPMessageCopy(KprCoAPMessage original, KprCoAPMessage *it);
KprCoAPMessage KprCoAPMessageRetain(KprCoAPMessage self);

Boolean KprCoAPMessageIsConfirmable(KprCoAPMessage self);

FskErr KprCoAPMessageAppendEmptyOption(KprCoAPMessage self, KprCoAPMessageOption option);
FskErr KprCoAPMessageAppendOpaqueOption(KprCoAPMessage self, KprCoAPMessageOption option, const void *data, UInt32 length);
FskErr KprCoAPMessageAppendUintOption(KprCoAPMessage self, KprCoAPMessageOption option, UInt32 uint);
FskErr KprCoAPMessageAppendStringOption(KprCoAPMessage self, KprCoAPMessageOption option, const char *string);
FskErr KprCoAPMessageRemoveOptions(KprCoAPMessage self, KprCoAPMessageOption option);
KprCoAPMessageOptionRecord *KprCoAPMessageFindOption(KprCoAPMessage self, KprCoAPMessageOption option);
KprCoAPMessageOptionFormat KprCoAPMessageOptionGetFormat(KprCoAPMessageOption option);

FskErr KprCoAPMessageSetToken(KprCoAPMessage self, const void *token, UInt32 length);
FskErr KprCoAPMessageSetPayload(KprCoAPMessage self, const void *payload, UInt32 length);

KprCoAPContentFormat KprCoAPMessageGetContentFormat(KprCoAPMessage self);
FskErr KprCoAPMessageSetContentFormat(KprCoAPMessage self, KprCoAPContentFormat format);

FskErr KprCoAPMessageSerialize(KprCoAPMessage self, KprMemoryBlock *chunk);
FskErr KprCoAPMessageDeserialize(const void *buffer, UInt32 size, KprCoAPMessage *it);

FskErr KprCoAPMessageParseUri(KprCoAPMessage request, const char *uri);
FskErr KprCoAPMessageBuildUri(KprCoAPMessage request);

#endif
