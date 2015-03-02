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
#ifndef KINOMA_FAST_HUFFMAN

#include "kinoma_ipp_common.h"

IppStatus (__STDCALL *ippsVLCDecodeEscBlock_MP3_1u16s_universal)  (Ipp8u **ppBitStream, int *pBitOffset, int linbits, Ipp16s *pData, int len, const IppsVLCDecodeSpec_32s *pVLCSpec) = NULL;

/* function used in ippsVLCDecodeEscBlock_MP3_1u16s */
static Ipp16s __inline GetMp3Esc(Ipp8u **ppBitStream, int* pBitOffset, int linbits)
{
	int l;
	unsigned int	bits, currBits; 

	bits = ((*ppBitStream)[0] << 24) | ((*ppBitStream)[1] << 16) | ((*ppBitStream)[2] <<  8) | ((*ppBitStream)[3]);
	bits <<= *pBitOffset;

	currBits = bits >> (32-linbits);		/* the max linbits is 13*/

	l = linbits + *pBitOffset;	
	*ppBitStream += l >> 3; 
	*pBitOffset = l & 7;

	return (currBits + 15);	
}

/* function used in ippsVLCDecodeEscBlock_MP3_1u16s */
static int __inline SignMp3Esc(Ipp8u **ppBitStream, int* pBitOffset)
{
	int l;
	unsigned char	bits; 
	bits = (*ppBitStream)[0]; 
	bits <<= *pBitOffset;

	l = 1 + *pBitOffset;	
	*ppBitStream += l >> 3; 
	*pBitOffset = l & 7;	

	return ((bits >>  7) > 0 ? -1: 1);
}

/* following functions only used in mp3*/
IppStatus __STDCALL 
ippsVLCDecodeEscBlock_MP3_1u16s_c(Ipp8u **ppBitStream,
                                int *pBitOffset,
                                int linbits,
                                Ipp16s *pData,
                                int len,
                                const IppsVLCDecodeSpec_32s *pVLCSpec)
{
#ifdef KINOMA_VLC
	int x,c=0, needEsc;
	Ipp16s tmp,tmp1,tmp2;

	unsigned int	bits, currBits; 
	int		i,k,j,m,l;
	int		find;
	int		length;
	k_IppsVLCDecodeSpec_32s *pMyVLCSpec = (k_IppsVLCDecodeSpec_32s *)pVLCSpec;

	if (!ppBitStream || ! pData || ! pVLCSpec)
		return ippStsNullPtrErr;
	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;
	if (pMyVLCSpec->sig != VLC_SIG)
		return ippStsContextMatchErr;

	for (x = 0; x < len/2; x++)
	{
		bits = ((*ppBitStream)[0] << 24) | ((*ppBitStream)[1] << 16) | ((*ppBitStream)[2] <<  8) | ((*ppBitStream)[3]);
		bits <<= *pBitOffset;
		find = 0;

		for (m = 0; m < pMyVLCSpec->numSubTables; m++)
		{
			if (m == 0)
			{
				j = 1;
				l = 0;
			}
			else
			{
				j += pMyVLCSpec->pSubTablesSizes[m-1];
				l = pMyVLCSpec->tablePos[m-1];
			}

			for (k = j; k < j+pMyVLCSpec->pSubTablesSizes[m]; k++)
			{
				currBits = bits >> (32 - k);
				for (i = l; i < pMyVLCSpec->tablePos[m]; i++)
				{
					if((Ipp32s)currBits == pMyVLCSpec->pNewTable[i].code && k == pMyVLCSpec->pNewTable[i].length)
					{
						// found it
						length = k;
						tmp = pMyVLCSpec->pNewTable[i].value;
						find =1;
						break;
					}
				}
				if(find)
					break;
			}
			if(find)
				break;
		}

		if (!find)
			return ippStsVLCInputDataErr;

		length = length + *pBitOffset;
		*ppBitStream += length >> 3;
		*pBitOffset = length & 7;

		/* esc */
		tmp1 = tmp >> 8;
		tmp2 = tmp - 256 * tmp1 - 128 ;

		needEsc = 0;	
		/*move the bitOffset backward 1 or 2 bits which the corresponding code value != 0 in ESC mode*/
		if ((abs(tmp1) == 15 || abs(tmp2) == 15) && (abs(tmp1) + abs(tmp2) > 15))
		{
			*pBitOffset = *pBitOffset - 2;
			if (*pBitOffset < 0)
			{
				*pBitOffset += 8;
				(*ppBitStream)--;
			}
			needEsc = 1;
		}
		else if ((abs(tmp1) == 15 || abs(tmp2) == 15) && (abs(tmp1) + abs(tmp2) == 15))
		{
			*pBitOffset = *pBitOffset - 1;
			if (*pBitOffset < 0)
			{
				*pBitOffset = 7;
				(*ppBitStream)--;
			}
			needEsc = 1;
		}

		if (!needEsc)
		{
			pData[c++] = tmp1;
			pData[c++] = tmp2;
		}
		else
		{
			if (abs(tmp1) == 15)
				tmp1 = GetMp3Esc(ppBitStream,pBitOffset,linbits);

			if (tmp1 != 0)
				pData[c++] = abs(tmp1) * SignMp3Esc(ppBitStream,pBitOffset);
			else 
				pData[c++] = 0;


			if (abs(tmp2) == 15)
				tmp2 = GetMp3Esc(ppBitStream,pBitOffset,linbits);

			if (tmp2 != 0)
				pData[c++] = abs(tmp2) * SignMp3Esc(ppBitStream,pBitOffset);
			else 
				pData[c++] = 0;
		}

	}
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsVLCDecodeEscBlock_MP3_1u16s",-1);
#endif

#ifdef INTEL_VLC
	{
		IppStatus sts;
		sts = ippsVLCDecodeEscBlock_MP3_1u16s(ppBitStream,pBitOffset,linbits,pData,len,pVLCSpec);
		return sts;
	}
#endif
	return ippStsNoErr;
}

#endif
