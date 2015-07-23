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
#include "kprStorage.h"
#include "kprURL.h"
#include "kprUtilities.h"

//--------------------------------------------------
// INSTRUMENTATION
//--------------------------------------------------

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord gKprStorageInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprStorage", FskInstrumentationOffset(KprStorageRecord), NULL, 0, NULL, NULL, NULL, 0 };
static FskInstrumentedTypeRecord gKprStorageEntryInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprStorageEntry", FskInstrumentationOffset(KprStorageEntryRecord), NULL, 0, NULL, NULL, NULL, 0 };
#endif

//--------------------------------------------------
// Storage
//--------------------------------------------------

FskErr KprStorageNew(KprStorage* it, UInt32 type, UInt32 size, KprStorageEntryDispatch dispatch)
{
	FskErr err = kFskErrNone;
	KprStorage self;
	bailIfError(KprMemPtrNewClear(sizeof(KprStorageRecord) + ((size - 1) * sizeof(KprStorageEntry)), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &gKprStorageInstrumentation);
	self->dispatch = dispatch;
	self->type = type;
	self->size = size;
bail:
	return err;
}

void KprStorageDispose(KprStorage self)
{
	if (self) {
		KprStorageClear(self);
		FskInstrumentedItemDispose(self);
		KprMemPtrDispose(self);
	}
}

void KprStorageClear(KprStorage self)
{
	UInt32 c = self->size, i;
	for (i = 0; i < c; i++) {
		KprStorageEntry entry = self->entries[i];
		while (entry) {
			KprStorageEntry next = entry->next;
			(*self->dispatch->remove)(entry->value);
			KprStorageEntryDispose(entry);
			entry = next;
		}
		self->entries[i] = NULL;
	}
	self->count = 0;
}

Boolean KprStorageGet(KprStorage self, char* key, void** data)
{
	*data = NULL;
	if (key) {
		UInt32 sum = KprStorageEntrySum(key);
		KprStorageEntry entry = self->entries[sum % self->size];
		while (entry) {
			if ((entry->sum == sum) && entry->key && !FskStrCompare(entry->key, key)) {
				*data = entry->value;
				return true;
			}
			entry = entry->next;
		}
	}
	return false;
}

Boolean KprStorageGetFirstEntry(KprStorage self, char* key, KprStorageEntry* it)
{
	*it = NULL;
	if (key) {
		UInt32 sum = KprStorageEntrySum(key);
		KprStorageEntry entry = self->entries[sum % self->size];
		while (entry) {
			if ((entry->sum == sum) && entry->key && !FskStrCompare(entry->key, key)) {
				*it = entry;
				return true;
			}
			entry = entry->next;
		}
	}
	return false;
}

Boolean KprStorageGetNextEntry(KprStorage self UNUSED, KprStorageEntry* it)
{
	if (*it) {
		KprStorageEntry entry = *it;
		UInt32 sum = entry->sum;
		char* key = entry->key;
		*it = NULL;
		while (entry->next) {
			entry = entry->next;
			if ((entry->sum == sum) && entry->key && !FskStrCompare(entry->key, key)) {
				*it = entry;
				return true;
			}
		}
	}
	return false;
}

FskErr KprStoragePut(KprStorage self, char* key, void* data)
{
	FskErr err = kFskErrNone;
	KprStorageEntry entry, *address;
	bailIfError(KprStorageEntryNew(&entry, key, data));
	address = self->entries + (entry->sum % self->size);
	entry->next = *address;
	*address = entry;
	self->count += 1;
bail:
	return err;
}

FskErr KprStoragePutEntry(KprStorage self, KprStorageEntry entry)
{
	FskErr err = kFskErrNone;
	KprStorageEntry *address;
	address = self->entries + (entry->sum % self->size);
	entry->next = *address;
	*address = entry;
	self->count += 1;
//bail:
	return err;
}

void KprStorageRemove(KprStorage self,  char* key, void* data)
{
	KprStorageEntry entry = NULL, *address;
	UInt32 sum = KprStorageEntrySum(key);
	address = self->entries + (sum % self->size);
	while (*address) {
		entry = *address;
		if (entry->value == data) {
			if ((*self->dispatch->remove)(entry->value)) {
				*address = entry->next;
				KprStorageEntryDispose(entry);
				self->count -= 1;
				return;
			}
		}
		address = &entry->next;
	}
}

void KprStorageRemoveEntry(KprStorage self, KprStorageEntry entry)
{
	KprStorageEntry *address;
	address = self->entries + (entry->sum % self->size);
	while (*address) {
		if (*address == entry) {
			if ((*self->dispatch->remove)(entry->value)) {
				*address = entry->next;
				KprStorageEntryDispose(entry);
				self->count -= 1;
				return;
			}
		}
		address = &((*address)->next);
	}
}

FskErr KprStorageRead(KprStorage *it, char* path)
{
	FskErr err = kFskErrNone;
	FskFileMapping map = NULL;
	unsigned char *mapData;
	FskInt64 dataSize;
	char *data;
	UInt32 type;
	UInt32 size;
	UInt32 i;
	KprStorageEntry entry = NULL, *address;
	KprStorage self = *it;
	
	ASSERT(path);
	bailIfError(FskFileMap(path, &mapData, &dataSize, 0, &map));
	data = (char*)mapData;
	size = (UInt32)dataSize;

	bailIfError(KprStorageReadUInt32(&data, &size, &type));
	bailIfError(self->type != type);
	bailIfError(KprStorageReadUInt32(&data, &size, &self->count));

	for (i = 0; i < self->count; i++) {
		bailIfError(KprMemPtrNewClear(sizeof(KprStorageEntryRecord), &entry));
		bailIfError(KprStorageReadUInt32(&data, &size, &entry->sum));
		bailIfError(KprStorageReadString(&data, &size, &entry->key));
		bailIfError((*self->dispatch->read)(&(entry->value), &data, &size));
		address = self->entries + (entry->sum % self->size);
		entry->next = *address;
		*address = entry;
		entry = NULL;
	}
	if (size) err = kFskErrBadData;
bail:
	KprStorageEntryDispose(entry);
	FskFileDisposeMap(map);
	if (err)
		KprStorageClear(self);
	return err;
}

FskErr KprStorageReadBoolean(char** data, UInt32* size, Boolean* value)
{
	FskErr err = kFskErrNone;
	if (*size < sizeof(UInt32)) return kFskErrBadData;
	*value = (Boolean)*((UInt32*)*data);
	*data += sizeof(UInt32);
	*size -= sizeof(UInt32);
	return err;
}

FskErr KprStorageReadString(char** data, UInt32* size, char** string)
{
	FskErr err = kFskErrNone;
	UInt32 length, offset, pad;
	UInt32 remaining = *size;
	char* ptr = (char*)*data;

	while (remaining && *ptr) {
		ptr++;
		remaining--;
	}
	if (!remaining) return kFskErrBadData;
	length = *size - remaining + 1;
	pad = length % 4;
	offset = length + ((pad) ? 4 - pad : 0);
	if (*size < offset) return kFskErrBadData;
	bailIfError(FskMemPtrNewFromData(length, *data, string));
	*data += offset;
	*size -= offset;
bail:
	return err;
}

FskErr KprStorageReadUInt32(char** data, UInt32* size, UInt32* value)
{
	FskErr err = kFskErrNone;
	if (*size < sizeof(UInt32)) return kFskErrBadData;
	*value = *((UInt32*)*data);
	*data += sizeof(UInt32);
	*size -= sizeof(UInt32);
	return err;
}

FskErr KprStorageWrite(KprStorage self, char* path)
{
	FskErr err = kFskErrNone;
	FskFile file = NULL;
	UInt32 i;
	KprStorageEntry entry;
	
	FskFileDelete(path);
	FskFileCreate(path);
	bailIfError(FskFileOpen(path, kFskFilePermissionReadWrite, &file));
	
	bailIfError(KprStorageWriteUInt32(file, self->type));
	bailIfError(KprStorageWriteUInt32(file, self->count));
	for (i = 0; i < self->size; i++) {
		if ((entry = self->entries[i])) {
			while (entry) {
				bailIfError(KprStorageWriteUInt32(file, entry->sum));
				bailIfError(KprStorageWriteString(file, entry->key));
				(*self->dispatch->write)(entry->value, file);
				entry = entry->next;
			}
		}
	}
	
bail:
	FskFileClose(file);
	return err;
}

FskErr KprStorageWriteBoolean(FskFile file, Boolean value)
{
	FskErr err = kFskErrNone;
	UInt32 data = value;
	bailIfError(FskFileWrite(file, sizeof(UInt32), &data, NULL));
bail:
	return err;
}

FskErr KprStorageWriteString(FskFile file, char* string)
{
	FskErr err = kFskErrNone;
	UInt32 zero = 0;
	if (string) {
		UInt32 size = FskStrLen(string) + 1;
		UInt32 pad = size % 4;
		bailIfError(FskFileWrite(file, size, string, NULL));
		if (pad) {
			bailIfError(FskFileWrite(file, 4 - pad, &zero, NULL));
		}
	}
	else {
		bailIfError(FskFileWrite(file, 4, &zero, NULL));
	}
bail:
	return err;
}

FskErr KprStorageWriteUInt32(FskFile file, UInt32 value)
{
	FskErr err = kFskErrNone;
	bailIfError(FskFileWrite(file, sizeof(UInt32), &value, NULL));
bail:
	return err;
}

//--------------------------------------------------
// Storage Entry
//--------------------------------------------------

FskErr KprStorageEntryNew(KprStorageEntry *it, char* key, void* value)
{
	FskErr err = kFskErrNone;
	KprStorageEntry self = NULL;
	bailIfError(KprMemPtrNewClear(sizeof(KprStorageEntryRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &gKprStorageEntryInstrumentation);
	self->key = KprStrDoCopy(key);
	bailIfNULL(self->key);
	self->sum = KprStorageEntrySum(key);
	self->value = value;
bail:
	if (err)
		KprStorageEntryDispose(self);
	return err;
}

void KprStorageEntryDispose(KprStorageEntry self)
{
	if (self) {
		KprMemPtrDisposeAt(&self->key);
		FskInstrumentedItemDispose(self);
		KprMemPtrDispose(self);
	}
}

UInt32 KprStorageEntrySum(char* key)
{
	UInt32 sum = 0;
	if (key) {
		unsigned char* p = (unsigned char*)key;
		while (*p) {
			sum = (sum << 1) + *p++;
		}
	}
	return sum;
}

//--------------------------------------------------
// Local Storage
//--------------------------------------------------

#define kLocalStorage 'klo1'

static Boolean KprLocalStorageValueRemove(void* it)
{
	KprMemPtrDispose(it);
	return true;
}

static FskErr KprLocalStorageValueRead(void** it, char** data, UInt32* size UNUSED)
{
	FskErr err = kFskErrNone;
	bailIfError(KprStorageReadString(data, size, (char**)it));
bail:
	return err;
}

static FskErr KprLocalStorageValueWrite(void* it, FskFile file)
{
	FskErr err = kFskErrNone;
	bailIfError(KprStorageWriteString(file, (char*)it));
bail:
	return err;
}

static KprStorageEntryDispatchRecord kprLocalStorageDispatch = {
	KprLocalStorageValueRemove,
	KprLocalStorageValueRead,
	KprLocalStorageValueWrite
};

typedef struct KprLocalStorageStruct KprLocalStorageRecord, *KprLocalStorage;

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord gKprLocalStorageInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprLocalStorage", FskInstrumentationOffset(KprLocalStorageRecord), NULL, 0, NULL, NULL, NULL, 0 };
#endif

static KprLocalStorage gLocalStorage = NULL;

#define kprLocalStorageName "kpr.storage"

FskErr KprLocalStorageNew(char* path, char* url)
{
	FskErr err = kFskErrNone;
	KprURLPartsRecord parts;
	KprLocalStorage self = NULL;
	char* end = NULL;
	char c;
	KprURLSplit(url, &parts);
	
	if (gLocalStorage)
		KprLocalStorageDispose();
	
	bailIfError(KprMemPtrNewClear(sizeof(KprLocalStorageRecord), &self));
	FskInstrumentedItemNew(self, NULL, &gKprLocalStorageInstrumentation);

	if (parts.authorityLength) {
		UInt32 size = FskStrLen(kprLocalStorageName) + FskStrLen(path) + 1;
		end = parts.authority + parts.authorityLength;
		c = *end;
		*end = 0;
		size += parts.authorityLength + 1;
		bailIfError(KprURLMerge(path, parts.authority, &self->path));
		bailIfError(FskMemPtrRealloc(size, &self->path));
		FskStrCat(self->path, "/");
		FskStrCat(self->path, kprLocalStorageName);
		*end = c;
	}
	else {
		bailIfError(KprURLMerge(path, kprLocalStorageName, &self->path));
	}
	
	bailIfError(KprStorageNew(&self->storage, kLocalStorage, KprEnvironmentGetUInt32("httpLocalStorageSize", 197), &kprLocalStorageDispatch));
	(void)KprStorageRead(&self->storage, self->path);
	gLocalStorage = self;
	return err;
bail:
	if (self) {
		FskMemPtrDisposeAt(self->path);
		KprStorageDispose(self->storage);
		KprMemPtrDispose(self);
	}
	return err;
}

FskErr KprLocalStorageDispose()
{
	FskErr err = kFskErrNone;
	KprLocalStorage self = gLocalStorage;
    if (!self)
        return err;

	bailIfError(KprStorageWrite(self->storage, self->path));
bail:
	FskMemPtrDisposeAt(&self->path);
	KprStorageDispose(self->storage);
	FskInstrumentedItemDispose(gLocalStorage);
	KprMemPtrDisposeAt(&gLocalStorage);
	return err;
}

//--------------------------------------------------
// XS
//--------------------------------------------------

void KPR_localStorage_clear(xsMachine *the UNUSED)
{
//	KprStorage self = xsGetHostData(xsThis);
	KprStorage self = gLocalStorage->storage;
	KprStorageClear(self);
}

void KPR_localStorage_getItem(xsMachine *the)
{
	char* value = NULL;
//	KprStorage self = xsGetHostData(xsThis);
	KprStorage self = gLocalStorage->storage;
	KprStorageGet(self, xsToString(xsArg(0)), (void**)&value);
	if (value)
		xsResult = xsString(value);
	else
		xsResult = xsNull;
}

void KPR_localStorage_getLength(xsMachine *the)
{
//	KprStorage self = xsGetHostData(xsThis);
	KprStorage self = gLocalStorage->storage;
	xsResult = xsInteger(self->count);
}

void KPR_localStorage_key(xsMachine *the)
{
//	KprStorage self = xsGetHostData(xsThis);
	KprStorage self = gLocalStorage->storage;
	UInt32 c = self->size, i;
	UInt32 index = xsToInteger(xsArg(0));
	KprStorageEntry entry = NULL;
	if (index < self->count) {
		for (i = 0; i < c; i++) {
			entry = self->entries[i];
			while (entry) {
				KprStorageEntry next = entry->next;
				if (!index)
					goto bail;
				KprStorageEntryDispose(entry);
				entry = next;
				index--;
			}
		}
	}
bail:
    if (entry)
        xsResult = xsString(entry->key);
}

void KPR_localStorage_removeItem(xsMachine *the)
{
	KprStorageEntry entry;
//	KprStorage self = xsGetHostData(xsThis);
	KprStorage self = gLocalStorage->storage;
	if (KprStorageGetFirstEntry(self, xsToString(xsArg(0)), &entry))
		KprStorageRemoveEntry(self, entry);
}

void KPR_localStorage_setItem(xsMachine *the)
{
	KprStorageEntry entry;
//	KprStorage self = xsGetHostData(xsThis);
	KprStorage self = gLocalStorage->storage;
	char* key = xsToString(xsArg(0));
	char* data = KprStrDoCopy(xsToString(xsArg(1)));
	xsThrowIfNULL(data);
	if (KprStorageGetFirstEntry(self, key, &entry))
		KprStorageRemoveEntry(self, entry);
	KprStoragePut(self, key, data);
}


