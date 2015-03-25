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
//#include "Fsk.h"
#include "stdlib.h"
#include "stdio.h"

#include "omxtypes.h"
#include "armOMX.h"
#include "omxVC.h"

#include "armCOMM.h"
#include "armVC.h"

#include "kinoma_ipp_lib.h"
#include "kinoma_utilities.h"
#include "ippi.h"
#include "ippac.h"
#include "kinoma_ipp_common.h"
#include "FskPlatformImplementation.h"


#define ippiDequantTransformResidualAndAdd_H264_16s_C1I_NO_AC ippiDequantTransformResidualAndAdd_H264_16s_C1I_NO_AC_c

#ifdef FORCE_C_IMPLEMENTATION

#define armVCM4P10_DequantLumaAC4x4 DequantLumaAC4x4 

/*
 * Description:
 * Dequantize Luma AC block
 */
extern void DequantLumaAC4x4
(
     OMX_S16* pSrcDst,
     OMX_INT QP        
);

IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_openmax
(
  const Ipp8u*   pSrc,
		Ipp32s   srcStep,
		Ipp8u*   pDst,
		Ipp32s   dstStep,
		Ipp32s   dx,
		Ipp32s   dy,
		IppiSize roi   // Must be 16,8 or 4
)
{
	return (IppStatus)armVCM4P10_Interpolate_Luma
			(
				pSrc,
				srcStep,
				pDst,
				dstStep,
				roi.width,
				roi.height,
				dx,
				dy
			);
}

IppStatus __STDCALL ippiInterpolateChroma_H264_8u_C1R_openmax
(
  const Ipp8u*   pSrc,
		Ipp32s   srcStep,
		Ipp8u*   pDst,
		Ipp32s   dstStep,
		Ipp32s   dx,
		Ipp32s   dy,
		IppiSize roi   // Must be 16,8 or 4
)
{
	return (IppStatus)armVCM4P10_Interpolate_Chroma
			(
				pSrc,
				srcStep,
				pDst,
				dstStep,
				roi.width,
				roi.height,
				dx,
				dy
			);
}


IppStatus __STDCALL ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_openmax
(	
	Ipp8u *pSrcDst,	
	Ipp32s srcdstStep,	
	Ipp8u *pAlpha,	
	Ipp8u *pBeta,	
	Ipp8u *pThresholds,	
	Ipp8u *pBs
)
{
	return (IppStatus)omxVCM4P10_FilterDeblockingLuma_HorEdge_I 
			(
				pSrcDst,
				srcdstStep,
				pAlpha,
				pBeta,
				pThresholds,
				pBs
			);
}

IppStatus __STDCALL ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_openmax
(	
	Ipp8u *pSrcDst,	
	Ipp32s srcdstStep,	
	Ipp8u *pAlpha,	
	Ipp8u *pBeta,	
	Ipp8u *pThresholds,	
	Ipp8u *pBs
)
{
	return (IppStatus)omxVCM4P10_FilterDeblockingLuma_VerEdge_I 
			(
				pSrcDst,
				srcdstStep,
				pAlpha,
				pBeta,
				pThresholds,
				pBs
			);
}


#else

#ifdef TARGET_CPU_ARM

OMXResult omxVCM4P10_InterpolateLuma_8_bytes_alignment_safe (
     const OMX_U8* pSrc,
     OMX_S32 srcStep,
     OMX_U8* pDst,
     OMX_S32 dstStep,
     OMX_S32 dx,
     OMX_S32 dy,
     OMXSize roi
 );


IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_openmax
(
	const Ipp8u*   pSrc, 
	Ipp32s   srcStep,
	Ipp8u*   pDst,
	Ipp32s   dstStep,
	Ipp32s   dx,	
	Ipp32s   dy, 
	IppiSize roi
)
{
	OMXSize om_roi;

	om_roi.height = roi.height;
	om_roi.width  = roi.width;

	return (IppStatus)omxVCM4P10_InterpolateLuma_8_bytes_alignment_safe
        (
			(OMX_U8*)pSrc, 
			srcStep, 
			pDst, 
			dstStep, 
			dx, 
			dy,
			om_roi
		);
}


IppStatus __STDCALL ippiInterpolateChroma_H264_8u_C1R_openmax
(
  const Ipp8u*   pSrc,
		Ipp32s   srcStep,
		Ipp8u*   pDst,
		Ipp32s   dstStep,
		Ipp32s   dx,
		Ipp32s   dy,
		IppiSize roi 
 )
{
	return (IppStatus)armVCM4P10_Interpolate_Chroma
        (
			(OMX_U8*)pSrc, 
			srcStep, 
			pDst, 
			dstStep, 
			roi.width, 
			roi.height, 
			dx, 
			dy
		);

}

IppStatus __STDCALL ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_openmax
(	
	Ipp8u *pSrcDst,	
	Ipp32s srcdstStep,	
	Ipp8u *pAlpha,	
	Ipp8u *pBeta,	
	Ipp8u *pThresholds,	
	Ipp8u *pBs
)
{
	return (IppStatus)omxVCM4P10_FilterDeblockingLuma_HorEdge_I_8_bytes_alignment_safe 
			(
				pSrcDst,
				srcdstStep,
				pAlpha,
				pBeta,
				pThresholds,
				pBs
			);
}

IppStatus __STDCALL ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_openmax
(	
	Ipp8u *pSrcDst,	
	Ipp32s srcdstStep,	
	Ipp8u *pAlpha,	
	Ipp8u *pBeta,	
	Ipp8u *pThresholds,	
	Ipp8u *pBs
)
{
	return (IppStatus)omxVCM4P10_FilterDeblockingLuma_VerEdge_I_8_bytes_alignment_safe 
			(
				pSrcDst,
				srcdstStep,
				pAlpha,
				pBeta,
				pThresholds,
				pBs
			);
}

#endif
#endif


#ifdef FORCE_C_IMPLEMENTATION

OMXResult ADD_CLIP_GENERIC( short  *s, unsigned char *p, unsigned char *d, int p_pitch, int d_pitch )
{
    int t, tmp;
 	int const_255 = 255;

	tmp	= p[0] + s[ 0]; Clip1( const_255, tmp ); t  = tmp<<0;
	tmp	= p[1] + s[ 1]; Clip1( const_255, tmp ); t |= tmp<<8;
	tmp	= p[2] + s[ 2]; Clip1( const_255, tmp ); t |= tmp<<16;
	tmp	= p[3] + s[ 3]; Clip1( const_255, tmp ); t |= tmp<<24;	*((long *)d) = t; d += d_pitch; p+= p_pitch; 

	tmp	= p[0] + s[ 4]; Clip1( const_255, tmp ); t  = tmp<<0;
	tmp	= p[1] + s[ 5]; Clip1( const_255, tmp ); t |= tmp<<8;
	tmp	= p[2] + s[ 6]; Clip1( const_255, tmp ); t |= tmp<<16;
	tmp	= p[3] + s[ 7]; Clip1( const_255, tmp ); t |= tmp<<24;	*((long *)d) = t; d += d_pitch; p+= p_pitch;
	
	tmp	= p[0] + s[ 8]; Clip1( const_255, tmp ); t  = tmp<<0;
	tmp	= p[1] + s[ 9]; Clip1( const_255, tmp ); t |= tmp<<8;
	tmp	= p[2] + s[10]; Clip1( const_255, tmp ); t |= tmp<<16;
	tmp	= p[3] + s[11]; Clip1( const_255, tmp ); t |= tmp<<24;	*((long *)d) = t; d += d_pitch; p+= p_pitch;
	
	tmp	= p[0] + s[12]; Clip1( const_255, tmp ); t  = tmp<<0;
	tmp	= p[1] + s[13]; Clip1( const_255, tmp ); t |= tmp<<8;
	tmp	= p[2] + s[14]; Clip1( const_255, tmp ); t |= tmp<<16;
	tmp	= p[3] + s[15]; Clip1( const_255, tmp ); t |= tmp<<24;	*((long *)d) = t;

    return OMX_Sts_NoErr;
}


void ADD_CLIP_CHROMA_INTER( unsigned char *s, int   pp,  int pitch )
{
	int t, tmp;
	int const_255 = 255;

	tmp = s[0] + pp; Clip1( const_255, tmp ); t  = tmp<<0;
	tmp = s[1] + pp; Clip1( const_255, tmp ); t |= tmp<<8;
	tmp = s[2] + pp; Clip1( const_255, tmp ); t |= tmp<<16;
	tmp = s[3] + pp; Clip1( const_255, tmp ); t |= tmp<<24;	*((long *)s) = t; s += pitch;	
				 
	tmp = s[0] + pp; Clip1( const_255, tmp ); t  = tmp<<0;
	tmp = s[1] + pp; Clip1( const_255, tmp ); t |= tmp<<8;
	tmp = s[2] + pp; Clip1( const_255, tmp ); t |= tmp<<16;
	tmp = s[3] + pp; Clip1( const_255, tmp ); t |= tmp<<24;	*((long *)s) = t; s += pitch;	
				 
	tmp = s[0] + pp; Clip1( const_255, tmp ); t  = tmp<<0;
	tmp = s[1] + pp; Clip1( const_255, tmp ); t |= tmp<<8;
	tmp = s[2] + pp; Clip1( const_255, tmp ); t |= tmp<<16;
	tmp = s[3] + pp; Clip1( const_255, tmp ); t |= tmp<<24;	*((long *)s) = t; s += pitch;	
				 
	tmp = s[0] + pp; Clip1( const_255, tmp ); t  = tmp<<0;
	tmp = s[1] + pp; Clip1( const_255, tmp ); t |= tmp<<8;
	tmp = s[2] + pp; Clip1( const_255, tmp ); t |= tmp<<16;
	tmp = s[3] + pp; Clip1( const_255, tmp ); t |= tmp<<24;	*((long *)s) = t;
}


void ADD_CLIP_LUMA_INTRA(   unsigned char *s, int   pp,  unsigned char *d, int pitch )
{
	int t, tmp;
	int const_255 = 255;

	tmp = s[0] + pp; Clip1(const_255, tmp ); t  = tmp<<0;
	tmp = s[1] + pp; Clip1(const_255, tmp ); t |= tmp<<8;
	tmp = s[2] + pp; Clip1(const_255, tmp ); t |= tmp<<16;
	tmp = s[3] + pp; Clip1(const_255, tmp ); t |= tmp<<24;	*((long *)d) = t; d += pitch; s += 16; 
									   
	tmp = s[0] + pp; Clip1(const_255, tmp ); t  = tmp<<0;
	tmp = s[1] + pp; Clip1(const_255, tmp ); t |= tmp<<8;
	tmp = s[2] + pp; Clip1(const_255, tmp ); t |= tmp<<16;
	tmp = s[3] + pp; Clip1(const_255, tmp ); t |= tmp<<24;	*((long *)d) = t; d += pitch; s += 16;
									   
	tmp = s[0] + pp; Clip1(const_255, tmp ); t  = tmp<<0;
	tmp = s[1] + pp; Clip1(const_255, tmp ); t |= tmp<<8;
	tmp = s[2] + pp; Clip1(const_255, tmp ); t |= tmp<<16;
	tmp = s[3] + pp; Clip1(const_255, tmp ); t |= tmp<<24;	*((long *)d) = t; d += pitch; s += 16;
									   
	tmp = s[0] + pp; Clip1(const_255, tmp ); t  = tmp<<0;
	tmp = s[1] + pp; Clip1(const_255, tmp ); t |= tmp<<8;
	tmp = s[2] + pp; Clip1(const_255, tmp ); t |= tmp<<16;
	tmp = s[3] + pp; Clip1(const_255, tmp ); t |= tmp<<24;	*((long *)d) = t;
}


void ADD_CLIP_CHROMA_INTRA( unsigned char *s, int   pp,  unsigned char *d, int pitch )
{
	int t, tmp;
	int const_255 = 255;

	tmp = s[0] + pp; Clip1( const_255, tmp ); t  = tmp<<0;
	tmp = s[1] + pp; Clip1( const_255, tmp ); t |= tmp<<8;
	tmp = s[2] + pp; Clip1( const_255, tmp ); t |= tmp<<16;
	tmp = s[3] + pp; Clip1( const_255, tmp ); t |= tmp<<24;	*((long *)d) = t; d += pitch; s += 8;  
														  
	tmp = s[0] + pp; Clip1( const_255, tmp ); t  = tmp<<0;
	tmp = s[1] + pp; Clip1( const_255, tmp ); t |= tmp<<8;
	tmp = s[2] + pp; Clip1( const_255, tmp ); t |= tmp<<16;
	tmp = s[3] + pp; Clip1( const_255, tmp ); t |= tmp<<24;	*((long *)d) = t; d += pitch; s += 8; 
														  
	tmp = s[0] + pp; Clip1( const_255, tmp ); t  = tmp<<0;
	tmp = s[1] + pp; Clip1( const_255, tmp ); t |= tmp<<8;
	tmp = s[2] + pp; Clip1( const_255, tmp ); t |= tmp<<16;
	tmp = s[3] + pp; Clip1( const_255, tmp ); t |= tmp<<24;	*((long *)d) = t; d += pitch; s += 8; 

	tmp = s[0] + pp; Clip1( const_255, tmp ); t  = tmp<<0;
	tmp = s[1] + pp; Clip1( const_255, tmp ); t |= tmp<<8;
	tmp = s[2] + pp; Clip1( const_255, tmp ); t |= tmp<<16;
	tmp = s[3] + pp; Clip1( const_255, tmp ); t |= tmp<<24;	*((long *)d) = t;
}

#else

#define ADD_CLIP_CHROMA_INTRA( s, pp, d, pitch )	\
{													\
	if( pp >= 0 )									\
		ADD_CLIP_CHROMA_INTRA_u8(s, pp, d, pitch );	\
	else											\
		ADD_CLIP_CHROMA_INTRA_s8(s, pp, d, pitch );	\
}

#define ADD_CLIP_LUMA_INTRA( s, pp, d, pitch )		\
{													\
	if( pp >= 0 )									\
		ADD_CLIP_LUMA_INTRA_u8(s, pp, d, pitch );	\
	else											\
		ADD_CLIP_LUMA_INTRA_s8(s, pp, d, pitch );	\
}

#define ADD_CLIP_CHROMA_INTER( s, pp, pitch )		\
{													\
	if( pp >= 0 )									\
		ADD_CLIP_CHROMA_INTER_u8(s, pp, pitch );	\
	else											\
		ADD_CLIP_CHROMA_INTER_s8(s, pp, pitch );	\
}

#endif

#ifdef KINOMA_AVC
void ADD_CLIP_LUMA_INTRA_DC(unsigned char *s0, short *dc, unsigned char *d0, int pitch  )
{
	int pp;
	unsigned char *s, *d;

	pp = ((dc[0]+32)>>6);
	s = s0; d = d0;			ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[1]+32)>>6);
	s += 4; d += 4;			ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[2]+32)>>6);
	s += 4; d += 4;			ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[3]+32)>>6);
	s += 4; d += 4;			ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[4]+32)>>6);
	s = s0 + (4*16);
	d = d0 + (4*pitch);		ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[5]+32)>>6);
	s += 4; d += 4;			ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[6]+32)>>6);
	s += 4; d += 4;			ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[7]+32)>>6);
	s += 4; d += 4;			ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[8]+32)>>6);
	s = s0 + (8*16);
	d = d0 + (8*pitch);		ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[9]+32)>>6);
	s += 4; d += 4;			ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[10]+32)>>6);
	s += 4; d += 4;			ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[11]+32)>>6);
	s += 4; d += 4;			ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[12]+32)>>6);
	s = s0 + (12*16);
	d = d0 + (12*pitch);	ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[13]+32)>>6);
	s += 4; d += 4;			ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[14]+32)>>6);
	s += 4; d += 4;			ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );

	pp = ((dc[15]+32)>>6);
	s += 4; d += 4;			ADD_CLIP_LUMA_INTRA( s, pp,  d, pitch );
}

static  const Ipp16u dequant_coef_0[6]		= {10,11,13,14,16,18};	//only the first colum	--bnie  1/28/2009
static	const Ipp8u	ac_block_idx_x[16]		= {0, 4, 0, 4, 8, 12, 8, 12, 0, 4, 0, 4,  8, 12,  8, 12 };// These two array use for DC and AC mode for Luma -- JVTG50 6.4.3
static	const Ipp8u	ac_block_idx_y[16]		= {0, 0, 4, 4, 0,  0, 4,  4, 8, 8, 12,12, 8,  8, 12, 12 };
static	const Ipp8u	decode_block_scan[16]	= {0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15};		// For DC index in AC mode -- WWD
static	const int	cbpmask[16]				= {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
static	const Ipp8u	block_idx[]				= {0,0, 4,0, 0,4, 4,4};


IppStatus __STDCALL ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_openmax
(
	Ipp16s	**ppSrcCoeff,
	Ipp8u	*pSrcDstYPlane,
	Ipp32u	srcdstYStep,
	const IppIntra16x16PredMode_H264 intra_luma_mode,
	const Ipp32u cbp4x4,
	const Ipp32u QP,
	const Ipp8u  edge_type
)
{
	// ppSrcCoeff add 16 only when the "coresponding cbp bit" equal 1
	// cbp4x4 : 0 specify Luma DC
	//          1--16 specify Luma AC block coeffs
	int		i,	j, k;
	int		iv, ih, ib, ic, iaa;			// Used for Plane prediction
	unsigned int  cbp = cbp4x4 & 0x1ffff;	// Need 17 bits to represent Luma information 
	int		pitch = srcdstYStep;
	
	OMXResult result;
	OMX_S32 availability = 0;

	__ALIGN16(Ipp8u, prediction_array, 256);
	short		dcArray[4][4];
	__ALIGN4(short, dcResult0, 16);
	short		*dcResult = dcResult0;
	
	if( !(edge_type&IPPVC_LEFT_EDGE) )
		availability |= OMX_VC_LEFT;
	if( !(edge_type&IPPVC_TOP_EDGE) )
		availability |= OMX_VC_UPPER;

	if(cbp == 0)
	{
		result = omxVCM4P10_PredictIntra_16x16
				(
					pSrcDstYPlane - 1,			//const OMX_U8* pSrcLeft, 
					pSrcDstYPlane - pitch,		//const OMX_U8 *pSrcAbove, 
					pSrcDstYPlane - pitch - 1,	//const OMX_U8 *pSrcAboveLeft, 
					pSrcDstYPlane, 
					pitch, 
					pitch, 
					intra_luma_mode, 
					availability
				);
		goto bail;
	}

	result = omxVCM4P10_PredictIntra_16x16
			(
				pSrcDstYPlane - 1,				//const OMX_U8* pSrcLeft, 
				pSrcDstYPlane - pitch,			//const OMX_U8 *pSrcAbove, 
				pSrcDstYPlane - pitch - 1,		//const OMX_U8 *pSrcAboveLeft, 
				prediction_array, 
				pitch, 
				16, 
				intra_luma_mode, 
				availability
			);
		
	// Second, we do inverse Hadamard transform for DC coeffs
	if(cbp & 0x01)
	{
#ifdef FORCE_C_IMPLEMENTATION
		result =  omxVCM4P10_InvTransformDequant_LumaDC( *ppSrcCoeff,dcResult, QP );
#else
		memcpy( dcResult, *ppSrcCoeff, 16*sizeof(OMX_S16));
		result =  armVCM4P10_InvTransformDequantLumaDC4x4( dcResult, QP);
#endif
		*ppSrcCoeff += 16;
	}
	else
	{
		*((long *)(dcResult+ 0)) = 0;
		*((long *)(dcResult+ 2)) = 0;
		*((long *)(dcResult+ 4)) = 0;
		*((long *)(dcResult+ 6)) = 0;
		*((long *)(dcResult+ 8)) = 0;
		*((long *)(dcResult+10)) = 0;
		*((long *)(dcResult+12)) = 0;
		*((long *)(dcResult+14)) = 0;
	}

	if(cbp == 1)
	{
		ADD_CLIP_LUMA_INTRA_DC( prediction_array, dcResult, pSrcDstYPlane, pitch  );
		goto bail;
	}

	for(k=0; k< 16; k++)
	{
		// Please note: we use normal order according to AC components here-- WWD in 2006-05-06
		int block_x = ac_block_idx_x[k];
		int block_y = ac_block_idx_y[k];
		int	dc_idx  = decode_block_scan[k];
		Ipp8u  *d	= pSrcDstYPlane    + block_y*pitch + block_x;
		Ipp8u  *p	= prediction_array + block_y*16    + block_x;

		if( cbp & (1<<(1+k)) )
		{
			Ipp16s *coef =  *ppSrcCoeff;
			armVCM4P10_DequantLumaAC4x4(coef, QP);
			coef[0] = dcResult[dc_idx];
			armVCM4P10_TransformResidual4x4(coef,coef);
			ADD_CLIP_GENERIC(coef, p, d, 16, pitch );
			*ppSrcCoeff += 16;
		}	
		else
		{
			int t0 = ((dcResult[dc_idx]+32)>>6);
			ADD_CLIP_LUMA_INTRA( p, t0, d, pitch );
		}
	}

bail:
	return ippStsNoErr;
}

#define HAS_BB_FROM_EDGE (!(edge_type&IPPVC_TOP_EDGE))
#define HAS_AA_FROM_EDGE (!(edge_type&IPPVC_LEFT_EDGE))
#define HAS_CC_FROM_EDGE (!(edge_type&IPPVC_TOP_RIGHT_EDGE))
//#define HAS_DD_FROM_EDGE (!(edge_type&IPPVC_TOP_LEFT_EDGE))

#define A_POS 1
#define B_POS 0
#define C_POS 6

#define HAS_A (hasABC&(1<<A_POS))
#define HAS_B (hasABC&(1<<B_POS))
#define HAS_C (hasABC&(1<<C_POS))

#define CONST_ABC   ((1<<A_POS)|(1<<B_POS)|(1<<C_POS))
#define CONST_AB    ((1<<A_POS)|(1<<B_POS))

static Ipp8u	hasABC_ary[16] = { CONST_ABC, CONST_ABC, CONST_ABC, CONST_AB,    
								   CONST_ABC, CONST_ABC, CONST_ABC, CONST_AB,   
								   CONST_ABC, CONST_ABC, CONST_ABC, CONST_AB,  
								   CONST_ABC, CONST_AB,  CONST_ABC, CONST_AB };

IppStatus __STDCALL ippiReconstructLumaIntraMB_H264_16s8u_C1R_openmax(Ipp16s **ppSrcCoeff,
                                                              Ipp8u *pSrcDstYPlane,
                                                              Ipp32s srcdstYStep,
                                                              const IppIntra4x4PredMode_H264 *pMBIntraTypes,
                                                              const Ipp32u cbp4x4,
                                                              const Ipp32u QP,
                                                              const Ipp8u edge_type)
{
   
	__ALIGN8(Ipp8u, pred, 16+2*16);
	Ipp16s	*m0 = (Ipp16s *)&pred[16];

	Ipp8u			PredPel[16]; 
	int				block_x, block_y;
	int				k;
	Ipp8u			*dst;
	int				pitch   = srcdstYStep;
	unsigned int	cbp     = cbp4x4 & 0x1ffff;		// Need 17 bits to represent Luma information 
	int				hasB = HAS_BB_FROM_EDGE;
	int				hasA = HAS_AA_FROM_EDGE;
	int				hasC = HAS_CC_FROM_EDGE;
	OMXResult		result;
	int				tmp;
	
	tmp = (hasB<<B_POS) | (hasB<<C_POS);
	hasABC_ary[0] = (hasA<<A_POS) | tmp; 
	hasABC_ary[1] = (   1<<A_POS) | tmp; 
	hasABC_ary[4] = (   1<<A_POS) | tmp; 
	hasABC_ary[5] = (   1<<A_POS) | (hasB<<B_POS) | (hasC<<C_POS); 
	tmp =  (hasA<<A_POS) | (1<<B_POS) | (1<<C_POS);
	hasABC_ary[2]  = tmp; 
	hasABC_ary[8]  = tmp; 
	hasABC_ary[10] = tmp; 

	for(k=0; k<16; k++)			// In scan order (4x4 blocks)
	{
		IppIntra4x4PredMode_H264 predMode = pMBIntraTypes[k];
		OMX_S32 availability = hasABC_ary[k];

		block_x = ac_block_idx_x[k];
		block_y = ac_block_idx_y[k];
		dst = pSrcDstYPlane + block_y*srcdstYStep + block_x;			// Pointer to top_left cornor of one 4x4 block					
		
		result = omxVCM4P10_PredictIntra_4x4
				(
					dst - 1,				//const OMX_U8* pSrcLeft, 
					dst - pitch,			//const OMX_U8 *pSrcAbove, 
					dst - pitch - 1,		//const OMX_U8 *pSrcAboveLeft, 
					pred, 
					pitch, 
					4, 
					predMode, 
					availability
				);

		// Make decision for AC components
		if( cbp & cbpmask[k])
		{
			Ipp16s *coef =  *ppSrcCoeff;
			armVCM4P10_DequantLumaAC4x4(coef, QP);
			armVCM4P10_TransformResidual4x4(coef,coef);
			ADD_CLIP_GENERIC(coef, pred, dst, 4, pitch );
			*ppSrcCoeff += 16;
		}
		else
		{
			// MUST SAVE this result to array for next prediction
			*((long *)&dst[0])								= *((long *)&pred[0]);
			*((long *)&dst[srcdstYStep])					= *((long *)&pred[4]);
			*((long *)&dst[srcdstYStep<<1])					= *((long *)&pred[8]);
			*((long *)&dst[(srcdstYStep<<1)+srcdstYStep])	= *((long *)&pred[12]);
		}
	}

	return ippStsNoErr;
}


IppStatus __STDCALL ippiReconstructLumaInterMB_H264_16s8u_C1R_openmax
(	
	Ipp16s **ppSrcCoeff,
	Ipp8u *pSrcDstYPlane,
	Ipp32u srcdstYStep,
	Ipp32u cbp4x4,
	Ipp32s QP
)
{
	int		k;
	Ipp8u	*dst;
	int		pitch = srcdstYStep;
	unsigned int  cbp = cbp4x4 & 0x1ffff;		// Need 17 bits to represent Luma information 
	int		block_x, block_y;

	for(k=0; k<16; k++)			// In scan order
	{	
		Ipp16s *coef;

		if( !(cbp & (1<< (1+k))))
			continue;

		block_x = ac_block_idx_x[k];
		block_y = ac_block_idx_y[k];

		dst = pSrcDstYPlane + block_y*pitch + block_x;			// Pointer to top_left cornor of one 4x4 block
		coef =  *ppSrcCoeff;
		armVCM4P10_DequantLumaAC4x4(coef, QP);
		armVCM4P10_TransformResidual4x4(coef,coef);
		ADD_CLIP_GENERIC(coef, dst, dst, pitch, pitch );
		*ppSrcCoeff += 16;
	}

	return ippStsNoErr;
}


IppStatus __STDCALL ippiReconstructChromaInterMB_H264_16s8u_P2R_openmax
(	Ipp16s **ppSrcCoeff,
	Ipp8u *pSrcDstUPlane,
	Ipp8u *pSrcDstVPlane,
	const Ipp32u srcdstUVStep,
	const Ipp32u cbp4x4,
	const Ipp32u QP
)
{
	int		dcArrayU[4], dcResultU[4];
	int		dcArrayV[4], dcResultV[4];
	int		k;
	unsigned int	cbp = cbp4x4>> 17;			// Use Chroma part ONLY
	int		block_x, block_y;

	int		pitch = srcdstUVStep;
	int		qp_per = QP/6;
	int		qp_rem = QP%6;
	Ipp16u	dquant0 = dequant_coef_0[qp_rem];
	__ALIGN8(Ipp8u, pred, 16+2*16);
	Ipp16s	*m0 = (Ipp16s *)&pred[16];

	if(!(cbp & 0xffff))
		return ippStsNoErr;	// This branch is not tested yet, but OK! -- WWD in 2006-06-15

	// U
	if(cbp & 0x01)
	{
		if(QP >= 6)
		{
			int tmp = dquant0<< (qp_per - 1);
			dcArrayU[0] = (*ppSrcCoeff)[0] * tmp;
			dcArrayU[1] = (*ppSrcCoeff)[1] * tmp;
			dcArrayU[2] = (*ppSrcCoeff)[2] * tmp;
			dcArrayU[3] = (*ppSrcCoeff)[3] * tmp;
		}
		else
		{
			int tmp = dquant0 >> 1;
			dcArrayU[0] = (*ppSrcCoeff)[0] * tmp;
			dcArrayU[1] = (*ppSrcCoeff)[1] * tmp;
			dcArrayU[2] = (*ppSrcCoeff)[2] * tmp;
			dcArrayU[3] = (*ppSrcCoeff)[3] * tmp;
		}

		dcResultU[0] = dcArrayU[0] + dcArrayU[1] + dcArrayU[2] + dcArrayU[3];
		dcResultU[1] = dcArrayU[0] - dcArrayU[1] + dcArrayU[2] - dcArrayU[3];
		dcResultU[2] = dcArrayU[0] + dcArrayU[1] - dcArrayU[2] - dcArrayU[3];
		dcResultU[3] = dcArrayU[0] - dcArrayU[1] - dcArrayU[2] + dcArrayU[3];

		*ppSrcCoeff += 4;
	}
	else
	{
		dcResultU[0] = 0;
		dcResultU[1] = 0;
		dcResultU[2] = 0;
		dcResultU[3] = 0;
	}

	// V
	if(cbp & 0x02)
	{
		if(QP >= 6)
		{
			int tmp = dquant0<< (qp_per - 1);
			dcArrayV[0] = (*ppSrcCoeff)[0] * tmp;
			dcArrayV[1] = (*ppSrcCoeff)[1] * tmp;
			dcArrayV[2] = (*ppSrcCoeff)[2] * tmp;
			dcArrayV[3] = (*ppSrcCoeff)[3] * tmp;
		}
		else
		{
			int tmp = dquant0 >> 1;
			dcArrayV[0] = (*ppSrcCoeff)[0] * tmp;
			dcArrayV[1] = (*ppSrcCoeff)[1] * tmp;
			dcArrayV[2] = (*ppSrcCoeff)[2] * tmp;
			dcArrayV[3] = (*ppSrcCoeff)[3] * tmp;
		}

		dcResultV[0] = dcArrayV[0] + dcArrayV[1] + dcArrayV[2] + dcArrayV[3];
		dcResultV[1] = dcArrayV[0] - dcArrayV[1] + dcArrayV[2] - dcArrayV[3];
		dcResultV[2] = dcArrayV[0] + dcArrayV[1] - dcArrayV[2] - dcArrayV[3];
		dcResultV[3] = dcArrayV[0] - dcArrayV[1] - dcArrayV[2] + dcArrayV[3];

		*ppSrcCoeff += 4;
	}
	else
	{
		dcResultV[0] = 0;
		dcResultV[1] = 0;
		dcResultV[2] = 0;
		dcResultV[3] = 0;
	}

	// U -- AC	
	for(k=0; k<4; k++)									// CBP decision
	{
		block_x = block_idx[k<<1];
		block_y = block_idx[(k<<1) + 1];

		if(!(cbp&(1<<(2+k))))
		{												// This block has no AC information : but we have DC information
			int		dc = ((dcResultU[k]+32)>>6);			
			Ipp8u	*s =  pSrcDstUPlane + block_y *srcdstUVStep + block_x;

			ADD_CLIP_CHROMA_INTER( s, dc, srcdstUVStep );
		}
		else
		{
			Ipp16s *coef	=  *ppSrcCoeff;
			Ipp8u  *dst		= pSrcDstUPlane + block_y*pitch + block_x;
			Ipp8u  *p		= pSrcDstUPlane + block_y*pitch + block_x;
			
			armVCM4P10_DequantLumaAC4x4(coef, QP);
			coef[0] = dcResultU[k];
			armVCM4P10_TransformResidual4x4(coef,coef);
			ADD_CLIP_GENERIC(coef, dst, dst, pitch, pitch );
			*ppSrcCoeff += 16;
		}
	}
	
	// V -- AC
	for(k=0; k<4; k++)									// CBP decision
	{
		block_x = block_idx[k<<1];
		block_y = block_idx[(k<<1) + 1];

		// This statement is different for U and V
		if(!(cbp&(1<<(6+k))))
		{												// This block has no AC information : but we have DC information
			int		dc = ((dcResultV[k]+32)>>6);			
			Ipp8u	*s =  pSrcDstVPlane + block_y *srcdstUVStep + block_x;

			ADD_CLIP_CHROMA_INTER( s, dc, srcdstUVStep );
		}
		else
		{
			Ipp8u  *dst		= pSrcDstVPlane + block_y*pitch + block_x;
			Ipp8u  *p		= pSrcDstVPlane + block_y*pitch + block_x;
			Ipp16s *coef	=  *ppSrcCoeff;
			
			armVCM4P10_DequantLumaAC4x4(coef, QP);
			coef[0] = dcResultV[k];
			armVCM4P10_TransformResidual4x4(coef,coef);
			ADD_CLIP_GENERIC(coef, dst, dst, pitch, pitch );
			*ppSrcCoeff += 16;
		}
	}

	return ippStsNoErr;
}

IppStatus __STDCALL ippiReconstructChromaIntraMB_H264_16s8u_P2R_openmax
(
	Ipp16s **ppSrcCoeff,
    Ipp8u *pSrcDstUPlane,
    Ipp8u *pSrcDstVPlane,
    const Ipp32u srcdstUVStep,
    const IppIntraChromaPredMode_H264 intra_chroma_mode,
    const Ipp32u cbp4x4,
    const Ipp32u QP,
    const Ipp8u edge_type
)
{
	int		i,	j, k;
	Ipp8u   *dst_u		= pSrcDstUPlane;
	Ipp8u   *dst_v		= pSrcDstVPlane;
	int		pitch		= srcdstUVStep;
	int		qp_per_uv	= QP/6;
	int		qp_rem_uv	= QP%6;	
	__ALIGN8(Ipp8u, pred, 16+2*16);
	Ipp16s	*m0 = (Ipp16s *)&pred[16];

	__ALIGN8(Ipp8u, pred_uv, 64*2);
	unsigned char *pred_u = pred_uv;
	unsigned char *pred_v = pred_uv + 64;
	int		dcArrayU[4], dcResultU[4];
	int		dcArrayV[4], dcResultV[4];
	int		iv, ih, ib, ic, iaa;	// Used for Plane prediction
	int	ioff, joff;
	unsigned int cbp;
	OMXResult	result;
	OMX_S32 availability=0;

	if( !(edge_type&IPPVC_LEFT_EDGE) )
		availability |= OMX_VC_LEFT;
	if( !(edge_type&IPPVC_TOP_EDGE) )
		availability |= OMX_VC_UPPER;

	result =  omxVCM4P10_PredictIntraChroma_8x8
		(
			dst_u - 1,				//const OMX_U8* pSrcLeft, 
			dst_u - pitch,			//const OMX_U8 *pSrcAbove, 
			dst_u - pitch - 1,		//const OMX_U8 *pSrcAboveLeft, 
			pred_u, 
			pitch, 
			8, 
			intra_chroma_mode, 
			availability
		);

	result =  omxVCM4P10_PredictIntraChroma_8x8
		(
			dst_v - 1,				//const OMX_U8* pSrcLeft, 
			dst_v - pitch,			//const OMX_U8 *pSrcAbove, 
			dst_v - pitch - 1,		//const OMX_U8 *pSrcAboveLeft, 
			pred_v, 
			pitch, 
			8, 
			intra_chroma_mode, 
			availability
		);

	// Second, we do inverse transform for each 4x4 block
	//         include inverse Quantization and add operation
	cbp = cbp4x4>> 17;			// Use Chroma part ONLY
	if(!(cbp & 0xffff))
	{
		// Non- cbp has information: use prediction as coeffs ONLY  -- WWD in 2006-05-05
		Ipp8u *d;
		Ipp8u *s;

		d = dst_u - pitch;
		s = pred_u - 8;

		d += pitch; s += 8; *(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8;	*(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8; *(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8;	*(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8;	*(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8;	*(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8;	*(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8;	*(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);

		d = dst_v - pitch;
		s = pred_v - 8;

		d += pitch; s += 8; *(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8;	*(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8; *(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8;	*(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8;	*(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8;	*(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8;	*(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);
		d += pitch; s += 8;	*(long *)(d+0) = *(long *)(s+0); *(long *)(d+4) = *(long *)(s+4);

		goto bail;
	}

{
	Ipp16u dquant_dc = dequant_coef_0[qp_rem_uv];
	// U
	if(cbp & 0x01)
	{
		// 2.1 2x2 DC Transform
		// IPPVC_CBP_1ST_CHROMA_DC_BITPOS
		if(QP >= 6)
		{
			int tmp = (qp_per_uv - 1);

			dcArrayU[0] = ((*ppSrcCoeff)[0]) *dquant_dc << tmp;
			dcArrayU[1] = ((*ppSrcCoeff)[1]) *dquant_dc << tmp;
			dcArrayU[2] = ((*ppSrcCoeff)[2]) *dquant_dc << tmp;
			dcArrayU[3] = ((*ppSrcCoeff)[3]) *dquant_dc << tmp;
		}
		else
		{
			dcArrayU[0] = ((*ppSrcCoeff)[0]) *dquant_dc>> 1;
			dcArrayU[1] = ((*ppSrcCoeff)[1]) *dquant_dc>> 1;
			dcArrayU[2] = ((*ppSrcCoeff)[2]) *dquant_dc>> 1;
			dcArrayU[3] = ((*ppSrcCoeff)[3]) *dquant_dc>> 1;
		}

		dcResultU[0] = (dcArrayU[0] + dcArrayU[1] + dcArrayU[2] + dcArrayU[3]);
		dcResultU[1] = (dcArrayU[0] - dcArrayU[1] + dcArrayU[2] - dcArrayU[3]);
		dcResultU[2] = (dcArrayU[0] + dcArrayU[1] - dcArrayU[2] - dcArrayU[3]);
		dcResultU[3] = (dcArrayU[0] - dcArrayU[1] - dcArrayU[2] + dcArrayU[3]);

		*ppSrcCoeff += 4;
	}
	else
	{
		dcResultU[0] = 0;
		dcResultU[1] = 0;
		dcResultU[2] = 0;
		dcResultU[3] = 0;
	}

	// V
	if(cbp & 0x02)
	{
		if(QP >= 6)
		{
			int tmp = (qp_per_uv - 1);

			dcArrayV[0] = ((*ppSrcCoeff)[0]) *dquant_dc << tmp;
			dcArrayV[1] = ((*ppSrcCoeff)[1]) *dquant_dc << tmp;
			dcArrayV[2] = ((*ppSrcCoeff)[2]) *dquant_dc << tmp;
			dcArrayV[3] = ((*ppSrcCoeff)[3]) *dquant_dc << tmp;
		}
		else
		{
			dcArrayV[0] = ((*ppSrcCoeff)[0]) *dquant_dc>> 1;
			dcArrayV[1] = ((*ppSrcCoeff)[1]) *dquant_dc>> 1;
			dcArrayV[2] = ((*ppSrcCoeff)[2]) *dquant_dc>> 1;
			dcArrayV[3] = ((*ppSrcCoeff)[3]) *dquant_dc>> 1;
		}

		dcResultV[0] = (dcArrayV[0] + dcArrayV[1] + dcArrayV[2] + dcArrayV[3]);
		dcResultV[1] = (dcArrayV[0] - dcArrayV[1] + dcArrayV[2] - dcArrayV[3]);
		dcResultV[2] = (dcArrayV[0] + dcArrayV[1] - dcArrayV[2] - dcArrayV[3]);
		dcResultV[3] = (dcArrayV[0] - dcArrayV[1] - dcArrayV[2] + dcArrayV[3]);

		*ppSrcCoeff += 4;
	}
	else
	{
		dcResultV[0] = 0;
		dcResultV[1] = 0;
		dcResultV[2] = 0;
		dcResultV[3] = 0;
	}

	// U -- AC
	for(k=0; k<4; k++)									// CBP decision
	{
		int block_x = block_idx[k<<1];
		int block_y = block_idx[(k<<1) + 1];

		if(!(cbp&(1<<(2+k))))
		{
			Ipp8u	*d	= dst_u  + block_y*pitch + block_x;
			Ipp8u	*s	= pred_u + block_y*8     + block_x;
			int		pp  = ((dcResultU[k]+32)>>6);
			ADD_CLIP_CHROMA_INTRA( s, pp, d, pitch );
		}
		else
		{
			Ipp8u	*d	= dst_u  + block_y*pitch + block_x;
			Ipp8u	*p	= pred_u + block_y*8     + block_x;
			Ipp16s	*coef =  *ppSrcCoeff;

			armVCM4P10_DequantLumaAC4x4(coef, QP);
			coef[0] = dcResultU[k];
			armVCM4P10_TransformResidual4x4(coef,coef);
			ADD_CLIP_GENERIC(coef, p, d, 8, pitch );
			*ppSrcCoeff += 16;
		}
	}

	// V -- AC
	for(k=0; k<4; k++)	// CBP decision
	{
		int block_x = block_idx[k<<1];
		int block_y = block_idx[(k<<1) + 1];

		if(!(cbp&(1<<(6+k))))
		{
			Ipp8u	*d	= dst_v  + block_y*pitch + block_x;	// This block has no AC information : but we have DC information
			Ipp8u	*s	= pred_v + block_y*8     + block_x;
			int		pp  = (dcResultV[k]+32)>>6;
			ADD_CLIP_CHROMA_INTRA(s, pp, d, pitch );
		}
		else
		{
			Ipp8u  *d	= dst_v  + block_y*pitch + block_x;
			Ipp8u  *p	= pred_v + block_y*8     + block_x;
			Ipp16s *coef =  *ppSrcCoeff;

			armVCM4P10_DequantLumaAC4x4(coef, QP);
			coef[0] = dcResultV[k];
			armVCM4P10_TransformResidual4x4(coef,coef);
			ADD_CLIP_GENERIC(coef, p, d, 8, pitch );
			*ppSrcCoeff += 16;
		}
	}
}

bail:
	return ippStsNoErr;
}

IppStatus __STDCALL ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_openmax_intel
(
	Ipp32u **ppBitStream_in_out,
	Ipp32s *pBitOffset_in_out,
	Ipp16s *pNumCoeff_in_out,
	Ipp16s **ppDstCoeffs_in_out,
	const Ipp32s *pTblCoeffToken_whocares,
	const Ipp32s **ppTblTotalZerosCR_whocares,
	const Ipp32s **ppTblRunBeforeB_whocares 
)
{	
	OMX_U8 *pBitStream_local  = *ppBitStream_in_out;
	OMX_S32 pOffset_local     = 31 - (*pBitOffset_in_out);
	unsigned char  pNumCoeff_local = -1;
	unsigned char	CP[32];
	unsigned char	*pCP = CP;
	IppStatus		err = 0;

	err = (IppStatus)armVCM4P10_DecodeCoeffsToPair_intel 
			(
				(OMX_U8 **)&pBitStream_local,
				(OMX_S32 *)&pOffset_local,
				(OMX_U8 * )&pNumCoeff_local,
				(OMX_U8 **)&pCP, 17, 4
			 );

	if( pNumCoeff_local != 0 )
	{
		pCP = CP;
		armVCM4P10_UnpackBlock2x2( &pCP, *ppDstCoeffs_in_out );
		*ppDstCoeffs_in_out += 4;
	}

	*pBitOffset_in_out	 = 31 - pOffset_local;
	*ppBitStream_in_out  = pBitStream_local;
	*pNumCoeff_in_out    = pNumCoeff_local;

	return err;
}

IppStatus __STDCALL 
ippiDecodeCAVLCCoeffs_H264_1u16s_openmax_intel
(
	Ipp32u **ppBitStream_in_out,
	Ipp32s *pBitOffset_in_out,
	Ipp16s *pNumCoeff_in_out,
	Ipp16s **ppDstCoeffs_in_out,
	Ipp32u uVLCSelect,
	Ipp16s uMaxNumCoeff,
	const Ipp32s **whocares1,
	const Ipp32s **whocares2,
	const Ipp32s **whocares3,
	const Ipp32s *pScanMatrix_whocares 
)
{
	OMX_U8 *pBitStream_local  = *ppBitStream_in_out;
	OMX_S32 pOffset_local     = 31 - (*pBitOffset_in_out);
	unsigned char  pNumCoeff_local = -1;
	unsigned char	CP[64];
	unsigned char	*pCP = CP;
	IppStatus		err = 0;

	err = (IppStatus) armVCM4P10_DecodeCoeffsToPair_intel
			(
				(OMX_U8 **)&pBitStream_local,
				(OMX_S32 *)&pOffset_local,
				(OMX_U8 * )&pNumCoeff_local,
				(OMX_U8 **)&pCP,
				(OMX_INT)uVLCSelect,
				uMaxNumCoeff
			);
	
	if( pNumCoeff_local != 0 )
	{
		pCP = CP;
		armVCM4P10_UnpackBlock4x4( &pCP, *ppDstCoeffs_in_out );
		*ppDstCoeffs_in_out += 16;
	}

	*pBitOffset_in_out	 = 31 - pOffset_local;
	*ppBitStream_in_out  = pBitStream_local;
	*pNumCoeff_in_out    = pNumCoeff_local;
	
	return err;
}

#endif

void Init_AVC_Deblocking_dec_ARM_V6()
{
	ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = 	ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_openmax;
	ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal = 	ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_openmax;
}

#ifdef KINOMA_AVC_ENC
// QP range : luma[0--51], Chroma[0--39]
DECL_ALIGN_16 static unsigned short qp_per_table[52] = { 
	0, 0, 0, 0, 0, 0, /*0  -- 5 */
	1, 1, 1, 1, 1, 1, /*6  -- 11 */
	2, 2, 2, 2, 2, 2, /*12 -- 17 */
	3, 3, 3, 3, 3, 3, /*18 -- 23 */
	4, 4, 4, 4, 4, 4, /*24 -- 29 */
	5, 5, 5, 5, 5, 5, /*30 -- 35 */
	6, 6, 6, 6, 6, 6, /*36 -- 41 */
	7, 7, 7, 7, 7, 7, /*42 -- 47 */
	8, 8, 8, 8        /*48 -- 51  */
};

DECL_ALIGN_16 static unsigned short qp_rem_table[52] = {
	0, 1, 2, 3, 4, 5, /*0  -- 5 */
	0, 1, 2, 3, 4, 5, /*6  -- 11 */
	0, 1, 2, 3, 4, 5, /*12 -- 17 */
	0, 1, 2, 3, 4, 5, /*18 -- 23 */
	0, 1, 2, 3, 4, 5, /*24 -- 29 */
	0, 1, 2, 3, 4, 5, /*30 -- 35 */
	0, 1, 2, 3, 4, 5, /*36 -- 41 */
	0, 1, 2, 3, 4, 5, /*42 -- 47 */
	0, 1, 2, 3        /*48 -- 51  */
};

DECL_ALIGN_16 static	const unsigned short dequant_coef[6][16] = {
	  {10, 13, 10, 13,   13, 16, 13, 16,   10, 13, 10, 13,   13, 16, 13, 16},
	  {11, 14, 11, 14,   14, 18, 14, 18,   11, 14, 11, 14,   14, 18, 14, 18},
	  {13, 16, 13, 16,   16, 20, 16, 20,   13, 16, 13, 16,   16, 20, 16, 20},
	  {14, 18, 14, 18,   18, 23, 18, 23,   14, 18, 14, 18,   18, 23, 18, 23},
	  {16, 20, 16, 20,   20, 25, 20, 25,   16, 20, 16, 20,   20, 25, 20, 25},
	  {18, 23, 18, 23,   23, 29, 23, 29,   18, 23, 18, 23,   23, 29, 23, 29}
	};


IppStatus  __STDCALL  ippiDequantTransformResidualAndAdd_H264_16s_C1I_openmax(
   const Ipp8u*  pPred,
         Ipp16s* pSrcDst,
   const Ipp16s* pDC,
         Ipp8u*  pDst,
         Ipp32s  PredStep,
         Ipp32s  DstStep,
         Ipp32s  QP,
         Ipp32s  AC)
{
	DECL_ALIGN_16 short   block[16];
	int             i,j,k;

	if( AC == NULL )
	{
#if 0
	#define IClip(Min, Max, Val) (((Val) < (Min)) ? (Min) : (((Val) > (Max)) ? (Max) : (Val)))
	#define RECON(x, y)		(unsigned char)(IClip(0, 255, ((( (x) << 6) + 32 + (y) ) >> 6)))
		int             i,j,k;

		if(pDC != NULL)
			pSrcDst[0] = *pDC;                      // Add DC coeff in scan order
		else
		{
			int qp_per = qp_per_table[QP];
			int qp_rem = qp_rem_table[QP];
			pSrcDst[0] = (short)((pSrcDst[0] * dequant_coef[qp_rem][0]) << (qp_per));
		}

		// MUST SAVE this result to array for next prediction
		for(i=0, k=0, j=0; j<4; j++)
		{
			pDst[i + 0] = RECON(pPred[k +0], pSrcDst[0]);
			pDst[i + 1] = RECON(pPred[k +1], pSrcDst[0]);
			pDst[i + 2] = RECON(pPred[k +2], pSrcDst[0]);
			pDst[i + 3] = RECON(pPred[k +3], pSrcDst[0]);

			i += DstStep;
			k += PredStep;
		}
		
		return ippStsNoErr;
#else
		return  ippiDequantTransformResidualAndAdd_H264_16s_C1I_NO_AC
				( pPred, pSrcDst, pDC, pDst, PredStep, DstStep, QP);
#endif
	}

	armVCM4P10_DequantLumaAC4x4( pSrcDst, QP );     
	if(pDC != NULL)
		pSrcDst[0] = *pDC; // Add DC coeff in scan order

	// Do Inverse transform
	armVCM4P10_TransformResidual4x4((OMX_S16 *)block, (OMX_S16 *)pSrcDst);
	ADD_CLIP_GENERIC( block, (unsigned char *)pPred, pDst, PredStep, DstStep );

	return ippStsNoErr;
}

void Init_AVC_Interpolation_enc_ARM_V6()
{
	ippiInterpolateLuma_H264_8u_C1R_universal		= ippiInterpolateLuma_H264_8u_C1R_openmax;
	ippiInterpolateChroma_H264_8u_C1R_universal		= ippiInterpolateChroma_H264_8u_C1R_openmax;
}

void Init_AVC_Reconstruction_enc_ARM_V6()
{
	ippiDequantTransformResidualAndAdd_H264_16s_C1I_universal = ippiDequantTransformResidualAndAdd_H264_16s_C1I_openmax;
}

void kinoma_openmax_lib_avc_enc_init(int implementation)
{
	//return 0;
	//if( implementation >= FSK_ARCH_ARM_V6 )
	{
		Init_AVC_Interpolation_enc_ARM_V6();
		Init_AVC_Deblocking_dec_ARM_V6();
		Init_AVC_Reconstruction_enc_ARM_V6();
	}
}
#endif

#ifdef KINOMA_AVC

void Init_AVC_Interpolation_dec_ARM_V6()
{
	ippiInterpolateLuma_H264_8u_C1R_universal		= ippiInterpolateLuma_H264_8u_C1R_openmax;
	ippiInterpolateChroma_H264_8u_C1R_universal		= ippiInterpolateChroma_H264_8u_C1R_openmax;


	ippiInterpolateLuma_H264_8u_C1R_16x16_universal = ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_16x8_universal  = ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_8x16_universal  = ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_8x8_universal   = ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_8x4_universal   = ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_4x8_universal   = ippiInterpolateLuma_H264_8u_C1R_universal;
	ippiInterpolateLuma_H264_8u_C1R_4x4_universal   = ippiInterpolateLuma_H264_8u_C1R_universal;
}

void Init_AVC_Reconstruction_dec_ARM_V6()
{
	ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_universal = ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_openmax;
	ippiReconstructLumaIntraMB_H264_16s8u_C1R_universal		 = ippiReconstructLumaIntraMB_H264_16s8u_C1R_openmax;
	ippiReconstructLumaInterMB_H264_16s8u_C1R_universal		 = ippiReconstructLumaInterMB_H264_16s8u_C1R_openmax;
	ippiReconstructChromaIntraMB_H264_16s8u_P2R_universal	 = ippiReconstructChromaIntraMB_H264_16s8u_P2R_openmax;
	ippiReconstructChromaInterMB_H264_16s8u_P2R_universal	 = ippiReconstructChromaInterMB_H264_16s8u_P2R_openmax;
}

void Init_AVC_Bitstream_dec_ARM_V6()
{
	ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_universal	 = ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_openmax_intel;
	ippiDecodeCAVLCCoeffs_H264_1u16s_universal			 = ippiDecodeCAVLCCoeffs_H264_1u16s_openmax_intel;
}

void kinoma_openmax_lib_avc_dec_init(int implementation)
{
	//return 0;
	//if( implementation >= FSK_ARCH_ARM_V6 )	//***bnie:fix android 1/19/2011
	{
		Init_AVC_Interpolation_dec_ARM_V6();
		Init_AVC_Deblocking_dec_ARM_V6();
		Init_AVC_Reconstruction_dec_ARM_V6();
#ifdef _WIN32_WCE
		Init_AVC_Bitstream_dec_ARM_V6();
#endif

	}
}
#endif
