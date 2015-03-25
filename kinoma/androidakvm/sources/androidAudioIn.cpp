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
#include "FskInstrumentation.h"

#if (ANDROID_VERSION == 4)	// Gingerbread

#include <SLES/OpenSLES.h>
#include "SLES/OpenSLES_Android.h"

#define USE_SLES	1

#else
#include <media/mediarecorder.h>
#include <media/AudioRecord.h>
#include <media/AudioSystem.h>
#endif

#include "FskHardware.h"

#include "androidAudio.h"
#include "FskAudio.h"
#include "FskMemory.h"
#include "FskThread.h"

FskInstrumentedSimpleType(AudioInNative, audioinnative);


extern "C" {

	static void audioInTimerCallback(FskTimeCallBack callback, const FskTime time, void *param);
	void androidFskAudioInTerminate(void);
	void androidFskAudioInInitialize(void);
}

#if USE_SLES
extern void CheckErr(const char *name, SLresult res);
extern SLObjectItf engineObject;
#if SUPPORT_INSTRUMENTATION
#define BAIL_FAIL(x, s)	do { if (SL_RESULT_SUCCESS != x) { FskInstrumentedTypePrintfForLevel(FskInstrumentationGetErrorInstrumentation(), kFskInstrumentationLevelDebug, "BAIL_FAIL %s (%d) in function %s in file %s at line %d", (s), (result), __FUNCTION__, __FILE__, __LINE__); goto bail; } } while (0)
#else
#define BAIL_FAIL(x, s) do { if (SL_RESULT_SUCCESS != x) goto bail; } while (0)
#endif
#endif

#define kAudioInCallbackIntervalMS (125)

#if (ANDROID_VERSION == 4)	// Gingerbread

typedef struct gingerbreadInRecord {
	FskAudioIn	parent;
	SLEngineItf	engineItf;

	SLObjectItf	recorderObject;
	SLRecordItf	recorderRecord;
	SLAndroidSimpleBufferQueueItf recorderBufferQueue;

	UInt32			recorderSize;
	SLmilliHertz	recorderSR;

	int				nextBuffer;
	int				bufferSize;
	char 			*buffer[2];
} gingerbreadInRecord, *gingerbreadIn;


void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
	SLresult result;
	gingerbreadIn gin = (gingerbreadIn)context;
	AudioInQueue block;

//	FskAudioInNativePrintfDebug("bqRecorderCallback - nextBuf is %d\n", gin->nextBuffer);

	// grab old buffer
	
	BAIL_IF_ERR(FskMemPtrNew(sizeof(AudioInQueueRecord) + gin->bufferSize, (FskMemPtr*)&block));
	block->next = NULL;
	block->size = gin->bufferSize;
	FskMemCopy(&block->data, gin->buffer[gin->nextBuffer], gin->bufferSize);
	FskListMutexAppend(gin->parent->recordedQueue, block);

	result = (*gin->recorderBufferQueue)->Enqueue(gin->recorderBufferQueue, gin->buffer[gin->nextBuffer], gin->bufferSize);
	CheckErr("bqRecorderCallback: enqueue buffer", result);

	gin->nextBuffer = gin->nextBuffer ? 0 : 1;

bail:
	;
}

FskErr setupAudioIn(FskAudioIn audioIn) {
	int	audioFormat = 0;		// default == PCM
	uint32_t inFlags = 0;
	SLresult result;
	FskErr err = kFskErrNone;
    SLDataFormat_PCM        pcm;
	SLDataSource audioSrc;
	SLDataSink audioSnk;
	SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
	SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
	const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
	const SLboolean req[1] = {SL_BOOLEAN_TRUE};
	int sampleSize = 2; //kFskAudioFormatPCM16BitLittleEndian
	gingerbreadIn gin;

	gin = (gingerbreadIn)audioIn->nativeIn;

    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, (void*)&gin->engineItf);
	BAIL_FAIL(result, "GetInterface Engine");

	audioSrc.pLocator = &loc_dev;
	audioSrc.pFormat = NULL;

	// configure audio sink
	pcm.formatType = SL_DATAFORMAT_PCM;
	pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
	pcm.numChannels = audioIn->inNumChannels;
	pcm.samplesPerSec = audioIn->inSampleRate * 1000; // was SL_SAMPLINGRATE_16
	pcm.containerSize = sampleSize * 8;
	pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

	audioSnk.pLocator = &loc_bq;
	audioSnk.pFormat = &pcm;

	// create audio recorder
	// (requires the RECORD_AUDIO permission)
	result = (*gin->engineItf)->CreateAudioRecorder(gin->engineItf, &gin->recorderObject, &audioSrc, &audioSnk, 1, id, req);
	BAIL_FAIL(result, "CreateAudioRecorder");

	// realize the audio recorder
	result = (*gin->recorderObject)->Realize(gin->recorderObject, SL_BOOLEAN_FALSE);
	BAIL_FAIL(result, "CreateAudioRecorder - realize");

	// get the record interface
	result = (*gin->recorderObject)->GetInterface(gin->recorderObject, SL_IID_RECORD, &gin->recorderRecord);
	BAIL_FAIL(result, "CreateAudioRecorder - getInterface - record");

    // get the buffer queue interface
	result = (*gin->recorderObject)->GetInterface(gin->recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &gin->recorderBufferQueue);
	BAIL_FAIL(result, "CreateAudioRecorder - getInterface - bufferqueue");

    // register callback on the buffer queue
    result = (*gin->recorderBufferQueue)->RegisterCallback(gin->recorderBufferQueue, bqRecorderCallback, gin);
	BAIL_FAIL(result, "CreateAudioRecorder - register callback");

bail:
	return kFskErrNone;
}


FskErr androidAudioInNew(FskAudioIn audioIn) {
	FskErr err = kFskErrNone;
	gingerbreadIn gin;
	int sampleSize = 2; //kFskAudioFormatPCM16BitLittleEndian

	err = FskMemPtrNewClear(sizeof(gingerbreadInRecord), &gin);
	BAIL_IF_ERR(err);
	gin->parent = audioIn;
	audioIn->nativeIn = gin;

	gin->bufferSize = (audioIn->inSampleRate * audioIn->inNumChannels * sampleSize) / 8;
	FskAudioInNativePrintfDebug("AudioInNew - rate: %d, channels: %d, bufferSize: %d\n", audioIn->inSampleRate, audioIn->inNumChannels, gin->bufferSize);

	FskTimeCallbackNew(&audioIn->timerCallback);

	err = FskListMutexNew(&audioIn->recordedQueue, "audioIn Recorded");
	BAIL_IF_ERR(err);

	err = FskMemPtrNew(gin->bufferSize, (FskMemPtr*)&gin->buffer[0]);
	BAIL_IF_ERR(err);
	err = FskMemPtrNew(gin->bufferSize, (FskMemPtr*)&gin->buffer[1]);
	BAIL_IF_ERR(err);

bail:
	if (err)
		androidAudioInDispose(audioIn);
		
	return err;
}


#else						// Froyo and below
android::AudioRecord *gAudioIn = NULL;

void haveMoreCallback(int event, void *user, void *info) {
	FskAudioIn audioIn = (FskAudioIn)user;
	android::AudioRecord::Buffer *b;

	if (NULL == gAudioIn || !audioIn->recording)
		return;

	switch (event) {
		case android::AudioRecord::EVENT_MORE_DATA: {
			b = static_cast<android::AudioRecord::Buffer *>(info);

			if (b->size == 0) return;

			AudioInQueue block;
			BAIL_IF_ERR(FskMemPtrNew(sizeof(AudioInQueueRecord) + b->size, (FskMemPtr*)&block));
			block->next = NULL;
			block->size = b->size;
			FskMemCopy(&block->data, b->i8, b->size);
			FskListMutexAppend(audioIn->recordedQueue, block);
			}
			break;
	}
bail:
	return;
}


FskErr setupAudioIn(FskAudioIn audioIn) {
	int	audioFormat = 0;		// default == PCM
	uint32_t inFlags = 0;

	inFlags |= android::AudioSystem::AGC_ENABLE;
	if (NULL == gAudioIn) {
		android::status_t checkVal;

		gAudioIn = new android::AudioRecord(
#if (ANDROID_VERSION == 2 || ANDROID_VERSION == 3)
			android::AUDIO_SOURCE_DEFAULT,
#else
			android::AudioRecord::DEFAULT_INPUT,
#endif
			audioIn->inSampleRate,
			audioFormat,
#if (ANDROID_VERSION == 2 || ANDROID_VERSION == 3)
			android::AudioSystem::CHANNEL_IN_MONO,
#else
			audioIn->inNumChannels,
#endif
			0,
			0,					// inFlags,
			haveMoreCallback,		// callback function
			audioIn,				// refcon
			0);						// notification frmaes

		checkVal = gAudioIn->initCheck();
		if (android::NO_ERROR != checkVal) {
			FskAudioInNativePrintfDebug("audio in initCheck() fails with %d\n", checkVal);
			// -- MDK - need to fail properly here
		}
	}
	audioIn->nativeIn = gAudioIn;

	return kFskErrNone;
}


FskErr androidAudioInNew(FskAudioIn audioIn) {
	int	bufferSize;

	bufferSize = audioIn->inSampleRate * audioIn->inNumChannels * 2;
	FskAudioInNativePrintfDebug("androidAudioInNew bufferSize = %d, sample Rate: %d\n", bufferSize, audioIn->inSampleRate );

	FskTimeCallbackNew(&audioIn->timerCallback);

	return FskListMutexNew(&audioIn->recordedQueue, "audioIn Recorded");
}

#endif		// Froyo

void audioInTimerCallback(FskTimeCallBack callback, const FskTime time, void *param) {
	FskAudioIn audioIn = (FskAudioIn)param;
	FskTimeRecord when = *time;

	while (true) {
		AudioInQueue block = (AudioInQueue)FskListMutexRemoveFirst(audioIn->recordedQueue);
		if (!block) break;

		if (audioIn->callback) {
			(audioIn->callback)(audioIn, audioIn->callbackRefCon, block->data, block->size);
		}

		FskMemPtrDispose(block);
	}

	FskTimeAddMS(&when, kAudioInCallbackIntervalMS);
	FskTimeCallbackSet(audioIn->timerCallback, &when, audioInTimerCallback, audioIn);
}


FskErr androidAudioInDispose(FskAudioIn audioIn) {
	FskAudioInNativePrintfDebug("androidAudioInDispose\n");

#if (ANDROID_VERSION == 4)	// Gingerbread
	{
	gingerbreadIn gin = (gingerbreadIn)audioIn->nativeIn;
	if (gin) {
		if (gin->recorderObject) {
			(*gin->recorderObject)->Destroy(gin->recorderObject);
			gin->recorderObject = NULL;
			gin->recorderRecord = NULL;
			gin->recorderBufferQueue = NULL;
		}
		if (gin->buffer[0])
			FskMemPtrDispose(gin->buffer[0]);
		if (gin->buffer[0])
			FskMemPtrDispose(gin->buffer[1]);
		FskMemPtrDispose(gin);
		audioIn->nativeIn = NULL;
	}
	}
#else
	if (gAudioIn) {
		delete gAudioIn;
		gAudioIn = NULL;
	}
#endif
	return kFskErrNone;
}


FskErr androidAudioInSetFormat(FskAudioIn audioIn) {
//FskAudioInNativePrintfDebug("androidAudioInSetFormat\n");
	return kFskErrNone;
}


FskErr androidAudioInGetFormat(FskAudioIn audioIn) {
//FskAudioInNativePrintfDebug("androidAudioInGetFormat\n");
	return kFskErrNone;
}


FskErr androidAudioInStart(FskAudioIn audioIn) {
	FskErr err = kFskErrNone;

#if (ANDROID_VERSION == 4)	// Gingerbread
	SLresult result;
	gingerbreadIn gin = (gingerbreadIn)audioIn->nativeIn;

	if (!gin || !gin->recorderObject)
		BAIL_IF_ERR(err = setupAudioIn(audioIn));
#else
	if (!audioIn->nativeIn)
		BAIL_IF_ERR(err = setupAudioIn(audioIn));
#endif
		
	if (audioIn->nativeIn) {
#if (ANDROID_VERSION == 4)	// Gingerbread
		gin = (gingerbreadIn)audioIn->nativeIn;

		// in case already recording, stop recording and clear buffer queue
		result = (*gin->recorderRecord)->SetRecordState(gin->recorderRecord, SL_RECORDSTATE_STOPPED);
		BAIL_FAIL(result, "SetRecordState - record state stop");

		result = (*gin->recorderBufferQueue)->Clear(gin->recorderBufferQueue);
		BAIL_FAIL(result, "SetRecordState - clear buffer queue");

		// enqueue an empty buffer to be filled by the recorder
		// (for streaming recording, we would enqueue at least 2 empty buffers to start things off)
		result = (*gin->recorderBufferQueue)->Enqueue(gin->recorderBufferQueue, gin->buffer[0], gin->bufferSize);
		CheckErr("start Record - enqueue buffer 0", result);

		result = (*gin->recorderBufferQueue)->Enqueue(gin->recorderBufferQueue, gin->buffer[1], gin->bufferSize);
		CheckErr("start Record - enqueue buffer 1", result);

		gin->nextBuffer = 0;

		result = (*gin->recorderRecord)->SetRecordState(gin->recorderRecord, SL_RECORDSTATE_RECORDING);
		CheckErr("SetRecordState - record state start", result);


#else
		android::status_t status;

		status = gAudioIn->start();
//		FskAudioInNativePrintfDebug("gAudioIn start returns %d\n", status);

#endif

		FskTimeCallbackScheduleFuture(audioIn->timerCallback, 0, kAudioInCallbackIntervalMS, audioInTimerCallback, audioIn);

	}

	audioIn->recording = true;

bail:

	return err;
}


FskErr androidAudioInStop(FskAudioIn audioIn) {

	if (NULL == audioIn->nativeIn) {
		return kFskErrNone;
	}

	if (true) { 		//  == audioIn->recording) {
#if (ANDROID_VERSION == 4)	// Gingerbread
		SLresult result;
		gingerbreadIn gin = (gingerbreadIn)audioIn->nativeIn;

		// in case already recording, stop recording and clear buffer queue
		result = (*gin->recorderRecord)->SetRecordState(gin->recorderRecord, SL_RECORDSTATE_STOPPED);
		CheckErr("SetRecordState - record state stop", result);

		if (gin->recorderObject) {
			(*gin->recorderObject)->Destroy(gin->recorderObject);
			gin->recorderObject = NULL;
			gin->recorderRecord = NULL;
			gin->recorderBufferQueue = NULL;
		}
#else
		android::status_t status;

		audioIn->recording = false;
		status = gAudioIn->stop();
		if (gAudioIn) {
			delete gAudioIn;
			gAudioIn = NULL;
		}
#endif
	}

	if (NULL != audioIn->timerCallback)
		FskTimeCallbackRemove(audioIn->timerCallback);

	while (true) {
		AudioInQueue block = (AudioInQueue)FskListMutexRemoveFirst(audioIn->recordedQueue);
		if (!block) break;
		FskMemPtrDispose(block);
	}

	return kFskErrNone;
}

void androidFskAudioInInitialize() {
	gAndroidCallbacks->audioInNewCB = (FskAudioInNewFn)androidAudioInNew;
    gAndroidCallbacks->audioInDisposeCB = (FskAudioInDisposeFn)androidAudioInDispose;
    gAndroidCallbacks->audioInGetFormatCB = (FskAudioInGetFormatFn)androidAudioInGetFormat;
    gAndroidCallbacks->audioInSetFormatCB = (FskAudioInSetFormatFn)androidAudioInSetFormat;
    gAndroidCallbacks->audioInStartCB = (FskAudioInStartFn)androidAudioInStart;
    gAndroidCallbacks->audioInStopCB = (FskAudioInStopFn)androidAudioInStop;
}

void androidFskAudioInTerminate() {
}


