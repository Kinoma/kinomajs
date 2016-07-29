/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#define PCRE_STATIC
#include "pcre.h"
#include "kpr.h"
#include "kprMessage.h"
#include "kprURL.h"

enum {
	XS_BODY = 0,
	XS_DIRECTORIES,
	XS_DIRECTORY,
	XS_FILE,
	XS_MAP,
	XS_RESULT,
	XS_RESULTS,
	XS_URL,
	XS_COUNT
};


typedef struct {
	KprMessage message;
	void* pcre;
	int capture;
	int* offsets;
	char* name;
	int total;
} KprCodeSearchRecord, *KprCodeSearch;


static void KprCodeSearchCancel(KprService service, KprMessage message);
static void KprCodeSearchInvoke(KprService service, KprMessage message);
static void KprCodeSearchStart(KprService self, FskThread thread, xsMachine* the);
static void KprCodeSearchStop(KprService self);
static FskErr KprCodeSearchDirectory(xsMachine* the, KprCodeSearch self, char* path);
static FskErr KprCodeSearchFile(xsMachine* the, KprCodeSearch self, char* name, char* path);
static FskErr exceptionToFskErr(xsMachine *the);

KprServiceRecord gCodeSearch = {
	NULL,
	kprServicesThread,
	"xkpr://search/",
	NULL,
	NULL,
	KprServiceAccept,
	KprCodeSearchCancel,
	KprCodeSearchInvoke,
	KprCodeSearchStart,
	KprCodeSearchStop,
	NULL,
	NULL,
	NULL
};

void KprCodeSearchCancel(KprService service UNUSED, KprMessage message UNUSED)
{
}

void KprCodeSearchInvoke(KprService service, KprMessage message)
{
	FskErr err = kFskErrNone;
	KprCodeSearchRecord searchRecord;
	KprCodeSearch self = &searchRecord;
	char* path = NULL;
	FskMemSet(self, 0, sizeof(KprCodeSearchRecord));
	self->message = message;
	xsBeginHost(service->machine);
	{
		xsVars(XS_COUNT);
		{
			xsTry {
				KprMessageScriptTarget target = (KprMessageScriptTarget)message->stream;
				xsVar(XS_BODY) = xsDemarshall(message->request.body);
				if (FskStrCompareWithLength("results", message->parts.name, message->parts.nameLength) == 0) {
					xsVar(XS_MAP) = xsGet(xsGlobal, xsID("search/results"));
					xsVar(XS_URL) = xsGet(xsVar(XS_BODY), xsID_url);
					if (xsTest(xsVar(XS_MAP)) && xsTest(xsVar(XS_URL)))
						xsResult = xsCall1(xsVar(XS_MAP), xsID_get, xsVar(XS_URL));
					target->result = xsMarshall(xsResult);
				}
				else {
					int options = PCRE_UTF8, offset, c, i; 
					char* reason;
					xsStringValue pattern;
					xsBooleanValue caseless;
					
					xsResult = xsNewInstanceOf(xsArrayPrototype);
					xsVar(XS_MAP) = xsNew0(xsGlobal, xsID_Map);
					xsSet(xsGlobal, xsID("search/results"), xsVar(XS_MAP));

					pattern = xsToString(xsGet(xsVar(XS_BODY), xsID_pattern));
					caseless = xsToBoolean(xsGet(xsVar(XS_BODY), xsID("caseless")));
					if (caseless)
						options += PCRE_CASELESS;
					self->pcre = pcre_compile(pattern, options, (const char **)(void*)&reason, (int*)(void*)&offset, NULL); 
					xsThrowIfNULL(self->pcre);
					pcre_fullinfo(self->pcre, NULL, PCRE_INFO_CAPTURECOUNT, &self->capture);
					self->capture++;
					pcre_fullinfo(self->pcre, NULL, PCRE_INFO_BACKREFMAX, &offset);
					if (self->capture < offset)
						self->capture = offset;
					self->capture *= 3;
					xsThrowIfFskErr(FskMemPtrNew(self->capture * sizeof(int), &self->offsets));
					xsVar(XS_DIRECTORIES) = xsGet(xsVar(XS_BODY), xsID_items);
					c = xsToInteger(xsGet(xsVar(XS_DIRECTORIES), xsID_length));
					for (i = 0; i < c; i++) {
						if (!KprMessageContinue(self->message))
							break;
						xsVar(XS_DIRECTORY) = xsGet(xsVar(XS_DIRECTORIES), i);
						self->name = FskStrDoCopy(xsToString(xsGet(xsVar(XS_DIRECTORY), xsID_name)));
						xsThrowIfNULL(self->name);
						xsThrowIfFskErr(KprURLToPath(xsToString(xsGet(xsVar(XS_DIRECTORY), xsID_url)), &path));
						xsThrowIfFskErr(KprCodeSearchDirectory(the, self, path));
						FskMemPtrDisposeAt(&path);
						FskMemPtrDisposeAt(&self->name);
					}
					if (self->total > 100) {
						c = xsToInteger(xsGet(xsResult, xsID_length));
						for (i = 0; i < c; i++) {
							if (!KprMessageContinue(self->message))
								break;
							xsVar(XS_FILE) = xsGet(xsResult, i);
							xsSet(xsVar(XS_FILE), xsID_expanded, xsFalse);
							xsSet(xsVar(XS_FILE), xsID_items, xsNull);
						}
					}
					if (!KprMessageContinue(self->message)) {
						err = kFskErrOperationCancelled;
					}
					else {
						target->result = xsMarshall(xsResult);
					}
				}
			}
			xsCatch {
				err = exceptionToFskErr(the);
			}
		}
	}
	xsEndHost(service->machine);
	FskMemPtrDispose(path);
	FskMemPtrDispose(self->name);
	FskMemPtrDispose(self->offsets);
	if (self->pcre)
		pcre_free(self->pcre);
	message->error = err;
	message->status = (kFskErrNone == err) ? 200 : 500;
	KprMessageComplete(message);
}

void KprCodeSearchStart(KprService self, FskThread thread, xsMachine* the)
{
	self->machine = the;
	self->thread = thread;
}

void KprCodeSearchStop(KprService self UNUSED)
{
}


FskErr KprCodeSearchDirectory(xsMachine* the, KprCodeSearch self, char* path)
{
	static char* extensions[] = {
		".c",
		".h",
		".js",
		".json",
		".txt",
		".xml",
		".xs",
		NULL
	};
	FskErr err = kFskErrNone;
	UInt32 pathSize = FskStrLen(path);
	FskDirectoryIterator iterator = NULL;
	char *name;
	UInt32 itemType;
	UInt32 nameSize;
	char *buffer = NULL;
	bailIfError(FskDirectoryIteratorNew(path, &iterator, 0));
	while (kFskErrNone == FskDirectoryIteratorGetNext(iterator, &name, &itemType)) {
		if (!KprMessageContinue(self->message))
			break;
		if (name[0] == '.')
			continue;
		nameSize = FskStrLen(name);
		if (kFskDirectoryItemIsDirectory == itemType) {
			bailIfError(FskMemPtrNew(pathSize + nameSize + 2, &buffer));
			FskMemCopy(buffer, path, pathSize);
			FskMemCopy(buffer + pathSize, name, nameSize);
			buffer[pathSize + nameSize] = '/';
			buffer[pathSize + nameSize + 1] = 0;
			bailIfError(KprCodeSearchDirectory(the, self, buffer));
			FskMemPtrDisposeAt(&buffer);
		}
		else if (kFskDirectoryItemIsFile == itemType) {
			char* dot = FskStrRChr(name, '.');
			if (dot) {
				char** extension = extensions;
				while (*extension) {
					if (!FskStrCompare(dot, *extension))
						break;
					extension++;
				}
				if (*extension) {
					bailIfError(FskMemPtrNew(pathSize + nameSize + 1, &buffer));
					FskMemCopy(buffer, path, pathSize);
					FskMemCopy(buffer + pathSize, name, nameSize);
					buffer[pathSize + nameSize] = 0;
					bailIfError(KprCodeSearchFile(the, self, name, buffer));
					FskMemPtrDisposeAt(&buffer);
				}
			}
		}
	}
bail:
	FskMemPtrDispose(buffer);
	FskDirectoryIteratorDispose(iterator);
	return err;
}

FskErr KprCodeSearchFile(xsMachine* the, KprCodeSearch self, char* name, char* path)
{
	FskErr err = kFskErrNone;
	FskFileMapping map = NULL;
	unsigned char *data;
	FskInt64 dataSize;
	int offset, size, count, total = 0;
	char *fromFile, *toFile, *fromLine, *toLine, *from, *to, *url = NULL;
	xsTry {
		xsThrowIfFskErr(FskFileMap(path, &data, &dataSize, 0, &map));
		offset = 0;
		size = (int)dataSize;
		fromFile = (char*)data;
		toFile = (char*)data + size;
		for (;;) {
			if (!KprMessageContinue(self->message))
				break;
			count = pcre_exec(self->pcre, NULL, fromFile, size, offset, 0, self->offsets, self->capture);
			if (count <= 0) {
				break;
			}
			from = fromLine = fromFile + self->offsets[0];
			while (fromLine >= fromFile) {
				char c = *fromLine;
				if ((c == 10) || (c == 13))
					break;
				fromLine--;
			}
			fromLine++;
			while (fromLine < from) {
				char c = *fromLine;
				if ((c != 9) && (c != 32))
					break;
				fromLine++;
			}
			to = toLine = fromFile + self->offsets[1];
			while (toLine < toFile) {
				char c = *toLine;
				if ((c == 10) || (c == 13))
					break;
				toLine++;
			}
			toLine--;
			while (toLine > to) {
				char c = *toLine;
				if ((c != 9) && (c != 32))
					break;
				toLine--;
			}
			toLine++;
			offset = self->offsets[1];
			if (!url) {
				xsThrowIfFskErr(KprPathToURL(path, &url));
				xsVar(XS_URL) = xsString(url);
				xsVar(XS_RESULTS) = xsNewInstanceOf(xsArrayPrototype);
				xsVar(XS_FILE) = xsNewInstanceOf(xsObjectPrototype);
				xsNewHostProperty(xsVar(XS_FILE), xsID_url, xsVar(XS_URL), xsDefault, xsDontScript);
				xsNewHostProperty(xsVar(XS_FILE), xsID_title, xsString(self->name), xsDefault, xsDontScript);
				xsNewHostProperty(xsVar(XS_FILE), xsID_name, xsString(name), xsDefault, xsDontScript);
				xsNewHostProperty(xsVar(XS_FILE), xsID_items, xsVar(XS_RESULTS), xsDefault, xsDontScript);
				xsNewHostProperty(xsVar(XS_FILE), xsID_expanded, xsTrue, xsDefault, xsDontScript);
				(void)xsCall1(xsResult, xsID_push, xsVar(XS_FILE));
				(void)xsCall2(xsVar(XS_MAP), xsID_set, xsVar(XS_URL), xsVar(XS_RESULTS));
			}
			xsVar(XS_RESULT) = xsNewInstanceOf(xsObjectPrototype);
			xsNewHostProperty(xsVar(XS_RESULT), xsID_string, xsStringBuffer(fromLine, toLine - fromLine), xsDefault, xsDontScript);
			xsNewHostProperty(xsVar(XS_RESULT), xsID_offset, xsInteger(fxUTF8ToUnicodeOffset(fromFile, self->offsets[0])), xsDefault, xsDontScript);
			xsNewHostProperty(xsVar(XS_RESULT), xsID_length, xsInteger(fxUTF8ToUnicodeOffset(from, to - from)), xsDefault, xsDontScript);
			xsNewHostProperty(xsVar(XS_RESULT), xsID_delta, xsInteger(fxUTF8ToUnicodeOffset(fromLine, from - fromLine)), xsDefault, xsDontScript);
			(void)xsCall1(xsVar(XS_RESULTS), xsID_push, xsVar(XS_RESULT));
			total++;
		}
		if (url) {
			xsNewHostProperty(xsVar(XS_FILE), xsID_count, xsInteger(total), xsDefault, xsDontScript);
			self->total += 1 + total;
		}
	}
	xsCatch {
		err = exceptionToFskErr(the);
	}
	FskMemPtrDispose(url);
	FskFileDisposeMap(map);
	return err;
}

FskErr exceptionToFskErr(xsMachine *the)
{
    FskErr err = FskStrToNum(xsToString(xsGet(xsException, xsID("message"))));
    return err ? err : kFskErrUnknown;
}



