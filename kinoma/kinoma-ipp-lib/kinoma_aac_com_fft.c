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
#include "kinoma_ipp_lib.h"
#include "kinoma_ipp_common.h"
#include <assert.h>


#ifdef KINOMA_DEBUG
extern int g_kinoma_debug_on;
#endif


IppStatus (__STDCALL *ippsFFTInitAlloc_C_32sc_universal)(IppsFFTSpec_C_32sc** ppFFTSpec, int order, int flag, IppHintAlgorithm hint)=NULL;
IppStatus (__STDCALL *ippsFFTGetBufSize_C_32sc_universal)(const IppsFFTSpec_C_32sc* pFFTSpec, int* pSize)=NULL;
IppStatus (__STDCALL *ippsFFTFree_C_32sc_universal)		(IppsFFTSpec_C_32sc* pFFTSpec)=NULL;
IppStatus (__STDCALL *ippsFFTFwd_CToC_32sc_Sfs_universal)(const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)=NULL;

IppStatus (__STDCALL *ippsFFTInv_CToC_32s_Sfs_universal)(const Ipp32s* pSrcRe, const Ipp32s* pSrcIm, Ipp32s* pDstRe, Ipp32s* pDstIm, const IppsFFTSpec_C_32s* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)=NULL;
IppStatus (__STDCALL *ippsFFTInit_C_32s_universal)		(IppsFFTSpec_C_32s** ppFFTSpec, int order, int flag,IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit)=NULL;

/* the following 6 functions exist in sbr code */
IppStatus (__STDCALL *ippsFFTGetSize_C_32sc_universal)	(int order, int flag, IppHintAlgorithm hint, int* pSizeSpec, int* pSizeInit, int* pSizeBuf)=NULL;
IppStatus (__STDCALL *ippsFFTInit_C_32sc_universal)		(IppsFFTSpec_C_32sc** ppFFTSpec, int order, int flag, IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit)=NULL;
IppStatus (__STDCALL *ippsFFTInv_CToC_32sc_Sfs_universal)(const Ipp32sc* pSrc, Ipp32sc* pDst, const IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)=NULL;
IppStatus (__STDCALL *ippsFFTGetSize_C_32s_universal)	(int order, int flag, IppHintAlgorithm hint, int* pSizeSpec, int* pSizeInit, int* pSizeBuf)=NULL;


// Fixed COS/SIN table
/*20070714 -- WWD*/
// Order = 9
static const int cfft_table_512_cos[256] =
{
	0x7fffffff,   0x7ffd8859,   0x7ff62181,   0x7fe9cbbe,
	0x7fd8878c,   0x7fc25595,   0x7fa736b3,   0x7f872bf1,
	0x7f62368e,   0x7f3857f4,   0x7f0991c2,   0x7ed5e5c5,
	0x7e9d55fb,   0x7e5fe492,   0x7e1d93e8,   0x7dd6668d,
	0x7d8a5f3e,   0x7d3980eb,   0x7ce3ceb0,   0x7c894bdc,
	0x7c29fbed,   0x7bc5e28e,   0x7b5d039c,   0x7aef6322,
	0x7a7d055a,   0x7a05eeac,   0x798a23b0,   0x7909a92b,
	0x78848412,   0x77fab987,   0x776c4eda,   0x76d94987,
	0x7641af3b,   0x75a585ce,   0x7504d344,   0x745f9dd0,
	0x73b5ebd0,   0x7307c3cf,   0x72552c83,   0x719e2cd1,
	0x70e2cbc5,   0x70231098,   0x6f5f02b0,   0x6e96a99b,
	0x6dca0d13,   0x6cf934fa,   0x6c24295f,   0x6b4af277,
	0x6a6d98a3,   0x698c246b,   0x68a69e80,   0x67bd0fbb,
	0x66cf811f,   0x65ddfbd2,   0x64e88925,   0x63ef328e,
	0x62f201ab,   0x61f1003e,   0x60ec382f,   0x5fe3b38c,
	0x5ed77c88,   0x5dc79d7b,   0x5cb420df,   0x5b9d1152,
	0x5a827999,   0x59646497,   0x5842dd53,   0x571deef8,
	0x55f5a4d1,   0x54ca0a49,   0x539b2aee,   0x5269126d,
	0x5133cc93,   0x4ffb654c,   0x4ebfe8a3,   0x4d8162c3,
	0x4c3fdff2,   0x4afb6c97,   0x49b41532,   0x4869e664,
	0x471cece6,   0x45cd358e,   0x447acd4f,   0x4325c134,
	0x41ce1e64,   0x4073f21c,   0x3f1749b7,   0x3db832a5,
	0x3c56ba6f,   0x3af2eeb6,   0x398cdd31,   0x382493af,
	0x36ba2013,   0x354d9056,   0x33def286,   0x326e54c7,
	0x30fbc54c,   0x2f875261,   0x2e110a61,   0x2c98fbba,
	0x2b1f34eb,   0x29a3c484,   0x2826b927,   0x26a82185,
	0x25280c5d,   0x23a6887e,   0x2223a4c5,   0x209f701c,
	0x1f19f97a,   0x1d934fe5,   0x1c0b826a,   0x1a82a025,
	0x18f8b83c,   0x176dd9de,   0x15e21444,   0x145576b1,
	0x12c8106e,   0x1139f0ce,   0x0fab272b,   0x0e1bc2e3,
	0x0c8bd35d,   0x0afb6805,   0x096a9049,   0x07d95b9e,
	0x0647d97c,   0x04b6195d,   0x03242abe,   0x01921d1f,
	0x00000000,   0xfe6de2e1,   0xfcdbd542,   0xfb49e6a3,
	0xf9b82684,   0xf826a462,   0xf6956fb7,   0xf50497fb,
	0xf3742ca3,   0xf1e43d1d,   0xf054d8d5,   0xeec60f32,
	0xed37ef92,   0xebaa894f,   0xea1debbc,   0xe8922622,
	0xe70747c4,   0xe57d5fdb,   0xe3f47d96,   0xe26cb01b,
	0xe0e60686,   0xdf608fe4,   0xdddc5b3b,   0xdc597782,
	0xdad7f3a3,   0xd957de7b,   0xd7d946d9,   0xd65c3b7c,
	0xd4e0cb15,   0xd3670446,   0xd1eef59f,   0xd078ad9f,
	0xcf043ab4,   0xcd91ab39,   0xcc210d7a,   0xcab26faa,
	0xc945dfed,   0xc7db6c51,   0xc67322cf,   0xc50d114a,
	0xc3a94591,   0xc247cd5b,   0xc0e8b649,   0xbf8c0de4,
	0xbe31e19c,   0xbcda3ecc,   0xbb8532b1,   0xba32ca72,
	0xb8e3131a,   0xb796199c,   0xb64beace,   0xb5049369,
	0xb3c0200e,   0xb27e9d3d,   0xb140175d,   0xb0049ab4,
	0xaecc336d,   0xad96ed93,   0xac64d512,   0xab35f5b7,
	0xaa0a5b2f,   0xa8e21108,   0xa7bd22ad,   0xa69b9b69,
	0xa57d8667,   0xa462eeae,   0xa34bdf21,   0xa2386285,
	0xa1288378,   0xa01c4c74,   0x9f13c7d1,   0x9e0effc2,
	0x9d0dfe55,   0x9c10cd72,   0x9b1776db,   0x9a22042e,
	0x99307ee1,   0x9842f045,   0x97596180,   0x9673db95,
	0x9592675d,   0x94b50d89,   0x93dbd6a1,   0x9306cb06,
	0x9235f2ed,   0x91695665,   0x90a0fd50,   0x8fdcef68,
	0x8f1d343b,   0x8e61d32f,   0x8daad37d,   0x8cf83c31,
	0x8c4a1430,   0x8ba06230,   0x8afb2cbc,   0x8a5a7a32,
	0x89be50c5,   0x8926b679,   0x8893b126,   0x88054679,
	0x877b7bee,   0x86f656d5,   0x8675dc50,   0x85fa1154,
	0x8582faa6,   0x85109cde,   0x84a2fc64,   0x843a1d72,
	0x83d60413,   0x8376b424,   0x831c3150,   0x82c67f15,
	0x8275a0c2,   0x82299973,   0x81e26c18,   0x81a01b6e,
	0x8162aa05,   0x812a1a3b,   0x80f66e3e,   0x80c7a80c,
	0x809dc972,   0x8078d40f,   0x8058c94d,   0x803daa6b,
	0x80277874,   0x80163442,   0x8009de7f,   0x800277a7
};

static const int cfft_table_512_sin[256] = 
{
	0x00000000,   0xfe6de2e1,   0xfcdbd542,   0xfb49e6a3,
	0xf9b82684,   0xf826a462,   0xf6956fb7,   0xf50497fb,
	0xf3742ca3,   0xf1e43d1d,   0xf054d8d5,   0xeec60f32,
	0xed37ef92,   0xebaa894f,   0xea1debbc,   0xe8922622,
	0xe70747c4,   0xe57d5fdb,   0xe3f47d96,   0xe26cb01b,
	0xe0e60686,   0xdf608fe4,   0xdddc5b3b,   0xdc597782,
	0xdad7f3a3,   0xd957de7b,   0xd7d946d9,   0xd65c3b7c,
	0xd4e0cb15,   0xd3670446,   0xd1eef59f,   0xd078ad9f,
	0xcf043ab4,   0xcd91ab39,   0xcc210d7a,   0xcab26faa,
	0xc945dfed,   0xc7db6c51,   0xc67322cf,   0xc50d114a,
	0xc3a94591,   0xc247cd5b,   0xc0e8b649,   0xbf8c0de4,
	0xbe31e19c,   0xbcda3ecc,   0xbb8532b1,   0xba32ca72,
	0xb8e3131a,   0xb796199c,   0xb64beace,   0xb5049369,
	0xb3c0200e,   0xb27e9d3d,   0xb140175d,   0xb0049ab4,
	0xaecc336d,   0xad96ed93,   0xac64d512,   0xab35f5b7,
	0xaa0a5b2f,   0xa8e21108,   0xa7bd22ad,   0xa69b9b69,
	0xa57d8667,   0xa462eeae,   0xa34bdf21,   0xa2386285,
	0xa1288378,   0xa01c4c74,   0x9f13c7d1,   0x9e0effc2,
	0x9d0dfe55,   0x9c10cd72,   0x9b1776db,   0x9a22042e,
	0x99307ee1,   0x9842f045,   0x97596180,   0x9673db95,
	0x9592675d,   0x94b50d89,   0x93dbd6a1,   0x9306cb06,
	0x9235f2ed,   0x91695665,   0x90a0fd50,   0x8fdcef68,
	0x8f1d343b,   0x8e61d32f,   0x8daad37d,   0x8cf83c31,
	0x8c4a1430,   0x8ba06230,   0x8afb2cbc,   0x8a5a7a32,
	0x89be50c5,   0x8926b679,   0x8893b126,   0x88054679,
	0x877b7bee,   0x86f656d5,   0x8675dc50,   0x85fa1154,
	0x8582faa6,   0x85109cde,   0x84a2fc64,   0x843a1d72,
	0x83d60413,   0x8376b424,   0x831c3150,   0x82c67f15,
	0x8275a0c2,   0x82299973,   0x81e26c18,   0x81a01b6e,
	0x8162aa05,   0x812a1a3b,   0x80f66e3e,   0x80c7a80c,
	0x809dc972,   0x8078d40f,   0x8058c94d,   0x803daa6b,
	0x80277874,   0x80163442,   0x8009de7f,   0x800277a7,
	0x80000001,   0x800277a7,   0x8009de7f,   0x80163442,
	0x80277874,   0x803daa6b,   0x8058c94d,   0x8078d40f,
	0x809dc972,   0x80c7a80c,   0x80f66e3e,   0x812a1a3b,
	0x8162aa05,   0x81a01b6e,   0x81e26c18,   0x82299973,
	0x8275a0c2,   0x82c67f15,   0x831c3150,   0x8376b424,
	0x83d60413,   0x843a1d72,   0x84a2fc64,   0x85109cde,
	0x8582faa6,   0x85fa1154,   0x8675dc50,   0x86f656d5,
	0x877b7bee,   0x88054679,   0x8893b126,   0x8926b679,
	0x89be50c5,   0x8a5a7a32,   0x8afb2cbc,   0x8ba06230,
	0x8c4a1430,   0x8cf83c31,   0x8daad37d,   0x8e61d32f,
	0x8f1d343b,   0x8fdcef68,   0x90a0fd50,   0x91695665,
	0x9235f2ed,   0x9306cb06,   0x93dbd6a1,   0x94b50d89,
	0x9592675d,   0x9673db95,   0x97596180,   0x9842f045,
	0x99307ee1,   0x9a22042e,   0x9b1776db,   0x9c10cd72,
	0x9d0dfe55,   0x9e0effc2,   0x9f13c7d1,   0xa01c4c74,
	0xa1288378,   0xa2386285,   0xa34bdf21,   0xa462eeae,
	0xa57d8667,   0xa69b9b69,   0xa7bd22ad,   0xa8e21108,
	0xaa0a5b2f,   0xab35f5b7,   0xac64d512,   0xad96ed93,
	0xaecc336d,   0xb0049ab4,   0xb140175d,   0xb27e9d3d,
	0xb3c0200e,   0xb5049369,   0xb64beace,   0xb796199c,
	0xb8e3131a,   0xba32ca72,   0xbb8532b1,   0xbcda3ecc,
	0xbe31e19c,   0xbf8c0de4,   0xc0e8b649,   0xc247cd5b,
	0xc3a94591,   0xc50d114a,   0xc67322cf,   0xc7db6c51,
	0xc945dfed,   0xcab26faa,   0xcc210d7a,   0xcd91ab39,
	0xcf043ab4,   0xd078ad9f,   0xd1eef59f,   0xd3670446,
	0xd4e0cb15,   0xd65c3b7c,   0xd7d946d9,   0xd957de7b,
	0xdad7f3a3,   0xdc597782,   0xdddc5b3b,   0xdf608fe4,
	0xe0e60686,   0xe26cb01b,   0xe3f47d96,   0xe57d5fdb,
	0xe70747c4,   0xe8922622,   0xea1debbc,   0xebaa894f,
	0xed37ef92,   0xeec60f32,   0xf054d8d5,   0xf1e43d1d,
	0xf3742ca3,   0xf50497fb,   0xf6956fb7,   0xf826a462,
	0xf9b82684,   0xfb49e6a3,   0xfcdbd542,   0xfe6de2e1
};

// Order = 7
static const int cfft_table_128_cos[64] = 
{
	0x7fffffff,   0x7fd8878c,   0x7f62368e,   0x7e9d55fb,
	0x7d8a5f3e,   0x7c29fbed,   0x7a7d055a,   0x78848412,
	0x7641af3b,   0x73b5ebd0,   0x70e2cbc5,   0x6dca0d13,
	0x6a6d98a3,   0x66cf811f,   0x62f201ab,   0x5ed77c88,
	0x5a827999,   0x55f5a4d1,   0x5133cc93,   0x4c3fdff2,
	0x471cece6,   0x41ce1e64,   0x3c56ba6f,   0x36ba2013,
	0x30fbc54c,   0x2b1f34eb,   0x25280c5d,   0x1f19f97a,
	0x18f8b83c,   0x12c8106e,   0x0c8bd35d,   0x0647d97c,
	0x00000000,   0xf9b82684,   0xf3742ca3,   0xed37ef92,
	0xe70747c4,   0xe0e60686,   0xdad7f3a3,   0xd4e0cb15,
	0xcf043ab4,   0xc945dfed,   0xc3a94591,   0xbe31e19c,
	0xb8e3131a,   0xb3c0200e,   0xaecc336d,   0xaa0a5b2f,
	0xa57d8667,   0xa1288378,   0x9d0dfe55,   0x99307ee1,
	0x9592675d,   0x9235f2ed,   0x8f1d343b,   0x8c4a1430,
	0x89be50c5,   0x877b7bee,   0x8582faa6,   0x83d60413,
	0x8275a0c2,   0x8162aa05,   0x809dc972,   0x80277874
};

static const int cfft_table_128_sin[64] = 
{
	0x00000000,   0xf9b82684,   0xf3742ca3,   0xed37ef92,
	0xe70747c4,   0xe0e60686,   0xdad7f3a3,   0xd4e0cb15,
	0xcf043ab4,   0xc945dfed,   0xc3a94591,   0xbe31e19c,
	0xb8e3131a,   0xb3c0200e,   0xaecc336d,   0xaa0a5b2f,
	0xa57d8667,   0xa1288378,   0x9d0dfe55,   0x99307ee1,
	0x9592675d,   0x9235f2ed,   0x8f1d343b,   0x8c4a1430,
	0x89be50c5,   0x877b7bee,   0x8582faa6,   0x83d60413,
	0x8275a0c2,   0x8162aa05,   0x809dc972,   0x80277874,
	0x80000001,   0x80277874,   0x809dc972,   0x8162aa05,
	0x8275a0c2,   0x83d60413,   0x8582faa6,   0x877b7bee,
	0x89be50c5,   0x8c4a1430,   0x8f1d343b,   0x9235f2ed,
	0x9592675d,   0x99307ee1,   0x9d0dfe55,   0xa1288378,
	0xa57d8667,   0xaa0a5b2f,   0xaecc336d,   0xb3c0200e,
	0xb8e3131a,   0xbe31e19c,   0xc3a94591,   0xc945dfed,
	0xcf043ab4,   0xd4e0cb15,   0xdad7f3a3,   0xe0e60686,
	0xe70747c4,   0xed37ef92,   0xf3742ca3,   0xf9b82684
};

// Order = 6
static const int cfft_table_64_cos[32] =
{
	0x7fffffff,   0x7f62368e,   0x7d8a5f3e,   0x7a7d055a,
	0x7641af3b,   0x70e2cbc5,   0x6a6d98a3,   0x62f201ab,
	0x5a827999,   0x5133cc93,   0x471cece6,   0x3c56ba6f,
	0x30fbc54c,   0x25280c5d,   0x18f8b83c,   0x0c8bd35d,
	0x00000000,   0xf3742ca3,   0xe70747c4,   0xdad7f3a3,
	0xcf043ab4,   0xc3a94591,   0xb8e3131a,   0xaecc336d,
	0xa57d8667,   0x9d0dfe55,   0x9592675d,   0x8f1d343b,
	0x89be50c5,   0x8582faa6,   0x8275a0c2,   0x809dc972
};

static const int cfft_table_64_sin[32] =
{
	0x00000000,   0xf3742ca3,   0xe70747c4,   0xdad7f3a3,
	0xcf043ab4,   0xc3a94591,   0xb8e3131a,   0xaecc336d,
	0xa57d8667,   0x9d0dfe55,   0x9592675d,   0x8f1d343b,
	0x89be50c5,   0x8582faa6,   0x8275a0c2,   0x809dc972,
	0x80000001,   0x809dc972,   0x8275a0c2,   0x8582faa6,
	0x89be50c5,   0x8f1d343b,   0x9592675d,   0x9d0dfe55,
	0xa57d8667,   0xaecc336d,   0xb8e3131a,   0xc3a94591,
	0xcf043ab4,   0xdad7f3a3,   0xe70747c4,   0xf3742ca3
};

// Order = 5
static const int cfft_table_32_cos[16] =
{
	0x7fffffff,   0x7d8a5f3e,   0x7641af3b,   0x6a6d98a3,
	0x5a827999,   0x471cece6,   0x30fbc54c,   0x18f8b83c,
	0x00000000,   0xe70747c4,   0xcf043ab4,   0xb8e3131a,
	0xa57d8667,   0x9592675d,   0x89be50c5,   0x8275a0c2
};

static const int cfft_table_32_sin[16] =
{
	0x00000000,   0xe70747c4,   0xcf043ab4,   0xb8e3131a,
	0xa57d8667,   0x9592675d,   0x89be50c5,   0x8275a0c2,
	0x80000001,   0x8275a0c2,   0x89be50c5,   0x9592675d,
	0xa57d8667,   0xb8e3131a,   0xcf043ab4,   0xe70747c4
};

#define KINOMA_FFT
//#define INTEL_FFT

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
	int normInv;
	int normFwd;

	int  *sin1, *cos1;

	//double *sin;		//need sizeof(double)* (len/2))
	//double *cos;		//need sizeof(double)* (len/2))

	int	*ReverseTbl;
}k_IppsFFTSpec_C_32sc,k_IppsFFTSpec_C_32s;


/* the following 4 functions used in MDCT*/
IppStatus __STDCALL ippsFFTInitAlloc_C_32sc_c(IppsFFTSpec_C_32sc** ppFFTSpec, int order, int flag, IppHintAlgorithm hint)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32sc *pkSpec;
	int i;//, halfsize;	
	//double theta;
	
	if (!ppFFTSpec)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;
	
	pkSpec = (k_IppsFFTSpec_C_32sc*)ippsMalloc_8u_c(sizeof(k_IppsFFTSpec_C_32sc));
	if (!pkSpec)
		return ippStsMemAllocErr;

	pkSpec->sig = REAL_SIG;							/*means complex fft*/
	pkSpec->order = order;
	pkSpec->hint = hint;
	pkSpec->sizeWorkBuf = (1<<(order+2)) * 8 + 16;		/*work buf size which alloc memory in MDCT*/
	pkSpec->alloc = 1;									/*means this is InitAlloc func, not Init*/
	pkSpec->len = 1 << order;							/* 2^order will be the input sample length */

	if (flag == IPP_FFT_NODIV_BY_ANY)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 0;
		pkSpec->normFwd = pkSpec->normInv = 0;
	}
#if 0
	else if (flag == IPP_FFT_DIV_BY_SQRTN)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 1;
		pkSpec->normFwd = pkSpec->normInv = sqrt(pkSpec->len);	
	}
#endif
	else if (flag == IPP_FFT_DIV_INV_BY_N)
	{
		pkSpec->flagFwd = 0;
		pkSpec->normFwd = 0;
		pkSpec->flagInv = 1;
		pkSpec->normInv = pkSpec->len;
	}
	else
	{
		pkSpec->flagFwd = 1;
		pkSpec->normFwd = pkSpec->len;
		pkSpec->flagInv = 0;
		pkSpec->normInv = 0;
	}

	if(order == 9)
	{
		pkSpec->cos1 = (int *)&cfft_table_512_cos[0];
		pkSpec->sin1 = (int *)&cfft_table_512_sin[0];
	}
	else
	{
		pkSpec->cos1 = (int *)&cfft_table_64_cos[0];
		pkSpec->sin1 = (int *)&cfft_table_64_sin[0];
	}

#if 0
	/*sin and cos table*/
	halfsize = pkSpec->len >> 1;
	pkSpec->cos = (double*)ippsMalloc_8u_c(sizeof(double)*halfsize);
	pkSpec->sin = (double*)ippsMalloc_8u_c(sizeof(double)*halfsize);

	//pkSpec->cos1 = (int*)ippsMalloc_8u_c(sizeof(int)*halfsize);
	//pkSpec->sin1 = (int*)ippsMalloc_8u_c(sizeof(int)*halfsize);

	for(i = 0; i < halfsize; i++)
	{
		theta = K_2PI * i / (double)pkSpec->len;
		pkSpec->cos[i] = cos(theta);
		pkSpec->sin[i] = -sin(theta);

		//pkSpec->cos1[i] = cos(theta)* (2147483647);
		//pkSpec->sin1[i] = -sin(theta)*(2147483647);
	}

	{
		FILE * fp; int j;
		fp = fopen("wwd.txt", "wb");
		for(i = 0; i < halfsize; i+=4)
		{
			fprintf(fp, "0x%08x,   0x%08x,   0x%08x,   0x%08x\n", pkSpec->cos1[i],pkSpec->cos1[i+1],pkSpec->cos1[i+2],pkSpec->cos1[i+3]);

		}
		fprintf(fp, "\n\n");
		for(i = 0; i < halfsize; i+=4)
		{
			fprintf(fp, "0x%08x,   0x%08x,   0x%08x,   0x%08x\n", pkSpec->sin1[i],pkSpec->sin1[i+1],pkSpec->sin1[i+2],pkSpec->sin1[i+3]);

		}
		fflush(fp);
		fclose(fp);
	}
#endif
	/*bit reverse table*/
	//pkSpec->sizeReverseTbl = sizeof(int)*pkSpec->len;
	pkSpec->ReverseTbl = (int*)ippsMalloc_8u_c(sizeof(int)*pkSpec->len);
	for (i = 0; i < pkSpec->len; i++)
	{
		int reversed = 0;
		int b0;
		int tmp = i;

		for (b0 = 0; b0 < pkSpec->order; b0++)
		{
			reversed = (reversed << 1) | (tmp & 1);
			tmp >>= 1;
		}
		pkSpec->ReverseTbl[i] = reversed;
	}	

	*ppFFTSpec = (IppsFFTSpec_C_32sc*)pkSpec;

#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTInitAlloc_C_32sc_c",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTInitAlloc_C_32sc(ppFFTSpec,  order, flag,  hint);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTGetBufSize_C_32sc_c(const IppsFFTSpec_C_32sc* pFFTSpec, int* pSize)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32sc *pkSpec = (k_IppsFFTSpec_C_32sc*)pFFTSpec;

	if (!pFFTSpec)
		return ippStsNullPtrErr;

	if (pkSpec->sig != REAL_SIG)
	{
		*pSize = 0;
		return ippStsContextMatchErr;	
	}

	*pSize = ((k_IppsFFTSpec_C_32sc*)pFFTSpec)->sizeWorkBuf + 16;
#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTGetBufSize_C_32sc_c",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTGetBufSize_C_32sc(pFFTSpec, pSize);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippsFFTFree_C_32sc_c(IppsFFTSpec_C_32sc* pFFTSpec)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32sc *pkSpec = (k_IppsFFTSpec_C_32sc*)pFFTSpec;
	
	if (!pFFTSpec)
		return ippStsNullPtrErr;

	if (pkSpec->sig != REAL_SIG)		
		return ippStsContextMatchErr;	

	if (pkSpec->alloc)
	{
		//ippsFree_c(pkSpec->sin);
		//ippsFree_c(pkSpec->cos);
		ippsFree_c(pkSpec->ReverseTbl);
		ippsFree_c(pkSpec);
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTFree_C_32sc_c",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTFree_C_32sc(pFFTSpec);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTFwd_CToC_32sc_Sfs_c(const Ipp32sc* pSrc, Ipp32sc* pDst, const
											 IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32sc * pkSpec = (k_IppsFFTSpec_C_32sc*)pFFTSpec;

	int i,j;

	int step, shift, pos;
	int size, exp, estep;

#ifdef USE_DOUBLE
	double rounding;
	long double *xr, *xi, tmp;
#else
	Ipp64s rounding;

	Ipp64s *xr, *xi, tmp;
#endif

	if (!pkSpec || !pSrc || !pDst || !pBuffer)
	{
#ifdef KINOMA_DEBUG
		if( g_kinoma_debug_on)
			fprintf( stderr, "   kinoma debug: 1:ippsFFTFwd_CToC_32sc_Sfs_c:: bad input\n");
#endif		
		return ippStsNullPtrErr;
	}

	if (pkSpec->sig != REAL_SIG)
	{
#ifdef KINOMA_DEBUG
		if( g_kinoma_debug_on)
			fprintf( stderr, "   kinoma debug: 2:ippsFFTFwd_CToC_32sc_Sfs_c:: bad input\n");  
#endif		
		//return ippStsContextMatchErr;	
	}

#ifdef USE_DOUBLE
	xr = (long double*)pBuffer;
	xi = (long double*)(pBuffer + pkSpec->len * sizeof(long double));
#else
	xr = (Ipp64s*)pBuffer;
	xi = (Ipp64s*)(pBuffer + pkSpec->len * sizeof(Ipp64s));

#endif

#ifdef KINOMA_DEBUG
	if( g_kinoma_debug_on)
	{
		int *p1 = pSrc;
		fprintf( stderr, "   kinoma debug: o10:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
			p1[0],p1[1],p1[2],p1[3],p1[4],p1[5],p1[6],p1[8],p1[12],p1[15],p1[19],p1[28] );
	}
#endif

	for (i = 0; i < pkSpec->len; i++)
	{
		xr[i] = pSrc[i].re ;
		xi[i] = pSrc[i].im ;
	}

	/*bit reverse*/
	for (i = 0; i < pkSpec->len; i++)
	{
		j = pkSpec->ReverseTbl[i];
		if (j <= i)
			continue;

		tmp = xr[i];
		xr[i] = xr[j];
		xr[j] = tmp;

		tmp = xi[i];
		xi[i] = xi[j];
		xi[j] = tmp;	
	}
	/*reverse end*/

	/*fwd transform*/
	estep = size = pkSpec->len;
	for (step = 1; step < size; step *= 2)
	{
		int x1;
		int x2 = 0;
		estep >>= 1;
		for (pos = 0; pos < size; pos += (2 * step))
		{
			x1 = x2;
			x2 += step;
			exp = 0;
			for (shift = 0; shift < step; shift++)
			{
#ifdef USE_DOUBLE
				long double v2r, v2i;

				v2r = xr[x2] * pkSpec->cos[exp] - xi[x2] * pkSpec->sin[exp];
				v2i = xr[x2] * pkSpec->sin[exp] + xi[x2] * pkSpec->cos[exp];
#else
				Ipp64s v2r, v2i;
				v2r = (xr[x2] * pkSpec->cos1[exp] - xi[x2] * pkSpec->sin1[exp]) >> 31;
				v2i = (xr[x2] * pkSpec->sin1[exp] + xi[x2] * pkSpec->cos1[exp]) >> 31;
#endif
				xr[x2] = xr[x1] - v2r;
				xr[x1] += v2r;

				xi[x2] = xi[x1] - v2i;

				xi[x1] += v2i;

				exp += estep;

				x1++;
				x2++;
			}
		}
	}
	/*transform end*/

	/*normlizing & scaling*/
	if (scaleFactor)
		rounding = (Ipp64s)(1<< (scaleFactor -1 ));
	else
		rounding = 0;

	if (pkSpec->flagFwd)
	{
		for (i = 0; i < pkSpec->len; i++)
		{
			if (xr[i] < 0)
				pDst[i].re = (Ipp32s)(((__int64)((xr[i]/pkSpec->normFwd) - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp32s)(((__int64)((xr[i]/pkSpec->normFwd) + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp32s)(((__int64)((xi[i]/pkSpec->normFwd) - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp32s)(((__int64)((xi[i]/pkSpec->normFwd) + rounding)) >> scaleFactor);
		}
	}
	else
	{
		for (i = 0; i < pkSpec->len; i++)
		{
			if (xr[i] < 0)
				pDst[i].re = (Ipp32s)(((Ipp64s)(xr[i] - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp32s)(((Ipp64s)(xr[i] + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp32s)(((Ipp64s)(xi[i] - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp32s)(((Ipp64s)(xi[i] + rounding)) >> scaleFactor);
		}
	}
	
#ifdef KINOMA_DEBUG
		if( g_kinoma_debug_on)
		{
			Ipp32sc *p1 = pDst;
			fprintf( stderr, "   kinoma debug: o12:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
				p1[0].re,p1[1].re,p1[2].re,p1[3].re,p1[4].re,p1[5].re,p1[8].re,p1[10].re,p1[12].re,p1[17].re,p1[20].re,p1[25].re );
			fprintf( stderr, "   kinoma debug: o13:%8d,%8d,%8d,%8d,%8d,%8d...\n",  
				p1[0].im,p1[1].im,p1[2].im,p1[3].im,p1[4].im,p1[5].im,p1[8].im,p1[10].im,p1[12].im,p1[17].im,p1[20].im,p1[25].im );
		}
#endif

#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTFwd_CToC_32sc_Sfs",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTFwd_CToC_32sc_Sfs(pSrc, pDst,pFFTSpec, scaleFactor, pBuffer);
		return sts;
	}
#endif

	return ippStsNoErr;
}


/* the following 6 functions exist in sbr code */
IppStatus __STDCALL ippsFFTGetSize_C_32sc_c(int order, int flag, IppHintAlgorithm hint, int* pSizeSpec, int* pSizeInit, int* pSizeBuf)
{
#ifdef KINOMA_FFT

	int len;

	if (!pSizeSpec || !pSizeInit || !pSizeBuf)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;

	*pSizeInit = 0;
	*pSizeBuf = (1<<(order+2)) * 8 + 32;		
	len = 1 << order;

	// Ensure it is safe
	*pSizeSpec = sizeof(k_IppsFFTSpec_C_32sc) + sizeof(double)*len + sizeof(int)*len;
	
#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTGetSize_C_32sc",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTGetSize_C_32sc(order, flag, hint,pSizeSpec, pSizeInit,pSizeBuf);
		return sts;
	}
#endif

	return ippStsNoErr;
}

/****************************************************************************************
 * Please note: Order in this function must be 5,6,7,9. Otherwise we through assertion
 *
 ****************************************************************************************
 */
IppStatus __STDCALL ippsFFTInit_C_32sc_c(IppsFFTSpec_C_32sc** ppFFTSpec, int order, int flag, IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32sc *pkSpec;
	int i;//, halfsize;	
	//double theta;

	if (!ppFFTSpec || !pMemSpec)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;

	pkSpec = (k_IppsFFTSpec_C_32sc*) pMemSpec;

	pkSpec->sig = COMPLEX_SIG;							/*means complex fft*/
	pkSpec->order = order;
	pkSpec->hint = hint;
	pkSpec->sizeWorkBuf = (1<<(order+2)) * 8 + 32;		/*work buf size which alloc memory in MDCT*/
	pkSpec->alloc = 0;									/* 0 means this is Init alloc memory*/
	pkSpec->len = 1 << order;							/* 2^order will be the input sample length */

	if (flag == IPP_FFT_NODIV_BY_ANY)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 0;
		pkSpec->normFwd = pkSpec->normInv = 0;
	}
#if 0
	else if (flag == IPP_FFT_DIV_BY_SQRTN)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 1;
		pkSpec->normFwd = pkSpec->normInv = sqrt(pkSpec->len);	
	}
#endif
	else if (flag == IPP_FFT_DIV_INV_BY_N)
	{
		pkSpec->flagFwd = 0;
		pkSpec->normFwd = 0;
		pkSpec->flagInv = 1;
		pkSpec->normInv = pkSpec->len;
	}
	else
	{
		pkSpec->flagFwd = 1;
		pkSpec->normFwd = pkSpec->len;
		pkSpec->flagInv = 0;
		pkSpec->normInv = 0;
	}

	/*sin and cos table*/
	switch(order)
	{
		case 5 :
			pkSpec->cos1 = (int *)&cfft_table_32_cos[0];
			pkSpec->sin1 = (int *)&cfft_table_32_sin[0];
			break;
		case 6 :
			pkSpec->cos1 = (int *)&cfft_table_64_cos[0];
			pkSpec->sin1 = (int *)&cfft_table_64_sin[0];
			break;
		case 7 :
			pkSpec->cos1 = (int *)&cfft_table_128_cos[0];
			pkSpec->sin1 = (int *)&cfft_table_128_sin[0];
			break;
		case 9 :
			pkSpec->cos1 = (int *)&cfft_table_512_cos[0];
			pkSpec->sin1 = (int *)&cfft_table_512_sin[0];
			break;
		default:
			assert(0);
	}
#if 0
	halfsize = pkSpec->len >> 1;
	pkSpec->cos = (double*)(pMemSpec + sizeof(k_IppsFFTSpec_C_32sc)); 
	pkSpec->sin = (double*)(pMemSpec + sizeof(k_IppsFFTSpec_C_32sc) + sizeof(double)*halfsize);
	for(i = 0; i < halfsize; i++)
	{
		theta = K_2PI * i / (double)pkSpec->len;
		pkSpec->cos[i] = cos(theta);
		pkSpec->sin[i] = -sin(theta);
	}
#endif
	/*bit reverse table*/
	pkSpec->ReverseTbl = (int*)(pMemSpec + sizeof(k_IppsFFTSpec_C_32sc)+ sizeof(double)*pkSpec->len);
	for (i = 0; i < pkSpec->len; i++)
	{
		int reversed = 0;
		int b0;
		int tmp = i;

		for (b0 = 0; b0 < pkSpec->order; b0++)
		{
			reversed = (reversed << 1) | (tmp & 1);
			tmp >>= 1;
		}
		pkSpec->ReverseTbl[i] = reversed;
	}	

	*ppFFTSpec = (IppsFFTSpec_C_32sc*) pkSpec;
#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTInit_C_32sc",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTInit_C_32sc(ppFFTSpec, order, flag, hint, pMemSpec, pBufInit);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTInv_CToC_32sc_Sfs_c(const Ipp32sc* pSrc, Ipp32sc* pDst, const
											IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32sc * pkSpec = (k_IppsFFTSpec_C_32sc*)pFFTSpec;

	int i,j;

	int step, shift, pos;
	int size, exp, estep;


	Ipp64s rounding;

	Ipp64s *xr, *xi, tmp;


	if (!pkSpec || !pSrc || !pDst || !pBuffer)
		return ippStsNullPtrErr;

	if (pkSpec->sig != COMPLEX_SIG)		
		return ippStsContextMatchErr;	

	xr = (Ipp64s*)pBuffer;
	xi = (Ipp64s*)(pBuffer + pkSpec->len * sizeof(Ipp64s));


	for (i = 0; i < pkSpec->len; i++)
	{
		xr[i] = pSrc[i].re;
		xi[i] = -pSrc[i].im;
	}

	/*bit reverse*/
	for (i = 0; i < pkSpec->len; i++)
	{
		j = pkSpec->ReverseTbl[i];
		if (j <= i)
			continue;

		tmp = xr[i];
		xr[i] = xr[j];
		xr[j] = tmp;

		tmp = xi[i];
		xi[i] = xi[j];
		xi[j] = tmp;	
	}
	/*reverse end*/

	/*Inv transform*/
	estep = size = pkSpec->len;
	for (step = 1; step < size; step *= 2)
	{
		int x1;
		int x2 = 0;
		estep >>= 1;
		for (pos = 0; pos < size; pos += (2 * step))
		{
			x1 = x2;
			x2 += step;
			exp = 0;
			for (shift = 0; shift < step; shift++)
			{
				Ipp64s v2r, v2i;
				v2r = (xr[x2] * pkSpec->cos1[exp] - xi[x2] * pkSpec->sin1[exp]) >> 31;
				v2i = (xr[x2] * pkSpec->sin1[exp] + xi[x2] * pkSpec->cos1[exp]) >> 31;


				xr[x2] = xr[x1] - v2r;
				xr[x1] += v2r;

				xi[x2] = xi[x1] - v2i;

				xi[x1] += v2i;

				exp += estep;

				x1++;
				x2++;
			}
		}
	}
	/*transform end*/

	/*normlizing & scaling*/
	for (i = 0; i < pkSpec->len; i++)
	{
		xi[i] = -xi[i];
	}
	if (scaleFactor)
		rounding = (Ipp64s)(1<< (scaleFactor -1 ));
	else
		rounding = 0;
	if (pkSpec->flagInv)
	{
		for (i = 0; i < pkSpec->len; i++)
		{
			if (xr[i] < 0)
				pDst[i].re = (Ipp32s)(((Ipp64s)((xr[i]/pkSpec->normInv) - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp32s)(((Ipp64s)((xr[i]/pkSpec->normInv) + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp32s)(((Ipp64s)((xi[i]/pkSpec->normInv) - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp32s)(((Ipp64s)((xi[i]/pkSpec->normInv) + rounding)) >> scaleFactor);
		}
	}
	else
	{
		for (i = 0; i < pkSpec->len; i++)
		{
			if (xr[i] < 0)
				pDst[i].re = (Ipp32s)(((Ipp64s)(xr[i] - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp32s)(((Ipp64s)(xr[i] + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp32s)(((Ipp64s)(xi[i] - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp32s)(((Ipp64s)(xi[i] + rounding)) >> scaleFactor);
		}
	}
	
#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTInv_CToC_32sc_Sfs",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTInv_CToC_32sc_Sfs(pSrc,pDst, pFFTSpec, scaleFactor, pBuffer);
		return sts;
	}
#endif

	return ippStsNoErr;
}



IppStatus __STDCALL ippsFFTGetSize_C_32s_c(int order, int flag, IppHintAlgorithm hint, int*
										 pSizeSpec, int* pSizeInit, int* pSizeBuf)
{
#ifdef KINOMA_FFT
	int len;

	if (!pSizeSpec || !pSizeInit || !pSizeBuf)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;

	*pSizeInit = 0;
	*pSizeBuf = (1<<(order+2)) * 8 + 32;		
	len = 1 << order;
	*pSizeSpec = sizeof(k_IppsFFTSpec_C_32s) + sizeof(double)*len + sizeof(int)*len;

#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTGetSize_C_32s",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTGetSize_C_32s(order, flag, hint, pSizeSpec, pSizeInit, pSizeBuf);
		return sts;
	}
#endif

	return ippStsNoErr;
}

#if 1
// The following two functions are not used at all?
IppStatus __STDCALL ippsFFTInit_C_32s_c(IppsFFTSpec_C_32s** ppFFTSpec, int order, int flag, IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32s *pkSpec;
	int i; 

	if (!ppFFTSpec || !pMemSpec)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;

	pkSpec = (k_IppsFFTSpec_C_32s*) pMemSpec;

	pkSpec->sig = REAL_SIG;							/*means complex fft*/
	pkSpec->order = order;
	pkSpec->hint = hint;
	pkSpec->sizeWorkBuf = (1<<(order+2)) * 8 + 32;		/*work buf size which alloc memory in MDCT*/
	pkSpec->alloc = 0;									/* 0 means this is Init alloc memory*/
	pkSpec->len = 1 << order;							/* 2^order will be the input sample length */

	if (flag == IPP_FFT_NODIV_BY_ANY)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 0;
		pkSpec->normFwd = pkSpec->normInv = 0;
	}
#if 0
	else if (flag == IPP_FFT_DIV_BY_SQRTN)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 1;
		pkSpec->normFwd = pkSpec->normInv = sqrt(pkSpec->len);	
	}
#endif
	else if (flag == IPP_FFT_DIV_INV_BY_N)
	{
		pkSpec->flagFwd = 0;
		pkSpec->normFwd = 0;
		pkSpec->flagInv = 1;
		pkSpec->normInv = pkSpec->len;
	}
	else
	{
		pkSpec->flagFwd = 1;
		pkSpec->normFwd = pkSpec->len;
		pkSpec->flagInv = 0;
		pkSpec->normInv = 0;
	}

	/*sin and cos table*/
	switch(order)
	{
		case 5 :
			pkSpec->cos1 = (int *)&cfft_table_32_cos[0];
			pkSpec->sin1 = (int *)&cfft_table_32_sin[0];
			break;
		case 6 :
			pkSpec->cos1 = (int *)&cfft_table_64_cos[0];
			pkSpec->sin1 = (int *)&cfft_table_64_sin[0];
			break;
		case 7 :
			pkSpec->cos1 = (int *)&cfft_table_128_cos[0];
			pkSpec->sin1 = (int *)&cfft_table_128_sin[0];
			break;
		case 9 :
			pkSpec->cos1 = (int *)&cfft_table_512_cos[0];
			pkSpec->sin1 = (int *)&cfft_table_512_sin[0];
			break;
		default:
			assert(0);
	}
#if 0
	halfsize = pkSpec->len >> 1;
	pkSpec->cos = (double*)(pMemSpec + sizeof(k_IppsFFTSpec_C_32s)); 
	pkSpec->sin = (double*)(pMemSpec + sizeof(k_IppsFFTSpec_C_32s) + sizeof(double)*halfsize);
	for(i = 0; i < halfsize; i++)
	{
		theta = K_2PI * i / (double)pkSpec->len;
		pkSpec->cos[i] = cos(theta);
		pkSpec->sin[i] = -sin(theta);
	}
#endif
	/*bit reverse table*/
	pkSpec->ReverseTbl = (int*)(pMemSpec + sizeof(k_IppsFFTSpec_C_32s)+ sizeof(double)*pkSpec->len);
	for (i = 0; i < pkSpec->len; i++)
	{
		int reversed = 0;
		int b0;
		int tmp = i;

		for (b0 = 0; b0 < pkSpec->order; b0++)
		{
			reversed = (reversed << 1) | (tmp & 1);
			tmp >>= 1;
		}
		pkSpec->ReverseTbl[i] = reversed;
	}	

	*ppFFTSpec = (IppsFFTSpec_C_32s*) pkSpec;

#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTInit_C_32s",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTInit_C_32s(ppFFTSpec, order, flag, hint, pMemSpec,pBufInit);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTInv_CToC_32s_Sfs_c(const Ipp32s* pSrcRe, const Ipp32s* pSrcIm, Ipp32s* pDstRe, 
											  Ipp32s* pDstIm, const IppsFFTSpec_C_32s* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32s * pkSpec = (k_IppsFFTSpec_C_32s*)pFFTSpec;

	int i,j;

	int step, shift, pos;
	int size, exp, estep;

	Ipp64s rounding;

	Ipp64s *xr, *xi, tmp;

	if (!pSrcRe || !pSrcIm || !pDstRe || !pDstIm || !pFFTSpec || !pBuffer)
		return ippStsNullPtrErr;

	if (pkSpec->sig != REAL_SIG)		
		return ippStsContextMatchErr;	

	xr = (Ipp64s*)pBuffer;
	xi = (Ipp64s*)(pBuffer + pkSpec->len * sizeof(Ipp64s));


	for (i = 0; i < pkSpec->len; i++)
	{
		xr[i] = pSrcRe[i];
		xi[i] = -pSrcIm[i];
	}

	/*bit reverse*/
	for (i = 0; i < pkSpec->len; i++)
	{
		j = pkSpec->ReverseTbl[i];
		if (j <= i)
			continue;

		tmp = xr[i];
		xr[i] = xr[j];
		xr[j] = tmp;

		tmp = xi[i];
		xi[i] = xi[j];
		xi[j] = tmp;	
	}
	/*reverse end*/

	/*Inv transform*/
	estep = size = pkSpec->len;
	for (step = 1; step < size; step *= 2)
	{
		int x1;
		int x2 = 0;
		estep >>= 1;
		for (pos = 0; pos < size; pos += (2 * step))
		{
			x1 = x2;
			x2 += step;
			exp = 0;
			for (shift = 0; shift < step; shift++)
			{
				Ipp64s v2r, v2i;
				v2r = (xr[x2] * pkSpec->cos1[exp] - xi[x2] * pkSpec->sin1[exp]) >> 31;
				v2i = (xr[x2] * pkSpec->sin1[exp] + xi[x2] * pkSpec->cos1[exp]) >> 31;

				xr[x2] = xr[x1] - v2r;
				xr[x1] += v2r;

				xi[x2] = xi[x1] - v2i;

				xi[x1] += v2i;

				exp += estep;

				x1++;
				x2++;
			}
		}
	}
	/*transform end*/

	/*normlizing & scaling*/
	for (i = 0; i < pkSpec->len; i++)
	{
		xi[i] = -xi[i];
	}

	if (scaleFactor)
		rounding = (Ipp64s)(1<< (scaleFactor -1 ));
	else
		rounding = 0;

	if (pkSpec->flagInv)
	{
		for (i = 0; i < pkSpec->len; i++)
		{
			if (xr[i] < 0)
				pDstRe[i] = (Ipp32s)(((__int64)((xr[i]/pkSpec->normInv) - rounding)) >> scaleFactor);
			else
				pDstRe[i] = (Ipp32s)(((__int64)((xr[i]/pkSpec->normInv) + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDstIm[i] = (Ipp32s)(((__int64)((xi[i]/pkSpec->normInv) - rounding)) >> scaleFactor);
			else
				pDstIm[i] = (Ipp32s)(((__int64)((xi[i]/pkSpec->normInv) + rounding)) >> scaleFactor);
		}
	}
	else
	{
		for (i = 0; i < pkSpec->len; i++)
		{
			if (xr[i] < 0)
				pDstRe[i] = (Ipp32s)(((__int64)(xr[i] - rounding)) >> scaleFactor);
			else
				pDstRe[i] = (Ipp32s)(((__int64)(xr[i] + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDstIm[i] = (Ipp32s)(((__int64)(xi[i] - rounding)) >> scaleFactor);
			else
				pDstIm[i] = (Ipp32s)(((__int64)(xi[i] + rounding)) >> scaleFactor);
		}
	}
	
#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTInv_CToC_32s_Sfs",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTInv_CToC_32s_Sfs(pSrcRe, pSrcIm, pDstRe, pDstIm, pFFTSpec, scaleFactor, pBuffer);
		return sts;
	}
#endif

	return ippStsNoErr;
}

#endif
