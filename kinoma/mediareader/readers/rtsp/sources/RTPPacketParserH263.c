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
	Selection of Modes for the H.263 Payload Header

	http://standard.pictel.com/h263+rtp/draft20ncm.pdf
*/

#include "FskUtilities.h"
#include "FskEndian.h"
#include "RTPPacketParserH263.h"

// Private state
typedef struct {
	UInt32 state;
	UInt32 mediaFormat;
	UInt32 width;
	UInt32 height;
	UInt32 timeScale;
	UInt32 lastTimeStamp;
	
	RTPCompressedMediaFrame frames;
} H263PacketParserRecord, *H263PacketParser;

FskInstrumentedSimpleType(RTPPacketParserH263, rtppacketparserh263);

void h263PacketParserInitialize(RTPPacketHandler handler);

static FskErr _PushDataToLinkedFrames( int size, unsigned char *dataPtr, RTPCompressedMediaFrame *storedFrames );
static FskErr _GetLinkededFramesSize( RTPCompressedMediaFrame storedFrames, UInt32	*storedSizeOut );
static FskErr _MoveLinkedFramesDataToFrame( RTPCompressedMediaFrame storedFrames, RTPCompressedMediaFrame frame );
static FskErr _ClearLinkedFrames( RTPCompressedMediaFrame storedFrames );

static FskErr h263PacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName);
static FskErr h263PacketParserNew(RTPPacketParser parser);
static FskErr h263PacketParserDispose(RTPPacketParser parser);
static FskErr h263PacketParserProcessPacket(RTPPacketParser parser, RTPPacket packet);
static FskErr h263PacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet);
static FskErr h263PacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize);
static FskErr h263PacketParserFlush(RTPPacketParser parser);
static FskErr h263PacketParserDecodePictureHheaderPriv(unsigned char *b_ptr, UInt32 *width, UInt32 *height);

/* -----------------------------------------------------------------------*/

#define FRAME_LEVEL_ERROR_RESILIENCE	1
#define OUTPUT_MULTIPLE_FRAMES 1


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

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserH263TypeInstrumentation, "						append: %d", size);

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

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserH263TypeInstrumentation, "						_GetLinkededFramesSize: %d", storedSize);

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

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserH263TypeInstrumentation, "						_MoveLinkedFramesDataToFrame: %d", frame->length);

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

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserH263TypeInstrumentation, "						_ClearLinkedFrames");

	return 0;
}

/* -----------------------------------------------------------------------*/

void h263PacketParserInitialize(RTPPacketHandler handler)
{
	handler->version = 1;

	handler->doCanHandle = h263PacketParserCanHandle;
	handler->doNew = h263PacketParserNew;
	handler->doDispose = h263PacketParserDispose;
	handler->doProcessPacket = h263PacketParserProcessPacket;
	handler->doDisposePacket = h263PacketParserDisposePacket;
	handler->doGetInfo = h263PacketParserGetInfo;
}

/* -----------------------------------------------------------------------*/

FskErr h263PacketParserNew(RTPPacketParser parser)
{
	FskErr err;
	H263PacketParser h263PacketParser;
	SDPMediaDescription mediaDescription;
	SDPAttribute attribute;

	err = FskMemPtrNew(sizeof(H263PacketParserRecord), &h263PacketParser);
	if (0 != err) goto bail;
	
	parser->handlerRefCon = h263PacketParser;
	h263PacketParser->frames = NULL;

	h263PacketParser->mediaFormat = kRTPVideoFormatH263;
	h263PacketParser->width = 0;
	h263PacketParser->height = 0;
	h263PacketParser->lastTimeStamp = 0;
	
	// look for the "a=framesize" attribute in the SDP for width and height
	// referenced in 3GPP TS 26.234 version 6.3.0 Release 6
	if (0 == h263PacketParser->width) {
		char *tmp = NULL, *p[3] = {0};
		UInt16 nParts;
		attribute = SDPFindMediaAttribute(parser->mediaDescription, "framesize");
		if (attribute) {
			SDPMediaFormat mediaFormat = parser->mediaDescription->formatList->head;
			tmp = FskStrDoCopy(attribute->value);
			splitToken(tmp, &nParts, ' ', &p[0]);
			if ((UInt32)FskStrToNum(p[0]) == mediaFormat->payloadType) {
				splitToken(p[1], &nParts, '-', &p[0]);
				h263PacketParser->width = FskStrToNum(p[0]);
				h263PacketParser->height = FskStrToNum(p[1]);
			}
			FskMemPtrDispose(tmp);
		}
	}
	
	// look for the "a=Width" and "a=Height" attributes (TMI server)
	// a=Width:integer;176
	// a=Height:integer;144
	if (0 == h263PacketParser->width) {
		char *p[3] = {0};
		UInt16 nParts;
		attribute = SDPFindMediaAttribute(parser->mediaDescription, "Width");
		if (attribute) {
			char *value;
			value = FskStrDoCopy(attribute->value);
			splitToken(value, &nParts, ';', &p[0]);
			h263PacketParser->width = FskStrToNum(p[1]);
			FskMemPtrDispose(value);
		}
	}
	
	if (0 == h263PacketParser->height) {
		char *p[3] = {0};
		UInt16 nParts;
		attribute = SDPFindMediaAttribute(parser->mediaDescription, "Height");
		if (attribute) {
			char *value;
			value = FskStrDoCopy(attribute->value);
			splitToken(value, &nParts, ';', &p[0]);
			h263PacketParser->height = FskStrToNum(p[1]);
			FskMemPtrDispose(value);
		}
	}

	// Next try the cliprect attribute
	if (0 == h263PacketParser->width) {
		attribute = SDPFindMediaAttribute(parser->mediaDescription, "cliprect");
		if (NULL != attribute) {
			char *value, *parts[5];
			UInt16 nParts;
			value = FskStrDoCopy(attribute->value);
			splitToken(value, &nParts, ',', &parts[0]);
			if (4 == nParts) {
				h263PacketParser->width  = FskStrToNum(parts[3]);
				h263PacketParser->height  = FskStrToNum(parts[2]);
			}
			FskMemPtrDispose(value);
		}
	}
		
	// Next try the framesize in the fmtp attribute
	if (0 == h263PacketParser->width) {
		attribute = SDPFindMediaAttribute(parser->mediaDescription, "fmtp");
		if (NULL != attribute) {
			char *attr;

			if (NULL != copyAttributeValue(attribute->value, "framesize", &attr)) {
				char *value, *parts[5];
				UInt16 nParts;
				value = FskStrDoCopy(attr);
				splitToken(value, &nParts, '-', &parts[0]);
				if (2 == nParts) {
					h263PacketParser->width  = FskStrToNum(parts[0]);
					h263PacketParser->height  = FskStrToNum(parts[1]);
				}
				FskMemPtrDispose(attr);
				FskMemPtrDispose(value);
			}
		}
	}
	
	// Grab the timescale - should always be 90000
	h263PacketParser->timeScale = 90000;
	
	mediaDescription = parser->mediaDescription;
	if (NULL != mediaDescription) {
		SDPAttribute attribute;
		attribute = SDPFindMediaAttribute(mediaDescription, "rtpmap");
		if (NULL != attribute) {
			char *value, *parts[4];
			UInt16 nParts = 2;
			value = FskStrDoCopy(attribute->value);
			splitToken(value, &nParts, '/', &parts[0]);
			h263PacketParser->timeScale = FskStrToNum(parts[1]);
			FskMemPtrDispose(value);
		}
	}
	
bail:
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr h263PacketParserDispose(RTPPacketParser parser)
{
	H263PacketParser h263PacketParser = (H263PacketParser)parser->handlerRefCon;

	h263PacketParserFlush(parser);
	if (h263PacketParser)
		FskMemPtrDispose(h263PacketParser);

	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr h263PacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName)
{
	
	if (0 == FskStrCompareCaseInsensitive("H263-1998", encodingName) ||
		0 == FskStrCompareCaseInsensitive("H263-2000", encodingName))
		return 0;

	return kFskErrRTSPPacketParserUnsupportedFormat;
}


/* -----------------------------------------------------------------------*/

FskErr  h263PacketParserProcessPacket(RTPPacketParser parser, RTPPacket rtpHeader)
{	
	H263PacketParser		h263PacketParser;
	unsigned char			*headerStart;
	RTPPayloadH263			payloadHeader;
	UInt32					headerSize;
	UInt32					storedSize				= 0;
	RTPCompressedMediaFrame	outFrame				= NULL;
	Boolean					frameChanged			= false;
//	Boolean					isKeyFrame				= false;
	Boolean					isLastPacketOfAFrame	= false;
//	Boolean					thisPacketIsVOG			= false;
//	Boolean					lastPacketIsVOG			= false;
	UInt8					*dataPtr				= NULL;
	UInt32					dataSize				= 0;
	UInt8					*copyBuffer				= NULL;
	UInt32					err						= 0;

	if(!parser || !rtpHeader) {
		err = kFskErrRTSPBadPacket;
		goto bail;
	}

	h263PacketParser = (H263PacketParser)parser->handlerRefCon;
	headerStart = rtpHeader->data;

	headerSize = 2;
	
	/*
	The H.263+ payload header is structured as follows:

	0                   1
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|   RR    |P|V|   PLEN    |PEBIT|
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	RR: 5 bits
		Reserved bits. Shall be zero.

	P: 1 bit
		Indicates the picture start or a picture segment (GOB/Slice) start
		or a video sequence end (EOS or EOSBS).  Two bytes of zero bits
		then have to be prefixed to the payload of such a packet to compose
		a complete picture/GOB/slice/EOS/EOSBS start code.  This bit allows
		the omission of the two first bytes of the start codes, thus
		improving the compression ratio.

	V: 1 bit
		Indicates the presence of an 8 bit field containing information for
		Video Redundancy Coding (VRC), which follows immediately after the
		initial 16 bits of the payload header if present.  For syntax and
		semantics of that 8 bit VRC field see section 4.2.

	PLEN: 6 bits
		Length in bytes of the extra picture header. If no extra picture header 
		is attached, PLEN is 0. If PLEN>0, the extra picture header is attached 
		immediately following the rest of the payload header. Note the length 
		reflects the omission of the first two bytes of the picture start code 
		(PSC). See section 5.1.

	PEBIT: 3 bits
		Indicates the number of bits that shall be ignored in the last byte of 
		the picture header. If PLEN is not zero, the ignored bits shall be the l
		east significant bits of the byte. If PLEN is zero, then PEBIT shall also 
		be zero. 
	*/

	payloadHeader.P = (headerStart[0]&0x4) != 0;
	payloadHeader.V = (headerStart[0]&0x2) != 0;
	payloadHeader.PLEN = ((headerStart[0]&0x1)<<5)|(headerStart[1]>>3);
	payloadHeader.PEBIT = (headerStart[1]&0x7);
	
	frameChanged		 = h263PacketParser->lastTimeStamp != rtpHeader->timestamp;
	isLastPacketOfAFrame = rtpHeader->marker;
	//lastPacketIsVOG		 = mp4vPacketParserPtr->lastPacketIsVOG;
	h263PacketParser->lastTimeStamp   = rtpHeader->timestamp;
	//mp4vPacketParserPtr->lastPacketIsVOG = thisPacketIsVOG;

	// Parse the picture header
	if (payloadHeader.PLEN > 0) 
	{
		// only look in header for width/height information if necessary
		if (h263PacketParser->width == 0 && h263PacketParser->height == 0)
		{
			h263PacketParserDecodePictureHheaderPriv(headerStart, &h263PacketParser->width, &h263PacketParser->height);
		}
		headerSize += payloadHeader.PLEN;
	}

	// Skip over the extra VRC byte at the end of the header:
	if (payloadHeader.V) 
		++headerSize;

	dataSize = rtpHeader->dataSize - headerSize;
	dataPtr = rtpHeader->data + headerSize;
	
	/*
	Picture Segment Packets and Sequence Ending Packets (P=1)

	A picture segment packet is defined as a packet that starts at the location 
	of a Picture, GOB, or slice start code in the H.263+ data stream. This corresponds 
	to the definition of the start of a video picture segment as defined in H.263+. 
	For such packets, P=1 always.
	*/
	if(payloadHeader.P == 1)
	{
		/*
		Two bytes of zero bits then have to be prefixed to the payload 
		to compose a complete picture/GOB/slice/EOS/EOSBS start code. 
		*/
		err = FskMemPtrNew(2 + dataSize, (FskMemPtr*)&copyBuffer);
		BAIL_IF_ERR(err);
		
		copyBuffer[0] = copyBuffer[1] = 0;
		FskMemMove(&copyBuffer[2], dataPtr, dataSize);
		
		dataSize += 2;
		dataPtr = copyBuffer;
	}

	err = _GetLinkededFramesSize( h263PacketParser->frames, &storedSize );
	if( err != 0 ) goto bail;

	if( frameChanged && storedSize != 0 /*&&  !lastPacketIsVOG*/ )
	{
		err = FskMemPtrNewClear( storedSize + sizeof(RTPCompressedMediaFrameRecord), &outFrame );
		if( err != 0 )	goto bail;

		err = _MoveLinkedFramesDataToFrame( h263PacketParser->frames, outFrame );
		if( err != 0 )	goto bail;
		h263PacketParser->frames = NULL;

		//put this frame on top of output frame link list
		rtpHeader->frames	= outFrame;
		rtpHeader->dataSize = outFrame->length;
		FskInstrumentedTypePrintfDebug(&gRTPPacketParserH263TypeInstrumentation, "						probably pcket loss, output from stored data: %d", rtpHeader->dataSize);

		{
			//***if this's the last packet of a frame
			// we'll have to wait next call to output it, not very nice...
			// waiting for multiple frame output support
			// --bryan 6/30/2005
#if OUTPUT_MULTIPLE_FRAMES
			if( isLastPacketOfAFrame )
			{
				FskInstrumentedTypePrintfDebug(&gRTPPacketParserH263TypeInstrumentation, "************************attaching extra!!!");
				err = _PushDataToLinkedFrames( dataSize, dataPtr,&rtpHeader->frames );
			}
			else
#endif			
			err = _PushDataToLinkedFrames( dataSize, dataPtr, &h263PacketParser->frames );
			if( err != 0 )	goto bail;
			
			goto bail;
		}
	}

	err = _PushDataToLinkedFrames( dataSize, dataPtr, &h263PacketParser->frames );
	if( err != 0 )	goto bail;

	//if( thisPacketIsVOG )
	if( !isLastPacketOfAFrame )	//keep appending
		goto bail;

	err = _GetLinkededFramesSize( h263PacketParser->frames, &storedSize );
	if( err != 0 ) goto bail;

	err = FskMemPtrNewClear( storedSize + sizeof(RTPCompressedMediaFrameRecord), &outFrame );
	if( err != 0 ) goto bail;
	
	err = _MoveLinkedFramesDataToFrame( h263PacketParser->frames, outFrame );
	if( err != 0 ) goto bail;
	h263PacketParser->frames = NULL;
	
	//***when multiple frame output is supported, 
	// outFrame should be appended to rtpHeader->frames
	// and out size should be set in a appropriate way
	// --bryan 6/30/2005
	rtpHeader->frames	= outFrame;
	rtpHeader->dataSize	= outFrame->length;
	FskInstrumentedTypePrintfDebug(&gRTPPacketParserH263TypeInstrumentation, "						output from stored data: %d", rtpHeader->dataSize);


bail:
	// Test for a key frame
	if (NULL != rtpHeader) {
		if (0 != rtpHeader->frames) {
			UInt8 *bs = (UInt8*)(rtpHeader->frames+1);
			if( bs[0] == 0 && bs[1] == 0 && (bs[2]&0xfc)== 0x80) {
				rtpHeader->frames->key = !(bs[4]&0x02);
			}
		}
	}
		
	if( err != 0 && outFrame != NULL )
		FskMemPtrDispose(outFrame);
		
	FskMemPtrDispose(copyBuffer);
	
	return err;
}


#if 0
	// tests for a key frame
	{
		unsigned char *bs = (unsigned char *)(rtpHeader->frames+1);

		if( bs[0] == 0 && bs[1] == 0 && (bs[2]&0xfc)== 0x80)
		{	
			unsigned char isKeyFrame = !(bs[4]&0x02); 
			//if( isKeyFrame )
			//{
			//	fprintf(stderr, "key!\n" );
			//	fflush(stderr);
			//}
		}



	}
#endif

/* -----------------------------------------------------------------------*/

FskErr h263PacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet)
{
	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr h263PacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize)
{
	H263PacketParser h263PacketParser = (H263PacketParser)parser->handlerRefCon;
	FskErr err = 0;

	switch(selector) {
		case kRTPPacketParserSelectorMediaFormat:
			*(UInt32*)info = h263PacketParser->mediaFormat;
			break;
		case kRTPPacketParserSelectorVideoWidth:
			*(UInt32*)info = h263PacketParser->width;
			break;
		case kRTPPacketParserSelectorVideoHeight:
			*(UInt32*)info = h263PacketParser->height;
			break;
		case kRTPPacketParserSelectorRTPTimeScale:
		case kRTPPacketParserSelectorVideoTimeScale:
			*(UInt32*)info = h263PacketParser->timeScale;
			break;
		default:
			err = kFskErrInvalidParameter;
			break;
	}
	return err;
}

FskErr h263PacketParserFlush(RTPPacketParser parser)
{
	H263PacketParser h263PacketParser = (H263PacketParser)parser->handlerRefCon;
	
	if(h263PacketParser) {
		_ClearLinkedFrames( h263PacketParser->frames );
		h263PacketParser->frames = NULL;	
		h263PacketParser->lastTimeStamp = 0;
		//mp4vPacketParserPtr->lastPacketIsVOG = false;
	}
	
	return 0;
}

static unsigned int h263PacketParserGetBits(H263BitBuffer *bitBuffer, int n)
{
    unsigned int x=0;
    while(n-->0){
		if(!bitBuffer->bitcnt){
			// fill buff
			bitBuffer->buf = bitBuffer->buffer[bitBuffer->bufptr++];
			bitBuffer->bitcnt = 8;
		}
		x=(x<<1)|(bitBuffer->buf>>7);
		bitBuffer->buf<<=1;
		--bitBuffer->bitcnt;
    }
    return x;
}

static FskErr h263PacketParserDecodePictureHheaderPriv(unsigned char *b_ptr, UInt32 *width, UInt32 *height)
{
	H263BitBuffer bitBuffer = {0};
	UInt32 format;
    bitBuffer.buffer = b_ptr;
	
	// picture header
    h263PacketParserGetBits(&bitBuffer, 22); 

	// picture timestamp
    h263PacketParserGetBits(&bitBuffer, 8); 

	// marker
    if (h263PacketParserGetBits(&bitBuffer, 1) != 1){
        return -1;	
    }

    if (h263PacketParserGetBits(&bitBuffer, 1) != 0){
        return -1;	/* h263 id */
    }

	// split screen
    h263PacketParserGetBits(&bitBuffer, 1);	

	// camera
    h263PacketParserGetBits(&bitBuffer, 1);

	// freeze picture release
    h263PacketParserGetBits(&bitBuffer, 1);

	// format
    format = h263PacketParserGetBits(&bitBuffer, 3);

    if (format != 7) {
        // H.263 v1
        *width = h263_format[format][0];
        *height = h263_format[format][1];
		//pict_type
		h263PacketParserGetBits(&bitBuffer, 1);
		// unrestricted_mv
		h263PacketParserGetBits(&bitBuffer, 1);
		// SAC
		h263PacketParserGetBits(&bitBuffer, 1);
		// advanced prediction mode
		h263PacketParserGetBits(&bitBuffer, 1);
		// PB frame
		h263PacketParserGetBits(&bitBuffer, 1);
		// qscale
		h263PacketParserGetBits(&bitBuffer, 5);
		// Continuous Presence Multipoint mode
        h263PacketParserGetBits(&bitBuffer, 1);	
    } else {
        // H.263 plus
        if (h263PacketParserGetBits(&bitBuffer, 3) != 1){
            return kFskErrRTSPPacketParserUnsupportedFormat;
		}
		// custom source format
        if (h263PacketParserGetBits(&bitBuffer, 3) != 6)
		{
            return kFskErrRTSPPacketParserUnsupportedFormat;
		}
        h263PacketParserGetBits(&bitBuffer, 12);
        h263PacketParserGetBits(&bitBuffer, 3);

		// pict_type
		h263PacketParserGetBits(&bitBuffer, 3);

        h263PacketParserGetBits(&bitBuffer, 7);
		// aspect ratio
        h263PacketParserGetBits(&bitBuffer, 4); 
		// width
        *width = (h263PacketParserGetBits(&bitBuffer, 9) + 1) * 4;
        h263PacketParserGetBits(&bitBuffer, 1);
		// height
        *height = h263PacketParserGetBits(&bitBuffer, 9) * 4;
		// qscale
		h263PacketParserGetBits(&bitBuffer, 5);
    }

    // PEI
    while (h263PacketParserGetBits(&bitBuffer, 1) != 0) {
        h263PacketParserGetBits(&bitBuffer, 8);
    }

    return 0;
}
