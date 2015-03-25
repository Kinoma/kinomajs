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

#include "FskUtilities.h"
#include "FskEndian.h"
#include "FskInstrumentation.h"
#include "RTPPacketParser.h"
#include "RTPPacketBuffer.h"

#define DEBUG_DUMP_PACKETS	0
#define USE_NULL_PACKET_PARSER 0
#define USE_AVC_PACKET_PARSER 1
#define USE_PACKET_BUFFER 1
#define USE_QT_PACKET_PARSER 0

extern void qtPacketParserInitialize(RTPPacketHandler handler);
extern void nullPacketParserInitialize(RTPPacketHandler handler);
extern void aacPacketParserInitialize(RTPPacketHandler handler);
extern void lpcmPacketParserInitialize(RTPPacketHandler handler);
extern void h263PacketParserInitialize(RTPPacketHandler handler);
extern void mp4vPacketParserInitialize(RTPPacketHandler handler);
extern void amrPacketParserInitialize(RTPPacketHandler handler);
extern void latmPacketParserInitialize(RTPPacketHandler handler);
extern void qcelpPacketParserInitialize(RTPPacketHandler handler);
extern void avcPacketParserInitialize(RTPPacketHandler handler);

#if DEBUG_DUMP_PACKETS
static UInt32 CRC32(UInt8 *ptr, UInt32 bufSize);

static Boolean debuginit = true;
static FILE *f = NULL;
#endif

FskInstrumentedSimpleType(RTPPacketParser, rtppacketparser);

static void sUpParserUse(RTPPacketParser parser);
static void sDownParserUse(RTPPacketParser parser);

/* ----------------------------------------------------------------------------------------------- */

static FskErr getHandlerList(RTPPacketHandler *handlers)
{
	RTPPacketHandler fp;
	FskErr err;
	UInt16 count = 1;	// one for the terminator

#if USE_QT_PACKET_PARSER
	count += 1;		// QT generic payload parser
#endif
	count += 1;		// LPCM parser
	count += 1;		// AAC (generic mpeg-4) parser
	count += 1;		// H.263 (H263-1998/2000) parser
	count += 1;		// MPEG-4 Video (MP4V-ES) parser
	count += 1;		// AMR narrowband (AMR) parser
	count += 1;		// AAC (LATM) parser
	count += 1;		// QCELP parser

#ifdef USE_AVC_PACKET_PARSER
	count += 1;		// AVC parser
#endif

#if USE_NULL_PACKET_PARSER
	count += 1;		// NULL parser
#endif
	count *= sizeof(RTPPacketHandlerRecord);

	err = FskMemPtrNewClear(count, (FskMemPtr*)handlers);
	if (0 != err) goto bail;
	
	fp = *handlers;
#if USE_QT_PACKET_PARSER
	qtPacketParserInitialize(fp++);
#endif
	lpcmPacketParserInitialize(fp++);
	aacPacketParserInitialize(fp++);
	h263PacketParserInitialize(fp++);
	mp4vPacketParserInitialize(fp++);
	amrPacketParserInitialize(fp++);
	latmPacketParserInitialize(fp++);
	qcelpPacketParserInitialize(fp++);

#if USE_AVC_PACKET_PARSER
	avcPacketParserInitialize(fp++);
#endif

#if USE_NULL_PACKET_PARSER
	// Has to be last!
	nullPacketParserInitialize(fp++);
#endif


bail:
	return err;
}

/* ----------------------------------------------------------------------------------------------- */

FskErr RTPPacketParserNew(RTPPacketParser *parser, SDPMediaDescription mediaDescription, char *codecName)
{
	FskErr err = 0;
	RTPPacketHandler handlers = NULL, walker;

	err = getHandlerList(&handlers);
	BAIL_IF_ERR(err);
	
	walker = handlers;
	while (walker->version) {
		if (0 == walker->doCanHandle(mediaDescription, codecName))
			break;
		walker++;
	}
	if (!walker->version) {
		err = kFskErrRTSPPacketParserUnsupportedFormat;
		goto bail;
	}

	err = FskMemPtrNewClear(sizeof(RTPPacketParserRecord), (FskMemPtr*)parser);
	BAIL_IF_ERR(err);

	err = FskMemPtrNew(sizeof(RTPPacketHandlerRecord), (FskMemPtr*)&(*parser)->handler);
	BAIL_IF_ERR(err);
	
	*(*parser)->handler = *walker;
	(*parser)->mediaDescription = mediaDescription;
	(*parser)->mediaFormat = kRTPMediaFormatUnknown;
	sUpParserUse(*parser);

#if USE_PACKET_BUFFER
	{
	RTPPacketBuffer *packetBuffer;
	err = RTPPacketBufferNew(*parser, &packetBuffer, 5);
	BAIL_IF_ERR(err);

	(*parser)->packetBuffer = packetBuffer;
	}
#endif

	if (0 != walker->doNew) {
		walker->doNew(*parser);
	}

bail:
	FskMemPtrDispose(handlers);
	return err;
}

/* ----------------------------------------------------------------------------------------------- */
FskErr RTPPacketParserDispose(RTPPacketParser parser)
{
	FskErr err = 0;

	if(!parser) goto bail;

	if (--parser->useCount > 0)
		return 0;

#if USE_PACKET_BUFFER
	RTPPacketBufferDispose(parser->packetBuffer);
#endif

	if (parser->handler->doDispose)
		parser->handler->doDispose(parser);

	FskMemPtrDispose(parser->handler);

	FskMemPtrDispose(parser);

bail:
	return err;
}

/* ----------------------------------------------------------------------------------------------- */
FskErr RTPPacketParserSetSessionRefCon(RTPPacketParser parser, void *refCon)
{
	FskErr err = 0;

	parser->sessionRefCon = refCon;

	return err;
}

/* ----------------------------------------------------------------------------------------------- */
FskErr RTPPacketParserSetSessionReceivePacketCallback(RTPPacketParser parser, RTPPacketParserReceivePacketCallback callback)
{
	FskErr err = 0;
	
	parser->sessionReceivePacketCB = callback;

	return err;
}

/* ----------------------------------------------------------------------------------------------- */
FskErr RTPPacketParserSetAppRefCon(RTPPacketParser parser, void *refCon)
{
	FskErr err = 0;

	parser->appRefCon = refCon;

	return err;
}

/* ----------------------------------------------------------------------------------------------- */
FskErr RTPPacketParserSetAppReceivePacketCallback(RTPPacketParser parser, RTPPacketParserReceivePacketCallback callback)
{
	FskErr err = 0;
	
	if (NULL == parser) goto bail;

	parser->appReceivePacketCB = callback;

bail:
	return err;
}

/* ----------------------------------------------------------------------------------------------- */

FskErr RTPPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize)
{
	FskErr err = 1;

	if (parser->handler->doGetInfo)
		err = parser->handler->doGetInfo(parser, selector, info, infoSize);

	return err;
}

/* ----------------------------------------------------------------------------------------------- */

FskErr RTPPacketParserSetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 infoSize)
{
	FskErr err = 1;

	if (parser->handler->doSetInfo)
		err = parser->handler->doSetInfo(parser, selector, info, infoSize);

	return err;
}

/* ----------------------------------------------------------------------------------------------- */

FskErr RTPPacketParserFlush(RTPPacketParser parser)
{
	FskErr err = 0;

	if (parser) {
		if (parser->handler->doFlush)
			err = parser->handler->doFlush(parser);
#if USE_PACKET_BUFFER
		RTPPacketBufferFlush(parser->packetBuffer);
#endif
		parser->nextPacketNumber = 0;
	}

	return err;
}

/* ----------------------------------------------------------------------------------------------- */
/*
	The RTP header has the following format:
	0                   1                   2                   3     
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|V=2|P|X|  CC   |M|     PT      |       sequence number         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+    
	|                           timestamp                           |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+    
	|           synchronization source (SSRC) identifier            |
	+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+    
	|            contributing source (CSRC) identifiers             |    
	|                             ....                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	version (V): 2 bits
	padding (P): 1 bit
	extension (X): 1 bit
	CSRC count (CC): 4 bits
	marker (M): 1 bit
	payload type (PT): 7 bits
	sequence number: 16 bits
	timestamp: 32 bits 
	SSRC: 32 bits 
	CSRC list: 0 to 15 items, 32 bits each 

*/
FskErr RTPPacketParserProcessBuffer(RTPPacketParser parser, unsigned char *buffer, UInt32 bufferSize)
{
	FskErr err;
	unsigned char *packet = (unsigned char *)buffer;
	UInt32 packetSize = bufferSize;
	UInt32 rtpHdr;
	UInt32 i;
	RTPPacket rtpPacket = 0;

	sUpParserUse(parser);

	// Check for the 12-byte RTP header
	if (!parser || (packetSize < 12)) {
		err = kFskErrRTSPBadPacket;
		goto bail;
	}
	
	err = FskMemPtrNew(sizeof(RTPPacketRecord), &rtpPacket);
	if (0 != err) goto bail;

	rtpPacket->packetNumber = parser->nextPacketNumber++;
	rtpPacket->totalSamples = -1;

	// Get the first 32 bits in Big Endian
	rtpHdr = FskMisaligned32_GetBtoN(packet); 

	// Version: 2 bits
	rtpPacket->version = (UInt8)((rtpHdr)>>30);
	
	// Padding: 1 bit
	rtpPacket->padding = (UInt8)((rtpHdr>>29)&0x1); 

	// Extension: 1 bit
	rtpPacket->extension = (UInt8)((rtpHdr>>28)&0x7); 

	// CSRC count: 4 bits
	rtpPacket->CSRCcount = (UInt8)((rtpHdr>>24)&0xF);

	// Marker: 1 bit
	rtpPacket->marker = (UInt8)((rtpHdr >> 23)&0xFF);

	// Payload Type: 7 bits
	rtpPacket->payloadType = (UInt8)(rtpHdr>>16)&0x7f;

	// Sequence number: 16 bits
	rtpPacket->sequenceNumber = (UInt16)(rtpHdr&0xFFFF);

	// 2nd 32 bits
	packet+=4;
	packetSize-=4;
	
	// Time Stamp
	rtpPacket->timestamp = FskMisaligned32_GetBtoN(packet);

	// 3rd 32 bits
	packet+=4;
	packetSize-=4;
	
	// SSRC
	rtpPacket->SSRC = FskMisaligned32_GetBtoN(packet);

	packet+=4;
	packetSize-=4;

	// Read the CSRC denoted in the CSRCcount. Max is 15
	for(i = 0; i < rtpPacket->CSRCcount; i++)
	{
		if(rtpPacket->CSRCcount < 15)
			rtpPacket->CSRC[i] = FskMisaligned32_GetBtoN(packet);

		packet+=4;
		packetSize-=4;
	}

	// Check for (& ignore) any RTP header extension
	if (rtpPacket->extension) 
	{
		UInt32 extHdr, remExtSize;
		
		if (packetSize < 4) {
			err = kFskErrRTSPBadPacket;
			goto bail;
		}
		extHdr = FskMisaligned32_GetBtoN(packet); 
		packet+=4;
		packetSize-=4;

		remExtSize = 4*(extHdr&0xFFFF);
		if (packetSize < remExtSize) 
			goto bail;

		packet+=rtpPacket->CSRCcount*remExtSize;
		packetSize-=remExtSize;
	}

	// Discard any padding bytes:
	if (rtpPacket->padding ) 
	{
		UInt32 numPaddingBytes;
		
		if (packetSize == 0) {
			err = kFskErrRTSPBadPacket;
			goto bail;
		}
		
		numPaddingBytes = (UInt32)(packet)[packetSize-1];
		if (packetSize < numPaddingBytes) {
			err = kFskErrRTSPBadPacket;
			goto bail;
		}
		
		packet+=numPaddingBytes;
		packetSize-=numPaddingBytes;
	}  
	
	rtpPacket->dataSize = packetSize;
	err = FskMemPtrNew(rtpPacket->dataSize, (FskMemPtr*)&rtpPacket->data);
	BAIL_IF_ERR(err);
	FskMemMove(rtpPacket->data, packet, rtpPacket->dataSize);
	
	rtpPacket->frames = 0;
	
#if DEBUG_DUMP_PACKETS
	if (debuginit) {
//		f = fopen("packets", "w");
		debuginit = false;
	}
//	if (f)
	{
		fprintf(stderr, "\npacketNumber=%d\n", rtpPacket->packetNumber);
//		fprintf(f, "sequenceNumber=%d\n", rtpPacket->sequenceNumber);
		fprintf(stderr, "data type=0x%x, data size=%d, CRC=0x%x\n", rtpPacket->payloadType, packetSize, CRC32(packet, packetSize));
//		printf("wrote some data for packet #%d\n", rtpPacket->packetNumber);
	}
#endif

	// Call the session callback first, allowing it to reject this packet before doing other work
	if(parser->sessionReceivePacketCB)
	{
		err = parser->sessionReceivePacketCB(rtpPacket, parser->sessionRefCon);
		if (err) 
			goto bail;
	}
	
#if USE_PACKET_BUFFER
	// Push the packet into the packet ordering buffer
	err = RTPPacketBufferPushPacket(parser->packetBuffer, rtpPacket);
	if (err) 
		goto bail;
	
	while(1) 
	{ // Pull available packets from the packet buffer and pass to packet parser
		RTPPacketBufferPullPacket(parser->packetBuffer, 0, &rtpPacket);		
		if( rtpPacket == 0 )
			break;
		
		// Call the specific RTP packet parser
		err = parser->handler->doProcessPacket(parser, rtpPacket);
		if(err != 0)
			goto bail;


		// Call the application's call back with the compressed data
		// Note we call the callback even if there are no frames, so that the client can keep
		// up with the RTP packets.
		if(parser->appReceivePacketCB) {// Call the callback, application is responsible for calling
			err = parser->appReceivePacketCB(rtpPacket, parser->appRefCon);
		}
		else {
			RTPPacketDispose(parser, rtpPacket, true);
			rtpPacket = NULL;
		}
	}
#else
	// Call the specific RTP packet parser
	err = parser->handler->doProcessPacket(parser, rtpPacket);

	if(err == 0)
	{
		// Call the application's call back with the compressed data
		// Note we call the callback even if there are no frames, so that the client can keep
		// up with the RTP packets.
		if(parser->appReceivePacketCB)
		{
			// Call the callback, application is responsible for calling
			err = parser->appReceivePacketCB(rtpPacket, parser->appRefCon);
		} else 
		{
			// Dispose of the data, no one has registered for it
			RTPPacketDispose(parser, rtpPacket, true);
			rtpPacket = NULL;
		}
	}
#endif

bail:
	if (NULL != buffer)
		FskMemPtrDispose(buffer);
	if (0 != err && rtpPacket != 0 ) {
		RTPPacketDispose(parser, rtpPacket, true);
	}

	sDownParserUse(parser);

	return err;
}

/* ----------------------------------------------------------------------------------------------- */
// If 'disposeMediaData' is true, then dispose 'compressedData'
FskErr RTPPacketDispose(RTPPacketParser parser, RTPPacket packet, Boolean disposeMediaData)
{
	FskErr err = 0;

	if (NULL != packet) {
		if (NULL != parser && NULL != parser->handler->doDisposePacket)
			parser->handler->doDisposePacket(parser, packet);
		if (disposeMediaData && (NULL != packet->frames)) {
			RTPCompressedMediaFrame frame, walker;
			walker = packet->frames;
			while (NULL != walker) {
				frame = walker;
				walker = walker->next;
				FskMemPtrDispose(frame->sampleSizes);
				FskMemPtrDispose(frame);
			}
		}
		FskMemPtrDispose(packet->data);
		FskMemPtrDispose(packet);
	}

	return err;
}

void sUpParserUse(RTPPacketParser parser)
{
	if (parser)
		parser->useCount++;
}

void sDownParserUse(RTPPacketParser parser)
{
	if (NULL != parser) {
		parser->useCount--;
		if (parser->useCount < 1) {
			RTPPacketParserDispose(parser);
		}
	}
}

/* ----------------------------------------------------------------------------------------------- */
#if DEBUG_DUMP_PACKETS
static UInt32 CRC32(UInt8 *ptr, UInt32 bufSize)
{
	UInt32 hash = 0;
	UInt8 byteVal;
	int i;

	while (bufSize > 0)
	{
		byteVal = *ptr;
		for (i = 7; i >= 0; i--) {
			UInt8 nextBit, MSB;
			MSB = hash >> 31;
			nextBit = (byteVal >> i) & 1;
			if ((MSB ^ nextBit) == 1)
				hash = (hash << 1) ^ 0x04C11DB7;
			else
				hash = hash << 1;
		}
		ptr++;
		bufSize--;
	}

	return hash;
}
#endif

/* ----------------------------------------------------------------------------------------------- */

int PushDataToLinkedFrames( int compositionTimeStamp, int decodeTimeStamp, int size, unsigned char *dataPtr, RTPCompressedMediaFrame *storedFrames )
{
	RTPCompressedMediaFrame	frame = NULL;
	UInt32					err   = 0;

	err	= FskMemPtrNewClear( size + sizeof(RTPCompressedMediaFrameRecord), &frame);
	if (0 != err) goto bail;

	frame->compositionTimeStamp = compositionTimeStamp; 
	frame->decodeTimeStamp		= decodeTimeStamp;
	frame->next					= NULL;
	frame->length				= size;
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

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserTypeInstrumentation, "						append: %d", size);

bail:
	if( err != 0  && frame != NULL )
		FskMemPtrDispose(frame);

	return err;
}

int GetLinkededFramesSize( RTPCompressedMediaFrame storedFrames, UInt32	*storedSizeOut )
{
	UInt32	storedSize		= 0;
	
	// if we have saved data, get it and prepend it to the current data
	while(storedFrames)
	{
		storedSize += storedFrames->length;
		storedFrames = storedFrames->next;
	}

	*storedSizeOut = storedSize;

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserTypeInstrumentation, "						GetLinkededFramesSize: %d", storedSize);

	return 0;
}


int MoveLinkedFramesDataToFrame( RTPCompressedMediaFrame storedFrames, RTPCompressedMediaFrame frame )
{
	int				copiedSize = 0;
	unsigned char	*pData = NULL;
	int				compositionTimeStamp=0;		// used by player
	int				decodeTimeStamp=0;
	
	frame->next	= NULL;
	
	// Walk our list again and really prepend the data
	pData = (unsigned char *)( frame + 1 ); // Move pass the header
	
	while(storedFrames)
	{
		RTPCompressedMediaFrame currStoredFrames = storedFrames;
		UInt32					currStoredSize	 = storedFrames->length;
		
		compositionTimeStamp = storedFrames->compositionTimeStamp;
		decodeTimeStamp		 = storedFrames->decodeTimeStamp;
		FskMemMove(pData, storedFrames+1, currStoredSize);
		storedFrames = storedFrames->next;
		
		// Remove our stored data from our list
		FskMemPtrDispose(currStoredFrames);
		currStoredFrames = NULL;
		
		// increment our frame ptr
		pData		+= currStoredSize;
		copiedSize	+= currStoredSize;
	}

	frame->compositionTimeStamp	= compositionTimeStamp;
	frame->decodeTimeStamp		= decodeTimeStamp;
	frame->length				= copiedSize;

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserTypeInstrumentation, "						MoveLinkedFramesDataToFrame: %d", frame->length);

	return 0;
}

int ClearLinkedFrames( RTPCompressedMediaFrame storedFrames )
{
	while(storedFrames)
	{
		RTPCompressedMediaFrame currStoredFrames = storedFrames;
		
			storedFrames = storedFrames->next;
		
		// Remove our stored data from our list
		FskMemPtrDispose(currStoredFrames);
		currStoredFrames = NULL;
	}

	FskInstrumentedTypePrintfDebug(&gRTPPacketParserTypeInstrumentation, "						ClearLinkedFrames");

	return 0;
}
