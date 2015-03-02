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

#ifndef __ASFREADER__
#define __ASFREADER__

#if defined(__FSK_LAYER__) || defined (__FSK_JUMPER_APP__)
	#include "Fsk.h"
	#define ASF_READER_FILE64 1
#elif defined (__FSK_JUMPER_APP__)
	#include <windows.h>

	typedef UINT8 UInt8;
	typedef UINT16 UInt16;
	typedef UINT32 UInt32;
	typedef INT32 SInt32;
	typedef BOOL Boolean;
	#define true TRUE
	#define false FALSE

	#define ASF_READER_FILE64 1
	#define kFskDecompressorSlop (4)
#else
	#include "MacTypes.h"
#endif

#ifndef FskExport
	#define FskExport
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef SInt32 ASFErr;

#if !ASF_READER_FILE64
	typedef UInt32 ASFFileOffset;
#else
	#if defined(__FSK_LAYER__)
		typedef FskInt64 ASFFileOffset;
	#elif defined(WIN32)
		typedef __int64 ASFFileOffset;
	#else
		typedef long long ASFFileOffset;
	#endif
#endif

enum {
	kASFErrNone = 0,					// same as kFskErrNone
	kASFErrOutOfMemory = -1,			// same as kFskErrOutOfMemory
	kASFErrUnimplemented = -9,			// same as kFskErrUnimplemented
	kASFErrBadData = -13,				// same as kFskErrBadData
	kASFErrEndOfFile = -49,				// same as kFskErrEndOfFile
	kASFErrEndOfPackets = -4141,
	kASFErrDataUnavailableAfterHeader = -4142
};

typedef ASFErr (*ASFReadProc)(void *refCon, void *data, ASFFileOffset offset, UInt32 dataSize);
typedef ASFErr (*ASFAllocProc)(void *refCon, Boolean clear, UInt32 size, void **data);
typedef void (*ASFFreeProc)(void *refCon, void *data);

typedef ASFErr (*ASFDemuxerGetNextPacketProc)(void *readerRefCon, void *data);

struct ASFDemuxer;
typedef struct ASFDemuxerRecord ASFDemuxerRecord;
typedef struct ASFDemuxerRecord *ASFDemuxer;

struct ASFStream;
typedef struct ASFStreamRecord ASFStreamRecord;
typedef struct ASFStreamRecord *ASFStream;

struct ASFPayload;
typedef struct ASFPayloadRecord ASFPayloadRecord;
typedef struct ASFPayloadRecord *ASFPayload;

struct ASFContentDescriptor;
typedef struct ASFContentDescriptorRecord ASFContentDescriptorRecord;
typedef struct ASFContentDescriptorRecord *ASFContentDescriptor;

struct ASFAdvancedMutualExclusion;
typedef struct ASFAdvancedMutualExclusionRecord ASFAdvancedMutualExclusionRecord;
typedef struct ASFAdvancedMutualExclusionRecord *ASFAdvancedMutualExclusion;

struct ASFExtendedStreamProperties;
typedef struct ASFExtendedStreamPropertiesRecord ASFExtendedStreamPropertiesRecord;
typedef struct ASFExtendedStreamPropertiesRecord *ASFExtendedStreamProperties;

struct ASFDemuxerRecord {
	ASFAllocProc			allocProc;
	ASFFreeProc				freeProc;
	void					*allocRefCon;

	ASFReadProc				reader;
	void					*readerRefCon;

	Boolean					loaded;
	Boolean					seekable;

	ASFFileOffset			fileSize;
	ASFFileOffset			dataPacketsCount;
	ASFFileOffset			playDuration;
	ASFFileOffset			sendDuration;
	ASFFileOffset			preroll;
	UInt32					flags;
	UInt32					minPacketSize;
	UInt32					maxPacketSize;
	UInt32					maxBitRate;

	ASFStream				streams;

	// data object
	ASFFileOffset			dataObject_fileOffset;

	// content description object
	UInt16					*contentDesc_title;
	UInt16					*contentDesc_author;
	UInt16					*contentDesc_copyright;
	UInt16					*contentDesc_description;
	UInt16					*contentDesc_rating;

	// simple index object
	ASFFileOffset			simpleIndex_fileOffset;		// if non-zero, simple index is present

	ASFFileOffset			simpleIndex_interval;
	UInt32					simpleIndex_count;

	// index object
	ASFFileOffset			index_fileOffset;			// if non-zero, index is present

	// extended content description object
	ASFContentDescriptor	extendedContentDescriptors;
	UInt16					extendedContentDescriptorsCount;
	unsigned char			*extendedContentDescriptionObjectData;

	// metadata object from header extension object
	ASFContentDescriptor	metadata;
	UInt16					metadataCount;
	unsigned char			*metadataObjectData;

	// metadata library object from header extension object
	ASFContentDescriptor	metadataLibrary;
	UInt16					metadataLibraryCount;
	unsigned char			*metadataLibraryObjectData;

	// current packet info
	unsigned char			*packet;
	UInt32					packetNumber;
	ASFPayload				payloads;
	UInt32					payloadCount;
	UInt32					nextPayloadIndex;			// index of next payload to parse in this packet

	// mutual exclusion groups
	ASFAdvancedMutualExclusion	advancedMutualExclusions;

	// streaming support
	ASFDemuxerGetNextPacketProc	getNextPacket;

	// temporary hold for extended properties while parsing
	ASFExtendedStreamProperties	extendedProperties;
};

typedef struct {
	UInt16				formatTag;
	UInt16				numChannels;
	UInt32				samplesPerSecond;
	UInt32				averageBytesPerSecond;
	UInt16				blockAlign;
	UInt16				bitsPerSample;
	UInt16				codecSpecificDataSize;
	UInt8				*codecSpecificData;
} ASFStreamAudioRecord, *ASFStreamAudio;

typedef struct {
	UInt32				encodedImageWidth;
	UInt32				encodedImageHeight;

	UInt32				width;
	UInt32				height;
	UInt16				reserved;
	UInt16				bitsPerPixel;
	UInt32				compressionID;
	UInt32				imageSize;
	UInt32				hPixelsPerMeter;
	UInt32				vPixelsPerMeter;
	UInt32				colorsUsed;
	UInt32				importantColors;
	UInt32				codecSpecificDataSize;
	UInt8				*codecSpecificData;
} ASFStreamVideoRecord, *ASFStreamVideo;

typedef struct {
	UInt32				width;
	UInt32				height;
	UInt32				reserved;
} ASFStreamImageJFIFRecord, *ASFStreamImageJFIF;

enum {
	kASFMediaTypeUnknown = 0,
	kASFMediaTypeAudio,
	kASFMediaTypeVideo,
	kASFMediaTypeImageJFIF
};

struct ASFStreamRecord {
	ASFStream					next;
	ASFDemuxer					demuxer;

	unsigned char				guid[16];

	ASFFileOffset				timeOffset;
	UInt16						flags;
	UInt8						number;
	Boolean						encrypted;

	ASFExtendedStreamProperties	extendedProperties;

	UInt32						typeSpecificDataSize;
	UInt8						*typeSpecificData;

	UInt32						bytesInFrame;
	unsigned char				*frame;
	UInt32						framePresentationTime;
	UInt32						frameSize;

	UInt32						mediaType;
	union {
		ASFStreamAudioRecord		audio;
		ASFStreamVideoRecord		video;
		ASFStreamImageJFIFRecord	imageJFIF;
	} media;

	// runtime
	Boolean						disabled;
};

struct ASFPayloadRecord {
	UInt8					streamNumber;
	Boolean					keyFrame;
	Boolean					havePresentationTime;
	Boolean					isCompressed;

	ASFStream				stream;

	UInt32					mediaObjectNumber;
	UInt32					offsetIntoMediaObject;
	UInt32					mediaObjectSize;
	UInt32					presentationTime;

	unsigned char			*data;
	UInt32					dataSize;
};

struct ASFContentDescriptorRecord {
	UInt16					streamNumber;

	UInt16					dataType;

	UInt16					*name;
	unsigned char			*value;
	UInt32					valueLength;
};

enum {
	kASFDemuxerExclusionUnknown = 0,
	kASFDemuxerExclusionBitRate,
	kASFDemuxerExclusionLanguage
};

struct ASFAdvancedMutualExclusionRecord {
	ASFAdvancedMutualExclusion			next;

	UInt32								exclusionType;
	unsigned char						guid[16];
	UInt16								count;
	UInt16								streams[1];
};

struct ASFExtendedStreamPropertiesRecord {
	ASFExtendedStreamProperties			next;

	UInt16								streamNumber;
	UInt32								bitRate;

	ASFFileOffset						averageTimePerFrame;
};

FskExport(ASFErr) ASFDemuxerNew(ASFDemuxer *demuxer, ASFFileOffset maxOffset, ASFReadProc reader, void *readerRefCon, ASFAllocProc alloc, ASFFreeProc free, void *allocRefCon);
FskExport(void) ASFDemuxerDispose(ASFDemuxer demuxer);

FskExport(ASFErr) ASFDemuxerSeek(ASFDemuxer demuxer, ASFFileOffset targetTime, ASFFileOffset *actualTime);

FskExport(ASFErr) ASFDemuxerNextFrame(ASFDemuxer demuxer, void **data, UInt32 *dataSize, ASFStream *stream, ASFFileOffset *presentationTime, Boolean *keyFrame);

#define ASFDemuxerGetMetadata(demuxer, stream, name) (ASFDemuxerFindDescriptor(demuxer, name, stream, demuxer->metadata, demuxer->metadataCount))
#define ASFDemuxerGetMetadataLibrary(demuxer, stream, name) (ASFDemuxerFindDescriptor(demuxer, name, stream, demuxer->metadataLibrary, demuxer->metadataLibraryCount))
#define ASFDemuxerGetExtendedContentDescriptor(demuxer, name) (ASFDemuxerFindDescriptor(demuxer, name, 0, demuxer->extendedContentDescriptors, demuxer->extendedContentDescriptorsCount))

FskExport(ASFContentDescriptor) ASFDemuxerFindDescriptor(ASFDemuxer demuxer, UInt16 *name, UInt16 stream, ASFContentDescriptor descriptors, UInt32 descriptorCount);

FskExport(ASFErr) ASFDemuxerSetGetNextPacketProc(ASFDemuxer demuxer, ASFDemuxerGetNextPacketProc proc);

FskExport(void) ASFDemuxerStreamDispose(ASFStream stream);
FskExport(ASFStream) ASFDemuxerStreamGet(ASFDemuxer demuxer, UInt16 streamNumber);

FskExport(ASFErr) ASFDemuxerRescanForIndices(ASFDemuxer demuxer);
FskExport(ASFErr) ASFDemuxerScanPacket(ASFDemuxer demuxer, unsigned char *packet, UInt32 *sendTime, UInt32 *duration);

// utilities

Boolean ASFEqualGUIDS(const unsigned char *a, const unsigned char *b);

#ifdef __cplusplus
}
#endif

#endif
