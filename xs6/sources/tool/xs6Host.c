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
#include "xs6All.h"

#ifdef mxParse
	#include "xs6Script.h"
	#if mxWindows
		#define mxParserThrowElse(_ASSERTION) { if (!(_ASSERTION)) { parser->error = GetLastError(); c_longjmp(parser->firstJump->jmp_buf, 1); } }
	#else
		#define mxParserThrowElse(_ASSERTION) { if (!(_ASSERTION)) { parser->error = errno; c_longjmp(parser->firstJump->jmp_buf, 1); } }
	#endif
#endif

#if mxWindows
	#define mxSeparator '\\'
	#define bailAssert(_ASSERTION) { if (!(_ASSERTION)) { error = ERROR_INVALID_DATA; goto bail; } }
	#define bailElse(_ASSERTION) { if (!(_ASSERTION)) { error = GetLastError(); goto bail; } }
#else
	#if mxMacOSX
		#include <CoreServices/CoreServices.h>
	#endif
	#include <dlfcn.h>
	#include <fcntl.h>
	#include <pwd.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <unistd.h>
	
	#define mxSeparator '/'
	#define bailAssert(_ASSERTION) { if (!(_ASSERTION)) { error = EINVAL; goto bail; } }
	#define bailElse(_ASSERTION) { if (!(_ASSERTION)) { error = errno; goto bail; } }
#endif

#ifndef mxReport
#define mxReport 0
#endif

typedef struct {
#if mxWindows
	HANDLE file;
	HANDLE mapping;
#else
	int fd;
#endif
	int size;
	void* address;
	void* library;
	txCallback keys;
	txID symbolCount;
	txString* symbols;
	txID scriptCount;
	txScript* scripts;
	int baseLength;
	char base[PATH_MAX];
} txArchive;

typedef struct sxJob txJob;
typedef void (*txJobCallback)(txJob*);
struct sxJob {
	txJob* next;
	txMachine* the;
	txNumber when;
	txJobCallback callback;
	txSlot function;
	txSlot argument;
};

typedef void (*txLoader)(txMachine*, txString, txID);

typedef struct sxModuleData txModuleData;
struct sxModuleData {
	void* library;
	txModuleData* next;
	txMachine* the;
	char path[1];
};

mxExport void* fxMapArchive(txString path, txCallbackAt callbackAt);
mxExport void fxRunLoop(txMachine* the);
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
#ifdef mxParse
static txScript* fxLoadText(txMachine* the, txString path, txUnsigned flags);
static void fxLoadTextModule(txMachine* the, txString path, txID moduleID);
static void fxLoadTextProgram(txMachine* the, txString path);
#endif
static txString fxMergePath(txString base, txString name, txString path);
static void fxQueuePromiseJobsCallback(txJob* job);
static void fxUnloadLibrary(void* it);
static void fx_setTimeout(txMachine* the);
static void fx_setTimeoutCallback(txJob* job);

static txString gxExtensions[] = { 
	".jsb", 
	".xsb", 
#ifdef mxParse
	".js", 
#endif
	C_NULL,
};
static txLoader gxLoaders[] = {
	fxLoadBinaryModule,
	fxLoadBinaryModule,
#ifdef mxParse
	fxLoadTextModule,
#endif
	C_NULL,
};

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
	fxNewHostFunctionGlobal(the, fx_setTimeout, 1, fxID(the, "setTimeout"), XS_DONT_ENUM_FLAG);
	the->stack++;
}

void fxBuildKeys(txMachine* the)
{
	txArchive* archive = the->archive;
	if (archive && archive->keys)
		(*archive->keys)(the);
	else {
	#ifdef mxParse
		txSlot* code = &mxIDs;
		txID c, i;
		if (archive) {
			c = archive->symbolCount;
			code->value.code = (txByte *)fxNewChunk(the, c * sizeof(txID));
			code->kind = XS_CODE_KIND;	
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
			code->value.code = (txByte *)fxNewChunk(the, XS_ID_COUNT * sizeof(txID));
			code->kind = XS_CODE_KIND;	
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
	#else
		fxCheck(the, __FILE__, __LINE__);
	#endif
	}
}

#ifdef mxParse
void fxDisposeParserChunks(txParser* parser)
{
	txParserChunk* block = parser->first;
	while (block) {
		txParserChunk* next = block->next;
		c_free(block);
		block = next;
	}
}
#endif

txBoolean fxFindArchive(txMachine* the, txString path)
{
	txArchive* archive = the->archive;
	if (archive) {
		if (!c_strncmp(path, archive->base, archive->baseLength)) {
			txInteger c = archive->scriptCount;
			txScript* script = archive->scripts;
			path += archive->baseLength;
			while (c > 0) {
				if (!c_strcmp(path, script->path))
					return 1;
				c--;
				script++;
			}
		}
	}
	return 0;
}

txBoolean fxFindFile(txMachine* the, txString path)
{
	char real[PATH_MAX];
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
	txBoolean absolute, relative, search;
	txSlot *key, *iterator, *result;
	txString dot, slash;
	txID id;
	
	fxToStringBuffer(the, slot, name, sizeof(name));
	if ((!c_strncmp(name, "./", 2)) || (!c_strncmp(name, "../", 3))) {
		absolute = 0;
		relative = (moduleID == XS_NO_ID) ? 0 : 1;
		search = 0;
	}
	else if ((!c_strncmp(name, "/", 1))) {
		absolute = 1;
		relative = 0;
		search = 0;
	}
	else {
		absolute = 0;
		relative = (moduleID == XS_NO_ID) ? 0 : 1;
		search = 1;
	}
	slash = c_strrchr(name, '/');
	if (!slash)
		slash = name;
	dot = c_strrchr(slash, '.');
	if (!dot)
		dot = name + c_strlen(name);
	
	if (absolute) {
		if (fxFindURI(the, "", name, dot, &id))
			return id;
	}
	if (relative) {
		key = fxGetKey(the, moduleID);
  		c_strcpy(base, key->value.key.string);
		if (fxFindURI(the, base, name, dot, &id))
			return id;
	}
	if (search) {
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
	if (*dot) {
		fxMergePath(base, name, path);
		if (fxFindArchive(the, path) || fxFindFile(the, path)) {
			key = fxNewNameC(the, path);
			*id = key->ID;
			return 1; 
		}
	}
	else {
		txString* extension;
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

void fxFreeChunks(txMachine* the, void* theChunks)
{
	c_free(theChunks);
}

void fxFreeSlots(txMachine* the, void* theSlots)
{
	c_free(theSlots);
}

#ifdef mxParse
void fxIncludeScript(txParser* parser, txString string) 
{
	txSymbol* symbol = parser->path;
	char buffer[PATH_MAX];
	char include[PATH_MAX];
	FILE* file = NULL;
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
#endif

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
	atom->atomSize = ntohl(atom->atomSize);
	atom->atomType = ntohl(atom->atomType);
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
			c_strcpy(buffer, path);
			dot = c_strrchr(buffer, '.');
			if (dot)
				*dot = 0;
			#if mxWindows
				c_strcat(buffer, ".dll");
				#if mxReport
					fxReport(the, "# Loading library \"%s\"\n", buffer);
				#endif
				library = LoadLibrary(buffer);
				mxElseError(library);
				callback = (txCallback)GetProcAddress(library, "xsHostModule");
				mxElseError(callback);
			#else
				c_strcat(buffer, ".so");
				#if mxReport
					fxReport(the, "# Loading library \"%s\"\n", buffer);
				#endif
				library = dlopen(buffer, RTLD_NOW);
				mxElseError(library);
				callback = (txCallback)dlsym(library, "xsHostModule");
				mxElseError(callback);
			#endif
			data = c_malloc(sizeof(txModuleData) + c_strlen(buffer));
			mxElseError(data);
			data->library = library;
			data->the = the;
			c_strcpy(data->path, buffer);
			script->callback = callback;
			return data;
		}
	}
#if mxReport
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
 	char buffer[PATH_MAX];
	txString path = buffer;
	txString dot;
	txString* extension;
	txLoader* loader;
 	c_strcpy(path, key->value.key.string);
	if (archive) {
		if (!c_strncmp(path, archive->base, archive->baseLength)) {
			txInteger c = archive->scriptCount, i;
			txScript* script = archive->scripts;
			path += archive->baseLength;
			for (i = 0; i < c; i++) {
				if (!c_strcmp(path, script->path)) {
					fxResolveModule(the, moduleID, script, fxLoadLibrary(the, key->value.key.string, C_NULL), fxUnloadLibrary);
					return;
				}
				script++;
			}
		}
	}
	dot = c_strrchr(path, '.');
	for (extension = gxExtensions, loader = gxLoaders; *extension; extension++, loader++) {
		if (!c_strcmp(dot, *extension)) {
			(**loader)(the, buffer, moduleID);
			return;
		}
	}
}

#ifdef mxParse
txScript* fxLoadText(txMachine* the, txString path, txUnsigned flags)
{
	txParser _parser;
	txParser* parser = &_parser;
	txParserJump jump;
	FILE* file = NULL;
	txString name = NULL;
	char buffer[PATH_MAX];
	char map[PATH_MAX];
	txScript* script = NULL;
	fxInitializeParser(parser, the, 32*1024, 1993);
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
	return script;
}

void fxLoadTextModule(txMachine* the, txString path, txID moduleID)
{
	#ifdef mxDebug
		txUnsigned flags = mxDebugFlag;
	#else
		txUnsigned flags = 0;
	#endif
	txScript* script = fxLoadText(the, path, flags);
	fxResolveModule(the, moduleID, script, fxLoadLibrary(the, path, C_NULL), fxUnloadLibrary);
}

void fxLoadTextProgram(txMachine* the, txString path)
{
	#ifdef mxDebug
		txUnsigned flags = mxProgramFlag | mxDebugFlag;
	#else
		txUnsigned flags = mxProgramFlag;
	#endif
	txScript* script = fxLoadText(the, path, flags);
	fxRunScript(the, script, &mxGlobal, C_NULL, C_NULL, C_NULL);
}
#endif

void* fxMapArchive(txString base, txCallbackAt callbackAt)
{
	int error = 0;
	txArchive* archive = C_NULL;
#if mxWindows
#else
	struct stat statbuf;
#endif
	txString p;
	Atom atom;
	int c, i;
	txString* symbol;
	txScript* script;

	archive = c_malloc(sizeof(txArchive));
	bailElse(archive);
	c_memset(archive, 0, sizeof(txArchive));
	c_strcpy(archive->base, base);
#if mxWindows
	archive->file = CreateFile(base, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	bailElse(archive->file != INVALID_HANDLE_VALUE);
	archive->size = GetFileSize(archive->file, &archive->size);
	bailElse(archive->size != INVALID_FILE_SIZE);
	archive->mapping = CreateFileMapping(archive->file, NULL, PAGE_READONLY, 0, (SIZE_T)archive->size, NULL);
	bailElse(archive->mapping != INVALID_HANDLE_VALUE);
	archive->address = MapViewOfFile(archive->mapping, FILE_MAP_READ, 0, 0, (SIZE_T)archive->size);
	bailElse(archive->address != NULL);
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
		char* dot;
		dot = c_strrchr(archive->base, '.');
		if (dot)
			*dot = 0;
		#if mxWindows
			c_strcat(archive->base, ".dll");
			archive->library = LoadLibrary(archive->base);
			bailElse(archive->library);
			callbackAt = (txCallbackAt)GetProcAddress(archive->library, "xsHostModuleAt");
			bailElse(callbackAt);
		#else
			c_strcat(archive->base, ".so");
			archive->library = dlopen(archive->base, RTLD_NOW);
			if (!archive->library)
        		fprintf(stderr, "%s\n", dlerror());
			bailElse(archive->library);
			callbackAt = (txCallbackAt)dlsym(archive->library, "xsHostModuleAt");
			bailElse(callbackAt);
		#endif
		archive->keys = callbackAt(0);
	}
	
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
		script->path = p;
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
	
	p = c_strrchr(archive->base, mxSeparator);
	if (p) {
		p++;
		*p = 0;
		archive->baseLength = p - archive->base;
	}
bail:
	if (error) {
		fxUnmapArchive(archive);
		archive = C_NULL;
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
	the->host = C_NULL;
}

txString fxMergePath(txString base, txString name, txString path)
{
	txSize baseLength, nameLength;
	txString separator = strrchr(base, mxSeparator);
	if (separator) {
		separator++;
		baseLength = separator - base;
	}
	else
		baseLength = 0;
	nameLength = c_strlen(name);
	if (baseLength)
		c_memcpy(path, base, baseLength);
#if mxWindows
	{
		char c ;
		separator = path + baseLength;
		while ((c = *name)) {
			if (c == '/')
				*separator++ = mxSeparator;
			else
				*separator++ = c;
			name++;
		}
		*separator = 0;
	}
#else		
	c_memcpy(path + baseLength, name, nameLength + 1);
#endif
	return path;
}

#ifdef mxParse
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
#endif

txScript* fxParseScript(txMachine* the, void* stream, txGetter getter, txUnsigned flags)
{
#ifdef mxParse
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
#else
	return C_NULL;
#endif
}

void fxQueuePromiseJobs(txMachine* the)
{
	c_timeval tv;
	txJob* job;
	txJob** address = (txJob**)&(the->context);
	while ((job = *address))
		address = &(job->next);
	job = *address = malloc(sizeof(txJob));
    c_memset(job, 0, sizeof(txJob));
    job->the = the;
    job->callback = fxQueuePromiseJobsCallback;
	c_gettimeofday(&tv, NULL);
	job->when = ((txNumber)(tv.tv_sec) * 1000.0) + ((txNumber)(tv.tv_usec) / 1000.0);
}

void fxQueuePromiseJobsCallback(txJob* job) 
{
	txMachine* the = job->the;
	fxBeginHost(the);
	{
		fxRunPromiseJobs(the);
	}
	fxEndHost(the);
}

void fxRunLoop(txMachine* the)
{
	c_timeval tv;
	txNumber when;
	txJob* job;
	txJob** address;
	for (;;) {
		c_gettimeofday(&tv, NULL);
		when = ((txNumber)(tv.tv_sec) * 1000.0) + ((txNumber)(tv.tv_usec) / 1000.0);
		address = (txJob**)&(the->context);;
		if (!*address)
			break;
		while ((job = *address)) {
			if (job->when <= when) {
				(*job->callback)(job);	
				*address = job->next;
				c_free(job);
			}
			else
				address = &(job->next);
		}
	}
}

void fxRunModule(txMachine* the, txString path)
{
	mxPushStringC(path);
	fxRequireModule(the, XS_NO_ID, the->stack);
	the->stack++;
	fxRunLoop(the);
}

void fxRunProgram(txMachine* the, txString path)
{
	txString dot = strrchr(path, '.');
	if (dot) {
		if (!strcmp(dot, ".jsb") || !strcmp(dot, ".xsb"))
			fxLoadBinaryProgram(the, path);
	#ifdef mxParse
		else if (!strcmp(dot, ".js"))
			fxLoadTextProgram(the, path);
	#endif
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
			#if mxReport
				fxReport(data->the, "# Unloading library \"%s\"\n", data->path);
			#endif
			#if mxWindows
				FreeLibrary(data->library);
			#else
				dlclose(data->library);
			#endif
		}
		else {
			#if mxReport
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
		if (archive->scripts)
			free(archive->scripts);
		if (archive->symbols)
			free(archive->symbols);
		#if mxWindows
			if (archive->library)
				FreeLibrary(archive->library);
			if (archive->address)
				UnmapViewOfFile(archive->address);
			if (archive->mapping)
				CloseHandle(archive->mapping);
			if (archive->file)
				CloseHandle(archive->file);
		#else
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

void fx_setTimeout(txMachine* the)
{
	c_timeval tv;
	txJob* job;
	txJob** address = (txJob**)&(the->context);
	while ((job = *address))
		address = &(job->next);
	job = *address = malloc(sizeof(txJob));
	c_memset(job, 0, sizeof(txJob));
	job->the = the;
	job->callback = fx_setTimeoutCallback;
	c_gettimeofday(&tv, NULL);
	job->when = ((txNumber)(tv.tv_sec) * 1000.0) + ((txNumber)(tv.tv_usec) / 1000.0) + fxToNumber(the, mxArgv(1));
	job->function.kind = mxArgv(0)->kind;
	job->function.value = mxArgv(0)->value;
	fxRemember(the, &(job->function));
	if (mxArgc > 2) {
		job->argument.kind = mxArgv(2)->kind;
		job->argument.value = mxArgv(2)->value;
	}
	fxRemember(the, &(job->argument));
}

void fx_setTimeoutCallback(txJob* job)
{
	txMachine* the = job->the;
	fxBeginHost(the);
	{
		mxPush(job->argument);
		/* ARGC */
		mxPushInteger(1);
		/* THIS */
		mxPushUndefined();
		/* FUNCTION */
		mxPush(job->function);
		fxCall(the);
		the->stack++;
	}
	fxEndHost(the);
}
