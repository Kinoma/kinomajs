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
#ifndef __FSKLIST__
#define __FSKLIST__

#include "FskSynchronization.h"
#include "FskUtilities.h"

// Single linked list

typedef void *FskList;
typedef void *FskListElement;

#ifdef __cplusplus
extern "C" {
#endif

#define FskListPrepend(list, element) FskListPrepend_((FskList *)(void *)(list), element)
#define FskListAppend(list, element) FskListAppend_((FskList *)(void *)(list), element)
#define FskListInsertAfter(list, element, after) FskListInsertAfter_((FskList *)(void *)(list), element, after)
#define FskListInsertSorted(list, element, compareFn) FskListInsertSorted_((FskList *)(void *)(list), element, compareFn)
#define FskListRemove(list, element) FskListRemove_((FskList *)(void *)(list), element)
#define FskListRemoveFirst(list) FskListRemoveFirst_((FskList *)(void *)(list))
#define FskListRemoveLast(list) FskListRemoveLast_((FskList *)(void *)(list))

FskAPI(void) FskListPrepend_(FskList *list, FskListElement element);
FskAPI(void) FskListAppend_(FskList *list, FskListElement element);
FskAPI(void) FskListInsertAfter_(FskList *list, FskListElement element, FskListElement after);
FskAPI(void) FskListInsertSorted_(FskList *list, FskListElement element, FskCompareFunction compareFn);
FskAPI(Boolean) FskListRemove_(FskList *list, FskListElement element);
FskAPI(FskListElement) FskListRemoveFirst_(FskList *list);
FskAPI(FskListElement) FskListRemoveLast_(FskList *list);
FskAPI(FskListElement) FskListGetPrevious(FskList list, FskListElement element);
FskAPI(FskListElement) FskListGetNext(FskList list, FskListElement element);
FskAPI(UInt32) FskListCount(FskList list);
FskAPI(Boolean) FskListContains(FskList list, FskListElement element);

// double linked list

typedef struct FskListDoubleElementRecord FskListDoubleElementRecord;
typedef struct FskListDoubleElementRecord *FskListDoubleElement;

typedef struct FskListDoubleRecord {
    FskListDoubleElement    first;
    FskListDoubleElement    last;
} FskListDoubleRecord, *FskListDouble;

struct FskListDoubleElementRecord {
    FskListDoubleElement    next;
    FskListDoubleElement    prev;
};

FskAPI(void) FskListDoublePrepend(FskListDouble list, FskListDoubleElement element);
FskAPI(FskListDoubleElement) FskListDoubleRemove(FskListDouble list, FskListDoubleElement element);
#define FskListDoubleRemoveFirst(list) FskListDoubleRemove(list, (list)->first)
#define FskListDoubleRemoveLast(list) FskListDoubleRemove(list, (list)->last)

    // Singly linked list protected by a mutex

typedef struct FskListMutexRecord {
	FskList		list;
	FskMutex	mutex;
#if SUPPORT_INSTRUMENTATION
	char		*name;
#endif
} FskListMutexRecord, *FskListMutex;

FskAPI(FskErr) FskListMutexNew(FskListMutex *list, const char* name);
FskAPI(FskErr) FskListMutexDispose(FskListMutex list);

FskAPI(void) FskListMutexPrepend(FskListMutex list, FskListElement element);
FskAPI(void) FskListMutexAppend(FskListMutex list, FskListElement element);
FskAPI(Boolean) FskListMutexRemove(FskListMutex list, FskListElement element);
FskAPI(FskListElement) FskListMutexRemoveFirst(FskListMutex list);
FskAPI(FskListElement) FskListMutexRemoveLast(FskListMutex list);
FskAPI(FskListElement) FskListMutexGetNext(FskListMutex list, FskListElement element);
FskAPI(UInt32) FskListMutexCount(FskListMutex list);
FskAPI(Boolean) FskListMutexContains(FskListMutex list, FskListElement element);

FskErr FskListMutexNew_uninstrumented(FskListMutex *list, const char *name);
FskErr FskListMutexDispose_uninstrumented(FskListMutex list);
void FskListMutexAcquireMutex_uninstrumented(FskListMutex mtxList);
void FskListMutexReleaseMutex_uninstrumented(FskListMutex mtxList);

#define FskListMutexIsEmpty(l) ((NULL == l) || (NULL == ((FskListMutex)l)->list)) ? true : false
#define FskListMutexAcquireMutex(list) FskMutexAcquire((list)->mutex)
#define FskListMutexReleaseMutex(list) FskMutexRelease((list)->mutex)

#ifdef __cplusplus
	}
#endif

#endif
