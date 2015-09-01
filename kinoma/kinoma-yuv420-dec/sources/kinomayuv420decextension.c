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
#define __FSKIMAGE_PRIV__

#include "kinomayuv420dec.h"


static FskImageDecompressorRecord yuv420Decompress =
	{yuv420DecodeCanHandle, yuv420DecodeNew, yuv420DecodeDispose, yuv420DecodeDecompressFrame, NULL, yuv420DecodeGetMetaData, yuv420DecodeProperties, yuv420DecodeSniff, yuv420DecodeFlush };


FskExport(FskErr) kinomayuv420dec_fskLoad(FskLibrary library)
{
	FskErr err = kFskErrNone;
	FskImageDecompressorInstall(&yuv420Decompress);
	return err;
}


FskExport(FskErr) kinomayuv420dec_fskUnload(FskLibrary library)
{
	FskImageDecompressorUninstall(&yuv420Decompress);
	return kFskErrNone;
}
