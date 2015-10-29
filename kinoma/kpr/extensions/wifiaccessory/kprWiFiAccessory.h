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
#ifndef __KPRWIFIACCESSORY__
#define __KPRWIFIACCESSORY__

#include "kpr.h"
#include "xs.h"

typedef struct KprWiFiAccessoryBrowserStruct KprWiFiAccessoryBrowserRecord, *KprWiFiAccessoryBrowser;
typedef struct KprWiFiAccessoryStruct KprWiFiAccessoryRecord, *KprWiFiAccessory;

void KPR_WiFiAccessory_Browser(xsMachine *the);

void KPR_WiFiAccessory_browser_destructor(void *it);
void KPR_WiFiAccessory_browser_get_accessories(xsMachine *the);
void KPR_WiFiAccessory_browser_start(xsMachine *the);
void KPR_WiFiAccessory_browser_stop(xsMachine *the);

void KPR_WiFiAccessory_accessory_destructor(void *it);
void KPR_WiFiAccessory_accessory_get_name(xsMachine *the);
void KPR_WiFiAccessory_accessory_configure(xsMachine *the);

FskErr kprWiFiAccessory_fskLoad(FskLibrary library);
FskErr kprWiFiAccessory_fskUnload(FskLibrary library);

FskErr KprWiFiAccessoryBrowserNew(KprWiFiAccessoryBrowser *it, xsMachine *the, xsIndex *code, xsSlot this);
FskErr KprWiFiAccessoryBrowserDispose(KprWiFiAccessoryBrowser self);
FskErr KprWiFiAccessoryBrowserGetAccessories(KprWiFiAccessoryBrowser self, xsMachine *the);
FskErr KprWiFiAccessoryBrowserStart(KprWiFiAccessoryBrowser self);
FskErr KprWiFiAccessoryBrowserStop(KprWiFiAccessoryBrowser self);

FskErr KprWiFiAccessoryNew(KprWiFiAccessory *it);
FskErr KprWiFiAccessoryDispose(KprWiFiAccessory self);
FskErr KprWiFiAccessoryGetName(KprWiFiAccessory self, xsMachine *the);
FskErr KprWiFiAccessoryConfigure(KprWiFiAccessory self, xsSlot callback);

#endif
