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
#ifndef __FSKECMASCRIPT__
#define __FSKECMASCRIPT__

#include "Fsk.h"

#include "xs.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __FSKECMASCRIPT_PRIV__
#include "FskExtensions.h"
#include "FskThread.h"

extern void FskECMAScriptGetDebug(char **host, char **breakpoints, Boolean *keepSource);
extern void FskECMAScriptHibernate(void);
extern FskErr FskECMAScriptInitialize(const char *configPath);
extern FskErr FskECMAScriptInitializationComplete(void);
extern void FskECMAScriptTerminate(void);
extern FskErr loadGrammar(const char *xsbPath, xsGrammar *grammar);
extern void xsMemPtrToChunk(xsMachine *the, xsSlot *ref, FskMemPtr data, UInt32 dataSize, Boolean alreadyAllocated);

#endif

#if FSK_EMBED
	extern void FskExtensionsEmbedLoad(char *vmName);
	extern void FskExtensionsEmbedUnload(char *vmName);
	extern void FskExtensionsEmbedGrammar(char *vmName, char **xsbName, xsGrammar **grammar);
#endif

FskAPI(FskErr) FskECMAScriptLoadLibrary(const char *name);

#if FSK_TEXT_FREETYPE
FskErr FskTextFreeTypeInstallFonts(char* fontsPath, char* defaultFont);
#endif

#ifdef __cplusplus
}
#endif

#endif
