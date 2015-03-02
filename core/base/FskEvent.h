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
#ifndef __FSKEVENT__
#define __FSKEVENT__

#include "FskRectangle.h"
#include "FskTime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	kFskEventNone = 0,
	kFskEventMouseDown = 1024,
	kFskEventMouseDoubleClick,
	kFskEventMouseUp,
	kFskEventMouseMoved,
	kFskEventMouseStillDown,
	kFskEventRightMouseDown,
	kFskEventRightMouseUp,
	kFskEventRightMouseStillDown,
	kFskEventMouseLeave,
	kFskEventMouseWheel,

	kFskEventKeyDown = 2048,
	kFskEventKeyUp,
	kFskEventButton,

	kFskEventWindowInitialize = 3072,
	kFskEventWindowClose,
	kFskEventWindowUpdate,
	kFskEventWindowResize,
	kFskEventWindowAfterUpdate,
	kFskEventWindowDragEnter,
	kFskEventWindowDragOver,
	kFskEventWindowDragDrop,
	kFskEventWindowDragLeave,
	kFskEventWindowActivated,
	kFskEventWindowDeactivated,
	kFskEventWindowOpenFiles,
	kFskEventClipboardChanged,
	kFskEventWindowBeforeUpdate,
	kFskEventWindowTransition,
	kFskEventWindowBeforeResize,
	kFskEventWindowAfterResize,
	kFskEventWindowBitmapChanged,

	kFskEventMenuCommand = 4096,
	kFskEventMenuEnableItems,
	kFskEventMenuStatus,

	kFskEventThreadCallback = 5120,

	kFskEventMediaPlayerIdle = 16384,
	kFskEventMediaPlayerStateChange,
	kFskEventMediaPlayerBoundsChange,
	kFskEventMediaPlayerEditChange,
	kFskEventMediaPlayerInitialize,
	kFskEventMediaPlayerTerminate,
	kFskEventMediaPlayerEnd,
	kFskEventMediaPlayerResourceLost,
	kFskEventMediaPlayerMetaDataChanged,
	kFskEventMediaPlayerAuthenticate,
	kFskEventMediaPlayerWarning,
	kFskEventMediaPlayerReady,				// buffering complete, ready to play
	kFskEventMediaPlayerHeaders,

	kFskEventMediaReaderDataArrived = 16640,

	kFskEventApplication = 0x6000,

	kFskEventSystemQuitRequest = 0x80000000,
	kFskEventHWSwitchQuery,

	kFskEventHWSwitchPower = 0x80000010,
	kFskEventHWSwitchHold,
	kFskEventHWSwitchCharge,
	kFskEventHWSwitchAC,
	kFskEventHWSwitchAV,
	kFskEventHWSwitch1394USB,
	kFskEventHWSwitchHeadphone,
	kFskEventHWSwitchWLAN,

	kFskEventSystemThermal = 0x80000100,
	kFskEventSystemLowBattery,

	kFskEventHWSetSubsystemPower = 0x80000200,

	kFskEventHWAudioDecodeError = 0x80000300,

	kFskEventUserFirst = 0xF0000000
} FskEventCodeEnum;

typedef enum {
    kFskEventModifierNone = 0,

	kFskEventModifierShift = 1L<<0,
	kFskEventModifierControl = 1L<<1,
	kFskEventModifierAlt = 1L<<2,
	kFskEventModifierCapsLock = 1L<<3,
	kFskEventModifierMouseButton = 1L<<4,

	kFskEventModifierNotSet =  -1		// special value for FskEventNew
} FskEventModifierEnum;

typedef enum {
	kFskEventParameterNone = 0,
	kFskEventParameterTime,
	kFskEventParameterModifiers,

	kFskEventParameterMouseLocation = 1024,
	kFskEventParameterMouseClickNumber,
	kFskEventParameterMouseWheelDelta,

	kFskEventParameterKeyUTF8 = 2048,
	kFskEventParameterFunctionKey,

	kFskEventParameterUpdateRectangle = 3072,
	kFskEventParameterFileList,
	kFskEventParameterTransition,

	kFskEventParameterCommand = 4096,

	kFskEventParameterEditInfo = 5120,

	kFskEventParameterDVDNavigationCommand = 6144,
	kFskEventParameterDVDNavigationButtonArea,

	kFskEventParameterThreadCallbackArgs = 7168,

	kFskEventParameterBatteryTemp = 8192,
	kFskEventParameterCaseTemp,
	kFskEventParameterBatteryLevel,

	kFskEventParameterHWButtonState,
	kFskEventParameterHWSwitch,
	kFskEventParameterHWSubsystem,
	kFskEventParameterHWPower,

	kFskEventParameterApplicationParam1,
	kFskEventParameterApplicationParam2,

	kFskEventParameterBooleanValue = 9216,
	kFskEventParameterIntegerValue,
	kFskEventParameterNumberValue,
	kFskEventParameterStringValue,
	kFskEventParameterCustomValue
} FskEventParameterEnum;

typedef enum {
	kFskEventFunctionKeyPlay = 0x10000L,
	kFskEventFunctionKeyPause,
	kFskEventFunctionKeyStop,
	kFskEventFunctionKeyTogglePlayPause,
	kFskEventFunctionKeyPreviousTrack,
	kFskEventFunctionKeyNextTrack,
	kFskEventFunctionKeyBeginSeekingBackward,
	kFskEventFunctionKeyEndSeekingBackward,
	kFskEventFunctionKeyBeginSeekingForward,
	kFskEventFunctionKeyEndSeekingForward,
	kFskEventFunctionKeyHome,
	kFskEventFunctionKeySearch,
	kFskEventFunctionKeyPower,
	kFskEventFunctionKeyMenu,
	kFskEventFunctionKeyVolumeUp,
	kFskEventFunctionKeyVolumeDown,
	kFskEventFunctionKeyMute,
	
	kFskEventFunctionKeyBeginInterruption = 0x20000L,
	kFskEventFunctionKeyEndInterruption,
	kFskEventFunctionKeyResumeFromInterruption,
} FskEventFunctionKeyEnum;

enum {
	kEventVirtualKeyUp = 1,
	kEventVirtualKeyDown,
	kEventVirtualKeyLeft,
	kEventVirtualKeyRight,
	kEventVirtualKeySelect,
	kEventVirtualKeyBack
};

FskDeclarePrivateType(FskEvent)

#if defined(__FSKEVENT_PRIV__) || SUPPORT_INSTRUMENTATION
	struct FskEventParameterRecord {
		struct FskEventParameterRecord	*next;
		FskEventParameterEnum			parameterCode;
		UInt32							parameterSize;
		char							parameterData[1];
	};
	typedef struct FskEventParameterRecord FskEventParameterRecord;
	typedef FskEventParameterRecord *FskEventParameter;

	struct FskEventRecord {
		struct FskEventRecord			*next;
		FskEventCodeEnum                eventCode;
		FskTimeRecord					atTime;
		FskEventModifierEnum			modifiers;
		FskInstrumentedItemDeclaration
		FskEventParameter				parameters;
	};
#endif

FskAPI(FskErr) FskEventNew(FskEvent *event, FskEventCodeEnum eventCode, FskTime atTime, FskEventModifierEnum modifiers);
FskAPI(FskErr) FskEventDispose(FskEvent event);
FskAPI(FskErr) FskEventCopy(FskEvent oldEvent, FskEvent *newEvent);

FskAPI(FskErr) FskEventParameterAdd(FskEvent event, FskEventParameterEnum parameterCode, UInt32 parameterSize, const void *parameterData);
FskAPI(FskErr) FskEventParameterRemove(FskEvent event, FskEventParameterEnum parameterCode);
FskAPI(FskErr) FskEventParameterGet(FskEvent event, FskEventParameterEnum parameterCode, void *parameterData);
FskAPI(FskErr) FskEventParameterGetSize(FskEvent event, FskEventParameterEnum parameterCode, UInt32 *parameterSize);

typedef struct {
	FskPointRecord	pt;
	UInt32			ticks;
	UInt32			index;			// which finger
} FskPointAndTicksRecord, *FskPointAndTicks;

#ifdef __cplusplus
}
#endif

#endif
