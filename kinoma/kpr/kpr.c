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
#define __FSKEVENT_PRIV__
#define __FSKPORT_PRIV__
#include "Fsk.h"
#include "FskECMAScript.h"
#include "FskEnvironment.h"
#include "FskEvent.h"
#include "FskExtensions.h"
#include "FskPixelOps.h"

#include "kpr.h"
#include "kprApplication.h"
#include "kprBehavior.h"
#include "kprImage.h"
#include "kprContent.h"
#include "kprHTTPClient.h"
#include "kprLayer.h"
#include "kprMedia.h"
#include "kprMessage.h"
#include "kprScroller.h"
#include "kprSkin.h"
#include "kprStorage.h"
#include "kprTable.h"
#include "kprURL.h"
#include "kprShell.h"
#include "kprUtilities.h"

#ifndef KPR_NO_GRAMMAR
#if FSK_EXTENSION_EMBED
extern void FskExtensionsEmbedGrammar(char *vmName, char **xsbName, xsGrammar **grammar);
#else
FskExport(xsGrammar *) fskGrammar = &kprGrammar;
#endif
extern FskErr loadGrammar(const char *xsbPath, xsGrammar *grammar);
#endif /* KPR_NO_GRAMMAR */

extern KprServiceRecord gFILEService;
extern KprServiceRecord gHTTPService;
extern KprServiceRecord gHTTPSService;
extern KprServiceRecord gXKPRService;
#if TARGET_OS_IPHONE
extern KprServiceRecord gALService;
#endif

// extension load/unload
FskExport(FskErr) kpr_fskLoad(FskLibrary library UNUSED);
FskErr kpr_fskLoad(FskLibrary library UNUSED)
{
	KprServiceRegister(&gXKPRService);
	KprServiceRegister(&gFILEService);
#if TARGET_OS_IPHONE
	KprServiceRegister(&gALService);
#endif
	KprServiceRegister(&gHTTPService);
	KprServiceRegister(&gHTTPSService);
	return kFskErrNone;
}

FskExport(FskErr) kpr_fskUnload(FskLibrary library UNUSED);
FskErr kpr_fskUnload(FskLibrary library UNUSED)
{
	return kFskErrNone;
}

typedef struct {
	unsigned char *buffer;
	FskInt64 offset;
	FskInt64 size;
} KprFileMapStreamRecord, *KprFileMapStream;
static int KprFileMapStreamGetter(void* it);

static char** gModulesBases = NULL;
static UInt32 gModulesBasesCount = 0;
KprShell gShell = NULL;

FskErr KprModulesBasesSetup(char* url, char* path)
{
	FskErr err = kFskErrNone;
	char* paths = FskEnvironmentGet("modulePath");
	UInt32 i = 1;
	char* colon;
	if (path)
		i++;
	if (paths) {
		colon = FskStrChr(paths, ';');
		while (colon) {
			i++;
			colon = FskStrChr(colon + 1, ';');
		}
		i++;
	}
	bailIfError(FskMemPtrNewClear(i * sizeof(char*), &gModulesBases));
	i = 1;
	if (path) {
		bailIfError(KprURLMerge(url, path, &gModulesBases[i]));
		i++;
	}
	if (paths) {
		path = paths;
		colon = FskStrChr(path, ';');
		while (colon) {
			*colon = 0;
			bailIfError(KprURLMerge(url, path, &gModulesBases[i]));
			*colon = ';';
			i++;
			path = colon + 1;
			colon = FskStrChr(path, ';');
		}
		bailIfError(KprURLMerge(url, path, &gModulesBases[i]));
		i++;
	}
	gModulesBasesCount = i;
bail:
	return err;
}

void KprModulesBasesCleanup(void)
{
	if (gModulesBases) {
		UInt32 i = 1;
		while (i < gModulesBasesCount) {
			FskMemPtrDispose(gModulesBases[i]);
			i++;
		}
		FskMemPtrDisposeAt(&gModulesBases);
		gModulesBasesCount = 0;
	}
}


#ifndef KPR_NO_GRAMMAR

extern xsIndex fxFindModule(xsMachine* the, xsIndex moduleID, xsSlot* slot);
static xsIndex fxFindModuleKPR(xsMachine* the, xsIndex moduleID, xsSlot* slot);

extern xsBooleanValue fxFindArchive(xsMachine* the, xsStringValue path);
static xsBooleanValue fxFindFile(xsMachine* the, xsStringValue path);
static xsBooleanValue fxFindURI(xsMachine* the, xsStringValue base, xsStringValue name, xsStringValue dot, xsIndex* id);

static xsStringValue gxExtensions[] = { 
	".jsb", 
	".xsb", 
	".js", 
	".xml", 
	NULL,
};

xsBooleanValue fxFindFile(xsMachine* the, xsStringValue path)
{
	FskFileInfo fileInfo;
	if (kFskErrNone == FskFileGetFileInfo(path, &fileInfo))
		return 1;
	return 0;
}

xsIndex fxFindModule(xsMachine* the, xsIndex moduleID, xsSlot* slot)
{
	return fxFindModuleKPR(the, moduleID, slot);
}

xsIndex fxFindModuleKPR(xsMachine* the, xsIndex moduleID, xsSlot* slot)
{
	char name[1024];
	xsBooleanValue absolute, relative;
	xsStringValue dot, slash;
	xsIndex id;
	
	xsToStringBuffer(*slot, name, sizeof(name));
	if ((!FskStrCompareWithLength(name, "./", 2)) || (!FskStrCompareWithLength(name, "../", 3))) {
		absolute = 0;
		relative = (moduleID == XS_NO_ID) ? 0 : 1;
	}
	else if ((!FskStrCompareWithLength(name, "/", 1))) {
		FskMemMove(name, name + 1, FskStrLen(name));
		absolute = 1;
		relative = 0;
	}
	else {
		absolute = 1;
		relative = (moduleID == XS_NO_ID) ? 0 : 1;
	}
	slash = FskStrRChr(name, '/');
	if (!slash)
		slash = name;
	dot = FskStrRChr(slash, '.');
	if (!dot)
		dot = name + FskStrLen(name);
	if (relative) {
		if (fxFindURI(the, xsName(moduleID), name, dot, &id))
			return id;
	}
	if (absolute) {
		xsStringValue* bases = gModulesBases;
		UInt32 c = gModulesBasesCount;
		UInt32 i = 1;
		while (i < c) {
			if (fxFindURI(the, bases[i], name, dot, &id))
				return id;
			i++;
		}
	}
	return XS_NO_ID;
}

// Node.js module resolution
// https://nodejs.org/dist/v4.2.1/docs/api/modules.html#modules_all_together

xsBooleanValue fxFindURI(xsMachine* the, xsStringValue base, xsStringValue name, xsStringValue dot, xsIndex* id)
{
	xsStringValue url = NULL;
	xsStringValue path = NULL;
	xsBooleanValue result = 0;
	*id = XS_NO_ID;
	if (*dot) {
		xsThrowIfFskErr(KprURLMerge(base, name, &url));
		xsThrowIfFskErr(KprURLToPath(url, &path));
		if (fxFindArchive(the, path) || fxFindFile(the, path)) {
			*id = xsID(url);
			result = 1;
		}
		FskMemPtrDisposeAt(&path);
		FskMemPtrDisposeAt(&url);
	}
	else {
		xsStringValue* extension;
		for (extension = gxExtensions; *extension; extension++) {
 			FskStrCopy(dot, *extension);
			xsThrowIfFskErr(KprURLMerge(base, name, &url));
			xsThrowIfFskErr(KprURLToPath(url, &path));
			if (fxFindArchive(the, path) || fxFindFile(the, path)) {
				*id = xsID(url);
				result = 1;
			}
			FskMemPtrDisposeAt(&path);
			FskMemPtrDisposeAt(&url);
			if (result)
				break;
		}
		*dot = 0;
	}
	return result;
}

KprImageCache gPictureImageCache = NULL;
KprImageCache gThumbnailImageCache = NULL;

#ifdef mxDebug
void* KprGetHostData(xsMachine* the, xsSlot* slot, xsIndex index, char* param, char* id)
{
	if (!xsIsInstanceOf(*slot, xsGet(xsGet(xsGlobal, xsID_KPR), index))) {
		xsThrowDiagnosticIfFskErr(kFskErrParameterError, "# %s is no KPR.%s!", param, id);
	}
	return xsGetHostData(*slot);
}
#endif

#ifdef mxDebug
void* KprGetHostData2(xsMachine* the, xsSlot* slot, xsIndex index, xsIndex index2, char* param, char* id, char* id2)
{
	if (xsIsInstanceOf(*slot, xsGet(xsGet(xsGlobal, xsID_KPR), index)))
		return xsGetHostData(*slot);
	if (xsIsInstanceOf(*slot, xsGet(xsGet(xsGlobal, xsID_KPR), index2)))
		return xsGetHostData(*slot);
	xsThrowDiagnosticIfFskErr(kFskErrParameterError, "# %s is neither KPR.%s nor KPR.%s!", param, id, id2);
	return NULL;
}
#endif

int KprFileMapStreamGetter(void* it)
{
	KprFileMapStream self = it;
	int result = EOF;
	if (self->offset < self->size) {
		result = *(self->buffer + self->offset);
		self->offset++;
	}
	return result;
}

/* UTILITIES */

static void KPR_array_shuffle(xsMachine* the)
{
	int n = xsToInteger(xsGet(xsThis, xsID("length")));
	xsVars(2);
	if (n > 1) {
		size_t i;
		for (i = 0; (int)i < n - 1; i++) {
		  size_t j = i + FskRandom() / (RAND_MAX / (n - i) + 1);
		  xsVar(0) = xsGetAt(xsThis, xsInteger(i));
		  xsVar(1) = xsGetAt(xsThis, xsInteger(j));
		  xsSetAt(xsThis, xsInteger(i), xsVar(1));
		  xsSetAt(xsThis, xsInteger(j), xsVar(0));
		}
	}
}

static void KPR_Grammar_load(xsMachine* the)
{
	char* url = NULL;
	char* path = NULL;
	FskFileMapping map = NULL;
	xsTry {
		xsStringValue reference = xsToString(xsArg(0));
		xsStringValue base = xsToString(xsModuleURL);
		xsThrowIfFskErr(KprURLMerge(base, reference, &url));
		if (!FskStrCompareWithLength(url, "file://", 7)) {
			KprFileMapStreamRecord stream;
			xsThrowIfFskErr(KprURLToPath(url, &path));
			xsThrowIfFskErr(FskFileMap(path, &stream.buffer, &stream.size, 0, &map));
			stream.offset = 0;
			xsResult = xsParse0(&stream, KprFileMapStreamGetter, path, 1, xsSourceFlag | xsSandboxFlag | xsDebugFlag);
			FskFileDisposeMap(map);
			map = NULL;
			FskMemPtrDisposeAt(&path);
		}
		else
			xsThrowIfFskErr(kFskErrUnimplemented);
		FskMemPtrDisposeAt(&url);
	}
	xsCatch {
		FskFileDisposeMap(map);
		FskMemPtrDispose(path);
		FskMemPtrDispose(url);
		xsThrow(xsException);
	}
}

void KPR_instrument(xsMachine *the)
{
#if SUPPORT_INSTRUMENTATION
	xsIntegerValue c = xsToInteger(xsArgc);
	xsStringValue typeName = xsToString(xsArg(0));
	UInt32 level = kFskInstrumentationLevelUpToNormal;
	if ((c > 1) && xsTest(xsArg(1))) {
		xsStringValue string = xsToString(xsArg(1));
		if (!FskStrCompare(string, "minimal"))
			level = kFskInstrumentationLevelUpToMinimal;
		else if (!FskStrCompare(string, "normal"))
			level = kFskInstrumentationLevelUpToNormal;
		else if (!FskStrCompare(string, "verbose"))
			level = kFskInstrumentationLevelUpToVerbose;
		else if (!FskStrCompare(string, "debug"))
			level = kFskInstrumentationLevelUpToDebug;
	}
	FskInstrumentationSimpleClientAddType(typeName, level);
#endif
}

void KPR_blendColors(xsMachine *the)
{
	FskColorRGBARecord foreColor, backColor, color;
	UInt32 dst, src;
	if (KprParseColor(the, xsToString(xsArg(2)), &foreColor)) {
		if (KprParseColor(the, xsToString(xsArg(1)), &backColor)) {
			xsNumberValue alpha = xsToNumber(xsArg(0));
			FskConvertColorRGBAToBitmapPixel(&backColor, kFskBitmapFormat32ARGB, &dst);
			FskConvertColorRGBAToBitmapPixel(&foreColor, kFskBitmapFormat32ARGB, &src);
			FskAlphaBlend32RGBA(&dst, src, (UInt8)(alpha * 255.0));
			FskConvertBitmapPixelToColorRGBA(&dst, kFskBitmapFormat32ARGB, &color);
			KprSerializeColor(the, &color, &xsResult);
		}
	}
}

void KPR_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("Array"));
	xsResult = xsGet(xsResult, xsID("prototype"));
	xsNewHostProperty(xsResult, xsID("shuffle"), xsNewHostFunction(KPR_array_shuffle, 0), xsDontEnum, xsDontScript);
	xsResult = xsGet(xsGlobal, xsID("Grammar"));
	xsNewHostProperty(xsResult, xsID("load"), xsNewHostFunction(KPR_Grammar_load, 1), xsDefault, xsDontScript);
	xsNewHostProperty(xsGlobal, xsID("instrument"), xsNewHostFunction(KPR_instrument, 2), xsDefault, xsDontScript);
	xsNewHostProperty(xsGlobal, xsID("blendColors"), xsNewHostFunction(KPR_blendColors, 3), xsDefault, xsDontScript);
}

void KPR_get_controlKey(xsMachine *the)
{
	xsResult = (gShell->modifiers & kFskEventModifierControl) ? xsTrue : xsFalse;
}

void KPR_get_optionKey(xsMachine *the)
{
	xsResult = (gShell->modifiers & kFskEventModifierAlt) ? xsTrue : xsFalse;
}

void KPR_get_shiftKey(xsMachine *the)
{
	xsResult = (gShell->modifiers & kFskEventModifierShift) ? xsTrue : xsFalse;
}

void KPR_get_screenScale(xsMachine *the)
{
	xsResult = xsNumber(FskPortDoubleScale(gShell->port, 1));
}

void KPR_decodeBase64(xsMachine *the)
{
	xsStringValue string = xsToString(xsArg(0)), buffer;
	xsIntegerValue stringSize = FskStrLen(string);
	UInt32 bufferSize;
	xsThrowIfFskErr(FskStrB64Decode(string, stringSize, &buffer, &bufferSize));
	xsResult = xsStringBuffer(buffer, bufferSize);
	FskMemPtrDispose(buffer);
}

void KPR_encodeBase64(xsMachine *the)
{
	xsStringValue string = xsToString(xsArg(0));
	xsIntegerValue size = FskStrLen(string);
	xsResult = xsNew1(xsGlobal, xsID_Chunk, xsInteger(size));
	string = xsToString(xsArg(0));
	FskMemMove(xsGetHostData(xsResult), string, size);
	xsResult = xsCall0(xsResult, xsID_toString);
}

static char gxRfc3986[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,	/* 2x   !"#$%&'()*+,-./  */
	 1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0 	/* 7X  pqrstuvwxyz{|}~   */
};

void KPR_encodeURIComponentRFC3986(xsMachine *the)
{
	fxEncodeURI(the, gxRfc3986);
}

void KPR_getEnvironmentVariable(xsMachine *the)
{
	char *name = xsToString(xsArg(0));
	const char *value = FskEnvironmentGet(name);
	if (value)
		xsResult = xsString((xsStringValue)value);
}

void KPR_setEnvironmentVariable(xsMachine *the)
{
	char *name = xsToString(xsArg(0));
	char *value = xsToString(xsArg(1));
	FskEnvironmentSet(name, value);
	if (!FskStrCompare(name, "modulePath")) {
		char* path = FskStrDoCopy(gModulesBases[1]);
		KprModulesBasesCleanup();
		KprModulesBasesSetup(gShell->url, path);
		FskMemPtrDispose(path);
	}
#ifdef mxDebug
	else if (!FskStrCompare(name, "debugger")) {
		(void)xsCall1(xsGet(xsGet(xsGlobal, xsID("xs")), xsID("debug")), xsID("setAddress"), xsArg(1));
	}
#endif
}

void kprGrammar_countLines(xsMachine* the)
{
	char* aString;
	char c;
	int i = 0;
	aString = xsToString(xsArg(0));
	while ((c = *aString)) {
		if (c <= 0x20) {
			if (c == 0x0A)
				i++;
		}
		else
			break;
		aString++;
	}
	xsResult = xsInteger(i);
}

#endif /* KPR_NO_GRAMMAR */
