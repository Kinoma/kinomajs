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
#ifndef __MPEG_X_DEMUXER_H__
#define __MPEG_X_DEMUXER_H__

// #define STAND_ALONE
// #define MPEG_DEBUG_MESSAGE

#ifdef STAND_ALONE
#ifndef FskErr
#define FskErr		int
#endif
#ifndef kFskErrBadData
#define kFskErrBadData -23
#endif
#define BAIL_IF_ERR(err)					do { if ((err) != 0)	{ goto bail;                 } } while(0)
#define 	GET_TIME_64(t)									\
(														\
    (FskInt64)(((FskInt64)GET_TIME_64_Hi(t))<<32)|		\
    (FskInt64)GET_TIME_64_Lo(t)							\
)
#define	UInt16		unsigned short
#define	SInt32		signed long
#define	UInt32		unsigned long
#define FskInt64    long long
#define FskMemPtr	unsigned char *
#define kFskErrNone				0
#define kFskErrUnknownElement	-12
#define FskMemMove memmove
void	FskMemCopy(void *dst, const void *src, UInt32 count);
void	FskMemSet(void *dst, char fill, UInt32 count);
FskErr	FskMemPtrNew(UInt32 size, FskMemPtr *newMemory);
FskErr	FskMemPtrNewClear(UInt32 size, FskMemPtr *newMemory);
FskErr	FskMemPtrDisposeAt(void **ptr);

#else
#include "FskMemory.h"
#endif

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#ifdef MPEG_DEBUG_MESSAGE

//#define MY_DUMP_FILE_PATGH	"E:\\debug_log\\mpeg_err.txt"
//#define MY_DUMP_FILE_PATGH    "/sdcard/e"
//#define MY_DUMP_FILE_PATGH	"/data/data/com.kinoma.kinomaplay/files/Download/mpg_r"
#define MY_DUMP_FILE_PATGH	"/tmp/ermp2"

extern FILE *fMPEGErr;
#define dlog(...)							\
{												\
	if( fMPEGErr == NULL )							\
		fMPEGErr = fopen(MY_DUMP_FILE_PATGH, "w"); 	\
												\
	if( fMPEGErr != NULL )							\
	{											\
		fprintf(fMPEGErr, "%d: ",__LINE__);			\
		fprintf(fMPEGErr,__VA_ARGS__);				\
		fflush( fMPEGErr );							\
	}											\
}


#define viewerr_exit(err)	 { dlog("XXXXXXXXXXXXXXXXXXXXXXXXXXXX err = %d, exiting!!!\n", (int)err); exit(-1); }
#define viewerr_safe(err)	 { dlog("XXXXXXXXXXXXXXXXXXXXXXXXXXXX err = %d\n", (int)err);}

#else

#define dlog(...)
#define viewerr_exit(err)
#define viewerr_safe(err)

#endif

#define MAX_PROGRAM_NUM					8
#define MAX_TRACK						5

#define MIN_FRAME_SAMPLE_BUFFER_SIZE	1024*16
#define MAX_COUNT_FOR_FRAME_RATE		11

#define MPEG_TS_SYNC_BYTE	0x47
#define NALU_LEN		4

// NAL unit definitions
//#define NAL_STORAGE_IDC_BITS    0x60

#define NAL_UNITTYPE_BITS				0x1f
#define MPEG_VIDEO_DEFAULT_TIME_SCALE	90000

#define 	kMPEGDemuxerErrNeedMoreTime		-202

#define ID_VIDEO            0xE0
#define ID_AUDIO            0xC0

//PF.J 2012.01.11
/* MPEG start code IDs */
#define PICTURE_START_CODE      0x100
#define SLICE_START_CODE_MIN    0x101
#define SLICE_START_CODE_MAX    0x1AF
#define USER_DATA_START_CODE    0x1B2
#define SEQUENCE_HEADER_CODE    0x1B3
#define SEQUENCE_ERROR_CODE     0x1B4
#define EXTENSION_START_CODE    0x1B5
#define SEQUENCE_END_CODE       0x1B7
#define GROUP_START_CODE        0x1B8

/* extension start code IDs */
#define SEQUENCE_EXTENSION_ID                    1
#define SEQUENCE_DISPLAY_EXTENSION_ID            2
#define QUANT_MATRIX_EXTENSION_ID                3
#define COPYRIGHT_EXTENSION_ID                   4
#define SEQUENCE_SCALABLE_EXTENSION_ID           5
#define PICTURE_DISPLAY_EXTENSION_ID             7
#define PICTURE_CODING_EXTENSION_ID              8
#define PICTURE_SPATIAL_SCALABLE_EXTENSION_ID    9
#define PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID  10


typedef enum 
{
    NAL_UT_RESERVED  = 0x00, // Reserved
    NAL_UT_SLICE     = 0x01, // Coded Slice - slice_layer_no_partioning_rbsp
    NAL_UT_DPA       = 0x02, // Coded Data partition A - dpa_layer_rbsp
    NAL_UT_DPB       = 0x03, // Coded Data partition A - dpa_layer_rbsp
    NAL_UT_DPC       = 0x04, // Coded Data partition A - dpa_layer_rbsp
    NAL_UT_IDR_SLICE = 0x05, // Coded Slice of a IDR Picture - slice_layer_no_partioning_rbsp
    NAL_UT_SEI       = 0x06, // Supplemental Enhancement Information - sei_rbsp
    NAL_UT_SPS       = 0x07, // Sequence Parameter Set - seq_parameter_set_rbsp
    NAL_UT_PPS       = 0x08, // Picture Parameter Set - pic_parameter_set_rbsp
    NAL_UT_PD        = 0x09, // Picture Delimiter - pic_delimiter_rbsp
    NAL_UT_FD        = 0x0a  // Filler Data - filler_data_rbsp
} NAL_Unit_Type;

typedef enum 
{
    TRACK_MPEG1V		= 0x01,
    TRACK_MPEG2V		= 0x02,
    TRACK_MPEGA			= 0x03,
    TRACK_MPEGA_x		= 0x04,
    TRACK_AAC			= 0x0F,
    TRACK_MPEG4V		= 0x10,
    TRACK_AAC_x			= 0x11,
    TRACK_H264			= 0x1A,
    TRACK_H264_x		= 0x1B,
    TRACK_AC3			= 0x81,
    TRACK_LPCM			= 0x83
} MPEG_Stream_Type;


#define IS_VIDEO_FORMAT(x) ((x==TRACK_MPEG1V)||(x==TRACK_MPEG2V) ||(x==TRACK_MPEG4V)||(x==TRACK_H264 )||(x==TRACK_H264_x))
#define IS_AUDIO_FORMAT(x) ((x==TRACK_MPEGA )||(x==TRACK_MPEGA_x)||(x==TRACK_AAC   )||(x==TRACK_AAC_x)||(x==TRACK_AC3 )||(x==TRACK_LPCM  ))


typedef struct
{
	unsigned char 	configurationVersion;
	unsigned char 	AVCProfileIndication;
	unsigned char 	profile_compatibility;
	unsigned char 	ACVLevelIndication;
	unsigned char 	naluLengthSize;
	unsigned char 	numberofPictureParameterSets;
	unsigned char 	numberofSequenceParameterSets;

	unsigned char 	*sps;
	unsigned short 	spsSize;
	unsigned char 	*pps;
	unsigned short 	ppsSize;
	
	unsigned char 	spspps[256];
	unsigned short 	spsppsSize;
} mpeg_AVCC;

typedef struct 
{
	unsigned long 		hi;
	unsigned long 		lo;
}TIME_64;

#if 0

#define MAX_U32_VALUE				0xffffffff
#define IS_VALID_TIME(t)				((t)->hi!= MAX_U32_VALUE && (t)->lo!= MAX_U32_VALUE )
#define INVALIDATE_TIME(t)			{(t)->hi = MAX_U32_VALUE;   (t)->lo = MAX_U32_VALUE;}
#define BIGGER_TIME(t1, t2 )			( (t1)->lo > (t2)->lo )	//***bnie: not regarding hi bit yet
//#define DIFF_TIME(t1, t2 )			( (t1)->lo - (t2)->lo )	//***bnie: not regarding hi bit yet
#define MY_64_ABS(a)					((a)->lo=(a)->lo>=0?(a)->lo:-(a)->lo)
#define DIFF_TIME_64(t0, t1, t2 )	{(t0)->hi = 0; (t0)->lo = (t1)->lo - (t2)->lo;}	//***bnie: not regarding hi bit yet
#define DIFF_TIME_64_ABS(t0, t1, t2) { DIFF_TIME_64(t0, t1, t2 ); MY_64_ABS(t0); }	//***bnie: not regarding hi bit yet
#define GET_TIME_64(t )				( (t)->lo )	//***bnie: not regarding hi bit yet

#else
#define MAX_U32_VALUE				0xffffffff
#define IS_VALID_TIME(t)				((t)->hi!= MAX_U32_VALUE && (t)->lo!= MAX_U32_VALUE )
#define INVALIDATE_TIME(t)			{(t)->hi = MAX_U32_VALUE;   (t)->lo = MAX_U32_VALUE;}
#define GET_TIME_64_Hi(t )			( (t)->hi )
#define GET_TIME_64_Lo(t )			( (t)->lo )
#define BIGGER_TIME(t1, t2 )			(										\
										(t1)->hi == (t2)->hi ?				\
										( (t1)->lo > (t2)->lo )				\
										:									\
										( (t1)->hi > (t2)->hi ?  1 : 0 )	\
									)	

#define IS_DIFFERENT_TIME( a, b )	(										\
										IS_VALID_TIME(a) &&					\
										IS_VALID_TIME(b) &&					\
										(									\
											((a)->hi!=(b)->hi) ||			\
											((a)->lo!=(b)->lo)				\
										)									\
									)

#define DIFF_TIME_64_ABS(t0, t1, t2) {										\
										TIME_64 *a, *b;						\
										if( BIGGER_TIME( t1, t2 ) )			\
										{									\
											a = (t1);						\
											b = (t2);						\
										}									\
										else								\
										{									\
											b = (t1);						\
											a = (t2);						\
										}									\
																			\
										if( a->hi == b->hi )				\
										{									\
											(t0)->hi = 0;				    \
											(t0)->lo = a->lo - b->lo;		\
										}									\
										else								\
										{									\
											unsigned long hi = a->hi - b->hi;	\
											unsigned long lo = (a->lo >= b->lo) ? a->lo - b->lo :  b->lo - a->lo;	\
																			\
											if( a->lo < b->lo)				\
											{								\
												hi--;						\
												lo = MAX_U32_VALUE - lo;	\
											}								\
																			\
											(t0)->hi = hi;					\
											(t0)->lo = lo;					\
										}									\
									}

#endif

typedef struct 
{
	int				pid;
	int				stream_id;

	TIME_64			time_pre;
	TIME_64			time_dec;
	int				size;
	unsigned char   *buf;
	int				buf_size;
	int 			has_frame;
}FRAME_BUFFER;

typedef struct 
{
	int is_valid;
	int stream_id;
	int pes_packet_length;
	int always_10;
	int pes_scrambling_control;
	int pes_priority;
	int data_alignment_indicator;
	int copy_right;
	int original_or_copy;
	
	int pts_dts_flag;
	int escr_flag;
	int es_rate_flag;
	int dsm_trick_mode_flag;
	int additional_copy_info_flag;
	int pes_crc_flag;
	int pes_extension_flag;

	TIME_64 presentation_time_stamp;
	TIME_64 decoding_time_stamp;

	int	pes_header_data_length;

	int ES_rate;

	int trick_mode_control;
	int	field;
	int intra_slice_refresh;
	int frequency_truncation;
	int rep_cntrl;

	int additional_copy_info;
	int previous_PES_packet_CRC;

}PES_HEADER;

//PF.J 2012.01.12 MPEG HEADER
typedef struct
{
	int				horizontal_size;
	int				vertical_size;
	int				aspect_fatio_information;
	int				frame_rate_code;
	int				bit_rate_code;
	int				mp2_video_flag;
	
}MPEG_SEQ_HEADER;

typedef struct
{
	int				profile_and_level_indication;
	int				progressive_sequence;
	int				chroma_format;
	int				horizontal_size_extension;
	int				vertical_size_extension;
	int				bit_rate_extension;
	int				low_delay;
	int				frame_rate_extension_n;
	int				frame_rate_extension_d;
	
}MPEG_SEQEXT_HEADER;

typedef struct 
{
	int				initialized;

	int				video_header_initialized;
	int				video_elementary_pid;
	int				video_format;
	int				video_timescale;
	//PF.J 2012.01.12
	int				video_bitrate;
	float			video_framerate;
	int				video_height;
	int				video_width;
	int				video_profile;
	int				video_level;
	
	int				audio_header_initialized;
	int				audio_elementary_pid;
	int				audio_format;
	int				audio_sample_offset;
	//Pf.J 2012.01.13
	UInt32			audio_profile;
	UInt32			audio_level;
	int				audio_bitrate;
	int				audio_samplerate;
	int				audio_channel_total;
	int				audio_layer;

	FRAME_BUFFER	fb_ary[MAX_TRACK];

	unsigned char	*audio_codec_header_data;
	int				audio_codec_header_data_size;
	
	unsigned char	*video_codec_header_data;
	int				video_codec_header_data_size;

	void			*priv;

	int				frame_total;
	int				frame_offsets[64];
	int				frame_sizes[64];
	
	//PF.J 2012.01.12
	MPEG_SEQ_HEADER    msh;
	MPEG_SEQEXT_HEADER mseh;

}MPEG_DEMUXER;



#define SHOW_BYTE(d, p)		{ d = ((p[0]<<0)); }
#define SHOW_SHORT_BE(d, p)	{ d = ((p[0]<<8)|(p[1]<<0)); }
#define SHOW_LONG_BE(d, p)	{ d = ((p[0]<<24)|(p[1]<<16)|(p[2]<<8)|(p[3]<<0)); }

#define GET_BYTE(d, p)		{ d = ((p[0]<<0)); p++; }
#define GET_SHORT_BE(d, p)	{ d = ((p[0]<<8)|(p[1]<<0)); p+= 2; }
#define GET_24BITS_BE(d, p)	{ d = ((p[0]<<16)|(p[1]<<8)|(p[0]<<0)); p+= 3; }
#define GET_LONG_BE(d, p)	{ d = ((p[0]<<24)|(p[1]<<16)|(p[2]<<8)|(p[3]<<0)); p += 4; }
#define TS_LENGTH			188
#define IS_AUDIO_STREAM(id)				((id&0xe0) == 0xc0 )		
#define IS_VIDEO_STREAM(id)				((id&0xf0) == 0xe0 )	
#define IS_PADDING_STREAM(id)			((id     ) == 0xbe )	
#define IS_PRIVATE_STREAM(id)			((id     ) == 0xbd )	
#define IS_PROGRAM_ASSOCIATION_TABLE(id)	((id)	   == 0x00 )

#define LONGB2N(a)  (((a>>24)&0x000000ff) | \
					((a>>8 )&0x0000ff00) | \
					((a<<8 )&0x00ff0000) | \
					((a<<24)&0xff000000) )



#define DECODE_TS(p_x, hi_x, lo_x, ref_x )							\
{																	\
	int bits_00xx;													\
	int marker_bit;													\
	int bits_3;														\
	int bits_15_1;													\
	int bits_15_2;													\
	int tmp_x;														\
																	\
	tmp_x = p_x[0]; p_x++;											\
	bits_00xx = (tmp_x>>4)&0x0f;									\
	if( bits_00xx != ref_x )										\
	{																\
		dlog( "wrong time stamp start marker!!!\n");		\
	}																\
	bits_3	  = (tmp_x>>1)&0x07;									\
	marker_bit= (tmp_x>>0)&0x01;									\
																	\
	GET_LONG_BE(tmp_x, p)											\
	bits_15_1 = (tmp_x>>17)&0x7fff;									\
	marker_bit= (tmp_x>>16)&0x0001;									\
	bits_15_2 = (tmp_x>> 1)&0x7fff;									\
	marker_bit= (tmp_x>> 0)&0x0001;									\
																	\
	hi_x =  bits_3>>2;												\
	lo_x = (bits_3<<30)|(bits_15_1<<15)|bits_15_2;					\
}

#ifdef __cplusplus
extern "C" {
#endif

int  md_new( MPEG_DEMUXER **md_out );
void md_dispose( MPEG_DEMUXER *md );
int  md_reset_buffer( MPEG_DEMUXER *md );

void find_mp3_boundary_from_end( unsigned char *src, int size, int *boundary );
int parse_pes_packet( unsigned char *p, PES_HEADER *ph, int *offset, int *size );
int pars_mp3_header(unsigned char *header, long size, long *sampleRate, long *channelCount);
int divide_adts( unsigned char *s, int size, int *total, int offset, int *sizes, int *offsets );
int divide_mp3( unsigned char *s, int size, int *total, int *sizes, int *offsets );
int check_avc_sample_integrity(unsigned char *src, int size, int naluLengthSize );


int check_nalu( int is_startcode, unsigned char *data, int size, long *nalu_type_out, long *ref_idc_out );
void my_assert(int is_true );

int md_ps_parse_header_x( MPEG_DEMUXER *md, unsigned char  *mpeg_bytes, int *is_audio, PES_HEADER  *pesh_x, int *pes_offset, int *pes_size  );
int md_ts_parse_header( MPEG_DEMUXER *md, unsigned char  *mpeg_bytes );
int md_ps_reassemble_sample ( MPEG_DEMUXER *md, PES_HEADER *pesh_x, int use_raw, unsigned char *pes_bytes, int pes_size, int *is_video, unsigned char **bs_bytes_out, int *bs_size, int sc_flag, int sc_size, TIME_64 *time_pre, TIME_64 *time_dec);
int md_parse_video_codec( MPEG_DEMUXER *md, unsigned char *bs_bytes, int bs_size );
int md_parse_audio_codec( MPEG_DEMUXER *md, unsigned char *bs_bytes, int bs_size );
FRAME_BUFFER *get_fb_by_idx( FRAME_BUFFER *track, int pid, int stream_id );
int startcode2len( unsigned char *data, int size, unsigned char *qt_data, int *out_size );
int startcode2lenskip( unsigned char *data, int size, unsigned char *qt_data, int *out_size, int *skip_bytes );
int checkframestart( unsigned char *data, int size, int *hasframe );
void rest_frame_buffer( FRAME_BUFFER *f );
void refit_frame_buffer( FRAME_BUFFER *f, int size );
int check_start_code(unsigned char *data, int size, int *offset, int format);
	
//PF.J 2012.01.11
void sequence_header(MPEG_DEMUXER *md, unsigned char *mpeg_bytes);
void group_of_pictures_header(MPEG_DEMUXER *md, unsigned char *mpeg_bytes);
void picture_header(MPEG_DEMUXER *md, unsigned char *mpeg_bytes);
void extension_and_user_data(MPEG_DEMUXER *md, unsigned char *mpeg_bytes);
#ifdef __cplusplus
}
#endif


#endif
