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
#include "mc_event.h"
#if XS_ARCHIVE
#if mxMC
#include "mc.xs.h"
#include "mc.xsa.h"
#else
#include <dlfcn.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#endif
#ifdef mxParse
#include "xs6Script.h"
#define mxParserThrowElse(_ASSERTION) { if (!(_ASSERTION)) { parser->error = errno; c_longjmp(parser->firstJump->jmp_buf, 1); } }
#endif

#define bailAssert(_ASSERTION) { if (!(_ASSERTION)) { error = EINVAL; goto bail; } }
#define bailElse(_ASSERTION) { if (!(_ASSERTION)) { error = errno; goto bail; } }

static inline uint32_t swap32(uint32_t x)
{
	return (x << 24) | ((x & 0xff00) << 8)  | ((x & 0xff0000) >> 8) | (x >> 24);
}

#if mxLittleEndian
#define fix32(x)	swap32(x)
#else
#define fix32(x)	(x)
#endif

typedef struct sxModuleData txModuleData;
struct sxModuleData {
	void* library;
	txModuleData* next;
	txMachine* the;
	char path[1];
};

typedef struct {
	int fd;
	int size;
	void* address;
	void* library;
	txCallback keys;
	txID symbolCount;
	txString* symbols;
	txID scriptCount;
	txScript* scripts;
#if !mxMC
	int baseLength;
	char base[PATH_MAX];
#endif
} txArchive;

typedef struct {
	txMachine* the;
	txID moduleID;
} txJob;

typedef void (*txLoader)(txMachine*, txString, txID);


mxExport void* fxMapArchive(txString path, txCallbackAt callbackAt);
mxExport void fxRunModule(txMachine* the, txString path);
mxExport void fxRunProgram(txMachine* the, txString path);
mxExport void fxUnmapArchive(void* it);

static txBoolean fxFindArchive(txMachine* the, txString path);
static txBoolean fxFindFile(txMachine* the, txString path);
static txBoolean fxFindURI(txMachine* the, txString base, txString name, txString dot, txID* id);
static void* fxMapAtom(void* address, Atom* atom);
static txScript* fxLoadBinary(txMachine* the, txString path);
static void fxLoadBinaryAtom(txMachine* the, FILE* file, Atom* atom);
static void fxLoadBinaryModule(txMachine* the, txString path, txID moduleID);
static void fxLoadBinaryProgram(txMachine* the, txString path);
static txModuleData* fxLoadLibrary(txMachine* the, txString path, txScript* script);
static txString fxMergePath(txString base, txString name, txString path);
static void fxUnloadLibrary(void* it);
#ifdef mxParse
static txScript* fxLoadText(txMachine* the, txString path, txUnsigned flags);
static void fxLoadTextModule(txMachine* the, txString path, txID moduleID);
#endif

static const txString gxExtensions[] = { 
	".xsb",
	".jsb",
#ifdef mxParse
	".js", 
#endif
	C_NULL,
};
static const txLoader gxLoaders[] = {
	fxLoadBinaryModule,
	fxLoadBinaryModule,
#ifdef mxParse
	fxLoadTextModule,
#endif
	C_NULL,
};

#ifndef roundup
#define roundup(x, y)	((((x) + (y) - 1) / (y)) * (y))
#endif

#if mxMC
void* fxAllocateChunks(txMachine* the, txSize theSize)
{
	txByte* aData;
	txBlock* aBlock;

	if (!(the->collectFlag & XS_SKIPPED_COLLECT_FLAG))
		theSize = roundup(theSize, the->minimumChunksSize);
	theSize += sizeof(txBlock);
	aData = (txByte *)mc_xs_chunk_allocator(theSize);
	if (!aData) {
#ifdef mxDebug
		fxDebugThrow(the, __FILE__, __LINE__, "C: fxAllocateChunk");
#endif
		fxJump(the);
	}
	/* check if the newly allocated block is continuous to the current block */
	if (the->firstBlock != C_NULL && the->firstBlock->limit == aData) {
		the->firstBlock->limit += theSize;
		aBlock = the->firstBlock;
	}
	else {
		aBlock = (txBlock*)aData;
		aBlock->nextBlock = the->firstBlock;
		aBlock->current = aData + sizeof(txBlock);
		aBlock->limit = aData + theSize;
		aBlock->temporary = C_NULL;
		the->firstBlock = aBlock;
	}
	the->maximumChunksSize += theSize;
#if mxReport
	fxReport(the, "# Chunk allocation: reserved %ld used %ld peak %ld bytes\n", 
		the->maximumChunksSize, the->currentChunksSize, the->peakChunksSize);
#endif
	return aData;
}

txSlot* fxAllocateSlots(txMachine* the, txSize theCount)
{
	txSlot* result;
	
	result = (txSlot *)mc_xs_slot_allocator(theCount * sizeof(txSlot));
	if (!result) {
		fxReport(the, "# Slot allocation: failed. trying to make a room...\n");
		fxCollect(the, 1);	/* expecting memory from the chunk pool */
		if (the->firstBlock != C_NULL && the->firstBlock->limit == mc_xs_chunk_allocator(0)) {	/* sanity check just in case */
			fxReport(the, "# Slot allocation: %d bytes returned\n", the->firstBlock->limit - the->firstBlock->current);
			the->maximumChunksSize -= the->firstBlock->limit - the->firstBlock->current;
			mc_xs_sbrk(the->firstBlock->current);
			the->firstBlock->limit = the->firstBlock->current;
		}
		result = (txSlot *)mc_xs_slot_allocator(theCount * sizeof(txSlot));
		if (!result) {
#ifdef mxDebug
			fxDebugThrow(the, __FILE__, __LINE__, "C: fxAllocateSlots");
#endif
			fxJump(the);
		}
	}
	return result;
}

void fxFreeChunks(txMachine* the, void* theChunks)
{
	mc_xs_chunk_disposer(theChunks);
}

void fxFreeSlots(txMachine* the, void* theSlots)
{
	mc_xs_slot_disposer(theSlots);
}

#else /* mxMC */

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

void fxFreeChunks(txMachine* the, void* theChunks)
{
	c_free(theChunks);
}

void fxFreeSlots(txMachine* the, void* theSlots)
{
	c_free(theSlots);
}

#endif /* mxMC */

void fxBuildHost(txMachine* the)
{
	/* nothing to build for now */
}

void fxBuildKeys(txMachine* the)
{
	txArchive* archive = the->archive;
	if (archive && archive->keys)
		(*archive->keys)(the);
	else {
	#ifdef mxParse
		txSlot* callback = &mxIDs;
		txID c, i;
		if (archive) {
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
	#elif mxDebug
		fxCheck(the, __FILE__, __LINE__);
	#endif
	}
}

txBoolean fxFindArchive(txMachine* the, txString path)
{
	txArchive* archive = the->archive;
	if (archive) {
		txInteger c = archive->scriptCount;
		txScript* script = archive->scripts;
		while (c > 0) {
			if (!c_strcmp(path, script->path))
				return 1;
			c--;
			script++;
		}
	}
	return 0;
}

txBoolean fxFindFile(txMachine* the, txString path)
{
	char real[PATH_MAX];

	mc_check_stack();

	// fxReport(the, "# Finding module in the file system... \"%s\"\n", path);
	if (path[0] != '/')	/* @@ optimizing. assuming no modules in ftfs */
		return 0;
	if (realpath(path, real)) {
		c_strcpy(path, real);
		return 1;
	}
	return 0;
}

txID fxFindModule(txMachine* the, txID moduleID, txSlot* slot)
{
	char name[PATH_MAX];
	char base[PATH_MAX];
	txBoolean absolute, relative;
	txSlot *key, *iterator, *result;
	txString dot, slash;
	txID id;
	
	mc_check_stack();

	fxToStringBuffer(the, slot, name, sizeof(name));
	if ((!c_strncmp(name, "./", 2)) || (!c_strncmp(name, "../", 3))) {
		absolute = 0;
		relative = (moduleID == XS_NO_ID) ? 0 : 1;
	}
	else if ((!c_strncmp(name, "/", 1))) {
		c_memmove(name, name + 1, c_strlen(name));
		absolute = 1;
		relative = 0;
	}
	else {
		absolute = 1;
		relative = (moduleID == XS_NO_ID) ? 0 : 1;
	}
	slash = c_strrchr(name, '/');
	if (!slash)
		slash = name;
	dot = c_strrchr(slash, '.');
	if (!dot)
		dot = name + c_strlen(name);
	
	if (relative) {
		key = fxGetKey(the, moduleID);
  		c_strcpy(base, key->value.key.string);
		if (fxFindURI(the, base, name, dot, &id))
			return id;
	}
	if (absolute) {
		mxCallID(&mxModulePaths, mxID(_Symbol_iterator), 0);
		iterator = the->stack;
		for (;;) {
			mxCallID(iterator, mxID(_next), 0);
			result = the->stack;
			mxGetID(result, mxID(_done));
			if (fxToBoolean(the, the->stack))
				break;
			the->stack++;
			mxGetID(result, mxID(_value));
			fxToStringBuffer(the, the->stack++, base, sizeof(base));
			if (fxFindURI(the, base, name, dot, &id))
				return id;
		}
	}
	return XS_NO_ID;
}

txBoolean fxFindURI(txMachine* the, txString base, txString name, txString dot, txID* id)
{
	char path[PATH_MAX];
	txSlot* key;

	mc_check_stack();

	if (*dot) {
		fxMergePath(base, name, path);
		if (fxFindArchive(the, path) || fxFindFile(the, path)) {
			key = fxNewNameC(the, path);
			*id = key->ID;
			return 1; 
		}
	}
	else {
		txString const* extension;
		for (extension = gxExtensions; *extension; extension++) {
 			c_strcpy(dot, *extension);
			fxMergePath(base, name, path);
			if (fxFindArchive(the, path) || fxFindFile(the, path)) {
				key = fxNewNameC(the, path);
				*id = key->ID;
				return 1; 
			}
		}
		*dot = 0;
	}
	*id = XS_NO_ID;
	return 0;
}

txScript* fxLoadBinary(txMachine* the, txString path)
{
	int error = 0;
	txScript* script = NULL;
	FILE* file = NULL;
	Atom atom;
	txByte version[4];
	script = c_malloc(sizeof(txScript));
	bailElse(script);
	c_memset(script, 0, sizeof(txScript));
	file = fopen(path, "rb");
	bailElse(file);
	fxLoadBinaryAtom(the, file, &atom);
	bailAssert(atom.atomType == XS_ATOM_BINARY);
	
	fxLoadBinaryAtom(the, file, &atom);
	bailAssert(atom.atomType == XS_ATOM_VERSION);
	bailElse(fread(script->version, sizeof(version), 1, file) == 1);	
	bailAssert(script->version[0] == XS_MAJOR_VERSION);
	bailAssert(script->version[1] == XS_MINOR_VERSION);
	bailAssert(script->version[2] == XS_PATCH_VERSION);
	bailAssert(script->version[3] != -1);
	
	fxLoadBinaryAtom(the, file, &atom);
	bailAssert(atom.atomType == XS_ATOM_SYMBOLS);
	script->symbolsSize = atom.atomSize - sizeof(atom);
	script->symbolsBuffer = c_malloc(script->symbolsSize);
	bailElse(fread(script->symbolsBuffer, script->symbolsSize, 1, file) == 1);	
	
	fxLoadBinaryAtom(the, file, &atom);
	bailAssert(atom.atomType == XS_ATOM_CODE);
	script->codeSize = atom.atomSize - sizeof(atom);
	script->codeBuffer = c_malloc(script->codeSize);
	bailElse(fread(script->codeBuffer, script->codeSize, 1, file) == 1);
	fclose(file);
	
	return script;
bail:
	if (file)
		fclose(file);
	if (script)
		fxDeleteScript(script);
	return C_NULL;
}

void fxLoadBinaryAtom(txMachine* the, FILE* file, Atom* atom)
{
	int error = 0;
	bailElse(fread(atom, sizeof(Atom), 1, file) == 1);	
	atom->atomSize = fix32(atom->atomSize);
	atom->atomType = fix32(atom->atomType);
bail:
	return;
}

void fxLoadBinaryModule(txMachine* the, txString path, txID moduleID)
{
	txScript* script = fxLoadBinary(the, path);
	txModuleData* data = fxLoadLibrary(the, path, script);
	fxResolveModule(the, moduleID, script, data, fxUnloadLibrary);
}

void fxLoadBinaryProgram(txMachine* the, txString path)
{
	txScript* script = fxLoadBinary(the, path);
	fxRunScript(the, script, &mxGlobal, C_NULL, C_NULL, C_NULL);
}

txModuleData* fxLoadLibrary(txMachine* the, txString path, txScript* script)
{
	txModuleData* data = C_NULL;
	if (script) {
		if (script->version[3] == 1) {
			char buffer[PATH_MAX];
			char* dot;
			void* library;
			txCallback callback;

			mc_check_stack();

			c_strcpy(buffer, path);
			dot = c_strrchr(buffer, '.');
			if (dot)
				*dot = 0;
			c_strcat(buffer, ".so");
#if mxReport || mxHostReport
			fxReport(the, "# Loading library \"%s\"\n", buffer);
#endif
			library = dlopen(buffer, RTLD_NOW);
			mxElseError(library);
			callback = (txCallback)dlsym(library, "xsHostModule");
			mxElseError(callback);
			data = c_malloc(sizeof(txModuleData) + c_strlen(buffer));
			mxElseError(data);
			data->library = library;
			data->the = the;
			c_strcpy(data->path, buffer);
			script->callback = callback;
			return data;
		}
	}
#if mxReport || mxHostReport
	data = c_malloc(sizeof(txModuleData) + c_strlen(path));
	mxElseError(data);
	data->library = C_NULL;
	data->the = the;
	c_strcpy(data->path, path);
	fxReport(the, "# Loading module \"%s\"\n", path);
#endif
	return data;
}

void fxLoadModule(txMachine* the, txID moduleID)
{
	txArchive* archive = the->archive;
	txSlot* key = fxGetKey(the, moduleID);
 	char path[PATH_MAX];
	txString dot;
	txString const* extension;
	const txLoader* loader;

	mc_check_stack();

 	c_strcpy(path, key->value.key.string);
	if (archive) {
		int c = archive->scriptCount, i;
		txScript* script = archive->scripts;
		for (i = 0; i < c; i++) {
			if (!c_strcmp(path, script->path)) {
				fxResolveModule(the, moduleID, script, fxLoadLibrary(the, key->value.key.string, C_NULL), fxUnloadLibrary);
				return;
			}
			script++;
		}
	}
	dot = c_strrchr(path, '.');
	for (extension = gxExtensions, loader = gxLoaders; *extension; extension++, loader++) {
		if (!c_strcmp(dot, *extension)) {
			(**loader)(the, path, moduleID);
			return;
		}
	}
}

void* fxMapArchive(txString base, txCallbackAt callbackAt)
{
#if XS_ARCHIVE
	int error = 0;
	txArchive* archive = C_NULL;
	txString p;
	Atom atom;
	int c, i;
	txString* symbol;
	txScript* script;
#if !mxMC
	struct stat statbuf;
#endif

#if mxMC
	archive = c_malloc(sizeof(txArchive));
	bailElse(archive);
	c_memset(archive, 0, sizeof(txArchive));
	archive->address = (void *)mc_xsa;
	archive->size = sizeof(mc_xsa);
#else
	archive->fd = open(base, O_RDONLY);
	bailElse(archive->fd >= 0);
	fstat(archive->fd, &statbuf);
	archive->size = statbuf.st_size;
	archive->address = mmap(NULL, archive->size, PROT_READ, MAP_SHARED, archive->fd, 0);
	bailElse(archive->address != MAP_FAILED);
#endif
	
	p = archive->address;
	p = fxMapAtom(p, &atom);
	bailAssert(atom.atomType == XS_ATOM_ARCHIVE);
	
	p = fxMapAtom(p, &atom);
	bailAssert(atom.atomType == XS_ATOM_VERSION);
	bailAssert(*p++ == XS_MAJOR_VERSION);
	bailAssert(*p++ == XS_MINOR_VERSION);
	bailAssert(*p++ == XS_PATCH_VERSION);
	if ((*p++) && !callbackAt) {
#if mxMC
		callbackAt = (txCallbackAt)xsHostModuleAt;
#else
		c_strcat(archive->base, ".so");
		archive->library = dlopen(archive->base, RTLD_NOW);
		if (!archive->library)
        		fprintf(stderr, "%s\n", dlerror());
		bailElse(archive->library);
		callbackAt = (txCallbackAt)dlsym(archive->library, "xsHostModuleAt");
		bailElse(callbackAt);
#endif
	}
	archive->keys = callbackAt(0);
	
	p = fxMapAtom(p, &atom);
	bailAssert(atom.atomType == XS_ATOM_SYMBOLS);
	c = archive->symbolCount = ntohs(*((unsigned short*)p));
	p += 2;
	if (c) {
		symbol = archive->symbols = c_malloc(c * sizeof(txString));
		for (i = 0; i < c; i++) {
			*symbol = p;
			p += c_strlen(p) + 1;
			symbol++;
		}
	}
	
	p = fxMapAtom(p, &atom);
	bailAssert(atom.atomType == XS_ATOM_PATHS);
	c = archive->scriptCount = ntohs(*((unsigned short*)p));
	p += 2;
	script = archive->scripts = c_malloc(c * sizeof(txScript));
	c_memset(script, 0, c * sizeof(txScript));
	for (i = 1; i <= c; i++) {
		if (callbackAt)
			script->callback = callbackAt(i);
		script->path = fxMergePath(base, p, C_NULL);
		bailElse(script->path);
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
bail:
	if (error) {
		fxUnmapArchive(archive);
		archive = C_NULL;
	}
	return archive;
#else
	return NULL;
#endif
}

void* fxMapAtom(void* p, Atom* atom)
{
	atom->atomSize = fix32(*((int32_t *)p)); 
	p += sizeof(int32_t);
	atom->atomType = fix32(*((uint32_t *)p)); 
	p += sizeof(uint32_t);
	return p;
}

void fxMarkHost(txMachine* the)
{
	the->host = C_NULL;
}

txString fxMergePath(txString base, txString name, txString path)
{
	txSize baseLength, nameLength;
	txString separator = strrchr(base, '/');
	if (separator) {
		separator++;
		baseLength = separator - base;
	}
	else
		baseLength = 0;
	nameLength = c_strlen(name);
	if (!path)
		path = c_malloc(baseLength + nameLength + 1);
	if (path) {
		if (baseLength)
			c_memcpy(path, base, baseLength);
		c_memcpy(path + baseLength, name, nameLength + 1);
	}
	return path;
}

txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags)
{
#ifdef mxParse
	txParser* parser;
	txParserJump jump;
	txScript* script = NULL;

	if ((parser = c_malloc(sizeof(txParser))) == NULL)
		return NULL;
	fxInitializeParser(parser, the, 8*1024, 1993);
	parser->firstJump = &jump;
	if (c_setjmp(jump.jmp_buf) == 0) {
		fxParserTree(parser, stream, getter, flags, NULL);
		fxParserHoist(parser);
		fxParserBind(parser);
		script = fxParserCode(parser);
	}
	fxTerminateParser(parser);
	c_free(parser);
	return script;
#else
	return C_NULL;
#endif
}

static void fxPerformJob(xsMachine *_the, void *closure)
{
	txMachine *the = _the;

	fxBeginHost(the);
	{
		fxRunPromiseJobs(the);
	}
	fxEndHost(the);
}

void fxQueuePromiseJobs(txMachine* the)
{
	mc_event_thread_call(fxPerformJob, NULL, MC_CALL_ASYNC);
}

void fxRunModule(txMachine* the, txString path)
{
	mxPushStringC(path);
	fxRequireModule(the, XS_NO_ID, the->stack);
	the->stack++;
	mc_event_main(the, NULL);
}

void fxRunLoop(txMachine* the)
{
	mc_event_main(the, NULL);
}

void fxRunProgram(txMachine* the, txString path)
{
	txString dot = strrchr(path, '.');
	if (dot) {
		if (!strcmp(dot, ".jsb") || !strcmp(dot, ".xsb"))
			fxLoadBinaryProgram(the, path);
		else
			mxError(C_EINVAL);
	}
	else
		mxError(C_EINVAL);
}

void fxSweepHost(txMachine* the)
{
	txModuleData* data = the->host;
	while (data) {
		txModuleData* next = data->next;
		if (data->library) {
#if mxReport || mxHostReport
			fxReport(data->the, "# Unloading library \"%s\"\n", data->path);
#endif
			dlclose(data->library);
		}
		else {
#if mxReport || mxHostReport
			fxReport(data->the, "# Unloading module \"%s\"\n", data->path);
#endif
		}
		c_free(data);
		data = next;
	}
}

void fxUnloadLibrary(void* it)
{
	txModuleData* data = it;
	if (data) {
		data->next = data->the->host;
		data->the->host = data;
	}
}

void fxUnmapArchive(void* it)
{
	txArchive* archive = it;
	if (archive) {
		if (archive->scripts) {
			if (archive->scripts->path != NULL)
				c_free(archive->scripts->path);
			c_free(archive->scripts);
		}
		if (archive->symbols)
			c_free(archive->symbols);
#if !mxMC
		if (archive->library)
			dlclose(archive->library);
		if (archive->address)
			munmap(archive->address, archive->size);
		if (archive->fd >= 0)
			close(archive->fd);
#endif
		c_free(archive);
	}
}

#ifdef mxParse
// #define DUMP_XSB
#ifdef DUMP_XSB
static void fxStoreBinaryAtom(txMachine *the, FILE *file, Atom *atom)
{
	Atom t;

	t.atomSize = fix32(atom->atomSize);
	t.atomType = fix32(atom->atomType);
	fwrite(&t, sizeof(t), 1, file);
}

static void fxStoreXSB(txMachine *the, txScript *script, txString path)
{
	FILE *file;
	Atom atom;
	txByte version[4];
	char xpath[PATH_MAX], *p;

	mc_check_stack();

	if (script == NULL)
		return;
	strcpy(xpath, path);
	if ((p = strrchr(xpath, '.')) == NULL)
		p = xpath + strlen(xpath);
	strcpy(p, ".xsb");
	if ((file = fopen(xpath, "w+")) == NULL)
		return;
	atom.atomType = XS_ATOM_BINARY;
	atom.atomSize = sizeof(version) + script->symbolsSize + script->codeSize + sizeof(Atom) * 4;
	fxStoreBinaryAtom(the, file, &atom);
	atom.atomType = XS_ATOM_VERSION;
	atom.atomSize = sizeof(version) + sizeof(Atom);
	version[0] = XS_MAJOR_VERSION;
	version[1] = XS_MINOR_VERSION;
	version[2] = XS_PATCH_VERSION;
	version[3] = 0;
	fxStoreBinaryAtom(the, file, &atom);
	fwrite(version, 1, sizeof(version), file);
	atom.atomType = XS_ATOM_SYMBOLS;
	atom.atomSize = script->symbolsSize + sizeof(Atom);
	fxStoreBinaryAtom(the, file, &atom);
	fwrite(script->symbolsBuffer, script->symbolsSize, 1, file);
	atom.atomType = XS_ATOM_CODE;
	atom.atomSize = script->codeSize + sizeof(Atom);
	fxStoreBinaryAtom(the, file, &atom);
	fwrite(script->codeBuffer, script->codeSize, 1, file);
	fclose(file);
}
#endif

static txScript* fxLoadText(txMachine* the, txString path, txUnsigned flags)
{
	txParser* parser = NULL;
	txParserJump jump;
	FILE* file = NULL;
	txString name = NULL;
	txScript* script = NULL;
	char buffer[PATH_MAX];
	char map[PATH_MAX];

	mc_check_stack();

	if ((parser = c_malloc(sizeof(txParser))) == NULL)
		return NULL;
	fxInitializeParser(parser, the, 8*1024, 1993);
	parser->firstJump = &jump;
	if (c_setjmp(jump.jmp_buf) == 0) {
		parser->path = fxNewParserSymbol(parser, path);
		file = fopen(path, "r");
		mxParserThrowElse(file);
		fxParserTree(parser, file, (txGetter)fgetc, flags, &name);
		fclose(file);
		file = NULL;
		if (name) {
			fxMergePath(path, name, buffer);
			mxParserThrowElse(realpath(buffer, map));
			parser->path = fxNewParserSymbol(parser, map);
			file = fopen(map, "r");
			mxParserThrowElse(file);
			fxParserSourceMap(parser, file, (txGetter)fgetc, flags, &name);
			fclose(file);
			file = NULL;
			fxMergePath(map, name, buffer);
			mxParserThrowElse(realpath(buffer, map));
			parser->path = fxNewParserSymbol(parser, map);
		}
		fxParserHoist(parser);
		fxParserBind(parser);
		script = fxParserCode(parser);
	}
	if (file)
		fclose(file);
	fxTerminateParser(parser);
	c_free(parser);
#ifdef DUMP_XSB
	fxStoreXSB(the, script, path);
#endif
	return script;
}

static void fxLoadTextModule(txMachine* the, txString path, txID moduleID)
{
	#ifdef mxDebug
		txUnsigned flags = mxDebugFlag;
	#else
		txUnsigned flags = 0;
	#endif
	txScript* script = fxLoadText(the, path, flags);
	fxResolveModule(the, moduleID, script, fxLoadLibrary(the, path, C_NULL), fxUnloadLibrary);
}

void fxIncludeScript(txParser* parser, txString string) 
{
	txSymbol* symbol = parser->path;
	FILE* file = NULL;
	char buffer[PATH_MAX];
	char include[PATH_MAX];

	mc_check_stack();

	c_strcpy(include, string);
	c_strcat(include, ".js");
	fxMergePath(symbol->string, include, buffer);
	mxParserThrowElse(realpath(buffer, include));
	parser->path = fxNewParserSymbol(parser, include);
	file = fopen(include, "r");
	mxParserThrowElse(file);
	fxParserTree(parser, file, (txGetter)fgetc, parser->flags, NULL);
	fclose(file);
	file = NULL;
	parser->path = symbol;
}

#define PARSER_ALIGN	(sizeof(txNumber))

#if mxMC
void* fxNewParserChunk(txParser* parser, txSize size)
{
	unsigned char *p;
	txParserChunk *block;
	int align;

	if ((p = mc_xs_chunk_allocator(sizeof(txParserChunk) + size)) == NULL)
		fxThrowMemoryError(parser);
	if ((align = (unsigned)p % PARSER_ALIGN) != 0) {
		align = PARSER_ALIGN - align;
		if (mc_xs_chunk_allocator(align) == NULL)	/* has to be continuous */
			fxThrowMemoryError(parser);
	}
	block = (txParserChunk *)p;
	parser->total += sizeof(txParserChunk) + size + align;
	block->next = parser->first;
	parser->first = block;
	return p + sizeof(txParserChunk) + align;
}

void fxDisposeParserChunks(txParser* parser)
{
	txParserChunk* block = parser->first;
	while (block) {
		txParserChunk* next = block->next;
		mc_xs_chunk_disposer(block);
		block = next;
	}
}

#else /* mxMC */

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

void fxDisposeParserChunks(txParser* parser)
{
	txParserChunk* block = parser->first;
	while (block) {
		txParserChunk* next = block->next;
		c_free(block);
		block = next;
	}
}

#endif /* mxMC */
#endif /* mxParse */
