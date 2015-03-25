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

#ifndef mxReport
#define mxReport 0
#endif
#ifndef mxStress
#define mxStress 0
#endif
#ifndef mxFill
#define mxFill 0
#endif

int gxStress = 0;

#define mxChunkFlag 0x80000000

#define mxRoundSize(_SIZE) ((_SIZE + (sizeof(txChunk) - 1)) & ~(sizeof(txChunk) - 1))

static void* fxAllocateChunks(txMachine* the, txSize theSize);
static txSlot* fxAllocateSlots(txMachine* the, txSize theCount);
static void fxFreeChunks(txMachine* the, void* theChunks);
static void fxFreeSlots(txMachine* the, void* theSlots);
static void fxGrowHeap(txMachine* the, txSize theCount); 
static void fxMark(txMachine* the, void (*theMarker)(txMachine*, txSlot*));
static void fxMarkInstance(txMachine* the, txSlot* theCurrent, void (*theMarker)(txMachine*, txSlot*));
static void fxMarkReference(txMachine* the, txSlot* theSlot);
static void fxMarkValue(txMachine* the, txSlot* theSlot);
static void fxSweep(txMachine* the);
static void fxSweepValue(txMachine* the, txSlot* theSlot);

//#define mxNever 1
#ifdef mxNever

static void fxCheckChunk(txMachine* the, txKind theKind, void* theData)
{
	txChunk* aChunk;
	txBlock* aBlock;
	txByte* mByte;
	txByte* nByte;

	aChunk = (txChunk*)(((txByte*)(theData)) - sizeof(txChunk));
	aBlock = the->firstBlock;
	while (aBlock) {
		mByte = ((txByte*)aBlock) + sizeof(txBlock);
		nByte = aBlock->current;
		if ((mByte <= ((txByte*)aChunk)) && ((((txByte*)aChunk) + aChunk->size) <= nByte)) {
			if (aChunk->temporary)
				fprintf(stderr, "### INVALID CHUNK %d %8.8X %8.8X\n", theKind, aChunk, aChunk->temporary);
			return;
		}
		aBlock = aBlock->nextBlock;
	}
	if (the->sharedMachine) {
		aBlock = the->sharedMachine->firstBlock;
		while (aBlock) {
			mByte = ((txByte*)aBlock) + sizeof(txBlock);
			nByte = aBlock->current;
			if ((mByte <= ((txByte*)aChunk)) && ((((txByte*)aChunk) + (aChunk->size & ~0x80000000)) <= nByte)) {
				if (aChunk->temporary)
					fprintf(stderr, "### INVALID SHARED CHUNK %d %8.8X %8.8X\n", theKind, aChunk, aChunk->temporary);
				return;
			}
			aBlock = aBlock->nextBlock;
		}
	}
	fprintf(stderr, "### UNKNOWN CHUNK %d %8.8X %8.8s\n", theKind, aChunk, aChunk);
}

static void fxCheckReference(txMachine* the, txKind theKind, txSlot* theSlot)
{
	txSlot* aSlot;
	txSlot* bSlot;
	
	if ((the->stack <= theSlot) && (theSlot < the->stackTop))
		return;
	aSlot = the->firstHeap;
	while (aSlot) {
		bSlot = aSlot->value.reference;
		if ((aSlot < theSlot) && (theSlot < bSlot))
			return;
		aSlot = aSlot->next;
	}
	if (the->sharedMachine) {
		aSlot = the->sharedMachine->firstHeap;
		while (aSlot) {
			bSlot = aSlot->value.reference;
			if ((aSlot < theSlot) && (theSlot < bSlot))
				return;
			aSlot = aSlot->next;
		}
	}
	fprintf(stderr, "### UNKNOWN REFERENCE %d %8.8X\n", theKind, theSlot);
}

static void fxCheckValue(txMachine* the, txSlot* theSlot)
{
	txSlot* aSlot;
	if (theSlot->flag & XS_MARK_FLAG) {
		fprintf(stderr, "### MARKED SLOT %d %s", theSlot->kind, theSlot->value.string);
//		fxDebugger(the, 0, 0);
		}
	switch (theSlot->kind) {
	case XS_INSTANCE_KIND:
		if (theSlot->flag & XS_SHARED_FLAG)
			fprintf(stderr, "### SHARED INSTANCE %d %8.8X\n", theSlot->kind, theSlot);
		aSlot = theSlot->value.instance.prototype;
		if (aSlot)
			fxCheckReference(the, theSlot->kind, aSlot);
		break;
	case XS_REFERENCE_KIND:
		aSlot = theSlot->value.reference;
		fxCheckReference(the, theSlot->kind, aSlot);
		break;
	case XS_REGEXP_KIND:
		fxCheckChunk(the, theSlot->kind, theSlot->value.regexp.code);
		fxCheckChunk(the, theSlot->kind, theSlot->value.regexp.offsets);
		break;
	case XS_STRING_KIND:
		fxCheckChunk(the, theSlot->kind, theSlot->value.string);
		break;
	case XS_ARRAY_KIND:
		if (theSlot->value.array.cache)
			fxCheckChunk(the, theSlot->kind, theSlot->value.array.cache);
		break;
	case XS_GLOBAL_KIND:
		fxCheckChunk(the, theSlot->kind, theSlot->value.global.cache);
		fxCheckChunk(the, theSlot->kind, theSlot->value.global.sandboxCache);
		break;
	case XS_CODE_KIND:
		fxCheckChunk(the, theSlot->kind, theSlot->value.code);
		break;
	case XS_SYMBOL_KIND:
		if (theSlot->value.symbol.string)
			fxCheckChunk(the, theSlot->kind, theSlot->value.symbol.string);
		break;
	case XS_LIST_KIND:
		aSlot = theSlot->value.list.first;
		while (aSlot) {
			fxCheckReference(the, theSlot->kind, aSlot);
			if (aSlot->kind == XS_STRING_POINTER || aSlot->kind == XS_STRING_CONCAT || aSlot->kind == XS_STRING_CONCATBY || aSlot->kind == XS_STRING)
				fxCheckChunk(the, theSlot->kind, aSlot->value.string);
			aSlot = aSlot->next;
		}
		break;
	case XS_NODE_KIND:
		aSlot = theSlot->value.node.link;
		if (aSlot)
			fxCheckReference(the, theSlot->kind, aSlot);
		break;
	case XS_PREFIX_KIND:
		if (theSlot->value.prefix.string)
			fxCheckChunk(the, theSlot->kind, theSlot->value.prefix.string);
		break;
	case XS_ATTRIBUTE_RULE:
	case XS_DATA_RULE:
	case XS_PI_RULE:
	case XS_EMBED_RULE:
	case XS_REFER_RULE:
	case XS_REPEAT_RULE:
	case XS_ERROR_RULE:
		if (theSlot->value.rule.data)
			fxCheckChunk(the, theSlot->kind, theSlot->value.rule.data);
		break;
	case XS_JUMP_RULE:
		if (theSlot->value.rule.data) {
			fxCheckChunk(the, theSlot->kind, theSlot->value.rule.data);
			aSlot = theSlot->value.rule.data->link;
			if (aSlot)
				fxCheckReference(the, theSlot->kind, aSlot);
		}
		break;
	case XS_ATTRIBUTE_PATTERN:
	case XS_DATA_PATTERN:
	case XS_PI_PATTERN:
	case XS_EMBED_PATTERN:
	case XS_JUMP_PATTERN:
	case XS_REFER_PATTERN:
	case XS_REPEAT_PATTERN:
		fxCheckChunk(the, theSlot->kind, theSlot->value.pattern);
		break;
	}
}

extern void fxCheckMemory(txMachine* the);
void fxCheckMemory(txMachine* the)
{
	txSlot** anArray;
	txID anIndex;
	txSlot* aSlot;
	txSlot* bSlot;
	txSlot* cSlot;
	txBlock* aBlock;
	txByte* mByte;
	txByte* nByte;
	txSize aSize;
	
	if ((the->collectFlag & XS_COLLECTING_FLAG) == 0)
		return;
	fprintf(stderr, "symbols\n");
	anArray = the->symbolArray;
	anIndex = the->symbolIndex;
	while (anIndex) {
		if ((aSlot = *anArray))
			fxCheckReference(the, XS_SYMBOL_KIND, aSlot);
		anArray++;
		anIndex--;
	}
	fprintf(stderr, "aliases\n");
	anArray = the->aliasArray;
	anIndex = the->aliasIndex;
	while (anIndex) {
		if ((aSlot = *anArray))
			fxCheckReference(the, XS_ALIAS_KIND, aSlot);
		anArray++;
		anIndex--;
	}
	if ((anArray = the->linkArray)) {
		fprintf(stderr, "links\n");
		anIndex = the->aliasIndex;
		while (anIndex) {
			if ((aSlot = *anArray))
				fxCheckReference(the, XS_ALIAS_KIND, aSlot);
			anArray++;
			anIndex--;
		}
	}
	fprintf(stderr, "stack\n");
	aSlot = the->stack;
	while (aSlot < the->stackTop) {
		fxCheckValue(the, aSlot);
		aSlot++;
	}
	aSlot = the->cRoot;
	while (aSlot) {
		fxCheckValue(the, aSlot);
		aSlot = aSlot->next;
	}
	fprintf(stderr, "heap\n");
	aSlot = the->firstHeap;
	while (aSlot) {
		bSlot = aSlot + 1;
		cSlot = aSlot->value.reference;
		while (bSlot < cSlot) {
			fxCheckValue(the, bSlot);
			bSlot++;
		}
		aSlot = aSlot->next;
	}
	fprintf(stderr, "chunks\n");
	aBlock = the->firstBlock;
	while (aBlock) {
		mByte = ((txByte*)aBlock) + sizeof(txBlock);
		nByte = aBlock->current;
		while (mByte < nByte) {
			aSize = ((txChunk*)mByte)->size;
			if (aSize & mxChunkFlag) {
				aSize &= ~mxChunkFlag;
				fprintf(stderr, "### MARKED CHUNK %8.8X %8.8s\n", mByte, mByte + sizeof(txChunk));
			}
			mByte += aSize;
		}	
		aBlock = aBlock->nextBlock;
	}
}

txSize gxRenewChunkCases[4] = { 0, 0, 0, 0 };

typedef struct sxSample txSample;
struct sxSample {
	wide time;
	wide duration;
	long count;
	char* label;
};

void reportTime(txSample* theSample) 
{
	long aDurationMinute;
	long aRemainder;
	long aDurationSecond;
	long aDurationMilli;

	aDurationMinute = WideDivide(&(theSample->duration), 60000000, &aRemainder);
	aDurationSecond = aRemainder / 1000000;
	aRemainder = aRemainder % 1000000;
	aDurationMilli = aRemainder / 1000;  
	fprintf(stderr, "%s * %ld = %ld:%02ld.%03ld\n", theSample->label, theSample->count, 
			aDurationMinute, aDurationSecond, aDurationMilli);
}

void startTime(txSample* theSample) 
{
	Microseconds((UnsignedWide*)&(theSample->time));
}

void stopTime(txSample* theSample) 
{
	wide aTime;
	
	Microseconds((UnsignedWide*)&aTime);
	WideSubtract(&aTime, &(theSample->time));
	WideAdd(&(theSample->duration), &aTime);
	theSample->count++;
}

txSample gxLifeTime = { { 0, 0 }, { 0, 0 }, 0, "life" };
txSample gxMarkTime = { { 0, 0 }, { 0, 0 }, 0, "mark" };
txSample gxSweepChunkTime = { { 0, 0 }, { 0, 0 }, 0, "sweep chunk" };
txSample gxSweepSlotTime = { { 0, 0 }, { 0, 0 }, 0, "sweep slot" };
txSample gxCompactChunkTime = { { 0, 0 }, { 0, 0 }, 0, "compact chunk" };

#endif

void fxAllocate(txMachine* the, txAllocation* theAllocation)
{
#ifdef mxNever
	startTime(&gxLifeTime);
#endif

	the->currentChunksSize = 0;
	the->peakChunksSize = 0;
	the->maximumChunksSize = 0;
	the->minimumChunksSize = theAllocation->incrementalChunkSize - sizeof(txBlock);
	
	the->currentHeapCount = 0;
	the->peakHeapCount = 0;
	the->maximumHeapCount = 0;
	the->minimumHeapCount = theAllocation->incrementalHeapCount;
	
	the->firstBlock = C_NULL;
	fxAllocateChunks(the, theAllocation->initialChunkSize - sizeof(txBlock));
	
	the->firstHeap = C_NULL;
	fxGrowHeap(the, theAllocation->initialHeapCount);

	the->stackBottom = fxAllocateSlots(the, theAllocation->stackCount);
	the->stackTop = the->stackBottom + theAllocation->stackCount;
	the->stack = the->stackTop;
	
	the->symbolCount = (txID)theAllocation->symbolCount;
	the->symbolIndex = 0;
	the->symbolArray = (txSlot **)c_calloc(theAllocation->symbolCount, sizeof(txSlot*));
	if (!the->symbolArray)
		fxJump(the);

	the->symbolModulo = theAllocation->symbolModulo;
	the->symbolTable = (txSlot **)c_calloc(theAllocation->symbolModulo, sizeof(txSlot*));
	if (!the->symbolTable)
		fxJump(the);
		
	the->aliasCount = (txID)theAllocation->symbolCount;
	the->aliasIndex = 0;
	the->aliasArray = (txSlot **)c_calloc(theAllocation->symbolCount, sizeof(txSlot*));
	if (!the->aliasArray)
		fxJump(the);

	the->cRoot = C_NULL;
}

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
	fxReport(the, C_NULL, 0, "# Chunk allocation: reserved %ld used %ld peak %ld bytes\n", 
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

void fxCollect(txMachine* the, txBoolean theFlag)
{
	txSize aCount;
	txSlot* freeSlot;
	txSlot* aSlot;
	txSlot* bSlot;
	txSlot* cSlot;

	if ((the->collectFlag & XS_COLLECTING_FLAG) == 0) {
		the->collectFlag |= XS_SKIPPED_COLLECT_FLAG;
#if __FSK_LAYER__
		FskInstrumentedItemSendMessageNormal(the, kFskXSInstrSkipCollect, the);
#endif
		return;
	}
		
#ifdef mxProfile
	fxBeginGC(the);
#endif

#if __FSK_LAYER__
	FskInstrumentedItemSendMessageNormal(the, theFlag ? kFskXSInstrBeginCollectSlotsAndChunks : kFskXSInstrBeginCollectSlots, the);
#endif

	if (theFlag) {
		fxMark(the, fxMarkValue);	
		fxSweep(the);
	}
	else {
		fxMark(the, fxMarkReference);
	#ifdef mxNever
		startTime(&gxSweepSlotTime);
	#endif
		aCount = 0;
		freeSlot = C_NULL;
		aSlot = the->firstHeap;
		while (aSlot) {
			bSlot = aSlot + 1;
			cSlot = aSlot->value.reference;
			while (bSlot < cSlot) {
				if (bSlot->flag & XS_MARK_FLAG) {
					bSlot->flag &= ~XS_MARK_FLAG; 
					aCount++;
				}
				else {
					if ((bSlot->kind == XS_HOST_KIND) && (bSlot->value.host.variant.destructor)) {
						if (bSlot->flag & XS_HOST_HOOKS_FLAG) {
							if (bSlot->value.host.variant.hooks->destructor)
								(*(bSlot->value.host.variant.hooks->destructor))(bSlot->value.host.data);
						}
						else
							(*(bSlot->value.host.variant.destructor))(bSlot->value.host.data);
					}
				#if mxFill
					c_memset(bSlot, 0xFF, sizeof(txSlot));
				#endif
					bSlot->kind = XS_UNDEFINED_KIND;
					bSlot->next = freeSlot;
					freeSlot = bSlot;
				}
				bSlot++;
			}
			aSlot = aSlot->next;
		}
		the->currentHeapCount = aCount;
		the->freeHeap = freeSlot;
	#ifdef mxNever
		stopTime(&gxSweepSlotTime);
	#endif
	}
	
	aSlot = the->stack;
	while (aSlot < the->stackTop) {
		aSlot->flag &= ~XS_MARK_FLAG; 
		aSlot++;
	}
	
	if (!theFlag) {
		if (((the->maximumHeapCount - the->currentHeapCount) * 8) < the->maximumHeapCount)
			the->collectFlag |= XS_TRASHING_FLAG;
			else
				the->collectFlag &= ~XS_TRASHING_FLAG;
	}
	
#if mxReport
	if (theFlag)
		fxReport(the, C_NULL, 0, "# Chunk collection: reserved %ld used %ld peak %ld bytes\n", 
			the->maximumChunksSize, the->currentChunksSize, the->peakChunksSize);
	fxReport(the, C_NULL, 0, "# Slot collection: reserved %ld used %ld peak %ld bytes\n", 
		the->maximumHeapCount * sizeof(txSlot), 
		the->currentHeapCount * sizeof(txSlot), 
		the->peakHeapCount * sizeof(txSlot));
#endif
#if __FSK_LAYER__
	if (theFlag)
		FskInstrumentedItemSendMessageNormal(the, kFskXSInstrEndCollectChunks, the);
	FskInstrumentedItemSendMessageNormal(the, kFskXSInstrEndCollectSlots, the);
#endif

#ifdef mxProfile
	fxEndGC(the);
#endif
}

txSlot* fxDuplicateSlot(txMachine* the, txSlot* theSlot)
{
	txSlot* result;
	
	result = fxNewSlot(the);
	c_memcpy(result, theSlot, sizeof(txSlot));
	((txSlot*)result)->flag &= ~XS_MARK_FLAG;
	((txSlot*)result)->next = C_NULL;
	return result;
}

void fxFree(txMachine* the) 
{
	txSlot* aHeap;
	txBlock* aBlock;

	if (the->aliasArray)
		c_free(the->aliasArray);
	the->aliasArray = C_NULL;

	if (the->symbolTable)
		c_free(the->symbolTable);
	the->symbolTable = C_NULL;
	if (the->symbolArray)
		c_free(the->symbolArray);
	the->symbolArray = C_NULL;

	if (the->stackBottom)
		fxFreeSlots(the, the->stackBottom);
	the->stackBottom = C_NULL;
	the->stackTop = C_NULL;
	the->stack = C_NULL;
	
	while (the->firstHeap) {
		aHeap = the->firstHeap;
		the->firstHeap = aHeap->next;
		fxFreeSlots(the, aHeap);
	}
	the->firstHeap = C_NULL;
	
	while (the->firstBlock) {
		aBlock = the->firstBlock;
		the->firstBlock = aBlock->nextBlock;
		fxFreeChunks(the, aBlock);
	}
	the->firstBlock = C_NULL;
	
#ifdef mxNever
	stopTime(&gxLifeTime);
	reportTime(&gxLifeTime);
	fprintf(stderr, "chunk: %ld bytes\n", the->maximumChunksSize);
	fprintf(stderr, "slot: %ld bytes\n", the->maximumHeapCount * sizeof(txSlot));
	reportTime(&gxMarkTime);
	reportTime(&gxSweepChunkTime);
	reportTime(&gxSweepSlotTime);
	reportTime(&gxCompactChunkTime);
	fprintf(stderr, "renew: %ld %ld %ld %ld\n", 
			gxRenewChunkCases[0], 
			gxRenewChunkCases[1], 
			gxRenewChunkCases[2], 
			gxRenewChunkCases[3]);
#endif
}

void fxFreeChunks(txMachine* the, void* theChunks)
{
	c_free(theChunks);
}

void fxFreeSlots(txMachine* the, void* theSlots)
{
	c_free(theSlots);
}

void fxGrowHeap(txMachine* the, txSize theCount) 
{
	txSlot* aHeap;
	txSlot* aSlot;
	
	aHeap = fxAllocateSlots(the, theCount);
	the->maximumHeapCount += theCount;
	aHeap->next = the->firstHeap;
	aHeap->ID = 0;
	aHeap->flag = 0;
	aHeap->kind = 0;
	aHeap->value.reference = aHeap + theCount;
	the->firstHeap = aHeap;
	aSlot = aHeap + 1;
	theCount -= 2;
	while (theCount) {
		aSlot->next = aSlot + 1;
		aSlot->flag = XS_NO_FLAG;
		aSlot->kind = XS_UNDEFINED_KIND;
		aSlot = aSlot->next;
		theCount--;
	}
	aSlot->next = the->freeHeap;
	aSlot->flag = XS_NO_FLAG;
	aSlot->kind = XS_UNDEFINED_KIND;
	the->freeHeap = aHeap + 1;
	the->collectFlag &= ~XS_TRASHING_FLAG;
#if mxReport
	fxReport(the, C_NULL, 0, "# Slot allocation: reserved %ld used %ld peak %ld bytes\n", 
		the->maximumHeapCount * sizeof(txSlot), 
		the->currentHeapCount * sizeof(txSlot), 
		the->peakHeapCount * sizeof(txSlot));
#endif
#if __FSK_LAYER__
	FskInstrumentedItemSendMessageNormal(the, kFskXSInstrAllocateSlots, the);
#endif
}

void fxMark(txMachine* the, void (*theMarker)(txMachine*, txSlot*))
{
	txInteger anIndex;
	txSlot** anArray;
	txSlot* aSlot;

#ifdef mxNever
	startTime(&gxMarkTime);
#endif
	anArray = the->symbolArray;
	anIndex = the->symbolIndex;
#if mxOptimize
	anArray += the->symbolOffset;
	anIndex -= the->symbolOffset;
#endif
	while (anIndex) {
		if ((aSlot = *anArray)) {
			aSlot->flag |= XS_MARK_FLAG;
			(*theMarker)(the, aSlot);
		}
		anArray++;
		anIndex--;
	}
	
	anArray = the->aliasArray;
	anIndex = the->aliasIndex;
	while (anIndex) {
		if ((aSlot = *anArray))
			if (!(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG)))
				fxMarkInstance(the, aSlot, theMarker);
		anArray++;
		anIndex--;
	}
	if ((anArray = the->linkArray)) {
		anIndex = the->aliasIndex;
		while (anIndex) {
			if ((aSlot = *anArray))
				if (!(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG)))
					fxMarkInstance(the, aSlot, theMarker);
			anArray++;
			anIndex--;
		}
	}
	
	aSlot = the->stack;
	while (aSlot < the->stackTop) {
		(*theMarker)(the, aSlot);
		aSlot++;
	}
	aSlot = the->cRoot;
	while (aSlot) {
		(*theMarker)(the, aSlot);
		aSlot = aSlot->next;
	}
#ifdef mxNever
	stopTime(&gxMarkTime);
#endif
}

void fxMarkInstance(txMachine* the, txSlot* theCurrent, void (*theMarker)(txMachine*, txSlot*))
{
	txSlot* aProperty;
	txSlot* aTemporary;

	aProperty = theCurrent;
	theCurrent->value.instance.garbage = C_NULL;
	for (;;) {
		if (aProperty) {
			if (!(aProperty->flag & XS_MARK_FLAG)) {
				aProperty->flag |= XS_MARK_FLAG;
				switch (aProperty->kind) {
				case XS_INSTANCE_KIND:
					aTemporary = aProperty->value.instance.prototype;
					if (aTemporary && !(aTemporary->flag & (XS_MARK_FLAG | XS_SHARED_FLAG)))
						fxMarkInstance(the, aTemporary, theMarker);
					aProperty = aProperty->next;
					break;
				case XS_REFERENCE_KIND:
					aTemporary = aProperty->value.reference;
					if (!(aTemporary->flag & (XS_MARK_FLAG | XS_SHARED_FLAG))) {
						aProperty->value.reference = theCurrent;
						theCurrent = aTemporary;
						theCurrent->value.instance.garbage = aProperty;
						aProperty = theCurrent;
					}
					else
						aProperty = aProperty->next;
					break;
				default:
					(*theMarker)(the, aProperty);
					aProperty = aProperty->next;
					break;	
				}
			}
			else
				aProperty = aProperty->next;
		}
		else if (theCurrent->value.instance.garbage) {
			aProperty = theCurrent->value.instance.garbage;
			theCurrent->value.instance.garbage = C_NULL;
			
			aTemporary = aProperty->value.reference;
			aProperty->value.reference = theCurrent;
			theCurrent = aTemporary;

			aProperty = aProperty->next;
		}
		else
			break;
	}
}

void fxMarkReference(txMachine* the, txSlot* theSlot)
{
	txSlot* aSlot;
	switch (theSlot->kind) {
	case XS_REFERENCE_KIND:
		aSlot = theSlot->value.reference;
		if (!(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG)))
			fxMarkInstance(the, theSlot->value.reference, fxMarkReference);
		break;
	case XS_ACCESSOR_KIND:
		aSlot = theSlot->value.accessor.getter;
		if (aSlot && !(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG)))
			fxMarkInstance(the, aSlot, fxMarkReference);
		aSlot = theSlot->value.accessor.setter;
		if (aSlot && !(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG)))
			fxMarkInstance(the, aSlot, fxMarkReference);
		break;
	case XS_ARRAY_KIND:
		if ((aSlot = theSlot->value.array.address)) {
			txInteger aLength = theSlot->value.array.length;
			while (aLength) {
				fxMarkReference(the, aSlot);
				aSlot++;
				aLength--;
			}
		}
		break;
	case XS_LIST_KIND:
		aSlot = theSlot->value.list.first;
		while (aSlot) {
			aSlot->flag |= XS_MARK_FLAG; 
			aSlot = aSlot->next;
		}
		break;
	case XS_NODE_KIND:
		aSlot = theSlot->value.node.link;
		if (aSlot && (!(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG))))
			fxMarkInstance(the, aSlot, fxMarkReference);
		break;
	case XS_JUMP_RULE:
		if (theSlot->value.rule.data) {
			aSlot = theSlot->value.rule.data->link;
			if (aSlot && (!(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG))))
				fxMarkInstance(the, aSlot, fxMarkReference);
		}
		break;
	case XS_HOST_KIND:
		if ((theSlot->flag & XS_HOST_HOOKS_FLAG) && (theSlot->value.host.variant.hooks->marker))
			(*theSlot->value.host.variant.hooks->marker)(theSlot->value.host.data, fxMarkReference);
		break;
	case XS_CLOSURE_KIND:
		aSlot = theSlot->value.closure.reference;
		if (aSlot && (!(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG))))
			fxMarkInstance(the, aSlot, fxMarkReference);
		break;
	}
}

void fxMarkValue(txMachine* the, txSlot* theSlot)
{
#define mxMarkChunk(_THE_DATA) \
	((txChunk*)(((txByte*)_THE_DATA) - sizeof(txChunk)))->size |= mxChunkFlag

	txSlot* aSlot;
	switch (theSlot->kind) {
	case XS_REFERENCE_KIND:
		aSlot = theSlot->value.reference;
		if (!(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG)))
			fxMarkInstance(the, aSlot, fxMarkValue);
		break;
	case XS_ACCESSOR_KIND:
		aSlot = theSlot->value.accessor.getter;
		if (aSlot && !(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG)))
			fxMarkInstance(the, aSlot, fxMarkValue);
		aSlot = theSlot->value.accessor.setter;
		if (aSlot && !(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG)))
			fxMarkInstance(the, aSlot, fxMarkValue);
		break;
	case XS_REGEXP_KIND:
		mxMarkChunk(theSlot->value.regexp.code);
		mxMarkChunk(theSlot->value.regexp.offsets);
		break;
	case XS_STRING_KIND:
		mxMarkChunk(theSlot->value.string);
		break;
	case XS_ARRAY_KIND:
		if ((aSlot = theSlot->value.array.address)) {
			txInteger aLength = theSlot->value.array.length;
			while (aLength) {
				fxMarkValue(the, aSlot);
				aSlot++;
				aLength--;
			}
			mxMarkChunk(theSlot->value.array.address);
		}
		break;
	case XS_GLOBAL_KIND:
		mxMarkChunk(theSlot->value.global.cache);
		mxMarkChunk(theSlot->value.global.sandboxCache);
		break;
	case XS_CODE_KIND:
		mxMarkChunk(theSlot->value.code);
		break;
	case XS_SYMBOL_KIND:
		if (theSlot->value.symbol.string)
			mxMarkChunk(theSlot->value.symbol.string);
		break;
	case XS_LIST_KIND:
		aSlot = theSlot->value.list.first;
		while (aSlot) {
			aSlot->flag |= XS_MARK_FLAG;
			if (aSlot->kind == XS_STRING_POINTER || aSlot->kind == XS_STRING_CONCAT || aSlot->kind == XS_STRING_CONCATBY || aSlot->kind == XS_STRING)
				mxMarkChunk(aSlot->value.string);
			aSlot = aSlot->next;
		}
		break;
	case XS_NODE_KIND:
		aSlot = theSlot->value.node.link;
		if (aSlot && (!(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG))))
			fxMarkInstance(the, aSlot, fxMarkValue);
		break;
	case XS_PREFIX_KIND:
		if (theSlot->value.prefix.string)
			mxMarkChunk(theSlot->value.prefix.string);
		break;
	case XS_ATTRIBUTE_RULE:
	case XS_DATA_RULE:
	case XS_PI_RULE:
	case XS_EMBED_RULE:
	case XS_REFER_RULE:
	case XS_REPEAT_RULE:
	case XS_ERROR_RULE:
		if (theSlot->value.rule.data)
			mxMarkChunk(theSlot->value.rule.data);
		break;
	case XS_JUMP_RULE:
		if (theSlot->value.rule.data) {
			mxMarkChunk(theSlot->value.rule.data);
			aSlot = theSlot->value.rule.data->link;
			if (aSlot && (!(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG))))
				fxMarkInstance(the, aSlot, fxMarkValue);
		}
		break;
	case XS_ATTRIBUTE_PATTERN:
	case XS_DATA_PATTERN:
	case XS_PI_PATTERN:
	case XS_EMBED_PATTERN:
	case XS_JUMP_PATTERN:
	case XS_REFER_PATTERN:
	case XS_REPEAT_PATTERN:
		mxMarkChunk(theSlot->value.pattern);
		break;
	case XS_HOST_KIND:
		if ((theSlot->flag & XS_HOST_HOOKS_FLAG) && (theSlot->value.host.variant.hooks->marker))
			(*theSlot->value.host.variant.hooks->marker)(theSlot->value.host.data, fxMarkValue);
		break;
	case XS_CLOSURE_KIND:
		aSlot = theSlot->value.closure.reference;
		if (aSlot && (!(aSlot->flag & (XS_MARK_FLAG | XS_SHARED_FLAG))))
			fxMarkInstance(the, aSlot, fxMarkValue);
		if (theSlot->value.closure.symbolMap)
			mxMarkChunk(theSlot->value.closure.symbolMap);
		break;
	}
}

void* fxNewChunk(txMachine* the, txSize theSize)
{
	txBlock* aBlock;
	txByte* aData;
	txBoolean once = 1;
	
#if mxStress
	if (gxStress) {
		fxCollect(the, 1);
		once = 0;
	}
#endif
	theSize = mxRoundSize(theSize) + sizeof(txChunk);
again:
	aBlock = the->firstBlock;
	while (aBlock) {
		if ((aBlock->current + theSize) <= aBlock->limit) {
			aData = aBlock->current;
			((txChunk*)aData)->size = theSize;
			((txChunk*)aData)->temporary = C_NULL;
			aBlock->current += theSize;
			the->currentChunksSize += theSize;
			if (the->peakChunksSize < the->currentChunksSize)
				the->peakChunksSize = the->currentChunksSize;
			return aData + sizeof(txChunk);
		}
		aBlock = aBlock->nextBlock;
	}
	if (once) {
#if __FSK_LAYER__
		FskInstrumentedItemSendMessageDebug(the, kFskXSInstrNewChunkWork, (void *)theSize);
#endif
		fxCollect(the, 1);
		once = 0;
	}
	else
		fxAllocateChunks(the, theSize);
	goto again;
	
	return C_NULL;
}

txSlot* fxNewSlot(txMachine* the) 
{
	txSlot* aSlot;
	txBoolean once = 1;
	
#if mxStress
	if (gxStress) {
		fxCollect(the, 1);
		once = 0;
	}
#endif
again:
	aSlot = the->freeHeap;
	if (aSlot) {
		the->freeHeap = aSlot->next;
		aSlot->next = C_NULL;
		aSlot->ID = XS_NO_ID;
		aSlot->flag = XS_NO_FLAG;
		aSlot->kind = XS_UNDEFINED_KIND;
		the->currentHeapCount++;
		if (the->peakHeapCount < the->currentHeapCount)
			the->peakHeapCount = the->currentHeapCount;
		return aSlot;
	}
	if (once) {
		txBoolean wasThrashing = ((the->collectFlag & XS_TRASHING_FLAG) != 0), isThrashing;

		fxCollect(the, 0);

		isThrashing = ((the->collectFlag & XS_TRASHING_FLAG) != 0);
		if (wasThrashing && isThrashing)
			fxGrowHeap(the, !(the->collectFlag & XS_SKIPPED_COLLECT_FLAG) ? the->minimumHeapCount : 64);

		once = 0;
	}
	else
		fxGrowHeap(the, !(the->collectFlag & XS_SKIPPED_COLLECT_FLAG) ? the->minimumHeapCount : 64);
	goto again;
	return C_NULL;
}

void* fxRenewChunk(txMachine* the, void* theData, txSize theSize)
{
	txByte* aData = ((txByte*)theData) - sizeof(txChunk);
	txChunk* aChunk = (txChunk*)aData;
	txBlock* aBlock = the->firstBlock;
	theSize = mxRoundSize(theSize) + sizeof(txChunk); 
	
	if (aChunk->size == theSize) {
	#ifdef mxNever
		gxRenewChunkCases[0]++;
	#endif
		return theData;
	}
	aData += aChunk->size;
	theSize -= aChunk->size;
	while (aBlock) {
		if (aBlock->current == aData) {
			if (aData + theSize <= aBlock->limit) {
				aBlock->current += theSize;
				aChunk->size += theSize;
				the->currentChunksSize += theSize;
				if (the->peakChunksSize < the->currentChunksSize)
					the->peakChunksSize = the->currentChunksSize;
			#ifdef mxNever
				gxRenewChunkCases[1]++;
			#endif
				return theData;
			}
			else {
			#ifdef mxNever
				gxRenewChunkCases[3]++;
			#endif
				return C_NULL;
			}
		}
		aBlock = aBlock->nextBlock;
	}
	if (theSize < 0) {
		the->currentChunksSize += theSize;
		if (the->peakChunksSize < the->currentChunksSize)
			the->peakChunksSize = the->currentChunksSize;
		aChunk->size += theSize;
		aData += theSize;
		((txChunk*)aData)->size = -theSize;
		((txChunk*)aData)->temporary = C_NULL;
	#ifdef mxNever
		gxRenewChunkCases[2]++;
	#endif
		return theData;
	}
#ifdef mxNever
	gxRenewChunkCases[3]++;
#endif
	return C_NULL;
}

void fxShare(txMachine* the)
{
	txSlot* aSlot;
	txSlot* bSlot;
	txSlot* cSlot;

	aSlot = the->firstHeap;
	while (aSlot) {
		bSlot = aSlot + 1;
		cSlot = aSlot->value.reference;
		while (bSlot < cSlot) {
			if (bSlot->kind == XS_INSTANCE_KIND)
				bSlot->flag |= XS_SHARED_FLAG; 
			bSlot++;
		}
		aSlot = aSlot->next;
	}
}

void fxSweep(txMachine* the)
{
	txSize aTotal;
	txBlock* aBlock;
	txByte* mByte;
	txByte* nByte;
	txByte* pByte;
	txSize aSize;
	txByte** aCodeAddress;
	txSlot* aSlot;
	txSlot* bSlot;
	txSlot* cSlot;
	txSlot* freeSlot;

#ifdef mxNever
	startTime(&gxSweepChunkTime);
#endif

	aTotal = 0;
	aBlock = the->firstBlock;
	while (aBlock) {
		mByte = ((txByte*)aBlock) + sizeof(txBlock);
		nByte = aBlock->current;
		pByte = mByte;
		while (mByte < nByte) {
			aSize = ((txChunk*)mByte)->size;
			if (aSize & mxChunkFlag) {
				aSize &= ~mxChunkFlag;
				((txChunk*)mByte)->size = aSize;
				((txChunk*)mByte)->temporary = pByte;
				pByte += aSize;
				aTotal += aSize;
			}
			mByte += aSize;
		}	
		aBlock->temporary = pByte;
		aBlock = aBlock->nextBlock;
	}
	the->currentChunksSize = aTotal;

	aCodeAddress = &(the->code);
	aSlot = the->frame;
	while (aSlot) {
		mxCheck(the, aSlot->kind == XS_FRAME_KIND);
		if ((aSlot->flag & XS_C_FLAG) == 0) {
			bSlot = (aSlot + 2)->value.reference->next;
			if (bSlot->kind == XS_CODE_KIND) {
				mByte = bSlot->value.code;
				pByte = (txByte*)(((txChunk*)(mByte - sizeof(txChunk)))->temporary);
				if (pByte) {
					pByte += sizeof(txChunk);
					aSize = pByte - mByte;
					*aCodeAddress = *aCodeAddress + aSize;
					mxCheck(the, (aSlot - 1)->kind == XS_ROUTE_KIND);
					aCodeAddress = &((aSlot - 1)->value.route.code);
					if (*aCodeAddress)
						*aCodeAddress = *aCodeAddress + aSize;
				}
			}
		}
		else {
			mByte = *aCodeAddress;
			if (mByte) {
				pByte = (txByte*)(((txChunk*)(mByte - sizeof(txChunk)))->temporary);
				if (pByte)
					*aCodeAddress = pByte + sizeof(txChunk);
			}
		}
		aCodeAddress = &(aSlot->value.frame.code);
		aSlot = aSlot->next;
	}
	aSlot = the->stack;
	while (aSlot < the->stackTop) {
		fxSweepValue(the, aSlot);
		aSlot++;
	}
	aSlot = the->cRoot;
	while (aSlot) {
		fxSweepValue(the, aSlot);
		aSlot = aSlot->next;
	}

#ifdef mxNever
	stopTime(&gxSweepChunkTime);
	startTime(&gxSweepSlotTime);
#endif
	
	aTotal = 0;
	freeSlot = C_NULL;
	aSlot = the->firstHeap;
	while (aSlot) {
		bSlot = aSlot + 1;
		cSlot = aSlot->value.reference;
		while (bSlot < cSlot) {
			if (bSlot->flag & XS_MARK_FLAG) {
				bSlot->flag &= ~XS_MARK_FLAG; 
				fxSweepValue(the, bSlot);
				aTotal++;
			}
			else {
				if ((bSlot->kind == XS_HOST_KIND) && (bSlot->value.host.variant.destructor)) {
					if (bSlot->flag & XS_HOST_HOOKS_FLAG) {
						if (bSlot->value.host.variant.hooks->destructor)
							(*(bSlot->value.host.variant.hooks->destructor))(bSlot->value.host.data);
					}
					else
						(*(bSlot->value.host.variant.destructor))(bSlot->value.host.data);
				}
			#if mxFill
				c_memset(bSlot, 0xFF, sizeof(txSlot));
			#endif
				bSlot->kind = XS_UNDEFINED_KIND;
				bSlot->next = freeSlot;
				freeSlot = bSlot;
			}
			bSlot++;
		}
		aSlot = aSlot->next;
	}
	the->currentHeapCount = aTotal;
	the->freeHeap = freeSlot;
	
#ifdef mxNever
	stopTime(&gxSweepSlotTime);
	startTime(&gxCompactChunkTime);
#endif

	aBlock = the->firstBlock;
	while (aBlock) {
		mByte = ((txByte*)aBlock) + sizeof(txBlock);
		nByte = aBlock->current;
		while (mByte < nByte) {
			aSize = ((txChunk*)mByte)->size;
			if ((pByte = ((txChunk*)mByte)->temporary)) {
				((txChunk*)mByte)->temporary = C_NULL;
				if (pByte != mByte)
					c_memmove(pByte, mByte, aSize);
			}
			mByte += aSize;
		}	
	#if mxFill
		c_memset(aBlock->temporary, 0xFF, aBlock->current - aBlock->temporary);
	#endif
		aBlock->current = aBlock->temporary;
		aBlock->temporary = C_NULL;
		aBlock = aBlock->nextBlock;
	}
	
#ifdef mxNever
	stopTime(&gxCompactChunkTime);
#endif
}

void fxSweepValue(txMachine* the, txSlot* theSlot)
{
	txSlot* aSlot;
	txByte* data;
#define mxSweepChunk(_THE_DATA, _THE_DATA_TYPE) \
	if ((data = (txByte*)(((txChunk*)(((txByte*)(_THE_DATA)) - sizeof(txChunk)))->temporary))) \
		((_THE_DATA)) = (_THE_DATA_TYPE)(data + sizeof(txChunk))

	switch (theSlot->kind) {
	case XS_REGEXP_KIND:
		mxSweepChunk(theSlot->value.regexp.code, void*);
		mxSweepChunk(theSlot->value.regexp.offsets, txInteger*);
		break;
	case XS_STRING_POINTER:
	case XS_STRING_CONCAT:
	case XS_STRING_CONCATBY:
	case XS_STRING_KIND:
	case XS_STRING:
		mxSweepChunk(theSlot->value.string, txString);
		break;
	case XS_ARRAY_KIND:
		if ((aSlot = theSlot->value.array.address)) {
			txInteger aLength = theSlot->value.array.length;
			while (aLength) {
				fxSweepValue(the, aSlot);
				aSlot++;
				aLength--;
			}
			mxSweepChunk(theSlot->value.array.address, txSlot*);
		}
		break;
	case XS_GLOBAL_KIND:
		mxSweepChunk(theSlot->value.global.cache, txSlot**);
		mxSweepChunk(theSlot->value.global.sandboxCache, txSlot**);
		break;
	case XS_CODE_KIND:
		mxSweepChunk(theSlot->value.code, txByte*);
		break;
	case XS_SYMBOL_KIND:
		if (theSlot->value.symbol.string)
			mxSweepChunk(theSlot->value.symbol.string, txString);
		break;
	case XS_PREFIX_KIND:
		if (theSlot->value.prefix.string)
			mxSweepChunk(theSlot->value.prefix.string, txString);
		break;
	case XS_ATTRIBUTE_RULE:
	case XS_DATA_RULE:
	case XS_PI_RULE:
	case XS_EMBED_RULE:
	case XS_JUMP_RULE:
	case XS_REFER_RULE:
	case XS_REPEAT_RULE:
	case XS_ERROR_RULE:
		if (theSlot->value.rule.data)
			mxSweepChunk(theSlot->value.rule.data, txRuleData *);
		break;
	case XS_ATTRIBUTE_PATTERN:
	case XS_DATA_PATTERN:
	case XS_PI_PATTERN:
	case XS_EMBED_PATTERN:
	case XS_JUMP_PATTERN:
	case XS_REFER_PATTERN:
	case XS_REPEAT_PATTERN:
		mxSweepChunk(theSlot->value.pattern, txPatternData *);
		break;
	case XS_CLOSURE_KIND:
		if (theSlot->value.closure.symbolMap)
			mxSweepChunk(theSlot->value.closure.symbolMap, txByte*);
		break;
	}
}
