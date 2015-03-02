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
#include <assert.h>


#include "kinoma_avc_defines.h"
#include "ippvc.h"
#include "kinoma_ipp_lib.h"
#include "kinoma_utilities.h"

IppStatus (__STDCALL *ippiDecodeExpGolombOne_H264_1u16s_universal)						(Ipp32u **ppBitStream, Ipp32s *pBitOffset,Ipp16s *pDst,Ipp8u isSigned)=NULL;//***
Ipp32s	  (__STDCALL *ippiDecodeExpGolombOne_H264_1u16s_signed_universal)				(Ipp32u **ppBitStream, Ipp32s *pBitOffset)=NULL;//***
Ipp32s    (__STDCALL *ippiDecodeExpGolombOne_H264_1u16s_unsigned_universal)				(Ipp32u **ppBitStream, Ipp32s *pBitOffset)=NULL;//***
IppStatus (__STDCALL *ippiDecodeCAVLCCoeffs_H264_1u16s_universal)						(Ipp32u **ppBitStream,  Ipp32s *pOffset,   Ipp16s *pNumCoeff,Ipp16s **ppDstCoeffs,Ipp32u uVLCSelect,Ipp16s uMaxNumCoeff, const Ipp32s **ppTblCoeffToken, const Ipp32s **ppTblTotalZeros,const Ipp32s **ppTblRunBefore,  const Ipp32s *pScanMatrix)=NULL;//***
IppStatus (__STDCALL *ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_universal)				(Ipp32u **ppBitStream, Ipp32s *pOffset,Ipp16s *pNumCoeff,Ipp16s **ppDstCoeffs, const Ipp32s *pTblCoeffToken, const Ipp32s **ppTblTotalZerosCR,   const Ipp32s **ppTblRunBefore)=NULL;//***

#ifdef __KINOMA_IPP__

#if 0
//***dummy
void Init_AVC_CAVLC(void)
{
}

#if defined(_WIN32_WCE)
#define _ARM_PROCESS_FOR_TEST_
#endif
#ifdef _ARM_PROCESS_FOR_TEST_
#include <Cmnintrin.h>
#endif

static const int		incVlc[8] = {0,3,6,12,24,48,32768};    // maximum vlc = 6
static const Ipp8u TotalZero_N_lowTBL[8][32][2]=
{
	{	// TC = 2
		{VLD_LARGE, 6},	{VLD_LARGE, 6},
		{10, 5}, {9,  5},	{8,  4}, {8,  4},	{7,  4}, {7,  4}, {6,  4}, {6,  4},	{5,  4}, {5,  4},	{4,  3},	
		{4,  3}, {4,  3},	{4,  3}, {3,  3},	{3,  3}, {3,  3},	{3,  3}, {2,  3},	{2,  3}, {2,  3},	{2,  3},
		{1,  3}, {1,  3},	{1,  3}, {1,  3},	{0,  3}, {0,  3},	{0,  3}, {0,  3}
	},
	{	// TC = 3
		{VLD_LARGE, 6},	{VLD_LARGE, 5},
		{10, 5},{9,  5},{8,  4},{8,  4},{5,  4},{5,  4},{4,  4},	{4,  4},{0,  4},{0,  4},	{7,  3},{7,  3},{7,  3}, 
		{7,  3},{6,  3},{6,  3},{6,  3},{6,  3},{3,  3},{3,  3},	{3,  3},{3,  3},{2,  3},	{2,  3},{2,  3},{2,  3},
		{1,  3},{1,  3},{1,  3},{1,  3}
	},
	{	// TC = 4
		{12, 5},{11, 5},{10, 5},{0,  5},{9,  4},{9,  4},{7,  4},	{7,  4}	,{3,  4},{3,  4},	{2,  4},{2,  4},{8,  3},
		{8,  3},{8,  3}, {8,  3},{6,  3},{6,  3},{6,  3},{6,  3},{5,  3},{5,  3},{5,  3},{5,  3},{4,  3},{4,  3},
		{4,  3},{4,  3},{1,  3},{1,  3},{1,  3}, {1,  3}
	},
	// TC = 5
	{
		{11, 5},{9 , 5},{10, 4},{10, 4},{8,  4},{8,  4},	{2,  4}	,{2,  4},{1,  4},{1,  4},{0,  4},{0,  4},{7,  3},{7,  3},
		{7,  3},{7,  3},{6,  3},{6,  3},{6,  3},{6,  3},{5,  3},{5,  3},{5,  3},{5,  3},{4,  3},{4,  3},{4,  3},
		{4,  3},{3,  3},{3,  3},{3,  3},{3,  3}
	},
	{	// TC = 6
		{VLD_LARGE, 6},	{1 , 5},{8 , 4},	{8,  4},	{9,  3},	{9,  3},	{9,  3}	,	{9,  3}	,	{7,  3},	{7,  3},	
		{7,  3},	{7,  3},	{6,  3},	{6,  3},	{6,  3},	 {6,  3},		{5,  3},	{5,  3},	{5,  3},	{5,  3},	{4,  3},	
		{4,  3},	{4,  3},	{4,  3},	{3,  3},	{3,  3},	{3,  3},	{3,  3},	{2,  3},	{2,  3},	{2,  3},	 {2,  3}
	},
	{	// TC = 7
		{VLD_LARGE, 6},	{1 , 5},	{7,  4},	{7,  4},	{8,  3},	{8,  3},	{8,  3}	,	{8,  3}	,	{6,  3},	{6,  3},	{6,  3},	
		{6,  3},	{4,  3},	{4,  3},	{4,  3},	 {4,  3},{3,  3},	{3,  3},	{3,  3},	{3,  3},	{2,  3},	{2,  3},	{2,  3},	
		{2,  3},	{5,  2},	{5,  2},	{5,  2},	{5,  2},	{5,  2},	{5,  2},	{5,  2},	 {5,  2}
	},
	{	// TC = 8
		{VLD_LARGE, 6},	{2,  5},	{1,  4},	{1,  4},	{7,  3},	{7,  3},	{7,  3}	,	{7,  3}	,	{6,  3},{6,  3},	{6,  3},	
		{6,  3},	{3,  3},	{3,  3},	{3,  3},	 {3,  3},	{5,  2},	{5,  2},	{5,  2},	{5,  2},	{5,  2},	{5,  2},	{5,  2},	
		{5,  2},	{4,  2},	{4,  2},	{4,  2},	{4,  2},	{4,  2},	{4,  2},	{4,  2},	 {4,  2}
	},
	{	// TC = 9
		{VLD_LARGE, 6},	{7,  5},{2,  4},{2,  4},	{5,  3},{5,  3},	{5,  3}	,	{5,  3}	,	{6,  2},	{6,  2},	
		{6,  2},	{6,  2},	{6,  2},	{6,  2},	{6,  2},	 {6,  2},	{4,  2},	{4,  2},	{4,  2},	{4,  2},{4,  2},
		{4,  2},	{4,  2},	{4,  2},	{3,  2},	{3,  2},	{3,  2},	{3,  2},	{3,  2},	{3,  2},	{3,  2},{3,  2}
	}
};

static const Ipp8u TotalZero_N_lowTBL_link[8][4] =
{
	{ 	// TC2
		14,	13,	12,	11 	},
	{	// TC3
		13,	11, 12,12  },
	{	// TC4
		VLD_ERROR, VLD_ERROR, VLD_ERROR, VLD_ERROR	},
	{	// TC5
		VLD_ERROR, VLD_ERROR, VLD_ERROR, VLD_ERROR },
	{ 	// TC6
		10,	0 , VLD_ERROR, VLD_ERROR},
	{	// TC7
		9 , 0 , VLD_ERROR, VLD_ERROR},
	{
		// TC8
		8 , 0 , VLD_ERROR, VLD_ERROR},
	{
		// TC9
		1 , 0 ,	VLD_ERROR, VLD_ERROR},
};

static const Ipp8u TotalZero_N_highTBL[6][8][2] =
{
	{ // TC= 10
		{VLD_LARGE,  0}, {2,  3},{5,  2},{5,  2},{4,  2},{4,  2},	{3,  2},{3,  2} },
	{ // TC= 11
		{VLD_LARGE,  0},{2,  3},{3,  3},	{5,  3},{4,  1},{4,  1},{4,  1},{4,  1} },
	{ // TC= 12
		{VLD_LARGE,  0},{4,  3},	{2,  2},{2,  2},	{3,  1},	{3,  1},	{3,  1},	{3,  1} 	},
	{ // TC= 13
		{0,  3},{1,  3},{3,  2},	{3,  2},{2,  1},{2,  1},	{2,  1},{2,  1} 	},
	{ // TC= 14
		{0,  2},{0,  2},{1,  2},{1,  2},{2,  1},{2,  1},{2,  1},	{2,  1} 	},
	{ // TC= 15
		{0,  1},{0,  1},{0,  1},	{0,  1},{1,  1},{1,  1},{1,  1},{1,  1} 	}
};

static const Ipp8u TotalZero_N_highTBL_link[6][4][2] =	
{
	{ // TC = 10
		{1,  5}, {0, 5}, {6, 4}, {6,4}
	},
	{ // TC= 11
		{0, 4},  {0, 4}, {1, 4}, {1, 4}
	},
	{ // TC= 12
		{0, 4}, {0, 4},  {1, 4}, {1, 4}
	},
	{ // TC= 13
		{VLD_ERROR, 0}, {VLD_ERROR, 0}, {VLD_ERROR, 0}, {VLD_ERROR, 0}
	},
	{ // TC= 14
		{VLD_ERROR, 0}, {VLD_ERROR, 0}, {VLD_ERROR, 0}, {VLD_ERROR, 0}
	},
	{ // TC= 15
		{VLD_ERROR, 0}, {VLD_ERROR, 0}, {VLD_ERROR, 0}, {VLD_ERROR, 0}
	}
};

// This function is used for K_ippiDecodeCAVLCCoeffs_H264 and K_ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s ONLY
//  We do not check if the bitsNumber is exceed current buffer, So, please ensure this in caller
 /*
 **************************************************************************
 * Function:    readBits
 * Description: Read bits from stream. 
 *				pBitOffset		:is offset of stream DWORD [0, 31]
 *				bitsNumber  :is bits required from its caller 
 * Problems   : 
 *
 * 
 **************************************************************************
 */
// TODO: if we have bxtr instruction, we can do this more efficient! -- WWD
static inline Ipp32u readBits(Ipp32u **ppBitStream, Ipp32s *pBitOffset, const  int	bitsNumber)
{
	Ipp32u  value;			// Right aligned -- WWD
	Ipp32u	valuetmp;
	int		bitsLeft, bitsLeftinCurentDBuff;

	assert(bitsNumber >0);

	// number of bits left in current DWORD buff
	bitsLeft = *pBitOffset+1;
	bitsLeftinCurentDBuff = bitsLeft - bitsNumber;

	// To see if bits in current DWORD is enough for reading
	value = **ppBitStream;
	if(bitsLeftinCurentDBuff>=0)
	{

		// Here we need one bxtr instruction like MPIS,how  about ARM?
		value >>= (bitsLeftinCurentDBuff);
		// Reset to ZERO for high bits for value
		value <<= (32 - bitsNumber);
		value >>= (32 - bitsNumber);

		// Update offset
		*pBitOffset -= bitsNumber;

		// Need to check ASM code to see if compiler can do such type of optimization??????????????
		if(*pBitOffset < 0)		// Need read new data from bit-stream
		{
			(*ppBitStream)++;
			*pBitOffset = 31;	// available bits is 31+(1)
		}
	}
	else		// Do not enough, need read another DWORD. But we  sure that new DWORD is always enough (The maximum length is 28 bits according SPEC G050)
	{
		int bitsLeftInsecondDBuff = 32 + bitsLeftinCurentDBuff;
		value <<= (32 - bitsLeft);

		(*ppBitStream)++;

		// Extract bits from second DWORD
		valuetmp = **ppBitStream;
		valuetmp >>= bitsLeftInsecondDBuff;

		// Combine
		// Now value is left aligned, At this time we need right aligned
		value >>= (32 - bitsNumber);  // Can  not use -- (32 - bitsLeft)
		value |= valuetmp;

		// Update offset
		*pBitOffset = bitsLeftInsecondDBuff - 1;
	}

	return value;
}

static inline Ipp32u peekBits(Ipp32u   *pBitStream, Ipp32s   BitOffset, const  int	bitsNumber)
{
	Ipp32u  value;			// Right aligned -- WWD
	Ipp32u	valuetmp;
	int		bitsLeft, bitsLeftinCurentDBuff;

	assert(bitsNumber >0);

	// number of bits left in current DWORD buff
	bitsLeft = BitOffset+1;
	bitsLeftinCurentDBuff = bitsLeft - bitsNumber;

	// To see if bits in current DWORD is enough for reading
	value = *pBitStream;
	if(bitsLeftinCurentDBuff>=0)
	{

		// Here we need one bxtr instruction like MPIS,how  about ARM?
		value >>= (bitsLeftinCurentDBuff);
		// Reset to ZERO for high bits for value
		value <<= (32 - bitsNumber);
		value >>= (32 - bitsNumber);

		// Update (omit for it is peekbits only)
	}
	else		// Do not enough, need read another DWORD. But we  sure that new DWORD is always enough (The maximum length is 28 bits according SPEC G050)
	{
		int bitsLeftInsecondDBuff = 32 + bitsLeftinCurentDBuff;
		value <<= (32 - bitsLeft);

		// Extract bits from second DWORD
		valuetmp = *(pBitStream+1);
		valuetmp >>= bitsLeftInsecondDBuff;

		// Combine
		// Now value is left aligned, At this time we need right aligned
		value >>= (32 - bitsNumber);  // Can  not use -- (32 - bitsLeft)
		value |= valuetmp;

		// Update (omit for it is peekbits only)
	}

	return value;
}

// This function used when skipped bits is know, we just update only!
static inline void skipBits(Ipp32u **ppBitStream, Ipp32s *pBitOffset, const  int	bitsNumber)
{
	int		bitsLeftinCurentDBuff;

	assert(bitsNumber >0);

	// number of bits left in current DWORD buff
	bitsLeftinCurentDBuff = *pBitOffset - bitsNumber;

	// To see if bits in current DWORD is enough for reading
	if(bitsLeftinCurentDBuff>=0)
	{
		// Update offset
		*pBitOffset = bitsLeftinCurentDBuff;
	}
	else		// Do not enough, need read another DWORD. But we  sure that new DWORD is always enough (The maximum length is 28 bits according SPEC G050)
	{
		(*ppBitStream)++;
		// Update offset
		*pBitOffset = bitsLeftinCurentDBuff + 32;
	}
}
static inline int COUNT_LEADING_ZEROS(int input, int start_pos/*0-31*/)
{
	unsigned int mask = (1<<start_pos);
	int count = 0;
	int i;
	for(i=start_pos; i>=0; i++)
	{
		if((input & mask) == 0)
			count++;
		else
			break;
		mask = mask>>1;
	}

	return count;
}

///////////////////////////////////////////////////////      TC+T1s     //////////////////////////////////////////////////////////
/**************************************************************************************************************************
 * New function for decode Total coeff + Trailing Ones
 *
 **************************************************************************************************************************
 */
static inline void KN_avcdec_TC0(Ipp32u **ppBitStream, Ipp32s *pOffset, Ipp32s *pNumCoeff, Ipp32s *pNumTrailingOnes)
{
	unsigned int current_code,nCoeffs, nOnes,bits_skiped, temp;

	current_code = peekBits(*ppBitStream, *pOffset, 9);

	if(current_code >= 4)
	{
		/*9 bits */
		if(current_code >= 48)
		{
			nCoeffs	= COUNT_LEADING_ZEROS(current_code, 8);  //ARM support this or not?
			nOnes	= nCoeffs;
			bits_skiped	= nCoeffs+1+ ((nCoeffs +1)>>2);
		}
		else if(current_code >= 24)
		{
			temp = 5 - (current_code>>3);
			nOnes = (temp + (temp>>1));
			nCoeffs = nOnes +1;
			bits_skiped = 6;
		}
		else
		{
			temp = (current_code>>3);
			nOnes = 7 - (current_code >>temp);
			nCoeffs = nOnes + (3-temp) + ((nOnes+1)>>2);
			bits_skiped = 9- temp;
		}
	}
	else
	{
		/*16 bits */
		current_code = peekBits(*ppBitStream, *pOffset,  16);
		if(current_code >= 128)
		{
			/* 10, 11 bits*/
			temp = current_code >> 8;
			nOnes = 7 - (current_code >>(temp+5));
			bits_skiped = 11 - temp;
			nCoeffs = nOnes + 5 - temp +((nOnes+1)>>2);
		}
		else if(current_code >= 64)
		{
			/*13 bits */
			temp = (current_code<72)?7:(current_code>>3);
			nOnes = (3 - (temp & 3));
			bits_skiped = 13;
			nCoeffs = nOnes + 6 + ((15-temp)>>2) + ((nOnes+1)>>2);
		}
		else if(current_code >= 16)
		{
			/* 14, 15 bits*/
			unsigned temp2;
			temp = (current_code >> 5);
			temp2 = (current_code >> (temp+1));
			nOnes = (3 - (temp2 & 3));
			bits_skiped = 15 - temp;
			nCoeffs = nOnes + 10 - (temp <<1) + ((15 - temp2)>>2) + ((4-nOnes)>>2);
		}
		else if(current_code >= 5)
		{
			/* 16 bits*/
			nOnes = (3 - (current_code & 3));
			bits_skiped = 16;
			nCoeffs = nOnes + 12 + ((15 - current_code)>>2) + ((current_code & 2)>>1);
		}
		else
		{
			/* Other */
			temp = (current_code >> 2);
			nOnes = (1 - temp);
			bits_skiped = 15 + temp;
			nCoeffs = 13 + (3 * temp);
		}
	}
	
	// Update
	skipBits(ppBitStream, pOffset,  bits_skiped);;
	*pNumCoeff = nCoeffs; 
	*pNumTrailingOnes = nOnes;
}
static inline void KN_avcdec_TC1(Ipp32u **ppBitStream, Ipp32s *pOffset, Ipp32s *pNumCoeff, Ipp32s *pNumTrailingOnes)
{
	unsigned int current_code,nCoeffs, nOnes,bits_skiped, temp;

	current_code = peekBits(*ppBitStream, *pOffset, 9);

	if(current_code >= 4)
	{
		unsigned temp2;
		/*9 bits */
		if(current_code >= 128)
		{
			/* 2, 3, 4 bits*/
			temp = 1 - (current_code >>8);
			temp2 = 7 - (current_code >> (6-temp));
			nOnes	= (temp<<1) + (temp2>>1);
			nCoeffs	= nOnes + ((temp + temp2)>>2);
			bits_skiped	= temp + 2 + ((nOnes +1)>>2);
		}
		else if(current_code >= 16)
		{
			unsigned temp3, temp4;
			/* 5, 6, 7 bits*/
			temp = (current_code>>5);
			temp2 = ((temp +1)>>2);
			temp3 = (temp + 11)>>2;
			nOnes = 3-((current_code>>temp3) & (3-temp2));
			temp4 = (nOnes +1)>>1;
			nCoeffs = (3-temp) + (temp4<<1) + (temp4>>1);
			bits_skiped = 9 - temp2 - temp3;
		}
		else
		{
			unsigned temp3;
			/* 8, 9 bits */
			temp = (current_code>>3);
			temp2 = (current_code >> temp);
			temp3 = temp + ((current_code == 5)? 1:0);
			nOnes = ((temp2 - temp) == 3)?0:(7-temp2);
			nCoeffs = nOnes + (6-temp) - (temp3 & (temp2 &1));
			bits_skiped = 9- temp;
		}
	}
	else
	{
		unsigned temp2;

		/*14 bits */
		current_code = peekBits(*ppBitStream, *pOffset,14);
		if(current_code >= 32)
		{
			/* 10, 11 bits*/
			temp = current_code >> 6;
			temp2 = (current_code<36)?7:(current_code>>(2+temp));
			nOnes = 3 - (temp2 & 3);
			bits_skiped = 12 - temp;
			nCoeffs = nOnes + 9 - (temp<<1) +((15-temp2)>>2) - ((temp2 & 1) & (nOnes >>1));
		}
		else if(current_code >= 12)
		{
			/*13 bits */
			temp = (current_code< 14)? 5 :(current_code>>1);
			nOnes = (3 - (temp & 3));
			bits_skiped = 13;
			nCoeffs = 12 + ((15- temp)>>2) + ((nOnes+1)>>2);
		}
		else if(current_code >= 8)
		{
			/* 14, 15 bits*/
			nOnes = 12 - current_code - 3*((13 - current_code)>>2);
			bits_skiped = 14;
			nCoeffs = 17 - ((current_code +1)>>2);
		}
		else
		{
			/* 16 bits*/
			temp = current_code>>2;
			nOnes = (current_code < 4) ? 3 : (7-current_code);
			bits_skiped = 13 + temp;
			nCoeffs = 15 + temp;
		}
	}
	
	// Update 
	skipBits(ppBitStream, pOffset,  bits_skiped);;
	*pNumCoeff = nCoeffs; 
	*pNumTrailingOnes = nOnes;
}
static inline void KN_avcdec_TC2(Ipp32u **ppBitStream, Ipp32s *pOffset, Ipp32s *pNumCoeff, Ipp32s *pNumTrailingOnes)
{
	unsigned int current_code,num_coeffs, num_ones,bits_skiped, temp;

	current_code = peekBits(*ppBitStream, *pOffset, 7);

	if(current_code >= 11)
	{
		/*9 bits */
		if(current_code >= 64)
		{
			temp = 15 - (current_code >> 3);
			num_ones	= (temp >3)?3:temp;
			num_coeffs	= temp;
			bits_skiped	= 4;
		}
		else if(current_code >= 32)
		{
			temp = 16 - (current_code>>2);
			num_ones = (temp <4)? temp:(1 + (temp &1));
			num_coeffs = (temp ==3)?8:(num_ones + (temp>>1) +((9-temp)>>3));
			bits_skiped = 5;
		}
		else if(current_code >= 18)
		{
			unsigned int temp2;
			temp = 15 - (current_code>>1);
			num_ones = (temp & 3);
			temp2 = (num_ones +1)>>1;
			num_coeffs = (temp2 <<2) + (1+(temp>>2)) + (temp2 &1);
			bits_skiped = 6;

		}
		else
		{
			temp = current_code >> 4;
			num_ones = ((15 - current_code) & 3) * (1-temp);
			bits_skiped = 7 - temp;
			num_coeffs = (num_ones == 0)?(7- (current_code>>2)):(((num_ones+7)>>1)<<1);
		}
	}
	else
	{
		/*16 bits */
		current_code = peekBits(*ppBitStream, *pOffset, 10);
		if(current_code >= 64)
		{
			/* 10, 11 bits*/
			temp = current_code >> 3;
			num_ones = ((temp +6) >> 4)<<1;
			bits_skiped = 7;
			num_coeffs = num_ones + 7 - (temp & 1);
		}
		else if(current_code >= 18)
		{
			unsigned temp2;
			/*13 bits */
			temp = (current_code >> 5);
			temp2 = (current_code>> (temp + 1));
			num_ones = (3 - (temp2 & 3));
			bits_skiped = 9 - temp;
			num_coeffs = num_ones + ((5 - temp)<<1) +((15-temp2)>>2);
		}
		else if(current_code >= 14)
		{
			num_ones = (current_code & 2)>>1;
			bits_skiped = 9;
			num_coeffs = num_ones + 12;
		}
		else
		{
			/* Other */
			num_ones = (13 - current_code) &3;
			bits_skiped = 10;
			num_coeffs = 16 - ((current_code -1)>>2);
		}
	}
	
	// Update
	skipBits(ppBitStream, pOffset,  bits_skiped);;
	*pNumCoeff = num_coeffs; 
	*pNumTrailingOnes = num_ones;
}
static inline void KN_avcdec_TCC(Ipp32u **ppBitStream, Ipp32s *pOffset, Ipp32s *pNumCoeff, Ipp32s *pNumTrailingOnes)
{
	unsigned int current_code,nCoeffs, nOnes,bits_skiped, temp;

	current_code = peekBits(*ppBitStream, *pOffset, 8);

	if(current_code >= 32)
	{
		temp = current_code >> 5;
		nOnes	= (temp >> 2) + ((( 9- temp) >>3)<<1);
		nCoeffs	= nOnes;
		bits_skiped	= 3 - ((temp>>1) & (3-nOnes));

		// bits_skiped can use LCZOB
	}
	else if(current_code >=8)
	{
		unsigned int temp2;
		temp = current_code >> 2;
		temp2 = 7 - temp;
		nOnes = ((temp -1) >> 2) * (temp2 + (temp2>>1));
		nCoeffs = temp2 +1 -(((temp2+1)>>2)<<1);
		bits_skiped = 6;
	}
	else if(current_code >=2)
	{
		temp = (current_code >> 2);
		nOnes = 4 - (current_code >> temp);
		nCoeffs = 4 - temp;
		bits_skiped = 8 - temp;
	}
	else
	{
		nOnes = 3;
		nCoeffs = 4;
		bits_skiped = 7;
	}
	
	// Update
	skipBits(ppBitStream, pOffset,  bits_skiped);
	*pNumCoeff = nCoeffs; 
	*pNumTrailingOnes = nOnes;
}
static inline int  KN_avcdec_RunBefore(Ipp32u **ppBitStream, Ipp32s *pOffset, unsigned int zeroLeft)
{
	unsigned int current_code,nRuns,bits_skiped, temp;

	if(zeroLeft <= ZEROLEFT_1)
	{
		current_code = peekBits(*ppBitStream, *pOffset, zeroLeft);

		nRuns	= zeroLeft - (current_code & ((current_code>>1)+1));
		bits_skiped	= zeroLeft - (current_code >>1);
	}
	else if(zeroLeft <= ZEROLEFT_2)
	{
		current_code = peekBits(*ppBitStream, *pOffset, 3);

		if((3+(current_code>>1)) >= zeroLeft)
		{
			nRuns	= 3 - (current_code>>1);
			bits_skiped	= 2;
		}
		else
		{
			temp = (zeroLeft - (current_code &3));
			nRuns	= (zeroLeft<6)? temp:((current_code<2)?(current_code+1):temp);
			bits_skiped	= 3;
		}
	}
	else
	{
		current_code = peekBits(*ppBitStream, *pOffset,  11);
		if(current_code >= 256)
		{
			nRuns	= 7 - (current_code >> 8);
			bits_skiped	= 3;
		}
		else
		{
			temp = COUNT_LEADING_ZEROS(current_code, 11-1); 
			nRuns	= 4 + temp;
			bits_skiped	= temp +1;
		}
	}

	// Update
	skipBits(ppBitStream, pOffset,  bits_skiped);

	return nRuns;
}
/**************************************************************************************************************************
 * New function for decode Total Zeros
 *
 **************************************************************************************************************************
 */
static inline int KN_avcdec_TZ1(Ipp32u **ppBitStream, Ipp32s *pOffset)
{
	unsigned int current_code,total_zeros,bits_skiped, temp;

	current_code = peekBits(*ppBitStream, *pOffset, 9);

	if(current_code >= 256)
	{
		total_zeros	= 0;
		bits_skiped	= 1;
	}
	else if(current_code >=4)
	{
		temp = COUNT_LEADING_ZEROS(current_code, 9-1);
		total_zeros = (temp <<1) - ((current_code>>(7-temp)) & 1);
		bits_skiped	= temp + 2;
	}
	else
	{
		total_zeros = 16 - current_code;
		bits_skiped = 9;
	}

	// Update
	skipBits(ppBitStream, pOffset,  bits_skiped);
	return total_zeros;
}
static inline int KN_avcdec_TZC(Ipp32u **ppBitStream, Ipp32s *pOffset, int TotalCoeffs)
{
	unsigned int current_code,total_zeros,bits_skiped, temp;

	temp = 4 - TotalCoeffs;
	current_code = peekBits(*ppBitStream, *pOffset, 4 - TotalCoeffs);

	if(current_code <= 1)
	{
		total_zeros	= temp - current_code;
		bits_skiped	= temp;
	}
	else
	{
		temp --;
		total_zeros = 1 - ((current_code & (temp<<1)) >> temp);
		bits_skiped	= total_zeros + 1;
	}

	// Update
	skipBits(ppBitStream, pOffset,  bits_skiped);;
	return total_zeros;
}
////////////////////////////////////////// Functions for decode Level ///////////////////////////////////////////////////////////////////////
static inline int KN_avcdec_decode_level0(Ipp32u **ppBitStream, Ipp32s *pOffset, Ipp16u *levelabs)
{
	unsigned int currDBuff;
	int leadingZeros;
	int sign, code;
	int level; 
	
	currDBuff = peekBits(*ppBitStream, *pOffset, 32); // We read 32 bits from bit stream to avoid branch in follow -- WWD
														// Now currDBuff is enough for level function
	leadingZeros = CNTLZ(currDBuff);

	if (leadingZeros < 14)
	{
		sign =   leadingZeros & 1;
		level = (leadingZeros >>1) + 1;

	}
	else if (leadingZeros == 14)
	{
		// escape code: total 19bits= 14(lz) + "1" + 4(info)
		unsigned int temp;
		temp = (currDBuff<<15) >> (32-4); //
		code = (1 << 4) | temp;

		sign =  (code & 1);
		level = ((code >> 1) & 0x7) + 8;

		leadingZeros += 4;
	}
	else if (leadingZeros >= 15)
	{
		// escape code: total 28bits = 15(lz) + "1" + 12(info)
		unsigned int temp;
		temp = (currDBuff<<16) >> 20; //(32-12)
		code = (1 << 12) | temp;

		sign =  (code & 1);
		level = ((code >> 1) & 0x7ff) + 16;

		leadingZeros  += 12;
	}
	skipBits(ppBitStream, pOffset, leadingZeros+1);

	*levelabs = level; 

	return sign;
}
static short VLCEscapeTbl[8]	= {16, 16, 31, 61,121, 241, 481, 4095};
static unsigned char VLCNonEscapeTbl[8]	= {1,1, 2, 3, 4, 5, 6, 255};		// Include sign bits
static inline int KN_avcdec_decode_levelN(Ipp32u **ppBitStream, Ipp32s *pOffset, int vlcnum, Ipp16u *levelabs)
{
	unsigned int currDBuff;
	int leadingZeros;
	int levabs, sign;
	int code;	  

	assert(vlcnum >0);
	currDBuff = peekBits(*ppBitStream, *pOffset, 32); // We read 32 bits from bit stream to avoid branch in follow -- WWD
														// Now currDBuff is enough for level function
	leadingZeros = CNTLZ(currDBuff);

	if (leadingZeros < 15)
	{
		levabs = (leadingZeros<<(vlcnum-1)) + 1;

		currDBuff = (currDBuff<< (leadingZeros+1))>>(32-vlcnum);
		levabs += (currDBuff>>1) ;
    
		// sign
		sign = currDBuff & 0x01;

		leadingZeros += vlcnum;
	}
	else // escape
	{
		// Total 28 bits
		// escape code: total 28bits = 15(lz) + "1" + 12(info)
		unsigned int temp;
		temp = (currDBuff<<16) >> 20; //(32-12)
		code = (1 << 12) | temp;

		sign =  (code & 1);
		levabs = ((code >> 1) & 0x7ff) + VLCEscapeTbl[vlcnum]; // 11 bits real data + 1 bit sign

		leadingZeros  += 12;
	}
	
	skipBits(ppBitStream, pOffset, leadingZeros+1);
	  
	*levelabs = levabs;

	return sign;
}
/////////////////////////////////////////////// Level functions END /////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////  NEW Functons END ///////////////////////////////////////////////////////////////////
IppStatus __STDCALL ippiDecodeCAVLCCoeffs_H264_1u16s_c(Ipp32u **ppBitStream,
                                                     Ipp32s *pOffset,
                                                     Ipp16s *pNumCoeff,
                                                     Ipp16s **ppDstCoeffs,
                                                     Ipp32u uVLCSelect,
                                                     Ipp16s uMaxNumCoeff,
                                                     const Ipp32s **ppTblCoeffToken,
                                                     const Ipp32s **ppTblTotalZeros,
                                                     const Ipp32s **ppTblRunBefore,
                                                     const Ipp32s *pScanMatrix )//***
{
	int		levarr[16], runarr[16];
	int		nTotalCoeffs, nTrailingOnes, nTotalZeros, nNonOnesCoeffs;
	short			level;
	unsigned short	levelabs;
	int		vlcnum, code, sign;
	int		coeffNumber;
	int		i,	k;

	Profile_Start(ippiDecodeCAVLCCoeffs_H264_1u16s_c_profile);

	nTotalCoeffs = 0;

	// First, decode TC and T1s
	if(uVLCSelect < 2)
		KN_avcdec_TC0(ppBitStream, pOffset, &nTotalCoeffs, &nTrailingOnes);
	else if(uVLCSelect < 4)
	{
		KN_avcdec_TC1(ppBitStream, pOffset, &nTotalCoeffs, &nTrailingOnes);
	}
	else if(uVLCSelect < 8)
	{
		KN_avcdec_TC2(ppBitStream, pOffset, &nTotalCoeffs, &nTrailingOnes);
	}
	else
	{
		// read 6 bit FLC
		code = readBits(ppBitStream, pOffset, 6);

		nTrailingOnes = code & 3;
		nTotalCoeffs = (code >> 2);

		// Speical check 
		if (!nTotalCoeffs && nTrailingOnes == 3)
		{
			// #c = 0, #t1 = 3 =>  #c = 0
			nTrailingOnes = 0;
		}
		else
		{
			nTotalCoeffs++;
		}
	}

	*pNumCoeff = nTotalCoeffs;
	if (nTotalCoeffs)
	{
		// Reset??
		memset((*ppDstCoeffs), 0, 16*sizeof(Ipp16s));
		memset(runarr, 0, 16*sizeof(int));

		// First: Get the sign of trailing ones
		if (nTrailingOnes)
		{
			int nTrailingPos = nTotalCoeffs -1;
			code = readBits(ppBitStream, pOffset, nTrailingOnes);

			//DO not need loop: use if is enough
			if(nTrailingOnes == 3)
			{
				levarr[nTrailingPos--] = (code & 0x04)?-1:1;	// Condition move 
			}
			if(nTrailingOnes >= 2)
			{
				levarr[nTrailingPos--] = (code & 0x02)?-1:1;	// Condition move 
			}
			
			levarr[nTrailingPos] = (code & 0x01)?-1:1;		// Condition move 
		}

		// Second: decode other levels(If needed)
		// Add one statement to see if need decode level
		nNonOnesCoeffs = nTotalCoeffs - nTrailingOnes;
		if(nNonOnesCoeffs > 0)
		{
			// 2.1 init VLD context, and decode first non-zero coeff (vlcnum==0 or vlcnum==1)
			if (nTotalCoeffs > 10 && nTrailingOnes < 3)
			{
				vlcnum = 1;
				// must be "1" (vlcnum==1)
				sign = KN_avcdec_decode_levelN(ppBitStream, pOffset, vlcnum/*Must be 1*/, &levelabs);
			}
			else
			{
				vlcnum = 0;
				sign = KN_avcdec_decode_level0(ppBitStream, pOffset, &levelabs);
			}

			//level_two_or_higher = 1;
			//if (nTotalCoeffs > 3 && nTrailingOnes == 3)
			//{
			//	level_two_or_higher = 0;
			//}
			//if (level_two_or_higher)
			level = (sign)?-levelabs:levelabs;
			if(nTotalCoeffs<=3 || nTrailingOnes!=3)
			{
				// Shoule use condition move
				// TODO: check ASM code
				if (level > 0)
					level ++;
				else
					level --;
			}

			levarr[--nNonOnesCoeffs] = level;		// Here must be --nNonOnesCoeffs: WWD


			// update VLC table
			if (vlcnum == 0)
			{
				vlcnum++;
			}
			if (absm(level)>3)		// First level is special, so, do not use levelabs directly.
				vlcnum = 2;

			// 2.3 all other non-zero coeffs(vlcnum>=1, but <=6)
			for (; nNonOnesCoeffs > 0; )
			{
				//decode current level
				sign = KN_avcdec_decode_levelN(ppBitStream, pOffset, vlcnum, &levelabs);
				level = (sign)?-levelabs:levelabs;
				levarr[--nNonOnesCoeffs] = level;	// Here must be --nNonOnesCoeffs: WWD

				// update VLC table
				if (levelabs>incVlc[vlcnum])
				{
					vlcnum++;
				}
			}
		}
		// Third: decode Run information : Chroma are different from Luma in this part
		if (nTotalCoeffs < uMaxNumCoeff)
		{
			// 3.1 TotalZeros,  Its context is (nTotalCoeffs-1) (Do not need -1 in IPP)
			if(nTotalCoeffs == 1)
			{
				nTotalZeros = KN_avcdec_TZ1(ppBitStream, pOffset);
			}
			else
			{
				unsigned int temp, tmp;
				unsigned int symbol_len;

				// Look-up-table method
				if(nTotalCoeffs < 10)
				{
					int index = nTotalCoeffs - 2;

					tmp = peekBits(*ppBitStream, *pOffset, 5);

					nTotalZeros = TotalZero_N_lowTBL[index][tmp][0];  
					symbol_len = TotalZero_N_lowTBL[index][tmp][1];

					// too larger
					if(nTotalZeros >= VLD_LARGE)
					{
						tmp = peekBits(*ppBitStream, *pOffset, 6);
						temp = tmp & 0x3;

						nTotalZeros = TotalZero_N_lowTBL_link[index][temp];
					}

				}				
				else
				{
					int index = nTotalCoeffs - 10;

					tmp = peekBits(*ppBitStream, *pOffset, 3);

					nTotalZeros = TotalZero_N_highTBL[index][tmp][0];  
					symbol_len = TotalZero_N_highTBL[index][tmp][1];

					if(nTotalZeros >= VLD_LARGE)
					{
						tmp = peekBits(*ppBitStream, *pOffset, 5);
						temp = tmp & 0x3;

						nTotalZeros = TotalZero_N_highTBL_link[index][temp][0];
						symbol_len = TotalZero_N_highTBL_link[index][temp][1];
					}

				}
				// Update
				skipBits(ppBitStream, pOffset,  symbol_len);
			}
			// 3.2 decode run before: need more check
			{
				int nRuns;

				i = nTotalCoeffs - 1;
				while(nTotalZeros && i )
				{
					nRuns = KN_avcdec_RunBefore(ppBitStream, pOffset,nTotalZeros);
					runarr[i] = nRuns;
					nTotalZeros -= nRuns;
					i --;
				} 
			}
			runarr[i] = nTotalZeros;

			// Forth : Inverse-SCAN and save to ppDst
			if(uMaxNumCoeff == 16)		// Luma
				coeffNumber = -1;
			else
				coeffNumber = 0;		// Chroma AC part, We do not use [0] element WWD 

			for(k=0; k< nTotalCoeffs; k++)
			{
				coeffNumber += runarr[k] +1;		// Need reset runs array at begin of this block4x4??
				(*ppDstCoeffs)[pScanMatrix[coeffNumber]] = levarr[k];
			}
		}
		else
		{
			// Forth : Inverse-SCAN and save to ppDst
			//    seldom: do not need run array
			if(uMaxNumCoeff == 16)		// Luma 
			{
				for(k=0; k< nTotalCoeffs; k++)
				{
					(*ppDstCoeffs)[pScanMatrix[k]] = levarr[k];
				}
			}
			else			// Chroma AC, We do not use [0] element WWD 
			{	
				coeffNumber = 0;
				for(k=0; k< nTotalCoeffs; k++)
				{
					(*ppDstCoeffs)[pScanMatrix[k+1]] = levarr[k];
				}
			}
		}

		// According to Function spec in Ippiman.pdf
		(*ppDstCoeffs) += 16;		 
	}

	Profile_End(ippiDecodeCAVLCCoeffs_H264_1u16s_c_profile);

	return ippStsNoErr;

}

#define MAX_COEFF_FOR_CHROMA		4
IppStatus __STDCALL ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c(Ipp32u **ppBitStream,
                                                             Ipp32s *pOffset,
                                                             Ipp16s *pNumCoeff,
                                                             Ipp16s **ppDstCoeffs,
                                                             const Ipp32s *pTblCoeffToken,
                                                             const Ipp32s **ppTblTotalZerosCR,
                                                             const Ipp32s **ppTblRunBeforeB )//***
{
	int		levarr[16], runarr[16];
	int		nTotalCoeffs, nTrailingOnes, nTotalZeros, nNonOnesCoeffs;
	short			level;
	unsigned short	levelabs;
	int		vlcnum, code, sign;
	int		coeffNumber;

	int		i,	k;	

	Profile_Start(ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_profile);

	// Support YUV420 format ONLY at present
	KN_avcdec_TCC(ppBitStream, pOffset, &nTotalCoeffs, &nTrailingOnes);


	*pNumCoeff = nTotalCoeffs;
	if (nTotalCoeffs)
	{
		// Reset??
		memset((*ppDstCoeffs), 0, 16*sizeof(Ipp16s));
		memset(runarr, 0, 16*sizeof(int));

		// First: Get the sign of trailing ones
		if (nTrailingOnes)
		{
			int nTrailingPos = nTotalCoeffs -1;
			code = readBits(ppBitStream, pOffset, nTrailingOnes);

			//DO not need loop: use if is enough
			if(nTrailingOnes == 3)
			{
				levarr[nTrailingPos--] = (code & 0x04)?-1:1;	// Condition move 
			}
			if(nTrailingOnes >= 2)
			{
				levarr[nTrailingPos--] = (code & 0x02)?-1:1;	// Condition move 
			}
			
			levarr[nTrailingPos] = (code & 0x01)?-1:1;		// Condition move 
		}

		// Second: decode other levels(If needed)
		// Add one statement to see if need decode level
		nNonOnesCoeffs = nTotalCoeffs - nTrailingOnes;
		if(nNonOnesCoeffs > 0)
		{
			// 2.1 init VLD context, and decode first non-zero coeff (vlcnum==0 or vlcnum==1)
			if (nTotalCoeffs > 10 && nTrailingOnes < 3)
			{
				vlcnum = 1;
				// must be "1" (vlcnum==1)
				sign = KN_avcdec_decode_levelN(ppBitStream, pOffset, vlcnum/*Must be 1*/, &levelabs);
			}
			else
			{
				vlcnum = 0;
				sign = KN_avcdec_decode_level0(ppBitStream, pOffset, &levelabs);
			}

			// level_two_or_higher used when nTrailingOnes <3 because we do "-1" then encoding, so need special process when decoding
			//level_two_or_higher = 1;
			//if (nTotalCoeffs > 3 && nTrailingOnes == 3)
			//{
			//	level_two_or_higher = 0;
			//}
			//if (level_two_or_higher)
			level = (sign)?-levelabs:levelabs;
			if(nTotalCoeffs<=3 || nTrailingOnes!=3)
			{
				// Shoule use condition move
				// TODO: check ASM code
				if (level > 0)
					level ++;
				else
					level --;
			}

			levarr[--nNonOnesCoeffs] = level;		// Here must be --nNonOnesCoeffs: WWD


			// update VLC table
			if (vlcnum == 0)
			{
				vlcnum++;
			}
			if (absm(level)>3)		// First level is special, so, do not use levelabs directly.
				vlcnum = 2;

			// 2.3 all other non-zero coeffs(vlcnum>=1, but <=6)
			for (; nNonOnesCoeffs > 0; )
			{
				//decode current level
				sign = KN_avcdec_decode_levelN(ppBitStream, pOffset, vlcnum, &levelabs);
				level = (sign)?-levelabs:levelabs;
				levarr[--nNonOnesCoeffs] = level;	// Here must be --nNonOnesCoeffs: WWD

				// update VLC table
				if (levelabs>incVlc[vlcnum])
				{
					vlcnum++;
				}
			}
		}
		// Third: decode runs information
		if (nTotalCoeffs < MAX_COEFF_FOR_CHROMA)
		{
			// decode total run
			vlcnum = nTotalCoeffs - 1;

			nTotalZeros = KN_avcdec_TZC(ppBitStream, pOffset, nTotalCoeffs);

			// 3.2 decode run before
			{
				int nRuns;
				i = nTotalCoeffs - 1;
				while(nTotalZeros && i )
				{
					nRuns = KN_avcdec_RunBefore(ppBitStream, pOffset,nTotalZeros);
					runarr[i] = nRuns;
					nTotalZeros -= nRuns;
					i --;
				} 
			}
			runarr[i] = nTotalZeros;

			// 3.3
			coeffNumber = -1;
			for(k=0; k< nTotalCoeffs; k++)
			{
				coeffNumber += runarr[k] +1;
				(*ppDstCoeffs)[coeffNumber] = levarr[k];
			}
		}
		else
		{
			for(k=0; k< MAX_COEFF_FOR_CHROMA; k++)
			{
				(*ppDstCoeffs)[k] = levarr[k];
			}
		}

		// According to Function spec in Ippiman.pdf
		(*ppDstCoeffs) += 4;		 
	}

	Profile_End(ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_profile);

	return ippStsNoErr;
}
#else

typedef unsigned short Element0;
typedef unsigned char  Element1;
typedef unsigned char  Element2;

#define PK0(a, b, c) c<<9|((a>10)&(b<3))<<8|(b<3)<<7|a<<2|b
#define PK1(a, b)	 b <<4 | a
#define PK2(a, b)	 b <<4 | a 

#define UPK0_1(v)	(v>>(2+5+1+1))&0x7F	//num_bits_to_flush				: 7
#define UPK0_2(v)	(v>>(2		))&0x1F	//num_coeff 					: 1
#define UPK0_3(v)	(v>>(0		))&0x03	//num_trailing_ones 			: 1

#define UPK1_1(v)	(v>>4)&0x0F			//num_zeros						:4 
#define UPK1_2(v)	(v   )&0x0F 		//num_bits_to_flush				:4
#define UPK2_1(v)	(v>>4)&0x0F			//num_of_bits_to_flush			:4
#define UPK2_2(v)	(v   )&0x0F			//num_of_zeros_in_this_run		:4

static DECL_ALIGN_8 Element0 const tab0[208] =
{
	//case 0
	PK0( 0, 0,  1),													//0		00[1]	
	PK0( 1, 1,  2),													//1 	01[1]	
	PK0( 2, 2,  3),													//2 	02[1]	
	PK0( 2, 1,  6), PK0( 1, 0,  6), PK0( 3, 3,  5), PK0( 3, 3,  5),	//3 	03[4]	
	PK0( 5, 3,  7), PK0( 3, 2,  7),	PK0( 4, 3,  6), PK0( 4, 3,  6),	//7 	04[4]	
	PK0( 6, 3,  8), PK0( 4, 2,  8), PK0( 3, 1,  8), PK0( 2, 0,  8), //11  	05[4]	
	PK0( 7, 3,  9), PK0( 5, 2,  9), PK0( 4, 1,  9), PK0( 3, 0,  9), //15	06[4]	
	PK0( 8, 3, 10), PK0( 6, 2, 10), PK0( 5, 1, 10), PK0( 4, 0, 10), //19	07[4]	
	PK0( 9, 3, 11), PK0( 7, 2, 11), PK0( 6, 1, 11), PK0( 5, 0, 11), //23	08[4]	
	PK0( 8, 0, 13), PK0( 9, 2, 13),	PK0( 8, 1, 13), PK0( 7, 0, 13),	//27	09[8]	
	PK0(10, 3, 13), PK0( 8, 2, 13),	PK0( 7, 1, 13), PK0( 6, 0, 13),	
	PK0(12, 3, 14), PK0(11, 2, 14),	PK0(10, 1, 14), PK0(10, 0, 14),	//35	10[8]	
	PK0(11, 3, 14), PK0(10, 2, 14), PK0( 9, 1, 14), PK0( 9, 0, 14),		
	PK0(14, 3, 15), PK0(13, 2, 15),	PK0(12, 1, 15), PK0(12, 0, 15),	//43	11[8]	
	PK0(13, 3, 15), PK0(12, 2, 15),	PK0(11, 1, 15), PK0(11, 0, 15),	
	PK0(16, 3, 16), PK0(15, 2, 16),	PK0(15, 1, 16), PK0(14, 0, 16),	//51	12[8]	
	PK0(15, 3, 16), PK0(14, 2, 16),	PK0(14, 1, 16), PK0(13, 0, 16),	
	PK0(16, 0, 16), PK0(16, 2, 16),	PK0(16, 1, 16), PK0(15, 0, 16),	//59	13[4]	
	PK0(13, 1, 15),													//63	14[1]			

	//case 1
	PK0( 1, 1,  2), PK0( 0, 0,  2),									//64	00[2]	
	PK0( 4, 3,  4), PK0( 3, 3,  4),	PK0( 2, 2,  3), PK0( 2, 2,  3),	//66	10[4]	
	PK0( 6, 3,  6), PK0( 3, 2,  6),	PK0( 3, 1,  6), PK0( 1, 0,  6),	PK0( 5, 3,  5), PK0( 5, 3,  5),	PK0( 2, 1,  5), PK0( 2, 1,  5),	//70	02[8]	
	PK0( 7, 3,  6), PK0( 4, 2,  6),	PK0( 4, 1,  6), PK0( 2, 0,  6),	//78	03[4]	
	PK0( 8, 3,  7), PK0( 5, 2,  7),	PK0( 5, 1,  7), PK0( 3, 0,  7),	//82	04[4]	
	PK0( 5, 0,  8), PK0( 6, 2,  8),	PK0( 6, 1,  8), PK0( 4, 0,  8),	//86	05[4]	
	PK0( 9, 3,  9), PK0( 7, 2,  9),	PK0( 7, 1,  9), PK0( 6, 0,  9),	//90	06[4]	
	PK0(11, 3, 11), PK0( 9, 2, 11),	PK0( 9, 1, 11), PK0( 8, 0, 11),	PK0(10, 3, 11), PK0( 8, 2, 11), PK0( 8, 1, 11), PK0( 7, 0, 11),	//94	07[8]
	PK0(11, 0, 12), PK0(11, 2, 12),	PK0(11, 1, 12), PK0(10, 0, 12),	PK0(12, 3, 12), PK0(10, 2, 12),	PK0(10, 1, 12), PK0( 9, 0, 12),	//102 	08[8]	
	PK0(14, 3, 13), PK0(13, 2, 13),	PK0(13, 1, 13), PK0(13, 0, 13),	PK0(13, 3, 13), PK0(12, 2, 13),	PK0(12, 1, 13), PK0(12, 0, 13),	//110 	09[8]
	PK0(15, 1, 14), PK0(15, 0, 14),	PK0(15, 2, 14), PK0(14, 1, 14),	PK0(14, 2, 13), PK0(14, 2, 13),	PK0(14, 0, 13), PK0(14, 0, 13),	//118 	10[8]	
	PK0(16, 3, 14), PK0(16, 2, 14),	PK0(16, 1, 14), PK0(16, 0, 14),	//126 	11[4]	
	PK0(15, 3, 13),													//130	12[1]

	//case 2
	PK0( 7, 3,  4), PK0( 6, 3,  4),	PK0( 5, 3,  4), PK0( 4, 3,  4),	PK0( 3, 3,  4), PK0( 2, 2,  4),	PK0( 1, 1,  4), PK0( 0, 0,  4),	//131 	00[8]
	PK0( 5, 1,  5), PK0( 5, 2,  5),	PK0( 4, 1,  5), PK0( 4, 2,  5),	PK0( 3, 1,  5), PK0( 8, 3,  5),	PK0( 3, 2,  5), PK0( 2, 1,  5),	//139 	01[8]
	PK0( 3, 0,  6), PK0( 7, 2,  6),	PK0( 7, 1,  6), PK0( 2, 0,  6),	PK0( 9, 3,  6), PK0( 6, 2,  6),	PK0( 6, 1,  6), PK0( 1, 0,  6),	//147 	02[8]	
	PK0( 7, 0,  7), PK0( 6, 0,  7),	PK0( 9, 2,  7), PK0( 5, 0,  7),	PK0(10, 3,  7), PK0( 8, 2,  7),	PK0( 8, 1,  7), PK0( 4, 0,  7),	//155 	03[8]	
	PK0(12, 3,  8), PK0(11, 2,  8),	PK0(10, 1,  8), PK0( 9, 0,  8),	PK0(11, 3,  8), PK0(10, 2,  8),	PK0( 9, 1,  8), PK0( 8, 0,  8),	//163 	04[8]	
	PK0(12, 0,  9), PK0(13, 2,  9),	PK0(12, 1,  9), PK0(11, 0,  9),	PK0(13, 3,  9), PK0(12, 2,  9),	PK0(11, 1,  9), PK0(10, 0,  9),	//171 	05[8]	
	PK0(15, 1, 10), PK0(14, 0, 10),	PK0(14, 3, 10), PK0(14, 2, 10),	PK0(14, 1, 10), PK0(13, 0, 10),	PK0(13, 1,  9), PK0(13, 1,  9),	//179 	06[8]	
	PK0(16, 1, 10), PK0(15, 0, 10),	PK0(15, 3, 10), PK0(15, 2, 10),	//187 	07[4]	
	PK0(16, 3, 10), PK0(16, 2, 10),									//191 	08[2]	
	PK0(16, 0, 10),													//193 	09[1]	
	
	/////////////////////////////////////////////////////////////////////////////
	//case chroma_dc.
	PK0( 1, 1,  1),													//194 		
	PK0( 0, 0,  2),													//195 		
	PK0( 2, 2,  3),													//196 		
	PK0( 2, 0,  6), PK0( 3, 3,  6), PK0( 2, 1,  6), PK0( 1, 0,  6), //197 	03[4]	
	PK0( 4, 0,  6), PK0( 3, 0,  6),									//201 	04[2]	
	PK0( 3, 2,  7), PK0( 3, 1,  7),									//203 	05[2]	
	PK0( 4, 2,  8), PK0( 4, 1,  8),									//205 	06[2]	
	PK0( 4, 3,  7)													//207 	07[1]	
};

static DECL_ALIGN_8 Element1 const tab1[251]=
{
	//1								
	PK1( 0, 1), PK1( 0, 1),								//  0	01_0[2]
	PK1( 2, 3), PK1( 1, 3),								//  2	01_1[2]
	PK1( 4, 4), PK1( 3, 4),								//  4	01_2[2]
	PK1( 6, 5), PK1( 5, 5),								//  6	01_3[2]
	PK1( 8, 6), PK1( 7, 6),								//  8	01_4[2]
	PK1(10, 7), PK1( 9, 7),								// 10	01_5[2]
	PK1(12, 8), PK1(11, 8),								// 12	01_6[2]
	PK1(14, 9), PK1(13, 9),								// 14	01_7[2]
	PK1(15, 9), PK1(15, 9),								// 16	01_8[2]
	
	//2								
	PK1( 3, 3), PK1( 2, 3),	PK1( 1, 3), PK1( 0, 3),		// 18	02_0[4]
	PK1( 6, 4), PK1( 5, 4),	PK1( 4, 3), PK1( 4, 3),		// 22	02_1[4]				
	PK1( 8, 4), PK1( 8, 4),	PK1( 7, 4), PK1( 7, 4),		// 26	02_2[4]
	PK1(10, 5), PK1(10, 5),	PK1( 9, 5), PK1( 9, 5),		// 30	02_3[4]				
	PK1(12, 6), PK1(12, 6),	PK1(11, 6), PK1(11, 6),		// 34	02_4[4]				
	PK1(13, 6), PK1(13, 6),	PK1(13, 6), PK1(13, 6),		// 38	02_5[4]				
	PK1(14, 6), PK1(14, 6),	PK1(14, 6), PK1(14, 6),		// 42	02_6[4]

	//3									
	PK1( 6, 3), PK1( 3, 3),	PK1( 2, 3), PK1( 1, 3),		// 46	03_0[4]	
	PK1( 4, 4), PK1( 0, 4),	PK1( 7, 3), PK1( 7, 3),		// 50	03_1[4]				
	PK1( 8, 4), PK1( 8, 4),	PK1( 5, 4), PK1( 5, 4),		// 54	03_2[4]			
	PK1(10, 5), PK1(10, 5),	PK1( 9, 5), PK1( 9, 5),		// 58	03_3[4]			
	PK1(12, 5), PK1(12, 5),	PK1(12, 5), PK1(12, 5),		// 62	03_4[4]				
	PK1(11, 6), PK1(11, 6),	PK1(11, 6), PK1(11, 6),		// 66	03_5[4]				
	PK1(13, 6), PK1(13, 6),	PK1(13, 6), PK1(13, 6),		// 70	03_6[4]
	
	//4		
	PK1( 6, 3), PK1( 5, 3),	PK1( 4, 3), PK1( 1, 3),		// 74	04_0[4]				
	PK1( 3, 4), PK1( 2, 4),	PK1( 8, 3), PK1( 8, 3),		// 78	04_1[4]				
	PK1( 9, 4), PK1( 9, 4),	PK1( 7, 4), PK1( 7, 4),		// 82	04_2[4]				
	PK1(10, 5), PK1(10, 5),	PK1( 0, 5), PK1( 0, 5),		// 86	04_3[4]				
	PK1(11, 5), PK1(11, 5),	PK1(11, 5), PK1(11, 5),		// 90	04_4[4]				
	PK1(12, 5), PK1(12, 5),	PK1(12, 5), PK1(12, 5),		// 94	04_5[4]
	
	//5								
	PK1( 6, 3), PK1( 5, 3),	PK1( 4, 3), PK1( 3, 3),		// 98	05_0[4]
	PK1( 1, 4), PK1( 0, 4), PK1( 7, 3), PK1( 7, 3),		//102	05_1[4]				
	PK1( 8, 4), PK1( 8, 4),	PK1( 2, 4), PK1( 2, 4),		//106	05_2[4]
	PK1(10, 4), PK1(10, 4),	PK1(10, 4), PK1(10, 4),		//110	05_3[4]
	PK1( 9, 5), PK1( 9, 5),	PK1( 9, 5), PK1( 9, 5),		//114	05_4[4]
	PK1(11, 5), PK1(11, 5),	PK1(11, 5), PK1(11, 5),		//118	05_5[4]

	//6
	PK1( 5, 3), PK1( 4, 3),	PK1( 3, 3), PK1( 2, 3),		//122	06_0[4]
	PK1( 7, 3), PK1( 7, 3),	PK1( 6, 3), PK1( 6, 3),		//126	06_1[4]
	PK1( 9, 3), PK1( 9, 3),	PK1( 9, 3), PK1( 9, 3),		//130	06_2[4]
	PK1( 8, 4), PK1( 8, 4),	PK1( 8, 4), PK1( 8, 4),		//134	06_3[4]
	PK1( 1, 5), PK1( 1, 5),	PK1( 1, 5), PK1( 1, 5),		//138	06_4[4]
	PK1( 0, 6), PK1( 0, 6),	PK1( 0, 6), PK1( 0, 6),		//142	06_5[4]
	PK1(10, 6), PK1(10, 6),	PK1(10, 6), PK1(10, 6),		//146	06_6[4]

	//7
	PK1( 3, 3), PK1( 2, 3),	PK1( 5, 2), PK1( 5, 2),		//150	07_0[4]
	PK1( 6, 3), PK1( 6, 3),	PK1( 4, 3), PK1( 4, 3),		//154	07_1[4]
	PK1( 8, 3), PK1( 8, 3),	PK1( 8, 3), PK1( 8, 3),		//158	07_2[4]
	PK1( 7, 4), PK1( 7, 4),	PK1( 7, 4), PK1( 7, 4),		//162	07_3[4]
	PK1( 1, 5), PK1( 1, 5),	PK1( 1, 5), PK1( 1, 5),		//166	07_4[4]
	PK1( 0, 6), PK1( 0, 6),	PK1( 0, 6), PK1( 0, 6),		//170	07_5[4]
	PK1( 9, 6), PK1( 9, 6),	PK1( 9, 6), PK1( 9, 6),		//174	07_6[4]
	
	//8
	PK1( 5, 2), PK1( 4, 2), 		//178	08_0[2]
	PK1( 6, 3), PK1( 3, 3), 		//180	08_1[2]
	PK1( 7, 3), PK1( 7, 3), 		//182	08_2[2]
	PK1( 1, 4), PK1( 1, 4), 		//184	08_3[2]
	PK1( 2, 5), PK1( 2, 5), 		//186	08_4[2]
	PK1( 0, 6), PK1( 0, 6), 		//188	08_5[2]
	PK1( 8, 6), PK1( 8, 6), 		//190	08_6[2]
	

	//9
	PK1( 4, 2), PK1( 3, 2), 		//192	09_0[2]
	PK1( 6, 2), PK1( 6, 2), 		//194	09_1[2]
	PK1( 5, 3), PK1( 5, 3), 		//196	09_2[2]
	PK1( 2, 4), PK1( 2, 4), 		//198	09_3[2]
	PK1( 7, 5), PK1( 7, 5), 		//200	09_4[2]
	PK1( 0, 6), PK1( 0, 6), 		//202	09_5[2]
	PK1( 1, 6), PK1( 1, 6), 		//204	09_6[2]
	

	//10
	PK1( 4, 2), PK1( 3, 2), 		//206	10_0[2]
	PK1( 5, 2), PK1( 5, 2), 		//208	10_1[2]
	PK1( 2, 3), PK1( 2, 3), 		//210	10_2[2]
	PK1( 6, 4), PK1( 6, 4), 		//212	10_3[2]
	PK1( 0, 5), PK1( 0, 5), 		//214	10_4[2]
	PK1( 1, 5), PK1( 1, 5), 		//216	10_5[2]
	
	//11
	PK1( 4, 1), PK1( 4, 1), 		//218	11_0[2]
	PK1( 3, 3), PK1( 5, 3), 		//220	11_1[2]
	PK1( 2, 3), PK1( 2, 3), 		//222	11_2[2]
	PK1( 1, 4), PK1( 1, 4), 		//224	11_3[2]
	PK1( 0, 4), PK1( 0, 4), 		//226	11_4[2]	

	//12
	PK1( 3, 1),						//228	12_0[1]
	PK1( 2, 2),						//229	12_1[1]
	PK1( 4, 3),						//230	12_2[1]
	PK1( 1, 4),						//231	12_3[1]
	PK1( 0, 4),						//232	12_4[1]

	//13
	PK1( 2, 1),						//233	13_0[1]
	PK1( 3, 2),						//234	13_1[1]
	PK1( 1, 3),						//235	13_2[1]
	PK1( 0, 3),						//236	13_3[1]

	//14
	PK1( 2, 1),						//237	14_0[1]
	PK1( 1, 2),						//238	14_1[1]
	PK1( 0, 2),						//239	14_2[1]

	//15
	PK1( 1, 1),						//240	15_0[1]
	PK1( 0, 1),						//241	15_1[1]

	/////////////////////////////////////////////////////////
	//chroma_dc  1
	PK1( 0, 1),						//242	01_0_chroma_dc[1]	
	PK1( 1, 2),						//243	01_1_chroma_dc[1]	
	PK1( 2, 3),						//244	01_2_chroma_dc[1]	
	PK1( 3, 3),						//245	01_3_chroma_dc[1]	

	//chroma_dc  2
	PK1( 0, 1),						//246	02_0_chroma_dc[1]	
	PK1( 1, 2),						//247	02_1_chroma_dc[1]	
	PK1( 2, 2),						//248	02_2_chroma_dc[1]	

	//chroma_dc  3						
	PK1( 0, 1),						//249	03_0_chroma_dc[1]	
	PK1( 1, 1),						//250	03_1_chroma_dc[1]	
};

static DECL_ALIGN_8 Element2 const tab2[88]=
{
	//1
	PK2( 0, 1),						// 0	1_0[1]	
	PK2( 1, 1),						// 1	1_1[1]
									
	//2
	PK2( 0, 1),						// 2	2_0[1]
	PK2( 1, 2),						// 3	2_1[1]					
	PK2( 2, 2),						// 4	2_2[1]
											
	//3
	PK2( 1, 2), PK2( 0, 2),			// 5	3_0[2]
	PK2( 2, 2), PK2( 2, 2),			// 7	3_1[2]
	PK2( 3, 2), PK2( 3, 2),			// 9	3_2[2]

	//4		
	PK2( 1, 2), PK2( 0, 2),			//11	4_0[2]
	PK2( 2, 2), PK2( 2, 2),			//13	4_1[2]
	PK2( 3, 3), PK2( 3, 3),			//15	4_2[2]
	PK2( 4, 3), PK2( 4, 3),			//17	4_3[2]

	//5
	PK2( 1, 2), PK2( 0, 2),			//19	5_0[2]
	PK2( 3, 3), PK2( 2, 3),			//21	5_1[2]
	PK2( 4, 3), PK2( 4, 3),			//23	5_2[2]
	PK2( 5, 3), PK2( 5, 3),			//25	5_3[2]
											
	//6
	PK2( 6, 3), PK2( 5, 3), PK2( 0, 2), PK2( 0, 2),			//27    6_0[4]
	PK2( 4, 3), PK2( 4, 3), PK2( 3, 3), PK2( 3, 3),			 //31    6_1[4]
	PK2( 2, 3), PK2( 2, 3), PK2( 2, 3), PK2( 2, 3),		    //35    6_2[4]
	PK2( 1, 3), PK2( 1, 3), PK2( 1, 3), PK2( 1, 3),		    //39    6_3[4]
											
	//7
	PK2( 3, 3), PK2( 2, 3), PK2( 1, 3), PK2( 0, 3),		    //43    7_00[4]
	PK2( 5, 3), PK2( 5, 3), PK2( 4, 3), PK2( 4, 3),		    //47    7_01[4]
	PK2( 6, 3), PK2( 6, 3), PK2( 6, 3), PK2( 6, 3),		    //51    7_02[4]
	PK2( 7, 4), PK2( 7, 4), PK2( 7, 4), PK2( 7, 4),		    //55    7_03[4]
	PK2( 8, 5), PK2( 8, 5), PK2( 8, 5), PK2( 8, 5),		    //59    7_04[4]
	PK2( 9, 6), PK2( 9, 6), PK2( 9, 6), PK2( 9, 6),		    //63    7_05[4]
	PK2(10, 7), PK2(10, 7), PK2(10, 7), PK2(10, 7),		    //67    7_06[4]
	PK2(11, 8), PK2(11, 8), PK2(11, 8), PK2(11, 8),		    //71    7_07[4]
	PK2(12, 9), PK2(12, 9), PK2(12, 9), PK2(12, 9),		    //75    7_08[4]
	PK2(13,10), PK2(13,10), PK2(13,10), PK2(13,10),		    //79    7_09[4]
	PK2(14,11), PK2(14,11), PK2(14,11), PK2(14,11),		    //83    7_10[4]
};											

typedef struct Rec0
{
	unsigned char	offset;
	unsigned char 	num_idx;
}Rec0;

typedef struct Rec1
{
	unsigned char * offsets;
	unsigned int	force1bit_num_idx;
}Rec1;

typedef struct Rec2
{
	unsigned char * offsets;
	unsigned int	force1bit_num_idx;
}Rec2;

static unsigned char const rect0_ary_offset[3] = { 0, 15, 28 };
static DECL_ALIGN_8 Rec0 const rect0_ary[38] =
{
	//rec00[15]=
	{ 0, -0 + 32},	{ 1, -0 + 32},	{ 2, -0 + 32},	{ 3, -2 + 32},
	{ 7, -2 + 32},	{11, -2 + 32},	{15, -2 + 32},	{19, -2 + 32},
	{23, -2 + 32},	{27, -3 + 32},	{35, -3 + 32},	{43, -3 + 32},
	{51, -3 + 32},	{59, -2 + 32},	{63, -0 + 32},

	//rec01[13]
	{ 64, -1 + 32},	{ 66, -2 + 32},	{ 70, -3 + 32},	{ 78, -2 + 32},
	{ 82, -2 + 32},	{ 86, -2 + 32},	{ 90, -2 + 32},	{ 94, -3 + 32},
	{102, -3 + 32},	{110, -3 + 32},	{118, -3 + 32},	{126, -2 + 32},
	{130, -0 + 32},

	//rec02[10]
	{131, -3 + 32},	{139, -3 + 32},	{147, -3 + 32},	{155, -3 + 32},
	{163, -3 + 32},	{171, -3 + 32},	{179, -3 + 32},	{187, -2 + 32},
	{191, -1 + 32},	{193, -0 + 32}
};

static DECL_ALIGN_8 Rec0 const rec0_cd[20]=
{
	{194, -0 + 32},	{195, -0 + 32},	{196, -0 + 32},	{197, -2 + 32},
	{201, -1 + 32},	{203, -1 + 32},	{205, -1 + 32},	{207, -0 + 32},	
	{207, -0 + 32},

	{0,0},	{0,0},	{0,0},	{0,0},
	{0,0},	{0,0},	{0,0},	{0,0},
	{0,0},	{0,0},	{0,0}
};

unsigned char  tab1_01[9] = {0, 2,	4, 6, 8, 10, 12, 14, 16};		
unsigned char  tab1_02[7] = {18, 22, 26, 30, 34, 38, 42};		
unsigned char  tab1_03[7] = {46, 50, 54, 58, 62, 66, 70};
unsigned char  tab1_04[6] = {74, 78, 82, 86, 90, 94};		
unsigned char  tab1_05[6] = {98, 102, 106,	110, 114,  118};
unsigned char  tab1_06[7] = {122, 126, 130, 134, 138, 142,	146};		
unsigned char  tab1_07[7] = {150, 154, 158, 162, 166, 170,	174};
unsigned char  tab1_08[7] = {178, 180, 182, 184, 186, 188,	190};		
unsigned char  tab1_09[7] = {192, 194, 196, 198, 200, 202,	204};		
unsigned char  tab1_10[6] = {206, 208, 210, 212, 214, 216};
unsigned char  tab1_11[5] = {218, 220, 222, 224, 226};		
unsigned char  tab1_12[5] = {228, 229, 230, 231, 232};		
unsigned char  tab1_13[4] = {233, 234, 235, 236};		
unsigned char  tab1_14[3] = {237, 238, 239};		
unsigned char  tab1_15[2] = {240, 241};	


unsigned char  tab2_1[2]  = { 0, 1};
unsigned char  tab2_2[3]  = { 2, 3, 4};
unsigned char  tab2_3[3]  = { 5, 7, 9};
unsigned char  tab2_4[4]  = { 11, 13, 15, 17};
unsigned char  tab2_5[4]  = { 19, 21, 23, 25};
unsigned char  tab2_6[4]  = { 27, 31, 35, 39};
unsigned char  tab2_7[11] = { 43, 47, 51, 55,	59,	63,	67,	71,	75,	79,	83};

Rec1   rec_ary1[15];
Rec2   rec_ary2[7];

unsigned char  tab1_01_cd[4] = {242, 243,	244, 245};
unsigned char  tab1_02_cd[3] = {246, 247,	248};		
unsigned char  tab1_03_cd[2] = {249, 250};

Rec1   rec_ary1_cd[3];

/*
static struct RecAry
{
	Rec0 const* const	rec_ary0[3];
	Rec1 const			rec_ary1[15];
	Rec1 const			rec_ary1_cd[3];
	Rec2 const			rec_ary2[7];
}const rec_ary =
{
	{
		rec00,
		rec01,
		rec02
	},

	{	//{NULL, 0},
		{tab1_01, 1<<(-10+31)| ( -1 + 32)},
		{tab1_02, 1<<( -6+31)| ( -2 + 32)},
		{tab1_03, 1<<( -6+31)| ( -2 + 32)},
		{tab1_04, 1<<( -5+31)| ( -2 + 32)},
		{tab1_05, 1<<( -5+31)| ( -2 + 32)},
		{tab1_06, 1<<( -6+31)| ( -2 + 32)},
		{tab1_07, 1<<( -6+31)| ( -2 + 32)},
		{tab1_08, 1<<( -6+31)| ( -1 + 32)},
		{tab1_09, 1<<( -6+31)| ( -1 + 32)},
		{tab1_10, 1<<( -5+31)| ( -1 + 32)},
		{tab1_11, 1<<( -4+31)| ( -1 + 32)},
		{tab1_12, 1<<( -4+31)| ( -0 + 32)},
		{tab1_13, 1<<( -3+31)| ( -0 + 32)},
		{tab1_14, 1<<( -2+31)| ( -0 + 32)},
		{tab1_15, 1<<( -1+31)| ( -0 + 32)}
	},

	{	//{NULL, 0},
		{tab1_01_cd, 1<<( -3+31)| ( -0 + 32)},
		{tab1_02_cd, 1<<( -2+31)| ( -0 + 32)},
		{tab1_03_cd, 1<<( -1+31)| ( -0 + 32)}
	},

	{	//{NULL, 0}, 
 		{tab2_1, 1<<( -1+31)| ( -0 + 32)},
		{tab2_2, 1<<( -2+31)| ( -0 + 32)},
		{tab2_3, 1<<( -2+31)| ( -1 + 32)},
		{tab2_4, 1<<( -3+31)| ( -1 + 32)},
		{tab2_5, 1<<( -3+31)| ( -1 + 32)},
		{tab2_6, 1<<( -3+31)| ( -2 + 32)},
		{tab2_7, 1<<(-10+31)| ( -2 + 32)},
	}
};
*/

void Init_AVC_CAVLC(void)
{
	rec_ary1[ 0].offsets = tab1_01;
	rec_ary1[ 1].offsets = tab1_02;
	rec_ary1[ 2].offsets = tab1_03;
	rec_ary1[ 3].offsets = tab1_04;
	rec_ary1[ 4].offsets = tab1_05;
	rec_ary1[ 5].offsets = tab1_06;
	rec_ary1[ 6].offsets = tab1_07;
	rec_ary1[ 7].offsets = tab1_08;
	rec_ary1[ 8].offsets = tab1_09;
	rec_ary1[ 9].offsets = tab1_10;
	rec_ary1[10].offsets = tab1_11;
	rec_ary1[11].offsets = tab1_12;
	rec_ary1[12].offsets = tab1_13;
	rec_ary1[13].offsets = tab1_14;
	rec_ary1[14].offsets = tab1_15;

	rec_ary1[ 0].force1bit_num_idx = 1<<(-10+31)| ( -1 + 32);
	rec_ary1[ 1].force1bit_num_idx = 1<<( -6+31)| ( -2 + 32);
	rec_ary1[ 2].force1bit_num_idx = 1<<( -6+31)| ( -2 + 32);
	rec_ary1[ 3].force1bit_num_idx = 1<<( -5+31)| ( -2 + 32);
	rec_ary1[ 4].force1bit_num_idx = 1<<( -5+31)| ( -2 + 32);
	rec_ary1[ 5].force1bit_num_idx = 1<<( -6+31)| ( -2 + 32);
	rec_ary1[ 6].force1bit_num_idx = 1<<( -6+31)| ( -2 + 32);
	rec_ary1[ 7].force1bit_num_idx = 1<<( -6+31)| ( -1 + 32);
	rec_ary1[ 8].force1bit_num_idx = 1<<( -6+31)| ( -1 + 32);
	rec_ary1[ 9].force1bit_num_idx = 1<<( -5+31)| ( -1 + 32);
	rec_ary1[10].force1bit_num_idx = 1<<( -4+31)| ( -1 + 32);
	rec_ary1[11].force1bit_num_idx = 1<<( -4+31)| ( -0 + 32);
	rec_ary1[12].force1bit_num_idx = 1<<( -3+31)| ( -0 + 32);
	rec_ary1[13].force1bit_num_idx = 1<<( -2+31)| ( -0 + 32);
	rec_ary1[14].force1bit_num_idx = 1<<( -1+31)| ( -0 + 32);


	rec_ary1_cd[ 0].offsets = tab1_01_cd;
	rec_ary1_cd[ 1].offsets = tab1_02_cd;
	rec_ary1_cd[ 2].offsets = tab1_03_cd;

	rec_ary1_cd[ 0].force1bit_num_idx = 1<<( -3+31)| ( -0 + 32);
	rec_ary1_cd[ 1].force1bit_num_idx = 1<<( -2+31)| ( -0 + 32);
	rec_ary1_cd[ 2].force1bit_num_idx = 1<<( -1+31)| ( -0 + 32);


	rec_ary2[ 0].offsets = tab2_1;
	rec_ary2[ 1].offsets = tab2_2;
	rec_ary2[ 2].offsets = tab2_3;
	rec_ary2[ 3].offsets = tab2_4;
	rec_ary2[ 4].offsets = tab2_5;
	rec_ary2[ 5].offsets = tab2_6;
	rec_ary2[ 6].offsets = tab2_7;

	rec_ary2[ 0].force1bit_num_idx = 1<<( -1+31)| ( -0 + 32);
	rec_ary2[ 1].force1bit_num_idx = 1<<( -2+31)| ( -0 + 32);
	rec_ary2[ 2].force1bit_num_idx = 1<<( -2+31)| ( -1 + 32);
	rec_ary2[ 3].force1bit_num_idx = 1<<( -3+31)| ( -1 + 32);
	rec_ary2[ 4].force1bit_num_idx = 1<<( -3+31)| ( -1 + 32);
	rec_ary2[ 5].force1bit_num_idx = 1<<( -3+31)| ( -2 + 32);
	rec_ary2[ 6].force1bit_num_idx = 1<<(-10+31)| ( -2 + 32);
};

unsigned short tab3[6] = {3, 6, 12, 24, 48, 0xFFFF};

unsigned char  const pin_to_7[14] = {1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7};

#define BITSTREAM_LOCAL					\
	Ipp32u* p_stream;					\
	Ipp32u	four_bytes;					\
	int		bit_offset;					\
	int		bit_avail;

// bit_offset refer to bits used till now (From high bit)
#define START_BITSTREAM_LOCAL			\
{										\
	p_stream   = *ppBitStream;			\
	bit_offset = 31 - (*pBitOffset);	\
	bit_avail  = 0;						\
}

#define FINISH_BITSTREAM_LOCAL			\
{										\
	*ppBitStream  = p_stream;			\
	*pBitOffset   = 31 - bit_offset;	\
}

#if !defined( _WIN32_WCE )
#define LOAD_32BITS									\
{													\
	Ipp32u next_four;								\
													\
	four_bytes = *p_stream;							\
	next_four  = *(p_stream+1);						\
													\
	if(	 bit_offset != 0 )							\
	{												\
		four_bytes <<= bit_offset;					\
		four_bytes |= next_four>>(32-bit_offset);	\
	}												\
	bit_avail = 32;									\
}	
#else
#define LOAD_32BITS									\
{													\
	Ipp32u next_four;								\
													\
	four_bytes = *p_stream;							\
	next_four  = *(p_stream+1);						\
													\
	{												\
		four_bytes <<= bit_offset;					\
		four_bytes |= next_four>>(32-bit_offset);	\
	}												\
	bit_avail = 32;									\
}	
#endif

//TODO: FLUSH_BITS(n) should be called in LOAD_32BITS only
#define FLUSH_BITS(n)								\
{													\
	four_bytes <<= n;								\
	bit_avail -= n;									\
	{												\
		int word_offset;							\
		int tmp;									\
													\
		tmp			= bit_offset + n;				\
		word_offset = (tmp>>5);						\
		bit_offset	= (tmp&0x1f);					\
		p_stream    += word_offset;					\
	}												\
}	

//***remove all leading zero and the first encountered one
#define COUNT_LEADING_ZERO(value)					\
{													\
	LOAD_32BITS										\
	value = CNTLZ(four_bytes);						\
	FLUSH_BITS(value+1);							\
}

#define PEEK_BITS(n, value)							\
{													\
	if( bit_avail < n )								\
		LOAD_32BITS									\
													\
	value = four_bytes>>(32 - n );					\
													\
}

#define INITIAL_PEEK_BITS(n, value)					\
{													\
    LOAD_32BITS                                     \
    value = four_bytes>>(32 - n );					\
}

#define READ_BITS(n, value)							\
{													\
	if( bit_avail < n )								\
		LOAD_32BITS									\
													\
	value = four_bytes>>(32 - n );					\
													\
	FLUSH_BITS(n)									\
}

 /*
 **************************************************************************
 * Function:    ippiDecodeCAVLCCoeffs_H264_1u16s_c
 * Description: Do CAVLC decoding
 *
 * Problems   : 
 *
 * 
 **************************************************************************
 */
static unsigned char const 
vlc_select_tab[12]= { 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3};

IppStatus __STDCALL 
ippiDecodeCAVLCCoeffs_H264_1u16s_c
(
	Ipp32u **ppBitStream,
	Ipp32s *pBitOffset,
	Ipp16s *pNumCoeff,
	Ipp16s **ppDstCoeffs,
	Ipp32u uVLCSelect,
	Ipp16s uMaxNumCoeff,
	const Ipp32s **ppTblCoeffToken_whocars,
	const Ipp32s **ppTblTotalZeros_whocars,
	const Ipp32s **ppTblRunBefore_whocars,
	const Ipp32s *pScanMatrix 
)
{
	Profile_Start(ippiDecodeCAVLCCoeffs_H264_1u16s_c_profile);

	BITSTREAM_LOCAL
	START_BITSTREAM_LOCAL

	IppStatus	err = ippStsNoErr;
	int		i,	k;
	int		num_trailing_ones;
	int		numones, totzeros, level;
	int		zerosleft;
	int		code;
	int		retval;
	__ALIGN8(int, tmp0, 32);
	int		*levarr = &tmp0[0];
	int		*runarr = &tmp0[16];
	int		coeffNumber;
	int		num_coeff = 0;
	int		max_coeff_num = uMaxNumCoeff;
	int		numcoeff_vlc = vlc_select_tab[uVLCSelect]; 	// numcoeff_vlc is the index of Table used to code coeff_token
														// numcoeff_vlc==3 means (8<=nC) which uses 6bit FLC
	unsigned int next_bits;
	unsigned int num_levels;
	int			 flush_n_bits;
	INITIAL_PEEK_BITS(32, next_bits)

	if( uVLCSelect > 11 )
		uVLCSelect = 11;

	numcoeff_vlc = vlc_select_tab[uVLCSelect]; 	// numcoeff_vlc is the index of Table used to code coeff_token

	if (numcoeff_vlc == 3)
	{	// read 6 bit FLC
		code = next_bits>>26;			
		num_trailing_ones = code & 3;	
		num_coeff = (code >> 2);

		if (!num_coeff && num_trailing_ones == 3)
			num_trailing_ones = 0;// #c = 0, #t1 = 3 =>  #c = 0
		else
			num_coeff++;

		retval = 0;
		
		flush_n_bits = 6;
	}
	else
	{
		unsigned int	next_bits_1		  = next_bits|(1<<15);
		unsigned int	num_leading_zeros = CNTLZ(next_bits_1); //ippiDecodeCAVLCCoeffs_H264_1u16s_c

		next_bits_1  <<= 1;
		next_bits_1  <<= num_leading_zeros;

		Rec0	const*  rec	= rect0_ary + rect0_ary_offset[numcoeff_vlc];
		unsigned int		num_idx	= rec[num_leading_zeros].num_idx;
		unsigned short	const*  tab	= rec[num_leading_zeros].offset + tab0;
		unsigned int			idx	= (next_bits_1>>num_idx);
#ifndef _WIN32_WCE
		if(num_idx>=32) 
			idx = 0;
#endif
		unsigned short r	= tab[idx];
		flush_n_bits		= UPK0_1( r );
		num_coeff			= UPK0_2( r );
		num_trailing_ones	= UPK0_3( r );
	}

	numones		= num_trailing_ones;
	*pNumCoeff	= num_coeff;
	if (!num_coeff)
	{
		FLUSH_BITS(flush_n_bits)	
		goto bail;
	}

	ippsZero_32s_x( tmp0, 32 );	//clear levarr, runarr
	ippsZero_16s_x( (*ppDstCoeffs), 16 );

	// Get the sign of trailing ones
	if (num_trailing_ones)
	{
		flush_n_bits += num_trailing_ones;
		code = next_bits>>(32-flush_n_bits);

		int *tmp_lev = levarr + num_coeff - num_trailing_ones;
		for (k = 0; k < num_trailing_ones; k++)
		{
			if ((code>>k)&1)
				tmp_lev[0] = -1;
			else
				tmp_lev[0] = 1;
			tmp_lev++;
		}
	}
	FLUSH_BITS(flush_n_bits)	

	num_levels = num_coeff - num_trailing_ones;
	if(num_levels>0)
	{
		unsigned short const* tab = tab3;
		int			  ones_less_than_3;
		unsigned int		  offset_base;
		int			  width, with_is_0;

		ones_less_than_3 = num_trailing_ones<3;
		offset_base      = 2*ones_less_than_3 + 2;
		width			 = num_coeff>10&&ones_less_than_3;
		with_is_0		 = 1^(width);

		// Changed from tab = tab3 - 1;
		tab --;
		for (k = num_levels - 1 ; k >= 0; k--)
		{
			int offset;
			int bits;

			PEEK_BITS(32, next_bits)				
			int num_leading_zeros = CNTLZ(next_bits); //ippiDecodeCAVLCCoeffs_H264_1u16s_c
			
			if(num_leading_zeros < 15 )
			{
				next_bits <<= 1;
				next_bits <<= num_leading_zeros;

				flush_n_bits = (num_leading_zeros + 1 + width);
				bits		 = next_bits>>(32-width);

#ifndef _WIN32_WCE
				if((32-width)>=32) 
					bits = 0;
#endif

				offset=offset_base+ (num_leading_zeros<<width);
				if(width==0)
				{
					bits     = num_leading_zeros;
					offset = offset_base;
					if(num_leading_zeros==14)
					{
						bits		  = next_bits>>28;
						offset		  = offset_base + 14;
						flush_n_bits += 4;
					}
				}
			}
			else
			{
				bits	   = ( next_bits>>4 )&0xFFF;
				offset =  offset_base+( 15<<(width+with_is_0) );
				flush_n_bits = 28;
			}
			
			int mag = (bits+offset)>>1;					
			int sig = -( (int)(bits&0x01) );				
			level   = (mag^sig)-sig;	

			FLUSH_BITS(flush_n_bits);

			width += with_is_0;
			width += mag > tab[width];

			offset_base	= 2;
			with_is_0	= false;		
			levarr[k]	= level;
		}
	}

	if (num_coeff < max_coeff_num)
	{	// decode total run
		Rec1   const*	rec		=&(rec_ary1[num_coeff-1]);
		unsigned int force1bit_num_idx  = rec->force1bit_num_idx;
		unsigned int			num_idx = force1bit_num_idx&0xFF;	
		unsigned char const* tab_ary = rec->offsets;

		unsigned int next_bits;
		PEEK_BITS(32, next_bits)
		next_bits		|= force1bit_num_idx;
		unsigned int tab_idx	 = CNTLZ(next_bits); //ippiDecodeCAVLCCoeffs_H264_1u16s_c
		
		next_bits <<= 1;
		next_bits <<= tab_idx;

		Element1 const*	tab = tab_ary[tab_idx]+tab1;
		unsigned int			idx = next_bits>>num_idx;
#ifndef _WIN32_WCE
		if(num_idx>=32) 
			idx = 0;
#endif
		Element1 e	 = tab[idx];
		unsigned int	 f	 = UPK1_1(e);
		totzeros	 = UPK1_2(e);

		FLUSH_BITS(f);
	}
	else
		totzeros = 0;

	// decode run before each coefficient
	zerosleft = totzeros;
	i = num_coeff-1;
	if (zerosleft > 0 && i > 0)
		do 
		{
			unsigned int numZerosInThisRun = 0;
			if( totzeros > 0 )
			{
				unsigned int pinnedNumZerosLeftToDistribute = pin_to_7[totzeros-1];

				Rec2 const*			 rec	= &rec_ary2[pinnedNumZerosLeftToDistribute-1];
				unsigned char const* tables	= rec->offsets;
				unsigned int		 force1bit_num_idx	= rec->force1bit_num_idx;
				unsigned int		 num_idx	= force1bit_num_idx&0xFF;

				unsigned int next_bits;
				PEEK_BITS(32, next_bits )
				next_bits |= force1bit_num_idx;
				
				unsigned int tab_idx = CNTLZ(next_bits); //ippiDecodeCAVLCCoeffs_H264_1u16s_c
				next_bits <<= 1;
				next_bits <<= tab_idx;

				Element2 const* tab = tables[tab_idx]+tab2;
				unsigned int idx = next_bits>>num_idx;
#ifndef _WIN32_WCE
				if(num_idx>=32) 
					idx = 0;
#endif
				int numZerosInThisRun_Record = (int)tab[idx];
				unsigned int flush_n_bits	 = UPK2_1(numZerosInThisRun_Record);
				numZerosInThisRun			 = UPK2_2(numZerosInThisRun_Record);
				FLUSH_BITS(flush_n_bits);
				totzeros -= numZerosInThisRun;
			}

			runarr[i] = numZerosInThisRun;
			zerosleft -= runarr[i];
			i --;

		}while (zerosleft > 0 && i != 0);

	if( zerosleft < 0 )
		zerosleft = 0;

	runarr[i] = zerosleft;

	// The last step : SCAN order
	if(uMaxNumCoeff == 16)		// Luma
		coeffNumber = -1;
	else
		coeffNumber = 0;		// Chroma AC part, We do not use [0] element WWD 

	for(k=0; k< num_coeff; k++)
	{
		coeffNumber += runarr[k] +1;
		(*ppDstCoeffs)[pScanMatrix[coeffNumber]] = levarr[k];
	}

	// According to Function spec in Ippiman.pdf
	(*ppDstCoeffs) += 16;		 

bail:
	FINISH_BITSTREAM_LOCAL

	Profile_End(ippiDecodeCAVLCCoeffs_H264_1u16s_c_profile);

	return err;
}


IppStatus __STDCALL ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c
(
	Ipp32u **ppBitStream,
	Ipp32s *pBitOffset,
	Ipp16s *pNumCoeff,
	Ipp16s **ppDstCoeffs,
	const Ipp32s *pTblCoeffToken_whocares,
	const Ipp32s **ppTblTotalZerosCR_whocares,
	const Ipp32s **ppTblRunBeforeB_whocares 
)
{
	Profile_Start(ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_profile);

	BITSTREAM_LOCAL
	START_BITSTREAM_LOCAL

	int		i,	k;
	int		num_coeff, num_trailing_ones;
	int		numones, totzeros, level;
	int		zerosleft;
	
	int		code;
	__ALIGN8(int, tmp0, 8);
	int		*levarr = &tmp0[0];
	int		*runarr = &tmp0[4];

	int		coeffNumber;
	IppStatus err =	ippStsNoErr;
	unsigned int  num_levels;

	num_coeff = 0;
	unsigned int next_bits;
	int  flush_n_bits;
	{
		INITIAL_PEEK_BITS(32, next_bits)
		unsigned int	next_bits_1		  = next_bits|(1<<23);
		unsigned int	num_leading_zeros = CNTLZ(next_bits_1); //ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c

		next_bits_1  <<= 1;
		next_bits_1  <<= num_leading_zeros;

		unsigned int		num_idx	= rec0_cd[num_leading_zeros].num_idx;
		unsigned short	const*  tab	= rec0_cd[num_leading_zeros].offset + tab0;
		unsigned int			idx	= (next_bits_1>>num_idx);

#ifndef _WIN32_WCE
		if(num_idx>=32) 
			idx = 0;
#endif
		int	r   = (int)tab[idx];
		flush_n_bits= UPK0_1( r );
		num_coeff	= UPK0_2( r );
		num_trailing_ones	= UPK0_3( r );	
	}
	
	numones = num_trailing_ones;
	*pNumCoeff = num_coeff;

	if (!num_coeff)
	{
		FLUSH_BITS(flush_n_bits)		
		goto bail;
	}

	ippsZero_32s_x( tmp0, 8 );	//clear levarr, runarr
	ippsZero_16s_x( (*ppDstCoeffs), 4 );

	//for (k = 0; k < 4; k++)
	//{
	//	levarr[k] = 0;
	//	runarr[k] = 0;
//
	//	(*ppDstCoeffs)[k] = 0;
	//}

	// Get the sign of trailing ones
	if (num_trailing_ones)
	{
		flush_n_bits += num_trailing_ones;
		code = next_bits>>(32-flush_n_bits);

		int *tmp_lev = levarr + num_coeff - num_trailing_ones;
		for (k = 0; k < num_trailing_ones; k++)
		{
			if ((code>>k)&1)
				tmp_lev[0] = -1;
			else
				tmp_lev[0] = 1;
			tmp_lev++;
		}
	}
	FLUSH_BITS(flush_n_bits)	

	num_levels = num_coeff - num_trailing_ones;
	if(num_levels>0)
	{
		unsigned short const* tab = tab3;
		int			  ones_less_than_3;
		unsigned int		  offset_base;
		int			  width, with_is_0;

		//Changed from tab = tab3 - 1;
		tab --;
		ones_less_than_3 = num_trailing_ones<3;
		offset_base      = 2*ones_less_than_3 + 2;
		width			 = num_coeff>10&&ones_less_than_3;
		with_is_0		 = 1^(width);

		for (k = num_levels - 1 ; k >= 0; k--)
		{
			int offset;
			int bits;

			PEEK_BITS(32, next_bits)				
			int num_leading_zeros = CNTLZ(next_bits); //ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c
			
			if(num_leading_zeros < 15 )
			{
				next_bits <<= 1;
				next_bits <<= num_leading_zeros;

				flush_n_bits = (num_leading_zeros + 1 + width);
				bits		 = next_bits>>(32-width);

#ifndef _WIN32_WCE
				if((32-width)>=32) 
					bits = 0;
#endif

				offset=offset_base+ (num_leading_zeros<<width);
				if(width==0)
				{
					bits     = num_leading_zeros;
					offset = offset_base;
					if(num_leading_zeros==14)
					{
						bits		  = next_bits>>28;
						offset		  = offset_base + 14;
						flush_n_bits += 4;
					}
				}
			}
			else
			{
				bits	   = ( next_bits>>4 )&0xFFF;
				offset =  offset_base+( 15<<(width+with_is_0) );
				flush_n_bits = 28;
			}
			
			int mag = (bits+offset)>>1;					
			int sig = -( (int)(bits&0x01) );				
			level   = (mag^sig)-sig;	

			FLUSH_BITS(flush_n_bits);

			width += with_is_0;
			width += mag > tab[width];

			offset_base	= 2;
			with_is_0	= false;		
			levarr[k]	= level;
		}
	}

	if (num_coeff < 4)
	{
		// decode total run
		Rec1   const*	rec		=&(rec_ary1_cd[num_coeff-1]);
		unsigned int force1bit_num_idx  = rec->force1bit_num_idx;
		unsigned int			num_idx = force1bit_num_idx&0xFF;	
		unsigned char const* tab_ary = rec->offsets;

		unsigned int next_bits;
		PEEK_BITS(32, next_bits)
		next_bits		|= force1bit_num_idx;
		unsigned int tab_idx	 = CNTLZ(next_bits); //ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c
		
		next_bits <<= 1;
		next_bits <<= tab_idx;

		Element1 const*	tab = tab_ary[tab_idx]+tab1;
		unsigned int			idx = next_bits>>num_idx;
#ifndef _WIN32_WCE
		if(num_idx>=32) 
			idx = 0;
#endif
		Element1 e	 = tab[idx];
		unsigned int	 f	 = UPK1_1(e);
		totzeros	 = UPK1_2(e);

		FLUSH_BITS(f);
	}
	else
		totzeros = 0;

	// decode run before each coefficient
	zerosleft = totzeros;
	i = num_coeff-1;
	if (zerosleft > 0 && i > 0)
	{
		do 
		{
			unsigned int numZerosInThisRun = 0;
			if( totzeros > 0 )
			{
				unsigned int pinnedNumZerosLeftToDistribute = pin_to_7[totzeros-1];

				Rec2 const*				 rec	= &rec_ary2[pinnedNumZerosLeftToDistribute-1];
				unsigned char const* tables	= rec->offsets;
				unsigned int		force1bit_num_idx	= rec->force1bit_num_idx;
				unsigned int				  num_idx	= force1bit_num_idx&0xFF;

				unsigned int next_bits;
				PEEK_BITS(32, next_bits )
				next_bits |= force1bit_num_idx;
				
				unsigned int tab_idx = CNTLZ(next_bits); //ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c
				next_bits <<= 1;
				next_bits <<= tab_idx;

				Element2 const* tab = tables[tab_idx]+tab2;
				unsigned int idx = next_bits>>num_idx;
#ifndef _WIN32_WCE
				if(num_idx>=32) 
					idx = 0;
#endif
				Element2 numZerosInThisRun_Record = tab[idx];
				unsigned int flush_n_bits	  = UPK2_1(numZerosInThisRun_Record);
				numZerosInThisRun	  = UPK2_2(numZerosInThisRun_Record);
				FLUSH_BITS(flush_n_bits);
				totzeros -= numZerosInThisRun;
			}

			runarr[i] = numZerosInThisRun;
			zerosleft -= runarr[i];
			i --;
		} while (zerosleft != 0 && i != 0);
	}

	runarr[i] = zerosleft;

	// The last step : SCAN order
	// Ipp16s **ppDstCoeffs
	// const Ipp32s *pScanMatrix
	//if(uMaxNumCoeff == 16)		// Luma
		coeffNumber = -1;
	//else
	//	coeffNumber = 0;		// Chroma AC part, We do not use [0] element WWD 

	for(k=0; k< num_coeff; k++)
	{

		coeffNumber += runarr[k] +1;
		(*ppDstCoeffs)[coeffNumber] = levarr[k];
	}

	// According to Function spec in Ippiman.pdf
	(*ppDstCoeffs) += 4;		 

bail:
	FINISH_BITSTREAM_LOCAL

	Profile_End(ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_profile);

	return err;
}


IppStatus __STDCALL 
ippiDecodeCAVLCCoeffs_H264_1u16s_c_only
(
	Ipp32u **ppBitStream,
	Ipp32s *pBitOffset,
	Ipp16s *pNumCoeff,
	Ipp16s **ppDstCoeffs,
	Ipp32u uVLCSelect,
	Ipp16s uMaxNumCoeff,
	const Ipp32s **ppTblCoeffToken_whocares,
	const Ipp32s **ppTblTotalZeros_whocares,
	const Ipp32s **ppTblRunBefore_whocares,
	const Ipp32s *pScanMatrix 
)
{
	Profile_Start(ippiDecodeCAVLCCoeffs_H264_1u16s_c_profile);

	BITSTREAM_LOCAL
	START_BITSTREAM_LOCAL

	IppStatus	err = ippStsNoErr;
	int		i,	k;
	int		num_trailing_ones;
	int		numones, totzeros, level;
	int		zerosleft;
	int		code;
	int		retval;
	__ALIGN8(int, tmp0, 32);
	int		*levarr = &tmp0[0];
	int		*runarr = &tmp0[16];
	int		coeffNumber;
	int		num_coeff = 0;
	int		max_coeff_num = uMaxNumCoeff;
	int		numcoeff_vlc = vlc_select_tab[uVLCSelect]; 	// numcoeff_vlc is the index of Table used to code coeff_token
														// numcoeff_vlc==3 means (8<=nC) which uses 6bit FLC
	unsigned int next_bits;
	unsigned int num_levels;
	int			 flush_n_bits;
	INITIAL_PEEK_BITS(32, next_bits)

	if( uVLCSelect > 11 )
		uVLCSelect = 11;

	numcoeff_vlc = vlc_select_tab[uVLCSelect]; 	// numcoeff_vlc is the index of Table used to code coeff_token

	if (numcoeff_vlc == 3)
	{	// read 6 bit FLC
		code = next_bits>>26;			
		num_trailing_ones = code & 3;	
		num_coeff = (code >> 2);

		if (!num_coeff && num_trailing_ones == 3)
			num_trailing_ones = 0;// #c = 0, #t1 = 3 =>  #c = 0
		else
			num_coeff++;

		retval = 0;
		
		flush_n_bits = 6;
	}
	else
	{
		unsigned int	next_bits_1		  = next_bits|(1<<15);
		unsigned int	num_leading_zeros = CLZ_c(next_bits_1); //ippiDecodeCAVLCCoeffs_H264_1u16s_c_only

		next_bits_1  <<= 1;
		next_bits_1  <<= num_leading_zeros;

		Rec0	const*  rec	= rect0_ary + rect0_ary_offset[numcoeff_vlc];
		unsigned int		num_idx	= rec[num_leading_zeros].num_idx;
		unsigned short	const*  tab	= rec[num_leading_zeros].offset + tab0;
		unsigned int			idx	= (next_bits_1>>num_idx);
#ifndef _WIN32_WCE
		if(num_idx>=32) 
			idx = 0;
#endif
		unsigned short r	= tab[idx];
		flush_n_bits		= UPK0_1( r );
		num_coeff			= UPK0_2( r );
		num_trailing_ones	= UPK0_3( r );
	}

	numones		= num_trailing_ones;
	*pNumCoeff	= num_coeff;
	if (!num_coeff)
	{
		FLUSH_BITS(flush_n_bits)	
		goto bail;
	}

	ippsZero_32s_x( tmp0, 32 );	//clear levarr, runarr
	ippsZero_16s_x( (*ppDstCoeffs), 16 );

	// Get the sign of trailing ones
	if (num_trailing_ones)
	{
		flush_n_bits += num_trailing_ones;
		code = next_bits>>(32-flush_n_bits);

		int *tmp_lev = levarr + num_coeff - num_trailing_ones;
		for (k = 0; k < num_trailing_ones; k++)
		{
			if ((code>>k)&1)
				tmp_lev[0] = -1;
			else
				tmp_lev[0] = 1;
			tmp_lev++;
		}
	}
	FLUSH_BITS(flush_n_bits)	

	num_levels = num_coeff - num_trailing_ones;
	if(num_levels>0)
	{
		unsigned short const* tab = tab3;
		int			  ones_less_than_3;
		unsigned int		  offset_base;
		int			  width, with_is_0;

		//Changed from tab = tab3 - 1;
		tab --;
		ones_less_than_3 = num_trailing_ones<3;
		offset_base      = 2*ones_less_than_3 + 2;
		width			 = num_coeff>10&&ones_less_than_3;
		with_is_0		 = 1^(width);

		for (k = num_levels - 1 ; k >= 0; k--)
		{
			int offset;
			int bits;

			PEEK_BITS(32, next_bits)				
			int num_leading_zeros = CLZ_c(next_bits); //ippiDecodeCAVLCCoeffs_H264_1u16s_c_only
			
			if(num_leading_zeros < 15 )
			{
				next_bits <<= 1;
				next_bits <<= num_leading_zeros;

				flush_n_bits = (num_leading_zeros + 1 + width);
				bits		 = next_bits>>(32-width);

#ifndef _WIN32_WCE
				if((32-width)>=32) 
					bits = 0;
#endif

				offset=offset_base+ (num_leading_zeros<<width);
				if(width==0)
				{
					bits     = num_leading_zeros;
					offset = offset_base;
					if(num_leading_zeros==14)
					{
						bits		  = next_bits>>28;
						offset		  = offset_base + 14;
						flush_n_bits += 4;
					}
				}
			}
			else
			{
				bits	   = ( next_bits>>4 )&0xFFF;
				offset =  offset_base+( 15<<(width+with_is_0) );
				flush_n_bits = 28;
			}
			
			int mag = (bits+offset)>>1;					
			int sig = -( (int)(bits&0x01) );				
			level   = (mag^sig)-sig;	

			FLUSH_BITS(flush_n_bits);

			width += with_is_0;
			width += mag > tab[width];

			offset_base	= 2;
			with_is_0	= false;		
			levarr[k]	= level;
		}
	}

	if (num_coeff < max_coeff_num)
	{	// decode total run
		Rec1   const*	rec		=&(rec_ary1[num_coeff-1]);
		unsigned int force1bit_num_idx  = rec->force1bit_num_idx;
		unsigned int			num_idx = force1bit_num_idx&0xFF;	
		unsigned char const* tab_ary = rec->offsets;

		unsigned int next_bits;
		PEEK_BITS(32, next_bits)
		next_bits		|= force1bit_num_idx;
		unsigned int tab_idx	 = CLZ_c(next_bits); //ippiDecodeCAVLCCoeffs_H264_1u16s_c_only
		
		next_bits <<= 1;
		next_bits <<= tab_idx;

		Element1 const*	tab = tab_ary[tab_idx]+tab1;
		unsigned int			idx = next_bits>>num_idx;
#ifndef _WIN32_WCE
		if(num_idx>=32) 
			idx = 0;
#endif
		Element1 e	 = tab[idx];
		unsigned int	 f	 = UPK1_1(e);
		totzeros	 = UPK1_2(e);

		FLUSH_BITS(f);
	}
	else
		totzeros = 0;

	// decode run before each coefficient
	zerosleft = totzeros;
	i = num_coeff-1;
	if (zerosleft > 0 && i > 0)
		do 
		{
			unsigned int numZerosInThisRun = 0;
			if( totzeros > 0 )
			{
				unsigned int pinnedNumZerosLeftToDistribute = pin_to_7[totzeros-1];

				Rec2 const*			 rec	= &rec_ary2[pinnedNumZerosLeftToDistribute-1];
				unsigned char const* tables	= rec->offsets;
				unsigned int		 force1bit_num_idx	= rec->force1bit_num_idx;
				unsigned int		 num_idx	= force1bit_num_idx&0xFF;

				unsigned int next_bits;
				PEEK_BITS(32, next_bits )
				next_bits |= force1bit_num_idx;
				
				unsigned int tab_idx = CLZ_c(next_bits); //ippiDecodeCAVLCCoeffs_H264_1u16s_c_only
				next_bits <<= 1;
				next_bits <<= tab_idx;

				Element2 const* tab = tables[tab_idx]+tab2;
				unsigned int idx = next_bits>>num_idx;
#ifndef _WIN32_WCE
				if(num_idx>=32) 
					idx = 0;
#endif
				int numZerosInThisRun_Record = (int)tab[idx];
				unsigned int flush_n_bits	 = UPK2_1(numZerosInThisRun_Record);
				numZerosInThisRun			 = UPK2_2(numZerosInThisRun_Record);
				FLUSH_BITS(flush_n_bits);
				totzeros -= numZerosInThisRun;
			}

			runarr[i] = numZerosInThisRun;
			zerosleft -= runarr[i];
			i --;

		}while (zerosleft > 0 && i != 0);

	// !!!FIXME: This fix just make it same semantics as before but it should not corerct. I will check later!!! 
	//if( zerosleft < 0 || (int)levarr == 0xffffffff )
	if( zerosleft < 0 || (long)levarr == -1 )

		goto bail;

	runarr[i] = zerosleft;

	// The last step : SCAN order
	if(uMaxNumCoeff == 16)		// Luma
		coeffNumber = -1;
	else
		coeffNumber = 0;		// Chroma AC part, We do not use [0] element WWD 

	for(k=0; k< num_coeff; k++)
	{
		coeffNumber += runarr[k] +1;
		(*ppDstCoeffs)[pScanMatrix[coeffNumber]] = levarr[k];
	}

	// According to Function spec in Ippiman.pdf
	(*ppDstCoeffs) += 16;		 

bail:
	FINISH_BITSTREAM_LOCAL

	Profile_End(ippiDecodeCAVLCCoeffs_H264_1u16s_c_profile);

	return err;
}


IppStatus __STDCALL ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_only
(
	Ipp32u **ppBitStream,
	Ipp32s *pBitOffset,
	Ipp16s *pNumCoeff,
	Ipp16s **ppDstCoeffs,
	const Ipp32s *pTblCoeffToken_whocars,
	const Ipp32s **ppTblTotalZerosCR_whocars,
	const Ipp32s **ppTblRunBeforeB_whocars 
)
{
	Profile_Start(ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_profile);

	BITSTREAM_LOCAL
	START_BITSTREAM_LOCAL

	int		i,	k;
	int		num_coeff, num_trailing_ones;
	int		numones, totzeros, level;
	int		zerosleft;
	
	int		code;
	__ALIGN8(int, tmp0, 8);
	int		*levarr = &tmp0[0];
	int		*runarr = &tmp0[4];

	int		coeffNumber;
	IppStatus err =	ippStsNoErr;
	unsigned int  num_levels;

	num_coeff = 0;
	unsigned int next_bits;
	int  flush_n_bits;
	{
		INITIAL_PEEK_BITS(32, next_bits)				
		unsigned int	next_bits_1		  = next_bits|(1<<23);
		unsigned int	num_leading_zeros = CLZ_c(next_bits_1); //ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_only

		next_bits_1  <<= 1;
		next_bits_1  <<= num_leading_zeros;

		unsigned int		num_idx	= rec0_cd[num_leading_zeros].num_idx;
		unsigned short	const*  tab	= rec0_cd[num_leading_zeros].offset + tab0;
		unsigned int			idx	= (next_bits_1>>num_idx);

#ifndef _WIN32_WCE
		if(num_idx>=32) 
			idx = 0;
#endif
		int	r				= (int)tab[idx];
		flush_n_bits		= UPK0_1( r );
		num_coeff			= UPK0_2( r );
		num_trailing_ones	= UPK0_3( r );	
	}
	
	numones = num_trailing_ones;
	*pNumCoeff = num_coeff;

	if (!num_coeff)
	{
		FLUSH_BITS(flush_n_bits)		
		goto bail;
	}

	ippsZero_32s_x( tmp0, 8 );	//clear levarr, runarr
	ippsZero_16s_x( (*ppDstCoeffs), 4 );

	//for (k = 0; k < 4; k++)
	//{
	//	levarr[k] = 0;
	//	runarr[k] = 0;
//
	//	(*ppDstCoeffs)[k] = 0;
	//}

	// Get the sign of trailing ones
	if (num_trailing_ones)
	{
		flush_n_bits += num_trailing_ones;
		code = next_bits>>(32-flush_n_bits);

		int *tmp_lev = levarr + num_coeff - num_trailing_ones;
		for (k = 0; k < num_trailing_ones; k++)
		{
			if ((code>>k)&1)
				tmp_lev[0] = -1;
			else
				tmp_lev[0] = 1;
			tmp_lev++;
		}
	}
	FLUSH_BITS(flush_n_bits)	

	num_levels = num_coeff - num_trailing_ones;
	if(num_levels>0)
	{
		unsigned short const* tab = tab3;
		int			  ones_less_than_3;
		unsigned int		  offset_base;
		int			  width, with_is_0;

		//Changed from tab = tab3 - 1;
		tab--;
		ones_less_than_3 = num_trailing_ones<3;
		offset_base      = 2*ones_less_than_3 + 2;
		width			 = num_coeff>10&&ones_less_than_3;
		with_is_0		 = 1^(width);

		for (k = num_levels - 1 ; k >= 0; k--)
		{
			int offset;
			int bits;

			PEEK_BITS(32, next_bits)				
			int num_leading_zeros = CLZ_c(next_bits); //ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_only
			
			if(num_leading_zeros < 15 )
			{
				next_bits <<= 1;
				next_bits <<= num_leading_zeros;

				flush_n_bits = (num_leading_zeros + 1 + width);
				bits		 = next_bits>>(32-width);

#ifndef _WIN32_WCE
				if((32-width)>=32) 
					bits = 0;
#endif

				offset=offset_base+ (num_leading_zeros<<width);
				if(width==0)
				{
					bits     = num_leading_zeros;
					offset = offset_base;
					if(num_leading_zeros==14)
					{
						bits		  = next_bits>>28;
						offset		  = offset_base + 14;
						flush_n_bits += 4;
					}
				}
			}
			else
			{
				bits	   = ( next_bits>>4 )&0xFFF;
				offset =  offset_base+( 15<<(width+with_is_0) );
				flush_n_bits = 28;
			}
			
			int mag = (bits+offset)>>1;					
			int sig = -( (int)(bits&0x01) );				
			level   = (mag^sig)-sig;	

			FLUSH_BITS(flush_n_bits);

			width += with_is_0;
			width += mag > tab[width];

			offset_base	= 2;
			with_is_0	= false;		
			levarr[k]	= level;
		}
	}

	if (num_coeff < 4)
	{
		// decode total run
		Rec1   const*	rec		=&(rec_ary1_cd[num_coeff-1]);
		unsigned int force1bit_num_idx  = rec->force1bit_num_idx;
		unsigned int			num_idx = force1bit_num_idx&0xFF;	
		unsigned char const* tab_ary = rec->offsets;

		unsigned int next_bits;
		PEEK_BITS(32, next_bits)
		next_bits		|= force1bit_num_idx;
		unsigned int tab_idx	 = CLZ_c(next_bits); //ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_only
		
		next_bits <<= 1;
		next_bits <<= tab_idx;

		Element1 const*	tab = tab_ary[tab_idx]+tab1;
		unsigned int			idx = next_bits>>num_idx;
#ifndef _WIN32_WCE
		if(num_idx>=32) 
			idx = 0;
#endif
		Element1 e	 = tab[idx];
		unsigned int	 f	 = UPK1_1(e);
		totzeros	 = UPK1_2(e);

		FLUSH_BITS(f);
	}
	else
		totzeros = 0;

	// decode run before each coefficient
	zerosleft = totzeros;
	i = num_coeff-1;
	if (zerosleft > 0 && i > 0)
	{
		do 
		{
			unsigned int numZerosInThisRun = 0;
			if( totzeros > 0 )
			{
				unsigned int pinnedNumZerosLeftToDistribute = pin_to_7[totzeros-1];

				Rec2 const*				 rec	= &rec_ary2[pinnedNumZerosLeftToDistribute-1];
				unsigned char const* tables	= rec->offsets;
				unsigned int		force1bit_num_idx	= rec->force1bit_num_idx;
				unsigned int				  num_idx	= force1bit_num_idx&0xFF;

				unsigned int next_bits;
				PEEK_BITS(32, next_bits )
				next_bits |= force1bit_num_idx;
				
				unsigned int tab_idx = CLZ_c(next_bits); //ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_only
				next_bits <<= 1;
				next_bits <<= tab_idx;

				Element2 const* tab = tables[tab_idx]+tab2;
				unsigned int idx = next_bits>>num_idx;
#ifndef _WIN32_WCE
				if(num_idx>=32) 
					idx = 0;
#endif
				Element2 numZerosInThisRun_Record = tab[idx];
				unsigned int flush_n_bits	  = UPK2_1(numZerosInThisRun_Record);
				numZerosInThisRun	  = UPK2_2(numZerosInThisRun_Record);
				FLUSH_BITS(flush_n_bits);
				totzeros -= numZerosInThisRun;
			}

			runarr[i] = numZerosInThisRun;
			zerosleft -= runarr[i];
			i --;
		} while (zerosleft != 0 && i != 0);
	}

	runarr[i] = zerosleft;

	// The last step : SCAN order
	// Ipp16s **ppDstCoeffs
	// const Ipp32s *pScanMatrix
	//if(uMaxNumCoeff == 16)		// Luma
		coeffNumber = -1;
	//else
	//	coeffNumber = 0;		// Chroma AC part, We do not use [0] element WWD 

	for(k=0; k< num_coeff; k++)
	{

		coeffNumber += runarr[k] +1;
		(*ppDstCoeffs)[coeffNumber] = levarr[k];
	}

	// According to Function spec in Ippiman.pdf
	(*ppDstCoeffs) += 4;		 

bail:
	FINISH_BITSTREAM_LOCAL

	Profile_End(ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_profile);

	return err;
}




#endif


// We use right align mode for this function and
//   data have been flipped already if neccesary
 /*
 **************************************************************************
 * Function:    ippiDecodeExpGolombOne_H264_1u16s_x
 * Description: Do ExpGolomb decoding
 *
 * Note		  :  The maximum bits is 28 bits for level according to SPEC
 *
 * 
 **************************************************************************
 */
// Further optimized in 20070811
#define MAX_LEVEL_LEN	25 // Level need 28 bits, but other ue/se symbols do not need so much (I have confirmed that 16bits are not enough)

#if 1

#ifndef DROP_C_NO

int CLZ_c(int value);
static inline int ippiDecodeExpGolombOne_H264_1u16s_0_c(Ipp32u **ppBitStream, Ipp32s *pBitOffset/*,	Ipp16s *pDst,	Ipp8u isSigned */)//***
{
	unsigned int	currDBuff, nextDBuff, processDBuff;
	unsigned int	info, mask;
	int				nCode, bitsLeft, nZeros, bitsLeftinCurentDBuff;

	// Assert we have data to do ExpGolomb
	assert((*ppBitStream != NULL) && pBitOffset !=NULL);

	currDBuff = **ppBitStream; 	
	bitsLeft = *pBitOffset + 1;

	// First, combine two DBuff if necessary: Here because Intel's source codes process 4bytes as one element, need change later
	processDBuff = (currDBuff << (32 - bitsLeft));
#if !defined (WINCE)  // shift number must less than 31, otherwise, the result is not predict
	if(bitsLeft < MAX_LEVEL_LEN)
#endif
	{
		nextDBuff = *(*ppBitStream + 1);
		processDBuff = (processDBuff | (nextDBuff >> bitsLeft));
	}

	nZeros = CLZ_c(processDBuff);

	// Read info for current symbol: special, just read from current processDBuff, it is enough!!
	// Just extract bits from processDBuff
	mask = (1 << nZeros) -1;
	nZeros = (nZeros << 1) + 1;
	info = (processDBuff >> (32 - nZeros)) & mask;
	nCode = mask + info;

	// Update bit stream
	bitsLeftinCurentDBuff = *pBitOffset - nZeros;
	if(bitsLeftinCurentDBuff>=0)
	{
		// Update offset
		*pBitOffset = bitsLeftinCurentDBuff;
	}
	else
	{
		(*ppBitStream)++;
		*pBitOffset = bitsLeftinCurentDBuff + 32;
	}
	
	return nCode;
}

Ipp32s __STDCALL ippiDecodeExpGolombOne_H264_1u16s_unsigned_c(Ipp32u **ppBitStream, Ipp32s *pBitOffset )//***
{
	Profile_Start(ippiDecodeExpGolombOne_H264_1u16s_c_profile);

	Ipp32s	value = 0;
#if 0
	value = ippiDecodeExpGolombOne_H264_1u16s_unsigned_arm(ppBitStream, pBitOffset );
#else
	value = ippiDecodeExpGolombOne_H264_1u16s_0_c( ppBitStream, pBitOffset);
#endif

	Profile_End(ippiDecodeExpGolombOne_H264_1u16s_c_profile);

	return value;
}

Ipp32s __STDCALL ippiDecodeExpGolombOne_H264_1u16s_signed_c(Ipp32u **ppBitStream, Ipp32s *pBitOffset )//***
{
	Profile_Start(ippiDecodeExpGolombOne_H264_1u16s_c_profile);

	Ipp32s	value = 0;
#if 0
	value = ippiDecodeExpGolombOne_H264_1u16s_signed_arm(ppBitStream, pBitOffset );
#else
	value = ippiDecodeExpGolombOne_H264_1u16s_0_c( ppBitStream, pBitOffset);
	{
		// here is one condition move, do not need if statements. Check compiler result in ARM
		if((value & 0x01)==0)      // lsb is signed bit
		{
			value = -((value+1) >>1);
		}
		else
			value = (value+1) >>1;
	}
#endif

	Profile_End(ippiDecodeExpGolombOne_H264_1u16s_c_profile);

	return value;
}
#endif

#else

IppStatus __STDCALL ippiDecodeExpGolombOne_H264_1u16s_c(Ipp32u **ppBitStream, Ipp32s *pBitOffset,
												Ipp16s *pDst, Ipp8u isSigned )//***
{
	BITSTREAM_LOCAL
	START_BITSTREAM_LOCAL

	int		num_zeros = 0;
	Ipp32u	value;
	

	four_bytes = *p_stream;	
	four_bytes <<= bit_offset;

	if( bit_offset > 15 )//***8 is safer
	{
		Ipp32u next_four = *(p_stream + 1);
		four_bytes |= next_four>>(32 - bit_offset);
	}

	num_zeros = CNTLZ(four_bytes);

	{														
		int word_offset;								
		int tmp;									

		four_bytes <<= ( 1 + num_zeros);	
		four_bytes >>= (32 - num_zeros);					
		if(num_zeros==0) // on win32, >> 32 does nothing
			four_bytes = 0;								
		
		value		= ( 1<<num_zeros ) - 1 + four_bytes;																\
		tmp			= bit_offset + num_zeros+ 1 + num_zeros;	
		word_offset = (tmp>>5); 
		bit_offset	= (tmp&0x1f);
		p_stream    += word_offset;									
	}

	// Process signed value
	if(isSigned)
	{
		int tmp;													

		value++;
		tmp = value;
		value   = ( value>>1);												
		if(tmp&0x01)                           // lsb is signed bit
			value = -value;
	}

	*pDst = value;

	FINISH_BITSTREAM_LOCAL

	return ippStsNoErr;
}

#endif

#ifdef __INTEL_IPP__
Ipp32s __STDCALL ippiDecodeExpGolombOne_H264_1u16s_signed_ipp(Ipp32u **ppBitStream,	Ipp32s *pBitOffset )
{
	Ipp16s	value = 0;

	ippiDecodeExpGolombOne_H264_1u16s(   ppBitStream,  pBitOffset, &value,	true );
	
	return value;
}

Ipp32s __STDCALL ippiDecodeExpGolombOne_H264_1u16s_unsigned_ipp(Ipp32u **ppBitStream, Ipp32s *pBitOffset )											
{
	Ipp16s	value = 0;

	ippiDecodeExpGolombOne_H264_1u16s( ppBitStream, pBitOffset, &value, false );

	return value;
}
#endif

#endif
