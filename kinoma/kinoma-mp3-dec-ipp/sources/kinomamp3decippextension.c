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
#include "kinomamp3decipp.h"
#include <dlfcn.h>

#include "FskHardware.h"
#include "FskArch.h"

static FskAudioDecompressorRecord audioDecompressors[] = 
{
	{mp3DecodeCanHandle, mp3DecodeNew, mp3DecodeDispose, mp3DecodeDecompressFrames,	mp3DecodeDiscontinuity, mp3DecodeProperties},
	{NULL, NULL, NULL, NULL}
};
 

FskExport(FskErr) kinomamp3decipp_fskLoad(FskLibrary library)
{
	FskErr err = kFskErrNone;

	dlog( "into kinomamp3decipp_fskLoad\n"); 
	FskAudioDecompressorInstall(&audioDecompressors);

	return err;
}


FskExport(FskErr) kinomamp3decipp_fskUnload(FskLibrary library)
{
	dlog( "into kinomamp3ipp_fskUnload\n");
	FskAudioDecompressorUninstall(&audioDecompressors);

	return kFskErrNone;
}
