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
#include "mpeg_x_demuxer.h"
#include "mpeg_ts_demuxer.h"

int ts_validate( unsigned char *d, int size, int *is_ts, int *offset_out )
{
	int offset = 0;
	int err = kFskErrUnknownElement;

	dlog("into ts_validate()\n");;
	
	*offset_out = 0;
	*is_ts = -1;

	if( size < TS_LENGTH*3 && size >= TS_LENGTH )
	{
		if( d[0] == MPEG_TS_SYNC_BYTE && d[1] != 'I' && d[2] != 'F' )
			err = kFskErrNone;

		goto bail;
	}
	else	//size >= 3*TS_LENGTH
	{
		while(1)
		{
			while( d[0] != MPEG_TS_SYNC_BYTE || d[TS_LENGTH] != MPEG_TS_SYNC_BYTE || d[TS_LENGTH*2] != MPEG_TS_SYNC_BYTE)
			{
				offset++;
				d++;
				if( offset >= TS_LENGTH )
					goto bail;
			}

			{ 
				TS_HEADER		th1, th2;
				PES_HEADER		ph1, ph2;
				unsigned char	*payload_start;
				int				payload_size;

				//extra check on ts header to make sure it looks legitimate
				err = parse_ts( d, &th1, &ph1, &payload_start, &payload_size );
				if( err == kFskErrNone )
				{
					err = parse_ts( d, &th2, &ph2, &payload_start, &payload_size );
					if( err == kFskErrNone )
					{
						*is_ts = 1;
						err = kFskErrNone;
						goto bail;
					}
				}
				else
					err = kFskErrUnknownElement;
			}

			d++;
		}
	}

bail:
	if( err == kFskErrNone )
		*offset_out = offset;

	dlog("out of ts_validate(), err: %d\n", (int)err);
	return err;
}


int parse_ts( unsigned char *p, TS_HEADER *th, PES_HEADER *ph, unsigned char **payload_start_out, int *payload_size_out )
{
	unsigned char *payload_start = p;
	int payload_size   = TS_LENGTH;
	int tmp;

	FskMemSet((void*)th, 0, sizeof( TS_HEADER));	
	FskMemSet((void*)ph, 0, sizeof(PES_HEADER));

	INVALIDATE_TIME(&th->program_clock_reference_base);
	INVALIDATE_TIME(&th->original_program_clock_reference_base);

	INVALIDATE_TIME(&ph->presentation_time_stamp);
	INVALIDATE_TIME(&ph->decoding_time_stamp);
	
	GET_LONG_BE( tmp, p );
	th->sync_byte					= (tmp>>24)&0x000000ff;
	th->transport_err_indicator		= (tmp>>23)&0x00000001;
	th->payload_unit_start_indicator= (tmp>>22)&0x00000001;
	th->transport_priporty			= (tmp>>21)&0x00000001;
	th->PID							= (tmp>> 8)&0x00001fff;
	th->transport_scrambling_control = (tmp>> 6)&0x00000003;
	th->adaption_field_control		= (tmp>> 4)&0x00000003;
	th->continuity_counter			= (tmp>> 0)&0x0000000f;
		
	if( th->sync_byte != MPEG_TS_SYNC_BYTE )
		return -1;

	payload_start  += 4;
	payload_size   -= 4;
    
    if (th->PID == 0x1fff) { //skip Null packet
        payload_size = 0;
        goto bail;
    }

	if( th->adaption_field_control == 0x02 || th->adaption_field_control == 0x03 )
	{
		GET_SHORT_BE( tmp, p );
		th->adaption_field_length				= (tmp>>8)&0x00ff;
		th->discontinuity_indicotor				= (tmp>>7)&0x0001;
		th->random_access_indicator				= (tmp>>6)&0x0001;
		th->elementary_stream_priority_indicator= (tmp>>5)&0x0001;
		th->pcr_flag								= (tmp>>4)&0x0001;
		th->opcr_flag							= (tmp>>3)&0x0001;
		th->splicing_point_flag					= (tmp>>2)&0x0001;
		th->transport_private_data_flag			= (tmp>>1)&0x0001;
		th->adaption_field_extension_flag			= (tmp>>0)&0x0001;

		payload_start += 1 + th->adaption_field_length;
		payload_size  -= 1 + th->adaption_field_length;
		if( payload_size < 0 || payload_size > TS_LENGTH )
			return -1;

		if( th->pcr_flag != 0 )
		{
			GET_LONG_BE( tmp, p );
			th->program_clock_reference_base.hi = (tmp>>31)&0x01;
			th->program_clock_reference_base.lo = tmp<<1;
			
			tmp = p[0]; p++;

			th->program_clock_reference_base.lo |= (tmp>>7)&0x01;
			th->program_clock_reference_base_extension = (tmp&0x01)<<8;
			
			tmp = p[0]; p++;
			th->program_clock_reference_base_extension |= tmp;

		}

		if( th->opcr_flag != 0 )
		{
			GET_LONG_BE( tmp, p );
			th->original_program_clock_reference_base.hi = (tmp>>31)&0x01;
			th->original_program_clock_reference_base.lo = tmp<<1;
			
			tmp = p[0]; p++;

			th->original_program_clock_reference_base.lo |= (tmp>>7)&0x01;
			th->original_program_clock_reference_base_extension = (tmp&0x01)<<8;
			
			tmp = p[0]; p++;
			th->original_program_clock_reference_base_extension |= tmp;
		}

		if( th->splicing_point_flag != 0 )
		{
			th->splice_countdown = p[0];
			p++;
		}

		if( th->transport_private_data_flag != 0 )
		{
			th->transport_private_data_length = p[0];
			p++;
			p+= th->transport_private_data_length;
		}

		if( th->adaption_field_extension_flag != 0 )
		{
			//my_umimplemented();
			th->adaption_field_extension_length = p[0];
			p++;
			tmp = p[0]; p++;
			th->ltw_flag				= (tmp>>7)&0x01;
			th->piecewise_rate_flag	= (tmp>>6)&0x01;
			th->seamless_splice_flag = (tmp>>5)&0x01;
			
			if( th->ltw_flag == 1 )
			{
				GET_SHORT_BE( tmp, p );
				th->ltw_valid_flag = (tmp>>15)&0x01;
				th->ltw_offset    = (tmp>>0) &0x7fff;	
			}

			if( th->piecewise_rate_flag == 1 )
			{
				GET_24BITS_BE( tmp, p );
				th->piecewise_rate = (tmp>>0) &0x003fffff;	
			}

			if( th->seamless_splice_flag == 1 )
			{
				int marker_bit;
				int dts_next_au_32_30;
				int dts_next_au_29_15;
				int dts_next_au_14_0;

				tmp = p[0]; p++;
				th->splice_type		= (tmp>>4)&0x0f;
				dts_next_au_32_30   = (tmp>>1)&0x07;
				marker_bit			= (tmp>>0) &0x01;	
				
				GET_SHORT_BE( tmp, p );
				dts_next_au_29_15   = (tmp>>1)&0x7fff;
				marker_bit			= (tmp>>0) &0x01;	

				GET_SHORT_BE( tmp, p );
				dts_next_au_14_0    = (tmp>>1)&0x7fff;
				marker_bit			= (tmp>>0) &0x01;	

				th->DTS_next_AU_bit32 = (dts_next_au_32_30>>2)&0x01;
				th->DTS_next_AU_bits  = (((dts_next_au_32_30)&0x03)<<30) |
										(((dts_next_au_29_15)     )<<15) |
										(((dts_next_au_14_0 )     )<<0 );
			}
		}
	}

	if( payload_start[0]== 0x00 && payload_start[1]== 0x00 && payload_start[2]== 0x01 )
	{
		int copy_offset = 0;
		int size_no_use = 0;
		
		//***bnie: in some cases payload_start may not contain a pes packet
		// but piece of a raw bit stream, in this case size_no_use is false
		parse_pes_packet( payload_start, ph, &copy_offset, &size_no_use );
		dlog( "parsed pec packet: stream_id: %x, pes_packet_length: %d, presentation_time_stamp: %d\n",
					(int)ph->stream_id, (int)ph->pes_packet_length, (int)ph->presentation_time_stamp.lo );

		payload_start += copy_offset;
		payload_size  -= copy_offset;	

		if( payload_size < 0 || payload_size > TS_LENGTH )
			return -1;
	}

bail:
	*payload_start_out = payload_start;
	*payload_size_out  = payload_size;

	return 0;
}


int parse_pat( unsigned char *p, int size, PAT *pat )
{
	int tmp;
	int i;
	int err = 0;

	tmp = p[0]; p+= 2;;
	pat->table_id = tmp;
	
	GET_SHORT_BE( tmp, p );
	pat->section_syntax_indicator	= (tmp>>15)&0x0001;
	my_assert( pat->section_syntax_indicator==1 );
	pat->always_0xx					= (tmp>>12)&0x0007;
	pat->section_length				= (tmp>> 0)&0x0fff;

	GET_SHORT_BE( tmp, p );
	pat->transport_stream_id			= tmp;

	tmp = p[0]; p++;
	pat->version_number				= (tmp>>1)&0x1f;
	pat->current_next_indicator		= (tmp>>0)&0x01;
	
	tmp = p[0]; p++;
	pat->section_number =tmp;
	
	tmp = p[0]; p++;
	pat->last_section_number =tmp;

	pat->session_total = (pat->section_length - 9)/4;
	if( pat->session_total > MAX_PROGRAM_NUM )
		pat->session_total = MAX_PROGRAM_NUM;

	for( i = 0; i < pat->session_total; i++ )
	{
		GET_SHORT_BE( tmp, p );
		pat->program_number[i]			= tmp;

		GET_SHORT_BE( tmp, p );
		pat->network_PID[i]				= tmp&0x1fff;
	}

	GET_LONG_BE( tmp, p );
	pat->CRC = tmp;

//bail:
	return err;
}


int is_associated_pid( PAT *pat, int pid )
{
	int i;

	for( i = 0; i < pat->session_total; i++ )
	{
		if( pat->network_PID[i] == pid )
			return 1;
	}

	return 0;
}


int parse_pmt( PAT *pat, unsigned char *p, int size, PMT *pmt )
{
	int tmp;
	int i, j, k, bytes_in_loop;
	int err = 0;

	tmp = p[0]; p+= 2;;
	pmt->table_id = tmp;
	
	GET_SHORT_BE( tmp, p );
	pmt->section_syntax_indicator	= (tmp>>15)&0x0001;
	my_assert( pmt->section_syntax_indicator==1 );
	pmt->always_0xx					= (tmp>>12)&0x0007;
	pmt->section_length				= (tmp>> 0)&0x0fff;

	GET_SHORT_BE( tmp, p );
	pmt->proram_number				= tmp;

	tmp = p[0]; p++;
	pmt->version_number				= (tmp>>1)&0x1f;
	pmt->current_next_indicator		= (tmp>>0)&0x01;
	
	tmp = p[0]; p++;
	pmt->section_number				= tmp;
	
	tmp = p[0]; p++;
	pmt->last_section_number		= tmp;

	GET_SHORT_BE( tmp, p );
	pmt->PRC_PID					= tmp&0x1fff;

	GET_SHORT_BE( tmp, p );
	pmt->program_info_length		= tmp&0x0fff;

	//pmt->session_total = ( pmt->section_length - 13 ) /(5 + pmt->program_info_length );
	//if( pmt->session_total > MAX_PROGRAM_NUM )
	//	pmt->session_total = MAX_PROGRAM_NUM;

	for (i = 0; i< pmt->program_info_length; i++) {
		GET_BYTE ( tmp, p );
	}

	bytes_in_loop = pmt->section_length - pmt->program_info_length - 13;
	for( i = 0, j = 0; j < bytes_in_loop; i ++)
	{
		tmp = p[0]; p++;
		pmt->stream_type[i]			= tmp;

		GET_SHORT_BE( tmp, p );
		pmt->elementary_PID[i]		= tmp&0x1fff;

		GET_SHORT_BE( tmp, p );
		pmt->ES_info_length[i]		= tmp&0x0fff;

		for (k = 0; k < pmt->ES_info_length[i]; k++) {
			GET_BYTE ( tmp, p );
		}

		j += 5 + pmt->ES_info_length[i];
	}

	pmt->session_total = i;

	GET_LONG_BE( tmp, p );
	pmt->CRC = tmp;

//bail:
	return err;
}


int pid_map_to_format( int pid, int total, int *type_ary, int *pid_ary )
{
	int i;

	for( i = 0; i < total; i++ )
	{
		if( pid_ary[i] == pid )
			return type_ary[i];
	}

	return 0;
}


void md_ts_dispose( MPEG_DEMUXER *md )
{
	if( md != NULL )
	{
		MPEG_TS_PRIV *priv = (MPEG_TS_PRIV *)md->priv;;
		
		if( priv != NULL )
			FskMemPtrDisposeAt( (void **)&priv );
		
		md_dispose(md);
	}
}


int  md_ts_new( MPEG_DEMUXER **md_out )
{
	MPEG_DEMUXER *md;
	MPEG_TS_PRIV *priv;
	int err = 0;

	dlog( "into md_ts_new()\n");
	
	err = md_new( &md );
	BAIL_IF_ERR( err );

	err = FskMemPtrNewClear( sizeof( MPEG_TS_PRIV ), (FskMemPtr *)&priv );
	BAIL_IF_ERR( err );

	md->priv = priv;

bail:
	if( err != 0 )
	{
		md_ts_dispose( md );
		md = NULL;
	}

	*md_out = md;

	return err;
}


int md_ts_parse_header( MPEG_DEMUXER *md, unsigned char  *mpeg_bytes )
{
	MPEG_TS_PRIV	*priv = (MPEG_TS_PRIV *)md->priv;
	unsigned char	*payload_start;
	int				payload_size;
	TS_HEADER		th;
	PES_HEADER		ph;
	int				err = 0;
	
	dlog( "\n" );
	dlog( "into md_ts_parse_header()\n");
	parse_ts( mpeg_bytes, &th, &ph, &payload_start, &payload_size );

	if( IS_PROGRAM_ASSOCIATION_TABLE(th.PID) && !priv->pat_initialized )
	{
		dlog( "IS_PROGRAM_ASSOCIATION_TABLE(th.PID) && !priv->pat_initialized\n");
		parse_pat( payload_start, payload_size, &priv->pat );
		priv->pat_initialized = 1;
	}
	else if ( is_associated_pid( &priv->pat, th.PID ) && !priv->pmt_initialized )
	{
		//int i = 0;

		dlog( "is_associated_pid( &priv->pat, th.PID ) && !priv->pmt_initialized\n");
		parse_pmt( &priv->pat, payload_start, payload_size, &priv->pmt );
		priv->pmt_initialized = 1;
#if 0
		for( i = 0; i < priv->pmt.session_total; i++ )
		{
			int format = priv->pmt.stream_type[i];

			if( IS_VIDEO_FORMAT(format) && md->video_header_initialized == 0 && priv->pat_initialized && priv->pmt_initialized )
			{
				md->video_elementary_pid	= priv->pmt.elementary_PID[i];
				md->video_format			= format;
			}
			else if(  IS_AUDIO_FORMAT(format) && md->audio_header_initialized == 0 && priv->pat_initialized && priv->pmt_initialized  )								
			{
				md->audio_elementary_pid	= priv->pmt.elementary_PID[i];;
				md->audio_format			= format;
			}
		}
#endif
	}
	else if( IS_VIDEO_STREAM( ph.stream_id ) && !md->video_header_initialized && priv->pat_initialized && priv->pmt_initialized )
	{
		dlog( "IS_VIDEO_STREAM( ph.stream_id ) && !md->video_header_initialized && priv->pat_initialized && priv->pmt_initialized\n");
		md->video_elementary_pid	= th.PID;
		md->video_format			= pid_map_to_format(md->video_elementary_pid, priv->pmt.session_total, priv->pmt.stream_type, priv->pmt.elementary_PID );
		err = md_parse_video_codec( md, payload_start, payload_size );
	}
	else if( IS_AUDIO_STREAM( ph.stream_id ) && !md->audio_header_initialized && priv->pat_initialized && priv->pmt_initialized )
	{
		dlog( "IS_AUDIO_STREAM( ph.stream_id ) && !md->audio_header_initialized && priv->pat_initialized && priv->pmt_initialized\n");
		md->audio_elementary_pid		= th.PID;		
		md->audio_format				= pid_map_to_format(md->audio_elementary_pid, priv->pmt.session_total, priv->pmt.stream_type, priv->pmt.elementary_PID );
		err = md_parse_audio_codec( md, payload_start, payload_size );
	}
	else if( IS_PRIVATE_STREAM( ph.stream_id ) && priv->pat_initialized && priv->pmt_initialized )
	{
		int elementary_format;
		
		elementary_format = pid_map_to_format(th.PID, priv->pmt.session_total, priv->pmt.stream_type, priv->pmt.elementary_PID );

		if (IS_AUDIO_FORMAT(elementary_format) && !md->audio_header_initialized)
		{
			md->audio_elementary_pid = th.PID;
			md->audio_format		 = elementary_format;
			err = md_parse_audio_codec( md, payload_start, payload_size );
		}
		else if (IS_VIDEO_FORMAT(elementary_format) && !md->video_header_initialized)
		{
			md->video_elementary_pid = th.PID;
			md->video_format		 = elementary_format;
			err = md_parse_video_codec( md, payload_start, payload_size );
		}
	}
	
	if( priv->pmt_initialized )
	{
		dlog( "priv->pmt_initialized\n");
		if( priv->pmt.session_total == 1 )
		{
			dlog( "priv->pmt.session_total == 1, md->audio_header_initialized/md->video_header_initialized: %d/%d\n",
					(int)md->audio_header_initialized, (int)md->video_header_initialized);
			if( md->audio_header_initialized || md->video_header_initialized )
				md->initialized = 1;
		}
		else if( priv->pmt.session_total >= 2 )
		{
			dlog( "priv->pmt.session_total >= 2, md->audio_header_initialized/md->video_header_initialized: %d/%d\n",
					(int)md->audio_header_initialized, (int)md->video_header_initialized);
			if( md->audio_header_initialized  &&  md->video_header_initialized )
				md->initialized = 1;
		}
	}	

//bail:
	dlog( "out of md_ts_parse_header()\n" );
	return err;
}


int md_ts_process_sample
( 
	MPEG_DEMUXER	*md, 
	int				use_raw, 
	unsigned char	*mpeg_bytes, 
	int				*is_video, 
	unsigned char	**bs_bytes_out, 
	int				*bs_size, 
	TIME_64			*time_pre,  
	TIME_64			*time_dec
)
{
	MPEG_TS_PRIV	*priv = (MPEG_TS_PRIV *)md->priv;
	unsigned char	*bs_bytes = NULL;
	unsigned char	*payload_start;
	int				payload_size;
	TS_HEADER		th;
	PES_HEADER		ph;
	FRAME_BUFFER	*tt;
	int err = 0;

	dlog( "\n" );
	dlog("into md_ts_process_sample()\n");;

	if( mpeg_bytes == NULL )
		return kFskErrBadData;
	
	*bs_size = 0;
	memset( (void *)&th, 0, sizeof( TS_HEADER ) );
	memset( (void *)&ph, 0, sizeof( PES_HEADER ) );
	
	err = parse_ts( mpeg_bytes, &th, &ph, &payload_start, &payload_size );
    if (err) goto bail;

	tt = get_fb_by_idx( md->fb_ary, th.PID, ph.stream_id );
	if( tt == NULL  ) goto bail;

	INVALIDATE_TIME(time_pre);
	INVALIDATE_TIME(time_dec);

    if (
        IS_VIDEO_STREAM( tt->stream_id ) &&
        tt->size != 0					&&
        tt->has_frame )
    {
        //two pics within one PES packet
        int this_size = 0;
        int skip_bytes = 0;

        dlog("has_frame in this PES packet\n");
        *is_video = 2;

        err = FskMemPtrNew(tt->size + 64, &bs_bytes);
        BAIL_IF_ERR( err );

        if (use_raw)
        {
            ; //not implement yet!
        }
        else
        {
            err = startcode2lenskip( tt->buf, tt->size, bs_bytes, &this_size, &skip_bytes );
            dlog("converted to QT style box format, tt->size: %d, this_size: %d\n", (int)tt->size, (int)this_size);
            if( err != 0 || this_size == 0 || skip_bytes >= tt->size)
            {
                if( bs_bytes != NULL )
                {
                    FskMemPtrDisposeAt( (void **)&bs_bytes );
                    bs_bytes = NULL;
                    err = 0;	//***bnie: 4/12/2010, for error resilience
                }
                rest_frame_buffer(tt);
                goto bail;
            }

            *bs_size = this_size;
            *time_pre = tt->time_pre;
            *time_dec = tt->time_dec;

            refit_frame_buffer(tt, skip_bytes);

            if (tt->size >= 6) {
                err = checkframestart( tt->buf, tt->size, &tt->has_frame );
                BAIL_IF_ERR( err );
                if ( tt->has_frame || err)
                    dlog("if there are extra frame in PES packet, has frame = %d", tt->has_frame);
            }
            goto bail;
        }
    }
    else if(
           IS_VIDEO_STREAM( ph.stream_id ) &&
           tt->size != 0					&&
           (
            IS_DIFFERENT_TIME(&tt->time_pre, &ph.presentation_time_stamp) ||
            IS_DIFFERENT_TIME(&tt->time_dec, &ph.decoding_time_stamp)
            )
           )
    {
        int this_size = 0;

        dlog("IS_VIDEO_STREAM( ph.stream_id ), tt->size: %d\n", (int)tt->size);

        //my_assert( (payload_start[0]==0x00 && payload_start[1]==0x00 && payload_start[2]==0x00 && payload_start[3]==0x01 &&
        //			payload_start[4]==0x09  ));

        *is_video = 1;

        err = FskMemPtrNew(tt->size + 64, &bs_bytes);
        BAIL_IF_ERR( err );

        if( use_raw )
        {//save raw bitstream
            this_size = tt->size;
            FskMemCopy( bs_bytes, tt->buf, this_size );
        }
        else
        {//convert to QT style box format
            err = startcode2len( tt->buf, tt->size, bs_bytes, &this_size );
            dlog("converted to QT style box format, tt->size: %d, this_size: %d\n", (int)tt->size, (int)this_size);
            if( err != 0 || this_size == 0 )
            {
                if( bs_bytes != NULL )
                {
                    FskMemPtrDisposeAt( (void **)&bs_bytes );
                    bs_bytes = NULL;
                    err = 0;	//***bnie: 4/12/2010, for error resilience
                }
                rest_frame_buffer(tt);
                goto bail;
            }
        }

        *bs_size = this_size;
        *time_pre = tt->time_pre;
        *time_dec = tt->time_dec;

        rest_frame_buffer(tt);
    }
	else if( IS_AUDIO_STREAM( ph.stream_id ) && tt->size != 0 )
	{
		unsigned char *src = tt->buf;
		int			   src_size = tt->size;
		int				audio_frame_size = 0;

		dlog( "IS_AUDIO_STREAM( ph.stream_id ) && tt->size: %d\n", tt->size );
		*is_video = 0;

		audio_frame_size = src_size;// - md->audio_sample_offset;
		if( audio_frame_size > 0 && IS_VALID_TIME( &tt->time_pre ) )
		{
			//src += md->audio_sample_offset;
			err = FskMemPtrNew(tt->size + 16, &bs_bytes);
			BAIL_IF_ERR(err);

			if( bs_bytes == NULL ) { err = -1;  goto bail; }

			FskMemCopy( bs_bytes, src, audio_frame_size );

			*bs_size = audio_frame_size;
			*time_pre = tt->time_pre;
			*time_dec = tt->time_dec;
			
			if( IS_VALID_TIME(time_pre) && !IS_VALID_TIME(time_dec) )
				*time_dec = *time_pre;
		}

		rest_frame_buffer(tt);
	}
	else if( IS_PROGRAM_ASSOCIATION_TABLE(th.PID)  ||  is_associated_pid( &priv->pat, th.PID ) )
	{
		dlog( "IS_PROGRAM_ASSOCIATION_TABLE(th.PID)  ||  is_associated_pid( &priv->pat, th.PID )\n" );
		goto bail;
	}

	if( !IS_VALID_TIME( &tt->time_dec) && IS_VALID_TIME( &ph.decoding_time_stamp ) )
		tt->time_dec = ph.decoding_time_stamp;
	
	if( !IS_VALID_TIME( &tt->time_pre) && IS_VALID_TIME( &ph.presentation_time_stamp) )
		tt->time_pre = ph.presentation_time_stamp;

	if( payload_size > 0 )
	{
		int total_data_size = payload_size + tt->size;

		if( total_data_size > tt->buf_size )
		{
			unsigned char *new_buf;
			
			if( total_data_size < MIN_FRAME_SAMPLE_BUFFER_SIZE )
				total_data_size = MIN_FRAME_SAMPLE_BUFFER_SIZE;

			total_data_size = total_data_size + (total_data_size>>2);//make it 25% bigger

			dlog( "resize sample buffer, old size: %d, new size: %d\n", (int)tt->buf_size, (int)total_data_size );

			err = FskMemPtrNew(total_data_size, &new_buf);
			BAIL_IF_ERR(err);

			if( tt->size > 0 )
				FskMemCopy( new_buf, tt->buf, tt->size );

			if( tt->buf != NULL )
				FskMemPtrDisposeAt( (void **)&tt->buf );

			tt->buf = new_buf;
			tt->buf_size = total_data_size;
		}
		
		FskMemCopy( &tt->buf[tt->size], payload_start, payload_size );
		dlog( "appended new payload data to sample, payload_size: %d, sample size: %d\n", (int)payload_size,  (int)tt->size );
		tt->size += payload_size;

		if ( IS_VIDEO_STREAM( tt->stream_id ) && tt->size >= 6)
		{
			err = checkframestart( tt->buf, tt->size, &tt->has_frame );
			BAIL_IF_ERR( err );
            if ( tt->has_frame )
                dlog("if there are extra frame in PES packet, has frame = %d", tt->has_frame);
		}
	}

bail:
	dlog("exit md_ts_process_sample()\n");;

	*bs_bytes_out = bs_bytes;

	return err;
}


int md_ts_get_sample_time( MPEG_DEMUXER *md, unsigned char  *mpeg_bytes, int *is_video, int *is_audio, TIME_64 *time_pre, TIME_64 *time_dec, int *offset, int *size )
{
	MPEG_TS_PRIV	*priv = (MPEG_TS_PRIV *)md->priv;
	unsigned char	*payload_start;
	int				payload_size;
	TS_HEADER		th;
	PES_HEADER		ph;
	int err = 0;

	*is_video = 0;
	*is_audio = 0;
	*offset = 0;
	*size   = 0;

	parse_ts( mpeg_bytes, &th, &ph, &payload_start, &payload_size );
	
	if( IS_PROGRAM_ASSOCIATION_TABLE(th.PID) && !priv->pat_initialized )
	{
		dlog( "IS_PROGRAM_ASSOCIATION_TABLE(th.PID) && !priv->pat_initialized\n");
		parse_pat( payload_start, payload_size, &priv->pat );
		priv->pat_initialized = 1;
	}
	else if ( is_associated_pid( &priv->pat, th.PID ) && !priv->pmt_initialized )
	{		
		dlog( "is_associated_pid( &priv->pat, th.PID ) && !priv->pmt_initialized\n");
		parse_pmt( &priv->pat, payload_start, payload_size, &priv->pmt );
		priv->pmt_initialized = 1;
	}

	if( IS_VIDEO_STREAM( ph.stream_id ) )
	{
		//my_assert( (payload_start[0]==0x00 && payload_start[1]==0x00 && payload_start[2]==0x00 && payload_start[3]==0x01 && 
		//			payload_start[4]==0x09  ));
		
		*is_video = 1;
		*is_audio = 0;

		*time_pre = ph.presentation_time_stamp;
		*time_dec = ph.decoding_time_stamp;
	}
	else if( IS_AUDIO_STREAM( ph.stream_id ) )
	{
		*is_video = 0;
		*is_audio = 1;

		*time_pre = ph.presentation_time_stamp;
		*time_dec = ph.decoding_time_stamp;		
	}
	else if( priv->pat_initialized && priv->pmt_initialized )
	{
		int elementary_format;
		elementary_format = pid_map_to_format(th.PID, priv->pmt.session_total, priv->pmt.stream_type, priv->pmt.elementary_PID );
		
		if (IS_AUDIO_FORMAT(elementary_format))
			*is_audio = 1;
	}
	else
	{
		INVALIDATE_TIME( time_pre );
		INVALIDATE_TIME( time_dec );
	}

	*offset = payload_start - mpeg_bytes;
	*size   = payload_size;
	
//bail:

	return err;
}


#ifdef STAND_ALONE

static int get_nalu_size( unsigned char *data, int nalu_len_size )
{
    unsigned char *s = (unsigned char *)data;
    int			  src_size;

    if( nalu_len_size == 4 )
        src_size = (s[0]<<24)|(s[1]<<16)|(s[2]<<8)|(s[3]);
    else if(  nalu_len_size == 2 )
        src_size = (s[0]<<8)|(s[1]<<0);
    else
        src_size = s[0];

    return src_size;
}

static int size_to_start_code(unsigned char *data, int data_size, int start_code_type, int nalu_len_size)
{
    unsigned char *p = data;
    int size = 0, used_bytes = 0;

    if (start_code_type == 4) {
        while (1) {
            size = get_nalu_size(p, nalu_len_size);
            p[0] = 0x0;
            p[1] = 0x0;
            p[2] = 0x0;
            p[3] = 0x1;
            p += 4 + size;
            used_bytes += 4 + size;
            if (used_bytes >= data_size) {
                break;
            }
        }
    }
    else {
        ;//not implemented!
    }

    return used_bytes;
}


int ts_get_duration( MPEG_DEMUXER	*md, unsigned char	*mpeg_all_bytes, int packet_total, TIME_64 *dur, TIME_64 *time_init )
{
	TIME_64 min_pre, min_dec;
	TIME_64 max_pre, max_dec;
	int is_video = 0, is_audio = 0;
	int i;
	int sample_offset, sample_size;
	int err = 0;

	INVALIDATE_TIME( &min_pre );
	INVALIDATE_TIME( &min_dec );
	INVALIDATE_TIME( &max_pre );
	INVALIDATE_TIME( &max_dec );
	INVALIDATE_TIME( dur );
	INVALIDATE_TIME( time_init );

	for( i = 0; i < packet_total; i++ )
	{
		unsigned char	*mpeg_bytes = mpeg_all_bytes + i*TS_LENGTH;
		
		err = md_ts_get_sample_time( md, mpeg_bytes, &is_video, &is_audio, &min_pre, &min_dec, &sample_offset, &sample_size);
		if( err != 0 ) goto bail;

		if( IS_VALID_TIME( &min_pre ) || IS_VALID_TIME( &min_dec ) )
			break;
	}

	for( i = packet_total-1; i >= 0; i-- )
	{
		unsigned char	*mpeg_bytes = mpeg_all_bytes + i*TS_LENGTH;
		int				this_is_video;
		
		err = md_ts_get_sample_time( md, mpeg_bytes, &this_is_video, &is_audio, &max_pre, &max_dec, &sample_offset, &sample_size);
		if( err != 0 ) 
			goto bail;
		
		//we only care as long as they are the same media
		if( this_is_video == is_video && ( IS_VALID_TIME( &max_pre) || IS_VALID_TIME( &max_dec) ) )
			break;
	}

	if( IS_VALID_TIME(&min_dec) && IS_VALID_TIME(&max_dec) )
	{
		DIFF_TIME_64_ABS(dur, &max_dec, &min_dec);
		*time_init = min_dec;
	}
	else if( IS_VALID_TIME(&min_pre) && IS_VALID_TIME(&max_pre) )
	{
		DIFF_TIME_64_ABS(dur, &max_pre, &min_pre);
		*time_init = min_pre;
	}

bail:
	return err;
}

//#define ADTS_ONLY //parse aac/adts files only!

#ifdef ADTS_ONLY
extern int parse_adts_header( unsigned char *s, int *sizeOut, unsigned char *p, int *offset,  int *frame_size, int *sample_rate, int *channel_total, int *profile, int *level, int *bitrate, int frame_info_only );
#endif

int ts_to_box()
{
	char			src_path[256];
	FILE			*src_file	= NULL;
	FILE			*video_file=NULL;
	FILE			*audio_file=NULL;
	unsigned char	*mpeg_all_bytes = NULL;
	int				packet_total;
	int				src_size;
	int				i;
	int				err = 0;
	FskInt64        dur;
	FskInt64        time_init;
    
    TIME_64         t_dur;
    TIME_64         t_time_init;
	
	MPEG_DEMUXER	*md;
	MPEG_TS_PRIV	*priv;
    int				fsk_time;
    int				cmp_time;
    int             video_frame_got = 0;
	//int				track_total = 0;
	
	if(video_file == NULL )
		video_file = fopen("/Users/marvell/work/WebShare/mpeg_video_mpg.box", "wb");
	if( video_file == NULL )
		return -1;
	if(audio_file == NULL )
		audio_file = fopen("/Users/marvell/work/WebShare/mpeg_audio_mpg.box", "wb");
	if( audio_file == NULL )
		return -1;
	
	strcpy( src_path, "/Users/marvell/work/WebShare/" );
	strcat( src_path, "bryan_dump.ts" );
	// strcat( src_path, "media-ume3lpa2t_b750000_2449.ts" );
	// strcat( src_path, "fileSequence0.ts" );
    
	src_file = fopen(src_path, "rb");
	if( src_file == NULL )
		return -1;

	fseek(src_file, 0, SEEK_END);
	src_size = ftell(src_file);
	fseek(src_file, 0, SEEK_SET);
	
	err = FskMemPtrNewClear(src_size, &mpeg_all_bytes);
	if( err != 0 ) goto bail;

	err = md_ts_new( &md );
	BAIL_IF_ERR(err);
	
	priv = md->priv;

	fread( mpeg_all_bytes, 1, src_size, src_file );
	packet_total = src_size/TS_LENGTH;
	
#ifdef ADTS_ONLY
	{
		int offset, audio_frame_size, audio_samplerate, audio_channel_total, audio_profile, audio_level, audio_bitrate;
		err = parse_adts_header( mpeg_all_bytes, &src_size, mpeg_all_bytes, &offset, &audio_frame_size, &audio_samplerate, &audio_channel_total, &audio_profile, &audio_level, &audio_bitrate, 0 );
		return err;
	}
#endif
	
	{
		ts_get_duration( md, mpeg_all_bytes, packet_total, &t_dur, &t_time_init );
        
        if( IS_VALID_TIME( &t_dur) )
            dur	= (FskInt64)GET_TIME_64(&t_dur);
        if( IS_VALID_TIME( &t_time_init) )
            time_init = (FskInt64)GET_TIME_64(&t_time_init);
	}

	for( i = 0; i < packet_total; i++ )
	{
		unsigned char	*mpeg_bytes = mpeg_all_bytes + i*TS_LENGTH;
		
		err = md_ts_parse_header( md, mpeg_bytes );
		if( err != 0 ) goto bail;

		if( md->initialized )
			break;
	}

	if( md->video_header_initialized )
	{
        int spsppsSize = md->video_codec_header_data_size;
        //		fwrite( &spsppsSize,  1, 4, video_file);
        size_to_start_code(md->video_codec_header_data, spsppsSize, 4, 4);
        fwrite( md->video_codec_header_data,  1, spsppsSize, video_file);
	}

	if( md->audio_header_initialized )
	{
		long signature = 'esds';
						
		fwrite(&signature,  4,  1, audio_file);
		fwrite(&md->audio_codec_header_data_size,  4,  1, audio_file);
		fwrite(md->audio_codec_header_data, md->audio_codec_header_data_size, 1, audio_file);
		fflush(audio_file);
	}

	for( i = 0; i < packet_total; i++ )
	{
		unsigned char	*mpeg_bytes = mpeg_all_bytes + i*TS_LENGTH;
		int				is_video = 0;
		unsigned char	*bs_bytes = NULL;
		int				bs_size;
		TIME_64			time_pre;
		TIME_64			time_dec;
		
		err = md_ts_process_sample( md, 0, mpeg_bytes, &is_video, &bs_bytes, &bs_size, &time_pre, &time_dec );
		if( err != 0 ) goto bail;

        if (is_video == 2 && i > 0) {
            i --;
        }

		if( is_video  && bs_size != 0 )
		{
            if (IS_VALID_TIME(&time_dec))
                fsk_time = GET_TIME_64(&time_dec) - time_init;// /(double)mpeg->md.video_timescale;
            if (IS_VALID_TIME(&time_pre))
                cmp_time = GET_TIME_64(&time_pre) - time_init;
            else
                cmp_time = -1;
            
            video_frame_got ++;
            if (video_frame_got == 7400)
                video_frame_got = 7400;
            
printf("outputting one V frame, size = %d\n", bs_size);

//			fwrite( &bs_size,  1, 4, video_file);
            size_to_start_code(bs_bytes, bs_size, 4, 4);
			fwrite( bs_bytes,  1, bs_size, video_file);
			fflush(video_file);
		}
		else if( bs_size != 0 )
		{
			int offset;

			err = divide_adts( (unsigned char *)bs_bytes, (int)bs_size, (int *)&md->frame_total, (int)offset, (int *)md->frame_sizes, NULL );

			fwrite(&bs_size,  4, 1, audio_file);
			fwrite(bs_bytes, bs_size, 1, audio_file );
			fflush(audio_file);
		}

		if( bs_bytes != NULL )
			FskMemPtrDisposeAt( (void **)&bs_bytes );
	}

	//if( 
	//	same_files( "C:\\test_avc\\mpeg_video_avc.box", "C:\\test_avc\\mpeg_video_avc.ref.box" ) 
	//	&&
	//	same_files( "C:\\test_avc\\mpeg_audio_aac.box", "C:\\test_avc\\mpeg_audio_aac.ref.box" )
	//)
	//{
	//	dlog("VVV!!!\n");
	//}
	//else
	//{
	//	dlog("XXX!!!\n");
	//}

bail:
	md_dispose( md );

    dlog("exit successfully!\n")
	return err;
}

#endif
