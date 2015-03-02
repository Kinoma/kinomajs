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
#define __FSKAUDIO_PRIV__
#include "FskAudio.h"

FskErr FskAudioCodecInitialize(void)
{
	static FskAudioDecompressorRecord audioDecompressors[] = {
		{NULL, NULL, NULL, NULL, NULL}
	};

	static FskAudioCompressorRecord audioCompressors[] = {
		{NULL, NULL, NULL, NULL}
	};
	FskAudioDecompressor walkerD;
	FskAudioCompressor walkerC;

	for (walkerD = audioDecompressors; NULL != walkerD->doCanHandle; walkerD++)
		FskAudioDecompressorInstall(walkerD);

	for (walkerC = audioCompressors; NULL != walkerC->doCanHandle; walkerC++)
		FskAudioCompressorInstall(walkerC);

	return kFskErrNone;
}

/*
	Audio decompression manager
*/

#if SUPPORT_INSTRUMENTATION
	static Boolean doFormatMessageAudioDecompress(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

	static FskInstrumentedTypeRecord gAudioDecompressTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"audiodecompress",
		FskInstrumentationOffset(FskAudioDecompressRecord),
		NULL,
		0,
		NULL,
		doFormatMessageAudioDecompress
	};
#endif

FskErr FskAudioDecompressNew(FskAudioDecompress *decoOut, UInt32 audioFormat, const char *mimeType, UInt32 sampleRate, UInt32 channelCount, void *formatInfo, UInt32 formatInfoSize)
{
	FskErr err;
	FskAudioDecompressor decoder;
	FskAudioDecompress deco;
	UInt32 i = 0;

	while (true) {
		Boolean canHandle = false;
		decoder = (FskAudioDecompressor)FskExtensionGetByIndex(kFskExtensionAudioDecompressor, i++);
		if (NULL == decoder) {
			if (decoOut)
				*decoOut = NULL;
			return kFskErrExtensionNotFound;
		}

		if ((kFskErrNone == decoder->doCanHandle(audioFormat, mimeType, &canHandle)) && canHandle)
			break;
	}

	if (NULL == decoOut)
		return kFskErrNone;			// can handler

	err = FskMemPtrNewClear(sizeof(FskAudioDecompressRecord), &deco);
	BAIL_IF_ERR(err);

	deco->inputSampleRate = sampleRate;
	deco->inputChannelCount = channelCount;
    deco->outputChannelCount = channelCount;

	deco->formatInfoSize = formatInfoSize;
	err = FskMemPtrNewFromData(formatInfoSize, formatInfo, (FskMemPtr*)(void*)&deco->formatInfo);
	BAIL_IF_ERR(err);

	deco->decoder = decoder;
	FskInstrumentedItemNew(deco, FskStrDoCopy_Untracked(mimeType), &gAudioDecompressTypeInstrumentation);
	err = deco->decoder->doNew(deco, audioFormat, mimeType);
	BAIL_IF_ERR(err);

bail:
	if (err) {
		FskAudioDecompressDispose(deco);
		deco = NULL;
	}
	*decoOut = deco;

	return err;
}

FskErr FskAudioDecompressDispose(FskAudioDecompress deco)
{
	if (deco) {
		if (deco->decoder && deco->decoder->doDispose)
			deco->decoder->doDispose(deco->state, deco);
		FskMemPtrDispose((void *)deco->formatInfo);
#if SUPPORT_INSTRUMENTATION
		FskInstrumentedItemDispose(deco);
		FskMemPtrDispose_Untracked((char *)deco->_instrumented.name);
#endif
		FskMemPtrDispose(deco);
	}
	return kFskErrNone;
}

FskErr FskAudioDecompressRequestedOutputFormat(FskAudioDecompress deco, UInt32 audioFormat)
{
	deco->requestedOutputFormat = audioFormat;
	return kFskErrNone;
}

FskErr FskAudioDecompressFrames(FskAudioDecompress deco, const void *data, UInt32 dataSize, UInt32 frameCount, UInt32 *frameSizes, void **samples, UInt32 *audioFormat, UInt32 *sampleCount, UInt32 *channelCount)
{
	FskErr err;
	UInt32 samplesSize = 0;

#if SUPPORT_INSTRUMENTATION
	FskTimeRecord start, now;

	if (FskInstrumentedItemHasListenersForLevel(deco, kFskInstrumentationLevelDebug))
		FskTimeGetNow(&start);
	else
		start.seconds = start.useconds = 0;
#endif

	*samples = 0;
	*sampleCount = 0;

	err = (deco->decoder->doDecompressFrames)(deco->state, deco, data, dataSize, frameCount, frameSizes, samples, &samplesSize);
	if (err) {
		FskInstrumentedItemSendMessageMinimal(deco, kFskAudioDecompressInstrDecompressFailed, (void *)err);
		goto bail;
	}

	deco->frameNumber += frameCount;

	*sampleCount = samplesSize / (2 * deco->outputChannelCount);
	if (audioFormat) *audioFormat = deco->outputFormat;
	if (channelCount) *channelCount = deco->outputChannelCount;

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(deco, kFskInstrumentationLevelMinimal)) {
		void *msgData[5];
		msgData[0] = (void *)data;
		msgData[1] = (void *)dataSize;
		msgData[2] = (void *)frameCount;
		msgData[3] = (void *)*sampleCount;
		if (!start.seconds)
			msgData[4] = (void *)0;
		else {
			FskTimeGetNow(&now);
			FskTimeSub(&start, &now);
			msgData[4] = (void *)FskTimeInMS(&now);
		}
		FskInstrumentedItemSendMessageForLevel(deco, kFskAudioDecompressInstrDecompress, msgData, kFskInstrumentationLevelMinimal);
	}
#endif

bail:
	return err;
}

FskErr FskAudioDecompressDiscontinuity(FskAudioDecompress deco)
{
	FskInstrumentedItemSendMessageVerbose(deco, kFskAudioDecompressInstrDiscontinuity, NULL);
	if (deco->decoder->doDiscontinuity)
		return deco->decoder->doDiscontinuity(deco->state, deco);
	return kFskErrNone;
}

FskErr FskAudioDecompressHasProperty(FskAudioDecompress deco, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	return FskMediaHasProperty(deco->decoder->properties, propertyID, get, set, dataType);
}

FskErr FskAudioDecompressSetProperty(FskAudioDecompress deco, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaSetProperty(deco->decoder->properties, deco->state, deco, propertyID, property);
}

FskErr FskAudioDecompressGetProperty(FskAudioDecompress deco, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaGetProperty(deco->decoder->properties, deco->state, deco, propertyID, property);
}

#if SUPPORT_INSTRUMENTATION

Boolean doFormatMessageAudioDecompress(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
		case kFskAudioDecompressInstrDiscontinuity:
			snprintf(buffer, bufferSize, "discontinuity");
			return true;

		case kFskAudioDecompressInstrDecompressFailed:
			snprintf(buffer, bufferSize, "decompress failed, err=%ld", (long)msgData);
			return true;

		case kFskAudioDecompressInstrDecompress: {
			void **md = (void **)msgData;
			snprintf(buffer, bufferSize, "decompress samplesOut=%ld, framesIn=%ld, dataSizeIn=%ld, data=%p, ms=%ld", (UInt32)md[3], (UInt32)md[2], (UInt32)md[1], md[0], (UInt32)md[4]);
			}
			return true;
	}

	return false;
}

#endif

/*
	Audio compression manager
*/

FskErr FskAudioCompressNew(FskAudioCompress *compOut, const char *outputFormat, const char *inputFormat, UInt32 inputSampleRate, UInt32 inputChannelCount)
{
	FskErr err;
	FskAudioCompressor encoder;
	FskAudioCompress comp;
	UInt32 i = 0;

	while (true) {
		Boolean canHandle = false;
		encoder = (FskAudioCompressor)FskExtensionGetByIndex(kFskExtensionAudioCompressor, i++);
		if (NULL == encoder) {
			*compOut = NULL;
			return kFskErrExtensionNotFound;
		}

		if ((kFskErrNone == encoder->doCanHandle(outputFormat, &canHandle)) && canHandle)
			break;
	}

	err = FskMemPtrNewClear(sizeof(FskAudioCompressRecord), &comp);
	BAIL_IF_ERR(err);

	comp->outputFormat = FskStrDoCopy(outputFormat);
	comp->inputFormat = FskStrDoCopy(inputFormat);
	comp->inputSampleRate = inputSampleRate;
	comp->inputChannelCount = inputChannelCount;

	comp->outputSampleRate = inputSampleRate;
	comp->outputChannelCount = inputChannelCount;

	comp->encoder = encoder;
	err = comp->encoder->doNew(comp);
	BAIL_IF_ERR(err);

bail:
	if (err) {
		FskAudioCompressDispose(comp);
		comp = NULL;
	}
	*compOut = comp;

	return err;
}

FskErr FskAudioCompressDispose(FskAudioCompress comp)
{
	if (comp) {
		if (comp->encoder && comp->encoder->doDispose)
			comp->encoder->doDispose(comp->state, comp);

		FskMemPtrDispose(comp->inputFormat);
		FskMemPtrDispose(comp->outputFormat);
		FskMemPtrDispose(comp->desc);
		FskMemPtrDispose(comp);
	}
	return kFskErrNone;
}

FskErr FskAudioCompressGetDescription(FskAudioCompress comp, void **desc, UInt32 *descSize)
{
	if (comp->desc) {
		*descSize = comp->descSize;
		return FskMemPtrNewFromData(comp->descSize, comp->desc, (FskMemPtr *)desc);
	}
	*desc = NULL;
	*descSize = 0;
	return kFskErrNone;
}

FskErr FskAudioCompressSetBitRate(FskAudioCompress comp, UInt32 bitsPerSecond)
{
	if (0 != comp->frameNumber)
		return kFskErrOutOfSequence;
	comp->requestedBitRate = bitsPerSecond;
	return kFskErrNone;
}

FskErr FskAudioCompressFrames(FskAudioCompress comp, const void *inData, UInt32 inDataSize, UInt32 inSampleCount,
				void **outSamples, UInt32 *outDataSize, UInt32 **outFrameSizes, UInt32 *outFrameCount, UInt32 *outSampleCount, UInt32 *outSampleRate, UInt32 *outChannelCount)
{
	FskErr err = kFskErrNone;
	UInt32 frameCount;

	*outSamples = NULL;
	*outFrameSizes = NULL;
	if (outFrameCount)
		*outFrameCount = 0;

	err = (comp->encoder->doCompressFrames)(comp->state, comp, inData, inDataSize, inSampleCount,
							outSamples, outDataSize, outFrameSizes, &frameCount);
	comp->frameNumber += 1;
	BAIL_IF_ERR(err);

	if (outSampleRate)
		*outSampleRate = comp->outputSampleRate;
	if (outChannelCount)
		*outChannelCount = comp->outputChannelCount;
	if (outFrameCount)
		*outFrameCount = frameCount;
	if (outSampleCount)
		*outSampleCount = comp->outputSamplesPerFrame * frameCount;

bail:
	return err;
}

FskErr FskAudioCompressHasProperty(FskAudioCompress comp, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	return FskMediaHasProperty(comp->encoder->properties, propertyID, get, set, dataType);
}

FskErr FskAudioCompressGetProperty(FskAudioCompress comp, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaGetProperty(comp->encoder->properties, comp->state, comp, propertyID, property);
}

FskErr FskAudioCompressSetProperty(FskAudioCompress comp, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaSetProperty(comp->encoder->properties, comp->state, comp, propertyID, property);
}
