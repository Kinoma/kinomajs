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

#include "xsAll.h"

txUTF8Sequence gxUTF8Sequences[] = {
	{1, 0x80, 0x00, 0*6, 0x0000007F, 0x00000000},
	{2, 0xE0, 0xC0, 1*6, 0x000007FF, 0x00000080},
	{3, 0xF0, 0xE0, 2*6, 0x0000FFFF, 0x00000800},
	{4, 0xF8, 0xF0, 3*6, 0x001FFFFF, 0x00010000},
	{5, 0xFC, 0xF8, 4*6, 0x03FFFFFF, 0x00200000},
	{6, 0xFE, 0xFC, 5*6, 0x7FFFFFFF, 0x04000000},
	{0, 0, 0, 0, 0, 0},
};

txString fxConcatString(txMachine* the, txSlot* a, txSlot* b)
{
	txSize aSize = c_strlen(a->value.string);
	txSize bSize = c_strlen(b->value.string);
	txString result = (txString)fxNewChunk(the, aSize + bSize + 1);
	c_memcpy(result, a->value.string, aSize);
	c_memcpy(result + aSize, b->value.string, bSize + 1);
	a->value.string = result;
	return result;
}

txString fxConcatStringC(txMachine* the, txSlot* a, txString b)
{
	txSize aSize = c_strlen(a->value.string);
	txSize bSize = c_strlen(b);
	txSize resultSize = aSize + bSize + 1;
	txString result = (txString)fxRenewChunk(the, a->value.string, resultSize);
	if (!result) {
		result = (txString)fxNewChunk(the, resultSize);
		c_memcpy(result, a->value.string, aSize);
		a->value.string = result;
	}
	c_memcpy(result + aSize, b, bSize + 1);
	return result;
}

txString fxCopyString(txMachine* the, txSlot* a, txSlot* b)
{
	txString result = b->value.string;
	a->value.string = result;
	a->kind = b->kind;
	return result;
}

txString fxCopyStringC(txMachine* the, txSlot* a, txString b)
{
	txSize bSize = c_strlen(b);
	txString result = (txString)fxNewChunk(the, bSize + 1);
	c_memcpy(result, b, bSize + 1);
	a->value.string = result;
	a->kind = XS_STRING_KIND;
	return result;
}

txString fxResizeString(txMachine* the, txSlot* a, txSize theSize)
{
	txString result = (txString)fxRenewChunk(the, a->value.string, theSize);
	if (!result) {
		txChunk* aChunk = (txChunk*)(a->value.string - sizeof(txChunk));
		txSize aSize = aChunk->size - sizeof(txChunk); 
		result = (txString)fxNewChunk(the, theSize);
		aChunk = (txChunk*)(result - sizeof(txChunk));
		theSize = aChunk->size - sizeof(txChunk);
		c_memcpy(result, a->value.string, (aSize < theSize) ? aSize : theSize);
		a->value.string = result;
	}
	return result;
}

int fxStringGetter(void* theStream)
{
	txStringStream* aStream = (txStringStream*)theStream;
	int result = C_EOF;
	
	if (aStream->offset < aStream->size) {
		result = *(aStream->slot->value.string + aStream->offset);
		aStream->offset++;
	}
	return result;
}

int fxStringCGetter(void* theStream)
{
	txStringCStream* aStream = (txStringCStream*)theStream;
	int result = C_EOF;
	
	if (aStream->offset < aStream->size) {
		result = *(aStream->buffer + aStream->offset);
		aStream->offset++;
	}
	return result;
}

void fxJump(txMachine* the)
{
	txJump* aJump = the->firstJump;
	the->firstJump = aJump->nextJump;
	aJump->nextJump = C_NULL;
	the->frame = aJump->frame;
	the->scope = aJump->scope;
	the->stack = aJump->stack;
	the->code = aJump->code;
	c_longjmp(aJump->buffer, 1);
}

txInteger fxUnicodeCharacter(txString theString)
{
	txU1* aString = (txU1*)theString;
	txU4 aResult = *aString++;
	txUTF8Sequence *aSequence;
	txInteger aSize;

	for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
		if ((aResult & aSequence->cmask) == aSequence->cval)
			break;
	}
	aSize = aSequence->size - 1;
	while (aSize > 0) {
		aSize--;
		aResult = (aResult << 6) | (*aString++ & 0x3F);
	}
	aResult &= aSequence->lmask;
	return (txInteger)aResult;
}

txInteger fxUnicodeLength(txString theString)
{
	txU1* p = (txU1*)theString;
	txU1 c;
	txInteger anIndex = 0;
	
	while ((c = *p++)) {
		if ((c & 0xC0) != 0x80)
			anIndex++;
	}
	return anIndex;
}

txInteger fxUnicodeToUTF8Offset(txString theString, txInteger theOffset)
{
	txU1* p = (txU1*)theString;
	txU1 c;
	txInteger unicodeOffset = 0;
	txInteger utf8Offset = 0;
	
	while ((c = *p++)) {
		if ((c & 0xC0) != 0x80) {
			if (unicodeOffset == theOffset)
				return utf8Offset;
			unicodeOffset++;
		}
		utf8Offset++;
	}
	if (unicodeOffset == theOffset)
		return utf8Offset;
	else
		return -1;
}

txInteger fxUTF8ToUnicodeOffset(txString theString, txInteger theOffset)
{
	txU1* p = (txU1*)theString;
	txU1 c;
	txInteger unicodeOffset = 0;
	txInteger utf8Offset = 0;
	
	while ((c = *p++)) {
		if ((c & 0xC0) != 0x80) {
			if (utf8Offset == theOffset)
				return unicodeOffset;
			unicodeOffset++;
		}
		utf8Offset++;
	}
	if (utf8Offset == theOffset)
		return unicodeOffset;
	else
		return -1;
}
