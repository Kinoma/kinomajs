/***************************************************************************************** 
Copyright (c) 2009, Marvell International Ltd. 
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Marvell nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY MARVELL ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL MARVELL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************************/


#ifndef _CODECDEF_H_
#define _CODECDEF_H_

/* Include Intel IPP external header file(s). */
#include "ippdefs.h"	/* General IPP external header file*/

#ifdef __cplusplus
extern "C" {
#endif


/***** Data Types, Data Structures and Constants ******************************/

/***** Common Data Types *****/

/* Codec Status Code */
typedef enum {

	/* Audio return code */
	IPP_STATUS_INIT_ERR		= -9999,
	IPP_STATUS_INIT_OK,				
	IPP_STATUS_BUFFER_UNDERRUN,	
	IPP_STATUS_FRAME_COMPLETE,		
	IPP_STATUS_BS_END,				
	IPP_STATUS_FRAME_ERR,	
	IPP_STATUS_FRAME_HEADER_INVALID,
	IPP_STATUS_FRAME_UNDERRUN,

	/* Mpeg4 return code*/
	IPP_STATUS_MP4_SHORTHEAD,

	/* Audio return code*/
	IPP_STATUS_READEVENT,

	/* Signal return code */
	IPP_STATUS_DTMF_NOTSUPPORTEDFS  = -200, /* DTMF: Not supported sample rate */


	/* General return code*/
    IPP_STATUS_TIMEOUT_ERR      = -13,
    IPP_STATUS_STREAMFLUSH_ERR  = -12,
    IPP_STATUS_BUFOVERFLOW_ERR  = -11,
    IPP_STATUS_NOTSUPPORTED_ERR = -10,
    IPP_STATUS_MISALIGNMENT_ERR = -9,
    IPP_STATUS_BITSTREAM_ERR    = -8,
	IPP_STATUS_INPUT_ERR		= -7,
	IPP_STATUS_SYNCNOTFOUND_ERR	= -6,
    IPP_STATUS_BADARG_ERR       = -5,
    IPP_STATUS_NOMEM_ERR        = -4,
	IPP_STATUS_ERR				= -2,
    IPP_STATUS_NOERR            = 0,

    /* Warnings */
    IPP_STATUS_NOTSUPPORTED,

    /* JPEG return code */
    IPP_STATUS_JPEG_EOF,
	IPP_STATUS_JPEG_CONTINUE,
	/* JPEG stream out */
	IPP_STATUS_OUTPUT_DATA,
	/* JPEG stream in */
	IPP_STATUS_NEED_INPUT,

    /* H.264 return code */
    IPP_STATUS_NEW_VIDEO_SEQ,
    IPP_STATUS_BUFFER_FULL,
    
    /* GIF return code */
	IPP_STATUS_GIF_FINISH = 100,
	IPP_STATUS_GIF_MORE,
	IPP_STATUS_GIF_NOIMAGE,

    /* MPEG2 mp decoder return. */
	IPP_STATUS_FATAL_ERR = 200,
	IPP_STATUS_FIELD_PICTURE_TOP,
	IPP_STATUS_FIELD_PICTURE_BOTTOM,
    /* vmeta decoder return */
    IPP_STATUS_NEED_OUTPUT_BUF      = 300,
    IPP_STATUS_RETURN_INPUT_BUF     = 301,
    IPP_STATUS_END_OF_STREAM        = 302,
    IPP_STATUS_WAIT_FOR_EVENT       = 303,
    IPP_STATUS_END_OF_PICTURE       = 304
} IppCodecStatus;


/* Bitstream */
typedef struct _IppBitstream {
    Ipp8u *     pBsBuffer;
    int         bsByteLen;
    Ipp8u *     pBsCurByte;
    int         bsCurBitOffset;
} IppBitstream;


/* Sound */
typedef struct _IppSound {
    Ipp16s *    pSndFrame;
    int         sndLen;
    int         sndChannelNum;
    int         sndSampleRate;
} IppSound;


/* Picture */
#define IPP_MAXPLANENUM 5

typedef struct _IppPicture {
    void *      ppPicPlane[IPP_MAXPLANENUM];
    int         picWidth;
    int         picHeight;
    int         picPlaneStep[IPP_MAXPLANENUM];
    int         picPlaneNum;
    int         picChannelNum;
    int         picFormat;
    IppiRect    picROI;
	Ipp32s      picOrderCnt;
    Ipp64s      picTimeStamp;
    Ipp32u      picStatus;      /* displayed/reference/short/long/non existent */
} IppPicture;


/* Color format */
typedef enum {
	IPP_BINARY              = 0,
    IPP_GRAY                = 1,
    IPP_YCbCr411            = 2,
    IPP_YCbCrB4114          = 3,
    IPP_YCbCrBA41144        = 4,
    IPP_YCbCr422            = 5,
    IPP_YCbCr444            = 6,
    IPP_BGR888              = 7,
    IPP_BGRA8888            = 8,
    IPP_BGR565              = 9,
    IPP_BGR555              = 10,
    IPP_YCbCr422I           = 11,

    IPP_COLOR_FMT_UNAVAIL   = 0x7fffffff
} IppColorFormat;

typedef int (*MiscMallocCallback)(void**, int, unsigned char);
typedef int (*MiscCallocCallback)(void**, int, unsigned char);
typedef int (*MiscFreeCallback)(void**);
typedef int (*MiscStreamReallocCallback)(void**, int, int);
typedef int (*MiscStreamFlushCallback)(void*, void*, int, int);
typedef int (*MiscFileSeekCallback)(void *, int, int);
typedef int (*MiscFileReadCallback)(void **, int, int, void *);
typedef int (*MiscWriteFileCallBack)(void *, int, int, void *);

typedef struct _MiscGeneralCallbackTable
{
    MiscMallocCallback	fMemMalloc;
    MiscCallocCallback	fMemCalloc;
	MiscFreeCallback	fMemFree;
	MiscStreamReallocCallback	fStreamRealloc;
    MiscStreamFlushCallback		fStreamFlush;
	MiscFileSeekCallback		fFileSeek;
	MiscFileReadCallback		fFileRead;
	MiscWriteFileCallBack		fFileWrite;
} MiscGeneralCallbackTable;


/***** Multimedia Codec Functions *************************************************/

#if !defined( IPPCODECAPI )
	#define	IPPCODECAPI	IPPAPI
#endif

#if !defined( IPPCODECFUN )
	#define IPPCODECFUN(type, name)		extern type __STDCALL name
#endif

#if !defined( IPPCODECEXAPI )
	#if defined( WINCE ) || defined( _WIN32 ) || defined( _WIN64 )
		#define IPPCODECEXAPI( type,name,arg )	extern type __STDCALL name arg;
	#else
		#define IPPCODECEXAPI( type,name,arg )	__attribute__((visibility("default"))) extern type __STDCALL name arg;
	#endif
#endif

#if !defined( IPPCODECEXFUN )
	#if defined( WINCE ) || defined( _WIN32 ) || defined( _WIN64 )
		#define IPPCODECEXFUN(type, name)		extern type __STDCALL name
	#else
		#define IPPCODECEXFUN(type, name)		__attribute__((visibility("default"))) extern type __STDCALL name
	#endif
#endif



#ifdef __cplusplus
}
#endif

#endif    /* #ifndef _CODECDEF_H_ */

/* EOF */


