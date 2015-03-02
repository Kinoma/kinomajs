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
#define INITGUID
#include "Fsk.h"

#define __FSKAUDIO_PRIV__
#include "FskAudio.h"

#include "FskDIDLGenMedia.h"
#include "FskEndian.h"
#include "FskMedia.h"
#include "FskTime.h"
#include "FskUtilities.h"
#include "stddef.h"		// for offsetof macro

#if TARGET_OS_WIN32
	#include "FskPlatformImplementation.h"

	#if SUPPORT_WIN32_DIRECTSOUND
		#include "math.h"

		// we don't fully fill DirectSound buffers by this number of samples, to avoid an ambiguity between buffer full & empty
		#define kDirectSoundRefillSlop (1)
	#elif SUPPORT_WIN32_WAVEOUT
		#include "FskMain.h"
	#endif
#endif

#if TARGET_OS_MAC
	#if TARGET_OS_IPHONE
		#include "FskCocoaSupportPhone.h"
	#else
		#include "FskCocoaSupport.h"
	#endif
#endif

#if TARGET_OS_KPL
	#include "FskAudioKpl.h"
#endif

#if !(TARGET_OS_KPL || TARGET_OS_LINUX)	// is in an extension now
static FskErr initializePlatformOutput(FskAudioOut audioOut, UInt32 format);
static void terminatePlatformOutput(FskAudioOut audioOut);

static void refillQueue(FskAudioOut audioOut);
static void removeUnusedFromQueue(FskAudioOut audioOut);
static void removeAllFromQueue(FskAudioOut audioOut);
#endif

#if !SUPPORT_WIN32_DIRECTSOUND && !TARGET_OS_MAC && !TARGET_OS_KPL && !TARGET_OS_LINUX
	static void renderAudioBlock(FskAudioOut audioOut, FskAudioOutBlock block);
#endif

static UInt32 getSamplesQueued(FskAudioOut audioOut);
static UInt32 getChunkRequestSize(FskAudioOut audioOut);
static UInt32 getTargetQueueLength(FskAudioOut audioOut);
static FskErr audioOutEnqueue(FskAudioOut audioOut, void *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, UInt32 *frameSizes, Boolean silence, FskAudioOutBlock *blockOut);

#if SUPPORT_WIN32_DIRECTSOUND
	static FskErr createDirectSoundBuffer(FskAudioOut audioOut, UInt32 bufferSize, UInt32 bufferCount);
	static void releaseDirectSoundBuffer(FskAudioOut audioOut);
	static void renderAudioBlocks(FskAudioOut audioOut);
	static FskErr updateDirectSoundVolume(FskAudioOut audioOut);
#elif SUPPORT_WIN32_WAVEOUT
	static HANDLE gPendingWaveOut;
	static FskTimeCallBack gPendingWaveOutTimer;
	static void closePendingWaveOut(FskTimeCallBack callback, const FskTime time, void *param);
#endif

#if SUPPORT_INSTRUMENTATION
	static FskInstrumentedTypeRecord gAudioOutInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"audioout",
		FskInstrumentationOffset(FskAudioOutRecord),
		NULL,
		0,
		NULL,
		NULL
	};
#endif
FskInstrumentedTypePrintfsDefine(AudioOut, gAudioOutInstrumentation);

#if !(TARGET_OS_KPL || TARGET_OS_LINUX)

static FskListMutex gAudioOuts;

static FskErr coreAudioOutNew(FskAudioOut *audioOutOut, UInt32 outputID, UInt32 format)
{
	FskErr err = kFskErrNone;
	FskAudioOut audioOut = NULL;

	if (NULL == gAudioOuts) {
		FskAudioOutPrintfDebug("creating mutex gAudioOuts");
		err = FskListMutexNew(&gAudioOuts, "gAudioOuts");
		BAIL_IF_ERR(err);
	}

	err = FskMemPtrNewClear(sizeof(FskAudioOutRecord), (FskMemPtr *)&audioOut);
	BAIL_IF_ERR(err);

	audioOut->thread = FskThreadGetCurrent();
	audioOut->leftVolume = 256;
	audioOut->rightVolume = 256;

	FskInstrumentedItemNew(audioOut, NULL, &gAudioOutInstrumentation);

	err = FskMutexNew(&audioOut->mutex, "audioOut mutex");
	BAIL_IF_ERR(err);

	err = initializePlatformOutput(audioOut, format);
	BAIL_IF_ERR(err);

	FskListMutexPrepend(gAudioOuts, audioOut);

bail:
	if (err) {
		FskAudioOutDispose(audioOut);
		audioOut = NULL;
	}
	*audioOutOut = audioOut;

	return err;
}

static FskErr coreAudioOutDispose(FskAudioOut audioOut)
{
	if (audioOut) {
		FskAudioOutStop(audioOut);

		terminatePlatformOutput(audioOut);

		FskListMutexRemove(gAudioOuts, audioOut);
		if (0 == FskListMutexCount(gAudioOuts)) {
			FskListMutexDispose(gAudioOuts);
			gAudioOuts = NULL;
			FskAudioOutPrintfDebug("disposed mutex gAudioOuts");
		}

		FskMutexDispose(audioOut->mutex);

		FskInstrumentedItemDispose(audioOut);

		FskMemPtrDispose(audioOut);
	}

	return kFskErrNone;
}

static FskErr coreAudioOutIsValid(FskAudioOut audioOut, Boolean *isValid)
{
	FskAudioOut walker = NULL;
	*isValid = false;
	while (gAudioOuts) {
		walker = (FskAudioOut)FskListMutexGetNext(gAudioOuts, walker);
		if (NULL == walker)
			break;
		if (walker == audioOut) {
			*isValid = true;
			break;
		}
	}
	return kFskErrNone;
}

static FskErr coreAudioOutGetFormat(FskAudioOut audioOut, UInt32 *format, UInt16 *numChannels, double *sampleRate)
{
	FskInstrumentedItemPrintfDebug(audioOut, "getFormat");
	if (format) *format = audioOut->format;
	if (numChannels) *numChannels = audioOut->numChannels;
	if (sampleRate) *sampleRate = audioOut->sampleRate;
	return kFskErrNone;
}

#if TARGET_OS_MAC && USE_AUDIO_QUEUE
static FskErr coreAudioOutSetFormat(FskAudioOut audioOut, UInt32 format, UInt16 numChannels, double sampleRate, unsigned char *formatInfo, UInt32 formatInfoSize)
{
	FskInstrumentedItemPrintfDebug(audioOut, "setFormat format=%d, channels=%d, sampleRate=%f, formatInfo=%p", format, (int)numChannels, sampleRate, formatInfo);
    if (!FskCocoaAudioSetFormat(audioOut, format, numChannels, sampleRate, formatInfo, formatInfoSize)) {
        return kFskErrCodecNotFound;
    }
    return kFskErrNone;
}

static FskErr coreAudioHasProperty(FskAudioOut audioOut, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	FskCocoaAudioHasProperty(audioOut, propertyID, get, set, dataType);
	return kFskErrNone;
}

static FskErr coreAudioSetProperty(FskAudioOut audioOut, UInt32 propertyID, FskMediaPropertyValue property)
{
	if (FskCocoaAudioSetProperty(audioOut, propertyID, property))
		return kFskErrNone;
	else
		return kFskErrUnimplemented;
}

static FskErr coreAudioGetProperty(FskAudioOut audioOut, UInt32 propertyID, FskMediaPropertyValue property)
{
	if (FskCocoaAudioGetProperty(audioOut, propertyID, property))
		return kFskErrNone;
	else
		return kFskErrUnimplemented;
}

#endif

#if !(TARGET_OS_MAC && USE_AUDIO_QUEUE)
static FskErr coreAudioOutSetFormatStub(FskAudioOut audioOut, UInt32 format, UInt16 numChannels, double sampleRate, unsigned char *formatInfo, UInt32 formatInfoSize)
{
    return kFskErrNone;
}

#endif

static FskErr coreAudioOutSetOutputBufferSize(FskAudioOut audioOut, UInt32 chunkSize, UInt32 bufferedSamplesTarget)
{
	FskInstrumentedItemPrintfDebug(audioOut, "setOutputBufferSize chunkSize=%d, bufferedSamplesTarget=%d", chunkSize, bufferedSamplesTarget);
	audioOut->chunkRequestSize = chunkSize;
	audioOut->bufferedSamplesTarget = bufferedSamplesTarget;
	return kFskErrNone;
}

static FskErr coreAudioOutGetOutputBufferSize(FskAudioOut audioOut, UInt32 *chunkSize, UInt32 *bufferedSamplesTarget, UInt32 *minimumBytesToStart)
{
	FskInstrumentedItemPrintfDebug(audioOut, "getOutputBufferSize");

	if (NULL != chunkSize)
		*chunkSize = getChunkRequestSize(audioOut);

	// @@ this should be getTargetQueueLength().
	if (NULL != bufferedSamplesTarget) {
#if TARGET_OS_MAC || TARGET_OS_LINUX
		*bufferedSamplesTarget = audioOut->sampleRate * 3;
#else
		*bufferedSamplesTarget = audioOut->sampleRate * 2;
#endif
	}

	if (NULL != minimumBytesToStart)
		*minimumBytesToStart = 0;

	return kFskErrNone;
}

static FskErr coreAudioOutSetVolume(FskAudioOut audioOut, UInt16 left, UInt16 right)
{
	FskInstrumentedItemPrintfDebug(audioOut, "setVolume left=%d, right=%d", (int)left, (int)right);

	audioOut->leftVolume = left;
	audioOut->rightVolume = right;
	{
#if SUPPORT_WIN32_WAVEOUT
	DWORD volume;
	WAVEOUTCAPS caps;

	if (left > 255) left = 255;
	if (right > 255) right = 255;
	volume = (right << 24) | (right << 16) | (left << 8) | left;

	waveOutGetDevCaps((UINT_PTR)audioOut->waveOut, &caps, sizeof(caps));
	if (0 == (caps.dwSupport & WAVECAPS_VOLUME))
		return kFskErrUnimplemented;

	if (0 == (caps.dwSupport & WAVECAPS_LRVOLUME) && (left != right))
		return kFskErrUnimplemented;

	if (0 != waveOutSetVolume(audioOut->waveOut, volume))
		return kFskErrUnimplemented;

	return kFskErrNone;
#elif SUPPORT_WIN32_DIRECTSOUND
	return updateDirectSoundVolume(audioOut);
#elif TARGET_OS_MAC
    FskCocoaAudioSetVolume(audioOut, left, right);

    return kFskErrNone;
#else
	return kFskErrUnimplemented;
#endif
	}
}

static FskErr coreAudioOutGetVolume(FskAudioOut audioOut, UInt16 *left, UInt16 *right)
{
	FskInstrumentedItemPrintfDebug(audioOut, "getVolume");
#if TARGET_OS_WIN32
	if (left) *left = audioOut->leftVolume;
	if (right) *right = audioOut->rightVolume;
	return kFskErrNone;
#elif TARGET_OS_MAC
	FskCocoaAudioGetVolume(audioOut, left, right);

	return kFskErrNone;
#else
	return kFskErrUnimplemented;
#endif

}

static FskErr coreAudioOutSetDoneCallback(FskAudioOut audioOut, FskAudioOutDoneCallback cb, void *refCon)
{
	FskInstrumentedItemPrintfDebug(audioOut, "setDoneCallback");
	audioOut->doneCB = cb;
	audioOut->doneRefCon = refCon;
	return kFskErrNone;
}

static FskErr coreAudioOutSetMoreCallback(FskAudioOut audioOut, FskAudioOutMoreCallback cb, void *refCon)
{
	FskInstrumentedItemPrintfDebug(audioOut, "setMoreCallback");
	audioOut->moreCB = cb;
	audioOut->moreRefCon = refCon;
	return kFskErrNone;
}

static FskErr coreAudioOutStart(FskAudioOut audioOut, FskSampleTime atSample)
{
	FskErr err = kFskErrNone;

	FskInstrumentedItemPrintfDebug(audioOut, "start");

	if (audioOut->playing)
		goto bail;

#if SUPPORT_WIN32_DIRECTSOUND
	err = createDirectSoundBuffer(audioOut, (audioOut->sampleRate * 2 * audioOut->numChannels) / 4, 4);
	BAIL_IF_ERR(err);
#endif

	audioOut->zeroTime = atSample;

	refillQueue(audioOut);
	audioOut->playing = true;

#if TARGET_OS_MAC
    if (!FskCocoaAudioStart(audioOut))
        BAIL(kFskErrOperationFailed);
#elif SUPPORT_WIN32_DIRECTSOUND
	renderAudioBlocks(audioOut);
	IDirectSoundBuffer_Play(audioOut->dSBuffer, 0, 0, DSBPLAY_LOOPING);
#else
	#if SUPPORT_WIN32_WAVEOUT
		audioOut->dontRefill = true;		// to avoid calling refillQueue in the loop below
	#endif
    {
	FskAudioOutBlock walker = (FskAudioOutBlock)audioOut->blocks;
	while (walker) {
		renderAudioBlock(audioOut, walker);
		walker = walker->next;
	}
    }
	#if SUPPORT_WIN32_WAVEOUT
		audioOut->dontRefill = false;
	#endif
#endif

bail:
	return err;
}

static FskErr coreAudioOutStop(FskAudioOut audioOut)
{
	FskErr err = kFskErrNone;

	FskInstrumentedItemPrintfDebug(audioOut, "stop");

	if (!audioOut)
		return err;

	if (!audioOut->playing)
		goto bail;

#if !SUPPORT_WIN32_DIRECTSOUND && !SUPPORT_WIN32_WAVEOUT
	FskMutexAcquire(audioOut->mutex);
#endif

	audioOut->playing = false;

#if SUPPORT_WIN32_WAVEOUT
	waveOutReset(audioOut->waveOut);
	if (audioOut->flushPending)
		removeUnusedFromQueue(audioOut);
#elif SUPPORT_WIN32_DIRECTSOUND
	releaseDirectSoundBuffer(audioOut);
#elif TARGET_OS_MAC
    FskCocoaAudioStop(audioOut);
#endif

#if !SUPPORT_WIN32_DIRECTSOUND && !SUPPORT_WIN32_WAVEOUT
	FskMutexRelease(audioOut->mutex);
#endif

bail:
	removeAllFromQueue(audioOut);

	return err;
}

static FskErr coreAudioOutEnqueue(FskAudioOut audioOut, void *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, UInt32 *frameSizes)
{
	return audioOutEnqueue(audioOut, data, dataSize, dataRefCon, frameCount, frameSizes, false, NULL);
}

static FskErr coreAudioOutGetSamplePosition(FskAudioOut audioOut, FskSampleTime *position)
{
	if (false == audioOut->playing)
		return kFskErrBadState;
	else {
#if SUPPORT_WIN32_WAVEOUT
	MMTIME mmtime;

	mmtime.wType = TIME_BYTES;
	if (MMSYSERR_NOERROR != waveOutGetPosition(audioOut->waveOut, &mmtime, sizeof(mmtime)))
		return kFskErrOperationFailed;

	*position = audioOut->zeroTime + ((FskSampleTime)mmtime.u.cb / (2 * audioOut->numChannels));

	return kFskErrNone;
#elif SUPPORT_WIN32_DIRECTSOUND
	HRESULT hr;
	DWORD playCursor;
	SInt32 unplayed;

	hr = IDirectSoundBuffer_GetCurrentPosition(audioOut->dSBuffer, &playCursor, NULL);
	if (FAILED(hr))
		return kFskErrOperationFailed;

	unplayed = (SInt32)((audioOut->bytesWritten % audioOut->bufferSize) - playCursor);
	if (unplayed < 0)
		unplayed += audioOut->bufferSize;

	*position = audioOut->zeroTime + ((audioOut->bytesWritten - unplayed) / ( 2 * audioOut->numChannels));

	return kFskErrNone;
#elif TARGET_OS_MAC
    FskCocoaAudioGetSamplePosition(audioOut, position);

    return kFskErrNone;
#else
	return kFskErrOperationFailed;
#endif
	}
}

static FskErr coreAudioOutGetSamplesQueued(FskAudioOut audioOut, UInt32 *samplesQueuedOut, UInt32 *targetQueueLengthOut)
{
	if (samplesQueuedOut)
		*samplesQueuedOut = getSamplesQueued(audioOut);

	if (targetQueueLengthOut)
		*targetQueueLengthOut = getTargetQueueLength(audioOut);

	return kFskErrNone;
}

static FskErr coreAudioOutSingleThreadedClient(FskAudioOut audioOut, Boolean *isSingleThreaded)
{
#if SUPPORT_WIN32_WAVEOUT || SUPPORT_WIN32_DIRECTSOUND
	*isSingleThreaded = true;		// we always call into the originating thread, so it is effectively single threaded
#elif TARGET_OS_MAC
	*isSingleThreaded = false;
#else
	*isSingleThreaded = true;
#endif
	return kFskErrNone;
}

#endif /* !(TARGET_OS_KPL || TARGET_OS_LINUX) */

FskErr audioOutEnqueue(FskAudioOut audioOut, void *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, UInt32 *frameSizes, Boolean silence, FskAudioOutBlock *blockOut)
{
	FskErr err;
	FskAudioOutBlock block;

	FskInstrumentedItemPrintfDebug(audioOut, "enqueue data=%p, dataSize=%d, frameCount=%d, silence=%d", data, dataSize, frameCount, (int)silence);

	err = FskMemPtrNewClear(sizeof(FskAudioOutBlockRecord) + ((frameCount && frameSizes) ? sizeof(UInt32) * frameCount : 0), (FskMemPtr*)(void*)(&block));
	BAIL_IF_ERR(err);

	block->data = (unsigned char *)data;
	block->dataSize = dataSize;
	block->sampleCount = dataSize / (2 * audioOut->numChannels);
	block->refCon = dataRefCon;
	block->audioOut = audioOut;
	block->silence = silence;

	block->frameCount = frameCount;
	if (frameCount && frameSizes) {
		FskMemMove(block->frameSizesArray, frameSizes, sizeof(UInt32) * frameCount);
		block->frameSizes = block->frameSizesArray;
	}

	FskMutexAcquire(audioOut->mutex);

	FskListAppend((FskList*)(void*)&audioOut->blocks, (FskList*)(void*)block);

#if SUPPORT_WIN32_WAVEOUT
	if (audioOut->flushPending) {
		removeUnusedFromQueue(audioOut);
		audioOut->flushPending = false;
	}
	if (audioOut->playing)
		renderAudioBlock(audioOut, block);
#elif SUPPORT_WIN32_DIRECTSOUND
	if (audioOut->playing)
		renderAudioBlocks(audioOut);
#elif TARGET_OS_MAC && USE_AUDIO_QUEUE
	if (audioOut->playing)
	  FskCocoaAudioEnqueueBuffers(audioOut);
#endif

	FskMutexRelease(audioOut->mutex);

bail:
	if (blockOut) *blockOut = block;

	return err;
}

UInt32 getSamplesQueued(FskAudioOut audioOut)
{
	UInt32 count = 0;
	FskAudioOutBlock walker = audioOut->blocks;

	while (walker) {
		if (false == walker->done) {
			count += walker->sampleCount;
#if SUPPORT_WIN32_DIRECTSOUND
			count += walker->samplesUsed;
#endif
		}

		walker = walker->next;
	}
	return count;
}

UInt32 getChunkRequestSize(FskAudioOut audioOut)
{
#if TARGET_OS_MAC
    SInt32 chunkRequestSize = audioOut->sampleRate / 4;	// quarter second chunks would be nice
#elif SUPPORT_WIN32_WAVEOUT
	SInt32 chunkRequestSize = audioOut->sampleRate / 15;		// prefer small decompresses to avoid long periods of time decompressing
#elif SUPPORT_WIN32_DIRECTSOUND
	SInt32 chunkRequestSize = audioOut->sampleRate / 4;
#else
	SInt32 chunkRequestSize = audioOut->sampleRate / 4;	// quarter second chunks would be nice
#endif

	if (audioOut->chunkRequestSize)
		chunkRequestSize = audioOut->chunkRequestSize;

	return chunkRequestSize;
}

UInt32 getTargetQueueLength(FskAudioOut audioOut)
{
	SInt32 samplesTarget = audioOut->sampleRate;	// try to keep one second on buffer
#if SUPPORT_WIN32_WAVEOUT
	samplesTarget = (samplesTarget * 3) >> 1;		// same calculation appears in waveOutProc, update both
#endif

	if (audioOut->bufferedSamplesTarget)
		samplesTarget = audioOut->bufferedSamplesTarget;

	return samplesTarget;
}

void refillQueue(FskAudioOut audioOut)
{
	if (audioOut->moreCB) {
		SInt32 samplesTarget = getTargetQueueLength(audioOut);
		SInt32 chunkRequestSize = getChunkRequestSize(audioOut);

		while (true) {
			FskAudioOutBlock block;
			FskMemPtr silence;
			SInt32 samplesNeeded = samplesTarget - getSamplesQueued(audioOut);

			if (samplesNeeded <= 0)
				break;

			if (samplesNeeded > chunkRequestSize)
				samplesNeeded = chunkRequestSize;

			if (kFskErrNone == (audioOut->moreCB)(audioOut, audioOut->moreRefCon, samplesNeeded))
				continue;

			// make silence
			if ((audioOut->format != kFskAudioFormatPCM16BitLittleEndian) && (audioOut->format != kFskAudioFormatPCM16BitBigEndian))
				break;

			if (kFskErrNone != FskMemPtrNewClear(samplesNeeded * 2 * audioOut->numChannels, &silence))
				break;

			if (kFskErrNone != audioOutEnqueue(audioOut, silence, samplesNeeded * 2 * audioOut->numChannels, NULL, 0, NULL, true, &block))
				break;
		}
	}
}

void removeAllFromQueue(FskAudioOut audioOut)
{
	while (true) {
		FskAudioOutBlock block;

		block = (FskAudioOutBlock)FskListRemoveFirst((FskList*)(void*)&audioOut->blocks);
		if (NULL == block) break;

#if SUPPORT_WIN32_WAVEOUT
		if (block->prepared)
			waveOutUnprepareHeader(audioOut->waveOut, &block->waveHdr, sizeof(block->waveHdr));
#endif

		if (block->silence)
			FskMemPtrDispose(block->data);
		else if (audioOut->doneCB)			// && block->refCon
			(audioOut->doneCB)(audioOut, audioOut->doneRefCon, block->refCon, true);
		FskMemPtrDispose(block);
	}
}

void removeUnusedFromQueue(FskAudioOut audioOut)
{
	while (true) {
		FskAudioOutBlock block = audioOut->blocks;

		if ((NULL == block) || (false == block->done))
			break;

		FskListRemove((FskList*)(void*)&audioOut->blocks, block);

#if SUPPORT_WIN32_WAVEOUT
		if (block->prepared)
			waveOutUnprepareHeader(audioOut->waveOut, &block->waveHdr, sizeof(block->waveHdr));
#endif

		if (block->silence)
			FskMemPtrDispose(block->data);
		else if (audioOut->doneCB)
			(audioOut->doneCB)(audioOut, audioOut->doneRefCon, block->refCon, true);

		FskMemPtrDispose(block);
	}
}

#if TARGET_OS_WIN32

static void flushAndRefill(void *arg0, void *arg1, void *arg2, void *arg3);
void flushAndRefill(void *arg0, void *arg1, void *arg2, void *arg3)
{
	FskAudioOut audioOut = (FskAudioOut)arg0;
	Boolean isValid = false;

	if ((kFskErrNone != FskAudioOutIsValid(audioOut, &isValid)) || !isValid)
		return;

	FskMutexAcquire(audioOut->mutex);

	if (false == audioOut->playing)
		goto bail;

	removeUnusedFromQueue(audioOut);
	refillQueue(audioOut);
#if SUPPORT_WIN32_DIRECTSOUND
	if (audioOut->playing)
		renderAudioBlocks(audioOut);
#elif SUPPORT_WIN32_WAVEOUT
	audioOut->flushPending = false;
#endif

bail:
	FskMutexRelease(audioOut->mutex);
}

#endif

#if SUPPORT_WIN32_WAVEOUT

static void flushPending(void *arg0, void *arg1, void *arg2, void *arg3);
void flushPending(void *arg0, void *arg1, void *arg2, void *arg3)
{
	FskAudioOut audioOut = (FskAudioOut)arg0;
	Boolean isValid = false;

	if ((kFskErrNone != FskAudioOutIsValid(audioOut, &isValid)) || !isValid)
		return;

	FskMutexAcquire(audioOut->mutex);

	removeUnusedFromQueue(audioOut);
	audioOut->flushPending = false;

	FskMutexRelease(audioOut->mutex);
}

static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

FskErr initializePlatformOutput(FskAudioOut audioOut, UInt32 audioFormat)
{
	FskErr err = kFskErrNone;
	WAVEFORMATEX format;
	FskMutex mutex = gAudioOuts ? gAudioOuts->mutex : NULL;

	audioOut->sampleRate = 44100;
	audioOut->numChannels = 2;
	audioOut->format = kFskAudioFormatPCM16BitLittleEndian;

	if (mutex) {
		FskMutexAcquire(mutex);

		if (gPendingWaveOut) {
			audioOut->waveOut = gPendingWaveOut;
			gPendingWaveOut = NULL;

			FskTimeCallbackDispose(gPendingWaveOutTimer);
			gPendingWaveOutTimer = NULL;
		}
		FskMutexRelease(mutex);

		if (audioOut->waveOut)
			return kFskErrNone;
	}

	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = audioOut->numChannels;
	format.nSamplesPerSec = audioOut->sampleRate;
	format.nAvgBytesPerSec = audioOut->sampleRate * 2 * audioOut->numChannels;
	format.nBlockAlign = 2 * audioOut->numChannels;
	format.wBitsPerSample = 16;
	format.cbSize = 0;

	if (0 != waveOutOpen(&audioOut->waveOut, WAVE_MAPPER, &format,
		(DWORD_PTR)waveOutProc, (DWORD_PTR)NULL, CALLBACK_FUNCTION))
        BAIL(kFskErrOperationFailed);

bail:
	return err;
}

void terminatePlatformOutput(FskAudioOut audioOut)
{
	FskMutex mutex = gAudioOuts ? gAudioOuts->mutex : NULL;

	if (mutex)
		FskMutexAcquire(mutex);

	if (audioOut->waveOut) {
		if (audioOut->flushPending)
			removeUnusedFromQueue(audioOut);

		if ((NULL == gPendingWaveOut) && !gQuitting) {
			gPendingWaveOut = audioOut->waveOut;
			audioOut->waveOut = NULL;

			FskTimeCallbackNew(&gPendingWaveOutTimer);
			if (gPendingWaveOutTimer)
				FskTimeCallbackScheduleFuture(gPendingWaveOutTimer, 5, 0, closePendingWaveOut, NULL);
		}
		else {
			waveOutClose(audioOut->waveOut);
		}
	}

	if (mutex)
		FskMutexRelease(mutex);
}

void closePendingWaveOut(FskTimeCallBack callback, const FskTime time, void *param)
{
	FskMutex mutex = gAudioOuts ? gAudioOuts->mutex : NULL;

	if (mutex)
		FskMutexAcquire(mutex);

	FskTimeCallbackDispose(gPendingWaveOutTimer);
	gPendingWaveOutTimer = NULL;

	if (gPendingWaveOut) {
		waveOutClose(gPendingWaveOut);
		gPendingWaveOut = NULL;
	}

	if (mutex)
		FskMutexRelease(mutex);
}

void renderAudioBlock(FskAudioOut audioOut, FskAudioOutBlock block)
{
	block->prepared = true;
	block->waveHdr.lpData = block->data;
	block->waveHdr.dwBufferLength = block->dataSize;
	if (0 == waveOutPrepareHeader(audioOut->waveOut, &block->waveHdr, sizeof(block->waveHdr)))
		waveOutWrite(audioOut->waveOut, &block->waveHdr, sizeof(block->waveHdr));
	else
		block->prepared = false;
}

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if (WOM_DONE == uMsg) {
		WAVEHDR *waveHdr = (WAVEHDR *)dwParam1;
		FskAudioOutBlock block = (FskAudioOutBlock)(((unsigned char *)waveHdr) - offsetof(FskAudioOutBlockRecord, waveHdr));
		FskAudioOut audioOut = block->audioOut;

		FskMutexAcquire(audioOut->mutex);

		block->done = true;

		if (audioOut->playing && !audioOut->dontRefill && (getSamplesQueued(audioOut) <= ((audioOut->sampleRate * 3) >> 1))) {		// same calculation appears in refillQueue, update both
			refillQueue(audioOut);
			audioOut->flushPending = true;
		}

		FskMutexRelease(audioOut->mutex);
	}
}

#elif SUPPORT_WIN32_DIRECTSOUND

static DWORD WINAPI directSoundProc(LPVOID lpParameter);

FskErr initializePlatformOutput(FskAudioOut audioOut, UInt32 audioFormat)
{
	FskErr err = kFskErrNone;
	LPDIRECTSOUND8 lpDirectSound = NULL;
	HRESULT hr;

	audioOut->sampleRate = 44100;
	audioOut->numChannels = 2;
	audioOut->format = kFskAudioFormatPCM16BitLittleEndian;

	hr = DirectSoundCreate8(NULL, &lpDirectSound, NULL);
	if (FAILED(hr))
		BAIL(kFskErrOperationFailed);

	hr = IDirectSound8_SetCooperativeLevel(lpDirectSound, gUtilsWnd, DSSCL_PRIORITY);
	if (FAILED(hr))
		BAIL(kFskErrOperationFailed);

bail:
	if (err && (NULL != lpDirectSound)) {
		IDirectSound8_Release(lpDirectSound);
		lpDirectSound = NULL;
	}

	audioOut->dS = lpDirectSound;

	return err;
}

void terminatePlatformOutput(FskAudioOut audioOut)
{
	if (NULL != audioOut->dS)
		releaseDirectSoundBuffer(audioOut);

	if (NULL != audioOut->dS)
		IDirectSound8_Release(audioOut->dS);
}

FskErr createDirectSoundBuffer(FskAudioOut audioOut, UInt32 bufferSize, UInt32 bufferCount)
{
	HRESULT hr = S_OK;
	DSBUFFERDESC dsbdsc;
	WAVEFORMATEX format;
	HANDLE hNotifyEvent = NULL, hEndThread = NULL;
	LPDIRECTSOUNDBUFFER8 lpDirectSoundBuffer = NULL;
	DWORD i, threadID;
	LPDSBPOSITIONNOTIFY aPosNotify = NULL;
	LPDIRECTSOUNDNOTIFY pDSNotify = NULL;
	LPDIRECTSOUNDBUFFER lpDsb = NULL;
	FskErr err = kFskErrNone;

	format.wFormatTag = WAVE_FORMAT_PCM;
	format.wBitsPerSample = 16;/* bitrate??*/
	format.nChannels = audioOut->numChannels;
	format.nSamplesPerSec = audioOut->sampleRate;
	format.nBlockAlign = format.wBitsPerSample / 8 * audioOut->numChannels;
	format.nAvgBytesPerSec = format.nBlockAlign * format.nSamplesPerSec;
	format.cbSize = 0;

	hNotifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	hEndThread = CreateEvent(NULL, FALSE, FALSE, NULL);

	err = FskMemPtrNew(sizeof(DSBPOSITIONNOTIFY) * bufferCount, (FskMemPtr *)&aPosNotify);
	BAIL_IF_ERR(err);

	for (i = 0; i < bufferCount; i++) {
        aPosNotify[i].dwOffset = ((DWORD)bufferSize * i) + (DWORD)bufferSize - 1;
        aPosNotify[i].hEventNotify = hNotifyEvent;
    }

	ZeroMemory(&dsbdsc, sizeof(dsbdsc));

	dsbdsc.dwSize = sizeof(dsbdsc);
	dsbdsc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME| DSBCAPS_CTRLPAN;
	dsbdsc.dwBufferBytes = bufferSize * bufferCount;
    dsbdsc.lpwfxFormat = &format;

	hr = IDirectSound8_CreateSoundBuffer(audioOut->dS, &dsbdsc, &lpDsb, NULL);
	if (FAILED(hr)) goto bail;

#if !defined(__cplusplus)
	hr = IDirectSound8_QueryInterface(lpDsb, &IID_IDirectSoundBuffer8, (LPVOID *)&lpDirectSoundBuffer);
	if (FAILED(hr)) goto bail;

	hr = IDirectSoundBuffer_QueryInterface(lpDsb, &IID_IDirectSoundNotify, (LPVOID *)&pDSNotify);
	if (FAILED(hr)) goto bail;
#else
	hr = IDirectSound8_QueryInterface(lpDsb, (const IID &)IID_IDirectSoundBuffer8, (LPVOID *)&lpDirectSoundBuffer);
	if (FAILED(hr)) goto bail;

	hr = IDirectSoundBuffer_QueryInterface(lpDsb, (const IID &)IID_IDirectSoundNotify, (LPVOID *)&pDSNotify);
	if (FAILED(hr)) goto bail;
#endif

	hr = IDirectSoundBuffer_SetCurrentPosition(lpDirectSoundBuffer, 0);
	if (FAILED(hr)) goto bail;

	hr = IDirectSoundNotify_SetNotificationPositions(pDSNotify, bufferCount, aPosNotify);
	if (FAILED(hr)) goto bail;

	audioOut->dSBuffer = lpDirectSoundBuffer;
	audioOut->hNotify = hNotifyEvent;
	audioOut->hEndThread = hEndThread;
	audioOut->bufferSize = bufferSize * bufferCount;
	audioOut->bufferCount = bufferCount;
	audioOut->samplesNeeded = (audioOut->bufferSize / (2 * audioOut->numChannels)) - kDirectSoundRefillSlop;
	audioOut->bytesWritten = 0;

	audioOut->hNotifyThread = CreateThread(NULL, 0, directSoundProc, audioOut, 0, &threadID);
	if (NULL == audioOut->hNotifyThread)
        BAIL(kFskErrOperationFailed);

	updateDirectSoundVolume(audioOut);

bail:
	if ((kFskErrNone == err) && FAILED(hr))
		err = kFskErrOperationFailed;

	if (kFskErrNone != err) {
		if (lpDirectSoundBuffer)
			IDirectSoundBuffer_Release(lpDirectSoundBuffer);

		if (hEndThread)
			CloseHandle(hEndThread);

		if (hNotifyEvent)
			CloseHandle(hNotifyEvent);

		if (audioOut->hNotifyThread) {
			SetEvent(audioOut->hNotifyThread);
			WaitForSingleObject(audioOut->hNotifyThread, INFINITE);
			audioOut->hNotifyThread = NULL;
		}
	}

	if (pDSNotify)
		IDirectSoundNotify_Release(pDSNotify);

	if (lpDsb)
		IDirectSoundBuffer_Release(lpDsb);

	FskMemPtrDispose(aPosNotify);

	return err;
}

void releaseDirectSoundBuffer(FskAudioOut audioOut)
{
	if (NULL != audioOut->dSBuffer)
		IDirectSoundBuffer_Stop(audioOut->dSBuffer);

	if (audioOut->hEndThread)
		SetEvent(audioOut->hEndThread);

	if (audioOut->hNotifyThread) {
		WaitForSingleObject(audioOut->hNotifyThread, INFINITE);
		CloseHandle(audioOut->hNotifyThread);
		audioOut->hNotifyThread = NULL;
	}

	if (NULL != audioOut->dSBuffer) {
		IDirectSoundBuffer_Release(audioOut->dSBuffer);
		audioOut->dSBuffer = NULL;
	}

	if (NULL != audioOut->hNotify) {
		CloseHandle(audioOut->hNotify);
		audioOut->hNotify = NULL;
	}

	if (NULL != audioOut->hEndThread) {
		CloseHandle(audioOut->hEndThread);
		audioOut->hEndThread = NULL;
	}
}

// push as much audio as we can into DirectSound
void renderAudioBlocks(FskAudioOut audioOut)
{
	FskAudioOutBlock block;

	FskMutexAcquire(audioOut->mutex);

	for (block = audioOut->blocks; (NULL != block) && (0 != audioOut->samplesNeeded); block = block->next) {
		LPBYTE p1, p2;
		DWORD b1, b2;
		HRESULT hr;
		UInt32 samplesToUse;
		unsigned char *data = block->data;

		if (block->done)
			break;

		samplesToUse = block->sampleCount - block->samplesUsed;

		if (samplesToUse > audioOut->samplesNeeded)
			samplesToUse = audioOut->samplesNeeded;

		data += (block->samplesUsed * audioOut->numChannels * 2);

		hr = IDirectSoundBuffer_Lock(audioOut->dSBuffer, (UInt32)(audioOut->bytesWritten % audioOut->bufferSize), (DWORD)(samplesToUse * 2 * audioOut->numChannels),
			(LPVOID *)&p1, &b1, (LPVOID *)&p2, &b2, 0);
		if (FAILED(hr))
			break;

		if (NULL != p1) {
			memcpy(p1, data, b1);
			if (NULL != p2)
				memcpy(p2, b1 + data, b2);

			audioOut->samplesNeeded -= samplesToUse;
			audioOut->bytesWritten += samplesToUse * 2 * audioOut->numChannels;

			block->samplesUsed += samplesToUse;
			block->done = (block->samplesUsed >= block->sampleCount);
		}

		IDirectSoundBuffer_Unlock(audioOut->dSBuffer, p1, b1, p2, b2);
	}

	FskMutexRelease(audioOut->mutex);
}

DWORD WINAPI directSoundProc(LPVOID lpParameter)
{
	HANDLE handles[2];
	FskAudioOut audioOut = (FskAudioOut)lpParameter;

	if (NULL == audioOut)
		return 0;

	handles[0] = audioOut->hEndThread;
	handles[1] = audioOut->hNotify;
	while (true) {
		DWORD waitObject = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
		if (WAIT_OBJECT_0 == waitObject)
			break;

		FskMutexAcquire(audioOut->mutex);

		if (false == audioOut->playing) {
			FskMutexRelease(audioOut->mutex);
			continue;
		}


			audioOut->samplesNeeded += ((audioOut->bufferSize / (2 * audioOut->numChannels)) / audioOut->bufferCount) - kDirectSoundRefillSlop;

			// apparently sometimes DirectSound misses a callback (documented by microsoft!), so we check the position
			{
			HRESULT hr;
			DWORD playCursor;

			hr = IDirectSoundBuffer_GetCurrentPosition(audioOut->dSBuffer, &playCursor, NULL);
			if (SUCCEEDED(hr)) {
				SInt32 unplayed = (SInt32)((audioOut->bytesWritten % audioOut->bufferSize) - playCursor);

				if (unplayed < 0)
					unplayed += audioOut->bufferSize;

				audioOut->samplesNeeded = ((audioOut->bufferSize - unplayed) / (2 * audioOut->numChannels)) - kDirectSoundRefillSlop;
			}
			}
		FskMutexRelease(audioOut->mutex);

		FskThreadPostCallback(audioOut->thread, flushAndRefill, audioOut, NULL, NULL, NULL);
	}

	return 1;
}

FskErr updateDirectSoundVolume(FskAudioOut audioOut)
{
	HRESULT hr;
	LONG dsVol = DSBVOLUME_MIN, dsPan;
	UInt16 lr = audioOut->leftVolume > audioOut->rightVolume ? audioOut->leftVolume : audioOut->rightVolume;

	if (NULL == audioOut->dSBuffer)
		return kFskErrNone;

	if (lr) {
		// convert to dB
		dsVol = (LONG)(20.0 * log(((double)lr) / 256.0) * 100.0);
		if (dsVol > DSBVOLUME_MAX)
			dsVol = DSBVOLUME_MAX;
		else if (dsVol < DSBVOLUME_MIN)
			dsVol = DSBVOLUME_MIN;
	}

	if (0 != audioOut->leftVolume)
		dsPan = (LONG)(20.0 * log((double)audioOut->rightVolume / (double)audioOut->leftVolume) * 100);
	else if (0 != audioOut->rightVolume)
		dsPan = DSBPAN_RIGHT;
	else
		dsPan = 0;

	if (dsPan > DSBPAN_RIGHT)
		dsPan = DSBPAN_RIGHT;
	if (dsPan < DSBPAN_LEFT)
		dsPan = DSBPAN_LEFT;

	hr = IDirectSoundBuffer_SetVolume(audioOut->dSBuffer, dsVol);
	if (FAILED(hr))
		return kFskErrInvalidParameter;

	hr = IDirectSoundBuffer_SetPan(audioOut->dSBuffer, dsPan);
	if (FAILED(hr))
		return kFskErrInvalidParameter;

	return kFskErrNone;
}

#elif TARGET_OS_LINUX || TARGET_OS_KPL

// now in extension

#elif TARGET_OS_MAC
    FskErr initializePlatformOutput(FskAudioOut audioOut, UInt32 format)
    {
        FskErr err = kFskErrNone;

        if (!FskCocoaAudioInitialize(audioOut))
            err = kFskErrOperationFailed;

        return err;
    }

    void terminatePlatformOutput(FskAudioOut audioOut)
    {
        FskCocoaAudioTerminate(audioOut);
    }
#else
	FskErr initializePlatformOutput(FskAudioOut audioOut, UInt32 format) { return kFskErrUnimplemented; }
	void terminatePlatformOutput(FskAudioOut audioOut) {}
#endif



#if TARGET_OS_LINUX || TARGET_OS_KPL
static AudioOutVectorSet audioVectors = NULL;
#else
FskErr FskAudioOutGetVolume(FskAudioOut audioOut, UInt16 *left, UInt16 *right);
FskErr FskAudioOutSetVolume(FskAudioOut audioOut, UInt16 left, UInt16 right);


static AudioOutVectors avecRec = {
	coreAudioOutNew,
	coreAudioOutDispose,
	coreAudioOutIsValid,
	coreAudioOutGetFormat,
#if TARGET_OS_MAC && USE_AUDIO_QUEUE
	coreAudioOutSetFormat,
#else
	coreAudioOutSetFormatStub,			//FskAudioOutSetFormat,
#endif
	coreAudioOutSetOutputBufferSize,
	coreAudioOutGetVolume,
	coreAudioOutSetVolume,
	coreAudioOutSetDoneCallback,
	coreAudioOutSetMoreCallback,
    coreAudioOutStart,
	coreAudioOutStop,
	coreAudioOutEnqueue,
	coreAudioOutGetSamplePosition,
	coreAudioOutGetSamplesQueued,
	coreAudioOutSingleThreadedClient,
	NULL,							// FskAudioOutGetDeviceHandle
	coreAudioOutGetOutputBufferSize,
#if TARGET_OS_MAC && USE_AUDIO_QUEUE
	coreAudioHasProperty,
	coreAudioGetProperty,
	coreAudioSetProperty,
#else
	NULL, NULL, NULL,
#endif
};
static AudioOutVectorSet audioVectors = &avecRec;
#endif

void FskAudioOutSetVectors(AudioOutVectorSet vectors) {
	audioVectors = vectors;
}

void FskAudioOutGetVectors(AudioOutVectorSet *vectors) {
	*vectors = audioVectors;
}

FskErr FskAudioOutNew(FskAudioOut *audioOut, UInt32 outputID, UInt32 format) {
#if TARGET_OS_KPL
	if (NULL == audioVectors) {
		FskAudioKplInitialize();
	}
#endif
	if ((NULL == audioVectors) || (NULL == audioVectors->doNew))
		return kFskErrUnimplemented;
	return (*audioVectors->doNew)(audioOut, outputID, format);
}

FskErr FskAudioOutDispose(FskAudioOut audioOut) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doDispose))
		return kFskErrUnimplemented;
	return (*audioVectors->doDispose)(audioOut);
}


FskErr FskAudioOutIsValid(FskAudioOut audioOut, Boolean *isValid) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doIsValid))
		return kFskErrUnimplemented;
	return (*audioVectors->doIsValid)(audioOut, isValid);
}


FskErr FskAudioOutGetFormat(FskAudioOut audioOut, UInt32 *format, UInt16 *numChannels, double *sampleRate) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doGetFormat))
		return kFskErrUnimplemented;
	return (*audioVectors->doGetFormat)(audioOut, format, numChannels, sampleRate);
}

FskErr FskAudioOutSetFormat(FskAudioOut audioOut, UInt32 format, UInt16 numChannels, double sampleRate, unsigned char *formatInfo, UInt32 formatInfoSize) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doSetFormat))
		return kFskErrUnimplemented;
	return (*audioVectors->doSetFormat)(audioOut, format, numChannels, sampleRate, formatInfo, formatInfoSize);
}

FskErr FskAudioOutSetOutputBufferSize(FskAudioOut audioOut, UInt32 chunkSize, UInt32 bufferedSamplesTarget) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doSetOutputBufferSize))
		return kFskErrUnimplemented;
	return (*audioVectors->doSetOutputBufferSize)(audioOut, chunkSize, bufferedSamplesTarget);
}

FskErr FskAudioOutGetOutputBufferSize(FskAudioOut audioOut, UInt32 *chunkSize, UInt32 *bufferedSamplesTarget, UInt32 *minimumBytesToStart) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doGetOutputBufferSize))
		return kFskErrUnimplemented;
	return (*audioVectors->doGetOutputBufferSize)(audioOut, chunkSize, bufferedSamplesTarget, minimumBytesToStart);
}

FskErr FskAudioSetVolume(FskAudioOut audioOut, UInt16 left, UInt16 right) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doSetVolume))
		return kFskErrUnimplemented;
	return (*audioVectors->doSetVolume)(audioOut, left, right);
}

FskErr FskAudioGetVolume(FskAudioOut audioOut, UInt16 *left, UInt16 *right) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doGetVolume))
		return kFskErrUnimplemented;
	return (*audioVectors->doGetVolume)(audioOut, left, right);
}


FskErr FskAudioOutSetDoneCallback(FskAudioOut audioOut, FskAudioOutDoneCallback cb, void *refCon) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doSetDoneCallback))
		return kFskErrUnimplemented;
	return (*audioVectors->doSetDoneCallback)(audioOut, cb, refCon);
}

FskErr FskAudioOutSetMoreCallback(FskAudioOut audioOut, FskAudioOutMoreCallback cb, void *refCon) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doSetMoreCallback))
		return kFskErrUnimplemented;
	return (*audioVectors->doSetMoreCallback)(audioOut, cb, refCon);
}


FskErr FskAudioOutStart(FskAudioOut audioOut, FskSampleTime atSample) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doStart))
		return kFskErrUnimplemented;
	return (*audioVectors->doStart)(audioOut, atSample);
}

FskErr FskAudioOutStop(FskAudioOut audioOut) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doStop))
		return kFskErrUnimplemented;
	return (*audioVectors->doStop)(audioOut);
}


FskErr FskAudioOutEnqueue(FskAudioOut audioOut, void *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, UInt32 *frameSizes) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doEnqueue))
		return kFskErrUnimplemented;
	return (*audioVectors->doEnqueue)(audioOut, data, dataSize, dataRefCon, frameCount, frameSizes);
}

FskErr FskAudioOutGetSamplePosition(FskAudioOut audioOut, FskSampleTime *position) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doGetSamplePosition))
		return kFskErrUnimplemented;
	return (*audioVectors->doGetSamplePosition)(audioOut, position);
}

FskErr FskAudioOutGetSamplesQueued(FskAudioOut audioOut, UInt32 *samplesQueued, UInt32 *targetQueueLength) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doGetSamplesQueued))
		return kFskErrUnimplemented;
	return (*audioVectors->doGetSamplesQueued)(audioOut, samplesQueued, targetQueueLength);
}


FskErr FskAudioOutSingleThreadedClient(FskAudioOut audioOut, Boolean *isSingleThreaded) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doSingleThreadedClient))
		return kFskErrUnimplemented;
	return (*audioVectors->doSingleThreadedClient)(audioOut, isSingleThreaded);
}

FskErr FskAudioOutGetDeviceHandle(FskAudioOut audioOut, void *handle) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doGetDeviceHandle))
		return kFskErrUnimplemented;
	return (*audioVectors->doGetDeviceHandle)(audioOut, handle);
}

FskErr FskAudioOutHasProperty(FskAudioOut audioOut, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doHasProperty))
		return kFskErrUnimplemented;
	return (*audioVectors->doHasProperty)(audioOut, propertyID, get, set, dataType);
}

FskErr FskAudioOutSetProperty(FskAudioOut audioOut, UInt32 propertyID, FskMediaPropertyValue property) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doSetProperty))
		return kFskErrUnimplemented;
	return (*audioVectors->doSetProperty)(audioOut, propertyID, property);
}

FskErr FskAudioOutGetProperty(FskAudioOut audioOut, UInt32 propertyID, FskMediaPropertyValue property) {
	if ((NULL == audioVectors) || (NULL == audioVectors->doGetProperty))
		return kFskErrUnimplemented;
	return (*audioVectors->doGetProperty)(audioOut, propertyID, property);
}

#if TARGET_OS_MAC

void FskAudioOutRefillQueue(FskAudioOut audioOut)
{
	if (audioOut)
		refillQueue(audioOut);
}

void FskAudioOutRemoveUnusedFromQueue(FskAudioOut audioOut)
{
	if (audioOut)
		removeUnusedFromQueue(audioOut);
}

#endif


typedef struct {
	void				*next;
	void				*data;
	UInt32				dataSize;
	UInt32				frameCount;
	FskSndChannelBlock	queueBlock;
	UInt32				numChannels;
	UInt32				format;
	UInt32				sampleRate;
} FskSndChannelPlayBlockRecord, *FskSndChannelPlayBlock;

static void updateVolume(FskSndChannel sndChan);
static void sndChannelDone(struct FskAudioOutRecord *audioOut, void *refCon, void *dataRefCon, Boolean played);
static void sndChannelThreadSafeDone(struct FskAudioOutRecord *audioOut, void *refCon, void *dataRefCon, Boolean played);
static void sndChannelFreePlayBlockContents(FskSndChannel sndChan, FskSndChannelPlayBlock block);
static void sndChannelDisposeQueueBlock(FskSndChannel sndChan, FskSndChannelBlock block);
static FskErr sndChannelMore(struct FskAudioOutRecord *audioOut, void *refCon, UInt32 requestedSamples);
static void doSndChannelRefill(void *arg1, void *arg2, void *arg3, void *arg4);
static FskErr sndChannelThreadSafeMore(struct FskAudioOutRecord *audioOut, void *refCon, UInt32 requestedSamples);
static FskErr processBlock(FskSndChannel sndChan, FskSndChannelBlock block, UInt32 requestedSamples, UInt32 *samplesProcessed, unsigned char **output, UInt32 *outputSize);
static void doSndChannelMoreEmpty(void *a1, void *a2, void *a3, void *a4);
static void sndChannelRefill(FskTimeCallBack callback, const FskTime time, void *param);
static void flushPlayBlocks(FskSndChannel sndChan);

static FskErr sndChannelSetTRInfo(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr sndChannelSetPlayRate(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr sndChannelSetEQ(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr sndChannelGetBufferDuration(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr sndChannelGetSampleRate(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr sndChannelGetFlags(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr sndChannelSetHibernate(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr sndChannelSetAudioCategory(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord gSndChannelProperties[] = {
	{kFskMediaPropertyTRInfo,			kFskMediaPropertyTypeData,		NULL,							sndChannelSetTRInfo},
	{kFskMediaPropertyPlayRate,			kFskMediaPropertyTypeFloat,		NULL,							sndChannelSetPlayRate},
	{kFskMediaPropertyEQ,				kFskMediaPropertyTypeString,	NULL,							sndChannelSetEQ},
	{kFskMediaPropertyBufferDuration,	kFskMediaPropertyTypeInteger,	sndChannelGetBufferDuration,	NULL},
	{kFskMediaPropertySampleRate,		kFskMediaPropertyTypeInteger,	sndChannelGetSampleRate,		NULL},
	{kFskMediaPropertyFlags,			kFskMediaPropertyTypeInteger,	sndChannelGetFlags,				NULL},
	{kFskMediaPropertyHibernate,		kFskMediaPropertyTypeBoolean,	NULL,							sndChannelSetHibernate},
	{kFskMediaPropertyAudioCategory,	kFskMediaPropertyTypeInteger,	NULL,	sndChannelSetAudioCategory},
	{kFskMediaPropertyUndefined,		kFskMediaPropertyTypeUndefined,	NULL,							NULL}
};

static FskListMutex gSndChannels = NULL;

#if SUPPORT_INSTRUMENTATION
	static FskInstrumentedTypeRecord gSndChannelTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"sndchannel",
		FskInstrumentationOffset(FskSndChannelRecord),
		NULL,
		0,
		NULL,
		NULL
	};
#endif

FskErr FskSndChannelNew(FskSndChannel *sndChanOut, UInt32 outputID, UInt32 outputFormat, UInt32 flags)
{
	FskErr err;
	FskSndChannel sndChan;
	double sampleRate;
	Boolean singleThreaded;

	err = FskMemPtrNewClear(sizeof(FskSndChannelRecord), &sndChan);
	BAIL_IF_ERR(err);

	if (NULL == gSndChannels) {
		err = FskListMutexNew(&gSndChannels, "gSndChannels");
		BAIL_IF_ERR(err);
        FskAssert(NULL != gSndChannels);
	}

	sndChan->thread = FskThreadGetCurrent();
	FskInstrumentedItemNew(sndChan, NULL, &gSndChannelTypeInstrumentation);
	FskMutexAcquire(gSndChannels->mutex);

	while (true) {
		FskSndChannel walker;
		Boolean killedOne = false;

		err = FskAudioOutNew(&sndChan->audioOut, outputID, outputFormat);
		if (kFskErrNone == err)
			break;

		for (walker = (FskSndChannel)gSndChannels->list; NULL != walker; walker = walker->next) {
			if ((NULL != walker->abortCB) && (walker->outputID == outputID) && (sndChan->thread == walker->thread))  {
				FskSndChannelAbortCallback abortCB = walker->abortCB;
				void *abortRefCon = walker->abortRefCon;
				(abortCB)(walker, abortRefCon, kFskErrShutdown);
				FskSndChannelDispose(walker);
				killedOne = true;
				break;
			}
		}

		if (false == killedOne)
			break;
	}

	FskListAppend(&gSndChannels->list, sndChan);

	FskMutexRelease(gSndChannels->mutex);

	BAIL_IF_ERR(err);

	sndChan->outputID = outputID;
	sndChan->playRate = 1.0;

	err = FskAudioOutGetFormat(sndChan->audioOut, &sndChan->outFormat, &sndChan->outNumChannels, &sampleRate);
	BAIL_IF_ERR(err);
	sndChan->outSampleRate = (UInt32)sampleRate;

	err = FskAudioOutSingleThreadedClient(sndChan->audioOut, &singleThreaded);
	if (kFskErrNone != err) {
		singleThreaded = true;
		err = kFskErrNone;
	}

	if (singleThreaded)
		flags |= kFskSndChannelThreadSafeClient;				// client can be thread safe or not, as the audio output will always call in a single thread

	sndChan->threadSafeClient = 0 != (flags & kFskSndChannelThreadSafeClient);

	if (sndChan->threadSafeClient) {
		FskAudioOutSetDoneCallback(sndChan->audioOut, sndChannelThreadSafeDone, sndChan);
		FskAudioOutSetMoreCallback(sndChan->audioOut, sndChannelThreadSafeMore, sndChan);
	}
	else {
		FskAudioOutSetDoneCallback(sndChan->audioOut, sndChannelDone, sndChan);
		FskAudioOutSetMoreCallback(sndChan->audioOut, sndChannelMore, sndChan);

		FskTimeCallbackNew(&sndChan->refillCallback);
		FskListMutexNew(&sndChan->playBlocks, "sndChan playBlocks");
	}

	FskMutexNew(&sndChan->mutex, "sndChan mutex");
	FskMutexNew(&sndChan->mutexBlocks, "sndChan mutexBlocks");

	sndChan->volume = 1.0;
	updateVolume(sndChan);

bail:
	if (err) {
		FskSndChannelDispose(sndChan);
		sndChan = NULL;
	}

	*sndChanOut = sndChan;

	return err;
}

FskErr FskSndChannelDispose(FskSndChannel sndChan)
{
	if (sndChan) {
		FskListMutexRemove(gSndChannels, sndChan);

		if (NULL == FskListMutexGetNext(gSndChannels, NULL)) {
			FskListMutexDispose(gSndChannels);
			gSndChannels = NULL;
		}

		FskSndChannelStop(sndChan);
		FskTimeCallbackDispose(sndChan->refillCallback);
		FskAudioDecompressDispose(sndChan->deco);	// This needs to happen before disposing the audio out on S60
		FskAudioOutDispose(sndChan->audioOut);
		FskMemPtrDispose(sndChan->inMIME);
		FskMemPtrDispose(sndChan->inFormatInfo);
		FskAudioFilterDispose(sndChan->rateConverter);
		FskAudioFilterDispose(sndChan->timeScaler);
		FskAudioFilterDispose(sndChan->toneController);
		FskMemPtrDispose(sndChan->toneSettings);
		FskListMutexDispose(sndChan->playBlocks);
		FskMediaPropertyEmpty(&sndChan->trInfo);
		FskMutexDispose(sndChan->mutex);
		FskMutexDispose(sndChan->mutexBlocks);
		FskInstrumentedItemDispose(sndChan);
		FskMemPtrDispose(sndChan);
	}
	return kFskErrNone;
}

FskErr FskSndChannelSetFormat(FskSndChannel sndChan, UInt32 format, const char *mime, UInt16 numChannels, double sampleRate,
					unsigned char *formatInfo, UInt32 formatInfoSize)
{
	FskErr err = kFskErrNone;

	FskSndChannelStop(sndChan);

	if (formatInfoSize) {
		err = FskMemPtrNewFromData(formatInfoSize, formatInfo, (FskMemPtr *)&formatInfo);
		BAIL_IF_ERR(err);
	}

	FskMemPtrDispose(sndChan->inFormatInfo);
	FskAudioFilterDispose(sndChan->rateConverter);
	sndChan->rateConverter = NULL;
	FskAudioFilterDispose(sndChan->timeScaler);
	sndChan->timeScaler = NULL;
	FskAudioFilterDispose(sndChan->toneController);
	sndChan->toneController = NULL;
	FskMemPtrDisposeAt((void**)(void*)&sndChan->inMIME);

	if ((kFskAudioFormatUndefined == format) && (NULL != mime))
		FskAudioMIMEToFormat(mime, &format);

	if (sndChan->audioOut) {
        err = FskAudioOutSetFormat(sndChan->audioOut, format, numChannels, sampleRate, formatInfo, formatInfoSize);
        if (err)
            err = FskAudioOutSetFormat(sndChan->audioOut, format, 2, 44100, formatInfo, formatInfoSize);
		if (kFskErrNone == err) {
			double theRate;
			err = FskAudioOutGetFormat(sndChan->audioOut, &sndChan->outFormat, &sndChan->outNumChannels, &theRate);
            BAIL_IF_ERR(err);
			sndChan->outSampleRate = (UInt32)theRate;
		}
	}
	sndChan->inFormat = format;
	sndChan->inNumChannels = numChannels;
	sndChan->inSampleRate = (UInt32)sampleRate;
	sndChan->inFormatInfo = formatInfo;
	sndChan->inFormatInfoSize = formatInfoSize;
	sndChan->inMIME = FskStrDoCopy(mime);

bail:
	return err;
}

FskErr FskSndChannelGetFormat(FskSndChannel sndChan, UInt32 *format, char **mime, UInt16 *numChannels, double *sampleRate,
					unsigned char **formatInfo, UInt32 *formatInfoSize)
{
	FskErr err = kFskErrNone;

	if (formatInfo) {
		*formatInfo = NULL;
		*formatInfoSize = sndChan->inFormatInfoSize;
		if (sndChan->inFormatInfo) {
			err = FskMemPtrNewFromData(sndChan->inFormatInfoSize, sndChan->inFormatInfo, (FskMemPtr *)formatInfo);
			BAIL_IF_ERR(err);
		}
	}

	if (format) *format = sndChan->inFormat;
	if (numChannels) *numChannels = sndChan->inNumChannels;
	if (sampleRate) *sampleRate = (double)sndChan->inSampleRate;
	if (mime) *mime = FskStrDoCopy(sndChan->inMIME);

bail:
	return err;
}

FskErr FskSndChannelSetVolume(FskSndChannel sndChan, float volume)
{
	sndChan->volume = volume;

	updateVolume(sndChan);

	return kFskErrNone;
}

FskErr FskSndChannelGetVolume(FskSndChannel sndChan, float *volume)
{
	*volume = sndChan->volume;

	return kFskErrNone;
}

FskErr FskSndChannelSetPan(FskSndChannel sndChan, SInt32 pan)
{
	sndChan->pan = pan;

	updateVolume(sndChan);

	return kFskErrNone;
}

FskErr FskSndChannelGetPan(FskSndChannel sndChan, SInt32 *pan)
{
	*pan = sndChan->pan;

	return kFskErrNone;
}

FskErr FskSndChannelEnqueue(FskSndChannel sndChan, void *data, UInt32 dataSize, UInt32 frameCount, UInt32 samplesPerFrame, void *dataRefCon, UInt32 *frameSizes)
{
	FskErr err;
	FskSndChannelBlock block;

	err = FskMemPtrNewClear(sizeof(FskSndChannelBlockRecord) + (frameSizes ? sizeof(UInt32) * frameCount : 0), &block);
	BAIL_IF_ERR(err);

	block->data = (unsigned char *)data;
	block->dataSize = dataSize;
	block->frameCount = frameCount;
	block->samplesPerFrame = samplesPerFrame;
	block->refCon = dataRefCon;
	block->sndChan = sndChan;
	if (NULL != frameSizes) {
		block->frameSizes = &block->dummy;
		FskMemMove(&block->dummy, frameSizes, sizeof(UInt32) * frameCount);
	}

	FskMutexAcquire(sndChan->mutexBlocks);
		FskListAppend((FskList*)(void*)&sndChan->blocks, block);
	FskMutexRelease(sndChan->mutexBlocks);

bail:
	return err;
}

FskErr FskSndChannelEnqueueWithSkip(FskSndChannel sndChan, void *data, UInt32 dataSize, UInt32 frameCount, UInt32 samplesPerFrame, void *dataRefCon, UInt32 *frameSizes, SInt32 samplesToSkip)
{
	FskErr err;
	FskSndChannelBlock block;

	err = FskMemPtrNewClear(sizeof(FskSndChannelBlockRecord) + (frameSizes ? sizeof(UInt32) * frameCount : 0), &block);
	BAIL_IF_ERR(err);

	block->data = (unsigned char *)data;
	block->dataSize = dataSize;
	block->frameCount = frameCount;
	block->samplesPerFrame = samplesPerFrame;
	block->samplesToSkip = samplesToSkip;
	block->refCon = dataRefCon;
	block->sndChan = sndChan;
	if (NULL != frameSizes) {
		block->frameSizes = &block->dummy;
		FskMemMove(&block->dummy, frameSizes, sizeof(UInt32) * frameCount);
	}

	FskMutexAcquire(sndChan->mutexBlocks);
		FskListAppend((FskList*)(void*)&sndChan->blocks, block);
	FskMutexRelease(sndChan->mutexBlocks);

bail:
	return err;
}

FskErr FskSndChannelGetSamplePosition(FskSndChannel sndChan, FskSampleTime *atSample)
{
	FskErr err;
	FskSampleTime position;
	double temp;

	err = FskAudioOutGetSamplePosition(sndChan->audioOut, &position);
	BAIL_IF_ERR(err);

	temp = (double)position;
	temp /= sndChan->outSampleRate;
	temp *= sndChan->inSampleRate;
	temp *= sndChan->playRate;
	*atSample = sndChan->startSample + (FskSampleTime)(temp + 0.5);

bail:
	return err;
}

FskErr FskSndChannelHasProperty(FskSndChannel sndChan, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	return FskMediaHasProperty(gSndChannelProperties, propertyID, get, set, dataType);
}

FskErr FskSndChannelSetProperty(FskSndChannel sndChan, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaSetProperty(gSndChannelProperties, sndChan, NULL, propertyID, property);
}

FskErr FskSndChannelGetProperty(FskSndChannel sndChan, UInt32 propertyID, FskMediaPropertyValue property)
{
	return FskMediaGetProperty(gSndChannelProperties, sndChan, NULL, propertyID, property);
}

FskErr FskSndChannelSetDoneCallback(FskSndChannel sndChan, FskSndChannelDoneCallback cb, void *refCon)
{
	sndChan->doneCB = cb;
	sndChan->doneRefCon = refCon;
	return kFskErrNone;
}

FskErr FskSndChannelSetMoreCallback(FskSndChannel sndChan, FskSndChannelMoreCallback cb, void *refCon)
{
	sndChan->moreCB = cb;
	sndChan->moreRefCon = refCon;
	return kFskErrNone;
}

FskErr FskSndChannelSetAbortCallback(FskSndChannel sndChan, FskSndChannelAbortCallback cb, void *refCon)
{
	sndChan->abortCB = cb;
	sndChan->abortRefCon = refCon;
	return kFskErrNone;
}

FskErr FskSndChannelStart(FskSndChannel sndChan, FskSampleTime atSample)
{
	FskErr err;

	sndChan->err = kFskErrNone;

	if (!sndChan->audioOut) {
		err = FskAudioOutNew(&sndChan->audioOut, sndChan->outputID, sndChan->outFormat);
		BAIL_IF_ERR(err);

		if (sndChan->threadSafeClient) {
			FskAudioOutSetDoneCallback(sndChan->audioOut, sndChannelThreadSafeDone, sndChan);
			FskAudioOutSetMoreCallback(sndChan->audioOut, sndChannelThreadSafeMore, sndChan);
		}
		else {
			FskAudioOutSetDoneCallback(sndChan->audioOut, sndChannelDone, sndChan);
			FskAudioOutSetMoreCallback(sndChan->audioOut, sndChannelMore, sndChan);
		}
		if (kFskErrNone == FskAudioOutSetFormat(sndChan->audioOut, sndChan->inFormat, sndChan->inNumChannels, sndChan->inSampleRate, sndChan->inFormatInfo, sndChan->inFormatInfoSize)) {
			double theRate;
			err = FskAudioOutGetFormat(sndChan->audioOut, &sndChan->outFormat, &sndChan->outNumChannels, &theRate);
            BAIL_IF_ERR(err);
			sndChan->outSampleRate = (UInt32)theRate;
		}

		updateVolume(sndChan);
	}

	if (sndChan->rateConverter) {
		err = FskAudioFilterStart(sndChan->rateConverter);
		BAIL_IF_ERR(err);
	}

	if (sndChan->timeScaler) {
		err = FskAudioFilterStart(sndChan->timeScaler);
		BAIL_IF_ERR(err);
	}

	if (sndChan->toneController) {
		err = FskAudioFilterStart(sndChan->toneController);
		BAIL_IF_ERR(err);
	}

	if (!sndChan->threadSafeClient)
		sndChannelRefill(sndChan->refillCallback, NULL, sndChan);

	sndChan->startSample = atSample;

	err = FskAudioOutStart(sndChan->audioOut, 0);
	BAIL_IF_ERR(err);

	sndChan->playing = true;

bail:
	return err ? err : sndChan->err;
}

FskErr FskSndChannelStop(FskSndChannel sndChan)
{
	if (!sndChan || !sndChan->mutex)
		return kFskErrNone;

	sndChan->playing = false;

	FskMutexAcquire(sndChan->mutex);

	if (sndChan->audioOut)
		FskAudioOutStop(sndChan->audioOut);

	flushPlayBlocks(sndChan);			// must precede disposal of queue blocks

	while (sndChan->blocks)
		sndChannelDisposeQueueBlock(sndChan, sndChan->blocks);

	if (sndChan->deco)
		FskAudioDecompressDiscontinuity(sndChan->deco);

	if (sndChan->rateConverter)
		FskAudioFilterStop(sndChan->rateConverter);

	if (sndChan->timeScaler)
		FskAudioFilterStop(sndChan->timeScaler);

	if (sndChan->toneController)
		FskAudioFilterStop(sndChan->toneController);

	FskTimeCallbackRemove(sndChan->refillCallback);

	FskMutexRelease(sndChan->mutex);

	return kFskErrNone;
}

void updateVolume(FskSndChannel sndChan)
{
	if (sndChan->volume < 0)
		sndChan->effectiveVolume = 0;
	else
		sndChan->effectiveVolume = (UInt16)(sndChan->volume * 256.0);	// 24.8 fixed point format

	sndChan->volL = sndChan->effectiveVolume;
	sndChan->volR = sndChan->effectiveVolume;

	if (sndChan->pan < 0)
		sndChan->volR = (UInt16)((sndChan->volR * (256 + sndChan->pan)) / 256);
	else if (sndChan->pan > 0)
		sndChan->volL = (UInt16)((sndChan->volL * (256 - sndChan->pan)) / 256);

	// if output can handle the volume scaling, we don't have to do it in the channel
	if (sndChan->audioOut && (kFskErrNone == FskAudioSetVolume(sndChan->audioOut, sndChan->volL, sndChan->volR))) {
		sndChan->effectiveVolume = 256;

		sndChan->volL = 256;
		sndChan->volR = 256;
	}
	else
	if ((sndChan->volL != sndChan->volR) && (kFskErrNone == FskAudioSetVolume(sndChan->audioOut, sndChan->effectiveVolume, sndChan->effectiveVolume))) {
		// doens't support balance, but does support volume.
		sndChan->effectiveVolume = 256;

		sndChan->volL = sndChan->effectiveVolume;
		sndChan->volR = sndChan->effectiveVolume;

		if (sndChan->pan < 0)
			sndChan->volR = (UInt16)((sndChan->volR * (256 + sndChan->pan)) / 256);
		else if (sndChan->pan > 0)
			sndChan->volL = (UInt16)((sndChan->volL * (256 - sndChan->pan)) / 256);
	}
}

void sndChannelDone(struct FskAudioOutRecord *audioOut, void *refCon, void *dataRefCon, Boolean played)
{
	FskSndChannel sndChan = (FskSndChannel)refCon;
	FskSndChannelPlayBlock playBlock = (FskSndChannelPlayBlock)dataRefCon;
	FskListMutexAppend(sndChan->playBlocks, (FskListElement)playBlock);
}

void sndChannelThreadSafeDone(struct FskAudioOutRecord *audioOut, void *refCon, void *dataRefCon, Boolean played)
{
	FskSndChannel sndChan = (FskSndChannel)refCon;
	FskSndChannelPlayBlock playBlock = (FskSndChannelPlayBlock)dataRefCon;

	if (playBlock) {
		sndChannelFreePlayBlockContents(sndChan, playBlock);
		FskMemPtrDispose(playBlock);
	}
	else {
/* hack */
		if (sndChan->abortCB) {
			FskSndChannelAbortCallback abortCB = sndChan->abortCB;
			void *abortRefCon = sndChan->abortRefCon;
			(abortCB)(sndChan, abortRefCon, kFskErrAudioOutReset);
		}
	}

}

// only frees the contents, not the block record itself
void sndChannelFreePlayBlockContents(FskSndChannel sndChan, FskSndChannelPlayBlock playBlock)
{
	if (NULL == playBlock->queueBlock)
		FskMemPtrDispose(playBlock->data);
	else
		sndChannelDisposeQueueBlock(sndChan, playBlock->queueBlock);

	playBlock->data = NULL;
	playBlock->queueBlock = NULL;
}

void sndChannelDisposeQueueBlock(FskSndChannel sndChan, FskSndChannelBlock block)
{
	FskMutexAcquire(sndChan->mutexBlocks);
		if (block->samplesToSkip && block->next)
			block->next->samplesToSkip += block->samplesToSkip;
		FskListRemove((FskList*)(void*)&sndChan->blocks, block);
	FskMutexRelease(sndChan->mutexBlocks);

	if (sndChan->doneCB)
		(sndChan->doneCB)(sndChan, sndChan->doneRefCon, block->refCon, false);
	FskMemPtrDispose(block);
}

FskErr sndChannelMore(struct FskAudioOutRecord *audioOut, void *refCon, UInt32 requestedSamples)
{
	FskSndChannel sndChan = (FskSndChannel)refCon;

	// schedule callback to fire immediately. looks like we're behind. (can't use refillCallback here because callbacks aren't thread safe - they must be called from the owner thread)
	FskThreadPostCallback(sndChan->thread, doSndChannelRefill, sndChan, NULL, NULL, NULL);

	return kFskErrOperationFailed;
}

void doSndChannelRefill(void *arg1, void *arg2, void *arg3, void *arg4)
{
	FskSndChannel sndChan = NULL, walker;

	if (!gSndChannels)
		return;

	FskMutexAcquire(gSndChannels->mutex);
		for (walker = (FskSndChannel)gSndChannels->list; (NULL != walker) && (NULL == sndChan); walker = walker->next) {
			if (walker == (FskSndChannel)arg1)
				sndChan = (FskSndChannel)arg1;
		}
	FskMutexRelease(gSndChannels->mutex);

	if (sndChan && sndChan->audioOut)
		sndChannelRefill(sndChan->refillCallback, NULL, sndChan);
}

FskErr sndChannelThreadSafeMore(struct FskAudioOutRecord *audioOut, void *refCon, UInt32 requestedSamples)
{
	FskSndChannel sndChan = (FskSndChannel)refCon;
	FskErr err = kFskErrNone;

	if (NULL == sndChan->moreCB)
		goto bail;

	while (requestedSamples > 0) {
		FskSndChannelBlock thisBlock;
		UInt32 samplesProcessed;

		FskMutexAcquire(sndChan->mutexBlocks);
			for (thisBlock = sndChan->blocks; thisBlock && thisBlock->processed; thisBlock = thisBlock->next)
				;
		FskMutexRelease(sndChan->mutexBlocks);

		if (NULL == thisBlock) {
			if (sndChan->thread == FskThreadGetCurrent()) {
				doSndChannelMoreEmpty(sndChan, (void *)requestedSamples, NULL, NULL);

				FskMutexAcquire(sndChan->mutexBlocks);
					for (thisBlock = sndChan->blocks; thisBlock && thisBlock->processed; thisBlock = thisBlock->next)
						;
				FskMutexRelease(sndChan->mutexBlocks);

				if (NULL == thisBlock)		// they didn't give us anything new in more. get out.
					goto bail;
			}
			else {
				FskThreadPostCallback(sndChan->thread, doSndChannelMoreEmpty, sndChan, NULL, NULL, NULL);
				goto bail;
			}
		}

		FskMutexAcquire(sndChan->mutex);
		err = processBlock(sndChan, thisBlock, requestedSamples, &samplesProcessed, NULL, NULL);
		FskMutexRelease(sndChan->mutex);

		BAIL_IF_ERR(err);

        if (samplesProcessed >= requestedSamples)
            requestedSamples = 0;
        else
            requestedSamples -= samplesProcessed;
	}

bail:
	if (err == kFskErrAudioOutReset)
		return err;

	return (requestedSamples > 0) ? kFskErrOperationFailed : kFskErrNone;
}

void doSndChannelMoreEmpty(void *a1, void *a2, void *a3, void *a4)
{
	FskSndChannel sndChan = a1;
	UInt32 inputSamplesNeeded = (UInt32)a2;

	if (!FskListMutexContains(gSndChannels, sndChan))
		return;

	FskMutexAcquire(sndChan->mutex);

	//@@ potential round off error
	if (sndChan->inSampleRate != sndChan->outSampleRate) {
		inputSamplesNeeded = (inputSamplesNeeded * sndChan->inSampleRate) / sndChan->outSampleRate;
		if (0 == inputSamplesNeeded)
			inputSamplesNeeded = 1;			// never request zero samples
	}

	(sndChan->moreCB)(sndChan, sndChan->moreRefCon, inputSamplesNeeded);

	FskMutexRelease(sndChan->mutex);
}

FskErr processBlock(FskSndChannel sndChan, FskSndChannelBlock block, UInt32 requestedSamples, UInt32 *samplesProcessed, unsigned char **out, UInt32 *outSize)
{
	FskErr err = kFskErrNone;
	SInt16 *i, *o;
	UInt32 count;
	UInt16 volL = sndChan->volL;
	UInt16 volR = sndChan->volR;
	UInt16 vol = sndChan->effectiveVolume;
	FskSndChannelPlayBlock playBlock = NULL;
	SInt16 *output;
	UInt32 outputSize;

	*samplesProcessed = 0;

	// need some state to keep track of the samples
	err = FskMemPtrNewClear(sizeof(FskSndChannelPlayBlockRecord), &playBlock);
	BAIL_IF_ERR(err);

	playBlock->data = block->data;
	playBlock->dataSize = block->dataSize;
	playBlock->frameCount = block->frameCount;
	playBlock->queueBlock = block;
	playBlock->numChannels = sndChan->inNumChannels;
	playBlock->format = sndChan->inFormat;
	playBlock->sampleRate = sndChan->inSampleRate;

	block->processed = true;

	if (NULL == playBlock->data) {
		UInt32 insertSilence = (playBlock->frameCount * sndChan->outSampleRate) / sndChan->inSampleRate;

		playBlock->dataSize = sizeof(UInt16) * sndChan->outNumChannels * insertSilence;
		err = FskMemPtrNewClear(playBlock->dataSize, (FskMemPtr*)(void*)&playBlock->data);
		BAIL_IF_ERR(err);

		playBlock->frameCount = insertSilence;
		playBlock->numChannels = sndChan->outNumChannels;
		playBlock->format = sndChan->outFormat;
		playBlock->sampleRate = sndChan->outSampleRate;
		sndChannelDisposeQueueBlock(sndChan, playBlock->queueBlock);
		playBlock->queueBlock = NULL;

		err = FskAudioOutEnqueue(sndChan->audioOut, playBlock->data,
					playBlock->dataSize, playBlock,
					playBlock->frameCount, NULL);
		BAIL_IF_ERR(err);

		playBlock = NULL;
		*samplesProcessed += insertSilence;

		goto bail;
	}

	//@@ need to implement samplesToSkip for uncompressed format
	if (playBlock->format != sndChan->outFormat) {
		// format conversion
		switch (playBlock->format) {
			case kFskAudioFormatPCM8BitOffsetBinary:
			case kFskAudioFormatPCM8BitTwosComplement: {
				UInt8 *i8;
				UInt8 mask = (kFskAudioFormatPCM8BitOffsetBinary == playBlock->format) ? 0x80 : 0;

				outputSize = playBlock->frameCount * playBlock->numChannels * 2;

				err = FskMemPtrNew(outputSize, &output);
				BAIL_IF_ERR(err);

				count = playBlock->frameCount * playBlock->numChannels;
				i8 = (UInt8 *)playBlock->data;
				o = output;
				while (count--) {
					UInt16 s8 = *i8++ ^ mask;
					*o++ = (s8 << 8) | s8;
				}
				}
				break;

			case kFskAudioFormatPCM16BitBigEndian:
			case kFskAudioFormatPCM16BitLittleEndian: {
				outputSize = playBlock->frameCount * playBlock->numChannels * 2;

				err = FskMemPtrNew(outputSize, &output);
				BAIL_IF_ERR(err);

				count = playBlock->frameCount * playBlock->numChannels;
				i = (SInt16 *)playBlock->data;
				o = output;
				while (count--) {
					*o++ = FskEndian16_Swap(*i);
					i++;
				}
				}
				break;

			default: {
				UInt32 outputFrameCount;
				Boolean firstFrame = false;

				if (NULL == sndChan->deco) {
					FskMediaPropertyValueRecord prop;

					err = FskAudioDecompressNew(&sndChan->deco, playBlock->format, sndChan->inMIME, playBlock->sampleRate, playBlock->numChannels, sndChan->inFormatInfo, sndChan->inFormatInfoSize);
					if (err) {
						if (kFskErrExtensionNotFound == err)
							err = kFskErrCodecNotFound;
						goto bail;
					}

					FskInstrumentedItemSetOwner(sndChan->deco, sndChan);

					prop.type = kFskMediaPropertyTypeBoolean;
					prop.value.b = true;
					FskAudioDecompressSetProperty(sndChan->deco, kFskMediaPropertyCanChangeSampleRate, &prop);
					FskAudioDecompressSetProperty(sndChan->deco, kFskMediaPropertyCanChangeChannelCount, &prop);

					if (NULL != sndChan->trInfo.value.data.data) {
						err = FskAudioDecompressSetProperty(sndChan->deco, kFskMediaPropertyTRInfo, &sndChan->trInfo);
						BAIL_IF_ERR(err);
					}

					sndChan->decompressedSampleRate = playBlock->sampleRate;

					firstFrame = true;
				}

				if (((playBlock->frameCount * block->samplesPerFrame) > requestedSamples) && (playBlock->frameCount > 1)) {
					// don't want to decompress more samples than requested
					UInt32 newFrameCount = (requestedSamples + block->samplesPerFrame - 1) / block->samplesPerFrame;
					UInt32 i, frameSize = playBlock->dataSize / playBlock->frameCount;
					for (i=playBlock->frameCount - 1; i >= newFrameCount; i--)
						playBlock->dataSize -= block->frameSizes ? block->frameSizes[i] : frameSize;
					playBlock->frameCount = newFrameCount;
					block->processed = playBlock->frameCount == block->frameCount;		// maybe more to do next time
				}

				err = FskAudioDecompressFrames(sndChan->deco, ((void*)(-1) == playBlock->data) ? NULL : playBlock->data, playBlock->dataSize,
						playBlock->frameCount, playBlock->queueBlock->frameSizes,
						(void **)(void *)&output, NULL, &outputFrameCount, &playBlock->numChannels);
				BAIL_IF_ERR(err);

				if (firstFrame) {
					FskMediaPropertyValueRecord decompressedSampleRate, decompressedNumChannels;

					// fetch this after the first frame is decoded because in some cases AAC-HE sample rate cannot be determined without inspecting the bit-stream
					if (kFskErrNone == FskAudioDecompressGetProperty(sndChan->deco, kFskMediaPropertySampleRate, &decompressedSampleRate))
						sndChan->decompressedSampleRate = decompressedSampleRate.value.integer;

					if (kFskErrNone == FskAudioDecompressGetProperty(sndChan->deco, kFskMediaPropertyChannelCount, &decompressedNumChannels)) {
						sndChan->inNumChannels = (UInt16)decompressedNumChannels.value.integer;
						playBlock->numChannels = sndChan->inNumChannels;
					}
				}

				if (false == block->processed) {
					// adjust block to account for frames decompressed
					block->frameCount -= playBlock->frameCount;
					block->dataSize -= playBlock->dataSize;
					block->data += playBlock->dataSize;
					if (block->frameSizes)
						block->frameSizes += playBlock->frameCount;
				}

				if (block->samplesToSkip > 0) {
					UInt32 offset, samplesToSkip;

					samplesToSkip = (block->samplesToSkip * sndChan->decompressedSampleRate) / playBlock->sampleRate;
					if (samplesToSkip > outputFrameCount)
						samplesToSkip = outputFrameCount;

					offset = (samplesToSkip * 2 * playBlock->numChannels);
					FskMemMove(output, offset + (unsigned char *)output, (outputFrameCount * 2 * playBlock->numChannels) - offset);
					outputFrameCount -= samplesToSkip;
					block->samplesToSkip -= (samplesToSkip * playBlock->sampleRate) / sndChan->decompressedSampleRate;
				}

				playBlock->frameCount = outputFrameCount;
				playBlock->sampleRate = sndChan->decompressedSampleRate;

				outputSize = playBlock->frameCount * 2 * playBlock->numChannels;
				}
				break;
		}

		// free the source data; swap in the new data
		if (block->processed)
			sndChannelFreePlayBlockContents(sndChan, playBlock);
		else
			playBlock->queueBlock = NULL;
		playBlock->data = output;
		playBlock->dataSize = outputSize;

		playBlock->format = sndChan->outFormat;

		if (0 == playBlock->frameCount) {
			// can happen if decompresor has a pipeline
			FskMemPtrDisposeAt(&playBlock);
			goto bail;
		}
	}

	if (1.0 != sndChan->playRate) {
		UInt32 convertedSampleCount, convertedSamplesSize;

		if (NULL == sndChan->timeScaler) {
			FskMediaPropertyValueRecord value;

			err = FskAudioFilterNew(&sndChan->timeScaler, "time scale");
			BAIL_IF_ERR(err);

			FskInstrumentedItemSetOwner(sndChan->timeScaler, sndChan);

			err = FskAudioFilterSetInputFormat(sndChan->timeScaler, playBlock->format, playBlock->sampleRate, playBlock->numChannels);
			BAIL_IF_ERR(err);

			value.type = kFskMediaPropertyTypeBoolean;
			value.value.b = true;
			sndChan->timeScalerDoesRateConversion = kFskErrNone == FskAudioFilterSetProperty(sndChan->timeScaler, kFskMediaPropertyCanChangeSampleRate, &value);

			value.type = kFskMediaPropertyTypeInteger;
			value.value.integer = sndChan->timeScalerDoesRateConversion ? sndChan->outSampleRate : playBlock->sampleRate;
			err = FskAudioFilterSetProperty(sndChan->timeScaler, kFskMediaPropertySampleRate, &value);
			BAIL_IF_ERR(err);

			value.type = kFskMediaPropertyTypeFloat;
			value.value.number = sndChan->playRate;
			err = FskAudioFilterSetProperty(sndChan->timeScaler, kFskMediaPropertyPlayRate, &value);
			BAIL_IF_ERR(err);

			err = FskAudioFilterStart(sndChan->timeScaler);
			BAIL_IF_ERR(err);
		}

		err = FskAudioFilterGetMaximumBufferSize(sndChan->timeScaler, playBlock->frameCount, &outputSize);
		BAIL_IF_ERR(err);

		err = FskMemPtrNew(outputSize, &output);
		BAIL_IF_ERR(err);

		err = FskAudioFilterProcessSamples(sndChan->timeScaler, playBlock->data, playBlock->frameCount,
												output, &convertedSampleCount, &convertedSamplesSize);
		BAIL_IF_ERR(err);

		// free the source data; swap in the new data
		sndChannelFreePlayBlockContents(sndChan, playBlock);
		playBlock->data = output;
		playBlock->dataSize = convertedSamplesSize;

		if (sndChan->timeScalerDoesRateConversion)
			playBlock->sampleRate = sndChan->outSampleRate;
		playBlock->frameCount = convertedSampleCount;
	}

	if (playBlock->sampleRate != sndChan->outSampleRate) {
		UInt32 convertedSampleCount, convertedSamplesSize;

		if (NULL == sndChan->rateConverter) {
			// instantiate sample rate converter
			FskMediaPropertyValueRecord value;

			err = FskAudioFilterNew(&sndChan->rateConverter, "rate converter");
			BAIL_IF_ERR(err);

			FskInstrumentedItemSetOwner(sndChan->rateConverter, sndChan);

			err = FskAudioFilterSetInputFormat(sndChan->rateConverter, playBlock->format, playBlock->sampleRate, playBlock->numChannels);
			BAIL_IF_ERR(err);

			value.type = kFskMediaPropertyTypeInteger;
			value.value.integer = sndChan->outSampleRate;
			err = FskAudioFilterSetProperty(sndChan->rateConverter, kFskMediaPropertySampleRate, &value);
			BAIL_IF_ERR(err);

			err = FskAudioFilterStart(sndChan->rateConverter);
			BAIL_IF_ERR(err);
		}

		err = FskAudioFilterGetMaximumBufferSize(sndChan->rateConverter, playBlock->frameCount, &outputSize);
		BAIL_IF_ERR(err);

		err = FskMemPtrNew(outputSize, &output);
		BAIL_IF_ERR(err);

		err = FskAudioFilterProcessSamples(sndChan->rateConverter, playBlock->data, playBlock->frameCount,
												output, &convertedSampleCount, &convertedSamplesSize);
		BAIL_IF_ERR(err);

		// free the source data; swap in the new data
		sndChannelFreePlayBlockContents(sndChan, playBlock);
		playBlock->data = output;
		playBlock->dataSize = convertedSamplesSize;

		playBlock->sampleRate = sndChan->outSampleRate;
		playBlock->frameCount = convertedSampleCount;
	}

	if (NULL != sndChan->toneSettings) {
		UInt32 convertedSampleCount, convertedSamplesSize;

		if (NULL == sndChan->toneController) {
			FskMediaPropertyValueRecord value;

			err = FskAudioFilterNew(&sndChan->toneController, "tone controller");
			BAIL_IF_ERR(err);

			FskInstrumentedItemSetOwner(sndChan->toneController, sndChan);

			err = FskAudioFilterSetInputFormat(sndChan->toneController, playBlock->format, playBlock->sampleRate, playBlock->numChannels);
			BAIL_IF_ERR(err);

			value.type = kFskMediaPropertyTypeString;
			value.value.str = sndChan->toneSettings;
			err = FskAudioFilterSetProperty(sndChan->toneController, kFskMediaPropertyCompressionSettings, &value);
			BAIL_IF_ERR(err);

			err = FskAudioFilterStart(sndChan->toneController);
			BAIL_IF_ERR(err);
		}

		err = FskAudioFilterGetMaximumBufferSize(sndChan->toneController, playBlock->frameCount, &outputSize);
		BAIL_IF_ERR(err);

		err = FskMemPtrNew(outputSize, &output);
		BAIL_IF_ERR(err);

		err = FskAudioFilterProcessSamples(sndChan->toneController, playBlock->data, playBlock->frameCount,
												output, &convertedSampleCount, &convertedSamplesSize);
		if (err) {
			FskMemPtrDispose(output);
			goto bail;
		}

		// free the source data; swap in the new data
		sndChannelFreePlayBlockContents(sndChan, playBlock);
		playBlock->data = output;
		playBlock->dataSize = convertedSamplesSize;
	}

	if ((playBlock->numChannels != sndChan->outNumChannels) ||
		(sndChan->volL != sndChan->volR) || (256 != sndChan->effectiveVolume)) {
		// apply volume, panning and fix up channel count
		// Note: if performance is a big issue, we could avoid all the pinning
		//	code with a separate set of loops that is only run when volume is
		//	less than 1.0.
		SInt16 *output;
		UInt32 outputSize = playBlock->frameCount * sndChan->outNumChannels * 2;

		err = FskMemPtrNew(outputSize, &output);
		BAIL_IF_ERR(err);

		count = playBlock->frameCount;
		o = output;
		i = (SInt16 *)playBlock->data;
		if (playBlock->numChannels == sndChan->outNumChannels) {
			if (1 == playBlock->numChannels) {
				// mono to mono with volume scaling
				while (count--) {
					SInt32 value = (*i++ * vol) >> 8;
					if (value < -32768) value = -32768;
					else if (value > 32767) value = 32767;
					*o++ = (SInt16)value;
				}
			}
			else {
				// stereo to stereo with volume scaling & pan
				while (count--) {
					SInt32 value = (*i++ * volL) >> 8;
					if (value < -32768) value = -32768;
					else if (value > 32767) value = 32767;
					*o++ = (SInt16)value;
					value = (*i++ * volR) >> 8;
					if (value < -32768) value = -32768;
					else if (value > 32767) value = 32767;
					*o++ = (SInt16)value;
				}
			}
		}
		else
		if (1 == playBlock->numChannels) {
			if (volL == volR) {
				// mono to stereo with volume scale (no panning requested)
				if (256 == vol) {
					while (count--) {
						SInt16 value16 = *i++;
						*o++ = value16;
						*o++ = value16;
					}
				}
				else
				if (vol < 256) {
					while (count--) {
						SInt32 value = (*i++ * vol) >> 8;
						*o++ = (SInt16)value;
						*o++ = (SInt16)value;
					}
				}
				else {
					while (count--) {
						SInt32 value = (*i++ * vol) >> 8;
						if (value < -32768) value = -32768;
						else if (value > 32767) value = 32767;
						*o++ = (SInt16)value;
						*o++ = (SInt16)value;
					}
				}
			}
			else {
				// mono to stereo with volume scale and panning
				while (count--) {
					SInt32 value = (*i * volL) >> 8;
					if (value < -32768) value = -32768;
					else if (value > 32767) value = 32767;
					*o++ = (SInt16)value;
					value = (*i++ * volR) >> 8;
					if (value < -32768) value = -32768;
					else if (value > 32767) value = 32767;
					*o++ = (SInt16)value;
				}
			}
		}
		else if (2 == playBlock->numChannels) {
			// stereo to mono with volume scaling
			while (count--) {
				SInt32 value = *i++;
				value += *i++;
				value = (value * vol) >> 9;
				if (value < -32768) value = -32768;
				else if (value > 32767) value = 32767;
				*o++ = (SInt16)value;
			}
		}

		// free the source data; swap in the new data
		sndChannelFreePlayBlockContents(sndChan, playBlock);
		playBlock->data = output;
		playBlock->dataSize = outputSize;

		playBlock->numChannels = sndChan->outNumChannels;
	}

	if ((playBlock->format == kFskAudioFormatAAC)
		|| (playBlock->format == kFskAudioFormatATRAC3)
		|| (playBlock->format == kFskAudioFormatMP3))
		*samplesProcessed += playBlock->frameCount * 4096;
	else
		*samplesProcessed += playBlock->frameCount;

	// and queue the bytes up with the audio output
	if (out) {
		err = FskMemPtrRealloc(playBlock->dataSize + *outSize, out);
		BAIL_IF_ERR(err);

		FskMemMove(*out + *outSize, playBlock->data, playBlock->dataSize);
		*outSize += playBlock->dataSize;

		// playBlock is freed in bail below
	}
	else
	if (0 != playBlock->frameCount) {
		err = FskAudioOutEnqueue(sndChan->audioOut, playBlock->data,
					playBlock->dataSize, playBlock,
					playBlock->queueBlock ? playBlock->frameCount : 0,
					playBlock->queueBlock ? playBlock->queueBlock->frameSizes : NULL);
		BAIL_IF_ERR(err);

		playBlock = NULL;
	}

bail:
	if (playBlock) {
		sndChannelFreePlayBlockContents(sndChan, playBlock);
		FskMemPtrDispose(playBlock);
	}

	if (err && !sndChan->err)
		sndChan->err = err;

	return err;
}

void sndChannelRefill(FskTimeCallBack callback, const FskTime time, void *param)
{
	FskSndChannel sndChan = (FskSndChannel)param;
	UInt32 chunkRequestSize = 0;
	double sampleRate = 0;
	UInt32 msec = 334;
	FskErr err = kFskErrNone;

	while (true) {
		UInt32 samplesQueued, targetQueueLength;

		if (kFskErrNone != FskAudioOutGetSamplesQueued(sndChan->audioOut, &samplesQueued, &targetQueueLength))
			break;

		if (samplesQueued >= targetQueueLength)
			break;

		err = sndChannelThreadSafeMore(sndChan->audioOut, sndChan, targetQueueLength - samplesQueued);
		if (err)
			break;
	}

	flushPlayBlocks(sndChan);

	if (err == kFskErrAudioOutReset)
		return;

	// If chunkRequestSize is small, reduce the period.
	// Twice of chunk size seems to be nice.
	FskAudioOutGetOutputBufferSize(sndChan->audioOut, &chunkRequestSize, NULL, NULL);
	FskAudioOutGetFormat(sndChan->audioOut, NULL, NULL, &sampleRate);
	if ((chunkRequestSize > 0) && (sampleRate > 0)) {
		UInt32 msec2 = (chunkRequestSize * 1000 / (UInt32)sampleRate) * 2;
		if (msec2 < msec) msec = msec2;
	}
	FskTimeCallbackScheduleFuture(callback, 0, msec, sndChannelRefill, param);
}

void flushPlayBlocks(FskSndChannel sndChan)
{
	while (NULL != sndChan->playBlocks) {
		FskSndChannelPlayBlock playBlock = (FskSndChannelPlayBlock)FskListMutexRemoveFirst(sndChan->playBlocks);
		if (!playBlock) break;

		sndChannelFreePlayBlockContents(sndChan, playBlock);
		FskMemPtrDispose(playBlock);
	}
}

FskErr sndChannelSetTRInfo(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskSndChannel sndChan = (FskSndChannel)state;

	FskMediaPropertyEmpty(&sndChan->trInfo);
	return FskMediaPropertyCopy(property, &sndChan->trInfo);
}

FskErr sndChannelSetPlayRate(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskSndChannel sndChan = (FskSndChannel)state;
	FskErr err = kFskErrNone;

	sndChan->playRate = property->value.number;

	if (sndChan->timeScaler) {
		FskMediaPropertyValueRecord value;
		value.type = kFskMediaPropertyTypeFloat;
		value.value.number = sndChan->playRate;
		err = FskAudioFilterSetProperty(sndChan->timeScaler, kFskMediaPropertyPlayRate, &value);
	}

	return err;
}

FskErr sndChannelSetEQ(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskSndChannel sndChan = (FskSndChannel)state;

	// If the audio output handles this we're done
	if (kFskErrNone == FskAudioOutSetProperty(sndChan->audioOut, kFskAudioOutPropertyEQ, property))
		return kFskErrNone;

	if (!sndChan->toneController && (kFskErrNone != FskAudioFilterNew(NULL, "tone controller")))
		return kFskErrUnimplemented;			// no tone settings if no filter installer

	FskMutexAcquire(sndChan->mutex);

	FskMemPtrDisposeAt((void**)(void*)&sndChan->toneSettings);
	if ((NULL != property->value.str) && (0 != *property->value.str))
		sndChan->toneSettings = FskStrDoCopy(property->value.str);

	if (NULL != sndChan->toneController) {
		FskMediaPropertyValueRecord value;

		if (sndChan->playing)
			FskAudioFilterStop(sndChan->toneController);

		value.type = kFskMediaPropertyTypeString;
		value.value.str = sndChan->toneSettings;
		FskAudioFilterSetProperty(sndChan->toneController, kFskMediaPropertyCompressionSettings, &value);

		if (sndChan->playing)
			FskAudioFilterStart(sndChan->toneController);
	}

	FskMutexRelease(sndChan->mutex);

	return kFskErrNone;
}

FskErr sndChannelGetBufferDuration(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskSndChannel sndChan = (FskSndChannel)state;

	property->type = kFskMediaPropertyTypeInteger;
	return FskAudioOutGetOutputBufferSize(sndChan->audioOut, NULL, (UInt32*)&property->value.integer, NULL);
}

FskErr sndChannelGetSampleRate(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskSndChannel sndChan = (FskSndChannel)state;
	double sampleRate;
    FskErr err;

	err = FskAudioOutGetFormat(sndChan->audioOut, NULL, NULL, &sampleRate);
    BAIL_IF_ERR(err);

	property->type = kFskMediaPropertyTypeInteger;
	property->value.number = sampleRate;

bail:
	return err;
}

FskErr sndChannelGetFlags(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskSndChannel sndChan = (FskSndChannel)state;
	return FskAudioOutGetProperty(sndChan->audioOut, propertyID, property);
}

FskErr sndChannelSetHibernate(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskSndChannel sndChan = (FskSndChannel)state;

	if (property->value.b) {
		FskSndChannelStop(sndChan);

		FskAudioOutDispose(sndChan->audioOut);
		sndChan->audioOut = NULL;
	}

	return kFskErrNone;
}

FskErr sndChannelSetAudioCategory(void *state, void *dummy, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskSndChannel sndChan = (FskSndChannel)state;

	return FskAudioOutSetProperty(sndChan->audioOut, kFskAudioOutPropertyCategory, property);
}

FskErr FskAudioMIMEToFormat(const char *mime, UInt32 *audioFormat)
{
	if (0 == FskStrCompare("x-audio-codec/pcm-16-be", mime))
		*audioFormat = kFskAudioFormatPCM16BitBigEndian;
	else if (0 == FskStrCompare("x-audio-codec/pcm-16-le", mime))
		*audioFormat = kFskAudioFormatPCM16BitLittleEndian;
	else if (0 == FskStrCompare("x-audio-codec/pcm-8-twos", mime))
		*audioFormat = kFskAudioFormatPCM8BitTwosComplement;
	else if (0 == FskStrCompare("x-audio-codec/pcm-8-offset", mime))
		*audioFormat = kFskAudioFormatPCM8BitOffsetBinary;
	else if (0 == FskStrCompare("x-audio-codec/mp3", mime))
		*audioFormat = kFskAudioFormatMP3;
	else if (0 == FskStrCompare("x-audio-codec/aac", mime))
		*audioFormat = kFskAudioFormatAAC;
	else if (0 == FskStrCompare("x-audio-codec/atrac3", mime))
		*audioFormat = kFskAudioFormatATRAC3;
	else if (0 == FskStrCompare("x-audio-codec/atrac3-plus", mime))
		*audioFormat = kFskAudioFormatATRAC3Plus;
	else if (0 == FskStrCompare("x-audio-codec/atrac3-lossless", mime))
		*audioFormat = kFskAudioFormatATRACAdvancedLossless;
	else if (0 == FskStrCompare("x-audio-codec/mpeg-1", mime))
		*audioFormat = kFskAudioFormatMP1A;
	else if (0 == FskStrCompare("x-audio-codec/ac3", mime))
		*audioFormat = kFskAudioFormatAC3;
	else if (0 == FskStrCompare("x-audio-codec/qcelp", mime))
		*audioFormat = kFskAudioFormatQCELP;
	else if (0 == FskStrCompare("x-audio-codec/evrc", mime))
		*audioFormat = kFskAudioFormatEVRC;
	else if (0 == FskStrCompare("x-audio-codec/amr-nb", mime))
		*audioFormat = kFskAudioFormatAMRNB;
	else if (0 == FskStrCompare("x-audio-codec/amr-wb", mime))
		*audioFormat = kFskAudioFormatAMRWB;
	else if (0 == FskStrCompare("x-audio-codec/wma", mime))
		*audioFormat = kFskAudioFormatWMA;
	else if (0 == FskStrCompare("x-audio-codec/speex-nb", mime))
		*audioFormat = kFskAudioFormatSPEEXNB;
	else
		*audioFormat = kFskAudioFormatUndefined;

	return kFskErrNone;
}
