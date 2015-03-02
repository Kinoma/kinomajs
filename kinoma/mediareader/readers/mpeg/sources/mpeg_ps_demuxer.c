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
#include "mpeg_ps_demuxer.h"

int parse_ps_pack_header_x( unsigned char *p, PS_PACK_HEADER_x *ph, int *offset )
{
	int tmp;
	int mark, reserved;
	int half_hi, half_lo;
	int err = 0;

	*offset = 0;

	FskMemSet((void*)ph, 0, sizeof( PS_PACK_HEADER_x));	
	
	GET_LONG_BE( tmp, p );

	ph->packet_start_code = tmp;
	if( ph->packet_start_code != PS_PACKET_START_CODE )
		 goto bail;
	
	GET_LONG_BE( tmp, p );
	
	if ( (mark = (tmp>>30)&0x03) == 0x01 )
	{//mpeg2
		ph->system_clock_reference_base_2 = (tmp>>27)&0x07;
		
		mark = (tmp>>26)&0x01;
		my_assert( mark == 0x01 );
		
		ph->system_clock_reference_base_1 = (tmp>>11)&0x7fff;
		
		mark = (tmp>>10)&0x01;
		my_assert( mark == 0x01 );
		
		half_hi = (tmp>>0)&0x03ff;
		
		GET_SHORT_BE( tmp, p );
		half_lo = (tmp>11)&0x1f;
		ph->system_clock_reference_base_0 = (half_hi<<5)|half_lo;
		
		mark = (tmp>>10)&0x01;
		my_assert( mark == 0x01 );
		
		ph->system_color_reference_extension = (tmp>>1)&0x01ff;
		
		mark = (tmp>>0)&0x01;
		my_assert( mark == 0x01 );
		
		GET_LONG_BE( tmp, p );
		
		ph->program_mux_rate = (tmp>>10)&0x3fffff;
		
		mark = (tmp>>9)&0x01;
		my_assert( mark == 0x01 );
		
		mark = (tmp>>8)&0x01;
		my_assert( mark == 0x01 );
		
		reserved = (tmp>>3)&0x1f;
		
		ph->program_stuffing_length = (tmp>>0)&0x03;
		
		ph->is_valid = 1;
		*offset = PACKET_HEADER_LENGTH + ph->program_stuffing_length;
	}
	else if ( (mark = (tmp>>28)&0x0f) == 0x02 ) //PF.J return err when detect MPEG1 system type
	{//mpeg1
		ph->system_clock_reference_base_2 = (tmp>>25)&0x07;
		
		mark = (tmp>>24)&0x01;
		my_assert( mark == 0x01 );
		
		ph->system_clock_reference_base_1 = (tmp>>9)&0x7fff;
		
		mark = (tmp>>8)&0x01;
		my_assert( mark == 0x01 );
		
		half_hi = (tmp>>0)&0xff;
		
		GET_LONG_BE( tmp, p );
		half_lo = (tmp>>25)&0x7f;
		ph->system_clock_reference_base_0 = (half_hi<<7)|half_lo;
		
		mark = (tmp>>24)&0x01;
		my_assert( mark == 0x01 );
		
		mark = (tmp>>23)&0x01;
		my_assert( mark == 0x01 );
		
		ph->program_mux_rate = (tmp>>1)&0x3fffff;
		
		mark = (tmp>>0)&0x01;
		my_assert( mark == 0x01 );
		
		ph->is_valid = 1;
		*offset = PACK_HEADER_LENGTH_MPEG1;
	}

	//my_assert( mark == 0x02 );

bail:
	return err;
}


int parse_ps_system_header_x( unsigned char *p, PS_SYSTEM_HEADER_x *sh, int *offset )
{
	int tmp;
	int mark, reserved;
	//int half_hi, half_lo;
	//int rest_size = 0;
	int /*max_stream_total, */i;
	int err = 0;

	*offset = 0;

	FskMemSet((void*)sh, 0, sizeof( PS_SYSTEM_HEADER_x));	

	GET_LONG_BE( tmp, p );
	sh->system_header_start_code = tmp;
	if( sh->system_header_start_code != SYSTEM_HEADER_START_START_CODE )
		goto bail;

	GET_SHORT_BE( tmp, p );
	sh->header_length = tmp;

	GET_LONG_BE( tmp, p );
	
	mark = (tmp>>31)&0x01;
	my_assert( mark == 0x01 );		//***jpf: this must be true

	sh->rate_bound = (tmp>>9)&0x3fffff;

	mark = (tmp>>8)&0x01;
	my_assert( mark == 0x01 );		//**jpf: this must be true

	sh->audio_bound = (tmp>>2)&0x3f;
	sh->fixed_flag	= (tmp>>1)&0x01;
	sh->CSPS_flag	= (tmp>>0)&0x01;

	GET_BYTE( tmp, p );

	sh->system_audio_clock_flag = (tmp>>7)&0x01;
	sh->system_video_clock_flag = (tmp>>6)&0x01;

	mark = (tmp>>5)&0x01;
	my_assert( mark == 0x01 );		//***jpf: this must be true
	
	sh->video_bound = (tmp>>0)&0x1f;

	GET_BYTE( tmp, p );

	sh->packet_rate_restriction_flag = (tmp>>7)&0x01;
	reserved = (tmp>>0)&0x3f;
	my_assert( reserved == 0x3f );		

	//rest_size = sh->header_length - SYSTEM_HEADER_AFTER_LENGTH_LENGTH;
	//*offset   = sh->header_length + SYSTEM_HEADER_BEFORE_LENGTH_LENGTH;

	//max_stream_total = rest_size/SYSTEM_HEADER_STREAM_SPEC_LENGTH;
	
	SHOW_BYTE(tmp, p);
	i = 0;
	while ( tmp&0x80 ) //for( i = 0; i < max_stream_total; i++ )
	{
		int tmp1, tmp2;
		PS_SYSTEM_HEADER_STREAM_SPEC_x *ss = &sh->stream_spec[i];
		
		GET_BYTE( tmp1, p );
		if( (tmp1>>7)!= 1 )
			break;
		
		sh->stream_total++;
		if( sh->stream_total > MAX_STREAM_TOTAL )
			break;

		ss->stream_id = tmp1;

		GET_SHORT_BE( tmp2, p );

		mark = (tmp2>>14)&0x03;
		my_assert( mark == 0x03 );

		ss->P_STD_buffer_bound_scale = (tmp2>>13)&0x01;
		ss->P_STD_buffer_size_bound  = (tmp2>>0)&0x1fff;
		
		if ((++i) >= MAX_STREAM_TOTAL)
			break; //definitely no such many streams!!!
		
		SHOW_BYTE(tmp, p);
	}

	sh->is_valid = 1;
	*offset =  sh->stream_total*SYSTEM_HEADER_STREAM_SPEC_LENGTH + SYSTEM_HEADER_BEFORE_LENGTH_LENGTH + SYSTEM_HEADER_AFTER_LENGTH_LENGTH;

bail:
	return err;
}



int md_ps_new( MPEG_DEMUXER **md_out  )
{
	MPEG_DEMUXER *md;
	MPEG_PS_PRIV *priv;
	//int i;
	int err = 0;

	err = md_new( &md );
	BAIL_IF_ERR( err );

	err = FskMemPtrNewClear( sizeof( MPEG_PS_PRIV ), (FskMemPtr *)&priv );
	BAIL_IF_ERR( err );

	md->priv = priv;

bail:
	if( err != 0 )
	{
		md_dispose( md );
		md = NULL;
	}

	*md_out = md;

	return err;
}


void md_ps_dispose( MPEG_DEMUXER *md )
{
	//int err = 0;

	if( md != NULL )
	{
		MPEG_PS_PRIV *priv = (MPEG_PS_PRIV *)md->priv;;
		
		if( priv != NULL )
			FskMemPtrDisposeAt( (void **)&priv );

		md_dispose( md );
	}
}


int ps_validate( unsigned char *d, int size, int *is_ts, int *offset_out )
{
	int offset = 0;
	int err = kFskErrUnknownElement;
	
	dlog("into ps_validate()\n");;
	
	*offset_out = 0;
	*is_ts = -1;

#define PROGRAM_STREAM_MIN_BYTES_TO_VALIDATE	256*2
#define PROGRAM_STREAM_CODE		0xba
	if( size < PROGRAM_STREAM_MIN_BYTES_TO_VALIDATE && d[0] == 0x00 &&  d[1] == 0x00 && d[2] == 0x01 && d[3] == PROGRAM_STREAM_CODE )
	{
		*is_ts = 0;
		err = kFskErrNone;
		dlog("validated ps!\n");;
		goto bail;
	}
	else
	{
		dlog("shifting bytes to find PROGRAM_STREAM_CODE...\n");;
		while(1)
		{
			while( 
					d[0] != 0x00 || 
					d[1] != 0x00 ||
					d[2] != 0x01 ||
					d[3] != PROGRAM_STREAM_CODE
				 )
			{
				offset++;
				d++;
				if( offset >= size - 4 )
				{
					dlog("reached end and no PROGRAM_STREAM_CODE found, invalidated!!!\n");;
					goto bail;
				}
			}

			dlog( "more check can invalidate this and make it check further\n");
			{
				PS_PACK_HEADER_x	pach_x;
				PS_SYSTEM_HEADER_x	sysh_x;
				PES_HEADER			pesh_x;
				int pes_offset = 0;
				int pes_size   = 0;
				int this_err = 0;

				FskMemSet((void*)&pesh_x, 0, sizeof( PES_HEADER));	

				this_err = ps_scan_header_x(  d,  &pach_x, &sysh_x, &pesh_x, &pes_offset,  &pes_size );
				BAIL_IF_ERR( this_err );

				if( sysh_x.is_valid )
				{		
					*is_ts = 0;
					err = kFskErrNone;
					dlog( "header looks right, validate ps!\n");
					goto bail;
				}
			}

			d += 4;
			offset += 4;
		}
	}

bail:
	if( err == kFskErrNone )
		*offset_out = offset;

	return err;
}


int ps_scan_header_x
( 
	unsigned char		*mpeg_bytes, 
	PS_PACK_HEADER_x	*pakh_x,
	PS_SYSTEM_HEADER_x	*sysh_x,
	PES_HEADER			*pesh_x,
	int					*pes_offset, 
	int					*pes_size 
)
{
	unsigned char   *mpeg_bytes_in = mpeg_bytes;
	//unsigned char	*payload_start;
	//int				payload_size;
	int				offset = 0;
	int				size = 0;
	int				err = 0;

	FskMemSet((void*)pakh_x, 0, sizeof(PS_PACK_HEADER_x));
	FskMemSet((void*)sysh_x, 0, sizeof(PS_SYSTEM_HEADER_x));
	FskMemSet((void*)pesh_x, 0, sizeof( PES_HEADER));	
	
	INVALIDATE_TIME( &pesh_x->presentation_time_stamp );
	INVALIDATE_TIME( &pesh_x->decoding_time_stamp );
	
	//TODO: mpeg1 system parsing, which diff from mpeg2!!!

	err = parse_ps_pack_header_x( mpeg_bytes, pakh_x, &offset );
	BAIL_IF_ERR( err );

	mpeg_bytes += offset;

	err = parse_ps_system_header_x( mpeg_bytes, sysh_x, &offset );
	BAIL_IF_ERR( err );

	mpeg_bytes += offset;
	err = parse_pes_packet( mpeg_bytes, pesh_x, &offset, &size );
	BAIL_IF_ERR( err );

	mpeg_bytes += offset;
	
	*pes_offset = mpeg_bytes - mpeg_bytes_in;
	*pes_size = size;

bail:

	return err;
}

static int check_audio_fomat(unsigned char  *mpeg_bytes, int *format, int bs_size, int *offset)
{
	int err = 0;
	unsigned char *bs_bytes = mpeg_bytes;
	
	*offset = 0;

#define TRACK_AC3_PS 0x82
	while (bs_bytes < mpeg_bytes + bs_size)
	{
		if (bs_bytes[0]==0xff && (bs_bytes[1]==0xfd || bs_bytes[1]==0xfc)) {
			*format = TRACK_MPEGA;
			break;
		}
		else if (bs_bytes[0]==0xb && bs_bytes[1]==0x77) {
			*format = TRACK_AC3_PS; //ac3 parse in ps is not ready.
			break;
		}
		bs_bytes ++;
	}
	
	if (bs_bytes == mpeg_bytes+bs_size)
		err = kFskErrUnknownElement;
	
	*offset = bs_bytes - mpeg_bytes;
	
	return err;
}

static int check_video_fomat(unsigned char  *mpeg_bytes, int *format)
{
	int err = 0;
	unsigned char *bs_bytes = mpeg_bytes;
	
#define TRACK_H264_PS 0x1C
	if (bs_bytes[0]==0x00 && bs_bytes[1]==0x00 && bs_bytes[2]==0x00 && bs_bytes[3]==0x01 && bs_bytes[4]==0x09)
		*format = TRACK_H264_PS; //avc parse in ps is not ready.
	else if (bs_bytes[0]==0x00 && bs_bytes[1]==0x00 && bs_bytes[2]==0x01)
		*format = TRACK_MPEG2V;
	else if (0) //TODO: mpeg1 video
		*format = TRACK_MPEG1V;
	else
		err = kFskErrUnknownElement;
	
	return err;
}

int md_ps_parse_header_x( MPEG_DEMUXER *md, unsigned char  *mpeg_bytes, int *is_audio, PES_HEADER	*pesh_x, int *pes_offset, int *pes_size  )
{
	MPEG_PS_PRIV		*priv = (MPEG_PS_PRIV *)md->priv;
	PS_PACK_HEADER_x	pach_x;
	PS_SYSTEM_HEADER_x	sysh_x = {0};
	int					err = 0;
	
	*pes_offset = 0;
	*pes_size   = 0;
	*is_audio   = 0;
	FskMemSet((void*)pesh_x, 0, sizeof( PES_HEADER));	
	
	err = ps_scan_header_x(  mpeg_bytes,  &pach_x, &sysh_x, pesh_x, pes_offset,  pes_size );
	BAIL_IF_ERR( err );
	
	if (IS_AUDIO_STREAM( pesh_x->stream_id ))
		*is_audio = 1;
	
	if( !priv->sys_initialized && sysh_x.is_valid )
	{
		priv->sys				= sysh_x;
		priv->sys_initialized	= 1;
	}
	
	if( /*!pesh_x->is_valid || */md->initialized )
		goto bail;
	
	mpeg_bytes +=  *pes_offset;
	
 	if( IS_VIDEO_STREAM( pesh_x->stream_id ) && !md->video_header_initialized )
	{
		md->video_elementary_pid	= pesh_x->stream_id;
		err = check_video_fomat(mpeg_bytes, &md->video_format);
		if (err)
			md->video_format		= TRACK_MPEG2V;
		
		err = md_parse_video_codec( md, mpeg_bytes, *pes_size );
	}
	else if( IS_AUDIO_STREAM( pesh_x->stream_id ) && !md->audio_header_initialized  )
	{
		md->audio_elementary_pid	= pesh_x->stream_id;
		md->audio_format			= TRACK_MPEGA;
		
		err = md_parse_audio_codec( md, mpeg_bytes, *pes_size );
	}
	else if( IS_PRIVATE_STREAM(pesh_x->stream_id ) /*&& !md->audio_header_initialized*/ )
	{
		int ps_offset = 0;
		md->audio_elementary_pid	= pesh_x->stream_id;
		err = check_audio_fomat(mpeg_bytes, &md->audio_format, *pes_size, &ps_offset);
		if (err)
			goto bail;
		
		err = md_parse_audio_codec( md, mpeg_bytes+ps_offset, *pes_size );
		pesh_x->is_valid = 1;
	}
	BAIL_IF_ERR(err );

	if( priv->sys_initialized )
	{
		if( priv->sys.stream_total == 1 )
		{
			if( md->audio_header_initialized || md->video_header_initialized )
				md->initialized = 1;
		}
		else if( priv->sys.stream_total >= 2 )
		{
			if( md->audio_header_initialized  &&  md->video_header_initialized )
				md->initialized = 1;
		}
	}	

bail:

	return err;
}

int length2startcode(unsigned char *data, int size, unsigned char *mg_data, int pes_size, int *out_size, int format)
{
	unsigned char *byte = mg_data;
	int offset = 0;
	
	if (format == TRACK_MPEG2V || format == TRACK_MPEG1V)
	{
		while(1)
		{
			if (byte[0]==0x0 && byte[1]==0x0 && byte[2]==0x1 && byte[3]==0x0)
				break;
			else
				byte ++;
			
			if (byte >= mg_data + pes_size)
				break;
		}
		
		offset = byte - mg_data;
		*out_size += offset;
	}
	else if (format == TRACK_H264 || format == TRACK_H264_x)
	{
		while(1)
		{
			if (byte[0]==0x0 && byte[1]==0x0 && byte[2]==0x0 && byte[3]==0x1 && (byte[4]==0x65 || byte[4]==0x41))
				break;
			else
				byte ++;
			
			if (byte >= mg_data + pes_size)
				break;
		}
		
		offset = byte - mg_data;
		*out_size += offset;
	}
	else
		;
	
	return 0;
}

int check_start_code(unsigned char *data, int size, int *offset, int format)
{
	unsigned char *byte = data;
	int cnt = 2;

	if (format == TRACK_MPEG2V || format == TRACK_MPEG1V)
	{
		while(1)
		{
			if (byte[0]==0x0 && byte[1]==0x0 && byte[2]==0x1 && byte[3]==0x0)
			{
				if (cnt == 2)
				{	
					byte ++;
					cnt --;
				}
				else if(cnt == 1)
				{
					*offset = byte - data;
					return 1;
				}
			}
			else
				byte ++;

			if (byte >= data + size)
			{
				*offset = 0;
				return 0;
			}
		}
	}
	else if (format == TRACK_H264 || format == TRACK_H264_x)
	{
		while(1)
		{
			if (byte[0]==0x0 && byte[1]==0x0 && byte[2]==0x0 && byte[3]==0x1 && (byte[4]==0x65 || byte[4]==0x41))
			{
				if (cnt == 2)
				{	
					byte ++;
					cnt --;
				}
				else if(cnt == 1)
				{
					*offset = byte - data;
					return 1;
				}
			}
			else
				byte ++;
			
			if (byte >= data + size)
			{
				*offset = 0;
				return 0;
			}
		}
	}

	return 0;
}

int md_ps_reassemble_sample
( 
	MPEG_DEMUXER	*md, 
	PES_HEADER		*pesh_x, 
	int				use_raw, 
	unsigned char	*pes_bytes, 
	int				pes_size, 
	int				*is_video, 
	unsigned char	**bs_bytes_out, 
	int				*bs_size, 
	int				sc_flag,
	int				sc_size,
	TIME_64			*time_pre,
	TIME_64			*time_dec
)
{
	unsigned char		*bs_bytes = NULL;
	FRAME_BUFFER		*tt;
	int					fake_PID = 0;
	int					err = 0;

	*bs_bytes_out = NULL;
	*bs_size	  = 0;

	INVALIDATE_TIME( time_pre );
	INVALIDATE_TIME( time_dec );

	if( IS_AUDIO_STREAM(pesh_x->stream_id) )
		fake_PID = pesh_x->stream_id;
	else if( IS_VIDEO_STREAM(pesh_x->stream_id) )
		fake_PID = pesh_x->stream_id;
	else 
		goto bail;//only deal with audio and video pes

	tt = get_fb_by_idx( md->fb_ary, fake_PID, pesh_x->stream_id );
	if( tt == NULL  ) 
		goto bail;

	if (sc_flag == 1)
	{
		err = FskMemPtrNew(sc_size + 64, &bs_bytes);
		BAIL_IF_ERR( err );

		FskMemCopy( bs_bytes, tt->buf, sc_size );
		tt->size -= sc_size;

		memmove(tt->buf, tt->buf+sc_size, tt->size);

		*bs_size = sc_size;
		*is_video = 1;

		*time_pre = tt->time_pre;
		*time_dec = tt->time_dec;

		goto bail;
	}

	if
	( 
		tt->size != 0										&&
		IS_VIDEO_STREAM( pesh_x->stream_id )				&& 
		IS_VALID_TIME( &pesh_x->presentation_time_stamp) 
	)
	{
		int this_size, size_last_pic;

		*is_video = 1;
		this_size = tt->size;

		length2startcode(tt->buf, tt->size, pes_bytes, pes_size, &this_size, md->video_format);
		size_last_pic = this_size - tt->size;
		
		err = FskMemPtrNew(this_size + 64, &bs_bytes);
		BAIL_IF_ERR( err );
		
		if( use_raw && size_last_pic)
		{//save raw bitstream
			FskMemCopy( bs_bytes, tt->buf, tt->size );
			FskMemCopy( bs_bytes+tt->size, pes_bytes, size_last_pic);
			
			pes_bytes += size_last_pic;
			pes_size  -= size_last_pic;
		}
		else
		{//convert to QT style box format
			err = startcode2len( tt->buf, tt->size, bs_bytes, &this_size );
			if( err != 0 || this_size == 0 )
				goto bail;
		}

		*bs_size = this_size;
		*time_pre = tt->time_pre;
		*time_dec = tt->time_dec;

		rest_frame_buffer(tt);
	}
	else if
	( 
		tt->size != 0										&&
		IS_AUDIO_STREAM( pesh_x->stream_id )				&& 
		IS_VALID_TIME( &pesh_x->presentation_time_stamp ) 
	)
	{
		unsigned char *src = tt->buf;
		int			   src_size = tt->size;
		int				audio_frame_size = 0;
		int				boundary = 0;

		*is_video = 0;

		audio_frame_size = src_size - md->audio_sample_offset;
		
		if( md->audio_format == TRACK_MPEGA || md->audio_format == TRACK_MPEGA_x )
		{
			if( pes_bytes[0] != 0xff || pes_bytes[1] != 0xfd || pes_bytes[1] != 0xfc)
			{
				find_mp3_boundary_from_end( src, src_size, &boundary );
				audio_frame_size -= src_size - boundary;
			}
		}
		
		if( audio_frame_size > 0 && IS_VALID_TIME(&tt->time_pre) )
		{
			src += md->audio_sample_offset;

			err = FskMemPtrNew(tt->size + 4, &bs_bytes);
			BAIL_IF_ERR(err);

			if( bs_bytes == NULL ) { err = -1;  goto bail; }

			FskMemCopy( bs_bytes, src, audio_frame_size );

			*bs_size = audio_frame_size;
			*time_pre = tt->time_pre;
			*time_dec = tt->time_dec;
			
			if( IS_VALID_TIME( time_pre) && !IS_VALID_TIME( time_dec) )
				*time_dec = *time_pre;
		}

		if( boundary != 0 )
		{
			int left_size = tt->size - boundary;

			FskMemCopy( tt->buf, tt->buf+boundary, tt->size - boundary );
			
			INVALIDATE_TIME( &tt->time_pre );
			INVALIDATE_TIME( &tt->time_dec );
			
			tt->size = left_size;
		}
		else
			rest_frame_buffer(tt);
	}
	//else  if( IS_PROGRAM_ASSOCIATION_TABLE(th.PID)  ||  is_associated_pid( &md->pat, th.PID ) )
	//	goto bail;

	if( !IS_VALID_TIME(&tt->time_dec) && IS_VALID_TIME(&pesh_x->decoding_time_stamp) )
		tt->time_dec = pesh_x->decoding_time_stamp;
	
	if( !IS_VALID_TIME(&tt->time_pre) && IS_VALID_TIME(&pesh_x->presentation_time_stamp) )
		tt->time_pre = pesh_x->presentation_time_stamp;

	if( pes_size > 0 )
	{
		int total_data_size = pes_size + tt->size;

		if( total_data_size > tt->buf_size )
		{
			unsigned char *new_buf;

			if( total_data_size < MIN_FRAME_SAMPLE_BUFFER_SIZE )
				total_data_size = MIN_FRAME_SAMPLE_BUFFER_SIZE;

			total_data_size = total_data_size + (total_data_size>>2);//make it 25% bigger

			err = FskMemPtrNew(total_data_size, &new_buf);
			BAIL_IF_ERR(err);

			if( tt->size > 0 )
				FskMemCopy( new_buf, tt->buf, tt->size );

			if( tt->buf != NULL )
				FskMemPtrDisposeAt( (void **)&tt->buf );

			tt->buf = new_buf;
			tt->buf_size = total_data_size;
		}
		
		FskMemCopy( &tt->buf[tt->size], pes_bytes, pes_size );
		tt->size += pes_size;
	}

bail:
	*bs_bytes_out = bs_bytes;

	return err;
}

#ifdef STAND_ALONE
#define GET_TIME_64(t)									\
(														\
	(FskInt64)(((FskInt64)GET_TIME_64_Hi(t))<<32)|		\
	(FskInt64)GET_TIME_64_Lo(t)							\
)

int ps_get_duration( MPEG_DEMUXER	*md, unsigned char	*mpeg_all_bytes, int src_size, TIME_64 *dur, TIME_64 *time_init )
{
	TIME_64 min_dec, max_dec;
	TIME_64 min_pre, max_pre;
	//int is_video;
	int progress = 0, is_video = 0, audio_size = 0, bps, progress_end = 0;
	//int sample_offset, sample_size;
	int err = 0, check_max_time = 1;
	//unsigned char	*mpeg_bytes = &mpeg_all_bytes[progress];
	//PS_PACK_HEADER_x	ph_x;
	//PS_SYSTEM_HEADER_x	sh_x;
	//PES_HEADER			pesh_x;
	//int pes_offset;
	//int pes_size;
	//int i;

	INVALIDATE_TIME( &min_dec );
	INVALIDATE_TIME( &max_dec );
	INVALIDATE_TIME( &min_pre );
	INVALIDATE_TIME( &max_pre );
	
	INVALIDATE_TIME( dur );
	INVALIDATE_TIME( time_init );

	//Min
	while(progress < src_size )
	{
		unsigned char	*mpeg_bytes = &mpeg_all_bytes[progress];
		PES_HEADER			pesh_x;
		int					pes_offset = 0;
		int					pes_size   = 0;
		int					is_audio;
		//TIME_64				time;
		
		is_video = 0;
		
		err = md_ps_parse_header_x( md, mpeg_bytes, &is_audio, &pesh_x, &pes_offset, &pes_size );
		//if( err != 0 ) goto bail;

		if (IS_VIDEO_STREAM(pesh_x.stream_id))
            is_video = 1;
		
		if ((pes_offset + pes_size) == 0)
			progress += 1;
		else
			progress += pes_offset + pes_size;

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
	if (src_size > progress+MIN_SIZE_FOR_BIGGEST)
		progress = src_size - MIN_SIZE_FOR_BIGGEST;
	
	progress_end = src_size;
	
check_max:	
	while(progress < progress_end )
	{
		unsigned char	*mpeg_bytes = &mpeg_all_bytes[progress];
		PES_HEADER			pesh_x;
		int					pes_offset = 0;
		int					pes_size   = 0;
		int					is_audio;
		//TIME_64				time;
		
		is_video = 0;
		
		err = md_ps_parse_header_x( md, mpeg_bytes, &is_audio, &pesh_x, &pes_offset, &pes_size );
		//if( err != 0 ) goto bail;
		
		if (IS_VIDEO_STREAM(pesh_x.stream_id))
            is_video = 1;
		
		if ((pes_offset + pes_size) == 0)
			progress += 1;
		else
			progress += pes_offset + pes_size;
		
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
		if (src_size <= check_max_time*MIN_SIZE_FOR_BIGGEST)
			goto bail;
		else {
			progress = src_size - check_max_time*MIN_SIZE_FOR_BIGGEST;
			progress_end = progress + MIN_SIZE_FOR_BIGGEST;
			goto check_max;
		}
	}
	
	if( IS_VALID_TIME( &min_dec) && IS_VALID_TIME( &max_dec) )
	{
		DIFF_TIME_64_ABS(dur, &max_dec, &min_dec );
		*time_init = min_dec;
	}
	else if( IS_VALID_TIME( &min_pre ) && IS_VALID_TIME( &max_pre ) )
	{
		DIFF_TIME_64_ABS( dur, &max_pre, &min_pre );
		*time_init = min_pre;
	}
	
	if( IS_VALID_TIME( dur) )
	{
		float audio_dur	= (float)GET_TIME_64(dur);
		bps = (int)(8*(float)audio_size*MPEG_VIDEO_DEFAULT_TIME_SCALE/audio_dur);
		dlog( " \naudio duration = %f, audio_size = %d\n", audio_dur, audio_size );
	}
	else
	{
		dlog("\naudio duration is not valid!!\n");
		bps = 0;
	}

bail:
	return err;
}



int md_ps_process_sample
( 
	MPEG_DEMUXER	*md, 
	int				use_raw, 
	unsigned char	*mpeg_bytes, 
    int				bytes_left,
	int				*is_video, 
	unsigned char	**bs_bytes_out, 
	int				*bs_size, 
	TIME_64			*time_pre, 
	TIME_64			*time_dec, 
	int				*offset_out 
)
{
	//unsigned char	*bs_bytes = NULL;
	//unsigned char	*payload_start;
	//int				payload_size;
	PS_PACK_HEADER_x	pakh_x;
	PS_SYSTEM_HEADER_x	sysh_x;
	PES_HEADER			pesh_x;
	//FRAME_BUFFER	*tt;
	unsigned char	*pes_bytes = NULL;
	int				pes_size = 0;
	int				pes_offset = 0;
	int				err = 0;
	int				sc_flag = 0, sc_size = 0;
	FRAME_BUFFER		*tt;

	*bs_size = 0;

	err = ps_scan_header_x( mpeg_bytes, &pakh_x, &sysh_x, &pesh_x, &pes_offset, &pes_size );
	BAIL_IF_ERR( err );

	if (pes_offset + pes_size > bytes_left) //check whether last pes over file.
		goto bail;

	tt = get_fb_by_idx( md->fb_ary, pesh_x.stream_id, pesh_x.stream_id );
	if( tt == NULL  ) 
		goto bail;
	
	if (tt->size)
		sc_flag = check_start_code(tt->buf, tt->size, &sc_size, md->video_format);

	if (sc_flag)
		pes_offset = pes_size = -1;

	pes_bytes = mpeg_bytes + pes_offset;

	err = md_ps_reassemble_sample
			( 
				md, 
				&pesh_x, 
				use_raw, 
				pes_bytes, 
				pes_size, 
				is_video, 
				bs_bytes_out, 
				bs_size, 
				sc_flag,
				sc_size,
				time_pre,
				time_dec
			);

	BAIL_IF_ERR( err );
	
bail:
	*offset_out   = pes_offset + pes_size;

	return err;
}


int ps_to_box()
{
	char			src_path[256];
	FILE			*src_file	= NULL;
	FILE			*video_file=NULL;
	FILE			*audio_file=NULL;
	unsigned char	*mpeg_all_bytes = NULL;
	int				src_size;
	int				progress = 0;
	int				err = 0;
	
	MPEG_DEMUXER	*md;
	TIME_64			dur;
	TIME_64			time_init;
	
	//int				track_total = 0;
	//int				dur = 0;
	//int				time_init;
	
	if(video_file == NULL )
		video_file = fopen("/test2/others/mpeg_av_box/mpeg_video_mpg.box", "wb");
	if( video_file == NULL )
		return -1;
	if(audio_file == NULL )
		audio_file = fopen("/test2/others/mpeg_av_box/mpeg_audio_mpg.box", "wb");
	if( audio_file == NULL )
		return -1;

	strcpy( src_path, "/test2/video/mpeg/mpegNEW/" );
	strcat( src_path, "mpeg2.vob" );
	src_file = fopen(src_path, "rb");
	if( src_file == NULL )
		return -1;

	fseek(src_file, 0, SEEK_END);
	src_size = ftell(src_file);
	fseek(src_file, 0, SEEK_SET);
	
	err = FskMemPtrNewClear(src_size, &mpeg_all_bytes);
	if( err != 0 ) goto bail;

	err = md_ps_new( &md );
	BAIL_IF_ERR(err);
	
	fread( mpeg_all_bytes, 1, src_size, src_file );

	err = ps_get_duration( md, mpeg_all_bytes, src_size, &dur, &time_init );
	BAIL_IF_ERR(err);

 	while( progress < src_size )
	{
		unsigned char	*mpeg_bytes = &mpeg_all_bytes[progress];
		int				pes_offset;
		int				pes_size;
		PES_HEADER		pesh_x;
		int				is_audio;

		err = md_ps_parse_header_x(  md, mpeg_bytes, &is_audio, &pesh_x, &pes_offset, &pes_size );
		BAIL_IF_ERR( err );

		if( md->initialized )
			break;

		if ((pes_offset + pes_size) == 0)
			progress += 1;
		else
			progress += pes_offset + pes_size;
	}

	progress = 0;
	while( progress < src_size )
	{
		unsigned char		*mpeg_bytes = &mpeg_all_bytes[progress];
		int					offset;
		int					is_video	= 0;
		unsigned char		*bs_bytes	= NULL;
		int					bs_size;
		TIME_64				time_pre;
		TIME_64				time_dec;

		err = md_ps_process_sample( md, 1, mpeg_bytes, src_size-progress, &is_video, &bs_bytes, &bs_size, &time_pre, &time_dec, &offset );
		BAIL_IF_ERR( err );

		if( bs_size != 0 )
		{
			if( is_video )
			{
				//fwrite( &bs_size,  1, 4, video_file);
				fwrite( bs_bytes,  1, bs_size, video_file);
				fflush(video_file);
			}
			else
			{
				int frame_total = 0;
				int frame_sizes[64];
				int offsets[64];
				
				err = divide_mp3( bs_bytes, bs_size, &frame_total, frame_sizes, offsets );
	
				//fwrite(&bs_size,  4, 1, audio_file);
				fwrite(bs_bytes, bs_size, 1, audio_file );
				fflush(audio_file);
			}
		}
		
		if (offset == 0)
			progress += 1;
		else if (offset == -2)
			;
		else
			progress += offset;
	}
	
	fclose(video_file);
	fclose(audio_file);

	//if( 
	//	same_files( "E:\\test_mpeg\\mpeg_ps_video.box", "E:\\test_avc\\mpeg_ps_video.ref.box" ) 
	//	&&
	//	same_files( "E:\\test_mpeg\\mpeg_ps_audio.box", "E:\\test_avc\\mpeg_ps_audio.ref.box" )
	//)
	//{
	//	bnie("VVV!!!\n");
	//}
	//else
	//{
	//	bnie("XXX!!!\n");
	//}

bail:
	md_ps_dispose( md );

	return err;
}


#endif
