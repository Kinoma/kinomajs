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
#include "FskFiles.h"
#include "FskMediaReader.h"
#include "FskAudio.h"

#include "kprHandler.h"
#include "kprShell.h"
#include "kprSkin.h"
#include "kprSound.h"
#include "kprURL.h"

typedef struct {
	const char *path;
	FskFile fref;
	char buffer[1024];
} KprSoundLoadSpoolerRefconRecord, *KprSoundLoadSpoolerRefcon;

static void KprSoundDispose(void* it);
static FskErr KprSoundLoad(KprSound self);
static FskErr KprSoundLoadSpoolerOpen(FskMediaSpooler spooler, UInt32 permissions);
static FskErr KprSoundLoadSpoolerClose(FskMediaSpooler spooler);
static FskErr KprSoundLoadSpoolerRead(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesRead);
static FskErr KprSoundLoadSpoolerGetSize(FskMediaSpooler spooler, FskInt64 *size);
static FskErr KprSoundPlayMore(struct FskSndChannelRecord *sndChan, void *refCon, SInt32 requestedSamples);
static void KprSoundPlayMoreCallback(struct FskTimeCallBackRecord *callback UNUSED, const FskTime time UNUSED, void *it);
static void KprSoundUnload(KprSound self);

static FskErr (*FskMediaReaderNewProc)(FskMediaReader *reader, const char *mimeType, const char *uri, FskMediaSpooler source, FskMediaReaderEvent eventHandler, void *refCon) = NULL;
static FskErr (*FskMediaReaderDisposeProc)(FskMediaReader reader) = NULL;
static FskErr (*FskMediaReaderExtractProc)(FskMediaReader reader, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data) = NULL;
static FskErr (*FskMediaReaderGetPropertyProc)(FskMediaReader reader, UInt32 propertyID, FskMediaPropertyValue property) = NULL;
static FskErr (*FskMediaReaderGetTrackProc)(FskMediaReader reader, SInt32 index, FskMediaReaderTrack *track) = NULL;
static FskErr (*FskMediaReaderStartProc)(FskMediaReader reader, double *startTime, double *endTime) = NULL;
static FskErr (*FskMediaReaderStopProc)(FskMediaReader reader) = NULL;
static FskErr (*FskMediaReaderTrackGetPropertyProc)(FskMediaReaderTrack track, UInt32 propertyID, FskMediaPropertyValue property) = NULL;

static FskSndChannel gSoundChannel = NULL;
static FskTimeCallBack gSoundTimer = NULL;

void KprSoundSetup()
{
	FskErr err = kFskErrNone;
	FskMediaPropertyValueRecord ambientAudio = {kFskMediaPropertyTypeInteger, {kFskAudioOutCategoryAmbient}};
#if FSK_EXTENSION_EMBED
#else
	char* applicationPath = NULL;
	char* libraryPath = NULL;
#if TARGET_OS_WIN32
	char* libraryName = "mediareader.dll";
#else
	char* libraryName = "mediareader.so";
#endif
	FskLibrary library = NULL;
#endif
	
	if (gSoundChannel) goto bail;
	
#if FSK_EXTENSION_EMBED
	FskMediaReaderNewProc = FskMediaReaderNew;
	FskMediaReaderDisposeProc = FskMediaReaderDispose;
	FskMediaReaderExtractProc = FskMediaReaderExtract;
	FskMediaReaderGetPropertyProc = FskMediaReaderGetProperty;
	FskMediaReaderGetTrackProc = FskMediaReaderGetTrack;
	FskMediaReaderStartProc = FskMediaReaderStart;
	FskMediaReaderStopProc = FskMediaReaderStop;
	FskMediaReaderTrackGetPropertyProc = FskMediaReaderTrackGetProperty;
#else
	applicationPath = FskGetApplicationPath();
	bailIfNULL(applicationPath);
	bailIfError(FskMemPtrNewClear(FskStrLen(applicationPath) + FskStrLen(libraryName) + 1, &libraryPath));
	FskStrCopy(libraryPath, applicationPath);
	FskStrCat(libraryPath, libraryName);
	bailIfError(FskLibraryLoad(&library, libraryPath));
	bailIfError(FskLibraryGetSymbolAddress(library, "FskMediaReaderNew", &FskMediaReaderNewProc));
	bailIfError(FskLibraryGetSymbolAddress(library, "FskMediaReaderDispose", &FskMediaReaderDisposeProc));
	bailIfError(FskLibraryGetSymbolAddress(library, "FskMediaReaderExtract", &FskMediaReaderExtractProc));
	bailIfError(FskLibraryGetSymbolAddress(library, "FskMediaReaderGetProperty", &FskMediaReaderGetPropertyProc));
	bailIfError(FskLibraryGetSymbolAddress(library, "FskMediaReaderGetTrack", &FskMediaReaderGetTrackProc));
	bailIfError(FskLibraryGetSymbolAddress(library, "FskMediaReaderStart", &FskMediaReaderStartProc));
	bailIfError(FskLibraryGetSymbolAddress(library, "FskMediaReaderStop", &FskMediaReaderStopProc));
	bailIfError(FskLibraryGetSymbolAddress(library, "FskMediaReaderTrackGetProperty", &FskMediaReaderTrackGetPropertyProc));
#endif
	bailIfError(FskSndChannelNew(&gSoundChannel, 0, kFskAudioFormatUndefined, 0));
	FskTimeCallbackNew(&gSoundTimer);

	FskSndChannelSetProperty(gSoundChannel, kFskMediaPropertyAudioCategory, &ambientAudio);

bail:
#if FSK_EXTENSION_EMBED
#else
	if (err)
		FskLibraryUnload(library);
	else
		FskMemPtrDispose(library);
	FskMemPtrDispose(libraryPath);
	FskMemPtrDispose(applicationPath);
#endif
	return;
}

void KprSoundCleanup()
{
	FskSndChannelDispose(gSoundChannel);
	gSoundChannel = NULL;
	FskTimeCallbackDispose(gSoundTimer);
	gSoundTimer = NULL;
}

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprSoundInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprSound", FskInstrumentationOffset(KprSoundRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif

FskErr KprSoundNew(KprSound *it, KprContext context, char* base, char* url, char* mime)
{
	FskErr err = kFskErrNone;
	KprSound self;
	KprSoundSetup();
	bailIfError(KprAssetNew((KprAsset *)it, sizeof(KprSoundRecord), context, &context->firstSound, KprSoundDispose));
	self = *it;	
	FskInstrumentedItemNew(self, NULL, &KprSoundInstrumentation);
#if SUPPORT_INSTRUMENTATION
	FskInstrumentedItemSetOwner(self, context);
#endif
	if (base && url)
		bailIfError(KprURLMerge(base, url, &self->url));	
	if (mime) {
		self->mime = FskStrDoCopy(mime);
		bailIfNULL(self->mime);
	}
bail:
	return err;
}

void KprSoundDispose(void* it)
{
	KprSound self = it;
	KprSoundUnload(self);
	FskMemPtrDispose(self->mime);
	FskMemPtrDispose(self->url);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

FskErr KprSoundLoad(KprSound self)
{
	FskErr err = kFskErrNone;
	char *path = NULL;
	FskFile fref = NULL;
	FskMediaSpoolerRecord spooler;
	KprSoundLoadSpoolerRefconRecord refcon;
	FskMediaReader reader = NULL;
	FskMediaReaderTrack track;
	FskMediaPropertyValueRecord property;
	char *audioFormat = NULL;
	double duration;
	unsigned char *formatInfo = NULL;
	UInt32 formatInfoSize;
	UInt16 numChannels;
	double sampleRate;
	double timeScale;
	FskMediaReaderSampleInfo info = NULL;
	unsigned char *buffer = NULL;
	KprSoundFrame frames = NULL;
	UInt32 frameCount = 0;
	unsigned char *data = NULL;
	UInt32 dataSize = 0;

	if (self->data) goto bail;

	bailIfError(KprURLToPath(self->url, &path));
	if (!self->mime) {
		unsigned char buffer[4096];
		UInt32 size;
        char *sniff = NULL;
		bailIfError(FskFileOpen(path, kFskFilePermissionReadOnly, &fref));
		bailIfError(FskFileRead(fref, sizeof(buffer), buffer, &size));
		FskFileClose(fref);
		fref = NULL;
		bailIfError(FskMediaPlayerSniffForMIME(buffer, size, NULL, self->url, &sniff));
		self->mime = sniff;
	}
	FskMemSet(&spooler, 0, sizeof(spooler));
	FskMemSet(&refcon, 0, sizeof(refcon));
	refcon.path = path;
	spooler.refcon = &refcon;
	spooler.doOpen = KprSoundLoadSpoolerOpen;
	spooler.doClose = KprSoundLoadSpoolerClose;
	spooler.doRead = KprSoundLoadSpoolerRead;
	spooler.doGetSize = KprSoundLoadSpoolerGetSize;
	bailIfError((*FskMediaReaderNewProc)(&reader, self->mime, self->url, &spooler, NULL, NULL));
	bailIfError((*FskMediaReaderGetPropertyProc)(reader, kFskMediaPropertyTimeScale, &property));
	timeScale = property.value.integer;
	(*FskMediaReaderGetPropertyProc)(reader, kFskMediaPropertyDuration, &property);
	duration = property.value.number;
	(*FskMediaReaderGetTrackProc)(reader, 0, &track);
	(*FskMediaReaderTrackGetPropertyProc)(track, kFskMediaPropertyFormat, &property);
	audioFormat = property.value.str;
	if (kFskErrNone == (*FskMediaReaderTrackGetPropertyProc)(track, kFskMediaPropertyFormatInfo, &property)) {
		formatInfo = property.value.data.data;
		formatInfoSize = property.value.data.dataSize;
	}
	else
		formatInfoSize = 0;

	(*FskMediaReaderTrackGetPropertyProc)(track, kFskMediaPropertyChannelCount, &property);
	numChannels = property.value.integer;
	(*FskMediaReaderTrackGetPropertyProc)(track, kFskMediaPropertySampleRate, &property);
	sampleRate = property.value.integer;
	bailIfError((*FskMediaReaderStartProc)(reader, NULL, NULL));
	for (;;) {
		UInt32 c = 0, i, s;
		unsigned char *d;
		err = (*FskMediaReaderExtractProc)(reader, &track, &c, &info, &buffer);
        if (err == kFskErrEndOfFile) {
            err = kFskErrNone;
            break;
        }
        if (err != kFskErrNone)
            goto bail;
		for (i = 0, d = buffer; i < c; i++, d += s) {
			s = info[i].sampleSize * info[i].samples;
            bailIfError(FskMemPtrRealloc((frameCount + 1) * sizeof(KprSoundFrameRecord), &frames));
			frames[frameCount].count = info[i].samples;
			frames[frameCount].frameSize = info[i].sampleSize;
			frames[frameCount].samplesPerFrame = info[i].sampleDuration;
			frames[frameCount].frameOffset = dataSize;
            frameCount++;
			bailIfError(FskMemPtrRealloc(dataSize + s, &data));
			FskMemCopy(data + dataSize, d, s);
			dataSize += s;
		}
		FskMemPtrDispose(buffer);
		buffer = NULL;
		FskMemPtrDispose(info);
		info = NULL;
	}
	(*FskMediaReaderStopProc)(reader);
	self->audioFormat = audioFormat;
	self->duration = duration;
	self->formatInfo = formatInfo;
	self->formatInfoSize = formatInfoSize;
	self->numChannels = numChannels;
	self->sampleRate = sampleRate;
	self->timeScale = timeScale;
	self->data = data;
	self->dataSize = dataSize;
	self->frames = frames;
	self->frameCount = frameCount;
bail:
	if (err) {
		FskMemPtrDispose(data);
		FskMemPtrDispose(frames);
		FskMemPtrDispose(formatInfo);
		FskMemPtrDispose(audioFormat);
	}
	FskMemPtrDispose(buffer);
	FskMemPtrDispose(info);
	(*FskMediaReaderDisposeProc)(reader);
    FskFileClose(fref);
	FskMemPtrDispose(path);
	return err;
}

FskErr KprSoundLoadSpoolerClose(FskMediaSpooler spooler)
{
	KprSoundLoadSpoolerRefcon refcon = spooler->refcon;
	return FskFileClose(refcon->fref);
}

FskErr KprSoundLoadSpoolerGetSize(FskMediaSpooler spooler, FskInt64 *size)
{
	KprSoundLoadSpoolerRefcon refcon = spooler->refcon;
	return FskFileGetSize(refcon->fref, size);
}

FskErr KprSoundLoadSpoolerOpen(FskMediaSpooler spooler, UInt32 permissions)
{
	KprSoundLoadSpoolerRefcon refcon = spooler->refcon;
	return FskFileOpen(refcon->path, permissions, &refcon->fref);
}

FskErr KprSoundLoadSpoolerRead(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesRead)
{
	FskErr err = kFskErrNone;
	KprSoundLoadSpoolerRefcon refcon = spooler->refcon;
    bailIfError(FskFileSetPosition(refcon->fref, &position));
	bailIfError(FskFileRead(refcon->fref, (bytesToRead > sizeof(refcon->buffer)) ? sizeof(refcon->buffer) : bytesToRead, refcon->buffer, bytesRead));
	*buffer = refcon->buffer;
bail:
    return err;
}

FskErr KprSoundPlay(KprSound self)
{
	FskErr err = kFskErrNone;
	FskSndChannel soundChannel = gSoundChannel;
    float volume;
	UInt32 c;
	KprSoundFrame frame;
	
	bailIfNULL(soundChannel);
	bailIfError(KprSoundLoad(self));
	FskTimeCallbackRemove(gSoundTimer);
	FskSndChannelSetMoreCallback(soundChannel, NULL, NULL);
	FskSndChannelGetVolume(soundChannel, &volume);
    FskSndChannelStop(soundChannel);
	bailIfError(FskSndChannelSetFormat(soundChannel, kFskAudioFormatUndefined, self->audioFormat, self->numChannels, self->sampleRate, self->formatInfo, self->formatInfoSize));
	c = self->frameCount;
	frame = &self->frames[0];
	while (c) {
		bailIfError(FskSndChannelEnqueue(soundChannel, self->data + frame->frameOffset, frame->frameSize * frame->count, frame->count, frame->samplesPerFrame, NULL, NULL));
		c--;
		frame++;
	}
    self->playing = true;
	FskSndChannelSetVolume(soundChannel, volume);
	FskSndChannelSetMoreCallback(soundChannel, KprSoundPlayMore, self);
	bailIfError(FskSndChannelStart(soundChannel, 0));
bail:	
	return err;
}

FskErr KprSoundPlayMore(struct FskSndChannelRecord *sndChan, void *it, SInt32 requestedSamples UNUSED)
{
	KprSound self = it;
	FskSampleTime now;
	FskTimeRecord when;

	if (kFskErrNone != FskSndChannelGetSamplePosition(sndChan, &now))
		now = 0;

	FskTimeGetNow(&when);
	FskTimeAddMS(&when, 100 + (1000.0 * ((self->duration / self->timeScale) - ((double)now / (double)self->sampleRate))));
	FskTimeCallbackSet(gSoundTimer, &when, KprSoundPlayMoreCallback, self);

	FskSndChannelSetMoreCallback(sndChan, NULL, NULL);
	return kFskErrEndOfFile;
}

void KprSoundPlayMoreCallback(struct FskTimeCallBackRecord *callback UNUSED, const FskTime time UNUSED, void *it UNUSED) 
{
    FskSndChannelStop(gSoundChannel);
}

void KprSoundUnload(KprSound self)
{
    FskSndChannelStop(gSoundChannel);
	FskMemPtrDisposeAt(&self->audioFormat);
	self->numChannels = 0;
	self->sampleRate = 0;
	FskMemPtrDisposeAt(&self->formatInfo);
	self->formatInfoSize = 0;
	FskMemPtrDisposeAt(&self->frames);
	self->frameCount = 0;
	FskMemPtrDisposeAt(&self->data);
	self->dataSize = 0;
}

/* ECMASCRIPT */

void KPR_sound(void *it)
{
	if (it) {
		KprSound self = it;
		kprVolatileDestructor(KPR_sound);
		KprAssetDispose(self);
	}
}

void KPR_Sound(xsMachine *the)
{
	KprSound self;
	xsStringValue reference = xsToString(xsArg(0));
	xsStringValue base = xsToString(xsModuleURL);
    FskErr err = KprSoundNew(&self, xsGetContext(the), base, reference, NULL);
	xsThrowDiagnosticIfFskErr(err, "SoundNew failed with error %s", FskInstrumentationGetErrorString(err));
	kprVolatileConstructor(KPR_Sound);
}

void KPR_sound_play(xsMachine *the)
{
	KprSound self = xsGetHostData(xsThis);
	FskErr err = KprSoundPlay(self);
    if (kFskErrNone != err)
        xsTraceDiagnostic("Sound play failed with error %s", FskInstrumentationGetErrorString(err));
}

void KPR_Sound_get_volume(xsMachine *the)
{
	float volume = 0;
	if (NULL != gSoundChannel)
		FskSndChannelGetVolume(gSoundChannel, &volume);
	xsResult = xsNumber(volume);
}

void KPR_Sound_set_volume(xsMachine *the)
{
	float volume = (float)xsToNumber(xsArg(0));
	if (NULL != gSoundChannel)
		FskSndChannelSetVolume(gSoundChannel, volume);
}

void KPR_Sound_hibernate(xsMachine *the UNUSED)
{
	FskMediaPropertyValueRecord value;
    FskSndChannelStop(gSoundChannel);
	FskTimeCallbackRemove(gSoundTimer);
	value.value.b = true;
	value.type = kFskMediaPropertyTypeBoolean;
	FskSndChannelSetProperty(gSoundChannel, kFskMediaPropertyHibernate, &value);
}

extern void fxAliasInstance(xsMachine* the, xsSlot* slot);

void KPR_Sound_patch(xsMachine *the)
{
	xsResult = xsGet(xsGlobal, xsID("Sound"));
	xsNewHostProperty(xsResult, xsID("volume"), xsNewHostFunction(KPR_Sound_get_volume, 0), xsIsGetter, xsDontScript | xsIsGetter);
	xsNewHostProperty(xsResult, xsID("volume"), xsNewHostFunction(KPR_Sound_set_volume, 1), xsIsSetter, xsDontScript | xsIsSetter);
	xsNewHostProperty(xsResult, xsID("hibernate"), xsNewHostFunction(KPR_Sound_hibernate, 0), xsDefault, xsDontScript);
}
