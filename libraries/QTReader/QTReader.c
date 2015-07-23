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

#include "QTReader.h"

#include "QTMoviesFormat.h"

#define UNUSED(x)		(void)(x)

#define DeclareAtomProc(foo) static QTErr foo(QTMovie movie, QTFileOffset offset, QTFileOffset size)

DeclareAtomProc(qtMovieAtom);
DeclareAtomProc(qtFtypAtom);
DeclareAtomProc(qtProfileAtom);
DeclareAtomProc(qtMovieHeaderAtom);
DeclareAtomProc(qtMovieUserDataAtom);
DeclareAtomProc(qtMovieReferenceMovieAtom);
DeclareAtomProc(qtMovieReferenceMovieDescriptor);
DeclareAtomProc(qtMovieReferenceMovieDataRef);
DeclareAtomProc(qtMovieReferenceMovieDataRate);
DeclareAtomProc(qtMovieReferenceMovieVersionCheck);
#if QT_READER_MTDT
	DeclareAtomProc(qtMovieUUIDAtom);
#endif
#if QT_READER_ZLIB
	#include "zlib.h"
	DeclareAtomProc(qtMovieCompressedMovieAtom);
#endif
DeclareAtomProc(qtTrackAtom);
DeclareAtomProc(qtTrackHeaderAtom);
#if QT_READER_EDITS
	DeclareAtomProc(qtTrackEditsAtom);
	DeclareAtomProc(qtEditListAtom);
#endif /* QT_READER_EDITS */
#if QT_READER_TRACKREF
	DeclareAtomProc(qtTrackReferenceAtom);
#endif /* QT_READER_TRACKREF */
DeclareAtomProc(qtTrackUserDataAtom);
DeclareAtomProc(qtMediaAtom);
DeclareAtomProc(qtMediaHeaderAtom);
DeclareAtomProc(qtMediaHandlerAtom);
DeclareAtomProc(qtMediaInfoAtom);
DeclareAtomProc(qtDataInfoAtom);
DeclareAtomProc(qtDataRefsAtom);
DeclareAtomProc(qtDataRefAtom);
DeclareAtomProc(qtSampleTableAtom);
DeclareAtomProc(qtSampleDescAtom);
DeclareAtomProc(qtSampleDescriptionAtom);
DeclareAtomProc(qtTimeToSampleAtom);
DeclareAtomProc(qtSyncSampleAtom);
DeclareAtomProc(qtSampleToChunkAtom);
DeclareAtomProc(qtSampleSizeAtom);
#if QT_READER_STZ2
	#if QT_READER_AUTHORING
#pragma error("Cannot combine QT_READER_AUTHORING and QT_READER_STZ2")
	#endif /* QT_READER_AUTHORING */
	DeclareAtomProc(qtSampleSize2Atom);
#endif /* QT_READER_STZ2 */
DeclareAtomProc(qtChunkOffsetAtom);
DeclareAtomProc(qtChunkOffset64Atom);
DeclareAtomProc(qtCompositionTimeOffsetAtom);

static void qtSampleDescriptionGenericFlip(void *desc, Boolean toNative);
static void qtSampleDescriptionVideoFlip(void *desc, Boolean toNative);
static void qtSampleDescriptionSoundFlip(void *desc, Boolean toNative);
static void flipExtensions(SInt32 *header, UInt32 size, Boolean toNative);
static void appendSampleDescription(QTMedia media, QTSampleDescription desc);

#include "FskEndian.h"
#include "FskUtilities.h"
#define QTReaderToNative64(a) FskEndianU64_BtoN(a)
#define QTReaderToNative32(a) FskEndianU32_BtoN(a)
#define QTReaderToNative16(a) FskEndianU16_BtoN(a)
#define QTReaderMisaligned32_GetBtoN(a) FskMisaligned32_GetBtoN(a)
#define QTReaderMisaligned32_GetN(a) 	FskMisaligned32_GetN(a)
#define QTReaderMisaligned32_PutN(a,b)	FskMisaligned32_PutN(a,b)
#define QTReaderToBigEndian64(a) FskEndianU64_NtoB(a)
#define QTReaderToBigEndian32(a) FskEndianU32_NtoB(a)
#define QTReaderToBigEndian16(a) FskEndianU16_NtoB(a)

#define qtMovieCalloc(movie, size, data) (movie->allocProc)(movie->allocRefCon, true, size, (void **)(void *)(data))
#define qtMovieMalloc(movie, size, data) (movie->allocProc)(movie->allocRefCon, false, size, (void **)(void *)(data))
#define qtMovieFree(movie, data) (movie->freeProc)(movie->allocRefCon, data)

#define qtMovieRealloc(movie, mem, oldSize, newSize, slopSize) qtMovieRealloc_(movie, (void **)(void *)(mem), oldSize, newSize, slopSize)
static QTErr qtMovieRealloc_(QTMovie movie, void **mem, UInt32 oldSize, UInt32 newSize, UInt32 *slopSize);

#define qtMemMove(d, s, c) FskMemMove(d, s, c)
static UInt32 qtListCount(void *listHead);

#if QT_READER_MATRIX
    #if QT_READER_AUTHORING
        static void copyMatrixToBigEndian(UInt32 *dest, UInt32 *src);
    #endif /* QT_READER_AUTHORING */
	static void copyMatrixToNative(UInt32 *dest, UInt32 *src);
	static void initializeMatrix(QTMatrixRecord *matrix);
#endif /* QT_READER_MATRIX */

#if QT_READER_AUTHORING
static UInt32 qtGetCurrentTime(void);
#endif /* QT_READER_AUTHORING */

#ifndef QT_READER_SKIP_COVER_IN_MINIMAL
	#define QT_READER_SKIP_COVER_IN_MINIMAL 1
#endif /* !QT_READER_SKIP_COVER_IN_MINIMAL */

#if QT_READER_SKIP_COVER_IN_MINIMAL
	DeclareAtomProc(selectiveUserDataAtom);
	DeclareAtomProc(selectiveMetaUserDataAtom);
	DeclareAtomProc(selectiveILSTUserDataAtom);
#endif /* QT_READER_SKIP_COVER_IN_MINIMAL */

#if !QT_READER_STZ2
	#define qtReaderGetSampleSize(media, index) (media->sampleSizes[index])
#else /* QT_READER_STZ2 */
	static UInt32 qtReaderGetSampleSize(QTMedia media, UInt32 index);
#endif /* QT_READER_STZ2 */

static void calculateMovieDuration(QTMovie movie);

static QTErr QTUserDataGetiTunes_Priv(QTUserData userData, UInt32 udType, Boolean isText, void **userPtr, UInt32 *userPtrSize);

QTErr QTMovieNewFromReader(QTMovie *movieOut, Boolean minimal, QTMovieReadProc reader, void *readerRefCon, QTMovieAllocProc alloc, QTMovieFreeProc free, void *allocRefCon)
{
	QTErr err = 0;
	QTMovie movie = NULL;
	QTMovieAtomWalkersRecord walkers[] = {{MovieAID, NULL}, {'ftyp', NULL}, {'prfl', NULL}, {0,0}};
	walkers[0].walker = qtMovieAtom;
	walkers[1].walker = qtFtypAtom;
	walkers[2].walker = qtProfileAtom;

	err = QTMovieNew(&movie, 600, alloc, free, allocRefCon);
	if (err) goto bail;

	movie->reader = reader;
	movie->readerRefCon = readerRefCon;
	movie->minimal = minimal;
#if QT_READER_AUTHORING
	movie->loaded = false;
#endif /* QT_READER_AUTHORING */

	// scan for movie atom
#if !QT_READER_FILE64
	err = QTMovieWalkAtoms(movie, 0, 0xffffffff, walkers);
#else /* QT_READER_FILE64 */
	err = QTMovieWalkAtoms(movie, 0, 0x7fffffffffffffffll, walkers);
#endif /* QT_READER_FILE64 */

	if (err) goto bail;

bail:
	if (err) {
		QTMovieDispose(movie);
		movie = NULL;
	}

	*movieOut = movie;

	return err;
}

void QTMovieDispose(QTMovie movie)
{
	if (NULL == movie)
		return;

	while (movie->tracks) 
		QTTrackDispose(movie->tracks);

#if QT_READER_MTDT
		while (movie->mtdt) {
			QTMTDT next = movie->mtdt->next;
			qtMovieFree(movie, movie->mtdt);
			movie->mtdt = next;
		}
#endif /* QT_READER_MTDT */

#if QT_READER_EXTRA_ATOMS
		while (movie->extras) {
			QTExtraAtom next = movie->extras->next;
			qtMovieFree(movie, movie->extras);
			movie->extras = next;
		}
#endif /* QT_READER_EXTRA_ATOMS */

	while (movie->refMovieDescriptor) {
		QTReferenceMovieDescriptor desc = movie->refMovieDescriptor;
		QTReferenceMovieDescriptor next = desc->next;
		qtMovieFree(movie, desc->dataRef);
		qtMovieFree(movie, desc);
		movie->refMovieDescriptor = next;
	}

	qtMovieFree(movie, movie->compatibleBrands);
	qtMovieFree(movie, movie->profile);
	qtMovieFree(movie, movie->userData);
	qtMovieFree(movie, movie);
}

QTErr QTMovieWalkAtoms(QTMovie movie, QTFileOffset offset, QTFileOffset size, QTMovieAtomWalkers walkers)
{
	QTErr err = 0;
	UInt32 atomHeader[2];

	// scan those atoms
	// currently we ignore uuid atoms (thank you Microsoft) but that should not be necessary
	while ((size >= 8) && (false == movie->loaded)) {
		QTMovieAtomWalkers w = walkers;
		QTFileOffset atomSize;
		UInt32 atomHeaderSize;

		err = (movie->reader)(movie->readerRefCon, atomHeader, offset, sizeof(atomHeader));
		if (err) goto bail;

		atomSize = QTReaderToNative32(atomHeader[0]);
		atomHeader[1] = QTReaderToNative32(atomHeader[1]);

		if (1 != atomSize)
			atomHeaderSize = 8;
		else {
			unsigned char size64bytes[8];
			QTFileOffset size64;

			atomHeaderSize = 16;

			err = (movie->reader)(movie->readerRefCon, &size64bytes, offset + 8, 8);
			if (err) goto bail;

#if QT_READER_FILE64
			qtMemMove(&size64, size64bytes, 8);
			atomSize = QTReaderToNative64(size64);
#else /* !QT_READER_FILE64 */
			if (size64bytes[0] || size64bytes[1] || size64bytes[2] || size64bytes[3]) {
				err = -1;		// we can't handle a true 64 bit offset 
				goto bail;
			}
			qtMemMove(&size64, size64bytes + 4, 4);
			atomSize = QTReaderToNative32(size64);
#endif /* !QT_READER_FILE64 */
		}


		if ((atomSize < 8) || (atomSize > size)) {		// atom size too small (not even room for header) or too big (needs more space than remains in this span)
			err = -1;
			goto bail;
		}

		while (w->atomType) {
			if ((w->atomType == atomHeader[1]) || (kQTMovieWalkAnyAtom == w->atomType)) {
				err = (w->walker)(movie, offset + atomHeaderSize, atomSize - atomHeaderSize);
				if (err) goto bail;

				break;
			}

			w++;
		}

		offset += atomSize;
		size -= atomSize;
	}

bail:
	return err;
}

QTErr qtFtypAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;

	err = (movie->reader)(movie->readerRefCon, &movie->ftypBrand, offset, sizeof(movie->ftypBrand));
	if (err) goto bail;

	movie->ftypBrand = QTReaderToNative32(movie->ftypBrand);

	if (size > 8) {
		UInt32 i;

		movie->compatibleBrandCount = (UInt32)((size - 8) / 4);

		err = QTLoadData(movie, offset + 8, sizeof(UInt32) * movie->compatibleBrandCount, &movie->compatibleBrands);
		if (err) goto bail;

		for (i=0; i<movie->compatibleBrandCount; i++)
			movie->compatibleBrands[i] = QTReaderToNative32(movie->compatibleBrands[i]);
	}

bail:
	return err;
}

QTErr qtProfileAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	return QTLoadData(movie, offset, size, &movie->profile);		//@@ endian flip
}

QTErr qtMovieAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	QTMovieAtomWalkersRecord walkers[] = {{MovieHeaderAID, NULL}, {TrackAID, NULL}, {UserDataAID, NULL},  {ReferenceMovieRecordAID, NULL}, 
#if QT_READER_MTDT
		{'uuid', NULL},
#endif
#if QT_READER_ZLIB
		{'cmov', NULL},
#endif
		{0,0}};
	walkers[0].walker = qtMovieHeaderAtom;
	walkers[1].walker = qtTrackAtom;
	walkers[2].walker = qtMovieUserDataAtom;
	walkers[3].walker = qtMovieReferenceMovieAtom;
#if QT_READER_MTDT
	walkers[4].walker = qtMovieUUIDAtom;
	#if QT_READER_ZLIB
		walkers[5].walker = qtMovieCompressedMovieAtom;
	#endif
#elif QT_READER_ZLIB
	walkers[4].walker = qtMovieCompressedMovieAtom;
#endif

	err = QTMovieWalkAtoms(movie, offset, size, walkers);
	movie->loaded = true;

	if ((0 == err) && (0 == movie->duration))
		calculateMovieDuration(movie);

	return err;
}

QTErr qtMovieHeaderAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	MovieHeader *header = NULL;
	QTErr err;

	err = QTLoadData(movie, offset, size, &header);
	if (err) goto bail;

	movie->duration = QTReaderToNative32(header->duration);
	movie->scale = QTReaderToNative32(header->timeScale);

#if QT_READER_AUTHORING
	movie->nextTrackID = QTReaderToNative32(header->nextTrackID);
#endif /* QT_READER_AUTHORING */

bail:
	qtMovieFree(movie, header);

	return err;
}

QTErr qtMovieUserDataAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	QTUserData userData;

#if QT_READER_SKIP_COVER_IN_MINIMAL
	if (false == movie->minimal) {
#endif /* QT_READER_SKIP_COVER_IN_MINIMAL */
		// we over allocate by 4 bytes to get a trailing null to terminate the user data atom walker
		err = qtMovieCalloc(movie, (UInt32)(size + sizeof(UInt32) + sizeof(QTUserDataRecord)), &userData);
		if (err) goto bail;

		movie->userData = userData;
		userData->movie = movie;
		userData->size = (UInt32)size;

		err = (movie->reader)(movie->readerRefCon, &userData->data, offset, (UInt32)size);
#if QT_READER_SKIP_COVER_IN_MINIMAL
	}
	else {
		QTMovieAtomWalkersRecord walkers[] = {{kQTMovieWalkAnyAtom, NULL}, {0,0}};
		walkers[0].walker = selectiveUserDataAtom;

		// we over allocate by 4 bytes to get a trailing null to terminate the user data atom walker
		err = qtMovieCalloc(movie, sizeof(UInt32) + sizeof(QTUserDataRecord), &userData);
		if (err) goto bail;

		movie->userData = userData;
		userData->movie = movie;
		userData->size = 0;

		err = QTMovieWalkAtoms(movie, offset, size, walkers);
	}
#endif /* QT_READER_SKIP_COVER_IN_MINIMAL */

bail:
	return err;
}

QTErr qtMovieReferenceMovieAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTMovieAtomWalkersRecord walkers[] = {{ReferenceMovieDescriptorAID, NULL}, {0, 0}};

	walkers[0].walker = qtMovieReferenceMovieDescriptor;
	return QTMovieWalkAtoms(movie, offset, size, walkers);
}

QTErr qtMovieReferenceMovieDescriptor(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	QTMovieAtomWalkersRecord walkers[] = {{ReferenceMovieDataRefAID, NULL}, {ReferenceMovieDataRateAID, NULL}, {ReferenceMovieVersionCheckAID, NULL}, {0, 0}};
	QTReferenceMovieDescriptor desc;

	walkers[0].walker = qtMovieReferenceMovieDataRef;
	walkers[1].walker = qtMovieReferenceMovieDataRate;
	walkers[2].walker = qtMovieReferenceMovieVersionCheck;

	err = qtMovieCalloc(movie, sizeof(QTReferenceMovieDescriptorRecord), &desc);
	if (err) return err;

	if (NULL == movie->refMovieDescriptor)
		movie->refMovieDescriptor = desc;
	else {
		QTReferenceMovieDescriptor w = movie->refMovieDescriptor;
		while (w) {
			if (!w->next) {
				w->next = desc;
				break;
			}
			w = w->next;
		}
	}

	movie->curRefMovieDescriptor = desc;

	return QTMovieWalkAtoms(movie, offset, size, walkers);
}

QTErr qtMovieReferenceMovieDataRef(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt32 data[3];

	err = (movie->reader)(movie->readerRefCon, data, offset, sizeof(data));
	if (err) return err;

	if (('url ' != QTReaderToBigEndian32(data[1])) || (0 != data[0]))
		return kFskErrUnimplemented;

	movie->curRefMovieDescriptor->dataRefSize = QTReaderToBigEndian32(data[2]);
	return QTLoadData(movie, offset + sizeof(data), movie->curRefMovieDescriptor->dataRefSize, &movie->curRefMovieDescriptor->dataRef);
}

QTErr qtMovieReferenceMovieDataRate(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt32 data[2];

	err = (movie->reader)(movie->readerRefCon, data, offset, sizeof(data));
	if (err) return err;

	movie->curRefMovieDescriptor->dataRate = QTReaderToBigEndian32(data[1]);

	return 0;
}

QTErr qtMovieReferenceMovieVersionCheck(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt32 data[4];

	err = (movie->reader)(movie->readerRefCon, data, offset, sizeof(data));
	if (err) return err;

	if (('mobi' == QTReaderToBigEndian32(data[1])) && (1 & QTReaderToBigEndian32(data[2])) && (1 & QTReaderToBigEndian32(data[3])))
		movie->curRefMovieDescriptor->wantsMobile = true;

	return 0;
}

#if QT_READER_MTDT

QTErr qtMovieUUIDAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt16 count, i;
	const unsigned char uuid[] = {0x55, 0x53, 0x4D, 0x54, 0x21, 0xD2, 0x4F, 0xCE, 0xBB, 0x88, 0x69, 0x5C, 0xFA, 0xC9, 0xC7, 0x40};
	unsigned char header[16 + 8 + 2];

	err = (movie->reader)(movie->readerRefCon, header, offset, sizeof(header));
	offset += sizeof(header);
	if (err) goto bail;

	for (i=0; i<16; i++) {
		if (uuid[i] != header[i])
			goto bail;
	}

	if (('M' != header[20]) || ('T' != header[21]) || ('D' != header[22]) || ('T' != header[23]))
		goto bail;

	count = (header[24] << 8) + header[25];
	for (i=0; i<count; i++) {
		QTMTDTRecord temp;
		QTMTDT mtdt;

		err = (movie->reader)(movie->readerRefCon, &temp.dataSize, offset, 10);
		offset += 10;
		if (err) goto bail;

		temp.next = NULL;
		temp.dataSize = QTReaderToBigEndian16(temp.dataSize) - 10;
		temp.dataType = QTReaderToBigEndian32(temp.dataType);
		temp.language = QTReaderToBigEndian16(temp.language);
		temp.encoding = QTReaderToBigEndian16(temp.encoding);

		err = qtMovieMalloc(movie, temp.dataSize + sizeof(QTMTDTRecord), &mtdt);
		if (err) goto bail;

		qtMemMove(mtdt, &temp, sizeof(temp) - 1);

		mtdt->next = movie->mtdt;
		movie->mtdt = mtdt;

		err = (movie->reader)(movie->readerRefCon, &mtdt->data, offset, mtdt->dataSize);
		offset += mtdt->dataSize;
		if (err) goto bail;
	}

bail:
	return err;
}

#endif

QTErr qtTrackAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt16 walkerIndex = 0;
	QTMovieAtomWalkersRecord walkers[] = {{TrackHeaderAID, NULL}, 
#if QT_READER_EDITS
											{EditsAID, NULL},
#endif /* QT_READER_EDITS */
#if QT_READER_TRACKREF
											{TrackReferenceAID, NULL},
#endif /* QT_READER_TRACKREF */
       {MediaAID, NULL}, {UserDataAID, NULL}, {0,0}};
	
	walkers[walkerIndex++].walker = qtTrackHeaderAtom;
#if QT_READER_EDITS
	walkers[walkerIndex++].walker = qtTrackEditsAtom;
#endif /* QT_READER_EDITS */
#if QT_READER_TRACKREF
	walkers[walkerIndex++].walker = qtTrackReferenceAtom;
#endif /* QT_READER_TRACKREF */
	walkers[walkerIndex++].walker = qtMediaAtom;
	walkers[walkerIndex++].walker = qtTrackUserDataAtom;

	err = QTTrackNew(&movie->curTrack, movie, 0, 0, 0);
	if (err) return err;

	return QTMovieWalkAtoms(movie, offset, size, walkers);
}

QTErr qtTrackHeaderAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	TrackHeader *header = NULL;
	QTTrack track = movie->curTrack;
	QTErr err;

	err = QTLoadData(movie, offset, size, &header);
	if (err) goto bail;

	// load header fields
	track->width = (UInt16)(QTReaderToNative32(header->trackWidth) >> 16);
	track->height = (UInt16)(QTReaderToNative32(header->trackHeight) >> 16);
	track->volume = QTReaderToNative16(header->volume);

	track->duration = QTReaderToNative32(header->duration);
	track->id = QTReaderToNative32(header->trackID);
	track->enabled = 0 != (QTReaderToNative32(header->flags) & 1);
#if QT_READER_MATRIX
	copyMatrixToNative(track->matrix, (UInt32 *)(void*)&header->matrix);
#endif /* QT_READER_MATRIX */

bail:
	qtMovieFree(movie, header);

	return err;
}

#if QT_READER_EDITS

QTErr qtTrackEditsAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTMovieAtomWalkersRecord walkers[] = {{EditListAID, NULL}, {0,0}};
	walkers[0].walker = qtEditListAtom;
	return QTMovieWalkAtoms(movie, offset, size, walkers);
}

QTErr qtEditListAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	QTEdit edit;
	UInt16 i;

	err = QTLoadData(movie, offset + 8, size - 8, &movie->curTrack->edits);
	if (err) goto bail;

	movie->curTrack->editsCount = (UInt16)((size - 8) / 12);
	for (i = 0,	edit = movie->curTrack->edits; i < movie->curTrack->editsCount; i++, edit++) {
		edit->mediaTime = QTReaderToNative32(edit->mediaTime);
		edit->trackDuration = QTReaderToNative32(edit->trackDuration);
		edit->rate = QTReaderToNative32(edit->rate);
	}

bail:
	return err;
}

#endif /* QT_READER_EDITS */

#if QT_READER_TRACKREF

QTErr qtTrackReferenceAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	movie->curTrack->trackRefSize = (UInt32)size;
	return QTLoadData(movie, offset, size, &movie->curTrack->trackRefs);
}

#endif /* QT_READER_TRACKREF */

QTErr qtTrackUserDataAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	QTUserData userData;
	
	// we over allocate by 4 bytes to get a trailing null to terminate the user data atom walker
	err = qtMovieCalloc(movie, (UInt32)size + sizeof(UInt32) + sizeof(QTUserDataRecord), &userData);
	if (err) goto bail;

	movie->curTrack->userData = userData;
	userData->movie = movie;
	userData->size = (UInt32)size;

	err = (movie->reader)(movie->readerRefCon, &userData->data, offset, (UInt32)size);
	if (err) goto bail;

bail:
	return err;
}

QTErr qtMediaAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	QTMovieAtomWalkersRecord walkers[] = {{MediaHeaderAID, NULL}, {HandlerAID, NULL}, {MediaInfoAID, NULL}, {0,0}};
	walkers[0].walker = qtMediaHeaderAtom;
	walkers[1].walker = qtMediaHandlerAtom;
	walkers[2].walker = qtMediaInfoAtom;

	err = QTMediaNew(&movie->curMedia, movie->curTrack, 600, 0);
	if (err) return err;

	return QTMovieWalkAtoms(movie, offset, size, walkers);
}

QTErr qtMediaHeaderAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	MediaHeader header;
	QTMedia media = movie->curMedia;
	QTErr err;

	err = (movie->reader)(movie->readerRefCon, &header, offset, (UInt32)size);
	if (err) goto bail;

	media->scale = QTReaderToNative32(header.timeScale);
	media->duration = QTReaderToNative32(header.duration);

bail:
	return err;
}

QTErr qtMediaHandlerAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	PublicHandlerInfo info;
	QTErr err;
	UNUSED(size);

	err = (movie->reader)(movie->readerRefCon, &info, offset, sizeof(info));
	if (err) return err;

	movie->curMedia->mediaType = QTReaderToNative32(info.componentSubType);

	return err;
}

QTErr qtMediaInfoAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTMovieAtomWalkersRecord walkers[] = {{DataInfoAID, NULL}, {SampleTableAID, NULL}, {0,0}};
	walkers[0].walker = qtDataInfoAtom;
	walkers[1].walker = qtSampleTableAtom;
	return QTMovieWalkAtoms(movie, offset, size, walkers);
}

QTErr qtDataInfoAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTMovieAtomWalkersRecord walkers[] = {{DataRefAID, NULL}, {0,0}};
	walkers[0].walker = qtDataRefsAtom;
	return QTMovieWalkAtoms(movie, offset, size, walkers);
}

QTErr qtDataRefsAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTMovieAtomWalkersRecord walkers[] = {{kQTMovieWalkAnyAtom, NULL}, {0,0}};
	walkers[0].walker = qtDataRefAtom;
	return QTMovieWalkAtoms(movie, offset + 8, size - 8, walkers);
}

QTErr qtDataRefAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt32 d[3];
	void *dr = NULL;

	err = (movie->reader)(movie->readerRefCon, d, offset - 8, 12);
	if (err) return err;

	if (4 == size)
		dr = NULL;
	else if (size > 4) {
		// not self-=contained, so load-up the data too
		err = QTLoadData(movie, offset + 4, size - 4, &dr);
		if (err) return err;
	}
	else
		return -1;

	err = QTMediaAddDataReference(movie->curMedia, QTReaderToNative32(d[1]), QTReaderToNative32(d[2]), dr, (UInt32)(size - 4), NULL);
	qtMovieFree(movie, dr);

	return err;
}

QTErr qtSampleTableAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTMovieAtomWalkersRecord walkers[] = {{STTimeToSampAID, NULL},
			{STSampleToChunkAID, NULL},
			{STSampleSizeAID, NULL},
#if QT_READER_STZ2
			 {'stz2', NULL},
#endif /* QT_READER_STZ2 */
			{STSampleDescAID, NULL}, 
			{STSyncSampleAID, NULL}, {STChunkOffsetAID, NULL}, 
			{STChunkOffset64AID, NULL}, {STCompositionTimeAID, NULL},
			{0,0}};
	QTMovieAtomWalkersRecord *w = walkers;
	UInt32 minimalIndex;
	w->walker = qtTimeToSampleAtom;  w++;
	w->walker = qtSampleToChunkAtom;  w++;
	w->walker = qtSampleSizeAtom;  w++;
#if QT_READER_STZ2
	w->walker = qtSampleSize2Atom;  w++;
#endif /* QT_READER_STZ2 */
	minimalIndex = w - walkers;
	w->walker = qtSampleDescAtom;  w++;
	w->walker = qtSyncSampleAtom;  w++;
	w->walker = qtChunkOffsetAtom;  w++;
	w->walker = qtChunkOffset64Atom;  w++;
	w->walker = qtCompositionTimeOffsetAtom;  w++;
	return QTMovieWalkAtoms(movie, offset, size, walkers + (movie->minimal ? minimalIndex : 0));
}

QTErr qtSampleDescAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTMovieAtomWalkersRecord walkers[] = {{kQTMovieWalkAnyAtom, NULL}, {0,0}};
	walkers[0].walker = qtSampleDescriptionAtom;
	return QTMovieWalkAtoms(movie, offset + 8, size - 8, walkers);
}

QTErr qtSampleDescriptionAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	QTSampleDescription desc;

	// in this case, we want the atom header too
	size += 8;
	offset -= 8;

	err = qtMovieCalloc(movie, (UInt32)size + sizeof(QTSampleDescriptionRecord), &desc);
	if (err) return err;

	err = (movie->reader)(movie->readerRefCon, &desc->desc, offset, (UInt32)size);
	if (err) return err;

	appendSampleDescription(movie->curMedia, desc);

    QTSampleDescriptionFlip(desc->desc, movie->curMedia->mediaType, true);

	return 0;
}

QTErr qtTimeToSampleAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	QTTimeToSample timeToSamples, tts;
	UInt32 count = (UInt32)((size - 8) / sizeof(QTTimeToSampleRecord)), i;

	err = QTLoadData(movie, offset + 8, size - 8, &timeToSamples);
	if (err) return err;

	for (i=0, tts=timeToSamples; i<count; i++, tts++) {
		tts->count = QTReaderToNative32(tts->count);
		tts->duration = QTReaderToNative32(tts->duration);
	}

	movie->curMedia->timeToSamples = timeToSamples;
	movie->curMedia->timeToSampleCount = count;

	return 0;
}

QTErr qtSyncSampleAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt32 *syncSamples, *ss;
	UInt32 count = (UInt32)((size - 8) / 4), i;
	if (0 == count) return 0;

	err = QTLoadData(movie, offset + 8, size - 8, &syncSamples);
	if (err) return err;

	for (i=0, ss=syncSamples; i<count; i++,ss++)
		*ss = QTReaderToNative32(*ss);

	movie->curMedia->syncSamples = syncSamples;
	movie->curMedia->syncSampleCount = count;

	return 0;
}

QTErr qtSampleToChunkAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	QTSampleToChunk sampleToChunks, stc;
	UInt32 count = (UInt32)((size - 8) / sizeof(QTSampleToChunkRecord)), i;

	err = QTLoadData(movie, offset + 8, size - 8, &sampleToChunks);
	if (err) return err;

	for (i=0, stc=sampleToChunks; i<count; i++, stc++) {
		stc->firstChunk = QTReaderToNative32(stc->firstChunk);
		stc->samplesPerChunk = QTReaderToNative32(stc->samplesPerChunk);
		stc->sampleDescriptionIndex = QTReaderToNative32(stc->sampleDescriptionIndex);
	}

	movie->curMedia->sampleToChunks = sampleToChunks;
	movie->curMedia->sampleToChunkCount = count;

	return 0;
}

QTErr qtSampleSizeAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt32 *sampleSizes, *ss;
	UInt32 count = (UInt32)((size - 12) / 4), i;
	UInt32 d[3];
#if QT_READER_STZ2
	UInt32 maxSize = 0, sampleSizeBits = 32;
#endif /* QT_READER_STZ2 */

	err = (movie->reader)(movie->readerRefCon, d, offset, sizeof(UInt32) * 3);
	if (err) return err;

	movie->curMedia->sampleCount = QTReaderToNative32(d[2]);

	if (0 == count) {
		count = 1;

		err = QTLoadData(movie, offset + 4, 4, &sampleSizes);
	}
	else
		err = QTLoadData(movie, offset + 12, size - 12, &sampleSizes);

	if (err) return err;

	for (i=0, ss=sampleSizes; i<count; i++, ss++) {
		*ss = QTReaderToNative32(*ss);
#if QT_READER_STZ2
		if (*ss > maxSize)
			maxSize = *ss;
#endif /* QT_READER_STZ2 */
	}

#if QT_READER_STZ2
	if (maxSize <= 0xffff) {
		if (maxSize <= 0xff) {
			unsigned char *ss8;

			if (0 != qtMovieMalloc(movie, count, &ss8))
				goto done;

			for (i=0, ss=sampleSizes; i<count; i++)
				ss8[i] = (unsigned char)*ss++;

			qtMovieFree(movie, sampleSizes);
			sampleSizes = (UInt32 *)ss8;
			sampleSizeBits = 8;
		}
		else {
			UInt16 *ss16;

			if (0 != qtMovieMalloc(movie, count * 2, &ss16))
				goto done;

			for (i=0, ss=sampleSizes; i<count; i++)
				ss16[i] = (UInt16)*ss++;

			qtMovieFree(movie, sampleSizes);
			sampleSizes = (UInt32 *)ss16;
			sampleSizeBits = 16;
		}
	}
done:
	movie->curMedia->sampleSizeBits = sampleSizeBits;
#endif /* QT_READER_STZ2 */

	movie->curMedia->sampleSizes = sampleSizes;
	movie->curMedia->sampleSizeCount = count;

	return 0;
}

#if QT_READER_STZ2
QTErr qtSampleSize2Atom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt32 *sampleSizes;
	UInt32 count = (UInt32)((size - 12) / 4), i;
	UInt32 d[3];
//	UInt32 sampleSizeBits = 32;

	err = (movie->reader)(movie->readerRefCon, d, offset, sizeof(UInt32) * 3);
	if (err) return err;

	movie->curMedia->sampleSizeBits = (UInt8)d[1];
	movie->curMedia->sampleCount = QTReaderToNative32(d[2]);

	if (0 == count) {
		count = 1;

		err = QTLoadData(movie, offset + 4, (movie->curMedia->sampleSizeBits + 4) >> 3, &sampleSizes);
	}
	else
		err = QTLoadData(movie, offset + 12, size - 12, &sampleSizes);

	if (err) return err;

	if (32 == movie->curMedia->sampleSizeBits) {
		UInt32 *ss = sampleSizes;
		for (i=0; i<count; i++, ss++)
			*ss = QTReaderToNative32(*ss);
	}
	else if (16 == movie->curMedia->sampleSizeBits) {
		UInt16 *ss = (UInt16 *)sampleSizes;
		for (i=0; i<count; i++, ss++)
			*ss = QTReaderToNative16(*ss);
	}
	else if (8 == movie->curMedia->sampleSizeBits)
		;
	else
		;	//@@ 4 bit case

	movie->curMedia->sampleSizes = sampleSizes;
	movie->curMedia->sampleSizeCount = count;

	return 0;
}
#endif /* QT_READER_STZ2 */

QTErr qtChunkOffsetAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt32 *chunkOffsets, *co;
	UInt32 count = (UInt32)((size - 8) / 4), i;

	err = QTLoadData(movie, offset + 8, size - 8, &chunkOffsets);
	if (err) return err;

	for (i=0, co=chunkOffsets; i<count; i++, co++)
		*co = QTReaderToNative32(*co);

	movie->curMedia->chunkOffsets = chunkOffsets;
	movie->curMedia->chunkOffsetCount = count;

	return 0;
}

QTErr qtChunkOffset64Atom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	QTFileOffset *chunkOffsets64, *co;
	UInt32 count = (UInt32)((size - 8) / 8), i;

	err = QTLoadData(movie, offset + 8, size - 8, &chunkOffsets64);
	if (err) return err;

#if QT_READER_FILE64
	for (i=0, co=chunkOffsets64; i<count; i++, co++)
		*co = QTReaderToNative64(*co);

	movie->curMedia->chunkOffsets64 = chunkOffsets64;
	movie->curMedia->chunkOffsetCount = count;
#else
	{
	// if possible, crush 64 bit offset table to 32 bit offset table
	UInt32 *co32, *chunkOffsets32;

	for (i=0, co=chunkOffsets64; i<count; i++, co += 2) {
		unsigned char *p = (unsigned char *)co;
		if ((0 != p[0]) || (0 != p[1]) || (0 != p[2]) || (0 != p[3])) {
			qtMovieFree(movie, chunkOffsets64);
			return -1;
		}
	}

	err = qtMovieMalloc(movie, (size - 8) / 2, &chunkOffsets32);
	if (err) {
		qtMovieFree(movie, chunkOffsets64);
		return err;
	}

	for (i=0, co=chunkOffsets64, co32 = chunkOffsets32; i<count; i++, co += 2, co32++)
		*co32 = QTReaderToNative32(co[1]);
	
	qtMovieFree(movie, chunkOffsets64);
	
	movie->curMedia->chunkOffsets = chunkOffsets32;
	movie->curMedia->chunkOffsetCount = count;
	}
#endif

	return 0;
}

QTErr qtCompositionTimeOffsetAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	QTCompositionTimeOffset compositionTimeOffsets, cto;
	UInt32 count = (UInt32)((size - 8) / 8), i;

	if (count <= 1)
		return 0;			// lots of bad movies in the world with one ctts entry applying a consntant offset to all frames (0xfffffff in some cases!)

	err = QTLoadData(movie, offset + 8, size - 8, &compositionTimeOffsets);
	if (err) return err;

	for (i=0, cto=compositionTimeOffsets; i<count; i++, cto++) {
		cto->count = QTReaderToNative32(cto->count);
		cto->offset = QTReaderToNative32(cto->offset);
	}

	movie->curMedia->compositionTimeOffsets = compositionTimeOffsets;
	movie->curMedia->compositionTimeCount = count;

	return 0;
}


/*
	Track helpers
*/

QTTrack QTMovieGetIndTrack(QTMovie movie, UInt32 index)
{
	QTTrack walker = movie->tracks;

	while (walker && --index)
		walker = walker->next;

	return walker;
}

QTTrack QTMovieGetIndTrackType(QTMovie movie, UInt32 mediaType, UInt32 index)
{
	QTTrack walker = movie->tracks;

	while (walker) {
		if (walker->media->mediaType == mediaType) {
			if (--index == 0)
				return walker;
		}
		walker = walker->next;
	}

	return walker;
}

QTTrack QTMovieGetTrackByID(QTMovie movie, UInt32 id)
{
	QTTrack walker = movie->tracks;

	while (walker) {
		if (walker->id == id)
			return walker;
		walker = walker->next;
	}

	return NULL;
}


void *QTTrackGetIndSampleDescription(QTTrack track, UInt32 index)
{
	void *sampleDescription = NULL;
	QTSampleDescription descWalker = track->media->sampleDescriptions;

	while (descWalker) {
		if (1 == index) {
			sampleDescription = descWalker->desc;
			break;
		}

		index -= 1;
		descWalker = (QTSampleDescription)descWalker->next;
	}

	return sampleDescription;
}

void *QTVideoSampleDescriptionGetExtension(QTImageDescription id, UInt32 extension)
{
	SInt32 size = id->idSize - sizeof(QTImageDescriptionRecord);
	SInt32 *header = (SInt32 *)(id + 1);

	while (size >= 8) {
		UInt32 atomSize = QTReaderMisaligned32_GetN(&header[0]);
		if (atomSize < 8)
			break;

		if (QTReaderMisaligned32_GetN(&header[1]) == extension)
			return header;

		header = (SInt32 *)(atomSize + (char *)header);
		size -= atomSize;
	}

	return NULL;
}

QTErr QTTrackGetIndDataReference(QTTrack track, UInt32 index, UInt32 *kind, UInt32 *flags, void **dataRef, UInt32 *dataRefSize)
{
	QTErr err = -1;
	QTDataReference drWalker = track->media->dataReferences;

	while (drWalker) {
		if (1 == index) {
			if (kind) *kind = drWalker->kind;
			if (flags) *flags = drWalker->flags;
			if (dataRef) *dataRef = drWalker->dataRef;
			if (dataRefSize) *dataRefSize = drWalker->drSize;
			err = 0;
			break;
		}

		index -= 1;
		drWalker = (QTDataReference)drWalker->next;
	}

	return err;
}

void *QTAudioSampleDescriptionGetExtension(QTSoundDescription id, UInt32 extension)
{
	SInt32 offset = sizeof(QTSoundDescriptionRecord) - (id->version ? 0 : 16);
	SInt32 size = id->descSize - offset;
	SInt32 *header = (SInt32 *)(offset + (char *)id);

	while (size >= 8) {
		UInt32 atomSize = QTReaderMisaligned32_GetN(&header[0]);
		if (atomSize < 8)
			break;

		if (QTReaderMisaligned32_GetN(&header[1]) == extension)
			return header;

		header = (SInt32 *)(atomSize + (char *)header);
		size -= atomSize;
	}

	return NULL;
}

void *QTAudioSampleDescriptionGetESDS(QTSoundDescription sd, UInt32 *size)
{
	void *formatInfo = QTAudioSampleDescriptionGetExtension(sd, 'esds');

	*size = 0;

	if (NULL != formatInfo) {
		*size = (((UInt32 *)formatInfo)[0]) - 8;
		formatInfo = 8 + (char *)formatInfo;
	}
	else {
		// could be a quicktime file, so try the wave atom
		formatInfo = QTAudioSampleDescriptionGetExtension(sd, 'wave');
		if (NULL != formatInfo) {
			UInt32 *walk = (UInt32 *)(8 + (char *)formatInfo);
			while (*walk) {
				if (FskEndianU32_BtoN('esds') == walk[1]) {
					*size = (FskEndianU32_BtoN(walk[0])) - 8;
					formatInfo = walk + 2;
					break;
				}
				walk = (UInt32 *)(FskEndianU32_BtoN(walk[0]) + (char *)walk);
			}
			if (0 == *walk)
				formatInfo = NULL;
		}
	}

	return formatInfo;
}

void *QTAudioSampleDescriptionGetALAC(QTSoundDescription sd, UInt32 *size)
{
	void *formatInfo = QTAudioSampleDescriptionGetExtension(sd, 'alac');

	*size = 0;

	if (NULL != formatInfo) {
        int atomsize_4cc_versinflags = 4 + 4 + 4;
		*size = (((UInt32 *)formatInfo)[0]) - atomsize_4cc_versinflags;
		formatInfo = (char *)formatInfo + atomsize_4cc_versinflags;
	}

	return formatInfo;
}

void QTSampleDescriptionFlip(void *desc, UInt32 mediaType, Boolean toNative)
{
	if (kQTVideoType == mediaType)
		qtSampleDescriptionVideoFlip(desc, toNative);
	else if (kQTSoundType == mediaType)
		qtSampleDescriptionSoundFlip(desc, toNative);
	else
		qtSampleDescriptionGenericFlip(desc, toNative);
}

/*
	Time helpers
*/

UInt32 QTMovieToTrackDuration(UInt32 movieDur, QTTrack track)
{
	double temp;

	temp = (double)movieDur * (double)track->media->scale;
	temp /= (double)track->movie->scale;

	return (UInt32)(0.5 + temp);
}

UInt32 QTTrackToMovieDuration(UInt32 trackDur, QTTrack track)
{
	double temp;

	temp = (double)trackDur * (double)track->movie->scale;
	temp /= (double)track->media->scale;

	return (UInt32)(0.5 + temp);
}


#if QT_READER_EDITS

/*
	Edit helpers
*/

QTErr QTTrackMovieTimeToEditIndex(QTTrack track, UInt32 movieTime, UInt32 *index)
{
	UInt32 timeNow = 0;
	QTEdit edit;
	UInt32 i;

	if (NULL == track->edits) {
		// no edit list, so apply default rule: one edit using all media
		*index = 0;
		return (movieTime < track->duration) ? 0 : -1;
	}

	for (i = 0, edit = track->edits; i < track->editsCount; i++, edit++) {
		timeNow += edit->trackDuration;
		if (movieTime < timeNow) {
			*index = i;
			return 0;
		}
	}

	return -1;
}

QTErr QTTrackGetEditInfo(QTTrack track, UInt32 index, UInt32 *movieStartTime, SInt32 *trackStartTime, SInt32 *rate, UInt32 *movieDuration)
{
	UInt32 timeNow = 0;
	QTEdit edit = track->edits;
	UInt32 i;

	if (NULL == track->edits) {
		// no edit list, so apply default rule: one edit using all media
		if (0 != index)
			return -1;

		if (movieStartTime) *movieStartTime = 0;
		if (trackStartTime) *trackStartTime = 0;
		if (rate) *rate = 0x010000;
		if (movieDuration) *movieDuration = track->duration;
		return 0;
	}

	if (index >= track->editsCount)
		return -1;

	for (i = index; i--; edit++)
		timeNow += edit->trackDuration;

	if (movieStartTime) *movieStartTime = timeNow;
	if (trackStartTime) *trackStartTime = edit->mediaTime;
	if (rate) *rate = edit->rate;
	if (movieDuration) *movieDuration = edit->trackDuration;

	return 0;
}

#endif /* QT_READER_EDITS */

#if QT_READER_TRACKREF

QTTrack QTTrackGetReference(QTTrack fromTrack, UInt32 referenceType, UInt32 referenceIndex)
{
	SInt32 size = (SInt32)fromTrack->trackRefSize;
	UInt32 *tp = fromTrack->trackRefs;

	referenceIndex -= 1;		// indices start at 1

	while (size > 0) {
		if (referenceType == QTReaderMisaligned32_GetBtoN(&tp[1])) {
			UInt32 referenceCount = (QTReaderMisaligned32_GetBtoN(&tp[0]) - 8) / 4;
			if (referenceIndex <= referenceCount)
				return QTMovieGetTrackByID(fromTrack->movie, QTReaderMisaligned32_GetBtoN(&tp[2 + referenceIndex]));
		}

		tp = (UInt32 *)(QTReaderMisaligned32_GetBtoN(&tp[0]) + (unsigned char *)tp);
	}

	return NULL;
}

#endif /* QT_READER_TRACKREF */

/*
	Sample table helpers
*/

QTErr QTTrackTimeToSample(QTTrack track, UInt32 atMediaTime, UInt32 *sampleNumberOut, UInt32 *mediaTimeOut, UInt32 *sampleDurationOut)
{
	UInt32 sampleNumber = 1;
	UInt32 mediaTime = 0;
	QTMedia media = track->media;
	UInt32 i = media->timeToSampleCount;
	QTTimeToSample tts = media->timeToSamples;

	if (1 == i) {
		if (0 == tts->duration)
			return -1;

		*sampleNumberOut = 1 + (atMediaTime / tts->duration);
		if (*sampleNumberOut > media->sampleCount)
			return -1;
		if (mediaTimeOut) *mediaTimeOut = ((atMediaTime - mediaTime) / tts->duration) * tts->duration;
		if (sampleDurationOut) *sampleDurationOut = tts->duration;
		return 0;
	}

	// check cache from last lookup
	if (media->ttsCacheSampleNumber && (media->ttsCacheMediaTime <= atMediaTime)) {
		i -= media->ttsCacheIndex;
		tts += media->ttsCacheIndex;
		mediaTime = media->ttsCacheMediaTime;
		sampleNumber = media->ttsCacheSampleNumber;
	}

	while (i--) {
		UInt32 timeIncrement = tts->count * tts->duration;

		if ((mediaTime <= atMediaTime) && (atMediaTime < (mediaTime + timeIncrement))) {
			if (0 == tts->duration)
				return -1;

			*sampleNumberOut = sampleNumber + ((atMediaTime - mediaTime) / tts->duration);
			if (mediaTimeOut) *mediaTimeOut = mediaTime + ((atMediaTime - mediaTime) / tts->duration) * tts->duration;
			if (sampleDurationOut) *sampleDurationOut = tts->duration;

			// save cache position
			media->ttsCacheIndex = (UInt32)(tts - media->timeToSamples);
			media->ttsCacheMediaTime = mediaTime;
			media->ttsCacheSampleNumber = sampleNumber;

			return 0;
		}

		sampleNumber += tts->count;
		mediaTime += timeIncrement;

		tts += 1;
	}

	return -1;
}

QTErr QTTrackSampleToTime(QTTrack track, UInt32 sampleNumber, UInt32 *mediaTimeOut)
{
	UInt32 sampleNumberWalker = 1;
	UInt32 mediaTime = 0;
	QTMedia media = track->media;
	UInt32 i = media->timeToSampleCount;
	QTTimeToSample tts = media->timeToSamples;

	if (sampleNumber > media->sampleCount)
		return -1;

	if (1 == i) {
		*mediaTimeOut = (sampleNumber - 1) * tts->duration;
		return 0;
	}

	while (i--) {
		if ((sampleNumberWalker <= sampleNumber) && (sampleNumber < (sampleNumberWalker + tts->count))) {
			*mediaTimeOut = mediaTime + (sampleNumber - sampleNumberWalker) * tts->duration;
			return 0;
		}

		sampleNumberWalker += tts->count;
		mediaTime += (tts->count * tts->duration);

		tts += 1;
	}

	return -1;
}

QTErr QTTrackSampleToSyncSamples(QTTrack track, UInt32 sampleNumber, UInt32 *prevSyncSampleNumberOut, UInt32 *nextSyncSampleNumberOut)
{
	QTMedia media = track->media;
	UInt32 *syncSamples = media->syncSamples;
	UInt32 i = media->syncSampleCount;
	UInt32 lastSync = 1;

	if (0 == i) {
		if (prevSyncSampleNumberOut) *prevSyncSampleNumberOut = sampleNumber;
		if (nextSyncSampleNumberOut) *nextSyncSampleNumberOut = sampleNumber + 1;
		return 0;
	}

	// check the cache
	if (media->ssCacheSampleNumber && (media->ssCacheSampleNumber <= sampleNumber)) {
		lastSync = media->ssCacheSampleNumber;
		i -= media->ssCacheIndex;
		syncSamples += media->ssCacheIndex;
	}

	while (i--) {
		if ((lastSync <= sampleNumber) && (sampleNumber < *syncSamples)) {
			if (prevSyncSampleNumberOut) *prevSyncSampleNumberOut = lastSync;
			if (nextSyncSampleNumberOut) {
				if (0 == i)
					*nextSyncSampleNumberOut = 0;
				else
					*nextSyncSampleNumberOut = *syncSamples;
			}
			
			// cache result
			media->ssCacheSampleNumber = lastSync;
			media->ssCacheIndex = (UInt32)(syncSamples - media->syncSamples);

			return 0;
		}

		lastSync = *syncSamples++;
	}


	// fell off the end - the answer must be the last sync sample
	if (prevSyncSampleNumberOut) *prevSyncSampleNumberOut = *(media->syncSamples + media->syncSampleCount - 1);
	if (nextSyncSampleNumberOut) *nextSyncSampleNumberOut = 0;

	return 0;
}

QTErr QTTrackTimeToSyncSamples(QTTrack track, UInt32 atMediaTime, UInt32 *prevSyncSampleNumberOut, UInt32 *nextSyncSampleNumberOut)
{
	QTErr err;
	UInt32 sampleNumber;

	err = QTTrackTimeToSample(track, atMediaTime, &sampleNumber, NULL, NULL);
	if (err) return err;

	return QTTrackSampleToSyncSamples(track, sampleNumber, prevSyncSampleNumberOut, nextSyncSampleNumberOut);
}

// this function is basically the same as  QTTrackTimeToSample - just uses a different key (sampleNumber vs mediaTime)
QTErr QTTrackGetSampleTemporalInfo(QTTrack track, UInt32 sampleNumber, UInt32 *mediaTimeOut, UInt32 *compositionTimeOffsetOut, UInt32 *durationOut)
{
	UInt32 sampleNumberWalker = 1;
	UInt32 mediaTime = 0;
	QTMedia media = track->media;
	UInt32 i = media->timeToSampleCount;
	QTTimeToSample tts = media->timeToSamples;

	// check cache from last lookup
	if (media->ttsCacheSampleNumber && (media->ttsCacheSampleNumber <= sampleNumber)) {
		i -= media->ttsCacheIndex;
		tts += media->ttsCacheIndex;
		mediaTime = media->ttsCacheMediaTime;
		sampleNumberWalker = media->ttsCacheSampleNumber;
	}

	while (i--) {
		if ((sampleNumberWalker <= sampleNumber) && (sampleNumber < (sampleNumberWalker + tts->count))) {
			*mediaTimeOut = mediaTime + ((sampleNumber - sampleNumberWalker) * tts->duration);
			*durationOut = tts->duration;
			if (compositionTimeOffsetOut) {
				if (0 == media->compositionTimeCount)
					*compositionTimeOffsetOut = 0;
				else {
					//@@ need to implement cache here to avoid linear search each time
					QTCompositionTimeOffset cto = media->compositionTimeOffsets;
					UInt32 snw = 1, j;
					for (j=0; j<media->compositionTimeCount; j++, cto++) {
						if ((snw <= sampleNumber) && (sampleNumber < (snw + cto->count))) {
							*compositionTimeOffsetOut = cto->offset;
							break;
						}
						snw += cto->count;
					}
				}
			}

			// save cache position
			media->ttsCacheIndex = (UInt32)(tts - media->timeToSamples);
			media->ttsCacheMediaTime = mediaTime;
			media->ttsCacheSampleNumber = sampleNumberWalker;

			return 0;
		}

		sampleNumberWalker += tts->count;
		mediaTime += tts->count * tts->duration;

		tts += 1;
	}

	return -1;
}

QTErr QTTrackSampleToChunk(QTTrack track, UInt32 sampleNumber, UInt32 *chunkNumber, QTFileOffset *chunkOffset, UInt32 *chunkFirstSample, QTSampleToChunk stcOut)
{
	QTMedia media = track->media;
	UInt32 i = media->sampleToChunkCount;
	QTSampleToChunk stc = media->sampleToChunks;
	UInt32 sampleWalker = 1;

	// see if we can accelerate this search by starting from previous point
	if (media->stcCacheSampleNumber && (media->stcCacheSampleNumber <= sampleNumber)) {
		i -= media->stcCacheIndex;
		stc += media->stcCacheIndex;
		sampleWalker = media->stcCacheSampleNumber;
	}

	while (i--) {
		UInt32 nextChunkNum, entrySamples;

		if (0 == i)
			nextChunkNum = media->chunkOffsetCount + 1;
		else
			nextChunkNum = stc[1].firstChunk;

		entrySamples = stc->samplesPerChunk * (nextChunkNum - stc->firstChunk);

		if ((sampleWalker <= sampleNumber) && (sampleNumber < (sampleWalker + entrySamples))) {
			UInt32 chunkNum = stc->firstChunk + ((sampleNumber - sampleWalker) / stc->samplesPerChunk);

			if (chunkNumber)
				*chunkNumber = chunkNum;
			if (chunkFirstSample)
				*chunkFirstSample = sampleWalker + ((chunkNum - stc->firstChunk) * stc->samplesPerChunk);
			if (chunkOffset) {
#if !QT_READER_FILE64
				*chunkOffset = media->chunkOffsets[chunkNum - 1];
#else /* QT_READER_FILE64 */
				*chunkOffset = media->chunkOffsets64 ? media->chunkOffsets64[chunkNum - 1] : media->chunkOffsets[chunkNum - 1];
#endif /* QT_READER_FILE64 */
			}
			if (stcOut)
				*stcOut = *stc;

			// remember this entry in our cache
			media->stcCacheIndex = (UInt32)(stc - media->sampleToChunks);
			media->stcCacheSampleNumber = sampleWalker;

			return 0;
		}

		sampleWalker += entrySamples;
		stc += 1;
	}

	return -1;
}

QTErr QTTrackGetChunkInfo(QTTrack track, UInt32 chunkNumber, QTFileOffset *chunkOffset, UInt32 *chunkSize32)
{
	QTErr err = 0;
	QTMedia media = track->media;

	if ((0 == chunkNumber) || (chunkNumber > media->chunkOffsetCount))
		return -1;

	if (chunkOffset) {
#if !QT_READER_FILE64
		*chunkOffset = media->chunkOffsets[chunkNumber - 1];
#else /* QT_READER_FILE64 */
		*chunkOffset = media->chunkOffsets64 ? media->chunkOffsets64[chunkNumber - 1] : media->chunkOffsets[chunkNumber - 1];
#endif /* QT_READER_FILE64 */
	}
	if (chunkSize32) {
		// determine the starting sample number of this chunk
		QTSampleToChunk stc = media->sampleToChunks;
		UInt32 i = media->sampleToChunkCount;
		UInt32 sampleWalker = 1;

		while (i--) {
			UInt32 nextChunkNum;

			if (0 == i)
				nextChunkNum = media->chunkOffsetCount + 1;
			else
				nextChunkNum = stc[1].firstChunk;

			if ((stc->firstChunk <= chunkNumber) && (chunkNumber < nextChunkNum)) {
				QTFileOffset chunkSize;
				UInt32 sampleNumber = sampleWalker + ((chunkNumber - stc->firstChunk) * stc->samplesPerChunk);

				err = QTTrackGetSampleSizes(track, sampleNumber, stc->samplesPerChunk, &chunkSize);
				if (err) goto bail;

				*chunkSize32 = (UInt32)chunkSize;
				goto bail;
			}

			sampleWalker += stc->samplesPerChunk * (nextChunkNum - stc->firstChunk);
			stc += 1;
		}

		err = -1;
	}

bail:
	return err;
}

QTErr QTTrackGetSampleSizes(QTTrack track, UInt32 sampleNumber, UInt32 sampleCount, QTFileOffset *size)
{
	QTErr err = 0;
	QTMedia media = track->media;

	if (((sampleNumber - 1 + sampleCount) > media->sampleSizeCount) && (1 != media->sampleSizeCount))
		err = -1;
	else if ((kQTSoundType == track->media->mediaType) && (1 == media->sampleSizeCount) && (1 == qtReaderGetSampleSize(media, 0))) {
		// special case for quicktime
		QTSoundDescription ssd = (QTSoundDescription)media->sampleDescriptions->desc;
		if (ssd->version >= 1)
			*size = (sampleCount / ssd->samplesPerPacket) * ssd->bytesPerFrame;
		else
			*size = sampleCount * ((ssd->numChannels * ssd->sampleSize) >> 3);
	}
	else if (1 == media->sampleSizeCount)
		*size = sampleCount * qtReaderGetSampleSize(media, 0);
	else if (1 == sampleCount)
		*size = qtReaderGetSampleSize(media, sampleNumber - 1);
	else {
		UInt32 accumulateSize = 0;
#if QT_READER_STZ2
		if (32 == media->sampleSizeBits) {
#endif /* QT_READER_STZ2 */
			UInt32 *ssp = &media->sampleSizes[sampleNumber - 1];

			while (sampleCount--)
				accumulateSize += *ssp++;
#if QT_READER_STZ2
		}
		else
		if (16 == media->sampleSizeBits) {
			UInt16 *ssp = &((UInt16 *)media->sampleSizes)[sampleNumber - 1];

			while (sampleCount--)
				accumulateSize += *ssp++;
		}
		else
		if (8 == media->sampleSizeBits) {
			UInt8 *ssp = &((UInt8 *)media->sampleSizes)[sampleNumber - 1];

			while (sampleCount--)
				accumulateSize += *ssp++;
		}
		else
			; //@@ 4 bit case
#endif /* QT_READER_STZ2 */

		*size = accumulateSize;
	}

	return err;
}

QTErr QTTrackGetChunkSamplesOfSameSize(QTTrack track, UInt32 sampleNumber, UInt32 *count)
{
	QTErr err = 0;
	QTMedia media = track->media;
	QTSampleToChunkRecord stc;
	UInt32 chunkNumber, firstChunkSample;

	*count = 0;
	if ((sampleNumber > media->sampleSizeCount) && (1 != media->sampleSizeCount)) {
		err = -1;
		goto bail;
	}

	err = QTTrackSampleToChunk(track, sampleNumber, &chunkNumber, NULL, &firstChunkSample, &stc);
	if (err) goto bail;

	stc.samplesPerChunk -= (sampleNumber - firstChunkSample);

	if (((kQTSoundType == track->media->mediaType) && (media->sampleSizeCount < 2) && (1 == qtReaderGetSampleSize(media, 0))) ||
		(1 == media->sampleSizeCount))
		*count = stc.samplesPerChunk;
	else {
		UInt32 index = sampleNumber - 1;
		UInt32 size = qtReaderGetSampleSize(media, index);

		while (stc.samplesPerChunk-- && (size == qtReaderGetSampleSize(media, index++)))
			*count += 1;
	}

bail:
	return err;
}

QTErr QTTrackLoadSample(QTTrack track, UInt32 sampleNumber, void **dataOut, UInt32 *dataSize)
{
	QTErr err;
	char *data = NULL;
	QTFileOffset chunkOffset, sampleOffset, sampleSize;
	UInt32 chunkNumber, firstChunkSample;
	QTDataReference dataRef;

	err = QTTrackSampleToChunk(track, sampleNumber, &chunkNumber, &chunkOffset, &firstChunkSample, NULL);
	if (err) goto bail;

	if (firstChunkSample == sampleNumber)
		sampleOffset = chunkOffset;
	else {
		QTFileOffset offset;

		err = QTTrackGetSampleSizes(track, firstChunkSample, sampleNumber - firstChunkSample, &offset);
		if (err) goto bail;

		sampleOffset = chunkOffset + (UInt32)offset;
	}

	err = QTTrackGetSampleSizes(track, sampleNumber, 1, &sampleSize);
	if (err) goto bail;

	err = qtMovieMalloc(track->movie, (UInt32)sampleSize + kFskDecompressorSlop, &data);
	if (err) goto bail;

	dataRef = track->media->dataReferences;
	if ('hndl' == dataRef->kind) {
		unsigned char *drP = (unsigned char *)(dataRef->dataRef + 4);
		SInt32 drSize = dataRef->drSize - 4;
		
		if (drSize > 0) {
			Boolean foundIt = false;
			SInt32 strLen = *drP + 1;
			drP += strLen;
			drSize -= strLen;		// skip file name
			while ((drSize >= 8) && !foundIt) {
				UInt32 atomSize = QTReaderMisaligned32_GetBtoN(drP);
				UInt32 atomType = QTReaderMisaligned32_GetBtoN((drP + 4));
				
				if ('data' == atomType) {
					if ((atomSize - 8) < (sampleOffset + sampleSize)) {
						err = -1;
						goto bail;
					}

					qtMemMove(data, drP + sampleOffset + 8, (UInt32)sampleSize);
					foundIt = true;
				}
				drSize -= atomSize;
				drP += atomSize;
			}

			if (!foundIt) {
				err = -1;
				goto bail;
			}
		}		
	}
	else if (1 & dataRef->flags) {
		err = (track->movie->reader)(track->movie->readerRefCon, data, sampleOffset, (UInt32)sampleSize);
		if (err) goto bail;
	}
	else {
		err = kFskErrUnimplemented;
		goto bail;
	}

	*dataSize = (UInt32)sampleSize;

bail:
	if (err) {
		qtMovieFree(track->movie, data);
		data = NULL;
	}

	*dataOut = data;

	return err;
}

/*
	UserData
*/

QTErr QTUserDataGet(QTUserData userData, UInt32 udType, UInt32 udIndex, void **ud, UInt32 *udSize)
{
	QTErr err = 0;

	*ud = NULL;
	if (udSize) *udSize = 0;

	if (userData) {
		SInt32 *p = (SInt32 *)(void*)&userData->data;
		unsigned char *end = userData->size + (unsigned char *)p;

		while (((8 + (unsigned char *)p) < end) && (QTReaderMisaligned32_GetBtoN(p) >= 8)) {
			UInt32 atomSize = QTReaderMisaligned32_GetBtoN(p);
			if (udType == QTReaderMisaligned32_GetBtoN(p+1)) {
				if (--udIndex == 0) {
					atomSize -= 8;

					if ((atomSize + (unsigned char *)&p[2]) > end) {
						err = -1;
						goto bail;
					}

					err = qtMovieMalloc(userData->movie, atomSize, ud);
					if (err) goto bail;

					if (udSize) *udSize = atomSize;

					qtMemMove(*ud, &p[2], atomSize);
					goto bail;
				}
			}

			p = (SInt32 *)(atomSize + (char *)p);
		}
	}

	err = -1;		// not found

bail:
	return err;
}

QTErr QTUserDataGetText(QTUserData userData, UInt32 udType, void **text)
{
	QTErr err = 0;
	void *ud = NULL;
	UInt32 udSize;

	err = QTUserDataGet(userData, udType, 1, &ud, &udSize);
	if (err) goto bail;

	if (udSize > 4) {
		char *i = 4 + (char *)ud, *o;
		UInt16 textSize = QTReaderToNative16(*(UInt16 *)ud);
		err = qtMovieCalloc(userData->movie, textSize + 1, text);
		if (err) goto bail;

		o = (char *)*text;
		while (textSize--)
			*o++ = *i++;
	}

bail:
	qtMovieFree(userData->movie, ud);

	return err;
}

QTErr QTUserDataGetTextMP4(QTUserData userData, UInt32 udType, void **text)
{
	QTErr err = 0;
	unsigned char *ud;
	UInt32 udSize;
	SInt32 i;

	err = QTUserDataGet(userData, udType, 1, (void **)(void *)&ud, &udSize);
	if (err) goto bail;

	if (udSize < 6)
		return -1;

	// strip version & language code
	for (i=0, udSize -=6; i<(SInt32)udSize; i++)
		ud[i] = ud[i + 6];
	ud[udSize] = 0;

	*text = ud;

bail:
	return err;
}

typedef struct {
	UInt32				tag;
	UInt32				type;
	UInt32				unknown;
	UInt32				dataSize;
	unsigned char		*data;
	Boolean				skip;
} iTunesMetaRecord, *iTunesMeta; 

static QTErr iTunesMetaParse(QTUserData ud, unsigned char *metaAtom, UInt32 metaAtomSize, iTunesMeta *itmOut, UInt32 *itmCountOut);
static QTErr QTUserDataGetiTunes_Priv(QTUserData userData, UInt32 udType, Boolean isText, void **userPtr, UInt32 *userPtrSize);

QTErr iTunesMetaParse(QTUserData ud, unsigned char *metaAtom, UInt32 metaAtomSize, iTunesMeta *itmOut, UInt32 *itmCountOut)
{
	QTErr err = 0;
	iTunesMeta itm = NULL;
	UInt32 itmCount = 0;
	UInt32 itmSize = 0, itmSlopSize = 0;
	Boolean validatedHandler = false;

	// skip flags
	metaAtomSize -= 4;
	metaAtom += 4;

	while (metaAtomSize) {
		UInt32 atomSize = QTReaderMisaligned32_GetBtoN(metaAtom);
		UInt32 atomType = QTReaderMisaligned32_GetBtoN((metaAtom + 4));

		if ((atomSize < 8) || (atomSize > metaAtomSize)) {
			err = -1;
			goto bail;
		}

		if (('hdlr' == atomType) && (atomSize >= 0x18)) {
			UInt32 aLong = QTReaderMisaligned32_GetBtoN((metaAtom + 16));
			UInt32 bLong = QTReaderMisaligned32_GetBtoN((metaAtom + 20));
			validatedHandler = ('mdir' == aLong) && ('appl' == bLong);
		}

		if (('ilst' == atomType) && validatedHandler) {
			UInt32 ilstAtomSize = atomSize - 8;
			unsigned char *ilstAtom = metaAtom + 8;

			while (ilstAtomSize) {
				UInt32 itemSize = QTReaderMisaligned32_GetBtoN(ilstAtom);
				UInt32 itemType = QTReaderMisaligned32_GetBtoN((ilstAtom + 4));

				if ((itemSize < 8) || (itemSize > metaAtomSize)) {
					err = -1;
					goto bail;
				}

				err = qtMovieRealloc(ud->movie, &itm, itmSize, itmSize + sizeof(iTunesMetaRecord), &itmSlopSize);
				if (err) goto bail;

				itm[itmCount].tag = itemType;
				itm[itmCount].type = QTReaderMisaligned32_GetBtoN((ilstAtom + 16));
				itm[itmCount].unknown = QTReaderMisaligned32_GetBtoN((ilstAtom + 20));
				itm[itmCount].dataSize = itemSize - 24;
				itm[itmCount].data = ilstAtom + 24;
				itm[itmCount].skip = false;
				itmCount += 1;
				itmSize += sizeof(iTunesMetaRecord);

				ilstAtomSize -= itemSize;
				ilstAtom += itemSize;
			}
			break;
		}

		metaAtom += atomSize;
		metaAtomSize -= atomSize;
	}

bail:
	if ((0 != err) && (NULL != itm)) {
		qtMovieFree(ud->movie, itm);
		itm = NULL;
		itmCount = 0;
	}
	*itmOut = itm;
	*itmCountOut = itmCount;

	return err;
}

QTErr QTUserDataGetiTunes_Priv(QTUserData userData, UInt32 udType, Boolean isText, void **userPtr, UInt32 *userPtrSize)
{
	QTErr err;
	unsigned char *metaAtom = NULL;
	UInt32 metaAtomSize;
	iTunesMeta itm = NULL;
	UInt32 itmCount = 0, i;

	*userPtr = NULL;
	if (userPtrSize) *userPtrSize = 0;

	err = QTUserDataGet(userData, 'meta', 1, (void **)(void *)&metaAtom, &metaAtomSize);
	if (err) goto bail;

	iTunesMetaParse(userData, metaAtom, metaAtomSize, &itm, &itmCount);
	for (i=0; i<itmCount; i++) {
		if (itm[i].tag != udType)
			continue;

		err = qtMovieMalloc(userData->movie, itm[i].dataSize + (isText ? 1 : 0), userPtr);
		if (err) goto bail;

		qtMemMove(*userPtr, itm[i].data, itm[i].dataSize + (isText ? 1 : 0));
		if (isText)
			(*(char **)userPtr)[itm[i].dataSize] = 0;

		if (userPtrSize) *userPtrSize = itm[i].dataSize + (isText ? 1 : 0);
		goto bail;
	}

	err = -1;

bail:
	qtMovieFree(userData->movie, metaAtom);
	qtMovieFree(userData->movie, itm);
	return err;
}

QTErr QTUserDataGetiTunes(QTUserData userData, UInt32 udType, void **userPtr, UInt32 *userPtrSize)
{
	return QTUserDataGetiTunes_Priv(userData, udType, false, userPtr, userPtrSize);
}

QTErr QTUserDataGetTextiTunes(QTUserData userData, UInt32 udType, void **text)
{
	return QTUserDataGetiTunes_Priv(userData, udType, true, text, NULL);
}

#if QT_READER_AUTHORING

QTErr QTUserDataSetiTunesData(QTUserData userData, UInt32 udType, UInt32 iTunesType, const void *data, UInt32 dataSize)
{
	QTErr err = 0;
	QTMovie movie = userData->movie;
	unsigned char *metaAtom = NULL;
	UInt32 metaAtomSize;
	iTunesMeta itm = NULL;
	UInt32 itmCount = 0;
	UInt32 itmSize = 0;
	UInt32 i;
	unsigned char *result = NULL, *p8;
	UInt32 *p32;
	UInt32 resultSize;

	// parse what is there now
	QTUserDataGet(userData, 'meta', 1, (void **)(void *)&metaAtom, &metaAtomSize);
	if (metaAtom)
		iTunesMetaParse(userData, metaAtom, metaAtomSize, &itm, &itmCount);

	// remove anything that intersects with what we are adding
	for (i=0; i<itmCount; i++) {
		if (itm[i].tag == udType)
			itm[i].skip = true;
	}

	// add the new one
	err = qtMovieRealloc(movie, &itm, itmCount * sizeof(iTunesMetaRecord), (1 + itmCount) * sizeof(iTunesMetaRecord), NULL);
	if (err) goto bail;

	itm[itmCount].tag = udType;
	itm[itmCount].type = iTunesType;
	itm[itmCount].unknown = 0;
	itm[itmCount].dataSize = dataSize;
	itm[itmCount].data = (unsigned char *)data;
	itm[itmCount].skip = false;
	itmCount += 1;

	// calculate total size
	for (i=0; i<itmCount; i++) {
		if (!itm[i].skip)
			itmSize += 24 + itm[i].dataSize;
	}

	// put it all together
	resultSize = 0x002e + itmSize;
	err = qtMovieMalloc(movie, resultSize, &result);
	if (err) goto bail;

	p32 = (UInt32 *)result;
	*p32++ = 0;
	*p32++ = QTReaderToBigEndian32(0x0022);
	*p32++ = QTReaderToBigEndian32('hdlr');
	*p32++ = 0;
	*p32++ = 0;
	*p32++ = QTReaderToBigEndian32('mdir');
	*p32++ = QTReaderToBigEndian32('appl');
	*p32++ = 0;
	*p32++ = 0;
	p8 = (UInt8 *)p32;
	*p8++ = 0;
	*p8++ = 0xa9;
	p32 = (UInt32 *)p8;
	itmSize += 8;
	*p32++ = QTReaderToBigEndian32(itmSize);
	*p32++ = QTReaderToBigEndian32('ilst');

	for (i=0; i<itmCount; i++) {
		UInt32 size = itm[i].dataSize + 24;

		if (itm[i].skip)
			continue;

		*p32++ = QTReaderToBigEndian32(size);
		*p32++ = QTReaderToBigEndian32(itm[i].tag);
		size -= 8;
		*p32++ = QTReaderToBigEndian32(size);
		*p32++ = QTReaderToBigEndian32('data');
		*p32++ = QTReaderToBigEndian32(itm[i].type);
		*p32++ = QTReaderToBigEndian32(itm[i].unknown);
		qtMemMove(p32, itm[i].data, itm[i].dataSize);
		p32 = (UInt32 *)(itm[i].dataSize + (UInt8 *)p32);
	}

	QTUserDataRemove(userData, 'meta', 1);
	QTUserDataAdd(userData, 'meta', result, resultSize);

bail:
	qtMovieFree(movie, metaAtom);
	qtMovieFree(movie, itm);
	qtMovieFree(movie, result);

	return err;
}

#endif

/*
	Sample description flippers
*/

void qtSampleDescriptionGenericFlip(void *desc, Boolean toNative)
{
	// note: this function is only correct for minimal sample descriptions
	QTGenericDescription gd = (QTGenericDescription)desc;
	UNUSED(toNative);

	gd->descSize = QTReaderToNative32(gd->descSize);
	gd->dataFormat = QTReaderToNative32(gd->dataFormat);
	gd->resvd1 = QTReaderToNative32(gd->resvd1);
	gd->resvd2 = QTReaderToNative16(gd->resvd2);
	gd->dataRefIndex = QTReaderToNative16(gd->dataRefIndex);
}

void qtSampleDescriptionVideoFlip(void *desc, Boolean toNative)
{
	QTImageDescription id = (QTImageDescription)desc;
	SInt32 size = id->idSize;

	id->idSize = QTReaderToNative32(size);
	id->cType = QTReaderToNative32(id->cType);
	id->resvd1 = QTReaderToNative32(id->resvd1);
	id->resvd2 = QTReaderToNative16(id->resvd2);
	id->dataRefIndex = QTReaderToNative16(id->dataRefIndex);
	id->version = QTReaderToNative16(id->version);
	id->revisionLevel = QTReaderToNative16(id->revisionLevel);
	id->vendor = QTReaderToNative32(id->vendor);
	id->temporalQuality = QTReaderToNative32(id->temporalQuality);
	id->spatialQuality = QTReaderToNative32(id->spatialQuality);
	id->width = QTReaderToNative16(id->width);
	id->height = QTReaderToNative16(id->height);
	id->hRes = QTReaderToNative32(id->hRes);
	id->vRes = QTReaderToNative32(id->vRes);
	id->dataSize = QTReaderToNative32(id->dataSize);
	id->frameCount = QTReaderToNative16(id->frameCount);
	id->depth = QTReaderToNative16(id->depth);
	id->clutID = QTReaderToNative16(id->clutID);

	//@@ if clutID is NOT -1, then we need to skip the color table too

	// flip the size/type of any image description extensions
	if (toNative)
		size = id->idSize;
	flipExtensions((SInt32 *)(id + 1), size - sizeof(QTImageDescriptionRecord), toNative);
}

void qtSampleDescriptionSoundFlip(void *desc, Boolean toNative)
{
	QTSoundDescription sd = (QTSoundDescription)desc;
	UInt16 version = sd->version;
	UInt32 size = sd->descSize, baseSize = sizeof(QTSoundDescriptionRecord) - (sizeof(UInt32) * 4);

	sd->descSize = QTReaderToNative32(size);
	sd->dataFormat = QTReaderToNative32(sd->dataFormat);
	sd->resvd1 = QTReaderToNative32(sd->resvd1);
	sd->resvd2 = QTReaderToNative16(sd->resvd2);
	sd->dataRefIndex = QTReaderToNative16(sd->dataRefIndex);
	sd->version = QTReaderToNative16(version);
	sd->revlevel = QTReaderToNative16(sd->revlevel);
	sd->vendor = QTReaderToNative32(sd->vendor);
	sd->numChannels = QTReaderToNative16(sd->numChannels);
	sd->sampleSize = QTReaderToNative16(sd->sampleSize);
	sd->compressionID = QTReaderToNative16(sd->compressionID);
	sd->packetSize = QTReaderToNative16(sd->packetSize);
	sd->sampleRate = QTReaderToNative32(sd->sampleRate);

	if (toNative) {
		version = sd->version;
		size = sd->descSize;
	}

	if (version >= 1) {
		sd->samplesPerPacket = QTReaderToNative32(sd->samplesPerPacket);
		sd->bytesPerPacket = QTReaderToNative32(sd->bytesPerPacket);
		sd->bytesPerFrame = QTReaderToNative32(sd->bytesPerFrame);
		sd->bytesPerSample = QTReaderToNative32(sd->bytesPerSample);
		baseSize += sizeof(UInt32) * 4;
	}

	flipExtensions((SInt32 *)(baseSize + (char *)sd), size - baseSize, toNative);
}

void flipExtensions(SInt32 *header, UInt32 size, Boolean toNative)
{
	while (size >= 8) {
		UInt32 atomSize = 0, h;

		if (false == toNative)
			atomSize = QTReaderMisaligned32_GetN(header);
		h = QTReaderMisaligned32_GetBtoN(header);
		QTReaderMisaligned32_PutN(&h, header);
		if (toNative)
			atomSize = h;

		if ((atomSize < 8) || (atomSize > size))
			break;				// bad data

		h = QTReaderMisaligned32_GetBtoN(header+1);
		QTReaderMisaligned32_PutN(&h, header+1);

		header = (SInt32 *)(atomSize + (char *)header);
		size -= atomSize;
	}
}

/*
	Utilities
*/

QTErr qtMovieRealloc_(QTMovie movie, void **mem, UInt32 oldSize, UInt32 newSize, UInt32 *slopSize)
{
	void *newMem;
	UInt32 copySize;

	if (slopSize && (newSize >= oldSize)) {
		if (*slopSize >= newSize)
			return 0;

		newSize += 2048;				// could try to be more clever here and base the slop on the handle size, but this is a simple start
	}

	qtMovieMalloc(movie, newSize, &newMem);
	if (NULL == newMem)
		return -1;

	if (oldSize <= newSize)
		copySize = oldSize;
	else
		copySize = newSize;
	qtMemMove(newMem, *mem, copySize);

	qtMovieFree(movie, *mem);

	*mem = newMem;

	if (slopSize)
		*slopSize = newSize;

	return 0;
}

UInt32 qtListCount(void *list)
{
	UInt32 count = 0;

	while (list) {
		count += 1;
		list = *(void **)list;
	}

	return count;
}

Boolean QTMovieIsSelfContained(QTMovie movie)
{
	QTTrack track = movie->tracks;

	while (track) {
		if (track->media) {
			QTDataReference drWalker = track->media->dataReferences;
			while (drWalker) {
				if ((!(kDataRefIsSelfContained & drWalker->flags)) && ('hndl' != drWalker->kind))		// we consider Handle data handlers as self-contained because QTTrackLoadSample can deal with it
					return false;
				drWalker = (QTDataReference)drWalker->next;
			}
		}
		track = track->next;
	}

	return true;
}

/*
	Authoring
*/

QTErr QTMovieNew(QTMovie *movieOut, UInt32 scale, QTMovieAllocProc alloc, QTMovieFreeProc free, void *allocRefCon)
{
	QTErr err = 0;
	QTMovie movie = NULL;

	// set up
	err = (alloc)(allocRefCon, true, sizeof(QTMovieRecord), (void **)(void *)&movie);
	if (err) goto bail;

	movie->allocProc = alloc;
	movie->freeProc = free;
	movie->allocRefCon = allocRefCon;
	movie->scale = scale;

#if QT_READER_AUTHORING
	movie->nextTrackID = 1;
	movie->loaded = true;
#endif /* QT_READER_AUTHORING */

bail:
	if (err) {
		QTMovieDispose(movie);
		movie = NULL;
	}

	*movieOut = movie;

	return err;
}

QTErr QTTrackNew(QTTrack *trackOut, QTMovie movie, UInt16 width, UInt16 height, UInt16 volume)
{
	QTErr err = 0;
	QTTrack track;

	err = qtMovieCalloc(movie, sizeof(QTTrackRecord), &track);
	if (err) goto bail;

	track->movie = movie;

	// add to end of track list
	if (NULL == movie->tracks)
		movie->tracks = track;
	else {
		QTTrack tw = movie->tracks;
		while (tw->next)
			tw = tw->next;
		tw->next = track;
	}

	track->width = width;
	track->height = height;
	track->volume = volume;
	track->enabled = true;
#if QT_READER_MATRIX
	initializeMatrix((QTMatrixRecord *)(void*)&track->matrix);
#endif /* QT_READER_MATRIX */

#if QT_READER_AUTHORING
	if (movie->loaded)
		track->id = movie->nextTrackID++;
#endif /* QT_READER_AUTHORING */

bail:
	if (err) {
		QTTrackDispose(track);
		track = NULL;
	}

	*trackOut = track;

	return err;
}

void QTTrackDispose(QTTrack t)
{
	QTMovie movie;
	QTMedia md;

	if (NULL == t)
		return;

	movie = t->movie;
	md = t->media;

	if (t == movie->tracks)
		movie->tracks = t->next;
	else {
		QTTrack walker;
		for (walker = movie->tracks; NULL != walker; walker = walker->next) {
			if (walker->next == t) {
				walker->next = t->next;
				break;
			}
		}
	}

#if QT_READER_MTDT
		while (t->mtdt) {
			QTMTDT next = t->mtdt->next;
			qtMovieFree(movie, t->mtdt);
			t->mtdt = next;
		}
#endif /* QT_READER_MTDT */

#if QT_READER_EXTRA_ATOMS
	while (t->extras) {
		QTExtraAtom next = t->extras->next;
		qtMovieFree(movie, t->extras);
		t->extras = next;
	}
#endif /* QT_READER_EXTRA_ATOMS */

	if (md) {
		while (md->sampleDescriptions) {
			void *next = md->sampleDescriptions->next;
			qtMovieFree(movie, md->sampleDescriptions);
			md->sampleDescriptions = (QTSampleDescription)next;
		}

		while (md->dataReferences) {
			void *next = md->dataReferences->next;
			qtMovieFree(movie, md->dataReferences);
			md->dataReferences = (QTDataReference)next;
		}

		qtMovieFree(movie, md->chunkOffsets);
#if QT_READER_FILE64
		qtMovieFree(movie, md->chunkOffsets64);
#endif /* QT_READER_FILE64 */
		qtMovieFree(movie, md->sampleSizes);
		qtMovieFree(movie, md->syncSamples);
#if QT_READER_STPS
		qtMovieFree(movie, md->partialSyncSamples);
#endif
#if QT_READER_SDTP
		qtMovieFree(movie, md->sampleDependencies);
#endif
		qtMovieFree(movie, md->sampleToChunks);
		qtMovieFree(movie, md->timeToSamples);
		qtMovieFree(movie, md->compositionTimeOffsets);

		qtMovieFree(movie, md);
	}

#if QT_READER_EDITS
	qtMovieFree(movie, t->edits);
#endif /* QT_READER_EDITS */
#if QT_READER_TRACKREF
	qtMovieFree(movie, t->trackRefs);
#endif /* QT_READER_TRACKREF */
	qtMovieFree(movie, t->userData);
	qtMovieFree(movie, t);
}

QTErr QTMediaNew(QTMedia *mediaOut, QTTrack track, UInt32 scale, UInt32 mediaType)
{
	QTMedia media;
	QTErr err;

	err = qtMovieCalloc(track->movie, sizeof(QTMediaRecord), &media);
	if (err) goto bail;

	track->media = media;
	media->track = track;

	media->scale = scale;
	media->mediaType = mediaType;

bail:
	*mediaOut = media;

	return err;
}

void appendSampleDescription(QTMedia media, QTSampleDescription desc)
{
	// add to end of track list
	if (NULL == media->sampleDescriptions)
		media->sampleDescriptions = desc;
	else {
		QTSampleDescription sd = media->sampleDescriptions;
		while (sd->next)
			sd= (QTSampleDescription)sd->next;
		sd->next = desc;
	}
}

QTErr QTMediaAddDataReference(QTMedia media, UInt32 kind, UInt32 flags, void *dr, UInt32 drSize, UInt32 *index)
{
	QTErr err;
	QTMovie movie = media->track->movie;
	QTDataReference dataRef, drWalker = media->dataReferences;

	err = qtMovieCalloc(movie, drSize + sizeof(QTDataReferenceRecord), &dataRef);
	if (err) return err;

	dataRef->flags = flags;
	dataRef->kind = kind;
	dataRef->drSize = drSize;
	if (NULL != dr)
		qtMemMove(&dataRef->dataRef, dr, drSize);

	if (NULL == drWalker)
		media->dataReferences = dataRef;
	else {
		while (drWalker->next)
			drWalker = (QTDataReference)drWalker->next;
		drWalker->next = dataRef;
	}

	if (index)
		*index = qtListCount(media->dataReferences);

	return 0;
}

#if QT_READER_EDITS

QTErr QTTrackSetEditList(QTTrack track, UInt32 count, QTEdit editList)
{
	qtMovieFree(track->movie, track->edits);
	track->edits = editList;
	track->editsCount = count;

	if (track->editsCount) {
		QTEdit edit;
		UInt32 i;
		for (i = count, track->duration = 0, edit = track->edits; i--; edit++)
			track->duration += edit->trackDuration;
	}
	else
		track->duration = (UInt32)(((double)track->media->duration * (double)track->movie->scale) / (double)track->media->scale);

	calculateMovieDuration(track->movie);

	return 0;
}

#endif /* QT_READER_EDITS */

QTErr QTTrackSetEnabled(QTTrack track, Boolean enabled)
{
	track->enabled = enabled;
	calculateMovieDuration(track->movie);
	return 0;
}

void calculateMovieDuration(QTMovie movie)
{
	QTTrack walker = movie->tracks;

	movie->duration = 0;
	while (walker) {
		if (walker->enabled && (walker->duration > movie->duration))
			movie->duration = walker->duration;
		walker = walker->next;
	}
}

#if QT_READER_STZ2
UInt32 qtReaderGetSampleSize(QTMedia media, UInt32 index)
{
	UInt32 sampleSizeBits = media->sampleSizeBits;
	if (32 == sampleSizeBits)
		return media->sampleSizes[index];
	if (16 == sampleSizeBits)
		return ((UInt16 *)(media->sampleSizes))[index];
	if (8 == sampleSizeBits)
		return ((unsigned char *)(media->sampleSizes))[index];
	return 0;		//@@ 4 bit case
}
#endif /* QT_READER_STZ2 */

/*
	Utilities
*/

QTErr QTLoadData_(QTMovie movie, QTFileOffset offset, QTFileOffset size, void **data)
{
	QTErr err;

	err = qtMovieCalloc(movie, (UInt32)size, data);
	if (err) goto bail;

	err = (movie->reader)(movie->readerRefCon, *data, offset, (UInt32)size);
	if (err) {
		qtMovieFree(movie, *data);
		*data = NULL;
		goto bail;
	}

bail:
	return err;
}

#if QT_READER_AUTHORING

/*
	Write
*/

typedef struct {
	QTFileOffset	offset;
	UInt32			type;
} QTAtomWriteStateRecord, *QTAtomWriteState;

static QTErr appendData(QTMovie movie, const void *data, UInt32 dataSize);
static void openAtom(QTMovie movie, QTAtomWriteState atom, UInt32 atomType);
static QTErr closeAtom(QTMovie movie, const QTAtomWriteState atom);
static void initializeMatrixBigEndian(QTMatrixRecord *matrix);

static QTErr qtWriteUserData(QTMovie movie, QTUserData userData);
static QTErr qtWriteTrack(QTMovie movie, QTTrack track);
#if QT_READER_EDITS
	static QTErr qtWriteEditList(QTMovie movie, UInt32 count, QTEdit edits);
#endif /* QT_READER_EDITS */
#if QT_READER_EXTRA_ATOMS
	static QTErr qtWriteExtras(QTMovie movie, QTExtraAtom extras);
#endif /* QT_READER_EXTRA_ATOMS */
#if QT_READER_MTDT
	static QTErr qtWriteMetaData(QTMovie movie, QTMTDT mtdt);
#endif /* QT_READER_MTDT */

static QTErr qtWriteMedia(QTMovie movie, QTMedia media);
static QTErr qtWriteMediaInfo(QTMovie movie, QTMedia media);
static QTErr qtWriteDataInfo(QTMovie movie, QTMedia media);
static QTErr qtWriteSampleTable(QTMovie movie, QTMedia media);

QTErr QTMovieSave(QTMovie movie, QTFileOffset writeOffset, QTMovieWriteProc write, void *writeRefCon)
{
	QTErr err = 0;
	QTAtomWriteStateRecord moovAtom, movieHeaderAtom;
	MovieHeader mh;
	QTTrack walker;

	movie->writer = write;
	movie->writerRefCon = writeRefCon;
	movie->writeOffset = writeOffset;

	openAtom(movie, &moovAtom, MovieAID);

	openAtom(movie, &movieHeaderAtom, MovieHeaderAID);

    mh.flags = QTReaderToBigEndian32(0);
	mh.creationTime = qtGetCurrentTime();
    mh.creationTime = QTReaderToBigEndian32(mh.creationTime);
    mh.modificationTime = mh.creationTime;
    mh.timeScale = QTReaderToBigEndian32(movie->scale);
    mh.duration = QTReaderToBigEndian32(movie->duration);
    mh.preferredRate = QTReaderToBigEndian32(0x00010000);
    mh.preferredVolume = QTReaderToBigEndian16(0x0100);
    mh.reserved1 = 0;
    mh.preferredLong1 = 0;
    mh.preferredLong2 = 0;
	initializeMatrixBigEndian(&mh.matrix);
    mh.previewTime = QTReaderToBigEndian32(0);
    mh.previewDuration = QTReaderToBigEndian32(0);
    mh.posterTime = QTReaderToBigEndian32(0);
    mh.selectionTime = QTReaderToBigEndian32(0);
    mh.selectionDuration = QTReaderToBigEndian32(0);
    mh.currentTime = QTReaderToBigEndian32(0);
    mh.nextTrackID = QTReaderToBigEndian32(movie->nextTrackID);

	err = appendData(movie, &mh, sizeof(mh));
	if (err) goto bail;

	err = closeAtom(movie, &movieHeaderAtom);
	if (err) goto bail;

	for (walker = movie->tracks; NULL != walker; walker = walker->next)
		qtWriteTrack(movie, walker);

	err = qtWriteUserData(movie, movie->userData);
	if (err) goto bail;

#if QT_READER_MTDT
	err = qtWriteMetaData(movie, movie->mtdt);
	if (err) goto bail;
#endif /* QT_READER_MTDT */

#if QT_READER_EXTRA_ATOMS
	err = qtWriteExtras(movie, movie->extras);
	if (err) goto bail;
#endif /* QT_READER_EXTRA_ATOMS */

	err = closeAtom(movie, &moovAtom);
	if (err) goto bail;

bail:
	return err;
}

QTErr QTMediaAddSampleDescription(QTMedia media, void *sampleDescription, UInt32 *index)
{
	QTErr err;
	QTMovie movie = media->track->movie;
	QTSampleDescription desc;
	UInt32 sdSize = *(UInt32 *)sampleDescription;

	err = qtMovieCalloc(movie, sdSize + sizeof(QTSampleDescriptionRecord), &desc);
	if (err) return err;

	qtMemMove(&desc->desc, sampleDescription, sdSize);

	appendSampleDescription(media, desc);

	if (index)
		*index = qtListCount(media->sampleDescriptions);

	return 0;
}

QTErr QTUserDataRemove(QTUserData userData, UInt32 udType, UInt32 udIndex)
{
	SInt32 *p = (SInt32 *)&userData->data;
	SInt32 foundSize = 0;

	while (QTReaderMisaligned32_GetBtoN(p) >= 8) {
		UInt32 atomSize = QTReaderMisaligned32_GetBtoN(p);
		if (foundSize)
			qtMemMove(-foundSize + (UInt8 *)p, p, atomSize);
		else if (udType == QTReaderMisaligned32_GetBtoN(p+1)) {
			if (--udIndex == 0)
				foundSize = atomSize;
		}

		p = (SInt32 *)(atomSize + (char *)p);
	}

	if (foundSize) {
		*p++ = 0;		// put back trailing NULL
		userData->size -= foundSize;
	}

	return 0;
}

QTErr QTUserDataAdd(QTUserData userData, UInt32 udType, const void *ud, UInt32 udSize)
{
	QTErr err;
	UInt32 oldSize = userData->size;
	QTUserData newUD;
	UInt32 header[2];

	err = qtMovieCalloc(userData->movie, oldSize + udSize + sizeof(QTUserDataRecord) + sizeof(UInt32) + 8, &newUD);
	if (err) goto bail;

	qtMemMove(newUD, userData, oldSize + sizeof(QTUserDataRecord));

	header[0] = QTReaderToBigEndian32(udSize + 8);
	header[1] = QTReaderToBigEndian32(udType);

	qtMemMove(((char *)newUD) + sizeof(QTUserDataRecord) - 4 + oldSize, header, 8);
	qtMemMove(((char *)newUD) + sizeof(QTUserDataRecord) - 4 + oldSize + 8, ud, udSize);

	switch (userData->parentType) {
	case kQTUserDataMovieLevel: {
		QTMovie parent = (QTMovie)userData->parent;
		qtMovieFree(userData->movie, parent->userData);
		parent->userData = newUD;
		break;
	}
	case kQTUserDataTrackLevel: {
		QTTrack parent = (QTTrack)userData->parent;
		qtMovieFree(userData->movie, parent->userData);
		parent->userData = newUD;
		break;
	}
	}
	newUD->size += udSize + 8;

bail:
	return err;
}

QTErr QTUserDataNew(QTMovie movie, QTUserData *userData, UInt32 parentType, void *parent)
{
	QTErr err = qtMovieCalloc(movie, sizeof(QTUserDataRecord), userData);
	if (err) return err;
	(*userData)->movie = movie;
	(*userData)->parentType = parentType;
	(*userData)->parent = parent;
	return 0;
}

QTErr qtWriteUserData(QTMovie movie, QTUserData userData)
{
	QTErr err;
	QTAtomWriteStateRecord userDataAtom;

	if (NULL == userData)
		return 0;

	openAtom(movie, &userDataAtom, UserDataAID);

	err = appendData(movie, userData->data, userData->size);
	if (err) goto bail;

	err = closeAtom(movie, &userDataAtom);

bail:
	return err;
}

QTErr qtWriteTrack(QTMovie movie, QTTrack track)
{
	QTErr err;
	QTAtomWriteStateRecord trackAtom, trackHeaderAtom;
	TrackHeader th;

	openAtom(movie, &trackAtom, TrackAID);
	openAtom(movie, &trackHeaderAtom, TrackHeaderAID);

    th.flags = QTReaderToBigEndian32(7);
	th.creationTime = qtGetCurrentTime();
    th.creationTime = QTReaderToBigEndian32(th.creationTime);
    th.modificationTime = th.creationTime;
    th.trackID = QTReaderToBigEndian32(track->id);
    th.reserved1 = 0;
    th.duration = QTReaderToBigEndian32(track->duration);
    th.reserved2 = 0;
    th.reserved3 = 0;
    th.layer = 0;
    th.alternateGroup = 0;
    th.volume = QTReaderToBigEndian16(track->volume);
    th.reserved4 = 0;
#if QT_READER_MATRIX
	copyMatrixToBigEndian((UInt32 *)&th.matrix, track->matrix);
#else /* !QT_READER_MATRIX */
	initializeMatrixBigEndian(&th.matrix);
#endif /* !QT_READER_MATRIX */
    th.trackWidth = QTReaderToBigEndian32(track->width << 16);
    th.trackHeight = QTReaderToBigEndian32(track->height << 16);

	err = appendData(movie, &th, sizeof(th));
	if (err) goto bail;

	err = closeAtom(movie, &trackHeaderAtom);
	if (err) goto bail;

#if QT_READER_EDITS
	if (0 != track->editsCount) {
		err = qtWriteEditList(movie, track->editsCount, track->edits);
		if (err) goto bail;
	}
#endif /* QT_READER_EDITS */

	err = qtWriteMedia(movie, track->media);
	if (err) goto bail;

	err = qtWriteUserData(movie, track->userData);
	if (err) goto bail;

#if QT_READER_MTDT
	err = qtWriteMetaData(movie, track->mtdt);
	if (err) goto bail;
#endif /* QT_READER_MTDT */

#if QT_READER_EXTRA_ATOMS
	err = qtWriteExtras(movie, track->extras);
	if (err) goto bail;
#endif /* QT_READER_EXTRA_ATOMS */

	err = closeAtom(movie, &trackAtom);
	if (err) goto bail;

bail:
	return err;
}

#if QT_READER_EDITS

QTErr qtWriteEditList(QTMovie movie, UInt32 count, QTEdit edits)
{
	QTErr err;
	QTAtomWriteStateRecord editsAtom, editListAtom;
	UInt32 data[2];
	UInt32 i;

	openAtom(movie, &editsAtom, EditsAID);

	openAtom(movie, &editListAtom, EditListAID);

	data[0] = 0;
	data[1] = QTReaderToBigEndian32(count);
	err = appendData(movie, data, sizeof(data));
	if (err) goto bail;

	for (i=0; i<count; i++) {
		QTEditRecord edit;

		edit.mediaTime = QTReaderToBigEndian32(edits[i].mediaTime);
		edit.rate = QTReaderToBigEndian32(edits[i].rate);
		edit.trackDuration = QTReaderToBigEndian32(edits[i].trackDuration);

		err = appendData(movie, &edit, sizeof(edit));
		if (err) goto bail;
	}

	err = closeAtom(movie, &editListAtom);
	if (err) goto bail;

	err = closeAtom(movie, &editsAtom);
	if (err) goto bail;

bail:
	return err;
}

#endif /* QT_READER_EDITS */

#if QT_READER_EXTRA_ATOMS

QTErr qtWriteExtras(QTMovie movie, QTExtraAtom extras)
{
	QTErr err = 0;

	for ( ; NULL != extras; extras = extras->next) {
		QTAtomWriteStateRecord atom;

		openAtom(movie, &atom, extras->type);

		err = appendData(movie, extras->data, extras->size);
		if (err) goto bail;

		err = closeAtom(movie, &atom);
		if (err) goto bail;
	}

bail:
	return err;
}

#endif /* QT_READER_EXTRA_ATOMS */

#if QT_READER_MTDT

QTErr qtWriteMetaData(QTMovie movie, QTMTDT mtdt)
{
	QTErr err;
	QTMTDT walker;
	UInt16 count = 0;
	UInt32 size = 0;
	QTAtomWriteStateRecord atom;
	UInt32 header[2];
	const unsigned char uuid[] = {0x55, 0x53, 0x4D, 0x54, 0x21, 0xD2, 0x4F, 0xCE, 0xBB, 0x88, 0x69, 0x5C, 0xFA, 0xC9, 0xC7, 0x40};

	if (NULL == mtdt)
		return 0;

	for (walker = mtdt; NULL != walker; walker = walker->next) {
		count += 1;
		size += walker->dataSize + 10;
	}

	openAtom(movie, &atom, 'uuid');

	err = appendData(movie, uuid, sizeof(uuid));
	if (err) goto bail;

	header[0] = size + sizeof(header) + sizeof(count);
	header[0] = QTReaderToBigEndian32(header[0]);
	header[1] = QTReaderToBigEndian32('MTDT');
	err = appendData(movie, header, sizeof(header));
	if (err) goto bail;

	count = QTReaderToBigEndian16(count);
	err = appendData(movie, &count, sizeof(count));
	if (err) goto bail;

	for (walker = mtdt; NULL != walker; walker = walker->next) {
		QTMTDTRecord temp = *walker;
		temp.dataSize += 10;
		temp.dataSize = QTReaderToBigEndian16(temp.dataSize);
		temp.dataType = QTReaderToBigEndian32(temp.dataType);
		temp.language = QTReaderToBigEndian16(temp.language);
		temp.encoding = QTReaderToBigEndian16(temp.encoding);

		err = appendData(movie, &temp.dataSize, 10);
		if (err) goto bail;

		err = appendData(movie, walker->data, walker->dataSize);
		if (err) goto bail;
	}

	err = closeAtom(movie, &atom);
	if (err) goto bail;

bail:
	return err;
}

#endif /* QT_READER_MTDT */

QTErr qtWriteMedia(QTMovie movie, QTMedia media)
{
	QTErr err;
	QTAtomWriteStateRecord mediaAtom, mediaHeaderAtom, handlerAtom;
	MediaHeader mh;
	PublicHandlerInfo ph;
	const char noMediaHandlerName[] = {0};
	const char *mediaHandlerName;

	openAtom(movie, &mediaAtom, MediaAID);
	openAtom(movie, &mediaHeaderAtom, MediaHeaderAID);

    mh.flags = QTReaderToBigEndian32(0);
	mh.creationTime = qtGetCurrentTime();
    mh.creationTime = QTReaderToBigEndian32(mh.creationTime);
    mh.modificationTime = mh.creationTime;
    mh.timeScale = QTReaderToBigEndian32(media->scale);
    mh.duration = QTReaderToBigEndian32(media->duration);
	mh.language = movie->isQT ? 0 : QTReaderToBigEndian16(0x555c4);
    mh.quality = QTReaderToBigEndian16(0);

	err = appendData(movie, &mh, sizeof(mh));
	if (err) goto bail;

	err = closeAtom(movie, &mediaHeaderAtom);
	if (err) goto bail;

	openAtom(movie, &handlerAtom, HandlerAID);

    ph.flags = 0;

	ph.componentType = movie->isQT ? QTReaderToBigEndian32('mhlr') : 0;
    ph.componentSubType = QTReaderToBigEndian32(media->mediaType);
    ph.componentManufacturer = 0;
	ph.componentFlags = 0;
    ph.componentFlagsMask = 0;

	err = appendData(movie, &ph, sizeof(UInt32) * 6);
	if (err) goto bail;

	if (false == movie->isQT) {
		// audio & video media handler names required by Memory Stick Video spec. Is compatible with MPEG-4.
		const char *p;
		UInt32 len;

		if (kQTVideoType == media->mediaType)
			mediaHandlerName = "Video Media Handler";
		else if (kQTSoundType == media->mediaType)
			mediaHandlerName = "Sound Media Handler";
		else
			mediaHandlerName = noMediaHandlerName;

		for (p = mediaHandlerName, len = 0; 0 != *p; p++, len++)
			;

		err = appendData(movie, mediaHandlerName, len + 1);
	}
	else
		err = appendData(movie, noMediaHandlerName, sizeof(noMediaHandlerName));
	if (err) goto bail;

	err = closeAtom(movie, &handlerAtom);
	if (err) goto bail;
	
	err = qtWriteMediaInfo(movie, media);
	if (err) goto bail;

	err = closeAtom(movie, &mediaAtom);
	if (err) goto bail;

bail:
	return err;
}

QTErr qtWriteMediaInfo(QTMovie movie, QTMedia media)
{
	QTErr err;
	QTAtomWriteStateRecord mediaInfoAtom, mediaHeaderAtom;

	openAtom(movie, &mediaInfoAtom, MediaInfoAID);

	// media type specific atoms
	if (kQTVideoType == media->mediaType) {
		VideoMediaInfoHeader vh;

		openAtom(movie, &mediaHeaderAtom, VideoMediaInfoHeaderAID);

		vh.flags = QTReaderToBigEndian32(1);
		vh.graphicsMode = movie->isQT ? QTReaderToBigEndian16(64) : 0;
		vh.opColorRed = 0;
		vh.opColorGreen = 0;
		vh.opColorBlue = 0;

		err = appendData(movie, &vh, sizeof(vh));
		if (err) goto bail;

		err = closeAtom(movie, &mediaHeaderAtom);
		if (err) goto bail;
	}
	else if (kQTSoundType == media->mediaType) {
		SoundMediaInfoHeader sh;

		openAtom(movie, &mediaHeaderAtom, SoundMediaInfoHeaderAID);

		sh.flags = 0;
		sh.balance = 0;
		sh.rsrvd = 0;

		err = appendData(movie, &sh, sizeof(sh));
		if (err) goto bail;

		err = closeAtom(movie, &mediaHeaderAtom);
		if (err) goto bail;
	}

	err = qtWriteDataInfo(movie, media);
	if (err) goto bail;

	err = qtWriteSampleTable(movie, media);
	if (err) goto bail;

	err = closeAtom(movie, &mediaInfoAtom);
	if (err) goto bail;

bail:
	return err;
}

QTErr qtWriteDataInfo(QTMovie movie, QTMedia media)
{
	QTErr err;
	QTAtomWriteStateRecord dataInfoAtom, dataRefsAtom;
	UInt32 d[3];
	QTDataReference drWalker;

	openAtom(movie, &dataInfoAtom, DataInfoAID);
	openAtom(movie, &dataRefsAtom, DataRefAID);

	d[0] = 0;
	d[1] = qtListCount(media->dataReferences);
	d[1] = QTReaderToBigEndian32(d[1]);
	err = appendData(movie, d, sizeof(UInt32) * 2);
	if (err) goto bail;

	for (drWalker = (QTDataReference)media->dataReferences; drWalker; drWalker = (QTDataReference)drWalker->next) {
		d[0] = 12 + drWalker->drSize;
		d[0] = QTReaderToBigEndian32(d[0]);
		d[1] = QTReaderToBigEndian32(drWalker->kind);
		d[2] = QTReaderToBigEndian32(drWalker->flags);
		err = appendData(movie, d, sizeof(UInt32) * 3);
		if (err) goto bail;
		err = appendData(movie, drWalker->dataRef, drWalker->drSize);
		if (err) goto bail;
	}

	err = closeAtom(movie, &dataRefsAtom);
	if (err) goto bail;

	err = closeAtom(movie, &dataInfoAtom);
	if (err) goto bail;

bail:
	return err;
}

QTErr qtWriteSampleTable(QTMovie movie, QTMedia media)
{
	QTErr err;
	QTAtomWriteStateRecord sampleTableAtom, chunkOffsetAtom, sampleSizeAtom, syncSampleAtom, timeToSampleAtom, sampleToChunkAtom, sampleDescriptionAtom, compositionTimeOffsetAtom;
	UInt32 d[3], i, sampleDescriptionCount;
	QTSampleDescription sdWalker;

	openAtom(movie, &sampleTableAtom, SampleTableAID);

	// sample description atom
	openAtom(movie, &sampleDescriptionAtom, STSampleDescAID);

	for (sampleDescriptionCount = 0, sdWalker = (QTSampleDescription)media->sampleDescriptions; NULL != sdWalker; sdWalker = (QTSampleDescription)sdWalker->next)
		sampleDescriptionCount += 1;

	d[0] = 0;
	d[1] = QTReaderToBigEndian32(sampleDescriptionCount);
	err = appendData(movie, d, sizeof(UInt32) * 2);
	if (err) goto bail;

	for (sdWalker = (QTSampleDescription)media->sampleDescriptions; NULL != sdWalker; sdWalker = (QTSampleDescription)sdWalker->next) {
		UInt32 descSize = *(UInt32 *)sdWalker->desc;
		unsigned char *desc;

		err = qtMovieMalloc(movie, descSize, &desc);
		if (err) goto bail;

		for (i=0; i<descSize; i++)
			desc[i] = sdWalker->desc[i];

        QTSampleDescriptionFlip(desc, media->mediaType, false);

		err = appendData(movie, desc, descSize);
		if (err) goto bail;

		qtMovieFree(movie, desc);
	}

	err = closeAtom(movie, &sampleDescriptionAtom);
	if (err) goto bail;

	// time to sample atom
	openAtom(movie, &timeToSampleAtom, STTimeToSampAID);

	d[0] = 0;
	d[1] = QTReaderToBigEndian32(media->timeToSampleCount);
	err = appendData(movie, d, sizeof(UInt32) * 2);
	if (err) goto bail;

	for (i=0; i<media->timeToSampleCount; i++) {
		d[0] = QTReaderToBigEndian32(media->timeToSamples[i].count);
		d[1] = QTReaderToBigEndian32(media->timeToSamples[i].duration);
		err = appendData(movie, d, sizeof(UInt32) * 2);
		if (err) goto bail;
	}

	err = closeAtom(movie, &timeToSampleAtom);
	if (err) goto bail;

	// sample to chunk atom
	openAtom(movie, &sampleToChunkAtom, STSampleToChunkAID);

	d[0] = 0;
	d[1] = QTReaderToBigEndian32(media->sampleToChunkCount);
	err = appendData(movie, d, sizeof(UInt32) * 2);
	if (err) goto bail;

	for (i=0; i<media->sampleToChunkCount; i++) {
		d[0] = QTReaderToBigEndian32(media->sampleToChunks[i].firstChunk);
		d[1] = QTReaderToBigEndian32(media->sampleToChunks[i].samplesPerChunk);
		d[2] = QTReaderToBigEndian32(media->sampleToChunks[i].sampleDescriptionIndex);
		err = appendData(movie, d, sizeof(UInt32) * 3);
		if (err) goto bail;
	}

	err = closeAtom(movie, &sampleToChunkAtom);
	if (err) goto bail;

	// sample size atom
	openAtom(movie, &sampleSizeAtom, STSampleSizeAID);

	d[0] = 0;
	if (1 == media->sampleSizeCount)
		d[1] = QTReaderToBigEndian32(media->sampleSizes[0]);
	else
		d[1] = 0;
	d[2] = QTReaderToBigEndian32(media->sampleCount);
	err = appendData(movie, d, sizeof(UInt32) * 3);
	if (err) goto bail;

	if (media->sampleSizeCount > 1) {
		for (i=0; i<media->sampleSizeCount; i++) {
			d[0] = QTReaderToBigEndian32(media->sampleSizes[i]);
			err = appendData(movie, d, sizeof(UInt32));
			if (err) goto bail;
		}
	}

	err = closeAtom(movie, &sampleSizeAtom);
	if (err) goto bail;

	// chunk offset atom
#if QT_READER_FILE64
	if (NULL == media->chunkOffsets64) {
#endif /* QT_READER_FILE64 */
		openAtom(movie, &chunkOffsetAtom, STChunkOffsetAID);

		d[0] = 0;
		d[1] = QTReaderToBigEndian32(media->chunkOffsetCount);
		err = appendData(movie, d, sizeof(UInt32) * 2);
		if (err) goto bail;

		for (i=0; i<media->chunkOffsetCount; i++) {
			d[0] = QTReaderToBigEndian32(media->chunkOffsets[i]);
			err = appendData(movie, d, sizeof(UInt32));
			if (err) goto bail;
		}
#if QT_READER_FILE64
	}
	else {
		openAtom(movie, &chunkOffsetAtom, STChunkOffset64AID);

		d[0] = 0;
		d[1] = QTReaderToBigEndian32(media->chunkOffsetCount);
		err = appendData(movie, d, sizeof(UInt32) * 2);
		if (err) goto bail;

		for (i=0; i<media->chunkOffsetCount; i++) {
			QTFileOffset offset = media->chunkOffsets64[i];
			offset = QTReaderToBigEndian64(offset);
			err = appendData(movie, &offset, sizeof(QTFileOffset));
			if (err) goto bail;
		}
	}
#endif /* QT_READER_FILE64 */
	err = closeAtom(movie, &chunkOffsetAtom);
	if (err) goto bail;

	// sync sample atom
	if (0 != media->syncSampleCount) {
		openAtom(movie, &syncSampleAtom, STSyncSampleAID);

		d[0] = 0;
		d[1] = QTReaderToBigEndian32(media->syncSampleCount);
		err = appendData(movie, d, sizeof(UInt32) * 2);
		if (err) goto bail;

		for (i=0; i<media->syncSampleCount; i++) {
			d[0] = QTReaderToBigEndian32(media->syncSamples[i]);
			err = appendData(movie, d, sizeof(UInt32));
			if (err) goto bail;
		}

		err = closeAtom(movie, &syncSampleAtom);
		if (err) goto bail;
	}

#if QT_READER_STPS
	if (0 != media->partialSyncSampleCount) {
		QTAtomWriteStateRecord partialSyncSampleAtom;

		openAtom(movie, &partialSyncSampleAtom, STPartialSyncSampleAID);

		d[0] = 0;
		d[1] = QTReaderToBigEndian32(media->partialSyncSampleCount);
		err = appendData(movie, d, sizeof(UInt32) * 2);
		if (err) goto bail;

		for (i=0; i<media->partialSyncSampleCount; i++) {
			d[0] = QTReaderToBigEndian32(media->partialSyncSamples[i]);
			err = appendData(movie, d, sizeof(UInt32));
			if (err) goto bail;
		}

		err = closeAtom(movie, &partialSyncSampleAtom);
		if (err) goto bail;
	}
#endif

#if QT_READER_SDTP
	if (0 != media->sampleDependencyCount) {
		QTAtomWriteStateRecord sampleDependencyAtom;

		openAtom(movie, &sampleDependencyAtom, STSampleDependencyAID);

		d[0] = 0;		// flags
		err = appendData(movie, d, sizeof(UInt32));
		if (err) goto bail;

		err = appendData(movie, media->sampleDependencies, media->sampleDependencyCount);
		if (err) goto bail;

		err = closeAtom(movie, &sampleDependencyAtom);
		if (err) goto bail;
	}
#endif

	// composition time offset atom
	if (0 != media->compositionTimeCount) {
		openAtom(movie, &compositionTimeOffsetAtom, STCompositionTimeAID);

		d[0] = 0;
		d[1] = QTReaderToBigEndian32(media->compositionTimeCount);
		err = appendData(movie, d, sizeof(UInt32) * 2);
		if (err) goto bail;

		for (i=0; i<media->compositionTimeCount; i++) {
			d[0] = QTReaderToBigEndian32(media->compositionTimeOffsets[i].count);
			d[1] = QTReaderToBigEndian32(media->compositionTimeOffsets[i].offset);
			err = appendData(movie, d, sizeof(UInt32) * 2);
			if (err) goto bail;
		}

		err = closeAtom(movie, &compositionTimeOffsetAtom);
		if (err) goto bail;
	}

	// close out sample table atom
	err = closeAtom(movie, &sampleTableAtom);
	if (err) goto bail;

bail:
	return err;
}

QTErr appendData(QTMovie movie, const void *data, UInt32 dataSize)
{
	QTErr err = (movie->writer)(movie->writerRefCon, data, movie->writeOffset, dataSize);
	if (0 == err)
		movie->writeOffset += dataSize;
	return err;
}

void openAtom(QTMovie movie, QTAtomWriteState atom, UInt32 atomType)
{
	atom->type = atomType;
	atom->offset = movie->writeOffset;
	movie->writeOffset += sizeof(UInt32) * 2;
}

QTErr closeAtom(QTMovie movie, const QTAtomWriteState atom)
{
	UInt32 atomHeader[2];

	atomHeader[0] = (UInt32)(movie->writeOffset - atom->offset);
	atomHeader[1] = atom->type;
	atomHeader[0] = QTReaderToBigEndian32(atomHeader[0]);
	atomHeader[1] = QTReaderToBigEndian32(atomHeader[1]);
	return (movie->writer)(movie->writerRefCon, atomHeader, atom->offset, sizeof(UInt32) * 2);
}

void initializeMatrixBigEndian(QTMatrixRecord *matrix)
{
	matrix->matrix[0] = QTReaderToBigEndian32(0x00010000);
	matrix->matrix[1] = 0;
	matrix->matrix[2] = 0;
	matrix->matrix[3] = 0;
	matrix->matrix[4] = matrix->matrix[0];
	matrix->matrix[5] = 0;
	matrix->matrix[6] = 0;
	matrix->matrix[7] = 0;
	matrix->matrix[8] = QTReaderToBigEndian32(0x40000000);
}

#if TARGET_OS_MAC || TARGET_OS_WIN32 || TARGET_OS_LINUX
	#include <time.h>

	// 1970/01/01 - 1904/01/01 (in sec)
	#define MAC_EPOCH (2082758400)

	UInt32 qtGetCurrentTime(void)
	{
		return (UInt32)time(NULL) + MAC_EPOCH;
	}
#else
	UInt32 qtGetCurrentTime(void)
	{
		return 0;
	}
#endif

QTErr QTMediaAddSamples(QTMedia media, UInt16 sampleDescriptionIndex, QTFileOffset fileOffset, UInt32 infoCount, QTSampleInfo samples)
{
	QTErr err = 0;
	UInt32 totalSamples, totalDuration;
	UInt32 i;
	QTSampleInfo walker;
	QTTrack track = media->track;
	QTMovie movie = track->movie;
	QTSampleToChunk stc;
	QTSampleInfo samplesCopy = NULL;

	// parameter checks (could validate sampleDescriptionIndex too)
	if (0 == infoCount) {
		err = -1;
		goto bail;
	}

	// pre-compute some totals
	for (i=0, walker=samples, totalSamples=0, totalDuration=0; i<infoCount; i++, walker++) {
		totalSamples += walker->sampleCount;
		totalDuration += walker->sampleCount * walker->sampleDuration;
	}

	// think about auto-chunking (calls to QTMediaAddSamples should include no more than media->audioChunkCount samples or this code may fail to generate an optimal movie)
	stc = &media->sampleToChunks[media->sampleToChunkCount - 1];
	if (0 != media->autoChunkCount) {
		if ((NULL != media->sampleToChunks) && (stc->samplesPerChunk < media->autoChunkCount) && (stc->sampleDescriptionIndex == sampleDescriptionIndex)) {		// there's room in the last chunk
			QTFileOffset chunkSize;
#if !QT_READER_FILE64
			QTFileOffset chunkOffset = media->chunkOffsets[media->chunkOffsetCount - 1];
#else /* QT_READER_FILE64 */
			QTFileOffset chunkOffset = media->chunkOffsets64 ? media->chunkOffsets64[media->chunkOffsetCount - 1] : media->chunkOffsets[media->chunkOffsetCount - 1];
#endif /* QT_READER_FILE64 */

			QTTrackGetSampleSizes(track, media->sampleCount - (stc->samplesPerChunk - 1), stc->samplesPerChunk, &chunkSize);
			if (fileOffset == (chunkOffset + chunkSize)) {			// this chunk is contiguous with previous chunk
				QTSampleInfo samples2;
				UInt32 infoCount2 = infoCount;

				// copy input sample records so we can modify them
				err = qtMovieMalloc(movie, sizeof(QTSampleInfoRecord) * infoCount, &samplesCopy);
				if (err) goto bail;

				qtMemMove(samplesCopy, samples, sizeof(QTSampleInfoRecord) * infoCount);
				samples2 = samplesCopy;

				while (stc->samplesPerChunk < media->autoChunkCount) {
					UInt32 samplesNeeded = media->autoChunkCount - stc->samplesPerChunk;
					UInt32 samplesToUse;
					if (stc->firstChunk != media->chunkOffsetCount) {
						// extend sample-to-chunk table
						err = qtMovieRealloc(movie, &media->sampleToChunks, media->sampleToChunkCount * sizeof(QTSampleToChunkRecord), (media->sampleToChunkCount + 1) * sizeof(QTSampleToChunkRecord), &media->sampleToChunksSize);
						if (err) goto bail;

						stc = &media->sampleToChunks[media->sampleToChunkCount];
						stc->firstChunk = media->chunkOffsetCount;
						stc->sampleDescriptionIndex = sampleDescriptionIndex;
						stc->samplesPerChunk = stc[-1].samplesPerChunk;

						media->sampleToChunkCount += 1;
					}

					samplesToUse = samplesNeeded;
					if (samplesToUse > samples2->sampleCount)
						samplesToUse = samples2->sampleCount;
					stc->samplesPerChunk += samplesToUse;
					if ((media->sampleToChunkCount >= 2) && (stc[-1].samplesPerChunk == stc[0].samplesPerChunk)) {
						media->sampleToChunkCount -= 1;		// merge last two samplePerChunk entries together
						stc -= 1;
					}
					fileOffset += samplesToUse * samples2->sampleSize;
					totalSamples -= samplesToUse;

					samples2->sampleCount -= samplesToUse;
					if (0 == samples2->sampleCount) {
						samples2 += 1;
						infoCount2 -= 1;
						if (0 == infoCount2)
							break;
					}
				}
			}
		}
	}

	if (0 != totalSamples) {
		// update chunk offset
#if !QT_READER_FILE64
		err = qtMovieRealloc(movie, &media->chunkOffsets, media->chunkOffsetCount * sizeof(UInt32), (media->chunkOffsetCount + 1) * sizeof(UInt32), &media->chunkOffsetsSize);
		if (err) goto bail;

		media->chunkOffsets[media->chunkOffsetCount] = fileOffset;		
#else /* QT_READER_FILE64 */
		if ((fileOffset > 0xffffffff) && (NULL == media->chunkOffsets64)) {
			// time to convert over
			QTFileOffset *chunkOffsets64;

			err = qtMovieMalloc(movie, (media->chunkOffsetCount + 1) * sizeof(QTFileOffset), &chunkOffsets64);
			if (err) goto bail;

			media->chunkOffsetsSize = (media->chunkOffsetCount + 1) * sizeof(QTFileOffset);

			for (i=0; i<media->chunkOffsetCount; i++)
				chunkOffsets64[i] = media->chunkOffsets[i];

			qtMovieFree(movie, media->chunkOffsets);
			media->chunkOffsets = NULL;
			media->chunkOffsets64 = chunkOffsets64;
		}

		if (NULL == media->chunkOffsets64) {
			err = qtMovieRealloc(movie, &media->chunkOffsets, media->chunkOffsetCount * sizeof(UInt32), (media->chunkOffsetCount + 1) * sizeof(UInt32), &media->chunkOffsetsSize);
			if (err) goto bail;

			media->chunkOffsets[media->chunkOffsetCount] = (UInt32)fileOffset;
		}
		else {
			err = qtMovieRealloc(movie, &media->chunkOffsets64, media->chunkOffsetCount * sizeof(QTFileOffset), (media->chunkOffsetCount + 1) * sizeof(QTFileOffset), &media->chunkOffsetsSize);
			if (err) goto bail;

			media->chunkOffsets64[media->chunkOffsetCount] = fileOffset;
		}
#endif /* QT_READER_FILE64 */
		media->chunkOffsetCount += 1;

		// update sample to chunk
		stc = &media->sampleToChunks[media->sampleToChunkCount - 1];
		if (!media->sampleToChunks || (stc->sampleDescriptionIndex != sampleDescriptionIndex) || (stc->samplesPerChunk != totalSamples)) {
			err = qtMovieRealloc(movie, &media->sampleToChunks, media->sampleToChunkCount * sizeof(QTSampleToChunkRecord), (media->sampleToChunkCount + 1) * sizeof(QTSampleToChunkRecord), &media->sampleToChunksSize);
			if (err) goto bail;

			stc = &media->sampleToChunks[media->sampleToChunkCount];
			stc->firstChunk = media->chunkOffsetCount;
			stc->sampleDescriptionIndex = sampleDescriptionIndex;
			stc->samplesPerChunk = totalSamples;

			media->sampleToChunkCount += 1;
		}
	}

	for (i=0, walker=samples; i<infoCount; i++, walker++) {
		UInt32 j;
		QTTimeToSample tts;

		// update sample sizes
		if (0 == media->sampleSizeCount) {
			err = qtMovieMalloc(movie, sizeof(UInt32), &media->sampleSizes);
			if (err) goto bail;

			media->sampleSizeCount = 1;
			media->sampleSizes[0] = walker->sampleSize;
		}
		if ((1 == media->sampleSizeCount) && (media->sampleSizes[0] == walker->sampleSize))
			;		// nothing to do
		else {
			if (1 == media->sampleSizeCount) {
				// need to expand out existing table
				err = qtMovieRealloc(movie, &media->sampleSizes, sizeof(UInt32), sizeof(UInt32) * media->sampleCount, NULL);
				if (err) goto bail;

				for (j=0; j<media->sampleCount; j++)
					media->sampleSizes[j] = media->sampleSizes[0];

				media->sampleSizeCount = media->sampleCount;
			}

			// add new entries
			err = qtMovieRealloc(movie, &media->sampleSizes, sizeof(UInt32) * media->sampleCount,
										sizeof(UInt32) * (media->sampleCount + walker->sampleCount), &media->sampleSizesSize);
			if (err) goto bail;

			for (j=0; j<walker->sampleCount; j++)
				media->sampleSizes[j + media->sampleSizeCount] = walker->sampleSize;

			media->sampleSizeCount += walker->sampleCount;
		}

		// update time to sample
		if (media->timeToSampleCount) {
            tts = &media->timeToSamples[media->timeToSampleCount - 1];
            if (media->timeToSamples && (tts->duration == walker->sampleDuration)) {
                tts->count += walker->sampleCount;
            }
        }

        if (media->timeToSampleCount == 0) {
			err = qtMovieMalloc(movie, sizeof(QTTimeToSampleRecord), &media->timeToSamples);
			if (err) goto bail;

			tts = &media->timeToSamples[0];
			tts->count = walker->sampleCount;
			tts->duration = walker->sampleDuration;

            media->timeToSampleCount = 1;
        }
		else {
			// add new entries
			err = qtMovieRealloc(movie, &media->timeToSamples, sizeof(QTTimeToSampleRecord) * media->timeToSampleCount,
										sizeof(QTTimeToSampleRecord) * (media->timeToSampleCount + 1), &media->timeToSamplesSize);
			if (err) goto bail;

			tts = &media->timeToSamples[media->timeToSampleCount];
			tts->count = walker->sampleCount;
			tts->duration = walker->sampleDuration;

			media->timeToSampleCount += 1;
		}

		// update sync samples
		if (walker->keyFrame && (0 == media->syncSampleCount))
			;		// nothing to do
		else {
			if ((0 == media->syncSampleCount) && (0 != media->sampleCount)) {
				// expand out the table. all existing samples are sync samples
				err = qtMovieMalloc(movie, sizeof(UInt32) * media->sampleCount, &media->syncSamples);
				if (err) goto bail;

				for (j=0; j<media->sampleCount; j++)
					media->syncSamples[j] = j + 1;

				media->syncSampleCount = media->sampleCount;
			}

			if (walker->keyFrame) {
				// add key frame entries for this group of samples
				err = qtMovieRealloc(movie, &media->syncSamples, sizeof(UInt32) * media->syncSampleCount,
											sizeof(UInt32) * (media->syncSampleCount + walker->sampleCount), &media->syncSamplesSize);
				if (err) goto bail;

				for (j=0; j<walker->sampleCount; j++)
					media->syncSamples[media->syncSampleCount + j] = media->sampleCount + j + 1;

				media->syncSampleCount += walker->sampleCount;
			}
		}

#if QT_READER_STPS
		if (walker->partialSync) {
			// update partial sync sample table
			err = qtMovieRealloc(movie, &media->partialSyncSamples, sizeof(UInt32) * media->partialSyncSampleCount,
										sizeof(UInt32) * (media->partialSyncSampleCount + walker->sampleCount), &media->partialSyncSamplesSize);
			if (err) goto bail;

			for (j=0; j<walker->sampleCount; j++)
				media->partialSyncSamples[media->partialSyncSampleCount + j] = media->sampleCount + j + 1;

			media->partialSyncSampleCount += walker->sampleCount;
		}
#endif

#if QT_READER_SDTP
		if ((NULL != media->sampleDependencies) || walker->droppable) {
			UInt8 dependencies;

			if (NULL == media->sampleDependencies) {
				err = qtMovieMalloc(movie, (media->sampleCount + walker->sampleCount) * sizeof(UInt8), &media->sampleDependencies);
				if (err) goto bail;

				media->sampleDependencyCount = media->sampleCount;
				media->sampleDependenciesSize = media->sampleCount;

				for (j=0; j<media->sampleDependencyCount; j++)
					media->sampleDependencies[j] = 0;
			}
			else {
				err = qtMovieRealloc(movie, &media->sampleDependencies, sizeof(UInt8) * media->sampleDependencyCount,
											sizeof(UInt8) * (media->sampleDependencyCount + walker->sampleCount), &media->sampleDependenciesSize);
				if (err) goto bail;
			}

			if (!walker->keyFrame && walker->droppable)
				dependencies = 1 << 3;		// see ISO AVC file format document, section 4.3
			else
				dependencies = 0;

			for (j=0; j<walker->sampleCount; j++)
				media->sampleDependencies[media->sampleDependencyCount + j] = dependencies;

			media->sampleDependencyCount += walker->sampleCount;
		}
#endif

		// update composition time offset
		if (0 == media->compositionTimeCount) {
			if (0 != walker->compositionOffset) {
				// create new table
				UInt32 count = 1 + (media->sampleCount ? 1 : 0);
				err = qtMovieMalloc(movie, count * sizeof(QTCompositionTimeOffsetRecord), &media->compositionTimeOffsets);
				if (err) goto bail;

				if (1 == count) {
					media->compositionTimeOffsets[0].count = walker->sampleCount;
					media->compositionTimeOffsets[0].offset = walker->compositionOffset;
				}
				else {
					media->compositionTimeOffsets[0].count = media->sampleCount;
					media->compositionTimeOffsets[0].offset = 0;
					media->compositionTimeOffsets[1].count = walker->sampleCount;
					media->compositionTimeOffsets[1].offset = walker->compositionOffset;
				}
				media->compositionTimeCount = count;
			}
			else
				;		// no table needed yet
		}
		else
		if (walker->compositionOffset == media->compositionTimeOffsets[media->compositionTimeCount - 1].offset) {
			// extend existing entry
			media->compositionTimeOffsets[media->compositionTimeCount - 1].count += walker->sampleCount;
		}
		else {
			// add table entry
			err = qtMovieRealloc(movie, &media->compositionTimeOffsets, sizeof(QTCompositionTimeOffsetRecord) * media->compositionTimeCount,
										sizeof(QTCompositionTimeOffsetRecord) * (media->compositionTimeCount + 1), &media->compositionTimeOffsetsSize);
			if (err) goto bail;

			media->compositionTimeOffsets[media->compositionTimeCount].count = walker->sampleCount;
			media->compositionTimeOffsets[media->compositionTimeCount].offset = walker->compositionOffset;
			media->compositionTimeCount += 1;
		}

		// update sampleCount
		media->sampleCount += walker->sampleCount;
	}

	// invalidate caches
	media->ssCacheSampleNumber = 0;
	media->ssCacheIndex = 0;
	media->stcCacheSampleNumber = 0;
	media->stcCacheIndex = 0;
	media->ttsCacheIndex = 0;
	media->ttsCacheMediaTime = 0;
	media->ttsCacheSampleNumber = 0;

	// update media header
	media->duration += totalDuration;

	// update track header
	track->duration = QTTrackToMovieDuration(media->duration, track);

	// update movie header
	calculateMovieDuration(movie);

bail:
	qtMovieFree(movie, samplesCopy);

	return err;
}

QTErr QTAtomWriterOpen(QTAtomWriter *atomOut, UInt32 atomType, QTFileOffset fileOffset, Boolean couldBeBig, QTMovieWriteProc write, void *writeRefCon, QTMovieAllocProc alloc, QTMovieFreeProc free, void *allocRefCon)
{
	QTErr err;
	QTAtomWriter atom;

	err = (alloc)(allocRefCon, false, sizeof(QTAtomWriterRecord), (void **)(void *)&atom);
	if (err) goto bail;

	atom->atomType = atomType;
	atom->size = 2 * sizeof(UInt32);
#if QT_READER_FILE64
	if (couldBeBig)
		atom->size += 8;
	atom->couldBeBig = couldBeBig;
#endif /* QT_READER_FILE64 */
	atom->offset = fileOffset;
	atom->writer = write;
	atom->writerRefCon = writeRefCon;
	atom->alloc = alloc;
	atom->free = free;
	atom->allocRefCon = allocRefCon;

bail:
	if (err) {
		(free)(allocRefCon, atom);
		atom = NULL;
	}

	*atomOut = atom;

	return err;
}

QTErr QTAtomWriterAppend(QTAtomWriter atom, const void *data, UInt32 dataSize, QTFileOffset *dataOffset)
{
	QTErr err;

	if (NULL != dataOffset)
		*dataOffset = atom->offset + atom->size;

	err = (atom->writer)(atom->writerRefCon, (void *)data, atom->offset + atom->size, dataSize);
	if (0 == err)
		atom->size += dataSize;

	return err;
}

QTErr QTAtomWriterClose(QTAtomWriter atom, QTFileOffset *nextOffset)
{
	QTErr err;
	UInt32 header[2];

	if (NULL == atom) return 0;

	if (NULL != nextOffset)
		*nextOffset = atom->offset + atom->size;

#if QT_READER_FILE64
	if (atom->size <= 0xffffffff) {
		if (atom->couldBeBig) {
			header[0] = QTReaderToBigEndian32(8);
			header[1] = QTReaderToBigEndian32('free');
			err = (atom->writer)(atom->writerRefCon, header, atom->offset, sizeof(UInt32) * 2);
			if (err) goto bail;

			atom->offset += 8;
			atom->size -= 8;
		}
#endif /* QT_READER_FILE64 */
		header[0] = QTReaderToBigEndian32(atom->size);
		header[1] = QTReaderToBigEndian32(atom->atomType);
		err = (atom->writer)(atom->writerRefCon, header, atom->offset, sizeof(UInt32) * 2);
#if QT_READER_FILE64
	}
	else {
		QTFileOffset size64;
		if (false == atom->couldBeBig) {
			err = -1;
			goto bail;
		}

		header[0] = QTReaderToBigEndian32(1);
		header[1] = QTReaderToBigEndian32(atom->atomType);
		err = (atom->writer)(atom->writerRefCon, header, atom->offset, sizeof(UInt32) * 2);
		if (err) goto bail;

		size64 = QTReaderToBigEndian64(atom->size);
		err = (atom->writer)(atom->writerRefCon, &size64, atom->offset + 8, sizeof(size64));
	}

bail:
#endif /* QT_READER_FILE64 */
	(atom->free)(atom->allocRefCon, atom);

	return err;
}

QTErr QTSampleDescriptionAddExtension(QTMovie movie, void **sd, UInt32 extension, const void *data, UInt32 dataSize)
{
	QTErr err;
	UInt32 header[2];
	QTGenericDescription gd = (QTGenericDescription)*sd;

	err = qtMovieRealloc(movie, sd, gd->descSize, gd->descSize + dataSize + sizeof(header), NULL);
	if (err) return err;

	gd = (QTGenericDescription)*sd;
	header[0] = sizeof(header) + dataSize;
	header[1] = extension;
	qtMemMove(gd->descSize + (char *)gd, header, sizeof(header));
	qtMemMove(gd->descSize + sizeof(header) + (char *)gd, (void *)data, dataSize);

	gd->descSize += dataSize + sizeof(header);

	return 0;
}

#if QT_READER_MTDT

QTErr QTAddMTDT(QTMovie movie, QTTrack track, UInt16 dataSize, UInt32 dataType, UInt16 language, UInt16 encoding, void *data)
{
	QTErr err;
	QTMTDT mtdt;

	err = qtMovieMalloc(movie, sizeof(QTMTDTRecord) + dataSize, &mtdt);
	if (err) return err;

	mtdt->next = NULL;
	mtdt->dataSize = dataSize;
	mtdt->dataType = dataType;
	mtdt->language = language;
	mtdt->encoding = encoding;
	qtMemMove(&mtdt->data, data, dataSize);

	if (track) {
		mtdt->next = track->mtdt;
		track->mtdt = mtdt;
	}
	else {
		mtdt->next = movie->mtdt;
		movie->mtdt = mtdt;
	}

	return 0;
}

#endif /* QT_READER_MTDT */

#endif /* QT_READER_AUTHORING */

#if QT_READER_MATRIX

void initializeMatrix(QTMatrixRecord *matrix)
{
	matrix->matrix[0] = 0x00010000;
	matrix->matrix[1] = 0;
	matrix->matrix[2] = 0;
	matrix->matrix[3] = 0;
	matrix->matrix[4] = matrix->matrix[0];
	matrix->matrix[5] = 0;
	matrix->matrix[6] = 0;
	matrix->matrix[7] = 0;
	matrix->matrix[8] = 0x40000000;
}

#if QT_READER_AUTHORING

void copyMatrixToBigEndian(UInt32 *dest, UInt32 *src)
{
	UInt16 i;
	for (i=0; i<9; i++)
		dest[i] = QTReaderToBigEndian32(src[i]);
}
#endif /* QT_READER_AUTHORING */

void copyMatrixToNative(UInt32 *dest, UInt32 *src)
{
	UInt16 i;
	for (i=0; i<9; i++)
		dest[i] = QTReaderToNative32(src[i]);
}

#endif

#if QT_READER_QTATOM

void *QTAtomGetRootAtom(void *qtAtom)
{
	return 12 + (char *)qtAtom;
}

void *QTAtomGetAtomByID(void *qtAtom, UInt32 atomType, UInt32 atomID)
{
	unsigned char *aw = (unsigned char *)qtAtom;
	SInt32 size = QTReaderMisaligned32_GetBtoN(&aw[0]);

	size -= 20;
	aw += 20;

	while (size >= 20) {
		UInt32 atomSize = QTReaderMisaligned32_GetBtoN(&aw[0]);
		if ((atomType == QTReaderMisaligned32_GetBtoN(&aw[4])) &&
			(atomID == QTReaderMisaligned32_GetBtoN(&aw[8]))) {
			return aw;
		}
		size -= atomSize;
		aw += atomSize;
	}

	return NULL;
}

void *QTAtomGetAtomDataPtr(void *qtAtom, UInt32 *dataSize)
{
	unsigned char *aw = (unsigned char *)qtAtom;
	UInt32 childCount = (aw[14] << 8) | aw[15];
	if (childCount)
		return NULL;
	if (dataSize)
		*dataSize = QTReaderMisaligned32_GetBtoN(&aw[0]) - 20;
	return aw + 20;
}

#endif /* QT_READER_QTATOM */

#if QT_READER_SKIP_COVER_IN_MINIMAL

QTErr selectiveUserDataAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt32 d[2];
	UInt32 sizeNow = sizeof(QTUserDataRecord) + movie->userData->size + sizeof(UInt32);
	char *zero;

	err = (movie->reader)(movie->readerRefCon, d, offset - 8, 8);
	if (err) return err;

	d[0] = QTReaderToNative32(d[0]);
	d[1] = QTReaderToNative32(d[1]);

	if (d[1] != 'meta') {
		// append this atom to the userdata
		err = qtMovieRealloc(movie, &movie->userData, sizeNow, sizeNow + (UInt32)size + 8, NULL);
		if (err) goto bail;

		err = (movie->reader)(movie->readerRefCon, movie->userData->data + movie->userData->size, offset - 8, (UInt32)size + 8);
		if (err) goto bail;
		
		movie->userData->size += (UInt32)(size + 8);
	}
	else {
		// selectively parse the metadata
		QTMovieAtomWalkersRecord walkers[] = {{kQTMovieWalkAnyAtom, NULL}, {0,0}};
		UInt32 originalSize;

		walkers[0].walker = selectiveMetaUserDataAtom;
		err = qtMovieRealloc(movie, &movie->userData, sizeNow, sizeNow + 12, NULL);
		if (err) goto bail;

		err = (movie->reader)(movie->readerRefCon, movie->userData->data + movie->userData->size, offset - 8, 12);
		if (err) goto bail;
		
		originalSize = movie->userData->size;
		movie->userData->size += 12;

		err = QTMovieWalkAtoms(movie, offset + 4, size - 4, walkers);
		if (err) goto bail;

		// update meta size atom
		sizeNow = movie->userData->size - originalSize;
		sizeNow = QTReaderToBigEndian32(sizeNow);
		qtMemMove(movie->userData->data + originalSize, &sizeNow, 4);
	}


	// trailing null		
	zero = movie->userData->data + movie->userData->size;
	zero[0] = zero[1] = zero[2] = zero[3] = 0;
	
bail:
	return err;
}

QTErr selectiveMetaUserDataAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt32 d[2];
	UInt32 sizeNow = sizeof(QTUserDataRecord) + movie->userData->size + sizeof(UInt32);

	err = (movie->reader)(movie->readerRefCon, d, offset - 8, 8);
	if (err) return err;

	d[0] = QTReaderToNative32(d[0]);
	d[1] = QTReaderToNative32(d[1]);

	if ('free' == d[1])
		;
	else if ('ilst' != d[1]) {
		// append this atom to the userdata
		UInt32 sizeNow = sizeof(QTUserDataRecord) + movie->userData->size + sizeof(UInt32);

		err = qtMovieRealloc(movie, &movie->userData, sizeNow, sizeNow + (UInt32)size + 8, NULL);
		if (err) goto bail;

		err = (movie->reader)(movie->readerRefCon, movie->userData->data + movie->userData->size, offset - 8, (UInt32)size + 8);
		if (err) goto bail;
		
		movie->userData->size += (UInt32)(size + 8);
	}
	else {
		// selectively parse the ilst
		QTMovieAtomWalkersRecord walkers[] = {{kQTMovieWalkAnyAtom, NULL}, {0,0}};
		UInt32 originalSize;

		walkers[0].walker = selectiveILSTUserDataAtom;
		err = qtMovieRealloc(movie, &movie->userData, sizeNow, sizeNow + 8, NULL);
		if (err) goto bail;

		err = (movie->reader)(movie->readerRefCon, movie->userData->data + movie->userData->size, offset - 8, 8);
		if (err) goto bail;
		
		originalSize = movie->userData->size;
		movie->userData->size += 8;

		err = QTMovieWalkAtoms(movie, offset, size, walkers);
		if (err) goto bail;

		// update ilst size atom
		sizeNow = movie->userData->size - originalSize;
		sizeNow = QTReaderToBigEndian32(sizeNow);
		qtMemMove(movie->userData->data + originalSize, &sizeNow, 4);
	}


bail:
	return err;
}

QTErr selectiveILSTUserDataAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt32 d[2];

	err = (movie->reader)(movie->readerRefCon, d, offset - 8, 8);
	if (err) return err;

	d[0] = QTReaderToNative32(d[0]);
	d[1] = QTReaderToNative32(d[1]);

	if (kiTunesUserDataCoverArt != d[1]) {
		// append this atom to the userdata
		UInt32 sizeNow = sizeof(QTUserDataRecord) + movie->userData->size + sizeof(UInt32);

		err = qtMovieRealloc(movie, &movie->userData, sizeNow, sizeNow + (UInt32)size + 8, NULL);
		if (err) goto bail;

		err = (movie->reader)(movie->readerRefCon, movie->userData->data + movie->userData->size, offset - 8, (UInt32)size + 8);
		if (err) goto bail;
		
		movie->userData->size += (UInt32)(size + 8);
	}

bail:
	return err;
}

#endif // QT_READER_SKIP_COVER_IN_MINIMAL

#if QT_READER_ZLIB

static void *gzAlloc(void *state, uInt items, uInt size);
static void gzFree(void *state, void *mem);
static QTErr memoryReader(void *refCon, void *data, QTFileOffset offset, UInt32 dataSize);

QTErr qtMovieCompressedMovieAtom(QTMovie movie, QTFileOffset offset, QTFileOffset size)
{
	QTErr err;
	UInt32 d[5];
	unsigned char *compressedData = NULL, *uncompressedData = NULL;
	UInt32 uncompressedSize;
	z_stream zlib;
	Boolean zlibOpen = false;
	int result;
	void *saveReader, *saveReaderRefCon;

	err = (movie->reader)(movie->readerRefCon, d, offset, 5 * 4);
	if (err) return err;

	d[0] = QTReaderToNative32(d[0]);
	d[1] = QTReaderToNative32(d[1]);
	d[2] = QTReaderToNative32(d[2]);
	d[3] = QTReaderToNative32(d[3]);
	d[4] = QTReaderToNative32(d[4]);
	
	if ((12 != d[0]) || ('dcom' != d[1]) || ('zlib' != d[2]) || ('cmvd' != d[4]))
		return kFskErrUnimplemented;

	err = QTLoadData(movie, offset + 20, d[3] - 8, &compressedData);
	if (err) return err;

	uncompressedSize = QTReaderToNative32(*(UInt32 *)compressedData);

	err = qtMovieMalloc(movie, uncompressedSize, &uncompressedData);
	if (err) goto bail;

	zlib.zalloc = gzAlloc;
	zlib.zfree = gzFree;
	zlib.opaque = NULL;
	if (Z_OK != inflateInit2(&zlib, MAX_WBITS)) {
		err = kFskErrBadData;;
		goto bail;
	}
	zlibOpen = true;

	zlib.total_in = d[3] - 12;
	zlib.next_in = compressedData + 4;
	zlib.avail_in = d[3] - 12;
	zlib.next_out	= (Bytef *)uncompressedData;
	zlib.avail_out	= uncompressedSize;
	zlib.total_out	= 0;
	result = inflate(&zlib, Z_PARTIAL_FLUSH);
	if ((Z_OK != result) && (Z_STREAM_END != result)) {
		err = kFskErrOperationFailed;
		goto bail;
	}

	if ('moov' != QTReaderToNative32(*(UInt32 *)(uncompressedData + 4))) {
		err = kFskErrBadData;
		goto bail;
	}

	if (QTReaderToNative32(*(UInt32 *)uncompressedData) > uncompressedSize) {
		err = kFskErrBadData;
		goto bail;
	}

	saveReader = movie->reader;
	saveReaderRefCon = movie->readerRefCon;

	movie->reader = memoryReader;
	movie->readerRefCon = uncompressedData;

	err = qtMovieAtom(movie, 8, QTReaderToNative32(*(UInt32 *)(uncompressedData)) -8);

	movie->reader = saveReader;
	movie->readerRefCon = saveReaderRefCon;

bail:
	if (zlibOpen)
		inflateEnd(&zlib);
	qtMovieFree(movie, compressedData);
	qtMovieFree(movie, uncompressedData);

	return err;
}

void *gzAlloc(void *state, uInt items, uInt size)
{
	return FskMemPtrAlloc(items * size);
}

void gzFree(void *state, void *mem)
{
	FskMemPtrDispose(mem);
}

QTErr memoryReader(void *refCon, void *data, QTFileOffset offset, UInt32 dataSize)
{
	qtMemMove(data, (unsigned char *)refCon + offset, dataSize);

	return 0;
}


#endif

// sloppy implementation
Boolean QTESDSScanAudio(unsigned char *esds, UInt32 count, UInt8 *codec, UInt32 *audioType, UInt32 *sampleRate, UInt32 *channelCount)
{
	const UInt32 sampleRates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000};

	while (count-- && (4 != *esds))
		esds += 1;

	if (4 == *esds++) {
		while (*esds++ & 0x80)
			;
		*codec = *esds++;

		esds += 12;
		if (0x05 == *esds++) {
			while (*esds++ & 0x80)
				;
			*audioType  = (esds[0]>>3)&0x1f;
			*sampleRate = sampleRates[((esds[0] & 0x07) << 1) | ((esds[1] & 0x80) >> 7)];
			*channelCount = (esds[1] & 0x78) >> 3;
			return true;
		}
	}

	return false;
}

/*
typedef struct ALACSpecificConfig
{
	uint32_t	frameLength;
	uint8_t		compatibleVersion;
	uint8_t		bitDepth;
	uint8_t		pb;
	uint8_t		mb;
	uint8_t		kb;
	uint8_t		numChannels;
	uint16_t	maxRun;
	uint32_t	maxFrameBytes;
	uint32_t	avgBitRate;
	uint32_t	sampleRate;

} ALACSpecificConfig;
*/
Boolean QTALACScanAudio(unsigned char *alac, UInt32 count, UInt32 *frameLength, UInt8 *bitDepth, UInt8 *numChannels, UInt32 *maxFrameBytes, UInt32 *avgBitRate, UInt32 *sampleRate)
{
    *frameLength = QTReaderMisaligned32_GetBtoN(alac);alac+=4;
    alac++;      // compatibleVersion
    *bitDepth = *alac++;
    alac++; //pb
    alac++; //mb
    alac++; //kb
    *numChannels = *alac++;
    alac+=2; //maxRun
    *maxFrameBytes = QTReaderMisaligned32_GetBtoN(alac);alac+=4;
    *avgBitRate = QTReaderMisaligned32_GetBtoN(alac);alac+=4;
    *sampleRate = QTReaderMisaligned32_GetBtoN(alac);

	return true;
}

typedef struct {
	unsigned char	*data;
	int				position;
} esdsScanRecord, *esdsScan;

static unsigned long getMSBPosition(unsigned long input, unsigned long bitLength);
static unsigned long getBits(esdsScan bits, int count);

// sloppy implementation
Boolean QTESDSScanVideo(unsigned char *esds, UInt32 esdsSize, UInt32 *width, UInt32 *height, UInt8 *profile_level)
{
	UInt32 start = 0;
	esdsScanRecord bits;
	long shape, ver_id, timeIncRes;

	*profile_level = 0;
	
	// find the video object start code
	start = (esds[0] << 24) | (esds[1] << 16) | (esds[2] << 8) | esds[3];
	esds += 4; esdsSize -= 4;
	while (esdsSize) {
		if( 0x01b0 == start )
			*profile_level = esds[0];
		
		if ((0x0120 <= start) && (start <= 0x012f))
			break;

		start = (start << 8) | *esds++;
		esdsSize -= 1;
	}

	if (!esdsSize)
		return false;

	// parse those bits
	bits.data = esds;
	bits.position = 0;

	getBits(&bits, 1);						// random accessible
	getBits(&bits, 8);						// object type
	if (getBits(&bits, 1)) {				// is identifier
		ver_id = getBits(&bits, 4);			// ver_id
		getBits(&bits, 3);					// priority
	}
	else
		ver_id = 1;		//@@
	if (15 == getBits(&bits, 4))			// aspect ratio
		getBits(&bits, 16);
	if (getBits(&bits, 1)) {				// vol control parameters
		getBits(&bits, 3);					// chroma format, low delay
		if (getBits(&bits, 1)) {			// vbv parameters present
			getBits(&bits, 15);					// first half bit rate
			if (!getBits(&bits, 1)) goto bail;	// marker
			getBits(&bits, 15);					// other half bit rate
			if (!getBits(&bits, 1)) goto bail;	// marker

			getBits(&bits, 15);					// first part vbv buffer size
			if (!getBits(&bits, 1)) goto bail;	// marker
			getBits(&bits, 3);					// other part vbv buffer size

			getBits(&bits, 11);					// first part vbv occupancy
			if (!getBits(&bits, 1)) goto bail;	// marker
			getBits(&bits, 15);					// other part vbv occupancy
			if (!getBits(&bits, 1)) goto bail;	// marker
		}
	}
	shape = getBits(&bits, 2);				// shape
	if ((3 == shape) && (1 != ver_id ))
		getBits(&bits, 4);					// shape extension

	if (!getBits(&bits, 1)) goto bail;		// marker
	timeIncRes = getBits(&bits, 16);		// time increment resolution
	if (!getBits(&bits, 1)) goto bail;		// marker
	if (getBits(&bits, 1)) {				// fixed vop rate
		long inc = getMSBPosition(timeIncRes - 1, 16);
		if (!inc) inc = 1;
		getBits(&bits, inc);
	}

	if (0 == shape) {						// rectangular
		if (!getBits(&bits, 1)) goto bail;	// marker
	    *width = getBits(&bits, 13);		// width
		if (!getBits(&bits, 1)) goto bail;	// marker
	    *height = getBits(&bits, 13);		// height
		return true;
	}

bail:
	return false;
}

unsigned long getBits(esdsScan bits, int count)
{
	unsigned long result = 0;

	while (count--) {
		result <<= 1;
		if (*(bits->data) & (1 << (7 - bits->position)))
			result |= 1;
		bits->position += 1;
		if (8 == bits->position) {
			bits->position = 0;
			bits->data += 1;
		}
	}

	return result;
}

unsigned long getMSBPosition(unsigned long input, unsigned long bitLength)
{
	unsigned long i;

	for (i=1; i<=bitLength; i++) {
		unsigned long mask = 1 << (bitLength - i);
		if (input & mask)
			return bitLength - i + 1;
	}

	return 0;
}

//debug only
//static int Bytes_used = 0;
//static int bitss_used = 0;
//Bytes_used+=byteOffset;
//bitss_used =bitOffset;

//parsing code to get AAC level and SBR
#define ShowBits(value, numBits) value=( bits32>>(32-(numBits)) );
#define PopBits(ptr, value, numBits)								\
{																	\
	unsigned int byteOffset;										\
	unsigned int tmpUINT;											\
																	\
	value=( bits32>>(32-(numBits)) );								\
																	\
	if(numBits==0) value=0;											\
																	\
	tmpUINT    =bitOffset+(numBits);								\
	byteOffset =(tmpUINT>>3); bitOffset=(tmpUINT&0x7);				\
	ptr		  +=byteOffset;											\
	bits32     =(ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|(ptr[3]<<0);	\
	bits32	 <<=bitOffset;											\
}
#define SkipBits(ptr, numBits)                                      \
{																	\
    unsigned int byteOffset;										\
    unsigned int tmpUINT;											\
                                                                    \
    tmpUINT    =bitOffset+(numBits);								\
    byteOffset =(tmpUINT>>3); bitOffset=(tmpUINT&0x7);				\
    ptr		  +=byteOffset;											\
    bits32     =(ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|(ptr[3]<<0);	\
    bits32	 <<=bitOffset;											\
}

static unsigned char hcb_sf[][2] = {
    { /*  0 */  1, 2 },
    { /*  1 */  60, 0 },
    { /*  2 */  1, 2 },
    { /*  3 */  2, 3 },
    { /*  4 */  3, 4 },
    { /*  5 */  59, 0 },
    { /*  6 */  3, 4 },
    { /*  7 */  4, 5 },
    { /*  8 */  5, 6 },
    { /*  9 */  61, 0 },
    { /* 10 */  58, 0 },
    { /* 11 */  62, 0 },
    { /* 12 */  3, 4 },
    { /* 13 */  4, 5 },
    { /* 14 */  5, 6 },
    { /* 15 */  57, 0 },
    { /* 16 */  63, 0 },
    { /* 17 */  4, 5 },
    { /* 18 */  5, 6 },
    { /* 19 */  6, 7 },
    { /* 20 */  7, 8 },
    { /* 21 */  56, 0 },
    { /* 22 */  64, 0 },
    { /* 23 */  55, 0 },
    { /* 24 */  65, 0 },
    { /* 25 */  4, 5 },
    { /* 26 */  5, 6 },
    { /* 27 */  6, 7 },
    { /* 28 */  7, 8 },
    { /* 29 */  66, 0 },
    { /* 30 */  54, 0 },
    { /* 31 */  67, 0 },
    { /* 32 */  5, 6 },
    { /* 33 */  6, 7 },
    { /* 34 */  7, 8 },
    { /* 35 */  8, 9 },
    { /* 36 */  9, 10 },
    { /* 37 */  53, 0 },
    { /* 38 */  68, 0 },
    { /* 39 */  52, 0 },
    { /* 40 */  69, 0 },
    { /* 41 */  51, 0 },
    { /* 42 */  5, 6 },
    { /* 43 */  6, 7 },
    { /* 44 */  7, 8 },
    { /* 45 */  8, 9 },
    { /* 46 */  9, 10 },
    { /* 47 */  70, 0 },
    { /* 48 */  50, 0 },
    { /* 49 */  49, 0 },
    { /* 50 */  71, 0 },
    { /* 51 */  6, 7 },
    { /* 52 */  7, 8 },
    { /* 53 */  8, 9 },
    { /* 54 */  9, 10 },
    { /* 55 */  10, 11 },
    { /* 56 */  11, 12 },
    { /* 57 */  72, 0 },
    { /* 58 */  48, 0 },
    { /* 59 */  73, 0 },
    { /* 60 */  47, 0 },
    { /* 61 */  74, 0 },
    { /* 62 */  46, 0 },
    { /* 63 */  6, 7 },
    { /* 64 */  7, 8 },
    { /* 65 */  8, 9 },
    { /* 66 */  9, 10 },
    { /* 67 */  10, 11 },
    { /* 68 */  11, 12 },
    { /* 69 */  76, 0 },
    { /* 70 */  75, 0 },
    { /* 71 */  77, 0 },
    { /* 72 */  78, 0 },
    { /* 73 */  45, 0 },
    { /* 74 */  43, 0 },
    { /* 75 */  6, 7 },
    { /* 76 */  7, 8 },
    { /* 77 */  8, 9 },
    { /* 78 */  9, 10 },
    { /* 79 */  10, 11 },
    { /* 80 */  11, 12 },
    { /* 81 */  44, 0 },
    { /* 82 */  79, 0 },
    { /* 83 */  42, 0 },
    { /* 84 */  41, 0 },
    { /* 85 */  80, 0 },
    { /* 86 */  40, 0 },
    { /* 87 */  6, 7 },
    { /* 88 */  7, 8 },
    { /* 89 */  8, 9 },
    { /* 90 */  9, 10 },
    { /* 91 */  10, 11 },
    { /* 92 */  11, 12 },
    { /* 93 */  81, 0 },
    { /* 94 */  39, 0 },
    { /* 95 */  82, 0 },
    { /* 96 */  38, 0 },
    { /* 97 */  83, 0 },
    { /* 98 */  7, 8 },
    { /* 99 */  8, 9 },
    { /* 00 */  9, 10 },
    { /* 01 */  10, 11 },
    { /* 02 */  11, 12 },
    { /* 03 */  12, 13 },
    { /* 04 */  13, 14 },
    { /* 05 */  37, 0 },
    { /* 06 */  35, 0 },
    { /* 07 */  85, 0 },
    { /* 08 */  33, 0 },
    { /* 09 */  36, 0 },
    { /* 10 */  34, 0 },
    { /* 11 */  84, 0 },
    { /* 12 */  32, 0 },
    { /* 13 */  6, 7 },
    { /* 14 */  7, 8 },
    { /* 15 */  8, 9 },
    { /* 16 */  9, 10 },
    { /* 17 */  10, 11 },
    { /* 18 */  11, 12 },
    { /* 19 */  87, 0 },
    { /* 20 */  89, 0 },
    { /* 21 */  30, 0 },
    { /* 22 */  31, 0 },
    { /* 23 */  8, 9 },
    { /* 24 */  9, 10 },
    { /* 25 */  10, 11 },
    { /* 26 */  11, 12 },
    { /* 27 */  12, 13 },
    { /* 28 */  13, 14 },
    { /* 29 */  14, 15 },
    { /* 30 */  15, 16 },
    { /* 31 */  86, 0 },
    { /* 32 */  29, 0 },
    { /* 33 */  26, 0 },
    { /* 34 */  27, 0 },
    { /* 35 */  28, 0 },
    { /* 36 */  24, 0 },
    { /* 37 */  88, 0 },
    { /* 38 */  9, 10 },
    { /* 39 */  10, 11 },
    { /* 40 */  11, 12 },
    { /* 41 */  12, 13 },
    { /* 42 */  13, 14 },
    { /* 43 */  14, 15 },
    { /* 44 */  15, 16 },
    { /* 45 */  16, 17 },
    { /* 46 */  17, 18 },
    { /* 47 */  25, 0 },
    { /* 48 */  22, 0 },
    { /* 49 */  23, 0 },
    { /* 50 */  15, 16 },
    { /* 51 */  16, 17 },
    { /* 52 */  17, 18 },
    { /* 53 */  18, 19 },
    { /* 54 */  19, 20 },
    { /* 55 */  20, 21 },
    { /* 56 */  21, 22 },
    { /* 57 */  22, 23 },
    { /* 58 */  23, 24 },
    { /* 59 */  24, 25 },
    { /* 60 */  25, 26 },
    { /* 61 */  26, 27 },
    { /* 62 */  27, 28 },
    { /* 63 */  28, 29 },
    { /* 64 */  29, 30 },
    { /* 65 */  90, 0 },
    { /* 66 */  21, 0 },
    { /* 67 */  19, 0 },
    { /* 68 */   3, 0 },
    { /* 69 */   1, 0 },
    { /* 70 */   2, 0 },
    { /* 71 */   0, 0 },
    { /* 72 */  23, 24 },
    { /* 73 */  24, 25 },
    { /* 74 */  25, 26 },
    { /* 75 */  26, 27 },
    { /* 76 */  27, 28 },
    { /* 77 */  28, 29 },
    { /* 78 */  29, 30 },
    { /* 79 */  30, 31 },
    { /* 80 */  31, 32 },
    { /* 81 */  32, 33 },
    { /* 82 */  33, 34 },
    { /* 83 */  34, 35 },
    { /* 84 */  35, 36 },
    { /* 85 */  36, 37 },
    { /* 86 */  37, 38 },
    { /* 87 */  38, 39 },
    { /* 88 */  39, 40 },
    { /* 89 */  40, 41 },
    { /* 90 */  41, 42 },
    { /* 91 */  42, 43 },
    { /* 92 */  43, 44 },
    { /* 93 */  44, 45 },
    { /* 94 */  45, 46 },
    { /* 95 */   98, 0 },
    { /* 96 */   99, 0 },
    { /* 97 */  100, 0 },
    { /* 98 */  101, 0 },
    { /* 99 */  102, 0 },
    { /* 00 */  117, 0 },
    { /* 01 */   97, 0 },
    { /* 02 */   91, 0 },
    { /* 03 */   92, 0 },
    { /* 04 */   93, 0 },
    { /* 05 */   94, 0 },
    { /* 06 */   95, 0 },
    { /* 07 */   96, 0 },
    { /* 08 */  104, 0 },
    { /* 09 */  111, 0 },
    { /* 10 */  112, 0 },
    { /* 11 */  113, 0 },
    { /* 12 */  114, 0 },
    { /* 13 */  115, 0 },
    { /* 14 */  116, 0 },
    { /* 15 */  110, 0 },
    { /* 16 */  105, 0 },
    { /* 17 */  106, 0 },
    { /* 18 */  107, 0 },
    { /* 19 */  108, 0 },
    { /* 20 */  109, 0 },
    { /* 21 */  118, 0 },
    { /* 22 */    6, 0 },
    { /* 23 */    8, 0 },
    { /* 24 */    9, 0 },
    { /* 25 */   10, 0 },
    { /* 26 */    5, 0 },
    { /* 27 */  103, 0 },
    { /* 28 */  120, 0 },
    { /* 29 */  119, 0 },
    { /* 30 */    4, 0 },
    { /* 31 */    7, 0 },
    { /* 32 */   15, 0 },
    { /* 33 */   16, 0 },
    { /* 34 */   18, 0 },
    { /* 35 */   20, 0 },
    { /* 36 */   17, 0 },
    { /* 37 */   11, 0 },
    { /* 38 */   12, 0 },
    { /* 39 */   14, 0 },
    { /* 40 */   13, 0 }
};

static const unsigned short swb_offset_1024_96[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
    64, 72, 80, 88, 96, 108, 120, 132, 144, 156, 172, 188, 212, 240,
    276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024
};
static const unsigned short swb_offset_1024_64[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
    64, 72, 80, 88, 100, 112, 124, 140, 156, 172, 192, 216, 240, 268,
    304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 784, 824,
    864, 904, 944, 984, 1024
};
static const unsigned short swb_offset_1024_48[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
    80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
    768, 800, 832, 864, 896, 928, 1024
};
static const unsigned short swb_offset_1024_32[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
    80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
    768, 800, 832, 864, 896, 928, 960, 992, 1024
};
static const unsigned short swb_offset_1024_24[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68,
    76, 84, 92, 100, 108, 116, 124, 136, 148, 160, 172, 188, 204, 220,
    240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652, 704,
    768, 832, 896, 960, 1024
};
static const unsigned short swb_offset_1024_16[] =
{
    0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124,
    136, 148, 160, 172, 184, 196, 212, 228, 244, 260, 280, 300, 320, 344,
    368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832, 896, 960, 1024
};
static const unsigned short swb_offset_1024_8[] =
{
    0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172,
    188, 204, 220, 236, 252, 268, 288, 308, 328, 348, 372, 396, 420, 448,
    476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944, 1024
};

static const unsigned short *swb_offset_1024_window[] =
{
    swb_offset_1024_96,      /* 96000 */
    swb_offset_1024_96,      /* 88200 */
    swb_offset_1024_64,      /* 64000 */
    swb_offset_1024_48,      /* 48000 */
    swb_offset_1024_48,      /* 44100 */
    swb_offset_1024_32,      /* 32000 */
    swb_offset_1024_24,      /* 24000 */
    swb_offset_1024_24,      /* 22050 */
    swb_offset_1024_16,      /* 16000 */
    swb_offset_1024_16,      /* 12000 */
    swb_offset_1024_16,      /* 11025 */
    swb_offset_1024_8        /* 8000  */
};





typedef struct
{
	const unsigned char *buf;
	unsigned int bits32;
	unsigned int bitOffset;
} stream;

typedef struct
{
	unsigned char common_window;
} element;

typedef struct
{
	unsigned char  window_sequence;
	unsigned char  window_shape;
	unsigned char  max_sfb;
	unsigned char  num_swb;
	unsigned char  scale_factor_grouping;
	unsigned char  ms_mask_present;
	unsigned char  num_window_groups;
	unsigned char  window_group_length[8];
	unsigned short sect_sfb_offset[8][15*8];
	unsigned short sfb_cb[8][15*8];
    unsigned short swb_offset[52];
	unsigned short swb_offset_max;
	unsigned char  pred_data_present;
	unsigned char  pulse_data_present;
	unsigned char  tns_data_present;
	unsigned char  gain_control_data_present;
	unsigned char  num_windows;
	unsigned char  num_sec[8];
	unsigned char  ms_used[8][51];
	unsigned char  sect_cb[8][15*8];
	unsigned short sect_start[8][15*8];
    unsigned short sect_end[8][15*8];
} icstream;

int PCU[] = 
{
	0,
	5,	//AAC Main
	3,	//AAC LC
	4,	//AAC SSR
	4,	//AAC LTP
	3	//SBR
	//...
};

float RCU[] = 
{
	0,
	5,	//AAC Main
	3,	//AAC LC
	3,	//AAC SSR
	4,	//AAC LTP
	2.5 //SBR
	//...
};

static int guess_aac_level(int profile_ID, int chnl_num, int sple_rte, int pcu_max, int rcu_max)
{
	int level;
	
	if (profile_ID == 2)
	{
		if (chnl_num <= 2 && sple_rte <= 24000 && pcu_max <= 3 && rcu_max <= 5)
			level = 1;
		else if (chnl_num <= 2 && sple_rte <= 48000 && pcu_max <= 6 && rcu_max <= 5)
			level = 2;
		else if (chnl_num <= 5 && sple_rte <= 48000 && pcu_max <= 19 && rcu_max <= 15)
			level = 4;
		else if (chnl_num <= 5 && sple_rte <= 96000 && pcu_max <= 38 && rcu_max <= 15)
			level = 5;
		else {
			level = 0;
		}
	}
	else if (profile_ID == 5)
	{
		if (chnl_num <= 2 && sple_rte <= 48000 && pcu_max <= 9 && rcu_max <= 10)
			level = 2;
		else if (chnl_num <= 2 && sple_rte <= 48000 && pcu_max <= 15 && rcu_max <= 10)
			level = 3;
		else if (chnl_num <= 5 && sple_rte <= 48000 && pcu_max <= 25 && rcu_max <= 28)
			level = 4;
		else if (chnl_num <= 5 && sple_rte <= 96000 && pcu_max <= 49 && rcu_max <= 28)
			level = 5;
		else {
			level = 0;
		}
	}
	else
	{
		level = 0; //no recognized
	}
	
	return level;
}


/* AAC Parsing */
/* First object type that has ER */
#define ER_OBJECT_START 17

/* Bitstream */
#define LEN_SE_ID 3
#define LEN_TAG   4
#define LEN_BYTE  8

#define ONLY_LONG_SEQUENCE   0x0
#define LONG_START_SEQUENCE  0x1
#define EIGHT_SHORT_SEQUENCE 0x2
#define LONG_STOP_SEQUENCE   0x3

#define ZERO_HCB       0
#define FIRST_PAIR_HCB 5
#define ESC_HCB        11
#define QUAD_LEN       4
#define PAIR_LEN       2
#define NOISE_HCB      13
#define INTENSITY_HCB2 14
#define INTENSITY_HCB  15

#define EXT_FIL            0
#define EXT_FILL_DATA      1
#define EXT_DATA_ELEMENT   2
#define EXT_DYNAMIC_RANGE 11
#define ANC_DATA           0

/* Syntax elements */
#define ID_SCE 0x0
#define ID_CPE 0x1
#define ID_CCE 0x2
#define ID_LFE 0x3
#define ID_DSE 0x4
#define ID_PCE 0x5
#define ID_FIL 0x6
#define ID_END 0x7

/* SBR type */
#define EXT_SBR_DATA     13
#define EXT_SBR_DATA_CRC 14

#define READER_VAR                  \
    const unsigned char *s;         \
    unsigned int bits32;            \
    unsigned int bitOffset;
//save stream vialable
#define STORE_S                     \
    ld->buf = s;                    \
    ld->bits32 = bits32;            \
    ld->bitOffset = bitOffset;
//reset steam vialable
#define RESET_S                     \
    s = ld->buf;                    \
    bits32 = ld->bits32;            \
    bitOffset = ld->bitOffset;

#define bit_set(A, B) ((A) & (1<<(B)))
static int window_grouping_info(icstream *ics, int sf_index)
{
	static const unsigned char num_swb_1024_window[] = { 41, 41, 47, 49, 49, 51, 47, 47, 43, 43, 43, 40 };
	static const unsigned char num_swb_128_window[] = { 12, 12, 12, 14, 14, 14, 15, 15, 15, 15, 15, 15 };
	static const unsigned short swb_offset_128_96[] = { 0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128 };
	static const unsigned short swb_offset_128_64[] = { 0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128 };
	static const unsigned short swb_offset_128_48[] = { 0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 128 };
	static const unsigned short swb_offset_128_24[] = { 0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 128 };
	static const unsigned short swb_offset_128_16[] = { 0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 128 };
	static const unsigned short swb_offset_128_8[]  = { 0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108, 128 };
	static const unsigned short *swb_offset_128_window[] =
	{
		swb_offset_128_96,       /* 96000 */
		swb_offset_128_96,       /* 88200 */
		swb_offset_128_64,       /* 64000 */
		swb_offset_128_48,       /* 48000 */
		swb_offset_128_48,       /* 44100 */
		swb_offset_128_48,       /* 32000 */
		swb_offset_128_24,       /* 24000 */
		swb_offset_128_24,       /* 22050 */
		swb_offset_128_16,       /* 16000 */
		swb_offset_128_16,       /* 12000 */
		swb_offset_128_16,       /* 11025 */
		swb_offset_128_8         /* 8000  */
	};
	unsigned int frameLength = 1024;
	unsigned char i, g;
	
	switch (ics->window_sequence) {
		case ONLY_LONG_SEQUENCE:
		case LONG_START_SEQUENCE:
		case LONG_STOP_SEQUENCE:
			ics->num_windows = 1;
			ics->num_window_groups = 1;
			ics->window_group_length[ics->num_window_groups-1] = 1;
			
			//object type = 2
			if (frameLength == 1024) {
				ics->num_swb = num_swb_1024_window[sf_index];
			}
			
			if (ics->max_sfb > ics->num_swb) {
				return 32;
			}
			
			for (i=0; i<ics->num_swb; i++) {
				ics->swb_offset[i] = ics->sect_sfb_offset[0][i] = swb_offset_1024_window[sf_index][i];
			}
			ics->sect_sfb_offset[0][ics->num_swb] = ics->swb_offset[ics->num_swb] = ics->swb_offset_max = frameLength;
			return 0;
			
		case EIGHT_SHORT_SEQUENCE:
			ics->num_windows = 8;
			ics->num_window_groups = 1;
			ics->window_group_length[ics->num_window_groups-1] = 1;
			
			ics->num_swb = num_swb_128_window[sf_index];
			
			if (ics->max_sfb > ics->num_swb) {
				return 32;
			}
			
			for (i = 0; i < ics->num_swb; i++)
				ics->swb_offset[i] = swb_offset_128_window[sf_index][i];
			ics->swb_offset[ics->num_swb] = ics->swb_offset_max = frameLength/8;
			
			for (i = 0; i < ics->num_windows-1; i++) {
				if (bit_set(ics->scale_factor_grouping, 6-i) == 0)
				{
					ics->num_window_groups += 1;
					ics->window_group_length[ics->num_window_groups-1] = 1;
				} else {
					ics->window_group_length[ics->num_window_groups-1] += 1;
				}
			}
			
			/* preparation of sect_sfb_offset for short blocks */
			for (g = 0; g < ics->num_window_groups; g++)
			{
				unsigned short width;
				unsigned char sect_sfb = 0;
				unsigned short offset = 0;
				
				for (i = 0; i < ics->num_swb; i++)
				{
					if (i+1 == ics->num_swb)
					{
						width = (frameLength/8) - swb_offset_128_window[sf_index][i];
					} else {
						width = swb_offset_128_window[sf_index][i+1] -
                        swb_offset_128_window[sf_index][i];
					}
					width *= ics->window_group_length[g];
					ics->sect_sfb_offset[g][sect_sfb++] = offset;
					offset += width;
				}
				ics->sect_sfb_offset[g][sect_sfb] = offset;
			}
			return 0;
			
		default:
			return 32;
	}

	return 0;
}

//raw_data_block
static int fill_element(stream *ld, SInt32 *sbr_present_flag )
{
	unsigned short /*i, */tmp, count/*, dataElementLength*/;
	unsigned char  /*align=4, */bs_extension_type/*, extension_type, data_element_version, dataElementLengthPart, loopCounter*/;
	READER_VAR
	RESET_S
	
	PopBits(s, count, 4);
	if (count == 15)
	{
		PopBits(s, tmp, 8);
		count += tmp-1 ;
	}
	if (count > 0)
	{
		PopBits(s, bs_extension_type, 4);
		if (bs_extension_type==EXT_SBR_DATA || bs_extension_type==EXT_SBR_DATA_CRC)
		{
			*sbr_present_flag = 1;
		}
		/*else //extension_payload
		{
			while (count > 0) {
				//extension payload
				PopBits(s, extension_type, 4);
				switch (extension_type) {
					case EXT_DYNAMIC_RANGE:
						;//dynamic_range_info
						break;
						
					case EXT_FILL_DATA:
						PopBits(s, tmp, 4);		//'0000'
						for(i=0; i<count-1; i++)
							PopBits(s, tmp, 8); //fill_bype, '10100101'
						break;

					case EXT_DATA_ELEMENT:
						PopBits(s, data_element_version, 4);
						if (data_element_version == ANC_DATA) {
							loopCounter = 0;
							dataElementLength = 0;
							do {
								PopBits(s, dataElementLengthPart, 8);
								dataElementLength += dataElementLengthPart;
								loopCounter ++;
							}while (dataElementLengthPart == 255);
							
							for (i=0; i<dataElementLength; i++) {
								PopBits(s, tmp, 8);
							}
						}
						else {
							align = 0;
						}
					
					case EXT_FIL:
					default:
						PopBits(s, tmp, align); //align
						for(i=0; i<count-1; i++)
							PopBits(s, tmp, 8); //fill_bype
						break;
				}
			}
		}*/
	}
	
	STORE_S
	
	return 0;
}

/* huffman decoding structure */
/* 1st step table */
typedef struct
{
    unsigned char offset;
    unsigned char extra_bits;
} hcb;

/* 2nd step table with quadruple data */
typedef struct
{
    unsigned char bits;
    char x;
    char y;
} hcb_2_pair;

typedef struct
{
    unsigned char is_leaf;
    char data[2];
} hcb_bin_pair;

typedef struct
{
    unsigned char bits;
    char x;
    char y;
    char v;
    char w;
} hcb_2_quad;

/* binary search table */
typedef struct
{
    unsigned char is_leaf;
    char data[4];
} hcb_bin_quad;

/* table */
unsigned char hcbN[] = { 0, 5, 5, 0, 5, 0, 5, 0, 5, 0, 6, 5 };

// 1st step table
static hcb hcb1_1[] = {
    { /* 00000 */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /* 10000 */ 1, 0 },
    { /* 10001 */ 2, 0 },
    { /* 10010 */ 3, 0 },
    { /* 10011 */ 4, 0 },
    { /* 10100 */ 5, 0 },
    { /* 10101 */ 6, 0 },
    { /* 10110 */ 7, 0 },
    { /* 10111 */ 8, 0 },
	
    /* 7 bit codewords */
    { /* 11000 */ 9,  2 },
    { /* 11001 */ 13, 2 },
    { /* 11010 */ 17, 2 },
    { /* 11011 */ 21, 2 },
    { /* 11100 */ 25, 2 },
    { /* 11101 */ 29, 2 },
	
    /* 9 bit codewords */
    { /* 11110 */ 33, 4 },
	
    /* 9/10/11 bit codewords */
    { /* 11111 */ 49, 6 }
};
static hcb hcb2_1[] = {
    { /* 00000 */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /* 00100 */ 1, 0 },
    { /*       */ 1, 0 },
    { /* 00110 */ 2, 0 },
    { /* 00111 */ 3, 0 },
    { /* 01000 */ 4, 0 },
    { /* 01001 */ 5, 0 },
    { /* 01010 */ 6, 0 },
    { /* 01011 */ 7, 0 },
    { /* 01100 */ 8, 0 },
	
    /* 6 bit codewords */
    { /* 01101 */ 9,  1 },
    { /* 01110 */ 11, 1 },
    { /* 01111 */ 13, 1 },
    { /* 10000 */ 15, 1 },
    { /* 10001 */ 17, 1 },
    { /* 10010 */ 19, 1 },
    { /* 10011 */ 21, 1 },
    { /* 10100 */ 23, 1 },
    { /* 10101 */ 25, 1 },
    { /* 10110 */ 27, 1 },
    { /* 10111 */ 29, 1 },
    { /* 11000 */ 31, 1 },
	
    /* 7 bit codewords */
    { /* 11001 */ 33, 2 },
    { /* 11010 */ 37, 2 },
    { /* 11011 */ 41, 2 },
	
    /* 7/8 bit codewords */
    { /* 11100 */ 45, 3 },
	
    /* 8 bit codewords */
    { /* 11101 */ 53, 3 },
    { /* 11110 */ 61, 3 },
	
    /* 8/9 bit codewords */
    { /* 11111 */ 69, 4 }
};
static hcb hcb4_1[] = {
    /* 4 bit codewords */
    { /* 00000 */ 0, 0 },
    { /*       */ 0, 0 },
    { /* 00010 */ 1, 0 },
    { /*       */ 1, 0 },
    { /* 00100 */ 2, 0 },
    { /*       */ 2, 0 },
    { /* 00110 */ 3, 0 },
    { /*       */ 3, 0 },
    { /* 01000 */ 4, 0 },
    { /*       */ 4, 0 },
    { /* 01010 */ 5, 0 },
    { /*       */ 5, 0 },
    { /* 01100 */ 6, 0 },
    { /*       */ 6, 0 },
    { /* 01110 */ 7, 0 },
    { /*       */ 7, 0 },
    { /* 10000 */ 8, 0 },
    { /*       */ 8, 0 },
    { /* 10010 */ 9, 0 },
    { /*       */ 9, 0 },
	
    /* 5 bit codewords */
    { /* 10100 */ 10, 0 },
    { /* 10101 */ 11, 0 },
    { /* 10110 */ 12, 0 },
    { /* 10111 */ 13, 0 },
    { /* 11000 */ 14, 0 },
    { /* 11001 */ 15, 0 },
	
    /* 7 bit codewords */
    { /* 11010 */ 16, 2 },
    { /* 11011 */ 20, 2 },
	
    /* 7/8 bit codewords */
    { /* 11100 */ 24, 3 },
	
    /* 8 bit codewords */
    { /* 11101 */ 32, 3 },
	
    /* 8/9 bit codewords */
    { /* 11110 */ 40, 4 },
	
    /* 9/10/11/12 bit codewords */
    { /* 11111 */ 56, 7 }
};
static hcb hcb6_1[] = {
    /* 4 bit codewords */
    { /* 00000 */ 0, 0 },
    { /*       */ 0, 0 },
    { /* 00010 */ 1, 0 },
    { /*       */ 1, 0 },
    { /* 00100 */ 2, 0 },
    { /*       */ 2, 0 },
    { /* 00110 */ 3, 0 },
    { /*       */ 3, 0 },
    { /* 01000 */ 4, 0 },
    { /*       */ 4, 0 },
    { /* 01010 */ 5, 0 },
    { /*       */ 5, 0 },
    { /* 01100 */ 6, 0 },
    { /*       */ 6, 0 },
    { /* 01110 */ 7, 0 },
    { /*       */ 7, 0 },
    { /* 10000 */ 8, 0 },
    { /*       */ 8, 0 },
	
    /* 6 bit codewords */
    { /* 10010 */ 9, 1 },
    { /* 10011 */ 11, 1 },
    { /* 10100 */ 13, 1 },
    { /* 10101 */ 15, 1 },
    { /* 10110 */ 17, 1 },
    { /* 10111 */ 19, 1 },
    { /* 11000 */ 21, 1 },
    { /* 11001 */ 23, 1 },
	
    /* 7 bit codewords */
    { /* 11010 */ 25, 2 },
    { /* 11011 */ 29, 2 },
    { /* 11100 */ 33, 2 },
	
    /* 7/8 bit codewords */
    { /* 11101 */ 37, 3 },
	
    /* 8/9 bit codewords */
    { /* 11110 */ 45, 4 },
	
    /* 9/10/11 bit codewords */
    { /* 11111 */ 61, 6 }
};
static hcb hcb8_1[] = {
    /* 3 bit codeword */
    { /* 00000 */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
    { /*       */ 0, 0 },
	
    /* 4 bit codewords */
    { /* 00100 */ 1, 0 },
    { /*       */ 1, 0 },
    { /* 00110 */ 2, 0 },
    { /*       */ 2, 0 },
    { /* 01000 */ 3, 0 },
    { /*       */ 3, 0 },
    { /* 01010 */ 4, 0 },
    { /*       */ 4, 0 },
    { /* 01100 */ 5, 0 },
    { /*       */ 5, 0 },
	
    /* 5 bit codewords */
    { /* 01110 */ 6, 0 },
    { /* 01111 */ 7, 0 },
    { /* 10000 */ 8, 0 },
    { /* 10001 */ 9, 0 },
    { /* 10010 */ 10, 0 },
    { /* 10011 */ 11, 0 },
    { /* 10100 */ 12, 0 },
	
    /* 6 bit codewords */
    { /* 10101 */ 13, 1 },
    { /* 10110 */ 15, 1 },
    { /* 10111 */ 17, 1 },
    { /* 11000 */ 19, 1 },
    { /* 11001 */ 21, 1 },
	
    /* 7 bit codewords */
    { /* 11010 */ 23, 2 },
    { /* 11011 */ 27, 2 },
    { /* 11100 */ 31, 2 },
	
    /* 7/8 bit codewords */
    { /* 11101 */ 35, 3 },
	
    /* 8 bit codewords */
    { /* 11110 */ 43, 3 },
	
    /* 8/9/10 bit codewords */
    { /* 11111 */ 51, 5 }
};
static hcb hcb10_1[] = {
    /* 4 bit codewords */
    { /* 000000 */ 0, 0 },
    { /*        */ 0, 0 },
    { /*        */ 0, 0 },
    { /*        */ 0, 0 },
    { /* 000100 */ 1, 0 },
    { /*        */ 1, 0 },
    { /*        */ 1, 0 },
    { /*        */ 1, 0 },
    { /* 001000 */ 2, 0 },
    { /*        */ 2, 0 },
    { /*        */ 2, 0 },
    { /*        */ 2, 0 },
    /* 5 bit codewords */
    { /* 001100 */ 3, 0 },
    { /*        */ 3, 0 },
    { /* 001110 */ 4, 0 },
    { /*        */ 4, 0 },
    { /* 010000 */ 5, 0 },
    { /*        */ 5, 0 },
    { /* 010010 */ 6, 0 },
    { /*        */ 6, 0 },
    { /* 010100 */ 7, 0 },
    { /*        */ 7, 0 },
    { /* 010110 */ 8, 0 },
    { /*        */ 8, 0 },
    { /* 011000 */ 9, 0 },
    { /*        */ 9, 0 },
    { /* 011010 */ 10, 0 },
    { /*        */ 10, 0 },
    /* 6 bit codewords */
    { /* 011100 */ 11, 0 },
    { /* 011101 */ 12, 0 },
    { /* 011110 */ 13, 0 },
    { /* 011111 */ 14, 0 },
    { /* 100000 */ 15, 0 },
    { /* 100001 */ 16, 0 },
    { /* 100010 */ 17, 0 },
    { /* 100011 */ 18, 0 },
    { /* 100100 */ 19, 0 },
    { /* 100101 */ 20, 0 },
    { /* 100110 */ 21, 0 },
    { /* 100111 */ 22, 0 },
    { /* 101000 */ 23, 0 },
    { /* 101001 */ 24, 0 },
    /* 7 bit codewords */
    { /* 101010 */ 25, 1 },
    { /* 101011 */ 27, 1 },
    { /* 101100 */ 29, 1 },
    { /* 101101 */ 31, 1 },
    { /* 101110 */ 33, 1 },
    { /* 101111 */ 35, 1 },
    { /* 110000 */ 37, 1 },
    { /* 110001 */ 39, 1 },
    /* 7/8 bit codewords */
    { /* 110010 */ 41, 2 },
    /* 8 bit codewords */
    { /* 110011 */ 45, 2 },
    { /* 110100 */ 49, 2 },
    { /* 110101 */ 53, 2 },
    { /* 110110 */ 57, 2 },
    { /* 110111 */ 61, 2 },
    /* 8/9 bit codewords */
    { /* 111000 */ 65, 3 },
    /* 9 bit codewords */
    { /* 111001 */ 73, 3 },
    { /* 111010 */ 81, 3 },
    { /* 111011 */ 89, 3 },
    /* 9/10 bit codewords */
    { /* 111100 */ 97, 4 },
    /* 10 bit codewords */
    { /* 111101 */ 113, 4 },
    { /* 111110 */ 129, 4 },
    /* 10/11/12 bit codewords */
    { /* 111111 */ 145, 6 }
};
static hcb hcb11_1[] = {
    /* 4 bits */
    { /* 00000 */ 0, 0 },
    { /*       */ 0, 0 },
    { /* 00010 */ 1, 0 },
    { /*       */ 1, 0 },
	
    /* 5 bits */
    { /* 00100 */ 2, 0 },
    { /* 00101 */ 3, 0 },
    { /* 00110 */ 4, 0 },
    { /* 00111 */ 5, 0 },
    { /* 01000 */ 6, 0 },
    { /* 01001 */ 7, 0 },
	
    /* 6 bits */
    { /* 01010 */ 8,  1 },
    { /* 01011 */ 10, 1 },
    { /* 01100 */ 12, 1 },
	
    /* 6/7 bits */
    { /* 01101 */ 14, 2 },
	
    /* 7 bits */
    { /* 01110 */ 18, 2 },
    { /* 01111 */ 22, 2 },
    { /* 10000 */ 26, 2 },
	
    /* 7/8 bits */
    { /* 10001 */ 30, 3 },
	
    /* 8 bits */
    { /* 10010 */ 38, 3 },
    { /* 10011 */ 46, 3 },
    { /* 10100 */ 54, 3 },
    { /* 10101 */ 62, 3 },
    { /* 10110 */ 70, 3 },
    { /* 10111 */ 78, 3 },
	
    /* 8/9 bits */
    { /* 11000 */ 86, 4 },
	
    /* 9 bits */
    { /* 11001 */ 102, 4 },
    { /* 11010 */ 118, 4 },
    { /* 11011 */ 134, 4 },
	
    /* 9/10 bits */
    { /* 11100 */ 150, 5 },
	
    /* 10 bits */
    { /* 11101 */ 182, 5 },
    { /* 11110 */ 214, 5 },
	
    /* 10/11/12 bits */
    { /* 11111 */ 246, 7 }
};

//2nd step table
static hcb_2_pair hcb6_2[] = {
    /* 4 bit codewords */
    { 4,  0,  0 },
    { 4,  1,  0 },
    { 4,  0, -1 },
    { 4,  0,  1 },
    { 4, -1,  0 },
    { 4,  1,  1 },
    { 4, -1,  1 },
    { 4,  1, -1 },
    { 4, -1, -1 },
	
    /* 6 bit codewords */
    { 6,  2, -1 },
    { 6,  2,  1 },
    { 6, -2,  1 },
    { 6, -2, -1 },
    { 6, -2,  0 },
    { 6, -1,  2 },
    { 6,  2,  0 },
    { 6,  1, -2 },
    { 6,  1,  2 },
    { 6,  0, -2 },
    { 6, -1, -2 },
    { 6,  0,  2 },
    { 6,  2, -2 },
    { 6, -2,  2 },
    { 6, -2, -2 },
    { 6,  2,  2 },
	
    /* 7 bit codewords */
    { 7, -3,  1 },
    { 7,  3,  1 },
    { 7,  3, -1 },
    { 7, -1,  3 },
    { 7, -3, -1 },
    { 7,  1,  3 },
    { 7,  1, -3 },
    { 7, -1, -3 },
    { 7,  3,  0 },
    { 7, -3,  0 },
    { 7,  0, -3 },
    { 7,  0,  3 },
	
    /* 7/8 bit codewords */
    { 7,  3,  2 }, { 7,  3,  2 },
    { 8, -3, -2 },
    { 8, -2,  3 },
    { 8,  2,  3 },
    { 8,  3, -2 },
    { 8,  2, -3 },
    { 8, -2, -3 },
	
    /* 8 bit codewords */
    { 8, -3,  2 }, { 8, -3,  2 },
    { 8,  3,  3 }, { 8,  3,  3 },
    { 9,  3, -3 },
    { 9, -3, -3 },
    { 9, -3,  3 },
    { 9,  1, -4 },
    { 9, -1, -4 },
    { 9,  4,  1 },
    { 9, -4,  1 },
    { 9, -4, -1 },
    { 9,  1,  4 },
    { 9,  4, -1 },
    { 9, -1,  4 },
    { 9,  0, -4 },
	
    /* 9/10/11 bit codewords */
    { 9, -4,  2 }, { 9, -4,  2 }, { 9, -4,  2 }, { 9, -4,  2 },
    { 9, -4, -2 }, { 9, -4, -2 }, { 9, -4, -2 }, { 9, -4, -2 },
    { 9,  2,  4 }, { 9,  2,  4 }, { 9,  2,  4 }, { 9,  2,  4 },
    { 9, -2, -4 }, { 9, -2, -4 }, { 9, -2, -4 }, { 9, -2, -4 },
    { 9, -4,  0 }, { 9, -4,  0 }, { 9, -4,  0 }, { 9, -4,  0 },
    { 9,  4,  2 }, { 9,  4,  2 }, { 9,  4,  2 }, { 9,  4,  2 },
    { 9,  4, -2 }, { 9,  4, -2 }, { 9,  4, -2 }, { 9,  4, -2 },
    { 9, -2,  4 }, { 9, -2,  4 }, { 9, -2,  4 }, { 9, -2,  4 },
    { 9,  4,  0 }, { 9,  4,  0 }, { 9,  4,  0 }, { 9,  4,  0 },
    { 9,  2, -4 }, { 9,  2, -4 }, { 9,  2, -4 }, { 9,  2, -4 },
    { 9,  0,  4 }, { 9,  0,  4 }, { 9,  0,  4 }, { 9,  0,  4 },
    { 10, -3, -4 }, { 10, -3, -4 },
    { 10, -3,  4 }, { 10, -3,  4 },
    { 10,  3, -4 }, { 10,  3, -4 },
    { 10,  4, -3 }, { 10,  4, -3 },
    { 10,  3,  4 }, { 10,  3,  4 },
    { 10,  4,  3 }, { 10,  4,  3 },
    { 10, -4,  3 }, { 10, -4,  3 },
    { 10, -4, -3 }, { 10, -4, -3 },
    { 11,  4,  4 },
    { 11, -4,  4 },
    { 11, -4, -4 },
    { 11,  4, -4 }
};
static hcb_2_pair hcb8_2[] = {
    /* 3 bit codeword */
    { 3,  1,  1 },
	
    /* 4 bit codewords */
    { 4,  2,  1 },
    { 4,  1,  0 },
    { 4,  1,  2 },
    { 4,  0,  1 },
    { 4,  2,  2 },
	
    /* 5 bit codewords */
    { 5,  0,  0 },
    { 5,  2,  0 },
    { 5,  0,  2 },
    { 5,  3,  1 },
    { 5,  1,  3 },
    { 5,  3,  2 },
    { 5,  2,  3 },
	
    /* 6 bit codewords */
    { 6,  3,  3 },
    { 6,  4,  1 },
    { 6,  1,  4 },
    { 6,  4,  2 },
    { 6,  2,  4 },
    { 6,  3,  0 },
    { 6,  0,  3 },
    { 6,  4,  3 },
    { 6,  3,  4 },
    { 6,  5,  2 },
	
    /* 7 bit codewords */
    { 7,  5,  1 },
    { 7,  2,  5 },
    { 7,  1,  5 },
    { 7,  5,  3 },
    { 7,  3,  5 },
    { 7,  4,  4 },
    { 7,  5,  4 },
    { 7,  0,  4 },
    { 7,  4,  5 },
    { 7,  4,  0 },
    { 7,  2,  6 },
    { 7,  6,  2 },
	
    /* 7/8 bit codewords */
    { 7,  6,  1 }, { 7,  6,  1 },
    { 7,  1,  6 }, { 7,  1,  6 },
    { 8,  3,  6 },
    { 8,  6,  3 },
    { 8,  5,  5 },
    { 8,  5,  0 },
	
    /* 8 bit codewords */
    { 8,  6,  4 },
    { 8,  0,  5 },
    { 8,  4,  6 },
    { 8,  7,  1 },
    { 8,  7,  2 },
    { 8,  2,  7 },
    { 8,  6,  5 },
    { 8,  7,  3 },
	
    /* 8/9/10 bit codewords */
    { 8,  1,  7 }, { 8,  1,  7 }, { 8,  1,  7 }, { 8,  1,  7 },
    { 8,  5,  6 }, { 8,  5,  6 }, { 8,  5,  6 }, { 8,  5,  6 },
    { 8,  3,  7 }, { 8,  3,  7 }, { 8,  3,  7 }, { 8,  3,  7 },
    { 9,  6,  6 }, { 9,  6,  6 },
    { 9,  7,  4 }, { 9,  7,  4 },
    { 9,  6,  0 }, { 9,  6,  0 },
    { 9,  4,  7 }, { 9,  4,  7 },
    { 9,  0,  6 }, { 9,  0,  6 },
    { 9,  7,  5 }, { 9,  7,  5 },
    { 9,  7,  6 }, { 9,  7,  6 },
    { 9,  6,  7 }, { 9,  6,  7 },
    { 10,  5,  7 },
    { 10,  7,  0 },
    { 10,  0,  7 },
    { 10,  7,  7 }
};
static hcb_2_pair hcb10_2[] = {
    /* 4 bit codewords */
    { 4,  1,  1 },
    { 4,  1,  2 },
    { 4,  2,  1 },
	
    /* 5 bit codewords */
    { 5,  2,  2 },
    { 5,  1,  0 },
    { 5,  0,  1 },
    { 5,  1,  3 },
    { 5,  3,  2 },
    { 5,  3,  1 },
    { 5,  2,  3 },
    { 5,  3,  3 },
	
    /* 6 bit codewords */
    { 6,  2,  0 },
    { 6,  0,  2 },
    { 6,  2,  4 },
    { 6,  4,  2 },
    { 6,  1,  4 },
    { 6,  4,  1 },
    { 6,  0,  0 },
    { 6,  4,  3 },
    { 6,  3,  4 },
    { 6,  3,  0 },
    { 6,  0,  3 },
    { 6,  4,  4 },
    { 6,  2,  5 },
    { 6,  5,  2 },
	
    /* 7 bit codewords */
    { 7,  1,  5 },
    { 7,  5,  1 },
    { 7,  5,  3 },
    { 7,  3,  5 },
    { 7,  5,  4 },
    { 7,  4,  5 },
    { 7,  6,  2 },
    { 7,  2,  6 },
    { 7,  6,  3 },
    { 7,  4,  0 },
    { 7,  6,  1 },
    { 7,  0,  4 },
    { 7,  1,  6 },
    { 7,  3,  6 },
    { 7,  5,  5 },
    { 7,  6,  4 },
	
    /* 7/8 bit codewords */
    { 7,  4,  6 }, { 7,  4,  6 },
    { 8,  6,  5 },
    { 8,  7,  2 },
	
    /* 8 bit codewords */
    { 8,  3,  7 },
    { 8,  2,  7 },
    { 8,  5,  6 },
    { 8,  8,  2 },
    { 8,  7,  3 },
    { 8,  5,  0 },
    { 8,  7,  1 },
    { 8,  0,  5 },
    { 8,  8,  1 },
    { 8,  1,  7 },
    { 8,  8,  3 },
    { 8,  7,  4 },
    { 8,  4,  7 },
    { 8,  2,  8 },
    { 8,  6,  6 },
    { 8,  7,  5 },
    { 8,  1,  8 },
    { 8,  3,  8 },
    { 8,  8,  4 },
    { 8,  4,  8 },
	
    /* 8/9 bit codewords */
    { 8,  5,  7 }, { 8,  5,  7 },
    { 8,  8,  5 }, { 8,  8,  5 },
    { 8,  5,  8 }, { 8,  5,  8 },
    { 9,  7,  6 },
    { 9,  6,  7 },
	
    /* 9 bit codewords */
    { 9,  9,  2 },
    { 9,  6,  0 },
    { 9,  6,  8 },
    { 9,  9,  3 },
    { 9,  3,  9 },
    { 9,  9,  1 },
    { 9,  2,  9 },
    { 9,  0,  6 },
    { 9,  8,  6 },
    { 9,  9,  4 },
    { 9,  4,  9 },
    { 9, 10,  2 },
    { 9,  1,  9 },
    { 9,  7,  7 },
    { 9,  8,  7 },
    { 9,  9,  5 },
    { 9,  7,  8 },
    { 9, 10,  3 },
    { 9,  5,  9 },
    { 9, 10,  4 },
    { 9,  2, 10 },
    { 9, 10,  1 },
    { 9,  3, 10 },
    { 9,  9,  6 },
	
    /* 9/10 bit codewords */
    { 9,  6,  9 }, { 9,  6,  9 },
    { 9,  8,  0 }, { 9,  8,  0 },
    { 9,  4, 10 }, { 9,  4, 10 },
    { 9,  7,  0 }, { 9,  7,  0 },
    { 9, 11,  2 }, { 9, 11,  2 },
    { 10,  7,  9 },
    { 10, 11,  3 },
    { 10, 10,  6 },
    { 10,  1, 10 },
    { 10, 11,  1 },
    { 10,  9,  7 },
	
    /* 10 bit codewords */
    { 10,  0,  7 },
    { 10,  8,  8 },
    { 10, 10,  5 },
    { 10,  3, 11 },
    { 10,  5, 10 },
    { 10,  8,  9 },
    { 10, 11,  5 },
    { 10,  0,  8 },
    { 10, 11,  4 },
    { 10,  2, 11 },
    { 10,  7, 10 },
    { 10,  6, 10 },
    { 10, 10,  7 },
    { 10,  4, 11 },
    { 10,  1, 11 },
    { 10, 12,  2 },
    { 10,  9,  8 },
    { 10, 12,  3 },
    { 10, 11,  6 },
    { 10,  5, 11 },
    { 10, 12,  4 },
    { 10, 11,  7 },
    { 10, 12,  5 },
    { 10,  3, 12 },
    { 10,  6, 11 },
    { 10,  9,  0 },
    { 10, 10,  8 },
    { 10, 10,  0 },
    { 10, 12,  1 },
    { 10,  0,  9 },
    { 10,  4, 12 },
    { 10,  9,  9 },
	
    /* 10/11/12 bit codewords */
    { 10, 12,  6 }, { 10, 12,  6 }, { 10, 12,  6 }, { 10, 12,  6 },
    { 10,  2, 12 }, { 10,  2, 12 }, { 10,  2, 12 }, { 10,  2, 12 },
    { 10,  8, 10 }, { 10,  8, 10 }, { 10,  8, 10 }, { 10,  8, 10 },
    { 11,  9, 10 }, { 11,  9, 10 },
    { 11,  1, 12 }, { 11,  1, 12 },
    { 11, 11,  8 }, { 11, 11,  8 },
    { 11, 12,  7 }, { 11, 12,  7 },
    { 11,  7, 11 }, { 11,  7, 11 },
    { 11,  5, 12 }, { 11,  5, 12 },
    { 11,  6, 12 }, { 11,  6, 12 },
    { 11, 10,  9 }, { 11, 10,  9 },
    { 11,  8, 11 }, { 11,  8, 11 },
    { 11, 12,  8 }, { 11, 12,  8 },
    { 11,  0, 10 }, { 11,  0, 10 },
    { 11,  7, 12 }, { 11,  7, 12 },
    { 11, 11,  0 }, { 11, 11,  0 },
    { 11, 10, 10 }, { 11, 10, 10 },
    { 11, 11,  9 }, { 11, 11,  9 },
    { 11, 11, 10 }, { 11, 11, 10 },
    { 11,  0, 11 }, { 11,  0, 11 },
    { 11, 11, 11 }, { 11, 11, 11 },
    { 11,  9, 11 }, { 11,  9, 11 },
    { 11, 10, 11 }, { 11, 10, 11 },
    { 11, 12,  0 }, { 11, 12,  0 },
    { 11,  8, 12 }, { 11,  8, 12 },
    { 12, 12,  9 },
    { 12, 10, 12 },
    { 12,  9, 12 },
    { 12, 11, 12 },
    { 12, 12, 11 },
    { 12,  0, 12 },
    { 12, 12, 10 },
    { 12, 12, 12 }
};
static hcb_2_pair hcb11_2[] = {
    /* 4 */
    { 4,  0,  0 },
    { 4,  1,  1 },
	
    /* 5 */
    { 5, 16, 16 },
    { 5,  1,  0 },
    { 5,  0,  1 },
    { 5,  2,  1 },
    { 5,  1,  2 },
    { 5,  2,  2 },
	
    /* 6 */
    { 6,  1,  3 },
    { 6,  3,  1 },
    { 6,  3,  2 },
    { 6,  2,  0 },
    { 6,  2,  3 },
    { 6,  0,  2 },
	
    /* 6/7 */
    { 6,  3,  3 }, { 6,  3,  3 },
    { 7,  4,  1 },
    { 7,  1,  4 },
	
    /* 7 */
    { 7,  4,  2 },
    { 7,  2,  4 },
    { 7,  4,  3 },
    { 7,  3,  4 },
    { 7,  3,  0 },
    { 7,  0,  3 },
    { 7,  5,  1 },
    { 7,  5,  2 },
    { 7,  2,  5 },
    { 7,  4,  4 },
    { 7,  1,  5 },
    { 7,  5,  3 },
	
    /* 7/8 */
    { 7,  3,  5 }, { 7,  3,  5 },
    { 7,  5,  4 }, { 7,  5,  4 },
    { 8,  4,  5 },
    { 8,  6,  2 },
    { 8,  2,  6 },
    { 8,  6,  1 },
	
    /* 8 */
    { 8,  6,  3 },
    { 8,  3,  6 },
    { 8,  1,  6 },
    { 8,  4, 16 },
    { 8,  3, 16 },
    { 8, 16,  5 },
    { 8, 16,  3 },
    { 8, 16,  4 },
    { 8,  6,  4 },
    { 8, 16,  6 },
    { 8,  4,  0 },
    { 8,  4,  6 },
    { 8,  0,  4 },
    { 8,  2, 16 },
    { 8,  5,  5 },
    { 8,  5, 16 },
    { 8, 16,  7 },
    { 8, 16,  2 },
    { 8, 16,  8 },
    { 8,  2,  7 },
    { 8,  7,  2 },
    { 8,  3,  7 },
    { 8,  6,  5 },
    { 8,  5,  6 },
    { 8,  6, 16 },
    { 8, 16, 10 },
    { 8,  7,  3 },
    { 8,  7,  1 },
    { 8, 16,  9 },
    { 8,  7, 16 },
    { 8,  1, 16 },
    { 8,  1,  7 },
    { 8,  4,  7 },
    { 8, 16, 11 },
    { 8,  7,  4 },
    { 8, 16, 12 },
    { 8,  8, 16 },
    { 8, 16,  1 },
    { 8,  6,  6 },
    { 8,  9, 16 },
    { 8,  2,  8 },
    { 8,  5,  7 },
    { 8, 10, 16 },
    { 8, 16, 13 },
    { 8,  8,  3 },
    { 8,  8,  2 },
    { 8,  3,  8 },
    { 8,  5,  0 },
	
    /* 8/9 */
    { 8, 16, 14 }, { 8, 16, 14 },
    { 8, 11, 16 }, { 8, 11, 16 },
    { 8,  7,  5 }, { 8,  7,  5 },
    { 8,  4,  8 }, { 8,  4,  8 },
    { 8,  6,  7 }, { 8,  6,  7 },
    { 8,  7,  6 }, { 8,  7,  6 },
    { 8,  0,  5 }, { 8,  0,  5 },
    { 9,  8,  4 },
    { 9, 16, 15 },
	
    /* 9 */
    { 9, 12, 16 },
    { 9,  1,  8 },
    { 9,  8,  1 },
    { 9, 14, 16 },
    { 9,  5,  8 },
    { 9, 13, 16 },
    { 9,  3,  9 },
    { 9,  8,  5 },
    { 9,  7,  7 },
    { 9,  2,  9 },
    { 9,  8,  6 },
    { 9,  9,  2 },
    { 9,  9,  3 },
    { 9, 15, 16 },
    { 9,  4,  9 },
    { 9,  6,  8 },
    { 9,  6,  0 },
    { 9,  9,  4 },
    { 9,  5,  9 },
    { 9,  8,  7 },
    { 9,  7,  8 },
    { 9,  1,  9 },
    { 9, 10,  3 },
    { 9,  0,  6 },
    { 9, 10,  2 },
    { 9,  9,  1 },
    { 9,  9,  5 },
    { 9,  4, 10 },
    { 9,  2, 10 },
    { 9,  9,  6 },
    { 9,  3, 10 },
    { 9,  6,  9 },
    { 9, 10,  4 },
    { 9,  8,  8 },
    { 9, 10,  5 },
    { 9,  9,  7 },
    { 9, 11,  3 },
    { 9,  1, 10 },
    { 9,  7,  0 },
    { 9, 10,  6 },
    { 9,  7,  9 },
    { 9,  3, 11 },
    { 9,  5, 10 },
    { 9, 10,  1 },
    { 9,  4, 11 },
    { 9, 11,  2 },
    { 9, 13,  2 },
    { 9,  6, 10 },
	
    /* 9/10 */
    { 9, 13,  3 }, { 9, 13,  3 },
    { 9,  2, 11 }, { 9,  2, 11 },
    { 9, 16,  0 }, { 9, 16,  0 },
    { 9,  5, 11 }, { 9,  5, 11 },
    { 9, 11,  5 }, { 9, 11,  5 },
    { 10, 11,  4 },
    { 10,  9,  8 },
    { 10,  7, 10 },
    { 10,  8,  9 },
    { 10,  0, 16 },
    { 10,  4, 13 },
    { 10,  0,  7 },
    { 10,  3, 13 },
    { 10, 11,  6 },
    { 10, 13,  1 },
    { 10, 13,  4 },
    { 10, 12,  3 },
    { 10,  2, 13 },
    { 10, 13,  5 },
    { 10,  8, 10 },
    { 10,  6, 11 },
    { 10, 10,  8 },
    { 10, 10,  7 },
    { 10, 14,  2 },
    { 10, 12,  4 },
    { 10,  1, 11 },
    { 10,  4, 12 },
	
    /* 10 */
    { 10, 11,  1 },
    { 10,  3, 12 },
    { 10,  1, 13 },
    { 10, 12,  2 },
    { 10,  7, 11 },
    { 10,  3, 14 },
    { 10,  5, 12 },
    { 10,  5, 13 },
    { 10, 14,  4 },
    { 10,  4, 14 },
    { 10, 11,  7 },
    { 10, 14,  3 },
    { 10, 12,  5 },
    { 10, 13,  6 },
    { 10, 12,  6 },
    { 10,  8,  0 },
    { 10, 11,  8 },
    { 10,  2, 12 },
    { 10,  9,  9 },
    { 10, 14,  5 },
    { 10,  6, 13 },
    { 10, 10, 10 },
    { 10, 15,  2 },
    { 10,  8, 11 },
    { 10,  9, 10 },
    { 10, 14,  6 },
    { 10, 10,  9 },
    { 10,  5, 14 },
    { 10, 11,  9 },
    { 10, 14,  1 },
    { 10,  2, 14 },
    { 10,  6, 12 },
    { 10,  1, 12 },
    { 10, 13,  8 },
    { 10,  0,  8 },
    { 10, 13,  7 },
    { 10,  7, 12 },
    { 10, 12,  7 },
    { 10,  7, 13 },
    { 10, 15,  3 },
    { 10, 12,  1 },
    { 10,  6, 14 },
    { 10,  2, 15 },
    { 10, 15,  5 },
    { 10, 15,  4 },
    { 10,  1, 14 },
    { 10,  9, 11 },
    { 10,  4, 15 },
    { 10, 14,  7 },
    { 10,  8, 13 },
    { 10, 13,  9 },
    { 10,  8, 12 },
    { 10,  5, 15 },
    { 10,  3, 15 },
    { 10, 10, 11 },
    { 10, 11, 10 },
    { 10, 12,  8 },
    { 10, 15,  6 },
    { 10, 15,  7 },
    { 10,  8, 14 },
    { 10, 15,  1 },
    { 10,  7, 14 },
    { 10,  9,  0 },
    { 10,  0,  9 },
	
    /* 10/11/12 */
    { 10,  9, 13 }, { 10,  9, 13 }, { 10,  9, 13 }, { 10,  9, 13 },
    { 10,  9, 12 }, { 10,  9, 12 }, { 10,  9, 12 }, { 10,  9, 12 },
    { 10, 12,  9 }, { 10, 12,  9 }, { 10, 12,  9 }, { 10, 12,  9 },
    { 10, 14,  8 }, { 10, 14,  8 }, { 10, 14,  8 }, { 10, 14,  8 },
    { 10, 10, 13 }, { 10, 10, 13 }, { 10, 10, 13 }, { 10, 10, 13 },
    { 10, 14,  9 }, { 10, 14,  9 }, { 10, 14,  9 }, { 10, 14,  9 },
    { 10, 12, 10 }, { 10, 12, 10 }, { 10, 12, 10 }, { 10, 12, 10 },
    { 10,  6, 15 }, { 10,  6, 15 }, { 10,  6, 15 }, { 10,  6, 15 },
    { 10,  7, 15 }, { 10,  7, 15 }, { 10,  7, 15 }, { 10,  7, 15 },
	
    { 11,  9, 14 }, { 11,  9, 14 },
    { 11, 15,  8 }, { 11, 15,  8 },
    { 11, 11, 11 }, { 11, 11, 11 },
    { 11, 11, 14 }, { 11, 11, 14 },
    { 11,  1, 15 }, { 11,  1, 15 },
    { 11, 10, 12 }, { 11, 10, 12 },
    { 11, 10, 14 }, { 11, 10, 14 },
    { 11, 13, 11 }, { 11, 13, 11 },
    { 11, 13, 10 }, { 11, 13, 10 },
    { 11, 11, 13 }, { 11, 11, 13 },
    { 11, 11, 12 }, { 11, 11, 12 },
    { 11,  8, 15 }, { 11,  8, 15 },
    { 11, 14, 11 }, { 11, 14, 11 },
    { 11, 13, 12 }, { 11, 13, 12 },
    { 11, 12, 13 }, { 11, 12, 13 },
    { 11, 15,  9 }, { 11, 15,  9 },
    { 11, 14, 10 }, { 11, 14, 10 },
    { 11, 10,  0 }, { 11, 10,  0 },
    { 11, 12, 11 }, { 11, 12, 11 },
    { 11,  9, 15 }, { 11,  9, 15 },
    { 11,  0, 10 }, { 11,  0, 10 },
    { 11, 12, 12 }, { 11, 12, 12 },
    { 11, 11,  0 }, { 11, 11,  0 },
    { 11, 12, 14 }, { 11, 12, 14 },
    { 11, 10, 15 }, { 11, 10, 15 },
    { 11, 13, 13 }, { 11, 13, 13 },
    { 11,  0, 13 }, { 11,  0, 13 },
    { 11, 14, 12 }, { 11, 14, 12 },
    { 11, 15, 10 }, { 11, 15, 10 },
    { 11, 15, 11 }, { 11, 15, 11 },
    { 11, 11, 15 }, { 11, 11, 15 },
    { 11, 14, 13 }, { 11, 14, 13 },
    { 11, 13,  0 }, { 11, 13,  0 },
    { 11,  0, 11 }, { 11,  0, 11 },
    { 11, 13, 14 }, { 11, 13, 14 },
    { 11, 15, 12 }, { 11, 15, 12 },
    { 11, 15, 13 }, { 11, 15, 13 },
    { 11, 12, 15 }, { 11, 12, 15 },
    { 11, 14,  0 }, { 11, 14,  0 },
    { 11, 14, 14 }, { 11, 14, 14 },
    { 11, 13, 15 }, { 11, 13, 15 },
    { 11, 12,  0 }, { 11, 12,  0 },
    { 11, 14, 15 }, { 11, 14, 15 },
    { 12,  0, 14 },
    { 12,  0, 12 },
    { 12, 15, 14 },
    { 12, 15,  0 },
    { 12,  0, 15 },
    { 12, 15, 15 }
};
static hcb_bin_pair hcb5[] = {
    { /*  0 */ 0, {  1,  2 } },
    { /*  1 */ 1, {  0,  0 } }, /* 0 */
    { /*  2 */ 0, {  1,  2 } },
    { /*  3 */ 0, {  2,  3 } },
    { /*  4 */ 0, {  3,  4 } },
    { /*  5 */ 0, {  4,  5 } },
    { /*  6 */ 0, {  5,  6 } },
    { /*  7 */ 0, {  6,  7 } },
    { /*  8 */ 0, {  7,  8 } },
    { /*  9 */ 1, { -1,  0 } }, /* 1000 */
    { /* 10 */ 1, {  1,  0 } }, /* 1001 */
    { /* 11 */ 1, {  0,  1 } }, /* 1010 */
    { /* 12 */ 1, {  0, -1 } }, /* 1011 */
    { /* 13 */ 0, {  4,  5 } },
    { /* 14 */ 0, {  5,  6 } },
    { /* 15 */ 0, {  6,  7 } },
    { /* 16 */ 0, {  7,  8 } },
    { /* 17 */ 1, {  1, -1 } },
    { /* 18 */ 1, { -1,  1 } },
    { /* 19 */ 1, { -1, -1 } },
    { /* 20 */ 1, {  1,  1 } },
    { /* 21 */ 0, {  4,  5 } },
    { /* 22 */ 0, {  5,  6 } },
    { /* 23 */ 0, {  6,  7 } },
    { /* 24 */ 0, {  7,  8 } },
    { /* 25 */ 0, {  8,  9 } },
    { /* 26 */ 0, {  9, 10 } },
    { /* 27 */ 0, { 10, 11 } },
    { /* 28 */ 0, { 11, 12 } },
    { /* 29 */ 0, { 12, 13 } },
    { /* 30 */ 0, { 13, 14 } },
    { /* 31 */ 0, { 14, 15 } },
    { /* 32 */ 0, { 15, 16 } },
    { /* 33 */ 1, { -2,  0 } },
    { /* 34 */ 1, {  0,  2 } },
    { /* 35 */ 1, {  2,  0 } },
    { /* 36 */ 1, {  0, -2 } },
    { /* 37 */ 0, { 12, 13 } },
    { /* 38 */ 0, { 13, 14 } },
    { /* 39 */ 0, { 14, 15 } },
    { /* 40 */ 0, { 15, 16 } },
    { /* 41 */ 0, { 16, 17 } },
    { /* 42 */ 0, { 17, 18 } },
    { /* 43 */ 0, { 18, 19 } },
    { /* 44 */ 0, { 19, 20 } },
    { /* 45 */ 0, { 20, 21 } },
    { /* 46 */ 0, { 21, 22 } },
    { /* 47 */ 0, { 22, 23 } },
    { /* 48 */ 0, { 23, 24 } },
    { /* 49 */ 1, { -2, -1 } },
    { /* 50 */ 1, {  2,  1 } },
    { /* 51 */ 1, { -1, -2 } },
    { /* 52 */ 1, {  1,  2 } },
    { /* 53 */ 1, { -2,  1 } },
    { /* 54 */ 1, {  2, -1 } },
    { /* 55 */ 1, { -1,  2 } },
    { /* 56 */ 1, {  1, -2 } },
    { /* 57 */ 1, { -3,  0 } },
    { /* 58 */ 1, {  3,  0 } },
    { /* 59 */ 1, {  0, -3 } },
    { /* 60 */ 1, {  0,  3 } },
    { /* 61 */ 0, { 12, 13 } },
    { /* 62 */ 0, { 13, 14 } },
    { /* 63 */ 0, { 14, 15 } },
    { /* 64 */ 0, { 15, 16 } },
    { /* 65 */ 0, { 16, 17 } },
    { /* 66 */ 0, { 17, 18 } },
    { /* 67 */ 0, { 18, 19 } },
    { /* 68 */ 0, { 19, 20 } },
    { /* 69 */ 0, { 20, 21 } },
    { /* 70 */ 0, { 21, 22 } },
    { /* 71 */ 0, { 22, 23 } },
    { /* 72 */ 0, { 23, 24 } },
    { /* 73 */ 1, { -3, -1 } },
    { /* 74 */ 1, {  1,  3 } },
    { /* 75 */ 1, {  3,  1 } },
    { /* 76 */ 1, { -1, -3 } },
    { /* 77 */ 1, { -3,  1 } },
    { /* 78 */ 1, {  3, -1 } },
    { /* 79 */ 1, {  1, -3 } },
    { /* 80 */ 1, { -1,  3 } },
    { /* 81 */ 1, { -2,  2 } },
    { /* 82 */ 1, {  2,  2 } },
    { /* 83 */ 1, { -2, -2 } },
    { /* 84 */ 1, {  2, -2 } },
    { /* 85 */ 0, { 12, 13 } },
    { /* 86 */ 0, { 13, 14 } },
    { /* 87 */ 0, { 14, 15 } },
    { /* 88 */ 0, { 15, 16 } },
    { /* 89 */ 0, { 16, 17 } },
    { /* 90 */ 0, { 17, 18 } },
    { /* 91 */ 0, { 18, 19 } },
    { /* 92 */ 0, { 19, 20 } },
    { /* 93 */ 0, { 20, 21 } },
    { /* 94 */ 0, { 21, 22 } },
    { /* 95 */ 0, { 22, 23 } },
    { /* 96 */ 0, { 23, 24 } },
    { /* 97 */ 1, { -3, -2 } },
    { /* 98 */ 1, {  3, -2 } },
    { /* 99 */ 1, { -2,  3 } },
    { /* 00 */ 1, {  2, -3 } },
    { /* 01 */ 1, {  3,  2 } },
    { /* 02 */ 1, {  2,  3 } },
    { /* 03 */ 1, { -3,  2 } },
    { /* 04 */ 1, { -2, -3 } },
    { /* 05 */ 1, {  0, -4 } },
    { /* 06 */ 1, { -4,  0 } },
    { /* 07 */ 1, {  4,  1 } },
    { /* 08 */ 1, {  4,  0 } },
    { /* 09 */ 0, { 12, 13 } },
    { /* 10 */ 0, { 13, 14 } },
    { /* 11 */ 0, { 14, 15 } },
    { /* 12 */ 0, { 15, 16 } },
    { /* 13 */ 0, { 16, 17 } },
    { /* 14 */ 0, { 17, 18 } },
    { /* 15 */ 0, { 18, 19 } },
    { /* 16 */ 0, { 19, 20 } },
    { /* 17 */ 0, { 20, 21 } },
    { /* 18 */ 0, { 21, 22 } },
    { /* 19 */ 0, { 22, 23 } },
    { /* 20 */ 0, { 23, 24 } },
    { /* 21 */ 1, { -4, -1 } },
    { /* 22 */ 1, {  0,  4 } },
    { /* 23 */ 1, {  4, -1 } },
    { /* 24 */ 1, { -1, -4 } },
    { /* 25 */ 1, {  1,  4 } },
    { /* 26 */ 1, { -1,  4 } },
    { /* 27 */ 1, { -4,  1 } },
    { /* 28 */ 1, {  1, -4 } },
    { /* 29 */ 1, {  3, -3 } },
    { /* 30 */ 1, { -3, -3 } },
    { /* 31 */ 1, { -3,  3 } },
    { /* 32 */ 1, { -2,  4 } },
    { /* 33 */ 1, { -4, -2 } },
    { /* 34 */ 1, {  4,  2 } },
    { /* 35 */ 1, {  2, -4 } },
    { /* 36 */ 1, {  2,  4 } },
    { /* 37 */ 1, {  3,  3 } },
    { /* 38 */ 1, { -4,  2 } },
    { /* 39 */ 0, { 6, 7 } },
    { /* 40 */ 0, { 7, 8 } },
    { /* 41 */ 0, { 8, 9 } },
    { /* 42 */ 0, { 9, 10 } },
    { /* 43 */ 0, { 10, 11 } },
    { /* 44 */ 0, { 11, 12 } },
    { /* 45 */ 1, { -2, -4 } },
    { /* 46 */ 1, {  4, -2 } },
    { /* 47 */ 1, {  3, -4 } },
    { /* 48 */ 1, { -4, -3 } },
    { /* 49 */ 1, { -4,  3 } },
    { /* 50 */ 1, {  3,  4 } },
    { /* 51 */ 1, { -3,  4 } },
    { /* 52 */ 1, {  4,  3 } },
    { /* 53 */ 1, {  4, -3 } },
    { /* 54 */ 1, { -3, -4 } },
    { /* 55 */ 0, { 2, 3 } },
    { /* 56 */ 0, { 3, 4 } },
    { /* 57 */ 1, {  4, -4 } },
    { /* 58 */ 1, { -4,  4 } },
    { /* 59 */ 1, {  4,  4 } },
    { /* 60 */ 1, { -4, -4 } }
};
static hcb_bin_pair hcb7[] = {
    { /*  0 */ 0, { 1, 2 } },
    { /*  1 */ 1, { 0, 0 } },
    { /*  2 */ 0, { 1, 2 } },
    { /*  3 */ 0, { 2, 3 } },
    { /*  4 */ 0, { 3, 4 } },
    { /*  5 */ 1, { 1, 0 } },
    { /*  6 */ 1, { 0, 1 } },
    { /*  7 */ 0, { 2, 3 } },
    { /*  8 */ 0, { 3, 4 } },
    { /*  9 */ 1, { 1,  1 } },
    { /* 10 */ 0, { 3, 4 } },
    { /* 11 */ 0, { 4, 5 } },
    { /* 12 */ 0, { 5, 6 } },
    { /* 13 */ 0, { 6, 7 } },
    { /* 14 */ 0, { 7, 8 } },
    { /* 15 */ 0, { 8, 9 } },
    { /* 16 */ 0, { 9, 10 } },
    { /* 17 */ 0, { 10, 11 } },
    { /* 18 */ 0, { 11, 12 } },
    { /* 19 */ 1, { 2,  1 } },
    { /* 20 */ 1, { 1,  2 } },
    { /* 21 */ 1, { 2,  0 } },
    { /* 22 */ 1, { 0,  2 } },
    { /* 23 */ 0, { 8, 9 } },
    { /* 24 */ 0, { 9, 10 } },
    { /* 25 */ 0, { 10, 11 } },
    { /* 26 */ 0, { 11, 12 } },
    { /* 27 */ 0, { 12, 13 } },
    { /* 28 */ 0, { 13, 14 } },
    { /* 29 */ 0, { 14, 15 } },
    { /* 30 */ 0, { 15, 16 } },
    { /* 31 */ 1, { 3,  1 } },
    { /* 32 */ 1, { 1,  3 } },
    { /* 33 */ 1, { 2,  2 } },
    { /* 34 */ 1, { 3,  0 } },
    { /* 35 */ 1, { 0,  3 } },
    { /* 36 */ 0, { 11, 12 } },
    { /* 37 */ 0, { 12, 13 } },
    { /* 38 */ 0, { 13, 14 } },
    { /* 39 */ 0, { 14, 15 } },
    { /* 40 */ 0, { 15, 16 } },
    { /* 41 */ 0, { 16, 17 } },
    { /* 42 */ 0, { 17, 18 } },
    { /* 43 */ 0, { 18, 19 } },
    { /* 44 */ 0, { 19, 20 } },
    { /* 45 */ 0, { 20, 21 } },
    { /* 46 */ 0, { 21, 22 } },
    { /* 47 */ 1, { 2,  3 } },
    { /* 48 */ 1, { 3,  2 } },
    { /* 49 */ 1, { 1,  4 } },
    { /* 50 */ 1, { 4,  1 } },
    { /* 51 */ 1, { 1,  5 } },
    { /* 52 */ 1, { 5,  1 } },
    { /* 53 */ 1, { 3,  3 } },
    { /* 54 */ 1, { 2,  4 } },
    { /* 55 */ 1, { 0,  4 } },
    { /* 56 */ 1, { 4,  0 } },
    { /* 57 */ 0, { 12, 13 } },
    { /* 58 */ 0, { 13, 14 } },
    { /* 59 */ 0, { 14, 15 } },
    { /* 60 */ 0, { 15, 16 } },
    { /* 61 */ 0, { 16, 17 } },
    { /* 62 */ 0, { 17, 18 } },
    { /* 63 */ 0, { 18, 19 } },
    { /* 64 */ 0, { 19, 20 } },
    { /* 65 */ 0, { 20, 21 } },
    { /* 66 */ 0, { 21, 22 } },
    { /* 67 */ 0, { 22, 23 } },
    { /* 68 */ 0, { 23, 24 } },
    { /* 69 */ 1, { 4,  2 } },
    { /* 70 */ 1, { 2,  5 } },
    { /* 71 */ 1, { 5,  2 } },
    { /* 72 */ 1, { 0,  5 } },
    { /* 73 */ 1, { 6,  1 } },
    { /* 74 */ 1, { 5,  0 } },
    { /* 75 */ 1, { 1,  6 } },
    { /* 76 */ 1, { 4,  3 } },
    { /* 77 */ 1, { 3,  5 } },
    { /* 78 */ 1, { 3,  4 } },
    { /* 79 */ 1, { 5,  3 } },
    { /* 80 */ 1, { 2,  6 } },
    { /* 81 */ 1, { 6,  2 } },
    { /* 82 */ 1, { 1,  7 } },
    { /* 83 */ 0, { 10, 11 } },
    { /* 84 */ 0, { 11, 12 } },
    { /* 85 */ 0, { 12, 13 } },
    { /* 86 */ 0, { 13, 14 } },
    { /* 87 */ 0, { 14, 15 } },
    { /* 88 */ 0, { 15, 16 } },
    { /* 89 */ 0, { 16, 17 } },
    { /* 90 */ 0, { 17, 18 } },
    { /* 91 */ 0, { 18, 19 } },
    { /* 92 */ 0, { 19, 20 } },
    { /* 93 */ 1, { 3,  6 } },
    { /* 94 */ 1, { 0,  6 } },
    { /* 95 */ 1, { 6,  0 } },
    { /* 96 */ 1, { 4,  4 } },
    { /* 97 */ 1, { 7,  1 } },
    { /* 98 */ 1, { 4,  5 } },
    { /* 99 */ 1, { 7,  2 } },
    { /* 00 */ 1, { 5,  4 } },
    { /* 01 */ 1, { 6,  3 } },
    { /* 02 */ 1, { 2,  7 } },
    { /* 03 */ 1, { 7,  3 } },
    { /* 04 */ 1, { 6,  4 } },
    { /* 05 */ 1, { 5,  5 } },
    { /* 06 */ 1, { 4,  6 } },
    { /* 07 */ 1, { 3,  7 } },
    { /* 08 */ 0, { 5, 6 } },
    { /* 09 */ 0, { 6, 7 } },
    { /* 10 */ 0, { 7, 8 } },
    { /* 11 */ 0, { 8, 9 } },
    { /* 12 */ 0, { 9, 10 } },
    { /* 13 */ 1, { 7,  0 } },
    { /* 14 */ 1, { 0,  7 } },
    { /* 15 */ 1, { 6,  5 } },
    { /* 16 */ 1, { 5,  6 } },
    { /* 17 */ 1, { 7,  4 } },
    { /* 18 */ 1, { 4,  7 } },
    { /* 19 */ 1, { 5,  7 } },
    { /* 20 */ 1, { 7,  5 } },
    { /* 21 */ 0, { 2, 3 } },
    { /* 22 */ 0, { 3, 4 } },
    { /* 23 */ 1, { 7,  6 } },
    { /* 24 */ 1, { 6,  6 } },
    { /* 25 */ 1, { 6,  7 } },
    { /* 26 */ 1, { 7,  7 } }
};
static hcb_bin_pair hcb9[] = {
    { /*  0 */ 0, { 1, 2 } },
    { /*  1 */ 1, { 0, 0 } },
    { /*  2 */ 0, { 1, 2 } },
    { /*  3 */ 0, { 2, 3 } },
    { /*  4 */ 0, { 3, 4 } },
    { /*  5 */ 1, { 1,  0 } },
    { /*  6 */ 1, { 0,  1 } },
    { /*  7 */ 0, { 2, 3 } },
    { /*  8 */ 0, { 3, 4 } },
    { /*  9 */ 1, { 1,  1 } },
    { /* 10 */ 0, { 3, 4 } },
    { /* 11 */ 0, { 4, 5 } },
    { /* 12 */ 0, { 5, 6 } },
    { /* 13 */ 0, { 6, 7 } },
    { /* 14 */ 0, { 7, 8 } },
    { /* 15 */ 0, { 8, 9 } },
    { /* 16 */ 0, { 9, 10 } },
    { /* 17 */ 0, { 10, 11 } },
    { /* 18 */ 0, { 11, 12 } },
    { /* 19 */ 1, { 2,  1 } },
    { /* 20 */ 1, { 1,  2 } },
    { /* 21 */ 1, { 2,  0 } },
    { /* 22 */ 1, { 0,  2 } },
    { /* 23 */ 0, { 8, 9 } },
    { /* 24 */ 0, { 9, 10 } },
    { /* 25 */ 0, { 10, 11 } },
    { /* 26 */ 0, { 11, 12 } },
    { /* 27 */ 0, { 12, 13 } },
    { /* 28 */ 0, { 13, 14 } },
    { /* 29 */ 0, { 14, 15 } },
    { /* 30 */ 0, { 15, 16 } },
    { /* 31 */ 1, { 3,  1 } },
    { /* 32 */ 1, { 2,  2 } },
    { /* 33 */ 1, { 1,  3 } },
    { /* 34 */ 0, { 13, 14 } },
    { /* 35 */ 0, { 14, 15 } },
    { /* 36 */ 0, { 15, 16 } },
    { /* 37 */ 0, { 16, 17 } },
    { /* 38 */ 0, { 17, 18 } },
    { /* 39 */ 0, { 18, 19 } },
    { /* 40 */ 0, { 19, 20 } },
    { /* 41 */ 0, { 20, 21 } },
    { /* 42 */ 0, { 21, 22 } },
    { /* 43 */ 0, { 22, 23 } },
    { /* 44 */ 0, { 23, 24 } },
    { /* 45 */ 0, { 24, 25 } },
    { /* 46 */ 0, { 25, 26 } },
    { /* 47 */ 1, { 3,  0 } },
    { /* 48 */ 1, { 0,  3 } },
    { /* 49 */ 1, { 2,  3 } },
    { /* 50 */ 1, { 3,  2 } },
    { /* 51 */ 1, { 1,  4 } },
    { /* 52 */ 1, { 4,  1 } },
    { /* 53 */ 1, { 2,  4 } },
    { /* 54 */ 1, { 1,  5 } },
    { /* 55 */ 0, { 18, 19 } },
    { /* 56 */ 0, { 19, 20 } },
    { /* 57 */ 0, { 20, 21 } },
    { /* 58 */ 0, { 21, 22 } },
    { /* 59 */ 0, { 22, 23 } },
    { /* 60 */ 0, { 23, 24 } },
    { /* 61 */ 0, { 24, 25 } },
    { /* 62 */ 0, { 25, 26 } },
    { /* 63 */ 0, { 26, 27 } },
    { /* 64 */ 0, { 27, 28 } },
    { /* 65 */ 0, { 28, 29 } },
    { /* 66 */ 0, { 29, 30 } },
    { /* 67 */ 0, { 30, 31 } },
    { /* 68 */ 0, { 31, 32 } },
    { /* 69 */ 0, { 32, 33 } },
    { /* 70 */ 0, { 33, 34 } },
    { /* 71 */ 0, { 34, 35 } },
    { /* 72 */ 0, { 35, 36 } },
    { /* 73 */ 1, { 4,  2 } },
    { /* 74 */ 1, { 3,  3 } },
    { /* 75 */ 1, { 0,  4 } },
    { /* 76 */ 1, { 4,  0 } },
    { /* 77 */ 1, { 5,  1 } },
    { /* 78 */ 1, { 2,  5 } },
    { /* 79 */ 1, { 1,  6 } },
    { /* 80 */ 1, { 3,  4 } },
    { /* 81 */ 1, { 5,  2 } },
    { /* 82 */ 1, { 6,  1 } },
    { /* 83 */ 1, { 4,  3 } },
    { /* 84 */ 0, { 25, 26 } },
    { /* 85 */ 0, { 26, 27 } },
    { /* 86 */ 0, { 27, 28 } },
    { /* 87 */ 0, { 28, 29 } },
    { /* 88 */ 0, { 29, 30 } },
    { /* 89 */ 0, { 30, 31 } },
    { /* 90 */ 0, { 31, 32 } },
    { /* 91 */ 0, { 32, 33 } },
    { /* 92 */ 0, { 33, 34 } },
    { /* 93 */ 0, { 34, 35 } },
    { /* 94 */ 0, { 35, 36 } },
    { /* 95 */ 0, { 36, 37 } },
    { /* 96 */ 0, { 37, 38 } },
    { /* 97 */ 0, { 38, 39 } },
    { /* 98 */ 0, { 39, 40 } },
    { /* 99 */ 0, { 40, 41 } },
    { /* 00 */ 0, { 41, 42 } },
    { /* 01 */ 0, { 42, 43 } },
    { /* 02 */ 0, { 43, 44 } },
    { /* 03 */ 0, { 44, 45 } },
    { /* 04 */ 0, { 45, 46 } },
    { /* 05 */ 0, { 46, 47 } },
    { /* 06 */ 0, { 47, 48 } },
    { /* 07 */ 0, { 48, 49 } },
    { /* 08 */ 0, { 49, 50 } },
    { /* 09 */ 1, { 0,  5 } },
    { /* 10 */ 1, { 2,  6 } },
    { /* 11 */ 1, { 5,  0 } },
    { /* 12 */ 1, { 1,  7 } },
    { /* 13 */ 1, { 3,  5 } },
    { /* 14 */ 1, { 1,  8 } },
    { /* 15 */ 1, { 8,  1 } },
    { /* 16 */ 1, { 4,  4 } },
    { /* 17 */ 1, { 5,  3 } },
    { /* 18 */ 1, { 6,  2 } },
    { /* 19 */ 1, { 7,  1 } },
    { /* 20 */ 1, { 0,  6 } },
    { /* 21 */ 1, { 8,  2 } },
    { /* 22 */ 1, { 2,  8 } },
    { /* 23 */ 1, { 3,  6 } },
    { /* 24 */ 1, { 2,  7 } },
    { /* 25 */ 1, { 4,  5 } },
    { /* 26 */ 1, { 9,  1 } },
    { /* 27 */ 1, { 1,  9 } },
    { /* 28 */ 1, { 7,  2 } },
    { /* 29 */ 0, { 30, 31 } },
    { /* 30 */ 0, { 31, 32 } },
    { /* 31 */ 0, { 32, 33 } },
    { /* 32 */ 0, { 33, 34 } },
    { /* 33 */ 0, { 34, 35 } },
    { /* 34 */ 0, { 35, 36 } },
    { /* 35 */ 0, { 36, 37 } },
    { /* 36 */ 0, { 37, 38 } },
    { /* 37 */ 0, { 38, 39 } },
    { /* 38 */ 0, { 39, 40 } },
    { /* 39 */ 0, { 40, 41 } },
    { /* 40 */ 0, { 41, 42 } },
    { /* 41 */ 0, { 42, 43 } },
    { /* 42 */ 0, { 43, 44 } },
    { /* 43 */ 0, { 44, 45 } },
    { /* 44 */ 0, { 45, 46 } },
    { /* 45 */ 0, { 46, 47 } },
    { /* 46 */ 0, { 47, 48 } },
    { /* 47 */ 0, { 48, 49 } },
    { /* 48 */ 0, { 49, 50 } },
    { /* 49 */ 0, { 50, 51 } },
    { /* 50 */ 0, { 51, 52 } },
    { /* 51 */ 0, { 52, 53 } },
    { /* 52 */ 0, { 53, 54 } },
    { /* 53 */ 0, { 54, 55 } },
    { /* 54 */ 0, { 55, 56 } },
    { /* 55 */ 0, { 56, 57 } },
    { /* 56 */ 0, { 57, 58 } },
    { /* 57 */ 0, { 58, 59 } },
    { /* 58 */ 0, { 59, 60 } },
    { /* 59 */ 1, {  6,  0 } },
    { /* 60 */ 1, {  5,  4 } },
    { /* 61 */ 1, {  6,  3 } },
    { /* 62 */ 1, {  8,  3 } },
    { /* 63 */ 1, {  0,  7 } },
    { /* 64 */ 1, {  9,  2 } },
    { /* 65 */ 1, {  3,  8 } },
    { /* 66 */ 1, {  4,  6 } },
    { /* 67 */ 1, {  3,  7 } },
    { /* 68 */ 1, {  0,  8 } },
    { /* 69 */ 1, { 10,  1 } },
    { /* 70 */ 1, {  6,  4 } },
    { /* 71 */ 1, {  2,  9 } },
    { /* 72 */ 1, {  5,  5 } },
    { /* 73 */ 1, {  8,  0 } },
    { /* 74 */ 1, {  7,  0 } },
    { /* 75 */ 1, {  7,  3 } },
    { /* 76 */ 1, { 10,  2 } },
    { /* 77 */ 1, {  9,  3 } },
    { /* 78 */ 1, {  8,  4 } },
    { /* 79 */ 1, {  1, 10 } },
    { /* 80 */ 1, {  7,  4 } },
    { /* 81 */ 1, {  6,  5 } },
    { /* 82 */ 1, {  5,  6 } },
    { /* 83 */ 1, {  4,  8 } },
    { /* 84 */ 1, {  4,  7 } },
    { /* 85 */ 1, {  3,  9 } },
    { /* 86 */ 1, { 11,  1 } },
    { /* 87 */ 1, {  5,  8 } },
    { /* 88 */ 1, {  9,  0 } },
    { /* 89 */ 1, {  8,  5 } },
    { /* 90 */ 0, { 29, 30 } },
    { /* 91 */ 0, { 30, 31 } },
    { /* 92 */ 0, { 31, 32 } },
    { /* 93 */ 0, { 32, 33 } },
    { /* 94 */ 0, { 33, 34 } },
    { /* 95 */ 0, { 34, 35 } },
    { /* 96 */ 0, { 35, 36 } },
    { /* 97 */ 0, { 36, 37 } },
    { /* 98 */ 0, { 37, 38 } },
    { /* 99 */ 0, { 38, 39 } },
    { /* 00 */ 0, { 39, 40 } },
    { /* 01 */ 0, { 40, 41 } },
    { /* 02 */ 0, { 41, 42 } },
    { /* 03 */ 0, { 42, 43 } },
    { /* 04 */ 0, { 43, 44 } },
    { /* 05 */ 0, { 44, 45 } },
    { /* 06 */ 0, { 45, 46 } },
    { /* 07 */ 0, { 46, 47 } },
    { /* 08 */ 0, { 47, 48 } },
    { /* 09 */ 0, { 48, 49 } },
    { /* 10 */ 0, { 49, 50 } },
    { /* 11 */ 0, { 50, 51 } },
    { /* 12 */ 0, { 51, 52 } },
    { /* 13 */ 0, { 52, 53 } },
    { /* 14 */ 0, { 53, 54 } },
    { /* 15 */ 0, { 54, 55 } },
    { /* 16 */ 0, { 55, 56 } },
    { /* 17 */ 0, { 56, 57 } },
    { /* 18 */ 0, { 57, 58 } },
    { /* 19 */ 1, { 10,  3 } },
    { /* 20 */ 1, {  2, 10 } },
    { /* 21 */ 1, {  0,  9 } },
    { /* 22 */ 1, { 11,  2 } },
    { /* 23 */ 1, {  9,  4 } },
    { /* 24 */ 1, {  6,  6 } },
    { /* 25 */ 1, { 12,  1 } },
    { /* 26 */ 1, {  4,  9 } },
    { /* 27 */ 1, {  8,  6 } },
    { /* 28 */ 1, {  1, 11 } },
    { /* 29 */ 1, {  9,  5 } },
    { /* 30 */ 1, { 10,  4 } },
    { /* 31 */ 1, {  5,  7 } },
    { /* 32 */ 1, {  7,  5 } },
    { /* 33 */ 1, {  2, 11 } },
    { /* 34 */ 1, {  1, 12 } },
    { /* 35 */ 1, { 12,  2 } },
    { /* 36 */ 1, { 11,  3 } },
    { /* 37 */ 1, {  3, 10 } },
    { /* 38 */ 1, {  5,  9 } },
    { /* 39 */ 1, {  6,  7 } },
    { /* 40 */ 1, {  8,  7 } },
    { /* 41 */ 1, { 11,  4 } },
    { /* 42 */ 1, {  0, 10 } },
    { /* 43 */ 1, {  7,  6 } },
    { /* 44 */ 1, { 12,  3 } },
    { /* 45 */ 1, { 10,  0 } },
    { /* 46 */ 1, { 10,  5 } },
    { /* 47 */ 1, {  4, 10 } },
    { /* 48 */ 1, {  6,  8 } },
    { /* 49 */ 1, {  2, 12 } },
    { /* 50 */ 1, {  9,  6 } },
    { /* 51 */ 1, {  9,  7 } },
    { /* 52 */ 1, {  4, 11 } },
    { /* 53 */ 1, { 11,  0 } },
    { /* 54 */ 1, {  6,  9 } },
    { /* 55 */ 1, {  3, 11 } },
    { /* 56 */ 1, {  5, 10 } },
    { /* 57 */ 0, { 20, 21 } },
    { /* 58 */ 0, { 21, 22 } },
    { /* 59 */ 0, { 22, 23 } },
    { /* 60 */ 0, { 23, 24 } },
    { /* 61 */ 0, { 24, 25 } },
    { /* 62 */ 0, { 25, 26 } },
    { /* 63 */ 0, { 26, 27 } },
    { /* 64 */ 0, { 27, 28 } },
    { /* 65 */ 0, { 28, 29 } },
    { /* 66 */ 0, { 29, 30 } },
    { /* 67 */ 0, { 30, 31 } },
    { /* 68 */ 0, { 31, 32 } },
    { /* 69 */ 0, { 32, 33 } },
    { /* 70 */ 0, { 33, 34 } },
    { /* 71 */ 0, { 34, 35 } },
    { /* 72 */ 0, { 35, 36 } },
    { /* 73 */ 0, { 36, 37 } },
    { /* 74 */ 0, { 37, 38 } },
    { /* 75 */ 0, { 38, 39 } },
    { /* 76 */ 0, { 39, 40 } },
    { /* 77 */ 1, {  8,  8 } },
    { /* 78 */ 1, {  7,  8 } },
    { /* 79 */ 1, { 12,  5 } },
    { /* 80 */ 1, {  3, 12 } },
    { /* 81 */ 1, { 11,  5 } },
    { /* 82 */ 1, {  7,  7 } },
    { /* 83 */ 1, { 12,  4 } },
    { /* 84 */ 1, { 11,  6 } },
    { /* 85 */ 1, { 10,  6 } },
    { /* 86 */ 1, {  4, 12 } },
    { /* 87 */ 1, {  7,  9 } },
    { /* 88 */ 1, {  5, 11 } },
    { /* 89 */ 1, {  0, 11 } },
    { /* 90 */ 1, { 12,  6 } },
    { /* 91 */ 1, {  6, 10 } },
    { /* 92 */ 1, { 12,  0 } },
    { /* 93 */ 1, { 10,  7 } },
    { /* 94 */ 1, {  5, 12 } },
    { /* 95 */ 1, {  7, 10 } },
    { /* 96 */ 1, {  9,  8 } },
    { /* 97 */ 1, {  0, 12 } },
    { /* 98 */ 1, { 11,  7 } },
    { /* 99 */ 1, {  8,  9 } },
    { /* 00 */ 1, {  9,  9 } },
    { /* 01 */ 1, { 10,  8 } },
    { /* 02 */ 1, {  7, 11 } },
    { /* 03 */ 1, { 12,  7 } },
    { /* 04 */ 1, {  6, 11 } },
    { /* 05 */ 1, {  8, 11 } },
    { /* 06 */ 1, { 11,  8 } },
    { /* 07 */ 1, {  7, 12 } },
    { /* 08 */ 1, {  6, 12 } },
    { /* 09 */ 0, { 8, 9 } },
    { /* 10 */ 0, { 9, 10 } },
    { /* 11 */ 0, { 10, 11 } },
    { /* 12 */ 0, { 11, 12 } },
    { /* 13 */ 0, { 12, 13 } },
    { /* 14 */ 0, { 13, 14 } },
    { /* 15 */ 0, { 14, 15 } },
    { /* 16 */ 0, { 15, 16 } },
    { /* 17 */ 1, {  8, 10 } },
    { /* 18 */ 1, { 10,  9 } },
    { /* 19 */ 1, {  8, 12 } },
    { /* 20 */ 1, {  9, 10 } },
    { /* 21 */ 1, {  9, 11 } },
    { /* 22 */ 1, {  9, 12 } },
    { /* 23 */ 1, { 10, 11 } },
    { /* 24 */ 1, { 12,  9 } },
    { /* 25 */ 1, { 10, 10 } },
    { /* 26 */ 1, { 11,  9 } },
    { /* 27 */ 1, { 12,  8 } },
    { /* 28 */ 1, { 11, 10 } },
    { /* 29 */ 1, { 12, 10 } },
    { /* 30 */ 1, { 12, 11 } },
    { /* 31 */ 0, { 2, 3 } },
    { /* 32 */ 0, { 3, 4 } },
    { /* 33 */ 1, { 10, 12 } },
    { /* 34 */ 1, { 11, 11 } },
    { /* 35 */ 1, { 11, 12 } },
    { /* 36 */ 1, { 12, 12 } }
};
static hcb_2_quad hcb1_2[] = {
    /* 1 bit codeword */
    { 1,  0,  0,  0,  0 },
	
    /* 5 bit codewords */
    { 5,  1,  0,  0,  0 },
    { 5, -1,  0,  0,  0 },
    { 5,  0,  0,  0, -1 },
    { 5,  0,  1,  0,  0 },
    { 5,  0,  0,  0,  1 },
    { 5,  0,  0, -1,  0 },
    { 5,  0,  0,  1,  0 },
    { 5,  0, -1,  0,  0 },
	
    /* 7 bit codewords */
    /* first 5 bits: 11000 */
    { 7,  1, -1,  0,  0 },
    { 7, -1,  1,  0,  0 },
    { 7,  0,  0, -1,  1 },
    { 7,  0,  1, -1,  0 },
    /* first 5 bits: 11001 */
    { 7,  0, -1,  1,  0 },
    { 7,  0,  0,  1, -1 },
    { 7,  1,  1,  0,  0 },
    { 7,  0,  0, -1, -1 },
    /* first 5 bits: 11010 */
    { 7, -1, -1,  0,  0 },
    { 7,  0, -1, -1,  0 },
    { 7,  1,  0, -1,  0 },
    { 7,  0,  1,  0, -1 },
    /* first 5 bits: 11011 */
    { 7, -1,  0,  1,  0 },
    { 7,  0,  0,  1,  1 },
    { 7,  1,  0,  1,  0 },
    { 7,  0, -1,  0,  1 },
    /* first 5 bits: 11100 */
    { 7,  0,  1,  1,  0 },
    { 7,  0,  1,  0,  1 },
    { 7, -1,  0, -1,  0 },
    { 7,  1,  0,  0,  1 },
    /* first 5 bits: 11101 */
    { 7, -1,  0,  0, -1 },
    { 7,  1,  0,  0, -1 },
    { 7, -1,  0,  0,  1 },
    { 7,  0, -1,  0, -1 },
	
    /* 9 bit codeword */
    /* first 5 bits: 11110 */
    { 9,  1,  1, -1,  0 },
    { 9, -1,  1, -1,  0 },
    { 9,  1, -1,  1,  0 },
    { 9,  0,  1,  1, -1 },
    { 9,  0,  1, -1,  1 },
    { 9,  0, -1,  1,  1 },
    { 9,  0, -1,  1, -1 },
    { 9,  1, -1, -1,  0 },
    { 9,  1,  0, -1,  1 },
    { 9,  0,  1, -1, -1 },
    { 9, -1,  1,  1,  0 },
    { 9, -1,  0,  1, -1 },
    { 9, -1, -1,  1,  0 },
    { 9,  0, -1, -1,  1 },
    { 9,  1, -1,  0,  1 },
    { 9,  1, -1,  0, -1 },
	
    /* 9/10/11 bit codewords */
    /* first 5 bits: 11111 */
    /* 9 bit: reading 11 bits -> 2 too much so 4 entries for each codeword */
    { 9, -1,  1,  0, -1 }, { 9, -1,  1,  0, -1 }, { 9, -1,  1,  0, -1 }, { 9, -1,  1,  0, -1 },
    { 9, -1, -1, -1,  0 }, { 9, -1, -1, -1,  0 }, { 9, -1, -1, -1,  0 }, { 9, -1, -1, -1,  0 },
    { 9,  0, -1, -1, -1 }, { 9,  0, -1, -1, -1 }, { 9,  0, -1, -1, -1 }, { 9,  0, -1, -1, -1 },
    { 9,  0,  1,  1,  1 }, { 9,  0,  1,  1,  1 }, { 9,  0,  1,  1,  1 }, { 9,  0,  1,  1,  1 },
    { 9,  1,  0,  1, -1 }, { 9,  1,  0,  1, -1 }, { 9,  1,  0,  1, -1 }, { 9,  1,  0,  1, -1 },
    { 9,  1,  1,  0,  1 }, { 9,  1,  1,  0,  1 }, { 9,  1,  1,  0,  1 }, { 9,  1,  1,  0,  1 },
    { 9, -1,  1,  0,  1 }, { 9, -1,  1,  0,  1 }, { 9, -1,  1,  0,  1 }, { 9, -1,  1,  0,  1 },
    { 9,  1,  1,  1,  0 }, { 9,  1,  1,  1,  0 }, { 9,  1,  1,  1,  0 }, { 9,  1,  1,  1,  0 },
    /* 10 bit: reading 11 bits -> 1 too much so 2 entries for each codeword */
    { 10, -1, -1,  0,  1 }, { 10, -1, -1,  0,  1 },
    { 10, -1,  0, -1, -1 }, { 10, -1,  0, -1, -1 },
    { 10,  1,  1,  0, -1 }, { 10,  1,  1,  0, -1 },
    { 10,  1,  0, -1, -1 }, { 10,  1,  0, -1, -1 },
    { 10, -1,  0, -1,  1 }, { 10, -1,  0, -1,  1 },
    { 10, -1, -1,  0, -1 }, { 10, -1, -1,  0, -1 },
    { 10, -1,  0,  1,  1 }, { 10, -1,  0,  1,  1 },
    { 10,  1,  0,  1,  1 }, { 10,  1,  0,  1,  1 },
    /* 11 bit */
    { 11,  1, -1,  1, -1 },
    { 11, -1,  1, -1,  1 },
    { 11, -1,  1,  1, -1 },
    { 11,  1, -1, -1,  1 },
    { 11,  1,  1,  1,  1 },
    { 11, -1, -1,  1,  1 },
    { 11,  1,  1, -1, -1 },
    { 11, -1, -1,  1, -1 },
    { 11, -1, -1, -1, -1 },
    { 11,  1,  1, -1,  1 },
    { 11,  1, -1,  1,  1 },
    { 11, -1,  1,  1,  1 },
    { 11, -1,  1, -1, -1 },
    { 11, -1, -1, -1,  1 },
    { 11,  1, -1, -1, -1 },
    { 11,  1,  1,  1, -1 }
};
static hcb_2_quad hcb2_2[] = {
    /* 3 bit codeword */
    { 3,  0,  0,  0,  0 },
	
    /* 4 bit codeword */
    { 4,  1,  0,  0,  0 },
	
    /* 5 bit codewords */
    { 5, -1,  0,  0,  0 },
    { 5,  0,  0,  0,  1 },
    { 5,  0,  0, -1,  0 },
    { 5,  0,  0,  0, -1 },
    { 5,  0, -1,  0,  0 },
    { 5,  0,  0,  1,  0 },
    { 5,  0,  1,  0,  0 },
	
    /* 6 bit codewords */
    { 6,  0, -1,  1,  0 },
    { 6, -1,  1,  0,  0 },
    { 6,  0,  1, -1,  0 },
    { 6,  0,  0,  1, -1 },
    { 6,  0,  1,  0, -1 },
    { 6,  0,  0, -1,  1 },
    { 6, -1,  0,  0, -1 },
    { 6,  1, -1,  0,  0 },
    { 6,  1,  0, -1,  0 },
    { 6, -1, -1,  0,  0 },
    { 6,  0,  0, -1, -1 },
    { 6,  1,  0,  1,  0 },
    { 6,  1,  0,  0,  1 },
    { 6,  0, -1,  0,  1 },
    { 6, -1,  0,  1,  0 },
    { 6,  0,  1,  0,  1 },
    { 6,  0, -1, -1,  0 },
    { 6, -1,  0,  0,  1 },
    { 6,  0, -1,  0, -1 },
    { 6, -1,  0, -1,  0 },
    { 6,  1,  1,  0,  0 },
    { 6,  0,  1,  1,  0 },
    { 6,  0,  0,  1,  1 },
    { 6,  1,  0,  0, -1 },
	
    /* 7 bit codewords */
    { 7,  0,  1, -1,  1 },
    { 7,  1,  0, -1,  1 },
    { 7, -1,  1, -1,  0 },
    { 7,  0, -1,  1, -1 },
    { 7,  1, -1,  1,  0 },
    { 7,  1,  1,  0, -1 },
    { 7,  1,  0,  1,  1 },
    { 7, -1,  1,  1,  0 },
    { 7,  0, -1, -1,  1 },
    { 7,  1,  1,  1,  0 },
    { 7, -1,  0,  1, -1 },
    { 7, -1, -1, -1,  0 },
	
    /* 7/8 bit codewords */
    { 7, -1,  0, -1,  1 }, { 7, -1,  0, -1,  1 },
    { 7,  1, -1, -1,  0 }, { 7,  1, -1, -1,  0 },
    { 7,  1,  1, -1,  0 }, { 7,  1,  1, -1,  0 },
    { 8,  1, -1,  0,  1 },
    { 8, -1,  1,  0, -1 },
	
    /* 8 bit codewords */
    { 8, -1, -1,  1,  0 },
    { 8, -1,  0,  1,  1 },
    { 8, -1, -1,  0,  1 },
    { 8, -1, -1,  0, -1 },
    { 8,  0, -1, -1, -1 },
    { 8,  1,  0,  1, -1 },
    { 8,  1,  0, -1, -1 },
    { 8,  0,  1, -1, -1 },
    { 8,  0,  1,  1,  1 },
    { 8, -1,  1,  0,  1 },
    { 8, -1,  0, -1, -1 },
    { 8,  0,  1,  1, -1 },
    { 8,  1, -1,  0, -1 },
    { 8,  0, -1,  1,  1 },
    { 8,  1,  1,  0,  1 },
    { 8,  1, -1,  1, -1 },
	
    /* 8/9 bit codewords */
    { 8, -1,  1, -1,  1 }, { 8, -1,  1, -1,  1 },
    { 9,  1, -1, -1,  1 },
    { 9, -1, -1, -1, -1 },
    { 9, -1,  1,  1, -1 },
    { 9, -1,  1,  1,  1 },
    { 9,  1,  1,  1,  1 },
    { 9, -1, -1,  1, -1 },
    { 9,  1, -1,  1,  1 },
    { 9, -1,  1, -1, -1 },
    { 9, -1, -1,  1,  1 },
    { 9,  1,  1, -1, -1 },
    { 9,  1, -1, -1, -1 },
    { 9, -1, -1, -1,  1 },
    { 9,  1,  1, -1,  1 },
    { 9,  1,  1,  1, -1 }
};
static hcb_2_quad hcb4_2[] = {
    /* 4 bit codewords */
    { 4,  1,  1,  1,  1 },
    { 4,  0,  1,  1,  1 },
    { 4,  1,  1,  0,  1 },
    { 4,  1,  1,  1,  0 },
    { 4,  1,  0,  1,  1 },
    { 4,  1,  0,  0,  0 },
    { 4,  1,  1,  0,  0 },
    { 4,  0,  0,  0,  0 },
    { 4,  0,  0,  1,  1 },
    { 4,  1,  0,  1,  0 },
	
    /* 5 bit codewords */
    { 5,  1,  0,  0,  1 },
    { 5,  0,  1,  1,  0 },
    { 5,  0,  0,  0,  1 },
    { 5,  0,  1,  0,  1 },
    { 5,  0,  0,  1,  0 },
    { 5,  0,  1,  0,  0 },
	
    /* 7 bit codewords */
    /* first 5 bits: 11010 */
    { 7,  2,  1,  1,  1 },
    { 7,  1,  1,  2,  1 },
    { 7,  1,  2,  1,  1 },
    { 7,  1,  1,  1,  2 },
    /* first 5 bits: 11011 */
    { 7,  2,  1,  1,  0 },
    { 7,  2,  1,  0,  1 },
    { 7,  1,  2,  1,  0 },
    { 7,  2,  0,  1,  1 },
	
    /* 7/8 bit codewords */
    /* first 5 bits: 11100 */
    { 7,  0,  1,  2,  1 }, { 7,  0,  1,  2,  1 },
    { 8,  0,  1,  1,  2 },
    { 8,  1,  1,  2,  0 },
    { 8,  0,  2,  1,  1 },
    { 8,  1,  0,  1,  2 },
    { 8,  1,  2,  0,  1 },
    { 8,  1,  1,  0,  2 },
	
    /* 8 bit codewords */
    { 8,  1,  0,  2,  1 },
    { 8,  2,  1,  0,  0 },
    { 8,  2,  0,  1,  0 },
    { 8,  1,  2,  0,  0 },
    { 8,  2,  0,  0,  1 },
    { 8,  0,  1,  0,  2 },
    { 8,  0,  2,  1,  0 },
    { 8,  0,  0,  1,  2 },
	
    /* 8/9 bit codewords */
    { 8,  0,  1,  2,  0 }, { 8,  0,  1,  2,  0 },
    { 8,  0,  2,  0,  1 }, { 8,  0,  2,  0,  1 },
    { 8,  1,  0,  0,  2 }, { 8,  1,  0,  0,  2 },
    { 8,  0,  0,  2,  1 }, { 8,  0,  0,  2,  1 },
    { 8,  1,  0,  2,  0 }, { 8,  1,  0,  2,  0 },
    { 8,  2,  0,  0,  0 }, { 8,  2,  0,  0,  0 },
    { 8,  0,  0,  0,  2 }, { 8,  0,  0,  0,  2 },
    { 9,  0,  2,  0,  0 },
    { 9,  0,  0,  2,  0 },
	
    /* 9/10/11 bit codewords */
    /* 9 bit codewords repeated 2^3 = 8 times */
    { 9,  1,  2,  2,  1 }, { 9,  1,  2,  2,  1 }, { 9,  1,  2,  2,  1 }, { 9,  1,  2,  2,  1 },
    { 9,  1,  2,  2,  1 }, { 9,  1,  2,  2,  1 }, { 9,  1,  2,  2,  1 }, { 9,  1,  2,  2,  1 },
    { 9,  2,  2,  1,  1 }, { 9,  2,  2,  1,  1 }, { 9,  2,  2,  1,  1 }, { 9,  2,  2,  1,  1 },
    { 9,  2,  2,  1,  1 }, { 9,  2,  2,  1,  1 }, { 9,  2,  2,  1,  1 }, { 9,  2,  2,  1,  1 },
    { 9,  2,  1,  2,  1 }, { 9,  2,  1,  2,  1 }, { 9,  2,  1,  2,  1 }, { 9,  2,  1,  2,  1 },
    { 9,  2,  1,  2,  1 }, { 9,  2,  1,  2,  1 }, { 9,  2,  1,  2,  1 }, { 9,  2,  1,  2,  1 },
    { 9,  1,  1,  2,  2 }, { 9,  1,  1,  2,  2 }, { 9,  1,  1,  2,  2 }, { 9,  1,  1,  2,  2 },
    { 9,  1,  1,  2,  2 }, { 9,  1,  1,  2,  2 }, { 9,  1,  1,  2,  2 }, { 9,  1,  1,  2,  2 },
    { 9,  1,  2,  1,  2 }, { 9,  1,  2,  1,  2 }, { 9,  1,  2,  1,  2 }, { 9,  1,  2,  1,  2 },
    { 9,  1,  2,  1,  2 }, { 9,  1,  2,  1,  2 }, { 9,  1,  2,  1,  2 }, { 9,  1,  2,  1,  2 },
    { 9,  2,  1,  1,  2 }, { 9,  2,  1,  1,  2 }, { 9,  2,  1,  1,  2 }, { 9,  2,  1,  1,  2 },
    { 9,  2,  1,  1,  2 }, { 9,  2,  1,  1,  2 }, { 9,  2,  1,  1,  2 }, { 9,  2,  1,  1,  2 },
    /* 10 bit codewords repeated 2^2 = 4 times */
    { 10,  1,  2,  2,  0 }, { 10,  1,  2,  2,  0 }, { 10,  1,  2,  2,  0 }, { 10,  1,  2,  2,  0 },
    { 10,  2,  2,  1,  0 }, { 10,  2,  2,  1,  0 }, { 10,  2,  2,  1,  0 }, { 10,  2,  2,  1,  0 },
    { 10,  2,  1,  2,  0 }, { 10,  2,  1,  2,  0 }, { 10,  2,  1,  2,  0 }, { 10,  2,  1,  2,  0 },
    { 10,  0,  2,  2,  1 }, { 10,  0,  2,  2,  1 }, { 10,  0,  2,  2,  1 }, { 10,  0,  2,  2,  1 },
    { 10,  0,  1,  2,  2 }, { 10,  0,  1,  2,  2 }, { 10,  0,  1,  2,  2 }, { 10,  0,  1,  2,  2 },
    { 10,  2,  2,  0,  1 }, { 10,  2,  2,  0,  1 }, { 10,  2,  2,  0,  1 }, { 10,  2,  2,  0,  1 },
    { 10,  0,  2,  1,  2 }, { 10,  0,  2,  1,  2 }, { 10,  0,  2,  1,  2 }, { 10,  0,  2,  1,  2 },
    { 10,  2,  0,  2,  1 }, { 10,  2,  0,  2,  1 }, { 10,  2,  0,  2,  1 }, { 10,  2,  0,  2,  1 },
    { 10,  1,  0,  2,  2 }, { 10,  1,  0,  2,  2 }, { 10,  1,  0,  2,  2 }, { 10,  1,  0,  2,  2 },
    { 10,  2,  2,  2,  1 }, { 10,  2,  2,  2,  1 }, { 10,  2,  2,  2,  1 }, { 10,  2,  2,  2,  1 },
    { 10,  1,  2,  0,  2 }, { 10,  1,  2,  0,  2 }, { 10,  1,  2,  0,  2 }, { 10,  1,  2,  0,  2 },
    { 10,  2,  0,  1,  2 }, { 10,  2,  0,  1,  2 }, { 10,  2,  0,  1,  2 }, { 10,  2,  0,  1,  2 },
    { 10,  2,  1,  0,  2 }, { 10,  2,  1,  0,  2 }, { 10,  2,  1,  0,  2 }, { 10,  2,  1,  0,  2 },
    { 10,  1,  2,  2,  2 }, { 10,  1,  2,  2,  2 }, { 10,  1,  2,  2,  2 }, { 10,  1,  2,  2,  2 },
    /* 11 bit codewords repeated 2^1 = 2 times */
    { 11,  2,  1,  2,  2 }, { 11,  2,  1,  2,  2 },
    { 11,  2,  2,  1,  2 }, { 11,  2,  2,  1,  2 },
    { 11,  0,  2,  2,  0 }, { 11,  0,  2,  2,  0 },
    { 11,  2,  2,  0,  0 }, { 11,  2,  2,  0,  0 },
    { 11,  0,  0,  2,  2 }, { 11,  0,  0,  2,  2 },
    { 11,  2,  0,  2,  0 }, { 11,  2,  0,  2,  0 },
    { 11,  0,  2,  0,  2 }, { 11,  0,  2,  0,  2 },
    { 11,  2,  0,  0,  2 }, { 11,  2,  0,  0,  2 },
    { 11,  2,  2,  2,  2 }, { 11,  2,  2,  2,  2 },
    { 11,  0,  2,  2,  2 }, { 11,  0,  2,  2,  2 },
    { 11,  2,  2,  2,  0 }, { 11,  2,  2,  2,  0 },
    /* 12 bit codewords */
    { 12,  2,  2,  0,  2 },
    { 12,  2,  0,  2,  2 },
};
static hcb_bin_quad hcb3[] = {
    { /*  0 */ 0, {  1,  2, 0, 0 } },
    { /*  1 */ 1, {  0,  0, 0, 0 } }, /* 0 */
    { /*  2 */ 0, {  1,  2, 0, 0 } },
    { /*  3 */ 0, {  2,  3, 0, 0 } },
    { /*  4 */ 0, {  3,  4, 0, 0 } },
    { /*  5 */ 0, {  4,  5, 0, 0 } },
    { /*  6 */ 0, {  5,  6, 0, 0 } },
    { /*  7 */ 0, {  6,  7, 0, 0 } },
    { /*  8 */ 0, {  7,  8, 0, 0 } },
    { /*  9 */ 1, {  1,  0, 0, 0 } }, /* 1000 */
    { /* 10 */ 1, {  0,  0, 0, 1 } }, /* 1001 */
    { /* 11 */ 1, {  0,  1, 0, 0 } }, /* 1010 */
    { /* 12 */ 1, {  0,  0, 1, 0 } }, /* 1011 */
    { /* 13 */ 0, {  4,  5, 0, 0 } },
    { /* 14 */ 0, {  5,  6, 0, 0 } },
    { /* 15 */ 0, {  6,  7, 0, 0 } },
    { /* 16 */ 0, {  7,  8, 0, 0 } },
    { /* 17 */ 1, {  1,  1, 0, 0 } },
    { /* 18 */ 1, {  0,  0, 1, 1 } },
    { /* 19 */ 0, {  6,  7, 0, 0 } },
    { /* 20 */ 0, {  7,  8, 0, 0 } },
    { /* 21 */ 0, {  8,  9, 0, 0 } },
    { /* 22 */ 0, {  9, 10, 0, 0 } },
    { /* 23 */ 0, { 10, 11, 0, 0 } },
    { /* 24 */ 0, { 11, 12, 0, 0 } },
    { /* 25 */ 1, {  0,  1, 1, 0 } }, /* 110100 */
    { /* 26 */ 1, {  0,  1, 0, 1 } }, /* 110101 */
    { /* 27 */ 1, {  1,  0, 1, 0 } }, /* 110110 */
    { /* 28 */ 1, {  0,  1, 1, 1 } }, /* 110111 */
    { /* 29 */ 1, {  1,  0, 0, 1 } }, /* 111000 */
    { /* 30 */ 1, {  1,  1, 1, 0 } }, /* 111001 */
    { /* 31 */ 0, {  6,  7, 0, 0 } },
    { /* 32 */ 0, {  7,  8, 0, 0 } },
    { /* 33 */ 0, {  8,  9, 0, 0 } },
    { /* 34 */ 0, {  9, 10, 0, 0 } },
    { /* 35 */ 0, { 10, 11, 0, 0 } },
    { /* 36 */ 0, { 11, 12, 0, 0 } },
    { /* 37 */ 1, {  1,  1, 1, 1 } }, /* 1110100 */
    { /* 38 */ 1, {  1,  0, 1, 1 } }, /* 1110101 */
    { /* 39 */ 1, {  1,  1, 0, 1 } }, /* 1110110 */
    { /* 40 */ 0, {  9, 10, 0, 0 } },
    { /* 41 */ 0, { 10, 11, 0, 0 } },
    { /* 42 */ 0, { 11, 12, 0, 0 } },
    { /* 43 */ 0, { 12, 13, 0, 0 } },
    { /* 44 */ 0, { 13, 14, 0, 0 } },
    { /* 45 */ 0, { 14, 15, 0, 0 } },
    { /* 46 */ 0, { 15, 16, 0, 0 } },
    { /* 47 */ 0, { 16, 17, 0, 0 } },
    { /* 48 */ 0, { 17, 18, 0, 0 } },
    { /* 49 */ 1, {  2,  0, 0, 0 } }, /* 11101110 */
    { /* 50 */ 1, {  0,  0, 0, 2 } }, /* 11101111 */
    { /* 51 */ 1, {  0,  0, 1, 2 } }, /* 11110000 */
    { /* 52 */ 1, {  2,  1, 0, 0 } }, /* 11110001 */
    { /* 53 */ 1, {  1,  2, 1, 0 } }, /* 11110010 */
    { /* 54 */ 0, { 13, 14, 0, 0 } },
    { /* 55 */ 0, { 14, 15, 0, 0 } },
    { /* 56 */ 0, { 15, 16, 0, 0 } },
    { /* 57 */ 0, { 16, 17, 0, 0 } },
    { /* 58 */ 0, { 17, 18, 0, 0 } },
    { /* 59 */ 0, { 18, 19, 0, 0 } },
    { /* 60 */ 0, { 19, 20, 0, 0 } },
    { /* 61 */ 0, { 20, 21, 0, 0 } },
    { /* 62 */ 0, { 21, 22, 0, 0 } },
    { /* 63 */ 0, { 22, 23, 0, 0 } },
    { /* 64 */ 0, { 23, 24, 0, 0 } },
    { /* 65 */ 0, { 24, 25, 0, 0 } },
    { /* 66 */ 0, { 25, 26, 0, 0 } },
    { /* 67 */ 1, {  0,  0, 2, 1 } },
    { /* 68 */ 1, {  0,  1, 2, 1 } },
    { /* 69 */ 1, {  1,  2, 0, 0 } },
    { /* 70 */ 1, {  0,  1, 1, 2 } },
    { /* 71 */ 1, {  2,  1, 1, 0 } },
    { /* 72 */ 1, {  0,  0, 2, 0 } },
    { /* 73 */ 1, {  0,  2, 1, 0 } },
    { /* 74 */ 1, {  0,  1, 2, 0 } },
    { /* 75 */ 1, {  0,  2, 0, 0 } },
    { /* 76 */ 1, {  0,  1, 0, 2 } },
    { /* 77 */ 1, {  2,  0, 1, 0 } },
    { /* 78 */ 1, {  1,  2, 1, 1 } },
    { /* 79 */ 1, {  0,  2, 1, 1 } },
    { /* 80 */ 1, {  1,  1, 2, 0 } },
    { /* 81 */ 1, {  1,  1, 2, 1 } },
    { /* 82 */ 0, { 11, 12, 0, 0 } },
    { /* 83 */ 0, { 12, 13, 0, 0 } },
    { /* 84 */ 0, { 13, 14, 0, 0 } },
    { /* 85 */ 0, { 14, 15, 0, 0 } },
    { /* 86 */ 0, { 15, 16, 0, 0 } },
    { /* 87 */ 0, { 16, 17, 0, 0 } },
    { /* 88 */ 0, { 17, 18, 0, 0 } },
    { /* 89 */ 0, { 18, 19, 0, 0 } },
    { /* 90 */ 0, { 19, 20, 0, 0 } },
    { /* 91 */ 0, { 20, 21, 0, 0 } },
    { /* 92 */ 0, { 21, 22, 0, 0 } },
    { /* 93 */ 1, {  1,  2,  0,  1 } }, /* 1111101010 */
    { /* 94 */ 1, {  1,  0,  2,  0 } }, /* 1111101011 */
    { /* 95 */ 1, {  1,  0,  2,  1 } }, /* 1111101100 */
    { /* 96 */ 1, {  0,  2,  0,  1 } }, /* 1111101101 */
    { /* 97 */ 1, {  2,  1,  1,  1 } }, /* 1111101110 */
    { /* 98 */ 1, {  1,  1,  1,  2 } }, /* 1111101111 */
    { /* 99 */ 1, {  2,  1,  0,  1 } }, /* 1111110000 */
    { /* 00 */ 1, {  1,  0,  1,  2 } }, /* 1111110001 */
    { /* 01 */ 1, {  0,  0,  2,  2 } }, /* 1111110010 */
    { /* 02 */ 1, {  0,  1,  2,  2 } }, /* 1111110011 */
    { /* 03 */ 1, {  2,  2,  1,  0 } }, /* 1111110100 */
    { /* 04 */ 1, {  1,  2,  2,  0 } }, /* 1111110101 */
    { /* 05 */ 1, {  1,  0,  0,  2 } }, /* 1111110110 */
    { /* 06 */ 1, {  2,  0,  0,  1 } }, /* 1111110111 */
    { /* 07 */ 1, {  0,  2,  2,  1 } }, /* 1111111000 */
    { /* 08 */ 0, {  7,  8, 0, 0 } },
    { /* 09 */ 0, {  8,  9, 0, 0 } },
    { /* 10 */ 0, {  9, 10, 0, 0 } },
    { /* 11 */ 0, { 10, 11, 0, 0 } },
    { /* 12 */ 0, { 11, 12, 0, 0 } },
    { /* 13 */ 0, { 12, 13, 0, 0 } },
    { /* 14 */ 0, { 13, 14, 0, 0 } },
    { /* 15 */ 1, {  2,  2,  0,  0 } }, /* 11111110010 */
    { /* 16 */ 1, {  1,  2,  2,  1 } }, /* 11111110011 */
    { /* 17 */ 1, {  1,  1,  0,  2 } }, /* 11111110100 */
    { /* 18 */ 1, {  2,  0,  1,  1 } }, /* 11111110101 */
    { /* 19 */ 1, {  1,  1,  2,  2 } }, /* 11111110110 */
    { /* 20 */ 1, {  2,  2,  1,  1 } }, /* 11111110111 */
    { /* 21 */ 1, {  0,  2,  2,  0 } }, /* 11111111000 */
    { /* 22 */ 1, {  0,  2,  1,  2 } }, /* 11111111001 */
    { /* 23 */ 0, {  6,  7, 0, 0 } },
    { /* 24 */ 0, {  7,  8, 0, 0 } },
    { /* 25 */ 0, {  8,  9, 0, 0 } },
    { /* 26 */ 0, {  9, 10, 0, 0 } },
    { /* 27 */ 0, { 10, 11, 0, 0 } },
    { /* 28 */ 0, { 11, 12, 0, 0 } },
    { /* 29 */ 1, {  1,  0,  2,  2 } }, /* 111111110100 */
    { /* 30 */ 1, {  2,  2,  0,  1 } }, /* 111111110101 */
    { /* 31 */ 1, {  2,  1,  2,  0 } }, /* 111111110110 */
    { /* 32 */ 1, {  2,  2,  2,  0 } }, /* 111111110111 */
    { /* 33 */ 1, {  0,  2,  2,  2 } }, /* 111111111000 */
    { /* 34 */ 1, {  2,  2,  2,  1 } }, /* 111111111001 */
    { /* 35 */ 1, {  2,  1,  2,  1 } }, /* 111111111010 */
    { /* 36 */ 1, {  1,  2,  1,  2 } }, /* 111111111011 */
    { /* 37 */ 1, {  1,  2,  2,  2 } }, /* 111111111100 */
    { /* 38 */ 0, {  3,  4, 0, 0 } },
    { /* 39 */ 0, {  4,  5, 0, 0 } },
    { /* 40 */ 0, {  5,  6, 0, 0 } },
    { /* 41 */ 1, {  0,  2,  0,  2 } }, /* 1111111111010 */
    { /* 42 */ 1, {  2,  0,  2,  0 } }, /* 1111111111011 */
    { /* 43 */ 1, {  1,  2,  0,  2 } }, /* 1111111111100 */
    { /* 44 */ 0, {  3, 4, 0, 0 } },
    { /* 45 */ 0, {  4, 5, 0, 0 } },
    { /* 46 */ 0, {  5, 6, 0, 0 } },
    { /* 47 */ 1, {  2,  0,  2,  1 } }, /* 11111111111010 */
    { /* 48 */ 1, {  2,  1,  1,  2 } }, /* 11111111111011 */
    { /* 49 */ 1, {  2,  1,  0,  2 } }, /* 11111111111100 */
    { /* 50 */ 0, { 3, 4, 0, 0 } },
    { /* 51 */ 0, { 4, 5, 0, 0 } },
    { /* 52 */ 0, { 5, 6, 0, 0 } },
    { /* 53 */ 1, {  2,  2,  2,  2 } }, /* 111111111111010 */
    { /* 54 */ 1, {  2,  2,  1,  2 } }, /* 111111111111011 */
    { /* 55 */ 1, {  2,  1,  2,  2 } }, /* 111111111111100 */
    { /* 56 */ 1, {  2,  0,  1,  2 } }, /* 111111111111101 */
    { /* 57 */ 1, {  2,  0,  0,  2 } }, /* 111111111111110 */
    { /* 58 */ 0, { 1, 2, 0, 0 } },
    { /* 59 */ 1, {  2,  2,  0,  2 } }, /* 1111111111111110 */
    { /* 60 */ 1, {  2,  0,  2,  2 } }  /* 1111111111111111 */
};

hcb *hcb_table[] = {
    0, hcb1_1, hcb2_1, 0, hcb4_1, 0, hcb6_1, 0, hcb8_1, 0, hcb10_1, hcb11_1
};
hcb_2_pair *hcb_2_pair_table[] = {
    0, 0, 0, 0, 0, 0, hcb6_2, 0, hcb8_2, 0, hcb10_2, hcb11_2
};
hcb_bin_pair *hcb_bin_table[] = {
    0, 0, 0, 0, 0, hcb5, 0, hcb7, 0, hcb9, 0, 0
};
hcb_2_quad *hcb_2_quad_table[] = {
    0, hcb1_2, hcb2_2, 0, hcb4_2, 0, 0, 0, 0, 0, 0, 0
};

#define huffman_2step_pair(cb, sp)	\
{	unsigned int cw;				\
	unsigned short offset = 0;		\
	unsigned extra_bits;			\
	ShowBits(cw, hcbN[cb]);			\
	offset = hcb_table[cb][cw].offset;	\
	extra_bits = hcb_table[cb][cw].extra_bits;	\
	if (extra_bits)					\
	{								\
		PopBits(s, tmp, hcbN[cb]);	\
		ShowBits(tmp, extra_bits);	\
	    offset += tmp;				\
		PopBits(s, tmp, (hcb_2_pair_table[cb][offset].bits - hcbN[cb]));	\
	}								\
	else							\
		PopBits(s, tmp, hcb_2_pair_table[cb][offset].bits) \
	sp[0] = hcb_2_pair_table[cb][offset].x;	\
	sp[1] = hcb_2_pair_table[cb][offset].y;	\
}

#define huffman_sign_bits(sp, len)	\
{	unsigned char i;				\
	for (i=0; i<len; i++)			\
	{	if (sp[i])					\
		{	PopBits(s, tmp, 1);		\
			if (tmp) sp[i] = -sp[i];\
		}							\
	}								\
}

#define huffman_2step_pair_sign(cb, sp, len)	\
	huffman_2step_pair(cb, sp)	\
	huffman_sign_bits(sp, len)

#define huffman_getescap(sp)		\
{	unsigned char neg, i;			\
	short j, off;					\
	if (sp==-16 || sp==16)			\
	{	if (sp < 0) neg=1;			\
		else neg=0;					\
		for (i=4; ; i++)			\
		{	PopBits(s, tmp, 1)		\
			if (tmp == 0) break;	\
		}							\
		PopBits(s, off, i);			\
		j = off | (1<<i);			\
		if (neg) j = -j; sp = j;	\
	}								\
}

#define huffman_binary_pair(cb, sp)	\
{	unsigned short offset = 0;		\
	while(!hcb_bin_table[cb][offset].is_leaf)	\
	{	unsigned char b;						\
		PopBits(s, b, 1);						\
		offset += hcb_bin_table[cb][offset].data[b];	\
	}													\
	sp[0] = hcb_bin_table[cb][offset].data[0];	\
	sp[1] = hcb_bin_table[cb][offset].data[1];	\
}

#define huffman_2step_quad(cb, sp)	\
{	unsigned int cw;				\
	unsigned short offset = 0;		\
	unsigned char extra_bits;		\
	ShowBits(cw, hcbN[cb]);			\
	offset = hcb_table[cb][cw].offset;	\
	extra_bits = hcb_table[cb][cw].extra_bits;	\
	if (extra_bits) {				\
		PopBits(s, tmp, hcbN[cb]);	\
		ShowBits(tmp, extra_bits); offset += tmp;	\
		PopBits(s, tmp, (hcb_2_quad_table[cb][offset].bits - hcbN[cb]));	\
	} else PopBits(s, tmp, hcb_2_quad_table[cb][offset].bits);	\
	sp[0] = hcb_2_quad_table[cb][offset].x;	\
	sp[1] = hcb_2_quad_table[cb][offset].y;	\
	sp[2] = hcb_2_quad_table[cb][offset].v;	\
	sp[3] = hcb_2_quad_table[cb][offset].w;	\
}

#define huffman_binary_quad(cb, sp)	\
{	unsigned short offset = 0;		\
	while(!hcb3[offset].is_leaf)	\
	{	unsigned char b;			\
		PopBits(s, b, 1);			\
		offset += hcb3[offset].data[b];	\
	}								\
	sp[0] = hcb3[offset].data[0];	\
	sp[1] = hcb3[offset].data[1];	\
	sp[2] = hcb3[offset].data[2];	\
	sp[3] = hcb3[offset].data[3];	\
}

#define huffman_binary_pair_sign(cb, sp, len)	\
	huffman_binary_pair(cb, sp)	\
	huffman_sign_bits(sp, len)

#define huffman_2step_quad_sign(cb, sp, len)	\
	huffman_2step_quad(cb, sp)	\
	huffman_sign_bits(sp, len)

#define huffman_binary_quad_sign(cb, sp, len)	\
	huffman_binary_quad(cb, sp)	\
	huffman_sign_bits(sp, len)

#define huffman_codebook(num, sp)	\
{	static const unsigned int data = 16428320;	\
	if (num == 0) sp = (short)(data >> 16) & 0xFFFF;	\
	else		  sp = (short)data & 0xFFFF;	\
}

static int huffman_spectral_data(stream *ld, icstream *ics, unsigned char cb)
{
	short sp[1024];
	unsigned int tmp;
	READER_VAR
	RESET_S
	
	switch (cb) {
		case 1:
		case 2:
			huffman_2step_quad(cb, sp);
			break;
		case 3:
			huffman_binary_quad_sign(cb, sp, QUAD_LEN);
			break;
		case 4:
			huffman_2step_quad_sign(cb, sp, QUAD_LEN);
			break;
		case 5:
			huffman_binary_pair(cb, sp);
			break;
		case 6:
			huffman_2step_pair(cb, sp);
			break;
		case 7:
		case 9:
			huffman_binary_pair_sign(cb, sp, PAIR_LEN);
			break;
		case 8:
		case 10:
			huffman_2step_pair_sign(cb, sp, PAIR_LEN);
			break;
		case 12:
			huffman_2step_pair(11, sp);
			huffman_codebook(0, sp[0]);
			huffman_codebook(1, sp[1]);
			break;
		case 11:
			huffman_2step_pair_sign(cb, sp, PAIR_LEN);
			huffman_getescap(sp[0]);
			huffman_getescap(sp[1]);
			break;
		default:
			;
	}

	STORE_S
	return 0;
}

static int tns_data(stream *ld, icstream *ics)
{
	unsigned char w, filt, i, start_coef_bits, coef_bits;
	unsigned char n_filt_bits = 2, length_bits = 6, order_bits = 5;
	unsigned char tns_n_filt, tns_coef_res, tns_order, tns_coef_compress;
	READER_VAR
	RESET_S
	
	if (ics->window_sequence == EIGHT_SHORT_SEQUENCE) {
		n_filt_bits = 1;
        length_bits = 4;
        order_bits = 3;
	}
	
	for (w=0; w<ics->num_windows; w++) {
		PopBits(s, tns_n_filt, n_filt_bits);
		if (tns_n_filt) {
			PopBits(s, tns_coef_res, 1);
			if (tns_coef_res) start_coef_bits = 4;
			else start_coef_bits = 3;
		}
		for (filt=0; filt<tns_n_filt; filt++) {
			SkipBits(s, length_bits); // tns_length
			PopBits(s, tns_order, order_bits);
			if (tns_order) {
				SkipBits(s, 1); // tns_direction
				PopBits(s, tns_coef_compress, 1);
				coef_bits = start_coef_bits - tns_coef_compress;
				for (i=0; i<tns_order; i++) {
					SkipBits(s, coef_bits);     // tns_coef
				}
			}
		}
	}
			
	STORE_S
	return 0;
}

static int individual_channel_stream(stream *ld, element *ele, icstream *ics, int sf_index)
{
	unsigned char scal_flag = 0, g, sfb, groups = 0, sect_cb;
	unsigned char ele_common_window;
	unsigned char sect_esc_val, sect_bits;
	unsigned short k = 0, p = 0, frameLength = 1024, nshort, inc;
	short i;
	READER_VAR
	RESET_S
	
	ele_common_window = ele->common_window;
	nshort = frameLength/8;	
	
	//side_info START ...
	SkipBits(s, 8); // global_gain
	if (!ele_common_window && !scal_flag)
	{
		//ics_info
		SkipBits(s, 1); //reserved_bit
		PopBits(s, ics->window_sequence, 2);
		PopBits(s, ics->window_shape, 1);
		
		if (ics->window_sequence == EIGHT_SHORT_SEQUENCE) {
			PopBits(s, ics->max_sfb, 4);
			PopBits(s, ics->scale_factor_grouping, 7);
		}
		else {
			PopBits(s, ics->max_sfb, 6);
		}
		
		window_grouping_info(ics, sf_index);
		
		if (ics->window_sequence != EIGHT_SHORT_SEQUENCE)
		{
			PopBits(s, ics->pred_data_present, 1);
			if (ics->pred_data_present) {
				return -1;//AAC predictor, not supported, implement if need.
			}
		}
		//ics_info ending
	}
	
	//section_data
	if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
		sect_bits = 3;
	else {
		sect_bits = 5;
	}
	sect_esc_val = (1<<sect_bits) - 1;
	
	for (g=0; g<ics->num_window_groups; g++) {
		unsigned char k = 0;
        unsigned char i = 0;
		while (k < ics->max_sfb) {
			unsigned char sect_len_incr, sect_cb_bits = 4;
            unsigned short sect_len = 0;
			
			PopBits(s, ics->sect_cb[g][i], sect_cb_bits);
			PopBits(s, sect_len_incr, sect_bits);
			while (sect_len_incr == sect_esc_val) {
				sect_len += sect_len_incr;
				PopBits(s, sect_len_incr, sect_bits);
			}
			sect_len += sect_len_incr;
			ics->sect_start[g][i] = k;
            ics->sect_end[g][i] = k + sect_len;
			
			for (sfb=k; sfb<(unsigned char)(k+sect_len); sfb++)
				ics->sfb_cb[g][sfb] = ics->sect_cb[g][i];

			k += sect_len; i++;
		}
		ics->num_sec[g] = i;
	}
	
	//section_factor_data
	//......
	//decode_scale_factors
	for (g=0; g<ics->num_window_groups; g++) {
		for (sfb=0; sfb<ics->max_sfb; sfb++) {
			switch (ics->sfb_cb[g][sfb]) {
				case ZERO_HCB:
					break;
					
				case NOISE_HCB:
					return -1;//Noise DRM not supported, implement if need.
					//break;
					
				case INTENSITY_HCB:
				case INTENSITY_HCB2:
				default:
				{
					unsigned short offset = 0;
					while (hcb_sf[offset][1])
					{
						unsigned char b;
						PopBits(s, b, 1);
						offset += hcb_sf[offset][b];
						
						if (offset > 240)
							return -1;
					}
					break;
				}
			}
		}
	}	
	
	if (!scal_flag)
	{
		PopBits(s, ics->pulse_data_present, 1);
		if (ics->pulse_data_present) {
			return -1;//pulse_data
		}
		
		PopBits(s, ics->tns_data_present, 1);
		if (ics->tns_data_present) {
			STORE_S
			tns_data(ld, ics);
			RESET_S
		}
		
		PopBits(s, ics->gain_control_data_present, 1);
		if (ics->gain_control_data_present) {
			return -1;//gain_control_data
		}
	}
	;//Error Resilience
	
	//side_info END ...
	
	
	;//object_type > ER_OBJECT_TYPE
	;//Error Resilience
	
	//spectral_data
	for (g=0; g<ics->num_window_groups; g++) {
		p = groups * nshort;
		for (i=0; i<ics->num_sec[g]; i++) {
			sect_cb = ics->sect_cb[g][i];
			inc = (sect_cb >= FIRST_PAIR_HCB) ? 2 : 4;
			
			switch (sect_cb) {
				case ZERO_HCB:
				case NOISE_HCB:
				case INTENSITY_HCB:
				case INTENSITY_HCB2:
					p += (ics->sect_sfb_offset[g][ics->sect_end[g][i]] - ics->sect_sfb_offset[g][ics->sect_start[g][i]]);
					break;
					
				default:
					for (k = ics->sect_sfb_offset[g][ics->sect_start[g][i]]; k < ics->sect_sfb_offset[g][ics->sect_end[g][i]]; k += inc)
					{
						STORE_S
						huffman_spectral_data(ld, ics, sect_cb);
						RESET_S
					}
					break;
			}
		}
		groups += ics->window_group_length[g];
	}
	if (ics->pulse_data_present) {
		if (ics->window_sequence != EIGHT_SHORT_SEQUENCE) {
			return -1;//pulse_decode, not supported, implement if need.
		}
		else {
			return 2; /*pulse coding not allowed for short blocks*/
		}
	}
	
	STORE_S
	return 0;
}

static int decode_cpe(stream *ld, SInt32 *sbr_present_flag, int sf_index ) //channel pair element
{
	unsigned char cpe_common_window;
	unsigned short tmp;
	int err = 0;
	element elemt={0}, *ele = &elemt;
	icstream istm1={0}, *ics1 = &istm1;
	icstream istm2={0}, *ics2 = &istm2;
	READER_VAR
	RESET_S
	
	ics1->num_window_groups = 1;
	
	//channel_pair_element START ...
	SkipBits(s, LEN_TAG); // tag
	PopBits(s, cpe_common_window, 1);
	if (cpe_common_window) {
		//ics_info
		PopBits(s, tmp, 1); //reserved_bit
		PopBits(s, ics1->window_sequence, 2);
		PopBits(s, ics1->window_shape, 1);
		
		if (ics1->window_sequence == EIGHT_SHORT_SEQUENCE) {
			PopBits(s, ics1->max_sfb, 4);
			PopBits(s, ics1->scale_factor_grouping, 7);
		}
		else {
			PopBits(s, ics1->max_sfb, 6);
		}
		
		window_grouping_info(ics1, sf_index);
		
		if (ics1->window_sequence != EIGHT_SHORT_SEQUENCE)
		{
			PopBits(s, ics1->pred_data_present, 1);
			if (ics1->pred_data_present) {
				return -1;//AAC predictor doesn't support
			}
		}
		//ics_info ending
		
		PopBits(s, ics1->ms_mask_present, 2);
		if (ics1->ms_mask_present == 1) {
			unsigned char g, sfb;
            for (g = 0; g < ics1->num_window_groups; g++)
            {
                for (sfb = 0; sfb < ics1->max_sfb; sfb++)
                {
                    PopBits(s, ics1->ms_used[g][sfb], 1);
                }
            }
		}
		;//Error Resilience
		;//LTP_DEC
		FskMemCopy(ics2, ics1, sizeof(icstream));
	}
	else {
		ics1->ms_mask_present = 0;
	}

	ele->common_window = cpe_common_window;
	
	STORE_S
	err = individual_channel_stream(ld, ele, ics1, sf_index);
    BAIL_IF_ERR(err);
	RESET_S
	
	;//Error Resilience
	;//LTP_DEC
	
	STORE_S
	err = individual_channel_stream(ld, ele, ics2, sf_index);
    BAIL_IF_ERR(err);
	RESET_S

	//check fill element
	ShowBits(tmp, LEN_SE_ID);
	if (tmp == ID_FIL) {
		PopBits(s, tmp, LEN_SE_ID);
		
		STORE_S
		fill_element(ld, sbr_present_flag);
		RESET_S
	}
	//channel_pair_element END ...

bail:
	STORE_S
	return err;
}

static int decode_sce_lfe(stream *ld, SInt32 *sbr_present_flag, int sf_index ) //single channel element,
{
	unsigned short tmp;
	int err = 0;
	element elemt={0}, *ele = &elemt;
	icstream istm={0}, *ics = &istm;
	READER_VAR
	RESET_S
	
	ics->num_window_groups = 1;
	
	//single_lfe_channel_element START ...
	SkipBits(s, LEN_TAG); // tag
	
	ele->common_window = 0;
	
	STORE_S
	err = individual_channel_stream(ld, ele, ics, sf_index);
    BAIL_IF_ERR(err);
	RESET_S
	
	//check fill element
	ShowBits(tmp, LEN_SE_ID);
	if (tmp == ID_FIL) {
		PopBits(s, tmp, LEN_SE_ID);
		
		STORE_S
		fill_element(ld, sbr_present_flag);
		RESET_S
	}
	//single_lfe_channel_element END ...

bail:
	STORE_S
	return err;
}

static void get_sample_frequency_index(int sample_rate, int *sf_index)
{
	if (sample_rate > 88200)
		*sf_index = 0;
	else if (sample_rate > 64000)
		*sf_index = 1;
	else if (sample_rate > 48000)
		*sf_index = 2;
	else if (sample_rate > 44100)
		*sf_index = 3;
	else if (sample_rate > 32000)
		*sf_index = 4;
	else if (sample_rate > 24000)
		*sf_index = 5;
	else if (sample_rate > 22050)
		*sf_index = 6;
	else if (sample_rate > 16000)
		*sf_index = 7;
	else if (sample_rate > 12000)
		*sf_index = 8;
	else if (sample_rate > 11025)
		*sf_index = 9;
	else if (sample_rate > 8000)
		*sf_index = 10;
	else
		*sf_index = 11;
	
	return;
}

QTErr get_extended_aac_profile_level( const unsigned char *s, UInt32 sample_rate, UInt32 channel_total, UInt32 *sample_rate_ext, SInt32 *sbr_present_flag, UInt32 *profile, UInt32 *level )
{
	int    pcu, rcu;
	int     dontUpSampleImplicitSBR=0,/*forceUpSampling=0, */downSampledSBR=0;
	unsigned char id_syn_ele;
	unsigned int   bits32    = (s[0]<<24)|(s[1]<<16)|(s[2]<<8)|(s[3]<<0);															
	unsigned int   bitOffset = 0;
	int				err = 0;
	int				sf_index;
	int				play_sample_rate = sample_rate;
    //int             sampling_frequency_index;
	stream	str, *ld = &str;
	
	get_sample_frequency_index(sample_rate, &sf_index);
	//sampling_frequency_index = sf_index;
	
	/*if (sample_rate <= 24000 && dontUpSampleImplicitSBR == 0)
		forceUpSampling = 1;
	else */
    if ( sample_rate > 24000 && dontUpSampleImplicitSBR == 0)
		downSampledSBR = 1;
	
	//raw data parse
	if( *profile < ER_OBJECT_START )
	{
		PopBits(s, id_syn_ele, LEN_SE_ID); //check syntax

		//check one element once!!!
		//while (id_syn_ele != ID_END && *sbr_present_flag==0) 
		{
			
			STORE_S
			switch (id_syn_ele) {
				case ID_SCE:
				case ID_LFE:
				{
					err = decode_sce_lfe(ld, sbr_present_flag, sf_index);
					BAIL_IF_ERR(err);
					break;
				}
				case ID_CPE:
				{
					err = decode_cpe(ld, sbr_present_flag, sf_index);
					BAIL_IF_ERR(err);
					break;
				}
				case ID_FIL:
				{
					fill_element(ld, sbr_present_flag);
					break;
				}
				default:
					break;
			}
			RESET_S

			//PopBits(s, id_syn_ele, LEN_SE_ID);
		}
	}
	else 
	{
		err = -1;//doesn't support
		goto bail;
	}
	
	if ( *sbr_present_flag == 1/* || forceUpSampling == 1*/) 
	{
		if (!downSampledSBR) 
		{
			play_sample_rate *= 2;
			//sampling_frequency_index -= 3;
		}
		
		if (*sbr_present_flag == 1)
		{
			*profile = 5; //HE AAC
		}
	}
	
	if (*profile > 5)
		*profile = 5; //in case error happend! Fix Coverity 10795
	
	pcu = (int)(((float)play_sample_rate) / (float)(48000) * channel_total * PCU[*profile]);
	rcu = (int)RCU[*profile];
	
	*level = guess_aac_level(*profile, channel_total<5 ? channel_total:5, play_sample_rate, pcu, rcu);
    *sample_rate_ext = play_sample_rate;
bail:
	return err;
}
