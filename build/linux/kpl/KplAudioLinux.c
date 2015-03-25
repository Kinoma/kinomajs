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

#include "KplAudio.h"

FskErr KplAudioNew(KplAudio *audioOutOut)
{
	return kFskErrUnimplemented;
}

FskErr KplAudioDispose(KplAudio audio)
{
	return kFskErrUnimplemented;
}

FskErr KplAudioSetFormat(KplAudio audio, const char *format, UInt32 channels, double sampleRate, const unsigned char *formatInfo, UInt32 formatInfoSize)
{
	return kFskErrUnimplemented;
}

FskErr KplAudioGetFormat(KplAudio kplAudio, const char **format, UInt32 *channels, double *sampleRate, const unsigned char **formatInfo, UInt32 *formatInfoSize)
{
	return kFskErrUnimplemented;
}

FskErr KplAudioWrite(KplAudio audio, const char *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, const UInt32 *frameSizes)
{
	return kFskErrUnimplemented;
}

FskErr KplAudioStart(KplAudio audio)
{
	return kFskErrUnimplemented;
}

FskErr KplAudioStop(KplAudio audio)
{
	return kFskErrUnimplemented;
}

FskErr KplAudioGetProperty(KplAudio audio, UInt32 propertyID, KplProperty value)
{
	return kFskErrUnimplemented;
}

FskErr KplAudioSetProperty(KplAudio audio, UInt32 propertyID, KplProperty value)
{
	return kFskErrUnimplemented;
}

FskErr KplAudioSetDoneCallback(KplAudio audio, KplAudioDoneCallback cb, void *refCon)
{
	return kFskErrUnimplemented;
}

FskErr KplAudioSetMoreCallback(KplAudio audio, KplAudioMoreCallback cb, void *refCon)
{
	return kFskErrUnimplemented;
}

FskErr KplAudioGetSamplesQueued(KplAudio audio, UInt32 *samplesQueuedOut, UInt32 *targetQueueLengthOut)
{
	return kFskErrUnimplemented;
}
