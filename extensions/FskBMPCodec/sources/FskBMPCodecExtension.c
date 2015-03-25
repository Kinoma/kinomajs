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
#include "FskBMPDecode.h"

extern FskMediaPropertyEntryRecord bmpDecodeProperties[];

static FskImageDecompressorRecord sFskBMPDecoder = {bmpDecodeCanHandle, bmpDecodeNew, bmpDecodeDispose, bmpDecodeDecompressFrame, NULL, bmpDecodeGetMetaData, bmpDecodeProperties, bmpDecodeSniff};
static FskImageCompressorRecord sFskBMPEncoder = {bmpEncodeCanHandle, bmpEncodeNew, bmpEncodeDispose, bmpEncodeCompressFrame, NULL};

FskExport(FskErr) FskBMPCodec_fskLoad(FskLibrary library)
{
	FskImageDecompressorInstall(&sFskBMPDecoder);
	FskImageCompressorInstall(&sFskBMPEncoder);
	
	return kFskErrNone;
}

FskExport(FskErr) FskBMPCodec_fskUnload(FskLibrary library)
{
	FskImageCompressorUninstall(&sFskBMPEncoder);
	FskImageDecompressorUninstall(&sFskBMPDecoder);
	
	return kFskErrNone;
}
