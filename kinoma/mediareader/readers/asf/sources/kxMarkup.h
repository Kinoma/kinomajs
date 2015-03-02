/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#ifndef __KXMARKUP__
#define __KXMARKUP__

#include "Fsk.h"

typedef struct {
	char	*name;
	char	*value;
	UInt32	valueLen;
} txAttribute;

struct sxMarkupCallbacks {
	void (*processStartTag)(void *state, char *tagName, txAttribute *attributes);
	void (*processStopTag)(void *state, char *tagName);
	void (*processText)(void *state, char *text);
	void (*lookupEntity)(void *state);
	void (*processComment)(void *state);
	void (*processProcessingInstruction)(void *state, char *tagName, char *value);
	void (*processDoctype)(void *state);
	long (*processGetCharacterEncoding)(void *state);
};

enum {
	charEncodingUnknown = 0,
	charEncodingISO8859_1,
	charEncodingUTF8,
	charEncodingUTF16BE,
	charEncodingUTF16LE
};

typedef UInt32 txSize;
typedef char *txString;
typedef UInt32 txU4;
typedef SInt32 txS4;
typedef txS4 txInteger;
typedef unsigned char txU1;
typedef int txIndex;
typedef struct sxMarkupCallbacks txMarkupCallbacks; 

void kxParseMarkupBuffer(void* theBuffer, txSize theSize, 
		char* thePath, long theLine, txMarkupCallbacks* theCallbacks, void *state);

#define C_EOF (-1)

typedef struct {
	txString buffer;
	txSize offset;
	txSize size;
} txStringCStream;

#endif
