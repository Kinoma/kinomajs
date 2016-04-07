/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#define __FSKECMASCRIPT_PRIV__
#include "xs.h"
#ifdef KPR_CONFIG
#include "FskManifest.xs.h"
#define FskECMAScriptThrowIf(THE, ERROR) xsThrowIfFskErr(ERROR)
#endif

#include "FskMediaReader.h"
#include "FskECMAScript.h"
#include "FskFiles.h"

typedef struct xsNativeMediaReaderTrackRecord xsNativeMediaReaderTrackRecord;
typedef struct xsNativeMediaReaderTrackRecord *xsNativeMediaReaderTrack;

struct xsNativeMediaReaderTrackRecord {
	FskMediaReader				reader;
	FskMediaReaderTrack			readerTrack;
};

typedef struct {
	FskMediaReader				reader;

	char						*url;
	char						*path;
	FskFile						fref;
	unsigned char				*readBuffer;
	UInt32						readBufferSize;

	xsNativeMediaReaderTrack	tracks;

	xsMachine					*the;
	xsSlot						obj;
#ifndef KPR_CONFIG
	FskECMAScript				vm;
#endif
	FskMediaSpoolerRecord		spooler;

	FskFileOffset				streamOffset;
	void						*lastReadBuffer;
} xsNativeMediaReaderRecord, *xsNativeMediaReader;

static FskErr mediaReaderOpenFile(FskMediaSpooler spooler, UInt32 permissions);
static FskErr mediaReaderCloseFile(FskMediaSpooler spooler);
static FskErr mediaReaderReadFile(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesRead);
static FskErr mediaReaderGetSizeFile(FskMediaSpooler spooler, FskInt64 *size);

#ifndef KPR_CONFIG
static FskErr mediaReaderOpenStream(FskMediaSpooler spooler, UInt32 permissions);
static FskErr mediaReaderCloseStream(FskMediaSpooler spooler);
static FskErr mediaReaderReadStream(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesRead);
static FskErr mediaReaderGetSizeStream(FskMediaSpooler spooler, FskInt64 *size);
#endif

static FskErr mediaReaderEvent(FskMediaReader reader, void *refCon, FskEventCodeEnum eventCode, FskEvent event);

void xs_mediareader_destructor(void *data);

static FskErr instantiateReader(xsMachine *the, xsNativeMediaReader nmr, const char *mime);
static void updateTrackList(xsNativeMediaReader nmr);

void xs_mediareader_canHandle(xsMachine *the)
{
	FskErr err;

	if (xsReferenceType == xsTypeOf(xsArg(0))) {
		unsigned char *data = NULL;
		UInt32 dataSize;
		char *mime, *uri = NULL;

		if (xsIsInstanceOf(xsArg(0), xsChunkPrototype)) {
			xsTrace("Calling MediaReader.canHandle with Chunk is deprecated. Pass ArrayBuffer.\n");
			data = xsGetHostData(xsArg(0));
			dataSize = xsToInteger(xsGet(xsArg(0), xsID("length")));
		}
		else if (xsIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
			data = xsToArrayBuffer(xsArg(0));
			dataSize = xsGetArrayBufferLength(xsArg(0));
		}
		
		if (!data)
			return;

		// could take http client instance as optional argument to extract headers

		if (xsToInteger(xsArgc) >= 2) {
			if (xsTest(xsArg(1)))
				uri = xsToStringCopy(xsArg(1));
		}

		err = FskMediaReaderSniffForMIME(data, dataSize, NULL, uri, &mime);
		FskMemPtrDispose(uri);
		if (kFskErrNone != err)
			xsResult = xsFalse;
		else {
			xsResult = xsString(mime);
			FskMemPtrDispose(mime);
		}
	}
	else {
		err = FskMediaReaderNew(NULL, xsToString(xsArg(0)), NULL, NULL, NULL, NULL);
		xsResult = xsBoolean(kFskErrNone == err);
	}
}

#ifdef KPR_CONFIG
void FskMediaReader_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("FskMediaReader"));
	xsNewHostProperty(xsResult, xsID("canHandle"), xsNewHostFunction(xs_mediareader_canHandle, 1), xsDefault, xsDontScript);
}

void FskMediaProperty_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("FskMediaProperty"));
	xsNewHostProperty(xsResult, xsID("enabled"), xsInteger(kFskMediaPropertyEnabled), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("mediaType"), xsInteger(kFskMediaPropertyMediaType), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("DRMKey"), xsInteger(kFskMediaPropertyDRMKey), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("DRMInfo"), xsInteger(kFskMediaPropertyDRMInfo), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("timeScale"), xsInteger(kFskMediaPropertyTimeScale), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("flags"), xsInteger(kFskMediaPropertyFlags), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("duration"), xsInteger(kFskMediaPropertyDuration), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("editList"), xsInteger(kFskMediaPropertyEditList), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("dimensions"), xsInteger(kFskMediaPropertyDimensions), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("format"), xsInteger(kFskMediaPropertyFormat), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("esds"), xsInteger(kFskMediaPropertyESDS), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("bitRate"), xsInteger(kFskMediaPropertyBitRate), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("sampleRate"), xsInteger(kFskMediaPropertySampleRate), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("channelCount"), xsInteger(kFskMediaPropertyChannelCount), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("securityType"), xsInteger(kFskMediaPropertySecurityType), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("securityData"), xsInteger(kFskMediaPropertySecurityData), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("id"), xsInteger(kFskMediaPropertyID), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("message"), xsInteger(kFskMediaPropertyMessage), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("clip"), xsInteger(kFskMediaPropertyClip), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("xsObject"), xsInteger(kFskMediaPropertyXSObject), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("crop"), xsInteger(kFskMediaPropertyCrop), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("dataSize"), xsInteger(kFskMediaPropertyDataSize), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("requestHeaders"), xsInteger(kFskMediaPropertyRequestHeaders), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("samplesPerChunk"), xsInteger(kFskMediaPropertySamplesPerChunk), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("volume"), xsInteger(kFskMediaPropertyVolume), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("pixelFormat"), xsInteger(kFskMediaPropertyPixelFormat), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("quality"), xsInteger(kFskMediaPropertyQuality), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("encryptedMetaData"), xsInteger(kFskMediaPropertyEncryptedMetaData), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("trInfo"), xsInteger(kFskMediaPropertyTRInfo), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("omaTagMode"), xsInteger(kFskMediaPropertyOMATagMode), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("metaDataItems"), xsInteger(kFskMediaPropertyMetaDataItems), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("profile"), xsInteger(kFskMediaPropertyProfile), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("canChangeSampleRate"), xsInteger(kFskMediaPropertyCanChangeSampleRate), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("canChangeChannelCount"), xsInteger(kFskMediaPropertyCanChangeChannelCount), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("playRate"), xsInteger(kFskMediaPropertyPlayRate), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("time"), xsInteger(kFskMediaPropertyTime), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("state"), xsInteger(kFskMediaPropertyState), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("frameRate"), xsInteger(kFskMediaPropertyFrameRate), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("keyFrameRate"), xsInteger(kFskMediaPropertyKeyFrameRate), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("compressionSettings"), xsInteger(kFskMediaPropertyCompressionSettings), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("pixelAspectRatio"), xsInteger(kFskMediaPropertyPixelAspectRatio), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("balance"), xsInteger(kFskMediaPropertyBalance), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("buffer"), xsInteger(kFskMediaPropertyBuffer), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("contrast"), xsInteger(kFskMediaPropertyContrast), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("brightness"), xsInteger(kFskMediaPropertyBrightness), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("scrub"), xsInteger(kFskMediaPropertyScrub), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("eq"), xsInteger(kFskMediaPropertyEQ), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("redirect"), xsInteger(kFskMediaPropertyRedirect), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("error"), xsInteger(kFskMediaPropertyError), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("markTimes"), xsInteger(kFskMediaPropertyMarkTimes), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("authentication"), xsInteger(kFskMediaPropertyAuthentication), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("authorization"), xsInteger(kFskMediaPropertyAuthorization), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("seekableSegment"), xsInteger(kFskMediaPropertySeekableSegment), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("downloadPath"), xsInteger(kFskMediaPropertyDownloadPath), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("downloadPosition"), xsInteger(kFskMediaPropertyDownloadPosition), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("download"), xsInteger(kFskMediaPropertyDownload), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("sprites"), xsInteger(kFskMediaPropertySprites), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("location"), xsInteger(kFskMediaPropertyLocation), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("hibernate"), xsInteger(kFskMediaPropertyHibernate), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("next"), xsInteger(kFskMediaPropertyNext), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("position"), xsInteger(kFskMediaPropertyPosition), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("sampleCount"), xsInteger(kFskMediaPropertySampleCount), xsDefault, xsDontScript);			
	xsNewHostProperty(xsResult, xsID("mp4DecSpecificInfo"), xsInteger(kFskMediaPropertyExtraDataMP4DecSpecificInfo), xsDefault, xsDontScript);			
	xsNewHostProperty(xsResult, xsID("mp2vSequenceHeader"), xsInteger(kFskMediaPropertyExtraDataMP2VSequenceHeader), xsDefault, xsDontScript);			
	xsNewHostProperty(xsResult, xsID("atracFormatInfo"), xsInteger(kFskMediaPropertyExtraDataATRACFormatInfo), xsDefault, xsDontScript);			
	xsNewHostProperty(xsResult, xsID("formatInfo"), xsInteger(kFskMediaPropertyFormatInfo), xsDefault, xsDontScript);			
	xsNewHostProperty(xsResult, xsID("lens"), xsInteger(kFskMediaPropertyLens), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("autoFocusState"), xsInteger(kFskMediaPropertyAutoFocusState), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("autoFocusArea"), xsInteger(kFskMediaPropertyAutoFocusArea), xsDefault, xsDontScript);
	//xsNewHostProperty(xsResult, xsID("dimensionIndex"), xsInteger(kFskMediaPropertyDimensionIndex), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("dimensionList"), xsInteger(kFskMediaPropertyDimensionsList), xsDefault, xsDontScript);
}
#endif

void xs_mediareader_URL(xsMachine *the)
{
	FskErr err = kFskErrNone;
	xsNativeMediaReader nmr;
	char *url = NULL;
	char *mime = NULL;

	err = FskMemPtrNewClear(sizeof(xsNativeMediaReaderRecord), &nmr);
	BAIL_IF_ERR(err);

	{
	xsTry {
		url = xsToStringCopy(xsArg(0));
		mime = xsToStringCopy(xsArg(1));

		if (0 == FskStrCompareWithLength(url, "file://", 7)) {
			nmr->spooler.refcon = nmr;
			nmr->spooler.flags = kFskMediaSpoolerValid;
			nmr->spooler.doOpen = mediaReaderOpenFile;
			nmr->spooler.doClose = mediaReaderCloseFile;
			nmr->spooler.doRead = mediaReaderReadFile;
			nmr->spooler.doGetSize = mediaReaderGetSizeFile;

			nmr->path = xsToStringCopy(xsCall1(xsGlobal, xsID("decodeURI"), xsArg(0)));
			
#ifdef KPR_CONFIG
	#if TARGET_OS_WIN32
			// @@ BSF - file:/// -> file://
			if (url[7] == '/') {
				char *urlCopy = FskStrDoCat("file://", url + 8);
				BAIL_IF_NULL(urlCopy, err, kFskErrMemFull);
				FskMemPtrDispose(url);
				url = urlCopy;
			}
			if (nmr->path[7] == '/') {
				char *pathCopy = FskStrDoCat("file://", nmr->path + 8);
				BAIL_IF_NULL(pathCopy, err, kFskErrMemFull);
				FskMemPtrDispose(nmr->path);
				nmr->path = pathCopy;
			}
	#endif
#endif
		}

		nmr->the = the;
		nmr->obj = xsThis;
#ifndef KPR_CONFIG
		nmr->vm = (FskECMAScript)xsGetContext(the);
#endif
		nmr->url = url;
		url = NULL;

		err = instantiateReader(the, nmr, mime);
	}
	xsCatch {
		err = kFskErrOperationFailed;
	}
	}

bail:
	if (kFskErrNone != err)
		xs_mediareader_destructor(nmr);

	FskMemPtrDispose(url);
	FskMemPtrDispose(mime);
	FskECMAScriptThrowIf(the, err);
}

#ifndef KPR_CONFIG
void xs_mediareader_Stream(xsMachine *the)
{
	FskErr err = kFskErrNone;
	xsNativeMediaReader nmr;
	char *mime = NULL;

	err = FskMemPtrNewClear(sizeof(xsNativeMediaReaderRecord), &nmr);
	BAIL_IF_ERR(err);

	{
	xsTry {
		mime = xsToStringCopy(xsArg(1));

		nmr->spooler.refcon = nmr;
		nmr->spooler.flags = kFskMediaSpoolerValid;
		nmr->spooler.doOpen = mediaReaderOpenStream;
		nmr->spooler.doClose = mediaReaderCloseStream;
		nmr->spooler.doRead = mediaReaderReadStream;
		nmr->spooler.doGetSize = mediaReaderGetSizeStream;

		nmr->the = the;
		nmr->obj = xsThis;

		xsSet(xsThis, nmr->vm->id_stream, xsArg(0));

		err = instantiateReader(the, nmr, mime);
	}
	xsCatch {
		err = kFskErrOperationFailed;
	}
	}

bail:
	if (kFskErrNone != err)
		xs_mediareader_destructor(nmr);

	FskMemPtrDispose(mime);
	FskECMAScriptThrowIf(the, err);
}
#endif

FskErr instantiateReader(xsMachine *the, xsNativeMediaReader nmr, const char *mime)
{
	FskErr err;

	xsSet(xsThis, xsID("tracks"), xsNewInstanceOf(xsArrayPrototype));
	xsSetHostData(xsThis, nmr);
	err = FskMediaReaderNew(&nmr->reader, mime, nmr->url, (kFskMediaSpoolerValid & nmr->spooler.flags) ? &nmr->spooler : NULL,
							mediaReaderEvent, nmr);
	if (err)
		xsSetHostData(xsThis, NULL);
	return err;
}

void xs_mediareader_destructor(void *data)
{
	xsNativeMediaReader nmr = data;

	if (NULL == nmr)
		return;

	FskMediaReaderDispose(nmr->reader);

	FskMemPtrDispose(nmr->url);
	FskMemPtrDispose(nmr->path);
	FskFileClose(nmr->fref);
	FskMemPtrDispose(nmr->lastReadBuffer);
	FskMemPtrDispose(nmr);
}

void xs_mediareader_close(xsMachine *the)
{
	xs_mediareader_destructor(xsGetHostData(xsThis));
	xsSetHostData(xsThis, NULL);
}

void xs_mediareader_set(xsMachine *the)
{
	xsNativeMediaReader nmr = xsGetHostData(xsThis);
	FskErr err;
	UInt32 propertyID = xsToInteger(xsArg(0));
	UInt32 propertyDataType;
	Boolean get, set;

	err = FskMediaReaderHasProperty(nmr->reader, propertyID, &get, &set, &propertyDataType);
	if ((kFskErrNone == err) && set)
		err = FskMediaSetPropertyXSHelper(the, propertyDataType, propertyID, 1, (FskMediaSetPropertyXSHelperProc)FskMediaReaderSetProperty, nmr->reader);

	FskECMAScriptThrowIf(the, err);
}

void xs_mediareader_get(xsMachine *the)
{
	xsNativeMediaReader nmr = xsGetHostData(xsThis);
	FskMediaPropertyValueRecord property;

	if (kFskErrNone == FskMediaReaderGetProperty(nmr->reader, xsToInteger(xsArg(0)), &property))
		FskMediaGetPropertyXSHelper(the, &property);
}

void xs_mediareader_has(xsMachine *the)
{
	xsNativeMediaReader nmr = xsGetHostData(xsThis);
	Boolean get, set;
	UInt32 dataType = 0;
	FskMediaReaderHasProperty(nmr->reader, xsToInteger(xsArg(0)), &get, &set, &dataType);
	if (get || set)
		xsResult = xsInteger(dataType);
	else
		xsResult = xsUndefined;
}

void xs_mediareader_start(xsMachine *the)
{
	xsNativeMediaReader nmr = xsGetHostData(xsThis);
	double startTime, endTime;
	FskErr err;

	switch (xsToInteger(xsArgc)) {
		case 0:
			err	= FskMediaReaderStart(nmr->reader, NULL, NULL);
			break;

		case 1:
			startTime = xsToNumber(xsArg(0));
			err	= FskMediaReaderStart(nmr->reader, &startTime, NULL);
			break;

		default:
			startTime = xsToNumber(xsArg(0));
			endTime = xsToNumber(xsArg(1));
			err	= FskMediaReaderStart(nmr->reader, &startTime, &endTime);
			break;
	}
	FskECMAScriptThrowIf(the, err);
}

void xs_mediareader_stop(xsMachine *the)
{
	xsNativeMediaReader nmr = xsGetHostData(xsThis);
	FskErr err = FskMediaReaderStop(nmr->reader);
	FskECMAScriptThrowIf(the, err);
}

void xs_mediareader_extract(xsMachine *the)
{
	xsNativeMediaReader nmr = xsGetHostData(xsThis);
	FskErr err;
	FskMediaReaderTrack readerTrack;
	UInt32 infoCount, i;
	FskMediaReaderSampleInfo info, ai;
	unsigned char *data;
	UInt32 dataSize = 0;

	xsVars(3);
#ifdef KPR_CONFIG
	xsResult = xsNewInstanceOf(xsGet(xsGlobal, xsID("fskMediaReaderData")));
#else
	xsResult = xsNewInstanceOf(xsGet(xsGet(xsGet(xsGlobal, xsID("Media")), xsID("Reader")), xsID("data")));
#endif
	err = FskMediaReaderExtract(nmr->reader, &readerTrack, &infoCount, &info, &data);
	if (kFskErrNone != err) {
		if (kFskErrEndOfFile == err) {
			xsResult = xsNull;
			return;
		}

		if (kFskErrNeedMoreTime == err)
			return;

		FskECMAScriptThrowIf(the, err);
	}

	xsVar(0) = xsNew1(xsGlobal, xsID("Array"), xsInteger(infoCount));

	for (i = 0, ai = info; i < infoCount; i++, ai++) {
#ifdef KPR_CONFIG
		xsVar(1) = xsNewInstanceOf(xsGet(xsGlobal, xsID("fskMediaReaderSampleInfo")));
#else
		xsVar(1) = xsNewInstanceOf(xsGet(xsGet(xsGet(xsGlobal, xsID("Media")), xsID("Reader")), xsID("sampleInfo")));
#endif
		xsSet(xsVar(1), xsID("samples"), xsInteger(ai->samples));
		xsSet(xsVar(1), xsID("sampleSize"), xsInteger(ai->sampleSize));
		xsSet(xsVar(1), xsID("flags"), xsInteger(ai->flags));
		xsSet(xsVar(1), xsID("decodeTime"), xsNumber((xsNumberValue)ai->decodeTime));
		if (ai->sampleDuration)
			xsSet(xsVar(1), xsID("sampleDuration"), xsInteger(ai->sampleDuration));
		if (-1 != ai->compositionTime)
			xsSet(xsVar(1), xsID("compositionTime"), xsNumber((xsNumberValue)ai->compositionTime));
		xsSet(xsVar(0), (xsIndex)i, xsVar(1));

		dataSize += ai->samples * ai->sampleSize;
	}

	xsSet(xsResult, xsID("info"), xsVar(0));
    FskMemPtrDispose((void *)info);

	xsVar(0) = xsGet(xsThis, xsID("tracks"));
	for (i = 0; true; i++) {
		xsNativeMediaReaderTrack nmrt;
		xsVar(1) = xsGet(xsVar(0), (xsIndex)i);
		if (!xsTest(xsVar(1)))
			FskECMAScriptThrowIf(the, -1);	//@@ can't find track

		nmrt = xsGetHostData(xsVar(1));
		if (nmrt->readerTrack == readerTrack) {
			xsSet(xsResult, xsID("track"), xsVar(1));
			break;
		}
	}

	xsVar(2) = xsNew1(xsGlobal, xsID_ArrayBuffer, xsInteger(dataSize));
	FskMemMove(xsToArrayBuffer(xsVar(2)), data, (SInt32)dataSize);
	xsSet(xsResult, xsID("buffer"), xsVar(2));
	FskMemPtrDispose(data);
}

void xs_mediareader_getMetadata(xsMachine *the)
{
	xsNativeMediaReader nmr = xsGetHostData(xsThis);
	FskMediaPropertyValueRecord meta;
	UInt32 index = (xsToInteger(xsArgc) <= 1) ? 1 : xsToInteger(xsArg(1));

	if (0 != FskMediaReaderGetMetadata(nmr->reader, xsToString(xsArg(0)), index, &meta, NULL))
		return;

	FskMediaGetPropertyXSHelper(the, &meta);
}

/*
	track functions
*/

void xs_mediareadertrack_destructor(void *data)
{
	FskMemPtrDispose(data);
}

void xs_mediareadertrack_set(xsMachine *the)
{
	xsNativeMediaReaderTrack nmrt = xsGetHostData(xsThis);
	FskErr err;
	UInt32 propertyID = xsToInteger(xsArg(0));
	UInt32 propertyDataType;
	Boolean get, set;

	err = FskMediaReaderTrackHasProperty(nmrt->readerTrack, propertyID, &get, &set, &propertyDataType);
	if ((kFskErrNone == err) && set)
		err = FskMediaSetPropertyXSHelper(the, propertyDataType, propertyID, 1, (FskMediaSetPropertyXSHelperProc)FskMediaReaderTrackSetProperty, nmrt->readerTrack);

	FskECMAScriptThrowIf(the, err);
}

void xs_mediareadertrack_get(xsMachine *the)
{
	xsNativeMediaReaderTrack nmrt = xsGetHostData(xsThis);
	FskMediaPropertyValueRecord property;

	if (kFskErrNone == FskMediaReaderTrackGetProperty(nmrt->readerTrack, xsToInteger(xsArg(0)), &property))
		FskMediaGetPropertyXSHelper(the, &property);
}

void xs_mediareadertrack_has(xsMachine *the)
{
	xsNativeMediaReaderTrack nmrt = xsGetHostData(xsThis);
	Boolean get, set;
	UInt32 dataType = 0;
	FskMediaReaderTrackHasProperty(nmrt->readerTrack, xsToInteger(xsArg(0)), &get, &set, &dataType);
	if (get || set)
		xsResult = xsInteger(dataType);
	else
		xsResult = xsUndefined;
}
/*
	local functions
*/

FskErr mediaReaderOpenFile(FskMediaSpooler spooler, UInt32 permissions)
{
	xsNativeMediaReader nmr = spooler->refcon;

	return FskFileOpen(nmr->path + 7, kFskFilePermissionReadOnly, &nmr->fref);
}

FskErr mediaReaderCloseFile(FskMediaSpooler spooler)
{
	xsNativeMediaReader nmr = spooler->refcon;

	FskFileClose(nmr->fref);
	nmr->fref = NULL;

	return kFskErrNone;
}

FskErr mediaReaderReadFile(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesRead)
{
	xsNativeMediaReader nmr = spooler->refcon;
	FskErr err;

	if (bytesToRead > nmr->readBufferSize) {
		FskMemPtrDisposeAt((void**)(void*)(&nmr->readBuffer));
		err = FskMemPtrNew(bytesToRead, (FskMemPtr *)&nmr->readBuffer);
		BAIL_IF_ERR(err);

		nmr->readBufferSize = bytesToRead;
	}

	FskFileSetPosition(nmr->fref, &position);

	err = FskFileRead(nmr->fref, bytesToRead, nmr->readBuffer, bytesRead);
	BAIL_IF_ERR(err);

	*buffer = nmr->readBuffer;

bail:
	return err;
}

FskErr mediaReaderGetSizeFile(FskMediaSpooler spooler, FskInt64 *size)
{
	xsNativeMediaReader nmr = spooler->refcon;
	return FskFileGetSize(nmr->fref, size);
}

#ifndef KPR_CONFIG
FskErr mediaReaderOpenStream(FskMediaSpooler spooler, UInt32 permissions)
{
	xsNativeMediaReader nmr = spooler->refcon;

	nmr->streamOffset = 0;

	return kFskErrNone;
}

FskErr mediaReaderCloseStream(FskMediaSpooler spooler)
{
	return kFskErrNone;
}

FskErr mediaReaderReadStream(FskMediaSpooler spooler, FskInt64 position, UInt32 bytesToRead, void **buffer, UInt32 *bytesReadOut)
{
	xsNativeMediaReader nmr = spooler->refcon;
	FskErr err = kFskErrNone;
	UInt32 bytesRead = 0;

	FskMemPtrDisposeAt((void **)&nmr->lastReadBuffer);

	xsBeginHost(nmr->the);

	xsVars(2);
	xsVar(0) = xsGet(nmr->obj, nmr->vm->id_stream);

	if (nmr->streamOffset != position) {
		xsCall0_noResult(xsVar(0), xsID("rewind"));
		xsCall1_noResult(xsVar(0), xsID("seek"), xsNumber((xsNumberValue)position));
		nmr->streamOffset = position;
	}

	{
	xsTry {
		xsVar(1) = xsCall1(xsVar(0), nmr->vm->id_readChunk, xsInteger(bytesToRead));

		if (xsTest(xsVar(1))) {
			xsSetHostDestructor(xsVar(1), NULL);
			bytesRead = xsToInteger(xsGet(xsVar(1), nmr->vm->id_length));
			nmr->lastReadBuffer = xsGetHostData(xsVar(1));
			*buffer = nmr->lastReadBuffer;
		}
		else
 			err = kFskErrNeedMoreTime;

		nmr->streamOffset += bytesRead;
	}
	xsCatch {
		err = xsToInteger(xsGet(xsException, xsID("code")));			//@@ check that it is a Fsk native error first
		if (0 == err)
			err = kFskErrOperationFailed;
	}
	}

	xsEndHost(nmr->the);

	if (bytesReadOut)
		*bytesReadOut = bytesRead;

	return err;
}

FskErr mediaReaderGetSizeStream(FskMediaSpooler spooler, FskInt64 *sizeOut)
{
	xsNativeMediaReader nmr = spooler->refcon;
	FskInt64 size = 0;

	xsBeginHost(nmr->the);

	xsResult = xsGet(nmr->obj, xsID("stream"));
	xsResult = xsCall0(xsResult, xsID("getBytesAvailable"));
	size = (FskInt64)xsToNumber(xsResult);		// to allow for > 2GB values

	xsEndHost(nmr->the);

	*sizeOut = size;

	return kFskErrNone;
}
#endif

FskErr mediaReaderEvent(FskMediaReader reader, void *refCon, FskEventCodeEnum eventCode, FskEvent event)
{
	xsNativeMediaReader nmr = refCon;
	xsMachine *the = nmr->the;
	char *handlerName;

	switch (eventCode) {
		case kFskEventMediaPlayerStateChange:
			handlerName = "onStateChange";
			break;
		case kFskEventMediaPlayerInitialize:
			handlerName = "onInitialize";
			updateTrackList(nmr);
			break;
		case kFskEventMediaReaderDataArrived:
			handlerName = "onMediaDataArrived";
			break;
        default:
            handlerName = NULL;
            break;
	}

	if (handlerName) {
		xsIndex id_handlerName = xsID(handlerName);
		xsBeginHost(the);

		if (xsHas(nmr->obj, id_handlerName)) {

#ifndef KPR_CONFIG
			if (event) {
				xsNativeEvent ne;
				FskECMAScript vm = xsGetContext(the);
				xsVars(1);
				xsVar(0) = xsNew2(xsGlobal, vm->id_Event, xsNull, xsInteger(0));
				ne = xsGetHostData(xsVar(0));
				ne->event = event;
				ne->needToDispose = false;
				xsCall1_noResult(nmr->obj, id_handlerName, xsVar(0));
			}
			else
#endif
				xsCall0_noResult(nmr->obj, xsID(handlerName));
		}

		xsEndHost(the);
	}

	return kFskErrNone;
}

void updateTrackList(xsNativeMediaReader nmr)
{
	FskErr err = kFskErrNone;

	xsBeginHost(nmr->the); {
		SInt32 index;

		xsVars(2);
		xsVar(0) = xsGet(nmr->obj, xsID("tracks"));
		xsSet(xsVar(0), xsID("length"), xsInteger(0));

		for (index = 0; true; index += 1) {
			FskMediaReaderTrack readerTrack;
			xsNativeMediaReaderTrack nmrt;

			if (kFskErrNone != FskMediaReaderGetTrack(nmr->reader, index, &readerTrack))
				break;

			err = FskMemPtrNewClear(sizeof(xsNativeMediaReaderTrackRecord), &nmrt);
			if (err) break;

			nmrt->reader = nmr->reader;
			nmrt->readerTrack = readerTrack;

#ifdef KPR_CONFIG
			xsVar(1) = xsNewInstanceOf(xsGet(xsGlobal, xsID("fskMediaReaderTrack")));
#else
			xsVar(1) = xsNewInstanceOf(xsGet(xsGet(xsGet(xsGlobal, xsID("Media")), xsID("Reader")), xsID("track")));
#endif
			xsSetHostData(xsVar(1), nmrt);
			xsCall1_noResult(xsVar(0), xsID("push"), xsVar(1));
		}
	}
	xsEndHost(nmr->the);
}

