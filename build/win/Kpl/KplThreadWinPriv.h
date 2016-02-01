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
#ifndef __KPLTHREAD_PRIV_H__
#define __KPLTHREAD_PRIV_H__

#include "windows.h"
#include "KplThread.h"
#include "KplTime.h"
#include "FskList.h"

#define SUPPORT_TIMER_THREAD 1

typedef struct {
	SInt32		x;
	SInt32		y;
} KplPointRecord, *KplPoint;

typedef struct {
	KplPointRecord	pt;
	UInt32			ticks;
} KplPointAndTicksRecord, *KplPointAndTicks;

struct KplThreadRecord {
	KplThread					next;
	
	HANDLE						handle;
	unsigned int				id;
	HWND						window;
	char						*name;

	UInt32						haveMouseMove;
	MSG							mouseMove;
	KplPointAndTicksRecord		points[50];
	UInt32						nextTicks;

#if SUPPORT_WAITABLE_TIMER
	HANDLE						waitableTimer;
#elif SUPPORT_TIMER_THREAD
	HANDLE						waitableTimer;
	KplTimeRecord				nextCallbackTime;
	Boolean						timerSignaled;
#else
	UINT_PTR					timer;
#endif
	UInt32						flags;
	
	KplThreadProc				clientProc;
	void						*clientRefCon;
	
	FskList						timerCallbacks;
	FskList						suspendedTimerCallbacks;
};

extern FskListMutex gKplThreads;

#endif
