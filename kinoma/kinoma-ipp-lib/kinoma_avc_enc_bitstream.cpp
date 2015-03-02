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
#include "ippvc.h"
#include "kinoma_avc_defines.h"
#include "kinoma_ipp_lib.h"

IppStatus  (__STDCALL *ippiEncodeCoeffsCAVLC_H264_16s_universal)						(const Ipp16s* pSrc, Ipp8u   AC, const Ipp32s  *pScanMatrix, Ipp8u Count,  Ipp8u   *Trailing_Ones,Ipp8u   *Trailing_One_Signs, Ipp8u   *NumOutCoeffs, Ipp8u   *TotalZeros,  Ipp16s  *Levels, Ipp8u   *Runs )=NULL;
IppStatus  (__STDCALL *ippiEncodeChromaDcCoeffsCAVLC_H264_16s_universal)				(const Ipp16s* pSrc, Ipp8u   *Trailing_Ones, Ipp8u   *Trailing_One_Signs, Ipp8u   *NumOutCoeffs,  Ipp8u   *TotalZeros,  Ipp16s  *Levels,  Ipp8u   *Runs)=NULL;

#ifdef __KINOMA_IPP__
// Same as writeCoeff4x4_CAVLC in function side
IppStatus  __STDCALL   ippiEncodeCoeffsCAVLC_H264_16s_c(
      const Ipp16s* pSrc,
      Ipp8u   AC,
      const Ipp32s  *pScanMatrix,
      Ipp8u    Count,
      Ipp8u   *Trailing_Ones,
      Ipp8u   *Trailing_One_Signs,
      Ipp8u   *NumOutCoeffs,
      Ipp8u   *TotalZeros,
      Ipp16s  *Levels,
      Ipp8u   *Runs )
{
	short	scanedArray[16];
	short	pLevel[16];

	int		level,run;
	int		numcoeff, numtrailingones, totzeros, trainling=1;

	int		i,k, j;

	*Trailing_Ones = 0;
	*NumOutCoeffs  = 0;
	*TotalZeros    = 0;
	*Trailing_One_Signs = 0;

	numcoeff = 0;
	numtrailingones = 0;
	totzeros = 0;

	// Count indicate noz-zero coeficients in current Block (It is the last non-zero coeff in scan order)
	// NOT real count, it is count+1
	for(i=0; i<=Count; i++)
	{
		scanedArray[i] = pSrc[pScanMatrix[i]];
	}

	level = 1;
	run =0;
	// AC indicate scanedArray[0] can be compressed or not
	// except numtrailingones -- levels
	// inlucde trainling --- runs

	for(i=0/*index of Runs*/, j=0/*Index of Level */, k = Count; k>= AC; k--)
	{
		if(scanedArray[k])
		{
			level = scanedArray[k]; // level
			if (((level == 1) || (level ==-1)) && numtrailingones<3 && trainling)
			{

				// Recode sign for ones
				// 这里的最右面的位表示按照正常循序先得到的+1或者-1。因为这里是反序扫描，所以正好相反
				// 

				pLevel[numtrailingones] = level;
				numtrailingones ++;

				//if(level < 0)
				//	*Trailing_One_Signs = *Trailing_One_Signs | (1<< numtrailingones);
			}
			else
			{
				Levels[j++] = level; /*Please notes we do not recoder +1/-1 for levele*/
				trainling = 0;
			}

			if (run)
				totzeros += run;

			if(k != Count)
				Runs[i++]	=	run;					// run
			run = 0;

			numcoeff ++;
		}
		else
			run ++;
	}

	// The last one
	Runs[i]	=	run;

	*Trailing_Ones = numtrailingones;
	*NumOutCoeffs  = numcoeff;
	*TotalZeros    = totzeros + run /*The last one if it is DC is ZERO */;
	for(i=0; i< numtrailingones; i++)
	{
		if(pLevel[i] < 0)
			*Trailing_One_Signs = *Trailing_One_Signs | (1<< (numtrailingones-i-1));


	}

	return ippStsNoErr;
}

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiEncodeChromaDcCoeffsCAVLC_H264_16s_c
//
//  Purpose: Calculates characteristics of 2X2 Chroma DC for CAVLC encoding.
//
//  Parameters:
//              pSrc                            Pointer to 2x2 block - array of size 4.
//              Traling_One                     The number of 搕railing ones?transform coefficient levels
//                                                      in a range[0;3]. This argument is calculated by the function.
//              Traling_One_Signs       Code that describes signs of 搕railing ones?
//                                                      (Trailing_One ? -      i)-bit in this code corresponds to a sign
//                                                      of i-搕railing one?in the current block. In this code 1 indicates
//                                                      negative value, 0 ?positive value. This  argument is calculated
//                                                      by the function.
//              NumOutCoeffs            The number of non-zero coefficients in block (including 搕railing
//                                                      ones?. This argument is calculated by the function.
//              TotalZeros                      The number of zero coefficients in block (except 搕railing zeros?. This
//                                                      argument is calculated by the function.
//              pLevels                         Pointer to an array of size 4 that contains non-zero quantized
//                                                      coefficients of the current block (except 搕railing ones? in reverse scan
//                                                      matrix order.
//              pRuns                           Pointer to an array of size 4 that contains runs before non-zero
//                                                      quantized coefficients (including 搕railing ones? of the current block in
//                                                      reverse scan matrix order (except run before the first non-zero
//                                                      coefficient in block, which can be calculated using TotalZeros).
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     if a pointer is NULL
//
//  Notes:
//    H.264 standard: JVT-G050. ITU-T Recommendation and
//    Final Draft International Standard of Joint Video Specification
//    (ITU-T Rec. H.264 | ISO/IEC 14496-10 AVC) March, 2003.
*/
IppStatus  __STDCALL  ippiEncodeChromaDcCoeffsCAVLC_H264_16s_c(
      const Ipp16s* pSrc,
      Ipp8u   *Trailing_Ones,
      Ipp8u   *Trailing_One_Signs,
      Ipp8u   *NumOutCoeffs,
      Ipp8u   *TotalZeros,
      Ipp16s  *Levels,
      Ipp8u   *Runs)
{
	short	scanedArray[4];
	short	pLevel[4];
	unsigned char pRun[4];

	int		i,k, j;

	int		level,run;
	int		numcoeff, lastcoeff, numtrailingones, totzeros;


	// Count indicate noz-zero coeficients in current Block (It is the last non-zero coeff in scan order)
	for(i=0; i<4; i++)
	{
		scanedArray[i] = pSrc[i];
	}

	*Trailing_Ones = 0;
	*NumOutCoeffs  = 0;
	*TotalZeros    = 0;
	*Trailing_One_Signs = 0;

	numcoeff = 0;
	numtrailingones = 0;
	lastcoeff = 0;
	totzeros = 0;

	level = 1;
	run =0;
	// AC indicate scanedArray[0] can be compressed or not
	for(k=0; k<4; k++)
	{
		if(scanedArray[k])
		{
			pLevel[numcoeff] = level = scanedArray[k]; // level
			pRun[numcoeff]	=	run;					// run
			if (run)
				totzeros += run;

			run = 0;

			if ((level == 1) || (level ==-1))
			{
				numtrailingones ++;
				if (numtrailingones > 3)
				{
					numtrailingones = 3; /* clip to 3 */
				}
			}
			else
			{
				numtrailingones = 0;
			}

			lastcoeff = k;

			numcoeff ++;

		}
		else
			run ++;
	}

	// except numtrailingones -- levels
	// inlucde trainling --- runs
	for(i=0; i<numtrailingones; i++)
	{
		Runs[i]   = pRun[numcoeff-i-1];
	}
	for(j =0, k=numcoeff- numtrailingones-1; k>=0; k--)
	{
		Levels[j] = pLevel[k];
		Runs[j+numtrailingones]   = pRun[k];
		j++;

	}


	*Trailing_Ones = numtrailingones;
	*NumOutCoeffs  = numcoeff;
	*TotalZeros    = totzeros;

	
	if(numcoeff && numtrailingones)
	{
		for(i= 0; i< numtrailingones; i++)
		{
			//if(absm(pLevel[j]) == 1)
			{
				if(pLevel[j] < 0)
					*Trailing_One_Signs = *Trailing_One_Signs | (1<< i);

				numcoeff --;
			}

			j++;
		}
	}


	return ippStsNoErr;
}

#endif
