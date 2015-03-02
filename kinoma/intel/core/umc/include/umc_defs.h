/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __UMC_DEFS_H__
#define __UMC_DEFS_H__

// This file contains defines which will switch on/off support of
// codecs and renderers on application level


/*
// Windows on IA32
*/

#if defined(WIN32) && defined(_WIN32_WINNT)

    // readers/writters
    #define UMC_ENABLE_FILE_READER
    #define UMC_ENABLE_FIO_READER
    #define UMC_ENABLE_FILE_WRITER

    // video renderers
    #define UMC_ENABLE_DX_VIDEO_RENDER
    #define UMC_ENABLE_BLT_VIDEO_RENDER
    #define UMC_ENABLE_GDI_VIDEO_RENDER
    #define UMC_ENABLE_FW_VIDEO_RENDER

    // audio renderers
    #define UMC_ENABLE_WINMM_AUDIO_RENDER
    #define UMC_ENABLE_DSOUND_AUDIO_RENDER
    #define UMC_ENABLE_FW_AUDIO_RENDER

    // splitters
    #define UMC_ENABLE_AVI_SPLITTER
    #define UMC_ENABLE_MPEG2_SPLITTER
    #define UMC_ENABLE_MP4_SPLITTER

    // video decoders
    #define UMC_ENABLE_DV_VIDEO_DECODER
    #define UMC_ENABLE_H261_VIDEO_DECODER
    #define UMC_ENABLE_H263_VIDEO_DECODER
    #define UMC_ENABLE_H264_VIDEO_DECODER
    #define UMC_ENABLE_MPEG2_VIDEO_DECODER
    #define UMC_ENABLE_MPEG4_VIDEO_DECODER
    #define UMC_ENABLE_MJPEG_VIDEO_DECODER

    // audio decoders
    #define UMC_ENABLE_AAC_INT_AUDIO_DECODER
    #define UMC_ENABLE_MP3_INT_AUDIO_DECODER
    #define UMC_ENABLE_AAC_AUDIO_DECODER
    #define UMC_ENABLE_AC3_AUDIO_DECODER
    #define UMC_ENABLE_MP3_AUDIO_DECODER
    #define UMC_ENABLE_AMR_AUDIO_DECODER
#endif // Win32 on IA32


/*
// WindowsCE on IA32
*/

#if defined(_WIN32_WCE) && (defined(x86) || defined(_X86_))

    // readers/writters
    #define UMC_ENABLE_FIO_READER
    #define UMC_ENABLE_FILE_WRITER

    // video renderers
    #define UMC_ENABLE_GDI_VIDEO_RENDER
    #define UMC_ENABLE_FW_VIDEO_RENDER

//  #define UMC_ENABLE_DXWCE_VIDEO_RENDER

    // audio renderers
    #define UMC_ENABLE_WINMM_AUDIO_RENDER
    #define UMC_ENABLE_FW_AUDIO_RENDER

    // splitters
    #define UMC_ENABLE_AVI_SPLITTER
    #define UMC_ENABLE_MPEG2_SPLITTER
    #define UMC_ENABLE_MP4_SPLITTER

    // video decoders
    #define UMC_ENABLE_DV_VIDEO_DECODER
    #define UMC_ENABLE_H261_VIDEO_DECODER
    #define UMC_ENABLE_H263_VIDEO_DECODER
    #define UMC_ENABLE_H264_VIDEO_DECODER
    #define UMC_ENABLE_MPEG2_VIDEO_DECODER
    #define UMC_ENABLE_MPEG4_VIDEO_DECODER
    #define UMC_ENABLE_MJPEG_VIDEO_DECODER

    // audio decoders
    #define UMC_ENABLE_AAC_INT_AUDIO_DECODER
    #define UMC_ENABLE_MP3_INT_AUDIO_DECODER
    #define UMC_ENABLE_AAC_AUDIO_DECODER
    #define UMC_ENABLE_AC3_AUDIO_DECODER
    #define UMC_ENABLE_MP3_AUDIO_DECODER
    #define UMC_ENABLE_AMR_AUDIO_DECODER
#endif // WinCE on IA32


/*
// Windows on EM64T
*/

#if defined(WIN64) && defined (_AMD64_)

    // readers/writters
    #define UMC_ENABLE_FILE_READER
    #define UMC_ENABLE_FILE_WRITER

    // video renderers
    #define UMC_ENABLE_GDI_VIDEO_RENDER
    #define UMC_ENABLE_FW_VIDEO_RENDER

    // audio renderers
    #define UMC_ENABLE_WINMM_AUDIO_RENDER
    #define UMC_ENABLE_FW_AUDIO_RENDER

    // splitters
    #define UMC_ENABLE_AVI_SPLITTER
    #define UMC_ENABLE_MPEG2_SPLITTER
    #define UMC_ENABLE_MP4_SPLITTER

    // video decoders
    #define UMC_ENABLE_DV_VIDEO_DECODER
    #define UMC_ENABLE_H264_VIDEO_DECODER
    #define UMC_ENABLE_MPEG2_VIDEO_DECODER
    #define UMC_ENABLE_MPEG4_VIDEO_DECODER
    #define UMC_ENABLE_MJPEG_VIDEO_DECODER
    #define UMC_ENABLE_H263_VIDEO_DECODER
    #define UMC_ENABLE_H261_VIDEO_DECODER

    // audio decoders
    #define UMC_ENABLE_AAC_INT_AUDIO_DECODER
    #define UMC_ENABLE_MP3_INT_AUDIO_DECODER
    #define UMC_ENABLE_AAC_AUDIO_DECODER
    #define UMC_ENABLE_AC3_AUDIO_DECODER
    #define UMC_ENABLE_MP3_AUDIO_DECODER
    #define UMC_ENABLE_AMR_AUDIO_DECODER

#endif // Winx64 on EM64T


/*
// Windows on IA64
*/

#if defined(WIN64) && !defined (_AMD64_)

    // readers/writters
    #define UMC_ENABLE_FILE_READER
    #define UMC_ENABLE_FILE_WRITER

    // video renderers
    #define UMC_ENABLE_GDI_VIDEO_RENDER
    #define UMC_ENABLE_FW_VIDEO_RENDER

    // audio renderers
    #define UMC_ENABLE_WINMM_AUDIO_RENDER
    #define UMC_ENABLE_FW_AUDIO_RENDER

    // splitters
    #define UMC_ENABLE_AVI_SPLITTER
    #define UMC_ENABLE_MPEG2_SPLITTER
    #define UMC_ENABLE_MP4_SPLITTER

    // video decoders
    #define UMC_ENABLE_DV_VIDEO_DECODER
    #define UMC_ENABLE_H261_VIDEO_DECODER
    #define UMC_ENABLE_H263_VIDEO_DECODER
    #define UMC_ENABLE_H264_VIDEO_DECODER
    #define UMC_ENABLE_MPEG2_VIDEO_DECODER
    #define UMC_ENABLE_MPEG4_VIDEO_DECODER
    #define UMC_ENABLE_MJPEG_VIDEO_DECODER

    // audio decoders
    #define UMC_ENABLE_AAC_INT_AUDIO_DECODER
    #define UMC_ENABLE_MP3_INT_AUDIO_DECODER
    #define UMC_ENABLE_AAC_AUDIO_DECODER
    #define UMC_ENABLE_AC3_AUDIO_DECODER
    #define UMC_ENABLE_MP3_AUDIO_DECODER
    #define UMC_ENABLE_AMR_AUDIO_DECODER

#endif // Win64 on IA64


/*
// WindowsCE on IXP4xx
*/

#if defined(_WIN32_WCE) && (defined (ARM) || defined(_ARM_))

    // readers/writters
    #define UMC_ENABLE_FIO_READER
    #define UMC_ENABLE_FILE_WRITER

    // video renderers
    #define UMC_ENABLE_MTWGX_VIDEO_RENDER
    #define UMC_ENABLE_FW_VIDEO_RENDER

    // audio renderers
    #define UMC_ENABLE_WINMM_AUDIO_RENDER
    #define UMC_ENABLE_FW_AUDIO_RENDER

    // splitters
    #define UMC_ENABLE_AVI_SPLITTER
    #define UMC_ENABLE_MPEG2_SPLITTER
    #define UMC_ENABLE_MP4_SPLITTER

    // video decoders
    #define UMC_ENABLE_DV_VIDEO_DECODER
    #define UMC_ENABLE_H261_VIDEO_DECODER
    #define UMC_ENABLE_H263_VIDEO_DECODER
    #define UMC_ENABLE_H264_VIDEO_DECODER
    #define UMC_ENABLE_MPEG2_VIDEO_DECODER
    #define UMC_ENABLE_MPEG4_VIDEO_DECODER
    #define UMC_ENABLE_MJPEG_VIDEO_DECODER

    // audio decoders
    #define UMC_ENABLE_AAC_INT_AUDIO_DECODER
    #define UMC_ENABLE_MP3_INT_AUDIO_DECODER
    #define UMC_ENABLE_AMR_AUDIO_DECODER

#endif // WinCE on IXP4xx


//----------------------------------------------------------------------------


/*
// Linux on IA32
*/

#if defined(LINUX32) && !(defined (_ARM_) || defined(ARM))

    // readers/writters
    #define UMC_ENABLE_FILE_READER
    #define UMC_ENABLE_FILE_WRITER

    // video renderers
#ifndef OSX32
   #define UMC_ENABLE_FB_VIDEO_RENDER
   #define UMC_ENABLE_SDL_VIDEO_RENDER
#endif /* OSX32 */
    #define UMC_ENABLE_FW_VIDEO_RENDER

    // audio renderers
#ifndef OSX32
    #define UMC_ENABLE_OSS_AUDIO_RENDER
    #define UMC_ENABLE_SDL_AUDIO_RENDER
#endif /* OSX32 */
    #define UMC_ENABLE_FW_AUDIO_RENDER

    // splitters
    #define UMC_ENABLE_AVI_SPLITTER
    #define UMC_ENABLE_MPEG2_SPLITTER
    #define UMC_ENABLE_MP4_SPLITTER

    // video decoders
    #define UMC_ENABLE_DV_VIDEO_DECODER
    #define UMC_ENABLE_H261_VIDEO_DECODER
    #define UMC_ENABLE_H263_VIDEO_DECODER
    #define UMC_ENABLE_H264_VIDEO_DECODER
    #define UMC_ENABLE_MPEG2_VIDEO_DECODER
    #define UMC_ENABLE_MPEG4_VIDEO_DECODER
    #define UMC_ENABLE_MJPEG_VIDEO_DECODER

    // audio decoders
    #define UMC_ENABLE_AAC_INT_AUDIO_DECODER
    #define UMC_ENABLE_MP3_INT_AUDIO_DECODER
    #define UMC_ENABLE_AAC_AUDIO_DECODER
    #define UMC_ENABLE_AC3_AUDIO_DECODER
    #define UMC_ENABLE_MP3_AUDIO_DECODER
    #define UMC_ENABLE_AMR_AUDIO_DECODER

#endif // Linux on IA32

/*
// Linux on EM64T
*/

/*
// Linux on IA64
*/

/*
// Linux on IXP4xx
*/

#if defined(LINUX32) && (defined (ARM) || defined(_ARM_))

    // readers/writters
    #define UMC_ENABLE_FIO_READER
    #define UMC_ENABLE_FILE_WRITER

    // video renderers
    #define UMC_ENABLE_FB_VIDEO_RENDER
    #define UMC_ENABLE_FW_VIDEO_RENDER
//  #define UMC_ENABLE_SDL_VIDEO_RENDER

    // audio renderers
//    #define UMC_ENABLE_OSS_AUDIO_RENDER
//    #define UMC_ENABLE_SDL_AUDIO_RENDER
    #define UMC_ENABLE_FW_AUDIO_RENDER

    // splitters
    #define UMC_ENABLE_AVI_SPLITTER
    #define UMC_ENABLE_MPEG2_SPLITTER
    #define UMC_ENABLE_MP4_SPLITTER

    // video decoders
    #define UMC_ENABLE_DV_VIDEO_DECODER
    #define UMC_ENABLE_H261_VIDEO_DECODER
    #define UMC_ENABLE_H263_VIDEO_DECODER
    #define UMC_ENABLE_H264_VIDEO_DECODER
    #define UMC_ENABLE_MPEG2_VIDEO_DECODER
    #define UMC_ENABLE_MPEG4_VIDEO_DECODER
    #define UMC_ENABLE_MJPEG_VIDEO_DECODER

    // audio decoders
    #define UMC_ENABLE_AAC_INT_AUDIO_DECODER
    #define UMC_ENABLE_MP3_INT_AUDIO_DECODER
    #define UMC_ENABLE_AMR_AUDIO_DECODER

#endif // Linux on IXP4xx


#endif // __UMC_DEFS_H__

