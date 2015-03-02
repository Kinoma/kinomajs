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
//#define PIXEL_BYTES_SHIFT	2						//1 for 16rgb
//#define PIXEL_BYTES		(1<<PIXEL_BYTES_SHIFT)
//#define DST_DATA_TYPE				long										//short for 16rgb
//#define DOWNSCALE_PACK  			PACK_32BGRA									//PACK_16RGB_lo for 16rgb
//#define UPSCALE_GET_Y_i			GET_Y_i_32BGRA								//GET_Y_i for 16rgb
//#define DST_RGB_TYPE				32GBRA										//16RGB565SE for 16rgb
//#define DOWNSCALE_FUNCTION_NAME	ttt_yuv420to16RGB565SE_down_scale_bc_p_i_c	//yuv420to32GBRA_down_scale_bc_p_i_c					//16RGB565SE for 16rgb
//#define UPSCALE_FUNCTION_NAME		ttt_yuv420to16RGB565SE_up_scale_bc_p_i_c	//yuv420to32GBRA_up_scale_bc_p_i_c					//16RGB565SE for 16rgb
#ifdef YUV420i_RGB_C_IMPLEMENTATION

#define BLEND_AND_COPY												\
{																	\
	unsigned char *this_src = sprite_back_buffer;					\
	unsigned char *this_dst = sprite_front_buffer + PIXEL_BYTES;	\
	int this_width;													\
	int this_height;												\
	FskVideoSprite s;												\
																	\
	ALIGN_4_BYTES( pattern )										\
	s =(FskVideoSprite)( *((long  *)(pattern + 4) ) );				\
	blend_proc(s);													\
	this_width	=*((short *)(pattern + 0));  /*horizontal offset*/	\
	this_height	=*((short *)(pattern + 2));  /*vertical offset*/	\
	pattern    += COPY_SPRITE_PARAM_BYTES;							\
																	\
	while(1)														\
	{																\
		memcpy( this_dst, this_src, this_width<<PIXEL_BYTES_SHIFT );\
		this_height--;												\
		if( this_height==0 )										\
			break;													\
																	\
		this_src += drb;											\
		this_dst += drb;											\
	}																\
																	\
	dst = sprite_front_buffer + (this_width<<PIXEL_BYTES_SHIFT);	\
}


#define SET_BLOCK(xxx, yyy, vvv, ddd)					\
{														\
	int patch_stride = drb - xxx*PIXEL_BYTES;			\
	int i, j;											\
														\
	for( j = 0; j < yyy; j++ )							\
	{													\
		for( i = 0; i < xxx; i++ )						\
		{												\
			ddd += PIXEL_BYTES;							\
			*((DST_DATA_TYPE *)ddd) = vvv;				\
		}												\
		ddd += patch_stride;							\
	}													\
}


#define SET_BLOCK2(xxx, yyy, vvv, ddd)					\
{														\
	int patch_stride = drb - xxx*PIXEL_BYTES;			\
	int i, j;											\
														\
	for( j = 0; j < yyy; j++ )							\
	{													\
		for( i = 0; i < xxx; i++ )						\
		{												\
			*((DST_DATA_TYPE *)ddd) = vvv;				\
			ddd += PIXEL_BYTES;							\
		}												\
		ddd += patch_stride;							\
	}													\
}



#define REPEAT_PIX(xxx, vvv, ddd)					\
{													\
	int i;											\
													\
	for( i = 0; i < xxx; i++ )						\
	{												\
		*((DST_DATA_TYPE *)ddd) = vvv;				\
		ddd += PIXEL_BYTES;							\
	}												\
}



MYSTATIC void DOWNSCALE_FUNCTION_NAME
(
	unsigned char	*yuv,
	unsigned char	*dst,
	int				Bx,
	int				Cx,
	unsigned char	*pattern,
	int				drb,
	int				yuvrb,
	int				dst_width,
	unsigned char	*sprite_back_buffer
)
{
	int	C1 =  Cx >> FIXED_SUBBITS;						/* Integral   part of contrast */
	int	C2 = (Cx << FIXED_SUBBITS) >> FIXED_SUBBITS;	/* Fractional part of contrast */
	int	dst_stride_plus_drb;
	int	yuv_stride;
	int yuv_width_bytes;
	int src_width;
	unsigned char *sprite_front_buffer = NULL;		//temporarily hold pointer to the screen

	src_width = *((long *)pattern);
	pattern += 4;

	dst_stride_plus_drb	= (drb<<1) - (dst_width<<PIXEL_BYTES_SHIFT);
	yuv_width_bytes		= src_width * 3;
	yuv_stride			= yuvrb - yuv_width_bytes;
	dst -= PIXEL_BYTES;

	while(1)
	{
		int p = *(pattern++);
		//00 10 01 11   00 00 00   01 10 11
		//00 00 00 00   10 01 11   01 10 11
		switch( p )
		{
			case kCommonScalePattern_EOF:
				goto bail;
			case kDownScalePattern_EOL_0:
				yuv += yuvrb;
				break;
			case kDownScalePattern_EOL_1:
				yuv += yuv_stride;
				dst+= dst_stride_plus_drb - drb;
				break;
			case kDownScalePattern_EOL_2:
				yuv += yuv_stride;
				dst += dst_stride_plus_drb;
				break;
			case kDownScalePattern_SKIP://00  top
				yuv += 6;
				break;
			case kCommonPattern_Switch_Buffer_SPRITE:
				sprite_front_buffer = dst;
				dst = sprite_back_buffer - PIXEL_BYTES;
				break;
			case kCommonPattern_Blend_SPRITE:
				ALIGN_4_BYTES( pattern )
				parse_proc(pattern);
				pattern += BLEND_SPRITE_PARAM_BYTES;
				break;
			case kCommonPattern_Copy_Buffer_SPRITE:
				BLEND_AND_COPY
				break;

			case kDownScalePattern_10:	//10
				{						//00
					GET_UV_i
					y = *yuv;
					yuv += 4;
					y = y * C2 + Bx;
					DOWNSCALE_PACK
					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;
				}
				break;
			case kDownScalePattern_9:		//00
				{							//10
					GET_UV_i
					y = *(yuv+2);
					yuv += 4;
					y = y * C2 + Bx;
					DOWNSCALE_PACK
					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;
				}
				break;
			case kDownScalePattern_6:	//01
				{						//00
					GET_UV_i
					y = *(yuv+1);
					yuv += 4;
					y = y * C2 + Bx;
					DOWNSCALE_PACK
					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;
				}
				break;
			case kDownScalePattern_5:	//00
				{						//01
					GET_UV_i
					y = *(yuv+3);
					yuv += 4;
					y = y * C2 + Bx;
					DOWNSCALE_PACK
					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;
				}
				break;
			case kDownScalePattern_14:	//11
				{						//00
					GET_UV_i
					y = (*yuv++);
					y = y * C2 + Bx;
					DOWNSCALE_PACK
					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;

					y = *yuv;
					yuv += 3;
					y = y * C2 + Bx;
					DOWNSCALE_PACK

					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;
				}
				break;
			case kDownScalePattern_13:	//00
				{						//11
					GET_UV_i
					yuv += 2;
					y = *(yuv++);
					y = y * C2 + Bx;
					DOWNSCALE_PACK

					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;

					y = (*yuv++);
					y = y * C2 + Bx;
					DOWNSCALE_PACK

					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;
				}
				break;

			case kDownScalePattern_11:		//10
				{							//10
					GET_UV_i
					y = *yuv;
					yuv += 2;
					y = y * C2 + Bx;
					DOWNSCALE_PACK

					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;

					y = *yuv;
					yuv += 2;
					y = y * C2 + Bx;
					DOWNSCALE_PACK
					*((DST_DATA_TYPE *)(dst + drb)) = pix;
				}
				break;
			case kDownScalePattern_7:	//01
				{						//01
					GET_UV_i
					yuv++;
					y   = *yuv;
					yuv  += 2;
					y = y * C2 + Bx;
					DOWNSCALE_PACK

					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;

					y = *(yuv++);
					y = y * C2 + Bx;
					DOWNSCALE_PACK
					*((DST_DATA_TYPE *)(dst + drb)) = pix;
				}
				break;
			case kDownScalePattern_15:		//11
				{							//11
					GET_UV_i
					y = (*yuv++);
					y = y * C2 + Bx;
					DOWNSCALE_PACK
					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;

					y = (*yuv++);
					y = y * C2 + Bx;
					DOWNSCALE_PACK
					*((DST_DATA_TYPE *)(dst+PIXEL_BYTES)) = pix;

					y = (*yuv++);
					y = y * C2 + Bx;
					DOWNSCALE_PACK
					*((DST_DATA_TYPE *)(dst + drb)) = pix;

					y = (*yuv++);
					y = y * C2 + Bx;
					DOWNSCALE_PACK
					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst + drb)) = pix;
				}
				break;

#ifdef SUPPORT_NEON
			case kDownScalePattern_4x2:			//11, 11
				{								//11, 11
					int repeat = 2;

					while( repeat )
					{
						GET_UV_i
						y = (*yuv++);
						y = y * C2 + Bx;
						DOWNSCALE_PACK
						dst += PIXEL_BYTES;
						*((DST_DATA_TYPE *)dst) = pix;

						y = (*yuv++);
						y = y * C2 + Bx;
						DOWNSCALE_PACK
						*((DST_DATA_TYPE *)(dst+PIXEL_BYTES)) = pix;

						y = (*yuv++);
						y = y * C2 + Bx;
						DOWNSCALE_PACK
						*((DST_DATA_TYPE *)(dst + drb)) = pix;

						y = (*yuv++);
						y = y * C2 + Bx;
						DOWNSCALE_PACK
						dst += PIXEL_BYTES;
						*((DST_DATA_TYPE *)(dst + drb)) = pix;

						repeat--;
					}
				}
				break;
#endif

			case kDownScalePattern_HQ_1://x0
				{						//00
					int yy0, yy1, yy2, yy3;

					GET_UV_i
					yy0 = (*yuv++);
					yy1 = (*yuv++);
					yy2 = (*yuv++);
					yy3 = (*yuv++);
					y = (yy0+yy1+yy2+yy3)>>2;
					y = y * C2 + Bx;
					DOWNSCALE_PACK
					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;
				}
				break;
			case kDownScalePattern_HQ_2_HOR:	//xx
				{						//00
					int yy0, yy1;
					GET_UV_i
					yy0 = *(yuv+0);
					yy1 = *(yuv+2);
					y = (yy0 + yy1 )>>1;
					y = y * C2 + Bx;
					DOWNSCALE_PACK
					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;

					yy0 = *(yuv+1);
					yy1 = *(yuv+3);
					y = (yy0 + yy1 )>>1;
					y = y * C2 + Bx;
					DOWNSCALE_PACK

					yuv += 4;
					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;
				}
				break;
			case kDownScalePattern_HQ_2_VER:	//x0
				{								//x0
					int yy0, yy1;
					GET_UV_i
					yy0 = *(yuv+0);
					yy1 = *(yuv+1);
					y = (yy0 + yy1 )>>1;
					y = y * C2 + Bx;
					DOWNSCALE_PACK

					dst += PIXEL_BYTES;
					*((DST_DATA_TYPE *)dst) = pix;

					yy0 = *(yuv+2);
					yy1 = *(yuv+3);
					y = (yy0 + yy1 )>>1;
					y = y * C2 + Bx;
					DOWNSCALE_PACK

					yuv += 4;
					*((DST_DATA_TYPE *)(dst + drb)) = pix;
				}
				break;
		}
	}

bail:
	;
}

MYSTATIC void UPSCALE_FUNCTION_NAME
(
	unsigned char	*yuv,
	unsigned char	*dst,
	int				Bx,
	int				Cx,
	unsigned char	*pattern,
	int				drb,
	int				yuvrb,
	int				dst_width,
	unsigned char	*sprite_back_buffer

)
{
	int	C1 =  Cx >> FIXED_SUBBITS;						/* Integral   part of contrast */
	int	C2 = (Cx << FIXED_SUBBITS) >> FIXED_SUBBITS;	/* Fractional part of contrast */

	int	yuv_stride;
	int dst_single_stride = (drb - (dst_width<<PIXEL_BYTES_SHIFT));
	int yuv_width_bytes;
	int src_width;
	unsigned char *sprite_front_buffer = NULL;		//temporarily hold pointer to the screen

	src_width = *((long *)pattern);
	pattern += 4;

	yuv_width_bytes = src_width * 3;
	yuv_stride		= yuvrb - yuv_width_bytes;
	dst -= PIXEL_BYTES;

	while(1)
	{
		int p = *(pattern++);

		switch( p )
		{
			case kCommonScalePattern_EOF:
				goto bail;

			case kUpScalePattern_GENERIC_EOL:
				{
					int x = *(pattern++);
					yuv += yuv_stride;
					dst += drb*x + dst_single_stride;
				}
				break;

			case kCommonPattern_Switch_Buffer_SPRITE:
				sprite_front_buffer = dst;
				dst = sprite_back_buffer - PIXEL_BYTES;
				break;

			case kCommonPattern_Copy_Buffer_SPRITE:
				BLEND_AND_COPY
				break;

			case kCommonPattern_Blend_SPRITE:
				ALIGN_4_BYTES( pattern )
				parse_proc(pattern);
				pattern += BLEND_SPRITE_PARAM_BYTES;
				break;

			case kUpScalePattern_GENERIC_BLOCK:
				{
					int rgb[4];
					int x0 = *(pattern++);
					int x1 = *(pattern++);
					int y0 = *(pattern++);
					int y1 = *(pattern++);
					unsigned char *this_dst;
					int this_value;

					GET_UV_i

					UPSCALE_GET_Y_i
					rgb[0] = pix;

					UPSCALE_GET_Y_i
					rgb[1] = pix;

					UPSCALE_GET_Y_i
					rgb[2] = pix;

					UPSCALE_GET_Y_i
					rgb[3] = pix;

					this_dst = dst;
					this_value = rgb[0];
					SET_BLOCK(x0, y0, this_value, this_dst)

					this_dst = dst + PIXEL_BYTES * x0;
					this_value = rgb[1];
					SET_BLOCK(x1, y0, this_value, this_dst)

					this_dst = dst + (drb * y0);
					this_value = rgb[2];
					SET_BLOCK(x0, y1, this_value, this_dst)

					this_dst = dst + (drb * y0) + (PIXEL_BYTES * x0);
					this_value = rgb[3];
					SET_BLOCK(x1, y1, this_value, this_dst)

					dst += PIXEL_BYTES * ( x0 + x1 );
				}
				break;


			//ab		aab			abb			aabb
			//cd		ccd			cdd			ccdd
			case kUpScalePattern_1:	//ab
				{					//cd
					GET_UV_i

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst+0))	   = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst+PIXEL_BYTES))	   = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb)) = pix;
				}
				break;
			case kUpScalePattern_9:	//aab
				{					//ccd
					GET_UV_i

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst+0)) = pix;
										*((DST_DATA_TYPE *)(dst+PIXEL_BYTES)) = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst+(2*PIXEL_BYTES))) = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst + drb)) = pix;
					dst += PIXEL_BYTES; *((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb)) = pix;
				}
				break;
			case kUpScalePattern_5:	//abb
				{					//cdd
					GET_UV_i

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst+0)) = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst+PIXEL_BYTES)) = pix;
										*((DST_DATA_TYPE *)(dst+(2*PIXEL_BYTES))) = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb)) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb)) = pix;
				}
				break;

			case kUpScalePattern_13:	//aabb
				{						//ccdd
					GET_UV_i

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst+0)) = pix;
										*((DST_DATA_TYPE *)(dst+PIXEL_BYTES)) = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst+(2*PIXEL_BYTES))) = pix;
										*((DST_DATA_TYPE *)(dst+(3*PIXEL_BYTES))) = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst + drb)) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb)) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb)) = pix;
				}
				break;

			//ab		aab			abb			aabb
			//ab		aab			abb			aabb
			//cd		ccd			cdd			ccdd
			case kUpScalePattern_3:	//ab
				{					//ab
					GET_UV_i		//cd

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst -= PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
				}
				break;
			case kUpScalePattern_11://aab
				{					//aab
					GET_UV_i		//ccd

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst+0))	   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst -= (2*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
				}
				break;
			case kUpScalePattern_7:	//abb
				{					//abb
					GET_UV_i		//cdd

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))	= pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))	= pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst -= (2*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
				}
				break;

			case kUpScalePattern_15://aabb
				{					//aabb
					GET_UV_i		//ccdd

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))	= pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))	= pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst -= (3*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
				}
				break;

			//ab		aab			abb			aabb
			//cd		ccd			cdd			ccdd
			//cd		ccd			cdd			ccdd
			case kUpScalePattern_2:	//ab
				{					//cd
					GET_UV_i		//cd

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst+0))			 = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst+PIXEL_BYTES))			 = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst + drb	  )) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb	  )) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
				}
				break;
			case kUpScalePattern_10:		//aab
				{							//ccd
					GET_UV_i				//ccd

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst+0)) = pix;
										*((DST_DATA_TYPE *)(dst+PIXEL_BYTES)) = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst+(2*PIXEL_BYTES))) = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst + drb	  )) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb	  )) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb     )) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
				}
				break;
			case kUpScalePattern_6:			//abb
				{							//cdd
					GET_UV_i				//cdd

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst+0)) = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst+PIXEL_BYTES)) = pix;
										*((DST_DATA_TYPE *)(dst+(2*PIXEL_BYTES))) = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst + drb	  )) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb	  )) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb	  )) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
				}
				break;

			case kUpScalePattern_14:		//aabb
				{							//ccdd
					GET_UV_i				//ccdd

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst+0)) = pix;
										*((DST_DATA_TYPE *)(dst+PIXEL_BYTES)) = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst+(2*PIXEL_BYTES))) = pix;
										*((DST_DATA_TYPE *)(dst+(3*PIXEL_BYTES))) = pix;

					UPSCALE_GET_Y_i
										*((DST_DATA_TYPE *)(dst + drb     )) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb     )) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb     )) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + drb     )) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
				}
				break;

			//ab		aab			abb			aabb
			//ab		aab			abb			aabb
			//cd		ccd			cdd			ccdd
			case kUpScalePattern_4:			//ab
				{							//ab
					GET_UV_i				//cd
											//cd
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst -= PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +       (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +       (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
				}
				break;
			case kUpScalePattern_12:		//aab
				{							//aab
					GET_UV_i				//ccd
											//ccd
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst+0))	   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst -= (2*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +       (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +       (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +       (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
				}
				break;
			case kUpScalePattern_8:			//abb
				{							//abb
					GET_UV_i				//cdd
											//cdd
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst -= (2*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +       (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +       (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +       (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
				}
				break;

			case kUpScalePattern_16_4x4:	//aabb
				{							//aabb
					GET_UV_i				//ccdd
											//ccdd
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))	= pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))	= pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + 0))   = pix;
										*((DST_DATA_TYPE *)(dst + drb)) = pix;

					UPSCALE_GET_Y_i
					dst -= (3*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +   (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +       (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +       (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +       (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
				}
				break;

			case kUpScalePattern_4x5:	//aabb
				{						//aabb
										//aabb
					GET_UV_i			//ccdd
										//ccdd
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst -= (3*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
				}
				break;

			case kUpScalePattern_5x4:	//aaabb
				{						//aaabb
					GET_UV_i			//cccdd
										//cccdd
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;

					UPSCALE_GET_Y_i
					dst -= (4*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst       + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst       + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst       + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst       + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +     + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
				}
				break;

			case kUpScalePattern_5x5:	//aaabb
				{						//aaabb
										//aaabb
										//cccdd
										//cccdd
					GET_UV_i

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst -= (4*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
				}
				break;

			case kUpScalePattern_5x6:	//aaabb
				{						//aaabb
										//aaabb
					GET_UV_i			//cccdd
										//cccdd
					UPSCALE_GET_Y_i		//cccdd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst -= (4*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;
				}
				break;

			case kUpScalePattern_4x6:	//aabb
				{						//aabb
										//aabb
					GET_UV_i			//ccdd
										//ccdd
					UPSCALE_GET_Y_i		//ccdd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst -= (3*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;
				}
				break;

			case kUpScalePattern_6x5:	//aaabbb
				{						//aaabbb
										//aaabbb
					GET_UV_i			//cccddd
										//cccddd
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst -= (5*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
				}
				break;

			case kUpScalePattern_6x4:	//aaabbb
				{						//aaabbb
					GET_UV_i			//cccddd
										//cccddd
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;

					UPSCALE_GET_Y_i
					dst -= (5*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst + 0   + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + 0   + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + 0   + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + 0   + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + 0   + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + 0   + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
				}
				break;


			case kUpScalePattern_6x6:	//aaabbb
				{						//aaabbb
										//aaabbb
					GET_UV_i			//cccddd
										//cccddd
					UPSCALE_GET_Y_i		//cccddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst + (0     ))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst -= (5*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst       + (drb<<2))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<2))) = pix;
				}
				break;

			case kUpScalePattern_6x7:	//aaabbb
				{						//aaabbb
										//aaabbb
										//aaabbb
					GET_UV_i			//cccddd
										//cccddd
					UPSCALE_GET_Y_i		//cccddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += (drb<<2);
					dst -= (5*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst + (0)))      = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst -= (drb<<2);
				}
				break;

			case kUpScalePattern_6x8:	//aaabbb
				{						//aaabbb
										//aaabbb
										//aaabbb
					GET_UV_i			//cccddd
										//cccddd
										//cccddd
					UPSCALE_GET_Y_i		//cccddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += (drb<<2);
					dst -= (5*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst + (0)))      = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst -= (drb<<2);
				}
				break;

			case kUpScalePattern_7x6:	//aaaabbb
				{						//aaaabbb
										//aaaabbb
					GET_UV_i			//ccccddd
										//ccccddd
					UPSCALE_GET_Y_i		//ccccddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += 3*drb;
					dst -= (6*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +   0 + (0  <<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +   0 + (0  <<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +   0 + (0  <<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +   0 + (0  <<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +   0 + (0  <<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +   0 + (0  <<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +   0 + (0  <<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +   0 + (drb<<1))) = pix;
					dst -= 3*drb;
				}
				break;

			case kUpScalePattern_7x7:	//aaaabbb
				{						//aaaabbb
										//aaaabbb
										//aaaabbb
					GET_UV_i			//ccccddd
										//ccccddd
					UPSCALE_GET_Y_i		//ccccddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += (drb<<2);
					dst -= (6*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst + (0)))      = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst -= (drb<<2);
				}
				break;

			case kUpScalePattern_7x8:	//aaaabbb
				{						//aaaabbb
										//aaaabbb
										//aaaabbb
					GET_UV_i			//ccccddd
										//ccccddd
										//ccccddd
					UPSCALE_GET_Y_i		//ccccddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += (drb<<2);
					dst -= (6*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +		(0)))      = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst -= (drb<<2);
				}
				break;

			case kUpScalePattern_8x7:	//aaaabbbb
				{						//aaaabbbb
										//aaaabbbb
										//aaaabbbb
					GET_UV_i			//ccccdddd
										//ccccdddd
					UPSCALE_GET_Y_i		//ccccdddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += (drb<<2);
					dst -= (7*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +		(0)))      = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst -= (drb<<2);
				}
				break;

			case kUpScalePattern_8x6://aaaabbbb
				{						//aaaabbbb
										//aaaabbbb
					GET_UV_i			//ccccdddd
										//ccccdddd
					UPSCALE_GET_Y_i		//ccccdddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += drb + (drb<<1);
					dst -= (7*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +		(0)))      = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
					dst -= (drb<<2);
				}
				break;

			case kUpScalePattern_8x8:	//aaaabbbb
				{						//aaaabbbb
										//aaaabbbb
										//aaaabbbb
					GET_UV_i			//ccccdddd
										//ccccdddd
										//ccccdddd
					UPSCALE_GET_Y_i		//ccccdddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += (drb<<2);
					dst -= (7*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +		(0)))      = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst -= (drb<<2);
				}
				break;

			case kUpScalePattern_8x9:	//aaaabbbb
				{						//aaaabbbb
										//aaaabbbb
										//aaaabbbb
										//aaaabbbb
					GET_UV_i			//ccccdddd
										//ccccdddd
										//ccccdddd
					UPSCALE_GET_Y_i		//ccccdddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += 5*drb;
					dst -= (7*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +		(0)))      = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst -= 5*drb;
				}
				break;

			case kUpScalePattern_8x10://aaaabbbb
				{						//aaaabbbb
										//aaaabbbb
										//aaaabbbb
										//aaaabbbb
					GET_UV_i			//ccccdddd
										//ccccdddd
										//ccccdddd
										//ccccdddd
					UPSCALE_GET_Y_i		//ccccdddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += 5*drb;
					dst -= (7*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +		(0)))      = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst -= 5*drb;
				}
				break;

			case kUpScalePattern_9x8:	//aaaaabbbb
				{						//aaaaabbbb
										//aaaaabbbb
										//aaaaabbbb
					GET_UV_i			//cccccdddd
										//cccccdddd
										//cccccdddd
					UPSCALE_GET_Y_i		//cccccdddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += 4*drb;
					dst -= (8*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +		(0)))      = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst -= 4*drb;
				}
				break;

			case kUpScalePattern_9x9:	//aaaaabbbb
				{						//aaaaabbbb
										//aaaaabbbb
										//aaaaabbbb
										//aaaaabbbb
					GET_UV_i			//cccccdddd
										//cccccdddd
										//cccccdddd
					UPSCALE_GET_Y_i		//cccccdddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += 5*drb;
					dst -= (8*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +		(0)))      = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst -= 5*drb;
				}
				break;

			case kUpScalePattern_9x10://aaaaabbbb
				{						//aaaaabbbb
										//aaaaabbbb
										//aaaaabbbb
										//aaaaabbbb
					GET_UV_i			//cccccdddd
										//cccccdddd
										//cccccdddd
										//cccccdddd
					UPSCALE_GET_Y_i		//cccccdddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += 5*drb;
					dst -= (8*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +		(0)))      = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))	   = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))	   = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))	   = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))	   = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))	   = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))	   = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))	   = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))	   = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst -= 5*drb;
				}
				break;

			case kUpScalePattern_10x9://aaaaabbbbb
				{						//aaaaabbbbb
										//aaaaabbbbb
										//aaaaabbbbb
										//aaaaabbbbb
					GET_UV_i			//cccccddddd
										//cccccddddd
										//cccccddddd
					UPSCALE_GET_Y_i		//cccccddddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += 5*drb;
					dst -= (9*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +		(0)))      = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst -= 5*drb;
				}
				break;

			case kUpScalePattern_10x8://aaaaabbbbb
				{						//aaaaabbbbb
										//aaaaabbbbb
										//aaaaabbbbb
					GET_UV_i			//cccccddddd
										//cccccddddd
										//cccccddddd
					UPSCALE_GET_Y_i		//cccccddddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += 4*drb;
					dst -= (9*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +		(0)))      = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
					dst -= 4*drb;
				}
				break;

			case kUpScalePattern_10x10://aaaaabbbbb
				{						 //aaaaabbbbb
										 //aaaaabbbbb
										 //aaaaabbbbb
										 //aaaaabbbbb
					GET_UV_i			 //cccccddddd
										 //cccccddddd
										 //cccccddddd
										 //cccccddddd
					UPSCALE_GET_Y_i		 //cccccddddd
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;	*((DST_DATA_TYPE *)(dst +		(0     ))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
										*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
										*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += 5*drb;
					dst -= (9*PIXEL_BYTES);	*((DST_DATA_TYPE *)(dst +		(0)))      = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst +		(0)))		 = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst +		(drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;

					UPSCALE_GET_Y_i
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst += PIXEL_BYTES;		*((DST_DATA_TYPE *)(dst + (0)))		 = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<0))) = pix;
											*((DST_DATA_TYPE *)(dst + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst + drb + (drb<<1))) = pix;
											*((DST_DATA_TYPE *)(dst +     + (drb<<2))) = pix;
					dst -= 5*drb;
				}
				break;


			default:
				break;
		}
	}

bail:
	;
}


MYSTATIC void GENERIC_SCALE_FUNCTION_NAME
(
	unsigned char	*d0,
	FskFixed		Bx,
	FskFixed		Cx,
	FskFixed		xd,
	unsigned char	*Y0,
	int				width,
	FskFixed		x0,
	FskFixed		y0,
	int				yuvrb,
	int				drb,
	FskFixed		yd,
	int				height
)
{
	int		j, i;
	unsigned char	*Yr, *Y;
	FskFixed		xx,yy;

	unsigned char	*dst;
	int				last_ty = -1;

	int	C102 =  Cx >> FIXED_SUBBITS;					/* Integral   part of contrast */
	int	C2   = (Cx << FIXED_SUBBITS) >> FIXED_SUBBITS;	/* Fractional part of contrast */

	//x0 = (x0>>FWD_SUBBITS)<<FWD_SUBBITS;
	//y0 = (y0>>FWD_SUBBITS)<<FWD_SUBBITS;

	yy = y0;
	for( i = height; i--;)
	{
		//int last_tx = - 1;
		//int last_tcx = - 1;

		if( (yy >> FWD_SUBBITS) == last_ty )
		{
			memcpy( (void *)d0, (void *)(d0-drb), width<<PIXEL_BYTES_SHIFT );
			d0 += drb;
			last_ty = (yy >> FWD_SUBBITS);
			yy += yd;
			continue;
		}
		last_ty = (yy >> FWD_SUBBITS);
		Yr = Y0 + (yy >> CHR_SUBBITS) * yuvrb;

		dst = d0;
		xx   = x0;
		for( j = width; j--; )
		{
			int			y;
			int			up;
			int			vp;
			int			pix;
			register int	p;
			int uvr, uvb, uvg;
			int offset;

			Y = Yr + ( xx >> CHR_SUBBITS) * 6;
			offset = ((xx >> FWD_SUBBITS) & 1) | ((yy >> (FWD_SUBBITS-1)) & 2);

			up = *Y;  Y++;
			vp = *Y;  Y++;
			y = Y[offset];
			y = y * C2 + Bx;

			up = up - 128;
			vp = vp - 128;

			{
				int	t;

				t   = up*(C102>>1);
				uvg = vp*C102      + t;			//102/2=51=>52, 102=>104
				uvb = up*(C102<<1) + t;			//51+204=>255
				uvr = uvg - t;					//102*2=204
			}

			DOWNSCALE_PACK

			*(DST_DATA_TYPE *)dst = pix;
			dst += PIXEL_BYTES;

			xx += xd;
		}

		d0 += drb;
		yy += yd;
	}
}

#endif



#ifdef YUV420i_RGB_NEON1_C_IMPLEMENTATION

MYSTATIC void UPSCALE_FUNCTION_NAME_NEON1
(
	unsigned char	*yuv,
	unsigned char	*dst,
	int				Bx,
	int				Cx,
	unsigned char	*pattern,
	int				drb,
	int				yuvrb,
	int				dst_width,
	unsigned char	*sprite_back_buffer

)
{
	int	C1 =  Cx >> FIXED_SUBBITS;						/* Integral   part of contrast */
	int	C2 = (Cx << FIXED_SUBBITS) >> FIXED_SUBBITS;	/* Fractional part of contrast */

	int	yuv_stride;
	int dst_single_stride = (drb - (dst_width<<PIXEL_BYTES_SHIFT));
	int yuv_width_bytes;
	int src_width;
	int pix0, pix1, pix2, pix3, pix4, pix5, pix6, pix7;
	unsigned char *sprite_front_buffer = NULL;		//temporarily hold pointer to the screen
	unsigned char	*dst0;
	unsigned char	*dst1;
	int v0, v1;

	src_width = *((long *)pattern);
	pattern += 4;

	v0 = *(pattern++);
	v1 = *(pattern++);

	yuv_width_bytes = src_width * 3;
	yuv_stride		= yuvrb - yuv_width_bytes;

	dst0  = dst;
	dst1  = dst0 + v0 * drb;

	while(1)
	{
				int p = *(pattern++);

		switch( p )
		{
			case kCommonScalePattern_EOF:
				goto bail;

			case kUpScalePattern_GENERIC_EOL:
				{
					int x = v0 + v1 - 1;

					v0 = *(pattern++);
					v1 = *(pattern++);

					yuv  += yuv_stride;
					dst0 = dst0 + drb*x + dst_single_stride;
					dst1 = dst0 + v0 * drb;
				}
				break;

			case kUpScalePattern_GENERIC_BLOCK:
				{
					int rgb[4];
					int h0 = *(pattern++);
					int h1 = *(pattern++);
					unsigned char *this_dst;
					int this_value;

						GET_UV_i

					UPSCALE_GET_Y_i
					rgb[0] = pix;

					UPSCALE_GET_Y_i
					rgb[1] = pix;

					UPSCALE_GET_Y_i
					rgb[2] = pix;

					UPSCALE_GET_Y_i
					rgb[3] = pix;

					this_dst = dst0;
					this_value = rgb[0];
					SET_BLOCK2(h0, v0, this_value, this_dst)

					this_dst = dst0 + PIXEL_BYTES * h0;
					this_value = rgb[1];
					SET_BLOCK2(h1, v0, this_value, this_dst)

					this_dst = dst1;
					this_value = rgb[2];
					SET_BLOCK2(h0, v1, this_value, this_dst)

					this_dst = dst1 + (PIXEL_BYTES * h0);
					this_value = rgb[3];
					SET_BLOCK2(h1, v1, this_value, this_dst)

					dst0 += PIXEL_BYTES * ( h0 + h1 );
					dst1 += PIXEL_BYTES * ( h0 + h1 );
				}
				break;


			case kUpScalePattern_GENERIC_COPY_BOTH_EOL:
				{
					unsigned char *ddd;

					yuv += yuv_stride;

					dst0 = dst0 + dst_single_stride - drb;
					dst1 = dst1 + dst_single_stride - drb;
					if( v0 > 1 )
					{
						v0--;
						ddd = dst0 + drb;
						while(v0--)
						{
							memcpy( ddd, dst0, dst_width<<PIXEL_BYTES_SHIFT );
							ddd += drb;
						}
					}

					ddd = dst1 + drb;
					if( v1 > 1 )
					{
						v1--;
						while(v1--)
						{
							memcpy( ddd, dst1, dst_width<<PIXEL_BYTES_SHIFT );
							ddd += drb;
						}
					}

					dst0 = ddd	;
					v0 = *(pattern++);

					dst1 = ddd + drb * v0;
					v1 = *(pattern++);
				}
				break;

			case kUpScalePattern_GENERIC_COPY_TOP_EOL:
				{
					unsigned char *ddd;
					int vvv;

					yuv += yuv_stride;

					dst0 = dst0 + dst_single_stride - drb;
					dst1 = dst1 + dst_single_stride - drb;
					vvv = v0;
					if( vvv > 1 )
					{
						vvv--;
						ddd = dst0 + drb;
						while(vvv--)
						{
							memcpy( ddd, dst0, dst_width<<PIXEL_BYTES_SHIFT );
							ddd += drb;
						}
					}

					dst0 += (v0 + v1)*drb;
					v0 = *(pattern++);

					dst1 = dst0 + drb * v0;
					v1 = *(pattern++);
				}
				break;

			case kUpScalePattern_GENERIC_COPY_BOT_EOL:
				{
					unsigned char *ddd;
					int vvv;

					yuv += yuv_stride;

					dst0 = dst0 + dst_single_stride - drb;
					dst1 = dst1 + dst_single_stride - drb;

					ddd = dst1 + drb;
					vvv = v1;
					if( vvv > 1 )
					{
						vvv--;
						while(vvv--)
						{
							memcpy( ddd, dst1, dst_width<<PIXEL_BYTES_SHIFT );
							ddd += drb;
						}
					}

					dst0 += (v0 + v1)*drb;
					v0 = *(pattern++);

					dst1 = dst0 + drb * v0;
					v1 = *(pattern++);
				}
				break;

			case kCommonPattern_Switch_Buffer_SPRITE:
				sprite_front_buffer = dst0;
				dst0 = sprite_back_buffer - PIXEL_BYTES;
				break;

			case kCommonPattern_Copy_Buffer_SPRITE:
				BLEND_AND_COPY
				break;

			case kCommonPattern_Blend_SPRITE:
				ALIGN_4_BYTES( pattern )
				parse_proc(pattern);
				pattern += BLEND_SPRITE_PARAM_BYTES;
				break;

			case kUpScalePattern_Neon1_GENERIC_2x1_TOP:
				{
					int rgb[2];
					int x0 = *(pattern++);
					int x1 = *(pattern++);
					unsigned char *this_dst;
					int this_value;

					GET_UV_i

					UPSCALE_GET_Y_i
					rgb[0] = pix;

					UPSCALE_GET_Y_i
					rgb[1] = pix;

					yuv += 2;

					this_dst = dst0;
					this_value = rgb[0];
					REPEAT_PIX(x0, this_value, this_dst)

					this_dst = dst0 + PIXEL_BYTES * x0;
					this_value = rgb[1];
					REPEAT_PIX(x1, this_value, this_dst)

					dst0 += PIXEL_BYTES * ( x0 + x1 );
					dst1 += PIXEL_BYTES * ( x0 + x1 );
				}
				break;

			case kUpScalePattern_Neon1_GENERIC_2x1_BOT:
				{
					int rgb[2];
					int x0 = *(pattern++);
					int x1 = *(pattern++);
					unsigned char *this_dst;
					int this_value;

					GET_UV_i

					yuv += 2;

					UPSCALE_GET_Y_i
					rgb[0] = pix;

					UPSCALE_GET_Y_i
					rgb[1] = pix;

					this_dst = dst1;
					this_value = rgb[0];
					REPEAT_PIX(x0, this_value, this_dst)

					this_dst = dst1 + PIXEL_BYTES * x0;
					this_value = rgb[1];
					REPEAT_PIX(x1, this_value, this_dst)

					dst0 += PIXEL_BYTES * ( x0 + x1 );
					dst1 += PIXEL_BYTES * ( x0 + x1 );
				}
				break;

			case kUpScalePattern_Neon1_GENERIC_2x2:
				{
					int rgb[4];
					int x0 = *(pattern++);
					int x1 = *(pattern++);
					unsigned char *this_dst;
					int this_value;

					GET_UV_i

					UPSCALE_GET_Y_i
					rgb[0] = pix;

					UPSCALE_GET_Y_i
					rgb[1] = pix;

					UPSCALE_GET_Y_i
					rgb[2] = pix;

					UPSCALE_GET_Y_i
					rgb[3] = pix;

					this_dst = dst0;
					this_value = rgb[0];
					REPEAT_PIX(x0, this_value, this_dst)

					this_dst = dst0 + PIXEL_BYTES * x0;
					this_value = rgb[1];
					REPEAT_PIX(x1, this_value, this_dst)

					this_dst = dst1;
					this_value = rgb[2];
					REPEAT_PIX(x0, this_value, this_dst)

					this_dst = dst1 + PIXEL_BYTES * x0;
					this_value = rgb[3];
					REPEAT_PIX(x1, this_value, this_dst)

					dst0 += PIXEL_BYTES * ( x0 + x1 );
					dst1 += PIXEL_BYTES * ( x0 + x1 );
				}
				break;

			//color converting and repeating cases
			case kUpScalePattern_Neon1_4:	//01 45
				{							//23 67
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_5:	//01 455
				{							//23 677
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_6:	//011 455
				{							//233 677
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_7:	//0011 455
				{							//2233 677
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_8:	//0011 4455
				{							//2233 6677
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_9:	//0011 44555
				{							//2233 66777
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_10:	//00111 44555
				{							//22331 66777
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_11:	//000111 44555
				{							//222331 66777
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_12:	//000111 444555
				{							//222331 666777
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_13:	//000111 4445555
				{							//222333 6667777
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_14:	//0001111 4445555
				{							//2223333 6667777
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;
			case kUpScalePattern_Neon1_15:	//00001111 4445555
				{							//22223333 6667777
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_16:	//00001111 44445555
				{							//22223333 66667777
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_17:	//00001111 444455555
				{							//22223333 666677777
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_18:	//000011111 444455555
				{							//222233333 666677777
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_19:	//0000011111 444455555
				{							//2222233333 666677777
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_20:	//0000011111 4444455555
				{							//2222233333 6666677777
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_21:	//0000011111 44444555555
				{							//2222233333 66666777777
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_4x2:	//01 45
				{							//23 67
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES	;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_4x3:	//01 45
				{							//23 67
					GET_Y_i_4x2				//23 67

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 4*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					dst1 -= drb;

				}
				break;

			case kUpScalePattern_Neon1_5x2:	//01 455
				{							//23 677
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_5x3:	//01 455
				{							//23 677
					GET_Y_i_4x2				//23 677

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 5*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					dst1 -= drb;
				}
				break;


			case kUpScalePattern_Neon1_5x4:	//01 455
				{							//01 455
					GET_Y_i_4x2				//23 677
											//23 677
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					dst0 += drb - 5*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					dst0 -= drb;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 5*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					dst1 -= drb;
				}
				break;


			case kUpScalePattern_Neon1_6x2:	//011 455
				{							//233 677
					GET_Y_i_4x2

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
				*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
				}
				break;

			case kUpScalePattern_Neon1_6x3:	//011 455
				{							//233 677
					GET_Y_i_4x2				//233 677

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 6*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					dst1 -= drb;
				}
				break;


			case kUpScalePattern_Neon1_6x4:	//011 455
				{							//011 455
					GET_Y_i_4x2				//233 677
											//233 677
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					dst0 += drb - 6*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					dst0 -= drb;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 6*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					dst1 -= drb;
				}
				break;

			case kUpScalePattern_Neon1_7x3:	//0011 455
				{							//2233 677
					GET_Y_i_4x2				//2233 677

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 7*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					dst1 -= drb;
				}
				break;


			case kUpScalePattern_Neon1_7x4:	//0011 455
				{							//0011 455
					GET_Y_i_4x2				//2233 677
											//2233 677
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					dst0 += drb - 7*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					dst0 -= drb;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 7*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					dst1 -= drb;
				}
				break;

			case kUpScalePattern_Neon1_7x5:	//0011 455
				{							//0011 455
					GET_Y_i_4x2				//2233 677
											//2233 677
											//2233 677

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					dst0 += drb - 7*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					dst0 -= drb;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 7*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 7*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 -= 2*drb;
				}
				break;

			case kUpScalePattern_Neon1_8x3:	//0011 4455
				{							//2233 6677
					GET_Y_i_4x2				//2233 6677

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 8*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					dst1 -= drb;
				}
				break;


			case kUpScalePattern_Neon1_8x4:	//0011 4455
				{							//0011 4455
					GET_Y_i_4x2				//2233 6677
											//2233 6677
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					dst0 += drb - 8*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					dst0 -= drb;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 8*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					dst1 -= drb;
				}
				break;

			case kUpScalePattern_Neon1_8x5:	//0011 4455
				{							//0011 4455
					GET_Y_i_4x2				//2233 6677
											//2233 6677
											//2233 6677

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					dst0 += drb - 8*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					dst0 -= drb;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 8*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 8*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 -= 2*drb;
				}
				break;

			case kUpScalePattern_Neon1_9x4:	//0011 44555
				{							//0011 44555
					GET_Y_i_4x2				//2233 66777
											//2233 66777
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					dst0 += drb - 8*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					dst0 -= drb;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 8*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					dst1 -= drb;
				}
				break;

			case kUpScalePattern_Neon1_9x5:	//0011 44555
				{							//0011 44555
					GET_Y_i_4x2				//2233 66777
											//2233 66777
											//2233 66777

					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					dst0 += drb - 8*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					dst0 -= drb;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 8*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 8*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 -= 2*drb;
				}
				break;

			case kUpScalePattern_Neon1_9x6:	//0011 44555
				{							//0011 44555
					GET_Y_i_4x2				//0011 44555
											//2233 66777
											//2233 66777
											//2233 66777
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					dst0 += drb - 9*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					dst0 += drb - 9*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix0;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix1;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix4;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst0)) = pix5;	dst0 += PIXEL_BYTES;

					dst0 -= 2*drb;

					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 9*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 += drb - 9*PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix2;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix3;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix6;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;
					*((DST_DATA_TYPE *)(dst1)) = pix7;	dst1 += PIXEL_BYTES;

					dst1 -= 2*drb;
				}
				break;



			default:
				break;
		}
	}

bail:
	;
}

#endif


#ifdef YUV420i_PATCHES

extern FskRectTransferProc
GetUnityProcByPixFormat(UInt32 srcIndex, UInt32 dstIndex, UInt32 mode, UInt32 alpha);

#ifdef OVERRIDE_QUALITY_DOWN_SCALE
extern	int g_downscale_hight_quality;
#endif

void PATCH_FUNCTION_NAME_SCALE_BC(const FskRectBlitParams *p)
{
	unsigned char	*dst	= (UInt8*)(p->dstBaseAddr);
	unsigned char	*yuv	= (UInt8*)(p->srcBaseAddr );
	int				yuvrb	= p->srcRowBytes;
	int				drb		= p->dstRowBytes;
	FskFixed		x0		= p->srcX0;
	FskFixed		y0		= p->srcY0;
	FskFixed		dx		= p->srcXInc;
	FskFixed		dy		= p->srcYInc;
	int			dst_width   = p->dstWidth;
	int			dst_height  = p->dstHeight;
	int				Bx,  Cx;
	int				background_changed = has_context_change( p );
	//int			sprite_changed  = 0;
	int				downscale_hight_quality = 0;
	FskVideoSpriteWorld	spriteWorld;

#ifdef HIGH_QUALITY_DOWN_SCALE
	downscale_hight_quality = 1;
#endif

#ifdef OVERRIDE_QUALITY_DOWN_SCALE
	downscale_hight_quality = g_downscale_hight_quality;
#endif

	if(dst_height == 0 || FskFloorFixedToInt(dx) < 0 || FskFloorFixedToInt(dy) < 0 )
		return;

	{
		int	constrast  = p->contrast;
		int	brightness = p->brightness;
		int C88, B88;

		if( constrast > 0 )
			C88   = 32 + ((constrast*3)>>12);
		else
			C88   = 32 + (constrast>>11);

		B88 = ( brightness / 16 + 88 * ( 32 - C88 ));
		Bx  = ((B88 - (16 * C88)) * 149);
		Cx  = ((C88*102)<<16)|(C88 * 149);
	}

	spriteWorld = (FskVideoSpriteWorld)p->sprites;
	if( spriteWorld != NULL )
	{
		FskVideoSprite w;

		for (w = spriteWorld->sprites; NULL != w; w = w->next)
		{
			w->blitter = GetUnityProcByPixFormat(w->pixelFormat, p->dstPixelFormat, w->mode, 255);
			w->p.srcXInc = FWD_ONE;
			w->p.srcYInc = FWD_ONE;
			w->p.srcX0   = 0;
			w->p.srcY0   = 0;
			w->p.srcRowBytes = w->row_bytes;
			w->p.dstRowBytes = p->dstRowBytes;
			//w->p.dstBaseAddr = dst;
			//w->p.srcBaseAddr = src;
			//w->p.srcWidth  = width;
			//w->p.dstWidth  = width;
			//w->p.srcHeight = height;
			//w->p.dstHeight = height;
		}
	}

	if(  dx >=  FWD_ONE / 128 )
	{
		if(background_changed)
		{
			int err = -1;

			g_clipped_dst_width = dst_width;
			err = get_2x2_scale_pattern( dst_width,dst_height,x0,y0,dx,dy,
					&g_blockPatternPtr, &g_pattern_buf_size,
					&g_clipped_dst_width, &g_current_is_upscale );
			if( err != 0 || g_blockPatternPtr == NULL )
				goto generic_case;

			if( g_current_is_upscale == 0 && downscale_hight_quality )
				upgrade_2x2_downscale_pattern(	g_blockPatternPtr );

			g_current_pattern	   = g_blockPatternPtr;
			g_current_pattern_size = g_pattern_buf_size;
		}

		if( NULL != spriteWorld && (spriteWorld->updated || background_changed ) )
		{
			int updated = 0;

			spriteWorld->updated = false;

			add_sprites
			(
				PIXEL_BYTES,
				g_current_is_upscale,
				dy,
				(x0/dx),
				(y0/dy),
				g_clipped_dst_width,
				p->dstHeight,
				p->dstRowBytes,
				p->srcHeight,
				spriteWorld->sprites,
				g_blockPatternPtr,
				g_pattern_buf_size,
				&g_sprite_blockPatternPtr,
				&g_sprite_pattern_buf_size,
				&g_sprite_back_buffer,
				&g_sprite_back_buffer_x,
				&g_sprite_back_buffer_size,
				&updated
			);

			if( updated )
			{
				g_current_pattern	   = g_sprite_blockPatternPtr;
				g_current_pattern_size = g_sprite_pattern_buf_size;
			}
		}

#ifdef YUV420
		//Information for YUV420 planar
		unsigned char	*y	= (UInt8*)(p->srcBaseAddr );
		unsigned char	*u	= (UInt8*)(p->srcUBaseAddr);
		unsigned char	*v	= (UInt8*)(p->srcVBaseAddr);
		int				yrb	= p->srcRowBytes;
		int             crb	= yrb >> 1;

		if( x0 >= FWD_ONE || y0 >= FWD_ONE )
		{
			y += (y0 >> FWD_SUBBITS) * yrb + (x0 >> FWD_SUBBITS);
			u += (y0 >> CHR_SUBBITS) * crb + (x0 >> CHR_SUBBITS);
			v += (y0 >> CHR_SUBBITS) * crb + (x0 >> CHR_SUBBITS);
		}

		unsigned char *yuv_pack[3];
		yuv_pack[0] = y;
		yuv_pack[1] = u;
		yuv_pack[2] = v;

		yuv = yuv_pack;
#else
		if( x0 >= FWD_ONE || y0 >= FWD_ONE )
			yuv += (y0 >> CHR_SUBBITS) * yuvrb + (x0 >> CHR_SUBBITS) * 6;
#endif

		if(  g_current_is_upscale == 0 )
			DOWN_SCALE_BC_i_CASE_FUNCTION_NAME
			( yuv,dst,Bx,Cx,g_current_pattern,drb,yuvrb,g_clipped_dst_width, g_sprite_back_buffer_x);
		else
			UP_SCALE_BC_i_CASE_FUNCTION_NAME
			( yuv,dst,Bx,Cx,g_current_pattern,drb,yuvrb,g_clipped_dst_width, g_sprite_back_buffer_x);

		return;
	}

generic_case:
		reset_change_context_();
		GENERIC_SCALE_BC_i_CASE_FUNCTION_NAME
		( dst, Bx, Cx, dx, yuv,  dst_width, x0, y0, yuvrb, drb, dy, dst_height );
}

#endif
