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
#define _XOPEN_SOURCE
#define __FSKHTTPSERVER_PRIV__
#include "FskMediaPlayer.h"
#include "FskThread.h"

#include "kprHTTPClient.h"
#include "kprMessage.h"
#include "kprUtilities.h"

#include "kprLibrary.h"
#include "kprLibraryServer.h"

static FskErr KprLibraryServerRequestConditionCallback(FskHTTPServerRequest request, UInt32 condition, void *refCon);
static FskErr KprLibraryServerRequestBodyCallback(FskHTTPServerRequest request UNUSED, char *data, UInt32 size, UInt32 *consumed, void *refCon);
static FskErr KprLibraryServerResponseBodyCallback(FskHTTPServerRequest request UNUSED, char *data, UInt32 size, UInt32 *generated, void *refCon);
static FskErr KprLibrarySessionNew(KprLibrarySession* it, FskHTTPServerRequest request);
static void KprLibrarySessionDispose(KprLibrarySession self);
static void KprLibrarySessionRedirectCallback(KprMessage message, void* it);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprLibraryQueryInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprLibraryQuery", FskInstrumentationOffset(KprLibraryQueryRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
static FskInstrumentedTypeRecord KprLibraryServerInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprLibraryServer", FskInstrumentationOffset(KprLibraryServerRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
static FskInstrumentedTypeRecord KprLibrarySessionInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprLibrarySession", FskInstrumentationOffset(KprLibrarySessionRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

static KprLibraryServer gLibraryServer = NULL;

static FskErr KprLibraryQueryNew(KprLibraryQuery* it)
{
	FskErr err = kFskErrNone;
	bailIfError(FskMemPtrNewClear(sizeof(KprLibraryQueryRecord), it));
	FskInstrumentedItemNew((KprLibraryQuery)*it, NULL, &KprLibraryQueryInstrumentation);
bail:
	return err;
}

static void KprLibraryQueryDispose(void* it)
{
	KprLibraryQuery self = it;

	FskMemPtrDispose(self->info);
	FskMemPtrDispose(self->mime);
	FskMemPtrDispose(self->url);
	FskMemPtrDispose(self->authorization);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

FskErr KprLibraryServerNew(KprLibraryServer* it)
{
	FskErr err = kFskErrNone;
	KprLibraryServer self = NULL;
	FskTimeRecord now;
	
	bailIfError(KprMemPtrNewClear(sizeof(KprLibraryServerRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprLibraryServerInstrumentation);

	self->vectors.requestCondition = KprLibraryServerRequestConditionCallback;
	self->vectors.requestReceiveRequest = KprLibraryServerRequestBodyCallback;
	self->vectors.requestGenerateResponseBody = KprLibraryServerResponseBodyCallback;
    
    FskMutexNew(&self->queryMutex, "query");
	FskTimeGetNow(&now);
	self->queryIndex = FskTimeInSecs(&now);
	gLibraryServer = self;
bail:
	return err;
}

void KprLibraryServerDispose(KprLibraryServer self)
{
	if (self) {
		KprLibraryServerStop(self);
   		FskMutexDispose(self->queryMutex);
		FskInstrumentedItemDispose(self);
		KprMemPtrDispose(self);
		gLibraryServer = NULL;
	}
}

FskErr KprLibraryServerRequestConditionCallback(FskHTTPServerRequest request, UInt32 condition, void *refCon)
{
	FskErr err = kFskErrNone;
	KprLibraryServer self = request->http->refCon;
	KprLibrarySession session = refCon;
	
	switch (condition) {
	case kFskHTTPConditionConnectionInitialized:
	case kFskHTTPConditionNoSocket:
	case kFskHTTPConditionRequestReceivedRequestHeaders:
		break;

	case kFskHTTPConditionRequestRequestFinished: { 
		FskHeaders *headers;
		UInt32 queryIndex; 
		KprLibraryQuery query;
		
		bailIfError(KprLibrarySessionNew(&session, request));
		FskInstrumentedItemSetOwner(session, self);

		headers = FskHTTPServerRequestGetRequestHeaders(request);
		queryIndex = FskStrToNum(headers->filename);
		FskMutexAcquire(self->queryMutex);
		query = self->queries[queryIndex % kQueryCount];
		if (query && (query->index == queryIndex)) {
			session->info = FskStrDoCopy(query->info);
			session->kind = query->kind;
			session->mime = FskStrDoCopy(query->mime);
 			session->url = FskStrDoCopy(query->url);
 			if (query->authorization)
 				session->authorization = FskStrDoCopy(query->authorization);
 		}
 		else
			err = kFskErrNotFound;
   		FskMutexRelease(self->queryMutex);
		
		if (kFskErrNone == err) {
			KprURLSplit(session->url, &session->parts);
			if (0 == FskStrCompareWithLength(session->url, "file", 4))
				err = KprFileServerOpen(session);
			else if (0 == FskStrCompareWithLength(session->url, "http", 4)) {
				session->http.location = FskStrDoCopy(session->url);
				err = KprProxyServerOpen(session);
			}
			else
				err = KprDataServerOpen(session);
		}
		if (kFskErrNeedMoreTime == err)
			FskHTTPServerRequestSuspend(request);
		else
			session->error = err;

		err = kFskErrNone;
	}
	break;

	case kFskHTTPConditionRequestGenerateResponseHeaders:
		if (session) {
			FskHeaders *responseHeaders = FskHTTPServerRequestGetResponseHeaders(request);
			if (session->error) {
				if (kFskErrNotFound == session->error)
					responseHeaders->responseCode = 404;
				else
					responseHeaders->responseCode = 500;
				FskHeaderAddString(kFskStrContentLength, "0", responseHeaders);
				FskInstrumentedItemPrintfNormal(session, "response headers error %ld", session->error);
			}
			else if (session->file.file)
				KprFileServerGenerateResponseHeaders(session, responseHeaders);
			else if (session->http.client)
				KprProxyServerGenerateResponseHeaders(session, responseHeaders);
			else 
				KprDataServerGenerateResponseHeaders(session, responseHeaders);
		} 
		break;

	case kFskHTTPConditionConnectionTerminating:
         err = kFskErrUnimplemented;     //@@ or hang on exit... weird.
	case kFskHTTPConditionRequestResponseFinished:
	case kFskHTTPConditionRequestErrorAbort:
		if (session)
			KprLibrarySessionDispose(session);
		break;

	default:
		err = kFskErrUnimplemented;
		break;
	}

bail:
	return err;
}

FskErr KprLibraryServerRequestBodyCallback(FskHTTPServerRequest request UNUSED, char *data, UInt32 size, UInt32 *consumed, void *refCon)
{
    return kFskErrNone;
}

FskErr KprLibraryServerResponseBodyCallback(FskHTTPServerRequest request, char *data, UInt32 size, UInt32 *generated, void *refCon)
{
	FskErr err = kFskErrNone;
	KprLibrarySession session = refCon;
	if (session) {
		if (session->file.file)
			err = KprFileServerGenerateResponseBody(session, data, size, generated);
		else if (session->http.client)
			err = KprProxyServerGenerateResponseBody(session, data, size, generated);
		else
			err = KprDataServerGenerateResponseBody(session, data, size, generated);
	}
	return err;
}

FskErr KprLibraryServerStart(KprLibraryServer self, char* value)
{
	FskErr err = kFskErrNone;
	SInt32 port = 8080;
	if (self->server)
		return err;
	if (value)
		port = FskStrToNum(value);
	bailIfError(FskHTTPServerCreate(port, NULL, &self->server, self, false));
	bailIfError(FskHTTPServerSetCallbacks(self->server, &self->vectors));
    FskHTTPServerStart(self->server);
bail:
    return err;
}

FskErr KprLibraryServerStop(KprLibraryServer self)
{
	FskErr err = kFskErrNone;
	if (!self->server)
		return err;
	FskHTTPServerStop(self->server, true);
	FskHTTPServerDispose(self->server);
	self->server = NULL;
    return err;
}

FskErr KprLibrarySessionNew(KprLibrarySession* it, FskHTTPServerRequest request)
{
	FskErr err = kFskErrNone;
	KprLibrarySession self = NULL;
	
	bailIfError(KprMemPtrNewClear(sizeof(KprLibrarySessionRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprLibrarySessionInstrumentation);
    FskHTTPServerRequestSetRefcon(request, self);
	self->request = request;
bail:
	return err;
}

void KprLibrarySessionDispose(KprLibrarySession self)
{
	if (self) {
		if (self->file.file)
			KprFileServerClose(self);
		else if (self->http.client)
			KprProxyServerClose(self);
		else
			KprDataServerClose(self);
		FskMemPtrDispose(self->authorization);
		FskMemPtrDispose(self->url);
		FskMemPtrDispose(self->mime);
		FskMemPtrDispose(self->info);
		FskHTTPServerRequestSetRefcon(self->request, NULL);
		FskInstrumentedItemDispose(self);
		KprMemPtrDispose(self);
	}
}

FskErr KprLibrarySessionRedirect(KprLibrarySession session)
{
	FskErr err = kFskErrNone;
	KprMessage message;
	bailIfError(KprMessageNew(&message, session->url));
	KprMessageInvoke(message, KprLibrarySessionRedirectCallback, NULL, session->request);
	err = kFskErrNeedMoreTime;
bail:
	return err;
}

void KprLibrarySessionRedirectCallback(KprMessage message, void* it)
{
	FskHTTPServerRequest request = it;
	KprLibrarySession session = FskHTTPServerRequestGetRefcon(request);
	session->data.buffer = message->response.body;
	session->data.size = message->response.size;
	message->response.body = NULL;
	message->response.size = 0;
	FskThreadPostCallback(request->http->owner, FskHTTPServerRequestResume, request, NULL, NULL, NULL);
}

FskErr KprFunctionTargetNew(KprFunctionTarget* it, xsMachine* the, xsSlot* slot)
{
	KprFunctionTarget self;
	FskErr err = kFskErrNone;
	bailIfError(FskMemPtrNewClear(sizeof(KprFunctionTargetRecord), it));
	self = *it;
	self->the = the;
	self->slot = *slot;
	self->code = the->code;
	fxRemember(self->the, &self->slot);
bail:
	return err;
}

void KprFunctionTargetDispose(void* it)
{
	KprFunctionTarget self = it;
	fxForget(self->the, &self->slot);
	FskMemPtrDispose(self);
}

static void KPRLibrarySniffPodcastComplete(KprMessage message, void* it)
{
	KprFunctionTarget self = it;
	char* mime = KprMessageGetResponseHeader(message, "content-type");
	char* sniff = NULL;
	if (message->response.body) {
		if (kFskErrNone == FskMediaPlayerSniffForMIME(message->response.body, message->response.size, NULL, message->url, &sniff)) {
			mime = sniff;
		}
	}
	{
		xsBeginHostSandboxCode(self->the, self->code);
		if (mime)
			xsResult = xsString(mime);
        else
			xsResult = xsString("");
		(void)xsCallFunction1(self->slot, xsGlobal, xsResult);
		xsEndHostSandboxCode();
	}
	FskMemPtrDispose(sniff);
}

void KPRLibrarySniffPodcast(xsMachine *the)
{
	KprMessage message;
	KprFunctionTarget target;
	xsThrowIfFskErr(KprMessageNew(&message, xsToString(xsArg(1))));
	xsThrowIfFskErr(KprMessageSetRequestHeader(message, "Range", "bytes=0-4095"));
	xsThrowIfFskErr(KprFunctionTargetNew(&target, the, &xsArg(2)));
	xsThrowIfFskErr(KprMessageInvoke(message, KPRLibrarySniffPodcastComplete, KprFunctionTargetDispose, target));
}

void Library_cacheQuery(xsMachine* the)
{
	KprLibraryServer self = gLibraryServer;
	UInt32 index; 
	KprLibraryQuery query;
    FskMutexAcquire(self->queryMutex);
	index = self->queryIndex % kQueryCount;
    query = self->queries[index];
    if (query) {
    	KprLibraryQueryDispose(query);
    	self->queries[index] = NULL;
    }
	KprLibraryQueryNew(&query);
	FskInstrumentedItemSetOwner(query, self);
	xsEnterSandbox();
	query->index = self->queryIndex;
	query->info = FskStrDoCopy(xsToString(xsGet(xsArg(0), xsID("info"))));
	query->kind = xsToInteger(xsGet(xsArg(0), xsID("kind")));
	query->mime = FskStrDoCopy(xsToString(xsGet(xsArg(0), xsID("mime"))));
	query->url = FskStrDoCopy(xsToString(xsGet(xsArg(0), xsID("url"))));
	xsResult = xsGet(xsArg(0), xsID("authorization"));
	if (xsTest(xsResult))
		query->authorization = FskStrDoCopy(xsToString(xsResult));
	xsLeaveSandbox();
    self->queries[index] = query;
	xsResult = xsInteger(self->queryIndex);
    self->queryIndex++;
    FskMutexRelease(self->queryMutex);
}

void Library_getURI(xsMachine* the)
{
	KprLibraryServer self = gLibraryServer;
	UInt32 queryIndex; 
	UInt32 index; 
	KprLibraryQuery query;
    FskMutexAcquire(self->queryMutex);
    queryIndex = xsToInteger(xsArg(0));
    index = queryIndex % kQueryCount;
    query = self->queries[index];
	if (query && (query->index == queryIndex))
		xsResult = xsString(query->url);
	else
		xsResult = xsNull;
    FskMutexRelease(self->queryMutex);
}


