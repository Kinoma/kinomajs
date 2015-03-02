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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SPS_COUNT 32
#define MAX_PIC_COUNT 32
typedef struct
{
    int profile_idc;
    int level_idc;
    int constrained_set0_flag;
    int constrained_set1_flag;
    int constrained_set2_flag;
    int constrained_set3_flag;
	int sps_id;
    int chroma_format_idc;
	int residual_colour_transform_flag;
	int bit_depth_luma;
	int bit_depth_chroma;
    int qpprime_y_zero_transform_bypass_flag;  

    int seq_scaling_matrix_present_flag;
    unsigned char ScalingLists4x4[6][16];
    unsigned char ScalingLists8x8[2][64];
	
    int log2_max_frame_num;							//< log2_max_frame_num_minus4 
    int pic_order_cnt_type;							
    int log2_max_pic_order_cnt_lsb;					//< log2_max_pic_order_cnt_lsb_minus4
    int delta_pic_order_always_zero_flag;
    int frame_mbs_only_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    int num_ref_frames_in_pic_order_cnt_cycle;		
    int num_ref_frames;								
    int gaps_in_frame_num_allowed_flag;
    int frame_width_in_mbs;							//< frame_width_in_mbs_minus1 + 1
    int frame_height_in_mbs;						//< frame_height_in_mbs_minus1 + 1
    int mb_adaptive_frame_field_flag;              
    int direct_8x8_inference_flag;
    int frame_cropping_flag;						
    int frame_cropping_rect_left_offset;            
    int frame_cropping_rect_right_offset;           
    int frame_cropping_rect_top_offset;             
    int frame_cropping_rect_bottom_offset;          
    
	
	int vui_parameters_present_flag;
    //AVRational sar;
    int timing_info_present_flag;
    int num_units_in_tick;
    int time_scale;
    int fixed_frame_rate_flag;
    short offset_for_ref_frame[256]; 
    int bitstream_restriction_flag;
    int num_reorder_frames;
}SeqParamSet;


typedef struct BitStream {
	unsigned char *p_cur;
	unsigned char *p_end;
	int bits_left;
} BitStream;


static const unsigned int kmask[33] = {
    0x00,
    0x01,      0x03,      0x07,      0x0f,
    0x1f,      0x3f,      0x7f,      0xff,
    0x1ff,     0x3ff,     0x7ff,     0xfff,
    0x1fff,    0x3fff,    0x7fff,    0xffff,
    0x1ffff,   0x3ffff,   0x7ffff,   0xfffff,
    0x1fffff,  0x3fffff,  0x7fffff,  0xffffff,
    0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff,
    0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff
};

#define save_bitstream_status \
    unsigned char *cur = bs->p_cur; \
    unsigned char *end = bs->p_end; \
    int bs_left        = bs->bits_left;

#define restore_bitstream_status \
    bs->p_cur     = cur;         \
    bs->p_end     = end;         \
    bs->bits_left = bs_left;     \


static void bitstream_init(BitStream *bs, unsigned char *data, int size)
{
    bs->p_cur = data;
    bs->p_end = bs->p_cur + size;
    bs->bits_left = 8;
}

static int get_bits(BitStream *bs, int len)
{
	int bits_remain;
    unsigned int val = 0;
    
    if (bs->p_cur >= bs->p_end || len < 0) {
        return -1; //error case
    }
    
    while (len > 0) {
        if (0 <= (bits_remain = bs->bits_left - len)) { //bits left is enough
            val |= (*bs->p_cur >> bits_remain) & kmask[len];
            bs->bits_left -= len;
            if(0 == bs->bits_left)
            {
                bs->p_cur ++;
                bs->bits_left = 8;
            }
            return(val);
        }
        else { //bits left is not enough
            val |= (*bs->p_cur & kmask[bs->bits_left]) << (-bits_remain);
            len -= bs->bits_left;
            bs->p_cur++;
            bs->bits_left = 8;
        }
    }

	return val;
}

static int get_bits1(BitStream *bs)
{
    unsigned int val;
	
    if(bs->p_cur >= bs->p_end) {
        return -1; //error case
    }

    bs->bits_left --;
    val = (*bs->p_cur >> bs->bits_left) & kmask[1];
    if( bs->bits_left == 0 )
    {
        bs->p_cur ++;
        bs->bits_left = 8;
    }
    return val;
}

//static int get_ue_golomb_31(BitStream *bs)
//{
//	//***to be implemented
//	return 0;
//}

static int get_ue_golomb(BitStream *bs)
{
    int i = 0;
    
    while(get_bits1(bs) == 0 && bs->p_cur < bs->p_end && i < 32)
    {
        i++;
    }

    return((1 << i) - 1 + get_bits(bs, i));
}

static int get_se_golomb(BitStream *bs)
{
    int val = get_ue_golomb(bs);
    
    return val&kmask[1] ? (val+1)/2 : -(val/2);
}

static int decode_seq_param_set(BitStream *bs, SeqParamSet *sps)
{
    int i;
	int err = 0;
	
    
	get_bits(bs, 8); //skip nalu 8 bits
    
	sps->profile_idc= get_bits(bs, 8);
    // printf("profile = %d\n", sps->profile_idc);

    sps->constrained_set0_flag = get_bits1(bs); 
    sps->constrained_set1_flag = get_bits1(bs); 
    sps->constrained_set2_flag = get_bits1(bs); 
    sps->constrained_set3_flag = get_bits1(bs); 
    
    get_bits(bs, 4); // reserved
    sps->level_idc= get_bits(bs, 8);

	sps->sps_id= get_ue_golomb(bs);
	if(sps->sps_id >= MAX_SPS_COUNT) 
	{
		err = -1;//bad sps id
        // printf("error when parse sps ID\n");
		goto bail;
	}
	
    //memset(sps->scaling_matrix4, 16, sizeof(sps->scaling_matrix4));
    //memset(sps->scaling_matrix8, 16, sizeof(sps->scaling_matrix8));
    sps->seq_scaling_matrix_present_flag = 0;
    if(sps->profile_idc >= 100)//high profile
	{ 
		err = -1;//***bnie: skip it for now
		goto bail;

        sps->chroma_format_idc= get_ue_golomb(bs);
        if(sps->chroma_format_idc == 3)
            sps->residual_colour_transform_flag = get_bits1(bs);
        sps->bit_depth_luma   = get_ue_golomb(bs) + 8;
        sps->bit_depth_chroma = get_ue_golomb(bs) + 8;
        sps->qpprime_y_zero_transform_bypass_flag = get_bits1(bs);
       // decode_scaling_matrices(h, sps, NULL, 1, sps->scaling_matrix4, sps->scaling_matrix8);
    }
	else
	{
        sps->chroma_format_idc= 1;
    }
    // printf("level = %d\n", sps->level_idc);
	
    sps->log2_max_frame_num= get_ue_golomb(bs) + 4;
    sps->pic_order_cnt_type= get_ue_golomb(bs);
    // printf("log2_max_frame_num, poc = %d, %d\n", sps->log2_max_frame_num, sps->pic_order_cnt_type);
	
    if(sps->pic_order_cnt_type == 0)
        sps->log2_max_pic_order_cnt_lsb= get_ue_golomb(bs) + 4;
	else if(sps->pic_order_cnt_type == 1)
	{
        sps->delta_pic_order_always_zero_flag = get_bits1(bs);
        sps->offset_for_non_ref_pic			  = get_se_golomb(bs);
        sps->offset_for_top_to_bottom_field   = get_se_golomb(bs);
        sps->num_ref_frames_in_pic_order_cnt_cycle                 = get_ue_golomb(bs);
		
		#define ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
        if((unsigned)sps->num_ref_frames_in_pic_order_cnt_cycle >= ARRAY_ELEMS(sps->offset_for_ref_frame))
		{
			err = -1;//num_ref_frames_in_pic_order_cnt_cycle overflow
			goto bail;
        }
		
        for(i=0; i<sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
            sps->offset_for_ref_frame[i]= get_se_golomb(bs);
		
    }
	else if(sps->pic_order_cnt_type != 2)
	{
        err = -1;//illegal POC type
        goto bail;
    }
	
    sps->num_ref_frames= get_ue_golomb(bs);
    if(sps->num_ref_frames > MAX_PIC_COUNT-2 || sps->num_ref_frames >= 32)
	{
        err = -1; //too many reference frames
        goto bail;
    }
    // printf("num_ref_frames = %d\n", sps->num_ref_frames);

    sps->gaps_in_frame_num_allowed_flag= get_bits1(bs);
    sps->frame_width_in_mbs = get_ue_golomb(bs) + 1;
    sps->frame_height_in_mbs= get_ue_golomb(bs) + 1;
    // printf("gaps_in_frame_num_allowed_flag = %d\n", sps->gaps_in_frame_num_allowed_flag);
    // printf("frame width, height in mbs = %d, %d\n", sps->frame_width_in_mbs, sps->frame_height_in_mbs);

	#define MY_INT_SIZE16 1024
    if((unsigned)sps->frame_width_in_mbs >= MY_INT_SIZE16 || (unsigned)sps->frame_height_in_mbs >= MY_INT_SIZE16 )
	{
        err = -1; //frame_width_in_mbs/height overflow
        goto bail;
    }
	
    sps->frame_mbs_only_flag = get_bits1(bs);
    // printf("frame_mbs_only_flag = %d\n", sps->frame_mbs_only_flag);
    if(!sps->frame_mbs_only_flag)
        sps->mb_adaptive_frame_field_flag= get_bits1(bs);
    else
        sps->mb_adaptive_frame_field_flag= 0;
	
    sps->direct_8x8_inference_flag= get_bits1(bs);
    // printf("direct_8x8_inference_flag = %d\n", sps->direct_8x8_inference_flag);
	
    sps->frame_cropping_flag= get_bits1(bs);
    // printf("frame_cropping_flag = %d\n", sps->frame_cropping_flag);
    if(sps->frame_cropping_flag)
	{
        sps->frame_cropping_rect_left_offset  = get_ue_golomb(bs);
        sps->frame_cropping_rect_right_offset = get_ue_golomb(bs);
        sps->frame_cropping_rect_top_offset   = get_ue_golomb(bs);
        sps->frame_cropping_rect_bottom_offset= get_ue_golomb(bs);
    }
	else
	{
        sps->frame_cropping_rect_left_offset  = 0;
        sps->frame_cropping_rect_right_offset = 0;
        sps->frame_cropping_rect_top_offset   = 0;
        sps->frame_cropping_rect_bottom_offset= 0;
    }
	
	//***bnie: not needed yet
    //sps->vui_parameters_present_flag= get_bits1(bs);
    //if( sps->vui_parameters_present_flag )
    //    decode_vui_parameters(h, sps);
	
bail:
    return err;
}

static int kinoma_avc_dec_parse_header_info( BitStream *bs, int *left, int *right, int *top, int *bottom, int *frame_mbs_only_flag, int *profile, int *level )
{
	SeqParamSet		sps;
	int				width16, height16;
	int				err = 0;

    // printf("calling into decode_seq_param_set\n");
	err = decode_seq_param_set(bs, &sps );
	if( err ) goto bail;
	
	width16  = 16 * ( sps.frame_width_in_mbs );
	height16 = 16 * ( sps.frame_height_in_mbs );
    // printf("width, height = %d, %d\n", width16, height16);
	
	if( left && right && top && bottom )
	{
		if( sps.frame_cropping_flag )
		{
			if( left   != NULL )	*left   = 2*sps.frame_cropping_rect_left_offset;
			if( right  != NULL )	*right  = width16 - (2*sps.frame_cropping_rect_right_offset+1);
			if( top    != NULL )	*top    = 2*sps.frame_cropping_rect_top_offset;
			if( bottom != NULL )	*bottom = height16 - (2*sps.frame_cropping_rect_bottom_offset+1);
		}
		else
		{
			if( left   != NULL )	*left   = 0;
			if( right  != NULL )	*right  = width16 - 1;
			if( top    != NULL )	*top    = 0;
			if( bottom != NULL )	*bottom = height16 - 1;
		}
	}
	
	if( frame_mbs_only_flag != NULL ) *frame_mbs_only_flag	= sps.frame_mbs_only_flag;
	if( profile				!= NULL ) *profile				= sps.profile_idc;
	if( level				!= NULL ) *level				= sps.level_idc;
	
bail:	
	return err;
}

#define ZEROBYTES_SHORTSTARTCODE 2 //indicates the number of zero bytes in the short start-code prefix

int NALUtoRBSP(unsigned char *streamBuffer, int end_bytepos, int begin_bytepos)
{
    int i, j, count;
    count = 0;
    
    if(end_bytepos < begin_bytepos)
        return end_bytepos;
    
    j = begin_bytepos;
    
    for(i = begin_bytepos; i < end_bytepos; i++) 
    { //starting from begin_bytepos to avoid header information
        if(count == ZEROBYTES_SHORTSTARTCODE && streamBuffer[i] == 0x03)
        {
            i++;
            count = 0;
        }
        streamBuffer[j] = streamBuffer[i];
        if(streamBuffer[i] == 0x00)
            count++;
        else
            count = 0;
        j++;
    }
    
    return j;
}


int parse_avc_header( unsigned char *sps, int sps_size, int *width, int *height, int *profile, int *level )
{
	int left = 0, right = 0, top = 0, bottom = 0, frame_mbs_only_flag = 0;
    BitStream bs;
    int err = 0;
    unsigned char *n2r = NULL;

    n2r = (unsigned char *)malloc(sps_size + 4);
    memcpy((void *)n2r, (void *)sps, sps_size);
    NALUtoRBSP(n2r, sps_size, 1); //nalu ==> rbsp
    bitstream_init(&bs, n2r, sps_size); //skip nal bytes, intialize bitstream.
    
    // printf("after bs_init = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", bs.p_cur[0], bs.p_cur[1], bs.p_cur[2], bs.p_cur[3], bs.p_cur[4], bs.p_cur[5], bs.p_cur[6], bs.p_cur[7], bs.p_cur[8], bs.p_cur[9]);
	
	err = kinoma_avc_dec_parse_header_info( &bs, &left, &right, &top, &bottom, &frame_mbs_only_flag, profile, level );
    // printf("SPS parsing finish!\n");
	
	*width  = right  - left + 1;
	*height = bottom - top  + 1;

    if(n2r != NULL)
        free(n2r);

	return err;
}



//#define TEST_APP
#ifdef TEST_APP

static int until_next_start_code( unsigned char *d, int size )
{
#define SC_LEN      4	//startcode length
    unsigned char *first = d;
	unsigned char *last  = d + size - SC_LEN;

	if(size < SC_LEN)
		return size;
    
	while(d <= last)
	{
		if( d[0] == 0x00 && d[1] == 0x00 && d[2] == 0x00 && d[3] == 0x01  )
			break;
        
		d++;
	}
    
	if(d >= last+1)
		return size;
    
	return d - first;
}



int main()
{
	//int nalu_len_size = 4;
    int count;
    unsigned char sps[100] = {0};
	int sps_size = 8;
	int left, right, top, bottom, frame_mbs_only_flag, profile, level;
    FILE *fp;
    BitStream bs;
    
    if (NULL == (fp = fopen("/Users/marvell/Downloads/video_encoded.264", "rb"))) {
        printf("can not open this file!\n");
    }
    
    count = fread(sps, 1, 100, fp);
    printf("read count = %d\n", count);
    
    sps_size = until_next_start_code(sps+4, 100);
    printf("sps size = %d\n", sps_size);
    printf("sps data = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", sps[0], sps[1], sps[2], sps[3], sps[4], sps[5], sps[6], sps[7], sps[8], sps[9], sps[10], sps[11], sps[12], sps[13]);
    
    NALUtoRBSP(sps+5, sps_size, 1); //nalu ==> rbsp

    bitstream_init(&bs, sps+5, sps_size); //skip nal bytes, intialize bitstream.
    
    printf("after bs_init = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", bs.p_cur[0], bs.p_cur[1], bs.p_cur[2], bs.p_cur[3], bs.p_cur[4], bs.p_cur[5], bs.p_cur[6], bs.p_cur[7], bs.p_cur[8], bs.p_cur[9]);

	kinoma_avc_dec_parse_header_info( &bs, &left, &right, &top, &bottom, &frame_mbs_only_flag, &profile, &level );
    printf("SPS parsing finish!\n");
	
	return 0;
}

#endif


