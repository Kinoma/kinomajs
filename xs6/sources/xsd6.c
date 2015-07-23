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
#include "xs6Common.h"

#define bailAssert(_ASSERTION) { if (!(_ASSERTION)) { fprintf(stderr, "%s\n", #_ASSERTION); error = EINVAL; goto bail; } }
#define bailElse(_ASSERTION) { if (!(_ASSERTION)) { fprintf(stderr, "%s\n", #_ASSERTION); error = errno; goto bail; } }

typedef struct {
	txS1* name;
	txS4 debug;
	txS4 property;
	txS4 variable;
} txUsage;

static void fxDumpCode(txS1* path, txS1** symbols, txUsage* usages, txS1** hosts, txS1* codeBuffer, size_t codeSize) 
{
	txS1* p = codeBuffer;
	txS1* q = codeBuffer + codeSize;
	txS1 s1; txS2 s2; txS4 s4; 
	txU1 u1; txU2 u2; 
	double number;
	while (p < q) {
		u1 = *((txU1*)p);
		fprintf(stderr, "%8.8X: %d %s ", p - codeBuffer, u1, gxCodeNames[u1]);
		p++;
		switch (u1) {
		case XS_CODE_BRANCH_1:
		case XS_CODE_BRANCH_ELSE_1:
		case XS_CODE_BRANCH_IF_1:
		case XS_CODE_BRANCH_STATUS_1:
		case XS_CODE_CATCH_1:
		case XS_CODE_CODE_1:
		case XS_CODE_CODE_ARCHIVE_1:
			s1 = *p++;
			fprintf(stderr, "%8.8X", p + s1 - codeBuffer);
			break;
		case XS_CODE_BRANCH_2:
		case XS_CODE_BRANCH_ELSE_2:
		case XS_CODE_BRANCH_IF_2:
		case XS_CODE_BRANCH_STATUS_2:
		case XS_CODE_CATCH_2:
		case XS_CODE_CODE_2:
		case XS_CODE_CODE_ARCHIVE_2:
			mxDecode2(p, s2);
			fprintf(stderr, "%8.8X", p + s2 - codeBuffer);
			break;
		case XS_CODE_BRANCH_4:
		case XS_CODE_BRANCH_ELSE_4:
		case XS_CODE_BRANCH_IF_4:
		case XS_CODE_BRANCH_STATUS_4:
		case XS_CODE_CATCH_4:
		case XS_CODE_CODE_4:
		case XS_CODE_CODE_ARCHIVE_4:
			mxDecode4(p, s4);
			fprintf(stderr, "%8.8X", p + s4 - codeBuffer);
			break;
			
		case XS_CODE_ARROW:
		case XS_CODE_FILE:
		case XS_CODE_FUNCTION:
		case XS_CODE_GENERATOR:
			mxDecode2(p, s2);
			if (s2 == XS_NO_ID) 
				fprintf(stderr, "?");
			else if (symbols) {
				s2 &= 0x7FFF;
				usages[s2].debug++;
				fprintf(stderr, "%s", symbols[s2]);
			}
			else
				fprintf(stderr, "%d", s2);
			break;
			
		case XS_CODE_CONST_GLOBAL:
		case XS_CODE_DELETE_EVAL:
		case XS_CODE_DELETE_GLOBAL:
		case XS_CODE_DELETE_PROPERTY:
		case XS_CODE_DELETE_SUPER:
		case XS_CODE_GET_EVAL:
		case XS_CODE_GET_GLOBAL:
		case XS_CODE_GET_PROPERTY:
		case XS_CODE_GET_SUPER:
		case XS_CODE_NEW_EVAL:
		case XS_CODE_NEW_GLOBAL:
		case XS_CODE_SET_EVAL:
		case XS_CODE_SET_GLOBAL:
		case XS_CODE_SET_PROPERTY:
		case XS_CODE_SET_SUPER:
		case XS_CODE_SYMBOL:
			mxDecode2(p, s2);
			if (s2 == XS_NO_ID) 
				fprintf(stderr, "?");
			else if (symbols) {
				s2 &= 0x7FFF;
				usages[s2].property++;
				fprintf(stderr, "%s", symbols[s2]);
			}
			else
				fprintf(stderr, "%d", s2);
			break;
			
		case XS_CODE_NEW_CLOSURE:
		case XS_CODE_NEW_LOCAL:
			mxDecode2(p, s2);
			if (s2 == XS_NO_ID) 
				fprintf(stderr, "?");
			else if (symbols) {
				s2 &= 0x7FFF;
				usages[s2].variable++;
				fprintf(stderr, "%s", symbols[s2]);
			}
			else
				fprintf(stderr, "%d", s2);
			break;

		case XS_CODE_NEW_PROPERTY:
			u1 = *((txU1*)p++);
			if (u1 & XS_DONT_DELETE_FLAG)
				fprintf(stderr, "C");
			else
				fprintf(stderr, "c");
			if (u1 & XS_DONT_ENUM_FLAG)
				fprintf(stderr, "E");
			else
				fprintf(stderr, "e");
			if (u1 & XS_DONT_SET_FLAG)
				fprintf(stderr, "W");
			else
				fprintf(stderr, "w");
			if (u1 & XS_STATIC_FLAG)
				fprintf(stderr, "static");
			if (u1 & XS_GETTER_FLAG)
				fprintf(stderr, " getter");
			if (u1 & XS_SETTER_FLAG)
				fprintf(stderr, " setter");
			break;
			
		case XS_CODE_ARGUMENT:
		case XS_CODE_ARGUMENTS:
		case XS_CODE_ARGUMENTS_SLOPPY:
		case XS_CODE_ARGUMENTS_STRICT:
		case XS_CODE_BEGIN_SLOPPY:
		case XS_CODE_BEGIN_STRICT:
		case XS_CODE_BEGIN_STRICT_BASE:
		case XS_CODE_BEGIN_STRICT_DERIVED:
			u1 = *((txU1*)p++);
			fprintf(stderr, "%d", u1);
			break;
			
		case XS_CODE_LINE:
			mxDecode2(p, u2);
			fprintf(stderr, "%d", u2);
			break;
	
		case XS_CODE_RESERVE_1:
		case XS_CODE_RETRIEVE_1:
		case XS_CODE_UNWIND_1:
			u1 = *((txU1*)p++);
			fprintf(stderr, "#%d", u1);
			break;
			
		case XS_CODE_RESERVE_2:
		case XS_CODE_RETRIEVE_2:
		case XS_CODE_UNWIND_2:
			mxDecode2(p, u2);
			fprintf(stderr, "#%d", u2);
			break;
			
		case XS_CODE_CONST_CLOSURE_1:
		case XS_CODE_CONST_LOCAL_1:
		case XS_CODE_DELETE_CLOSURE_1:
		case XS_CODE_DELETE_LOCAL_1:
		case XS_CODE_GET_CLOSURE_1:
		case XS_CODE_GET_LOCAL_1:
		case XS_CODE_PULL_CLOSURE_1:
		case XS_CODE_PULL_LOCAL_1:
		case XS_CODE_SET_CLOSURE_1:
		case XS_CODE_SET_LOCAL_1:
		case XS_CODE_STORE_1:
			u1 = *((txU1*)p++);
			fprintf(stderr, "[%d]", u1 - 2);
			break;
		case XS_CODE_CONST_CLOSURE_2:
		case XS_CODE_CONST_LOCAL_2:
		case XS_CODE_DELETE_CLOSURE_2:
		case XS_CODE_DELETE_LOCAL_2:
		case XS_CODE_GET_CLOSURE_2:
		case XS_CODE_GET_LOCAL_2:
		case XS_CODE_PULL_CLOSURE_2:
		case XS_CODE_PULL_LOCAL_2:
		case XS_CODE_SET_CLOSURE_2:
		case XS_CODE_SET_LOCAL_2:
		case XS_CODE_STORE_2:
			mxDecode2(p, u2);
			fprintf(stderr, "[%d]", u2 - 2);
			break;
		
		case XS_CODE_INTEGER_1: 
			s1 = *p++;
			fprintf(stderr, "%d", s1);
			break;
		case XS_CODE_INTEGER_2: 
			mxDecode2(p, s2);
			fprintf(stderr, "%d", s2);
			break;
		case XS_CODE_INTEGER_4: 
			mxDecode4(p, s4);
			fprintf(stderr, "%ld", s4);
			break;
		case XS_CODE_NUMBER:
			mxDecode8(p, number);
			fprintf(stderr, "%lf", number);
			break;
		case XS_CODE_STRING_1:
		case XS_CODE_STRING_ARCHIVE_1:
			u1 = *((txU1*)p++);
			fprintf(stderr, "\"%s\"", p);
			p += u1;
			break;
		case XS_CODE_STRING_2:
		case XS_CODE_STRING_ARCHIVE_2:
			mxDecode2(p, u2);
			fprintf(stderr, "\"%s\"", p);
			p += u2;
			break;
		
		case XS_CODE_HOST:
			mxDecode2(p, u2);
			if (hosts)
				fprintf(stderr, "%s", hosts[u2]);
			else
				fprintf(stderr, "[%d]", u2);
			break;
		}
		fprintf(stderr, "\n");
	}
}

int compareUsages(const void* a, const void* b) {
	return strcmp((char*)((txUsage*)a)->name, (char*)((txUsage*)b)->name);
}

int main(int argc, char* argv[]) 
{
	int error = 0;
	typedef struct {
		long atomSize;
		unsigned long atomType;
	} Atom;
	FILE* aFile = NULL;
	size_t aSize;
	txS1* aBuffer = NULL;
	Atom atom;
	txByte isArchive;
	txByte hasC;
	txS1* codeBuffer;
	size_t codeSize;
	txS1* p;
	int c, i;
	int symbolsCount;
	txS1** symbols = NULL;
	txUsage* usages = NULL;
	txS1** paths = NULL;
	txS1** hosts = NULL;

	if (argc < 2) {
		fprintf(stderr, "### No input!\n");
		return 1;
	}

	aFile = fopen(argv[1], "rb");
	bailElse(aFile);
	
	bailElse(fseek(aFile, 0, SEEK_END) == 0);
	aSize = ftell(aFile);
	bailElse(fseek(aFile, 0, SEEK_SET) == 0);
	aBuffer = malloc(aSize);
	bailElse(aBuffer != NULL);
	bailElse(fread(aBuffer, aSize, 1, aFile) == 1);	
	p = aBuffer;
	atom.atomSize = ntohl(((Atom*)p)->atomSize);
	atom.atomType = ntohl(((Atom*)p)->atomType);
	fprintf(stdout, "%4.4s %8ld\n", (char*)(p + sizeof(long)), atom.atomSize);
	if (atom.atomType == XS_ATOM_ARCHIVE)
		isArchive = 1;
	else if (atom.atomType == XS_ATOM_BINARY)
		isArchive = 0;
	else { 
		error = EINVAL; 
		goto bail;
	}
	p += sizeof(atom);
	atom.atomSize = ntohl(((Atom*)p)->atomSize);
	atom.atomType = ntohl(((Atom*)p)->atomType);
	bailAssert(atom.atomType == XS_ATOM_VERSION);
	fprintf(stdout, "%4.4s %8ld %d %d %d %d\n", (char*)(p + sizeof(long)), atom.atomSize, *(p + sizeof(atom)), *(p + sizeof(atom) + 1), *(p + sizeof(atom) + 2), *(p + sizeof(atom) + 3));
	p += sizeof(atom);
	bailAssert(*p++ == XS_MAJOR_VERSION);
	bailAssert(*p++ == XS_MINOR_VERSION);
	bailAssert(*p++ == XS_PATCH_VERSION);
	hasC = *p++;	
	atom.atomSize = ntohl(((Atom*)p)->atomSize);
	atom.atomType = ntohl(((Atom*)p)->atomType);
	fprintf(stdout, "%4.4s %8ld\n", (char*)(p + sizeof(long)), atom.atomSize);
	bailAssert(atom.atomType == XS_ATOM_SYMBOLS);
	p += sizeof(atom);
	c = symbolsCount = ntohs(*((unsigned short*)p));
	p += 2;
	if (c) {
		symbols = malloc(c * sizeof(txS1*));
		usages = malloc(c * sizeof(txUsage));
		for (i = 0; i < c; i++) {
			symbols[i] = p;
			usages[i].name = p;
			usages[i].debug = 0;
			usages[i].property = 0;
			usages[i].variable = 0;
			p += c_strlen((char*)p) + 1;
		}
	}
	atom.atomSize = ntohl(((Atom*)p)->atomSize);
	atom.atomType = ntohl(((Atom*)p)->atomType);
	fprintf(stdout, "%4.4s %8ld\n", (char*)(p + sizeof(long)), atom.atomSize);
	if (isArchive) {
		bailAssert(atom.atomType == XS_ATOM_PATHS);
		p += sizeof(atom);
		c = ntohs(*((unsigned short*)p));
		p += 2;
		if (c) {
			paths = malloc(c * sizeof(txS1*));
			for (i = 0; i < c; i++) {
				paths[i] = p;
				p += c_strlen((char*)p) + 1;
			}
			for (i = 0; i < c; i++) {
				atom.atomSize = ntohl(((Atom*)p)->atomSize);
				atom.atomType = ntohl(((Atom*)p)->atomType);
				bailAssert(atom.atomType == XS_ATOM_CODE);
				fprintf(stdout, "%4.4s %8ld %s\n", (char*)(p + sizeof(long)), atom.atomSize, paths[i]);
				fxDumpCode(paths[i], symbols, usages, hosts, (txS1*)(p + sizeof(atom)), atom.atomSize - sizeof(atom));
				p += atom.atomSize;
			}
		}
	}
	else {
		bailAssert(atom.atomType == XS_ATOM_CODE);
		codeBuffer = p + sizeof(atom);
		codeSize = atom.atomSize - sizeof(atom);
		if (hasC == -1) {
			p += atom.atomSize;
			atom.atomSize = ntohl(((Atom*)p)->atomSize);
			atom.atomType = ntohl(((Atom*)p)->atomType);
			fprintf(stdout, "%4.4s %8ld\n", (char*)(p + sizeof(long)), atom.atomSize);
			bailAssert(atom.atomType == XS_ATOM_HOSTS);
			p += sizeof(atom);
			c = ntohs(*((unsigned short*)p));
			p += 2;
			if (c) {
				hosts = malloc(c * sizeof(txS1*));
				for (i = 0; i < c; i++) {
					hosts[i] = p;
					p += c_strlen((char*)p) + 1;
				}
			}
		}
		fxDumpCode((txS1*)argv[1], symbols, usages, hosts, codeBuffer, codeSize);
	}
	if (symbolsCount) {
		qsort(usages, symbolsCount, sizeof(txUsage), compareUsages);
		fprintf(stdout, "DEBUG\tPROP\tVAR\n");
		for (i = 0; i < symbolsCount; i++) {
			txUsage* usage = &usages[i];
			if (usage->debug || usage->property || usage->variable)
				fprintf(stdout, "%5ld\t%5ld\t%5ld\t%s\n", usage->debug, usage->property, usage->variable, usage->name);
		}
	}
bail:
	if (aFile)
		fclose(aFile);
	return error;
}
