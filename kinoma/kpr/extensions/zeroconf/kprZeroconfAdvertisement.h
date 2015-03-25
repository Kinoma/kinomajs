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
#ifndef __KPRZEROCONFADVERTISEMENT__
#define __KPRZEROCONFADVERTISEMENT__

typedef struct KprZeroconfAdvertisementStruct KprZeroconfAdvertisementRecord, *KprZeroconfAdvertisement;

typedef void (*KprZeroconfAdvertisementRegisteredCallback)(KprZeroconfAdvertisement self);
typedef void (*KprZeroconfAdvertisementUnregisteredCallback)(KprZeroconfAdvertisement self);

struct KprZeroconfAdvertisementStruct {
	KprZeroconfCommonPart;
	char* serviceName;
	UInt32 port;
	FskAssociativeArray txt;
	KprZeroconfAdvertisementRegisteredCallback registeredCallback;
	KprZeroconfAdvertisementUnregisteredCallback unregisteredCallback;
	FskInstrumentedItemDeclaration
};

FskErr KprZeroconfAdvertisementNew(KprZeroconfAdvertisement *it, const char* serviceType, const char* serviceName, UInt32 port, FskAssociativeArray txt);
void KprZeroconfAdvertisementDispose(KprZeroconfAdvertisement self);
KprZeroconfAdvertisement KprZeroconfAdvertisementFind(KprZeroconfAdvertisement self, const char* serviceType, const UInt32 port);
void KprZeroconfAdvertisementServiceRegistered(KprZeroconfAdvertisement self, KprZeroconfServiceInfo serviceInfo);
FskErr KprZeroconfAdvertisementStart(KprZeroconfAdvertisement self);
FskErr KprZeroconfAdvertisementStop(KprZeroconfAdvertisement self);

FskErr KprZeroconfPlatformAdvertisementNew(KprZeroconfAdvertisement self);
void KprZeroconfPlatformAdvertisementDispose(KprZeroconfAdvertisement self);
FskErr KprZeroconfPlatformAdvertisementStart(KprZeroconfAdvertisement self);
FskErr KprZeroconfPlatformAdvertisementStop(KprZeroconfAdvertisement self);

#endif
