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
#ifdef __KINOMA_IPP__

#include <assert.h>

#include "ipps.h"
#include "ippdc.h"

#include "kinoma_aac_defines.h"
#include "kinoma_ipp_lib.h"


// AAC encoder VLC
IppStatus (__STDCALL *ippsVLCEncodeBlock_16s1u_universal)		(const Ipp16s* pSrc, int srcLen, Ipp8u** ppDst, int* pDstBitsOffset, const IppsVLCEncodeSpec_32s* pVLCSpec)=NULL;
IppStatus (__STDCALL *ippsVLCEncodeEscBlock_AAC_16s1u_universal)	(const Ipp16s *pSrc,int srcLen,  Ipp8u **ppDst,  int *pDstBitsOffset, const IppsVLCEncodeSpec_32s *pVLCSpec)=NULL;
IppStatus (__STDCALL *ippsVLCEncodeInitAlloc_32s_universal)		(const IppsVLCTable_32s* pInputTable, int inputTableSize, IppsVLCEncodeSpec_32s** ppVLCSpec)=NULL;
void 	  (__STDCALL *ippsVLCEncodeFree_32s_universal)				(IppsVLCEncodeSpec_32s* pVLCSpec)=NULL;
IppStatus (__STDCALL *ippsVLCCountBits_16s32s_universal)			(const Ipp16s* pSrc, int srcLen, Ipp32s* pCountBits, const IppsVLCEncodeSpec_32s* pVLCSpec)=NULL;
IppStatus (__STDCALL *ippsVLCCountEscBits_AAC_16s32s_universal)	(const Ipp16s *pSrc, int srcLen,  Ipp32s *pCountBits,    const IppsVLCEncodeSpec_32s *pVLCSpec)=NULL;


#define unsued_item 0xbaadf00d

static unsigned int VLC_mask[33] = {
  0x0,
  0x01,         0x03,       0x07,       0x0F,
  0x01F,        0x03F,      0x07F,      0x0FF,
  0x01FF,       0x03FF,     0x07FF,     0x0FFF,
  0x01FFF,      0x03FFF,    0x07FFF,    0x0FFFF,
  0x01FFFF,     0x03FFFF,   0x07FFFF,   0x0FFFFF,
  0x01FFFFF,    0x03FFFFF,  0x07FFFFF,  0x0FFFFFF,
  0x01FFFFFF,   0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
  0x1FFFFFFF,   0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};


/*
typedef struct _K_IppsVLCEncodeSpec_32s
{
	unsigned int *startAddr;
	int minValue;
	int order;
	int alloc;
	int flag;  //xSML
	int dummy0, dummy1, dummy2;
}K_IppsVLCEncodeSpec_32s;
*/

#ifdef _BIG_ENDIAN_
#define BSWAP(x) (x)
#else
#define BSWAP(x) (unsigned int)(((x) << 24) | (((x)&0xff00) << 8) | (((x) >> 8)&0xff00) | ((x&0xff000000) >> 24));
#endif

static __inline void Buff_full(Ipp8u* pDst, unsigned code)
{
	*pDst = (code >> 24) & 0xFF;
	*(pDst+1) = (code >> 16) & 0xFF;
	*(pDst+2) = (code >> 8) & 0xFF;
	*(pDst+3) = (code ) & 0xFF;
}
/************************************************************************************************************
 * Here we must note one thing: preserve bits and "|" to the result
 *   That means VLC is not output in sequence
 *
 ************************************************************************************************************
 */
/************************************************************************************************************
 *  Function Name : ippsVLCEncodeEscBlock_AAC_16s1u_c
 *  Note:           (+/-, s1's sign)16 * 64 + 16 + (+/-,S2's sign)16
 *
 *
 ************************************************************************************************************
 */
// This function generate different bit-stream when compared with IPP function, but it do NOT matter, the decoded stream is same.
//   I have double checked this and found that the differenc lies in bits after STOP bits. So, it do not matter
IppStatus __STDCALL ippsVLCEncodeBlock_16s1u_c(const Ipp16s* pSrc, int srcLen, Ipp8u** ppDst, int* pDstBitsOffset, const IppsVLCEncodeSpec_32s* pVLCSpec)
{
	Ipp16s value;
	unsigned int symbol_code, symbol_len, code0;
	int i, k;
	K_IppsVLCEncodeSpec_32s *pcurrVLCTbl = (K_IppsVLCEncodeSpec_32s *) (pVLCSpec);
	unsigned int buff, bits_to_go, bitsOffset;

	int valueOffset = 1<< pcurrVLCTbl->order;
	unsigned int *startAddr = pcurrVLCTbl->startAddr;
	int  minValue = pcurrVLCTbl->minValue;

	// The left-most bits is useful
	buff = **ppDst;
	bits_to_go = 32 - *pDstBitsOffset;
	buff >>= (8- *pDstBitsOffset);

	for(i=0; i<srcLen; i++)
	{
		value = (*pSrc) + minValue;

		// Find correct code and length for this value
		symbol_len  = (unsigned int)(*(startAddr + value));
		symbol_code = (unsigned int)(*(startAddr + valueOffset + value));

		// Store code into bit-stream
		if(bits_to_go >= symbol_len) //enough bits
		{
			buff = (buff << (symbol_len)) | symbol_code;
			bits_to_go -= symbol_len;
			if(bits_to_go == 0)
			{
				Buff_full(*ppDst, buff);
				*ppDst += 4;
				bits_to_go = 32;
			}
			
		}
		else // need store
		{
			k= symbol_len - bits_to_go;
			code0 = symbol_code >> k;

			buff = (buff << bits_to_go) | code0;
			Buff_full(*ppDst, buff);
			*ppDst += 4;
			buff = (symbol_code - (code0<<k));
			bits_to_go = 32 - k;
			assert(k<32);
		}
		pSrc ++;
	}


	// Tail
	if(bits_to_go == 32)
	{
		*pDstBitsOffset = 0;
	}
	else
	{
		// Set pDstBitsOffset
		k = (32 - bits_to_go +7)/8;
		buff <<= (bits_to_go);

		for(i=0; i<k-1; i++)
		{
			**ppDst = (buff >> ((3-i)*8)) & 0xFF;
			*ppDst += 1;
		}

		bitsOffset = bits_to_go%8;
		code0 = (**ppDst) & VLC_mask[bitsOffset];

		buff = ((buff >>(4-k)*8) & 0xFF) | code0;
		**ppDst = buff;

		*pDstBitsOffset = 8 - bitsOffset;

		if(bitsOffset ==0)
		{
			*ppDst += 1;
			*pDstBitsOffset = 0;
		}
	}

	return ippStsNoErr;
}

/*********************************************************************************************************************************
 * Here is notes for further optimization (ASM part)
 * We encode one pair here (first_coeff, second_coeff)
 *  Index = first_coeff*64 + second_coeff + 16
 *    But coeffs are in range [-16, 16], and Intel's table consist sign already
 *    For example, spec says (18, 4, 1) in tableA-13. It is used to encode (1,1)
 *        1*64 + 16 +1 == 81
 *        0001, 4bits ==> 0001 00 (sign) = 00 0100 (6 bits) ==> {   80, 0x0000000a,  6}, in table IppsVLCTable_32s spec_book11[] 
 *
 *   Then we need encode esc_sequence (This is need ONLY when coeff>=16): N's "1" + "0" + N+4's (value) 
 **********************************************************************************************************************************
 */
IppStatus __STDCALL ippsVLCEncodeEscBlock_AAC_16s1u_c(const Ipp16s *pSrc,int srcLen,  Ipp8u **ppDst,  int *pDstBitsOffset, const IppsVLCEncodeSpec_32s *pVLCSpec)
{
	Ipp8u *pDstStart;
	Ipp32s value, value1, value2, valuetemp;
	unsigned int symbol_code, symbol_len, code0;
	int i, k;
	K_IppsVLCEncodeSpec_32s *pcurrVLCTbl = (K_IppsVLCEncodeSpec_32s *) (pVLCSpec);
	unsigned int buff, bits_to_go, real_bits, bitsOffset;
	unsigned char buff0, buff1;  // buff0 is used for the first char (**ppDst), buff1 is used for (**ppDst -- result position)

	int valueOffset = 1<< pcurrVLCTbl->order;
	unsigned int *startAddr = pcurrVLCTbl->startAddr;
	int  minValue = pcurrVLCTbl->minValue;

	// First caculate start addr (4bytes aligned position)
	pDstStart = *ppDst;

	// The left-most bits is useful
	buff = buff0 = (*pDstStart);
	real_bits = *pDstBitsOffset;
	bits_to_go = 32 - real_bits;
	buff >>= (8- real_bits);


	for(i=0; i<srcLen/2; i++)
	{
		// First ESC flag
		value1 = (*pSrc ==0)?0 :(*pSrc);
		if(value1 >=16) 
			value1 = 16;
		else
			if(value1 <= -16)
				value1 = -16;
		value2 = (*(pSrc+1) ==0)?0 :(*(pSrc+1));
		if(value2 >=16) 
			value2 = 16;
		else
			if(value2 <= -16)
				value2 = -16;
		value = (value1)*64 + value2 + 16;

		value += minValue;

		// Find correct code and length for this value
		symbol_len  = (unsigned int)(*(startAddr + value));
		symbol_code = (unsigned int)(*(startAddr + valueOffset + value));

		// Store code into bit-stream
		if(bits_to_go >= symbol_len) //enough bits
		{
			buff = (buff << (symbol_len)) | symbol_code;
			bits_to_go -= symbol_len;
			if(bits_to_go == 0)
			{
				Buff_full(*ppDst, buff);
				*ppDst += 4;
				bits_to_go = 32;
			}			
		}
		else // need store
		{
			k= symbol_len - bits_to_go;
			code0 = symbol_code >> k;

			buff = (buff << bits_to_go) | code0;
			Buff_full(*ppDst, buff);
			*ppDst += 4;
			buff = (symbol_code - (code0<<k));
			bits_to_go = 32 - k;
			assert(k<32);
		}

		// Second, ESC sequence : spec says the max bits for each one is 22bits, so, 32 bits is enough (We do not need store bits inter)
		value1 = abs(*pSrc);
		if(value1 >= 16)
		{
			// Caculate N
			symbol_code = 0;
			symbol_len = 0;

			if(value1 > 31)
			{
				k=1;
				symbol_code = 1;
				valuetemp = value1>>5;
				while(valuetemp > 1)
				{
					k++; valuetemp >>= 1; symbol_code = (symbol_code<<1) |1;
				}

				symbol_len = k;
			}

			// One ZERo seprator
			symbol_len ++; // one zero
			symbol_code <<= 1;

			// N+4 bits
			valuetemp = (1<< (symbol_len -1)) * 16;
			value1 = value1 - valuetemp;


			symbol_code = (symbol_code << ((symbol_len -1) + 4))| value1;
			symbol_len  += (symbol_len -1) + 4;  // (symbol_len -1) = N

			// Store code into bit-stream
			if(bits_to_go >= symbol_len) //enough bits
			{
				buff = (buff << (symbol_len)) | symbol_code;
				bits_to_go -= symbol_len;
				if(bits_to_go == 0)
				{
					Buff_full(*ppDst, buff);
					*ppDst += 4;
					bits_to_go = 32;
				}
			}
			else // need store
			{
				k= symbol_len - bits_to_go;
				code0 = symbol_code >> k;
				buff = (buff << bits_to_go) | code0;

				Buff_full(*ppDst, buff);
				*ppDst += 4;
				buff = (symbol_code - (code0<<k));
				bits_to_go = 32 - k;
				assert(k<32);
			}
		}

		value2 = abs(*(pSrc+1));
		if(value2 >= 16)
		{
			// Caculate N
			symbol_code = 0;
			symbol_len = 0;
			
			if(value2 > 31)
			{
				k=1;
				symbol_code = 1;
				valuetemp = value2>>5;
				while(valuetemp > 1)
				{
					k++; valuetemp >>= 1; symbol_code = (symbol_code<<1) |1;
				}

				symbol_len = k;
			}

			// One ZERo seprator
			symbol_len ++; // one zero
			symbol_code <<= 1;

			//
			valuetemp = (1<< (symbol_len -1)) * 16;
			value1 = value2 - valuetemp;


			symbol_code = (symbol_code << ((symbol_len -1) + 4))| value1;
			symbol_len  += (symbol_len -1) + 4;

			// Store code into bit-stream
			if(bits_to_go >= symbol_len) //enough bits
			{
				buff = (buff << (symbol_len)) | symbol_code;
				bits_to_go -= symbol_len;
				if(bits_to_go == 0)
				{
					Buff_full(*ppDst, buff);
					*ppDst += 4;
					bits_to_go = 32;
				}
			}
			else // need store
			{
				k= symbol_len - bits_to_go;
				code0 = symbol_code >> k;
				buff = (buff << bits_to_go) | code0;

				Buff_full(*ppDst, buff);
				*ppDst += 4;
				buff = (symbol_code - (code0<<k));
				bits_to_go = 32 - k;
				assert(k<32);
			}

		}

		pSrc += 2;
	}


	// Clean tail bits in buff
	// Note we need decision *ppDst =? pDstStart
	if(bits_to_go == 32)
	{
		*pDstBitsOffset = 0;
	}
	else
	{
		// Set pDstBitsOffset
		k = (32 - bits_to_go +7)/8;
		buff <<= (bits_to_go);

		for(i=0; i<k-1; i++)
		{
			**ppDst = (buff >> ((3-i)*8)) & 0xFF;
			*ppDst += 1;
		}

		bitsOffset = bits_to_go%8;
		code0 = (**ppDst) & VLC_mask[bitsOffset];

		buff = ((buff >>(4-k)*8) & 0xFF) | code0;
		**ppDst = buff;

		*pDstBitsOffset = 8 - bitsOffset;

		if(bitsOffset ==0)
		{
			*ppDst += 1;
			*pDstBitsOffset = 0;
		}
	}



	return ippStsNoErr;

}
// Above two function may generate difference for VLC part (But it does not matter, just after "stop bits"
IppStatus __STDCALL ippsVLCEncodeInitAlloc_32s_c(const IppsVLCTable_32s* pInputTable, int inputTableSize, IppsVLCEncodeSpec_32s** ppVLCSpec)
{
	int min_value, max_value;
	K_IppsVLCEncodeSpec_32s *pcurrVLCTbl;
	int i,j, k, len, valueOffset;
	unsigned int symbol_code, symbol_len;
	unsigned int *startAddr;
	unsigned char *ptr;

	min_value = pInputTable[0].value; max_value = pInputTable[0].value; 
	for(i=1; i< inputTableSize; i++)
	{
	if(min_value> pInputTable[i].value)
		min_value = pInputTable[i].value;
	if(max_value < pInputTable[i].value)
		max_value = pInputTable[i].value;
	}
	len = max_value - min_value;
	k=0;
	while(len > 0)
	{
		len = (len >> 1);
		k++;
	}
	ptr = (void*)ippsMalloc_8u_c(sizeof(K_IppsVLCEncodeSpec_32s) + (1<<(k+2+1)));
	startAddr = ptr + sizeof(K_IppsVLCEncodeSpec_32s);
	pcurrVLCTbl = (K_IppsVLCEncodeSpec_32s *) (ptr);

	pcurrVLCTbl->alloc = 1;
	pcurrVLCTbl->order = k;
	pcurrVLCTbl->flag = 0x4c4d5378;
	pcurrVLCTbl->minValue = abs(min_value);
	pcurrVLCTbl->startAddr = startAddr;
	pcurrVLCTbl->dummy0 = pcurrVLCTbl->dummy1 = pcurrVLCTbl->dummy2 = unsued_item;

	valueOffset = 1<< k;

	// Set Tbl
	for(i=0; i< valueOffset;i++)
	{
		for(j=0; j< inputTableSize; j++)
		{
			if(pInputTable[j].value == (i+min_value))
				break;

		}
		if(j < inputTableSize) // Found item in Tbl
		{
			symbol_len = pInputTable[j].length;
			symbol_code = pInputTable[j].code;

			(*(startAddr + i)) = symbol_len;
			(*(startAddr + valueOffset + i)) = symbol_code;

		}
		else // Not found
		{
			(*(startAddr + i)) = unsued_item;
			(*(startAddr + valueOffset + i)) = unsued_item;

		}
	}

	* ppVLCSpec = (IppsVLCEncodeSpec_32s*)pcurrVLCTbl;
	return ippStsNoErr;
}

void __STDCALL ippsVLCEncodeFree_32s_c(IppsVLCEncodeSpec_32s* pVLCSpec)
{
	ippsFree_c((void *)(pVLCSpec));

}

IppStatus __STDCALL ippsVLCCountBits_16s32s_c(const Ipp16s* pSrc, int srcLen, Ipp32s* pCountBits, const IppsVLCEncodeSpec_32s* pVLCSpec)
{
	Ipp16s value;
	unsigned int symbol_len;
	int i;
	K_IppsVLCEncodeSpec_32s *pcurrVLCTbl = (K_IppsVLCEncodeSpec_32s *) (pVLCSpec);
	unsigned int real_bits;

	unsigned int *startAddr = pcurrVLCTbl->startAddr;
	int  minValue = pcurrVLCTbl->minValue;


	real_bits = 0;


	for(i=0; i<srcLen; i++)
	{
		value = (*pSrc) + minValue;

		// Find correct code and length for this value
		symbol_len  = (unsigned int)(*(startAddr + value));
		//symbol_code = (unsigned int)(*(startAddr + valueOffset + value));
		
		real_bits += symbol_len;

		pSrc ++;
	}

	*pCountBits = real_bits;

	return ippStsNoErr;
}

IppStatus __STDCALL ippsVLCCountEscBits_AAC_16s32s_c(const Ipp16s *pSrc, int srcLen,  Ipp32s *pCountBits,    const IppsVLCEncodeSpec_32s *pVLCSpec)
{
	Ipp32s value, value1, value2, valuetemp;
	unsigned int symbol_code, symbol_len;
	int i, k;
	K_IppsVLCEncodeSpec_32s *pcurrVLCTbl = (K_IppsVLCEncodeSpec_32s *) (pVLCSpec);

	int	real_bits = 0;

	int  minValue = pcurrVLCTbl->minValue;

	int valueOffset = 1<< pcurrVLCTbl->order;
	unsigned int *startAddr = pcurrVLCTbl->startAddr;

	for(i=0; i<srcLen/2; i++)
	{
		// First ESC flag
		value1 = (*pSrc ==0)?0 :(*pSrc);
		if(value1 >=16) 
			value1 = 16;
		else
			if(value1 <= -16)
				value1 = -16;
		value2 = (*(pSrc+1) ==0)?0 :(*(pSrc+1));
		if(value2 >=16) 
			value2 = 16;
		else
			if(value2 <= -16)
				value2 = -16;
		value = (value1)*64 + value2 + 16;

		value += minValue;

		// Find correct code and length for this value
		symbol_len  = (unsigned int)(*(startAddr + value));
		symbol_code = (unsigned int)(*(startAddr + valueOffset + value));

		real_bits += symbol_len;

		// Second, ESC sequence : spec says the max bits for each one is 22bits, so, 32 bits is enough (We do not need store bits inter)
		value1 = abs(*pSrc);
		if(value1 >= 16)
		{
			// Caculate N
			symbol_code = 0;
			symbol_len = 0;

			if(value1 > 31)
			{
				k=1;
				symbol_code = 1;
				valuetemp = value1>>5;
				while(valuetemp > 1)
				{
					k++; valuetemp >>= 1; symbol_code = (symbol_code<<1) |1;
				}

				symbol_len = k;
			}

			// One ZERo seprator
			symbol_len ++; // one zero
			symbol_code <<= 1;

			//
			//valuetemp = (1<< (symbol_len -1)) * 16;
			//value1 = value1 - valuetemp;
			//symbol_code = (symbol_code << ((symbol_len -1) + 4))| value1;
			symbol_len  += (symbol_len -1) + 4;

			real_bits += symbol_len;

		}

		value2 = abs(*(pSrc+1));
		if(value2 >= 16)
		{
			// Caculate N
			symbol_code = 0;
			symbol_len = 0;
			
			if(value2 > 31)
			{
				k=1;
				symbol_code = 1;
				valuetemp = value2>>5;
				while(valuetemp > 1)
				{
					k++; valuetemp >>= 1; symbol_code = (symbol_code<<1) |1;
				}

				symbol_len = k;
			}

			// One ZERo seprator
			symbol_len ++; // one zero
			symbol_code <<= 1;

			//
			//valuetemp = (1<< (symbol_len -1)) * 16;
			//value1 = value2 - valuetemp;
			//symbol_code = (symbol_code << ((symbol_len -1) + 4))| value1;
			symbol_len  += (symbol_len -1) + 4;

			real_bits += symbol_len;

		}

		pSrc += 2;
	}


	*pCountBits = real_bits;
	return ippStsNoErr;
}
#endif
