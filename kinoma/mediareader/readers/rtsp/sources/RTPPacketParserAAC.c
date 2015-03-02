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

#include "FskUtilities.h"
#include "FskEndian.h"
#include "FskInstrumentation.h"
#include "RTPPacketParser.h"

/*
	RFC-3640
+---------+-----------+-----------+---------------+
| RTP     | AU Header | Auxiliary | Access Unit  |
| Header  | Section   | Section   | Data Section |
+---------+-----------+-----------+---------------+
<----------RTP Packet Payload----------->

+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- .. -+-+-+-+-+-+-+-+-+-+
|AU-headers-length|AU-header|AU-header|     |AU-header|padding|
|                 |   (1)   |   (2)   |     |   (n)   | bits  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- .. -+-+-+-+-+-+-+-+-+-+
Figure 2: The AU Header Section

*/

typedef struct {
	UInt16 size;
	UInt16 index;			// index is valid only on first header
	UInt16 indexDelta;		// indexDelta is valid only on second and subsequent headers 

	UInt8 *auData;			// pointer to AU data corresponsing to this header
} AUHeaderRecord, *AUHeader;

typedef struct {
	RTPPacket packet;
	AUHeaderRecord *au;
} InterleavedPacketRecord, *InterleavedPacket;

// Private state
typedef struct {
	UInt16 profileLevelID;
	UInt16 sizeLength;
	UInt16 indexLength;
	UInt16 indexDeltaLength;

	char *config;		// audio config hex string
	UInt8 esds[32];
	UInt32 esdsSize;

	UInt32 mediaFormat;
	UInt32 sampleRate;
	UInt16 bitsPerSample;
	UInt16 nChannels;
	
	UInt32 lastTimeStamp;
	UInt32 lastSequenceNumber;

	Boolean needInterleaveCheck;
	InterleavedPacketRecord *interleavePacketBuffer;
	UInt16 interleavePacketBufferLength;
	UInt16 interleavePacketBufferCount;
	UInt16 interleaveIndexDelta;
	UInt16 interleaveGroupLength;
	UInt16 interleaveFramesPerPacket;
	
	RTPCompressedMediaFrame frames;
} AACPacketParserRecord, *AACPacketParser;

FskInstrumentedSimpleType(RTPPacketParserAAC, rtppacketparseraac);

void aacPacketParserInitialize(RTPPacketHandler handler);


static FskErr aacPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName);
static FskErr aacPacketParserNew(RTPPacketParser parser);
static FskErr aacPacketParserDispose(RTPPacketParser parser);
static FskErr aacPacketParserFlush(RTPPacketParser parser);
static FskErr aacPacketParserProcessPacket(RTPPacketParser parser, RTPPacket packet);
static FskErr aacPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet);
static FskErr aacPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize);

static void buildESDS(AACPacketParser aacPacketParser);

static FskErr _PushDataToLinkedFrames( int size, unsigned char *dataPtr, RTPCompressedMediaFrame *storedFrames );
static FskErr _GetLinkededFramesSize( RTPCompressedMediaFrame storedFrames, UInt32	*storedSizeOut );
static FskErr _MoveLinkedFramesDataToFrame( RTPCompressedMediaFrame storedFrames, RTPCompressedMediaFrame frame );
static FskErr _ClearLinkedFrames( RTPCompressedMediaFrame storedFrames );

static FskErr interleaveReset(AACPacketParser aacPacketParser);
static FskErr interleavePushPacket(AACPacketParser aacPacketParser, RTPPacket packet);
static FskErr interleavePullPacket(AACPacketParser aacPacketParser, RTPPacket packet);

#define OUTPUT_MULTIPLE_FRAMES 1

/* -----------------------------------------------------------------------*/

void aacPacketParserInitialize(RTPPacketHandler handler)
{
	handler->version = 1;

	handler->doCanHandle = aacPacketParserCanHandle;
	handler->doNew = aacPacketParserNew;
	handler->doDispose = aacPacketParserDispose;
	handler->doFlush = aacPacketParserFlush;
	handler->doProcessPacket = aacPacketParserProcessPacket;
	handler->doDisposePacket = aacPacketParserDisposePacket;
	handler->doGetInfo = aacPacketParserGetInfo;
}

/* -----------------------------------------------------------------------*/

FskErr aacPacketParserNew(RTPPacketParser parser)
{
	FskErr err = kFskErrNone;
	AACPacketParser aacPacketParser;
	SDPMediaDescription mediaDescription;

	err = FskMemPtrNewClear(sizeof(AACPacketParserRecord), &aacPacketParser);
	if (0 != err) goto bail;
	
	parser->mediaFormat = kRTPAudioFormatAAC;
	parser->handlerRefCon = aacPacketParser;

	mediaDescription = parser->mediaDescription;
	if (NULL != mediaDescription) {
		SDPAttribute attribute;

		// Grab what we need from the "fmtp" attribute
		attribute = SDPFindMediaAttribute(mediaDescription, "fmtp");
		if (NULL != attribute) {
			char *attr;

			if (NULL != copyAttributeValue(attribute->value, "profile-level-id", &attr)) {
				aacPacketParser->profileLevelID = (UInt16)FskStrToNum(attr);
				FskMemPtrDispose(attr);
			}
			if (NULL != copyAttributeValue(attribute->value, "sizelength", &attr)) {
				aacPacketParser->sizeLength = (UInt16)FskStrToNum(attr);
				FskMemPtrDispose(attr);
			}
			if (NULL != copyAttributeValue(attribute->value, "indexlength", &attr)) {
				aacPacketParser->indexLength = (UInt16)FskStrToNum(attr);
				FskMemPtrDispose(attr);
			}
			if (NULL != copyAttributeValue(attribute->value, "indexdeltalength", &attr)) {
				aacPacketParser->indexDeltaLength = (UInt16)FskStrToNum(attr);
				FskMemPtrDispose(attr);
			}
			if (NULL != copyAttributeValue(attribute->value, "config", &attr)) {
				aacPacketParser->config = FskStrDoCopy(attr);
				
				// Only look at the first two bytes in the config string
				if (FskStrLen(aacPacketParser->config) > 4) {
					aacPacketParser->config[4] = 0;
				}
				FskMemPtrDispose(attr);
			}
		}

		// Grab what we need from the "rtpmap" attribute
		attribute = SDPFindMediaAttribute(mediaDescription, "rtpmap");
		if (NULL != attribute) {
			char *value, *parts[3];
			UInt16 nParts = 2;
			value = FskStrDoCopy(attribute->value);
			splitToken(value, &nParts, ' ', &parts[0]);
			if (0 == FskStrCompareCaseInsensitiveWithLength(parts[1], "mpeg4-generic", FskStrLen("mpeg4-generic"))) {
				nParts = 3;
				splitToken(parts[1], &nParts, '/', &parts[0]);
				aacPacketParser->sampleRate = FskStrToNum(parts[1]);
				aacPacketParser->nChannels = (UInt16)FskStrToNum(parts[2]);
				aacPacketParser->bitsPerSample = 16;
				aacPacketParser->mediaFormat = kRTPAudioFormatAAC;
				parser->mediaFormat = aacPacketParser->mediaFormat;
			}
			FskMemPtrDispose(value);
		}
		
		if (aacPacketParser->config) {
			// allow ESDS config to override rtpmap values in case they don't match
		    const UInt32 sampleRates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000};
			UInt16 configValue = (UInt16)FskStrHexToNum((const char*)aacPacketParser->config, FskStrLen(aacPacketParser->config));
			UInt32 sampleRate;
			UInt16 channelCount;
			UInt8 config0, config1;

			config0 = (configValue & 0xFF00) >> 8;
			config1 = (configValue & 0x00FF);
			
			sampleRate = sampleRates[((config0 & 0x07) << 1) | ((config1 & 0x80) >> 7)];
			channelCount = (config1 & 0x78) >> 3;
			
			if (sampleRate != aacPacketParser->sampleRate) {
				aacPacketParser->sampleRate = sampleRate;
			}
			if (channelCount != aacPacketParser->nChannels) {
				aacPacketParser->nChannels = channelCount;
			}
		}
	}

	// Build an ESDS compatible with Kinoma Player / FhG decoder
	buildESDS(aacPacketParser);

	aacPacketParser->needInterleaveCheck = true;

bail:
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr aacPacketParserDispose(RTPPacketParser parser)
{
	AACPacketParser aacPacketParser = (AACPacketParser)parser->handlerRefCon;

	if (aacPacketParser) {
		UInt16 i;

		aacPacketParserFlush(parser);
		for (i = 0; i < aacPacketParser->interleavePacketBufferLength; ++i)
			FskMemPtrDispose(aacPacketParser->interleavePacketBuffer[i].au);
		FskMemPtrDispose(aacPacketParser->interleavePacketBuffer);
		FskMemPtrDispose(aacPacketParser->config);
		FskMemPtrDispose(aacPacketParser);
	}

	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr aacPacketParserFlush(RTPPacketParser parser)
{
	AACPacketParser aacPacketParser = (AACPacketParser)parser->handlerRefCon;
	
	if(aacPacketParser) {
		_ClearLinkedFrames( aacPacketParser->frames );
		interleaveReset(aacPacketParser);
		aacPacketParser->frames = NULL;	
		aacPacketParser->lastTimeStamp = 0;
		aacPacketParser->lastSequenceNumber = 0;
	}
	
	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr aacPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName)
{
	Boolean canHandle = false;

	// We handle mpeg4-generic encoding
	if (0 != FskStrCompareCaseInsensitive("mpeg4-generic", encodingName)) {
		goto bail;
	}

	if (NULL != mediaDescription) {
		SDPAttribute attribute;
		attribute = SDPFindMediaAttribute(mediaDescription, "fmtp");
		if (NULL != attribute) {
			char *attr;
	
			// We only handle the 'AAC-hbr' (high bit rate) mode
			if (0 != copyAttributeValue(attribute->value, "mode", &attr)) {
				if (0 == FskStrCompareCaseInsensitive("AAC-hbr", attr)) {
					canHandle = true;
				}
				FskMemPtrDispose(attr);
			}
				
			// The sizeLength, indexLength, and indexDeltaLength must all be present non-zero (p. 25)
			if (canHandle) {
				UInt16 sizeLength = 0, indexLength = 0, indexDeltaLength = 0;
				if (NULL != copyAttributeValue(attribute->value, "sizelength", &attr)) {
					sizeLength = (UInt16)FskStrToNum(attr);
					FskMemPtrDispose(attr);
				}
				if (NULL != copyAttributeValue(attribute->value, "indexlength", &attr)) {
					indexLength = (UInt16)FskStrToNum(attr);
					FskMemPtrDispose(attr);
				}
				if (NULL != copyAttributeValue(attribute->value, "indexdeltalength", &attr)) {
					indexDeltaLength = (UInt16)FskStrToNum(attr);
					FskMemPtrDispose(attr);
				}
				if (!sizeLength || !indexLength || !indexDeltaLength)
					canHandle = false;
			}
			
			// A config string must be present
			if (canHandle) {
				if (NULL != copyAttributeValue(attribute->value, "config", &attr)) {
					FskMemPtrDispose(attr);
				}
				else
					canHandle = false;
			}
		}
	}

bail:
	return canHandle ? 0 : kFskErrRTSPPacketParserUnsupportedFormat;
}

/* -----------------------------------------------------------------------*/

FskErr aacPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize)
{
	AACPacketParser aacPacketParser = (AACPacketParser)parser->handlerRefCon;
	FskErr err = 0;

	switch(selector) {
		case kRTPPacketParserSelectorESDS:
			*(UInt8**)info = aacPacketParser->esds;
			if (infoSize)
				*infoSize = aacPacketParser->esdsSize;
			break;

		case kRTPPacketParserSelectorRTPTimeScale:
		case kRTPPacketParserSelectorAudioSampleRate:
			*(UInt32*)info = aacPacketParser->sampleRate;
			break;
		case kRTPPacketParserSelectorAudioChannels:
			*(UInt32*)info = aacPacketParser->nChannels;
			break;
		case kRTPPacketParserSelectorAudioBitsPerSample:
			*(UInt32*)info = aacPacketParser->bitsPerSample;
			break;
		case kRTPPacketParserSelectorMediaFormat:
			*(UInt32*)info = aacPacketParser->mediaFormat;
			break;

		default:
			err = kFskErrInvalidParameter;
			break;
	}
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr aacPacketParserProcessPacket(RTPPacketParser parser, RTPPacket rtpHeader)
{
	AACPacketParser aacPacketParser = (AACPacketParser)parser->handlerRefCon;
	UInt16 AUHeadersLength, nAUHeaders = 0, nHeaders;
	SInt16 AUHeaderBits;
	AUHeader AUHeaders = 0, header;
	bitStreamRecord bits;
	UInt8 *auData;
	RTPCompressedMediaFrame frame;
	UInt8 *rtpDataWalker;
	UInt8 *frameData;
	UInt32 totalSize, storedSize;
	FskErr err = 0;
	RTPCompressedMediaFrame frameToDispose = 0;
	RTPCompressedMediaFrame	outFrame				= NULL;
	Boolean					frameChanged			= false;
	Boolean					isLastPacketOfAFrame	= false;

	frameChanged		 = aacPacketParser->lastTimeStamp != rtpHeader->timestamp;
	isLastPacketOfAFrame = rtpHeader->marker;
	aacPacketParser->lastTimeStamp   = rtpHeader->timestamp;

	// We don't support splitting AAC frames across packets
	if (!isLastPacketOfAFrame) {
		err = kFskErrRTSPPacketParserUnsupportedFormat;
		goto bail;
	}
	
	// Don't accept late or repeated frames when processing interleaved data
	if (rtpHeader->sequenceNumber < aacPacketParser->lastSequenceNumber && 0 != aacPacketParser->interleaveGroupLength) {
		err = kFskErrRTSPBadPacket;
		goto bail;
	}
	aacPacketParser->lastSequenceNumber = rtpHeader->sequenceNumber;

	// Short circuit all this nonsense if we know this packet represents a complete frame
	if (frameChanged && (NULL == aacPacketParser->frames)) {
		rtpDataWalker = rtpHeader->data;
		goto AUDecode;
	}

	err = _GetLinkededFramesSize( aacPacketParser->frames, &storedSize );
	if( err != 0 ) goto bail;

	if( frameChanged && storedSize != 0 )
	{
		err = FskMemPtrNewClear( storedSize + sizeof(RTPCompressedMediaFrameRecord), &outFrame );
		if( err != 0 )	goto bail;

		err = _MoveLinkedFramesDataToFrame( aacPacketParser->frames, outFrame );
		if( err != 0 )	goto bail;
		aacPacketParser->frames = NULL;

		//put this frame on top of output frame link list
		rtpHeader->frames	= outFrame;
		rtpHeader->dataSize = outFrame->length;
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserAACTypeInstrumentation, "						probably pcket loss, output from stored data: %d", rtpHeader->dataSize);

		{
			//***if this's the last packet of a frame
			// we'll have to wait next call to output it, not very nice...
			// waiting for multiple frame output support
			// --bryan 6/30/2005
#if OUTPUT_MULTIPLE_FRAMES
			FskInstrumentedTypePrintfDebug(&gRTPPacketParserAACTypeInstrumentation, "************************attaching extra!!!");
			err = _PushDataToLinkedFrames( rtpHeader->dataSize, rtpHeader->data, &rtpHeader->frames );
			goto completeFrame;
#endif			
			err = _PushDataToLinkedFrames( rtpHeader->dataSize, rtpHeader->data, &aacPacketParser->frames );
			goto bail;
		}
	}

	err = _PushDataToLinkedFrames( rtpHeader->dataSize, rtpHeader->data, &aacPacketParser->frames );
	if( err != 0 )	goto bail;

	err = _GetLinkededFramesSize( aacPacketParser->frames, &storedSize );
	if( err != 0 ) goto bail;

	err = FskMemPtrNewClear( storedSize + sizeof(RTPCompressedMediaFrameRecord), &outFrame );
	if( err != 0 ) goto bail;
	
	err = _MoveLinkedFramesDataToFrame( aacPacketParser->frames, outFrame );
	if( err != 0 ) goto bail;
	aacPacketParser->frames = NULL;
	
	rtpHeader->frames	= outFrame;
	rtpHeader->dataSize	= outFrame->length;
	
completeFrame:
	frameToDispose = rtpHeader->frames;
	rtpHeader->frames = NULL;
	rtpHeader->dataSize = 0;
	
	rtpDataWalker = (UInt8*)(frameToDispose + 1);

AUDecode:
	// Required - two byte length in bits of the headers which follow excluding padding bits
	AUHeadersLength = FskEndianU16_BtoN(*(UInt16*)rtpDataWalker);
	rtpDataWalker += 2;
	auData = rtpDataWalker + ((AUHeadersLength + 7) / 8);

	// Calculate the number of headers
	// Note this implementation assumes that only the AU-size, and AU-Index / AU-Index-delta fields are present
	AUHeaderBits = AUHeadersLength - (aacPacketParser->sizeLength + aacPacketParser->indexLength);
    if (AUHeaderBits >= 0 && (aacPacketParser->sizeLength + aacPacketParser->indexDeltaLength) > 0) {
		nAUHeaders = AUHeaderBits/(aacPacketParser->sizeLength + aacPacketParser->indexDeltaLength) + 1;
    }
	if (0 == nAUHeaders) goto bail;

	// Determine if this stream has interleaved access units
	if (aacPacketParser->needInterleaveCheck) {
		aacPacketParser->needInterleaveCheck = false;
		if (nAUHeaders > 1 && 0 != aacPacketParser->indexDeltaLength) {
			UInt16 i;

			bits.position = 0;
			bits.data = rtpDataWalker;

			// skip the first AU
			getBits(&bits, aacPacketParser->sizeLength);
			getBits(&bits, aacPacketParser->indexLength);

			// Grab the index delta from the second AU
			getBits(&bits, aacPacketParser->sizeLength);
			aacPacketParser->interleaveIndexDelta = (UInt16)getBits(&bits, aacPacketParser->indexDeltaLength);

			// A non-zero index delta indicates interleaved access units
			aacPacketParser->interleaveFramesPerPacket = nAUHeaders;
			aacPacketParser->interleaveGroupLength = aacPacketParser->interleaveFramesPerPacket * (aacPacketParser->interleaveIndexDelta + 1);
			aacPacketParser->interleavePacketBufferLength = aacPacketParser->interleaveGroupLength / aacPacketParser->interleaveFramesPerPacket;
	
			err = FskMemPtrNew(aacPacketParser->interleavePacketBufferLength * sizeof(InterleavedPacketRecord), (FskMemPtr*)&aacPacketParser->interleavePacketBuffer);
			if (0 != err) goto bail;

			for (i = 0; i < aacPacketParser->interleavePacketBufferLength; ++i) {
				err = FskMemPtrNew(nAUHeaders * sizeof(AUHeaderRecord), (FskMemPtr*)&aacPacketParser->interleavePacketBuffer[i].au);
				if (0 != err) goto bail;
			}

			aacPacketParser->interleavePacketBufferCount = 0;
		}
	}

	// Handle interleaved frames separately
	if (0 != aacPacketParser->interleaveGroupLength) {
		err = interleavePushPacket(aacPacketParser, rtpHeader);
		if (0 != err) goto bail;
		err = interleavePullPacket(aacPacketParser, rtpHeader);

		if (outFrame == frameToDispose)
			frameToDispose = NULL;
		goto bail;
	}

	// Allocate the headers
	err = FskMemPtrNew(sizeof(AUHeaderRecord) * nAUHeaders, &AUHeaders);
	if (0 != err) goto bail;

	// Populate the AU headers
	// The first header has the index value while subsequent headers have the index delta
	bits.data = rtpDataWalker;
	bits.position = 0;
	header = AUHeaders;
	header->size = (UInt16)getBits(&bits, aacPacketParser->sizeLength);
	header->index = (UInt16)getBits(&bits, aacPacketParser->indexLength);
	header->auData = auData;

	// For high bit rate AAC, each AU-index field must be zero
	if (0 != header->index) {
		err = kFskErrRTSPPacketParserUnsupportedFormat;
		goto bail;
	}

	// If the index delta value is non-zero then interleaving is applied to the AU's (yuk)
	nHeaders = nAUHeaders - 1;
	auData += header->size;
	totalSize = header->size;
	++header;

	while (nHeaders--) {
		header->size = (UInt16)getBits(&bits, aacPacketParser->sizeLength);
		header->indexDelta = (UInt16)getBits(&bits, aacPacketParser->indexDeltaLength);

		header->auData = auData;
		auData += header->size;
		totalSize += header->size;
		++header;
	}
	
	// Allocate a single frame to contain all the AU data
	err = FskMemPtrNew(sizeof(RTPCompressedMediaFrameRecord) + totalSize, &frame);
	if (0 == err) {
		UInt32 *pSampleSize;
		frame->next = NULL;
		frame->length = totalSize;
		rtpHeader->frames = frame;
		rtpHeader->totalSamples = nAUHeaders * 1024;
		
		nHeaders = nAUHeaders;
		
		err = FskMemPtrNew(sizeof(UInt32) * (nHeaders + 1), (FskMemPtr*)&frame->sampleSizes);
		if (0 != err) goto bail;
		
		pSampleSize = &frame->sampleSizes[0];
		header = AUHeaders;
		frameData = (UInt8*)(frame + 1);
		while (nHeaders--) {
			FskMemMove(frameData, header->auData, header->size);
			frameData += header->size;
			*pSampleSize++ = header->size;
			++header;
		}
		*pSampleSize = 0;
	}
	
bail:
	if( err != 0 && outFrame != NULL )
		FskMemPtrDispose(outFrame);

	FskMemPtrDispose(frameToDispose);
	FskMemPtrDispose(AUHeaders);
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr aacPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet)
{
	return 0;
}

/* -----------------------------------------------------------------------*/

void buildESDS(AACPacketParser aacPacketParser)
{
	UInt8 *esdsPtr = aacPacketParser->esds;
	UInt16 configValue;

	*esdsPtr++ = 0x4;		// <DecoderConfigDesciptorTag>
	*esdsPtr++ = 0;			// pad
	*esdsPtr++ = 0x40;		// <objectTypeIndication>

	*esdsPtr++ = 0x14;		// <streamType>, <upstream>, <reserved> 
	esdsPtr += 3;			// buffersize DB
	esdsPtr += 4;			// max bitrate
	esdsPtr += 4;			// avg bitrate

	*esdsPtr++ = 0x05;		// <DecoderSpecificInfoTag>
	*esdsPtr++ = 0;			// pad

	configValue = (UInt16)FskStrHexToNum((const char*)aacPacketParser->config, FskStrLen(aacPacketParser->config));
	*esdsPtr++ = (configValue >> 8) & 0xFF;	// config bytes
	*esdsPtr++ = configValue & 0xFF;

	aacPacketParser->esdsSize = (UInt32)(esdsPtr - aacPacketParser->esds);
}

/* -----------------------------------------------------------------------*/

FskErr _PushDataToLinkedFrames( int size, unsigned char *dataPtr, RTPCompressedMediaFrame *storedFrames )
{
	RTPCompressedMediaFrame	frame = NULL;
	UInt32					err   = 0;

	err	= FskMemPtrNewClear( size + sizeof(RTPCompressedMediaFrameRecord), &frame);
	if (0 != err) goto bail;

	frame->next		= NULL;
	frame->length	= size;
	FskMemMove( frame + 1, dataPtr, size );		

	// Add the current buffer to our link list
	if( (*storedFrames) == NULL )
		*storedFrames = frame;
	else 
	{
		RTPCompressedMediaFrame  currentFrames = *storedFrames;

		while( currentFrames->next != NULL )
			currentFrames = currentFrames->next;

		currentFrames->next = frame;
	}

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserAACTypeInstrumentation, "						append: %d", size);

bail:
	if( err != 0  && frame != NULL )
		FskMemPtrDispose(frame);

	return err;
}

FskErr _GetLinkededFramesSize( RTPCompressedMediaFrame storedFrames, UInt32	*storedSizeOut )
{
	UInt32	storedSize		= 0;
	
	// if we have saved data, get it and prepend it to the current data
	while(storedFrames)
	{
		storedSize += storedFrames->length;
		storedFrames = storedFrames->next;
	}

	*storedSizeOut = storedSize;

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserAACTypeInstrumentation, "						_GetLinkededFramesSize: %d", storedSize);

	return 0;
}


FskErr _MoveLinkedFramesDataToFrame( RTPCompressedMediaFrame storedFrames, RTPCompressedMediaFrame frame )
{
	int				copiedSize = 0;
	unsigned char	*pData = NULL;
	
	frame->next	= NULL;
	
	// Walk our list again and really prepend the data
	pData = (unsigned char *)( frame + 1 ); // Move pass the header
	
	while(storedFrames)
	{
		RTPCompressedMediaFrame currStoredFrames = storedFrames;
		UInt32					currStoredSize	 = storedFrames->length;
		
		FskMemMove(pData, storedFrames+1, currStoredSize);
		storedFrames = storedFrames->next;
		
		// Remove our stored data from our list
		FskMemPtrDispose(currStoredFrames);
		currStoredFrames = NULL;
		
		// increment our frame ptr
		pData += currStoredSize;
		copiedSize += currStoredSize;
	}

	frame->length	= copiedSize;

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserAACTypeInstrumentation, "						_MoveLinkedFramesDataToFrame: %d", frame->length);

	return 0;
}

FskErr _ClearLinkedFrames( RTPCompressedMediaFrame storedFrames )
{
	while(storedFrames)
	{
		RTPCompressedMediaFrame currStoredFrames = storedFrames;
		
			storedFrames = storedFrames->next;
		
		// Remove our stored data from our list
		FskMemPtrDispose(currStoredFrames);
		currStoredFrames = NULL;
	}

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserAACTypeInstrumentation, "						_ClearLinkedFrames");

	return 0;
}

FskErr interleaveFlushToIndex(AACPacketParser aacPacketParser, UInt16 index)
{
	UInt16 i;
	for (i = 0; i < index; ++i) {
		RTPPacket packet = aacPacketParser->interleavePacketBuffer[i].packet;
		FskMemPtrDispose(packet->data);
		FskMemPtrDispose(packet);
	}

	return 0;
}

FskErr interleaveReset(AACPacketParser aacPacketParser)
{
	FskErr err = 0;

	interleaveFlushToIndex(aacPacketParser, aacPacketParser->interleavePacketBufferCount);
	aacPacketParser->interleavePacketBufferCount = 0;

	return err;
}

FskErr interleavePushPacket(AACPacketParser aacPacketParser, RTPPacket packet)
{
	FskErr err;
	RTPPacket packetCopy = 0;
	Boolean gap = false;

	err = FskMemPtrNewFromData(sizeof(RTPPacketRecord), packet, &packetCopy);
	if (0 != err) goto bail;

	err = FskMemPtrNewFromData(packet->dataSize, packet->data, &packetCopy->data);
	if (0 != err) goto bail;

	if (0 == aacPacketParser->interleavePacketBufferCount) {
		aacPacketParser->interleavePacketBuffer[aacPacketParser->interleavePacketBufferCount++].packet = packetCopy;
	}
	else {
		RTPPacket lastPacket = aacPacketParser->interleavePacketBuffer[aacPacketParser->interleavePacketBufferCount-1].packet;
		if (lastPacket->sequenceNumber == packetCopy->sequenceNumber - 1) {
			UInt32 timeDelta;
			aacPacketParser->interleavePacketBuffer[aacPacketParser->interleavePacketBufferCount++].packet = packetCopy;
			timeDelta = packetCopy->timestamp - lastPacket->timestamp;
			if (timeDelta != 1024 && timeDelta != 2048) {
				gap = true;
			}
		}
		else {
			// flush - packet loss
			interleaveFlushToIndex(aacPacketParser, aacPacketParser->interleavePacketBufferCount);
			aacPacketParser->interleavePacketBufferCount = 0;
			aacPacketParser->interleavePacketBuffer[aacPacketParser->interleavePacketBufferCount++].packet = packetCopy;
		}
	}

	// Detect and fix the situation where we may have assembled a group that crosses multiple interleaved groups
	if (gap) {
		InterleavedPacket dst, src;
		UInt16 startGroupPacketIndex = aacPacketParser->interleavePacketBufferCount - 1;
		interleaveFlushToIndex(aacPacketParser, startGroupPacketIndex);
		src = &aacPacketParser->interleavePacketBuffer[startGroupPacketIndex];
		dst = &aacPacketParser->interleavePacketBuffer[0];
		dst->packet = src->packet;
		aacPacketParser->interleavePacketBufferCount = 1;
	}

#if 0
	if (aacPacketParser->interleavePacketBufferCount > 1) {
		UInt16 i, j;
		for (i = 0; i < aacPacketParser->interleavePacketBufferCount - 1; ++i) {
			UInt32 gap;
			RTPPacket p0 = aacPacketParser->interleavePacketBuffer[i].packet;
			RTPPacket p1 = aacPacketParser->interleavePacketBuffer[i+1].packet;
			gap = p1->timestamp - p0->timestamp;
			if (gap != 1024 && gap != 2048) {
				InterleavedPacket dst, src;

				++i;	// Point at the first packet in the new group

				// Slide the new group to the head of the interleave
				interleaveFlushToIndex(aacPacketParser, i);
				src = &aacPacketParser->interleavePacketBuffer[i];
				dst = &aacPacketParser->interleavePacketBuffer[0];
				for (j = i; j < aacPacketParser->interleavePacketBufferCount; ++j) {
					dst->packet = src->packet;
					++src;
					++dst;
				}
				aacPacketParser->interleavePacketBufferCount = aacPacketParser->interleavePacketBufferCount - i;
				break;
			}
		}
	}
#endif

bail:
	return err;
}

FskErr interleavePullPacket(AACPacketParser aacPacketParser, RTPPacket packet)
{
	FskErr err = kFskErrRTSPBadPacket;

	// If we've accumulated enough packets to compose one interleaved group, emit that group here
	if (aacPacketParser->interleavePacketBufferCount == aacPacketParser->interleavePacketBufferLength) {
		UInt8 *auData, *rtpDataWalker, *frameData;
		RTPCompressedMediaFrameRecord *frame;
		UInt16 i, j;
		bitStreamRecord bits;
		UInt32 *pSampleSize;
		UInt16 totalSize = 0;

		// Populate the AU records
		for (i = 0; i < aacPacketParser->interleavePacketBufferCount; ++i) {
			UInt16 AUHeadersLength, nHeaders;
			RTPPacket thisPacket = aacPacketParser->interleavePacketBuffer[i].packet;
			AUHeader header = &aacPacketParser->interleavePacketBuffer[i].au[0];

			rtpDataWalker = thisPacket->data;
			AUHeadersLength = FskEndianU16_BtoN(*(UInt16*)rtpDataWalker);
			rtpDataWalker += 2;
			auData = rtpDataWalker + ((AUHeadersLength + 7) / 8);
			bits.data = rtpDataWalker;
			bits.position = 0;

			header->size = (UInt16)getBits(&bits, aacPacketParser->sizeLength);
			header->index = (UInt16)getBits(&bits, aacPacketParser->indexLength);
			header->auData = auData;

			nHeaders = aacPacketParser->interleaveFramesPerPacket - 1;
			auData += header->size;
			totalSize += header->size;
			++header;

			while (nHeaders--) {
				header->size = (UInt16)getBits(&bits, aacPacketParser->sizeLength);
				header->indexDelta = (UInt16)getBits(&bits, aacPacketParser->indexDeltaLength);

				header->auData = auData;
				auData += header->size;
				totalSize += header->size;
				++header;
			}
		}

		// Allocate the media frames
		err = FskMemPtrNew(sizeof(RTPCompressedMediaFrameRecord) + totalSize, &frame);
		if (0 != err) goto bail;

		err = FskMemPtrNew(sizeof(UInt32) * (aacPacketParser->interleaveGroupLength + 1), &frame->sampleSizes);
		if (0 != err) goto bail;

		// Populate the output packet
		packet->timestamp = aacPacketParser->interleavePacketBuffer[0].packet->timestamp;
		packet->sequenceNumber = aacPacketParser->interleavePacketBuffer[0].packet->sequenceNumber;
		packet->frames = frame;
		packet->totalSamples = 1024 * aacPacketParser->interleaveGroupLength;
		frame->next = NULL;
		frame->length = totalSize;
		pSampleSize = &frame->sampleSizes[0];
		frameData = (UInt8*)(frame + 1);

		// Walk the interleaved group, de-interleaving the frames into the output
		for (i = 0; i < aacPacketParser->interleaveFramesPerPacket; ++i) {
			for (j = 0; j < aacPacketParser->interleavePacketBufferCount; ++j) {
				AUHeader header = &aacPacketParser->interleavePacketBuffer[j].au[i];

				FskMemMove(frameData, header->auData, header->size);
				frameData += header->size;
				*pSampleSize++ = header->size;
			}
		}
		*pSampleSize = 0;

		// We've emitted the group, so clear our state to receive the next group
		interleaveReset(aacPacketParser);

		err = 0;
	}

bail:
	return err;
}
