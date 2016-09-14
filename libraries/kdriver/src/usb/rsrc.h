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

#include <stdint.h>

typedef struct {
	uint32_t size;
	uint32_t word;
	uint8_t *pool;
	void **addresses;
	int freeIndex;
} DcdResourcePoolDef_t;

#define DcdDefineResourcePool(name, size, word) \
uint8_t rsrcPool_##name[size * word]; \
void *addresses_##name[size]; \
DcdResourcePoolDef_t rsrcPool_def_##name = \
{ (size), (word), (rsrcPool_##name), (addresses_##name), 0 }

#define DcdDefineResourcePoolAligned(name, size, word, alignment) \
uint8_t rsrcPool_##name[size * word] __attribute__ ((aligned (alignment))); \
void *addresses_##name[size]; \
DcdResourcePoolDef_t rsrcPool_def_##name = \
{ (size), (word), (rsrcPool_##name), (addresses_##name), 0 }

#define DcdGetResourcePool(name) \
&rsrcPool_def_##name

void DcdInitResourcePool(DcdResourcePoolDef_t *pool);

void *DcdAllocResource(DcdResourcePoolDef_t *pool);

void DcdDisposeResource(DcdResourcePoolDef_t *pool, void *resource);
