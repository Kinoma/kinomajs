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
#include "mpeg_x_demuxer.h"

#define PS_PACKET_START_CODE					0x000001BA
#define SYSTEM_HEADER_START_START_CODE		0x000001BB
#define PACKET_HEADER_LENGTH					14
#define PACK_HEADER_LENGTH_MPEG1				12
#define SYSTEM_HEADER_BEFORE_LENGTH_LENGTH	6
#define SYSTEM_HEADER_AFTER_LENGTH_LENGTH	6
#define SYSTEM_HEADER_STREAM_SPEC_LENGTH		3
#define MAX_STREAM_TOTAL						8

#define PROGRAM_STREAM_MIN_BYTES_TO_VALIDATE	256*2
#define PROGRAM_STREAM_CODE					0xba

typedef struct 
{
	int is_valid;
	int	packet_start_code;
	//int const_01;
	int system_clock_reference_base_2;
	int system_clock_reference_base_1;
	int system_clock_reference_base_0;
	int	system_color_reference_extension;
	int program_mux_rate;
	int	program_stuffing_length;
	
}PS_PACK_HEADER_x;


typedef struct 
{
	int stream_id;
	int P_STD_buffer_bound_scale;
	int P_STD_buffer_size_bound;
}PS_SYSTEM_HEADER_STREAM_SPEC_x;

typedef struct 
{
	int is_valid;
	int system_header_start_code;
	int	header_length;
	int rate_bound;
	int audio_bound;
	int	fixed_flag;
	int CSPS_flag;
	int system_audio_clock_flag;
	int system_video_clock_flag;
	int video_bound;
	int packet_rate_restriction_flag;

	int	stream_total;
	PS_SYSTEM_HEADER_STREAM_SPEC_x  stream_spec[MAX_STREAM_TOTAL];
	
}PS_SYSTEM_HEADER_x;


typedef struct 
{
	int					sys_initialized;
	PS_SYSTEM_HEADER_x	sys;
}MPEG_PS_PRIV;



#ifdef __cplusplus
extern "C" {
#endif

int  md_ps_new( MPEG_DEMUXER **md_out  );
void md_ps_dispose( MPEG_DEMUXER *md );
int  ps_scan_header( MPEG_DEMUXER *md, unsigned char  *mpeg_bytes );
int  ps_validate( unsigned char *d, int size, int *is_ts, int *offset_out );

int ps_scan_header_x (unsigned char *mpeg_bytes, PS_PACK_HEADER_x *pakh_x, PS_SYSTEM_HEADER_x *sysh_x, PES_HEADER *pesh_x, int *pes_offset, int *pes_size );

#ifdef STAND_ALONE
int ps_get_duration( MPEG_DEMUXER	*md, unsigned char	*mpeg_all_bytes, int packet_total, TIME_64 *dur, TIME_64 *time_init );
int ps_to_box();
#endif

#ifdef __cplusplus
}
#endif
