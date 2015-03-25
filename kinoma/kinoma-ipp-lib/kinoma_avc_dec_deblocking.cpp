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
#include <stdio.h>
#include <math.h>
#include <assert.h>



#include "kinoma_avc_defines.h"
#include "ippvc.h"
#include "kinoma_ipp_lib.h"
#include "kinoma_utilities.h"


IppStatus (__STDCALL *ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal)			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs)=NULL;
IppStatus (__STDCALL *ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal)			(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs)=NULL;
IppStatus (__STDCALL *ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal)		(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs)=NULL;
IppStatus (__STDCALL *ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal)		(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp8u*    pAlpha,  const Ipp8u*    pBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs)=NULL;
IppStatus (__STDCALL *ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_universal)	(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp32u    nAlpha,  const Ipp32u    nBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs)=NULL;
IppStatus (__STDCALL *ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_universal)	(Ipp8u*    pSrcDst, Ipp32s    srcdstStep, const Ipp32u    nAlpha,  const Ipp32u    nBeta, const Ipp8u*    pThresholds, const Ipp8u*    pBs)=NULL;

#ifdef __KINOMA_IPP__
// Add in 20070901
#define USE_MUL_REPLACE_SHIFT // USE_SHIFT_REPLACE_MUL Or define USE_MUL_REPLACE_SHIFT

#define NEED_INLINE
#ifdef NEED_INLINE
#if defined(__KINOMA_IPP_ARM_V5__)
	#if !TARGET_OS_LINUX && !TARGET_OS_IPHONE
		#define KINOMA_INLINE static __forceinline
		#include <armintr.h>
	#else
		#define KINOMA_INLINE static __inline
	#endif
#else
#define KINOMA_INLINE static __inline
#endif
#else
#define KINOMA_INLINE 
#endif

static void ( *loopFilter_LumaV_BS4_with16pel_universal) (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)=NULL;
static void ( *loopFilter_LumaV_BS4_with8pel_universal ) (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)=NULL;
static void ( *loopFilter_LumaH_BS4_with16pel_universal) (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)=NULL;
static void ( *loopFilter_LumaV_BSN_universal )			 (Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep, Ipp8u C0)=NULL;
static void ( *loopFilter_LumaH_BSN_universal )			 (Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep, Ipp8u C0)=NULL;

void loopFilter_LumaV_BS4_with16pel_c (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep);
void loopFilter_LumaV_BS4_with8pel_c  (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep);
void loopFilter_LumaH_BS4_with16pel_c (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep);
void loopFilter_LumaV_BSN_c           (Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep, Ipp8u C0);
void loopFilter_LumaH_BSN_c           (Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep, Ipp8u C0);

#if _KINOMA_LOSSY_OPT_
static void ( *loopFilter_LumaV_BS4_with16pel_sim_universal) (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)=NULL;
//static void ( *loopFilter_LumaV_BS4_with8pel_sim_universal ) (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)=NULL;
static void ( *loopFilter_LumaH_BS4_with16pel_sim_universal) (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)=NULL;
static void ( *loopFilter_LumaV_BSN_sim_universal )			 (Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep)=NULL;
static void ( *loopFilter_LumaH_BSN_sim_universal )			 (Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep)=NULL;

void loopFilter_LumaV_BS4_with16pel_simply_c (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep);
void loopFilter_LumaH_BS4_with16pel_simply_c (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep);
void loopFilter_LumaV_BSN_simply_c           (Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep);
void loopFilter_LumaH_BSN_simply_c           (Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep);
#endif

#ifdef __cplusplus
extern "C" {
#endif

void loopFilter_LumaV_BS4_with16pel_arm (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep);
void loopFilter_LumaV_BS4_with8pel_arm  (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep);
void loopFilter_LumaH_BS4_with16pel_arm (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep);
void loopFilter_LumaV_BSN_arm           (Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep, Ipp8u C0);
void loopFilter_LumaH_BSN_arm           (Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep, Ipp8u C0);


#if _KINOMA_LOSSY_OPT_
void loopFilter_LumaV_BS4_with16pel_simply_arm (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep);
void loopFilter_LumaH_BS4_with16pel_simply_arm (Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep);
void loopFilter_LumaV_BSN_simply_arm           (Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep);
void loopFilter_LumaH_BSN_simply_arm           (Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep);
#endif

#ifdef __cplusplus
}
#endif

#define loopFilter_LumaV_BS4_with16pel_x				(*loopFilter_LumaV_BS4_with16pel_universal)
#define loopFilter_LumaV_BS4_with8pel_x 				(*loopFilter_LumaV_BS4_with8pel_universal ) 
#define loopFilter_LumaH_BS4_with16pel_x          		(*loopFilter_LumaH_BS4_with16pel_universal)
#define loopFilter_LumaV_BSN_x							(*loopFilter_LumaV_BSN_universal         )
#define loopFilter_LumaH_BSN_x							(*loopFilter_LumaH_BSN_universal         )

void Init_AVC_Deblocking_C(int for_avc)
{
	//common
	ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal = ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c;
	ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c;

#ifndef DROP_MBAFF
	if( for_avc )
	{
		ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_universal =   ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_c;
		ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_universal = ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_c;
	}
#endif
	
	//can be replaced by optimized versions
#ifndef DROP_C_NO
	
	if( for_avc )
	{
		ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal = ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_c;
		ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal = ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c;
	}

	loopFilter_LumaV_BS4_with16pel_universal	= loopFilter_LumaV_BS4_with16pel_c;
	loopFilter_LumaV_BS4_with8pel_universal		= loopFilter_LumaV_BS4_with8pel_c;
	loopFilter_LumaH_BS4_with16pel_universal	= loopFilter_LumaH_BS4_with16pel_c;
	loopFilter_LumaV_BSN_universal				= loopFilter_LumaV_BSN_c;         
	loopFilter_LumaH_BSN_universal				= loopFilter_LumaH_BSN_c;

#if _KINOMA_LOSSY_OPT_
	loopFilter_LumaV_BS4_with16pel_sim_universal= loopFilter_LumaV_BS4_with16pel_simply_c;
	loopFilter_LumaH_BS4_with16pel_sim_universal= loopFilter_LumaH_BS4_with16pel_simply_c;
	loopFilter_LumaV_BSN_sim_universal          = loopFilter_LumaV_BSN_simply_c;
	loopFilter_LumaH_BSN_sim_universal          = loopFilter_LumaH_BSN_simply_c;
#endif

#endif
}

#ifdef __INTEL_IPP__

void Init_AVC_Deblocking_XScale(int for_avc)
{
	//common
	ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_universal = ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR;
	ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_universal = ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR;

#ifndef DROP_MBAFF
	if( for_avc )
	{
		ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_universal =   ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR;
		ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_universal = ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR;
	}
#endif
	
	if( for_avc )
	{
		ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal = ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR;
		ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal = ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR;
	}
}

#endif

void Init_AVC_Deblocking_ARM_V5(int for_avc)
{
#if defined(__KINOMA_IPP_ARM_V5__) && TARGET_CPU_ARM

	if( for_avc )
	{
		ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_universal = ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_arm;
		ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_universal = ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_arm;
	}

	loopFilter_LumaV_BS4_with16pel_universal	= loopFilter_LumaV_BS4_with16pel_arm;
	loopFilter_LumaV_BS4_with8pel_universal		= loopFilter_LumaV_BS4_with8pel_arm;
	loopFilter_LumaH_BS4_with16pel_universal	= loopFilter_LumaH_BS4_with16pel_arm;
	loopFilter_LumaV_BSN_universal				= loopFilter_LumaV_BSN_arm;         
	loopFilter_LumaH_BSN_universal				= loopFilter_LumaH_BSN_arm;         

#if _KINOMA_LOSSY_OPT_
	loopFilter_LumaV_BS4_with16pel_sim_universal= loopFilter_LumaV_BS4_with16pel_simply_arm;
	loopFilter_LumaH_BS4_with16pel_sim_universal= loopFilter_LumaH_BS4_with16pel_simply_arm;
	loopFilter_LumaV_BSN_sim_universal          = loopFilter_LumaV_BSN_simply_arm;         
	loopFilter_LumaH_BSN_sim_universal          = loopFilter_LumaH_BSN_simply_arm;         
#endif

#endif
}

////////////////////////////////  Need convert these eight functions into ASM code (they are used in C version ONLY) ///////////////////////////////////////
#ifndef DROP_C_NO
// BS=4
KINOMA_INLINE void loopFilter_LumaV_BS4(Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)
{
	Ipp8u	L2 , L1, L0, R0, R1, R2 , L3, R3;
	//unsigned int LL, RR;
	unsigned int     temp, RL0;
	int		FLAGs, gap;
	int		AbsDelta, Alpha22 = (Alpha >>2) +2;
	int		pel;

	for(pel =8; pel>0; pel--)
	{
		// Left
		L1  = SrcPtr[2]; L0  = SrcPtr[3];
		// Right
		R0  = SrcPtr[ 4]; R1  = SrcPtr[ 5];
		
		AbsDelta  = absm( R0 - L0 );

		// In ARM platform, we want to eliminate b instruction for they do not have delayed branch
		//FLAGs = (absm( R0 - R1) < Beta) && (absm(L0 - L1) < Beta) && (AbsDelta < Alpha);
		FLAGs = (absm( R0 - R1) < Beta) * (absm(L0 - L1) < Beta) * (AbsDelta < Alpha);

		if(FLAGs)
		{
			FLAGs = AbsDelta - Alpha22;
			RL0 = R0 + L0;
			//L2 = (LL>>8) & 0xFF ;	
			L2  = SrcPtr[1];
			gap = absm(L0 - L2) - Beta;
			if((FLAGs<0) && (gap <0))			// Compiler can not do this efficeint, so need use ASM code!!!
			{	
				int temp2; 
				//L3 = LL & 0xFF;	
				L3  = SrcPtr[0];
				temp = L1 + RL0 +2; 
				temp2 = L2 + temp;
				SrcPtr[1] = (((L3 + L2) <<1) + temp2 + 2) >> 3 ;
				SrcPtr[2] = (temp2) >> 2 ;
				SrcPtr[3] = (R1 + (temp << 1) +  L2) >> 3;
			}
			else
				SrcPtr[3] = ((L1 << 1) + L0 + R1 + 2) >> 2 ;
			
			//R2 = (RR>>16) & 0xFF;	
			R2  = SrcPtr[ 6];
			gap = absm( R0 - R2) - Beta;
			if((FLAGs<0) && (gap<0))
			{
				int temp2;
				//R3 = (RR>> 24) & 0xFF;	
				R3  = SrcPtr[ 7] ;
				temp = RL0 + R1 + 2;
				temp2 = temp + R2;
				SrcPtr[ 4] = ( L1 + (temp << 1) +  R2) >> 3 ;					              
				SrcPtr[ 5] = (temp2) >> 2 ;					              
				SrcPtr[ 6] = (((R3 + R2) <<1) + temp2 + 2) >> 3 ;
				}
			else
				SrcPtr[ 4] = ((R1 << 1) + R0 + L1 + 2) >> 2 ;

		}

		// Update
		SrcPtr += srcdstStep;
	}
}
void loopFilter_LumaH_BS4_with16pel_c(Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)
{
	Ipp8u	L2 , L1, L0, R0, R1, R2 , L3, R3;
	unsigned short RL0;
	int     AbsDelta, temp, pel;
	int		FLAGs, gap;

	int Alpha22 = (Alpha >> 2) +2;

	for(pel=16; pel> 0; pel--)
	{
		L1  = SrcPtr[-2*srcdstStep]; 	L0  = SrcPtr[-srcdstStep ];
		R0  = SrcPtr[ 0]; 	R1  = SrcPtr[ srcdstStep ];

		AbsDelta  = absm( R0 - L0 );

		FLAGs = (absm( R0 - R1) < Beta) * (absm(L0 - L1) < Beta) * (AbsDelta < Alpha);
		if(FLAGs)
		{
			FLAGs = AbsDelta - Alpha22;
			RL0 = L0 + R0 ;

			L2  = SrcPtr[-3*srcdstStep];

			
			gap = absm(L0 - L2) - Beta;
			
			if((FLAGs<0) && (gap <0))	
			{
				int temp2;
				L3  = SrcPtr[-4*srcdstStep];
				temp = L1 + RL0 + 2;
				temp2 = L2 + temp;
				SrcPtr[-3*srcdstStep] = (((L3 + L2) <<1) + temp2 + 2) >> 3 ;
				SrcPtr[-2*srcdstStep] = (temp2) >> 2 ;
				SrcPtr[-srcdstStep ] = ( R1 + (temp << 1) +  L2) >> 3 ;

			}
			else
				SrcPtr[-srcdstStep ] = ((L1 << 1) + L0 + R1 + 2) >> 2 ;

			R2  = SrcPtr[ 2*srcdstStep];
			gap = absm( R0 - R2) - Beta;
			if((FLAGs<0) && (gap <0))	
			{
				int temp2;
				R3  = SrcPtr[ 3*srcdstStep];
				temp = RL0 + R1 + 2;
				temp2 = temp + R2;
				SrcPtr[ 0          ] = ( L1 + (temp << 1) +  R2) >> 3 ;					              
				SrcPtr[ srcdstStep ] = (temp2) >> 2 ;					              
				SrcPtr[ 2*srcdstStep] = (((R3 + R2) <<1) + temp2 + 2) >> 3;
			}
			else
				SrcPtr[ 0          ] = ((R1 << 1) + R0 + L1 + 2) >> 2 ;	
		}

	// Update
	SrcPtr++;

	}
}

void loopFilter_LumaV_BS4_with16pel_c(Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)
{
	loopFilter_LumaV_BS4(SrcPtr, Alpha, Beta, srcdstStep);
	loopFilter_LumaV_BS4(SrcPtr+(srcdstStep<<3), Alpha, Beta, srcdstStep);
}
void loopFilter_LumaV_BS4_with8pel_c(Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)
{
	loopFilter_LumaV_BS4(SrcPtr, Alpha, Beta, srcdstStep);
}
// BS=1,2,3
void loopFilter_LumaV_BSN_c(Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep, Ipp8u C0)
{
	int		L2 , L1, L0, R0, R1, R2 , RL0;
	int     c0, Delta, dif, AbsDelta ;
	Ipp8u	*SrcPtr = pSrcPtr;
	int     i;
	int		FLAGs, temp;

	for(i=0; i<4; i++)
	{
		L1  = SrcPtr[ 2] ;
		L0  = SrcPtr[ 3] ;

		R0  = SrcPtr[ 4] ;
		R1  = SrcPtr[ 5] ;

		AbsDelta  = absm( Delta = R0 - L0 );
		FLAGs = (absm( R0 - R1) < Beta) * (absm(L0 - L1) < Beta) * (AbsDelta < Alpha);

		if(FLAGs)
		{
			RL0 = (L0 + R0+1)>>1;

			L2  = SrcPtr[ 1] ;
			c0 = C0;
			if( (absm( L0 - L2) < Beta ) )
			{
				temp = ( L2 + RL0  - (L1<<1)) >> 1;
				SrcPtr[ 2] += IClip( -C0,  C0,  temp) ;
				c0 ++;
			}

			R2  = SrcPtr[ 6] ;
			if( (absm( R0 - R2) < Beta )  )
			{
				temp = ( R2 + RL0  - (R1<<1)) >> 1;
				SrcPtr[ 5] += IClip( -C0,  C0,  temp) ;
				c0 ++;
			}

			temp = ((Delta << 2) + (L1 - R1) + 4) >> 3;
			dif  = IClip( -c0, c0,  temp) ;

			SrcPtr[  3]  = clip_uint8(L0 + dif) ;
			SrcPtr[  4]  = clip_uint8(R0 - dif) ;
		}

		SrcPtr += srcdstStep;
	}
}


void loopFilter_LumaH_BSN_c(Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep, Ipp8u C0)
{
	int		L2 , L1, L0, R0, R1, R2 , RL0;
	int     c0, Delta, dif, AbsDelta ;
	int     i;
	int		FLAGs;

	for(i=0; i<4; i++)
	{
		L1  = SrcPtr[-2*srcdstStep] ;
		L0  = SrcPtr[-srcdstStep ] ;

		R0  = SrcPtr[ 0] ;
		R1  = SrcPtr[ srcdstStep ] ;

		AbsDelta  = absm( Delta = R0 - L0 );
		FLAGs = (AbsDelta < Alpha) * (absm( R0 - R1) < Beta) * (absm(L0 - L1) < Beta);
		if(FLAGs)
		{
			L2  = SrcPtr[-3*srcdstStep] ;
			RL0 = (L0 + R0 +1) >>1;

			c0 = C0;
			if( absm( L0 - L2) < Beta )
			{
				SrcPtr[-2*srcdstStep] += IClip( -C0,  C0, ( L2 + RL0 - (L1<<1)) >> 1 ) ;
				c0 ++;
			}

			R2  = SrcPtr[ 2*srcdstStep] ;
			if( absm( R0 - R2) < Beta  )
			{
				SrcPtr[ srcdstStep ] += IClip( -C0,  C0, ( R2 + RL0  - (R1<<1)) >> 1 );
				c0 ++;
			}

			dif  = IClip( -c0, c0, ( (Delta << 2) + (L1 - R1) + 4) >> 3 ) ;
			SrcPtr[ -srcdstStep]  = clip_uint8(L0 + dif) ;
			SrcPtr[  0         ]  = clip_uint8(R0 - dif) ;						
		}

		SrcPtr ++;
	}
}

//////////////////////////////////  LOSSY OPT  /////////////////////////////////////
#ifdef _KINOMA_LOSSY_OPT_
void loopFilter_LumaV_BS4_with16pel_simply_c(Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)
{
	Ipp8u	L1, L0, R0, R1;
	int		FLAGs;
	int		AbsDelta; //, Alpha22 = (Alpha >>2) +2;
	int		pel;

	for(pel = 16; pel>0; pel--)
	{
		// Left
		L1  = SrcPtr[2]; L0  = SrcPtr[3];
		// Right
		R0  = SrcPtr[ 4]; R1  = SrcPtr[ 5];
		
		AbsDelta  = absm( R0 - L0 );

		// In ARM platform, we want to eliminate b instruction for they do not have delayed branch
		//FLAGs = (absm( R0 - R1) < Beta) && (absm(L0 - L1) < Beta) && (AbsDelta < Alpha);
		FLAGs = (absm( R0 - R1) < Beta) * (absm(L0 - L1) < Beta) * (AbsDelta < Alpha);

		if(FLAGs)
		{
			SrcPtr[3] = ((L1 << 1) + L0 + R1 + 2) >> 2;
			SrcPtr[4] = ((R1 << 1) + R0 + L1 + 2) >> 2;
		}

		// Update
		SrcPtr += srcdstStep;
	}
}


void loopFilter_LumaH_BS4_with16pel_simply_c(Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)
{
	Ipp8u	L1, L0, R0, R1;
	int     AbsDelta, pel;
	int		FLAGs;
	int     srcdstStep2 = -2*srcdstStep;

	//int Alpha22 = (Alpha >> 2) +2;

	for(pel=16; pel> 0; pel--)
	{
		L1  = SrcPtr[srcdstStep2]; 	L0  = SrcPtr[-srcdstStep ];
		R0  = SrcPtr[ 0]; 	R1  = SrcPtr[ srcdstStep ];

		AbsDelta  = absm( R0 - L0 );

		FLAGs = (absm( R0 - R1) < Beta) * (absm(L0 - L1) < Beta) * (AbsDelta < Alpha);

		if(FLAGs)
		{
			SrcPtr[-srcdstStep ] = ((L1 << 1) + L0 + R1 + 2) >> 2;
			SrcPtr[ 0          ] = ((R1 << 1) + R0 + L1 + 2) >> 2;			
		}

		// Update
		SrcPtr++;
	}

}

// BS=1,2,3
void loopFilter_LumaV_BSN_simply_c(Ipp8u *pSrcPtr,int Alpha, int Beta, int srcdstStep)
{
	int		L1, L0, R0, R1;
	int     AbsDelta ;
	Ipp8u	*SrcPtr = pSrcPtr;
	int     i;
	int		FLAGs;

	for(i=0; i<4; i++)
	{
		L1  = SrcPtr[ 2] ;
		L0  = SrcPtr[ 3] ;

		R0  = SrcPtr[ 4] ;
		R1  = SrcPtr[ 5] ;

		AbsDelta  = absm(R0 - L0 );
		FLAGs = (absm( R0 - R1) < Beta) * (absm(L0 - L1) < Beta) * (AbsDelta < Alpha);
		//FLAGs =  (AbsDelta < Alpha);

		if(FLAGs)
		{
			SrcPtr[3] = (L1  + (L0<<1) + R1 + 2) >> 2 ;
			SrcPtr[4] = (R1  + (R0<<1) + L1 + 2) >> 2;
		}

		SrcPtr += srcdstStep;
	}

}


void loopFilter_LumaH_BSN_simply_c(Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)
{
	int		L1, L0, R0, R1;
	int     AbsDelta ;
	int     i;
	int		FLAGs;
	int     srcdstStep2 = -2*srcdstStep;

	for(i=0; i<4; i++)
	{
		L1  = SrcPtr[srcdstStep2] ;
		L0  = SrcPtr[-srcdstStep ] ;

		R0  = SrcPtr[ 0] ;
		R1  = SrcPtr[ srcdstStep ] ;

		AbsDelta  = absm(R0 - L0 );
		FLAGs = (absm( R0 - R1) < Beta) * (absm(L0 - L1) < Beta) * (AbsDelta < Alpha);
		//FLAGs =  (AbsDelta < Alpha);

		if(FLAGs)
		{
			SrcPtr[-srcdstStep ] = (L1 + (L0<<1) + R1 + 2) >> 2;
			SrcPtr[ 0          ] = (R1 + (R0<<1) + L1 + 2) >> 2;
		}

		SrcPtr ++;
	}

}

#endif
//////////////////////////////// LOSSY OPT END  //////////////////////////////////////
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
KINOMA_INLINE void DeblockingLuma_VerEdge_BSN(Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep,  unsigned int BS, unsigned int Threshold)
{
	int pitch = 4*srcdstStep;

		// All BS can not be 4
		if(BS & 0xff)
		{
			loopFilter_LumaV_BSN_x(SrcPtr, Alpha, Beta, srcdstStep, Threshold&0xff);
		}

		SrcPtr += pitch;
		if((BS>>8) & 0xFF)
		{
			loopFilter_LumaV_BSN_x(SrcPtr, Alpha, Beta, srcdstStep, (Threshold>>8)&0xff);
		}

		SrcPtr += pitch;
		if((BS>>16) & 0xFF)
		{
			loopFilter_LumaV_BSN_x(SrcPtr, Alpha, Beta, srcdstStep, (Threshold>>16)&0xff);
		}

		SrcPtr += pitch;
		if((BS>>24))
		{
			loopFilter_LumaV_BSN_x(SrcPtr, Alpha, Beta, srcdstStep, (Threshold>>24));
		}
}
KINOMA_INLINE void DeblockingLuma_HorEdge_BSN(Ipp8u *SrcPtr, int Alpha, int Beta,  unsigned int BS_int, unsigned int Threshold, int srcdstStep)
{
		if(BS_int & 0xff)
		{
			loopFilter_LumaH_BSN_x(SrcPtr, Alpha, Beta, srcdstStep, Threshold&0xff);
		}

		SrcPtr += 4;
		if((BS_int>>8) & 0xFF)
		{
			loopFilter_LumaH_BSN_x(SrcPtr, Alpha, Beta, srcdstStep, (Threshold>>8)&0xff);
		}

		SrcPtr += 4;
		if((BS_int>>16) & 0xFF)
		{
			loopFilter_LumaH_BSN_x(SrcPtr, Alpha, Beta, srcdstStep, (Threshold>>16)&0xff);
		}

		SrcPtr += 4;
		if((BS_int>>24))
		{
			loopFilter_LumaH_BSN_x(SrcPtr, Alpha, Beta, srcdstStep, (Threshold>>24));
		}
}

#ifdef _KINOMA_LOSSY_OPT_
KINOMA_INLINE void DeblockingLuma_VerEdge_BSN_simple(Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep,  unsigned int BS)
{
	int pitch = 4*srcdstStep;

	// All BS can not be 4
	if(BS & 0xff)
	{
		loopFilter_LumaV_BSN_sim_universal(SrcPtr, Alpha, Beta, srcdstStep);
	}

	SrcPtr += pitch;
	if((BS>>8) & 0xFF)
	{
		loopFilter_LumaV_BSN_sim_universal(SrcPtr, Alpha, Beta, srcdstStep);
	}

	SrcPtr += pitch;
	if((BS>>16) & 0xFF)
	{
		loopFilter_LumaV_BSN_sim_universal(SrcPtr, Alpha, Beta, srcdstStep);
	}

	SrcPtr += pitch;
	if((BS>>24))
	{
		loopFilter_LumaV_BSN_sim_universal(SrcPtr, Alpha, Beta, srcdstStep);
	}
}
KINOMA_INLINE void DeblockingLuma_HorEdge_BSN_simple(Ipp8u *SrcPtr, int Alpha, int Beta,  unsigned int BS_int, int srcdstStep)
{
	if(BS_int & 0xff)
	{
		loopFilter_LumaH_BSN_sim_universal(SrcPtr, Alpha, Beta, srcdstStep);
	}

	SrcPtr += 4;
	if((BS_int>>8) & 0xFF)
	{
		loopFilter_LumaH_BSN_sim_universal(SrcPtr, Alpha, Beta, srcdstStep);
	}

	SrcPtr += 4;
	if((BS_int>>16) & 0xFF)
	{
		loopFilter_LumaH_BSN_sim_universal(SrcPtr, Alpha, Beta, srcdstStep);
	}

	SrcPtr += 4;
	if((BS_int>>24))
	{
		loopFilter_LumaH_BSN_sim_universal(SrcPtr, Alpha, Beta, srcdstStep);
	}
}


#endif
////////////////////////////////////////////////////////// Start Chroma /////////////////////////////////////////////////
// Chroma
KINOMA_INLINE void loopFilter_ChromaV_BS4(Ipp8u *SrcPtr, int Alpha, int Beta)
{
	int		L1, L0, R0, R1, RL0;
	int     AbsDelta ;

	L1  = SrcPtr[-2] ;
	L0  = SrcPtr[-1] ;
	R0  = SrcPtr[ 0] ;
	R1  = SrcPtr[ 1] ;
	//R2  = SrcPtr[ 2] ;

	AbsDelta  = absm(R0 - L0 );
	if((absm( R0 - R1) < Beta) * (absm(L0 - L1) < Beta) * (AbsDelta < Alpha))
	{
		RL0 = R1 + L1 +2;
		SrcPtr[ -1] = (RL0 + L0 + L1) >> 2; 
		SrcPtr[  0] = (RL0 + R0 + R1) >> 2; 
	}
}

KINOMA_INLINE void loopFilter_ChromaH_BS4(Ipp8u *SrcPtr, int Alpha, int Beta, int srcdstStep)
{
	int		L1, L0, R0, R1, tmpresult;
	int     AbsDelta ;

	L1  = SrcPtr[(-2)*srcdstStep];
	L0  = SrcPtr[-srcdstStep];

	R0  = SrcPtr[ 0         ];
	R1  = SrcPtr[ srcdstStep];

	AbsDelta  = absm((R0 - L0) );
	if((absm( R0 - R1) < Beta) * (absm(L0 - L1) < Beta) * (AbsDelta < Alpha))
	{
		tmpresult = R1 + L1 +2;
		SrcPtr[ -srcdstStep] = (tmpresult + L0 + L1) >> 2; 
		SrcPtr[  0         ] = (tmpresult + R0 + R1) >> 2; 
	}
}

// BS=1,2,3
KINOMA_INLINE void loopFilter_ChromaV_BSN(Ipp8u *SrcPtr, int Alpha, int Beta, int c0)
{
	int		L1, L0, R0, R1;
	int     AbsDelta, Delta, dif;

	L1  = SrcPtr[-2] ;
	L0  = SrcPtr[-1] ;
	R0  = SrcPtr[ 0] ;
	R1  = SrcPtr[ 1] ;

	AbsDelta  = absm( Delta = R0 - L0 );

	if((absm( R0 - R1) < Beta) * (absm(L0 - L1) < Beta) * (AbsDelta < Alpha))
	{
		dif   = IClip( -c0, c0, ( (Delta << 2) + (L1 - R1) + 4) >> 3 ) ;

		SrcPtr[ -1]  = clip_uint8(L0 + dif) ;
		SrcPtr[  0]  = clip_uint8(R0 - dif) ;
	}
}
KINOMA_INLINE void loopFilter_ChromaH_BSN(Ipp8u *SrcPtr, int Alpha, int Beta, int c0, int srcdstStep)
{
	int		L1, L0, R0, R1, RL0;
	int     AbsDelta, Delta, dif;

	L1  = SrcPtr[-2*srcdstStep];
	L0  = SrcPtr[-srcdstStep];

	R0  = SrcPtr[ 0         ];
	R1  = SrcPtr[ srcdstStep];

	AbsDelta  = absm( Delta = R0 - L0 );
	if((absm( R0 - R1) < Beta) * (absm(L0 - L1) < Beta) *(AbsDelta < Alpha))
	{
		RL0 = ( (Delta << 2) + (L1 - R1) + 4) >> 3 ;
		dif   = IClip( -c0, c0,RL0 ) ;

		SrcPtr[ -srcdstStep]  = clip_uint8(L0 + dif) ;
		SrcPtr[     0      ]  = clip_uint8(R0 - dif) ;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////  The following function are not need optimized any more ////////////////////////////
//                                                       And They are Interface functions in this file  //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 /*
 **************************************************************************
 * Function:    ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_x
 * Description: Do horizontal filter for Luma (16x16 block)
 *
 * 
 **************************************************************************
 */
/* Map of block4x4s in this function for one MB
 *   0  4  8   12
 *   1  5  9   13
 *   2  6  10  14
 *   3  7  11  15
 *
 */
/*
	Please note, we filter FOUR edge for each direction, But, any of them can be all ZERO (BS)
 */

IppStatus __STDCALL ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c(Ipp8u *pSrcDst,	Ipp32s srcdstStep, const Ipp8u *pAlpha,	const Ipp8u *pBeta, const Ipp8u *pThresholds, 	const Ipp8u* pBs)
{
	Profile_Start(ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c_profile);

#ifdef PROFILE_IPP
	IppStatus status = ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR(pSrcDst,	srcdstStep, pAlpha,	pBeta, pThresholds, pBs);

	Profile_End(ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c_profile);

	return status;
#endif

	// For, we use different QP for edge=0 and 1,2,3 (may different), we got different index for beta, alpha and clip
	//  But the process is identical
	unsigned int BS, Threshold;
	Ipp8u   *SrcPtr;
	int		Beta, Alpha;
	//int		pitch = 4*srcdstStep;

	// First, edge =0 External edge filter
	// If pBs[0] ==4, all four elements from 0 to 3 must be 4 for this external edge
	Beta  = pBeta[0];
	Alpha = pAlpha[0];

	SrcPtr = pSrcDst - 4;
	if(pBs[0] == 4)
	{
		loopFilter_LumaV_BS4_with16pel_x(SrcPtr, Alpha, Beta, srcdstStep);
	}
	else
	{		
		BS = *((int *)&pBs[0]);
		if(BS)
		{
		Threshold = *((int *)&pThresholds[0]);

		DeblockingLuma_VerEdge_BSN(SrcPtr, Alpha, Beta, srcdstStep, BS, Threshold);
		}
	}

	// Next, we process other three edge
	Beta  = pBeta[1];
	Alpha = pAlpha[1];	

	SrcPtr = pSrcDst;  // Do not add +4, for use left in subfunction  loopFilter_LumaV_BSN:
	BS = *((int *)&pBs[4]);
	if(BS)
	{
	Threshold = *((int *)&pThresholds[4]);
	DeblockingLuma_VerEdge_BSN(SrcPtr, Alpha, Beta, srcdstStep, BS, Threshold);
	}

	SrcPtr += 4; // Next 4 pixels (next edge)
	BS = *((int *)&pBs[8]);
	if(BS)
	{
	Threshold = *((int *)&pThresholds[8]);
	DeblockingLuma_VerEdge_BSN(SrcPtr, Alpha, Beta, srcdstStep, BS, Threshold);
	}


	SrcPtr += 4; // Next 4 pixels (next edge)
	BS = *((int *)&pBs[12]);
	if(BS)
	{
	Threshold = *((int *)&pThresholds[12]);
	DeblockingLuma_VerEdge_BSN(SrcPtr, Alpha, Beta, srcdstStep, BS, Threshold);
	}

	Profile_End(ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c_profile);

	return ippStsNoErr;
}
/*
 **************************************************************************
 * Function:    ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR
 * Description: Do horizontal filter for Luma (8x8 block)
 *
 * 
 **************************************************************************
 */
IppStatus __STDCALL ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c(Ipp8u *pSrcDst,	Ipp32s srcdstStep,	const Ipp8u *pAlpha,	const Ipp8u *pBeta,	const Ipp8u *pThresholds,	const Ipp8u *pBs)
{
	// For, we use different QP for edge=0 and 1,2,3 (may different), we got different index for beta, alpha and cllip
	//  But the process is identical
	unsigned int BS_int, Threshold;
	Ipp8u   *SrcPtr;
	Ipp8u	Beta, Alpha;

	Profile_Start(ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c_profile);

	// First, edge =0
	// External edge filter
	Beta  = pBeta[0];
	Alpha = pAlpha[0];	
	
	SrcPtr = pSrcDst;

	if(pBs[0] == 4)
	{
		loopFilter_LumaH_BS4_with16pel_x(SrcPtr, Alpha, Beta, srcdstStep);
	}
	else
	{
		BS_int = *((int *)&pBs[0]);
		if(BS_int)
		{
		Threshold = *((int *)&pThresholds[0]);
		DeblockingLuma_HorEdge_BSN(SrcPtr, Alpha, Beta, BS_int, Threshold, srcdstStep);
		}
	}

	// Next, we process other three edge
	Beta  = pBeta[1];
	Alpha = pAlpha[1];

	SrcPtr = pSrcDst + (srcdstStep<<2);  // Do not add +4, for use left in subfunction  loopFilter_LumaV_BSN:
	BS_int = *((int *)&pBs[4]);
	if(BS_int)
	{
	Threshold = *((int *)&pThresholds[4]);
	DeblockingLuma_HorEdge_BSN(SrcPtr, Alpha, Beta, BS_int, Threshold, srcdstStep);
	}

	SrcPtr += (srcdstStep<<2);
	BS_int = *((int *)&pBs[8]);
	if(BS_int)
	{
	Threshold = *((int *)&pThresholds[8]);
	DeblockingLuma_HorEdge_BSN(SrcPtr, Alpha, Beta, BS_int, Threshold, srcdstStep);
	}

	SrcPtr += (srcdstStep<<2);
	BS_int = *((int *)&pBs[12]);
	if(BS_int)
	{
	Threshold = *((int *)&pThresholds[12]);
	DeblockingLuma_HorEdge_BSN(SrcPtr, Alpha, Beta, BS_int, Threshold, srcdstStep);
	}

	Profile_End(ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c_profile);

	return ippStsNoErr;
}
#ifdef _KINOMA_LOSSY_OPT_
// Lossy optimization funcitons
IppStatus __STDCALL ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_simple(Ipp8u *pSrcDst,	Ipp32s srcdstStep, const Ipp8u *pAlpha,	const Ipp8u *pBeta, const Ipp8u *pThresholds, 	const Ipp8u* pBs)
{
	Profile_Start(ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c_profile);

	// For, we use different QP for edge=0 and 1,2,3 (may different), we got different index for beta, alpha and clip
	//  But the process is identical
	unsigned int BS;
	Ipp8u   *SrcPtr;
	int		Beta, Alpha;

	// First, edge =0 External edge filter
	// If pBs[0] ==4, all four elements from 0 to 3 must be 4 for this external edge
	Beta  = pBeta[0];
	Alpha = pAlpha[0];

	SrcPtr = pSrcDst - 4;
	if(pBs[0] == 4)
	{
		loopFilter_LumaV_BS4_with16pel_sim_universal(SrcPtr, Alpha, Beta, srcdstStep);
	}
	else
	{		
		BS = *((int *)&pBs[0]);
		if(BS)
		{
			DeblockingLuma_VerEdge_BSN_simple(SrcPtr, Alpha, Beta, srcdstStep, BS);
		}
	}

	// Next, we process other three edge
	Beta  = pBeta[1];
	Alpha = pAlpha[1];	

	SrcPtr = pSrcDst;  // Do not add +4, for use left in subfunction  loopFilter_LumaV_BSN:
	BS = *((int *)&pBs[4]);
	if(BS)
	{
		DeblockingLuma_VerEdge_BSN_simple(SrcPtr, Alpha, Beta, srcdstStep, BS);
	}

	BS = *((int *)&pBs[8]);
	if(BS)
	{
		DeblockingLuma_VerEdge_BSN_simple(SrcPtr+4, Alpha, Beta, srcdstStep, BS);
	}

	BS = *((int *)&pBs[12]);
	if(BS)
	{
		DeblockingLuma_VerEdge_BSN_simple(SrcPtr+8, Alpha, Beta, srcdstStep, BS);
	}

	Profile_End(ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c_profile);

	return ippStsNoErr;
}
IppStatus __STDCALL ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_simple(Ipp8u *pSrcDst,	Ipp32s srcdstStep,	const Ipp8u *pAlpha,	const Ipp8u *pBeta,	const Ipp8u *pThresholds,	const Ipp8u *pBs)
{
	// For, we use different QP for edge=0 and 1,2,3 (may different), we got different index for beta, alpha and cllip
	//  But the process is identical
	unsigned int BS_int;
	Ipp8u   *SrcPtr;
	Ipp8u	Beta, Alpha;
	int		srcdstStep4 = srcdstStep<<2;

	Profile_Start(ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c_profile);

	// First, edge =0
	// External edge filter
	Beta  = pBeta[0];
	Alpha = pAlpha[0];	
	
	SrcPtr = pSrcDst;

	if(pBs[0] == 4)
	{
		loopFilter_LumaH_BS4_with16pel_sim_universal(SrcPtr, Alpha, Beta, srcdstStep);
	}
	else
	{
		BS_int = *((int *)&pBs[0]);
		if(BS_int)
		{
			DeblockingLuma_HorEdge_BSN_simple(SrcPtr, Alpha, Beta, BS_int, srcdstStep);
		}
	}

	// Next, we process other three edge
	Beta  = pBeta[1];
	Alpha = pAlpha[1];

	SrcPtr = pSrcDst + srcdstStep4;  // Do not add +4, for use left in subfunction  loopFilter_LumaV_BSN:
	BS_int = *((int *)&pBs[4]);
	if(BS_int)
	{
		DeblockingLuma_HorEdge_BSN_simple(SrcPtr, Alpha, Beta, BS_int, srcdstStep);
	}

	BS_int = *((int *)&pBs[8]);
	if(BS_int)
	{
		DeblockingLuma_HorEdge_BSN_simple(SrcPtr + srcdstStep4, Alpha, Beta, BS_int, srcdstStep);
	}

	BS_int = *((int *)&pBs[12]);
	if(BS_int)
	{
		DeblockingLuma_HorEdge_BSN_simple(SrcPtr+ 2*srcdstStep4, Alpha, Beta, BS_int, srcdstStep);
	}

	Profile_End(ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c_profile);

	return ippStsNoErr;
}
#endif
/*
 **************************************************************************
 * Function:    ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_c
 * Description: Do vertical filter for Chroma (8x8 block)
 *
 * Problems   : Need to eliminate too much branch (If statements) -- WWD in 2006-06-10
 *              Need new method to load/save data more efficiently(byte)
 *
 * Notes      : This function use pBs[0-3, 4-7]
 **************************************************************************
 */
IppStatus __STDCALL ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_c(Ipp8u *pSrcDst, Ipp32s srcdstStep,const Ipp8u *pAlpha, const Ipp8u *pBeta, const Ipp8u *pThresholds, const Ipp8u *pBs)
{
	Profile_Start(ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_c_profile);

#if 0
	IppStatus status = ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_arm(pSrcDst, srcdstStep,pAlpha, pBeta, pThresholds, pBs);


	Profile_End(ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_c_profile);

	return status;
#else

	// For, we use different QP for edge=0 and 1,2,3 (may different), we got different index for beta, alpha and cllip
	//  But the process is identical
	int		bsIndex;
	Ipp8u   *SrcPtr;
	Ipp8u	Beta, Alpha, BS, c0;
	int		srcdstStep2 = srcdstStep<<1;


	// First, edge =0
	// External edge filter
	Beta  = pBeta[0];
	Alpha = pAlpha[0];

	SrcPtr = pSrcDst;
	BS = pBs[0];
	if(BS== 4)
	{
	for(bsIndex=0; bsIndex <4; bsIndex++)			// bsIndex change to 2x2 block for chroma
	{
		BS = pBs[bsIndex];

			loopFilter_ChromaV_BS4(SrcPtr, Alpha, Beta);
			SrcPtr += srcdstStep;

			loopFilter_ChromaV_BS4(SrcPtr, Alpha, Beta);
			SrcPtr += srcdstStep;
		}
	}
	else
	{
		for(bsIndex=0; bsIndex <4; bsIndex++)			// bsIndex change to 2x2 block for chroma
		{
			BS = pBs[bsIndex];
			if(BS != 0)
		{
			c0  = pThresholds[ bsIndex ] + 1;
			loopFilter_ChromaV_BSN(SrcPtr, Alpha, Beta, c0);
			SrcPtr += srcdstStep;

			loopFilter_ChromaV_BSN(SrcPtr, Alpha, Beta, c0);
			SrcPtr += srcdstStep;
		}
		else
			SrcPtr += srcdstStep2;
	}
	}

	// Next, we process other three edge
	Beta  = pBeta[1];
	Alpha = pAlpha[1];
	SrcPtr = pSrcDst + 4;
	for(bsIndex=0; bsIndex <4; bsIndex++)
	{
		BS = pBs[8 + bsIndex];
		if(BS)
		{		
			c0  = pThresholds[ 4 + bsIndex ] + 1;

			loopFilter_ChromaV_BSN(SrcPtr, Alpha, Beta, c0);
			SrcPtr += srcdstStep;

			loopFilter_ChromaV_BSN(SrcPtr, Alpha, Beta, c0);
			SrcPtr += srcdstStep;
		}
		else
			SrcPtr += srcdstStep2;
	}


	Profile_End(ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_c_profile);


#endif

	return ippStsNoErr;
}

/*
 **************************************************************************
 * Function:    ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c
 * Description: Do vertical filter for Chroma (8x8 block)
 *
 * Problems   : Need to eliminate too much branch (If statements) -- WWD in 2006-06-10
 *              Need new method to load/save data more efficiently(byte)
 *
 * Notes      : This function use pBs[0-3, 4-7]
 **************************************************************************
 */
IppStatus __STDCALL ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c(Ipp8u*  pSrcDst,Ipp32s  srcdstStep, const Ipp8u* pAlpha,const Ipp8u* pBeta, const Ipp8u* pThresholds,	const Ipp8u*  pBs)
{
	Profile_Start(ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c_profile);

#if 0
	IppStatus status = ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_arm(pSrcDst, srcdstStep,pAlpha, pBeta, pThresholds, pBs);


	Profile_End(ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c_profile);
	

	return status;
#else

	// For, we use different QP for edge=0 and 1,2,3 (may different), we got different index for beta, alpha and cllip
	//  But the process is identical
	int		bsIndex;
	Ipp8u   *SrcPtr;
	Ipp8u	Beta, Alpha, BS, c0;
	//int		srcdstStep2 = srcdstStep<<1;

	// First, edge =0
	// External edge filter
	Beta  = pBeta[0];
	Alpha = pAlpha[0];

	SrcPtr = pSrcDst;

	BS  = pBs[0];
	if(BS == 4)
	{
	for(bsIndex=0; bsIndex <4; bsIndex++)			// bsIndex change to 2x2 block for chroma
	{
		BS  = pBs[bsIndex];

			loopFilter_ChromaH_BS4(SrcPtr, Alpha, Beta, srcdstStep);
			SrcPtr++;

			loopFilter_ChromaH_BS4(SrcPtr, Alpha, Beta, srcdstStep);
			SrcPtr++;
		}
	}
	else
	{
		for(bsIndex=0; bsIndex <4; bsIndex++)			// bsIndex change to 2x2 block for chroma
		{
			BS  = pBs[bsIndex];
			if(BS !=0)
		{
			c0  = pThresholds[ bsIndex ] + 1;
			loopFilter_ChromaH_BSN(SrcPtr, Alpha, Beta, c0, srcdstStep);
			SrcPtr++;

			loopFilter_ChromaH_BSN(SrcPtr, Alpha, Beta, c0, srcdstStep);
			SrcPtr++;
		}
		
		else
			SrcPtr += 2;
	}
	}


	// Next, we process other three edge
	Beta  = pBeta[1];
	Alpha = pAlpha[1];
	SrcPtr = pSrcDst + (srcdstStep << 2);

	for(bsIndex=0; bsIndex <4; bsIndex++)			// bsIndex change to 2x2 block for chroma
	{
		if(pBs[8 + bsIndex] != 0)
		{
			BS = pBs[8 + bsIndex];
			c0  = pThresholds[ 4 + bsIndex ] + 1;

			loopFilter_ChromaH_BSN(SrcPtr, Alpha, Beta, c0, srcdstStep);
			SrcPtr ++;

			loopFilter_ChromaH_BSN(SrcPtr, Alpha, Beta, c0, srcdstStep);
			SrcPtr ++;
		}
		else
			SrcPtr += 2;
	}

	Profile_End(ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c_profile);

#endif

	return ippStsNoErr;
}

#ifndef DROP_MBAFF
IppStatus __STDCALL ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_c(Ipp8u *pSrcDst, Ipp32s srcdstStep,  Ipp32u nAlpha, Ipp32u nBeta, Ipp8u *pThresholds, Ipp8u *pBs)
{
	// For, we use different QP for edge=0 and 1,2,3 (may different), we got different index for beta, alpha and clip
	//  But the process is identical
	Ipp8u   *SrcPtr;
	int		Beta, Alpha;

	Profile_Start(ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_c_profile);

	// First, edge =0 External edge filter
	// If pBs[0] ==4, all four elements from 0 to 3 must be 4 for this external edge
	Beta  = nBeta;
	Alpha = nAlpha;

	SrcPtr = pSrcDst - 4;
	if(pBs[0] == 4)
	{
			loopFilter_LumaV_BS4_with8pel_x(SrcPtr, Alpha, Beta, srcdstStep);
	}
	else
	{
		unsigned int BS, Threshold;
		//int		pitch = 4*srcdstStep;
		// All BS can not be 4
		BS = *((int *)&pBs[0]);
		Threshold = *((int *)&pThresholds[0]);
		// All BS can not be 4
		DeblockingLuma_VerEdge_BSN(SrcPtr, Alpha, Beta, BS, srcdstStep, Threshold);
	}


	Profile_End(ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_c_profile);

	return ippStsNoErr;
}
/*
 **************************************************************************
 * Function:    ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c
 * Description: Do vertical filter for Chroma (8x8 block)
 *
 * Problems   : Need to eliminate too much branch (If statements) -- WWD in 2006-06-10
 *              Need new method to load/save data more efficiently(byte)
 *
 * Notes      : This function use pBs[0-3, 4-7]
 **************************************************************************
 */
IppStatus __STDCALL ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_c(Ipp8u* pSrcDst, Ipp32s srcdstStep,Ipp32u nAlpha, Ipp32u nBeta, Ipp8u*  pThresholds, Ipp8u* pBs)
{
	// For, we use different QP for edge=0 and 1,2,3 (may different), we got different index for beta, alpha and cllip
	//  But the process is identical
	int		bsIndex;
	Ipp8u   *SrcPtr;
	Ipp8u	Beta, Alpha, BS, c0;

	int		srcdstStep2 = srcdstStep<<1;

	Profile_Start(ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_c_profile);

	// First, edge =0
	// External edge filter
	Beta  = nBeta;
	Alpha = nAlpha;	
	SrcPtr = pSrcDst;

	for(bsIndex=0; bsIndex <4; bsIndex++)			// bsIndex change to 2x2 block for chroma
	{
		BS = pBs[bsIndex];

		if(BS )
		{
			c0  = pThresholds[ bsIndex ] + 1;

			if(BS == 4 )    // INTRA strong filtering
			{
				loopFilter_ChromaV_BS4(SrcPtr, Alpha, Beta);
				SrcPtr += srcdstStep;

				loopFilter_ChromaV_BS4(SrcPtr, Alpha, Beta);
				SrcPtr += srcdstStep;
			}
			else
			{	
				loopFilter_ChromaV_BSN(SrcPtr, Alpha, Beta, c0);
				SrcPtr += srcdstStep;

				loopFilter_ChromaV_BSN(SrcPtr, Alpha, Beta, c0);
				SrcPtr += srcdstStep;
			}
		}
		else
			SrcPtr += srcdstStep2;

	}

	Profile_End(ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_c_profile);

	return ippStsNoErr;
}

#endif

#if 0
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
|DeblockingLuma_VerEdge_BSN_arm| PROC
	stmdb       sp!, {r4 - r11, lr} 
	sub         sp, sp, #4
:   BS = [sp, #40]
:   Threshold =  [sp, #44]
	
	
	ldr		tmp,		[sp, #40]
	tst		tmp,		#0xFF
	beq		|end_1|
	mov		r4,	r0
	mov     r5, r1
	mov		r6, r2
	mov		r7, r3
	ldr		tmp2_left,	[sp, #44]	
	and	tmp2_left,	tmp2_left, #0xFF
	str	tmp2_left,	[sp]
	bl	loopFilter_LumaV_BSN_arm
	mov		r0,	r4
	mov		r1, r5
	mov	    r2, r6
	mov     r3, r7


|end_1|
	add		srcPtr, srcPtr, srcdstStep lsl #2
	ldr		tmp,		[sp, #40]
	tst		tmp,		#0xFF00
	beq     |end_2|
	mov		r4,	r0
	mov     r5, r1
	mov		r6, r2
	mov		r7, r3
	ldr		tmp2_left,	[sp, #44]	
	mov	tmp2_left,	tmp2_left lsr #8
	and	tmp2_left,  tmp2_left, #0xFF
	str	tmp2_left,	[sp]
	bl	loopFilter_LumaV_BSN_arm
	mov		r0,	r4
	mov		r1, r5
	mov	    r2, r6
	mov     r3, r7
	
|end_2|	
	add		srcPtr, srcPtr, srcdstStep lsl #2
	tst		tmp,		#0xFF0000
	beq     |end_3|
	ldr		tmp,		[sp, #40]
	mov		r4,	r0
	mov     r5, r1
	mov		r6, r2
	mov		r7, r3
	ldr		tmp2_left,	[sp, #44]	
	mov	tmp2_left,	tmp2_left lsr #16
	and	tmp2_left,  tmp2_left, #0xFF
	str	tmp2_left,	[sp]
	bl	loopFilter_LumaV_BSN_arm
	mov		r0,	r4
	mov		r1, r5
	mov	    r2, r6
	mov     r3, r7
	
|end_3|	
	add		srcPtr, srcPtr, srcdstStep lsl #2
	ldr		tmp,		[sp, #40]
	tst		tmp,		#0xFF000000
	beq     |end_4|
	mov		r4,	r0
	mov     r5, r1
	mov		r6, r2
	mov		r7, r3
	ldr		tmp2_left,	[sp, #44]	
	mov	tmp2_left,	tmp2_left lsr #24
	and	tmp2_left,  tmp2_left, #0xFF
	str	tmp2_left,	[sp]
	bl	loopFilter_LumaV_BSN_arm
	mov		r0,	r4
	mov		r1, r5
	mov	    r2, r6
	mov     r3, r7		
	
|end_4|
|VNN_end_function|
	add		sp, sp, #4
	ldmia       sp!, {r4 - r11, pc} 
	ENDP  ; |DeblockingLuma_VerEdge_BSN_arm|
#endif
#endif
