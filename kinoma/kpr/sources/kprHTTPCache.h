/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#ifndef __KPRHTTPCACHE__
#define __KPRHTTPCACHE__

#include "kpr.h"
#include "kprStorage.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* HTTP CACHE */

struct KprHTTPCacheStruct {
	KprStorage table;
	FskList disk;
	UInt32 diskSize;
	UInt32 maxDiskSize;
	char* path;
	char* dataPath;
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprHTTPCacheNew(KprHTTPCache* it, UInt32 size, UInt32 diskSize, char* path);
FskAPI(void) KprHTTPCacheDispose(KprHTTPCache self);

FskAPI(void) KprHTTPCacheCleanup(KprHTTPCache self);
FskAPI(void) KprHTTPCacheClear(KprHTTPCache self);
FskAPI(void) KprHTTPCacheDump(KprHTTPCache self);
FskAPI(void) KprHTTPCacheFetch(KprHTTPCache self, char* url, KprHTTPCacheValue* it);
FskAPI(void) KprHTTPCacheGet(KprHTTPCache self, char* url, KprHTTPCacheValue* it);
FskAPI(FskErr) KprHTTPCachePut(KprHTTPCache self, char* url, KprHTTPCacheValue cached);
FskAPI(void) KprHTTPCacheRemove(KprHTTPCache self, char* url);
FskAPI(void) KprHTTPCacheRead(KprHTTPCache self);
FskAPI(FskErr) KprHTTPCacheWrite(KprHTTPCache self);

struct KprHTTPCacheValueStruct {
	struct KprHTTPCacheValueStruct* next;
	KprStorageEntry entry;
	char* path;
	
	void* headers;
	void* body;
	UInt32 size;

	UInt32 age;
	UInt32 date;
	UInt32 lifetime;
	UInt32 requestDate;
	UInt32 responseDate;
	UInt32 status;
	
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprHTTPCacheValueNew(KprHTTPCacheValue* it);
FskAPI(void) KprHTTPCacheValueDispose(KprHTTPCacheValue self);
FskAPI(void) KprHTTPCacheValueCleanupHeaders(KprHTTPCacheValue self);
FskAPI(FskErr) KprHTTPCacheValueDeleteData(KprHTTPCacheValue self);
FskAPI(Boolean) KprHTTPCacheValueIsFresh(KprHTTPCacheValue self);
FskAPI(FskErr) KprHTTPCacheValueReadData(KprHTTPCacheValue self);
FskAPI(void) KprHTTPCacheValueUpdateHeaders(KprHTTPCacheValue self, void* headers);
FskAPI(FskErr) KprHTTPCacheValueWriteData(KprHTTPCacheValue self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
