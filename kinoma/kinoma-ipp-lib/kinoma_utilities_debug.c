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
#include "kinoma_utilities.h"
#include "stdio.h"
#include "stdlib.h"

#ifndef BNIE_LOG
//void dlog(void *foo, ...) {}
#endif

#if defined( WIN32) || defined( _WIN32_WCE )
void  MyTimerStart( MyTimer *t )
{
	QueryPerformanceCounter(&(t)->start);
}

float MyTimerStop( MyTimer *t )
{						
	float delta;		
	QueryPerformanceCounter(&(t)->stop);
	delta = t->stop.QuadPart - t->start.QuadPart;
	t->duration += delta;

	return delta;
}

float MyTimerDur(MyTimer *t)
{
	float		v=0.0;
	//__int64	res;

	//res  = t->stop.QuadPart;
	//res -= t->start.QuadPart;

	v = (float)t->duration;
	v /= t->freq.QuadPart;
	//v *= 1.0e6f;

	return v;
}

MyTimer *MyTimerNew()
{   
	MyTimer *t = (MyTimer *)malloc(sizeof(MyTimer));
    
 	QueryPerformanceFrequency(&t->freq);

	t->duration = 0;
    
    return t; 
}

void  MyTimerDispose(MyTimer *t)
{
    if( t != NULL )
	    free(t);
}

void  MyTimerReset(MyTimer *t)
{
 	QueryPerformanceFrequency(&t->freq);
    t->duration = 0;
}
// following for testing every function's performance
#elif TARGET_OS_ANDROID

#include <time.h>

void  MyTimerStart( MyTimer *t )
{
	struct timeval tv;
	
	gettimeofday(&tv, NULL );
	t->start = tv.tv_sec*t->freq + tv.tv_usec;
	//dlog( "MyTimerStart: tv.tv_sec :%d, tv.tv_usec: %d, t->start: %d\n", (int)tv.tv_sec, (int)tv.tv_usec, (int)t->start);
}

float MyTimerStop( MyTimer *t )
{						
	float delta;		
	struct timeval tv;
	
	gettimeofday(&tv, NULL );
	t->stop = tv.tv_sec*t->freq + tv.tv_usec;
	//dlog( "MyTimerStop: tv.tv_sec :%d, tv.tv_usec: %d, t->start: %d\n", (int)tv.tv_sec, (int)tv.tv_usec, (int)t->stop);

	delta = (float)(t->stop - t->start);
	t->duration += delta;

	//dlog( "MyTimerStop: delta: %d, t->duration: %d\n", (int)delta, (int)t->duration);

	return delta;
}

float MyTimerDur(MyTimer *t)
{
	float		v=0.0;

	v = (float)t->duration;
	v /= t->freq;

	//dlog( "MyTimerStop: t->duration: %d, v: %d\n", (int)t->duration, (int)v );

	return v;
}

MyTimer *MyTimerNew()
{   
	MyTimer *t = (MyTimer *)malloc(sizeof(MyTimer));
    
	t->freq = 1000000;
	t->duration = 0;
    
    return t; 
}

void  MyTimerDispose(MyTimer *t)
{
    if( t != NULL )
	    free(t);
}

void  MyTimerReset(MyTimer *t)
{
 	//QueryPerformanceFrequency(&t->freq);
    t->duration = 0;
}

#endif

#ifdef DO_PROFILE
// following for testing every function's performance
const char func_names[FUNC_TOTAL][128] =
{
	"ippsZero_8u_c",											
	"ippsSet_8u_c",											
	"ippiCopy_8u_C1R_c",										
	"ippiSet_8u_C1R_c",										
	"ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_c",			
	"ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_c",			
	"ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_c",		
	"ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_c",		
	"ippiInterpolateBlock_H264_8u_P2P1R_c",					
	"ippiInterpolateLuma_H264_8u_C1R_c",
	"ippiInterpolateLuma_H264_8u_C1R_4x4_c",						
	"ippiInterpolateLuma_H264_8u_C1R_4x8_c",						
	"ippiInterpolateLuma_H264_8u_C1R_8x4_c",						
	"ippiInterpolateLuma_H264_8u_C1R_8x8_c",						
	"ippiInterpolateLuma_H264_8u_C1R_8x16_c",						
	"ippiInterpolateLuma_H264_8u_C1R_16x8_c",						
	"ippiInterpolateLuma_H264_8u_C1R_16x16_c",						
	"ippiInterpolateChroma_H264_8u_C1R_c",						
	"ippiInterpolateChroma_H264_8u_C1R_2x2_c",						
	"ippiDecodeExpGolombOne_H264_1u16s_c",						
	"ippiDecodeCAVLCCoeffs_H264_1u16s_c",						
	"ippiDecodeCAVLCChromaDcCoeffs_H264_1u16s_c",				
	"ippiFilterDeblockingLuma_VerEdge_MBAFF_H264_8u_C1IR_c",	
	"ippiFilterDeblockingChroma_VerEdge_MBAFF_H264_8u_C1IR_c",
	"ippiInterpolateLumaTop_H264_8u_C1R_c",					
	"ippiInterpolateLumaBottom_H264_8u_C1R_c",					
	"ippiInterpolateChromaTop_H264_8u_C1R_c",					
	"ippiInterpolateChromaBottom_H264_8u_C1R_c",				
	"ippiInterpolateBlock_H264_8u_P3P1R_c",					
	"ippiUniDirWeightBlock_H264_8u_C1R_c",						
	"ippiBiDirWeightBlock_H264_8u_P2P1R_c",					
	"ippiBiDirWeightBlock_H264_8u_P3P1R_c",					
	"ippiBiDirWeightBlockImplicit_H264_8u_P3P1R_c",			
	"ippiBiDirWeightBlockImplicit_H264_8u_P2P1R_c",			
	"ippiReconstructLumaIntra16x16MB_H264_16s8u_C1R_c",		
	"ippiReconstructChromaIntraMB_H264_16s8u_P2R_c",			
	"ippiReconstructChromaIntraHalfsMB_H264_16s8u_P2R_c",		
	"ippiReconstructLumaIntraMB_H264_16s8u_C1R_c",				
	"ippiReconstructLumaIntraHalfMB_H264_16s8u_C1R_c",			
	"ippiReconstructLumaInterMB_H264_16s8u_C1R_c",				
	"ippiReconstructLumaIntra_16x16MB_H264_16s8u_C1R_c",		
	"ippiReconstructLumaIntra8x8MB_H264_16s8u_C1R_c",			
	"ippiReconstructLumaIntraHalf8x8MB_H264_16s8u_C1R_c",		
	"ippiReconstructLumaInter8x8MB_H264_16s8u_C1R_c",			
	"ippiReconstructLumaIntra4x4MB_H264_16s8u_C1R_c",			
	"ippiReconstructLumaIntraHalf4x4MB_H264_16s8u_C1R_c",		
	"ippiReconstructLumaInter4x4MB_H264_16s8u_C1R_c",			
	"ippiReconstructChromaIntraHalfs4x4MB_H264_16s8u_P2R_c",	
	"ippiReconstructChromaInter4x4MB_H264_16s8u_P2R_c",		
	"ippiReconstructChromaInterMB_H264_16s8u_P2R_c",			
	"ippiReconstructChromaIntra4x4MB_H264_16s8u_P2R_c",
	"ipp_util_CopyBlockFromBottom_c",
	"ipp_util_CopyBlockFromTop_c",

	"ippiFilterDeblockingLuma",
	"ippiFilterDeblockingChroma",

	"ippiDCT8x8Inv_16s8u_C1R_arm",	//mp4 
	"ippiDCT8x8Inv_16s8u_C1R_c",
	"ippiDCT8x8Inv_16s_C1I_arm",
	"ippiDCT8x8Inv_16s_C1I_c",	

	"ippiDCT8x8Inv_4x4_16s_C1I_c",
	"ippiDCT8x8Inv_4x4_16s_C1I_arm",
	"ippiDCT8x8Inv_2x2_16s_C1I_c",
	"ippiDCT8x8Inv_2x2_16s_C1I_arm",

	"none"
};

ProileRec prof_ary[FUNC_TOTAL];
ProileRec prof_shadow_ary[FUNC_TOTAL];

void Profile_Init(void)
{
	int i;
	for (i = 0; i < FUNC_TOTAL; i++)
	{
		ProileRec *p;

		p = &prof_ary[i];
		p->count = 0;
		MyTimerReset( &p->timer );

		p = &prof_shadow_ary[i];
		p->count = 0;
		MyTimerReset( &p->timer );
	}
}

void Profile_Start(int func_idx)
{
	ProileRec *p = &prof_ary[func_idx];

	p->count++;
	MyTimerStart( &p->timer );
}

void Profile_End(int func_idx)
{
	ProileRec *p;
	
	p = &prof_ary[func_idx];

	MyTimerStop( &(p->timer) );

	p = &prof_shadow_ary[func_idx];
	p->count++;
	MyTimerStart( &p->timer );

	p = &prof_shadow_ary[func_idx];
	MyTimerStop( &(p->timer) );
}

void Profile_Print(char *filename)
{
	int i;
	ProileRec *ref = &prof_ary[none_profile];
	float ref_dur = MyTimerDur( &ref->timer )*1000;
	FILE *fp = NULL;

	if( filename !=NULL )
		fp = fopen(filename, "w");

	for (i = 0; i < FUNC_TOTAL; i++)
	{
		ProileRec *p = &prof_ary[i];

		if( p->count != 0 )
		{
			float dur = MyTimerDur( &p->timer )*1000;
			char  *name = func_names[i];
			ProileRec *p_s = &prof_shadow_ary[i];
			float dur_s	= MyTimerDur( &p_s->timer )*1000;

			dur -= dur_s;

			if( fp != NULL )
				fprintf(fp, "%s,    calls:%d,    Time: [%10.2f]ms\n", name, p->count,dur );
			else
				fprintf(stderr, "%s,    calls:%d/%d,    Time: [%10.2f]ms\n", name, p->count, p_s->count ,dur );

		}
	}

	if( fp != NULL )
		fclose(fp);
}
#endif

//file utilities
int diff_files( int width, int height, char *ref_path, char *src_path )
{
	FILE  *ref_file = NULL, *src_file = NULL;
	int frame_size = width * height *3 / 2;
	int	  ref_size = 0,  src_size = 0;
	unsigned char *ref_ptr = NULL, *src_ptr = NULL;
	int	  diff = 0, i;
	int count = 0;

	ref_file = fopen(ref_path, "rb");
	if( ref_file == NULL )
	{
		fprintf( stderr, "reference file not exists!!!\n");
		diff = -1;
		goto bail;
	}

	fseek(ref_file, 0, SEEK_END);
	ref_size = ftell(ref_file);
	fseek(ref_file, 0, SEEK_SET);

	src_file = fopen(src_path, "rb");
	if( src_file == NULL )
	{
		fprintf( stderr, "can't open source file!!! \n");
		diff = -1;
		goto bail;
	}

	fseek(src_file, 0, SEEK_END);
	src_size = ftell(src_file);
	fseek(src_file, 0, SEEK_SET);

	if( src_size != ref_size )
		fprintf( stderr, "source file size different from reference size, compare any way...\n");
	
	if( frame_size == 0 )
		frame_size = 1024*1024;

	ref_ptr = (unsigned char*)malloc( frame_size );
	src_ptr = (unsigned char*)malloc( frame_size );

	while(1)
	{
		int bytes_read;
		bytes_read = fread( (void *)ref_ptr, 1, frame_size,  ref_file );
		if( bytes_read < frame_size )
			break;
		bytes_read = fread( (void *)src_ptr, 1, frame_size,  src_file );
		if( bytes_read < frame_size )
			break;

		//fprintf( stderr, "\ncomparing frame: %d", count);
		for( i = 0; i < frame_size; i++ )
		{
			if( ref_ptr[i] != src_ptr[i] )
			{
				int size_y = height*width;
				int size_uv = size_y / 4;
				int x, y, idx; 

				fprintf( stderr, "\nXXX => difference found at #%d, offset: %d, %3d/%3d, ", count, i, ref_ptr[i], src_ptr[i]);
				if( i < size_y )
				{
					idx = i;
					x = idx%width;
					y = idx/width;
					fprintf( stderr, "Y, %3d,%3d", x, y);
				}
				else if( i >= size_y && i < size_y + size_uv )
				{
					idx = i - size_y;
					x = idx%(width/2);
					y = idx/(width/2);
					fprintf( stderr, "U, %3d,%3d", x, y); 
				}
				else
				{
					idx = i - size_y - size_uv;
					x = idx%(width/2);
					y = idx/(width/2);
					fprintf( stderr, "V, %3d,%3d", x, y); 
				}
				diff = -1;
				break;
			}
		}

		count++;
	}

bail:
	if( diff == 0 )
		fprintf( stderr, "\nVVV => frames match\n");

	if( ref_ptr != 0 )
		free( ref_ptr );

	if( src_ptr != 0 )
		free( src_ptr );

	if( ref_file != NULL )
		fclose( ref_file);

	if( src_file != NULL )
		fclose( src_file);
	
	return diff;
}


int same_files( char *ref_path, char *src_path )
{
	FILE  *ref_file = NULL, *src_file = NULL;
	int	  ref_size = 0,  src_size = 0;
	unsigned char *ref_ptr = NULL, *src_ptr = NULL;
	int	  same = 1, i;

	ref_file = fopen(ref_path, "rb");
	if( ref_file == NULL )
	{
		same = 0;
		goto bail;
	}

	fseek(ref_file, 0, SEEK_END);
	ref_size = ftell(ref_file);
	fseek(ref_file, 0, SEEK_SET);

	src_file = fopen(src_path, "rb");
	if( src_file == NULL )
	{
		fprintf( stderr, "can't open source file!!! \n");
		same = 0;
		goto bail;
	}

	fseek(src_file, 0, SEEK_END);
	src_size = ftell(src_file);
	fseek(src_file, 0, SEEK_SET);

	if( src_size != ref_size )
	{
		fprintf( stderr, "source file size different from reference size:%d::%d\n", src_size, ref_size );
		fprintf( stderr, "   compare any way...\n");
	}

	ref_ptr = (unsigned char*)malloc( ref_size );
	src_ptr = (unsigned char*)malloc( src_size );

	{
		int comp_size = ref_size < src_size ? ref_size : src_size;

		fread( (void *)ref_ptr, 1, ref_size,  ref_file );
		fread( (void *)src_ptr, 1, src_size,  src_file );

		for( i = 0; i < comp_size; i++ )
		{
			if( ref_ptr[i] != src_ptr[i] )
			{
				fprintf( stderr, "\nXXX => difference found at offset: %d\n", i);
				same = 0;
				break;
			}
		}
	}

bail:
	if( ref_ptr != 0 )
		free( ref_ptr );

	if( src_ptr != 0 )
		free( src_ptr );

	if( ref_file != NULL )
		fclose( ref_file);

	if( src_file != NULL )
		fclose( src_file);
	
	return same;
}

void Len2Startcode(  unsigned char *data, unsigned char len, short size )
{
	int frame_size;
	unsigned char *end = data + size;
	
	if( len != 4 )
		return;//***we are doomed
	
	while( data <= end - 3 )
	{
		frame_size = (data[0]<<24)|(data[1]<<16)|(data[2]<<8)|(data[3]);
		//IPP_Printf("    got a nalu with size: %d\n", frame_size);
		//IPP_Printf("    set startcode\n");
		data[0] = 0x00;
		data[1] = 0x00;
		data[2] = 0x00;
		data[3] = 0x01;
		
		data += 4 + frame_size;
	}
}


void nal2startcode( char *in_path, char *out_path )
{
	FILE *in_f  = NULL;
	FILE *out_f = NULL;
	int in_size  = 0;
	unsigned char *in_data		= NULL;
	unsigned char *in_data_cur  = NULL;
	unsigned char *in_data_end  = NULL;
	
	//IPP_Printf("opening : %s for read\n", in_path);
	in_f  = fopen(in_path,  "rb");
	if( in_f == NULL )
	{
		//IPP_Printf("open %s failed\n", in_path);
		return;
	}
	//IPP_Printf("opening : %s for write\n", out_path);
	out_f = fopen(out_path, "wb");
	if( out_f == NULL )
	{
		//IPP_Printf("open %s failed\n", out_path);
		return;
	}
	
	fseek(in_f, 0, SEEK_END);
	in_size = ftell(in_f);
	fseek(in_f, 0, SEEK_SET);
	//IPP_Printf("got input file size: %d\n", in_size);
	
	in_data     = (unsigned char *)malloc(in_size);
	if( in_data == NULL )
	{
		//IPP_Printf( "input data buffer allocation failed\n" );
		return;
	}
	
	in_data_cur = in_data;
	in_data_end = in_data + in_size;
	
	fread( in_data, 1, in_size, in_f );
	
	while( in_data <= in_data_end - 3 )
	{
		int				frame_size = (in_data[0]<<0)|(in_data[1]<<8)|(in_data[2]<<16)|(in_data[3]<<24);
		unsigned char	*frame_data = in_data + 4;
		
		//IPP_Printf("got a frame with size: %d\n", frame_size);
		//IPP_Printf("set startcode for all nalu in the frame\n");
		Len2Startcode(  frame_data, 4, frame_size );
		fwrite( frame_data, 1, frame_size, out_f );
		in_data += 4 + frame_size;
	}
	
	//IPP_Printf("cleaning up\n");
	free( in_data );
	fclose( in_f );
	fclose( out_f );
}




void qtm4v2rawm4v( char *in_path, char *out_path )
{
	FILE *in_f  = NULL;
	FILE *out_f = NULL;
	int in_size  = 0;
	unsigned char *in_data		= NULL;
	unsigned char *in_data_cur  = NULL;
	unsigned char *in_data_end  = NULL;
	
	//IPP_Printf("opening : %s for read\n", in_path);
	in_f  = fopen(in_path,  "rb");
	if( in_f == NULL )
	{
		//IPP_Printf("open %s failed\n", in_path);
		return;
	}
	//IPP_Printf("opening : %s for write\n", out_path);
	out_f = fopen(out_path, "wb");
	if( out_f == NULL )
	{
		//IPP_Printf("open %s failed\n", out_path);
		return;
	}
	
	fseek(in_f, 0, SEEK_END);
	in_size = ftell(in_f);
	fseek(in_f, 0, SEEK_SET);
	//IPP_Printf("got input file size: %d\n", in_size);
	
	in_data     = (unsigned char *)malloc(in_size);
	if( in_data == NULL )
	{
		//IPP_Printf( "input data buffer allocation failed\n" );
		return;
	}
	
	in_data_cur = in_data;
	in_data_end = in_data + in_size;
	
	fread( in_data, 1, in_size, in_f );
	
	while( in_data <= in_data_end - 3 )
	{
		int				frame_size = (in_data[0]<<0)|(in_data[1]<<8)|(in_data[2]<<16)|(in_data[3]<<24);
		unsigned char	*frame_data = in_data + 4;
		
		//IPP_Printf("got a frame with size: %d\n", frame_size);
		//IPP_Printf("set startcode for all nalu in the frame\n");
		fwrite( frame_data, 1, frame_size, out_f );
		in_data += 4 + frame_size;
	}
	
	//IPP_Printf("cleaning up\n");
	free( in_data );
	fclose( in_f );
	fclose( out_f );
}


void StartcodeToStartlen( unsigned char *avc_bytes, long size )
{
	long offsetAry[256];
	long i, idx = 0;
	
	for( i = 0; i < size-3; i++ )
	{
		if( avc_bytes[i]   == 0 && 
			avc_bytes[i+1] == 0 &&
			avc_bytes[i+2] == 0 &&
			avc_bytes[i+3] == 1
			)
		{
			offsetAry[idx] = i;
			idx++;
		}
	}

	for( i = 0; i < idx; i++ )
	{
		long thisOffset = offsetAry[i];
		long thisSize;

		if( i == idx - 1 )
			thisSize = size - thisOffset;
		else
			thisSize = offsetAry[i+1] - thisOffset;

		thisSize -= 4;

		// WWD change it to (unsigned char) in 2006-04-10
		avc_bytes[thisOffset+0] = (unsigned char)(thisSize>>24)&0x000000ff;
		avc_bytes[thisOffset+1] = (unsigned char)(thisSize>>16)&0x000000ff;
		avc_bytes[thisOffset+2] = (unsigned char)(thisSize>>8 )&0x000000ff;
		avc_bytes[thisOffset+3] = (unsigned char)(thisSize>>0 )&0x000000ff;
	}

}


//assuming it's always LE
#define LongAryB2N(a) ( a[0]<<24|a[1]<<16|a[2]<<8|a[3] )
int oln_to_box(char *path )
{
	char		*src_avc_path	= NULL;
	FILE		*src_avc		= NULL;
	long		src_avc_size;

	char		src_oln_path[256];
	FILE		*src_oln		= NULL;
	long		src_oln_size;

	char		dst_box_path[256];
	FILE		*dst_box		= NULL;

	unsigned char oln_bytes[4];
	unsigned char *avc_bytes = NULL;
	long signature = 'bnie';
	long idx, totalSamples;
	long offset, size;

	//src_avc
	src_avc_path = path;
	src_avc = fopen(path, "rb");
	if( src_avc == NULL )
		return -1;

	fseek(src_avc, 0, SEEK_END);
	src_avc_size = ftell(src_avc);
	fseek(src_avc, 0, SEEK_SET);

	//src_oln
	strcpy( src_oln_path, src_avc_path );
	strcat( src_oln_path, ".oln" );
	src_oln = fopen(src_oln_path, "rb");

	if( src_oln == NULL )
		return -1;

	fseek(src_oln, 0, SEEK_END);
	src_oln_size = ftell(src_oln);
	fseek(src_oln, 0, SEEK_SET);
	
	//dst_yuv
	strcpy( dst_box_path, src_avc_path );
	strcat( dst_box_path, ".box" );
	dst_box = fopen(dst_box_path, "wb");
	if( src_oln == NULL )
		return -1;

	avc_bytes = (unsigned char *)malloc(1024*1024*5);

	fread( &oln_bytes, 1, 4, src_oln );
	if( signature != LongAryB2N(oln_bytes) )
		return -1;
	
	fread( oln_bytes, 1, 4, src_oln );
	totalSamples = LongAryB2N(oln_bytes);

	fread( oln_bytes, 1, 4, src_oln );
	offset = LongAryB2N(oln_bytes);

	fread( oln_bytes, 1, 4, src_oln );
	size = LongAryB2N(oln_bytes);

	fseek(src_avc, offset, SEEK_SET);
	fread( avc_bytes, 1, size, src_avc );

	fwrite( &size, 1, 4, dst_box );
	StartcodeToStartlen( avc_bytes, size );
	fwrite( avc_bytes, 1, size, dst_box );

	for( idx = 0; idx < totalSamples; idx++ )			// totalSamples
	{	
		fread( oln_bytes, 1, 4, src_oln );
		offset = LongAryB2N(oln_bytes);

		fread( oln_bytes, 1, 4, src_oln );
		size = LongAryB2N(oln_bytes);

		fseek(src_avc, offset, SEEK_SET);
		fread( avc_bytes, 1, size, src_avc );

		fwrite( &size, 1, 4, dst_box );
		StartcodeToStartlen( avc_bytes, size );
		fwrite( avc_bytes, 1, size, dst_box );
	}
	
	if( src_avc != NULL )
		fclose( src_avc	);

	if( src_avc != NULL )
		fclose( src_avc	);

	if( dst_box != NULL )
		fclose( dst_box	);

	if( avc_bytes != NULL )
		free( avc_bytes );

	return 0;
}


int fix_line_break(char *path )
{
	char		*src_path = NULL;
	char		dst_path[256];
	FILE		*src = NULL, *dst = NULL;
	long		src_size;
	long		src_idx = 0, dst_idx = 0;
	unsigned char *src_bytes = NULL, *dst_bytes = NULL;

	//src
	src_path = path;
	src = fopen(path, "rb");
	if( src == NULL )
		return -1;

	fseek(src, 0, SEEK_END);
	src_size = ftell(src);
	fseek(src, 0, SEEK_SET);
	
	//dst
	strcpy( dst_path, src_path );
	strcat( dst_path, ".converted" );
	dst = fopen(dst_path, "wb");
	if( dst == NULL )
		return -1;

	src_bytes = (unsigned char *)malloc(src_size);
	dst_bytes = (unsigned char *)malloc(src_size);
	fread( src_bytes, 1, src_size, src );

	while( src_idx < src_size )
	{	
		if( src_bytes[src_idx] == 0x0d && src_bytes[src_idx+1] == 0x0a )
		{
			dst_bytes[dst_idx] = 0x0a;
			src_idx++;
		}
		else
			dst_bytes[dst_idx] = src_bytes[dst_idx];

		src_idx++;
		dst_idx++;
	}
	
	fwrite( dst_bytes, 1, dst_idx, dst );

	if( src != NULL )
		fclose( src	);

	if( dst != NULL )
		fclose( dst	);

	if( src_bytes != NULL )
		free( src_bytes );

	return 0;
}

//sound utilities
typedef struct 
{
	long			maxFileOffset;
	unsigned char*	bufferPtr;
} BitBuffer;

void bb_init(BitBuffer *bb, long size, unsigned char *data )
{
	bb->maxFileOffset	= size;
	bb->bufferPtr		= data;
}

#define PopBits(value, numBits)										\
{																	\
	unsigned long byteOffset;										\
	unsigned long tmpUINT;											\
																	\
	value=( bits32>>(32-(numBits)) );								\
																	\
	if(numBits==0) value=0;											\
																	\
	tmpUINT    =bitOffset+(numBits);								\
	byteOffset =(tmpUINT>>3); bitOffset=(tmpUINT&0x7);				\
	ptr		  +=byteOffset;											\
	bits32     =(ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|(ptr[3]<<0);	\
	bits32	 <<=bitOffset;											\
}

int next_start_code(  unsigned char *p, long *offset, long *frame_size)
{
	long   ID;
	long   Layer;
	long   protection_absent;
	long   Profile;
	long   sampling_frequency_index;
	long   private_bit;
	long   channel_configuration;
	long   original_copy;
	long   Home;
	// long   Emphasis;
	long	copyright_identification_bit;
	long	copyright_identification_start;
	long	aac_frame_length;
	long	adts_buffer_fullness;
	long	no_raw_data_blocks_in_frame;

	unsigned char*  ptr       = p;
	unsigned long   bits32    = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|(p[3]<<0);															\
	unsigned long   bitOffset = 0;
	unsigned long	tmp;
	
	PopBits(tmp,12);
	if (tmp != 0xfff)
	{
		printf("bad adts syncword");
		return 1;
	}

	//ADTS fixed header
	PopBits(ID,1);
	PopBits(Layer,2);
	PopBits(protection_absent,1);
	PopBits(Profile,2);
	PopBits(sampling_frequency_index,4);
	PopBits(private_bit,1);
	PopBits(channel_configuration,3);
	PopBits(original_copy,1);
	PopBits(Home,1);

	//ADTS variable header
	PopBits(copyright_identification_bit,1);
	PopBits(copyright_identification_start,1);
	PopBits(aac_frame_length,13);
	PopBits(adts_buffer_fullness,11);
	PopBits(no_raw_data_blocks_in_frame,2);
	if (protection_absent == 0)
	{
		PopBits(tmp,16);			//crc_check 16 only if protection_absent == 0 

		*frame_size = aac_frame_length - 9;
		*offset = 9;
	}
	else
	{
		*frame_size = aac_frame_length - 7;
		*offset = 7;
	}
	return 0;
}

void ParseADTSHeader( unsigned char *p, long *sizeOut, unsigned char **pOut, long *offset,  long *frame_size)
{
	long   ID;
	long   Layer;
	long   protection_absent;
	long   Profile;
	long   sampling_frequency_index;
	long   private_bit;
	long   channel_configuration;
	long   original_copy;
	long   Home;
	long	copyright_identification_bit;
	long	copyright_identification_start;
	long	aac_frame_length;
	long	adts_buffer_fullness;
	long	no_raw_data_blocks_in_frame;

	unsigned char*  ptr       = p;
	unsigned long   bits32    = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|(p[3]<<0);															
	unsigned long   bitOffset = 0;
	unsigned long	tmp;

	PopBits(tmp,12);
	//added by CSQ, check syncword if 'FFF'
	if (tmp != 0xfff)
	{
		printf("not adts syncword");
		return;
	}
	//added end

	//ADTS fixed header
	PopBits(ID,1);
	PopBits(Layer,2);
	PopBits(protection_absent,1);
	PopBits(Profile,2);
	PopBits(sampling_frequency_index,4);
	PopBits(private_bit,1);
	PopBits(channel_configuration,3);
	PopBits(original_copy,1);
	PopBits(Home,1);

	//ADTS variable header
	PopBits(copyright_identification_bit,1);
	PopBits(copyright_identification_start,1);
	PopBits(aac_frame_length,13);
	PopBits(adts_buffer_fullness,11);
	PopBits(no_raw_data_blocks_in_frame,2);

	//added by CSQ
	if (protection_absent == 0)
	{
		PopBits(tmp,16);			//crc_check 16 only if protection_absent == 0 

		*frame_size = aac_frame_length - 9;
		*offset = 9;
	}
	else
	{
		*frame_size = aac_frame_length - 7;
		*offset = 7;
	}
	//added end 

	{
		unsigned char	*p, *p0;
		long			size = 0;
		//int				err = 0;
		
		size     = 43;
		*sizeOut = 0;
		*pOut	 = NULL;

		p0 = (unsigned char *)malloc(size);
		if(p0 == NULL) 
			goto bail;
		
		p = p0;

		*p = 0;					p++;		//version
		*p = 0;					p++;		//flags
		*p = 0;					p++;		
		*p = 0;					p++;		

		//esd starts here
		*p = 0x03;				p++;		//tag
		*p = 0x80;				p++;		//size
		*p = 0x80;				p++;		//size
		*p = 0x80;				p++;		//size
		*p = 0x22;				p++;		//size

		*p = 0;					p++;		//ES_ID
		*p = 0;					p++;		
		*p = 0;					p++;		//flags&priority: 

		//decoder config descriptor
		*p = 0x04;				p++;		//object type
		*p = 0x80;				p++;		//size
		*p = 0x80;				p++;		//size
		*p = 0x80;				p++;		//size
		*p = 0x14;				p++;		//size

			*p = 0x40;			p++;		//objectTypeId
			*p = 0x15;			p++;		//streamType

			*p = 0;				p++;		//bufferSizeDB, 24 bits: 0x1800
			*p = 0x18;			p++;		
			*p = 0;				p++;		

			*p = 0;				p++;		//maxBitRate, 32 bits:
			*p = 0x01;			p++;		
			*p = 0xf4;			p++;		
			*p = 0;				p++;		

			*p = 0;				p++;		//avgBitRate, 32 bits:
			*p = 0x01;			p++;		
			*p = 0xf4;			p++;		
			*p = 0;				p++;		

			//decoder specific info
			*p = 0x05;			p++;		//tag
			*p = 0x80;			p++;		//size
			*p = 0x80;			p++;		//size
			*p = 0x80;			p++;		//size
			*p = 0x02;			p++;		//size

			*p = 0x10 |(sampling_frequency_index>>1);
			p++;//0x12;			p++;		//decoderConfig
			*p = ((sampling_frequency_index&0x01)<<7)| ((channel_configuration&0x0f)<<3);
			p++;//0x10;			p++;

			//decoder specific info
			*p = 0x06;			p++;		//tag
			*p = 0x80;			p++;		//size
			*p = 0x80;			p++;		//size
			*p = 0x80;			p++;		//size
			*p = 0x01;			p++;		//size
			*p = 0x02;			p++;		//predefined: use timestamps from mp4 stbl

		//no IPI_DescrPointer
		//no IP_IdentificationDataSet
		//no IPMP_DescriptorPointer
		//no LanguageDescriptor
		//no QoS_Descriptor
		//no RegistrationDescriptor
		//no ExtensionDescriptor

		*sizeOut = size;
		*pOut	 = p0;
	}

bail:
	;
}

int convert_adts_to_box( FILE *in, FILE *out )
{ 
	BitBuffer	bb;
	long 		err = 0;
	long		hasESDS = 0;
	long		offset = 0;
	long		frameSize;
	int			framecount = 0;

	unsigned char *data;
	{//read in all bitstream data
		long size;

		fseek(in, 0, SEEK_END);
		size = ftell(in);
		fseek( in, 0, SEEK_SET);
		data = (unsigned char*) malloc(size);
		if (data == NULL) 
		{
			err = -1;
			goto bail;
		}
	
		fread(data, 1, size, in );
		bb_init(&bb, size, data);
	}

	while(1) 
	{
		if (!hasESDS) 
		{
			unsigned char *esds;
			long esdsSize = 7;
			long signature = 'esds';
			
			hasESDS = 1;

			ParseADTSHeader( bb.bufferPtr, &esdsSize, &esds, &offset, &frameSize );

			fwrite(&signature, 4, 1, out);
			fwrite(&esdsSize,  4, 1, out);
			fwrite(esds, esdsSize, 1, out );
			free(esds);

		}
			
		{
			fwrite(&frameSize,  4, 1, out);
			bb.bufferPtr += offset;
			fwrite(bb.bufferPtr, frameSize, 1, out );
			bb.bufferPtr += frameSize;
			framecount++;
		}
		if (bb.bufferPtr - data < bb.maxFileOffset)
		{
			if(next_start_code(bb.bufferPtr,&offset,&frameSize))
				break;
		}
		else
		{
			break;
		}
	}

	free(data);

bail:
	return err;
}

#define ShortSwapEndian( s )  ( ( s&0x00ff )<<8 | ( ( s&0xff00 )>> 8 ) )
#define LongSwapEndian( s )  (  ( s&0x000000ff )<<24 | ( s&0x0000ff00 )<<8 | ( s&0x00ff0000 )>>8 | ( s&0xff000000 )>>24 )

#ifdef _BIG_ENDIAN_ 
	#define ShortN2L(s)  ShortSwapEndian(s)
	#define LongN2L(s)   LongSwapEndian(s)
	#define LongL2N(s)   LongSwapEndian(s)
	#define ShortL2N(s)  ShortSwapEndian(s)
	#define LongN2B(s)   (s)
	#define LongB2N(a)   (s)
	#define ShortB2N(s)  (s)
#else
	#define ShortN2L(s)  (s)
	#define LongN2L(s)   (s)
	#define LongL2N(s)   (s)
	#define ShortL2N(s)  (s)
	#define LongN2B(s)   LongSwapEndian(s)
	#define LongB2N(s)   LongSwapEndian(s)
	#define ShortB2N(s)  ShortSwapEndian(s)
#endif	

#define WRITE_LE_LONG( v, d )   {								\
									d[0]	= (v>>0)&0x000000ff;	\
									d[1]	= (v>>8)&0x000000ff;	\
									d[2]	= (v>>16)&0x000000ff;	\
									d[3]	= (v>>24)&0x000000ff;	\
								}

#define WRITE_LE_SHORT( v, d )  {								\
									d[0]	= (v>>0)&0x00ff;	\
									d[1]	= (v>>8)&0x00ff;	\
								}

int rgb_to_bmp( int pix_bytes, int width, int height, unsigned char *rgb_data, int rgb_size, char *bmp_file_path, char *ref_bmp_path, int *diff )
{
	unsigned char *bmp_data   = NULL;
	long		  bmp_size;
	unsigned char *dst	  = NULL;
	unsigned char *src	  = NULL;
	long		  headerSize = 26;
	long		  outputRowBytes;
	int			  err = 0;
	//unsigned char srcStr[64];
	//long		  srcStrLen;
	
	FILE		 *bmp_file = NULL;
	
	bmp_file = fopen(bmp_file_path, "wb");
	if( bmp_file == NULL )
	{
		//dlog( "failed to create dst file for write: %s\n", bmp_file_path );
		err = -10;
		goto bail;
	}
	
	src = rgb_data;

	outputRowBytes = width * 3;
	if (outputRowBytes % 4)
		outputRowBytes += 4 - (outputRowBytes % 4);

	bmp_size = headerSize + (height * outputRowBytes);
	bmp_data = malloc(bmp_size);
	if (bmp_data == NULL)
	{ 
		err = -1;
		goto bail;
	}

	memset((void *)bmp_data, 0, bmp_size );
	dst  = bmp_data;

	//imageFileType
	dst[0] 	= 'B';
	dst[1] 	= 'M';
	dst	   += 2;

	//fileSize
	WRITE_LE_LONG( bmp_size, dst )
	dst += 4;

	//reserved
	WRITE_LE_LONG( 0, dst )
	dst += 4;

	//imageDataOffset
	WRITE_LE_LONG( headerSize, dst )
	dst += 4;
	
	//biSize
	WRITE_LE_LONG( 12, dst )
	dst += 4;

	//width
	WRITE_LE_SHORT( width, dst )
	dst += 2;

	//height
	WRITE_LE_SHORT( height, dst )
	dst += 2;

	//biPlanes
	WRITE_LE_SHORT( 1, dst )
	dst += 2;

	//biBitCount
	WRITE_LE_SHORT( 24, dst )
	dst += 2;
	
	{
		long x, y;

		dst += (height-1)*outputRowBytes;
		for (y = 0; y < height; y++) 
		{
			for (x = 0; x < width; x++) 
			{
				unsigned char r, g, b;

				if( pix_bytes == 2 )
				{
					short this_rgb = (src[1]<<8)|(src[0]);
					
					r = (this_rgb&0xf800)>>8;
					g = (this_rgb&0x07e0)>>3;
					b = (this_rgb&0x001f)<<3;
				}
				else
				{
					b = src[0];
					g = src[1];
					r = src[2];
				}

				dst[0] = b;
				dst[1] = g;
				dst[2] = r;
				src += pix_bytes;
				dst += 3;
			}

			dst -= width * 3;
			dst -= outputRowBytes;
		}
	}

	fwrite( bmp_data, bmp_size, 1, bmp_file );
	fclose( bmp_file );

	if( ref_bmp_path != NULL )
	{
		FILE *ref_bmp_file;
		int	  ref_bmp_size = bmp_size;
		unsigned char *ref_bmp_ptr, *ref_bmp_ptr0;
		unsigned char *this_bmp_ptr = bmp_data;
		int	 i;

		ref_bmp_file = fopen(ref_bmp_path, "rb");
		if( ref_bmp_file == NULL )
		{
			//dlog( "reference file not exists!!!\n");
			*diff = 1;
			return -11;
		}

		ref_bmp_ptr0 = malloc( ref_bmp_size );
		if( ref_bmp_ptr0 == NULL )
			return -1;

		ref_bmp_ptr = ref_bmp_ptr0;

		fread( (void *)ref_bmp_ptr, ref_bmp_size, 1,  ref_bmp_file );

		for( i = 0; i < ref_bmp_size; i++ )
		{
			if( *ref_bmp_ptr != *this_bmp_ptr )
			{
				//dlog( "difference found!!!\n");
				*diff = 1;
				break;
			}
			
			ref_bmp_ptr++;
			this_bmp_ptr++;

		}

		free( ref_bmp_ptr0 );
		fclose( ref_bmp_file);
	}
	else 
		*diff = -1;

bail:
	if( bmp_data != NULL )
		free( bmp_data );

	return err;
}




int wav_create_for_write( char *filename, long  sampleRate, long  numChannels, FILE **inputFile )
{
	FILE	*soundFile = *inputFile;
	
	if( inputFile == NULL )
		return -1;

	soundFile = fopen(filename,"wb");	
	*inputFile = soundFile;
	if( soundFile == NULL )
	{
		printf("cannot open dump file for write (%s)\n", filename );
		return -1;
	}	
	{
		long	fourBytes;
		short	twoBytes;
		
		fourBytes = LongN2B('RIFF');
		fwrite( &fourBytes,  4, 1, soundFile);

		fourBytes = 1024*1024*10;							//total size not known yet
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);

		fourBytes = LongN2B('WAVE');		
		fwrite( &fourBytes, 4, 1, soundFile);

		fourBytes = LongN2B('fmt ');		
		fwrite( &fourBytes, 4, 1, soundFile);

		fourBytes = 16;		
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);

		twoBytes = 1;										//compression mode, 1 PCM/uncompressed
		twoBytes = ShortN2L( twoBytes );		
		fwrite( &twoBytes, 2, 1, soundFile);

		twoBytes = numChannels;								//	number of channels
		twoBytes = ShortN2L( twoBytes );		
		fwrite( &twoBytes, 2, 1, soundFile);

		fourBytes = sampleRate;								//sample rate
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);
		
		fourBytes = sampleRate * 2 * numChannels;			//average bytes per second
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);
		
		twoBytes = numChannels * 2;							//	block align
		twoBytes = ShortN2L( twoBytes );		
		fwrite( &twoBytes, 2, 1, soundFile);

		twoBytes = 16;										//	significant bits per sample
		twoBytes = ShortN2L( twoBytes );		
		fwrite( &twoBytes, 2, 1, soundFile);

		fourBytes = LongN2B('data');
		fwrite( &fourBytes,  4, 1, soundFile);

		fourBytes = 1024*1024*50;							//total size not known yet
		fourBytes = LongN2L(fourBytes);
		fwrite( &fourBytes, 4, 1, soundFile);
	}
	
	fflush( soundFile );

	return 0;
}




int au_create_for_read(    char *filename, long *sampleRate, long *numChannels, long *sampleTotla, FILE **inputFile )
{
	FILE	*soundFile = *inputFile;
	int		err = 0;
	int		offset = 0;
	int		size   = 0;
	int		format   = 0;
	int		bytesTotal;
	
	if( filename == NULL )
		return -1;

	soundFile = fopen( filename, "rb" );	
	*inputFile = soundFile;
	if( soundFile == NULL )
	{
		printf("cannot open dump file for read (%s)\n", filename );
		return -1;
	}
	
	*sampleTotla = 0;
	fseek( soundFile, 0, SEEK_END );
	bytesTotal = ftell( soundFile );
	fseek( soundFile, 0, SEEK_SET );

	{
		long	fourBytes;
		
		fread( &fourBytes,  4, 1, soundFile);
		if( fourBytes != LongN2B('.snd'))
			return -1;

		fread( &fourBytes, 4, 1, soundFile);
		offset = LongB2N(fourBytes);//size

		fread( &fourBytes, 4, 1, soundFile);
		size = LongB2N(fourBytes);//size

		fread( &fourBytes, 4, 1, soundFile);
		format = LongB2N(fourBytes);//size

		if( format != 3 )
			fprintf( stderr, "format we can't handle!!!\n");

		fread( &fourBytes, 4, 1, soundFile);			//size should be 16
		fourBytes = LongB2N(fourBytes);
		*sampleRate = fourBytes;
		
		fread( &fourBytes, 4, 1, soundFile);				//	number of channels
		fourBytes = LongB2N(fourBytes);
		*numChannels = fourBytes;
	}

	if( sampleTotla != NULL )
		*sampleTotla = ( bytesTotal - 24 ) / ( 2 * (*numChannels) );

	return err;
}

int au_read( FILE *soundFile, long numChannels, long sampleCount, short *soundDataPtr )
{
	int i;
	
	if( soundFile == NULL )
		return 0;
	
	for( i = 0; i < sampleCount; i++ )
	{
		short twoBytes;
		short thisTotal;
		
		thisTotal = fread( &twoBytes, 2, 1, soundFile);
		if( thisTotal == 0 )
		{
			printf( "reaching end of file\n" );
			break;
		}
			
		twoBytes = ShortB2N( twoBytes );
		*soundDataPtr = twoBytes;
		soundDataPtr++;
		
		if( numChannels == 2 )
		{
			thisTotal = fread( &twoBytes, 2, 1, soundFile);
			if( thisTotal == 0 )
			{
				printf( "reaching end of file\n" );
				break;
			}
				
			twoBytes = ShortB2N( twoBytes );
			*soundDataPtr = twoBytes;
			soundDataPtr++;
		}
	}
	
	return i;
}




int wav_create_for_read(    char *filename, long *sampleRate, long *numChannels, long *sampleTotla, FILE **inputFile )
{
	FILE	*soundFile = *inputFile;
	int		err = 0;
	int		bytesTotal;
	
	if( filename == NULL )
		return -1;

	soundFile = fopen( filename, "rb" );	
	*inputFile = soundFile;
	if( soundFile == NULL )
	{
		printf("cannot open dump file for read (%s)\n", filename );
		return -1;
	}
	
	*sampleTotla = 0;
	fseek( soundFile, 0, SEEK_END );
	bytesTotal = ftell( soundFile );
	fseek( soundFile, 0, SEEK_SET );

	{
		long	fourBytes;
		short	twoBytes;
		
		fread( &fourBytes,  4, 1, soundFile);
		if( fourBytes != LongN2B('RIFF'))
			return -1;

		fread( &fourBytes, 4, 1, soundFile);
		fourBytes = LongL2N(fourBytes);//size

		fread( &fourBytes, 4, 1, soundFile);
		if( fourBytes != LongN2B('WAVE'))
			return -1;

		fread( &fourBytes, 4, 1, soundFile);
		if( fourBytes != LongN2B('fmt ') )
			return -1;

		fread( &fourBytes, 4, 1, soundFile);			//size should be 16
		fourBytes = LongL2N(fourBytes);
		if( fourBytes != 16 )
			return -1;

		fread( &twoBytes, 2, 1, soundFile);				//compression mode, 1 PCM/uncompressed
		twoBytes = ShortL2N(twoBytes);
		if( twoBytes != 1 )
			return -1;
		
		fread( &twoBytes, 2, 1, soundFile);				//	number of channels
		twoBytes = ShortL2N(twoBytes);
		*numChannels = twoBytes;

		fread( &fourBytes, 4, 1, soundFile);			//size should be 16
		fourBytes = LongL2N(fourBytes);
		*sampleRate = fourBytes;

		fread( &fourBytes, 4, 1, soundFile);			//average bytes per second
		fourBytes = LongL2N(fourBytes);

		fread( &twoBytes, 2, 1, soundFile);				//	block alig
		twoBytes = ShortL2N(twoBytes);

		fread( &twoBytes, 2, 1, soundFile);				//	block alig
		twoBytes = ShortL2N(twoBytes);

		fread( &fourBytes, 4, 1, soundFile);
		if( fourBytes != LongN2B('data') )
			return -1;

		fread( &fourBytes, 4, 1, soundFile);			//chunck size
		fourBytes = LongL2N(fourBytes);
	}

	if( sampleTotla != NULL )
		*sampleTotla = ( bytesTotal - 44 ) / ( 2 * (*numChannels) );

	return err;
}

int wav_read( FILE *soundFile, long numChannels, long sampleCount, short *soundDataPtr )
{
	int i;
	
	if( soundFile == NULL )
		return 0;
	
	for( i = 0; i < sampleCount; i++ )
	{
		short twoBytes;
		short thisTotal;
		
		thisTotal = fread( &twoBytes, 2, 1, soundFile);
		if( thisTotal == 0 )
		{
			printf( "reaching end of file\n" );
			break;
		}
			
		twoBytes = ShortN2L( twoBytes );
		*soundDataPtr = twoBytes;
		soundDataPtr++;
		
		if( numChannels == 2 )
		{
			thisTotal = fread( &twoBytes, 2, 1, soundFile);
			if( thisTotal == 0 )
			{
				printf( "reaching end of file\n" );
				break;
			}
				
			twoBytes = ShortN2L( twoBytes );
			*soundDataPtr = twoBytes;
			soundDataPtr++;
		}
	}
	
	return i;
}




void wav_write( short *soundDataPtr, long numChannels, long sampleCount, FILE *soundFile )
{
	int i;
	
	if( soundFile == NULL )
		return;
	
	for( i = 0; i < sampleCount; i++ )
	{
		short twoBytes;
		
		twoBytes = *soundDataPtr;
		twoBytes = ShortN2L( twoBytes );
		fwrite( &twoBytes, 2, 1, soundFile);
		soundDataPtr++;
		
		if( numChannels == 2 )
		{
			twoBytes = *soundDataPtr;
			twoBytes = ShortN2L( twoBytes );
			fwrite( &twoBytes, 2, 1, soundFile);
			soundDataPtr++;
		}
	}
	
	fflush( soundFile );
}


#include <string.h>
#include <stdarg.h>

/*
//keep the code here in case we want to fix the font 
long fontLong[1248] =
{
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00100000, 0x01010000, 0x00000000, 0x00100000, 0x01001000, 0x00000000, 0x00100000,
	0x00000000, 0x00100000, 0x01010000, 0x01010000, 0x01111000, 0x10101000, 0x01000000, 0x00100000,
	0x00000000, 0x00100000, 0x01010000, 0x01010000, 0x10100000, 0x01010000, 0x10100000, 0x00100000,
	0x00000000, 0x00100000, 0x00000000, 0x11111000, 0x10100000, 0x00010000, 0x10100000, 0x00000000,
	0x00000000, 0x00100000, 0x00000000, 0x01010000, 0x01110000, 0x00100000, 0x01000000, 0x00000000,
	0x00000000, 0x00100000, 0x00000000, 0x11111000, 0x00101000, 0x01000000, 0x10100000, 0x00000000,
	0x00000000, 0x00100000, 0x00000000, 0x01010000, 0x00101000, 0x01010000, 0x10011000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x01010000, 0x11110000, 0x10101000, 0x10010000, 0x00000000,
	0x00000000, 0x00100000, 0x00000000, 0x00000000, 0x00100000, 0x10010000, 0x01101000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00010000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00100000, 0x00100000, 0x00100000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00001000,
	0x00100000, 0x00100000, 0x10101000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00001000,
	0x01000000, 0x00010000, 0x01110000, 0x00100000, 0x00000000, 0x00000000, 0x00000000, 0x00010000,
	0x01000000, 0x00010000, 0x10101000, 0x00100000, 0x00000000, 0x00000000, 0x00000000, 0x00010000,
	0x01000000, 0x00010000, 0x00100000, 0x11111000, 0x00000000, 0x11111000, 0x00000000, 0x00100000,
	0x01000000, 0x00010000, 0x00000000, 0x00100000, 0x00000000, 0x00000000, 0x00000000, 0x01000000,
	0x01000000, 0x00010000, 0x00000000, 0x00100000, 0x00000000, 0x00000000, 0x00000000, 0x01000000,
	0x00100000, 0x00100000, 0x00000000, 0x00000000, 0x00110000, 0x00000000, 0x00100000, 0x10000000,
	0x00100000, 0x00100000, 0x00000000, 0x00000000, 0x00100000, 0x00000000, 0x01110000, 0x10000000,
	0x00010000, 0x01000000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x00100000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00100000, 0x00100000, 0x01110000, 0x11111000, 0x00010000, 0x11111000, 0x01110000, 0x11111000,
	0x01010000, 0x01100000, 0x10001000, 0x00001000, 0x00010000, 0x10000000, 0x10001000, 0x00001000,
	0x10001000, 0x10100000, 0x10001000, 0x00010000, 0x00110000, 0x10000000, 0x10000000, 0x00010000,
	0x10001000, 0x00100000, 0x00001000, 0x00100000, 0x01010000, 0x10110000, 0x10000000, 0x00010000,
	0x10001000, 0x00100000, 0x00010000, 0x01110000, 0x01010000, 0x11001000, 0x11110000, 0x00100000,
	0x10001000, 0x00100000, 0x00100000, 0x00001000, 0x10010000, 0x00001000, 0x10001000, 0x00100000,
	0x10001000, 0x00100000, 0x01000000, 0x00001000, 0x11111000, 0x00001000, 0x10001000, 0x01000000,
	0x01010000, 0x00100000, 0x10000000, 0x10001000, 0x00010000, 0x10001000, 0x10001000, 0x01000000,
	0x00100000, 0x11111000, 0x11111000, 0x01110000, 0x00010000, 0x01110000, 0x01110000, 0x01000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x01110000, 0x01110000, 0x00000000, 0x00000000, 0x00001000, 0x00000000, 0x10000000, 0x01110000,
	0x10001000, 0x10001000, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0x01000000, 0x10001000,
	0x10001000, 0x10001000, 0x00100000, 0x00100000, 0x00100000, 0x00000000, 0x00100000, 0x10001000,
	0x10001000, 0x10001000, 0x01110000, 0x01110000, 0x01000000, 0x11111000, 0x00010000, 0x00001000,
	0x01110000, 0x01111000, 0x00100000, 0x00100000, 0x10000000, 0x00000000, 0x00001000, 0x00010000,
	0x10001000, 0x00001000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x00010000, 0x00100000,
	0x10001000, 0x00001000, 0x00000000, 0x00000000, 0x00100000, 0x11111000, 0x00100000, 0x00100000,
	0x10001000, 0x10001000, 0x00100000, 0x00110000, 0x00010000, 0x00000000, 0x01000000, 0x00000000,
	0x01110000, 0x01110000, 0x01110000, 0x00100000, 0x00001000, 0x00000000, 0x10000000, 0x00100000,
	0x00000000, 0x00000000, 0x00100000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x01110000, 0x00100000, 0x11110000, 0x01110000, 0x11110000, 0x11111000, 0x11111000, 0x01110000,
	0x10001000, 0x01010000, 0x01001000, 0x10001000, 0x01001000, 0x10000000, 0x10000000, 0x10001000,
	0x10001000, 0x10001000, 0x01001000, 0x10000000, 0x01001000, 0x10000000, 0x10000000, 0x10000000,
	0x10011000, 0x10001000, 0x01001000, 0x10000000, 0x01001000, 0x10000000, 0x10000000, 0x10000000,
	0x10101000, 0x10001000, 0x01110000, 0x10000000, 0x01001000, 0x11110000, 0x11110000, 0x10000000,
	0x10101000, 0x11111000, 0x01001000, 0x10000000, 0x01001000, 0x10000000, 0x10000000, 0x10011000,
	0x10110000, 0x10001000, 0x01001000, 0x10000000, 0x01001000, 0x10000000, 0x10000000, 0x10001000,
	0x10000000, 0x10001000, 0x01001000, 0x10001000, 0x01001000, 0x10000000, 0x10000000, 0x10001000,
	0x01111000, 0x10001000, 0x11110000, 0x01110000, 0x11110000, 0x11111000, 0x10000000, 0x01110000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x10001000, 0x01110000, 0x00111000, 0x10001000, 0x10000000, 0x10001000, 0x10001000, 0x01110000,
	0x10001000, 0x00100000, 0x00010000, 0x10001000, 0x10000000, 0x10001000, 0x11001000, 0x10001000,
	0x10001000, 0x00100000, 0x00010000, 0x10010000, 0x10000000, 0x11011000, 0x11001000, 0x10001000,
	0x10001000, 0x00100000, 0x00010000, 0x10100000, 0x10000000, 0x10101000, 0x10101000, 0x10001000,
	0x11111000, 0x00100000, 0x00010000, 0x11000000, 0x10000000, 0x10101000, 0x10101000, 0x10001000,
	0x10001000, 0x00100000, 0x00010000, 0x10100000, 0x10000000, 0x10001000, 0x10011000, 0x10001000,
	0x10001000, 0x00100000, 0x00010000, 0x10010000, 0x10000000, 0x10001000, 0x10011000, 0x10001000,
	0x10001000, 0x00100000, 0x10010000, 0x10001000, 0x10000000, 0x10001000, 0x10001000, 0x10001000,
	0x10001000, 0x01110000, 0x01100000, 0x10001000, 0x11111000, 0x10001000, 0x10001000, 0x01110000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x11110000, 0x01110000, 0x11110000, 0x01110000, 0x11111000, 0x10001000, 0x10001000, 0x10001000,
	0x10001000, 0x10001000, 0x10001000, 0x10001000, 0x00100000, 0x10001000, 0x10001000, 0x10001000,
	0x10001000, 0x10001000, 0x10001000, 0x10000000, 0x00100000, 0x10001000, 0x10001000, 0x10001000,
	0x10001000, 0x10001000, 0x10001000, 0x10000000, 0x00100000, 0x10001000, 0x10001000, 0x10001000,
	0x11110000, 0x10001000, 0x11110000, 0x01110000, 0x00100000, 0x10001000, 0x01010000, 0x10101000,
	0x10000000, 0x10001000, 0x10100000, 0x00001000, 0x00100000, 0x10001000, 0x01010000, 0x10101000,
	0x10000000, 0x10001000, 0x10010000, 0x00001000, 0x00100000, 0x10001000, 0x01010000, 0x10101000,
	0x10000000, 0x10101000, 0x10001000, 0x10001000, 0x00100000, 0x10001000, 0x00100000, 0x10101000,
	0x10000000, 0x01110000, 0x10001000, 0x01110000, 0x00100000, 0x01110000, 0x00100000, 0x01010000,
	0x00000000, 0x00001000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x01110000, 0x00000000, 0x01110000, 0x00000000, 0x00000000,
	0x10001000, 0x10001000, 0x11111000, 0x01000000, 0x10000000, 0x00010000, 0x00100000, 0x00000000,
	0x10001000, 0x10001000, 0x00001000, 0x01000000, 0x10000000, 0x00010000, 0x01010000, 0x00000000,
	0x01010000, 0x01010000, 0x00010000, 0x01000000, 0x01000000, 0x00010000, 0x10001000, 0x00000000,
	0x01010000, 0x01010000, 0x00010000, 0x01000000, 0x01000000, 0x00010000, 0x00000000, 0x00000000,
	0x00100000, 0x00100000, 0x00100000, 0x01000000, 0x00100000, 0x00010000, 0x00000000, 0x00000000,
	0x01010000, 0x00100000, 0x01000000, 0x01000000, 0x00010000, 0x00010000, 0x00000000, 0x00000000,
	0x01010000, 0x00100000, 0x01000000, 0x01000000, 0x00010000, 0x00010000, 0x00000000, 0x00000000,
	0x10001000, 0x00100000, 0x10000000, 0x01000000, 0x00001000, 0x00010000, 0x00000000, 0x00000000,
	0x10001000, 0x00100000, 0x11111000, 0x01000000, 0x00001000, 0x00010000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x01110000, 0x00000000, 0x01110000, 0x00000000, 0x11111000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00100000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00010000, 0x00000000, 0x10000000, 0x00000000, 0x00001000, 0x00000000, 0x00110000, 0x00000000,
	0x00000000, 0x00000000, 0x10000000, 0x00000000, 0x00001000, 0x00000000, 0x01001000, 0x00000000,
	0x00000000, 0x00000000, 0x10000000, 0x00000000, 0x00001000, 0x00000000, 0x01000000, 0x00000000,
	0x00000000, 0x01110000, 0x11110000, 0x01110000, 0x01111000, 0x01110000, 0x01000000, 0x01110000,
	0x00000000, 0x00001000, 0x10001000, 0x10001000, 0x10001000, 0x10001000, 0x11110000, 0x10001000,
	0x00000000, 0x01111000, 0x10001000, 0x10000000, 0x10001000, 0x11111000, 0x01000000, 0x10001000,
	0x00000000, 0x10001000, 0x10001000, 0x10000000, 0x10001000, 0x10000000, 0x01000000, 0x10001000,
	0x00000000, 0x10011000, 0x10001000, 0x10001000, 0x10001000, 0x10001000, 0x01000000, 0x01111000,
	0x00000000, 0x01101000, 0x11110000, 0x01110000, 0x01111000, 0x01110000, 0x01000000, 0x00001000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x10001000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01110000,
	
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x10000000, 0x00000000, 0x00000000, 0x10000000, 0x01100000, 0x00000000, 0x00000000, 0x00000000,
	0x10000000, 0x00100000, 0x00010000, 0x10000000, 0x00100000, 0x00000000, 0x00000000, 0x00000000,
	0x10000000, 0x00000000, 0x00000000, 0x10000000, 0x00100000, 0x00000000, 0x00000000, 0x00000000,
	0x10110000, 0x01100000, 0x00110000, 0x10010000, 0x00100000, 0x11010000, 0x10110000, 0x01110000,
	0x11001000, 0x00100000, 0x00010000, 0x10100000, 0x00100000, 0x10101000, 0x11001000, 0x10001000,
	0x10001000, 0x00100000, 0x00010000, 0x11000000, 0x00100000, 0x10101000, 0x10001000, 0x10001000,
	0x10001000, 0x00100000, 0x00010000, 0x10100000, 0x00100000, 0x10101000, 0x10001000, 0x10001000,
	0x10001000, 0x00100000, 0x00010000, 0x10010000, 0x00100000, 0x10101000, 0x10001000, 0x10001000,
	0x10001000, 0x01110000, 0x10010000, 0x10001000, 0x01110000, 0x10001000, 0x10001000, 0x01110000,
	0x00000000, 0x00000000, 0x10010000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x01100000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000,
	0x11110000, 0x01111000, 0x10110000, 0x01110000, 0x11110000, 0x10001000, 0x10001000, 0x10001000,
	0x10001000, 0x10001000, 0x11001000, 0x10001000, 0x01000000, 0x10001000, 0x10001000, 0x10001000,
	0x10001000, 0x10001000, 0x10000000, 0x01100000, 0x01000000, 0x10001000, 0x10001000, 0x10101000,
	0x10001000, 0x10001000, 0x10000000, 0x00010000, 0x01000000, 0x10001000, 0x01010000, 0x10101000,
	0x11110000, 0x01111000, 0x10000000, 0x10001000, 0x01001000, 0x10011000, 0x01010000, 0x10101000,
	0x10000000, 0x00001000, 0x10000000, 0x01110000, 0x00110000, 0x01101000, 0x00100000, 0x01010000,
	0x10000000, 0x00001000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x10000000, 0x00001000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00011000, 0x00000000, 0x11000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00100000, 0x00100000, 0x00100000, 0x01001000, 0x01110000,
	0x00000000, 0x00000000, 0x00000000, 0x00100000, 0x00100000, 0x00100000, 0x10101000, 0x10001000,
	0x00000000, 0x00000000, 0x00000000, 0x00100000, 0x00100000, 0x00100000, 0x10010000, 0x10001000,
	0x10001000, 0x10001000, 0x11111000, 0x00100000, 0x00100000, 0x00100000, 0x00000000, 0x00001000,
	0x01010000, 0x10001000, 0x00010000, 0x11000000, 0x00100000, 0x00011000, 0x00000000, 0x00010000,
	0x00100000, 0x10001000, 0x00100000, 0x00100000, 0x00100000, 0x00100000, 0x00000000, 0x00100000,
	0x00100000, 0x10011000, 0x01000000, 0x00100000, 0x00100000, 0x00100000, 0x00000000, 0x00100000,
	0x01010000, 0x01101000, 0x10000000, 0x00100000, 0x00100000, 0x00100000, 0x00000000, 0x00000000,
	0x10001000, 0x00001000, 0x11111000, 0x00100000, 0x00100000, 0x00100000, 0x00000000, 0x00100000,
	0x00000000, 0x10001000, 0x00000000, 0x00011000, 0x00000000, 0x11000000, 0x00000000, 0x00000000,
	0x00000000, 0x01110000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

#define kFontGroupWidth		8
#define kFontGroupHeight	13

void CreateFont()
{
	int i, j, k;
	
	for( i = 0; i < kFontGroupHeight; i++ )
	{
		for( j = 0; j < kFontGroupWidth; j++ )
		{
			long	line0 = i * kFontGroupWidth * kFontGroupHeight + j;
			
			for( k = 0; k < 13; k++ )
			{
				long	line1 = line0 + k * kFontGroupWidth;
				long	value = fontLong[line1];
				long	v0, v1, v2, v3, v4, v5, v6, v7;
				unsigned char out;
				
				v7 = ( value&0x10000000 ) != 0;
				v6 = ( value&0x01000000 ) != 0;
				v5 = ( value&0x00100000 ) != 0;
				v4 = ( value&0x00010000 ) != 0;
				v3 = ( value&0x00001000 ) != 0;
				v2 = ( value&0x00000100 ) != 0;
				v1 = ( value&0x00000010 ) != 0;
				v0 = ( value&0x00000001 ) != 0;
				
				out = (v0<<0)|(v1<<1)|(v2<<2)|(v3<<3)|(v4<<4)|(v5<<5)|(v6<<6)|(v7<<7);
				printf( "%4d,", out );
			}
			printf( "\n" );
		}
		printf( "\n" );
	}	
	exit(0);
}
*/

#define kFontDisplayWidth	6
#define kFontWidth			5
#define kFontHeight			14
#define kFontTotal			96
#define kHiLightValue		255
#define	kShadowValue		96

void buf_printf(int is_interleaved, unsigned char* yPtr, int rowbytes, int height, int x, int y, const char* fmt, ...)
{
	unsigned  char font[kFontTotal*kFontHeight] =
	{
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,  32,  32,  32,  32,  32,  32,  32,   0,  32,   0,   0,   0,
	   0,   0,  80,  80,  80,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,  80,  80, 248,  80, 248,  80,  80,   0,   0,   0,   0,
	   0,   0,  32, 120, 160, 160, 112,  40,  40, 240,  32,   0,   0,   0,
	   0,   0,  72, 168,  80,  16,  32,  64,  80, 168, 144,   0,   0,   0,
	   0,   0,   0,  64, 160, 160,  64, 160, 152, 144, 104,   0,   0,   0,
	   0,   0,  32,  32,  32,   0,   0,   0,   0,   0,   0,   0,   0,   0,
																	 
	   0,  16,  32,  32,  64,  64,  64,  64,  64,  32,  32,  16,   0,   0,
	   0,  64,  32,  32,  16,  16,  16,  16,  16,  32,  32,  64,   0,   0,
	   0,   0,  32, 168, 112, 168,  32,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,  32,  32, 248,  32,  32,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,  48,  32,  64,   0,   0,
	   0,   0,   0,   0,   0,   0, 248,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,  32, 112,  32,   0,   0,
	   0,   0,   8,   8,  16,  16,  32,  64,  64, 128, 128,   0,   0,   0,
																	 
	   0,   0,  32,  80, 136, 136, 136, 136, 136,  80,  32,   0,   0,   0,
	   0,   0,  32,  96, 160,  32,  32,  32,  32,  32, 248,   0,   0,   0,
	   0,   0, 112, 136, 136,   8,  16,  32,  64, 128, 248,   0,   0,   0,
	   0,   0, 248,   8,  16,  32, 112,   8,   8, 136, 112,   0,   0,   0,
	   0,   0,  16,  16,  48,  80,  80, 144, 248,  16,  16,   0,   0,   0,
	   0,   0, 248, 128, 128, 176, 200,   8,   8, 136, 112,   0,   0,   0,
	   0,   0, 112, 136, 128, 128, 240, 136, 136, 136, 112,   0,   0,   0,
	   0,   0, 248,   8,  16,  16,  32,  32,  64,  64,  64,   0,   0,   0,
																	 
	   0,   0, 112, 136, 136, 136, 112, 136, 136, 136, 112,   0,   0,   0,
	   0,   0, 112, 136, 136, 136, 120,   8,   8, 136, 112,   0,   0,   0,
	   0,   0,   0,   0,  32, 112,  32,   0,   0,  32, 112,  32,   0,   0,
	   0,   0,   0,   0,  32, 112,  32,   0,   0,  48,  32,  64,   0,   0,
	   0,   0,   8,  16,  32,  64, 128,  64,  32,  16,   8,   0,   0,   0,
	   0,   0,   0,   0,   0, 248,   0,   0, 248,   0,   0,   0,   0,   0,
	   0,   0, 128,  64,  32,  16,   8,  16,  32,  64, 128,   0,   0,   0,
	   0,   0, 112, 136, 136,   8,  16,  32,  32,   0,  32,   0,   0,   0,
																	 
	   0,   0, 112, 136, 136, 152, 168, 168, 176, 128, 120,   0,   0,   0,
	   0,   0,  32,  80, 136, 136, 136, 248, 136, 136, 136,   0,   0,   0,
	   0,   0, 240,  72,  72,  72, 112,  72,  72,  72, 240,   0,   0,   0,
	   0,   0, 112, 136, 128, 128, 128, 128, 128, 136, 112,   0,   0,   0,
	   0,   0, 240,  72,  72,  72,  72,  72,  72,  72, 240,   0,   0,   0,
	   0,   0, 248, 128, 128, 128, 240, 128, 128, 128, 248,   0,   0,   0,
	   0,   0, 248, 128, 128, 128, 240, 128, 128, 128, 128,   0,   0,   0,
	   0,   0, 112, 136, 128, 128, 128, 152, 136, 136, 112,   0,   0,   0,
																	 
	   0,   0, 136, 136, 136, 136, 248, 136, 136, 136, 136,   0,   0,   0,
	   0,   0, 112,  32,  32,  32,  32,  32,  32,  32, 112,   0,   0,   0,
	   0,   0,  56,  16,  16,  16,  16,  16,  16, 144,  96,   0,   0,   0,
	   0,   0, 136, 136, 144, 160, 192, 160, 144, 136, 136,   0,   0,   0,
	   0,   0, 128, 128, 128, 128, 128, 128, 128, 128, 248,   0,   0,   0,
	   0,   0, 136, 136, 216, 168, 168, 136, 136, 136, 136,   0,   0,   0,
	   0,   0, 136, 200, 200, 168, 168, 152, 152, 136, 136,   0,   0,   0,
	   0,   0, 112, 136, 136, 136, 136, 136, 136, 136, 112,   0,   0,   0,
																	 
	   0,   0, 240, 136, 136, 136, 240, 128, 128, 128, 128,   0,   0,   0,
	   0,   0, 112, 136, 136, 136, 136, 136, 136, 168, 112,   8,   0,   0,
	   0,   0, 240, 136, 136, 136, 240, 160, 144, 136, 136,   0,   0,   0,
	   0,   0, 112, 136, 128, 128, 112,   8,   8, 136, 112,   0,   0,   0,
	   0,   0, 248,  32,  32,  32,  32,  32,  32,  32,  32,   0,   0,   0,
	   0,   0, 136, 136, 136, 136, 136, 136, 136, 136, 112,   0,   0,   0,
	   0,   0, 136, 136, 136, 136,  80,  80,  80,  32,  32,   0,   0,   0,
	   0,   0, 136, 136, 136, 136, 168, 168, 168, 168,  80,   0,   0,   0,
																	 
	   0,   0, 136, 136,  80,  80,  32,  80,  80, 136, 136,   0,   0,   0,
	   0,   0, 136, 136,  80,  80,  32,  32,  32,  32,  32,   0,   0,   0,
	   0,   0, 248,   8,  16,  16,  32,  64,  64, 128, 248,   0,   0,   0,
	   0, 112,  64,  64,  64,  64,  64,  64,  64,  64,  64, 112,   0,   0,
	   0,   0, 128, 128,  64,  64,  32,  16,  16,   8,   8,   0,   0,   0,
	   0, 112,  16,  16,  16,  16,  16,  16,  16,  16,  16, 112,   0,   0,
	   0,   0,  32,  80, 136,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 248,   0,   0,
																	 
	   0,  32,  16,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0, 112,   8, 120, 136, 152, 104,   0,   0,   0,
	   0,   0, 128, 128, 128, 240, 136, 136, 136, 136, 240,   0,   0,   0,
	   0,   0,   0,   0,   0, 112, 136, 128, 128, 136, 112,   0,   0,   0,
	   0,   0,   8,   8,   8, 120, 136, 136, 136, 136, 120,   0,   0,   0,
	   0,   0,   0,   0,   0, 112, 136, 248, 128, 136, 112,   0,   0,   0,
	   0,   0,  48,  72,  64,  64, 240,  64,  64,  64,  64,   0,   0,   0,
	   0,   0,   0,   0,   0, 112, 136, 136, 136, 120,   8, 136, 112,   0,
																	 
	   0,   0, 128, 128, 128, 176, 200, 136, 136, 136, 136,   0,   0,   0,
	   0,   0,   0,  32,   0,  96,  32,  32,  32,  32, 112,   0,   0,   0,
	   0,   0,   0,  16,   0,  48,  16,  16,  16,  16, 144, 144,  96,   0,
	   0,   0, 128, 128, 128, 144, 160, 192, 160, 144, 136,   0,   0,   0,
	   0,   0,  96,  32,  32,  32,  32,  32,  32,  32, 112,   0,   0,   0,
	   0,   0,   0,   0,   0, 208, 168, 168, 168, 168, 136,   0,   0,   0,
	   0,   0,   0,   0,   0, 176, 200, 136, 136, 136, 136,   0,   0,   0,
	   0,   0,   0,   0,   0, 112, 136, 136, 136, 136, 112,   0,   0,   0,
																	 
	   0,   0,   0,   0,   0, 240, 136, 136, 136, 240, 128, 128, 128,   0,
	   0,   0,   0,   0,   0, 120, 136, 136, 136, 120,   8,   8,   8,   0,
	   0,   0,   0,   0,   0, 176, 200, 128, 128, 128, 128,   0,   0,   0,
	   0,   0,   0,   0,   0, 112, 136,  96,  16, 136, 112,   0,   0,   0,
	   0,   0,   0,  64,  64, 240,  64,  64,  64,  72,  48,   0,   0,   0,
	   0,   0,   0,   0,   0, 136, 136, 136, 136, 152, 104,   0,   0,   0,
	   0,   0,   0,   0,   0, 136, 136, 136,  80,  80,  32,   0,   0,   0,
	   0,   0,   0,   0,   0, 136, 136, 168, 168, 168,  80,   0,   0,   0,
																	 
	   0,   0,   0,   0,   0, 136,  80,  32,  32,  80, 136,   0,   0,   0,
	   0,   0,   0,   0,   0, 136, 136, 136, 152, 104,   8, 136, 112,   0,
	   0,   0,   0,   0,   0, 248,  16,  32,  64, 128, 248,   0,   0,   0,
	   0,  24,  32,  32,  32,  32, 192,  32,  32,  32,  32,  24,   0,   0,
	   0,   0,  32,  32,  32,  32,  32,  32,  32,  32,  32,   0,   0,   0,
	   0, 192,  32,  32,  32,  32,  24,  32,  32,  32,  32, 192,   0,   0,
	   0,   0,  72, 168, 144,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0, 112, 136, 136,   8,  16,  32,  32,   0,  32,   0,   0,   0
	};
	char			buf[128];
	int				chCount, chIdx, lineIdx;
	unsigned char	*y0Ptr = yPtr;
	unsigned char	*y1Ptr = y0Ptr + height * rowbytes;
	
#define W_BYTES(x) (is_interleaved ? (x>>1)*6		 : x )
#define	H_BYTES(y) (is_interleaved ? (y>>1)*rowbytes : y * rowbytes ) 

	y1Ptr  = y0Ptr + H_BYTES(height);
	y0Ptr += H_BYTES(y) + W_BYTES(x);

	if( y0Ptr > y1Ptr )  //out of reach
		return;
		
	{
		va_list	ap;
		
		va_start(ap, fmt);
		chCount = vsprintf(buf, fmt, ap);
		va_end(ap);
	}
	
	for( chIdx = 0; chIdx < chCount; chIdx++ )
	{
		unsigned char *draw0Ptr;
		int		      fontIdx; 
		char		  ch = (unsigned char)buf[chIdx];

		draw0Ptr  = y0Ptr + W_BYTES(chIdx * kFontDisplayWidth);

		if( draw0Ptr > y1Ptr - W_BYTES(kFontDisplayWidth) )
			break;
		
		ch -= 0x20;

		if( ch > kFontTotal )
			ch = 0;	         //default space
		
		fontIdx = ch * kFontHeight;
		
		for( lineIdx = 0; lineIdx < kFontHeight; lineIdx += 2 )
		{
			unsigned char *draw1Ptr;
			unsigned char fontLine;
			unsigned char value[2] = { kShadowValue, kHiLightValue };
			int p00,p01,p02,p03,p04,p05;
			int p10,p11,p12,p13,p14,p15;

			fontLine = font[ fontIdx + lineIdx ];
			p00 = value[ (( fontLine & 0x80 ) != 0) ];
			p01 = value[ (( fontLine & 0x40 ) != 0) ];
			p02 = value[ (( fontLine & 0x20 ) != 0) ];
			p03 = value[ (( fontLine & 0x10 ) != 0) ];
			p04 = value[ (( fontLine & 0x08 ) != 0) ];
			p05 = value[0];

			fontLine = font[ fontIdx + (lineIdx+1) ];
			p10 = value[ (( fontLine & 0x80 ) != 0) ];
			p11 = value[ (( fontLine & 0x40 ) != 0) ];
			p12 = value[ (( fontLine & 0x20 ) != 0) ];
			p13 = value[ (( fontLine & 0x10 ) != 0) ];
			p14 = value[ (( fontLine & 0x08 ) != 0) ];
			p15 = value[0];


			if( !is_interleaved )
			{
				draw1Ptr = draw0Ptr + H_BYTES(lineIdx);
				if( draw1Ptr > y1Ptr - W_BYTES(kFontDisplayWidth) )
					break;

				draw1Ptr[0] = p00;
				draw1Ptr[1] = p01;
				draw1Ptr[2] = p02;
				draw1Ptr[3] = p03;
				draw1Ptr[4] = p04;
				draw1Ptr[5] = p05;
			
				draw1Ptr = draw0Ptr + H_BYTES((lineIdx+1));
				if( draw1Ptr > y1Ptr - W_BYTES(kFontDisplayWidth) )
					break;
				
				draw1Ptr[0] = p10;
				draw1Ptr[1] = p11;
				draw1Ptr[2] = p12;
				draw1Ptr[3] = p13;
				draw1Ptr[4] = p14;
				draw1Ptr[5] = p15;
			}
			else
			{
				draw1Ptr = draw0Ptr + H_BYTES(lineIdx);
				if( draw1Ptr > y1Ptr - W_BYTES(kFontDisplayWidth) )
					break;

				draw1Ptr[ 2] = p00;
				draw1Ptr[ 3] = p01;
				draw1Ptr[ 4] = p10;
				draw1Ptr[ 5] = p11;

				draw1Ptr[ 8] = p02;
				draw1Ptr[ 9] = p03;
				draw1Ptr[10] = p12;
				draw1Ptr[11] = p13;

				draw1Ptr[14] = p04;
				draw1Ptr[15] = p05;
				draw1Ptr[16] = p14;
				draw1Ptr[17] = p15;
			}
		}
	}		
}

/*
#ifdef __cplusplus
extern "C" {
#endif

#include "ippdefs.h"

void bilinear_qpel
(
	const Ipp8u*	src,
	Ipp32s			srcStep,
	Ipp8u*			dst,
	Ipp32s			dstStep,
	Ipp32s			dx,
	Ipp32s			dy,
	IppiSize		roiSize
);

IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R_c
(
  const Ipp8u*   pSrc,
		Ipp32s   srcStep,
		Ipp8u*   dst,
		Ipp32s   dstStep,
		Ipp32s   dx,
		Ipp32s   dy,
		IppiSize roi   // Must be 16,8 or 4
);


#ifdef __cplusplus
}
#endif

void test_bilinear_qpel(void)
{
#define SRC_WID 320
#define SRC_HET 320
#define DST_WID 256
#define DST_HET 256
#define BW  16
#define BH  16

	unsigned char *src0, *src, *dst;
	int src_size, dst_size;
	int i, j;
	int bw, bh, dx, dy;

	char path[255] = {"C:\\test_avc\\test_avc_intpl.yuv"};
	static FILE	*f	= NULL;

	if( f == NULL )
	{
		f = fopen(path, "wb");
		if( f == NULL )
			return;
	}

	src_size = (SRC_WID * SRC_HET * 3) / 2;
	dst_size = (DST_WID * DST_HET * 3) / 2;
	src0 = malloc( src_size );
	dst = malloc( dst_size );
	
	src = src0 + 4 * SRC_WID + 4;

	bw = 4;
	bh = 12;
	dx = 0;
	dy = 0;

	for( bw = 4; bw <= 16; bw += 4 )
	for( bh = 4; bh <= 16; bh += 4 )
	for( dx = 0; dx < 4;  dx++ )
	for( dy = 0; dy < 4;  dy++ )
	{
		if( bw == 12 || bh == 12 )
			continue;
		
		for( j = 0; j < SRC_HET; j++ )
		{
			for( i = 0; i < SRC_WID; i++ )
			{
				unsigned char *this_src = src + j * SRC_WID + i;
				int value;

				if( i <= j )
					value = j;
				else
					value = i;
				
				if( value > 255 )
					value = 255;

				*this_src = value;
			}
		}

		for( i = SRC_WID * SRC_HET; i < src_size; i ++ )
			src[i] = 128;


		buf_printf(src, SRC_WID, SRC_HET, 10, 20, "~ ! @ # $ % ^ & * , . ( ) - +");
		buf_printf(src, SRC_WID, SRC_HET, 10, 50, "bw=%d, bh=%d, dx=%d, dy=%d", bw, bh, dx, dy);

		for( j = 0; j < DST_HET; j++ )
			for( i = 0; i < DST_WID; i++ )
			{
				unsigned char *this_dst = dst + j * DST_WID + i;
				*this_dst = 0;
			}

		for( i = DST_WID * DST_HET; i < dst_size; i ++ )
			dst[i] = 128;


		for( j = 0; j < DST_HET; j+= bh )
		{
			for( i = 0; i < DST_WID; i+= bw )
			{
				unsigned char	*this_src = src + j * SRC_WID + i;
				unsigned char	*this_dst = dst + j * DST_WID + i;
				IppiSize		roiSize;

				roiSize.width  = bw;
				roiSize.height = bh;

				//ippiInterpolateLuma_H264_8u_C1R_c
				bilinear_qpel
				(
					this_src,
					SRC_WID,
					this_dst,
					DST_WID,
					dx,
					dy,
					roiSize
				);
			}
		}

		//buf_printf(src, WID, HET, 10, 100, "bw=%d, bh=%d, dx=%d, dy=%d", bw, bh, dx, dy);
		buf_printf(dst, DST_WID, DST_HET, 10, 100, "bw=%d, bh=%d, dx=%d, dy=%d", bw, bh, dx, dy);
		//fwrite(src, 1, size, f);
		fwrite(dst, 1, dst_size, f);
	}
}

*/

#define MAC_BUILD

#ifdef MAC_BUILD
#define DUMP_INPUT_PATH	"/input.bin"
#define DUMP_YUV_PATH	"/output.yuv"
#else
#define DUMP_INPUT_PATH	"E:\\input.bin"
#define DUMP_YUV_PATH	"E:\\output.yuv"
#endif
static FILE *gInputFile = NULL;

void dump_bitstream_raw( unsigned char *in, long size )
{
	unsigned char *d0 = NULL;
	unsigned char *d = NULL;
	long total = 0;

	d0 = (unsigned char *)malloc( size );
	memcpy((void *)d0, (void *)in, size );
	d = d0;

	if( gInputFile == NULL )
	{
		gInputFile = fopen( DUMP_INPUT_PATH, "wb" );	
		if( gInputFile == NULL )
			fprintf(stderr, "cannot creat input file\n" );
	}
	
	while( total < size )
	{
		long thisSize = d[0]<<24|d[1]<<16|d[2]<<8|d[3]<<0;

		d[0] = 0;
		d[1] = 0;
		d[2] = 0;
		d[3] = 1;

		fwrite( d,  1, thisSize + 4, gInputFile);
		fflush(gInputFile);

		total += thisSize + 4;
		d     += thisSize + 4;
	}

	free( (void *)d0 );
}


void dump_input( unsigned char *in, long size )
{
	if( gInputFile == NULL )
	{
		gInputFile = fopen( DUMP_INPUT_PATH, "wb" );	
		if( gInputFile == NULL )
			fprintf(stderr, "cannot creat input file\n" );
	}
	
	fwrite( in,  1, size, gInputFile);
	fflush(gInputFile);
}


void dump_bitstream_box( unsigned char *in, long size )
{
	
	if( gInputFile == NULL )
	{
		gInputFile = fopen( DUMP_INPUT_PATH, "wb" );	
		if( gInputFile == NULL )
			fprintf(stderr, "cannot creat input file\n" );
	}
	
	fwrite( &size,  1, 4, gInputFile);
	fwrite( in,  1, size, gInputFile);
	fflush(gInputFile);

}


void dump_amr( unsigned char *in, long size )
{
#define AMR_MAGIC_NUMBER "#!AMR\n"
	
	if( gInputFile == NULL )
	{
		char header[6] = {0x23, 0x21, 0x41, 0x4d, 0x52, 0x0a};

		gInputFile = fopen( "C:\\test_amr\\dump.amr", "wb" );	
		if( gInputFile == NULL )
			fprintf(stderr, "cannot creat amr dump file\n" );

		fwrite( header,  1, 6, gInputFile);
		fflush(gInputFile);
	}
	
	fwrite( in,  1, size, gInputFile);
	fflush(gInputFile);
}


void close_input_dump_file()
{
	if( gInputFile != NULL )
		fclose( gInputFile );

	gInputFile = NULL;
}


void dump_data( unsigned char *data, int size, char *path ) 
{
	static FILE *fp = NULL;

	if( fp == NULL )
	{
		if( path != NULL )
			fp = fopen( path, "wb" );	
		else
			fp = fopen( "C:\\output.yuv", "wb" );	
		
		if( fp == NULL )
			fprintf(stderr, "cannot open yuv dump file\n" );
	}

	fwrite( data,  1, size, fp);

	fflush(fp);
}


void dump_yuv( long w, long h, long y_rowbytes, long uv_rowbytes, unsigned char *y, unsigned char *cb, unsigned char *cr, char *path ) 
{
	static FILE *fp = NULL;

	if( fp == NULL )
	{
		if( path != NULL )
			fp = fopen( path, "wb" );	
		else
			fp = fopen( "C:\\output.yuv", "wb" );	
		
		if( fp == NULL )
			fprintf(stderr, "cannot open yuv dump file\n" );
	}

	{
		unsigned char *p;
		long i;

		p = y;
		for( i = 0; i < h; i++ )
		{
			fwrite( p,  1, w, fp);
			p += y_rowbytes;
		}
		
		p = cb;
		for( i = 0; i < h>>1; i++ )
		{
			fwrite( p,  1, w>>1, fp);
			p += uv_rowbytes;
		}
		
		p = cr;
		for( i = 0; i < h>>1; i++ )
		{
			fwrite( p,  1, w>>1, fp);
			p += uv_rowbytes;
		}		
	}

	fflush(fp);
}

#define kFrameDuration_ms		75

int h263_box_to_flv(char *src_name )
{
	char	dst_name[256];
	FILE	*src_file	= NULL;
	FILE	*dst_file	= NULL;

	int				src_size	= 0;
	int				count		= 0;
	unsigned char	*src_bytes	= NULL;

	int time_stamp			= 0;		//3
	int previous_tag_size	= 0;
	char	tmp_bytes[16];

	src_file = fopen(src_name, "rb");
	if( src_file == NULL )
		return -1;

	fseek(src_file, 0, SEEK_END);
	src_size = ftell(src_file);
	fseek(src_file, 0, SEEK_SET);

	//strcpy( srcName, INPUT_DIR_PATH);//PATH );
	//strcat( srcName, INPUT_DEFAULT_FILE );

	strcpy( dst_name, src_name );
	strcat( dst_name, ".flv" );

	dst_file = fopen( dst_name, "wb" );
	if( dst_file == NULL )
		return -1;

	src_bytes = (unsigned char *)malloc( src_size );
	if( src_bytes == NULL )
		return -1;
	fread( src_bytes, 1, src_size, src_file );

	{
		char flv_header[16] = { 0x46,0x4c,0x56,0x01,0x05,0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x00};
		fwrite( flv_header, 13,  1, dst_file );
	}

	while( count < src_size )
	{
		int	dst_bytes_total  = GET_LE_LONG(src_bytes + count);
		unsigned char *dst_bytes		 = src_bytes + count + 4;
		int type				= 0x09;					//1	video: 9, audio: 8
		int	body_length			= dst_bytes_total+1;	//3	
			//int time_stamp		= 0;				//3
		int time_stamp_extended = 0;					//1
		int stream_id			= 0;					//3	=>11 total
		int body_first_byte		= 0x12;

		PUSH_UINT8_BE( type, tmp_bytes);				fwrite( tmp_bytes, 1,  1, dst_file );
		PUSH_UINT24_BE(body_length, tmp_bytes);			fwrite( tmp_bytes, 3,  1, dst_file );
		PUSH_UINT24_BE(time_stamp, tmp_bytes);			fwrite( tmp_bytes, 3,  1, dst_file );
		PUSH_UINT8_BE(time_stamp_extended, tmp_bytes);	fwrite( tmp_bytes, 1,  1, dst_file );
		PUSH_UINT24_BE(stream_id, tmp_bytes);			fwrite( tmp_bytes, 3,  1, dst_file );
		PUSH_UINT8_BE( body_first_byte, tmp_bytes);		fwrite( tmp_bytes, 1,  1, dst_file );

														fwrite( dst_bytes, dst_bytes_total,  1, dst_file );

		previous_tag_size	= body_length + 11;
		PUSH_UINT32_BE( previous_tag_size, tmp_bytes);	fwrite( tmp_bytes, 4,  1, dst_file );

		fflush(dst_file);
		
		count		+= 4 + dst_bytes_total;
		time_stamp	+= kFrameDuration_ms;					//3	
	}

//bail:
	return 0;
}




int make_adts_header( int sample_rate, unsigned short channel_configuration, unsigned short aac_frame_length, unsigned char *header )
{
	//const long sampleRates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000};
	unsigned char		sample_rate_index = 0;
	unsigned char		profile = 1;
	unsigned short		adts_buffer_fullness = 2047;

	int		err = 0;

	if( sample_rate == 96000 )
		sample_rate_index = 0;
	else if( sample_rate == 88200 )
		sample_rate_index = 1;
	else if( sample_rate == 64000 )
		sample_rate_index = 2;
	else if( sample_rate == 48000 )
		sample_rate_index = 3;
	else if( sample_rate == 44100 )
		sample_rate_index = 4;
	else if( sample_rate == 32000 )
		sample_rate_index = 5;
	else if( sample_rate == 24000 )
		sample_rate_index = 6;
	else if( sample_rate == 22050 )
		sample_rate_index = 7;
	else if( sample_rate == 16000 )
		sample_rate_index = 8;
	else if( sample_rate == 12000 )
		sample_rate_index = 9;
	else if( sample_rate == 11025 )
		sample_rate_index = 10;
	else if( sample_rate == 8000 )
		sample_rate_index = 11;
	else
		return -1;

	aac_frame_length += 7;

	header[0] = 0xff;
	header[1] = 0xf9;
	
	header[2] = ((profile&0x03)<<6) | ((sample_rate_index&0x0f)<<2) | ((channel_configuration&0x07)>>2);
	header[3] = ((channel_configuration&0x07)<<6) | ((aac_frame_length&0x1800)>>11);
	header[4] = ((aac_frame_length&0x07f8)>>3)&0x00ff;
	header[5] = ((aac_frame_length&0x0007)<<5) | ((adts_buffer_fullness&0x07c0)>>6);
	header[6] = ((adts_buffer_fullness&0x0003f)<<2);

//bail:
	return err;
}


#if 0		//NEON assembly debug code
#include "../../kinoma/kinoma-ipp-lib/kinoma_log.h"
FILE *fErr = NULL;

void alpha_blend_255_16rgbse_c( unsigned char *s, unsigned char *d, unsigned char *color, int width0, int height, int srb, int drb );
void alpha_blend_generic_16rgbse_c( unsigned char *s, unsigned char *d, unsigned char *color, int width0, int height, int srb, int drb );
void alpha_blend_generic_32argb_c( unsigned char *s, unsigned char *d, unsigned char *color, int width0, int height, int srb, int drb );
void alpha_blend_255_32argb_c( unsigned char *s, unsigned char *d, unsigned char *color, int width0, int height, int srb, int drb );
void verify( unsigned char *src0, unsigned long *dst0, unsigned char *alpha_color,int width0, int height0, int srb, int drb);

void my_check_reg(int stop_flag, int idx, unsigned char *data )
{
	int i;
	long *sp   = (long *)data;
	long *core = (long *)(data + 256);
	
	dlog("\n");
	dlog("BreakPoint: %d\n", idx);
	
	if( stop_flag == 0 )
		return;
	
	dlog("\n");
	dlog("NEON Registers:\n");
	for( i = 0; i < 32; i += 2 )
	{
		dlog( "D%2d|%2d:  %2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x | %2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",
				i, i+1,
				data[0]  , data[1] , data[2] ,  data[3] ,  data[4] ,  data[5] ,  data[6] ,  data[7],
				data[8]  , data[9] , data[10] , data[11] , data[12] , data[13] , data[14] , data[15]  );
		data += 16;
	}
	
	dlog("\n");
	dlog("Core Registers:\n");
	dlog( "r0 :  %8x\n", (int)core[0] );
	dlog( "r1 :  %8x\n", (int)core[1] );
	dlog( "r2 :  %8x\n", (int)core[2] );
	dlog( "r3 :  %8x\n", (int)core[3] );
	dlog( "r4 :  %8x\n", (int)core[4] );
	dlog( "r5 :  %8x\n", (int)core[5] );
	dlog( "r6 :  %8x\n", (int)core[6] );
	dlog( "r7 :  %8x\n", (int)core[7] );
	dlog( "r8 :  %8x\n", (int)core[8] );
	dlog( "r9 :  %8x\n", (int)core[9] );
	dlog( "r10:  %8x\n", (int)core[10] );
	dlog( "r11:  %8x\n", (int)core[11] );
	dlog( "r12:  %8x\n", (int)core[12] );
	dlog( "r13:  %8x\n", (int)sp );
	dlog( "r14:  %8x\n", (int)core[13] );
	
	dlog("\n");
	
	if( stop_flag == 66 )
		exit(0);
}


void my_check_mem( int idx, unsigned char *addr0, int offset, int size )
{
	int i;
	unsigned char *addr = addr0 + offset;
	
	dlog("\n");
	dlog("BreakPoint: %d\n", idx);
	dlog("\n");
	
	dlog("idx: %d, addr0: %x, offset: %d, size: %d\n", (int)idx,  (int)addr0, (int)offset, (int)size);
	
	dlog( "Addr:%8x: ",  (int)addr );
	for( i = 0; i < size; i++ )\
		dlog_0( "%2x, ", addr[i] );
	dlog_0("\n");
	
	dlog("\n");
}

void print_mem_char( unsigned char *d, int rb, int width, int height )
{
	int i, j;
	
	dlog_0( "alpha map:: width: %d, height: %d, rb: %d\n", width, height, rb )
	for( i = 0;i < height; i++ )
	{
		for( j = 0; j < width; j++ )
		{
			dlog_0( "%8x,", (int)*d );
			d++;
		}
		dlog_0( "\n" )
		d += rb;
	}
	
	dlog_0( "\n" );
}


void print_mem_short( short *d, int rb, int width, int height )
{
	int i, j;
	
	dlog_0( "dst map:: width: %d, height: %d, rb: %d\n", width, height, rb )
	for( i = 0;i < height; i++ )
	{
		for( j = 0; j < width; j++ )
		{
			dlog_0( "%8x,", (int)*d );
			d++;
		}
		dlog_0( "\n" )
		d += (rb>>1);
	}
	
	dlog_0( "\n" );
	dlog_0( "\n" );
}


void print_mem_1( unsigned char *d, int width, int height )
{
	int i, j;
	
	for( i = 0; i < height; i++ )
	{
		for( j = 0; j < width; j++ )
		{
			int idx = i*width + j;
			
			dlog_0( "%8x, ", (int)d[idx] );
		}
		dlog_0( "\n");
	}
	
	dlog_0( "\n");
}

void print_mem_2( unsigned short *d, int width, int height )
{
	int i, j;
	
	for( i = 0; i < height; i++ )
	{
		for( j = 0; j < width; j++ )
		{
			int idx = i*width + j;
			
			dlog_0( "%8x, ", (int)d[idx] );
		}
		dlog_0( "\n");
	}
	
	dlog_0( "\n");
}

void print_mem_4( unsigned long *d, int width, int height )
{
	int i, j;
	
	for( i = 0; i < height; i++ )
	{
		for( j = 0; j < width; j++ )
		{
			int idx = i*width + j;
			
			dlog_0( "%8x, ", (int)d[idx] );
		}
		dlog_0( "\n");
	}
	
	dlog_0( "\n");
}


#define DST_BYTES	4
#define DST_TYPE	unsigned long
#define DST_PRINT	print_mem_4
void verify( unsigned char *src0, DST_TYPE *dst0, unsigned char *alpha_color,int width0, int height0, int srb, int drb)
{
	int src_size = width0 * height0;
	int dst_size = src_size * DST_BYTES;
	unsigned char *src_copy0  = (unsigned char *)malloc( src_size );
	DST_TYPE *dst_c0    = (DST_TYPE *)malloc( dst_size );
	DST_TYPE *dst_a0    = (DST_TYPE *)malloc( dst_size );
	
	if( src_copy0 == NULL || dst_c0 == NULL || dst_a0 == NULL )
	{
		dlog( "malloc faulure!!!\n");
		exit(-1);
	}
	
	dlog( "verify: copy\n");
	{
		unsigned char *src_copy = src_copy0;
		DST_TYPE *dst_c = dst_c0;
		DST_TYPE *dst_a = dst_a0;
		
		unsigned char *src  = src0;
		DST_TYPE *dst = dst0;
		
		int				width     = width0;
		int				height    = height0;
		
		while(1)
		{
			width = width0;
			
			while(1)
			{
				DST_TYPE	p0	= *dst;
				unsigned char	ss	= *src;
				
				*dst_c = p0;
				*dst_a = p0;
				dst_c++;
				dst_a++;
				dst++;
				
				*src_copy = ss;
				src_copy++;
				src++;
				
				width--;
				if( width == 0 )
					break;
			}
			
			height--;
			if( height == 0 )
				break;
			
			src += srb;
			dst += (drb>>(DST_BYTES>>1));
		}
	}
	dlog( "verify: blend\n");
	dlog( "original dst:\n") ;
	DST_PRINT( (DST_TYPE *)dst_c0, width0, height0 );
	
	
	alpha_blend_generic_32argb_c      (		src_copy0, (unsigned char *)dst_c0, alpha_color, width0, height0, 0, 0 );
	alpha_blend_generic_32argb_arm_v7 (		src_copy0, (unsigned char *)dst_a0, alpha_color, width0, height0, 0, 0 );
	
	
	dlog( "src:\n" );
	print_mem_1( src_copy0, width0, height0 );
	
	dlog( "c refence:\n") ;
	DST_PRINT( (DST_TYPE *)dst_c0, width0, height0 );
	
	dlog( "arm v7:\n" );
	DST_PRINT( (DST_TYPE *)dst_a0, width0, height0 );
	
	dlog( "verify: compare\n");
	{
		int i, j;
		
		for( i = 0; i < height0; i++ )
			for( j = 0; j < width0; j++ )
			{
				int idx = i*width0 + j;
				
				if( dst_c0[idx] != dst_a0[idx] )
				{
					dlog( "diff found: x:%d, y:%d, c: %8x, a: %8x\n", j, i, (int)dst_c0[idx], (int)dst_a0[idx] );
					
					exit(-1);
				}
			}
	}
	
	dlog( "verify: good\n");
	dlog( "\n");
	
	if( src_copy0 != NULL ) free( src_copy0 );
	if( dst_c0 != NULL )	free( dst_c0 );
	if( dst_a0 != NULL )	free( dst_a0 );
}


#endif





#ifdef BNIE_LOG
#include "kinoma_log_debug.h"

FILE *fErr = NULL;

defineFillProc(fillColor16_verify);
defineFillProc(fillColor32_verify);



void my_check_reg(int stop_flag, int idx, unsigned char *data )
{
	int i;
	long *sp   = (long *)data;
	long *core = (long *)(data + 256);
	unsigned char *addr;
	
	dlog("\n");
	dlog("BreakPoint: %d\n", idx);
	
	if( stop_flag == 0 )
		return;
	
	dlog("\n");
	dlog("NEON Registers:\n");
	for( i = 0; i < 32; i += 2 )
	{
		dlog( "D%2d|%2d:  %2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x | %2x, %2x, %2x, %2x, %2x, %2x, %2x, %2x\n",
				i, i+1,
				data[0]  , data[1] , data[2] ,  data[3] ,  data[4] ,  data[5] ,  data[6] ,  data[7],
				data[8]  , data[9] , data[10] , data[11] , data[12] , data[13] , data[14] , data[15]  );
		data += 16;
	}
	
	dlog("\n");
	dlog("Core Registers:\n");
	dlog( "r0 :  %8x\n", core[0] );
	dlog( "r1 :  %8x\n", core[1] );
	dlog( "r2 :  %8x\n", core[2] );
	dlog( "r3 :  %8x\n", core[3] );
	dlog( "r4 :  %8x\n", core[4] );
	dlog( "r5 :  %8x\n", core[5] );
	dlog( "r6 :  %8x\n", core[6] );
	dlog( "r7 :  %8x\n", core[7] );
	dlog( "r8 :  %8x\n", core[8] );
	dlog( "r9 :  %8x\n", core[9] );
	dlog( "r10:  %8x\n", core[10] );
	dlog( "r11:  %8x\n", core[11] );
	dlog( "r12:  %8x\n", core[12] );
	dlog( "r13:  %8x\n", sp );
	dlog( "r14:  %8x\n", core[13] );
	
	dlog("\n");
	
	if( stop_flag == 66 )
		exit(0);
}


void my_check_mem( int idx, unsigned char *addr0, int offset, int size )
{
	int i;
	unsigned char *addr = addr0 + offset;
	
	dlog("\n");
	dlog("BreakPoint: %d\n", idx);
	dlog("\n");
	
	dlog("idx: %d, addr0: %x, offset: %d, size: %d\n", (int)idx,  (int)addr0, (int)offset, (int)size);
	
	dlog( "Addr:%8x: ",  (int)addr );
	for( i = 0; i < size; i++ )\
		dlog_0( "%2x, ", addr[i] );
	dlog_0("\n");
	
	dlog("\n");
}
#endif




#if 0//def BNIE_LOG
#define 	PRINT_SATAUS( original, pix )										\
{																				\
dlog( "width_progress:%d, height_progress:%d, original:%x, pix:%x\n",	\
(int)width_progress, (int)height_progress, (int)original, (int)pix )				\
}



void fillColor32_verify(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	UInt32 *dst32 = (UInt32 *)dst;
	UInt32 c4 = *(UInt32 *)src;
    UInt32 stragglers = width & 15;
	int height_progress = 0;
	int original = 0;
    UNUSED(bpp);
    UNUSED(state);
	
    width -= stragglers;
    width >>= 4;
	
	do {
		UInt32 s = stragglers, w = width;
		int width_progress = 0;
		
		while (s--)
		{
			original = *dst32;
			if( original != c4 )
			{
				dlog( "first several pixels, dst32:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
		}
		
		
		while (w--) {
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "0/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "1/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "2/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "3/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "4/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "5/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "6/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "7/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "8/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "9/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "10/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "11/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "12/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "13/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "14/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
			
			original = *dst32;
			if( original != c4 )
			{
				dlog( "15/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, c4 );
			}
			width_progress++;
			
			*dst32++ = c4;
		}
		
		height_progress++;
		
		dst32 = (UInt32 *)(dstRowBump + (char *)dst32);
	} while (--height);
}




void fillColor16_verify(UInt32 height, SInt32 dstRowBump, const unsigned char *src, unsigned char *dst, UInt32 width, UInt16 bpp, void *state)
{
	UInt16 *dst16 = (UInt16 *)dst;
	UInt16 c2 = *(UInt16 *)src;
	UInt32 twoPixels = (c2 << 16) | c2;
	int height_progress = 0;
	int original = 0;
	
    UNUSED(bpp);
    UNUSED(state);
	
	do {
		UInt32 w = width;
		UInt32 width2;
		UInt32 *dst32;
		int width_progress = 0;
		
		
		// make sure we are long aligned
		if (2 & (long)dst16) {
			original = *dst16;
			if( original != c2 )
			{
				dlog( "first 2 pixels, dst16:%x\n", (int)dst16);
				PRINT_SATAUS( original, c2 );
			}
			
			*dst16++ = c2;
			width_progress++;
			
			w--;
		}
		
		// blast longs
		width2 = w >> 1;
		dst32 = (UInt32 *)dst16;
		while (width2 & 7) {
			original = *dst32;
			if( original != twoPixels )
			{
				dlog( "0 to 15 pixels, dst16:%x\n", (int)dst16);
				PRINT_SATAUS( original, twoPixels );
			}
			
			
			*dst32++ = twoPixels;
			width2--;
			
			width_progress += 2;
			
		}
		
		width2 = width2 >> 3;
		while (width2--) {
			original = *dst32;
			if( original != twoPixels )
			{
				dlog( "01/16 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, twoPixels );
			}
			*dst32++ = twoPixels;
			width_progress += 2;
			
			original = *dst32;
			if( original != twoPixels )
			{
				dlog( "23/4*8 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, twoPixels );
			}
			*dst32++ = twoPixels;
			width_progress += 2;
			
			original = *dst32;
			if( original != twoPixels )
			{
				dlog( "45/4*8 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, twoPixels );
			}
			*dst32++ = twoPixels;
			width_progress += 2;
			
			original = *dst32;
			if( original != twoPixels )
			{
				dlog( "67/4*8 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, twoPixels );
			}
			*dst32++ = twoPixels;
			width_progress += 2;
			
			original = *dst32;
			if( original != twoPixels )
			{
				dlog( "89/4*8 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, twoPixels );
			}
			*dst32++ = twoPixels;
			width_progress += 2;
			
			original = *dst32;
			if( original != twoPixels )
			{
				dlog( "ab/4*8 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, twoPixels );
			}
			*dst32++ = twoPixels;
			width_progress += 2;
			
			original = *dst32;
			if( original != twoPixels )
			{
				dlog( "cd/4*8 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, twoPixels );
			}
			*dst32++ = twoPixels;
			width_progress += 2;
			
			original = *dst32;
			if( original != twoPixels )
			{
				dlog( "ef/4*8 pixels, dst16:%x\n", (int)dst32);
				PRINT_SATAUS( original, twoPixels );
				//exit(0);
			}
			*dst32++ = twoPixels;
			width_progress += 2;
			
		}
		
		// pick up the straggler
		if (w & 1) {
			original = *(UInt16 *)dst32;
			if( original != c2 )
			{
				PRINT_SATAUS( original, c2 );
			}
			*(UInt16 *)dst32 = c2;
			dst32 = (UInt32 *)(2 + (char *)dst32);
			width_progress++;
		}
		
		dst16 = (UInt16 *)(dstRowBump + (char *)dst32);
		
		height_progress++;
		
	} while (--height);
}
#endif
