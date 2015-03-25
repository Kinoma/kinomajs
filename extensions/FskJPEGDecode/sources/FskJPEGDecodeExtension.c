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
#include "FskECMAScript.h"
#include "FskImage.h"
#include "FskJPEGDecode.h"

extern FskMediaPropertyEntryRecord jpegDecodeProperties[];

static FskImageDecompressorRecord sFskJPEGDecoder = {jpegDecodeCanHandle, jpegDecodeNew, jpegDecodeDispose, jpegDecodeDecompressFrame, jpegDecodeSetData, jpegDecodeGetMetaData, jpegDecodeProperties, jpegDecodeSniff};

FskExport(FskErr) FskJPEGDecode_fskLoad(FskLibrary library)
{
	FskImageDecompressorInstall(&sFskJPEGDecoder);
	
	return kFskErrNone;
}

FskExport(FskErr) FskJPEGDecode_fskUnload(FskLibrary library)
{
	FskImageDecompressorUninstall(&sFskJPEGDecoder);
	
	return kFskErrNone;
}
