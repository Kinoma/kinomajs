/* ////////////////////////////////////////////////////////////////////////// */
/*
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//        Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//
*/
#ifndef __UMC_MPEG4_VIDEO_DECODER_H
#define __UMC_MPEG4_VIDEO_DECODER_H

#include "ipps.h"
#include "ippvc.h"
#include "umc_structures.h"
#include "umc_video_decoder.h"
#include "umc_base_color_space_converter.h"
#include "mp4.h"

namespace UMC
{

typedef struct
{
	long			isValid;
	long			pixelFormat;
	long			width16;
	long			height16;
	long			left;
	long			top;
	long			right;
	long			bottom;
	long			rowBytesY;
	long			rowBytesCb;
	long			rowBytesCr;
	unsigned char  *y;
	unsigned char  *cb;
	unsigned char  *cr;
} YUVPlannar;

class MPEG4VideoDecoder : public VideoDecoder_V51
{
    DYNAMIC_CAST_DECL(MPEG4VideoDecoder, VideoDecoder_V51)

public:
    // Default constructor
    MPEG4VideoDecoder(void);
    // Destructor
    virtual ~MPEG4VideoDecoder(void);
    // Initialize for subsequent frame decoding.
    virtual Status Init(BaseCodecParams_V51 *init);
    // Get next frame
    virtual Status GetFrame(MediaData_V51* in, MediaData_V51* out);
    // Close decoding & free all allocated resources
    virtual Status Close(void);
    // Reset decoder to initial state
    virtual Status  Reset(void);
    // Get information, valid after initialization
    virtual Status GetInfo(BaseCodecParams_V51* ininfo);
    // Get performance
    Status GetPerformance(double *perf);
    // reset skip frame counter
    Status  ResetSkipCount();
    // increment skip frame counter
    Status  SkipVideoFrame(int);
    // get skip frame counter statistic
    vm_var32 GetSkipedFrame();
    // get pointers to internal current frame
    mp4_Frame* GetCurrentFramePtr(void);
	
	//***
	Status GetYUV(YUVPlannar *yuv);
	Status GetDimensions(int *width, int *height);
	Status GetQp(int *qp);

	//***kinoma optimization
	void SetApprox(int level);

protected:
    bool                    m_IsInit, m_IsReset;
    double                  m_dec_time, m_dec_time_prev, m_dec_time_base, m_dec_time_fr, m_dec_time_frinc;
    int                     m_buffered_frame, m_is_skipped_b, m_skipped_fr, m_b_prev;
    int                     m_time_reset;
    VideoDecoderParams_V51  m_Param;
#ifndef DROP_COLOR_CONVERSION
    ColorConversionParams   m_Convert;
#endif
    mp4_Info               *m_decInfo;
    Status                  InsideInit(VideoDecoderParams_V51 *pParam);
};

} // end namespace UMC

#endif //__UMC_MPEG4_VIDEO_DECODER_H
