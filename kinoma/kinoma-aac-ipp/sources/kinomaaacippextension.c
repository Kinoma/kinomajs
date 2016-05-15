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
#define __FSKAACDECODE_PRIV__
#include "kinomaaacippdecode.h"


static FskAudioDecompressorRecord audioDecompressors[] = {
	{aacDecodeCanHandle, aacDecodeNew, aacDecodeDispose, aacDecodeDecompressFrames,	aacDecodeDiscontinuity, aacDecodeProperties},
	{NULL, NULL, NULL, NULL}
};


FskExport(FskErr) kinomaaacipp_fskLoad(FskLibrary library)
{
	//FskAudioDecompressor walkerAD;

	dlog( "into kinomaaacipp_fskLoad\n");
	FskAudioDecompressorInstall(&audioDecompressors);

	//for (walkerAD = audioDecompressors; NULL != walkerAD->doCanHandle; walkerAD++)
	//	FskAudioDecompressorInstall(walkerAD);

	return kFskErrNone;
}

FskExport(FskErr) kinomaaacipp_fskUnload(FskLibrary library)
{
	//FskAudioDecompressor walkerAD;

	dlog( "into kinomaaacipp_fskUnload\n");

	FskAudioDecompressorUninstall(&audioDecompressors);
	
	//for (walkerAD = audioDecompressors; NULL != walkerAD->doCanHandle; walkerAD++)
	//	FskAudioDecompressorUninstall(walkerAD);

	return kFskErrNone;
}
