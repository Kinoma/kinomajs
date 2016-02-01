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
#include <windows.h>

#include "KplDevice.h"
#include "KplThread.h"

#include "FskList.h"
#include "FskMemory.h"

//#define TEST_PROPERTY_NOTIFIERS 1

#if TEST_PROPERTY_NOTIFIERS
	#include "KplTime.h"
	static void setupTestNotifier(UInt32 propertyID);
	static UInt32 gOrientation = 0;
#endif

typedef struct {
	struct KplPhoneNotifyRecord *next;
	UInt32		propertyID;
	KplPropertyChangedCallback	proc;
	void		*refcon;
	KplThread	thread;
} KplPhoneNotifyRecord, *KplPhoneNotify;

static FskListMutex gPhoneNotify = NULL;

static void notifyThreadCallback(void *arg1, void *arg2, void *arg3, void *arg4);

FskErr KplDeviceGetProperty(UInt32 propertyID, KplProperty value)
{
	FskErr err = kFskErrUnimplemented;
	SYSTEM_POWER_STATUS sps;
	
	switch(propertyID) {
		case kKplPropertyDeviceBatteryLevel:
			if (GetSystemPowerStatus(&sps)) {
				if (255 == sps.BatteryLifePercent)
					sps.BatteryLifePercent = 100;
				value->integer = sps.BatteryLifePercent;
				err = kFskErrNone;
			}
			break;
		case kKplPropertyDeviceCharging:
			if (GetSystemPowerStatus(&sps)) {
				value->b = (0 != sps.ACLineStatus);
				err = kFskErrNone;
			}
			break;
		case kKplPropertyPhoneSignalStrength:
			break;
		case kKplPropertyPhoneCallStatus:
			break;
		case kKplPropertyDeviceTouch:
			break;
		case kKplPropertyDeviceOS:
			break;
		case kKplPropertyDeviceLanguage: {
			char buf[128];
			// @@ This should better handle multi-byte characters
			if (0 != GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, buf, sizeof(buf))) {
				char c, *src, *dst;
				FskMemPtrNew(128, (FskMemPtr*)&value->string);
				src = buf;
				dst = value->string;
				while ((c = *src++)) {
					if ((c & 0x80) == 0x00)
						*dst++ = toupper(c);
					else
						*dst++ = c;
				}
				*dst = 0;
				err = kFskErrNone;
			}
			}
			break;
		case kKplPropertyDeviceLocation:
			break;
		case kKplPropertyDeviceOrientation:
#if TEST_PROPERTY_NOTIFIERS
			++gOrientation;
			if (gOrientation > 6)
				gOrientation = 0;
			value->integer = gOrientation;
			err = kFskErrNone;
#endif
			break;
		case kKplPropertyDevicePowerFlags:
			break;
		default:
			break;
	}

	return err;
}

FskErr KplDeviceSetProperty(UInt32 propertyID, KplProperty value)
{
	FskErr err = kFskErrUnimplemented;
	
	switch(propertyID) {
		case kKplPropertyDevicePowerFlags:
			break;
		default:
			break;
	}

	return err;
}

FskErr KplDeviceNotifyPropertyChangedNew(UInt32 propertyID, KplPropertyChangedCallback proc, void *refcon)
{
	FskErr err = kFskErrNone;
	KplPhoneNotify kpn;
	
	if (NULL == gPhoneNotify)
		FskListMutexNew(&gPhoneNotify, "PhoneNotify");

	FskMemPtrNewClear(sizeof(KplPhoneNotifyRecord), (FskMemPtr *)&kpn);
	kpn->propertyID = propertyID;
	kpn->proc = proc;
	kpn->refcon = refcon;
	kpn->thread = KplThreadGetCurrent();
	
	FskListMutexAppend(gPhoneNotify, kpn);

#if TEST_PROPERTY_NOTIFIERS
	setupTestNotifier(propertyID);
#endif

	return err;
}

FskErr KplDeviceNotifyPropertyChangedDispose(UInt32 propertyID, KplPropertyChangedCallback proc, void *refcon)
{
	FskErr err = kFskErrNone;
	KplPhoneNotifyRecord *kpn;
	
	FskListMutexAcquireMutex(gPhoneNotify);
	for (kpn = gPhoneNotify->list; NULL != kpn; kpn = (KplPhoneNotifyRecord*)kpn->next) {
		if (propertyID == kpn->propertyID && proc == kpn->proc) {
			break;
		}
	}
	FskListMutexReleaseMutex(gPhoneNotify);

	if (NULL != kpn) {
		FskListMutexRemove(gPhoneNotify, kpn);
		FskMemPtrDispose(kpn);
	}
		
	return err;
}

void notifyThreadCallback(void *arg1, void *arg2, void *arg3, void *arg4)
{
	KplPropertyChangedCallback	proc = arg1;
	UInt32 propertyID = (UInt32)arg2;
	void *refcon = arg3;
	(*proc)(propertyID, refcon);
}

void notifyPropertyChanged(SInt32 prop)
{
	KplPhoneNotifyRecord *kpn;
	
	if (!gPhoneNotify) return;

	FskListMutexAcquireMutex(gPhoneNotify);

	for (kpn = gPhoneNotify->list; NULL != kpn; kpn = (KplPhoneNotifyRecord*)kpn->next) {
		if (prop == kpn->propertyID) {
			FskKplThreadPostCallback(kpn->thread, notifyThreadCallback, kpn->proc, (void*)kpn->propertyID, kpn->refcon, NULL);
		}
	}

	FskListMutexReleaseMutex(gPhoneNotify);
}


#if TEST_PROPERTY_NOTIFIERS
KplTimeCallback gTimeCB = NULL;

static void testNotifierCallback(KplTimeCallback cb, const KplTime time, void *param)
{
	UInt32 propertyID = (UInt32)param;
	KplTimeRecord when;
	
	notifyPropertyChanged(propertyID);
	
	KplTimeGetNow(&when);
	when.seconds += 1;
	KplTimeCallbackSet(gTimeCB, &when, testNotifierCallback, (void*)propertyID);
}

void setupTestNotifier(UInt32 propertyID)
{
	KplTimeRecord when;
	KplTimeCallbackNew(&gTimeCB);
	KplTimeGetNow(&when);
	when.seconds += 1;
	KplTimeCallbackSet(gTimeCB, &when, testNotifierCallback, (void*)propertyID);
}
#endif
