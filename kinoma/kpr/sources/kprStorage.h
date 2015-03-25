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
#ifndef __KPRSTORAGE__
#define __KPRSTORAGE__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef Boolean (*KprStorageEntryRemoveProc)(void* it);
typedef FskErr (*KprStorageEntryReadProc)(void** it, char** data, UInt32* size);
typedef FskErr (*KprStorageEntryWriteProc)(void* it, FskFile file);

struct KprStorageEntryDispatchStruct {
	KprStorageEntryRemoveProc remove;
	KprStorageEntryReadProc read;
	KprStorageEntryWriteProc write;
};

struct KprStorageStruct {
	KprStorageEntryDispatch dispatch;
	UInt32 type;
	UInt32 size;
	UInt32 count;
	KprStorageEntry entries[197];
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprStorageNew(KprStorage* it, UInt32 type, UInt32 size, KprStorageEntryDispatch dispatch);
FskAPI(void) KprStorageDispose(KprStorage self);
FskAPI(void) KprStorageClear(KprStorage self);
FskAPI(Boolean) KprStorageGet(KprStorage self, char* key, void** data);
FskAPI(Boolean) KprStorageGetFirstEntry(KprStorage self, char* key, KprStorageEntry* entry);
FskAPI(Boolean) KprStorageGetNextEntry(KprStorage self, KprStorageEntry* entry);
FskAPI(FskErr) KprStoragePut(KprStorage self, char* key, void* data);
FskAPI(FskErr) KprStoragePutEntry(KprStorage self, KprStorageEntry entry);
FskAPI(void) KprStorageRemove(KprStorage self,  char* key, void* data);
FskAPI(void) KprStorageRemoveEntry(KprStorage self, KprStorageEntry entry);
FskAPI(FskErr) KprStorageRead(KprStorage *it, char* path);
FskAPI(FskErr) KprStorageReadBoolean(char** data, UInt32* size, Boolean* value);
FskAPI(FskErr) KprStorageReadString(char** data, UInt32* size, char** string);
FskAPI(FskErr) KprStorageReadUInt32(char** data, UInt32* size, UInt32* value);
FskAPI(FskErr) KprStorageWrite(KprStorage self, char* path);
FskAPI(FskErr) KprStorageWriteBoolean(FskFile file, Boolean value);
FskAPI(FskErr) KprStorageWriteString(FskFile file, char* string);
FskAPI(FskErr) KprStorageWriteUInt32(FskFile file, UInt32 value);

struct KprStorageEntryStruct {
	KprStorageEntry next;
	UInt32 sum;
	char* key;
	void* value;
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprStorageEntryNew(KprStorageEntry *it, char* key, void* value);
FskAPI(void) KprStorageEntryDispose(KprStorageEntry self);
FskAPI(UInt32) KprStorageEntrySum(char* key);

struct KprLocalStorageStruct {
	KprStorage storage;
	char* path;
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprLocalStorageNew(char* path, char* url);
FskAPI(FskErr) KprLocalStorageDispose();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
