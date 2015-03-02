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
#ifndef __FSKYUV420COPY__
#define __FSKYUV420COPY__


#ifndef __FSK__
	#include "Fsk.h"
#endif /* __FSK__ */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "FskBitmap.h"

#ifndef SUPPORT_NEON
//#define SUPPORT_NEON
#endif

#define ALIGN_4_BYTES( p )						\
{												\
	int	pat = (int)(p+3);						\
												\
	p = (unsigned char *)((pat>>2)<<2 );		\
}

#define ALIGN_2_BYTES( p )						\
{												\
	int	pat = (int)(p+1);						\
												\
	p = (unsigned char *)((pat>>1)<<1 );		\
}

#if APP_MODE
#define MY_STATIC
#else
#define MY_STATIC static
#endif

#define IS_WITHIN( a, b, c )  (a > b && a < c )

#define kMaxSpriteBackBufferHeight0				4
#define SWITCH_BUFFER_BYTES						1
#define BLEND_SPRITE_PARAM_BYTES				16
#define COPY_SPRITE_PARAM_BYTES					8 

#define kCommonScalePattern_EOF					0
#define kCommonPattern_Switch_Buffer_SPRITE     19
#define kCommonPattern_Copy_Buffer_SPRITE		20
#define kCommonPattern_Blend_SPRITE				21
#define kUpScalePatternReserve0					22	
#define kUpScalePatternReserve1					23		
#define kUpScalePatternReserve2					24		
#define kUpScalePattern_GENERIC_EOL				25
#define kUpScalePattern_GENERIC_BLOCK			26

//down scale algorithm

#define kDownScaleShape___0			PACK_4(0,0,0,0)
#define kDownScaleShape___1			PACK_4(0,0,0,1)							
#define kDownScaleShape___2			PACK_4(0,0,1,0)
#define kDownScaleShape___3			PACK_4(0,0,1,1)
#define kDownScaleShape___4			PACK_4(0,1,0,0)
#define kDownScaleShape___5			PACK_4(0,1,0,1)
#define kDownScaleShape___6			PACK_4(0,1,1,0)
#define kDownScaleShape___7			PACK_4(0,1,1,1)
#define kDownScaleShape___8			PACK_4(1,0,0,0)
#define kDownScaleShape___9			PACK_4(1,0,0,1)
#define kDownScaleShape___10		PACK_4(1,0,1,0)
#define kDownScaleShape___11		PACK_4(1,0,1,1)
#define kDownScaleShape___12		PACK_4(1,1,0,0)
#define kDownScaleShape___13		PACK_4(1,1,0,1)
#define kDownScaleShape___14		PACK_4(1,1,1,0)
#define kDownScaleShape___15		PACK_4(1,1,1,1)

#ifdef SUPPORT_NEON
#define NEON_2x2_CASE_near				2
#define NEON_2x2_CASE_far				0x7fffffff
#endif

#define GENERIC_BLOCK_FLAG	0x80
#define GENERIC_BLOCK_MASK	0x7f


enum
{
	//kScalePattern_EOF = 0,			//0  h0,h1,v0,v1
	kDownScalePattern_EOL_0 = 1,		//1  xx00	0, 4, 8, 12	
	kDownScalePattern_SKIP,				//4  00xx	1,2,3		
	kDownScalePattern_5,	        	//5  0101	5
	kDownScalePattern_6,	        	//6  0110	6
	kDownScalePattern_EOL_1,			//2  xx0x				
	kDownScalePattern_7,           		//7  0111	7
	kDownScalePattern_9,	         	//8  1001	9
	kDownScalePattern_HQ_1,				//high quality mode	1 in 4: 1, 2, 4, 8
	kDownScalePattern_HQ_2_HOR,			//					2 in 4 hor: 3, 12
	kDownScalePattern_HQ_2_VER,			//					2 in 4 ver: 5, 10
	kDownScalePattern_10,				//9  1010	10
	kDownScalePattern_11,	         	//10 1011	11
	kDownScalePattern_EOL_2,			//3  xx11				
	kDownScalePattern_13,				//11 1101	13
	kDownScalePattern_14,	         	//12 1110	14
	kDownScalePattern_15,            	//13 1111	15

#ifdef SUPPORT_NEON
	kDownScalePattern_4x2,				//17
#else
	kDownScalePattern_reserved_0,	    //17
#endif
	kDownScalePattern_reserved_1        //18
	//kDownScalePattern_Switch_Buffer_SPRITE = 19, //common
	//kDownScalePattern_Copy_Buffer_SPRITE,	
	//kDownScalePattern_Blend_SPRITE,		
};

//up scale algorithm
enum
{
	//kScalePattern_EOF = 0,
	kUpScalePattern_1 = 1,			//
	kUpScalePattern_2,				//
	kUpScalePattern_3,				//
	kUpScalePattern_4,				// 
	kUpScalePattern_5,				//
	kUpScalePattern_6,				// 
	kUpScalePattern_7,				//
	kUpScalePattern_8,				// 
	kUpScalePattern_9,				//
	kUpScalePattern_10,				// 
	kUpScalePattern_11,				//
	kUpScalePattern_12,				// 
	kUpScalePattern_13,				//
	kUpScalePattern_14,				// 
	kUpScalePattern_15,				//
	kUpScalePattern_16_4x4,			// 
	kUpScalePattern_reserved_0,		//17
	kUpScalePattern_reserved_1,		//18
	//kUpScalePattern_Switch_Buffer_SPRITE,//19 common 
	//kUpScalePattern_Copy_Buffer_SPRITE,	//20 common
	//kUpScalePattern_Blend_SPRITE,			//21 common

	//kUpScalePatternReserve0,				//22,		
	//kUpScalePatternReserve1,				//23,		
	//kUpScalePatternReserve2,				//24,		

	//kUpScalePattern_GENERIC_EOL,			//25
	//kUpScalePattern_GENERIC_BLOCK,		//26
												
	kUpScalePattern_4x5 = 27,	//4x5
	kUpScalePattern_4x6,		//4x6
	kUpScalePattern_5x4,		//5x4
	kUpScalePattern_5x5,		//5x5
	kUpScalePattern_5x6,		//5x6
	kUpScalePattern_6x4,		//6x4
	kUpScalePattern_6x5,		//6x5
	kUpScalePattern_6x6,		//6x6
	kUpScalePattern_6x7,		//6x7
	kUpScalePattern_6x8,		//6x8
	kUpScalePattern_7x6,		//7x6
	kUpScalePattern_7x7,		//7x7
	kUpScalePattern_7x8,		//7x8
	kUpScalePattern_8x6,		//8x6
	kUpScalePattern_8x7,		//8x7
	kUpScalePattern_8x8,		//8x8
	kUpScalePattern_8x9,		//8x9
	kUpScalePattern_8x10,		//8x10
	kUpScalePattern_9x8,		//9x8
	kUpScalePattern_9x9,		//9x9
	kUpScalePattern_9x10,		//9x10
	kUpScalePattern_10x8,		//10x8
	kUpScalePattern_10x9,		//10x9
	kUpScalePattern_10x10		//10x10	==>50
};



//up scale algorithm 1

#define kUpScalePattern_Neon1_HScale_Max		21
#define kUpScalePattern_Neon1_VScale_Max		11
#define kUpScalePattern_Neon1_EOL_Offset		25
#define kUpScalePattern_Neon1_Width_Offset	3

enum
{
	//kScalePattern_EOF = 0,
	kUpScalePattern_Neon1_4 = 1,		//1 + 3 = 4
	kUpScalePattern_Neon1_5,			//2
	kUpScalePattern_Neon1_6,			//3
	kUpScalePattern_Neon1_7,			//4 
	kUpScalePattern_Neon1_8,			//5
	kUpScalePattern_Neon1_9,			//6 
	kUpScalePattern_Neon1_10,			//7
	kUpScalePattern_Neon1_11,			//8 
	kUpScalePattern_Neon1_12,			//9
	kUpScalePattern_Neon1_13,			//10 
	kUpScalePattern_Neon1_14,			//11
	kUpScalePattern_Neon1_15,			//12 
	kUpScalePattern_Neon1_16,			//13
	kUpScalePattern_Neon1_17,			//14 
	kUpScalePattern_Neon1_18,			//15
	kUpScalePattern_Neon1_19,			//16 
	kUpScalePattern_Neon1_20,			//17 
	kUpScalePattern_Neon1_21,			//18 + 3  = 21

	//kUpScalePattern_Switch_Buffer_SPRITE,//19 common 
	//kUpScalePattern_Copy_Buffer_SPRITE,	//20 common
	//kUpScalePattern_Blend_SPRITE,		//21 common

	kUpScalePattern_GENERIC_COPY_BOTH_EOL = 22,	//22,		
	kUpScalePattern_GENERIC_COPY_TOP_EOL  = 23,
	kUpScalePattern_GENERIC_COPY_BOT_EOL  = 24,

	//kUpScalePattern_GENERIC_EOL,			//25
	//kUpScalePattern_GENERIC_BLOCK,		//26
	kUpScalePattern_Neon1_4x2 = 27,			//27
	kUpScalePattern_Neon1_4x3,				//28
	kUpScalePattern_Neon1_5x2,				//29
	kUpScalePattern_Neon1_5x3,				//30
	kUpScalePattern_Neon1_5x4,				//31
	kUpScalePattern_Neon1_6x2,				//32
	kUpScalePattern_Neon1_6x3,				//33
	kUpScalePattern_Neon1_6x4,				//34
	kUpScalePattern_Neon1_7x3,				//35
	kUpScalePattern_Neon1_7x4,				//36
	kUpScalePattern_Neon1_7x5,				//37
	kUpScalePattern_Neon1_8x3,				//38
	kUpScalePattern_Neon1_8x4,				//39
	kUpScalePattern_Neon1_8x5,				//40
	kUpScalePattern_Neon1_9x4,				//41
	kUpScalePattern_Neon1_9x5,				//42
	kUpScalePattern_Neon1_9x6,				//43
	kUpScalePattern_Neon1_GENERIC_2x1_TOP,	//44	
	kUpScalePattern_Neon1_GENERIC_2x1_BOT,	//45
	kUpScalePattern_Neon1_GENERIC_2x2		//46
};



#define PACK_4(h0, h1, v0, v1 ) ( (h0<<3)+(h1<<2)+(v0<<1)+(v1<<0) )

//order of 4 bits in the 2x2 matrix for building up mask and pattern
//a, b
//c, d

//												
//1			2			3			4			
//ab		aab			abb			aabb		
//cd		ccd			cdd			ccdd		

//5			6			7			8			
//ab		aab			abb			aabb		
//ab		aab			abb			aabb		
//cd		ccd			cdd			ccdd		

//the following can be done by copying the last line 
//from the above defined cases
//9 		10			11			2
//ab		aab			abb			aabb		
//cd		ccd			cdd			ccdd		
//cd		ccd			cdd			ccdd		

//13		14			15			16
//ab		aab			abb			aabb	
//ab		aab			abb			aabb	
//cd		ccd			cdd			ccdd	
//cd		ccd			cdd			ccdd	
//											
								     //v1,v0,h1,h0 
#define kUpScaleShape___1			( PACK_4(0,0,0,0) + PACK_4(2,2,2,2) )	//#
#define kUpScaleShape___2			( PACK_4(0,0,0,1) + PACK_4(2,2,2,2) )	//=>2								
#define kUpScaleShape___3			( PACK_4(0,0,1,0) + PACK_4(2,2,2,2) )	//#
#define kUpScaleShape___4			( PACK_4(0,0,1,1) + PACK_4(2,2,2,2) )	//#
#define kUpScaleShape___5			( PACK_4(0,1,0,0) + PACK_4(2,2,2,2) )	//=>4
#define kUpScaleShape___6			( PACK_4(0,1,0,1) + PACK_4(2,2,2,2) )	//=>6
#define kUpScaleShape___7			( PACK_4(0,1,1,0) + PACK_4(2,2,2,2) )	//=>5
#define kUpScaleShape___8			( PACK_4(0,1,1,1) + PACK_4(2,2,2,2) )	//=>7
#define kUpScaleShape___9			( PACK_4(1,0,0,0) + PACK_4(2,2,2,2) )	//#
#define kUpScaleShape___10		( PACK_4(1,0,0,1) + PACK_4(2,2,2,2) )	//=>5
#define kUpScaleShape___11		( PACK_4(1,0,1,0) + PACK_4(2,2,2,2) )	//#
#define kUpScaleShape___12		( PACK_4(1,0,1,1) + PACK_4(2,2,2,2) )	//#
#define kUpScaleShape___13		( PACK_4(1,1,0,0) + PACK_4(2,2,2,2) ) //#
#define kUpScaleShape___14		( PACK_4(1,1,0,1) + PACK_4(2,2,2,2) )	//=>12
#define kUpScaleShape___15		( PACK_4(1,1,1,0) + PACK_4(2,2,2,2) )	//#
														
#define kUpScaleShape___16		( PACK_4(0,0,0,0) + PACK_4(3,3,3,3) )	//#
#define kUpScaleShape___17		( PACK_4(0,0,0,1) + PACK_4(3,3,3,3) )	//=>15
#define kUpScaleShape___18		( PACK_4(0,0,1,0) + PACK_4(3,3,3,3) )	//#
#define kUpScaleShape___19		( PACK_4(0,0,1,1) + PACK_4(3,3,3,3) ) //#
#define kUpScaleShape___20		( PACK_4(0,1,0,0) + PACK_4(3,3,3,3) )	//=>17
#define kUpScaleShape___21		( PACK_4(0,1,0,1) + PACK_4(3,3,3,3) )	//=>19
#define kUpScaleShape___22		( PACK_4(0,1,1,0) + PACK_4(3,3,3,3) )	//=>18
#define kUpScaleShape___23		( PACK_4(0,1,1,1) + PACK_4(3,3,3,3) )	//=>20
#define kUpScaleShape___24		( PACK_4(1,0,0,0) + PACK_4(3,3,3,3) )	//#
#define kUpScaleShape___25		( PACK_4(1,0,0,1) + PACK_4(3,3,3,3) )	//=>18
#define kUpScaleShape___26		( PACK_4(1,0,1,0) + PACK_4(3,3,3,3) )	//#
#define kUpScaleShape___27		( PACK_4(1,0,1,1) + PACK_4(3,3,3,3) )	//#
#define kUpScaleShape___28		( PACK_4(1,1,0,0) + PACK_4(3,3,3,3) )	//#
#define kUpScaleShape___29		( PACK_4(1,1,0,1) + PACK_4(3,3,3,3) )	//=>25
#define kUpScaleShape___30		( PACK_4(1,1,1,0) + PACK_4(3,3,3,3) )	//#

#define kUpScaleShape___31		( PACK_4(0,0,0,0) + PACK_4(4,4,4,4) )	//#
#define kUpScaleShape___32		( PACK_4(0,0,0,1) + PACK_4(4,4,4,4) )	//=>28
#define kUpScaleShape___33		( PACK_4(0,0,1,0) + PACK_4(4,4,4,4) )	//#
#define kUpScaleShape___34		( PACK_4(0,0,1,1) + PACK_4(4,4,4,4) )	//#
#define kUpScaleShape___35		( PACK_4(0,1,0,0) + PACK_4(4,4,4,4) )	//=>30
#define kUpScaleShape___36		( PACK_4(0,1,0,1) + PACK_4(4,4,4,4) )	//=>32
#define kUpScaleShape___37		( PACK_4(0,1,1,0) + PACK_4(4,4,4,4) )	//=>31
#define kUpScaleShape___38		( PACK_4(0,1,1,1) + PACK_4(4,4,4,4) )	//=>33
#define kUpScaleShape___39		( PACK_4(1,0,0,0) + PACK_4(4,4,4,4) )	//#
#define kUpScaleShape___40		( PACK_4(1,0,0,1) + PACK_4(4,4,4,4) )	//=>31
#define kUpScaleShape___41		( PACK_4(1,0,1,0) + PACK_4(4,4,4,4) )	//#
#define kUpScaleShape___42		( PACK_4(1,0,1,1) + PACK_4(4,4,4,4) )	//#
#define kUpScaleShape___43		( PACK_4(1,1,0,0) + PACK_4(4,4,4,4) )	//#
#define kUpScaleShape___44		( PACK_4(1,1,0,1) + PACK_4(4,4,4,4) )	//=>38
#define kUpScaleShape___45		( PACK_4(1,1,1,0) + PACK_4(4,4,4,4) )	//#
															   
#define kUpScaleShape___46		( PACK_4(0,0,0,0) + PACK_4(5,5,5,5) )	//#


// 0,
//	 1+ 0,  2+ 0,  3+ 0,  4+ 0,  5+ 0,  6+ 0,  7+ 0,  8+ 0,  9+ 0, 10+ 0, 11+ 0, 12+ 0, 13+ 0, 14+ 0,
//15,
//	 1+15,  2+15,  3+15,  4+15,  5+15,  6+15,  7+15,  8+15,  9+15, 10+15, 11+15, 12+15, 13+15, 14+15,
//30,
//	 1+30,  2+30,  3+30,  4+30,  5+30,  6+30,  7+30,  8+30,  9+30, 10+30, 11+30, 12+30, 13+30, 14+30,
//45


#define PATTERN_AND_4B_ALIGNMENT_BYTES	4
#define PATTERN_AND_2B_ALIGNMENT_BYTES	2

#define BLEND_SPRITE_BYTES		BLEND_SPRITE_PARAM_BYTES + PATTERN_AND_4B_ALIGNMENT_BYTES
#define COPY_SPRITE_BYTES		COPY_SPRITE_PARAM_BYTES  + PATTERN_AND_2B_ALIGNMENT_BYTES

#define SET_BLEND_SPRITE_PARAM(pattern, dst, src, width, height, sprite )						\
{																								\
	ALIGN_4_BYTES( pattern )																	\
	*((long  *)(pattern +  0)) = (long )(dst);					/*dst addr, empty*/				\
	*((long  *)(pattern +  4)) = (long )(src);					/*src add*/						\
	*((short *)(pattern +  8)) = (short)(width);					/*horizontal offset*/		\
	*((short *)(pattern + 10)) = (short)(height);					/*vertical offset*/			\
	*((long  *)(pattern + 12)) = (long)(sprite);					/*horizontal offset*/		\
																								\
	pattern += BLEND_SPRITE_PARAM_BYTES;														\
}

#define SET_COPY_SPRITE_PARAM(pattern, width, height, sprites)											\
{																								\
	ALIGN_4_BYTES( pattern )																	\
	*((short *)(pattern + 0)) = (short)(width);	/*horizontal offset*/						\
	*((short *)(pattern + 2)) = (short)(height);	/*vertical offset*/							\
	*((long  *)(pattern + 4)) = (long)(sprites);					/*horizontal offset*/		\
	pattern += COPY_SPRITE_PARAM_BYTES;															\
}

#define GET_COPY_SPRITE_PARAM(pattern, width, height)											\
{																								\
	width		=*((short *)(pattern + 0));  /*horizontal offset*/								\
	height 		=*((short *)(pattern + 2));  /*vertical offset*/								\
	pattern    += COPY_SPRITE_PARAM_BYTES;														\
}

#ifndef min
	#define min(a,b)	((a < b) ? a : b)
#endif

#define my_pin_31  { if( p < 0 ) p = 0; else if(p > 31 ) p = 31; }
#define my_pin_63  { if( p < 0 ) p = 0; else if(p > 63 ) p = 63; }
#define my_pin_255 { if( p < 0 ) p = 0; else if(p > 255) p = 255;}

#define PACK_16RGB_lo			\
{								\
	p = ( y + (uvr<<1) ) >> 15;	\
	my_pin_31;					\
								\
	pix  = p << (11);			\
								\
	p = ( y - uvg) >> 14;		\
	my_pin_63;					\
								\
	pix |= p << (5);			\
								\
	p = (y + uvb) >> 15;		\
	my_pin_31;					\
								\
	pix |= p;					\
}


#define PACK_32BGRA				\
{								\
	p = ( y + (uvr<<1) ) >> 12;	\
	my_pin_255;					\
								\
	/*pix  = (255)<<24;*/		\
	pix  = p << (16);			\
								\
	p = ( y - uvg) >> 12;		\
	my_pin_255;					\
								\
	pix |= p << (8);			\
								\
	p = (y + uvb) >> 12;		\
	my_pin_255;					\
								\
	pix |= p;					\
	pix |= 0xff000000;			\
}

#define PACK_16RGB_hi			\
{								\
	p = ( y + (uvr<<1) ) >> 15;	\
	my_pin_31;					\
								\
	pix  |= p << (11+16);		\
								\
	p = ( y - uvg) >> 14;		\
	my_pin_63;					\
								\
	pix |= p << (5+16);			\
								\
	p = (y + uvb) >> 15;		\
	my_pin_31;					\
								\
	pix |= (p<<16);				\
}

#define GET_UV					\
	int y, u, v, uvr, uvg, uvb;	\
	int p, pix, t;				\
								\
	u = (*++u0);  				\
	v = (*++v0);  				\
								\
	u = u - 128;				\
	v = v - 128;				\
								\
	t   = u*(C1>>1);			\
	uvg = v*C1      + t;		\
	uvb = u*(C1<<1) + t;		\
	uvr = uvg - t;			

#define GET_UV_i				\
	int y, u, v, uvr, uvg, uvb;	\
	int p, pix, t;				\
								\
	u = (*yuv++);  				\
	v = (*yuv++);  				\
								\
	u = u - 128;				\
	v = v - 128;				\
								\
	t   = u*(C1>>1);			\
	uvg = v*C1      + t;		\
	uvb = u*(C1<<1) + t;		\
	uvr = uvg - t;			

#define GET_Y_i					\
	y = (*yuv++);				\
	y = y * C2 + Bx;			\
	PACK_16RGB_lo


#define GET_Y_i_32BGRA			\
	y = (*yuv++);				\
	y = y * C2 + Bx;			\
	PACK_32BGRA



#define GET_Y_i_RGB565_4x2			\
{									\
	GET_UV_i						\
	GET_Y_i;		pix0 = pix;		\
	GET_Y_i;		pix1 = pix;		\
	GET_Y_i;		pix2 = pix;		\
	GET_Y_i;		pix3 = pix;		\
}									\
{									\
	GET_UV_i						\
	GET_Y_i;		pix4 = pix;		\
	GET_Y_i;		pix5 = pix;		\
	GET_Y_i;		pix6 = pix;		\
	GET_Y_i;		pix7 = pix;		\
}

#define GET_Y_i_32BGRA_4x2			\
{									\
	GET_UV_i						\
	GET_Y_i_32BGRA;	pix0 = pix;		\
	GET_Y_i_32BGRA;	pix1 = pix;		\
	GET_Y_i_32BGRA;	pix2 = pix;		\
	GET_Y_i_32BGRA;	pix3 = pix;		\
}									\
{									\
	GET_UV_i						\
	GET_Y_i_32BGRA;	pix4 = pix;		\
	GET_Y_i_32BGRA;	pix5 = pix;		\
	GET_Y_i_32BGRA;	pix6 = pix;		\
	GET_Y_i_32BGRA;	pix7 = pix;		\
}
	

FskAPI(void)	FskYUV420Copy(
			UInt32			width,
			UInt32			height,
			const			UInt8 *srcY,
			const			UInt8 *srcU,
			const			UInt8 *srcV,
			SInt32			srcYRowBytes,
			SInt32			srcUVRowBytes,
			UInt8			*dstY,
			UInt8			*dstU,
			UInt8			*dstV,
			SInt32			dstYRowBytes,
			SInt32			dstUVRowBytes
		);

FskAPI(void)	FskYUV420Interleave_Generic
(
	unsigned char	*y0, 
	unsigned char	*u0, 
	unsigned char	*v0, 
	unsigned char	*yuv0,
	int				height, 
	int				width, 
	int				yrb, 
	int				uvrb,
	int				yuvrb,
	int				rotation
);

FskAPI(void) FskYUV420iRotate_Generic
(
	int height, 
	int width, 
	unsigned char *src, 
	unsigned char *dst, 
	int src_rb, 
	int dst_rb,
	int	rotation
);

FskAPI(void) ConfigCopyYUV420iProcs( int implementation );
FskAPI(void) ConfigCopyYUV420iProcs_c();

enum
{
	kRotationNone	= 0,
	kRotationCW90,
	kRotationCW180,
	kRotationCW270
};

#define kRotationCCW90 kRotationCW270

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKYUV420COPY__ */
