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
#ifndef __KPRWINDOW__
#define __KPRWINDOW__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct KprIdleLinkStruct {
	KprSlotPart;
	KprTimerPart;
};

struct KprShellStruct {
	KprSlotPart;
	KprContentPart;
	KprContainerPart;
	KprContextPart;
	FskWindow window;
	FskPort port;
	KprContent focus;
	KprContentChainRecord touchChain;
	xsMachine* root;
	xsIndex* code;
	FskRectangleRecord caretBounds;
	UInt32 caretFlags;
	double caretTicks;
	UInt32 modifiers;
	FskList messages;
	KprContentChainRecord applicationChain;
	FskWindowEventHandler handler;
	KprIdleLink idleChain;
	KprIdleLink idleLoop;
	KprContentChainRecord placeChain;
	void *archive;
#if SUPPORT_EXTERNAL_SCREEN
	FskWindow extWindow;
	FskExtScreenNotifier extScreenNotifier;
	int extScreenId;
#endif	/* SUPPORT_EXTERNAL_SCREEN */
};

FskAPI(FskErr) KprShellNew(KprShell* it, FskWindow window, FskRectangle bounds, char* shellPath, char* modulesPath, char* cdata, char* debugFile, int debugLine);

FskAPI(void) KprShellAdjust(KprShell self);
FskAPI(void) KprShellCaptureTouch(void* it, KprContent content, UInt32 id, SInt32 x, SInt32 y, double ticks);
FskAPI(FskThread) KprShellGetThread(KprShell self);
FskAPI(void) KprShellPurge(KprShell self);
FskAPI(void) KprShellQuit(KprShell self);
FskAPI(void) KprShellReflow(KprShell self, UInt32 flags);
FskAPI(void) KprShellSetCaret(KprShell self, void* it, FskRectangle bounds);
FskAPI(void) KprShellSetFocus(KprShell self, KprContent content);
FskAPI(void) KprShellStartIdling(KprShell self, void* it);
FskAPI(void) KprShellStopIdling(KprShell self, void* it);
FskAPI(void) KprShellStartPlacing(KprShell self, void* it);
FskAPI(void) KprShellStopPlacing(KprShell self, void* it);
FskAPI(double) KprShellTicks(void* it);

extern void KprShellKeyActivate(void* it, Boolean activateIt);
extern void KprShellIdleLoop(void* it, const FskTime atTime);
extern void KprShellKeySelect(void* it);

FskAPI(void) KprShellHTTPAuthenticate(KprShell self, void* it, char* host, char* realm);

FskAPI(double) KprShellGetVolume(KprShell self);
FskAPI(void) KprShellSetVolume(KprShell self, double volume);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
