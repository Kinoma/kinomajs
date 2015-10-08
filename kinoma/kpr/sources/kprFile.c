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
#include "FskImage.h"
#include "FskMediaPlayer.h"

#include "kpr.h"
#include "kprMessage.h"
#include "kprShell.h"
#include "kprURL.h"

static void KprFILEServiceStart(KprService self, FskThread thread, xsMachine* the);
static void KprFILEServiceStop(KprService self);
static void KprFILEServiceCancel(KprService self, KprMessage message);
static void KprFILEServiceInvoke(KprService self, KprMessage message);

KprServiceRecord gFILEService = {
	NULL,
	kprServicesThread,
	"file:",
	NULL,
	NULL,
	KprServiceAccept,
	KprFILEServiceCancel,
	KprFILEServiceInvoke,
	KprFILEServiceStart,
	KprFILEServiceStop,
	NULL,
	NULL,
	NULL
};

void KprFILEServiceStart(KprService self, FskThread thread, xsMachine* the)
{
	self->machine = the;
	self->thread = thread;
}

void KprFILEServiceStop(KprService self UNUSED)
{
}

void KprFILEServiceCancel(KprService service UNUSED, KprMessage message UNUSED)
{
}

void KprFILEServiceInvoke(KprService service UNUSED, KprMessage message)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFileInfo info;
	FskDirectoryIterator iterator = NULL;
	char* pathName = NULL;
	char* sniff = NULL;
	FskFile fref = NULL;
	if (KprMessageContinue(message)) {
		if (!message->method || FskStrCompare(message->method, "GET")) {
			bailIfError(KprURLToPath(message->url, &path));
			bailIfError(FskFileGetFileInfo(path, &info));
			if (kFskDirectoryItemIsDirectory == info.filetype) {
				unsigned char buffer[4096];
				char *name;
				UInt32 itemType;
				double date;
				UInt32 size;
				xsBeginHostSandboxCode(gFILEService.machine, NULL);
				{
					xsVars(3);
					{
						xsTry {
							xsVar(0) = xsGet(xsGlobal, xsID("Files"));
							xsResult = xsNewInstanceOf(xsArrayPrototype);
							bailIfError(FskDirectoryIteratorNew(path, &iterator, 0));
							while (kFskErrNone == FskDirectoryIteratorGetNext(iterator, &name, &itemType)) {
								if (name[0] == '.')
									continue;
								if (kFskDirectoryItemIsFile == itemType) {
									pathName = FskStrDoCat(path, name);
									bailIfError(FskFileGetFileInfo(pathName, &info));
									bailIfError(FskFileOpen(pathName, kFskFilePermissionReadOnly, &fref));
									bailIfError(FskFileRead(fref, sizeof(buffer), buffer, &size));
									FskFileClose(fref);
									fref = NULL;
									if (kFskErrNone == FskMediaPlayerSniffForMIME(buffer, size, NULL, pathName, &sniff)) {
									}
									else if (kFskErrNone == FskImageDecompressSniffForMIME(buffer, size, NULL, pathName, &sniff)) {
									}
									FskMemPtrDispose(pathName);
									pathName = NULL;
								}
								xsVar(1) = xsNewInstanceOf(xsObjectPrototype);
								xsNewHostProperty(xsVar(1), xsID("name"), xsString(name), xsDefault, xsDontScript);
								xsNewHostProperty(xsVar(1), xsID("path"), xsString(name), xsDefault, xsDontScript);
								FskStrCopy((char*)buffer, message->url);
								FskStrCat((char*)buffer, name);
								if (kFskDirectoryItemIsDirectory == itemType) {
									xsVar(2) = xsGet(xsVar(0), xsID("directoryType"));
									FskStrCat((char*)buffer, "/");
								}
								else if (kFskDirectoryItemIsFile == itemType) {
									xsVar(2) = xsGet(xsVar(0), xsID("fileType"));
									if (info.fileModificationDate > info.fileCreationDate)
										date = info.fileModificationDate * 1000.0;	
									else
										date = info.fileCreationDate * 1000.0;
									xsNewHostProperty(xsVar(1), xsID("date"), xsNumber(date), xsDefault, xsDontScript);
									xsNewHostProperty(xsVar(1), xsID("size"), xsNumber(info.filesize), xsDefault, xsDontScript);
									if (sniff) {
										xsNewHostProperty(xsVar(1), xsID("mime"), xsString(sniff), xsDefault, xsDontScript);
										FskMemPtrDispose(sniff);
										sniff = NULL;
									}
								}
								else
									xsVar(2) = xsGet(xsVar(0), xsID("linkType"));
								xsNewHostProperty(xsVar(1), xsID("type"), xsVar(2), xsDefault, xsDontScript);
								xsNewHostProperty(xsVar(1), xsID("url"), xsString((char*)buffer), xsDefault, xsDontScript);
								(void)xsCall1(xsResult, xsID("push"), xsVar(1));
							}
							xsResult = xsCall1(xsGet(xsGlobal, xsID("JSON")), xsID("stringify"), xsResult);
							message->response.body = FskStrDoCopy(xsToString(xsResult));
							message->response.size = FskStrLen(message->response.body);
							KprMessageTransform(message, gFILEService.machine);
						}
						xsCatch {
						}
					}
				}
				xsEndHostSandboxCode();
			}
			else if (kFskDirectoryItemIsFile == info.filetype) {
				FskInt64 size;
				bailIfError(FskFileOpen(path, kFskFilePermissionReadOnly, &fref));
				bailIfError(FskFileGetSize(fref, &size));
				bailIfError(FskMemPtrNew(size, &message->response.body));
				bailIfError(FskFileRead(fref, size, message->response.body, &message->response.size));
				KprMessageTransform(message, gFILEService.machine);
			}
			else
				err = kFskErrUnimplemented;
		}
		else
			err = kFskErrUnimplemented;
	bail:
		FskMemPtrDispose(sniff);
		FskFileClose(fref);
		FskMemPtrDispose(pathName);
		FskDirectoryIteratorDispose(iterator);
		FskMemPtrDispose(path);
		message->error = err;
	}
	FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)KprMessageComplete, message, NULL, NULL, NULL);
}

static void KPR_Files_getSpecialDir(xsMachine* the, UInt32 dirType, Boolean create)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	char* url = NULL;
	bailIfError(FskDirectoryGetSpecialPath(dirType, create, NULL, &path));
	bailIfError(KprPathToURL(path, &url));
	xsResult = xsString(url);
bail:
	FskMemPtrDispose(url);
	FskMemPtrDispose(path);
}

void KPR_Files_get_documentsDirectory(xsMachine* the)
{
	KPR_Files_getSpecialDir(the, kFskDirectorySpecialTypeDocument, true);
}

void KPR_Files_get_picturesDirectory(xsMachine* the)
{
#if TARGET_OS_IPHONE
	xsResult = xsString("assets-library://pictures/");
#else
	KPR_Files_getSpecialDir(the, kFskDirectorySpecialTypePhoto, true);
#endif
}

void KPR_Files_get_preferencesDirectory(xsMachine* the)
{
	KPR_Files_getSpecialDir(the, kFskDirectorySpecialTypeApplicationPreference, true);
}

void KPR_Files_get_temporaryDirectory(xsMachine* the)
{
	KPR_Files_getSpecialDir(the, kFskDirectorySpecialTypeTemporary, true);
}

static FskErr KPR_Files_deleteDirectoryContent(xsMachine* the, char* path)
{
	FskErr err = kFskErrNone;
	FskDirectoryIterator iterator = NULL;
	char* name = NULL;
	char* itemPath = NULL;
	UInt32 itemType;
	bailIfError(FskDirectoryIteratorNew(path, &iterator, 1));
	while (kFskErrNone == (err = FskDirectoryIteratorGetNext(iterator, &name, &itemType))) {
		bailIfError(FskMemPtrRealloc(FskStrLen(path) + FskStrLen(name) + 2, &itemPath));
		FskStrCopy(itemPath, path);
		FskStrCat(itemPath, name);
		if (kFskDirectoryItemIsDirectory == itemType) {
			FskStrCat(itemPath, "/");
			bailIfError(KPR_Files_deleteDirectoryContent(the, itemPath));
			bailIfError(FskFileDeleteDirectory(itemPath));
		}
		else
			bailIfError(FskFileDelete(itemPath));
		FskMemPtrDisposeAt(&name);
		FskMemPtrDisposeAt(&itemPath);
	}
	if (kFskErrIteratorComplete == err)
		err = kFskErrNone;
bail:
	FskDirectoryIteratorDispose(iterator);
	FskMemPtrDispose(name);
	FskMemPtrDispose(itemPath);
	return err;
}

void KPR_Files_deleteDirectory(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	if ((xsToInteger(xsArgc) > 1) && xsTest(xsArg(1))) // recurse
		bailIfError(KPR_Files_deleteDirectoryContent(the, path));
	bailIfError(FskFileDeleteDirectory(path));
	xsResult = xsBoolean(kFskErrNone == err);
bail:
	FskMemPtrDispose(path);
	if (kFskErrFileNotFound != err)
		xsThrowIfFskErr(err);
}

void KPR_Files_deleteFile(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	err = FskFileDelete(path);
	xsResult = xsBoolean(kFskErrNone == err);
bail:
	FskMemPtrDispose(path);
	if (kFskErrFileNotFound != err)
		xsThrowIfFskErr(err);
}

void KPR_Files_ensureDirectory(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFileInfo info;
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	if (kFskErrNone != FskFileGetFileInfo(path, &info)) {
		char* p = path + 1;
		char* q = FskStrRChr(p, '/');
		if (q) {
			q++;
			*q = 0;
			while ((p = FskStrChr(p, '/'))) {
				char c;
				p++;
				c = *p;
				*p = 0;
				if (kFskErrNone != FskFileGetFileInfo(path, &info)) {
					bailIfError(FskFileCreateDirectory(path));
				}
				*p = c;
			}
		}
	}
bail:
	FskMemPtrDispose(path);
}

void KPR_Files_exists(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFileInfo info;
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	if (kFskErrNone == FskFileGetFileInfo(path, &info)) {
		xsResult = xsGet(xsGlobal, xsID_Files);
		if (kFskDirectoryItemIsDirectory == info.filetype)
			xsResult = xsGet(xsResult, xsID_directoryType);
		else if (kFskDirectoryItemIsFile == info.filetype)
			xsResult = xsGet(xsResult, xsID_fileType);
		else
			xsResult = xsGet(xsResult, xsID_linkType);
	}
bail:
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}

void KPR_Files_getInfo(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFileInfo info;
	xsNumberValue created;
	xsNumberValue date;
	xsVars(1);
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	if (kFskErrNone == FskFileGetFileInfo(path, &info)) {
		xsResult = xsNewInstanceOf(xsObjectPrototype);
		xsVar(0) = xsGet(xsGlobal, xsID_Files);
		if (kFskDirectoryItemIsDirectory == info.filetype)
			xsVar(0) = xsGet(xsVar(0), xsID_directoryType);
		else if (kFskDirectoryItemIsFile == info.filetype)
			xsVar(0) = xsGet(xsVar(0), xsID_fileType);
		else
			xsVar(0) = xsGet(xsVar(0), xsID_linkType);
		xsNewHostProperty(xsResult, xsID("type"), xsVar(0), xsDefault, xsDontScript);
		created = info.fileCreationDate * 1000.0;
		if (info.fileModificationDate > info.fileCreationDate)
			date = info.fileModificationDate * 1000.0;	
		else
			date = info.fileCreationDate * 1000.0;
		xsNewHostProperty(xsResult, xsID("created"), xsNumber(created), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID("date"), xsNumber(date), xsDefault, xsDontScript);
		xsNewHostProperty(xsResult, xsID("size"), xsInteger((xsIntegerValue)info.filesize), xsDefault, xsDontScript);
	}
bail:
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}

void KPR_Files_getVolumeInfo(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	Boolean isRemovable;
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	bailIfError(FskVolumeGetInfoFromPath(path, NULL, NULL, NULL, &isRemovable, NULL, NULL));
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID("removable"), xsBoolean(isRemovable), xsDefault, xsDontScript);
bail:
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}

typedef struct {
	FskDirectoryChangeNotifier notifier;
	xsMachine* the;
	xsSlot slot;
} KprDirectoryChangeNotifierRecord, *KprDirectoryChangeNotifier;

static void KPR_Files_DirectoryNotifier_callback(UInt32 flags, const char *path, void *it);

void KPR_Files_DirectoryNotifier(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	UInt32 flags = 0;
	KprDirectoryChangeNotifier data;
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	xsSet(xsThis, xsID_callback, xsArg(1));
	bailIfError(FskMemPtrNewClear(sizeof(KprDirectoryChangeNotifierRecord), &data));
	data->the = the;
	data->slot = xsThis;
	xsSetHostData(xsThis, data);
	bailIfError(FskDirectoryChangeNotifierNew(path, flags, KPR_Files_DirectoryNotifier_callback, data, &data->notifier));
bail:
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}

void KPR_Files_DirectoryNotifier_callback(UInt32 flags, const char *path, void *it)
{
	KprDirectoryChangeNotifier self = (KprDirectoryChangeNotifier)it;
	xsBeginHost(self->the);
	xsThis = self->slot;
	xsResult = xsGet(xsThis, xsID_callback);
	(void)xsCallFunction2(xsResult, xsThis, path ? xsString(path) : xsNull, xsInteger(flags));
	xsEndHost(self->the);
}

void KPR_Files_directoryNotifier(void *data)
{
	KprDirectoryChangeNotifier self = (KprDirectoryChangeNotifier)data;
	if (self) {
		FskDirectoryChangeNotifierDispose(self->notifier);
		FskMemPtrDispose(self);
	}
}

void KPR_Files_directoryNotifier_close(xsMachine *the)
{
	KPR_Files_directoryNotifier(xsGetHostData(xsThis));
	xsSetHostData(xsThis, NULL);
}

void KPR_Files_renameFile(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	char* name = NULL;
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	name = xsToString(xsArg(1));
	bailIfError(FskFileRename(path, name));
bail:
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}

void KPR_Files_renameDirectory(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	char* name = NULL;
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	name = xsToString(xsArg(1));
	bailIfError(FskFileRenameDirectory(path, name));
bail:
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}

void KPR_Files_readChunk(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFileMapping map = NULL;
	unsigned char *data;
	FskInt64 size;
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	bailIfError(FskFileMap(path, &data, &size, 0, &map));
	xsResult = xsNew1(xsGlobal, xsID_Chunk, xsInteger(size));
	FskMemMove(xsGetHostData(xsResult), data, (SInt32)size);
bail:
	FskFileDisposeMap(map);
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}

void KPR_Files_readJSON(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFileMapping map = NULL;
	unsigned char *data;
	FskInt64 size;
	xsVars(2);
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	bailIfError(FskFileMap(path, &data, &size, 0, &map));
	xsVar(0) = xsGet(xsGlobal, xsID_JSON);
	xsVar(1) = xsNewInstanceOf(xsChunkPrototype);
	xsSetHostData(xsVar(1), data);
	xsSetHostDestructor(xsVar(1) , NULL);
	xsSet(xsVar(1), xsID_length, xsInteger(size));
	xsEnterSandbox();
	xsResult = xsCall1(xsVar(0), xsID_parse, xsVar(1));	
	xsLeaveSandbox();
bail:
	FskFileDisposeMap(map);
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}

void KPR_Files_readText(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFileMapping map = NULL;
	unsigned char *data;
	FskInt64 size;
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	bailIfError(FskFileMap(path, &data, &size, 0, &map));
	xsResult = xsStringBuffer((xsStringValue)data, (xsIntegerValue)size);
bail:
	FskFileDisposeMap(map);
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}		

void KPR_Files_readXML(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFileMapping map = NULL;
	unsigned char *data;
	FskInt64 size;
	xsVars(2);
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	bailIfError(FskFileMap(path, &data, &size, 0, &map));
	xsVar(0) = xsGet(xsGlobal, xsID_DOM);
	xsVar(1) = xsNewInstanceOf(xsChunkPrototype);
	xsSetHostDestructor(xsVar(1) , NULL);
	xsSetHostData(xsVar(1), data);
	xsSet(xsVar(1), xsID_length, xsInteger(size));
	xsResult = xsCall1(xsVar(0), xsID_parse, xsVar(1));	
bail:
	FskFileDisposeMap(map);
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}

void KPR_Files_writeChunk(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFile file = NULL;
	void* data = xsGetHostData(xsArg(1));
	xsIntegerValue size = xsToInteger(xsGet(xsArg(1), xsID_length));
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	err = FskFileOpen(path, kFskFilePermissionReadWrite, &file);
	if (kFskErrFileNotFound == err) {
		bailIfError(FskFileCreate(path));
		err = FskFileOpen(path, kFskFilePermissionReadWrite, &file);
	}
	else {
		FskInt64 zero = 0;
		err = FskFileSetSize(file, &zero);
	}
	bailIfError(err);
	bailIfError(FskFileWrite(file, size, data, NULL));
bail:
	FskFileClose(file);
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}

void KPR_Files_writeJSON(xsMachine* the)
{
	xsVars(2);
	xsEnterSandbox();
	xsVar(0) = xsGet(xsGlobal, xsID_JSON);
	xsVar(1) = xsCall1(xsVar(0), xsID_stringify, xsArg(1));
	xsLeaveSandbox();
	xsCall2(xsThis, xsID_writeText, xsArg(0), xsVar(1));
}

void KPR_Files_writeText(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFile file = NULL;
	char* data = xsToString(xsArg(1));
	xsIntegerValue size = FskStrLen(data);
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	err = FskFileOpen(path, kFskFilePermissionReadWrite, &file);
	if (kFskErrFileNotFound == err) {
		bailIfError(FskFileCreate(path));
		err = FskFileOpen(path, kFskFilePermissionReadWrite, &file);
	}
	else {
		FskInt64 zero = 0;
		err = FskFileSetSize(file, &zero);
	}
	bailIfError(err);
	bailIfError(FskFileWrite(file, size, data, NULL));
bail:
	FskFileClose(file);
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}		

void KPR_Files_writeXML(xsMachine* the)
{
	xsVars(2);
	xsVar(0) = xsGet(xsGlobal, xsID_DOM);
	xsVar(1) = xsCall1(xsVar(0), xsID_serialize, xsArg(1));
	xsCall2(xsThis, xsID_writeText, xsArg(0), xsVar(1));
}

void KPR_Files_appendText(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFile file = NULL;
	char* data = xsToString(xsArg(1));
	xsIntegerValue size = FskStrLen(data);
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	err = FskFileOpen(path, kFskFilePermissionReadWrite, &file);
	if (kFskErrFileNotFound == err) {
		bailIfError(FskFileCreate(path));
		err = FskFileOpen(path, kFskFilePermissionReadWrite, &file);
	}
	else {
		FskInt64 fileSize;
		bailIfError(FskFileGetSize(file, &fileSize));
		bailIfError(FskFileSetPosition(file, &fileSize));
	}
	bailIfError(err);
	bailIfError(FskFileWrite(file, size, data, NULL));
bail:
	FskFileClose(file);
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}		

void KPR_Files_appendChunk(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
	FskFile file = NULL;
	void* data = xsGetHostData(xsArg(1));
	xsIntegerValue size = xsToInteger(xsGet(xsArg(1), xsID_length));
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	err = FskFileOpen(path, kFskFilePermissionReadWrite, &file);
	if (kFskErrFileNotFound == err) {
		bailIfError(FskFileCreate(path));
		err = FskFileOpen(path, kFskFilePermissionReadWrite, &file);
	}
	else {
		FskInt64 fileSize;
		bailIfError(FskFileGetSize(file, &fileSize));
		bailIfError(FskFileSetPosition(file, &fileSize));
	}
	bailIfError(err);
	bailIfError(FskFileWrite(file, size, data, NULL));
bail:
	FskFileClose(file);
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}

void KPR_Files_iterator(void* data)
{
	FskDirectoryIteratorDispose(data);
}

void KPR_Files_Iterator(xsMachine* the)
{
	FskErr err = kFskErrNone;
	char* path = NULL;
    UInt32 flags = 0;
	FskDirectoryIterator iterator = NULL;
	bailIfError(KprURLToPath(xsToString(xsArg(0)), &path));
	if ((xsToInteger(xsArgc) > 1) && xsTest(xsArg(1))) // hidden
		flags = 1;
	bailIfError(FskDirectoryIteratorNew(path, &iterator, flags));
	xsSetHostData(xsThis, iterator);
bail:
	FskMemPtrDispose(path);
	xsThrowIfFskErr(err);
}

void KPR_Files_iterator_getNext(xsMachine* the)
{
	FskErr err = kFskErrNone;
	FskDirectoryIterator iterator = xsGetHostData(xsThis);
	xsVars(1);
	if (iterator) {
		char *name;
		UInt32 itemType;
		err = FskDirectoryIteratorGetNext(iterator, &name, &itemType);
		if (kFskErrNone == err) {
			xsResult = xsNewInstanceOf(xsObjectPrototype);
			xsNewHostProperty(xsResult, xsID_path, xsString(name), xsDefault, xsDontScript);
			xsVar(0) = xsGet(xsGlobal, xsID_Files);
			if (kFskDirectoryItemIsDirectory == itemType)
				xsVar(0) = xsGet(xsVar(0), xsID_directoryType);
			else if (kFskDirectoryItemIsFile == itemType)
				xsVar(0) = xsGet(xsVar(0), xsID_fileType);
			else
				xsVar(0) = xsGet(xsVar(0), xsID_linkType);
			xsNewHostProperty(xsResult, xsID_type, xsVar(0), xsDefault, xsDontScript);
			FskMemPtrDispose(name);
		}
		else {
			FskDirectoryIteratorDispose(iterator);
			xsSetHostData(xsThis, NULL);
		}
	}
	if (kFskErrIteratorComplete != err)
		xsThrowIfFskErr(err);
}

void KPR_Files_volume_iterator(void* data)
{
	FskVolumeIteratorDispose(data);
}

void KPR_Files_Volume_Iterator(xsMachine* the)
{
	FskErr err = kFskErrNone;
	FskVolumeIterator iterator = NULL;
	bailIfError(FskVolumeIteratorNew(&iterator));
	xsSetHostData(xsThis, iterator);
bail:
	xsThrowIfFskErr(err);
}

void KPR_Files_volume_iterator_getNext(xsMachine* the)
{
	FskErr err = kFskErrNone;
	FskVolumeIterator iterator = xsGetHostData(xsThis);
	if (iterator) {
		char *name;
		char *path;
		UInt32 id;
		err = FskVolumeIteratorGetNext(iterator, &id, &path, &name);
		if (kFskErrNone == err) {
			xsResult = xsNewInstanceOf(xsObjectPrototype);
			xsNewHostProperty(xsResult, xsID_name, xsString(name), xsDefault, xsDontScript);
			xsNewHostProperty(xsResult, xsID_path, xsString(path), xsDefault, xsDontScript);
			xsNewHostProperty(xsResult, xsID_id, xsInteger(id), xsDefault, xsDontScript);
			FskMemPtrDispose(name);
			FskMemPtrDispose(path);
		}
		else {
			FskVolumeIteratorDispose(iterator);
			xsSetHostData(xsThis, NULL);
		}
	}
	if (kFskErrIteratorComplete != err)
		xsThrowIfFskErr(err);
}

