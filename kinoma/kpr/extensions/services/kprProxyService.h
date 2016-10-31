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
#ifndef __KPRPROXYSERVICE__
#define __KPRPROXYSERVICE__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef struct KprProxyServiceVMStruct KprProxyServiceVMRecord, *KprProxyServiceVM;

	typedef void (*KprProxyServiceVMCallback)(xsMachine *the, void *refcon);
	
	FskAPI(FskErr) KprProxyServiceVMNew(void *context, void *refcon, KprProxyServiceVM *it);
	FskAPI(KprProxyServiceVM) KprProxyServiceVMFind(SInt32 index);
	FskAPI(FskErr) KprProxyServiceVMDispose(KprProxyServiceVM self);

	FskAPI(void) KprProxyServiceVMStart(KprProxyServiceVM self, KprProxyServiceVMCallback onStart, void *refcon);

	FskAPI(void) KprProxyServiceVMExec(KprProxyServiceVM self, KprProxyServiceVMCallback callback, void *refcon);

	FskAPI(SInt32) KprProxyServiceVMGetIndex(KprProxyServiceVM self);
	FskAPI(xsMachine *) KprProxyServiceVMGetMachine(KprProxyServiceVM self);
	FskAPI(void *) KprProxyServiceVMGetRefcon(KprProxyServiceVM self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
