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
#define __FSKHTTPSERVER_PRIV__

#if TARGET_OS_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

#include "Fsk.h"
#include "FskEndian.h"
#include "FskUUID.h"

#include "kpr.h"
#include "kprApplication.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHandler.h"
#include "kprHTTPClient.h"
#define __KPRHTTPSERVER_PRIV__
#include "kprHTTPServer.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprURL.h"
#include "kprUtilities.h"

#include "FskHTTPServer.h"

#define kKPRSSDPKinomaServe "urn:schemas-kinoma-com:device:%s:1"

#if TARGET_OS_IPHONE
    #include <SystemConfiguration/CaptiveNetwork.h>
#elif TARGET_OS_MAC
    #include "FskCocoaSupport.h"
#elif TARGET_OS_ANDROID
    #include "FskHardware.h"
#endif

#define kprUploadSizeDefault 0x7fff
#define kprUploadSizeMax 0x7fffffff

/* HTTP SERVER */

struct KprHTTPServerStruct {
	KprHTTPServer next;
	UInt32 port;
	char* id;
	UInt32 idLength;
	char* authority;
	UInt32 authorityLength;
	FskHTTPServer server;
	FskHTTPServerCallbackVectors vectors;
	KprHandler handler;
	xsMachine* the;
	xsSlot slot;
	xsIndex* code;
	FskInstrumentedItemDeclaration
};

static FskErr KprHTTPServerRequestConditionCallback(FskHTTPServerRequest request, UInt32 condition, void *refCon);
static FskErr KprHTTPServerRequestBodyCallback(FskHTTPServerRequest request UNUSED, char *data, UInt32 size, UInt32 *consumed, void *refCon);
static FskErr KprHTTPServerResponseBodyCallback(FskHTTPServerRequest request UNUSED, char *data, UInt32 size, UInt32 *generated, void *refCon);

/* HTTP TARGET */

typedef struct KprHTTPTargetStruct KprHTTPTargetRecord, *KprHTTPTarget;
typedef struct KprHTTPTargetFileStruct KprHTTPTargetFileRecord, *KprHTTPTargetFile;

struct KprHTTPTargetFileStruct {
	FskFile file;
	FskFileInfo info;
	char* path;
	FskInt64 from;
	FskInt64 to;
};

struct KprHTTPTargetStruct {
// 	KprHTTPTarget next;
	KprMessage message;
	Boolean processed;
	FskHTTPServerRequest request;
	KprHTTPServer server;
	KprHTTPTargetFile download;
	KprHTTPTargetFile upload;
	FskInt64 requestBodyOffset;
	FskInt64 responseBodyOffset;
	FskInstrumentedItemDeclaration
};

static FskErr KprHTTPTargetNew(KprHTTPTarget* it, KprMessage message);
static void KprHTTPTargetDispose(void* it);
static void KprHTTPTargetMessagePrepareDownload(KprMessage message, KprHTTPTargetFile file, FskHTTPServerRequest request);
static void KprHTTPTargetMessagePrepareUpload(KprMessage message, KprHTTPTargetFile file);
static void KprHTTPTargetMessageRequestCallback(KprMessage message, void* it);
static FskErr KprHTTPTargetMessageRequestBodyCallback(KprHTTPTarget self, KprMessage message, char *data, UInt32 size, UInt32 *consumed);
static FskErr KprHTTPTargetMessageResponseBodyCallback(KprHTTPTarget self, KprMessage message, char *data, UInt32 size, UInt32 *generated);

static void KprHTTPServerCallHandler(KprHTTPServer self, char* function, KprMessage message);

//--------------------------------------------------
// KprHTTPServer
//--------------------------------------------------

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprHTTPServerInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprHTTPServer", FskInstrumentationOffset(KprHTTPServerRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

KprHTTPServer gKprHTTPServerList = NULL;

FskErr KprHTTPServerNew(KprHTTPServer* it, char* authority, char* path, UInt32 preferredPort, FskSocketCertificateRecord* certs)
{
	FskErr err = kFskErrNone;
	UInt32 i;
	KprHTTPServer self = NULL;
	UInt32 port = KprEnvironmentGetUInt32("httpServerPort", 8080);
	UInt32 portRange = KprEnvironmentGetUInt32("httpServerPortRange", 10);
	KprHTTPServer server = NULL, previous = NULL;

	for (server = FskListGetNext(gKprHTTPServerList, NULL); server; server = FskListGetNext(gKprHTTPServerList, server)) {
		if (authority && FskStrCompareCaseInsensitive(server->authority, authority) == 0)
			return kFskErrDuplicateElement;
		if (preferredPort && (server->port == preferredPort))
			return kFskErrDuplicateElement;
		else if (port == server->port) {
			port++;
			previous = server;
		}
	}
	bailIfError(KprMemPtrNewClear(sizeof(KprHTTPServerRecord), it));
	self = *it;
	if (authority) {
		self->authority = FskStrDoCopy(authority);
		bailIfNULL(self->authority);
		self->authorityLength = FskStrLen(authority);
	}
	if (preferredPort) {
		self->port = preferredPort;
		err = FskHTTPServerCreate(self->port, NULL, &self->server, self, certs ? true : false);
	}
	else {
		self->port = port;
		for (i = 0; i < portRange; i++) {
			err = FskHTTPServerCreate(self->port, NULL, &self->server, self, certs ? true : false);
			if (!err)
				break;
			self->port++;
		}
	}
	bailIfError(err);
	if (certs) {
		bailIfError(FskHTTPServerSetCertificates(self->server, certs));
	}
	self->vectors.requestCondition = KprHTTPServerRequestConditionCallback;
	self->vectors.requestReceiveRequest = KprHTTPServerRequestBodyCallback;
	self->vectors.requestGenerateResponseBody = KprHTTPServerResponseBodyCallback;
	bailIfError(FskHTTPServerSetCallbacks(self->server, &self->vectors));
	if (previous)
		FskListInsertAfter(&gKprHTTPServerList, self, previous);
	else
		FskListPrepend(&gKprHTTPServerList, self);
	FskInstrumentedItemNew(self, NULL, &KprHTTPServerInstrumentation);
	if (authority)
		FskInstrumentedItemPrintfVerbose(self, "map ip:%d TO %s", self->port, authority);
	return err;
bail:
	KprHTTPServerDispose(self);
	return err;
}

void KprHTTPServerDispose(KprHTTPServer self)
{
	if (self) {
		FskListRemove(&gKprHTTPServerList, self);
		FskHTTPServerDispose(self->server);
		self->server = NULL;
		KprMemPtrDispose(self->authority);
		FskInstrumentedItemDispose(self);
		KprMemPtrDispose(self);
	}
}

KprHTTPServer KprHTTPServerGet(char* id)
{
	KprHTTPServer server = NULL;
	for (server = FskListGetNext(gKprHTTPServerList, NULL); server; server = FskListGetNext(gKprHTTPServerList, server)) {
		if (FskStrCompareCaseInsensitive(server->authority, id) == 0)
			return server;
	}
	return NULL;
}

UInt32 KprHTTPServerGetPort(KprHTTPServer self)
{
	return self->port;
}

UInt32 KprHTTPServerGetTimeout(KprHTTPServer self)
{
	return self->server->keepAliveTimeout;
}

Boolean KprHTTPServerIsSecure(KprHTTPServer self)
{
	return self->server->ssl;
}

void KprHTTPServerSetTimeout(KprHTTPServer self, UInt32 timeout)
{
	FskHTTPServerSetKeepAliveTimeout(self->server, timeout);
}

FskErr KprHTTPServerStart(KprHTTPServer self)
{
	return FskHTTPServerStart(self->server);
}

FskErr KprHTTPServerStop(KprHTTPServer self, Boolean flush)
{
	return FskHTTPServerStop(self->server, flush);
}

//--------------------------------------------------
// KprHTTPServer callbacks from Fsk
//--------------------------------------------------

FskErr KprHTTPServerRequestConditionCallback(FskHTTPServerRequest request, UInt32 condition, void *refCon)
{
	FskErr err = kFskErrNone;
	KprHTTPServer server = request->http->refCon;
	KprHTTPTarget target = refCon;
	KprMessage message = (target) ? target->message : NULL;
	KprURLPartsRecord parts;
	
	switch (condition) {
		case kFskHTTPConditionConnectionInitialized:
		case kFskHTTPConditionNoSocket:
			break;

		case kFskHTTPConditionRequestReceivedRequestHeaders: {
				FskHeaders *headers = FskHTTPServerRequestGetRequestHeaders(request);
				char* method = FskHeaderMethod(headers);
				char* requestURI = FskHeaderURI(headers);
				FskAssociativeArrayIterator iterate;
				if (requestURI)
					KprURLSplit(requestURI, &parts);
				if (!requestURI || !parts.scheme) {
					FskHeaders *headers = FskHTTPServerRequestGetResponseHeaders(request);
					headers->responseCode = 400;
					request->state = kHTTPServerError;
					return err;
				}
				bailIfError(KprMessageNew(&message, requestURI));
				bailIfError(KprHTTPTargetNew(&target, message));

				target->server = server;
				target->request = request;
				message->usage++; // processing
				message->method = FskStrDoCopy(method);

				iterate = FskHeaderIteratorNew(headers);
				while (iterate) {
					bailIfError(KprMessageSetRequestHeader(message, iterate->name, iterate->value));
					iterate = FskHeaderIteratorNext(iterate);
				}
				FskHeaderIteratorDispose(iterate);
				FskHTTPServerRequestSetRefcon(request, target);
#if SUPPORT_INSTRUMENTATION
				{
					int IP, port;
					char remoteHost[64];
					FskNetSocketGetRemoteAddress(request->skt, (UInt32 *)&IP, &port);
					FskNetIPandPortToString(IP, port, remoteHost);
					FskInstrumentedItemPrintfVerbose(target, "accept from %s %s", remoteHost, message->url);
				}
#endif
                message->request.callback = KprHTTPTargetMessageRequestCallback; // @@
                message->request.target = target; // @@
				if (!server->authority)
					KprHTTPServerCallHandler(server, "onAccept", message);
				else if (!FskStrCompare(server->authority, gShell->id))
					KprContextAccept(gShell, message);
				else {
					KprContentLink link = gShell->applicationChain.first;
					while (link) {
						KprApplication application = (KprApplication)link->content;
						if (application->id && (!FskStrCompare(server->authority, application->id))) {
							KprContextAccept(application, message);
							break;
						}
						link = link->next;
					}
				}
                message->request.callback = NULL; // @@
                message->request.target = NULL; // @@
            
				if (message->status != 404) {
					if (target->upload) {
						KprHTTPTargetMessagePrepareUpload(message, target->upload);
					}
				}
			}
			break;

		case kFskHTTPConditionRequestRequestFinished:
	        FskAssert(message);
				if (target->upload) {
					FskFileClose(target->upload->file);
					target->upload->file = NULL;
				}
				FskHTTPServerRequestSuspend(request);
				message->waiting = true;
				message->request.callback = KprHTTPTargetMessageRequestCallback;
				message->request.dispose = NULL;
				message->request.target = target;
				message->usage++; // request
				FskListAppend(&gShell->messages, message);
				message->usage++; // message queue
				
				if (!server->authority)
					KprHTTPServerCallHandler(server, "onInvoke", message);
				else if (!FskStrCompare(server->authority, gShell->id))
					KprContextInvoke(gShell, message);
				else {
					KprContentLink link = gShell->applicationChain.first;
					while (link) {
						KprApplication application = (KprApplication)link->content;
						if (application->id && (!FskStrCompare(server->authority, application->id))) {
							KprContextInvoke(application, message);
							break;
						}
						link = link->next;
					}
				}
				if (!message->response.callback) {
			        KprMessageTransform(message, gShell->the);
					KprMessageComplete(message);
				}
				//KprMessageInvoke(message, KprHTTPTargetMessageRequestCallback, NULL, target);
			break;

		case kFskHTTPConditionRequestGenerateResponseHeaders:
            FskAssert(message);
         	{
				Boolean isChunked = true;
				FskHeaders *headers = FskHTTPServerRequestGetResponseHeaders(request);
				FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(message->response.headers);
				headers->responseCode = message->status;
				while (iterate) {
					if (isChunked)
						isChunked = FskStrCompareCaseInsensitive(iterate->name, kFskStrContentLength);
					FskHeaderAddString(iterate->name, iterate->value, headers);
					iterate = FskAssociativeArrayIteratorNext(iterate);
				}
				FskAssociativeArrayIteratorDispose(iterate);
				if (isChunked) {
					FskHeaderAddString(kFskStrTransferEncoding, kFskStrChunked, headers);
				}
			}
			break;

		case kFskHTTPConditionRequestResponseFinished:
            FskAssert(message);
			if (message->timeout)
				FskHTTPServerRequestSetKeepAliveTimeout(request, message->timeout);
		case kFskHTTPConditionConnectionTerminating:
		case kFskHTTPConditionRequestErrorAbort:
            if (target) {
                KprHTTPTargetDispose(target);
				FskHTTPServerRequestSetRefcon(request, NULL);
			}
			if (message) {
				KprMessageCancel(message);
				message->usage--; // processing
				if (!message->usage)
					KprMessageDispose(message);
            }
			else
				err = kFskErrNotFound;
			break;

		default:
			err = kFskErrUnimplemented;
			break;
	}

bail:
	if (err == kFskErrMemFull) {
		KprHTTPTargetDispose(target);
		KprMessageDispose(message);
	}
	return err;
}

FskErr KprHTTPServerRequestBodyCallback(FskHTTPServerRequest request UNUSED, char *data, UInt32 size, UInt32 *consumed, void *refCon)
{
	FskErr err = kFskErrNone;
	KprHTTPTarget target = refCon;
	KprMessage message = target->message;
	bailIfError(KprHTTPTargetMessageRequestBodyCallback(target, message, data, size, consumed));
bail:
	return err;
}

FskErr KprHTTPServerResponseBodyCallback(FskHTTPServerRequest request UNUSED, char *data, UInt32 size, UInt32 *generated, void *refCon)
{
	FskErr err = kFskErrNone;
	KprHTTPTarget target = refCon;
	KprMessage message = target->message;
	bailIfError(KprHTTPTargetMessageResponseBodyCallback(target, message, data, size, generated));
bail:
	return err;
}

//--------------------------------------------------
// KprHTTPTarget
//--------------------------------------------------

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprHTTPTargetInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprHTTPServerTarget", FskInstrumentationOffset(KprHTTPTargetRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprHTTPTargetNew(KprHTTPTarget* it, KprMessage message)
{
	KprHTTPTarget self;
	FskErr err = kFskErrNone;
	bailIfError(FskMemPtrNewClear(sizeof(KprHTTPTargetRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprHTTPTargetInstrumentation);
	self->message = message;
bail:
	return err;
}

void KprHTTPTargetDispose(void* it)
{
	KprHTTPTarget self = it;
	if (self) {
		KprHTTPTargetFile file = self->upload;
		if (file) {
			FskMemPtrDisposeAt(&file->path);
			FskFileClose(file->file);
		}
		file = self->download;
		if (file) {
			FskMemPtrDisposeAt(&file->path);
			FskFileClose(file->file);
		}
		FskMemPtrDispose(self->upload);
		FskMemPtrDispose(self->download);
		FskInstrumentedItemDispose(self);
		FskMemPtrDispose(self);
	}
}

void KprHTTPTargetMessageRequestCallback(KprMessage message, void* it)
{
	KprHTTPTarget self = it;
	FskHTTPServerRequest request = self->request;
	KprHTTPTargetFile download = self->download;
	char string[16];

	FskInstrumentedItemPrintfVerbose(self, "message  %p '%s'", message, message->url);
	
	self->processed = true;
	self->responseBodyOffset = 0;
	
	if (download) {
		KprHTTPTargetMessagePrepareDownload(message, download, request);
	}
	else {
		if (!KprMessageGetResponseHeader(message, kFskStrContentLength)) {
			FskStrNumToStr(message->response.size, string, sizeof(string));
			KprMessageSetResponseHeader(message, kFskStrContentLength, string);
		}
		if (!message->status) {
			if (message->response.body)
				message->status = 200;
			else
				message->status = 204;
		}
	}
	FskHTTPServerRequestResume(request);
	return;
}

void KprHTTPTargetMessagePrepareDownload(KprMessage message, KprHTTPTargetFile file, FskHTTPServerRequest request UNUSED)
{
	message->status = 500;
	if (!FskStrCompareCaseInsensitive("GET", message->method) || !FskStrCompareCaseInsensitive("HEAD", message->method))
		if (FskFileGetFileInfo(file->path, &file->info) == kFskErrNone) {
			char* modified = KprMessageGetRequestHeader(message, kprHTTPHeaderIfModifiedSince);
			char* ifNoneMatch = KprMessageGetRequestHeader(message, kprHTTPHeaderIfNoneMatch);
			char etag[48];
			UInt32 date = 0;
			
			snprintf(etag, 47, "\"%lX%lX%lX%lX%lX\"", (UInt32)((file->info.fileNode) >> 32), (UInt32)(file->info.fileNode), file->info.fileModificationDate, (UInt32)((file->info.filesize) >> 32), (UInt32)(file->info.filesize));
			
			if (modified)
				if (kFskErrNone != KprDateFromHTTP(modified, &date))
					date = 0;

			if (ifNoneMatch) {
				if (FskStrStr(ifNoneMatch, etag) || FskStrStr(ifNoneMatch, "*")) {
					if (!date || (file->info.fileModificationDate == date)) {
						message->status = 304;
						return;
					}
				}
			}
			if (kFskDirectoryItemIsFile == file->info.filetype) {
				FskInt64 length = file->info.filesize;
				FskInt64 from = 0;
				FskInt64 to = length - 1;
				char* range = KprMessageGetRequestHeader(message, "RANGE");
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
							if (FskFileOpen(file->path, kFskFilePermissionReadOnly, &file->file) == kFskErrNone) {
								length = to - from + 1;
								message->status = 206;
								if (from)
									FskFileSetPosition(file->file, &from);
							}
						}
						else
							message->status = 416;
					}
					else
						message->status = 416;
				}
				else if (FskFileOpen(file->path, kFskFilePermissionReadOnly, &file->file) == kFskErrNone) {
					message->status = 200;
				}
				file->from = from;
				file->to = to;
				if (range) {
					if (message->status == 206) {
						sprintf(buffer, "bytes %lld-%lld/%lld", from, to, file->info.filesize);
						KprMessageSetResponseHeader(message, kprHTTPHeaderContentRange, buffer);
					}
					else if (message->status == 416) {
						sprintf(buffer, "bytes */%lld", file->info.filesize);
						KprMessageSetResponseHeader(message, kprHTTPHeaderContentRange, buffer);
					}
				}
				if (message->status / 100 == 2) {
					sprintf(buffer, "%lld", length);
					KprMessageSetResponseHeader(message, kFskStrContentLength, buffer);
					strftime(buffer, 31, "%a, %d %b %Y %H:%M:%S GMT", gmtime((const time_t*)&file->info.fileModificationDate));
					KprMessageSetResponseHeader(message, kprHTTPHeaderLastModified, buffer);
					KprMessageSetResponseHeader(message, kprHTTPHeaderETag, etag);
				}
			}
		}
}

void KprHTTPTargetMessagePrepareUpload(KprMessage message, KprHTTPTargetFile upload)
{
	FskErr err = kFskErrNone;
	FskFileInfo info;
	UInt32 consumed;
	
	if (kFskErrNone == FskFileGetFileInfo(upload->path, &info)) {
		bailIfError(FskFileDelete(upload->path));
	}
	else {
		KprEnsureDirectory(upload->path);
	}
	bailIfError(FskFileCreate(upload->path));
	bailIfError(FskFileOpen(upload->path, kFskFilePermissionReadWrite, &upload->file));
	bailIfError(FskFileWrite(upload->file, message->request.size, message->request.body, &consumed));
	return;
bail:
	message->status = 500;
	return;
}

FskErr KprHTTPTargetMessageRequestBodyCallback(KprHTTPTarget self, KprMessage message, char *data, UInt32 size, UInt32 *consumed)
{
	FskErr err = kFskErrNone;
	KprHTTPTargetFile upload = self->upload;
	FskInstrumentedItemPrintfVerbose(self, "request body %p '%s'", message, message->url);
	if (upload) {
		if (upload->file) {
			bailIfError(FskFileWrite(upload->file, size, data, consumed));
		}
		else if (consumed)
			*consumed = size;
	}
	else {
		if (!message->request.body) {
			char* value;
			message->request.size = size;
			value = KprMessageGetRequestHeader(message, kFskStrContentLength);
			if (value)
				message->request.size = atoi(value);
			bailIfError(FskMemPtrNew(message->request.size, &message->request.body));
		}
		if (self->requestBodyOffset + size >= message->request.size) {
			message->request.size = self->requestBodyOffset + size;
			bailIfError(FskMemPtrRealloc(message->request.size, &message->request.body));
		}
		FskMemCopy((UInt8*)message->request.body + self->requestBodyOffset, data, size);
		self->requestBodyOffset += size;
		if (consumed)
			*consumed = size;
	}
bail:
	return err;
}

FskErr KprHTTPTargetMessageResponseBodyCallback(KprHTTPTarget self, KprMessage message, char *data, UInt32 size, UInt32 *generated)
{
	FskErr err = kFskErrNone;
	KprHTTPTargetFile download = self->download;
	FskInstrumentedItemPrintfVerbose(self, "response %p '%s'", message, message->url);
	if (download) {
		if (download->file) {
			UInt32 amt = 0;
			if (size > download->to - download->from + 1)
				size = download->to - download->from + 1;
			if (FskFileRead(download->file, size, data, &amt) == kFskErrNone) {
				*generated = amt;
				download->from += amt;
				if (amt == 0)
					err = kFskErrEndOfFile;
			}
			else {
				*generated = 0;
				err = kFskErrEndOfFile;
			}
		}
		else {
			*generated = 0;
			err = kFskErrEndOfFile;
		}
	}
	else {
		UInt32 messageSize = message->response.size;
		UInt32 offset = self->responseBodyOffset;
		
		if (messageSize - offset <= size) {
			size = messageSize - offset;
		}
		if (size) {
			FskMemCopy(data, (char *)message->response.body + offset, size);
			self->responseBodyOffset += size;
		}
		*generated = size;	
		if (self->responseBodyOffset == messageSize)
			err = kFskErrEndOfFile;
	}
	return err;
}

void KprHTTPTargetMessageSetResponseProtocol(KprMessage message, char* protocol)
{
	KprHTTPTarget target = message->request.target;
	FskHTTPServerRequest request = target ? target->request : NULL;
	if (request) {
		if (FskStrCompare(request->requestHeaders->protocol, protocol)) {
			request->responseHeaders->protocol = FskStrDoCopy(protocol);
		}
	}
}

//--------------------------------------------------
// Network Interface
//--------------------------------------------------

static FskNetInterface gNetworkInterface = NULL;
static FskNetInterfaceNotifier gNetInterfaceNotifier = NULL;

static void KprNetworkInterfaceAdd(FskNetInterface iface);
static Boolean KprNetworkInterfaceIsLocal(FskNetInterface iface);
static void KprNetworkInterfaceNotifyConnect(Boolean setup);
static void KprNetworkInterfaceRemove(FskNetInterface iface);

void KprNetworkInterfaceActivate(Boolean activateIt)
{
#if TARGET_OS_IPHONE
	if (activateIt)
		FskNetInterfacesChanged();
#endif
}

void KprNetworkInterfaceAdd(FskNetInterface iface)
{
	FskErr err = kFskErrNone;
	FskNetInterface existing = NULL;
	char ip[32];
	char *buffer = NULL;
	unsigned char* MAC;
	KprMessage message = NULL;
	Boolean local;
	
	if (iface->ip && (0x7f000001 != iface->ip) && iface->status) {
		bailIfError(FskMemPtrNewFromData(sizeof(FskNetInterfaceRecord), iface, &existing));
		existing->next = NULL;
		existing->name = FskStrDoCopy(iface->name);
		FskListAppend(&gNetworkInterface, existing);
		FskNetIPandPortToString(existing->ip, 0, ip);
		FskDebugStr("ADD INTERFACE %s", ip);
		MAC = (unsigned char*)existing->MAC;
		local = KprNetworkInterfaceIsLocal(existing);
		bailIfError(FskMemPtrNewClear(45 + FskStrLen(existing->name) + FskStrLen(ip) + 25, &buffer));
		sprintf(buffer, "xkpr:///network/interface/add?name=%s&ip=%s&MAC=%02x:%02x:%02x:%02x:%02x:%02x&local=%d", existing->name, ip, MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5], local);
		bailIfError(KprMessageNew(&message, buffer));
		KprMessageNotify(message);
	}
bail:
	FskMemPtrDispose(buffer);
}

int KprNetworkInterfaceCallback(struct FskNetInterfaceRecord* iface, UInt32 status, void* param)
{
	FskErr err = kFskErrNone;
	Boolean connected = gNetworkInterface != NULL;
	FskDebugStr("KprNetworkInterfaceCallback %s -> %d", iface->name, status);
	switch (status) {
		case kFskNetInterfaceStatusNew:
			KprNetworkInterfaceAdd(iface);
		break;
		case kFskNetInterfaceStatusRemoved:
			KprNetworkInterfaceRemove(iface);
		break;
		case kFskNetInterfaceStatusChanged:
			KprNetworkInterfaceRemove(iface);
			KprNetworkInterfaceAdd(iface);
		break;
	}
	if (connected != (gNetworkInterface != NULL)) {
		KprNetworkInterfaceNotifyConnect(false);
	}
	return err;
}

void KprNetworkInterfaceCleanup()
{
	FskNetInterface existing, next;
	for (existing = gNetworkInterface; existing; existing = next) {
		next = existing->next;
		FskNetInterfaceDescriptionDispose(existing);
	}
	FskNetInterfaceRemoveNotifier(gNetInterfaceNotifier);
	gNetInterfaceNotifier = NULL;
}

Boolean KprNetworkInterfaceIsLocal(FskNetInterface iface)
{
#if TARGET_OS_IPHONE
	return (iface->name[0] == 'e') && (iface->name[1] == 'n');
#elif TARGET_OS_MAC
	return (iface->name[0] == 'e') && (iface->name[1] == 'n');
#elif TARGET_OS_ANDROID
	return !FskStrCompareWithLength(iface->name, "wlan", 4);
#else
	return true;
#endif
}

void KprNetworkInterfaceNotifyConnect(Boolean setup)
{
	KprMessage message = NULL;
	char buffer[128];
	if (gNetworkInterface != NULL) {
		char ip[32];
		unsigned char* MAC = (unsigned char*)gNetworkInterface->MAC;
		FskNetIPandPortToString(gNetworkInterface->ip, 0, ip);
		snprintf(buffer, sizeof(buffer), "xkpr:///network/connect?status=true&ip=%s&MAC=%02x:%02x:%02x:%02x:%02x:%02x", ip, MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
	}
	else {
		snprintf(buffer, sizeof(buffer), "xkpr:///network/connect?status=false");
	}
	if (setup)
		FskStrCat(buffer, "&setup=true");
	if (KprMessageNew(&message, buffer) == kFskErrNone)
		KprMessageNotify(message);
}

void KprNetworkInterfaceRemove(FskNetInterface iface)
{
	FskErr err = kFskErrNone;
	char ip[22];
	char *buffer = NULL;
	KprMessage message = NULL;
	FskNetInterface existing = NULL;
	Boolean local;
	
	for (existing = gNetworkInterface; existing; existing = existing->next) {
		if (!FskStrCompare(existing->name, iface->name)) break;
	}
	if (existing) {
		FskListRemove(&gNetworkInterface, existing);
		FskNetIPandPortToString(existing->ip, 0, ip);
		FskDebugStr("REMOVE INTERFACE %s", ip);
		local = KprNetworkInterfaceIsLocal(existing);
		bailIfError(FskMemPtrNewClear(51 + FskStrLen(existing->name) + FskStrLen(ip), &buffer));
		sprintf(buffer, "xkpr:///network/interface/remove?ip=%s&name=%s&local=%d", ip, existing->name, local);
		bailIfError(KprMessageNew(&message, buffer));
		KprMessageNotify(message);
	}
bail:
	FskNetInterfaceDescriptionDispose(existing);
	FskMemPtrDispose(buffer);
}

void KprNetworkInterfaceSetup()
{
	UInt32 i, count;

	count = FskNetInterfaceEnumerate();
	for (i = 0; i < count; i++) {
		FskNetInterface iface;
		FskNetInterfaceDescribe(i, &iface);
		KprNetworkInterfaceAdd(iface);
		FskNetInterfaceDescriptionDispose(iface);
	}
	KprNetworkInterfaceNotifyConnect(true);
	gNetInterfaceNotifier = FskNetInterfaceAddNotifier(KprNetworkInterfaceCallback, NULL, "KprNetworkInterfaceCallback");
}

//--------------------------------------------------
// XS message
//--------------------------------------------------

void KPR_message_get_remoteIP(xsMachine *the)
{
	KprMessage self = kprGetHostData(xsThis, this, message);
	if (self->request.callback == KprHTTPTargetMessageRequestCallback) {
		FskHTTPServerRequest request = ((KprHTTPTarget)self->request.target)->request;
		int IP;
		char remoteIP[64];
		FskNetSocketGetRemoteAddress(request->skt, (UInt32 *)&IP, NULL);
		FskNetIPandPortToString(IP, 0, remoteIP);
		xsResult = xsString(remoteIP);
	}
}

void KPR_message_set_requestPath(xsMachine *the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFile file = NULL;
	KprMessage self = kprGetHostData(xsThis, this, message);
	if (self->request.callback == KprHTTPTargetMessageRequestCallback) {
		KprHTTPTarget target = self->request.target;
		if (!target->upload) {
			bailIfError(FskMemPtrNewClear(sizeof(KprHTTPTargetFileRecord), &target->upload));
			bailIfError(KprURLToPath(xsToString(xsArg(0)), &target->upload->path));
		}
	}
	else {
		bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
		err = FskFileOpen(path, kFskFilePermissionReadWrite, &file);
		if (kFskErrFileNotFound == err) {
			bailIfError(FskFileCreate(path));
			err = FskFileOpen(path, kFskFilePermissionReadWrite, &file);
		}
		else {
			FskInt64 size = 0;
			err = FskFileSetSize(file, &size);
		}
		bailIfError(err);
		bailIfError(FskFileWrite(file, self->request.size, self->request.body, NULL));
		FskFileClose(file);
		file = NULL;
		FskMemPtrDispose(path);
		path = NULL;
		self->sniffing = true;
	}
bail:
	FskFileClose(file);
	FskMemPtrDispose(path);
	return;
}

void KPR_message_set_responsePath(xsMachine *the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFileMapping map = NULL;
	unsigned char *data;
	FskInt64 size;
	KprMessage self = kprGetHostData(xsThis, this, message);
	if (self->request.callback == KprHTTPTargetMessageRequestCallback) {
		KprHTTPTarget target = self->request.target;
		if (!target->download) {
			bailIfError(FskMemPtrNewClear(sizeof(KprHTTPTargetFileRecord), &target->download));
			bailIfError(KprURLToPath(xsToString(xsArg(0)), &target->download->path));
		}
	}
	else {
		char* path;
		bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
		bailIfError(FskFileMap(path, &data, &size, 0, &map));
		self->response.size = (UInt32)size;
		bailIfError(FskMemPtrNew(self->response.size, &self->response.body));
		FskMemMove(self->response.body, data, self->response.size);
	}
bail:
	FskFileDisposeMap(map);
	FskMemPtrDispose(path);
	return;
}

//--------------------------------------------------
// XS context
//--------------------------------------------------

static Boolean KprContextGetServices(xsMachine *the, xsSlot slot, char* services, UInt32 size)
{
	Boolean shareIt = false;
	
	xsEnterSandbox();
	fxPush(slot);
	fxRunForIn(the);
	for (xsVar(0) = fxPop(); xsTypeOf(xsVar(0)) != xsNullType; xsVar(0) = fxPop()) {
		if (xsTypeOf(xsVar(0)) == xsStringType) {
			xsVar(1) = xsGetAt(slot, xsVar(0));
			if ((xsTypeOf(xsVar(1)) == xsBooleanType) && xsTest(xsVar(1))) {
				char* service = xsToString(xsVar(0));
				shareIt = true;
				FskStrNCat(services, service, size);
				FskStrNCat(services, ":", size);
			}
		}
	}
	xsLeaveSandbox();
	return shareIt;
}


void KPR_context_get_serverIsSecure(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprHTTPServer self = KprHTTPServerGet(context->id);
	if (self) {
		xsResult = xsBoolean(KprHTTPServerIsSecure(self));
	}
}

void KPR_context_get_serverPort(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprHTTPServer self = KprHTTPServerGet(context->id);
	if (self) {
		xsResult = xsInteger(self->port);
	}
}

void KPR_context_get_shared(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprHTTPServer self = KprHTTPServerGet(context->id);
	xsResult = xsBoolean(self != NULL);
}

void KPR_context_get_uuid(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	char* uuid = FskUUIDGetForKey(context->id);
	xsResult = xsString(uuid);
}

void KPR_context_discover(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	char services[256] = ":";
	if ((xsToInteger(xsArgc) > 1) && xsIsInstanceOf(xsArg(1), xsObjectPrototype)) {
		xsVars(2);
		KprContextGetServices(the, xsArg(1), services, 255);
		KprServicesDiscover((KprContext)context, xsToString(xsArg(0)), services);
	}
	else
		KprServicesDiscover((KprContext)context, xsToString(xsArg(0)), NULL);
}

void KPR_context_forget(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprServicesForget((KprContext)context, xsToString(xsArg(0)));
}

void KPR_context_set_shared(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprHTTPServer self = KprHTTPServerGet(context->id);
	
	if (xsTest(xsArg(0))) {
		if (!self) {
			UInt32 port = KprEnvironmentGetUInt32((context == (KprContext)gShell) ? "httpShellPort" : "httpApplicationPort", 0);
			xsThrowIfFskErr(KprHTTPServerNew(&self, context->id, "", port, NULL));
			KprHTTPServerStart(self);
			KprServicesShare((KprContext)context, true, NULL);
		}
	}
	else {
		if (self) {
			KprServicesShare((KprContext)context, false, NULL);
			KprHTTPServerStop(self, true);
			KprHTTPServerDispose(self);
		}
	}
	return;
}

void KPR_context_share(xsMachine *the)
{
	KprContext context = xsGetContext(the);
	KprHTTPServer self = KprHTTPServerGet(context->id);
	Boolean shareIt;
	char services[256] = ":";
	xsVars(2);
	
	if (xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
		shareIt = true;
		KprContextGetServices(the, xsArg(0), services, 255);
	}
	else
		shareIt = xsTest(xsArg(0));
	
	xsEnterSandbox();
	if (shareIt) {
		if (!self) {
			UInt32 port = 0;
			FskSocketCertificateRecord* certs = NULL;
			FskSocketCertificateRecord certsRecord = {
				NULL, 0,
				NULL,
				NULL,
				NULL, 0,
			};
			if (xsIsInstanceOf(xsArg(0), xsObjectPrototype) && xsHas(xsArg(0), xsID("port")))
				port = xsToInteger(xsGet(xsArg(0), xsID("port")));
			else
				port = KprEnvironmentGetUInt32((context == (KprContext)gShell) ? "httpShellPort" : "httpApplicationPort", 0);
			if (xsIsInstanceOf(xsArg(0), xsObjectPrototype) && xsHas(xsArg(0), xsID("ssl"))) {
				xsVar(0) = xsGet(xsArg(0), xsID("ssl"));
				if (xsTest(xsVar(0))) {
					if (xsTypeOf(xsVar(0)) == xsBooleanType) {
						certs = &certsRecord;
					}
					else if (xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
						certs = &certsRecord;
						if (xsHas(xsVar(0), xsID("certificates"))) {
							certs->certificates = (void*)xsToString(xsGet(xsVar(0), xsID("certificates")));
							certs->certificatesSize = FskStrLen(certs->certificates);
						}
						if (xsHas(xsVar(0), xsID("policies"))) {
							certs->policies = xsToString(xsGet(xsVar(0), xsID("policies")));
						}
						if (xsHas(xsVar(0), xsID("hostname"))) {
							certs->hostname = xsToString(xsGet(xsVar(0), xsID("hostname")));
						}
						if (xsHas(xsVar(0), xsID("key"))) {
							certs->key = (void*)xsToString(xsGet(xsVar(0), xsID("key")));
							certs->keySize = FskStrLen(certs->key);
						}
					}
					if (certs) {
						if (!certs->certificates) {
							certs->certificates = (void*)kKprHTTPServerDefaultCretificates;
							certs->certificatesSize = FskStrLen(kKprHTTPServerDefaultCretificates);
						}
						if (!certs->key) {
							certs->key = (void*)kKprHTTPServerDefaultKey;
							certs->keySize = FskStrLen(kKprHTTPServerDefaultKey);
						}
					}
				}
			}
			xsThrowIfFskErr(KprHTTPServerNew(&self, context->id, "", port, certs));
			KprHTTPServerStart(self);
		}
		KprServicesShare((KprContext)context, true, services);
	}
	else {
		if (self) {
			KprServicesShare((KprContext)context, false, NULL);
			KprHTTPServerStop(self, true);
			KprHTTPServerDispose(self);
		}
	}
	xsLeaveSandbox();
	return;
}

//--------------------------------------------------
// XS system
//--------------------------------------------------

void KPR_system_get_SSID(xsMachine *the)
{
#if TARGET_OS_IPHONE
	CFArrayRef array = CNCopySupportedInterfaces();
	if (array) {
        CFDictionaryRef dictionary = CNCopyCurrentNetworkInfo(CFArrayGetValueAtIndex(array, 0));
        if (dictionary) {
            CFStringRef ssid = CFDictionaryGetValue(dictionary, kCNNetworkInfoKeySSID);
            if (ssid) {
				char buffer[1024];
				CFStringGetCString(ssid, buffer, sizeof(buffer), kCFStringEncodingUTF8);
				xsResult = xsString(buffer);
			}
            CFRelease(dictionary);
        }
        CFRelease(array);
   }
#elif TARGET_OS_MAC
	const char *ssid = FskCocoaSSID();
	if (ssid)
		xsResult = xsString((char *)ssid);
#elif TARGET_OS_ANDROID
	if (gFskPhoneHWInfo && gFskPhoneHWInfo->ssid) {
		UInt32 length = FskStrLen(gFskPhoneHWInfo->ssid);
		if (gFskPhoneHWInfo->ssid[0] == '"' && gFskPhoneHWInfo->ssid[length-1] == '"')
			xsResult = xsStringBuffer(gFskPhoneHWInfo->ssid + 1, length - 2);
		else
			xsResult = xsString(gFskPhoneHWInfo->ssid);
	}
#endif
}

//--------------------------------------------------
// KPR HTTP Server
//--------------------------------------------------

void KPR_HTTP_Server(xsMachine *the)
{
	KprHTTPServer self = NULL;
//	static UInt32 serverIndex = 0;
	xsVars(2);
	xsEnterSandbox();
	{
		xsTry {
			UInt32 port = 0;
			FskSocketCertificateRecord* certs = NULL;
			FskSocketCertificateRecord certsRecord = {
				NULL, 0,
				NULL,
				NULL,
				NULL, 0,
			};
			if (xsIsInstanceOf(xsArg(0), xsObjectPrototype) && xsHas(xsArg(0), xsID("port")))
				port = xsToInteger(xsGet(xsArg(0), xsID("port")));
			if (xsIsInstanceOf(xsArg(0), xsObjectPrototype) && xsHas(xsArg(0), xsID("ssl"))) {
				xsVar(0) = xsGet(xsArg(0), xsID("ssl"));
				if (xsTest(xsVar(0))) {
					if (xsTypeOf(xsVar(0)) == xsBooleanType) {
						certs = &certsRecord;
					}
					else if (xsIsInstanceOf(xsArg(0), xsObjectPrototype)) {
							certs = &certsRecord;
						if (xsHas(xsVar(0), xsID("certificates"))) {
							certs->certificates = (void*)xsToString(xsGet(xsVar(0), xsID("certificates")));
							certs->certificatesSize = FskStrLen(certs->certificates);
						}
						if (xsHas(xsVar(0), xsID("policies"))) {
							certs->policies = xsToString(xsGet(xsVar(0), xsID("policies")));
						}
						if (xsHas(xsVar(0), xsID("hostname"))) {
							certs->hostname = xsToString(xsGet(xsVar(0), xsID("hostname")));
						}
						if (xsHas(xsVar(0), xsID("key"))) {
							certs->key = (void*)xsToString(xsGet(xsVar(0), xsID("key")));
							certs->keySize = FskStrLen(certs->key);
						}
				}
					if (certs) {
						if (!certs->certificates) {
							certs->certificates = (void*)kKprHTTPServerDefaultCretificates;
							certs->certificatesSize = FskStrLen(kKprHTTPServerDefaultCretificates);
						}
						if (!certs->key) {
							certs->key = (void*)kKprHTTPServerDefaultKey;
							certs->keySize = FskStrLen(kKprHTTPServerDefaultKey);
						}
					}
				}
			}
 			xsThrowIfFskErr(KprHTTPServerNew(&self, NULL, "", port, certs));
			xsSetHostData(xsResult, self);
		}
		xsCatch {
		}
	}
	xsLeaveSandbox();
}

void KPR_HTTP_server(void* it)
{
	KprHTTPServer self = it;
	if (self) {
		if (self->handler) {
			KprHTTPServerStop(self, true);
			KprContextRemoveHandler(gShell, self->handler);
			self->handler = NULL;
		}
		KprHTTPServerDispose(self);
	}
}

void KPR_HTTP_server_get_port(xsMachine *the)
{
	KprHTTPServer self = xsGetHostData(xsThis);
	if (self)
		xsResult = xsInteger(self->port);
	else
		xsResult = xsUndefined;
}

void KPR_HTTP_server_get_running(xsMachine *the)
{
	KprHTTPServer self = xsGetHostData(xsThis);
	xsResult = self->handler ? xsTrue : xsFalse;
}

void KPR_HTTP_server_start(xsMachine *the)
{
	KprHTTPServer self = xsGetHostData(xsThis);
	if (self && !self->handler) {
		self->the = the;
		self->code = the->code;
		self->slot = xsThis;
		KprHTTPServerStart(self);
		xsThrowIfFskErr(KprHandlerNew(&self->handler, NULL));
		KprContextPutHandler(gShell, self->handler);
		
	}
}

void KPR_HTTP_server_stop(xsMachine *the)
{
	KprHTTPServer self = xsGetHostData(xsThis);
	if (self && self->handler) {
		KprHTTPServerStop(self, true);
		self->the = NULL;
		self->code = NULL;
		self->slot = xsUndefined;
		KprContextRemoveHandler(gShell, self->handler);
		self->handler = NULL;
	}
}

void KprHTTPServerCallHandler(KprHTTPServer self, char* function, KprMessage message)
{
// 	if (!self->handler) {
// 		message->status = 503; // Service Unavailable
// 		return;
// 	}
	xsBeginHostSandboxCode(self->the, self->code);
	{
		xsVars(3);
		{
			xsTry {
				message->status = 404;
				xsVar(0) = xsAccess(self->slot);
				if (xsFindResult(xsVar(0), xsID_behavior)) {
					if (xsTest(xsResult)) {
						xsVar(0) = xsResult;
						if (xsIsInstanceOf(xsVar(0), xsObjectPrototype) && xsFindResult(xsVar(0), xsID(function))) {
							xsVar(1) = kprContentGetter(self->handler);
							xsSet(xsVar(1), xsID_behavior, xsVar(0));
							xsVar(2) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), xsID_message));
							xsSetHostData(xsVar(2), message);
							FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageConstruct, message);
							message->status = 0;
							message->usage++; // host
							self->handler->message = message;
							(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
							if (self->handler) // if the server is stopped in the callback
								self->handler->message = NULL;
						}
					}
				}
			}
			xsCatch {
				message->status = 500;
			}
		}
	}
	xsEndHostSandboxCode();
}

