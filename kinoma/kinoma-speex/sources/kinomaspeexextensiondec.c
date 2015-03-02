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
#include "kinomaspeexdec.h"

FskMediaPropertyEntryRecord speexDecodeProperties[] = 
{
	{kFskMediaPropertyUndefined,		kFskMediaPropertyTypeUndefined,		NULL,							NULL}
};

static FskAudioDecompressorRecord audioDecompressors[] = {
	{speexDecodeCanHandle, speexDecodeNew, speexDecodeDispose, speexDecodeDecompressFrames,	speexDecodeDiscontinuity, speexDecodeProperties},
	{NULL, NULL, NULL, NULL}
};

FskExport(FskErr) kinomaspeexdec_fskLoad(FskLibrary library)
{
	FskAudioDecompressor walkerAD;

	for (walkerAD = audioDecompressors; NULL != walkerAD->doCanHandle; walkerAD++)
		FskAudioDecompressorInstall(walkerAD);


	return kFskErrNone;
}

FskExport(FskErr) kinomaspeexdec_fskUnload(FskLibrary library)
{
	FskAudioDecompressor walkerAD;

	for (walkerAD = audioDecompressors; NULL != walkerAD->doCanHandle; walkerAD++)
		FskAudioDecompressorUninstall(walkerAD);

	return kFskErrNone;
}
