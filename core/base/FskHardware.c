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
#define __FSKTHREAD_PRIV__
#include "Fsk.h"
#include "FskMemory.h"
#include "FskSynchronization.h"
#include "FskNetInterface.h"
#include "FskList.h"
#include "FskThread.h"
#include "FskHardware.h"

FskInstrumentedSimpleType(Hardware, hardware);

#if TARGET_OS_ANDROID

FskListMutex phoneCBList = NULL;
FskPhoneHWInfo gFskPhoneHWInfo = NULL;
FskCallStatusCB gCallStatusCB = NULL;

FskErr FskHardwareInitialize(void) {
	FskErr err;

	err = FskMemPtrNewClear(sizeof(FskPhoneHWInfoRecord), &gFskPhoneHWInfo);
    if (kFskErrNone == err) {
        // first time- set some defaults
        gFskPhoneHWInfo->backlightOn = true;
    }
    return err;
}

FskErr FskHardwareTerminate(void) {
    return kFskErrNone;
}

FskErr FskSetCallStatusCB(FskCallStatusCB cb) {
	gCallStatusCB = cb;
	return kFskErrNone;
}

FskErr FskSetCallStatus(int line, int state) {
	FskHardwarePrintfDebug("set callstatus %d, %d callback: %x", line, state, gCallStatusCB);
	if (gCallStatusCB)
		return (*gCallStatusCB)(line, state);
	return kFskErrNone;
}

char *connStateStr(int state) {
	switch (state) {
		case 0: return "Disconnected";
		case 1: return "Connecting";
		case 2: return "Connected";
		case 3: return "Suspended";
		case 4: return "Off";
		case 5: return "EmergencyOnly";
	}
	return "unknown";
}

static FskPhonePropertyCB gFskPhonePropertyCB = NULL;

void FskHardwareSetPhonePropertyCallback(FskPhonePropertyCB cb)
{
	gFskPhonePropertyCB = cb;
}

static void androidPhoneDoCallbacks(SInt32 prop)
{
	if (gFskPhonePropertyCB)
		(*gFskPhonePropertyCB)(prop);
}

FskErr FskSetPhoneHWInfo(FskPhoneHWInfo info) {
	FskErr err = kFskErrNone;
    FskPhoneHWInfo hw = gFskPhoneHWInfo;

	FskHardwarePrintfDebug("FskSetPhoneHWInfo info: %x\n", info);

	if (hw->batteryLevel != info->batteryLevel) {
		FskHardwarePrintfDebug("battery level changed %d to %d", hw->batteryLevel, info->batteryLevel);
		hw->batteryLevel = info->batteryLevel;
		androidPhoneDoCallbacks(kFskPhonePropertyBattery);
	}
	if (hw->signalStrength != info->signalStrength) {
		FskHardwarePrintfDebug("signalStrength changed %d to %d", hw->signalStrength, info->signalStrength);
		hw->signalStrength = info->signalStrength;
		androidPhoneDoCallbacks(kFskPhonePropertySignalStrength);
	}
	if (hw->dataSignalStrength != info->dataSignalStrength) {
		FskHardwarePrintfDebug("dataSignalStrength changed %d to %d", hw->dataSignalStrength, info->dataSignalStrength);
		hw->dataSignalStrength = info->dataSignalStrength;
		androidPhoneDoCallbacks(kFskPhonePropertyDataNetworkSignalStrength);
	}
	if (hw->chargerPlugged != info->chargerPlugged) {
		FskHardwarePrintfDebug("chargerPlugged changed %d to %d", hw->chargerPlugged, info->chargerPlugged);
		hw->chargerPlugged = info->chargerPlugged;
		androidPhoneDoCallbacks(kFskPhonePropertyPluggedIn);
	}
	if (hw->cellularNetworkType != info->cellularNetworkType) {
		FskHardwarePrintfDebug("cellularNetworkType changed %d to %d\n", hw->cellularNetworkType, info->cellularNetworkType);
		hw->cellularNetworkType = info->cellularNetworkType;
		androidPhoneDoCallbacks(kFskPhonePropertyCellularNetworkType);
	}
	if (hw->dataNetworkType != info->dataNetworkType) {
		FskHardwarePrintfDebug("dataNetworkType changed %d to %d\n", hw->dataNetworkType, info->dataNetworkType);
		hw->dataNetworkType = info->dataNetworkType;
		androidPhoneDoCallbacks(kFskPhonePropertyDataNetworkType);
	}
	if (hw->missedCalls != info->missedCalls) {
		FskHardwarePrintfDebug("missedCalls changed %d to %d\n", hw->missedCalls, info->missedCalls);
		hw->missedCalls = info->missedCalls;
		androidPhoneDoCallbacks(kFskPhonePropertyMissedCalls);
	}
	if (hw->unreadMessages != info->unreadMessages) {
		FskHardwarePrintfDebug("unreadMessages changed %d to %d\n", hw->unreadMessages, info->unreadMessages);
		hw->unreadMessages = info->unreadMessages;
		androidPhoneDoCallbacks(kFskPhonePropertyUnreadMessages);
	}
	if (0 != FskStrCompare(hw->operatorStr, info->operatorStr)) {
		FskHardwarePrintfDebug("operatorStr changed %s to %s\n", hw->operatorStr, info->operatorStr);
		FskMemPtrDispose(hw->operatorStr);
		hw->operatorStr = FskStrDoCopy(info->operatorStr);
		androidPhoneDoCallbacks(kFskPhonePropertyOperator);
	}
	if (0 != FskStrCompare(hw->ssid, info->ssid)) {
		FskHardwarePrintfDebug("ssid changed %s to %s\n", hw->ssid, info->ssid);
		FskMemPtrDispose(hw->ssid);
		hw->ssid = FskStrDoCopy(info->ssid);
		androidPhoneDoCallbacks(kFskPhonePropertySSID);
	}
	if (0 != FskStrCompare(hw->imei, info->imei)) {
		FskHardwarePrintfDebug("imei changed %s to %s\n", hw->imei, info->imei);
		FskMemPtrDispose(hw->imei);
		hw->imei = FskStrDoCopy(info->imei);
		androidPhoneDoCallbacks(kFskPhonePropertyIMEI);
	}
	if ((hw->activeNetworkType == info->activeNetworkType)
		|| (hw->cellularNetworkState == info->cellularNetworkState)
		|| (hw->dataNetworkState == info->dataNetworkState)) {
		if ((((hw->cellularNetworkState == 2) || (info->cellularNetworkState == 2)) && (hw->cellularNetworkState != info->cellularNetworkState))
			|| (((hw->dataNetworkState == 2) || (info->dataNetworkState == 2)) && (hw->dataNetworkState != info->dataNetworkState))) {
			FskHardwarePrintfDebug(" -- poke the interface change\n");
			LinuxInterfacesChanged();	// new
		}
		hw->activeNetworkType = info->activeNetworkType;
		hw->cellularNetworkState = info->cellularNetworkState;
		hw->dataNetworkState = info->dataNetworkState;
		FskHardwarePrintfDebug("network stuff changed - ");
		FskHardwarePrintfDebug(" cell: (%d:%s) data: (%d:%s) ", 
			hw->cellularNetworkState, connStateStr(hw->cellularNetworkState),
			hw->dataNetworkState, connStateStr(hw->dataNetworkState) );
		FskHardwarePrintfDebug(" - activeNetworkType %d\n", hw->activeNetworkType);
		androidPhoneDoCallbacks(kFskPhonePropertyConnected);
	}

	if (hw->backlightOn != info->backlightOn) {
		FskHardwarePrintfDebug("android told us backlight is %s\n", info->backlightOn ? "on" : "off");
		hw->backlightOn = info->backlightOn;
		androidPhoneDoCallbacks(kFskPhonePropertyBacklight);
	}

	if ((hw->gpsStatus != info->gpsStatus)
		|| (hw->gpsLat != info->gpsLat)
		|| (hw->gpsLong != info->gpsLong)
		|| (hw->gpsSpeed != info->gpsSpeed)
		|| (hw->gpsVisible != info->gpsVisible)
		|| (hw->gpsUTC != info->gpsUTC)) {
		hw->gpsStatus = info->gpsStatus;
		hw->gpsLat = info->gpsLat;
		hw->gpsLong = info->gpsLong;
		hw->gpsAlt = info->gpsAlt;
		hw->gpsHeading = info->gpsHeading;
		hw->gpsSpeed = info->gpsSpeed;
		hw->gpsVisible = info->gpsVisible;
		hw->gpsUTC = info->gpsUTC;
		hw->gpsWhen = info->gpsWhen;
		hw->gpsAccuracy = info->gpsAccuracy;

		FskHardwarePrintfDebug("GPS update\n");
		androidPhoneDoCallbacks(kFskPhonePropertyGPS);
	}

	if (hw->orientation != info->orientation) {
		FskHardwarePrintfDebug("setting orientation to %d\n", info->orientation);
		hw->orientation = info->orientation;
	}

	if (hw->keyboardType != info->keyboardType) {
		FskHardwarePrintfDebug("hw - kbdType now %d\n", info->keyboardType);
		hw->keyboardType = info->keyboardType;
	}

	if (hw->wifiAddr != info->wifiAddr) {
		FskHardwarePrintfDebug("hw - wifiAddr now %x\n", info->wifiAddr);
		hw->wifiAddr = info->wifiAddr;
	}

	if (hw->afterInit != info->afterInit) {
		FskHardwarePrintfDebug("hw - afterInit now %d\n", info->afterInit);
		hw->afterInit = info->afterInit;
	}

	if (FskStrCompare(hw->language, info->language)) {
		FskHardwarePrintfDebug("hw - language now %s\n", info->language);
		if (hw->language)
			FskMemPtrDispose(hw->language);
		hw->language = FskStrDoCopy(info->language);
		androidPhoneDoCallbacks(kFskPhonePropertySystemLanguage);
	}

	if (FskStrCompare(hw->uuid, info->uuid)) {
		FskHardwarePrintfDebug("hw - uuid now %s\n", info->uuid);
		if (hw->uuid)
			FskMemPtrDispose(hw->uuid);
		hw->uuid = FskStrDoCopy(info->uuid);
		androidPhoneDoCallbacks(kFskPhonePropertyUUID);
	}

	if (FskStrCompare(hw->MACAddress, info->MACAddress)) {
		FskHardwarePrintfDebug("hw - MACAddress now %s\n", info->MACAddress);
		if (hw->MACAddress)
			FskMemPtrDispose(hw->MACAddress);
		hw->MACAddress = FskStrDoCopy(info->MACAddress);
		androidPhoneDoCallbacks(kFskPhonePropertyMACAddress);
	}

	if (hw->needsRotationTransition != info->needsRotationTransition) {
		hw->needsRotationTransition = info->needsRotationTransition;
	}

	return err;
}


FskAndroidCallbacks gAndroidCallbacks = NULL;

FskErr FskSetAndroidCallbacks(FskAndroidCallbacks callbacks) {
// this is too early, if we call instrumentation here we will crash
// --	FskHardwarePrintfDebug("set android callbacks\n");
    FskMemPtrDisposeAt(&gAndroidCallbacks);
    return FskMemPtrNewFromData(sizeof(FskAndroidCallbacksRecord), callbacks, &gAndroidCallbacks);
}


#elif TARGET_OS_KPL

#else

FskErr FskHardwareInitialize(void) {
	return kFskErrNone;
}
FskErr FskHardwareTerminate(void) {
	return kFskErrNone;
}

#endif
