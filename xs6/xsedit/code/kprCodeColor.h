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
#ifndef __KPRCOLORPARSER__
#define __KPRCOLORPARSER__

#include "kprCode.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
	FskGrowableArray runs;
	UInt32 character;
	SInt32 columnCount;
	SInt32 columnIndex;
	SInt32 lineCount;
	SInt32 input;
	SInt32 output;
	SInt32 size;
	UInt8* string;
	SInt32 tab;
	
	SInt32 color;
	SInt32 offset;
	SInt32 token;
} KprCodeParser;

extern void KprCodeParserAdvance(KprCodeParser* parser);
extern FskErr KprCodeParserBegin(KprCode self, KprCodeParser* parser);
extern FskErr KprCodeParserColorAt(KprCodeParser* parser, SInt32 color, SInt32 offset);
extern FskErr KprCodeParserEnd(KprCode self, KprCodeParser* parser);
extern FskErr KprCodeParserFill(KprCodeParser* parser);
extern FskErr KprCodeParserReturn(KprCodeParser* parser);
extern FskErr KprCodeParserTab(KprCodeParser* parser);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPRCODE__ */
