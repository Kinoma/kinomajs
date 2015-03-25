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
#include "kprLibraryServer.h"

#include "kprHTTPClient.h"
#include "kprMessage.h"
#include "kprUtilities.h"

// FskErr KprDataServerOpen(KprLibrarySession session)
// platform specific

void KprDataServerClose(KprLibrarySession session)
{
	FskInstrumentedItemPrintfNormal(session, "close data");
	FskMemPtrDispose(session->data.buffer);
}

void KprDataServerGenerateResponseHeaders(KprLibrarySession session, FskHeaders *responseHeaders)
{
	char buffer[128];
	responseHeaders->responseCode = 200;
	sprintf(buffer, "%ld", session->data.size);
	FskHeaderAddString(kFskStrContentLength, buffer, responseHeaders);
	FskHeaderAddString(kFskStrContentType, session->mime, responseHeaders);
	FskHeaderAddString("ContentFeatures.dlna.org", session->info, responseHeaders);
	FskHeaderAddString("transfermode.DLNA.ORG", "Interactive", responseHeaders);
	FskInstrumentedItemPrintfNormal(session, "response headers data %ld", session->data.size);
}

FskErr KprDataServerGenerateResponseBody(KprLibrarySession session, char *data, UInt32 size, UInt32 *generated)
{
	FskErr err = kFskErrNone;
	UInt32 messageSize = session->data.size;
	UInt32 offset = session->data.offset;
	if (messageSize - offset <= size) {
		size = messageSize - offset;
	}
	if (size) {
		FskMemCopy(data, (char *)session->data.buffer + offset, size);
		session->data.offset += size;
	}
	*generated = size;	
	if ((UInt32)session->data.offset == messageSize)
		err = kFskErrEndOfFile;
    FskInstrumentedItemPrintfDebug(session, "response body data %ld (%d)", *generated, err);
	return err;
}

FskErr KprFileServerOpen(KprLibrarySession session)
{
	FskErr err = kFskErrNone;
	session->file.path = FskStrDoCopy(session->parts.path);
	bailIfError(FskFileGetFileInfo(session->file.path, &session->file.info));
	if (kFskDirectoryItemIsFile != session->file.info.filetype) {
		err = kFskErrNotFound;
		goto bail;
	}
	bailIfError(FskFileOpen(session->file.path, kFskFilePermissionReadOnly, &session->file.file));
bail:
	FskInstrumentedItemPrintfNormal(session, "open %s (%d)", session->url, err);
	return err;
}

void KprFileServerClose(KprLibrarySession session)
{
	FskInstrumentedItemPrintfNormal(session, "close file");
	FskFileClose(session->file.file);
	FskMemPtrDispose(session->file.path);
}

void KprFileServerGenerateResponseHeaders(KprLibrarySession session, FskHeaders *responseHeaders)
{
	FskHeaders *headers = FskHTTPServerRequestGetRequestHeaders(session->request);
	FskInt64 length = session->file.info.filesize;
	FskInt64 from = 0;
	FskInt64 to = length - 1;
	char* range = FskHeaderFind("Range", headers);
	char buffer[128];
	if (range) {
		from = -1;
		if ((FskStrStr(range, "bytes=") == range) && !FskStrChr(range, ',')) {
			char* dash;
			range += 6;
			dash = FskStrChr(range, '-');
			if (dash == range) {
				from = 0;
			}
			else {
				*dash= 0;
				from = FskStrToNum(range);
			}
			range = dash + 1;
			if (*range)
				to = FskStrToNum(range);
			else
				to = length - 1;
			if ((from >= 0) && (from <= to) && (to < length)) {
				length = to - from + 1;
				responseHeaders->responseCode = 206;
				if (from)
					FskFileSetPosition(session->file.file, &from);
			}
			else
				responseHeaders->responseCode = 416;
		}
		else
			responseHeaders->responseCode = 416;
	}
	else
		responseHeaders->responseCode = 200;
	session->file.from = from;
	session->file.to = to;
	if (responseHeaders->responseCode / 100 == 2) {
		sprintf(buffer, "%lld", length);
		FskHeaderAddString(kFskStrContentLength, buffer, responseHeaders);
	}
	if (range) {
		if (responseHeaders->responseCode == 206) {
			sprintf(buffer, "bytes %lld-%lld/%lld", from, to, session->file.info.filesize);
			FskHeaderAddString(kprHTTPHeaderContentRange, buffer, responseHeaders);
		}
		else if (responseHeaders->responseCode == 416) {
			sprintf(buffer, "bytes */%lld", session->file.info.filesize);
			FskHeaderAddString(kprHTTPHeaderContentRange, buffer, responseHeaders);
		}
	}
	FskHeaderAddString("Accept-Ranges", "bytes", responseHeaders);
	FskHeaderAddString(kFskStrContentType, session->mime, responseHeaders);
	FskHeaderAddString("ContentFeatures.dlna.org", session->info, responseHeaders);
	FskHeaderAddString("transfermode.DLNA.ORG", (0 == FskStrCompareWithLength(session->mime, "image", 5)) ? "Interactive" : "Streaming", responseHeaders);
	FskInstrumentedItemPrintfNormal(session, "response headers file %lld-%lld", session->file.from, session->file.to);
}

FskErr KprFileServerGenerateResponseBody(KprLibrarySession session, char *data, UInt32 size, UInt32 *generated)
{
	FskErr err = kFskErrNone;
	UInt32 amt = 0;
	if (size > session->file.to - session->file.from + 1)
		size = session->file.to - session->file.from + 1;
	if (FskFileRead(session->file.file, size, data, &amt) == kFskErrNone) {
		*generated = amt;
		session->file.from += amt;
		if (amt == 0)
			err = kFskErrEndOfFile;
	}
	else {
		*generated = 0;
		err = kFskErrEndOfFile;
	}
    FskInstrumentedItemPrintfDebug(session, "response body file %ld (%d)", *generated, err);
	return err;
}

FskInstrumentedSimpleType(KprProxyServer, KprProxyServer);

static FskErr httpClientFinished(FskHTTPClient client, void *refCon);
static FskErr httpRequestHeadersReceived(FskHTTPClientRequest request, FskHeaders *responseHeaders, void *refCon);
static FskErr httpRequestDataReceived(FskHTTPClientRequest request, char *buffer, int bufferSize, void *refCon);
static FskErr httpRequestFinished(FskHTTPClient client, FskHTTPClientRequest request, void *refCon);

FskErr KprProxyServerOpen(KprLibrarySession session)
{
	FskErr err = kFskErrNone;
	FskHeaders *headers;
	char* value;

	session->http.bufferSize = 16 * 32 * 1024;
	session->http.readOffset = 0;
	session->http.writeOffset = 0;
	session->http.done = false;
	bailIfError(FskMemPtrNew(session->http.bufferSize, &session->http.buffer));
	
	session->http.responseCode = 500;

	bailIfError(FskHTTPClientNew(&session->http.client, "proxy"));
	FskHTTPClientSetRefCon(session->http.client, session);
	FskHTTPClientSetIdleTimeout(session->http.client, 60);			// after 60 seconds, drop the connection
	FskHTTPClientSetFinishedCallback(session->http.client, httpClientFinished);

	bailIfError(FskHTTPClientRequestNew(&session->http.request, session->http.location));
	FskHTTPClientRequestSetRefCon(session->http.request, session);
	FskHTTPClientRequestSetReceivedResponseHeadersCallback(session->http.request, httpRequestHeadersReceived, kHTTPClientResponseHeadersOnRedirect);
	FskHTTPClientRequestSetReceivedDataCallback(session->http.request, httpRequestDataReceived, NULL, 32 * 1024, kFskHTTPClientReadAnyData);
	FskHTTPClientRequestSetFinishedCallback(session->http.request, httpRequestFinished);

	headers = FskHTTPServerRequestGetRequestHeaders(session->request);
	
	FskHTTPClientRequestSetMethod(session->http.request, FskHeaderMethod(headers));

	value = FskHeaderFind("Range", headers);
	if (value)
		FskHTTPClientRequestAddHeader(session->http.request, "Range", value);
		
	if (session->authorization)
		FskHTTPClientRequestAddHeader(session->http.request, "Authorization", session->authorization);

	bailIfError(FskHTTPClientBeginRequest(session->http.client, session->http.request));
	err = kFskErrNeedMoreTime;
bail:
	FskInstrumentedItemPrintfNormal(session, "open %s (%d)", session->http.location, err);
	return err;
}

void KprProxyServerClose(KprLibrarySession session)
{
	FskInstrumentedItemPrintfNormal(session, "close http");
	FskMemPtrDispose(session->http.contentRange);
	session->http.contentRange = NULL;
	FskMemPtrDispose(session->http.contentLength);
	session->http.contentLength = NULL;
	FskHTTPClientDispose(session->http.client);
	session->http.client = NULL;
	session->http.request = NULL;
	FskMemPtrDispose(session->http.buffer);
	session->http.buffer = NULL;
	FskMemPtrDispose(session->http.location);
	session->http.location = NULL;
}

void KprProxyServerGenerateResponseHeaders(KprLibrarySession session, FskHeaders *responseHeaders)
{
	UInt32 responseCode = session->http.responseCode;
 	responseHeaders->responseCode = responseCode;
	if (2 == (responseCode / 100)) {
		if (session->http.contentLength) 
        	FskHeaderAddString(kFskStrContentLength, session->http.contentLength, responseHeaders);
		if (session->http.contentRange) 
        	FskHeaderAddString(kprHTTPHeaderContentRange, session->http.contentRange, responseHeaders);
		FskHeaderAddString("Accept-Ranges", "bytes", responseHeaders);
		FskHeaderAddString(kFskStrContentType, session->mime, responseHeaders);
		FskHeaderAddString("ContentFeatures.dlna.org", session->info, responseHeaders);
		FskHeaderAddString("transfermode.DLNA.ORG", (0 == FskStrCompareWithLength(session->mime, "image", 5)) ? "Interactive" : "Streaming", responseHeaders);
	}
	else {
		FskHeaderAddString(kFskStrContentLength, "0", responseHeaders);
	}
	if ((!session->http.done) && (session->http.readOffset == 0)) {
		FskHTTPServerRequestSuspend(session->request);
		FskHTTPClientResume(session->http.client);
	}
	FskInstrumentedItemPrintfNormal(session, "response headers http");
}

FskErr KprProxyServerGenerateResponseBody(KprLibrarySession session, char *data, UInt32 size, UInt32 *generated)
{
	FskErr err = kFskErrNone;
	UInt32 available = session->http.readOffset - session->http.writeOffset;
	if (available <= size) {
		FskMemCopy(data, session->http.buffer + session->http.writeOffset, available);
		*generated = available;
		session->http.readOffset = 0;
		session->http.writeOffset = 0;
		if (session->http.done)
			err = kFskErrEndOfFile;
		else {
			FskHTTPServerRequestSuspend(session->request);
			FskHTTPClientResume(session->http.client);
		}
	}
	else {
		FskMemCopy(data, session->http.buffer + session->http.writeOffset, size);
		*generated = size;
		session->http.writeOffset += size;
	}
    FskInstrumentedItemPrintfDebug(session, "response body http %ld (%d)", *generated, err);
    return err;
}

FskErr httpClientFinished(FskHTTPClient client, void *refCon)
{
	KprLibrarySession session = refCon;
	FskErr err = kFskErrNone;

    FskInstrumentedItemPrintfNormal(session, "proxy client finished");

	if ((kFskErrTimedOut == client->status.lastErr) 
			|| (kFskErrNetworkInterfaceRemoved == client->status.lastErr) 
			|| (kFskErrConnectionClosed == client->status.lastErr) 
			|| (kFskErrConnectionDropped == client->status.lastErr)) {
		KprProxyServerClose(session);
	}
	return err;
}

FskErr httpRequestHeadersReceived(FskHTTPClientRequest request, FskHeaders *responseHeaders, void *refCon)
{
	KprLibrarySession session = refCon;
	FskErr err = kFskErrNone;
	UInt32 responseCode = FskHeaderResponseCode(responseHeaders);
	char *value;

    FskInstrumentedItemPrintfNormal(session, "proxy headers received %d", (int)responseCode);

	if ((301 == responseCode) || (302 == responseCode) || (303 == responseCode) || (305 == responseCode) || (307 == responseCode)) {
		char *redirect = FskHeaderFind(kFskStrLocation, responseHeaders);
		char *location = NULL;

		if (NULL == redirect)
			return kFskErrOperationCancelled;

		if ('/' != redirect[0]) {
			if (0 != FskStrStr(redirect, "://"))
				location = FskStrDoCopy(redirect);
			else {
				char *t = FskStrDoCat(session->http.request->parsedUrl->scheme, "://");
				location = FskStrDoCat(t, redirect);		// no protocol given, so use the current one
				FskMemPtrDispose(t);
			}
		}
		else {
			if (kFskErrNone == FskMemPtrNew(FskStrLen(session->http.request->parsedUrl->scheme) + 4 + FskStrLen(session->http.request->parsedUrl->host) + FskStrLen(redirect), &location)) {
				FskStrCopy(location, session->http.request->parsedUrl->scheme);
				FskStrCat(location, "://");
				FskStrCat(location, session->http.request->parsedUrl->host);
				FskStrCat(location, redirect);
			}
		}

//		FskMemPtrDisposeAt(&session->http.additionalHeaders);		// conservative: cannot send headers across domain. ultimately need an event to allow client to provide correct headers.
//@@		FskMediaPlayerSendEvent(session->player, kFskEventMediaPlayerHeaders);

		KprProxyServerClose(session);
		session->http.location = location;
		KprProxyServerOpen(session);
		return kFskErrOperationCancelled;
	}
	session->http.responseCode = responseCode;
	value = FskHeaderFind(kFskStrContentLength, responseHeaders);
	if (value)
		session->http.contentLength = FskStrDoCopy(value);
	value = FskHeaderFind(kprHTTPHeaderContentRange, responseHeaders);
	if (value)
		session->http.contentRange = FskStrDoCopy(value);
 	
	FskHTTPClientSuspend(session->http.client);
	FskHTTPServerRequestResume(session->request);
	return err;
}

FskErr httpRequestDataReceived(FskHTTPClientRequest request, char *data, int size, void *refCon)
{
	KprLibrarySession session = refCon;
	FskErr err = kFskErrNone;

    FskInstrumentedItemPrintfDebug(session, "proxy data received %d", size);
	if (session->http.readOffset + size > session->http.bufferSize) {
    	FskInstrumentedItemPrintfDebug(session, "proxy buffer overflow");
    	session->http.bufferSize = session->http.readOffset + size;
    	bailIfError(FskMemPtrRealloc(session->http.bufferSize, &session->http.buffer));
	}
	FskMemCopy(session->http.buffer + session->http.readOffset, data, size);
	session->http.readOffset += size;
	FskHTTPClientSuspend(session->http.client);
	FskHTTPServerRequestResume(session->request);
bail:
	return err;
}

FskErr httpRequestFinished(FskHTTPClient client, FskHTTPClientRequest request, void *refCon)
{
	KprLibrarySession session = refCon;

    FskInstrumentedItemPrintfNormal(session, "proxy request finished");
    session->http.done = true;
	FskHTTPServerRequestResume(session->request);

	return kFskErrNone;
}




