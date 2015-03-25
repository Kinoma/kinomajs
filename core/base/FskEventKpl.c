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
#include "FskEventKpl.h"

FskErr KplUIEventToFskEvent(KplUIEvent kplEvent, FskEvent *fskEvent)
{
	FskEvent fskEventOut = NULL;
	FskErr err;
	UInt32 eventCode;
	
	switch(kplEvent->eventID) {
		case kKplUIEventMouseDown:
		case kKplUIEventMouseUp:
			eventCode = (kKplUIEventMouseDown == kplEvent->eventID ? kFskEventMouseDown : kFskEventMouseUp);
			err = FskEventNew(&fskEventOut, eventCode, NULL, kFskEventModifierNotSet);
			if (0 == err) {
				FskPointAndTicksRecord pat;
				pat.pt.x = kplEvent->mouse.x;
				pat.pt.y = kplEvent->mouse.y;
				pat.ticks = (UInt32)(kplEvent->eventTime.seconds * 1000) + (UInt32)(kplEvent->eventTime.useconds / 1000);
				pat.index = kplEvent->mouse.index;
				FskEventParameterAdd(fskEventOut, kFskEventParameterMouseLocation, sizeof(pat), &pat);
			}
			break;
		case kKplUIEventMouseDrag:
			{
				UInt32 index = 0;
				eventCode = kFskEventMouseMoved;
				err = FskEventNew(&fskEventOut, eventCode, NULL, kFskEventModifierNotSet);
				if (0 == err) {
					FskPointAndTicksRecord pat;
					pat.pt.x = kplEvent->mouse.x;
					pat.pt.y = kplEvent->mouse.y;
					pat.ticks = (UInt32)(kplEvent->eventTime.seconds * 1000) + (UInt32)(kplEvent->eventTime.useconds / 1000);
					pat.index = kplEvent->mouse.index;
					FskEventParameterAdd(fskEventOut, kFskEventParameterMouseLocation, sizeof(pat), &pat);
					FskEventParameterAdd(fskEventOut, kFskEventParameterCommand, sizeof(index), &index);
				}
			}
			break;
		case kKplUIEventKeyDown:
		case kKplUIEventKeyUp:
			eventCode = (kKplUIEventKeyDown == kplEvent->eventID ? kFskEventKeyDown : kFskEventKeyUp);
			err = FskEventNew(&fskEventOut, eventCode, NULL, kplEvent->key.modifiers);
			if (0 == err) {
				char key[2];

				if (kplEvent->key.keyCode > 0xF0000) {
					UInt32 functionKey = kplEvent->key.keyCode & 0xFFFF;
					FskEventParameterAdd(fskEventOut, kFskEventParameterFunctionKey, sizeof(functionKey), &functionKey);
				}
				else {
					key[0] = (char)kplEvent->key.keyCode;
					key[1] = 0;
					FskEventParameterAdd(fskEventOut, kFskEventParameterKeyUTF8, 2, key);
				}
			}
			break;
		case kKplUIEventScreenActive:
			eventCode = (kKplUIEventScreenActive == kplEvent->eventID ? kFskEventWindowActivated : kFskEventWindowDeactivated);
			err = FskEventNew(&fskEventOut, eventCode, NULL, kFskEventModifierNotSet);
			break;
			
		case kKplUIEventSensorBegan:
			err = FskEventNew(&fskEventOut, kFskEventButton, NULL, kFskEventModifierShift);
			goto kKplUIEventSensorAll;
		case kKplUIEventSensorChanged:
			err = FskEventNew(&fskEventOut, kFskEventButton, NULL, kFskEventModifierControl);
			goto kKplUIEventSensorAll;
		case kKplUIEventSensorEnded:
			err = FskEventNew(&fskEventOut, kFskEventButton, NULL, kFskEventModifierAlt);
		kKplUIEventSensorAll:
			if (0 == err) {
				err = FskEventParameterAdd(fskEventOut, kFskEventParameterCommand, sizeof(UInt32), &kplEvent->sensor.id);
				if (0 == err) {
					if (kplEvent->sensor.arity) {
						err = FskEventParameterAdd(fskEventOut, kFskEventParameterApplicationParam1, kplEvent->sensor.arity * sizeof(double), kplEvent->sensor.values);
					}
				}
			}
			break;
			
		default:
			err = kFskErrUnimplemented;
			break;
	}
	
	if (err) {
		FskEventDispose(fskEventOut);
		fskEventOut = NULL;
	}
	*fskEvent = fskEventOut;
	
	return err;
}
