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
#ifndef DUMP_H_CSQ_20060506
#define DUMP_H_CSQ_20060506

/*------------------------------------------------------------
*	dump the calling funtion's name and record some info
*/
#ifdef __cplusplus
extern "C" {
#endif
typedef FINFO  
{
	char szInfoList[100][256];
	int value[100][20];
	int count;
}func_info;

typedef struct
{
	int frame_count;
	int valid_frame_count;
	int width;
	int height;
} DEC_INFO;

/* debugging functions */

void set_func_info();
void dump(char *szInfo,int value);
void print_ref_info(char *src_sln_path,DEC_INFO dec_info);

#define		SNippsZero_8u_c												0
#define		SNippsSet_8u_c												1
#define		SNippiCopy_8u_C1R_c											2
#define		SNippiSet_8u_C1R_c											3
#define		SNippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c			4
#define		SNippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c			5
#define		SNippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_c			6
#define		SNippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c			7
#define		SNippiInterpolateBlock_H264_8u_P2P1R_c						8
#define		SNippiInterpolateLuma_H264_8u_C1R_c							9
#define		SNippiInterpolateChroma_H264_8u_C1R_c						10
#define		SNippiDecodeExpGolombOne_H264_1u16s_c						11
#define		SNippiDecodeCAVLCCoeffs_H264_1u16s_c						12
#define		SNippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c				13
#define		SNippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_c		14
#define		SNippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_c	15
#define		SNippiInterpolateLumaTop_H264_8u_C1R_c						16
#define		SNippiInterpolateLumaBottom_H264_8u_C1R_c					17
#define		SNippiInterpolateChromaTop_H264_8u_C1R_c					18
#define		SNippiInterpolateChromaBottom_H264_8u_C1R_c					19
#define		SNippiInterpolateBlock_H264_8u_P3P1R_c						20
#define		SNippiUniDirWeightBlock_H264_8u_C1R_c						21
#define		SNippiBiDirWeightBlock_H264_8u_P2P1R_c						22
#define		SNippiBiDirWeightBlock_H264_8u_P3P1R_c						23
#define		SNippiBiDirWeightBlockImplicit_H264_8u_P3P1R_c				24	
#define		SNippiBiDirWeightBlockImplicit_H264_8u_P2P1R_c				25
#define		SNippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c			26
#define		SNippiReconstructChromaIntraMB_H264_16s8u_P2R_c				27
#define		SNippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_c		28	
#define		SNippiReconstructLumaIntraMB_H264_16s8u_C1R_c				29
#define		SNippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_c			30
#define		SNippiReconstructLumaInterMB_H264_16s8u_C1R_c				31
#define		SNippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_c			32
#define		SNippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_c			33
#define		SNippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_c		34
#define		SNippiReconstructLumaInter8x8MB_H264_16s8u_C1R_c			35
#define		SNippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_c			36
#define		SNippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_c		37
#define		SNippiReconstructLumaInter4x4MB_H264_16s8u_C1R_c			38
#define		SNippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_c		39
#define		SNippiReconstructChromaInter4x4MB_H264_16s8u_P2R_c			40
#define		SNippiReconstructChromaInterMB_H264_16s8u_P2R_c				41
#define		SNippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_c			42

	
#define FUNC_NUM 43

#ifdef DO_PROFILE
// following for testing every function's performance
//#include "vm_time.h"
typedef unsigned __int64 vm_var64;
typedef vm_var64 vm_tick;


static vm_tick t_dur[FUNC_NUM];
static vm_tick t_time;
static long	calls[FUNC_NUM]; 
static char	func_name[FUNC_NUM][128];

void TimeZero();
void Profile_Start(int func);
void Profile_End(int func);
void TimePrint(char *filename);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
