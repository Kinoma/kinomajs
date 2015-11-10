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
#define __FSKECMASCRIPT_PRIV__
#define __FSKUTILITIES_PRIV__
#define __FSKTEXT_PRIV__
#define __FSKTHREAD_PRIV__
#include "FskECMAScript.h"
#include "FskEndian.h"
#include "FskEnvironment.h"
#include "FskExtensions.h"
#include "FskFiles.h"
#include "FskMain.h"
#include "kprShell.h"
#if TARGET_OS_IPHONE
#include "FskCocoaSupportPhone.h"
#elif TARGET_OS_KPL
	#include "KplECMAScript.h"
#endif

void FskECMAScriptGetDebug(char **host, char **breakpoints, Boolean *keepSource)
{
    if (host)
        *host = FskEnvironmentGet("debugger");
    if (breakpoints)
        *breakpoints = NULL;
    if (keepSource)
        *keepSource = 0;
}

void FskECMAScriptHibernate(void)
{
}

void FskECMAScriptInitializeThread()
{
	FskRectangleRecord bounds;
	char* shellPath;
	KprShell shell;

	bounds.x = 0;
	bounds.y = 0;
	bounds.width = KprEnvironmentGetUInt32("windowWidth", 800);
	bounds.height = KprEnvironmentGetUInt32("windowHeight", 600);
 	shellPath = FskEnvironmentGet("shellPath");
#if !TARGET_OS_ANDROID
    KprShellNew(&shell, NULL, &bounds, shellPath, NULL, NULL, NULL, 0);
#else
	if (kFskErrNone == KprShellNew(&shell, NULL, &bounds, shellPath, NULL, NULL, NULL, 0)) {
		while (!gQuitting) // (!gQuitting && !vmi->vm->quitting)
			FskThreadRunloopCycle(-1);
	}
#endif
}

FskErr FskECMAScriptInitialize(const char *configPath)
{
	FskErr err = kFskErrNone;
 	FskExtensionsEmbedLoad(NULL);
#if TARGET_OS_ANDROID
    {
	FskThread thread = NULL;
	err = FskThreadCreate(&thread, FskECMAScriptInitializeThread, kFskThreadFlagsDefault, NULL, "KPR");
    }
#else
	FskECMAScriptInitializeThread();
#endif
    return err;
}

FskErr FskECMAScriptInitializationComplete(void)
{
	FskErr err = kFskErrNone;
#if TARGET_OS_IPHONE
	FskCocoaApplicationRunLoopAddCallback();
#elif TARGET_OS_ANDROID
	{	int useGL = 0;	// let Android override "useGL"
		if (gAndroidCallbacks->getModelInfoCB) {
			gAndroidCallbacks->getModelInfoCB(NULL, NULL, NULL, NULL, &useGL);
			FskDebugStr("useGL: %d", useGL);
			FskEnvironmentSet("useGL", useGL ? "1" : "0");
		}
	}
#endif
    return err;
}

void FskECMAScriptTerminate(void)
{
 	FskExtensionsEmbedUnload(NULL);
}

FskErr FskECMAScriptLoadLibrary(const char *name)
{
	FskErr err = kFskErrBadData;
	FskLibraryLoadProc libLoad;
	char *extension = NULL, *symbolName = NULL, *dot, *slash, *fullPath = NULL, *temp = NULL;
	FskLibrary library = NULL;
	char* root = NULL;
	FskFileInfo itemInfo;

	FskDebugStr("FskECMAScriptLoadLibrary: %s", name);
	dot = FskStrRChr(name, '.');
	slash = FskStrRChr(name, '/');
	if ((NULL == dot) || (slash && (slash > dot))) {
#if TARGET_OS_WIN32
		extension = ".dll";
#elif TARGET_OS_MAC || TARGET_OS_LINUX
		extension = ".so";
#elif TARGET_OS_KPL
		extension = (char*)KplECMAScriptGetExtension();
#endif
	}

	if (extension)
		fullPath = FskStrDoCat(name, extension);
	else
		fullPath = FskStrDoCopy(name);

	if (kFskErrNone != FskFileGetFileInfo(fullPath, &itemInfo)) {
#if TARGET_OS_ANDROID
		char *libDir;
		libDir = FskStrDoCat(gAndroidCallbacks->getStaticDataDirCB(), "../lib/lib");
		temp = FskStrDoCat(libDir, fullPath);
		FskMemPtrDispose(libDir);
#else
		root = FskGetApplicationPath();
		temp = FskStrDoCat(root, fullPath);
		FskMemPtrDispose(root);
#endif
		if (kFskErrNone != FskFileGetFileInfo(temp, &itemInfo)) {
			FskDebugStr(" - no file: %s", temp);
			goto bail;
		}
		FskMemPtrDispose(fullPath);
		fullPath = temp;
		temp = NULL;
	}

	err = FskLibraryLoad(&library, fullPath);
	FskDebugStr(" - try: %s -> %d", fullPath, err);
	if (err) goto bail;

	symbolName = FskStrDoCat(name, "_fskLoad");

	if ((kFskErrNone == FskLibraryGetSymbolAddress(library, symbolName, &libLoad)) ||
		(kFskErrNone == FskLibraryGetSymbolAddress(library, "fskLoad", &libLoad))) {

		err = (libLoad)(library);
		FskDebugStr(" - symbolName: %x -> %d", libLoad, err);
		if (err)
			goto bail;

		err = kFskErrNone;
	}

bail:
	if (err) {
		FskLibraryUnload(library);
	}

	FskMemPtrDispose(symbolName);
	FskMemPtrDispose(fullPath);

	return err;
}

FskErr FskExtensionLoad(const char *name)
{
	FskErr err = kFskErrNone;
	err = FskECMAScriptLoadLibrary(name);
	return err;
}

FskErr FskExtensionUnload(const char *name)
{
	FskErr err = kFskErrNone;
	return err;
}

#if FSK_TEXT_FREETYPE
FskErr FskTextFreeTypeInstallFonts(char* fontsPath, char* defaultFont)
{
	FskErr err = kFskErrNone;
	FskTextEngine fte;
	char *fromDirectory = NULL;
	char* fromPath = NULL;
	FskDirectoryIterator iterator = NULL;
	char *name = NULL;
	UInt32 type;
#if TARGET_OS_ANDROID
	char *toDirectory = NULL;
	char* toPath = NULL;
	FskFileInfo itemInfo;
	FskFileMapping map = NULL;
	unsigned char *data;
	FskInt64 dataSize;
	FskFile file = NULL;
#endif	
	err = FskTextEngineNew(&fte, NULL);
	if (err) goto bail;
	fromDirectory = FskEnvironmentDoApply(FskStrDoCopy(fontsPath));
	if (!fromDirectory) { err = kFskErrMemFull; goto bail; }
#if TARGET_OS_ANDROID
	if (!FskStrCompareWithLength(fromDirectory, "/data/app/", 10)) {
		err = FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeApplicationPreference, false, NULL, &toDirectory);
		if (err) goto bail;
	}
#endif
	err = FskDirectoryIteratorNew(fromDirectory, &iterator, 0);
	if (err) goto bail;
	while (kFskErrNone == FskDirectoryIteratorGetNext(iterator, &name, &type)) {
		if (type == kFskDirectoryItemIsFile) {
			fromPath = FskStrDoCat(fromDirectory, name);
			if (!fromPath) { err = kFskErrMemFull; goto bail; }
			FskDebugStr("from %s", fromPath);
#if TARGET_OS_ANDROID
			if (toDirectory) {
				toPath = FskStrDoCat(toDirectory, name);
				FskDebugStr("to %s", toPath);
				if (kFskErrNone != FskFileGetFileInfo(toPath, &itemInfo)) {
					err = FskFileMap(fromPath, &data, &dataSize, 0, &map);
					if (err) goto bail;
					err = FskFileCreate(toPath);
					if (err) goto bail;
					err = FskFileOpen(toPath, kFskFilePermissionReadWrite, &file);
					if (err) goto bail;
					err = FskFileWrite(file, dataSize, data, NULL);
					if (err) goto bail;
					FskFileClose(file);
					file = NULL;
					FskFileDisposeMap(map);
					map = NULL;
				}
				FskDebugStr("add %s", toPath);
				FskTextAddFontFile(fte, toPath);
				FskMemPtrDisposeAt(&toPath);
			}
			else
#endif
				FskTextAddFontFile(fte, fromPath);
			FskMemPtrDisposeAt(&fromPath);
		}
		FskMemPtrDisposeAt(&name);
	}
#if TARGET_OS_ANDROID
	if (gAndroidCallbacks->getModelInfoCB) {
		char* osVersion;
		gAndroidCallbacks->getModelInfoCB(NULL, &osVersion, NULL, NULL, NULL);
		if ((FskStrStr(osVersion, "android.6") == osVersion) // for KPR applications
			|| (FskStrStr(osVersion, "6.") == osVersion)) {  // for tests
			BAIL_IF_ERR(err = FskFTAddMapping("/system/etc/fonts.xml"));
		}
		if ((FskStrStr(osVersion, "android.5") == osVersion) // for KPR applications
			|| (FskStrStr(osVersion, "5.") == osVersion)) {  // for tests
			BAIL_IF_ERR(err = FskFTAddMapping("/system/etc/fonts.xml"));
		}
		if ((FskStrStr(osVersion, "android.4") == osVersion) // for KPR applications
			|| (FskStrStr(osVersion, "4.") == osVersion)) {  // for tests
			BAIL_IF_ERR(err = FskFTAddMapping("/system/etc/system_fonts.xml"));
			err = FskFTAddMapping("/vendor/etc/fallback_fonts.xml");
			if (err != kFskErrFileNotFound) BAIL(err);
			BAIL_IF_ERR(err = FskFTAddMapping("/system/etc/fallback_fonts.xml"));
		}
		else {
			defaultFont = "Droid Sans";
			BAIL_IF_ERR(err = FskFTAddMapping(NULL));
		}
	}
#endif
	if (defaultFont)
		FskTextDefaultFontSet(fte, defaultFont);
bail:
#if TARGET_OS_ANDROID
	if (file)
		FskFileClose(file);
	if (map)
		FskFileDisposeMap(map);
	FskMemPtrDispose(toPath);
	FskMemPtrDispose(toDirectory);
#endif
	FskMemPtrDispose(name);
	FskDirectoryIteratorDispose(iterator);
	FskMemPtrDispose(fromPath);
	FskMemPtrDispose(fromDirectory);
	FskTextEngineDispose(fte);
	return err;
}
#elif TARGET_OS_WIN32
FskErr FskTextFreeTypeInstallFonts(char* fontsPath, char* defaultFont)
{
	FskErr err = kFskErrNone;
	FskTextEngine fte;
	char *fromDirectory = NULL;
	char *fromPath = NULL;
	FskDirectoryIterator iterator = NULL;
	char *name = NULL;
	UInt32 type;

	err = FskTextEngineNew(&fte, NULL);
	if (err) goto bail;
	fromDirectory = FskEnvironmentDoApply(FskStrDoCopy(fontsPath));
	if (!fromDirectory) { err = kFskErrMemFull; goto bail; }
	
	err = FskDirectoryIteratorNew(fromDirectory, &iterator, 0);
	if (err) goto bail;

	while (kFskErrNone == FskDirectoryIteratorGetNext(iterator, &name, &type)) {
		if (type == kFskDirectoryItemIsFile) {
			fromPath = FskStrDoCat(fromDirectory, name);
			if (!fromPath) { err = kFskErrMemFull; goto bail; }
			FskTextAddFontFile(fte, fromPath);
			FskMemPtrDisposeAt(&fromPath);
		}
		FskMemPtrDisposeAt(&name);
	}

bail:
	FskTextEngineDispose(fte);
	FskMemPtrDispose(fromDirectory);
	FskDirectoryIteratorDispose(iterator);

	return err;
}
#endif

extern FskErr FSK_fskLoad(FskLibrary library);
extern FskErr FSK_fskUnload(FskLibrary library);
extern FskErr FskCore_fskLoad(FskLibrary library);
extern FskErr FskCore_fskUnload(FskLibrary library);
extern FskErr FskSSLAll_fskLoad(FskLibrary library);
extern FskErr FskSSLAll_fskUnload(FskLibrary library);

FskErr FskPlatform_fskLoad(FskLibrary library)
{
	FskErr err = kFskErrNone;
    return err;
}

FskErr FskPlatform_fskUnload(FskLibrary library)
{
	FskErr err = kFskErrNone;
    return err;
}

FskErr FskCore_fskLoad(FskLibrary library)
{
	FskErr err = kFskErrNone;
    return err;
}

FskErr FskCore_fskUnload(FskLibrary library)
{
	FskErr err = kFskErrNone;
    return err;
}

FskErr FskSSLAll_fskLoad(FskLibrary library)
{
	FskErr err = kFskErrNone;
    return err;
}

FskErr FskSSLAll_fskUnload(FskLibrary library)
{
	FskErr err = kFskErrNone;
    return err;
}

static xsGrammar FskManifestGrammar = {
	NULL,
	(xsStringValue)0,
	0,
	(xsStringValue)0,
	0,
	"FskManifest"
};

void FskExtensionsEmbedGrammar(char *vmName, char **xsbName, xsGrammar **grammar)
{
	*xsbName = FskEnvironmentGet("grammarName");
	*grammar = &FskManifestGrammar;
}

void xsMemPtrToChunk(xsMachine *the, xsSlot *ref, FskMemPtr data, UInt32 dataSize, Boolean alreadyAllocated)
{
	xsDestructor destructor;
	if (!alreadyAllocated)
		*ref = xsNewInstanceOf(xsChunkPrototype);
	xsSetHostData(*ref, data);
	destructor =  xsGetHostDestructor(*ref);
	xsSetHostDestructor(*ref, NULL);
	xsSet(*ref, xsID_length, xsInteger(dataSize));
	xsSetHostDestructor(*ref, destructor);
}
