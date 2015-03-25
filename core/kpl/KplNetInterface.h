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
#ifndef __KPL_NET_INTERFACE_H__
#define __KPL_NET_INTERFACE_H__

#include "FskErrors.h"

typedef struct KplNetInterfaceRecord {
	struct KplNetInterfaceRecord *next;
	char		*name;
	int			ip;
	int			netmask;
	char		MAC[6];
	int			status;
} KplNetInterfaceRecord, *KplNetInterface;

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*KplNetInterfaceChangedCallback)(void *refCon);

FskErr KplNetInterfaceInitialize(void);
FskErr KplNetInterfaceTerminate(void);

FskErr KplNetInterfaceEnumerate(KplNetInterfaceRecord **interfaceList);

FskErr KplNetInterfaceSetChangedCallback(KplNetInterfaceChangedCallback cb, void *refCon);

#ifdef __cplusplus
}
#endif

#endif // __KPL_NET_INTERFACE_H__
