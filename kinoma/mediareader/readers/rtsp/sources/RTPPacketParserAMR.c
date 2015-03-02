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

/*
	RFC-3267
*/

#include "FskUtilities.h"
#include "FskEndian.h"
#include "RTPPacketParser.h"

#define AMR_CHUNKY_STYLE 0

#define AMR_NO_DATA 15

static SInt32 AMRFrameTypeToOctets[] =
{
	12,
	13,
	15,
	17,
	19,
	20,
	26,
	31,
	5,
	6,
	5,
	5,
	-1,		// undefined
	-1,		// undefined
	-1,		// undefined
	0		// AMR_NO_DATA
};

// Private state
typedef struct {
	Boolean crc;
} AMRPacketParserRecord, *AMRPacketParser;

void amrPacketParserInitialize(RTPPacketHandler handler);


static FskErr amrPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName);
static FskErr amrPacketParserNew(RTPPacketParser parser);
static FskErr amrPacketParserDispose(RTPPacketParser parser);
static FskErr amrPacketParserProcessPacket(RTPPacketParser parser, RTPPacket packet);
static FskErr amrPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet);
static FskErr amrPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize);

/* -----------------------------------------------------------------------*/

void amrPacketParserInitialize(RTPPacketHandler handler)
{
	handler->version = 1;

	handler->doCanHandle = amrPacketParserCanHandle;
	handler->doNew = amrPacketParserNew;
	handler->doDispose = amrPacketParserDispose;
	handler->doProcessPacket = amrPacketParserProcessPacket;
	handler->doDisposePacket = amrPacketParserDisposePacket;
	handler->doGetInfo = amrPacketParserGetInfo;
}

/* -----------------------------------------------------------------------*/

FskErr amrPacketParserNew(RTPPacketParser parser)
{
	FskErr err = kFskErrNone;
	AMRPacketParser amrPacketParser;
	SDPMediaDescription mediaDescription;

	err = FskMemPtrNewClear(sizeof(AMRPacketParserRecord), &amrPacketParser);
	if (0 != err) goto bail;
	
	parser->mediaFormat = kRTPAudioFormatAMR;
	parser->handlerRefCon = amrPacketParser;

	mediaDescription = parser->mediaDescription;
	if (NULL != mediaDescription) {
		SDPAttribute attribute;

		// Grab what we need from the "fmtp" attribute
		attribute = SDPFindMediaAttribute(mediaDescription, "fmtp");
		if (NULL != attribute) {
			char *attr;
			UInt32 valueNum;
			
			if (0 != copyAttributeValue(attribute->value, "crc", &attr)) {
				valueNum = FskStrToNum(attr);
				if (1 == valueNum)
					amrPacketParser->crc = true;
				FskMemPtrDispose(attr);
			}
		}
	}

bail:
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr amrPacketParserDispose(RTPPacketParser parser)
{
	AMRPacketParser amrPacketParser = (AMRPacketParser)parser->handlerRefCon;

	if (amrPacketParser) {
		FskMemPtrDispose(amrPacketParser);
	}

	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr amrPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName)
{
	Boolean canHandle = false;
	char *value = 0;
	UInt32 valueNum;

	if (0 != FskStrCompareCaseInsensitive("AMR", encodingName)) goto bail;

	if (NULL != mediaDescription) {
		SDPAttribute attribute;
		
		// We only handle AMR/8000/1
		attribute = SDPFindMediaAttribute(mediaDescription, "rtpmap");
		if (NULL != attribute) {
			char *value, *parts[4];
			UInt16 nParts = 2;
			UInt32 sampleRate, nChannels = 1;
			
			value = FskStrDoCopy(attribute->value);
			splitToken(value, &nParts, ' ', &parts[0]);
			if (nParts > 1) {
				nParts = 3;
				splitToken(parts[1], &nParts, '/', &parts[0]);
				if (nParts > 2) {
					sampleRate = FskStrToNum(parts[1]);
					nChannels = (UInt16)FskStrToNum(parts[2]);
					if (1 != nChannels || 8000 != sampleRate)
						goto bail;
				}
			}
				
			FskMemPtrDisposeAt(&value);
		}

		attribute = SDPFindMediaAttribute(mediaDescription, "fmtp");
		if (NULL != attribute) {
		
			// We don't handle interleaved frames
			if (0 != copyAttributeValue(attribute->value, "interleaving", &value)) {
				valueNum = FskStrToNum(value);
				if (1 == valueNum) goto bail;
				FskMemPtrDisposeAt(&value);
			}
			
			// We don't handle robust sorting
			if (0 != copyAttributeValue(attribute->value, "robust-sorting", &value)) {
				valueNum = FskStrToNum(value);
				if (1 == valueNum) goto bail;
				FskMemPtrDisposeAt(&value);
			}

			// We only handle octet-aligned payload types
			if (0 != copyAttributeValue(attribute->value, "octet-align", &value)) {
				valueNum = FskStrToNum(value);
				if (0 == valueNum) goto bail;
				FskMemPtrDisposeAt(&value);
				canHandle = true;
			}
		}
	}

bail:
	FskMemPtrDispose(value);
	return canHandle ? 0 : kFskErrRTSPPacketParserUnsupportedFormat;
}

/* -----------------------------------------------------------------------*/

FskErr amrPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize)
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
			*(UInt32*)info = kRTPAudioFormatAMR;
			break;

		default:
			err = kFskErrInvalidParameter;
			break;
	}
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr amrPacketParserProcessPacket(RTPPacketParser parser, RTPPacket rtpHeader)
{
	UInt8 *p = rtpHeader->data;
	UInt32 nFrames, i;
	UInt8 CMR, F, R, FT;
	FskErr err = 0;
	UInt8 *ToCs;
	
	CMR = (*p >> 4) & 0xF;
	R = (*p & 0x0F);
	
	// From RFC-3267:
	//  R: is a reserved bit that MUST be set to zero.  All R bits MUST be
	//     ignored by the receiver.
	// ...
	//	If receiving a payload with a CMR value which is not a speech mode or
	//	NO_DATA, the CMR MUST be ignored by the receiver.
	//
	// So don't reject these packets...
	if (0 != R || (CMR > 7 && CMR < 15)) {
/*
		err = kFskErrRTSPBadPacket;
		goto bail;
*/
	}
		
	++p;
	nFrames = 0;
	
	// Read the ToC
	ToCs = p;
	do {
		F = *p & 0x80;
		FT = (*p >> 3) & 0xF;
		if (FT >= 9 && FT <= 14) {
			err = kFskErrRTSPPacketParserUnsupportedFormat;
			goto bail;
		}
			
		++nFrames;		// count AMR_NO_DATA frames as well, so their ToC entries are written out
		++p;
	} while (F != 0);

	// Pack all the AMR frames into one RTP packet frame for efficiency
	if (0 != nFrames) {
		RTPCompressedMediaFrame frame = NULL;
		UInt32 framesSize = rtpHeader->dataSize - (UInt32)(p - rtpHeader->data);
		UInt32 frameSize;
		UInt8 *dst, ToC;

		err = FskMemPtrNewClear(sizeof(RTPCompressedMediaFrameRecord) + framesSize + nFrames, &frame);
		if (0 != err || NULL == frame) goto bail;
		
#if AMR_CHUNKY_STYLE
		err = FskMemPtrNewClear(sizeof(UInt32) * (1 + 1), (FskMemPtr*)&frame->sampleSizes);
#else
		err = FskMemPtrNew(sizeof(UInt32) * (nFrames + 1), (FskMemPtr*)&frame->sampleSizes);
#endif
		if (0 != err) goto bail;
		
		rtpHeader->frames = frame;
		dst = (UInt8 *)(frame + 1);
		// insert a ToC byte before each AMR frame
		for (i = 0; i < nFrames; i++) {
			ToC = ToCs[i] & 0x7F;
			FT = (ToC >> 3) & 0xF;
			
			frameSize = AMRFrameTypeToOctets[FT];
#if AMR_CHUNKY_STYLE
			frame->sampleSizes[0] += frameSize + 1;
#else
			frame->sampleSizes[i] = frameSize + 1;	// plus one for ToC
#endif
			
			// prefix a ToC
			*dst++ = ToC;
			// copy the frame
			FskMemMove(dst, p, frameSize);
			p	+= frameSize;
			dst	+= frameSize;
		}
#if AMR_CHUNKY_STYLE
		frame->sampleSizes[1] = 0;
#else
		frame->sampleSizes[nFrames] = 0;
#endif
		frame->length = framesSize + nFrames;
		frame->next = NULL;
		
		rtpHeader->totalSamples = nFrames * 160;
	}
	
bail:
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr amrPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet)
{
	return 0;
}

/* -----------------------------------------------------------------------*/

