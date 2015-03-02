/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#ifndef __FSKEXTENSIONS__
#define __FSKEXTENSIONS__

#include "Fsk.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
	extension lists
*/

enum {
	kFskExtensionImageDecompressor = 1,
	kFskExtensionImageCompressor,
	kFskExtensionAudioDecompressor,
	kFskExtensionAudioCompressor,
	kFskExtensionMediaPlayer,
	kFskExtensionQTMediaPlayerTrack,
	kFskExtensionSoundChannel,					// not implemented as an extension yet
	kFskExtensionMuxer,
	kFskExtensionAudioFilter,
	kFskExtensionFileSystem,
	kFskExtensionDocumentViewer,
	kFskExtensionCaptureTrack,
	kFskExtensionNetDevice,
	kFskExtensionMediaReader,
	kFskExtensionTextEngine
};


#ifdef __FSKEXTENSIONS_PRIV__
	#include "FskThread.h"

	typedef struct {
		UInt32			count;
		void			**list;
	} FskExtensionTypeRecord, *FskExtensionType;

	FskErr FskExtensionsInitialize(void);
	void FskExtensionsTerminateThread(FskThread thread);
	void FskExtensionsTerminate(void);
#endif

FskAPI(FskErr) FskExtensionInstall(UInt32 extensionType, void *value);
FskAPI(FskErr) FskExtensionUninstall(UInt32 extensionType, void *value);

FskAPI(void) *FskExtensionGetByIndex(UInt32 extensionType, UInt32 index);
FskAPI(UInt32) FskExtensionGetCount(UInt32 extensionType);

/*
	native libraries
*/

typedef struct FskLibraryRecord FskLibraryRecord;
typedef FskLibraryRecord *FskLibrary;

#ifdef __FSKEXTENSIONS_PRIV__

	#if TARGET_OS_WIN32
		#include "Windows.h"
	#endif

	struct FskLibraryRecord {
#if TARGET_OS_WIN32
		HMODULE			module;
#elif TARGET_OS_LINUX || TARGET_OS_MAC
		void 			*module;
#elif TARGET_OS_KPL
		void 			*module;
#else
		int				nothing;
#endif
	};
#endif

typedef FskErr (*FskLibraryLoadProc)(FskLibrary library);
typedef FskErr (*FskLibraryUnloadProc)(FskLibrary library);

FskAPI(FskErr) FskLibraryLoad(FskLibrary *library, const char *path);
FskAPI(FskErr) FskLibraryUnload(FskLibrary library);

#define FskLibraryGetSymbolAddress(library, symbol, address) FskLibraryGetSymbolAddress_(library, symbol, (void **)(void *)(address))
FskAPI(FskErr) FskLibraryGetSymbolAddress_(FskLibrary library, const char *symbol, void **address);

#ifdef __cplusplus
}
#endif

#endif
