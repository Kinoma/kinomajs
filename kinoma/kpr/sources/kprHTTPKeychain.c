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
#include "kprHTTPKeychain.h"
#include "kprHTTPClient.h"
#include "kprURL.h"
#include "kprUtilities.h"

typedef struct KprHTTPKeychainValueStruct KprHTTPKeychainValueRecord, *KprHTTPKeychainValue;
struct KprHTTPKeychainValueStruct {
	char* user;
	char* password;
};

static FskErr KprHTTPKeychainKeyNew(char** key, char* host, char* realm);

static FskErr KprHTTPKeychainValueNew(KprHTTPKeychainValue* it, char* user, char* password);
static void KprHTTPKeychainValueDispose(KprHTTPKeychainValue self);
static Boolean KprHTTPKeychainValueRemove(void* it);
static FskErr KprHTTPKeychainValueRead(void** it, char** data, UInt32* size);
static FskErr KprHTTPKeychainValueWrite(void* it, FskFile file);

/* KprHTTPKeychain */

#define kprHTTPKeychainStorage 'kke2'

static KprStorageEntryDispatchRecord kprHTTPKeychainValueDispatch = {
	KprHTTPKeychainValueRemove,
	KprHTTPKeychainValueRead,
	KprHTTPKeychainValueWrite
};

FskErr KprHTTPKeychainNew(KprHTTPKeychain* it, UInt32 size, char* path)
{
	FskErr err = kFskErrNone;
	KprHTTPKeychain self;
	bailIfError(KprMemPtrNewClear(sizeof(KprHTTPKeychainRecord), it));
	self = *it;
	bailIfError(KprStorageNew(&(self->table), kprHTTPKeychainStorage, size, &kprHTTPKeychainValueDispatch));
	self->path = KprStrDoCopy(path);
	bailIfNULL(self->path);
bail:
	return err;
}

void KprHTTPKeychainDispose(KprHTTPKeychain self)
{
	if (self) {
		KprStorageDispose(self->table);
		KprMemPtrDisposeAt(&self->path);
		KprMemPtrDispose(self);
	}
}

void KprHTTPKeychainClear(KprHTTPKeychain self)
{
	KprStorageClear(self->table);
}

void KprHTTPKeychainGet(KprHTTPKeychain self, char* host, char* realm, char** user, char** password)
{
	FskErr err = kFskErrNone;
	char* key = NULL;
	KprHTTPKeychainValue value = NULL;
	bailIfError(KprHTTPKeychainKeyNew(&key, host, realm));
	if (KprStorageGet(self->table, key, (void**)&value)) {
		*user = FskStrDoCopy(value->user);
		bailIfNULL(*user);
		*password = FskStrDoCopy(value->password);
		bailIfNULL(*password);
	}
	else {
		*user = NULL;
		*password = NULL;
	}
bail:
	FskMemPtrDispose(key);			
}

FskErr KprHTTPKeychainPut(KprHTTPKeychain self, char* host, char* realm, char* user, char* password)
{
	FskErr err = kFskErrNone;
	char* key = NULL;
	KprStorageEntry entry;
	KprHTTPKeychainValue value = NULL;
	bailIfError(KprHTTPKeychainKeyNew(&key, host, realm));
	bailIfError(KprHTTPKeychainValueNew(&value, user, password));
	if (KprStorageGetFirstEntry(self->table, key, &entry))
		KprStorageRemoveEntry(self->table, entry);
	bailIfError(KprStoragePut(self->table, key, value));
bail:
	if (err && value)
		KprHTTPKeychainValueDispose(value);
	FskMemPtrDispose(key);			
	return err;
}

void KprHTTPKeychainRemove(KprHTTPKeychain self, char* host, char* realm)
{
	FskErr err = kFskErrNone;
	char* key = NULL;
	KprStorageEntry entry;
	bailIfError(KprHTTPKeychainKeyNew(&key, host, realm));
	if (KprStorageGetFirstEntry(self->table, key, &entry))
		KprStorageRemoveEntry(self->table, entry);
bail:
	FskMemPtrDispose(key);			
}

void KprHTTPKeychainRead(KprHTTPKeychain self)
{
	(void)KprStorageRead(&(self->table), self->path);
}

FskErr KprHTTPKeychainWrite(KprHTTPKeychain self)
{
	FskErr err = kFskErrNone;
	bailIfError(!self->path);
	bailIfError(KprStorageWrite(self->table, self->path));
bail:
	return err;
}

/* KprHTTPKeychainKey */

FskErr KprHTTPKeychainKeyNew(char** key, char* host, char* realm)
{
	FskErr err = kFskErrNone;
	UInt32 keyLength = FskStrLen(host) + 1 + FskStrLen(realm) + 1;
	bailIfError(FskMemPtrNew(keyLength, key));
	FskStrCopy(*key, host);
	FskStrCat(*key, "/");
	FskStrCat(*key, realm);
bail:
	return err;
}

/* KprHTTPKeychainValue */

FskErr KprHTTPKeychainValueNew(KprHTTPKeychainValue* it, char* user, char* password)
{
	FskErr err = kFskErrNone;
	KprHTTPKeychainValue self;
	bailIfError(KprMemPtrNewClear(sizeof(KprHTTPKeychainValueRecord), it));
	self = *it;
	if (user) {
		self->user = KprStrDoCopy(user);
		bailIfNULL(self->user);
	}
	if (password) {
		self->password = KprStrDoCopy(password);
		bailIfNULL(self->password);
	}
bail:
	return err;
}

void KprHTTPKeychainValueDispose(KprHTTPKeychainValue self)
{
	if (self) {
		KprMemPtrDisposeAt(&self->user);
		KprMemPtrDisposeAt(&self->password);
		KprMemPtrDispose(self);
	}
}

Boolean KprHTTPKeychainValueRemove(void* it)
{
	KprHTTPKeychainValue self = it;
	KprHTTPKeychainValueDispose(self);
	return true;
}

FskErr KprHTTPKeychainValueRead(void** it, char** data, UInt32 *size)
{
	FskErr err = kFskErrNone;
	char* start = *data;
	KprHTTPKeychainValue self;

	bailIfError(KprHTTPKeychainValueNew((KprHTTPKeychainValue*)it, NULL, NULL));
	self = *it;
	bailIfError(KprStorageReadString(data, size, &self->user));
	bailIfError(KprStorageReadString(data, size, &self->password));
	*size -= *data - start;
bail:
	return err;
}

FskErr KprHTTPKeychainValueWrite(void* it, FskFile file)
{
	FskErr err = kFskErrNone;
	KprHTTPKeychainValue self = it;
	bailIfError(KprStorageWriteString(file, self->user));
	bailIfError(KprStorageWriteString(file, self->password));
bail:
	return err;
}
