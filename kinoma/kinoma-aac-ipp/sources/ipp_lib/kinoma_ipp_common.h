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
#ifndef KINOMA_IPP_COMMON_H
#define KINOMA_IPP_COMMON_H

#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <string.h>

#include <ippdc.h>
#include <ippdefs.h>

//#define    MAX_64U  18446744073709551615		//0xFFFFFFFFFFFFFFFF
//#define    MAX_64S  9223372036854775807		//0x7FFFFFFFFFFFFFFF
//#define    MIN_64S  -9223372036854775808		//0x8000000000000000
//
//#define    MAX_32U  4294967295				//0xFFFFFFFF
//#define    MAX_32S	2147483647				//0x7FFFFFFF
//#define    MIN_32S  -2147483648				//0x80000000
//
//#define    MAX_16U  65535						//0xFFFF
//#define    MAX_16S  32767						//0x7FFF
//#define    MIN_16S  -32768					//0x8000
//
//#define	   CLIP(x, max, min) ((x)>(max)?(max):((-(x))>(-(min))?(min):(x)))

static const unsigned __INT64  MAX_64U = 0xFFFFFFFFFFFFFFFF;		//18446744073709551615;	
static const __INT64    MAX_64S = 0x7FFFFFFFFFFFFFFF;				//9223372036854775807;		
static const __INT64    MIN_64S = 0x8000000000000000;				//-9223372036854775808;	

static const unsigned __INT64  MAX_32U =  0xFFFFFFFF;				//4294967295;		
static const __INT64    MAX_32S	= 0x7FFFFFFF;						//2147483647;		
static const __INT64    MIN_32S = 0xFFFFFFFF80000000;				//-2147483648;		

static const unsigned __INT64  MAX_16U = 0xFFFF;					//65535;		
static const __INT64    MAX_16S = 0x7FFF;							//32767;		
static const __INT64    MIN_16S = 0xFFFFFFFFFFFF8000;				//-32768;		

#define	   CLIP(x, max, min) ((x)>(max)?(max):((x)<(min)?(min):(x)))
#define    OVER(x,max,min) ((x)>0?(max):((x)<0?(min):(x)))

#define	   COMPLEX_SIG	1280136035	//cSML
#define	   REAL_SIG		1280136036	//dSML
#define    VLC_SIG		1280136057	//ySML

#define K_PI     3.14159265358979323846   
#define K_2PI    6.28318530717958647692  /* 2*pi                         */


#define KINOMA_VLC
//#define INTEL_VLC

#define KINOMA_SYNTH
//#define INTEL_SYNTH

#define KINOMA_FFT
//#define INTEL_FFT


/* struct for vlc about functions */
typedef struct
{
	int sig;		//ySML
	IppsVLCTable_32s* pInputTable;
	int inputTableSize;
	Ipp32s* pSubTablesSizes;
	int numSubTables;
	IppsVLCTable_32s* pNewTable;
	int tablePos[32];
}k_IppsVLCDecodeSpec_32s;

/* struct for FFT about functions*/
typedef struct
{
	int sig;	//cSML or dSML
	int order;	
	IppHintAlgorithm hint;
	int sizeWorkBuf;	//16, 32, 64,128,528
	int alloc;		//if initAlloc alloc == 1, if init alloc = 0;
	int len;		// sample number 2^(order-1)

	int flagInv;
	int flagFwd;
	double normInv;
	double normFwd;

	double *sin;		//need sizeof(double)* (len/2))
	double *cos;		//need sizeof(double)* (len/2))

	int	*ReverseTbl;
}k_IppsFFTSpec_C_32sc,k_IppsFFTSpec_C_32s;


/* used in  ippsSortAscend_32s_I */
void quick_sort(int data[], int x, int y);


/* function used in SQRT,   data = root^2 + remainder , root & remainder are integers*/
static void __inline  qk_sqrt(__INT64 data, __INT64 *remainder, __INT64 *root)
{
	__INT64  squaredbit;

	if (data < 1)
	{
		*remainder = 0;
		*root = 0;
		return;
	}

	if (data > 2147483647)	//1 << 31 = 2147483648
	{
		squaredbit  = (__INT64) ((((unsigned __INT64) ~0) >> 1) & 
			~(((unsigned __INT64) ~0) >> 2));
	}
	else 
	{
		squaredbit  = (int) ((((unsigned int) ~0) >> 1) & 
			~(((unsigned int) ~0) >> 2));
	}

	*remainder = data;  
	*root = 0;
	while (squaredbit > 0) {
		if (*remainder >= (squaredbit | *root)) 
		{
			*remainder -= (squaredbit | *root);
			*root >>= 1; 
			*root |= squaredbit;
		} 
		else 
		{
			*root >>= 1;
		}
		squaredbit >>= 2; 
	}
}

/* calc the float part of sqrt root
sqrt(data) = (Iroot+froot)   // Iroot is the integer part of root, froot is the float part
==> data = (Iroot+froot) * (Iroot+froot) = Iroot^2 + 2*Iroot*froot + froot^2
and also data = Iroot^2 + remainder
so, remainder = 2*Iroot*froot + froot^2
then, froot = (remainder - froot^2) / (2*Iroot)
here,we can use iterative method to approach the froot
*/
static long double  __inline float_part(Ipp64s remainder, Ipp64s root)
{
	long double froot,ftmp;
	const long double limit = 0.000000000001;

	Ipp64s Iroot2 = (root) * 2;

	if (root == 0 || remainder == 0)
		return 0.0f;

	froot = remainder / (long double)Iroot2;
	do
	{
		ftmp = froot;
		froot =(remainder - ftmp*ftmp) / (long double)Iroot2;
	}while (fabs(froot - ftmp) > limit);

	return froot;
}


/* function for DIV */
static __inline int isDIVbyZERO(int *isMinus, __INT64 *base, __INT64 *div, const Ipp32s Src, Ipp32s *srcDst, IppStatus *sts)   			
{
	*isMinus = 0;	
	if ((*div = (__INT64)(*srcDst)) < 0)	
	{					
		*div = -(*div);			
		*isMinus = !(*isMinus);		
	}							
	if ((*base = (__INT64)Src) < 0)		
	{							
		*base = -(*base);			
		*isMinus = !(*isMinus);		
	}							
	if (*base == 0)				
	{							
		if (*div ==0)			
		{						
			*srcDst = 0;		
		}						
		else if (*isMinus)
		{
			*srcDst = (Ipp32s)MIN_32S;
		}
		else
		{
			*srcDst = (Ipp32s)MAX_32S;
		}
		*sts = ippStsDivByZero;
		return 1;
	}
	return 0;
}


/* function used in ippsVLCDecodeEscBlock_AAC_1u16s */
static Ipp16s __inline GetAacEsc(Ipp16s data, Ipp8u **ppBitStream, int* pBitOffset)
{
	int k,n,l;
	int sign = data < 0 ? -1: 1;
	unsigned int	bits, currBits; 

	bits = ((*ppBitStream)[0] << 24) | ((*ppBitStream)[1] << 16) | ((*ppBitStream)[2] <<  8) | ((*ppBitStream)[3]);
	bits <<= *pBitOffset;
	n = 0;
	for (k = 1; k < 19; k++)		/* get N'1 */
	{
		currBits = bits >> (32 - k);
		if (currBits & 0x1)
			n++;
		else
		{
			l = n + 1 + *pBitOffset;	
			*ppBitStream += l >> 3; 
			*pBitOffset = l & 7;			
			break;
		}
	}
	bits = ((*ppBitStream)[0] << 24) | ((*ppBitStream)[1] << 16) | ((*ppBitStream)[2] <<  8) | ((*ppBitStream)[3]);
	bits <<= *pBitOffset;
	
	currBits = bits >> (28-n);	// 32 - (n + 4)
	l = n + 4 + *pBitOffset;	
	*ppBitStream += l >> 3; 
	*pBitOffset = l & 7;

	return (sign * (currBits + (1<<(n+4))));
}


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


/*
void DP2QF(double *pSrc, Ipp32s *pDst, int intbits, int fltbits, int len)		//double float pointing number --->  Q format
{
	Ipp32s Mag = 1 << fltbits;
	int i;
	for (i = 0; i < len; i++)
	{
		pDst[i] = (Ipp32s)(pSrc[i] * Mag + 0.5f);
	}

}
*/

/* Q format ---> double float pointing number, used in ippsSynthPQMF_MP3_32s16s*/
static __inline void QF2DP(Ipp32s *pSrc, double *pDst, int intbits, int fltbits, int len)		
{
	Ipp32s Mag = 1 << fltbits;
	int i;
	for (i = 0; i < len; i++)
	{
		pDst[i] = (double)pSrc[i] / Mag;
	}

}

#endif
