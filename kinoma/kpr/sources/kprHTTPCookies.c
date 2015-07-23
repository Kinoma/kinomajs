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
#include "kprHTTPCookies.h"
#include "kprHTTPClient.h"
#include "kprURL.h"
#include "kprUtilities.h"

#define kprHTTPCookieStorage 'kco2'

/* KprHTTPCookies */

static Boolean KprHTTPCookieRemove(void* it);
static FskErr KprHTTPCookieRead(void** it, char** data, UInt32* size);
static FskErr KprHTTPCookieWrite(void* it, FskFile file);

static KprStorageEntryDispatchRecord kprHTTPCookieDispatch = {
	KprHTTPCookieRemove,
	KprHTTPCookieRead,
	KprHTTPCookieWrite
};

FskErr KprHTTPCookiesNew(KprHTTPCookies* it, UInt32 size, char* path)
{
	FskErr err = kFskErrNone;
	KprHTTPCookies self;
	bailIfError(KprMemPtrNewClear(sizeof(KprHTTPCookiesRecord), it));
	self = *it;
	bailIfError(KprStorageNew(&(self->table), kprHTTPCookieStorage, size, &kprHTTPCookieDispatch));
	self->path = KprStrDoCopy(path);
	bailIfNULL(self->path);
bail:
	return err;
}

void KprHTTPCookiesDispose(KprHTTPCookies self)
{
	if (self) {
		KprStorageDispose(self->table);
		KprMemPtrDisposeAt(&self->path);
		KprMemPtrDispose(self);
	}
}

void KprHTTPCookiesCleanup(KprHTTPCookies self, Boolean session)
{
	KprStorage storage = self->table;
	UInt32 c = storage->size, i;
	UInt32 now;
	Boolean remove;
	
	now = KprDateNow();
	for (i = 0; i < c; i++) {
		KprStorageEntry entry, *address;
		address = storage->entries + i;
		while ((entry = *address)) {
			KprHTTPCookie cookie = entry->value;
			if (cookie->expire)
				remove = cookie->expire < now;
			else
				remove = session;
			if (remove) {
				*address = entry->next;
				KprHTTPCookieDispose(entry->value);
				KprStorageEntryDispose(entry);
				storage->count -= 1;
			}
			else
				address = &entry->next;
		}
	}
}

void KprHTTPCookiesClear(KprHTTPCookies self)
{
	KprStorageClear(self->table);
}

FskErr KprHTTPCookiesGet(KprHTTPCookies self, char* url, char** it)
{
	FskErr err = kFskErrNone;
	KprStorageEntry entry = NULL;
	KprHTTPCookie cookie;
	char* start;
	char* end;
	char c;
	char* cookies = NULL;
	UInt32 size = 0, offset = 0, length, nameLength, valueLength;
	
	KprURLPartsRecord parts;
	KprURLSplit(url, &parts);
	
	if (parts.authority) {
		start = parts.authority - 1;
		end = parts.authority + parts.authorityLength;
		c = *end;
		*end = 0;
		while (start) {
			for (KprStorageGetFirstEntry(self->table, start + 1, &entry); entry; KprStorageGetNextEntry(self->table, &entry)) {
				cookie = entry->value;
				*end = c;
				if (!parts.path || !FskStrCompareWithLength(parts.path, cookie->path, FskStrLen(cookie->path))) {
					nameLength = FskStrLen(cookie->name);
					valueLength = FskStrLen(cookie->value);
					length = nameLength + valueLength;
					if (nameLength && valueLength)
						length++;
					if (!cookies) {
						bailIfError(KprMemPtrNewClear(length + 1, &cookies));
						size = length + 1;
					}
					else {
						bailIfError(KprMemPtrRealloc(size + length + 2, &cookies));
						size += length + 2;
						FskStrCopy(cookies + offset, ", "); offset += 2;
					}
					FskStrCopy(cookies + offset, cookie->name); offset += nameLength;
					if (nameLength && valueLength)
						FskStrCopy(cookies + offset, "="); offset += 1;
					FskStrCopy(cookies + offset, cookie->value); offset += valueLength;
				}
				*end = 0;
			}
			start = FskStrChr(start + 1, '.');
		}
		*end = c;
	}
	*it = cookies;
	return err;
bail:
	KprMemPtrDispose(cookies);
	*it = NULL;
	return err;
}

FskErr KprHTTPCookiesPut(KprHTTPCookies self, char* url, char* cookies)
{
	FskErr err = kFskErrNone;
	char* stopString = ", ";
	char* start = cookies;
	char* stop;
	char* end = cookies + FskStrLen(cookies);
	char c;
	KprHTTPCookie new = NULL;
	KprHTTPCookie cookie = NULL;
	char* domain = NULL;
	while (start < end) {
		stop = FskStrStrCaseInsensitive(start, stopString);
		if (!stop)
			stop = end;
		else if (stop > start + 11) {
			if (FskStrCompareCaseInsensitiveWithLength(stop - 11, "expires=", 8) == 0) {
				stop = FskStrStrCaseInsensitive(stop + 2, stopString);
				if (!stop)
					stop = end;
			}
		}
		if (stop == start)
			continue;
		c = *stop;
		*stop = 0;
		bailIfError(KprHTTPCookieParseString(&new, url, start, &domain));
		if (new) {
			KprStorageEntry entry = NULL, insert = NULL, *address;
			KprStorage storage = self->table;
			UInt32 newLength, cookieLength, sum = KprStorageEntrySum(domain);
			address = storage->entries + (sum % storage->size);
			while ((entry = *address)) {
				if ((entry->sum == sum) && entry->key && !FskStrCompare(entry->key, domain)) {
					cookie = entry->value;
					newLength = FskStrLen(new->path);
					cookieLength = FskStrLen(cookie->path);
					if (newLength > cookieLength) {
						// insert new more specific path cookie
						break;
					}
					else if (newLength == cookieLength) {
						if ((new->secure == cookie->secure)
							&& (new->httpOnly == cookie->httpOnly)
							&& !FskStrCompare(new->name, cookie->name)
							&& !FskStrCompare(new->path, cookie->path)) {
							// replace cookie
							cookie->expire = new->expire;
							KprMemPtrDispose(cookie->value);
							cookie->value = new->value;
							new->value = NULL;
							KprHTTPCookieDispose(new);
							insert = entry;
							break;
						}
					}
				}
				address = &entry->next;
			}
			if (!insert) {
				// insert new cookie
				bailIfError(KprStorageEntryNew(&insert, domain, new));
				*address = insert;
				insert->next = entry;
				storage->count += 1;
			}
			KprMemPtrDisposeAt(&domain);
		}
		*stop = c;
		start = stop + 2;
	}
	return err;
bail:
	KprMemPtrDispose(domain);
	return err;
}

void KprHTTPCookiesRead(KprHTTPCookies self)
{
	(void)KprStorageRead(&(self->table), self->path);
}

FskErr KprHTTPCookiesWrite(KprHTTPCookies self)
{
	FskErr err = kFskErrNone;
	bailIfError(!self->path);
	bailIfError(KprStorageWrite(self->table, self->path));
bail:
	return err;
}

/* KprHTTPCookie */

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprHTTPCookieInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprHTTPCookie", FskInstrumentationOffset(KprHTTPCookieRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprHTTPCookieNew(KprHTTPCookie* it)
{
	FskErr err = kFskErrNone;
	bailIfError(KprMemPtrNewClear(sizeof(KprHTTPCookieRecord), it));
	FskInstrumentedItemNew(*it, NULL, &KprHTTPCookieInstrumentation);
bail:
	return err;
}

void KprHTTPCookieDispose(KprHTTPCookie self)
{
	if (self) {
		KprMemPtrDisposeAt(&self->name);
		KprMemPtrDisposeAt(&self->value);
		KprMemPtrDisposeAt(&self->path);
		FskInstrumentedItemDispose(self);
		KprMemPtrDispose(self);
	}
}

FskErr KprHTTPCookieParseString(KprHTTPCookie* it, char* url, char* cookie, char** key)
{
	FskErr err = kFskErrNone;
	KprHTTPCookie self = NULL;
	char* start = cookie;
	char* next;
	char* stop;
	char* end = cookie + FskStrLen(cookie);
	SInt32 seconds;
	char c;
	Boolean escape = false;
	char** ptr;
	
	// split
	while (start < end) {
		if (*start == ';') *start = 0;
		start++;
	}
	
	bailIfError(KprHTTPCookieNew(it));
	self = *it;
	start = cookie;
	
	stop = start;
	while ((c = *stop)) {
		if (!self->name && (c == '=')) {
			bailIfError(KprMemPtrNewClear(stop - start + 1, &self->name));
			if (escape) {
				*stop = 0;
				FskStrDecodeEscapedChars(start, self->name);
				*stop = '=';
			}
			else
				FskStrNCopy(self->name, start, stop - start);
			start = stop + 1;
			escape = false;
		}
		else if (c == '%') {
			escape = true;
		}
		stop++;
	}
	ptr = (self->name) ? &self->value : &self->name;
	bailIfError(KprMemPtrNewClear(stop - start + 1, ptr));
	if (escape)
		FskStrDecodeEscapedChars(start, *ptr);
	else
		FskStrNCopy(*ptr, start, stop - start);
	
	start += FskStrLen(start) + 1;
	while (start <= end) {
		next = start + FskStrLen(start) + 1;
		start = FskStrStripHeadSpace(start);
		if (!FskStrCompareCaseInsensitiveWithLength(start, "domain", 6)) {
			bailIfError(!(start = FskStrStripHeadSpace(start + 6)));
			bailIfError(*start != '=');
			bailIfError(!(start = FskStrStripHeadSpace(start + 1)));
			if (*start == '.')
				start++;
			bailIfError(!*start); // empty domain
			*key = KprStrDoCopy(start);
			bailIfNULL(*key);
		}
		else if (!FskStrCompareCaseInsensitiveWithLength(start, "expires", 7)) {
			if (self->expire == 0) {
				bailIfError(!(start = FskStrStripHeadSpace(start + 7)));
				bailIfError(*start != '=');
				bailIfError(!(start = FskStrStripHeadSpace(start + 1)));
				bailIfError(KprDateFromHTTP(start, &self->expire));
			}
		}
		else if (!FskStrCompareCaseInsensitiveWithLength(start, "max-age", 7)) {
			bailIfError(!(start = FskStrStripHeadSpace(start + 7)));
			bailIfError(*start != '=');
			bailIfError(!(start = FskStrStripHeadSpace(start + 1)));
			self->expire = KprDateNow();
			seconds = FskStrToNum(start);
			self->expire += seconds;
		}
		else if (!FskStrCompareCaseInsensitiveWithLength(start, "path", 4)) {
			bailIfError(!(start = FskStrStripHeadSpace(start + 4)));
			bailIfError(*start != '=');
			bailIfError(!(start = FskStrStripHeadSpace(start + 1)));
			self->path = KprStrDoCopy(start);
			bailIfNULL(self->path);
			FskStrStripTailSpace(self->path);
		}
		else if (!FskStrCompareCaseInsensitive(start, "secure"))
			self->secure = true;
		else if (!FskStrCompareCaseInsensitive(start, "httpOnly"))
			self->httpOnly = true;
		start = next;
	}
	
	// default values for domain and path
	if (!*key || !self->path) {
		KprURLPartsRecord parts;
		KprURLSplit(url, &parts);
		if (!*key) {
			bailIfError(KprMemPtrNewClear(parts.authorityLength + 1, key));
			FskStrNCopy(*key, parts.authority, parts.authorityLength);
		}
		if (!self->path) {
			if (!parts.pathLength) {
				self->path = KprStrDoCopy("/");
				bailIfNULL(self->path);
			}
			else {
				bailIfError(KprMemPtrNewClear(parts.pathLength + 1, &self->path));
				FskStrNCopy(self->path, parts.path, parts.pathLength);
			}
		}
		
	}
bail:
	// unsplit
	start = cookie;
	while (start < end) {
		if (*start == 0) *start++ = ';';
		start++;
	}
	if (err) {
		KprMemPtrDisposeAt(key);
		KprHTTPCookieDispose(self);
		*it = NULL;
	}
	return err;
}

static Boolean KprHTTPCookieRemove(void* it)
{
	KprHTTPCookie self = it;
	KprHTTPCookieDispose(self);
	return true;
}

static FskErr KprHTTPCookieRead(void** it, char** data, UInt32* size)
{
	FskErr err = kFskErrNone;
	KprHTTPCookie self = NULL;
	
	bailIfError(KprHTTPCookieNew(&self));
	bailIfError(KprStorageReadString(data, size, &self->name));
	bailIfError(KprStorageReadString(data, size, &self->value));
	bailIfError(KprStorageReadString(data, size, &self->path));
	bailIfError(KprStorageReadUInt32(data, size, &self->expire));
	bailIfError(KprStorageReadBoolean(data, size, &self->secure));
	bailIfError(KprStorageReadBoolean(data, size, &self->httpOnly));
	
	*it = self;
bail:
	return err;
}

static FskErr KprHTTPCookieWrite(void* it, FskFile file)
{
	FskErr err = kFskErrNone;
	KprHTTPCookie self = it;
	bailIfError(KprStorageWriteString(file, self->name));
	bailIfError(KprStorageWriteString(file, self->value));
	bailIfError(KprStorageWriteString(file, self->path));
	bailIfError(KprStorageWriteUInt32(file, self->expire));
	bailIfError(KprStorageWriteBoolean(file, self->secure));
	bailIfError(KprStorageWriteBoolean(file, self->httpOnly));

bail:
	return err;
}
