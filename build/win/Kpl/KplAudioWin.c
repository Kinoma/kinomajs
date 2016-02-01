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
#undef SUPPORT_WIN32_WAVEOUT
#define SUPPORT_WIN32_DIRECTSOUND 1

#include "windows.h"
#include "mmsystem.h"
#include "Mmsystem.h"
#include "mmreg.h"
#include "Msacm.h"

#include "dsound.h"

#if SUPPORT_WIN32_DIRECTSOUND
    #include "math.h" 

	// we don't fully fill DirectSound buffers by this number of samples, to avoid an ambiguity between buffer full & empty
	#define kDirectSoundRefillSlop (1)
#elif SUPPORT_WIN32_WAVEOUT
	#include "FskMain.h"
#endif

#include "Kpl.h"
#include "KplAudio.h"
#include "KplSynchronization.h"
#include "KplThread.h"

#include "FskList.h"
#include "FskString.h"

struct KplAudioBlockRecord {
	struct KplAudioBlockRecord  *next;

	unsigned char		*data;
	UInt32				dataSize;
	UInt32				sampleCount;

	void				*refCon;
	Boolean				done;
	Boolean				silence;
	Boolean				unused0;
	Boolean				unused1;

	void				*audioOut;

#if SUPPORT_WIN32_WAVEOUT
	WAVEHDR				waveHdr;
	Boolean				prepared;
#elif SUPPORT_WIN32_DIRECTSOUND
	UInt32				samplesUsed;
#endif
	UInt32				frameCount;
	UInt32				*frameSizes;
	UInt32				frameSizesArray[1];		// must be last
};
typedef struct KplAudioBlockRecord KplAudioBlockRecord;
typedef KplAudioBlockRecord *KplAudioBlock;

struct KplAudioRecord {
	struct KplAudioRecord  *next;

	UInt32						sampleRate;
	const char					*mime;
	Boolean						pcm;
	UInt16						numChannels;

	UInt16						leftVolume;
	UInt16						rightVolume;

	Boolean						playing;

	KplAudioMoreCallback		moreCB;
	void						*moreRefCon;

	KplAudioDoneCallback		doneCB;
	void						*doneRefCon;

	KplThread					thread;
	KplAudioBlock			blocks;

	UInt32						chunkRequestSize;
	UInt32						bufferedSamplesTarget;

	KplMutex					mutex;

#if SUPPORT_WIN32_WAVEOUT
	HWAVEOUT					waveOut;
	Boolean						flushPending;
	Boolean						dontRefill;
#elif SUPPORT_WIN32_DIRECTSOUND
	LPDIRECTSOUND8				dS;
	LPDIRECTSOUNDBUFFER8		dSBuffer;
	HANDLE						hNotify;
	HANDLE						hEndThread;
	HANDLE						hNotifyThread;
	UInt32						bufferSize;
	FskInt64					bytesWritten;
	UInt32						bufferCount;
	UInt32						samplesNeeded;
#endif
};

static FskErr initializePlatformOutput(KplAudio audio);
static void terminatePlatformOutput(KplAudio audio);

static UInt32 getSamplesQueued(KplAudio audioOut);
static void refillQueue(KplAudio audioOut);
static void removeUnusedFromQueue(KplAudio audioOut);
static void removeAllFromQueue(KplAudio audioOut);
static void renderAudioBlocks(KplAudio audioOut);
static void flushAndRefill(void *arg0, void *arg1, void *arg2, void *arg3);
static FskErr audioOutEnqueue(KplAudio audioOut, void *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, UInt32 *frameSizes, Boolean silence, KplAudioBlock *blockOut);

static FskErr KplAudioIsValid(KplAudio audioOut, Boolean *isValid);

#if SUPPORT_WIN32_DIRECTSOUND
static DWORD WINAPI directSoundProc(LPVOID lpParameter);
static FskErr createDirectSoundBuffer(KplAudio audioOut, UInt32 bufferSize, UInt32 bufferCount);
static void releaseDirectSoundBuffer(KplAudio audioOut);
static FskErr updateDirectSoundVolume(KplAudio audioOut);
#endif

extern HWND gUtilsWnd;

static FskListMutex gAudioOuts = NULL;

FskErr KplAudioNew(KplAudio *audioOutOut)
{
	FskErr err = kFskErrNone;
	KplAudio audioOut = NULL;

	if (NULL == gAudioOuts) {
		err = FskListMutexNew(&gAudioOuts, "gAudioOuts");
		if (err) goto bail;
	}

	err = FskMemPtrNewClear(sizeof(KplAudioRecord), (FskMemPtr *)&audioOut);
	if (err) goto bail;

	audioOut->thread = KplThreadGetCurrent();
	audioOut->leftVolume = 256;
	audioOut->rightVolume = 256;

	err = KplMutexNew(&audioOut->mutex);
	if (err) goto bail;

	err = initializePlatformOutput(audioOut);
	if (err) goto bail;

	audioOut->pcm = (0 == FskStrCompare("x-audio-codec/pcm-16-be", audioOut->mime)) || (0 == FskStrCompare("x-audio-codec/pcm-16-le", audioOut->mime));

	FskListMutexPrepend(gAudioOuts, audioOut);

bail:
	if (err) {
		KplAudioDispose(audioOut);
		audioOut = NULL;
	}
	*audioOutOut = audioOut;

	return err;
}

FskErr KplAudioDispose(KplAudio audio)
{
	KplAudio audioOut = (KplAudio)audio;
	
	if (audioOut) {
		KplAudioStop(audioOut);

		terminatePlatformOutput(audioOut);	

		FskListMutexRemove(gAudioOuts, audioOut);

		KplMutexDispose(audioOut->mutex);

		FskMemPtrDispose(audioOut);
	}

	return kFskErrNone;
}

FskErr KplAudioSetFormat(KplAudio audio, const char *format, UInt32 channels, double sampleRate, const unsigned char *formatInfo, UInt32 formatInfoSize)
{
	return kFskErrUnimplemented;
}

FskErr KplAudioGetFormat(KplAudio kplAudio, const char **format, UInt32 *channels, double *sampleRate, const unsigned char **formatInfo, UInt32 *formatInfoSize)
{
	KplAudio audioOut = (KplAudio)kplAudio;
	
	if (format)
		*format = "x-audio-codec/pcm-16-le";
	if (channels)
		*channels = audioOut->numChannels;
	if (sampleRate)
		*sampleRate = audioOut->sampleRate;
		
	return kFskErrNone;
}

FskErr KplAudioWrite(KplAudio audio, const char *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, const UInt32 *frameSizes)
{
	KplAudio audioOut = (KplAudio)audio;
	return audioOutEnqueue(audioOut, (void*)data, dataSize, dataRefCon, frameCount, (UInt32*)frameSizes, false, NULL);
}

FskErr KplAudioStart(KplAudio audio)
{
	KplAudio audioOut = (KplAudio)audio;
	FskErr err = kFskErrNone;
#if !SUPPORT_WIN32_DIRECTSOUND
	KplAudioBlock walker;
#endif

#if SUPPORT_WIN32_DIRECTSOUND
	err = createDirectSoundBuffer(audioOut, (audioOut->sampleRate * 2 * audioOut->numChannels) / 4, 4);
	if (err) goto bail;
#endif

	refillQueue(audioOut);
	audioOut->playing = true;

#if SUPPORT_WIN32_DIRECTSOUND
	renderAudioBlocks(audioOut);
	IDirectSoundBuffer_Play(audioOut->dSBuffer, 0, 0, DSBPLAY_LOOPING);
#else
	#if SUPPORT_WIN32_WAVEOUT
		audioOut->dontRefill = true;		// to avoid calling refillQueue in the loop below
	#endif
	walker = (KplAudioBlock)audioOut->blocks;
	while (walker) {
		renderAudioBlock(audioOut, walker);
		walker = walker->next;
	}
	#if SUPPORT_WIN32_WAVEOUT
		audioOut->dontRefill = false;
	#endif
#endif

bail:
	return err;
}

FskErr KplAudioStop(KplAudio audio)
{
	KplAudio audioOut = (KplAudio)audio;
	FskErr err = kFskErrNone;

	if (!audioOut)
		return err;

	audioOut->playing = false;
	
#if SUPPORT_WIN32_WAVEOUT
	waveOutReset(audioOut->waveOut);
	if (audioOut->flushPending)
		removeUnusedFromQueue(audioOut);
#elif SUPPORT_WIN32_DIRECTSOUND
	releaseDirectSoundBuffer(audioOut);  
#endif

	removeAllFromQueue(audioOut);

	return err;
}

FskErr KplAudioGetProperty(KplAudio audio, UInt32 propertyID, KplProperty value)
{
	KplAudio audioOut = (KplAudio)audio;
	FskErr err = kFskErrNone;
	
	switch(propertyID) {
		case kKplPropertyAudioPreferredSampleRate:
			value->number = 44100;
			break;
		case kKplPropertyAudioPreferredUncompressedFormat:
			value->string = FskStrDoCopy(audioOut->mime);
			break;
		case kKplPropertyAudioPreferredBufferSize:
			value->integer = audioOut->sampleRate;
			break;
		case kKplPropertyAudioSamplePosition: {
#if SUPPORT_WIN32_WAVEOUT
			MMTIME mmtime;

			mmtime.wType = TIME_BYTES;
			if (MMSYSERR_NOERROR != waveOutGetPosition(audioOut->waveOut, &mmtime, sizeof(mmtime)))
				return kFskErrOperationFailed;

			value->number = ((KplSampleTime)mmtime.u.cb / (2 * audioOut->numChannels));
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

			value->number = (double)((audioOut->bytesWritten - unplayed) / ( 2 * audioOut->numChannels));
#endif
			}
			break;
		case kKplPropertyAudioVolume:
			err = FskMemPtrNew(sizeof(UInt32) * 2, (FskMemPtr*)&value->integers.integer);
			if (err) goto bail;
			value->integers.integer[0] = audioOut->leftVolume;
			value->integers.integer[1] = audioOut->rightVolume;
			break;
		case kKplPropertyAudioSingleThreadedClient:
			value->b = true;
			break;
		default:
			err = kFskErrUnimplemented;
			goto bail;
	}

bail:
	return err;
}

FskErr KplAudioSetProperty(KplAudio audio, UInt32 propertyID, KplProperty value)
{
	KplAudio audioOut = (KplAudio)audio;
	
	switch(propertyID) {
		case kKplPropertyAudioVolume: {
			audioOut->leftVolume = (UInt16)value->integers.integer[0];
			audioOut->rightVolume = (UInt16)value->integers.integer[1];
			
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
		#endif
			}
			break;
		default:
			return kFskErrUnimplemented;
	}
	return kFskErrNone;
}

FskErr KplAudioGetSamplesQueued(KplAudio audio, UInt32 *samplesQueuedOut, UInt32 *targetQueueLengthOut)
{
	KplAudio audioOut = (KplAudio)audio;
	
	if (samplesQueuedOut)
		*samplesQueuedOut = getSamplesQueued(audioOut);

	if (targetQueueLengthOut)
		*targetQueueLengthOut = audioOut->sampleRate * 2;			//@@ not even true!

	return kFskErrNone;
}

FskErr KplAudioSetDoneCallback(KplAudio audio, KplAudioDoneCallback cb, void *refCon)
{
	KplAudio audioOut = (KplAudio)audio;
	
	audioOut->doneCB = cb;
	audioOut->doneRefCon = refCon;
	
	return kFskErrNone;
}

FskErr KplAudioSetMoreCallback(KplAudio audio, KplAudioMoreCallback cb, void *refCon)
{
	KplAudio audioOut = (KplAudio)audio;
	
	audioOut->moreCB = cb;
	audioOut->moreRefCon = refCon;
	
	return kFskErrNone;
}

FskErr KplAudioIsValid(KplAudio audioOut, Boolean *isValid)
{
	KplAudio walker = NULL;
	*isValid = false;
	while (true) {
		walker = (KplAudio)FskListMutexGetNext(gAudioOuts, walker);
		if (NULL == walker)
			break;
		if (walker == audioOut)
			*isValid = true;
	}
	return kFskErrNone;
}

FskErr audioOutEnqueue(KplAudio audioOut, void *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, UInt32 *frameSizes, Boolean silence, KplAudioBlock *blockOut)
{
	FskErr err;
	KplAudioBlock block;

	err = FskMemPtrNewClear(sizeof(KplAudioBlockRecord) + ((frameCount && frameSizes) ? sizeof(UInt32) * frameCount : 0), (FskMemPtr *)&block);
	if (err) goto bail;

	block->data = (unsigned char *)data;
	block->dataSize = dataSize;
	block->sampleCount = dataSize / (2 * audioOut->numChannels);
	block->refCon = dataRefCon;
	block->audioOut = audioOut;
	block->silence = silence;

	block->frameCount = frameCount;
	if (frameCount && frameSizes) {
		memmove(block->frameSizesArray, frameSizes, sizeof(UInt32) * frameCount);
		block->frameSizes = block->frameSizesArray;
	}

	KplMutexAcquire(audioOut->mutex);

	FskListAppend((FskList *)&audioOut->blocks, (FskList *)block);

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
#endif

	KplMutexRelease(audioOut->mutex);

bail:
	if (blockOut) *blockOut = block;

	return err;
}

UInt32 getSamplesQueued(KplAudio audioOut)
{
	UInt32 count = 0;
	KplAudioBlock walker = audioOut->blocks;

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

void refillQueue(KplAudio audioOut)
{
	if (audioOut->moreCB) {
		SInt32 samplesTarget = audioOut->sampleRate;	// try to keep one second on buffer
#if SUPPORT_WIN32_WAVEOUT
		SInt32 chunkRequestSize = audioOut->sampleRate / 15;		// prefer small decompresses to avoid long periods of time decompressing
		samplesTarget = (samplesTarget * 3) >> 1;					// same calculation appears in waveOutProc, update both
#elif SUPPORT_WIN32_DIRECTSOUND
		SInt32 chunkRequestSize = audioOut->sampleRate / 4;
#endif

		if (audioOut->chunkRequestSize)
			chunkRequestSize = audioOut->chunkRequestSize;
		if (audioOut->bufferedSamplesTarget)
			samplesTarget = audioOut->bufferedSamplesTarget;

		while (true) {
			KplAudioBlock block;
			FskMemPtr silence;
			SInt32 samplesNeeded = samplesTarget - getSamplesQueued(audioOut);

			if (samplesNeeded <= 0)
				break;

			if (samplesNeeded > chunkRequestSize)
				samplesNeeded = chunkRequestSize;

			if (kFskErrNone == (audioOut->moreCB)(audioOut, audioOut->moreRefCon, samplesNeeded))
				continue;

			// make silence
			if (!audioOut->pcm)
				break;

			if (kFskErrNone != FskMemPtrNewClear(samplesNeeded * 2 * audioOut->numChannels, &silence))
				break;

			if (kFskErrNone != audioOutEnqueue(audioOut, silence, samplesNeeded * 2 * audioOut->numChannels, NULL, 0, NULL, true, &block))
				break;
		}
	}
}

void removeAllFromQueue(KplAudio audioOut)
{
	while (true) {
		KplAudioBlock block;

		block = (KplAudioBlock)FskListRemoveFirst((FskList *)&audioOut->blocks);
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

void removeUnusedFromQueue(KplAudio audioOut)
{
	while (true) {
		KplAudioBlock block = audioOut->blocks;

		if ((NULL == block) || (false == block->done))
			break;

		FskListRemove((FskList *)&audioOut->blocks, block);

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

void flushAndRefill(void *arg0, void *arg1, void *arg2, void *arg3)
{
	KplAudio audioOut = (KplAudio)arg0;
	Boolean isValid = false;

	if ((kFskErrNone != KplAudioIsValid(audioOut, &isValid)) || !isValid)
		return;

	KplMutexAcquire(audioOut->mutex);

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
	KplMutexRelease(audioOut->mutex);
}

#if SUPPORT_WIN32_DIRECTSOUND

static DWORD WINAPI directSoundProc(LPVOID lpParameter);
static FskErr createDirectSoundBuffer(KplAudio audioOut, UInt32 bufferSize, UInt32 bufferCount);
static void releaseDirectSoundBuffer(KplAudio audioOut);
static FskErr updateDirectSoundVolume(KplAudio audioOut);

FskErr initializePlatformOutput(KplAudio audioOut)
{
	FskErr err = kFskErrNone;
	LPDIRECTSOUND8 lpDirectSound = NULL;
	HRESULT hr;

	audioOut->sampleRate = 44100;
	audioOut->numChannels = 2;
	audioOut->mime = "x-audio-codec/pcm-16-le";

	hr = DirectSoundCreate8(NULL, &lpDirectSound, NULL);
	if (FAILED(hr)){
		err = kFskErrOperationFailed;
		goto bail;
	}

	hr = IDirectSound8_SetCooperativeLevel(lpDirectSound, gUtilsWnd, DSSCL_PRIORITY);
	if (FAILED(hr)) {
		err = kFskErrOperationFailed;
		goto bail;
	}
	
bail:
	if (err && (NULL != lpDirectSound)) {
		IDirectSound8_Release(lpDirectSound);
		lpDirectSound = NULL;
	}

	audioOut->dS = lpDirectSound;

	return err;
}

void terminatePlatformOutput(KplAudio audioOut)
{
	if (NULL != audioOut->dS)
		releaseDirectSoundBuffer(audioOut);

	if (NULL != audioOut->dS)
		IDirectSound8_Release(audioOut->dS);
}

// push as much audio as we can into DirectSound
void renderAudioBlocks(KplAudio audioOut)
{
	KplAudioBlock block;

	KplMutexAcquire(audioOut->mutex);

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

	KplMutexRelease(audioOut->mutex);
}

FskErr createDirectSoundBuffer(KplAudio audioOut, UInt32 bufferSize, UInt32 bufferCount)
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
	if (err) goto bail;

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
	if (NULL == audioOut->hNotifyThread) {
		err = kFskErrOperationFailed;
		goto bail;
	}

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

	FskMemPtrDispose((FskMemPtr)aPosNotify);

	return err;
}

void releaseDirectSoundBuffer(KplAudio audioOut)
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

FskErr updateDirectSoundVolume(KplAudio audioOut)
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

DWORD WINAPI directSoundProc(LPVOID lpParameter)
{
	HANDLE handles[2];
	KplAudio audioOut = (KplAudio)lpParameter;

	if (NULL == audioOut)
		return 0;

	handles[0] = audioOut->hEndThread;
	handles[1] = audioOut->hNotify;
	while (true) {
		DWORD waitObject = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
		if (WAIT_OBJECT_0 == waitObject)
			break;

		KplMutexAcquire(audioOut->mutex);

		if (false == audioOut->playing) {
			KplMutexRelease(audioOut->mutex);
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
		
		KplMutexRelease(audioOut->mutex);

		FskKplThreadPostCallback(audioOut->thread, flushAndRefill, audioOut, (void*)audioOut->samplesNeeded, NULL, NULL);
	}

	return 1;
}

#endif
