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
#include "Fsk.h"

#include "FskAudioIn.h"

#include "FskAudio.h"
#include "FskECMAScript.h"
#include "FskTime.h"
#include "FskHardware.h"

#if MINITV && !BG3CDP && !ANDROID_PLATFORM      /* @@ better conditional than this? */
    #define TARGET_RT_ALSA 1
#else
    #define TARGET_RT_ALSA 0
#endif

#if TARGET_OS_WIN32
#include "mmsystem.h"
#include "mmreg.h"
#include "Msacm.h"

typedef struct {
	void			*next;
	UInt32			maxSamples;
	WAVEHDR			waveHeader;
	char			data[2];
} AudioInQueueRecord, *AudioInQueue;

#elif TARGET_OS_MAC
	#include <AudioToolbox/AudioServices.h>
	#include <AudioToolbox/AudioQueue.h>
	#define kFskAudioMacBufferCount (8)
#if TARGET_OS_IPHONE
    #include "FskCocoaSupportCommon.h"
#endif
#endif

struct FskAudioInStruct { 
	UInt32						inSampleRate;
	UInt32						inFormat;
	UInt16						inNumChannels;

    SInt16                      useCount;
	Boolean                     disposing;
	Boolean                     recording;

	FskAudioInCallback			callback;
	void						*callbackRefCon;

	UInt32						timeQueued;
	FskListMutex				recordedQueue;

	FskTimeCallBack				timerCallback;
    
#if TARGET_OS_WIN32
	HANDLE						powerRequirement;
	HWAVEIN						waveIn;
#elif TARGET_OS_ANDROID
	void						*nativeIn;
#elif TARGET_OS_MAC
	AudioQueueRef				queue;
	FskThread					thread;
	FskMutex					mutex;

	UInt32						bufferCount;
	AudioQueueBufferRef			buffers[kFskAudioMacBufferCount];

	Boolean						active;
#elif TARGET_RT_ALSA
    int                         recStatus;
    FskThread                   alsaThread;
    FskThread                   clientThread;
#endif
};

static void audioInUse(FskAudioIn audioIn);
static void audioInUnuse(FskAudioIn audioIn);

#if TARGET_OS_WIN32
static void audioInFillUpBuffers(FskAudioIn audioIn);
static void CALLBACK audioInCallback(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
static void audioInTimerCallback(FskTimeCallBack callback, const FskTime time, void *param);
#elif TARGET_OS_MAC
static FskErr macAudioInNew(FskAudioIn audioIn);
static FskErr macAudioInStart(FskAudioIn audioIn);
static FskErr macAudioInStop(FskAudioIn audioIn);
static FskErr macAudioInDispose(FskAudioIn audioIn);
static void macAudioInBufferHandler(   void *                          inUserData,
									AudioQueueRef                   inAQ,
									AudioQueueBufferRef             inBuffer,
									const AudioTimeStamp *          inStartTime,
									UInt32                          inNumPackets,
									const AudioStreamPacketDescription *inPacketDesc);

static void macAudioInCallback(void *a0, void *a1, void *a2, void *a3);
#elif TARGET_RT_ALSA
static FskErr alsaAudioInNew(FskAudioIn audioIn);
static FskErr alsaAudioInStart(FskAudioIn audioIn);
static FskErr alsaAudioInStop(FskAudioIn audioIn);
static FskErr alsaAudioInDispose(FskAudioIn audioIn);
#endif


#define kAudioInCallbackIntervalMS (125)


FskErr FskAudioInNew(FskAudioIn *audioInOut, UInt32 inputID, FskAudioInCallback proc, void *refCon)
{
	FskErr err;
	FskAudioIn audioIn;

	err = FskMemPtrNewClear(sizeof(FskAudioInRecord), (FskMemPtr *)&audioIn);
	BAIL_IF_ERR(err);

	audioIn->callback = proc;
	audioIn->callbackRefCon = refCon;

	audioIn->inSampleRate = 8000;
	audioIn->inFormat = kFskAudioFormatPCM16BitLittleEndian;
	audioIn->inNumChannels = 1;

    audioIn->useCount = 1;

#if TARGET_OS_ANDROID
	err = gAndroidCallbacks->audioInNewCB(audioIn);
#elif TARGET_OS_MAC
	err = macAudioInNew(audioIn);
#elif TARGET_RT_ALSA
	err = alsaAudioInNew(audioIn);
#endif

	FskTimeCallbackNew(&audioIn->timerCallback);

	FskListMutexNew(&audioIn->recordedQueue, "audioIn Recorded");

bail:
	if (kFskErrNone != err) {
		FskAudioInDispose(audioIn);
		audioIn = NULL;
	}

	*audioInOut = audioIn;

	return err;
}

FskErr FskAudioInDispose(FskAudioIn audioIn)
{
	if (NULL != audioIn) {
        audioIn->disposing = true;

		FskAudioInStop(audioIn);
        
        audioIn->useCount -= 1;
        if (audioIn->useCount > 0)
            return kFskErrNone;
        
		FskTimeCallbackDispose(audioIn->timerCallback);
		FskListMutexDispose(audioIn->recordedQueue);

#if TARGET_OS_ANDROID
		gAndroidCallbacks->audioInDisposeCB(audioIn);
#elif TARGET_OS_MAC
		macAudioInDispose(audioIn);
#elif TARGET_RT_ALSA
        alsaAudioInDispose(audioIn);
#endif
		
		FskMemPtrDispose(audioIn);
	}

	return kFskErrNone;
}

FskErr FskAudioInSetFormat(FskAudioIn audioIn, UInt32 format, UInt16 numChannels, double sampleRate,
					unsigned char *formatInfo, UInt32 formatInfoSize)
{
	int changed = 0;

	if ((UInt32)sampleRate != audioIn->inSampleRate) {
		audioIn->inSampleRate = (UInt32)sampleRate;
		changed++;
	}
	if (format != audioIn->inFormat) {
		audioIn->inFormat = format;
		changed++;
	}
	if (numChannels != audioIn->inNumChannels) {
		audioIn->inNumChannels = numChannels;
		changed++;
	}

	if (changed) {
#if TARGET_OS_ANDROID
		gAndroidCallbacks->audioInSetFormatCB(audioIn);
#endif
	}

	return kFskErrNone;
}

FskErr FskAudioInGetFormat(FskAudioIn audioIn, UInt32 *format, UInt16 *numChannels, double *sampleRate,
					unsigned char **formatInfo, UInt32 *formatInfoSize)
{
#if TARGET_OS_ANDROID
	gAndroidCallbacks->audioInGetFormatCB(audioIn);
#endif

	*sampleRate = audioIn->inSampleRate;
	*format = audioIn->inFormat;
	*numChannels = audioIn->inNumChannels;

	*formatInfo = NULL;
	*formatInfoSize = 0;

	return kFskErrNone;
}

FskErr FskAudioInStart(FskAudioIn audioIn)
{
	FskErr err = kFskErrNone;

	audioIn->recording = true;

#if TARGET_OS_WIN32
    {
	WAVEFORMATEX waveFormat;
	MMRESULT mmErr;

	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nChannels = audioIn->inNumChannels;
	waveFormat.nSamplesPerSec = audioIn->inSampleRate;
	waveFormat.nBlockAlign = 2 * audioIn->inNumChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.wBitsPerSample = 16;
	waveFormat.cbSize = 0;
	mmErr = waveInOpen(&audioIn->waveIn, WAVE_MAPPER, &waveFormat, (DWORD_PTR)audioInCallback, (DWORD_PTR)audioIn, CALLBACK_FUNCTION);
	if (0 != mmErr) {
		// WAVERR_BADFORMAT == 32
		BAIL(kFskErrOperationFailed);
	}

	FskTimeCallbackScheduleFuture(audioIn->timerCallback, 0, kAudioInCallbackIntervalMS, audioInTimerCallback, audioIn);

	audioInFillUpBuffers(audioIn);

	if (0 != waveInStart(audioIn->waveIn)) {
		BAIL(kFskErrOperationFailed);
	}
    }
#elif TARGET_OS_ANDROID
	gAndroidCallbacks->audioInStartCB(audioIn);
#elif TARGET_OS_MAC
	macAudioInStart(audioIn);
#elif TARGET_RT_ALSA
    alsaAudioInStart(audioIn);
#endif
	
	FskUtilsEnergySaverUpdate(kFskUtilsEnergySaverDisableSleep, kFskUtilsEnergySaverDisableSleep);


#if TARGET_OS_WIN32
bail:
#endif
	return err;
}

FskErr FskAudioInStop(FskAudioIn audioIn)
{
	if (true == audioIn->recording) {
		audioIn->recording = false;
#if TARGET_OS_WIN32
		waveInStop(audioIn->waveIn);
		waveInReset(audioIn->waveIn);
#elif TARGET_OS_ANDROID
		gAndroidCallbacks->audioInStopCB(audioIn);
#elif TARGET_OS_MAC
		macAudioInStop(audioIn);
#elif TARGET_RT_ALSA
        alsaAudioInStop(audioIn);
#endif
		FskUtilsEnergySaverUpdate(kFskUtilsEnergySaverDisableSleep, 0);
	}

    FskTimeCallbackRemove(audioIn->timerCallback);

#if TARGET_OS_WIN32
	while (true) {
		AudioInQueue block = FskListMutexRemoveFirst(audioIn->recordedQueue);
		if (!block) break;

		waveInUnprepareHeader(audioIn->waveIn, &block->waveHeader, sizeof(block->waveHeader));
		FskMemPtrDispose(block);
	}

	if (NULL != audioIn->waveIn) {
		waveInClose(audioIn->waveIn);
		audioIn->waveIn = NULL;
	}
#endif

	return kFskErrNone;
}

void audioInUse(FskAudioIn audioIn)
{
    audioIn->useCount += 1;
}

void audioInUnuse(FskAudioIn audioIn)
{
    audioIn->useCount -= 1;
    if (audioIn->useCount > 0)
        return;

    FskAudioInDispose(audioIn);
}

#if TARGET_OS_WIN32
void audioInFillUpBuffers(FskAudioIn audioIn)
{
	UInt32 sampleSize = audioIn->inNumChannels * 2;
	UInt32 sampleCount = (audioIn->inSampleRate + 1) / 8;

	while (audioIn->timeQueued < (audioIn->inSampleRate * 2)) {
		AudioInQueue block;

		if (kFskErrNone != FskMemPtrNew(sampleCount * sampleSize + sizeof(AudioInQueueRecord), (FskMemPtr *)&block))
			break;

		block->next = NULL;
		FskMemSet(&block->waveHeader, 0, sizeof(block->waveHeader));
		block->maxSamples = sampleCount;

		block->waveHeader.lpData = block->data;
		block->waveHeader.dwBufferLength = sampleCount * sampleSize;
		block->waveHeader.dwFlags = 0;
		block->waveHeader.dwUser = (DWORD_PTR)block;
		waveInPrepareHeader(audioIn->waveIn, &block->waveHeader, sizeof(block->waveHeader));

		waveInAddBuffer(audioIn->waveIn, &block->waveHeader, sizeof(block->waveHeader));

		audioIn->timeQueued += sampleCount;
	}
}

void CALLBACK audioInCallback(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	FskAudioIn audioIn = (FskAudioIn)dwInstance;

	if (WIM_DATA == uMsg) {
		WAVEHDR *waveHdr = (WAVEHDR *)dwParam1;
		AudioInQueue block = (AudioInQueue)waveHdr->dwUser;
		FskListMutexAppend(audioIn->recordedQueue, block);

		audioIn->timeQueued -= block->maxSamples;
		if (audioIn->recording)
			audioInFillUpBuffers(audioIn);
	}
}

void audioInTimerCallback(FskTimeCallBack callback, const FskTime time, void *param)
{
	FskAudioIn audioIn = param;
	FskTimeRecord when = *time;

	while (true) {
		AudioInQueue block = FskListMutexRemoveFirst(audioIn->recordedQueue);
		if (!block) break;

		waveInUnprepareHeader(audioIn->waveIn, &block->waveHeader, sizeof(block->waveHeader));

		if (audioIn->callback)
			(audioIn->callback)(audioIn, audioIn->callbackRefCon, block->data, block->waveHeader.dwBytesRecorded);

		FskMemPtrDispose(block);
	}

	if (audioIn->recording)
		audioInFillUpBuffers(audioIn);

	FskTimeAddMS(&when, kAudioInCallbackIntervalMS);
	FskTimeCallbackSet(audioIn->timerCallback, &when, audioInTimerCallback, audioIn);
}

#elif TARGET_OS_MAC

FskErr macAudioInNew(FskAudioIn audioIn)
{
	OSStatus err;
#if TARGET_OS_IPHONE
	Float64 sampleRate;
    UInt32 channelsPerFrame;
    UInt32 propertySize;

#if TARGET_IPHONE_SIMULATOR
    sampleRate = 44100.0;
#else
    if (!FskCocoaAudioSessionSetupRecording(true)) return kFskErrOperationFailed;

    propertySize = sizeof(sampleRate);
    err = AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareSampleRate, &propertySize, &sampleRate);
    if (err) return kFskErrOperationFailed;

	propertySize = sizeof(channelsPerFrame);
	err = AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareInputNumberChannels, &propertySize, &channelsPerFrame);
    if (err) return kFskErrOperationFailed;

    audioIn->inNumChannels = channelsPerFrame;
#endif
#else
	AudioDeviceID deviceID = 0;
	Float64 sampleRate;
	
	// get the default input device
	AudioObjectPropertyAddress addr;
	UInt32 size;
	addr.mSelector = kAudioHardwarePropertyDefaultInputDevice;
	addr.mScope = kAudioObjectPropertyScopeGlobal;
	addr.mElement = 0;
	size = sizeof(AudioDeviceID);
	err = AudioHardwareServiceGetPropertyData(kAudioObjectSystemObject, &addr, 0, NULL, &size, &deviceID);
	if (err) return kFskErrOperationFailed;

	// get its sample rate
	addr.mSelector = kAudioDevicePropertyNominalSampleRate;
	addr.mScope = kAudioObjectPropertyScopeGlobal;
	addr.mElement = 0;
	size = sizeof(sampleRate);
	err = AudioHardwareServiceGetPropertyData(deviceID, &addr, 0, NULL, &size, &sampleRate);
	if (err) return kFskErrOperationFailed;
#endif
	
	audioIn->inSampleRate = (UInt32)sampleRate;

	audioIn->thread = FskThreadGetCurrent();

    FskMutexNew(&audioIn->mutex, "audioIn");
	
	return kFskErrNone;
}

FskErr macAudioInDispose(FskAudioIn audioIn)
{
#if TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
    if (!FskCocoaAudioSessionSetupRecording(false)) return kFskErrOperationFailed;
#endif

    if (audioIn) {
        FskMutexDispose(audioIn->mutex);
    }

	return kFskErrNone;
}

FskErr macAudioInStart(FskAudioIn audioIn)
{
	OSStatus err;
    AudioStreamBasicDescription recordFormat = {0};
	UInt32 bufferByteSize, i;
	
	recordFormat.mSampleRate = audioIn->inSampleRate;
	recordFormat.mChannelsPerFrame = audioIn->inNumChannels;
	recordFormat.mFormatID = kAudioFormatLinearPCM;
	recordFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
	recordFormat.mBitsPerChannel = 16;
	recordFormat.mBytesPerPacket = recordFormat.mBytesPerFrame = (recordFormat.mBitsPerChannel / 8) * recordFormat.mChannelsPerFrame;
	recordFormat.mFramesPerPacket = 1;
	recordFormat.mReserved = 0;
	
	err = AudioQueueNewInput(&recordFormat,
							 macAudioInBufferHandler,
							 audioIn /* userData */,
							 NULL /* run loop */, NULL /* run loop mode */,
							 0 /* flags */, &audioIn->queue);
	if (err) return kFskErrOperationFailed;

	
	bufferByteSize = (audioIn->inSampleRate * audioIn->inNumChannels * 2) / kFskAudioMacBufferCount;
	for (i = 0; i < kFskAudioMacBufferCount; ++i) {
		AudioQueueBufferRef buffer;
		err = AudioQueueAllocateBuffer(audioIn->queue, bufferByteSize, &buffer);
		if (err) break;
		err = AudioQueueEnqueueBuffer(audioIn->queue, buffer, 0, NULL);
		if (err) break;
	}
	if (err) return kFskErrOperationFailed;
	
	audioIn->active = true;
	audioIn->bufferCount = 0;

	err = AudioQueueStart(audioIn->queue, NULL);
	if (err) return kFskErrOperationFailed;	
	
	return kFskErrNone;
}

FskErr macAudioInStop(FskAudioIn audioIn)
{
	audioIn->active = false;
	if (audioIn->queue) {
		AudioQueueStop(audioIn->queue, true);
        audioInUse(audioIn);
        macAudioInCallback(audioIn, NULL, NULL, NULL);
		AudioQueueDispose(audioIn->queue, true);
		audioIn->queue = NULL;
	}

	return kFskErrNone;
}

void macAudioInBufferHandler(   void *                         inUserData,
								 AudioQueueRef                   inAQ,
								 AudioQueueBufferRef             inBuffer,
								 const AudioTimeStamp *          inStartTime,
								 UInt32                          inNumPackets,
								 const AudioStreamPacketDescription *inPacketDesc)
{
    FskAudioIn audioIn = (FskAudioIn)inUserData;

    FskMutexAcquire(audioIn->mutex);

	if ((audioIn->bufferCount < kFskAudioMacBufferCount) && audioIn->recording)
		audioIn->buffers[audioIn->bufferCount++] = inBuffer;
	else
		AudioQueueEnqueueBuffer(audioIn->queue, inBuffer, 0, NULL);

    if (1 == audioIn->bufferCount) {
        audioInUse(audioIn);
        FskThreadPostCallback(audioIn->thread, macAudioInCallback, audioIn, NULL, NULL, NULL);
    }

    FskMutexRelease(audioIn->mutex);
}

void macAudioInCallback(void *a0, void *a1, void *a2, void *a3)
{
	FskAudioIn audioIn = a0;

    FskMutexAcquire(audioIn->mutex);
    if (false == audioIn->disposing) {
        UInt32 i;

        for (i = 0; i < audioIn->bufferCount; i++) {
            if (audioIn->callback)
                (audioIn->callback)(audioIn, audioIn->callbackRefCon, audioIn->buffers[i]->mAudioData, audioIn->buffers[i]->mAudioDataByteSize);
            AudioQueueEnqueueBuffer(audioIn->queue, audioIn->buffers[i], 0, NULL);
        }
        audioIn->bufferCount = 0;
    }
    FskMutexRelease(audioIn->mutex);

    audioInUnuse(audioIn);
}

#elif TARGET_RT_ALSA

#include <alsa/asoundlib.h>

#define AUDIODEV_NAME   "default"

enum {
	REC_STOP = 0,
    REC_INIT,
	REC_ONGOING,
	REC_EXITING,
    REC_EXITED
    
};

static void alsaAudioInThread(void *refcon);
static void alsaDataReady(void *a0, void *a1, void *a2, void *a3);

FskErr alsaAudioInNew(FskAudioIn audioIn)
{
	audioIn->recStatus = REC_STOP;

    audioIn->clientThread = FskThreadGetCurrent();

    return kFskErrNone;
}

FskErr alsaAudioInDispose(FskAudioIn audioIn)
{
    return kFskErrNone;
}

FskErr alsaAudioInStart(FskAudioIn audioIn)
{
    FskErr err;

    audioIn->recStatus = REC_INIT;

	err = FskThreadCreate(&audioIn->alsaThread, alsaAudioInThread, kFskThreadFlagsHighPriority | kFskThreadFlagsWaitForInit, audioIn, "alsa recording");
    if (err)
		audioIn->recStatus = REC_STOP;

    return err;
}

FskErr alsaAudioInStop(FskAudioIn audioIn)
{
    audioIn->recStatus = REC_EXITING;

    while (REC_EXITED != audioIn->recStatus)
        FskDelay(1);

    return kFskErrNone;
}

void alsaAudioInThread(void *refcon)
{
    FskAudioIn audioIn = (FskAudioIn)refcon;
    FskErr err;
    int alsaErr = 0;
    snd_pcm_t *pcm = NULL;
    snd_pcm_hw_params_t * hw_params;
    unsigned int val, val2;
    int dir = 0;
    snd_pcm_uframes_t frames;
    FskMemPtr buffer = NULL;

    alsaErr = snd_pcm_open(&pcm, AUDIODEV_NAME, SND_PCM_STREAM_CAPTURE, 0);

    alsaErr = snd_pcm_hw_params_malloc(&hw_params);
    alsaErr = snd_pcm_hw_params_any(pcm, hw_params);

    alsaErr = snd_pcm_hw_params_set_rate_resample(pcm, hw_params, 1);
    alsaErr = snd_pcm_hw_params_set_access(pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    alsaErr = snd_pcm_hw_params_set_format(pcm, hw_params, SND_PCM_FORMAT_S16_LE);
    alsaErr = snd_pcm_hw_params_set_channels(pcm, hw_params, audioIn->inNumChannels);

    val = audioIn->inSampleRate;
    dir = 0;
    alsaErr = snd_pcm_hw_params_set_rate_near(pcm, hw_params, &val, &dir);

    frames = audioIn->inSampleRate / 32;
    dir = 0;
    alsaErr = snd_pcm_hw_params_set_period_size_near(pcm, hw_params, &frames, &dir);

    alsaErr = snd_pcm_hw_params(pcm, hw_params);
    alsaErr = snd_pcm_hw_params_get_rate_numden(hw_params, &val, &val2);

//    audioIn->sampleRateReal = val / val2;
    dir = 0;
    alsaErr = snd_pcm_hw_params_get_period_size(hw_params, &frames, &dir);

	audioIn->recStatus = REC_ONGOING;

    FskThreadInitializationComplete(FskThreadGetCurrent());

	while (REC_ONGOING == audioIn->recStatus) {
        int rc;

        err = FskMemPtrNew(frames * 2 * audioIn->inNumChannels, &buffer);
        BAIL_IF_ERR(err);

        rc = snd_pcm_readi(pcm, buffer, frames);
        if (-EPIPE == rc) {
            /* EPIPE means overrun */
            FskMemPtrDisposeAt(&buffer);
            snd_pcm_prepare(pcm);
        }
        else if (rc < 0) {
			audioIn->recStatus = REC_EXITING;
			break;
        }
        else if (rc > 0) {
            if (audioIn->callback && !audioIn->disposing) {
                audioInUse(audioIn);
                err = FskThreadPostCallback(audioIn->clientThread, alsaDataReady, audioIn, buffer, (void *)rc, (void *)NULL);
                if (err)
                    break;
                buffer = NULL;
            }
            else
                FskMemPtrDisposeAt(&buffer);
        }
    }

bail:
    if (pcm) {
        snd_pcm_drain(pcm);
        snd_pcm_close(pcm);
    }

    FskMemPtrDispose(buffer);

    audioIn->recStatus = REC_EXITED;
}

void alsaDataReady(void *a0, void *a1, void *a2, void *a3)
{
    FskAudioIn audioIn = a0;
    void *buffer = a1;

    if (audioIn->recording) {
        UInt32 frames = (UInt32)a2;

        (audioIn->callback)(audioIn, audioIn->callbackRefCon, buffer, frames * 2 * audioIn->inNumChannels);
    }
    
    FskMemPtrDispose(buffer);
    audioInUnuse(audioIn);
}

#endif
