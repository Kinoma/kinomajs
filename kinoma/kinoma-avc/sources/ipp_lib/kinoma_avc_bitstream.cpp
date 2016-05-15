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
#include <assert.h>


#ifdef __KINOMA_IPP__

#include "kinoma_avc_defines.h"
#include "ippvc.h"



// These function need further optimization for it suppport YUV422 and YUV444 current.

  int Luma_lentab_Token[3][4][17] = 
  {
    {   // 0702
      { 1, 6, 8, 9,10,11,13,13,13,14,14,15,15,16,16,16,16},
      { 0, 2, 6, 8, 9,10,11,13,13,14,14,15,15,15,16,16,16},
      { 0, 0, 3, 7, 8, 9,10,11,13,13,14,14,15,15,16,16,16},
      { 0, 0, 0, 5, 6, 7, 8, 9,10,11,13,14,14,15,15,16,16},
    },                                                 
    {                                                  
      { 2, 6, 6, 7, 8, 8, 9,11,11,12,12,12,13,13,13,14,14},
      { 0, 2, 5, 6, 6, 7, 8, 9,11,11,12,12,13,13,14,14,14},
      { 0, 0, 3, 6, 6, 7, 8, 9,11,11,12,12,13,13,13,14,14},
      { 0, 0, 0, 4, 4, 5, 6, 6, 7, 9,11,11,12,13,13,13,14},
    },                                                 
    {                                                  
      { 4, 6, 6, 6, 7, 7, 7, 7, 8, 8, 9, 9, 9,10,10,10,10},
      { 0, 4, 5, 5, 5, 5, 6, 6, 7, 8, 8, 9, 9, 9,10,10,10},
      { 0, 0, 4, 5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9,10,10,10},
      { 0, 0, 0, 4, 4, 4, 4, 4, 5, 6, 7, 8, 8, 9,10,10,10},
    },

  };

  
  int Chroma_lentab_Token[4][17] = 
    {{ 2, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 1, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    { 0, 0, 3, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    { 0, 0, 0, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

  int Luma_codtab_Token[3][4][17] = 
  {
    {
      { 1, 5, 7, 7, 7, 7,15,11, 8,15,11,15,11,15,11, 7,4}, 
      { 0, 1, 4, 6, 6, 6, 6,14,10,14,10,14,10, 1,14,10,6}, 
      { 0, 0, 1, 5, 5, 5, 5, 5,13, 9,13, 9,13, 9,13, 9,5}, 
      { 0, 0, 0, 3, 3, 4, 4, 4, 4, 4,12,12, 8,12, 8,12,8},
    },
    {
      { 3,11, 7, 7, 7, 4, 7,15,11,15,11, 8,15,11, 7, 9,7}, 
      { 0, 2, 7,10, 6, 6, 6, 6,14,10,14,10,14,10,11, 8,6}, 
      { 0, 0, 3, 9, 5, 5, 5, 5,13, 9,13, 9,13, 9, 6,10,5}, 
      { 0, 0, 0, 5, 4, 6, 8, 4, 4, 4,12, 8,12,12, 8, 1,4},
    },
    {
      {15,15,11, 8,15,11, 9, 8,15,11,15,11, 8,13, 9, 5,1}, 
      { 0,14,15,12,10, 8,14,10,14,14,10,14,10, 7,12, 8,4},
      { 0, 0,13,14,11, 9,13, 9,13,10,13, 9,13, 9,11, 7,3},
      { 0, 0, 0,12,11,10, 9, 8,13,12,12,12, 8,12,10, 6,2},
    },
  };


  int Chroma_codtab_Token[4][17] = 
   {{ 1, 7, 4, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 1, 6, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

  int Luma_lentab_TotalZeros[TOTRUN_NUM][16] = 
  {
    
    { 1,3,3,4,4,5,5,6,6,7,7,8,8,9,9,9},  
    { 3,3,3,3,3,4,4,4,4,5,5,6,6,6,6},  
    { 4,3,3,3,4,4,3,3,4,5,5,6,5,6},  
    { 5,3,4,4,3,3,3,4,3,4,5,5,5},  
    { 4,4,4,3,3,3,3,3,4,5,4,5},  
    { 6,5,3,3,3,3,3,3,4,3,6},  
    { 6,5,3,3,3,2,3,4,3,6},  
    { 6,4,5,3,2,2,3,3,6},  
    { 6,6,4,2,2,3,2,5},  
    { 5,5,3,2,2,2,4},  
    { 4,4,3,3,1,3},  
    { 4,4,2,1,3},  
    { 3,3,1,2},  
    { 2,2,1},  
    { 1,1},  
  };


  int Chroma_lentab_TotalZeros[3][4] = 
   {
	{ 1,2,3,3},
    { 1,2,2},
    { 1,1}
  };

  int Luma_codtab_TotalZeros[TOTRUN_NUM][16] = 
  {
    {1,3,2,3,2,3,2,3,2,3,2,3,2,3,2,1},
    {7,6,5,4,3,5,4,3,2,3,2,3,2,1,0},
    {5,7,6,5,4,3,4,3,2,3,2,1,1,0},
    {3,7,5,4,6,5,4,3,3,2,2,1,0},
    {5,4,3,7,6,5,4,3,2,1,1,0},
    {1,1,7,6,5,4,3,2,1,1,0},
    {1,1,5,4,3,3,2,1,1,0},
    {1,1,1,3,3,2,2,1,0},
    {1,0,1,3,2,1,1,1,},
    {1,0,1,3,2,1,1,},
    {0,1,1,2,1,3},
    {0,1,1,1,1},
    {0,1,1,1},
    {0,1,1},
    {0,1},  
  };

  int Chroma_codtab_TotalZeros[3][4] = 
  {
    { 1,1,1,0},
    { 1,1,0},
    { 1,0}
   };

  // 以下的变量对于luma以及Chroma是相同的
  int lentab_Run[7][16] = 
  {
    {1,1},
    {1,2,2},
    {2,2,2,2},
    {2,2,2,3,3},
    {2,2,3,3,3,3},
    {2,3,3,3,3,3,3},
    {3,3,3,3,3,3,3,4,5,6,7,8,9,10,11},
  };
 
  int codtab_Run[7][16] = 
  {
    {1,0},
    {1,1,0},
    {3,2,1,0},
    {3,2,1,1,0},
    {3,2,3,2,1,0},
    {3,0,1,3,2,5,4},
    {7,6,5,4,3,2,1,1,1,1,1,1,1,1,1},
  };

// We use right align mode for this function and
//   data have been flipped already if neccesary
 /*
 **************************************************************************
 * Function:    ippiDecodeExpGolombOne_H264_1u16s
 * Description: Do ExpGolomb coding
 *
 * Problems   : 
 *
 * 
 **************************************************************************
 */
IppStatus __STDCALL ippiDecodeExpGolombOne_H264_1u16s(Ipp32u **ppBitStream,
												Ipp32s *pBitOffset,
												Ipp16s *pDst,
												Ipp8u isSigned)
{
	int		number_zeros = 0;
	Ipp32u	content;

	unsigned char * pStream;

	int		info, info_bit;

	int		ctr_bit = 0;
	int		BitOffset = (*pBitOffset);

	int		bitcounter = 1;
	// Assert we have data to do ExpGolomb
	if((*ppBitStream == NULL) || pBitOffset==NULL || pDst==NULL)
		return ippStsNullPtrErr;

	pStream = (unsigned char*) *ppBitStream;

	content = **ppBitStream;
	// Shift bit to right
	content <<= (31 - BitOffset);

	ctr_bit = (content & (0x01<<31));

	// WE SHOULD note MSB/LSB here
	// Start 
	// According to Spec 9.1
	number_zeros = 1;

	while(ctr_bit == 0)
	{
#ifdef __WWD_BRANCH_TEST__
			test_branch[114] = 1;
#endif
		number_zeros += 1;
		BitOffset -= 1;
		content <<= 1;

		bitcounter+= 1;

		if(BitOffset < 0)
		{
			(*ppBitStream)++;

			content = **ppBitStream;

			BitOffset += 32;

#ifdef __WWD_BRANCH_TEST__
			test_branch[115] = 1;
#endif
		}

		ctr_bit = (content & (0x01<<31));

	}



	info = 0;
	for(info_bit=0; info_bit< (number_zeros-1); info_bit++)
	{
		BitOffset -= 1;
		content <<= 1;

		if(BitOffset < 0)
		{
			(*ppBitStream)++;
			content = **ppBitStream;

			BitOffset += 32;

#ifdef __WWD_BRANCH_TEST__
			test_branch[116] = 1;
#endif
		}

		info = info << 1;
		//if(content &  (unsigned int)(0x01<<position[bitOffset]))
		if(content & 0x80000000)
		{
			info |=1;

#ifdef __WWD_BRANCH_TEST__
			test_branch[117] = 1;
#endif
		}


		bitcounter+= 1;
	}

	//*pDst = (int)pow(2.0,(int)(bitcounter>>1)) + info - 1;
	*pDst = (1 << (bitcounter>>1)) + info - 1;

	// Reduce 0000*1?????
	BitOffset -= 1; 
	if(BitOffset < 0)
	{
		(*ppBitStream)++;

		//content = **ppBitStream;

		BitOffset += 32;

#ifdef __WWD_BRANCH_TEST__
			test_branch[118] = 1;
#endif
	}

	*pBitOffset = BitOffset ;

	// Process signed value
	if(isSigned)
	{
		content = *pDst ;
		*pDst = (content + 1)>>1;
		if((content & 0x01)==0)                           // lsb is signed bit
		{
			*pDst = -*pDst;
#ifdef __WWD_BRANCH_TEST__
			test_branch[119] = 1;
#endif
		}

#ifdef __WWD_BRANCH_TEST__
			test_branch[120] = 1;
#endif
	}
	
	
	return ippStsNoErr;
}

// This function is used for K_ippiDecodeCAVLCCoeffs_H264 and K_ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s ONLY
//  We do not check if the bitsNumber is exceed current buffer, So, please ensure this in caller
 /*
 **************************************************************************
 * Function:    readBits
 * Description: Read bits from stream. 
 *				pOffset		:is offset of stream DWORD [0, 31]
 *				bitsNumber  :is bits required from its caller 
 * Problems   : 
 *
 * 
 **************************************************************************
 */
static Ipp32u readBits(Ipp32u **ppBitStream,
                           Ipp32s *pOffset,
						   const  int	bitsNumber
						   )
{
	Ipp32u  value;			// Right aligned -- WWD
	Ipp32u	valuetmp;
	int		moreBits;

	assert(bitsNumber >0);

	if(bitsNumber <= *pOffset)
	{

		value = **ppBitStream;
		value >>= (*pOffset - bitsNumber +1);
		value <<= (32-bitsNumber);

		// Update offset
		*pOffset -= bitsNumber;

#ifdef __WWD_BRANCH_TEST__
			test_branch[121] = 1;
#endif
	}
	else		// Need next DWORD, we need one DWORD at most
	{
		moreBits = bitsNumber - *pOffset -1;			// Indicate need moreBits insecod DWORD

		if(moreBits == 0)
		{
			value = **ppBitStream;
			value <<= (31 - *pOffset);


			(*ppBitStream)++;
			*pOffset = 31;

#ifdef __WWD_BRANCH_TEST__
			test_branch[122] = 1;
#endif

		}
		else
		{
			value = **ppBitStream;
			value <<= (31 - *pOffset);


			(*ppBitStream)++;
			//*pOffset = 31;

			valuetmp = **ppBitStream;
			valuetmp >>= (32 - moreBits);
			valuetmp <<= (31 - moreBits - *pOffset);
			value |= valuetmp;

			// Update offset
			*pOffset = 31 - moreBits;


#ifdef __WWD_BRANCH_TEST__
			test_branch[123] = 1;
#endif
		}

	}

	// Now value is left aligned, At this time we need right aligned

	value >>= (32 - bitsNumber);


	return value;

}
static Ipp32u peekBits(Ipp32u **ppBitStream,
                           Ipp32s *pOffset,
						   const  int	bitsNumber
						   )
{
	Ipp32u  value;			// Right aligned -- WWD
	Ipp32u	valuetmp;
	int		moreBits;

	//assert(bitsNumber >0);

	if(bitsNumber <= *pOffset)
	{

		value = **ppBitStream;
		value >>= (*pOffset - bitsNumber +1);
		value <<= (32-bitsNumber);

#ifdef __WWD_BRANCH_TEST__
			test_branch[124] = 1;
#endif

	}
	else		// Need next DWORD, we need one DWORD at most
	{
		moreBits = bitsNumber - *pOffset -1;			// Indicate need moreBits insecod DWORD

		if(moreBits == 0)
		{
			value = **ppBitStream;
			value <<= (31 - *pOffset);


#ifdef __WWD_BRANCH_TEST__
			test_branch[125] = 1;
#endif
		}
		else
		{
			value = **ppBitStream;
			value <<= (31 - *pOffset);


			valuetmp = *(*ppBitStream+1);
			valuetmp >>= (32 - moreBits);
			valuetmp <<= (31 - moreBits - *pOffset);
			value |= valuetmp;

#ifdef __WWD_BRANCH_TEST__
			test_branch[126] = 1;
#endif

		}

	}

	value >>= (32 - bitsNumber);

	return value;

}
// Scan function: This function should be replaced after optimization
static int code_from_bitstream_2d(Ipp32u **ppBitStream,
                           Ipp32s *pOffset,
						   int *lentab,
                           int *codtab,
                           int tabwidth,
                           int tabheight,
                           int *code1,
						   int *code2)
{
	int i,j;
	int len, cod, code;

	// this VLC decoding method is not optimized for speed
	for (j = 0; j < tabheight; j++) 
	{
		for (i = 0; i < tabwidth; i++)
		{
			len = lentab[i];
			if (!len)
				continue;
			cod = codtab[i];

			if ((code = peekBits(ppBitStream, pOffset,  len)) == cod)
			{

#ifdef __WWD_BRANCH_TEST__
			test_branch[127] = 1;
#endif
				// Update pointer
				if(len <= *pOffset)
				{
					*pOffset -= len;

#ifdef __WWD_BRANCH_TEST__
			test_branch[128] = 1;
#endif
				}
				else
					if((len - *pOffset) == 1)
					{
						(*ppBitStream)++;
						*pOffset = 31;

#ifdef __WWD_BRANCH_TEST__
			test_branch[129] = 1;
#endif
					}
					else
					{
						(*ppBitStream)++;
						*pOffset = 31 - (len - *pOffset -1);

#ifdef __WWD_BRANCH_TEST__
			test_branch[130] = 1;
#endif
					}

				*code1 = i;
				*code2 = j;

				goto found_code;
			}
		}
		lentab += tabwidth;
		codtab += tabwidth;
	}
  
  return -1;  // failed to find code

found_code:

  //*code = cod;

  return 0;
}
 /*
 **************************************************************************
 * Function:    ippiDecodeCAVLCCoeffs_H264_1u16s
 * Description: Do CAVLC decoding
 *
 * Problems   : 
 *
 * 
 **************************************************************************
 */
IppStatus __STDCALL ippiDecodeCAVLCCoeffs_H264_1u16s(Ipp32u **ppBitStream,
                                                     Ipp32s *pOffset,
                                                     Ipp16s *pNumCoeff,
                                                     Ipp16s **ppDstCoeffs,
                                                     Ipp32u uVLCSelect,
                                                     Ipp16s uMaxNumCoeff,
                                                     const Ipp32s **ppTblCoeffToken,
                                                     const Ipp32s **ppTblTotalZeros,
                                                     const Ipp32s **ppTblRunBefore,
                                                     const Ipp32s *pScanMatrix)
{
	int		i,	k;

	int		numcoeff, numtrailingones, numcoeff_vlc;
	int		level_two_or_higher;
	int		numones, totzeros, level, cdc=0, cac=0;
	int		zerosleft, ntr, dptype = 0;
	int		max_coeff_num = 0;
	int		incVlc[] = {0,3,6,12,24,48,32768};    // maximum vlc = 6
	
	int		code, *ct, *lt;

	int		retval;

	int		levarr[16], runarr[16];

	int		vlcnum;

	int		coeffNumber;

	numcoeff = 0;

	max_coeff_num = uMaxNumCoeff;

	switch(uVLCSelect)
	{
		case 0:
		case 1:
			numcoeff_vlc  = 0;

#ifdef __WWD_BRANCH_TEST__
			test_branch[131] = 1;
#endif
			break;

		case 2:
		case 3:
			numcoeff_vlc  = 1;

#ifdef __WWD_BRANCH_TEST__
			test_branch[132] = 1;
#endif
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			numcoeff_vlc  = 2;


#ifdef __WWD_BRANCH_TEST__
			test_branch[133] = 1;
#endif
			break;

		default:				// Fixed length table
			numcoeff_vlc  = 3;

			break;
	}


	// numcoeff_vlc is the index of Table used to code coeff_token
	// numcoeff_vlc==3 means (8<=nC) which uses 6bit FLC
	if (numcoeff_vlc == 3)
	{
		// read 6 bit FLC
		code = readBits(ppBitStream, pOffset, 6);

		numtrailingones = code & 3;
		numcoeff = (code >> 2);

		if (!numcoeff && numtrailingones == 3)
		{
			// #c = 0, #t1 = 3 =>  #c = 0
			numtrailingones = 0;

#ifdef __WWD_BRANCH_TEST__
			test_branch[134] = 1;
#endif
		}
		else
		{
			numcoeff++;

#ifdef __WWD_BRANCH_TEST__
			test_branch[135] = 1;
#endif
		}

		retval = 0;
	}
	else
	{
		lt = &Luma_lentab_Token[numcoeff_vlc][0][0];
		ct = &Luma_codtab_Token[numcoeff_vlc][0][0];
		retval = code_from_bitstream_2d(ppBitStream, pOffset, lt, ct, 17, 4, &numcoeff, &numtrailingones);

		if(retval)
		{

#ifdef __WWD_BRANCH_TEST__
			test_branch[136] = 1;
#endif
			return ippStsNullPtrErr;		
		}
#ifdef __WWD_BRANCH_TEST__
			test_branch[137] = 1;
#endif
	}

	numones = numtrailingones;
	*pNumCoeff = numcoeff;
	if (numcoeff)
	{
		for (k = 0; k < 16; k++)		// max_coeff_num
		{
			levarr[k] = 0;
			runarr[k] = 0;

			(*ppDstCoeffs)[k] = 0;
		}

		// Get the sign of trailing ones
		if (numtrailingones)
		{
			code = readBits(ppBitStream, pOffset,numtrailingones);

			ntr = numtrailingones;
			for (k = numcoeff-1; k > numcoeff-1-numtrailingones; k--)
			{
				ntr --;
				if ((code>>ntr)&1)
					levarr[k] = -1;
				else
					levarr[k] = 1;
			}

#ifdef __WWD_BRANCH_TEST__
			test_branch[138] = 1;
#endif
		}

		// decode levels
		// level_two_or_higher 变量的用途为numtrailingones 不足3个的时候，因为在编码的时候已经确定（这一项先-1）再编码的
		//     所以解码的时候特殊处理
		level_two_or_higher = 1;
		if (numcoeff > 3 && numtrailingones == 3)
		{
			level_two_or_higher = 0;

#ifdef __WWD_BRANCH_TEST__
			test_branch[139] = 1;
#endif
		}

		if (numcoeff > 10 && numtrailingones < 3)
		{
			vlcnum = 1;

#ifdef __WWD_BRANCH_TEST__
			test_branch[140] = 1;
#endif
		}
		else
		{
			vlcnum = 0;

#ifdef __WWD_BRANCH_TEST__
			test_branch[141] = 1;
#endif
		}

		for (k = numcoeff - 1 - numtrailingones; k >= 0; k--)
		{
			// 最终得出的数据存放到levle中
			if (vlcnum == 0)
			{

#ifdef __WWD_BRANCH_TEST__
			test_branch[142] = 1;
#endif
				int len, sign = 0, code;
				int offset, addbit;

				level = 0; 

				len = 0;
				while (!readBits(ppBitStream, pOffset, 1))
					len++;

				len++;
				code = 1;

				if (len < 15)
				{
					sign = (len - 1) & 1;
					level = (len-1) / 2 + 1;

#ifdef __WWD_BRANCH_TEST__
			test_branch[143] = 1;
#endif
				}
				else if (len == 15)
				{
					// escape code
					code = (code << 4) | readBits(ppBitStream, pOffset,4);
					len += 4;

 					sign = (code & 1);
					level = ((code >> 1) & 0x7) + 8;

#ifdef __WWD_BRANCH_TEST__
			test_branch[144] = 1;
#endif
				}
				else if (len >= 16)
				{
					// escape code
					addbit=len-16;
					code = readBits(ppBitStream, pOffset, (len-4));

					len  = (len-4);
					sign =  (code & 1);

					offset=(2048<<addbit) +16-2048;
					level = (code >> 1) + offset;

					// Delete
					code |= (1 << (len)); // for display purpose only

#ifdef __WWD_BRANCH_TEST__
			test_branch[145] = 1;
#endif
				}

				if (sign)
				{
					level = -level;

#ifdef __WWD_BRANCH_TEST__
			test_branch[146] = 1;
#endif
				}

			}
			else
			{

				int levabs, sign;
				int len = 0;
				int code, sb;
				  
				int numPrefix;
				int addbit, offset;

				int shift = vlcnum - 1;
				int escape = (15<<shift)+1;

				numPrefix = 0;
				while (!readBits(ppBitStream, pOffset, 1))
					numPrefix++;
				  
				  
				len = numPrefix + 1;
				code = 1;


#ifdef __WWD_BRANCH_TEST__
			test_branch[147] = 1;
#endif
				if (numPrefix < 15)
				{
					levabs = (numPrefix<<shift) + 1;
				    
					// read (vlc-1) bits -> suffix
					if (vlcnum-1)
					{
						sb =  readBits(ppBitStream, pOffset, vlcnum-1);
						code = (code << (vlcnum-1) )| sb;
						levabs += sb;
						len += (vlcnum-1);

#ifdef __WWD_BRANCH_TEST__
			test_branch[148] = 1;
#endif
					}
				    
					// read 1 bit -> sign
					sign = readBits(ppBitStream, pOffset, 1);
					code = (code << 1)| sign;
					len ++;

#ifdef __WWD_BRANCH_TEST__
			test_branch[149] = 1;
#endif
				}
				else // escape
				{
					addbit = numPrefix - 15;

					sb = readBits(ppBitStream, pOffset, (11+addbit));
					code = (code << (11+addbit) )| sb;

					len   += (11+addbit);
					offset = (2048<<addbit)+escape-2048;
					levabs = sb + offset;

					// read 1 bit -> sign
					sign = readBits(ppBitStream, pOffset, 1);
					code = (code << 1)| sign;
					len++;

#ifdef __WWD_BRANCH_TEST__
			test_branch[150] = 1;
#endif
				}
				  
				level = (sign)?-levabs:levabs;
			}

			if (level_two_or_higher)
			{
				if (level > 0)
					level ++;
				else
					level --;

				level_two_or_higher = 0;

#ifdef __WWD_BRANCH_TEST__
			test_branch[151] = 1;
#endif
			}

			levarr[k] = level;


			// update VLC table
			if (absm(level)>incVlc[vlcnum])
			{
				vlcnum++;

#ifdef __WWD_BRANCH_TEST__
			test_branch[152] = 1;
#endif
			}

			// 这里和标准一致，第一次可能出现连续增加2次的情况
			if (k == numcoeff - 1 - numtrailingones && absm(level)>3)
				vlcnum = 2;

		}

		// 解码TotalZeros相关的信息
		// 它的context为(numcoeff-1) -- 在IPP中不需要-1
		if (numcoeff < max_coeff_num)
		{
			// decode total run
			vlcnum = numcoeff - 1;

			lt = &Luma_lentab_TotalZeros[vlcnum][0];
			ct = &Luma_codtab_TotalZeros[vlcnum][0];

			retval = code_from_bitstream_2d(ppBitStream, pOffset, lt, ct, 16, 1, &totzeros, &i);

			if (retval)
			{
				printf("ERROR: failed to find Total Zeros\n");
				return ippStsNullPtrErr;
			}

#ifdef __WWD_BRANCH_TEST__
			test_branch[153] = 1;
#endif
		}
		else
		{
			totzeros = 0;

#ifdef __WWD_BRANCH_TEST__
			test_branch[154] = 1;
#endif
		}


		// decode run before each coefficient
		zerosleft = totzeros;
		i = numcoeff-1;
		if (zerosleft > 0 && i > 0)
		{
			do 
			{
				// select VLC for runbefore
				vlcnum = zerosleft - 1;
				if (vlcnum > RUNBEFORE_NUM-1)
					vlcnum = RUNBEFORE_NUM-1;

				lt = &lentab_Run[vlcnum][0];
				ct = &codtab_Run[vlcnum][0];

				retval = code_from_bitstream_2d(ppBitStream, pOffset, lt, ct, 16, 1, &runarr[i], &k);

				if (retval)
					return ippStsNullPtrErr;

				zerosleft -= runarr[i];

				i --;
			} while (zerosleft != 0 && i != 0);

#ifdef __WWD_BRANCH_TEST__
			test_branch[155] = 1;
#endif
		}
		runarr[i] = zerosleft;


		// The last step : SCAN order
		// Ipp16s **ppDstCoeffs
		// const Ipp32s *pScanMatrix
		if(uMaxNumCoeff == 16)		// Luma
			coeffNumber = -1;
		else
			coeffNumber = 0;		// Chroma AC part, We do not use [0] element WWD 

		for(k=0; k< numcoeff; k++)
		{

			coeffNumber += runarr[k] +1;
			(*ppDstCoeffs)[pScanMatrix[coeffNumber]] = levarr[k];
		}


	// According to Function spec in Ippiman.pdf
	(*ppDstCoeffs) += 16;		 
	}

	return ippStsNoErr;

}
IppStatus __STDCALL ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s(Ipp32u **ppBitStream,
                                                             Ipp32s *pOffset,
                                                             Ipp16s *pNumCoeff,
                                                             Ipp16s **ppDstCoeffs,
                                                             const Ipp32s *pTblCoeffToken,
                                                             const Ipp32s **ppTblTotalZerosCR,
                                                             const Ipp32s **ppTblRunBefore)
{
	int		i,	k;

	int		numcoeff, numtrailingones;
	int		level_two_or_higher;
	int		numones, totzeros, level, cdc=0, cac=0;
	int		zerosleft, ntr, dptype = 0;
	int		max_coeff_num = 4;
	int		incVlc[] = {0,3,6,12,24,48,32768};    // maximum vlc = 6
	
	int		code, *ct, *lt;

	int		retval;

	int		levarr[16], runarr[16];

	int		vlcnum;

	int		coeffNumber;

	numcoeff = 0;

	//max_coeff_num = uMaxNumCoeff;

	// Support YUV420 format ONLY at present
	lt = &Chroma_lentab_Token[0][0];
	ct = &Chroma_codtab_Token[0][0];
	retval = code_from_bitstream_2d(ppBitStream, pOffset, lt, ct, 17, 4, &numcoeff, &numtrailingones);

	if(retval)
	{
		printf("ERR\n");
		return ippStsNullPtrErr;
	}

	for (k = 0; k < max_coeff_num; k++)
	{
		levarr[k] = 0;
		runarr[k] = 0;

		(*ppDstCoeffs)[k] = 0;

#ifdef __WWD_BRANCH_TEST__
			test_branch[156] = 1;
#endif
	}

	numones = numtrailingones;

	*pNumCoeff = numcoeff;


	if (numcoeff)
	{
		// 取得最后的几个+/-1，并存储
		if (numtrailingones)
		{

			code = readBits(ppBitStream, pOffset,numtrailingones);

			ntr = numtrailingones;
			for (k = numcoeff-1; k > numcoeff-1-numtrailingones; k--)
			{
				ntr --;
				if ((code>>ntr)&1)
					levarr[k] = -1;
				else
					levarr[k] = 1;
			}

#ifdef __WWD_BRANCH_TEST__
			test_branch[157] = 1;
#endif
		}

		// decode levels
		// level_two_or_higher 变量的用途为numtrailingones 不足3个的时候，因为在编码的时候已经确定（这一项先-1）再编码的
		//     所以解码的时候特殊处理
		level_two_or_higher = 1;
		if (numcoeff > 3 && numtrailingones == 3)
			level_two_or_higher = 0;

		if (numcoeff > 10 && numtrailingones < 3)
			vlcnum = 1;
		else
			vlcnum = 0;

		for (k = numcoeff - 1 - numtrailingones; k >= 0; k--)
		{
			// 最终得出的数据存放到levle中
			if (vlcnum == 0)
			{
				int len, sign = 0, code;
				int offset, addbit;

				level = 0; 

				len = 0;
				while (!readBits(ppBitStream, pOffset, 1))
					len++;

				len++;
				code = 1;

				if (len < 15)
				{
					sign = (len - 1) & 1;
					level = (len-1) / 2 + 1;

#ifdef __WWD_BRANCH_TEST__
			test_branch[158] = 1;
#endif
				}
				else if (len == 15)
				{
					// escape code
					code = (code << 4) | readBits(ppBitStream, pOffset,4);
					len += 4;

 					sign = (code & 1);
					level = ((code >> 1) & 0x7) + 8;

#ifdef __WWD_BRANCH_TEST__
			test_branch[159] = 1;
#endif
				}
				else if (len >= 16)
				{
					// escape code
					addbit=len-16;
					code = readBits(ppBitStream, pOffset, (len-4));

					len  = (len-4);
					sign =  (code & 1);

					offset=(2048<<addbit) +16-2048;
					level = (code >> 1) + offset;

					// Delete
					code |= (1 << (len)); // for display purpose only

#ifdef __WWD_BRANCH_TEST__
			test_branch[160] = 1;
#endif
				}

				if (sign)
				{
					level = -level;

#ifdef __WWD_BRANCH_TEST__
			test_branch[161] = 1;
#endif
				}
				
				//readSyntaxElement_Level_VLC0(&currSE, dP);

			}
			else
			{

				int levabs, sign;
				int len = 0;
				int code, sb;
				  
				int numPrefix;
				int addbit, offset;

				int shift = vlcnum - 1;
				int escape = (15<<shift)+1;

				numPrefix = 0;
				while (!readBits(ppBitStream, pOffset, 1))
					numPrefix++;
				  
				  
				len = numPrefix + 1;
				code = 1;

#ifdef __WWD_BRANCH_TEST__
			test_branch[162] = 1;
#endif
				if (numPrefix < 15)
				{
					levabs = (numPrefix<<shift) + 1;
				    
					// read (vlc-1) bits -> suffix
					if (vlcnum-1)
					{
						sb =  readBits(ppBitStream, pOffset, vlcnum-1);
						code = (code << (vlcnum-1) )| sb;
						levabs += sb;
						len += (vlcnum-1);

#ifdef __WWD_BRANCH_TEST__
			test_branch[163] = 1;
#endif
					}
				    
					// read 1 bit -> sign
					sign = readBits(ppBitStream, pOffset, 1);
					code = (code << 1)| sign;
					len ++;
				}
				else // escape
				{
					addbit = numPrefix - 15;

					sb = readBits(ppBitStream, pOffset, (11+addbit));
					code = (code << (11+addbit) )| sb;

					len   += (11+addbit);
					offset = (2048<<addbit)+escape-2048;
					levabs = sb + offset;

					// read 1 bit -> sign
					sign = readBits(ppBitStream, pOffset, 1);
					code = (code << 1)| sign;
					len++;

#ifdef __WWD_BRANCH_TEST__
			test_branch[164] = 1;
#endif
				}
				  
				level = (sign)?-levabs:levabs;
				//sym->len = len;

				//readSyntaxElement_Level_VLCN(&currSE, vlcnum, dP);

			}

			if (level_two_or_higher)
			{
				if (level > 0)
					level ++;
				else
					level --;

				level_two_or_higher = 0;

#ifdef __WWD_BRANCH_TEST__
			test_branch[165] = 1;
#endif
			}

			levarr[k] = level;


			// update VLC table
			if (absm(level)>incVlc[vlcnum])
			{
				vlcnum++;

#ifdef __WWD_BRANCH_TEST__
			test_branch[166] = 1;
#endif
			}

			// 这里和标准一致，第一次可能出现连续增加2次的情况
			if (k == numcoeff - 1 - numtrailingones && absm(level)>3)
			{
				vlcnum = 2;

#ifdef __WWD_BRANCH_TEST__
			test_branch[167] = 1;
#endif
			}

		}

		// 解码TotalZeros相关的信息
		// 它的context为(numcoeff-1) -- 在IPP中不需要-1
		if (numcoeff < max_coeff_num)
		{
			// decode total run
			vlcnum = numcoeff - 1;

			// Support YUV420 ONLY at present
			lt = &Chroma_lentab_TotalZeros[vlcnum][0];
			ct = &Chroma_codtab_TotalZeros[vlcnum][0];

			retval = code_from_bitstream_2d(ppBitStream, pOffset, lt, ct, 16, 1, &totzeros, &i);

			if (retval)
			{
				printf("ERROR: failed to find Total Zeros\n");
				return ippStsNullPtrErr;
			}

#ifdef __WWD_BRANCH_TEST__
			test_branch[168] = 1;
#endif
		}
		else
		{
			totzeros = 0;

#ifdef __WWD_BRANCH_TEST__
			test_branch[169] = 1;
#endif
		}


		// decode run before each coefficient
		zerosleft = totzeros;
		i = numcoeff-1;
		if (zerosleft > 0 && i > 0)
		{
			do 
			{
				// select VLC for runbefore
				vlcnum = zerosleft - 1;
				if (vlcnum > RUNBEFORE_NUM-1)
					vlcnum = RUNBEFORE_NUM-1;

				lt = &lentab_Run[vlcnum][0];
				ct = &codtab_Run[vlcnum][0];

				retval = code_from_bitstream_2d(ppBitStream, pOffset, lt, ct, 16, 1, &runarr[i], &k);

				if (retval)
				{
					printf("ERROR: failed to find Run\n");
					return ippStsNullPtrErr;
				}

				zerosleft -= runarr[i];

				i --;
			} while (zerosleft != 0 && i != 0);


#ifdef __WWD_BRANCH_TEST__
			test_branch[170] = 1;
#endif
		}
		runarr[i] = zerosleft;


		// The last step : SCAN order
		// Ipp16s **ppDstCoeffs
		// const Ipp32s *pScanMatrix
		//if(uMaxNumCoeff == 16)		// Luma
			coeffNumber = -1;
		//else
		//	coeffNumber = 0;		// Chroma AC part, We do not use [0] element WWD 

		for(k=0; k< numcoeff; k++)
		{

			coeffNumber += runarr[k] +1;
			(*ppDstCoeffs)[coeffNumber] = levarr[k];
		}


		// According to Function spec in Ippiman.pdf
		(*ppDstCoeffs) += 4;		 
	}

	return ippStsNoErr;
}
#endif
