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
#define __FSKTHREAD_PRIV__


#include <media/AudioTrack.h>

#define DEBUG 0
#define DEBUGTIMER 0

#define FORCE_SINGLE_AUDIO	0


#define USE_VOLUME	0

#define kFskAudioBuffers 8// 4	// 4? 8
#define DONT_GENERATE_SILENCE 1

#include "androidAudio.h"
#include "FskAudio.h"
#include "FskMemory.h"
#include "FskThread.h"
#include "FskHardware.h"

extern "C" {
#if USE_VOLUME
	extern void androidSetVolume(int vol);
	extern int androidGetVolume(void);
	extern int androidMaxVolume;
#endif

	void doDeleteNativeAudio(void *a, void *b, void *c, void *d);

	FskInstrumentedSimpleType(AudioNative, audionative);
}

FskListMutex audioOuts = NULL;

// SINGLE_AUDIO		-- for DROID X
FskMutex	gActiveAudioMutex = NULL;
FskAudioOut gActiveAudioOut = NULL;
android::AudioTrack *nativeOut = NULL;
int gSingleAudio = 0;		// use one FskAudio for whole run
int gAudioOutInUse = 0;

static int defaultChunkRequestSize = 8192;

static long sBufferedSamplesTarget = 0;

void fillBuffers(FskAudioOut audioOut);



typedef struct {
	Boolean		terminate;
	int			sampleSize;

	FskMutex		getSamplePositionMutex;
	FskSampleTime	startedAtSamplePosition;
	FskSampleTime	stoppedAtSamplePosition;
	UInt32		samplesLo;
	UInt32		samplesHi;

	UInt32		minFrameCount;
	UInt32		bytesEnqueued;
	FskTimeCallBack flushTimer;

	FskTimeRecord		lastTime;

	android::AudioTrack *native;
} androidAudioExt;

int linuxAudioOutPCM(FskAudioOut audioOut, char *p, int size);

#define kUsed 0
#define kAll	0xffffffff

void removeFromQueue(FskAudioOut audioOut, UInt32 what) {
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
					FskAudioNativePrintfVerbose("TRASHING audioblock with some data left: %u bytes\n", (unsigned)block->dataSize);
					FskAudioNativePrintfDebug(" tossing %u bytes (total left %u)\n", (unsigned)block->dataSize, (unsigned)(ext->bytesEnqueued - block->dataSize));
					ext->bytesEnqueued -= block->dataSize;
				}
				(audioOut->doneCB)(audioOut, audioOut->doneRefCon, block->refCon, true);
			}
			FskMemPtrDispose(block->frameSizes);
			FskMemPtrDispose(block);
		}
		FskMutexRelease(audioOut->blocks->mutex);
	}

}

void refillQueue(FskAudioOut audioOut);
void flushAndRefill(void *arg0, void *arg1, void *arg2, void *arg3) {
	FskAudioOut audioOut = (FskAudioOut)arg0;
	Boolean isValid = false;

	if ((kFskErrNone != FskAudioOutIsValid(audioOut, &isValid)) || !isValid)
		return;

	removeFromQueue(audioOut, kUsed);
	if (false == audioOut->playing)	
		return;

	refillQueue(audioOut);
}

void refillQueue(FskAudioOut audioOut) {
	androidAudioExt	*ext;

	ext = (androidAudioExt*)audioOut->ext;

	if (audioOut->moreCB) {
		SInt32 samplesTarget;
		SInt32 chunkRequestSize = defaultChunkRequestSize;
		samplesTarget = audioOut->sampleRate * audioOut->numChannels;

		if (audioOut->chunkRequestSize)
			chunkRequestSize = audioOut->chunkRequestSize;
		if (audioOut->bufferedSamplesTarget)
			samplesTarget = audioOut->bufferedSamplesTarget;

		while (true) {
			SInt32 samplesNeeded;
			FskErr err;
			samplesNeeded = samplesTarget - (ext->bytesEnqueued / ext->sampleSize);
			if (samplesNeeded < chunkRequestSize)
				break;
			if (kFskErrNone == (err = (audioOut->moreCB)(audioOut, audioOut->moreRefCon, chunkRequestSize)))
				continue;
#if DONT_GENERATE_SILENCE
			break;
#else
			{
			FskAudioOutBlock block;
			void *silence;
			// make silence
			FskAudioNativePrintfDebug("making silence because more callback returns %d\n", err);
			FskMemPtrNewClear(chunkRequestSize * 2 * audioOut->numChannels, (FskMemPtr*)&silence);
			if (kFskErrNone != audioOutEnqueue(audioOut, silence, chunkRequestSize * 2 * audioOut->numChannels, NULL, 0, NULL, &block))
				break;
			block->silence = true;
			}
#endif
		}
	}
}


void androidAudioOutFlush(FskAudioOut audioOut) {

	FskAudioNativePrintfVerbose("audioOut %x flush", audioOut);
	if (audioOut) {
		androidAudioExt	*ext;
		ext = (androidAudioExt*)audioOut->ext;
		ext->native->flush();
		ext->samplesLo = 0;
		ext->samplesHi = 0;
		ext->bytesEnqueued = 0;
	}
}



void needMoreCallback(int event, void *user, void *info) {
	FskAudioOut *aOut = (FskAudioOut*)user;
	FskAudioOut audioOut;
//	FskAudioOut audioOut = (FskAudioOut)user;
	androidAudioExt	*ext;
	char* q;
	android::AudioTrack::Buffer* b;
	Boolean isValid = false;

	FskMutexAcquire(gActiveAudioMutex);
audioOut = *aOut;
if (!audioOut) {
	FskAudioNativePrintfMinimal("In callback - audioOut is NULL - bail\n");
	goto bail;
}

	androidAudioOutIsValid(audioOut, &isValid);
	if (!isValid || !audioOut->playing) {
		FskAudioNativePrintfMinimal("In callback - audioOut is invalid %p || not playing\n", audioOut);
		goto bail;
	}

	ext = (androidAudioExt*)audioOut->ext;
	if (!ext) {
		FskAudioNativePrintfMinimal("In callback - ext is invalid %p\n", ext);
		goto bail;
	}

	switch (event) {
		case android::AudioTrack::EVENT_NEW_POS:
//			FskAudioNativePrintfDebug("audio callback - EVENT_NEW_POS\n");
			break;
		case android::AudioTrack::EVENT_UNDERRUN:
			FskAudioNativePrintfMinimal("audio callback - EVENT_UNDERRUN  trying to wake audio owner %s\n", audioOut->thread->name);
			FskThreadWake(audioOut->thread);
			break;
		case android::AudioTrack::EVENT_MORE_DATA:
			b = static_cast<android::AudioTrack::Buffer *>(info);

			if (b->size == 0)
				goto bail;

//FskAudioNativePrintfDebug("%x audioOut buffer wants %d \n", threadTag(FskThreadGetCurrent()), audioOut, b->size);
			// fill buffer
			q = (char*) b->i8;
			int amt;

			if (ext->bytesEnqueued >= b->size) {
				amt = linuxAudioOutPCM(audioOut, q, b->size);
//				FskAudioNativePrintfDebug("AudioOutPCM returns %d\n", amt);
			}
			else if (ext->bytesEnqueued) {
				FskAudioNativePrintfDebug("AudioOutPCM doesn't have enough data - only have %u\n", (unsigned)ext->bytesEnqueued);
				amt = linuxAudioOutPCM(audioOut, q, ext->bytesEnqueued);
				FskAudioNativePrintfDebug("trying to wake audio owner\n");
				FskThreadWake(audioOut->thread);
			}
			else
				amt = 0;
			if (amt >= 0) {
				b->size = amt;
			}
			break;
		default:
			FskAudioNativePrintfDebug("___ huh? not handling audio callback %d\n", event);
			break;
	}
bail:
	FskMutexRelease(gActiveAudioMutex);
	return;
}



FskErr androidAudioOutNew(FskAudioOut *audioOutOut, UInt32 outputID, UInt32 format) {
	FskErr err = kFskErrNone;
	FskAudioOut audioOut = NULL;
	androidAudioExt	*ext = NULL;

	if (NULL == audioOuts) {
		err = FskListMutexNew(&audioOuts, "audioOuts");
		BAIL_IF_ERR(err);
	}

	if (gSingleAudio) {
		FskAudioNativePrintfDebug("audioOutNew - single audio\n");
		if (gAudioOutInUse) {
			FskAudioNativePrintfDebug("audioOutNew - single audio - IN USE (%d)\n", gAudioOutInUse);
//			return kFskErrIsBusy;
		}
	}

	err = FskMemPtrNewClear(sizeof(FskAudioOutRecord), (FskMemPtr*)&audioOut);
	BAIL_IF_ERR(err);
	err = FskMemPtrNewClear(sizeof(androidAudioExt), (FskMemPtr*)&ext);
	BAIL_IF_ERR(err);
	audioOut->ext = ext;

	audioOut->thread = FskThreadGetCurrent();
	audioOut->format = kFskAudioFormatPCM16BitLittleEndian;

    int afSampleRate;
    int afFrameCount;

    if (android::AudioSystem::getOutputFrameCount(&afFrameCount, android::AudioSystem::MUSIC) != 0) {		// NO_ERROR
		FskAudioNativePrintfMinimal("failed getOutputFrameCount\n");
        return kFskErrMemFull;		// NO_INIT
    }

    if (android::AudioSystem::getOutputSamplingRate(&afSampleRate, android::AudioSystem::MUSIC) != 0) {		// NO_ERROR
		FskAudioNativePrintfMinimal("failed getOutputSamplingRate\n");
        return kFskErrMemFull;
    }

	audioOut->sampleRate = 44100;
    ext->minFrameCount = (audioOut->sampleRate*afFrameCount*kFskAudioBuffers)/afSampleRate;
	audioOut->numChannels = 2;
	ext->sampleSize = audioOut->numChannels * 2;

	if (sBufferedSamplesTarget > 0 && sBufferedSamplesTarget < 20) {
		audioOut->bufferedSamplesTarget = sBufferedSamplesTarget * audioOut->sampleRate * ext->sampleSize;
	}
	else
		audioOut->bufferedSamplesTarget = audioOut->sampleRate * audioOut->numChannels;   // 1/2 second default


	if (gSingleAudio && nativeOut) {
		gAudioOutInUse++;
		ext->native = nativeOut;
		FskAudioNativePrintfDebug("audioOutNew %p - single audio - REUSING (%d)\n", audioOut, gAudioOutInUse);
	}
	else {
#if (ANDROID_VERSION == 2 || ANDROID_VERSION == 3)
		ext->native = new android::AudioTrack(android::AudioSystem::MUSIC, audioOut->sampleRate, android::AudioSystem::PCM_16_BIT, android::AudioSystem::CHANNEL_OUT_STEREO, ext->minFrameCount * 2, 0, needMoreCallback, &gActiveAudioOut, ext->minFrameCount / 4);
#else
		ext->native = new android::AudioTrack(android::AudioSystem::MUSIC, audioOut->sampleRate, android::AudioSystem::PCM_16_BIT, 2, ext->minFrameCount * 2, 0, needMoreCallback, &gActiveAudioOut, ext->minFrameCount / 4);
#endif
		if (gSingleAudio) {
			gAudioOutInUse++;
			nativeOut = ext->native;
		}
	}

	FskAudioNativePrintfDebug("ext->native %x\n", ext->native);
	if ((ext->native == 0) || (ext->native->initCheck() != 0)) {
		FskAudioNativePrintfVerbose(" failed to initialize %d\n", ext->native->initCheck());
		LOGE("Unable to create audio track");
		delete ext->native;
		ext->native = NULL;
		err = kFskErrParameterError;
		goto bail;
	}

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

FskErr androidAudioOutDisposeSingle(FskAudioOut audioOut) {
	androidAudioExt *ext;

	FskAudioNativePrintfVerbose("audioOutDisposeSingle %x", audioOut);

	if (audioOut == NULL)
		return kFskErrNone;
	ext = (androidAudioExt*)audioOut->ext;

	FskAudioNativePrintfDebug("audioOutDisposeSingle ext is %x", ext);

	if (ext) {
		androidAudioOutStop(audioOut);
		audioOut->ext = 0;
		gAudioOutInUse--;

		FskListMutexRemove(audioOuts, audioOut);

		FskAudioNativePrintfDebug("removing audioOut->blocks list %x", audioOut->blocks);

		FskListMutexDispose(audioOut->blocks);
		FskMutexDispose(ext->getSamplePositionMutex);

		FskMemPtrDispose(ext);
	}

	FskMemPtrDispose(audioOut);
	return kFskErrNone;
}

FskErr androidAudioOutDispose(FskAudioOut audioOut) {
	androidAudioExt *ext;

	FskAudioNativePrintfVerbose("audioOutDispose %x", audioOut);
	if (audioOut == NULL)
		return kFskErrNone;
	ext = (androidAudioExt*)audioOut->ext;

	FskAudioNativePrintfDebug("audioOutDisposeext is %x", ext);
	if (ext) {
		androidAudioOutStop(audioOut);
audioOut->ext = 0;

		FskListMutexRemove(audioOuts, audioOut);

		FskAudioNativePrintfDebug("removing audioOut->blocks list %x", audioOut->blocks);
		FskListMutexDispose(audioOut->blocks);
		FskMutexDispose(ext->getSamplePositionMutex);

		if (ext->native) {
			delete ext->native;
 		}
		FskMemPtrDispose(ext);
	}

	FskMemPtrDispose(audioOut);
	return kFskErrNone;
}


FskErr androidAudioOutIsValid(FskAudioOut audioOut, Boolean *isValid) {
	FskAudioOut walker = NULL;
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
	FskAudioNativePrintfDebug("androidAudioOutGetOutputBufferSize returns %d %d", chunkSize ? *chunkSize : 0, bufferedSamplesTarget ? *bufferedSamplesTarget : 0 );

	return kFskErrNone;
}


FskErr androidAudioOutSetOutputBufferSize(FskAudioOut audioOut, UInt32 chunkSize, UInt32 bufferedSamplesTarget) {
	if (NULL == audioOuts)
		return kFskErrNone;
	FskAudioNativePrintfDebug("androidAudioOutSetOutputBufferSize %u, %u", (unsigned)chunkSize, (unsigned)bufferedSamplesTarget);

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
			walker->chunkRequestSize = defaultChunkRequestSize;
			if (bufferedSamplesTarget > 0 && bufferedSamplesTarget < 20) {
				walker->bufferedSamplesTarget = bufferedSamplesTarget * walker->sampleRate * walker->numChannels * 2;
			}
			else 
				walker->bufferedSamplesTarget = walker->sampleRate * walker->numChannels;	// 1/2 second default
		}
		FskListMutexReleaseMutex(audioOuts);
		return kFskErrNone;
	}
}


FskErr androidAudioOutGetVolume(FskAudioOut audioOut, UInt16 *left, UInt16 *right) {
	bool muted = false;
	FskErr err = kFskErrNone;

	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;

	muted = ext->native->muted();
#if USE_VOLUME	
	{
	unsigned int lvol, rvol;
	float flMax;
	lvol = androidGetVolume();
	rvol = lvol;
	flMax = (float)androidMaxVolume;
//FskAudioNativePrintfDebug("androidAudioOutGetVolume (l:%d r:%d mute:%s\n", lvol, rvol, muted ? "yes" : "no");
	lvol = ((float)lvol * 256.0) / flMax;
	rvol = lvol;

	if (muted) {
		audioOut->leftVolume = -lvol;
		audioOut->rightVolume = -rvol;
	}
	else {
		audioOut->leftVolume = lvol;
		audioOut->rightVolume = rvol;
	}
	}
#else
	if (muted) {
		audioOut->leftVolume = -audioOut->leftVolume;
		audioOut->rightVolume = -audioOut->rightVolume;
	}
//	else {
//		audioOut->leftVolume = audioOut->leftVolume;
//		audioOut->rightVolume = audioOut->rightVolume;
//	}
#endif

	if (left) *left = audioOut->leftVolume;
	if (right) *right = audioOut->rightVolume;

	return err;
}


FskErr androidAudioOutSetVolume(FskAudioOut audioOut, UInt16 left, UInt16 right) {
//	float lvol, rvol;
//	bool muted = false;
//	FskErr err = kFskErrNone;
//	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;

	FskAudioNativePrintfDebug("androidAudioOutSetVolume - left: %d, right %d", left, right);

//	if (left != right)
		return kFskErrUnimplemented;	// force software balance

#if 0
	audioOut->leftVolume = left;
	audioOut->rightVolume = right;

#if USE_VOLUME
	lvol = (float)left / 256.0;
	rvol = (float)right / 256.0;

	FskAudioNativePrintfDebug("androidAudioOutSetVolume - lvol: %f, rvol: %f\n", lvol, rvol);

//	if (lvol < 0) { lvol = -lvol; muted = true; }
//	if (rvol < 0) { rvol = -rvol; muted = true; }

	if (ext->native->muted()) {
		if (!muted)
			ext->native->mute(false);
	}
	else if (muted) {
		ext->native->mute(true);
	}

	if (!muted) {
		int vol;
		vol = (int)(lvol * (float)androidMaxVolume);
		androidSetVolume(vol);
//		ext->native->setVolume(lvol, rvol);
	}

	// FskAudioNativePrintfDebug("androidAudioOutSetVolume returns %d - %d:%d - hw= %f:%f\n", err, left, right, lvol, rvol);
#else
	// FskAudioNativePrintfDebug("androidAudioOutSetVolume setting hw volume %d %g", left, dvol);
#endif

	return err;
#endif
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
	android::AudioTrack::Buffer audioBuffer;
	androidAudioExt *ext;
	Boolean isValid = false;

	if ((kFskErrNone != FskAudioOutIsValid(audioOut, &isValid)) || !isValid) {
		FskAudioNativePrintfMinimal("in fillBuffers audioOut %p - %p isn't valid\n", audioOut, audioOut);
		return;
	}

	ext = (androidAudioExt*)audioOut->ext;
	if (!ext || !ext->native) {
		FskAudioNativePrintfDebug("in fillBuffers no ext %p - or no native: %p\n", ext, ext ? ext->native : 0);
		return;
	}

	while ((ext->bytesEnqueued / ext->sampleSize) > ext->minFrameCount) {
		audioBuffer.frameCount = ext->minFrameCount;
	FskAudioNativePrintfDebug("fillBuffers - obtainBuffer frames: %d\n", audioBuffer.frameCount);
		android::status_t status = ext->native->obtainBuffer(&audioBuffer, 0);
		int amt;
		if (status == (android::status_t)(android::AudioTrack::NO_MORE_BUFFERS)) {
			FskAudioNativePrintfDebug("fillBuffers - obtainBuffer: status NO_MORE_BUFFERS\n");
			return;
		}
		else if (status == android::AudioTrack::STOPPED) {
			FskAudioNativePrintfDebug("fillBuffers - obtainBuffer: status STOPPED\n");
		}
		else if (status != 0) {
			FskAudioNativePrintfDebug("fillBuffers - obtainBuffer: status %d\n", status);
			return;
		}
		amt = linuxAudioOutPCM(audioOut, (char*)audioBuffer.i8, audioBuffer.size);
		if ((unsigned)amt != audioBuffer.size) {
			FskAudioNativePrintfDebug("couldn't fill audioBuffer completely - huh? %d of %d\n",  amt, audioBuffer.size);
			audioBuffer.size = amt;
		}
		ext->native->releaseBuffer(&audioBuffer);
	};

}



void flushNowCallback(FskTimeCallBack callback, const FskTime time, void *param) {
	FskAudioOut audioOut = (FskAudioOut)param;
	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;

	FskAudioNativePrintfDebug("flushNowCallback");
//	removeFromQueue(audioOut, kUsed);
	FskThreadPostCallback(audioOut->thread, flushAndRefill, audioOut, NULL, NULL, NULL);
	if (audioOut->playing)
		FskTimeCallbackScheduleFuture(ext->flushTimer, 0, 500, flushNowCallback, audioOut);
}



FskErr androidAudioOutStart(FskAudioOut audioOut, FskSampleTime atSample) {
	FskErr err = kFskErrNone;
	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;

	FskAudioNativePrintfVerbose("audioOutStart %x - atSample %lld", audioOut, atSample);
	if (audioOut->playing)
		goto bail;
	if (!ext->native)
		FskAudioNativePrintfDebug("huh? No gAudio\n");

	if (gSingleAudio) {
		FskAudioOut walker;
		walker = NULL;
		FskListMutexAcquireMutex(audioOuts);
		while (true) {
			walker = (FskAudioOut)FskListGetNext(audioOuts->list, (void*)walker);
			if (NULL == walker)
				break;
			if (audioOut != walker) {
				FskAudioNativePrintfDebug("start of %p is causing stop of %p\n", audioOut, walker);
				androidAudioOutStop(walker);
			}
		}
		FskListMutexReleaseMutex(audioOuts);
	}


	audioOut->zeroTime = atSample;

	ext->samplesHi = 0;
	ext->samplesLo = 0;

	FskTimeCallbackNew(&ext->flushTimer);
	FskTimeCallbackScheduleFuture(ext->flushTimer, 0, 500, flushNowCallback, audioOut);
	FskAudioNativePrintfDebug("androidAudioOutStart %x - zeroTime %d", audioOut, audioOut->zeroTime);

	// in case volume has changed when audio was off
//	androidAudioOutSetVolume(audioOut, audioOut->leftVolume, audioOut->rightVolume);

	refillQueue(audioOut);
	fillBuffers(audioOut);
	audioOut->playing = true;
	refillQueue(audioOut);

    FskTimeGetNow(&ext->lastTime);

	FskMutexAcquire(gActiveAudioMutex);
	gActiveAudioOut = audioOut;
	FskMutexRelease(gActiveAudioMutex);
	
	ext->native->start();

uint32_t pos;
ext->native->getPosition(&pos);
ext->startedAtSamplePosition = pos;

FskAudioNativePrintfDebug("stoppedAtSamplePosition: %lld, startedAtSamplePosition %lld\n", ext->stoppedAtSamplePosition, ext->startedAtSamplePosition);


bail:
	return err;
}


FskErr androidAudioOutStop(FskAudioOut audioOut) {
	androidAudioExt	*ext;
//	FskSampleTime pos;

	FskAudioNativePrintfVerbose("audioOutStop %x", audioOut);

	if (!audioOut || !audioOut->playing) {
		FskAudioNativePrintfDebug(" -- wuzn't playin");
		goto bail;
	}

	ext = (androidAudioExt*)audioOut->ext;

//	androidAudioOutGetSamplePosition(audioOut, &ext->stoppedAtSamplePosition);	// get final pos before we shut it down
uint32_t npos;
ext->native->getPosition(&npos);
ext->stoppedAtSamplePosition = npos;
	FskAudioNativePrintfDebug("stoppedAtSamplePosition = %lld", ext->stoppedAtSamplePosition);
	
	audioOut->playing = false;
	FskAudioNativePrintfDebug("-- stopping audioOut: %x", audioOut);
	ext->native->stop();

	if (gActiveAudioOut == audioOut) {
		FskMutexAcquire(gActiveAudioMutex);
		gActiveAudioOut = NULL;
		FskMutexRelease(gActiveAudioMutex);
	}

	if (ext->flushTimer) {
		FskAudioNativePrintfVerbose("%x -- Disposing of flushTimer", audioOut);
		FskTimeCallbackDispose(ext->flushTimer);
		ext->flushTimer = NULL;
	}
	androidAudioOutFlush(audioOut);

	removeFromQueue(audioOut, kAll);	// coverity 10553 - move above bail
bail:

	return kFskErrNone;
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
//FskAudioNativePrintfDebug("[%s] enqueuing %d samples\n", FskThreadGetCurrent()->name, block->sampleCount);
	block->refCon = dataRefCon;
	block->audioOut = audioOut;
//FskAudioNativePrintfDebug("before FskListMutexAppend %x (blocks)\n", audioOut->blocks);
	FskListMutexAppend(audioOut->blocks, block);
//FskAudioNativePrintfDebug("after FskListMutexAppend %x (blocks)\n", audioOut->blocks);

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
	android::status_t status;
	FskInt64 add;
	FskErr err = kFskErrNone;

	FskMutexAcquire(ext->getSamplePositionMutex);
	*position = audioOut->zeroTime;

//	if (false == audioOut->playing)
//		goto releaseAndBail;

	if (audioOut->playing) {
		uint32_t pos;

		status = ext->native->getPosition(&pos);	// getPosition resets on Play
		if (status != 0) {
			FskAudioNativePrintfDebug("ext->native getPosition status; %d pos: %d\n", status, pos);
		}
		add = (FskInt64)pos;
	}
	else {
		add = ext->stoppedAtSamplePosition;
	}

	add -= ext->startedAtSamplePosition;
	*position += add;

{
FskAudioNativePrintfDebug("getSamplePosition %lld - %d ms -- ", *position, (*position * 1000) / audioOut->sampleRate);
FskAudioNativePrintfDebug("start@ %lld, last stop@ %lld\n", ext->startedAtSamplePosition, ext->stoppedAtSamplePosition);
}

//releaseAndBail:
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
//	FskErr err = kFskErrNone;
	FskAudioOutBlock audioBlock;
	androidAudioExt *ext = (androidAudioExt*)audioOut->ext;
	int wrote = 0;
int origSize = size;
int blocksBypassed = 0;

	if (NULL == audioOut->blocks->list)
		goto bail;

	if (0 != FskMutexTrylock(audioOut->blocks->mutex)) {
		FskMutexAcquire(audioOut->blocks->mutex);
	}
//	FskAudioNativePrintfDebug("before FskListMutex - grabbed blocks %x (blocks)\n", audioOut->blocks);
	while (size) {
		char *pos;
		audioBlock = (FskAudioOutBlock)audioOut->blocks->list;
		while (audioBlock && audioBlock->done)
{
blocksBypassed++;
			audioBlock = audioBlock->next;
}
		if (!audioBlock) {
			if (0 == ext->bytesEnqueued) {
				FskAudioNativePrintfDebug("no more free PCM blocks to play\n");
				FskAudioNativePrintfDebug(" - there are %u bytes enqueued - wanted %d (orig size %d)\n", (unsigned)ext->bytesEnqueued, size, origSize);
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
//FskAudioNativePrintfDebug("Eating %d bytes (tot left %d) %d bypassed\n", audioBlock->dataSize, ext->bytesEnqueued - audioBlock->dataSize, blocksBypassed));
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
//FskAudioNativePrintfDebug("Consuming %d bytes (block left: %d) (total left %d)\n", size, audioBlock->dataSize, ext->bytesEnqueued - size));
			ext->bytesEnqueued -= size;
			size = 0;
			break;
		}
	}
bail:
//FskAudioNativePrintfDebug("before FskListMutex - released blocks %x (blocks)\n", audioOut->blocks);
	FskMutexRelease(audioOut->blocks->mutex);

	return wrote;
}




AudioOutVectors avec;


void androidFskAudioInitialize() {
	char *modelName;

	gAndroidCallbacks->getModelInfoCB(&modelName, NULL, NULL, NULL, NULL);
	FskAudioNativePrintfDebug("AudioInit - model is %s\n", modelName);
	if (0 == FskStrCompare(modelName, "DROIDX")) {
		FskAudioNativePrintfDebug("AudioInit -- gSingleAudio\n");
		gSingleAudio = 1;
	}
#if FORCE_SINGLE_AUDIO
gSingleAudio = 1;
#endif

	FskAudioNativePrintfDebug("FskAudioInitialize\n");
	avec.doNew = androidAudioOutNew;
	if (gSingleAudio)
		avec.doDispose = androidAudioOutDisposeSingle;
	else
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
}
	
void androidFskAudioTerminate() {
	FskAudioNativePrintfDebug("FskAudioTerminate\n");
	FskAudioOutSetVectors(NULL);
	FskMutexDispose(gActiveAudioMutex);
}

