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
#include "FskHTTPClient.h"
#include "FskUUID.h"
#include "md5.h"

#include "kpr.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHTTPCookies.h"
#include "kprHTTPCache.h"
#include "kprHTTPClient.h"
#include "kprHTTPKeychain.h"
#include "kprMessage.h"
#include "kprSkin.h"
#include "kprURL.h"
#include "kprUtilities.h"
#include "kprShell.h"
#include "kprTransition.h"

typedef struct KprHTTPClientStruct KprHTTPClientRecord, *KprHTTPClient;
typedef struct KprHTTPConnectionStruct KprHTTPConnectionRecord, *KprHTTPConnection;
typedef struct KprHTTPTargetStruct KprHTTPTargetRecord, *KprHTTPTarget;

struct KprHTTPClientStruct {
	FskList connections;
	FskList targets;
	KprHTTPCache cache;
	KprHTTPCookies cookies;
	KprHTTPKeychain keychain;
	xsMachine* the;
	xsIndex* code;
	UInt32 contentLength;
	UInt32 connectionTimeout;
	FskInstrumentedItemDeclaration
};

struct KprHTTPConnectionStruct {
	KprHTTPConnection next;
	KprHTTPTarget target;
	UInt32 id;
	FskHTTPClient client;
	UInt32 priority;
	FskTimeRecord time;
	FskInstrumentedItemDeclaration
};

struct KprHTTPTargetStruct {
	KprHTTPTarget next;
	KprMessage message;
	FskThread thread;
	UInt32 redirects;
	UInt32 requestDate;
	UInt32 requestOffset;
	UInt32 responseDate;
	UInt32 responseOffset;
	KprHTTPCacheValue cached;
	FskInstrumentedItemDeclaration
};

/* HTTP CLIENT */

static void KprHTTPClientStart(KprService self, FskThread thread, xsMachine* the);
static void KprHTTPClientStop(KprService self);

//static void KprHTTPClientCacheCleanup();
//static void KprHTTPClientCacheClear();

//static void KprHTTPClientCookiesClear();
static void KprHTTPClientCookiesCleanup(Boolean session);
static FskErr KprHTTPClientCookiesPut(char* url, char* cookies);

static FskErr KprHTTPClientAddTarget(KprHTTPTarget target);
static void KprHTTPClientCancel(KprService service, KprMessage message);
//static void KprHTTPClientCancelTarget(KprHTTPTarget target);
static void KprHTTPClientInvoke(KprService service, KprMessage message);
static void KprHTTPClientIterate();
static Boolean KprHTTPClientPutTargetCache(KprHTTPTarget target);
static FskErr KprHTTPClientReadCache(KprMessage message, KprHTTPCacheValue cached);
//static void KprHTTPClientRemoveTarget(KprHTTPTarget target);
static void KprHTTPClientRemoveTargetCache(KprHTTPTarget target);
static FskErr KprHTTPClientResetTarget(KprHTTPTarget target);
static void KprHTTPClientResumeTarget(KprHTTPTarget target);

/* HTTP CONNECTION */

FskErr KprHTTPConnectionPoolNew(UInt32 size);
void KprHTTPConnectionPoolDispose();
void KprHTTPConnectionPoolIterate();

static FskErr KprHTTPConnectionNew(KprHTTPConnection* it);
static void KprHTTPConnectionDispose(KprHTTPConnection self);
static Boolean KprHTTPConnectionAvailable(KprHTTPConnection self);
static Boolean KprHTTPConnectionCandidate(KprHTTPConnection self, KprMessage message);
static void KprHTTPConnectionClose(KprHTTPConnection self);
static FskErr KprHTTPConnectionProcess(KprHTTPConnection self, KprHTTPTarget target);

static FskErr KprHTTPConnectionSendingCallback(FskHTTPClientRequest request, char** data, int* size, FskInt64 offset, void* context);
static FskErr KprHTTPConnectionAuthCallback(FskHTTPClient client, FskHTTPClientRequest request, char *url, char *realm, FskHTTPAuth auth, void *refCon);
static FskErr KprHTTPConnectionHeadersCallback(FskHTTPClientRequest request, FskHeaders* responseHeaders, void* context);
static FskErr KprHTTPConnectionReceivingCallback(FskHTTPClientRequest request, char* data, int size, void* context);
static FskErr KprHTTPConnectionCompleteCallback(FskHTTPClient client, FskHTTPClientRequest request, void* context);
static FskErr KprHTTPConnectionClosedCallback(FskHTTPClient client, void* context);

/* HTTP TARGET */

static FskErr KprHTTPTargetNew(KprHTTPTarget* it, KprMessage message);
static void KprHTTPTargetDispose(void* it);

/* HTTP CLIENT */

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprHTTPClientInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprHTTPClient", FskInstrumentationOffset(KprHTTPClientRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

#define kprHTTPCacheDiskSize 0x10000000
//#define kprHTTPCacheDiskSize 0x10000

KprHTTPClient gKprHTTPClient = NULL;

KprServiceRecord gHTTPService = {
	NULL,
	kprServicesThread,
	"http:",
	NULL,
	NULL,
	KprServiceAccept,
	KprHTTPClientCancel,
	KprHTTPClientInvoke,
	KprHTTPClientStart,
	KprHTTPClientStop,
	NULL,
	NULL,
	NULL
};

KprServiceRecord gHTTPSService = {
	NULL,
	kprServicesThread,
	"https:",
	NULL,
	NULL,
	KprServiceAccept,
	KprHTTPClientCancel,
	KprHTTPClientInvoke,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

void KprHTTPClientStart(KprService service, FskThread thread, xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* cachePath = NULL;
	char* preferencePath = NULL;
	char* temp = NULL;
	KprHTTPClient self = NULL;
	UInt32 i, size;
	KprHTTPConnection connection;
	
	bailIfError(FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeCache, true, NULL, &cachePath));
	bailIfError(FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeApplicationPreference, true, NULL, &preferencePath));
	bailIfError(FskMemPtrNewClear(sizeof(KprHTTPClientRecord), &self));
	FskInstrumentedItemNew(self, NULL, &KprHTTPClientInstrumentation);
	
	gKprHTTPClient = self;
	
	/* HTTP CACHE */
	bailIfError(KprURLMerge(cachePath, "kpr.cache", &temp));
	bailIfError(KprHTTPCacheNew(&self->cache,
		KprEnvironmentGetUInt32("httpCacheSize", 197),
		KprEnvironmentGetUInt32("httpCacheDiskSize", kprHTTPCacheDiskSize),
		temp));
	KprHTTPCacheRead(self->cache);
	FskMemPtrDisposeAt(&temp);
	
	/* COOKIES */
	bailIfError(KprURLMerge(preferencePath, "kpr.cookies", &temp));	
	bailIfError(KprHTTPCookiesNew(&self->cookies, KprEnvironmentGetUInt32("httpCookiesSize", 197), temp));
	KprHTTPCookiesRead(self->cookies);
	FskMemPtrDisposeAt(&temp);
	
	/* KEYCHAIN */
	bailIfError(KprURLMerge(preferencePath, "kpr.keychain", &temp));	
	bailIfError(KprHTTPKeychainNew(&self->keychain, KprEnvironmentGetUInt32("httpKeychainSize", 197), temp));
	KprHTTPKeychainRead(self->keychain);
	FskMemPtrDisposeAt(&temp);

	/* CONNECTIONS */
	size = KprEnvironmentGetUInt32("httpPoolSize", 5);
	for (i = 0; i < size; i++) {
		bailIfError(KprHTTPConnectionNew(&connection));
		FskInstrumentedItemSetOwner(connection, self);
		connection->id = i;
		FskListAppend(&self->connections, connection);
	}
	
	self->contentLength = KprEnvironmentGetUInt32("httpContentLength", 0x800000);
	self->connectionTimeout = KprEnvironmentGetUInt32("httpConnectionTimeout", 0);
	
	gHTTPService.machine = the;
	gHTTPService.thread = thread;
	gHTTPSService.machine = the;
	gHTTPSService.thread = thread;
	
bail:
	FskMemPtrDispose(temp);
	FskMemPtrDispose(preferencePath);
	FskMemPtrDispose(cachePath);
	return;
}

void KprHTTPClientStop(KprService service)
{
	KprHTTPClient self = gKprHTTPClient;
	KprHTTPConnection connection;	
	KprHTTPTarget target;	

	/* CONNECTIONS */
	while ((connection = self->connections)) {
		FskListRemove(&self->connections, connection);
		KprHTTPConnectionDispose(connection);
	}
	while ((target = self->targets)) {
		FskListRemove(&self->targets, target);
		// @@ cancel message?
	}
	
	/* KEYCHAIN */
	if (self->keychain) {
		KprHTTPKeychainWrite(self->keychain);
		KprHTTPKeychainDispose(self->keychain);
		self->keychain = NULL;
	}

	/* COOKIES */
	if (self->cookies) {
		KprHTTPCookiesCleanup(self->cookies, true);
		KprHTTPCookiesWrite(self->cookies);
		KprHTTPCookiesDispose(self->cookies);
		self->cookies = NULL;
	}

	/* HTTP CACHE */
	if (self->cache) {
		KprHTTPCacheWrite(self->cache);
		KprHTTPCacheDispose(self->cache);
		self->cache = NULL;
	}
	FskInstrumentedItemDispose(self);
	FskMemPtrDisposeAt(&gKprHTTPClient);
}

//--------------------------------------------------
// Cache
//--------------------------------------------------

KprHTTPCache KprHTTPClientCache()
{
	KprHTTPClient self = gKprHTTPClient;
	return self->cache;
}

#if 0
void KprHTTPClientCacheCleanup()
{
	KprHTTPClient self = gKprHTTPClient;
	KprHTTPCacheCleanup(self->cache);
}

void KprHTTPClientCacheClear()
{
	KprHTTPClient self = gKprHTTPClient;
	KprHTTPCacheClear(self->cache);
}
#endif

//--------------------------------------------------
// Cookies
//--------------------------------------------------

KprHTTPCookies KprHTTPClientCookies()
{
	KprHTTPClient self = gKprHTTPClient;
	return self->cookies;
}

#if 0
void KprHTTPClientCookiesClear()
{
	KprHTTPClient self = gKprHTTPClient;
	KprHTTPCookiesClear(self->cookies);
}
#endif

void KprHTTPClientCookiesCleanup(Boolean session)
{
	KprHTTPClient self = gKprHTTPClient;
	KprHTTPCookiesCleanup(self->cookies, session);
}

FskErr KprHTTPClientCookiesGet(char* url, char** it)
{
	KprHTTPClient self = gKprHTTPClient;
	return KprHTTPCookiesGet(self->cookies, url, it);
}

FskErr KprHTTPClientCookiesPut(char* url, char* cookies)
{
	KprHTTPClient self = gKprHTTPClient;
	return KprHTTPCookiesPut(self->cookies, url, cookies);
}

//--------------------------------------------------
// KeyChain
//--------------------------------------------------

KprHTTPKeychain KprHTTPClientKeychain()
{
	KprHTTPClient self = gKprHTTPClient;
	return self->keychain;
}

//--------------------------------------------------
// Machine
//--------------------------------------------------

xsMachine* KprHTTPClientMachine()
{
	KprHTTPClient self = gKprHTTPClient;
	return self->the;
}

//--------------------------------------------------
// Target
//--------------------------------------------------

FskErr KprHTTPClientAddTarget(KprHTTPTarget target)
{
	KprHTTPClient self = gKprHTTPClient;
	KprMessage message = target->message;
	FskErr err = kFskErrNone;
	SInt32 priority = abs(message->priority);
	KprHTTPTarget iterator, after = NULL;
	if (message->priority < 0) { // new requests with equal priority go to the start of that priority
		for (iterator = self->targets; iterator; iterator = FskListGetNext(self->targets, iterator)) {
			if (priority >= abs(iterator->message->priority))
				break;
			else
				after = iterator;
		}
	}
	else { // new requests with equal priority go to the end of that priority
		for (iterator = self->targets; iterator; iterator = FskListGetNext(self->targets, iterator)) {
			if (priority > abs(iterator->message->priority))
				break;
			else
				after = iterator;
		}
	}
	if (after)
		FskListInsertAfter(&self->targets, target, after);
	else
		FskListPrepend(&self->targets, target);
	KprHTTPClientIterate();
//bail:
	return err;
}

void KprHTTPClientCancel(KprService service UNUSED, KprMessage message UNUSED)
{
	KprHTTPClientIterate();
}

/*
void KprHTTPClientCancelTarget(KprHTTPTarget target)
{
	KprHTTPClient self = gKprHTTPClient;
	KprHTTPConnection connection;

	for (connection = FskListGetNext(self->connections, NULL); connection; connection = FskListGetNext(self->connections, connection)) {
		if (connection->target == target)
			break;
	}
	if (connection)
		KprHTTPConnectionClose(connection);
	else if (FskListContains(self->targets, target))
		KprHTTPClientRemoveTarget(target);
}
*/

void KprHTTPClientInvoke(KprService service UNUSED, KprMessage message)
{
	FskErr err = kFskErrNone;
	KprHTTPClient self = gKprHTTPClient;
	if (KprMessageContinue(message)) {
		KprHTTPTarget target = NULL;
		KprHTTPCacheValue cached = NULL;
		char* value;
		FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageHTTPBegin, message)
		bailIfError(KprHTTPTargetNew(&target, message));
		if (!message->method || !FskStrCompare(message->method, "GET"))
			KprHTTPCacheFetch(self->cache, message->url, &cached);
		if (cached) {
			if (KprHTTPCacheValueIsFresh(cached)) {
				if (KprHTTPClientReadCache(message, cached) == kFskErrNone) {
					KprMessageTransform(message, gHTTPService.machine);
					KprHTTPClientResumeTarget(target);
					return;
				}
			}
			else {
				value = FskAssociativeArrayElementGetString(cached->headers, kprHTTPHeaderLastModified);
				if (value)
					KprMessageSetRequestHeader(message, kprHTTPHeaderIfModifiedSince, value);
				value = FskAssociativeArrayElementGetString(cached->headers, kprHTTPHeaderETag);
				if (value)
					KprMessageSetRequestHeader(message, kprHTTPHeaderIfNoneMatch, value);
				target->cached = cached;
			}
		}
		KprHTTPClientCookiesGet(message->url, &value);
		if (value) {
			KprMessageSetRequestHeader(message, kprHTTPHeaderCookie, value);
			FskMemPtrDispose(value);
		}
		KprHTTPClientAddTarget(target);
		return;
	bail:
		message->error = err;
	}
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
}

void KprHTTPClientIterate()
{
	KprHTTPClient self = gKprHTTPClient;
	KprHTTPConnection connection, candidate, available;
	KprHTTPTarget target, next, other;
	KprMessage message;
	
	for (connection = FskListGetNext(self->connections, NULL); connection; connection = FskListGetNext(self->connections, connection)) {
		target = connection->target;
		if (target) {
			message = connection->target->message;
			if (!KprMessageContinue(message)) {
				KprHTTPConnectionClose(connection);
				KprHTTPClientResumeTarget(target);
			}
		}
	}
	target = FskListGetNext(self->targets, NULL);
	while (target) {
		next = FskListGetNext(self->targets, target);
		message = target->message;
		if (!KprMessageContinue(message)) {
			FskListRemove(&self->targets, target);
			KprHTTPClientResumeTarget(target);
		}
		target = next;
	}
	target = FskListGetNext(self->targets, NULL);
	while (target) {
		next = FskListGetNext(self->targets, target);
		message = target->message;
		candidate = NULL;
		available = NULL;
		for (connection = FskListGetNext(self->connections, NULL); connection; connection = FskListGetNext(self->connections, connection)) {
			if (KprHTTPConnectionCandidate(connection, message)) {
				candidate = connection;
				break;
			}
			else if (KprHTTPConnectionAvailable(connection)) {
				if (!available || (FskTimeCompare(&connection->time, &available->time) > 0)) // make the older connection the candidate for re-use
					available = connection;
			}
		}
		if (candidate) {
			if (KprHTTPConnectionAvailable(candidate)) {
				FskInstrumentedItemSendMessageDebug(candidate, kprInstrumentedHTTPConnectionCandidate, candidate->client);
			}
			else {
				candidate = NULL;
			}
		}
		else if (available) {
			// before closing the available connection, try to find a message that can use it
			for (other = FskListGetNext(self->targets, target); other; other = FskListGetNext(self->targets, other)) {
				if (abs(other->message->priority) < abs(message->priority)) {
					other = NULL;
					break;		// do not let a lower priority request get in ahead of a higher priority one
				}
				if (KprHTTPConnectionCandidate(available, other->message)) {
					break;
				}
			}
			if (other) {
				FskInstrumentedItemSendMessageDebug(available, kprInstrumentedHTTPConnectionCandidate, available->client);
				next = target;
				target = other;
			}
			else
				KprHTTPConnectionClose(available);
			candidate = available;
		}
		else
			break;
		if (candidate) {
			FskListRemove(&self->targets, target);
			if (KprHTTPConnectionProcess(candidate, target)) {
				// put back the message in the queue
				KprHTTPClientAddTarget(target);
			}
		}
		target = next;
	}
}

Boolean KprHTTPClientPutTargetCache(KprHTTPTarget target)
{
	FskErr err = kFskErrNone;
	char* control = NULL;
	KprHTTPClient self = gKprHTTPClient;
	KprMessage message = target->message;
	
	UInt32 date = 0;
	UInt32 maxAge = 0;
	UInt32 lifetime = 0;
	char* start;
	char* next;
	char* end;
	Boolean foundAge = false;
	Boolean revalidate = false;
	KprHTTPCacheValue cached = NULL;
	
	bailIfError(message->error
				|| !FskStrCompareCaseInsensitiveWithLength(message->url, "https://", 8)
				|| ((message->status != 200) && (message->status != 203))
				|| (message->method && FskStrCompare(message->method, "GET")) // only caching GET method
				|| (message->user != NULL) // do not cache request with credentials
				|| (message->password != NULL) // do not cache request with credentials
				|| (message->response.size == 0) // only caching something
				|| (message->response.size > 384000)); // not caching big files

	if ((control = KprMessageGetResponseHeader(message, kFskStrCacheControl))) {
		start = control;
		end = control + FskStrLen(control);
		// split
		while (start < end) {
			if (*start == ',') *start = 0;
			start++;
		}
		start = control;
		while (start <= end) {
			next = start + FskStrLen(start) + 1;
			start = FskStrStripHeadSpace(start);
			if (!FskStrCompareCaseInsensitiveWithLength(start, "max-age", 7)) {
				bailIfError(!(start = FskStrStripHeadSpace(start + 7)));
				bailIfError(*start != '=');
				bailIfError(!(start = FskStrStripHeadSpace(start + 1)));
				maxAge = FskStrToNum(start);
				foundAge = true;
			}
			else if (!FskStrCompareCaseInsensitiveWithLength(start, "no-cache", 8)) {
				revalidate = true;
			}
			else if (!FskStrCompareCaseInsensitiveWithLength(start, "no-store", 8)) {
				BAIL(kFskErrInvalidParameter);
			}
			else if (!FskStrCompareCaseInsensitiveWithLength(start, "must-revalidate", 15)) {
				revalidate = true;
			}
			start = next;
		}
	}
	KprDateFromHTTP(KprMessageGetResponseHeader(message, kprHTTPHeaderDate), &date);
	if (foundAge)
		lifetime = maxAge;
	else {
		UInt32 expire = 0;
		KprDateFromHTTP(KprMessageGetResponseHeader(message, kFskStrExpires), &expire);
		if (date && expire)
			lifetime = (expire > date) ? expire - date : 0;
		else if (!KprMessageGetResponseHeader(message, kprHTTPHeaderETag)
			&& !KprMessageGetResponseHeader(message, kprHTTPHeaderLastModified))
			BAIL(kFskErrInvalidParameter); // not cached
	}
	if (KprMessageGetResponseHeader(message, kprHTTPHeaderVary))
		revalidate = true;
	if (!revalidate)
		lifetime += 180; // guess 3 minutes
//	put in cache
//	fprintf(stderr, "%p: CACHE %s for %d (%d)\n", message, message->url, lifetime, revalidate);
	bailIfError(KprHTTPCacheValueNew(&cached));
	KprDateFromHTTP(KprMessageGetResponseHeader(message, kprHTTPHeaderAge), &cached->age);
	cached->date = date;
	cached->lifetime = lifetime;
	cached->requestDate = target->requestDate;
	cached->responseDate = target->responseDate;
	cached->status = message->status;
	
	cached->headers = FskAssociativeArrayNew();
	{
		FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(message->response.headers);
		while (iterate) {
			FskAssociativeArrayElementSet(cached->headers, iterate->name, iterate->value, 0, kFskStringType);
			iterate = FskAssociativeArrayIteratorNext(iterate);
		}
		FskAssociativeArrayIteratorDispose(iterate);
	}
	KprHTTPCacheValueCleanupHeaders(cached);
	
	cached->size = message->response.size;
	cached->body = message->response.body;
	target->cached = cached;
	bailIfError(KprHTTPCachePut(self->cache, message->url, cached));
	
bail:
	if (control) {
		// unsplit
		start = control;
		while (start < end) {
			if (*start == 0) *start = ',';
			start++;
		}
	}
	if (err == kFskErrNone) return true;
	target->cached = NULL;
	if (cached)
		KprMemPtrDispose(cached);
	return false;
}

FskErr KprHTTPClientReadCache(KprMessage message, KprHTTPCacheValue cached)
{
	FskErr err = kFskErrNone;
	FskAssociativeArrayIterator iterate = NULL;
	
	bailIfError(KprHTTPCacheValueReadData(cached));
	iterate = FskAssociativeArrayIteratorNew(cached->headers);
	while (iterate) {
		bailIfError(KprMessageSetResponseHeader(message, iterate->name, iterate->value));
		iterate = FskAssociativeArrayIteratorNext(iterate);
	}
	message->response.body = cached->body;
	cached->body = NULL;
	message->response.size = cached->size;
	message->status = cached->status;
bail:
	FskAssociativeArrayIteratorDispose(iterate);
	if (err)
		KprMemPtrDisposeAt(&cached->body);
	return err;
}

#if 0
void KprHTTPClientRemoveTarget(KprHTTPTarget target)
{
 	KprHTTPClient self = gKprHTTPClient;
 	FskListRemove(&self->targets, target);
 	KprHTTPClientIterate();
}
#endif

void KprHTTPClientRemoveTargetCache(KprHTTPTarget target)
{
	KprHTTPClient self = gKprHTTPClient;
	target->cached = NULL;
	KprHTTPCacheRemove(self->cache, target->message->url);
}

FskErr KprHTTPClientResetTarget(KprHTTPTarget target)
{
	FskErr err = kFskErrNone;
	KprMessage message = target->message;

	KprMessageClearRequestHeader(message, kprHTTPHeaderIfModifiedSince);
	KprMessageClearRequestHeader(message, kprHTTPHeaderIfNoneMatch);
	if (target->cached) {
		target->cached = NULL;
	}
	else {
		FskAssociativeArrayDispose(message->response.headers);
		KprMemPtrDispose(message->response.body);
	}
	message->error = 0;
	message->response.headers = NULL;
	message->response.body = NULL;
	message->response.size = 0;
	target->responseOffset = 0;
	FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageHTTPContinue, message);
//bail:
	return err;
}

void KprHTTPClientResumeTarget(KprHTTPTarget target)
{
	KprMessage message = target->message;
	FskInstrumentedItemSendMessageDebug(target->message, kprInstrumentedMessageHTTPEnd, message)
	KprHTTPTargetDispose(target);
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
}

//--------------------------------------------------
/* HTTP CONNECTION */
//--------------------------------------------------

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprHTTPConnectionInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprHTTPConnection", FskInstrumentationOffset(KprHTTPConnectionRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprHTTPConnectionNew(KprHTTPConnection* it)
{
	FskErr err = kFskErrNone;
	KprHTTPConnection self = NULL;
	bailIfError(FskMemPtrNewClear(sizeof(KprHTTPConnectionRecord), &self));
	*it = self;
	FskInstrumentedItemNew(self, NULL, &KprHTTPConnectionInstrumentation);
bail:
	return err;
}

void KprHTTPConnectionDispose(KprHTTPConnection self)
{
	if (self->client) {
		KprHTTPConnectionClose(self);
	}
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

Boolean KprHTTPConnectionAvailable(KprHTTPConnection self)
{
	if (self->client)
		return !self->client->httpRequests && !self->client->httpResponses;
	return true;
}

Boolean KprHTTPConnectionCandidate(KprHTTPConnection self, KprMessage message)
{
	Boolean result = false;
	if (!self->client)
		result = true;
	else if (!self->client->host)
		result = true;
	else {
		UInt32 hostLength = FskStrLen(self->client->host);
		result = (FskStrCompareCaseInsensitiveWithLength(self->client->host, message->parts.host, (hostLength > message->parts.hostLength) ? hostLength : message->parts.hostLength) == 0)
			&& (self->client->hostPort == (int)message->parts.port) 
			&& (self->client->priority == abs(message->priority));
		if (result && self->target) // only serialize if the message target is the same
			result = self->target->message->request.target == message->request.target;
	}
	return result;
}

void KprHTTPConnectionClose(KprHTTPConnection self)
{
	FskInstrumentedItemSendMessageVerbose(self, kprInstrumentedHTTPConnectionClose, self->client)
	FskHTTPClientDispose(self->client);
	self->client = NULL;
	self->target = NULL;
}

FskErr KprHTTPConnectionProcess(KprHTTPConnection self, KprHTTPTarget target)
{
	FskErr err = kFskErrNone;
	KprMessage message = target->message;
	FskHTTPClientRequest request = NULL;
	KprStream stream = message->stream;
	
	if (!self->client) {
		bailIfError(FskHTTPClientNewPrioritized(&(self->client), message->priority, "Kpr"));
		FskHTTPClientSetRefCon(self->client, self);
		FskHTTPClientSetAuthCallback(self->client, KprHTTPConnectionAuthCallback);
		FskHTTPClientSetFinishedCallback(self->client, KprHTTPConnectionClosedCallback);
		FskHTTPClientSetCredentials(self->client, NULL, NULL, 0, kFskHTTPAuthCredentialsTypeNone);
		FskHTTPClientSetCertificates(self->client, message->request.certificate, message->request.certificateSize, message->request.policies ? message->request.policies : "allowOrphan");
		FskInstrumentedItemSendMessageVerbose(self, kprInstrumentedHTTPConnectionOpen, self->client)
	}
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedHTTPConnectionProcess, message->url)
	/*if (message->user && message->password)
		FskHTTPClientSetCredentials(self->client, message->user, message->password, 0, kFskHTTPAuthCredentialsTypeString);
	else
		FskHTTPClientSetCredentials(self->client, NULL, NULL, 0, kFskHTTPAuthCredentialsTypeNone);*/
	FskTimeGetNow(&self->time);
	self->target = target;
	bailIfError(FskHTTPClientRequestNew(&request, message->url));
	
	if (stream && stream->dispatch->reconnectable)
		request->eligibleForReconnect = (*stream->dispatch->reconnectable)(stream, message);
	if (message->method)
		FskHTTPClientRequestSetMethod(request, message->method);
	// copy request headers
	{
		FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(message->request.headers);
		FskInstrumentedItemPrintfVerbose(self, "request  %s message %p %s", message->method ? message->method : "GET", message, message->url);
		while (iterate) {
			FskInstrumentedItemPrintfDebug(self, "   %s: %s", iterate->name, iterate->value);
			bailIfError(FskHTTPClientRequestRemoveHeader(request, iterate->name));
			bailIfError(FskHTTPClientRequestAddHeader(request, iterate->name, iterate->value));
			iterate = FskAssociativeArrayIteratorNext(iterate);
		}
		FskAssociativeArrayIteratorDispose(iterate);
	}
	FskHTTPClientRequestSetRefCon(request, self);
	if (message->request.body) {
		if (message->request.size
			&& !FskHeaderFind(kFskStrContentLength, request->requestHeaders)
			&& !FskHeaderFind(kFskStrTransferEncoding, request->requestHeaders))
			FskHeaderAddInteger(kFskStrContentLength, message->request.size, request->requestHeaders);
		FskHTTPClientRequestSetSendDataCallback(request, KprHTTPConnectionSendingCallback);
	}
	FskHTTPClientRequestSetReceivedResponseHeadersCallback(request, KprHTTPConnectionHeadersCallback, kHTTPClientResponseHeadersOnRedirect);
	bailIfError(FskHTTPClientRequestSetReceivedDataCallback(request, KprHTTPConnectionReceivingCallback, NULL, kprHTTPMessageDefaultSize, kFskHTTPClientReadAnyData));
	FskHTTPClientRequestSetFinishedCallback(request, KprHTTPConnectionCompleteCallback);
	target->requestDate = KprDateNow();
	if (message->timeout)
		FskHTTPClientSetIdleTimeout(self->client, message->timeout);
	else
		FskHTTPClientSetIdleTimeout(self->client, gKprHTTPClient->connectionTimeout);
	bailIfError(FskHTTPClientBeginRequest(self->client, request));
	return err;
bail:
	return err;
}

FskErr KprHTTPConnectionSendingCallback(FskHTTPClientRequest request UNUSED, char** data, int* size, FskInt64 offset, void* context)
{
	FskErr err = kFskErrNone;
	KprHTTPConnection self = context;
	KprHTTPTarget target = self->target;
	KprMessage message = target->message;
	KprStream stream = message->stream;
	if (stream && stream->dispatch->send)
		return (*stream->dispatch->send)(stream, message, gHTTPService.machine, data, size);
	if (offset == 0) {
		*data = message->request.body;
		*size = message->request.size;
		target->requestOffset = *size;
	}
	else {
		*size = 0;
		err = kFskErrEndOfFile;
	}
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedHTTPConnectionSending, size);
	return err;
}

FskErr KprHTTPConnectionAuthCallback(FskHTTPClient client UNUSED, FskHTTPClientRequest request, char *url UNUSED, char *realm, FskHTTPAuth auth UNUSED, void *context)
{
	FskErr err = kFskErrNone;
	KprHTTPConnection self = context;
	KprHTTPKeychain keychain = KprHTTPClientKeychain();
	char* user = NULL;
	char* password = NULL;
	KprHTTPKeychainGet(keychain, request->parsedUrl->host, realm, &user, &password);
	if (user && password)
		FskHTTPClientSetCredentials(self->client, user, password, 0, kFskHTTPAuthCredentialsTypeString);
	else
		err = kFskErrAuthFailed;
	FskMemPtrDispose(password);
	FskMemPtrDispose(user);
	return err;
}

FskErr KprHTTPConnectionHeadersCallback(FskHTTPClientRequest request UNUSED, FskHeaders* responseHeaders UNUSED, void* context)
{
	FskErr err = kFskErrNone;
	KprHTTPConnection self = context;
	KprHTTPTarget target = self->target;
	KprMessage message = target->message;
	message->status = FskHeaderResponseCode(request->responseHeaders);
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedHTTPConnectionHeaders, self);
	// copy response headers
	{
		FskHeaderIterator iterate = FskHeaderIteratorNew(request->responseHeaders);
		FskInstrumentedItemPrintfVerbose(self, "response %s message %p %s", message->method ? message->method : "GET", message, message->url);
		while (iterate) {
			FskInstrumentedItemPrintfDebug(self, "   %s: %s", iterate->name, iterate->value);
			KprMessageSetResponseHeader(message, iterate->name, iterate->value);
			iterate = FskHeaderIteratorNext(iterate);
		}
		FskHeaderIteratorDispose(iterate);
		// DLNA CTT 7.3.99.8: empty NOTIFY response with improper response code
		if ((responseHeaders->responseCode == 200) && (!message->response.headers) && message->method && !FskStrCompareCaseInsensitive(message->method, "NOTIFY"))
			responseHeaders->responseCode = 204;
	}
	KprHTTPClientCookiesPut(message->url, FskHeaderFind(kprHTTPHeaderSetCookie, request->responseHeaders));
	KprHTTPClientCookiesCleanup(false);
//bail:
	return err;
}

FskErr KprHTTPConnectionReceivingCallback(FskHTTPClientRequest request UNUSED, char* data, int size, void* context)
{
	FskErr err = kFskErrNone;
	KprHTTPConnection self = context;
	KprHTTPTarget target = self->target;
	KprMessage message = target->message;
	KprStream stream = message->stream;
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedHTTPConnectionReceiving, &size);
	if (stream && stream->dispatch->receive)
		return (*stream->dispatch->receive)(stream, message, gHTTPService.machine, data, size);
	if (!message->response.body) {
		char* value;
		FskInt64 contentSize;
		message->response.size = 1024;
		value = KprMessageGetResponseHeader(message, kFskStrContentLength);
		if (value) {
			contentSize = FskStrToFskInt64(value);
			if (contentSize < gKprHTTPClient->contentLength)
				message->response.size = contentSize;
		}
		if (message->response.size < (UInt32)size)
			message->response.size = (UInt32)size;
		bailIfError(FskMemPtrNew(message->response.size, &message->response.body));
	}
	if (message->response.size < target->responseOffset + size) {
		message->response.size = target->responseOffset + size;
		bailIfError(FskMemPtrRealloc(message->response.size, &message->response.body));
	}
	FskMemCopy((UInt8*)message->response.body + target->responseOffset, data, size);
	target->responseOffset += size;
bail:
	return err;
}

FskErr KprHTTPConnectionCompleteCallback(FskHTTPClient client UNUSED, FskHTTPClientRequest request UNUSED, void* context)
{
	FskErr err = kFskErrNone;
	KprHTTPConnection self = context;
	KprHTTPTarget target = self->target;
	KprMessage message = target->message;
	char* url = NULL;
	char* header = NULL;

	FskInstrumentedItemPrintfVerbose(self, "complete %s message %p %s", message->method ? message->method : "GET", message, message->url);
	message->response.size = target->responseOffset;
	message->error = self->client->status.lastErr;

	if (message->error == kFskErrConnectionDropped) {
		FskInstrumentedItemPrintfVerbose(self, "dropped");
		KprHTTPConnectionClose(self);
		KprHTTPClientResetTarget(target);
		if (KprHTTPConnectionProcess(self, target)) {
			// put back the message in the queue
			KprHTTPClientAddTarget(target);
		}
		return err;
	}
	header = KprMessageGetRequestHeader(message, kFskStrConnection);
	if (header && !FskStrCompareCaseInsensitive(header, kFskStrClose)) {
		FskInstrumentedItemPrintfVerbose(self, "request connection close");
		KprHTTPConnectionClose(self);
	}
	else {
		header = KprMessageGetResponseHeader(message, kFskStrProxyConnection);
		if (!header)
			header = KprMessageGetResponseHeader(message, kFskStrConnection);
		if (header && !FskStrCompareCaseInsensitive(header, kFskStrClose)) {
			FskInstrumentedItemPrintfVerbose(self, "response connection close");
			KprHTTPConnectionClose(self);
		}
	}

	self->target = NULL;
	target->responseDate = KprDateNow();
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedHTTPConnectionComplete, message);
	if (!message->error) {
		if (target->cached) {
			if (message->status == 304) {
				// cache revalidated
				KprHTTPCacheValueUpdateHeaders(target->cached, message->response.headers);
				target->cached->requestDate = target->requestDate;
				target->cached->responseDate = target->responseDate;
				FskAssociativeArrayDispose(message->response.headers);
				message->response.headers = NULL;
				if (KprHTTPClientReadCache(message, target->cached) != kFskErrNone) {
					// retry without cache
					KprHTTPClientRemoveTargetCache(target);
					KprHTTPClientResetTarget(target);
					KprHTTPClientAddTarget(target);
					return err;
				}
			}
			else {
				// cache miss, so remove the cache item before proceeding
				KprHTTPClientRemoveTargetCache(target);
			}
		}
		switch (message->status) {
		case 301:
		case 302:
		case 303:
		case 305:
		case 307:
			target->redirects += 1;
			if (target->redirects > 10)
				message->error = kFskErrTooManyRedirects;
			else {
				char* location = KprMessageGetResponseHeader(message, "Location");
				if (!location) {
					message->error = kFskErrRequestAborted;
					goto bail;
				}
				bailIfError(KprURLMerge(message->url, location, &url));
				KprMemPtrDisposeAt(&message->url);
				message->url = url;
				KprURLSplit(message->url, &message->parts);
				KprHTTPClientResetTarget(target);
				KprHTTPClientAddTarget(target);
				return err;
			}
			break;
		case 504: // treat Gateway Timeout like Redirect, retry 10 times
			target->redirects += 1;
			if (target->redirects > 10)
				message->error = kFskErrTooManyRedirects;
			else {
				KprHTTPClientResetTarget(target);
				KprHTTPClientAddTarget(target);
				return err;
			}
			break;
		}
		if (!target->cached)
			KprHTTPClientPutTargetCache(target);
	}
bail:
	KprMessageTransform(message, gHTTPService.machine);
	KprHTTPClientResumeTarget(target);
	if (!message->error)
		KprHTTPClientIterate();
	return err;
}

FskErr KprHTTPConnectionClosedCallback(FskHTTPClient client, void* context)
{
	FskErr err = kFskErrNone;
	KprHTTPConnection self = context;
	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedHTTPConnectionFinished, client)
	if (self->client) {
		bailIfError(self->client != client);
		if (!client->skt // socket was closed
			|| (client->status.lastErr == kFskErrTimedOut)
			|| (client->status.lastErr == kFskErrNetworkInterfaceRemoved)
			|| (client->status.lastErr == kFskErrConnectionDropped)
			|| (client->status.lastErr == kFskErrConnectionClosed)) {
			KprHTTPConnectionClose(self);
		}
	}
	KprHTTPClientIterate();
bail:
	return err;
}

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprHTTPTargetInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprHTTPTarget", FskInstrumentationOffset(KprHTTPTargetRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprHTTPTargetNew(KprHTTPTarget* it, KprMessage message)
{
	KprHTTPTarget self;
	FskErr err = kFskErrNone;
	bailIfError(FskMemPtrNewClear(sizeof(KprHTTPTargetRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprHTTPTargetInstrumentation);
	FskInstrumentedItemSetOwner(self, message);
	self->message = message;
	self->thread = FskThreadGetCurrent();
bail:
	return err;
}

void KprHTTPTargetDispose(void* it)
{
	KprHTTPTarget self = it;
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

