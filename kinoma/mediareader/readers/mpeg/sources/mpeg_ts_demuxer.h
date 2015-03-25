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
typedef struct 
{
	int sync_byte;
	int transport_err_indicator;
	int payload_unit_start_indicator;
	int transport_priporty;
	int PID;
	int transport_scrambling_control;
	int adaption_field_control;
	int continuity_counter;
	
	//***adaptation field
	int adaption_field_length;
	int discontinuity_indicotor;
	int random_access_indicator;
	int elementary_stream_priority_indicator;
	
	int pcr_flag;
	int opcr_flag;
	int splicing_point_flag;
	int transport_private_data_flag;
	int adaption_field_extension_flag;

	TIME_64 program_clock_reference_base;
	int program_clock_reference_base_extension;
	
	TIME_64 original_program_clock_reference_base;
	int original_program_clock_reference_base_extension;
	
	int	splice_countdown;

	int transport_private_data_length;
	
	int adaption_field_extension_length;
	int	ltw_flag;
	int piecewise_rate_flag;
	int seamless_splice_flag;
	int ltw_valid_flag;
	int ltw_offset;
	int piecewise_rate;
	int splice_type;
	int DTS_next_AU_bit32;
	int DTS_next_AU_bits;

	void *optional_field;
}TS_HEADER;


typedef struct 
{
	int table_id;
	int section_syntax_indicator;
	int always_0xx;
	int section_length;
	int transport_stream_id;
	int	two_bits;
	int version_number;
	int current_next_indicator;
	int section_number;
	int last_section_number;
	int program_number[MAX_PROGRAM_NUM];
	int network_PID[MAX_PROGRAM_NUM];
	int session_total;
	int	CRC;
}PAT;


typedef struct 
{
	int table_id;
	int section_syntax_indicator;
	int always_0xx;
	int section_length;
	int proram_number;
	int	two_bits;
	int version_number;
	int current_next_indicator;
	int section_number;
	int last_section_number;
	int	PRC_PID;
	int program_info_length;
	int stream_type[MAX_PROGRAM_NUM];
	int elementary_PID[MAX_PROGRAM_NUM];
	int ES_info_length[MAX_PROGRAM_NUM];
	int session_total;
	int	CRC;
}PMT;

typedef struct 
{
	int				pat_initialized;
	PAT				pat;
	int				pmt_initialized;
	PMT				pmt;
}MPEG_TS_PRIV;


#ifdef __cplusplus
extern "C" {
#endif

int  md_ts_new( MPEG_DEMUXER **md_out );
void md_ts_dispose( MPEG_DEMUXER *md );
int  md_ts_scan_header( MPEG_DEMUXER *md, unsigned char  *mpeg_bytes );
int  md_ts_get_sample_time( MPEG_DEMUXER *md, unsigned char  *mpeg_bytes, int *is_video, int *is_audio, TIME_64 *pts, TIME_64 *dts, int *offset, int *size );
int  md_ts_process_sample( MPEG_DEMUXER *md, int use_raw, unsigned char  *mpeg_bytes, int *is_video, unsigned char **bs_bytes_out, int *bs_size, TIME_64 *pts, TIME_64 *dts );

int ts_validate( unsigned char *d, int size, int *is_ts, int *offset_out );

int md_ts_parse_header( MPEG_DEMUXER *md, unsigned char  *mpeg_bytes );

int parse_ts( unsigned char *p, TS_HEADER *th, PES_HEADER *ph, unsigned char **payload_start_out, int *payload_size_out );
int md_ts_parse_header( MPEG_DEMUXER *md, unsigned char  *mpeg_bytes );


#ifdef STAND_ALONE
int ts_get_duration( MPEG_DEMUXER	*md, unsigned char	*mpeg_all_bytes, int packet_total, TIME_64 *dur, TIME_64 *time_init );
int ts_to_box();
#endif

#ifdef __cplusplus
}
#endif
