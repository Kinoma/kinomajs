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
#ifndef KINOMA_MPEG4_ASSISTANT_H_CSQ_20060418
#define KINOMA_MPEG4_ASSISTANT_H_CSQ_20060418

/*--------------------------------------------look here firstly---------------/ 
 * File Name : kinoma_mpeg4_assistant.h
 * Function  : some structures, tables and functions
 * Date      : 2006-04-18
 * Author    : Cheng Shiquan
 * Version   : 0.1 
-----------------------------------------------thank you---------------------*/
#include "stdio.h"
//#include "malloc.h"
#include "memory.h"
#include "math.h"
#include "ippvc.h"


#define SIGN(x) ((x)>=0? 1:-1)
#define CLIP(x) ((x)>255? 255:((x)<0?0:(x)))
#define  MIN(a,b)   (((a) < (b)) ? (a) : (b))
#define  MAX(a,b)   (((a) > (b)) ? (a) : (b))
#define CLIP_R(x,max,min) ((x)>(max)?(max):((x)<(min)?(min):(x)))

/* for quarter pel calc */

/*#define FILTER(tmp1,tmp2,tmp3,tmp4,round) \
		( (((tmp1)<<7) + ((tmp1)<<5)) - (((tmp2)<<5) + ((tmp2)<<4)) + (((tmp3)<<4) + ((tmp3)<<3)) - ((tmp4) << 3) + (round)) >> 8
*/

/*#define FILTER(T1,T2,T3,T4,R) \
		( ((T1)<<7) + (((T1)-(T2))<<5) + (((T3)-(T2))<<4) + (((T3)-(T4))<< 3) + (R)) >> 8
*/

/*#define FILTER(T1,T2,T3,T4,R) \
		(  (  ((((T1)<<4) + ((T1)<<2)) - (((T2)<<2) + ((T2)<<1)) + (((T3)<<1) + (T3)) - (T4) )<<3) + (R) ) >> 8
*/

/*#define FILTER(T1,T2,T3,T4,R) \
		(  ((((T1)<<4) + (((T1)-(T2))<<2) + (((T3)-(T2))<<1) + (T3)-(T4))<<3) + (R) ) >> 8
*/

#define FILTER(T1,T2,T3,T4,R) \
		(  (T1)*160 - (T2)* 48 +(T3) *24 - (T4) * 8 + (R) ) >> 8

/*#define FILTER(T1,T2,T3,T4,R) \
		(  (((T1)*20 - (T2)* 6 +(T3) *3 - (T4)) * 8) + (R) ) >> 8
*/



/* event structure for transform coeffs */
typedef struct 
{
  Ipp32s last, run, level, sign;
} Tcoef;

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
}k_IppiQuantInvInterSpec_MPEG4;

/**************************************************************
* my IppiQuantInvIntraSpec_MPEG4 structure					  *
* the size of Intel's is 156 bytes							  *
**************************************************************/
typedef struct
{
	int quant_max;
	int min, max;
	int bUseMat;
	Ipp8u matrix[64];
}k_IppiQuantInvIntraSpec_MPEG4;

/**************************************************************
* my IppiWarpSpec_MPEG4 structure							*
* the size of Intel's is 384 bytes							*
**************************************************************/
typedef struct
{
	/* reference points all */
	int vop_i0,vop_j0,vop_i1,vop_j1,vop_i2,vop_j2,vop_i3,vop_j3;
	int spr_i0,spr_j0,spr_i1,spr_j1,spr_i2,spr_j2,spr_i3,spr_j3;
	int spr_vi1, spr_vj1, spr_vi2, spr_vj2;

	/* Width, height, and virtual Width, height */
	int W, H, VW, VH;  /* VW, VH is for virtual sprite points */

	int minYW,maxYW,minCW,maxCW;	/* clip for warping points */
	int minYH,maxYH,minCH,maxCH;

	/* power for W, H, R*/
	int alpha, beta,rou;
	
	/* s*/
	int s,pwr_s,ps_round,s_pwr,ps_p, mv_scaler, mv_half, mv_denom;
	
	int min, max;

	int ic,jc; /* 2*vop_i0 - 1,    2*vop_j0 - 1*/

	int pwr_aArM1,aAr,aArA2;	/* 2pow(alph+rou-1),  (alpha+rou)  (alpha+rou+2) */

	/* for sprite_warping_points == 0  chroma*/

	/* for sprite_warping_points == 1 luma*/

	/* for sprite_warping_points == 1 and static  chroma*/
	int wpt1_1, wpt1_2;
	/* for sprite_warping_points == 1 and GMC  chroma*/

	/* for sprite_warping_points == 2 */
	int wpt2_1, wpt2_2, wpt2_3;	/* (-r*spr_i0+spr_vi1),  (r*spr_j0-spr_vj1), (-r*spr_j0+spr_vj1), */

	/* for sprite_warping_points == 2 chroma*/
	int wpt2_4;					/* (2*VW*r*spr_i0 -  16*VW + 2pow(alpha+rou+1))  */
	int wpt2_5;					/* (2*VW*r*spr_j0 -  16*VW + 2pow(alpha+rou+1))  */

	/* for sprite_warping_points == 3 */
	int wpt3_1, wpt3_2;		/* (-r*spr_i0+spr_vi1), (-r*spr_i0_spr_vi2)*2pow(alpha-beta)*/
	int wpt3_3, wpt3_4;		/* (-r*spr_j0+spr_vj1), (-r*spr_j0_spr_vj2)*2pow(alpha-beta)*/

	/* for sprite_warping_points == 3 chroma*/
	int wpt3_5;			/* (2*VW*r*spr_i0-16*VW+2pow(alpha+rou+1)) */
	int wpt3_6;			/* (2*VW*r*spr_j0-16*VW+2pow(alpha+rou+1)) */

	/* for sprite_warping_points == 4 */
	int g;		/* ((spr_i0-spr_i1-spr_i2+spr_i3)(spr_j2-spr_j3) - (spr_i2-spr_i3)(spr_j0-spr_j1-spr_j2+spr_j3)) *H  */
	int h;		/* ((spr_i1-spr_i3)(spr_j0-spr_j1-spr_j2+spr_j3) - (spr_i0-spr_i1-spr_i2+spr_i3)(spr_j1-spr_j3)) *W  */
	int D;		/* (spr_i1-spr_i3) (spr_j2-spr_j3) - (spr_i2-spr_i3)(spr_j1-spr_j3) */
	int a;		/* D(spr_i1 - spr_i0) H + g*spr_i1 */
	int b;		/* D(spr_i2 - spr_i0) W + h*spr_i2 */
	int c;		/* D*spr_i0*W*H  */
	int d;		/* D(spr_j1-spr_j0) H + g*spr_j1  */
	int e;		/* D(spr_j2-spr_j0) W + h*spr_j2  */
	int f;		/* D*spr_j0*W*H */


	/*	input data */
    int                 numWarpingPoints;
    int                 spriteType;

}k_IppiWarpSpec_MPEG4;


/* tables for inter luminance blocks */
static Ipp32s DCT3Dtab0[][2] = 
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

static Ipp32s DCT3Dtab1[][2] = 
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

static Ipp32s DCT3Dtab2[][2] = 
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

/* RVLC  table from VM*/
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


/* OBMC_H tables Ipp8u*/
static int obmch0[64] =
{
    4,5,5,5,5,5,5,4,
    5,5,5,5,5,5,5,5,
    5,5,6,6,6,6,5,5,
    5,5,6,6,6,6,5,5,
    5,5,6,6,6,6,5,5,
    5,5,6,6,6,6,5,5,
    5,5,5,5,5,5,5,5,
    4,5,5,5,5,5,5,4
};

static int obmch1[64] = 
{
	2,2,2,2,2,2,2,2,
    1,1,2,2,2,2,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,2,2,2,2,1,1,
    2,2,2,2,2,2,2,2

};

static int obmch2[64] = 
{
    2,1,1,1,1,1,1,2,
    2,2,1,1,1,1,2,2,
    2,2,1,1,1,1,2,2,
    2,2,1,1,1,1,2,2,
    2,2,1,1,1,1,2,2,
    2,2,1,1,1,1,2,2,
    2,2,1,1,1,1,2,2,
    2,1,1,1,1,1,1,2
};
/* obmc tables end*/

/*-----------------------------------------------------------------------------
*		bitstream functions
*----------------------------------------------------------------------------*/

static Ipp32u __inline k_mp4_ShowBits(Ipp8u **ppBitStream,int *pBitOffset, int n)
{
    Ipp8u* ptr = *ppBitStream;
    Ipp32u tmp = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] <<  8) | (ptr[3]);
    tmp <<= *pBitOffset;
    tmp >>= 32 - n;
    return tmp;

};

static Ipp32u __inline  k_mp4_GetBits(Ipp8u **ppBitStream, int *pBitOffset, int n)
{
    Ipp8u* ptr = *ppBitStream;
    register Ipp32u tmp = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] <<  8) | (ptr[3]);
    tmp <<= *pBitOffset;
    tmp >>= 32 - n;
    n = n + *pBitOffset;
    *ppBitStream += n >> 3;
    *pBitOffset = n & 7;
    return tmp;
};

static void __inline  k_mp4_FlushBits(Ipp8u **ppBitStream, int *pBitOffset, int n)
{
    register m = n + *pBitOffset;
    *ppBitStream += m >> 3;
    *pBitOffset = m & 7;
};

static Ipp32u __inline k_mp4_ShowBits11(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 16) | ((*ppBitStream)[1]<<8) | (*ppBitStream)[2];
	tmp >>= (13-*pBitOffset);
	return (tmp & 2047);
}

static Ipp32u __inline k_mp4_ShowBits12(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 16) | ((*ppBitStream)[1]<<8) | (*ppBitStream)[2];
	tmp >>= (12-*pBitOffset);
	return (tmp & 4095);
}

static Ipp32u __inline k_mp4_ShowBits15(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 16) | ((*ppBitStream)[1]<<8) | (*ppBitStream)[2];
	tmp >>= (9-*pBitOffset);
	return (tmp & 32767);
}

static Ipp32u __inline k_mp4_GetBit1(Ipp8u **ppBitStream,int *pBitOffset)
{
	//Ipp32u tmp = (*ppBitStream)[0];
	//register int n = 1 + *pBitOffset;
	//tmp >>= (7-*pBitOffset);

	//*ppBitStream += n >> 3;
	//*pBitOffset = n & 7;
 //
	//return (tmp & 1);

    register Ipp32u tmp = *ppBitStream[0];
    if (*pBitOffset == 7)
	{
        *pBitOffset = 0;
        (*ppBitStream)++;
    }
	else 
	{
        tmp >>= 7 - *pBitOffset;
        (*pBitOffset)++;
    }
    return (tmp & 1);
}

static Ipp32u __inline k_mp4_GetBit2(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 8) | (*ppBitStream)[1];
	register int n = 2 + *pBitOffset;
	tmp >>= (14-*pBitOffset);

	*ppBitStream += n >> 3;
	*pBitOffset = n & 7;
 
	return (tmp & 3);
}

static Ipp32u __inline k_mp4_GetBit5(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 8) | (*ppBitStream)[1];
	register int n = 5 + *pBitOffset;
	tmp >>= (11-*pBitOffset);

	*ppBitStream += n >> 3;
	*pBitOffset = n & 7;
	
	return (tmp & 31);
}

static Ipp32u __inline k_mp4_GetBit6(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 8) | (*ppBitStream)[1];
	register int n = 6 + *pBitOffset;
	tmp >>= (10-*pBitOffset);

	*ppBitStream += n >> 3;
	*pBitOffset = n & 7;
 
	return (tmp & 63);
}

static Ipp32u __inline k_mp4_GetBit8(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 8) | (*ppBitStream)[1];
	register int n = 8 + *pBitOffset;
	tmp >>= (8-*pBitOffset);

	*ppBitStream += 1;
	*pBitOffset = n & 7;
 
	return (tmp & 255);
}

static Ipp32u __inline k_mp4_GetBit11(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 16) | ((*ppBitStream)[1]<<8) | (*ppBitStream)[2];
	register int n = 11 + *pBitOffset;
	tmp >>= (13-*pBitOffset);

	*ppBitStream += n >> 3;
	*pBitOffset = n & 7;
 
	return (tmp & 2047);
}

static Ipp32u __inline k_mp4_GetBit12(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 16) | ((*ppBitStream)[1]<<8) | (*ppBitStream)[2];
	register int n = 12 + *pBitOffset;
	tmp >>= (12-*pBitOffset);

	*ppBitStream += n >> 3;
	*pBitOffset = n & 7;
 
	return (tmp & 4095);
}

/*-----------------------------------------------------------------------------
*		vlc about
*----------------------------------------------------------------------------*/
Ipp32u
VlcDecIntraDCPredSize (Ipp8u **ppBitStream, int *pBitOffset, int blockType);

Tcoef
VlcDecodeIntraTCOEF(Ipp8u **ppBitStream, int *pBitOffset);

Tcoef
VlcDecodeInterTCOEF(Ipp8u **ppBitStream, int *pBitOffset, int short_video_header, int h263_flv);

/* RVLC from VM*/
Tcoef
RvlcDecodeIntraTCOEF(Ipp8u **ppBitStream, int *pBitOffset);

Tcoef
RvlcDecodeInterTCOEF(Ipp8u **ppBitStream, int *pBitOffset);

/*-----------------------------------------------------------------------------
*		quarter pixel interpolation
*----------------------------------------------------------------------------*/
	/* only horizontal interpolation */
void calc_QP_only_h(const unsigned char *pSrc,int srcStep,int srcWidth,int srcHeight,
			unsigned char *pDst, int dstStep,int dstWidth, int dstHeight, int xFrac,int rounding);
	/* only vertical interpolation */
void calc_QP_only_v(const unsigned char *pSrc,int srcStep,int srcWidth,int srcHeight,
			unsigned char *pDst, int dstStep,int dstWidth, int dstHeight, int yFrac,int rounding);


void calc_QP_bias(const Ipp8u *pSrc, int srcStep, int srcWidth, int srcHeight,
							Ipp8u *pDst,int dstStep,int dstWidth, int dstHeight,
							int xFrac,int yFrac, int rounding);


/* 4x8 block 1/2 interpolation,  used in OBMC HP */
void Copy4x8HP_8u_C1R(							
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding);


#endif
