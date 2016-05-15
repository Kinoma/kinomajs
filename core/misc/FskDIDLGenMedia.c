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
#include "FskDIDLGenMedia.h"
#include "FskFiles.h"
#include "FskEndian.h"
#include "FskImage.h"
#include "FskTextConvert.h"
#include "FskAudio.h"
#include "QTReader.h"

/*
	Music
*/

typedef Boolean (*HeaderParserProc)(unsigned char *scan, DIDLMusicItem mi);
static Boolean parseXingTOC(unsigned char *scan, DIDLMusicItem mi);
static Boolean parseFhGTOC(unsigned char *scan, DIDLMusicItem mi);
static Boolean scanMP3ID3v1(const unsigned char *buffer, DIDLMusicItem mi);
static UInt32 id3v1StrLen(const unsigned char *c);
static Boolean isValidTag(unsigned char *t);
static char *id3v2TagToString(const unsigned char *buffer, UInt32 size);
static char *id3v3TagToString(const unsigned char *buffer, UInt32 size);
#if SUPPORT_OMA
	static char *omgTextToUTF8(char encoding, const unsigned char **p, SInt32 bytesAvailable);
	static FskErr addAsOMATag(UInt32 tag, const unsigned char *description, const unsigned char *tagData, UInt32 tagDataSize, DIDLMusicItem mi);
#endif

typedef char *(*id3TagAddProc)(const unsigned char *tinyhttp, UInt32);

/*
	MP3 files tagged as Latin1 encoding are often not really Latin-1.
	Instead, the text encoding is the user's local default text encoding.
	So, when we encounter Latin1, we cross our fingers and use the local encoding.
	This is consistent with iTunes, Windows Media Player, and Sonic Stage, and provides
	better results for many MP3 files with Japanese & Chinese metadata.
*/
#if TARGET_OS_WIN32
	#define mp3TextLatin1ToUTF8 FskTextToUTF8
#else
	#define mp3TextLatin1ToUTF8 FskTextLatin1ToUTF8
#endif

static FskErr scanMP3ReadFileProc(void *refCon, UInt32 bytesToRead, void *data, UInt32 *bytesRead, FskInt64 offset, Boolean dontSeekIfExpensive);

FskErr scanMP3ReadFileProc(void *refCon, UInt32 bytesToRead, void *data, UInt32 *bytesRead, FskInt64 offset, Boolean dontSeekIfExpensive)
{
	FskErr err = FskFileSetPosition((FskFile)refCon, &offset);
	if (err) return err;

	return FskFileRead((FskFile)refCon, bytesToRead, data, bytesRead);
}

FskErr scanMP3(DIDLMusicItem mi, const char *fullPath, Boolean getMetaData, Boolean *isMP3)
{
	FskErr err;
	FskFile fref = NULL;
	FskInt64 fileSize;

	*isMP3 = false;

	err = FskFileOpen(fullPath, kFskFilePermissionReadOnly, &fref);
	BAIL_IF_ERR(err);

	FskFileGetSize(fref, &fileSize);
	*isMP3 = scanMP3FromCallback(mi, scanMP3ReadFileProc, fref, fileSize, getMetaData);
		
bail:
	FskFileClose(fref);
	return err;
}

Boolean scanMP3FromCallback(DIDLMusicItem mi, scanMP3ReadProc readProc, void *readProcRefCon, FskInt64 totalSize, Boolean getMetaData)
{
	#define kMP3BufferSize (32768)
	Boolean valid = false, isOMA = false;
	unsigned char *buffer, *id3Buffer = NULL, *firstFrame = NULL;
	unsigned char *xingToc = NULL;
	double xingDuration = 0, fhgDuration = 0;
	Boolean haveFHG = false;
	UInt32 bytesToScan;
	HeaderParserProc headerProc = NULL;
	Boolean haveID3v2 = false;

	mi->fileSize = totalSize;

	mi->dataOffset = 0;
	mi->dataSize = totalSize;
	mi->meta = NULL;

	mi->id3TagSize = 0;
	mi->duration = 0;
	if (kFskErrNone != FskMemPtrNew(kMP3BufferSize, &buffer))
		return false;

	if (kFskErrNone == (readProc)(readProcRefCon, 10, buffer, &bytesToScan, 0, false)) {
		FskInt64 position = 0;
		unsigned char *scan;
		Boolean firstTime = true;

        BAIL_IF_ERR(FskMediaMetaDataNew(&mi->meta));

#if SUPPORT_OMA
		isOMA = (0 == FskStrCompareWithLength((const char*)buffer, (const char*)"ea3", 3));
#endif
		if (isOMA || (0 == FskStrCompareWithLength((const char*)buffer, (const char*)"ID3", 3))) {
			FskInt64 readPosition = 0;
			position = mi->id3TagSize = (((buffer[6] & 0x7f) << 21) | ((buffer[7] & 0x7f) << 14) | ((buffer[8] & 0x7f) << 7) | (buffer[9] & 0x7f)) + 10;

			if (getMetaData && (0 != mi->id3TagSize)) {
				// we're allowed to look for ID3 data now. experience the joy.
				UInt32 thisTagSize = mi->id3TagSize;
again:
				BAIL_IF_ERR(FskMemPtrNew(thisTagSize, &id3Buffer));

				BAIL_IF_ERR((readProc)(readProcRefCon, thisTagSize, id3Buffer, NULL, readPosition, false));

				haveID3v2 = scanMP3ID3v2(id3Buffer, id3Buffer + thisTagSize, mi);

				FskMemPtrDisposeAt(&id3Buffer);

				if (haveID3v2 && (kFskErrNone == (readProc)(readProcRefCon, 10, buffer, &bytesToScan, position, false)) && (0 == FskStrCompareWithLength((const char*)buffer, (const char*)"ID3", 3))) {
					thisTagSize = (((buffer[6] & 0x7f) << 21) | ((buffer[7] & 0x7f) << 14) | ((buffer[8] & 0x7f) << 7) | (buffer[9] & 0x7f)) + 10;
					if (thisTagSize > 0) {
						readPosition = position;
						position += thisTagSize;
						mi->id3TagSize += thisTagSize;
						goto again;
					}
				}
			}
		}

		if ((false == haveID3v2) && (mi->fileSize > 1024)) {
			unsigned char scratch[128];
			if (kFskErrNone == (readProc)(readProcRefCon, 128, scratch, NULL, mi->fileSize - 128, true)) {
				if (scanMP3ID3v1(scratch, mi))
					mi->dataSize -= 128;
			}
		}

		while (!valid) {		// MP3
			while (firstTime) {
				BAIL_IF_ERR((readProc)(readProcRefCon, kMP3BufferSize, buffer, &bytesToScan, position, false));

				if (0 != FskStrCompareWithLength((const char*)buffer, (const char*)"ID3", 3))
					break;

				// sometimes there are multiple id3 headers (different versions)
				position += (((buffer[6] & 0x7f) << 21) | ((buffer[7] & 0x7f) << 14) | ((buffer[8] & 0x7f) << 7) | (buffer[9] & 0x7f)) + 10;
			}
			scan = buffer;

			// look for sync marker
			while (scan < (buffer + bytesToScan - 512)) {
				if ((0xff == scan[0]) && (0xe0 == (scan[1] & 0xe0))) {		// works for both aac & mp3
					int i;

					// look into the header
					if (parseMP3Header(scan, mi)) {
						if (parseXingTOC(scan, mi)) {
							FskMemPtrDispose(xingToc);
							xingToc = mi->xingToc;
							mi->xingToc = NULL;
							xingDuration = mi->duration;
						}
						else if (parseFhGTOC(scan, mi)) {
							haveFHG = true;
							fhgDuration = mi->duration;
						}
						headerProc = parseMP3Header;
					}
					else
					if (parseADTSHeader(scan, mi))
						headerProc = parseADTSHeader;
					else
						headerProc = NULL;

					if (NULL != headerProc) {
						// check the next few frames to see if we're still good
						unsigned char *checker = scan;
						firstFrame = scan;
						for (i=1; i<5; i++) {
							checker += mi->frameLength;
							if ((checker > (buffer + bytesToScan - 128)) || (false == (headerProc)(checker, mi))) {
								firstFrame = NULL;
								goto notYet;
							}
						}
						valid = true;

						mi->dataOffset = position + (scan - buffer);
						if (0 != mi->dataSize)
							mi->dataSize -= mi->dataOffset;
						break;
					}
				}
notYet:
				scan += 1;
			}

			if (firstTime) {
				if (!valid) {
					position += bytesToScan;
					firstTime = false;
				}
			}
			else
				break;			// either way, we're done
		}
	}

	if (valid) {
		if (xingToc) {
			mi->xingToc = xingToc;
			xingToc = NULL;
			if (xingDuration)
				mi->duration = xingDuration;
		}
		else
		if (haveFHG) {
			if (fhgDuration)
				mi->duration = fhgDuration;
		}

		if (NULL != firstFrame) {
			// find average bitrate from a bunch of frames
			UInt32 frameCount = 0;
			UInt32 bitRate = 0;

			while (firstFrame < (buffer + bytesToScan - 512)) {
				DIDLMusicItemRecord mi2;
				
				mi2.doExtendedParsing = 0;
				if (false == (headerProc)(firstFrame, &mi2))
					break;
				frameCount += 1;
				bitRate += mi2.bitrate;
				firstFrame += mi2.frameLength;
			}
			if (frameCount > 1)
				mi->bitrate = bitRate / frameCount;
		}

		if (0 == mi->duration) {
			if ((0 != mi->dataSize) && (0 != mi->bitrate))
				mi->duration = ((double)mi->dataSize / (((double)mi->bitrate / 8.0) * 1000.0));
			else
				mi->duration = -1;
		}
	}

bail:
	if (!valid) {
		FskMediaMetaDataDispose(mi->meta);
		mi->meta = NULL;
		FskMemPtrDisposeAt(&mi->xingToc);
	}

	FskMemPtrDispose(buffer);
	FskMemPtrDispose(id3Buffer);
	FskMemPtrDispose(xingToc);

	return valid;
}

void scanMP3Dispose(DIDLMusicItem mi)
{
	FskMediaMetaDataDispose(mi->meta);
	FskMemPtrDispose(mi->xingToc);
}

Boolean parseMP3Header(unsigned char *scan, DIDLMusicItem mi)
{
	Boolean isOK = false;
	UInt32 frameLenCoefficient = 0, frameLenPadding = 0;
	int version, layer, bitrate, frequency, padding, channelmode;

	if (0xff != scan[0]) goto done;
	if (0xe0 != (scan[1] & 0xe0)) goto done;

	// parse the mp3 format
	version = (scan[1] & 0x18) >> 3;
	layer = (scan[1] & 0x06) >> 1;
//	protection = scan[1] & 1;
	bitrate = (scan[2] & 0xf0) >> 4;
	frequency = (scan[2] & 0x0c) >> 2;
	padding = (scan[2] & 0x02) >> 1;
//	privateF = scan[2] & 0x01;
	channelmode = (scan[3] & 0xc0) >> 6;
//	modeextension = (scan[3] & 0x30) >> 4;
//	copyright = (scan[3] & 0x08) >> 3;
//	original = (scan[3] & 0x04) >> 2;
//	emphasis = scan[3] & 0x03;

	// validate
	if (1 == version) goto done;
	if (0 == layer) goto done;
	if ((0 == bitrate) || (15 == bitrate)) goto done;
	if (3 == frequency) goto done;

	// sort out the bit rate and codec
	mi->codec = kFskAudioFormatMP3;
	switch (version) {
		case 0:		// 2.5
		case 2:		// 2
			switch (layer) {
				case 3: { // layer I
					static UInt16 bitrates[] = {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256};
					bitrate = bitrates[bitrate];
					mi->codec = kFskAudioFormatMP1;
					}
					break;

				case 2: // layer II
				case 1: { // layer IIII
					static UInt16 bitrates[] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160};
					bitrate = bitrates[bitrate];
					if (2 == layer)
						mi->codec = kFskAudioFormatMP2;
					}
					break;
			}
			break;

		case 3:		// 1
			switch (layer) {
				case 3: { // layer I
					static UInt16 bitrates[] = {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448};
					bitrate = bitrates[bitrate];
					mi->codec = kFskAudioFormatMP1;
					}
					break;

				case 2: { // layer II
					static UInt16 bitrates[] = {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384};
					bitrate = bitrates[bitrate];
					mi->codec = kFskAudioFormatMP2;
					}
					break;

				case 1: { // layer III
					static UInt16 bitrates[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};
					bitrate = bitrates[bitrate];
					}
					break;
			}
			break;
	}

	// sort out the frequency
	switch (version) {
		case 0:		// 2.5
			if (0 == frequency) frequency = 11025;
			else
			if (1 == frequency) frequency = 12000;
			else
			if (2 == frequency) frequency =  8000;
			break;

		case 2:		// 2
			if (0 == frequency) frequency = 22050;
			else
			if (1 == frequency) frequency = 24000;
			else
			if (2 == frequency) frequency = 16000;
			break;

		case 3:		// 1
			if (0 == frequency) frequency = 44100;
			else
			if (1 == frequency) frequency = 48000;
			else
			if (2 == frequency) frequency = 32000;
			break;
	}


	// figure out the channel count
	if (3 == channelmode)
		mi->channelCount = 1;
	else
		mi->channelCount = 2;

	// work on the frame size
	switch (version) {
		case 3:	// 1
			if (3 == layer)	{ // layer 1
				frameLenCoefficient = 48;
				frameLenPadding = padding * 4;
			}
			else {
				frameLenCoefficient = 144;
				frameLenPadding = padding;
			}
			break;

		case 2:	// 2
		case 0: // 2.5
			if (3 == layer)	{ // layer 1
				frameLenCoefficient = 24;
				frameLenPadding = padding * 4;
			}
			else {
				frameLenCoefficient = 72;
				frameLenPadding = padding;
			}
			break;
	}

	// work on the samples per frame
	mi->samplesPerFrame = 1152;
	switch (version) {
		case 3:	// 1
			if (3 == layer)
				mi->samplesPerFrame = 384;	 // layer 1
			break;

		case 2:	// 2
		case 0: // 2.5
			if (3 == layer)
				mi->samplesPerFrame = 384;	// layer 1
			else if (1 == layer)
				mi->samplesPerFrame = 576;	// layer 3
			break;
	}

	mi->frequency = frequency;
	mi->bitrate = bitrate;

	mi->frameLength = ((frameLenCoefficient * 1000 * bitrate) / frequency) + frameLenPadding;
	mi->duration = 0;
	mi->xingToc = NULL;

	isOK = true;

done:
	return isOK;
}

Boolean parseADTSHeader(unsigned char *stream, DIDLMusicItem mi)
{
	int channelConfig, blockCount;
	static const UInt32 sampleRates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0};
	static const UInt8 channels[] = {0, 1, 2, 3, 4, 5, 6, 8};
	UInt8 layer, samplerate_index;

	if ((0xff != stream[0]) || (0xf0 != (stream[1] & 0xf0)))		// 12 bits of sync word
		return false;
	
	// ignore 1 bit of id
	layer = (stream[1] >> 1) & 0x03;		// 2 bits of layer
	if (0 != layer)
		return false;

	mi->profile = 1 + ((stream[2] >> 6) & 0x03);					// 2 bits of profile
	samplerate_index = (stream[2] >> 2) & 0x0f;	// 4 bits of sample rate index
	mi->frequency = sampleRates[samplerate_index];
	if (0 == mi->frequency)
		return false;
	// ignore 1 bit of private
	channelConfig = ((stream[2] & 1) << 2) | ((stream[3] >> 6) & 0x03);
	channelConfig = channels[channelConfig];
	if (1 == channelConfig)
		mi->channelCount = 1;
	else if (2 == channelConfig)
		mi->channelCount = 2;
	else
		return false;

	// ignore 1 bit of original/copy
	// ignore 1 bit home
	// ignore 1 bit copyright id
	// ignore 1 bit copyright id start
	mi->frameLength = ((stream[3] & 3) << 11) | (stream[4] << 3) | ((stream[5] >> 5) & 0x07);		// 13 bits of aac_frame_length
	if (mi->frameLength < 7)
		return false;

	// ignore 11 bits of buffer fullness
	blockCount = (stream[6] & 0x03) + 1;
	
	if( mi->doExtendedParsing )
	{
		UInt32 sample_rate_ext = 0;
        SInt32 sbr_flag = 0;
		get_extended_aac_profile_level( &(stream[7]), mi->frequency, mi->channelCount, &sample_rate_ext, &sbr_flag, &mi->profile, &mi->level );
	}
	
	mi->samplesPerFrame = 1024 * blockCount;		//@@ if 1 != blockCount this won't work as expected

	mi->bitrate = ((((mi->frameLength << 3) * mi->frequency) / mi->samplesPerFrame) + 500) / 1000;

	mi->duration = 0;
	mi->xingToc = NULL;
	mi->codec = kFskAudioFormatAACADTS;

	return true;
}

#if SUPPORT_OMA
Boolean parseCODECInfoATRAC3(unsigned char *scan, DIDLMusicItem mi)
{
	Boolean isOK = false;
	
	static const int _At3BlockAlign[8] = {96, 136, 192, 272, 304, 384, 424, 512};
	static const int _At3BitRate[8] = {33,  47,  66,  94, 105, 132, 146, 176};
	UInt16 quality;

	if ((scan[0x02] >> 5) != 1) goto done;
	mi->frequency = 44100;
	mi->frameLength = (((scan[0x02] & 0x03) << 8) + scan[0x03]) * 8;
	mi->channelCount = 2;
	for (quality = 0; quality < 8; quality++)
		if (_At3BlockAlign[quality] == mi->frameLength)
			break;
	if (quality == 8) goto done;
	mi->bitrate = _At3BitRate[quality];
	mi->codec = kFskAudioFormatATRAC3;
	mi->samplesPerFrame = 1024;

	mi->atracParams.frameLength = (UInt16)mi->frameLength;
	mi->atracParams.bitrate = (UInt16)mi->bitrate;
	mi->atracParams.mode = mi->atracParams.bitrate <= 94 ? 1 : 0;
	mi->atracParams.emphasis = 0;

	isOK = true;

done:
	return isOK;
}

Boolean parseCODECInfoATRAC3plus(unsigned char *scan, DIDLMusicItem mi)
{
	Boolean isOK = false;

	static const int _AtxBlockAlign[10] = {192, 280, 376, 560, 744, 936, 1120, 1488, 1864, 2048};
	static const int _AtxBitRate[10]   = { 33,  48,  64,  96, 128, 160,  192,  256,  320, 352 };
	int i;
	if ((scan[0x02] >> 5) != 1) goto done;
	mi->frequency = 44100;
	mi->frameLength = (((scan[0x02] & 0x03) << 8) + scan[0x03] + 1) * 8;
	for (i = 0; i < 10; i++)
		if (_AtxBlockAlign[i] == mi->frameLength)
			break;
	if (i == 10) goto done;
	mi->bitrate = _AtxBitRate[i];
	switch ((scan[0x02] >> 2) & 0x07) {
	case 1:
		mi->channelCount = 1;
		break;
	case 2:
		mi->channelCount = 2;
		break;
	default:
		goto done;
	}
	mi->codec = kFskAudioFormatATRAC3Plus;
	mi->samplesPerFrame = 2048;

	mi->atracParams.frameLength = (UInt16)mi->frameLength;
	mi->atracParams.bitrate = (UInt16)mi->bitrate;
	mi->atracParams.mode = 0;
	mi->atracParams.emphasis = 0;

	isOK = true;

done:
	return isOK;
}


Boolean parseCODECInfoATRAC3AdvancedLossless(unsigned char *scan, DIDLMusicItem mi)
{
	Boolean isOK = false;

	unsigned int opmode;
	unsigned int wordlen;
	unsigned int adjustment;

	static const UInt16 _AtLosslessBlkLen[4] = { 512, 1024, 2048, 0 };
	static const int _AtLosslessFreq[4] = { 44100, 48000, 96000, 0 };

	// BLKLEN
	if (scan[0x01]>>6 == 0x03)
		goto done;
//	blklen = _AtLosslessBlkLen[scan[0x01]>>6];
	mi->atracParams.blockLength = _AtLosslessBlkLen[scan[0x01]>>6];
	mi->samplesPerFrame = _AtLosslessBlkLen[scan[0x01]>>6];

	// CCI
	if ((((scan[0x01]>>3)&0x07) != 0x02))
		goto done;
	mi->channelCount = 2;

	// OPMODE
	opmode = scan[0x01]&0x03;

	// FREQ
	if (scan[0x02]>>6 == 0x03)
		goto done;
	mi->frequency = _AtLosslessFreq[scan[0x02]>>6];

	// WORDLEN
	wordlen = scan[0x02]>>4 & 0x03;

	// ADJUSTMENT
	adjustment = (scan[0x02]&0x0F)<<8 | scan[0x03];
	mi->atracParams.adjustment = (UInt16)adjustment;
//	mi->codec = kFskAudioFormatATRACAdvancedLossless;

	isOK = true;

done:
	return isOK;	
}
#endif // #if SUPPORT_OMA

Boolean parseOMAHeader(unsigned char *scan, DIDLMusicItem mi)
{
	Boolean isOK = false;
#if SUPPORT_OMA
	UInt32 dataSize = (UInt32)(mi->fileSize - mi->id3TagSize);

	if (0 != FskStrCompareWithLength((const char*)scan, (const char*)"EA3", 3)) goto done;

	mi->omaHeaderSize = (scan[4] << 8) | scan[5];
	if (kOmaHeaderSize < mi->omaHeaderSize) goto done;	// E-ID must be 00001h
	dataSize -= mi->omaHeaderSize;

	switch (scan[0x20]) {
	case 0x00: // ATRAC3
		{
			if (false == parseCODECInfoATRAC3(scan + 0x20, mi))
				goto done;
			if (0 < dataSize)
				mi->duration = dataSize / mi->frameLength * mi->samplesPerFrame / mi->frequency;
		}
		break;
	case 0x01: // ATRAC-X
		{
			if (false == parseCODECInfoATRAC3plus(scan + 0x20, mi))
				goto done;
			if (0 < dataSize)
				mi->duration = dataSize / mi->frameLength * mi->samplesPerFrame / mi->frequency;
		}
		break;
	case 0x20:	// ATRAC Advanced Lossless (non base-layer)
		{
			if (false == parseCODECInfoATRAC3AdvancedLossless(scan + 0x20, mi))
				goto done;
			mi->duration = (scan[0x24]<<24 | scan[0x25]<<16 | scan[0x26]<<8 | scan[0x27])/1000;
		}
		break;
	case 0x21:	// ATRAC Advanced Lossless (ATRAC-X base-layer)
		{
			if (false == parseCODECInfoATRAC3plus(scan + 0x30, mi))
				goto done;
			if (false == parseCODECInfoATRAC3AdvancedLossless(scan + 0x20, mi))
				goto done;
			mi->duration = (scan[0x24]<<24 | scan[0x25]<<16 | scan[0x26]<<8 | scan[0x27])/1000;
		}
		break;
	case 0x22:	// ATRAC Advanced Lossless (ATRAC3 base-layer)
		{
			if (false == parseCODECInfoATRAC3(scan + 0x30, mi))
				goto done;
			if (false == parseCODECInfoATRAC3AdvancedLossless(scan + 0x20, mi))
				goto done;
			mi->duration = (scan[0x24]<<24 | scan[0x25]<<16 | scan[0x26]<<8 | scan[0x27])/1000;
		}
		break;
	default:
		goto done;
	}

	isOK = true;

done:
#endif
	return isOK;
}

Boolean parseXingTOC(unsigned char *scan, DIDLMusicItem mi)
{
	UInt32 bytes = mi->frameLength - 2;
	unsigned char *p = scan + 2;
	Boolean foundIt = false;

	for (foundIt = false; bytes > 112; p++, bytes--) {
		UInt32 flags;
		UInt32 frames = 0;

		if (('X' == p[0]) && ('i' == p[1]) && ('n' == p[2]) && ('g' == p[3]))
			foundIt = true;
		else if (('I' == p[0]) && ('n' == p[1]) && ('f' == p[2]) && ('o' == p[3]))
			foundIt = true;

		if (!foundIt)
			continue;

		FskMemPtrDisposeAt(&mi->xingToc);

		p += 4;
		flags = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		p += 4;
		if (1 & flags) {
			frames = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			p += 4;
		}
		if (2 & flags) {
			UInt32 value = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			p += 4;
			if (((FskInt64)value) < (mi->fileSize >> 1))
				break;							// this TOC covers less than half the file... that's bizarre. ignore it. this can happen when appending files together (e.g. a short VBR encoded advertisement, followed by the actual content)
		}
		if (frames)
			mi->duration = ((double)frames * mi->samplesPerFrame) / (double)mi->frequency;
		FskMemPtrNewFromData(100, p, &mi->xingToc);
		break;
	}

	return foundIt;
}


Boolean parseFhGTOC(unsigned char *scan, DIDLMusicItem mi)
{
	unsigned char *fhg = scan + 36;

	if (('V' == fhg[0]) && ('B' == fhg[1]) && ('R' == fhg[2]) && ('I' == fhg[3])) {
		UInt32 frames;

		frames = FskMisaligned32_GetN(fhg + 14);
		frames = FskEndianU32_BtoN(frames);
		mi->duration = (UInt32)(((double)frames * mi->samplesPerFrame) / (double)mi->frequency);

		return true;
	}

	return false;
}


Boolean scanMP3ID3v2(unsigned char *buffer, const unsigned char *bufferEnd, DIDLMusicItem mi)
{
	UInt16 majorVersion;
	UInt16 frameHeaderSize;
	id3TagAddProc addTagProc;
	UInt32 metaDataFormat;
	Boolean unsync;
#if SUPPORT_OMA
	UInt8 *fenca = NULL;
	UInt32 fencaSize = 0;
#endif

	if ((0 != FskStrCompareWithLength((const char*)buffer, (const char*)"ID3", 3))
#if SUPPORT_OMA
		&& (0 != FskStrCompareWithLength((const char*)buffer, (const char*)"ea3", 3))
#endif
		)
		return false;

	// skip header excitement
	majorVersion = buffer[3];
	unsync = 0 != (buffer[5] & 0x80);
	buffer += 10;

	if (majorVersion > 2) {
		frameHeaderSize = 10;
		addTagProc = id3v3TagToString;
		metaDataFormat = kFskMetaDataFormatID3v23;
	}
	else {
		frameHeaderSize = 6;
		addTagProc = id3v2TagToString;
		metaDataFormat = kFskMetaDataFormatID3v20;
	}

	if (4 == majorVersion) {
		// see if this might really be version 3 because mislabeling happens...
		unsigned char *b = buffer;

		while (b < (bufferEnd - 10)) {
			UInt32 size3, size4;

			size3 = (b[4] << 24) | (b[5] << 16) | (b[6] << 8) | b[7];
			size4 = ((b[4] & 0x7f) << 21) | ((b[5] & 0x7f) << 14) | ((b[6] & 0x7f) << 7) | (b[7] & 0x7f);

			if (size3 != size4) {
				if (0x80808080 & size3) {
					majorVersion = 3;			// not a sync size integer, can't be v4
					break;
				}

				if (size4 > (UInt32)(bufferEnd - b)) {
					// v4 interpretation would take us beyond end of data
					if (size3 <= (UInt32)(bufferEnd - b)) {
						// v3 stays in bounds, go with that
						majorVersion = 3;
					}
					break;
				}

				if (size3 > (UInt32)(bufferEnd - b)) {
					// v3 interpretation takes us beyond end of data (and v4 didn't), stick with v4
					break;
				}

				if ((b + size3 + 10) <= bufferEnd) {
					if (!isValidTag(b + size4 + 10) && isValidTag(b + size3 + 10)) {
						// garbage ahead if we stick with v4, but v3 looks ok.
						majorVersion = 3;
						break;
					}
				}
			}

			b += size4 + 10;
		}
	}

	// scan for tags we recognize
	while (buffer < (bufferEnd - 10)) {
		UInt32 tag = 0, size = 0;
		UInt16 flags = 0;
		unsigned char *t = NULL;
		char *fskTag = NULL;
		FskMediaPropertyValueRecord value;
		UInt32 unsyncSkip = 0;

		if (majorVersion > 2) {
			tag = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
			if (3 == majorVersion)
				size = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
			else
				size = ((buffer[4] & 0x7f) << 21) | ((buffer[5] & 0x7f) << 14) | ((buffer[6] & 0x7f) << 7) | (buffer[7] & 0x7f);
			flags = (buffer[8] << 8) | buffer[9];
		}
		else 
		if (2 == majorVersion) {
			tag = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
			size = (buffer[3] << 16) | (buffer[4] << 8) | buffer[5];
			flags = 0;
		}

		// must be garbage if it is this big
		if (size > 4 * 1024 * 1024)
			break;

		if ((SInt32)size > (bufferEnd - (buffer + frameHeaderSize)))
			break;

		if (0 == tag)
			break;

		if (flags & 0x00f0)
			goto nextTag;				// compressed or encrypted

		if (unsync) {
			unsigned char *i = buffer + frameHeaderSize;
			unsigned char *o = i;
			UInt32 ts = size;

			while (ts--) {
				unsigned char c = *i++;
				if ((0xff == c) && (0x00 == *i)) {
					i++;
					unsyncSkip++;
					ts--;
				}
				*o++ = c;
			}

			size -= unsyncSkip;
		}

		FskMediaMetaDataFormatTagToFskTag(metaDataFormat, &tag, (const char **)&fskTag, NULL, NULL);

		switch (tag) {
			case 'APIC': {
				const UInt8 *dataStart = buffer + 10;
				const UInt8 *s = dataStart;
				UInt8 textEncoding = *s++;
				const char *mime = (const char *)s;
				char *mimeSniffed;
				unsigned char *d;
				UInt32 newSize;

				s += FskStrLen(mime) + 1;
				if (NULL == FskStrChr(mime, '/'))
					mime = FskStrDoCat("image/", mime);

				s += 1;		// picture type
				if ((1 == textEncoding) || (2 == textEncoding))
					s += (FskUnicodeStrLen((UInt16 *)s) + 1) * 2;
				else
					s += FskStrLen((const char *)s) + 1;

				// the mime type labled is sometimes wrong, so sniff and trust that.
				if (kFskErrNone == FskImageDecompressSniffForMIME(s, size - (s - dataStart), NULL, NULL, &mimeSniffed)) {
					if (mime != (const char *)(dataStart + 1))
						FskMemPtrDispose((void *)mime);
					mime = mimeSniffed;
				}

				newSize = FskStrLen(mime) + 1 + size - (s - dataStart);
				if (kFskErrNone == FskMemPtrNew(newSize, (FskMemPtr *)&d)) {
					FskStrCopy((char *)d, mime);
					FskMemMove(d + FskStrLen(mime) + 1, s, size - (s - dataStart));

					value.type = kFskMediaPropertyTypeImage;
					value.value.data.data = d;
					value.value.data.dataSize = newSize;
					FskMediaMetaDataAdd(mi->meta, fskTag, NULL, &value, kFskMediaMetaDataFlagOwnIt);
				}

				if (mime != (const char *)(dataStart + 1))
					FskMemPtrDispose((void *)mime);

				fskTag = NULL;
				}
				break;

			case '\0PIC': {
				char *mimeType = NULL;
				if (('P' == buffer[7]) && ('N' == buffer[8]) && ('G' == buffer[9]))
					mimeType = "image/png";
				else if (('J' == buffer[7]) && ('P' == buffer[8]) && ('G' == buffer[9]))
					mimeType = "image/jpeg";
				else if (('G' == buffer[7]) && ('I' == buffer[8]) && ('F' == buffer[9]))
					mimeType = "image/gif";
				if (NULL != mimeType) {
					FskMemPtr albumArt;
					UInt32 mimeTypeLen = FskStrLen(mimeType) + 1;
					if (kFskErrNone == FskMemPtrNew(size + 20, &albumArt)) {
						char *d = (char *)albumArt;
						UInt32 descLen;
						UInt32 textEncoding = buffer[6];
						FskStrCopy(d, mimeType);		// mime type
						d += mimeTypeLen;
						if (1 != textEncoding)
							descLen = FskStrLen((const char *)&buffer[11]) + 1;
						else
							descLen = (FskUnicodeStrLen((UInt16 *)&buffer[11]) + 1) * 2;
						FskMemMove(d, &buffer[6 + 5 + descLen], size - (5 + descLen));

						value.type = kFskMediaPropertyTypeImage;
						value.value.data.data = albumArt;
						value.value.data.dataSize = mimeTypeLen + size - (5 + descLen);
						FskMediaMetaDataAdd(mi->meta, "AlbumArt", NULL, &value, kFskMediaMetaDataFlagOwnIt);
					}
				}
				}
				break;

			case 'PRIV':
				if (0 == FskStrCompare((const char *)buffer + 10, "WM/WMCollectionID")) {
					value.type = kFskMediaPropertyTypeData;
					value.value.data.data = (char *)buffer + 10 + 18;
					value.value.data.dataSize =	16;
					FskMediaMetaDataAdd(mi->meta, "WM/WMCollectionID", NULL, &value, 0);
				}
				break;

#if SUPPORT_OMA
			case 'GEOB': {
				const unsigned char *p = buffer + 10;
				unsigned char encoding = *p++;
				const unsigned char *mime = p;
				const unsigned char *fileName = mime + FskStrLen(mime) + 1;
				const unsigned char *description, *data;
				UInt32 dataSize;
				Boolean setAsAlbumArt = false;
				UInt32 metaFlags = kFskMediaMetaDataFlagOwnIt;

				if (0 == encoding)
					p = fileName + FskStrLen(fileName) + 1;
				else
					p = fileName + 2 * (FskUnicodeStrLen((UInt16 *)fileName) + 1);
				description = omgTextToUTF8(encoding, &p, size - (p - (buffer + 10)));
				data = p;
				dataSize = size - (data - (buffer + 10));

				if (0 == FskStrCompare(description, "OMG_FENCA1") ||
					0 == FskStrCompare(description, "OMG_E_FENCA1")) {

					if (0 == FskStrCompare(description, "OMG_FENCA1")) {
						if (kFskErrNone != FskMediaMetaDataGet(mi->meta, "AlbumArt", 1, NULL, NULL)) {
							// OMG_E_FENCA is not setted (, yet).
							fencaSize = size + 10;
							if (kFskErrNone == FskMemPtrNewFromData(fencaSize, buffer, &fenca))
								setAsAlbumArt = true;
						}
					} else {
						setAsAlbumArt = true;
						metaFlags |= kFskMediaMetaDataFlagEncrypted;
						if (NULL != fenca) {		// OMG_FENCA is already setted.
							FskMediaMetaDataRemove(mi->meta, "AlbumArt", 1);	// replace existing
							addAsOMATag(tag, description, fenca, fencaSize, mi);
							FskMemPtrDispose(fenca);
							fenca = NULL;
							fencaSize = 0;
						}
					}

				if (setAsAlbumArt) {
					UInt32 mimeLen = FskStrLen(mime) + 1;
					UInt32 albumArtSize = dataSize + mimeLen;
					FskMemPtr albumArt;

					if (kFskErrNone == FskMemPtrNew(albumArtSize, &albumArt)) {
						FskStrCopy(albumArt, mime);
						FskMemMove(albumArt + mimeLen, data, dataSize);
						value.type = kFskMediaPropertyTypeImage;
						value.value.data.data = albumArt;
						value.value.data.dataSize =	albumArtSize;
						FskMediaMetaDataAdd(mi->meta, "AlbumArt", NULL, &value, metaFlags);
					}
				}
				else {
						addAsOMATag(tag, description, buffer, size + 10, mi);
					}
				}
				else if (0 == FskStrCompare(description, "OMG_GNMID")) {
					value.type = kFskMediaPropertyTypeData;
					value.value.data.data = (void *)data;
					value.value.data.dataSize =	dataSize;
					FskMediaMetaDataAdd(mi->meta, "Sony/GracenoteID", NULL, &value, 0);
				}
				else if (0 == FskStrCompare(description, "OMG_MDLID")) {
					value.type = kFskMediaPropertyTypeData;
					value.value.data.data = (void *)data;
					value.value.data.dataSize =	dataSize;
					FskMediaMetaDataAdd(mi->meta, "Sony/MoodlogicID", NULL, &value, 0);
				}
				else if (0 == FskStrCompare(description, "OMG_LSI")) {
					value.type = kFskMediaPropertyTypeData;
					value.value.data.data = (void *)data;
					value.value.data.dataSize =	dataSize;
					FskMediaMetaDataAdd(mi->meta, "Sony/LSI", NULL, &value, 0);
				}
				else if (0 == FskStrCompare(description, "OMG_ULINF")) {
					value.type = kFskMediaPropertyTypeData;
					value.value.data.data = (void *)data;
					value.value.data.dataSize =	dataSize;
					FskMediaMetaDataAdd(mi->meta, "Sony/ULINF", NULL, &value, 0);
				}
				else if (0 == FskStrCompare(description, "OMG_OLINF")) {
					value.type = kFskMediaPropertyTypeData;
					value.value.data.data = (void *)data;
					value.value.data.dataSize =	dataSize;
					FskMediaMetaDataAdd(mi->meta, "Sony/OLINF", NULL, &value, 0);
				}
				else if (0 == FskStrCompare(description, "OMG_MARLN")) {
					value.type = kFskMediaPropertyTypeData;
					value.value.data.data = (void *)data;
					value.value.data.dataSize =	dataSize;
					FskMediaMetaDataAdd(mi->meta, "Sony/MARLN", NULL, &value, 0);
				}
				else {
					addAsOMATag(tag, description, buffer, size + 10, mi);
				}
				FskMemPtrDispose((FskMemPtr)description);
				}
				break;

			case 'TXXX': {
				const unsigned char *p = buffer + 10;
				unsigned char encoding = *p++;
				const char *description = omgTextToUTF8(encoding, &p, size - 1);
				t = omgTextToUTF8(encoding, &p, size - (p - (buffer + 10)));
				if (0 == FskStrCompare(description, "OMG_TRACK"))
					fskTag = "TrackNumber";
				else if (0 == FskStrCompare(description, "OMG_TPGMR"))
					fskTag = "Copyright";
				else {
					addAsOMATag(tag, description, buffer, size + 10, mi);
					FskMemPtrDisposeAt(&t);
				}
				FskMemPtrDispose((FskMemPtr)description);
				}
				break;

			case 'WXXX': {
				const unsigned char *p = buffer + 10;
				unsigned char encoding = *p++;
				const char *description = omgTextToUTF8(encoding, &p, size - 1);
				addAsOMATag(tag, description, buffer, size + 10, mi);
				FskMemPtrDispose((FskMemPtr)description);
				}
				break;
#endif
		}

		if ((NULL == t) && (NULL != fskTag))
			t = (unsigned char *)(*addTagProc)(buffer, size);

		if (t) {
			value.type = kFskMediaPropertyTypeString;
			value.value.str = (char *)t;
			FskMediaMetaDataAdd(mi->meta, fskTag, NULL, &value, kFskMediaMetaDataFlagOwnIt);
		}
#if SUPPORT_OMA
		else {
			if ('TXXX' != tag && 'WXXX' != tag && 'GEOB' != tag)
				addAsOMATag(tag, NULL, buffer, size + frameHeaderSize, mi);
		}
#endif

nextTag:
		buffer += (size + frameHeaderSize + unsyncSkip);
	}

#if SUPPORT_OMA
	FskMemPtrDispose(fenca);
#endif

	return true;
}

char *id3v2TagToString(const unsigned char *buffer, UInt32 size)
{
	char *result = NULL;

	if (0 == buffer[6])
		mp3TextLatin1ToUTF8((const char *)buffer + 7, size - 1, &result, NULL);
	else if (1 == buffer[6]) {
		if ((0xfe == buffer[7]) && (0xff == buffer[8]))
			FskTextUnicode16BEToUTF8((UInt16 *)(buffer + 9), size - 3, &result, NULL);
		else
		if ((0xff == buffer[7]) && (0xfe == buffer[8]))
			FskTextUnicode16LEToUTF8((UInt16 *)(buffer + 9), size - 3, &result, NULL);
	}

	return result;
}

char *id3v3TagToString(const unsigned char *buffer, UInt32 size)
{
	char *result = NULL;
	UInt8 characterEncoding = buffer[10];
	const unsigned char *text = &buffer[11];

	if (1 == characterEncoding) {
		// unicode 16 with BOM -> resolve BOM and move on
		if ((0xfe == text[0]) && (0xff == text[1]))
			FskTextUnicode16BEToUTF8((UInt16 *)(text + 2), size - 3, &result, NULL);
		else
		if ((0xff == text[0]) && (0xfe == text[1]))
			FskTextUnicode16LEToUTF8((UInt16 *)(text + 2), size - 3, &result, NULL);
	}
	else
	if (0 == characterEncoding)			// ISO 8859-1
		mp3TextLatin1ToUTF8((const char *)text, size - 1, &result, NULL);	
	else if (2 == characterEncoding)
		FskTextUnicode16BEToUTF8((UInt16 *)(text), size - 1, &result, NULL);
	else if (3 == characterEncoding) {	// UTF-8 - just what we wanted!
		if (kFskErrNone == FskMemPtrNew(size, &result)) {
			FskMemMove(result, text, size - 1);
			result[size - 1] = 0;
		}
	}

	return result;
}

Boolean scanMP3ID3v1(const unsigned char *buffer, DIDLMusicItem mi)
{
	UInt32 len;
	FskMediaPropertyValueRecord value;

	if (0 != FskStrCompareWithLength((const char*)buffer, (const char*)"TAG", 3))
		return false;

	value.type = kFskMediaPropertyTypeString;

	len = id3v1StrLen(buffer + 3);
	mp3TextLatin1ToUTF8((const char *)buffer + 3, len, &value.value.str, NULL);
	FskMediaMetaDataAdd(mi->meta, "FullName", NULL, &value, kFskMediaMetaDataFlagOwnIt);

	len = id3v1StrLen(buffer + 33);
	mp3TextLatin1ToUTF8((const char *)buffer + 33, len, &value.value.str, NULL);
	FskMediaMetaDataAdd(mi->meta, "Artist", NULL, &value, kFskMediaMetaDataFlagOwnIt);

	len = id3v1StrLen(buffer + 63);
	mp3TextLatin1ToUTF8((const char *)buffer + 63, len, &value.value.str, NULL);
	FskMediaMetaDataAdd(mi->meta, "Album", NULL, &value, kFskMediaMetaDataFlagOwnIt);

	if (0 == buffer[125]) {
		char trackNumber[10];

		FskStrNumToStr(buffer[126], trackNumber, sizeof(trackNumber));
		value.value.str = trackNumber;
		FskMediaMetaDataAdd(mi->meta, "TrackNumber", NULL, &value, 0);
	}

	if (0 != buffer[127]) {
		char genreCode[10];

		FskStrNumToStr(buffer[127], genreCode, sizeof(genreCode));
		value.value.str = genreCode;
		FskMediaMetaDataAdd(mi->meta, "Genre", NULL, &value, 0);
	}

	mi->id3TagSize = 128;

	return true;
}

UInt32 id3v1StrLen(const unsigned char *c)
{
	UInt32 i = 30;
	while (i--) {
		if (' ' != c[i])
			return i + 1;
	}
	return 0;

}

Boolean isValidTag(unsigned char *t)
{
	UInt32 i;

	for (i = 0; i < 4; i++) {
		char c = *t++;
		if ((('A' <= c) && (c <= 'Z')) || (('0' <= c) && (c <= '9')))
			continue;
		return false;
	}
	return true;
}

#if SUPPORT_OMA

char *omgTextToUTF8(char encoding, const unsigned char **p, SInt32 bytesAvailable)
{
	char *result = NULL;
	SInt32 bytesConsumed = 0;

	if (0 == encoding) {
		const unsigned char *t8;
		for (t8 = *p; bytesConsumed < bytesAvailable; t8++) {
			bytesConsumed += 1;
			if (0 == *t8)
				break;
		}
		mp3TextLatin1ToUTF8(*p, bytesConsumed, &result, NULL);
	}
	else if ((1 == encoding) || (2 == encoding)) {
		const UInt16 *t16;
		for (t16 = (UInt16 *)*p; bytesConsumed < bytesAvailable; t16++) {
			bytesConsumed += 2;
			if (0 == *t16)
				break;
		}

		if (1 == encoding) {
			if ((0xfe == (*p)[0]) && (0xff == (*p)[1]))
				FskTextUnicode16BEToUTF8((UInt16 *)(*p + 2), bytesConsumed - 2, &result, NULL);
			else
			if ((0xff == (*p)[0]) && (0xfe == (*p)[1]))
				FskTextUnicode16LEToUTF8((UInt16 *)(*p + 2), bytesConsumed - 2, &result, NULL);
		}
		else
			FskTextUnicode16BEToUTF8((UInt16 *)*p, bytesConsumed, &result, NULL);
	}

	*p += bytesConsumed;

	return result;
}

#if SUPPORT_OMA

#define FSK_TAG_LEN	50

FskErr addAsOMATag(UInt32 tag, const unsigned char *description, const unsigned char *tagData, UInt32 tagDataSize, DIDLMusicItem mi)
{
	FskMediaPropertyValueRecord value;
	char fskTag[FSK_TAG_LEN];
	char *p;
	int i, j;

	if ((NULL != description) && (0 == FskStrCompareWithLength("OMG_", description, 4)))
		FskStrCopy(fskTag, "OMA/");
	else
		FskStrCopy(fskTag, "ID3/");

	tag = FskEndianU32_NtoB(tag);
	p = (char *)&tag;
	for (i = 0, j = 4; i < 4; i++, p++) 
		if (*p != 0)
			fskTag[j++] = *p;
	fskTag[j] = 0;

	if (NULL != description) {
		if (FskStrLen(description) > FSK_TAG_LEN - 10)
		return kFskErrBadData;
		fskTag[j++] = '/';
		FskStrCopy(fskTag + j, description);
	}

	//printf("addAsOMATag: %s\n", fskTag);

	value.type = kFskMediaPropertyTypeData;
	value.value.data.data = (char *)tagData;
	value.value.data.dataSize =	tagDataSize;
	return FskMediaMetaDataAdd(mi->meta, fskTag, NULL, &value, 0);
}

#endif

#endif
