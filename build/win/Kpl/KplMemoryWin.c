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
#define _WIN32_WINNT 0x0500
#define __FSKMEMORY_KPL_PRIV__
#include "Fsk.h"
#include "KplMemory.h"
#include "stdlib.h"

FskErr KplMemPtrNew(UInt32 size, KplMemPtr *newMemory)
{
	*newMemory = (KplMemPtr)malloc(size);
	return NULL == *newMemory ? kFskErrMemFull : kFskErrNone;
}

FskErr KplMemPtrDispose(void *ptr)
{
	if (ptr)
		free(ptr);
	return kFskErrNone;
}

FskErr KplMemPtrRealloc(UInt32 size, KplMemPtr *newMemory)
{
	KplMemPtr result;
	result = (KplMemPtr)realloc(*newMemory, size);
	*newMemory = result;
	return (NULL == result && 0 != size) ? kFskErrMemFull : kFskErrNone;
}

FskErr KplMemPtrNewClear(UInt32 size, KplMemPtr *newMemory)
{
	*newMemory = (KplMemPtr)calloc(1, size);
	return NULL == *newMemory ? kFskErrMemFull : kFskErrNone;
}

void KplNotificationInitialize()
{
}

void KplMemoryInitialize()
{
}

void KplMemoryTerminate()
{
}

