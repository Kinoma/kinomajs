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
#if  1//ndef KINOMA_FAST_HUFFMAN		//this is still needed for aac sbr before it's optimized

#include "kinoma_ipp_lib.h"
#include "kinoma_ipp_common.h"

IppStatus (__STDCALL *ippsVLCDecodeBlock_1u16s_universal)		(Ipp8u** ppSrc, int* pSrcBitsOffset, Ipp16s* pDst, int dstLen, const IppsVLCDecodeSpec_32s* pVLCSpec)=NULL;
void	  (__STDCALL *ippsVLCDecodeFree_32s_universal)			(IppsVLCDecodeSpec_32s* pVLCSpec)=NULL;
IppStatus (__STDCALL *ippsVLCDecodeInitAlloc_32s_universal )	(const IppsVLCTable_32s* pInputTable, int inputTableSize, Ipp32s* pSubTablesSizes, int numSubTables, IppsVLCDecodeSpec_32s** ppVLCSpec)=NULL;
IppStatus (__STDCALL *ippsVLCDecodeOne_1u16s_universal)			(Ipp8u** ppSrc, int* pSrcBitsOffset, Ipp16s* pDst, const IppsVLCDecodeSpec_32s* pVLCSpec)=NULL;

IppStatus __STDCALL 
ippsVLCDecodeInitAlloc_32s_c(const IppsVLCTable_32s* pInputTable,
						   int inputTableSize, Ipp32s* pSubTablesSizes, int numSubTables,
						   IppsVLCDecodeSpec_32s** ppVLCSpec)
{
#ifdef KINOMA_VLC 
	int i,k,m,count = 0;
	int maxLength= 0, sumSubLen = 0;

	k_IppsVLCDecodeSpec_32s *pMyVLCSpec = (k_IppsVLCDecodeSpec_32s*)ippsMalloc_8u_c(sizeof(k_IppsVLCDecodeSpec_32s));
	if (!pMyVLCSpec)
		return ippStsMemAllocErr;

	/*verify input data*/
	if (!pInputTable || !pSubTablesSizes || !ppVLCSpec)
		return ippStsNullPtrErr;

	for (i = 0; i < inputTableSize; i++)
	{
		if (pInputTable->length > maxLength)
			maxLength = pInputTable->length;
		if (pInputTable->length > 32)
			return ippStsVLCUsrTblCodeLengthErr;
	}
	for (i = 0; i < numSubTables; i++)
	{
		sumSubLen += pSubTablesSizes[i];
		if (pSubTablesSizes[i] < 0)
			return ippStsVLCUsrTblCodeLengthErr;
	}
	if (sumSubLen < maxLength)
		return ippStsVLCUsrTblCodeLengthErr;
	/* verification end*/


	pMyVLCSpec->sig = VLC_SIG;
	pMyVLCSpec->pInputTable = (IppsVLCTable_32s*)pInputTable;
	pMyVLCSpec->inputTableSize = inputTableSize;
	pMyVLCSpec->pSubTablesSizes = pSubTablesSizes;
	pMyVLCSpec->numSubTables = numSubTables;

	pMyVLCSpec->pNewTable = (IppsVLCTable_32s*)ippsMalloc_8u_c(sizeof(IppsVLCTable_32s) * inputTableSize);
	if (!pMyVLCSpec->pNewTable)
	{
		ippsFree_c(pMyVLCSpec);
		return ippStsMemAllocErr;
	}
	/* rearrange the items into "numSubTables" subtables*/
	for (i = 0; i < numSubTables; i++)
	{
		if (i > 0)
			m += pSubTablesSizes[i-1];
		else
			m = 0;
		for (k = 0; k < inputTableSize; k++)
		{
			if (pInputTable[k].length <= (m+pSubTablesSizes[i]) && pInputTable[k].length > m)
			{
				pMyVLCSpec->pNewTable[count].code = pInputTable[k].code;
				pMyVLCSpec->pNewTable[count].length = pInputTable[k].length;
				pMyVLCSpec->pNewTable[count].value = pInputTable[k].value;
				count++;
			}
		}
		pMyVLCSpec->tablePos[i] = count;
	}

	*ppVLCSpec = (IppsVLCDecodeSpec_32s*)pMyVLCSpec;
#endif


#ifdef PRINT_REF_INFO
	//dump("ippsVLCDecodeInitAlloc_32s",-1);
#endif

#ifdef INTEL_VLC
	{
		IppStatus sts;
		sts = ippsVLCDecodeInitAlloc_32s(pInputTable, inputTableSize,  pSubTablesSizes,  numSubTables, ppVLCSpec);
		return sts;
	}
#endif

	return ippStsNoErr;
}

void __STDCALL ippsVLCDecodeFree_32s_c (IppsVLCDecodeSpec_32s* pVLCSpec)
{
#ifdef KINOMA_VLC
	k_IppsVLCDecodeSpec_32s *pMyVLCSpec = (k_IppsVLCDecodeSpec_32s *)pVLCSpec;

    if (pMyVLCSpec)
    {
        if (pMyVLCSpec->pNewTable)
            ippsFree_c(pMyVLCSpec->pNewTable);
            
        ippsFree_c(pMyVLCSpec);
    }
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsVLCDecodeFree_32s",-1);
#endif

#ifdef INTEL_VLC
	{
		ippsVLCDecodeFree_32s(pVLCSpec);
	}
#endif
}


IppStatus __STDCALL 
ippsVLCDecodeOne_1u16s_c(Ipp8u** ppSrc, int* pSrcBitsOffset, Ipp16s* pDst,
						const IppsVLCDecodeSpec_32s* pVLCSpec)
{
#ifdef KINOMA_VLC
	unsigned int	bits, currBits; 
	int		i,k,j,m,l;
	int		find=0;
	int		len;
	k_IppsVLCDecodeSpec_32s *pMyVLCSpec = (k_IppsVLCDecodeSpec_32s *)pVLCSpec;

	if (!ppSrc || ! pDst || ! pVLCSpec)
		return ippStsNullPtrErr;
	if (*pSrcBitsOffset < 0 || *pSrcBitsOffset > 7)
		return ippStsBitOffsetErr;
	if (pMyVLCSpec->sig != VLC_SIG)
		return ippStsContextMatchErr;

    bits = ((*ppSrc)[0] << 24) | ((*ppSrc)[1] << 16) | ((*ppSrc)[2] <<  8) | ((*ppSrc)[3]);
	bits <<= *pSrcBitsOffset;

	/*	search from 0 to end in the original table
	for(k = 1; k < 32; k++)
	{
		currBits = (bits >> (32 - k));

		// Start find current bits in table
		for(i = 0; i < pMyVLCSpec->inputTableSize; i++)
		{
			// 
			if(currBits == pMyVLCSpec->pInputTable[i].code && k == pMyVLCSpec->pInputTable[i].length)
			{
				// found it
				len = k;
				*pDst = pMyVLCSpec->pInputTable[i].value;
				find =1;
				break;
			}
		}
		if(find)
			break;
	}
	*/

	/* search in SubTables, faster than the search method in the above comment code */
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
					len = k;
					*pDst = pMyVLCSpec->pNewTable[i].value;
					find = 1;
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

	len = len + *pSrcBitsOffset;
    *ppSrc += len >> 3;
    *pSrcBitsOffset = len & 7;

#endif

#ifdef PRINT_REF_INFO
	//dump("ippsVLCDecodeOne_1u16s_c",-1);
#endif

#ifdef INTEL_VLC
	{
		IppStatus sts;
		sts = ippsVLCDecodeOne_1u16s(ppSrc,pSrcBitsOffset, pDst, pVLCSpec);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL 
ippsVLCDecodeBlock_1u16s_c (Ipp8u** ppSrc, int* pSrcBitsOffset, Ipp16s*
						  pDst, int dstLen, const IppsVLCDecodeSpec_32s* pVLCSpec)
{
#ifdef KINOMA_VLC
	int i;
	
	for (i = 0; i < dstLen; i++)
		ippsVLCDecodeOne_1u16s_c(ppSrc, pSrcBitsOffset, &pDst[i],pVLCSpec);
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsVLCDecodeBlock_1u16s",-1);
#endif

#ifdef INTEL_VLC
	{
		IppStatus sts;
		sts = ippsVLCDecodeBlock_1u16s (ppSrc, pSrcBitsOffset, pDst, dstLen,pVLCSpec);
		return sts;
	}
#endif 

	return ippStsNoErr;
}
#endif
