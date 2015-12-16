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
#include "kprCoAPClientResolver.h"
#include "kprCoAPMessage.h"
#include "kprCoAPEndpoint.h"
#include "kprCoAPReceiver.h"

static void KprCoAPClientResolverResolved(struct FskResolverRecord *rr);

FskErr KprCoAPClientResolverNew(KprCoAPClientResolver *it, KprCoAPClient client, const char *host, UInt16 port, KprCoAPMessage message)
{
	FskErr err = kFskErrNone;
	KprCoAPClientResolver self = NULL;

	bailIfError(KprMemPtrNewClear(sizeof(KprCoAPClientResolverRecord), &self));

	self->host = FskStrDoCopy(host);
	bailIfNULL(self->host);

	bailIfError(KprCoAPMessageQueueAppend(&self->waiting, message));

	self->client = client;
	self->port = port;

	bailIfError(FskNetHostnameResolveQTAsync((char *) host, 0, KprCoAPClientResolverResolved, self, &self->resolver));

	bailIfError(self->err);

	self->constructed = true;
	*it = self;

bail:
	if (err) {
		KprCoAPClientResolverDispose(self);
	}
	return err;
}

FskErr KprCoAPClientResolverDispose(KprCoAPClientResolver self)
{
	FskErr err = kFskErrNone;
	if (self) {
		FskDebugStr("CoAP ClientDestination Disposed: %p", self);

		KprMemPtrDispose((char *) self->host);

		KprCoAPMessageQueueDispose(self->waiting);

		if (self->resolver) FskResolverCancel(self->resolver);

		KprMemPtrDispose(self);
	}
	return err;
}

Boolean KprCoAPClientResolverIsResolved(KprCoAPClientResolver self)
{
	return (self->resolver == NULL);
}

FskErr KprCoAPClientResolverQueueMessage(KprCoAPClientResolver self, KprCoAPMessage message)
{
	return KprCoAPMessageQueueAppend(&self->waiting, message);
}

static void KprCoAPClientResolverResolved(FskResolver rr)
{
	FskErr err = kFskErrNone;

	KprCoAPClientResolver self = (KprCoAPClientResolver)rr->ref;
	KprCoAPMessageQueue entry;

	self->resolver = NULL;

	bailIfError(rr->err);

	self->ipaddr = rr->resolvedIP;
	FskTimeGetNow(&self->resolvedAt);

	entry = self->waiting;
	while (entry) {
		KprCoAPMessageQueue next = entry->next;
		KprCoAPMessage message = entry->message;

		bailIfError(KprCoAPClientStartRequest(self->client, self->ipaddr, self->port, message));

		entry = next;
	}


bail:
	if (err) {
		if (self->constructed) {
			KprCoAPClientReportError(self->client, err, "dns resolve error");
		} else {
			self->err = err;
		}
	}
}

