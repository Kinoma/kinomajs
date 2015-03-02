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
#ifndef __KINOMA_UTILITIES_H__
#define __KINOMA_UTILITIES_H__

//***
#include "kinoma_ipp_env.h"

#include <stdio.h>
#include <string.h>
//#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined( WIN32) || defined( _WIN32_WCE )
#include <windows.h>

#define __ALIGN16(type, name, size)		__declspec(align(16)) type name[size]
#define __ALIGN8(type, name, size)		__declspec(align(8))  type name[size]
#define __ALIGN4(type, name, size)		__declspec(align(4))  type name[size]

#define DECL_ALIGN_16	__declspec(align(16))
#define DECL_ALIGN_8	__declspec(align(8))
#define DECL_ALIGN_4	__declspec(align(4))

#elif TARGET_OS_ANDROID || TARGET_OS_MAC

#define __ALIGN16(type, name, size)		type name[size] __attribute__ ((aligned (16)))
#define __ALIGN8(type, name, size)		type name[size] __attribute__ ((aligned (8)))
#define __ALIGN4(type, name, size)		type name[size] __attribute__ ((aligned (4)))

#define DECL_ALIGN_16	__attribute__ ((aligned (16)))
#define DECL_ALIGN_8	__attribute__ ((aligned (8)))
#define DECL_ALIGN_4	__attribute__ ((aligned (4)))

#define LARGE_INTEGER long long

#else

#define __ALIGN16(type, name, size) Ipp8u _a16_##name[(size)*sizeof(type)+15]; type *name = (type*)(((Ipp32s)(_a16_##name) + 15) & ~15)
#define __ALIGN8(type, name, size)  Ipp8u _a8_##name[(size)*sizeof(type)+7]; type *name = (type*)(((Ipp32s)(_a8_##name) + 7) & ~7)
#define __ALIGN4(type, name, size)  Ipp8u _a4_##name[(size)*sizeof(type)+3]; type *name = (type*)(((Ipp32s)(_a4_##name) + 3) & ~3)

#define DECL_ALIGN_16
#define DECL_ALIGN_8
#define DECL_ALIGN_4

#define LARGE_INTEGER long long

#endif


#define GET_LE_LONG(d) ((d)[3]<<24|(d)[2]<<16|(d)[1]<<8|(d)[0]<<0)
#define GET_BE_LONG(d) ((d)[0]<<24|(d)[1]<<16|(d)[2]<<8|(d)[3]<<0)


#define	PUSH_UINT8_BE( s, d)	{ *(d+0)=s;}
#define	PUSH_UINT16_BE( s, d)				\
	{										\
		*(d+0)=(s&(0x0000ff00))>>8;			\
		*(d+1)=(s&(0x000000ff))>>0;			\
	}
#define	PUSH_UINT24_BE( s, d)				\
	{										\
		*(d+0)=(s&(0x00ff0000))>>16;		\
		*(d+1)=(s&(0x0000ff00))>>8;			\
		*(d+2)=(s&(0x000000ff))>>0;			\
	}
#define	PUSH_UINT32_BE( s, d)				\
	{										\
		*(d+0)=(s&(0xff000000))>>24;		\
		*(d+1)=(s&(0x00ff0000))>>16;		\
		*(d+2)=(s&(0x0000ff00))>>8;			\
		*(d+3)=(s&(0x000000ff))>>0;			\
	}

//typedef unsigned __int64 UINT64;
//typedef unsigned long long UINT64;
#if defined( WIN32) || defined( _WIN32_WCE ) || TARGET_OS_ANDROID || TARGET_OS_MAC || TARGET_OS_LINUX
typedef struct 
{
	LARGE_INTEGER freq;
	LARGE_INTEGER start;
    LARGE_INTEGER stop;

	int	duration;
}MyTimer;


//timer utilities
void  MyTimerStart( MyTimer *t );
float MyTimerStop( MyTimer *t );
float MyTimerDur(MyTimer *t);
MyTimer *MyTimerNew();
void  MyTimerDispose(MyTimer *t);
void  MyTimerReset(MyTimer *t);
	
#endif


// WWD-200711 (add four entry for deblocking for Intel's codes)
#define FUNC_TOTAL 64

enum
{
	ippsZero_8u_c_profile = 0,											
	ippsSet_8u_c_profile,											
	ippiCopy_8u_C1R_c_profile,										
	ippiSet_8u_C1R_c_profile,										
	ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c_profile,			
	ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c_profile,			
	ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_c_profile,		
	ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c_profile,		
	ippiInterpolateBlock_H264_8u_P2P1R_c_profile,					
	ippiInterpolateLuma_H264_8u_C1R_c_profile,						
	ippiInterpolateLuma_H264_8u_C1R_4x4_c_profile,						
	ippiInterpolateLuma_H264_8u_C1R_4x8_c_profile,						
	ippiInterpolateLuma_H264_8u_C1R_8x4_c_profile,						
	ippiInterpolateLuma_H264_8u_C1R_8x8_c_profile,						
	ippiInterpolateLuma_H264_8u_C1R_8x16_c_profile,						
	ippiInterpolateLuma_H264_8u_C1R_16x8_c_profile,						
	ippiInterpolateLuma_H264_8u_C1R_16x16_c_profile,						
	ippiInterpolateChroma_H264_8u_C1R_c_profile,						
	ippiInterpolateChroma_H264_8u_C1R_2x2_c_profile,						
	ippiDecodeExpGolombOne_H264_1u16s_c_profile,						
	ippiDecodeCAVLCCoeffs_H264_1u16s_c_profile,						
	ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c_profile,				
	ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_c_profile,	
	ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_c_profile,
	ippiInterpolateLumaTop_H264_8u_C1R_c_profile,					
	ippiInterpolateLumaBottom_H264_8u_C1R_c_profile,					
	ippiInterpolateChromaTop_H264_8u_C1R_c_profile,					
	ippiInterpolateChromaBottom_H264_8u_C1R_c_profile,				
	ippiInterpolateBlock_H264_8u_P3P1R_c_profile,					
	ippiUniDirWeightBlock_H264_8u_C1R_c_profile,						
	ippiBiDirWeightBlock_H264_8u_P2P1R_c_profile,					
	ippiBiDirWeightBlock_H264_8u_P3P1R_c_profile,					
	ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_c_profile,			
	ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_c_profile,			
	ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c_profile,		
	ippiReconstructChromaIntraMB_H264_16s8u_P2R_c_profile,			
	ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_c_profile,		
	ippiReconstructLumaIntraMB_H264_16s8u_C1R_c_profile,				
	ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_c_profile,			
	ippiReconstructLumaInterMB_H264_16s8u_C1R_c_profile,				
	ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_c_profile,		
	ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_c_profile,			
	ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_c_profile,		
	ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_c_profile,			
	ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_c_profile,			
	ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_c_profile,		
	ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_c_profile,			
	ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_c_profile,	
	ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_c_profile,		
	ippiReconstructChromaInterMB_H264_16s8u_P2R_c_profile,			
	ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_c_profile,  //50
	ipp_util_CopyBlockFromBottom_profile,	//51
	ipp_util_CopyBlockFromTop_profile,	//52

	ippiFilterDeblockingLuma,			//53
	ippiFilterDeblockingChroma,			//54

	ippiDCT8x8Inv_16s8u_C1R_arm_profile,	//for mp4 , 
	ippiDCT8x8Inv_16s8u_C1R_c_profile,
	ippiDCT8x8Inv_16s_C1I_arm_profile,
	ippiDCT8x8Inv_16s_C1I_c_profile,
	
	ippiDCT8x8Inv_4x4_16s_C1I_c_profile,
	ippiDCT8x8Inv_4x4_16s_C1I_arm_profile,
	ippiDCT8x8Inv_2x2_16s_C1I_c_profile,
	ippiDCT8x8Inv_2x2_16s_C1I_arm_profile,

	none_profile,  //63
};

typedef struct  
{
	int		count;
//	MyTimer timer;
}ProileRec;

//extern ProileRec prof_ary[FUNC_TOTAL];
#ifdef DO_PROFILE
void Profile_Init(void);
void Profile_Start(int func_idx);
void Profile_End(int func_idx);
void Profile_Print(char *);
#else
#define Profile_Init()
#define Profile_Start(a)
#define Profile_End(a)
#define Profile_Print(a)
#endif

void dump_data( unsigned char *data, int size, char *path );
void dump_yuv( long w, long h, long y_rowbytes, long uv_rowbytes, unsigned char *y, unsigned char *cb, unsigned char *cr, char *path );
void dump_bitstream_raw( unsigned char *in, long size );
void dump_bitstream_box( unsigned char *in, long size );
void dump_input( unsigned char *in, long size );
void dump_amr( unsigned char *in, long size );
void close_input_dump_file();

int diff_files( int width, int height, char *ref_path, char *src_path );
int same_files( char *ref_path, char *src_path );
int oln_to_box( char *path);

//sound
int  wav_create_for_write( char *filename, long  sampleRate, long  numChannels, FILE **inputFile );
int  wav_create_for_read(    char *filename, long *sampleRate, long *numChannels, long *sampleTotla, FILE **inputFile );
int  wav_read( FILE *soundFile, long numChannels, long sampleCount, short *soundDataPtr );
void wav_write( short *soundDataPtr, long numChannels, long sampleCount, FILE *soundFile );

int au_create_for_read(    char *filename, long *sampleRate, long *numChannels, long *sampleTotla, FILE **inputFile );
int au_read( FILE *soundFile, long numChannels, long sampleCount, short *soundDataPtr );

//raw bitstream, yuv
void dump_bitstream_raw( unsigned char *in, long size );
void dump_bitstream_box( unsigned char *in, long size );

//
void buf_printf(int is_interleaved, unsigned char* yPtr, int rowbytes, int height, int x, int y, const char* fmt, ...);
void test_bilinear_qpel(void);

int h263_box_to_flv(char *src_name );

int make_adts_header( int sample_rate, unsigned short channel_configuration, unsigned short aac_frame_length, unsigned char *header );

void nal2startcode( char *in_path, char *out_path );

#ifdef __cplusplus
}
#endif


#endif	//__KINOMA_UTILITIES_H__

