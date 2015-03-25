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
#define __FSKMEDIAREADER_PRIV__
#include "FskECMAScript.h"
#include "FskMediaReader.h"

#include "FskPlatformImplementation.h"
FskInstrumentedSimpleType(CameraAVFExtension, CameraAVFExtension);
#define mlog  FskCameraAVFExtensionPrintfMinimal
#define nlog  FskCameraAVFExtensionPrintfNormal
#define vlog  FskCameraAVFExtensionPrintfVerbose
#define dlog  FskCameraAVFExtensionPrintfDebug

extern FskMediaReaderDispatchRecord gCameraAVF;


FskExport(FskErr) FskCameraAVF_fskLoad(FskLibrary library)
{
	FskErr err = kFskErrNone;
	FskMediaReaderInstall(&gCameraAVF);
	
bail:
	dlog( "out of FskCameraAVF_fskLoad()\n");
	return err;
}

FskExport(FskErr) FskCameraAVF_fskUnload(FskLibrary library)
{
	dlog( "into FskCameraAVF_fskUnload()\n");
	FskMediaReaderUninstall(&gCameraAVF);
	
	dlog( "out of FskCameraAVF_fskUnload()\n");
	return kFskErrNone;
}
