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
#ifndef __FSKDIDLGENMEDIA__
#define __FSKDIDLGENMEDIA__

#include "Fsk.h"
#include "FskFiles.h"
#include "FskMedia.h"

#if SUPPORT_OMA

// magic atrac3 parameters.
typedef struct {
	UInt16				quality;
	UInt16				joint;
} atrac3MagicRecord;

// magic atrac3plus parameters.
typedef struct {
	UInt16				samplingFreq;
	UInt16				channelConfigIndex;
	UInt16				frameSize;
} atrac3plusMagicRecord;

typedef struct {
	UInt16 fileFlag;
	UInt16 flags;
	UInt16 version;
	UInt16 layer;
	UInt16 channelMode;
	UInt16 emphasis;
	UInt32 totalTime;
	UInt32 frameCount;

	UInt32 frequency;
	UInt32 bitrate;
} FskOmaMp3FormatInfoRecord, *FskOmaMp3FormatInfo;

typedef struct {
	UInt16 bitsPerSample;
	UInt16 bitrateMode;
	UInt32 totalTime;
	UInt32 frameCount;

	UInt32 frequency;
	UInt32 channelCount;
	UInt32 bitrate;
} FskOmaWmaFormatInfoRecord,*FskOmaWmaFormatInfo;

typedef struct {
	UInt16 bitsPerSample;

	UInt32 frequency;
	UInt32 channelCount;
} FskOmaPcmFormatInfoRecord, *FskOmaPcmFormatInfo;

typedef struct {
	UInt16		frameLength;
	UInt16		bitrate;
	UInt16		mode;
	UInt16		emphasis;
	UInt16		adjustment;
	UInt16		blockLength;
} FskAtracFormatInfoRecord, *FskAtracFormatInfo;

#endif

// MP3 support

typedef struct {
	FskInt64		fileSize;

	FskInt64		dataOffset;
	FskInt64		dataSize;

	UInt32			doExtendedParsing;
	UInt32			bitrate;				// actually kbps
	UInt32			channelCount;
	UInt32			profile;
	UInt32			level;
	UInt32			frequency;
	UInt32			frameLength;
	double			duration;				// in seconds
	unsigned char	*xingToc;

	FskMediaMetaData	meta;

	UInt32			id3TagSize;
	UInt32			samplesPerFrame;

	UInt32						codec;

#if SUPPORT_OMA
	UInt32						omaHeaderSize;

	FskAtracFormatInfoRecord	atracParams;

	union {
		atrac3MagicRecord		at3Params;
		atrac3plusMagicRecord	at3plusParams;
	};
#endif
} DIDLMusicItemRecord, *DIDLMusicItem;

typedef FskErr (*scanMP3ReadProc)(void *refCon, UInt32 bytesToRead, void *data, UInt32 *bytesRead, FskInt64 offset, Boolean dontSeekIfExpensive);

FskAPI(Boolean) scanMP3FromCallback(DIDLMusicItem mi, scanMP3ReadProc readProc, void *readProcRefCon, FskInt64 totalSize, Boolean getMetaData);
FskAPI(FskErr) scanMP3(DIDLMusicItem mi, const char *fullPath, Boolean getMetaData, Boolean *isMP3);
FskAPI(void) scanMP3Dispose(DIDLMusicItem mi);

FskAPI(Boolean) scanMP3ID3v2(unsigned char *buffer, const unsigned char *bufferEnd, DIDLMusicItem mi);

FskAPI(Boolean) parseMP3Header(unsigned char *scan, DIDLMusicItem mi);
FskAPI(Boolean) parseADTSHeader(unsigned char *stream, DIDLMusicItem mi);

#define kOmaHeaderSize (0x60)
FskAPI(Boolean) parseOMAHeader(unsigned char *scan, DIDLMusicItem mi);

#endif
