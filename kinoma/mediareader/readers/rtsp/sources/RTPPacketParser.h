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


#ifndef _RTP_PACKET_PARSER_H
#define _RTP_PACKET_PARSER_H

#include "Fsk.h"
#include "SDP.h"

// Media formats
#define kRTPMediaFormatUnknown 0x6666

enum {
	kRTPAudioFormatPCM16BitBigEndian = 1,
	kRTPAudioFormatPCM16BitLittleEndian,
	kRTPAudioFormatPCM8BitTwosComplement,
	kRTPAudioFormatPCM8BitOffsetBinary,

	kRTPAudioFormatAAC = 0x1010,
	kRTPAudioFormatAMR,
	kRTPAudioFormatQCELP
};

enum {
	kRTPVideoFormatH263 = 1,
	kRTPVideoFormatMPEG4,
	kRTPVideoFormatAVC
};

// Get info selectors
enum {	
	kRTPPacketParserSelectorESDS = 1,					// UInt8**
	kRTPPacketParserSelectorMediaFormat,				// UInt32
	kRTPPacketParserSelectorRTPTimeScale,				// UInt32
	kRTPPacketParserSelectorHandleDroppedPackets,		// UInt8**

	kRTPPacketParserSelectorVideoWidth = 0x1000,		// UInt32
	kRTPPacketParserSelectorVideoHeight,				// UInt32
	kRTPPacketParserSelectorVideoTimeScale,				// UInt32

	kRTPPacketParserSelectorAudioSampleRate = 0x2000,	// UInt32
	kRTPPacketParserSelectorAudioChannels,				// UInt32
	kRTPPacketParserSelectorAudioBitsPerSample,			// UInt32
	kRTPPacketParserSelectorAudioTimeScale,				// UInt32
	
	kRTPPacketParserSelectorEncodingName				// UInt8**
};


// A compressed media frame - consists of header followed by the compressed media data
struct RTPCompressedMediaFrameRecord {
	struct RTPCompressedMediaFrameRecord *next;
	UInt32 *sampleSizes;
	
	UInt32 decodeTimeStamp;
	UInt32 compositionTimeStamp;

	UInt32 key;		// non-zero if this is a key frame
	UInt32 droppable;
	UInt32 length;	// length of compressed data only
	// The compressed data follows
};
typedef struct RTPCompressedMediaFrameRecord RTPCompressedMediaFrameRecord;
typedef RTPCompressedMediaFrameRecord *RTPCompressedMediaFrame;

struct RTPPacketRecord {
	struct RTPPacketRecord *next;	// used by player
	UInt32 presentationTime;		// used by player
	
	UInt32 packetType;
	UInt8 version;
	UInt8 padding;
	UInt8 extension;
	UInt8 CSRCcount;
	UInt8 marker;
	UInt8 payloadType;
	UInt32 sequenceNumber;
	UInt32 timestamp;
	UInt32 SSRC;
	UInt32 CSRC[15];  // max
	UInt8 *data;
	UInt32 dataSize;
	
	SInt32 totalSamples;	// set to -1 if unknown
	RTPCompressedMediaFrame frames;

	UInt32 packetNumber;  // debug
};
typedef struct RTPPacketRecord RTPPacketRecord;
typedef RTPPacketRecord *RTPPacket;

typedef FskErr (*RTPPacketParserReceivePacketCallback)(RTPPacket packet, void *refCon);

struct RTPPacketParserRecord {
	struct RTPPacketHandlerRecord		*handler;
	void								*handlerRefCon;

	SDPMediaDescription mediaDescription;	// points at mediaStream->mediaDescription, don't dispose
	UInt32 nextPacketNumber;
	SInt32 useCount;

	UInt32 mediaFormat;
	
	void *sessionRefCon;
	RTPPacketParserReceivePacketCallback sessionReceivePacketCB;
	void *appRefCon;
	RTPPacketParserReceivePacketCallback appReceivePacketCB;
	
	void		*packetBuffer;
};
typedef struct RTPPacketParserRecord RTPPacketParserRecord;
typedef RTPPacketParserRecord *RTPPacketParser;

typedef FskErr (*RTPPacketHandlerCanHandle)(SDPMediaDescription mediaDescription, const char *encodingName);
typedef FskErr (*RTPPacketHandlerNew)(RTPPacketParser parser);
typedef FskErr (*RTPPacketHandlerDispose)(RTPPacketParser parser);
typedef FskErr (*RTPPacketHandlerProcessPacket)(RTPPacketParser parser, RTPPacket packet);
typedef FskErr (*RTPPacketHandlerDisposePacket)(RTPPacketParser parser, RTPPacket packet);
typedef FskErr (*RTPPacketHandlerFlush)(RTPPacketParser parser);
typedef FskErr (*RTPPacketHandlerGetInfo)(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize);
typedef FskErr (*RTPPacketHandlerSetInfo)(RTPPacketParser parser, UInt32 selector, void *info, UInt32 infoSize);

struct RTPPacketHandlerRecord {
	UInt32									version;

	RTPPacketHandlerCanHandle				doCanHandle;

	RTPPacketHandlerNew						doNew;
	RTPPacketHandlerDispose					doDispose;
	RTPPacketHandlerFlush					doFlush;

	RTPPacketHandlerProcessPacket			doProcessPacket;
	RTPPacketHandlerDisposePacket			doDisposePacket;

	RTPPacketHandlerGetInfo					doGetInfo;
	RTPPacketHandlerSetInfo					doSetInfo;
};
typedef struct RTPPacketHandlerRecord RTPPacketHandlerRecord;
typedef RTPPacketHandlerRecord *RTPPacketHandler;

#ifdef __cplusplus
extern "C" {
#endif

FskErr RTPPacketParserNew(RTPPacketParser *parser, SDPMediaDescription mediaDescription, char *codecName);
FskErr RTPPacketParserDispose(RTPPacketParser parser);

FskErr RTPPacketParserSetSessionRefCon(RTPPacketParser parser, void *refCon);
FskErr RTPPacketParserSetSessionReceivePacketCallback(RTPPacketParser parser, RTPPacketParserReceivePacketCallback callback);
FskErr RTPPacketParserSetAppRefCon(RTPPacketParser parser, void *refCon);
FskErr RTPPacketParserSetAppReceivePacketCallback(RTPPacketParser parser, RTPPacketParserReceivePacketCallback callback);

FskErr RTPPacketParserProcessBuffer(RTPPacketParser parser, unsigned char *buffer, UInt32 bufferSize);

FskErr RTPPacketParserGetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 *infoSize);
FskErr RTPPacketParserSetInfo(RTPPacketParser parser, UInt32 selector, void *info, UInt32 infoSize);

FskErr RTPPacketParserFlush(RTPPacketParser parser);

// If 'disposeMediaData' is true, then dispose 'compressedData'
FskErr RTPPacketDispose(RTPPacketParser parser, RTPPacket packet, Boolean disposeMediaData);

// Packet parser frame linking utilities
int PushDataToLinkedFrames( int compositionTime, int decodeTime, int size, unsigned char *dataPtr, RTPCompressedMediaFrame *storedFrames );
int GetLinkededFramesSize( RTPCompressedMediaFrame storedFrames, UInt32	*storedSizeOut );
int MoveLinkedFramesDataToFrame( RTPCompressedMediaFrame storedFrames, RTPCompressedMediaFrame frame );
int ClearLinkedFrames( RTPCompressedMediaFrame storedFrames );

#ifdef __cplusplus
}
#endif

#endif
