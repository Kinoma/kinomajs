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

#include "ASFReader.h"

#define kMilliToNano ((ASFFileOffset)10000)

typedef ASFErr (*asfObjectWalker)(ASFDemuxer demuxer, ASFFileOffset offset, ASFFileOffset size);

typedef struct {
	UInt8				guid[16];
	asfObjectWalker		walker;
} asfObjectWalkersRecord, *astObjectWalkers;

static ASFErr asfWalkObjects(ASFDemuxer demuxer, ASFFileOffset offset, ASFFileOffset size, astObjectWalkers walkers);

#define DeclareASFObjectProc(foo) static ASFErr foo(ASFDemuxer demuxer, ASFFileOffset objectOffset, ASFFileOffset objectSize)

DeclareASFObjectProc(asfHeaderObject);
DeclareASFObjectProc(asfDataObject);
DeclareASFObjectProc(asfSimpleIndexObject);
DeclareASFObjectProc(asfIndexObject);

DeclareASFObjectProc(asfFilePropertiesObject);
DeclareASFObjectProc(asfStreamPropertiesObject);
DeclareASFObjectProc(asfFileHeaderExtensionObject);
DeclareASFObjectProc(asfContentDescriptionObject);
DeclareASFObjectProc(asfExtendedContentDescriptionObject);
DeclareASFObjectProc(asfMetadataObject);
DeclareASFObjectProc(asfMetadataLibraryObject);
DeclareASFObjectProc(asfAdvancedMutualExclusionObject);
DeclareASFObjectProc(asfExtendedStreamPropertiesObject);

#define asfCalloc(demuxer, size, data) (demuxer->allocProc)(demuxer->allocRefCon, true, size, (void **)(void *)data)
#define asfMalloc(demuxer, size, data) (demuxer->allocProc)(demuxer->allocRefCon, false, size, (void **)(void *)data)
#define asfFree(demuxer, data) (demuxer->freeProc)(demuxer->allocRefCon, data)

#if !ASF_READER_FILE64
	static Boolean asfIs64Safe(unsigned char *p);
#else
	#define asfIs64Safe(b) (true)
#endif

#if TARGET_RT_BIG_ENDIAN
    #define asfReadNativeEndian32	asfReadBigEndian32
#else
    #define asfReadNativeEndian32	asfRead32
#endif

static ASFFileOffset asfRead64(unsigned char **p);
static UInt32 asfRead32(unsigned char **p);
//static UInt32 asfReadBigEndian32(unsigned char **p);
static UInt16 asfRead16(unsigned char **p);
static UInt8 asfRead8(unsigned char **p);
static UInt32 asfReadVariableSize(unsigned char **pp, UInt32 size);
static void asfMemMove(void *dstIn, void *srcIn, SInt32 count);
static UInt16 *asfExtractString(ASFDemuxer demuxer, unsigned char **pp, UInt16 len);
static void asfFlipContentDescriptor(ASFContentDescriptor desc);
static ASFErr asfReadMetaDataObject(ASFDemuxer demuxer, ASFFileOffset objectOffset, ASFFileOffset objectSize, ASFContentDescriptor *metadata, UInt16 *metadataCount, unsigned char **metadataObjectData);

static ASFErr asfBeginPacket(ASFDemuxer demuxer, UInt32 packetNumber);
static ASFErr asfEndPacket(ASFDemuxer demuxer);

ASFErr ASFDemuxerNew(ASFDemuxer *demuxerOut, ASFFileOffset maxOffset, ASFReadProc reader, void *readerRefCon, ASFAllocProc allocProc, ASFFreeProc freeProc, void *allocRefCon)
{
	ASFErr err = kASFErrNone;
	ASFDemuxer demuxer = NULL;
	UInt8 guid[16];
	asfObjectWalkersRecord walkers[] = {
		{{0x30, 0x26, 0xb2, 0x75, 0x8e, 0x66, 0xcf, 0x11, 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c}, NULL},
		{{0x36, 0x26, 0xb2, 0x75, 0x8e, 0x66, 0xcf, 0x11, 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c}, NULL},
		{{0x90, 0x08, 0x00, 0x33, 0xb1, 0xe5, 0xcf, 0x11, 0x89, 0xf4, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xcb}, NULL},
		{{0xd3, 0x29, 0xe2, 0xd6, 0xda, 0x35, 0xd1, 0x11, 0x90, 0x34, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xbe}, NULL},
		{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, NULL}
		};

	walkers[0].walker = asfHeaderObject;
	walkers[1].walker = asfDataObject;
	walkers[2].walker = asfSimpleIndexObject;
	walkers[3].walker = asfIndexObject;

	// check file signature (asf header object must be first)
	err = (reader)(readerRefCon, guid, 0, 16);
	BAIL_IF_ERR(err);

	if (false == ASFEqualGUIDS(guid, walkers[0].guid)) {
		err = kASFErrBadData;
		goto bail;
	}

	// create demuxer instance
	err = (allocProc)(allocRefCon, true, sizeof(ASFDemuxerRecord), (void **)(void *)&demuxer);
	BAIL_IF_ERR(err);

	demuxer->allocProc = allocProc;
	demuxer->freeProc = freeProc;
	demuxer->allocRefCon = allocRefCon;
	demuxer->reader = reader;
	demuxer->readerRefCon = readerRefCon;
	demuxer->packetNumber = -1;

	err = asfWalkObjects(demuxer, 0, maxOffset, walkers);
	if (kASFErrDataUnavailableAfterHeader == err) err = kASFErrNone;
	BAIL_IF_ERR(err);

	demuxer->loaded = true;

bail:
	if (err) {
		ASFDemuxerDispose(demuxer);
		demuxer = NULL;
	}

	*demuxerOut = demuxer;

	return err;
}

void ASFDemuxerDispose(ASFDemuxer demuxer)
{
	if (NULL == demuxer)
		return;

	while (demuxer->streams)
		ASFDemuxerStreamDispose(demuxer->streams);

	asfEndPacket(demuxer);
	asfFree(demuxer, demuxer->packet);

	asfFree(demuxer, demuxer->contentDesc_title);
	asfFree(demuxer, demuxer->contentDesc_author);
	asfFree(demuxer, demuxer->contentDesc_copyright);
	asfFree(demuxer, demuxer->contentDesc_description);
	asfFree(demuxer, demuxer->contentDesc_rating);

	asfFree(demuxer, demuxer->extendedContentDescriptors);
	asfFree(demuxer, demuxer->extendedContentDescriptionObjectData);

	asfFree(demuxer, demuxer->metadata);
	asfFree(demuxer, demuxer->metadataObjectData);

	asfFree(demuxer, demuxer->metadataLibrary);
	asfFree(demuxer, demuxer->metadataLibraryObjectData);

	while (demuxer->advancedMutualExclusions) {
		ASFAdvancedMutualExclusion exclusion = demuxer->advancedMutualExclusions;
		demuxer->advancedMutualExclusions = exclusion->next;
		asfFree(demuxer, exclusion);
	}

	asfFree(demuxer, demuxer);
}

ASFErr ASFDemuxerSeek(ASFDemuxer demuxer, ASFFileOffset targetTime, ASFFileOffset *actualTime)
{
	ASFErr err = kASFErrNone;
	UInt32 indexIndex, targetPacketNumber;
	ASFStream stream;

	asfEndPacket(demuxer);
	demuxer->packetNumber = -1;

again:
	// empty out pending frame data
	for (stream = demuxer->streams; NULL != stream; stream = stream->next) {
		asfFree(demuxer, stream->frame);
		stream->frame = NULL;
		stream->framePresentationTime = -1;
	}

	if (!demuxer->seekable)
		return kASFErrNone;			// no seeking allowed

	if (targetTime > demuxer->playDuration)
		targetTime = demuxer->playDuration;
	else
	if (targetTime < 0)
		targetTime = 0;

	if (0 != demuxer->index_fileOffset) {
		// read index entry from index
		unsigned char indexHeader[10];
		ASFFileOffset offset = demuxer->index_fileOffset;
		unsigned char *p;
		UInt32 intervalMS, specifiersCount, blockCount, i;
		UInt16 indexIntoEntry = kFskUInt16Max;
		UInt32 entryIndex;

		err = (demuxer->reader)(demuxer->readerRefCon, indexHeader, offset, sizeof(indexHeader));
		offset += sizeof(indexHeader);
		BAIL_IF_ERR(err);

		p = indexHeader;
		intervalMS = asfRead32(&p);
		specifiersCount = asfRead16(&p);
		blockCount = asfRead32(&p);
		if ((0 == intervalMS) || (0 == specifiersCount) || (0 == blockCount))
			goto indexFailed;

		// find a clean-point index for a stream we have
		for (i=0; i<specifiersCount; i++) {
			unsigned char indexSpecifier[4];
			UInt16 streamNumber, indexType;

			err = (demuxer->reader)(demuxer->readerRefCon, indexSpecifier, offset, sizeof(indexSpecifier));
			offset += sizeof(indexSpecifier);
			BAIL_IF_ERR(err);

			p = indexSpecifier;
			streamNumber = asfRead16(&p);
			indexType = asfRead16(&p);
			if (NULL == ASFDemuxerStreamGet(demuxer, streamNumber))
				continue;

			if (3 != indexType)		// clean-point index is what we want
				continue;

			indexIntoEntry = (UInt16)i;
			break;
		}

		if (kFskUInt16Max == indexIntoEntry) {
			demuxer->index_fileOffset = 0;		// don't bother look again, this index can't help us
			goto indexFailed;
		}

		entryIndex = (UInt32)((targetTime / kMilliToNano) / intervalMS);

		// walk the index blocks to find the one with this entryIndex
		for (i=0; i<blockCount; i++) {
			unsigned char scratch[8];
			UInt32 blockEntryCount, offsetFromBlockBase;
			ASFFileOffset blockBase;

			// get block entry count
			err = (demuxer->reader)(demuxer->readerRefCon, scratch, offset, 4);
			offset += 4;
			BAIL_IF_ERR(err);

			p = scratch;
			blockEntryCount = asfRead32(&p);
			if (blockEntryCount < entryIndex) {
				// not in this block, so skip it
				offset += (8 * specifiersCount) + (4 * specifiersCount * blockEntryCount);
				entryIndex -= blockEntryCount;
				continue;
			}

			// get block base
			err = (demuxer->reader)(demuxer->readerRefCon, scratch, offset + (indexIntoEntry * 8), 8);
			offset += 8 * specifiersCount;
			BAIL_IF_ERR(err);

			p = scratch;
			if (!asfIs64Safe(p))
				goto indexFailed;
			blockBase = asfRead64(&p);

			while (true) {
				// get offset from block base for this entry
				err = (demuxer->reader)(demuxer->readerRefCon, scratch,
											offset + (entryIndex * specifiersCount * 4) + (indexIntoEntry * 4), 4);
				BAIL_IF_ERR(err);

				p = scratch;
				offsetFromBlockBase = asfRead32(&p);
				if (0xffffffffu != offsetFromBlockBase)		// index entry unavailable
					break;

				if (0 == entryIndex) {
					offsetFromBlockBase = 0;
					break;
				}
				entryIndex -= 1;
			}

			targetPacketNumber = (UInt32)((blockBase + offsetFromBlockBase) / demuxer->maxPacketSize);

			goto loadPacket;
		}

		// didn't find it using the index, fall through to other cases
	}

indexFailed:
	if (0 != demuxer->simpleIndex_fileOffset) {
		// read index entry from simple index
		unsigned char simpleIndexEntry[6];
		unsigned char *p = simpleIndexEntry;

		indexIndex = (UInt32)(targetTime / demuxer->simpleIndex_interval);
		err = (demuxer->reader)(demuxer->readerRefCon, simpleIndexEntry,
				demuxer->simpleIndex_fileOffset + (indexIndex * 6), 6);
		BAIL_IF_ERR(err);

		targetPacketNumber = asfRead32(&p);
		goto loadPacket;
	}

	// assume constant bit rate and seek by packet
	if (targetTime < (demuxer->preroll * kMilliToNano))
		targetTime = 0;
	else
		targetTime -= demuxer->preroll * kMilliToNano;

	if (demuxer->playDuration)
		targetPacketNumber = (UInt32)((demuxer->dataPacketsCount * (targetTime / kMilliToNano)) / (demuxer->playDuration / kMilliToNano));
	else
		targetPacketNumber = 0;

loadPacket:
	// load up first packet from seek location
	demuxer->packetNumber = targetPacketNumber - 1;			// if begin packet fails because the data isn't availble yet (streaming) we need packetNumber to be correct, so we start demuxing from there when data does arrive
	err = asfBeginPacket(demuxer, targetPacketNumber);
	BAIL_IF_ERR(err);

	if (0 == demuxer->payloadCount) {
		err = kASFErrBadData;
		goto bail;
	}

	if ((demuxer->payloads[0].presentationTime > ((targetTime / kMilliToNano) + demuxer->preroll)) && (demuxer->simpleIndex_fileOffset || demuxer->index_fileOffset)) {
		err = kASFErrBadData;		// looks like a bad index
		goto bail;
	}

#if 0
//@@ bad for audio, good for video...
	// skip payloads until the first key frame
	while (demuxer->nextPayloadIndex < demuxer->payloadCount) {
		ASFPayload payload = &demuxer->payloads[demuxer->nextPayloadIndex];
		if (payload->keyFrame)
			break;
		demuxer->nextPayloadIndex++;
	}
#endif

	// return actual time seeked to
	if (actualTime)
		*actualTime = demuxer->payloads[demuxer->nextPayloadIndex].presentationTime * kMilliToNano;

bail:
	if (err) {
		// maybe a bad index, so bail on it and try again
        if (0 != demuxer->index_fileOffset) {
			demuxer->index_fileOffset = 0;
			goto again;
        }
		if (0 != demuxer->simpleIndex_fileOffset) {
			demuxer->simpleIndex_fileOffset = 0;
			goto again;
		}
	}
	

	return err;
}

ASFErr ASFDemuxerNextFrame(ASFDemuxer demuxer, void **data, UInt32 *dataSize, ASFStream *streamOut, ASFFileOffset *presentationTime, Boolean *keyFrame)
{
	ASFErr err = kASFErrNone;

	while (true) {
		ASFPayload payload;
		ASFStream stream;
		UInt32 bytesToUse;

		// make sure we've got a payload to read from
		if (demuxer->nextPayloadIndex >= demuxer->payloadCount) {
			err = asfBeginPacket(demuxer, demuxer->packetNumber + 1);
			BAIL_IF_ERR(err);
		}

		// start working on next payload
		payload = &demuxer->payloads[demuxer->nextPayloadIndex++];
		stream = payload->stream;
		if (NULL == stream)
			continue;						// no stream for this payload

		if ((NULL == stream->frame) || (payload->havePresentationTime && (payload->presentationTime != stream->framePresentationTime))) {
			if ((0 != payload->offsetIntoMediaObject) || (0 == payload->mediaObjectSize))
				continue;

			// start a fresh frame
			asfFree(demuxer, stream->frame);
			stream->frame = NULL;
			stream->framePresentationTime = -1;

			err = asfMalloc(demuxer, payload->mediaObjectSize + kFskDecompressorSlop, &stream->frame);
			BAIL_IF_ERR(err);

			stream->framePresentationTime = payload->presentationTime;
			stream->bytesInFrame = 0;
			stream->frameSize = payload->mediaObjectSize;
		}

		// copy this payload into frame
		bytesToUse = payload->dataSize;
		if ((bytesToUse + payload->offsetIntoMediaObject) > stream->frameSize)
			bytesToUse = stream->frameSize - payload->offsetIntoMediaObject;		//@@ sometimes when streaming we get bonus data?!?
		asfMemMove(stream->frame + payload->offsetIntoMediaObject, payload->data, bytesToUse);
		stream->bytesInFrame += bytesToUse;

		if (stream->bytesInFrame == stream->frameSize) {
			// this one is ready to return
			*data = stream->frame;
			*dataSize = stream->frameSize;
			if (streamOut) *streamOut = stream;
			if (presentationTime) *presentationTime = stream->framePresentationTime * kMilliToNano;
			if (keyFrame) *keyFrame = payload->keyFrame;

			stream->frame = NULL;
			stream->framePresentationTime = -1;
			return kASFErrNone;
		}
	}

bail:
	return err;
}

ASFContentDescriptor ASFDemuxerFindDescriptor(ASFDemuxer demuxer, UInt16 *name, UInt16 stream, ASFContentDescriptor descriptors, UInt32 descriptorCount)
{
	while (descriptorCount--) {
		if (stream == descriptors->streamNumber) {
			UInt16 *s1 = name, *s2 = descriptors->name;
			while (true) {
				UInt16 s2c = asfRead16((unsigned char **)(void *)&s2);
				if (*s1 == s2c) {
					if (0 == *s1)
						return descriptors;
				}
				else
					break;
				s1++;
			}
		}
		descriptors += 1;
	}

	return NULL;
}

ASFErr ASFDemuxerSetGetNextPacketProc(ASFDemuxer demuxer, ASFDemuxerGetNextPacketProc proc)
{
	demuxer->getNextPacket = proc;

	return kASFErrNone;
}

void ASFDemuxerStreamDispose(ASFStream stream)
{
	ASFDemuxer demuxer;
	ASFStream walker, prev;

	if (NULL == stream)
		return;

	demuxer = stream->demuxer;

	for (walker = demuxer->streams, prev = NULL; NULL != walker; walker = walker->next) {
		if (walker == stream) {
			if (NULL == prev)
				demuxer->streams = stream->next;
			else
				prev->next = stream->next;
			break;
		}
		prev = walker;
	}

	asfFree(demuxer, stream->typeSpecificData);
	asfFree(demuxer, stream->frame);
	asfFree(demuxer, stream->extendedProperties);
	asfFree(demuxer, stream);
}

ASFStream ASFDemuxerStreamGet(ASFDemuxer demuxer, UInt16 streamNumber)
{
	ASFStream stream;

	for (stream = demuxer->streams; NULL != stream; stream = stream->next) {
		if (stream->number == (UInt8)streamNumber)
			return stream;
	}

	return NULL;
}

ASFErr ASFDemuxerRescanForIndices(ASFDemuxer demuxer)
{
	ASFErr err = kASFErrNone;

	asfObjectWalkersRecord walkers[] = {
		{{0x90, 0x08, 0x00, 0x33, 0xb1, 0xe5, 0xcf, 0x11, 0x89, 0xf4, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xcb}, NULL},
		{{0xd3, 0x29, 0xe2, 0xd6, 0xda, 0x35, 0xd1, 0x11, 0x90, 0x34, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xbe}, NULL},
		{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, NULL}
		};

	walkers[0].walker = asfSimpleIndexObject;
	walkers[1].walker = asfIndexObject;

	if ((0 == demuxer->index_fileOffset) && (0 == demuxer->simpleIndex_fileOffset)) {
		Boolean saveLoaded = demuxer->loaded;
		demuxer->loaded = false;
		err = asfWalkObjects(demuxer, 0, 0x7fffffff, walkers);
		demuxer->loaded = saveLoaded;
	}

	return err;
	
}

ASFErr ASFDemuxerScanPacket(ASFDemuxer demuxer, unsigned char *p, UInt32 *sendTime, UInt32 *duration)
{
	ASFErr err = kASFErrNone;
	UInt8 lengthTypeFlags;

	if (0x80 & *p) {
		// error correction data present
		UInt8 errorCorrectionFlags = *p++;
		UInt8 errorCorrectionDataLength = (0 == (errorCorrectionFlags & 0x60)) ? (errorCorrectionFlags & 0x0f) : 0;

		if (errorCorrectionFlags & 0x10) {
			err = kASFErrBadData;			// opaque data present: must be zero
			goto bail;
		}

		p += errorCorrectionDataLength;
	}

	// payload parsing data
	lengthTypeFlags = *p++;
	p += 1;	// propertyFlags

	asfReadVariableSize(&p, (lengthTypeFlags >> 5) & 0x03);		// packetLength
	asfReadVariableSize(&p, (lengthTypeFlags >> 1) & 0x03);		// sequence
	asfReadVariableSize(&p, (lengthTypeFlags >> 3) & 0x03);		// paddingLength

	*sendTime = asfRead32(&p);
	*duration = asfRead16(&p);

bail:
	return err;
}

ASFErr asfWalkObjects(ASFDemuxer demuxer, ASFFileOffset offset, ASFFileOffset size, astObjectWalkers walkers)
{
	ASFErr err = kASFErrNone;

	// scan those objects
	while ((size >= 24) && (false == demuxer->loaded)) {
		astObjectWalkers w = walkers;
		unsigned char objectHeader[24];
		ASFFileOffset objectSize;
		unsigned char *p;

		err = (demuxer->reader)(demuxer->readerRefCon, objectHeader, offset, 24);
		BAIL_IF_ERR(err);

		if (!asfIs64Safe(&objectHeader[16])) {
			err = kASFErrUnimplemented;			// can't handle files with object sizes bigger than 32 bits
			goto bail;
		}

		p = &objectHeader[16];
		objectSize = asfRead64(&p);
		if (objectSize < 24) {
			err = kFskErrBadData;
			goto bail;
		}

		for (w = walkers; NULL != w->walker; w++) {
			if (!ASFEqualGUIDS(w->guid, objectHeader))
				continue;

			err = (w->walker)(demuxer, offset + 24, objectSize - 24);
			BAIL_IF_ERR(err);

			break;
		}

		offset += objectSize;
		size -= objectSize;
	}

bail:
	return err;
}

DeclareASFObjectProc(asfHeaderObject)
{
	ASFErr err;
	unsigned char header[6];
	asfObjectWalkersRecord walkers[] = {
		{{0xa1, 0xdc, 0xab, 0x8c, 0x47, 0xa9, 0xcf, 0x11, 0x8e, 0xe4, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65}, NULL},
		{{0x91, 0x07, 0xdc, 0xb7, 0xb7, 0xa9, 0xcf, 0x11, 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65}, NULL},
		{{0xb5, 0x03, 0xbf, 0x5f, 0x2e, 0xa9, 0xcf, 0x11, 0x8e, 0xe3, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65}, NULL},
		{{0x33, 0x26, 0xb2, 0x75, 0x8e, 0x66, 0xcf, 0x11, 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c}, NULL},
		{{0x40, 0xa4, 0xd0, 0xd2, 0x07, 0xe3, 0xd2, 0x11, 0x97, 0xf0, 0x00, 0xa0, 0xc9, 0x5e, 0xa8, 0x50}, NULL},
		{{0xDC, 0x29, 0xe2, 0xd6, 0xda, 0x35, 0xd1, 0x11, 0x90, 0x34, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xbe}, NULL},
		{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, NULL}
		};
	walkers[0].walker = asfFilePropertiesObject;
	walkers[1].walker = asfStreamPropertiesObject;
	walkers[2].walker = asfFileHeaderExtensionObject;
	walkers[3].walker = asfContentDescriptionObject;
	walkers[4].walker = asfExtendedContentDescriptionObject;
	walkers[5].walker = asfAdvancedMutualExclusionObject;			// bit-rate multiple exclusion group. same format as advanced mutual exclusion group, just different location

	if (objectSize < 6)
		return kASFErrBadData;

	err = (demuxer->reader)(demuxer->readerRefCon, header, objectOffset, 6);
	BAIL_IF_ERR(err);

	if (2 != header[5]) {
		err = kASFErrBadData;
		goto bail;
	}

	err = asfWalkObjects(demuxer, objectOffset + 6, objectSize - 6, walkers);
	BAIL_IF_ERR(err);

bail:
	// assign extended properties to streams, or toss them if stream not found
	while (NULL != demuxer->extendedProperties) {
		ASFExtendedStreamProperties exp = demuxer->extendedProperties;
		ASFStream stream;
		demuxer->extendedProperties = exp->next;
		exp->next = NULL;
		for (stream = demuxer->streams; (NULL != stream) && (NULL != exp); stream = stream->next) {
			if (stream->number == exp->streamNumber) {
				stream->extendedProperties = exp;
				exp = NULL;
			}
		}
		asfFree(demuxer, exp);
	}

	return err;
}

DeclareASFObjectProc(asfFilePropertiesObject)
{
	ASFErr err;
	unsigned char fileProperties[80];
	unsigned char *p = fileProperties + 16;		// +16 skips the file guid

	if (objectSize < sizeof(fileProperties))
		return kASFErrBadData;

	err = (demuxer->reader)(demuxer->readerRefCon, fileProperties, objectOffset, sizeof(fileProperties));
	BAIL_IF_ERR(err);

	if (!asfIs64Safe(p)) {
        err = kASFErrBadData;
        goto bail;
    }
	demuxer->fileSize = asfRead64(&p);

	p += 8;		// creation date

	if (!asfIs64Safe(p)) goto bail;
	demuxer->dataPacketsCount = asfRead64(&p);

	if (!asfIs64Safe(p)) goto bail;
	demuxer->playDuration = asfRead64(&p);

	if (!asfIs64Safe(p)) goto bail;
	demuxer->sendDuration = asfRead64(&p);

	if (!asfIs64Safe(p)) goto bail;
	demuxer->preroll = asfRead64(&p);

	if (0 != demuxer->playDuration)
		demuxer->playDuration -= (demuxer->preroll * kMilliToNano);
	
	if (!demuxer->dataPacketsCount)
		demuxer->dataPacketsCount = 0x7fffffffffffffffLL;		// live(-ish)

	err = kASFErrNone;

	demuxer->flags = asfRead32(&p);
	demuxer->minPacketSize = asfRead32(&p);
	demuxer->maxPacketSize = asfRead32(&p);
	demuxer->maxBitRate = asfRead32(&p);

	demuxer->seekable = 0 != (2 & demuxer->flags);

bail:
	return err;
}

DeclareASFObjectProc(asfStreamPropertiesObject)
{
	ASFErr err;
	unsigned char streamProperties[54];
	unsigned char *p = streamProperties + 16 + 16;	// skip stream type & error correction type
	ASFStream stream = NULL;
	const unsigned char audioStreamGUID[] = {0x40, 0x9e, 0x69, 0xf8, 0x4d, 0x5b, 0xcf, 0x11, 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b};
	const unsigned char videoStreamGUID[] = {0xc0, 0xef, 0x19, 0xbc, 0x4d, 0x5b, 0xcf, 0x11, 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b};
	const unsigned char imageJFIFStreamGUID[] = {0x00, 0xe1, 0x1b, 0xb6, 0x4e, 0x5b, 0xcf, 0x11, 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b};

	if (objectSize < sizeof(streamProperties))
		return kASFErrBadData;

	err = (demuxer->reader)(demuxer->readerRefCon, streamProperties, objectOffset, sizeof(streamProperties));
	BAIL_IF_ERR(err);

	err = asfCalloc(demuxer, sizeof(ASFStreamRecord), &stream);
	BAIL_IF_ERR(err);

	asfMemMove(stream->guid, &streamProperties[0], 16);

	if (!asfIs64Safe(p)) goto bail;
	stream->timeOffset = asfRead64(&p);
	stream->typeSpecificDataSize = asfRead32(&p);
	p += 4;		// error correction data length
	stream->flags = asfRead16(&p);
	stream->number = stream->flags & 0x7f;
	stream->encrypted = 0 != (stream->flags & 0x8000);

	if (stream->typeSpecificDataSize) {
		err = asfMalloc(demuxer, stream->typeSpecificDataSize, &stream->typeSpecificData);
		BAIL_IF_ERR(err);

		err = (demuxer->reader)(demuxer->readerRefCon, stream->typeSpecificData, objectOffset + 54, stream->typeSpecificDataSize);
		BAIL_IF_ERR(err);

		p = stream->typeSpecificData;
		if (ASFEqualGUIDS(audioStreamGUID, stream->guid)) {
			stream->mediaType = kASFMediaTypeAudio;

			stream->media.audio.formatTag = asfRead16(&p);
			stream->media.audio.numChannels = asfRead16(&p);
			stream->media.audio.samplesPerSecond = asfRead32(&p);
			stream->media.audio.averageBytesPerSecond = asfRead32(&p);
			stream->media.audio.blockAlign = asfRead16(&p);
			stream->media.audio.bitsPerSample = asfRead16(&p);
			stream->media.audio.codecSpecificDataSize = asfRead16(&p);
			if (stream->media.audio.codecSpecificDataSize)
				stream->media.audio.codecSpecificData = p;
		}
		else
		if (ASFEqualGUIDS(videoStreamGUID, stream->guid)) {
			UInt32 formatDataSize;

			stream->mediaType = kASFMediaTypeVideo;

			stream->media.video.encodedImageWidth = asfRead32(&p);
			stream->media.video.encodedImageHeight = asfRead32(&p);
			p += 1;		// skip undocumented flags
			formatDataSize = asfRead16(&p);
			if (formatDataSize < 40) {
				err = kASFErrBadData;
				goto bail;
			}

			formatDataSize = asfRead32(&p);		// yes, again
			stream->media.video.width = asfRead32(&p);
			stream->media.video.height = asfRead32(&p);
			stream->media.video.reserved = asfRead16(&p);
			stream->media.video.bitsPerPixel = asfRead16(&p);
			stream->media.video.compressionID = asfReadNativeEndian32(&p);
			stream->media.video.imageSize = asfRead32(&p);
			stream->media.video.hPixelsPerMeter = asfRead32(&p);
			stream->media.video.vPixelsPerMeter = asfRead32(&p);
			stream->media.video.colorsUsed = asfRead32(&p);
			stream->media.video.importantColors = asfRead32(&p);
			stream->media.video.codecSpecificDataSize = formatDataSize - 40;
			if (stream->media.video.codecSpecificDataSize)
				stream->media.video.codecSpecificData = p;
		}
		else
		if (ASFEqualGUIDS(imageJFIFStreamGUID, stream->guid)) {
			stream->mediaType = kASFMediaTypeImageJFIF;

			stream->media.imageJFIF.width = asfRead32(&p);
			stream->media.imageJFIF.height = asfRead32(&p);
		}
	}

	stream->demuxer = demuxer;
	stream->next = demuxer->streams;
	demuxer->streams = stream;

bail:
	if (err) {
		asfFree(demuxer, stream);
		stream = NULL;
	}

	return err;
}

DeclareASFObjectProc(asfFileHeaderExtensionObject)
{
	ASFErr err;
	unsigned char header[22];
	unsigned char *p;
	UInt32 dataSize;
	asfObjectWalkersRecord walkers[] = {
		{{0xea, 0xcb, 0xf8, 0xc5, 0xaf, 0x5b, 0x77, 0x48, 0x84, 0x67, 0xaa, 0x8c, 0x44, 0xfa, 0x4c, 0xca}, NULL},
		{{0x94, 0x1c, 0x23, 0x44, 0x98, 0x94, 0xd1, 0x49, 0xa1, 0x41, 0x1D, 0x13, 0x4E, 0x45, 0x70, 0x54}, NULL},
		{{0xcf, 0x49, 0x86, 0xa0, 0x75, 0x47, 0x70, 0x46, 0x8a, 0x16, 0x6e, 0x35, 0x35, 0x75, 0x66, 0xcd}, NULL},
		{{0xcb, 0xa5, 0xe6, 0x14, 0x72, 0xc6, 0x32, 0x43, 0x83, 0x99, 0xa9, 0x69, 0x52, 0x06, 0x5b, 0x5a}, NULL},
		{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, NULL}
		};
	walkers[0].walker = asfMetadataObject;
	walkers[1].walker = asfMetadataLibraryObject;
	walkers[2].walker = asfAdvancedMutualExclusionObject;
	walkers[3].walker = asfExtendedStreamPropertiesObject;

	if (objectSize < 22)
		return kASFErrBadData;

	err = (demuxer->reader)(demuxer->readerRefCon, header, objectOffset, 22);
	BAIL_IF_ERR(err);

	p = &header[18];
	dataSize = asfRead32(&p);

	err = asfWalkObjects(demuxer, objectOffset + 22, dataSize, walkers);
	BAIL_IF_ERR(err);

bail:
	return err;
}
DeclareASFObjectProc(asfContentDescriptionObject)
{
	ASFErr err;
	unsigned char *contentDescriptionObject = NULL;
	UInt16 titleLen, authorLen, copyrightLen, descriptionLen, ratingLen;
	unsigned char *p;
//@@ assert objectSize smaller than 32 bits
	err = asfMalloc(demuxer, (UInt32)objectSize, &contentDescriptionObject);
	BAIL_IF_ERR(err);

	err = (demuxer->reader)(demuxer->readerRefCon, contentDescriptionObject, objectOffset, (UInt32)objectSize);
	BAIL_IF_ERR(err);

	p = contentDescriptionObject;
	titleLen = asfRead16(&p);
	authorLen = asfRead16(&p);
	copyrightLen = asfRead16(&p);
	descriptionLen = asfRead16(&p);
	ratingLen = asfRead16(&p);

	demuxer->contentDesc_title = asfExtractString(demuxer, &p, titleLen);
	demuxer->contentDesc_author = asfExtractString(demuxer, &p, authorLen);
	demuxer->contentDesc_copyright = asfExtractString(demuxer, &p, copyrightLen);
	demuxer->contentDesc_description = asfExtractString(demuxer, &p, descriptionLen);
	demuxer->contentDesc_rating = asfExtractString(demuxer, &p, ratingLen);

bail:
	asfFree(demuxer, contentDescriptionObject);

	return err;
}

DeclareASFObjectProc(asfExtendedContentDescriptionObject)
{
	ASFErr err;
	unsigned char *p, *end;
	UInt16 i;
//@@ assert objectSize smaller than 32 bits
	err = asfMalloc(demuxer, (UInt32)objectSize, &demuxer->extendedContentDescriptionObjectData);
	BAIL_IF_ERR(err);

	err = (demuxer->reader)(demuxer->readerRefCon, demuxer->extendedContentDescriptionObjectData, objectOffset, (UInt32)objectSize);
	BAIL_IF_ERR(err);

	p = demuxer->extendedContentDescriptionObjectData;
	demuxer->extendedContentDescriptorsCount = asfRead16(&p);

	err = asfMalloc(demuxer, demuxer->extendedContentDescriptorsCount * sizeof(ASFContentDescriptorRecord), &demuxer->extendedContentDescriptors);
	BAIL_IF_ERR(err);

	end = p + objectSize - 2;

	for (i=0; i<demuxer->extendedContentDescriptorsCount; i++) {
		ASFContentDescriptor contentDescriptor = &demuxer->extendedContentDescriptors[i];
		UInt16 nameLength;

		if ((p + 4) > end) {
			err = kASFErrBadData;
			goto bail;
		}
		contentDescriptor->streamNumber = 0;
		nameLength = asfRead16(&p);
		contentDescriptor->name = (UInt16 *)p;
		if ((p + nameLength + 4) > end) {
			err = kASFErrBadData;
			goto bail;
		}
		p += nameLength;
		contentDescriptor->dataType = asfRead16(&p);
		contentDescriptor->valueLength = asfRead16(&p);
		if ((p + contentDescriptor->valueLength) > end) {
			err = kASFErrBadData;
			goto bail;
		}
		contentDescriptor->value = p;
		asfFlipContentDescriptor(contentDescriptor);
		p += contentDescriptor->valueLength;
	}

bail:
	return err;
}

DeclareASFObjectProc(asfDataObject)
{
	demuxer->dataObject_fileOffset = objectOffset + 26;

	if (objectSize & 0x8000000000000000ll)
		demuxer->loaded = true;				// live encoders may not set the size of the data object, so we just decide to be done here

	return kASFErrNone;
}

DeclareASFObjectProc(asfSimpleIndexObject)
{
	ASFErr err;
	unsigned char indexHeader[32];
	unsigned char *p = indexHeader + 16;		// +16 to skip File ID GUID

	if (objectSize < sizeof(indexHeader))
		return kASFErrBadData;

	err = (demuxer->reader)(demuxer->readerRefCon, indexHeader, objectOffset, sizeof(indexHeader));
	BAIL_IF_ERR(err);

	if (!asfIs64Safe(p)) {
		err = kASFErrUnimplemented;
		goto bail;
	}
	demuxer->simpleIndex_interval = asfRead64(&p);
	p += 4;		// skip maximum packet count
	demuxer->simpleIndex_count = asfRead32(&p);

	demuxer->simpleIndex_fileOffset = objectOffset + 32;

	if ((0 == demuxer->simpleIndex_interval) || (demuxer->simpleIndex_count <= 1))
		demuxer->simpleIndex_fileOffset = 0;		// useless index, so ignore it

bail:
	return err;
}

DeclareASFObjectProc(asfIndexObject)
{
	demuxer->index_fileOffset = objectOffset;

	return kASFErrNone;
}

DeclareASFObjectProc(asfMetadataObject)
{
	return asfReadMetaDataObject(demuxer, objectOffset, objectSize, &demuxer->metadata, &demuxer->metadataCount, &demuxer->metadataObjectData);
}

DeclareASFObjectProc(asfMetadataLibraryObject)
{
	return asfReadMetaDataObject(demuxer, objectOffset, objectSize, &demuxer->metadataLibrary, &demuxer->metadataLibraryCount, &demuxer->metadataLibraryObjectData);
}

DeclareASFObjectProc(asfAdvancedMutualExclusionObject)
{
	ASFErr err = kASFErrNone;
	unsigned char header[18];
	UInt16 streamCount, i;
	ASFAdvancedMutualExclusion me;
	unsigned char *p;
	const unsigned char bitrateGUID[] = {0x01, 0x2a, 0xe2, 0xd6, 0xda, 0x35, 0xd1, 0x11, 0x90, 0x34, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xbe};

	err = (demuxer->reader)(demuxer->readerRefCon, header, objectOffset, sizeof(header));
	BAIL_IF_ERR(err);

	streamCount = header[16] | (header[17] << 8);

	err = asfMalloc(demuxer, sizeof(ASFAdvancedMutualExclusionRecord) + (2 * streamCount), &me);
	BAIL_IF_ERR(err);

	err = (demuxer->reader)(demuxer->readerRefCon, me->streams, objectOffset + sizeof(header), 2 * streamCount);
	BAIL_IF_ERR(err);

	for (i = 0, p = (unsigned char *)me->streams; i < streamCount; i++)
		me->streams[i] = asfRead16(&p);

	if (ASFEqualGUIDS(bitrateGUID, header))
		me->exclusionType = kASFDemuxerExclusionBitRate;
	else
		me->exclusionType = kASFDemuxerExclusionUnknown;

	asfMemMove(&me->guid, header, 16);
	me->count = streamCount;
	me->next = demuxer->advancedMutualExclusions;
	demuxer->advancedMutualExclusions = me;

bail:
	return err;
}

DeclareASFObjectProc(asfExtendedStreamPropertiesObject)
{
	ASFErr err = kASFErrNone;
	unsigned char header[64];
	UInt16 streamNameCount, payloadExtensionCount;
	ASFFileOffset offset = objectOffset;
	ASFFileOffset averageTimePerFrame;
	unsigned char *p;
	ASFExtendedStreamProperties exp;

	err = (demuxer->reader)(demuxer->readerRefCon, header, objectOffset, sizeof(header));
	BAIL_IF_ERR(err);

	p = header + 52;
	averageTimePerFrame = asfRead64(&p);
	streamNameCount = header[60] | (header[61] << 8);
	payloadExtensionCount = header[62] | (header[63] << 8);

	offset += sizeof(header);
	while (streamNameCount--) {
		unsigned char nameLen[2];

		err = (demuxer->reader)(demuxer->readerRefCon, &nameLen, offset + 2, sizeof(nameLen));
		BAIL_IF_ERR(err);

		offset += 4 + (nameLen[0] + (nameLen[1] << 8));
	}

	while (payloadExtensionCount--) {
		unsigned char extensionLen[4];

		err = (demuxer->reader)(demuxer->readerRefCon, &extensionLen, offset + 18, sizeof(extensionLen));
		BAIL_IF_ERR(err);

		p = extensionLen;
		offset += 22 + asfRead32(&p);
	}

	err = asfCalloc(demuxer, sizeof(ASFExtendedStreamPropertiesRecord), &exp);
	BAIL_IF_ERR(err);

	exp->next = demuxer->extendedProperties;
	demuxer->extendedProperties = exp;

	p = &header[16];
	exp->bitRate = asfRead32(&p);
	exp->streamNumber = header[48] | (header[49] << 8);
	exp->averageTimePerFrame = averageTimePerFrame;

	if ((offset - objectOffset) < objectSize) {
		asfObjectWalkersRecord walkers[] = {
			{{0x91, 0x07, 0xdc, 0xb7, 0xb7, 0xa9, 0xcf, 0x11, 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65}, NULL},
			{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, NULL}
			};
		walkers[0].walker = asfStreamPropertiesObject;

		err = asfWalkObjects(demuxer, offset, objectSize - (offset - objectOffset), walkers);
		BAIL_IF_ERR(err);
	}

bail:
	return err;
}

ASFErr asfBeginPacket(ASFDemuxer demuxer, UInt32 packetNumber)
{
	ASFErr err;
	unsigned char *p, *pEnd;
	UInt8 lengthTypeFlags, propertyFlags;
	Boolean multiplePayloads;
	UInt32 packetLength, paddingLength;
	UInt32 replicatedDataLengthType, offsetIntoMediaObjectLengthType, mediaObjectNumberLengthType;
	UInt32 payloadIndex = 0;

	// unload previous packet
	asfEndPacket(demuxer);

	// validate packet number
	if (NULL == demuxer->getNextPacket) {
		if ((ASFFileOffset)packetNumber >= demuxer->dataPacketsCount) {
			err = kASFErrEndOfPackets;
			goto bail;
		}
	}

	// allocate packet buffer, if not already there
	if (NULL == demuxer->packet) {
		err = asfMalloc(demuxer, demuxer->maxPacketSize, &demuxer->packet);
		BAIL_IF_ERR(err);
	}

	// read packet
	if (NULL == demuxer->getNextPacket)
		err = (demuxer->reader)(demuxer->readerRefCon, demuxer->packet,
							demuxer->dataObject_fileOffset + (packetNumber * demuxer->maxPacketSize),
							demuxer->maxPacketSize);
	else
		err = demuxer->getNextPacket(demuxer->readerRefCon, demuxer->packet);
	BAIL_IF_ERR(err);

	// parse packet header
	p = demuxer->packet;
	pEnd = demuxer->packet + demuxer->maxPacketSize;

	if (0x80 & *p) {
		// error correction data present
		UInt8 errorCorrectionFlags = *p++;
		UInt8 errorCorrectionDataLength = (0 == (errorCorrectionFlags & 0x60)) ? (errorCorrectionFlags & 0x0f) : 0;

		if (errorCorrectionFlags & 0x10) {
			err = kASFErrBadData;			// opaque data present: must be zero
			goto bail;
		}

		p += errorCorrectionDataLength;
	}

	// payload parsing data
	lengthTypeFlags = *p++;
	propertyFlags = *p++;

	multiplePayloads = (1 & lengthTypeFlags) != 0;
	packetLength = asfReadVariableSize(&p, (lengthTypeFlags >> 5) & 0x03);
	(void)asfReadVariableSize(&p, (lengthTypeFlags >> 1) & 0x03);
	paddingLength = asfReadVariableSize(&p, (lengthTypeFlags >> 3) & 0x03);

	(void)asfRead32(&p);
	(void)asfRead16(&p);

	replicatedDataLengthType = (propertyFlags >> 0) & 0x03;
	offsetIntoMediaObjectLengthType = (propertyFlags >> 2) & 0x03;
	mediaObjectNumberLengthType = (propertyFlags >> 4) & 0x03;

	// packets
	if (false == multiplePayloads) {
		// single payload
		UInt8 streamNumber = asfRead8(&p);
		UInt32 replicatedDataLength;
		ASFPayloadRecord pr;

		pr.mediaObjectSize = 0;
		pr.presentationTime = 0;
		pr.streamNumber = streamNumber & 0x7f;
		pr.stream = ASFDemuxerStreamGet(demuxer, pr.streamNumber);
		pr.keyFrame = (0x80 & streamNumber) != 0;
		pr.mediaObjectNumber = asfReadVariableSize(&p, mediaObjectNumberLengthType);
		pr.offsetIntoMediaObject = asfReadVariableSize(&p, offsetIntoMediaObjectLengthType);
		replicatedDataLength = asfReadVariableSize(&p, replicatedDataLengthType);

		if (1 != replicatedDataLength) {
			err = asfMalloc(demuxer, sizeof(ASFPayloadRecord), &demuxer->payloads);
			BAIL_IF_ERR(err);

			demuxer->payloadCount = 1;

			if (0 != replicatedDataLength) {
				pr.mediaObjectSize = asfRead32(&p);
				pr.presentationTime = (UInt32)(asfRead32(&p) - demuxer->preroll);
				pr.havePresentationTime = true;
				p += replicatedDataLength - 8;
			}
			else
			{
				pr.mediaObjectSize = 0;
				pr.presentationTime = 0;
			}

			pr.dataSize = (packetLength ? packetLength : demuxer->maxPacketSize) - paddingLength - (p - demuxer->packet);
			pr.data = p;
			pr.isCompressed = false;

			// not good: structure copy
			demuxer->payloads[0] = pr;
		}
		else {
			// compressed payload data
			UInt8 presentationTimeDelta = *p++;
			SInt32 dataSize = (packetLength ? packetLength : demuxer->maxPacketSize) - paddingLength - (p - demuxer->packet);
			UInt8 *tp = p;
			UInt32 i;

			pr.presentationTime = (UInt32)(pr.offsetIntoMediaObject - demuxer->preroll);
			pr.havePresentationTime = true;
			pr.offsetIntoMediaObject = 0;
			pr.isCompressed = true;

			demuxer->payloadCount = 0;
			while (dataSize > 0) {
				UInt8 size = *tp++;
				dataSize -= (1 + size);
				tp += size;
//				if (tp > pr.dataSize) {
//					err = kASFErrBadData;
//					goto bail;
//				}
				demuxer->payloadCount += 1;
			}

			err = asfMalloc(demuxer, sizeof(ASFPayloadRecord) * demuxer->payloadCount, &demuxer->payloads);
			BAIL_IF_ERR(err);

			for (i=0; i<demuxer->payloadCount; i++) {
				pr.dataSize = *p++;
				pr.mediaObjectSize = pr.dataSize;
				pr.data = p;
				demuxer->payloads[i] = pr;

				p += pr.dataSize;				
				pr.presentationTime += presentationTimeDelta;
			}
		}
	}
	else {
		// multiple payloads
		UInt8 payloadFlags = *p++;
		UInt8 payloadCount = (0x3f & payloadFlags);
		UInt8 payloadLengthType = (payloadFlags >> 6) & 0x03;
		ASFPayload payload;
		UInt32 replicatedDataLength = 0;

		err = asfCalloc(demuxer, sizeof(ASFPayloadRecord) * payloadCount, &demuxer->payloads);
		BAIL_IF_ERR(err);

		for (payloadIndex = 0, payload = demuxer->payloads; payloadIndex < payloadCount; payloadIndex++, payload++) {
			UInt8 streamNumber = asfRead8(&p);

			if (p >= pEnd) {
				err = kASFErrBadData;
				goto bail;
			}

			payload->streamNumber = streamNumber & 0x7f;
			payload->stream = ASFDemuxerStreamGet(demuxer, payload->streamNumber);
			payload->keyFrame = (0x80 & streamNumber) != 0;
			payload->mediaObjectNumber = asfReadVariableSize(&p, mediaObjectNumberLengthType);
			payload->mediaObjectSize = 0;
			payload->presentationTime = 0;
			payload->havePresentationTime = false;
			
			payload->offsetIntoMediaObject = asfReadVariableSize(&p, offsetIntoMediaObjectLengthType);
			replicatedDataLength = asfReadVariableSize(&p, replicatedDataLengthType);

			if (1 != replicatedDataLength) {
				if (replicatedDataLength) {
					payload->mediaObjectSize = asfRead32(&p);
					payload->presentationTime = (UInt32)(asfRead32(&p) - demuxer->preroll);
					payload->havePresentationTime = true;
					p += replicatedDataLength - 8;
				}
				payload->dataSize = asfReadVariableSize(&p, payloadLengthType);
				payload->data = p;
				payload->isCompressed = false;
	
				p += payload->dataSize;
			}
			else {
				// multiple compressed payload
				ASFPayloadRecord pr;
				UInt8 presentationTimeDelta = *p++;
				UInt32 subPayloadLength;
				ASFPayload prev = demuxer->payloads;
				UInt8 *t;
				UInt32 tt;
				UInt8 compressedPayloadCount = 0;

				payload->presentationTime = (UInt32)(payload->offsetIntoMediaObject - demuxer->preroll);
				payload->havePresentationTime = true;
				payload->offsetIntoMediaObject = 0;
				payload->isCompressed = true;
				subPayloadLength = asfReadVariableSize(&p, payloadLengthType);

				pr = *payload;

				// count the sub-payloads
				t = p;
				tt = subPayloadLength;
				while (tt > 0)  {
					UInt8 thisPayloadLength = *t++;
					t += thisPayloadLength;
					tt -= (thisPayloadLength + 1);
					compressedPayloadCount++;
				}

				// increase payload array count to hold sub-payloads
				err = asfCalloc(demuxer, sizeof(ASFPayloadRecord) * (payloadCount + compressedPayloadCount - 1), &demuxer->payloads);
				BAIL_IF_ERR(err);

				asfMemMove(demuxer->payloads, prev, payloadIndex * sizeof(ASFPayloadRecord));
				asfFree(demuxer, prev);

				payloadCount += compressedPayloadCount - 1;
				payload = &demuxer->payloads[payloadIndex];

				// extract sub-payloads
				while (subPayloadLength > 0)  {
					UInt8 thisPayloadLength = *p++;

					if (p >= pEnd) {
						err = kASFErrBadData;
						goto bail;
					}

					pr.data = p;
					pr.dataSize = thisPayloadLength;
					pr.mediaObjectSize = thisPayloadLength;
					*payload++ = pr;
					payloadIndex++;

					subPayloadLength -= (thisPayloadLength + 1);
					pr.presentationTime += presentationTimeDelta;
					pr.mediaObjectNumber += 1;
					p += thisPayloadLength;
				}

				// allow for loop incrementing these
				payload--;
				payloadIndex--;
			}
		}

		demuxer->payloadCount = payloadCount;
	}

	if (0 == demuxer->payloadCount) {
		err = kASFErrBadData;
		goto bail;
	}

	// update current packet number 
	demuxer->packetNumber = packetNumber;
	demuxer->nextPayloadIndex = 0;

bail:
	if (err)
		asfEndPacket(demuxer);

	return err;
}

ASFErr asfEndPacket(ASFDemuxer demuxer)
{
	asfFree(demuxer, demuxer->payloads);
	demuxer->payloads = NULL;
	demuxer->payloadCount = 0;

	return kASFErrNone;
}

Boolean ASFEqualGUIDS(const unsigned char *a, const unsigned char *b)
{
	int i;

	for (i=0; i<16; i++) {
		if (a[i] != b[i])
			return false;
	}

	return true;
}


#if !ASF_READER_FILE64

Boolean asfIs64Safe(unsigned char *p)
{
	if (p[4] || p[5] || p[6] || p[7])
		return false;
	return true;
}
#endif

ASFFileOffset asfRead64(unsigned char **pp)
{
	unsigned char *p = *pp;
	ASFFileOffset result;

	result = p[0];
	result |= ((ASFFileOffset)p[1]) <<   8;
	result |= ((ASFFileOffset)p[2]) <<  16;
	result |= ((ASFFileOffset)p[3]) <<  24;
#if ASF_READER_FILE64
	result |= ((ASFFileOffset)p[4]) <<  32;
	result |= ((ASFFileOffset)p[5]) <<  40;
	result |= ((ASFFileOffset)p[6]) <<  48;
	result |= ((ASFFileOffset)p[7]) <<  56;
#endif

	*pp += 8;
	return result;
}

UInt32 asfRead32(unsigned char **pp)
{
	unsigned char *p = *pp;
	UInt32 result;

	result = p[0];
	result |= ((UInt32)p[1]) <<   8;
	result |= ((UInt32)p[2]) <<  16;
	result |= ((UInt32)p[3]) <<  24;

	*pp += 4;
	return result;
}

UInt16 asfRead16(unsigned char **pp)
{
	unsigned char *p = *pp;
	UInt16 result = (p[0] <<  0) | (p[1] <<  8);
	*pp += 2;
	return	result;
}

UInt8 asfRead8(unsigned char **p)
{
	UInt8 result = **p;
	*p += 1;
	return result;
}

UInt32 asfReadVariableSize(unsigned char **pp, UInt32 size)
{
	switch (size) {
		case 1:		return asfRead8(pp);
		case 2:		return asfRead16(pp);
		case 3:		return asfRead32(pp);
	}
	return 0;
}

void asfMemMove(void *dstIn, void *srcIn, SInt32 count)
{
	char *dst = dstIn, *src = srcIn;
	while (count--)
		*dst++ = *src++;
}

UInt16 *asfExtractString(ASFDemuxer demuxer, unsigned char **pp, UInt16 len)
{
	UInt16 *result;

	if (len <= 2)
		return NULL;

	if (0 != asfCalloc(demuxer, len + 2, &result))		// ensures that string is null terminated as required in spec
		return NULL;

	asfMemMove(result, *pp, len);

	*pp += len;

	return result;
}

void asfFlipContentDescriptor(ASFContentDescriptor desc)
{
	unsigned char *p = desc->value;
	UInt16 i16;
	UInt32 i32;

	switch (desc->dataType) {
		case 2:
			if (2 == desc->valueLength) {
				i16 = asfRead16(&p);
				asfMemMove(desc->value, &i16, 2);
			}
			else {
				i32 = asfRead32(&p);
				asfMemMove(desc->value, &i32, 4);
			}
			break;

		case 3:
			i32 = asfRead32(&p);
			asfMemMove(desc->value, &i32, 4);
			break;

		case 4:		//@@ 64 bit stuff
			break;

		case 5:
			i16 = asfRead16(&p);
			asfMemMove(desc->value, &i16, 2);
			break;

		case 6:
			break;	// GUID - nothing to do
	}
}

ASFErr asfReadMetaDataObject(ASFDemuxer demuxer, ASFFileOffset objectOffset, ASFFileOffset objectSize, ASFContentDescriptor *metadata, UInt16 *metadataCount, unsigned char **metadataObjectData)
{
	ASFErr err;
	unsigned char *p, *end;
	UInt16 i;
//@@ assert objectSize smaller than 32 bits
	err = asfMalloc(demuxer, (UInt32)objectSize, metadataObjectData);
	BAIL_IF_ERR(err);

	err = (demuxer->reader)(demuxer->readerRefCon, *metadataObjectData, objectOffset, (UInt32)objectSize);
	BAIL_IF_ERR(err);

	p = *metadataObjectData;
	*metadataCount = asfRead16(&p);

	err = asfMalloc(demuxer, *metadataCount * sizeof(ASFContentDescriptorRecord), metadata);
	BAIL_IF_ERR(err);

	end = p + objectSize;

	for (i=0; i<*metadataCount; i++) {
		ASFContentDescriptor md = &(*metadata)[i];
		UInt16 nameLength;

		if ((p + 12) > end) {
			err = kASFErrBadData;
			goto bail;
		}
		p += 2;			// reserved (or language index - which we ignore)
		md->streamNumber = asfRead16(&p);
		nameLength = asfRead16(&p);
		md->dataType = asfRead16(&p);
		md->valueLength = asfRead32(&p);
		md->name = (UInt16 *)p;
		p += nameLength;
		if ((p + md->valueLength) > end) {
			err = kASFErrBadData;
			goto bail;
		}
		md->value = p;
		asfFlipContentDescriptor(md);
		p += md->valueLength;
	}

bail:
	return err;
}


