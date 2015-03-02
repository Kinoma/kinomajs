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
#ifndef __KPL_UIEVENTS_H__
#define __KPL_UIEVENTS_H__

#include "FskErrors.h"
#include "KplRectangle.h"
#include "KplTime.h"

#ifdef __cplusplus
extern "C" {
#endif

struct KplUIEventRecord {
	UInt32			eventID;
	KplTimeRecord	eventTime;
	union {
		struct {
			SInt32	keyCode;
			UInt32	modifiers;
		} key;
		struct { 
			SInt32	x;
			SInt32	y;
			UInt32	index;
		} mouse;
		struct {
			SInt32	width;
			SInt32	height;
		} resize;
		struct {
			KplRectangleRecord	area;
		} update;
		struct {
			UInt32	id;
			UInt32	arity;
			double values[6];
		} sensor;
	};
};
typedef struct KplUIEventRecord KplUIEventRecord;
typedef KplUIEventRecord *KplUIEvent;

typedef FskErr (*KplUIEventCallback)(KplUIEvent event, void *refcon);

enum {
	kKplUIEventKeyDown = 1,
	kKplUIEventKeyRepeat,
	kKplUIEventKeyUp,

	kKplUIEventMouseHover,
	kKplUIEventMouseDown,
	kKplUIEventMouseDrag,
	kKplUIEventMouseUp,
	
	kKplUIEventScreenActive,
	kKplUIEventScreenInactive,
	kKplUIEventScreenUpdate,
	kKplUIEventScreenResize,
	
	kKplUIEventSensorBegan,
	kKplUIEventSensorChanged,
	kKplUIEventSensorEnded
};

FskErr KplUIEventNew(KplUIEvent *event, UInt32 eventID, KplTime eventTime);
FskErr KplUIEventDispose(KplUIEvent event);

FskErr KplUIEventSend(KplUIEvent event);

FskErr KplUIEventSetCallback(KplUIEventCallback proc, void *refcon);

#ifdef __cplusplus
}
#endif

#endif // __KPL_UIEVENTS_H__
