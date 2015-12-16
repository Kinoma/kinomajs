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
#include <math.h>
#include <memory.h>
#include <assert.h>

#include "kinoma_avc_defines.h"
#include "ippvc.h"
#include "kinoma_ipp_lib.h"
#include "kinoma_utilities.h"

IppStatus (__STDCALL *ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane,Ipp32u srcdstYStep,const IppIntra16x16PredMode_H264 intra_luma_mode,const Ipp32u cbp4x4, const Ipp32u QP,const Ipp8u edge_type)=NULL;
IppStatus (__STDCALL *ippiReconstructChromaIntraMB_H264_16s8u_P2R_universal)			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, const Ipp32u srcdstUVStep, const IppIntraChromaPredMode_H264 intra_chroma_mode, const Ipp32u cbp4x4, const Ipp32u ChromaQP, const Ipp8u edge_type)=NULL;
IppStatus (__STDCALL *ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_universal)		(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, Ipp32u srcdstUVStep, IppIntraChromaPredMode_H264 intra_chroma_mode, Ipp32u cbp4x4, Ipp32u ChromaQP, Ipp8u edge_type_top, Ipp8u edge_type_bottom)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntraMB_H264_16s8u_C1R_universal)				(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, const IppIntra4x4PredMode_H264 *pMBIntraTypes, const Ipp32u cbp4x4, const Ipp32u QP, const Ipp8u edge_type)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x2, Ipp32u QP, Ipp8u edge_type)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaInterMB_H264_16s8u_C1R_universal)				(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp4x4, Ipp32s QP)=NULL;
IppStatus (__STDCALL *ippiReconstructChromaInterMB_H264_16s8u_P2R_universal)			(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstUPlane, Ipp8u *pSrcDstVPlane, const Ipp32u srcdstUVStep, const Ipp32u cbp4x4, const Ipp32u ChromaQP)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_universal)		(Ipp16s **ppSrcCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, const IppIntra16x16PredMode_H264 intra_luma_mode, const Ipp32u cbp4x4, const Ipp32u QP, const Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag /*Resevr ONLY : do not support at present */)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra8x8PredMode_H264 *pMBIntraTypes, Ipp32u cbp8x8, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_universal)		(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra8x8PredMode_H264 *pMBIntraTypes, Ipp32u cbp8x2, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp8x8, Ipp32s QP, Ipp16s *pQuantTable, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x4, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_universal)		(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32s srcdstYStep, IppIntra4x4PredMode_H264 *pMBIntraTypes, Ipp32u cbp4x2, Ipp32u QP, Ipp8u edge_type, Ipp16s *pQuantTable, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_universal)			(Ipp16s **ppSrcDstCoeff, Ipp8u *pSrcDstYPlane, Ipp32u srcdstYStep, Ipp32u cbp4x4, Ipp32s QP, Ipp16s *pQuantTable, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_universal)			(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep, IppIntraChromaPredMode_H264 intra_chroma_mode, Ipp32u cbp4x4, Ipp32u ChromaQPU, Ipp32u ChromaQPV, Ipp8u edge_type, Ipp16s *pQuantTableU, Ipp16s *pQuantTableV, Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_universal)	(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep,IppIntraChromaPredMode_H264 intra_chroma_mode,Ipp32u cbp4x4,Ipp32u ChromaQPU,Ipp32u ChromaQPV,Ipp8u edge_type_top,Ipp8u edge_type_bottom,Ipp16s *pQuantTableU,Ipp16s *pQuantTableV,Ipp8u bypass_flag)=NULL;
IppStatus (__STDCALL *ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_universal)			(Ipp16s **ppSrcDstCoeff,Ipp8u *pSrcDstUPlane,Ipp8u *pSrcDstVPlane,Ipp32u srcdstUVStep,Ipp32u cbp4x4,Ipp32u ChromaQPU,Ipp32u ChromaQPV,Ipp16s *pQuantTableU,Ipp16s *pQuantTableV,Ipp8u bypass_flag)=NULL;

#ifdef __KINOMA_IPP__

#ifdef __WWD_BRANCH_TEST__
	int		test_branch[256] ;
#endif

#define HAS_AA (!(edge_type&IPPVC_TOP_EDGE))
#define HAS_BB (!(edge_type&IPPVC_LEFT_EDGE))
#define HAS_CC (!(edge_type&IPPVC_TOP_RIGHT_EDGE))
#define HAS_DD (!(edge_type&IPPVC_TOP_LEFT_EDGE))

#define DQ_BITS         6
#define DQ_ROUND        (1<<(DQ_BITS-1))

// For reference only 
#define P_X (PredPel[0])
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

//_DECLSPEC static	const int dequant_coef[6][16] = {
_DECLSPEC static Ipp16u dequant_coef[6][16] = 
{
	{10, 13, 10, 13,   13, 16, 13, 16,   10, 13, 10, 13,   13, 16, 13, 16},
	{11, 14, 11, 14,   14, 18, 14, 18,   11, 14, 11, 14,   14, 18, 14, 18},
	{13, 16, 13, 16,   16, 20, 16, 20,   13, 16, 13, 16,   16, 20, 16, 20},
	{14, 18, 14, 18,   18, 23, 18, 23,   14, 18, 14, 18,   18, 23, 18, 23},
	{16, 20, 16, 20,   20, 25, 20, 25,   16, 20, 16, 20,   20, 25, 20, 25},
	{18, 23, 18, 23,   23, 29, 23, 29,   18, 23, 18, 23,   23, 29, 23, 29}
};

// These two array use for DC and AC mode for Luma -- JVTG50 6.4.3  -- WWD
_DECLSPEC static	const Ipp8u	ac_block_idx_x[16]		= {0, 4, 0, 4, 8, 12, 8, 12, 0, 4, 0, 4,  8, 12,  8, 12 };
_DECLSPEC static	const Ipp8u	ac_block_idx_y[16]		= {0, 0, 4, 4, 0,  0, 4,  4, 8, 8, 12,12, 8,  8, 12, 12 };
_DECLSPEC static	const Ipp8u	decode_block_scan[16]	= {0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15};		// For DC index in AC mode -- WWD
_DECLSPEC static	const int	cbpmask[16]				= {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};

#ifndef DROP_MBAFF
_DECLSPEC static	const int	cbpmask_half[8]			= {1, 2, 4, 8, 16, 32, 64, 128}; //, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
_DECLSPEC static	const Ipp8u	dequant_dc[6]			= {10, 11, 13, 14, 16, 18};
#endif

_DECLSPEC static	const Ipp8u	block_idx[]				= {0,0, 4,0, 0,4, 4,4};

#ifndef DROP_MBAFF

/******************************************************************************************************
 * Do 8*8 block reconstruction for intra-Chroma
 * Use different edge type for this 8x8 block
 * The difference of this function and above function lies in:
 *            1, 
 ******************************************************************************************************
 */
/******************************************************************************************************
 * Do 8*16 block reconstruction for intra-LUMA
 * This function almost have same code as function ippiReconstructLumaIntraMB_H264_16s8u_C1R_x except 
 *              ## edge part decision part, please note this when we do optimization for it using AL
 ******************************************************************************************************
 */
IppStatus __STDCALL ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_c(Ipp16s **ppSrcCoeff,
                                                                Ipp8u *pSrcDstYPlane,
                                                                Ipp32s srcdstYStep,
                                                                IppIntra4x4PredMode_H264 *pMBIntraTypes,
                                                                Ipp32u cbp4x2,
                                                                Ipp32u QP,
                                                                Ipp8u edge_type)
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
static int top_filed = 1;
	_DECLSPEC unsigned char prediction_array[16];

	// These default value are changed according to above function {Change from 16 --> 8}
	_DECLSPEC int		avail_left[8]	    = { 1, 1, 1, 1, 1, 1, 1, 1}; //,    1, 1, 1, 1, 1, 1, 1, 1};
	_DECLSPEC int		avail_top [8]	    = { 1, 1, 1, 1, 1, 1, 1, 1}; //,    1, 1, 1, 1, 1, 1, 1, 1};
	_DECLSPEC int		avail_top_left[8]   = { 1, 1, 1, 1, 1, 1, 1, 1}; //,    1, 1, 1, 1, 1, 1, 1, 1};
	_DECLSPEC int		avail_top_right[8]  = { 1, 1, 1, 0, 1, 1, 1, 0}; //,    1, 1, 1, 0, 1, 0, 1, 0};

	_DECLSPEC int		acArray[16];
	_DECLSPEC int		PredPel[16];  // array of predictor pels
	_DECLSPEC int		m7[4][4], m5[4], m6[4];

	int		block_x, block_y;
	unsigned char * pPredictionArray;
	int		i,	j, k, t1, t2;
	Ipp8u	*pSrcforPrediction;
	int		s0;				// Used for DC prediction
	int		qp_per, qp_rem;
	// We do not need the first bit actually：
	unsigned int  cbp = cbp4x2 & 0x1ffff;		// Need 8 bits to represent Luma information

	Profile_Start(ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_c_profile);

	qp_per = QP/6;
	qp_rem = QP%6;

	//Just one workaround now
	if(top_filed == 1)
	{
		// First, we do intra-prediction according to edge_type and intra_luma_mode!!!!!!!!!!!!!!!!!!!!
		// 1-1, we need compute avail array for three array [ left, top and top_left]
		// TOP first
		avail_top[0] = 
		avail_top[1] = 
		avail_top[4] = 
		avail_top[5] =  HAS_AA;

		// Then LEFT
		avail_left[0] = 
		avail_left[2] = HAS_BB;

		// Last TOP_LEFT
		avail_top_left[0] =  HAS_DD;
		avail_top_left[1] = 
		avail_top_left[4] = 
		avail_top_left[5] = HAS_AA;
		avail_top_left[2] = HAS_BB;

		// TOP_RIGHT
		avail_top_right[0] = 
		avail_top_right[1] = 
		avail_top_right[4] = HAS_AA;
		avail_top_right[5] = HAS_CC;

		top_filed = 0;
	}
	else
	{
		// First, we do intra-prediction according to edge_type and intra_luma_mode!!!!!!!!!!!!!!!!!!!!
		// 1-1, we need compute avail array for three array [ left, top and top_left]
		// TOP first

		// Then LEFT
		avail_left[0] = 
		avail_left[2] = HAS_BB;

		// Last TOP_LEFT
		avail_top_left[0] = HAS_DD;
		avail_top_left[2] = HAS_BB;

		// TOP_RIGHT
		avail_top_right[5] = HAS_CC;

		top_filed = 1;
	}
	for(k=0; k<8; k++)			// In scan order (4x4 blocks)--- For this function we have 8 block4x4s only
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
		if( cbp & cbpmask_half[k])		// cbpmask
		{
			// Form acArray and do IQ
			//memset(acArray, 0, 64);
			for(i=0; i<16; i++)
			{
				//if((*ppSrcCoeff)[i])
					acArray[i] = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem][i]) << (qp_per);
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

			*ppSrcCoeff += 16;
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


	Profile_End(ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_c_profile);

	return ippStsNoErr;
}


IppStatus __STDCALL ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_c(Ipp16s **ppSrcCoeff,
                                                                Ipp8u *pSrcDstUPlane,
                                                                Ipp8u *pSrcDstVPlane,
                                                                Ipp32u srcdstUVStep,
                                                                IppIntraChromaPredMode_H264 intra_chroma_mode,
                                                                Ipp32u cbp4x4,
                                                                Ipp32u ChromaQP,
                                                                Ipp8u edge_type_top,
																Ipp8u edge_type_bottom)
{
	_DECLSPEC unsigned char prediction_arrayU[64], prediction_arrayV[64];
	_DECLSPEC int		m7[4][4], m5[4], m6[4];
	_DECLSPEC int		acArray[16];
	_DECLSPEC int		dcArrayU[4], dcResultU[4];
	_DECLSPEC int		dcArrayV[4], dcResultV[4];

	int		up_avail, left_avail_top, left_avail_bottom; //, up_left_avail;
	int		i,	j, k;
	Ipp8u	*pSrcforPredictionU, *pSrcforPredictionV, *pSrcforPrediction;

	int		iv, ih, ib, ic, iaa;	// Used for Plane prediction


	int		jsu0=0,	jsu1=0, jsu2=0, jsu3=0;
	int		jsv0=0,	jsv1=0, jsv2=0, jsv3=0;
	int		jsu[2][2], jsv[2][2];

	int		ii, jj, ioff, joff;

	int		qp_per_uv, qp_rem_uv;


	unsigned int	cbp = cbp4x4>> 17;			// Use Chroma part ONLY


	int		block_x, block_y;


	Profile_Start(ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_c_profile);

	qp_per_uv = ChromaQP/6;
	qp_rem_uv = ChromaQP%6;

	// First, we do intra-prediction according to edge_type and intra_luma_mode!!!!!!!!!!!!!!!!!!!!
	up_avail			= !(edge_type_top&IPPVC_TOP_EDGE);  // Just use it for two parts
	left_avail_top		= !(edge_type_top&IPPVC_LEFT_EDGE);
	left_avail_bottom	= !(edge_type_bottom&IPPVC_LEFT_EDGE);
	//up_left_avail	= !(edge_type&IPPVC_TOP_LEFT_EDGE);

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
			if(left_avail_bottom)
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

		if(!up_avail && !left_avail_top &!left_avail_bottom)
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


	// Intra prediction mode is same for top half and bottom half
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

		if(ChromaQP >= 6)
		{
			for (i=0;i<4;i++)
				dcArrayU[i] = ((*ppSrcCoeff)[i]) *dequant_coef[qp_rem_uv][0]<< (qp_per_uv - 1);
		}
		else
		{
			for (i=0;i<4;i++)
				dcArrayU[i] = ((*ppSrcCoeff)[i]) *dequant_coef[qp_rem_uv][0]>> 1;
		}

		dcResultU[0] = (dcArrayU[0] + dcArrayU[1] + dcArrayU[2] + dcArrayU[3]);
		dcResultU[1] = (dcArrayU[0] - dcArrayU[1] + dcArrayU[2] - dcArrayU[3]);
		dcResultU[2] = (dcArrayU[0] + dcArrayU[1] - dcArrayU[2] - dcArrayU[3]);
		dcResultU[3] = (dcArrayU[0] - dcArrayU[1] - dcArrayU[2] + dcArrayU[3]);


		*ppSrcCoeff += 4;

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
		if(ChromaQP >= 6)
		{
			for (i=0;i<4;i++)
				dcArrayV[i] = ((*ppSrcCoeff)[i]) *dequant_coef[qp_rem_uv][0]<< (qp_per_uv - 1);
		}
		else
		{
			for (i=0;i<4;i++)
				dcArrayV[i] = ((*ppSrcCoeff)[i]) *dequant_coef[qp_rem_uv][0] >>  1;
		}

		dcResultV[0] = (dcArrayV[0] + dcArrayV[1] + dcArrayV[2] + dcArrayV[3]);
		dcResultV[1] = (dcArrayV[0] - dcArrayV[1] + dcArrayV[2] - dcArrayV[3]);
		dcResultV[2] = (dcArrayV[0] + dcArrayV[1] - dcArrayV[2] - dcArrayV[3]);
		dcResultV[3] = (dcArrayV[0] - dcArrayV[1] - dcArrayV[2] + dcArrayV[3]);

		*ppSrcCoeff += 4;


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
				acArray[i] = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem_uv][i]) << (qp_per_uv);
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

			*ppSrcCoeff += 16;

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
				acArray[i] = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem_uv][i]) << (qp_per_uv);
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

			*ppSrcCoeff += 16;

		}

	}
	// Third, we do add operation and clip operation for finnal result!!!!!!!!!!!!!!!!!!!!!!!!!

	Profile_End(ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_c_profile);

	return ippStsNoErr;
}

#endif

#if 0

// This array use for DC only mode
_DECLSPEC static	const int		dc_block_idx_x[16] = {0, 4, 8, 12, 0, 4, 8, 12, 0, 4, 8, 12,  0,  4,  8, 12 };
_DECLSPEC static	const int		dc_block_idx_y[16] = {0, 0, 0, 0,  4, 4, 4, 4,  8, 8, 8, 8,  12, 12, 12, 12 };

//dummy
void Init_AVC_Reconstruction(int implementation)
{
}

IppStatus __STDCALL ippiReconstructChromaInterMB_H264_16s8u_P2R_c(Ipp16s **ppSrcCoeff,
                                                                Ipp8u *pSrcDstUPlane,
                                                                Ipp8u *pSrcDstVPlane,
                                                                const Ipp32u srcdstUVStep,
                                                                const Ipp32u cbp4x4,
                                                                const Ipp32u ChromaQP)
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

	int		qp_per_uv, qp_rem_uv;


	unsigned int	cbp = cbp4x4>> 17;			// Use Chroma part ONLY


	int		block_x, block_y;


	Profile_Start(ippiReconstructChromaInterMB_H264_16s8u_P2R_c_profile);

	qp_per_uv = ChromaQP/6;
	qp_rem_uv = ChromaQP%6;


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
		if(ChromaQP >= 6)
		{
			for (i=0;i<4;i++)
				dcArrayU[i] = ((*ppSrcCoeff)[i]) *dequant_coef[qp_rem_uv][0]<< (qp_per_uv - 1);
		}
		else
		{
			for (i=0;i<4;i++)
				dcArrayU[i] = ((*ppSrcCoeff)[i]) *dequant_coef[qp_rem_uv][0] >> 1;
		}

		dcResultU[0] = (dcArrayU[0] + dcArrayU[1] + dcArrayU[2] + dcArrayU[3]);
		dcResultU[1] = (dcArrayU[0] - dcArrayU[1] + dcArrayU[2] - dcArrayU[3]);
		dcResultU[2] = (dcArrayU[0] + dcArrayU[1] - dcArrayU[2] - dcArrayU[3]);
		dcResultU[3] = (dcArrayU[0] - dcArrayU[1] - dcArrayU[2] + dcArrayU[3]);


		*ppSrcCoeff += 4;

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
		if(ChromaQP >= 6)
		{
			for (i=0;i<4;i++)
				dcArrayV[i] = ((*ppSrcCoeff)[i]) *dequant_coef[qp_rem_uv][0]<< (qp_per_uv - 1);

		}
		else
		{
			for (i=0;i<4;i++)
				dcArrayV[i] = ((*ppSrcCoeff)[i]) *dequant_coef[qp_rem_uv][0] >>  1;

		}

		dcResultV[0] = (dcArrayV[0] + dcArrayV[1] + dcArrayV[2] + dcArrayV[3]);
		dcResultV[1] = (dcArrayV[0] - dcArrayV[1] + dcArrayV[2] - dcArrayV[3]);
		dcResultV[2] = (dcArrayV[0] + dcArrayV[1] - dcArrayV[2] - dcArrayV[3]);
		dcResultV[3] = (dcArrayV[0] - dcArrayV[1] - dcArrayV[2] + dcArrayV[3]);

		*ppSrcCoeff += 4;

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
				acArray[i] = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem_uv][i]) << (qp_per_uv);
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

			*ppSrcCoeff += 16;

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
				acArray[i] = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem_uv][i]) << (qp_per_uv);
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

			*ppSrcCoeff += 16;

#ifdef __WWD_BRANCH_TEST__
			test_branch[53] = 1;
#endif

		}

	}


	Profile_End(ippiReconstructChromaInterMB_H264_16s8u_P2R_c_profile);


	return ippStsNoErr;

}




/******************************************************************************************************
 * Do 16*16 block reconstruction for intra-LUMA
 ******************************************************************************************************
 */
IppStatus __STDCALL ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c(Ipp16s **ppSrcCoeff,
                                                                   Ipp8u *pSrcDstYPlane,
                                                                   Ipp32u srcdstYStep,
                                                                   const IppIntra16x16PredMode_H264 intra_luma_mode,
                                                                   const Ipp32u cbp4x4,
                                                                   const Ipp32u QP,
                                                                   const Ipp8u edge_type)
{

	Profile_Start(ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c_profile);

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

			goto bail;
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
					for(i=1; i<16; i++)
					{
						acArray[i] = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem][i]) << (qp_per);
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

bail:


	Profile_End(ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c_profile);

	return ippStsNoErr;

}


/******************************************************************************************************
 * Do 8*8 block reconstruction for intra-Chroma
 * Use same edge type for this 8x8 block
 ******************************************************************************************************
 */
IppStatus __STDCALL ippiReconstructChromaIntraMB_H264_16s8u_P2R_c(Ipp16s **ppSrcCoeff,
                                                                Ipp8u *pSrcDstUPlane,
                                                                Ipp8u *pSrcDstVPlane,
                                                                const Ipp32u srcdstUVStep,
                                                                const IppIntraChromaPredMode_H264 intra_chroma_mode,
                                                                const Ipp32u cbp4x4,
                                                                const Ipp32u ChromaQP,
                                                                const Ipp8u edge_type)
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

	int		qp_per_uv, qp_rem_uv;


	unsigned int	cbp = cbp4x4>> 17;			// Use Chroma part ONLY


	int		block_x, block_y;


	Profile_Start(ippiReconstructChromaIntraMB_H264_16s8u_P2R_c_profile);

	qp_per_uv = ChromaQP/6;
	qp_rem_uv = ChromaQP%6;

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

		goto bail;
	}
	// U
	if(cbp & 0x01)
	{
		// 2.1 2x2 DC Transform
		// IPPVC_CBP_1ST_CHROMA_DC_BITPOS
		// 这里需要进一步处理QP<6的情况
		if(ChromaQP >= 6)
		{
			for (i=0;i<4;i++)
				dcArrayU[i] = ((*ppSrcCoeff)[i]) *dequant_coef[qp_rem_uv][0]<< (qp_per_uv - 1);
		}
		else
		{
			for (i=0;i<4;i++)
				dcArrayU[i] = ((*ppSrcCoeff)[i]) *dequant_coef[qp_rem_uv][0]>> 1;
		}

		dcResultU[0] = (dcArrayU[0] + dcArrayU[1] + dcArrayU[2] + dcArrayU[3]);
		dcResultU[1] = (dcArrayU[0] - dcArrayU[1] + dcArrayU[2] - dcArrayU[3]);
		dcResultU[2] = (dcArrayU[0] + dcArrayU[1] - dcArrayU[2] - dcArrayU[3]);
		dcResultU[3] = (dcArrayU[0] - dcArrayU[1] - dcArrayU[2] + dcArrayU[3]);


		*ppSrcCoeff += 4;

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
		if(ChromaQP >= 6)
		{
			for (i=0;i<4;i++)
				dcArrayV[i] = ((*ppSrcCoeff)[i]) *dequant_coef[qp_rem_uv][0]<< (qp_per_uv - 1);
		}
		else
		{
			for (i=0;i<4;i++)
				dcArrayV[i] = ((*ppSrcCoeff)[i]) *dequant_coef[qp_rem_uv][0] >>  1;
		}

		dcResultV[0] = (dcArrayV[0] + dcArrayV[1] + dcArrayV[2] + dcArrayV[3]);
		dcResultV[1] = (dcArrayV[0] - dcArrayV[1] + dcArrayV[2] - dcArrayV[3]);
		dcResultV[2] = (dcArrayV[0] + dcArrayV[1] - dcArrayV[2] - dcArrayV[3]);
		dcResultV[3] = (dcArrayV[0] - dcArrayV[1] - dcArrayV[2] + dcArrayV[3]);

		*ppSrcCoeff += 4;


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
				acArray[i] = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem_uv][i]) << (qp_per_uv);
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

			*ppSrcCoeff += 16;

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
				acArray[i] = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem_uv][i]) << (qp_per_uv);
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

			*ppSrcCoeff += 16;

		}

	}
	// Third, we do add operation and clip operation for finnal result!!!!!!!!!!!!!!!!!!!!!!!!!

bail:


	Profile_End(ippiReconstructChromaIntraMB_H264_16s8u_P2R_c_profile);

	return ippStsNoErr;
}


IppStatus __STDCALL ippiReconstructLumaInterMB_H264_16s8u_C1R_c(Ipp16s **ppSrcCoeff,
                                                              Ipp8u *pSrcDstYPlane,
                                                              Ipp32u srcdstYStep,
                                                              Ipp32u cbp4x4,
                                                              Ipp32s QP)
{
 

	Profile_Start(ippiReconstructLumaInterMB_H264_16s8u_C1R_c_profile);



#if 0
	IppStatus status = ippiReconstructLumaInterMB_H264_16s8u_C1R(	   ppSrcCoeff,
                                                   pSrcDstYPlane,
                                                   srcdstYStep,
                                                   cbp4x4,
                                                   QP);


	Profile_End(ippiReconstructLumaInterMB_H264_16s8u_C1R_c_profile);


	return status;
#endif



	_DECLSPEC int		acArray[16];
	_DECLSPEC int		m7[4][4], m5[4], m6[4];

	int		i,	j, k;
	Ipp8u	*pSrcforPrediction;

	int		pitch = srcdstYStep;

	int		qp_per, qp_rem;

	unsigned int  cbp = cbp4x4 & 0x1ffff;		// Need 17 bits to represent Luma information 

	int		block_x, block_y;

	qp_per = QP/6;
	qp_rem = QP%6;

	for(k=0; k<16; k++)			// In scan order
	{	

		if( !(cbp & (1<< (1+k))))
		//if(!(cbp & cbpmask[k]))
		{

			continue;

		}
		else
		{
			block_x = ac_block_idx_x[k];
			block_y = ac_block_idx_y[k];

			pSrcforPrediction = pSrcDstYPlane + block_y*srcdstYStep + block_x;			// Pointer to top_left cornor of one 4x4 block

			// Form acArray and do IQ
			for(i=0; i<16; i++)
			{
				acArray[i] = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem][i]) << (qp_per);
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
				m5[0] = m7[0][i];
				m5[1] = m7[1][i];
				m5[2] = m7[2][i];
				m5[3] = m7[3][i];

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
			*ppSrcCoeff += 16;

		}

	}

	Profile_End(ippiReconstructLumaInterMB_H264_16s8u_C1R_c_profile);

	return ippStsNoErr;
}



IppStatus __STDCALL ippiReconstructLumaIntraMB_H264_16s8u_C1R_c(Ipp16s **ppSrcCoeff,
                                                              Ipp8u *pSrcDstYPlane,
                                                              Ipp32s srcdstYStep,
                                                              const IppIntra4x4PredMode_H264 *pMBIntraTypes,
                                                              const Ipp32u cbp4x4,
                                                              const Ipp32u QP,
                                                              const Ipp8u edge_type)
{

 
	Profile_Start(ippiReconstructLumaIntraMB_H264_16s8u_C1R_c_profile);


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

	unsigned int  cbp = cbp4x4 & 0x1ffff;		// Need 17 bits to represent Luma information 
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
					acArray[i] = ((*ppSrcCoeff)[i] * dequant_coef[qp_rem][i]) << (qp_per);
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

			*ppSrcCoeff += 16;
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
 
	Profile_End(ippiReconstructLumaIntraMB_H264_16s8u_C1R_c_profile);

	return ippStsNoErr;
}


#else

static void ( *dquant_trans_recon_dc_universal)	( Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch )=NULL;//***
static void ( *dquant_trans_recon_universal)	( Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch )=NULL;//***

static void ( *dquant_trans_recon_dc_lossy_universal)	( Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch )=NULL;//***
static void ( *dquant_trans_recon_lossy_universal)	( Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch )=NULL;//***

static void ( *dquant_trans_recon_dc_universal_2)	( Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch )=NULL;//***
static void ( *dquant_trans_recon_universal_2)	( Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch )=NULL;//***

static void ( *pred_intra16x16_plane_universal) ( int icc, int ib, int ic, Ipp8u *dst, int pitch )=NULL;//***

#ifdef __cplusplus
extern "C" {
#endif

void dquant_trans_recon_arm(  Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch );
void dquant_trans_recon_dc_arm(  Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch );
void dquant_trans_recon_lossy_arm(  Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch );
void dquant_trans_recon_dc_lossy_arm(  Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch );
void pred_intra16x16_plane_arm(int icc, int ib, int ic, Ipp8u *dst, int pitch);

#ifdef __cplusplus
}
#endif

#define dquant_trans_recon_dc_x					(*dquant_trans_recon_dc_universal)
#define dquant_trans_recon_x					(*dquant_trans_recon_universal)

#define dquant_trans_recon_dc_x_2				(*dquant_trans_recon_dc_universal_2)
#define dquant_trans_recon_x_2					(*dquant_trans_recon_universal_2)

#define pred_intra16x16_plane_x					(*pred_intra16x16_plane_universal)

#ifndef DROP_C_NO

static void dquant_trans_recon_dc_c(  Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch )
{
	Ipp8u *pred = ((Ipp8u *)m) - 16;

	{
		int	t0, t1;	
		int	p0, p1, p2, p3;	
		
		t0 = m[ 0 ];

		qp_per &= 0x00ff;

		t1 = (coef[ 2] * dquant[ 2]) << (qp_per);
		p0 = t0 + t1;
		p1 = t0 - t1;

		t0 = (coef[ 1] * dquant[ 1]) << (qp_per);
		t1 = (coef[ 3] * dquant[ 3]) << (qp_per);
		p2 = (t0>>1) - t1;
		p3 = t0      + (t1>>1);

		m[0]  = p0 + p3;
		m[3]  = p0 - p3;
		m[1]  = p1 + p2;
		m[2]  = p1 - p2;

		//
		//coef[4] = coef[5] = coef[6] = coef[7] = 0;
		
		t0 = (coef[ 4] * dquant[ 4]) << (qp_per);
		t1 = (coef[ 6] * dquant[ 6]) << (qp_per);
		p0 = t0 + t1;
		p1 = t0 - t1;

		t0 = (coef[ 5] * dquant[ 5]) << (qp_per);
		t1 = (coef[ 7] * dquant[ 7]) << (qp_per);
		p2 = (t0>>1) - t1;
		p3 =  t0     + (t1>>1);

		m[4]  = p0 + p3;
		m[7]  = p0 - p3;
		m[5]  = p1 + p2;
		m[6]  = p1 - p2;

		//
		//coef[8] = coef[9] = coef[10] = coef[11] = 0;

		t0 = (coef[ 8] * dquant[ 8]) << (qp_per);
		t1 = (coef[10] * dquant[10]) << (qp_per);
		p0 = t0  + t1;
		p1 = t0  - t1;

		t0 = (coef[ 9] * dquant[ 9]) << (qp_per);
		t1 = (coef[11] * dquant[11]) << (qp_per);
		p2 = (t0>>1) - t1;
		p3 = t0      + (t1>>1);

		m[8]  = p0 + p3;
		m[11] = p0 - p3;
		m[9]  = p1 + p2;
		m[10] = p1 - p2;

		//
		//***
		//coef[12] = coef[13] = coef[14] = coef[15] = 0;

		t0 = (coef[12] * dquant[12]) << (qp_per);
		t1 = (coef[14] * dquant[14]) << (qp_per);
		p0 = (t0     + t1);
		p1 = (t0     - t1);

		t0 = (coef[13] * dquant[13]) << (qp_per);
		t1 = (coef[15] * dquant[15]) << (qp_per);
		p2 = (t0>>1) - t1;
		p3 = t0      + (t1>>1);

		m[12]  = p0 + p3;
		m[15]  = p0 - p3;
		m[13]  = p1 + p2;
		m[14]  = p1 - p2;
	}


	//***
	if(0)
	{
		int i;
		fprintf( stderr, "coef:");					
		for( i = 0; i < 16; i++ )
			fprintf( stderr, "%4x,", coef[i]);					
		fprintf( stderr, "\n");

		fprintf( stderr, "dqua:");					
		for( i = 0; i < 16; i++ )
			fprintf( stderr, "%4x,", dquant[i]);					
		fprintf( stderr, "\n");

		fprintf( stderr, "pred:");					
		for( i = 0; i < 16; i++ )
			fprintf( stderr, "%4x,", pred[i]);					
		fprintf( stderr, "\n");
	}

	//***
	{
		int	t0, t1;	
		int	p0, p1, p2, p3;	
		int const_255 = 255;

		t0 = m[0];
		t1 = m[8];
		p0 = (t0+t1);
		p1 = (t0-t1);

		t0 = m[4];
		t1 = m[12];
		p2 =(t0>>1)-t1;
		p3 = t0+(t1>>1);

		t0 = ((pred[ 0]  << DQ_BITS) + p0+p3 + DQ_ROUND ) >> DQ_BITS;	Clip1(const_255, t0); m[ 0] = t0;
		t0 = ((pred[12]  << DQ_BITS) + p0-p3 + DQ_ROUND ) >> DQ_BITS;	Clip1(const_255, t0); m[12] = t0;
		t0 = ((pred[ 4]  << DQ_BITS) + p1+p2 + DQ_ROUND ) >> DQ_BITS;	Clip1(const_255, t0); m[ 4] = t0;
		t0 = ((pred[ 8]  << DQ_BITS) + p1-p2 + DQ_ROUND ) >> DQ_BITS;	Clip1(const_255, t0); m[ 8] = t0;

		//
		t0 = m[1];
		t1 = m[9];
		p0 = (t0+t1);
		p1 = (t0-t1);

		t0 = m[5];
		t1 = m[13];
		p2 = (t0>>1)-t1;
		p3 = t0+(t1>>1);

		t0 = ((pred[ 1]  << DQ_BITS) + p0+p3 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0); m[ 1] = t0;
		t0 = ((pred[13]  << DQ_BITS) + p0-p3 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0); m[13] = t0;
		t0 = ((pred[ 5]  << DQ_BITS) + p1+p2 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0); m[ 5] = t0;
		t0 = ((pred[ 9]  << DQ_BITS) + p1-p2 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0); m[ 9] = t0;

		//
		t0 = m[2];
		t1 = m[10];
		p0 = (t0+t1);
		p1 = (t0-t1);

		t0 = m[6];
		t1 = m[14];
		p2 = (t0>>1)-t1;
		p3 = t0+(t1>>1);

		t0 = ((pred[ 2]  << DQ_BITS) + p0+p3 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[ 2] = t0;
		t0 = ((pred[14]  << DQ_BITS) + p0-p3 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[14] = t0;
		t0 = ((pred[ 6]  << DQ_BITS) + p1+p2 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[ 6] = t0;
		t0 = ((pred[10]  << DQ_BITS) + p1-p2 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[10] = t0;

		//
		t0 = m[3];
		t1 = m[11];
		p0 = (t0+t1);
		p1 = (t0-t1);

		t0 = m[7];
		t1 = m[15];
		p2 = (t0>>1)-t1;
		p3 = t0+(t1>>1);

		t0 = ((pred[ 3]  << DQ_BITS) + p0+p3 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[ 3] = t0; 
		t0 = ((pred[15]  << DQ_BITS) + p0-p3 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[15] = t0; 
		t0 = ((pred[ 7]  << DQ_BITS) + p1+p2 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[ 7] = t0; 
		t0 = ((pred[11]  << DQ_BITS) + p1-p2 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[11] = t0; 
	}

	{
		int	t0;

		t0 = (m[3]<<24)|(m[2]<<16)|(m[1]<<8)|(m[0]<<0);
		*((int *)(dst)) = t0;		dst += pitch;

		t0 = (m[7]<<24)|(m[6]<<16)|(m[5]<<8)|(m[4]<<0);
		*((int *)(dst)) = t0;		dst += pitch;

		t0 = (m[11]<<24)|(m[10]<<16)|(m[9]<<8)|(m[8]<<0);
		*((int *)(dst)) = t0;		dst += pitch;

		t0 = (m[15]<<24)|(m[14]<<16)|(m[13]<<8)|(m[12]<<0);
		*((int *)(dst)) = t0;	
	}
}



static void dquant_trans_recon_dc_lossy_c(  Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch )
{
	Ipp8u *pred = ((Ipp8u *)m) - 16;

	{
		int	t0, t1;	
		int	p0, p1, p2, p3;	
		
		t0 = m[ 0 ];

		qp_per &= 0x00ff;

		t1 = (coef[ 2] * dquant[ 2]) << (qp_per);
		p0 = t0 + t1;
		p1 = t0 - t1;

		t0 = (coef[ 1] * dquant[ 1]) << (qp_per);
		t1 = (coef[ 3] * dquant[ 3]) << (qp_per);
		p2 = (t0>>1) - t1;
		p3 = t0      + (t1>>1);

		m[0]  = p0 + p3;
		m[3]  = p0 - p3;
		m[1]  = p1 + p2;
		m[2]  = p1 - p2;

		//
		//coef[4] = coef[5] = coef[6] = coef[7] = 0;
		
		t0 = (coef[ 4] * dquant[ 4]) << (qp_per);
		t1 = (coef[ 6] * dquant[ 6]) << (qp_per);
		p0 = t0 + t1;
		p1 = t0 - t1;

		t0 = (coef[ 5] * dquant[ 5]) << (qp_per);
		t1 = (coef[ 7] * dquant[ 7]) << (qp_per);
		p2 = (t0>>1) - t1;
		p3 =  t0     + (t1>>1);

		m[4]  = p0 + p3;
		m[7]  = p0 - p3;
		m[5]  = p1 + p2;
		m[6]  = p1 - p2;

		//
		m[8]  = 0;
		m[9]  = 0;
		m[10] = 0;
		m[11] = 0;

		//
		//***

		m[12]  = 0;
		m[13]  = 0;
		m[14]  = 0;
		m[15]  = 0;
	}

	//***
	{
		int	t0, t1;	
		int	p0, p1, p2, p3;	
		int const_255 = 255;

		t0 = m[0];
		t1 = m[8];
		p0 = (t0+t1);
		p1 = (t0-t1);

		t0 = m[4];
		t1 = m[12];
		p2 =(t0>>1)-t1;
		p3 = t0+(t1>>1);

		t0 = ((pred[ 0]  << DQ_BITS) + p0+p3 + DQ_ROUND ) >> DQ_BITS;	Clip1(const_255, t0); m[ 0] = t0;
		t0 = ((pred[12]  << DQ_BITS) + p0-p3 + DQ_ROUND ) >> DQ_BITS;	Clip1(const_255, t0); m[12] = t0;
		t0 = ((pred[ 4]  << DQ_BITS) + p1+p2 + DQ_ROUND ) >> DQ_BITS;	Clip1(const_255, t0); m[ 4] = t0;
		t0 = ((pred[ 8]  << DQ_BITS) + p1-p2 + DQ_ROUND ) >> DQ_BITS;	Clip1(const_255, t0); m[ 8] = t0;

		//
		t0 = m[1];
		t1 = m[9];
		p0 = (t0+t1);
		p1 = (t0-t1);

		t0 = m[5];
		t1 = m[13];
		p2 = (t0>>1)-t1;
		p3 = t0+(t1>>1);

		t0 = ((pred[ 1]  << DQ_BITS) + p0+p3 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0); m[ 1] = t0;
		t0 = ((pred[13]  << DQ_BITS) + p0-p3 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0); m[13] = t0;
		t0 = ((pred[ 5]  << DQ_BITS) + p1+p2 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0); m[ 5] = t0;
		t0 = ((pred[ 9]  << DQ_BITS) + p1-p2 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0); m[ 9] = t0;

		//
		t0 = m[2];
		t1 = m[10];
		p0 = (t0+t1);
		p1 = (t0-t1);

		t0 = m[6];
		t1 = m[14];
		p2 = (t0>>1)-t1;
		p3 = t0+(t1>>1);

		t0 = ((pred[ 2]  << DQ_BITS) + p0+p3 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[ 2] = t0;
		t0 = ((pred[14]  << DQ_BITS) + p0-p3 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[14] = t0;
		t0 = ((pred[ 6]  << DQ_BITS) + p1+p2 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[ 6] = t0;
		t0 = ((pred[10]  << DQ_BITS) + p1-p2 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[10] = t0;

		//
		t0 = m[3];
		t1 = m[11];
		p0 = (t0+t1);
		p1 = (t0-t1);

		t0 = m[7];
		t1 = m[15];
		p2 = (t0>>1)-t1;
		p3 = t0+(t1>>1);

		t0 = ((pred[ 3]  << DQ_BITS) + p0+p3 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[ 3] = t0; 
		t0 = ((pred[15]  << DQ_BITS) + p0-p3 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[15] = t0; 
		t0 = ((pred[ 7]  << DQ_BITS) + p1+p2 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[ 7] = t0; 
		t0 = ((pred[11]  << DQ_BITS) + p1-p2 + DQ_ROUND ) >> DQ_BITS; Clip1(const_255, t0);	m[11] = t0; 
	}

	{
		int	t0;

		t0 = (m[3]<<24)|(m[2]<<16)|(m[1]<<8)|(m[0]<<0);
		*((int *)(dst)) = t0;		dst += pitch;

		t0 = (m[7]<<24)|(m[6]<<16)|(m[5]<<8)|(m[4]<<0);
		*((int *)(dst)) = t0;		dst += pitch;

		t0 = (m[11]<<24)|(m[10]<<16)|(m[9]<<8)|(m[8]<<0);
		*((int *)(dst)) = t0;		dst += pitch;

		t0 = (m[15]<<24)|(m[14]<<16)|(m[13]<<8)|(m[12]<<0);
		*((int *)(dst)) = t0;	
	}
}




static void dquant_trans_recon_c(  Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch )
{
	int t0;

	t0 = (coef[ 0] * dquant[ 0]) << (qp_per);
	m[ 0 ] = t0;
		
	dquant_trans_recon_dc_c( coef, dquant, qp_per, m, dst, pitch );
}


static void dquant_trans_recon_lossy_c(  Ipp16s *coef, Ipp16u *dquant, int qp_per, Ipp16s *m, Ipp8u *dst, int pitch )
{
	int t0;

	t0 = (coef[ 0] * dquant[ 0]) << (qp_per);
	m[ 0 ] = t0;
		
	dquant_trans_recon_dc_lossy_c( coef, dquant, qp_per, m, dst, pitch );
}


static void pred_intra16x16_plane_c(int icc, int ib, int ic, Ipp8u *dst, int pitch)
{
	int i;
	int ibb0 = -7 *ib;
	int const_255 = 255;

	for(i=16;i--;)
	{
		int ibb = ibb0;
		int tmp;

		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 0 ] = tmp; 
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 1 ] = tmp; 
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 2 ] = tmp; 
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 3 ] = tmp; 
																				
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 4 ] = tmp; 
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 5 ] = tmp; 
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 6 ] = tmp; 
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 7 ] = tmp; 
																				
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 8 ] = tmp; 
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 9 ] = tmp; 
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 10] = tmp; 
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 11] = tmp; 
																				
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 12] = tmp; 
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 13] = tmp; 
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 14] = tmp; 
		tmp = ( ibb + icc )>>5;	ibb += ib; Clip1(const_255, tmp); dst[ 15] = tmp; 

		dst += pitch;
		icc += ic;
	}
}

#endif

//#define USE_INTEL_IPP_RECON
//***not thread safe  bnie  2/19/2008

void Init_AVC_Reconstruction_C()
{
	//common
	ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_universal	= ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c;
	ippiReconstructLumaIntraMB_H264_16s8u_C1R_universal			= ippiReconstructLumaIntraMB_H264_16s8u_C1R_c;
	ippiReconstructLumaInterMB_H264_16s8u_C1R_universal			= ippiReconstructLumaInterMB_H264_16s8u_C1R_c;
	ippiReconstructChromaIntraMB_H264_16s8u_P2R_universal		= ippiReconstructChromaIntraMB_H264_16s8u_P2R_c;
	ippiReconstructChromaInterMB_H264_16s8u_P2R_universal		= ippiReconstructChromaInterMB_H264_16s8u_P2R_c;

#ifndef DROP_MBAFF
	ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_universal	= ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_c;
	ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_universal	 	= ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_c;
#endif

	//can be repalced by optimized versions
#ifndef DROP_C_NO
	dquant_trans_recon_universal			= dquant_trans_recon_c;
	dquant_trans_recon_dc_universal			= dquant_trans_recon_dc_c;

	dquant_trans_recon_lossy_universal		= dquant_trans_recon_lossy_c;
	dquant_trans_recon_dc_lossy_universal	= dquant_trans_recon_dc_lossy_c;

	dquant_trans_recon_universal_2			= dquant_trans_recon_universal;
	dquant_trans_recon_dc_universal_2		= dquant_trans_recon_dc_universal;

	pred_intra16x16_plane_universal			= pred_intra16x16_plane_c;
#endif

}


void Init_AVC_Reconstruction_ARM_V5()
{
#if defined(__KINOMA_IPP_ARM_V5__) && TARGET_CPU_ARM
	dquant_trans_recon_universal          = dquant_trans_recon_arm;
	dquant_trans_recon_dc_universal		  = dquant_trans_recon_dc_arm;
	
	dquant_trans_recon_lossy_universal    = dquant_trans_recon_lossy_arm;
	dquant_trans_recon_dc_lossy_universal = dquant_trans_recon_dc_lossy_arm;
	
	dquant_trans_recon_universal_2		  = dquant_trans_recon_universal;
	dquant_trans_recon_dc_universal_2	  = dquant_trans_recon_dc_universal;
	
	pred_intra16x16_plane_universal		  = pred_intra16x16_plane_arm;
#endif
}


void AVC_Reconstruction_SetApprox(int approx_level)
{
	if( (approx_level & AVC_RECONSTRUCTION_SPEED)  == AVC_RECONSTRUCTION_SPEED )
	{
		dquant_trans_recon_universal_2    = dquant_trans_recon_lossy_universal;
		dquant_trans_recon_dc_universal_2 = dquant_trans_recon_dc_lossy_universal;
	}
	else
	{
		dquant_trans_recon_universal_2    = dquant_trans_recon_universal;
		dquant_trans_recon_dc_universal_2 = dquant_trans_recon_dc_universal;
	}
}


IppStatus __STDCALL ippiReconstructChromaInterMB_H264_16s8u_P2R_c(Ipp16s **ppSrcCoeff,
                                                                Ipp8u *pSrcDstUPlane,
                                                                Ipp8u *pSrcDstVPlane,
                                                                const Ipp32u srcdstUVStep,
                                                                const Ipp32u cbp4x4,
                                                                const Ipp32u ChromaQP)
{


	Profile_Start(ippiReconstructChromaInterMB_H264_16s8u_P2R_c_profile);


#ifdef USE_INTEL_IPP_RECON
	IppStatus status;
	status = ippiReconstructChromaInterMB_H264_16s8u_P2R
			(ppSrcCoeff,pSrcDstUPlane,pSrcDstVPlane,srcdstUVStep,cbp4x4,ChromaQP);

	Profile_End(ippiReconstructChromaInterMB_H264_16s8u_P2R_c_profile);

	return status;
#endif

	int		dcArrayU[4], dcResultU[4];
	int		dcArrayV[4], dcResultV[4];
	int		k;
	unsigned int	cbp = cbp4x4>> 17;			// Use Chroma part ONLY
	int		block_x, block_y;

	int		pitch = srcdstUVStep;
	int		qp_per = ChromaQP/6;
	int		qp_rem = ChromaQP%6;
	Ipp16u	*dquant = &(dequant_coef[qp_rem][0]);
	Ipp16u	dquant0 = dquant[0];
	__ALIGN8(Ipp8u, pred, 16+2*16);
	Ipp16s	*m0 = (Ipp16s *)&pred[16];

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

		if(ChromaQP >= 6)
		{
			int tmp = dquant0<< (qp_per - 1);
			dcArrayU[0] = (*ppSrcCoeff)[0] * tmp;
			dcArrayU[1] = (*ppSrcCoeff)[1] * tmp;
			dcArrayU[2] = (*ppSrcCoeff)[2] * tmp;
			dcArrayU[3] = (*ppSrcCoeff)[3] * tmp;
		}
		else
		{
			int tmp = dquant0 >> 1;
			dcArrayU[0] = (*ppSrcCoeff)[0] * tmp;
			dcArrayU[1] = (*ppSrcCoeff)[1] * tmp;
			dcArrayU[2] = (*ppSrcCoeff)[2] * tmp;
			dcArrayU[3] = (*ppSrcCoeff)[3] * tmp;
		}

		dcResultU[0] = dcArrayU[0] + dcArrayU[1] + dcArrayU[2] + dcArrayU[3];
		dcResultU[1] = dcArrayU[0] - dcArrayU[1] + dcArrayU[2] - dcArrayU[3];
		dcResultU[2] = dcArrayU[0] + dcArrayU[1] - dcArrayU[2] - dcArrayU[3];
		dcResultU[3] = dcArrayU[0] - dcArrayU[1] - dcArrayU[2] + dcArrayU[3];

		*ppSrcCoeff += 4;
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
		if(ChromaQP >= 6)
		{
			int tmp = dquant0<< (qp_per - 1);
			dcArrayV[0] = (*ppSrcCoeff)[0] * tmp;
			dcArrayV[1] = (*ppSrcCoeff)[1] * tmp;
			dcArrayV[2] = (*ppSrcCoeff)[2] * tmp;
			dcArrayV[3] = (*ppSrcCoeff)[3] * tmp;
		}
		else
		{
			int tmp = dquant0 >> 1;
			dcArrayV[0] = (*ppSrcCoeff)[0] * tmp;
			dcArrayV[1] = (*ppSrcCoeff)[1] * tmp;
			dcArrayV[2] = (*ppSrcCoeff)[2] * tmp;
			dcArrayV[3] = (*ppSrcCoeff)[3] * tmp;
		}

		dcResultV[0] = dcArrayV[0] + dcArrayV[1] + dcArrayV[2] + dcArrayV[3];
		dcResultV[1] = dcArrayV[0] - dcArrayV[1] + dcArrayV[2] - dcArrayV[3];
		dcResultV[2] = dcArrayV[0] + dcArrayV[1] - dcArrayV[2] - dcArrayV[3];
		dcResultV[3] = dcArrayV[0] - dcArrayV[1] - dcArrayV[2] + dcArrayV[3];

		*ppSrcCoeff += 4;
	}
	else
	{
		dcResultV[0] = 0;
		dcResultV[1] = 0;
		dcResultV[2] = 0;
		dcResultV[3] = 0;
	}

	// U -- AC	
	int const_255 = 255;
	for(k=0; k<4; k++)									// CBP decision
	{
		block_x = block_idx[k<<1];
		block_y = block_idx[(k<<1) + 1];

		if(!(cbp&(1<<(2+k))))
		{// This block has no AC information : but we have DC information
			int		dc = ((dcResultU[k]+32)>>6);			
			Ipp8u	*s =  pSrcDstUPlane + block_y *srcdstUVStep + block_x;
			int		tmp;

			tmp = s[0] + dc; Clip1( const_255, tmp ); s[0] = tmp;
			tmp = s[1] + dc; Clip1( const_255, tmp ); s[1] = tmp;
			tmp = s[2] + dc; Clip1( const_255, tmp ); s[2] = tmp;
			tmp = s[3] + dc; Clip1( const_255, tmp ); s[3] = tmp;

			s += srcdstUVStep;	
			tmp = s[0] + dc; Clip1( const_255, tmp ); s[0] = tmp;
			tmp = s[1] + dc; Clip1( const_255, tmp ); s[1] = tmp;
			tmp = s[2] + dc; Clip1( const_255, tmp ); s[2] = tmp;
			tmp = s[3] + dc; Clip1( const_255, tmp ); s[3] = tmp;
			
			s += srcdstUVStep;	
			tmp = s[0] + dc; Clip1( const_255, tmp ); s[0] = tmp;
			tmp = s[1] + dc; Clip1( const_255, tmp ); s[1] = tmp;
			tmp = s[2] + dc; Clip1( const_255, tmp ); s[2] = tmp;
			tmp = s[3] + dc; Clip1( const_255, tmp ); s[3] = tmp;
			
			s += srcdstUVStep;	
			tmp = s[0] + dc; Clip1( const_255, tmp ); s[0] = tmp;
			tmp = s[1] + dc; Clip1( const_255, tmp ); s[1] = tmp;
			tmp = s[2] + dc; Clip1( const_255, tmp ); s[2] = tmp;
			tmp = s[3] + dc; Clip1( const_255, tmp ); s[3] = tmp;
		}
		else
		{
			Ipp8u  *dst		= pSrcDstUPlane + block_y*pitch + block_x;
			Ipp8u  *p		= pSrcDstUPlane + block_y*pitch + block_x;
			Ipp16s *coef	=  *ppSrcCoeff;
			
			m0[0] = dcResultU[k];
			*((int *)(pred+0))	 = *((int *)(p+0  ));
			*((int *)(pred+4))  = *((int *)(p+pitch  ));
			*((int *)(pred+8))  = *((int *)(p+pitch*2));
			*((int *)(pred+12)) = *((int *)(p+pitch*3));

			dquant_trans_recon_dc_x_2(  coef, dquant, qp_per, m0, dst, pitch );
			*ppSrcCoeff += 16;
		}
	}
	
	// V -- AC
	for(k=0; k<4; k++)									// CBP decision
	{
		block_x = block_idx[k<<1];
		block_y = block_idx[(k<<1) + 1];

		// This statement is different for U and V
		if(!(cbp&(1<<(6+k))))
		{// This block has no AC information : but we have DC information
			int		dc = ((dcResultV[k]+32)>>6);			
			Ipp8u	*s =  pSrcDstVPlane + block_y *srcdstUVStep + block_x;
			int		tmp;

			tmp = s[0] + dc; Clip1( const_255, tmp ); s[0] = tmp;
			tmp = s[1] + dc; Clip1( const_255, tmp ); s[1] = tmp;
			tmp = s[2] + dc; Clip1( const_255, tmp ); s[2] = tmp;
			tmp = s[3] + dc; Clip1( const_255, tmp ); s[3] = tmp;

			s += srcdstUVStep;	
			tmp = s[0] + dc; Clip1( const_255, tmp ); s[0] = tmp;
			tmp = s[1] + dc; Clip1( const_255, tmp ); s[1] = tmp;
			tmp = s[2] + dc; Clip1( const_255, tmp ); s[2] = tmp;
			tmp = s[3] + dc; Clip1( const_255, tmp ); s[3] = tmp;
			
			s += srcdstUVStep;	
			tmp = s[0] + dc; Clip1( const_255, tmp ); s[0] = tmp;
			tmp = s[1] + dc; Clip1( const_255, tmp ); s[1] = tmp;
			tmp = s[2] + dc; Clip1( const_255, tmp ); s[2] = tmp;
			tmp = s[3] + dc; Clip1( const_255, tmp ); s[3] = tmp;
			
			s += srcdstUVStep;	
			tmp = s[0] + dc; Clip1( const_255, tmp ); s[0] = tmp;
			tmp = s[1] + dc; Clip1( const_255, tmp ); s[1] = tmp;
			tmp = s[2] + dc; Clip1( const_255, tmp ); s[2] = tmp;
			tmp = s[3] + dc; Clip1( const_255, tmp ); s[3] = tmp;
		}
		else
		{
			Ipp8u  *dst		= pSrcDstVPlane + block_y*pitch + block_x;
			Ipp8u  *p		= pSrcDstVPlane + block_y*pitch + block_x;
			Ipp16s *coef	=  *ppSrcCoeff;
			
			m0[0] = dcResultV[k];
			*((int *)(pred+0))	 = *((int *)(p+0  ));
			*((int *)(pred+4))  = *((int *)(p+pitch  ));
			*((int *)(pred+8))  = *((int *)(p+pitch*2));
			*((int *)(pred+12)) = *((int *)(p+pitch*3));

			dquant_trans_recon_dc_x_2(  coef, dquant, qp_per, m0, dst, pitch );

			*ppSrcCoeff += 16;
		}
	}


	Profile_End(ippiReconstructChromaInterMB_H264_16s8u_P2R_c_profile);


	return ippStsNoErr;

}


IppStatus __STDCALL ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c(Ipp16s **ppSrcCoeff,
                                                                   Ipp8u *pSrcDstYPlane,
                                                                   Ipp32u srcdstYStep,
                                                                   const IppIntra16x16PredMode_H264 intra_luma_mode,
                                                                   const Ipp32u cbp4x4,
                                                                   const Ipp32u QP,
                                                                   const Ipp8u edge_type)
{

	Profile_Start(ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c_profile);


#ifdef USE_INTEL_IPP_RECON
	IppStatus status;
	status = ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R
			(ppSrcCoeff,pSrcDstYPlane,srcdstYStep,intra_luma_mode,cbp4x4,QP,edge_type);

	Profile_End(ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c_profile);

	return status;
#endif

	// ppSrcCoeff add 16 only when the "coresponding cbp bit" equal 1
	//  cbp4x4 : 0 specify Luma DC
	//           1--16 specify Luma AC block coeffs
	int		i,	j, k;
	int		iv, ih, ib, ic, iaa;	// Used for Plane prediction
	unsigned int  cbp = cbp4x4 & 0x1ffff;		// Need 17 bits to represent Luma information 
	int		pitch = srcdstYStep;

	// First, we do intra-prediction according to edge_type and intra_luma_mode!!!!!!!!!!!!!!!!!!!!

	if(cbp == 0)
	{
		//***
		//count1++;
		// IPPVC_CBP_LUMA_DC
		switch (intra_luma_mode)
		{
			case 0:														// IPP_16X16_VERT
				{
					Ipp8u *s = pSrcDstYPlane - pitch;
					Ipp8u *d = pSrcDstYPlane;
					
					int t0 = *((int *)(s + 0 ));
					int t1 = *((int *)(s + 4 ));
					int t2 = *((int *)(s + 8 ));
					int t3 = *((int *)(s + 12));
					
								*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch; *(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
					d += pitch;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
				}
				break;
			case 1:														// IPP_16X16_HOR
				{
					Ipp8u *s = pSrcDstYPlane - 1;
					int	  t;

								t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
					s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(s+1)=t;*(int *)(s+5)=t;*(int *)(s+9)=t;*(int *)(s+13)=t;
				}
				break;

			case 2:	
				{
					// IPP_16X16_DC
					// Get DC value
					Ipp8u *s;
					int s0;
					int s1=0;
					int s2=0;

					if(HAS_AA)
					{
						Ipp8u *s = pSrcDstYPlane - pitch;		// UP
						
						s1 = s[0]+s[1]+s[2]+s[3]+s[4]+s[5]+s[6]+s[7]+s[8]+s[9]+s[10]+s[11]+s[12]+s[13]+s[14]+s[15];
					}

					if(HAS_BB)
					{
						s = pSrcDstYPlane - 1;					// LEFT
						
									 s2 += *s;
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
						s  += pitch; s2 += *s; 
					}

					if ( HAS_AA &&  HAS_BB)	s0=(s1+s2+16)>>5;		// no edge
					if (!HAS_AA &&  HAS_BB)	s0=(s2+8)>>4;			// upper edge -- use left
					if ( HAS_AA && !HAS_BB)	s0=(s1+8)>>4;			// left edge  -- use up
					if (!HAS_AA && !HAS_BB)	s0=128;					// top left corner, nothing to predict from

					// make prediction
					int t = s0;

					s = pSrcDstYPlane;
					t =(t<<24)|(t<<16)|(t<<8)|(t<<0);
								*(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
					s += pitch; *(int *)(s+0)=t;*(int *)(s+4)=t;*(int *)(s+8)=t;*(int *)(s+12)=t;
				}
				break;

			case 3:														// IPP_16X16_PLANE
				//if (!hasA || !hasD  || !hasB)
				//{
				//	fprintf(stdout, "%s\n", "I got error intra prediction mode in 16x16 mode, please check bitstream!");
				//	return ippStsErr;
				//}
				{
					Ipp8u *s, *ss;
					int	  s1;

					s = pSrcDstYPlane - pitch - 1;
					ih = s[9]-s[7] + (s[10]-s[6])*2 + (s[11]-s[5])*3 + (s[12]-s[4])*4 + (s[13]-s[3])*5 +
						(s[14] -s[2])*6 + (s[15]-s[1])*7 + (s[16]-s[0])*8;
					s1 = s[16];

					iv = 0;
					s	= pSrcDstYPlane - 1 + 8*pitch;				// Left
					ss	= s - 2*pitch;				// Left

											 iv += 1*( *s - *ss); 
					ss -= pitch; s += pitch; iv += 2*( *s - *ss); 
					ss -= pitch; s += pitch; iv += 3*( *s - *ss); 
					ss -= pitch; s += pitch; iv += 4*( *s - *ss); 
					ss -= pitch; s += pitch; iv += 5*( *s - *ss); 
					ss -= pitch; s += pitch; iv += 6*( *s - *ss); 
					ss -= pitch; s += pitch; iv += 7*( *s - *ss); 
					ss -= pitch; s += pitch; iv += 8*( *s - *ss);

					ib=(5*ih+32)>>6;
					ic=(5*iv+32)>>6;

					iaa=16*(*s + s1 + 1);

					int icc = -7 *ic + iaa;
					pred_intra16x16_plane_x( icc, ib, ic, pSrcDstYPlane, pitch);
					// store plane prediction
				}
				break;

			default:
				// Error!
				fprintf(stdout, "%s\n", "I got error intra prediction mode in 16x16 mode, please check bitstream!");
				return ippStsErr;
		}

		goto bail;
	}

{
	__ALIGN4(Ipp8u, prediction_array, 256);

	switch (intra_luma_mode)
	{
		case 0:	
			{
				Ipp8u *s = pSrcDstYPlane - pitch;
				Ipp8u *d = prediction_array;
				
				int t0 = *((int *)(s + 0 ));
				int t1 = *((int *)(s + 4 ));
				int t2 = *((int *)(s + 8 ));
				int t3 = *((int *)(s + 12));
				
							*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;

				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;

				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;

				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
				d += 16;	*(int *)(d+0)=t0;*(int *)(d+4)=t1;*(int *)(d+8)=t2;*(int *)(d+12)=t3;
			}
			break;
		case 1:														// IPP_16X16_HOR
			{
				Ipp8u *s = pSrcDstYPlane - 1;
				Ipp8u *d = prediction_array;
				int	  t;

								     t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; t = *s;	t =(t<<24)|(t<<16)|(t<<8)|(t<<0);*(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
			}

			break;

		case 2:	
			{
				// IPP_16X16_DC
				// Get DC value
				Ipp8u *s;
				int s0;
				int s1=0;
				int s2=0;

				if(HAS_AA)
				{
					Ipp8u *s = pSrcDstYPlane - pitch;		// UP
					
					s1 = s[0]+s[1]+s[2]+s[3]+s[4]+s[5]+s[6]+s[7]+s[8]+s[9]+s[10]+s[11]+s[12]+s[13]+s[14]+s[15];
				}

				if(HAS_BB)
				{
					s = pSrcDstYPlane - 1;					// LEFT
					
								 s2 += *s;
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
					s  += pitch; s2 += *s; 
				}

				if ( HAS_AA &&  HAS_BB)	s0=(s1+s2+16)>>5;		// no edge
				if (!HAS_AA &&  HAS_BB)	s0=(s2+8)>>4;			// upper edge -- use left
				if ( HAS_AA && !HAS_BB)	s0=(s1+8)>>4;			// left edge  -- use up
				if (!HAS_AA && !HAS_BB)	s0=128;					// top left corner, nothing to predict from

				// make prediction
				int		t = s0;
				Ipp8u  *d = prediction_array;

				s = pSrcDstYPlane;
				t =(t<<24)|(t<<16)|(t<<8)|(t<<0);
									 *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
				d += 16; s += pitch; *(int *)(d+0)=t;*(int *)(d+4)=t;*(int *)(d+8)=t;*(int *)(d+12)=t;
			}
			break;

		case 3:														// IPP_16X16_PLANE
			{
				Ipp8u *s, *ss;
				int	  s1;

				s = pSrcDstYPlane - pitch - 1;
				ih = s[9]-s[7] + (s[10]-s[6])*2 + (s[11]-s[5])*3 + (s[12]-s[4])*4 + (s[13]-s[3])*5 +
					(s[14] -s[2])*6 + (s[15]-s[1])*7 + (s[16]-s[0])*8;
				s1 = s[16];

				iv = 0;
				s	= pSrcDstYPlane - 1 + 8*pitch;				// Left
				ss	= s - 2*pitch;				// Left

										 iv += 1*( *s - *ss); 
				ss -= pitch; s += pitch; iv += 2*( *s - *ss); 
				ss -= pitch; s += pitch; iv += 3*( *s - *ss); 
				ss -= pitch; s += pitch; iv += 4*( *s - *ss); 
				ss -= pitch; s += pitch; iv += 5*( *s - *ss); 
				ss -= pitch; s += pitch; iv += 6*( *s - *ss); 
				ss -= pitch; s += pitch; iv += 7*( *s - *ss); 
				ss -= pitch; s += pitch; iv += 8*( *s - *ss);

				ib=(5*ih+32)>>6;
				ic=(5*iv+32)>>6;

				iaa=16*(*s + s1 + 1);

				Ipp8u  *d = prediction_array;
				int icc  = -7 *ic + iaa;
				
				pred_intra16x16_plane_x( icc, ib, ic, d, 16);


				// store plane prediction
			}
			break;

		default:
			// Error!
			fprintf(stdout, "%s\n", "I got error intra prediction mode in 16x16 mode, please check bitstream!");
			return ippStsErr;
	}
	//***
	//count2++;
	// Second, we do inverse Hadamard transform for DC coeffs
	int			qp_per  = QP/6;
	int			qp_rem  = QP%6;
	Ipp16u		*dquant = &(dequant_coef[qp_rem][0]);
	int			round   = (qp_per)?1:2;
	short		dcArray[4][4];
	__ALIGN4(short, dcResult0, 16);
	short		*dcResult = dcResult0;

	if(cbp & 0x01)
	{
		//***
		//count3++;
		int	M5[4], M6[4];
		Ipp16u dquant_dc = dquant[0];

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
	
		if(QP >= 12)
		{
			//for (j=0;j< 16;j+=4)
			//	for (i=0;i< 4;i++)
			int t0= qp_per-2;
			dcResult[0+0] = (((dcResult[0+0] *dquant_dc) << t0) ) ;
			dcResult[0+1] = (((dcResult[0+1] *dquant_dc) << t0) ) ;
			dcResult[0+2] = (((dcResult[0+2] *dquant_dc) << t0) ) ;
			dcResult[0+3] = (((dcResult[0+3] *dquant_dc) << t0) ) ;

			dcResult[4+0] = (((dcResult[4+0] *dquant_dc) << t0) ) ;
			dcResult[4+1] = (((dcResult[4+1] *dquant_dc) << t0) ) ;
			dcResult[4+2] = (((dcResult[4+2] *dquant_dc) << t0) ) ;
			dcResult[4+3] = (((dcResult[4+3] *dquant_dc) << t0) ) ;

			dcResult[8+0] = (((dcResult[8+0] *dquant_dc) << t0) ) ;
			dcResult[8+1] = (((dcResult[8+1] *dquant_dc) << t0) ) ;
			dcResult[8+2] = (((dcResult[8+2] *dquant_dc) << t0) ) ;
			dcResult[8+3] = (((dcResult[8+3] *dquant_dc) << t0) ) ;

			dcResult[12+0] = (((dcResult[12+0] *dquant_dc) << t0) ) ;
			dcResult[12+1] = (((dcResult[12+1] *dquant_dc) << t0) ) ;
			dcResult[12+2] = (((dcResult[12+2] *dquant_dc) << t0) ) ;
			dcResult[12+3] = (((dcResult[12+3] *dquant_dc) << t0) ) ;
		}
		else
		{
			int t0= 2-qp_per;
			dcResult[0+0] = (((dcResult[0+0] *dquant_dc + round) >> t0) ) ;
			dcResult[0+1] = (((dcResult[0+1] *dquant_dc + round) >> t0) ) ;
			dcResult[0+2] = (((dcResult[0+2] *dquant_dc + round) >> t0) ) ;
			dcResult[0+3] = (((dcResult[0+3] *dquant_dc + round) >> t0) ) ;

			dcResult[4+0] = (((dcResult[4+0] *dquant_dc + round) >> t0) ) ;
			dcResult[4+1] = (((dcResult[4+1] *dquant_dc + round) >> t0) ) ;
			dcResult[4+2] = (((dcResult[4+2] *dquant_dc + round) >> t0) ) ;
			dcResult[4+3] = (((dcResult[4+3] *dquant_dc + round) >> t0) ) ;

			dcResult[8+0] = (((dcResult[8+0] *dquant_dc + round) >> t0) ) ;
			dcResult[8+1] = (((dcResult[8+1] *dquant_dc + round) >> t0) ) ;
			dcResult[8+2] = (((dcResult[8+2] *dquant_dc + round) >> t0) ) ;
			dcResult[8+3] = (((dcResult[8+3] *dquant_dc + round) >> t0) ) ;

			dcResult[12+0] = (((dcResult[12+0] *dquant_dc + round) >> t0) ) ;
			dcResult[12+1] = (((dcResult[12+1] *dquant_dc + round) >> t0) ) ;
			dcResult[12+2] = (((dcResult[12+2] *dquant_dc + round) >> t0) ) ;
			dcResult[12+3] = (((dcResult[12+3] *dquant_dc + round) >> t0) ) ;

			//for (j=0;j< 16;j+=4)
			//	for (i=0;i< 4;i++)
			//		dcResult[j+i] = (((dcResult[j+i] *dquant_dc + round) >> (2 - qp_per)) ) ;
		}

		*ppSrcCoeff += 16;

	}
	else
	{
		
		//***
		//count4++;

		*((int *)(dcResult+ 0)) = 0;
		*((int *)(dcResult+ 2)) = 0;
		*((int *)(dcResult+ 4)) = 0;
		*((int *)(dcResult+ 6)) = 0;
		*((int *)(dcResult+ 8)) = 0;
		*((int *)(dcResult+10)) = 0;
		*((int *)(dcResult+12)) = 0;
		*((int *)(dcResult+14)) = 0;
		
		//memset((short *) dcResult, 0, 32);
	}

	// Third IDCT 
	if(cbp == 1)
	{
		//***
		//count5++;
		Ipp8u *d = pSrcDstYPlane;
		Ipp8u *s = prediction_array;					// LEFT
		int	  const_255 = 255;

		for( j = 4; j--; )
		{
			int t0 = ((dcResult[0]+32)>>6);
			int t1 = ((dcResult[1]+32)>>6);
			int t2 = ((dcResult[2]+32)>>6);
			int t3 = ((dcResult[3]+32)>>6);
			
			dcResult += 4;
			
			for( i = 4; i--; )
			{
				int t, tmp;

				tmp = (s[ 0] + t0); Clip1(const_255, tmp );	t  = tmp<<0;
				tmp = (s[ 1] + t0); Clip1(const_255, tmp );	t |= tmp<<8;
				tmp = (s[ 2] + t0); Clip1(const_255, tmp );	t |= tmp<<16;
				tmp = (s[ 3] + t0); Clip1(const_255, tmp );	t |= tmp<<24;	*((int *)(d +  0)) = t;

				tmp = (s[ 4] + t1); Clip1(const_255, tmp );	t  = tmp<<0;
				tmp = (s[ 5] + t1); Clip1(const_255, tmp );	t |= tmp<<8;
				tmp = (s[ 6] + t1); Clip1(const_255, tmp );	t |= tmp<<16;
				tmp = (s[ 7] + t1); Clip1(const_255, tmp );	t |= tmp<<24;	*((int *)(d +  4)) = t;

				tmp = (s[ 8] + t2); Clip1(const_255, tmp );	t  = tmp<<0;
				tmp = (s[ 9] + t2); Clip1(const_255, tmp );	t |= tmp<<8;
				tmp = (s[10] + t2); Clip1(const_255, tmp );	t |= tmp<<16;
				tmp = (s[11] + t2); Clip1(const_255, tmp );	t |= tmp<<24;	*((int *)(d +  8)) = t;
										  
				tmp = (s[12] + t3); Clip1(const_255, tmp );	t  = tmp<<0;
				tmp = (s[13] + t3); Clip1(const_255, tmp );	t |= tmp<<8;
				tmp = (s[14] + t3); Clip1(const_255, tmp );	t |= tmp<<16;
				tmp = (s[15] + t3); Clip1(const_255, tmp );	t |= tmp<<24;	*((int *)(d + 12)) = t;

				d += pitch;
				s += 16;
			}
		}

		goto bail;
	}

	//***
	//count6++;
	{
		__ALIGN8(Ipp8u, pred, 16+2*16);
		Ipp16s	*m0 = (Ipp16s *)&pred[16];
		int		const_255 = 255;

		for(k=0; k< 16; k++)
		{
			// Please note: we use normal order according to AC components here-- WWD in 2006-05-06
			{
				int block_x = ac_block_idx_x[k];
				int block_y = ac_block_idx_y[k];
				int	dc_idx  = decode_block_scan[k];
				Ipp8u  *d	= pSrcDstYPlane + block_y*pitch + block_x;
				Ipp8u  *p	= prediction_array + block_y*16 + block_x;

				if( !(cbp & (1<< (1+k))))
				{
					//***
					//count7++;
					int t0 = ((dcResult[dc_idx]+32)>>6);
					int tmp;

					tmp = p[0] + t0; Clip1(const_255, tmp ); d[0] = tmp;
					tmp = p[1] + t0; Clip1(const_255, tmp ); d[1] = tmp;
					tmp = p[2] + t0; Clip1(const_255, tmp ); d[2] = tmp;
					tmp = p[3] + t0; Clip1(const_255, tmp ); d[3] = tmp; p += 16; d += pitch;
														   
					tmp = p[0] + t0; Clip1(const_255, tmp ); d[0] = tmp;
					tmp = p[1] + t0; Clip1(const_255, tmp ); d[1] = tmp;
					tmp = p[2] + t0; Clip1(const_255, tmp ); d[2] = tmp;
					tmp = p[3] + t0; Clip1(const_255, tmp ); d[3] = tmp; p += 16; d += pitch;
														   
					tmp = p[0] + t0; Clip1(const_255, tmp ); d[0] = tmp;
					tmp = p[1] + t0; Clip1(const_255, tmp ); d[1] = tmp;
					tmp = p[2] + t0; Clip1(const_255, tmp ); d[2] = tmp;
					tmp = p[3] + t0; Clip1(const_255, tmp ); d[3] = tmp; p += 16; d += pitch;
														   
					tmp = p[0] + t0; Clip1(const_255, tmp ); d[0] = tmp;
					tmp = p[1] + t0; Clip1(const_255, tmp ); d[1] = tmp;
					tmp = p[2] + t0; Clip1(const_255, tmp ); d[2] = tmp;
					tmp = p[3] + t0; Clip1(const_255, tmp ); d[3] = tmp; 
				}
				else
				{
					//***
					//count8++;
					Ipp16s *coef =  *ppSrcCoeff;

					m0[0] = dcResult[dc_idx];
					*((int *)(pred+0))	 = *((int *)(p+0  ));
					*((int *)(pred+4))  = *((int *)(p+16  ));
					*((int *)(pred+8))  = *((int *)(p+16*2));
					*((int *)(pred+12)) = *((int *)(p+16*3));

					dquant_trans_recon_dc_x_2(  coef, dquant, qp_per, m0, d, pitch );
					*ppSrcCoeff += 16;
				}	
			}
		}
	}
}

bail:

	Profile_End(ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c_profile);


	return ippStsNoErr;

}


IppStatus __STDCALL ippiReconstructChromaIntraMB_H264_16s8u_P2R_c
(
	Ipp16s **ppSrcCoeff,
    Ipp8u *pSrcDstUPlane,
    Ipp8u *pSrcDstVPlane,
    const Ipp32u srcdstUVStep,
    const IppIntraChromaPredMode_H264 intra_chroma_mode,
    const Ipp32u cbp4x4,
    const Ipp32u ChromaQP,
    const Ipp8u edge_type
)
{

	Profile_Start(ippiReconstructChromaIntraMB_H264_16s8u_P2R_c_profile);


#ifdef USE_INTEL_IPP_RECON
	IppStatus status;
	status = ippiReconstructChromaIntraMB_H264_16s8u_P2R
			(ppSrcCoeff,pSrcDstUPlane,pSrcDstVPlane,srcdstUVStep,intra_chroma_mode,cbp4x4,ChromaQP,edge_type);

	Profile_End(ippiReconstructChromaIntraMB_H264_16s8u_P2R_c_profile);

	return status;
#endif

	int		i,	j, k;
	Ipp8u   *dst_u		= pSrcDstUPlane;
	Ipp8u   *dst_v		= pSrcDstVPlane;
	int		pitch		= srcdstUVStep;
	int		qp_per_uv	= ChromaQP/6;
	int		qp_rem_uv	= ChromaQP%6;	
	__ALIGN8(Ipp8u, pred, 16+2*16);
	Ipp16s	*m0 = (Ipp16s *)&pred[16];

	__ALIGN4(Ipp8u, pred_uv, 64*2);
	unsigned char *pred_u = pred_uv;
	unsigned char *pred_v = pred_uv + 64;
	_DECLSPEC int		dcArrayU[4], dcResultU[4];
	_DECLSPEC int		dcArrayV[4], dcResultV[4];
	int		iv, ih, ib, ic, iaa;	// Used for Plane prediction
	int	ioff, joff;

	switch (intra_chroma_mode)
	{
		case 0:		// DC_PRED_8
			{	
				// First, we do intra-prediction according to edge_type and intra_luma_mode!!!!!!!!!!!!!!!!!!!!
				int		hasA	= HAS_AA;
				int		hasB	= HAS_BB;
				//int		hasD	= HAS_DD;
				int	jsu[2][2], jsv[2][2];
				int	jsu0=0,	jsu1=0, jsu2=0, jsu3=0;
				int	jsv0=0,	jsv1=0, jsv2=0, jsv3=0;

				if(hasA)
				{
					Ipp8u *du	= dst_u - pitch;				// pointer to TOP U
					
					jsu0 = jsu0 + du[0] + du[1] + du[2] + du[3];
					jsu1 = jsu1 + du[4] + du[5] + du[6] + du[7];
				}

				if(hasB)
				{
					Ipp8u *du	= dst_u - 1 - pitch;
					
					//on arm, add offset when loading is free
					du += pitch; jsu2 += *du;
					du += pitch; jsu2 += *du;
					du += pitch; jsu2 += *du;
					du += pitch; jsu2 += *du;

					du += pitch; jsu3 += *du;
					du += pitch; jsu3 += *du;
					du += pitch; jsu3 += *du;
					du += pitch; jsu3 += *du;
				}

				if(hasA && hasB)
				{
				  jsu[0][0]=(jsu0+jsu2+4)>>3;
				  jsu[1][0]=(jsu1+2)>>2;
				  jsu[0][1]=(jsu3+2)>>2;
				  jsu[1][1]=(jsu1+jsu3+4)>>3;
				}

				if(hasA && !hasB)
				{
				  jsu[0][0]=(jsu0+2)>>2;
				  jsu[1][0]=(jsu1+2)>>2;
				  jsu[0][1]=(jsu0+2)>>2;
				  jsu[1][1]=(jsu1+2)>>2;
				}

				if(hasB && !hasA)
				{
				  jsu[0][0]=(jsu2+2)>>2;
				  jsu[1][0]=(jsu2+2)>>2;
				  jsu[0][1]=(jsu3+2)>>2;
				  jsu[1][1]=(jsu3+2)>>2;
				}

				if(!hasA && !hasB)
				{
				  jsu[0][0]=128;
				  jsu[1][0]=128;
				  jsu[0][1]=128;
				  jsu[1][1]=128;
				}
			
				//v
				if(hasA)
				{
					Ipp8u *dv	= dst_v - pitch;				// pointer to TOP V
					
					jsv0 = jsv0 + dv[0] + dv[1] + dv[2] + dv[3];
					jsv1 = jsv1 + dv[4] + dv[5] + dv[6] + dv[7];
				}

				if(hasB)
				{
					Ipp8u *dv	= dst_v - 1 - pitch;
					
					//on arm, add offset when loading is free
					dv += pitch; jsv2 += *dv;
					dv += pitch; jsv2 += *dv;
					dv += pitch; jsv2 += *dv;
					dv += pitch; jsv2 += *dv;
										   
					dv += pitch; jsv3 += *dv;
					dv += pitch; jsv3 += *dv;
					dv += pitch; jsv3 += *dv;
					dv += pitch; jsv3 += *dv;
				}

				if(hasA && hasB)
				{
				  jsv[0][0]=(jsv0+jsv2+4)>>3;
				  jsv[1][0]=(jsv1+2)>>2;
				  jsv[0][1]=(jsv3+2)>>2;
				  jsv[1][1]=(jsv1+jsv3+4)>>3;
				}

				if(hasA && !hasB)
				{
				  jsv[0][0]=(jsv0+2)>>2;
				  jsv[1][0]=(jsv1+2)>>2;
				  jsv[0][1]=(jsv0+2)>>2;
				  jsv[1][1]=(jsv1+2)>>2;
				}

				if(hasB && !hasA)
				{
				  jsv[0][0]=(jsv2+2)>>2;
				  jsv[1][0]=(jsv2+2)>>2;
				  jsv[0][1]=(jsv3+2)>>2;
				  jsv[1][1]=(jsv3+2)>>2;
				}

				if(!hasA && !hasB)
				{
				  jsv[0][0]=128;
				  jsv[1][0]=128;
				  jsv[0][1]=128;
				  jsv[1][1]=128;
				}

				int	tmp1, tmp2, tmp3, tmp4;
				unsigned char *p;
				
				tmp1 = (jsu[0][0]<<24)|(jsu[0][0]<<16)|(jsu[0][0]<<8)|(jsu[0][0]<<0);			
				tmp2 = (jsu[1][0]<<24)|(jsu[1][0]<<16)|(jsu[1][0]<<8)|(jsu[1][0]<<0);			
				tmp3 = (jsu[0][1]<<24)|(jsu[0][1]<<16)|(jsu[0][1]<<8)|(jsu[0][1]<<0);			
				tmp4 = (jsu[1][1]<<24)|(jsu[1][1]<<16)|(jsu[1][1]<<8)|(jsu[1][1]<<0);			
				
				p = pred_u -4;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;

				p += 4; *(int *)p = tmp3;
				p += 4; *(int *)p = tmp4;
				p += 4; *(int *)p = tmp3;
				p += 4; *(int *)p = tmp4;
				p += 4; *(int *)p = tmp3;
				p += 4; *(int *)p = tmp4;
				p += 4; *(int *)p = tmp3;
				p += 4; *(int *)p = tmp4;

				tmp1 = (jsv[0][0]<<24)|(jsv[0][0]<<16)|(jsv[0][0]<<8)|(jsv[0][0]<<0);
				tmp2 = (jsv[1][0]<<24)|(jsv[1][0]<<16)|(jsv[1][0]<<8)|(jsv[1][0]<<0);
				tmp3 = (jsv[0][1]<<24)|(jsv[0][1]<<16)|(jsv[0][1]<<8)|(jsv[0][1]<<0);
				tmp4 = (jsv[1][1]<<24)|(jsv[1][1]<<16)|(jsv[1][1]<<8)|(jsv[1][1]<<0);

				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;

				p += 4; *(int *)p = tmp3;
				p += 4; *(int *)p = tmp4;
				p += 4; *(int *)p = tmp3;
				p += 4; *(int *)p = tmp4;
				p += 4; *(int *)p = tmp3;
				p += 4; *(int *)p = tmp4;
				p += 4; *(int *)p = tmp3;
				p += 4; *(int *)p = tmp4;
			}
		break;
		case 1:										// HOR_PRED_8
			{
				int tmp;
				unsigned char *s;
				unsigned char *p;

				s   = dst_u - 1 - pitch;
				p   = pred_u - 4;
				
				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				////
				s   = dst_v - 1 - pitch;				
				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;

				s  += pitch;
				tmp = (s[0]<<24)|(s[0]<<16)|(s[0]<<8)|(s[0]<<0);
				p += 4; *(int *)p = tmp;
				p += 4; *(int *)p = tmp;
			}

			break;
		case 2:										// VERT_PRED_8
			{
				int tmp1, tmp2;
				unsigned char *s;
				unsigned char *p;

				s   = dst_u  - pitch;
				p   = pred_u - 4;	
				tmp1 = *(int *)s;
				tmp2 = *(int *)(s+4);
				
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;

				s   = dst_v  - pitch;				
				tmp1 = *(int *)s;
				tmp2 = *(int *)(s+4);
				
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
				p += 4; *(int *)p = tmp1;
				p += 4; *(int *)p = tmp2;
			}
		break;
		case 3:										// PLANE_8
			for (j=0;j<2;j++)
			{	
				joff=j*4;
				for(i=0;i<2;i++)
				{
					ioff=i*4;

					// First, do prediction for U
					ih = iv = 0;

					Ipp8u *pa	= dst_u -pitch;	// Up
					Ipp8u *pb	= dst_u -1;				// Left

					ih += 1*(pa[4] - pa[ 2]);
					ih += 2*(pa[5] - pa[ 1]);
					ih += 3*(pa[6] - pa[ 0]);
					ih += 4*(pa[7] - pa[-1]);

					iv += 1*(pb[4*pitch] - pb[ 2*pitch]);
					iv += 2*(pb[5*pitch] - pb[ 1*pitch]);
					iv += 3*(pb[6*pitch] - pb[ 0*pitch]);
					iv += 4*(pb[7*pitch] - pb[-1*pitch]);

					ib=(17*ih+16)>>5;
					ic=(17*iv+16)>>5;

					iaa=16*(dst_u[7-pitch] + dst_u[-1+7*pitch]);

					{
						int const_255 = 255;
						int jj;

						for (jj=0; jj<4; jj++)
						{
							int tmp;
							Ipp8u *d = pred_u + joff*8+ioff+8*jj - 1;

							tmp = (iaa+(0+ioff-3)*ib +(jj+joff-3)*ic + 16)>>5;	Clip1( const_255, tmp ); 
							*(++d) = tmp;

							tmp = (iaa+(1+ioff-3)*ib +(jj+joff-3)*ic + 16)>>5;	Clip1( const_255, tmp ); 
							*(++d) = tmp;

							tmp = (iaa+(2+ioff-3)*ib +(jj+joff-3)*ic + 16)>>5;	Clip1( const_255, tmp ); 
							*(++d) = tmp;

							tmp = (iaa+(3+ioff-3)*ib +(jj+joff-3)*ic + 16)>>5;	Clip1( const_255, tmp ); 
							*(++d) = tmp;
						}
					}	
				}
			}
					
			for (j=0;j<2;j++)
			{	
				joff=j*4;
				for(i=0;i<2;i++)
				{
					ioff=i*4;

					// Second, do prediction for V
					ih = iv = 0;

					Ipp8u *pSrcforPredictionV	= dst_v -pitch;	// Up
					Ipp8u *pSrcforPrediction	= dst_v -1;				// Left

					{
						int ii;
						for (ii=1;ii<5;ii++)
						{
							ih += ii*(pSrcforPredictionV[3+ii] - pSrcforPredictionV[3-ii]);
							iv += ii*(pSrcforPrediction[(3+ii)*pitch] - pSrcforPrediction[(3-ii)*pitch]);
						}
					}
					ib=(17*ih+16)>>5;
					ic=(17*iv+16)>>5;

					iaa=16*(dst_v[7-pitch] + dst_v[-1+7*pitch]);
					//iaa=16*(pSrcDstYPlane[-1+15*srcdstYStep] + pSrcDstYPlane[15-srcdstYStep]);

					{
						int const_255 = 255;
						int ii, jj;
						for (jj=0; jj<4; jj++)
							for (ii=0; ii<4; ii++)
							{
								int tmp = (iaa+(ii+ioff-3)*ib +(jj+joff-3)*ic + 16)>>5;

								Clip1( const_255, tmp );
								pred_v[joff*8+ioff+8*jj+ii] = tmp;
							}
					}

				}
			}
		  break;
		default:
			//error("illegal chroma intra prediction mode", 600);
		break;
	}

	// Second, we do inverse transform for each 4x4 block!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//         include inverse Quantization and add operation
	unsigned int cbp = cbp4x4>> 17;			// Use Chroma part ONLY
	if(!(cbp & 0xffff))
	{
		// Non- cbp has information: use prediction as coeffs ONLY  -- WWD in 2006-05-05
		Ipp8u *d;
		Ipp8u *s;

		d = dst_u - pitch;
		s = pred_u - 8;

		d += pitch; s += 8; *(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8;	*(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8; *(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8;	*(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8;	*(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8;	*(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8;	*(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8;	*(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);

		d = dst_v - pitch;
		s = pred_v - 8;

		d += pitch; s += 8; *(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8;	*(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8; *(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8;	*(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8;	*(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8;	*(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8;	*(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);
		d += pitch; s += 8;	*(int *)(d+0) = *(int *)(s+0); *(int *)(d+4) = *(int *)(s+4);

		goto bail;
	}

{
	Ipp16u	*dquant = &(dequant_coef[qp_rem_uv][0]);
	Ipp16u dquant_dc = dquant[0];
	// U
	if(cbp & 0x01)
	{
		// 2.1 2x2 DC Transform
		// IPPVC_CBP_1ST_CHROMA_DC_BITPOS
		if(ChromaQP >= 6)
		{
			int tmp = (qp_per_uv - 1);

			dcArrayU[0] = ((*ppSrcCoeff)[0]) *dquant_dc << tmp;
			dcArrayU[1] = ((*ppSrcCoeff)[1]) *dquant_dc << tmp;
			dcArrayU[2] = ((*ppSrcCoeff)[2]) *dquant_dc << tmp;
			dcArrayU[3] = ((*ppSrcCoeff)[3]) *dquant_dc << tmp;
		}
		else
		{
			dcArrayU[0] = ((*ppSrcCoeff)[0]) *dquant_dc>> 1;
			dcArrayU[1] = ((*ppSrcCoeff)[1]) *dquant_dc>> 1;
			dcArrayU[2] = ((*ppSrcCoeff)[2]) *dquant_dc>> 1;
			dcArrayU[3] = ((*ppSrcCoeff)[3]) *dquant_dc>> 1;
		}

		dcResultU[0] = (dcArrayU[0] + dcArrayU[1] + dcArrayU[2] + dcArrayU[3]);
		dcResultU[1] = (dcArrayU[0] - dcArrayU[1] + dcArrayU[2] - dcArrayU[3]);
		dcResultU[2] = (dcArrayU[0] + dcArrayU[1] - dcArrayU[2] - dcArrayU[3]);
		dcResultU[3] = (dcArrayU[0] - dcArrayU[1] - dcArrayU[2] + dcArrayU[3]);

		*ppSrcCoeff += 4;
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
		if(ChromaQP >= 6)
		{
			int tmp = (qp_per_uv - 1);

			dcArrayV[0] = ((*ppSrcCoeff)[0]) *dquant_dc << tmp;
			dcArrayV[1] = ((*ppSrcCoeff)[1]) *dquant_dc << tmp;
			dcArrayV[2] = ((*ppSrcCoeff)[2]) *dquant_dc << tmp;
			dcArrayV[3] = ((*ppSrcCoeff)[3]) *dquant_dc << tmp;
		}
		else
		{
			dcArrayV[0] = ((*ppSrcCoeff)[0]) *dquant_dc>> 1;
			dcArrayV[1] = ((*ppSrcCoeff)[1]) *dquant_dc>> 1;
			dcArrayV[2] = ((*ppSrcCoeff)[2]) *dquant_dc>> 1;
			dcArrayV[3] = ((*ppSrcCoeff)[3]) *dquant_dc>> 1;
		}

		dcResultV[0] = (dcArrayV[0] + dcArrayV[1] + dcArrayV[2] + dcArrayV[3]);
		dcResultV[1] = (dcArrayV[0] - dcArrayV[1] + dcArrayV[2] - dcArrayV[3]);
		dcResultV[2] = (dcArrayV[0] + dcArrayV[1] - dcArrayV[2] - dcArrayV[3]);
		dcResultV[3] = (dcArrayV[0] - dcArrayV[1] - dcArrayV[2] + dcArrayV[3]);

		*ppSrcCoeff += 4;
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
		int block_x = block_idx[k<<1];
		int block_y = block_idx[(k<<1) + 1];

		if(!(cbp&(1<<(2+k))))
		{
			int const_255 = 255;
			Ipp8u *d = dst_u             + block_y*pitch - pitch + block_x;
			Ipp8u *s = pred_u + block_y*8     - 8     + block_x;
			int tmp;

			// This block has no AC information : but we have DC information
			d += pitch;
			s += 8;
			tmp = s[0] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[0] = tmp;
			tmp = s[1] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[1] = tmp;
			tmp = s[2] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[2] = tmp;
			tmp = s[3] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[3] = tmp; 

			d += pitch;
			s += 8;
			tmp = s[0] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[0] = tmp;
			tmp = s[1] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[1] = tmp;
			tmp = s[2] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[2] = tmp;
			tmp = s[3] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[3] = tmp; 

			d += pitch;
			s += 8;
			tmp = s[0] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[0] = tmp;
			tmp = s[1] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[1] = tmp;
			tmp = s[2] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[2] = tmp;
			tmp = s[3] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[3] = tmp; 
																			
			d += pitch;
			s += 8;
			tmp = s[0] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[0] = tmp;
			tmp = s[1] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[1] = tmp;
			tmp = s[2] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[2] = tmp;
			tmp = s[3] + ((dcResultU[k]+32)>>6);	Clip1( const_255, tmp ); d[3] = tmp; 
		}
		else
		{
			Ipp8u  *dst    = dst_u + block_y*pitch + block_x;
			Ipp8u  *p      = pred_u + block_y*8 + block_x;
			Ipp16s *coef   =  *ppSrcCoeff;
			int		qp_per = qp_per_uv|0x8000;
			
			m0[0] = dcResultU[k];
			*((int *)(pred+0))	 = *((int *)(p+0  ));
			*((int *)(pred+4))  = *((int *)(p+8  ));
			*((int *)(pred+8))  = *((int *)(p+8*2));
			*((int *)(pred+12)) = *((int *)(p+8*3));
			//***
			//dquant_trans_recon_dc_arm(  coef, dquant, qp_per, m0, dst, pitch );
			dquant_trans_recon_dc_x_2(  coef, dquant, qp_per, m0, dst, pitch );
			*ppSrcCoeff += 16;
		}
	}

	// V -- AC
	for(k=0; k<4; k++)	// CBP decision
	{
		int block_x = block_idx[k<<1];
		int block_y = block_idx[(k<<1) + 1];

		// This statement is different for U and V
		if(!(cbp&(1<<(6+k))))
		{
			// This block has no AC information : but we have DC information
			int const_255 = 255;
			Ipp8u *d_v = dst_v             + block_y*pitch - pitch + block_x;
			Ipp8u *s_v = pred_v + block_y*8     - 8     + block_x;
			int tmp;

			d_v += pitch;
			s_v += 8;
			tmp = s_v[0] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[0] = tmp;
			tmp = s_v[1] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[1] = tmp;
			tmp = s_v[2] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[2] = tmp;
			tmp = s_v[3] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[3] = tmp; 

			d_v += pitch;
			s_v += 8;
			tmp = s_v[0] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[0] = tmp;
			tmp = s_v[1] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[1] = tmp;
			tmp = s_v[2] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[2] = tmp;
			tmp = s_v[3] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[3] = tmp; 

			d_v += pitch;
			s_v += 8;
			tmp = s_v[0] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[0] = tmp;
			tmp = s_v[1] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[1] = tmp;
			tmp = s_v[2] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[2] = tmp;
			tmp = s_v[3] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[3] = tmp; 
																			
			d_v += pitch;
			s_v += 8;
			tmp = s_v[0] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[0] = tmp;
			tmp = s_v[1] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[1] = tmp;
			tmp = s_v[2] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[2] = tmp;
			tmp = s_v[3] + ((dcResultV[k]+32)>>6);	Clip1( const_255, tmp ); d_v[3] = tmp; 
		}
		else
		{
			Ipp8u  *dst    = dst_v + block_y*pitch + block_x;
			Ipp8u  *p      = pred_v + block_y*8 + block_x;
			Ipp16s *coef   =  *ppSrcCoeff;
			int		qp_per = qp_per_uv|0x8000;
			
			m0[0] = dcResultV[k];
			*((int *)(pred+0))	 = *((int *)(p+0  ));
			*((int *)(pred+4))  = *((int *)(p+8  ));
			*((int *)(pred+8))  = *((int *)(p+8*2));
			*((int *)(pred+12)) = *((int *)(p+8*3));
			//***
			//dquant_trans_recon_dc_arm(  coef, dquant, qp_per, m0, dst, pitch );
			dquant_trans_recon_dc_x_2(  coef, dquant, qp_per, m0, dst, pitch );
			*ppSrcCoeff += 16;
		}
	}
}

bail:
	// Third, we do add operation and clip operation for finnal result!!!!!!!!!!!!!!!!!!!!!!!!!

	Profile_End(ippiReconstructChromaIntraMB_H264_16s8u_P2R_c_profile);

	return ippStsNoErr;
}



IppStatus __STDCALL ippiReconstructLumaInterMB_H264_16s8u_C1R_c(Ipp16s **ppSrcCoeff,
                                                              Ipp8u *pSrcDstYPlane,
                                                              Ipp32u srcdstYStep,
                                                              Ipp32u cbp4x4,
                                                              Ipp32s QP)
{

	Profile_Start(ippiReconstructLumaInterMB_H264_16s8u_C1R_c_profile);


#ifdef USE_INTEL_IPP_RECON
	IppStatus status;
	status = ippiReconstructLumaInterMB_H264_16s8u_C1R
			(ppSrcCoeff,pSrcDstYPlane,srcdstYStep,cbp4x4,QP);

	Profile_End(ippiReconstructLumaInterMB_H264_16s8u_C1R_c_profile);

	return status;
#endif

	int		k;
	Ipp8u	*dst;
	int		pitch = srcdstYStep;
	unsigned int  cbp = cbp4x4 & 0x1ffff;		// Need 17 bits to represent Luma information 
	int		block_x, block_y;
	int		qp_per = QP/6;
	int		qp_rem = QP%6;
	Ipp16u	*dquant = &(dequant_coef[qp_rem][0]);
	__ALIGN8(Ipp8u, pred, 16+2*16);
	Ipp16s	*m0 = (Ipp16s *)&pred[16];

	for(k=0; k<16; k++)			// In scan order
	{	
		if( !(cbp & (1<< (1+k))))
			continue;

		block_x = ac_block_idx_x[k];
		block_y = ac_block_idx_y[k];

		dst = pSrcDstYPlane + block_y*pitch + block_x;			// Pointer to top_left cornor of one 4x4 block
		{
			Ipp16s *coef =  *ppSrcCoeff;
			
			*((int *)(pred+0))	 = *((int *)(dst+0      ));
			*((int *)(pred+4))  = *((int *)(dst+pitch  ));
			*((int *)(pred+8))  = *((int *)(dst+pitch*2));
			*((int *)(pred+12)) = *((int *)(dst+pitch*3));
			//dquant_trans_recon_arm(  coef, dquant, qp_per, m0, dst, pitch );
			dquant_trans_recon_x_2(  coef, dquant, qp_per, m0, dst, pitch );
		}

		*ppSrcCoeff += 16;
	}

	Profile_End(ippiReconstructLumaInterMB_H264_16s8u_C1R_c_profile);

	return ippStsNoErr;
}


// For reference only 
#define X0 (PredPel[0])
#define X1 (PredPel[1])
#define X2 (PredPel[2])
#define X3 (PredPel[3])
#define X4 (PredPel[4])
#define X5 (PredPel[5])
#define X6 (PredPel[6])
#define X7 (PredPel[7])
#define X8 (PredPel[8])
#define Y1 (PredPel[9])
#define Y2 (PredPel[10])
#define Y3 (PredPel[11])
#define Y4 (PredPel[12])

IppStatus __STDCALL ippiReconstructLumaIntraMB_H264_16s8u_C1R_c(Ipp16s **ppSrcCoeff,
                                                              Ipp8u *pSrcDstYPlane,
                                                              Ipp32s srcdstYStep,
                                                              const IppIntra4x4PredMode_H264 *pMBIntraTypes,
                                                              const Ipp32u cbp4x4,
                                                              const Ipp32u QP,
                                                              const Ipp8u edge_type)
{


	Profile_Start(ippiReconstructLumaIntraMB_H264_16s8u_C1R_c_profile);


#ifdef USE_INTEL_IPP_RECON
	IppStatus status;
	status = ippiReconstructLumaIntraMB_H264_16s8u_C1R
			(ppSrcCoeff,pSrcDstYPlane,srcdstYStep,pMBIntraTypes,cbp4x4,QP,edge_type);

	Profile_End(ippiReconstructLumaIntraMB_H264_16s8u_C1R_c_profile);

	return status;
#endif

#define HAS_A (hasABC&0x04)
#define HAS_B (hasABC&0x02)
#define HAS_C (hasABC&0x01)
   
	__ALIGN8(Ipp8u, pred, 16+2*16);
	Ipp16s	*m = (Ipp16s *)&pred[16];

	Ipp8u			hasABC_ary[16] = { 7, 7, 7, 6,    7, 7, 7, 6,    7, 7, 7, 6,   7, 6, 7, 6};
	Ipp8u			PredPel[16]; 
	int				block_x, block_y;
	int				k;
	Ipp8u			*dst;
	int				pitch = srcdstYStep;
	int				qp_per, qp_rem;
	unsigned int	cbp = cbp4x4 & 0x1ffff;		// Need 17 bits to represent Luma information 
	Ipp16u			*dquant;

	qp_per	= QP/6;
	qp_rem	= QP%6;
	dquant	= &(dequant_coef[qp_rem][0]);

	{
		int tmp;

		int hasA = HAS_AA;
		int hasB = HAS_BB;
		int hasC = HAS_CC;
		
		tmp = (hasA<<1)|hasA;
		hasABC_ary[0] = (hasB<<2)|tmp; 
		hasABC_ary[1] = (hasABC_ary[1]&0x04)|tmp; 
		hasABC_ary[4] = (hasABC_ary[4]&0x04)|tmp; 

		tmp =  (hasA<<1)|hasC;
		hasABC_ary[5] = (hasABC_ary[5]&0x04)|tmp; 

		tmp =  (hasB<<2);
		hasABC_ary[2] = (hasABC_ary[2]&0x03)|tmp; 
		hasABC_ary[8] = (hasABC_ary[8]&0x03)|tmp; 
		hasABC_ary[10] =(hasABC_ary[10]&0x03)|tmp; 
	}

	for(k=0; k<16; k++)			// In scan order (4x4 blocks)
	{
		block_x = ac_block_idx_x[k];
		block_y = ac_block_idx_y[k];
		dst = pSrcDstYPlane + block_y*srcdstYStep + block_x;			// Pointer to top_left cornor of one 4x4 block				

		switch(pMBIntraTypes[k])
		{
			case IPP_4x4_VERT:  		
				//***
				//i_count[0]++;
				
				{
					int tmp_long = *((int *)(dst - srcdstYStep));
					
					*(int *)(pred + 0 ) = tmp_long;
					*(int *)(pred + 4 ) = tmp_long;
					*(int *)(pred + 8 ) = tmp_long;
					*(int *)(pred + 12) = tmp_long;
				}
				break;

			case IPP_4x4_HOR:                        /* horizontal prediction from left block */
				//***
				//i_count[1]++;
				
				{
					Ipp8u *tmp = dst - 1;

					Y1 = tmp[ 0 ];
					Y2 = tmp[ srcdstYStep ];
					Y3 = tmp[ srcdstYStep + srcdstYStep ];
					Y4 = tmp[(srcdstYStep<<1) + srcdstYStep ];
				}
				
				{
					int tmp_long;
					
					tmp_long = (Y1<<24)|(Y1<<16)|(Y1<<8)|(Y1<<0);
					*(int *)(pred + 0 ) = tmp_long;
					tmp_long = (Y2<<24)|(Y2<<16)|(Y2<<8)|(Y2<<0);
					*(int *)(pred + 4 ) = tmp_long;
					tmp_long = (Y3<<24)|(Y3<<16)|(Y3<<8)|(Y3<<0);
					*(int *)(pred + 8 ) = tmp_long;
					tmp_long = (Y4<<24)|(Y4<<16)|(Y4<<8)|(Y4<<0);
					*(int *)(pred + 12) = tmp_long;
				}
				break;

			case IPP_4x4_DC:                         /* DC prediction */
				//***
				//i_count[2]++;
				
				{
					int hasABC = hasABC_ary[k];
					int	s0;

					if (HAS_B && HAS_A)// no edge
					{
						Ipp8u *tmp = dst - srcdstYStep;

						X1 = tmp[0];
						X2 = tmp[1];
						X3 = tmp[2];
						X4 = tmp[3];

						tmp = dst - 1;

						Y1 = tmp[ 0 ];
						Y2 = tmp[ srcdstYStep ];
						Y3 = tmp[ srcdstYStep + srcdstYStep ];
						Y4 = tmp[(srcdstYStep<<1) + srcdstYStep ];
						s0 = (X1 + X2 + X3 + X4 + Y1 + Y2 + Y3 + Y4 + 4)>>3;
					}
					else if (!HAS_B && HAS_A)// upper edge
					{				
						Ipp8u *tmp = dst - 1;

						Y1 = tmp[ 0 ];
						Y2 = tmp[ srcdstYStep ];
						Y3 = tmp[ srcdstYStep + srcdstYStep ];
						Y4 = tmp[(srcdstYStep<<1) + srcdstYStep ];

						s0 = (Y1 + Y2 + Y3 + Y4 + 2)>>2;             
					}			
					else if (HAS_B && !HAS_A)// left edge
					{				
						Ipp8u *tmp = dst - srcdstYStep;

						X1 = tmp[0];
						X2 = tmp[1];
						X3 = tmp[2];
						X4 = tmp[3];

						s0 = (X1 + X2 + X3 + X4 + 2)>>2;             
					}
					else //if (!block_available_up && !block_available_left)// top left corner, nothing to predict from
					{				
						s0 = 128;                           
					}

					{
						int tmp_long = (s0<<24)|(s0<<16)|(s0<<8)|(s0<<0);
						*(int *)(pred + 0 ) = tmp_long;
						*(int *)(pred + 4 ) = tmp_long;
						*(int *)(pred + 8 ) = tmp_long;
						*(int *)(pred + 12) = tmp_long;
					}
				}
				break;

			case IPP_4x4_DIAG_DL:
				//***
				//i_count[3]++;
				
				{
					int hasABC = hasABC_ary[k];

					Ipp8u *tmp = dst - srcdstYStep;

					X1 = tmp[0];
					X2 = tmp[1];
					X3 = tmp[2];
					X4 = tmp[3];

					if (HAS_C)
					{
						X5 = tmp[4];
						X6 = tmp[5];
						X7 = tmp[6];
						X8 = tmp[7];
					}
					else
						X5 = X6 = X7 = X8 = X4;
				}

				pred[0*4 + 0] = (X1 + X3 + (X2<<1) + 2) >> 2;
				pred[1*4 + 0] = 
				pred[0*4 + 1] = (X2 + X4 + (X3<<1) + 2) >> 2;
				pred[2*4 + 0] =
				pred[1*4 + 1] =
				pred[0*4 + 2] = (X3 + X5 + (X4<<1) + 2) >> 2;
				pred[3*4 + 0] = 
				pred[2*4 + 1] = 
				pred[1*4 + 2] = 
				pred[0*4 + 3] = (X4 + X6 + (X5<<1) + 2) >> 2;
				pred[3*4 + 1] = 
				pred[2*4 + 2] = 
				pred[1*4 + 3] = (X5 + X7 + (X6<<1) + 2) >> 2;
				pred[3*4 + 2] = 
				pred[2*4 + 3] = (X6 + X8 + (X7<<1) + 2) >> 2;
				pred[3*4 + 3] = (X7 + X8 + (X8<<1) + 2) >> 2;				
				break;

			case IPP_4x4_DIAG_DR:
				//***
				//i_count[4]++;
				
				{
					Ipp8u *tmp = dst - srcdstYStep;

					X1 = tmp[0];
					X2 = tmp[1];
					X3 = tmp[2];
					X4 = tmp[3];

					tmp = (Ipp8u *)(dst - 1);					
					X0 = *(dst - srcdstYStep - 1);

					Y1 = tmp[ 0 ];
					Y2 = tmp[ srcdstYStep ];
					Y3 = tmp[ srcdstYStep + srcdstYStep ];
					Y4 = tmp[(srcdstYStep<<1) + srcdstYStep ];
				}

				pred[0 + 3*4] = (Y4 + (Y3<<1) + Y2 + 2) >> 2;
				pred[0 + 2*4] =
				pred[1 + 3*4] = (Y3 + (Y2<<1) + Y1 + 2) >> 2; 
				pred[0 + 1*4] =
				pred[1 + 2*4] = 
				pred[2 + 3*4] = (Y2 + (Y1<<1) + X0 + 2) >> 2; 
				pred[0 + 0*4] =
				pred[1 + 1*4] =
				pred[2 + 2*4] =
				pred[3 + 3*4] = (Y1 + (X0<<1) + X1 + 2) >> 2; 
				pred[1 + 0*4] =
				pred[2 + 1*4] =
				pred[3 + 2*4] = (X0 + (X1<<1) + X2 + 2) >> 2;
				pred[2 + 0*4] =
				pred[3 + 1*4] = (X1 + (X2<<1) + X3 + 2) >> 2;
				pred[3 + 0*4] = (X2 + (X3<<1) + X4 + 2) >> 2;		
				break;

			case  IPP_4x4_VR:/* diagonal prediction -22.5 deg to horizontal plane */
				//***
				//i_count[5]++;
				
				{
					Ipp8u *tmp = dst - srcdstYStep;

					X1 = tmp[0];
					X2 = tmp[1];
					X3 = tmp[2];
					X4 = tmp[3];

					tmp = dst - 1;
					X0 = *(dst  -srcdstYStep - 1);

					Y1 = tmp[ 0 ];
					Y2 = tmp[ srcdstYStep ];
					Y3 = tmp[ srcdstYStep + srcdstYStep ];
				}

				pred[0 + 0*4] = 
				pred[1 + 2*4] = (X0 + X1 + 1) >> 1;
				pred[1 + 0*4] = 
				pred[2 + 2*4] = (X1 + X2 + 1) >> 1;
				pred[2 + 0*4] = 
				pred[3 + 2*4] = (X2 + X3 + 1) >> 1;
				pred[3 + 0*4] = (X3 + X4 + 1) >> 1;
				pred[0 + 1*4] = 
				pred[1 + 3*4] = (Y1 + (X0<<1) + X1 + 2) >> 2;
				pred[1 + 1*4] = 
				pred[2 + 3*4] = (X0 + (X1<<1) + X2 + 2) >> 2;
				pred[2 + 1*4] = 
				pred[3 + 3*4] = (X1 + (X2<<1) + X3 + 2) >> 2;
				pred[3 + 1*4] = (X2 + (X3<<1) + X4 + 2) >> 2;
				pred[0 + 2*4] = (X0 + (Y1<<1) + Y2 + 2) >> 2;
				pred[0 + 3*4] = (Y1 + (Y2<<1) + Y3 + 2) >> 2;				
				break;

			case  IPP_4x4_HD:/* diagonal prediction -22.5 deg to horizontal plane */
				//***
				//i_count[6]++;
				
				{
					Ipp8u *tmp = dst - srcdstYStep;

					X1 = tmp[0];
					X2 = tmp[1];
					X3 = tmp[2];
				
					tmp = dst - 1;
					X0 = *(dst - srcdstYStep - 1);

					Y1 = tmp[ 0 ];
					Y2 = tmp[ srcdstYStep ];
					Y3 = tmp[ srcdstYStep + srcdstYStep ];
					Y4 = tmp[(srcdstYStep<<1) + srcdstYStep ];
				}

				pred[0 + 0*4] = 
				pred[2 + 1*4] = (X0 + Y1 + 1) >> 1;
				pred[1 + 0*4] = 
				pred[3 + 1*4] = (Y1 + (X0<<1) + X1 + 2) >> 2;
				pred[2 + 0*4] = (X0 + (X1<<1) + X2 + 2) >> 2;
				pred[3 + 0*4] = (X1 + (X2<<1) + X3 + 2) >> 2;
				pred[0 + 1*4] = 
				pred[2 + 2*4] = (Y1 + Y2 + 1) >> 1;
				pred[1 + 1*4] = 
				pred[3 + 2*4] = (X0 + (Y1<<1) + Y2 + 2) >> 2;
				pred[0 + 2*4] = 
				pred[2 + 3*4] = (Y2 + Y3 + 1) >> 1;
				pred[1 + 2*4] = 
				pred[3 + 3*4] = (Y1 + (Y2<<1) + Y3 + 2) >> 2;
				pred[0 + 3*4] = (Y3 + Y4 + 1) >> 1;
				pred[1 + 3*4] = (Y2 + (Y3<<1) + Y4 + 2) >> 2;			
				break;

			case  IPP_4x4_VL:/* diagonal prediction -22.5 deg to horizontal plane */
				//***
				//i_count[7]++;
				
				{
					int hasABC = hasABC_ary[k];
					Ipp8u *tmp = dst - srcdstYStep;

					X1 = tmp[0];
					X2 = tmp[1];
					X3 = tmp[2];
					X4 = tmp[3];

					if (HAS_C)
					{
						X5 = tmp[4];
						X6 = tmp[5];
						X7 = tmp[6];
						X8 = tmp[7];
					}
					else
						X5 = X6 = X7 = X8 = X4;
				}

				pred[0 + 0*4] = (X1 + X2 + 1) >> 1;
				pred[1 + 0*4] = 
				pred[0 + 2*4] = (X2 + X3 + 1) >> 1;
				pred[0 + 1*4] = (X1 + (X2<<1) + X3 + 2) >> 2;
				pred[2 + 0*4] = 
				pred[1 + 2*4] = (X3 + X4 + 1) >> 1;
				pred[1 + 1*4] = 
				pred[0 + 3*4] = (X2 + (X3<<1) + X4 + 2) >> 2;
				pred[3 + 0*4] = 
				pred[2 + 2*4] = (X4 + X5 + 1) >> 1;
				pred[2 + 1*4] = 
				pred[1 + 3*4] = (X3 + (X4<<1) + X5 + 2) >> 2;
				pred[3 + 2*4] = (X5 + X6 + 1) >> 1;
				pred[3 + 1*4] = 
				pred[2 + 3*4] = (X4 + (X5<<1) + X6 + 2) >> 2;
				pred[3 + 3*4] = (X5 + (X6<<1) + X7 + 2) >> 2;				
				break;

			case  IPP_4x4_HU:/* diagonal prediction -22.5 deg to horizontal plane */
				//***
				//i_count[8]++;
				
				{
					Ipp8u *tmp = dst - 1;

					Y1 = tmp[ 0 ];
					Y2 = tmp[ srcdstYStep ];
					Y3 = tmp[ srcdstYStep + srcdstYStep ];
					Y4 = tmp[(srcdstYStep<<1) + srcdstYStep ];
				}

				pred[0 + 0*4] = (Y1 + Y2 + 1) >> 1;
				pred[1 + 0*4] = (Y1 + (Y2<<1) + Y3 + 2) >> 2;
				pred[2 + 0*4] = 
				pred[0 + 1*4] = (Y2 + Y3 + 1) >> 1;
				pred[3 + 0*4] = 
				pred[1 + 1*4] = (Y2 + (Y3<<1) + Y4 + 2) >> 2;
				pred[2 + 1*4] = 
				pred[0 + 2*4] = (Y3 + Y4 + 1) >> 1;
				pred[3 + 1*4] = 
				pred[1 + 2*4] = (Y3 + (Y4<<1) + Y4 + 2) >> 2;
				pred[3 + 2*4] = 
				pred[1 + 3*4] = 
				pred[0 + 3*4] = 
				pred[2 + 2*4] = 
				pred[2 + 3*4] = 
				pred[3 + 3*4] = Y4;
				break;

			default:
				printf("Error: illegal intra_4x4 prediction mode\n");
				return ippStsErr;
				break;
		}

		// Make decision for AC components
		if( cbp & cbpmask[k])
		{
			Ipp16s *coef =  *ppSrcCoeff;

			dquant_trans_recon_x_2(  coef, dquant, qp_per, m, dst, pitch );
			coef += 16;
			*ppSrcCoeff = coef;
		}
		else
		{
			// MUST SAVE this result to array for next prediction
			*((int *)&dst[0])								= *((int *)&pred[0]);
			*((int *)&dst[srcdstYStep])                     = *((int *)&pred[4]);
			*((int *)&dst[srcdstYStep<<1])					= *((int *)&pred[8]);
			*((int *)&dst[(srcdstYStep<<1)+srcdstYStep])	= *((int *)&pred[12]);
		}
	}

	Profile_End(ippiReconstructLumaIntraMB_H264_16s8u_C1R_c_profile);

	return ippStsNoErr;
}

#endif

#endif
