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
#ifndef __QTMOVIESFORMAT__
#define __QTMOVIESFORMAT__

#if PRAGMA_STRUCT_ALIGN
    #pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
    #pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
    #pragma pack(2)
#endif

#ifndef FOUR_CHAR_CODE
 #define FOUR_CHAR_CODE(a) (a)
#endif

#if !TARGET_OS_MAC || TARGET_OS_IPHONE
	typedef SInt32 TimeValue;
	typedef SInt32 Fixed;
	typedef UInt32 OSType;
#endif

typedef struct {
	SInt16	top;
	SInt16	left;
	SInt16	bottom;
	SInt16	right;
} PACK_STRUCT QTRect;

typedef struct {
	UInt32 matrix[9];
} PACK_STRUCT QTMatrixRecord;

#if !((TARGET_OS_MAC && !TARGET_OS_IPHONE) && defined(__MOVIESFORMAT__))
/****************************************
*
*   General Types -
*       These types are used in more than one of the
*       directory types.
*
****************************************/
/* MoviesUserData is the type used for user data in movie and track directories */

struct MoviesUserData {
    SInt32                          size;                       /* size of this user data */
    SInt32                          udType;                     /* type of user data */
    char                            data[1];                    /* the user data */
} PACK_STRUCT;
typedef struct MoviesUserData           MoviesUserData;

struct UserDataAtom {
    SInt32                          size;
    SInt32                          atomType;
    MoviesUserData                  userData[1];
} PACK_STRUCT;
typedef struct UserDataAtom             UserDataAtom;
/* MoviesDataDescription tells us where the data for the movie or track lives.
   The data can follow the directory, be in the datafork of the same file as the directory resource,
   be in the resource fork of the same file as the directory resource, be in another file in the
   data fork or resource fork, or require a specific bottleneck to fetch the data. */
/****************************************
*
*   MediaDirectory information -
*       The MediaDirectory is tightly coupled to the data.
*
****************************************/


struct SampleDescriptionAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'stsd' */
    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */
    SInt32                          numEntries;
//    SampleDescription               sampleDescTable[1];
	void							*sampleDescTable;
} PACK_STRUCT;
typedef struct SampleDescriptionAtom    SampleDescriptionAtom;
/* TimeToSampleNum maps physical sample time to physical sample number. */

struct TimeToSampleNum {
    SInt32                            sampleCount;
    TimeValue                       sampleDuration;
} PACK_STRUCT;
typedef struct TimeToSampleNum          TimeToSampleNum;

struct TimeToSampleNumAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'stts' */
    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */
    SInt32                          numEntries;
    TimeToSampleNum                 timeToSampleNumTable[1];
} PACK_STRUCT;
typedef struct TimeToSampleNumAtom      TimeToSampleNumAtom;
/* SyncSamples is a list of the physical samples which are self contained. */

struct SyncSampleAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'stss' */
    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */
    SInt32                          numEntries;
    SInt32                          syncSampleTable[1];
} PACK_STRUCT;
typedef struct SyncSampleAtom           SyncSampleAtom;
/* SampleToChunk maps physical sample number to chunk number. */
/* same as SampleToChunk, but redundant first sample is removed */

struct SampleToChunk {
    SInt32                          firstChunk;
    SInt32                          samplesPerChunk;
    SInt32                          sampleDescriptionID;
} PACK_STRUCT;
typedef struct SampleToChunk            SampleToChunk;

struct SampleToChunkAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'stsc' */
    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */
    SInt32                          numEntries;
    SampleToChunk                   sampleToChunkTable[1];
} PACK_STRUCT;
typedef struct SampleToChunkAtom        SampleToChunkAtom;

struct ChunkOffsetAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'stco' */
    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */
    SInt32                          numEntries;
    SInt32                          chunkOffsetTable[1];
} PACK_STRUCT;
typedef struct ChunkOffsetAtom          ChunkOffsetAtom;

struct SampleSizeAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'stsz' */
    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */
    SInt32                          sampleSize;
    SInt32                          numEntries;
    SInt32                          sampleSizeTable[1];
} PACK_STRUCT;
typedef struct SampleSizeAtom           SampleSizeAtom;

struct ShadowSync {
    SInt32                          fdSampleNum;
    SInt32                          syncSampleNum;
} PACK_STRUCT;
typedef struct ShadowSync               ShadowSync;

struct ShadowSyncAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'stsz' */
    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */
    SInt32                          numEntries;
    ShadowSync                      shadowSyncTable[1];
} PACK_STRUCT;
typedef struct ShadowSyncAtom           ShadowSyncAtom;

struct SampleTableAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'stbl' */

    SampleDescriptionAtom           sampleDescription;
    TimeToSampleNumAtom             timeToSampleNum;
    SampleToChunkAtom               sampleToChunk;
    SyncSampleAtom                  syncSample;
    SampleSizeAtom                  sampleSize;
    ChunkOffsetAtom                 chunkOffset;
    ShadowSyncAtom                  shadowSync;
} PACK_STRUCT;
typedef struct SampleTableAtom          SampleTableAtom;

struct PublicHandlerInfo {
    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */

    SInt32                          componentType;
    SInt32                          componentSubType;
    SInt32                          componentManufacturer;
    SInt32                          componentFlags;
    SInt32                          componentFlagsMask;
    char                            componentName[1];
} PACK_STRUCT;
typedef struct PublicHandlerInfo        PublicHandlerInfo;

struct HandlerAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'hdlr' */

    PublicHandlerInfo               hInfo;
} PACK_STRUCT;
typedef struct HandlerAtom              HandlerAtom;
/* a data reference is a private structure */

typedef SInt32                          DataRefAtom;

struct DataInfoAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'dinf' */

    DataRefAtom                     dataRef;
} PACK_STRUCT;
typedef struct DataInfoAtom             DataInfoAtom;


/***********************
* Media Info Example Structures
***********************/


struct VideoMediaInfoHeader {
    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */

    short                           graphicsMode;               /* for QD - transfer mode */
    short                           opColorRed;                 /* opcolor for transfer mode */
    short                           opColorGreen;
    short                           opColorBlue;
} PACK_STRUCT;
typedef struct VideoMediaInfoHeader     VideoMediaInfoHeader;

struct VideoMediaInfoHeaderAtom {
    SInt32                          size;                       /* size of Media info */
    SInt32                          atomType;                   /* = 'vmhd' */
    VideoMediaInfoHeader            vmiHeader;
} PACK_STRUCT;
typedef struct VideoMediaInfoHeaderAtom VideoMediaInfoHeaderAtom;

struct VideoMediaInfo {
    SInt32                          size;                       /* size of Media info */
    SInt32                          atomType;                   /* = 'minf' */

    VideoMediaInfoHeaderAtom        header;

    HandlerAtom                     dataHandler;

    DataInfoAtom                    dataInfo;

    SampleTableAtom                 sampleTable;
} PACK_STRUCT;
typedef struct VideoMediaInfo           VideoMediaInfo;

struct SoundMediaInfoHeader {
    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */

    short                           balance;
    short                           rsrvd;
} PACK_STRUCT;
typedef struct SoundMediaInfoHeader     SoundMediaInfoHeader;

struct SoundMediaInfoHeaderAtom {
    SInt32                          size;                       /* size of Media info */
    SInt32                          atomType;                   /* = 'vmhd' */

    SoundMediaInfoHeader            smiHeader;
} PACK_STRUCT;
typedef struct SoundMediaInfoHeaderAtom SoundMediaInfoHeaderAtom;

struct SoundMediaInfo {
    SInt32                          size;                       /* size of Media info */
    SInt32                          atomType;                   /* = 'minf' */

    SoundMediaInfoHeaderAtom        header;

    HandlerAtom                     dataHandler;

    DataRefAtom                     dataReference;

    SampleTableAtom                 sampleTable;
} PACK_STRUCT;
typedef struct SoundMediaInfo           SoundMediaInfo;
/* whatever data the media handler needs goes after the atomType */

struct MediaInfo {
    SInt32                          size;
    SInt32                          atomType;
} PACK_STRUCT;
typedef struct MediaInfo                MediaInfo;
/***********************
* Media Directory Structures
***********************/

struct MediaHeader {
    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */

    SInt32                          creationTime;               /* seconds since Jan 1904 when directory was created */
    SInt32                          modificationTime;           /* seconds since Jan 1904 when directory was appended */

    TimeValue                       timeScale;                  /* start time for Media (Media time) */
    TimeValue                       duration;                   /* length of Media (Media time) */

    short                           language;
    short                           quality;
} PACK_STRUCT;
typedef struct MediaHeader              MediaHeader;

struct MediaHeaderAtom {
    SInt32                          size;
    SInt32                          atomType;

    MediaHeader                     header;
} PACK_STRUCT;
typedef struct MediaHeaderAtom          MediaHeaderAtom;

struct MediaDirectory {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'mdia' */

    MediaHeaderAtom                 mediaHeader;                /* standard Media information */

    HandlerAtom                     mediaHandler;

    MediaInfo                       mediaInfo;
} PACK_STRUCT;
typedef struct MediaDirectory           MediaDirectory;
/***********************
* Track Structures
***********************/
enum {
    TrackEnable                 = 1 << 0,
    TrackInMovie                = 1 << 1,
    TrackInPreview              = 1 << 2,
    TrackInPoster               = 1 << 3
};


struct TrackHeader {
    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */

    SInt32                          creationTime;               /* seconds since Jan 1904 when directory was created */
    SInt32                          modificationTime;           /* seconds since Jan 1904 when directory was appended */

    SInt32                          trackID;

    SInt32                          reserved1;

    TimeValue                       duration;                   /* length of track (track time) */

    SInt32                          reserved2;
    SInt32                          reserved3;

    short                           layer;
    short                           alternateGroup;

    short                           volume;
    short                           reserved4;

    QTMatrixRecord                    matrix;
    Fixed                           trackWidth;
    Fixed                           trackHeight;
} PACK_STRUCT;
typedef struct TrackHeader              TrackHeader;

struct TrackHeaderAtom {
    SInt32                          size;                       /* size of track header */
    SInt32                          atomType;                   /* = 'tkhd' */

    TrackHeader                     header;
} PACK_STRUCT;
typedef struct TrackHeaderAtom          TrackHeaderAtom;

struct EditListType {
    TimeValue                       trackDuration;
    TimeValue                       mediaTime;
    Fixed                           mediaRate;
} PACK_STRUCT;
typedef struct EditListType             EditListType;

struct EditListAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = elst */

    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */

    SInt32                          numEntries;
    EditListType                    editListTable[1];
} PACK_STRUCT;
typedef struct EditListAtom             EditListAtom;

struct EditsAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = edts */

    EditListAtom                    editList;
} PACK_STRUCT;
typedef struct EditsAtom                EditsAtom;

struct TrackLoadSettings {
    TimeValue                       preloadStartTime;
    TimeValue                       preloadDuration;
    SInt32                          preloadFlags;
    SInt32                          defaultHints;
} PACK_STRUCT;
typedef struct TrackLoadSettings        TrackLoadSettings;

struct TrackLoadSettingsAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = load */

    TrackLoadSettings               settings;
} PACK_STRUCT;
typedef struct TrackLoadSettingsAtom    TrackLoadSettingsAtom;

/****************************************
*
*   MovieDirectory -
*       The MovieDirectory is the top level structure which
*       holds the TrackInstance describing where the
*       TrackDirectories are.
*
****************************************/

struct MovieHeader {
    SInt32                          flags;                      /* 1 byte of version / 3 bytes of flags */

    SInt32                          creationTime;               /* seconds since Jan 1904 when directory was created */
    SInt32                          modificationTime;           /* seconds since Jan 1904 when directory was appended */

    TimeValue                       timeScale;                  /* Time specifications */
    TimeValue                       duration;
    Fixed                           preferredRate;              /* rate at which to play this movie */

    short                           preferredVolume;            /* volume to play movie at */
    short                           reserved1;

    SInt32                          preferredLong1;
    SInt32                          preferredLong2;

    QTMatrixRecord                    matrix;

    TimeValue                       previewTime;                /* time in track the proxy begins (track time) */
    TimeValue                       previewDuration;            /* how long the proxy lasts (track time) */

    TimeValue                       posterTime;                 /* time in track the proxy begins (track time) */

    TimeValue                       selectionTime;              /* time in track the proxy begins (track time) */
    TimeValue                       selectionDuration;          /* time in track the proxy begins (track time) */
    TimeValue                       currentTime;                /* time in track the proxy begins (track time) */

    SInt32                          nextTrackID;                /* next value to use for a TrackID */
} PACK_STRUCT;
typedef struct MovieHeader              MovieHeader;

struct MovieHeaderAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'mvhd' */

    MovieHeader                     header;
} PACK_STRUCT;
typedef struct MovieHeaderAtom          MovieHeaderAtom;

/****************************************/

/* atom id's */
enum {
    MovieAID                    = FOUR_CHAR_CODE('moov'),
    MovieHeaderAID              = FOUR_CHAR_CODE('mvhd'),
    ClipAID                     = FOUR_CHAR_CODE('clip'),
    RgnClipAID                  = FOUR_CHAR_CODE('crgn'),
    MatteAID                    = FOUR_CHAR_CODE('matt'),
    MatteCompAID                = FOUR_CHAR_CODE('kmat'),
    TrackAID                    = FOUR_CHAR_CODE('trak'),
    UserDataAID                 = FOUR_CHAR_CODE('udta'),
    TrackHeaderAID              = FOUR_CHAR_CODE('tkhd'),
    EditsAID                    = FOUR_CHAR_CODE('edts'),
    EditListAID                 = FOUR_CHAR_CODE('elst'),
    MediaAID                    = FOUR_CHAR_CODE('mdia'),
    MediaHeaderAID              = FOUR_CHAR_CODE('mdhd'),
    MediaInfoAID                = FOUR_CHAR_CODE('minf'),
    VideoMediaInfoHeaderAID     = FOUR_CHAR_CODE('vmhd'),
    SoundMediaInfoHeaderAID     = FOUR_CHAR_CODE('smhd'),
    GenericMediaInfoHeaderAID   = FOUR_CHAR_CODE('gmhd'),
    GenericMediaInfoAID         = FOUR_CHAR_CODE('gmin'),
    DataInfoAID                 = FOUR_CHAR_CODE('dinf'),
    DataRefAID                  = FOUR_CHAR_CODE('dref'),
    SampleTableAID              = FOUR_CHAR_CODE('stbl'),
    STSampleDescAID             = FOUR_CHAR_CODE('stsd'),
    STTimeToSampAID             = FOUR_CHAR_CODE('stts'),
    STSyncSampleAID             = FOUR_CHAR_CODE('stss'),
    STSampleToChunkAID          = FOUR_CHAR_CODE('stsc'),
    STShadowSyncAID             = FOUR_CHAR_CODE('stsh'),
    HandlerAID                  = FOUR_CHAR_CODE('hdlr'),
    STSampleSizeAID             = FOUR_CHAR_CODE('stsz'),
    STChunkOffsetAID            = FOUR_CHAR_CODE('stco'),
	STPartialSyncSampleAID		= FOUR_CHAR_CODE('stps'),
	STSampleDependencyAID       = FOUR_CHAR_CODE('sdtp'),
    STChunkOffset64AID          = FOUR_CHAR_CODE('co64'),
    STSampleIDAID               = FOUR_CHAR_CODE('stid'),
    DataRefContainerAID         = FOUR_CHAR_CODE('drfc'),
    TrackReferenceAID           = FOUR_CHAR_CODE('tref'),
    ColorTableAID               = FOUR_CHAR_CODE('ctab'),
    LoadSettingsAID             = FOUR_CHAR_CODE('load'),
    PropertyAtomAID             = FOUR_CHAR_CODE('code'),
    InputMapAID                 = FOUR_CHAR_CODE('imap'),
    MovieBufferHintsAID         = FOUR_CHAR_CODE('mbfh'),
    MovieDataRefAliasAID        = FOUR_CHAR_CODE('mdra'),
    SoundLocalizationAID        = FOUR_CHAR_CODE('sloc'),
    CompressedMovieAID          = FOUR_CHAR_CODE('cmov'),
    CompressedMovieDataAID      = FOUR_CHAR_CODE('cmvd'),
    DataCompressionAtomAID      = FOUR_CHAR_CODE('dcom'),
    ReferenceMovieRecordAID     = FOUR_CHAR_CODE('rmra'),
    ReferenceMovieDescriptorAID = FOUR_CHAR_CODE('rmda'),
    ReferenceMovieDataRefAID    = FOUR_CHAR_CODE('rdrf'),
    ReferenceMovieVersionCheckAID = FOUR_CHAR_CODE('rmvc'),
    ReferenceMovieDataRateAID   = FOUR_CHAR_CODE('rmdr'),
    ReferenceMovieComponentCheckAID = FOUR_CHAR_CODE('rmcd'),
    ReferenceMovieQualityAID    = FOUR_CHAR_CODE('rmqu'),
    ReferenceMovieLanguageAID   = FOUR_CHAR_CODE('rmla'),
    ReferenceMovieCPURatingAID  = FOUR_CHAR_CODE('rmcs'),
    ReferenceMovieAlternateGroupAID = FOUR_CHAR_CODE('rmag'),
    ReferenceMovieNetworkStatusAID = FOUR_CHAR_CODE('rnet'),
    CloneMediaAID               = FOUR_CHAR_CODE('clon')
};

/* Text ATOM definitions*/


struct TextBoxAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'tbox' */
    QTRect                            textBox;                    /* New text box (overrides defaultTextBox)*/
} PACK_STRUCT;
typedef struct TextBoxAtom              TextBoxAtom;

struct HiliteAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = 'hlit' */
    SInt32                          selStart;                   /* hilite selection start character*/
    SInt32                          selEnd;                     /* hilite selection end character*/
} PACK_STRUCT;
typedef struct HiliteAtom               HiliteAtom;

struct KaraokeRec {
    TimeValue                       timeVal;
    short                           beginHilite;
    short                           endHilite;
} PACK_STRUCT;
typedef struct KaraokeRec               KaraokeRec;

struct KaraokeAtom {
    SInt32                          numEntries;
    KaraokeRec                      karaokeEntries[1];
} PACK_STRUCT;
typedef struct KaraokeAtom              KaraokeAtom;
/* for ReferenceMovieDataRefRecord.flags*/
enum {
    kDataRefIsSelfContained     = (1 << 0)
};


struct ReferenceMovieDataRefRecord {
    SInt32                          flags;
    OSType                          dataRefType;
    SInt32                          dataRefSize;
    char                            dataRef[1];
} PACK_STRUCT;
typedef struct ReferenceMovieDataRefRecord ReferenceMovieDataRefRecord;
/* for VersionCheckRecord.checkType*/
enum {
    kVersionCheckMin            = 0,                            /* val1 is the min. version required*/
    kVersionCheckMask           = 1                             /* (gestalt return value & val2) must == val1*/
};


struct QTAltVersionCheckRecord {
    SInt32                          flags;                      /* currently always 0*/
    OSType                          gestaltTag;
    UInt32                          val1;
    UInt32                          val2;
    short                           checkType;
} PACK_STRUCT;
typedef struct QTAltVersionCheckRecord  QTAltVersionCheckRecord;
/* some helpful constants for DataRateRecord.dataRate */
enum {
    kDataRate144ModemRate       = 1400L,
    kDataRate288ModemRate       = 2800L,
    kDataRateISDNRate           = 5600L,
    kDataRateDualISDNRate       = 11200L,
    kDataRate256kbpsRate        = 25600L,
    kDataRate384kbpsRate        = 38400L,
    kDataRate512kbpsRate        = 51200L,
    kDataRate768kbpsRate        = 76800L,
    kDataRate1MbpsRate          = 100000L,
    kDataRateT1Rate             = 150000L,
    kDataRateInfiniteRate       = 0x7FFFFFFF,
    kDataRateDefaultIfNotSet    = kDataRateISDNRate
};


struct QTAltDataRateRecord {
    SInt32                          flags;                      /* currently always 0*/
    SInt32                          dataRate;
} PACK_STRUCT;
typedef struct QTAltDataRateRecord      QTAltDataRateRecord;

struct QTAltLanguageRecord {
    SInt32                          flags;                      /* currently always 0*/
    short                           language;
} PACK_STRUCT;
typedef struct QTAltLanguageRecord      QTAltLanguageRecord;

enum {
    kQTCPUSpeed1Rating          = 100,                          /* slowest*/
    kQTCPUSpeed2Rating          = 200,
    kQTCPUSpeed3Rating          = 300,
    kQTCPUSpeed4Rating          = 400,
    kQTCPUSpeed5Rating          = 500                           /* fastest*/
};


struct QTAltCPURatingRecord {
    UInt32                          flags;                      /* currently always 0*/
    UInt16                          speed;
} PACK_STRUCT;
typedef struct QTAltCPURatingRecord     QTAltCPURatingRecord;

struct ReferenceMovieNetworkStatusRecord {
    UInt32                          flags;                      /* currently always 0*/
    UInt32                          valueCount;                 /* how many status values are in array*/
    SInt32                          netStatusValues[1];         /* a value from kQTNetworkStatus... constants*/
} PACK_STRUCT;
typedef struct ReferenceMovieNetworkStatusRecord ReferenceMovieNetworkStatusRecord;

struct CloneRecord {
    SInt32                          flags;
    SInt32                          masterTrackID;              /* track ID of the track we're cloning */
} PACK_STRUCT;
typedef struct CloneRecord              CloneRecord;

struct CloneAtom {
    SInt32                          size;
    SInt32                          atomType;                   /* = clon */

    CloneRecord                     cloneInfo;
} PACK_STRUCT;
typedef struct CloneAtom                CloneAtom;

#endif
//
// security info atoms based on ISMA Crypt
//
typedef struct  {
	UInt32 atomSize;
	UInt32 atomType;    // should be 'sinf'
	struct frmaAtomType {
		UInt32 atomSize;
		UInt32 atomType;    // should be 'frma'
		UInt32 format;	    // original codec type
	} frma;
	struct schmAtomType {
		UInt32 atomSize;
		UInt32 atomType;    // should be 'schm'
		UInt8 version;
		UInt8 flags[3];
		UInt32 schmType;
		UInt32 schmVersion;
	} schm;
	struct schiAtomType {
		UInt32 atomSize;
		UInt32 atomType;    // should be 'schi'
		UInt8 *data;        // actual data from here when writing
	} schi;
#define SCHI_ATOM_HEADER_SIZE	(sizeof(struct schiAtomType) - sizeof(UInt8 *))
} sinfAtomType;
#if PRAGMA_STRUCT_ALIGN
    #pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
    #pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
    #pragma pack()
#endif

#ifdef PRAGMA_IMPORT_OFF
#pragma import off
#elif PRAGMA_IMPORT
#pragma import reset
#endif

#endif

