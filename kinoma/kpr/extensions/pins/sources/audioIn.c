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
#include "kprPins.h"

#include "FskECMAScript.h"
#include "FskAudioIn.h"
#include "FskMemory.h"
#include "FskManifest.xs.h"

#define KPR_NO_GRAMMAR 1

typedef struct {
    // input
    FskAudioIn      audioIn;
    FskMemPtr       samples;
    UInt32          samplesSize; // bytes

    // output
    FskSndChannel       sndChan;

    // both
    KprPinsPoller       poller;
    UInt32              sampleSize;
} xsAudioRecord, *xsAudio;

static FskErr audioInCallback(FskAudioIn audioIn, void *refCon, void *data, UInt32 dataSize);
static void audioOutBlockDone(FskSndChannel sndChan, void *refCon, void *dataRefCon, Boolean played);
static FskErr audioOutMore(FskSndChannel sndChan, void *refCon, SInt32 requestedSamples);

void xs_audio(void *data)
{
    xsAudio a = data;
    if (a) {
        FskSndChannelDispose(a->sndChan);
        FskAudioInDispose(a->audioIn);
        FskMemPtrDispose(a->samples);
        FskMemPtrDispose(a);
    }
}

void xs_audio_init(xsMachine* the)
{
    FskErr err;
    xsAudio a;
    SInt32 sampleRate = xsToInteger(xsGet(xsThis, xsID("sampleRate")));
    SInt32 channels = xsToInteger(xsGet(xsThis, xsID("channels")));
    char *direction = xsToString(xsGet(xsThis, xsID("direction")));

    if (xsGetHostData(xsThis))
        xsThrowDiagnosticIfFskErr(kFskErrBadState, "Audio pin already initialized.", kFskErrBadState);

    err = FskMemPtrNewClear(sizeof(xsAudioRecord), &a);
    xsThrowIfFskErr(err);

    a->sampleSize = 2 * channels;

    xsSetHostData(xsThis, a);

    if (0 == FskStrCompare(direction, "input")) {
        err = FskAudioInNew(&a->audioIn, 0, audioInCallback, a);
        if (err) goto bail;

        err = FskAudioInSetFormat(a->audioIn, kFskAudioFormatPCM16BitLittleEndian, channels, sampleRate, NULL, 0);
        if (err) goto bail;
    }
    else
    if (0 == FskStrCompare(direction, "output")) {
        err = FskSndChannelNew(&a->sndChan, 0, kFskAudioFormatPCM16BitLittleEndian, 0);
        if (err) goto bail;

        err = FskSndChannelSetFormat(a->sndChan, kFskAudioFormatPCM16BitLittleEndian, NULL, channels, sampleRate, NULL, 0);
        if (err) goto bail;

        FskSndChannelSetDoneCallback(a->sndChan, audioOutBlockDone, a);
        FskSndChannelSetMoreCallback(a->sndChan, audioOutMore, a);
    }
    else {
        xsTraceDiagnostic("Audio invalid pin direction %s", direction);
		err = kFskErrInvalidParameter;
        goto bail;
    }

bail:
    if (err) {
        xs_audio(a);
        xsThrowDiagnosticIfFskErr(err, "Audio initialization failed with error %s", FskInstrumentationGetErrorString(err));
    }
}

void xs_audio_read(xsMachine* the)
{
    xsAudio a = xsGetHostData(xsThis);
    if (a) {
        xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(a->samplesSize));
        FskMemMove(xsGetHostData(xsResult), a->samples, a->samplesSize);

        FskMemPtrDisposeAt(&a->samples);
        a->samplesSize = 0;
    }
}

void xs_audio_repeat(xsMachine* the)
{
    xsAudio a = xsGetHostData(xsThis);
    if (a)
        a->poller = (xsTest(xsArg(0))) ? xsGetHostData(xsArg(0)) : NULL;
}

void xs_audio_close(xsMachine* the)
{
    xs_audio(xsGetHostData(xsThis));
    xsSetHostData(xsThis, NULL);
}

FskErr audioInCallback(FskAudioIn audioIn, void *refCon, void *data, UInt32 dataSize)
{
    xsAudio a = refCon;
    FskErr err = kFskErrNone;

    err = FskMemPtrRealloc(a->samplesSize + dataSize, &a->samples);
    if (err) goto bail;

    FskMemMove(a->samples + a->samplesSize, data, dataSize);
    a->samplesSize += dataSize;

	if (a->poller)
		KprPinsPollerRun(a->poller);

bail:
    return err;
}

void xs_audio_write(xsMachine* the)
{
    xsAudio a = xsGetHostData(xsThis);
    if (a) {
		FskErr err;
        void *data = xsGetHostData(xsArg(0)), *scratch;
        SInt32 dataSize = xsToInteger(xsGet(xsArg(0), xsID("length")));
        FskMemPtrNewFromData(dataSize, data, &scratch);
        err = FskSndChannelEnqueue(a->sndChan, scratch, dataSize, dataSize / a->sampleSize, 1, scratch, NULL);
        xsThrowDiagnosticIfFskErr(err, "Audio write failed with error %s", FskInstrumentationGetErrorString(err));
    }
}

void xs_audio_start(xsMachine *the)
{
    xsAudio a = xsGetHostData(xsThis);
    if (a)
        if (a->sndChan)
            FskSndChannelStart(a->sndChan, 0);
        if (a->audioIn)
            FskAudioInStart(a->audioIn);
}

void xs_audio_stop(xsMachine *the)
{
    xsAudio a = xsGetHostData(xsThis);
    if (a) {
        if (a->sndChan)
            FskSndChannelStop(a->sndChan);
        if (a->audioIn)
            FskAudioInStop(a->audioIn);
    }
}

void xs_audio_setVolume(xsMachine *the)
{
    xsAudio a = xsGetHostData(xsThis);
    if (a) {
        if (a->sndChan)
            FskSndChannelSetVolume(a->sndChan, xsToNumber(xsArg(0)));
    }
}

void audioOutBlockDone(FskSndChannel sndChan, void *refCon, void *dataRefCon, Boolean played)
{
    FskMemPtrDispose(dataRefCon);
}

FskErr audioOutMore(FskSndChannel sndChan, void *refCon, SInt32 requestedSamples)
{
    xsAudio a = refCon;

	if (a->poller)
		KprPinsPollerRun(a->poller);       // untested!

    return kFskErrNone;
}
