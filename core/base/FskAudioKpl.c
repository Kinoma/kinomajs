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
#include "FskAudio.h"
#include "KplAudio.h"

typedef struct {
	FskSampleTime zeroTime;
	Boolean playing;
	
	KplAudio kplAudio;
} FskAudioKplRecord, *FskAudioKpl;

static FskErr FskAudioFormatToMIME(UInt32 audioFormat, const char **mime);

static AudioOutVectors gAudioOutVectors;

FskErr FskAudioKplNew(FskAudioOut *audioOut, UInt32 outputID, UInt32 format)
{
	FskErr err;
	FskAudioKpl audio = NULL;
	
	err = FskMemPtrNewClear(sizeof(FskAudioKplRecord), (FskMemPtr*)&audio);
	BAIL_IF_ERR(err);
	
	err = KplAudioNew(&audio->kplAudio);
	BAIL_IF_ERR(err);
	
bail:
	if (0 != err) {
		if (audio) {
			FskMemPtrDispose(audio);
			audio = NULL;
		}
	}
	*audioOut = (FskAudioOut)audio;
	
	return err;
}

FskErr FskAudioKplDispose(FskAudioOut audioOut)
{
	FskErr err = kFskErrNone;
	
	if (audioOut) {
		err = KplAudioDispose(((FskAudioKpl)audioOut)->kplAudio);
		FskMemPtrDispose(audioOut);
	}
	
	return err;
}

FskErr FskAudioKplGetFormat(FskAudioOut audioOut, UInt32 *format, UInt16 *numChannels, double *sampleRate)
{
	FskErr err;
	const char *mime;
	UInt32 channels;
	double rate;
	
	err = KplAudioGetFormat(((FskAudioKpl)audioOut)->kplAudio, &mime, &channels, &rate, NULL, NULL);
	if (0 == err) {
		if (format)
			FskAudioMIMEToFormat(mime, format);
		if (numChannels)
			*numChannels = (UInt16)channels;
		if (sampleRate)
			*sampleRate = rate;
	}
	
	return err;
}

FskErr FskAudioKplSetFormat(FskAudioOut audioOut, UInt32 format, UInt16 numChannels, double sampleRate, unsigned char *formatInfo, UInt32 formatInfoSize)
{
	FskErr err;
	const char *mime = NULL;
	
	err = FskAudioFormatToMIME(format, &mime);
	if (0 == err) {
		err = KplAudioSetFormat(((FskAudioKpl)audioOut)->kplAudio, mime, numChannels, sampleRate, formatInfo, formatInfoSize);
	}
	
	return err;
}

FskErr FskAudioKplGetOutputBufferSize(FskAudioOut audioOut, UInt32 *chunkSize, UInt32 *bufferedSamplesTarget, UInt32 *minimumBytesToStart)
{
	FskErr err = kFskErrNone;
	FskAudioKpl audio = (FskAudioKpl)audioOut;
	KplPropertyRecord property;
	
	err = KplAudioGetProperty(audio->kplAudio, kKplPropertyAudioPreferredBufferSize, &property);
	BAIL_IF_ERR(err);

	if (bufferedSamplesTarget)
		*bufferedSamplesTarget = 0;
	if (chunkSize)
		*chunkSize = property.integer;
	if (minimumBytesToStart)
		*minimumBytesToStart = 0;
	
bail:
	return err;
}

FskErr FskAudioKplGetVolume(FskAudioOut audioOut, UInt16 *left, UInt16 *right)
{
	FskErr err = kFskErrNone;
	KplPropertyRecord property = {0};
	
	err = KplAudioGetProperty(((FskAudioKpl)audioOut)->kplAudio, kKplPropertyAudioVolume, &property);
	BAIL_IF_ERR(err);
	
	*left = (UInt16)property.integers.integer[0];
	*right = (UInt16)property.integers.integer[1];
	
bail:
	FskMemPtrDispose(property.integers.integer);
	
	return err;
}

FskErr FskAudioKplSetVolume(FskAudioOut audioOut, UInt16 left, UInt16 right)
{
	FskErr err = kFskErrNone;
	KplPropertyRecord property = {0};
	UInt32 values[2];

	property.propertyType = kKplPropertyTypeIntegers;
	property.integers.integer = values;

	values[0] = left;
	values[1] = right;
	
	err = KplAudioSetProperty(((FskAudioKpl)audioOut)->kplAudio, kKplPropertyAudioVolume, &property);
	BAIL_IF_ERR(err);
	
bail:
	return err;
}

FskErr FskAudioKplStart(FskAudioOut audioOut, FskSampleTime atSample)
{
	FskErr err = kFskErrNone;
	FskAudioKpl audio = (FskAudioKpl)audioOut;
	
	if (audio->playing)
		goto bail;
	
	err = KplAudioStart(audio->kplAudio);
	BAIL_IF_ERR(err);
	
	audio->zeroTime = atSample;
	audio->playing = true;
	
bail:
	return err;
}

FskErr FskAudioKplStop(FskAudioOut audioOut)
{
	FskErr err = kFskErrNone;
	FskAudioKpl audio = (FskAudioKpl)audioOut;
	
	if (!audio)
		return err;
	
	if (!audio->playing)
		goto bail;
	
	KplAudioStop(audio->kplAudio);
	
	audio->playing = false;
	
bail:
	return err;
}

FskErr FskAudioKplEnqueue(FskAudioOut audioOut, void *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, UInt32 *frameSizes)
{
	FskErr err;
	FskAudioKpl audio = (FskAudioKpl)audioOut;
	
	err = KplAudioWrite(audio->kplAudio, data, dataSize, dataRefCon, frameCount, frameSizes);
	
	return err;
}

FskErr FskAudioKplGetSamplePosition(FskAudioOut audioOut, FskSampleTime *position)
{
	FskErr err = kFskErrNone;
	FskAudioKpl audio = (FskAudioKpl)audioOut;
	KplPropertyRecord property;
	
	if (false == audio->playing)
		return kFskErrBadState;
	
	err = KplAudioGetProperty(audio->kplAudio, kKplPropertyAudioSamplePosition, &property);
	BAIL_IF_ERR(err);
	
	*position = (FskSampleTime)(audio->zeroTime + property.number);
	
bail:
	return err;
}

FskErr FskAudioKplGetSamplesQueued(FskAudioOut audioOut, UInt32 *samplesQueued, UInt32 *targetQueueLength)
{
	FskErr err;
	FskAudioKpl audio = (FskAudioKpl)audioOut;
	
	err = KplAudioGetSamplesQueued(audio->kplAudio, samplesQueued, targetQueueLength);
	
	return err;
}

FskErr FskAudioKplSingleThreadedClient(FskAudioOut audioOut, Boolean *isSingleThreaded)
{
	FskErr err;
	FskAudioKpl audio = (FskAudioKpl)audioOut;
	KplPropertyRecord property;
	
	err = KplAudioGetProperty(audio->kplAudio, kKplPropertyAudioSingleThreadedClient, &property);
	BAIL_IF_ERR(err);
	
	*isSingleThreaded = property.b;
	
bail:
	return err;
}

FskErr FskAudioKplSetDoneCallback(FskAudioOut audioOut, FskAudioOutDoneCallback cb, void *refCon)
{
	FskErr err;
	FskAudioKpl audio = (FskAudioKpl)audioOut;
	
	err = KplAudioSetDoneCallback(audio->kplAudio, (KplAudioDoneCallback)cb, refCon);
	
	return err;
}

FskErr FskAudioKplSetMoreCallback(FskAudioOut audioOut, FskAudioOutMoreCallback cb, void *refCon)
{
	FskErr err;
	FskAudioKpl audio = (FskAudioKpl)audioOut;
	
	err = KplAudioSetMoreCallback(audio->kplAudio, (KplAudioMoreCallback)cb, refCon);
	
	return err;
}

void FskAudioKplInitialize()
{
	gAudioOutVectors.doNew = FskAudioKplNew;
	gAudioOutVectors.doDispose = FskAudioKplDispose;
	gAudioOutVectors.doGetFormat = FskAudioKplGetFormat;
	gAudioOutVectors.doSetFormat = FskAudioKplSetFormat;
	gAudioOutVectors.doGetVolume = FskAudioKplGetVolume;
	gAudioOutVectors.doSetVolume = FskAudioKplSetVolume;
	gAudioOutVectors.doSetDoneCallback = FskAudioKplSetDoneCallback;
	gAudioOutVectors.doSetMoreCallback = FskAudioKplSetMoreCallback;
	gAudioOutVectors.doStart = FskAudioKplStart;
	gAudioOutVectors.doStop = FskAudioKplStop;
	gAudioOutVectors.doEnqueue = FskAudioKplEnqueue;
	gAudioOutVectors.doGetSamplePosition = FskAudioKplGetSamplePosition;
	gAudioOutVectors.doGetSamplesQueued = FskAudioKplGetSamplesQueued;
	gAudioOutVectors.doSingleThreadedClient = FskAudioKplSingleThreadedClient;
	gAudioOutVectors.doGetOutputBufferSize = FskAudioKplGetOutputBufferSize;
	
	gAudioOutVectors.doIsValid = NULL;
	gAudioOutVectors.doSetOutputBufferSize = NULL;
	gAudioOutVectors.doGetDeviceHandle = NULL;
	gAudioOutVectors.doHasProperty = NULL;
	gAudioOutVectors.doGetProperty = NULL;
	gAudioOutVectors.doSetProperty = NULL;
	
	FskAudioOutSetVectors(&gAudioOutVectors);
}

void FskAudioKplTerminate()
{
	FskAudioOutSetVectors(NULL);
}

FskErr FskAudioFormatToMIME(UInt32 audioFormat, const char **mime)
{
	if (kFskAudioFormatPCM16BitBigEndian == audioFormat)
		*mime = "x-audio-codec/pcm-16-be";
	else if (kFskAudioFormatPCM16BitLittleEndian == audioFormat)
		*mime = "x-audio-codec/pcm-16-le";
	else if (kFskAudioFormatPCM8BitTwosComplement == audioFormat)
		*mime = "x-audio-codec/pcm-8-twos";
	else if (kFskAudioFormatPCM8BitOffsetBinary == audioFormat)
		*mime = "x-audio-codec/pcm-8-offset";
	else if (kFskAudioFormatMP3 == audioFormat)
		*mime = "x-audio-codec/mp3";
	else if (kFskAudioFormatAAC == audioFormat)
		*mime = "x-audio-codec/aac";
	else if (kFskAudioFormatATRAC3 == audioFormat)
		*mime = "x-audio-codec/atrac3";
	else if (kFskAudioFormatATRAC3Plus == audioFormat)
		*mime = "x-audio-codec/atrac3-plus";
	else if (kFskAudioFormatATRACAdvancedLossless == audioFormat)
		*mime = "x-audio-codec/atrac3-lossless";
	else if (kFskAudioFormatMP1A == audioFormat)
		*mime = "x-audio-codec/mpeg-1";
	else if (kFskAudioFormatAC3 == audioFormat)
		*mime = "x-audio-codec/ac3";
	else if (kFskAudioFormatQCELP == audioFormat)
		*mime = "x-audio-codec/qcelp";
	else if (kFskAudioFormatEVRC == audioFormat)
		*mime = "x-audio-codec/evrc";
	else if (kFskAudioFormatAMRNB == audioFormat)
		*mime = "x-audio-codec/amr-nb";
	else if (kFskAudioFormatAMRWB == audioFormat)
		*mime = "x-audio-codec/amr-wb";
	else if (kFskAudioFormatWMA == audioFormat)
		*mime = "x-audio-codec/wma";
	else if (kFskAudioFormatSPEEXNB == audioFormat)
		*mime = "x-audio-codec/speex-nb";
	else
		return kFskAudioFormatUndefined;
	
	return kFskErrNone;
}
