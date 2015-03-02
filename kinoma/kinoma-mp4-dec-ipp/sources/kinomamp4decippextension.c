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
#define __FSKIMAGE_PRIV__
#include "kinomamp4decipp.h"

static FskImageDecompressorRecord mp4Decompress =
	{mp4DecodeCanHandle, mp4DecodeNew, mp4DecodeDispose, mp4DecodeDecompressFrame, NULL, mp4DecodeGetMetaData, mp4DecodeProperties, NULL, mp4DecodeFlush };


FskExport(FskErr) kinomamp4decipp_fskLoad(FskLibrary library)
{
	FskErr err = kFskErrNone;
	
	dlog( "into kinomamp4decipp_fskLoad\n"); 
	FskImageDecompressorInstall(&mp4Decompress);
	dlog( "out of kinomamp4decipp_fskLoad\n"); 
	
	return err;
}


FskExport(FskErr) kinomamp4decipp_fskUnload(FskLibrary library)
{
	dlog( "into kinomamp4ipp_fskUnload\n");
	FskImageDecompressorUninstall(&mp4Decompress);
	dlog( "out of kinomamp4decipp_fskUnload\n"); 
	
	return kFskErrNone;
}
