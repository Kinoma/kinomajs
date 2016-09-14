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
#ifndef __K5_MUTEX_H__
#define __K5_MUTEX_H__

#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

#define xSemaphoreTakeFromISR( xSemaphore, pxHigherPriorityTaskWoken )	xQueueReceiveFromISR( ( xQueueHandle ) ( xSemaphore ), NULL, ( pxHigherPriorityTaskWoken ) )

#include "88mw300.h"

#define isISRContext() (__get_IPSR() != 0)

static inline void MutexLock(xSemaphoreHandle handle)
{
	if (isISRContext()) {
		signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreTakeFromISR(handle, &xHigherPriorityTaskWoken);
	} else {
		/* Wait Forever */
		while (!xSemaphoreTake(handle, 1000));
	}
}

static inline void MutexUnlock(xSemaphoreHandle handle)
{
	if (isISRContext()) {
		signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(handle, &xHigherPriorityTaskWoken);
	} else {
		xSemaphoreGive(handle);
	}
}

#endif
