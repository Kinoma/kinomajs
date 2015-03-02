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
#define __FSKWINDOW_PRIV__

#include "FskMemory.h"
#include "FskThread.h"

#include "string.h"

#if TARGET_OS_WIN32
	#include "shlobj.h"
	#define COBJMACROS
	#include "ntverp.h"
#endif

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
typedef struct tGenericListEl {
	struct tGenericListEl	*next;
} tGenericListEl;

typedef struct tOrderedListEl {
	struct tOrderedListEl	*next;
	UInt32	order;
} tOrderedListEl;

#if DEBUG_LISTS
int sCheckList(void *el, void **listHead) {
	int count = 0;

	tGenericListEl* cur = (tGenericListEl*)listHead;
	while (cur) {
		if (cur == el)
			count++;

		if(cur->next == cur)
			fprintf(stderr, "AAAACK!! - circular reference\n");

		cur = cur->next;
	}
	return count;
}
#endif

// ---------------------------------------------------------------------
void FskListPrepend_(FskList *listHead, FskListElement el)
{
	tGenericListEl *theEl = (tGenericListEl *)el, **head = (tGenericListEl**)listHead;

#if DEBUG_LISTS
	fprintf(stderr, "Add to Head - el:(%x), head (%x)\n", el, *listHead);
	if (sCheckList(el, listHead)) {
		fprintf(stderr, "************************* Already in list **************\n");
		return;
	}
#endif
	if (*listHead)
		theEl->next = *head;
	else
		theEl->next = NULL;
	*listHead = el;
#if DEBUG_LISTS
	if(theEl->next == theEl)
		fprintf(stderr, "AAAACK!! - circular reference\n");
#endif
}

// ---------------------------------------------------------------------
void FskListAppend_(FskList *listHead, FskListElement el)
{
	tGenericListEl *cur = (tGenericListEl*)*listHead;

#if DEBUG_LISTS
	fprintf(stderr, "Add to End - el:(%x), head (%x)\n", el, *listHead);
	if (sCheckList(el, listHead))
		fprintf(stderr, "************************* Already in list **************\n");
#endif

	((tGenericListEl*)el)->next = NULL;
	if (cur) {
		while (cur->next != NULL)
			cur = cur->next;
		cur->next = (tGenericListEl *)el;
	}
	else {
		*listHead = el;
	}
#if DEBUG_LISTS
	if(	((tGenericListEl*)el)->next == el)
		fprintf(stderr, "AAAACK!! - circular reference\n");
#endif
}

// ---------------------------------------------------------------------
void FskListInsertAfter_(FskList *listHead, FskListElement el, FskListElement after)
{
	if (after == NULL) {
		((tGenericListEl*)el)->next = (tGenericListEl *)*listHead;
		*listHead = el;
	}
	else {
		((tGenericListEl*)el)->next = ((tGenericListEl*)after)->next;
		((tGenericListEl*)after)->next = (tGenericListEl *)el;
	}
}

// ---------------------------------------------------------------------
void FskListInsertSorted_(FskList *listHead, FskListElement el, FskCompareFunction compareFn)
{
	tGenericListEl *cur;

	cur = (tGenericListEl *) *listHead;

	if (cur) {
		if (compareFn(el, cur) < 0) {
			FskListPrepend(listHead, el);
			return;
		}

		while (cur != NULL) {
			tGenericListEl *next = cur->next;

			if (next && compareFn(el, next) < 0) {
				FskListInsertAfter(listHead, el, cur);
				return;
			}
			cur = next;
		}
	}

	FskListAppend(listHead, el);
}

// ---------------------------------------------------------------------
FskListElement FskListRemoveLast_(FskList *listHead)
{
	tGenericListEl *cur = (tGenericListEl*)*listHead;
	FskListElement result = NULL;

#if DEBUG_LISTS
	fprintf(stderr, "Remove from End -head (%x)\n", *listHead);
#endif
	if (cur) {
		if (cur->next == NULL) {
			result = *listHead;
			*listHead = NULL;
		}
		else {
			while (cur->next->next != NULL)
				cur = cur->next;
			result = cur->next;
			cur->next = NULL;
		}
	}

	return result;
}

// ---------------------------------------------------------------------
FskListElement FskListRemoveFirst_(FskList *listHead)
{
	FskListElement result = *listHead;

#if DEBUG_LISTS
	fprintf(stderr, "Remove from Head - head (%x)\n", *listHead);
	if (!*listHead)
		fprintf(stderr, "nothing to remove\n");
#endif
	if (*listHead)
		*listHead = ((tGenericListEl*)(*listHead))->next;
	if (result)
		((tGenericListEl*)result)->next = NULL;
	return result;
}

// ---------------------------------------------------------------------
Boolean FskListRemove_(FskList *listHead, FskListElement el)
{
	tGenericListEl* cur = (tGenericListEl*)*listHead;
#if DEBUG_LISTS
	fprintf(stderr, "Remove  - el:(%x), head (%x)\n", el, *listHead);
#endif
	if (NULL == el)
		return false;

	if (cur == el) {
		*listHead = cur->next;
		((tGenericListEl*)el)->next = NULL;
		return true;
	}

	while (cur) {
		if (cur->next == el) {
			cur->next = ((tGenericListEl*)el)->next;
			((tGenericListEl*)el)->next = NULL;
			return true;
		}
		cur = cur->next;
	}
	return false;
}

// ---------------------------------------------------------------------
FskListElement FskListGetPrevious(FskList listHead, FskListElement el)
{
	tGenericListEl* cur = (tGenericListEl*)listHead;
	while (cur) {
		if (cur->next == el)
			return cur;
		cur = cur->next;
	}
	return NULL;
}

// ---------------------------------------------------------------------
FskListElement FskListGetNext(FskList listHead, FskListElement el)
{
	if (el)
		return ((tGenericListEl*)el)->next;
	else
		return (tGenericListEl*)listHead;
}

// ---------------------------------------------------------------------
UInt32 FskListCount(FskList listHead)
{
	UInt32 count = 0;

	tGenericListEl* cur = (tGenericListEl*)listHead;
	while (cur) {
		count++;
		cur = cur->next;
	}
	return count;
}

// ---------------------------------------------------------------------
Boolean FskListContains(FskList listHead, FskListElement el)
{
	tGenericListEl* cur = (tGenericListEl*)listHead;
	while (cur) {
		if (el == (void *)cur)
			return true;
		cur = cur->next;
	}

	return false;
}

/*
        doubly linked list
 */

void FskListDoublePrepend(FskListDouble list, FskListDoubleElement element)
{
    element->prev = NULL;
    if (list->first) {
        element->next = list->first;
        list->first->prev = element;
        list->first = element;
    }
    else {
        element->next = NULL;
        list->first = element;
        if (NULL == list->last)
            list->last = element;
    }
}

FskListDoubleElement FskListDoubleRemove(FskListDouble list, FskListDoubleElement element)
{
    if (NULL == element)
        return NULL;

    if (element == list->first) {
        if (element == list->last)
            list->first = list->last = NULL;
        else
            list->first = element->next;
    }
    else if (element == list->last)
        list->last = element->prev;
    else {
        element->prev->next = element->next;
        element->next->prev = element->prev;
    }

    element->next = element->prev = NULL;
    
    return element;
}

/*
	List with Mutex
*/


FskErr FskListMutexNew(FskListMutex *listOut, const char *name)
{
	FskListMutex	mtxList;
	FskErr			err;

	err = FskMemPtrNew(sizeof(FskListMutexRecord), &mtxList);
	if (err)
		return err;

	err = FskMutexNew(&mtxList->mutex, name);
	if (err) {
		FskMemPtrDispose(mtxList);
		return err;
	}

#if SUPPORT_INSTRUMENTATION
	mtxList->name = FskStrDoCopy_Untracked(name);
#endif
	mtxList->list = NULL;
	*listOut = mtxList;
	return kFskErrNone;
}

FskErr FskListMutexDispose(FskListMutex mtxList)
{
	if (mtxList) {
#if SUPPORT_INSTRUMENTATION
		if (mtxList->list)
			FskInstrumentedSystemPrintf("List: %s has elements remaining. They will be leaked.", mtxList->name);
		FskMemPtrDispose_Untracked(mtxList->name);
#endif
		FskMutexDispose(mtxList->mutex);
		FskMemPtrDispose(mtxList);
	}

	return kFskErrNone;
}

void FskListMutexPrepend(FskListMutex mtxList, FskListElement el)
{
	FskMutexAcquire(mtxList->mutex);
	FskListPrepend(&mtxList->list, el);
	FskMutexRelease(mtxList->mutex);
}

void FskListMutexAppend(FskListMutex mtxList, FskListElement el)
{
	FskMutexAcquire(mtxList->mutex);
	FskListAppend(&mtxList->list, el);
	FskMutexRelease(mtxList->mutex);
}

FskListElement FskListMutexRemoveLast(FskListMutex mtxList)
{
	FskListElement el;
	FskMutexAcquire(mtxList->mutex);
	el = FskListRemoveLast(&mtxList->list);
	FskMutexRelease(mtxList->mutex);
	return el;
}

FskListElement FskListMutexRemoveFirst(FskListMutex mtxList)
{
	FskListElement el;
	FskMutexAcquire(mtxList->mutex);
	el = FskListRemoveFirst(&mtxList->list);
	FskMutexRelease(mtxList->mutex);
	return el;
}

void FskListMutexRemove(FskListMutex mtxList, FskListElement el)
{
	FskMutexAcquire(mtxList->mutex);
	FskListRemove(&mtxList->list, el);
	FskMutexRelease(mtxList->mutex);
}

FskListElement FskListMutexGetNext(FskListMutex mtxList, FskListElement el)
{
	if (NULL == mtxList)
		return NULL;

	FskMutexAcquire(mtxList->mutex);
	if (NULL == el)
		el = mtxList->list;
	else
		el = *(void **)el;
	FskMutexRelease(mtxList->mutex);
	return el;
}

UInt32 FskListMutexCount(FskListMutex mtxList)
{
	int c = 0;
	if (mtxList) {
		FskMutexAcquire(mtxList->mutex);
		c = FskListCount(mtxList->list);
		FskMutexRelease(mtxList->mutex);
	}
	return c;
}

Boolean FskListMutexContains(FskListMutex mtxList, FskListElement el)
{
	Boolean b;
	FskMutexAcquire(mtxList->mutex);
	b = FskListContains(&mtxList->list, el);
	FskMutexRelease(mtxList->mutex);
	return b;
}


FskErr FskListMutexNew_uninstrumented(FskListMutex *listOut, const char *name)
{
	FskListMutex	mtxList;
	FskErr			err;

	err = FskMemPtrNew(sizeof(FskListMutexRecord), &mtxList);
	if (err)
		return err;

	err = FskMutexNew_uninstrumented(&mtxList->mutex, name);
	if (err) {
		FskMemPtrDispose(mtxList);
		return err;
	}

#if SUPPORT_INSTRUMENTATION
	mtxList->name = FskStrDoCopy(name);
#endif
	mtxList->list = NULL;
	*listOut = mtxList;
	return kFskErrNone;
}

FskErr FskListMutexDispose_uninstrumented(FskListMutex mtxList)
{
	if (mtxList) {
		FskMutexDispose_uninstrumented(mtxList->mutex);
#if SUPPORT_INSTRUMENTATION
		FskMemPtrDispose(mtxList->name);
#endif
		FskMemPtrDispose(mtxList);
	}

	return kFskErrNone;
}

void FskListMutexAcquireMutex_uninstrumented(FskListMutex mtxList)
{
	FskMutexAcquire_uninstrumented(mtxList->mutex);
}

void FskListMutexReleaseMutex_uninstrumented(FskListMutex mtxList)
{
	FskMutexRelease_uninstrumented(mtxList->mutex);
}


