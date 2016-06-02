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
#include "kprCodeColor.h"

typedef struct {
	txS4 size;
	txU4 cmask;
	txU4 cval;
	txS4 shift;
	txU4 lmask;
	txU4 lval;
} txUTF8Sequence;

static const txUTF8Sequence gxUTF8Sequences[] = {
	{1, 0x80, 0x00, 0*6, 0x0000007F, 0x00000000},
	{2, 0xE0, 0xC0, 1*6, 0x000007FF, 0x00000080},
	{3, 0xF0, 0xE0, 2*6, 0x0000FFFF, 0x00000800},
	{4, 0xF8, 0xF0, 3*6, 0x001FFFFF, 0x00010000},
	{5, 0xFC, 0xF8, 4*6, 0x03FFFFFF, 0x00200000},
	{6, 0xFE, 0xFC, 5*6, 0x7FFFFFFF, 0x04000000},
	{0, 0, 0, 0, 0, 0},
};

void KprCodeParserAdvance(KprCodeParser* parser)
{
	UInt32 character;
	UInt8* string;
	const txUTF8Sequence *sequence;
	SInt32 size;
	
	parser->columnIndex++;
	parser->input += parser->size;
	string = parser->string + parser->input;
	character = *string;
	if (character & 0x80) {
		for (sequence = gxUTF8Sequences; sequence->size; sequence++) {
			if ((character & sequence->cmask) == sequence->cval)
				break;
		}
		size = parser->size = sequence->size;
		if (size == 0) {
			character = 0;
		}
		else {
			size--;
			while (size) {
				size--;
				character = (character << 6) | (*string++ & 0x3F);
			}
			character &= sequence->lmask;
		}
	}
	else
		parser->size = 1;
	parser->character = character;
}

FskErr KprCodeParserBegin(KprCode self, KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	FskGrowableArrayDispose(self->runs);
	self->runs = NULL;
	bailIfError(FskGrowableArrayNew(sizeof(KprCodeRunRecord), 1024, &(self->runs)));
	parser->runs = self->runs;
	parser->columnCount = 0;
	parser->columnIndex = -1;
	parser->lineCount = 1;
	parser->input = 0;
	parser->output = 0;
	parser->size = 0;
	parser->string = (UInt8*)self->string;
	parser->tab = 4;
	parser->color = 0;
	parser->offset = 0;
	KprCodeParserAdvance(parser);
bail:
	return err;
}

FskErr KprCodeParserColorAt(KprCodeParser* parser, SInt32 color, SInt32 offset)
{
	FskErr err = kFskErrNone;
	KprCodeRunRecord runRecord;
	if (parser->color != color) {
		if (offset > parser->output) {
			runRecord.kind = kprCodeColorKind;
			runRecord.color = parser->color;
			runRecord.count = offset - parser->output;
			bailIfError(FskGrowableArrayAppendItem(parser->runs, (void *)&runRecord));
			parser->output = offset;
		}
		parser->color = color;
	}
bail:
	return err;
}

FskErr KprCodeParserEnd(KprCode self, KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
	while (parser->character) {
		switch (parser->character) {
		case 9:
			bailIfError(KprCodeParserTab(parser));
			break;
		case 10:
		case 13:
			bailIfError(KprCodeParserReturn(parser));
			break;
		default:
			KprCodeParserAdvance(parser);
			break;
		}
	}
	bailIfError(KprCodeParserFill(parser));
	if (parser->columnCount < parser->columnIndex)
		parser->columnCount = parser->columnIndex;
	self->columnCount = parser->columnCount;
	self->lineCount = parser->lineCount;
	/*{
		KprCodeRun run;
		char* string = self->string;
		FskGrowableArrayGetPointerToItem(self->runs, 0, (void **)&run);
		fprintf(stderr, "### %ld %ld\n", parser->lineCount, parser->columnCount);
		while (*string) {
			if (run->kind == kprCodeLineKind)
				fprintf(stderr, "\n#");
			else if (run->kind == kprCodeTabKind)
				fprintf(stderr, "<%d>", run->color);
			else {
				char c = string[run->count];
				string[run->count] = 0;
				fprintf(stderr, "(%d)%s", run->color, string);
				string[run->count] = c;
			}
			string += run->count;
			run++;
		}
		fprintf(stderr, "\n");
	}*/
bail:
	return err;
}

FskErr KprCodeParserFill(KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	KprCodeRunRecord runRecord;
	if (parser->input > parser->output) {
		runRecord.kind = kprCodeColorKind;
		runRecord.color = parser->color;
		runRecord.count = parser->input - parser->output;
		bailIfError(FskGrowableArrayAppendItem(parser->runs, (void *)&runRecord));
		parser->output = parser->input;
	}
bail:
	return err;
}

FskErr KprCodeParserReturn(KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	UInt32 character = parser->character;
	KprCodeRunRecord runRecord;
	bailIfError(KprCodeParserFill(parser));
	KprCodeParserAdvance(parser);
	if ((character == 13) && (parser->character == 10))
		KprCodeParserAdvance(parser);
	runRecord.kind = kprCodeLineKind;
	runRecord.color = 0;
	runRecord.count = parser->input - parser->output;
	bailIfError(FskGrowableArrayAppendItem(parser->runs, (void *)&runRecord));
	parser->output = parser->input;
	if (parser->columnCount < parser->columnIndex)
		parser->columnCount = parser->columnIndex;
	parser->columnIndex = 0;
	parser->lineCount++;
bail:
	return err;
}

FskErr KprCodeParserTab(KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	KprCodeRunRecord runRecord;
	bailIfError(KprCodeParserFill(parser));
	KprCodeParserAdvance(parser);
	SInt32 columnIndex = parser->columnIndex - 1;
	parser->columnIndex = columnIndex + parser->tab;
	parser->columnIndex -= parser->columnIndex % parser->tab;
	runRecord.kind = kprCodeTabKind;
	runRecord.color = parser->columnIndex - columnIndex;
	runRecord.count = 1;
	bailIfError(FskGrowableArrayAppendItem(parser->runs, (void *)&runRecord));
	parser->output = parser->input;
bail:
	return err;
}
