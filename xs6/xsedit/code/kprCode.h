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
#ifndef __KPRCODE__
#define __KPRCODE__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "kprMessage.h"

typedef struct KprCodeStruct KprCodeRecord, *KprCode;
typedef struct KprCodeIteratorStruct KprCodeIteratorRecord, *KprCodeIterator;
typedef struct KprCodeRunStruct KprCodeRunRecord, *KprCodeRun;
typedef struct KprCodeResultStruct KprCodeResultRecord, *KprCodeResult;

struct KprCodeStruct {
	KprSlotPart;
	KprContentPart;
	char* string;
	UInt32 length;
	SInt32 from;
	SInt32 fromColumn;
	SInt32 fromLine;
	SInt32 to;
	SInt32 toColumn;
	SInt32 toLine;
	SInt32 columnCount;
	SInt32 lineCount;
	double columnWidth;
	double lineHeight;
	FskGrowableArray runs;
	SInt32 type;
	void* pcre;
	SInt32 pcreCount;
	SInt32* pcreOffsets;
	FskGrowableArray results;
};

struct KprCodeIteratorStruct {
	char character;
	SInt16 color;
	SInt16 kind;
	SInt32 limit;
	SInt32 offset;
	KprCodeRun run;
};

struct KprCodeRunStruct {
	SInt16 kind;
	SInt16 color;
	SInt32 count;
};

struct KprCodeResultStruct {
	SInt32 count;
	SInt32 from;
	SInt32 fromColumn;
	SInt32 fromLine;
	SInt32 to;
	SInt32 toColumn;
	SInt32 toLine;
};

enum {
	kprCodeColorKind = 0,
	kprCodeLineKind,
	kprCodeTabKind,
};

FskAPI(FskErr) KprCodeNew(KprCode* it,  KprCoordinates coordinates, KprSkin skin, KprStyle style, char* string);
FskAPI(void) KprCodeFlagged(KprCode self);
FskAPI(void) KprCodeGetSelectionBounds(KprCode self, FskRectangle bounds);
FskAPI(SInt32) KprCodeHitOffset(KprCode self, double x, double y);
FskAPI(FskErr) KprCodeInsertString(KprCode self, char* string);
FskAPI(FskErr) KprCodeInsertStringWithLength(KprCode self, char* string, UInt32 size);
FskAPI(FskErr) KprCodeSelect(KprCode self, SInt32 selectionOffset, UInt32 selectionLength);
FskAPI(FskErr) KprCodeSetString(KprCode self, char* string);
FskAPI(FskErr) KprCodeSetStringWithLength(KprCode self, char* string, UInt32 size);

extern FskErr KprCodeMeasureJS(KprCode self);
extern FskErr KprCodeMeasureJSON(KprCode self);
extern FskErr KprCodeMeasureXML(KprCode self);
extern FskErr KprCodeMeasureMarkdown(KprCode self);

extern KprServiceRecord gCodeSearch;
extern KprServiceRecord gCodeService;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPRCODE__ */
