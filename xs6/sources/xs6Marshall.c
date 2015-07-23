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

typedef struct sxMarshallBuffer txMarshallBuffer; 
struct sxMarshallBuffer {
	txByte* base;
	txByte* current;
	txSlot* link;
	txSize size;
	txID symbolCount;
	txIndex symbolSize;
};

static void fxDemarshallChunk(txMachine* the, void* theData, void** theDataAddress);
static void fxDemarshallSlot(txMachine* the, txSlot* theSlot, txSlot* theResult, txID* theSymbolMap);
static void fxMarshallChunk(txMachine* the, void* theData, void** theDataAddress, txMarshallBuffer* theBuffer);
static void fxMarshallSlot(txMachine* the, txSlot* theSlot, txSlot** theSlotAddress, txMarshallBuffer* theBuffer);
static void fxMeasureChunk(txMachine* the, void* theData, txMarshallBuffer* theBuffer);
static void fxMeasureSlot(txMachine* the, txSlot* theSlot, txMarshallBuffer* theBuffer);

#define mxMarshallAlign(POINTER,SIZE) \
	if (((SIZE) &= ((sizeof(txNumber) - 1)))) (POINTER) += sizeof(txNumber) - (SIZE)

void fxDemarshall(txMachine* the, void* theData)
{
	txFlag aFlag;
	txByte* p = (txByte*)theData;
	txByte* q = p + *((txSize*)(p));
	txID aSymbolCount;
	txID aSymbolLength;
	txID* aSymbolMap;
	txID* aSymbolPointer;
	txSlot* aSlot;
	txChunk* aChunk;
	txInteger skipped;
	txIndex aLength;
	
	aFlag = (txFlag)the->collectFlag;
	the->collectFlag &= ~(XS_COLLECTING_FLAG | XS_SKIPPED_COLLECT_FLAG);
	{
		mxTry(the) {
			p += sizeof(txSize);
			aSymbolCount = *((txID*)p);
			p += sizeof(txID);
			aSymbolMap = aSymbolPointer = (txID*)p;
			p += aSymbolCount * sizeof(txID);
			while (aSymbolCount) {
				aSlot = fxNewNameC(the, (char *)p);
				aSymbolLength = *aSymbolPointer;
				*aSymbolPointer++ = aSlot->ID;
				aSymbolCount--;
				p += aSymbolLength;
			}
			aLength = p - (txByte*)theData;
			mxMarshallAlign(p, aLength);
			mxPushUndefined();
			fxDemarshallSlot(the, (txSlot*)p, the->stack, aSymbolMap);
		}
		mxCatch(the) {
			the->stack->kind = XS_UNDEFINED_KIND;
			break;
		}
		while (p < q) {
			aSlot = (txSlot*)p;
			p += sizeof(txSlot);
			switch (aSlot->kind) {
			case XS_STRING_KIND:
				aChunk = (txChunk*)p;
				p += aChunk->size;
				break;
			case XS_HOST_KIND:
				aLength = aSlot->next->value.integer;
				if (aLength) {
					p += aLength;
					mxMarshallAlign(p, aLength);
				}
				break;
			case XS_INSTANCE_KIND:
				aSlot->value.instance.garbage = C_NULL;
				break;
			}
		}
	}
	skipped = the->collectFlag & XS_SKIPPED_COLLECT_FLAG;
	the->collectFlag = aFlag;
	if (skipped)
		fxCollectGarbage(the);
}

void fxDemarshallChunk(txMachine* the, void* theData, void** theDataAddress)
{
	txSize aSize = ((txChunk*)(((txByte*)theData) - sizeof(txChunk)))->size - sizeof(txChunk);
	txByte* aResult = (txByte *)fxNewChunk(the, aSize);
	c_memcpy(aResult, theData, aSize);
	*theDataAddress = aResult;
}

void fxDemarshallSlot(txMachine* the, txSlot* theSlot, txSlot* theResult, txID* theSymbolMap)
{
	txID anID;
	txSlot* aSlot;
	txIndex aLength;
	txSlot* aResult;
	txSlot** aSlotAddress;
	
	anID = theSlot->ID;
	if (anID < XS_NO_ID) {
		txID anIndex = anID & 0x7FFF;
		if (anIndex >= the->keyOffset)
			anID = theSymbolMap[anIndex - the->keyOffset];
	}
	theResult->ID = anID;
	theResult->flag = theSlot->flag;
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
	case XS_BOOLEAN_KIND:
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
	case XS_DATE_KIND:
	case XS_STRING_X_KIND:
		theResult->value = theSlot->value;
		theResult->kind = theSlot->kind;
		break;
	case XS_STRING_KIND:
		{
		char **s = &theResult->value.string;
		fxDemarshallChunk(the, theSlot->value.string, (void **)s);
		theResult->kind = theSlot->kind;
		}
		break;
	case XS_REFERENCE_KIND:
		aSlot = theSlot->value.reference;
		if ((aSlot->flag & XS_SHARED_FLAG) == 0) {
			if (aSlot->value.instance.garbage) {
				theResult->value.reference = aSlot->value.instance.garbage;
				theResult->kind = theSlot->kind;
			}
			else {
				theResult->value.reference = fxNewSlot(the);
				theResult->kind = theSlot->kind;
				fxDemarshallSlot(the, aSlot, theResult->value.reference, theSymbolMap);
			}
		}
		else {
			theResult->value.reference = aSlot;
			theResult->kind = theSlot->kind;
		}
		break;
	case XS_ARRAY_KIND:
		theResult->value.array.length = 0;
		theResult->value.array.address = C_NULL;
		theResult->kind = theSlot->kind;
		if ((aLength = theSlot->value.array.length)) {
			theResult->value.array.length = aLength;
			theResult->value.array.address = (txSlot *)fxNewChunk(the, aLength * sizeof(txSlot));
			c_memset(theResult->value.array.address, 0, aLength * sizeof(txSlot));
			aSlot = theSlot->value.array.address;
			aResult = theResult->value.array.address;
			while (aLength) {
				fxDemarshallSlot(the, aSlot, aResult, theSymbolMap);
				aLength--;
				aSlot = aSlot->next;
				aResult++;
			}
		}
		break;
	case XS_INSTANCE_KIND:
		theSlot->value.instance.garbage = theResult;
		
		theResult->value.instance.garbage = C_NULL;
		if (theSlot->value.instance.prototype == C_NULL)
			theResult->value.instance.prototype = mxObjectPrototype.value.reference;
		else
			theResult->value.instance.prototype = theSlot->value.instance.prototype;
		theResult->kind = theSlot->kind;
		
		aSlot = theSlot->next;
		if (theResult->flag & XS_VALUE_FLAG) {
			switch (aSlot->kind) {
			case XS_ARRAY_KIND: theResult->value.instance.prototype = mxArrayPrototype.value.reference; break;
			case XS_BOOLEAN_KIND: theResult->value.instance.prototype = mxBooleanPrototype.value.reference; break;
			case XS_DATE_KIND: theResult->value.instance.prototype = mxDatePrototype.value.reference; break;
			case XS_NUMBER_KIND: theResult->value.instance.prototype = mxNumberPrototype.value.reference; break;
			case XS_STRING_KIND: theResult->value.instance.prototype = mxStringPrototype.value.reference; break;
			}
		}
		aSlotAddress = &(theResult->next);
		while (aSlot) {
			*aSlotAddress = fxNewSlot(the);
			fxDemarshallSlot(the, aSlot, *aSlotAddress, theSymbolMap);
			aSlot = aSlot->next;
			aSlotAddress = &((*aSlotAddress)->next);
		}
		break;	
	default:
		break;
	}
}

void* fxMarshall(txMachine* the)
{
	txMarshallBuffer aBuffer = { C_NULL, C_NULL, C_NULL, 0, 0, 0 };
	txSlot* aSlot;
	txSlot* bSlot;
	txSlot* cSlot;
	
	mxTry(the) {
		aBuffer.symbolSize = sizeof(txSize) + sizeof(txID);
        the->stack->ID = XS_NO_ID;
		fxMeasureSlot(the, the->stack, &aBuffer);
		aBuffer.size += aBuffer.symbolSize;
		mxMarshallAlign(aBuffer.size, aBuffer.symbolSize);
		
		aBuffer.base = aBuffer.current = (txByte *)c_malloc(aBuffer.size);
		if (!aBuffer.base)
			mxRangeError("marshall: cannot allocate buffer");
		*((txSize*)(aBuffer.current)) = aBuffer.size;
		aBuffer.current += sizeof(txSize);
		*((txID*)(aBuffer.current)) = aBuffer.symbolCount;
		aBuffer.current += sizeof(txID);
		if (aBuffer.symbolCount) {
			txID anIndex = the->keyOffset;
			txSlot** p = the->keyArray + anIndex;
			txSlot** q = the->keyArray + the->keyIndex;
			txID* lengths = (txID*)aBuffer.current;
			aBuffer.current += aBuffer.symbolCount * sizeof(txID);
			while (p < q) {
				aSlot = *p;
				if (aSlot->flag & XS_MARK_FLAG) {
					txID aLength = aSlot->ID;
					aSlot->ID = anIndex | 0x8000;
					c_memcpy(aBuffer.current, aSlot->value.key.string, aLength);
					aBuffer.current += aLength;
					anIndex++;
					*lengths++ = aLength;
				}
				p++;
			}
		}
		mxMarshallAlign(aBuffer.current, aBuffer.symbolSize);
		
		fxMarshallSlot(the, the->stack, &aSlot, &aBuffer);
		aSlot = aBuffer.link;
		while (aSlot) {
			bSlot = aSlot->value.instance.garbage;
			aSlot->flag &= ~XS_MARK_FLAG;
			aSlot->value.instance.garbage = C_NULL;
			aSlot = bSlot;
		}
		
		mxCheck(the, aBuffer.current == aBuffer.base + aBuffer.size);
	}
	mxCatch(the) {
		aSlot = the->firstHeap;
		while (aSlot) {
			bSlot = aSlot + 1;
			cSlot = aSlot->value.reference;
			while (bSlot < cSlot) {
				bSlot->flag &= ~XS_MARK_FLAG; 
				bSlot++;
			}
			aSlot = aSlot->next;
		}
		if (aBuffer.base) {
			c_free(aBuffer.base);
			aBuffer.base = C_NULL;
		}
		break;
	}
	if (aBuffer.symbolCount) {
		txID anIndex = the->keyOffset;
		txSlot** p = the->keyArray + anIndex;
		txSlot** q = the->keyArray + the->keyIndex;
		while (p < q) {
			aSlot = *p;
			if (aSlot->flag & XS_MARK_FLAG) {
				aSlot->flag &= ~XS_MARK_FLAG;
				aSlot->ID = anIndex | 0x8000;
			}
			p++;
			anIndex++;
		}
	}
	the->stack++;
	return aBuffer.base;
}

void fxMarshallChunk(txMachine* the, void* theData, void** theDataAddress, txMarshallBuffer* theBuffer)
{
	txChunk* aChunk = ((txChunk*)(((txByte*)theData) - sizeof(txChunk)));
	txSize aSize = aChunk->size & 0x7FFFFFFF;
	txByte* aResult = theBuffer->current;
	theBuffer->current += aSize;
	c_memcpy(aResult, aChunk, aSize);
	aChunk = (txChunk*)aResult;
	aChunk->size &= 0x7FFFFFFF;
	*theDataAddress = aResult + sizeof(txChunk);
	mxMarshallAlign(theBuffer->current, aSize);
}

void fxMarshallSlot(txMachine* the, txSlot* theSlot, txSlot** theSlotAddress, txMarshallBuffer* theBuffer)
{
	txSlot* aResult;
	txID anID;
	txSlot* aSlot;
	txIndex aLength;
	txSlot** aSlotAddress;
	
	aResult = (txSlot*)(theBuffer->current);
	theBuffer->current += sizeof(txSlot);
	anID = theSlot->ID;
	if (anID < XS_NO_ID)
		anID = the->keyArray[anID & 0x7FFF]->ID;
	aResult->ID = anID;
	aResult->flag = theSlot->flag;
	aResult->kind = theSlot->kind;
	aResult->value = theSlot->value;
	*theSlotAddress = aResult;
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
	case XS_BOOLEAN_KIND:
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
	case XS_DATE_KIND:
	case XS_STRING_X_KIND:
		break;
	case XS_STRING_KIND: {
		char **s = &aResult->value.string;
		fxMarshallChunk(the, theSlot->value.string, (void **)s, theBuffer);
		}
		break;
	case XS_REFERENCE_KIND:
		aSlot = theSlot->value.reference;
		if ((aSlot->flag & XS_SHARED_FLAG) == 0) {
			if (aSlot->value.instance.garbage)
				aResult->value.reference = aSlot->value.instance.garbage;
			else
				fxMarshallSlot(the, aSlot, &(aResult->value.reference), theBuffer);
		}
		break;
	case XS_ARRAY_KIND:
		aLength = theSlot->value.array.length;
		aSlot = theSlot->value.array.address;
		aSlotAddress = &(aResult->value.array.address);
		while (aLength) {
			fxMarshallSlot(the, aSlot, aSlotAddress, theBuffer);
			aLength--;
			aSlot++;
			aSlotAddress = &((*aSlotAddress)->next);
		}
		break;
	case XS_INSTANCE_KIND:
		aResult->value.instance.garbage = theBuffer->link;
		if (theSlot->value.instance.prototype && (theSlot->value.instance.prototype->flag & XS_SHARED_FLAG))
			aResult->value.instance.prototype = theSlot->value.instance.prototype;
		else
			aResult->value.instance.prototype = C_NULL;
		theSlot->value.instance.garbage = aResult;
		theBuffer->link = theSlot;
		aSlot = theSlot->next;
		aSlotAddress = &(aResult->next);
		while (aSlot) {
			fxMarshallSlot(the, aSlot, aSlotAddress, theBuffer);
			aSlot = aSlot->next;
			aSlotAddress = &((*aSlotAddress)->next);
		}
		*aSlotAddress = C_NULL;
		break;	
	default:
		break;
	}
}

void fxMeasureChunk(txMachine* the, void* theData, txMarshallBuffer* theBuffer)
{
	txChunk* aChunk = ((txChunk*)(((txByte*)theData) - sizeof(txChunk)));
	txSize aSize = aChunk->size & 0x7FFFFFFF;
	theBuffer->size += aSize;
	mxMarshallAlign(theBuffer->size, aSize);
}

void fxMeasureSlot(txMachine* the, txSlot* theSlot, txMarshallBuffer* theBuffer)
{
	txID anID;
	txSlot* aSlot;
	txIndex aLength;

	anID = theSlot->ID;
	if (anID < XS_NO_ID) {
		anID &= 0x7FFF;
		if (anID >= the->keyOffset) {
			aSlot = the->keyArray[anID];
			if (!(aSlot->flag & XS_MARK_FLAG)) {
				aSlot->flag |= XS_MARK_FLAG;
				aLength = c_strlen(aSlot->value.key.string) + 1;
				if (aLength > 0x7FFF)
					mxRangeError("marshall: too long symbol");
				aSlot->ID = (txID)aLength;
				theBuffer->symbolSize += sizeof(txID);
				theBuffer->symbolSize += aLength;
				theBuffer->symbolCount++;
			}
		}
	}
	theBuffer->size += sizeof(txSlot);
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
	case XS_BOOLEAN_KIND:
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
	case XS_DATE_KIND:
	case XS_STRING_X_KIND:
		break;
	case XS_STRING_KIND:
		fxMeasureChunk(the, theSlot->value.string, theBuffer);
		break;
	case XS_REFERENCE_KIND:
		aSlot = theSlot->value.reference;
		if ((aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG)) == 0)
			fxMeasureSlot(the, aSlot, theBuffer);
		break;
	case XS_ARRAY_KIND:
		aLength = theSlot->value.array.length;
		aSlot = theSlot->value.array.address;
		while (aLength) {
			fxMeasureSlot(the, aSlot, theBuffer);
			aLength--;
			aSlot++;
		}
		break;
	case XS_INSTANCE_KIND:
		theSlot->flag |= XS_MARK_FLAG;
		theSlot->value.instance.garbage = C_NULL;
		aSlot = theSlot->next;
		while (aSlot) {
			fxMeasureSlot(the, aSlot, theBuffer);
			aSlot = aSlot->next;
		}
		break;	
	default:
		mxDebugID(XS_TYPE_ERROR, "marshall %s: no way", theSlot->ID);
		break;
	}
}

