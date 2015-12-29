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
#include <stdio.h>
#include "mc_env.h"

#define fxPop() (*(the->stack++))
#define fxPush(_SLOT) (*(--the->stack) = (_SLOT))

typedef struct {
	unsigned int maximum;
	unsigned int current;
	unsigned int peak;
	unsigned int nblocks;
} txMemoryUse;

#if mxMC
extern unsigned _heap2_start, _heap2_end;	/* defined in .ld */

static int xsCountFreeSlots(txMachine *the)
{
	uint8_t *map;
	uint8_t *heap_ptr = (uint8_t *)&_heap2_start;
	uint8_t *heap_pend = (uint8_t *)&_heap2_end;
	int nSlotBlocks = (heap_pend - heap_ptr) / 2048;
	int freeSlotBlocks = 0;

	if ((map = mc_calloc(1, nSlotBlocks)) != NULL) {
		txSlot *aSlot;
		for (aSlot = the->freeHeap; aSlot != NULL; aSlot = aSlot->next) {
			if ((uint8_t *)aSlot >= heap_ptr && (uint8_t *)aSlot < heap_pend)
				map[((uint8_t *)aSlot - heap_ptr) / 2048]++;
		}
		freeSlotBlocks = 0;
		while (--nSlotBlocks >= 0) {
			if (map[nSlotBlocks] == (2048 /16) - 1)
				freeSlotBlocks++;
			/*
			fprintf(stderr, "%d:", map[nSlotBlocks]);
			if ((nSlotBlocks % 20) == 0)
				fprintf(stderr, "\n");
			*/
		}
		mc_free(map);
	}
	return freeSlotBlocks;
}
#else
#define xsCountFreeSlots(the)	(0)
#endif

void xsReportMemoryUse(txMachine *the, txMemoryUse *slot, txMemoryUse *chunk)
{
	txBlock* aBlock;
	int n = 0;

	for (aBlock = the->firstBlock; aBlock; aBlock = aBlock->nextBlock)
		n++;
	if (chunk != NULL) {
		chunk->maximum = the->maximumChunksSize;
		chunk->current = the->currentChunksSize;
		chunk->peak = the->peakChunksSize;
		chunk->nblocks = n;
	}
	else {
		fxReport(the, "# Chunk allocation: reserved %ld used %ld peak %ld bytes, %d blocks\n",
			 the->maximumChunksSize, the->currentChunksSize, the->peakChunksSize, n);
	}
	n = xsCountFreeSlots(the);
	if (slot != NULL) {
		slot->maximum = the->maximumHeapCount * sizeof(txSlot);
		slot->current = the->currentHeapCount * sizeof(txSlot);
		slot->peak = the->peakHeapCount * sizeof(txSlot);
		slot->nblocks = n;
	}
	else {
		fxReport(the, "# Slot allocation: reserved %ld used %ld peak %ld bytes, %d free\n",
			 the->maximumHeapCount * sizeof(txSlot),
			 the->currentHeapCount * sizeof(txSlot),
			 the->peakHeapCount * sizeof(txSlot),
			 n);
	}
}

int xsStartDebug(txMachine *the, const char *host)
{
#ifdef mxDebug
	mc_env_set_default("XSBUG_HOST", host);
	mc_env_store(NULL);
	fxLogin(the);
	return fxIsConnected(the);
#endif
}

void xsStopDebug(txMachine *the)
{
#ifdef mxDebug
	fxLogout(the);
#endif
}

void _xsNewInstanceOf(txMachine *the, txSlot *res, txSlot *proto)
{
	fxOverflow(the, -1, C_NULL, 0);
	fxPush(*proto);
	fxNewInstanceOf(the);
	*res = fxPop();
}

txBoolean _xsIsInstanceOf(txMachine *the, txSlot *v, txSlot *proto)
{
	fxOverflow(the, -2, C_NULL, 0);
	fxPush(*proto);
	fxPush(*v);
	return fxIsInstanceOf(the);
}

txBoolean _xsHas(txMachine *the, txSlot *self, txInteger id)
{
	fxOverflow(the, -1, C_NULL, 0);
	fxPush(*self);
	return fxHasID(the, id);
}

txBoolean _xsHasOwn(txMachine *the, txSlot *self, txInteger id)
{
	fxOverflow(the, -1, C_NULL, 0);
	fxPush(*self);
	return fxHasOwnID(the, id);
}

void _xsGet(txMachine *the, txSlot *res, txSlot *self, txInteger id)
{
	fxOverflow(the, -1, C_NULL, 0);
	fxPush(*self);
	fxGetID(the, id);
	*res = fxPop();
}

void _xsGetAt(txMachine *the, txSlot *res, txSlot *self, txSlot *at)
{
	fxOverflow(the, -2, C_NULL, 0);
	fxPush(*self);
	fxPush(*at);
	fxGetAt(the);
	*res = fxPop();
}

void _xsSet(txMachine *the, txSlot *self, txInteger id, txSlot *v)
{
	fxOverflow(the, -2, C_NULL, 0);
	fxPush(*v);
	fxPush(*self);
	fxSetID(the, id);
	the->stack++;
}

void _xsSetAt(txMachine *the, txSlot *self, txSlot *at , txSlot *v)
{
	fxOverflow(the, -3, C_NULL, 0);
	fxPush(*v);
	fxPush(*self);
	fxPush(*at);
	fxSetAt(the);
	the->stack++;
}

void _xsDelete(txMachine *the, txSlot *self, txInteger id)
{
	fxOverflow(the, -1, C_NULL, 0);
	fxPush(*self);
	fxDeleteID(the, id);
}

void _xsDeleteAt(txMachine *the, txSlot *self, txSlot *at)
{
	fxOverflow(the, -2, C_NULL, 0);
	fxPush(*self);
	fxPush(*at);
	fxDeleteAt(the);
}

void _xsCall(txMachine *the, txSlot *res, txSlot *self, txInteger id, ...)
{
	va_list ap;
	int n;
	txSlot *v;

	va_start(ap, id);
	for (n = 0; va_arg(ap, txSlot *) != NULL; n++)
		;
	va_end(ap);
	fxOverflow(the, -(n+2), C_NULL, 0);
	va_start(ap, id);
	while ((v = va_arg(ap, txSlot *)) != NULL)
		fxPush(*v);
	va_end(ap);
	fxInteger(the, --the->stack, n);
	fxPush(*self);
	fxCallID(the, id);
	if (res != NULL)
		*res = fxPop();
	else
		the->stack++;
}

void _xsNew(txMachine *the, txSlot *res, txSlot *self, txInteger id, ...)
{
	va_list ap;
	int n;
	txSlot *v;

	va_start(ap, id);
	for (n = 0; va_arg(ap, txSlot *) != NULL; n++)
		;
	va_end(ap);
	fxOverflow(the, -(n+2), C_NULL, 0);
	va_start(ap, id);
	while ((v = va_arg(ap, txSlot *)) != NULL)
		fxPush(*v);
	va_end(ap);
	fxInteger(the, --the->stack, n);
	fxPush(*self);
	fxNewID(the, id);
	*res = fxPop();
}

txBoolean _xsTest(txMachine *the, txSlot *v)
{
	fxOverflow(the, -1, C_NULL, 0);
	fxPush(*v);
	return fxRunTest(the);
}

txInteger fxIncrementalVars(txMachine* the, txInteger theCount)
{
	txSlot* aStack = the->frame - 1;
	txInteger aVar;

	if (aStack - aStack->value.integer != the->stack)
		mxSyntaxError("C: xsVars: too late");
	fxOverflow(the, theCount, C_NULL, 0);
	aVar = aStack->value.integer;
	aStack->value.integer += theCount;
	if (theCount > 0) {
		while (theCount) {
			mxPushUndefined();
			theCount--;
		}
	}
	else
		the->stack -= theCount;
	return aVar;
}

txBoolean xsGetCollectFlag(txMachine *the)
{
	return the->collectFlag;
}
