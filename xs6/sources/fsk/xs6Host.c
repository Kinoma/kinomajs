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
#include "xs6All.h"
#include "xs6Script.h"

#define __FSKECMASCRIPT_PRIV__
#define __FSKNETUTILS_PRIV__

#include "FskEndian.h"
#include "FskEnvironment.h"
#include "FskFiles.h"
#include "FskECMAScript.h"
#include "FskPlatformImplementation.h"

#if mxWindows
	#define PATH_MAX 1024
	#define mxSeparator '\\'
#else
	#define mxSeparator '/'
#endif

#define bailAssert(_ASSERTION) { if (!(_ASSERTION)) { err = kFskErrInvalidParameter; goto bail; } }
#define bailIfError(X) { err = (X); if (err != kFskErrNone) goto bail; }
#define bailIfNULL(X) { if ((X) == NULL) { err = kFskErrMemFull; goto bail; } }

typedef struct {
	FskFileMapping map;
	FskInt64 size;
	void* address;
	FskLibrary library;
	txCallback keys;
	txID symbolCount;
	txString* symbols;
	txID scriptCount;
	txScript* scripts;
	int baseLength;
	char base[PATH_MAX];
} txArchive;

typedef struct {
	unsigned char *buffer;
	FskInt64 offset;
	FskInt64 size;
} txFileMapStream;

typedef void (*txLoader)(txMachine*, txString, txID);

mxExport txBoolean fxFindArchive(txMachine* the, txString path);
mxExport void* fxMapArchive(txString path, txCallbackAt callbackAt);
mxExport void fxRunModule(txMachine* the, txString path);
mxExport void fxRunProgram(txMachine* the, txString path);
mxExport void fxUnmapArchive(void* it);

static int fxFileMapGetter(void* it);
static void fxLoadModuleJS(txMachine* the, txString path, txID moduleID);
static void fxLoadModuleJSB(txMachine* the, txString path, txID moduleID);
static FskErr fxLoadModuleJSBAtom(txMachine* the, FskFile fref, Atom* atom);
static void fxLoadModuleXML(txMachine* the, txString path, txID moduleID);
static void* fxMapAtom(void* p, Atom* atom);

static txString gxExtensions[] = { 
	".jsb", 
	".xsb", 
	".js", 
	".xml", 
	NULL,
};
static txLoader gxLoaders[] = {
	fxLoadModuleJSB,
	fxLoadModuleJSB,
	fxLoadModuleJS,
	fxLoadModuleXML,
	NULL,
};

/* COMPATIBILITY  */

typedef struct {
	txSize initialChunkSize; /* xs.h */
	txSize incrementalChunkSize; /* xs.h */
	txSize initialHeapCount; /* xs.h */
	txSize incrementalHeapCount; /* xs.h */
	txSize stackCount; /* xs.h */
	txSize symbolCount; /* xs.h */
	txSize symbolModulo; /* xs.h */
} txAllocation;

typedef struct {
	txString strings[5];
	txInteger sizes[5];
	txInteger offset;
	txInteger index;
} txNewFunctionStream;

mxExport txMachine* fxAliasMachine(txAllocation* theAllocation, txMachine* theMachine, txString theName, void* theContext);
mxExport void fxEnterSandbox(txMachine* the);
mxExport txBoolean fxExecute(txMachine*, void*, txGetter, txString, txInteger);
mxExport void fxLeaveSandbox(txMachine* the);
mxExport void fxLink(txMachine*, xsGrammar*);
mxExport void fxModuleURL(txMachine* the);
mxExport void fxNewFunction(txMachine*, txString, txInteger, txString, txInteger, txFlag, txString, txInteger);
static int fxNewFunctionStreamGetter(void* it);
mxExport txMachine* fxNewMachine(txAllocation*, xsGrammar*, void*);
mxExport void fxRunForIn(txMachine* the);
static void fxRunForInProperty(txMachine* the, txSlot* limit, txID id, txIndex index, txSlot* property);
mxExport void fxSandbox(txMachine* the);
mxExport txInteger fxScript(txMachine* the);

mxExport void fxParse(txMachine* the, void* theStream, txGetter theGetter, txString thePath, long theLine, txFlag theFlag);
mxExport void fxSerialize(txMachine* the, void* theStream, txPutter thePutter);

static void fx_get_sandbox(txMachine* the);
static void fx_Object_prototype_get_sandbox(txMachine* the);

static void fx_xs_execute(txMachine* the);
static void fx_xs_isInstanceOf(txMachine* the);
static void fx_xs_newInstanceOf(txMachine* the);
static void fx_xs_script(txMachine* the);

#ifdef mxProfile
static void fx_xs_isProfiling(txMachine* the);
static void fx_xs_getProfilingDirectory(txMachine* the);
static void fx_xs_setProfilingDirectory(txMachine* the);
static void fx_xs_startProfiling(txMachine* the);
static void fx_xs_stopProfiling(txMachine* the);
#endif

#ifdef mxDebug
static void fx_xs_debug_getAddress(txMachine* the);
static void fx_xs_debug_setAddress(txMachine* the);
static void fx_xs_debug_getAutomatic(txMachine* the);
static void fx_xs_debug_setAutomatic(txMachine* the);
static void fx_xs_debug_getBreakOnException(txMachine* the);
static void fx_xs_debug_setBreakOnException(txMachine* the);
static void fx_xs_debug_getConnected(txMachine* the);
static void fx_xs_debug_setConnected(txMachine* the);
static void fx_xs_debug_clearAllBreakpoints(txMachine* the);
static void fx_xs_debug_clearBreakpoint(txMachine* the);
static void fx_xs_debug_setBreakpoint(txMachine* the);
#endif

void* fxAllocateChunks(txMachine* the, txSize theSize)
{
	txByte* aData;
	txBlock* aBlock;

	if ((theSize < the->minimumChunksSize) && !(the->collectFlag & XS_SKIPPED_COLLECT_FLAG))
		theSize = the->minimumChunksSize;
	theSize += sizeof(txBlock);
	aData = (txByte *)c_malloc(theSize);
	if (!aData)
		fxJump(the);
	aBlock = (txBlock*)aData;
	aBlock->nextBlock = the->firstBlock;
	aBlock->current = aData + sizeof(txBlock);
	aBlock->limit = aData + theSize;
	aBlock->temporary = C_NULL;
	the->firstBlock = aBlock;
	the->maximumChunksSize += theSize;
#if mxReport
	fxReport(the, "# Chunk allocation: reserved %ld used %ld peak %ld bytes\n", 
		the->maximumChunksSize, the->currentChunksSize, the->peakChunksSize);
#endif
#if __FSK_LAYER__
	FskInstrumentedItemSendMessageNormal(the, kFskXSInstrAllocateChunks, the);
#endif

	return aData;
}

txSlot* fxAllocateSlots(txMachine* the, txSize theCount)
{
	txSlot* result;
	
	result = (txSlot *)c_malloc(theCount * sizeof(txSlot));
	if (!result)
		fxJump(the);
	return result;
}

void fxBuildHost(txMachine* the)
{
	txSlot* property;
	txSlot* slot;
	
	fxNewHostAccessorGlobal(the, fx_get_sandbox, C_NULL, fxID(the, "sandbox"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	property = fxLastProperty(the, mxObjectPrototype.value.reference);
	property = fxNextHostAccessorProperty(the, property, fx_Object_prototype_get_sandbox, C_NULL, fxID(the, "sandbox"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);

	mxPush(mxObjectPrototype);
	property = fxLastProperty(the, fxNewObjectInstance(the));
	property = fxNextHostFunctionProperty(the, property, fx_xs_execute, 1, fxID(the, "execute"), XS_GET_ONLY);
	property = fxNextHostFunctionProperty(the, property, fx_xs_isInstanceOf, 2, fxID(the, "isInstanceOf"), XS_GET_ONLY);
	property = fxNextHostFunctionProperty(the, property, fx_xs_newInstanceOf, 1, fxID(the, "newInstanceOf"), XS_GET_ONLY);
	property = fxNextHostFunctionProperty(the, property, fx_xs_script, 1, fxID(the, "script"), XS_GET_ONLY);
#ifdef mxProfile
	property = fxNextHostFunctionProperty(the, property, fx_xs_isProfiling, 0, fxID(the, "isProfiling"), XS_GET_ONLY);
	property = fxNextHostFunctionProperty(the, property, fx_xs_getProfilingDirectory, 0, fxID(the, "getProfilingDirectory"), XS_GET_ONLY);
	property = fxNextHostFunctionProperty(the, property, fx_xs_setProfilingDirectory, 1, fxID(the, "setProfilingDirectory"), XS_GET_ONLY);
	property = fxNextHostFunctionProperty(the, property, fx_xs_startProfiling, 0, fxID(the, "startProfiling"), XS_GET_ONLY);
	property = fxNextHostFunctionProperty(the, property, fx_xs_stopProfiling, 0, fxID(the, "stopProfiling"), XS_GET_ONLY);
#endif
#ifdef mxDebug
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_xs_debug_getAddress, 0, fxID(the, "getAddress"), XS_GET_ONLY);
	slot = fxNextHostFunctionProperty(the, slot, fx_xs_debug_setAddress, 1, fxID(the, "setAddress"), XS_GET_ONLY);
	slot = fxNextHostFunctionProperty(the, slot, fx_xs_debug_getAutomatic, 0, fxID(the, "getAutomatic"), XS_GET_ONLY);
	slot = fxNextHostFunctionProperty(the, slot, fx_xs_debug_setAutomatic, 1, fxID(the, "setAutomatic"), XS_GET_ONLY);
	slot = fxNextHostFunctionProperty(the, slot, fx_xs_debug_getBreakOnException, 0, fxID(the, "getBreakOnException"), XS_GET_ONLY);
	slot = fxNextHostFunctionProperty(the, slot, fx_xs_debug_setBreakOnException, 1, fxID(the, "setBreakOnException"), XS_GET_ONLY);
	slot = fxNextHostFunctionProperty(the, slot, fx_xs_debug_getConnected, 0, fxID(the, "getConnected"), XS_GET_ONLY);
	slot = fxNextHostFunctionProperty(the, slot, fx_xs_debug_setConnected, 1, fxID(the, "setConnected"), XS_GET_ONLY);
	slot = fxNextHostFunctionProperty(the, slot, fx_xs_debug_clearAllBreakpoints, 0, fxID(the, "clearAllBreakpoints"), XS_GET_ONLY);
	slot = fxNextHostFunctionProperty(the, slot, fx_xs_debug_clearBreakpoint, 2, fxID(the, "clearBreakpoint"), XS_GET_ONLY);
	slot = fxNextHostFunctionProperty(the, slot, fx_xs_debug_setBreakpoint, 2, fxID(the, "setBreakpoint"), XS_GET_ONLY);
	property = fxNextSlotProperty(the, property, the->stack, fxID(the, "debug"), XS_GET_ONLY);
	the->stack++;
#endif
	slot = fxSetGlobalProperty(the, mxGlobal.value.reference, fxID(the, "xs"));
	slot->flag = XS_GET_ONLY;
	slot->kind = the->stack->kind;
	slot->value = the->stack->value;
	the->stack++;
}

void fxDisposeParserChunks(txParser* parser)
{
	txParserChunk* block = parser->first;
	while (block) {
		txParserChunk* next = block->next;
		c_free(block);
		block = next;
	}
}

void fxBuildKeys(txMachine* the)
{
	txArchive* archive = the->archive;
	txSlot* callback = &mxIDs;
	txID c, i;
	if (archive) {
		if (archive->keys)
			(*archive->keys)(the);
		else {
			c = archive->symbolCount;
			callback->value.callback.address = C_NULL;
			callback->value.callback.IDs = (txID*)fxNewChunk(the, c * sizeof(txID));
			callback->kind = XS_CALLBACK_KIND;	
			for (i = 0; i < XS_SYMBOL_ID_COUNT; i++) {
				txString string = archive->symbols[i];
				txID id = the->keyIndex;
				txSlot* description = fxNewSlot(the);
				fxCopyStringC(the, description, string);
				the->keyArray[id] = description;
				the->keyIndex++;
				mxID(i) = 0x8000 | id;
			}
			for (; i < c; i++) {
				txString string = archive->symbols[i];
				mxID(i) = fxNewNameX(the, string)->ID;
			}
		}
	}
	else {
		callback->value.callback.address = C_NULL;
		callback->value.callback.IDs = (txID*)fxNewChunk(the, XS_ID_COUNT * sizeof(txID));
		callback->kind = XS_CALLBACK_KIND;	
		for (i = 0; i < XS_SYMBOL_ID_COUNT; i++) {
			txID id = the->keyIndex;
			txSlot* description = fxNewSlot(the);
			fxCopyStringC(the, description, gxIDStrings[i]);
			the->keyArray[id] = description;
			the->keyIndex++;
			mxID(i) = 0x8000 | id;
		}
		for (; i < XS_ID_COUNT; i++) {
			mxID(i) = fxID(the, gxIDStrings[i]);
		}
	}
}

int fxFileMapGetter(void* it)
{
	txFileMapStream* self = it;
	int result = EOF;
	if (self->offset < self->size) {
		result = *(self->buffer + self->offset);
		self->offset++;
	}
	return result;
}

txBoolean fxFindArchive(txMachine* the, txString path)
{
	txArchive* archive = the->archive;
	if (archive) {
		if (!c_strncmp(path, archive->base, archive->baseLength)) {
			txInteger c = archive->scriptCount;
			txScript* script = archive->scripts;
			char name[PATH_MAX];
			c_strcpy(name, path + archive->baseLength);
			#if mxWindows
			{
				char* separator = name;
				while (*separator) {
					if (*separator == '/')
						*separator = mxSeparator;
					separator++;
				}
			}
			#endif
			while (c > 0) {
				if (!c_strcmp(name, script->path))
					return 1;
				c--;
				script++;
			}
		}
	}
	return 0;
}

void fxFreeChunks(txMachine* the, void* theChunks)
{
	c_free(theChunks);
}

void fxFreeSlots(txMachine* the, void* theSlots)
{
	c_free(theSlots);
}

extern FskErr KprPathToURL(char* path, char** result);
extern FskErr KprURLMerge(char* base, char* reference, char** result);
extern FskErr KprURLToPath(char* url, char** result);

void fxIncludeScript(txParser* parser, txString string) 
{
	txBoolean done = 0;
	char* base = NULL;
	char name[PATH_MAX];
	char* slash = NULL;
	char* dot = NULL;
	char* extension = NULL;
	char* url = NULL;
	char* path = NULL;
	FskFileInfo fileInfo;
	FskFileMapping map= NULL;
	txFileMapStream fileMapStream;
	txStringStream stringStream;
	txMachine* the = parser->console;
	fxBeginHost(the);
	{
		mxTry(the) {
			xsThrowIfFskErr(KprPathToURL(parser->path->string, &base));
			FskStrCopy(name, string);
			slash = FskStrRChr(name, '/');
			if (!slash)
				slash = name;
			dot = FskStrRChr(slash, '.');
			if (dot)
				extension = dot;
			else
				extension = name + FskStrLen(name);
			if (!dot)
				FskStrCopy(extension, ".js");
			if (!FskStrCompare(extension, ".js")) {
				xsThrowIfFskErr(KprURLMerge(base, name, &url));
				xsThrowIfFskErr(KprURLToPath(url, &path));
				if (kFskErrNone == FskFileGetFileInfo(path, &fileInfo)) {
					xsThrowIfFskErr(FskFileMap(path, (unsigned char**)&(fileMapStream.buffer), &(fileMapStream.size), 0, &map));
					fileMapStream.offset = 0;
					fxIncludeTree(parser, &fileMapStream, fxFileMapGetter, parser->flags, path);
					done = 1;
					FskFileDisposeMap(map);
					map = NULL;
				}
				FskMemPtrDisposeAt(&path);
				FskMemPtrDisposeAt(&url);
			}
			if (!dot)
				FskStrCopy(extension, ".xml");
			if (!FskStrCompare(extension, ".xml")) {
				xsThrowIfFskErr(KprURLMerge(base, name, &url));
				xsThrowIfFskErr(KprURLToPath(url, &path));
				if (kFskErrNone == FskFileGetFileInfo(path, &fileInfo)) {
					xsThrowIfFskErr(FskFileMap(path, &fileMapStream.buffer, &fileMapStream.size, 0, &map));
					fileMapStream.offset = 0;
					mxPushInteger(0);
					mxPushInteger(0);
					mxPushInteger(0);
					mxPushInteger(0);
					mxPushInteger(3);
					fxParse(the, &fileMapStream, fxFileMapGetter, path, 1, xsSourceFlag | xsDebugFlag);
					fxCallID(the, fxID(the, "generate"));
					fxToString(the, the->stack);
					stringStream.slot = the->stack;
					stringStream.size = c_strlen(stringStream.slot->value.string);
					stringStream.offset = 0;
					fxIncludeTree(parser, &stringStream, fxStringGetter, parser->flags, path);
					done = 1;
					FskFileDisposeMap(map);
					map = NULL;
				}
				FskMemPtrDisposeAt(&path);
				FskMemPtrDisposeAt(&url);
			}
			FskMemPtrDispose(base);
		}
		mxCatch(the) {
			FskFileDisposeMap(map);
			FskMemPtrDispose(path);
			FskMemPtrDispose(url);
			FskMemPtrDispose(base);
			break;
		}
	}
	fxEndHost(the);
	if (!done)	
		fxReportParserError(parser, "include file not found: %s", string);
}

void fxLoadModule(txMachine* the, txID moduleID)
{
	txArchive* archive = the->archive;
	txSlot* key = fxGetKey(the, moduleID);
	txString path = NULL;
 	char buffer[PATH_MAX];
	txString dot = NULL;
	txString* extension;
	txLoader* loader;
 	KprURLToPath(key->value.key.string, &path);
	c_strcpy(buffer, path);
	c_free(path);
	path = buffer;
	if (archive) {
		if (!c_strncmp(path, archive->base, archive->baseLength)) {
			txInteger c = archive->scriptCount, i;
			txScript* script = archive->scripts;
			path += archive->baseLength;
			#if mxWindows
			{
				char* separator = path;
				while (*separator) {
					if (*separator == '/')
						*separator = mxSeparator;
					separator++;
				}
			}
			#endif
			for (i = 0; i < c; i++) {
				if (!c_strcmp(path, script->path)) {
					fxResolveModule(the, moduleID, script, C_NULL, C_NULL);
					return;
				}
				script++;
			}
		}
	}
	dot = FskStrRChr(path, '.');
	for (extension = gxExtensions, loader = gxLoaders; *extension; extension++, loader++) {
		if (!FskStrCompare(dot, *extension)) {
			(**loader)(the, buffer, moduleID);
			break;
		}
	}
}

void fxLoadModuleJS(txMachine* the, txString path, txID moduleID)
{
	FskErr err = kFskErrNone;
	FskFileMapping map = NULL;
	txFileMapStream stream;
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	txScript* script = NULL;
	bailIfError(FskFileMap(path, (unsigned char**)&(stream.buffer), &(stream.size), 0, &map));
	stream.offset = 0;
	fxInitializeParser(parser, the, 32*1024, 1993);
	parser->firstJump = &jump;
	parser->path = fxNewParserSymbol(parser, path);
	if (c_setjmp(jump.jmp_buf) == 0) {
		fxParserTree(parser, &stream, fxFileMapGetter, 2, NULL);
		fxParserHoist(parser);
		fxParserBind(parser);
		script = fxParserCode(parser);
	}
	fxTerminateParser(parser);
bail:
	FskFileDisposeMap(map);
	if (err && script) {
		fxDeleteScript(script);
		script = NULL;
	}
	fxResolveModule(the, moduleID, script, NULL, NULL);
}

void fxLoadModuleJSB(txMachine* the, txString path, txID moduleID)
{
	FskErr err = kFskErrNone;
	txScript* script = NULL;
	FskFile fref = NULL;
	Atom atom;

	bailIfError(FskMemPtrNewClear(sizeof(txScript), &script));
	bailIfError(FskFileOpen(path, kFskFilePermissionReadOnly, &fref));
	bailIfError(fxLoadModuleJSBAtom(the, fref, &atom));
	bailAssert(atom.atomType == XS_ATOM_BINARY);
	
	bailIfError(fxLoadModuleJSBAtom(the, fref, &atom));
	bailAssert(atom.atomType == XS_ATOM_VERSION);
	bailIfError(FskFileRead(fref, sizeof(script->version), script->version, NULL));
	bailAssert(script->version[0] == XS_MAJOR_VERSION);
	bailAssert(script->version[1] == XS_MINOR_VERSION);
	bailAssert(script->version[2] == XS_PATCH_VERSION);
	bailAssert(script->version[3] != -1);
	
	bailIfError(fxLoadModuleJSBAtom(the, fref, &atom));
	bailAssert(atom.atomType == XS_ATOM_SYMBOLS);
	script->symbolsSize = atom.atomSize - sizeof(atom);
	bailIfError(FskMemPtrNew(script->symbolsSize, &script->symbolsBuffer));
	bailIfError(FskFileRead(fref, script->symbolsSize, script->symbolsBuffer, NULL));
	
	bailIfError(fxLoadModuleJSBAtom(the, fref, &atom));
	bailAssert(atom.atomType == XS_ATOM_CODE);
	script->codeSize = atom.atomSize - sizeof(atom);
	bailIfError(FskMemPtrNew(script->codeSize, &script->codeBuffer));
	bailIfError(FskFileRead(fref, script->codeSize, script->codeBuffer, NULL));

bail:
	if (fref)
		FskFileClose(fref);
	if (err) {
		if (script) {
			fxDeleteScript(script);
			script = NULL;
		}
	}
	fxResolveModule(the, moduleID, script, NULL, NULL);
}

FskErr fxLoadModuleJSBAtom(txMachine* the, FskFile fref, Atom* atom)
{
	FskErr err = kFskErrNone;
	char buffer[sizeof(Atom)];
	void* p = buffer;
	bailIfError(FskFileRead(fref, sizeof(Atom), p, NULL));
	mxDecode4(p, atom->atomSize);
	mxDecode4(p, atom->atomType);
bail:
	return err;
}

void fxLoadModuleXML(txMachine* the, txString path, txID moduleID)
{
	FskFileMapping map = NULL;
	txFileMapStream fileMapStream;
	txStringStream stringStream;
	txScript* script = NULL;
	
	mxTry(the) {
		fxBeginHost(the);
		xsThrowIfFskErr(FskFileMap(path, &fileMapStream.buffer, &fileMapStream.size, 0, &map));
		fileMapStream.offset = 0;
		mxPushInteger(0);
		mxPushInteger(0);
		mxPushInteger(0);
		mxPushInteger(0);
		mxPushInteger(3);
		fxParse(the, &fileMapStream, fxFileMapGetter, path, 1, xsSourceFlag | xsDebugFlag);
		fxCallID(the, fxID(the, "generate"));
		fxToString(the, the->stack);
		stringStream.slot = the->stack;
		stringStream.size = c_strlen(stringStream.slot->value.string);
		stringStream.offset = 0;
		script = fxParseScript(the, &stringStream, fxStringGetter, mxDebugFlag);
		fxEndHost(the);
	}
	mxCatch(the) {
		break;
	}
	FskFileDisposeMap(map);
	fxResolveModule(the, moduleID, script, NULL, NULL);
}

void* fxMapArchive(txString base, txCallbackAt callbackAt)
{
	FskErr err = kFskErrNone;
	txArchive* archive = NULL;
	txString p;
	Atom atom;
	int c, i;
	txString* symbol;
	txScript* script;

	bailIfError(FskMemPtrNewClear(sizeof(txArchive), &archive));
	c_strcpy(archive->base, base);
	bailIfError(FskFileMap(base, (unsigned char**)&(archive->address), &(archive->size), 0, &(archive->map)));

	p = archive->address;
	p = fxMapAtom(p, &atom);
	bailAssert(atom.atomType == XS_ATOM_ARCHIVE);
	
	p = fxMapAtom(p, &atom);
	bailAssert(atom.atomType == XS_ATOM_VERSION);
	bailAssert(*p++ == XS_MAJOR_VERSION);
	bailAssert(*p++ == XS_MINOR_VERSION);
	bailAssert(*p++ == XS_PATCH_VERSION);
	if ((*p++) && !callbackAt) {
		char* dot;
		dot = c_strrchr(archive->base, '.');
		if (dot)
			*dot = 0;
		#if mxWindows
			c_strcat(archive->base, ".dll");
		#else
			c_strcat(archive->base, ".so");
		#endif
		bailIfError(FskLibraryLoad(&(archive->library), archive->base));
		bailIfError(FskLibraryGetSymbolAddress(archive->library, "xsHostModuleAt", &callbackAt));
		archive->keys = callbackAt(0);
	}

	p = fxMapAtom(p, &atom);
	bailAssert(atom.atomType == XS_ATOM_SYMBOLS);
	mxDecode2(p, archive->symbolCount);
	c = archive->symbolCount;
	if (c) {
		bailIfError(FskMemPtrNew(c * sizeof(txString), &(archive->symbols)));
		symbol = archive->symbols;
		for (i = 0; i < c; i++) {
			*symbol = p;
			p += c_strlen(p) + 1;
			symbol++;
		}
	}

	p = fxMapAtom(p, &atom);
	bailAssert(atom.atomType == XS_ATOM_PATHS);
	mxDecode2(p, archive->scriptCount);
	c = archive->scriptCount;
	bailIfError(FskMemPtrNewClear(c * sizeof(txScript), &(archive->scripts)));
	script = archive->scripts;
	for (i = 1; i <= c; i++) {
		if (callbackAt)
			script->callback = callbackAt(i);
		script->path = p;
		bailIfNULL(script->path);
		p += c_strlen(p) + 1;
		script++;
	}
	
	script = archive->scripts;
	for (i = 0; i < c; i++) {
		p = fxMapAtom(p, &atom);
		bailAssert(atom.atomType == XS_ATOM_CODE);
		script->codeSize = atom.atomSize - sizeof(atom);
		script->codeBuffer = (txS1*)p;
		p += script->codeSize;
		script++;
	}
	
	p = c_strrchr(archive->base, '/');
	if (p) {
		p++;
		*p = 0;
		archive->baseLength = p - archive->base;
	}
bail:
	if (err) {
		fxUnmapArchive(archive);
		archive = NULL;
	}
	return archive;
}

void* fxMapAtom(void* p, Atom* atom)
{
	mxDecode4(p, atom->atomSize);
	mxDecode4(p, atom->atomType);
	return p;
}

void fxMarkHost(txMachine* the)
{
}

void* fxNewParserChunk(txParser* parser, txSize size)
{
	txParserChunk* block = c_malloc(sizeof(txParserChunk) + size);
	if (!block)
		fxThrowMemoryError(parser);
	parser->total += sizeof(txParserChunk) + size;
	block->next = parser->first;
	parser->first = block;
	return block + 1;
}

txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags)
{	
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	txScript* script = NULL;
	fxInitializeParser(parser, the, 32*1024, 1993);
	parser->firstJump = &jump;
	if (c_setjmp(jump.jmp_buf) == 0) {
		fxParserTree(parser, stream, getter, flags, NULL);
		fxParserHoist(parser);
		fxParserBind(parser);
		script = fxParserCode(parser);
	}
	fxTerminateParser(parser);
	return script;
}

void fxPerformPromiseJobs(void* it) 
{
	txMachine* the = it;
	fxBeginHost(the);
	{
		fxRunPromiseJobs(the);
	}
	fxEndHost(the);
}

void fxQueuePromiseJobs(txMachine* the)
{
	FskThread thread = FskThreadGetCurrent();
	FskThreadPostCallback(thread, (FskThreadCallback)fxPerformPromiseJobs, the, NULL, NULL, NULL);
}

void fxRunModule(txMachine* the, txString path)
{
	mxPushStringC(path);
	fxRequireModule(the, XS_NO_ID, the->stack);
	the->stack++;
}

void fxRunProgram(txMachine* the, txString path)
{
	txArchive* archive = the->archive;
	if (archive) {
		if (!c_strncmp(path, archive->base, archive->baseLength)) {
			txInteger c = archive->scriptCount, i;
			txScript* script = archive->scripts;
			path += archive->baseLength;
			for (i = 0; i < c; i++) {
				if (!c_strcmp(path, script->path)) {
					fxRunScript(the, script, &mxGlobal, C_NULL, C_NULL, C_NULL);
					return;
				}
				script++;
			}
		}
	}
}

void fxSweepHost(txMachine* the)
{
}

void fxUnmapArchive(void* it)
{
	txArchive* archive = it;
	if (archive) {
		if (archive->scripts)
			FskMemPtrDispose(archive->scripts);
		if (archive->symbols)
			FskMemPtrDispose(archive->symbols);
		if (archive->library)
			FskLibraryUnload(archive->library);
		FskFileDisposeMap(archive->map);
		FskMemPtrDispose(archive);
	}
}

/* COMPATIBILITY  */

txMachine* fxAliasMachine(txAllocation* theAllocation, txMachine* theMachine, txString theName, void* theContext)
{
	txCreation creation;
	creation.initialChunkSize = theAllocation->initialChunkSize;
	creation.incrementalChunkSize = theAllocation->incrementalChunkSize;
	creation.initialHeapCount = theAllocation->initialHeapCount;
	creation.incrementalHeapCount = theAllocation->incrementalHeapCount;
	creation.stackCount = theAllocation->stackCount;
	creation.keyCount = theAllocation->symbolCount;
	creation.nameModulo = theAllocation->symbolModulo;
	creation.symbolModulo = 127;
	return fxCloneMachine(&creation, theMachine, theName, theContext);
}

void fxEnterSandbox(txMachine* the)
{
	/* nop */
}

txBoolean fxExecute(txMachine* the, void* theStream, txGetter theGetter, txString thePath, txInteger theLine)
{
#ifdef mxDebug
	txUnsigned flags = mxProgramFlag | mxDebugFlag;
#else
	txUnsigned flags = mxProgramFlag;
#endif
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	txScript* script = NULL;
	fxInitializeParser(parser, the, 32*1024, 1993);
	parser->firstJump = &jump;
	if (c_setjmp(jump.jmp_buf) == 0) {
		if (thePath)
			parser->path = fxNewParserSymbol(parser, thePath);
		fxParserTree(parser, theStream, theGetter, flags, NULL);
		fxParserHoist(parser);
		fxParserBind(parser);
		script = fxParserCode(parser);
	}
	fxTerminateParser(parser);
	if (script) {
		fxRunScript(the, script, &mxGlobal, C_NULL, C_NULL, C_NULL);
		return 1;
	}
	return 0;
}

void fxLeaveSandbox(txMachine* the)
{
	/* nop */
}

void fxLink(txMachine* the, xsGrammar* theGrammar)
{
	txScript* script = NULL;
	FskErr err = kFskErrNone;
	err = FskMemPtrNewClear(sizeof(txScript), &script);
	if (err) goto bail;
	err = FskMemPtrNew(theGrammar->symbolsSize, &script->symbolsBuffer);
	if (err) goto bail;
	err = FskMemPtrNew(theGrammar->codeSize, &script->codeBuffer);
	if (err) goto bail;
	FskMemCopy(script->symbolsBuffer, theGrammar->symbols, theGrammar->symbolsSize);
	script->symbolsSize = theGrammar->symbolsSize;
	FskMemCopy(script->codeBuffer, theGrammar->code, theGrammar->codeSize);
	script->codeSize = theGrammar->codeSize;
	script->callback = theGrammar->callback;
	fxRunScript(the, script, C_NULL, C_NULL, C_NULL, C_NULL);
	script = C_NULL;
bail:
	fxDeleteScript(script);
	return;
}

void fxModuleURL(txMachine* the)
{
	txID id = fxCurrentModuleID(the);
	if (id != XS_NO_ID) {
		txSlot* key = fxGetKey(the, id);
		if (key->kind == XS_KEY_KIND)
			mxPushString(key->value.key.string);
		else
			mxPushStringX(key->value.key.string);
	}
    else {
        mxPushUndefined();
    }
}

void fxNewFunction(txMachine* the, txString arguments, txInteger argumentsSize, txString body, txInteger bodySize, txFlag theFlag, txString thePath, txInteger theLine)
{
	txNewFunctionStream stream;
#ifdef mxDebug
	txUnsigned flags = mxProgramFlag | mxDebugFlag;
#else
	txUnsigned flags = mxProgramFlag;
#endif
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	txScript* script = NULL;
	stream.strings[0] = "(function";
	stream.strings[1] = arguments;
	stream.strings[2] = "{";
	stream.strings[3] = body;
	stream.strings[4] = "})";
	stream.sizes[0] = 9;
	stream.sizes[1] = argumentsSize;
	stream.sizes[2] = 1;
	stream.sizes[3] = bodySize;
	stream.sizes[4] = 2;
	stream.offset = 0;
	stream.index = 0;
	fxInitializeParser(parser, the, 32*1024, 1993);
	parser->firstJump = &jump;
	if (c_setjmp(jump.jmp_buf) == 0) {
		if (thePath)
			parser->path = fxNewParserSymbol(parser, thePath);
		fxParserTree(parser, &stream, fxNewFunctionStreamGetter, flags, NULL);
		fxParserHoist(parser);
		fxParserBind(parser);
		script = fxParserCode(parser);
	}
	fxTerminateParser(parser);
	fxRunScript(the, script, C_NULL, C_NULL, C_NULL, C_NULL);
}

int fxNewFunctionStreamGetter(void* it)
{
	txNewFunctionStream* stream = (txNewFunctionStream*)it;
	int result = C_EOF;
	while (stream->index < 5) {
		if (stream->offset < stream->sizes[stream->index]) {
			result = *(stream->strings[stream->index] + stream->offset);
			stream->offset++;
			break;
		}
		stream->index++;
		stream->offset = 0;
	}
	return result;
}

txMachine* fxNewMachine(txAllocation* theAllocation, xsGrammar* theGrammar, void* theContext)
{
	txCreation creation;
	creation.initialChunkSize = theAllocation->initialChunkSize;
	creation.incrementalChunkSize = theAllocation->incrementalChunkSize;
	creation.initialHeapCount = theAllocation->initialHeapCount;
	creation.incrementalHeapCount = theAllocation->incrementalHeapCount;
	creation.stackCount = theAllocation->stackCount;
	creation.keyCount = theAllocation->symbolCount;
	creation.nameModulo = theAllocation->symbolModulo;
	creation.symbolModulo = 127;
	txMachine* the = fxCreateMachine(&creation, NULL, theGrammar->name, theContext);
	if (the) {
		mxTry(the) {
			fxLink(the, theGrammar);
		}
		mxCatch(the) {
		}
	}
	return the;
}

void fxRunForIn(txMachine* the)
{
	txSlot* limit = the->stack;
	txSlot* slot = fxToInstance(the, limit);
	while (slot) {
		fxEachInstanceProperty(the, slot, XS_EACH_ENUMERABLE_FLAG | XS_EACH_STRING_FLAG, fxRunForInProperty, limit, slot);
		slot = fxGetParent(the, slot);
	}
	slot = the->stack;
	while (slot < limit) {
		txInteger id = slot->value.integer;
		if (id < 0) {
			txSlot* key = fxGetKey(the, (txID)id);
			if (key && (key->flag & XS_DONT_ENUM_FLAG)) {
				if (key->kind == XS_KEY_KIND) {
					slot->kind = XS_STRING_KIND;
					slot->value.string = key->value.key.string;
				}
				else {
					slot->kind = XS_STRING_X_KIND;
					slot->value.string = key->value.key.string;
				}
			}
			else {
				slot->kind = XS_SYMBOL_KIND;
				slot->value.symbol = (txID)id;
			}
		}
		slot++;
	}
	limit->kind = XS_NULL_KIND;
}

void fxRunForInProperty(txMachine* the, txSlot* limit, txID id, txIndex index, txSlot* property) 
{
	txSlot* slot = the->stack;
	while (slot < limit) {
		if (slot->value.integer == id)
			return;
		slot++;
	}
	mxPushInteger(id);
}

void fxSandbox(txMachine* the)
{
	/* nop */
}

txInteger fxScript(txMachine* the)
{
	return 0;
}

void fx_get_sandbox(txMachine* the)
{
	*mxResult = mxGlobal;
}

void fx_Object_prototype_get_sandbox(txMachine* the)
{
	*mxResult = *mxThis;
}

void fx_xs_execute(txMachine* the)
{
	if (mxArgc > 0) {
		txStringStream stream;
		txString path;
		txInteger line;
		fxToString(the, mxArgv(0));
		stream.slot = mxArgv(0);
		stream.size = c_strlen(stream.slot->value.string);
		stream.offset = 0;
		path = (mxArgc > 1) ? fxToString(the, mxArgv(1)) : NULL;
		line = (mxArgc > 2) ? fxToInteger(the, mxArgv(2)) : 0;
		fxExecute(the, &stream, fxStringGetter, path, line);
		*mxResult = *the->stack;
	}
}

void fx_xs_isInstanceOf(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("instance is no object");
	if ((mxArgc < 2) || (mxArgv(1)->kind != XS_REFERENCE_KIND))
		mxTypeError("prototype is no object");
	mxPushSlot(mxArgv(1));
	mxPushSlot(mxArgv(0));
	mxResult->value.boolean = fxIsInstanceOf(the);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_xs_newInstanceOf(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_REFERENCE_KIND))
		mxTypeError("prototype is no object");
	mxPushSlot(mxArgv(0));
	fxNewInstanceOf(the);
	mxPullSlot(mxResult);
}

void fx_xs_script(txMachine* the)
{
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = 0;
}

#ifdef mxDebug

void fx_xs_debug_getAddress(txMachine* the)
{
	fxCopyStringC(the, mxResult, fxGetAddress(the));
}

void fx_xs_debug_setAddress(txMachine* the)
{
	if ((mxArgc < 1) || (!mxIsStringPrimitive(mxArgv(0))))
		mxTypeError("address is no string");
	fxSetAddress(the, mxArgv(0)->value.string);
}

void fx_xs_debug_getAutomatic(txMachine* the)
{
	mxResult->value.boolean = fxGetAutomatic(the);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_xs_debug_setAutomatic(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_BOOLEAN_KIND))
		mxTypeError("argument is no boolean");
	fxSetAutomatic(the, mxArgv(0)->value.boolean);
}

void fx_xs_debug_getBreakOnException(txMachine* the)
{
	mxResult->value.boolean = the->breakOnExceptionFlag;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_xs_debug_setBreakOnException(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_BOOLEAN_KIND))
		mxTypeError("argument is no boolean");
	the->breakOnExceptionFlag = mxArgv(0)->value.boolean;
}

void fx_xs_debug_getConnected(txMachine* the)
{
	mxResult->value.boolean = fxIsConnected(the);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_xs_debug_setConnected(txMachine* the)
{
	if ((mxArgc < 1) || (mxArgv(0)->kind != XS_BOOLEAN_KIND))
		mxTypeError("argument is no boolean");
	if (mxArgv(0)->value.boolean)
		fxLogin(the);
	else
		fxLogout(the);
}

void fx_xs_debug_clearAllBreakpoints(txMachine* the)
{
	fxClearAllBreakpoints(the);
}

void fx_xs_debug_clearBreakpoint(txMachine* the)
{
	if ((mxArgc < 1) || (!mxIsStringPrimitive(mxArgv(0))))
		mxTypeError("file is no string");
	if ((mxArgc < 2) || (!mxIsStringPrimitive(mxArgv(1))))
		mxTypeError("line is no string");
	fxClearBreakpoint(the, mxArgv(0)->value.string, mxArgv(1)->value.string);
}

void fx_xs_debug_setBreakpoint(txMachine* the)
{
	if ((mxArgc < 1) || (!mxIsStringPrimitive(mxArgv(0))))
		mxTypeError("file is no string");
	if ((mxArgc < 2) || (!mxIsStringPrimitive(mxArgv(1))))
		mxTypeError("line is no string");
	fxSetBreakpoint(the, mxArgv(0)->value.string, mxArgv(1)->value.string);
}

#endif

#ifdef mxProfile

void fx_xs_isProfiling(txMachine* the)
{
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = fxIsProfiling(the);
}

void fx_xs_getProfilingDirectory(txMachine* the)
{
	if (the->profileDirectory)
		fxString(the, mxResult, the->profileDirectory);
}

void fx_xs_setProfilingDirectory(txMachine* the)
{
	if (the->profileDirectory) {
		c_free(the->profileDirectory);
		the->profileDirectory = C_NULL;
	}
	if (mxArgc > 0) {
		mxPushSlot(mxArgv(0));
		if (fxRunTest(the)) {
			txString aString = fxToString(the, mxArgv(0));
			txInteger aLength = c_strlen(aString) + 1;
			the->profileDirectory = c_malloc(aLength);
			if (!the->profileDirectory)
				fxJump(the);
			c_memcpy(the->profileDirectory, aString, aLength);
		}
	}
}

void fx_xs_startProfiling(txMachine* the)
{
	fxStartProfiling(the);
}

void fx_xs_stopProfiling(txMachine* the)
{
	fxStopProfiling(the);
}

#endif

FskErr loadGrammar(const char *xsbPath, xsGrammar *theGrammar)
{
	FskErr err;
	FskFile fref = NULL;
	UInt32 atom[2];
	void* p = atom;

	err = FskFileOpen(xsbPath, kFskFilePermissionReadOnly, &fref);
	if (err) goto bail;

	err = FskFileRead(fref, sizeof(atom), atom, NULL);
	if (err) goto bail;

	mxDecode4(p, atom[0]);
	mxDecode4(p, atom[1]);
	if (atom[1] == 'XS6B') {
		SInt32 totalSize = (SInt32)atom[0] - sizeof(atom);
		while (totalSize > 0) {
			UInt32 blockSize;
			char *block;

			err = FskFileRead(fref, sizeof(atom), atom, NULL);
			if (err) break;
			p = atom;
			mxDecode4(p, atom[0]);
			mxDecode4(p, atom[1]);

			totalSize -= atom[0];

			blockSize = atom[0] - sizeof(atom);
			err = FskMemPtrNew(blockSize, &block);
			if (err) break;

			err = FskFileRead(fref, blockSize, block, NULL);
			if (err) break;

			switch (atom[1]) {
				case 'SYMB':
					theGrammar->symbols = block;
					theGrammar->symbolsSize = blockSize;
					break;

				case 'CODE':
					theGrammar->code = block;
					theGrammar->codeSize = blockSize;
					break;

				case 'VERS':
					FskMemPtrDispose(block);
					break;

				default:
					FskMemPtrDispose(block);
					err = kFskErrBadData;
					break;
			}
		}
	}
	else
		err = kFskErrBadData;
bail:
	FskFileClose(fref);
	return err;
}

