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
#include "RTPPacketParser.h"
#include "FskInstrumentation.h"

FskInstrumentedSimpleType(RTPPacketParserQCELP, rtppacketparserqcelp);

#if SUPPORT_INSTRUMENTATION
	#define	LOG(...)	do { FskInstrumentedTypePrintfDebug  (&gRTPPacketParserQCELPTypeInstrumentation, __VA_ARGS__); } while(0)
#else
	#define LOG(...)
#endif

#define DEBUG_EMBED_COUNT	0

#define PT_QCELP	12

// total RTP frame size (incl. 1-byte frame type) in bytes for a given frame type
static SInt16 OctetZeroToFrameSize[] =
{
	1,
	4,
	8,
	17,
	35
};

static UInt16 nOctetZeroValues = 5;

// Private state
typedef struct {
	UInt8 *frames;
	UInt32 size;
	UInt32 frameCount;
} PacketRecord;

typedef struct {
	PacketRecord *packets;
	UInt8 B;				// bundling value (number of frames in an RTP packet)
	UInt8 L;				// interleave value (there are L+1 RTP packets in an interleave group)
	UInt8 nextExpectedN;	// N+1 for last packet accounted for
	UInt32 nextExpectedTime;// timestamp expected for the next incoming packet
							// A future packet can use this as a reference point to detect packet loss
							// and insert an appropriate number of erasure frames.
	Boolean initGroup;		// are we in the middle of an interleave group?
	UInt8 initialB;			// keep this around to know how many arrays to dispose
#if DEBUG_EMBED_COUNT	
	UInt8 count;
#endif
} QCELPPacketParserRecord, *QCELPPacketParser;

void qcelpPacketParserInitialize(RTPPacketHandler handler);


static FskErr qcelpPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName);
static FskErr qcelpPacketParserNew(RTPPacketParser parser);
static FskErr qcelpPacketParserDispose(RTPPacketParser parser);
static FskErr qcelpPacketParserProcessPacket(RTPPacketParser parser, RTPPacket packet);
static FskErr qcelpPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet);
static FskErr qcelpPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize);
static FskErr qcelpPacketParserFlush(RTPPacketParser parser);

static FskErr emitInterleaveGroup(RTPPacketParser parser, RTPPacket rtpHeader);

/* -----------------------------------------------------------------------*/

static FskErr emitInterleaveGroup(RTPPacketParser parser, RTPPacket rtpHeader)
{
	FskErr err = kFskErrNone;
	QCELPPacketParser qcelpPacketParser = (QCELPPacketParser)parser->handlerRefCon;
	
	// flush parser state by serializing all frames in all packets
	// in the current interleave group
	
	// put all frames into one big buffer to simplify handling of trailing erasure frames
	
	UInt8 *dst;
	UInt32 framesSize = 0;
	UInt16 i;
	UInt32 sampleSizesSize = 0;
	UInt32 totalFrameCount = 0;
	
	if (!qcelpPacketParser || !rtpHeader) goto bail;
	
	for (i = 0; i < qcelpPacketParser->B; i++) {
		framesSize += qcelpPacketParser->packets[i].size;
	}
	if (framesSize == 0) goto bail;
	
	// only alloc these once, realloc thereafter
	if (!rtpHeader->frames) {
		err = FskMemPtrNewClear(sizeof(RTPCompressedMediaFrameRecord) + framesSize, (FskMemPtr*)&rtpHeader->frames);
		if (0 != err) goto bail;
		err = FskMemPtrNew(sizeof(UInt32) * (1 + 1), (FskMemPtr*)&rtpHeader->frames->sampleSizes);
		if (0 != err) goto bail;
	}
	else {
		err = FskMemPtrRealloc(sizeof(RTPCompressedMediaFrameRecord) + rtpHeader->frames->length + framesSize, (FskMemPtr*)&rtpHeader->frames);
		if (0 != err) goto bail;
		
		// determine how big sampleSizes currently is (excluding trailing zero)
		while (rtpHeader->frames->sampleSizes[sampleSizesSize] != 0) {
			sampleSizesSize++;
		}
		
		err = FskMemPtrRealloc(sizeof(UInt32) * (sampleSizesSize + 1 + 1), (FskMemPtr*)&rtpHeader->frames->sampleSizes);
		if (0 != err) goto bail;
	}

	dst = (UInt8 *)(rtpHeader->frames + 1) + rtpHeader->frames->length;

	for (i = 0; i < qcelpPacketParser->B; i++) {
		FskMemMove(dst, qcelpPacketParser->packets[i].frames, qcelpPacketParser->packets[i].size);

#if DEBUG_EMBED_COUNT
		*dst = qcelpPacketParser->count++;
#endif
		
		dst += qcelpPacketParser->packets[i].size;
		totalFrameCount += qcelpPacketParser->packets[i].frameCount;
	}
	
	rtpHeader->frames->sampleSizes[sampleSizesSize] = framesSize;
	rtpHeader->frames->sampleSizes[1+sampleSizesSize] = 0;
	rtpHeader->frames->length += framesSize;
	rtpHeader->frames->next = NULL;

	if (rtpHeader->totalSamples == -1)
		rtpHeader->totalSamples = 0;
	rtpHeader->totalSamples += totalFrameCount * 160;

	// modify the outgoing timestamp to correspond to the beginning of this interleave group
	rtpHeader->timestamp = qcelpPacketParser->nextExpectedTime - rtpHeader->totalSamples;
	
	LOG("Emit group t, samples", rtpHeader->timestamp, rtpHeader->totalSamples);

	// reset some state, but leave nextExpectedTime alone
	qcelpPacketParser->initGroup = false;
	qcelpPacketParser->nextExpectedN = 0;
	for (i = 0; i < qcelpPacketParser->B; i++) {
		qcelpPacketParser->packets[i].size = 0;
		qcelpPacketParser->packets[i].frameCount = 0;
	}
bail:
	return err;
}

/* -----------------------------------------------------------------------*/

void qcelpPacketParserInitialize(RTPPacketHandler handler)
{
	handler->version = 1;

	handler->doCanHandle = qcelpPacketParserCanHandle;
	handler->doNew = qcelpPacketParserNew;
	handler->doDispose = qcelpPacketParserDispose;
	handler->doProcessPacket = qcelpPacketParserProcessPacket;
	handler->doDisposePacket = qcelpPacketParserDisposePacket;
	handler->doGetInfo = qcelpPacketParserGetInfo;
	handler->doFlush = qcelpPacketParserFlush;
}

/* -----------------------------------------------------------------------*/

FskErr qcelpPacketParserNew(RTPPacketParser parser)
{
	FskErr err = 0;
	QCELPPacketParser qcelpPacketParser;

	err = FskMemPtrNewClear(sizeof(QCELPPacketParserRecord), &qcelpPacketParser);
	if (0 != err) goto bail;
	
	parser->mediaFormat = kRTPAudioFormatQCELP;
	parser->handlerRefCon = qcelpPacketParser;

bail:
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr qcelpPacketParserDispose(RTPPacketParser parser)
{
	QCELPPacketParser qcelpPacketParser = (QCELPPacketParser)parser->handlerRefCon;

	if (qcelpPacketParser) {
		if (qcelpPacketParser->packets) {
			UInt16 i;
			for (i = 0; i < qcelpPacketParser->initialB; i++) {
				FskMemPtrDispose(qcelpPacketParser->packets[i].frames);
			}
			FskMemPtrDispose(qcelpPacketParser->packets);
		}
		FskMemPtrDispose(qcelpPacketParser);
	}

	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr qcelpPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName)
{
	// QCELP is a static payload type, so encodingName isn't in the SDP and can be NULL
	if (NULL != mediaDescription) {
		SDPMediaFormat format = mediaDescription->formatList->head;
		if (PT_QCELP == format->payloadType)  // @@ what about the other types in the payload type list?
			return 0;
	}
	// although apparently Real has chosen to make it a dynamic type...
	if (0 == FskStrCompareCaseInsensitive("QCELP", encodingName)) return 0;

	return kFskErrRTSPPacketParserUnsupportedFormat;
}

/* -----------------------------------------------------------------------*/

FskErr qcelpPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize)
{
	FskErr err = 0;

	switch(selector) {
		case kRTPPacketParserSelectorRTPTimeScale:
		case kRTPPacketParserSelectorAudioSampleRate:
			*(UInt32*)info = 8000;
			break;
		case kRTPPacketParserSelectorAudioChannels:
			*(UInt32*)info = 1;
			break;
		case kRTPPacketParserSelectorMediaFormat:
			*(UInt32*)info = kRTPAudioFormatQCELP;
			break;
		case kRTPPacketParserSelectorHandleDroppedPackets:
			*(UInt8*)info = 1;
			break;

		default:
			err = kFskErrInvalidParameter;
			break;
	}
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr qcelpPacketParserProcessPacket(RTPPacketParser parser, RTPPacket rtpHeader)
{
	QCELPPacketParser qcelpPacketParser = (QCELPPacketParser)parser->handlerRefCon;
	FskErr err = 0;
	UInt8 *p = rtpHeader->data;
	UInt8 flags, L, N;
	UInt16 B = 0;
	UInt32 framesSize = 0;
	UInt16 i = 0;
	Boolean trailingErasure = false;
	UInt32 savedTimestamp;

	flags = *p++;
	L = (flags & 0x38) >> 3;
	N = flags & 0x07;
	// upper 2 bits are ignored

	if (L > 5 || N > L) {
		err = kFskErrRTSPBadPacket;
		goto bail;
	}
	
	savedTimestamp = rtpHeader->timestamp;
	
	// count up the frames to find B (bundling value)
	while ((UInt32)(p - rtpHeader->data) < rtpHeader->dataSize) {
		if (*p >= nOctetZeroValues) {
			// invalid packet
			err = kFskErrRTSPBadPacket;
			goto bail;
		}
		B++;
		framesSize += OctetZeroToFrameSize[*p];
		p += OctetZeroToFrameSize[*p];
	}
	
	LOG("L=%u N=%u B=%u", L, N, B);
	
	if (!qcelpPacketParser->packets) {
		// L and B are guaranteed not to increase for this SSRC, so we can
		// safely allocate all necessary memory once.
		err = FskMemPtrNewClear(B * sizeof(PacketRecord), (FskMemPtr*)&qcelpPacketParser->packets);
		if (0 != err) goto bail;
		for (i = 0; i < B; i++) {
			err = FskMemPtrNew((L+1) * OctetZeroToFrameSize[4], &qcelpPacketParser->packets[i].frames);
			if (0 != err) goto bail;
		}
		qcelpPacketParser->initialB = (UInt8)B;
	}

	p = rtpHeader->data + 1;  // skip flags

	LOG("seq#,time", rtpHeader->sequenceNumber, savedTimestamp);
	LOG("expected time", qcelpPacketParser->nextExpectedTime);
	
	// detect (using timestamps) any packet loss here, emit (if appropriate)
	// and insert erasure frames first
	if (qcelpPacketParser->nextExpectedTime > 0) {
		if (savedTimestamp < qcelpPacketParser->nextExpectedTime) {
			// this packet contains frames older than what we care about,
			// dump it.
			LOG("threw away old packet");
			err = kFskErrRTSPBadPacket;
			goto bail;				
		}
		
// slightly safer calculation:
// edit: this calc assumes erasure frames need to be emitted in bundles... in some cases
// (when packet timestamps don't align) you need to emit just a few erasure frames
//		while (savedTimestamp >= qcelpPacketParser->nextExpectedTime + 160) {
		while (savedTimestamp > qcelpPacketParser->nextExpectedTime) {
			// When a gap is between interleave groups, just fill it with enough erasure frames.
			// When a gap is within an interleave group, we need special logic to fill in enough
			// data to compensate for a lost packet
			if (0 == qcelpPacketParser->nextExpectedN) {
				i = 0;
				LOG("inserting erasure frames from t1 to t2", qcelpPacketParser->nextExpectedTime, savedTimestamp);
				while (savedTimestamp > qcelpPacketParser->nextExpectedTime) {
					qcelpPacketParser->packets[i].frames[qcelpPacketParser->packets[i].size] = 0x0E;
					qcelpPacketParser->packets[i].size++;
					qcelpPacketParser->packets[i].frameCount++;
					i = (i+1) % qcelpPacketParser->B;
					qcelpPacketParser->nextExpectedTime += 160;
				}
				err = emitInterleaveGroup(parser, rtpHeader);
				trailingErasure = true;
				LOG("emitted erasure group1, next time=", qcelpPacketParser->nextExpectedTime);
			}
			else {
				// generate B erasure frames for a lost packet
				for (i = 0; i < qcelpPacketParser->B; i++) {
					qcelpPacketParser->packets[i].frames[qcelpPacketParser->packets[i].size] = 0x0E;
					qcelpPacketParser->packets[i].size++;
					qcelpPacketParser->packets[i].frameCount++;
				}
				qcelpPacketParser->nextExpectedTime += 160;
				qcelpPacketParser->nextExpectedN++;
				LOG("1 erasure packet up to time", qcelpPacketParser->nextExpectedTime);
				// emit an interleave group's worth of frames at a time
				if (qcelpPacketParser->nextExpectedN == L+1) {
					qcelpPacketParser->nextExpectedTime = qcelpPacketParser->nextExpectedTime
														  - (160 * (qcelpPacketParser->L+1))
														  + (160 * qcelpPacketParser->B * (qcelpPacketParser->L+1));
					err = emitInterleaveGroup(parser, rtpHeader);
					// since we finished off that group with an erasure frame, we need to
					// make sure to include the first frame of the current packet at the end
					// so the the QCELP decoder doesn't get a buffer that ends in an
					// erasure frame
					trailingErasure = true;
					LOG("emitted erasure group2, next time=", qcelpPacketParser->nextExpectedTime);
				}
			}
		}
		
		
	}
	else {
		// must be starting up either for the first time or after a flush
		
		// this packet may not be the first in the interleave group, however.
		// *should* insert enough erasure frames to match nextExpectedN and N
		
		// OR let the N-mismatch code below throw away packets from this group
		// until we hit the next group!

		// XXX it may not be acceptable to dump the first group
		// upon packet loss, as the audio queue may appear dry
		// for quite some time...		
	}
	
	if (N != qcelpPacketParser->nextExpectedN) {
		// either we're starting up with a packet other than the
		// first in an interleave group, or something very wrong has happened.
		// ignore this packet
		
		LOG("threw away N mismatch");
		err = kFskErrRTSPBadPacket;
		goto bail;
	}		

	if (!qcelpPacketParser->initGroup) {
		// initialize state for this group
		qcelpPacketParser->B = (UInt8)B;
		qcelpPacketParser->L = L;
		qcelpPacketParser->initGroup = true;
	}
	
	// append all frames in this packet to the group's arrays
	for (i = 0; i < B; i++) {
		FskMemMove(qcelpPacketParser->packets[i].frames + qcelpPacketParser->packets[i].size, p, OctetZeroToFrameSize[*p]);
		qcelpPacketParser->packets[i].size += OctetZeroToFrameSize[*p];
		qcelpPacketParser->packets[i].frameCount++;
		p += OctetZeroToFrameSize[*p];
	}
	
	qcelpPacketParser->nextExpectedN++;
	qcelpPacketParser->nextExpectedTime = savedTimestamp + 160;

	if (trailingErasure) {
		// move off the first array's contents to pad the end of the last group
		// this avoids any buffers that end with erasure frames
		PacketRecord *packet = &qcelpPacketParser->packets[0];
		UInt8 *dst;
		UInt32 sampleSizesSize = 0;
		
		LOG("Trailing erasure in last emit @", rtpHeader->frames->length);
		
		err = FskMemPtrRealloc(sizeof(RTPCompressedMediaFrameRecord) + rtpHeader->frames->length + packet->size, (FskMemPtr*)&rtpHeader->frames);
		if (0 != err) goto bail;
		
		// determine how big sampleSizes currently is (excluding trailing zero)
		while (rtpHeader->frames->sampleSizes[sampleSizesSize] != 0) {
			sampleSizesSize++;
		}

		dst = (UInt8 *)(rtpHeader->frames + 1) + rtpHeader->frames->length;
		FskMemMove(dst, packet->frames, packet->size);
		
		rtpHeader->frames->sampleSizes[sampleSizesSize-1] += packet->size;
		rtpHeader->frames->length += packet->size;
		rtpHeader->totalSamples += packet->frameCount * 160;

		LOG("Added X frames with size Y", packet->frameCount, packet->size);
		
		packet->size = 0;
		packet->frameCount = 0;
	}

	// if this is the last packet of the group, emit the group
	if (qcelpPacketParser->nextExpectedN == L+1) {
		// this hairy timestamp calculation for the beginning of the next group
		// is necessary because packet timestamps within an interleave group only
		// differ by 160, so skip back to beginning timestamp and move forward
		// using the number of frames in the group
		qcelpPacketParser->nextExpectedTime = qcelpPacketParser->nextExpectedTime - (160 * (L+1)) + (160 * B * (L+1));
		err = emitInterleaveGroup(parser, rtpHeader);
		LOG("finished group, next time=", qcelpPacketParser->nextExpectedTime);
	}
bail:
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr qcelpPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet)
{
	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr qcelpPacketParserFlush(RTPPacketParser parser)
{
	QCELPPacketParser qcelpPacketParser = (QCELPPacketParser)parser->handlerRefCon;
	if(qcelpPacketParser)
	{
		UInt16 i;
		
		qcelpPacketParser->initGroup = false;
		qcelpPacketParser->nextExpectedN = 0;
		qcelpPacketParser->nextExpectedTime = 0;
		
		for (i = 0; i < qcelpPacketParser->B; i++) {
			qcelpPacketParser->packets[i].size = 0;
			qcelpPacketParser->packets[i].frameCount = 0;
		}
	}

	return 0;
}

/* -----------------------------------------------------------------------*/
