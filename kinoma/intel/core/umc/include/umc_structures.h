/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_STRUCTURES_H__
#define __UMC_STRUCTURES_H__

//***
#include "kinoma_ipp_env.h"

#include "vm_types.h"
//#include "umc_malloc.h"

#define BSWAP16(x) \
    (vm_var16) ((x) >> 8 | (x) << 8)

#define BSWAP32(x) \
    (vm_var32)(((x) << 24) + \
    (((x)&0xff00) << 8) + \
    (((x) >> 8)&0xff00) + \
    ((vm_var32)(x) >> 24))

#define BSWAP64(x) \
    (vm_var64)(((x) << 56) + \
    (((x)&0xff00) << 40) + \
    (((x)&0xff0000) << 24) + \
    (((x)&0xff000000) << 8) + \
    (((x) >> 8)&0xff000000) + \
    (((x) >> 24)&0xff0000) + \
    (((x) >> 40)&0xff00) + \
    ((x) >> 56))

#ifdef _BIG_ENDIAN_
#   define BIG_ENDIAN_SWAP16(x) BSWAP16(x)
#   define BIG_ENDIAN_SWAP32(x) BSWAP32(x)
#   define BIG_ENDIAN_SWAP64(x) BSWAP64(x)
#   define LITTLE_ENDIAN_SWAP16(x) (x)
#   define LITTLE_ENDIAN_SWAP32(x) (x)
#   define LITTLE_ENDIAN_SWAP64(x) (x)
#else // _BIG_ENDIAN_
#   define BIG_ENDIAN_SWAP16(x) (x)
#   define BIG_ENDIAN_SWAP32(x) (x)
#   define BIG_ENDIAN_SWAP64(x) (x)
#   define LITTLE_ENDIAN_SWAP16(x) BSWAP16(x)
#   define LITTLE_ENDIAN_SWAP32(x) BSWAP32(x)
#   define LITTLE_ENDIAN_SWAP64(x) BSWAP64(x)
#endif // _BIG_ENDIAN_

// macro to create FOURCC
#ifndef DO_FOURCC
#define DO_FOURCC(ch0, ch1, ch2, ch3) \
    ( (vm_var32)(vm_byte)(ch0) | ( (vm_var32)(vm_byte)(ch1) << 8 ) | \
    ( (vm_var32)(vm_byte)(ch2) << 16 ) | ( (vm_var32)(vm_byte)(ch3) << 24 ) )
#endif // DO_FOURCC

namespace UMC
{

    enum SystemStreamType
    {
        UNDEF_STREAM            = 0x00000000, //unsupported stream type
        AVI_STREAM              = 0x00000001, //AVI RIFF
        MP4_ATOM_STREAM         = 0x00000010, //ISO/IEC 14496-14 stream
        ASF_STREAM              = 0x00000100, //ASF stream

        H26x_PURE_VIDEO_STREAM  = 0x00100000,
        H261_PURE_VIDEO_STREAM  = H26x_PURE_VIDEO_STREAM|0x00010000,
        H263_PURE_VIDEO_STREAM  = H26x_PURE_VIDEO_STREAM|0x00020000,
        H264_PURE_VIDEO_STREAM  = H26x_PURE_VIDEO_STREAM|0x00040000,

        MPEGx_SYSTEM_STREAM     = 0x00001000,                    //MPEG 1,2 - like system

        MPEG1_SYSTEM_STREAM     = MPEGx_SYSTEM_STREAM|0x00000100,//MPEG 1 system
        MPEG2_SYSTEM_STREAM     = MPEGx_SYSTEM_STREAM|0x00000200,//MPEG 2 system
        MPEG4_SYSTEM_STREAM     = MPEGx_SYSTEM_STREAM|0x00000400,//MPEG 2 system

        MPEGx_PURE_VIDEO_STREAM = MPEGx_SYSTEM_STREAM|0x00000010,//MPEG 1,2 - like pure video data
        MPEGx_PURE_AUDIO_STREAM = MPEGx_SYSTEM_STREAM|0x00000020,//MPEG 1,2 - like pure audio data
        MPEGx_PES_PACKETS_STREAM= MPEGx_SYSTEM_STREAM|0x00000040,//MPEG 1,2 - like pes packets system
        MPEGx_PROGRAMM_STREAM   = MPEGx_SYSTEM_STREAM|0x00000080,//MPEG 1,2 - like programm system
        MPEGx_TRANSPORT_STREAM  = MPEGx_SYSTEM_STREAM|0x000000c0,//MPEG 1,2 - like transport system


        MPEG1_PURE_VIDEO_STREAM = MPEG1_SYSTEM_STREAM|MPEGx_PURE_VIDEO_STREAM, //MPEG1 pure video stream
        MPEG1_PURE_AUDIO_STREAM = MPEG1_SYSTEM_STREAM|MPEGx_PURE_AUDIO_STREAM, //MPEG1 pure video stream
        MPEG1_PES_PACKETS_STREAM= MPEG1_SYSTEM_STREAM|MPEGx_PES_PACKETS_STREAM,//MPEG1 pes packets stream
        MPEG1_PROGRAMM_STREAM   = MPEG1_SYSTEM_STREAM|MPEGx_PROGRAMM_STREAM,   //MPEG1 programm stream

        MPEG2_PURE_VIDEO_STREAM = MPEG2_SYSTEM_STREAM|MPEGx_PURE_VIDEO_STREAM,//MPEG2 pure video stream
        MPEG2_PURE_AUDIO_STREAM = MPEG2_SYSTEM_STREAM|MPEGx_PURE_AUDIO_STREAM,//MPEG2 pure audio stream
        MPEG2_PES_PACKETS_STREAM= MPEG2_SYSTEM_STREAM|MPEGx_PES_PACKETS_STREAM,//MPEG2 pes packets stream
        MPEG2_PROGRAMM_STREAM   = MPEG2_SYSTEM_STREAM|MPEGx_PROGRAMM_STREAM,   //MPEG2 programm stream
        MPEG2_TRANSPORT_STREAM  = MPEG2_SYSTEM_STREAM|MPEGx_TRANSPORT_STREAM,  //MPEG2 transport stream
        MPEG2_TRANSPORT_STREAM_TTS= MPEG2_SYSTEM_STREAM|MPEGx_TRANSPORT_STREAM | 1,  //MPEG2 transport stream

        MPEG4_PURE_VIDEO_STREAM = MPEG4_SYSTEM_STREAM|MPEGx_PURE_VIDEO_STREAM,//MPEG4 pure video stream

        WEB_CAM_STREAM          = 0x00100000,
        ADIF_STREAM             = 0x00200000,
        ADTS_STREAM             = 0x00400000,
        STILL_IMAGE             = 0x00800000
    };

    enum AudioStreamType
    {
        UNDEF_AUDIO             = 0x00000000,
        PCM_AUDIO               = 0x00000001,
        LPCM_AUDIO              = 0x00000002,
        AC3_AUDIO               = 0x00000004,
        TWINVQ_AUDIO            = 0x00000008,

        MPEG1_AUDIO             = 0x00000100,
        MPEG2_AUDIO             = 0x00000200,
        MPEG_AUDIO_LAYER1       = 0x00000010,
        MPEG_AUDIO_LAYER2       = 0x00000020,
        MPEG_AUDIO_LAYER3       = 0x00000040,

        MP1L1_AUDIO             = MPEG1_AUDIO|MPEG_AUDIO_LAYER1,
        MP1L2_AUDIO             = MPEG1_AUDIO|MPEG_AUDIO_LAYER2,
        MP1L3_AUDIO             = MPEG1_AUDIO|MPEG_AUDIO_LAYER3,
        MP2L1_AUDIO             = MPEG2_AUDIO|MPEG_AUDIO_LAYER1,
        MP2L2_AUDIO             = MPEG2_AUDIO|MPEG_AUDIO_LAYER2,
        MP2L3_AUDIO             = MPEG2_AUDIO|MPEG_AUDIO_LAYER3,

        VORBIS_AUDIO            = 0x00000400,
        AAC_AUDIO               = 0x00000800,
        AAC_FMT_UNDEF           = 0x00000000,   /// Undefined stream format, the decoder have to identify by bitsream
        AAC_FMT_RAW             = 0x00000001,   /// Raw input stream format, the fisrt frame keeps init information
        AAC_FMT_EX_GA           = 0x00000010,   /// GASpecificConfig header within the first frame.
        AAC_MPEG4_STREAM        = AAC_AUDIO | AAC_FMT_RAW | AAC_FMT_EX_GA,

        AMR_NB_AUDIO            = 0x00000777,  //narrow band amr
        AMR_WB_AUDIO            = 0x00000778   //wide band amr

    };

    #define WAVE_FORMAT_SPEECH         (0x4D41)

    enum TrickModesType
    {
        UMC_TRICK_MODES_NO          = 0x00000000,
        UMC_TRICK_MODES_FORWARD     = 0x00000001,
        UMC_TRICK_MODES_FAST        = 0x00000002,
        UMC_TRICK_MODES_FASTER      = 0x00000004,
        UMC_TRICK_MODES_SLOW        = 0x00000020,
        UMC_TRICK_MODES_SLOWER      = 0x00000040,
        UMC_TRICK_MODES_REVERSE     = 0x00000200,

        UMC_TRICK_MODES_FFW_FAST    = UMC_TRICK_MODES_FAST   | UMC_TRICK_MODES_FORWARD,
        UMC_TRICK_MODES_FFW_FASTER  = UMC_TRICK_MODES_FASTER | UMC_TRICK_MODES_FORWARD,
        UMC_TRICK_MODES_SFW_SLOW    = UMC_TRICK_MODES_SLOW   | UMC_TRICK_MODES_FORWARD,
        UMC_TRICK_MODES_SFW_SLOWER  = UMC_TRICK_MODES_SLOWER | UMC_TRICK_MODES_FORWARD,

        UMC_TRICK_MODES_FR_FAST     = UMC_TRICK_MODES_FAST   | UMC_TRICK_MODES_REVERSE,
        UMC_TRICK_MODES_FR_FASTER   = UMC_TRICK_MODES_FASTER | UMC_TRICK_MODES_REVERSE,
        UMC_TRICK_MODES_SR_SLOW     = UMC_TRICK_MODES_SLOW   | UMC_TRICK_MODES_REVERSE,
        UMC_TRICK_MODES_SR_SLOWER   = UMC_TRICK_MODES_SLOWER | UMC_TRICK_MODES_REVERSE
    };

    enum AudioStreamSubType
    {
        UNDEF_AUDIO_SUBTYPE     = 0x00000000,
        AAC_LC_PROFILE          = 0x00000001,
        AAC_LTP_PROFILE         = 0x00000002,
        AAC_MAIN_PROFILE        = 0x00000004,
        AAC_SSR_PROFILE         = 0x00000008,
        AAC_HE_PROFILE          = 0x00000010
    };

    enum VideoStreamType
    {
        UNDEF_VIDEO             = 0x00000000,
        UNCOMPRESSED_VIDEO      = 0x00000001,
        MPEG1_VIDEO             = 0x00000011,
        MPEG2_VIDEO             = 0x00000012,
        MPEG4_VIDEO             = 0x00000014,
        H261_VIDEO              = 0x00000120,
        H263_VIDEO              = 0x00000140,
        H264_VIDEO              = 0x00000180,
        DIGITAL_VIDEO_SD        = 0x00001200,
        DIGITAL_VIDEO_50        = 0x00001400,
        DIGITAL_VIDEO_HD        = 0x00001800,
        WMV_VIDEO               = 0x00010000,
        MJPEG_VIDEO             = 0x00020000
    };

    enum VideoRenderType
    {
        DEF_VIDEO_RENDER = 0,
        DX_VIDEO_RENDER,
        BLT_VIDEO_RENDER,
        GDI_VIDEO_RENDER,
        GX_VIDEO_RENDER,
        SDL_VIDEO_RENDER,
        FB_VIDEO_RENDER,
        NULL_VIDEO_RENDER,
        FW_VIDEO_RENDER,
        MTWREG_VIDEO_RENDER,
        OVL2_VIDEO_RENDER,
        DXWCE_VIDEO_RENDER,
        AGL_VIDEO_RENDER
    };

    enum AudioRenderType
    {
        DEF_AUDIO_RENDER = 0,
        DSOUND_AUDIO_RENDER,
        WINMM_AUDIO_RENDER,
        OSS_AUDIO_RENDER,
        NULL_AUDIO_RENDER,
        FW_AUDIO_RENDER,
        COREAUDIO_RENDER
     };

    enum VideoStreamSubType
    {
        UNDEF_VIDEO_SUBTYPE     = 0x00000000,
        MPEG4_VIDEO_DIVX5       = 0x00000001,
        MPEG4_VIDEO_QTIME       = 0x00000002,
        DIGITAL_VIDEO_TYPE_1 = 3,
        DIGITAL_VIDEO_TYPE_2,
        MPEG4_VIDEO_DIVX3,
        MPEG4_VIDEO_DIVX4,
        AVC1_VIDEO
    };

    enum
    {
        UNDEF_MUXER             = 0x00000000,
        AUDIO_MUXER             = 0x00000001,
        VIDEO_MUXER             = 0x00000002,
        AV_MUXER                = AUDIO_MUXER|VIDEO_MUXER,

        //some muxers may have a behavior to run internal thread
        //to prohibit asynchronious muxing use this flag
        FLAG_VMUX_NO_INTERNAL_THREAD= 0x00002000,

        FLAG_VMUX_NOSIZELIMIT   = 0x00000010

    };

    enum ColorFormat
    {

        NONE    = -1,
        YV12    = 0,    // Planar Y, U, V, 8 bit per component
        NV12    ,       // Planar merged Y, U->V, 8 bit per component
        YUY2    ,       // Composite Y->U->Y->V 8 bit per component
        UYVY    ,       // Composite U->Y->V->Y 8 bit per component
        YUV411  ,       // Planar Y, U, V, 8 bit per component
        YUV420  ,       // Planar Y, U, V, 8 bit per component (trained peolpe use only)
        YUV422  ,       // Planar Y, U, V,
        YUV444  ,       // Planar Y, U, V,
        YUV420M ,       // Planar merged Y, U->V, 8 bit per component (trained peolpe use only)
        YUV422M ,       // Planar merged Y, U->V, (trained peolpe use only)
        YUV444M ,       // Planar merged Y, U->V, (trained peolpe use only)
        RGB32   ,       // Composite B->G->R->A 8 bit per component
        RGB24   ,       // Composite B->G->R 8 bit per component
        RGB565  ,       // Composite B->G->R 5 bit per B & R, 6 bit per G
        RGB555  ,       // Composite B->G->R->A, 5 bit per component, 1 bit per A
        RGB444          // Composite B->G->R->A, 4 bit per component

    };

    enum FrameType
    {
        NONE_PICTURE            = 0,
        I_PICTURE               = 1,
        P_PICTURE               = 2,
        B_PICTURE               = 3,
        D_PICTURE               = 4

    };

    enum InterlaceType
    {
        PROGRESSIVE                    = 0,
        FIELD_PICTURE                  = 1,
        INTERLEAVED_TOP_FIELD_FIRST    = 2,
        INTERLEAVED_BOTTOM_FIELD_FIRST = 3
    };

    enum // decoding flags
    {
        //this falg reccomends decoder to use UV merged(NV12) internal format
        //if decoder supports it. May be usefull if graphics accelerator
        //supports such color format as native
        FLAG_VDEC_UVMERGED      = 0x00000001,

        //receiveing this flag decoder must output decompressed data
        //in proper display order, otherwise it will output decompressed data
        //in decoding order, application is responsible to reorder frames to
        //before displying
        FLAG_VDEC_REORDER       = 0x00000004,

        //no preview required from decoder
        FLAG_VDEC_NO_PREVIEW    = 0x00000008,

        //next flag describes endian rlated properties of input data
        //when set, means that coded data should be accessed by 4-reading operations
        //for little-endian systems it means that each 4 bytes are swapped
        //i.e [0]<->[3], [1]<->[2]
        //for big-endian systems swapping is not required
        FLAG_VDEC_4BYTE_ACCESS = 0x00000100,

        //traditional, not UMC specific behaviour
        //original byte order, headers before data, return bytes consumed
        FLAG_VDEC_COMPATIBLE= 0x00001000
    };

    enum // encoding flags
    {
        // The encoder should reorder the incoming frames in the encoding order itself.
        FLAG_VENC_REORDER       = 0x00000004
    };

    enum // color space conversion flag(s)
    {
        FLAG_CCNV_NONE          = 0x00000000,                       // do not use conversion
        FLAG_CCNV_CONVERT       = 0x00000001,                       // use color conversion
        FLAG_CCNV_RESIZE_2S     = 0x00000002 | FLAG_CCNV_CONVERT,   // convert & subsampling by 2
        FLAG_CCNV_RESIZE_4S     = 0x00000004 | FLAG_CCNV_CONVERT,   // convert & subsampling by 4
        FLAG_CCNV_RESIZE_8S     = 0x00000006 | FLAG_CCNV_CONVERT,   // convert & subsampling by 8
        FLAG_CCNV_RESIZE_2X     = 0x0000000a | FLAG_CCNV_CONVERT,   // convert & oversampling by 2
        FLAG_CCNV_RESIZE_4X     = 0x0000000c | FLAG_CCNV_CONVERT,   // convert & oversampling by 4
        FLAG_CCNV_RESIZE_8X     = 0x0000000e | FLAG_CCNV_CONVERT,   // convert & oversampling by 8
        FLAG_CCNV_RESIZE_CUSTOM = 0x00000010 | FLAG_CCNV_CONVERT,   // convert & custom resize

        FLAG_CCNV_DEINTERLACE   = 0x00000001                        // perform deinterlace after conversion

    };

    enum // video renderer flags
    {
        //render inited with this flag will rerder decompressed data from decoder
        //in proper display order
        //see FLAG_VDEC_REORDER flag as pair for this
        FLAG_VREN_REORDER       = 0x00000001,
        FLAG_VREN_CONVERT       = 0x00000002,
        FLAG_VREN_USECOLORKEY   = 0x00000004
    };

    enum AudioChannelConfig
    {
        CHANNEL_FRONT_LEFT      = 0x1,
        CHANNEL_FRONT_RIGHT     = 0x2,
        CHANNEL_FRONT_CENTER    = 0x4,
        CHANNEL_LOW_FREQUENCY   = 0x8,
        CHANNEL_BACK_LEFT       = 0x10,
        CHANNEL_BACK_RIGHT      = 0x20,
        CHANNEL_FRONT_LEFT_OF_CENTER = 0x40,
        CHANNEL_FRONT_RIGHT_OF_CENTER = 0x80,
        CHANNEL_BACK_CENTER     = 0x100,
        CHANNEL_SIDE_LEFT       = 0x200,
        CHANNEL_SIDE_RIGHT      = 0x400,
        CHANNEL_TOP_CENTER      = 0x800,
        CHANNEL_TOP_FRONT_LEFT  = 0x1000,
        CHANNEL_TOP_FRONT_CENTER = 0x2000,
        CHANNEL_TOP_FRONT_RIGHT = 0x4000,
        CHANNEL_TOP_BACK_LEFT   = 0x8000,
        CHANNEL_TOP_BACK_CENTER = 0x10000,
        CHANNEL_TOP_BACK_RIGHT  = 0x20000,
        CHANNEL_RESERVED        = 0x80000000
    };

    enum // audio renderer flags
    {
        FLAG_AREN_VOID          = 0x00000001
    };

    enum // splitter flags
    {
        //invalid value
        UNDEF_SPLITTER          = 0x00000000,
        //audio splittinf required in any present in stream
        AUDIO_SPLITTER          = 0x00000001,
        //video splitting required in any present in stream
        VIDEO_SPLITTER          = 0x00000002,
        //example if setup VIDEO_SPLITTER && !set AUDIO_SPLITTER, splitter will ignore
        //any audio elementary stream, only video data request will be valid

        //audio and video splitting required if any present in stream
        AV_SPLITTER             = AUDIO_SPLITTER|VIDEO_SPLITTER,

        //main video header (sequence header) is required to return from Init
        //splitter function, application is responsible to pass it to decoder
        //as a regular video data for properly decoding consequent data
        FLAG_VSPL_VIDEO_HEADER_REQ = 0x00000010,

        //the first video frame is required to return from Init
        //splitter function, application is responsible to pass it to decoder
        //as a regular video data for properly decoding consequent data.
        //The first frame will follow main video header. This flag expands
        //splitter behavior for FLAG_VSPL_VIDEO_HEADER_REQ case
        FLAG_VSPL_VIDEO_FRAME_REQ  = 0x00000020,
        FLAG_VSPL_AUDIO_INFO_REQ   = 0x00000040,
        FLAG_VSPL_VIDEO_INFO_REQ   = 0x00000080,

        //next flag describes endian rlated properties of input data
        //when set, means that coded data should be accessed by 4-reading operations
        //for little-endian systems it means that each 4 bytes are swapped
        //i.e [0]<->[3], [1]<->[2]
        //for big-endian systems swapping is not required
        FLAG_VSPL_4BYTE_ACCESS = 0x00000100,

        //traditional, not UMC specific behaviour
        //original byte order, headers before data, return bytes consumed
        FLAG_VSPL_COMPATIBLE        = 0x00001000,
        //some splitters may have a behavior to run internal
        //to prohibit asynchronious splitting use this flag
        FLAG_VSPL_NO_INTERNAL_THREAD= 0x00002000
    };

   enum // values to select video or audio output channels
    {
        SELECT_ANY_VIDEO_PID    = 0x00000000, //ask for one of available video streams
        SELECT_ANY_AUDIO_PID    = 0x00000000, //ask for one of available audio streams
        SELECT_ALL_AUDIO_PIDS   = 0xffffffff, //ask for all of available audio streams
        SELECT_ALL_VIDEO_PIDS   = 0xffffffff  //ask for all of available video streams
    };

    enum
    {
        SINGLE_CLIENT           = 0,    // Connection oriented with single client per server
        MULTIPLE_CLIENTS,               // Connection oriented with multiple clients per server
        BROADCAST                       // Connection less mode
    };

    enum
    {
        MAXIMUM_PATH            = 1024
    };

    typedef struct sAudioStreamInfo
    {
        int channels;                                           // (int) number of audio channels
        int sample_frequency;                                   // (int) sample rate in Hz
        vm_var32 bitrate;                                       // (vm_var32) bitstream in bps
        vm_var32 bitPerSample;                                  // (vm_var32) 0 if compressed

        double duration;                                        // (double) duration of the stream

        AudioStreamType stream_type;                            // (AudioStreamType) general type of stream
        AudioStreamSubType stream_subtype;                      // (AudioStreamSubType) minor type of stream

        vm_var32 channel_mask;                                  // (vm_var32) channel mask
        vm_var32 streamPID;                                     // (vm_var32) unique ID

        bool               is_protected;                        // audio is encrypted

    } AudioStreamInfo;

    typedef struct sClipInfo
    {
        int width;                                              // (int) width of media
        int height;                                             // (int) height of media

    } ClipInfo;

    typedef struct sVideoStreamInfo
    {
        ClipInfo            clip_info;                          // (ClipInfo) size of video stream
        ColorFormat         color_format;                       // (ColorFormat) color format of uncompressed video
        vm_var32            bitrate;                            // (vm_var32) bitrate of video
        int                 aspect_ratio_width;                 // (int) video aspect ratio width
        int                 aspect_ratio_height;                // (int) video aspect ratio height
        double              framerate;                          // (double) frame rate of video
        double              duration;                           // (double) duration of media stream
        InterlaceType       interlace_type;                     // (InterlaceType) interlaced info
        VideoStreamType     stream_type;                        // (VideoStreamType) video stream type
        VideoStreamSubType  stream_subtype;                     // (VideoStreamSubType) video stream type
        vm_var32            streamPID;                          // (vm_var32) unique ID

		//***
		unsigned char 		*codecPriv;
    } VideoStreamInfo;

    typedef struct sSystemStreamInfo
    {

        double muxrate;                                         // (double) stream bitrate
        SystemStreamType stream_type;                           // (SystemStreamType) stream type

    } SystemStreamInfo;

    struct RECT
    {
        RECT():
            left(0),
            top(0),
            right(0),
            bottom(0)
        {}

        signed short left;
        signed short top;
        signed short right;
        signed short bottom;
        inline
        void SwapBigEndian()
        {
            left = BIG_ENDIAN_SWAP16(left);
            top = BIG_ENDIAN_SWAP16(top);
            right = BIG_ENDIAN_SWAP16(right);
            bottom = BIG_ENDIAN_SWAP16(bottom);
        }
    };

    enum eUMC_Status
    {
        UMC_OK                  = VM_OK,                //0,     // no error
        UMC_OPERATION_FAILED    = VM_OPERATION_FAILED,  //-999,
        UMC_NOT_INITIALIZED     = VM_NOT_INITIALIZED,   //-998,
        UMC_TIMEOUT             = VM_TIMEOUT,           //-987,
        UMC_NOT_ENOUGH_DATA     = VM_NOT_ENOUGH_DATA,   //-996,   // not enough input data

        UMC_NULL_PTR            = VM_NULL_PTR,    // null pointer in input parameters
        UMC_SO_CANT_LOAD        = VM_SO_CANT_LOAD,
        UMC_SO_INVALID_HANDLE   = VM_SO_INVALID_HANDLE,
        UMC_SO_CANT_GET_ADDR    = VM_SO_CANT_GET_ADDR,

        UMC_FAILED_TO_INITIALIZE =-899,   // failed to initialize codec
        UMC_NOT_FIND_SYNCWORD   =-897,   // can't find sync word in buffer
        UMC_NOT_ENOUGH_BUFFER   =-896,   // not enough buffer to put output data
        UMC_END_OF_STREAM       =-895,
        UMC_FAILED_TO_OPEN_DEVICE =-894,
        UMC_FAILED_TO_ALLOCATE_BUFFER =-883,
        UMC_BAD_FORMAT          =-882,
        UMC_ALLOC               =-881,
        UMC_BAD_STREAM          =-880,
        UMC_UNSUPPORTED         =-879,
        UMC_NOT_IMPLEMENTED     =-878,
        UMC_STOP                =-877,
        UMC_FLAGS_ERROR         =-876,

        UMC_WRN_NOT_CRITICAL_BAD_FORMAT = 1,
        UMC_WAIT_FOR_REPOSITION = 2
   };

   typedef int Status;

    const vm_char* GetErrString(Status ErrCode);
    const vm_char* GetStreamTypeString(SystemStreamType Code);
    const vm_char* GetFormatTypeString(ColorFormat Code);
    const vm_char* GetAudioTypeString(AudioStreamType Code);
    const vm_char* GetVideoTypeString(VideoStreamType Code);
    const vm_char* GetVideoRenderTypeString(VideoRenderType Code);
    const vm_char* GetAudioRenderTypeString(AudioRenderType Code);

    enum
    {
        DEFAULT_ALIGN_VALUE     = 16
    };

    // forward declaration of template
    template<class T> inline
    T align_pointer(void *pv, size_t lAlignValue = DEFAULT_ALIGN_VALUE)
    {
        // some compilers complain to conversion to/from
        // pointer types from/to integral types.
        return (T) ((((size_t) (pv)) + (lAlignValue - 1)) &
                    ~(lAlignValue - 1));
    }

    // forward declaration of template
    template<class T> inline
    T align_value(size_t nValue, size_t lAlignValue = DEFAULT_ALIGN_VALUE)
    {
        return static_cast<T> ((nValue + (lAlignValue - 1)) &
                               ~(lAlignValue - 1));
    }

} // namespace UMC

#endif /* __UMC_STRUCTURES_H__ */
