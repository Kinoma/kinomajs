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

#if __FSK_LAYER__
	#include "Fsk.h"
#endif

#ifndef __QTREADER__
#define __QTREADER__

#if TARGET_OS_LINUX
	#define PACK_STRUCT __attribute__((packed))
#else
	#define PACK_STRUCT
#endif

#if defined(__FSK_LAYER__) || defined (__FSK_JUMPER_APP__)
	#include "Fsk.h"
	#define QTAPI FskAPI
#else
	#include "MacTypes.h"
#endif

#ifndef QTAPI
	#define QTAPI
#endif

#ifndef QT_READER_AUTHORING
	#define QT_READER_AUTHORING 0
#endif

#ifndef QT_READER_MATRIX
	#define QT_READER_MATRIX 1
#endif

#ifndef QT_READER_EDITS
	#define QT_READER_EDITS 1
#endif

#ifndef QT_READER_FILE64
	#define QT_READER_FILE64 1
#endif

#ifndef QT_READER_TRACKREF
	#define QT_READER_TRACKREF 1
#endif

#ifndef QT_READER_QTATOM
	#define QT_READER_QTATOM 1
#endif

#if QT_READER_AUTHORING
	#ifndef QT_READER_EXTRA_ATOMS
		#define QT_READER_EXTRA_ATOMS 0
	#endif
	#ifndef QT_READER_MTDT
		#define QT_READER_MTDT 1
	#endif
	#ifndef QT_READER_STPS
		#define QT_READER_STPS 1
	#endif
	#ifndef QT_READER_SDTP
		#define QT_READER_SDTP 1
	#endif
#endif

#if !defined(QT_READER_ZLIB) && __FSK_LAYER__
	#define QT_READER_ZLIB 1
#endif

#if __FSK_LAYER__
	#ifndef QT_READER_STZ2
		#define QT_READER_STZ2 1
	#endif
	#ifndef QT_READER_STPS
		#define QT_READER_STPS 1
	#endif
	#ifndef QT_READER_SDTP
		#define QT_READER_SDTP 1
	#endif
	#ifndef QT_READER_MTDT
		#define QT_READER_MTDT 1
	#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef SInt32 QTErr;

#if !QT_READER_FILE64
	typedef UInt32 QTFileOffset;
#else
# if defined(__FSK_LAYER__) || defined (__FSK_JUMPER_APP__)
	typedef FskFileOffset QTFileOffset;
# else
	typedef UInt64 QTFileOffset;
# endif
#endif

typedef QTErr (*QTMovieReadProc)(void *refCon, void *data, QTFileOffset offset, UInt32 dataSize);
typedef QTErr (*QTMovieAllocProc)(void *refCon, Boolean clear, UInt32 size, void **data);
typedef void (*QTMovieFreeProc)(void *refCon, void *data);
#if QT_READER_AUTHORING
	typedef QTErr (*QTMovieWriteProc)(void *refCon, const void *data, QTFileOffset offset, UInt32 dataSize);
#endif

#if QT_READER_EXTRA_ATOMS
	typedef struct QTExtraAtomRecord QTExtraAtomRecord; 
	typedef struct QTExtraAtomRecord *QTExtraAtom; 

	struct QTExtraAtomRecord {
		QTExtraAtom			next;
		UInt32				type;
		UInt32				size;
		char				data[1];
	};
#endif

#if QT_READER_MTDT
	typedef struct QTMTDTRecord QTMTDTRecord; 
	typedef struct QTMTDTRecord *QTMTDT; 
#endif

struct QTMovieRecord;
typedef struct QTMovieRecord QTMovieRecord;
typedef struct QTMovieRecord *QTMovie;

struct QTTrackRecord;
typedef struct QTTrackRecord QTTrackRecord;
typedef struct QTTrackRecord *QTTrack;

struct QTMediaRecord;
typedef struct QTMediaRecord QTMediaRecord;
typedef struct QTMediaRecord *QTMedia;

#if TARGET_OS_MAC
  #if TARGET_CPU_ARM64 || TARGET_CPU_X86_64
    #pragma pack(2)
  #else
    #pragma options align=mac68k
  #endif
#elif TARGET_OS_WIN32
    #pragma pack(push, 2)
#elif TARGET_OS_KPL
	#pragma pack(2)
#elif TARGET_OS_LINUX
    #pragma pack(2)
#else
	#error - QTReader.h needs pack macro definition
#endif

typedef struct {
    UInt32 idSize;
    UInt32 cType;
    UInt32 resvd1;
    UInt16 resvd2;
    UInt16 dataRefIndex;
    UInt16 version;
    UInt16 revisionLevel;
    UInt32 vendor;
    UInt32 temporalQuality;
    UInt32 spatialQuality;
    UInt16 width;
    UInt16 height;
    UInt32 hRes;
    UInt32 vRes;
    UInt32 dataSize;
    UInt16 frameCount;
    char name[32];
    UInt16 depth;
    UInt16 clutID;
} PACK_STRUCT QTImageDescriptionRecord, *QTImageDescription;

typedef struct {
    UInt32	descSize;
    UInt32	dataFormat;
    UInt32	resvd1;
    UInt16	resvd2;
    UInt16	dataRefIndex;
    UInt16	version;
    UInt16	revlevel;
    UInt32	vendor;
    UInt16	numChannels;
    UInt16	sampleSize;
    UInt16	compressionID;
    UInt16	packetSize;
    UInt32	sampleRate;

    UInt32	samplesPerPacket;
    UInt32	bytesPerPacket;
    UInt32	bytesPerFrame;
    UInt32	bytesPerSample;
} PACK_STRUCT QTSoundDescriptionRecord, *QTSoundDescription;

typedef struct {
	UInt32	descSize;
	UInt32	dataFormat;
	UInt32	resvd1;
	UInt16	resvd2;
	UInt16	dataRefIndex;
} PACK_STRUCT QTGenericDescriptionRecord, *QTGenericDescription;

#if QT_READER_MTDT
	struct QTMTDTRecord {
		QTMTDT				next;
		UInt16				dataSize;
		UInt32				dataType;
		UInt16				language;	// and read-only flag
		UInt16				encoding;	// 0=binary, 1=UTF-16BE
		char				data[1];
	} PACK_STRUCT;
#endif

#if TARGET_OS_MAC
    #pragma options align=reset
#elif TARGET_OS_WIN32
    #pragma pack(pop)
#elif TARGET_OS_KPL
    #pragma pack()
#elif TARGET_OS_LINUX
    #pragma pack()
#else
	#error - QTReader.h needs pack macro definition
#endif

typedef struct {
	void			*next;

	UInt32			flags;
	UInt32			kind;
	UInt32			drSize;

	char			dataRef[1];
} PACK_STRUCT QTDataReferenceRecord, *QTDataReference;

typedef struct {
	void			*next;

	char			desc[1];
} QTSampleDescriptionRecord, *QTSampleDescription;

typedef struct {
	QTMovie			movie;
	UInt32			size;
	UInt32			parentType;
	void			*parent;

	char			data[4];
} QTUserDataRecord, *QTUserData;

typedef struct {
    UInt32		count;
    UInt32		duration;
} PACK_STRUCT QTTimeToSampleRecord, *QTTimeToSample;

typedef struct {
    UInt32		firstChunk;
    UInt32		samplesPerChunk;
    UInt32		sampleDescriptionIndex;
} PACK_STRUCT QTSampleToChunkRecord, *QTSampleToChunk;


typedef struct {
    UInt32		count;
    UInt32		offset;
} PACK_STRUCT QTCompositionTimeOffsetRecord, *QTCompositionTimeOffset;

#define STCompositionTimeAID 'ctts'

struct QTMediaRecord {
	QTTrack					track;

	UInt32					duration;
	UInt32					scale;

	UInt32					mediaType;

	QTSampleDescription		sampleDescriptions;
	QTDataReference			dataReferences;

	UInt32					*chunkOffsets;
#if QT_READER_FILE64
	QTFileOffset			*chunkOffsets64;
#endif
	UInt32					chunkOffsetCount;

	UInt32					*sampleSizes;
	UInt32					sampleSizeCount;
#if QT_READER_STZ2
	UInt32					sampleSizeBits;
#endif
	UInt32					sampleCount;

	UInt32					*syncSamples;
	UInt32					syncSampleCount;
	UInt32					ssCacheSampleNumber;
	UInt32					ssCacheIndex;

#if QT_READER_STPS
	UInt32					*partialSyncSamples;
	UInt32					partialSyncSampleCount;
#endif

#if QT_READER_SDTP
	UInt8					*sampleDependencies;
	UInt32					sampleDependencyCount;
#endif

	QTSampleToChunk			sampleToChunks;
	UInt32					sampleToChunkCount;
	UInt32					stcCacheSampleNumber;
	UInt32					stcCacheIndex;

	QTTimeToSample			timeToSamples;
	UInt32					timeToSampleCount;
	UInt32					ttsCacheIndex;
	UInt32					ttsCacheMediaTime;
	UInt32					ttsCacheSampleNumber;

	QTCompositionTimeOffset	compositionTimeOffsets;
	UInt32					compositionTimeCount;

#if QT_READER_AUTHORING
	UInt32					chunkOffsetsSize;
	UInt32					sampleSizesSize;
	UInt32					syncSamplesSize;
	UInt32					sampleToChunksSize;
	UInt32					timeToSamplesSize;
	UInt32					compositionTimeOffsetsSize;
#if QT_READER_STPS
	UInt32					partialSyncSamplesSize;
#endif
#if QT_READER_SDTP
	UInt32					sampleDependenciesSize;
#endif

	UInt32					autoChunkCount;
#endif
};

struct QTEditRecord {
    UInt32			trackDuration;
    SInt32			mediaTime;
	SInt32			rate;
} PACK_STRUCT;
typedef struct QTEditRecord QTEditRecord;
typedef struct QTEditRecord *QTEdit;

struct QTTrackRecord {
	QTTrack		next;

	QTMovie		movie;

	UInt32		duration;
	UInt32		id;
	UInt16		width;
	UInt16		height;
	SInt16		volume;
	Boolean		enabled;

	QTMedia		media;

	QTUserData	userData;

#if QT_READER_MATRIX
	UInt32		matrix[9];
#endif
#if QT_READER_EDITS
	UInt32		editsCount;
	QTEdit		edits;
#endif
#if QT_READER_TRACKREF
	UInt32		trackRefSize;			// in bytes
	UInt32		*trackRefs;				// contents of tref atom
#endif
#if QT_READER_MTDT
	QTMTDT		mtdt;
#endif
#if QT_READER_EXTRA_ATOMS
	QTExtraAtom	extras;
#endif
};

typedef struct QTReferenceMovieDescriptorRecord QTReferenceMovieDescriptorRecord;
typedef struct QTReferenceMovieDescriptorRecord *QTReferenceMovieDescriptor;

struct QTReferenceMovieDescriptorRecord {
	struct QTReferenceMovieDescriptorRecord
					*next;

	char			*dataRef;
	UInt32			dataRefSize;

	UInt32			dataRate;

	Boolean			wantsMobile;
};

struct QTMovieRecord {
	UInt32		duration;
	UInt32		scale;

	QTTrack		tracks;

	QTUserData	userData;
	UInt32		ftypBrand;				// primary type listed in ftyp atom
	UInt32		compatibleBrandCount;
	UInt32		*compatibleBrands;

	void		*profile;				// contents of profile atom

	QTMovieAllocProc	allocProc;
	QTMovieFreeProc		freeProc;
	void				*allocRefCon;

	QTReferenceMovieDescriptor
						refMovieDescriptor;

	// load time only
	QTMovieReadProc		reader;
	void				*readerRefCon;
	QTTrack				curTrack;
	QTMedia				curMedia;

	Boolean				loaded;
	Boolean				minimal;

	QTReferenceMovieDescriptor
						curRefMovieDescriptor;

#if QT_READER_AUTHORING
	// save only
	QTMovieWriteProc	writer;
	void				*writerRefCon;

	QTFileOffset		writeOffset;

	UInt32				nextTrackID;

	Boolean				isQT;						// defaults to MP4
#endif
#if QT_READER_MTDT
	QTMTDT		mtdt;
#endif
#if QT_READER_EXTRA_ATOMS
	QTExtraAtom			extras;
#endif
};

QTAPI(QTErr) QTMovieNewFromReader(QTMovie *movie, Boolean minimal, QTMovieReadProc reader, void *readerRefCon, QTMovieAllocProc alloc, QTMovieFreeProc free, void *allocRefCon);
QTAPI(void) QTMovieDispose(QTMovie movie);

QTAPI(QTTrack) QTMovieGetIndTrack(QTMovie movie, UInt32 index);
QTAPI(QTTrack) QTMovieGetIndTrackType(QTMovie movie, UInt32 mediaType, UInt32 index);
QTTrack QTMovieGetTrackByID(QTMovie movie, UInt32 id);
QTAPI(void) *QTTrackGetIndSampleDescription(QTTrack track, UInt32 index);
QTErr QTTrackGetIndDataReference(QTTrack track, UInt32 index, UInt32 *kind, UInt32 *flags, void **dataRef, UInt32 *dataRefSize);

QTAPI(void *) QTVideoSampleDescriptionGetExtension(QTImageDescription id, UInt32 extension);
QTAPI(void *) QTAudioSampleDescriptionGetExtension(QTSoundDescription id, UInt32 extension);
QTAPI(void *) QTAudioSampleDescriptionGetESDS(QTSoundDescription sd, UInt32 *size);
QTAPI(void *) QTAudioSampleDescriptionGetALAC(QTSoundDescription sd, UInt32 *size);

QTAPI(void) QTSampleDescriptionFlip(void *desc, UInt32 mediaType, Boolean toNative);

QTAPI(UInt32) QTMovieToTrackDuration(UInt32 movieDur, QTTrack track);
QTAPI(UInt32) QTTrackToMovieDuration(UInt32 trackDur, QTTrack track);

#if QT_READER_EDITS
	QTAPI(QTErr) QTTrackMovieTimeToEditIndex(QTTrack track, UInt32 movieTime, UInt32 *index);
	QTAPI(QTErr) QTTrackGetEditInfo(QTTrack track, UInt32 index, UInt32 *movieStartTime, SInt32 *trackStartTime, SInt32 *rate, UInt32 *movieDuration);
#endif

#if QT_READER_TRACKREF
	QTAPI(QTTrack) QTTrackGetReference(QTTrack fromTrack, UInt32 referenceType, UInt32 referenceIndex);
#endif

QTAPI(QTErr) QTTrackTimeToSample(QTTrack track, UInt32 atMediaTime, UInt32 *sampleNumberOut, UInt32 *mediaTimeOut, UInt32 *sampleDurationOut);
QTAPI(QTErr) QTTrackSampleToTime(QTTrack track, UInt32 sampleNumber, UInt32 *mediaTimeOut);
QTAPI(QTErr) QTTrackSampleToSyncSamples(QTTrack track, UInt32 sampleNumber, UInt32 *prevSyncSampleNumberOut, UInt32 *nextSyncSampleNumberOut);
QTErr QTTrackTimeToSyncSamples(QTTrack track, UInt32 atMediaTime, UInt32 *prevSyncSampleNumberOut, UInt32 *nextSyncSampleNumberOut);
QTAPI(QTErr) QTTrackGetSampleTemporalInfo(QTTrack track, UInt32 sampleNumber, UInt32 *mediaTimeOut, UInt32 *compositionTimeOffsetOut, UInt32 *durationOut);
QTAPI(QTErr) QTTrackSampleToChunk(QTTrack track, UInt32 sampleNumber, UInt32 *chunkNumber, QTFileOffset *chunkOffset, UInt32 *chunkFirstSample, QTSampleToChunk stcOut);
QTErr QTTrackGetChunkInfo(QTTrack track, UInt32 chunkNumber, QTFileOffset *chunkOffset, UInt32 *chunkSize);
QTAPI(QTErr) QTTrackGetSampleSizes(QTTrack track, UInt32 sampleNumber, UInt32 sampleCount, QTFileOffset *size);
QTAPI(QTErr) QTTrackGetChunkSamplesOfSameSize(QTTrack track, UInt32 firstSampleNumber, UInt32 *count);

#define QTLoadData(movie, offset, size, data) QTLoadData_(movie, offset, size, (void **)(void *)(data))
QTErr QTLoadData_(QTMovie movie, QTFileOffset offset, QTFileOffset size, void **data);
QTAPI(QTErr) QTTrackLoadSample(QTTrack track, UInt32 sampleNumber, void **data, UInt32 *dataSize);

QTAPI(QTErr) QTUserDataGet(QTUserData userData, UInt32 udType, UInt32 udIndex, void **ud, UInt32 *udSize);
QTAPI(QTErr) QTUserDataGetText(QTUserData userData, UInt32 udType, void **ud);
QTAPI(QTErr) QTUserDataGetTextMP4(QTUserData userData, UInt32 udType, void **text);
QTAPI(QTErr) QTUserDataGetiTunes(QTUserData userData, UInt32 udType, void **userPtr, UInt32 *userPtrSize);
QTAPI(QTErr) QTUserDataGetTextiTunes(QTUserData userData, UInt32 udType, void **text);

QTAPI(QTErr) QTUserDataSetiTunesData(QTUserData userData, UInt32 udType, UInt32 iTunesType, const void *data, UInt32 dataSize);

enum {
	kQTVideoType = 'vide',
	kQTSoundType = 'soun',
	kQTTextType = 'text',
	kQTPanoramaType = 'pano',
	kQTStreamType = 'strm'
};
	
enum {
    kQTUserDataArtist = 0xA9415254,//'�ART',
    kQTUserDataCopyright = 0xA9637079,//'�cpy',
    kQTUserDataFullName = 0xA96E616D,//'�nam',
    kQTUserDataAuthor = 0xA9617574,//'�aut',
    kQTUserDataDate = 0xA9646179,//'�day',
    kQTUserDataSoftware = 0xA9737772,//'�swr',
    kQTUserDataTrack = 0xA974726B,//'�trk',

	kMP4UserDataFullName = 'titl',
    kMP4UserDataCopyright = 'cprt',
    kMP4UserDataAuthor = 'auth',

    kiTunesUserDataAlbum = 0xA9616C62,//'�alb',
    kiTunesUserDataTool = 0xA9746F6F,//'�too',
    kiTunesUserDataComposer = 0xA9777274,//'�wrt',
    kiTunesUserDataGrouping = 0xA9677270,//'�grp',
    kiTunesUserDataGenre = 0xA967656E,//'�gen',
    kiTunesUserDataComment = 0xA9636D74,//'�cmt',
    kiTunesUserDataDate = 0xA9646179,//'�day',

	// not stored as null-terminated strings
    kiTunesUserDataBPM = 'tmpo',
    kiTunesUserDataCompilation = 'cpil',
    kiTunesUserDataCoverArt = 'covr',
    kiTunesUserDataTrackNum = 'trkn',
    kiTunesUserDataDiscNum = 'disk',
	kiTunesUserDataGenreAsInteger = 'gnre'
};

enum {
	kQTUserDataMovieLevel,
	kQTUserDataTrackLevel
};

Boolean QTMovieIsSelfContained(QTMovie movie);

// authoring support that is shared by reader
QTAPI(QTErr) QTMovieNew(QTMovie *movie, UInt32 scale, QTMovieAllocProc alloc, QTMovieFreeProc free, void *allocRefCon);
QTAPI(QTErr) QTTrackNew(QTTrack *track, QTMovie movie, UInt16 width, UInt16 height, UInt16 volume);
QTAPI(void) QTTrackDispose(QTTrack track);
QTAPI(QTErr) QTMediaNew(QTMedia *media, QTTrack track, UInt32 scale, UInt32 mediaType);
QTAPI(QTErr) QTMediaAddDataReference(QTMedia media, UInt32 kind, UInt32 flags, void *dataRef, UInt32 drSize, UInt32 *index);
#if QT_READER_EDITS
	QTAPI(QTErr) QTTrackSetEditList(QTTrack track, UInt32 count, QTEdit editList);
#endif
QTAPI(QTErr) QTTrackSetEnabled(QTTrack track, Boolean enabled);

// low level atom walking

typedef QTErr (*QTMovieAtomWalker)(QTMovie movie, QTFileOffset offset, QTFileOffset size);

typedef struct {
	UInt32				atomType;
	QTMovieAtomWalker	walker;
} QTMovieAtomWalkersRecord, *QTMovieAtomWalkers;

#define kQTMovieWalkAnyAtom 0xffffffff
QTAPI(QTErr) QTMovieWalkAtoms(QTMovie movie, QTFileOffset offset, QTFileOffset size, QTMovieAtomWalkers walkers);

#if QT_READER_AUTHORING

	typedef struct {
		UInt32		sampleCount;
		UInt32		sampleSize;
		UInt32		sampleDuration;
		UInt32		compositionOffset;
		Boolean		keyFrame;
#if QT_READER_STPS
		Boolean		partialSync;
#endif
#if QT_READER_SDTP
		Boolean		droppable;
#endif
	} QTSampleInfoRecord, *QTSampleInfo;

	QTAPI(QTErr) QTMediaAddSampleDescription(QTMedia media, void *sampleDescription, UInt32 *index);
	QTAPI(QTErr) QTUserDataNew(QTMovie movie, QTUserData *userData, UInt32 parentType, void *parent);
	QTAPI(QTErr) QTUserDataAdd(QTUserData userData, UInt32 udType, const void *ud, UInt32 udSize);
	QTErr QTUserDataRemove(QTUserData userData, UInt32 udType, UInt32 udIndex);
	QTAPI(QTErr) QTMediaAddSamples(QTMedia media, UInt16 sampleDescriptionIndex, QTFileOffset fileOffset, UInt32 infoCount, QTSampleInfo samples);
	QTAPI(QTErr) QTMovieSave(QTMovie movie, QTFileOffset offset, QTMovieWriteProc write, void *writeRefCon);

	QTAPI(QTErr) QTSampleDescriptionAddExtension(QTMovie movie, void **sampleDescription, UInt32 extension, const void *data, UInt32 dataSize);

	typedef struct {
		QTFileOffset		offset;
		QTFileOffset		size;

		UInt32				atomType;

#if QT_READER_FILE64
		Boolean				couldBeBig;
#endif

		QTMovieWriteProc	writer;
		void				*writerRefCon;

		QTMovieAllocProc	alloc;
		QTMovieFreeProc		free;
		void				*allocRefCon;
	} QTAtomWriterRecord, *QTAtomWriter;

	QTAPI(QTErr) QTAtomWriterOpen(QTAtomWriter *atom, UInt32 atomType, QTFileOffset fileOffset, Boolean couldBeBig, QTMovieWriteProc write, void *writeRefCon, QTMovieAllocProc alloc, QTMovieFreeProc free, void *allocRefCon);
	QTAPI(QTErr) QTAtomWriterAppend(QTAtomWriter atom, const void *data, UInt32 dataSize, QTFileOffset *dataOffset);
	QTAPI(QTErr) QTAtomWriterClose(QTAtomWriter atom, QTFileOffset *nextOffset);

	#if QT_READER_MTDT
		QTAPI(QTErr) QTAddMTDT(QTMovie movie, QTTrack track, UInt16 dataSize, UInt32 dataType, UInt16 language, UInt16 encoding, void *data);
	#endif
#endif

// QTAtom reader

#if QT_READER_QTATOM
	QTAPI(void *) QTAtomGetRootAtom(void *qtAtom);
	QTAPI(void *) QTAtomGetAtomByID(void *qtAtom, UInt32 atomType, UInt32 atomID);
	QTAPI(void *) QTAtomGetAtomDataPtr(void *qtAtom, UInt32 *dataSize);
#endif

QTAPI(Boolean) QTESDSScanVideo(unsigned char *esds, UInt32 esdsSize, UInt32 *width, UInt32 *height, UInt8 *profile_level);
QTAPI(Boolean) QTESDSScanAudio(unsigned char *esds, UInt32 count, UInt8 *codec, UInt32 *audioType, UInt32 *sampleRate, UInt32 *channelCount);
QTAPI(Boolean) QTALACScanAudio(unsigned char *alac, UInt32 count, UInt32 *frameLength, UInt8 *bitDepth, UInt8 *numChannels, UInt32 *maxFrameBytes, UInt32 *avgBitRate, UInt32 *sampleRate);
QTAPI(QTErr) get_extended_aac_profile_level( const unsigned char *s, UInt32 sample_rate, UInt32 channel_total, UInt32 *sample_rate_ext, SInt32 *sbr_present_flag, UInt32 *profile, UInt32 *level );

#ifdef __cplusplus
}
#endif

#endif
