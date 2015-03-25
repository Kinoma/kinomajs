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
#define __FSKMEDIAREADER_PRIV__
#include "FskEndian.h"
#include "FskFiles.h"
#include "FskImage.h"
#include "FskMediaReader.h"

static Boolean jpegWebcamCanHandle(const char *mimeType);
static FskErr jpegWebcamNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr jpegWebcamDispose(FskMediaReader reader, void *readerState);
static FskErr jpegWebcamGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr jpegWebcamStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr jpegWebcamStop(FskMediaReader reader, void *readerState);
static FskErr jpegWebcamExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr jpegWebcamGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
static FskErr jpegWebcamSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

static FskErr jpegWebcamGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegWebcamSetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegWebcamGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegWebcamGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegWebcamGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegWebcamGetBufferDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegWebcamReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord jpegWebcamProperties[] = {
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			jpegWebcamGetDuration,		jpegWebcamSetDuration},
	{kFskMediaPropertyTime,					kFskMediaPropertyTypeFloat,			jpegWebcamGetTime,			NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		jpegWebcamGetTimeScale,		NULL},
	{kFskMediaPropertyState,				kFskMediaPropertyTypeInteger,		jpegWebcamGetState,			NULL},
	{kFskMediaPropertyBufferDuration,		kFskMediaPropertyTypeInteger,		jpegWebcamGetBufferDuration,NULL},
	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	jpegWebcamReaderGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderDispatchRecord gMediaReaderJpegWebcam = {jpegWebcamCanHandle, jpegWebcamNew, jpegWebcamDispose, jpegWebcamGetTrack, jpegWebcamStart, jpegWebcamStop, jpegWebcamExtract, jpegWebcamGetMetadata, jpegWebcamProperties, jpegWebcamSniff};

static FskErr jpegWebcamTrackGetMediaType(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegWebcamTrackGetFormat(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegWebcamTrackGetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr jpegWebcamTrackGetDimensions(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord jpegWebcamVideoTrackProperties[] = {
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		jpegWebcamTrackGetMediaType,			NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		jpegWebcamTrackGetFormat,			NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		jpegWebcamTrackGetBitRate,			NULL},
	{kFskMediaPropertyDimensions,			kFskMediaPropertyTypeDimension,		jpegWebcamTrackGetDimensions,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderTrackDispatchRecord gMediaReaderJpegWebcamTrack = {jpegWebcamVideoTrackProperties};

Boolean jpegWebcamCanHandle(const char *mimeType)
{
	if (0 == FskStrCompare("image/x-kinoma-jpeg-webcam", mimeType))
		return true;

	return false;
}

enum {
	kMultiParseStateBoundary = 0,
	kMultiParseStateHeaders,
	kMultiParseStateImage
};
	
typedef struct {
	UInt32 parseState;
	UInt32 parseOffset;
	
	UInt32 imageStartOffset;
	UInt32 imageEndOffset;
} MultiPartParserRecord, *MultiPartParser;

typedef struct jpegWebcamRecord jpegWebcamRecord;
typedef struct jpegWebcamRecord *jpegWebcam;

 struct jpegWebcamRecord {
	FskMediaSpooler			spooler;
	Boolean					spoolerOpen;

	UInt32					atTime;

	FskInt64				fileOffset;

	UInt32					width;
	UInt32					height;
	UInt32					scale;
	UInt32					duration;
	Boolean					isMotionWebcam;
	UInt32					refreshIntervalSeconds;
	UInt32					metadataSizeMultiplier;

	FskMediaReader			reader;
	FskMediaReaderTrackRecord
							track;

	unsigned char			*buffer;
	unsigned char			*bufferPtr;
	UInt32					bytesInBuffer;
	UInt32					bufferSize;
	UInt32					frameNumber;

	char					*multipartBoundary;
	MultiPartParserRecord	parser;
	FskTimeCallBack			refreshCB;
};

static FskErr instantiateJpegWebcam(jpegWebcam state);
static FskErr doRead(jpegWebcam state, FskInt64 offset, UInt32 size, void *bufferIn, UInt32 *bytesRead);
static FskErr jpegWebcamInstantiate(jpegWebcam state);
static FskErr jpegWebcamSpoolerCallback(void *clientRefCon, UInt32 operation, void *param);
static Boolean jpegGetDimensions(unsigned char *data, SInt32 dataSize, SInt16 *width, SInt16 *height);
static FskErr refillBuffer(jpegWebcam state);
static FskErr parseReceivedMultipartData(jpegWebcam state, Boolean stopAtImageStart, UInt32 *dataOutSize, unsigned char **dataOut);
static void refreshTimerCB(FskTimeCallBack callback, const FskTime time, void *param);

 FskErr jpegWebcamNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	jpegWebcam state = NULL;
	char *refresh;

    BAIL_IF_NULL(spooler, err, kFskErrUnimplemented);
	
	err = FskMemPtrNewClear(sizeof(jpegWebcamRecord), &state);
	BAIL_IF_ERR(err);

	state->bufferSize = 64 * 1024L;
	err = FskMemPtrNewClear(state->bufferSize, &state->buffer);
	BAIL_IF_ERR(err);

	*readerState = state;			// must be set before anything that might issue a callback
	state->spooler = spooler;
	state->reader = reader;
	state->bufferPtr = state->buffer;

	if (spooler->doOpen) {
		err = (spooler->doOpen)(spooler, kFskFilePermissionReadOnly);
		BAIL_IF_ERR(err);

		state->spoolerOpen = true;
	}

	state->spooler->onSpoolerCallback = jpegWebcamSpoolerCallback;
	state->spooler->clientRefCon = state;

	state->scale = 1000;
	state->metadataSizeMultiplier = 1;

	state->track.state = state;
	state->track.dispatch = &gMediaReaderJpegWebcamTrack;

	if (NULL != (refresh = FskStrStr(uri, "refresh="))) {
		refresh += 8;
		state->refreshIntervalSeconds = FskStrToNum(refresh);
	}

	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);

	err = jpegWebcamInstantiate(state);
	if (err) {
		if (kFskErrNeedMoreTime == err)
			err = kFskErrNone;
		goto bail;
	}

bail:
	if ((kFskErrNone != err) && (NULL != state)) {
		jpegWebcamDispose(reader, state);
		state = NULL;
	}

	*readerState = state;

	return err;
}

FskErr jpegWebcamDispose(FskMediaReader reader, void *readerState)
{
	jpegWebcam state = readerState;

	if (NULL != state) {

		if (state->spoolerOpen && state->spooler->doClose)
			(state->spooler->doClose)(state->spooler);

		FskMemPtrDispose(state->buffer);
		FskMemPtrDispose(state->multipartBoundary);
		FskTimeCallbackDispose(state->refreshCB);
		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}

FskErr jpegWebcamGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track)
{
	jpegWebcam state = readerState;
	FskErr err = kFskErrNone;

	if (0 != index)
		return kFskErrInvalidParameter;

	*track = &state->track;

	return err;
}

FskErr jpegWebcamStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	jpegWebcam state = readerState;

	state->atTime = startTime ? (UInt32)*startTime : 0;
	state->fileOffset = 0;

	return kFskErrNone;
}

FskErr jpegWebcamStop(FskMediaReader reader, void *readerState)
{
	jpegWebcam state = readerState;

	state->frameNumber = 0;

	state->bufferPtr = state->buffer;
	state->bytesInBuffer = 0;

	state->parser.parseState = kMultiParseStateBoundary;
	state->parser.imageStartOffset = 0;
	state->parser.imageEndOffset = 0;

	return kFskErrNone;
}

FskErr jpegWebcamExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *trackOut, UInt32 *infoCountOut, FskMediaReaderSampleInfo *infoOut, unsigned char **dataOut)
{
	jpegWebcam state = readerState;
	FskErr err = kFskErrNone;
	FskSampleTime decodeTime = state->atTime;	
	FskMediaReaderSampleInfo info;
	unsigned char *jpg = NULL;
	UInt32 jpgSize = 0;

	if (!state->spoolerOpen)
		return kFskErrNeedMoreTime;

	// top off buffer and grab one-shot frames
	if (state->bytesInBuffer < state->bufferSize - 1024) {
		err = refillBuffer(state);

		// One-shot frame handled here
		if ((kFskErrEndOfFile == err) && !state->isMotionWebcam) {
			jpgSize = state->bytesInBuffer;
			err = FskMemPtrNew(jpgSize, (FskMemPtr*)&jpg);
            BAIL_IF_ERR(err);

			FskMemMove(jpg, state->buffer, jpgSize);

			state->bufferPtr = state->buffer;
			state->bytesInBuffer = 0;
			state->fileOffset = 0;

			if (state->spoolerOpen && state->spooler->doClose) {
				(state->spooler->doClose)(state->spooler);
				state->spoolerOpen = false;

				if (0 == state->refreshIntervalSeconds) {
					err = (state->spooler->doOpen)(state->spooler, kFskFilePermissionReadOnly);
					BAIL_IF_ERR(err);

					state->spoolerOpen = true;
				}
				else {
					FskTimeRecord refreshTime;
					FskTimeGetNow(&refreshTime);
					FskTimeAddSecs(&refreshTime, 5);
					state->refreshCB = FskTimeCallbackAddNew(&refreshTime, refreshTimerCB, state);
				}
			}
		}
        BAIL_IF_ERR(err);
	}

	// parse multipart data
	if (state->isMotionWebcam) {
		err = parseReceivedMultipartData(state, false, &jpgSize, &jpg);
		BAIL_IF_ERR(err);
	}

	if (NULL != jpg) {
		err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
		if (err) {
			FskMemPtrDispose(jpg);
			*dataOut = NULL;
			goto bail;
		}
		*dataOut = jpg;

		info->samples = 1;
		info->sampleSize = jpgSize;
		info->flags = kFskImageFrameTypeSync;
		info->decodeTime = decodeTime + (state->frameNumber++ * 33);		// @@
		info->compositionTime = -1;
		info->sampleDuration = 33;	//@@

		//fprintf(stderr, "sample extracted at time %ld\n", (UInt32)info->decodeTime);
		*infoCountOut = 1;
		*infoOut = info;
		*trackOut = &state->track;
	}
	else {
		err = kFskErrNeedMoreTime;
	}

bail:
	return err;
}

FskErr jpegWebcamGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	return kFskErrUnimplemented;
}

FskErr jpegWebcamSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	Boolean isWebcam = false;

	if (!uri)
		goto bail;

	if (0 != FskStrStr(uri, "kinoma.com/webcam"))
		isWebcam = true;

	// Detect motion webcams
	if (0 != FskStrStr(uri, "axis-cgi/mjpg") ||
		0 != FskStrStr(uri, "/mjpg/video.mjpg") ||
		0 != FskStrStr(uri, "/nphMotionJpeg") ||
		0 != FskStrStr(uri, "/cgi-bin/video.jpg") ||
		0 != FskStrStr(uri, "/mjpg/video.cgi")) {
		isWebcam = true;
	}
		
	if (0 != FskStrStr(uri, "axis-cgi/jpg") ||
		0 != FskStrStr(uri, "/still?cam=")) {
		isWebcam = true;
	}

	if (!isWebcam)
		goto bail;

	*mime = FskStrDoCopy("image/x-kinoma-jpeg-webcam");
	return 0;

bail:
	return kFskErrUnknownElement;
}

/*
	reader properties
*/

FskErr jpegWebcamGetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = -1.0;

	return kFskErrNone;
}

FskErr jpegWebcamSetDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	jpegWebcam state = readerState;

	state->duration = (UInt32)property->value.number;

	return kFskErrNone;
}

FskErr jpegWebcamGetTime(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	jpegWebcam state = readerState;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (double)state->atTime;

	return kFskErrNone;
}

FskErr jpegWebcamGetTimeScale(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	jpegWebcam state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->scale;

	return kFskErrNone;
}

FskErr jpegWebcamGetState(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	jpegWebcam state = readerState;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = state->reader->mediaState;

	return kFskErrNone;
}

FskErr jpegWebcamGetBufferDuration(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = 1;

	return kFskErrNone;
}

FskErr jpegWebcamReaderGetDLNASinks(void *readerState, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "image/x-kinoma-jpeg-webcam\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

/*
	track properties
*/

FskErr jpegWebcamTrackGetMediaType(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy("video");

	return kFskErrNone;
}

FskErr jpegWebcamTrackGetFormat(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy("image/jpeg");

	return kFskErrNone;
}

FskErr jpegWebcamTrackGetBitRate(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = 0;

	return kFskErrNone;
}

FskErr jpegWebcamTrackGetDimensions(void *readerState, void *trackIgnore, UInt32 propertyID, FskMediaPropertyValue property)
{
	jpegWebcam state = readerState;

	property->type = kFskMediaPropertyTypeDimension;
	property->value.dimension.width = state->width;
	property->value.dimension.height = state->height;

	return kFskErrNone;
}

/*
	local
*/

FskErr instantiateJpegWebcam(jpegWebcam state)
{
	FskErr err = kFskErrNone;
	FskInt64 totalSize;
	SInt16 width = 0, height = 0;
	UInt32 readSize;
	UInt8 *dontCareJPG;
	UInt32 dontCareSize;
	unsigned char *dataPtr, *data = 0;
	UInt32 bytesRead;

	#define kMetaSize (8 * 1024L)

	if (NULL != state->spooler->doGetSize) {
		err = (state->spooler->doGetSize)(state->spooler, &totalSize);
		BAIL_IF_ERR(err);
	}
	else
		totalSize = 0;

	// Try to get the dimensions
	readSize = kMetaSize * state->metadataSizeMultiplier;
	if (0 != totalSize && readSize > totalSize)
		readSize = (UInt32)totalSize;

	err = FskMemPtrNew(readSize, (FskMemPtr*)&data);
	BAIL_IF_ERR(err);

	err = doRead(state, 0, readSize, data, &bytesRead);
	if (0 == bytesRead) goto bail;

	dataPtr = data;
	readSize = bytesRead;

	if (state->isMotionWebcam) {
		FskMemMove(state->buffer, data, readSize);
		state->bufferPtr += readSize;

		err = parseReceivedMultipartData(state, true, &dontCareSize, &dontCareJPG);
		BAIL_IF_ERR(err);

		if (kMultiParseStateImage != state->parser.parseState) {
			goto bail;
		}

		dataPtr += state->parser.imageStartOffset;
		readSize -= state->parser.imageStartOffset;
	}

	jpegGetDimensions(dataPtr, readSize, &width, &height);

bail:
	FskMemPtrDispose(data);

	if (0 == width || 0 == height) {
		if (0 == err && ++state->metadataSizeMultiplier == 4)
			err = kFskErrBadData;
		else
			err = kFskErrNeedMoreTime;
	}
	else {
		state->width = width;
		state->height = height;
	}

	state->parser.parseState = kMultiParseStateBoundary;
	state->parser.imageStartOffset = state->parser.imageEndOffset = state->parser.parseOffset = 0;
	state->bufferPtr = state->buffer;

	return err;
}

FskErr doRead(jpegWebcam state, FskInt64 offset, UInt32 size, void *bufferIn, UInt32 *bytesReadOut)
{
	FskErr err = kFskErrNone;
	unsigned char *buffer = bufferIn;

	*bytesReadOut = 0;

	while (0 != size) {
		unsigned char *readBuffer;
		UInt32 bytesRead;

		err = FskMediaSpoolerRead(state->spooler, offset, size, &readBuffer, &bytesRead);
		if (err) return err;

		*bytesReadOut += bytesRead;

		FskMemMove(buffer, readBuffer, bytesRead);

		offset += bytesRead;
		buffer += bytesRead;
		size -= bytesRead;
	}

	return err;
}

FskErr jpegWebcamInstantiate(jpegWebcam state)
{
	FskErr err;

	err = instantiateJpegWebcam(state);
	BAIL_IF_ERR(err);

	(state->reader->doSetState)(state->reader, kFskMediaPlayerStateStopped);

bail:
	return err;
}

FskErr jpegWebcamSpoolerCallback(void *clientRefCon, UInt32 operation, void *param)
{
	jpegWebcam state = clientRefCon;
	FskErr err = kFskErrNone;
	FskHeaders *headers;
	char *mime;

	switch (operation) {
		case kFskMediaSpoolerOperationDataReady:
			if (state->reader->mediaState < kFskMediaPlayerStateStopped)
				err = jpegWebcamInstantiate(state);
			break;

		case kFskMediaSpoolerOperationGetHeaders:
			headers = param;
			mime = FskHeaderFind(kFskStrContentType, headers);
			if ((0 != mime) && (0 == FskStrCompareWithLength(mime, "multipart/x-mixed-replace", 25))) {
				char *boundary = FskStrStr(mime, "boundary=");
				if (NULL != boundary) {
					boundary += 9;
					if (0 != *boundary) {
						char *delimeter;

						state->multipartBoundary = FskStrDoCopy(boundary);
						state->isMotionWebcam = true;

						// Sometimes there's a comma after the boundary followed by the image/jpeg mime type
						delimeter = FskStrChr(state->multipartBoundary, ',');
						if (NULL != delimeter)
							*delimeter = 0;
					}
				}
			}
			break;
	}

	return err;
}

Boolean jpegGetDimensions(unsigned char *data, SInt32 dataSize, SInt16 *width, SInt16 *height)
{
	FskErr err;
	FskImageDecompress deco;
	FskMediaPropertyValueRecord value;

	err = FskImageDecompressNew(&deco, 0, "image/jpeg", NULL);
	if (err) return false;

	err = FskImageDecompressSetData(deco, data, dataSize);
	BAIL_IF_ERR(err);
	
	err = FskImageDecompressGetMetaData(deco, kFskImageDecompressMetaDataDimensions, 0, &value, NULL);
	BAIL_IF_ERR(err);

	*width = (SInt16)value.value.dimension.width;
	*height = (SInt16)value.value.dimension.height;

bail:
	FskImageDecompressDispose(deco);

	return kFskErrNone == err;
}

static FskErr refillBuffer(jpegWebcam state)
{
	FskErr err;
	UInt32 bytesToRead, bytesRead;
	unsigned char *buffer;

	bytesToRead = state->bufferSize - state->bytesInBuffer;

	err = FskMediaSpoolerRead(state->spooler, state->fileOffset, bytesToRead, &buffer, &bytesRead);
	BAIL_IF_ERR(err);

	state->fileOffset += bytesRead;
	state->bytesInBuffer += bytesRead;
	FskMemMove(state->bufferPtr, buffer, bytesRead);
	state->bufferPtr += bytesRead;

bail:
	return err;
}

FskErr parseReceivedMultipartData(jpegWebcam state, Boolean stopAtImageStart, UInt32 *dataOutSize, unsigned char **dataOut)
{
	FskErr err = kFskErrNone;
	UInt8 *p, *q, *start, *end, *boundaryStart, *imageEnd, saveCh;
	SInt32 boundaryLength;
	MultiPartParser parser = &state->parser;
	
	*dataOut = NULL;
	end = state->bufferPtr;
	p = start = state->buffer + parser->parseOffset;
	
	switch(parser->parseState) {
		case kMultiParseStateBoundary:
			// Look for the delimeter sequence and then skip past any trailing CR/LF characters
			while (p < end) {
				if (p[0] == state->multipartBoundary[0] && p[1] == state->multipartBoundary[1]) {
					boundaryLength = FskStrLen(state->multipartBoundary);
					
					// Bail if not enough data received
					if (p + boundaryLength + 5 >= end)
						break;
						
					q = p + boundaryLength;
					saveCh = *q;
					*q = 0;
					
					if (0 == FskStrCompare((const char*)p, (const char *)state->multipartBoundary)) {
						*q = saveCh;
						
						// Skip past boundary string delimeter
						p += boundaryLength;
						while (*p == 0x0D || *p == 0x0A)
							++p;
						parser->parseOffset = (UInt32)(p - state->buffer);
						++parser->parseState;
						break;
					}

					return kFskErrBadData;
				}
				++p;
			}
			// Fall through

		case kMultiParseStateHeaders:
			// We parse past any HTTP headers looking for the double CR/LF header termination sequence
			while (p + 4 < end) {
				if (p[0] == 0x0D && p[1] == 0x0A && p[2] == 0x0D && p[3] == 0x0A) {
					parser->parseOffset = (UInt32)(p - state->buffer) + 4;
					parser->imageStartOffset = parser->parseOffset;
					++parser->parseState;
					break;
				}
				else
				if (p[0] == 0x0A && p[1] == 0x0A) {	// Stupid webcams from Switzerland...
					parser->parseOffset = (UInt32)(p - state->buffer) + 2;
					parser->imageStartOffset = parser->parseOffset;
					++parser->parseState;
					break;
				}
				
				++p;
			}

			if (stopAtImageStart && (kMultiParseStateImage == parser->parseState))
				goto bail;

			break;
		case kMultiParseStateImage:
			// We parse looking for the boundary tag
			while (p < end) {
				if (p[0] == state->multipartBoundary[0] && p[1] == state->multipartBoundary[1]) {
					boundaryLength = FskStrLen(state->multipartBoundary);
					
					// Bail if not enough data received
					if (p + boundaryLength >= end)
						break;
						
					q = p + boundaryLength;
					saveCh = *q;
					*q = 0;
					
					if (0 == FskStrCompare((const char*)p, (const char *)state->multipartBoundary)) {
						UInt32 leftover, dataSize;
						unsigned char *end, *jpg;

						*q = saveCh;
						
						// Back up and consume and prepended CR/LF characters
						boundaryStart = p;
						--p;
						while (p > start && (*p == 0x0A || *p == 0x0D)) {
							--p;
						}
						imageEnd = ++p;
						
						parser->imageEndOffset = (UInt32)(imageEnd - state->buffer);
						
						parser->parseOffset = (UInt32)(boundaryStart - p);
						
						// We've got an image!
						dataSize = parser->imageEndOffset - parser->imageStartOffset;
						
						err = FskMemPtrNewFromData(dataSize, state->buffer + parser->imageStartOffset, (FskMemPtr*)&jpg);
						BAIL_IF_ERR(err);
						
						*dataOut = jpg;
						*dataOutSize = dataSize;

						// Slide the portion remaining to the head of the buffer
						end = state->buffer + parser->imageEndOffset;
						leftover = (UInt32)(state->bufferPtr - end); 
						FskMemMove(state->buffer, end, leftover);
						state->bufferPtr = state->buffer + leftover;
						state->bytesInBuffer = (UInt32)(state->bufferPtr - state->buffer);

						parser->parseState = kMultiParseStateBoundary;
						parser->imageStartOffset = 0;
						parser->imageEndOffset = 0;
						
						break;
					}
						
					*q = saveCh;
				}
				++p;
			}
			break;
	}
	
bail:
	return err;
}

void refreshTimerCB(FskTimeCallBack callback, const FskTime time, void *param)
{
	jpegWebcam state = (jpegWebcam)param;
	FskErr err;

	FskMediaReaderUsing(state->reader, true);

	FskTimeCallbackDispose(state->refreshCB);	// @@ is this necessary?
	state->refreshCB = NULL;

	err = (state->spooler->doOpen)(state->spooler, kFskFilePermissionReadOnly);
	BAIL_IF_ERR(err);

	state->spoolerOpen = true;

bail:
	FskMediaReaderUsing(state->reader, false);
}
