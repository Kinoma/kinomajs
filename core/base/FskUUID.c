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
#define __FSKUUID_PRIV__
#include "FskUUID.h"

#include "FskAssociativeArray.h"
#include "FskEndian.h"
#include "FskEnvironment.h"
#include "FskFiles.h"
#include "FskNetUtils.h"
#include "FskUtilities.h"
#include "FskNetInterface.h"
#include "FskPlatformImplementation.h"

FskErr FskUUIDCreate(FskUUID uuid)
{
	FskTimeRecord time;
	static UInt32 clockSequence = 0;
	static FskTimeRecord lastTime;
	char MAC[6];
	FskNetInterfaceRecord *netInterface = NULL;
	UInt32 numInterfaces;

	// find a suitable network interface to get the MAC address from
	numInterfaces = FskNetInterfaceEnumerate();

	while (numInterfaces--) {
		FskNetInterfaceDescribe(numInterfaces, &netInterface);
		if ((0 == netInterface->MAC[0]) && (0 == netInterface->MAC[1]) && (0 == netInterface->MAC[2]) &&
			(0 == netInterface->MAC[3]) && (0 == netInterface->MAC[4]) && (0 == netInterface->MAC[5])) {
			FskNetInterfaceDescriptionDispose(netInterface);
			netInterface = NULL;
			continue;
		}
		break;
	}

	if (NULL == netInterface) {
		// can't fail - clients need a value. make something up. (this happens if all network interfaces are disabled on a phone)
		int i;
		for (i = 0; i < 6; i++)
			MAC[i] = (char)FskRandom();
	}
	else {
		FskMemMove(MAC, netInterface->MAC, sizeof(netInterface->MAC));
		FskNetInterfaceDescriptionDispose(netInterface);
	}

	// make sure the clock sequence is good
	while (0 == clockSequence)
		clockSequence = FskRandom();

	// we need the time, and make sure it is unique.
	FskTimeGetNow(&time);                                           //@@ should be UTC time
	time.useconds = (time.useconds >> 4) | (time.seconds << 28);	// only uses 60 bits of time
	time.seconds >>= 4;												// only uses 60 bits of time
	if (FskTimeCompare(&time, &lastTime) <= 0)
		clockSequence += 1;

	lastTime = time;

	// put the pieces together
	time.useconds = FskEndianU32_NtoB(time.useconds);
	FskMemCopy(&uuid->value[0], &time.useconds, sizeof(time.useconds));
	uuid->value[4] = (UInt8)time.seconds;
	uuid->value[5] = (UInt8)(time.seconds >> 8);
	uuid->value[6] = (((UInt8)(time.seconds >> 24)) & 0x0f) | 1;
	uuid->value[7] = (UInt8)(time.seconds >> 16);
	uuid->value[8] = ((UInt8)(clockSequence >> 8) & 0x3f) | 0x80;
	uuid->value[9] = (UInt8)clockSequence;
	FskMemCopy(&uuid->value[10], MAC, sizeof(MAC));

	return kFskErrNone;
}

char *FskUUIDtoString(FskUUID uuid)
{
	// uppercase 000000000000-0000-0000-000000000000
	char *str = NULL;

	if (kFskErrNone == FskMemPtrNewClear(36, &str)) {
		UInt32 u32;
		UInt16 u16;
		UInt8 *ptr = uuid->value;

		u32 = *(UInt32 *)(ptr + 0);
		FskStrNumToHex(FskEndianU32_NtoB(u32), &str[0], 8);
		u16 = *(UInt16 *)(ptr + 4);
		FskStrNumToHex(FskEndianU16_NtoB(u16), &str[8], 4);
		str[12] = '-';
		u16 = *(UInt16 *)(ptr + 6);
		FskStrNumToHex(FskEndianU16_NtoB(u16), &str[13], 4);
		str[17] = '-';
		u16 = *(UInt16 *)(ptr + 8);
		FskStrNumToHex(FskEndianU16_NtoB(u16), &str[18], 4);
		str[22] = '-';
		u16 = *(UInt16 *)(ptr + 10);
		FskStrNumToHex(FskEndianU16_NtoB(u16), &str[23], 4);
		u32 = *(UInt32 *)(ptr + 12);
		FskStrNumToHex(FskEndianU32_NtoB(u32), &str[27], 8);
	}

	return str;
}

char *FskUUIDtoString_844412(FskUUID uuid)
{
	// lowercase 00000000-0000-0000-0000-000000000000
	char *str = NULL;

	if (kFskErrNone == FskMemPtrNewClear(37, &str)) {
		UInt32 u32;
		UInt16 u16;
		UInt8 *ptr = uuid->value;

		u32 = *(UInt32 *)(ptr + 0);
		FskStrNumToHex(FskEndianU32_NtoB(u32), &str[0], 8);
		str[8] = '-';
		u16 = *(UInt16 *)(ptr + 4);
		FskStrNumToHex(FskEndianU16_NtoB(u16), &str[9], 4);
		str[13] = '-';
		u16 = *(UInt16 *)(ptr + 6);
		FskStrNumToHex(FskEndianU16_NtoB(u16), &str[14], 4);
		str[18] = '-';
		u16 = *(UInt16 *)(ptr + 8);
		FskStrNumToHex(FskEndianU16_NtoB(u16), &str[19], 4);
		str[23] = '-';
		u16 = *(UInt16 *)(ptr + 10);
		FskStrNumToHex(FskEndianU16_NtoB(u16), &str[24], 4);
		u32 = *(UInt32 *)(ptr + 12);
		FskStrNumToHex(FskEndianU32_NtoB(u32), &str[28], 8);
		for (u16 = 0; u16 < 36; u16++)
			str[u16] = tolower(str[u16]);		//@@ should use portable function
	}

	return str;
}

static FskAssociativeArray gUUIDCache;

char *FskUUIDGetForKey(const char *key)
{
	char *uuidStr;
	FskUUIDRecord uuid;
	char *uuidCachePath = NULL, *prefFolder;
	FskFile fref;

	if (kFskErrNone == FskDirectoryGetSpecialPath(kFskDirectorySpecialTypeApplicationPreference, true, NULL, &prefFolder)) {
		uuidCachePath = FskStrDoCat(prefFolder, "fskuuidcache.txt");
		FskMemPtrDispose(prefFolder);
	}

	// load the cache
	if (NULL == gUUIDCache) {
		char *uuidCache;
		FskInt64 cacheSize;

		gUUIDCache = FskAssociativeArrayNew();

		if ((NULL != uuidCachePath) && (kFskErrNone == FskFileLoad(uuidCachePath, (unsigned char **)(void *)&uuidCache, &cacheSize))) {
			char *p = uuidCache;
			while (true) {
				char *uuid = p;
				char *mykey = FskStrChr(p, '\t');
				char *cr;

				if (NULL == mykey)
					break;
				cr = FskStrChr(mykey, '\n');
				if (NULL == cr)
					break;
				*mykey++ = 0;
				*cr = 0;

				FskAssociativeArrayElementSetString(gUUIDCache, mykey, uuid);

				p = cr + 1;
			}
			FskMemPtrDispose(uuidCache);
		}
	}

	// check the cache
	uuidStr = FskAssociativeArrayElementGetString(gUUIDCache, key);
	if (uuidStr) {
		FskMemPtrDispose(uuidCachePath);
		return uuidStr;
	}

	// not in cache
	FskUUIDCreate(&uuid);
	uuidStr = FskUUIDtoString_844412(&uuid);

	FskAssociativeArrayElementSetString(gUUIDCache, key, uuidStr);

	FskMemPtrDispose(uuidStr);

	// flush cache
	FskFileDelete(uuidCachePath);
	FskFileCreate(uuidCachePath);
	if (kFskErrNone == FskFileOpen(uuidCachePath, kFskFilePermissionReadWrite, &fref)) {
		FskAssociativeArrayIterator iterate = FskAssociativeArrayIteratorNew(gUUIDCache);
		char tab = '\t', cr = '\n';

		while (iterate) {
			FskFileWrite(fref, FskStrLen(iterate->value), iterate->value, NULL);
			FskFileWrite(fref, 1, &tab, NULL);
			FskFileWrite(fref, FskStrLen(iterate->name), iterate->name, NULL);
			FskFileWrite(fref, 1, &cr, NULL);

			iterate = FskAssociativeArrayIteratorNext(iterate);
		}
		FskFileClose(fref);
	}

	FskMemPtrDispose(uuidCachePath);

	return FskAssociativeArrayElementGetString(gUUIDCache, key);	// caller doesn't have to dispose
}

void FskUUIDTerminate(void)
{
	FskAssociativeArrayDispose(gUUIDCache);
}
