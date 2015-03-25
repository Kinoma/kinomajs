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
#define __FSKAUDIO_PRIV__
#define __FSKTHREAD_PRIV__

/* OpenSLES notes -
We recommend that you do not rely on either behavior; after a transition to SL_PLAYSTATE_STOPPED, you should explicitly call BufferQueue::Clear. This will place the buffer queue into a known state.

*/

#include "SLES/OpenSLES.h"
#include "SLES/OpenSLES_Android.h"
#define USE_SLES	1

#define DONT_GENERATE_SILENCE 1

#include "androidAudio.h"
#include "FskAudio.h"
#include "FskMemory.h"
#include "FskThread.h"
#include "FskHardware.h"

extern "C" {
	void doDeleteNativeAudio(void *a, void *b, void *c, void *d);

	FskInstrumentedSimpleType(AudioNative, audionative);
}

FskListMutex audioOuts = NULL;

FskMutex	gActiveAudioMutex = NULL;
FskAudioOut gActiveAudioOut = NULL;
//int gAudioOutInUse = 0;

#define MAX_NUMBER_INTERFACES 3
#define MAX_NUMBER_OUTPUT_DEVICES 6

#define AUDIO_DATA_STORAGE_SIZE 4096

//#define AUDIO_DATA_STORAGE_SIZE 8192
#define AUDIO_DATA_BUFFER_SIZE AUDIO_DATA_STORAGE_SIZE/2

#define kFlushAndRefillTime	500

static long sBufferedSamplesTarget = 0;


void fillBuffers(FskAudioOut audioOut);


typedef struct {
	void		*parent;
	int			sampleSize;

	FskMutex		getSamplePositionMutex;
	FskSampleTime	startedAtSamplePosition;
	FskSampleTime	stoppedAtSamplePosition;

	UInt32		bytesEnqueued;
	FskTimeCallBack flushTimer;

	FskTimeRecord		lastTime;

	SLEngineItf 			EngineItf;

	SLObjectItf 			player;
	SLPlayItf 				playItf;

	SLAndroidSimpleBufferQueueItf		bufferQueueItf;
	SLDataLocator_AndroidSimpleBufferQueue bufferQueue;
//	SLBufferQueueState		state;

	SLDataSource			audioSource;
	SLDataFormat_PCM		pcm;
	SLDataSink				audioSink;
	SLDataLocator_OutputMix locator_outputmix;

	SLObjectItf				outputMixObject;
//	SLVolumeItf				volumeItf;

	int						nextBuf;
	int						audioFilled[2];
	char					*audioBuf[2];
} androidAudioExt;

SLObjectItf engineObject;

//static SLObjectItf outputMixObject = NULL;


void CheckErr(const char *name, SLresult res) {
	if (res != 0)
		FskAudioNativePrintfMinimal("CheckErr: [%d - %x]  %s", (int)res, (unsigned)res, name);
}




int linuxAudioOutPCM(FskAudioOut audioOut, char *p, int size);

#define kUsed 0
#define kAll	0xffffffff

void removeFromQueue(FskAudioOut audioOut, UInt32 what) {
	int freed = 0;
	androidAudioExt *ext;

	FskAudioNativePrintfVerbose("removeFromQueue %x", what);
	if (audioOut->blocks) {
		int err;
		err = FskMutexTrylock(audioOut->blocks->mutex);
		if (err != 0) {
			FskAudioNativePrintfMinimal("removeUnusedFrom Queue - trylock was busy");
			return;
		}
		while (true) {
			FskAudioOutBlock block = (FskAudioOutBlock)audioOut->blocks->list;
			if ((NULL == block) || ((what == kUsed) & (false == block->done)))
				break;
			block = (FskAudioOutBlock)FskListRemoveFirst((FskList*)&audioOut->blocks->list);
			if (block->silence)
				FskMemPtrDispose(block->data);
			else if (audioOut->doneCB) {
				if (block->dataSize && (what != kAll)) {
					ext = (androidAudioExt*)audioOut->ext;
					FskAudioNativePrintfMinimal("TRASHING audioblock with some data left: %u bytes - (total left %u)", (unsigned)block->dataSize, (unsigned)(ext->bytesEnqueued - block->dataSize));
					ext->bytesEnqueued -= block->dataSize;
				}
				(audioOut->doneCB)(audioOut, audioOut->doneRefCon, block->refCon, true);
			}
			FskMemPtrDispose(block->frameSizes);
			FskMemPtrDispose(block);
			freed++;
		}
		FskMutexRelease(audioOut->blocks->mutex);
	}
}

void refillQueue(FskAudioOut audioOut);
void flushAndRefill(void *arg0, void *arg1, void *arg2, void *arg3) {
	FskAudioOut audioOut = (FskAudioOut)arg0;
	androidAudioExt *ext;
	Boolean isValid = false;

//FskAudioNativePrintfDebug("- flushAndRefill");
	if ((kFskErrNone != FskAudioOutIsValid(audioOut, &isValid)) || !isValid)
		return;

	ext = (androidAudioExt*)audioOut->ext;

	removeFromQueue(audioOut, kUsed);
	if (false == audioOut->playing)
		return;

	refillQueue(audioOut);
}

void refillQueue(FskAudioOut audioOut) {
	void *silence;
	FskAudioOutBlock block;
	androidAudioExt	*ext;

	ext = (androidAudioExt*)audioOut->ext;

	FskAudioNativePrintfDebug("refillQueue");
	if (audioOut->moreCB) {
		SInt32 samplesTarget;
//		SInt32 chunkRequestSize = 8192;
		SInt32 chunkRequestSize = AUDIO_DATA_BUFFER_SIZE;
		samplesTarget = audioOut->sampleRate * audioOut->numChannels;

		if (audioOut->chunkRequestSize)
			chunkRequestSize = audioOut->chunkRequestSize;
		if (audioOut->bufferedSamplesTarget)
			samplesTarget = audioOut->bufferedSamplesTarget;

		while (true) {
			SInt32 samplesNeeded;
			FskErr err;
			samplesNeeded = samplesTarget - (ext->bytesEnqueued / ext->sampleSize);
			//FskAudioNativePrintfDebug("samplesNeeded %d - samplesTarget %d", samplesNeeded, samplesTarget);
			if (samplesNeeded < chunkRequestSize)
				break;
			if (kFskErrNone == (err = (audioOut->moreCB)(audioOut, audioOut->moreRefCon, chunkRequestSize)))
				continue;
#if DONT_GENERATE_SILENCE
			break;
#else
			// make silence
			FskAudioNativePrintfDebug("making silence because more callback returns %d", err);
			FskMemPtrNewClear(chunkRequestSize * 2 * audioOut->numChannels, (FskMemPtr*)&silence);
			if (kFskErrNone != audioOutEnqueue(audioOut, silence, chunkRequestSize * 2 * audioOut->numChannels, NULL, 0, NULL, &block))
				break;
			block->silence = true;
#endif
		}
	}
	FskAudioNativePrintfDebug("refillQueue - DONE");
}

void androidAudioOutFlush(FskAudioOut audioOut) {
	SLresult res;

	FskAudioNativePrintfVerbose("audioOut %x flush", audioOut);
	if (audioOut) {
		androidAudioExt	*ext;
		ext = (androidAudioExt*)audioOut->ext;
//		ext->native->flush();

		res = (*ext->bufferQueueItf)->Clear(ext->bufferQueueItf);
		CheckErr("AudioOutFlush - calling clear on bufferQueue", res);

		if (ext->audioFilled[0]) {
			FskAudioNativePrintfDebug("there were %d bytes left in buffer 0", ext->audioFilled[0]);
			ext->audioFilled[0] = 0;
		}
		if (ext->audioFilled[1]) {
			FskAudioNativePrintfDebug("there were %d bytes left in buffer 1", ext->audioFilled[1]);
			ext->audioFilled[1] = 0;
		}

		ext->bytesEnqueued = 0;
	}
}



void BufferQueueCallback(SLAndroidSimpleBufferQueueItf queueItf, void *pContext) {
	SLresult res;
	androidAudioExt	*ext = (androidAudioExt*)pContext;
	FskAudioOut aOut = (FskAudioOut)ext->parent;
	Boolean isValid = false;
	int amt, written = 0;

	FskMutexAcquire(gActiveAudioMutex);
	if (!aOut) {
		FskAudioNativePrintfMinimal("In BQ callback - audioOut is NULL - bail");
		goto bail;
	}
	androidAudioOutIsValid(aOut, &isValid);
	if (!isValid || !aOut->playing) {
		FskAudioNativePrintfMinimal("In BQ callback - aOut is invalid %p || not playing", aOut);
		goto bail;
	}

	if (ext->audioFilled[ext->nextBuf]) {
		FskAudioNativePrintfDebug("BQCallback: buffer %d is filled and has %d bytes to enqueue.", ext->nextBuf, ext->audioFilled[ext->nextBuf]);
		res = (*queueItf)->Enqueue(queueItf, (void*)ext->audioBuf[ext->nextBuf], ext->audioFilled[ext->nextBuf]);
		CheckErr("BufferQueueCallback - Enqueue data", res);
		if (SL_RESULT_SUCCESS == res) {
			written += ext->audioFilled[ext->nextBuf];
			ext->audioFilled[ext->nextBuf] = 0;
			ext->nextBuf = ext->nextBuf ? 0 : 1;
		}
	}
	else {
		FskAudioNativePrintfDebug("BQCallback: buffer wasn't filled - what to do? a: %d, b: %d", ext->audioFilled[0], ext->audioFilled[1]);
	}

	if (!ext->audioFilled[ext->nextBuf]) {
		FskAudioNativePrintfDebug("BQCallback:  buffer %d is not filled - attempting to fill", ext->nextBuf);

		amt = linuxAudioOutPCM(aOut, ext->audioBuf[ext->nextBuf], AUDIO_DATA_BUFFER_SIZE);
		if (amt > 0) {
			FskAudioNativePrintfDebug("    buffer %d filled with %d bytes", ext->nextBuf, amt);
			ext->audioFilled[ext->nextBuf] = amt;
		}
		else {
			FskAudioNativePrintfMinimal("    buffer %d didn't fill", ext->nextBuf);
			ext->audioFilled[ext->nextBuf] = 0;
		}
	}

bail:
	if (!written) {
		FskAudioNativePrintfMinimal("TRY - nothing written to output - stall?");
		androidAudioOutGetSamplePosition(aOut, &ext->stoppedAtSamplePosition);
		res = (*ext->playItf)->SetPlayState(ext->playItf, SL_PLAYSTATE_STOPPED);
		aOut->playing = false;
	}
	else {
//		FskAudioNativePrintfDebug("wrote - written: %d", written);

	}
	FskMutexRelease(gActiveAudioMutex);
	return;
}



FskErr androidAudioOutNew(FskAudioOut *audioOutOut, UInt32 outputID, UInt32 format) {
	SLresult res;
	FskErr err = kFskErrNone;
	FskAudioOut audioOut = NULL;
	androidAudioExt	*ext = NULL;
	int i;

	SLDataLocator_OutputMix loc_outmix;
	SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
	SLDataFormat_PCM pcm;

	SLDataSink audioSnk;
	SLDataSource audioSrc = {&loc_bufq, &pcm};

	if (NULL == audioOuts) {
		err = FskListMutexNew(&audioOuts, "audioOuts");
		if (err) goto bail;
	}

	err = FskMemPtrNewClear(sizeof(FskAudioOutRecord), (FskMemPtr*)&audioOut);
	if (err) goto bail;
	err = FskMemPtrNewClear(sizeof(androidAudioExt), (FskMemPtr*)&ext);
	if (err) goto bail;
	err = FskMemPtrNewClear(AUDIO_DATA_BUFFER_SIZE, (FskMemPtr*)&ext->audioBuf[0]);
	if (err) goto bail;
	err = FskMemPtrNewClear(AUDIO_DATA_BUFFER_SIZE, (FskMemPtr*)&ext->audioBuf[1]);
	if (err) goto bail;
	ext->nextBuf = 0;
	ext->audioFilled[0] = 0;
	ext->audioFilled[1] = 0;

	audioOut->ext = ext;
	ext->parent = audioOut;

	audioOut->thread = FskThreadGetCurrent();
	audioOut->format = kFskAudioFormatPCM16BitLittleEndian;

	audioOut->sampleRate = 44100;
	audioOut->numChannels = 2;
	ext->sampleSize = audioOut->numChannels * 2;

    if (sBufferedSamplesTarget > 0 && sBufferedSamplesTarget < 20) {
		audioOut->bufferedSamplesTarget = sBufferedSamplesTarget * audioOut->sampleRate * ext->sampleSize;
	}
	else
		audioOut->bufferedSamplesTarget = audioOut->sampleRate * ext->sampleSize;   // 1 second default

	SLboolean required[MAX_NUMBER_INTERFACES];
	SLInterfaceID iidArray[MAX_NUMBER_INTERFACES];

    pcm.formatType = SL_DATAFORMAT_PCM;
    pcm.numChannels = audioOut->numChannels;
    pcm.samplesPerSec = audioOut->sampleRate * 1000;
    pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    pcm.containerSize = 16;
    pcm.channelMask = audioOut->numChannels == 1 ? SL_SPEAKER_FRONT_CENTER :
        (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT);
    pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

	for (i=0; i<MAX_NUMBER_INTERFACES; i++) {
		required[i] = SL_BOOLEAN_FALSE;
		iidArray[i] = SL_IID_NULL;
	}

	res = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, (void*)&ext->EngineItf);
	CheckErr("GetInterface", res);

	res = (*ext->EngineItf)->CreateOutputMix(ext->EngineItf, &ext->outputMixObject, 0, NULL, NULL);
	CheckErr("CreateOutputMix", res);

	res = (*ext->outputMixObject)->Realize(ext->outputMixObject, SL_BOOLEAN_FALSE);
	CheckErr("Realize OutputMix", res);

	loc_outmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
	loc_outmix.outputMix = ext->outputMixObject;
	audioSnk.pLocator = &loc_outmix;
	audioSnk.pFormat = NULL;

	iidArray[0] = SL_IID_BUFFERQUEUE;
	required[0] = SL_BOOLEAN_TRUE;

	res = (*ext->EngineItf)->CreateAudioPlayer(ext->EngineItf, &ext->player, &audioSrc, &audioSnk, 1, iidArray, required);
	CheckErr("CreateAudioPlayer", res);

	res = (*ext->player)->Realize(ext->player, SL_BOOLEAN_FALSE);
	CheckErr(" player->Realize", res);

	res = (*ext->player)->GetInterface(ext->player, SL_IID_PLAY, (void*)&ext->playItf);
	CheckErr(" ext->player->GetInterface Play", res);

	res = (*ext->player)->GetInterface(ext->player, SL_IID_BUFFERQUEUE, (void*)&ext->bufferQueueItf);
	CheckErr(" player->GetInterface BufferQueue", res);

	res = (*ext->bufferQueueItf)->RegisterCallback(ext->bufferQueueItf, BufferQueueCallback, ext);
	CheckErr(" bufferQueueItf->RegisterCallback  BufferQueue", res);

//	res = (*ext->volumeItf)->SetVolumeLevel(ext->volumeItf, -300);
//	CheckErr("  volumeItf->SetVolumeLevel", res);


//	if (gAudio)
//		FskAudioNativePrintfDebug("- about to trash existing gAudio %x", gAudio);


	FskListMutexNew(&audioOut->blocks, "audio blocks");
	FskMutexNew(&ext->getSamplePositionMutex, "audio getSamplePosition");

	FskListMutexAppend(audioOuts, audioOut);

bail:
	if (err) {
		androidAudioOutDispose(audioOut);
		audioOut = NULL;
	}
	*audioOutOut = audioOut;
	return err;
}


FskErr androidAudioOutDispose(FskAudioOut audioOut) {
	androidAudioExt *ext;

	FskAudioNativePrintfVerbose("audioOutDispose %x", audioOut);
	if (audioOut == NULL)
		return kFskErrNone;
	ext = (androidAudioExt*)audioOut->ext;

	FskAudioNativePrintfVerbose("audioOutDispose ext is %x", ext);
	if (ext) {
		androidAudioOutStop(audioOut);
audioOut->ext = 0;

		FskListMutexRemove(audioOuts, audioOut);

		FskAudioNativePrintfDebug("removing audioOut->blocks list %x", audioOut->blocks);
		FskListMutexDispose(audioOut->blocks);
		FskMutexDispose(ext->getSamplePositionMutex);

		if (ext->playItf) {
			SLresult	res;
 			FskAudioNativePrintfDebug("before delete ext->playItf: %x", ext->playItf);
			res = (*ext->playItf)->SetPlayState(ext->playItf, SL_PLAYSTATE_STOPPED);
			CheckErr("audioOutDispose - set playstate STOPPED", res);
			res = (*ext->bufferQueueItf)->Clear(ext->bufferQueueItf);
			CheckErr("audioOutDispose - calling clear on bufferQueue", res);

 			FskAudioNativePrintfDebug("after delete ext->playItf: %x", ext->playItf);
 		}

		if (ext->audioBuf[0])
			FskMemPtrDispose(ext->audioBuf[0]);
		if (ext->audioBuf[1])
			FskMemPtrDispose(ext->audioBuf[1]);

		if (ext->player != NULL)
			(*ext->player)->Destroy(ext->player);
		if (ext->outputMixObject != NULL)
			(*ext->outputMixObject)->Destroy(ext->outputMixObject);

		FskMemPtrDispose(ext);
	}

	FskMemPtrDispose(audioOut);
	return kFskErrNone;
}



FskErr androidAudioOutIsValid(FskAudioOut audioOut, Boolean *isValid) {
	FskAudioOut walker = NULL;

	if (!audioOuts) {
		*isValid = false;
		return kFskErrNone;
	}

	FskListMutexAcquireMutex(audioOuts);
	*isValid = false;
	while (true) {
		walker = (FskAudioOut)FskListGetNext(audioOuts->list, (void*)walker);
		if (NULL == walker)
			break;
		if (walker == audioOut)
			*isValid = true;
	}
	FskListMutexReleaseMutex(audioOuts);
	return kFskErrNone;
}


FskErr androidAudioOutGetFormat(FskAudioOut audioOut, UInt32 *format, UInt16 *numChannels, double *sampleRate) {
	if (format) *format = audioOut->format;
	if (numChannels) *numChannels = audioOut->numChannels;
	if (sampleRate) *sampleRate = audioOut->sampleRate;
	return kFskErrNone;
}


FskErr androidAudioOutSetFormat(FskAudioOut audioOut, UInt32 format, UInt16 numChannels, double sampleRate, unsigned char *formatInfo, UInt32 formatInfoSize) {
	// nuttin. We'll use a fixed format
	return kFskErrNone;
}


FskErr androidAudioOutGetOutputBufferSize(FskAudioOut audioOut, UInt32 *chunkSize, UInt32 *bufferedSamplesTarget, UInt32 *minimumBytesToStart) {
//	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;
	if (chunkSize) {
		if (audioOut->chunkRequestSize)
			*chunkSize = audioOut->chunkRequestSize;
		else
			*chunkSize = 0;
	}
	if (bufferedSamplesTarget) {
		if (audioOut->bufferedSamplesTarget)
			*bufferedSamplesTarget = audioOut->bufferedSamplesTarget;
		else
			*bufferedSamplesTarget = audioOut->sampleRate * 4;
	}
	if (NULL != minimumBytesToStart)
		*minimumBytesToStart = 0;
	FskAudioNativePrintfVerbose("androidAudioOutGetOutputBufferSize returns %d %d", chunkSize ? *chunkSize : 0, bufferedSamplesTarget ? *bufferedSamplesTarget : 0 );
	return kFskErrNone;
}


FskErr androidAudioOutSetOutputBufferSize(FskAudioOut audioOut, UInt32 chunkSize, UInt32 bufferedSamplesTarget) {
	if (NULL == audioOuts)
		return kFskErrNone;
		FskAudioNativePrintfVerbose("androidAudioOutSetOutputBufferSize %u, %u", (unsigned)chunkSize, (unsigned)bufferedSamplesTarget);
	if (audioOut) {
		audioOut->chunkRequestSize = chunkSize;
		audioOut->bufferedSamplesTarget = bufferedSamplesTarget;
		return kFskErrNone;
	}
    else {
        FskAudioOut walker = NULL;
sBufferedSamplesTarget = bufferedSamplesTarget;
        FskListMutexAcquireMutex(audioOuts);
        while (true) {
            walker = (FskAudioOut)FskListGetNext(audioOuts->list, (void*)walker);
            if (NULL == walker)
                break;
            walker->chunkRequestSize = AUDIO_DATA_BUFFER_SIZE;
            if (bufferedSamplesTarget > 0 && bufferedSamplesTarget < 20) {
                walker->bufferedSamplesTarget = bufferedSamplesTarget * walker->sampleRate * walker->numChannels * 2;
            }
            else
                walker->bufferedSamplesTarget = walker->sampleRate * walker->numChannels * 2;   // 1 second default
        }
        FskListMutexReleaseMutex(audioOuts);
        return kFskErrNone;
    }

}


FskErr androidAudioOutGetVolume(FskAudioOut audioOut, UInt16 *left, UInt16 *right) {
	unsigned int lvol, rvol;
	FskErr err = kFskErrNone;
	SLboolean	muted = false;
//	SLresult	res;

	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;

//	res = (*ext->volumeItf)->GetMute(ext->volumeItf, &muted);
//	CheckErr("  volumeItf->GetMute", muted);

	lvol = gAndroidCallbacks->getVolumeCB();
	if (muted) {
		audioOut->leftVolume = -lvol;
		audioOut->rightVolume = -lvol;
	}
	else {
		audioOut->leftVolume = lvol;
		audioOut->rightVolume = lvol;
	}

	if (left) *left = audioOut->leftVolume;
	if (right) *right = audioOut->rightVolume;

	FskAudioNativePrintfVerbose("androidAudioOutGetVolume - left: %d, right %d", audioOut->leftVolume, audioOut->rightVolume);


bail:
	return err;
}


FskErr androidAudioOutSetVolume(FskAudioOut audioOut, UInt16 left, UInt16 right) {
	double dvol;
	FskErr err = kFskErrNone;
	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;
	SLboolean	muted;
//	SLresult	res;

//	res = (*ext->volumeItf)->GetMute(ext->volumeItf, &muted);
//	CheckErr("  SetVolume: volumeItf->GetMute", muted);


	FskAudioNativePrintfVerbose("androidAudioOutSetVolume - left: %d, right %d", left, right);

//	if (left != right)
	FskAudioNativePrintfVerbose("androidAudioOutSetVolume -force software\n");
		return kFskErrUnimplemented;	// force software balance

	audioOut->leftVolume = left;
	audioOut->rightVolume = right;

	dvol = (double)left / 256.0;
	gAndroidCallbacks->setVolumeCB(dvol);
// FskAudioNativePrintfDebug("androidAudioOutSetVolume setting hw volume %d %g", left, dvol);

bail:
	return err;
}


FskErr androidAudioOutSetDoneCallback(FskAudioOut audioOut, FskAudioOutDoneCallback cb, void *refCon) {
	audioOut->doneCB = cb;
	audioOut->doneRefCon = refCon;
	return kFskErrNone;
}


FskErr androidAudioOutSetMoreCallback(FskAudioOut audioOut, FskAudioOutMoreCallback cb, void *refCon) {
	audioOut->moreCB = cb;
	audioOut->moreRefCon = refCon;
	return kFskErrNone;
}

void fillBuffers(FskAudioOut audioOut) {
	androidAudioExt *ext;
	Boolean isValid = false;
	Boolean doWake = false;
	int nxtBuf, amt;
	SLresult	res;

	if ((kFskErrNone != FskAudioOutIsValid(audioOut, &isValid)) || !isValid)
	{
		FskAudioNativePrintfMinimal("in fillBuffers audioOut %p - %p isn't valid", audioOut, audioOut);
		return;
	}

	ext = (androidAudioExt*)audioOut->ext;
	if (!ext || !ext->playItf)
	{
		FskAudioNativePrintfMinimal("in fillBuffers no ext %p - or no native: %p", ext, ext ? ext->playItf : 0);
		return;
	}

	nxtBuf = ext->nextBuf;
	if (!ext->audioFilled[nxtBuf]) {
		FskAudioNativePrintfDebug("fillBytes A -  buffer %d is not filled - attempting to fill", nxtBuf);
		amt = linuxAudioOutPCM(audioOut, ext->audioBuf[nxtBuf], AUDIO_DATA_BUFFER_SIZE);
		FskAudioNativePrintfDebug(" -- fillBytes A -  got %d bytes", amt);
		if (amt > 0)
			ext->audioFilled[nxtBuf] = amt;
		else {
			ext->audioFilled[nxtBuf] = 0;
			doWake = true;
			goto ugh;
		}
	}
	nxtBuf = nxtBuf ? 0 : 1;
	if (!ext->audioFilled[nxtBuf]) {
		FskAudioNativePrintfDebug("fillBytes B -  buffer %d is not filled - attempting to fill", nxtBuf);
		amt = linuxAudioOutPCM(audioOut, ext->audioBuf[nxtBuf], AUDIO_DATA_BUFFER_SIZE);
		if (amt > 0)
			ext->audioFilled[nxtBuf] = amt;
		else {
			ext->audioFilled[nxtBuf] = 0;
			doWake = true;
		}
		FskAudioNativePrintfDebug(" -- fillBytes B -  got %d bytes", amt);
	}

	if (ext->audioFilled[ext->nextBuf]) {
		res = (*ext->bufferQueueItf)->Enqueue(ext->bufferQueueItf, ext->audioBuf[ext->nextBuf], ext->audioFilled[ext->nextBuf]);
		CheckErr(" fillBuffers - enqueue", res);

		if (res == SL_RESULT_SUCCESS) {
			ext->bytesEnqueued += ext->audioFilled[ext->nextBuf];
			ext->audioFilled[ext->nextBuf] = 0;
		}
		ext->nextBuf = ext->nextBuf ? 0 : 1;
	}

ugh:
	if (doWake)
		FskThreadWake(audioOut->thread);
}



void flushNowCallback(FskTimeCallBack callback, const FskTime time, void *param) {
	FskAudioOut audioOut = (FskAudioOut)param;
	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;
	Boolean isValid = false;

	FskAudioNativePrintfDebug("flushNowCallback");

	androidAudioOutIsValid(audioOut, &isValid);

	if (!isValid || audioOut->thread == NULL) {
		FskAudioNativePrintfVerbose("flushNowCallback - isValid? (%s) - audioOut->thread (%x) - bail", isValid ? "yes" : "no", audioOut->thread);
		return;
	}

//	removeFromQueue(audioOut, kUsed);
	if (FskThreadGetCurrent() == audioOut->thread)
		flushAndRefill(audioOut, NULL, NULL, NULL);
	else
		FskThreadPostCallback(audioOut->thread, flushAndRefill, audioOut, NULL, NULL, NULL);
	if (audioOut->playing)
		FskTimeCallbackScheduleFuture(ext->flushTimer, 0, kFlushAndRefillTime, flushNowCallback, audioOut);
}

//#define MSToSamples(audioOut, ms)	(audioOut->sampleRate * ms) / 1000

FskInt64 MSToSamples(FskAudioOut audioOut, SLmillisecond msec) {
	long long ms, rate, out;
    
	ms = (long long)msec;
	rate = (long long)audioOut->sampleRate;
	out = (rate * ms) / 1000LL;
    
	return (FskInt64)out;
};

FskErr androidAudioOutStart(FskAudioOut audioOut, FskSampleTime atSample) {
	FskErr err = kFskErrNone;
	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;
	SLresult	res;

	FskAudioNativePrintfVerbose("audioOutStart %x - atSample %lld", audioOut, atSample);
	if (audioOut->playing)
		goto bail;
	if (!ext->playItf) {
		FskAudioNativePrintfMinimal("huh? No playItf");
		err = kFskErrOperationFailed;
		goto bail;
	}

	audioOut->zeroTime = atSample;
	ext->stoppedAtSamplePosition = 0;

	FskTimeCallbackNew(&ext->flushTimer);
	FskTimeCallbackScheduleFuture(ext->flushTimer, 0, kFlushAndRefillTime, flushNowCallback, audioOut);
	FskAudioNativePrintfVerbose("androidAudioOutStart %x - zeroTime %d", audioOut, audioOut->zeroTime);

	// in case volume has changed when audio was off
//	androidAudioOutSetVolume(audioOut, audioOut->leftVolume, audioOut->rightVolume);

	refillQueue(audioOut);
	fillBuffers(audioOut);
	audioOut->playing = true;
//	refillQueue(audioOut);

    FskTimeGetNow(&ext->lastTime);

	FskMutexAcquire(gActiveAudioMutex);
	gActiveAudioOut = audioOut;
	FskMutexRelease(gActiveAudioMutex);

	res = (*ext->playItf)->SetPlayState(ext->playItf, SL_PLAYSTATE_PLAYING);
	CheckErr(" audioOutStart - set playstate playing", res);

	{
		SLmillisecond msec;
		res = (*ext->playItf)->GetPosition(ext->playItf, &msec);
		CheckErr(" androidAudioOutStart", res);
		ext->startedAtSamplePosition = MSToSamples(audioOut, msec);
	}

bail:
	return err;
}


FskErr androidAudioOutStop(FskAudioOut audioOut) {
	androidAudioExt	*ext;
	FskSampleTime pos;
	SLresult	res;
	FskErr err = kFskErrNone;

	FskAudioNativePrintfVerbose("audioOutStop %x", audioOut);

	BAIL_IF_NULL(audioOut, err, kFskErrNone);

	ext = (androidAudioExt*)audioOut->ext;
	BAIL_IF_NULL(ext, err, kFskErrNone);

	if (ext->flushTimer) {
		FskAudioNativePrintfVerbose("%x -- Disposing of flushTimer", audioOut);
		FskTimeCallbackDispose(ext->flushTimer);
		ext->flushTimer = NULL;
	}

	if (!audioOut->playing) {
		FskAudioNativePrintfDebug(" -- wuzn't playin");
		BAIL(kFskErrNone);
	}


	androidAudioOutGetSamplePosition(audioOut, &ext->stoppedAtSamplePosition);	// get final pos before we shut it down
	FskAudioNativePrintfVerbose("stoppedAtSamplePosition = %lld", ext->stoppedAtSamplePosition);

	audioOut->playing = false;
	FskAudioNativePrintfVerbose("-- stopping audioOut: %x", audioOut);

	res = (*ext->playItf)->SetPlayState(ext->playItf, SL_PLAYSTATE_STOPPED);
	CheckErr(" audioOutStop - set playstate stopped", res);

	if (gActiveAudioOut == audioOut) {
		FskMutexAcquire(gActiveAudioMutex);
		gActiveAudioOut = NULL;
		FskMutexRelease(gActiveAudioMutex);
	}

	androidAudioOutFlush(audioOut);

bail:
	removeFromQueue(audioOut, kAll);

	return err;
}


FskErr audioOutEnqueue(FskAudioOut audioOut, void *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, UInt32 *frameSizes, FskAudioOutBlock *blockOut) {
	FskErr err;
	FskAudioOutBlock block;
	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;

	err = FskMemPtrNewClear(sizeof(FskAudioOutBlockRecord), (FskMemPtr*)&block);
	BAIL_IF_ERR(err);

	block->data = (unsigned char*)data;
	block->dataSize = dataSize;
	block->sampleCount = dataSize / (2 * audioOut->numChannels);
	block->refCon = dataRefCon;
	block->audioOut = audioOut;
	FskListMutexAppend(audioOut->blocks, block);

//	FskAudioNativePrintfDebug("Enqueuing %d bytes (total %d)", dataSize, ext->bytesEnqueued + dataSize);
	ext->bytesEnqueued += dataSize;

bail:
	if (blockOut) *blockOut = block;

	return err;
}


FskErr androidAudioOutEnqueue(FskAudioOut audioOut, void *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, UInt32 *frameSizes) {
	return audioOutEnqueue(audioOut, data, dataSize, dataRefCon, frameCount, frameSizes, NULL);
}


FskErr androidAudioOutGetSamplePosition(FskAudioOut audioOut, FskSampleTime *position) {
	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;
	FskInt64 add;
	FskErr err = kFskErrNone;
	SLresult	res;
	UInt32 pos;

	FskMutexAcquire(ext->getSamplePositionMutex);
	*position = audioOut->zeroTime;

	if (audioOut->playing) {
		SLmillisecond msec;
		res = (*ext->playItf)->GetPosition(ext->playItf, &msec);
		CheckErr(" audioOutGetSamplePosition", res);

		add = MSToSamples(audioOut, msec);
		pos = (UInt32)add;
		FskAudioNativePrintfVerbose("GetSamplePosition returns msec: %d, pos: %d", msec, pos);
	}
	else {
		add = ext->stoppedAtSamplePosition;
	}

	add -= ext->startedAtSamplePosition;
	*position += add;


	FskAudioNativePrintfVerbose("getSamplePosition %d - %d ms -- start@ %d, last stop@ %d, zero@ %lld", (UInt32)*position, (*position * 1000) / audioOut->sampleRate , (UInt32)ext->startedAtSamplePosition, (UInt32)ext->stoppedAtSamplePosition, audioOut->zeroTime);

releaseAndBail:
	FskMutexRelease(ext->getSamplePositionMutex);

	return err;
}


FskErr androidAudioOutGetSamplesQueued(FskAudioOut audioOut, UInt32 *samplesQueuedOut, UInt32 *targetQueueLengthOut) {
	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;
	if (samplesQueuedOut)
		*samplesQueuedOut = ext->bytesEnqueued / ext->sampleSize;
	if (targetQueueLengthOut)
		*targetQueueLengthOut = audioOut->sampleRate * 2;

	return kFskErrNone;
}


FskErr androidAudioOutSingleThreadedClient(FskAudioOut audioOut, Boolean *isSingleThreaded) {
	*isSingleThreaded = false;
	return kFskErrNone;
}


int linuxAudioOutPCM(FskAudioOut audioOut, char *p, int size) {
	FskErr err = kFskErrNone;
	FskAudioOutBlock audioBlock;
	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;
	int wrote = 0, remains = 0;
int origSize = size;

	if (NULL == audioOut->blocks->list)
		goto bail;

	if (0 != FskMutexTrylock(audioOut->blocks->mutex)) {
		FskMutexAcquire(audioOut->blocks->mutex);
	}
//FskAudioNativePrintfDebug("before FskListMutex - grabbed blocks %x (blocks)", audioOut->blocks);
	while (size) {
		char *pos;
		audioBlock = (FskAudioOutBlock)audioOut->blocks->list;
		while (audioBlock && audioBlock->done)
{
			audioBlock = audioBlock->next;
}
		if (!audioBlock) {
			if (0 == ext->bytesEnqueued) {
				FskAudioNativePrintfMinimal("no more free PCM blocks to play");
				FskAudioNativePrintfMinimal(" - there are %u bytes enqueued - wanted %d (orig size %d)", (unsigned)ext->bytesEnqueued, size, origSize);
			}
			goto bail;
		}

		if (audioBlock->loc)		// remainder
			 pos = audioBlock->loc;
		else
			 pos = (char*)audioBlock->data;

		if (audioBlock->dataSize <= (unsigned)size) {
			FskMemCopy(p, pos, audioBlock->dataSize);
			wrote += audioBlock->dataSize;
			p += audioBlock->dataSize;
			size -= audioBlock->dataSize;
			ext->bytesEnqueued -= audioBlock->dataSize;
audioBlock->dataSize = 0;	// only setting this for a debug message
			audioBlock->done = true;
			continue;
		}
		else {
			FskMemCopy(p, pos, size);
			wrote += size;
			audioBlock->loc = pos + size;
			audioBlock->dataSize -= size;
			FskAudioNativePrintfDebug("Consuming %d bytes (block left: %d) (total left %d)", size, audioBlock->dataSize, ext->bytesEnqueued - size);
			ext->bytesEnqueued -= size;
			size = 0;
			break;
		}
	}
bail:
	FskMutexRelease(audioOut->blocks->mutex);

	return wrote;
}




AudioOutVectors avec;


void androidFskAudioInitialize() {
	SLresult res;
	char *modelName;

	gAndroidCallbacks->getModelInfoCB(&modelName, NULL, NULL, NULL, NULL);

	FskAudioNativePrintfMinimal("FskAudioInitialize - model is %s", modelName);
	avec.doNew = androidAudioOutNew;
	avec.doDispose = androidAudioOutDispose;
	avec.doIsValid = androidAudioOutIsValid;
	avec.doGetFormat = androidAudioOutGetFormat;
	avec.doSetFormat = androidAudioOutSetFormat;
	avec.doSetOutputBufferSize = androidAudioOutSetOutputBufferSize;
	avec.doGetVolume = androidAudioOutGetVolume;
	avec.doSetVolume = androidAudioOutSetVolume;
	avec.doSetDoneCallback = androidAudioOutSetDoneCallback;
	avec.doSetMoreCallback = androidAudioOutSetMoreCallback;
	avec.doStart = androidAudioOutStart;
	avec.doStop = androidAudioOutStop;
	avec.doEnqueue = androidAudioOutEnqueue;
	avec.doGetSamplePosition = androidAudioOutGetSamplePosition;
	avec.doGetSamplesQueued = androidAudioOutGetSamplesQueued;
	avec.doSingleThreadedClient = androidAudioOutSingleThreadedClient;
	avec.doGetDeviceHandle = NULL;
	avec.doGetOutputBufferSize = androidAudioOutGetOutputBufferSize;

	FskAudioOutSetVectors(&avec);
	if (modelName)
		FskMemPtrDispose(modelName);

	FskMutexNew(&gActiveAudioMutex, "activeAudioMutex");

	res = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
	CheckErr("slCreateEngine", res);

	res = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
	CheckErr("slCreateEngine - realize", res);

}

void androidFskAudioTerminate() {
	FskAudioNativePrintfMinimal("FskAudioTerminate");
	FskAudioOutSetVectors(NULL);
	FskMutexDispose(gActiveAudioMutex);
	(*engineObject)->Destroy(engineObject);
	engineObject = NULL;
}

