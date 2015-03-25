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

#include "mpeg_x_demuxer.h"
#include "mpeg_ts_demuxer.h"
#include "mpeg_ps_demuxer.h"

#include "kinoma_ipp_lib.h"
#include "kinoma_avc_header_parser.h"


static Boolean mpegReaderCanHandle(const char *mimeType);
static FskErr mpegReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler source);
static FskErr mpegReaderDispose(FskMediaReader reader, void *readerState);
static FskErr mpegReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track);
static FskErr mpegReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime);
static FskErr mpegReaderStop(FskMediaReader reader, void *readerState);
static FskErr mpegReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *track, UInt32 *infoCount, FskMediaReaderSampleInfo *info, unsigned char **data);
static FskErr mpegReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags);
static FskErr mpegReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime);

static FskErr mpegReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mpegReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mpegReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mpegReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mpegReaderSetScrub(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mpegReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

int validate_mpeg( unsigned char *d, int size, int *is_ts, int *offset_out );

static FskMediaPropertyEntryRecord mpegReaderProperties[] = 
{
	{kFskMediaPropertyDuration,				kFskMediaPropertyTypeFloat,			mpegReaderGetDuration,		NULL},
	{kFskMediaPropertyTime,					kFskMediaPropertyTypeFloat,			mpegReaderGetTime,			NULL},
	{kFskMediaPropertyTimeScale,			kFskMediaPropertyTypeInteger,		mpegReaderGetTimeScale,		NULL},
	{kFskMediaPropertyState,				kFskMediaPropertyTypeInteger,		mpegReaderGetState,			NULL},
	{kFskMediaPropertyScrub,				kFskMediaPropertyTypeBoolean,		NULL,						mpegReaderSetScrub},
	{kFskMediaPropertyDLNASinks,			kFskMediaPropertyTypeStringList,	mpegReaderGetDLNASinks,		NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,						NULL}
};

FskMediaReaderDispatchRecord gMediaReaderMPEG = {mpegReaderCanHandle, mpegReaderNew, mpegReaderDispose, mpegReaderGetTrack, mpegReaderStart, mpegReaderStop, mpegReaderExtract, mpegReaderGetMetadata, mpegReaderProperties, mpegReaderSniff};

static FskErr mpegReaderTrackGetMediaType(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mpegReaderTrackGetFormat(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mpegReaderTrackGetFormatInfo(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mpegReaderVideoTrackGetFrameRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mpegReaderAudioTrackGetSampleRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mpegReaderAudioTrackGetChannelCount(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mpegReaderVideoTrackGetDimensions(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mpegReaderTrackGetBitRate(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr mpegReaderTrackGetProfile(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property);

static FskMediaPropertyEntryRecord mpegReaderAudioTrackProperties[] = 
{
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		mpegReaderTrackGetMediaType,		NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		mpegReaderTrackGetFormat,			NULL},
	{kFskMediaPropertySampleRate,			kFskMediaPropertyTypeInteger,		mpegReaderAudioTrackGetSampleRate,	NULL},
	{kFskMediaPropertyChannelCount,			kFskMediaPropertyTypeInteger,		mpegReaderAudioTrackGetChannelCount,NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		mpegReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			mpegReaderTrackGetFormatInfo,		NULL},
	{kFskMediaPropertyProfile,				kFskMediaPropertyTypeString,		mpegReaderTrackGetProfile,			NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,								NULL}
};

static FskMediaPropertyEntryRecord mpegReaderVideoTrackProperties[] = 
{
	{kFskMediaPropertyMediaType,			kFskMediaPropertyTypeString,		mpegReaderTrackGetMediaType,		NULL},
	{kFskMediaPropertyFormat,				kFskMediaPropertyTypeString,		mpegReaderTrackGetFormat,			NULL},
	{kFskMediaPropertyFormatInfo,			kFskMediaPropertyTypeData,			mpegReaderTrackGetFormatInfo,		NULL},
	{kFskMediaPropertyFrameRate,			kFskMediaPropertyTypeRatio,			mpegReaderVideoTrackGetFrameRate,	NULL},
	{kFskMediaPropertyDimensions,			kFskMediaPropertyTypeDimension,		mpegReaderVideoTrackGetDimensions,	NULL},
	{kFskMediaPropertyBitRate,				kFskMediaPropertyTypeInteger,		mpegReaderTrackGetBitRate,			NULL},
	{kFskMediaPropertyProfile,				kFskMediaPropertyTypeString,		mpegReaderTrackGetProfile,			NULL},
	{kFskMediaPropertyUndefined,			kFskMediaPropertyTypeUndefined,		NULL,								NULL}
};

FskMediaReaderTrackDispatchRecord gMPEGReaderAudioTrack = {mpegReaderAudioTrackProperties};
FskMediaReaderTrackDispatchRecord gMPEGReaderVideoTrack = {mpegReaderVideoTrackProperties};


enum 
{
	kMPEGMediaTypeUnknown = 0,
	kMPEGMediaTypeAudio,
	kMPEGMediaTypeVideo,
	kMPEGMediaTypeImageJFIF
};


#define 	GET_TIME_64(t)									\
(														\
	(FskInt64)(((FskInt64)GET_TIME_64_Hi(t))<<32)|		\
	(FskInt64)GET_TIME_64_Lo(t)							\
)

typedef struct MPEGReaderRecord MPEGReaderRecord;
typedef struct MPEGReaderRecord *MPEGReader;

typedef struct MPEGReaderTrackRecord MPEGReaderTrackRecord;
typedef struct MPEGReaderTrackRecord *MPEGReaderTrack;

struct MPEGReaderTrackRecord 
{
	MPEGReaderTrack				next;
	FskMediaReaderTrackRecord	reader_track;
	MPEGReader					mpeg;

	char						*format;
	UInt32						mediaType;
	
	int							profile;
	int							level;
	int							bps;

	UInt8						*codecSpecificDataRef;
	UInt32						codecSpecificDataSize;

	union 
	{
		struct 
		{
			UInt32				samplerate;
			UInt32				channel_total;
		} audio;

		struct 
		{
			FskDimensionRecord	dimensions;
			FskImageDecompress	deco;
		} video;
	};
};


 struct MPEGReaderRecord 
 {
	FskMediaSpooler			spooler;
	Boolean					spoolerOpen;
	Boolean					dontSeekIfExpensive;
	Boolean					scrub;
	
	int						is_ts;
	FskInt64				total_size_fsk64;
	int						audio_total_size;
	//FskInt64				video_total_size_fsk64;
	FskInt64				progress;
	int						valid_offset;
	int						seek_offset;
	FskInt64				duration;
	FskInt64				time_init_fsk64;
	FskMediaReader			reader;

	int						video_frame_count;
	int						wait_for_video_sync_frame;
	int 					last_B_cmptime;
	int 					last_P_cmptime;
	int 					last_dectime;
	int 					dectime_interval;
	//TIME_64				early_time_pre;
	//TIME_64				later_time_pre;
	TIME_64					early_time_dec;
	TIME_64					later_time_dec;
	unsigned char			*mpeg_bytes;
	int						mpeg_bytes_size;

	MPEGReaderTrack			mpeg_tracks;
	MPEG_DEMUXER			*md;
};

typedef FskErr (*mpegChunkWalker)(MPEGReader mpeg, FskInt64 offset, FskInt64 size);

typedef struct 
{
	UInt32				chunkType;
	UInt32				listType;
	mpegChunkWalker		walker;
} mpegChunkWalkersRecord, *mpegChunkWalkers;

	
//static int CHECK_ERR( int err )
//{
//	return 0;
//}

static void find_pic_start_code(unsigned char *data, int size, int *dc_size);

FskErr doRead(MPEGReader state, FskInt64 offset, UInt32 size, void *bufferIn)
{
	FskErr err = kFskErrNone;
	unsigned char *buffer = bufferIn;
	
	//dlog( "into doRead(): offset: %d, size : %d, ", (int)offset, (int)size ); 
	while (0 != size) 
	{
		unsigned char *readBuffer;
		UInt32 bytesRead = 0;

		if (state->dontSeekIfExpensive)
			state->spooler->flags |= kFskMediaSpoolerDontSeekIfExpensive;

		err = FskMediaSpoolerRead(state->spooler, offset, size, &readBuffer, &bytesRead);
		if (state->dontSeekIfExpensive)
			state->spooler->flags &= ~kFskMediaSpoolerDontSeekIfExpensive;
		BAIL_IF_ERR(err);

		FskMemMove(buffer, readBuffer, bytesRead);

		offset += bytesRead;
		buffer += bytesRead;
		size -= bytesRead;
	}

bail:
	//dlog( "exit err: %d\n", (int)err );
	
	return err;
}


int check_avc_sync( int is_startcode, unsigned char *data, int data_size, int *frame_type )
{
	int err = 0;
	long nalu_type, ref_idc;

	err = check_nalu( is_startcode, data, data_size, &nalu_type, &ref_idc );
	BAIL_IF_ERR(err);

	if( nalu_type == NAL_UT_IDR_SLICE )
		*frame_type = kFskImageFrameTypeSync;
	else if( ref_idc != 0 )
		*frame_type = kFskImageFrameTypeDifference;
	else
		*frame_type = kFskImageFrameTypeDroppable | kFskImageFrameTypeDifference;

bail:
	return err;
}

#if 1

int get_ts_audio_bps( MPEGReader mpeg, int *bps )
{
	MPEG_DEMUXER	*md = mpeg->md;
	TIME_64			min_pre, min_dec;
	TIME_64			max_pre, max_dec;
	TIME_64			dur, time_init;
	int				is_video = 0, is_audio = 0;
	FskInt64		offset;
	int				err = 0;
	int				bs_offset, bs_size;
	int				audio_size = 0;
	
	dlog( "\n");
	dlog( "into get_ts_adts_bps()\n"); 
	
	INVALIDATE_TIME( &min_pre );
	INVALIDATE_TIME( &min_dec );
	INVALIDATE_TIME( &max_pre );
	INVALIDATE_TIME( &max_dec );
	INVALIDATE_TIME( &dur );
	INVALIDATE_TIME( &time_init );
	
	for( offset = 0; offset < mpeg->total_size_fsk64; offset += TS_LENGTH )
	{
		err = doRead(mpeg, offset+mpeg->valid_offset, TS_LENGTH, mpeg->mpeg_bytes);
		BAIL_IF_ERR(err);
		
		err = md_ts_get_sample_time( md, mpeg->mpeg_bytes, &is_video, &is_audio, &min_pre, &min_dec, &bs_offset, &bs_size );
		BAIL_IF_ERR(err);
		
		//if( is_audio )
		//	audio_size += bs_size;
		
		if( is_audio && ( IS_VALID_TIME(&min_pre) || IS_VALID_TIME(&min_dec)))
		{
			audio_size = bs_size;
			break;
		}
	}
	
	for( ; offset < mpeg->total_size_fsk64; offset += TS_LENGTH )
	{
		int	this_is_video;
		
		err = doRead(mpeg, offset+mpeg->valid_offset, TS_LENGTH, mpeg->mpeg_bytes);
		BAIL_IF_ERR(err);
		
		err = md_ts_get_sample_time( md, mpeg->mpeg_bytes, &this_is_video, &is_audio, &max_pre, &max_dec, &bs_offset, &bs_size );
		BAIL_IF_ERR(err);

		if( is_audio )
			audio_size += bs_size;
		
		if( is_audio && ( audio_size >= 50000 ) && ( IS_VALID_TIME( &max_dec) || IS_VALID_TIME( &max_pre)))
			break;
	}
	
	dlog("\n min_dec=(%d, %d), min_pre=(%d, %d), max_dec=(%d, %d), max_pre=(%d, %d)\n", min_dec.hi, min_dec.lo, min_pre.hi, min_pre.lo, max_dec.hi, max_dec.lo, max_pre.hi, max_pre.lo);
	
bail:
	if( IS_VALID_TIME(&min_dec) )	//prefer dts
		time_init	= min_dec;
	else if( IS_VALID_TIME(&min_pre) )
		time_init	= min_pre;
	
	if( IS_VALID_TIME(&min_dec) && IS_VALID_TIME(&max_dec) )	//prefer dts
	{	
		DIFF_TIME_64_ABS(&dur, &max_dec, &min_dec);
	}
	else if( IS_VALID_TIME(&min_pre) && IS_VALID_TIME(&max_pre) )
	{
		DIFF_TIME_64_ABS(&dur, &max_pre, &min_pre);
	}
	else
	{
		TIME_64 *min = IS_VALID_TIME(&min_dec) ? &min_dec : &  min_pre;
		TIME_64 *max = IS_VALID_TIME(&max_dec) ? &max_dec : &  max_pre;
		if( IS_VALID_TIME(min) && IS_VALID_TIME(max) )
			DIFF_TIME_64_ABS(&dur, max, min);
	}
	
	if( IS_VALID_TIME( &dur) )
	{
		float audio_dur	= (float)GET_TIME_64(&dur);
		*bps = (int)(8*(float)audio_size*MPEG_VIDEO_DEFAULT_TIME_SCALE/audio_dur);
		dlog( " \naudio duration = %f, audio_size = %d\n", audio_dur, audio_size );
	}
	else
	{
		dlog("\naudio duration is not valid!!\n");
		*bps = 0;
	}
	
	dlog( "exit get_ts_adts_bps(), bps: %d\n", (int)*bps );
	
	return err;
}

#define PS_HEADER_LENGTH 2*TS_LENGTH
int get_ps_audio_bps( MPEGReader mpeg, int *bps )
{
	MPEG_DEMUXER		*md	= mpeg->md;
	TIME_64				min_pre, min_dec;
	TIME_64				max_pre, max_dec;
	TIME_64				dur, time_init;
	int					progress = 0;
	int					err = 0;
	int					audio_size = 0;
	
	dlog( "\n");
	dlog( "into get_ps_audio_bps()\n"); 
	
	INVALIDATE_TIME( &min_pre );
	INVALIDATE_TIME( &min_dec );
	INVALIDATE_TIME( &max_pre );
	INVALIDATE_TIME( &max_dec );
	INVALIDATE_TIME( &dur );
	INVALIDATE_TIME( &time_init );
	
	while(progress < mpeg->total_size_fsk64 )
	{
		PES_HEADER	pesh_x;
		int			pes_offset = 0;
		int			pes_size   = 0;
		int			is_audio;
		
		err = doRead(mpeg, progress+mpeg->valid_offset, PS_HEADER_LENGTH, mpeg->mpeg_bytes);
		BAIL_IF_ERR(err);
		
		err = md_ps_parse_header_x( md, mpeg->mpeg_bytes, &is_audio, &pesh_x, &pes_offset, &pes_size );
		BAIL_IF_ERR(err);
		
		if ((pes_offset + pes_size) == 0)
			progress += 1;
		else
			progress += pes_offset + pes_size;
		
		if( !pesh_x.is_valid )	//if it's not something we can handle, go to next 
			continue;

		if (is_audio)
		{
			audio_size += pes_size;
			//get valid smaller dts
			if ( IS_VALID_TIME(&pesh_x.decoding_time_stamp) && !IS_VALID_TIME(&min_dec) )
				min_dec = pesh_x.decoding_time_stamp;
		
			//get valid smaller pts
			if ( IS_VALID_TIME(&pesh_x.presentation_time_stamp)	&& !IS_VALID_TIME(&min_pre) )
				min_pre = pesh_x.presentation_time_stamp;
		
			//get valid bigger dts
			if ( IS_VALID_TIME(&pesh_x.decoding_time_stamp) && (!IS_VALID_TIME(&max_dec)||BIGGER_TIME(&pesh_x.decoding_time_stamp, &max_dec)) )
				max_dec = pesh_x.decoding_time_stamp;
		
			//get valid bigger pts
			if ( IS_VALID_TIME(&pesh_x.presentation_time_stamp) && (!IS_VALID_TIME(&max_pre)||BIGGER_TIME(&pesh_x.presentation_time_stamp, &max_pre)) )
				max_pre = pesh_x.presentation_time_stamp;
		}
		
		if (audio_size >= 50000)
			break;
	}

	dlog("\n min_dec=(%d, %d), min_pre=(%d, %d), max_dec=(%d, %d), max_pre=(%d, %d)\n", min_dec.hi, min_dec.lo, min_pre.hi, min_pre.lo, max_dec.hi, max_dec.lo, max_pre.hi, max_pre.lo);
	
bail:
	
	if( IS_VALID_TIME(&min_dec) )	//prefer dts
		time_init	= min_dec;
	else if( IS_VALID_TIME(&min_pre) )
		time_init	= min_pre;
	
	if( IS_VALID_TIME(&min_dec) && IS_VALID_TIME(&max_dec) )	//prefer dts
	{	
		DIFF_TIME_64_ABS(&dur, &max_dec, &min_dec);
	}
	else if( IS_VALID_TIME(&min_pre) && IS_VALID_TIME(&max_pre) )
	{	
		DIFF_TIME_64_ABS(&dur, &max_pre, &min_pre);
	}
	else
	{
		TIME_64 *min = IS_VALID_TIME(&min_dec) ? &min_dec : &  min_dec;
		TIME_64 *max = IS_VALID_TIME(&max_dec) ? &max_pre : &  max_pre;
		if( IS_VALID_TIME(min) && IS_VALID_TIME(max) )
			DIFF_TIME_64_ABS(&dur, max, min);
	}
	
	if( IS_VALID_TIME( &dur) )
	{
		float audio_dur	= (float)GET_TIME_64(&dur);
		*bps = (int)(8*(float)audio_size*MPEG_VIDEO_DEFAULT_TIME_SCALE/audio_dur);
		dlog( " \naudio duration = %f, audio_size = %d\n", audio_dur, audio_size );
	}
	else
	{
		dlog("\naudio duration is not valid!!\n");
		*bps = 0;
	}
	
	dlog( "exiting get_ps_audio_bps()\n"); 
	
	return err;
}


int get_audio_bps( MPEGReader mpeg, int *bps )
{
	if( mpeg->is_ts)
		return get_ts_audio_bps( mpeg, bps );
	else		   
		return get_ps_audio_bps( mpeg, bps );
}


#endif

int get_ts_duration( MPEGReader mpeg, FskInt64 *dur_out, FskInt64 *time_init_out )
{
	MPEG_DEMUXER	*md = mpeg->md;
	TIME_64			min_pre, min_dec;
	TIME_64			max_pre, max_dec;
	TIME_64			dur, time_init;
	int				is_video = 0, is_audio = 0;
	FskInt64		offset;
	int				err = 0;
	int				bs_offset, bs_size;

	dlog( "\n");
	dlog( "into get_ts_duration()\n"); 

	*dur_out		= -1;
	*time_init_out	= -1;

	INVALIDATE_TIME( &min_pre );
	INVALIDATE_TIME( &min_dec );
	INVALIDATE_TIME( &max_pre );
	INVALIDATE_TIME( &max_dec );
	INVALIDATE_TIME( &dur );
	INVALIDATE_TIME( &time_init );

	for( offset = 0; offset < mpeg->total_size_fsk64; offset += TS_LENGTH )
	{
		err = doRead(mpeg, offset+mpeg->valid_offset, TS_LENGTH, mpeg->mpeg_bytes);
		BAIL_IF_ERR(err);
		
		err = md_ts_get_sample_time( md, mpeg->mpeg_bytes, &is_video, &is_audio, &min_pre, &min_dec, &bs_offset, &bs_size );
		BAIL_IF_ERR(err);

		if (is_video) {
            if( IS_VALID_TIME(&min_pre) || IS_VALID_TIME(&min_dec)  )
                break;
		}
	}

	for( offset = mpeg->total_size_fsk64 - TS_LENGTH; offset >= 0; offset -= TS_LENGTH )
	{
		int	this_is_video;
				
		err = doRead(mpeg, offset+mpeg->valid_offset, TS_LENGTH, mpeg->mpeg_bytes);
		BAIL_IF_ERR(err);
		
		err = md_ts_get_sample_time( md, mpeg->mpeg_bytes, &this_is_video, &is_audio, &max_pre, &max_dec, &bs_offset, &bs_size );
		BAIL_IF_ERR(err);
		
		if( this_is_video == is_video && ( IS_VALID_TIME( &max_dec) || IS_VALID_TIME( &max_pre) ) )
			break;
	}

bail:
	if( IS_VALID_TIME(&min_dec) )	//prefer dts
		time_init	= min_dec;
	else if( IS_VALID_TIME(&min_pre) )
		time_init	= min_pre;

	if( IS_VALID_TIME(&min_dec) && IS_VALID_TIME(&max_dec) )	//prefer dts
	{	
		DIFF_TIME_64_ABS(&dur, &max_dec, &min_dec);
	}
	else if( IS_VALID_TIME(&min_pre) && IS_VALID_TIME(&max_pre) )
	{
		DIFF_TIME_64_ABS(&dur, &max_pre, &min_pre);
	}
	else
	{
		TIME_64 *min = IS_VALID_TIME(&min_dec) ? &min_dec : &  min_pre;
		TIME_64 *max = IS_VALID_TIME(&max_dec) ? &max_dec : &  max_pre;
		if( IS_VALID_TIME(min) && IS_VALID_TIME(max) )
			DIFF_TIME_64_ABS(&dur, max, min);
	}

	if( IS_VALID_TIME( &dur) )
		*dur_out	= (FskInt64)GET_TIME_64(&dur);

	if( IS_VALID_TIME( &time_init) )
		*time_init_out = (FskInt64)GET_TIME_64(&time_init);
	
	dlog( "exit get_ts_duration(), dur: %d, time_init: %d\n", *dur_out, *time_init_out );

	return err;
}

int get_ps_duration( MPEGReader mpeg, FskInt64 *dur_out, FskInt64 *time_init_out )
{
	MPEG_DEMUXER		*md	= mpeg->md;
	TIME_64				min_pre, min_dec;
	TIME_64				max_pre, max_dec;
	TIME_64				dur, time_init;
	int					progress = 0, progress_end = 0;
	int					err = 0, is_video = 0, check_max_time = 1;
	
	dlog( "\n");
	dlog( "into get_ps_duration()\n"); 
	
	*dur_out		= -1;
	*time_init_out	= -1;
	
	INVALIDATE_TIME( &min_pre );
	INVALIDATE_TIME( &min_dec );
	INVALIDATE_TIME( &max_pre );
	INVALIDATE_TIME( &max_dec );
	INVALIDATE_TIME( &dur );
	INVALIDATE_TIME( &time_init );
	
	//min
	while(progress < mpeg->total_size_fsk64 )
	{
		PES_HEADER	pesh_x;
		int			pes_offset = 0;
		int			pse_size   = 0;
		int			is_audio;
		
		is_video = 0;
		
		err = doRead(mpeg, progress+mpeg->valid_offset, PS_HEADER_LENGTH, mpeg->mpeg_bytes);
		BAIL_IF_ERR(err);
		
		err = md_ps_parse_header_x( md, mpeg->mpeg_bytes, &is_audio, &pesh_x, &pes_offset, &pse_size );
		BAIL_IF_ERR(err);
        
        if (IS_VIDEO_STREAM(pesh_x.stream_id))
            is_video = 1;
		
		if ((pes_offset + pse_size) == 0)
			progress += 1;
		else
			progress += pes_offset + pse_size;
		
		if( !pesh_x.is_valid )	//if it's not something we can handle, go to next 
			continue;
		
        if (is_video) {
            if	//get valid smaller dts
			( 
				IS_VALID_TIME(&pesh_x.decoding_time_stamp)					&&
				(
					!IS_VALID_TIME(&min_dec)								||
					BIGGER_TIME( &min_dec, &pesh_x.decoding_time_stamp )
				)
			)
			min_dec = pesh_x.decoding_time_stamp;
			
            if	//get valid smaller pts
			( 
				IS_VALID_TIME(&pesh_x.presentation_time_stamp)				&&
				(
					!IS_VALID_TIME(&min_pre)								||
					BIGGER_TIME( &min_pre, &pesh_x.presentation_time_stamp )
				)
			)
			min_pre = pesh_x.presentation_time_stamp;
        } //is_video
		
		if( md->initialized && ( IS_VALID_TIME( &min_dec) || IS_VALID_TIME( &min_pre) ) )
			break;
	}
	
	//max
#define MIN_SIZE_FOR_BIGGEST 16*1000	//minimum data size for max time stamp.
	if (mpeg->total_size_fsk64 > progress+MIN_SIZE_FOR_BIGGEST)
		progress = mpeg->total_size_fsk64 - MIN_SIZE_FOR_BIGGEST;
	
	progress_end = mpeg->total_size_fsk64;
	
check_max:	
	while(progress < progress_end )
	{
		PES_HEADER	pesh_x;
		int			pes_offset = 0;
		int			pse_size   = 0;
		int			is_audio;
		
		is_video = 0;
		
		if (progress + PS_HEADER_LENGTH > progress_end) //if file could not end in one whole packet.
			break; //goto check next page!!!

		err = doRead(mpeg, progress+mpeg->valid_offset, PS_HEADER_LENGTH, mpeg->mpeg_bytes);
		BAIL_IF_ERR(err);
		
		err = md_ps_parse_header_x( md, mpeg->mpeg_bytes, &is_audio, &pesh_x, &pes_offset, &pse_size );
		BAIL_IF_ERR(err);
		
		if (IS_VIDEO_STREAM(pesh_x.stream_id))
			is_video = 1;
		
		if ((pes_offset + pse_size) == 0)
			progress += 1;
		else
			progress += pes_offset + pse_size;
		
		if( !pesh_x.is_valid )	//if it's not something we can handle, go to next 
			continue;
		
		if (is_video) {
			if	//get valid bigger dts
			( 
				IS_VALID_TIME(&pesh_x.decoding_time_stamp)						&&
				(
				    !IS_VALID_TIME(&max_dec)								||
					BIGGER_TIME( &pesh_x.decoding_time_stamp, &max_dec )
				)
			)
			max_dec = pesh_x.decoding_time_stamp;
			
			if	//get valid bigger pts
			( 
				IS_VALID_TIME(&pesh_x.presentation_time_stamp)					&&
				( 
					!IS_VALID_TIME(&max_pre)								|| 
					BIGGER_TIME( &pesh_x.presentation_time_stamp, &max_pre ) 
				)
			)
			max_pre = pesh_x.presentation_time_stamp;
		} //is_video
	}
	
	check_max_time += 1;
	if ( (!IS_VALID_TIME( &max_dec)) && (!IS_VALID_TIME( &max_pre)) )
	{	
		if (mpeg->total_size_fsk64 <= check_max_time*MIN_SIZE_FOR_BIGGEST)
			goto bail;
		else {
			progress = mpeg->total_size_fsk64 - check_max_time*MIN_SIZE_FOR_BIGGEST;
			progress_end = progress + MIN_SIZE_FOR_BIGGEST;
			goto check_max;
		}
	}
	
bail:
	
    dlog( "max_dec, max_pre, min_dec, min_pre : %x, %x, %x, %x\n\n", max_dec.lo, max_pre.lo, min_dec.lo, min_pre.lo );
	if( IS_VALID_TIME(&min_dec) )	//prefer dts
		time_init	= min_dec;
	else if( IS_VALID_TIME(&min_pre) )
		time_init	= min_pre;
	
	if( IS_VALID_TIME(&min_dec) && IS_VALID_TIME(&max_dec) )	//prefer dts
	{	
		DIFF_TIME_64_ABS(&dur, &max_dec, &min_dec);
	}
	else if( IS_VALID_TIME(&min_pre) && IS_VALID_TIME(&max_pre) )
	{	
		DIFF_TIME_64_ABS(&dur, &max_pre, &min_pre);
	}
	else
	{
		TIME_64 *min = IS_VALID_TIME(&min_dec) ? &min_dec : &  min_dec;
		TIME_64 *max = IS_VALID_TIME(&max_dec) ? &max_pre : &  max_pre;
		if( IS_VALID_TIME(min) && IS_VALID_TIME(max) )
			DIFF_TIME_64_ABS(&dur, max, min);
	}
	
	if( IS_VALID_TIME( &dur) )
		*dur_out	= (FskInt64)GET_TIME_64(&dur);
	
	if( IS_VALID_TIME( &time_init) )
		*time_init_out = (FskInt64)GET_TIME_64(&time_init);
	
	return err;
}


int mp_get_duration( MPEGReader mpeg, FskInt64 *dur, FskInt64 *time_init )
{
	if( mpeg->is_ts)
		return get_ts_duration( mpeg, dur, time_init );
	else		   
		return get_ps_duration( mpeg, dur, time_init );
}


int init_headers( MPEGReader mpeg )
{
	int offset;
	int err = 0;

	dlog( "into init_headers\n");
	
	if( mpeg->is_ts )
	{
		for( offset = mpeg->progress; offset < mpeg->total_size_fsk64; offset += TS_LENGTH )
		{
			//dlog( "going through all TS chunks to initialize...\n");
			if( mpeg->md->initialized )
			{
				dlog( "successfully initialized, break!\n");
				break;
			}
			err = doRead(mpeg, offset+mpeg->valid_offset, TS_LENGTH, mpeg->mpeg_bytes);
			if( err == kFskErrNeedMoreTime ) mpeg->progress = offset;	//next time start from here
			BAIL_IF_ERR(err);

			err = md_ts_parse_header( mpeg->md, mpeg->mpeg_bytes );
			if( err == kFskErrNeedMoreTime ) mpeg->progress = offset;	//next time start from here
			if( kFskErrNeedMoreTime == err )
				err = kFskErrNone;
			BAIL_IF_ERR(err);
		}
	}
	else
	{
 		while( mpeg->progress < mpeg->total_size_fsk64 )
		{
			PES_HEADER		pesh_x_no_use;
			int				pes_offset;
			int				pes_size;
			int				is_audio;

			if( mpeg->md->initialized )
				break;

			err = doRead(mpeg, mpeg->progress+mpeg->valid_offset, PS_HEADER_LENGTH, mpeg->mpeg_bytes);
			//if( err == kFskErrNeedMoreTime );	//next time start from here
			BAIL_IF_ERR(err);

			err = md_ps_parse_header_x(  mpeg->md, mpeg->mpeg_bytes, &is_audio, &pesh_x_no_use, &pes_offset, &pes_size );
			BAIL_IF_ERR(err);

			if ((pes_offset + pes_size) == 0)
				mpeg->progress += 1;
			else
				mpeg->progress += pes_offset + pes_size;
		}
	}

bail:
	return err;
}


int ts_get_frame_dur( MPEGReader mpeg, int *frame_dur )
{
	MPEG_DEMUXER	*md = mpeg->md;
	TIME_64			early_time_dec;
	TIME_64			later_time_dec;
	//TIME_64		early_time_pre;	//only need to calculate dts
	//TIME_64		later_time_pre;
	int				is_video = 0, is_audio = 0;
	int				offset;
	int				bs_offset, bs_size;
	int				video_frame_count = 0;
	int				err = 0;

	INVALIDATE_TIME( &early_time_dec );
	INVALIDATE_TIME( &later_time_dec );
	//INVALIDATE_TIME( &early_time_pts );
	//INVALIDATE_TIME( &later_time_pts );

	*frame_dur = 0;

	for( offset = 0; offset < mpeg->total_size_fsk64; offset += TS_LENGTH )
	{
		TIME_64	this_time_pre, this_time_dec;

		err = doRead(mpeg, offset+mpeg->valid_offset, TS_LENGTH, mpeg->mpeg_bytes);
		BAIL_IF_ERR(err);
		
		err = md_ts_get_sample_time( md, mpeg->mpeg_bytes, &is_video, &is_audio, &this_time_pre,  &this_time_dec, &bs_offset, &bs_size );
		BAIL_IF_ERR(err);

		if( IS_VALID_TIME(&this_time_dec) && is_video )
		{
			video_frame_count++;

			if( video_frame_count == 1 )
				early_time_dec = this_time_dec;

			later_time_dec = this_time_dec;

			if( video_frame_count >= MAX_COUNT_FOR_FRAME_RATE )
				break;
		}
	}

	if( video_frame_count > 1 && IS_VALID_TIME( &later_time_dec ) && IS_VALID_TIME( &early_time_dec ) )
	{
		TIME_64			dur_64;   
		unsigned int	dur;
			
		DIFF_TIME_64_ABS( &dur_64, &later_time_dec, &early_time_dec );
		dur = GET_TIME_64( &dur_64 );
		*frame_dur = dur / (video_frame_count-1);// / MPEG_VIDEO_DEFAULT_TIME_SCALE;
	}

bail:
	return err;
}


int ts_seek_packet_by_time( MPEGReader mpeg, unsigned int time, int *seek_offset )
{
	MPEG_DEMUXER	*md = mpeg->md;

	TIME_64			frame_time_pre;
	TIME_64			frame_time_dec;
	int				offset;
	int				last_sync_candidate_idx			= -1;
	int				sync_candidate_idx				= -1;
	int				last_none_sync_candidate_idx	= -1;
	int				none_sync_candidate_idx			= -1;
	int				none_sync_found = 0;
	int				err = 0;
	//int				found = 0;
	int				bs_offset, bs_size;

	INVALIDATE_TIME( &frame_time_pre );
	INVALIDATE_TIME( &frame_time_dec );

	for( offset = 0; offset < mpeg->total_size_fsk64; offset += TS_LENGTH )
	{
		int	is_video = 0, is_audio = 0;

		err = doRead(mpeg, offset+mpeg->valid_offset, TS_LENGTH, mpeg->mpeg_bytes);
		BAIL_IF_ERR(err);
		
		err = md_ts_get_sample_time( md, mpeg->mpeg_bytes, &is_video, &is_audio, &frame_time_pre,  &frame_time_dec, &bs_offset, &bs_size );
		BAIL_IF_ERR(err);
		
		if( IS_VALID_TIME(&frame_time_dec) )
		{
			unsigned int this_time = GET_TIME_64( &frame_time_dec) - mpeg->time_init_fsk64;
			int is_video_sync = 0;
			unsigned char *bs_bytes = mpeg->mpeg_bytes + bs_offset;

			if (is_video ) 
			{
				int sample_flag;

				err = check_avc_sync( 1, bs_bytes, bs_size, &sample_flag );
				BAIL_IF_ERR(err);
				
				is_video_sync = sample_flag == kFskImageFrameTypeSync;
			}

			if( is_video_sync )
			{
				last_sync_candidate_idx	= sync_candidate_idx;
				sync_candidate_idx = offset;
				if( this_time >=  time )
					break;
			}
			else if( !none_sync_found )
			{
				last_none_sync_candidate_idx	= none_sync_candidate_idx;
				none_sync_candidate_idx = offset;
				if( this_time >=  time )
					none_sync_found = 1;
			}
		}
	}
	
	sync_candidate_idx		= last_sync_candidate_idx		!= -1 ? last_sync_candidate_idx			: sync_candidate_idx;
	none_sync_candidate_idx = last_none_sync_candidate_idx	!= -1 ? last_none_sync_candidate_idx	: none_sync_candidate_idx;

	{
		int candidate_idx = sync_candidate_idx;

		if( none_sync_candidate_idx != -1 && none_sync_candidate_idx < sync_candidate_idx )
			candidate_idx = none_sync_candidate_idx;

		*seek_offset = candidate_idx;
	}

	err = md_reset_buffer( mpeg->md );

bail:

	return err;
}



int ps_seek_packet_by_time( MPEGReader mpeg, unsigned int time, int *seek_offset )
{
	MPEG_DEMUXER	*md = mpeg->md;
	int				last_sync_candidate_idx			= -1;
	int				sync_candidate_idx				= -1;
	int				last_none_sync_candidate_idx	= -1;
	int				none_sync_candidate_idx			= -1;
	int				none_sync_found = 0;
	int				err = 0;
	//int				found = 0;
	int				bs_offset, bs_size;
	int				progress = 0;

	while( progress <= mpeg->total_size_fsk64 )
	{
		//TIME_64	frame_time_pre;
		TIME_64		frame_time_dec;
		int			is_video = 0;
		PES_HEADER	pesh_x;
		int			is_audio;

		//INVALIDATE_TIME( &frame_time_pts );
		INVALIDATE_TIME( &frame_time_dec );

		err = doRead( mpeg, progress+mpeg->valid_offset, PS_HEADER_LENGTH, mpeg->mpeg_bytes);
		BAIL_IF_ERR(err);
		
		err = md_ps_parse_header_x( md, mpeg->mpeg_bytes, &is_audio, &pesh_x, &bs_offset, &bs_size );
		BAIL_IF_ERR(err);

		if( IS_VIDEO_STREAM( pesh_x.stream_id ))
		{
			//my_assert( (payload_start[0]==0x00 && payload_start[1]==0x00 && payload_start[2]==0x00 && payload_start[3]==0x01 && 
			//			payload_start[4]==0x09  ));
			is_video = 1;
			if( IS_VALID_TIME(&pesh_x.decoding_time_stamp) )
				frame_time_dec = pesh_x.decoding_time_stamp;
		}
		else if( IS_AUDIO_STREAM( pesh_x.stream_id ) )
		{
			is_video = 0;
			if( IS_VALID_TIME(&pesh_x.decoding_time_stamp) )
				frame_time_dec = pesh_x.decoding_time_stamp;
		}

		if( IS_VALID_TIME(&frame_time_dec) )
		{
			unsigned int	this_time = GET_TIME_64(&frame_time_dec) - mpeg->time_init_fsk64;
			int				is_video_sync = 0;
			unsigned char	*bs_bytes = mpeg->mpeg_bytes + bs_offset;

			if (is_video ) 
			{
				int sample_flag;
				
				if( md->video_format == TRACK_H264   || md->video_format == TRACK_H264_x )
				{
					err = check_avc_sync( 1, bs_bytes, bs_size, &sample_flag );
					BAIL_IF_ERR(err);
					
					is_video_sync = sample_flag == kFskImageFrameTypeSync;
				}
				else// take care of non-avc case later
					is_video_sync = 1;//kFskImageFrameTypeSync;
			}

			if( is_video_sync )
			{
				last_sync_candidate_idx	= sync_candidate_idx;
				sync_candidate_idx = progress;
				if( this_time >=  time )
					break;
			}
			else if( !none_sync_found )
			{
				last_none_sync_candidate_idx	= none_sync_candidate_idx;
				none_sync_candidate_idx = progress;
				if( this_time >=  time )
					none_sync_found = 1;
			}
		}
		if ((bs_offset + bs_size) == 0)
			progress += 1;
		else
			progress += bs_offset + bs_size;
	}
	
	sync_candidate_idx		= last_sync_candidate_idx		!= -1 ? last_sync_candidate_idx			: sync_candidate_idx;
	none_sync_candidate_idx = last_none_sync_candidate_idx	!= -1 ? last_none_sync_candidate_idx	: none_sync_candidate_idx;

	{
		int candidate_idx = sync_candidate_idx;

		if( none_sync_candidate_idx != -1 && none_sync_candidate_idx < sync_candidate_idx )
			candidate_idx = none_sync_candidate_idx;

		*seek_offset = candidate_idx;
	}

	err = md_reset_buffer( mpeg->md );

bail:
	return err;
}


FskErr mpegInstantiate(MPEGReader mpeg)
{
	FskErr	err;
	
	dlog("into mpegInstantiate()\n");
	if( mpeg->is_ts == -1 )
	{//first read to validate stream
		err = doRead(mpeg, 0, TS_LENGTH*3, mpeg->mpeg_bytes);
		if( err == kFskErrNeedMoreTime )
		BAIL_IF_ERR(err);

		err = validate_mpeg( mpeg->mpeg_bytes, TS_LENGTH*3, &mpeg->is_ts, &mpeg->valid_offset );
		BAIL_IF_ERR(err);

		err = mpeg->spooler->doGetSize(mpeg->spooler, &mpeg->total_size_fsk64);
		BAIL_IF_ERR(err);
		
		//mpeg->video_total_size_fsk64 = 0;
		mpeg->audio_total_size = 0;
		
		dlog("got size: %d\n", (int)mpeg->total_size_fsk64);
		
		mpeg->total_size_fsk64 -= mpeg->valid_offset;
		mpeg->total_size_fsk64 -= mpeg->total_size_fsk64 % TS_LENGTH;
		if( mpeg->total_size_fsk64 <= 0 ) {
			mpeg->total_size_fsk64 = 0x7fffffffffffffffLL;		// never ending...
		}

		if( mpeg->md == NULL )
		{
			if( mpeg->is_ts )
				err = md_ts_new( &mpeg->md );
			else
				err = md_ps_new( &mpeg->md );
			BAIL_IF_ERR(err);
		}
	}

	//if( mpeg->time_init_fsk64 == -1 )	//check if it's already tried before
	if( mpeg->duration == -1 )	//check if it's already tried before
	{
		err = mp_get_duration(  mpeg, &mpeg->duration, &mpeg->time_init_fsk64 );
		
		dlog("\n init duration = %d, time_init = %d \n", (int)mpeg->duration, (int)mpeg->time_init_fsk64);
		
		if (err) 
		{
			if (kFskErrNeedMoreTime == err)
				err = kFskErrNone;
			
			BAIL_IF_ERR(err);
		}
	}

	err = init_headers( mpeg );
	BAIL_IF_ERR(err);
		
	mpeg->seek_offset = 0;

	if( mpeg->md->video_format == TRACK_H264   || mpeg->md->video_format == TRACK_H264_x ||
	    mpeg->md->video_format == TRACK_MPEG1V || mpeg->md->video_format == TRACK_MPEG2V )
	{
		MPEGReaderTrack  track;
		
		dlog( "creating video track.\n");
		err = FskMemPtrNewClear(sizeof(MPEGReaderTrackRecord), &track);
		BAIL_IF_ERR(err);

		track->mpeg	= mpeg;
		track->reader_track.dispatch = &gMPEGReaderVideoTrack;
		track->reader_track.state	 = track;
		if( mpeg->md->video_format == TRACK_H264   || mpeg->md->video_format == TRACK_H264_x )
			track->format				=FskStrDoCopy("x-video-codec/avc");
		else
			track->format				=FskStrDoCopy("x-video-codec/mpg");

		track->mediaType			= kMPEGMediaTypeVideo;
		track->codecSpecificDataRef  = mpeg->md->video_codec_header_data;
		track->codecSpecificDataSize = mpeg->md->video_codec_header_data_size;
#if 0
		FskImageDecompressNew(&track->video.deco, 0, track->format, NULL);		// instance for determining frame type

		if (track->video.deco) 
		{
			FskMediaPropertyValueRecord value;
			unsigned char	*bs_bytes = track->codecSpecificDataRef;
			int				bs_size   = track->codecSpecificDataSize;
			FskImageDecompressSetData(track->video.deco, bs_bytes, bs_size);
			if (kFskErrNone == FskImageDecompressGetMetaData(track->video.deco, kFskImageDecompressMetaDataFrameType, 9, &value, NULL))
			{
				track->video.dimensions.width  = (value.value.integer>>16)&0xffff;
				track->video.dimensions.height = (value.value.integer>> 0)&0xffff;
			}
		}
		else
		{
			dlog( "no decoder found to get dimention, set default 320x240 so that the size doesn't go crazy\n");
			track->video.dimensions.width  = 320;
			track->video.dimensions.height = 240;
		}
#else
		
		if( mpeg->md->video_width != 0 && mpeg->md->video_height !=  0  )
		{
			track->video.dimensions.width  = mpeg->md->video_width;
			track->video.dimensions.height = mpeg->md->video_height;
			dlog( "got available dimension info: width: %d, height: %d\n", mpeg->md->video_width, mpeg->md->video_height );
		}
		else if( mpeg->md->video_format == TRACK_H264   || mpeg->md->video_format == TRACK_H264_x )
		{
			unsigned char	*bs_bytes = track->codecSpecificDataRef;
			int				bs_size   = track->codecSpecificDataSize;
			int width_src = 320, height_src = 240;
			SInt32 width_clip, height_clip;
			int profile = 0, level = 0;
			
			dlog(" calling parse_avc_header():\n");
			
			err = parse_avc_header( bs_bytes+4, bs_size, (int *)&width_clip, (int *)&height_clip, &profile, &level );
			BAIL_IF_ERR(err);

			width_src = (width_clip + 15) & ~15;
			height_src = (height_clip + 15) & ~15;
			//else
			//{
			//	dlog(" calling kinoma_avc_dec_parse_header_info() failed, set default width/height: 320/240\n");
			//	width_src  = 320;
			//	height_src = 240;
			//}
			
			track->video.dimensions.width  = width_src;
			track->video.dimensions.height = height_src;
			track->profile = profile;
			track->level   = level;
			dlog( "width: %d, height: %d, profile: %d, level: %d\n", width_src, height_src, profile, level );
		}
		else
		{
			dlog( "no available dimension info, set default 320x240 so that the size doesn't go crazy\n");
			track->video.dimensions.width  = 320;
			track->video.dimensions.height = 240;
		}
#endif
		
		FskListAppend(&mpeg->mpeg_tracks, track);
	}

	if( mpeg->md->audio_format == TRACK_AAC   || mpeg->md->audio_format == TRACK_AAC_x   ||
	    mpeg->md->audio_format == TRACK_MPEGA || mpeg->md->audio_format == TRACK_MPEGA_x ||
	    mpeg->md->audio_format == TRACK_AC3 )
	{
		MPEGReaderTrack  track;

		dlog( "creating AAC or MP3 or AC3 track.\n");

		err = FskMemPtrNewClear(sizeof(MPEGReaderTrackRecord), &track);
		BAIL_IF_ERR(err);

		track->mpeg	= mpeg;
		track->reader_track.dispatch = &gMPEGReaderAudioTrack;
		track->reader_track.state	 = track;
		
		if( mpeg->md->audio_format == TRACK_AAC   || mpeg->md->audio_format == TRACK_AAC_x )
			track->format = FskStrDoCopy("x-audio-codec/aac");
		else if( mpeg->md->audio_format == TRACK_MPEGA   || mpeg->md->audio_format == TRACK_MPEGA_x )
		{
			if( mpeg->md->audio_layer == 3 )
				track->format = FskStrDoCopy("x-audio-codec/mp3");
			else if( mpeg->md->audio_layer == 2 )
				track->format = FskStrDoCopy("x-audio-codec/mpeg-audio-layer2");
			else if( mpeg->md->audio_layer == 1 )
				track->format = FskStrDoCopy("x-audio-codec/mpeg-audio-layer1");
		}
		else
			track->format = FskStrDoCopy("x-audio-codec/ac3");

		track->mediaType = kMPEGMediaTypeAudio;
		track->bps = mpeg->md->audio_bitrate;
		track->profile = mpeg->md->audio_profile;
		track->level   = mpeg->md->audio_level;

		if( track->bps == 0 && (mpeg->md->audio_format == TRACK_AAC   || mpeg->md->audio_format == TRACK_AAC_x)  )
			get_audio_bps( mpeg, &track->bps );
		
		track->audio.samplerate    = mpeg->md->audio_samplerate;
		track->audio.channel_total = mpeg->md->audio_channel_total;

		track->codecSpecificDataRef	= mpeg->md->audio_codec_header_data;
		track->codecSpecificDataSize = mpeg->md->audio_codec_header_data_size;

		FskListAppend(&mpeg->mpeg_tracks, track);
	}

	if( !mpeg->md->initialized )
	{
		if( mpeg->md->video_header_initialized || mpeg->md->audio_header_initialized )
		mpeg->md->initialized = 1;
	}
	
	(mpeg->reader->doSetState)(mpeg->reader, kFskMediaPlayerStateStopped);

bail:
	if( mpeg->md != NULL )
	{
		dlog("exit mpegInstantiate(): initialized: %d, is_ts: %d\n", (int)mpeg->md->initialized, (int)mpeg->is_ts );
	}
	return err;
}


FskErr mpegSpoolerCallback(void *clientRefCon, UInt32 operation, void *param)
{
	MPEGReader mpeg = clientRefCon;
	FskErr err = kFskErrNone;

	dlog("into mpegSpoolerCallback()\n");

	switch (operation) 
	{
		case kFskMediaSpoolerOperationGetHeaders:
			mpeg->spooler->flags |= kFskMediaSpoolerCantSeek;
			break;

		case kFskMediaSpoolerOperationDataReady:
			if (mpeg->reader->mediaState < kFskMediaPlayerStateStopped)
				err = mpegInstantiate(mpeg);
			break;

		default:
			return kFskErrUnimplemented;
	}

	return err;
}

#define SUPPORT_PS
int validate_mpeg( unsigned char *d, int size, int *is_ts, int *offset_out )
{
	int err = 0;
	
	dlog(" into validate_mpeg\n");
	
	err = ts_validate( d, size, is_ts, offset_out );
#ifdef SUPPORT_PS
	if( err )
		err = ps_validate( d, size, is_ts, offset_out );
#endif

	return err;
}


//
//
//demuxer interface
//
//

Boolean mpegReaderCanHandle(const char *mimeType)
{
	dlog("\n\n");
	dlog("into mpegReaderCanHandle(), mimeType: %s\n", mimeType);
	
	if (0 == FskStrCompareCaseInsensitive("video/mpeg", mimeType))
		return true;

	return false;
}


FskErr mpegReaderSniff(const unsigned char *data, UInt32 dataSize, FskHeaders *headers, const char *uri, char **mime)
{
	int err = kFskErrUnknownElement;
	int offset = 0;

	dlog("\n\n");
	dlog("into mpegReaderSniff(), dataSize: %d\n", (int)dataSize);

	if (dataSize >= 12) 
	{	
		int is_ts;

		dlog("data: %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x\n",
			data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],
			data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]);

		if (kFskErrNone == validate_mpeg((unsigned char *)data, dataSize, &is_ts, &offset ))
        {
            dlog("successfully got mime:video/mpeg\n");
            err = kFskErrNone;
            *mime = FskStrDoCopy("video/mpeg");
        }
	}

	dlog("out of mpegReaderSniff(), err: %d\n", (int)err );
	return err;
}

FskErr mpegReaderNew(FskMediaReader reader, void **readerState, const char *mimeType, const char *uri, FskMediaSpooler spooler)
{
	FskErr err;
	MPEGReader mpeg = NULL;

	dlog("\n\n");
	dlog("into mpegReaderNew()\n");

    BAIL_IF_NULL(spooler, err, kFskErrUnimplemented);
	
	err = FskMemPtrNewClear(sizeof(MPEGReaderRecord), &mpeg);
	BAIL_IF_ERR(err);

	*readerState = mpeg;			// must be set before anything that might issue a callback
	mpeg->spooler = spooler;
	mpeg->reader = reader;

	mpeg->spooler->onSpoolerCallback = mpegSpoolerCallback;
	mpeg->spooler->clientRefCon		 = mpeg;
	mpeg->spooler->flags |= kFskMediaSpoolerForwardOnly;
	mpeg->time_init_fsk64			 = -1;
	mpeg->duration					 = -1;
	mpeg->wait_for_video_sync_frame  = 1;
	mpeg->is_ts						 = -1;//don't know yet
	mpeg->last_B_cmptime			 = 0;
	mpeg->last_P_cmptime			 = 0;
	mpeg->dectime_interval			 = 0;
	mpeg->last_dectime				 = 0;

	mpeg->mpeg_bytes_size = TS_LENGTH*4;
	err = FskMemPtrNewClear( mpeg->mpeg_bytes_size, &mpeg->mpeg_bytes );
	BAIL_IF_ERR(err);

	if (spooler->doOpen) {
		err = (spooler->doOpen)(spooler, kFskFilePermissionReadOnly);
		BAIL_IF_ERR(err);

		mpeg->spoolerOpen = true;
	}

	(reader->doSetState)(reader, kFskMediaPlayerStateInstantiating);

	err = mpegInstantiate(mpeg);
	if (err) 
	{
		if (kFskErrNeedMoreTime == err)
			err = kFskErrNone;
		goto bail;
	}

bail:
	if ((kFskErrNone != err) && (NULL != mpeg)) 
	{
		mpegReaderDispose(reader, mpeg);
		mpeg = NULL;
	}

	*readerState = mpeg;

	return err;
}

FskErr mpegReaderDispose(FskMediaReader reader, void *readerState)
{
	MPEGReader mpeg = readerState;

	dlog("\n\n");
	dlog("into mpegReaderDispose()\n");

	if( mpeg != NULL )
	{
		while (mpeg->mpeg_tracks) 
		{
			MPEGReaderTrack track = mpeg->mpeg_tracks;
			mpeg->mpeg_tracks = track->next;
			if (kMPEGMediaTypeVideo == track->mediaType) 
			{
				if( track->video.deco != NULL )
				{
					FskImageDecompressDispose(track->video.deco);
					track->video.deco = 0;
				}
			}
			//else if (kMPEGMediaTypeAudio == track->mediaType)
			//	;//***FskMemPtrDispose(track->media.audio.mp3Data);
			
			FskMemPtrDispose(track->format);
			FskMemPtrDispose(track);
		}

		if( mpeg->md != NULL )
		{
			md_ts_dispose( mpeg->md );
			mpeg->md = NULL;
		}

		if (mpeg->spoolerOpen && mpeg->spooler->doClose)
			(mpeg->spooler->doClose)(mpeg->spooler);

		
		FskMemPtrDisposeAt( &mpeg->mpeg_bytes );

		FskMemPtrDispose(mpeg);
	}
	
	return kFskErrNone;
}

FskErr mpegReaderGetTrack(FskMediaReader reader, void *readerState, SInt32 index, FskMediaReaderTrack *track)
{
	MPEGReader		mpeg   = readerState;
	MPEGReaderTrack walker = mpeg->mpeg_tracks;

	dlog("\n\n");
	dlog("into mpegReaderGetTrack(), index: %d\n", (int)index);

	if( !mpeg->md->initialized )
		return -1;

	while ((index > 0) && walker) 
	{
		walker = walker->next;
		index -= 1;
	}

	if (walker) 
	{
		*track = &walker->reader_track;
		return kFskErrNone;
	}
	else
		return kFskErrNotFound;
}


//#define DUMP_TS	1
#ifdef DUMP_TS
FILE *ts_dump = NULL;
#endif

FskErr mpegReaderStart(FskMediaReader reader, void *readerState, double *startTime, double *endTime)
{
	MPEGReader	mpeg = readerState;
	FskErr		err = kFskErrNone;
	
	dlog("\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n\n");
	dlog("into mpegReaderStart()\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n\n");
	
	if( !mpeg->md->initialized )
		return -1;

	mpeg->wait_for_video_sync_frame = 1;

	if( mpeg->is_ts )
		err = ts_seek_packet_by_time( mpeg, (unsigned int)(startTime ? *startTime : 0), &mpeg->seek_offset );
	else
		err = ps_seek_packet_by_time( mpeg, (unsigned int)(startTime ? *startTime : 0), &mpeg->seek_offset );

	mpeg->video_frame_count	= 0;

	//IS_VALID_TIME(&mpeg->early_time_pts);
	//IS_VALID_TIME(&mpeg->later_time_pts);
	INVALIDATE_TIME(&mpeg->early_time_dec);
	INVALIDATE_TIME(&mpeg->later_time_dec);

	if( mpeg->seek_offset == -1 && err == kFskErrNone )
		err = kFskErrBadData;	//gone through all data and no sample found

#ifdef DUMP_TS
	if( err == kFskErrNone )
	{
		if( ts_dump == NULL )
			ts_dump = fopen("E:\\mpeg_demux_input_dump.ts","wb");	

		if( ts_dump != NULL )
		{
			unsigned char *tmp = NULL;

			tmp = malloc( mpeg->seek_offset );
			
			if( tmp != NULL )
			{
				while(1)
				{
					err = doRead( mpeg, mpeg->valid_offset, mpeg->seek_offset, tmp );
					if( err == kFskErrNone )
						break;
				}

				fwrite( tmp, mpeg->seek_offset, 1, ts_dump);
				fflush( ts_dump );

				free( tmp );
			}
		}
	}
#endif

	return err;
}

FskErr mpegReaderStop(FskMediaReader reader, void *readerState)
{
	return kFskErrNone;		//@@
}


FskErr mpegReaderExtract(FskMediaReader reader, void *readerState, FskMediaReaderTrack *trackOut, UInt32 *infoCountOut, FskMediaReaderSampleInfo *infoOut, unsigned char **data)
{
	MPEGReader mpeg = readerState;
	FskMediaReaderSampleInfo info;
	FskErr err = kFskErrNone;

	dlog("\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n\n");
	dlog("into mpegReaderExtract()\n\n");
	dlog("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n\n");

	*infoCountOut	= 0;
	*infoOut		= NULL;
	*trackOut		= NULL;
	*data			= NULL;

	if( mpeg->md == NULL || !mpeg->md->initialized ) //Fix Covertity 10716
		goto bail;

	if( mpeg->seek_offset < 0 )
		goto bail;

	if( mpeg->is_ts < 0 )
		goto bail;
	
	while(1)
	{
		int				is_video = 0;
		unsigned char	*bs_bytes = NULL;
		int				bs_size = 0;
		TIME_64			time_pre;
		TIME_64			time_dec;
		int				fsk_time;
		int				cmp_time;

		INVALIDATE_TIME( &time_pre );
		INVALIDATE_TIME( &time_dec );

		if( mpeg->is_ts )
		{
			err = doRead( mpeg, mpeg->seek_offset + mpeg->valid_offset, TS_LENGTH, mpeg->mpeg_bytes );
			BAIL_IF_ERR(err);
			
#ifdef DUMP_TS
			if( ts_dump != NULL )
			{
				fwrite( mpeg->mpeg_bytes, TS_LENGTH, 1, ts_dump);
				fflush( ts_dump );
			}	
#endif

			err = md_ts_process_sample( mpeg->md, 0, mpeg->mpeg_bytes, &is_video, &bs_bytes, &bs_size, &time_pre, &time_dec );
			dlog("passed 1 packet, is_video = %d, bs_size = %d\n", is_video, bs_size);
			BAIL_IF_ERR(err);

			if (is_video == 2) //check this packet again
				;
			else
				mpeg->seek_offset += TS_LENGTH;
		}
		else
		{
			PS_PACK_HEADER_x	pakh_x;
			PS_SYSTEM_HEADER_x	sysh_x;
			PES_HEADER			pesh_x;
			//unsigned char		*pes_bytes = NULL;
			int					pes_size = 0;
			int					pes_offset = 0;
			int				    sc_flag = 0, sc_size = 0;
			FRAME_BUFFER		*tt;
			
			err = doRead(mpeg, mpeg->seek_offset+mpeg->valid_offset, PS_HEADER_LENGTH, mpeg->mpeg_bytes);
			BAIL_IF_ERR(err);
			
			err = ps_scan_header_x( mpeg->mpeg_bytes, &pakh_x, &sysh_x, &pesh_x, &pes_offset, &pes_size );
			BAIL_IF_ERR(err);
			
			if (mpeg->seek_offset+mpeg->valid_offset+pes_offset+pes_size > mpeg->total_size_fsk64)
                BAIL(kFskErrEndOfFile);
			
			tt = get_fb_by_idx( mpeg->md->fb_ary, pesh_x.stream_id, pesh_x.stream_id );
			if( tt == NULL  ) 
				goto bail;
			
			if (tt->size)
				sc_flag = check_start_code(tt->buf, tt->size, &sc_size, mpeg->md->video_format);
			
			if (sc_flag)
				pes_offset = pes_size = 0;
			else
			{
				mpeg->seek_offset += pes_offset;

				if( mpeg->mpeg_bytes_size < pes_size )
				{
					FskMemPtrDisposeAt( &mpeg->mpeg_bytes );

					mpeg->mpeg_bytes_size = pes_size * 3/2; 
					err = FskMemPtrNewClear( mpeg->mpeg_bytes_size, &mpeg->mpeg_bytes );
					BAIL_IF_ERR(err);
				}

				err = doRead(mpeg, mpeg->seek_offset+mpeg->valid_offset, pes_size, mpeg->mpeg_bytes);
				BAIL_IF_ERR(err);
				mpeg->seek_offset += pes_size;
				
				if ((pes_offset+pes_size) == 0)
					mpeg->seek_offset += 1;
			}

			err = md_ps_reassemble_sample( mpeg->md, &pesh_x, 1, mpeg->mpeg_bytes, 
						pes_size, &is_video, &bs_bytes, &bs_size, sc_flag, sc_size, &time_pre, &time_dec);
			BAIL_IF_ERR(err);
		}
		
		if (IS_VALID_TIME(&time_dec))
			fsk_time = GET_TIME_64(&time_dec) - mpeg->time_init_fsk64;// /(double)mpeg->md.video_timescale;
		if (IS_VALID_TIME(&time_pre))
			cmp_time = GET_TIME_64(&time_pre) - mpeg->time_init_fsk64;
		else
			cmp_time = -1;

		if( is_video  && bs_size != 0 )
		{
			int sample_flag = kFskImageFrameTypeSync;
			MPEGReaderTrack track = mpeg->mpeg_tracks;
			//int keeper_condition = 0;
			
			if( mpeg->md->video_format == TRACK_H264   || mpeg->md->video_format == TRACK_H264_x )
			{
				err = check_avc_sync( 0, bs_bytes, bs_size, &sample_flag );
				BAIL_IF_ERR(err);
			}
			else if (mpeg->md->video_format == TRACK_MPEG1V || mpeg->md->video_format == TRACK_MPEG2V)
			{
				//sample_flag = kFskImageFrameTypeSync;
				int dc_size, pic_type;
				find_pic_start_code(bs_bytes, bs_size, &dc_size);
				pic_type = (bs_bytes[dc_size+5]>>3)&0x7; //check detail!!! PF.J
				
				if( pic_type == 1 ) //I
					sample_flag = kFskImageFrameTypeSync;
				else if( pic_type == 2 ) //P
					sample_flag = kFskImageFrameTypeDifference;
				else //B
					sample_flag = kFskImageFrameTypeDroppable | kFskImageFrameTypeDifference;
				
				dlog("\ndc_size= %d, pic_type = %d, frame_type = %d\n", dc_size, pic_type, sample_flag);
			}
			else {
				;//unimplement
			}

			if( kFskImageFrameTypeSync == sample_flag && mpeg->wait_for_video_sync_frame )
				mpeg->wait_for_video_sync_frame = 0;

			if(( mpeg->scrub ) && (kFskImageFrameTypeSync != sample_flag))
			{
				FskMemPtrDisposeAt((void **)&bs_bytes);
				continue;
			}

			if( mpeg->wait_for_video_sync_frame && (kFskImageFrameTypeSync != sample_flag))
			{
				FskMemPtrDisposeAt((void **)&bs_bytes);
				continue;
			}

			err = FskMemPtrNewClear(sizeof(FskMediaReaderSampleInfoRecord), &info);
			if (err) 
			{
				FskMemPtrDisposeAt((void **)&bs_bytes);
				goto bail;
			}

			*data = bs_bytes;
			
			if(0)
			{
				static FILE *f= NULL;

				if(f==NULL)
					f = fopen("E:\\test_mpeg\\ps-video-monitor.box", "wb");

				fwrite( &bs_size, 1, 4, f );
				fwrite( bs_bytes, 1, bs_size, f );
				fflush(f);
			}

			if (mpeg->dectime_interval == 0)
				mpeg->dectime_interval = fsk_time - mpeg->last_dectime;

			if (fsk_time && fsk_time == mpeg->last_dectime) {
				fsk_time = mpeg->last_dectime + mpeg->dectime_interval;

				if (sample_flag == (kFskImageFrameTypeDroppable | kFskImageFrameTypeDifference)) {
					cmp_time = fsk_time;
				} else if (sample_flag == kFskImageFrameTypeDifference || sample_flag == kFskImageFrameTypeSync) {
					if (cmp_time <= mpeg->last_B_cmptime)
					{
						if (sample_flag == kFskImageFrameTypeDifference)
							cmp_time = mpeg->last_P_cmptime + 2 * mpeg->dectime_interval;
						else
							cmp_time = mpeg->last_P_cmptime + mpeg->dectime_interval;
					}
				}
			}
			mpeg->last_dectime = fsk_time;
			if (sample_flag == (kFskImageFrameTypeDroppable | kFskImageFrameTypeDifference)) {
				mpeg->last_B_cmptime = cmp_time;
			} else if (sample_flag == kFskImageFrameTypeDifference || sample_flag == kFskImageFrameTypeSync) {
				mpeg->last_P_cmptime = cmp_time;
			}

			info->flags				= sample_flag;
			info->samples			= 1;
			info->sampleSize		= bs_size;
			info->decodeTime		= (FskInt64)fsk_time;
			info->compositionTime	= (FskInt64)cmp_time;

			*infoCountOut			= 1;
			*infoOut				= info;
			*trackOut				= &track->reader_track;
			
			dlog("outputting 1 video frame: size: %d, decode_time: %d, decode_time: %d, sample_flag: %d\n", 
					(int)info->sampleSize, (int)info->decodeTime, (int)info->compositionTime, (int)info->flags);

			mpeg->video_frame_count++;
			if( mpeg->video_frame_count == 1 )
			{
				mpeg->early_time_dec = time_dec;
				//sample_flag = kFskImageFrameImmediate; //check detail!!! PF.J
				//info->flags = sample_flag;
			}
			
			mpeg->later_time_dec = time_dec;

			break;
		}
		else if( !is_video  && bs_size != 0 )
		{
			MPEGReaderTrack track = mpeg->mpeg_tracks->next;
			int i;

			mpeg->audio_total_size += bs_size;
			
			if( mpeg->md->audio_format == TRACK_MPEGA   || mpeg->md->audio_format == TRACK_MPEGA_x )
				divide_mp3( bs_bytes, bs_size, &mpeg->md->frame_total, mpeg->md->frame_sizes, mpeg->md->frame_offsets );
			else
			{
				divide_adts( bs_bytes, bs_size, &mpeg->md->frame_total, 0, mpeg->md->frame_sizes, mpeg->md->frame_offsets );

				//frame_total = 1;
				//frame_sizes[0] = bs_size;
			}

			//if( mpeg->md->audio_sample_offset > 0 )
			//{ 
			//	FskMemCopy(bs_bytes, bs_bytes + mpeg->md->audio_sample_offset, bs_size - mpeg->md->audio_sample_offset);
			//}

			err = FskMemPtrNewClear(mpeg->md->frame_total*sizeof(FskMediaReaderSampleInfoRecord), &info);
			if (err) 
			{
				FskMemPtrDisposeAt((void **)&bs_bytes);
				goto bail;
			}

			*data = bs_bytes;	

			{
				unsigned char *this_src = bs_bytes;
				unsigned char *this_dst = bs_bytes;

				for( i = 0; i < mpeg->md->frame_total; i++ )
				{
					FskMediaReaderSampleInfo this_info = &info[i];
					int this_size   = mpeg->md->frame_sizes[i];
					int this_offset = mpeg->md->frame_offsets[i];

					this_info->flags				= kFskImageFrameTypeSync;
					this_info->samples			= 1;
					this_info->sampleSize		= this_size;
					this_info->decodeTime		= fsk_time;
					this_info->compositionTime	= -1;
					
					FskMemCopy(this_dst, this_src + this_offset, this_size);

					dlog("outputting 1 audio frame: size: %d, decode_time: %d, sample_flag: %d\n", 
							(int)info->sampleSize, (int)info->decodeTime, (int)info->flags);

					this_dst += this_size;
					this_src += this_offset + this_size;
				}
			}

			*infoCountOut			= mpeg->md->frame_total;
			*infoOut				= info;
			*trackOut				= &track->reader_track;
			break;
		}
		else if( /*bs_size != 0 && */bs_bytes != NULL ) //Fix Coverity 10819
		{
			FskMemPtrDisposeAt((void **)&bs_bytes);
		}
	}

	if( *infoOut == NULL )
		err = kFskErrNeedMoreTime;
bail:
	
	return err;
}


//
//meta data etc.
//

FskErr mpegReaderGetMetadata(FskMediaReader reader, void *readerState, const char *metaDataType, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	dlog("\n\n");
	dlog("into mpegReaderGetMetadata()\n");
	return kFskErrUnknownElement;
}


FskErr mpegReaderGetDuration(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	MPEGReader	mpeg = state;
	float		dur = 0;
	
	dlog("\n\n");
	dlog("into mpegReaderGetDuration()\n");
	if( !mpeg->md->initialized )
		return -1;
	
	dur = (float)(mpeg->duration/* & 0xFFFFFFFF*/);
	dlog("got float dur: %f\n", dur);
	property->type = kFskMediaPropertyTypeFloat;
	property->value.number = (float)dur;// /(double)mpeg->md.video_timescale;
	
	return kFskErrNone;
}

FskErr mpegReaderGetTime(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	dlog("\n\n");
	dlog("into mpegReaderGetTime()\n");
	return -1;
}

FskErr mpegReaderGetTimeScale(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	MPEGReader mpeg = state;

	dlog("\n\n");
	dlog("into mpegReaderGetTimeScale()\n");
	if( !mpeg->md->initialized )
		return -1;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = mpeg->md->video_timescale;
		
	dlog("got mpeg->md->video_timescale: %d\n", mpeg->md->video_timescale);

	return kFskErrNone;
}

FskErr mpegReaderGetState(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	dlog("\n\n");
	dlog("into mpegReaderGetState()\n");
	return -1;
}

FskErr mpegReaderSetScrub(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	MPEGReader mpeg = state;
	
	dlog("\n\n");
	dlog("into mpegReaderSetScrub()\n");
	if( !mpeg->md->initialized )
		return -1;

	mpeg->scrub = property->value.b;

	return kFskErrNone;
}

FskErr mpegReaderGetDLNASinks(void *state, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	static const char *list = "video/mpeg\000";

	property->type = kFskMediaPropertyTypeStringList;
	FskStrListDoCopy(list, &property->value.str);

	return kFskErrNone;
}

//
//
//track interface
//
//

FskErr mpegReaderTrackGetMediaType(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	MPEGReaderTrack track = state;
	MPEGReader		mpeg  = track->mpeg;

	dlog("\n\n");
	dlog("into mpegReaderTrackGetMediaType()\n");
	if( !mpeg->md->initialized )
		return -1;

	property->type = kFskMediaPropertyTypeString;
	if (kMPEGMediaTypeAudio == track->mediaType)
		property->value.str = FskStrDoCopy("audio");
	else
	if (kMPEGMediaTypeVideo == track->mediaType)
		property->value.str = FskStrDoCopy("video");
	else
		property->value.str = FskStrDoCopy("unknown");

	dlog("got track->mediaType: %d, property->value.str: %s\n", (int)track->mediaType, property->value.str);
	
	return kFskErrNone;
}


FskErr mpegReaderTrackGetProfile(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	MPEGReaderTrack track = state;
	MPEGReader		mpeg  = track->mpeg;
	int profile = track->profile;
	int level   = track->level;
	const char *profile_str;
	char level_str[16];
	
	dlog("\n\n");
	dlog("into mpegReaderTrackGetMediaType()\n");
	if( !mpeg->md->initialized )
		return -1;

	if (kMPEGMediaTypeVideo == track->mediaType)
	{
		if( 0 == FskStrCompare(track->format, "x-video-codec/avc")) 
		{
				 if( profile == 66 )	profile_str = "baseline,";
			else if( profile == 77 )	profile_str = "main,";
			else if( profile == 88 )	profile_str = "Extended,";
			else if( profile == 100 )	profile_str = "High,";
			else if( profile == 110 )	profile_str = "High 10,";
			else if( profile == 122 )	profile_str = "High 4:2:2,";
			else if( profile == 244 )	profile_str = "High 4:4:4,";
			else
				profile_str = "unknown,";
		}
		else
			profile_str = "unknown";
	}
	else if (kMPEGMediaTypeAudio == track->mediaType)
	{
		if( 0 == FskStrCompare(track->format, "x-audio-codec/aac")) 
		{
				 if( profile == 1 )		profile_str = "AAC Main,";
			else if( profile == 2 )		profile_str = "AAC LC,";
			else if( profile == 3 )		profile_str = "AAC SSR,";
			else if( profile == 4 )		profile_str = "AAC LTP,";
			else if( profile == 5 )		profile_str = "SBR,";
			else if( profile == 6 )		profile_str = "AAC Scalable,";
			else if( profile == 7 )		profile_str = "TwinVQ,";
			else if( profile == 8 )		profile_str = "CELP,";
			else if( profile == 9 )		profile_str = "HXVC,";
			else
				profile_str = "unknown,";
		}
		else
			profile_str = "unknown,";
	}
	else
		profile_str = "unknown,";
	
	FskStrNumToStr( level, level_str, sizeof(level_str));
	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCat( profile_str, level_str );
	
	dlog("got track->mediaType: %d, property->value.str: %s\n", (int)track->mediaType, property->value.str);

	return kFskErrNone;
}


FskErr mpegReaderTrackGetFormat(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	MPEGReaderTrack track = state;
	MPEGReader		mpeg  = track->mpeg;

	dlog("\n\n");
	dlog("into mpegReaderTrackGetFormat()\n");
	if( !mpeg->md->initialized )
		return -1;

	property->type = kFskMediaPropertyTypeString;
	property->value.str = FskStrDoCopy(track->format);
	
	dlog("got track->format: %s\n", track->format);

	return kFskErrNone;
}

static void find_pic_start_code(unsigned char *data, int size, int *dc_size)
{
	unsigned char *p = data;
	*dc_size = 0;
	
	while(1)
	{
		if (p[0]==0x0 && p[1]==0x0 && p[2]==0x1 && p[3]==0x0)
			break;
		else {
			(*dc_size) ++;
			p ++;
		}
		
		if ((*dc_size) >= size)
			break;
	}
	
	return;
}


int create_avcc_data_from_spspps( unsigned char *avcc_data, int *avcc_data_size, unsigned char *spspps, int spspps_size )
{
	//Example AlienSong.MP4
	//01						configurationVersion
	//42						AVCProfileIndication
	//E0						profile_compatibility
	//0D						ACVLevelIndication
	
	//FF						reserved6BitsAndLengthSizeMinusOne
	//E1						reserved3BitsAndNumberofSequenceParameterSets
	//00 16						sps_size
	
	//example: 27 42 E0 0D A9 18 28 3F	sps
	
	//sps structures
	//27						nalu type
	//42						profile idc
	//E0						4 constraint bits + 4 reserved bits
	//0D						level idc
	
	//60 0D 41 80 41 AD B7 A0 
	//2F 01 E9 7B DF 01 
	//01						numberofPictureParameterSets
	//00 04						pps_size
	//28 DE 09 88				pps
	
	unsigned char *sps, *pps;
	int			  sps_size, pps_size;
	unsigned char configurationVersion;
	unsigned char AVCProfileIndication;//0x42;	
	unsigned char profile_compatibility;//0xE0;	
	unsigned char ACVLevelIndication;//0x0;	
	unsigned char reserved6BitsAndLengthSizeMinusOne;	
	unsigned char reserved3BitsAndNumberofSequenceParameterSets;	
	
	sps		  = spspps;
	sps_size  = (int)FskMisaligned32_GetBtoN(sps);
	sps		 += 4;//always 4 since we set it earlier
	
	pps		  = sps + sps_size;
	pps_size  = (int)FskMisaligned32_GetBtoN(pps);
	pps		 += 4;//always 4 since we set it earlier
	
	configurationVersion	= 0x01;
	AVCProfileIndication	= *(sps+1);//0x42;
	profile_compatibility	= *(sps+2);//0xE0;
	ACVLevelIndication	= *(sps+3);//0x0;
	reserved6BitsAndLengthSizeMinusOne = 0xFF;
	reserved3BitsAndNumberofSequenceParameterSets = 0xE1;
	
	*(avcc_data++) = configurationVersion;
	*(avcc_data++) = AVCProfileIndication;
	*(avcc_data++) = profile_compatibility;
	*(avcc_data++) = ACVLevelIndication;
	
	*(avcc_data++) = reserved6BitsAndLengthSizeMinusOne;
	*(avcc_data++) = reserved3BitsAndNumberofSequenceParameterSets;
	
	*(avcc_data++) = (sps_size&0x0000ff00)>>8;
	*(avcc_data++) = (sps_size&0x000000ff)>>0;;
	memcpy((void *)avcc_data, (void *)sps, sps_size);
	avcc_data += sps_size;
	
	*(avcc_data++) = 1;
	*(avcc_data++) = (pps_size&0x0000ff00)>>8;
	*(avcc_data++) = (pps_size&0x000000ff)>>0;;
	memcpy((void *)avcc_data, (void *)pps, pps_size);
//	avcc_data += pps_size;
	
	*avcc_data_size = 	4 +					//header
						1 +					//nalu
						1 +	2 + sps_size +	//sps
						1 + 2 + pps_size;	//pps
	
	return 0;
}


FskErr mpegReaderTrackGetFormatInfo(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	MPEGReaderTrack track = state;
	MPEGReader		mpeg  = track->mpeg;
	//MPEG_TS_PRIV	*priv = (MPEG_TS_PRIV *)mpeg->md->priv;
	FskErr			err   = kFskErrNone;

	dlog("\n\n");
	dlog("into mpegReaderTrackGetFormatInfo()\n");
	if( !mpeg->md->initialized )
		return -1;

	dlog("track->mediaType: %d, track->format: %s\n", (int)track->mediaType, track->format);
	if (kMPEGMediaTypeVideo == track->mediaType && 0 == FskStrCompare(track->format, "x-video-codec/avc")) 
	{
		unsigned char *desc, *spspps, *avcC;
		UInt32 descSize, spspps_size, avcC_size;
		UInt32 const_avcC = 'avcC', const_avc1 = 'avc1';
		
		if (0 == track->codecSpecificDataSize)
			return kFskErrUnimplemented;

		spspps		= track->codecSpecificDataRef;
		spspps_size = track->codecSpecificDataSize;
		
		avcC_size = spspps_size + 11;
		descSize = sizeof(QTImageDescriptionRecord) + avcC_size;

		err = FskMemPtrNewClear(descSize, (FskMemPtr *)&desc);
		BAIL_IF_ERR(err);

		FskMemMove(desc, &descSize, 4);			
		FskMemMove(desc + 4, &const_avc1, 4);		
		avcC = desc + sizeof(QTImageDescriptionRecord);
		
		FskMemMove(avcC, &avcC_size, 4);		
		FskMemMove(avcC + 4, &const_avcC, 4);	
		avcC += 8;
		
		create_avcc_data_from_spspps( avcC, (int *)&avcC_size, spspps, (UInt32)spspps_size );

		{//set extra info
			QTImageDescriptionRecord *image_desc = (QTImageDescriptionRecord *)desc;
			
			//image_desc->idSize;						//83 00 00 00
			//image_desc->cType;						//31 63 76 61
			//image_desc->resvd1;						//00 00 00 00
			//image_desc->resvd2;						//00 00
			image_desc->dataRefIndex	= 1;			//01 00
			image_desc->version			= 0;			//00 00 
			image_desc->revisionLevel	= 0;			//00 00 
			image_desc->vendor			= 0;			//00 00 00 00
			image_desc->temporalQuality = 0x00000200;	//00 02 00 00
			image_desc->spatialQuality  = 0x00000200;	//00 02 00 00
			image_desc->width			= (UInt16)track->video.dimensions.width;
			image_desc->height			= (UInt16)track->video.dimensions.height;
			image_desc->hRes			= 0x00480000;	//00 00 48 00 
			image_desc->vRes			= 0x00480000;	//00 00 48 00 
			image_desc->dataSize		= 0;			//00 00 00 00
			image_desc->frameCount		= 1;			//01 00 
			//image_desc->name[32];	
			image_desc->depth			= 0x0018;		//18 00
			image_desc->clutID			= 0xffff;		//ff ff
		}

		property->type = kFskMediaPropertyTypeData;
		property->value.data.dataSize = descSize;
		property->value.data.data = desc;
	}
	else if (kMPEGMediaTypeAudio == track->mediaType && 0 == FskStrCompare(track->format, "x-audio-codec/aac")) 
	{
		unsigned char *esds = NULL;
		int	esds_size = track->codecSpecificDataSize;

		err = FskMemPtrNewClear(esds_size, (FskMemPtr *)&esds);
		BAIL_IF_ERR(err);
		
		FskMemCopy( (void *)esds, (void *)track->codecSpecificDataRef, esds_size );

		property->type = kFskMediaPropertyTypeData;
		property->value.data.dataSize = esds_size;
		property->value.data.data = esds;
	}
	else if (kMPEGMediaTypeVideo == track->mediaType && 0 == FskStrCompare(track->format, "x-video-codec/mpg")) 
	{
		unsigned char *esds = NULL;
		int	esds_size = track->codecSpecificDataSize;
		
		err = FskMemPtrNewClear(esds_size, (FskMemPtr *)&esds);
		BAIL_IF_ERR(err);
		
		FskMemCopy( (void *)esds, (void *)track->codecSpecificDataRef, esds_size );
		
		property->type = kFskMediaPropertyTypeData;
		property->value.data.dataSize = esds_size;
		property->value.data.data = esds;
	}
	//else if (kMPEGMediaTypeAudio == track->mediaType && 0 == FskStrCompare(track->format, "x-audio-codec/ac3")) 
	//{
	//}
	else
	{
		dlog("not avc or aac, need to be implemented!!!\n");
		err = -1;
	}

bail:
	return err;
}


FskErr mpegReaderVideoTrackGetFrameRate(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	MPEGReaderTrack	track = state;
	MPEGReader			mpeg = track->mpeg;
	int					frame_dur;
	FskErr				err = kFskErrNone;

	dlog("\n\n");
	dlog("into mpegReaderVideoTrackGetFrameRate()\n");
	if( !mpeg->md->initialized )
		return -1;

	if( mpeg->md->video_framerate != 0 )
	{
		property->type = kFskMediaPropertyTypeInteger;
		property->value.integer = mpeg->md->video_framerate;
		dlog("got track->video.framerate: %d\n", (int)mpeg->md->video_framerate);
		return kFskErrNone;
	}
	
	
	if( mpeg->video_frame_count > 1 )
	{
		TIME_64	frame_dur_64;

		DIFF_TIME_64_ABS(&frame_dur_64, &mpeg->later_time_dec, &mpeg->early_time_dec );
		frame_dur = GET_TIME_64(&frame_dur_64); 
		frame_dur /= (mpeg->video_frame_count-1);

		dlog("mpeg->video_frame_count: %d\n", mpeg->video_frame_count);
	}
	else
	{
		if( mpeg->is_ts )
		{
			err = ts_get_frame_dur( mpeg, &frame_dur );
			dlog("mpeg->is_ts is true, frame_dur: %d\n", frame_dur );
		}
		else
		{
			frame_dur = 0; //need to implement
			dlog("mpeg->is_ts is false, frame_dur: %d\n", frame_dur );
		}

		BAIL_IF_ERR(err);
	}

	property->type = kFskMediaPropertyTypeRatio;
	property->value.ratio.numer = MPEG_VIDEO_DEFAULT_TIME_SCALE;
	property->value.ratio.denom = frame_dur;

bail:
	return err;
}

FskErr mpegReaderAudioTrackGetSampleRate(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	MPEGReaderTrack track = state;
	MPEGReader		mpeg  = track->mpeg;

	dlog("\n\n");
	dlog("into mpegReaderAudioTrackGetSampleRate()\n");
	if( !mpeg->md->initialized )
		return -1;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->audio.samplerate;//track->media.audio.samplesPerSecond;

	dlog("got track->audio.samplerate: %d\n", (int)track->audio.samplerate);
	
	return kFskErrNone;
}

FskErr mpegReaderAudioTrackGetChannelCount(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	MPEGReaderTrack track = state;
	MPEGReader		mpeg  = track->mpeg;

	dlog("\n\n");
	dlog("into mpegReaderAudioTrackGetChannelCount()\n");
	if( !mpeg->md->initialized )
		return -1;

	property->type = kFskMediaPropertyTypeInteger;
	property->value.integer = track->audio.channel_total;

	dlog("got track->audio.channel_total: %d\n", (int)track->audio.channel_total);

	return kFskErrNone;
}

FskErr mpegReaderVideoTrackGetDimensions(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	MPEGReaderTrack track = state;
	MPEGReader mpeg = track->mpeg;

	dlog("\n\n");
	dlog("into mpegReaderVideoTrackGetDimensions()\n");
	if( !mpeg->md->initialized )
		return -1;

	property->type = kFskMediaPropertyTypeDimension;
	property->value.dimension.width  = track->video.dimensions.width;
	property->value.dimension.height = track->video.dimensions.height;

	dlog("got track->video.dimensions.width/height: %d, %d\n", (int)track->video.dimensions.width, (int)track->video.dimensions.height);
	
	return kFskErrNone;
}

FskErr mpegReaderTrackGetBitRate(void *state, void *trackState, UInt32 propertyID, FskMediaPropertyValue property)
{
	MPEGReaderTrack track = state;
	MPEGReader		mpeg  = track->mpeg;
	MPEGReaderTrack this_track;
	int			audio_bps = 0;

	dlog("\n\n");
	dlog("into mpegReaderTrackGetBitRate()\n");
	if( !mpeg->md->initialized )
		return -1;

	if (mpeg->duration == -1)
	{// can not get valid infor in this situation
		property->value.integer = 0;
		property->type = kFskMediaPropertyTypeInteger;

		return kFskErrNone;
	}

	if( mpeg->video_frame_count > 1 )
	{
		TIME_64	dur_64;
		float dur_f;
		
		DIFF_TIME_64_ABS(&dur_64, &mpeg->later_time_dec, &mpeg->early_time_dec );

		dur_f = (float)GET_TIME_64(&dur_64);

		dlog("\n dur_f = %d \n", (int)dur_f);
		
		if( dur_f != 0 )
			audio_bps = (int)(8*MPEG_VIDEO_DEFAULT_TIME_SCALE*(float)mpeg->audio_total_size / dur_f);
	}
	
	property->type = kFskMediaPropertyTypeInteger;
	if (kMPEGMediaTypeVideo == track->mediaType) 
	{
		float duration = (float)mpeg->duration / (float)mpeg->md->video_timescale;
		
		dlog("\n baseduration = %d, duration = %d, timescale = %d, audio_bps = %d \n", (int)mpeg->duration, (int)duration, (int)mpeg->md->video_timescale, (int)audio_bps);

		for (this_track = mpeg->mpeg_tracks; NULL != this_track; this_track = this_track->next) 
		{
			if (kMPEGMediaTypeAudio == this_track->mediaType  )
			{
				audio_bps = this_track->bps != 0 ? this_track->bps: audio_bps;
				break;
			}
		}
		property->value.integer = (SInt32)((mpeg->total_size_fsk64* 8 / duration) - audio_bps);
	}
	else if (kMPEGMediaTypeAudio == track->mediaType)
	{
		property->value.integer = track->bps != 0 ? track->bps : audio_bps;
	}
	
	return kFskErrNone;
}


