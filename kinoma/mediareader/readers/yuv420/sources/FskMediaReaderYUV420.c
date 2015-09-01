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
#define __FSKMEDIAREADER_PRIV__
#include "FskMediaReader.h"
#include "FskDIDLGenMedia.h"
#include "FskEndian.h"
#include "FskFiles.h"
#include "FskImage.h"
#include "QTReader.h"
#include "kinoma_utilities_buf_print.h"
//#include "jsmn.h"

#include "FskPlatformImplementation.h"
FskInstrumentedSimpleType(FskMediaReaderYUV420, FskMediaReaderYUV420);
#define mlog  FskFskMediaReaderYUV420PrintfMinimal
#define nlog  FskFskMediaReaderYUV420PrintfNormal
#define vlog  FskFskMediaReaderYUV420PrintfVerbose
#define dlog  FskFskMediaReaderYUV420PrintfDebug

#define SIMULATE_CAMERA 1

static Boolean yuv420ReaderCanHandle(const char *mimeType);
static FskErr yuv420ReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr yuv420ReaderDispose(FskMediaReader reader, void *readerState);
static FskErr yuv420ReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr yuv420ReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr yuv420ReaderStop(FskMediaReader reader, void *readerState);
static FskErr yuv420ReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr yuv420ReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
static FskErr yuv420ReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

static FskErr yuv420ReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
//static FskErr yuv420ReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420ReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
//static FskErr yuv420ReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420ReaderSetScrub(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420ReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

int validate_yuv420( unsigned char *d, int size, int *is_ts, int *offset_out );
#ifdef SIMULATE_CAMERA
static FskErr yuv420ReaderSetLens(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420ReaderSetAutoFocusState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420ReaderGetJSON(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420ReaderGetCameraCount(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static void askExtractMoreCallback(FskTimeCallBack callback, const FskTime time, void *param);
#endif

static FskMediaPropertyEntryRecord yuv420ReaderProperties[] = 
{
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			yuv420ReaderGetDuration,		NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		yuv420ReaderGetTimeScale,		NULL},
	{kFskMediaPropertyScrub,				kFskMediaPropertyTypeBoolean,		NULL,						yuv420ReaderSetScrub},
	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	yuv420ReaderGetDLNASinks,		NULL},
#ifdef SIMULATE_CAMERA
	{kFskMediaPropertyLens,                 kFskMediaPropertyTypeString,   		NULL,                       yuv420ReaderSetLens},
	{kFskMediaPropertyAutoFocusState,       kFskMediaPropertyTypeInteger,   	NULL,                       yuv420ReaderSetAutoFocusState},
//	{kFskMediaPropertyJSON,                 kFskMediaPropertyTypeString,   		yuv420ReaderGetJSON,        NULL},
//	{kFskMediaPropertyCameraCount,          kFskMediaPropertyTypeString,   		yuv420ReaderGetCameraCount,     NULL},
#endif
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,                           NULL}
};

FskMediaReaderDispatchRecord gMediaReaderYUV420 = {yuv420ReaderCanHandle, yuv420ReaderNew, yuv420ReaderDispose, yuv420ReaderGetTrack, yuv420ReaderStart, yuv420ReaderStop, yuv420ReaderExtract, yuv420ReaderGetMetadata, yuv420ReaderProperties, yuv420ReaderSniff};

static FskErr yuv420ReaderTrackGetMediaType(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420ReaderTrackGetFormat(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420ReaderTrackGetFormatInfo(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420ReaderTrackGetFrameRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420ReaderTrackSetDimensions(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420ReaderTrackGetDimensions(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420ReaderTrackGetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
#ifdef SIMULATE_CAMERA
static FskErr yuv420ReaderTrackGetDimensionList(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr yuv420ReaderTrackSetEnabled(void *state_in, void *track, UInt32 propertyID, FskMediaPropertyValue property);
#endif

static FskMediaPropertyEntryRecord yuv420ReaderVideoTrackProperties[] = 
{
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		yuv420ReaderTrackGetMediaType,		NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		yuv420ReaderTrackGetFormat,			NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			yuv420ReaderTrackGetFormatInfo,		NULL},
	{kFskMediaPropertyFrameRate,			kFskMediaPropertyTypeRatio,			yuv420ReaderTrackGetFrameRate,		NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		yuv420ReaderTrackGetBitRate,		NULL},
	{kFskMediaPropertyDimensions,			kFskMediaPropertyTypeDimension,		yuv420ReaderTrackGetDimensions,		yuv420ReaderTrackSetDimensions},
#ifdef SIMULATE_CAMERA
	//{kFskMediaPropertyDimensionList,		kFskMediaPropertyTypeUInt32List,	yuv420ReaderTrackGetDimensionList,	NULL},
	{kFskMediaPropertyEnabled,              kFskMediaPropertyTypeBoolean,   	NULL,                               yuv420ReaderTrackSetEnabled},
#endif
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,								NULL}
};

FskMediaReaderTrackDispatchRecord gYUV420ReaderVideoTrack = {yuv420ReaderVideoTrackProperties};

enum 
{
	kYUV420MediaTypeUnknown = 0,
	kYUV420MediaTypeAudio,
	kYUV420MediaTypeVideo,
	kYUV420MediaTypeImageJFIF
};


#define YUV420_HEADER_SIZE				12
#define YUV420_DEFAULT_TIME_SCALE		3000
#define kExtractInterval                66


typedef struct YUV420ReaderRecord YUV420ReaderRecord;
typedef struct YUV420ReaderRecord *YUV420Reader;

typedef struct YUV420ReaderTrackRecord YUV420ReaderTrackRecord;
typedef struct YUV420ReaderTrackRecord *YUV420ReaderTrack;

struct YUV420ReaderTrackRecord 
{
	YUV420ReaderTrack				next;
	FskMediaReaderTrackRecord		reader_track;
	YUV420Reader					yuv420;
    char                            *media_type;
};


struct YUV420ReaderRecord 
{
	FskMediaSpooler			spooler;
	Boolean					spoolerOpen;
	Boolean					dontSeekIfExpensive;
	Boolean					scrub;
	

	int						initialized;
	int						seek_offset;
	int						width;
	int						height;
    int                     dimenstion_count;
	int						time_scale;
	int						frame_size;
    unsigned char           *yuv420_bytes;
	int						frame_total;
    int                     brightness;
	int						frame_dur;
	int						frame_rate;
	FskInt64				duration_fsk64;
	FskInt64				file_size_fsk64;
	FskInt64				progress_fsk64;
	FskMediaReader			reader;

	int						pixel_format;
	int						video_frame_count;
	unsigned char			*yuv420_header[YUV420_HEADER_SIZE];

	YUV420ReaderTrack		yuv420_tracks;
#ifdef SIMULATE_CAMERA
    FskDimensionRecord      *dimension_list;    //**dimension_list test code
    unsigned char           *photo_data;
    int                     photo_data_size;

    FskTimeCallBack			extractTimer;
	UInt32					extractInterval;
#endif
};

typedef FskErr (*yuv420ChunkWalker)(YUV420Reader yuv420, FskInt64 offset, FskInt64 size);

typedef struct 
{
	UInt32				chunkType;
	UInt32				listType;
	yuv420ChunkWalker		walker;
} yuv420ChunkWalkersRecord, *yuv420ChunkWalkers;

	
//static int CHECK_ERR( int err )
//{
//	return 0;
//}


static FskErr doRead(YUV420Reader yuv420, FskInt64 offset, UInt32 size, void *bufferIn)
{
	FskErr err = kFskErrNone;
	unsigned char *buffer = bufferIn;
	
	dlog( "into doRead(): offset: %d, size : %d, ", (int)offset, (int)size ); 
	while (0 != size || err == kFskErrEndOfFile ) 
	{
		unsigned char *readBuffer;
		UInt32 bytesRead = 0;

		if (yuv420->dontSeekIfExpensive)
			yuv420->spooler->flags |= kFskMediaSpoolerDontSeekIfExpensive;

		err = FskMediaSpoolerRead(yuv420->spooler, offset, size, &readBuffer, &bytesRead);
		if (yuv420->dontSeekIfExpensive)
			yuv420->spooler->flags &= ~kFskMediaSpoolerDontSeekIfExpensive;
		if( err != kFskErrEndOfFile )
		{
			BAIL_IF_ERR( err );
		}
		
		FskMemMove(buffer, readBuffer, bytesRead);

		offset += bytesRead;
		buffer += bytesRead;
		size -= bytesRead;
	}

bail:
	dlog( "out of doRead() err: %d", (int)err );
	
	return err;
}


FskErr yuv420Instantiate(YUV420Reader yuv420)
{
	unsigned char *s = (unsigned char *)yuv420->yuv420_header;
	FskErr	err = 0;
	
	dlog("into yuv420Instantiate()");
	if( yuv420->initialized )
	{
		dlog("already initialized!!!");
		goto bail;
	}
	
	//yuv420 header
	//012345 67 89 ab
	//yuv420,fr,wd,ht,
	
	err = doRead(yuv420, 0, YUV420_HEADER_SIZE, yuv420->yuv420_header );
	if( err == kFskErrNeedMoreTime )
		goto bail;
	//BAIL_IF_ERR( err );

	err = yuv420->spooler->doGetSize(yuv420->spooler, &yuv420->file_size_fsk64);
	BAIL_IF_ERR( err );
		
	dlog("parsing header:");
	if
	( 
	   s[0] == 'y' &&
	   s[1] == 'u' &&
	   s[2] == 'v' &&
	   s[3] == '4' &&
	   s[4] == '2' &&
	   s[5] == '0' 
	)
	{
		dlog("found yuv420!");
		yuv420->pixel_format = kFskBitmapFormatYUV420;
	}
	else if
	( 
		s[0] == '2' &&
		s[1] == 'v' &&
		s[2] == 'u' &&
		s[3] == 'y' &&
		s[4] == ' ' &&
		s[5] == ' ' 
	)
	{
		dlog("found 2vuy  !");
		yuv420->pixel_format = kFskBitmapFormatUYVY;
	}
	else if
        (
         s[0] == 's' &&
         s[1] == 'p' &&
         s[2] == 'u' &&
         s[3] == 'v' &&
         s[4] == ' ' &&
         s[5] == ' '
         )
	{
		dlog("found spuv  !");
		yuv420->pixel_format = kFskBitmapFormatYUV420spuv;
	}
	else if
        (
         s[0] == 's' &&
         s[1] == 'p' &&
         s[2] == 'v' &&
         s[3] == 'u' &&
         s[4] == ' ' &&
         s[5] == ' '
         )
	{
		dlog("found spvu  !");
		yuv420->pixel_format = kFskBitmapFormatYUV420spvu;
	}
	else
	{
		err = kFskErrUnimplemented;
		goto bail;
	}
	
	mlog("got yuv420->pixel_format: %d", yuv420->pixel_format);
	
	s += 6;
	yuv420->frame_rate  = (s[0]<<8)|(s[1]<<0); s+=2;
	yuv420->width       = (s[0]<<8)|(s[1]<<0); s+=2;
	yuv420->height      = (s[0]<<8)|(s[1]<<0);

	mlog("got yuv420->frame_rate: %d", yuv420->frame_rate);
	mlog("got yuv420->width: %d", yuv420->width);
	mlog("got yuv420->height: %d", yuv420->height);

#ifdef SIMULATE_CAMERA
    {//**dimension_list test code
        int i;
        yuv420->dimenstion_count = 2;
        err = FskMemPtrNewClear(sizeof(FskDimensionRecord)*yuv420->dimenstion_count, &yuv420->dimension_list);
        BAIL_IF_ERR( err );

        for( i = 0; i < yuv420->dimenstion_count; i++ )
        {
            FskDimensionRecord	*this_dimension = yuv420->dimension_list + i;
            this_dimension->width  = yuv420->width  >> i;
            this_dimension->height = yuv420->height >> i;
        }
    }
#endif

	if( yuv420->pixel_format == kFskBitmapFormatYUV420      ||
        yuv420->pixel_format == kFskBitmapFormatYUV420spuv  ||
        yuv420->pixel_format == kFskBitmapFormatYUV420spvu
       )
		yuv420->frame_size  = (yuv420->width*yuv420->height)/2*3;
	else
		yuv420->frame_size  = (yuv420->width<<1)*yuv420->height;

	yuv420->time_scale  = YUV420_DEFAULT_TIME_SCALE;
	yuv420->frame_total = yuv420->file_size_fsk64 / yuv420->frame_size;
	yuv420->frame_dur   = yuv420->time_scale / yuv420->frame_rate;
	
	yuv420->duration_fsk64 = yuv420->frame_total == 0 ? -1 : yuv420->frame_dur * yuv420->frame_total;

	mlog("got yuv420->time_scale: %d", yuv420->time_scale);
	mlog("got yuv420->frame_total: %d", yuv420->frame_total);
	mlog("got yuv420->frame_dur: %d", yuv420->frame_dur);
	
	err = FskMemPtrNew(yuv420->frame_size, &yuv420->yuv420_bytes);
	BAIL_IF_ERR( err );
    
	yuv420->seek_offset = 0;
	yuv420->initialized = 1;

	{
		YUV420ReaderTrack  track;
		
		dlog( "creating yuv420 track.");
		err = FskMemPtrNewClear(sizeof(YUV420ReaderTrackRecord), &track);
		BAIL_IF_ERR( err );
		
		track->yuv420				 = yuv420;
		track->reader_track.dispatch = &gYUV420ReaderVideoTrack;
		track->reader_track.state	 = track;
		track->media_type  =  FskStrDoCopy("video");

		FskListAppend(&yuv420->yuv420_tracks, track);

#ifdef SIMULATE_CAMERA
		dlog( "creating fake cama video preview track.");
		err = FskMemPtrNewClear(sizeof(YUV420ReaderTrackRecord), &track);
		BAIL_IF_ERR( err );

		track->yuv420				 = yuv420;
		track->reader_track.dispatch = &gYUV420ReaderVideoTrack;
		track->reader_track.state	 = track;
		track->media_type  =  FskStrDoCopy("video-preview");

		FskListAppend(&yuv420->yuv420_tracks, track);

		dlog( "creating fake cama photo track.");
		err = FskMemPtrNewClear(sizeof(YUV420ReaderTrackRecord), &track);
		BAIL_IF_ERR( err );
        
		track->yuv420				 = yuv420;
		track->reader_track.dispatch = &gYUV420ReaderVideoTrack;
		track->reader_track.state	 = track;
		track->media_type  =  FskStrDoCopy("image");

		FskListAppend(&yuv420->yuv420_tracks, track);
#endif
    }
	
	(yuv420->reader->doSetState)(yuv420->reader, kFskMediaPlayerStateStopped);

bail:
	dlog("out of yuv420Instantiate, initialized: %d", (int)yuv420->initialized);
	return err;
}


FskErr yuv420SpoolerCallback(void *clientRefCon, UInt32 operation, void *param)
{
	YUV420Reader yuv420 = clientRefCon;
	FskErr err = kFskErrNone;

	dlog("into yuv420SpoolerCallback()");;

	switch (operation) 
	{
		case kFskMediaSpoolerOperationGetHeaders:
			yuv420->spooler->flags |= kFskMediaSpoolerCantSeek;
			break;

		case kFskMediaSpoolerOperationDataReady:
			if (yuv420->reader->mediaState < kFskMediaPlayerStateStopped)
				err = yuv420Instantiate(yuv420);
			break;

		default:
			return kFskErrUnimplemented;
	}

	return err;
}

//
//
//demuxer interface
//
//

Boolean yuv420ReaderCanHandle(const char *mimeType)
{
	Boolean can_handle = false;

	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderCanHandle(), mimeType: %s", mimeType);

	if 
	(	
		 0 == FskStrCompareCaseInsensitive("video/yuv420",       mimeType) ||
		 0 == FskStrCompareCaseInsensitive("video/2vuy",         mimeType) ||
         0 == FskStrCompareCaseInsensitive("video/yuv420spuv",   mimeType) ||
         0 == FskStrCompareCaseInsensitive("video/yuv420spvu",   mimeType)
	)
		can_handle = true;

	mlog("yuv420ReaderCanHandle(), mimeType: %s, returns can_handle: %d", mimeType, can_handle);

	return can_handle;
}


FskErr yuv420ReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	int err = kFskErrUnknownElement;
	//int offset = 0;

	mlog("###########################################################################################");
	mlog("into yuv420ReaderSniff()");

	if (dataSize >= 12) 
	{	
		if
		( 
		   data[0] == 'y' &&
		   data[1] == 'u' &&
		   data[2] == 'v' &&
		   data[3] == '4' &&
		   data[4] == '2' &&
		   data[5] == '0' 
		)
		{
			*mime = FskStrDoCopy("video/yuv420");
			err = 0;
		}
		else if
            (
             data[0] == '2' &&
             data[1] == 'v' &&
             data[2] == 'u' &&
             data[3] == 'y' &&
             data[4] == ' ' &&
             data[5] == ' '
             )
		{
			*mime = FskStrDoCopy("video/2vuy");
			err = 0;
		}
		else if
            (
             data[0] == 's' &&
             data[1] == 'p' &&
             data[2] == 'u' &&
             data[3] == 'v' &&
             data[4] == ' ' &&
             data[5] == ' '
             )
		{
			*mime = FskStrDoCopy("video/yuv420spuv");
			err = 0;
		}
		else if
            (
             data[0] == 's' &&
             data[1] == 'p' &&
             data[2] == 'v' &&
             data[3] == 'u' &&
             data[4] == ' ' &&
             data[5] == ' '
             )
		{
			*mime = FskStrDoCopy("video/yuv420spvu");
			err = 0;
		}
	}

	if( *mime != NULL )
		mlog(" yuv420ReaderSniff() returning *mime: %s", *mime);

//bail:
	return err;
}


FskErr yuv420ReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	YUV420Reader yuv420 = NULL;

	mlog( "###########################################################################################" );
	mlog("into yuv420ReaderNew()");;

	if (NULL == spooler) 
	{
		dlog("NULL == spooler, goto bail!!!");;
		err = kFskErrUnimplemented;
		goto bail;
	}
	
	err = FskMemPtrNewClear(sizeof(YUV420ReaderRecord), &yuv420);
	BAIL_IF_ERR( err );

	*readerState = yuv420;			// must be set before anything that might issue a callback
	yuv420->spooler = spooler;
	yuv420->reader = reader;

	yuv420->spooler->onSpoolerCallback = yuv420SpoolerCallback;
	yuv420->spooler->clientRefCon		 = yuv420;
	yuv420->spooler->flags |= kFskMediaSpoolerForwardOnly;

	if (spooler->doOpen) 
	{
		dlog("opening spooler");;
		err = (spooler->doOpen)(spooler, kFskFilePermissionReadOnly);
		BAIL_IF_ERR( err );

		yuv420->spoolerOpen = true;
	}

    
	dlog("calling reader->doSetState");;
	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);
    
	err = yuv420Instantiate(yuv420);
	if (err) 
	{
		if (kFskErrNeedMoreTime == err)
			err = kFskErrNone;
		goto bail;
	}

#ifdef SIMULATE_CAMERA
	if (NULL == yuv420->extractTimer)
    {
        yuv420->extractInterval = 1000/yuv420->frame_rate;//kExtractInterval;
		FskTimeCallbackNew(&yuv420->extractTimer);
		BAIL_IF_NULL( yuv420->extractTimer, err, kFskErrUnknown );
	}

    yuv420->photo_data = NULL;
    yuv420->photo_data_size = 0;
#endif

bail:
	if ((kFskErrNone != err) && (NULL != yuv420)) 
	{
		yuv420ReaderDispose(reader, yuv420);
		yuv420 = NULL;
	}

	*readerState = yuv420;
	
	dlog("out of yuv420ReaderNew()");;
	return err;
}


FskErr yuv420ReaderDispose(FskMediaReader reader, void *readerState)
{
	YUV420Reader yuv420 = readerState;

	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderDispose()");;

	if( yuv420 != NULL )
	{
		while (yuv420->yuv420_tracks) 
		{
			YUV420ReaderTrack track = yuv420->yuv420_tracks;
			yuv420->yuv420_tracks = track->next;
			FskMemPtrDispose(track);
		}

		if (yuv420->spoolerOpen && yuv420->spooler->doClose)
			(yuv420->spooler->doClose)(yuv420->spooler);


#ifdef SIMULATE_CAMERA
        if( yuv420->extractTimer )
        {
            FskTimeCallbackDispose(yuv420->extractTimer);
            yuv420->extractTimer = NULL;
        }
        
        //**dimension_list test code
        FskMemPtrDisposeAt(&yuv420->dimension_list);
        FskMemPtrDisposeAt(&yuv420->photo_data);
#endif

        FskMemPtrDisposeAt(&yuv420->yuv420_bytes);
		FskMemPtrDispose(yuv420);
	}

	return kFskErrNone;
}

FskErr yuv420ReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track)
{
	YUV420Reader	  yuv420  = readerState;
	YUV420ReaderTrack walker  = yuv420->yuv420_tracks;

	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderGetTrack(), index: %d", index);

	if( !yuv420->initialized )
	{
		dlog("yuv420->initialized false, bail!!!");
		return -1;
	}
	
	while((index > 0) && walker) 
	{
		walker = walker->next;
		index -= 1;
	}

	if( walker ) 
	{
		*track = &walker->reader_track;
		mlog("track found, *track: %x", (int)*track );
		return kFskErrNone;
	}
	else
	{
		mlog("no track found!!!" );
		return kFskErrNotFound;
	}
}


FskErr yuv420ReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	YUV420Reader	yuv420 = readerState;
	FskInt64		start_time_fsk64  = startTime == NULL ? 0 : *startTime;
	FskErr			err = kFskErrNone;
	
	mlog( "###########################################################################################" );
	mlog("into yuv420ReaderStart(), start_time_fsk64: %d", (int)start_time_fsk64);
	
	if( !yuv420->initialized )
	{
		mlog("yuv420->initialized false, bail!!!");
		return -1;
	}
	
	yuv420->seek_offset = start_time_fsk64 / yuv420->frame_dur * yuv420->frame_size;
	yuv420->video_frame_count	= 0;

#ifdef SIMULATE_CAMERA
    if( yuv420->frame_rate != 0 )
        yuv420->extractInterval = 1000/yuv420->frame_rate;
	FskTimeCallbackScheduleFuture(yuv420->extractTimer, 0, yuv420->extractInterval, askExtractMoreCallback, yuv420);
#endif
	return err;
}


FskErr yuv420ReaderStop(FskMediaReader reader, void *readerState)
{
	mlog( "###########################################################################################" );
	mlog("into yuv420ReaderStop()");

#ifdef SIMULATE_CAMERA
	mlog("closing extractTimer");
	{
		YUV420Reader yuv420 = readerState;
		if( yuv420->extractTimer )
		{
			FskTimeCallbackDispose(yuv420->extractTimer);
			yuv420->extractTimer = NULL;
		}
    }
#endif
    
	return kFskErrNone;
}


void frame_event_callback(void *refCon, int event )
{
	YUV420Reader  yuv420  = (YUV420Reader)refCon;
    FskMediaReader reader = yuv420->reader;
    
	dlog("////////////////////////////////////////////////////");
    dlog("FRAME ARRIVING: refCon: %x, event: %d", (int)refCon, event);
    dlog("////////////////////////////////////////////////////");
    
    (reader->eventHandler)(reader, reader->eventHandlerRefCon, kFskEventMediaReaderDataArrived, NULL);
    
}

#ifdef SIMULATE_CAMERA
void askExtractMoreCallback(FskTimeCallBack callback, const FskTime time, void *param)
{
 	YUV420Reader  yuv420  = (YUV420Reader)param;
    
    frame_event_callback((void *)yuv420, kFskEventMediaReaderDataArrived );
   
 	FskTimeCallbackScheduleFuture(yuv420->extractTimer, 0, yuv420->extractInterval, askExtractMoreCallback, yuv420);
}
#endif

FskErr yuv420ReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *trackOut, UInt32 *infoCountOut, FskMediaReaderSampleInfo *infoOut, unsigned char **data)
{
	YUV420Reader			 yuv420 = readerState;
	YUV420ReaderTrack		 track	= NULL;
	int						 fsk_time = 0;
	unsigned char			 *yuv420_bytes = NULL;
	FskMediaReaderSampleInfo info = NULL;
	FskErr					 err = kFskErrNone;

	mlog( "###########################################################################################" );
	mlog("into yuv420ReaderExtract()");

	*infoCountOut	= 0;
	*infoOut		= NULL;
	*trackOut		= NULL;
	*data			= NULL;

	if( !yuv420->initialized )
	{
		mlog("yuv420->initialized false, bail!!!");
		return -1;
	}
    
#ifdef SIMULATE_CAMERA
	for (track = yuv420->yuv420_tracks; NULL != track; track = track->next)
    {
        if(	0 == FskStrCompareCaseInsensitive("image", track->media_type) )
        {
			mlog("@image track");
            break;
        }
	}

    if( track != NULL )
    {
        unsigned char *photo_data = yuv420->photo_data;
        int photo_data_size = yuv420->photo_data_size;
        yuv420->photo_data = NULL;
        yuv420->photo_data_size = 0;
        dlog("check if there is any photo data");
        if( photo_data_size != 0 )
        {
            mlog("allocating a new sample record for photo");
            err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
            BAIL_IF_ERR( err );
            
            info->flags				= kFskImageFrameImmediate;
            info->samples			= 1;
            info->decodeTime		= 0;
            info->compositionTime	= -1;
            
            *infoCountOut			= 1;
            *infoOut				= info;
            *trackOut				= &track->reader_track;
            
            info->sampleSize	= photo_data_size;
            *data				= photo_data;
            
            mlog("done packing a new sample record for photo, photo_data_size: %d", photo_data_size);
            
            goto bail;
        }
    }
#endif

	//only look at "video" track, "video-preview" track is dummy here
	for (track = yuv420->yuv420_tracks; NULL != track; track = track->next)
	{
		if(	0 == FskStrCompareCaseInsensitive("video", track->media_type) )
		{
			mlog("@video track");
			break;
		}
	}

	fsk_time = yuv420->seek_offset/yuv420->frame_size*yuv420->frame_dur;
	if( fsk_time >= yuv420->duration_fsk64 && yuv420->frame_total != 0 )
	{
		mlog("no more frames, returning err: kFskErrEndOfFile");
		err = kFskErrEndOfFile;

#ifdef SIMULATE_CAMERA
        if( yuv420->extractTimer )
        {
            FskTimeCallbackDispose(yuv420->extractTimer);
            yuv420->extractTimer = NULL;
        }
#endif
        
		goto bail;
	}

	err = FskMemPtrNew(yuv420->frame_size, &yuv420_bytes);
	BAIL_IF_ERR( err );

    if( yuv420->frame_total == 0 )
    {
        int uv_value = 128;
        int y_frame_size = yuv420->frame_size / 3 * 2;
        unsigned char *yuv = yuv420_bytes;
        
        if( yuv420->pixel_format != kFskBitmapFormatYUV420 )
        {
            err = kFskErrEndOfFile;//for now
            goto bail;
        }

        memset( yuv, yuv420->brightness, y_frame_size );
        yuv += y_frame_size;
        yuv420->brightness++;
        if( yuv420->brightness > 255 )
            yuv420->brightness = 0;
        
        memset( yuv, uv_value, y_frame_size / 2 );
        
        buf_printf(0, yuv420_bytes, yuv420->width, yuv420->height, 10, 10, "Synthetic YUV420 frames from YUV Media Reader");
        buf_printf(0, yuv420_bytes, yuv420->width, yuv420->height, 10, 30, "#%d, brignness:%d", yuv420->video_frame_count, yuv420->brightness);

    }
    else
    {
        err = doRead( yuv420, yuv420->seek_offset, yuv420->frame_size, yuv420_bytes );
        if( err != kFskErrEndOfFile )
        {	
            BAIL_IF_ERR( err );
        }
	}
    
	yuv420->seek_offset += yuv420->frame_size;
		
	err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
	BAIL_IF_ERR( err );
	

	info->flags				= kFskImageFrameTypeSync;
	info->samples			= 1;
	info->sampleSize		= yuv420->frame_size;
	info->decodeTime		= (FskInt64)fsk_time;
	info->sampleDuration	= yuv420->frame_dur;
	info->compositionTime	= -1;

	*infoCountOut			= 1;
	*infoOut				= info;
	*trackOut				= &track->reader_track;
	*data					= yuv420_bytes;
	
    //cache latest frame
    memcpy(yuv420->yuv420_bytes,yuv420_bytes,yuv420->frame_size);
    
	mlog("outputting 1 video frame: size: %d, decode_time: %d, sample_flag: %d",
			(int)info->sampleSize, (int)info->decodeTime, (int)info->flags);

	yuv420->video_frame_count++;

bail:
	if( *infoOut == NULL && err != kFskErrEndOfFile )
		err = kFskErrNeedMoreTime;

	return err;
}


FskErr yuv420ReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderGetMetadata(), propertyID: %d, index: %d", (int)metaDataType, (int)index);
	return kFskErrUnknownElement;
}


FskErr yuv420ReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	YUV420Reader	yuv420 = state;
	float			dur = 0;
	
	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderGetDuration(), propertyID: %d", (int)propertyID);
	if( !yuv420->initialized )
	{
		dlog("yuv420->initialized false, bail!!!");
		return -1;
	}
	
	dur = (float)yuv420->duration_fsk64;

	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = dur;

	mlog("returning duration: %f", property->value.number);

	return kFskErrNone;
}


FskErr yuv420ReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	YUV420Reader yuv420 = state;

	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderGetTimeScale(), propertyID: %d", (int)propertyID);
	if( !yuv420->initialized )
	{
		dlog("yuv420->initialized false, bail!!!");
		return -1;
	}
	
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = yuv420->time_scale;

	mlog("returning time_scale: %d", property->value.integer);

	return kFskErrNone;
}


FskErr yuv420ReaderSetScrub(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	YUV420Reader yuv420 = state;
	
	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderSetScrub(), propertyID: %d", (int)propertyID);
	if( !yuv420->initialized )
	{
		dlog("yuv420->initialized false, bail!!!");
		return -1;
	}
	
	yuv420->scrub = property->value.b;

	return kFskErrNone;
}

FskErr yuv420ReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "video/yuv420\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}



//
//
//track interface
//
//

FskErr yuv420ReaderTrackGetMediaType(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	YUV420ReaderTrack track = state;
	YUV420Reader		yuv420  = track->yuv420;

	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderTrackGetMediaType(), propertyID: %d", (int)propertyID);
	if( !yuv420->initialized )
	{
		dlog("yuv420->initialized false, bail!!!");
		return -1;
	}
	
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy(track->media_type);

    dlog("got media, property->value.str: %s", property->value.str);

	return kFskErrNone;
}

FskErr yuv420ReaderTrackGetFormat(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	YUV420ReaderTrack track = state;
	YUV420Reader		yuv420  = track->yuv420;

	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderTrackGetFormat(), propertyID: %d", (int)propertyID);
	if( !yuv420->initialized )
	{
		dlog("yuv420->initialized false, bail!!!");
		return -1;
	}
	
	property->type = kFskMediaPropertyTypeString;
	dlog("checking yuv420->pixel_format: %d", yuv420->pixel_format);;
	if( yuv420->pixel_format == kFskBitmapFormatYUV420 )
	{
		dlog("returning x-video-codec/yuv420");;
		property->value.str = FskStrDoCopy("x-video-codec/yuv420");
	}
	else if( yuv420->pixel_format == kFskBitmapFormatUYVY )
	{
		dlog("returning x-video-codec/2vuy");;
		property->value.str = FskStrDoCopy("x-video-codec/2vuy");
	}
	else if( yuv420->pixel_format == kFskBitmapFormatYUV420spuv )
	{
		dlog("returning x-video-codec/yuv420spuv");;
		property->value.str = FskStrDoCopy("x-video-codec/yuv420spuv");
	}
	else if( yuv420->pixel_format == kFskBitmapFormatYUV420spvu )
	{
		dlog("returning x-video-codec/yuv420spvu");;
		property->value.str = FskStrDoCopy("x-video-codec/yuv420spvu");
	}
	
	return kFskErrNone;
}


FskErr yuv420ReaderTrackGetFormatInfo(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	YUV420ReaderTrack track = state;
	YUV420Reader	 yuv420 = track->yuv420;
	QTImageDescription	desc;
	int				descSize;
	FskErr			err   = kFskErrNone;

	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderTrackGetFormatInfo(), propertyID: %d", (int)propertyID);
	if( !yuv420->initialized )
	{
		dlog("yuv420->initialized false, bail!!!");
		return -1;
	}
	
	descSize = sizeof(QTImageDescriptionRecord);
	err = FskMemPtrNewClear(descSize, (FskMemPtr *)&desc);
	BAIL_IF_ERR( err );

	dlog("setting  desc->cType: %d", (int)desc->cType);
	desc->cType	 = yuv420->pixel_format;
	desc->width  = yuv420->width;
	desc->height = yuv420->height;

	property->type = kFskMediaPropertyTypeData;
	property->value.data.dataSize = descSize;
	property->value.data.data = desc;

bail:
	return err;
}


FskErr yuv420ReaderTrackGetFrameRate(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	YUV420ReaderTrack	track = state;
	YUV420Reader			yuv420 = track->yuv420;
	//int					frame_dur;
	FskErr				err = kFskErrNone;

	mlog( "###########################################################################################" );
	mlog("into yuv420ReaderTrackGetFrameRate(), propertyID: %d", (int)propertyID);
	if( !yuv420->initialized )
	{
		mlog("yuv420->initialized false, bail!!!");
		return -1;
	}
	
	property->type = kFskMediaPropertyTypeRatio;
	property->value.ratio.numer = yuv420->time_scale;
	property->value.ratio.denom = yuv420->frame_dur;

//bail:
	return err;
}


FskErr yuv420ReaderTrackSetDimensions(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	YUV420ReaderTrack track = state;
	YUV420Reader yuv420 = track->yuv420;
    int width, height, type;

	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderTrackGetDimensions(), propertyID: %d", (int)propertyID);
	if( !yuv420->initialized )
	{
		dlog("yuv420->initialized false, bail!!!");
		return -1;
	}

	type = property->type;
	width = property->value.dimension.width;
	height = property->value.dimension.height;

	return kFskErrNone;
}


FskErr yuv420ReaderTrackGetDimensions(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	YUV420ReaderTrack track = state;
	YUV420Reader yuv420 = track->yuv420;

	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderTrackGetDimensions(), propertyID: %d", (int)propertyID);
	if( !yuv420->initialized )
	{
		dlog("yuv420->initialized false, bail!!!");
		return -1;
	}
	
	property->type = kFskMediaPropertyTypeDimension;
	property->value.dimension.width  = yuv420->width;
	property->value.dimension.height = yuv420->height;

	mlog("returning width: %d", property->value.dimension.width);
	mlog("returning height: %d", property->value.dimension.height);

	return kFskErrNone;
}


FskErr yuv420ReaderTrackGetBitRate(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	YUV420ReaderTrack track = state;
	YUV420Reader	  yuv420  = track->yuv420;

	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderTrackGetBitRate(), propertyID: %d", (int)propertyID);
	if( !yuv420->initialized )
	{
		dlog("yuv420->initialized false, bail!!!");
		return -1;
	}
	
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = (SInt32)( yuv420->frame_size * yuv420->frame_rate * 8);

	mlog("returning bitrate: %d", property->value.integer);

	return kFskErrNone;
}

#ifdef SIMULATE_CAMERA
//***camera mockup
//a simple json
const char js0[] = \
"\n\
{\n\
\"previewSizes\":\n\
[\n\
{\"height\":720,\"width\":960},\n\
{\"height\":720,\"width\":1280},\n\
{\"height\":480,\"width\":640},\n\
{\"height\":288,\"width\":352},\n\
{\"height\":240,\"width\":320}\n\
]\n\
,\n\
\"pictureSizes\":\n\
[\n\
{\"height\":480,\"width\":640},\n\
{\"height\":720,\"width\":960},\n\
{\"height\":768,\"width\":1024},\n\
{\"height\":720,\"width\":1280},\n\
{\"height\":1200,\"width\":1600},\n\
{\"height\":1920,\"width\":2560},\n\
{\"height\":2448,\"width\":3264},\n\
{\"height\":1536,\"width\":2048},\n\
{\"height\":1836,\"width\":3264},\n\
{\"height\":1152,\"width\":2048},\n\
{\"height\":2176,\"width\":3264}\n\
]\n\
}";

unsigned char *js = NULL;
int js_size = 0;
char js_path[] = "/Volumes/android/js2.json"; //js1.json, js2.json are checked in under the same dir with this C file
FskErr yuv420ReaderGetJSON(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	//YUV420Reader yuv420 = state;
	FskErr		  err   = kFskErrNone;
    
    mlog("into yuv420ReaderGetJSON()");
    
    {
        FILE *f = fopen(js_path, "rb");
        if( f == NULL )
        {
            mlog( "json file not exists!!!");
            goto bail;
        }
        else
        {
            fseek(f, 0, SEEK_END);
            js_size = ftell(f);
            fseek(f, 0, SEEK_SET);
            mlog( "js_size: %d", js_size);
            
            if( js != NULL )
                free(js);
            
            js = malloc( js_size + 1 );
            fread( (void *)js, 1, js_size,  f );
            fclose(f);
            js[js_size] = 0;
        }
    }
#if 0//def SIMULATE_CAMERA
    {
        jsmn_parser p;
        jsmntok_t tokens[2000];
        int this_w, this_h;
        int r;
        int i;
        char *this_string;

        mlog("got camera params, js: %s", js);

        jsmn_init(&p);
        r = jsmn_parse(&p, (const char*)js, strlen((const char*)js), tokens, 2000);
        mlog("jsmn_parse() called, r: %d", r);

        i = 0;
        this_string = (char *)"previewSize";
        JSON_FIND_STRING( tokens, r, this_string, i );
        i++;
        if( i >= r ) goto bail;
        GET_W_H( tokens, i, this_w, this_h );
        mlog("cam param ==> defult preview size %d x %d", this_w, this_h);
        
        i = 0;
        this_string = (char *)"pictureSize";
        JSON_FIND_STRING( tokens, r, this_string, i );
        i++;
        if( i >= r ) goto bail;
        GET_W_H( tokens, i, this_w, this_h );
        mlog("cam param ==> defult photo size %d x %d", this_w, this_h);

        i = 0;
        this_string = (char *)"pictureSizeValues";
        JSON_FIND_STRING( tokens, r, this_string, i );
        i++;
        if( i >= r ) goto bail;
        if( tokens[i].type == JSMN_ARRAY )
        {
            int array_total = tokens[i].size;
            mlog("this_string: %s, is an array with total: %d", this_string, array_total);
            i++;//point to height/width object
            for( ;array_total--; )
            {
                GET_W_H( tokens, i, this_w, this_h );
                mlog("cam param ==> picture size %d x %d", this_w, this_h);
            }
        }

        this_string = (char *)"previewSizeValues";
        JSON_FIND_STRING( tokens, r, this_string, i );
        i++;
        if( i >= r ) goto bail;
        if( tokens[i].type == JSMN_ARRAY )
        {
            int array_total = tokens[i].size;
            mlog("this_string: %s, is an array with total: %d", this_string, array_total);
            i++;//point to height/width object
            for( ;array_total--; )
            {
                GET_W_H( tokens, i, this_w, this_h );
                mlog("cam param ==> preview size %d x %d", this_w, this_h);
            }
        }
    }
#endif
    property->type = kFskMediaPropertyTypeString;
	property->value.str = (char *)js;
    
bail:
	return err;
}


FskErr yuv420ReaderGetCameraCount(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	//YUV420Reader yuv420 = state;
	FskErr		 err = kFskErrNone;
    
    dlog("into yuv420ReaderGetCameraCount(), camera count is 1");
    
	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = 1;
    
//bail:
	return err;
}

FskErr yuv420ReaderSetLens(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	//YUV420Reader  yuv420 = state;
	FskErr		  err = kFskErrNone;
	char		  *s	= property->value.str;
    
	mlog("");
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");
	mlog("into yuv420ReaderSetLens(), camera name: %s", s);;
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");
    
	if( strcmp( s, "back" ) == 0 )
    {
		mlog("choosing back camera!!!");;
	}
	else if( strcmp( s, "front" ) == 0 )
    {
		mlog("choosing front camera!!!");;
	}
	
//bail:
	mlog("out of yuv420ReaderSetLens() : %s", s);
	return err;
}


FskErr yuv420ReaderSetAutoFocusState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
//	YUV420Reader  yuv420 = state;
	FskErr		  err   = kFskErrNone;
	int		  af_state	= property->value.integer;
    
    mlog("into yuv420ReaderSetAutoFocusState");
    
	if( af_state == 1 )
    {
		mlog("autoFocus!!!");;
	}
    
//bail:
	mlog("out of yuv420ReaderSetAutoFocusState() : %d", af_state);
	return err;
    
}


FskErr yuv420ReaderTrackGetDimensionList(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	YUV420ReaderTrack track = state;
	YUV420Reader yuv420 = track->yuv420;
    
	dlog( "###########################################################################################" );
	dlog("into yuv420ReaderTrackGetDimensionList(), propertyID: %d", (int)propertyID);
	if( !yuv420->initialized )
	{
		dlog("yuv420->initialized false, bail!!!");
		return -1;
	}
    
	property->type = kFskMediaPropertyTypeUInt32List;
    property->value.integers.count   = yuv420->dimenstion_count * 2;
	property->value.integers.integer = (UInt32 *)yuv420->dimension_list;
    
	return kFskErrNone;
}

FskErr save_jpeg(unsigned char *yuv_bytes, int size, int yuv_format, int width, int height, unsigned char **data, int *data_size );
FskErr yuv420ReaderTrackSetEnabled(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	YUV420ReaderTrack track = state;
	YUV420Reader yuv420 = track->yuv420;
	FskErr		  err = kFskErrNone;
	Boolean		  enabled	= property->value.b;
    
	if( !yuv420->initialized )
	{
		dlog("yuv420ReaderTrackSetEnabled()=>yuv420->initialized false, bail!!!");
		return -1;
	}
 
	mlog("");
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");
	mlog("into yuv420ReaderTrackSetEnabled(), yuv420->media_type: %s, enabled: %d", track->media_type, enabled);
	mlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");
    
   
	if( enabled )
    {
		unsigned char *photo_data=NULL;
		int photo_data_size=0;
        mlog("taking a picture!!!");
   
        err = save_jpeg(yuv420->yuv420_bytes, yuv420->frame_size, yuv420->pixel_format, yuv420->width,yuv420->height,
                        &photo_data, &photo_data_size );
        
        FskMemPtrDisposeAt(&yuv420->photo_data);
        yuv420->photo_data = photo_data;
        yuv420->photo_data_size = photo_data_size;
        
        if(0)
        {
            FILE *f = fopen("/Volumes/android/yuv_cam.jpg", "wb");
            if(f)
            {
                fwrite( photo_data, 1, photo_data_size, f );
                fclose(f);
            }
        }
        
        mlog("////////////////////////////////////////////////////");
        mlog("CAMERA FRAME ARRIVING");
        mlog("////////////////////////////////////////////////////");
        frame_event_callback((void *)yuv420, kFskEventMediaReaderDataArrived );
    }
    
//bail:
	mlog("out of yuv420ReaderTrackSetEnabled()");
	return err;
}


FskErr save_jpeg(unsigned char *yuv_bytes, int size, int yuv_format, int width, int height, unsigned char **data, int *data_size )
{
    FskImageCompress    image_comp;
    char                mime_output[]="image/jpeg";
    FskBitmap           bits = NULL;
    int                 pix_depth = 16;
    void                *frame = NULL;
    UInt32              frameSize, flags;
    int                 yuv_stride = yuv_format == kFskBitmapFormatUYVY ? width *2 : width;
	FskErr              err = kFskErrNone;

    mlog("creating compressor");
    err = FskImageCompressNew(&image_comp, 0, mime_output, width, height );
    BAIL_IF_ERR(err);
    
    mlog("into KprMediaWriterAddFrame");
    err = FskBitmapNewWrapper(width, height, yuv_format, pix_depth, (void *)yuv_bytes, yuv_stride, &bits);
    BAIL_IF_ERR(err);
    
    err = FskImageCompressFrame(image_comp, bits, &frame, &frameSize, NULL, NULL, &flags, NULL, NULL);
    BAIL_IF_ERR(err);

    mlog("save_jpeg=>frameSize: %d, flags: %d", frameSize, flags);
    err = FskBitmapDispose( bits );
    BAIL_IF_ERR(err);

    FskImageCompressDispose(image_comp);
    
    *data = frame;
    *data_size = frameSize;
    
bail:
    return err;
}
#endif



