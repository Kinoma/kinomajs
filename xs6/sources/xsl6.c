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
#include "xs6Common.h"
#if mxWindows
	#define mxSeparator '\\'
#else
	#include <sys/stat.h>
	#define mxSeparator '/'
#endif

#define mxAssert(_ASSERTION) { if (!(_ASSERTION)) { fprintf(stderr, "### '%s': invalid file\n", path); linker->error = C_EINVAL; c_longjmp(linker->jmp_buf, 1); } }
#define mxThrowElse(_ASSERTION) { if (!(_ASSERTION)) { linker->error = errno; c_longjmp(linker->jmp_buf, 1); } }

typedef struct sxLinker txLinker;
typedef struct sxLinkerChunk txLinkerChunk;
typedef struct sxLinkerScript txLinkerScript;
typedef struct sxSymbol txSymbol;

struct sxLinker {
	c_jmp_buf jmp_buf;
	void* dtoa;
	int error;
	txLinkerChunk* firstChunk;
	
	txLinkerScript* firstScript;
	txID scriptCount;
	txID hostsCount;
	
	txSize codeSize;
	
	txS1* pathsBuffer;
	txSize pathsSize;
	
	txSize symbolModulo;
	txSymbol** symbolTable;
	txID symbolCount;
	txID symbolIndex;
	txSymbol** symbolArray;
	
	txS1* symbolsBuffer;
	txSize symbolsSize;
};

struct sxLinkerChunk {
	txLinkerChunk* nextChunk;
};

struct sxLinkerScript {
	txLinkerScript* nextScript;
	txS1* codeBuffer;
	txSize codeSize;
	txS1* hostsBuffer;
	txSize hostsSize;
	txS1* symbolsBuffer;
	txSize symbolsSize;
	txString path;
	txSize pathSize;
	txID scriptIndex;
};

struct sxSymbol {
	txSymbol* next;
	txID ID;
	txInteger length;
	txString string;
	txSize sum;
};

static void fxBaseScript(txLinker* linker, txLinkerScript* script, txString base, txInteger baseLength);
static void fxBufferPaths(txLinker* linker);
static void fxBufferSymbols(txLinker* linker, txInteger modulo);
static void fxDefaultSymbols(txLinker* linker);
static void fxInitializeLinker(txLinker* linker);
static txBoolean fxIsCIdentifier(txLinker* linker, txString string);
static void fxLoadArchive(txLinker* linker, txString path, FILE** fileAddress);
static void fxLoadScript(txLinker* linker, txString path, txLinkerScript** link, FILE** fileAddress);
static void fxMapCode(txLinker* linker, txLinkerScript* script, txID* theIDs);
static void fxMapHosts(txLinker* linker, txLinkerScript* script, txID* theIDs);
static void fxMapScript(txLinker* linker, txLinkerScript* script);
static txID* fxMapSymbols(txLinker* linker, txS1* symbolsBuffer);
static void* fxNewLinkerChunk(txLinker* linker, txSize size);
static void* fxNewLinkerChunkClear(txLinker* linker, txSize size);
static txString fxNewLinkerString(txLinker* linker, txString buffer, txSize size);
static txSymbol* fxNewLinkerSymbol(txLinker* linker, txString theString);
static txString fxRealDirectoryPath(txLinker* linker, txString path);
static txString fxRealFilePath(txLinker* linker, txString path);
static void fxReportError(txLinker* linker, txString theFormat, ...);
static void fxTerminateLinker(txLinker* linker);
static void fxWriteExterns(txLinker* linker, FILE* file);
static void fxWriteHosts(txLinker* linker, FILE* file, txInteger modulo);
static void fxWriteIDs(txLinker* linker, FILE* file);
static void fxWriteScriptExterns(txLinkerScript* script, FILE* file);
static void fxWriteScriptHosts(txLinkerScript* script, FILE* file);
static void fxWriteSymbolString(txLinker* linker, FILE* file, txString string);

void fxBaseScript(txLinker* linker, txLinkerScript* script, txString base, txInteger baseLength)
{
	if (c_strncmp(script->path, base, baseLength))
		fxReportError(linker, "'%s': not relative to '%s'", script->path, base);
	
	script->path += baseLength;
	script->pathSize = c_strlen(script->path) + 1;
	script->scriptIndex = linker->scriptCount;
	
	linker->codeSize += script->codeSize;
	linker->pathsSize += script->pathSize;
	linker->scriptCount++;
}

void fxBufferPaths(txLinker* linker)
{
	txByte* p;
	txLinkerScript* script;

	linker->pathsBuffer = fxNewLinkerChunk(linker, linker->pathsSize);
	p = linker->pathsBuffer;
	mxEncode2(p, linker->scriptCount);
	script = linker->firstScript;
	while (script) {
		c_memcpy(p, script->path, script->pathSize);
		p += script->pathSize;
		script = script->nextScript;
	}
}

void fxBufferSymbols(txLinker* linker, txInteger modulo)
{
	if (modulo) {
		linker->symbolsBuffer = fxNewLinkerChunkClear(linker, 2);
		linker->symbolsSize = 2;
	}
	else {
		txByte* p;
		txS2 c, i;
		txSymbol** address;
		txInteger size;

		c = (txS2)(linker->symbolIndex);
		size = 2;
		address = &linker->symbolArray[0];
		for (i = 0; i < c; i++) {
			size += (*address)->length;
			address++;
		}
	
		linker->symbolsBuffer = fxNewLinkerChunk(linker, size);
		linker->symbolsSize = size;
	
		p = linker->symbolsBuffer;
		mxEncode2(p, c);
		address = &(linker->symbolArray[0]);
		for (i = 0; i < c; i++) {
			c_memcpy(p, (*address)->string, (*address)->length);
			p += (*address)->length;
			address++;
		}
	}
}

void fxDefaultSymbols(txLinker* linker)
{
	int i;
	for (i = 0; i < XS_ID_COUNT; i++) {
		fxNewLinkerSymbol(linker, gxIDStrings[i]);
	}
}

void fxInitializeLinker(txLinker* linker)
{
	c_memset(linker, 0, sizeof(txLinker));
	linker->dtoa = fxNew_dtoa();
	linker->pathsSize = 2;
	linker->symbolModulo = 1993;
	linker->symbolCount = 0x7FFF;
	linker->symbolArray = fxNewLinkerChunkClear(linker, linker->symbolCount * sizeof(txSymbol*));
	linker->symbolIndex = 0;
}

txBoolean fxIsCIdentifier(txLinker* linker, txString string)
{
	static char gxIdentifierSet[128] = {
	  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
		 0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
		 1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
		 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
		 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
		 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
		 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0 	/* 7X  pqrstuvwxyz{|}~   */
	};
	txU1 c;
	while ((c = *((txU1*)string))) {
		if ((c > 127) || (gxIdentifierSet[c] == 0))
			return 0;
		string++;
	}
	return 1;
}

void fxLoadArchive(txLinker* linker, txString path, FILE** fileAddress)
{
	FILE* aFile = NULL;
	Atom atom;
	txByte version[4];
	txSize symbolsSize;
	txS1* symbolsBuffer;
	aFile = fopen(path, "rb");
	mxThrowElse(aFile);
	*fileAddress = aFile;
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
	atom.atomSize = ntohl(atom.atomSize);
	atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_ARCHIVE);
	
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
	atom.atomSize = ntohl(atom.atomSize);
	atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_VERSION);
	mxThrowElse(fread(version, sizeof(version), 1, aFile) == 1);	
	mxAssert(version[0] == XS_MAJOR_VERSION);
	mxAssert(version[1] == XS_MINOR_VERSION);
	mxAssert(version[2] == XS_PATCH_VERSION);
	
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
	atom.atomSize = ntohl(atom.atomSize);
	atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_SYMBOLS);
	symbolsSize = atom.atomSize - sizeof(atom);
	symbolsBuffer = fxNewLinkerChunk(linker, symbolsSize);
	mxThrowElse(fread(symbolsBuffer, symbolsSize, 1, aFile) == 1);
	fxMapSymbols(linker, symbolsBuffer);
	
	fclose(aFile);
	*fileAddress = NULL;
}

void fxLoadScript(txLinker* linker, txString path, txLinkerScript** link, FILE** fileAddress)
{
	txLinkerScript* script = NULL;
	FILE* aFile = NULL;
	Atom atom;
	txByte version[4];
	script = fxNewLinkerChunkClear(linker, sizeof(txLinkerScript));
	script->path = fxNewLinkerString(linker, path, c_strlen(path));
	aFile = fopen(path, "rb");
	mxThrowElse(aFile);
	*fileAddress = aFile;
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
	atom.atomSize = ntohl(atom.atomSize);
	atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_BINARY);
	
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
	atom.atomSize = ntohl(atom.atomSize);
	atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_VERSION);
	mxThrowElse(fread(version, sizeof(version), 1, aFile) == 1);	
	mxAssert(version[0] == XS_MAJOR_VERSION);
	mxAssert(version[1] == XS_MINOR_VERSION);
	mxAssert(version[2] == XS_PATCH_VERSION);
	mxAssert(version[3] != 1);
	
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
	atom.atomSize = ntohl(atom.atomSize);
	atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_SYMBOLS);
	script->symbolsSize = atom.atomSize - sizeof(atom);
	script->symbolsBuffer = fxNewLinkerChunk(linker, script->symbolsSize);
	mxThrowElse(fread(script->symbolsBuffer, script->symbolsSize, 1, aFile) == 1);
		
	mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
    atom.atomSize = ntohl(atom.atomSize);
    atom.atomType = ntohl(atom.atomType);
	mxAssert(atom.atomType == XS_ATOM_CODE);
	script->codeSize = atom.atomSize - sizeof(atom);
	script->codeBuffer = fxNewLinkerChunk(linker, script->codeSize);
	mxThrowElse(fread(script->codeBuffer, script->codeSize, 1, aFile) == 1);
	
	if (version[3] == -1) {
		mxThrowElse(fread(&atom, sizeof(atom), 1, aFile) == 1);	
		atom.atomSize = ntohl(atom.atomSize);
		atom.atomType = ntohl(atom.atomType);
		mxAssert(atom.atomType == XS_ATOM_HOSTS);
		script->hostsSize = atom.atomSize - sizeof(atom);
		script->hostsBuffer = fxNewLinkerChunk(linker, script->hostsSize);
		mxThrowElse(fread(script->hostsBuffer, script->hostsSize, 1, aFile) == 1);	
	}
	
	fclose(aFile);
	*fileAddress = NULL;
	*link = script;
}

void fxMapCode(txLinker* linker, txLinkerScript* script, txID* theIDs)
{
	register const txS1* sizes = gxCodeSizes;
	register txByte* p = script->codeBuffer;
	register txByte* q = p + script->codeSize;
	register txS1 offset;
	txU1 code;
	txU2 index;
	txID id;
	while (p < q) {
		code = *((txU1*)p);
		if (XS_CODE_CODE_1 == code)
			*((txU1*)p) = code = XS_CODE_CODE_ARCHIVE_1;
		else if (XS_CODE_CODE_2 == code)
			*((txU1*)p) = code = XS_CODE_CODE_ARCHIVE_2;
		else if (XS_CODE_CODE_4 == code)
			*((txU1*)p) = code = XS_CODE_CODE_ARCHIVE_4;
		if (XS_CODE_STRING_1 == code)
			*((txU1*)p) = code = XS_CODE_STRING_ARCHIVE_1;
		else if (XS_CODE_STRING_2 == code)
			*((txU1*)p) = code = XS_CODE_STRING_ARCHIVE_2;
		offset = sizes[code];
		if (0 < offset)
			p += offset;
		else if (0 == offset) {
			p++;
			mxDecode2(p, id);
			if (id != XS_NO_ID) {
				id &= 0x7FFF;
				id = theIDs[id];
				p -= 2;
				mxEncode2(p, id);
			}
		}
		else if (-1 == offset) {
			p++;
			index = *((txU1*)p);
			p += 1 + index;
		}
		else if (-2 == offset) {
			p++;
			mxDecode2(p, index);
			p += index;
		}
	}
}

void fxMapHosts(txLinker* linker, txLinkerScript* script, txID* theIDs)
{
	txByte* p = script->hostsBuffer;
	if (p) {
		txID c, i, id;
		mxDecode2(p, c);
		for (i = 0; i < c; i++) {
			p++;
			mxDecode2(p, id);
			if (id != XS_NO_ID) {
				id = theIDs[id];
				p -= 2;
				mxEncode2(p, id);
			}
			p += c_strlen((char*)p) + 1;
		}
		linker->hostsCount += c;
	}
}

void fxMapScript(txLinker* linker, txLinkerScript* script)
{
	txID* symbols = fxMapSymbols(linker, script->symbolsBuffer);
	fxMapCode(linker, script, symbols);
	fxMapHosts(linker, script, symbols);
}

txID* fxMapSymbols(txLinker* linker, txS1* symbolsBuffer)
{
	txID* symbols = C_NULL;
	txByte* p = symbolsBuffer;
	txInteger c = *((txU1*)p), i;
	p++;
	c <<= 8;
	c += *((txU1*)p);
	p++;
	symbols = fxNewLinkerChunk(linker, c * sizeof(txID*));
	for (i = 0; i < c; i++) {
		txSymbol* symbol = fxNewLinkerSymbol(linker, (txString)p);
		symbols[i] = symbol->ID;
		p += c_strlen((char*)p) + 1;
	}
	return symbols;
}

void* fxNewLinkerChunk(txLinker* linker, txSize size)
{
	txLinkerChunk* block = c_malloc(sizeof(txLinkerChunk) + size);
	mxThrowElse(block);
	block->nextChunk = linker->firstChunk;
	linker->firstChunk = block;
	return block + 1;
}

void* fxNewLinkerChunkClear(txLinker* linker, txSize size)
{
	void* result = fxNewLinkerChunk(linker, size);
    c_memset(result, 0, size);
	return result;
}

txString fxNewLinkerString(txLinker* linker, txString buffer, txSize size)
{
	txString result = fxNewLinkerChunk(linker, size + 1);
	c_memcpy(result, buffer, size);
	result[size] = 0;
	return result;
}

txSymbol* fxNewLinkerSymbol(txLinker* linker, txString theString)
{
	txString aString;
	txSize aLength;
	txSize aSum;
	txSize aModulo;
	txSymbol* aSymbol;
	txID anID;
	
	aString = theString;
	aLength = 0;
	aSum = 0;
	while(*aString != 0) {
		aLength++;
		aSum = (aSum << 1) + *aString++;
	}
	aSum &= 0x7FFFFFFF;
	aModulo = aSum % linker->symbolModulo;
	aSymbol = linker->symbolTable[aModulo];
	while (aSymbol != C_NULL) {
		if (aSymbol->sum == aSum)
			if (c_strcmp(aSymbol->string, theString) == 0)
				break;
		aSymbol = aSymbol->next;
	}
	if (aSymbol == C_NULL) {
		anID = linker->symbolIndex;
		if (anID == linker->symbolCount) {
			exit(1);
		}
		aSymbol = fxNewLinkerChunk(linker, sizeof(txSymbol));
		aSymbol->next = linker->symbolTable[aModulo];
		aSymbol->ID = 0x8000 | anID;
		aSymbol->length = aLength + 1;
		aSymbol->string = fxNewLinkerString(linker, theString, aLength);
		aSymbol->sum = aSum;
		linker->symbolArray[anID] = aSymbol;
		linker->symbolTable[aModulo] = aSymbol;
		linker->symbolIndex++;
	}
	return aSymbol;
}

txString fxRealDirectoryPath(txLinker* linker, txString path)
{
#if mxWindows
	char buffer[PATH_MAX];
	DWORD attributes;
	if (_fullpath(buffer, path, PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if ((attributes != 0xFFFFFFFF) && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
  	 		strcat(buffer, "\\");
			return fxNewLinkerString(linker, buffer, strlen(buffer));
		}
	}
#else
	char buffer[PATH_MAX];
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(buffer, &a_stat) == 0) {
			if (S_ISDIR(a_stat.st_mode)) {
  	 			strcat(buffer, "/");
				return fxNewLinkerString(linker, buffer, strlen(buffer));
			}
		}
	}
#endif
	return NULL;
}

txString fxRealFilePath(txLinker* linker, txString path)
{
#if mxWindows
	char buffer[PATH_MAX];
	DWORD attributes;
	if (_fullpath(buffer, path, PATH_MAX) != NULL) {
		attributes = GetFileAttributes(buffer);
		if ((attributes != 0xFFFFFFFF) && (!(attributes & FILE_ATTRIBUTE_DIRECTORY))) {
			return fxNewLinkerString(linker, buffer, strlen(buffer));
		}
	}
#else
	char buffer[PATH_MAX];
	struct stat a_stat;
	if (realpath(path, buffer) != NULL) {
		if (stat(buffer, &a_stat) == 0) {
			if (S_ISREG(a_stat.st_mode)) {
				return fxNewLinkerString(linker, buffer, strlen(buffer));
			}
		}
	}
#endif
	return NULL;
}

void fxReportError(txLinker* linker, txString theFormat, ...)
{
	c_va_list arguments;
	fprintf(stderr, "### ");
	c_va_start(arguments, theFormat);
	vfprintf(stderr, theFormat, arguments);
	c_va_end(arguments);
	fprintf(stderr, "!\n");
	linker->error = C_EINVAL; 
	c_longjmp(linker->jmp_buf, 1); 
}

void fxTerminateLinker(txLinker* linker)
{
	txLinkerChunk* block = linker->firstChunk;
	while (block) {
		txLinkerChunk* nextChunk = block->nextChunk;
		c_free(block);
		block = nextChunk;
	}
	if (linker->dtoa)
		fxDelete_dtoa(linker->dtoa);
}

void fxWriteExterns(txLinker* linker, FILE* file)
{
	txLinkerScript* script;
	fprintf(file, "mxExport xsCallback xsHostModuleAt(xsIndex i);\n\n");
	script = linker->firstScript;
	while (script) {
		fxWriteScriptExterns(script, file);
		script = script->nextScript;
	}
	fprintf(file, "\n");
}

void fxWriteHosts(txLinker* linker, FILE* file, txInteger modulo)
{
	txLinkerScript* script;
	script = linker->firstScript;
	while (script) {
		if (script->hostsBuffer)
			fprintf(file, "static void xsHostModule%d(xsMachine* the);\n", script->scriptIndex);
		script = script->nextScript;
	}
	fprintf(file, "\n");
	script = linker->firstScript;
	while (script) {
		fxWriteScriptHosts(script, file);
		script = script->nextScript;
	}
	fprintf(file, "xsCallback xsHostModuleAt(xsIndex i)\n");
	fprintf(file, "{\n");
	fprintf(file, "\tstatic const xsCallback callbacks[%d] = {\n", 1 + linker->scriptCount);
	if (modulo)
		fprintf(file, "\t\txsHostKeys,\n");
	else
		fprintf(file, "\t\tNULL,\n");
	script = linker->firstScript;
	while (script) {
		if (script->hostsBuffer)
			fprintf(file, "\t\txsHostModule%d,\n", script->scriptIndex);
		else
			fprintf(file, "\t\tNULL,\n");
		script = script->nextScript;
	}
	fprintf(file, "\t};\n");
	fprintf(file, "\treturn callbacks[i];\n");
	fprintf(file, "}\n\n");
}

void fxWriteIDs(txLinker* linker, FILE* file)
{
	txID c, i;
	txSymbol** address;
	
	c = linker->symbolIndex;
	address = &(linker->symbolArray[0]);
	for (i = 0; i < c; i++) {
		if (fxIsCIdentifier(linker, (*address)->string))
			fprintf(file, "#define xsID_%s (the->code[%d])\n", (*address)->string, i);
		address++;
	}
}

void fxWriteScriptExterns(txLinkerScript* script, FILE* file)
{
	txByte* p = script->hostsBuffer;
	if (p) {
		txID c, i;
		mxDecode2(p, c);
		for (i = 0; i < c; i++) {
			txS1 length = *p++;
			p += 2;
			if (length < 0)
				fprintf(file, "extern void %s(void* data);\n", p);
			else
				fprintf(file, "extern void %s(xsMachine* the);\n", p);
			p += c_strlen((char*)p) + 1;
		}
	}
}

void fxWriteScriptHosts(txLinkerScript* script, FILE* file)
{
	txByte* p = script->hostsBuffer;
	if (p) {
		txID c, i, id;
		mxDecode2(p, c);
		fprintf(file, "void xsHostModule%d(xsMachine* the)\n", script->scriptIndex);
		fprintf(file, "{\n");
		fprintf(file, "\tstatic const xsHostBuilder builders[%d] = {\n", c);
		for (i = 0; i < c; i++) {
			txS1 length = *p++;
			mxDecode2(p, id);
			if (id != XS_NO_ID)
				id &= 0x7FFF;
			if (length < 0)
				fprintf(file, "\t\t{ (xsCallback)%s, -1, -1 },\n", p);
			else
				fprintf(file, "\t\t{ %s, %d, %d },\n", p, length, id);
			p += c_strlen((char*)p) + 1;
		}
		fprintf(file, "\t};\n");
		fprintf(file, "\txsResult = xsBuildHosts(%d, (xsHostBuilder*)builders);\n", c);
		fprintf(file, "}\n\n");
	}
}

void fxWriteSymbolString(txLinker* linker, FILE* file, txString string)
{
	txString p = string;
	char c;
	while ((c = *p++)) {
		if (c == '"')
			fprintf(file, "\\\"");
		else if (c == '\\')
			fprintf(file, "\\\\");
		else
			fprintf(file, "%c", c);
	}
}

int main(int argc, char* argv[]) 
{
	int argi;
	txString base = NULL;
    txString output = NULL;
	txString input = NULL;
	char name[PATH_MAX];
	char path[PATH_MAX];
	txLinker _linker;
	txLinker* linker = &_linker;
	txInteger modulo = 0;
	txLinkerScript** link;
	txLinkerScript* script;
	FILE* file = C_NULL;
	txSize size;
	txByte byte;
	
	fxInitializeLinker(linker);
	if (c_setjmp(linker->jmp_buf) == 0) {
		c_strcpy(name, "a");
		link = &(linker->firstScript);
		for (argi = 1; argi < argc; argi++) {
			if (!c_strcmp(argv[argi], "-a")) {
				argi++;
				c_strncpy(name, argv[argi], sizeof(name));
			}
			else if (!c_strcmp(argv[argi], "-b")) {
				argi++;
				if (argi >= argc)
					fxReportError(linker, "-b: no directory");
				base = fxRealDirectoryPath(linker, argv[argi]);
				if (!base)
					fxReportError(linker, "-b '%s': directory not found", argv[argi]);
			}
			else if (!c_strcmp(argv[argi], "-o")) {
				argi++;
				if (argi >= argc)
					fxReportError(linker, "-o: no directory");
				output = fxRealDirectoryPath(linker, argv[argi]);
				if (!output)
					fxReportError(linker, "-o '%s': directory not found", argv[argi]);
			}
			else if (!c_strcmp(argv[argi], "-r")) {
				argi++;
				if (argi >= argc)
					fxReportError(linker, "-r: no modulo");
				modulo = (txInteger)fxStringToNumber(linker->dtoa, argv[argi], 1);
				if (modulo <= 0)
					fxReportError(linker, "-r: invalid modulo");
				linker->symbolModulo = modulo;
			}
			else {
				input = fxRealFilePath(linker, argv[argi]);
				if (!input)
					fxReportError(linker, "'%s': file not found", argv[argi]);
				fxLoadScript(linker, input, link, &file);
				link = &((*link)->nextScript);
			}
		}
		if (!output)
			output = fxRealDirectoryPath(linker, ".");
		if (!base)
			base = output;
		linker->symbolTable = fxNewLinkerChunkClear(linker, linker->symbolModulo * sizeof(txSymbol*));
		
		c_strcpy(path, output);
		c_strcat(path, name);
		c_strcat(path, ".xsa");
		if (!modulo && fxRealFilePath(linker, path))
			fxLoadArchive(linker, path, &file);
			
		size = c_strlen(base);
		script = linker->firstScript;
		while (script) {
			fxBaseScript(linker, script, base, size);
			script = script->nextScript;
		}
		fxBufferPaths(linker);
	
		fxDefaultSymbols(linker);
		script = linker->firstScript;
		while (script) {
			fxMapScript(linker, script);
			script = script->nextScript;
		}
		fxBufferSymbols(linker, modulo);
		
		file = fopen(path, "wb");
		mxThrowElse(file);
		size = 8 + 8 + 4 + 8 + linker->symbolsSize + 8 + linker->pathsSize + (8 * linker->scriptCount) + linker->codeSize;
		size = htonl(size);
		mxThrowElse(fwrite(&size, 4, 1, file) == 1);
		mxThrowElse(fwrite("XS6A", 4, 1, file) == 1);

		size = 8 + 4;
		size = htonl(size);
		mxThrowElse(fwrite(&size, 4, 1, file) == 1);
		mxThrowElse(fwrite("VERS", 4, 1, file) == 1);
		byte = XS_MAJOR_VERSION;
		mxThrowElse(fwrite(&byte, 1, 1, file) == 1);
		byte = XS_MINOR_VERSION;
		mxThrowElse(fwrite(&byte, 1, 1, file) == 1);
		byte = XS_PATCH_VERSION;
		mxThrowElse(fwrite(&byte, 1, 1, file) == 1);
		byte = (linker->hostsCount) ? 1 : 0;
		mxThrowElse(fwrite(&byte, 1, 1, file) == 1);

		size = 8 + linker->symbolsSize;
		size = htonl(size);
		mxThrowElse(fwrite(&size, 4, 1, file) == 1);
		mxThrowElse(fwrite("SYMB", 4, 1, file) == 1);
		mxThrowElse(fwrite(linker->symbolsBuffer, linker->symbolsSize, 1, file) == 1);

		size = 8 + linker->pathsSize;
		size = htonl(size);
		mxThrowElse(fwrite(&size, 4, 1, file) == 1);
		mxThrowElse(fwrite("PATH", 4, 1, file) == 1);
		mxThrowElse(fwrite(linker->pathsBuffer, linker->pathsSize, 1, file) == 1);

		script = linker->firstScript;
		while (script) {
			size = 8 + script->codeSize;
			size = htonl(size);
			mxThrowElse(fwrite(&size, 4, 1, file) == 1);
			mxThrowElse(fwrite("CODE", 4, 1, file) == 1);
			mxThrowElse(fwrite(script->codeBuffer, script->codeSize, 1, file) == 1);
			script = script->nextScript;
		}
		fclose(file);
		file = NULL;
		
		if (linker->hostsCount || modulo) {
			c_strcpy(path, base);
			c_strcat(path, name);
			c_strcat(path, ".xs.h");
			file = fopen(path, "w");
			mxThrowElse(file);
			fprintf(file, "/* XS GENERATED FILE; DO NOT EDIT! */\n\n");
			fprintf(file, "#include <xs.h>\n\n");
			fxWriteExterns(linker, file);
			fxWriteIDs(linker, file);
			fclose(file);
			file = NULL;
		
			c_strcpy(path, base);
			c_strcat(path, name);
			c_strcat(path, ".xs.c");
			file = fopen(path, "w");
			mxThrowElse(file);
			fprintf(file, "/* XS GENERATED FILE; DO NOT EDIT! */\n\n");
			if (modulo)
				fprintf(file, "#include <xs6All.h>\n\n");
			fprintf(file, "#include \"%s.xs.h\"\n\n", name);
			if (modulo) {
				txS2 c, i;
				txSymbol** address;
				c = (txS2)(linker->symbolIndex);
				address = &(linker->symbolArray[0]);
				for (i = 0; i < c; i++) {
					txSymbol* symbol = *address;
					fprintf(file, "static const txSlot xs_key_%d = {\n", symbol->ID & 0x7FFF);
					if (symbol->next)
						fprintf(file, "\t(txSlot *)&xs_key_%d,\n", symbol->next->ID & 0x7FFF);
					else
						fprintf(file, "\tNULL,\n");
					if (i < XS_SYMBOL_ID_COUNT) {
						fprintf(file, "\tXS_NO_ID, XS_DONT_ENUM_FLAG | XS_MARK_FLAG, XS_STRING_X_KIND,\n");
						fprintf(file, "\t{ .string = \"");
						fxWriteSymbolString(linker, file, symbol->string);
						fprintf(file, "\" }\n");
					}
					else {
						fprintf(file, "\t%d, XS_DONT_ENUM_FLAG | XS_MARK_FLAG, XS_KEY_X_KIND,\n", (int)symbol->ID);
						fprintf(file, "\t{ .key = {\"");
						fxWriteSymbolString(linker, file, symbol->string);
						fprintf(file, "\", 0x%X} }\n", (int)symbol->sum);
					}
					fprintf(file, "};\n");
					address++;
				}
				fprintf(file, "static const txSlot *xs_keys[%d] = {\n", c);
				address = &(linker->symbolArray[0]);
				for (i = 0; i < c; i++) {
					txSymbol* symbol = *address;
					fprintf(file, "\t&xs_key_%d,\n", symbol->ID & 0x7FFF);
					address++;
				}
				fprintf(file, "};\n\n");
			
				fprintf(file, "static void xsHostKeys(txMachine* the)\n");
				fprintf(file, "{\n");
				fprintf(file, "\ttxSlot* callback = &mxIDs;\n");
				fprintf(file, "\ttxInteger i;\n");
				fprintf(file, "\tmxCheck(the, the->nameModulo == %d);\n", modulo);
				fprintf(file, "\tcallback->value.callback.address = NULL;\n", c);
				fprintf(file, "\tcallback->value.callback.IDs = (txID*)fxNewChunk(the, %d * sizeof(txID));\n", c);
				fprintf(file, "\tcallback->kind = XS_CALLBACK_KIND;\n");
				fprintf(file, "\tfor (i = 0; i < XS_SYMBOL_ID_COUNT; i++) {\n");
				fprintf(file, "\t\ttxSlot *description = (txSlot *)xs_keys[i];\n");
				fprintf(file, "\t\tthe->keyArray[i] = description;\n");
				fprintf(file, "\t\tmxID(i) = 0x8000 | i;\n");
				fprintf(file, "\t}\n");
				fprintf(file, "\tfor (; i < %d; i++) {\n",  c);
				fprintf(file, "\t\ttxSlot *key = (txSlot *)xs_keys[i];\n");
				fprintf(file, "\t\tthe->keyArray[i] = key;\n");
				fprintf(file, "\t\tthe->nameTable[key->value.key.sum %% %d] = key;\n", modulo);
				fprintf(file, "\t\tmxID(i) = key->ID;\n");
				fprintf(file, "\t}\n");
				fprintf(file, "\tthe->keyIndex = %d;\n", c);
				fprintf(file, "\tthe->keyOffset = %d;\n", c);
				fprintf(file, "}\n\n");
			}
			fxWriteHosts(linker, file, modulo);
			fclose(file);
			file = NULL;
		}
		
	}
	else {
		if (linker->error != C_EINVAL) {
		#if mxWindows
			char buffer[2048];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, linker->error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer), NULL);
			fprintf(stderr, "### %s\n", buffer);
		#else
			fprintf(stderr, "### %s\n", strerror(linker->error));
		#endif
		}
	}
	if (file)
		fclose(file);
	fxTerminateLinker(linker);
	return linker->error;
}




