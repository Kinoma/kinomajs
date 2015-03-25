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

/*
	RFC-3016
*/

#include "FskUtilities.h"
#include "FskEndian.h"
#include "RTPPacketParser.h"
#include "QTReader.h"

#define OUTPUT_MULTIPLE_FRAMES 1

// Private state
typedef struct {

	UInt8 *esds;
	UInt32 esdsSize;

	UInt32 mediaFormat;
	UInt32 width;
	UInt32 height;
	UInt32 timeScale;

	UInt32 lastTimeStamp;
	Boolean lastPacketIsVOG;
	//	UInt32 state;
	RTPCompressedMediaFrame frames;

} MP4VPacketParser;

FskInstrumentedSimpleType(RTPPacketParserMP4V, rtppacketparsermp4v);

void mp4vPacketParserInitialize(RTPPacketHandler handler);


static FskErr mp4vPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName);
static FskErr mp4vPacketParserNew(RTPPacketParser parser);
static FskErr mp4vPacketParserDispose(RTPPacketParser parser);
static FskErr mp4vPacketParserProcessPacket(RTPPacketParser parser, RTPPacket packet);
static FskErr mp4vPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet);
static FskErr mp4vPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize);
FskErr mp4vPacketParserFlush(RTPPacketParser parser);

static Boolean configToESDS(char *config, UInt8 **esds, UInt32 *esdsSize, UInt32 *width, UInt32 *height);

//static Boolean HasMultipleVOP( long dataSize, UInt8 *dataPtr );

/* -----------------------------------------------------------------------*/

void mp4vPacketParserInitialize(RTPPacketHandler handler)
{
	handler->version = 1;

	handler->doCanHandle = mp4vPacketParserCanHandle;
	handler->doNew = mp4vPacketParserNew;
	handler->doDispose = mp4vPacketParserDispose;
	handler->doProcessPacket = mp4vPacketParserProcessPacket;
	handler->doDisposePacket = mp4vPacketParserDisposePacket;
	handler->doGetInfo = mp4vPacketParserGetInfo;
	handler->doFlush = mp4vPacketParserFlush;
}

/* -----------------------------------------------------------------------*/

FskErr mp4vPacketParserNew(RTPPacketParser parser)
{
	FskErr err = kFskErrNone;
	MP4VPacketParser *mp4vPacketParserPtr = NULL;
	SDPMediaDescription mediaDescription;

	err = FskMemPtrNewClear(sizeof(MP4VPacketParser), &mp4vPacketParserPtr);
	if (0 != err) goto bail;
	
	parser->mediaFormat = kRTPVideoFormatMPEG4;
	parser->handlerRefCon = mp4vPacketParserPtr;

	mp4vPacketParserPtr->mediaFormat = kRTPVideoFormatMPEG4;
	mp4vPacketParserPtr->timeScale = 90000;
	mp4vPacketParserPtr->lastTimeStamp = 0;
	mp4vPacketParserPtr->lastPacketIsVOG = false;

	mediaDescription = parser->mediaDescription;
	if (NULL != mediaDescription) {
		SDPAttribute attribute;

		// Grab what we need from the "fmtp" attribute
		attribute = SDPFindMediaAttribute(mediaDescription, "fmtp");
		if (NULL != attribute) {
			char *value;

			if (0 != copyAttributeValue(attribute->value, "config", &value)) {
				configToESDS(value, &mp4vPacketParserPtr->esds, &mp4vPacketParserPtr->esdsSize, &mp4vPacketParserPtr->width, &mp4vPacketParserPtr->height);
				FskMemPtrDispose(value);
			}
		}

		// Grab what we need from the "rtpmap" attribute
		attribute = SDPFindMediaAttribute(mediaDescription, "rtpmap");
		if (NULL != attribute) {
			char *value, *parts[3];
			UInt16 nParts = 2;
			value = FskStrDoCopy(attribute->value);
			splitToken(value, &nParts, ' ', &parts[0]);
			if (0 == FskStrCompareCaseInsensitiveWithLength(parts[1], "MP4V-ES", FskStrLen("MP4V-ES"))) {
				nParts = 3;
				splitToken(parts[1], &nParts, '/', &parts[0]);
				if (nParts > 0)
					mp4vPacketParserPtr->timeScale = FskStrToNum(parts[1]);
			}
			FskMemPtrDispose(value);
		}
	}

bail:
	return err;
}


FskErr mp4vPacketParserDispose(RTPPacketParser parser)
{
	MP4VPacketParser *mp4vPacketParserPtr = (MP4VPacketParser *)parser->handlerRefCon;

	if (mp4vPacketParserPtr) 
	{
		RTPCompressedMediaFrame frames		= mp4vPacketParserPtr->frames;
		RTPCompressedMediaFrame framesNext	= NULL;
		
		while(frames)
		{			
			framesNext = frames->next;
			// Remove any stored data from our list
			FskMemPtrDispose(frames);
			frames = framesNext;
		}

		FskMemPtrDispose(mp4vPacketParserPtr->esds);
		FskMemPtrDispose(mp4vPacketParserPtr);
	}

	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr mp4vPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName)
{
	UInt32 profileLevelID = 0;
	Boolean validESDS = false;
	Boolean validProfileID = false;

	// We handle MPEG-4 Visual
	if (0 != FskStrCompareCaseInsensitive("MP4V-ES", encodingName)) return kFskErrRTSPPacketParserUnsupportedFormat;

	if (NULL != mediaDescription) {
		SDPAttribute attribute;
		attribute = SDPFindMediaAttribute(mediaDescription, "fmtp");
		if (NULL != attribute) {
			char *value;
			
			// We handle simple profile levels 0, 1, 2, 3
			// According to ISO/IEC 14496-2, these correspond to
			// IDs 8, 1, 2, 3, respectively
			if (0 != copyAttributeValue(attribute->value, "profile-level-id", &value)) {
				profileLevelID = FskStrToNum(value);
				switch (profileLevelID) {
					case 8:
					case 1:
					case 2:
					case 3:
					case 9:
					case 247:	// Axis network cameras
						validProfileID = true;
						break;
				}
				FskMemPtrDispose(value);
			}
			else {
				// According to RFC-3016 p. 13, if the profile-level-id string is not present,
				// we should assume the video is simple profile level 1
				validProfileID = true;
			}
							
			// We require a valid config string
			if (0 != copyAttributeValue(attribute->value, "config", &value)) {
				UInt8 *esds = 0;
				UInt32 esdsSize, width, height;
				validESDS = configToESDS(value, &esds, &esdsSize, &width, &height);
				FskMemPtrDispose(value);
				FskMemPtrDispose(esds);
			}
		}
	}

	return (validESDS && validProfileID) ? 0 : kFskErrRTSPPacketParserUnsupportedFormat;
}

/* -----------------------------------------------------------------------*/

FskErr mp4vPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize)
{
	MP4VPacketParser *mp4vPacketParserPtr = (MP4VPacketParser *)parser->handlerRefCon;
	FskErr err = 0;

	switch(selector) {
		case kRTPPacketParserSelectorESDS:
			*(UInt8**)info = mp4vPacketParserPtr->esds;
			if (infoSize)
				*infoSize = mp4vPacketParserPtr->esdsSize;
			break;

		case kRTPPacketParserSelectorMediaFormat:
			*(UInt32*)info = mp4vPacketParserPtr->mediaFormat;
			break;
			
		case kRTPPacketParserSelectorVideoWidth:
			*(UInt32*)info = mp4vPacketParserPtr->width;
			break;

		case kRTPPacketParserSelectorVideoHeight:
			*(UInt32*)info = mp4vPacketParserPtr->height;
			break;
		case kRTPPacketParserSelectorRTPTimeScale:
		case kRTPPacketParserSelectorVideoTimeScale:
			*(UInt32*)info = mp4vPacketParserPtr->timeScale;
			break;

		default:
			err = kFskErrInvalidParameter;
			break;
	}
	return err;
}

/* -----------------------------------------------------------------------*/

#define DEBUG_STRINGS					0		
#define FRAME_LEVEL_ERROR_RESILIENCE	1


#if 0
static Boolean LocateNextHeader( long headerBigEndian, long dataSize, UInt8 *dataPtr, long *offset )
{
	long	i;
	Boolean	hasIt		 =  false;
	long	sizeLong	 = dataSize/4;
	long	*dataLongPtr = (long *)dataPtr;
	
	*offset = dataSize;
	if( sizeLong <= 0 )
		goto bail;
	
	for( i = 0; i < sizeLong; i++ )
	{
		if( dataLongPtr[i] == headerBigEndian )
		{
			*offset = i*4;
			hasIt = true;
			break;
		}
	}

bail:
	return hasIt;
}

Boolean HasMultipleVOP( long dataSize, UInt8 *dataPtr )
{
	long count  = 0;
	long offset = 0;
	long headerBigEndian = 0;

	{
		((UInt8 *)&headerBigEndian)[0] = 0x00;
		((UInt8 *)&headerBigEndian)[1] = 0x00;
		((UInt8 *)&headerBigEndian)[2] = 0x01;
		((UInt8 *)&headerBigEndian)[3] = 0xB6;
	}

	while( dataSize > 0 )
	{
		if( LocateNextHeader( headerBigEndian, dataSize, dataPtr, &offset ) )
			count++;
		
		dataPtr  += offset + 4;
		dataSize -= offset + 4;
	}

	return count>1;
}
#endif

static void FindVOP(unsigned char *data, long size, long *offsetOut )
{
	long i;

	for( i = 0; i < size - 3; i++ )
		if( data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01  &&data[i+3] == 0xb6 )
			break;

	*offsetOut = i;
}


int is_key_frame( unsigned char *bs, int size )
{
	int i = 0;

	//make sure to at least check the first vop
	while(i < size - 3)
	{
		if( bs[i] == 0x00 && bs[i+1] == 0x00 && bs[i+2] == 0x01  &&bs[i+3] == 0xb6 )
			return (bs[i+4]&0xC0)== 0;

		i++;
	}

	return 0;
}


FskErr mp4vPacketParserProcessPacket(RTPPacketParser parser, RTPPacket rtpHeader)
{
/*
test movie
rtsp://stats.kinoma.com/MP4-mp4v.mov
rtsp://stats.kinoma.com/chicken_ffmpeg.mp4
rtsp://stats.kinoma.com/small_ffmpeg.mp4
rtsp://stats.kinoma.com/small1_qt.mp4
*/
	UInt32					storedSize				= 0;
	RTPCompressedMediaFrame	outFrame				= NULL;
	MP4VPacketParser		*mp4vPacketParserPtr	= NULL;
	Boolean					frameChanged			= false;
	Boolean					isKeyFrame			= false;
	Boolean					isLastPacketOfAFrame	= false;
	Boolean					thisPacketIsVOG			= false;
	Boolean					lastPacketIsVOG			= false;
//	Boolean					hasMultipleVOP			= 0;
	UInt8					*dataPtr				= NULL;
	UInt32					dataSize				= 0;
	UInt32					err = 0;

	if(!parser || !rtpHeader) {
		err = kFskErrRTSPBadPacket;
		goto bail;
	}
	
	dataPtr			   = rtpHeader->data;		//input 
	dataSize		   = rtpHeader->dataSize;
	rtpHeader->frames  = NULL;					//output

	mp4vPacketParserPtr = (MP4VPacketParser *)parser->handlerRefCon;
	if( mp4vPacketParserPtr == NULL ) {
		err = kFskErrBadData;
		goto bail;
	}

	if( dataPtr[0] == 0x00 && dataPtr[1] == 0x00 && dataPtr[2] == 0x01 )
	{

		if( dataPtr[3] != 0xB6 )
		{
			if( dataPtr[3] == 0xB3 )
				thisPacketIsVOG = true;

			FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "******not vop!******");
			{
				long offset = 0;

				FindVOP(dataPtr, dataSize, &offset );
				dataPtr  += offset;
				dataSize -= offset;

				if( offset != 0 )
					FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "******offset = %d******", offset);
			}
		}
		
		if(  dataPtr[3] == 0xB6 )
		{
			isKeyFrame = (dataPtr[4]&0xC0)== 0;
			if( isKeyFrame )
			{
				FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "******key!******");
			}
			else
			{
				FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "******not key!******");
			}
		}
	}

	frameChanged		 = mp4vPacketParserPtr->lastTimeStamp != rtpHeader->timestamp;
	isLastPacketOfAFrame = rtpHeader->marker;
	lastPacketIsVOG		 = mp4vPacketParserPtr->lastPacketIsVOG;
	mp4vPacketParserPtr->lastTimeStamp   = rtpHeader->timestamp;
	mp4vPacketParserPtr->lastPacketIsVOG = thisPacketIsVOG;
	
	//***no real use for the moment
	// --bryan 6/30/2005
#if 0
	hasMultipleVOP = HasMultipleVOP( dataSize, dataPtr );
#endif

#if DEBUG_STRINGS
	{
		static int frameIndx = 0;
		int	i;

		frameIndx++;
		
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "#%d, dataSize = %d", frameIndx, dataSize);
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "#%d, rtpHeader->timestamp = %d", frameIndx, rtpHeader->timestamp);
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "#%d, isLastPacketOfAFrame = %d", frameIndx, isLastPacketOfAFrame);
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "#%d, thisPacketIsVOG = %d", frameIndx, thisPacketIsVOG);
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "#%d, lastPacketIsVOG = %d", frameIndx, lastPacketIsVOG);
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "#%d, frameChanged = %d", frameIndx, frameChanged);
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "#%d, hasMultipleVOP = %d", frameIndx, hasMultipleVOP);
		
		for( i = 0; i < 16; i++ )
			FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "0x%x, ", dataPtr[i]);
/*		
		for( i = 16; i < 32; i++ )
			FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "0x%x, ", dataPtr[i]);

		for( i = 32; i < 48; i++ )
			FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "0x%x, ", dataPtr[i]);
		
		for( i = 48; i < 64; i++ )
			FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "0x%x, ", dataPtr[i]);
		
		for( i = 64; i < 80; i++ )
			FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "0x%x, ", dataPtr[i]);
		
		for( i = 80; i < 96; i++ )
			FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "0x%x, ", dataPtr[i]);
		
*/		
	}
#endif

	err = GetLinkededFramesSize( mp4vPacketParserPtr->frames, &storedSize );
	if( err != 0 ) goto bail;

	if( frameChanged && storedSize != 0 &&  !lastPacketIsVOG )
	{
#if !FRAME_LEVEL_ERROR_RESILIENCE
		ClearLinkedFrames( mp4vPacketParserPtr->frames );
		mp4vPacketParserPtr->frames = NULL;	
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "						probably pcket loss, discard stored data");
#else
		err = FskMemPtrNewClear( storedSize + sizeof(RTPCompressedMediaFrameRecord), &outFrame );
		if( err != 0 )	goto bail;

		err = MoveLinkedFramesDataToFrame( mp4vPacketParserPtr->frames, outFrame );
		if( err != 0 )	goto bail;
		mp4vPacketParserPtr->frames = NULL;

		//put this frame on top of output frame link list
		rtpHeader->frames	= outFrame;
		rtpHeader->dataSize = outFrame->length;
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "						probably pcket loss, output from stored data: %d", rtpHeader->dataSize);

		{
			//***if this's the last packet of a frame
			// we'll have to wait next call to output it, not very nice...
			// waiting for multiple frame output support
			// --bryan 6/30/2005
#if OUTPUT_MULTIPLE_FRAMES
			if( isLastPacketOfAFrame )
			{
				FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "************************attaching extra!!!");
				err = PushDataToLinkedFrames( rtpHeader->timestamp, 0, dataSize, dataPtr,&rtpHeader->frames );
			}
			else
#endif			
			err = PushDataToLinkedFrames( rtpHeader->timestamp, 0, dataSize, dataPtr, &mp4vPacketParserPtr->frames );
			if( err != 0 )	goto bail;
			
			goto bail;
		}
#endif
	}

	err = PushDataToLinkedFrames( rtpHeader->timestamp, 0, dataSize, dataPtr, &mp4vPacketParserPtr->frames );
	if( err != 0 )	goto bail;

	//if( thisPacketIsVOG )
	if( !isLastPacketOfAFrame )	//keep appending
		goto bail;

	err = GetLinkededFramesSize( mp4vPacketParserPtr->frames, &storedSize );
	if( err != 0 ) goto bail;

	err = FskMemPtrNewClear( storedSize + sizeof(RTPCompressedMediaFrameRecord), &outFrame );
	if( err != 0 ) goto bail;
	
	err = MoveLinkedFramesDataToFrame( mp4vPacketParserPtr->frames, outFrame );
	if( err != 0 ) goto bail;
	mp4vPacketParserPtr->frames = NULL;
	
	//***when multiple frame output is supported, 
	// outFrame should be appended to rtpHeader->frames
	// and out size should be set in a appropriate way
	// --bryan 6/30/2005
	rtpHeader->frames	= outFrame;
	rtpHeader->dataSize	= outFrame->length;
	FskInstrumentedTypePrintfDebug(&gRTPPacketParserMP4VTypeInstrumentation, "						output from stored data: %d", rtpHeader->dataSize);

bail:
	// Test for a key frame
	if (NULL != rtpHeader) {
		RTPCompressedMediaFrame thisFrame = rtpHeader->frames;

		while( thisFrame )		//if (0 != rtpHeader->frames)
		{
			UInt8 *bs = (UInt8*)(thisFrame + 1);
			int this_size = thisFrame->length;

			thisFrame->key = is_key_frame( bs, this_size );
			thisFrame = thisFrame->next;
		}
	}

	if( err != 0 && outFrame != NULL )
		FskMemPtrDispose(outFrame);

	return err;
}

/* -----------------------------------------------------------------------*/

FskErr mp4vPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet)
{
	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr mp4vPacketParserFlush(RTPPacketParser parser)
{
	MP4VPacketParser		*mp4vPacketParserPtr;
	
	mp4vPacketParserPtr = (MP4VPacketParser *)parser->handlerRefCon;
	
	ClearLinkedFrames( mp4vPacketParserPtr->frames );
	mp4vPacketParserPtr->frames = NULL;	
	mp4vPacketParserPtr->lastTimeStamp = 0;
	mp4vPacketParserPtr->lastPacketIsVOG = false;
	
	return 0;
}


static void LocateLastVS( unsigned char *esds, UInt32 *sizeInOut )
{
	long size = *sizeInOut;
	long i, offset = 0;

	for( i = 0; i < size - 3; i++ )
	{
		if( esds[i] == 0x00 && esds[i+1] == 0x00 && esds[i+2] == 0x01  &&esds[i+3] == 0xb0 )
			offset = i;
	}
	
	if( offset != 0 )
	{
		*sizeInOut -= offset;

		FskMemMove( esds, (esds+offset), size - offset);
	}
}

/* -----------------------------------------------------------------------*/

static Boolean configToESDS(char *config, UInt8 **esdsOut, UInt32 *esdsSize, UInt32 *width, UInt32 *height)
{
	UInt8 *esds, *esdsPtr, *p;
	UInt8 *decoderSpecificInfo;
	UInt32 decoderSpecificInfoSize;
	UInt8 esDescriptor[9];
	UInt32 esDescriptorSize;
	UInt8 decoderConfigDescriptor[15];
	UInt32 decoderConfigDescriptorSize;
	UInt8 slConfigDescriptor[3];
	UInt32 slConfigDescriptorSize;
	Boolean result = false;
	UInt8 profile_level;

	// 8.6.5 ES_Descriptor
	esDescriptorSize = 9;
	esDescriptor[0] = esDescriptor[1] = esDescriptor[2] = esDescriptor[3] = 0;	// version flags (part of the header)
	esDescriptor[4] = 0x3;	// ES_DescrTag
	esDescriptor[5] = 0;	// length
	esDescriptor[6] = esDescriptor[7] = 0;	// ES_ID
	esDescriptor[8] = 0;	// streamDependenceFlag(1), URL_Flag(1), OCRstreamFlag(1), streamPriority(5)

	// 8.6.6 DecoderConfigDescriptor
	decoderConfigDescriptorSize = 15;
	decoderConfigDescriptor[0] = 0x4;	// DecoderConfigDescrTag
	decoderConfigDescriptor[1] = 0;		// length
	decoderConfigDescriptor[2] = 0x20;	// objectTypeIndication
	decoderConfigDescriptor[3] = 0x11;	// streamType(6), upStream(1), reserved(1)=1
	decoderConfigDescriptor[4] = 0;  decoderConfigDescriptor[5] = 0x3b; decoderConfigDescriptor[6] = 0xd1;	// bufferSizeDB
	decoderConfigDescriptor[7] = 0; decoderConfigDescriptor[8] = 0x08; decoderConfigDescriptor[9] = 0x0B; decoderConfigDescriptor[10] = 0xF8;	// maxBitrate
	decoderConfigDescriptor[11] = 0; decoderConfigDescriptor[12] = 0; decoderConfigDescriptor[13] = 0; decoderConfigDescriptor[14] = 0;	// avgBitrate

	// Decoder specific info
	decoderSpecificInfoSize = FskStrLen(config)/2;
	BAIL_IF_ERR(FskMemPtrNewClear(decoderSpecificInfoSize + 2, &decoderSpecificInfo));
	decoderSpecificInfo[0] = 0x5;	// DecSpecificInfoTag
	decoderSpecificInfo[1] = (UInt8)decoderSpecificInfoSize;

	p = (UInt8*)config;
	esdsPtr = &decoderSpecificInfo[2];
	while (*p) {
		UInt8 a, b;
		if (*p >= '0' && *p <= '9')
			a = *p - '0';
		else if (*p >= 'a' && *p <= 'f')
			a = *p - 'a' + 10;
		else
			a = *p - 'A' + 10;
		++p;
		
		if (*p >= '0' && *p <= '9')
			b = *p - '0';
		else if (*p >= 'a' && *p <= 'f')
			b = *p - 'a' + 10;
		else
			b = *p - 'A' + 10;
		++p;
		
		*esdsPtr++ = (a << 4) + b;
	}

	LocateLastVS( &decoderSpecificInfo[2], &decoderSpecificInfoSize );

	// SLConfigDescriptor
	slConfigDescriptorSize = 3;
	slConfigDescriptor[0] = 0x6;	// SLConfigDescrTag 
	slConfigDescriptor[1] = 1;		// length
	slConfigDescriptor[2] = 2;

	// Glue everything together
	*esdsSize = esDescriptorSize + decoderConfigDescriptorSize + decoderSpecificInfoSize + 2 + slConfigDescriptorSize;
	BAIL_IF_ERR(FskMemPtrNew(*esdsSize, (FskMemPtr*)&esds));
	*esdsOut = esds;
	esdsPtr = esds;
	FskMemMove(esdsPtr, &esDescriptor[0], esDescriptorSize);
	esdsPtr += esDescriptorSize;
	FskMemMove(esdsPtr, &decoderConfigDescriptor[0], decoderConfigDescriptorSize);
	esdsPtr += decoderConfigDescriptorSize;
	FskMemMove(esdsPtr, &decoderSpecificInfo[0], decoderSpecificInfoSize + 2);
	esdsPtr += (decoderSpecificInfoSize + 2);
	FskMemMove(esdsPtr, &slConfigDescriptor[0], slConfigDescriptorSize);
	esdsPtr += slConfigDescriptorSize;

	// Back patch descriptor lengths
	esds[5] = (UInt8)(esdsPtr - &esds[5] - 1);
	esds[10] = (UInt8)(decoderConfigDescriptorSize + decoderSpecificInfoSize);

	result = QTESDSScanVideo(decoderSpecificInfo, decoderSpecificInfoSize, width, height, &profile_level);

	FskMemPtrDispose(decoderSpecificInfo);

bail:
	return result;
}
