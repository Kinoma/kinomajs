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
#define SUPPORT_SOCKET_STREAM	1

#define __FSKECMASCRIPT_PRIV__
#define __FSKNETUTILS_PRIV__
#include "FskECMAScript.h"
#ifdef KPR_CONFIG
#include "FskManifest.xs.h"
#define FskECMAScriptThrowIf(THE, ERROR) xsThrowIfFskErr(ERROR)
#else
#include "FskCore.xs.h"
#endif
#include "FskFiles.h"
#include "FskNetUtils.h"
#include "FskNetInterface.h"
#include "FskUtilities.h"
#include "FskHTTPClient.h"
#include "FskHTTPAuth.h"
#include "FskDIDLGenMedia.h"
#include "FskImage.h"
#include "FskMediaPlayer.h"
#include "FskThread.h"
#include "FskTextConvert.h"
#include "zlib.h"

#if TARGET_OS_WIN32
	#include "FskPlatformImplementation.h"
#endif

#define kXSMaxInt (0x80000000)

#ifndef KPR_CONFIG

/*
	File System
*/

void xs_FileSystem_getFileInfo(xsMachine *the)
{
	FskFileInfo itemInfo;

	if (xsToInteger(xsArgc) >= 1) {
		FskECMAScript vm = (FskECMAScript)xsGetContext(the);
		xsVars(1);

		if (kFskErrNone == FskFileGetFileInfo(xsToString(xsArg(0)), &itemInfo)) {
			char *fType;

			xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, vm->id_FileSystem), vm->id_moreFileInfo));
			if (itemInfo.filesize < kXSMaxInt)
				xsSet(xsResult, vm->id_size, xsInteger((xsIntegerValue)itemInfo.filesize));
			else
				xsSet(xsResult, vm->id_size, xsNumber((xsNumberValue)itemInfo.filesize));
			if (itemInfo.filetype == kFskDirectoryItemIsFile)
				fType = NULL;			// default value in prototype is "file"
			else if (itemInfo.filetype == kFskDirectoryItemIsDirectory)
				fType = "directory";
			else if (itemInfo.filetype == kFskDirectoryItemIsLink)
				fType = "link";
			else
				fType = NULL;

			if (NULL != fType)
				xsSet(xsResult, vm->id_type, xsGet(xsResult, xsID(fType)));

			// The ECMAScript date object constructor expects UTC values in milliseconds
			xsVar(0) = xsNew1(xsGlobal, vm->id_Date, xsNumber(itemInfo.fileCreationDate * 1000.0));
			xsSet(xsResult, vm->id_creationDate, xsVar(0));
			xsVar(0) = xsNew1(xsGlobal, vm->id_Date, xsNumber(itemInfo.fileModificationDate * 1000.0));
			xsSet(xsResult, vm->id_modificationDate, xsVar(0));

			if (kFileFileHidden & itemInfo.flags)
				xsSet(xsResult, vm->id_hidden, xsTrue);
			if (kFileFileLocked & itemInfo.flags)
				xsSet(xsResult, vm->id_locked, xsTrue);
		}
	}
}

void xs_FileSystem_simplifyPath(xsMachine* the)
{
#if TARGET_OS_WIN32
	WCHAR platform_simplified[1024];
	char* original = NULL;
	char* simplified = NULL;
	UInt16 *platform_original = NULL;

	xsResult = xsArg(0);
	original = xsToString(xsArg(0));
	if (!FskFilePathToNative(original, (char **)&platform_original)) {
		if (GetFullPathNameW((LPCWSTR)platform_original, sizeof(platform_simplified) / 2, platform_simplified, NULL)) {
			simplified = fixUpPathForFsk(platform_simplified);
			if (simplified) {
				xsResult = xsString(simplified);
				FskMemPtrDispose(simplified);
			}
			FskMemPtrDispose(platform_original);
		}
	}
#elif TARGET_OS_KPL
	xsResult = xsArg(0);	// @@
#else 
	int c0, c1;
	char* path0;
	char path1[PATH_MAX + 1];
	path0 = xsToString(xsArg(0));

	xsResult = xsArg(0);
	if (realpath(path0, path1)) {
		c0 = strlen(path0);
		c1 = strlen(path1);
		if ((path0[c0 - 1] == '/') && (path1[c1 - 1] != '/')) {
			path1[c1] = '/';
			path1[c1 + 1] = 0;
		}
		xsResult = xsString(path1);
	}
#endif
}

void xs_FileSystem_setFileInfo(xsMachine *the)
{
	if (xsToInteger(xsArgc) > 1) {
		char *path;
		FskFileInfo itemInfo;

		path = xsToStringCopy(xsArg(0));

		if (kFskErrNone == FskFileGetFileInfo(path, &itemInfo)) {
			FskErr err;

			if (xsToBoolean(xsGet(xsArg(1), xsID("locked"))))
				itemInfo.flags |= kFileFileLocked;
			else
				itemInfo.flags &= ~kFileFileLocked;

			if (xsToBoolean(xsGet(xsArg(1), xsID("hidden"))))
				itemInfo.flags |= kFileFileHidden;
			else
				itemInfo.flags &= ~kFileFileHidden;

			itemInfo.fileCreationDate = (UInt32)(xsToNumber(xsGet(xsArg(1), xsID("creationDate"))) / 1000.0);
			itemInfo.fileModificationDate = (UInt32)(xsToNumber(xsGet(xsArg(1), xsID("modificationDate"))) / 1000.0);

			err = FskFileSetFileInfo(path, &itemInfo);
			xsResult = xsBoolean(kFskErrNone == err);
		}

		FskMemPtrDispose(path);
	}
}

void xs_FileSystem_deleteFile(xsMachine *the)
{
	if (xsToInteger(xsArgc) >= 1) {
		FskErr err;

		err = FskFileDelete(xsToString(xsArg(0)));
		xsResult = xsBoolean(kFskErrNone == err);
		if (kFskErrFileNotFound != err)
			FskECMAScriptThrowIf(the, err);
	}
}

void xs_FileSystem_renameFile(xsMachine *the)
{
	if (xsToInteger(xsArgc) >= 2) {
		FskErr err;
		char *to = xsToStringCopy(xsArg(1));
		char *from = xsToString(xsArg(0));

		err = FskFileRename(from ,to);
		FskMemPtrDispose(to);
		xsResult = xsBoolean(kFskErrNone == err);
	}
}

void xs_FileSystem_resolveLink(xsMachine *the)
{
	char *path = xsToString(xsArg(0));
	char *resolved;

	if (kFskErrNone == FskFileResolveLink(path, &resolved)) {
		xsResult = xsString(resolved);
		FskMemPtrDispose(resolved);
	}
}

void xs_FileSystem_createDirectory(xsMachine *the)
{
	if (xsToInteger(xsArgc) >= 1) {
		FskErr err;

		err = FskFileCreateDirectory(xsToString(xsArg(0)));
		if (kFskErrFileExists == err)
			err = kFskErrNone;
		FskECMAScriptThrowIf(the, err);
	}
}

void xs_FileSystem_deleteDirectory(xsMachine *the)
{
	if (xsToInteger(xsArgc) >= 1) {
		FskErr err;

		err = FskFileDeleteDirectory(xsToString(xsArg(0)));
		xsResult = xsBoolean(kFskErrNone == err);
	}
}

void xs_FileSystem_renameDirectory(xsMachine *the)
{
	if (xsToInteger(xsArgc) >= 2) {
		FskErr err;
		char *to = xsToStringCopy(xsArg(1));
		char *from = xsToString(xsArg(0));

		err = FskFileRenameDirectory(from, to);
		FskMemPtrDispose(to);
		xsResult = xsBoolean(kFskErrNone == err);
	}
}

void xs_FileSystem_getVolumeInfo(xsMachine *the)
{
	FskErr err;
	Boolean haveID = false;
	UInt32 id;
	char *path = NULL, *name = NULL;
	UInt32 volumeType;
	Boolean isRemovable;
	FskInt64 free, capacity;
	Boolean requestDeviceInfo = false, requestVolumeType = true, requestIsRemovable = true;
	
	if (xsToInteger(xsArgc) >= 2) {
		UInt32 flags = xsToInteger(xsArg(1));
		if (1 & flags)
			requestDeviceInfo = true;
		
		requestVolumeType = 0 == (2 & flags);
		requestIsRemovable = 0 == (4 & flags);
	}

	if (xsStringType == xsTypeOf(xsArg(0))) {
		err = FskVolumeGetInfoFromPath(xsToString(xsArg(0)), &path, &name,
				requestVolumeType ? &volumeType : NULL, requestIsRemovable ? &isRemovable : NULL, &capacity, &free);
		if (kFskErrNone == err)
			haveID = kFskErrNone == FskVolumeGetID(path, &id);
	}
	else {
		id = xsToInteger(xsArg(0));
		haveID = true;
		err = FskVolumeGetInfo(id, &path, &name,
				requestVolumeType ? &volumeType : NULL, requestIsRemovable ? &isRemovable : NULL, &capacity, &free);
	}
	FskECMAScriptThrowIf(the, err);

	xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("FileSystem")), xsID("volumeInfo")));
	xsSet(xsResult, xsID("path"), xsString(path));
	xsSet(xsResult, xsID("name"), xsString(name));
	if (haveID)
		xsSet(xsResult, xsID("id"), xsInteger(id));
	if (requestIsRemovable)
		xsSet(xsResult, xsID("removable"), xsBoolean(isRemovable));
	xsSet(xsResult, xsID("capacity"), xsNumber((xsNumberValue)capacity));
	xsSet(xsResult, xsID("free"), xsNumber((xsNumberValue)free));
	if (requestVolumeType && (volumeType <= kFskVolumeTypeDirectory)) {
		static char *kinds[] = {"", "unknown", "fixed", "floppy", "optical", "CD", "DVD", "network", "Memory Stick", "MMC", "SD Memory", "Compact Flash", "SmartMedia", "directory"};
		xsSet(xsResult, xsID("kind"), xsString(kinds[volumeType]));
	}

	FskMemPtrDispose(path);
	FskMemPtrDispose(name);

	if (requestDeviceInfo && haveID) {
		char *vendor, *product, *revision, *vendorSpecific;

		err = FskVolumeGetDeviceInfo(id, &vendor, &product, &revision, &vendorSpecific);
		if (err) {
			xsSet(xsResult, xsID("device"), xsNull);	// set null rather than throwing an error
			return;
		}

		xsVars(1);
		xsVar(0) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("FileSystem")), xsID("deviceInfo")));
		if (vendor) {
			xsSet(xsVar(0), xsID("vendor"), xsString(vendor));
			FskMemPtrDispose(vendor);
		}
		if (product) {
			xsSet(xsVar(0), xsID("product"), xsString(product));
			FskMemPtrDispose(product);
		}
		if (revision) {
			xsSet(xsVar(0), xsID("revision"), xsString(revision));
			FskMemPtrDispose(revision);
		}
		if (vendorSpecific) {
			xsSet(xsVar(0), xsID("vendorSpecific"), xsString(vendorSpecific));
			FskMemPtrDispose(vendorSpecific);
		}

		xsSet(xsResult, xsID("device"), xsVar(0));
	}
}

void xs_FileSystem_ejectVolume(xsMachine *the)
{
	FskErr err = FskVolumeEject(xsToString(xsArg(0)), 0);
	xsResult = err ? xsFalse : xsTrue;
}

/*
	Thumbnail
*/

void xs_FileSystem_getThumbnail(xsMachine *the)
{
	int argc = xsToInteger(xsArgc);
	FskErr err;
	UInt32 width, height;
	FskBitmap thumbnail;

	if (argc >= 3) {
		width = xsToInteger(xsArg(1));
		height = xsToInteger(xsArg(2));
	}
	else
		width = height = 0;

	err = FskFileGetThumbnail(xsToString(xsArg(0)), width, height, &thumbnail);
	FskECMAScriptThrowIf(the, err);

	fskBitmapToXSBitmap(the, thumbnail, true, &xsResult);
}

/*
	Directory Iterator
*/

typedef struct {
	FskDirectoryIterator	dirIt;
	FskVolumeIterator		volIt;

	xsIndex					id_FileSystem;
	xsIndex					id_fileInfo;
	xsIndex					id_path;
	xsIndex					id_type;
	xsIndex					id_directory;
	xsIndex					id_link;
	xsIndex					id_name;
	xsIndex					id_volume;
	xsIndex					id_id;
} xsNativeFSIteratorRecord, *xsNativeFSIterator;

void xs_FileSystem_Iterator(xsMachine* the)
{
	xsNativeFSIterator fsit;
	FskErr err;
	const char *path;

	err = FskMemPtrNewClear(sizeof(xsNativeFSIteratorRecord), &fsit);
	FskECMAScriptThrowIf(the, err);
	xsSetHostData(xsResult, fsit);
	
	fsit->id_FileSystem = xsID("FileSystem");
	fsit->id_fileInfo = xsID("fileInfo");
	fsit->id_path = xsID("path");
	fsit->id_type = xsID("type");
	fsit->id_directory = xsID("directory");
	fsit->id_link = xsID("link");
	fsit->id_name = xsID("name");
	fsit->id_volume = xsID("volume");
	fsit->id_id = xsID("id");

	path = xsToInteger(xsArgc) ? xsToString(xsArg(0)) : NULL;
	if ((NULL == path) || (0 == *path)) {
		err = FskVolumeIteratorNew(&fsit->volIt);
		FskECMAScriptThrowIf(the, err);
//		FskInstrumentedItemSetOwner(fsit->volIt, (FskECMAScript)xsGetContext(the));
	}
	else {
		UInt32 flags = 0;
		if ((xsToInteger(xsArgc) > 1) && xsTest(xsArg(1)))
			flags = 1;
		err = FskDirectoryIteratorNew(path, &fsit->dirIt, flags);
		FskECMAScriptThrowIf(the, err);
		FskInstrumentedItemSetOwner(fsit->dirIt, (FskECMAScript)xsGetContext(the));
	}
}

void xs_FileSystem_iterator_destructor(void *it)
{
	if (it) {
		xsNativeFSIterator fsit = (xsNativeFSIterator)it;
		FskDirectoryIteratorDispose(fsit->dirIt);
		FskVolumeIteratorDispose(fsit->volIt);
		FskMemPtrDispose(fsit);
	}
}

void xs_FileSystem_iterator_getNext(xsMachine* the)
{
	xsNativeFSIterator fsit = (xsNativeFSIterator)xsGetHostData(xsThis);
	char *name;
	FskErr err;

	if (!fsit)
		;
	else
	if (fsit->dirIt) {
		UInt32 itemType;

		err = FskDirectoryIteratorGetNext(fsit->dirIt, &name, &itemType);
		if (kFskErrNone == err) {
			xsVars(1);
			xsVar(0) = xsGet(xsGet(xsGlobal, fsit->id_FileSystem), fsit->id_fileInfo);
			xsResult = xsNewInstanceOf(xsVar(0));
			xsSet(xsResult, fsit->id_path, xsString(name));
			if (kFskDirectoryItemIsDirectory == itemType)
				xsSet(xsResult, fsit->id_type, xsGet(xsVar(0), fsit->id_directory));
			else if (kFskDirectoryItemIsLink == itemType)
				xsSet(xsResult, fsit->id_type, xsGet(xsVar(0), fsit->id_link));
			FskMemPtrDispose(name);
		}
		else {
			xs_FileSystem_iterator_close(the);
			xsResult = xsNull;
			if (kFskErrIteratorComplete != err)
				FskECMAScriptThrowIf(the, err);
		}
	}
	else if (fsit->volIt) {
		UInt32 id;
		char *path;

		err = FskVolumeIteratorGetNext(fsit->volIt, &id, &path, &name);
		if (kFskErrNone == err) {
			xsVars(1);

			xsVar(0) = xsGet(xsGet(xsGlobal, fsit->id_FileSystem), fsit->id_fileInfo);
			xsResult = xsNewInstanceOf(xsVar(0));
			xsSet(xsResult, fsit->id_path, xsString(path));
			if (name)
				xsSet(xsResult, fsit->id_name, xsString(name));
			xsSet(xsResult, fsit->id_type, xsString("volume"));
			xsSet(xsResult, fsit->id_id, xsInteger(id));
			FskMemPtrDispose(name);
			FskMemPtrDispose(path);
		}
		else {
			xs_FileSystem_iterator_close(the);
			xsResult = xsNull;
			if (kFskErrIteratorComplete != err)
				FskECMAScriptThrowIf(the, err);
		}
	}
}

void xs_FileSystem_iterator_close(xsMachine* the)
{
	xsNativeFSIterator fsit = (xsNativeFSIterator)xsGetHostData(xsThis);
	if (fsit) {
		FskDirectoryIteratorDispose(fsit->dirIt);
		FskVolumeIteratorDispose(fsit->volIt);
		FskMemPtrDispose(fsit);
		xsSetHostData(xsThis, NULL);
	}
}

/*
	Special Directory
*/

void xs_FileSystem_getSpecialDirectory(xsMachine *the)
{
	FskErr err;
	char *path;
	char *typeString = xsToString(xsArg(0));
	UInt32 type, typeFlags = 0;
	Boolean create;
	char *volume;
	SInt32 argc = xsToInteger(xsArgc);

	if (0 == FskStrCompareWithLength(typeString, "Shared/", 7)) {
		typeString += 7;
		typeFlags = kFskDirectorySpecialTypeSharedFlag;
	}

	create = argc >= 2 ? xsTest(xsArg(1)) : false;
	volume = argc >= 3 ? xsToString(xsArg(2)) : NULL;

	if (FskStrCompare(typeString, "Preferences") == 0) {
		type = kFskDirectorySpecialTypeApplicationPreference;
		if ((argc < 2) || (xsUndefinedType == xsTypeOf(xsArg(1))))
			create = true;
	}
	else if (FskStrCompare(typeString, "Document") == 0)
		type = kFskDirectorySpecialTypeDocument;
	else if (FskStrCompare(typeString, "Photo") == 0)
		type = kFskDirectorySpecialTypePhoto;
	else if (FskStrCompare(typeString, "Music") == 0)
		type = kFskDirectorySpecialTypeMusic;
	else if (FskStrCompare(typeString, "Video") == 0)
		type = kFskDirectorySpecialTypeVideo;
	else if (FskStrCompare(typeString, "TV") == 0)
		type = kFskDirectorySpecialTypeTV;
	else if (FskStrCompare(typeString, "PreferencesRoot") == 0)
		type = kFskDirectorySpecialTypeApplicationPreferenceRoot;
	else if (FskStrCompare(typeString, "Temporary") == 0)
		type = kFskDirectorySpecialTypeTemporary;
	else if (FskStrCompare(typeString, "Download") == 0)
		type = kFskDirectorySpecialTypeDownload;
	else if (FskStrCompare(typeString, "Start") == 0)
		type = kFskDirectorySpecialTypeStartMenu;
	else if (FskStrCompare(typeString, "MusicSync") == 0)
		type = kFskDirectorySpecialTypeMusicSync;
	else if (FskStrCompare(typeString, "PhotoSync") == 0)
		type = kFskDirectorySpecialTypePhotoSync;
	else if (FskStrCompare(typeString, "VideoSync") == 0)
		type = kFskDirectorySpecialTypeVideoSync;
	else if (FskStrCompare(typeString, "PlaylistSync") == 0)
		type = kFskDirectorySpecialTypePlaylistSync;
	else
		return;

	err = FskDirectoryGetSpecialPath(type | typeFlags, create, volume, &path);
	if (kFskErrNone == err) {
		xsResult = xsString(path);
		FskMemPtrDispose(path);
	}
	else if (kFskErrInvalidParameter == err)
		;		// unknown directory
	else
		FskECMAScriptThrowIf(the, err);
}

/*
	volume notifier
*/

typedef struct {
	FskVolumeNotifier	notifier;
	FskECMAScript		vm;
	xsSlot				obj;
} xsNativeVolumeNotifierRecord, *xsNativeVolumeNotifier;

static FskErr volumeNotifier(UInt32 status, UInt32 volumeID, void *refCon);

void xs_FileSystem_Notifier_Volume(xsMachine *the)
{
	xsNativeVolumeNotifier nvn;
	FskErr err;

	err = FskMemPtrNewClear(sizeof(xsNativeVolumeNotifierRecord), &nvn);
	FskECMAScriptThrowIf(the, err);

	nvn->obj = xsThis;
	nvn->vm = (FskECMAScript)xsGetContext(the);
	err = FskVolumeNotifierNew(volumeNotifier, nvn, &nvn->notifier);
	if (err) {
		FskMemPtrDispose(nvn);
		FskECMAScriptThrowIf(the, err);
	}
	xsSetHostData(xsThis, nvn);
	FskInstrumentedItemSetOwner(nvn->notifier, nvn->vm);
}

void xs_FileSystem_notifier_volume_destructor(void *data)
{
	if (data) {
		xsNativeVolumeNotifier nvn = (xsNativeVolumeNotifier)data;
		FskVolumeNotifierDispose(nvn->notifier);
		FskMemPtrDispose(nvn);
	}
}

void xs_FileSystem_notifier_volume_close(xsMachine *the)
{
	xsNativeVolumeNotifier nvn = (xsNativeVolumeNotifier)xsGetHostData(xsThis);
	FskVolumeNotifierDispose(nvn->notifier);
	nvn->notifier = NULL;
}

FskErr volumeNotifier(UInt32 status, UInt32 volumeID, void *refCon)
{
	xsNativeVolumeNotifier nvn = (xsNativeVolumeNotifier)refCon;

	xsBeginHost(nvn->vm->the);
		xsCall1_noResult(nvn->obj, xsID((kFskVolumeHello == status) ? "onFound" : "onLost"), xsInteger(volumeID));
	xsEndHost(nvn->vm->the);

	return kFskErrNone;
}

/*
	directory change notifier
*/

typedef struct {
	FskDirectoryChangeNotifier	notifier;
	FskECMAScript				vm;
	xsSlot						obj;
} xsNativeDirectoryChangeNotifierRecord, *xsNativeDirectoryChangeNotifier;

static FskErr directoryChangeNotifier(UInt32 flags, const char *path, void *refCon);

void xs_FileSystem_change_Notifier(xsMachine *the)
{
	FskErr err;
	xsNativeDirectoryChangeNotifier ndc;
	UInt32 flags = 0;

	err = FskMemPtrNewClear(sizeof(xsNativeDirectoryChangeNotifierRecord), &ndc);
	BAIL_IF_ERR(err);

	ndc->obj = xsThis;
	ndc->vm = (FskECMAScript)xsGetContext(the);
	xsSetHostData(xsThis, ndc);

	if ((xsToInteger(xsArgc) > 1) && xsTest(xsArg(1)))
		flags |= kFskDirectoryChangeMonitorSubTree;

	err = FskDirectoryChangeNotifierNew(xsToString(xsArg(0)), flags, directoryChangeNotifier, ndc, &ndc->notifier);

bail:
	FskECMAScriptThrowIf(the, err);
}

void xs_FileSystem_change_notifier_destructor(void *data)
{
	if (NULL != data) {
		FskDirectoryChangeNotifierDispose(((xsNativeDirectoryChangeNotifier)data)->notifier);
		FskMemPtrDispose(data);
	}
}

void xs_FileSystem_change_notifier_close(xsMachine *the)
{
	xs_FileSystem_change_notifier_destructor(xsGetHostData(xsThis));
	xsSetHostData(xsThis, NULL);
}

FskErr directoryChangeNotifier(UInt32 flags, const char *path, void *refCon)
{
	xsNativeDirectoryChangeNotifier ndc = (xsNativeDirectoryChangeNotifier)refCon;

	xsBeginHost(ndc->vm->the);
		if (path)
			xsCall2_noResult(ndc->obj, xsID("onUpdate"), xsString((char *)path), xsInteger(flags));
		else
			xsCall2_noResult(ndc->obj, xsID("onUpdate"), xsNull, xsInteger(flags));
	xsEndHost(ndc->vm->the);

	return kFskErrNone;
}

void xs_FileSystem_load(xsMachine *the)
{
	void *data;
	FskInt64 dataSize;

	if (kFskErrNone == FskFileLoad(xsToString(xsArg(0)), (FskMemPtr *)(void*)&data, &dataSize)) {		// always puts a trailing null on result, so we can safely treat it as a string
		int argc = xsToInteger(xsArgc);
		if ((argc > 1) && xsTest(xsArg(1)))
			xsMemPtrToChunk(the, &xsResult, (FskMemPtr)data, (UInt32)dataSize, false);
		else {
			if (FskTextUTF8IsValid(data, (SInt32)dataSize))
				xsResult = xsString(data);
			FskMemPtrDispose(data);
		}
	}
}

/*
	HTTP Client
*/

static FskErr xs_HTTPClient_finished(FskHTTPClient client, void *refCon);

void xs_HTTPClient(xsMachine* the)
{
	FskErr err;
	xsNativeHTTPClient nht;
	SInt32 priority;

 	err = FskMemPtrNewClear(sizeof(xsNativeHTTPClientRecord), &nht);
	BAIL_IF_ERR(err);

	priority = kFskNetSocketDefaultPriority;
	if (xsToInteger(xsArgc) > 0)
		priority = xsToInteger(xsArg(0));
	err = FskHTTPClientNewPrioritized(&nht->client, priority, "xs_HTTPClient");
	BAIL_IF_ERR(err);

	FskHTTPClientSetRefCon(nht->client, nht);

	nht->obj = xsThis;
	nht->vm = (FskECMAScript)xsGetContext(the);
	nht->useCount = 1;

	xsSetHostData(xsThis, nht);

	FskHTTPClientSetFinishedCallback(nht->client, xs_HTTPClient_finished);

bail:
	if (err) {
		xs_HTTPClient_destructor(nht);
		FskECMAScriptThrowIf(the, err);
	}
}

void xs_HTTPClient_destructor(void *ref)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)ref;

	if (nht) {
		nht->useCount -= 1;
		if (0 == nht->useCount) {
			FskHTTPClientDispose(nht->client);
			FskMemPtrDispose(nht);
		}
	}
}

void xs_HTTPClient_close(xsMachine* the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);

	if (!nht)
		return;

	FskHTTPClientCancel(nht->client);

	xs_HTTPClient_destructor(nht);
	xsSetHostData(xsThis, NULL);
}

void xs_HTTPClient_suspend(xsMachine* the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);
	FskHTTPClientSuspend(nht->client);
}

void xs_HTTPClient_resume(xsMachine* the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);
	FskHTTPClientResume(nht->client);
}

FskErr xs_HTTPClient_finished(FskHTTPClient client, void *refCon)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)refCon;
	xsMachine *the = nht->vm->the;

	xsBeginHost(the);
		xsCall1_noResult(nht->obj, xsID("onComplete"), xsInteger(client->status.lastErr));
	xsEndHost(the);

	return kFskErrNone;		// please don't dispose us
}
	
void xs_HTTPClient_isFinished(xsMachine* the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);

	if (FskHTTPClientIsIdle(nht->client))
		xsResult = xsTrue;
	else {
		xsResult = xsFalse;
		FskThreadYield();
		FskThreadRunloopCycle(100);
	}
}

#define kHTTPClientPostBufferSize (8192)

static FskErr xs_HTTPRequest_sendRequestData(FskHTTPClientRequest req, char **buffer, int *bufferSize, FskInt64 position, void *refCon);
static FskErr xs_HTTPRequest_responseHeaderCallback(FskHTTPClientRequest req, FskHeaders *responseHeaders, void *refCon);
static FskErr xs_HTTPRequest_receiveDataCallback(FskHTTPClientRequest req, char *buffer, int bufferSize, void *refCon);
static FskErr xs_HTTPRequest_requestFinishedCallback(FskHTTPClient client, FskHTTPClientRequest req, void *refCon);

static void xs_HTTPRequest_empty(xsNativeHTTPRequest nhr);

void xs_HTTPClient_setUserPass(xsMachine* the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);
	char *user, *pass;
	int passType = kFskHTTPAuthCredentialsTypeString;
	int argc = xsToInteger(xsArgc);

	if (0 == argc) {
		FskHTTPClientSetCredentials(nht->client, NULL, NULL, 0, kFskHTTPAuthCredentialsTypeNone);
		return;
	}

	if (argc > 1) {
		user = xsToString(xsArg(0));
		pass = xsToString(xsArg(1));
		if (argc > 2) {
			passType = xsToInteger(xsArg(2));
		}
/* Note: - if type isn't TypeString, then the size should be specified below */
		FskHTTPClientSetCredentials(nht->client, user, pass, 0, passType);
	}
}

void xs_HTTPClient_setCertificates(xsMachine *the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);
	void *certs = NULL, *certsString = NULL;
	int certsSize = 0;
	char *policies = NULL;

	if (ac < 1)
		return;
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		certs = certsString = xsToStringCopy(xsArg(0));
		certsSize = FskStrLen(certs);
	}
	else if (xsIsInstanceOf(xsArg(0), xsChunkPrototype)) {
		certs = xsGetHostData(xsArg(0));
		certsSize = xsToInteger(xsGet(xsArg(0), xsID("length")));
	}
	if (ac >= 2 && xsTypeOf(xsArg(1)) == xsStringType)
		policies = xsToString(xsArg(1));
	FskHTTPClientSetCertificates(nht->client, certs, certsSize, policies);
	if (certsString != NULL)
		FskMemPtrDispose(certsString);
}

void xs_HTTPClient_setTimeout(xsMachine* the)
{
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);
	if (!nht)
		FskECMAScriptThrowIf(the, kFskErrOutOfSequence);
	FskHTTPClientSetIdleTimeout(nht->client, xsToInteger(xsArg(0)));
}

void xs_HTTPClient_request(xsMachine* the)
{	
	xsNativeHTTPClient nht = (xsNativeHTTPClient)xsGetHostData(xsThis);
	xsNativeHTTPRequest nhr;
	FskErr err;
	char *url;

	xsVars(1);

	xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("FskHTTP")), xsID("request")));

	url = xsToString(xsArg(0));

	err = FskMemPtrNewClear(sizeof(xsNativeHTTPRequestRecord), &nhr);
	BAIL_IF_ERR(err);

	xsSetHostData(xsResult, nhr);

	nhr->nht = nht;
	nhr->nht->useCount += 1;
	nhr->vm = nht->vm;
	nhr->obj = xsResult;

	err = FskHTTPClientRequestNew(&nhr->req, url);
	BAIL_IF_ERR(err);

	FskHTTPClientRequestSetRefCon(nhr->req, nhr);
	FskHTTPClientRequestSetReceivedResponseHeadersCallback(nhr->req, xs_HTTPRequest_responseHeaderCallback, kHTTPClientResponseHeadersOnRedirect);
	FskHTTPClientRequestSetReceivedDataCallback(nhr->req, xs_HTTPRequest_receiveDataCallback, NULL, 32768, kFskHTTPClientReadAnyData);
	FskHTTPClientRequestSetFinishedCallback(nhr->req, xs_HTTPRequest_requestFinishedCallback);

	xsSet(xsResult, nht->vm->id_stream, xsNew0(xsGet(xsGlobal, xsID("Stream")), nht->vm->id_Chunk));
	xsVar(0) = xsNew0(xsGlobal, nht->vm->id_Chunk);
	xsSetHostDestructor(xsVar(0), NULL);
	xsSet(xsResult, nht->vm->id_buffer, xsVar(0));

bail:
	FskECMAScriptThrowIf(the, err);

	xsRemember(nhr->obj);
	nhr->needsForget = true;
}

void xs_HTTPRequest_destructor(void *data)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)data;

	xs_HTTPRequest_empty(nhr);

	FskMemPtrDispose(nhr);
}

void xs_HTTPRequest_empty(xsNativeHTTPRequest nhr)
{
	if (NULL == nhr)
		return;

	if (0 == nhr->stage) {
		FskHTTPClientRequestDispose(nhr->req);
		nhr->req = NULL;
	}
	FskMemPtrDisposeAt((void **)(void*)&nhr->postBuffer);

	xs_HTTPClient_destructor(nhr->nht);
	nhr->nht = NULL;
}

void xs_HTTPRequest_close(xsMachine *the)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)xsGetHostData(xsThis);
	if (NULL == nhr)
		return;
	if (nhr->needsForget) {
		nhr->needsForget = false;
		xsForget(nhr->obj);
	}
	if (1 == nhr->stage)
		nhr->stage = 0;			// force to not-yet-added stage to force us to call dispose
	xs_HTTPRequest_empty(nhr);
}

void xs_HTTPRequest_setRequestMethod(xsMachine* the)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)xsGetHostData(xsThis);
	FskHTTPClientRequestSetMethod(nhr->req, xsToString(xsArg(0)));
}

void xs_HTTPRequest_addHeader(xsMachine *the)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)xsGetHostData(xsThis);
	char *name = xsToStringCopy(xsArg(0));
	char *value = xsToString(xsArg(1));
	if (0 == FskStrCompareCaseInsensitive(name, "Content-Length"))
		nhr->hasContentLength = true;
	else
	if (0 == FskStrCompareCaseInsensitive(name, "User-Agent"))
		nhr->hasUserAgent = true;
	FskHTTPClientRequestAddHeader(nhr->req, name, value);
	FskMemPtrDispose(name);
}

void xs_HTTPRequest_getHeader(xsMachine *the)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)xsGetHostData(xsThis);
	char *header;

	header = FskHeaderFind(xsToString(xsArg(0)), nhr->req->responseHeaders);
	if (header)
		xsResult = xsString(header);
}

void xs_HTTPRequest_start(xsMachine* the)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)xsGetHostData(xsThis);
	FskErr err;
	SInt32 argc = xsToInteger(xsArgc);
	Boolean havePostStream = false;
	Boolean doStart = true;

	if (0 != nhr->stage)
		FskECMAScriptThrowIf(the, kFskErrOutOfSequence);

	if (argc > 0) {
		if (xsReferenceType == xsTypeOf(xsArg(0)))
			havePostStream = true;
		else if (1 == argc)
			doStart = xsToBoolean(xsArg(0));

		if (argc >= 2)
			doStart = xsToBoolean(xsArg(1));
	}

	if (havePostStream) {
		// request message body stream
		xsSet(xsThis, xsID("postStream"), xsArg(0));
		FskHTTPClientRequestSetSendDataCallback(nhr->req, xs_HTTPRequest_sendRequestData);

		if (false == nhr->hasContentLength) {
			StreamBufferPtr aStreamPtr = (StreamBufferPtr)xsGetHostData(xsArg(0));
			if (0 != aStreamPtr->bytesAvailable) {
				char str[40];
				FskStrNumToStr((UInt32)aStreamPtr->bytesAvailable, str, sizeof(str));
				FskHTTPClientRequestAddHeader(nhr->req, "Content-Length", str);
			}
		}
	}

	if (false == nhr->hasUserAgent) {
		char *userAgent = FskEnvironmentGet("http-user-agent");
		if (NULL != userAgent) {
			FskHTTPClientRequestRemoveHeader(nhr->req, "User-Agent");
			FskHTTPClientRequestAddHeader(nhr->req, "User-Agent", userAgent);
		}
	}

	err = FskHTTPClientAddRequest(nhr->nht->client, nhr->req);
	FskECMAScriptThrowIf(the, err);

	nhr->stage = 1;

	if (doStart) {
		err = FskHTTPClientBegin(nhr->nht->client);
		FskECMAScriptThrowIf(the, err);
	}
}

FskErr xs_HTTPRequest_sendRequestData(FskHTTPClientRequest req, char **buffer, int *bufferSize, FskInt64 position, void *refCon)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)refCon;
	FskErr err = kFskErrNone;
	{
		xsBeginHost(nhr->vm->the);
		{
			xsVars(3);

			// out with the old
			FskMemPtrDisposeAt((void **)(void*)&nhr->postBuffer);

			// in with the new
			xsVar(0) = xsGet(nhr->obj, xsID("postStream"));
			if (0 == position) {
				xsCall0_noResult(xsVar(0), xsID("rewind"));
			}
			else {
				// what to do when position isn't 0?
			}

			xsVar(1) = xsCall1(xsVar(0), nhr->vm->id_readChunk, xsInteger(kHTTPClientPostBufferSize));
			if (xsReferenceType == xsTypeOf(xsVar(1))) {
				*bufferSize = xsToInteger(xsGet(xsVar(1), nhr->vm->id_length));
				nhr->postBuffer = (char *)xsGetHostData(xsVar(1));

				xsSetHostData(xsVar(1), NULL);
				xsSet(xsVar(1), nhr->vm->id_length, xsInteger(0));

				*buffer = nhr->postBuffer;
			}
			else {
				xsCall0_noResult(xsVar(0), xsID("rewind"));
				err = kFskErrEndOfFile;	// all done
			}
			
			xsCall1_noResult(nhr->obj, xsID("onDataSent"), xsInteger((xsIntegerValue)req->status.bytesSent));
		}
		xsEndHost(nhr->vm->the);
	}

	return err;
}

FskErr xs_HTTPRequest_responseHeaderCallback(FskHTTPClientRequest req, FskHeaders *responseHeaders, void *refCon)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)refCon;
	FskErr err = kFskErrNone;
	UInt32 typeOf;
	FskECMAScript vm = nhr->vm;

	xsBeginHost(vm->the);
	{
		int response = FskHeaderResponseCode(responseHeaders);
		Boolean isRedirect = (301 == response) || (302 == response) || (305 == response) || (307 == response);
		if (false == isRedirect) {
			// if there's a MIME type in the header, attach it to the stream object
			char *mimeType = FskHeaderFind(kFskStrContentType, responseHeaders);
			if (mimeType)
				xsSet(xsGet(nhr->obj, vm->id_stream), xsID("mime"), xsString(mimeType));
		}
		else
			nhr->hasUserAgent = false;

		xsSet(nhr->obj, xsID("statusCode"), xsInteger(FskHeaderResponseCode(req->responseHeaders)));
		xsResult = xsCall0(nhr->obj, xsID("onHeaders"));

		typeOf = xsTypeOf(xsResult);
		if ((xsNumberType == typeOf) || (xsIntegerType == typeOf))
			err = xsToInteger(xsResult);

		if (isRedirect && nhr->req && (false == nhr->hasUserAgent)) {
			char *userAgent = FskEnvironmentGet("http-user-agent");
			if (NULL != userAgent) {
				FskHTTPClientRequestRemoveHeader(nhr->req, "User-Agent");
				FskHTTPClientRequestAddHeader(req, "User-Agent", userAgent);
			}
		}
	}
	xsEndHost(vm->the);

	return err;
}

FskErr xs_HTTPRequest_receiveDataCallback(FskHTTPClientRequest req, char *buffer, int bufferSize, void *refCon)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)refCon;
	FskErr err = kFskErrNone;
	xsBeginHost(nhr->vm->the);
	{
		xsVars(1);

		xsVar(0) = xsGet(nhr->obj, nhr->vm->id_buffer);
		xsSetHostData(xsVar(0), buffer);
		xsSet(xsVar(0), nhr->vm->id_length, xsInteger(bufferSize));

		xsResult = xsCall1(nhr->obj, xsID("onDataReady"), xsVar(0));
		err = xsToInteger(xsResult);
	}
	xsEndHost(nhr->vm->the);

	return err;
}

FskErr xs_HTTPRequest_requestFinishedCallback(FskHTTPClient client, FskHTTPClientRequest req, void *refCon)
{
	xsNativeHTTPRequest nhr = (xsNativeHTTPRequest)refCon;

	nhr->stage = 2;

	xsBeginHost(nhr->vm->the);
	{
		xsTry {
			xsIndex id_status = xsID("status");

			xsForget(nhr->obj);
			nhr->needsForget = false;

			xsSet(nhr->obj, id_status, xsInteger(client->status.lastErr));
			if (req->responseHeaders->headersParsed)
				xsSet(nhr->obj, xsID("statusCode"), xsInteger(FskHeaderResponseCode(req->responseHeaders)));

			xsVars(1);
			xsVar(0) = xsGet(nhr->obj, nhr->vm->id_stream);
			xsCall0_noResult(xsVar(0), xsID("flush"));
			xsCall0_noResult(xsVar(0), xsID("rewind"));
			xsResult = xsCall0(nhr->obj, xsID("onTransferComplete"));

			xsVar(0) = xsGet(nhr->obj, nhr->vm->id_stream);
			xsCall0_noResult(xsVar(0), nhr->vm->id_close);
		}
		xsCatch {
		}
	}

	xsEndHost(nhr->vm->the);

	return kFskErrNone;
}

#if TARGET_OS_WIN32
	#include "FskTextConvert.h"
	#include <wininet.h>

	enum
	{
		kHTTPSStateIdle = 0,
		kHTTPSStateConnecting,
		kHTTPSStateWaitingForResponse,
		kHTTPSStateReading,
		kHTTPSStateClosing,
		kHTTPSStateClosed
	};

	const UInt16 *kHTTPSVersion = L"HTTPS/1.1";

	struct xsNativeHTTPSRecord {
		struct xsNativeHTTPSRecord *next;
		xsMachine	*the;
		xsSlot		obj;
		FskMutex	mutex;
		FskThread	thread;
		FskResolver	resolver;
		HINTERNET	internetOpenHandle;
		HINTERNET	internetConnectHandle;
		HINTERNET	httpRequestHandle;
		DWORD		statusCode;
		UInt16		*pathW;
		UInt16		*methodW;
		UInt16		*userAgentW;
		xsSlot		stream;
		xsSlot		postStream;
		void		*postChunk;
		UInt32		postChunkLength;
		UInt32		state;
		char		**requestHeaders;
		UInt32		requestHeaderCount;
		Boolean		hasContentLength;
		Boolean		didResolve;
		Boolean		gotHeaders;
		SInt16		count;
	};

	typedef struct xsNativeHTTPSRecord xsNativeHTTPSRecord;
	typedef xsNativeHTTPSRecord *xsNativeHTTPS;

	FskListMutex gHTTPS;

	void HTTPSResolverCallback(FskResolver rr);
	void CALLBACK HTTPSStatusCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);
	void HTTPSOpenRequest(void *param1, void *param2, void *param3, void *param4);
	void HTTPSSendRequest(void *param1, void *param2, void *param3, void *param4);
	void HTTPSResponseReceived(void *param1, void *param2, void *param3, void *param4);
	void HTTPSRequestComplete(void *param1, void *param2, void *param3, void *param4);
	void HTTPSClosingConnection(void *param1, void *param2, void *param3, void *param4);

	void HTTPSClose(xsNativeHTTPS nativeHTTPS);

	FskErr HTTPSAddHeader(xsNativeHTTPS nativeHTTPS, char *header, char *value);
	FskErr HTTPSGetHeaderValue(xsNativeHTTPS nativeHTTPS, char *header, char **value);
#endif

void xs_HTTPS_init(xsMachine *the)
{
}

void xs_HTTPS_initialize(xsMachine *the)
{
#if TARGET_OS_WIN32
	FskErr			err = kFskErrNone;
	xsNativeHTTPS	nativeHTTPS;

	if (!gHTTPS) {
		err = FskListMutexNew(&gHTTPS, "https");
		FskECMAScriptThrowIf(the, err);
	}

	err = FskMemPtrNewClear(sizeof(xsNativeHTTPSRecord), &nativeHTTPS);
	BAIL_IF_ERR(err);

	nativeHTTPS->the = the;
	nativeHTTPS->obj = xsThis;
	nativeHTTPS->thread = FskThreadGetCurrent();
	nativeHTTPS->state = kHTTPSStateIdle;
	nativeHTTPS->count = 1;

	nativeHTTPS->stream = xsNew0(xsGet(xsGlobal, xsID("Stream")), xsID("Chunk"));
	xsSet(nativeHTTPS->obj, xsID("stream"), nativeHTTPS->stream);

	err = FskMutexNew(&nativeHTTPS->mutex, "nativeHTTPS");
	BAIL_IF_ERR(err);

	FskListMutexAppend(gHTTPS, nativeHTTPS);

	xsSetHostData(xsThis, nativeHTTPS);

bail:
	if (err && nativeHTTPS) {
		FskMutexDispose(nativeHTTPS->mutex);
		FskMemPtrDispose(nativeHTTPS);
	}

	FskECMAScriptThrowIf(the, err);
#endif
}

void xs_HTTPS_destructor(void *hostData)
{
#if TARGET_OS_WIN32
	xsNativeHTTPS nativeHTTPS = hostData;

	if (nativeHTTPS)
		HTTPSClose(nativeHTTPS);
#endif
}

void xs_HTTPS_start(xsMachine *the)
{
#if TARGET_OS_WIN32
	FskErr			err = kFskErrNone;
	xsNativeHTTPS	nativeHTTPS = NULL;
	char			*path, *method, *userAgent, contentLength[40];
	StreamBufferPtr streamBufferPtr;
	UInt32			bytesToRead;

	nativeHTTPS = xsGetHostData(xsThis);

	if (nativeHTTPS->state != kHTTPSStateIdle) {
		BAIL(kFskErrBadState);
	}

	nativeHTTPS->state = kHTTPSStateConnecting;

	if (xsToInteger(xsArgc) >= 1) {
		streamBufferPtr = (StreamBufferPtr)xsGetHostData(xsArg(0));

		xsSet(xsThis, xsID("_postStream"), xsArg(0));

		xsCall0_noResult(xsArg(0), xsID("rewind"));
		xsResult = xsCall0(xsArg(0), xsID("toChunk"));
		nativeHTTPS->postChunk = xsGetHostData(xsResult);
		bytesToRead = xsToInteger(xsGet(xsResult, xsID("length")));
		nativeHTTPS->postChunkLength = bytesToRead;
		xsSetHostData(xsResult, NULL);

		if (!nativeHTTPS->hasContentLength && bytesToRead) {
			FskStrNumToStr(bytesToRead, contentLength, sizeof(contentLength));

			HTTPSAddHeader(nativeHTTPS, "Content-Length", contentLength);
		}
	}

	path = xsToString(xsGet(xsThis, xsID("_path")));
	method = xsToString(xsGet(xsThis, xsID("_method")));
	userAgent = FskEnvironmentGet("http-user-agent");

	nativeHTTPS->statusCode = 0;

	if (path) {
		err = FskTextUTF8ToUnicode16LE(path, FskStrLen(path), &nativeHTTPS->pathW, NULL);
		BAIL_IF_ERR(err);
	}

	if (method) {
		err = FskTextUTF8ToUnicode16LE(method, FskStrLen(method), &nativeHTTPS->methodW, NULL);
		BAIL_IF_ERR(err);
	}
	else {
		err = FskMemPtrNewFromData(8, L"GET", &nativeHTTPS->methodW);
		BAIL_IF_ERR(err);
	}

	err = FskTextUTF8ToUnicode16LE(userAgent, FskStrLen(userAgent), &nativeHTTPS->userAgentW, NULL);
	BAIL_IF_ERR(err);

	nativeHTTPS->count += 1;
	err = FskNetHostnameResolveAsync(NULL, HTTPSResolverCallback, nativeHTTPS, &nativeHTTPS->resolver);
	BAIL_IF_ERR(err);

bail:
	FskECMAScriptThrowIf(the, err);
#endif
}

void xs_HTTPS_addHeader(xsMachine *the)
{
#if TARGET_OS_WIN32
	FskErr			err = kFskErrNone;
	xsNativeHTTPS	nativeHTTPS = NULL;
	char			*header, *value;

	nativeHTTPS = xsGetHostData(xsThis);

	if (xsToInteger(xsArgc) < 2) {
		BAIL(kFskErrInvalidParameter);
	}

	header = xsToStringCopy(xsArg(0));
	value = xsToStringCopy(xsArg(1));

	err = HTTPSAddHeader(nativeHTTPS, header, value);
	FskMemPtrDispose(header);
	FskMemPtrDispose(value);
	BAIL_IF_ERR(err);

bail:
	FskECMAScriptThrowIf(the, err);
#endif
}

void xs_HTTPS_getHeader(xsMachine *the)
{
#if TARGET_OS_WIN32
	FskErr			err = kFskErrNone;
	xsNativeHTTPS	nativeHTTPS;
	char			*header, *value = NULL;

	nativeHTTPS = xsGetHostData(xsThis);

	if (xsToInteger(xsArgc) < 1) {
		xsResult = xsNull;

		return;
	}

	header = xsToString(xsArg(0));

	err = HTTPSGetHeaderValue(nativeHTTPS, header, &value);
	BAIL_IF_ERR(err);

	if (value)
		xsResult = xsString(value);
	else
		xsResult = xsNull;

bail:
	FskMemPtrDispose(value);
#endif
}

void xs_HTTPS_close(xsMachine *the)
{
#if TARGET_OS_WIN32
	xsNativeHTTPS nativeHTTPS;

	nativeHTTPS = xsGetHostData(xsThis);
	xsSetHostData(xsThis, NULL);

	xs_HTTPS_destructor(nativeHTTPS);
#endif
}

#if TARGET_OS_WIN32
void HTTPSResolverCallback(FskResolver rr)
{
	FskErr			err = kFskErrNone;
	xsNativeHTTPS	nativeHTTPS;
	char			*host;
	UInt16			*hostW = NULL;

	nativeHTTPS = rr->ref;

	if (nativeHTTPS->state == kHTTPSStateClosing) {
		HTTPSClose(nativeHTTPS);
		return;
	}
	else
		nativeHTTPS->count -= 1;

	if (rr->err == kFskErrNone) {
		xsBeginHost(nativeHTTPS->the);
			host = xsToString(xsGet(nativeHTTPS->obj, xsID("_host")));
			err = FskTextUTF8ToUnicode16LE(host, FskStrLen(host), &hostW, NULL);
		xsEndHost(nativeHTTPS->the);

		BAIL_IF_ERR(err);

		nativeHTTPS->internetOpenHandle = InternetOpenW(nativeHTTPS->userAgentW, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_ASYNC);

		InternetSetStatusCallback(nativeHTTPS->internetOpenHandle, HTTPSStatusCallback);

		nativeHTTPS->state = kHTTPSStateWaitingForResponse;
		nativeHTTPS->didResolve = true;

		InternetConnectW(nativeHTTPS->internetOpenHandle, hostW, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, (DWORD)nativeHTTPS);
	}
	else
		nativeHTTPS->state = kHTTPSStateClosed;

	nativeHTTPS->resolver = NULL;

bail:
	FskMemPtrDispose(hostW);
}

void CALLBACK HTTPSStatusCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
{
	xsNativeHTTPS nativeHTTPS;

	nativeHTTPS = (xsNativeHTTPS)dwContext;
	if (!FskListMutexContains(gHTTPS, nativeHTTPS))
		return;

	FskMutexAcquire(nativeHTTPS->mutex);

	switch (dwInternetStatus) {
		case INTERNET_STATUS_HANDLE_CREATED:
			if ((hInternet == nativeHTTPS->internetOpenHandle) && (nativeHTTPS->internetConnectHandle == NULL)) {
				nativeHTTPS->internetConnectHandle = (HINTERNET)(((INTERNET_ASYNC_RESULT *)lpvStatusInformation)->dwResult);
				nativeHTTPS->count += 1;

				FskThreadPostCallback(nativeHTTPS->thread, HTTPSOpenRequest, nativeHTTPS, NULL, NULL, NULL);
			}
			else if ((hInternet == nativeHTTPS->internetConnectHandle) && (nativeHTTPS->httpRequestHandle == NULL)) {
				nativeHTTPS->httpRequestHandle = (HINTERNET)(((INTERNET_ASYNC_RESULT *)lpvStatusInformation)->dwResult);
				nativeHTTPS->count += 1;

				FskThreadPostCallback(nativeHTTPS->thread, HTTPSSendRequest, nativeHTTPS, NULL, NULL, NULL);
			}
			break;
		case INTERNET_STATUS_RESPONSE_RECEIVED:
			if (nativeHTTPS->state == kHTTPSStateWaitingForResponse) {
				nativeHTTPS->state = kHTTPSStateReading;
				FskThreadPostCallback(nativeHTTPS->thread, HTTPSResponseReceived, nativeHTTPS, NULL, NULL, NULL);
			}
			break;
		case INTERNET_STATUS_REQUEST_COMPLETE:
			if (nativeHTTPS->state != kHTTPSStateClosing) {
				INTERNET_ASYNC_RESULT *iar = lpvStatusInformation;
				FskThreadPostCallback(nativeHTTPS->thread, HTTPSRequestComplete, nativeHTTPS, (void *)(iar ? iar->dwError : ERROR_SUCCESS), NULL, NULL);
			}
			break;

		case INTERNET_STATUS_HANDLE_CLOSING:
			FskMutexRelease(nativeHTTPS->mutex);
			HTTPSClose(nativeHTTPS);
			return;

		case INTERNET_STATUS_CLOSING_CONNECTION:
			if ((nativeHTTPS->state != kHTTPSStateReading) && (nativeHTTPS->state != kHTTPSStateClosing))
				FskThreadPostCallback(nativeHTTPS->thread, HTTPSClosingConnection, nativeHTTPS, NULL, NULL, NULL);
			break;
	}

	FskMutexRelease(nativeHTTPS->mutex);
}

void HTTPSOpenRequest(void *param1, void *param2, void *param3, void *param4)
{
	xsNativeHTTPS	nativeHTTPS;
	Boolean			close = false;

	nativeHTTPS = param1;

	if (!FskListMutexContains(gHTTPS, nativeHTTPS))
		return;

	HttpOpenRequestW(nativeHTTPS->internetConnectHandle, nativeHTTPS->methodW, nativeHTTPS->pathW, kHTTPSVersion, NULL, NULL, (INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_NO_CACHE_WRITE), (DWORD)nativeHTTPS);
}

void HTTPSSendRequest(void *param1, void *param2, void *param3, void *param4)
{
	FskErr			err = kFskErrNone;
	xsNativeHTTPS	nativeHTTPS;
	DWORD			securityFlags, bufferLength;
	char			*headers = NULL;
	UInt16			*headersW = NULL;
	UInt32			headersLength = 0, headerLength, valueLength, index;
	Boolean			close = false;

	nativeHTTPS = param1;
	bufferLength = sizeof(securityFlags);

	InternetQueryOption(nativeHTTPS->httpRequestHandle, INTERNET_OPTION_SECURITY_FLAGS, &securityFlags, &bufferLength);
	securityFlags |= (SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID);
	InternetSetOption(nativeHTTPS->httpRequestHandle, INTERNET_OPTION_SECURITY_FLAGS, &securityFlags, bufferLength);

	if (nativeHTTPS->requestHeaderCount > 0) {
		for (index = 0; index < nativeHTTPS->requestHeaderCount; index++) {
			headerLength = FskStrLen(nativeHTTPS->requestHeaders[index * 2]);
			valueLength = FskStrLen(nativeHTTPS->requestHeaders[(index * 2) + 1]);

			err = FskMemPtrRealloc(headersLength + headerLength + valueLength + 4 + 1, &headers);
			BAIL_IF_ERR(err);

			if (headersLength == 0)
				headers[0] = 0;

			FskStrCat(headers, nativeHTTPS->requestHeaders[index * 2]);
			FskStrCat(headers, ": ");
			FskStrCat(headers, nativeHTTPS->requestHeaders[(index * 2) + 1]);
			FskStrCat(headers, "\r\n");

			headersLength += headerLength + valueLength + 4;
		}

		err = FskTextUTF8ToUnicode16LE(headers, FskStrLen(headers), &headersW, NULL);
		BAIL_IF_ERR(err);

		HttpAddRequestHeadersW(nativeHTTPS->httpRequestHandle, headersW, wcslen(headersW), (HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE));
	}

	if (nativeHTTPS->postChunk) {
		xsBeginHost(nativeHTTPS->the);
			HttpSendRequestW(nativeHTTPS->httpRequestHandle, NULL, 0, nativeHTTPS->postChunk, nativeHTTPS->postChunkLength);

			xsCall1_noResult(nativeHTTPS->obj, xsID("onDataSent"), xsInteger(nativeHTTPS->postChunkLength));
		xsEndHost(nativeHTTPS->the);
	}
	else
		HttpSendRequestW(nativeHTTPS->httpRequestHandle, NULL, 0, NULL, 0);

bail:
	FskMemPtrDispose(headers);
	FskMemPtrDispose(headersW);
}

void HTTPSResponseReceived(void *param1, void *param2, void *param3, void *param4)
{
	xsNativeHTTPS	nativeHTTPS;
	xsMachine		*the;
	DWORD			statusCodeSize;
	char			*value = NULL;
	Boolean			close = false;

	nativeHTTPS = param1;
	if (!FskListMutexContains(gHTTPS, nativeHTTPS))
		return;
	the = nativeHTTPS->the;

	xsBeginHost(the);
	FskMutexAcquire(nativeHTTPS->mutex);

		statusCodeSize = sizeof(nativeHTTPS->statusCode);

		if (HttpQueryInfo(nativeHTTPS->httpRequestHandle, (HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER), &nativeHTTPS->statusCode, &statusCodeSize, NULL)) {
			xsSet(nativeHTTPS->obj, xsID("statusCode"), xsInteger(nativeHTTPS->statusCode));

			if ((HTTPSGetHeaderValue(nativeHTTPS, "Content-Type", &value) == kFskErrNone) && value)
				xsSet(xsGet(nativeHTTPS->obj, xsID("stream")), xsID("mime"), xsString(value));

			nativeHTTPS->gotHeaders = true;
		}
	FskMutexRelease(nativeHTTPS->mutex);

	if (nativeHTTPS->gotHeaders)
		xsCall0_noResult(nativeHTTPS->obj, xsID("onHeaders"));

	xsEndHost(the);

	FskMemPtrDispose(value);
}

void HTTPSRequestComplete(void *param1, void *param2, void *param3, void *param4)
{
	xsNativeHTTPS		nativeHTTPS;
	char				*value = NULL;
	DWORD				statusCodeSize;
	Boolean				done = false;
	xsMachine			*the;
	char				data[1024];
	UInt32				totalBufferSize = 0;
	DWORD				result = (DWORD)param2;

	nativeHTTPS = param1;
	if (!FskListMutexContains(gHTTPS, nativeHTTPS))
		return;
	the = nativeHTTPS->the;

	xsBeginHost(the);

	if (ERROR_SUCCESS != result) {
		FskErr status;

		if (12007 == result)	// ERROR_WINHTTP_NAME_NOT_RESOLVED 
			status = kFskErrNameLookupFailed;
		else
			status = kFskErrNetworkErr;

		done = true;

		xsSet(nativeHTTPS->obj, xsID("status"), xsInteger(status));

		goto done;
	}

	if (!nativeHTTPS->gotHeaders) {
		statusCodeSize = sizeof(nativeHTTPS->statusCode);

		if (HttpQueryInfo(nativeHTTPS->httpRequestHandle, (HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER), &nativeHTTPS->statusCode, &statusCodeSize, NULL)) {
			xsSet(nativeHTTPS->obj, xsID("statusCode"), xsInteger(nativeHTTPS->statusCode));

			if ((HTTPSGetHeaderValue(nativeHTTPS, "Content-Type", &value) == kFskErrNone) && value)
				xsSet(xsGet(nativeHTTPS->obj, xsID("stream")), xsID("mime"), xsString(value));

			nativeHTTPS->gotHeaders = true;

			xsCall0_noResult(nativeHTTPS->obj, xsID("onHeaders"));
		}
	}

	while (true) {
		INTERNET_BUFFERSA	buffers = {0};

		buffers.dwStructSize = sizeof(buffers);
		buffers.lpvBuffer = data;
		buffers.dwBufferLength = sizeof(data);
		if (!InternetReadFileExA(nativeHTTPS->httpRequestHandle, &buffers, (IRF_ASYNC | IRF_USE_CONTEXT | IRF_NO_WAIT), (DWORD_PTR)nativeHTTPS))
			break;

		if (buffers.dwBufferLength <= 0) {
			done = true;
			break;
		}
		if (totalBufferSize == 0)
			xsResult = xsNew0(xsGlobal, xsID("Chunk"));

		xsSet(xsResult, xsID("length"), xsInteger(totalBufferSize + buffers.dwBufferLength));
		FskMemMove((char *)xsGetHostData(xsResult) + totalBufferSize, data, buffers.dwBufferLength);
		totalBufferSize += buffers.dwBufferLength;
	}

	if (totalBufferSize > 0)
		xsCall1_noResult(nativeHTTPS->obj, xsID("onDataReady"), xsResult);

done:
	if (done && (kHTTPSStateClosing != nativeHTTPS->state)) {
		xsCall0_noResult(nativeHTTPS->stream, xsID("rewind"));
		xsCall0_noResult(nativeHTTPS->obj, xsID("onTransferComplete"));
	}

	xsEndHost(the);

	FskMemPtrDispose(value);
}

void HTTPSClosingConnection(void *param1, void *param2, void *param3, void *param4)
{
	xsNativeHTTPS	nativeHTTPS;
	xsMachine		*the;

	nativeHTTPS = param1;
	the = nativeHTTPS->the;

	xsBeginHost(the);
		xsSet(nativeHTTPS->obj, xsID("status"), xsInteger(-1));

		xsCall0_noResult(nativeHTTPS->obj, xsID("onTransferComplete"));
	xsEndHost(the);
}

void HTTPSClose(xsNativeHTTPS nativeHTTPS)
{
	UInt32 index;

#if TARGET_OS_WIN32

	FskMutexAcquire(nativeHTTPS->mutex);

	if (kHTTPSStateClosing != nativeHTTPS->state) {
		nativeHTTPS->state = kHTTPSStateClosing;

		if (nativeHTTPS->httpRequestHandle) {
			InternetCloseHandle(nativeHTTPS->httpRequestHandle);
			nativeHTTPS->httpRequestHandle = NULL;
		}

		if (nativeHTTPS->internetConnectHandle) {
			InternetCloseHandle(nativeHTTPS->internetConnectHandle);
			nativeHTTPS->internetConnectHandle = NULL;
		}

		InternetSetStatusCallback(nativeHTTPS->internetOpenHandle, NULL);
		InternetCloseHandle(nativeHTTPS->internetOpenHandle);
		nativeHTTPS->internetOpenHandle = NULL;
	}

	nativeHTTPS->count -= 1;
	if (nativeHTTPS->count > 0) {
		FskMutexRelease(nativeHTTPS->mutex);
		return;
	}

	FskMutexRelease(nativeHTTPS->mutex);

	FskListMutexRemove(gHTTPS, nativeHTTPS);

	FskMemPtrDispose(nativeHTTPS->postChunk);
	FskMemPtrDispose(nativeHTTPS->pathW);
	FskMemPtrDispose(nativeHTTPS->methodW);
	FskMemPtrDispose(nativeHTTPS->userAgentW);
	for (index = 0; index < nativeHTTPS->requestHeaderCount; index++) {
		FskMemPtrDispose(nativeHTTPS->requestHeaders[index * 2]);
		FskMemPtrDispose(nativeHTTPS->requestHeaders[index * 2 + 1]);
	}
	FskMemPtrDispose(nativeHTTPS->requestHeaders);
	FskMutexDispose(nativeHTTPS->mutex);
	FskMemPtrDispose(nativeHTTPS);
#endif
}

FskErr HTTPSAddHeader(xsNativeHTTPS nativeHTTPS, char *header, char *value)
{
	FskErr	err = kFskErrNone;
	UInt32	index;
	Boolean	replaced = false;

	if (FskStrCompareCaseInsensitive(header, "Content-Length") == 0)
		nativeHTTPS->hasContentLength = true;

	for (index = 0; index < nativeHTTPS->requestHeaderCount; index++) {
		if (FskStrCompare(nativeHTTPS->requestHeaders[index * 2], header) == 0) {
			FskMemPtrDispose(nativeHTTPS->requestHeaders[index * 2 + 1]);
			nativeHTTPS->requestHeaders[index * 2 + 1] = FskStrDoCopy(value);
			replaced = true;
			break;
		}
	}

	if (!replaced) {
		err = FskMemPtrRealloc(sizeof(char *) * 2 * (nativeHTTPS->requestHeaderCount + 1), &nativeHTTPS->requestHeaders);
		BAIL_IF_ERR(err);

		nativeHTTPS->requestHeaders[nativeHTTPS->requestHeaderCount * 2] = FskStrDoCopy(header);
		nativeHTTPS->requestHeaders[(nativeHTTPS->requestHeaderCount * 2) + 1] = FskStrDoCopy(value);
		nativeHTTPS->requestHeaderCount++;
	}

bail:
	return err;
}

FskErr HTTPSGetHeaderValue(xsNativeHTTPS nativeHTTPS, char *header, char **value)
{
	FskErr	err = kFskErrNone;
	UInt16	*headerW = NULL;
	DWORD	valueSize;
	char	*h = NULL;
	DWORD	i = 0, j;

	*value = NULL;

	while (true) {
		FskMemPtrDisposeAt(&headerW);

		err = FskTextUTF8ToUnicode16LE(header, FskStrLen(header), &headerW, NULL);
		BAIL_IF_ERR(err);

		valueSize = wcslen(headerW);

		j = i;
		if (!HttpQueryInfoW(nativeHTTPS->httpRequestHandle, HTTP_QUERY_CUSTOM, headerW, &valueSize, &j)) {
			DWORD winErr = GetLastError();

			if (winErr == 0) {
				goto bail;
			}
			if (winErr != ERROR_INSUFFICIENT_BUFFER) goto bail;

			err = FskMemPtrRealloc(valueSize << 1, &headerW);			// contrary to documentation, valueSize in this case is characters not bytes.
			BAIL_IF_ERR(err);

			j = i;
			if (!HttpQueryInfoW(nativeHTTPS->httpRequestHandle, HTTP_QUERY_CUSTOM, headerW, &valueSize, &j))
				goto bail;
		}
		i = j;

		err = FskTextUnicode16LEToUTF8(headerW, valueSize, &h, NULL);
		BAIL_IF_ERR(err);

		if (*value) {
			char *tmp = *value;
			*value = FskStrDoCat(tmp, ",");
			FskMemPtrDispose(tmp); tmp = *value;
			*value = FskStrDoCat(tmp, h);
			FskMemPtrDisposeAt(&h);
		}
		else {
			*value = h;
			h = NULL;
		}
	}

bail:
	FskMemPtrDispose(headerW);
	FskMemPtrDispose(h);

	if ((kFskErrNone == err) && (NULL == *value))
		err = kFskErrNotFound;

	return err;
}
#endif

void xs_Network_set(xsMachine *the)
{
	do_setMediaProperty(the, NULL, FskNetworkHasProperty, FskNetworkSetProperty);
}

void xs_Network_get(xsMachine *the)
{
	do_getMediaProperty(the, NULL, FskNetworkGetProperty);
}

typedef struct {
	xsMachine		*the;
	xsSlot			obj;
	FskNetInterfaceNotifier netInterfaceNotifier;
} xsNativeNetworkNotifierRecord, *xsNativeNetworkNotifier;

static void networkNotifier(int what, int message, void *refCon);
static int networkInterfaceNotifier(struct FskNetInterfaceRecord *iface, UInt32 status, void *params);

void xs_Network_Notifier(xsMachine *the)
{
	FskErr err;
	xsNativeNetworkNotifier nn;
	int what = xsToInteger(xsArg(0));

	err = FskMemPtrNew(sizeof(xsNativeNetworkNotifierRecord), &nn);
	FskECMAScriptThrowIf(the, err);

	xsSetHostData(xsThis, nn);
	nn->the = the;
	nn->obj = xsThis;

	if (what == kFskNetNotificationNetworkInterfaceChanged)
		nn->netInterfaceNotifier = FskNetInterfaceAddNotifier(networkInterfaceNotifier, nn, "network notifier");
	else {
		FskNetNotificationNew(what, networkNotifier, nn);
		nn->netInterfaceNotifier = NULL;
	}
}

void xs_Network_notifier_destructor(void *data)
{
	if (data) {
		xsNativeNetworkNotifier nn = data;
		if (nn->netInterfaceNotifier)
			FskNetInterfaceRemoveNotifier(nn->netInterfaceNotifier);
		else
			FskNetNotificationDispose(networkNotifier, data);
		FskMemPtrDispose(data);
	}
}

void xs_Network_notifier_close(xsMachine *the)
{
	xs_Network_notifier_destructor(xsGetHostData(xsThis));
	xsSetHostData(xsThis, NULL);
}

void networkNotifier(int what, int message, void *refCon)
{
	xsNativeNetworkNotifier nn = refCon;

	xsBeginHost(nn->the);
		xsCall0_noResult(nn->obj, xsID("onUpdate"));
	xsEndHost(nn->the);
}

int networkInterfaceNotifier(struct FskNetInterfaceRecord *iface, UInt32 status, void *params)
{
	xsNativeNetworkNotifier nn = params;
	char *ss;
	Boolean flag;

	switch (status) {
	case kFskNetInterfaceStatusNew:
		ss = "new";
		flag = iface->status;
		break;
	case kFskNetInterfaceStatusChanged:
		ss = "changed";
		flag = iface->status;
		break;
	case kFskNetInterfaceStatusRemoved:
		ss = "removed";
		flag = 0;
		break;
	default:
		return 0;
	}
	xsBeginHost(nn->the);
		xsCall2_noResult(nn->obj, xsID("onUpdate"), xsString(ss), xsBoolean(flag));
	xsEndHost(nn->the);
	return 0;
}

static char *getStringCopy(xsMachine *the, xsSlot *slot);

void xs_FileSystem_chooseFile(xsMachine *the)
{
	FskErr err;
	int argc = xsToInteger(xsArgc);
	char *prompt = (argc >= 2) ? getStringCopy(the, &xsArg(1)) : NULL;
	Boolean allowMultiple = (argc >= 3) ? xsToBoolean(xsArg(2)) : false;
	char *initialPath = (argc >= 4) ? getStringCopy(the, &xsArg(3)) : NULL;
	char *files;
	FskFileChooseEntry fileTypes = NULL;

	xsVars(3);

#if TARGET_OS_WIN32 || TARGET_OS_MAC
	if ((argc >= 1) && (xsTypeOf(xsArg(0)) != xsNullType)) {
		SInt32 length = xsToInteger(xsGet(xsArg(0), xsID("length"))), i;

		err = FskMemPtrNewClear(sizeof(FskFileChooseEntryRecord) * (length + 1), (FskMemPtr *)&fileTypes);
		BAIL_IF_ERR(err);

		for (i = 0; i < length; i += 2) {
			char *extension;
			UInt32 typeOf;

			xsVar(0) = xsGet(xsArg(0), (xsIndex)i);
			typeOf = xsTypeOf(xsVar(0));
			if (xsStringType == typeOf) {
				if (0 != FskStrCompare(xsToString(xsVar(0)), "*/*"))
					extension = xsToString(xsCall1(xsGet(xsGlobal, xsID("FileSystem")), xsID("getExtensionFromMIMEType"), xsVar(0)));
				else
					extension = "*";

				err = FskMemPtrNewClear(FskStrLen(extension) + 2, (FskMemPtr *)&fileTypes[i / 2].extension);
				BAIL_IF_ERR(err);

				FskStrCopy(fileTypes[i / 2].extension, extension);
			}
			else if ((xsReferenceType == typeOf) && (xsIsInstanceOf(xsArg(0), xsArrayPrototype))) {
				UInt32 extensionLength = 0, count, j;

				count = xsToInteger(xsGet(xsVar(0), xsID("length")));
				for (j=0; j < count; j++) {
					xsVar(1) = xsGet(xsVar(0), (xsIndex)j);
					if (0 != FskStrCompare(xsToString(xsVar(1)), "*/*"))
						extension = xsToString(xsCall1(xsGet(xsGlobal, xsID("FileSystem")), xsID("getExtensionFromMIMEType"), xsVar(1)));
					else
						extension = "*";

					err = FskMemPtrRealloc(extensionLength + 2 + FskStrLen(extension), (FskMemPtr *)&fileTypes[i / 2].extension);
					BAIL_IF_ERR(err);

					FskMemMove(fileTypes[i / 2].extension + extensionLength, extension, FskStrLen(extension) + 1);
					extensionLength += FskStrLen(extension) + 1;
					fileTypes[i / 2].extension[extensionLength] = 0;
				}
			}

			if ((i + 1) < length)
				fileTypes[i / 2].label = xsToStringCopy(xsGet(xsArg(0), (xsIndex)(i + 1)));
		}
	}

#endif

	err = FskFileChoose(fileTypes, prompt, allowMultiple, initialPath, &files);
	if ((kFskErrNone == err) && (NULL != files)) {
		char *walker;
		xsIndex i = 0;
		xsResult = xsNewInstanceOf(xsArrayPrototype);
		for (walker = files; 0 != *walker; walker += (FskStrLen(walker) + 1))
			xsSet(xsResult, i++, xsString(walker));
		FskMemPtrDispose(files);
	}

#if TARGET_OS_WIN32 || TARGET_OS_MAC
bail:
	if (NULL != fileTypes) {
		FskFileChooseEntry walker;

		for (walker = fileTypes; NULL != walker->extension; walker++) {
			FskMemPtrDispose(walker->extension);
			FskMemPtrDispose(walker->label);
		}
		FskMemPtrDispose(fileTypes);
	}
#endif

	FskMemPtrDispose(prompt);
	FskMemPtrDispose(initialPath);
}

void xs_FileSystem_chooseFileSave(xsMachine *the)
{
	FskErr err;
	int argc = xsToInteger(xsArgc);
	char *defaultName = (argc >= 1) ? getStringCopy(the, &xsArg(0)) : NULL;
	char *prompt = (argc >= 2) ? getStringCopy(the, &xsArg(1)) : NULL;
	char *initialDirectory = (argc >= 3) ? getStringCopy(the, &xsArg(2)) : NULL;
	char *file;

	err = FskFileChooseSave(defaultName, prompt, initialDirectory, &file);
	if ((kFskErrNone == err) && (NULL != file)) {
		xsResult = xsString(file);
		FskMemPtrDispose(file);
	}

	FskMemPtrDispose(defaultName);
	FskMemPtrDispose(prompt);
	FskMemPtrDispose(initialDirectory);
}

void xs_FileSystem_chooseDirectory(xsMachine *the)
{
	FskErr err;
	int argc = xsToInteger(xsArgc);
	char *prompt = (argc >= 1) ? getStringCopy(the, &xsArg(0)) : NULL;
	char *initialPath = (argc >= 2) ? getStringCopy(the, &xsArg(1)) : NULL;
	char *path;

	err = FskDirectoryChoose(prompt, initialPath, &path);
	if ((kFskErrNone == err) && (NULL != path)) {
		xsResult = xsString(path);
		FskMemPtrDispose(path);
	}

	FskMemPtrDispose(prompt);
	FskMemPtrDispose(initialPath);
}

char *getStringCopy(xsMachine *the, xsSlot *slot)
{
	int slotType = xsTypeOf(*slot);
	if ((xsUndefinedType == slotType) || (xsNullType == slotType))
		return NULL;
	return xsToStringCopy(*slot);
}

/*
	in-memory http cache
*/

typedef struct {
	UInt32		flags;

	SInt32		id;
	char		*uri;
	double		last;
	SInt32		count;
	char		*ETag;
	char		*lastModified;
	double		expires;
	char		*mime;
	SInt32		size;
	char		*compress;
} xsHTTPCacheEntryRecord, *xsHTTPCacheEntry;

void xsHTTPCacheEntryConstructor(xsMachine *the)
{
	xsHTTPCacheEntry entry;

	FskMemPtrNewClear(sizeof(xsHTTPCacheEntryRecord), &entry);
	xsSetHostData(xsThis, entry);
}

void xsHTTPCacheEntryDestructor(void *data)
{
	xsHTTPCacheEntry entry = data;
	
	if (!data) return;

	FskMemPtrDispose(entry->uri);
	FskMemPtrDispose(entry->ETag);
	FskMemPtrDispose(entry->lastModified);
	FskMemPtrDispose(entry->mime);
	FskMemPtrDispose(entry->compress);
	FskMemPtrDispose(data);
}

void xsHTTPCacheEntrySet_id(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	entry->id = xsToInteger(xsArg(0));
}

void xsHTTPCacheEntryGet_id(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	xsResult = xsInteger(entry->id);
}

void xsHTTPCacheEntrySet_last(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	entry->last = xsToNumber(xsArg(0));
}

void xsHTTPCacheEntryGet_last(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	xsResult = xsNumber(entry->last);
}

void xsHTTPCacheEntrySet_expires(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	entry->expires = xsToNumber(xsArg(0));
}

void xsHTTPCacheEntryGet_expires(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	xsResult = xsNumber(entry->expires);
}

void xsHTTPCacheEntrySet_count(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	entry->count = xsToInteger(xsArg(0));
}

void xsHTTPCacheEntryGet_count(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	xsResult = xsInteger(entry->count);
}

void xsHTTPCacheEntrySet_size(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	entry->size = xsToInteger(xsArg(0));
}

void xsHTTPCacheEntryGet_size(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	xsResult = xsInteger(entry->size);
}


void xsHTTPCacheEntrySet_uri(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	FskMemPtrDispose(entry->uri);
	entry->uri = xsToStringCopy(xsArg(0));
}

void xsHTTPCacheEntryGet_uri(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	if (entry->uri) xsResult = xsString(entry->uri);
}

void xsHTTPCacheEntrySet_ETag(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	FskMemPtrDispose(entry->ETag);
	entry->ETag = xsToStringCopy(xsArg(0));
}

void xsHTTPCacheEntryGet_ETag(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	if (entry->ETag) xsResult = xsString(entry->ETag);
}

void xsHTTPCacheEntrySet_lastModified(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	FskMemPtrDispose(entry->lastModified);
	entry->lastModified = xsToStringCopy(xsArg(0));
}

void xsHTTPCacheEntryGet_lastModified(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	if (entry->lastModified) xsResult = xsString(entry->lastModified);
}

void xsHTTPCacheEntrySet_mime(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	FskMemPtrDispose(entry->mime);
	entry->mime = xsToStringCopy(xsArg(0));
}

void xsHTTPCacheEntryGet_mime(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	if (entry->mime) xsResult = xsString(entry->mime);
}

void xsHTTPCacheEntrySet_compress(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	FskMemPtrDispose(entry->compress);
	entry->compress = xsToStringCopy(xsArg(0));
}

void xsHTTPCacheEntryGet_compress(xsMachine *the)
{
	xsHTTPCacheEntry entry = xsGetHostData(xsThis);
	if (entry->compress) xsResult = xsString(entry->compress);
}

void xsHTTPCacheFind(xsMachine *the)
{
	SInt32 i, length;
	char *uri = xsToString(xsArg(0));

	xsVars(4);
	
	xsVar(0) = xsGet(xsThis, xsID("entries"));
	length = xsToInteger(xsGet(xsVar(0), xsID("length")));
	for (i = 0; i < length; i++) {
		xsHTTPCacheEntry entry;
		xsVar(1) = xsGet(xsVar(0), i);
		entry = xsGetHostData(xsVar(1));
		if (FskStrCompare(entry->uri, uri))
			continue;

		xsResult = xsVar(1);
		break;
	}
}

#endif
