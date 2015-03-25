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
#ifndef __KPRHANDLER__
#define __KPRHANDLER__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct KprContextStruct {
	KprSlotPart;
	KprContentPart;
	KprContainerPart;
	KprContextPart;
};

FskAPI(void) KprContextAccept(void* it, KprMessage message);
FskAPI(void) KprContextDisposeHandlers(void* it);
FskAPI(KprHandler) KprContextGetHandler(void* it, KprURLParts parts);
FskAPI(void) KprContextGLContextLost(void* it);
FskAPI(void) KprContextInvoke(void* it, KprMessage message);
FskAPI(void) KprContextPurge(void* it, Boolean flag);
FskAPI(void) KprContextPutHandler(void* it, KprHandler handler);
FskAPI(void) KprContextRemoveHandler(void* it, KprHandler handler);

extern void KprContextMark(void* it, xsMarkRoot markRoot);

struct KprHandlerStruct {
	KprSlotPart;
	KprTimerPart;
	KprBehavior behavior;
	KprShell shell;
	KprContainer container;
	KprHandler previous;
	KprHandler next;
	UInt32 flags;
	char* path;
	KprMessage message;
};

FskAPI(FskErr) KprHandlerNew(KprHandler *self, char* id);
FskErr KprHandlerTrigger(KprHandler self, KprMessage message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
