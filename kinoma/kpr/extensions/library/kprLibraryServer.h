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
#ifndef __KPRLIBRARYSERVER__
#define __KPRLIBRARYSERVER__

#define KPR_NO_GRAMMAR 1
#include "kpr.h"
#include "kprURL.h"
#include "FskHTTPClient.h"
#include "FskHTTPServer.h"

#define kQueryCount 10
typedef struct KprLibraryQueryStruct KprLibraryQueryRecord, *KprLibraryQuery;
typedef struct KprLibraryServerStruct KprLibraryServerRecord, *KprLibraryServer;
typedef  struct KprLibrarySessionStruct KprLibrarySessionRecord, *KprLibrarySession;

struct KprLibraryQueryStruct {
	UInt32 index;
	char* info;
	UInt32 kind;
	char* mime;
	char* url;
	char* authorization;
	FskInstrumentedItemDeclaration
};

struct KprLibraryServerStruct {
	FskHTTPServer server;
	FskHTTPServerCallbackVectors vectors;
	FskMutex queryMutex;
	UInt32 queryIndex;
	KprLibraryQuery queries[kQueryCount];
	FskInstrumentedItemDeclaration
};

struct KprLibrarySessionStruct {
	FskHTTPServerRequest request;
	UInt32 kind;
	char* info;
	char* mime;
	char* url;
	char* authorization;

	FskErr error;
	KprURLPartsRecord parts;

	struct {
		void* buffer;
		SInt32 offset;
		SInt32 size;
	} data;

	struct {
		char* path;
		FskFileInfo info;
		FskFile file;
		FskInt64 from;
		FskInt64 to;
	} file;
	
	struct {
		char* location;
		char* buffer;
		FskHTTPClient client;
		FskHTTPClientRequest request;
		UInt32 responseCode;
		char* contentLength;
		char* contentRange;
		UInt32 bufferSize;
		UInt32 readOffset;
		UInt32 writeOffset;
		Boolean done;
	} http;
	
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprLibraryServerNew(KprLibraryServer* it);
FskAPI(void) KprLibraryServerDispose(KprLibraryServer self);

FskAPI(FskErr) KprLibraryServerStart(KprLibraryServer self, char* value);
FskAPI(FskErr) KprLibraryServerStop(KprLibraryServer self);

extern FskErr KprLibrarySessionRedirect(KprLibrarySession session);

FskAPI(FskErr) KprDataServerOpen(KprLibrarySession session);
FskAPI(void) KprDataServerClose(KprLibrarySession session);
FskAPI(void) KprDataServerGenerateResponseHeaders(KprLibrarySession session, FskHeaders *responseHeaders);
FskAPI(FskErr) KprDataServerGenerateResponseBody(KprLibrarySession session, char *data, UInt32 size, UInt32 *generated);

FskAPI(FskErr) KprFileServerOpen(KprLibrarySession session);
FskAPI(void) KprFileServerClose(KprLibrarySession session);
FskAPI(void) KprFileServerGenerateResponseHeaders(KprLibrarySession session, FskHeaders *responseHeaders);
FskAPI(FskErr) KprFileServerGenerateResponseBody(KprLibrarySession session, char *data, UInt32 size, UInt32 *generated);

FskAPI(FskErr) KprProxyServerOpen(KprLibrarySession session);
FskAPI(void) KprProxyServerClose(KprLibrarySession session);
FskAPI(void) KprProxyServerGenerateResponseHeaders(KprLibrarySession session, FskHeaders *responseHeaders);
FskAPI(FskErr) KprProxyServerGenerateResponseBody(KprLibrarySession session, char *data, UInt32 size, UInt32 *generated);

#endif
