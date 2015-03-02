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
#ifndef __KPL_DEVICE_H__
#define __KPL_DEVICE_H__

#include "FskErrors.h"
#include "KplProperty.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef FskErr (*KplPropertyChangedCallback)(UInt32 propertyID, void *refcon);

FskErr KplDeviceGetProperty(UInt32 propertyID, KplProperty value);
FskErr KplDeviceSetProperty(UInt32 propertyID, KplProperty value);

FskErr KplDeviceNotifyPropertyChangedNew(UInt32 propertyID, KplPropertyChangedCallback proc, void *refcon);
FskErr KplDeviceNotifyPropertyChangedDispose(UInt32 propertyID, KplPropertyChangedCallback proc, void *refcon);

#ifdef __cplusplus
}
#endif

#endif
