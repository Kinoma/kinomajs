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
	RFC-3016 - Low-overhead MPEG-4 Audio Transport Multiplex
	ISO/IEC 14496-3
*/

#include "FskUtilities.h"
#include "FskEndian.h"
#include "RTPPacketParser.h"

// Private state
typedef struct {
	char *config;		// audio config hex string
	UInt8 esds[32];
	UInt32 esdsSize;

	UInt32 mediaFormat;
	UInt32 sampleRate;
	UInt32 bitsPerSample;
	UInt32 nChannels;
	UInt32 nSubFrames;
	
	RTPCompressedMediaFrame frames;
} LATMPacketParserRecord, *LATMPacketParser;

void latmPacketParserInitialize(RTPPacketHandler handler);


static FskErr latmPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName);
static FskErr latmPacketParserNew(RTPPacketParser parser);
static FskErr latmPacketParserDispose(RTPPacketParser parser);
static FskErr latmPacketParserProcessPacket(RTPPacketParser parser, RTPPacket packet);
static FskErr latmPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet);
static FskErr latmPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize);

static Boolean validateConfig(char *config, UInt32 *audioChannels, UInt32 *sampleRate, UInt32 *subFrames, UInt8 *esds, UInt32 *esdsSize);

/* -----------------------------------------------------------------------*/

void latmPacketParserInitialize(RTPPacketHandler handler)
{
	handler->version = 1;

	handler->doCanHandle = latmPacketParserCanHandle;
	handler->doNew = latmPacketParserNew;
	handler->doDispose = latmPacketParserDispose;
	handler->doProcessPacket = latmPacketParserProcessPacket;
	handler->doDisposePacket = latmPacketParserDisposePacket;
	handler->doGetInfo = latmPacketParserGetInfo;
}

/* -----------------------------------------------------------------------*/

FskErr latmPacketParserNew(RTPPacketParser parser)
{
	FskErr err;
	LATMPacketParser latmPacketParser;
	SDPMediaDescription mediaDescription;

	err = FskMemPtrNewClear(sizeof(LATMPacketParserRecord), &latmPacketParser);
	BAIL_IF_ERR(err);
	
	parser->handlerRefCon = latmPacketParser;
	parser->mediaFormat = kRTPAudioFormatAAC;

	latmPacketParser->bitsPerSample = 16;
	latmPacketParser->mediaFormat = kRTPAudioFormatAAC;

	mediaDescription = parser->mediaDescription;
	if (NULL != mediaDescription) {
		SDPAttribute attribute;

		// Grab what we need from the "fmtp" attribute
		attribute = SDPFindMediaAttribute(mediaDescription, "fmtp");
		if (NULL != attribute) {
			char *value, *attr;

			value = FskStrDoCopy(attribute->value);
			if (NULL != copyAttributeValue(value, "config", &attr)) {
				latmPacketParser->config = FskStrDoCopy(attr);
				FskMemPtrDispose(attr);
			}
			FskMemPtrDispose(value);
		}
	}

	// Build an ESDS compatible with Kinoma Player / FhG decoder
	if (!validateConfig(latmPacketParser->config, &latmPacketParser->nChannels, &latmPacketParser->sampleRate, &latmPacketParser->nSubFrames, latmPacketParser->esds, &latmPacketParser->esdsSize)) {
		err = kFskErrRTSPPacketParserUnsupportedFormat;
	}

bail:
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr latmPacketParserDispose(RTPPacketParser parser)
{
	LATMPacketParser latmPacketParser = (LATMPacketParser)parser->handlerRefCon;

	if (latmPacketParser) {
		RTPCompressedMediaFrame frames		= latmPacketParser->frames;
		RTPCompressedMediaFrame framesNext	= NULL;
		
		while (frames) {
			framesNext = frames->next;
			FskMemPtrDispose(frames);
			frames = framesNext;
		}
		FskMemPtrDispose(latmPacketParser->config);
		FskMemPtrDispose(latmPacketParser);
	}

	return 0;
}

/* -----------------------------------------------------------------------*/

FskErr latmPacketParserCanHandle(SDPMediaDescription mediaDescription, const char *encodingName)
{
	Boolean canHandle = false;

	// We handle MP4A-LATM encoding
	if (0 != FskStrCompareCaseInsensitive("MP4A-LATM", encodingName)) goto bail;

	if (NULL != mediaDescription) {
		SDPAttribute attribute;
		attribute = SDPFindMediaAttribute(mediaDescription, "fmtp");
		if (NULL != attribute) {
			char *attr;
	
			// The config string must be present in the SDP (i.e. out of band)
			if (NULL != copyAttributeValue(attribute->value, "cpresent", &attr)) {
				canHandle = (0 == FskStrToNum(attr));
				FskMemPtrDispose(attr);
			}
			// Validate the configuration
			if (canHandle) {
				if (NULL != copyAttributeValue(attribute->value, "config", &attr)) {
					canHandle = validateConfig(attr, NULL, NULL, NULL, NULL, NULL);
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

FskErr latmPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize)
{
	LATMPacketParser latmPacketParser = (LATMPacketParser)parser->handlerRefCon;
	FskErr err = 0;

	switch(selector) {
		case kRTPPacketParserSelectorESDS:
			*(UInt8**)info = latmPacketParser->esds;
			if (infoSize)
				*infoSize = latmPacketParser->esdsSize;
			break;

		case kRTPPacketParserSelectorRTPTimeScale:
		case kRTPPacketParserSelectorAudioSampleRate:
			*(UInt32*)info = latmPacketParser->sampleRate;
			break;
		case kRTPPacketParserSelectorAudioChannels:
			*(UInt32*)info = latmPacketParser->nChannels;
			break;
		case kRTPPacketParserSelectorAudioBitsPerSample:
			*(UInt32*)info = latmPacketParser->bitsPerSample;
			break;
		case kRTPPacketParserSelectorMediaFormat:
			*(UInt32*)info = latmPacketParser->mediaFormat;
			break;
		case kRTPPacketParserSelectorEncodingName:
			*(UInt8**)info = (UInt8*)FskStrDoCopy("MP4A-LATM");
			break;

		default:
			err = kFskErrInvalidParameter;
			break;
	}
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr latmPacketParserProcessPacket(RTPPacketParser parser, RTPPacket rtpHeader)
{
	LATMPacketParser		latmPacketParser = (LATMPacketParser)parser->handlerRefCon;
	UInt32					frameLength;
	RTPCompressedMediaFrame	frame = 0;
	FskErr					err = 0;

	if( rtpHeader->marker == 0 ) {
		// Not a complete frame, store our data in a link list
		RTPCompressedMediaFrame storedFrames = latmPacketParser->frames;
		
		frameLength = rtpHeader->dataSize;
		err			= FskMemPtrNewClear(frameLength + sizeof(RTPCompressedMediaFrameRecord), &frame);
		if (0 != err) goto bail;

		// Add the current buffer to our link list
		if(!storedFrames)
		{
			latmPacketParser->frames = frame;
		} 
		else 
		{
			while(storedFrames->next != NULL)
				storedFrames = storedFrames->next;

			storedFrames->next = frame;
		}

		frame->next		= NULL;
		frame->length	= frameLength;
		FskMemMove(frame + 1, rtpHeader->data, frameLength);		
		rtpHeader->frames = NULL;
	}
	else {
		RTPCompressedMediaFrame storedFrames	= latmPacketParser->frames;
		UInt32					storedSize		= 0;
		
		// if we have saved data, get it and prepend it to the current data
		while(storedFrames)
		{
			storedSize += storedFrames->length;
			storedFrames = storedFrames->next;
		}
		if(storedSize > 0)
		{
			frameLength = (rtpHeader->dataSize) + storedSize;
			err = FskMemPtrNewClear(frameLength + sizeof(RTPCompressedMediaFrameRecord), &frame);
			if (0 != err)
			{
			} 
			else 
			{
				unsigned char *pData = NULL;
				
				rtpHeader->frames	= frame;
				rtpHeader->totalSamples = 1024;
				frame->next			= NULL;
				
				// Walk our list again and really prepend the data
				storedFrames	= latmPacketParser->frames;
				frame += 1;  // Move pass the header
				pData = (unsigned char *)frame;
				
				while(storedFrames)
				{
					RTPCompressedMediaFrame currStoredFrames = storedFrames;
					UInt32					currStoredSize	 = storedFrames->length;
					
					FskMemMove(pData, storedFrames+1, currStoredSize);
					storedFrames = storedFrames->next;
					
					// Remove our stored data from our list
					FskMemPtrDispose(currStoredFrames);
					
					// increment our frame ptr
					pData += currStoredSize;
				}

				// Advance past the length bytes
				do {
					--frameLength;
				} while (*pData++ == 0xFF);
				
				err = FskMemPtrNew(sizeof(UInt32) * 2, (FskMemPtr*)&frame->sampleSizes);
				if (0 != err) goto bail;
				
				frame->length = frameLength;
				frame->sampleSizes[0] = frameLength;
				frame->sampleSizes[1] = 0;
				
				// Now copy our current frame
				FskMemMove(pData, rtpHeader->data, rtpHeader->dataSize);
				rtpHeader->dataSize = storedSize + rtpHeader->dataSize;
				
				// Set our link list to NULL
				latmPacketParser->frames = NULL;
			}
		} 
		else 
		{
			// Packet is self contained
			// Allocate a pointer to the current + saved compressed data
			UInt8 *start, *data = rtpHeader->data;
			UInt32 i, len;
			UInt32 nFrames = latmPacketParser->nSubFrames + 1;
			
			// Pre-flight validation - we've found some servers/hinters don't handle
			// multiple sub-frames correctly...
			frameLength = 0;
			for (i = 0; i <= latmPacketParser->nSubFrames; ++i) {
				len = 0;
				
				// Advance past the length bytes
				do {
					len += *data;
				} while (*data++ == 0xFF);

				data += len;
				frameLength += len;
			}
			if (frameLength > rtpHeader->dataSize) {
				err = kFskErrRTSPBadPacket;
				goto bail;
			}
			
			err = FskMemPtrNewClear(rtpHeader->dataSize + sizeof(RTPCompressedMediaFrameRecord), &frame);
			if (0 != err) goto bail;
				
			err = FskMemPtrNew(sizeof(UInt32) * (nFrames + 1), (FskMemPtr*)&frame->sampleSizes);
			if (0 != err) goto bail;
			
			// Now grab the encoded frames
			frameLength = 0;
			data = rtpHeader->data;
			start = (UInt8*)(frame + 1);
			for (i = 0; i <= latmPacketParser->nSubFrames; ++i) {
				len = 0;
				
				// Advance past the length bytes
				do {
					len += *data;
				} while (*data++ == 0xFF);
				
				// This frame starts at 'data' and contains 'len' bytes
				FskMemMove(start, data, len);
				
				frame->sampleSizes[i] = len;
				
				data += len;
				start += len;
				frameLength += len;
			}
			frame->sampleSizes[nFrames] = 0;
			frame->next = NULL;
			frame->length = frameLength;
			
			rtpHeader->frames = frame;
			rtpHeader->totalSamples = nFrames * 1024;
		}
	}

bail:
	return err;
}

/* -----------------------------------------------------------------------*/

FskErr latmPacketParserDisposePacket(RTPPacketParser parser, RTPPacket packet)
{
	return 0;
}

/* -----------------------------------------------------------------------*/

Boolean validateConfig(char *config, UInt32 *audioChannels, UInt32 *sampleRate, UInt32 *subFrames, UInt8 *esds, UInt32 *esdsSize)
{
	UInt8 *buffer = 0, *p, *q;
	bitStreamRecord bits;
	UInt32 length = FskStrLen(config);
	UInt32 configBitsRemaining;
	Boolean valid = false;
	UInt8 audioMuxVersion, numSubFrames, numProgram, numLayer;
	UInt8 audioObjectType, samplingFrequencyIndex, channelConfiguration;
	UInt8 dependsOnCoreCoder, extensionFlag, frameLengthType;
	UInt32 samplingFrequency;
	const UInt32 samplingFrequencyTable[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};

	BAIL_IF_ERR(FskMemPtrNewClear(length, &buffer));

	p = (UInt8*)config;
	q = buffer;
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
		
		*q++ = (a << 4) + b;
	}

	// Parse the StreamMuxConfig
	bits.data = buffer;
	bits.position = 0;
	audioMuxVersion = (UInt8)getBits(&bits, 1);
	if (0 != audioMuxVersion) goto bail;

	(void)getBits(&bits, 1);    // allStreamsSameTimeFraming
	numSubFrames = (UInt8)getBits(&bits, 6);
	numProgram = (UInt8)getBits(&bits, 4);
	numLayer = (UInt8)getBits(&bits, 3);
	
	// We only handle one layer/sub-frame/program
	// Note these bit fields are stored "minus one", i.e. one layer == (0 == numLayer)
	if (0 != numProgram || 0 != numLayer)
		goto bail;

	// Parse the AudioSpecificConfig
	audioObjectType = (UInt8)getBits(&bits, 5);
	if (2 != audioObjectType) goto bail;	// AAC LC
	samplingFrequencyIndex = (UInt8)getBits(&bits, 4);
	if ( samplingFrequencyIndex==0xf )
		samplingFrequency = getBits(&bits, 24);
	else
		if (samplingFrequencyIndex > 0xC) goto bail;
	else
		samplingFrequency = samplingFrequencyTable[samplingFrequencyIndex];

	channelConfiguration = (UInt8)getBits(&bits, 4);
	if (channelConfiguration == 0 || channelConfiguration > 2) goto bail;

	valid = true;

	// Parse the GASpecificConfig
	(void)getBits(&bits, 1);   // frameLengthFlag
	dependsOnCoreCoder = (UInt8)getBits(&bits, 1);
	if (dependsOnCoreCoder) {
		(void)getBits(&bits, 14);       // coreCoderDelay
	}
	extensionFlag = (UInt8)getBits(&bits, 1);
	if (extensionFlag) {
		getBits(&bits, 1);	// extensionFlag3
	}
	
	// Back to StreamMuxConfig()
	frameLengthType = (UInt8)getBits(&bits, 3);
	if (0 == frameLengthType) {
		(void)getBits(&bits, 8);    // latmBufferFullness
	}
	
	if (NULL != audioChannels)
		*audioChannels = channelConfiguration;
	if (NULL != sampleRate)
		*sampleRate = samplingFrequency;
	if (NULL != subFrames)
		*subFrames = numSubFrames;

	// Generate the esds from the AudioSpecificConfig
	if (NULL != esds) {
		UInt8 *esdsPtr = esds;

		*esdsPtr++ = 0x4;		// <DecoderConfigDesciptorTag>
		*esdsPtr++ = 0;			// pad
		*esdsPtr++ = 0x40;		// <objectTypeIndication>

		*esdsPtr++ = 0x14;		// <streamType>, <upstream>, <reserved> 
		esdsPtr += 3;			// buffersize DB
		esdsPtr += 4;			// max bitrate
		esdsPtr += 4;			// avg bitrate

		*esdsPtr++ = 0x05;		// <DecoderSpecificInfoTag>
		*esdsPtr++ = 0;			// pad

		// Rewind back to the AudioSpecificConfig
		bits.data = buffer;
		bits.position = 0;
		getBits(&bits, 15);
		
		configBitsRemaining = (length * 4) - 15;
		
		// Append the AudioSpecificConfig and GASpecificConfig to the esds
		while (configBitsRemaining) {
			UInt8 nBits = (UInt8)(configBitsRemaining > 8 ? 8 : configBitsRemaining);

			*esdsPtr++ = (UInt8)getBits(&bits, nBits);
			configBitsRemaining -= nBits;
		}
		*esdsSize = (UInt32)(esdsPtr - esds);
	}

bail:
	FskMemPtrDispose(buffer);
	return valid;
}

/* -----------------------------------------------------------------------*/
