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
//***


#define MP4_VLC_INTERNAL		1   // Just for vlc table
#include "kinoma_ipp_lib.h"

#include "kinoma_mpeg4_com_bitstream.h"

IppStatus (__STDCALL *ippiReconstructCoeffsInter_MPEG4_1u16s_universal)	(Ipp8u**ppBitStream, int*pBitOffset,Ipp16s* pCoeffs,int* pIndxLastNonZero,int rvlcFlag,int scan,const IppiQuantInvInterSpec_MPEG4* pQuantInvInterSpec,int QP)=NULL;
IppStatus (__STDCALL *ippiDecodeDCIntra_MPEG4_1u16s_universal)			(Ipp8u **ppBitStream, int *pBitOffset,Ipp16s *pDC, int blockType)=NULL;
IppStatus (__STDCALL *ippiDecodeCoeffsIntra_MPEG4_1u16s_universal)		(Ipp8u**  ppBitStream,int* pBitOffset,Ipp16s*  pCoeffs, int* pIndxLastNonZero, int rvlcFlag, int noDCFlag,int scan)=NULL;
IppStatus (__STDCALL *ippiReconstructCoeffsIntra_H263_1u16s_universal) ( Ipp8u** ppBitStream, int* pBitOffset, Ipp16s* pCoef, int* pIndxLastNonZero, int cbp, int QP, int advIntraFlag, int scan, int modQuantFlag)=NULL;
IppStatus (__STDCALL *ippiReconstructCoeffsInter_H263_1u16s_universal) 	(Ipp8u** ppBitStream,int* pBitOffset,Ipp16s* pCoef,int* pIndxLastNonZero, int QP, int modQuantFlag)=NULL;

/* event structure for transform coeffs */
typedef struct 
{
  Ipp32s last, run, level, sign;
} Tcoef;


/* tables for inter luminance blocks */
//***static 
Ipp32s DCT3Dtab0[][2] = 
{
	{4225,7}, {4209,7}, {4193,7}, {4177,7}, {193,7}, {177,7}, 
	{161,7}, {4,7}, {4161,6}, {4161,6}, {4145,6}, {4145,6}, 
	{4129,6}, {4129,6}, {4113,6}, {4113,6}, {145,6}, {145,6}, 
	{129,6}, {129,6}, {113,6}, {113,6}, {97,6}, {97,6}, 
	{18,6}, {18,6}, {3,6}, {3,6}, {81,5}, {81,5}, 
	{81,5}, {81,5}, {65,5}, {65,5}, {65,5}, {65,5}, 
	{49,5}, {49,5}, {49,5}, {49,5}, {4097,4}, {4097,4}, 
	{4097,4}, {4097,4}, {4097,4}, {4097,4}, {4097,4}, {4097,4}, 
	{1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, 
	{1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, 
	{1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, 
	{1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, 
	{1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, 
	{1,2}, {1,2}, {17,3}, {17,3}, {17,3}, {17,3}, 
	{17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, 
	{17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, 
	{33,4}, {33,4}, {33,4}, {33,4}, {33,4}, {33,4}, 
	{33,4}, {33,4}, {2,4}, {2,4},{2,4},{2,4},
	{2,4}, {2,4},{2,4},{2,4},
};

//***static 
Ipp32s DCT3Dtab1[][2] = 
{
	{9,10}, {8,10}, {4481,9}, {4481,9}, {4465,9}, {4465,9}, 
	{4449,9}, {4449,9}, {4433,9}, {4433,9}, {4417,9}, {4417,9}, 
	{4401,9}, {4401,9}, {4385,9}, {4385,9}, {4369,9}, {4369,9}, 
	{4098,9}, {4098,9}, {353,9}, {353,9}, {337,9}, {337,9}, 
	{321,9}, {321,9}, {305,9}, {305,9}, {289,9}, {289,9}, 
	{273,9}, {273,9}, {257,9}, {257,9}, {241,9}, {241,9}, 
	{66,9}, {66,9}, {50,9}, {50,9}, {7,9}, {7,9}, 
	{6,9}, {6,9}, {4353,8}, {4353,8}, {4353,8}, {4353,8}, 
	{4337,8}, {4337,8}, {4337,8}, {4337,8}, {4321,8}, {4321,8}, 
	{4321,8}, {4321,8}, {4305,8}, {4305,8}, {4305,8}, {4305,8}, 
	{4289,8}, {4289,8}, {4289,8}, {4289,8}, {4273,8}, {4273,8}, 
	{4273,8}, {4273,8}, {4257,8}, {4257,8}, {4257,8}, {4257,8}, 
	{4241,8}, {4241,8}, {4241,8}, {4241,8}, {225,8}, {225,8}, 
	{225,8}, {225,8}, {209,8}, {209,8}, {209,8}, {209,8}, 
	{34,8}, {34,8}, {34,8}, {34,8}, {19,8}, {19,8}, 
	{19,8}, {19,8}, {5,8}, {5,8}, {5,8}, {5,8}, 
};

//***static 
Ipp32s DCT3Dtab2[][2] = 
{
	{4114,11}, {4114,11}, {4099,11}, {4099,11}, {11,11}, {11,11}, 
	{10,11}, {10,11}, {4545,10}, {4545,10}, {4545,10}, {4545,10}, 
	{4529,10}, {4529,10}, {4529,10}, {4529,10}, {4513,10}, {4513,10}, 
	{4513,10}, {4513,10}, {4497,10}, {4497,10}, {4497,10}, {4497,10}, 
	{146,10}, {146,10}, {146,10}, {146,10}, {130,10}, {130,10}, 
	{130,10}, {130,10}, {114,10}, {114,10}, {114,10}, {114,10}, 
	{98,10}, {98,10}, {98,10}, {98,10}, {82,10}, {82,10}, 
	{82,10}, {82,10}, {51,10}, {51,10}, {51,10}, {51,10}, 
	{35,10}, {35,10}, {35,10}, {35,10}, {20,10}, {20,10}, 
	{20,10}, {20,10}, {12,11}, {12,11}, {21,11}, {21,11}, 
	{369,11}, {369,11}, {385,11}, {385,11}, {4561,11}, {4561,11}, 
	{4577,11}, {4577,11}, {4593,11}, {4593,11}, {4609,11}, {4609,11}, 
	{22,12}, {36,12}, {67,12}, {83,12}, {99,12}, {162,12}, 
	{401,12}, {417,12}, {4625,12}, {4641,12}, {4657,12}, {4673,12}, 
	{4689,12}, {4705,12}, {4721,12}, {4737,12}, {7167,7}, 
	{7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, 
	{7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, 
	{7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, 
	{7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, 
	{7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, 
	{7167,7}, };

/* tables for Intra luminance blocks */
static Ipp32s DCT3Dtab3[][2] = 
{
    {0x10401, 7}, {0x10301, 7}, {0x00601, 7}, {0x10501, 7},
    {0x00701, 7}, {0x00202, 7}, {0x00103, 7}, {0x00009, 7},
    {0x10002, 6}, {0x10002, 6}, {0x00501, 6}, {0x00501, 6}, 
    {0x10201, 6}, {0x10201, 6}, {0x10101, 6}, {0x10101, 6},
    {0x00401, 6}, {0x00401, 6}, {0x00301, 6}, {0x00301, 6},
    {0x00008, 6}, {0x00008, 6}, {0x00007, 6}, {0x00007, 6}, 
    {0x00102, 6}, {0x00102, 6}, {0x00006, 6}, {0x00006, 6},
    {0x00201, 5}, {0x00201, 5}, {0x00201, 5}, {0x00201, 5},
    {0x00005, 5}, {0x00005, 5}, {0x00005, 5}, {0x00005, 5}, 
    {0x00004, 5}, {0x00004, 5}, {0x00004, 5}, {0x00004, 5}, 
    {0x10001, 4}, {0x10001, 4}, {0x10001, 4}, {0x10001, 4},
    {0x10001, 4}, {0x10001, 4}, {0x10001, 4}, {0x10001, 4},
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2},
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2},
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, 
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2},
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2},
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, 
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2},
    {0x00001, 2}, {0x00001, 2}, {0x00001, 2}, {0x00001, 2},
    {0x00002, 3}, {0x00002, 3}, {0x00002, 3}, {0x00002, 3}, 
    {0x00002, 3}, {0x00002, 3}, {0x00002, 3}, {0x00002, 3},
    {0x00002, 3}, {0x00002, 3}, {0x00002, 3}, {0x00002, 3},
    {0x00002, 3}, {0x00002, 3}, {0x00002, 3}, {0x00002, 3}, 
    {0x00101, 4}, {0x00101, 4}, {0x00101, 4}, {0x00101, 4},
    {0x00101, 4}, {0x00101, 4}, {0x00101, 4}, {0x00101, 4},
    {0x00003, 4}, {0x00003, 4}, {0x00003, 4}, {0x00003, 4},
    {0x00003, 4}, {0x00003, 4}, {0x00003, 4}, {0x00003, 4},
};


static Ipp32s DCT3Dtab4[][2] = 
{
    {0x00012,10}, {0x00011,10}, {0x10e01, 9}, {0x10e01, 9},
    {0x10d01, 9}, {0x10d01, 9}, {0x10c01, 9}, {0x10c01, 9},
    {0x10b01, 9}, {0x10b01, 9}, {0x10a01, 9}, {0x10a01, 9}, 
    {0x10102, 9}, {0x10102, 9}, {0x10004, 9}, {0x10004, 9},
    {0x00c01, 9}, {0x00c01, 9}, {0x00b01, 9}, {0x00b01, 9},
    {0x00702, 9}, {0x00702, 9}, {0x00602, 9}, {0x00602, 9}, 
    {0x00502, 9}, {0x00502, 9}, {0x00303, 9}, {0x00303, 9},
    {0x00203, 9}, {0x00203, 9}, {0x00106, 9}, {0x00106, 9},
    {0x00105, 9}, {0x00105, 9}, {0x00010, 9}, {0x00010, 9}, 
    {0x00402, 9}, {0x00402, 9}, {0x0000f, 9}, {0x0000f, 9},
    {0x0000e, 9}, {0x0000e, 9}, {0x0000d, 9}, {0x0000d, 9},
    {0x10801, 8}, {0x10801, 8}, {0x10801, 8}, {0x10801, 8}, 
    {0x10701, 8}, {0x10701, 8}, {0x10701, 8}, {0x10701, 8},
    {0x10601, 8}, {0x10601, 8}, {0x10601, 8}, {0x10601, 8},
    {0x10003, 8}, {0x10003, 8}, {0x10003, 8}, {0x10003, 8},  
    {0x00a01, 8}, {0x00a01, 8}, {0x00a01, 8}, {0x00a01, 8},
    {0x00901, 8}, {0x00901, 8}, {0x00901, 8}, {0x00901, 8},
    {0x00801, 8}, {0x00801, 8}, {0x00801, 8}, {0x00801, 8},  
    {0x10901, 8}, {0x10901, 8}, {0x10901, 8}, {0x10901, 8},
    {0x00302, 8}, {0x00302, 8}, {0x00302, 8}, {0x00302, 8},
    {0x00104, 8}, {0x00104, 8}, {0x00104, 8}, {0x00104, 8},  
    {0x0000c, 8}, {0x0000c, 8}, {0x0000c, 8}, {0x0000c, 8},
    {0x0000b, 8}, {0x0000b, 8}, {0x0000b, 8}, {0x0000b, 8},
    {0x0000a, 8}, {0x0000a, 8}, {0x0000a, 8}, {0x0000a, 8}, 
};

static Ipp32s DCT3Dtab5[][2] = 
{
    {0x10007,11}, {0x10007,11}, {0x10006,11}, {0x10006,11},
    {0x00016,11}, {0x00016,11}, {0x00015,11}, {0x00015,11},
    {0x10202,10}, {0x10202,10}, {0x10202,10}, {0x10202,10},  
    {0x10103,10}, {0x10103,10}, {0x10103,10}, {0x10103,10},
    {0x10005,10}, {0x10005,10}, {0x10005,10}, {0x10005,10},
    {0x00d01,10}, {0x00d01,10}, {0x00d01,10}, {0x00d01,10},  
    {0x00503,10}, {0x00503,10}, {0x00503,10}, {0x00503,10},
    {0x00802,10}, {0x00802,10}, {0x00802,10}, {0x00802,10},
    {0x00403,10}, {0x00403,10}, {0x00403,10}, {0x00403,10},  
    {0x00304,10}, {0x00304,10}, {0x00304,10}, {0x00304,10},
    {0x00204,10}, {0x00204,10}, {0x00204,10}, {0x00204,10},
    {0x00107,10}, {0x00107,10}, {0x00107,10}, {0x00107,10}, 
    {0x00014,10}, {0x00014,10}, {0x00014,10}, {0x00014,10},
    {0x00013,10}, {0x00013,10}, {0x00013,10}, {0x00013,10},
    {0x00017,11}, {0x00017,11}, {0x00018,11}, {0x00018,11}, 
    {0x00108,11}, {0x00108,11}, {0x00902,11}, {0x00902,11},
    {0x10302,11}, {0x10302,11}, {0x10402,11}, {0x10402,11},
    {0x10f01,11}, {0x10f01,11}, {0x11001,11}, {0x11001,11}, 
    {0x00019,12}, {0x0001a,12}, {0x0001b,12}, {0x00109,12},
    {0x00603,12}, {0x0010a,12}, {0x00205,12}, {0x00703,12},
    {0x00e01,12}, {0x10008,12}, {0x10502,12}, {0x10602,12}, 
    {0x11101,12}, {0x11201,12}, {0x11301,12}, {0x11401,12},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
    {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7}, {0x01bff, 7},
};



/* RVLC tables */

static Ipp32s RvlcDCT3Dtab0[][2] = {
  {1,3},  {2,3},  {3,4},  {4,5},  {5,6},  {6,6},  {7,7},
  {8,8},  {9,8},  {10,9},  {11,9},  {12,10},  {13,10},  {14,10},
  {15,11},  {16,11},  {17,11},  {18,12},  {19,12},  {20,13},  {21,13},
  {22,12},  {23,13},  {24,14},  {25,14},  {26,14},  {27,15},  {257,4},
  {258,5},  {259,7},  {260,8},  {261,8},  {262,9},  {263,10},  {264,11},
  {265,11},  {266,12},  {267,13},  {268,14},  {269,14},  {513,5},  {514,7},
  {515,9},  {516,10},  {517,11},  {518,11},  {519,13},  {520,13},  {521,13},
  {522,14},  {523,14},  {769,5},  {770,8},  {771,9},  {772,11},  {773,12},
  {774,13},  {775,14},  {776,14},  {777,15},  {1025,6},  {1026,8},  {1027,10},
  {1028,12},  {1029,12},  {1030,14},  {1281,6},  {1282,9},  {1283,11},  {1284,12},
  {1285,14},  {1286,14},  {1537,7},  {1538,10},  {1539,11},  {1540,12},  {1541,15},
  {1793,7},  {1794,10},  {1795,11},  {1796,13},  {1797,15},  {2049,8},  {2050,10},
  {2051,13},  {2052,14},  {2305,8},  {2306,11},  {2307,13},  {2308,15},  {2561,9},
  {2562,12},  {2817,10},  {2818,13},  {3073,10},  {3074,15},  {3329,11},  {3585,13},
  {3841,13},  {4097,14},  {4353,14},  {4609,14},  {4865,15},  {65537,4},  {65538,8},
  {65539,11},  {65540,13},  {65541,14},  {65793,5},  {65794,9},  {65795,12},  {65796,14},
  {65797,15},  {66049,5},  {66050,11},  {66051,15},  {66305,6},  {66306,12},  {66561,6},
  {66562,12},  {66817,6},  {66818,13},  {67073,6},  {67074,13},  {67329,7},  {67330,13},
  {67585,7},  {67586,13},  {67841,7},  {67842,13},  {68097,7},  {68098,14},  {68353,7},
  {68354,14},  {68609,8},  {68610,14},  {68865,8},  {68866,15},  {69121,8},  {69377,9},
  {69633,9},  {69889,9},  {70145,9},  {70401,9},  {70657,9},  {70913,10},  {71169,10},
  {71425,10},  {71681,10},  {71937,10},  {72193,11},  {72449,11},  {72705,11},  {72961,12},
  {73217,12},  {73473,12},  {73729,12},  {73985,12},  {74241,12},  {74497,12},  {74753,13},
  {75009,13},  {75265,14},  {75521,14},  {75777,14},  {76033,15},  {76289,15},  {76545,15},
  {76801,15},  {7167,4}   /* last entry: escape code */
};


static Ipp32s RvlcDCT3Dtab1[][2] = {
  {1,3},  {2,4},  {3,5},  {4,7},  {5,8},  {6,8},  {7,9},  {8,10},  {9,10},  {10,11},
  {11,11},  {12,12},  {13,13},  {14,13},  {15,13},  {16,13},  {17,14},  {18,14},  {19,15},  {257,3},
  {258,6},  {259,8},  {260,9},  {261,10},  {262,11},  {263,12},  {264,13},  {265,14},  {266,14},  {513,4},
  {514,7},  {515,9},  {516,11},  {517,12},  {518,14},  {519,14},  {769,5},  {770,8},  {771,10},  {772,12},
  {773,13},  {774,14},  {775,15},  {1025,5},  {1026,8},  {1027,11},  {1028,13},  {1029,15},  {1281,5},
  {1282,9},  {1283,11},  {1284,13},  {1537,6},  {1538,10},  {1539,12},  {1540,14},  {1793,6},  {1794,10},
  {1795,12},  {1796,15},  {2049,6},  {2050,10},  {2051,13},  {2305,7},  {2306,10},  {2307,14},  {2561,7},
  {2562,11},  {2817,7},  {2818,12},  {3073,8},  {3074,13},  {3329,8},  {3330,14},  {3585,8},  {3586,14},
  {3841,9},  {3842,14},  {4097,9},  {4098,14},  {4353,9},  {4354,15},  {4609,10},  {4865,10},  {5121,10},
  {5377,11},  {5633,11},  {5889,11},  {6145,11},  {6401,11},  {6657,11},  {6913,12},  {7169,12},  {7425,12},
  {7681,13},  {7937,13},  {8193,13},  {8449,13},  {8705,14},  {8961,14},  {9217,14},  {9473,15},  {9729,15},
  {65537,4},  {65538,8},  {65539,11},  {65540,13},  {65541,14},  {65793,5},  {65794,9},  {65795,12},  {65796,14},
  {65797,15},  {66049,5},  {66050,11},  {66051,15},  {66305,6},  {66306,12},  {66561,6},  {66562,12},  {66817,6},
  {66818,13},  {67073,6},  {67074,13},  {67329,7},  {67330,13},  {67585,7},  {67586,13},  {67841,7},  {67842,13},
  {68097,7},  {68098,14},  {68353,7},  {68354,14},  {68609,8},  {68610,14},  {68865,8},  {68866,15},  {69121,8},
  {69377,9},  {69633,9},  {69889,9},  {70145,9},  {70401,9},  {70657,9},  {70913,10},  {71169,10},  {71425,10},
  {71681,10},  {71937,10},  {72193,11},  {72449,11},  {72705,11},  {72961,12},  {73217,12},  {73473,12},  {73729,12},
  {73985,12},  {74241,12},  {74497,12},  {74753,13},  {75009,13},  {75265,14},  {75521,14},  {75777,14},  {76033,15},
  {76289,15},  {76545,15},  {76801,15},  {7167,4}          /* last entry: escape code */
};
  

static int dc_lum_tbl[8][2] = 
{
	{0, 0}, {3, 4}, {3, 3}, {3, 0},
	{2, 2}, {2, 2}, {2, 1}, {2, 1},	
};

Ipp32u
VlcDecIntraDCPredSize(Ipp8u **ppBitStream, int *pBitOffset, int blockType)
{
	Ipp32u  code;
	int i;

	if( IPPVC_BLOCK_LUMA == blockType ) /* luminance block */
	{
		//***use CLZ  --bnie 7/10/07
		/*code = k_mp4_ShowBits (ppBitStream,pBitOffset, 11);*/
		code = k_mp4_ShowBits11 (ppBitStream,pBitOffset);
		for (i = 11; i > 3; i--)
		{
			if ( code == 1)
			{
				k_mp4_FlushBits(ppBitStream,pBitOffset, i);
				return (i + 1);
			}
			code >>= 1;
		}
		k_mp4_FlushBits(ppBitStream,pBitOffset,dc_lum_tbl[code][0]);
		return dc_lum_tbl[code][1];
	}
	else /* chrominance block */
	{
		//***use CLZ  --bnie 7/10/07
		/*code = k_mp4_ShowBits (ppBitStream, pBitOffset,12);*/
		code = k_mp4_ShowBits12 (ppBitStream, pBitOffset);
		for (i = 12; i > 2; i--)
		{
			if ( code == 1)
			{
				k_mp4_FlushBits(ppBitStream,pBitOffset, i);
				return i;
			}
			code >>= 1;
		}
		/*return (3 - k_mp4_GetBits(ppBitStream, pBitOffset,2));*/
		return (3 - k_mp4_GetBit2(ppBitStream, pBitOffset));

	}    
}	/* VlcDecIntraDCPredSize */



Tcoef
VlcDecodeIntraTCOEF(Ipp8u **ppBitStream, int *pBitOffset)
{
	Ipp32u		code;
	Ipp32s		*tab;
	Tcoef		tcoef =	{0, 0, 0};

	/*code = k_mp4_ShowBits (ppBitStream,pBitOffset,12);*/
	code = k_mp4_ShowBits12 (ppBitStream,pBitOffset);

	if (code > 511)
		tab = DCT3Dtab3[(code >> 5) - 16];
	else if (code > 127)
		tab = DCT3Dtab4[(code >> 2) - 32];
	else if (code > 7)
		tab = DCT3Dtab5[(code >> 0) - 8];
	else
	{
		return tcoef;
	}

	k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);
	if (tab[0] != 7167)	
	{
		tcoef.run = (tab[0] >> 8) & 255;
		tcoef.level = tab[0] & 255;
		tcoef.last = (tab[0] >> 16) & 1;
		/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);        */
		tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);        
	}
	
	/* the following is modified for 3-mode escape -- boon */
	else		/* if (tab[0] == 7167)	ESCAPE */
	{
		int level_offset;
		/*level_offset = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
		level_offset = k_mp4_GetBit1(ppBitStream, pBitOffset);

		if (!level_offset) 
		{
			/* first escape mode. level is offset */
			/*code = k_mp4_ShowBits (ppBitStream,pBitOffset, 12);*/
			code = k_mp4_ShowBits12 (ppBitStream,pBitOffset);

			if (code > 511)
				tab = DCT3Dtab3[(code >> 5) - 16];
			else if (code >= 128)
				tab = DCT3Dtab4[(code >> 2) - 32];
			else if (code >= 8)
				tab = DCT3Dtab5[(code >> 0) - 8];
			else
			{
				return tcoef;
			}

			k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);
			tcoef.run = (tab[0] >> 8) & 255;
			tcoef.level = tab[0] & 255;
			tcoef.last = (tab[0] >> 16) & 1;

			/* need to add back the max level */
			tcoef.level = tcoef.level + intra_max_level[tcoef.last][tcoef.run];

			/* sign bit */
			/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);       */
			tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);       
		} 
		else 
		{
			int run_offset;
			/*run_offset = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
			run_offset = k_mp4_GetBit1(ppBitStream, pBitOffset);

			if (!run_offset) 
			{
				/* second escape mode. run is offset */
				/*code = k_mp4_ShowBits (ppBitStream,pBitOffset, 12);*/
				code = k_mp4_ShowBits12 (ppBitStream,pBitOffset);

				if (code > 511)
					tab = DCT3Dtab3[(code >> 5) - 16];
				else if (code > 127)
					tab = DCT3Dtab4[(code >> 2) - 32];
				else if (code > 7)
					tab = DCT3Dtab5[(code >> 0) - 8];
				else
				{
					return tcoef;
				}

				k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);

				tcoef.run = (tab[0] >> 8) & 255;
				tcoef.level = tab[0] & 255;
				tcoef.last = (tab[0] >> 16) & 1;

				/* need to add back the max run */
				if (tcoef.last)
					tcoef.run = tcoef.run + intra_max_run1[tcoef.level]+1;
				else
					tcoef.run = tcoef.run + intra_max_run0[tcoef.level]+1;				 
				/* sign bit */
				/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);  */      
				tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);        
			} 
			else 
			{
				/* third escape mode. flc */
				/*tcoef.last = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
				tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);

				/*tcoef.run = k_mp4_GetBits(ppBitStream, pBitOffset, 6);*/
				tcoef.run = k_mp4_GetBit6(ppBitStream, pBitOffset);

				/* 11.08.98 Sven Brandau: "insert marker_bit" due to N2339, Clause 2.1.21 */
				k_mp4_FlushBits(ppBitStream, pBitOffset, 1);

				/*tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset, 12);*/
				tcoef.level = k_mp4_GetBit12(ppBitStream, pBitOffset);

				/* 11.08.98 Sven Brandau: "insert marker_bit" due to N2339, Clause 2.1.21 */
				k_mp4_FlushBits(ppBitStream, pBitOffset, 1);

				if (tcoef.level > 2047)
				{
					tcoef.sign = 1;
					tcoef.level = 4096 - tcoef.level;
				}
				else
				{
					tcoef.sign = 0;
				}

			} /* flc */
		}
	}

	return tcoef;
}

Tcoef
VlcDecodeInterTCOEF(Ipp8u **ppBitStream, int *pBitOffset, int short_video_header)
{
	Ipp32u    code;
	Ipp32s  *tab;
	Tcoef   tcoef =	{0, 0, 0};

	/*code = k_mp4_ShowBits (ppBitStream,pBitOffset,12);*/
	code = k_mp4_ShowBits12 (ppBitStream,pBitOffset);

	if (code > 511)
		tab = DCT3Dtab0[(code >> 5) - 16];
	else if (code > 127)
		tab = DCT3Dtab1[(code >> 2) - 32];
	else if (code > 7)
		tab = DCT3Dtab2[(code >> 0) - 8];
	else
	{
		return tcoef;
	}

	k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);

	if (tab[0] != 7167)
	{
		tcoef.run = (tab[0] >> 4) & 255;
		tcoef.level = tab[0] & 15;
		tcoef.last = (tab[0] >> 12) & 1;
		/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1); */
		tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);
		return tcoef;
	}
	/*if (tab[0] == 7167) ESCAPE */
	if (short_video_header) 
	{              
		/* escape mode 4 - H.263 type */
		/*tcoef.last = k_mp4_GetBits(ppBitStream, pBitOffset, 1); */
		tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);

		/*tcoef.run = k_mp4_GetBits(ppBitStream, pBitOffset, 6); */
		tcoef.run = k_mp4_GetBit6(ppBitStream, pBitOffset); 

		/*tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset, 8); */
		tcoef.level = k_mp4_GetBit8(ppBitStream, pBitOffset);

		if (tcoef.level == 0 || tcoef.level == 128) 
		{
#ifdef _DEBUG
			printf ("Illegal LEVEL for ESCAPE mode 4: 0 or 128\n");
#endif
			return tcoef;
		}

		if (tcoef.level > 127) 
		{ 
			tcoef.sign = 1; 
			tcoef.level = 256 - tcoef.level; 
		} 
		else 
		{ 
			tcoef.sign = 0; 
		}
	}
#ifdef SUPPORT_H263_ONLY
	else return tcoef;//MP4_STATUS_FILE_ERROR;
#else	
	else 
	{   /* not escape mode 4 */
		int level_offset;
		/*level_offset = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
		level_offset = k_mp4_GetBit1(ppBitStream, pBitOffset);

		if (!level_offset) 
		{
			/* first escape mode. level is offset */
			/*code = k_mp4_ShowBits(ppBitStream,pBitOffset, 12);*/
			code = k_mp4_ShowBits12(ppBitStream,pBitOffset);

			if (code > 511)
				tab = DCT3Dtab0[(code >> 5) - 16];
			else if (code > 127)
				tab = DCT3Dtab1[(code >> 2) - 32];
			else if (code > 7)
				tab = DCT3Dtab2[(code >> 0) - 8];
			else
			{
				return tcoef;
			}

			k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);

			tcoef.run = (tab[0]>> 4) & 255;
			tcoef.level = tab[0] & 15;
			tcoef.last = (tab[0] >> 12) & 1;

			/* need to add back the max level */
			tcoef.level = tcoef.level + inter_max_level[tcoef.last][tcoef.run];

			/* sign bit */
			/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);       */
			tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);       

		}
		else
		{
			int run_offset;
			/*run_offset = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
			run_offset = k_mp4_GetBit1(ppBitStream, pBitOffset);

			if (!run_offset) 
			{
				/* second escape mode. run is offset */
				/*code = k_mp4_ShowBits (ppBitStream,pBitOffset, 12);*/
				code = k_mp4_ShowBits12 (ppBitStream,pBitOffset);

				if (code > 511)
					tab = DCT3Dtab0[(code >> 5) - 16];
				else if (code > 127)
					tab = DCT3Dtab1[(code >> 2) - 32];
				else if (code > 7)
					tab = DCT3Dtab2[(code >> 0) - 8];
				else
				{
					return tcoef;
				}

				k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);

				tcoef.run = (tab[0] >> 4) & 255;
				tcoef.level = tab[0] & 15;
				tcoef.last = (tab[0] >> 12) & 1;

				/* need to add back the max run */
				if (tcoef.last)
					tcoef.run = tcoef.run + inter_max_run1[tcoef.level]+1;
				else
					tcoef.run = tcoef.run + inter_max_run0[tcoef.level]+1;

				/* sign bit */
				/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);        */
				tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);        

			}
			else
			{
				/* third escape mode. flc */
				/*tcoef.last = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
				tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);

				/*tcoef.run = k_mp4_GetBits(ppBitStream, pBitOffset, 6);*/
				tcoef.run = k_mp4_GetBit6(ppBitStream, pBitOffset);

				k_mp4_FlushBits(ppBitStream, pBitOffset, 1);

				/*tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset, 12);*/
				tcoef.level = k_mp4_GetBit12(ppBitStream, pBitOffset);

				k_mp4_FlushBits(ppBitStream, pBitOffset, 1);

				if (tcoef.level > 2047)
				{
					tcoef.sign = 1;
					tcoef.level = 4096 - tcoef.level;
				}
				else
				{
					tcoef.sign = 0;
				}

			} /* flc */
		}
	}
#endif

	return tcoef;
}


Tcoef
RvlcDecodeIntraTCOEF(Ipp8u **ppBitStream, int *pBitOffset)
{
	Ipp32u		code, mask; 
	Ipp32s		*tab;
	Tcoef		tcoef =	{0, 0, 0};

    int count, len;
 
	mask = 0x4000;      /* mask  100000000000000   */
    /*code = k_mp4_ShowBits (ppBitStream, pBitOffset,15);*/
    code = k_mp4_ShowBits15 (ppBitStream, pBitOffset);
    len = 1;
     
    //***use CLZ  --bnie 7/10/07
	if (code & mask) {
		count = 1;
		while (count > 0) {
			mask = mask >> 1;
			if (code & mask) 
				count--;
			len++;
		}
	}
	else {
		count = 2;
		while (count > 0) {
			mask = mask >> 1;
			if (!(code & mask))
				count--;
			len++;
		}
	}
    
    code = code & 0x7fff;
    code = code >> (15 - (len + 1));
    
	switch(code) {

	case 0x0:
		tab = RvlcDCT3Dtab0[169];
		break;

	case 0x1: 
		tab = RvlcDCT3Dtab0[27];
		break;

	case 0x4: 
		tab = RvlcDCT3Dtab0[40];
		break;

	case 0x5:
		tab = RvlcDCT3Dtab0[51];
		break;

	case 0x6:
		tab = RvlcDCT3Dtab0[0];
		break;

	case 0x7:
		tab = RvlcDCT3Dtab0[1];
		break;

	case 0x8:
		tab = RvlcDCT3Dtab0[28];
		break;

	case 0x9:
		tab = RvlcDCT3Dtab0[3];
		break;

	case 0xa:
		tab = RvlcDCT3Dtab0[2];
		break;

	case 0xb:
		tab = RvlcDCT3Dtab0[103];
		break;

	case 0xc:
		tab = RvlcDCT3Dtab0[60];
		break;

	case 0xd:
		tab = RvlcDCT3Dtab0[66];
		break;

	case 0x12:
		tab = RvlcDCT3Dtab0[108];
		break;

	case 0x13:
		tab = RvlcDCT3Dtab0[113];
		break;

	case 0x14:
		tab = RvlcDCT3Dtab0[4];
		break;

	case 0x15:
		tab = RvlcDCT3Dtab0[5];
		break;

	case 0x18:
		tab = RvlcDCT3Dtab0[116];
		break;

	case 0x19:
		tab = RvlcDCT3Dtab0[118];
		break;

	case 0x1c:
		tab = RvlcDCT3Dtab0[72];
		break;

	case 0x1d:
		tab = RvlcDCT3Dtab0[77];
		break;

	case 0x22:
		tab = RvlcDCT3Dtab0[120];
		break;

	case 0x23:
		tab = RvlcDCT3Dtab0[122];
		break;

	case 0x2c:
		tab = RvlcDCT3Dtab0[41];
		break;

	case 0x2d:
		tab = RvlcDCT3Dtab0[29];
		break;

	case 0x34:
		tab = RvlcDCT3Dtab0[6];
		break;

	case 0x35:
		tab = RvlcDCT3Dtab0[124];
		break;

	case 0x38:
		tab = RvlcDCT3Dtab0[126];
		break;

	case 0x39:
		tab = RvlcDCT3Dtab0[128];
		break;

	case 0x3c:
		tab = RvlcDCT3Dtab0[82];
		break;

	case 0x3d:
		tab = RvlcDCT3Dtab0[86];
		break;

	case 0x42:
		tab = RvlcDCT3Dtab0[130];
		break;

	case 0x43:
		tab = RvlcDCT3Dtab0[132];
		break;

	case 0x5c:
		tab = RvlcDCT3Dtab0[52];
		break;

	case 0x5d:
		tab = RvlcDCT3Dtab0[61];
		break;

	case 0x6c:
		tab = RvlcDCT3Dtab0[30];
		break;

	case 0x6d:
		tab = RvlcDCT3Dtab0[31];
		break;

	case 0x74:
		tab = RvlcDCT3Dtab0[7];
		break;

	case 0x75:
		tab = RvlcDCT3Dtab0[8];
		break;

	case 0x78:
		tab = RvlcDCT3Dtab0[104];
		break;

	case 0x79:
		tab = RvlcDCT3Dtab0[134];
		break;

	case 0x7c:
		tab = RvlcDCT3Dtab0[90];
		break;

	case 0x7d:
		tab = RvlcDCT3Dtab0[67];
		break;

	case 0x82:
		tab = RvlcDCT3Dtab0[136];
		break;

	case 0x83:
		tab = RvlcDCT3Dtab0[138];
		break;

	case 0xbc:
		tab = RvlcDCT3Dtab0[42];
		break;

	case 0xbd:
		tab = RvlcDCT3Dtab0[53];
		break;

	case 0xdc:
		tab = RvlcDCT3Dtab0[32];
		break;

	case 0xdd:
		tab = RvlcDCT3Dtab0[9];
		break;

	case 0xec:
		tab = RvlcDCT3Dtab0[10];
		break;

	case 0xed:
		tab = RvlcDCT3Dtab0[109];
		break;

	case 0xf4:
		tab = RvlcDCT3Dtab0[139];
		break;

	case 0xf5:
		tab = RvlcDCT3Dtab0[140];
		break;

	case 0xf8:
		tab = RvlcDCT3Dtab0[141];
		break;

	case 0xf9:
		tab = RvlcDCT3Dtab0[142];
		break;

	case 0xfc:
		tab = RvlcDCT3Dtab0[92];
		break;

	case 0xfd:
		tab = RvlcDCT3Dtab0[94];
		break;

	case 0x102:
		tab = RvlcDCT3Dtab0[143];
		break;

	case 0x103:
		tab = RvlcDCT3Dtab0[144];
		break;

	case 0x17c:
		tab = RvlcDCT3Dtab0[73];
		break;

	case 0x17d:
		tab = RvlcDCT3Dtab0[78];
		break;

	case 0x1bc:
		tab = RvlcDCT3Dtab0[83];
		break;

	case 0x1bd:
		tab = RvlcDCT3Dtab0[62];
		break;

	case 0x1dc:
		tab = RvlcDCT3Dtab0[43];
		break;

	case 0x1dd:
		tab = RvlcDCT3Dtab0[33];
		break;

	case 0x1ec:
		tab = RvlcDCT3Dtab0[11];
		break;

	case 0x1ed:
		tab = RvlcDCT3Dtab0[12];
		break;

	case 0x1f4:
		tab = RvlcDCT3Dtab0[13];
		break;

	case 0x1f5:
		tab = RvlcDCT3Dtab0[145];
		break;

	case 0x1f8:
		tab = RvlcDCT3Dtab0[146];
		break;

	case 0x1f9:
		tab = RvlcDCT3Dtab0[147];
		break;

	case 0x1fc:
		tab = RvlcDCT3Dtab0[96];
		break;

	case 0x1fd:
		tab = RvlcDCT3Dtab0[87];
		break;

	case 0x202:
		tab = RvlcDCT3Dtab0[148];
		break;

	case 0x203:
		tab = RvlcDCT3Dtab0[149];
		break;

	case 0x2fc:
		tab = RvlcDCT3Dtab0[68];
		break;

	case 0x2fd:
		tab = RvlcDCT3Dtab0[74];
		break;

	case 0x37c:
		tab = RvlcDCT3Dtab0[79];
		break;

	case 0x37d:
		tab = RvlcDCT3Dtab0[54];
		break;

	case 0x3bc:
		tab = RvlcDCT3Dtab0[44];
		break;

	case 0x3bd:
		tab = RvlcDCT3Dtab0[45];
		break;

	case 0x3dc:
		tab = RvlcDCT3Dtab0[34];
		break;

	case 0x3dd:
		tab = RvlcDCT3Dtab0[35];
		break;

	case 0x3ec:
		tab = RvlcDCT3Dtab0[14];
		break;

	case 0x3ed:
		tab = RvlcDCT3Dtab0[15];
		break;

	case 0x3f4:
		tab = RvlcDCT3Dtab0[16];
		break;

	case 0x3f5:
		tab = RvlcDCT3Dtab0[105];
		break;

	case 0x3f8:
		tab = RvlcDCT3Dtab0[114];
		break;

	case 0x3f9:
		tab = RvlcDCT3Dtab0[150];
		break;

	case 0x3fc:
		tab = RvlcDCT3Dtab0[91];
		break;

	case 0x3fd:
		tab = RvlcDCT3Dtab0[63];
		break;

	case 0x402:
		tab = RvlcDCT3Dtab0[151];
		break;

	case 0x403:
		tab = RvlcDCT3Dtab0[152];
		break;

	case 0x5fc:
		tab = RvlcDCT3Dtab0[69];
		break;

	case 0x5fd:
		tab = RvlcDCT3Dtab0[75];
		break;

	case 0x6fc:
		tab = RvlcDCT3Dtab0[55];
		break;

	case 0x6fd:
		tab = RvlcDCT3Dtab0[64];
		break;

	case 0x77c:
		tab = RvlcDCT3Dtab0[36];
		break;

	case 0x77d:
		tab = RvlcDCT3Dtab0[17];
		break;

	case 0x7bc:
		tab = RvlcDCT3Dtab0[18];
		break;

	case 0x7bd:
		tab = RvlcDCT3Dtab0[21];
		break;

	case 0x7dc:
		tab = RvlcDCT3Dtab0[110];
		break;

	case 0x7dd:
		tab = RvlcDCT3Dtab0[117];
		break;

	case 0x7ec:
		tab = RvlcDCT3Dtab0[119];
		break;

	case 0x7ed:
		tab = RvlcDCT3Dtab0[153];
		break;

	case 0x7f4:
		tab = RvlcDCT3Dtab0[154];
		break;

	case 0x7f5:
		tab = RvlcDCT3Dtab0[155];
		break;

	case 0x7f8:
		tab = RvlcDCT3Dtab0[156];
		break;

	case 0x7f9:
		tab = RvlcDCT3Dtab0[157];
		break;

	case 0x7fc:
		tab = RvlcDCT3Dtab0[97];
		break;

	case 0x7fd:
		tab = RvlcDCT3Dtab0[98];
		break;

	case 0x802:
		tab = RvlcDCT3Dtab0[158];
		break;

	case 0x803:
		tab = RvlcDCT3Dtab0[159];
		break;

	case 0xbfc:
		tab = RvlcDCT3Dtab0[93];
		break;

	case 0xbfd:
		tab = RvlcDCT3Dtab0[84];
		break;

	case 0xdfc:
		tab = RvlcDCT3Dtab0[88];
		break;

	case 0xdfd:
		tab = RvlcDCT3Dtab0[80];
		break;

	case 0xefc:
		tab = RvlcDCT3Dtab0[56];
		break;

	case 0xefd:
		tab = RvlcDCT3Dtab0[46];
		break;

	case 0xf7c:
		tab = RvlcDCT3Dtab0[47];
		break;

	case 0xf7d:
		tab = RvlcDCT3Dtab0[48];
		break;

	case 0xfbc:
		tab = RvlcDCT3Dtab0[37];
		break;

	case 0xfbd:
		tab = RvlcDCT3Dtab0[19];
		break;

	case 0xfdc:
		tab = RvlcDCT3Dtab0[20];
		break;

	case 0xfdd:
		tab = RvlcDCT3Dtab0[22];
		break;

	case 0xfec:
		tab = RvlcDCT3Dtab0[106];
		break;

	case 0xfed:
		tab = RvlcDCT3Dtab0[121];
		break;

	case 0xff4:
		tab = RvlcDCT3Dtab0[123];
		break;

	case 0xff5:
		tab = RvlcDCT3Dtab0[125];
		break;

	case 0xff8:
		tab = RvlcDCT3Dtab0[127];
		break;

	case 0xff9:
		tab = RvlcDCT3Dtab0[129];
		break;

	case 0xffc:
		tab = RvlcDCT3Dtab0[99];
		break;

	case 0xffd:
		tab = RvlcDCT3Dtab0[100];
		break;

	case 0x1002:
		tab = RvlcDCT3Dtab0[160];
		break;

	case 0x1003:
		tab = RvlcDCT3Dtab0[161];
		break;

	case 0x17fc:
		tab = RvlcDCT3Dtab0[101];
		break;

	case 0x17fd:
		tab = RvlcDCT3Dtab0[85];
		break;

	case 0x1bfc:
		tab = RvlcDCT3Dtab0[70];
		break;

	case 0x1bfd:
		tab = RvlcDCT3Dtab0[65];
		break;

	case 0x1dfc:
		tab = RvlcDCT3Dtab0[71];
		break;

	case 0x1dfd:
		tab = RvlcDCT3Dtab0[57];
		break;

	case 0x1efc:
		tab = RvlcDCT3Dtab0[58];
		break;

	case 0x1efd:
		tab = RvlcDCT3Dtab0[49];
		break;

	case 0x1f7c:
		tab = RvlcDCT3Dtab0[50];
		break;

	case 0x1f7d:
		tab = RvlcDCT3Dtab0[38];
		break;

	case 0x1fbc:
		tab = RvlcDCT3Dtab0[39];
		break;

	case 0x1fbd:
		tab = RvlcDCT3Dtab0[23];
		break;

	case 0x1fdc:
		tab = RvlcDCT3Dtab0[24];
		break;

	case 0x1fdd:
		tab = RvlcDCT3Dtab0[25];
		break;

	case 0x1fec:
		tab = RvlcDCT3Dtab0[107];
		break;

	case 0x1fed:
		tab = RvlcDCT3Dtab0[111];
		break;

	case 0x1ff4:
		tab = RvlcDCT3Dtab0[131];
		break;

	case 0x1ff5:
		tab = RvlcDCT3Dtab0[133];
		break;

	case 0x1ff8:
		tab = RvlcDCT3Dtab0[135];
		break;

	case 0x1ff9:
		tab = RvlcDCT3Dtab0[162];
		break;

	case 0x1ffc:
		tab = RvlcDCT3Dtab0[26];
		break;

	case 0x1ffd:
		tab = RvlcDCT3Dtab0[59];
		break;

	case 0x2002:
		tab = RvlcDCT3Dtab0[163];
		break;

	case 0x2003:
		tab = RvlcDCT3Dtab0[164];
		break;

	case 0x2ffc:
		tab = RvlcDCT3Dtab0[76];
		break;

	case 0x2ffd:
		tab = RvlcDCT3Dtab0[81];
		break;

	case 0x37fc:
		tab = RvlcDCT3Dtab0[89];
		break;

	case 0x37fd:
		tab = RvlcDCT3Dtab0[95];
		break;

	case 0x3bfc:
		tab = RvlcDCT3Dtab0[102];
		break;

	case 0x3bfd:
		tab = RvlcDCT3Dtab0[112];
		break;

	case 0x3dfc:
		tab = RvlcDCT3Dtab0[115];
		break;

	case 0x3dfd:
		tab = RvlcDCT3Dtab0[137];
		break;

	case 0x3efc:
		tab = RvlcDCT3Dtab0[165];
		break;

	case 0x3efd:
		tab = RvlcDCT3Dtab0[166];
		break;

	case 0x3f7c:
		tab = RvlcDCT3Dtab0[167];
		break;

	case 0x3f7d:
		tab = RvlcDCT3Dtab0[168];
		break;

	default:  
#ifdef _DEBUG
		printf ("Invalid Huffman code in RvlcDecTCOEF().\n");
#endif
		return tcoef;
		break;
	}

    k_mp4_FlushBits (ppBitStream,pBitOffset,tab[1]);

	if (tab[0] != 7167)
	{
		tcoef.run = (tab[0] >> 8) & 255;
		tcoef.level = tab[0] & 255;
		tcoef.last = (tab[0] >> 16) & 1;
		/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);        */
		tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);        
	}
	else 	/* if (tab[0] == 7167) ESCAPE */
	{
		k_mp4_FlushBits (ppBitStream,pBitOffset,1);

		/*tcoef.last = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
		tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);

		/*tcoef.run = k_mp4_GetBits(ppBitStream, pBitOffset, 6);*/
		tcoef.run = k_mp4_GetBit6(ppBitStream, pBitOffset);

		k_mp4_FlushBits(ppBitStream, pBitOffset, 1);
		
		/*tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset, 11);*/
		tcoef.level = k_mp4_GetBit11(ppBitStream, pBitOffset);
		
		k_mp4_FlushBits(ppBitStream, pBitOffset, 1);      

		/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 5);*/
		tcoef.sign = k_mp4_GetBit5(ppBitStream, pBitOffset);
	}

	return tcoef;
}

Tcoef
RvlcDecodeInterTCOEF(Ipp8u **ppBitStream, int *pBitOffset)
{
	Ipp32u		code, mask; 
	Ipp32s		*tab;
	Tcoef		tcoef =	{0, 0, 0};

    int count, len;
 
    mask = 0x4000;      /* mask  100000000000000   */
    /*code = k_mp4_ShowBits (ppBitStream, pBitOffset,15);*/
    code = k_mp4_ShowBits15 (ppBitStream, pBitOffset);
    len = 1;
     //***use CLZ  --bnie 7/10/07
	if (code & mask) {
		count = 1;
		while (count > 0) {
			mask = mask >> 1;
			if (code & mask) 
				count--;
			len++;
		}
	}
	else {
		count = 2;
		while (count > 0) {
			mask = mask >> 1;
			if (!(code & mask))
				count--;
			len++;
		}
	}
    
    code = code & 0x7fff;
    code = code >> (15 - (len + 1));
    
	switch(code) {

	case 0x0:
		tab = RvlcDCT3Dtab1[169];
		break;

	case 0x1: 
		tab = RvlcDCT3Dtab1[1];
		break;

	case 0x4: 
		tab = RvlcDCT3Dtab1[2];
		break;

	case 0x5:
		tab = RvlcDCT3Dtab1[36];
		break;

	case 0x6:
		tab = RvlcDCT3Dtab1[0];
		break;

	case 0x7:
		tab = RvlcDCT3Dtab1[19];
		break;

	case 0x8:
		tab = RvlcDCT3Dtab1[43];
		break;

	case 0x9:
		tab = RvlcDCT3Dtab1[48];
		break;

	case 0xa:
		tab = RvlcDCT3Dtab1[29];
		break;

	case 0xb:
		tab = RvlcDCT3Dtab1[103];
		break;

	case 0xc:
		tab = RvlcDCT3Dtab1[20];
		break;

	case 0xd:
		tab = RvlcDCT3Dtab1[52];
		break;

	case 0x12:
		tab = RvlcDCT3Dtab1[108];
		break;

	case 0x13:
		tab = RvlcDCT3Dtab1[113];
		break;

	case 0x14:
		tab = RvlcDCT3Dtab1[56];
		break;

	case 0x15:
		tab = RvlcDCT3Dtab1[60];
		break;

	case 0x18:
		tab = RvlcDCT3Dtab1[116];
		break;

	case 0x19:
		tab = RvlcDCT3Dtab1[118];
		break;

	case 0x1c:
		tab = RvlcDCT3Dtab1[3];
		break;

	case 0x1d:
		tab = RvlcDCT3Dtab1[30];
		break;

	case 0x22:
		tab = RvlcDCT3Dtab1[120];
		break;

	case 0x23:
		tab = RvlcDCT3Dtab1[122];
		break;

	case 0x2c:
		tab = RvlcDCT3Dtab1[63];
		break;

	case 0x2d:
		tab = RvlcDCT3Dtab1[66];
		break;

	case 0x34:
		tab = RvlcDCT3Dtab1[68];
		break;

	case 0x35:
		tab = RvlcDCT3Dtab1[124];
		break;

	case 0x38:
		tab = RvlcDCT3Dtab1[126];
		break;

	case 0x39:
		tab = RvlcDCT3Dtab1[128];
		break;

	case 0x3c:
		tab = RvlcDCT3Dtab1[4];
		break;

	case 0x3d:
		tab = RvlcDCT3Dtab1[5];
		break;

	case 0x42:
		tab = RvlcDCT3Dtab1[130];
		break;

	case 0x43:
		tab = RvlcDCT3Dtab1[132];
		break;

	case 0x5c:
		tab = RvlcDCT3Dtab1[21];
		break;

	case 0x5d:
		tab = RvlcDCT3Dtab1[37];
		break;

	case 0x6c:
		tab = RvlcDCT3Dtab1[44];
		break;

	case 0x6d:
		tab = RvlcDCT3Dtab1[70];
		break;

	case 0x74:
		tab = RvlcDCT3Dtab1[72];
		break;

	case 0x75:
		tab = RvlcDCT3Dtab1[74];
		break;

	case 0x78:
		tab = RvlcDCT3Dtab1[104];
		break;

	case 0x79:
		tab = RvlcDCT3Dtab1[134];
		break;

	case 0x7c:
		tab = RvlcDCT3Dtab1[6];
		break;

	case 0x7d:
		tab = RvlcDCT3Dtab1[22];
		break;

	case 0x82:
		tab = RvlcDCT3Dtab1[136];
		break;

	case 0x83:
		tab = RvlcDCT3Dtab1[138];
		break;

	case 0xbc:
		tab = RvlcDCT3Dtab1[31];
		break;

	case 0xbd:
		tab = RvlcDCT3Dtab1[49];
		break;

	case 0xdc:
		tab = RvlcDCT3Dtab1[76];
		break;

	case 0xdd:
		tab = RvlcDCT3Dtab1[78];
		break;

	case 0xec:
		tab = RvlcDCT3Dtab1[80];
		break;

	case 0xed:
		tab = RvlcDCT3Dtab1[109];
		break;

	case 0xf4:
		tab = RvlcDCT3Dtab1[139];
		break;

	case 0xf5:
		tab = RvlcDCT3Dtab1[140];
		break;

	case 0xf8:
		tab = RvlcDCT3Dtab1[141];
		break;

	case 0xf9:
		tab = RvlcDCT3Dtab1[142];
		break;

	case 0xfc:
		tab = RvlcDCT3Dtab1[7];
		break;

	case 0xfd:
		tab = RvlcDCT3Dtab1[8];
		break;

	case 0x102:
		tab = RvlcDCT3Dtab1[143];
		break;

	case 0x103:
		tab = RvlcDCT3Dtab1[144];
		break;

	case 0x17c:
		tab = RvlcDCT3Dtab1[23];
		break;

	case 0x17d:
		tab = RvlcDCT3Dtab1[38];
		break;

	case 0x1bc:
		tab = RvlcDCT3Dtab1[53];
		break;

	case 0x1bd:
		tab = RvlcDCT3Dtab1[57];
		break;

	case 0x1dc:
		tab = RvlcDCT3Dtab1[61];
		break;

	case 0x1dd:
		tab = RvlcDCT3Dtab1[64];
		break;

	case 0x1ec:
		tab = RvlcDCT3Dtab1[82];
		break;

	case 0x1ed:
		tab = RvlcDCT3Dtab1[83];
		break;

	case 0x1f4:
		tab = RvlcDCT3Dtab1[84];
		break;

	case 0x1f5:
		tab = RvlcDCT3Dtab1[145];
		break;

	case 0x1f8:
		tab = RvlcDCT3Dtab1[146];
		break;

	case 0x1f9:
		tab = RvlcDCT3Dtab1[147];
		break;

	case 0x1fc:
		tab = RvlcDCT3Dtab1[9];
		break;

	case 0x1fd:
		tab = RvlcDCT3Dtab1[10];
		break;

	case 0x202:
		tab = RvlcDCT3Dtab1[148];
		break;

	case 0x203:
		tab = RvlcDCT3Dtab1[149];
		break;

	case 0x2fc:
		tab = RvlcDCT3Dtab1[24];
		break;

	case 0x2fd:
		tab = RvlcDCT3Dtab1[32];
		break;

	case 0x37c:
		tab = RvlcDCT3Dtab1[45];
		break;

	case 0x37d:
		tab = RvlcDCT3Dtab1[50];
		break;

	case 0x3bc:
		tab = RvlcDCT3Dtab1[67];
		break;

	case 0x3bd:
		tab = RvlcDCT3Dtab1[85];
		break;

	case 0x3dc:
		tab = RvlcDCT3Dtab1[86];
		break;

	case 0x3dd:
		tab = RvlcDCT3Dtab1[87];
		break;

	case 0x3ec:
		tab = RvlcDCT3Dtab1[88];
		break;

	case 0x3ed:
		tab = RvlcDCT3Dtab1[89];
		break;

	case 0x3f4:
		tab = RvlcDCT3Dtab1[90];
		break;

	case 0x3f5:
		tab = RvlcDCT3Dtab1[105];
		break;

	case 0x3f8:
		tab = RvlcDCT3Dtab1[114];
		break;

	case 0x3f9:
		tab = RvlcDCT3Dtab1[150];
		break;

	case 0x3fc:
		tab = RvlcDCT3Dtab1[11];
		break;

	case 0x3fd:
		tab = RvlcDCT3Dtab1[25];
		break;

	case 0x402:
		tab = RvlcDCT3Dtab1[151];
		break;

	case 0x403:
		tab = RvlcDCT3Dtab1[152];
		break;

	case 0x5fc:
		tab = RvlcDCT3Dtab1[33];
		break;

	case 0x5fd:
		tab = RvlcDCT3Dtab1[39];
		break;

	case 0x6fc:
		tab = RvlcDCT3Dtab1[54];
		break;

	case 0x6fd:
		tab = RvlcDCT3Dtab1[58];
		break;

	case 0x77c:
		tab = RvlcDCT3Dtab1[69];
		break;

	case 0x77d:
		tab = RvlcDCT3Dtab1[91];
		break;

	case 0x7bc:
		tab = RvlcDCT3Dtab1[92];
		break;

	case 0x7bd:
		tab = RvlcDCT3Dtab1[93];
		break;

	case 0x7dc:
		tab = RvlcDCT3Dtab1[110];
		break;

	case 0x7dd:
		tab = RvlcDCT3Dtab1[117];
		break;

	case 0x7ec:
		tab = RvlcDCT3Dtab1[119];
		break;

	case 0x7ed:
		tab = RvlcDCT3Dtab1[153];
		break;

	case 0x7f4:
		tab = RvlcDCT3Dtab1[154];
		break;

	case 0x7f5:
		tab = RvlcDCT3Dtab1[155];
		break;

	case 0x7f8:
		tab = RvlcDCT3Dtab1[156];
		break;

	case 0x7f9:
		tab = RvlcDCT3Dtab1[157];
		break;

	case 0x7fc:
		tab = RvlcDCT3Dtab1[12];
		break;

	case 0x7fd:
		tab = RvlcDCT3Dtab1[13];
		break;

	case 0x802:
		tab = RvlcDCT3Dtab1[158];
		break;

	case 0x803:
		tab = RvlcDCT3Dtab1[159];
		break;

	case 0xbfc:
		tab = RvlcDCT3Dtab1[14];
		break;

	case 0xbfd:
		tab = RvlcDCT3Dtab1[15];
		break;

	case 0xdfc:
		tab = RvlcDCT3Dtab1[26];
		break;

	case 0xdfd:
		tab = RvlcDCT3Dtab1[40];
		break;

	case 0xefc:
		tab = RvlcDCT3Dtab1[46];
		break;

	case 0xefd:
		tab = RvlcDCT3Dtab1[51];
		break;

	case 0xf7c:
		tab = RvlcDCT3Dtab1[62];
		break;

	case 0xf7d:
		tab = RvlcDCT3Dtab1[71];
		break;

	case 0xfbc:
		tab = RvlcDCT3Dtab1[94];
		break;

	case 0xfbd:
		tab = RvlcDCT3Dtab1[95];
		break;

	case 0xfdc:
		tab = RvlcDCT3Dtab1[96];
		break;

	case 0xfdd:
		tab = RvlcDCT3Dtab1[97];
		break;

	case 0xfec:
		tab = RvlcDCT3Dtab1[106];
		break;

	case 0xfed:
		tab = RvlcDCT3Dtab1[121];
		break;

	case 0xff4:
		tab = RvlcDCT3Dtab1[123];
		break;

	case 0xff5:
		tab = RvlcDCT3Dtab1[125];
		break;

	case 0xff8:
		tab = RvlcDCT3Dtab1[127];
		break;

	case 0xff9:
		tab = RvlcDCT3Dtab1[129];
		break;

	case 0xffc:
		tab = RvlcDCT3Dtab1[16];
		break;

	case 0xffd:
		tab = RvlcDCT3Dtab1[17];
		break;

	case 0x1002:
		tab = RvlcDCT3Dtab1[160];
		break;

	case 0x1003:
		tab = RvlcDCT3Dtab1[161];
		break;

	case 0x17fc:
		tab = RvlcDCT3Dtab1[27];
		break;

	case 0x17fd:
		tab = RvlcDCT3Dtab1[28];
		break;

	case 0x1bfc:
		tab = RvlcDCT3Dtab1[34];
		break;

	case 0x1bfd:
		tab = RvlcDCT3Dtab1[35];
		break;

	case 0x1dfc:
		tab = RvlcDCT3Dtab1[41];
		break;

	case 0x1dfd:
		tab = RvlcDCT3Dtab1[55];
		break;

	case 0x1efc:
		tab = RvlcDCT3Dtab1[65];
		break;

	case 0x1efd:
		tab = RvlcDCT3Dtab1[73];
		break;

	case 0x1f7c:
		tab = RvlcDCT3Dtab1[75];
		break;

	case 0x1f7d:
		tab = RvlcDCT3Dtab1[77];
		break;

	case 0x1fbc:
		tab = RvlcDCT3Dtab1[79];
		break;

	case 0x1fbd:
		tab = RvlcDCT3Dtab1[98];
		break;

	case 0x1fdc:
		tab = RvlcDCT3Dtab1[99];
		break;

	case 0x1fdd:
		tab = RvlcDCT3Dtab1[100];
		break;

	case 0x1fec:
		tab = RvlcDCT3Dtab1[107];
		break;

	case 0x1fed:
		tab = RvlcDCT3Dtab1[111];
		break;

	case 0x1ff4:
		tab = RvlcDCT3Dtab1[131];
		break;

	case 0x1ff5:
		tab = RvlcDCT3Dtab1[133];
		break;

	case 0x1ff8:
		tab = RvlcDCT3Dtab1[135];
		break;

	case 0x1ff9:
		tab = RvlcDCT3Dtab1[162];
		break;

	case 0x1ffc:
		tab = RvlcDCT3Dtab1[18];
		break;

	case 0x1ffd:
		tab = RvlcDCT3Dtab1[42];
		break;

	case 0x2002:
		tab = RvlcDCT3Dtab1[163];
		break;

	case 0x2003:
		tab = RvlcDCT3Dtab1[164];
		break;

	case 0x2ffc:
		tab = RvlcDCT3Dtab1[47];
		break;

	case 0x2ffd:
		tab = RvlcDCT3Dtab1[59];
		break;

	case 0x37fc:
		tab = RvlcDCT3Dtab1[81];
		break;

	case 0x37fd:
		tab = RvlcDCT3Dtab1[101];
		break;

	case 0x3bfc:
		tab = RvlcDCT3Dtab1[102];
		break;

	case 0x3bfd:
		tab = RvlcDCT3Dtab1[112];
		break;

	case 0x3dfc:
		tab = RvlcDCT3Dtab1[115];
		break;

	case 0x3dfd:
		tab = RvlcDCT3Dtab1[137];
		break;

	case 0x3efc:
		tab = RvlcDCT3Dtab1[165];
		break;

	case 0x3efd:
		tab = RvlcDCT3Dtab1[166];
		break;

	case 0x3f7c:
		tab = RvlcDCT3Dtab1[167];
		break;

	case 0x3f7d:
		tab = RvlcDCT3Dtab1[168];
		break;

	default:  
#ifdef _DEBUG
		printf ("Invalid Huffman code in RvlcDecTCOEF().\n");
#endif
		return tcoef;
		break;
	}

    k_mp4_FlushBits (ppBitStream,pBitOffset,tab[1]);
	
	if (tab[0] != 7167)
	{
		tcoef.run = (tab[0] >> 8) & 255;
		tcoef.level = tab[0] & 255;
		tcoef.last = (tab[0] >> 16) & 1;
		/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);        */
		tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);        
	}
	else 	/* if (tab[0] == 7167) ESCAPE */
	{
		k_mp4_FlushBits (ppBitStream,pBitOffset,1);
		/*tcoef.last = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
		tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);
		/*tcoef.run = k_mp4_GetBits(ppBitStream, pBitOffset, 6);*/
		tcoef.run = k_mp4_GetBit6(ppBitStream, pBitOffset);

		k_mp4_FlushBits(ppBitStream, pBitOffset, 1);

		/*tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset, 11)*/;
		tcoef.level = k_mp4_GetBit11(ppBitStream, pBitOffset);

		k_mp4_FlushBits(ppBitStream, pBitOffset, 1);      

		/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 5);*/
		tcoef.sign = k_mp4_GetBit5(ppBitStream, pBitOffset);
	}

	return tcoef;

}



IppStatus __STDCALL 
ippiReconstructCoeffsIntra_H263_1u16s_c(
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     cbp,
  int     QP,
  int     advIntraFlag,
  int     scan,
  int     modQuantFlag)
{
	Ipp32s              i;
	Tcoef               run_level;
	Ipp32u				DC_coeff;
	Ipp32s				tmpCoef;
	const Ipp32s		*pZigzag = zigzag[scan+1];
	const int			QP_1 = QP <<1;
	const int			QP_2 = QP & 1 ? QP : (QP - 1);

	if (!ppBitStream || !pBitOffset || !pCoef)
		return ippStsNullPtrErr;

	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;
	
	if (QP < 1 || QP > 31)
		return ippStsQPErr;

#ifdef PRINT_REF_INFO
		{
			dump("ippiReconstructCoeffsIntra_H263_1u16s		with cbp(0,!0): ",cbp);
			dump("ippiReconstructCoeffsIntra_H263_1u16s		with scan(0,1,2): ",scan);
		}
#endif

	/* in this Intel sample, 
	  mp4decvop.c line # 791  and  mp4dec.h line # 410 use this function
	  and the modQuantFlag, advIntraFlag are both set with 0 obviously.
	  I think modQuantFlag and advIntraFlag are used in H.263, not mpeg4
	  short video header mode*/
	if (modQuantFlag || advIntraFlag)
	{
#ifdef _DEBUG
		printf("modQuantFlag or advIntraFlag is not used in short video header mode of mpeg4\n");
#endif 
		return ippStsErr;
	}

	/*DC_coeff = k_mp4_GetBits(ppBitStream, pBitOffset, 8);*/
	DC_coeff = k_mp4_GetBit8(ppBitStream, pBitOffset);

	if (DC_coeff == 128 || DC_coeff == 0) 
	{
#ifdef _DEBUG
		printf ("short header: Illegal DC coeff: 0 or 128\n");
#endif
		*pIndxLastNonZero = -1;
		return ippStsVLCErr;
	}
	if (DC_coeff == 255)
		DC_coeff = 128;

	pCoef[0] = DC_coeff << 3;	/* DC_coeff * 8 */
	pCoef[0] = CLIP_R(pCoef[0],2047,-2048);	/*	(1 << (8 + 3)); input parameter don't give us bitsperpixel, so we use 8 */

	if (0 == cbp)	/* only return the intra DC */
	{
		*pIndxLastNonZero = 0;
		return ippStsNoErr;
	}

	/* other intra TCoeffs */
	memset(pCoef+1,0,63*sizeof(Ipp16s));

	i = 1;
	do
	{
		run_level = VlcDecodeInterTCOEF(ppBitStream,pBitOffset,1);

		if ((i+= run_level.run) > 63)
		{
#ifdef _DEBUG
			printf ("Too Much AC Coesffs in short header Intra block!\n");
#endif
			*pIndxLastNonZero = -1;
			return ippStsVLCErr;
		}

		/*	inverse scan & inverse quant */
		tmpCoef = QP_1 * run_level.level + QP_2;

		if (run_level.sign)
			tmpCoef = -tmpCoef;

		pCoef[pZigzag[i]] = CLIP_R(tmpCoef,2047,-2048);	/*	(1 << (8 + 3)); input parameter don't give us bitsperpixel, so we use 8 */

		i++;
	}while (!run_level.last);

	*pIndxLastNonZero = i-1;

	return ippStsNoErr;
}


IppStatus __STDCALL 
ippiReconstructCoeffsInter_H263_1u16s_c(
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     QP,
  int     modQuantFlag)
{
	Ipp32s              i;
	Tcoef               run_level;
	Ipp32s				tmpCoef;
	const Ipp32s		*pZigzag = zigzag[1];
	const int			QP_1 = QP << 1;
	const int			QP_2 = (QP & 1) ? QP : (QP - 1);

	if (!ppBitStream || !pBitOffset || !pCoef)
		return ippStsNullPtrErr;

	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;
	
	if (QP < 1 || QP > 31)
		return ippStsQPErr;

#ifdef PRINT_REF_INFO
	dump("ippiReconstructCoeffsInter_H263_1u16s", -1);
#endif

	/* in this Intel sample, 
		mp4dec.h line #428 line #503 and mp4decvop.c line #816
	  the modQuantFlag is set with 0 obviously.
	  I think modQuantFlag are used in H.263, not mpeg4
	  short video header mode*/
	if (modQuantFlag)
	{
#ifdef _DEBUG
		printf("modQuantFlag is not used in short video header mode of mpeg4\n");
#endif 
		return ippStsErr;
	}

	memset(pCoef,0,64*sizeof(Ipp16s));

	i = 0;
	do
	{
		run_level = VlcDecodeInterTCOEF(ppBitStream,pBitOffset,1);

		if ((i+= run_level.run) > 63)
		{
#ifdef _DEBUG
			printf ("Too Much AC Coesffs in short header Inter block!\n");
#endif
			*pIndxLastNonZero = -1;
			return ippStsVLCErr;
		}

		tmpCoef = QP_1 * run_level.level + QP_2;

		if (run_level.sign)
			tmpCoef = -tmpCoef;

		pCoef[pZigzag[i]] = CLIP_R(tmpCoef,2047,-2048);	/*	(1 << (8 + 3)); input parameter don't give us bitsperpixel, so we use 8 */

		i++;
	}while (!run_level.last);

	*pIndxLastNonZero = i-1;

	return ippStsNoErr;
}



IppStatus __STDCALL 
ippiDecodeDCIntra_MPEG4_1u16s_c(Ipp8u **ppBitStream, int *pBitOffset,	
							  Ipp16s *pDC, int blockType)
{
	Ipp32u dc_size;
	Ipp32u code;
	int first_bit;

	if (!ppBitStream || !pBitOffset)
		return ippStsNullPtrErr;

	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;

#ifdef PRINT_REF_INFO	
	dump("ippiDecodeDCIntra_MPEG4_1u16s", -1);
#endif

	/* read DC size 2 - 8 bits */
	dc_size = VlcDecIntraDCPredSize(ppBitStream, pBitOffset, blockType);

	if (dc_size == 0)
	{
		*pDC = 0;
	}
	else
	{
		/* read delta DC 0 - 8 bits */
		code = k_mp4_GetBits(ppBitStream, pBitOffset, dc_size);

		first_bit = code >> (dc_size-1);
		if (first_bit == 0 )
		{ /* negative delta INTRA DC */
			*pDC  = -1*(code ^ ((1 << dc_size) - 1));
		}
		else
		{ /* positive delta INTRA DC */
			*pDC = code;
		}
		if (dc_size > 8)
		{
#ifdef _DEBUG
			printf("dc_size > 8 in ippiDecodeDCIntra_MPEG4_1u16s\n\n");
#endif
			(void) k_mp4_FlushBits(ppBitStream, pBitOffset, 1);
		}
	}

	return ippStsNoErr;
}



/*
DecodeCoeffsIntra_MPEG4
Decodes DCT coefficients for intra coded block.
*/
IppStatus __STDCALL 
ippiDecodeCoeffsIntra_MPEG4_1u16s_c(						
    Ipp8u**  ppBitStream,
    int*     pBitOffset,
    Ipp16s*  pCoeffs,
    int*     pIndxLastNonZero,
    int      rvlcFlag,
    int      noDCFlag,
    int      scan)
{
	Ipp32s              i;
	Tcoef               run_level;
	const Ipp32s        *pZigzag = zigzag[scan+1];

	if (!ppBitStream || !pBitOffset || !pCoeffs)
		return ippStsNullPtrErr;
	
	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;

	/* for inverse scan */

#ifdef PRINT_REF_INFO
	{
			dump("ippiDecodeCoeffsIntra_MPEG4_1u16s		with rvlcFlag(0,1): ",rvlcFlag);
			dump("ippiDecodeCoeffsIntra_MPEG4_1u16s		with scan(0,1,2): ",scan);
	}
#endif

	memset(pCoeffs+noDCFlag, 0, (64-noDCFlag)*sizeof(Ipp16s));

	i = noDCFlag;
	if(!rvlcFlag)	/* vlc decoding intra, and inverse scan */
	{	
		do
		{
			run_level = VlcDecodeIntraTCOEF(ppBitStream,pBitOffset);

			if ((i += run_level.run) > 63)	/* i >= 64 VLC error */
			{
#ifdef _DEBUG
				printf ("Too Much AC Coesffs in Intra block!");
#endif
				return ippStsVLCErr;
			}
			if (run_level.sign)		/* == 1 */
			{
				pCoeffs[pZigzag[i]] = -run_level.level;
			}
			else
			{
				pCoeffs[pZigzag[i]] = run_level.level;
			}

			i++;
		}
		while (!run_level.last);

		*pIndxLastNonZero = i-1;
	}
	else		/* RVLC */
	{
		do
		{
			run_level = RvlcDecodeIntraTCOEF(ppBitStream, pBitOffset);

			if ((i += run_level.run) > 63)	/* i >= 64 VLC error */
			{
#ifdef _DEBUG
				printf ("Too Much AC Coesffs in Intra block!");
#endif
				return ippStsVLCErr;
			}
			if (run_level.sign)		/* == 1 */
			{
				pCoeffs[pZigzag[i]] = -run_level.level;
			}
			else
			{
				pCoeffs[pZigzag[i]] = run_level.level;
			}
			i++;
		}
		while (!run_level.last);

		*pIndxLastNonZero = i-1;
	}

	return ippStsNoErr;
}


/*
ReconstructCoeffsInter_MPEG4
Decodes DCT coefficients, performs inverse scan and
inverse quantization for inter coded block.
*/
IppStatus __STDCALL 
ippiReconstructCoeffsInter_MPEG4_1u16s_c(			
    Ipp8u**                            ppBitStream,
    int*                               pBitOffset,
    Ipp16s*                            pCoeffs,
    int*                               pIndxLastNonZero,
    int                                rvlcFlag,
    int                                scan,
    const IppiQuantInvInterSpec_MPEG4* pQuantInvInterSpec,
    int                                QP)
{
	Ipp32s              i;
	int sum;
	Tcoef               run_level;
	Ipp32s				tmpCoef;
	const Ipp32s		*pZigzag = zigzag[scan+1];
	const int			QP_1 = QP <<1;
	const int			QP_2 = QP & 1 ? QP : (QP - 1);


	k_IppiQuantInvInterSpec_MPEG4 * pQuant = (k_IppiQuantInvInterSpec_MPEG4*)pQuantInvInterSpec;

	if (!ppBitStream || !pBitOffset || !pCoeffs || !pQuantInvInterSpec)
		return ippStsNullPtrErr;
	
	if (QP < 1 || QP > pQuant->quant_max)
		return ippStsQPErr;

	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;

	/* for inverse scan */
	sum=0;

#ifdef PRINT_REF_INFO
		{
			dump("ippiReconstructCoeffsInter_MPEG4_1u16s		with rvlcFlag(0,1): ",rvlcFlag);
			dump("ippiReconstructCoeffsInter_MPEG4_1u16s		with quant_type(0,1): ",pQuant->bUseMat);
			dump("ippiReconstructCoeffsInter_MPEG4_1u16s		with scan(0,1,2): ",scan);
		}
#endif

	memset(pCoeffs,0,64*sizeof(Ipp16s));

	i = 0;
	if(!rvlcFlag)	/* vlc decoding inter, and inverse scan , inverse quant*/
	{
		if (pQuant->bUseMat)	/*1 == quant_type */
		{
			do
			{
				run_level = VlcDecodeInterTCOEF(ppBitStream,pBitOffset,0);

				if ((i += run_level.run) > 63)	/* i >= 64 VLC error */
				{
#ifdef _DEBUG
					printf ("Too Much Coesffs in Inter block!\n");
#endif		
					return ippStsVLCErr;
				}

				tmpCoef = (((run_level.level<<1) + 1) * pQuant->matrix[pZigzag[i]] * QP) >> 4;	/* /16 */

				if (run_level.sign) 
					tmpCoef = -tmpCoef;

				tmpCoef = CLIP_R(tmpCoef,pQuant->max,pQuant->min);
				sum += tmpCoef;
				pCoeffs[pZigzag[i]] = tmpCoef;

				/*pCoeffs[pZigzag[i]] = CLIP_R(tmpCoef,pQuant->max,pQuant->min);
				sum += pCoeffs[pZigzag[i]];*/

				i++;
			}while (!run_level.last);
		}
		else		/*0 == quant_type */
		{
			do
			{
				run_level = VlcDecodeInterTCOEF(ppBitStream,pBitOffset,0);

				if ((i += run_level.run) > 63)	/* i >= 64 VLC error */
				{
#ifdef _DEBUG
					printf ("Too Much Coesffs in Inter block!\n");
#endif		
					return ippStsVLCErr;
				}
				tmpCoef = QP_1 * run_level.level + QP_2;

				if (run_level.sign)
					tmpCoef = -tmpCoef;

				pCoeffs[pZigzag[i]] = CLIP_R(tmpCoef,pQuant->max,pQuant->min);

				i++;	
			}while (!run_level.last);
		}
		*pIndxLastNonZero = i-1;
	}
	else		/*RVLC*/
	{
		if (pQuant->bUseMat)	/*1 == quant_type */
		{
			do
			{
				run_level = RvlcDecodeInterTCOEF(ppBitStream,pBitOffset);

				if ((i += run_level.run) > 63)	/* i >= 64 VLC error */
				{
#ifdef _DEBUG
					printf ("Too Much Coesffs in Inter block!");
#endif		
					return ippStsVLCErr;
				}
				tmpCoef = (((run_level.level<<1) + 1) * pQuant->matrix[pZigzag[i]] * QP) >> 4;	/* /16 */

				if (run_level.sign) 
					tmpCoef = -tmpCoef;

				tmpCoef = CLIP_R(tmpCoef,pQuant->max,pQuant->min);
				sum += tmpCoef;
				pCoeffs[pZigzag[i]] = tmpCoef;

				/*pCoeffs[pZigzag[i]] = CLIP_R(tmpCoef,pQuant->max,pQuant->min);
				sum += pCoeffs[pZigzag[i]];*/

				i++;
			}while (!run_level.last);
		}
		else	/*0 == quant_type */
		{
			do
			{
				run_level = RvlcDecodeInterTCOEF(ppBitStream,pBitOffset);

				if ((i += run_level.run) > 63)	/* i >= 64 VLC error */
				{
#ifdef _DEBUG
					printf ("Too Much Coesffs in Inter block!");
#endif		
					return ippStsVLCErr;
				}

				tmpCoef = QP_1 * run_level.level + QP_2;

				if (run_level.sign)
					tmpCoef = -tmpCoef;

				pCoeffs[pZigzag[i]] = CLIP_R(tmpCoef,pQuant->max,pQuant->min);

				i++;
			}while (!run_level.last);
		}
		*pIndxLastNonZero = i-1;
	}

	/* inverse quantization */
	if (pQuant->bUseMat)	 /*1 == quant_type*/
	{
		/*mismatch control*/
		if ((sum & 0x1) == 0) 
			pCoeffs[63] += ((pCoeffs[63] & 0x1)? -1:1); 
	}
	return ippStsNoErr;
}

