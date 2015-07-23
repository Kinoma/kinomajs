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
#ifndef __KPL_PROPERTY_H__
#define __KPL_PROPERTY_H__

#include "Kpl.h"

enum {
	kKplPropertyTypeInteger = 1,
	kKplPropertyTypeDouble,
	kKplPropertyTypeString,
	kKplPropertyTypeIntegers,
	kKplPropertyTypeDoubles,
	kKplPropertyTypeBoolean
};

enum {
	kKplPropertyDevicePowerFlagDisableScreenDimming = (1 << 0),
	kKplPropertyDevicePowerFlagDisableSleep = (1 << 1)
};

enum {
	kKplPropertyDeviceBatteryLevel = 1,
	kKplPropertyDeviceCharging,
	kKplPropertyDeviceTouch,
	kKplPropertyDeviceOS,
	kKplPropertyDeviceLanguage,
	kKplPropertyDeviceLocation,
	kKplPropertyDeviceOrientation,
	kKplPropertyDevicePowerFlags,
	
	kKplPropertyPhoneSignalStrength = 100,
	kKplPropertyPhoneCallStatus,

	kKplPropertyAudioPreferredSampleRate = 200,
	kKplPropertyAudioPreferredUncompressedFormat,
	kKplPropertyAudioPreferredBufferSize,
	kKplPropertyAudioSamplePosition,
	kKplPropertyAudioVolume,
	kKplPropertyAudioSingleThreadedClient,
	kKplPropertyAudioRealTime
};

typedef struct {
	UInt32 propertyType;
	union {
		SInt32 integer;
		double number;
		char *string;
		struct {
			UInt32 count;
			UInt32 *integer;
		} integers;
		struct {
			UInt32 count;
			double *number;
		} numbers;
		Boolean b;
	};
} KplPropertyRecord, *KplProperty;

#endif // __KPL_PROPERTY_H__
