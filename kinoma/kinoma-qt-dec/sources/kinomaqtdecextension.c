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
#include "kinomaqtdec.h"

static FskImageDecompressorRecord qtDecompress =
	{qtDecCanHandle, qtDecNew, qtDecDispose, qtDecDecompressFrame, NULL, qtDecGetMetaData, qtDecProperties, NULL, qtDecFlush };


FskExport(FskErr) kinomaqtdec_fskLoad(FskLibrary library)
{
	FskImageDecompressorInstall(&qtDecompress);
	return kFskErrNone;
}


FskExport(FskErr) kinomaqtdec_fskUnload(FskLibrary library)
{
	FskImageDecompressorUninstall(&qtDecompress);
	return kFskErrNone;
}
