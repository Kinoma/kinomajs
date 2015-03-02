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
#include <math.h>
#include <memory.h>
#include <assert.h>


#include "kinoma_avc_defines.h"
#include "ippvc.h"
#include "kinoma_ipp_lib.h"
#include "kinoma_utilities.h"


IppStatus (__STDCALL *ippiInterpolateBlock_H264_8u_P2P1R_universal)						(const Ipp8u *pSrc1,  const Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u uWidth,Ipp32u uHeight,Ipp32s pitch)=NULL;
IppStatus (__STDCALL *ippiInterpolateBlock_H264_8u_P3P1R_universal)						(const Ipp8u *pSrc1,  const Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u uWidth,Ipp32u uHeight,Ipp32s iPitchSrc1,Ipp32s iPitchSrc2,Ipp32s iPitchDst)=NULL;
IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_16x16_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_16x8_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_8x16_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_8x8_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_8x4_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_4x8_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_4x4_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateLuma_H264_8u_C1R_universal)						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateChroma_H264_8u_C1R_universal)						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateLumaTop_H264_8u_C1R_universal)						(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateLumaBottom_H264_8u_C1R_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateChromaTop_H264_8u_C1R_universal)					(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiInterpolateChromaBottom_H264_8u_C1R_universal)				(const Ipp8u* pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,  Ipp32s   dy, Ipp32s   outPixels, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippiUniDirWeightBlock_H264_8u_C1R_universal)						(Ipp8u *pSrcDst,Ipp32u pitch, Ipp32u ulog2wd, Ipp32s iWeight,Ipp32s iOffset,IppiSize roi)=NULL;
IppStatus (__STDCALL *ippiBiDirWeightBlock_H264_8u_P2P1R_universal)						(Ipp8u *pSrc1,  Ipp8u *pSrc2, Ipp8u *pDst,     Ipp32u pitch,  Ipp32u dstStep, Ipp32u ulog2wd, Ipp32s iWeight1, Ipp32s iOffset1, Ipp32s iWeight2,Ipp32s iOffset2,   IppiSize roi)=NULL;
IppStatus (__STDCALL *ippiBiDirWeightBlock_H264_8u_P3P1R_universal)						(const Ipp8u *pSrc1,const Ipp8u *pSrc2, Ipp8u *pDst,Ipp32u nSrcPitch1, Ipp32u nSrcPitch2,Ipp32u nDstPitch,Ipp32u ulog2wd,Ipp32s iWeight1,Ipp32s iOffset1,Ipp32s iWeight2,Ipp32s iOffset2,IppiSize roi)=NULL;
IppStatus (__STDCALL *ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_universal)				(const Ipp8u *pSrc1,const Ipp8u *pSrc2,Ipp8u *pDst,Ipp32u nSrcPitch1, Ipp32u nSrcPitch2,  Ipp32u nDstPitch,  Ipp32s iWeight1,Ipp32s iWeight2,  IppiSize roi)=NULL;
IppStatus (__STDCALL *ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_universal)				(Ipp8u *pSrc1,  Ipp8u *pSrc2,   Ipp8u *pDst,    Ipp32u pitch,  Ipp32u dstpitch,   Ipp32s iWeight1,  Ipp32s iWeight2,  IppiSize roi)=NULL;

#ifdef __KINOMA_IPP__

#ifdef __cplusplus
extern "C" {
#endif

IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_16x16_arm
(
  const Ipp8u*   pSrc,
		Ipp32s   srcStep,
		Ipp8u*   dst,
		Ipp32s   dstStep,
		Ipp32s   dx,
		Ipp32s   dy,
		IppiSize dummp
);

IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_8x16_arm
(
  const Ipp8u*   pSrc,
		Ipp32s   srcStep,
		Ipp8u*   dst,
		Ipp32s   dstStep,
		Ipp32s   dx,
		Ipp32s   dy,
		IppiSize dummp
);


IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_arm
(
  const Ipp8u*   pSrc,
		Ipp32s   srcStep,
		Ipp8u*   dst,
		Ipp32s   dstStep,
		Ipp32s   dx,
		Ipp32s   dy,
		IppiSize dummp
);

void bilinear_qpel
(
	const   Ipp8u*  src,
	Ipp32s			srcStep,
	Ipp8u*			dst,
	Ipp32s			dstStep,
	Ipp32s			dx,
	Ipp32s			dy,
	IppiSize		roiSize
);

#ifdef __cplusplus
}
#endif

static  Ipp8u* CopyBlockFromTop
(
    Ipp8u *pRefPlane,
    Ipp8u *TemporalPixels,
    Ipp32s pitch,
    Ipp32s outPixels,
    Ipp32s xh,
    Ipp32s yh,
    IppiSize roi
)
{
	Profile_Start(ipp_util_CopyBlockFromTop_profile);

    Ipp8u *pOut;
    Ipp8u *pOutReturn;
    int i;

    pOutReturn = pOut = TemporalPixels;
    Ipp32s padded_y = yh>0?3:0;
    Ipp32s padded_x = xh>0?3:0;
    pRefPlane+=pitch*outPixels;

    Ipp32s num_outs = outPixels+padded_y;
    Ipp32s sbheight = roi.height+padded_y*2;

    assert(num_outs>=0);
    pRefPlane-=padded_x;
    pOut-=padded_y*32;
    pOut-=padded_x;

    if (num_outs>sbheight) //starting point outside bottom boundary
    {
        num_outs = roi.height+2*padded_y;
        for(i=0;i<num_outs;i++,pOut+=32)
            memcpy(pOut,pRefPlane,roi.width+2*padded_x);
    }
    else
    {
        for(i=0;i<num_outs;i++,pOut+=32)
            memcpy(pOut,pRefPlane,roi.width+2*padded_x);

        for(i=0;i<sbheight-num_outs;i++,pOut+=32,pRefPlane+=pitch)
            memcpy(pOut,pRefPlane,roi.width+2*padded_x);
    }

	Profile_End(ipp_util_CopyBlockFromTop_profile);

    return pOutReturn;
}

static  Ipp8u* CopyBlockFromBottom
(	
	Ipp8u *pRefPlane,
    Ipp8u *dst,
    Ipp32s pitch,
    Ipp32s outPixels,
    Ipp32s xh,
    Ipp32s yh,
    IppiSize roi
)
{
	Profile_Start(ipp_util_CopyBlockFromBottom_profile);

	Ipp8u *pOut= dst;
    int i;

    Ipp32s padded_y = yh>0?3:0;
    Ipp32s padded_x = xh>0?3:0;
    Ipp32s sbheight = roi.height+padded_y*2;

    Ipp32s num_outs = outPixels+padded_y;
    pOut-=padded_y*32;
    pOut-=padded_x;
    pRefPlane-=padded_x;
    assert(num_outs>=0);

#if 0
	{
		static int dx_eq0=0, dx_bt0=0, dy_eq0=0, dy_bt0=0, xy_eq0=0, xy_bt0=0;

		if( xh == 0 ) dx_eq0++;
		if( xh > 0 )  dx_bt0++;
		if( yh == 0 ) dy_eq0++;
		if( yh > 0 )  dy_bt0++;

		fprintf( stderr, "xh==0 count:%d, xh>0 count: %d, yh==0 count:%d, yh>0 count: %d\n", dx_eq0, dx_bt0, dy_eq0, dy_bt0 );

		if( xh == 0 && yh == 0 )  xy_eq0++;
		if( xh >0 && yh > 0 )  xy_bt0++;
		fprintf( stderr, "xh==0&&yh==0 count:%d, xh>0 count: %d\n", xy_eq0, xy_bt0);
		
				
		static int x2=0, x4=0, x8=0, x16=0;
		static int y2=0, y4=0, y8=0, y16=0;

		if( roi.width == 2 ) x2++;
		if( roi.width == 4 ) x4++;
		if( roi.width == 8 ) x8++;
		if( roi.width == 16) x16++;
		fprintf( stderr, "x2=%d, x4=%d, x8=%d, x16=%d\n", x2, x4, x8, x16 );
		if( roi.height == 2 ) y2++;
		if( roi.height == 4 ) y4++;
		if( roi.height == 8 ) y8++;
		if( roi.height == 16) y16++;
		fprintf( stderr, "y2=%d, y4=%d, y8=%d, y16=%d\n\n", y2, y4, y8, y16 );
	}
#endif

    if (num_outs>sbheight) //starting point outside bottom boundary
    {
        num_outs = sbheight;
        pRefPlane-=(outPixels-roi.height+1)*pitch;//get boundary pixel location
        for(i=0;i<num_outs;i++)
		{
            memcpy(pOut,pRefPlane,roi.width+2*padded_x);
			pOut+=32;           
		}
    }
    else
    {
        pRefPlane -=padded_y*pitch;
        for(i=0;i<sbheight-num_outs;i++)
		{
            memcpy(pOut,pRefPlane,roi.width+2*padded_x);
			pOut+=32;
			pRefPlane+=pitch;
		}
		
        pRefPlane-=pitch;
        for(i=0;i<num_outs;i++)
		{
            memcpy(pOut,pRefPlane,roi.width+2*padded_x);
            pOut+=32;
		}
    }

	Profile_End(ipp_util_CopyBlockFromBottom_profile);

//bail:
    return dst;
}

#if 1

#define TMP_XY	22
#define MB_MAX	16


//#define FILTER_ONE( t0, t1, t2, t3, t4, t5 ) ( (t0+t5) - 5*( (t1+t4 ) - ((t2+t3 )<<2) ) )
#define FILTER_ONE( r, t0, t1, t2, t3, t4, t5 )					\
{																\
	mm = (t1+t4 ) - ((t2+t3 )<<2);								\
	r = (t0+t5) - ( mm + (mm<<2));								\
}

#define FILTER_AND_CLIP( tmp0, t0, t1, t2, t3, t4, t5 ) 		\
		FILTER_ONE(tmp0, t0, t1, t2, t3, t4, t5 );				\
		tmp0 = (16 + tmp0 )>>5;									\
		Clip1(const_255, tmp0);									

#define FILTER_AND_CLIP_hi( tmp0, t0, t1, t2, t3, t4, t5 )  	\
		FILTER_ONE(tmp0, t0, t1, t2, t3, t4, t5 );				\
		tmp0 = (512 + tmp0 )>>10;								\
		Clip1(const_255, tmp0);									

#define FILTER_AND_CLIP_t2( tmp0, t0, t1, t2, t3, t4, t5 )		\
		FILTER_AND_CLIP( tmp0, t0, t1, t2, t3, t4, t5 )			\
		tmp0 = (tmp0 + t2 + 1)>>1;								
																
#define FILTER_AND_CLIP_t3( tmp0, t0, t1, t2, t3, t4, t5 )		\
		FILTER_AND_CLIP( tmp0, t0, t1, t2, t3, t4, t5 )			\
		tmp0 = (tmp0 + t3 + 1)>>1;								
																
#define FILTER_AND_CLIP_hi_t2( tmp0, t0, t1, t2, t3, t4, t5 )	\
		FILTER_AND_CLIP_hi( tmp0, t0, t1, t2, t3, t4, t5 )		\
		mm   = (t2 + 16)>>5;									\
		Clip1(const_255, mm);									\
		tmp0 = (tmp0 + mm + 1)>>1;								\

#define FILTER_AND_CLIP_hi_t3( tmp0, t0, t1, t2, t3, t4, t5 )	\
		FILTER_AND_CLIP_hi( tmp0, t0, t1, t2, t3, t4, t5 )		\
		mm   = (t3 + 16)>>5;									\
		Clip1(const_255, mm);									\
		tmp0 = (tmp0 + mm + 1)>>1;								\


#define FILTER_AND_CLIP_avg(tmp, t0, t1, t2, t3, t4, t5 );		\
		FILTER_AND_CLIP(tmp, t0, t1, t2, t3, t4, t5 );			\
		mm  = t[i];												\
		tmp = (tmp + mm + 1)>>1;								


#define HORIZONTAL_PATTERN_PACKED( PATTERN, m_width, m_height, src, srcStep, dst, dstStep )	\
	for(i=0; i<m_height; i++)														\
	{																				\
		int t0, t1, t2, t3, t4, t5, mm;												\
		int tmp0, tmp1, tmp2, tmp3;													\
																					\
		src += srcStep;																\
		t0 = src[0];																\
		t1 = src[1];																\
		t2 = src[2];																\
		t3 = src[3];																\
		t4 = src[4];																\
		t5 = src[5];																\
						 PATTERN(tmp0, t0, t1, t2, t3, t4, t5 );					\
		t0 = src[6];	 PATTERN(tmp1, t1, t2, t3, t4, t5, t0 );					\
		t1 = src[7];	 PATTERN(tmp2, t2, t3, t4, t5, t0, t1 );					\
		t2 = src[8];	 PATTERN(tmp3, t3, t4, t5, t0, t1, t2 );					\
																					\
		tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);						\
		dst += dstStep;																\
		*(long *)(dst+0) = tmp3;													\
		if( m_width == 4 ) continue;													\
																					\
		t3 = src[9];	PATTERN(tmp0,  t4, t5, t0, t1, t2, t3 );					\
		t4 = src[10];	PATTERN(tmp1,  t5, t0, t1, t2, t3, t4 );					\
		t5 = src[11];	PATTERN(tmp2,  t0, t1, t2, t3, t4, t5 );					\
		t0 = src[12];	PATTERN(tmp3,  t1, t2, t3, t4, t5, t0 );					\
																					\
		tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);						\
		*(long *)(dst+4) = tmp3;													\
		if( m_width == 8 ) continue;													\
																					\
		t1 = src[13];	PATTERN(tmp0,  t2, t3, t4, t5, t0, t1 );					\
		t2 = src[14];	PATTERN(tmp1,  t3, t4, t5, t0, t1, t2 );					\
		t3 = src[15];	PATTERN(tmp2,  t4, t5, t0, t1, t2, t3 );					\
		t4 = src[16];	PATTERN(tmp3,  t5, t0, t1, t2, t3, t4 );					\
																					\
		tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);						\
		*(long *)(dst+8) = tmp3;													\
																					\
		t5 = src[17];	PATTERN(tmp0,  t0, t1, t2, t3, t4, t5 );					\
		t0 = src[18];	PATTERN(tmp1,  t1, t2, t3, t4, t5, t0 );					\
		t1 = src[19]; 	PATTERN(tmp2,  t2, t3, t4, t5, t0, t1 );					\
		t2 = src[20]; 	PATTERN(tmp3,  t3, t4, t5, t0, t1, t2 );					\
																					\
		tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);						\
		*(long *)(dst+12) = tmp3;													\
	}						
																				
#define HORIZONTAL_PATTERN( PATTERN, m_width, m_height, src, srcStep, dst, dstStep )						\
	for(i=0; i<m_height; i++)														\
	{																							\
		int t0, t1, t2, t3, t4, t5, mm;															\
		int tmp0;																				\
																								\
		src += srcStep;																			\
		t0 = src[0];																			\
		t1 = src[1];																			\
		t2 = src[2];																			\
		t3 = src[3];																			\
		t4 = src[4];																			\
		t5 = src[5];																			\
						 PATTERN(tmp0, t0, t1, t2, t3, t4, t5 ); dst += dstStep; dst[0] = tmp0;	\
		t0 = src[6];	 PATTERN(tmp0, t1, t2, t3, t4, t5, t0 ); dst[1] = tmp0;					\
		t1 = src[7];	 PATTERN(tmp0, t2, t3, t4, t5, t0, t1 ); dst[2] = tmp0;					\
		t2 = src[8];	 PATTERN(tmp0, t3, t4, t5, t0, t1, t2 ); dst[3] = tmp0;					\
																								\
		if( m_width == 4 ) continue;																\
																								\
		t3 = src[9];	PATTERN(tmp0,  t4, t5, t0, t1, t2, t3 ); dst[4] = tmp0;					\
		t4 = src[10];	PATTERN(tmp0,  t5, t0, t1, t2, t3, t4 ); dst[5] = tmp0;					\
		t5 = src[11];	PATTERN(tmp0,  t0, t1, t2, t3, t4, t5 ); dst[6] = tmp0;					\
		t0 = src[12];	PATTERN(tmp0,  t1, t2, t3, t4, t5, t0 ); dst[7] = tmp0;					\
																								\
		if( m_width == 8 ) continue;																\
																								\
		t1 = src[13];	PATTERN(tmp0,  t2, t3, t4, t5, t0, t1 ); dst[8] = tmp0;					\
		t2 = src[14];	PATTERN(tmp0,  t3, t4, t5, t0, t1, t2 ); dst[9] = tmp0;					\
		t3 = src[15];	PATTERN(tmp0,  t4, t5, t0, t1, t2, t3 ); dst[10] = tmp0;				\
		t4 = src[16];	PATTERN(tmp0,  t5, t0, t1, t2, t3, t4 ); dst[11] = tmp0;				\
																								\
		t5 = src[17];	PATTERN(tmp0,  t0, t1, t2, t3, t4, t5 ); dst[12] = tmp0;				\
		t0 = src[18];	PATTERN(tmp0,  t1, t2, t3, t4, t5, t0 ); dst[13] = tmp0;				\
		t1 = src[19]; 	PATTERN(tmp0,  t2, t3, t4, t5, t0, t1 ); dst[14] = tmp0;				\
		t2 = src[20]; 	PATTERN(tmp0,  t3, t4, t5, t0, t1, t2 ); dst[15] = tmp0;				\
	}																						
	
#define PLD_NEXT(step)						\
{											\
	void *next_add = (void *)(s0 + step) ;	\
	PLD( next_add );						\
}

#define VERTICAL_PATTERN( PATTERN, m_width, m_height, s0, sStep, d0, dStep )									\
	for(i=0; i<m_width; i++)													\
	{																							\
		int t0, t1, t2, t3, t4, t5, tmp, mm;															\
																										\
		s0 -= src_chunk;																				\
		d0 -= dst_chunk;																				\
																										\
						t0 = *s0;																		\
		s0 += sStep;	t1 = *s0;																		\
		s0 += sStep;	t2 = *s0;																		\
		s0 += sStep;	t3 = *s0;																		\
		s0 += sStep;	t4 = *s0;																		\
		s0 += sStep;	t5 = *s0; PATTERN(tmp, t0, t1, t2, t3, t4, t5 );	             *d0 = tmp;	\
		s0 += sStep;	t0 = *s0; PATTERN(tmp, t1, t2, t3, t4, t5, t0 );	d0 += dStep; *d0 = tmp;	\
		s0 += sStep;	t1 = *s0; PATTERN(tmp, t2, t3, t4, t5, t0, t1 );	d0 += dStep; *d0 = tmp;	\
		s0 += sStep;	t2 = *s0; PATTERN(tmp, t3, t4, t5, t0, t1, t2 );	d0 += dStep; *d0 = tmp;	\
																				\
		if(m_height==4) continue;													\
																				\
		s0 += sStep;	t3 = *s0; PATTERN(tmp, t4, t5, t0, t1, t2, t3 );	d0 += dStep; *d0 = tmp;	\
		s0 += sStep;	t4 = *s0; PATTERN(tmp, t5, t0, t1, t2, t3, t4 );	d0 += dStep; *d0 = tmp;	\
		s0 += sStep;	t5 = *s0; PATTERN(tmp, t0, t1, t2, t3, t4, t5 );	d0 += dStep; *d0 = tmp;	\
		s0 += sStep;	t0 = *s0; PATTERN(tmp, t1, t2, t3, t4, t5, t0 );	d0 += dStep; *d0 = tmp;	\
																				\
		if(m_height==8) continue;													\
																				\
		s0 += sStep;	t1 = *s0; PATTERN(tmp, t2, t3, t4, t5, t0, t1 );	d0 += dStep; *d0 = tmp;	\
		s0 += sStep;	t2 = *s0; PATTERN(tmp, t3, t4, t5, t0, t1, t2 );	d0 += dStep; *d0 = tmp;	\
		s0 += sStep;	t3 = *s0; PATTERN(tmp, t4, t5, t0, t1, t2, t3 );	d0 += dStep; *d0 = tmp;	\
		s0 += sStep;	t4 = *s0; PATTERN(tmp, t5, t0, t1, t2, t3, t4 );	d0 += dStep; *d0 = tmp;	\
																								\
		s0 += sStep;	t5 = *s0; PATTERN(tmp, t0, t1, t2, t3, t4, t5 );	d0 += dStep; *d0 = tmp;	\
		s0 += sStep;	t0 = *s0; PATTERN(tmp, t1, t2, t3, t4, t5, t0 );	d0 += dStep; *d0 = tmp;	\
		s0 += sStep;	t1 = *s0; PATTERN(tmp, t2, t3, t4, t5, t0, t1 );	d0 += dStep; *d0 = tmp;	\
		s0 += sStep;	t2 = *s0; PATTERN(tmp, t3, t4, t5, t0, t1, t2 );	d0 += dStep; *d0 = tmp;	\
	}																							


#define VERTICAL_PATTERN_4( PATTERN, m_width, s0, sStep, d0, dStep )									\
	for(i=0; i<m_width; i++)																			\
	{																									\
		int t0, t1, t2, t3, t4, t5, tmp, mm;															\
																										\
		s0 -= src_chunk;																				\
		d0 -= dst_chunk;																				\
																										\
						t0 = *s0;																		\
		s0 += sStep;	t1 = *s0;																		\
		s0 += sStep;	t2 = *s0;																		\
		s0 += sStep;	t3 = *s0;																		\
		s0 += sStep;	t4 = *s0;																		\
		s0 += sStep;	t5 = *s0; PATTERN(tmp, t0, t1, t2, t3, t4, t5 );	             *d0 = tmp;	\
		s0 += sStep;	t0 = *s0; PATTERN(tmp, t1, t2, t3, t4, t5, t0 );	d0 += dStep; *d0 = tmp;	\
		s0 += sStep;	t1 = *s0; PATTERN(tmp, t2, t3, t4, t5, t0, t1 );	d0 += dStep; *d0 = tmp;	\
		s0 += sStep;	t2 = *s0; PATTERN(tmp, t3, t4, t5, t0, t1, t2 );	d0 += dStep; *d0 = tmp;	\
	}

#if 1

#define VERTICAL_PATTERN_long_step( PATTERN, m_width, m_height, s0, sStep, d0, dStep )	\
{																						\
	int   i;																			\
	int	  src_chunk = 4*sStep + (sStep<<2) - 1;											\
	int	  dst_chunk = 4*dStep - dStep - 1;												\
																						\
	d0 += dst_chunk;																	\
	s0 += src_chunk - (sStep<<1);														\
																						\
	VERTICAL_PATTERN_4( PATTERN, m_width, s0, sStep, d0, dStep )						\
	if( m_height == 4 ) goto end;															\
																						\
	d0 += dst_chunk + dStep - m_width + 1;												\
	s0 += src_chunk - (sStep<<2) - m_width + 1;											\
	VERTICAL_PATTERN_4( PATTERN, m_width, s0, sStep, d0, dStep )						\
																						\
	if( m_height == 8 ) goto end;															\
																						\
	d0 += dst_chunk + dStep - m_width + 1;												\
	s0 += src_chunk - (sStep<<2) - m_width + 1;											\
	VERTICAL_PATTERN_4( PATTERN, m_width, s0, sStep, d0, dStep )						\
																						\
	d0 += dst_chunk + dStep - m_width + 1;												\
	s0 += src_chunk - (sStep<<2) - m_width + 1;											\
	VERTICAL_PATTERN_4( PATTERN, m_width, s0, sStep, d0, dStep )						\
																						\
end:																					\
	;																						\
}


#elif 1
#define VERTICAL_PATTERN_long_step( PATTERN, m_width, m_height, s0, sStep, d0, dStep )	\
{																					\
	int	half_height = m_height == 4 ? m_height : m_height>>1;													\
	int   i;																		\
	int	  src_chunk = half_height*sStep + (sStep<<2) - 1;							\
	int	  dst_chunk = half_height*dStep - dStep - 1;								\
																					\
	d0 += dst_chunk;																\
	s0 += src_chunk - (sStep<<1);													\
																					\
	VERTICAL_PATTERN( PATTERN, m_width, half_height, s0, sStep, d0, dStep )			\
																					\
	if( m_height == 4 )																\
		return;																		\
																					\
	d0 += dst_chunk + dStep - m_width + 1;											\
	s0 += src_chunk - (sStep<<2) - m_width + 1;										\
	VERTICAL_PATTERN( PATTERN, m_width, half_height, s0, sStep, d0, dStep )			\
}

#else
//#define VERTICAL_PATTERN_long_step VERTICAL_PATTERN_short_step
#define VERTICAL_PATTERN_long_step( PATTERN, width, height, s0, sStep, d0, dStep )	\
{																				\
	int i, j;																	\
	/*int src_chunk = 6*srcStep;*/ 												\
	/*int dst_chunk = 6*dstStep;*/												\
	s0 -= (sStep<<1);															\
																				\
	for(j=0; j<height; j++)														\
	{																			\
		/*PLD( (void *)( s0 + src_chunk ));	*/									\
		/*PLD( (void *)( d0 + dst_chunk ));	*/									\
																				\
		for(i=0; i<width; i++)													\
		{																		\
			int t0, t1, t2, t3, t4, t5, tmp, mm;								\
			top = s0 + i;														\
																				\
							t0 = *top;											\
			top += sStep;	t1 = *top;											\
			top += sStep;	t2 = *top;											\
			top += sStep;	t3 = *top;											\
			top += sStep;	t4 = *top;											\
			top += sStep;	t5 = *top;											\
																				\
			PATTERN(tmp, t0, t1, t2, t3, t4, t5 );								\
			dst[i] = tmp;														\
		}																		\
		s0 += sStep;															\
		d0 += dStep;															\
	}																			\
}
#endif

#if 1

#define VERTICAL_PATTERN_short_step( PATTERN, m_width, m_height, s0, sStep, d0, dStep )	\
{																				\
	int   i;																	\
	int	  src_chunk = m_height*sStep + (sStep<<2) - 1;							\
	int	  dst_chunk = m_height*dStep - dStep - 1;								\
																				\
	d0 += dst_chunk;															\
	s0 += src_chunk - (sStep<<1);												\
																				\
	VERTICAL_PATTERN( PATTERN, m_width, m_height, s0, sStep, d0, dStep )		\
}

#else

#define VERTICAL_PATTERN_short_step VERTICAL_PATTERN_long_step

#endif

#ifndef DROP_C_NO

//#define VERTICAL_PATTERN_short_step VERTICAL_PATTERN_long_step

#if defined(__KINOMA_IPP_ARM_V5__)
	#if TARGET_OS_LINUX || TARGET_OS_IPHONE
		#define KINOMA_INLINE static __inline
	#else
		#include <armintr.h>
		#define KINOMA_INLINE static __forceinline
	#endif
#else
#define KINOMA_INLINE static __inline
#endif

KINOMA_INLINE 
void case_0_4x_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )	
{																				
	int j;																	
	const Ipp8u *src = pSrc - srcStep;											
																				
	dst = dst - dstStep;														
	if( (((long)src)&0x03) == 0 )												
		for(j=0; j<height; j++)													
		{																		
			src += srcStep;														
			dst += dstStep;														
			*(long *)(dst+0)  = *(long *)(src+0);
		}																		
	else																		
		for(j=0; j<height; j++)													
		{																		
			int tmp0, tmp1, tmp2, tmp3;											
																				
			src += srcStep;														
			tmp0 = src[0];	tmp1 = src[1];	tmp2 = src[2];	tmp3 = src[3];		
			tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);				
			dst += dstStep;														
			*(long *)(dst+0) = tmp3;	
		}																		
}

KINOMA_INLINE 
void case_0_8x_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )	
{																				
	int j;																	
	const Ipp8u *src = pSrc - srcStep;											
																				
	dst = dst - dstStep;														
	if( (((long)src)&0x03) == 0 )												
		for(j=0; j<height; j++)													
		{																		
			src += srcStep;														
			dst += dstStep;														
			*(long *)(dst+0)  = *(long *)(src+0);
			*(long *)(dst+4)  = *(long *)(src+4);	
		}																		
	else																		
		for(j=0; j<height; j++)													
		{																		
			int tmp0, tmp1, tmp2, tmp3;											
																				
			src += srcStep;														
			tmp0 = src[0];	tmp1 = src[1];	tmp2 = src[2];	tmp3 = src[3];		
			tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);				
			dst += dstStep;														
			*(long *)(dst+0) = tmp3;	

			tmp0 = src[4];	tmp1 = src[5];	tmp2 = src[6];	tmp3 = src[7];		
			tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);				
			*(long *)(dst+4) = tmp3;
		}																		
}

KINOMA_INLINE 
void case_0_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )	
{																				
	int j;																	
	const Ipp8u *src = pSrc - srcStep;											
																				
	dst = dst - dstStep;														
	if( (((long)src)&0x03) == 0 )												
		for(j=0; j<height; j++)													
		{																		
			src += srcStep;														
			dst += dstStep;														
			*(long *)(dst+0)  = *(long *)(src+0);
			if( width == 4 ) continue;
			*(long *)(dst+4)  = *(long *)(src+4);	
			if( width == 8 ) continue;
			*(long *)(dst+8)  = *(long *)(src+8);								
			*(long *)(dst+12) = *(long *)(src+12);								
		}																		
	else																		
		for(j=0; j<height; j++)													
		{																		
			int tmp0, tmp1, tmp2, tmp3;											
																				
			src += srcStep;														
			tmp0 = src[0];	tmp1 = src[1];	tmp2 = src[2];	tmp3 = src[3];		
			tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);				
			dst += dstStep;														
			*(long *)(dst+0) = tmp3;	
			if( width == 4 ) continue;

			tmp0 = src[4];	tmp1 = src[5];	tmp2 = src[6];	tmp3 = src[7];		
			tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);				
			*(long *)(dst+4) = tmp3;
			if( width == 8 ) continue;

			tmp0 = src[8];	tmp1 = src[9];	tmp2 = src[10];tmp3 = src[11];		
			tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);				
			*(long *)(dst+8) = tmp3;											
																				
			tmp0 = src[12];tmp1 = src[13];tmp2 = src[14];tmp3 = src[15];		
			tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);				
			*(long *)(dst+12) = tmp3;											
		}																		
}

KINOMA_INLINE 
void case_1_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )
{																				
	int i;																	
	const Ipp8u *src = pSrc - 2 - srcStep;										
	int	  const_255 = 255;														
																				
	dst = dst - dstStep;																																
	HORIZONTAL_PATTERN_PACKED( FILTER_AND_CLIP_t2, width, height, src, srcStep, dst, dstStep )

}
KINOMA_INLINE 
void case_3_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )
{																			
	int i;																
	const Ipp8u *src = pSrc - 2 - srcStep;									
	int	  const_255 = 255;													
																			
	dst = dst - dstStep;													
	HORIZONTAL_PATTERN_PACKED( FILTER_AND_CLIP_t3, width, height, src, srcStep, dst, dstStep )
}


KINOMA_INLINE 
void case_2_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )
{																												
	int i;																										
	const Ipp8u *src = pSrc - 2 - srcStep;																		
	int	  const_255 = 255;																						
																												
	dst = dst - dstStep;																						
	HORIZONTAL_PATTERN_PACKED( FILTER_AND_CLIP, width, height, src, srcStep, dst, dstStep )
}	

KINOMA_INLINE																							
void case_4_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )						
{																	
	int	  const_255 = 255;	
	Ipp8u *s0 = (Ipp8u *)pSrc;

	VERTICAL_PATTERN_long_step( FILTER_AND_CLIP_t2, width, height, s0, srcStep, dst, dstStep )
}

KINOMA_INLINE																							
void case_12_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )						
{																	
	int	  const_255 = 255;											
	Ipp8u *s0 = (Ipp8u *)pSrc;
	VERTICAL_PATTERN_long_step( FILTER_AND_CLIP_t3, width, height, s0, srcStep, dst, dstStep )
}

KINOMA_INLINE																							
void case_8_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )									
{																	
	int	  const_255 = 255;	
	Ipp8u *s0 = (Ipp8u *)pSrc;
	VERTICAL_PATTERN_long_step( FILTER_AND_CLIP, width, height, s0, srcStep, dst, dstStep )
}

KINOMA_INLINE																							
void case_6_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )						
{																										
	int i;																								
	short	tt[MB_MAX*TMP_XY];	//width*(height+6)
																										
	/*Prepair data in H filter: Compute 1/2 position for H	*/											
	const Ipp8u *src = pSrc - (srcStep << 1) - srcStep - 2;												
	short *t		 = tt - width;																		
	int height_local = height+6;
	HORIZONTAL_PATTERN( FILTER_ONE, width, height_local, src, srcStep, t, width )
																										
	/* do V filter: compute 1/2 position for V -- But just in position (2,2) ONLY*/						
	/*no vertical 1/2 position was computed based on integer vertical position*/						
	int	  const_255 = 255;											

	short *s0 = tt + (width<<1);
	VERTICAL_PATTERN_short_step( FILTER_AND_CLIP_hi_t2, width, height, s0, width, dst, dstStep )
}

KINOMA_INLINE																							
void case_14_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )						
{																											
	int i;																									
	short	tt[MB_MAX*TMP_XY];	//(height+6)*(width+6)
																											
	/*Prepair data in H filter: Compute 1/2 position for H	*/												
	const Ipp8u *src = pSrc - (srcStep << 1) - srcStep - 2;												
	short		*t	 = tt - width;																			
	int height_local = height+6;
	HORIZONTAL_PATTERN( FILTER_ONE, width, height_local, src, srcStep, t, width )

	/* do V filter: compute 1/2 position for V -- But just in position (2,2) ONLY*/							
	/*no vertical 1/2 position was computed based on integer vertical position*/							
	int	  const_255 = 255;	

	short *s0 = tt + (width<<1);
	VERTICAL_PATTERN_short_step( FILTER_AND_CLIP_hi_t3, width, height, s0, width, dst, dstStep )
}																												


KINOMA_INLINE																							
void case_10_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )									
{	
	int i;																									
	short	tt[TMP_XY*MB_MAX];	//(width+6)*height
	short	*t		= tt;	
	
	width += 6;

	Ipp8u *s0 = (Ipp8u* )(pSrc - 2);	
	VERTICAL_PATTERN_short_step( FILTER_ONE, width, height, s0, srcStep, t, width )

	int const_255 = 255;																																																	
	t   = tt - width;																							
	dst -= dstStep;																								
	HORIZONTAL_PATTERN( FILTER_AND_CLIP_hi, width, height, t, width, dst, dstStep )
}


KINOMA_INLINE 
void case_9_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )
{																												
	short	tt[TMP_XY*MB_MAX];	//(width+6)*height
	short	*t = tt;																					
	
	int i;																										

	width += 6;

	Ipp8u *s0 = (Ipp8u* )(pSrc - 2);	
	VERTICAL_PATTERN_short_step( FILTER_ONE, width, height, s0, srcStep, t, width )
																												
	t     = tt - width;																							
	dst  -= dstStep;																																																
	int const_255 = 255;	
	HORIZONTAL_PATTERN( FILTER_AND_CLIP_hi_t2, width, height, t, width, dst, dstStep )
}


KINOMA_INLINE 
void case_11_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int width, int height )
{																												
	short	tt[TMP_XY*MB_MAX];	//(width+6)*height
	short	*t	= tt;																					
	int i;																										

	width += 6;

	Ipp8u *s0 = (Ipp8u* )(pSrc - 2);	
	VERTICAL_PATTERN_short_step( FILTER_ONE, width, height, s0, srcStep, t, width )
																												
	t     = tt - width;																							
	dst  -= dstStep;																																																			
	int const_255 = 255;	
	HORIZONTAL_PATTERN( FILTER_AND_CLIP_hi_t3, width, height, t, width, dst, dstStep )																																																
}

KINOMA_INLINE 
void case_5_7_13_15_c( const Ipp8u* pSrc, Ipp32s srcStep, Ipp8u* dst, Ipp32s dstStep, int dx, int dy, int width, int height )
{																								
	int i, j;																					
	unsigned char	tt[MB_MAX*MB_MAX*2];	//height*width
	unsigned char	*t = tt - width;																		
	const	Ipp8u *src = pSrc + (dy>> 1)*srcStep - 2 - srcStep;																					
	int		const_255 = 255;																		

	HORIZONTAL_PATTERN( FILTER_AND_CLIP, width, height, src, srcStep, t, width )

	t   = tt+MB_MAX*MB_MAX;	
	src = pSrc + (dx>> 1);																								

	VERTICAL_PATTERN_short_step( FILTER_AND_CLIP, width, height, src, srcStep, t, width )

#define AVERAGE_FOUR( r, t0, t1 )			\
		mm = mask&t0;						\
		r  = mask&t1;						\
		mm = mm + r + round;			    \
		r  = mask&(mm>>1) ;					\
											\
		mm = mask&(t0>>8);					\
		t1 = mask&(t1>>8);					\
		mm = mm + t1 + round;				\
		mm = mask&(mm>>1);					\
											\
		r = r | (mm<<8);					\

	t   = tt  - width;																						
	dst = dst - dstStep;																						
	int mask  = 0x00ff00ff;
	int	round = 0x00010001;
	
	for(j=0; j<height; j++)													
	{																		
		long t0, t1, mm, tmp;
		
		t   += width;														
		dst += dstStep;														
		
		t0 = *((long *)(&t[0]));  t1 = *((long *)(&t[0+MB_MAX*MB_MAX]));   AVERAGE_FOUR( tmp, t0, t1 );  *(long *)(dst+0)  = tmp;
		
		if( width == 4 ) continue;

		t0 = *((long *)(&t[4]));  t1 = *((long *)(&t[4+MB_MAX*MB_MAX]));   AVERAGE_FOUR( tmp, t0, t1 );  *(long *)(dst+4)  = tmp;	
		
		if( width == 8 ) continue;
		
		t0 = *((long *)(&t[8]));  t1 = *((long *)(&t[8+MB_MAX*MB_MAX]));   AVERAGE_FOUR( tmp, t0, t1 );  *(long *)(dst+8)  = tmp;
		t0 = *((long *)(&t[12])); t1 = *((long *)(&t[12+MB_MAX*MB_MAX]));  AVERAGE_FOUR( tmp, t0, t1 );  *(long *)(dst+12) = tmp;
	}																		
}


#if 1
IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_c
(
  const Ipp8u*   pSrc,
		Ipp32s   srcStep,
		Ipp8u*   dst,
		Ipp32s   dstStep,
		Ipp32s   dx,
		Ipp32s   dy,
		IppiSize roi   // Must be 16,8 or 4
)
{
	Profile_Start(ippiInterpolateLuma_H264_8u_C1R_c_profile);

	int	casedxy = (dy<<2) + dx;
	int height = roi.height;
	int width  = roi.width;

	switch(casedxy)
	{
		case 0:	 case_0_c(pSrc, srcStep, dst, dstStep, width, height );break;
		case 1:	 case_1_c(pSrc, srcStep, dst, dstStep, width, height );break;
		case 2:	 case_2_c(pSrc, srcStep, dst, dstStep, width, height );break;
		case 3:	 case_3_c(pSrc, srcStep, dst, dstStep, width, height );break;
		case 4:	 case_4_c(pSrc, srcStep, dst, dstStep, width, height );break;
		case 6:	 case_6_c(pSrc, srcStep, dst, dstStep, width, height );break;
		case 8:	 case_8_c(pSrc, srcStep, dst, dstStep, width, height );break;
		case 9:	 case_9_c(pSrc, srcStep, dst, dstStep, width, height );break;
		case 10: case_10_c(pSrc, srcStep, dst, dstStep, width, height );break;
		case 11: case_11_c(pSrc, srcStep, dst, dstStep, width, height );break;
		case 12: case_12_c(pSrc, srcStep, dst, dstStep, width, height );break;
		case 14: case_14_c(pSrc, srcStep, dst, dstStep, width, height );break;
		case 5:	 
		case 7:	 
		case 13: 
		case 15: case_5_7_13_15_c(pSrc, srcStep, dst, dstStep, dx, dy, width, height );break;
	}

	Profile_End(ippiInterpolateLuma_H264_8u_C1R_c_profile);

	return ippStsNoErr;
}

#else	

#define TMP_X	24
#define TMP_Y	24
//for reference only
IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_c
(
  const Ipp8u*   pSrc,
		Ipp32s   srcStep,
		Ipp8u*   dst,
		Ipp32s   dstStep,
		Ipp32s   dx,
		Ipp32s   dy,
		IppiSize roiSize   // Must be 16,8 or 4
)			
{
	Profile_Start(ippiInterpolateLuma_H264_8u_C1R_c_profile);
	
	int	width  = roiSize.width;
	int width4 = width>>2;
	int	height = roiSize.height;
	int	casedxy = (dy<<2) + dx;
	int	i, j;

	switch(casedxy)
	{
		case 0:		// dx =0 && dy =0	
			{
				const Ipp8u *src = pSrc - srcStep;
				
				dst = dst - dstStep;
				if( (((int)src)&0x03) == 0 )
					for(j=0; j<height; j++)
					{
						src += srcStep;
						dst += dstStep;
						*(long *)(dst+0)  = *(long *)(src+0);

						if( width4 == 1) continue;
						
						*(long *)(dst+4)  = *(long *)(src+4);
						
						if( width4 == 2) continue;
						
						*(long *)(dst+8)  = *(long *)(src+8);
						*(long *)(dst+12) = *(long *)(src+12);
					}
				else
					for(j=0; j<height; j++)
					{
						int tmp0, tmp1, tmp2, tmp3;
						
						src += srcStep;
						tmp0 = src[0];	tmp1 = src[1];	tmp2 = src[2];	tmp3 = src[3];
						tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);
						dst += dstStep;
						*(long *)(dst+0) = tmp3;
						
						if( width4 == 1 ) continue;

						tmp0 = src[4];	tmp1 = src[5];	tmp2 = src[6];	tmp3 = src[7];
						tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);
						*(long *)(dst+4) = tmp3;
						
						if( width4 == 2 ) continue;

						tmp0 = src[8];	tmp1 = src[9];	tmp2 = src[10];tmp3 = src[11];
						tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);
						*(long *)(dst+8) = tmp3;
						
						tmp0 = src[12];tmp1 = src[13];tmp2 = src[14];tmp3 = src[15];
						tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);
						*(long *)(dst+12) = tmp3;
					}
			}
			break;
		case 1:		// dx = 1 && dy = 0
		case 3:		// dx = 3 && dy = 0
			{	
				const Ipp8u *src = pSrc - 4 - srcStep;
				int	  ddx = dx>> 1;
				int	  const_255 = 255;

				dst = dst - dstStep;
				
				for(j=0; j<height; j++)
				{
					int t0, t1, t2, t3, t4, t5;
					int tmp0, tmp1, tmp2, tmp3, mm;

					src += srcStep;
					t0 = src[2];
					t1 = src[3];
					t2 = src[4];
					t3 = src[5];
					t4 = src[6];
					t5 = src[7];
									 FILTER_AND_CLIP(tmp0, t0, t1, t2, t3, t4, t5 ); tmp0 = (tmp0 + src[0+4+ddx] + 1)>>1;
					t0 = src[8];	 FILTER_AND_CLIP(tmp1, t1, t2, t3, t4, t5, t0 ); tmp1 = (tmp1 + src[1+4+ddx] + 1)>>1;
					t1 = src[9];	 FILTER_AND_CLIP(tmp2, t2, t3, t4, t5, t0, t1 ); tmp2 = (tmp2 + src[2+4+ddx] + 1)>>1;
					t2 = src[10];	 FILTER_AND_CLIP(tmp3, t3, t4, t5, t0, t1, t2 ); tmp3 = (tmp3 + src[3+4+ddx] + 1)>>1;
					
					tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);
					dst += dstStep;
					*(long *)(dst+0) = tmp3;
					
					if( width4 == 1 ) continue;
						
					t3 = src[11];	FILTER_AND_CLIP(tmp0,  t4, t5, t0, t1, t2, t3 ); tmp0 = (tmp0 + src[4+4+ddx] + 1)>>1;
					t4 = src[12];	FILTER_AND_CLIP(tmp1,  t5, t0, t1, t2, t3, t4 ); tmp1 = (tmp1 + src[5+4+ddx] + 1)>>1;
					t5 = src[13];	FILTER_AND_CLIP(tmp2,  t0, t1, t2, t3, t4, t5 ); tmp2 = (tmp2 + src[6+4+ddx] + 1)>>1;
					t0 = src[14];	FILTER_AND_CLIP(tmp3,  t1, t2, t3, t4, t5, t0 ); tmp3 = (tmp3 + src[7+4+ddx] + 1)>>1;
					
					tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);
					*(long *)(dst+4) = tmp3;

					if( width4 == 2 ) continue;

					t1 = src[15];	FILTER_AND_CLIP(tmp0,  t2, t3, t4, t5, t0, t1 ); tmp0 = (tmp0 + src[8+4+ddx] + 1)>>1;
					t2 = src[16];	FILTER_AND_CLIP(tmp1,  t3, t4, t5, t0, t1, t2 ); tmp1 = (tmp1 + src[9+4+ddx] + 1)>>1;
					t3 = src[17];	FILTER_AND_CLIP(tmp2,  t4, t5, t0, t1, t2, t3 ); tmp2 = (tmp2 + src[10+4+ddx] + 1)>>1;
					t4 = src[18];	FILTER_AND_CLIP(tmp3,  t5, t0, t1, t2, t3, t4 ); tmp3 = (tmp3 + src[11+4+ddx] + 1)>>1;
					
					tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);
					*(long *)(dst+8) = tmp3;
					
					t5 = src[19];	FILTER_AND_CLIP(tmp0,  t0, t1, t2, t3, t4, t5 ); tmp0 = (tmp0 + src[12+4+ddx] + 1)>>1;
					t0 = src[20];	FILTER_AND_CLIP(tmp1,  t1, t2, t3, t4, t5, t0 ); tmp1 = (tmp1 + src[13+4+ddx] + 1)>>1;
					t1 = src[21]; 	FILTER_AND_CLIP(tmp2,  t2, t3, t4, t5, t0, t1 ); tmp2 = (tmp2 + src[14+4+ddx] + 1)>>1;
					t2 = src[22]; 	FILTER_AND_CLIP(tmp3,  t3, t4, t5, t0, t1, t2 ); tmp3 = (tmp3 + src[15+4+ddx] + 1)>>1;
					
					tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);
					*(long *)(dst+12) = tmp3;
				}
			}
			break;

		case 2:		// dx = 2 && dy = 0
			{
				const Ipp8u *src = pSrc - 4 - srcStep;
				int	  const_255 = 255;

				dst = dst - dstStep;

				for(j=0; j<height; j++)
				{
					int t0, t1, t2, t3, t4, t5;
					int tmp0, tmp1, tmp2, tmp3, mm;

					src += srcStep;
					t0 = src[2];
					t1 = src[3];
					t2 = src[4];
					t3 = src[5];
					t4 = src[6];
					t5 = src[7];

									FILTER_AND_CLIP(tmp0,  t0, t1, t2, t3, t4, t5 );
					t0 = src[8]; 	FILTER_AND_CLIP(tmp1,  t1, t2, t3, t4, t5, t0 );
					t1 = src[9]; 	FILTER_AND_CLIP(tmp2,  t2, t3, t4, t5, t0, t1 );
					t2 = src[10];	FILTER_AND_CLIP(tmp3,  t3, t4, t5, t0, t1, t2 );
					
					tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);
					dst += dstStep;
					*(long *)(dst+0) = tmp3;

					if( width4 == 1 ) continue;

					t3 = src[11]; 	FILTER_AND_CLIP(tmp0,  t4, t5, t0, t1, t2, t3 );
					t4 = src[12]; 	FILTER_AND_CLIP(tmp1,  t5, t0, t1, t2, t3, t4 );
					t5 = src[13]; 	FILTER_AND_CLIP(tmp2,  t0, t1, t2, t3, t4, t5 );
					t0 = src[14]; 	FILTER_AND_CLIP(tmp3,  t1, t2, t3, t4, t5, t0 );
					
					tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);
					*(long *)(dst+4) = tmp3;

					if( width4 == 2 ) continue;

					t1 = src[15]; 	FILTER_AND_CLIP(tmp0,  t2, t3, t4, t5, t0, t1 );
					t2 = src[16]; 	FILTER_AND_CLIP(tmp1,  t3, t4, t5, t0, t1, t2 );
					t3 = src[17]; 	FILTER_AND_CLIP(tmp2,  t4, t5, t0, t1, t2, t3 );
					t4 = src[18];	FILTER_AND_CLIP(tmp3,  t5, t0, t1, t2, t3, t4 );
					
					tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);
					*(long *)(dst+8) = tmp3;

					t5 = src[19]; 	FILTER_AND_CLIP(tmp0,  t0, t1, t2, t3, t4, t5 );
					t0 = src[20]; 	FILTER_AND_CLIP(tmp1,  t1, t2, t3, t4, t5, t0 );
					t1 = src[21]; 	FILTER_AND_CLIP(tmp2,  t2, t3, t4, t5, t0, t1 );
					t2 = src[22]; 	FILTER_AND_CLIP(tmp3,  t3, t4, t5, t0, t1, t2 );
					
					tmp3 = (tmp0<<0) | (tmp1<<8) | (tmp2<<16) | (tmp3<<24);
					*(long *)(dst+12) = tmp3;
				}
			}
		break;

		case 4:			// dx = 0 && dy = 1
		case 12:		// dx = 0 && dy = 3
			{
				const Ipp8u *src = pSrc - (srcStep<<1);
				int	  const_255 = 255;
				int	  dyy = dy>> 1;
				int	  srcStep_6 = dyy*srcStep;
				
				// Prepair data in V filter
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						int t0, t1, t2, t3, t4, t5, t6, tmp, mm;
						const Ipp8u *s0 = src + i;		
										t0 = *s0;
						s0 += srcStep;	t1 = *s0; 
						s0 += srcStep;	t2 = *s0; t6 = *(s0 + srcStep_6 );
						s0 += srcStep;	t3 = *s0;
						s0 += srcStep;	t4 = *s0;
						s0 += srcStep;	t5 = *s0;

						FILTER_AND_CLIP(tmp, t0, t1, t2, t3, t4, t5 );
						tmp = (tmp + t6 + 1) >> 1;
						dst[i] = tmp;
					}
					src += srcStep;
					dst += dstStep;
				}
			}
			break;

		case 8:			// dx = 0 && dy = 2
			{
				const Ipp8u *src = pSrc - (srcStep<<1);
				int	  const_255 = 255;
				
				// Prepair data in V filter
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						int t0, t1, t2, t3, t4, t5, tmp, mm;
						const Ipp8u *s0 = src + i;		
						
										t0 = *s0;
						s0 += srcStep;	t1 = *s0;
						s0 += srcStep;	t2 = *s0;
						s0 += srcStep;	t3 = *s0;
						s0 += srcStep;	t4 = *s0;
						s0 += srcStep;	t5 = *s0;

						FILTER_AND_CLIP(tmp, t0, t1, t2, t3, t4, t5 );
						dst[i] = tmp;
					}
					src += srcStep;
					dst += dstStep;
				}
			}
			break;
		case 6:		// dx = 2 && dy = 1
		case 14:	// dx = 2 && dy = 3
			// Prepair data in H filter: Compute 1/2 position for H 
			{
				int		srcStep_2 = srcStep << 1;
				__ALIGN4(short, tt, TMP_Y*TMP_X);

				// Prepair data in H filter: Compute 1/2 position for H 
				const Ipp8u *src		= pSrc - srcStep_2 - srcStep - 4;
				short *t		= tt - TMP_X;
				for(j=0; j<(height+6); j++)
				{
					int t0, t1, t2, t3, t4, t5, mm;
					
					src += srcStep;

					t0 = src[2];
					t1 = src[3];
					t2 = src[4];
					t3 = src[5];
					t4 = src[6];
					t5 = src[7];
					t += TMP_X;    FILTER_ONE(t[0], t0, t1, t2, t3, t4, t5 );
					t0 = src[8];   FILTER_ONE(t[1], t1, t2, t3, t4, t5, t0 );
					t1 = src[9];   FILTER_ONE(t[2], t2, t3, t4, t5, t0, t1 );
					t2 = src[10];  FILTER_ONE(t[3], t3, t4, t5, t0, t1, t2 );

					if( width4 == 1 ) continue;

					t3 = src[11];  FILTER_ONE(t[4], t4, t5, t0, t1, t2, t3 );
					t4 = src[12];  FILTER_ONE(t[5], t5, t0, t1, t2, t3, t4 );
					t5 = src[13];  FILTER_ONE(t[6], t0, t1, t2, t3, t4, t5 );
					t0 = src[14];  FILTER_ONE(t[7], t1, t2, t3, t4, t5, t0 );

					if( width4 == 2 ) continue;

					t1 = src[15];  FILTER_ONE( t[8], t2, t3, t4, t5, t0, t1 );
					t2 = src[16];  FILTER_ONE( t[9], t3, t4, t5, t0, t1, t2 );
					t3 = src[17];  FILTER_ONE(t[10], t4, t5, t0, t1, t2, t3 );
					t4 = src[18];  FILTER_ONE(t[11], t5, t0, t1, t2, t3, t4 );

					t5 = src[19];  FILTER_ONE(t[12], t0, t1, t2, t3, t4, t5 );
					t0 = src[20];  FILTER_ONE(t[13], t1, t2, t3, t4, t5, t0 );
					t1 = src[21];  FILTER_ONE(t[14], t2, t3, t4, t5, t0, t1 );
					t2 = src[22];  FILTER_ONE(t[15], t3, t4, t5, t0, t1, t2 );
				}

				// do V filter: compute 1/2 position for V -- But just in position (2,2) ONLY, no vertical 1/2 position was computed based on integer vertical position
				int		const_255	= 255;
				int		dyy = dy>> 1;
				int	    dstStep2	= (dyy+2)*TMP_X;

				dst		= dst - dstStep;

#define STEP_3(i)	tmp1 = (512 + t[i] - 5*(t[TMP_X +i] + t[4*TMP_X +i]) + 20*(t[2*TMP_X +i] + t[3*TMP_X +i]) + t[5*TMP_X +i])>>10;		\
					tmp2 = ((t[dstStep2 +i] + 16)>>5);																					\
					Clip1( const_255, tmp1);																							\
					Clip1( const_255, tmp2);tmp1 = (tmp1 + tmp2 + 1) >> 1;																\
					dst[i] = tmp1;

				t = &tt[0] - TMP_X;
				for(j=0; j<height; j++)
				{
					int tmp1, tmp2;
					
					dst += dstStep;
					t   += TMP_X;
					
					STEP_3(0);
					STEP_3(1);
					STEP_3(2);
					STEP_3(3);

					if( width4 == 1 ) continue;

					STEP_3(4);
					STEP_3(5);
					STEP_3(6);
					STEP_3(7);

					if( width4 == 2 ) continue;

					STEP_3(8);
					STEP_3(9);
					STEP_3(10);
					STEP_3(11);

					STEP_3(12);
					STEP_3(13);
					STEP_3(14);
					STEP_3(15);
				}
			}
			break;

		case 10:	// dx = 2 && dy = 2
				// Prepair data in H filter: Compute 1/2 position for H 
			{
				__ALIGN4(short, tt, TMP_Y*TMP_X);
				short	*t			= tt;
				int		srcStep_2	= srcStep << 1;
				const Ipp8u	*src	= pSrc -2 - srcStep_2;

				// Prepair data in V filter
				for(j=0; j<height; j++)
				{
					for(i=0; i<width + 6; i++)
					{
						int t0, t1, t2, t3, t4, t5, mm;
						const Ipp8u *s0 = src + i;		
						
										t0 = *s0;
						s0 += srcStep;	t1 = *s0;
						s0 += srcStep;	t2 = *s0;
						s0 += srcStep;	t3 = *s0;
						s0 += srcStep;	t4 = *s0;
						s0 += srcStep;	t5 = *s0;

						FILTER_ONE( t[i], t0, t1, t2, t3, t4, t5 );
					}
					src += srcStep;
					t += TMP_X;
				}

				// do H filter
				//pdst = pDst;
				int const_255 = 255;

				t   = tt - TMP_X;
				dst -= dstStep;

				for(j=0; j<height; j++)
				{
					int t0, t1, t2, t3, t4, t5, mm;
					int tmp;

					dst += dstStep;
					t   += TMP_X;

					t0 = t[0];
					t1 = t[1];
					t2 = t[2];
					t3 = t[3];
					t4 = t[4];
					t5 = t[5];
								FILTER_ONE(tmp, t0, t1, t2, t3, t4, t5 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[ 0] = tmp;
					t0 = t[6];	FILTER_ONE(tmp, t1, t2, t3, t4, t5, t0 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[ 1] = tmp;
					t1 = t[7]; 	FILTER_ONE(tmp, t2, t3, t4, t5, t0, t1 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[ 2] = tmp;
					t2 = t[8]; 	FILTER_ONE(tmp, t3, t4, t5, t0, t1, t2 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[ 3] = tmp;

					if( width4 == 1 ) continue;

					t3 = t[9]; 	FILTER_ONE(tmp, t4, t5, t0, t1, t2, t3 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[ 4] = tmp;
					t4 = t[10];	FILTER_ONE(tmp, t5, t0, t1, t2, t3, t4 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[ 5] = tmp;
					t5 = t[11];	FILTER_ONE(tmp, t0, t1, t2, t3, t4, t5 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[ 6] = tmp;
					t0 = t[12];	FILTER_ONE(tmp, t1, t2, t3, t4, t5, t0 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[ 7] = tmp;

					if( width4 == 2 ) continue;

					t1 = t[13];	FILTER_ONE(tmp, t2, t3, t4, t5, t0, t1 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[ 8] = tmp;
					t2 = t[14];	FILTER_ONE(tmp, t3, t4, t5, t0, t1, t2 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[ 9] = tmp;
					t3 = t[15];	FILTER_ONE(tmp, t4, t5, t0, t1, t2, t3 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[10] = tmp;
					t4 = t[16];	FILTER_ONE(tmp, t5, t0, t1, t2, t3, t4 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[11] = tmp;

					t5 = t[17];	FILTER_ONE(tmp, t0, t1, t2, t3, t4, t5 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[12] = tmp;
					t0 = t[18];	FILTER_ONE(tmp, t1, t2, t3, t4, t5, t0 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[13] = tmp;
					t1 = t[19];	FILTER_ONE(tmp, t2, t3, t4, t5, t0, t1 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[14] = tmp;
					t2 = t[20];	FILTER_ONE(tmp, t3, t4, t5, t0, t1, t2 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); dst[15] = tmp;
				}
			}
			break;

		case 9:		// dx = 1 && dy = 2
		case 11:	// dx = 3 && dy = 2
				// Prepair data in V filter
			{
				__ALIGN4(short, tt, TMP_Y*TMP_X);
				short	*t			= tt;
				int		srcStep_2	= srcStep << 1;
				const Ipp8u	*src		= pSrc -2 - srcStep_2;
				
				// Prepair data in V filter
				for(j=0; j<height; j++)
				{
					for(i=0; i<width + 6; i++)
					{
						int t0, t1, t2, t3, t4, t5, mm;
						const Ipp8u *s0 = src + i;		
						
										t0 = *s0;
						s0 += srcStep;	t1 = *s0;
						s0 += srcStep;	t2 = *s0;
						s0 += srcStep;	t3 = *s0;
						s0 += srcStep;	t4 = *s0;
						s0 += srcStep;	t5 = *s0;

						 FILTER_ONE(t[i], t0, t1, t2, t3, t4, t5 );
					}
					src += srcStep;
					t += TMP_X;
				}

				// do H filter
				t     = tt - TMP_X;
				dst  -= dstStep;

				int		dxx = dx>> 1;
				short  *ttt = t + dxx + 2;
				int     const_255 = 255;

				for(j=0; j<height; j++)
				{
					int t0, t1, t2, t3, t4, t5;
					int tmp, tmp2, mm;

					dst += dstStep;
					t    += TMP_X;
					ttt  += TMP_X;

					t0 = t[0];
					t1 = t[1];
					t2 = t[2];
					t3 = t[3];
					t4 = t[4];
					t5 = t[5];
								FILTER_ONE(tmp,  t0, t1, t2, t3, t4, t5 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[ 0] + 16 )>>5; Clip1( const_255, tmp2 ); dst[ 0] = (tmp + tmp2 + 1)>>1;
					t0 = t[6];	FILTER_ONE(tmp,  t1, t2, t3, t4, t5, t0 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[ 1] + 16 )>>5; Clip1( const_255, tmp2 ); dst[ 1] = (tmp + tmp2 + 1)>>1; 
					t1 = t[7]; 	FILTER_ONE(tmp,  t2, t3, t4, t5, t0, t1 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[ 2] + 16 )>>5; Clip1( const_255, tmp2 ); dst[ 2] = (tmp + tmp2 + 1)>>1;
					t2 = t[8]; 	FILTER_ONE(tmp,  t3, t4, t5, t0, t1, t2 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[ 3] + 16 )>>5; Clip1( const_255, tmp2 ); dst[ 3] = (tmp + tmp2 + 1)>>1;

					if( width4 == 1 ) continue;

					t3 = t[9]; 	FILTER_ONE(tmp,  t4, t5, t0, t1, t2, t3 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[ 4] + 16 )>>5; Clip1( const_255, tmp2 ); dst[ 4] = (tmp + tmp2 + 1)>>1;
					t4 = t[10];	FILTER_ONE(tmp,  t5, t0, t1, t2, t3, t4 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[ 5] + 16 )>>5; Clip1( const_255, tmp2 ); dst[ 5] = (tmp + tmp2 + 1)>>1;
					t5 = t[11];	FILTER_ONE(tmp,  t0, t1, t2, t3, t4, t5 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[ 6] + 16 )>>5; Clip1( const_255, tmp2 ); dst[ 6] = (tmp + tmp2 + 1)>>1;
					t0 = t[12];	FILTER_ONE(tmp,  t1, t2, t3, t4, t5, t0 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[ 7] + 16 )>>5; Clip1( const_255, tmp2 ); dst[ 7] = (tmp + tmp2 + 1)>>1;

					if( width4 == 2 ) continue;

					t1 = t[13];	FILTER_ONE(tmp,  t2, t3, t4, t5, t0, t1 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[ 8] + 16 )>>5; Clip1( const_255, tmp2 ); dst[ 8] = (tmp + tmp2 + 1)>>1;
					t2 = t[14];	FILTER_ONE(tmp,  t3, t4, t5, t0, t1, t2 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[ 9] + 16 )>>5; Clip1( const_255, tmp2 ); dst[ 9] = (tmp + tmp2 + 1)>>1;
					t3 = t[15];	FILTER_ONE(tmp,  t4, t5, t0, t1, t2, t3 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[10] + 16 )>>5; Clip1( const_255, tmp2 ); dst[10] = (tmp + tmp2 + 1)>>1;
					t4 = t[16];	FILTER_ONE(tmp,  t5, t0, t1, t2, t3, t4 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[11] + 16 )>>5; Clip1( const_255, tmp2 ); dst[11] = (tmp + tmp2 + 1)>>1;

					t5 = t[17];	FILTER_ONE(tmp,  t0, t1, t2, t3, t4, t5 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[12] + 16 )>>5; Clip1( const_255, tmp2 ); dst[12] = (tmp + tmp2 + 1)>>1;
					t0 = t[18];	FILTER_ONE(tmp,  t1, t2, t3, t4, t5, t0 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[13] + 16 )>>5; Clip1( const_255, tmp2 ); dst[13] = (tmp + tmp2 + 1)>>1;
					t1 = t[19];	FILTER_ONE(tmp,  t2, t3, t4, t5, t0, t1 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[14] + 16 )>>5; Clip1( const_255, tmp2 ); dst[14] = (tmp + tmp2 + 1)>>1;
					t2 = t[20];	FILTER_ONE(tmp,  t3, t4, t5, t0, t1, t2 );	tmp = (512 + tmp) >> 10; Clip1( const_255, tmp ); tmp2 = (ttt[15] + 16 )>>5; Clip1( const_255, tmp2 ); dst[15] = (tmp + tmp2 + 1)>>1;
				}
			}
			break;

		case 5:
		case 7:
		case 13:
		case 15:
			{
				__ALIGN4(short, tt, TMP_Y*TMP_X);
				int		dxx = dx>> 1;
				int		dyy = dy>> 1;
				int		srcStep_2 = srcStep << 1;
				
				/* Algorithm 
				 *           G      b       H
				 *           d   e  f   g   
				 *           h   i  J   k   m
				 *           n   p  q   r
				 *           M      s       N
				 *
				 *
				*/
				// 以下的算法首先计算b/s，但是根据dyy则可能b不需要计算
				 /* Diagonal interpolation */
				// 这里需要进一步区分e,g  与 p, r （Spec）
				short *t = tt - TMP_X;
				int   const_255 = 255;
				const Ipp8u *src = pSrc + (dyy)*srcStep - 2 - srcStep;

				for(j=0; j< height; j++)
				{
					int t0, t1, t2, t3, t4, t5;
					int tmp, mm;

					t   += TMP_X;   
					src += srcStep;

					t0 = src[0];
					t1 = src[1];
					t2 = src[2];
					t3 = src[3];
					t4 = src[4];
					t5 = src[5];
								  FILTER_AND_CLIP(tmp, t0, t1, t2, t3, t4, t5 ); t[0]  = tmp;
					t0 = src[6];  FILTER_AND_CLIP(tmp, t1, t2, t3, t4, t5, t0 ); t[1]  = tmp;
					t1 = src[7];  FILTER_AND_CLIP(tmp, t2, t3, t4, t5, t0, t1 ); t[2]  = tmp; 
					t2 = src[8];  FILTER_AND_CLIP(tmp, t3, t4, t5, t0, t1, t2 ); t[3]  = tmp; 

					if( width4 == 1 ) continue;

					t3 = src[9];  FILTER_AND_CLIP(tmp, t4, t5, t0, t1, t2, t3 ); t[4]  = tmp;
					t4 = src[10]; FILTER_AND_CLIP(tmp, t5, t0, t1, t2, t3, t4 ); t[5]  = tmp;
					t5 = src[11]; FILTER_AND_CLIP(tmp, t0, t1, t2, t3, t4, t5 ); t[6]  = tmp;
					t0 = src[12]; FILTER_AND_CLIP(tmp, t1, t2, t3, t4, t5, t0 ); t[7]  = tmp;

					if( width4 == 2 ) continue;

					t1 = src[13]; FILTER_AND_CLIP(tmp, t2, t3, t4, t5, t0, t1 ); t[8]  = tmp;
					t2 = src[14]; FILTER_AND_CLIP(tmp, t3, t4, t5, t0, t1, t2 ); t[9]  = tmp;
					t3 = src[15]; FILTER_AND_CLIP(tmp, t4, t5, t0, t1, t2, t3 ); t[10] = tmp;
					t4 = src[16]; FILTER_AND_CLIP(tmp, t5, t0, t1, t2, t3, t4 ); t[11] = tmp;

					t5 = src[17]; FILTER_AND_CLIP(tmp, t0, t1, t2, t3, t4, t5 ); t[12] = tmp;
					t0 = src[18]; FILTER_AND_CLIP(tmp, t1, t2, t3, t4, t5, t0 ); t[13] = tmp;
					t1 = src[19]; FILTER_AND_CLIP(tmp, t2, t3, t4, t5, t0, t1 ); t[14] = tmp;
					t2 = src[20]; FILTER_AND_CLIP(tmp, t3, t4, t5, t0, t1, t2 ); t[15] = tmp;
				}

				// 以下计算h/m，但是根据dxx则可能h不需要计算
				//pdst = pDst;
				src = pSrc + dxx - srcStep_2;

				// Prepair data in V filter
				t = tt;
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						int t0, t1, t2, t3, t4, t5;
						int tmp, tmp2, mm;
						const Ipp8u *s0 = src + i;		
						
										t0 = *s0;
						s0 += srcStep;	t1 = *s0;
						s0 += srcStep;	t2 = *s0;
						s0 += srcStep;	t3 = *s0;
						s0 += srcStep;	t4 = *s0;
						s0 += srcStep;	t5 = *s0;

						FILTER_AND_CLIP(tmp, t0, t1, t2, t3, t4, t5 );
						tmp2 = t[i];
						
						dst[i] = (tmp + tmp2 + 1)>>1;
					}

					src  += srcStep;
					dst += dstStep;
					t    += TMP_X;
				}
			}
			break;
	}

	Profile_End(ippiInterpolateLuma_H264_8u_C1R_c_profile);

    return ippStsNoErr;
}

#endif

#endif  //#ifndef DROP_C

IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_bilinear_approx_c
(
  const Ipp8u*   src,
		Ipp32s   srcStep,
		Ipp8u*   dst,
		Ipp32s   dstStep,
		Ipp32s   dx,
		Ipp32s   dy,
		IppiSize roiSize
)
{
	Profile_Start(ippiInterpolateLuma_H264_8u_C1R_c_profile);

	dx >>= 1;
	dy >>= 1;
			
	bilinear_qpel
	(
		src,
		srcStep,
		dst,
		dstStep,
		dx,
		dy,
		roiSize
	);
		
	Profile_End(ippiInterpolateLuma_H264_8u_C1R_c_profile);
	
	return ippStsNoErr;
}


#if defined(__KINOMA_IPP_ARM_V5__) && TARGET_CPU_ARM

IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_16x16_arm_profile
(
  const Ipp8u*   pSrc,
		Ipp32s   srcStep,
		Ipp8u*   dst,
		Ipp32s   dstStep,
		Ipp32s   dx,
		Ipp32s   dy,
		IppiSize dummp
)
{
	Profile_Start(ippiInterpolateLuma_H264_8u_C1R_c_profile);
	//PLD((void*)pSrc);

	ippiInterpolateLuma_H264_8u_C1R_16x16_arm
	(
		pSrc,
		srcStep,
		dst,
		dstStep,
		dx,
		dy,
		dummp
	);	

#if 0
	static int count = 0;
	count++;
	//if(count >209 )
	unsigned char ref_buf1[256];
	unsigned char ref_buf2[256];

	ippiInterpolateLuma_H264_8u_C1R_c
	(
		pSrc,
		srcStep,
		ref_buf1,
		16,
		dx,
		dy,
		dummp
	);	

	ippiInterpolateLuma_H264_8u_C1R_16x16_arm
	(
		pSrc,
		srcStep,
		ref_buf2,
		16,
		dx,
		dy,
		dummp
	);	

	int i, j;
	unsigned char *r = ref_buf1;
	unsigned char *d = ref_buf2;
	int	diff = 0;
	for( j = 0; j < 16; j++ )
	{
		for( i = 0; i < 16; i++ )
		{
			int t1 = r[i];
			int t2 = d[i];
			if(t1 != t2 )
			{
				diff = 1;
				break;
			}
		}
		r += 16;
		d += 16;
	}

	if( diff )
	{
		fprintf(stderr, "error found\n");
		int ii, jj;
		unsigned char *rr = ref_buf1;
		unsigned char *dd = ref_buf2;
		for( jj = 0; jj < 16; jj++ )
		{
			for( ii = 0; ii < 8; ii++ )
			{
				if( dd[ii]==rr[ii] )
					fprintf(stderr, "%x/%x,", rr[ii], dd[ii] );
				else						
					fprintf(stderr, "%x/%x*", rr[ii], dd[ii] );
			}

			fprintf(stderr, "\n");
			rr += 16;
			dd += 16;
		}

		fprintf(stderr, "\n");

		const unsigned char *ss = pSrc;
		for( jj = 0; jj < 16; jj++ )
		{
			for( ii = 0; ii < 16; ii++ )
			{
				fprintf(stderr, "%x,", ss[ii] );
			}

			fprintf(stderr, "\n");
			ss += srcStep;
		}
		
		for( jj = 0; jj < 256; jj++ )
			ref_buf1[jj] = 0;
		
		ippiInterpolateLuma_H264_8u_C1R_16x16_arm
		(
			pSrc,
			srcStep,
			ref_buf1,
			16,
			dx,
			dy,
			dummp
		);
	}

#endif
	//Profile_End(ippiInterpolateLuma_H264_8u_C1R_c_profile);
	return ippStsNoErr;
}


IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_8x16_arm_profile
(
  const Ipp8u*   pSrc,
		Ipp32s   srcStep,
		Ipp8u*   dst,
		Ipp32s   dstStep,
		Ipp32s   dx,
		Ipp32s   dy,
		IppiSize dummp
)
{
	Profile_Start(ippiInterpolateLuma_H264_8u_C1R_c_profile);

	ippiInterpolateLuma_H264_8u_C1R_8x16_arm
	(
		pSrc,
		srcStep,
		dst,
		dstStep,
		dx,
		dy,
		dummp
	);

	Profile_End(ippiInterpolateLuma_H264_8u_C1R_c_profile);
	return ippStsNoErr;
}

IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_sub_arm_profile
(
  const Ipp8u*   pSrc,
		Ipp32s   srcStep,
		Ipp8u*   dst,
		Ipp32s   dstStep,
		Ipp32s   dx,
		Ipp32s   dy,
		IppiSize dummp
)
{
	Profile_Start(ippiInterpolateLuma_H264_8u_C1R_c_profile);

	ippiInterpolateLuma_H264_8u_C1R_arm
	(
		pSrc,
		srcStep,
		dst,
		dstStep,
		dx,
		dy,
		dummp
	);

	Profile_End(ippiInterpolateLuma_H264_8u_C1R_c_profile);
	return ippStsNoErr;
}

#endif

#else

//***for reference only

static const int COEF[6] = {    1, -5, 20, 20, -5, 1  };

_DECLSPEC const unsigned short InterPolationTbl20[] = {
    0,   20,   40,   60,   80,  100,  120,  140,  160,  180,
  200,  220,  240,  260,  280,  300,  320,  340,  360,  380,
  400,  420,  440,  460,  480,  500,  520,  540,  560,  580,
  600,  620,  640,  660,  680,  700,  720,  740,  760,  780,
  800,  820,  840,  860,  880,  900,  920,  940,  960,  980,
 1000, 1020, 1040, 1060, 1080, 1100, 1120, 1140, 1160, 1180,
 1200, 1220, 1240, 1260, 1280, 1300, 1320, 1340, 1360, 1380,
 1400, 1420, 1440, 1460, 1480, 1500, 1520, 1540, 1560, 1580,
 1600, 1620, 1640, 1660, 1680, 1700, 1720, 1740, 1760, 1780,
 1800, 1820, 1840, 1860, 1880, 1900, 1920, 1940, 1960, 1980,
 2000, 2020, 2040, 2060, 2080, 2100, 2120, 2140, 2160, 2180,
 2200, 2220, 2240, 2260, 2280, 2300, 2320, 2340, 2360, 2380,
 2400, 2420, 2440, 2460, 2480, 2500, 2520, 2540, 2560, 2580,
 2600, 2620, 2640, 2660, 2680, 2700, 2720, 2740, 2760, 2780,
 2800, 2820, 2840, 2860, 2880, 2900, 2920, 2940, 2960, 2980,
 3000, 3020, 3040, 3060, 3080, 3100, 3120, 3140, 3160, 3180,
 3200, 3220, 3240, 3260, 3280, 3300, 3320, 3340, 3360, 3380,
 3400, 3420, 3440, 3460, 3480, 3500, 3520, 3540, 3560, 3580,
 3600, 3620, 3640, 3660, 3680, 3700, 3720, 3740, 3760, 3780,
 3800, 3820, 3840, 3860, 3880, 3900, 3920, 3940, 3960, 3980,
 4000, 4020, 4040, 4060, 4080, 4100, 4120, 4140, 4160, 4180,
 4200, 4220, 4240, 4260, 4280, 4300, 4320, 4340, 4360, 4380,
 4400, 4420, 4440, 4460, 4480, 4500, 4520, 4540, 4560, 4580,
 4600, 4620, 4640, 4660, 4680, 4700, 4720, 4740, 4760, 4780,
 4800, 4820, 4840, 4860, 4880, 4900, 4920, 4940, 4960, 4980,
 5000, 5020, 5040, 5060, 5080, 5100, 5120, 5140, 5160, 5180,
 5200, 5220, 5240, 5260, 5280, 5300, 5320, 5340, 5360, 5380,
 5400, 5420, 5440, 5460, 5480, 5500, 5520, 5540, 5560, 5580,
 5600, 5620, 5640, 5660, 5680, 5700, 5720, 5740, 5760, 5780,
 5800, 5820, 5840, 5860, 5880, 5900, 5920, 5940, 5960, 5980,
 6000, 6020, 6040, 6060, 6080, 6100, 6120, 6140, 6160, 6180,
 6200, 6220, 6240, 6260, 6280, 6300, 6320, 6340, 6360, 6380,
 6400, 6420, 6440, 6460, 6480, 6500, 6520, 6540, 6560, 6580,
 6600, 6620, 6640, 6660, 6680, 6700, 6720, 6740, 6760, 6780,
 6800, 6820, 6840, 6860, 6880, 6900, 6920, 6940, 6960, 6980,
 7000, 7020, 7040, 7060, 7080, 7100, 7120, 7140, 7160, 7180,
 7200, 7220, 7240, 7260, 7280, 7300, 7320, 7340, 7360, 7380,
 7400, 7420, 7440, 7460, 7480, 7500, 7520, 7540, 7560, 7580,
 7600, 7620, 7640, 7660, 7680, 7700, 7720, 7740, 7760, 7780,
 7800, 7820, 7840, 7860, 7880, 7900, 7920, 7940, 7960, 7980,
 8000, 8020, 8040, 8060, 8080, 8100, 8120, 8140, 8160, 8180,
 8200, 8220, 8240, 8260, 8280, 8300, 8320, 8340, 8360, 8380,
 8400, 8420, 8440, 8460, 8480, 8500, 8520, 8540, 8560, 8580,
 8600, 8620, 8640, 8660, 8680, 8700, 8720, 8740, 8760, 8780,
 8800, 8820, 8840, 8860, 8880, 8900, 8920, 8940, 8960, 8980,
 9000, 9020, 9040, 9060, 9080, 9100, 9120, 9140, 9160, 9180,
 9200, 9220, 9240, 9260, 9280, 9300, 9320, 9340, 9360, 9380,
 9400, 9420, 9440, 9460, 9480, 9500, 9520, 9540, 9560, 9580,
 9600, 9620, 9640, 9660, 9680, 9700, 9720, 9740, 9760, 9780,
 9800, 9820, 9840, 9860, 9880, 9900, 9920, 9940, 9960, 9980,
10000,10020,10040,10060,10080,10100,10120,10140,10160,10180,
10200,10220
};

_DECLSPEC const unsigned short InterPolationTbl5[] = {
    0,    5,   10,   15,   20,   25,   30,   35,   40,   45,
   50,   55,   60,   65,   70,   75,   80,   85,   90,   95,
  100,  105,  110,  115,  120,  125,  130,  135,  140,  145,
  150,  155,  160,  165,  170,  175,  180,  185,  190,  195,
  200,  205,  210,  215,  220,  225,  230,  235,  240,  245,
  250,  255,  260,  265,  270,  275,  280,  285,  290,  295,
  300,  305,  310,  315,  320,  325,  330,  335,  340,  345,
  350,  355,  360,  365,  370,  375,  380,  385,  390,  395,
  400,  405,  410,  415,  420,  425,  430,  435,  440,  445,
  450,  455,  460,  465,  470,  475,  480,  485,  490,  495,
  500,  505,  510,  515,  520,  525,  530,  535,  540,  545,
  550,  555,  560,  565,  570,  575,  580,  585,  590,  595,
  600,  605,  610,  615,  620,  625,  630,  635,  640,  645,
  650,  655,  660,  665,  670,  675,  680,  685,  690,  695,
  700,  705,  710,  715,  720,  725,  730,  735,  740,  745,
  750,  755,  760,  765,  770,  775,  780,  785,  790,  795,
  800,  805,  810,  815,  820,  825,  830,  835,  840,  845,
  850,  855,  860,  865,  870,  875,  880,  885,  890,  895,
  900,  905,  910,  915,  920,  925,  930,  935,  940,  945,
  950,  955,  960,  965,  970,  975,  980,  985,  990,  995,
 1000, 1005, 1010, 1015, 1020, 1025, 1030, 1035, 1040, 1045,
 1050, 1055, 1060, 1065, 1070, 1075, 1080, 1085, 1090, 1095,
 1100, 1105, 1110, 1115, 1120, 1125, 1130, 1135, 1140, 1145,
 1150, 1155, 1160, 1165, 1170, 1175, 1180, 1185, 1190, 1195,
 1200, 1205, 1210, 1215, 1220, 1225, 1230, 1235, 1240, 1245,
 1250, 1255, 1260, 1265, 1270, 1275, 1280, 1285, 1290, 1295,
 1300, 1305, 1310, 1315, 1320, 1325, 1330, 1335, 1340, 1345,
 1350, 1355, 1360, 1365, 1370, 1375, 1380, 1385, 1390, 1395,
 1400, 1405, 1410, 1415, 1420, 1425, 1430, 1435, 1440, 1445,
 1450, 1455, 1460, 1465, 1470, 1475, 1480, 1485, 1490, 1495,
 1500, 1505, 1510, 1515, 1520, 1525, 1530, 1535, 1540, 1545,
 1550, 1555, 1560, 1565, 1570, 1575, 1580, 1585, 1590, 1595,
 1600, 1605, 1610, 1615, 1620, 1625, 1630, 1635, 1640, 1645,
 1650, 1655, 1660, 1665, 1670, 1675, 1680, 1685, 1690, 1695,
 1700, 1705, 1710, 1715, 1720, 1725, 1730, 1735, 1740, 1745,
 1750, 1755, 1760, 1765, 1770, 1775, 1780, 1785, 1790, 1795,
 1800, 1805, 1810, 1815, 1820, 1825, 1830, 1835, 1840, 1845,
 1850, 1855, 1860, 1865, 1870, 1875, 1880, 1885, 1890, 1895,
 1900, 1905, 1910, 1915, 1920, 1925, 1930, 1935, 1940, 1945,
 1950, 1955, 1960, 1965, 1970, 1975, 1980, 1985, 1990, 1995,
 2000, 2005, 2010, 2015, 2020, 2025, 2030, 2035, 2040, 2045,
 2050, 2055, 2060, 2065, 2070, 2075, 2080, 2085, 2090, 2095,
 2100, 2105, 2110, 2115, 2120, 2125, 2130, 2135, 2140, 2145,
 2150, 2155, 2160, 2165, 2170, 2175, 2180, 2185, 2190, 2195,
 2200, 2205, 2210, 2215, 2220, 2225, 2230, 2235, 2240, 2245,
 2250, 2255, 2260, 2265, 2270, 2275, 2280, 2285, 2290, 2295,
 2300, 2305, 2310, 2315, 2320, 2325, 2330, 2335, 2340, 2345,
 2350, 2355, 2360, 2365, 2370, 2375, 2380, 2385, 2390, 2395,
 2400, 2405, 2410, 2415, 2420, 2425, 2430, 2435, 2440, 2445,
 2450, 2455, 2460, 2465, 2470, 2475, 2480, 2485, 2490, 2495,
 2500, 2505, 2510, 2515, 2520, 2525, 2530, 2535, 2540, 2545,
 2550, 2555
};

_DECLSPEC const unsigned char ClipTbl[] = {
	 /* -80 -- -1 */
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,

	 /* 0 -- 255 */
     0,     1,     2,     3,     4,     5,     6,     7,     
	 8,     9,    10,    11,    12,    13,    14,    15,    
	 16,   17,    18,    19,    20,    21,    22,    23,    
	 24,   25,    26,    27,    28,    29,    30,    31,    
	 32,   33,    34,    35,    36,    37,    38,    39,    
	 40,   41,    42,    43,    44,    45,    46,    47,    
	 48,   49,    50,    51,    52,    53,    54,    55,    
	 56,   57,    58,    59,    60,    61,    62,    63,    
	 64,   65,    66,    67,    68,    69,    70,    71,    
	 72,   73,    74,    75,    76,    77,    78,    79,    
	 80,   81,    82,    83,    84,    85,    86,    87,    
	 88,   89,    90,    91,    92,    93,    94,    95,    
	 96,   97,    98,    99,   100,   101,   102,   103,   
	104,  105,   106,   107,   108,   109,   110,   111,  
	112,  113,   114,   115,   116,   117,   118,   119,  
	120,  121,   122,   123,   124,   125,   126,   127,  
	128,  129,   130,   131,   132,   133,   134,   135,  
	136,  137,   138,   139,   140,   141,   142,   143, 
	144,  145,   146,   147,   148,   149,   150,   151, 
	152,  153,   154,   155,   156,   157,   158,   159,  
	160,  161,   162,   163,   164,   165,   166,   167,   
	168,  169,   170,   171,   172,   173,   174,   175, 
	176,  177,   178,   179,   180,   181,   182,   183,   
	184,  185,   186,   187,   188,   189,   190,   191,
	192,  193,   194,   195,   196,   197,   198,   199, 
	200,  201,   202,   203,   204,   205,   206,   207,  
	208,  209,   210,   211,   212,   213,   214,   215, 
	216,  217,   218,   219,   220,   221,   222,   223, 
	224,  225,   226,   227,   228,   229,   230,   231, 
	232,  233,   234,   235,   236,   237,   238,   239, 
	240,  241,   242,   243,   244,   245,   246,   247,
	248,  249,   250,   251,   252,   253,   254,   255, 
	
	/* 256 -- 344 */
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255, 
	255,  255,   255,   255,   255,   255,   255,   255, 
	255,  255,   255,   255,   255,   255,   255,   255, 
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255
};

#define IClip2(x)    ClipTbl[(x)+80]

IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_c(
	  const Ipp8u*   pSrc,
			Ipp32s   srcStep,
			Ipp8u*   pDst,
			Ipp32s   dstStep,
			Ipp32s   dx,
			Ipp32s   dy,
			IppiSize roiSize)			// Must be 16,8 or 4
{

	Profile_Start(ippiInterpolateLuma_H264_8u_C1R_c_profile);
	
	int width = roiSize.width;
	int height= roiSize.height;

	_DECLSPEC int	temp_res[25][25];
/*

	static int xy22 = 0;
	static int xy24 = 0;
	static int xy42 = 0;
	static int xy44 = 0;
	static int xy48 = 0;
	static int xy84 = 0;
	static int xy88 = 0;
	static int xy816 = 0;
	static int xy168 = 0;
	static int xy1616 = 0;

	if( width == 2    && height == 2 )  xy22++;
	if( width == 2    && height == 4 )  xy24++;
	if( width == 4    && height == 4 )  xy44++;
	if( width == 4    && height == 8 )  xy48++;
	if( width == 8    && height == 4 )  xy84++;
	if( width == 8    && height == 8 )  xy88++;
	if( width == 8    && height == 16 ) xy816++;
	if( width == 16   && height == 8  ) xy168++;
	if( width == 16   && height == 16 ) xy1616++;

	fprintf( stderr, "xy22:%d, xy24:%d, xy44:%d, xy48:%d, xy84:%d, xy88:%d, xy816:%d, xy168:%d, xy1616:%d\n", 
						xy22,  xy24,     xy44,    xy48,   xy84,    xy88,    xy816,    xy168,    xy1616 );
*/
	int		i, j, k;

	const unsigned char	*psrc;
	unsigned char   *pdst;

	const unsigned int	*pSrcint;
	unsigned int   *pDstint;

	int		temp_result;

	int		dxx = dx>> 1;
	int		dyy = dy>> 1;

	int		casedxy = (dy<<2) + dx;


	int		srcStep_2 = srcStep << 1;
	int		dstStep_2 = dstStep << 1;

	int		srcStep_3 = srcStep_2 + srcStep;
	int		srcStep_4 = srcStep_2 + srcStep_2;
	int		srcStep_5 = srcStep_2 + srcStep_3;

	unsigned char	tmpvalue;

	// Check parameters
	if((height%4) || (width%4))
		return ippStsSizeErr;
	if(pSrc==NULL || pDst==NULL)
		return ippStsNullPtrErr;
	if(dx<0 || dx>3 || dy<0 || dy>3)
		return ippStsBadArgErr;
	if(srcStep < 16)
		return ippStsStepErr;


	switch(casedxy)
	{

		case 0:		// dx =0 && dy =0	
			psrc = pSrc;
			pdst = pDst;

			for(j=0; j<height; j++)
			{
#if defined(WIN32) && !defined(_WIN32_WCE)							
				// This is only valid and effective in INTEL and WIndows System -- WWD in 2006-06-10
				pSrcint = (unsigned int *) psrc;
				pDstint = (unsigned int *) pdst;				
				for(i=0; i<width; i+=4)
					*(pDstint++) = *(pSrcint++);				
#else
				memcpy( pdst, psrc, width);
#endif

				psrc += srcStep; pdst += dstStep;
			}

			break;
		case 1:		// dx = 1 && dy = 0
		case 3:		// dx = 3 && dy = 0
				psrc = pSrc - 4;
				pdst = pDst;
				{
				#ifdef LARGE_CACHE
					for(j=0; j<height; j++)
					{
						for(i=0; i<width; i++)
						{
							temp_result  = 16 + psrc[i +2 ] - InterPolationTbl5[psrc[i +3]+ psrc[i +6] ] + InterPolationTbl20[psrc[i+4] + psrc[i +5]] + psrc[i +7];
							tmpvalue = IClip2(temp_result>> 5);
							pdst[i] = (tmpvalue + psrc[i+4+dxx] + 1) >> 1;
							
						}
						psrc += srcStep;
						pdst += dstStep;
					}
				#else
					for(j=0; j<height; j++)
					{
						for(i=0; i<width; i++)
						{					
							temp_result  = 16 + psrc[i +2 ] - 5*psrc[i +3] + (psrc[i+4] + psrc[i +5]) *20 - 5*psrc[i +6] + psrc[i +7];
							//temp_result  = 16 + psrc[i -2 ] - InterPolationTbl5[psrc[i -1]] + InterPolationTbl20[psrc[i ]] + InterPolationTbl20[psrc[i +1]] - InterPolationTbl5[psrc[i +2]] + psrc[i +3];
							tmpvalue = IClip2(temp_result>> 5);
							
							temp_res[j][i] = (tmpvalue + psrc[i+4+dxx] + 1)>>1;
						}
						psrc += srcStep;
					}

					// Write Result
					for(j=0; j<height; j++)
					{
						for(i=0; i<width; i++)
						{					
							pdst[i] = temp_res[j][i];
						}
						pdst += dstStep;
					}
				#endif
				}

			break;

		case 2:		// dx = 2 && dy = 0
			psrc = pSrc - 4;
			pdst = pDst;
			{
			#ifdef LARGE_CACHE
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{			
						//temp_result  = 16 + psrc[i +2 ] - 5*psrc[i +3] + (psrc[i+4] + psrc[i +5]) *20 - 5*psrc[i +6] + psrc[i +7];
						temp_result  = 16 + psrc[i +2 ] - InterPolationTbl5[psrc[i +3]+ psrc[i +6] ] + InterPolationTbl20[psrc[i+4] + psrc[i +5]] + psrc[i +7];
						pdst[i] = IClip2((temp_result ) >> 5);
					}
					psrc += srcStep;
					pdst += dstStep;
				}
			#else
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{			
						temp_res[j][i]  = 16 + psrc[i +2 ] - 5*psrc[i +3] + (psrc[i+4] + psrc[i +5]) *20 - 5*psrc[i +6] + psrc[i +7];							
					}
					psrc += srcStep;
				}
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{			
						pdst[i] = IClip2((temp_res[j][i] ) >> 5);
					}
					pdst += dstStep;
				}
			#endif

			}

		break;

		case 4:			// dx = 0 && dy = 1
		case 12:		// dx = 0 && dy = 3
			psrc = pSrc-srcStep_2;
			pdst = pDst;					
			{
			#ifdef LARGE_CACHE
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{					
						//temp_result  = 16 + psrc[i] - 5*psrc[i +srcStep] + (psrc[i+srcStep_2 ] + psrc[i +srcStep_3]) *20 - 5*psrc[i +srcStep_4] + psrc[i +srcStep_5];
						temp_result  = 16 + psrc[i] - InterPolationTbl5[psrc[i +srcStep]+ psrc[i +srcStep_4] ] + InterPolationTbl20[psrc[i+srcStep_2 ] + psrc[i +srcStep_3]] + psrc[i +srcStep_5];
							
						pdst[i] = IClip2((temp_result) >> 5);

						pdst[i] = (pdst[i] + psrc[i+dyy*srcStep + srcStep_2] + 1) >> 1;
					}
					psrc += srcStep;
					pdst += dstStep;
				}
			#else
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{					
						temp_result  = 16 + psrc[i] - 5*psrc[i +srcStep] + (psrc[i+srcStep_2 ] + psrc[i +srcStep_3]) *20 - 5*psrc[i +srcStep_4] + psrc[i +srcStep_5];
							
						tmpvalue = IClip2((temp_result) >> 5);

						temp_res[j][i] = (tmpvalue + psrc[i+dyy*srcStep+srcStep_2] + 1) >> 1;
					}
					psrc += srcStep;
				}
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{					
						pdst[i] = temp_res[j][i];
					}
					pdst += dstStep;
				}
			#endif
			}
			break;

		case 8:			// dx = 0 && dy = 2
			psrc = pSrc - srcStep_2;
			pdst = pDst;				
			{
			#ifdef LARGE_CACHE
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						//temp_result  = 16 + psrc[i] - 5*psrc[i +srcStep] + (psrc[i+srcStep_2 ] + psrc[i +srcStep_3]) *20 - 5*psrc[i +srcStep_4] + psrc[i +srcStep_5];
						temp_result  = 16 + psrc[i] - InterPolationTbl5[psrc[i +srcStep]+ psrc[i +srcStep_4] ] + InterPolationTbl20[psrc[i+srcStep_2 ] + psrc[i +srcStep_3]] + psrc[i +srcStep_5];

						pdst[i] = IClip2((temp_result) >> 5);
					}
					psrc += srcStep;
					pdst += dstStep;
				}
			#else
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
						temp_res[j][i]  = 16 + psrc[i] - 5*psrc[i +srcStep] + (psrc[i+srcStep_2 ] + psrc[i +srcStep_3]) *20 - 5*psrc[i +srcStep_4] + psrc[i +srcStep_5];

					psrc += srcStep;
				}
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
						pdst[i] = IClip2((temp_res[j][i]) >> 5);
					pdst += dstStep;
				}
			#endif
			}
			break;
		case 6:		// dx = 2 && dy = 1
		case 14:	// dx = 2 && dy = 3
				// Prepair data in H filter: Compute 1/2 position for H 
			{
			// Prepair data in H filter: Compute 1/2 position for H 
			#ifdef LARGE_CACHE
				psrc = pSrc - srcStep_2 - 4;
				for(j=0; j<(height+6); j++)
				{
					for(i=0; i<width; i++)
						//temp_res[j][i]  = psrc[i +2 ] - 5*psrc[i +3] + 20*(psrc[i+4] + psrc[i +5])  - 5*psrc[i +6] + psrc[i +7];
						temp_res[j][i]  = psrc[i +2 ] - InterPolationTbl5[psrc[i +3] + psrc[i +6]]+ InterPolationTbl20[psrc[i+4] + psrc[i +5]]  + psrc[i +7];
					psrc += srcStep;
				}

				// do V filter: compute 1/2 position for V -- But just in position (2,2) ONLY, no vertical 1/2 position was computed based on integer vertical position
				pdst = pDst;
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						temp_result = 512 + temp_res[j][i] -5*temp_res[j + 1][i] + 20*(temp_res[j + 2][i] + temp_res[j + 3][i]) - 5*temp_res[j + 4][i] + temp_res[j + 5][i];
							
						pdst[i] = IClip(0, 255, (temp_result ) >> 10);
						pdst[i] = (pdst[i] + IClip(0, 255, ((temp_res[j+dyy+2][i] + 16)>>5)) + 1) >> 1;
					}

					pdst += dstStep;
				}
			#else
				psrc = pSrc - srcStep_2 - 4;
				for(j=0; j<(height+6); j++)
				{
					for(i=0; i<width; i++)
						temp_res[j][i]  = psrc[i +2 ] - 5*psrc[i +3] + 20*(psrc[i+4] + psrc[i +5])  - 5*psrc[i +6] + psrc[i +7];
					psrc += srcStep;
				}

				// do V filter: compute 1/2 position for V -- But just in position (2,2) ONLY, no vertical 1/2 position was computed based on integer vertical position
				pdst = pDst;
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						temp_result = 512 + temp_res[j][i] -5*temp_res[j + 1][i] + 20*(temp_res[j + 2][i] + temp_res[j + 3][i]) - 5*temp_res[j + 4][i] + temp_res[j + 5][i];
						
						pdst[i] = IClip(0, 255, (temp_result ) >> 10);
						pdst[i] = (pdst[i] + IClip(0, 255, ((temp_res[j+dyy+2][i] + 16)>>5)) + 1) >> 1;
					}

					pdst += dstStep;
				}
			#endif

			}

			break;

		case 10:	// dx = 2 && dy = 2
				// Prepair data in H filter: Compute 1/2 position for H 
			{
				psrc = pSrc -2 - srcStep_2;
			// Prepair data in V filter
			#ifdef LARGE_CACHE
				for(j=0; j<height; j++)
				{
					for(i=0; i<width+6; i++)
					{
						temp_result = 0;
						for(k=0; k<6; k++)
							temp_result += psrc[i +k*srcStep] * COEF[k];						
							
						temp_res[j][i] = temp_result;
					}
					psrc += srcStep;
				}

				// do H filter
				pdst = pDst;
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						temp_result = 512;

						for(k=0; k<6; k++)
							temp_result += temp_res[j][i + k] * COEF[k];
						
							
						pdst[i] = IClip(0, 255, (temp_result) >> 10);
					}
					pdst += dstStep;
				}
		#else
				for(j=0; j<height; j++)
				{
					for(i=0; i<width+6; i++)
					{
						temp_result =0;
						for(k=0; k<6; k++)
							temp_result += psrc[i +k*srcStep] * COEF[k];						
							
						temp_res[j][i] = temp_result;
					}
					psrc += srcStep;
				}

				// do H filter
				pdst = pDst;
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						temp_result = 512;

						for(k=0; k<6; k++)
							temp_result += temp_res[j][i + k] * COEF[k];
						
							
						pdst[i] = IClip(0, 255, (temp_result) >> 10);
					}
					pdst += dstStep;
				}
		#endif
			}

			break;

		case 9:		// dx = 1 && dy = 2
		case 11:	// dx = 3 && dy = 2
				// Prepair data in V filter
			{
				// Prepair data in V filter
				psrc = pSrc - srcStep_2 -2;
				for(j=0; j<height; j++)
				{
					// V filter
					for(i=0; i<width+6; i++)
					{
						temp_result =0;

						for(k=0; k<6; k++)
							temp_result += psrc[i + k*srcStep] * COEF[k];						
							
						temp_res[j][i] = temp_result;
					}
					psrc += srcStep;
				}

				// do H filter
				pdst = pDst ;
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						temp_result = 512;
						for(k=0; k<6; k++)
							temp_result += temp_res[j][i + k] * COEF[k];
						
							
						pdst[i] = IClip(0, 255, (temp_result) >> 10);
						pdst[i] = (pdst[i] + IClip(0, 255, ((temp_res[j][i+dxx+2]+16)>>5)) + 1) >> 1;

					}
					pdst += dstStep;
				}
			}

			break;

		case 5:
		case 7:
		case 13:
		case 15:
			{
				/* Algorithm 
				 *           G      b       H
				 *           d   e  f   g   
				 *           h   i  J   k   m
				 *           n   p  q   r
				 *           M      s       N
				 *
				 *
				*/
				// 以下的算法首先计算b/s，但是根据dyy则可能b不需要计算
				 /* Diagonal interpolation */
				// 这里需要进一步区分e,g  与 p, r （Spec）
				psrc = pSrc + (dyy)*srcStep -2;

				for (j = 0; j < height; j++) 
				{
					for (i = 0; i < width; i++) 
					{
						temp_result = 16;
						for (k = 0; k < 6; k++)
							temp_result += psrc[i + k] * COEF[k];
						
						temp_res[j][i] = IClip2((temp_result )>>5);
					}

					psrc += srcStep;
				}

				// 以下计算h/m，但是根据dxx则可能h不需要计算
				pdst = pDst;
				psrc = pSrc + dxx - srcStep_2;

				for (j = 0; j < height; j++) 
				{
					for (i = 0; i < width; i++) 
					{
						temp_result = 16;
						for (k = 0; k < 6; k++)
							temp_result += psrc[i + (k)*srcStep] * COEF[k];

						pdst[i] = (temp_res[j][i] + IClip2((temp_result )>>5) + 1) >> 1;

					}
					pdst += dstStep;
					psrc += srcStep;
				}

			}

			break;
	
	}

	Profile_End(ippiInterpolateLuma_H264_8u_C1R_c_profile);

    return ippStsNoErr;
}

#endif

//static int top_count = 0;
//static int bottom_count = 0;

#if 1
IppStatus  __STDCALL ippiInterpolateLumaTop_H264_8u_C1R_c(
        const Ipp8u*   pSrc,
        Ipp32s   srcStep,
        Ipp8u*   pDst,
        Ipp32s   dstStep,
        Ipp32s   dx,
        Ipp32s   dy,
        Ipp32s   outPixels,
        IppiSize roiSize)
{
    Ipp8u *pTemp;
    Ipp8u TemporalPixels[32*26];

    pTemp=CopyBlockFromTop((Ipp8u *)pSrc,TemporalPixels+32*4,srcStep,outPixels,dx,dy,roiSize);

	//top_count++;
	//printf("top: %d\n", top_count );
    return ippiInterpolateLuma_H264_8u_C1R_x(pTemp,32,pDst,dstStep,dx,dy,roiSize);
}
#else
IppStatus  __STDCALL ippiInterpolateLumaTop_H264_8u_C1R_c(
        const Ipp8u*   pSrc,
        Ipp32s   srcStep,
        Ipp8u*   pDst,
        Ipp32s   dstStep,
        Ipp32s   dx,
        Ipp32s   dy,
        Ipp32s   outPixels,
        IppiSize roi)
{
    Ipp8u *pTemp;
    Ipp8u TemporalPixels[32*26];
	Ipp8u *pOut;
	//Ipp8u *pOutReturn;
	int i;

	pTemp = pOut = TemporalPixels+32*4;
	Ipp32s padded_y = dy>0?3:0;
	Ipp32s padded_x = dx>0?3:0;
	pSrc+=srcStep*outPixels;

	Ipp32s num_outs = outPixels+padded_y;
	Ipp32s sbheight = roi.height+padded_y*2;

	assert(num_outs>=0);
	pSrc-=padded_x;
	pOut-=padded_y*32;
	pOut-=padded_x;

	if (num_outs>sbheight) //starting point outside bottom boundary
	{
		num_outs = roi.height+2*padded_y;
		for(i=0;i<num_outs;i++,pOut+=32)
			memcpy(pOut,pSrc,roi.width+2*padded_x);

		return ippiInterpolateLuma_H264_8u_C1R_x(pTemp,32,pDst,dstStep,dx,dy,roi);

	}
	else
	{
		for(i=0;i<num_outs;i++,pOut+=32)
			memcpy(pOut,pSrc,roi.width+2*padded_x);

		for(i=0;i<sbheight-num_outs;i++,pOut+=32,pSrc+=srcStep)
			memcpy(pOut,pSrc,roi.width+2*padded_x);
    
		return ippiInterpolateLuma_H264_8u_C1R_x(pTemp,32,pDst,dstStep,dx,dy,roi);
	}

}
#endif

IppStatus  __STDCALL ippiInterpolateLumaBottom_H264_8u_C1R_c(
       const Ipp8u*   pSrc,
       Ipp32s   srcStep,
       Ipp8u*   pDst,
       Ipp32s   dstStep,
       Ipp32s   dx,
       Ipp32s   dy,
       Ipp32s   outPixels,
       IppiSize roiSize)
{
    Ipp8u *pTemp;
    Ipp8u TemporalPixels[32*26];

    pTemp=CopyBlockFromBottom((Ipp8u *)pSrc,TemporalPixels+32*4,srcStep,outPixels,dx,dy,roiSize);

 	//bottom_count++;
	//printf("bottom: %d\n", bottom_count );
	return ippiInterpolateLuma_H264_8u_C1R_x(pTemp,32,pDst,dstStep,dx,dy,roiSize);
}

/*******************************************************************************
 * Function Name : ippiInterpolateChroma_H264_8u_C1R_x
 * Description   : Make interpolation for Chroma components
 * Author        : Wang Wendong
 * Version       : 0.1
 ******************************************************************************/
//1， 本函数进一步优化的策略在于roiSize中的数据 -- 将循环展开
//2， 将dx*dy事先计算好
//3， 检查精度问题

#if 1

IppStatus __STDCALL
ippiInterpolateChroma_H264_8u_C1R_2x2_c
(
	const   Ipp8u*  src,
	Ipp32s			srcStep,
	Ipp8u*			dst,
	Ipp32s			dstStep,
	Ipp32s			dx,
	Ipp32s			dy
)
{
	//int width  = 2;
	//int height = 2;

	if( dx==0 && dy==0 )
	{
		if( (((long)src)&0x01) == 0 )
		{
			*(short *)(dst+0)  = *(short *)(src+0);	
			src += srcStep;	dst += dstStep;
			*(short *)(dst+0)  = *(short *)(src+0);
		}
		else
		{
			int tmp0, tmp1, tmp3;
			
			tmp0 = src[0];	tmp1 = src[1];
			tmp3 = (tmp0<<0) | (tmp1<<8);
			*(short *)(dst+0) = tmp3;

			src += srcStep;
			tmp0 = src[0];	tmp1 = src[1];
			tmp3 = (tmp0<<0) | (tmp1<<8);
			dst += dstStep;
			*(short *)(dst+0) = tmp3;
		}
	}
	else if(dy == 0 )
	{
		if( dx == 4 )
		{
			int t0, t1;

			t0 = src[0];
			t1 = src[1];
			dst[0] = (t0 + t1 + 1) >> 1;	
			t0 = src[2];
			dst[1] = (t0 + t1 + 1) >> 1;
			
			src += srcStep;
			t0 = src[0];
			t1 = src[1];
			dst += dstStep;
			dst[0] = (t0 + t1 + 1) >> 1;	
			t0 = src[2];
			dst[1] = (t0 + t1 + 1) >> 1;
		}
		else
		{
			int	dx8 = 8 - dx;		// dx8 = [1, 7], dx = [1,7]

#define FILTER_DUB_x0(t0, t1)  (4 + (4<<16) + dx8*t0 + dx*t1 )
			{
				int r0, r1, tt0, tt1, t0, t1, t2, s0, s1, s2;

				t0 = src[0];
				t1 = src[1];				
				t2 = src[2];
				src += srcStep;
				s0 = src[0];
				s1 = src[1];
				s2 = src[2];

				tt0 = (t0<<16)|s0;			
				tt1 = (t1<<16)|s1;
				r0 = FILTER_DUB_x0(tt0, tt1);
				tt0 = (t2<<16)|s2;
				r1 = FILTER_DUB_x0(tt1, tt0);

				*(short *)dst = ((r1>>11)&0xff00)|(r0>>19);
				dst += dstStep;
				*(short *)dst = ((r1<<5)&0xff00)|((r0>>3)&0xff);
			}
		}
	}
	else if( dx == 0 )
	{
		int	dy8 = 8 - dy;	// dy8=[1,7],  dy=[1,7]

#define FILTER_DUB_0y(t0, t1)  (4 + (4<<16) + dy8*t0 + dy*t1 )
		{
			int tmp0, tmp1, t0, t1, s0, s1, s2, s3, s4, s5;

			s0 = src[0];		
			s3 = src[1];
			src += srcStep;
			s1 = src[0];	
			s4 = src[1];	
			src += srcStep;
			s2 = src[0]; 
			s5 = src[1]; 
			
			t0  = (s0<<16)|s1;
			t1  = (s1<<16)|s2;
			tmp0 = FILTER_DUB_0y( t0, t1);

			t0  = (s3<<16)|s4;
			t1  = (s4<<16)|s5;
			tmp1 = FILTER_DUB_0y( t0, t1);
			dst[0]			= tmp0>>19;
			dst[1]			= tmp1>>19;

			dst[0+dstStep]	= tmp0>>3;//***high bits are automaticlly clipped
			dst[1+dstStep]	= tmp1>>3;
		}
	}
	else
	{
		if( dx == 4 && dy == 4 )
		{
			int t00, t01, t10, t11, tmp0, tmp1;

			t00 = src[0];		  t01 = src[1];
			t10 = src[0+srcStep]; t11 = src[1+srcStep];
			tmp0  = ((t00 + t01 + t10 + t11 + 2)>>2);

			t00 = src[2];		  
			t10 = src[2+srcStep]; 
			tmp1  = ((t00 + t01 + t10 + t11 + 2)>>2);
			*(short *)(dst+0) = (tmp0<<0) | (tmp1<<8);//***little endian only

			src += srcStep;
			dst += dstStep;

			t00 = src[0];		  t01 = src[1];
			t10 = src[0+srcStep]; t11 = src[1+srcStep];
			tmp0  = ((t00 + t01 + t10 + t11 + 2)>>2);

			t00 = src[2];		  
			t10 = src[2+srcStep]; 
			tmp1  = ((t00 + t01 + t10 + t11 + 2)>>2);
			*(short *)(dst+0) = (tmp0<<0) | (tmp1<<8);//***little endian only
		}	
		else
		{
			int	dx8, dy8, dxy, dxy8;

			dx8 = dy*(8 - dx);
			dxy = dx*dy;
			dy8 = dx*(8 - dy);
			dxy8= (8-dx)*(8-dy);

#define FILTER_DUB(t0, t1, t2, t3)  (32 + (32<<16) + dxy8*t0 + dy8*t1 + dx8*t2 + dxy*t3 )
			{
				int tmp, t0, t1, t2, t3, s0, s1, s2, s3;

				s0 = src[0];		
				s1 = src[srcStep];	
				t0  = (s0<<16)|s1;

				s3 = src[srcStep<<1]; 
				src++;
				s0 = src[0];
				s2 = src[srcStep];
				t1  = (s0<<16)|s2;
				t2  = (s1<<16)|s3;

				s0 = src[srcStep<<1];
				t3  = (s2<<16)|s0;

				tmp = FILTER_DUB( t0, t1, t2, t3 );
				dst[0]			= tmp>>22;
				dst[0+dstStep]	= tmp>>6;//***high bits are automaticlly clipped

				src++;
				s0 = src[0];
				src += srcStep;
				s1 = src[0]; 
				src += srcStep;
				s2 = src[0]; 
				
				t0  = (s0<<16)|s1;
				t2  = (s1<<16)|s2;
				tmp = FILTER_DUB( t1, t0, t3, t2 );
				dst[1]			= tmp>>22;
				dst[1+dstStep]	= tmp>>6;
			}
		}
	}

	return ippStsNoErr;
}

#define H_STEP(idx, PATTERN, src, srcStep, dst, dstStep )			\
{																	\
	s2  = src[idx+1];												\
	s3  = src[idx+1+srcStep];										\
	s0 = src[idx+2];												\
	s1 = src[idx+2+srcStep];										\
	tt1 = (s2<<16)|s3;												\
																	\
	tmp = PATTERN(tt0, tt1);										\
	dst[idx]		   = tmp>>18;									\
	dst[idx+dstStep] = tmp>>2;										\
																	\
	tt0 = (s0<<16)|s1;												\
																	\
	tmp = PATTERN(tt1, tt0);										\
	dst[idx+1]		   = tmp>>18;									\
	dst[idx+1+dstStep] = tmp>>2;									\
}


#define HORIZONTAL_B_PATTERN( PATTERN, src, srcStep, dst, dstStep ) \
for(j=0; j<height; j+= 2)											\
{																	\
	int tmp, tt0, tt1, s0, s1, s2, s3;								\
																	\
	src += srcStep<<1;												\
	dst += dstStep<<1;												\
																	\
	s0 = src[0];													\
	s1 = src[0+srcStep];											\
	tt0 = (s0<<16)|s1;												\
																	\
	H_STEP(0, PATTERN, src, srcStep, dst, dstStep )					\
																	\
	if( width == 2 ) continue;										\
																	\
	H_STEP(2, PATTERN, src, srcStep, dst, dstStep )					\
																	\
	if( width == 4 ) continue;										\
																	\
	H_STEP(4, PATTERN, src, srcStep, dst, dstStep )					\
	H_STEP(6, PATTERN, src, srcStep, dst, dstStep )					\
																	\
	if( width == 8 ) continue;										\
																	\
	H_STEP( 8, PATTERN, src, srcStep, dst, dstStep )				\
	H_STEP(10, PATTERN, src, srcStep, dst, dstStep )				\
	H_STEP(12, PATTERN, src, srcStep, dst, dstStep )				\
	H_STEP(14, PATTERN, src, srcStep, dst, dstStep )				\
}

#define V_STEP(idx, PATTERN, src, srcStep, dst, dstStep )			\
{																	\
	s0 = src[idx];													\
	s1 = src[idx+ srcStep];											\
	s2 = src[idx+(srcStep<<1)];										\
																	\
	tt0  = (s0<<16)|s1;												\
	tt1  = (s1<<16)|s2;												\
																	\
	s0 = src[idx+1];												\
	s1 = src[idx+1+ srcStep];										\
	s2 = src[idx+1+(srcStep<<1)];									\
																	\
	tmp = PATTERN( tt0, tt1);										\
	dst[idx]			= tmp>>18;									\
	dst[idx+dstStep]	= tmp>>2;/*high bits are automaticlly clipped*/	\
																	\
	tt0  = (s0<<16)|s1;												\
	tt1  = (s1<<16)|s2;												\
	tmp = PATTERN( tt0, tt1);										\
	dst[idx+1]			= tmp>>18;									\
	dst[idx+1+dstStep]	= tmp>>2;/*high bits are automaticlly clipped*/	\
}



#define VERTICAL_B_PATTERN( PATTERN, src, srcStep, dst, dstStep )	\
																	\
for(j=0; j<height; j+= 2)											\
{																	\
	int tmp, tt0, tt1, s0, s1, s2;									\
																	\
	src += srcStep<<1;												\
	dst += dstStep<<1;												\
																	\
	V_STEP(0, PATTERN, src, srcStep, dst, dstStep)					\
																	\
	if( width == 2 ) continue;										\
																	\
	V_STEP(2, PATTERN, src, srcStep, dst, dstStep)					\
																	\
	if( width == 4 ) continue;										\
																	\
	V_STEP(4, PATTERN, src, srcStep, dst, dstStep)					\
	V_STEP(6, PATTERN, src, srcStep, dst, dstStep)					\
																	\
	if( width == 8 ) continue;										\
																	\
	V_STEP( 8, PATTERN, src, srcStep, dst, dstStep)					\
	V_STEP(10, PATTERN, src, srcStep, dst, dstStep)					\
	V_STEP(12, PATTERN, src, srcStep, dst, dstStep)					\
	V_STEP(14, PATTERN, src, srcStep, dst, dstStep)					\
}



#define B_STEP(idx, PATTERN, src, srcStep, dst, dstStep )			\
{																	\
	s0 = src[idx+1];												\
	s1 = src[idx+1+ srcStep];										\
	s2 = src[idx+1+(srcStep<<1)];									\
	t1  = (s0<<16)|s1;												\
	t3  = (s1<<16)|s2;												\
																	\
	s0 = src[idx+2];												\
	s1 = src[idx+2+ srcStep];										\
	s2 = src[idx+2+(srcStep<<1)];									\
																	\
	tmp = PATTERN( t0, t1, t2, t3 );								\
	dst[idx]			= tmp>>20;										\
	dst[idx+dstStep]	= tmp>>4;/*high bits are automaticlly clipped*/ \
																	\
	t0  = (s0<<16)|s1;												\
	t2  = (s1<<16)|s2;												\
	tmp = PATTERN( t1, t0, t3, t2 );								\
	dst[idx+1]			= tmp>>20;									\
	dst[idx+1+dstStep]	= tmp>>4;									\
}


#define MIDDLE_B_PATTERN( PATTERN, src, srcStep, dst, dstStep )		\
																	\
for(j=0; j<height; j+= 2)											\
{																	\
	int tmp, t0, t1, t2, t3, s0, s1, s2;							\
																	\
	src += srcStep<<1;												\
	dst += dstStep<<1;												\
																	\
	s0 = src[0];													\
	s1 = src[0+ srcStep];											\
	s2 = src[0+(srcStep<<1)];										\
	t0  = (s0<<16)|s1;												\
	t2  = (s1<<16)|s2;												\
																	\
	B_STEP(0, PATTERN, src, srcStep, dst, dstStep )					\
																	\
	if( width == 2 ) continue;										\
																	\
	B_STEP(2, PATTERN, src, srcStep, dst, dstStep )					\
																	\
	if( width == 4 ) continue;										\
																	\
	B_STEP(4, PATTERN, src, srcStep, dst, dstStep )					\
	B_STEP(6, PATTERN, src, srcStep, dst, dstStep )					\
																	\
	if( width == 8 ) continue;										\
																	\
	B_STEP( 8, PATTERN, src, srcStep, dst, dstStep )				\
	B_STEP(10, PATTERN, src, srcStep, dst, dstStep )				\
	B_STEP(12, PATTERN, src, srcStep, dst, dstStep )				\
	B_STEP(14, PATTERN, src, srcStep, dst, dstStep )				\
}

#define FILTER_DUB_x1(t0, t1)  ((2 + (4<<15) + 3*t0 + t1 ))
#define FILTER_DUB_x2(t0, t1)  ((1 + (4<<14) + t0 + t1 )<<1)
#define FILTER_DUB_x3(t0, t1)  ((2 + (4<<15) + t0 + 3*t1 ))

#define FILTER_DUB_y1(t0, t1)  ((2 + (4<<15) + 3*t0 + 1*t1 ))
#define FILTER_DUB_y2(t0, t1)  ((2 + (4<<15) + 2*t0 + 2*t1 ))
#define FILTER_DUB_y3(t0, t1)  ((2 + (4<<15) + 1*t0 + 3*t1 ))

#define FILTER_DUB_MID_11(t0, t1, t2, t3)  ((8 + (32<<14) + 9*t0 + 3*(t1 + t2) + t3   ))
#define FILTER_DUB_MID_12(t0, t1, t2, t3)  ((4 + (32<<13) + t1   + 3*(t0 + t2) + t3   )<<1)
#define FILTER_DUB_MID_13(t0, t1, t2, t3)  ((8 + (32<<14) + t1   + 3*(t0 + t3) + 9*t2 ))
#define FILTER_DUB_MID_21(t0, t1, t2, t3)  ((4 + (32<<13) + 3*(t0 + t1) + t2   + t3   )<<1)
#define FILTER_DUB_MID_22(t0, t1, t2, t3)  ((2 + (32<<12) + t0   + t1   + t2   + t3   )<<2)
#define FILTER_DUB_MID_23(t0, t1, t2, t3)  ((4 + (32<<13) + t0   + t1   + 3*(t2 + t3) )<<1)
#define FILTER_DUB_MID_31(t0, t1, t2, t3)  ((8 + (32<<14) + 3*(t0 + t3) + 9*t1 + t2   ))
#define FILTER_DUB_MID_32(t0, t1, t2, t3)  ((4 + (32<<13) + t0   + 3*(t1 + t3) + t2 )<<1)
#define FILTER_DUB_MID_33(t0, t1, t2, t3)  ((8 + (32<<14) + t0   + 3*(t1 + t2) + 9*t3 ))

void bilinear_qpel
(
	const   Ipp8u*  src,
	Ipp32s			srcStep,
	Ipp8u*			dst,
	Ipp32s			dstStep,
	Ipp32s			dx,
	Ipp32s			dy,
	IppiSize		roiSize
)
{	
	int	casedxy = (dy<<2) + dx;
	int	j;
	int width  = roiSize.width;
	int height = roiSize.height;

	switch(casedxy)
	{
		case 0:
		{
			src -= srcStep;
			dst -= dstStep;
			if( (((long)src)&0x01) == 0 )
				for(j=0; j<height; j++)
				{
					src += srcStep;
					dst += dstStep;
					*(short *)(dst+0)  = *(short *)(src+0);

					if( width == 2) continue;
						
					*(short *)(dst+2) = *(short *)(src+2);
					
					if( width == 4) continue;
					
					*(short *)(dst+4) = *(short *)(src+4);
					*(short *)(dst+6) = *(short *)(src+6);
					
					if( width == 8) continue;
					
					*(short *)(dst+ 8) = *(short *)(src+ 8);
					*(short *)(dst+10) = *(short *)(src+10);
					*(short *)(dst+12) = *(short *)(src+12);
					*(short *)(dst+14) = *(short *)(src+14);
				}
			else
				for(j=0; j<height; j++)
				{
#define COPY_STEP(idx )						\
{											\
	tmp0 = src[idx];	tmp1 = src[idx+1];	\
	tmp3 = (tmp0<<0) | (tmp1<<8);			\
	*(short *)(dst+idx) = tmp3;				\
}
					int tmp0, tmp1, tmp3;
					
					src += srcStep;
					dst += dstStep;	
					COPY_STEP(0)
					
					if( width == 2 ) continue;

					COPY_STEP(2)
						
					if( width == 4 ) continue;

					COPY_STEP(4)
					COPY_STEP(6)

					if( width == 8 ) continue;

					COPY_STEP( 8)
					COPY_STEP(10)
					COPY_STEP(12)
					COPY_STEP(14)
				}
			}
			break;

		case 1: 
			src -= srcStep<<1;
			dst -= dstStep<<1;
			HORIZONTAL_B_PATTERN( FILTER_DUB_x1, src, srcStep, dst, dstStep )
			break;

		case 2:
			src -= srcStep<<1;
			dst -= dstStep<<1;
			HORIZONTAL_B_PATTERN( FILTER_DUB_x2, src, srcStep, dst, dstStep )
			break;

		case 3:
			src -= srcStep<<1;
			dst -= dstStep<<1;
			HORIZONTAL_B_PATTERN( FILTER_DUB_x3, src, srcStep, dst, dstStep )
			break;

		case 4:
			src -= srcStep<<1;
			dst -= dstStep<<1;
			VERTICAL_B_PATTERN( FILTER_DUB_y1, src, srcStep, dst, dstStep )
			break;

		case 8:
			src -= srcStep<<1;
			dst -= dstStep<<1;
			VERTICAL_B_PATTERN( FILTER_DUB_y2, src, srcStep, dst, dstStep )
			break;

		case 12:
			src -= srcStep<<1;
			dst -= dstStep<<1;
			VERTICAL_B_PATTERN( FILTER_DUB_y3, src, srcStep, dst, dstStep )
			break;

		case 5:
			//if( dx == 1 && dy == 1 )
			src -= srcStep<<1;
			dst -= dstStep<<1;
			MIDDLE_B_PATTERN( FILTER_DUB_MID_11, src, srcStep, dst, dstStep )
			break;

		case 9:
			//else if( dx == 1 && dy == 2 )
			src -= srcStep<<1;
			dst -= dstStep<<1;
			MIDDLE_B_PATTERN( FILTER_DUB_MID_12, src, srcStep, dst, dstStep )
			break;

		case 13:
			//else if( dx == 1 && dy == 3 )
			src -= srcStep<<1;
			dst -= dstStep<<1;
			MIDDLE_B_PATTERN( FILTER_DUB_MID_13, src, srcStep, dst, dstStep )
			break;

		case 6:
			//else if( dx == 2 && dy == 1 )
			src -= srcStep<<1;
			dst -= dstStep<<1;
			MIDDLE_B_PATTERN( FILTER_DUB_MID_21, src, srcStep, dst, dstStep )
			break;

		case 10:
			//else if( dx == 2 && dy == 2 )
			src -= srcStep<<1;
			dst -= dstStep<<1;
			MIDDLE_B_PATTERN( FILTER_DUB_MID_22, src, srcStep, dst, dstStep )
			break;

		case 14:
			//else if( dx == 2 && dy == 3 )
			src -= srcStep<<1;
			dst -= dstStep<<1;
			MIDDLE_B_PATTERN( FILTER_DUB_MID_23, src, srcStep, dst, dstStep )
			break;

		case 7:
			//else if( dx == 3 && dy == 1 )
			src -= srcStep<<1;
			dst -= dstStep<<1;
			MIDDLE_B_PATTERN( FILTER_DUB_MID_31, src, srcStep, dst, dstStep )
			break;

		case 11:
			//else if( dx == 3 && dy == 2 )
			src -= srcStep<<1;
			dst -= dstStep<<1;
			MIDDLE_B_PATTERN( FILTER_DUB_MID_32, src, srcStep, dst, dstStep )
			break;

		case 15:
			//else if( dx == 3 && dy == 3 )
			src -= srcStep<<1;
			dst -= dstStep<<1;
			MIDDLE_B_PATTERN( FILTER_DUB_MID_33, src, srcStep, dst, dstStep )
			break;
	}
}


IppStatus __STDCALL
ippiInterpolateChroma_H264_8u_C1R_c
(
	const   Ipp8u*  src,
	Ipp32s			srcStep,
	Ipp8u*			dst,
	Ipp32s			dstStep,
	Ipp32s			dx,
	Ipp32s			dy,
	IppiSize		roiSize
)
{
#ifdef PPRFILE_IPP

	Profile_Start(ippiInterpolateChroma_H264_8u_C1R_c_profile);

	IppStatus status;

	status = ippiInterpolateChroma_H264_8u_C1R
	(
		src,
		srcStep,
		dst,
		dstStep,
		dx,
		dy,
		roiSize
	);

	Profile_End(ippiInterpolateChroma_H264_8u_C1R_c_profile);

	return status;
#endif

#if 0
	if( (dx&0x01) == 0 &&  (dy&0x01) == 0 )
	{
		Profile_Start(ippiInterpolateChroma_H264_8u_C1R_c_profile);
				
		dx >>= 1;
		dy >>= 1;
				
		bilinear_qpel
		(
			src,
			srcStep,
			dst,
			dstStep,
			dx,
			dy,
			roiSize
		);

		Profile_End(ippiInterpolateChroma_H264_8u_C1R_c_profile);
		
		return ippStsNoErr;
	}
#endif

	Profile_Start(ippiInterpolateChroma_H264_8u_C1R_c_profile);

	int width  = roiSize.width;
	int height = roiSize.height;

	//dx &= 0xFE;
	//dy &= 0xFE;
	if( width == 2 && height == 2 )
	{
		ippiInterpolateChroma_H264_8u_C1R_2x2_c( src,srcStep,dst,dstStep,dx,dy );
		Profile_End(ippiInterpolateChroma_H264_8u_C1R_c_profile);
		return ippStsNoErr;
	}

	int width2 = width>>1;
	int j;

	if( dx==0 && dy==0 )
	{
		src = src - srcStep;
		dst = dst - dstStep;
		if( (((long)src)&0x01) == 0 )
			for(j=0; j<height; j++)
			{
				src += srcStep;
				dst += dstStep;
				*(short *)(dst+0)  = *(short *)(src+0);

				if( width2 == 1) continue;
					
				*(short *)(dst+2) = *(short *)(src+2);
				
				if( width2 == 2) continue;
				
				*(short *)(dst+4) = *(short *)(src+4);
				*(short *)(dst+6) = *(short *)(src+6);
			}
		else
			for(j=0; j<height; j++)
			{
				int tmp0, tmp1, tmp3;
				
				src += srcStep;
				tmp0 = src[0];	tmp1 = src[1];
				tmp3 = (tmp0<<0) | (tmp1<<8);
				dst += dstStep;
				*(short *)(dst+0) = tmp3;
				
				if( width2 == 1 ) continue;

				tmp0 = src[2];	tmp1 = src[3];
				tmp3 = (tmp0<<0) | (tmp1<<8);
				*(short *)(dst+2) = tmp3;
				
				if( width2 == 2 ) continue;

				tmp0 = src[4];	tmp1 = src[5];;
				tmp3 = (tmp0<<0) | (tmp1<<8);
				*(short *)(dst+4) = tmp3;
				
				tmp0 = src[6];tmp1 = src[7];;
				tmp3 = (tmp0<<0) | (tmp1<<8);
				*(short *)(dst+6) = tmp3;
			}
	}
	else if(dy == 0 )
	{
		if( dx == 4 )
		{
			src -= srcStep;
			dst -= dstStep;
			for(j=0; j<height; j++)
			{
				int t0, t1;

				src += srcStep;
				dst += dstStep;

				t0 = src[0];
				t1 = src[1];
							dst[0] = (t0 + t1 + 1) >> 1;	
				t0 = src[2];dst[1] = (t0 + t1 + 1) >> 1;
				
				if( width2 == 1 ) continue;

				t1 = src[3];dst[2] = (t0 + t1 + 1) >> 1;
				t0 = src[4];dst[3] = (t0 + t1 + 1) >> 1;

				if( width2 == 2 ) continue;

				t1 = src[5];dst[4] = (t0 + t1 + 1) >> 1;
				t0 = src[6];dst[5] = (t0 + t1 + 1) >> 1;
				t1 = src[7];dst[6] = (t0 + t1 + 1) >> 1;
				t0 = src[8];dst[7] = (t0 + t1 + 1) >> 1;
			}
		}
		else
		{
			int	dx8 = 8 - dx;		// dx8 = [1, 7], dx = [1,7]

#if 1
#define FILTER_DUB_x0(t0, t1)  (4 + (4<<16) + dx8*t0 + dx*t1 )
			src -= srcStep<<1;
			dst -= dstStep<<1;

			for(j=0; j<height; j+= 2)
			{
				int t, tt0, tt1, t0, t1, s0, s1;

				src += srcStep<<1;

				t0   = src[0];
				s0   = src[0+srcStep];
				t1   = src[1];				
				s1   = src[1+srcStep];	
				dst += dstStep<<1;
				
				tt0 = (t0<<16)|s0;			
				t0  = src[2];			
				s0  = src[2+srcStep];
				tt1 = (t1<<16)|s1;
				t   = FILTER_DUB_x0(tt0, tt1);
				dst[0]		   = t>>19;
				dst[0+dstStep] = t>>3;

				tt0 = (t0<<16)|s0;
				t = FILTER_DUB_x0(tt1, tt0);
				dst[1]		   = t>>19;
				dst[1+dstStep] = t>>3;
								
				if( width2 == 1 ) continue;

				t1 = src[3];				
				s1 = src[3+srcStep];	
				t0 = src[4];			
				s0 = src[4+srcStep];
				
				tt1 = (t1<<16)|s1;
				t = FILTER_DUB_x0(tt0, tt1);
				dst[2]		   = t>>19;
				dst[2+dstStep] = t>>3;

				tt0 = (t0<<16)|s0;
				t = FILTER_DUB_x0(tt1, tt0);
				dst[3]		   = t>>19;
				dst[3+dstStep] = t>>3;

				if( width2 == 2 ) continue;

				t1 = src[5];				
				s1 = src[5+srcStep];	
				t0 = src[6];			
				s0 = src[6+srcStep];
				
				tt1 = (t1<<16)|s1;
				t = FILTER_DUB_x0(tt0, tt1);
				dst[4]		   = t>>19;
				dst[4+dstStep] = t>>3;

				tt0 = (t0<<16)|s0;
				t = FILTER_DUB_x0(tt1, tt0);
				dst[5]		   = t>>19;
				dst[5+dstStep] = t>>3;
				
				t1 = src[7];				
				s1 = src[7+srcStep];	
				t0 = src[8];			
				s0 = src[8+srcStep];
				
				tt1 = (t1<<16)|s1;
				t = FILTER_DUB_x0(tt0, tt1);
				dst[6]		   = t>>19;
				dst[6+dstStep] = t>>3;

				tt0 = (t0<<16)|s0;
				t = FILTER_DUB_x0(tt1, tt0);
				dst[7]		   = t>>19;
				dst[7+dstStep] = t>>3;
			}
#else
			src -= srcStep;
			dst -= dstStep;

			for(j=0; j<height; j++)
			{
				int t0, t1, tmp0, tmp1;

				src += srcStep;
				dst += dstStep;

				t0 = src[0];
				t1 = src[1];
							tmp0 = (t0*dx8 + t1*dx + 4) >> 3;	
				t0 = src[2];tmp1 = (t1*dx8 + t0*dx + 4) >> 3; *(short *)(dst + 0 ) = (tmp0<<0) | (tmp1<<8);//***little endian only
				
				if( width2 == 1 ) continue;

				t1 = src[3];tmp0 = (t0*dx8 + t1*dx + 4) >> 3;
				t0 = src[4];tmp1 = (t1*dx8 + t0*dx + 4) >> 3; *(short *)(dst + 2 ) = (tmp0<<0) | (tmp1<<8);//***little endian only

				if( width2 == 2 ) continue;

				t1 = src[5];tmp0 = (t0*dx8 + t1*dx + 4) >> 3;
				t0 = src[6];tmp1 = (t1*dx8 + t0*dx + 4) >> 3; *(short *)(dst + 4 ) = (tmp0<<0) | (tmp1<<8);//***little endian only
				t1 = src[7];tmp0 = (t0*dx8 + t1*dx + 4) >> 3;
				t0 = src[8];tmp1 = (t1*dx8 + t0*dx + 4) >> 3; *(short *)(dst + 6 ) = (tmp0<<0) | (tmp1<<8);//***little endian only
			}
#endif
		}
	}
	else if( dx == 0 )
	{
		int	dy8 = 8 - dy;	// dy8=[1,7],  dy=[1,7]

#if 0
#define FILTER_DUB_0y(t0, t1)  (4 + (4<<16) + dy8*t0 + dy*t1 )
		src -= srcStep<<1;
		dst -= dstStep<<1;
		for(j=0; j<height; j+= 2) 
		{
			int tmp, t0, t1, s0, s1, s2;

			src += srcStep<<1;
			dst += dstStep<<1;

			s0 = src[0];		
			s1 = src[0+ srcStep];	
			t0  = (s0<<16)|s1;
			s0 = src[0+(srcStep<<1)]; 
			t1  = (s1<<16)|s0;
			tmp = FILTER_DUB_0y( t0, t1);
			dst[0]			= tmp>>19;
			dst[0+dstStep]	= tmp>>3;//***high bits are automaticlly clipped

			s0 = src[1];		
			s1 = src[1+ srcStep];	
			t0  = (s0<<16)|s1;
			s0 = src[1+(srcStep<<1)]; 
			t1  = (s1<<16)|s0;
			tmp = FILTER_DUB_0y( t0, t1);
			dst[1]			= tmp>>19;
			dst[1+dstStep]	= tmp>>3;

			if( width2 == 1 ) continue;

			s0 = src[2];		
			s1 = src[2+ srcStep];	
			t0  = (s0<<16)|s1;
			s0 = src[2+(srcStep<<1)]; 
			t1  = (s1<<16)|s0;
			tmp = FILTER_DUB_0y( t0, t1);
			dst[2]			= tmp>>19;
			dst[2+dstStep]	= tmp>>3;//***high bits are automaticlly clipped

			s0 = src[3];		
			s1 = src[3+ srcStep];	
			t0  = (s0<<16)|s1;
			s0 = src[3+(srcStep<<1)]; 
			t1  = (s1<<16)|s0;
			tmp = FILTER_DUB_0y( t0, t1);
			dst[3]			= tmp>>19;
			dst[3+dstStep]	= tmp>>3;

			if( width2 == 2 ) continue;

			s0 = src[4];		
			s1 = src[4+ srcStep];	
			t0  = (s0<<16)|s1;
			s0 = src[4+(srcStep<<1)]; 
			t1  = (s1<<16)|s0;
			tmp = FILTER_DUB_0y( t0, t1);
			dst[4]			= tmp>>19;
			dst[4+dstStep]	= tmp>>3;//***high bits are automaticlly clipped

			s0 = src[5];		
			s1 = src[5+ srcStep];	
			t0  = (s0<<16)|s1;
			s0 = src[5+(srcStep<<1)]; 
			t1  = (s1<<16)|s0;
			tmp = FILTER_DUB_0y( t0, t1);
			dst[5]			= tmp>>19;
			dst[5+dstStep]	= tmp>>3;

			s0 = src[6];		
			s1 = src[6+ srcStep];	
			t0  = (s0<<16)|s1;
			s0 = src[6+(srcStep<<1)]; 
			t1  = (s1<<16)|s0;
			tmp = FILTER_DUB_0y( t0, t1);
			dst[6]			= tmp>>19;
			dst[6+dstStep]	= tmp>>3;//***high bits are automaticlly clipped

			s0 = src[7];		
			s1 = src[7+ srcStep];	
			t0  = (s0<<16)|s1;
			s0 = src[7+(srcStep<<1)]; 
			t1  = (s1<<16)|s0;
			tmp = FILTER_DUB_0y( t0, t1);
			dst[7]			= tmp>>19;
			dst[7+dstStep]	= tmp>>3;

		}
#else
		src -= srcStep;
		dst -= dstStep;
		for(j=0; j<height; j++)
		{
			int t0, t1, t2, t3, tt0, tt1;
			
			src += srcStep;
			dst += dstStep;

			t0 = src[0];
			t2 = src[1];
			t1 = src[0+srcStep]; 
			t3 = src[1+srcStep]; 
			tt0 = (t0*dy8 + t1*dy + 4) >> 3;	
			
			tt1 = (t2*dy8 + t3*dy + 4) >> 3;
			tt0 = (tt0<<0) | (tt1<<8);
			*(short *)(dst+0) = tt0;

			if( width2 == 1 ) continue;

			t0 = src[2];
			t2 = src[3];
			t1 = src[2+srcStep]; 
			t3 = src[3+srcStep]; 
			tt0 = (t0*dy8 + t1*dy + 4) >> 3;	
			tt1 = (t2*dy8 + t3*dy + 4) >> 3;
			tt0 = (tt0<<0) | (tt1<<8);
			*(short *)(dst+2) = tt0;
			if( width2 == 2 ) continue;

			t0 = src[4];
			t2 = src[5];
			t1 = src[4+srcStep]; 
			t3 = src[5+srcStep]; 
			tt0 = (t0*dy8 + t1*dy + 4) >> 3;	
			tt1 = (t2*dy8 + t3*dy + 4) >> 3;
			tt0 = (tt0<<0) | (tt1<<8);
			*(short *)(dst+4) = tt0;
			
			t0 = src[6];
			t2 = src[7];
			t1 = src[6+srcStep]; 
			t3 = src[7+srcStep]; 
			tt0 = (t0*dy8 + t1*dy + 4) >> 3;	
			tt1 = (t2*dy8 + t3*dy + 4) >> 3;
			tt0 = (tt0<<0) | (tt1<<8);
			*(short *)(dst+6) = tt0;
		}
#endif
	}
	else
	{
		if( dx == 4 && dy == 4 )
		{
			src -= srcStep;
			dst -= dstStep;
			for(j=0; j<height; j++)
			{
				int t00, t01, t02, t12, t10, t11, tmp0, tmp1;

				src += srcStep;
				dst += dstStep;

				t00 = src[0];		  
				t01 = src[1];
				t02 = src[2];		  
				t10 = src[0+srcStep]; 
				t11 = src[1+srcStep];
				t12 = src[2+srcStep]; 
				tmp0  = ((t00 + t01 + t10 + t11 + 2)>>2);
				tmp1  = ((t02 + t01 + t12 + t11 + 2)>>2);
				*(short *)(dst+0) = (tmp0<<0) | (tmp1<<8);//***little endian only

				if( width2 == 1 ) continue;

				t01 = src[3];		  
				t11 = src[3+srcStep]; 
				t00 = src[4];		  
				t10 = src[4+srcStep]; 
				tmp0  = ((t02 + t01 + t12 + t11 + 2)>>2);
				tmp1  = ((t00 + t01 + t10 + t11 + 2)>>2);
				*(short *)(dst+2) = (tmp0<<0) | (tmp1<<8);//***little endian only

				if( width2 == 2 ) continue;

				t01 = src[5];		  
				t02 = src[6];		  
				t11 = src[5+srcStep]; 
				t12 = src[6+srcStep]; 
				tmp0  = ((t00 + t01 + t10 + t11 + 2)>>2);
				tmp1  = ((t02 + t01 + t12 + t11 + 2)>>2);
				*(short *)(dst+4) = (tmp0<<0) | (tmp1<<8);//***little endian only

				t01 = src[7];		  
				t11 = src[7+srcStep]; 
				t00 = src[8];		  
				t10 = src[8+srcStep]; 
				tmp0  = ((t02 + t01 + t12 + t11 + 2)>>2);				
				tmp1  = ((t00 + t01 + t10 + t11 + 2)>>2);
				*(short *)(dst+6) = (tmp0<<0) | (tmp1<<8);//***little endian only
			}
		}	
		else
		{
			int	dx8, dy8, dxy, dxy8;

			dx8 = dy*(8 - dx);
			dxy = dx*dy;
			dy8 = dx*(8 - dy);
			dxy8= (8-dx)*(8-dy);

			src -= srcStep<<1;
			dst -= dstStep<<1;

#if 1
#define FILTER_DUB(t0, t1, t2, t3)  (32 + (32<<16) + dxy8*t0 + dy8*t1 + dx8*t2 + dxy*t3 )
			for(j=0; j<height; j+= 2) 
			{
				int tmp, t0, t1, t2, t3, s0, s1, s2;

				src += srcStep<<1;
				dst += dstStep<<1;

				s0 = src[0];		
				s1 = src[0+ srcStep];	
				s2 = src[1];
				tmp = src[1+ srcStep];
				t0  = (s0<<16)|s1;
				t1  = (s2<<16)|tmp;

				s0 = src[0+(srcStep<<1)]; 
				s2 = src[1+(srcStep<<1)];
				
				t2  = (s1<<16)|s0;
				t3  = (tmp<<16)|s2;

				s0 = src[2];		  
				s1 = src[2+ srcStep]; 
				s2 = src[2+(srcStep<<1)]; 

				tmp = FILTER_DUB( t0, t1, t2, t3 );
				dst[0]			= tmp>>22;
				dst[0+dstStep]	= tmp>>6;//***high bits are automaticlly clipped
				
				t0  = (s0<<16)|s1;
				t2  = (s1<<16)|s2;
				tmp = FILTER_DUB( t1, t0, t3, t2 );
				dst[1]			= tmp>>22;
				dst[1+dstStep]	= tmp>>6;

				if( width2 == 1 ) continue;

				s0 = src[3];		  
				s1 = src[3+srcStep]; 
				s2 = src[3+(srcStep<<1)]; 

				t1  = (s0<<16)|s1;
				t3  = (s1<<16)|s2;

				s0 = src[4];		  
				s1 = src[4+srcStep]; 
				s2 = src[4+(srcStep<<1)]; 

				tmp = FILTER_DUB( t0, t1, t2, t3 );
				dst[2]			= tmp>>22;
				dst[2+dstStep]	= tmp>>6;
				
				t0  = (s0<<16)|s1;
				t2  = (s1<<16)|s2;
				tmp = FILTER_DUB( t1, t0, t3, t2 );
				dst[3]			= tmp>>22;
				dst[3+dstStep]	= tmp>>6;

				if( width2 == 2 ) continue;

				s0 = src[5];		  
				s1 = src[5+srcStep]; 
				s2 = src[5+(srcStep<<1)]; 
				t1  = (s0<<16)|s1;
				t3  = (s1<<16)|s2;

				s0 = src[6];		  
				s1 = src[6+srcStep]; 
				s2 = src[6+(srcStep<<1)]; 

				tmp = FILTER_DUB( t0, t1, t2, t3 );
				dst[4]			= tmp>>22;
				dst[4+dstStep]	= tmp>>6;
				
				t0  = (s0<<16)|s1;
				t2  = (s1<<16)|s2;

				s0 = src[7];		  
				s1 = src[7+srcStep]; 
				s2 = src[7+(srcStep<<1)]; 

				tmp = FILTER_DUB( t1, t0, t3, t2 );
				dst[5]			= tmp>>22;
				dst[5+dstStep]	= tmp>>6;

				t1  = (s0<<16)|s1;
				t3  = (s1<<16)|s2;

				s0 = src[8];		  
				s1 = src[8+srcStep]; 
				s2 = src[8+(srcStep<<1)]; 

				tmp = FILTER_DUB( t0, t1, t2, t3 );
				dst[6]			= tmp>>22;
				dst[6+dstStep]	= tmp>>6;
				
				t0  = (s0<<16)|s1;
				t2  = (s1<<16)|s2;
				tmp = FILTER_DUB( t1, t0, t3, t2 );
				dst[7]			= tmp>>22;
				dst[7+dstStep]	= tmp>>6;
#else
#define FILTER_TWO(t0, t1, t2, t3)  ((t0*dxy8 + t1*dy8 + t2*dx8 + t3*dxy + 32)>>6)
			for(j=0; j<height; j+= 2) 
			{
				int t00, t01, t10, t11, t20, t21;

				src += srcStep<<1;
				dst += dstStep<<1;

				t00 = src[0];			
				t10 = src[0+ srcStep];	
				t20 = src[0+(srcStep<<1)]; 
				t01 = src[1];
				t11 = src[1+ srcStep];
				t21 = src[1+(srcStep<<1)];

				dst[0]			= FILTER_TWO(t00, t01, t10, t11);					
				dst[0+dstStep]  = FILTER_TWO(t10, t11, t20, t21);

				t00 = src[2];		  
				t10 = src[2+ srcStep]; 
				t20 = src[2+(srcStep<<1)]; 
				dst[1]			= FILTER_TWO(t01, t00, t11, t10);					
				dst[1+dstStep]  = FILTER_TWO(t11, t10, t21, t20);

				if( width2 == 1 ) continue;

				t01 = src[3];		  
				t11 = src[3+srcStep]; 
				t21 = src[3+(srcStep<<1)]; 
				dst[2]			= FILTER_TWO(t00, t01, t10, t11);
				dst[2+dstStep]  = FILTER_TWO(t10, t11, t20, t21);
				
				t00 = src[4];		  
				t10 = src[4+srcStep]; 
				t20 = src[4+(srcStep<<1)]; 
				dst[3]			= FILTER_TWO(t01, t00, t11, t10);					
				dst[3+dstStep]  = FILTER_TWO(t11, t10, t21, t20);

				if( width2 == 2 ) continue;

				t01 = src[5];		  
				t11 = src[5+srcStep]; 
				t21 = src[5+(srcStep<<1)]; 
				dst[4]			= FILTER_TWO(t00, t01, t10, t11);
				dst[4+dstStep]  = FILTER_TWO(t10, t11, t20, t21);
				
				t00 = src[6];		  
				t10 = src[6+srcStep]; 
				t20 = src[6+(srcStep<<1)]; 
				dst[5]			= FILTER_TWO(t01, t00, t11, t10);					
				dst[5+dstStep]  = FILTER_TWO(t11, t10, t21, t20);

				t01 = src[7];		  
				t11 = src[7+srcStep]; 
				t21 = src[7+(srcStep<<1)]; 
				dst[6]			= FILTER_TWO(t00, t01, t10, t11);
				dst[6+dstStep]  = FILTER_TWO(t10, t11, t20, t21);
				
				t00 = src[8];		  
				t10 = src[8+srcStep]; 
				t20 = src[8+(srcStep<<1)]; 
				dst[7]			= FILTER_TWO(t01, t00, t11, t10);					
				dst[7+dstStep]  = FILTER_TWO(t11, t10, t21, t20);
#endif
			}
		}
	}

	Profile_End(ippiInterpolateChroma_H264_8u_C1R_c_profile);

	return ippStsNoErr;
}


#else
IppStatus  __STDCALL ippiInterpolateChroma_H264_8u_C1R_c(
		const   Ipp8u*   pSrc,
				Ipp32s   srcStep,
				Ipp8u*   pDst,
				Ipp32s   dstStep,
				Ipp32s   dx,
				Ipp32s   dy,
				IppiSize roiSize)
{
	int		width  = roiSize.width;
	int		height = roiSize.height;
	int		i, j;
	unsigned int	tmpvalue0;
	int		dx8, dy8, dxy, dxy8;


	const unsigned char	*psrc= pSrc;
	unsigned char   *pdst= pDst;

	Profile_Start(ippiInterpolateChroma_H264_8u_C1R_c_profile);

	// Please note : roiSize must be 2^n : 8/4/2
	if((dx==0)&&(dy==0))
	{
		for(j=0; j<height; j++)
		{
			memcpy(pdst, psrc, width);
			psrc += srcStep;
			pdst += dstStep;
		}

	}
	else
	{
		if(dy == 0)
		{
			dx8 = 8 - dx;		// dx8 = [1, 7], dx = [1,7]
			for(j=0; j<height; j++)
			{
				for(i=0; i<width; i+=2)
				{
					tmpvalue0 = psrc[i+1]*dx;
					pdst[i]   = (psrc[i]*dx8 + tmpvalue0 + 4) >> 3;
					
					pdst[i+1] = ((psrc[i+1]<<3) - tmpvalue0 + psrc[i+2]*dx + 4) >> 3;
				}
				psrc += srcStep;
				pdst += dstStep;
			}
		}
		else
			if(dx == 0)
			{
				dy8 = 8 - dy;	// dy8=[1,7],  dy=[1,7]
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i+=2)
					{
						pdst[i]   = (psrc[i  ]*dy8 + psrc[i + srcStep  ]*dy + 4) >> 3;
						pdst[i+1] = (psrc[i+1]*dy8 + psrc[i + srcStep+1]*dy + 4) >> 3;

					}
					psrc += srcStep;
					pdst += dstStep;
				}
			}
			else
			{
				dx8 = dy*(8 - dx);
				dxy = dx*dy;
				dy8 = dx*(8 - dy);
				dxy8= (8-dx)*(8-dy);
#if 1
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i+=2)
					{
						pdst[i]   = (psrc[i  ]*dxy8 + psrc[i+1]*dy8 + psrc[i + srcStep  ]*dx8 + psrc[i+1+srcStep]*dxy + 32) >> 6;
						pdst[i+1] = (psrc[i+1]*dxy8 + psrc[i+2]*dy8 + psrc[i + srcStep+1]*dx8 + psrc[i+2+srcStep]*dxy + 32) >> 6;

					}
					psrc += srcStep;
					pdst += dstStep;
				}
#else
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i+=2)
					{
						tmp_res[j][i]   = (psrc[i  ]*dxy8 + psrc[i+1]*dy8 + psrc[i + srcStep  ]*dx8 + psrc[i+1+srcStep]*dxy + 32) >> 6;
						tmp_res[j][i+1] = (psrc[i+1]*dxy8 + psrc[i+2]*dy8 + psrc[i + srcStep+1]*dx8 + psrc[i+2+srcStep]*dxy + 32) >> 6;

					}
					for(i=0; i<width; i+=2)
					{
						pdst[i]   = tmp_res[j][i]  ;
						pdst[i+1] = tmp_res[j][i+1];

					}
					pdst += dstStep;

					psrc += srcStep;
				}
#endif
			}

	}

	Profile_End(ippiInterpolateChroma_H264_8u_C1R_c_profile);

	return ippStsNoErr;
}

#endif

IppStatus  __STDCALL ippiInterpolateChromaTop_H264_8u_C1R_c(
  const Ipp8u*   pSrc,
        Ipp32s   srcStep,
        Ipp8u*   pDst,
        Ipp32s   dstStep,
        Ipp32s   dx,
        Ipp32s   dy,
        Ipp32s   outPixels,
        IppiSize roiSize)
{
	Ipp8u *pTemp;
	__ALIGN4(Ipp8u, TemporalPixels, 32*26);
    //Ipp8u TemporalPixels[32*24];

    pTemp=CopyBlockFromTop((Ipp8u *)pSrc,TemporalPixels+32*4,srcStep,outPixels,dx,dy,roiSize);

	return ippiInterpolateChroma_H264_8u_C1R_c(pTemp,32,pDst,dstStep,dx,dy,roiSize);
}

IppStatus  __STDCALL ippiInterpolateChromaBottom_H264_8u_C1R_c(
  const Ipp8u*   pSrc,
        Ipp32s   srcStep,
        Ipp8u*   pDst,
        Ipp32s   dstStep,
        Ipp32s   dx,
        Ipp32s   dy,
        Ipp32s   outPixels,
        IppiSize roiSize)
{
	Ipp8u *pTemp;
	__ALIGN4(Ipp8u, TemporalPixels, 32*26);
	//Ipp8u TemporalPixels[32*24];

	pTemp=CopyBlockFromBottom((Ipp8u *)pSrc,TemporalPixels+32*4,srcStep,outPixels,dx,dy,roiSize);

	return ippiInterpolateChroma_H264_8u_C1R_c(pTemp,32,pDst,dstStep,dx,dy,roiSize);
}

/*******************************************************************************
 * Function Name : ippiInterpolateBlock_H264_8u_P2P1R_c
 * Description   : Average data in two streams (A + B +1)/2
 * Author        : Wang Wendong
 * Version       : 0.1
 ******************************************************************************/
IppStatus __STDCALL ippiInterpolateBlock_H264_8u_P2P1R_c(
			const Ipp8u *pSrc1,
			const Ipp8u *pSrc2,
			Ipp8u *pDst,
			Ipp32u uWidth,
			Ipp32u uHeight,
			Ipp32u pitch)
{
	unsigned int	j;
	register int	position;
	//register int	pitchdiff = pitch - uWidth;
	//unsigned char *pdst = pDst;

	Profile_Start(ippiInterpolateBlock_H264_8u_P2P1R_c_profile);
	
	position = 0;

	// uWidth and uHeight MUST be 2^n (n>=1)
	if(uWidth == 2)
	{
		for(j=0; j<uHeight; j++)
		{
			pDst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
			pDst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;

			position += pitch;
		}
	}
	else
		if(uWidth == 4)
		{
			for(j=0; j<uHeight; j++)
			{
				pDst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
				pDst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;
				pDst[position+2] = (pSrc1[position+2] + pSrc2[position+2] + 1) >> 1;
				pDst[position+3] = (pSrc1[position+3] + pSrc2[position+3] + 1) >> 1;

				position += pitch;
			}

		}
		else
			if(uWidth == 8)
			{
				for(j=0; j<uHeight; j++)
				{
					pDst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
					pDst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;
					pDst[position+2] = (pSrc1[position+2] + pSrc2[position+2] + 1) >> 1;
					pDst[position+3] = (pSrc1[position+3] + pSrc2[position+3] + 1) >> 1;
					pDst[position+4] = (pSrc1[position+4] + pSrc2[position+4] + 1) >> 1;
					pDst[position+5] = (pSrc1[position+5] + pSrc2[position+5] + 1) >> 1;
					pDst[position+6] = (pSrc1[position+6] + pSrc2[position+6] + 1) >> 1;
					pDst[position+7] = (pSrc1[position+7] + pSrc2[position+7] + 1) >> 1;

					position += pitch;
				}
			}
			else
			{
				// uWidth == 16)
				for(j=0; j<uHeight; j++)
				{
					pDst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
					pDst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;
					pDst[position+2] = (pSrc1[position+2] + pSrc2[position+2] + 1) >> 1;
					pDst[position+3] = (pSrc1[position+3] + pSrc2[position+3] + 1) >> 1;
					pDst[position+4] = (pSrc1[position+4] + pSrc2[position+4] + 1) >> 1;
					pDst[position+5] = (pSrc1[position+5] + pSrc2[position+5] + 1) >> 1;
					pDst[position+6] = (pSrc1[position+6] + pSrc2[position+6] + 1) >> 1;
					pDst[position+7] = (pSrc1[position+7] + pSrc2[position+7] + 1) >> 1;

					pDst[position+ 8] = (pSrc1[position+ 8] + pSrc2[position+ 8] + 1) >> 1;
					pDst[position+ 9] = (pSrc1[position+ 9] + pSrc2[position+ 9] + 1) >> 1;
					pDst[position+10] = (pSrc1[position+10] + pSrc2[position+10] + 1) >> 1;
					pDst[position+11] = (pSrc1[position+11] + pSrc2[position+11] + 1) >> 1;
					pDst[position+12] = (pSrc1[position+12] + pSrc2[position+12] + 1) >> 1;
					pDst[position+13] = (pSrc1[position+13] + pSrc2[position+13] + 1) >> 1;
					pDst[position+14] = (pSrc1[position+14] + pSrc2[position+14] + 1) >> 1;
					pDst[position+15] = (pSrc1[position+15] + pSrc2[position+15] + 1) >> 1;

					position += pitch;
				}
			}

#if 0   // Correct version for all situation
		for(j=0; j<uHeight; j++)
		{

			for(i=0; i<uWidth; i+=2)
			{

				pdst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
				pdst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;

				position += 2;

			}

			position += pitchdiff;

		}
#endif

	Profile_End(ippiInterpolateBlock_H264_8u_P2P1R_c_profile);

    return ippStsNoErr;
}

IppStatus __STDCALL ippiInterpolateBlock_H264_8u_P2P1R_c(
			const Ipp8u *pSrc1,
			const Ipp8u *pSrc2,
			Ipp8u *pDst,
			Ipp32u uWidth,
			Ipp32u uHeight,
			Ipp32s pitch)  // Just one parameters

{
	unsigned int	j;
	register int	position;
	//register int	pitchdiff = pitch - uWidth;
	//unsigned char *pdst = pDst;

	Profile_Start(ippiInterpolateBlock_H264_8u_P2P1R_c_profile);

	position = 0;

	// uWidth and uHeight MUST be 2^n (n>=1)
	if(uWidth == 2)
	{
		for(j=0; j<uHeight; j++)
		{

			pDst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
			pDst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;

			position += pitch;
		}
	}
	else
		if(uWidth == 4)
		{
			for(j=0; j<uHeight; j++)
			{

				pDst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
				pDst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;
				pDst[position+2] = (pSrc1[position+2] + pSrc2[position+2] + 1) >> 1;
				pDst[position+3] = (pSrc1[position+3] + pSrc2[position+3] + 1) >> 1;

				position += pitch;
			}
		}
		else
			if(uWidth == 8)
			{
				for(j=0; j<uHeight; j++)
				{
					pDst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
					pDst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;
					pDst[position+2] = (pSrc1[position+2] + pSrc2[position+2] + 1) >> 1;
					pDst[position+3] = (pSrc1[position+3] + pSrc2[position+3] + 1) >> 1;
					pDst[position+4] = (pSrc1[position+4] + pSrc2[position+4] + 1) >> 1;
					pDst[position+5] = (pSrc1[position+5] + pSrc2[position+5] + 1) >> 1;
					pDst[position+6] = (pSrc1[position+6] + pSrc2[position+6] + 1) >> 1;
					pDst[position+7] = (pSrc1[position+7] + pSrc2[position+7] + 1) >> 1;

					position += pitch;
				}
			}
			else
			{
				// uWidth == 16)
				for(j=0; j<uHeight; j++)
				{

					pDst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
					pDst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;
					pDst[position+2] = (pSrc1[position+2] + pSrc2[position+2] + 1) >> 1;
					pDst[position+3] = (pSrc1[position+3] + pSrc2[position+3] + 1) >> 1;
					pDst[position+4] = (pSrc1[position+4] + pSrc2[position+4] + 1) >> 1;
					pDst[position+5] = (pSrc1[position+5] + pSrc2[position+5] + 1) >> 1;
					pDst[position+6] = (pSrc1[position+6] + pSrc2[position+6] + 1) >> 1;
					pDst[position+7] = (pSrc1[position+7] + pSrc2[position+7] + 1) >> 1;

					pDst[position+ 8] = (pSrc1[position+ 8] + pSrc2[position+ 8] + 1) >> 1;
					pDst[position+ 9] = (pSrc1[position+ 9] + pSrc2[position+ 9] + 1) >> 1;
					pDst[position+10] = (pSrc1[position+10] + pSrc2[position+10] + 1) >> 1;
					pDst[position+11] = (pSrc1[position+11] + pSrc2[position+11] + 1) >> 1;
					pDst[position+12] = (pSrc1[position+12] + pSrc2[position+12] + 1) >> 1;
					pDst[position+13] = (pSrc1[position+13] + pSrc2[position+13] + 1) >> 1;
					pDst[position+14] = (pSrc1[position+14] + pSrc2[position+14] + 1) >> 1;
					pDst[position+15] = (pSrc1[position+15] + pSrc2[position+15] + 1) >> 1;

					position += pitch;
				}
			}

#if 0   // Correct version for all situation
		for(j=0; j<uHeight; j++)
		{

			for(i=0; i<uWidth; i+=2)
			{

				pdst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
				pdst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;

				position += 2;

			}

			position += pitchdiff;

		}
#endif

	Profile_End(ippiInterpolateBlock_H264_8u_P2P1R_c_profile);

    return ippStsNoErr;
}

/************************************************************************************
 * For we can not find any description for funtion ippiInterpolateBlock_H264_8u_P3P1R_x
 *   So, I do it just according to the name of its parameteters
 ***********************************************************************************
 */
#if 1

#define AVERAGE_4(base)						\
		t0 = src1[base+0]; t1 = src2[base+0];	\
		t2 = src1[base+1]; t3 = src2[base+1];	\
		t5 = (t0 + t1 + 1) >> 1;				\
		t0 = src1[base+2]; t1 = src2[base+2];	\
		t4 = (t2 + t3 + 1) >> 1; t5 |= t4<<8;	\
		t2 = src1[base+3]; t3 = src2[base+3];	\
		t4 = (t0 + t1 + 1) >> 1; t5 |= t4<<16;	\
		t4 = (t2 + t3 + 1) >> 1; t5 |= t4<<24;	\
		*((long *)(dst+base)) = t5;

#define AVERAGE_2(base)							\
		t0 = src1[base+0]; t1 = src2[base+0];	\
		t2 = src1[base+1]; t3 = src2[base+1];	\
		t5 = (t0 + t1 + 1) >> 1;				\
		t4 = (t2 + t3 + 1) >> 1;				\
		t5 |= t4<<8;							\
		*((short *)(dst+base)) = t5;


IppStatus __STDCALL ippiInterpolateBlock_H264_8u_P3P1R_c
(
	const Ipp8u *src1,
	const Ipp8u *src2,
	Ipp8u *dst,
	Ipp32u width,
	Ipp32u height,
	Ipp32s srcStep1,
	Ipp32s srcStep2,
	Ipp32s dstStep
)
{
	unsigned int j;

	Profile_Start(ippiInterpolateBlock_H264_8u_P3P1R_c_profile);
	
	src1 -= srcStep1;
	src2 -= srcStep2;
	dst  -= dstStep;
	for(j=0; j<height; j++)
	{
		int t0, t1, t2, t3, t4, t5;

		src1 += srcStep1;
		src2 += srcStep2;
		dst  += dstStep;

		AVERAGE_2(0)		
		if( width == 2 ) continue;

		AVERAGE_2(2)		

		if( width == 4 ) continue;
	
		AVERAGE_4(4)

		if( width == 8 ) continue;

		AVERAGE_4(8)
		AVERAGE_4(12)
	}
	
	Profile_End(ippiInterpolateBlock_H264_8u_P3P1R_c_profile);
 
	return ippStsNoErr;

}
#else
IppStatus __STDCALL ippiInterpolateBlock_H264_8u_P3P1R_c(Ipp8u *pSrc1,
                                                       Ipp8u *pSrc2,
                                                       Ipp8u *pDst,
                                                       Ipp32u uWidth,
                                                       Ipp32u uHeight,
                                                       Ipp32s iPitchSrc1,
                                                       Ipp32s iPitchSrc2,
                                                       Ipp32s iPitchDst)
{
	unsigned int	i, j;
	register int	positionSrc1=0, positionSrc2=0, positionDst=0;

	Profile_Start(ippiInterpolateBlock_H264_8u_P3P1R_c_profile);

#if 0
	{
		static int w_count_2,  h_count_2;
		static int w_count_4,  h_count_4;
		static int w_count_8,  h_count_8;
		static int w_count_16, h_count_16;
		
		if( uWidth == 2 )  w_count_2++;
		if( uWidth == 4 )  w_count_4++;
		if( uWidth == 8 )  w_count_8++;
		if( uWidth == 16 ) w_count_16++;

		if( uHeight == 2 )  h_count_2++;
		if( uHeight == 4 )  h_count_4++;
		if( uHeight == 8 )  h_count_8++;
		if( uHeight == 16 ) h_count_16++;

		fprintf( stderr, "w_2=%d, w_4=%d, w_8=%d, w_16=%d \n", w_count_2, w_count_4, w_count_8, w_count_16 );
		fprintf( stderr, "h_2=%d, h_4=%d, h_8=%d, h_16=%d \n\n", h_count_2, h_count_4, h_count_8, h_count_16 );

	}
#endif

	for(j=0; j<uHeight; j++)
	{
		for(i=0; i<uWidth; i++)
		{
			pDst[positionDst+i] = (pSrc1[positionSrc1+i] + pSrc2[positionSrc2+i] + 1 ) >> 1;
		}

		positionDst  += iPitchDst;
		positionSrc1 += iPitchSrc1;
		positionSrc2 += iPitchSrc2;
	}

	Profile_End(ippiInterpolateBlock_H264_8u_P3P1R_c_profile);
 
	return ippStsNoErr;

}

#endif
/***************************************************************************
 *
 *
 ***************************************************************************
 */
IppStatus __STDCALL ippiUniDirWeightBlock_H264_8u_C1R_MSc(
        Ipp8u *pSrcDst,
        Ipp32u pitch,
        Ipp32u ulog2wd,
        Ipp32s iWeight,
        Ipp32s iOffset,
        IppiSize roi
        )
{
        Ipp32u uRound;
        Ipp32u xpos, ypos;
        Ipp32s weighted_sample;

		Ipp32u uWidth = roi.width;
        Ipp32u uHeight= roi.height;

	Profile_Start(ippiUniDirWeightBlock_H264_8u_C1R_c_profile);

        if (ulog2wd > 0)
            uRound = 1<<(ulog2wd - 1);
        else
            uRound = 0;

        for (ypos=0; ypos<uHeight; ypos++)
        {
            for (xpos=0; xpos<uWidth; xpos++)
            {
                weighted_sample = (((Ipp32s)pSrcDst[xpos]*iWeight + (Ipp32s)uRound)>>ulog2wd) + iOffset;
                // clamp to 0..255. May be able to use ClampVal table for this,
                // if range of unclamped weighted_sample can be guaranteed not
                // to exceed table bounds.
                if (weighted_sample > 255) weighted_sample = 255;
                if (weighted_sample < 0) weighted_sample = 0;
                pSrcDst[xpos] = (Ipp8u)weighted_sample;
            }
            pSrcDst += pitch;
        }

	Profile_End(ippiUniDirWeightBlock_H264_8u_C1R_c_profile);

	return ippStsNoErr;
}

IppStatus __STDCALL ippiBiDirWeightBlock_H264_8u_P2P1R_MSc(
        Ipp8u *pSrc1,
        Ipp8u *pSrc2,
        Ipp8u *pDst,// may be same as pSrc1 or pSrc2
        Ipp32u pitch,// of pSrc1, pSrc2, pDst
        Ipp32u dstStep,
        Ipp32u ulog2wd,
        Ipp32s iWeight1,
        Ipp32s iOffset1,
        Ipp32s iWeight2,
        Ipp32s iOffset2,
        IppiSize roi
        )
{
        Ipp32u uRound;
        Ipp32u xpos, ypos;
        Ipp32s weighted_sample;

		Ipp32u uWidth = roi.width;
        Ipp32u uHeight= roi.height;

	Profile_Start(ippiBiDirWeightBlock_H264_8u_P2P1R_c_profile);

        uRound = 1<<ulog2wd;

        for (ypos=0; ypos<uHeight; ypos++)
        {
            for (xpos=0; xpos<uWidth; xpos++)
            {
                weighted_sample = ((((Ipp32s)pSrc1[xpos]*iWeight1 +
                    (Ipp32s)pSrc2[xpos]*iWeight2 + (Ipp32s)uRound)>>(ulog2wd+1))) +
                    ((iOffset1 + iOffset2 + 1)>>1);
                // clamp to 0..255. May be able to use ClampVal table for this,
                // if range of unclamped weighted_sample can be guaranteed not
                // to exceed table bounds.
                if (weighted_sample > 255) weighted_sample = 255;
                if (weighted_sample < 0) weighted_sample = 0;
                pDst[xpos] = (Ipp8u)weighted_sample;
            }
            pSrc1 += pitch;
            pSrc2 += pitch;
            pDst += pitch;
        }


	Profile_End(ippiBiDirWeightBlock_H264_8u_P2P1R_c_profile);

	return ippStsNoErr;
}

IppStatus __STDCALL ippiBiDirWeightBlock_H264_8u_P3P1R_MSc( 
	   const Ipp8u *pSrc1,
       const Ipp8u *pSrc2,
       Ipp8u *pDst,
       Ipp32u nSrcPitch1,
       Ipp32u nSrcPitch2,
       Ipp32u nDstPitch,
       Ipp32u ulog2wd,
       Ipp32s iWeight1,
       Ipp32s iOffset1,
       Ipp32s iWeight2,
       Ipp32s iOffset2,
       IppiSize roi
    )
{
        Ipp32u uRound;
        Ipp32u xpos, ypos;
        Ipp32s weighted_sample;

		Ipp32u uWidth = roi.width;
        Ipp32u uHeight= roi.height;

	Profile_Start(ippiBiDirWeightBlock_H264_8u_P3P1R_c_profile);

        uRound = 1<<ulog2wd;

        for (ypos=0; ypos<uHeight; ypos++)
        {
            for (xpos=0; xpos<uWidth; xpos++)
            {
                weighted_sample = ((((Ipp32s)pSrc1[xpos]*iWeight1 +
                    (Ipp32s)pSrc2[xpos]*iWeight2 + (Ipp32s)uRound)>>(ulog2wd+1))) +
                    ((iOffset1 + iOffset2 + 1)>>1);
                // clamp to 0..255. May be able to use ClampVal table for this,
                // if range of unclamped weighted_sample can be guaranteed not
                // to exceed table bounds.
                if (weighted_sample > 255) weighted_sample = 255;
                if (weighted_sample < 0) weighted_sample = 0;
                pDst[xpos] = (Ipp8u)weighted_sample;
            }
            pSrc1 += nSrcPitch1;
            pSrc2 += nSrcPitch2;
            pDst += nDstPitch;
        }

	Profile_End(ippiBiDirWeightBlock_H264_8u_P3P1R_c_profile);

		return ippStsNoErr;
}

/***************************************************************************
 * For we can not find any description for funtion ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_x
 *   So, I do it just according to the name of its parameteters *
 *
 *
 ***************************************************************************
 */
IppStatus __STDCALL ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_c(
	   const Ipp8u *pSrc1,
       const Ipp8u *pSrc2,
       Ipp8u *pDst,
       Ipp32u nSrcPitch1,
       Ipp32u nSrcPitch2,
       Ipp32u nDstPitch,
       Ipp32s iWeight1,
       Ipp32s iWeight2,
       IppiSize roi
    )
{
        Ipp32u xpos, ypos;
        Ipp32s weighted_sample;

		Ipp32u uWidth = roi.width;
        Ipp32u uHeight= roi.height;

	Profile_Start(ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_c_profile);

        for (ypos=0; ypos<uHeight; ypos++)
        {
            for (xpos=0; xpos<uWidth; xpos++)
            {
                weighted_sample = ((Ipp32s)pSrc1[xpos]*iWeight1 +
                    (Ipp32s)pSrc2[xpos]*iWeight2 + 32) >> 6;
                // clamp to 0..255. May be able to use ClampVal table for this,
                // if range of unclamped weighted_sample can be guaranteed not
                // to exceed table bounds.
                if (weighted_sample > 255) weighted_sample = 255;
                if (weighted_sample < 0) weighted_sample = 0;
                pDst[xpos] = (Ipp8u)weighted_sample;
            }
            pSrc1 += nSrcPitch1;
            pSrc2 += nSrcPitch2;
            pDst += nDstPitch;
        }

	Profile_End(ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_c_profile);

	return ippStsNoErr;
}

IppStatus __STDCALL ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_c(
        Ipp8u *pSrc1,
        Ipp8u *pSrc2,
        Ipp8u *pDst,
        Ipp32u pitch,// of pSrc1, pSrc2, pDst
        Ipp32u dstpitch,// of pDst
        Ipp32s iWeight1,
        Ipp32s iWeight2,
        IppiSize roi
        )
{
        Ipp32u xpos, ypos;
        Ipp32s weighted_sample;

		Ipp32u uWidth = roi.width;
        Ipp32u uHeight= roi.height;

	Profile_Start(ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_c_profile);

        for (ypos=0; ypos<uHeight; ypos++)
        {
            for (xpos=0; xpos<uWidth; xpos++)
            {
                weighted_sample = ((Ipp32s)pSrc1[xpos]*iWeight1 +
                    (Ipp32s)pSrc2[xpos]*iWeight2 + 32) >> 6;
                // clamp to 0..255. May be able to use ClampVal table for this,
                // if range of unclamped weighted_sample can be guaranteed not
                // to exceed table bounds.
                if (weighted_sample > 255) weighted_sample = 255;
                if (weighted_sample < 0) weighted_sample = 0;
                pDst[xpos] = (Ipp8u)weighted_sample;
            }
            pSrc1 += pitch;
            pSrc2 += pitch;
            pDst += dstpitch;
        }

	Profile_End(ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_c_profile);

	return ippStsNoErr;
}


void Init_AVC_Interpolation_C()
{
	//common
	ippiInterpolateChroma_H264_8u_C1R_universal			= ippiInterpolateChroma_H264_8u_C1R_c;
	
	ippiInterpolateBlock_H264_8u_P2P1R_universal		= ippiInterpolateBlock_H264_8u_P2P1R_c;
	ippiInterpolateBlock_H264_8u_P3P1R_universal		= ippiInterpolateBlock_H264_8u_P3P1R_c;	//***
	ippiInterpolateLumaTop_H264_8u_C1R_universal		= ippiInterpolateLumaTop_H264_8u_C1R_c;
	ippiInterpolateLumaBottom_H264_8u_C1R_universal		= ippiInterpolateLumaBottom_H264_8u_C1R_c;
	ippiInterpolateChromaTop_H264_8u_C1R_universal		= ippiInterpolateChromaTop_H264_8u_C1R_c;							
	ippiInterpolateChromaBottom_H264_8u_C1R_universal	= ippiInterpolateChromaBottom_H264_8u_C1R_c;

	//can be replaced by optimized versions
#ifndef DROP_C_NO
	ippiInterpolateLuma_H264_8u_C1R_universal			= ippiInterpolateLuma_H264_8u_C1R_c;
	ippiInterpolateLuma_H264_8u_C1R_16x16_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_16x8_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_8x16_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_8x8_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_8x4_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_4x8_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_4x4_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
#endif
}


#ifdef __INTEL_IPP__
void Init_AVC_Interpolation_XScale()
{
	//common
	ippiInterpolateChroma_H264_8u_C1R_universal			= ippiInterpolateChroma_H264_8u_C1R;
	
	ippiInterpolateBlock_H264_8u_P2P1R_universal		= ippiInterpolateBlock_H264_8u_P2P1R;
	ippiInterpolateBlock_H264_8u_P3P1R_universal		= ippiInterpolateBlock_H264_8u_P3P1R;	//***
	ippiInterpolateLumaTop_H264_8u_C1R_universal		= ippiInterpolateLumaTop_H264_8u_C1R;
	ippiInterpolateLumaBottom_H264_8u_C1R_universal		= ippiInterpolateLumaBottom_H264_8u_C1R;
	ippiInterpolateChromaTop_H264_8u_C1R_universal		= ippiInterpolateChromaTop_H264_8u_C1R;							
	ippiInterpolateChromaBottom_H264_8u_C1R_universal	= ippiInterpolateChromaBottom_H264_8u_C1R;

	//can be replaced by optimized versions
#ifndef DROP_C_NO
	ippiInterpolateLuma_H264_8u_C1R_universal			= ippiInterpolateLuma_H264_8u_C1R;
	ippiInterpolateLuma_H264_8u_C1R_16x16_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_16x8_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_8x16_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_8x8_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_8x4_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_4x8_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_4x4_universal		= ippiInterpolateLuma_H264_8u_C1R_universal;
#endif
}
#endif


void Init_AVC_Interpolation_ARM_V5()
{
#if defined(__KINOMA_IPP_ARM_V5__) && TARGET_CPU_ARM
	ippiInterpolateLuma_H264_8u_C1R_universal		= ippiInterpolateLuma_H264_8u_C1R_arm;
	ippiInterpolateLuma_H264_8u_C1R_16x16_universal = ippiInterpolateLuma_H264_8u_C1R_16x16_arm;
	ippiInterpolateLuma_H264_8u_C1R_16x8_universal  = ippiInterpolateLuma_H264_8u_C1R_arm;
	ippiInterpolateLuma_H264_8u_C1R_8x16_universal  = ippiInterpolateLuma_H264_8u_C1R_8x16_arm;
	ippiInterpolateLuma_H264_8u_C1R_8x8_universal   = ippiInterpolateLuma_H264_8u_C1R_arm;
	ippiInterpolateLuma_H264_8u_C1R_8x4_universal   = ippiInterpolateLuma_H264_8u_C1R_arm;
	ippiInterpolateLuma_H264_8u_C1R_4x8_universal   = ippiInterpolateLuma_H264_8u_C1R_arm;
	ippiInterpolateLuma_H264_8u_C1R_4x4_universal   = ippiInterpolateLuma_H264_8u_C1R_arm;
	//ippiInterpolateLuma_H264_8u_C1R_16x16_universal = ippiInterpolateLuma_H264_8u_C1R_bilinear_approx_c;
	//ippiInterpolateLuma_H264_8u_C1R_16x16_universal = ippiInterpolateLuma_H264_8u_C1R_c;
	//ippiInterpolateLuma_H264_8u_C1R_8x16_universal  = ippiInterpolateLuma_H264_8u_C1R_sub_arm_profile;
	//ippiInterpolateLuma_H264_8u_C1R_16x16_universal = ippiInterpolateLuma_H264_8u_C1R_arm;
	//ippiInterpolateLuma_H264_8u_C1R_16x16_universal = ippiInterpolateLuma_H264_8u_C1R_sub_arm_profile;
	//ippiInterpolateLuma_H264_8u_C1R_16x8_universal  = ippiInterpolateLuma_H264_8u_C1R_c;
#endif
}

#endif

#if 0
/********************************************************************
 * Function Name : ippiInterpolateLuma_H264_8u_C1R_x
 * Version       : Original version !!!!!!!!-- Do not remove, 
 *                 but do not use it, please use optimizaed version
 * Author        : Wendong
 * Date          : 06-10-2006
 *
 ********************************************************************
 */
IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_x(
	  const Ipp8u*   pSrc,
			Ipp32s   srcStep,
			Ipp8u*   pDst,
			Ipp32s   dstStep,
			Ipp32s   dx,
			Ipp32s   dy,
			IppiSize roiSize)
{
	int		width  = roiSize.width;
	int		height = roiSize.height;
	int		i, j, k;

	const unsigned char	*psrc;
	unsigned char   *pdst;

	int		temp_result;
	int		temp_res[25][25];

	int		dxx = dx>> 1;
	int		dyy = dy>> 1;

	if((dx==0)&&(dy==0))
	{
		for(j=0; j<height; j++)
		{
			psrc = pSrc + j*srcStep;
			pdst = pDst + j*dstStep;

			for(i=0; i<width; i++)
			{

				pdst[i] = psrc[i];
			}
		}
	}
	else
	{
		if(dy == 0)
		{
			for(j=0; j<height; j++)
			{
				psrc = pSrc + j*srcStep;
				pdst = pDst + j*dstStep;

				for(i=0; i<width; i++)
				{
					temp_result =0;

					for(k=0; k<6; k++)
					{

						temp_result += psrc[i -2 + k] * COEF[k];
					}
						
					pdst[i] = IClip(0, 255, (temp_result + 16) >> 5);
				}
			}
			// If dx ==2 ,just break

			if(dx & 1)				// For dx=1. or 3
			{
				for(j=0; j<height; j++)
				{
					psrc = pSrc + j*srcStep;
					pdst = pDst + j*dstStep;

					for(i=0; i<width; i++)
					{			
						pdst[i] = IClip(0, 255, (pdst[i] + psrc[i+dxx] + 1) >> 1);
					}
				}
			}

		}
		else if(dx ==0)
		{
			for(j=0; j<height; j++)
			{
				psrc = pSrc + j*srcStep;
				pdst = pDst + j*dstStep;

				for(i=0; i<width; i++)
				{
					temp_result =0;

					for(k=0; k<6; k++)
					{

						temp_result += psrc[i+ (-2 + k)*srcStep] * COEF[k];
					}
						
					pdst[i] = IClip(0, 255, (temp_result + 16) >> 5);
				}
			}
			// If dx ==2 ,just break

			if(dy & 1)				// For dx=1. or 3
			{
				for(j=0; j<height; j++)
				{
					psrc = pSrc + j*srcStep;
					pdst = pDst + j*dstStep;

					for(i=0; i<width; i++)
					{			
						pdst[i] = IClip(0, 255, (pdst[i] + psrc[i+dyy*srcStep] + 1) >> 1);
					}
				}
			}
		}
		else if(dx ==2)
		{
			// Prepair data in H filter: Compute 1/2 position for H 
			for(j=0; j<(height+6); j++)
			{
				psrc = pSrc + (j-2)*srcStep;

				for(i=0; i<width; i++)
				{
					temp_result =0;

					for(k=0; k<6; k++)
					{

						temp_result += psrc[i -2 + k] * COEF[k];
					}
						
					//temp_res[j][i] = IClip(0, 255, (temp_result + 16) >> 5);
					temp_res[j][i] = temp_result;

				}
			}

			// do V filter: compute 1/2 position for V -- But just in position (2,2) ONLY, no vertical 1/2 position was computed based on integer vertical position
			for(j=0; j<height; j++)
			{
				pdst = pDst + j*dstStep;

				for(i=0; i<width; i++)
				{
					temp_result =0;

					for(k=0; k<6; k++)
					{

						temp_result += temp_res[j + k][i] * COEF[k];
					}
						
					pdst[i] = IClip(0, 255, (temp_result + 512) >> 10);
				}
			}
			// If dy ==2 ,just break

			if(dy & 1)				// For dy=1. or 3
			{
				for(j=0; j<height; j++)
				{
					//psrc = pSrc + j*srcStep;
					pdst = pDst + j*dstStep;

					for(i=0; i<width; i++)
					{			
						pdst[i] = (pdst[i] + IClip(0, 255, ((temp_res[j+dyy+2][i]+16)>>5)) + 1) >> 1;
					}
				}
			}

		}
		else if(dy ==2)
		{
			// Prepair data in V filter
			for(j=0; j<height; j++)
			{
				//psrc = pSrc + (j)*srcStep;

				for(i=0; i<width+6; i++)
				{
					psrc = pSrc + j*srcStep + i - 2;

					temp_result =0;

					for(k=0; k<6; k++)
					{

						temp_result += psrc[(k-2)*srcStep] * COEF[k];
					}
						
					temp_res[j][i] = temp_result;
				}
			}

			// do H filter
			for(j=0; j<height; j++)
			{
				pdst = pDst + j*dstStep;

				for(i=0; i<width; i++)
				{
					temp_result =0;

					for(k=0; k<6; k++)
					{

						temp_result += temp_res[j][i + k] * COEF[k];
					}
						
					pdst[i] = IClip(0, 255, (temp_result + 512) >> 10);
				}
			}
			// If dx ==2 ,just break

			if(dx & 1)				// For dx=1. or 3
			{
				for(j=0; j<height; j++)
				{
					pdst = pDst + j*dstStep;

					for(i=0; i<width; i++)
					{			
						pdst[i] = (pdst[i] + IClip(0, 255, ((temp_res[j][i+dxx+2]+16)>>5)) + 1) >> 1;
					}
				}
			}
		}
		else
		{
			/* Algorithm 
			 *           G      b       H
			 *           d   e  f   g   
			 *           h   i  J   k   m
			 *           n   p  q   r
			 *           M      s       N
			 *
			 *
			*/
			// 以下的算法首先计算b/s，但是根据dyy则可能b不需要计算
			 /* Diagonal interpolation */
			// 这里需要进一步区分e,g  与 p, r （Spec）
			for (j = 0; j < height; j++) 
			{
				psrc = pSrc + (j+dyy)*srcStep;

				for (i = 0; i < width; i++) 
				{
					temp_result = 0;

					for (k = 0; k < 6; k++)
						temp_result += psrc[i + k-2] * COEF[k];

					temp_res[j][i] = IClip(0, 255,(temp_result + 16)>>5);
				}
			}

			// 以下计算h/m，但是根据dxx则可能h不需要计算
			for (j = 0; j < height; j++) 
			{
				pdst = pDst + j*dstStep;

				for (i = 0; i < width; i++) 
				{

					psrc = pSrc + (j)*srcStep + dxx + i;

					temp_result =0;

					for (k = 0; k < 6; k++)
						temp_result += psrc[(k-2)*srcStep] * COEF[k];

					pdst[i] = (temp_res[j][i] + IClip(0, 255,(temp_result + 16)>>5) + 1) >> 1;

				}
			}

		}
	}



    return ippStsNoErr;
}
#endif
