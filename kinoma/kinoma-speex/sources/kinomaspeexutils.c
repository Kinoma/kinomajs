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
#include "FskMemory.h"
#include "config.h"

void *speex_alloc (int size)
{
	void *p;
	FskErr err;
	err = FskMemPtrNewClear(size, (FskMemPtr*)&p);
	if (err) return NULL;
	return p;
}

void *speex_alloc_scratch (int size)
{
	void *p;
	FskErr err;
	err = FskMemPtrNewClear(size, (FskMemPtr*)&p);
	if (err) return NULL;
	return p;
}

void *speex_realloc (void *ptr, int size)
{
	FskErr err;
	err = FskMemPtrRealloc(size, (FskMemPtr*)ptr);
	if (err) return NULL;
	return ptr;
}

void speex_free (void *ptr)
{
	FskMemPtrDispose(ptr);
}

void speex_free_scratch (void *ptr)
{
	FskMemPtrDispose(ptr);
}





