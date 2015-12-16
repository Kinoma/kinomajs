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
#include "kpr.h"
#include "kprHTTPCache.h"
#include "kprHTTPClient.h"
#include "kprUtilities.h"

#include "FskUUID.h"

/* HTTP CACHE */

#define kprHTTPCacheStorage 'khc4'

//#define kprHTTPCacheDiskSize 0x10000000
#define kprHTTPCacheDiskSize 0x10000

static Boolean KprHTTPCacheValueRemove(void* it);
static FskErr KprHTTPCacheValueRead(void** it, char** data, UInt32* size);
static FskErr KprHTTPCacheValueWrite(void* it, FskFile file);

static KprStorageEntryDispatchRecord kprHTTPCacheValueDispatch = {
	KprHTTPCacheValueRemove,
	KprHTTPCacheValueRead,
	KprHTTPCacheValueWrite
};

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprHTTPCacheInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprHTTPCache", FskInstrumentationOffset(KprHTTPCacheRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprHTTPCacheNew(KprHTTPCache* it, UInt32 size, UInt32 diskSize, char* path)
{
	FskErr err = kFskErrNone;
	KprHTTPCache self;
	char* slash;
	bailIfError(KprMemPtrNewClear(sizeof(KprHTTPCacheRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprHTTPCacheInstrumentation);
	bailIfError(KprStorageNew(&(self->table), kprHTTPCacheStorage, size, &kprHTTPCacheValueDispatch));
	self->maxDiskSize = diskSize;
	self->path = KprStrDoCopy(path);
	bailIfNULL(self->path);
	slash = FskStrRChr(self->path, '/');
	bailIfNULL(slash);
	*slash = 0;
	bailIfError(KprMemPtrNewClear(FskStrLen(self->path) + 8, &self->dataPath));
	FskStrCopy(self->dataPath, self->path);
	*slash = '/';
	FskStrCat(self->dataPath, "/cache/");
	bailIfError(FskFileCreateDirectory(self->dataPath));
bail:
	if (err == kFskErrFileExists)
		err = kFskErrNone;
	return err;
}

void KprHTTPCacheDispose(KprHTTPCache self)
{
	if (self) {
		KprStorageDispose(self->table);
		KprMemPtrDisposeAt(&self->path);
		KprMemPtrDisposeAt(&self->dataPath);
		FskInstrumentedItemDispose(self);
		KprMemPtrDispose(self);
	}
}

void KprHTTPCacheCleanup(KprHTTPCache self)
{
	KprHTTPCacheValue cached;
	
	while (self->disk && (self->diskSize > self->maxDiskSize)) {
		cached = self->disk;
		while (cached->next) cached = cached->next;
		KprHTTPCacheRemove(self, cached->entry->key);
	}
#if kprDumpCache
	fprintf(stderr, "HTTP CACHE CLEANUP\n");
	KprHTTPCacheDump(self);
#endif
}

void KprHTTPCacheClear(KprHTTPCache self)
{
	UInt32 maxDiskSize = self->maxDiskSize;
	self->maxDiskSize = 0;
	KprHTTPCacheCleanup(self);
	self->maxDiskSize = maxDiskSize;
}

void KprHTTPCacheDump(KprHTTPCache self)
{
	KprHTTPCacheValue cached;
	fprintf(stderr, "HTTP CACHE (%u items)\n", (unsigned int)self->table->count);
	fprintf(stderr, "  DISK (%u bytes)\n", (unsigned int)self->diskSize);
	cached = self->disk;
	while (cached) {
		fprintf(stderr, "  - %08x %s (%u bytes)\n", (unsigned int)cached, cached->entry->key, (unsigned int)cached->size);
		cached = cached->next;
	}
}

void KprHTTPCacheFetch(KprHTTPCache self, char* url, KprHTTPCacheValue* it)
{
	KprHTTPCacheValue cached;
	KprStorageGet(self->table, url, (void**)it);
	cached = *it;
	if (cached) {
		FskListRemove(&self->disk, cached);
		FskListPrepend(&self->disk, cached);
#if kprDumpCache
		fprintf(stderr, "HTTP CACHE GET\n");
		KprHTTPCacheDump(self);
#endif
	}
}

void KprHTTPCacheGet(KprHTTPCache self, char* url, KprHTTPCacheValue* it)
{
	KprHTTPCacheValue cached;
	KprStorageGet(self->table, url, (void**)it);
	cached = *it;
	if (cached) {
		FskListRemove(&self->disk, cached);
		KprHTTPCacheValueReadData(cached);
		FskListPrepend(&self->disk, cached);
#if kprDumpCache
		fprintf(stderr, "HTTP CACHE GET\n");
		KprHTTPCacheDump(self);
#endif
	}
}

FskErr KprHTTPCachePut(KprHTTPCache self, char* url, KprHTTPCacheValue cached)
{
	FskErr err = kFskErrNone;
	KprStorageEntry entry = NULL;
	FskUUIDRecord uuid;
	char* uuidString = NULL;
	KprHTTPCacheRemove(self, url);
	bailIfError(KprStorageEntryNew(&entry, url, cached));
	cached->entry = entry;
	FskUUIDCreate(&uuid);
	uuidString = FskUUIDtoString_844412(&uuid);
	bailIfError(KprMemPtrNewClear(FskStrLen(self->dataPath) + 38, &cached->path));
	FskStrCopy(cached->path, self->dataPath);
	FskStrCat(cached->path, uuidString);
	FskMemPtrDispose(uuidString);
	if (cached->size < self->maxDiskSize) {
		bailIfError(KprHTTPCacheValueWriteData(cached));
	}
	self->diskSize += cached->size;
	FskListPrepend(&self->disk, cached);
	bailIfError(KprStoragePutEntry(self->table, entry));
#if kprDumpCache
	fprintf(stderr, "HTTP CACHE PUT\n");
	KprHTTPCacheDump(self);
#endif
	KprHTTPCacheCleanup(self);
	KprHTTPCacheWrite(self);
	return err;
bail:
	KprHTTPCacheValueDeleteData(cached);
	KprStorageEntryDispose(entry);
	FskMemPtrDispose(uuidString);
	return err;
}

void KprHTTPCacheRemove(KprHTTPCache self, char* url)
{
	KprStorageEntry entry;
	if (KprStorageGetFirstEntry(self->table, url, &entry)) {
		KprHTTPCacheValue cached = entry->value;
		if (FskListRemove(&self->disk, cached)) {
			self->diskSize -= cached->size;
		}
		KprHTTPCacheValueDeleteData(cached);
		KprStorageRemoveEntry(self->table, entry);
#if kprDumpCache
		fprintf(stderr, "HTTP CACHE REMOVE\n");
		KprHTTPCacheDump(self);
#endif
	}
}

void KprHTTPCacheRead(KprHTTPCache self)
{
	FskErr err = kFskErrNone;
	KprStorage storage;
	KprStorageEntry entry;
	KprHTTPCacheValue cached = NULL;
	char* path = NULL;
	char* name = NULL;
	FskDirectoryIterator iterator = NULL;
	UInt32 i;
	bailIfError(!self->path);

	if (KprStorageRead(&(self->table), self->path)) {
		// new cache version or cache description error: cleanup cache directory
		UInt32 itemType;

		bailIfError(FskMemPtrNewClear(FskStrLen(self->dataPath) + 37, &path));
		
		bailIfError(FskDirectoryIteratorNew(self->dataPath, &iterator, 0));
		while (FskDirectoryIteratorGetNext(iterator, &name, &itemType) == kFskErrNone) {
			if (name[0] == '.')
				continue;
			if (kFskDirectoryItemIsFile == itemType) {
				bailIfError(FskMemPtrRealloc(FskStrLen(self->dataPath) + FskStrLen(name) + 1, &path));
				FskStrCopy(path, self->dataPath);
				FskStrCat(path, name);
				FskFileDelete(path);
				// does not work without this when deleting file
				FskDirectoryIteratorDispose(iterator);
				iterator = NULL;
				bailIfError(FskDirectoryIteratorNew(self->dataPath, &iterator, 0));
			}
			else if (kFskDirectoryItemIsDirectory == itemType) {
				// skip directories
			}
			FskMemPtrDisposeAt(&name);
		}
	}
	else {
		storage = self->table;
		for (i = 0; i < storage->size; i++) {
			if ((entry = storage->entries[i])) {
				while (entry) {
					if (cached)
						FskListInsertAfter(&self->disk, entry->value, cached);
					else
						FskListAppend(&self->disk, entry->value);
					cached = entry->value;
					cached->entry = entry;
					self->diskSize += cached->size;
					entry = entry->next;
				}
			}
		}
	}
#if kprDumpCache
	fprintf(stderr, "HTTP CACHE READ\n");
	KprHTTPCacheDump(self);
#endif
bail:
	FskMemPtrDispose(name);
	FskMemPtrDispose(path);
	FskDirectoryIteratorDispose(iterator);
	return;
}

FskErr KprHTTPCacheWrite(KprHTTPCache self)
{
	FskErr err = kFskErrNone;
	bailIfError(!self->path);
	bailIfError(KprStorageWrite(self->table, self->path));
#if kprDumpCache
	fprintf(stderr, "HTTP CACHE WRITE\n");
//	FskInstrumentedItemSendMessageDebug(self, kprInstrumentedHTTPCacheWrite, self)
	KprHTTPCacheDump(self);
#endif
bail:
	return err;
}

/* KprHTTPCacheValue */

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprHTTPCacheValueInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprHTTPCacheValue", FskInstrumentationOffset(KprHTTPCacheValueRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprHTTPCacheValueNew(KprHTTPCacheValue* it)
{
	FskErr err = kFskErrNone;
	KprHTTPCacheValue self;
	bailIfError(KprMemPtrNewClear(sizeof(KprHTTPCacheValueRecord), &self));
	FskInstrumentedItemNew(self, NULL, &KprHTTPCacheValueInstrumentation);
	*it = self;
bail:
	return err;
}

void KprHTTPCacheValueDispose(KprHTTPCacheValue self)
{
	if (self) {
		FskInstrumentedItemSendMessageDebug(self, kprInstrumentedHTTPCacheValueDisposeData, self)
		FskAssociativeArrayDispose(self->headers);
	//	KprMemPtrDisposeAt(&self->body); // cache never owns the body
		KprMemPtrDisposeAt(&self->path);
		FskInstrumentedItemDispose(self);
		KprMemPtrDispose(self);
	}
}

void KprHTTPCacheValueCleanupHeaders(KprHTTPCacheValue self)
{
	// RFC 2616 - 13.5.1 End-to-end and Hop-by-hop Headers
	FskAssociativeArrayNameList list, last = NULL;
	FskAssociativeArray array = self->headers;
	
	list = array->arrayHead;
	while (list) {
		if ((FskStrCompareCaseInsensitive("Connection", list->name) == 0)
			|| (FskStrCompareCaseInsensitive("Keep-Alive", list->name) == 0)
			|| (FskStrCompareCaseInsensitive("Proxy-Authenticate", list->name) == 0)
			|| (FskStrCompareCaseInsensitive("Proxy-Authorization", list->name) == 0)
			|| (FskStrCompareCaseInsensitive("TE", list->name) == 0)
			|| (FskStrCompareCaseInsensitive("Trailers", list->name) == 0)
			|| (FskStrCompareCaseInsensitive("Transfer-Encoding", list->name) == 0)
			|| (FskStrCompareCaseInsensitive("Upgrade", list->name) == 0)) {
			// kill existing list
			if (last)
				last->next = list->next;
			else
				array->arrayHead = list->next;
			
			FskMemPtrDispose(list);

			break; // while(list)
		}
		last = list;
		list = list->next;
	}
}

FskErr KprHTTPCacheValueDeleteData(KprHTTPCacheValue self)
{
	FskErr err = kFskErrNone;
	FskFileInfo info;
	if (self) {
		if (kFskErrNone == FskFileGetFileInfo(self->path, &info))
			bailIfError(FskFileDelete(self->path));
	}
bail:
	return err;
}

Boolean KprHTTPCacheValueIsFresh(KprHTTPCacheValue self)
{
	// RFC 2616 - 13.2.3 Age Calculations
	UInt32 age = self->age;
	UInt32 date = self->date;
	UInt32 request = self->requestDate;
	UInt32 response = self->responseDate;
	UInt32 now = KprDateNow();
	UInt32 apparentAge; 
	
	if (!date) date = response;
	apparentAge = (response < date) ? 0 : response - date;
	age = (apparentAge > age) ? apparentAge : age;
	age += now - request;
	// RFC 2616 - 13.2.4 Expiration Calculations
	return (self->lifetime > age);
}

static FskErr KprHTTPCacheValueRead(void** it, char** data, UInt32 *size)
{
	FskErr err = kFskErrNone;
	KprHTTPCacheValue self = NULL;
	UInt32 count;
	char* name = NULL;
	char* value = NULL;
	
	bailIfError(KprHTTPCacheValueNew(&self));
	bailIfError(KprStorageReadUInt32(data, size, &self->age));
	bailIfError(KprStorageReadUInt32(data, size, &self->date));
	bailIfError(KprStorageReadUInt32(data, size, &self->lifetime));
	bailIfError(KprStorageReadUInt32(data, size, &self->requestDate));
	bailIfError(KprStorageReadUInt32(data, size, &self->responseDate));
	bailIfError(KprStorageReadUInt32(data, size, &self->status));
	bailIfError(KprStorageReadUInt32(data, size, &self->size));
	bailIfError(KprStorageReadString(data, size, &self->path));
	
	bailIfError(KprStorageReadUInt32(data, size, &count));
	if (count) {
		self->headers = FskAssociativeArrayNew();
		for (; count > 0; count--) {
			bailIfError(KprStorageReadString(data, size, &name));
			bailIfError(KprStorageReadString(data, size, &value));
			FskAssociativeArrayElementSet(self->headers, (const char*)name, (const char*)value, 0, kFskStringType);
			KprMemPtrDisposeAt(&value);
			KprMemPtrDisposeAt(&name);
		}
	}

	*it = self;
bail:
	KprMemPtrDispose(value);
	KprMemPtrDispose(name);
	return err;
}

FskErr KprHTTPCacheValueReadData(KprHTTPCacheValue self)
{
	FskErr err = kFskErrNone;
	FskFile file = NULL;
	UInt32 read = 0;
	bailIfError(FskFileOpen(self->path, kFskFilePermissionReadOnly, &file));
	bailIfError(KprMemPtrNew(self->size, &self->body));
	bailIfError(FskFileRead(file, self->size, self->body, &read));
	bailIfError(self->size != read);
bail:
	FskFileClose(file);
	if (err)
		KprMemPtrDisposeAt(&self->body);
	return err;
}

static Boolean KprHTTPCacheValueRemove(void* it)
{
	KprHTTPCacheValue self = it;
	self->entry = NULL;
	KprHTTPCacheValueDispose(self);
		
	return true;
}

void KprHTTPCacheValueUpdateHeaders(KprHTTPCacheValue self, void* headers)
{
	FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(headers);
//	fprintf(stderr, "%p: CACHE UPDATE %s\n", self, self->path);
	while (iterate) {
//		fprintf(stderr, "   %s: %s\n", iterate->name, iterate->value);
		FskAssociativeArrayElementSet(self->headers, iterate->name, iterate->value, 0, kFskStringType);
		iterate = FskAssociativeArrayIteratorNext(iterate);
	}
	FskAssociativeArrayIteratorDispose(iterate);
	KprHTTPCacheValueCleanupHeaders(self);
}

static FskErr KprHTTPCacheValueWrite(void* it, FskFile file)
{
	FskErr err = kFskErrNone;
	KprHTTPCacheValue self = it;
	bailIfError(KprStorageWriteUInt32(file, self->age));
	bailIfError(KprStorageWriteUInt32(file, self->date));
	bailIfError(KprStorageWriteUInt32(file, self->lifetime));
	bailIfError(KprStorageWriteUInt32(file, self->requestDate));
	bailIfError(KprStorageWriteUInt32(file, self->responseDate));
	bailIfError(KprStorageWriteUInt32(file, self->status));
	bailIfError(KprStorageWriteUInt32(file, self->size));
	bailIfError(KprStorageWriteString(file, self->path));
	{
		UInt32 count = 0;
		FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(self->headers);
		while (iterate) {
			count++;
			iterate = FskAssociativeArrayIteratorNext(iterate);
		}
		FskAssociativeArrayIteratorDispose(iterate);
		bailIfError(KprStorageWriteUInt32(file, count));
		iterate = FskAssociativeArrayIteratorNew(self->headers);
		while (iterate) {
			bailIfError(KprStorageWriteString(file, iterate->name));
			bailIfError(KprStorageWriteString(file, iterate->value));
			iterate = FskAssociativeArrayIteratorNext(iterate);
		}
		FskAssociativeArrayIteratorDispose(iterate);
	}
bail:
	return err;
}

FskErr KprHTTPCacheValueWriteData(KprHTTPCacheValue self)
{
	FskErr err = kFskErrNone;
	FskFile file = NULL;
	UInt32 written = 0;
	FskFileInfo info;
	
	if (kFskErrNone == FskFileGetFileInfo(self->path, &info))
		bailIfError(FskFileDelete(self->path));
	bailIfError(FskFileCreate(self->path));
	bailIfError(FskFileOpen(self->path, kFskFilePermissionReadWrite, &file));
	bailIfError(FskFileWrite(file, self->size, self->body, &written));
	bailIfError(self->size != written);
bail:
	bailIfError(FskFileClose(file));
	return err;
}
