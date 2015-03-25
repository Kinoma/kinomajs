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
#ifndef __NETINTERFACE_H__
#define __NETINTERFACE_H__

#include "Fsk.h"


#ifdef __cplusplus
extern "C" {
#endif


FskAPI(FskErr) FskNetInterfaceInitialize(void);
FskAPI(void) FskNetInterfaceTerminate(void);

FskAPI(Boolean) FskNetIsLocalAddress(int addr);
FskAPI(Boolean) FskNetIsLocalNetwork(int addr);


enum {
	kFskNetInterfaceStatusRemoved = 0,
	kFskNetInterfaceStatusNew,
	kFskNetInterfaceStatusChanged
};

struct FskNetInterfaceRecord;
typedef int (*FskNetInterfaceChangedCallback)(struct FskNetInterfaceRecord *iface, UInt32 status, void *param);

#ifndef __FSKNETUTILS_PRIV__
	FskDeclarePrivateType(FskNetInterfaceNotifier)
#else
typedef struct FskNetInterfaceNotifierRec {
	struct FskNetInterfaceNotifierRec *next;
	FskNetInterfaceChangedCallback callback;
	void 		*param;
	FskThread	thread;

	FskInstrumentedItemDeclaration

	char name[1];		// must be last
} FskNetInterfaceNotifierRec, *FskNetInterfaceNotifier;
#endif

typedef struct FskNetInterfaceRecord {
	struct FskNetInterfaceRecord *next;
	char		*name;
	int			ip;
	int			netmask;
	char		MAC[6];
	int			status;
} FskNetInterfaceRecord, *FskNetInterface;

#if SUPPORT_INSTRUMENTATION
enum {
	kFskNetInstrMsgInterfaceNotify = kFskInstrumentedItemFirstCustomMessage
};

typedef struct FskInterfaceInstrData {
	FskNetInterfaceNotifier notf;
	FskNetInterface ifc;
	int status;
} FskInterfaceInstrData;
#endif

FskAPI(UInt32) FskNetInterfaceEnumerate(void);
FskAPI(FskErr) FskNetCleanupNetworkEnumeration(void);
FskAPI(FskErr) FskNetInterfaceDescribe(int idx, FskNetInterfaceRecord **iface);
FskAPI(FskErr) FskNetInterfaceDescriptionDispose(FskNetInterfaceRecord *iface);
FskAPI(FskNetInterfaceRecord *) FskNetInterfaceFindByName(char *name);

FskAPI(FskNetInterfaceNotifier) FskNetInterfaceAddNotifier(
				FskNetInterfaceChangedCallback callback, void *param, char *name);
FskAPI(void) FskNetInterfaceRemoveNotifier(FskNetInterfaceNotifier callbackRef);

#if TARGET_OS_IPHONE
void FskNetInterfacesChanged();
#endif /* TARGET_OS_IPHONE */

#if TARGET_OS_LINUX
void LinuxInterfacesChanged();
#endif

#ifdef __cplusplus
}
#endif

#endif

