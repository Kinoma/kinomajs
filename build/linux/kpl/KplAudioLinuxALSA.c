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
#include "KplAudio.h"
#include "KplSynchronization.h"
#include "KplThread.h"

#include "FskAudio.h"
#include "FskList.h"
#include "FskThread.h"

#include <alsa/asoundlib.h>

#define alsaIfError(X) { (err = (X));\
if (err < 0) goto bail; \
else err = kFskErrNone; }

#define bailIfError(X) { (err = (X));\
if (err != kFskErrNone) goto bail; }

#define kAlsaDeviceName "plug:dmix"

typedef struct KplAudioOutBlockStruct KplAudioOutBlockRecord, *KplAudioOutBlock;

struct KplAudioOutBlockStruct {
	struct KplAudioOutBlockRecord* next;
	const char* data;
	UInt32 size;
	UInt32 offset;
	void* refCon;
	Boolean done;
};

struct KplAudioRecord {
	KplAudioOutBlock buffer;
	KplMutex mutex;
	FskThread thread;

	KplAudioMoreCallback moreCB;
	void* moreRefCon;

	KplAudioDoneCallback doneCB;
	void* doneRefCon;
	
	Boolean playing;
	Boolean silence;
	Boolean ending;
	Boolean stopping;
	UInt32 bufferSize;
	UInt32 bufferMin;
	UInt32 played;

	UInt16 volL;
	UInt16 volR;

// platform
	snd_pcm_uframes_t frames;
	snd_pcm_uframes_t period;
	char* data;
	UInt32 dataSize;
	UInt8 channels;
	SInt16 format;
	unsigned int rate;
};


// KplAudio
#if 0
static void KplAudioSetMasterVolume(long left, long right);
#endif

// KplAudioOutBlock
static FskErr KplAudioOutBlockAdd(KplAudio audio, const char* data, UInt32 size, void *refCon);
static FskErr KplAudioOutBlockFill(KplAudio audio);
static FskErr KplAudioOutBlockGetSize(KplAudio audio, UInt32* queued);
static FskErr KplAudioOutBlockRead(KplAudio audio, char* data, UInt32 size);
static FskErr KplAudioOutBlockReadSilence(KplAudio audio, char* data, UInt32 size);
static FskErr KplAudioOutBlockRemove(KplAudio audio, KplAudioOutBlock buffer);
static FskErr KplAudioOutBlockRemoveAll(KplAudio audio);

// KplAudioThread
static void KplAudioThread(void* refCon);

#if 0
#pragma mark -
#pragma mark KplAudio
#endif
// --------------------------------------------------
// KplAudio
// --------------------------------------------------

FskErr KplAudioDispose(KplAudio audio)
{
	FskErr err = kFskErrNone;

    if (!audio) goto bail;

	if (audio->thread) {
		audio->stopping = true;
		FskThreadJoin(audio->thread);
	}
	KplMutexDispose(audio->mutex);
	audio->mutex = NULL;
	FskMemPtrDisposeAt(&audio->data);
	FskMemPtrDispose(audio);
bail:
	return err;
}

FskErr KplAudioNew(KplAudio *audioOut)
{
	FskErr err = kFskErrNone;
	KplAudio audio = NULL;
	
	bailIfError(FskMemPtrNewClear(sizeof(KplAudioRecord), (FskMemPtr *)&audio));
	bailIfError(KplMutexNew(&audio->mutex));
	audio->format = SND_PCM_FORMAT_S16_LE;
#if TARGET_RT_LITTLE_ENDIAN
	audio->format = SND_PCM_FORMAT_S16_LE;
#else
	audio->format = SND_PCM_FORMAT_S16_BE;
#endif
	audio->volL = 256;
	audio->volR = 256;
	*audioOut = audio;
bail:
	if (err)
		KplAudioDispose(audio);
	return err;
}

FskErr KplAudioSetFormat(KplAudio audio, const char *format, UInt32 channels, double sampleRate, const unsigned char *formatInfo, UInt32 formatInfoSize)
{
	FskErr err = kFskErrNone;

    /* @@ this code should validate that format is an expected value - e.g. x-audio-codec/pcm-16-le on a little-endian system */

	audio->rate = sampleRate;
	audio->channels = channels;
	audio->bufferMin = audio->channels * 2 * audio->rate; // 1 second buffer

	audio->silence = true;

	return err;
}

FskErr KplAudioGetFormat(KplAudio audio, const char **format, UInt32 *channels, double *sampleRate, const unsigned char **formatInfo, UInt32 *formatInfoSize)
{
	if (format) {
		switch (audio->format) {
			case SND_PCM_FORMAT_S16_LE:
				*format = "x-audio-codec/pcm-16-le";
				break;
			case SND_PCM_FORMAT_S16_BE:
				*format = "x-audio-codec/pcm-16-be";
				break;
		}
	}
	if (channels)
		*channels = audio->channels;
	if (sampleRate)
		*sampleRate = audio->rate;

	return kFskErrNone;
}

FskErr KplAudioWrite(KplAudio audio, const char* data, UInt32 dataSize, void *dataRefCon, UInt32 frameCount, const UInt32 *frameSizes)
{
	FskErr err = kFskErrNone;
	
	bailIfError(KplAudioOutBlockAdd(audio, data, dataSize, dataRefCon));
bail:
	return err;
}

FskErr KplAudioStart(KplAudio audio)
{
	FskErr err = kFskErrNone;
	
	audio->playing = true;
	audio->played = 0;
	
	bailIfError(FskThreadCreate(&audio->thread, KplAudioThread, kFskThreadFlagsJoinable | kFskThreadFlagsWaitForInit, audio, "KplAudioThread"));
bail:
	return err;
}

FskErr KplAudioStop(KplAudio audio)
{
	if (audio->thread) {
        audio->stopping = true;
		FskThreadJoin(audio->thread);
	}
	return kFskErrNone;
}

FskErr KplAudioGetProperty(KplAudio audio, UInt32 propertyID, KplProperty value)
{
	FskErr err = kFskErrNone;

	switch (propertyID) {
		case kKplPropertyAudioPreferredSampleRate:
			value->number = audio->rate;
			break;
		case kKplPropertyAudioPreferredUncompressedFormat:
			switch (audio->format) {
				case SND_PCM_FORMAT_S16_LE:
					value->string = FskStrDoCopy("x-audio-codec/pcm-16-le");
					break;
				case SND_PCM_FORMAT_S16_BE:
					value->string = FskStrDoCopy("x-audio-codec/pcm-16-be");
					break;
			}
			break;
		case kKplPropertyAudioPreferredBufferSize:
			value->integer = audio->rate;
			break;
		case kKplPropertyAudioSamplePosition:
			value->number = (double)audio->played;
			break;
		case kKplPropertyAudioVolume:
			err = FskMemPtrNew(sizeof(UInt32) * 2, (FskMemPtr*)&value->integers.integer);
			if (err) goto bail;
			value->integers.integer[0] = audio->volL;
			value->integers.integer[1] = audio->volR;
			break;
		case kKplPropertyAudioSingleThreadedClient:
			value->b = false;
			break;
		default:
			err = kFskErrUnimplemented;
			break;
	}

bail:
	return err;
}

#if 0
void KplAudioSetMasterVolume(long left, long right)
{
	long min, max;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "Master";

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, card);
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, min + (left * (max - min)) / 256);
	snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, min + (right * (max - min)) / 256);

	snd_mixer_close(handle);
}
#endif

FskErr KplAudioSetProperty(KplAudio audio, UInt32 propertyID, KplProperty value)
{
	FskErr err = kFskErrNone;
	switch(propertyID) {
		case kKplPropertyAudioVolume:
			audio->volL = (UInt16)value->integers.integer[0];
			audio->volR = (UInt16)value->integers.integer[1];
			break;
		default:
			err = kFskErrUnimplemented;
			break;
	}
	return err;
}

FskErr KplAudioSetDoneCallback(KplAudio audio, KplAudioDoneCallback cb, void *refCon)
{
	FskErr err = kFskErrNone;
	
	audio->doneCB = cb;
	audio->doneRefCon = refCon;
	return err;
}

FskErr KplAudioSetMoreCallback(KplAudio audio, KplAudioMoreCallback cb, void *refCon)
{
	FskErr err = kFskErrNone;
	
	audio->moreCB = cb;
	audio->moreRefCon = refCon;
	return err;
}

FskErr KplAudioGetSamplesQueued(KplAudio audio, UInt32 *samplesQueuedOut, UInt32 *targetQueueLengthOut)
{
	FskErr err = kFskErrNone;
	
	if (samplesQueuedOut) {
		KplAudioOutBlockGetSize(audio, samplesQueuedOut);
		*samplesQueuedOut /= (2 * audio->channels);
	}

	if (targetQueueLengthOut)
		*targetQueueLengthOut = audio->bufferMin / (2 * audio->channels);
	return err;
}

#if 0
#pragma mark -
#pragma mark KplAudioOutBlock
#endif
// --------------------------------------------------
// KplAudioOutBlock
// --------------------------------------------------

static FskErr KplAudioOutBlockAdd(KplAudio audio, const char* data, UInt32 size, void *refCon)
{
	FskErr err = kFskErrNone;
	KplAudioOutBlock buffer = NULL;

	if (size) {
		bailIfError(FskMemPtrNewClear(sizeof(KplAudioOutBlockRecord), (FskMemPtr *)&buffer));
		buffer->data = data;
		buffer->size = size;
		buffer->refCon = refCon;
        KplMutexAcquire(audio->mutex);
		FskListAppend(&audio->buffer, buffer);
		audio->bufferSize += size;
        KplMutexRelease(audio->mutex);
	}
	else {
		audio->ending = true;
	}
bail:
	return err;
}

static FskErr KplAudioOutBlockFill(KplAudio audio)
{
	FskErr err = kFskErrNone;
	UInt32 bufferSize;

	// fill if buffers contain less than 1 sec
	if (audio->moreCB && !audio->ending) {
		while (audio->bufferSize < audio->bufferMin) {
			bufferSize = audio->bufferSize;
			(audio->moreCB)(audio, audio->moreRefCon, (audio->bufferMin - audio->bufferSize) / (2 * audio->channels));
			if ((audio->ending = (bufferSize == audio->bufferSize))) break;
		}
	}
//bail:
	return err;
}

static FskErr KplAudioOutBlockGetSize(KplAudio audio, UInt32* queued)
{
	KplAudioOutBlock buffer = audio->buffer;
	*queued = 0;
	while (buffer) {
		*queued += buffer->size - buffer->offset;
		buffer = (KplAudioOutBlock)buffer->next;
	}
	return kFskErrNone;
}

static FskErr KplAudioOutBlockRead(KplAudio audio, char* data, UInt32 size)
{
	FskErr err = kFskErrNone;
	KplAudioOutBlock buffer;
	UInt32 available;
	UInt16 volL = audio->volL;
	UInt16 volR = audio->volR;
	SInt16* p = (SInt16*)data;
	UInt32 samples = size >> 1;
	Boolean silence = !volL && !volR;
	
	audio->silence = false;
	while ((buffer = audio->buffer)) {
		available = buffer->size - buffer->offset;
		if (available > size)
			available = size;
		if (silence)
			memset(data, 0, available);
		else
			memcpy(data, buffer->data + buffer->offset, available);
		data += available;
		size -= available;
		buffer->offset += available;
		audio->bufferSize -= available;
		if (size == 0)
			break;
		if (buffer->offset == buffer->size)
			KplAudioOutBlockRemove(audio, buffer);
	}
	if (size != 0) {
		memset(data, 0, size); // fill with silence
		err = kFskErrOperationFailed;
	}
	// adjust volume
	if (!silence && ((volL < 256) || (volR < 256))) {
		if (audio->channels == 1) {
			volL = (volL + volR) / 2;
			for (; samples; samples--) {
				SInt32 value = (*p * volL) >> 8;
				if (value < -32768) value = -32768;
				else if (value > 32767) value = 32767;
				*p++ = (SInt16)value;
			}
		}
		else {
			for (samples >>= 1; samples; samples--) {
				SInt32 value = (*p * volL) >> 8;
				if (value < -32768) value = -32768;
				else if (value > 32767) value = 32767;
				*p++ = (SInt16)value;
				value = (*p * volR) >> 8;
				if (value < -32768) value = -32768;
				else if (value > 32767) value = 32767;
				*p++ = (SInt16)value;
			}
		}
	}
	return err;
}

static FskErr KplAudioOutBlockReadSilence(KplAudio audio, char* data, UInt32 size)
{
	FskErr err = kFskErrNone;
	
	if (!audio->silence) {
		memset(data, 0, size); // fill with silence
		audio->silence = true;
	}
	return err;
}

static FskErr KplAudioOutBlockRemove(KplAudio audio, KplAudioOutBlock buffer)
{
	FskErr err = kFskErrNone;

    KplMutexAcquire(audio->mutex);
	FskListRemove(&audio->buffer, buffer);
    KplMutexRelease(audio->mutex);
	if (audio->doneCB)
		(audio->doneCB)(audio, audio->doneRefCon, buffer->refCon, true);
	FskMemPtrDispose(buffer);
	return err;
}

static FskErr KplAudioOutBlockRemoveAll(KplAudio audio)
{
	FskErr err = kFskErrNone;
	KplAudioOutBlock buffer;

	while ((buffer = audio->buffer))
		KplAudioOutBlockRemove(audio, buffer);
	audio->bufferSize = 0;
	return err;
}

#if 0
#pragma mark -
#pragma mark KplAudioThread
#endif
// --------------------------------------------------
// KplAudioThread
// --------------------------------------------------

static void KplAudioThread(void* refCon)
{
	KplAudio audio = refCon;
	snd_pcm_sframes_t frames;
	snd_pcm_t *pcm = NULL;
	snd_pcm_hw_params_t *hw_params = NULL;
	snd_pcm_sw_params_t *sw_params = NULL;
	unsigned int bufferTime = 200000;
	unsigned int bufferPeriod = bufferTime / 4;
	unsigned int rate = audio->rate;
    int status;
	FskErr err = kFskErrNone;
	
    /*
        Thread is alive
    */
	FskThreadInitializationComplete(FskThreadGetCurrent());

    /*
        Connect
    */
    status = snd_pcm_open(&pcm, kAlsaDeviceName, SND_PCM_STREAM_PLAYBACK, 0);
    if (status < 0)
        return;

    /*
        Configure
    */
	alsaIfError(snd_pcm_hw_params_malloc(&hw_params));
	alsaIfError(snd_pcm_hw_params_any(pcm, hw_params));
	alsaIfError(snd_pcm_hw_params_set_rate_resample(pcm, hw_params, 1));
	alsaIfError(snd_pcm_hw_params_set_access(pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED));
	// alsaIfError(snd_pcm_hw_params_test_channels(pcm, hw_params, audio->channels));
	// alsaIfError(snd_pcm_hw_params_test_rate(pcm, hw_params, rate, 0));

	alsaIfError(snd_pcm_hw_params_set_format(pcm, hw_params, audio->format));
	alsaIfError(snd_pcm_hw_params_set_channels(pcm, hw_params, audio->channels));
	alsaIfError(snd_pcm_hw_params_set_rate_near(pcm, hw_params, &rate, NULL));
	bailIfError(rate != audio->rate);
	alsaIfError(snd_pcm_hw_params_set_period_time_near(pcm, hw_params, &bufferPeriod, 0));
	alsaIfError(snd_pcm_hw_params_set_buffer_time_near(pcm, hw_params, &bufferTime, 0));
	alsaIfError(snd_pcm_hw_params(pcm, hw_params));

	alsaIfError(snd_pcm_hw_params_get_period_size(hw_params, &audio->period, 0));
	alsaIfError(snd_pcm_hw_params_get_buffer_size(hw_params, &audio->frames));

	alsaIfError(snd_pcm_sw_params_malloc(&sw_params));
	alsaIfError(snd_pcm_sw_params_current(pcm, sw_params));
	alsaIfError(snd_pcm_sw_params_set_avail_min(pcm, sw_params, audio->period));

	alsaIfError(snd_pcm_sw_params_set_start_threshold(pcm, sw_params, (audio->frames / audio->period) * audio->period));
	alsaIfError(snd_pcm_sw_params(pcm, sw_params));
	audio->dataSize = audio->channels * audio->period * snd_pcm_format_physical_width(audio->format) / 8;
	FskMemPtrDispose(audio->data);
	bailIfError(FskMemPtrNewClear(audio->dataSize, (FskMemPtr *)&audio->data));

    /*
        Play
    */

	snd_pcm_prepare(pcm);
	while (!audio->stopping) {
		if (audio->playing) {
			KplMutexAcquire(audio->mutex);
			KplAudioOutBlockFill(audio);
			KplAudioOutBlockRead(audio, audio->data, audio->dataSize);
			KplMutexRelease(audio->mutex);
		}
		else {
	 		KplAudioOutBlockReadSilence(audio, audio->data, audio->dataSize);
		}
		if ((frames = snd_pcm_writei(pcm, audio->data, audio->period)) < 0)
			snd_pcm_recover(pcm, frames, 0);
		else
			audio->played += frames;
	}

    /*
        Finish
    */

    if (snd_pcm_state(pcm) == SND_PCM_STATE_RUNNING)
        snd_pcm_drop(pcm);

	snd_pcm_reset(pcm);

bail:
	if (hw_params)
		snd_pcm_hw_params_free(hw_params);
	if (sw_params)
		snd_pcm_sw_params_free(sw_params);

    /*
        Disconnect
    */

	snd_pcm_close(pcm);

    /*
        Reset
    */
    audio->thread = NULL;
    audio->stopping = false;
    audio->ending = false;

    KplAudioOutBlockRemoveAll(audio);
}
