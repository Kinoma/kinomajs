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
#include <string.h>

#include <memory.h>

#include "kinoma_ipp_env.h"

#include "ippdc.h"
#include "ipps.h"
#include "ippvc.h"

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

#if TARGET_OS_LINUX || TARGET_OS_MAC
	#define __int64 long long
#endif

static const unsigned __int64  MAX_64U = 0xFFFFFFFFFFFFFFFFULL;		//18446744073709551615;	
static const __int64    MAX_64S = 0x7FFFFFFFFFFFFFFFLL;				//9223372036854775807;		
static const __int64    MIN_64S = 0x8000000000000000LL;				//-9223372036854775808;	

static const unsigned __int64  MAX_32U =  0xFFFFFFFF;				//4294967295;		
static const __int64    MAX_32S	= 0x7FFFFFFF;						//2147483647;		
static const __int64    MIN_32S = 0xFFFFFFFF80000000LL;				//-2147483648;		

static const unsigned __int64  MAX_16U = 0xFFFF;					//65535;		
static const __int64    MAX_16S = 0x7FFF;							//32767;		
static const __int64    MIN_16S = 0xFFFFFFFFFFFF8000LL;				//-32768;		


#define SIGN(x)		((x)>=0? 1:-1)
#define SIGN0(a)    ( ((a)<0) ? -1 : (((a)>0) ? 1  : 0) )
#define CLIP(x)		((x)>255? 255:((x)<0?0:(x)))
#define CLIP_R(x,max,min) ((x)>(max)?(max):((x)<(min)?(min):(x)))
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define OVER(x,max,min) ((x)>0?(max):((x)<0?(min):(x)))



#define KINOMA_VLC
//#define INTEL_VLC

#define	   COMPLEX_SIG	1280136035	//cSML
#define	   REAL_SIG		0x4c4d5364	//dSML
#define    VLC_SIG		1280136057	//ySML

#define K_2PI    6.28318530717958647692  /* 2*pi                         */

/* struct for audio(aac and mp3) vlc decoding about functions */
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

#if (MP4_VLC_INTERNAL)
/****** some tables vlc and vld of mpeg4 *****/
/* intra level max value */
static Ipp32s intra_max_level[2][64] = 
{
	{
		27, 10,  5,  4,  3,  3,  3,  3, 
		2,  2,  1,  1,  1,  1,  1,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
	},

	{
		8,  3,  2,  2,  2,  2,  2,  1, 
		1,  1,  1,  1,  1,  1,  1,  1,
		1,  1,  1,  1,  1,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0
	}
};

/* intra run max value */
static Ipp32s intra_max_run0[28] = 
{
	999, 14,  9,  7,  3,  2,  1,   
	1,  1,  1,  1,  0,  0,  0, 
	0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0
};


static Ipp32s intra_max_run1[9] = 
{ 
	999, 20,  6,  
	1,  0,  0,  
	0,  0,  0
};
#endif

#if (MP4_VLC_INTERNAL || MP4_FLV_INTERNAL)
/* inter level max value */
static Ipp32s inter_max_level[2][64] = 
{
	{
		12,  6,  4,  3,  3,  3,  3,  2, 
		2,  2,  2,  1,  1,  1,  1,  1,
		1,  1,  1,  1,  1,  1,  1,  1,
		1,  1,  1,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0
	},

	{
		3,  2,  1,  1,  1,  1,  1,  1, 
		1,  1,  1,  1,  1,  1,  1,  1,
		1,  1,  1,  1,  1,  1,  1,  1,
		1,  1,  1,  1,  1,  1,  1,  1,
		1,  1,  1,  1,  1,  1,  1,  1,
		1,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0
	}
};

/* inter run max value */
static Ipp32s inter_max_run0[13] = 
{ 
	999, 
	26, 10,  6,  2,  1,  1,   
	0,  0,  0,  0,  0,  0
};

static Ipp32s inter_max_run1[4] = { 999, 40,  1,  0 };


/* scanning  */
/* Normal zigzag */
static const Ipp32s zigzag[4][64] = 
{
	/* no zigzag IPPVC_SCAN_NONE*/
	{0,  1,  2, 3,  4,  5,  6, 7,
	8, 9, 10, 11, 12, 13,  14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26,  27,  28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55,
	56, 57, 58, 59, 60, 61, 62, 63},

	/* normal zigzag  IPPVC_SCAN_ZIGZAG*/
	{0,  1,  8, 16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63},

	/* Vertical zigzag IPPVC_SCAN_VERTICAL*/
	{0,  8, 16, 24,  1,  9,  2, 10,
	17, 25, 32, 40, 48, 56, 57, 49,
	41, 33, 26, 18,  3, 11,  4, 12,
	19, 27, 34, 42, 50, 58, 35, 43,
	51, 59, 20, 28,  5, 13,  6, 14,
	21, 29, 36, 44, 52, 60, 37, 45,
	53, 61, 22, 30,  7, 15, 23, 31,
	38, 46, 54, 62, 39, 47, 55, 63},

	/* Horizontal zigzag IPPVC_SCAN_HORIZONTAL*/
	{0,  1,  2,  3,  8,  9, 16, 17,
	10, 11,  4,  5,  6,  7, 15, 14,
	13, 12, 19, 18, 24, 25, 32, 33,
	26, 27, 20, 21, 22, 23, 28, 29,
	30, 31, 34, 35, 40, 41, 48, 49,
	42, 43, 36, 37, 38, 39, 44, 45,
	46, 47, 50, 51, 56, 57, 58, 59,
	52, 53, 54, 55, 60, 61, 62, 63}

};
#endif

/**************************************************************
* my IppiQuantInvInterSpec_MPEG4 structure					  *
* the size of Intel's is 156 bytes							  *
**************************************************************/
typedef struct
{
	int quant_max;
	int min, max;
	int bUseMat;
	Ipp8u matrix[64];
}k_IppiQuantInvInterSpec_MPEG4, k_IppiQuantInvIntraSpec_MPEG4;

/**************************************************************
* my IppiQuantInvIntraSpec_MPEG4 structure					  *
* the size of Intel's is 156 bytes							  *
**************************************************************/
typedef struct
{
	int bUseMat;
	Ipp8u matrix[64];
}k_IppiQuantInterSpec_MPEG4, k_IppiQuantIntraSpec_MPEG4;

#endif
