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
#include "kprCoAPCommon.h"
#include "kprCoAPMessage.h"


Boolean KprCoAPStringMapForEach(KprCoAPStringMap *map, KprCoAPStringMapTask task, void *refcon1, void *refcon2)
{
	KprCoAPStringMapEntry *entry = map->entries;
	int i;
	for (i = 0; i < map->count; i++, entry++) {
		if (task(i, entry->value, entry->str, refcon1, refcon2)) return true;
	}
	return false;
}

static Boolean KprCoAPStringMapEntryFindMatchingStr(int index, int value, const char *str, void *refcon1, void *refcon2)
{
	const char *testStr = (const char *) refcon1;

	if (FskStrCompareCaseInsensitive(str, testStr) == 0) {
		int *it = (int *) refcon2;
		*it = value;
		return true;
	}
	return false;
}

static Boolean KprCoAPStringMapEntryFindMatchingValue(int index, int value, const char *str, void *refcon1, void *refcon2)
{
	int testValue = (int) refcon1;

	if (value == testValue) {
		const char **it = (const char **) refcon2;
		*it = str;
		return true;
	}
	return false;
}

FskErr KprCoAPStringMapFindMatchingStr(KprCoAPStringMap *map, const char *str, int *it)
{
	Boolean found = KprCoAPStringMapForEach(map, KprCoAPStringMapEntryFindMatchingStr, (void *) str, (void *) it);
	return (found ? kFskErrNone : kFskErrNotFound);
}

FskErr KprCoAPStringMapFindMatchingValue(KprCoAPStringMap *map, int value, const char **it)
{
	Boolean found = KprCoAPStringMapForEach(map, KprCoAPStringMapEntryFindMatchingValue, (void *) value, (void *) it);
	return (found ? kFskErrNone : kFskErrNotFound);
}

static KprCoAPStringMapEntry sKprCoAPMessageTypes[] = {
	{ kKprCoAPMessageTypeAcknowledgement, "Acknowledgement" },
	{ kKprCoAPMessageTypeConfirmable, "Confirmable" },
	{ kKprCoAPMessageTypeNonConfirmable, "Non-confirmable" },
	{ kKprCoAPMessageTypeReset, "Reset" },
};

FskErr KprCoAPMessageTypeToString(int type, const char **it)
{
	KprCoAPStringMapDefine(map, sKprCoAPMessageTypes);
	return KprCoAPStringMapFindMatchingValue(&map, type, it);
}

static KprCoAPStringMapEntry sKprCoAPMessageMethods[] = {
	{ kKprCoAPRequestMethodGET, "GET" },
	{ kKprCoAPRequestMethodPOST, "POST" },
	{ kKprCoAPRequestMethodPUT, "PUT" },
	{ kKprCoAPRequestMethodDELETE, "DELETE" },
};

FskErr KprCoAPMethodFromString(const char *str, int *method)
{
	KprCoAPStringMapDefine(map, sKprCoAPMessageMethods);
	return KprCoAPStringMapFindMatchingStr(&map, str, method);
}

FskErr KprCoAPMethodToString(int method, const char **it)
{
	KprCoAPStringMapDefine(map, sKprCoAPMessageMethods);
	return KprCoAPStringMapFindMatchingValue(&map, method, it);
}

static KprCoAPStringMapEntry sKprCoAPContentFormats[] = {
	{ kKprCoAPContentFormatPlainText, "text/plain; charset=utf-8" },
	{ kKprCoAPContentFormatPlainText, "text/plain" },
	{ kKprCoAPContentFormatPlainText, "text" },
	{ kKprCoAPContentFormatPlainText, "plaintext" },

	{ kKprCoAPContentFormatLinkFormat, "application/link-format" },
	{ kKprCoAPContentFormatLinkFormat, "link-format" },
	{ kKprCoAPContentFormatLinkFormat, "linkformat" },

	{ kKprCoAPContentFormatXml, "application/xml" },
	{ kKprCoAPContentFormatXml, "xml" },

	{ kKprCoAPContentFormatOctetStream, "application/octet-stream" },
	{ kKprCoAPContentFormatOctetStream, "octet-stream" },
	{ kKprCoAPContentFormatOctetStream, "octetstream" },

	{ kKprCoAPContentFormatExi, "application/exi" },
	{ kKprCoAPContentFormatExi, "exi" },

	{ kKprCoAPContentFormatJson, "application/json" },
	{ kKprCoAPContentFormatJson, "json" },
};

FskErr KprCoAPContentFormatFromString(const char *str, int *format)
{
	KprCoAPStringMapDefine(map, sKprCoAPContentFormats);
	return KprCoAPStringMapFindMatchingStr(&map, str, format);
}

FskErr KprCoAPContentFormatToString(int format, const char **it)
{
	KprCoAPStringMapDefine(map, sKprCoAPContentFormats);
	return KprCoAPStringMapFindMatchingValue(&map, format, it);
}

static KprCoAPStringMapEntry sKprCoAPMessageOptions[] = {
	{ kKprCoAPMessageOptionIfMatch, "If-Match" },
	{ kKprCoAPMessageOptionIfMatch, "IfMatch" },
	{ kKprCoAPMessageOptionUriHost, "Uri-Host" },
	{ kKprCoAPMessageOptionUriHost, "UriHost" },
	{ kKprCoAPMessageOptionETag, "ETag" },
	{ kKprCoAPMessageOptionIfNoneMatch, "If-None-Match" },
	{ kKprCoAPMessageOptionIfNoneMatch, "IfNoneMatch" },
	{ kKprCoAPMessageOptionUriPort, "Uri-Port" },
	{ kKprCoAPMessageOptionUriPort, "UriPort" },
	{ kKprCoAPMessageOptionLocationPath, "Location-Path" },
	{ kKprCoAPMessageOptionLocationPath, "LocationPath" },
	{ kKprCoAPMessageOptionUriPath, "Uri-Path" },
	{ kKprCoAPMessageOptionUriPath, "UriPath" },
	{ kKprCoAPMessageOptionContentFormat, "Content-Format" },
	{ kKprCoAPMessageOptionContentFormat, "ContentFormat" },
	{ kKprCoAPMessageOptionMaxAge, "Max-Age" },
	{ kKprCoAPMessageOptionMaxAge, "MaxAge" },
	{ kKprCoAPMessageOptionUriQuery, "Uri-Query" },
	{ kKprCoAPMessageOptionUriQuery, "UriQuery" },
	{ kKprCoAPMessageOptionAccept, "Accept" },
	{ kKprCoAPMessageOptionLocationQuery, "Location-Query" },
	{ kKprCoAPMessageOptionLocationQuery, "LocationQuery" },
	{ kKprCoAPMessageOptionProxyUri, "Proxy-Uri" },
	{ kKprCoAPMessageOptionProxyUri, "ProxyUri" },
	{ kKprCoAPMessageOptionProxyScheme, "Proxy-Scheme" },
	{ kKprCoAPMessageOptionProxyScheme, "ProxyScheme" },
	{ kKprCoAPMessageOptionSize1, "Size1" },

	{ kKprCoAPMessageOptionBlock2, "Block2" },
	{ kKprCoAPMessageOptionBlock1, "Block1" },
	{ kKprCoAPMessageOptionSize2, "Size2" },

	{ kKprCoAPMessageOptionObserve, "Observe" },
};

FskErr KprCoAPMessageOptionFromString(const char *str, int *option)
{
	KprCoAPStringMapDefine(map, sKprCoAPMessageOptions);
	return KprCoAPStringMapFindMatchingStr(&map, str, option);
}

FskErr KprCoAPMessageOptionToString(int option, const char **it)
{
	KprCoAPStringMapDefine(map, sKprCoAPMessageOptions);
	return KprCoAPStringMapFindMatchingValue(&map, option, it);
}

FskErr KprCoAPStrNewWithLength(const char *src, int length, char **it)
{
	FskErr err = kFskErrNone;

	if (length > 0) {
		bailIfError(KprMemPtrNew(length + 1, it));
		FskMemCopy(*it, src, length);
		(*it)[length] = 0;
	} else {
		*it = NULL;
	}

bail:
	return err;
}

double KprCoAPTimeToDouble(FskTime time)
{
	return time->seconds + ((double) time->useconds / 1000000);
}

double KprCoAPFromNow(FskTime time)
{
	return KprCoAPTimeToDouble(time) - KprCoAPGetNow();
}

double KprCoAPGetNow()
{
	FskTimeRecord now;
	FskTimeGetNow(&now);
	return KprCoAPTimeToDouble(&now);
}

FskErr KprCoAPMessageQueueAppend(KprCoAPMessageQueue *queue, KprCoAPMessage message)
{
	return KprCoAPMessageQueueAppendWithSize(queue, message, sizeof(KprCoAPMessageQueueRecord), NULL);
}

FskErr KprCoAPMessageQueueAppendWithSize(KprCoAPMessageQueue *queue, KprCoAPMessage message, size_t recordSize, KprCoAPMessageQueue *it)
{
	FskErr err = kFskErrNone;
	KprCoAPMessageQueue entry = NULL;

	bailIfError(KprMemPtrNewClear(recordSize, &entry));

	FskListAppend(queue, entry);
	entry->message = KprCoAPMessageRetain(message);

	if (it) {
		*it = entry;
	}

bail:
	if (err) {
		KprMemPtrDispose(entry);
	}

	return err;
}

FskErr KprCoAPMessageQueueDispose(KprCoAPMessageQueue queue)
{
	FskErr err = kFskErrNone;
	while (queue) {
		KprCoAPMessageQueue entry = queue;
		FskListRemove(&queue, entry);
		KprCoAPMessageDispose(entry->message);
		KprMemPtrDispose(entry);
	}
	return err;
}

FskErr KprCoAPMessageQueueRemove(KprCoAPMessageQueue *queue, KprCoAPMessage message)
{
	KprCoAPMessageQueue entry = KprCoAPMessageQueueFind(*queue, message);

	if (entry == NULL) return kFskErrNotFound;

	FskListRemove(queue, entry);
	KprCoAPMessageDispose(entry->message);
	KprMemPtrDispose(entry);

	return kFskErrNone;
}

KprCoAPMessageQueue KprCoAPMessageQueueFind(KprCoAPMessageQueue queue, KprCoAPMessage message)
{
	KprCoAPMessageQueue entry = queue;

	while (entry) {
		if (entry->message == message) {
			return entry;
		}

		entry = entry->next;
	}

	return NULL;
}


