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

#include "rsrc.h"

#include <stdint.h>
#include <stdlib.h>

void DcdInitResourcePool(DcdResourcePoolDef_t *pool)
{
	int i;

	pool->freeIndex = (pool->size - 1);
	for (i = 0; i < pool->size; i++) {
		pool->addresses[i] = &pool->pool[i * pool->word];
	}
}

void *DcdAllocResource(DcdResourcePoolDef_t *pool)
{
	if (pool->freeIndex < 0) {
		return NULL;
	}
	return pool->addresses[pool->freeIndex--];
}

void DcdDisposeResource(DcdResourcePoolDef_t *pool, void *resource)
{
	/* Return back to address pool */
	pool->addresses[++pool->freeIndex] = resource;
}
