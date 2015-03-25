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
#ifndef DROP_HIGH_PROFILE

#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <assert.h>

#ifdef __KINOMA_IPP__
#include "kinoma_avc_defines.h"
#include "ippvc.h"
#include "kinoma_ipp_lib.h"
#include "kinoma_utilities.h"

// This two will be deleted later -- WWD
#define DQ_BITS         6
#define DQ_ROUND        (1<<(DQ_BITS-1))


#define Q_BITS_8        16
#define DQ_BITS_8       6 
#define DQ_ROUND_8      (1<<(DQ_BITS_8-1))

const int quant_coef8[6][8][8] =
{
  {
    {13107, 12222,  16777,  12222,  13107,  12222,  16777,  12222},
    {12222, 11428,  15481,  11428,  12222,  11428,  15481,  11428},
    {16777, 15481,  20972,  15481,  16777,  15481,  20972,  15481},
    {12222, 11428,  15481,  11428,  12222,  11428,  15481,  11428},
    {13107, 12222,  16777,  12222,  13107,  12222,  16777,  12222},
    {12222, 11428,  15481,  11428,  12222,  11428,  15481,  11428},
    {16777, 15481,  20972,  15481,  16777,  15481,  20972,  15481},
    {12222, 11428,  15481,  11428,  12222,  11428,  15481,  11428}
  },
  {
    {11916, 11058,  14980,  11058,  11916,  11058,  14980,  11058},
    {11058, 10826,  14290,  10826,  11058,  10826,  14290,  10826},
    {14980, 14290,  19174,  14290,  14980,  14290,  19174,  14290},
    {11058, 10826,  14290,  10826,  11058,  10826,  14290,  10826},
    {11916, 11058,  14980,  11058,  11916,  11058,  14980,  11058},
    {11058, 10826,  14290,  10826,  11058,  10826,  14290,  10826},
    {14980, 14290,  19174,  14290,  14980,  14290,  19174,  14290},
    {11058, 10826,  14290,  10826,  11058,  10826,  14290,  10826}
  },
  {
    {10082, 9675, 12710,  9675, 10082,  9675, 12710,  9675},
    {9675,  8943, 11985,  8943, 9675, 8943, 11985,  8943},
    {12710, 11985,  15978,  11985,  12710,  11985,  15978,  11985},
    {9675,  8943, 11985,  8943, 9675, 8943, 11985,  8943},
    {10082, 9675, 12710,  9675, 10082,  9675, 12710,  9675},
    {9675,  8943, 11985,  8943, 9675, 8943, 11985,  8943},
    {12710, 11985,  15978,  11985,  12710,  11985,  15978,  11985},
    {9675,  8943, 11985,  8943, 9675, 8943, 11985,  8943}
  },
  {
    {9362,  8931, 11984,  8931, 9362, 8931, 11984,  8931},
    {8931,  8228, 11259,  8228, 8931, 8228, 11259,  8228},
    {11984, 11259,  14913,  11259,  11984,  11259,  14913,  11259},
    {8931,  8228, 11259,  8228, 8931, 8228, 11259,  8228},
    {9362,  8931, 11984,  8931, 9362, 8931, 11984,  8931},
    {8931,  8228, 11259,  8228, 8931, 8228, 11259,  8228},
    {11984, 11259,  14913,  11259,  11984,  11259,  14913,  11259},
    {8931,  8228, 11259,  8228, 8931, 8228, 11259,  8228}
  },
  {
    {8192,  7740, 10486,  7740, 8192, 7740, 10486,  7740},
    {7740,  7346, 9777, 7346, 7740, 7346, 9777, 7346},
    {10486, 9777, 13159,  9777, 10486,  9777, 13159,  9777},
    {7740,  7346, 9777, 7346, 7740, 7346, 9777, 7346},
    {8192,  7740, 10486,  7740, 8192, 7740, 10486,  7740},
    {7740,  7346, 9777, 7346, 7740, 7346, 9777, 7346},
    {10486, 9777, 13159,  9777, 10486,  9777, 13159,  9777},
    {7740,  7346, 9777, 7346, 7740, 7346, 9777, 7346}
  },
  {
    {7282,  6830, 9118, 6830, 7282, 6830, 9118, 6830},
    {6830,  6428, 8640, 6428, 6830, 6428, 8640, 6428},
    {9118,  8640, 11570,  8640, 9118, 8640, 11570,  8640},
    {6830,  6428, 8640, 6428, 6830, 6428, 8640, 6428},
    {7282,  6830, 9118, 6830, 7282, 6830, 9118, 6830},
    {6830,  6428, 8640, 6428, 6830, 6428, 8640, 6428},
    {9118,  8640, 11570,  8640, 9118, 8640, 11570,  8640},
    {6830,  6428, 8640, 6428, 6830, 6428, 8640, 6428}
  }
};



const int dequant_coef8[6][8][8] =
{
  {
    {20,  19, 25, 19, 20, 19, 25, 19},
    {19,  18, 24, 18, 19, 18, 24, 18},
    {25,  24, 32, 24, 25, 24, 32, 24},
    {19,  18, 24, 18, 19, 18, 24, 18},
    {20,  19, 25, 19, 20, 19, 25, 19},
    {19,  18, 24, 18, 19, 18, 24, 18},
    {25,  24, 32, 24, 25, 24, 32, 24},
    {19,  18, 24, 18, 19, 18, 24, 18}
  },
  {
    {22,  21, 28, 21, 22, 21, 28, 21},
    {21,  19, 26, 19, 21, 19, 26, 19},
    {28,  26, 35, 26, 28, 26, 35, 26},
    {21,  19, 26, 19, 21, 19, 26, 19},
    {22,  21, 28, 21, 22, 21, 28, 21},
    {21,  19, 26, 19, 21, 19, 26, 19},
    {28,  26, 35, 26, 28, 26, 35, 26},
    {21,  19, 26, 19, 21, 19, 26, 19}
  },
  {
    {26,  24, 33, 24, 26, 24, 33, 24},
    {24,  23, 31, 23, 24, 23, 31, 23},
    {33,  31, 42, 31, 33, 31, 42, 31},
    {24,  23, 31, 23, 24, 23, 31, 23},
    {26,  24, 33, 24, 26, 24, 33, 24},
    {24,  23, 31, 23, 24, 23, 31, 23},
    {33,  31, 42, 31, 33, 31, 42, 31},
    {24,  23, 31, 23, 24, 23, 31, 23}
  },
  {
    {28,  26, 35, 26, 28, 26, 35, 26},
    {26,  25, 33, 25, 26, 25, 33, 25},
    {35,  33, 45, 33, 35, 33, 45, 33},
    {26,  25, 33, 25, 26, 25, 33, 25},
    {28,  26, 35, 26, 28, 26, 35, 26},
    {26,  25, 33, 25, 26, 25, 33, 25},
    {35,  33, 45, 33, 35, 33, 45, 33},
    {26,  25, 33, 25, 26, 25, 33, 25}
  },
  {
    {32,  30, 40, 30, 32, 30, 40, 30},
    {30,  28, 38, 28, 30, 28, 38, 28},
    {40,  38, 51, 38, 40, 38, 51, 38},
    {30,  28, 38, 28, 30, 28, 38, 28},
    {32,  30, 40, 30, 32, 30, 40, 30},
    {30,  28, 38, 28, 30, 28, 38, 28},
    {40,  38, 51, 38, 40, 38, 51, 38},
    {30,  28, 38, 28, 30, 28, 38, 28}
  },
  {
    {36,  34, 46, 34, 36, 34, 46, 34},
    {34,  32, 43, 32, 34, 32, 43, 32},
    {46,  43, 58, 43, 46, 43, 58, 43},
    {34,  32, 43, 32, 34, 32, 43, 32},
    {36,  34, 46, 34, 36, 34, 46, 34},
    {34,  32, 43, 32, 34, 32, 43, 32},
    {46,  43, 58, 43, 46, 43, 58, 43},
    {34,  32, 43, 32, 34, 32, 43, 32}
  }

};


// Notation for comments regarding prediction and predictors.
// The pels of the 4x4 block are labelled a..p. The predictor pels above
// are labelled A..H, from the left I..P, and from above left X, as follows:
//
//  Z  A  B  C  D  E  F  G  H  I  J  K  L  M   N  O  P
//  Q  a1 b1 c1 d1 e1 f1 g1 h1
//  R  a2 b2 c2 d2 e2 f2 g2 h2
//  S  a3 b3 c3 d3 e3 f3 g3 h3
//  T  a4 b4 c4 d4 e4 f4 g4 h4
//  U  a5 b5 c5 d5 e5 f5 g5 h5
//  V  a6 b6 c6 d6 e6 f6 g6 h6
//  W  a7 b7 c7 d7 e7 f7 g7 h7
//  X  a8 b8 c8 d8 e8 f8 g8 h8


// Predictor array index definitions
#define P_Z (PredPel[0])
#define P_A (PredPel[1])
#define P_B (PredPel[2])
#define P_C (PredPel[3])
#define P_D (PredPel[4])
#define P_E (PredPel[5])
#define P_F (PredPel[6])
#define P_G (PredPel[7])
#define P_H (PredPel[8])
#define P_I (PredPel[9])
#define P_J (PredPel[10])
#define P_K (PredPel[11])
#define P_L (PredPel[12])
#define P_M (PredPel[13])
#define P_N (PredPel[14])
#define P_O (PredPel[15])
#define P_P (PredPel[16])
#define P_Q (PredPel[17])
#define P_R (PredPel[18])
#define P_S (PredPel[19])
#define P_T (PredPel[20])
#define P_U (PredPel[21])
#define P_V (PredPel[22])
#define P_W (PredPel[23])
#define P_X (PredPel[24])


_DECLSPEC static	const int dequant_coef[6][16] = {
	  {10, 13, 10, 13,   13, 16, 13, 16,   10, 13, 10, 13,   13, 16, 13, 16},
	  {11, 14, 11, 14,   14, 18, 14, 18,   11, 14, 11, 14,   14, 18, 14, 18},
	  {13, 16, 13, 16,   16, 20, 16, 20,   13, 16, 13, 16,   16, 20, 16, 20},
	  {14, 18, 14, 18,   18, 23, 18, 23,   14, 18, 14, 18,   18, 23, 18, 23},
	  {16, 20, 16, 20,   20, 25, 20, 25,   16, 20, 16, 20,   20, 25, 20, 25},
	  {18, 23, 18, 23,   23, 29, 23, 29,   18, 23, 18, 23,   23, 29, 23, 29}
	};

	// This array use for DC only mode
	_DECLSPEC static	const int		dc_block_idx_x[16] = {0, 4, 8, 12, 0, 4, 8, 12, 0, 4, 8, 12,  0,  4,  8, 12 };
	_DECLSPEC static	const int		dc_block_idx_y[16] = {0, 0, 0, 0,  4, 4, 4, 4,  8, 8, 8, 8,  12, 12, 12, 12 };

	// These two array use for DC and AC mode for Luma -- JVTG50 6.4.3  -- WWD
	_DECLSPEC static	const int		ac_block8x8_idx_x[4] = {0, 8, 0, 8}; 
	_DECLSPEC static	const int		ac_block8x8_idx_y[4] = {0, 0, 8, 8}; 

	_DECLSPEC static	const int		ac_block_idx_x[16] = {0, 4, 0, 4, 8, 12, 8, 12, 0, 4, 0, 4,  8, 12,  8, 12 };
	_DECLSPEC static	const int		ac_block_idx_y[16] = {0, 0, 4, 4, 0,  0, 4,  4, 8, 8, 12,12, 8,  8, 12, 12 };

	_DECLSPEC static	const unsigned char decode_block_scan[16] = {0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15};		// For DC index in AC mode -- WWD
	
	_DECLSPEC static	int		cbpmask[16] = {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
	_DECLSPEC static	int		cbpmask_8x8[4]  = {1, 2, 4, 8};		// Please note it is different from above one -- SPEC error in INTEL
	_DECLSPEC static	int		cbpmask_4x4[16] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};


IppStatus __STDCALL ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_c(
	   Ipp16s **ppSrcCoeff,
       Ipp8u *pSrcDstYPlane,
       Ipp32u srcdstYStep,
       const IppIntra16x16PredMode_H264 intra_luma_mode,
       const Ipp32u cbp4x4,
       const Ipp32u QP,
       const Ipp8u edge_type,
       Ipp16s *pQuantTable,
       Ipp8u bypass_flag /*Resevr ONLY : do not support at present */)
{
	// ppSrcCoeff add 16 only when the "coresponding cbp bit" equal 1
	//  cbp4x4 : 0 specify Luma DC
	//           1--16 specify Luma AC block coeffs

	_DECLSPEC unsigned char prediction_array[256];
	_DECLSPEC int		acArray[16];
	_DECLSPEC short		dcArray[4][4], dcResult[16];

	int		M5[4], M6[4];
	int		m7[4][4], m5[4], m6[4];


	int		up_avail, left_avail, up_left_avail;
	int		i,	j, k;
	Ipp8u	*pSrcforPrediction, *pSrcforPrediction2;

	int		pitch = srcdstYStep;

	int		s0, s1, s2;				// Used for DC prediction
	int		iv, ih, ib, ic, iaa;	// Used for Plane prediction

	int		qp_per, qp_rem;
	int		round ; 


	// 需要将以下的scan_order转换成为scan_position
	int		block_x, block_y;
	unsigned int  cbp = cbp4x4 & 0x1ffff;		// Need 17 bits to represent Luma information 

	Profile_Start(ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_c_profile);

	qp_per = QP/6;
	qp_rem = QP%6;


	// First, we do intra-prediction according to edge_type and intra_luma_mode!!!!!!!!!!!!!!!!!!!!
	left_avail		= !(edge_type&IPPVC_LEFT_EDGE);
	up_avail		= !(edge_type&IPPVC_TOP_EDGE);
	up_left_avail	= !(edge_type&IPPVC_TOP_LEFT_EDGE);

	round = (qp_per)?1:2;


	if(cbp == 0)
	{
		// IPPVC_CBP_LUMA_DC
		switch (intra_luma_mode)
		{
			case 0:														// IPP_16X16_VERT
				pSrcforPrediction = pSrcDstYPlane - pitch;				// UP
				for(j=0;j<16;j++)
				for(i=0;i<16;i++)
					pSrcDstYPlane[j*pitch+i] = pSrcforPrediction[i];	// store predicted 16x16 block

				break;
			case 1:														// IPP_16X16_HOR
				pSrcforPrediction = pSrcDstYPlane - 1;					// LEFT
				for(j=0;j<16;j++)
				for(i=0;i<16;i++)
					pSrcDstYPlane[j*pitch+i] = pSrcforPrediction[j*pitch];	// store predicted 16x16 block

				break;

			case 2:														// IPP_16X16_DC
				// Get DC value
				s1=s2=0;
				if(up_avail)
				{
					pSrcforPrediction = pSrcDstYPlane - srcdstYStep;		// UP
					for(i=0; i < 16; i++)
					{
						s1 += pSrcforPrediction[i];

					}
				}
				if(left_avail)
				{
					pSrcforPrediction = pSrcDstYPlane - 1;					// LEFT
					for(i=0; i < 16; i++)
					{
						s2 += pSrcforPrediction[i*srcdstYStep];

					}

				}
				if (up_avail && left_avail)
					s0=(s1+s2+16)>>5;									// no edge
				if (!up_avail && left_avail)
					s0=(s2+8)>>4;										// upper edge -- use left
				if (up_avail && !left_avail)
					s0=(s1+8)>>4;										// left edge  -- use up
				if (!up_avail && !left_avail)
					s0=128;											// top left corner, nothing to predict from

				// make prediction
				for(j=0;j<16;j++)
					for(i=0; i<16; i++)
					{
						//prediction_array[j] = s0;
						pSrcDstYPlane[j*pitch+i] = s0;
					}

				break;

			case 3:														// IPP_16X16_PLANE
				if (!up_avail || !up_left_avail  || !left_avail)
				{
					fprintf(stdout, "%s\n", "I got error intra prediction mode in 16x16 mode, please check bitstream!");
					return ippStsErr;
				}

				ih = 0;
				iv = 0;

				pSrcforPrediction	= pSrcDstYPlane -srcdstYStep;	// Up
				pSrcforPrediction2	= pSrcDstYPlane -1;				// Left
				for (i=1;i<9;i++)
				{
					ih += i*(pSrcforPrediction[7+i] - pSrcforPrediction[7-i]);
					iv += i*(pSrcforPrediction2[(7+i)*srcdstYStep] - pSrcforPrediction2[(7-i)*srcdstYStep]);
				}

				ib=(5*ih+32)>>6;
				ic=(5*iv+32)>>6;

				//iaa=16*(imgY[up.pos_y][up.pos_x+15]+imgY[left[16].pos_y][left[16].pos_x]);
				iaa=16*(pSrcDstYPlane[-1+15*srcdstYStep] + pSrcDstYPlane[15-srcdstYStep]);


				for (j=0;j< 16;j++)
				{
					for (i=0;i< 16;i++)
					{
						pSrcDstYPlane[j*pitch+i] = IClip(0,255, ((iaa+(i-7)*ib +(j-7)*ic + 16)>>5));
					}
				}// store plane prediction

				break;

			default:
				// Error!
				fprintf(stdout, "%s\n", "I got error intra prediction mode in 16x16 mode, please check bitstream!");
				return ippStsErr;
		}

	}
	else
	{
		// IPPVC_CBP_LUMA_DC
		switch (intra_luma_mode)
		{
			case 0:														// IPP_16X16_VERT
				pSrcforPrediction = pSrcDstYPlane - pitch;		// UP
				for(j=0;j<16;j++)
				for(i=0;i<16;i++)
					prediction_array[j*16+i] = pSrcforPrediction[i];	// store predicted 16x16 block

				break;
			case 1:														// IPP_16X16_HOR
				pSrcforPrediction = pSrcDstYPlane - 1;					// LEFT
				for(j=0;j<16;j++)
				for(i=0;i<16;i++)
					prediction_array[j*16+i] = pSrcforPrediction[j*pitch];	// store predicted 16x16 block

				break;

			case 2:														// IPP_16X16_DC
				// Get DC value
				s1=s2=0;
				if(up_avail)
				{
					pSrcforPrediction = pSrcDstYPlane - srcdstYStep;		// UP
					for(i=0; i < 16; i++)
					{
						s1 += pSrcforPrediction[i];

					}
				}
				if(left_avail)
				{
					pSrcforPrediction = pSrcDstYPlane - 1;					// LEFT
					for(i=0; i < 16; i++)
					{
						s2 += pSrcforPrediction[i*srcdstYStep];

					}

				}
				if (up_avail && left_avail)
					s0=(s1+s2+16)>>5;									// no edge
				if (!up_avail && left_avail)
					s0=(s2+8)>>4;										// upper edge -- use left
				if (up_avail && !left_avail)
					s0=(s1+8)>>4;										// left edge  -- use up
				if (!up_avail && !left_avail)
					s0=128;											// top left corner, nothing to predict from

				// make prediction
				for(j=0;j<256;j++)
				{
					prediction_array[j] = s0;
				}

				break;

			case 3:														// IPP_16X16_PLANE
				if (!up_avail || !up_left_avail  || !left_avail)
				{
					fprintf(stdout, "%s\n", "I got error intra prediction mode in 16x16 mode, please check bitstream!");
					return ippStsErr;
				}

				ih = 0;
				iv = 0;

				pSrcforPrediction	= pSrcDstYPlane -srcdstYStep;	// Up
				pSrcforPrediction2	= pSrcDstYPlane -1;				// Left
				for (i=1;i<9;i++)
				{
	/*				if (i<8)
						//ih += i*(imgY[up.pos_y][up.pos_x+7+i] - imgY[up.pos_y][up.pos_x+7-i]);
						ih += i*(imgY[up.pos_y][up.pos_x+7+i] - imgY[up.pos_y][up.pos_x+7-i]);

					else
						//ih += i*(imgY[up.pos_y][up.pos_x+7+i] - imgY[left[0].pos_y][left[0].pos_x]);
						ih += i*(imgY[up.pos_y][up.pos_x+7+i] - imgY[left[0].pos_y][left[0].pos_x]);
	*/
					ih += i*(pSrcforPrediction[7+i] - pSrcforPrediction[7-i]);
					iv += i*(pSrcforPrediction2[(7+i)*srcdstYStep] - pSrcforPrediction2[(7-i)*srcdstYStep]);
				}

				ib=(5*ih+32)>>6;
				ic=(5*iv+32)>>6;

				//iaa=16*(imgY[up.pos_y][up.pos_x+15]+imgY[left[16].pos_y][left[16].pos_x]);
				iaa=16*(pSrcDstYPlane[-1+15*srcdstYStep] + pSrcDstYPlane[15-srcdstYStep]);


				for (j=0;j< 16;j++)
				{
					for (i=0;i< 16;i++)
					{
						//prediction_array[i][j] = max(0,min((iaa+(i-7)*ib +(j-7)*ic + 16)>>5,255));
						prediction_array[j*16+i] = IClip(0,255, ((iaa+(i-7)*ib +(j-7)*ic + 16)>>5));
					}
				}// store plane prediction

				break;

			default:
				// Error!
				fprintf(stdout, "%s\n", "I got error intra prediction mode in 16x16 mode, please check bitstream!");
				return ippStsErr;
		}



		// Second, we do inverse Hadamard transform for DC coeffs
		if(cbp & 0x01)
		{

			// horizontal
			for (j=0;j<4;j++)
			{
				M5[0] = ((*ppSrcCoeff)[j*4 + 0]);
				M5[1] = ((*ppSrcCoeff)[j*4 + 1]);
				M5[2] = ((*ppSrcCoeff)[j*4 + 2]);
				M5[3] = ((*ppSrcCoeff)[j*4 + 3]);

				M6[0] = M5[0] + M5[2];
				M6[1] = M5[0] - M5[2];
				M6[2] = M5[1] - M5[3];
				M6[3] = M5[1] + M5[3];


				dcArray[j][0] = M6[0] + M6[3];
				dcArray[j][3] = M6[0] - M6[3];
				dcArray[j][1] = M6[1] + M6[2];
				dcArray[j][2] = M6[1] - M6[2];
			}

			// vertical
			for (i=0;i<4;i++)
			{

				M5[0] = dcArray[0][i];
				M5[1] = dcArray[1][i];
				M5[2] = dcArray[2][i];
				M5[3] = dcArray[3][i];

				M6[0] = M5[0] + M5[2];
				M6[1] = M5[0] - M5[2];
				M6[2] = M5[1] - M5[3];
				M6[3] = M5[1] + M5[3];


				dcResult[0*4+i] = M6[0] + M6[3];
				dcResult[3*4+i] = M6[0] - M6[3];
				dcResult[1*4+i] = M6[1] + M6[2];
				dcResult[2*4+i] = M6[1] - M6[2];

			}

#if 1		//WWD_DEBUG
			if(QP >= 12)
			{
				for (j=0;j< 16;j+=4)
				{
					for (i=0;i< 4;i++)
					{
						dcResult[j+i] = (((dcResult[j+i] *dequant_coef[qp_rem][0]) << (qp_per-2)) ) ;
					}
				}

			}
			else
			{
				for (j=0;j< 16;j+=4)
				{
					for (i=0;i< 4;i++)
					{
						dcResult[j+i] = (((dcResult[j+i] *dequant_coef[qp_rem][0] + round) >> (2 - qp_per)) ) ;

					}
				}

			}
#else
			for (j=0;j< 16;j+=4)
			{
				for (i=0;i< 4;i++)
				{
					dcResult[j+i] = (((dcResult[j+i] *pQuantTable[0] + (1<<(qp_per))) >> qp_per) ) ;
				}
			}
#endif

			*ppSrcCoeff += 16;

		}
		else
		{
			
			for (j=0;j<16;j++)
			{
				dcResult[j] = 0 ;
			}
			
			//memset((short *) dcResult, 0, 32);

		}

		// Third IDCT 
		if(cbp == 1)
		{

			for(k=0; k<16; k++)									// CBP decision
			{
				// Please note: we use normal order according to DC components -- WWD in 2006-05-06
				block_x = dc_block_idx_x[k];
				block_y = dc_block_idx_y[k];

				for(j=0; j<4; j++)
					for(i=0; i<4; i++)
					{
						pSrcDstYPlane[(block_y+j)*srcdstYStep+block_x+i] = IClip(0, 255,prediction_array[(block_y+j)*16+block_x+i] + ((dcResult[k]+32)>>6));

					}

			}

			return ippStsNoErr;
		}
		else  //if(cbp > 1)
		{

			for(k=0; k< 16; k++)
			{
				// Please note: we use normal order according to AC components here-- WWD in 2006-05-06
				block_x = ac_block_idx_x[k];
				block_y = ac_block_idx_y[k];


				if( !(cbp & (1<< (1+k))))
				{
					for(j=0; j<4; j++)
						for(i=0; i<4; i++)
						{
							pSrcDstYPlane[(block_y+j)*srcdstYStep+block_x+i] = IClip(0, 255,prediction_array[(block_y+j)*16+block_x+i] + ((dcResult[decode_block_scan[k]]+32)>>6));

						}

				}
				else
				{
					// Form acArray and do IQ
					//for(i=1; i<16; i++)
					//{
					//	acArray[i] = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem][i]) << (qp_per);
					//}
					for(i=1; i<16; i++)
					{
						acArray[i] = (((((*ppSrcCoeff)[i] * pQuantTable[i]) << qp_per) + 8)>>4);
					}

					acArray[0] = dcResult[decode_block_scan[k]];			// Add DC coeff in scan order

					// Do Inverse transform
					// horizontal
					for (j=0;j<4;j++)
					{
						m5[0] = acArray[0+j*4];
						m5[1] = acArray[1+j*4];
						m5[2] = acArray[2+j*4];
						m5[3] = acArray[3+j*4];


						m6[0] = (m5[0]     + m5[2]);
						m6[1] = (m5[0]     - m5[2]);
						m6[2] = (m5[1]>>1) - m5[3];
						m6[3] = m5[1]      + (m5[3]>>1);


						m7[j][0]  = m6[0] + m6[3];
						m7[j][3]  = m6[0] - m6[3];
						m7[j][1]  = m6[1] + m6[2];
						m7[j][2]  = m6[1] - m6[2];

					}

					// vertical
					for (i=0;i<4;i++)
					{
						m5[0]=m7[0][i];
						m5[1]=m7[1][i];
						m5[2]=m7[2][i];
						m5[3]=m7[3][i];

						m6[0]=(m5[0]+m5[2]);
						m6[1]=(m5[0]-m5[2]);
						m6[2]=(m5[1]>>1)-m5[3];
						m6[3]=m5[1]+(m5[3]>>1);
					
						m7[0][i]  = m6[0]+m6[3];
						m7[3][i]  = m6[0]-m6[3];
						m7[1][i]  = m6[1]+m6[2];
						m7[2][i]  = m6[1]-m6[2];

					}

					for(j=0; j<4; j++)
						for(i=0; i<4; i++)
						{
							//pSrcDstUPlane[(block_y+jj)*srcdstUVStep+block_x+ii] = IClip(0, 255,(((prediction_arrayU[(block_y+jj)*8 + block_x+ii]<<DQ_BITS) + m7[jj][ii] + DQ_ROUND )>> DQ_BITS));
							pSrcDstYPlane[(block_y+j)*srcdstYStep+block_x+i] =  IClip(0, 255,(((prediction_array[(block_y+j)*16 + block_x+i]<<DQ_BITS) + m7[j][i] + DQ_ROUND )>> DQ_BITS));

						}

					*ppSrcCoeff += 16;

				}				
			}
		}
	}

	Profile_End(ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_c_profile);

	return ippStsNoErr;

}


/*! 
 *************************************************************************************
 * \brief
 *    Prefiltering for Intra8x8 prediction
 *************************************************************************************
 */
void LowPassForIntra8x8Pred(int *PredPel, int block_up_left, int block_up, int block_left)
{
  int i;
  int LoopArray[25];
 

  for(i = 0; i < 25; i++)
     LoopArray[i] = PredPel[i] ;

 	if(block_up)
	{
		if(block_up_left) 
		{
			LoopArray[1] = ((&P_Z)[0] + ((&P_Z)[1]<<1) + (&P_Z)[2] + 2)>>2;
		}
		else
			LoopArray[1] = ((&P_Z)[1] + ((&P_Z)[1]<<1) + (&P_Z)[2] + 2)>>2; 


		for(i = 2; i <16; i++)
		{
			LoopArray[i] = ((&P_Z)[i-1] + ((&P_Z)[i]<<1) + (&P_Z)[i+1] + 2)>>2;
		}
		LoopArray[16] = (P_P + (P_P<<1) + P_O + 2)>>2;
	}

	if(block_up_left) 
	{
		
		if(block_up && block_left)
		{
				LoopArray[0] = (P_Q + (P_Z<<1) + P_A +2)>>2;
		}
		else
		{
			if(block_up)
				LoopArray[0] = (P_Z + (P_Z<<1) + P_A +2)>>2;
			else
				if(block_left)
					LoopArray[0] = (P_Z + (P_Z<<1) + P_Q +2)>>2;
		}

	}

	if(block_left)
	{
		if(block_up_left)
			LoopArray[17] = (P_Z + (P_Q<<1) + P_R + 2)>>2; 
		else
			LoopArray[17] = (P_Q + (P_Q<<1) + P_R + 2)>>2;

		for(i = 18; i <24; i++)
		{
			LoopArray[i] = ((&P_Z)[i-1] + ((&P_Z)[i]<<1) + (&P_Z)[i+1] + 2)>>2;
		}
		LoopArray[24] = (P_W + (P_X<<1) + P_X + 2)>>2;
	}

  for(i = 0; i < 25; i++)
    PredPel[i] = LoopArray[i];
}




/*************************************************************************************
 * Function for 8x8 Intra reconstruction (include prediction, IDCT and reconstruction)
 *
 *************************************************************************************
 */
IppStatus __STDCALL ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_c(Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstYPlane,
            Ipp32s srcdstYStep,
            IppIntra8x8PredMode_H264 *pMBIntraTypes,
            Ipp32u cbp8x8,
            Ipp32u QP,
            Ipp8u edge_type,
            Ipp16s *pQuantTable,
            Ipp8u bypass_flag)
{
	/*  Prediction mode
	  IPP_4x4_VERT     = 0,
	  IPP_4x4_HOR      = 1,
	  IPP_4x4_DC       = 2,
	  IPP_4x4_DIAG_DL  = 3,
	  IPP_4x4_DIAG_DR  = 4,
	  IPP_4x4_VR       = 5,
	  IPP_4x4_HD       = 6,
	  IPP_4x4_VL       = 7,
	  IPP_4x4_HU       = 8
	*/
	_DECLSPEC unsigned char prediction_array[64];

	_DECLSPEC int		avail_left[4]	    = { 1, 1, 1, 1};
	_DECLSPEC int		avail_top [4]	    = { 1, 1, 1, 1};
	_DECLSPEC int		avail_top_left[4]   = { 1, 1, 1, 1};
	_DECLSPEC int		avail_top_right[4]  = { 1, 1, 1, 0};

	_DECLSPEC int		acArray[64];
	_DECLSPEC int		PredPel[64];  // array of predictor pels
	_DECLSPEC int		m6[8][8], Array_idct[8][8];

	int		block_x, block_y;

	unsigned char * pPredictionArray;

	int		i,	j, k, t1, t2;
	Ipp8u	*pSrcforPrediction;

	int		pitch = srcdstYStep;

	int		s0;				// Used for DC prediction

	int		qp_per, qp_rem;

	unsigned int  cbp = cbp8x8 & 0x0f;		// Need 5 bits to represent Luma information 
	int *pacArray;

	Profile_Start(ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_c_profile);

	qp_per = QP/6;
	qp_rem = QP%6;


	// First, we do intra-prediction according to edge_type and intra_luma_mode!!!!!!!!!!!!!!!!!!!!
	// 1-1, we need compute avail array for three array [ left, top and top_left]
	// TOP first
	avail_top[0] = 
	avail_top[1] = !(edge_type&IPPVC_TOP_EDGE);

	// Then LEFT
	avail_left[0] = 
	avail_left[2] = !(edge_type&IPPVC_LEFT_EDGE);

	// Last TOP_LEFT
	avail_top_left[0] =  !(edge_type&IPPVC_TOP_LEFT_EDGE);
	avail_top_left[1] =  !(edge_type&IPPVC_TOP_EDGE);
	avail_top_left[2] =  !(edge_type&IPPVC_LEFT_EDGE);

	// TOP_RIGHT
	avail_top_right[0] = !(edge_type&IPPVC_TOP_EDGE);
	avail_top_right[1] = !(edge_type&IPPVC_TOP_RIGHT_EDGE);

	for(k=0; k<4; k++)			// In scan order (4x4 blocks)
	{
		block_x = ac_block8x8_idx_x[k];
		block_y = ac_block8x8_idx_y[k];

		pSrcforPrediction = pSrcDstYPlane + block_y*srcdstYStep + block_x;			// Pointer to top_left cornor of one 4x4 block
		// Prepare array for prediction
		if (avail_top_left[k])
		{
			P_Z = pSrcforPrediction[-srcdstYStep - 1];

		}
		else
		{
			P_Z = 128;

		}		
		
		if (avail_top[k])
		{
			P_A = pSrcforPrediction[-srcdstYStep    ];
			P_B = pSrcforPrediction[-srcdstYStep + 1];
			P_C = pSrcforPrediction[-srcdstYStep + 2];
			P_D = pSrcforPrediction[-srcdstYStep + 3];
			P_E = pSrcforPrediction[-srcdstYStep + 4];
			P_F = pSrcforPrediction[-srcdstYStep + 5];
			P_G = pSrcforPrediction[-srcdstYStep + 6];
			P_H = pSrcforPrediction[-srcdstYStep + 7];
	
		}
		else
		{
			P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = 128;

		}

		if (avail_top_right[k])
		{
			P_I = pSrcforPrediction[-srcdstYStep + 8];
			P_J = pSrcforPrediction[-srcdstYStep + 9];
			P_K = pSrcforPrediction[-srcdstYStep + 10];
			P_L = pSrcforPrediction[-srcdstYStep + 11];
			P_M = pSrcforPrediction[-srcdstYStep + 12];
			P_N = pSrcforPrediction[-srcdstYStep + 13];
			P_O = pSrcforPrediction[-srcdstYStep + 14];
			P_P = pSrcforPrediction[-srcdstYStep + 15];

		}
		else
		{
			P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;

		}

		if (avail_left[k])
		{
			P_Q = pSrcforPrediction[              -1];
			P_R = pSrcforPrediction[  srcdstYStep -1];
			P_S = pSrcforPrediction[srcdstYStep + srcdstYStep -1];
			P_T = pSrcforPrediction[(srcdstYStep<<1) + srcdstYStep -1];

			P_U = pSrcforPrediction[4*  srcdstYStep -1];
			P_V = pSrcforPrediction[5*  srcdstYStep -1];
			P_W = pSrcforPrediction[6*  srcdstYStep -1];
			P_X = pSrcforPrediction[7*  srcdstYStep -1];

		}
		else
		{
			P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = 128;

		}

		LowPassForIntra8x8Pred(&(P_Z), avail_top_left[k], avail_top[k], avail_left[k]);

		pPredictionArray = &prediction_array[0];
		// Form prediction result into corresponding position
		switch(pMBIntraTypes[k])
		{

			case IPP_4x4_VERT:                       /* vertical prediction from block above */
				for(j=0;j<64;j+=8)
				for(i=0;i<8;i++)
					pPredictionArray[j+i] = PredPel[1+i];	

				break;

			case IPP_4x4_HOR:                        /* horizontal prediction from left block */

				for(j=0;j<8;j++)
				for(i=0;i<8;i++)
					pPredictionArray[j*8+i] = PredPel[17 + j];
		
				break;


			case IPP_4x4_DC:                         /* DC prediction */

				s0 = 0;
				if (avail_top[k] && avail_left[k])
				{   
					// no edge
					//s0 = (P_A + P_B + P_C + P_D + P_I + P_J + P_K + P_L + 4)>>3;
					s0 = (P_A + P_B + P_C + P_D + P_E + P_F + P_G + P_H + P_Q + P_R + P_S + P_T + P_U + P_V + P_W + P_X + 8) >> 4;
				}
				else if (!avail_top[k] && avail_left[k])
				{
					// upper edge
					//s0 = (P_I + P_J + P_K + P_L + 2)>>2;             
					s0 = (P_Q + P_R + P_S + P_T + P_U + P_V + P_W + P_X + 4) >> 3;             
				}
				else if (avail_top[k] && !avail_left[k])
				{
					// left edge
					//s0 = (P_A + P_B + P_C + P_D + 2)>>2;             
					s0 = (P_A + P_B + P_C + P_D + P_E + P_F + P_G + P_H + 4) >> 3;             
				}
				else //if (!block_available_up && !block_available_left)
				{
					// top left corner, nothing to predict from
					s0 = 128;                           
				}

				for (j=0; j < 64; j+=8)
				{
					for (i=0; i < 8; i++)
					{
						// store DC prediction
						pPredictionArray[j+i] = s0;
					}
				}

				break;

			case IPP_4x4_DIAG_DL:
				if (!avail_top[k])
				printf ("warning: Intra_4x4_Diagonal_Down_Left prediction mode not allowed at mb \n");

				// Mode DIAG_DOWN_LEFT_PRED
				// Shiquan: here can refer x+y=0..13
				pPredictionArray[0 + 0 * 8] = (P_A + P_C + 2*(P_B) + 2) >> 2;
				pPredictionArray[0 + 1 * 8] = 
				pPredictionArray[1 + 0 * 8] = (P_B + P_D + 2*(P_C) + 2) >> 2;
				pPredictionArray[0 + 2 * 8] =
				pPredictionArray[1 + 1 * 8] =
				pPredictionArray[2 + 0 * 8] = (P_C + P_E + 2*(P_D) + 2) >> 2;
				pPredictionArray[0 + 3 * 8] = 
				pPredictionArray[1 + 2 * 8] = 
				pPredictionArray[2 + 1 * 8] = 
				pPredictionArray[3 + 0 * 8] = (P_D + P_F + 2*(P_E) + 2) >> 2;
				pPredictionArray[0 + 4 * 8] = 
				pPredictionArray[1 + 3 * 8] = 
				pPredictionArray[2 + 2 * 8] = 
				pPredictionArray[3 + 1 * 8] = 
				pPredictionArray[4 + 0 * 8] = (P_E + P_G + 2*(P_F) + 2) >> 2;
				pPredictionArray[0 + 5 * 8] = 
				pPredictionArray[1 + 4 * 8] = 
				pPredictionArray[2 + 3 * 8] = 
				pPredictionArray[3 + 2 * 8] = 
				pPredictionArray[4 + 1 * 8] = 
				pPredictionArray[5 + 0 * 8] = (P_F + P_H + 2*(P_G) + 2) >> 2;
				pPredictionArray[0 + 6 * 8] = 
				pPredictionArray[1 + 5 * 8] = 
				pPredictionArray[2 + 4 * 8] = 
				pPredictionArray[3 + 3 * 8] = 
				pPredictionArray[4 + 2 * 8] = 
				pPredictionArray[5 + 1 * 8] = 
				pPredictionArray[6 + 0 * 8] = (P_G + P_I + 2*(P_H) + 2) >> 2;
				pPredictionArray[0 + 7 * 8] = 
				pPredictionArray[1 + 6 * 8] = 
				pPredictionArray[2 + 5 * 8] = 
				pPredictionArray[3 + 4 * 8] = 
				pPredictionArray[4 + 3 * 8] = 
				pPredictionArray[5 + 2 * 8] = 
				pPredictionArray[6 + 1 * 8] = 
				pPredictionArray[7 + 0 * 8] = (P_H + P_J + 2*(P_I) + 2) >> 2;
				pPredictionArray[1 + 7 * 8] = 
				pPredictionArray[2 + 6 * 8] = 
				pPredictionArray[3 + 5 * 8] = 
				pPredictionArray[4 + 4 * 8] = 
				pPredictionArray[5 + 3 * 8] = 
				pPredictionArray[6 + 2 * 8] = 
				pPredictionArray[7 + 1 * 8] = (P_I + P_K + 2*(P_J) + 2) >> 2;
				pPredictionArray[2 + 7 * 8] = 
				pPredictionArray[3 + 6 * 8] = 
				pPredictionArray[4 + 5 * 8] = 
				pPredictionArray[5 + 4 * 8] = 
				pPredictionArray[6 + 3 * 8] = 
				pPredictionArray[7 + 2 * 8] = (P_J + P_L + 2*(P_K) + 2) >> 2;
				pPredictionArray[3 + 7 * 8] = 
				pPredictionArray[4 + 6 * 8] = 
				pPredictionArray[5 + 5 * 8] = 
				pPredictionArray[6 + 4 * 8] = 
				pPredictionArray[7 + 3 * 8] = (P_K + P_M + 2*(P_L) + 2) >> 2;
				pPredictionArray[4 + 7 * 8] = 
				pPredictionArray[5 + 6 * 8] = 
				pPredictionArray[6 + 5 * 8] = 
				pPredictionArray[7 + 4 * 8] = (P_L + P_N + 2*(P_M) + 2) >> 2;
				pPredictionArray[5 + 7 * 8] = 
				pPredictionArray[6 + 6 * 8] = 
				pPredictionArray[7 + 5 * 8] = (P_M + P_O + 2*(P_N) + 2) >> 2;
				pPredictionArray[6 + 7 * 8] = 
				pPredictionArray[7 + 6 * 8] = (P_N + P_P + 2*(P_O) + 2) >> 2;
				pPredictionArray[7 + 7 * 8] = (P_O + 3*(P_P) + 2) >> 2;
			
				break;

			case IPP_4x4_DIAG_DR:
				//if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
				//printf ("warning: Intra_4x4_Diagonal_Down_Right prediction mode not allowed at mb %d\n",img->current_mb_nr);
				// Mode DIAG_DOWN_RIGHT_PRED
				pPredictionArray[0 + 7 * 8] = (P_X + P_V + 2*(P_W) + 2) >> 2;
				pPredictionArray[0 + 6 * 8] = 
				pPredictionArray[1 + 7 * 8] = (P_W + P_U + 2*(P_V) + 2) >> 2;
				pPredictionArray[0 + 5 * 8] = 
				pPredictionArray[1 + 6 * 8] = 
				pPredictionArray[2 + 7 * 8] = (P_V + P_T + 2*(P_U) + 2) >> 2;
				pPredictionArray[0 + 4 * 8] = 
				pPredictionArray[1 + 5 * 8] = 
				pPredictionArray[2 + 6 * 8] = 
				pPredictionArray[3 + 7 * 8] = (P_U + P_S + 2*(P_T) + 2) >> 2;
				pPredictionArray[0 + 3 * 8] = 
				pPredictionArray[1 + 4 * 8] = 
				pPredictionArray[2 + 5 * 8] = 
				pPredictionArray[3 + 6 * 8] = 
				pPredictionArray[4 + 7 * 8] = (P_T + P_R + 2*(P_S) + 2) >> 2;
				pPredictionArray[0 + 2 * 8] = 
				pPredictionArray[1 + 3 * 8] = 
				pPredictionArray[2 + 4 * 8] = 
				pPredictionArray[3 + 5 * 8] = 
				pPredictionArray[4 + 6 * 8] = 
				pPredictionArray[5 + 7 * 8] = (P_S + P_Q + 2*(P_R) + 2) >> 2;
				pPredictionArray[0 + 1 * 8] = 
				pPredictionArray[1 + 2 * 8] = 
				pPredictionArray[2 + 3 * 8] = 
				pPredictionArray[3 + 4 * 8] = 
				pPredictionArray[4 + 5 * 8] = 
				pPredictionArray[5 + 6 * 8] = 
				pPredictionArray[6 + 7 * 8] = (P_R + P_Z + 2*(P_Q) + 2) >> 2;
				pPredictionArray[0 + 0 * 8] = 
				pPredictionArray[1 + 1 * 8] = 
				pPredictionArray[2 + 2 * 8] = 
				pPredictionArray[3 + 3 * 8] = 
				pPredictionArray[4 + 4 * 8] = 
				pPredictionArray[5 + 5 * 8] = 
				pPredictionArray[6 + 6 * 8] = 
				pPredictionArray[7 + 7 * 8] = (P_Q + P_A + 2*(P_Z) + 2) >> 2;
				pPredictionArray[1 + 0 * 8] = 
				pPredictionArray[2 + 1 * 8] = 
				pPredictionArray[3 + 2 * 8] = 
				pPredictionArray[4 + 3 * 8] = 
				pPredictionArray[5 + 4 * 8] = 
				pPredictionArray[6 + 5 * 8] = 
				pPredictionArray[7 + 6 * 8] = (P_Z + P_B + 2*(P_A) + 2) >> 2;
				pPredictionArray[2 + 0 * 8] = 
				pPredictionArray[3 + 1 * 8] = 
				pPredictionArray[4 + 2 * 8] = 
				pPredictionArray[5 + 3 * 8] = 
				pPredictionArray[6 + 4 * 8] = 
				pPredictionArray[7 + 5 * 8] = (P_A + P_C + 2*(P_B) + 2) >> 2;
				pPredictionArray[3 + 0 * 8] = 
				pPredictionArray[4 + 1 * 8] = 
				pPredictionArray[5 + 2 * 8] = 
				pPredictionArray[6 + 3 * 8] = 
				pPredictionArray[7 + 4 * 8] = (P_B + P_D + 2*(P_C) + 2) >> 2;
				pPredictionArray[4 + 0 * 8] = 
				pPredictionArray[5 + 1 * 8] = 
				pPredictionArray[6 + 2 * 8] = 
				pPredictionArray[7 + 3 * 8] = (P_C + P_E + 2*(P_D) + 2) >> 2;
				pPredictionArray[5 + 0 * 8] = 
				pPredictionArray[6 + 1 * 8] = 
				pPredictionArray[7 + 2 * 8] = (P_D + P_F + 2*(P_E) + 2) >> 2;
				pPredictionArray[6 + 0 * 8] = 
				pPredictionArray[7 + 1 * 8] = (P_E + P_G + 2*(P_F) + 2) >> 2;
				pPredictionArray[7 + 0 * 8] = (P_F + P_H + 2*(P_G) + 2) >> 2;		
			
				break;



			case  IPP_4x4_VR:/* diagonal prediction -22.5 deg to horizontal plane */
				//if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
				//printf ("warning: Intra_4x4_Vertical_Right prediction mode not allowed at mb %d\n",img->current_mb_nr);
				pPredictionArray[0 + 0 * 8] = 
				pPredictionArray[1 + 2 * 8] = 
				pPredictionArray[2 + 4 * 8] = 
				pPredictionArray[3 + 6 * 8] = (P_Z + P_A + 1) >> 1;
				pPredictionArray[1 + 0 * 8] = 
				pPredictionArray[2 + 2 * 8] = 
				pPredictionArray[3 + 4 * 8] = 
				pPredictionArray[4 + 6 * 8] = (P_A + P_B + 1) >> 1;
				pPredictionArray[2 + 0 * 8] = 
				pPredictionArray[3 + 2 * 8] = 
				pPredictionArray[4 + 4 * 8] = 
				pPredictionArray[5 + 6 * 8] = (P_B + P_C + 1) >> 1;
				pPredictionArray[3 + 0 * 8] = 
				pPredictionArray[4 + 2 * 8] = 
				pPredictionArray[5 + 4 * 8] = 
				pPredictionArray[6 + 6 * 8] = (P_C + P_D + 1) >> 1;
				pPredictionArray[4 + 0 * 8] = 
				pPredictionArray[5 + 2 * 8] = 
				pPredictionArray[6 + 4 * 8] = 
				pPredictionArray[7 + 6 * 8] = (P_D + P_E + 1) >> 1;
				pPredictionArray[5 + 0 * 8] = 
				pPredictionArray[6 + 2 * 8] = 
				pPredictionArray[7 + 4 * 8] = (P_E + P_F + 1) >> 1;
				pPredictionArray[6 + 0 * 8] = 
				pPredictionArray[7 + 2 * 8] = (P_F + P_G + 1) >> 1;
				pPredictionArray[7 + 0 * 8] = (P_G + P_H + 1) >> 1;
				pPredictionArray[0 + 1 * 8] = 
				pPredictionArray[1 + 3 * 8] = 
				pPredictionArray[2 + 5 * 8] = 
				pPredictionArray[3 + 7 * 8] = (P_Q + P_A + 2*P_Z + 2) >> 2;
				pPredictionArray[1 + 1 * 8] = 
				pPredictionArray[2 + 3 * 8] = 
				pPredictionArray[3 + 5 * 8] = 
				pPredictionArray[4 + 7 * 8] = (P_Z + P_B + 2*P_A + 2) >> 2;
				pPredictionArray[2 + 1 * 8] = 
				pPredictionArray[3 + 3 * 8] = 
				pPredictionArray[4 + 5 * 8] = 
				pPredictionArray[5 + 7 * 8] = (P_A + P_C + 2*P_B + 2) >> 2;
				pPredictionArray[3 + 1 * 8] = 
				pPredictionArray[4 + 3 * 8] = 
				pPredictionArray[5 + 5 * 8] = 
				pPredictionArray[6 + 7 * 8] = (P_B + P_D + 2*P_C + 2) >> 2;
				pPredictionArray[4 + 1 * 8] = 
				pPredictionArray[5 + 3 * 8] = 
				pPredictionArray[6 + 5 * 8] = 
				pPredictionArray[7 + 7 * 8] = (P_C + P_E + 2*P_D + 2) >> 2;
				pPredictionArray[5 + 1 * 8] = 
				pPredictionArray[6 + 3 * 8] = 
				pPredictionArray[7 + 5 * 8] = (P_D + P_F + 2*P_E + 2) >> 2;
				pPredictionArray[6 + 1 * 8] = 
				pPredictionArray[7 + 3 * 8] = (P_E + P_G + 2*P_F + 2) >> 2;
				pPredictionArray[7 + 1 * 8] = (P_F + P_H + 2*P_G + 2) >> 2;
				pPredictionArray[0 + 2 * 8] =
				pPredictionArray[1 + 4 * 8] =
				pPredictionArray[2 + 6 * 8] = (P_R + P_Z + 2*P_Q + 2) >> 2;
				pPredictionArray[0 + 3 * 8] =
				pPredictionArray[1 + 5 * 8] =
				pPredictionArray[2 + 7 * 8] = (P_S + P_Q + 2*P_R + 2) >> 2;
				pPredictionArray[0 + 4 * 8] =
				pPredictionArray[1 + 6 * 8] = (P_T + P_R + 2*P_S + 2) >> 2;
				pPredictionArray[0 + 5 * 8] =
				pPredictionArray[1 + 7 * 8] = (P_U + P_S + 2*P_T + 2) >> 2;
				pPredictionArray[0 + 6 * 8] = (P_V + P_T + 2*P_U + 2) >> 2;
				pPredictionArray[0 + 7 * 8] = (P_W + P_U + 2*P_V + 2) >> 2;
			
				break;


			case  IPP_4x4_HD:/* diagonal prediction -22.5 deg to horizontal plane */
				//if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
				//printf ("warning: Intra_4x4_Horizontal_Down prediction mode not allowed at mb %d\n",img->current_mb_nr);
				pPredictionArray[0 + 0 * 8] = 
				pPredictionArray[2 + 1 * 8] = 
				pPredictionArray[4 + 2 * 8] = 
				pPredictionArray[6 + 3 * 8] = (P_Q + P_Z + 1) >> 1;
				pPredictionArray[0 + 1 * 8] = 
				pPredictionArray[2 + 2 * 8] = 
				pPredictionArray[4 + 3 * 8] = 
				pPredictionArray[6 + 4 * 8] = (P_R + P_Q + 1) >> 1;
				pPredictionArray[0 + 2 * 8] = 
				pPredictionArray[2 + 3 * 8] = 
				pPredictionArray[4 + 4 * 8] = 
				pPredictionArray[6 + 5 * 8] = (P_S + P_R + 1) >> 1;
				pPredictionArray[0 + 3 * 8] = 
				pPredictionArray[2 + 4 * 8] = 
				pPredictionArray[4 + 5 * 8] = 
				pPredictionArray[6 + 6 * 8] = (P_T + P_S + 1) >> 1;
				pPredictionArray[0 + 4 * 8] = 
				pPredictionArray[2 + 5 * 8] = 
				pPredictionArray[4 + 6 * 8] = 
				pPredictionArray[6 + 7 * 8] = (P_U + P_T + 1) >> 1;
				pPredictionArray[0 + 5 * 8] = 
				pPredictionArray[2 + 6 * 8] = 
				pPredictionArray[4 + 7 * 8] = (P_V + P_U + 1) >> 1;
				pPredictionArray[0 + 6 * 8] = 
				pPredictionArray[2 + 7 * 8] = (P_W + P_V + 1) >> 1;
				pPredictionArray[0 + 7 * 8] = (P_X + P_W + 1) >> 1;
				pPredictionArray[1 + 0 * 8] =
				pPredictionArray[3 + 1 * 8] =
				pPredictionArray[5 + 2 * 8] =
				pPredictionArray[7 + 3 * 8] = (P_Q + P_A + 2*P_Z + 2) >> 2;
				pPredictionArray[1 + 1 * 8] =
				pPredictionArray[3 + 2 * 8] =
				pPredictionArray[5 + 3 * 8] =
				pPredictionArray[7 + 4 * 8] = (P_Z + P_R + 2*P_Q + 2) >> 2;
				pPredictionArray[1 + 2 * 8] =
				pPredictionArray[3 + 3 * 8] =
				pPredictionArray[5 + 4 * 8] =
				pPredictionArray[7 + 5 * 8] = (P_Q + P_S + 2*P_R + 2) >> 2;
				pPredictionArray[1 + 3 * 8] =
				pPredictionArray[3 + 4 * 8] =
				pPredictionArray[5 + 5 * 8] =
				pPredictionArray[7 + 6 * 8] = (P_R + P_T + 2*P_S + 2) >> 2;
				pPredictionArray[1 + 4 * 8] =
				pPredictionArray[3 + 5 * 8] =
				pPredictionArray[5 + 6 * 8] =
				pPredictionArray[7 + 7 * 8] = (P_S + P_U + 2*P_T + 2) >> 2;
				pPredictionArray[1 + 5 * 8] =
				pPredictionArray[3 + 6 * 8] =
				pPredictionArray[5 + 7 * 8] = (P_T + P_V + 2*P_U + 2) >> 2;
				pPredictionArray[1 + 6 * 8] =
				pPredictionArray[3 + 7 * 8] = (P_U + P_W + 2*P_V + 2) >> 2;
				pPredictionArray[1 + 7 * 8] = (P_V + P_X + 2*P_W + 2) >> 2;
				pPredictionArray[2 + 0 * 8] = 
				pPredictionArray[4 + 1 * 8] = 
				pPredictionArray[6 + 2 * 8] = (P_Z + P_B + 2*P_A + 2) >> 2;
				pPredictionArray[3 + 0 * 8] = 
				pPredictionArray[5 + 1 * 8] = 
				pPredictionArray[7 + 2 * 8] = (P_A + P_C + 2*P_B + 2) >> 2;
				pPredictionArray[4 + 0 * 8] = 
				pPredictionArray[6 + 1 * 8] = (P_B + P_D + 2*P_C + 2) >> 2;
				pPredictionArray[5 + 0 * 8] = 
				pPredictionArray[7 + 1 * 8] = (P_C + P_E + 2*P_D + 2) >> 2;
				pPredictionArray[6 + 0 * 8] = (P_D + P_F + 2*P_E + 2) >> 2;
				pPredictionArray[7 + 0 * 8] = (P_E + P_G + 2*P_F + 2) >> 2;

				break;

			case  IPP_4x4_VL:/* diagonal prediction -22.5 deg to horizontal plane */
				//if (!block_available_up)
				//printf ("warning: Intra_4x4_Vertical_Left prediction mode not allowed at mb %d\n",img->current_mb_nr);
				pPredictionArray[0 + 0 * 8] = (P_A + P_B + 1) >> 1;
				pPredictionArray[1 + 0 * 8] = 
				pPredictionArray[0 + 2 * 8] = (P_B + P_C + 1) >> 1;
				pPredictionArray[2 + 0 * 8] = 
				pPredictionArray[1 + 2 * 8] = 
				pPredictionArray[0 + 4 * 8] = (P_C + P_D + 1) >> 1;
				pPredictionArray[3 + 0 * 8] = 
				pPredictionArray[2 + 2 * 8] = 
				pPredictionArray[1 + 4 * 8] = 
				pPredictionArray[0 + 6 * 8] = (P_D + P_E + 1) >> 1;
				pPredictionArray[4 + 0 * 8] = 
				pPredictionArray[3 + 2 * 8] = 
				pPredictionArray[2 + 4 * 8] = 
				pPredictionArray[1 + 6 * 8] = (P_E + P_F + 1) >> 1;
				pPredictionArray[5 + 0 * 8] = 
				pPredictionArray[4 + 2 * 8] = 
				pPredictionArray[3 + 4 * 8] = 
				pPredictionArray[2 + 6 * 8] = (P_F + P_G + 1) >> 1;
				pPredictionArray[6 + 0 * 8] = 
				pPredictionArray[5 + 2 * 8] = 
				pPredictionArray[4 + 4 * 8] = 
				pPredictionArray[3 + 6 * 8] = (P_G + P_H + 1) >> 1;
				pPredictionArray[7 + 0 * 8] = 
				pPredictionArray[6 + 2 * 8] = 
				pPredictionArray[5 + 4 * 8] = 
				pPredictionArray[4 + 6 * 8] = (P_H + P_I + 1) >> 1;
				pPredictionArray[7 + 2 * 8] = 
				pPredictionArray[6 + 4 * 8] = 
				pPredictionArray[5 + 6 * 8] = (P_I + P_J + 1) >> 1;
				pPredictionArray[7 + 4 * 8] = 
				pPredictionArray[6 + 6 * 8] = (P_J + P_K + 1) >> 1;
				pPredictionArray[7 + 6 * 8] = (P_K + P_L + 1) >> 1;
				pPredictionArray[0 + 1 * 8] = (P_A + P_C + 2*P_B + 2) >> 2;
				pPredictionArray[1 + 1 * 8] = 
				pPredictionArray[0 + 3 * 8] = (P_B + P_D + 2*P_C + 2) >> 2;
				pPredictionArray[2 + 1 * 8] = 
				pPredictionArray[1 + 3 * 8] = 
				pPredictionArray[0 + 5 * 8] = (P_C + P_E + 2*P_D + 2) >> 2;
				pPredictionArray[3 + 1 * 8] = 
				pPredictionArray[2 + 3 * 8] = 
				pPredictionArray[1 + 5 * 8] = 
				pPredictionArray[0 + 7 * 8] = (P_D + P_F + 2*P_E + 2) >> 2;
				pPredictionArray[4 + 1 * 8] = 
				pPredictionArray[3 + 3 * 8] = 
				pPredictionArray[2 + 5 * 8] = 
				pPredictionArray[1 + 7 * 8] = (P_E + P_G + 2*P_F + 2) >> 2;
				pPredictionArray[5 + 1 * 8] = 
				pPredictionArray[4 + 3 * 8] = 
				pPredictionArray[3 + 5 * 8] = 
				pPredictionArray[2 + 7 * 8] = (P_F + P_H + 2*P_G + 2) >> 2;
				pPredictionArray[6 + 1 * 8] = 
				pPredictionArray[5 + 3 * 8] = 
				pPredictionArray[4 + 5 * 8] = 
				pPredictionArray[3 + 7 * 8] = (P_G + P_I + 2*P_H + 2) >> 2;
				pPredictionArray[7 + 1 * 8] = 
				pPredictionArray[6 + 3 * 8] = 
				pPredictionArray[5 + 5 * 8] = 
				pPredictionArray[4 + 7 * 8] = (P_H + P_J + 2*P_I + 2) >> 2;
				pPredictionArray[7 + 3 * 8] = 
				pPredictionArray[6 + 5 * 8] = 
				pPredictionArray[5 + 7 * 8] = (P_I + P_K + 2*P_J + 2) >> 2;
				pPredictionArray[7 + 5 * 8] = 
				pPredictionArray[6 + 7 * 8] = (P_J + P_L + 2*P_K + 2) >> 2;
				pPredictionArray[7 + 7 * 8] = (P_K + P_M + 2*P_L + 2) >> 2;
			    
				break;

			case  IPP_4x4_HU:/* diagonal prediction -22.5 deg to horizontal plane */
				//if (!block_available_left)
				//printf ("warning: Intra_4x4_Horizontal_Up prediction mode not allowed at mb %d\n",img->current_mb_nr);
				pPredictionArray[0 + 0 * 8] = (P_Q + P_R + 1) >> 1;
				pPredictionArray[0 + 1 * 8] =
				pPredictionArray[2 + 0 * 8] = (P_R + P_S + 1) >> 1;
				pPredictionArray[0 + 2 * 8] =
				pPredictionArray[2 + 1 * 8] =
				pPredictionArray[4 + 0 * 8] = (P_S + P_T + 1) >> 1;
				pPredictionArray[0 + 3 * 8] =
				pPredictionArray[2 + 2 * 8] =
				pPredictionArray[4 + 1 * 8] =
				pPredictionArray[6 + 0 * 8] = (P_T + P_U + 1) >> 1;
				pPredictionArray[0 + 4 * 8] =
				pPredictionArray[2 + 3 * 8] =
				pPredictionArray[4 + 2 * 8] =
				pPredictionArray[6 + 1 * 8] = (P_U + P_V + 1) >> 1;
				pPredictionArray[0 + 5 * 8] =
				pPredictionArray[2 + 4 * 8] =
				pPredictionArray[4 + 3 * 8] =
				pPredictionArray[6 + 2 * 8] = (P_V + P_W + 1) >> 1;
				pPredictionArray[0 + 6 * 8] =
				pPredictionArray[2 + 5 * 8] =
				pPredictionArray[4 + 4 * 8] =
				pPredictionArray[6 + 3 * 8] = (P_W + P_X + 1) >> 1;
				pPredictionArray[6 + 4 * 8] =
				pPredictionArray[7 + 4 * 8] =
				pPredictionArray[4 + 5 * 8] =
				pPredictionArray[5 + 5 * 8] =
				pPredictionArray[6 + 5 * 8] =
				pPredictionArray[7 + 5 * 8] =
				pPredictionArray[2 + 6 * 8] =
				pPredictionArray[3 + 6 * 8] =
				pPredictionArray[4 + 6 * 8] =
				pPredictionArray[5 + 6 * 8] =
				pPredictionArray[6 + 6 * 8] =
				pPredictionArray[7 + 6 * 8] =
				pPredictionArray[0 + 7 * 8] =
				pPredictionArray[1 + 7 * 8] =
				pPredictionArray[2 + 7 * 8] =
				pPredictionArray[3 + 7 * 8] =
				pPredictionArray[4 + 7 * 8] =
				pPredictionArray[5 + 7 * 8] =
				pPredictionArray[6 + 7 * 8] =
				pPredictionArray[7 + 7 * 8] = P_X;
				pPredictionArray[1 + 6 * 8] =
				pPredictionArray[3 + 5 * 8] =
				pPredictionArray[5 + 4 * 8] =
				pPredictionArray[7 + 3 * 8] = (P_W + 3*P_X + 2) >> 2;
				pPredictionArray[1 + 5 * 8] =
				pPredictionArray[3 + 4 * 8] =
				pPredictionArray[5 + 3 * 8] =
				pPredictionArray[7 + 2 * 8] = (P_X + P_V + 2*P_W + 2) >> 2;
				pPredictionArray[1 + 4 * 8] =
				pPredictionArray[3 + 3 * 8] =
				pPredictionArray[5 + 2 * 8] =
				pPredictionArray[7 + 1 * 8] = (P_W + P_U + 2*P_V + 2) >> 2;
				pPredictionArray[1 + 3 * 8] =
				pPredictionArray[3 + 2 * 8] =
				pPredictionArray[5 + 1 * 8] =
				pPredictionArray[7 + 0 * 8] = (P_V + P_T + 2*P_U + 2) >> 2;
				pPredictionArray[1 + 2 * 8] =
				pPredictionArray[3 + 1 * 8] =
				pPredictionArray[5 + 0 * 8] = (P_U + P_S + 2*P_T + 2) >> 2;
				pPredictionArray[1 + 1 * 8] =
				pPredictionArray[3 + 0 * 8] = (P_T + P_R + 2*P_S + 2) >> 2;
				pPredictionArray[1 + 0 * 8] = (P_S + P_Q + 2*P_R + 2) >> 2;

				break;

			default:
				printf("Error: illegal intra_4x4 prediction mode\n");
				return ippStsErr;
				break;
		}



		// Make decision for AC components
#if 1
		if( cbp & cbpmask_8x8[k])		// cbpmask
		{
			// Form acArray and do IQ
			// ITU page288(version0503)
			if(QP >=36)
			{
				for(i=0; i<64; i++)
				{
					//if((*ppSrcCoeff)[i])
						acArray[i] = ((*ppSrcDstCoeff)[i] * pQuantTable[i]) << (qp_per -6);
				}
			}
			else
			{
				for(i=0; i<64; i++)
				{
					//if((*ppSrcCoeff)[i])
						acArray[i] = ((*ppSrcDstCoeff)[i] * pQuantTable[i]) >> (6 - qp_per);
				}
			}

			pacArray = acArray;

			// Do Inverse transform
			// horizontal
			int a[8], b[8];
			int *m7;
			for( i = 0; i < 8; i++)
			{
				a[0] =  pacArray[0] + pacArray[4];
				a[4] =  pacArray[0] - pacArray[4];
				a[2] = (pacArray[2]>>1) - pacArray[6];
				a[6] =  pacArray[2] + (pacArray[6]>>1);

				b[0] = a[0] + a[6];
				b[2] = a[4] + a[2];
				b[4] = a[4] - a[2];
				b[6] = a[0] - a[6];

				a[1] = -pacArray[3] + pacArray[5] - pacArray[7] - (pacArray[7]>>1);
				a[3] =  pacArray[1] + pacArray[7] - pacArray[3] - (pacArray[3]>>1);
				a[5] = -pacArray[1] + pacArray[7] + pacArray[5] + (pacArray[5]>>1);
				a[7] =  pacArray[3] + pacArray[5] + pacArray[1] + (pacArray[1]>>1);

				b[1] =   a[1] + (a[7]>>2);
				b[7] = -(a[1]>>2) + a[7];
				b[3] =   a[3] + (a[5]>>2);
				b[5] =  (a[3]>>2) - a[5];

				m6[0][i] = b[0] + b[7];
				m6[1][i] = b[2] + b[5];
				m6[2][i] = b[4] + b[3];
				m6[3][i] = b[6] + b[1];
				m6[4][i] = b[6] - b[1];
				m6[5][i] = b[4] - b[3];
				m6[6][i] = b[2] - b[5];
				m6[7][i] = b[0] - b[7];

				pacArray += 8;
			}
			for( i = 0; i < 8; i++)
			{
				m7 = m6[i]; 
				a[0] = m7[0] + m7[4];
				a[4] = m7[0] - m7[4];
				a[2] = (m7[2]>>1) - m7[6];
				a[6] =  m7[2] + (m7[6]>>1);

				b[0] = a[0] + a[6];
				b[2] = a[4] + a[2];
				b[4] = a[4] - a[2];
				b[6] = a[0] - a[6];

				a[1] = -m7[3] + m7[5] - m7[7] - (m7[7]>>1);
				a[3] =  m7[1] + m7[7] - m7[3] - (m7[3]>>1);
				a[5] = -m7[1] + m7[7] + m7[5] + (m7[5]>>1);
				a[7] =  m7[3] + m7[5] + m7[1] + (m7[1]>>1);

				b[1] =   a[1] + (a[7]>>2);
				b[7] = -(a[1]>>2) + a[7];
				b[3] =   a[3] + (a[5]>>2);
				b[5] =  (a[3]>>2) - a[5];

				Array_idct[0][i] = b[0] + b[7];
				Array_idct[1][i] = b[2] + b[5];
				Array_idct[2][i] = b[4] + b[3];
				Array_idct[3][i] = b[6] + b[1];
				Array_idct[4][i] = b[6] - b[1];
				Array_idct[5][i] = b[4] - b[3];
				Array_idct[6][i] = b[2] - b[5];
				Array_idct[7][i] = b[0] - b[7];
			}


			// MUST SAVE this result to array for next prediction
			for(j=0; j<8; j++)
				for(i=0; i<8; i++)
				{
					pSrcforPrediction[(j)*srcdstYStep + i] = IClip(0, 255, (((pPredictionArray[j*8+i] << DQ_BITS_8) + Array_idct[j][i] + (1<< (DQ_BITS_8-1)) ) >> DQ_BITS_8));
				}

			*ppSrcDstCoeff += 64;
		}
		else
#endif
		{
			// MUST SAVE this result to array for next prediction
			for(t1=0,t2=0,  j=0; j<8; j++)
			{
				//for(i=0; i<4; i++)
				//	pSrcforPrediction[t1 + i] = pPredictionArray[t2 + i];
				memcpy(&pSrcforPrediction[t1], &pPredictionArray[t2], 8);
				t1 += srcdstYStep;
				t2 += 8;
			}
		}	
	}

	Profile_End(ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_c_profile);

	return ippStsNoErr;
}
IppStatus __STDCALL ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_c(Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstYPlane,
            Ipp32s srcdstYStep,
            IppIntra8x8PredMode_H264 *pMBIntraTypes,
            Ipp32u cbp8x2,
            Ipp32u QP,
            Ipp8u edge_type,
            Ipp16s *pQuantTable,
            Ipp8u bypass_flag)
{
	// 和上面函数的不同在于两点：
    //  1, Loop from 4 --> 2
    //  2, cbp8x8 --> cbp8x2
	/*  Prediction mode
	  IPP_4x4_VERT     = 0,
	  IPP_4x4_HOR      = 1,
	  IPP_4x4_DC       = 2,
	  IPP_4x4_DIAG_DL  = 3,
	  IPP_4x4_DIAG_DR  = 4,
	  IPP_4x4_VR       = 5,
	  IPP_4x4_HD       = 6,
	  IPP_4x4_VL       = 7,
	  IPP_4x4_HU       = 8
	*/
	_DECLSPEC unsigned char prediction_array[64];

	_DECLSPEC int		avail_left[4]	    = { 1, 1, 1, 1};
	_DECLSPEC int		avail_top [4]	    = { 1, 1, 1, 1};
	_DECLSPEC int		avail_top_left[4]   = { 1, 1, 1, 1};
	_DECLSPEC int		avail_top_right[4]  = { 1, 1, 1, 0};

	_DECLSPEC int		acArray[64];
	_DECLSPEC int		PredPel[64];  // array of predictor pels
	_DECLSPEC int		m6[8][8], Array_idct[8][8];

	int		block_x, block_y;

	unsigned char * pPredictionArray;

	int		i,	j, k, t1, t2;
	Ipp8u	*pSrcforPrediction;

	int		pitch = srcdstYStep;

	int		s0;				// Used for DC prediction

	int		qp_per, qp_rem;

	unsigned int  cbp = cbp8x2 & 0x03;		// Need 5 bits to represent Luma information 
	int *pacArray;


	Profile_Start(ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_c_profile);
 
	qp_per = QP/6;
	qp_rem = QP%6;


	// First, we do intra-prediction according to edge_type and intra_luma_mode!!!!!!!!!!!!!!!!!!!!
	// 1-1, we need compute avail array for three array [ left, top and top_left]
	// TOP first
	avail_top[0] = 
	avail_top[1] = !(edge_type&IPPVC_TOP_EDGE);

	// Then LEFT
	avail_left[0] = 
	avail_left[2] = !(edge_type&IPPVC_LEFT_EDGE);

	// Last TOP_LEFT
	avail_top_left[0] =  !(edge_type&IPPVC_TOP_LEFT_EDGE);
	avail_top_left[1] =  !(edge_type&IPPVC_TOP_EDGE);
	avail_top_left[2] =  !(edge_type&IPPVC_LEFT_EDGE);

	// TOP_RIGHT
	avail_top_right[0] = !(edge_type&IPPVC_TOP_EDGE);
	avail_top_right[1] = !(edge_type&IPPVC_TOP_RIGHT_EDGE);

	for(k=0; k<2; k++)			// In scan order (4x4 blocks)
	{
		block_x = ac_block8x8_idx_x[k];
		block_y = ac_block8x8_idx_y[k];

		pSrcforPrediction = pSrcDstYPlane + block_y*srcdstYStep + block_x;			// Pointer to top_left cornor of one 4x4 block
		// Prepare array for prediction
		if (avail_top_left[k])
		{
			P_Z = pSrcforPrediction[-srcdstYStep - 1];

		}
		else
		{
			P_Z = 128;

		}		
		
		if (avail_top[k])
		{
			P_A = pSrcforPrediction[-srcdstYStep    ];
			P_B = pSrcforPrediction[-srcdstYStep + 1];
			P_C = pSrcforPrediction[-srcdstYStep + 2];
			P_D = pSrcforPrediction[-srcdstYStep + 3];
			P_E = pSrcforPrediction[-srcdstYStep + 4];
			P_F = pSrcforPrediction[-srcdstYStep + 5];
			P_G = pSrcforPrediction[-srcdstYStep + 6];
			P_H = pSrcforPrediction[-srcdstYStep + 7];
	
		}
		else
		{
			P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = 128;

		}

		if (avail_top_right[k])
		{
			P_I = pSrcforPrediction[-srcdstYStep + 8];
			P_J = pSrcforPrediction[-srcdstYStep + 9];
			P_K = pSrcforPrediction[-srcdstYStep + 10];
			P_L = pSrcforPrediction[-srcdstYStep + 11];
			P_M = pSrcforPrediction[-srcdstYStep + 12];
			P_N = pSrcforPrediction[-srcdstYStep + 13];
			P_O = pSrcforPrediction[-srcdstYStep + 14];
			P_P = pSrcforPrediction[-srcdstYStep + 15];

		}
		else
		{
			P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = P_H;

		}

		if (avail_left[k])
		{
			P_Q = pSrcforPrediction[              -1];
			P_R = pSrcforPrediction[  srcdstYStep -1];
			P_S = pSrcforPrediction[srcdstYStep + srcdstYStep -1];
			P_T = pSrcforPrediction[(srcdstYStep<<1) + srcdstYStep -1];

			P_U = pSrcforPrediction[4*  srcdstYStep -1];
			P_V = pSrcforPrediction[5*  srcdstYStep -1];
			P_W = pSrcforPrediction[6*  srcdstYStep -1];
			P_X = pSrcforPrediction[7*  srcdstYStep -1];

		}
		else
		{
			P_Q = P_R = P_S = P_T = P_U = P_V = P_W = P_X = 128;

		}

		LowPassForIntra8x8Pred(&(P_Z), avail_top_left[k], avail_top[k], avail_left[k]);

		pPredictionArray = &prediction_array[0];
		// Form prediction result into corresponding position
		switch(pMBIntraTypes[k])
		{

			case IPP_4x4_VERT:                       /* vertical prediction from block above */
				for(j=0;j<64;j+=8)
				for(i=0;i<8;i++)
					pPredictionArray[j+i] = PredPel[1+i];	

				break;

			case IPP_4x4_HOR:                        /* horizontal prediction from left block */

				for(j=0;j<8;j++)
				for(i=0;i<8;i++)
					pPredictionArray[j*8+i] = PredPel[17 + j];
		
				break;


			case IPP_4x4_DC:                         /* DC prediction */

				s0 = 0;
				if (avail_top[k] && avail_left[k])
				{   
					// no edge
					//s0 = (P_A + P_B + P_C + P_D + P_I + P_J + P_K + P_L + 4)>>3;
					s0 = (P_A + P_B + P_C + P_D + P_E + P_F + P_G + P_H + P_Q + P_R + P_S + P_T + P_U + P_V + P_W + P_X + 8) >> 4;
				}
				else if (!avail_top[k] && avail_left[k])
				{
					// upper edge
					//s0 = (P_I + P_J + P_K + P_L + 2)>>2;             
					s0 = (P_Q + P_R + P_S + P_T + P_U + P_V + P_W + P_X + 4) >> 3;             
				}
				else if (avail_top[k] && !avail_left[k])
				{
					// left edge
					//s0 = (P_A + P_B + P_C + P_D + 2)>>2;             
					s0 = (P_A + P_B + P_C + P_D + P_E + P_F + P_G + P_H + 4) >> 3;             
				}
				else //if (!block_available_up && !block_available_left)
				{
					// top left corner, nothing to predict from
					s0 = 128;                           
				}

				for (j=0; j < 64; j+=8)
				{
					for (i=0; i < 8; i++)
					{
						// store DC prediction
						pPredictionArray[j+i] = s0;
					}
				}

				break;

			case IPP_4x4_DIAG_DL:
				if (!avail_top[k])
				printf ("warning: Intra_4x4_Diagonal_Down_Left prediction mode not allowed at mb \n");

				// Mode DIAG_DOWN_LEFT_PRED
				// Shiquan: here can refer x+y=0..13
				pPredictionArray[0 + 0 * 8] = (P_A + P_C + 2*(P_B) + 2) >> 2;
				pPredictionArray[0 + 1 * 8] = 
				pPredictionArray[1 + 0 * 8] = (P_B + P_D + 2*(P_C) + 2) >> 2;
				pPredictionArray[0 + 2 * 8] =
				pPredictionArray[1 + 1 * 8] =
				pPredictionArray[2 + 0 * 8] = (P_C + P_E + 2*(P_D) + 2) >> 2;
				pPredictionArray[0 + 3 * 8] = 
				pPredictionArray[1 + 2 * 8] = 
				pPredictionArray[2 + 1 * 8] = 
				pPredictionArray[3 + 0 * 8] = (P_D + P_F + 2*(P_E) + 2) >> 2;
				pPredictionArray[0 + 4 * 8] = 
				pPredictionArray[1 + 3 * 8] = 
				pPredictionArray[2 + 2 * 8] = 
				pPredictionArray[3 + 1 * 8] = 
				pPredictionArray[4 + 0 * 8] = (P_E + P_G + 2*(P_F) + 2) >> 2;
				pPredictionArray[0 + 5 * 8] = 
				pPredictionArray[1 + 4 * 8] = 
				pPredictionArray[2 + 3 * 8] = 
				pPredictionArray[3 + 2 * 8] = 
				pPredictionArray[4 + 1 * 8] = 
				pPredictionArray[5 + 0 * 8] = (P_F + P_H + 2*(P_G) + 2) >> 2;
				pPredictionArray[0 + 6 * 8] = 
				pPredictionArray[1 + 5 * 8] = 
				pPredictionArray[2 + 4 * 8] = 
				pPredictionArray[3 + 3 * 8] = 
				pPredictionArray[4 + 2 * 8] = 
				pPredictionArray[5 + 1 * 8] = 
				pPredictionArray[6 + 0 * 8] = (P_G + P_I + 2*(P_H) + 2) >> 2;
				pPredictionArray[0 + 7 * 8] = 
				pPredictionArray[1 + 6 * 8] = 
				pPredictionArray[2 + 5 * 8] = 
				pPredictionArray[3 + 4 * 8] = 
				pPredictionArray[4 + 3 * 8] = 
				pPredictionArray[5 + 2 * 8] = 
				pPredictionArray[6 + 1 * 8] = 
				pPredictionArray[7 + 0 * 8] = (P_H + P_J + 2*(P_I) + 2) >> 2;
				pPredictionArray[1 + 7 * 8] = 
				pPredictionArray[2 + 6 * 8] = 
				pPredictionArray[3 + 5 * 8] = 
				pPredictionArray[4 + 4 * 8] = 
				pPredictionArray[5 + 3 * 8] = 
				pPredictionArray[6 + 2 * 8] = 
				pPredictionArray[7 + 1 * 8] = (P_I + P_K + 2*(P_J) + 2) >> 2;
				pPredictionArray[2 + 7 * 8] = 
				pPredictionArray[3 + 6 * 8] = 
				pPredictionArray[4 + 5 * 8] = 
				pPredictionArray[5 + 4 * 8] = 
				pPredictionArray[6 + 3 * 8] = 
				pPredictionArray[7 + 2 * 8] = (P_J + P_L + 2*(P_K) + 2) >> 2;
				pPredictionArray[3 + 7 * 8] = 
				pPredictionArray[4 + 6 * 8] = 
				pPredictionArray[5 + 5 * 8] = 
				pPredictionArray[6 + 4 * 8] = 
				pPredictionArray[7 + 3 * 8] = (P_K + P_M + 2*(P_L) + 2) >> 2;
				pPredictionArray[4 + 7 * 8] = 
				pPredictionArray[5 + 6 * 8] = 
				pPredictionArray[6 + 5 * 8] = 
				pPredictionArray[7 + 4 * 8] = (P_L + P_N + 2*(P_M) + 2) >> 2;
				pPredictionArray[5 + 7 * 8] = 
				pPredictionArray[6 + 6 * 8] = 
				pPredictionArray[7 + 5 * 8] = (P_M + P_O + 2*(P_N) + 2) >> 2;
				pPredictionArray[6 + 7 * 8] = 
				pPredictionArray[7 + 6 * 8] = (P_N + P_P + 2*(P_O) + 2) >> 2;
				pPredictionArray[7 + 7 * 8] = (P_O + 3*(P_P) + 2) >> 2;
			
				break;

			case IPP_4x4_DIAG_DR:
				//if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
				//printf ("warning: Intra_4x4_Diagonal_Down_Right prediction mode not allowed at mb %d\n",img->current_mb_nr);
				// Mode DIAG_DOWN_RIGHT_PRED
				pPredictionArray[0 + 7 * 8] = (P_X + P_V + 2*(P_W) + 2) >> 2;
				pPredictionArray[0 + 6 * 8] = 
				pPredictionArray[1 + 7 * 8] = (P_W + P_U + 2*(P_V) + 2) >> 2;
				pPredictionArray[0 + 5 * 8] = 
				pPredictionArray[1 + 6 * 8] = 
				pPredictionArray[2 + 7 * 8] = (P_V + P_T + 2*(P_U) + 2) >> 2;
				pPredictionArray[0 + 4 * 8] = 
				pPredictionArray[1 + 5 * 8] = 
				pPredictionArray[2 + 6 * 8] = 
				pPredictionArray[3 + 7 * 8] = (P_U + P_S + 2*(P_T) + 2) >> 2;
				pPredictionArray[0 + 3 * 8] = 
				pPredictionArray[1 + 4 * 8] = 
				pPredictionArray[2 + 5 * 8] = 
				pPredictionArray[3 + 6 * 8] = 
				pPredictionArray[4 + 7 * 8] = (P_T + P_R + 2*(P_S) + 2) >> 2;
				pPredictionArray[0 + 2 * 8] = 
				pPredictionArray[1 + 3 * 8] = 
				pPredictionArray[2 + 4 * 8] = 
				pPredictionArray[3 + 5 * 8] = 
				pPredictionArray[4 + 6 * 8] = 
				pPredictionArray[5 + 7 * 8] = (P_S + P_Q + 2*(P_R) + 2) >> 2;
				pPredictionArray[0 + 1 * 8] = 
				pPredictionArray[1 + 2 * 8] = 
				pPredictionArray[2 + 3 * 8] = 
				pPredictionArray[3 + 4 * 8] = 
				pPredictionArray[4 + 5 * 8] = 
				pPredictionArray[5 + 6 * 8] = 
				pPredictionArray[6 + 7 * 8] = (P_R + P_Z + 2*(P_Q) + 2) >> 2;
				pPredictionArray[0 + 0 * 8] = 
				pPredictionArray[1 + 1 * 8] = 
				pPredictionArray[2 + 2 * 8] = 
				pPredictionArray[3 + 3 * 8] = 
				pPredictionArray[4 + 4 * 8] = 
				pPredictionArray[5 + 5 * 8] = 
				pPredictionArray[6 + 6 * 8] = 
				pPredictionArray[7 + 7 * 8] = (P_Q + P_A + 2*(P_Z) + 2) >> 2;
				pPredictionArray[1 + 0 * 8] = 
				pPredictionArray[2 + 1 * 8] = 
				pPredictionArray[3 + 2 * 8] = 
				pPredictionArray[4 + 3 * 8] = 
				pPredictionArray[5 + 4 * 8] = 
				pPredictionArray[6 + 5 * 8] = 
				pPredictionArray[7 + 6 * 8] = (P_Z + P_B + 2*(P_A) + 2) >> 2;
				pPredictionArray[2 + 0 * 8] = 
				pPredictionArray[3 + 1 * 8] = 
				pPredictionArray[4 + 2 * 8] = 
				pPredictionArray[5 + 3 * 8] = 
				pPredictionArray[6 + 4 * 8] = 
				pPredictionArray[7 + 5 * 8] = (P_A + P_C + 2*(P_B) + 2) >> 2;
				pPredictionArray[3 + 0 * 8] = 
				pPredictionArray[4 + 1 * 8] = 
				pPredictionArray[5 + 2 * 8] = 
				pPredictionArray[6 + 3 * 8] = 
				pPredictionArray[7 + 4 * 8] = (P_B + P_D + 2*(P_C) + 2) >> 2;
				pPredictionArray[4 + 0 * 8] = 
				pPredictionArray[5 + 1 * 8] = 
				pPredictionArray[6 + 2 * 8] = 
				pPredictionArray[7 + 3 * 8] = (P_C + P_E + 2*(P_D) + 2) >> 2;
				pPredictionArray[5 + 0 * 8] = 
				pPredictionArray[6 + 1 * 8] = 
				pPredictionArray[7 + 2 * 8] = (P_D + P_F + 2*(P_E) + 2) >> 2;
				pPredictionArray[6 + 0 * 8] = 
				pPredictionArray[7 + 1 * 8] = (P_E + P_G + 2*(P_F) + 2) >> 2;
				pPredictionArray[7 + 0 * 8] = (P_F + P_H + 2*(P_G) + 2) >> 2;		
			
				break;



			case  IPP_4x4_VR:/* diagonal prediction -22.5 deg to horizontal plane */
				//if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
				//printf ("warning: Intra_4x4_Vertical_Right prediction mode not allowed at mb %d\n",img->current_mb_nr);
				pPredictionArray[0 + 0 * 8] = 
				pPredictionArray[1 + 2 * 8] = 
				pPredictionArray[2 + 4 * 8] = 
				pPredictionArray[3 + 6 * 8] = (P_Z + P_A + 1) >> 1;
				pPredictionArray[1 + 0 * 8] = 
				pPredictionArray[2 + 2 * 8] = 
				pPredictionArray[3 + 4 * 8] = 
				pPredictionArray[4 + 6 * 8] = (P_A + P_B + 1) >> 1;
				pPredictionArray[2 + 0 * 8] = 
				pPredictionArray[3 + 2 * 8] = 
				pPredictionArray[4 + 4 * 8] = 
				pPredictionArray[5 + 6 * 8] = (P_B + P_C + 1) >> 1;
				pPredictionArray[3 + 0 * 8] = 
				pPredictionArray[4 + 2 * 8] = 
				pPredictionArray[5 + 4 * 8] = 
				pPredictionArray[6 + 6 * 8] = (P_C + P_D + 1) >> 1;
				pPredictionArray[4 + 0 * 8] = 
				pPredictionArray[5 + 2 * 8] = 
				pPredictionArray[6 + 4 * 8] = 
				pPredictionArray[7 + 6 * 8] = (P_D + P_E + 1) >> 1;
				pPredictionArray[5 + 0 * 8] = 
				pPredictionArray[6 + 2 * 8] = 
				pPredictionArray[7 + 4 * 8] = (P_E + P_F + 1) >> 1;
				pPredictionArray[6 + 0 * 8] = 
				pPredictionArray[7 + 2 * 8] = (P_F + P_G + 1) >> 1;
				pPredictionArray[7 + 0 * 8] = (P_G + P_H + 1) >> 1;
				pPredictionArray[0 + 1 * 8] = 
				pPredictionArray[1 + 3 * 8] = 
				pPredictionArray[2 + 5 * 8] = 
				pPredictionArray[3 + 7 * 8] = (P_Q + P_A + 2*P_Z + 2) >> 2;
				pPredictionArray[1 + 1 * 8] = 
				pPredictionArray[2 + 3 * 8] = 
				pPredictionArray[3 + 5 * 8] = 
				pPredictionArray[4 + 7 * 8] = (P_Z + P_B + 2*P_A + 2) >> 2;
				pPredictionArray[2 + 1 * 8] = 
				pPredictionArray[3 + 3 * 8] = 
				pPredictionArray[4 + 5 * 8] = 
				pPredictionArray[5 + 7 * 8] = (P_A + P_C + 2*P_B + 2) >> 2;
				pPredictionArray[3 + 1 * 8] = 
				pPredictionArray[4 + 3 * 8] = 
				pPredictionArray[5 + 5 * 8] = 
				pPredictionArray[6 + 7 * 8] = (P_B + P_D + 2*P_C + 2) >> 2;
				pPredictionArray[4 + 1 * 8] = 
				pPredictionArray[5 + 3 * 8] = 
				pPredictionArray[6 + 5 * 8] = 
				pPredictionArray[7 + 7 * 8] = (P_C + P_E + 2*P_D + 2) >> 2;
				pPredictionArray[5 + 1 * 8] = 
				pPredictionArray[6 + 3 * 8] = 
				pPredictionArray[7 + 5 * 8] = (P_D + P_F + 2*P_E + 2) >> 2;
				pPredictionArray[6 + 1 * 8] = 
				pPredictionArray[7 + 3 * 8] = (P_E + P_G + 2*P_F + 2) >> 2;
				pPredictionArray[7 + 1 * 8] = (P_F + P_H + 2*P_G + 2) >> 2;
				pPredictionArray[0 + 2 * 8] =
				pPredictionArray[1 + 4 * 8] =
				pPredictionArray[2 + 6 * 8] = (P_R + P_Z + 2*P_Q + 2) >> 2;
				pPredictionArray[0 + 3 * 8] =
				pPredictionArray[1 + 5 * 8] =
				pPredictionArray[2 + 7 * 8] = (P_S + P_Q + 2*P_R + 2) >> 2;
				pPredictionArray[0 + 4 * 8] =
				pPredictionArray[1 + 6 * 8] = (P_T + P_R + 2*P_S + 2) >> 2;
				pPredictionArray[0 + 5 * 8] =
				pPredictionArray[1 + 7 * 8] = (P_U + P_S + 2*P_T + 2) >> 2;
				pPredictionArray[0 + 6 * 8] = (P_V + P_T + 2*P_U + 2) >> 2;
				pPredictionArray[0 + 7 * 8] = (P_W + P_U + 2*P_V + 2) >> 2;
			
				break;


			case  IPP_4x4_HD:/* diagonal prediction -22.5 deg to horizontal plane */
				//if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
				//printf ("warning: Intra_4x4_Horizontal_Down prediction mode not allowed at mb %d\n",img->current_mb_nr);
				pPredictionArray[0 + 0 * 8] = 
				pPredictionArray[2 + 1 * 8] = 
				pPredictionArray[4 + 2 * 8] = 
				pPredictionArray[6 + 3 * 8] = (P_Q + P_Z + 1) >> 1;
				pPredictionArray[0 + 1 * 8] = 
				pPredictionArray[2 + 2 * 8] = 
				pPredictionArray[4 + 3 * 8] = 
				pPredictionArray[6 + 4 * 8] = (P_R + P_Q + 1) >> 1;
				pPredictionArray[0 + 2 * 8] = 
				pPredictionArray[2 + 3 * 8] = 
				pPredictionArray[4 + 4 * 8] = 
				pPredictionArray[6 + 5 * 8] = (P_S + P_R + 1) >> 1;
				pPredictionArray[0 + 3 * 8] = 
				pPredictionArray[2 + 4 * 8] = 
				pPredictionArray[4 + 5 * 8] = 
				pPredictionArray[6 + 6 * 8] = (P_T + P_S + 1) >> 1;
				pPredictionArray[0 + 4 * 8] = 
				pPredictionArray[2 + 5 * 8] = 
				pPredictionArray[4 + 6 * 8] = 
				pPredictionArray[6 + 7 * 8] = (P_U + P_T + 1) >> 1;
				pPredictionArray[0 + 5 * 8] = 
				pPredictionArray[2 + 6 * 8] = 
				pPredictionArray[4 + 7 * 8] = (P_V + P_U + 1) >> 1;
				pPredictionArray[0 + 6 * 8] = 
				pPredictionArray[2 + 7 * 8] = (P_W + P_V + 1) >> 1;
				pPredictionArray[0 + 7 * 8] = (P_X + P_W + 1) >> 1;
				pPredictionArray[1 + 0 * 8] =
				pPredictionArray[3 + 1 * 8] =
				pPredictionArray[5 + 2 * 8] =
				pPredictionArray[7 + 3 * 8] = (P_Q + P_A + 2*P_Z + 2) >> 2;
				pPredictionArray[1 + 1 * 8] =
				pPredictionArray[3 + 2 * 8] =
				pPredictionArray[5 + 3 * 8] =
				pPredictionArray[7 + 4 * 8] = (P_Z + P_R + 2*P_Q + 2) >> 2;
				pPredictionArray[1 + 2 * 8] =
				pPredictionArray[3 + 3 * 8] =
				pPredictionArray[5 + 4 * 8] =
				pPredictionArray[7 + 5 * 8] = (P_Q + P_S + 2*P_R + 2) >> 2;
				pPredictionArray[1 + 3 * 8] =
				pPredictionArray[3 + 4 * 8] =
				pPredictionArray[5 + 5 * 8] =
				pPredictionArray[7 + 6 * 8] = (P_R + P_T + 2*P_S + 2) >> 2;
				pPredictionArray[1 + 4 * 8] =
				pPredictionArray[3 + 5 * 8] =
				pPredictionArray[5 + 6 * 8] =
				pPredictionArray[7 + 7 * 8] = (P_S + P_U + 2*P_T + 2) >> 2;
				pPredictionArray[1 + 5 * 8] =
				pPredictionArray[3 + 6 * 8] =
				pPredictionArray[5 + 7 * 8] = (P_T + P_V + 2*P_U + 2) >> 2;
				pPredictionArray[1 + 6 * 8] =
				pPredictionArray[3 + 7 * 8] = (P_U + P_W + 2*P_V + 2) >> 2;
				pPredictionArray[1 + 7 * 8] = (P_V + P_X + 2*P_W + 2) >> 2;
				pPredictionArray[2 + 0 * 8] = 
				pPredictionArray[4 + 1 * 8] = 
				pPredictionArray[6 + 2 * 8] = (P_Z + P_B + 2*P_A + 2) >> 2;
				pPredictionArray[3 + 0 * 8] = 
				pPredictionArray[5 + 1 * 8] = 
				pPredictionArray[7 + 2 * 8] = (P_A + P_C + 2*P_B + 2) >> 2;
				pPredictionArray[4 + 0 * 8] = 
				pPredictionArray[6 + 1 * 8] = (P_B + P_D + 2*P_C + 2) >> 2;
				pPredictionArray[5 + 0 * 8] = 
				pPredictionArray[7 + 1 * 8] = (P_C + P_E + 2*P_D + 2) >> 2;
				pPredictionArray[6 + 0 * 8] = (P_D + P_F + 2*P_E + 2) >> 2;
				pPredictionArray[7 + 0 * 8] = (P_E + P_G + 2*P_F + 2) >> 2;

				break;

			case  IPP_4x4_VL:/* diagonal prediction -22.5 deg to horizontal plane */
				//if (!block_available_up)
				//printf ("warning: Intra_4x4_Vertical_Left prediction mode not allowed at mb %d\n",img->current_mb_nr);
				pPredictionArray[0 + 0 * 8] = (P_A + P_B + 1) >> 1;
				pPredictionArray[1 + 0 * 8] = 
				pPredictionArray[0 + 2 * 8] = (P_B + P_C + 1) >> 1;
				pPredictionArray[2 + 0 * 8] = 
				pPredictionArray[1 + 2 * 8] = 
				pPredictionArray[0 + 4 * 8] = (P_C + P_D + 1) >> 1;
				pPredictionArray[3 + 0 * 8] = 
				pPredictionArray[2 + 2 * 8] = 
				pPredictionArray[1 + 4 * 8] = 
				pPredictionArray[0 + 6 * 8] = (P_D + P_E + 1) >> 1;
				pPredictionArray[4 + 0 * 8] = 
				pPredictionArray[3 + 2 * 8] = 
				pPredictionArray[2 + 4 * 8] = 
				pPredictionArray[1 + 6 * 8] = (P_E + P_F + 1) >> 1;
				pPredictionArray[5 + 0 * 8] = 
				pPredictionArray[4 + 2 * 8] = 
				pPredictionArray[3 + 4 * 8] = 
				pPredictionArray[2 + 6 * 8] = (P_F + P_G + 1) >> 1;
				pPredictionArray[6 + 0 * 8] = 
				pPredictionArray[5 + 2 * 8] = 
				pPredictionArray[4 + 4 * 8] = 
				pPredictionArray[3 + 6 * 8] = (P_G + P_H + 1) >> 1;
				pPredictionArray[7 + 0 * 8] = 
				pPredictionArray[6 + 2 * 8] = 
				pPredictionArray[5 + 4 * 8] = 
				pPredictionArray[4 + 6 * 8] = (P_H + P_I + 1) >> 1;
				pPredictionArray[7 + 2 * 8] = 
				pPredictionArray[6 + 4 * 8] = 
				pPredictionArray[5 + 6 * 8] = (P_I + P_J + 1) >> 1;
				pPredictionArray[7 + 4 * 8] = 
				pPredictionArray[6 + 6 * 8] = (P_J + P_K + 1) >> 1;
				pPredictionArray[7 + 6 * 8] = (P_K + P_L + 1) >> 1;
				pPredictionArray[0 + 1 * 8] = (P_A + P_C + 2*P_B + 2) >> 2;
				pPredictionArray[1 + 1 * 8] = 
				pPredictionArray[0 + 3 * 8] = (P_B + P_D + 2*P_C + 2) >> 2;
				pPredictionArray[2 + 1 * 8] = 
				pPredictionArray[1 + 3 * 8] = 
				pPredictionArray[0 + 5 * 8] = (P_C + P_E + 2*P_D + 2) >> 2;
				pPredictionArray[3 + 1 * 8] = 
				pPredictionArray[2 + 3 * 8] = 
				pPredictionArray[1 + 5 * 8] = 
				pPredictionArray[0 + 7 * 8] = (P_D + P_F + 2*P_E + 2) >> 2;
				pPredictionArray[4 + 1 * 8] = 
				pPredictionArray[3 + 3 * 8] = 
				pPredictionArray[2 + 5 * 8] = 
				pPredictionArray[1 + 7 * 8] = (P_E + P_G + 2*P_F + 2) >> 2;
				pPredictionArray[5 + 1 * 8] = 
				pPredictionArray[4 + 3 * 8] = 
				pPredictionArray[3 + 5 * 8] = 
				pPredictionArray[2 + 7 * 8] = (P_F + P_H + 2*P_G + 2) >> 2;
				pPredictionArray[6 + 1 * 8] = 
				pPredictionArray[5 + 3 * 8] = 
				pPredictionArray[4 + 5 * 8] = 
				pPredictionArray[3 + 7 * 8] = (P_G + P_I + 2*P_H + 2) >> 2;
				pPredictionArray[7 + 1 * 8] = 
				pPredictionArray[6 + 3 * 8] = 
				pPredictionArray[5 + 5 * 8] = 
				pPredictionArray[4 + 7 * 8] = (P_H + P_J + 2*P_I + 2) >> 2;
				pPredictionArray[7 + 3 * 8] = 
				pPredictionArray[6 + 5 * 8] = 
				pPredictionArray[5 + 7 * 8] = (P_I + P_K + 2*P_J + 2) >> 2;
				pPredictionArray[7 + 5 * 8] = 
				pPredictionArray[6 + 7 * 8] = (P_J + P_L + 2*P_K + 2) >> 2;
				pPredictionArray[7 + 7 * 8] = (P_K + P_M + 2*P_L + 2) >> 2;
			    
				break;

			case  IPP_4x4_HU:/* diagonal prediction -22.5 deg to horizontal plane */
				//if (!block_available_left)
				//printf ("warning: Intra_4x4_Horizontal_Up prediction mode not allowed at mb %d\n",img->current_mb_nr);
				pPredictionArray[0 + 0 * 8] = (P_Q + P_R + 1) >> 1;
				pPredictionArray[0 + 1 * 8] =
				pPredictionArray[2 + 0 * 8] = (P_R + P_S + 1) >> 1;
				pPredictionArray[0 + 2 * 8] =
				pPredictionArray[2 + 1 * 8] =
				pPredictionArray[4 + 0 * 8] = (P_S + P_T + 1) >> 1;
				pPredictionArray[0 + 3 * 8] =
				pPredictionArray[2 + 2 * 8] =
				pPredictionArray[4 + 1 * 8] =
				pPredictionArray[6 + 0 * 8] = (P_T + P_U + 1) >> 1;
				pPredictionArray[0 + 4 * 8] =
				pPredictionArray[2 + 3 * 8] =
				pPredictionArray[4 + 2 * 8] =
				pPredictionArray[6 + 1 * 8] = (P_U + P_V + 1) >> 1;
				pPredictionArray[0 + 5 * 8] =
				pPredictionArray[2 + 4 * 8] =
				pPredictionArray[4 + 3 * 8] =
				pPredictionArray[6 + 2 * 8] = (P_V + P_W + 1) >> 1;
				pPredictionArray[0 + 6 * 8] =
				pPredictionArray[2 + 5 * 8] =
				pPredictionArray[4 + 4 * 8] =
				pPredictionArray[6 + 3 * 8] = (P_W + P_X + 1) >> 1;
				pPredictionArray[6 + 4 * 8] =
				pPredictionArray[7 + 4 * 8] =
				pPredictionArray[4 + 5 * 8] =
				pPredictionArray[5 + 5 * 8] =
				pPredictionArray[6 + 5 * 8] =
				pPredictionArray[7 + 5 * 8] =
				pPredictionArray[2 + 6 * 8] =
				pPredictionArray[3 + 6 * 8] =
				pPredictionArray[4 + 6 * 8] =
				pPredictionArray[5 + 6 * 8] =
				pPredictionArray[6 + 6 * 8] =
				pPredictionArray[7 + 6 * 8] =
				pPredictionArray[0 + 7 * 8] =
				pPredictionArray[1 + 7 * 8] =
				pPredictionArray[2 + 7 * 8] =
				pPredictionArray[3 + 7 * 8] =
				pPredictionArray[4 + 7 * 8] =
				pPredictionArray[5 + 7 * 8] =
				pPredictionArray[6 + 7 * 8] =
				pPredictionArray[7 + 7 * 8] = P_X;
				pPredictionArray[1 + 6 * 8] =
				pPredictionArray[3 + 5 * 8] =
				pPredictionArray[5 + 4 * 8] =
				pPredictionArray[7 + 3 * 8] = (P_W + 3*P_X + 2) >> 2;
				pPredictionArray[1 + 5 * 8] =
				pPredictionArray[3 + 4 * 8] =
				pPredictionArray[5 + 3 * 8] =
				pPredictionArray[7 + 2 * 8] = (P_X + P_V + 2*P_W + 2) >> 2;
				pPredictionArray[1 + 4 * 8] =
				pPredictionArray[3 + 3 * 8] =
				pPredictionArray[5 + 2 * 8] =
				pPredictionArray[7 + 1 * 8] = (P_W + P_U + 2*P_V + 2) >> 2;
				pPredictionArray[1 + 3 * 8] =
				pPredictionArray[3 + 2 * 8] =
				pPredictionArray[5 + 1 * 8] =
				pPredictionArray[7 + 0 * 8] = (P_V + P_T + 2*P_U + 2) >> 2;
				pPredictionArray[1 + 2 * 8] =
				pPredictionArray[3 + 1 * 8] =
				pPredictionArray[5 + 0 * 8] = (P_U + P_S + 2*P_T + 2) >> 2;
				pPredictionArray[1 + 1 * 8] =
				pPredictionArray[3 + 0 * 8] = (P_T + P_R + 2*P_S + 2) >> 2;
				pPredictionArray[1 + 0 * 8] = (P_S + P_Q + 2*P_R + 2) >> 2;

				break;

			default:
				printf("Error: illegal intra_4x4 prediction mode\n");
				return ippStsErr;
				break;
		}



		// Make decision for AC components
#if 1
		if( cbp & cbpmask_8x8[k])		// cbpmask
		{
			// Form acArray and do IQ
			// ITU page288(version0503)
			if(QP >=36)
			{
				for(i=0; i<64; i++)
				{
					//if((*ppSrcCoeff)[i])
						acArray[i] = ((*ppSrcDstCoeff)[i] * pQuantTable[i]) << (qp_per -6);
				}
			}
			else
			{
				for(i=0; i<64; i++)
				{
					//if((*ppSrcCoeff)[i])
						acArray[i] = ((*ppSrcDstCoeff)[i] * pQuantTable[i]) >> (6 - qp_per);
				}
			}

			pacArray = acArray;

			// Do Inverse transform
			// horizontal
			int a[8], b[8];
			int *m7;
			for( i = 0; i < 8; i++)
			{
				a[0] =  pacArray[0] + pacArray[4];
				a[4] =  pacArray[0] - pacArray[4];
				a[2] = (pacArray[2]>>1) - pacArray[6];
				a[6] =  pacArray[2] + (pacArray[6]>>1);

				b[0] = a[0] + a[6];
				b[2] = a[4] + a[2];
				b[4] = a[4] - a[2];
				b[6] = a[0] - a[6];

				a[1] = -pacArray[3] + pacArray[5] - pacArray[7] - (pacArray[7]>>1);
				a[3] =  pacArray[1] + pacArray[7] - pacArray[3] - (pacArray[3]>>1);
				a[5] = -pacArray[1] + pacArray[7] + pacArray[5] + (pacArray[5]>>1);
				a[7] =  pacArray[3] + pacArray[5] + pacArray[1] + (pacArray[1]>>1);

				b[1] =   a[1] + (a[7]>>2);
				b[7] = -(a[1]>>2) + a[7];
				b[3] =   a[3] + (a[5]>>2);
				b[5] =  (a[3]>>2) - a[5];

				m6[0][i] = b[0] + b[7];
				m6[1][i] = b[2] + b[5];
				m6[2][i] = b[4] + b[3];
				m6[3][i] = b[6] + b[1];
				m6[4][i] = b[6] - b[1];
				m6[5][i] = b[4] - b[3];
				m6[6][i] = b[2] - b[5];
				m6[7][i] = b[0] - b[7];

				pacArray += 8;
			}
			for( i = 0; i < 8; i++)
			{
				m7 = m6[i]; 
				a[0] = m7[0] + m7[4];
				a[4] = m7[0] - m7[4];
				a[2] = (m7[2]>>1) - m7[6];
				a[6] =  m7[2] + (m7[6]>>1);

				b[0] = a[0] + a[6];
				b[2] = a[4] + a[2];
				b[4] = a[4] - a[2];
				b[6] = a[0] - a[6];

				a[1] = -m7[3] + m7[5] - m7[7] - (m7[7]>>1);
				a[3] =  m7[1] + m7[7] - m7[3] - (m7[3]>>1);
				a[5] = -m7[1] + m7[7] + m7[5] + (m7[5]>>1);
				a[7] =  m7[3] + m7[5] + m7[1] + (m7[1]>>1);

				b[1] =   a[1] + (a[7]>>2);
				b[7] = -(a[1]>>2) + a[7];
				b[3] =   a[3] + (a[5]>>2);
				b[5] =  (a[3]>>2) - a[5];

				Array_idct[0][i] = b[0] + b[7];
				Array_idct[1][i] = b[2] + b[5];
				Array_idct[2][i] = b[4] + b[3];
				Array_idct[3][i] = b[6] + b[1];
				Array_idct[4][i] = b[6] - b[1];
				Array_idct[5][i] = b[4] - b[3];
				Array_idct[6][i] = b[2] - b[5];
				Array_idct[7][i] = b[0] - b[7];
			}


			// MUST SAVE this result to array for next prediction
			for(j=0; j<8; j++)
				for(i=0; i<8; i++)
				{
					pSrcforPrediction[(j)*srcdstYStep + i] = IClip(0, 255, (((pPredictionArray[j*8+i] << DQ_BITS_8) + Array_idct[j][i] + (1<< (DQ_BITS_8-1)) ) >> DQ_BITS_8));
				}

			*ppSrcDstCoeff += 64;
		}
		else
#endif
		{
			// MUST SAVE this result to array for next prediction
			for(t1=0,t2=0,  j=0; j<8; j++)
			{
				//for(i=0; i<4; i++)
				//	pSrcforPrediction[t1 + i] = pPredictionArray[t2 + i];
				memcpy(&pSrcforPrediction[t1], &pPredictionArray[t2], 8);
				t1 += srcdstYStep;
				t2 += 8;
			}
		}	
	}

	Profile_End(ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_c_profile);

	return ippStsNoErr;
		
}
IppStatus __STDCALL ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_c(Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstYPlane,
            Ipp32u srcdstYStep,
            Ipp32u cbp8x8,
            Ipp32s QP,
            Ipp16s *pQuantTable,
            Ipp8u bypass_flag)

{
	_DECLSPEC int		acArray[64];
	_DECLSPEC int		m6[8][8], Array_idct[8][8];

	int		block_x, block_y;

	int		i,	j, k;
	Ipp8u	*pSrcforPrediction;

	int		pitch = srcdstYStep;

	int		qp_per, qp_rem;

	unsigned int  cbp = cbp8x8 & 0x0f;		// Need 4 bits to represent Luma information 
	int *pacArray;


	Profile_Start(ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_c_profile);

	qp_per = QP/6;
	qp_rem = QP%6;

	for(k=0; k<4; k++)			// In scan order (4x4 blocks)
	{
		block_x = ac_block8x8_idx_x[k];
		block_y = ac_block8x8_idx_y[k];

		pSrcforPrediction = pSrcDstYPlane + block_y*srcdstYStep + block_x;			// Pointer to top_left cornor of one 4x4 block


		// Make decision for AC components
		if( cbp & cbpmask_8x8[k])		// cbpmask
		{
			// Form acArray and do IQ
			// ITU page288(version0503)
			if(QP >=36)
			{
				for(i=0; i<64; i++)
				{
					//if((*ppSrcCoeff)[i])
						acArray[i] = ((*ppSrcDstCoeff)[i] * pQuantTable[i]) << (qp_per -6);
				}
			}
			else
			{
				for(i=0; i<64; i++)
				{
					//if((*ppSrcCoeff)[i])
						acArray[i] = ((*ppSrcDstCoeff)[i] * pQuantTable[i]) >> (6 - qp_per);
				}
			}

			pacArray = acArray;

			// Do Inverse transform
			// horizontal
			int a[8], b[8];
			int *m7;
			for( i = 0; i < 8; i++)
			{
				a[0] =  pacArray[0] + pacArray[4];
				a[4] =  pacArray[0] - pacArray[4];
				a[2] = (pacArray[2]>>1) - pacArray[6];
				a[6] =  pacArray[2] + (pacArray[6]>>1);

				b[0] = a[0] + a[6];
				b[2] = a[4] + a[2];
				b[4] = a[4] - a[2];
				b[6] = a[0] - a[6];

				a[1] = -pacArray[3] + pacArray[5] - pacArray[7] - (pacArray[7]>>1);
				a[3] =  pacArray[1] + pacArray[7] - pacArray[3] - (pacArray[3]>>1);
				a[5] = -pacArray[1] + pacArray[7] + pacArray[5] + (pacArray[5]>>1);
				a[7] =  pacArray[3] + pacArray[5] + pacArray[1] + (pacArray[1]>>1);

				b[1] =   a[1] + (a[7]>>2);
				b[7] = -(a[1]>>2) + a[7];
				b[3] =   a[3] + (a[5]>>2);
				b[5] =  (a[3]>>2) - a[5];

				m6[0][i] = b[0] + b[7];
				m6[1][i] = b[2] + b[5];
				m6[2][i] = b[4] + b[3];
				m6[3][i] = b[6] + b[1];
				m6[4][i] = b[6] - b[1];
				m6[5][i] = b[4] - b[3];
				m6[6][i] = b[2] - b[5];
				m6[7][i] = b[0] - b[7];

				pacArray += 8;
			}
			for( i = 0; i < 8; i++)
			{
				m7 = m6[i]; 
				a[0] = m7[0] + m7[4];
				a[4] = m7[0] - m7[4];
				a[2] = (m7[2]>>1) - m7[6];
				a[6] =  m7[2] + (m7[6]>>1);

				b[0] = a[0] + a[6];
				b[2] = a[4] + a[2];
				b[4] = a[4] - a[2];
				b[6] = a[0] - a[6];

				a[1] = -m7[3] + m7[5] - m7[7] - (m7[7]>>1);
				a[3] =  m7[1] + m7[7] - m7[3] - (m7[3]>>1);
				a[5] = -m7[1] + m7[7] + m7[5] + (m7[5]>>1);
				a[7] =  m7[3] + m7[5] + m7[1] + (m7[1]>>1);

				b[1] =   a[1] + (a[7]>>2);
				b[7] = -(a[1]>>2) + a[7];
				b[3] =   a[3] + (a[5]>>2);
				b[5] =  (a[3]>>2) - a[5];

				Array_idct[0][i] = b[0] + b[7];
				Array_idct[1][i] = b[2] + b[5];
				Array_idct[2][i] = b[4] + b[3];
				Array_idct[3][i] = b[6] + b[1];
				Array_idct[4][i] = b[6] - b[1];
				Array_idct[5][i] = b[4] - b[3];
				Array_idct[6][i] = b[2] - b[5];
				Array_idct[7][i] = b[0] - b[7];
			}


			// MUST SAVE this result to array for next prediction
			for(j=0; j<8; j++)
				for(i=0; i<8; i++)
				{
					pSrcforPrediction[(j)*srcdstYStep + i] = IClip(0, 255, (((pSrcforPrediction[(j)*srcdstYStep + i] << DQ_BITS_8) + Array_idct[j][i] + (1<< (DQ_BITS_8-1)) ) >> DQ_BITS_8));
				}

			*ppSrcDstCoeff += 64;
		}
		else
		{
			continue;
		}	
	}

	Profile_End(ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_c_profile);

	return ippStsNoErr;
}

IppStatus __STDCALL ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_c(Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstYPlane,
            Ipp32s srcdstYStep,
            IppIntra4x4PredMode_H264 *pMBIntraTypes,
            Ipp32u cbp4x4,
            Ipp32u QP,
            Ipp8u edge_type,
            Ipp16s *pQuantTable,
            Ipp8u bypass_flag)
{
	/*  Prediction mode
	  IPP_4x4_VERT     = 0,
	  IPP_4x4_HOR      = 1,
	  IPP_4x4_DC       = 2,
	  IPP_4x4_DIAG_DL  = 3,
	  IPP_4x4_DIAG_DR  = 4,
	  IPP_4x4_VR       = 5,
	  IPP_4x4_HD       = 6,
	  IPP_4x4_VL       = 7,
	  IPP_4x4_HU       = 8
	*/
	_DECLSPEC unsigned char prediction_array[16];

	_DECLSPEC int		avail_left[16]	     = { 1, 1, 1, 1, 1, 1, 1, 1,    1, 1, 1, 1, 1, 1, 1, 1};
	_DECLSPEC int		avail_top [16]	     = { 1, 1, 1, 1, 1, 1, 1, 1,    1, 1, 1, 1, 1, 1, 1, 1};
	_DECLSPEC int		avail_top_left[16]   = { 1, 1, 1, 1, 1, 1, 1, 1,    1, 1, 1, 1, 1, 1, 1, 1};
	_DECLSPEC int		avail_top_right[16]  = { 1, 1, 1, 0, 1, 1, 1, 0,   1, 1, 1, 0, 1, 0, 1, 0};

	_DECLSPEC int		acArray[16];
	_DECLSPEC int		PredPel[16];  // array of predictor pels
	_DECLSPEC int		m7[4][4], m5[4], m6[4];

	int		block_x, block_y;

	unsigned char * pPredictionArray;

	int		i,	j, k, t1, t2;
	Ipp8u	*pSrcforPrediction;

	int		pitch = srcdstYStep;

	int		s0;				// Used for DC prediction

	int		qp_per, qp_rem;
	int		qp_add[3] = {8, 4, 2};

	unsigned int  cbp = cbp4x4 & 0x1ffff;		// Need 17 bits to represent Luma information 

	Profile_Start(ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_c_profile);

	qp_per = QP/6;
	qp_rem = QP%6;


	// First, we do intra-prediction according to edge_type and intra_luma_mode!!!!!!!!!!!!!!!!!!!!
	// 1-1, we need compute avail array for three array [ left, top and top_left]
	// TOP first
	avail_top[0] = 
	avail_top[1] = 
	avail_top[4] = 
	avail_top[5] =  !(edge_type&IPPVC_TOP_EDGE);

	// Then LEFT
	avail_left[0] = 
	avail_left[2] = 
	avail_left[8] = 
	avail_left[10] = !(edge_type&IPPVC_LEFT_EDGE);

	// Last TOP_LEFT
	avail_top_left[0] =  !(edge_type&IPPVC_TOP_LEFT_EDGE);
	avail_top_left[1] = 
	avail_top_left[4] = 
	avail_top_left[5] = !(edge_type&IPPVC_TOP_EDGE);
	avail_top_left[2] = 
	avail_top_left[8] = 
	avail_top_left[10] = !(edge_type&IPPVC_LEFT_EDGE);

	// TOP_RIGHT
	avail_top_right[0] = 
	avail_top_right[1] = 
	avail_top_right[4] = !(edge_type&IPPVC_TOP_EDGE);
	avail_top_right[5] = !(edge_type&IPPVC_TOP_RIGHT_EDGE);

	for(k=0; k<16; k++)			// In scan order (4x4 blocks)
	{
		block_x = ac_block_idx_x[k];
		block_y = ac_block_idx_y[k];

		pSrcforPrediction = pSrcDstYPlane + block_y*srcdstYStep + block_x;			// Pointer to top_left cornor of one 4x4 block
		// Prepare array for prediction
		if (avail_top_left[k])
		{
			P_X = pSrcforPrediction[-srcdstYStep - 1];

		}
		else
		{
			P_X = 128;

		}		
		
		if (avail_top[k])
		{
			P_A = pSrcforPrediction[-srcdstYStep    ];
			P_B = pSrcforPrediction[-srcdstYStep + 1];
			P_C = pSrcforPrediction[-srcdstYStep + 2];
			P_D = pSrcforPrediction[-srcdstYStep + 3];
	
		}
		else
		{
			P_A = P_B = P_C = P_D = 128;

		}

		if (avail_top_right[k])
		{
			P_E = pSrcforPrediction[-srcdstYStep + 4];
			P_F = pSrcforPrediction[-srcdstYStep + 5];
			P_G = pSrcforPrediction[-srcdstYStep + 6];
			P_H = pSrcforPrediction[-srcdstYStep + 7];

		}
		else
		{
			P_E = P_F = P_G = P_H = P_D;

		}

		if (avail_left[k])
		{
			P_I = pSrcforPrediction[              -1];
			P_J = pSrcforPrediction[  srcdstYStep -1];
			P_K = pSrcforPrediction[srcdstYStep + srcdstYStep -1];
			P_L = pSrcforPrediction[(srcdstYStep<<1) + srcdstYStep -1];

		}
		else
		{
			P_I = P_J = P_K = P_L = 128;

		}


		pPredictionArray = &prediction_array[0];
		// Form prediction result into corresponding position
		switch(pMBIntraTypes[k])
		{

			case IPP_4x4_VERT:                       /* vertical prediction from block above */
				for(j=0;j<16;j+=4)
				for(i=0;i<4;i++)
					pPredictionArray[j+i] = PredPel[1+i];	

				break;

			case IPP_4x4_HOR:                        /* horizontal prediction from left block */

				for(j=0;j<4;j++)
				for(i=0;i<4;i++)
					pPredictionArray[j*4+i] = PredPel[9 + j];
		
				break;


			case IPP_4x4_DC:                         /* DC prediction */

				s0 = 0;
				if (avail_top[k] && avail_left[k])
				{   
					// no edge
					s0 = (P_A + P_B + P_C + P_D + P_I + P_J + P_K + P_L + 4)>>3;
				}
				else if (!avail_top[k] && avail_left[k])
				{
					// upper edge
					s0 = (P_I + P_J + P_K + P_L + 2)>>2;             
				}
				else if (avail_top[k] && !avail_left[k])
				{
					// left edge
					s0 = (P_A + P_B + P_C + P_D + 2)>>2;             
				}
				else //if (!block_available_up && !block_available_left)
				{
					// top left corner, nothing to predict from
					s0 = 128;                           
				}

				for (j=0; j < 16; j+=4)
				{
					for (i=0; i < 4; i++)
					{
						// store DC prediction
						pPredictionArray[j+i] = s0;
					}
				}

				break;

			case IPP_4x4_DIAG_DL:
				if (!avail_top[k])
				printf ("warning: Intra_4x4_Diagonal_Down_Left prediction mode not allowed at mb \n");

				pPredictionArray[0*4 + 0] = (P_A + P_C + P_B + P_B + 2) >> 2;
				pPredictionArray[1*4 + 0] = 
				pPredictionArray[0*4 + 1] = (P_B + P_D + P_C + P_C + 2) >> 2;
				pPredictionArray[2*4 + 0] =
				pPredictionArray[1*4 + 1] =
				pPredictionArray[0*4 + 2] = (P_C + P_E + P_D + P_D + 2) >> 2;
				pPredictionArray[3*4 + 0] = 
				pPredictionArray[2*4 + 1] = 
				pPredictionArray[1*4 + 2] = 
				pPredictionArray[0*4 + 3] = (P_D + P_F + P_E + P_E + 2) >> 2;
				pPredictionArray[3*4 + 1] = 
				pPredictionArray[2*4 + 2] = 
				pPredictionArray[1*4 + 3] = (P_E + P_G + P_F + P_F + 2) >> 2;
				pPredictionArray[3*4 + 2] = 
				pPredictionArray[2*4 + 3] = (P_F + P_H + P_G + P_G + 2) >> 2;
				pPredictionArray[3*4 + 3] = (P_G + P_H + P_H + P_H + 2) >> 2;
				
			
				break;

			case IPP_4x4_DIAG_DR:
				//if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
				//printf ("warning: Intra_4x4_Diagonal_Down_Right prediction mode not allowed at mb %d\n",img->current_mb_nr);

				pPredictionArray[0 + 3*4] = (P_L + P_K + P_K + P_J + 2) >> 2;
				pPredictionArray[0 + 2*4] =
				pPredictionArray[1 + 3*4] = (P_K + P_J + P_J + P_I + 2) >> 2; 
				pPredictionArray[0 + 1*4] =
				pPredictionArray[1 + 2*4] = 
				pPredictionArray[2 + 3*4] = (P_J + P_I + P_I + P_X + 2) >> 2; 
				pPredictionArray[0 + 0*4] =
				pPredictionArray[1 + 1*4] =
				pPredictionArray[2 + 2*4] =
				pPredictionArray[3 + 3*4] = (P_I + P_X + P_X + P_A + 2) >> 2; 
				pPredictionArray[1 + 0*4] =
				pPredictionArray[2 + 1*4] =
				pPredictionArray[3 + 2*4] = (P_X + P_A + P_A + P_B + 2) >> 2;
				pPredictionArray[2 + 0*4] =
				pPredictionArray[3 + 1*4] = (P_A + P_B + P_B + P_C + 2) >> 2;
				pPredictionArray[3 + 0*4] = (P_B + P_C + P_C + P_D + 2) >> 2;
				
			
				break;



			case  IPP_4x4_VR:/* diagonal prediction -22.5 deg to horizontal plane */
				//if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
				//printf ("warning: Intra_4x4_Vertical_Right prediction mode not allowed at mb %d\n",img->current_mb_nr);

				pPredictionArray[0 + 0*4] = 
				pPredictionArray[1 + 2*4] = (P_X + P_A + 1) >> 1;
				pPredictionArray[1 + 0*4] = 
				pPredictionArray[2 + 2*4] = (P_A + P_B + 1) >> 1;
				pPredictionArray[2 + 0*4] = 
				pPredictionArray[3 + 2*4] = (P_B + P_C + 1) >> 1;
				pPredictionArray[3 + 0*4] = (P_C + P_D + 1) >> 1;
				pPredictionArray[0 + 1*4] = 
				pPredictionArray[1 + 3*4] = (P_I + P_X + P_X + P_A + 2) >> 2;
				pPredictionArray[1 + 1*4] = 
				pPredictionArray[2 + 3*4] = (P_X + P_A + P_A + P_B + 2) >> 2;
				pPredictionArray[2 + 1*4] = 
				pPredictionArray[3 + 3*4] = (P_A + P_B + P_B + P_C + 2) >> 2;
				pPredictionArray[3 + 1*4] = (P_B + P_C + P_C + P_D + 2) >> 2;
				pPredictionArray[0 + 2*4] = (P_X + P_I + P_I + P_J + 2) >> 2;
				pPredictionArray[0 + 3*4] = (P_I + P_J + P_J + P_K + 2) >> 2;
				
			
				break;


			case  IPP_4x4_HD:/* diagonal prediction -22.5 deg to horizontal plane */
				//if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
				//printf ("warning: Intra_4x4_Horizontal_Down prediction mode not allowed at mb %d\n",img->current_mb_nr);

				pPredictionArray[0 + 0*4] = 
				pPredictionArray[2 + 1*4] = (P_X + P_I + 1) >> 1;
				pPredictionArray[1 + 0*4] = 
				pPredictionArray[3 + 1*4] = (P_I + P_X + P_X + P_A + 2) >> 2;
				pPredictionArray[2 + 0*4] = (P_X + P_A + P_A + P_B + 2) >> 2;
				pPredictionArray[3 + 0*4] = (P_A + P_B + P_B + P_C + 2) >> 2;
				pPredictionArray[0 + 1*4] = 
				pPredictionArray[2 + 2*4] = (P_I + P_J + 1) >> 1;
				pPredictionArray[1 + 1*4] = 
				pPredictionArray[3 + 2*4] = (P_X + P_I + P_I + P_J + 2) >> 2;
				pPredictionArray[0 + 2*4] = 
				pPredictionArray[2 + 3*4] = (P_J + P_K + 1) >> 1;
				pPredictionArray[1 + 2*4] = 
				pPredictionArray[3 + 3*4] = (P_I + P_J + P_J + P_K + 2) >> 2;
				pPredictionArray[0 + 3*4] = (P_K + P_L + 1) >> 1;
				pPredictionArray[1 + 3*4] = (P_J + P_K + P_K + P_L + 2) >> 2;
				
			
				break;

			case  IPP_4x4_VL:/* diagonal prediction -22.5 deg to horizontal plane */
				//if (!block_available_up)
				//printf ("warning: Intra_4x4_Vertical_Left prediction mode not allowed at mb %d\n",img->current_mb_nr);
			    
				pPredictionArray[0 + 0*4] = (P_A + P_B + 1) >> 1;
				pPredictionArray[1 + 0*4] = 
				pPredictionArray[0 + 2*4] = (P_B + P_C + 1) >> 1;
				pPredictionArray[2 + 0*4] = 
				pPredictionArray[1 + 2*4] = (P_C + P_D + 1) >> 1;
				pPredictionArray[3 + 0*4] = 
				pPredictionArray[2 + 2*4] = (P_D + P_E + 1) >> 1;
				pPredictionArray[3 + 2*4] = (P_E + P_F + 1) >> 1;
				pPredictionArray[0 + 1*4] = (P_A + P_B + P_B + P_C + 2) >> 2;
				pPredictionArray[1 + 1*4] = 
				pPredictionArray[0 + 3*4] = (P_B + P_C + P_C + P_D + 2) >> 2;
				pPredictionArray[2 + 1*4] = 
				pPredictionArray[1 + 3*4] = (P_C + P_D + P_D + P_E + 2) >> 2;
				pPredictionArray[3 + 1*4] = 
				pPredictionArray[2 + 3*4] = (P_D + P_E + P_E + P_F + 2) >> 2;
				pPredictionArray[3 + 3*4] = (P_E + P_F + P_F + P_G + 2) >> 2;
				
			
				break;

			case  IPP_4x4_HU:/* diagonal prediction -22.5 deg to horizontal plane */
				//if (!block_available_left)
				//printf ("warning: Intra_4x4_Horizontal_Up prediction mode not allowed at mb %d\n",img->current_mb_nr);
			    
				pPredictionArray[0 + 0*4] = (P_I + P_J + 1) >> 1;
				pPredictionArray[1 + 0*4] = (P_I + P_J + P_J + P_K + 2) >> 2;
				pPredictionArray[2 + 0*4] = 
				pPredictionArray[0 + 1*4] = (P_J + P_K + 1) >> 1;
				pPredictionArray[3 + 0*4] = 
				pPredictionArray[1 + 1*4] = (P_J + P_K + P_K + P_L + 2) >> 2;
				pPredictionArray[2 + 1*4] = 
				pPredictionArray[0 + 2*4] = (P_K + P_L + 1) >> 1;
				pPredictionArray[3 + 1*4] = 
				pPredictionArray[1 + 2*4] = (P_K + P_L + P_L + P_L + 2) >> 2;
				pPredictionArray[3 + 2*4] = 
				pPredictionArray[1 + 3*4] = 
				pPredictionArray[0 + 3*4] = 
				pPredictionArray[2 + 2*4] = 
				pPredictionArray[2 + 3*4] = 
				pPredictionArray[3 + 3*4] = P_L;

				break;



			default:
				printf("Error: illegal intra_4x4 prediction mode\n");
				return ippStsErr;
				break;
		}



		// Make decision for AC components
		if( cbp & cbpmask[k])		// cbpmask
		{
			// Form acArray and do IQ
			//memset(acArray, 0, 64);
			for(i=0; i<16; i++)
			{
				//if((*ppSrcCoeff)[i]) 
					//acArray[i] = ((*ppSrcDstCoeff)[i] * dequant_coef[qp_rem][i]) << (qp_per);
				if(QP >= 24)
					acArray[i] = ((*ppSrcDstCoeff)[i] * pQuantTable[i]) << (qp_per -4);
				else
					acArray[i] = ((*ppSrcDstCoeff)[i] * pQuantTable[i] + qp_add[qp_per]) >> (4 - qp_per);
			}

			// Do Inverse transform
			// horizontal
			for (j=0;j<4;j++)
			{
				m5[0] = acArray[0+j*4];
				m5[1] = acArray[1+j*4];
				m5[2] = acArray[2+j*4];
				m5[3] = acArray[3+j*4];


				m6[0] = (m5[0]     + m5[2]);
				m6[1] = (m5[0]     - m5[2]);
				m6[2] = (m5[1]>>1) - m5[3];
				m6[3] = m5[1]      + (m5[3]>>1);


				m7[j][0]  = m6[0] + m6[3];
				m7[j][3]  = m6[0] - m6[3];
				m7[j][1]  = m6[1] + m6[2];
				m7[j][2]  = m6[1] - m6[2];

			}

			// vertical
			for (i=0;i<4;i++)
			{
				m5[0]=m7[0][i];
				m5[1]=m7[1][i];
				m5[2]=m7[2][i];
				m5[3]=m7[3][i];

				m6[0]=(m5[0]+m5[2]);
				m6[1]=(m5[0]-m5[2]);
				m6[2]=(m5[1]>>1)-m5[3];
				m6[3]=m5[1]+(m5[3]>>1);
			
				m7[0][i]  = m6[0]+m6[3];
				m7[3][i]  = m6[0]-m6[3];
				m7[1][i]  = m6[1]+m6[2];
				m7[2][i]  = m6[1]-m6[2];

			}

			// MUST SAVE this result to array for next prediction
			for(j=0; j<4; j++)
				for(i=0; i<4; i++)
				{

					pSrcforPrediction[(j)*srcdstYStep + i] = IClip(0, 255, (((pPredictionArray[j*4+i] << DQ_BITS) + m7[j][i] + DQ_ROUND ) >> DQ_BITS));

				}

			*ppSrcDstCoeff += 16;
		}
		else
		{
			// MUST SAVE this result to array for next prediction
			for(t1=0,t2=0,  j=0; j<4; j++)
			{
				//for(i=0; i<4; i++)
				//	pSrcforPrediction[t1 + i] = pPredictionArray[t2 + i];
				memcpy(&pSrcforPrediction[t1], &pPredictionArray[t2], 4);
				t1 += srcdstYStep;
				t2 += 4;

			}


		}		
	
	}

	Profile_End(ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_c_profile);

	return ippStsNoErr;
}



IppStatus __STDCALL ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_c(Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstYPlane,
            Ipp32s srcdstYStep,
            IppIntra4x4PredMode_H264 *pMBIntraTypes,
            Ipp32u cbp4x2,
            Ipp32u QP,
            Ipp8u edge_type,
            Ipp16s *pQuantTable,
            Ipp8u bypass_flag)

{
	/*  Prediction mode
	  IPP_4x4_VERT     = 0,
	  IPP_4x4_HOR      = 1,
	  IPP_4x4_DC       = 2,
	  IPP_4x4_DIAG_DL  = 3,
	  IPP_4x4_DIAG_DR  = 4,
	  IPP_4x4_VR       = 5,
	  IPP_4x4_HD       = 6,
	  IPP_4x4_VL       = 7,
	  IPP_4x4_HU       = 8
	*/
	_DECLSPEC unsigned char prediction_array[16];

	_DECLSPEC int		avail_left[16]	     = { 1, 1, 1, 1, 1, 1, 1, 1,    1, 1, 1, 1, 1, 1, 1, 1};
	_DECLSPEC int		avail_top [16]	     = { 1, 1, 1, 1, 1, 1, 1, 1,    1, 1, 1, 1, 1, 1, 1, 1};
	_DECLSPEC int		avail_top_left[16]   = { 1, 1, 1, 1, 1, 1, 1, 1,    1, 1, 1, 1, 1, 1, 1, 1};
	_DECLSPEC int		avail_top_right[16]  = { 1, 1, 1, 0, 1, 1, 1, 0,   1, 1, 1, 0, 1, 0, 1, 0};

	_DECLSPEC int		acArray[16];
	_DECLSPEC int		PredPel[16];  // array of predictor pels
	_DECLSPEC int		m7[4][4], m5[4], m6[4];

	int		block_x, block_y;

	unsigned char * pPredictionArray;

	int		i,	j, k, t1, t2;
	Ipp8u	*pSrcforPrediction;

	int		pitch = srcdstYStep;

	int		s0;				// Used for DC prediction

	int		qp_per, qp_rem;
	int		qp_add[3] = {8, 4, 2};

	unsigned int  cbp = cbp4x2 & 0xff;		// Need 8 bits to represent Luma information 


	Profile_Start(ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_c_profile);

	qp_per = QP/6;
	qp_rem = QP%6;


	// First, we do intra-prediction according to edge_type and intra_luma_mode!!!!!!!!!!!!!!!!!!!!
	// 1-1, we need compute avail array for three array [ left, top and top_left]
	// TOP first
	avail_top[0] = 
	avail_top[1] = 
	avail_top[4] = 
	avail_top[5] =  !(edge_type&IPPVC_TOP_EDGE);

	// Then LEFT
	avail_left[0] = 
	avail_left[2] = 
	avail_left[8] = 
	avail_left[10] = !(edge_type&IPPVC_LEFT_EDGE);

	// Last TOP_LEFT
	avail_top_left[0] =  !(edge_type&IPPVC_TOP_LEFT_EDGE);
	avail_top_left[1] = 
	avail_top_left[4] = 
	avail_top_left[5] = !(edge_type&IPPVC_TOP_EDGE);
	avail_top_left[2] = 
	avail_top_left[8] = 
	avail_top_left[10] = !(edge_type&IPPVC_LEFT_EDGE);

	// TOP_RIGHT
	avail_top_right[0] = 
	avail_top_right[1] = 
	avail_top_right[4] = !(edge_type&IPPVC_TOP_EDGE);
	avail_top_right[5] = !(edge_type&IPPVC_TOP_RIGHT_EDGE);

	for(k=0; k<8; k++)			// In scan order (4x4 blocks)
	{
		block_x = ac_block_idx_x[k];
		block_y = ac_block_idx_y[k];

		pSrcforPrediction = pSrcDstYPlane + block_y*srcdstYStep + block_x;			// Pointer to top_left cornor of one 4x4 block
		// Prepare array for prediction
		if (avail_top_left[k])
		{
			P_X = pSrcforPrediction[-srcdstYStep - 1];

		}
		else
		{
			P_X = 128;

		}		
		
		if (avail_top[k])
		{
			P_A = pSrcforPrediction[-srcdstYStep    ];
			P_B = pSrcforPrediction[-srcdstYStep + 1];
			P_C = pSrcforPrediction[-srcdstYStep + 2];
			P_D = pSrcforPrediction[-srcdstYStep + 3];
	
		}
		else
		{
			P_A = P_B = P_C = P_D = 128;

		}

		if (avail_top_right[k])
		{
			P_E = pSrcforPrediction[-srcdstYStep + 4];
			P_F = pSrcforPrediction[-srcdstYStep + 5];
			P_G = pSrcforPrediction[-srcdstYStep + 6];
			P_H = pSrcforPrediction[-srcdstYStep + 7];

		}
		else
		{
			P_E = P_F = P_G = P_H = P_D;

		}

		if (avail_left[k])
		{
			P_I = pSrcforPrediction[              -1];
			P_J = pSrcforPrediction[  srcdstYStep -1];
			P_K = pSrcforPrediction[srcdstYStep + srcdstYStep -1];
			P_L = pSrcforPrediction[(srcdstYStep<<1) + srcdstYStep -1];

		}
		else
		{
			P_I = P_J = P_K = P_L = 128;

		}


		pPredictionArray = &prediction_array[0];
		// Form prediction result into corresponding position
		switch(pMBIntraTypes[k])
		{

			case IPP_4x4_VERT:                       /* vertical prediction from block above */
				for(j=0;j<16;j+=4)
				for(i=0;i<4;i++)
					pPredictionArray[j+i] = PredPel[1+i];	

				break;

			case IPP_4x4_HOR:                        /* horizontal prediction from left block */

				for(j=0;j<4;j++)
				for(i=0;i<4;i++)
					pPredictionArray[j*4+i] = PredPel[9 + j];
		
				break;


			case IPP_4x4_DC:                         /* DC prediction */

				s0 = 0;
				if (avail_top[k] && avail_left[k])
				{   
					// no edge
					s0 = (P_A + P_B + P_C + P_D + P_I + P_J + P_K + P_L + 4)>>3;
				}
				else if (!avail_top[k] && avail_left[k])
				{
					// upper edge
					s0 = (P_I + P_J + P_K + P_L + 2)>>2;             
				}
				else if (avail_top[k] && !avail_left[k])
				{
					// left edge
					s0 = (P_A + P_B + P_C + P_D + 2)>>2;             
				}
				else //if (!block_available_up && !block_available_left)
				{
					// top left corner, nothing to predict from
					s0 = 128;                           
				}

				for (j=0; j < 16; j+=4)
				{
					for (i=0; i < 4; i++)
					{
						// store DC prediction
						pPredictionArray[j+i] = s0;
					}
				}

				break;

			case IPP_4x4_DIAG_DL:
				if (!avail_top[k])
				printf ("warning: Intra_4x4_Diagonal_Down_Left prediction mode not allowed at mb \n");

				pPredictionArray[0*4 + 0] = (P_A + P_C + P_B + P_B + 2) >> 2;
				pPredictionArray[1*4 + 0] = 
				pPredictionArray[0*4 + 1] = (P_B + P_D + P_C + P_C + 2) >> 2;
				pPredictionArray[2*4 + 0] =
				pPredictionArray[1*4 + 1] =
				pPredictionArray[0*4 + 2] = (P_C + P_E + P_D + P_D + 2) >> 2;
				pPredictionArray[3*4 + 0] = 
				pPredictionArray[2*4 + 1] = 
				pPredictionArray[1*4 + 2] = 
				pPredictionArray[0*4 + 3] = (P_D + P_F + P_E + P_E + 2) >> 2;
				pPredictionArray[3*4 + 1] = 
				pPredictionArray[2*4 + 2] = 
				pPredictionArray[1*4 + 3] = (P_E + P_G + P_F + P_F + 2) >> 2;
				pPredictionArray[3*4 + 2] = 
				pPredictionArray[2*4 + 3] = (P_F + P_H + P_G + P_G + 2) >> 2;
				pPredictionArray[3*4 + 3] = (P_G + P_H + P_H + P_H + 2) >> 2;
				
			
				break;

			case IPP_4x4_DIAG_DR:
				//if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
				//printf ("warning: Intra_4x4_Diagonal_Down_Right prediction mode not allowed at mb %d\n",img->current_mb_nr);

				pPredictionArray[0 + 3*4] = (P_L + P_K + P_K + P_J + 2) >> 2;
				pPredictionArray[0 + 2*4] =
				pPredictionArray[1 + 3*4] = (P_K + P_J + P_J + P_I + 2) >> 2; 
				pPredictionArray[0 + 1*4] =
				pPredictionArray[1 + 2*4] = 
				pPredictionArray[2 + 3*4] = (P_J + P_I + P_I + P_X + 2) >> 2; 
				pPredictionArray[0 + 0*4] =
				pPredictionArray[1 + 1*4] =
				pPredictionArray[2 + 2*4] =
				pPredictionArray[3 + 3*4] = (P_I + P_X + P_X + P_A + 2) >> 2; 
				pPredictionArray[1 + 0*4] =
				pPredictionArray[2 + 1*4] =
				pPredictionArray[3 + 2*4] = (P_X + P_A + P_A + P_B + 2) >> 2;
				pPredictionArray[2 + 0*4] =
				pPredictionArray[3 + 1*4] = (P_A + P_B + P_B + P_C + 2) >> 2;
				pPredictionArray[3 + 0*4] = (P_B + P_C + P_C + P_D + 2) >> 2;
				
			
				break;



			case  IPP_4x4_VR:/* diagonal prediction -22.5 deg to horizontal plane */
				//if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
				//printf ("warning: Intra_4x4_Vertical_Right prediction mode not allowed at mb %d\n",img->current_mb_nr);

				pPredictionArray[0 + 0*4] = 
				pPredictionArray[1 + 2*4] = (P_X + P_A + 1) >> 1;
				pPredictionArray[1 + 0*4] = 
				pPredictionArray[2 + 2*4] = (P_A + P_B + 1) >> 1;
				pPredictionArray[2 + 0*4] = 
				pPredictionArray[3 + 2*4] = (P_B + P_C + 1) >> 1;
				pPredictionArray[3 + 0*4] = (P_C + P_D + 1) >> 1;
				pPredictionArray[0 + 1*4] = 
				pPredictionArray[1 + 3*4] = (P_I + P_X + P_X + P_A + 2) >> 2;
				pPredictionArray[1 + 1*4] = 
				pPredictionArray[2 + 3*4] = (P_X + P_A + P_A + P_B + 2) >> 2;
				pPredictionArray[2 + 1*4] = 
				pPredictionArray[3 + 3*4] = (P_A + P_B + P_B + P_C + 2) >> 2;
				pPredictionArray[3 + 1*4] = (P_B + P_C + P_C + P_D + 2) >> 2;
				pPredictionArray[0 + 2*4] = (P_X + P_I + P_I + P_J + 2) >> 2;
				pPredictionArray[0 + 3*4] = (P_I + P_J + P_J + P_K + 2) >> 2;
				
			
				break;


			case  IPP_4x4_HD:/* diagonal prediction -22.5 deg to horizontal plane */
				//if ((!block_available_up)||(!block_available_left)||(!block_available_up_left))
				//printf ("warning: Intra_4x4_Horizontal_Down prediction mode not allowed at mb %d\n",img->current_mb_nr);

				pPredictionArray[0 + 0*4] = 
				pPredictionArray[2 + 1*4] = (P_X + P_I + 1) >> 1;
				pPredictionArray[1 + 0*4] = 
				pPredictionArray[3 + 1*4] = (P_I + P_X + P_X + P_A + 2) >> 2;
				pPredictionArray[2 + 0*4] = (P_X + P_A + P_A + P_B + 2) >> 2;
				pPredictionArray[3 + 0*4] = (P_A + P_B + P_B + P_C + 2) >> 2;
				pPredictionArray[0 + 1*4] = 
				pPredictionArray[2 + 2*4] = (P_I + P_J + 1) >> 1;
				pPredictionArray[1 + 1*4] = 
				pPredictionArray[3 + 2*4] = (P_X + P_I + P_I + P_J + 2) >> 2;
				pPredictionArray[0 + 2*4] = 
				pPredictionArray[2 + 3*4] = (P_J + P_K + 1) >> 1;
				pPredictionArray[1 + 2*4] = 
				pPredictionArray[3 + 3*4] = (P_I + P_J + P_J + P_K + 2) >> 2;
				pPredictionArray[0 + 3*4] = (P_K + P_L + 1) >> 1;
				pPredictionArray[1 + 3*4] = (P_J + P_K + P_K + P_L + 2) >> 2;
				
			
				break;

			case  IPP_4x4_VL:/* diagonal prediction -22.5 deg to horizontal plane */
				//if (!block_available_up)
				//printf ("warning: Intra_4x4_Vertical_Left prediction mode not allowed at mb %d\n",img->current_mb_nr);
			    
				pPredictionArray[0 + 0*4] = (P_A + P_B + 1) >> 1;
				pPredictionArray[1 + 0*4] = 
				pPredictionArray[0 + 2*4] = (P_B + P_C + 1) >> 1;
				pPredictionArray[2 + 0*4] = 
				pPredictionArray[1 + 2*4] = (P_C + P_D + 1) >> 1;
				pPredictionArray[3 + 0*4] = 
				pPredictionArray[2 + 2*4] = (P_D + P_E + 1) >> 1;
				pPredictionArray[3 + 2*4] = (P_E + P_F + 1) >> 1;
				pPredictionArray[0 + 1*4] = (P_A + P_B + P_B + P_C + 2) >> 2;
				pPredictionArray[1 + 1*4] = 
				pPredictionArray[0 + 3*4] = (P_B + P_C + P_C + P_D + 2) >> 2;
				pPredictionArray[2 + 1*4] = 
				pPredictionArray[1 + 3*4] = (P_C + P_D + P_D + P_E + 2) >> 2;
				pPredictionArray[3 + 1*4] = 
				pPredictionArray[2 + 3*4] = (P_D + P_E + P_E + P_F + 2) >> 2;
				pPredictionArray[3 + 3*4] = (P_E + P_F + P_F + P_G + 2) >> 2;
				
			
				break;

			case  IPP_4x4_HU:/* diagonal prediction -22.5 deg to horizontal plane */
				//if (!block_available_left)
				//printf ("warning: Intra_4x4_Horizontal_Up prediction mode not allowed at mb %d\n",img->current_mb_nr);
			    
				pPredictionArray[0 + 0*4] = (P_I + P_J + 1) >> 1;
				pPredictionArray[1 + 0*4] = (P_I + P_J + P_J + P_K + 2) >> 2;
				pPredictionArray[2 + 0*4] = 
				pPredictionArray[0 + 1*4] = (P_J + P_K + 1) >> 1;
				pPredictionArray[3 + 0*4] = 
				pPredictionArray[1 + 1*4] = (P_J + P_K + P_K + P_L + 2) >> 2;
				pPredictionArray[2 + 1*4] = 
				pPredictionArray[0 + 2*4] = (P_K + P_L + 1) >> 1;
				pPredictionArray[3 + 1*4] = 
				pPredictionArray[1 + 2*4] = (P_K + P_L + P_L + P_L + 2) >> 2;
				pPredictionArray[3 + 2*4] = 
				pPredictionArray[1 + 3*4] = 
				pPredictionArray[0 + 3*4] = 
				pPredictionArray[2 + 2*4] = 
				pPredictionArray[2 + 3*4] = 
				pPredictionArray[3 + 3*4] = P_L;

				break;



			default:
				printf("Error: illegal intra_4x4 prediction mode\n");
				return ippStsErr;
				break;
		}



		// Make decision for AC components
		if( cbp & cbpmask_4x4[k])		// cbpmask
		{
			// Form acArray and do IQ
			//memset(acArray, 0, 64);
			for(i=0; i<16; i++)
			{
				//if((*ppSrcCoeff)[i]) 
					//acArray[i] = ((*ppSrcDstCoeff)[i] * dequant_coef[qp_rem][i]) << (qp_per);
				if(QP >= 24)
					acArray[i] = ((*ppSrcDstCoeff)[i] * pQuantTable[i]) << (qp_per -4);
				else
					acArray[i] = ((*ppSrcDstCoeff)[i] * pQuantTable[i] + qp_add[qp_per]) >> (4 - qp_per);
			}

			// Do Inverse transform
			// horizontal
			for (j=0;j<4;j++)
			{
				m5[0] = acArray[0+j*4];
				m5[1] = acArray[1+j*4];
				m5[2] = acArray[2+j*4];
				m5[3] = acArray[3+j*4];


				m6[0] = (m5[0]     + m5[2]);
				m6[1] = (m5[0]     - m5[2]);
				m6[2] = (m5[1]>>1) - m5[3];
				m6[3] = m5[1]      + (m5[3]>>1);


				m7[j][0]  = m6[0] + m6[3];
				m7[j][3]  = m6[0] - m6[3];
				m7[j][1]  = m6[1] + m6[2];
				m7[j][2]  = m6[1] - m6[2];

			}

			// vertical
			for (i=0;i<4;i++)
			{
				m5[0]=m7[0][i];
				m5[1]=m7[1][i];
				m5[2]=m7[2][i];
				m5[3]=m7[3][i];

				m6[0]=(m5[0]+m5[2]);
				m6[1]=(m5[0]-m5[2]);
				m6[2]=(m5[1]>>1)-m5[3];
				m6[3]=m5[1]+(m5[3]>>1);
			
				m7[0][i]  = m6[0]+m6[3];
				m7[3][i]  = m6[0]-m6[3];
				m7[1][i]  = m6[1]+m6[2];
				m7[2][i]  = m6[1]-m6[2];

			}

			// MUST SAVE this result to array for next prediction
			for(j=0; j<4; j++)
				for(i=0; i<4; i++)
				{

					pSrcforPrediction[(j)*srcdstYStep + i] = IClip(0, 255, (((pPredictionArray[j*4+i] << DQ_BITS) + m7[j][i] + DQ_ROUND ) >> DQ_BITS));

				}

			*ppSrcDstCoeff += 16;
		}
		else
		{
			// MUST SAVE this result to array for next prediction
			for(t1=0,t2=0,  j=0; j<4; j++)
			{
				//for(i=0; i<4; i++)
				//	pSrcforPrediction[t1 + i] = pPredictionArray[t2 + i];
				memcpy(&pSrcforPrediction[t1], &pPredictionArray[t2], 4);
				t1 += srcdstYStep;
				t2 += 4;

			}
		}	
	}

	Profile_End(ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_c_profile);

	return ippStsNoErr;
}



IppStatus __STDCALL ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_c(Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstYPlane,
            Ipp32u srcdstYStep,
            Ipp32u cbp4x4,
            Ipp32s QP,
            Ipp16s *pQuantTable,
            Ipp8u bypass_flag)

{
	_DECLSPEC int		acArray[16];
	_DECLSPEC int		m7[4][4], m5[4], m6[4];

	int		block_x, block_y;

	int		i,	j, k;
	Ipp8u	*pSrcforPrediction;

	int		pitch = srcdstYStep;

	int		qp_per, qp_rem;
	int		qp_add[3] = {8, 4, 2};

	unsigned int  cbp = cbp4x4 & 0x1ffff;		// Need 17 bits to represent Luma information

	Profile_Start(ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_c_profile);

 
	qp_per = QP/6;
	qp_rem = QP%6;


	for(k=0; k<16; k++)			// In scan order (4x4 blocks)
	{
		block_x = ac_block_idx_x[k];
		block_y = ac_block_idx_y[k];

		pSrcforPrediction = pSrcDstYPlane + block_y*srcdstYStep + block_x;			// Pointer to top_left cornor of one 4x4 block

		// Make decision for AC components
		if( cbp & cbpmask[k])		// cbpmask  --  --- Here INTEL make too complicate for it
		{
			// Form acArray and do IQ
			//memset(acArray, 0, 64);
			for(i=0; i<16; i++)
			{
				//if((*ppSrcCoeff)[i]) 
					//acArray[i] = ((*ppSrcDstCoeff)[i] * dequant_coef[qp_rem][i]) << (qp_per);
				if(QP >= 24)
					acArray[i] = ((*ppSrcDstCoeff)[i] * pQuantTable[i]) << (qp_per -4);
				else
					acArray[i] = ((*ppSrcDstCoeff)[i] * pQuantTable[i] + qp_add[qp_per]) >> (4 - qp_per);
			}

			// Do Inverse transform
			// horizontal
			for (j=0;j<4;j++)
			{
				m5[0] = acArray[0+j*4];
				m5[1] = acArray[1+j*4];
				m5[2] = acArray[2+j*4];
				m5[3] = acArray[3+j*4];


				m6[0] = (m5[0]     + m5[2]);
				m6[1] = (m5[0]     - m5[2]);
				m6[2] = (m5[1]>>1) - m5[3];
				m6[3] = m5[1]      + (m5[3]>>1);


				m7[j][0]  = m6[0] + m6[3];
				m7[j][3]  = m6[0] - m6[3];
				m7[j][1]  = m6[1] + m6[2];
				m7[j][2]  = m6[1] - m6[2];

			}

			// vertical
			for (i=0;i<4;i++)
			{
				m5[0]=m7[0][i];
				m5[1]=m7[1][i];
				m5[2]=m7[2][i];
				m5[3]=m7[3][i];

				m6[0]=(m5[0]+m5[2]);
				m6[1]=(m5[0]-m5[2]);
				m6[2]=(m5[1]>>1)-m5[3];
				m6[3]=m5[1]+(m5[3]>>1);
			
				m7[0][i]  = m6[0]+m6[3];
				m7[3][i]  = m6[0]-m6[3];
				m7[1][i]  = m6[1]+m6[2];
				m7[2][i]  = m6[1]-m6[2];

			}

			// MUST SAVE this result to array for next prediction
			for(j=0; j<4; j++)
				for(i=0; i<4; i++)
				{

					pSrcforPrediction[(j)*srcdstYStep + i] = IClip(0, 255, (((pSrcforPrediction[(j)*srcdstYStep + i] << DQ_BITS) + m7[j][i] + DQ_ROUND ) >> DQ_BITS));

				}

			*ppSrcDstCoeff += 16;
		}
		else
		{
			continue;
		}		
	
	}

	Profile_End(ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_c_profile);

	return ippStsNoErr;
}



/***************************************************************************************
 * Need change QP to pQuantTable in the following three functions
 ***************************************************************************************
 */
IppStatus __STDCALL ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_c(Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstUPlane,
            Ipp8u *pSrcDstVPlane,
            Ipp32u srcdstUVStep,
            IppIntraChromaPredMode_H264 intra_chroma_mode,
            Ipp32u cbp4x4,
            Ipp32u ChromaQPU,
            Ipp32u ChromaQPV,
            Ipp8u edge_type,
            Ipp16s *pQuantTableU,
            Ipp16s *pQuantTableV,
            Ipp8u bypass_flag)

{
	_DECLSPEC unsigned char prediction_arrayU[64], prediction_arrayV[64];
	_DECLSPEC int		m7[4][4], m5[4], m6[4];
	_DECLSPEC int		acArray[16];
	_DECLSPEC int		dcArrayU[4], dcResultU[4];
	_DECLSPEC int		dcArrayV[4], dcResultV[4];

	const int dequant_dc[6] = {10, 11, 13, 14, 16, 18};

	int		block_idx[] = {0,0, 4,0, 0,4, 4,4};

	int		up_avail, left_avail, up_left_avail;
	int		i,	j, k;
	Ipp8u	*pSrcforPredictionU, *pSrcforPredictionV, *pSrcforPrediction;

	int		pitch = srcdstUVStep;

	int		iv, ih, ib, ic, iaa;	// Used for Plane prediction


	int		jsu0=0,	jsu1=0, jsu2=0, jsu3=0;
	int		jsv0=0,	jsv1=0, jsv2=0, jsv3=0;
	int		jsu[2][2], jsv[2][2];

	int		ii, jj, ioff, joff;

	int		qp_per_u, qp_per_v, qp_rem_u, qp_rem_v;


	unsigned int	cbp = cbp4x4>> 17;			// Use Chroma part ONLY


	int		block_x, block_y;

	Profile_Start(ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_c_profile);

	qp_per_u = ChromaQPU/6;
	qp_rem_u = ChromaQPU%6;
	qp_per_v = ChromaQPV/6;
	qp_rem_v = ChromaQPV%6;

	// First, we do intra-prediction according to edge_type and intra_luma_mode!!!!!!!!!!!!!!!!!!!!
	up_avail		= !(edge_type&IPPVC_TOP_EDGE);
	left_avail		= !(edge_type&IPPVC_LEFT_EDGE);
	up_left_avail	= !(edge_type&IPPVC_TOP_LEFT_EDGE);

	if (intra_chroma_mode == 0)			// DC mode
	{
		pSrcforPredictionU	= pSrcDstUPlane - srcdstUVStep;				// pointer to TOP U
		pSrcforPredictionV	= pSrcDstVPlane - srcdstUVStep;				// pointer to TOP V

		for(i=0;i<4;i++)
		{
			if(up_avail)
			{
				jsu0 = jsu0 + pSrcforPredictionU[i];
				jsu1 = jsu1 + pSrcforPredictionU[i+4];

				jsv0 = jsv0 + pSrcforPredictionV[i];
				jsv1 = jsv1 + pSrcforPredictionV[i+4];

			}
			if(left_avail)
			{
				jsu2 = jsu2 + pSrcDstUPlane[-1+i*srcdstUVStep];
				jsu3 = jsu3 + pSrcDstUPlane[-1+(i+4)*srcdstUVStep];

				jsv2 = jsv2 + pSrcDstVPlane[-1+i*srcdstUVStep];
				jsv3 = jsv3 + pSrcDstVPlane[-1+(i+4)*srcdstUVStep];

			}
		}

		if(up_avail && left_avail)
		{
		  jsu[0][0]=(jsu0+jsu2+4)>>3;
		  jsu[1][0]=(jsu1+2)>>2;
		  jsu[0][1]=(jsu3+2)>>2;
		  jsu[1][1]=(jsu1+jsu3+4)>>3;

		  jsv[0][0]=(jsv0+jsv2+4)>>3;
		  jsv[1][0]=(jsv1+2)>>2;
		  jsv[0][1]=(jsv3+2)>>2;
		  jsv[1][1]=(jsv1+jsv3+4)>>3;
		}

		if(up_avail && !left_avail)
		{
		  jsu[0][0]=(jsu0+2)>>2;
		  jsu[1][0]=(jsu1+2)>>2;
		  jsu[0][1]=(jsu0+2)>>2;
		  jsu[1][1]=(jsu1+2)>>2;

		  jsv[0][0]=(jsv0+2)>>2;
		  jsv[1][0]=(jsv1+2)>>2;
		  jsv[0][1]=(jsv0+2)>>2;
		  jsv[1][1]=(jsv1+2)>>2;
		}

		if(left_avail && !up_avail)
		{
		  jsu[0][0]=(jsu2+2)>>2;
		  jsu[1][0]=(jsu2+2)>>2;
		  jsu[0][1]=(jsu3+2)>>2;
		  jsu[1][1]=(jsu3+2)>>2;

		  jsv[0][0]=(jsv2+2)>>2;
		  jsv[1][0]=(jsv2+2)>>2;
		  jsv[0][1]=(jsv3+2)>>2;
		  jsv[1][1]=(jsv3+2)>>2;
		}

		if(!up_avail && !left_avail)
		{
		  jsu[0][0]=128;
		  jsu[1][0]=128;

		  jsu[0][1]=128;
		  jsu[1][1]=128;

		  jsv[0][0]=128;
		  jsv[1][0]=128;

		  jsv[0][1]=128;
		  jsv[1][1]=128;
		}
	}


	for (j=0;j<2;j++)
	{
		joff=j*4;

		for(i=0;i<2;i++)
		{
			ioff=i*4;

			switch (intra_chroma_mode)
			{
				case 0:									// DC_PRED_8
					for (jj=0; jj<4; jj++)
					  for (ii=0; ii<4; ii++)
					  {
							prediction_arrayU[joff*8+ioff+8*jj+ii] = jsu[i][j];
							prediction_arrayV[joff*8+ioff+8*jj+ii] = jsv[i][j];
					  }
				break;
				case 1:										// HOR_PRED_8
					pSrcforPredictionU	= pSrcDstUPlane - 1;				// pointer to left U
					pSrcforPredictionV	= pSrcDstVPlane - 1;				// pointer to left V

					for (jj=0; jj<4; jj++)
					{
						for (ii=0; ii<4; ii++)
						{
							prediction_arrayU[joff*8+ioff+8*jj+ii] = pSrcforPredictionU[(joff+jj)*srcdstUVStep];
							prediction_arrayV[joff*8+ioff+8*jj+ii] = pSrcforPredictionV[(joff+jj)*srcdstUVStep];

						}
					}

				break;
				case 2:										// VERT_PRED_8
					assert(up_avail);
					pSrcforPredictionU	= pSrcDstUPlane - srcdstUVStep;
					pSrcforPredictionV	= pSrcDstVPlane - srcdstUVStep;

					for (ii=0; ii<4; ii++)
					{
						//pred = imgUV[uv][up.pos_y][up.pos_x+ii+ioff];
						for (jj=0; jj<4; jj++)
						{
							prediction_arrayU[joff*8+ioff+8*jj+ii] = pSrcforPredictionU[ioff+ii];
							prediction_arrayV[joff*8+ioff+8*jj+ii] = pSrcforPredictionV[ioff+ii];
						}
					}
				break;
				case 3:										// PLANE_8

					// First, do prediction for U
					ih = iv = 0;

					pSrcforPredictionU	= pSrcDstUPlane -srcdstUVStep;	// Up
					pSrcforPrediction	= pSrcDstUPlane -1;				// Left

					for (ii=1;ii<5;ii++)
					{
						ih += ii*(pSrcforPredictionU[3+ii] - pSrcforPredictionU[3-ii]);
						iv += ii*(pSrcforPrediction[(3+ii)*srcdstUVStep] - pSrcforPrediction[(3-ii)*srcdstUVStep]);
					}
					ib=(17*ih+16)>>5;
					ic=(17*iv+16)>>5;

					iaa=16*(pSrcDstUPlane[7-srcdstUVStep] + pSrcDstUPlane[-1+7*srcdstUVStep]);
					//iaa=16*(pSrcDstYPlane[-1+15*srcdstYStep] + pSrcDstYPlane[15-srcdstYStep]);


					for (jj=0; jj<4; jj++)
						for (ii=0; ii<4; ii++)
							prediction_arrayU[joff*8+ioff+8*jj+ii] = IClip(0,255, ((iaa+(ii+ioff-3)*ib +(jj+joff-3)*ic + 16)>>5));

					// Second, do prediction for V
					ih = iv = 0;

					pSrcforPredictionV	= pSrcDstVPlane -srcdstUVStep;	// Up
					pSrcforPrediction	= pSrcDstVPlane -1;				// Left

					for (ii=1;ii<5;ii++)
					{
						ih += ii*(pSrcforPredictionV[3+ii] - pSrcforPredictionV[3-ii]);
						iv += ii*(pSrcforPrediction[(3+ii)*srcdstUVStep] - pSrcforPrediction[(3-ii)*srcdstUVStep]);
					}
					ib=(17*ih+16)>>5;
					ic=(17*iv+16)>>5;

					iaa=16*(pSrcDstVPlane[7-srcdstUVStep] + pSrcDstVPlane[-1+7*srcdstUVStep]);
					//iaa=16*(pSrcDstYPlane[-1+15*srcdstYStep] + pSrcDstYPlane[15-srcdstYStep]);


					for (jj=0; jj<4; jj++)
						for (ii=0; ii<4; ii++)
							prediction_arrayV[joff*8+ioff+8*jj+ii] = IClip(0,255, ((iaa+(ii+ioff-3)*ib +(jj+joff-3)*ic + 16)>>5));


				  break;
				default:
					//error("illegal chroma intra prediction mode", 600);
				break;
			}
		}
	}
	// Second, we do inverse transform for each 4x4 block!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//         include inverse Quantization and add operation

	if(!(cbp & 0xffff))
	{
		// Non- cbp has information: use prediction as coeffs ONLY  -- WWD in 2006-05-05

		for(j=0; j<8; j++)
			for(i=0; i<8; i++)
			{
				pSrcDstUPlane[j*srcdstUVStep+i] = prediction_arrayU[j*8+i];
				pSrcDstVPlane[j*srcdstUVStep+i] = prediction_arrayV[j*8+i];

			}

		return ippStsNoErr;
	}
	// U
	if(cbp & 0x01)
	{
		// 2.1 2x2 DC Transform
		// IPPVC_CBP_1ST_CHROMA_DC_BITPOS
		// 这里需要进一步处理QP<6的情况

		if(ChromaQPU >= 6)
		{
			for (i=0;i<4;i++)
				dcArrayU[i] = ((*ppSrcDstCoeff)[i]) *dequant_coef[qp_rem_u][0]<< (qp_per_u - 1);
		}
		else
		{
			for (i=0;i<4;i++)
				dcArrayU[i] = ((*ppSrcDstCoeff)[i]) *dequant_coef[qp_rem_u][0]>> 1;
		}

		dcResultU[0] = (dcArrayU[0] + dcArrayU[1] + dcArrayU[2] + dcArrayU[3]);
		dcResultU[1] = (dcArrayU[0] - dcArrayU[1] + dcArrayU[2] - dcArrayU[3]);
		dcResultU[2] = (dcArrayU[0] + dcArrayU[1] - dcArrayU[2] - dcArrayU[3]);
		dcResultU[3] = (dcArrayU[0] - dcArrayU[1] - dcArrayU[2] + dcArrayU[3]);


		*ppSrcDstCoeff += 4;

	}
	else
	{
		dcResultU[0] = 0;
		dcResultU[1] = 0;
		dcResultU[2] = 0;
		dcResultU[3] = 0;

	}


	// V
	if(cbp & 0x02)
	{
		if(ChromaQPV >= 6)
		{
			for (i=0;i<4;i++)
				dcArrayV[i] = ((*ppSrcDstCoeff)[i]) *dequant_coef[qp_rem_v][0]<< (qp_per_v - 1);
		}
		else
		{
			for (i=0;i<4;i++)
				dcArrayV[i] = ((*ppSrcDstCoeff)[i]) *dequant_coef[qp_rem_v][0] >>  1;
		}

		dcResultV[0] = (dcArrayV[0] + dcArrayV[1] + dcArrayV[2] + dcArrayV[3]);
		dcResultV[1] = (dcArrayV[0] - dcArrayV[1] + dcArrayV[2] - dcArrayV[3]);
		dcResultV[2] = (dcArrayV[0] + dcArrayV[1] - dcArrayV[2] - dcArrayV[3]);
		dcResultV[3] = (dcArrayV[0] - dcArrayV[1] - dcArrayV[2] + dcArrayV[3]);

		*ppSrcDstCoeff += 4;


	}
	else
	{
		dcResultV[0] = 0;
		dcResultV[1] = 0;
		dcResultV[2] = 0;
		dcResultV[3] = 0;


	}
	// U -- AC
	for(k=0; k<4; k++)									// CBP decision
	{
		block_x = block_idx[k<<1];
		block_y = block_idx[(k<<1) + 1];

		if(!(cbp&(1<<(2+k))))
		{
			// This block has no AC information : but we have DC information
			for(jj=0; jj<4; jj++)
				for(ii=0; ii<4; ii++)
				{
					pSrcDstUPlane[(block_y+jj)*srcdstUVStep+block_x+ii] = prediction_arrayU[(block_y+jj)*8+block_x+ii] + ((dcResultU[k]+32)>>6);
				}

		}
		else
		{

			// Form acArray and do IQ
			for(i=1; i<16; i++)
			{
				acArray[i] = ((*ppSrcDstCoeff)[i] * dequant_coef[qp_rem_u][i]) << (qp_per_u);
				//idctcoeff = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem_uv][i/4][i%4]) << (qp_per_uv);
			}

			acArray[0] = dcResultU[k];			// Add DC coeff

			// Do Inverse transform
			// horizontal
			for (j=0;j<4;j++)
			{
				m5[0] = acArray[0+j*4];
				m5[1] = acArray[1+j*4];
				m5[2] = acArray[2+j*4];
				m5[3] = acArray[3+j*4];


				m6[0] = (m5[0]     + m5[2]);
				m6[1] = (m5[0]     - m5[2]);
				m6[2] = (m5[1]>>1) - m5[3];
				m6[3] = m5[1]      + (m5[3]>>1);


				m7[j][0]  = m6[0] + m6[3];
				m7[j][3]  = m6[0] - m6[3];
				m7[j][1]  = m6[1] + m6[2];
				m7[j][2]  = m6[1] - m6[2];

			}

			// vertical
			for (i=0;i<4;i++)
			{
				m5[0]=m7[0][i];
				m5[1]=m7[1][i];
				m5[2]=m7[2][i];
				m5[3]=m7[3][i];

				m6[0]=(m5[0]+m5[2]);
				m6[1]=(m5[0]-m5[2]);
				m6[2]=(m5[1]>>1)-m5[3];
				m6[3]=m5[1]+(m5[3]>>1);
			
				m7[0][i]  = m6[0]+m6[3];
				m7[3][i]  = m6[0]-m6[3];
				m7[1][i]  = m6[1]+m6[2];
				m7[2][i]  = m6[1]-m6[2];

			}

			for(jj=0; jj<4; jj++)
				for(ii=0; ii<4; ii++)
				{
					pSrcDstUPlane[(block_y+jj)*srcdstUVStep+block_x+ii] = IClip(0, 255,(((prediction_arrayU[(block_y+jj)*8 + block_x+ii]<<DQ_BITS) + m7[jj][ii] + DQ_ROUND )>> DQ_BITS));

				}

			*ppSrcDstCoeff += 16;

		}

	}

	// V -- AC
	for(k=0; k<4; k++)									// CBP decision
	{
		block_x = block_idx[k<<1];
		block_y = block_idx[(k<<1) + 1];

		// This statement is different for U and V
		if(!(cbp&(1<<(6+k))))
		{
			// This block has no AC information : but we have DC information
			for(jj=0; jj<4; jj++)
				for(ii=0; ii<4; ii++)
				{
					pSrcDstVPlane[(block_y+jj)*srcdstUVStep+block_x+ii] = IClip(0, 255,prediction_arrayV[(block_y+jj)*8+block_x+ii] + ((dcResultV[k]+32)>>6));
				}

		}
		else
		{
			// Form acArray and do IQ
			for(i=1; i<16; i++)
			{
				acArray[i] = ((*ppSrcDstCoeff)[i] * dequant_coef[qp_rem_v][i]) << (qp_per_v);
				//idctcoeff = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem_uv][i/4][i%4]) << (qp_per_uv);
			}

			acArray[0] = dcResultV[k];			// Add DC coeff

			// Do Inverse transform
			// horizontal
			for (j=0;j<4;j++)
			{
				m5[0] = acArray[0+j*4];
				m5[1] = acArray[1+j*4];
				m5[2] = acArray[2+j*4];
				m5[3] = acArray[3+j*4];


				m6[0] = (m5[0]     + m5[2]);
				m6[1] = (m5[0]     - m5[2]);
				m6[2] = (m5[1]>>1) - m5[3];
				m6[3] = m5[1]      + (m5[3]>>1);


				m7[j][0]  = m6[0] + m6[3];
				m7[j][3]  = m6[0] - m6[3];
				m7[j][1]  = m6[1] + m6[2];
				m7[j][2]  = m6[1] - m6[2];

			}

			// vertical
			for (i=0;i<4;i++)
			{
				m5[0]=m7[0][i];
				m5[1]=m7[1][i];
				m5[2]=m7[2][i];
				m5[3]=m7[3][i];

				m6[0]=(m5[0]+m5[2]);
				m6[1]=(m5[0]-m5[2]);
				m6[2]=(m5[1]>>1)-m5[3];
				m6[3]=m5[1]+(m5[3]>>1);


				//m7[i][0]  = IClip(0,255,(m6[0]+m6[3]+(prediction_arrayU[i+ioff][0+joff] <<DQ_BITS)+DQ_ROUND)>>DQ_BITS);
				//m7[i][3]  = IClip(0,255,(m6[0]-m6[3]+(prediction_arrayU[i+ioff][3+joff] <<DQ_BITS)+DQ_ROUND)>>DQ_BITS);
				//m7[i][1]  = IClip(0,255,(m6[1]+m6[2]+(prediction_arrayU[i+ioff][1+joff] <<DQ_BITS)+DQ_ROUND)>>DQ_BITS);
				//m7[i][2]  = IClip(0,255,(m6[1]-m6[2]+(prediction_arrayU[i+ioff][2+joff] <<DQ_BITS)+DQ_ROUND)>>DQ_BITS);
				
				m7[0][i]  = m6[0]+m6[3];
				m7[3][i]  = m6[0]-m6[3];
				m7[1][i]  = m6[1]+m6[2];
				m7[2][i]  = m6[1]-m6[2];

			}

			for(jj=0; jj<4; jj++)
				for(ii=0; ii<4; ii++)
				{
					pSrcDstVPlane[(block_y+jj)*srcdstUVStep+block_x+ii] = IClip(0, 255,(((prediction_arrayV[(block_y+jj)*8 + block_x+ii]<<DQ_BITS) + m7[jj][ii] + DQ_ROUND )>> DQ_BITS));

				}

			*ppSrcDstCoeff += 16;

		}

	}
	// Third, we do add operation and clip operation for finnal result!!!!!!!!!!!!!!!!!!!!!!!!!

	Profile_End(ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_c_profile);

	return ippStsNoErr;
}



IppStatus __STDCALL ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_c(Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstUPlane,
            Ipp8u *pSrcDstVPlane,
            Ipp32u srcdstUVStep,
            IppIntraChromaPredMode_H264 intra_chroma_mode,
            Ipp32u cbp4x4,
            Ipp32u ChromaQPU,
            Ipp32u ChromaQPV,
            Ipp8u edge_type_top,
            Ipp8u edge_type_bottom,
            Ipp16s *pQuantTableU,
            Ipp16s *pQuantTableV,
            Ipp8u bypass_flag)
{
	_DECLSPEC unsigned char prediction_arrayU[64], prediction_arrayV[64];
	_DECLSPEC int		m7[4][4], m5[4], m6[4];
	_DECLSPEC int		acArray[16];
	_DECLSPEC int		dcArrayU[4], dcResultU[4];
	_DECLSPEC int		dcArrayV[4], dcResultV[4];

	const int dequant_dc[6] = {10, 11, 13, 14, 16, 18};

	int		block_idx[] = {0,0, 4,0, 0,4, 4,4};

	int		up_avail, left_avail_top, left_avail_bottom;
	int		i,	j, k;
	Ipp8u	*pSrcforPredictionU, *pSrcforPredictionV, *pSrcforPrediction;

	int		pitch = srcdstUVStep;

	int		iv, ih, ib, ic, iaa;	// Used for Plane prediction


	int		jsu0=0,	jsu1=0, jsu2=0, jsu3=0;
	int		jsv0=0,	jsv1=0, jsv2=0, jsv3=0;
	int		jsu[2][2], jsv[2][2];

	int		ii, jj, ioff, joff;

	int		qp_per_u, qp_per_v, qp_rem_u, qp_rem_v;


	unsigned int	cbp = cbp4x4>> 17;			// Use Chroma part ONLY


	int		block_x, block_y;

	Profile_Start(ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_c_profile);

	qp_per_u = ChromaQPU/6;
	qp_rem_u = ChromaQPU%6;
	qp_per_v = ChromaQPV/6;
	qp_rem_v = ChromaQPV%6;

	// First, we do intra-prediction according to edge_type and intra_luma_mode!!!!!!!!!!!!!!!!!!!!
	up_avail			= !(edge_type_top&IPPVC_TOP_EDGE);  // Just use it for two parts
	left_avail_top		= !(edge_type_top&IPPVC_LEFT_EDGE);
	left_avail_bottom	= !(edge_type_bottom&IPPVC_LEFT_EDGE);

	if (intra_chroma_mode == 0)			// DC mode
	{
		pSrcforPredictionU	= pSrcDstUPlane - srcdstUVStep;				// pointer to TOP U
		pSrcforPredictionV	= pSrcDstVPlane - srcdstUVStep;				// pointer to TOP V

		for(i=0;i<4;i++)
		{
			if(up_avail)
			{
				jsu0 = jsu0 + pSrcforPredictionU[i];
				jsu1 = jsu1 + pSrcforPredictionU[i+4];

				jsv0 = jsv0 + pSrcforPredictionV[i];
				jsv1 = jsv1 + pSrcforPredictionV[i+4];

			}
			if(left_avail_top)
			{
				jsu2 = jsu2 + pSrcDstUPlane[-1+i*srcdstUVStep];

				jsv2 = jsv2 + pSrcDstVPlane[-1+i*srcdstUVStep];
			}
			if(left_avail_top)
			{
				jsu3 = jsu3 + pSrcDstUPlane[-1+(i+4)*srcdstUVStep];

				jsv3 = jsv3 + pSrcDstVPlane[-1+(i+4)*srcdstUVStep];
			}
		}

		if(up_avail && left_avail_top)
		{
		  jsu[0][0]=(jsu0+jsu2+4)>>3;
		  jsu[1][0]=(jsu1+2)>>2;

		  jsv[0][0]=(jsv0+jsv2+4)>>3;
		  jsv[1][0]=(jsv1+2)>>2;
		}
		if(up_avail && left_avail_bottom)
		{
		  jsu[0][1]=(jsu3+2)>>2;
		  jsu[1][1]=(jsu1+jsu3+4)>>3;

		  jsv[0][1]=(jsv3+2)>>2;
		  jsv[1][1]=(jsv1+jsv3+4)>>3;
		}

		if(up_avail && !left_avail_top && !left_avail_bottom)
		{
		  jsu[0][0]=(jsu0+2)>>2;
		  jsu[1][0]=(jsu1+2)>>2;
		  jsu[0][1]=(jsu0+2)>>2;
		  jsu[1][1]=(jsu1+2)>>2;

		  jsv[0][0]=(jsv0+2)>>2;
		  jsv[1][0]=(jsv1+2)>>2;
		  jsv[0][1]=(jsv0+2)>>2;
		  jsv[1][1]=(jsv1+2)>>2;
		}

		if(left_avail_top && !up_avail)
		{
		  jsu[0][0]=(jsu2+2)>>2;
		  jsu[1][0]=(jsu2+2)>>2;

		  jsv[0][0]=(jsv2+2)>>2;
		  jsv[1][0]=(jsv2+2)>>2;
		}
		if(left_avail_bottom && !up_avail)
		{
		  jsu[0][1]=(jsu3+2)>>2;
		  jsu[1][1]=(jsu3+2)>>2;

		  jsv[0][1]=(jsv3+2)>>2;
		  jsv[1][1]=(jsv3+2)>>2;
		}

		if(!up_avail && !left_avail_top && !left_avail_bottom)
		{
		  jsu[0][0]=128;
		  jsu[1][0]=128;

		  jsu[0][1]=128;
		  jsu[1][1]=128;

		  jsv[0][0]=128;
		  jsv[1][0]=128;

		  jsv[0][1]=128;
		  jsv[1][1]=128;
		}
	}


	for (j=0;j<2;j++)
	{
		joff=j*4;

		for(i=0;i<2;i++)
		{
			ioff=i*4;

			switch (intra_chroma_mode)
			{
				case 0:									// DC_PRED_8
					for (jj=0; jj<4; jj++)
					  for (ii=0; ii<4; ii++)
					  {
							prediction_arrayU[joff*8+ioff+8*jj+ii] = jsu[i][j];
							prediction_arrayV[joff*8+ioff+8*jj+ii] = jsv[i][j];
					  }
				break;
				case 1:										// HOR_PRED_8
					pSrcforPredictionU	= pSrcDstUPlane - 1;				// pointer to left U
					pSrcforPredictionV	= pSrcDstVPlane - 1;				// pointer to left V

					for (jj=0; jj<4; jj++)
					{
						for (ii=0; ii<4; ii++)
						{
							prediction_arrayU[joff*8+ioff+8*jj+ii] = pSrcforPredictionU[(joff+jj)*srcdstUVStep];
							prediction_arrayV[joff*8+ioff+8*jj+ii] = pSrcforPredictionV[(joff+jj)*srcdstUVStep];

						}
					}

				break;
				case 2:										// VERT_PRED_8
					assert(up_avail);
					pSrcforPredictionU	= pSrcDstUPlane - srcdstUVStep;
					pSrcforPredictionV	= pSrcDstVPlane - srcdstUVStep;

					for (ii=0; ii<4; ii++)
					{
						//pred = imgUV[uv][up.pos_y][up.pos_x+ii+ioff];
						for (jj=0; jj<4; jj++)
						{
							prediction_arrayU[joff*8+ioff+8*jj+ii] = pSrcforPredictionU[ioff+ii];
							prediction_arrayV[joff*8+ioff+8*jj+ii] = pSrcforPredictionV[ioff+ii];
						}
					}
				break;
				case 3:										// PLANE_8

					// First, do prediction for U
					ih = iv = 0;

					pSrcforPredictionU	= pSrcDstUPlane -srcdstUVStep;	// Up
					pSrcforPrediction	= pSrcDstUPlane -1;				// Left

					for (ii=1;ii<5;ii++)
					{
						ih += ii*(pSrcforPredictionU[3+ii] - pSrcforPredictionU[3-ii]);
						iv += ii*(pSrcforPrediction[(3+ii)*srcdstUVStep] - pSrcforPrediction[(3-ii)*srcdstUVStep]);
					}
					ib=(17*ih+16)>>5;
					ic=(17*iv+16)>>5;

					iaa=16*(pSrcDstUPlane[7-srcdstUVStep] + pSrcDstUPlane[-1+7*srcdstUVStep]);
					//iaa=16*(pSrcDstYPlane[-1+15*srcdstYStep] + pSrcDstYPlane[15-srcdstYStep]);


					for (jj=0; jj<4; jj++)
						for (ii=0; ii<4; ii++)
							prediction_arrayU[joff*8+ioff+8*jj+ii] = IClip(0,255, ((iaa+(ii+ioff-3)*ib +(jj+joff-3)*ic + 16)>>5));

					// Second, do prediction for V
					ih = iv = 0;

					pSrcforPredictionV	= pSrcDstVPlane -srcdstUVStep;	// Up
					pSrcforPrediction	= pSrcDstVPlane -1;				// Left

					for (ii=1;ii<5;ii++)
					{
						ih += ii*(pSrcforPredictionV[3+ii] - pSrcforPredictionV[3-ii]);
						iv += ii*(pSrcforPrediction[(3+ii)*srcdstUVStep] - pSrcforPrediction[(3-ii)*srcdstUVStep]);
					}
					ib=(17*ih+16)>>5;
					ic=(17*iv+16)>>5;

					iaa=16*(pSrcDstVPlane[7-srcdstUVStep] + pSrcDstVPlane[-1+7*srcdstUVStep]);
					//iaa=16*(pSrcDstYPlane[-1+15*srcdstYStep] + pSrcDstYPlane[15-srcdstYStep]);


					for (jj=0; jj<4; jj++)
						for (ii=0; ii<4; ii++)
							prediction_arrayV[joff*8+ioff+8*jj+ii] = IClip(0,255, ((iaa+(ii+ioff-3)*ib +(jj+joff-3)*ic + 16)>>5));


				  break;
				default:
					//error("illegal chroma intra prediction mode", 600);
				break;
			}
		}
	}
	// Second, we do inverse transform for each 4x4 block!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//         include inverse Quantization and add operation

	if(!(cbp & 0xffff))
	{
		// Non- cbp has information: use prediction as coeffs ONLY  -- WWD in 2006-05-05

		for(j=0; j<8; j++)
			for(i=0; i<8; i++)
			{
				pSrcDstUPlane[j*srcdstUVStep+i] = prediction_arrayU[j*8+i];
				pSrcDstVPlane[j*srcdstUVStep+i] = prediction_arrayV[j*8+i];

			}

		return ippStsNoErr;
	}
	// U
	if(cbp & 0x01)
	{
		// 2.1 2x2 DC Transform
		// IPPVC_CBP_1ST_CHROMA_DC_BITPOS
		// 这里需要进一步处理QP<6的情况

		if(ChromaQPU >= 6)
		{
			for (i=0;i<4;i++)
				dcArrayU[i] = ((*ppSrcDstCoeff)[i]) *dequant_coef[qp_rem_u][0]<< (qp_per_u - 1);
		}
		else
		{
			for (i=0;i<4;i++)
				dcArrayU[i] = ((*ppSrcDstCoeff)[i]) *dequant_coef[qp_rem_u][0]>> 1;
		}

		dcResultU[0] = (dcArrayU[0] + dcArrayU[1] + dcArrayU[2] + dcArrayU[3]);
		dcResultU[1] = (dcArrayU[0] - dcArrayU[1] + dcArrayU[2] - dcArrayU[3]);
		dcResultU[2] = (dcArrayU[0] + dcArrayU[1] - dcArrayU[2] - dcArrayU[3]);
		dcResultU[3] = (dcArrayU[0] - dcArrayU[1] - dcArrayU[2] + dcArrayU[3]);


		*ppSrcDstCoeff += 4;

	}
	else
	{
		dcResultU[0] = 0;
		dcResultU[1] = 0;
		dcResultU[2] = 0;
		dcResultU[3] = 0;

	}


	// V
	if(cbp & 0x02)
	{
		if(ChromaQPV >= 6)
		{
			for (i=0;i<4;i++)
				dcArrayV[i] = ((*ppSrcDstCoeff)[i]) *dequant_coef[qp_rem_v][0]<< (qp_per_v - 1);
		}
		else
		{
			for (i=0;i<4;i++)
				dcArrayV[i] = ((*ppSrcDstCoeff)[i]) *dequant_coef[qp_rem_v][0] >>  1;
		}

		dcResultV[0] = (dcArrayV[0] + dcArrayV[1] + dcArrayV[2] + dcArrayV[3]);
		dcResultV[1] = (dcArrayV[0] - dcArrayV[1] + dcArrayV[2] - dcArrayV[3]);
		dcResultV[2] = (dcArrayV[0] + dcArrayV[1] - dcArrayV[2] - dcArrayV[3]);
		dcResultV[3] = (dcArrayV[0] - dcArrayV[1] - dcArrayV[2] + dcArrayV[3]);

		*ppSrcDstCoeff += 4;


	}
	else
	{
		dcResultV[0] = 0;
		dcResultV[1] = 0;
		dcResultV[2] = 0;
		dcResultV[3] = 0;


	}
	// U -- AC
	for(k=0; k<4; k++)									// CBP decision
	{
		block_x = block_idx[k<<1];
		block_y = block_idx[(k<<1) + 1];

		if(!(cbp&(1<<(2+k))))
		{
			// This block has no AC information : but we have DC information
			for(jj=0; jj<4; jj++)
				for(ii=0; ii<4; ii++)
				{
					pSrcDstUPlane[(block_y+jj)*srcdstUVStep+block_x+ii] = prediction_arrayU[(block_y+jj)*8+block_x+ii] + ((dcResultU[k]+32)>>6);
				}

		}
		else
		{

			// Form acArray and do IQ
			for(i=1; i<16; i++)
			{
				acArray[i] = ((*ppSrcDstCoeff)[i] * dequant_coef[qp_rem_u][i]) << (qp_per_u);
				//idctcoeff = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem_uv][i/4][i%4]) << (qp_per_uv);
			}

			acArray[0] = dcResultU[k];			// Add DC coeff

			// Do Inverse transform
			// horizontal
			for (j=0;j<4;j++)
			{
				m5[0] = acArray[0+j*4];
				m5[1] = acArray[1+j*4];
				m5[2] = acArray[2+j*4];
				m5[3] = acArray[3+j*4];


				m6[0] = (m5[0]     + m5[2]);
				m6[1] = (m5[0]     - m5[2]);
				m6[2] = (m5[1]>>1) - m5[3];
				m6[3] = m5[1]      + (m5[3]>>1);


				m7[j][0]  = m6[0] + m6[3];
				m7[j][3]  = m6[0] - m6[3];
				m7[j][1]  = m6[1] + m6[2];
				m7[j][2]  = m6[1] - m6[2];

			}

			// vertical
			for (i=0;i<4;i++)
			{
				m5[0]=m7[0][i];
				m5[1]=m7[1][i];
				m5[2]=m7[2][i];
				m5[3]=m7[3][i];

				m6[0]=(m5[0]+m5[2]);
				m6[1]=(m5[0]-m5[2]);
				m6[2]=(m5[1]>>1)-m5[3];
				m6[3]=m5[1]+(m5[3]>>1);
			
				m7[0][i]  = m6[0]+m6[3];
				m7[3][i]  = m6[0]-m6[3];
				m7[1][i]  = m6[1]+m6[2];
				m7[2][i]  = m6[1]-m6[2];

			}

			for(jj=0; jj<4; jj++)
				for(ii=0; ii<4; ii++)
				{
					pSrcDstUPlane[(block_y+jj)*srcdstUVStep+block_x+ii] = IClip(0, 255,(((prediction_arrayU[(block_y+jj)*8 + block_x+ii]<<DQ_BITS) + m7[jj][ii] + DQ_ROUND )>> DQ_BITS));

				}

			*ppSrcDstCoeff += 16;

		}

	}

	// V -- AC
	for(k=0; k<4; k++)									// CBP decision
	{
		block_x = block_idx[k<<1];
		block_y = block_idx[(k<<1) + 1];

		// This statement is different for U and V
		if(!(cbp&(1<<(6+k))))
		{
			// This block has no AC information : but we have DC information
			for(jj=0; jj<4; jj++)
				for(ii=0; ii<4; ii++)
				{
					pSrcDstVPlane[(block_y+jj)*srcdstUVStep+block_x+ii] = IClip(0, 255,prediction_arrayV[(block_y+jj)*8+block_x+ii] + ((dcResultV[k]+32)>>6));
				}

		}
		else
		{
			// Form acArray and do IQ
			for(i=1; i<16; i++)
			{
				acArray[i] = ((*ppSrcDstCoeff)[i] * dequant_coef[qp_rem_v][i]) << (qp_per_v);
				//idctcoeff = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem_uv][i/4][i%4]) << (qp_per_uv);
			}

			acArray[0] = dcResultV[k];			// Add DC coeff

			// Do Inverse transform
			// horizontal
			for (j=0;j<4;j++)
			{
				m5[0] = acArray[0+j*4];
				m5[1] = acArray[1+j*4];
				m5[2] = acArray[2+j*4];
				m5[3] = acArray[3+j*4];


				m6[0] = (m5[0]     + m5[2]);
				m6[1] = (m5[0]     - m5[2]);
				m6[2] = (m5[1]>>1) - m5[3];
				m6[3] = m5[1]      + (m5[3]>>1);


				m7[j][0]  = m6[0] + m6[3];
				m7[j][3]  = m6[0] - m6[3];
				m7[j][1]  = m6[1] + m6[2];
				m7[j][2]  = m6[1] - m6[2];

			}

			// vertical
			for (i=0;i<4;i++)
			{
				m5[0]=m7[0][i];
				m5[1]=m7[1][i];
				m5[2]=m7[2][i];
				m5[3]=m7[3][i];

				m6[0]=(m5[0]+m5[2]);
				m6[1]=(m5[0]-m5[2]);
				m6[2]=(m5[1]>>1)-m5[3];
				m6[3]=m5[1]+(m5[3]>>1);


				//m7[i][0]  = IClip(0,255,(m6[0]+m6[3]+(prediction_arrayU[i+ioff][0+joff] <<DQ_BITS)+DQ_ROUND)>>DQ_BITS);
				//m7[i][3]  = IClip(0,255,(m6[0]-m6[3]+(prediction_arrayU[i+ioff][3+joff] <<DQ_BITS)+DQ_ROUND)>>DQ_BITS);
				//m7[i][1]  = IClip(0,255,(m6[1]+m6[2]+(prediction_arrayU[i+ioff][1+joff] <<DQ_BITS)+DQ_ROUND)>>DQ_BITS);
				//m7[i][2]  = IClip(0,255,(m6[1]-m6[2]+(prediction_arrayU[i+ioff][2+joff] <<DQ_BITS)+DQ_ROUND)>>DQ_BITS);
				
				m7[0][i]  = m6[0]+m6[3];
				m7[3][i]  = m6[0]-m6[3];
				m7[1][i]  = m6[1]+m6[2];
				m7[2][i]  = m6[1]-m6[2];

			}

			for(jj=0; jj<4; jj++)
				for(ii=0; ii<4; ii++)
				{
					pSrcDstVPlane[(block_y+jj)*srcdstUVStep+block_x+ii] = IClip(0, 255,(((prediction_arrayV[(block_y+jj)*8 + block_x+ii]<<DQ_BITS) + m7[jj][ii] + DQ_ROUND )>> DQ_BITS));

				}

			*ppSrcDstCoeff += 16;

		}

	}
	// Third, we do add operation and clip operation for finnal result!!!!!!!!!!!!!!!!!!!!!!!!!

	Profile_End(ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_c_profile);

	return ippStsNoErr;
}




IppStatus __STDCALL ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_c(Ipp16s **ppSrcDstCoeff,
            Ipp8u *pSrcDstUPlane,
            Ipp8u *pSrcDstVPlane,
            Ipp32u srcdstUVStep,
            Ipp32u cbp4x4,
            Ipp32u ChromaQPU,
            Ipp32u ChromaQPV,
            Ipp16s *pQuantTableU,
            Ipp16s *pQuantTableV,
            Ipp8u bypass_flag)

{
	_DECLSPEC int		acArray[16];
	int		dcArrayU[4], dcResultU[4];
	int		dcArrayV[4], dcResultV[4];

	int		m7[4][4], m5[4], m6[4];
	const int dequant_dc[6] = {10, 11, 13, 14, 16, 18};

	int		block_idx[] = {0,0, 4,0, 0,4, 4,4};

	int		i,	j, k;

	int		pitch = srcdstUVStep;
	int		ii, jj;

	int		qp_per_u, qp_per_v, qp_rem_u, qp_rem_v;


	unsigned int	cbp = cbp4x4>> 17;			// Use Chroma part ONLY


	int		block_x, block_y;

	Profile_Start(ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_c_profile);

	qp_per_u = ChromaQPU/6;
	qp_rem_u = ChromaQPU%6;
	qp_per_v = ChromaQPV/6;
	qp_rem_v = ChromaQPV%6;



	if(!(cbp & 0xffff))
	{
		// This branch is not tested yet, but OK! -- WWD in 2006-06-15

		return ippStsNoErr;
	}


	// U
	if(cbp & 0x01)
	{
		// 2.1 2x2 DC Transform
		// IPPVC_CBP_1ST_CHROMA_DC_BITPOS
		// 这里需要进一步处理QP<6的情况

		if(ChromaQPU >= 6)
		{
			for (i=0;i<4;i++)
				dcArrayU[i] = ((*ppSrcDstCoeff)[i]) *dequant_coef[qp_rem_u][0]<< (qp_per_u - 1);
		}
		else
		{
			for (i=0;i<4;i++)
				dcArrayU[i] = ((*ppSrcDstCoeff)[i]) *dequant_coef[qp_rem_u][0] >> 1;
		}

		dcResultU[0] = (dcArrayU[0] + dcArrayU[1] + dcArrayU[2] + dcArrayU[3]);
		dcResultU[1] = (dcArrayU[0] - dcArrayU[1] + dcArrayU[2] - dcArrayU[3]);
		dcResultU[2] = (dcArrayU[0] + dcArrayU[1] - dcArrayU[2] - dcArrayU[3]);
		dcResultU[3] = (dcArrayU[0] - dcArrayU[1] - dcArrayU[2] + dcArrayU[3]);


		*ppSrcDstCoeff += 4;

#ifdef __WWD_BRANCH_TEST__
			test_branch[46] = 1;
#endif
	}
	else
	{
		dcResultU[0] = 0;
		dcResultU[1] = 0;
		dcResultU[2] = 0;
		dcResultU[3] = 0;

#ifdef __WWD_BRANCH_TEST__
			test_branch[47] = 1;
#endif
	}


	// V
	if(cbp & 0x02)
	{
		if(ChromaQPV >= 6)
		{
			for (i=0;i<4;i++)
				dcArrayV[i] = ((*ppSrcDstCoeff)[i]) *dequant_coef[qp_rem_v][0]<< (qp_per_v - 1);

		}
		else
		{
			for (i=0;i<4;i++)
				dcArrayV[i] = ((*ppSrcDstCoeff)[i]) *dequant_coef[qp_rem_v][0] >>  1;

		}

		dcResultV[0] = (dcArrayV[0] + dcArrayV[1] + dcArrayV[2] + dcArrayV[3]);
		dcResultV[1] = (dcArrayV[0] - dcArrayV[1] + dcArrayV[2] - dcArrayV[3]);
		dcResultV[2] = (dcArrayV[0] + dcArrayV[1] - dcArrayV[2] - dcArrayV[3]);
		dcResultV[3] = (dcArrayV[0] - dcArrayV[1] - dcArrayV[2] + dcArrayV[3]);

		*ppSrcDstCoeff += 4;

#ifdef __WWD_BRANCH_TEST__
			test_branch[48] = 1;
#endif
	}
	else
	{
		dcResultV[0] = 0;
		dcResultV[1] = 0;
		dcResultV[2] = 0;
		dcResultV[3] = 0;

#ifdef __WWD_BRANCH_TEST__
			test_branch[49] = 1;
#endif
	}
	// U -- AC
	for(k=0; k<4; k++)									// CBP decision
	{
		block_x = block_idx[k<<1];
		block_y = block_idx[(k<<1) + 1];

		if(!(cbp&(1<<(2+k))))
		{
			// This block has no AC information : but we have DC information
			for(jj=0; jj<4; jj++)
				for(ii=0; ii<4; ii++)
				{
					pSrcDstUPlane[(block_y+jj)*srcdstUVStep+block_x+ii] = IClip(0, 255,pSrcDstUPlane[(block_y+jj)*srcdstUVStep+block_x+ii] + ((dcResultU[k]+32)>>6));
				}

#ifdef __WWD_BRANCH_TEST__
			test_branch[50] = 1;
#endif
		}
		else
		{
			// Form acArray and do IQ
			for(i=1; i<16; i++)
			{
				acArray[i] = ((*ppSrcDstCoeff)[i] * dequant_coef[qp_rem_u][i]) << (qp_per_u);
			}

			acArray[0] = dcResultU[k];			// Add DC coeff

			// Do Inverse transform
			// horizontal
			for (j=0;j<4;j++)
			{
				m5[0] = acArray[0+j*4];
				m5[1] = acArray[1+j*4];
				m5[2] = acArray[2+j*4];
				m5[3] = acArray[3+j*4];


				m6[0] = (m5[0]     + m5[2]);
				m6[1] = (m5[0]     - m5[2]);
				m6[2] = (m5[1]>>1) - m5[3];
				m6[3] = m5[1]      + (m5[3]>>1);


				m7[j][0]  = m6[0] + m6[3];
				m7[j][3]  = m6[0] - m6[3];
				m7[j][1]  = m6[1] + m6[2];
				m7[j][2]  = m6[1] - m6[2];

			}

			// vertical
			for (i=0;i<4;i++)
			{
				m5[0]=m7[0][i];
				m5[1]=m7[1][i];
				m5[2]=m7[2][i];
				m5[3]=m7[3][i];

				m6[0]=(m5[0]+m5[2]);
				m6[1]=(m5[0]-m5[2]);
				m6[2]=(m5[1]>>1)-m5[3];
				m6[3]=m5[1]+(m5[3]>>1);
			
				m7[0][i]  = m6[0]+m6[3];
				m7[3][i]  = m6[0]-m6[3];
				m7[1][i]  = m6[1]+m6[2];
				m7[2][i]  = m6[1]-m6[2];

			}

			for(jj=0; jj<4; jj++)
				for(ii=0; ii<4; ii++)
				{
					pSrcDstUPlane[(block_y+jj)*srcdstUVStep+block_x+ii] = IClip(0, 255,(((pSrcDstUPlane[(block_y+jj)*srcdstUVStep+block_x+ii]<<DQ_BITS) + m7[jj][ii] + DQ_ROUND )>> DQ_BITS));

				}

			*ppSrcDstCoeff += 16;

#ifdef __WWD_BRANCH_TEST__
			test_branch[51] = 1;
#endif

		}

	}

	// V -- AC
	for(k=0; k<4; k++)									// CBP decision
	{
		block_x = block_idx[k<<1];
		block_y = block_idx[(k<<1) + 1];

		// This statement is different for U and V
		if(!(cbp&(1<<(6+k))))
		{
			// This block has no AC information : but we have DC information
			for(jj=0; jj<4; jj++)
				for(ii=0; ii<4; ii++)
				{
					pSrcDstVPlane[(block_y+jj)*srcdstUVStep+block_x+ii] = IClip(0, 255,pSrcDstVPlane[(block_y+jj)*srcdstUVStep+block_x+ii] + ((dcResultV[k]+32)>>6));
				}

#ifdef __WWD_BRANCH_TEST__
			test_branch[52] = 1;
#endif

		}
		else
		{
			// Form acArray and do IQ
			for(i=1; i<16; i++)
			{
				acArray[i] = ((*ppSrcDstCoeff)[i] * dequant_coef[qp_rem_v][i]) << (qp_per_v);
			}

			acArray[0] = dcResultV[k];			// Add DC coeff

			// Do Inverse transform
			// horizontal
			for (j=0;j<4;j++)
			{
				m5[0] = acArray[0+j*4];
				m5[1] = acArray[1+j*4];
				m5[2] = acArray[2+j*4];
				m5[3] = acArray[3+j*4];


				m6[0] = (m5[0]     + m5[2]);
				m6[1] = (m5[0]     - m5[2]);
				m6[2] = (m5[1]>>1) - m5[3];
				m6[3] = m5[1]      + (m5[3]>>1);


				m7[j][0]  = m6[0] + m6[3];
				m7[j][3]  = m6[0] - m6[3];
				m7[j][1]  = m6[1] + m6[2];
				m7[j][2]  = m6[1] - m6[2];

			}

			// vertical
			for (i=0;i<4;i++)
			{
				m5[0]=m7[0][i];
				m5[1]=m7[1][i];
				m5[2]=m7[2][i];
				m5[3]=m7[3][i];

				m6[0]=(m5[0]+m5[2]);
				m6[1]=(m5[0]-m5[2]);
				m6[2]=(m5[1]>>1)-m5[3];
				m6[3]=m5[1]+(m5[3]>>1);

	
				m7[0][i]  = m6[0]+m6[3];
				m7[3][i]  = m6[0]-m6[3];
				m7[1][i]  = m6[1]+m6[2];
				m7[2][i]  = m6[1]-m6[2];

			}

			for(jj=0; jj<4; jj++)
				for(ii=0; ii<4; ii++)
				{
					pSrcDstVPlane[(block_y+jj)*srcdstUVStep+block_x+ii] = IClip(0, 255,(((pSrcDstVPlane[(block_y+jj)*srcdstUVStep+block_x+ii]<<DQ_BITS) + m7[jj][ii] + DQ_ROUND )>> DQ_BITS));

				}

			*ppSrcDstCoeff += 16;

#ifdef __WWD_BRANCH_TEST__
			test_branch[53] = 1;
#endif

		}

	}

	Profile_End(ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_c_profile);

	return ippStsNoErr;

}


#endif

#endif
