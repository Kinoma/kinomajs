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
#define U32_READ_BITS(x_u32, x_left, x_n, x_result)		\
{														\
	int mask = (1<<x_n)-1;								\
	x_result = (x_u32>>(x_left-x_n))&mask;				\
	x_left  -= x_n;										\
}

#define U32_READ_ONE_BIT(x_u32, x_left)		(--x_left && ((x_u32 >> x_left) & 1))

#define BS_RESET(x_bs, x_n, x_offset)	\
{										\
	int	real_offset = x_n + x_offset;	\
	x_bs += real_offset >> 3;			\
	x_offset  = real_offset & 7;		\
}

#define BS_LOAD32(x_bs, x_offset, x_value)											\
{																					\
	x_value = (x_bs[0] << 24) | (x_bs[1] << 16) | (x_bs[2] <<  8) | (x_bs[3]);		\
	x_value <<= x_offset;															\
}


//***to go away soon...

#define DEC_UNSIGNED_PAIR( cw_x, y_x, z_x, shift_x, mask_x )	\
{													\
	y_x = (short)( cw_x >> shift_x);						\
	z_x = (short)((cw_x &  mask_x));						\
}

#define DEC_SIGNED_PAIR( cw_x, y_x, z_x, shift_x, mask_x, offset_x )	\
{															\
	y_x = (short)( cw_x >> shift_x);						\
	z_x = (short)((cw_x &  mask_x) - offset_x);				\
}

#define DEC_UNSIGNED_QUAD( cw_x, w_x, x_x, y_x, z_x, shift_x, mask_x )	\
{															\
	w_x = (short)(  cw_x >> (3 * shift_x));					\
	x_x = (short)(((cw_x >> (2 * shift_x)) & mask_x));		\
	y_x = (short)(((cw_x >> (shift_x)) & mask_x));			\
	z_x = (short)(( cw_x & mask_x));						\
}

#define DEC_SIGNED_QUAD( cw_x, w_x, x_x, y_x, z_x, shift_x, mask_x, offset_x )	\
{																	\
	w_x = (short)(  cw_x >> (3 * shift_x));							\
	x_x = (short)(((cw_x >> (2 * shift_x)) & mask_x)-offset_x);		\
	y_x = (short)(((cw_x >> (shift_x)) & mask_x)-offset_x);			\
	z_x = (short)(( cw_x & mask_x)-offset_x);						\
}

#define DEC_UNSIGNED_PAIR_ESC( cw_x, y_x, z_x, shift_x, mask_x )	\
{																\
	y_x = (short)( cw_x >> shift_x);							\
	z_x = (short)((cw_x &  mask_x));							\
}

#define ENC_UNSIGNED_PAIR_ESC( cw2, y_x, z_x )					\
{																\
	cw2 = (short)( (y_x<<8)|(z_x&0xff));						\
}

#define DEC_UNSIGNED_PAIR_ESC_XXX( cw_x, y_x, z_x )	\
{													\
	y_x = (short)( cw_x >> 8);						\
	z_x = (short)((cw_x & 0xff));					\
}


#define ENC_UNSIGNED_PAIR( cw2, y_x, z_x )					\
{																\
	cw2 = (short)( (y_x<<8)|(z_x&0xff));						\
}

#define DEC_UNSIGNED_PAIR_XXX( cw_x, y_x, z_x )	\
{												\
	y_x = (short)( cw_x >> 8);					\
	z_x = (short)( cw_x & 0xff);				\
}

#define ENC_UNSIGNED_QUAD( cw_x, w_x, x_x, y_x, z_x )		\
{															\
	cw_x = (short)( (w_x<<9)|(x_x<<6)|(y_x<<3)|(z_x<<0));	\
}

#define DEC_UNSIGNED_QUAD_XXX( cw_x, w_x, x_x, y_x, z_x )	\
{															\
	w_x = (short)(  cw_x >> (3 * 3));					\
	x_x = (short)(((cw_x >> (2 * 3)) & 0x07));		\
	y_x = (short)(((cw_x >> (3)) &  0x07));			\
	z_x = (short)(( cw_x &  0x07));						\
}

#define ENC_SIGNED_PAIR( cw_x, y_x, z_x )			\
{													\
	cw_x = (short)( ((y_x)<<8)|((z_x+32)&0xff));		\
}

#define DEC_SIGNED_PAIR_XXX( cw_x, y_x, z_x )	\
{												\
	y_x = (short)( cw_x >> 8);					\
	z_x = (short)( (cw_x & 0xff)-32 );			\
}


#define ENC_SIGNED_QUAD( cw_x, w_x, x_x, y_x, z_x )		\
{														\
	cw_x = (short)(										\
				(short)(w_x<<9)			|				\
				(short)((x_x+4)<<6)		|				\
				(short)((y_x+4)<<3)		|				\
				(short)((z_x+4)<<0)						\
			);											\
}

#define DEC_SIGNED_QUAD_XXX( cw_x, w_x, x_x, y_x, z_x )	\
{														\
	w_x = (short)(  cw_x >> (3 * 3));					\
	x_x = (short)(((cw_x >> (2 * 3)) & 0x07)-4);		\
	y_x = (short)(((cw_x >> (3)) &  0x07)-4);			\
	z_x = (short)(( cw_x &  0x07)-4);					\
}


#ifdef __cplusplus
extern "C" {
#endif

//for mp3
int kinoma_mp3_dec_huff_esc_block( unsigned char **ppBS, int *pOffset, short *dst, int	len, int idx );
int kinoma_mp3_dec_huff_quad( unsigned char **ppBS, int *pOffset, short *dst, int idx );
int kinoma_mp3_dec_huff_pair_block( unsigned char **ppBS, int *pOffset, short *dst, int len, int idx );
int kinoma_mp3_getlinbits( int idx );

//for aac
void kinoma_aac_dec_huff_init_tab();
int kinoma_aac_dec_huff_esc_block(unsigned char **bs_in, int* bit_offset_in, short *dst, int num, int idx);
int kinoma_aac_dec_huff_quad_block (unsigned char ** bs_in, int* bit_offset_in, short **pDst_in, int num, int idx );
int kinoma_aac_dec_huff_pair_block (unsigned char ** bs_in, int* bit_offset_in, short **pDst_in, int num, int idx );
int kinoma_aac_dec_huff_one_sf(unsigned char **bs_in, int *offset_in, short* pDst );

#ifdef __cplusplus
}
#endif
