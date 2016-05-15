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
/***********************************************************************************
 * Limitation: For Intel decoder do not support AFF mode, current deblocking function do
 *             NOT support corresponding operation also.
 ***********************************************************************************
 */
#include <stdio.h>
//#include <math.h>
#include <assert.h>

#ifdef __KINOMA_IPP__

#include "kinoma_avc_defines.h"

#include "ippvc.h"

// We should make a decision in this function if external edge need filter
//   decision condition is : pBs[0--3] is ZERO we do need filter, otherwise we need do it
// Please Note: 16 elments should be used for 4 vertical edge, each can use 4 of them (one 4x4 block use one element)/*
 /*
 **************************************************************************
 * Function:    ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR
 * Description: Do horizontal filter for Luma (8x8 block)
 *
 * Problems   : I eliminate some if statements in 2006-06-18
 *
 * 
 **************************************************************************
 */
IppStatus __STDCALL ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR(
	Ipp8u*    pSrcDst,
	Ipp32s    srcdstStep,
	Ipp8u*    pAlpha,
	Ipp8u*    pBeta,
	Ipp8u*    pThresholds,
	Ipp8u*    pBs
	)
{
	// For, we use different QP for edge=0 and 1,2,3 (may different), we got different index for beta, alpha and cllip
	//  But the process is identical
	int		edge, pel,  ap = 0, aq = 0;

	int		L2 , L1, L0, R0, R1, R2 , RL0, L3, R3 ;
	Ipp8u   *SrcPtr;

	int		bsIndex;

	int		Beta, Alpha;

	int      C0, c0, Delta, dif, AbsDelta ;
	int		 Alpha22;

	// First, edge =0 External edge filter
	// If pBs[0] ==4, all four elements ifrom 0 to 3 must be 4. Bu this is not correct for "0"
	Beta  = pBeta[0];
	Alpha = pAlpha[0];
	Alpha22 = (Alpha >>2) +2;

	SrcPtr = pSrcDst;
	if(pBs[0] == 4)
	{
		// All BS shoule be 4
		for(pel= 16; pel> 0; pel--)
		{
			L3  = SrcPtr[-4];
			L2  = SrcPtr[-3] ;
			L1  = SrcPtr[-2] ;
			L0  = SrcPtr[-1] ;

			R0  = SrcPtr[ 0] ;
			R1  = SrcPtr[ 1] ;
			R2  = SrcPtr[ 2] ;
			R3  = SrcPtr[ 3] ;
			
			AbsDelta  = absm( R0 - L0 );

			if((absm( R0 - R1) < Beta)&& (absm(L0 - L1) < Beta) && (AbsDelta < Alpha))
			{
				if(AbsDelta < Alpha22)
				{
					RL0 = L0 + R0 ;
		    
					if((absm( L0 - L2) < Beta ))
					{
						SrcPtr[-3] = (((L3 + L2) <<1) + L2 + L1 + RL0 + 4) >> 3 ;
						SrcPtr[-2] = ( L2 + L1 + RL0 + 2) >> 2 ;
						SrcPtr[-1] = ( R1 + ((L1 + RL0) << 1) +  L2 + 4) >> 3;
					}
					else
						SrcPtr[-1] = ((L1 << 1) + L0 + R1 + 2) >> 2 ;


					if((absm( R0 - R2) < Beta ))
					{
						

						SrcPtr[ 0] = ( L1 + ((R1 + RL0) << 1) +  R2 + 4) >> 3 ;					              
						SrcPtr[ 1] = ( R2 + RL0 + R1 + 2) >> 2 ;					              
						SrcPtr[ 2] = (((R3 + R2) <<1) + R2 + R1 + RL0 + 4) >> 3 ;
					}
					else
						SrcPtr[ 0] = ((R1 << 1) + R0 + L1 + 2) >> 2 ;
				}
				else
				{
					SrcPtr[-1] = ((L1 << 1) + L0 + R1 + 2) >> 2 ;
					SrcPtr[ 0] = ((R1 << 1) + R0 + L1 + 2) >> 2 ;

				}
			}
			SrcPtr += srcdstStep;
		}
	}
	else
	{
		// All BS can not be 4
		for(bsIndex=0; bsIndex <4; bsIndex++)
		{
			if(pBs[bsIndex] != 0)
			{
				C0  = pThresholds[ bsIndex ];

				for(pel=0; pel< 4; pel++)
				{
					L2  = SrcPtr[-3] ;
					L1  = SrcPtr[-2] ;
					L0  = SrcPtr[-1] ;

					R0  = SrcPtr[ 0] ;
					R1  = SrcPtr[ 1] ;
					R2  = SrcPtr[ 2] ;

					AbsDelta  = absm( Delta = R0 - L0 );
					if((AbsDelta < Alpha) && (absm( R0 - R1) < Beta)&& (absm(L0 - L1) < Beta))
					{
						RL0 = ((L0 + R0)+1)>>1 ;

						c0 = C0;
						if( (absm( L0 - L2) < Beta ) )
						{
							SrcPtr[-2] += IClip( -C0,  C0, ( L2 + RL0  - (L1<<1)) >> 1 ) ;
							c0 ++;

						}
						if( (absm( R0 - R2) < Beta )  )
						{
							SrcPtr[ 1] += IClip( -C0,  C0, ( R2 + RL0  - (R1<<1)) >> 1 ) ;
							c0 ++;

						}

						dif  = IClip( -c0, c0, ( (Delta << 2) + (L1 - R1) + 4) >> 3 ) ;

						SrcPtr[ -1]  = clip_uint8(L0 + dif) ;
						SrcPtr[  0]  = clip_uint8(R0 - dif) ;
					}

					SrcPtr += srcdstStep;
				}
			}
			else
				SrcPtr += 4*srcdstStep;

		}
	}

	// Next, we process other three edge
	Beta  = pBeta[1];
	Alpha = pAlpha[1];	

	//SrcPtr = pSrcDst + 4;
	for(edge =4; edge< 16; edge+=4)
	{
		pSrcDst += 4;
		SrcPtr = pSrcDst;

		for(bsIndex=0; bsIndex <4; bsIndex++)
		{
			//SrcPtr = pSrcDst +   bsIndex*4*srcdstStep;
			if(pBs[edge + bsIndex] != 0)
			{
				C0  = pThresholds[ edge + bsIndex ];

				for(pel=0; pel<4; pel++)
				{										
					L2  = SrcPtr[-3];
					L1  = SrcPtr[-2];
					L0  = SrcPtr[-1];

					R0  = SrcPtr[ 0];
					R1  = SrcPtr[ 1];
					R2  = SrcPtr[ 2];

					AbsDelta  = absm( Delta = R0 - L0 );
					if((AbsDelta < Alpha) && (absm( R0 - R1) < Beta)&& (absm(L0 - L1) < Beta))
					{
						RL0 = ((L0 + R0)+1)>>1 ;

						c0 = C0;
						if( (absm( L0 - L2) < Beta ) )
						{
							SrcPtr[-2] += IClip( -C0,  C0, ( L2 + RL0  - (L1<<1)) >> 1 ) ;
							c0 ++;

						}
						if( (absm( R0 - R2) < Beta )  )
						{
							SrcPtr[ 1] += IClip( -C0,  C0, ( R2 + RL0  - (R1<<1)) >> 1 ) ;
							c0 ++;

						}

						dif  = IClip( -c0, c0, ( (Delta << 2) + (L1 - R1) + 4) >> 3 ) ;

						SrcPtr[ -1]  = clip_uint8(L0 + dif) ;
						SrcPtr[  0]  = clip_uint8(R0 - dif) ;
					}

					SrcPtr += srcdstStep;
				}
			}
			else
				SrcPtr += 4*srcdstStep;

		}
		//SrcPtr += 4;
	}

	return ippStsNoErr;
}
/*
 **************************************************************************
 * Function:    ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR
 * Description: Do horizontal filter for Luma (8x8 block)
 *
 * Problems   : Need to eliminate too much branch (If statements) -- WWD in 2006-06-10
 *
 * 
 **************************************************************************
 */
IppStatus __STDCALL ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR(
	Ipp8u*    pSrcDst,
	Ipp32s    srcdstStep,
	Ipp8u*    pAlpha,
	Ipp8u*    pBeta,
	Ipp8u*    pThresholds,
	Ipp8u*    pBs
	)
{
	// For, we use different QP for edge=0 and 1,2,3 (may different), we got different index for beta, alpha and cllip
	//  But the process is identical
	int		edge, pel,  ap = 0, aq = 0;

	int		L2, L1, L0, R0, R1, R2, L3, R3, RL0 ;
	Ipp8u   *SrcPtr;

	int		Beta, Alpha;
	int		BS;
	int		bsIndex;	// from 0--3; from 4--15

	int      C0, c0, Delta, dif, AbsDelta ;
	int      small_gap;

	int		srcdstStep2= srcdstStep<<1, srcdstStep3 = (srcdstStep<<1) +srcdstStep, srcdstStep4 = srcdstStep<<2;

	int		Alpha22;

	// First, edge =0
	// External edge filter
	Beta  = pBeta[0];
	Alpha = pAlpha[0];	
	
	Alpha22 = (Alpha >> 2) +2;
	SrcPtr = pSrcDst;

	if(pBs[0] == 4)
	{
		for(pel=16; pel> 0; pel--)
		{
			L3  = SrcPtr[-srcdstStep4];
			L2  = SrcPtr[-srcdstStep3];
			L1  = SrcPtr[-srcdstStep2];
			L0  = SrcPtr[-srcdstStep ];

			R0  = SrcPtr[ 0];
			R1  = SrcPtr[ srcdstStep ];
			R2  = SrcPtr[ srcdstStep2];
			R3  = SrcPtr[ srcdstStep3];

			AbsDelta  = absm( R0 - L0 );

			if((absm( R0 - R1) < Beta)&& (absm(L0 - L1) < Beta)&& (AbsDelta < Alpha))
			{
				aq  = absm( R0 - R2) < Beta;
				ap  = absm( L0 - L2) < Beta;

				RL0 = L0 + R0 ;

				small_gap = (AbsDelta < Alpha22);
				
				aq &= small_gap;
				ap &= small_gap;
				
				if(ap)
				{
					SrcPtr[-srcdstStep3] = (((L3 + L2) <<1) + L2 + L1 + RL0 + 4) >> 3 ;
					SrcPtr[-srcdstStep2] = ( L2 + L1 + RL0 + 2) >> 2 ;
					SrcPtr[-srcdstStep ] = ( R1 + ((L1 + RL0) << 1) +  L2 + 4) >> 3 ;

				}
				else
					SrcPtr[-srcdstStep ] = ((L1 << 1) + L0 + R1 + 2) >> 2 ;

				if(aq)
				{
					SrcPtr[ 0          ] = ( L1 + ((R1 + RL0) << 1) +  R2 + 4) >> 3 ;					              
					SrcPtr[ srcdstStep ] = ( R2 + RL0 + R1 + 2) >> 2 ;					              
					SrcPtr[ srcdstStep2] = (((R3 + R2) <<1) + R2 + R1 + RL0 + 4) >> 3;

				}
				else
					SrcPtr[ 0          ] = ((R1 << 1) + R0 + L1 + 2) >> 2 ;	

			}

			SrcPtr ++;
		}

	}
	else
	{
		for(bsIndex=0; bsIndex <4; bsIndex++)
		{
			if(pBs[bsIndex])
			{
				C0  = pThresholds[bsIndex ];
				// normal filtering
				{
					for(pel=0; pel<4; pel++)
					{
						L2  = SrcPtr[-srcdstStep3] ;
						L1  = SrcPtr[-srcdstStep2] ;
						L0  = SrcPtr[-srcdstStep ] ;

						R0  = SrcPtr[ 0] ;
						R1  = SrcPtr[ srcdstStep ] ;
						R2  = SrcPtr[ srcdstStep2] ;

						AbsDelta  = absm( Delta = R0 - L0 );
						if((AbsDelta < Alpha) && (absm( R0 - R1) < Beta)&& (absm(L0 - L1) < Beta))
						{
							aq  = absm( R0 - R2) < Beta;
							ap  = absm( L0 - L2) < Beta;

							RL0 = (L0 + R0 +1) >>1;


							c0   = (C0 + ap + aq) ;
							dif  = IClip( -c0, c0, ( (Delta << 2) + (L1 - R1) + 4) >> 3 ) ;

							if( ap )
							{
								SrcPtr[-srcdstStep2] += IClip( -C0,  C0, ( L2 + RL0 - (L1<<1)) >> 1 ) ;

							}
							SrcPtr[ -srcdstStep]  = clip_uint8(L0 + dif) ;
							SrcPtr[  0         ]  = clip_uint8(R0 - dif) ;						

							if( aq  )
							{
								SrcPtr[ srcdstStep ] += IClip( -C0,  C0, ( R2 + RL0  - (R1<<1)) >> 1 );
							}
						}

						SrcPtr ++;
					}
				}
			}
			else
				SrcPtr += 4;

		}

	}
	// Next, we process other three edge
	Beta  = pBeta[1];
	Alpha = pAlpha[1];
	Alpha22 = (Alpha >> 2) +2;

	//SrcPtr = pSrcDst + srcdstStep4;
	for(edge = 4; edge< 16; edge+=4)
	{
		SrcPtr = pSrcDst + edge*srcdstStep  ;

		for(bsIndex=0; bsIndex <4; bsIndex++)
		{
			BS = pBs[edge + bsIndex];

			if(BS)
			{
				C0  = pThresholds[ edge + bsIndex ];

				for(pel=0; pel<4; pel++)
				{

					L3  = SrcPtr[-srcdstStep4] ;
					L2  = SrcPtr[-srcdstStep3] ;
					L1  = SrcPtr[-srcdstStep2] ;
					L0  = SrcPtr[-srcdstStep ] ;

					R0  = SrcPtr[ 0] ;
					R1  = SrcPtr[ srcdstStep ] ;
					R2  = SrcPtr[ srcdstStep2] ;
					R3  = SrcPtr[ srcdstStep3] ;

					AbsDelta  = absm( Delta = R0 - L0 );
					// This statement is time comsume!!!!!!-- WWD in 2006-06-15
					if( (absm( R0 - R1) < Beta ) && (absm(L0 - L1) < Beta ) && (AbsDelta < Alpha ) )
					{
						aq  = (absm( R0 - R2) < Beta );
						ap  = (absm( L0 - L2) < Beta );

						RL0  = (L0 + R0 +1) >> 1;

						c0   = (C0 + ap + aq) ;
						dif  = IClip( -c0, c0, ( (Delta << 2) + (L1 - R1) + 4) >> 3 ) ;

						if( ap )
						{
							SrcPtr[-srcdstStep2] += IClip( -C0,  C0, ( L2 + RL0 - (L1<<1)) >> 1 );
						}

						SrcPtr[ -srcdstStep]  = clip_uint8(L0 + dif) ;
						SrcPtr[  0         ]  = clip_uint8(R0 - dif) ;

						if( aq  )
						{
							SrcPtr[ srcdstStep ] += IClip( -C0,  C0, ( R2 + RL0 - (R1<<1)) >> 1 );
						}
					}
					
					SrcPtr ++;
				}

			}
			else
				SrcPtr += 4;
		}

		//SrcPtr += srcdstStep4;

	}
	return ippStsNoErr;
}


/*
 **************************************************************************
 * Function:    ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR
 * Description: Do vertical filter for Chroma (8x8 block)
 *
 * Problems   : Need to eliminate too much branch (If statements) -- WWD in 2006-06-10
 *              Need new method to load/save data more efficiently(byte)
 *
 * 
 **************************************************************************
 */
IppStatus __STDCALL ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR(
	Ipp8u*    pSrcDst,
	Ipp32s    srcdstStep,
	Ipp8u*    pAlpha,
	Ipp8u*    pBeta,
	Ipp8u*    pThresholds,
	Ipp8u*    pBs
	)
{
	// For, we use different QP for edge=0 and 1,2,3 (may different), we got different index for beta, alpha and cllip
	//  But the process is identical
	int		pel;

	int		L1, L0, R0, R1;

	Ipp8u   *SrcPtr;

	int		tmpresult;

	int		Beta, Alpha;
	int		BS;
	int		bsIndex;	// from 0--1; from 2--3

	int     c0, Delta, dif, AbsDelta ;
	int		srcdstStep2 = srcdstStep<<1;

	// First, edge =0
	// External edge filter
	Beta  = pBeta[0];
	Alpha = pAlpha[0];	
	SrcPtr = pSrcDst;

	for(bsIndex=0; bsIndex <4; bsIndex++)			// bsIndex change to 2x2 block for chroma
	{
		BS = pBs[bsIndex];

		if(BS )
		{
			c0  = pThresholds[ bsIndex ] + 1;

			if(BS == 4 )    // INTRA strong filtering
			{
				for(pel=2; pel>0; pel--)
				{				
					L1  = SrcPtr[-2] ;
					L0  = SrcPtr[-1] ;
					R0  = SrcPtr[ 0] ;
					R1  = SrcPtr[ 1] ;
					//R2  = SrcPtr[ 2] ;

					AbsDelta  = absm( Delta = R0 - L0 );
					if((absm( R0 - R1) < Beta)&& (absm(L0 - L1) < Beta) && (AbsDelta < Alpha))
					{
						tmpresult = R1 + L1 +2;
						SrcPtr[ -1] = L0 = (tmpresult + L0 + L1) >> 2; 
						SrcPtr[  0] = R0 = (tmpresult + R0 + R1) >> 2; 
					}

					SrcPtr += srcdstStep;
				}
			}
			else
			{
				for(pel=2; pel>0; pel--)
				{				
					L1  = SrcPtr[-2] ;
					L0  = SrcPtr[-1] ;
					R0  = SrcPtr[ 0] ;
					R1  = SrcPtr[ 1] ;

					AbsDelta  = absm( Delta = R0 - L0 );

					if((absm( R0 - R1) < Beta)&& (absm(L0 - L1) < Beta) && (AbsDelta < Alpha))
					{
						dif   = IClip( -c0, c0, ( (Delta << 2) + (L1 - R1) + 4) >> 3 ) ;

						SrcPtr[ -1]  = clip_uint8(L0 + dif) ;
						SrcPtr[  0]  = clip_uint8(R0 - dif) ;
					}

					SrcPtr += srcdstStep;
				}
			}
		}
		else
			SrcPtr += srcdstStep2;

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

			for(pel=0; pel<2; pel++)
			{
				L1  = SrcPtr[-2] ;
				L0  = SrcPtr[-1] ;
				R0  = SrcPtr[ 0] ;
				R1  = SrcPtr[ 1] ;

				AbsDelta  = absm( Delta = R0 - L0 ) ;				


				if( (absm( R0 - R1) < Beta )  && (absm(L0 - L1) < Beta ) &&( AbsDelta < Alpha )  ) 
				{
					dif   = IClip( -c0, c0, ( (Delta << 2) + (L1 - R1) + 4) >> 3 ) ;

					SrcPtr[ -1]  = clip_uint8(L0 + dif) ;
					SrcPtr[  0]  = clip_uint8(R0 - dif) ;
				}

				SrcPtr += srcdstStep;
			}

		}
		else
			SrcPtr += srcdstStep2;

	}
	return ippStsNoErr;
}

/*
 **************************************************************************
 * Function:    ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR
 * Description: Do horizontal filter for Chroma (8x8 block)
 *
 * Problems   : Need to eliminate too much branch (If statements) -- WWD in 2006-06-10
 *
 * 
 **************************************************************************
 */
// This is 2006-06-30 Version
	IppStatus __STDCALL ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR(
	Ipp8u*    pSrcDst,
	Ipp32s    srcdstStep,
	Ipp8u*    pAlpha,
	Ipp8u*    pBeta,
	Ipp8u*    pThresholds,
	Ipp8u*    pBs
	)
{
	// For, we use different QP for edge=0 and 1,2,3 (may different), we got different index for beta, alpha and cllip
	//  But the process is identical
	int		pel;

	int		L1, L0, R0, R1;
	Ipp8u   *SrcPtr;

	int		tmpresult;
	int		srcdstStep2 = srcdstStep<<1;

	int		Beta, Alpha;
	int		BS;
	int		bsIndex;	// from 0--3; from 4--7

	int      c0, Delta, dif, AbsDelta ;

	// First, edge =0
	// External edge filter
	Beta  = pBeta[0];
	Alpha = pAlpha[0];

	SrcPtr = pSrcDst;

	for(bsIndex=0; bsIndex <4; bsIndex++)			// bsIndex change to 2x2 block for chroma
	{
		if(pBs[bsIndex] != 0)
		{
			BS  = pBs[bsIndex];
			c0  = pThresholds[ bsIndex ] + 1;

			if(BS == 4 )
			{
				for(pel=0; pel<2; pel++)
				{
					L1  = SrcPtr[-srcdstStep2];
					L0  = SrcPtr[-srcdstStep];

					R0  = SrcPtr[ 0         ];
					R1  = SrcPtr[ srcdstStep];

					AbsDelta  = absm( Delta = R0 - L0 );
					if((absm( R0 - R1) < Beta)&& (absm(L0 - L1) < Beta) &&(AbsDelta < Alpha))
					{
						tmpresult = R1 + L1 +2;
						SrcPtr[ -srcdstStep] = (tmpresult + L0 + L1) >> 2; 
						SrcPtr[  0         ] = (tmpresult + R0 + R1) >> 2; 
					}

					SrcPtr++;
				}
			}
			else
			{
				for(pel=0; pel<2; pel++)
				{
					L1  = SrcPtr[-srcdstStep2];
					L0  = SrcPtr[-srcdstStep];

					R0  = SrcPtr[ 0         ];
					R1  = SrcPtr[ srcdstStep];

					AbsDelta  = absm( Delta = R0 - L0 );
					if((absm( R0 - R1) < Beta)&& (absm(L0 - L1) < Beta) &&(AbsDelta < Alpha))
					{
						tmpresult = ( (Delta << 2) + (L1 - R1) + 4) >> 3 ;
						dif   = IClip( -c0, c0,tmpresult ) ;

						SrcPtr[ -srcdstStep]  = clip_uint8(L0 + dif) ;
						SrcPtr[     0      ]  = clip_uint8(R0 - dif) ;
					}
					SrcPtr++;
				}
			}
		}
		else
			SrcPtr += 2;

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

			for(pel=0; pel<2; pel++)
			{			
				L1  = SrcPtr[-srcdstStep2];
				L0  = SrcPtr[-srcdstStep];

				R0  = SrcPtr[ 0         ];
				R1  = SrcPtr[ srcdstStep];

				AbsDelta  = absm( Delta = R0 - L0 )  ;				

				if( (absm( R0 - R1) < Beta ) && (absm(L0 - L1) < Beta ) && ( AbsDelta < Alpha ))
				{
					tmpresult = ( (Delta << 2) + (L1 - R1) + 4) >> 3;
					dif    = IClip( -c0, c0,tmpresult  ) ;


					SrcPtr[ -srcdstStep]  = clip_uint8(L0 + dif);
					SrcPtr[  0         ]  = clip_uint8(R0 - dif);
				}
				SrcPtr ++;
			}
		}
		else
			SrcPtr += 2;
	}

	return ippStsNoErr;
}
#endif
