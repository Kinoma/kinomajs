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
#ifndef __FSKMAIN__
#define __FSKMAIN__

#include "Fsk.h"

#if TARGET_OS_WIN32 || (TARGET_OS_KPL && defined(_MSC_VER))
	#include "Windows.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern Boolean gQuitting;

enum {
	kFskMainNetwork = 		1L << 0,
	kFskMainServer = 		1L << 1,
	kFskMainNoECMAScript = 	1L << 2
};

FskAPI(FskErr) FskMainInitialize(UInt32 flags, int argc, char **argv);

FskAPI(FskErr) FskMainApplicationLoop(void);

FskAPI(FskErr) FskMainTerminate(void);

FskAPI(FskErr) FskMainDoQuit(FskErr exitCode);

#if TARGET_OS_WIN32 || (TARGET_OS_KPL && defined(_MSC_VER))
	FskAPI(void) FskMainSetHInstance(HINSTANCE hInstance);	// must be called before FskMainInitialize
	FskAPI(HINSTANCE) FskMainGetHInstance(void);
#elif TARGET_OS_MAC
    FskErr FskCocoaMain(UInt32 flags, int argc, char **argv);
#endif

void mainExtensionInitialize(void);
void mainExtensionTerminate(void);

#ifdef __cplusplus
}
#endif

#endif
