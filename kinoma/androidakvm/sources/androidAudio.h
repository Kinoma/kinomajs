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
#ifndef __ANDROIDAUDIO_H__
#define __ANDROIDAUDIO_H__

#define __FSKAUDIO_PRIV__

#include "Fsk.h"
#include "FskAudio.h"

extern "C" {

void androidAudioOutThread(void *arg);
FskErr audioOutEnqueue(FskAudioOut audioOut, void *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, UInt32 *frameSizes, FskAudioOutBlock *blockOut);

void flushAndRefill(void *arg0, void *arg1, void* arg2, void *arg3);
int linuxAudioOutPCM(FskAudioOut audioOut);

FskErr androidAudioOutNew(FskAudioOut *audioOutOut, UInt32 outputID, UInt32 format);
FskErr androidAudioOutDispose(FskAudioOut audioOut);
FskErr androidAudioOutIsValid(FskAudioOut audioOut, Boolean *isValid);
FskErr androidAudioOutGetFormat(FskAudioOut audioOut, UInt32 *format, UInt16 *numChannels, double *sampleRate);
FskErr androidAudioOutSetFormat(FskAudioOut audioOut, UInt32 format, UInt16 numChannels, double sampleRate, unsigned char *formatInfo, UInt32 formatInfoSize);
FskErr androidAudioOutSetOutputBufferSize(FskAudioOut audioOut, UInt32 chunkSize, UInt32 bufferedSamplesTarget);
FskErr androidAudioOutGetVolume(FskAudioOut audioOut, UInt16 *left, UInt16 *right);
FskErr androidAudioOutSetVolume(FskAudioOut audioOut, UInt16 left, UInt16 right);
FskErr androidAudioOutSetDoneCallback(FskAudioOut audioOut, FskAudioOutDoneCallback cb, void *refCon);
FskErr androidAudioOutSetMoreCallback(FskAudioOut audioOut, FskAudioOutMoreCallback cb, void *refCon);
FskErr androidAudioOutStart(FskAudioOut audioOut, FskSampleTime atSample);
FskErr androidAudioOutStop(FskAudioOut audioOut);
FskErr androidAudioOutEnqueue(FskAudioOut audioOut, void *data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, UInt32 *frameSizes);
FskErr androidAudioOutGetSamplePosition(FskAudioOut audioOut, FskSampleTime *position);
FskErr androidAudioOutGetSamplesQueued(FskAudioOut audioOut, UInt32 *samplesQueuedOut, UInt32 *targetQueueLengthOut);
FskErr androidAudioOutSingleThreadedClient(FskAudioOut audioOut, Boolean *isSingleThreaded);

void androidFskAudioInitialize(void);
void androidFskAudioTerminate(void);

// audio in

typedef struct {
	void	*next;
	UInt32	size;
	char	data[2];
} AudioInQueueRecord, *AudioInQueue;

struct FskAudioInRecord;

typedef FskErr (*FskAudioInCallback)(struct FskAudioInRecord *audioIn, void *refCon, void *data, UInt32 dataSize);

struct FskAudioInRecord {
    UInt32                      inSampleRate;
    UInt32                      inFormat;
    UInt16                      inNumChannels;

    Boolean                     recording;

    FskAudioInCallback          callback;
    void                        *callbackRefCon;

    UInt32                      timeQueued;
    FskListMutex                recordedQueue;

    FskTimeCallBack             timerCallback;

    void                        *nativeIn;
};

typedef struct FskAudioInRecord FskAudioInRecord;
typedef FskAudioInRecord *FskAudioIn;


FskErr androidAudioInNew(FskAudioIn audioIn);
FskErr androidAudioInDispose(FskAudioIn audioIn);
FskErr androidAudioInSetFormat(FskAudioIn audioIn);
FskErr androidAudioInGetFormat(FskAudioIn audioIn);
FskErr androidAudioInStart(FskAudioIn audioIn);
FskErr androidAudioInStop(FskAudioIn audioIn);

}	/* extern "C" */

#endif /* __ANDROIDAUDIO_H__ */
