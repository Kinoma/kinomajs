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
#include "RTPPacketParser.h"

// Private state
typedef struct {
	UInt32 mediaFormat;
	UInt32 sampleRate;
	UInt16 bitsPerSample;
	UInt16 nChannels;
} LPCMPacketParserRecord, *LPCMPacketParser;

void lpcmPacketParserInitialize(RTPPacketHandler handler);

static FskErr lpcmPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName);
static FskErr lpcmPacketParserNew(RTPPacketParser parser);
static FskErr lpcmPacketParserDispose(RTPPacketParser parser);
static FskErr lpcmPacketParserProcessPacket(RTPPacketParser parser, RTPPacket packet);
static FskErr lpcmPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet);
static FskErr lpcmPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize);

/* -----------------------------------------------------------------------*/

void lpcmPacketParserInitialize(RTPPacketHandler handler)
{
	handler->version = 1;

	handler->doCanHandle = lpcmPacketParserCanHandle;
	handler->doNew = lpcmPacketParserNew;
	handler->doDispose = lpcmPacketParserDispose;
	handler->doProcessPacket = lpcmPacketParserProcessPacket;
	handler->doDisposePacket = lpcmPacketParserDisposePacket;
	handler->doGetInfo = lpcmPacketParserGetInfo;
}

/* -----------------------------------------------------------------------*/

FskErr lpcmPacketParserNew(RTPPacketParser parser)
{
	LPCMPacketParser lpcmPacketParser;
	SDPMediaDescription mediaDescription;
	FskErr err;

	err = FskMemPtrNewClear(sizeof(LPCMPacketParserRecord), &lpcmPacketParser);
	BAIL_IF_ERR(err);
	parser->handlerRefCon = lpcmPacketParser;

	// Tuck away the sample size, sample rate, and bits per sample
	mediaDescription = parser->mediaDescription;
	if (NULL != mediaDescription) {
		SDPAttribute attribute;
		attribute = SDPFindMediaAttribute(mediaDescription, "rtpmap");
		if (NULL != attribute) {
			char *value, *parts[3];
			UInt16 nParts = 2;
			value = FskStrDoCopy(attribute->value);
			splitToken(value, &nParts, ' ', &parts[0]);
			if (*parts[1] == 'L') {
				nParts = 3;
				splitToken(parts[1]+1, &nParts, '/', &parts[0]);
				lpcmPacketParser->bitsPerSample = (UInt16)FskStrToNum(parts[0]);
				lpcmPacketParser->sampleRate = FskStrToNum(parts[1]);
				lpcmPacketParser->nChannels = (UInt16)FskStrToNum(parts[2]);
				parser->mediaFormat = (16 == lpcmPacketParser->bitsPerSample ? kRTPAudioFormatPCM16BitBigEndian : kRTPAudioFormatPCM8BitOffsetBinary);
				lpcmPacketParser->mediaFormat = parser->mediaFormat;
			}
			FskMemPtrDispose(value);
		}
	}

bail:
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr lpcmPacketParserDispose(RTPPacketParser parser)
{
	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr lpcmPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName)
{
	if (0 == FskStrCompareCaseInsensitive("L8", encodingName)) return 0;
	if (0 == FskStrCompareCaseInsensitive("L16", encodingName)) return 0;

	return kFskErrRTSPPacketParserUnsupportedFormat;
}

/* -----------------------------------------------------------------------*/

FskErr lpcmPacketParserProcessPacket(RTPPacketParser parser, RTPPacket rtpHeader)
{
	LPCMPacketParser lpcmPacketParser = (LPCMPacketParser)parser->handlerRefCon;
	RTPCompressedMediaFrame frame;
	FskErr err;

	err = FskMemPtrNewClear(rtpHeader->dataSize + sizeof(RTPCompressedMediaFrameRecord), &frame);
	BAIL_IF_ERR(err);

	rtpHeader->frames = frame;
	frame->length = rtpHeader->dataSize;
	FskMemMove(frame + 1, rtpHeader->data, rtpHeader->dataSize);
	
	rtpHeader->totalSamples = frame->length / (lpcmPacketParser->bitsPerSample/8) / lpcmPacketParser->nChannels;

bail:
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr lpcmPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet)
{
	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr lpcmPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize)
{
	FskErr err = kFskErrNone;
	LPCMPacketParser lpcmPacketParser = (LPCMPacketParser)parser->handlerRefCon;
	
	switch(selector) {
		case kRTPPacketParserSelectorRTPTimeScale:
		case kRTPPacketParserSelectorAudioSampleRate:
			*(UInt32*)info = lpcmPacketParser->sampleRate;
			break;
		case kRTPPacketParserSelectorAudioChannels:
			*(UInt32*)info = lpcmPacketParser->nChannels;
			break;
		case kRTPPacketParserSelectorAudioBitsPerSample:
			*(UInt32*)info = lpcmPacketParser->bitsPerSample;
			break;
		case kRTPPacketParserSelectorMediaFormat:
			*(UInt32*)info = lpcmPacketParser->mediaFormat;
			break;
			
		default:
			err = kFskErrInvalidParameter;
			break;
	}

	return err;
}
 

