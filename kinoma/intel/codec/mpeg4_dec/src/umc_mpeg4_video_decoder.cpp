/* ///////////////////////////////////////////////////////////////////////////// */
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

//***
#include "kinoma_ipp_lib.h"

#include <string.h>
#include "vm_debug.h"
#include "umc_mpeg4_video_decoder.h"
#include "umc_video_data.h"
#include "mp4.h"

namespace UMC
{
//***
Status MPEG4VideoDecoder::GetDimensions(int *width, int *height)
{
	*width = m_decInfo->VisualObject.VideoObject.width;
	*height = m_decInfo->VisualObject.VideoObject.height;

	return UMC_OK;
}

//***
Status MPEG4VideoDecoder::GetYUV(YUVPlannar *yuv)
{
     mp4_Frame *thisFrame = m_decInfo->VisualObject.vFrame;  // frame for display
	//H264DecoderFrame *thisFrame = m_pDisplayFrame; //***m_pCurrentFrame
	//H264DecoderFrame *thisFrame = m_pCurrentFrame;
	
	if( thisFrame == NULL )
	{
		yuv->isValid = false;
	}
	else
	{
		yuv->isValid 		= true;
		yuv->pixelFormat	= 9;
		yuv->rowBytesY		= thisFrame->stepY;
		yuv->rowBytesCb	    = thisFrame->stepCb;
		yuv->rowBytesCr	    = thisFrame->stepCr;
		yuv->y				= thisFrame->pY;;
		yuv->cb				= thisFrame->pCb;
		yuv->cr				= thisFrame->pCr;
	
		yuv->width16		= m_decInfo->VisualObject.VideoObject.width;
		yuv->height16		= m_decInfo->VisualObject.VideoObject.height;
		yuv->left			= 0;
		yuv->top			= 0;
		yuv->right			= yuv->width16;
		yuv->bottom			= yuv->height16;
	}

	return UMC_OK;
}

Status MPEG4VideoDecoder::GetQp(int *qp)
{
	if( m_decInfo->VisualObject.VideoObject.short_video_header )
		*qp = m_decInfo->VisualObject.VideoObject.VideoObjectPlaneH263.vop_quant;
#ifdef SUPPORT_H263_ONLY 
	else return MP4_STATUS_FILE_ERROR;
#else
	else
		*qp = m_decInfo->VisualObject.VideoObject.VideoObjectPlane.quant;
#endif

	return UMC_OK;
}


Status MPEG4VideoDecoder::Init(BaseCodecParams_V51 *lpInit)
{
    Status  status = UMC_OK;
    VideoDecoderParams_V51 *pParam = DynamicCast<VideoDecoderParams_V51> (lpInit);

    if (NULL == m_decInfo)
        return UMC_ALLOC;
    if (NULL == pParam)
        return UMC_NULL_PTR;
    m_time_reset = 0;
    m_dec_time_fr = 0.0;
    m_dec_time_base = m_dec_time_prev = -1.0;
    m_dec_time_frinc = (pParam->info.framerate > 0) ? 1.0 / pParam->info.framerate : 0.0;
    m_is_skipped_b = m_skipped_fr = m_b_prev = 0;
    m_Param = *pParam;
    m_IsInit = false;
    memset(m_decInfo, 0, sizeof(mp4_Info));
	//***kinoma optimization
	mp4_SetDefaultIDCTProcs(m_decInfo);
    m_decInfo->VisualObject.verid = 1;
    if (NULL != pParam->m_pData0) {
        if (pParam->m_pData0->GetTime() >= 0.0)
            m_dec_time_base = pParam->m_pData0->GetTime();
        m_decInfo->buflen = pParam->m_pData0->GetDataSize();
        m_decInfo->bufptr = m_decInfo->buffer = reinterpret_cast<unsigned char *> (pParam->m_pData0->GetDataPointer());
        m_decInfo->bitoff = 0;
        status = InsideInit(&m_Param);
        if (UMC_OK == status) {
            if ((m_Param.lFlags & FLAG_VDEC_REORDER) && ((m_decInfo->buflen - ((size_t)m_decInfo->bufptr - (size_t)m_decInfo->buffer)) > 4))
                status = GetFrame(pParam->m_pData0, NULL);
            if (UMC_NOT_ENOUGH_DATA == status || UMC_NOT_FIND_SYNCWORD == status)
                status = UMC_OK;
        }
        pParam->m_pData0->MoveDataPointer(static_cast<vm_var32>((size_t)m_decInfo->bufptr - (size_t)m_decInfo->buffer));
    }
    if (UMC_OK == status) {
#ifndef DROP_COLOR_CONVERSION
        m_pConverter = pParam->lpConverter;
#endif
        // set VOP size if it is unknown in splitter
        if (pParam->info.clip_info.width == 0 || pParam->info.clip_info.height == 0) {
            if (m_decInfo->VisualObject.VideoObject.width == 0 && m_decInfo->VisualObject.VideoObject.height== 0)
                return UMC_BAD_STREAM;
            else {
                pParam->info.clip_info.width = m_Param.info.clip_info.width = m_decInfo->VisualObject.VideoObject.width;
                pParam->info.clip_info.height = m_Param.info.clip_info.height = m_decInfo->VisualObject.VideoObject.height;
            }
        }
#ifndef DROP_COLOR_CONVERSION
        if (pParam->lpConvertInit) {
            m_Convert.ConversionInit = *(pParam->lpConvertInit);
            m_Convert.ConversionInit.FormatSource = YV12;
            m_Convert.ConversionInit.FormatDest = (*pParam).cformat;
            m_Convert.ConversionInit.SizeSource.width =  pParam->info.clip_info.width;
            m_Convert.ConversionInit.SizeSource.height = pParam->info.clip_info.height;
            if (NULL != m_pConverter && (status = m_pConverter->Init(m_Convert.ConversionInit)) != UMC_OK)
                return status;
		}
#endif
    }
    return status;
}


Status MPEG4VideoDecoder::InsideInit(VideoDecoderParams_V51 *pParam)
{
    Ipp32u  code, h_vo_found = 0, h_vos_found = 0;

    Close();
    m_buffered_frame = m_Param.lFlags & FLAG_VDEC_REORDER;
    if (pParam->info.stream_subtype == MPEG4_VIDEO_DIVX5) {
        m_decInfo->ftype = 2;
        m_decInfo->ftype_f = 1;
    } else if (pParam->info.stream_subtype == MPEG4_VIDEO_QTIME) {
        m_decInfo->ftype = 1;
        m_decInfo->ftype_f = 0;
    } else { // UNDEF_VIDEO_SUBTYPE
        m_decInfo->ftype = 0;
        m_decInfo->ftype_f = 0;
    }
    for (;;) {
        if (!mp4_SeekStartCodeOrShortPtr(m_decInfo)) {
            mp4_Error("Error: Can't find Visual Object or Video Object start codes or short_video_start_marker\n");
            return UMC_NOT_FIND_SYNCWORD;
        }
        // check short_video_start_marker
        //***
        if ( ((mp4_ShowBits(m_decInfo, 24) & (0xFFFFFC)) == 0x80) ||
			 ((mp4_ShowBits(m_decInfo, 24) & (0xFFFFFC)) == 0x84) 
		   )
		
		{
            code = MP4_VIDEO_OBJECT_MIN_SC;
        } 
#ifdef SUPPORT_H263_ONLY_AND_NO_ESDS_PARSING		//***bnie: so that we can parse esds for other mp4v decoders
		else return MP4_STATUS_FILE_ERROR;
#else		
		else {
            code = mp4_GetBits(m_decInfo, 8);
            if (!h_vos_found && code == MP4_VISUAL_OBJECT_SEQUENCE_SC) {
                h_vos_found = 1;
            }
            if (!h_vo_found && code == MP4_VISUAL_OBJECT_SC) {
                h_vo_found = 1;
                if ((mp4_Parse_VisualObject(m_decInfo)) != MP4_STATUS_OK)
                    return UMC_BAD_STREAM;
            }
        }
#endif

        if ((int)code >= MP4_VIDEO_OBJECT_MIN_SC && code <= MP4_VIDEO_OBJECT_MAX_SC) {
            if ((mp4_Parse_VideoObject(m_decInfo)) != MP4_STATUS_OK)
			{
				continue;
				//return UMC_BAD_STREAM;
			}
            break;
        }
        // some streams can start with video_object_layer
        if ((int)code >= MP4_VIDEO_OBJECT_LAYER_MIN_SC && code <= MP4_VIDEO_OBJECT_LAYER_MAX_SC) {
            m_decInfo->bufptr -= 4;
            if ((mp4_Parse_VideoObject(m_decInfo)) != MP4_STATUS_OK)
                return UMC_BAD_STREAM;
            break;
        }
    }

    if (mp4_InitVOL(m_decInfo) != MP4_STATUS_OK) {
        mp4_Error("Error: No memory to allocate internal buffers\n");
        return UMC_ALLOC;
    }
    // set aspect ratio info
    switch (m_decInfo->VisualObject.VideoObject.aspect_ratio_info) {
        case MP4_ASPECT_RATIO_FORBIDDEN:
        case MP4_ASPECT_RATIO_1_1:
            pParam->info.aspect_ratio_width  = 1;
            pParam->info.aspect_ratio_height = 1;
            break;
        case MP4_ASPECT_RATIO_12_11:
            pParam->info.aspect_ratio_width  = 12;
            pParam->info.aspect_ratio_height = 11;
            break;
        case MP4_ASPECT_RATIO_10_11:
            pParam->info.aspect_ratio_width  = 10;
            pParam->info.aspect_ratio_height = 11;
            break;
        case MP4_ASPECT_RATIO_16_11:
            pParam->info.aspect_ratio_width  = 16;
            pParam->info.aspect_ratio_height = 11;
            break;
        case MP4_ASPECT_RATIO_40_33:
            pParam->info.aspect_ratio_width  = 40;
            pParam->info.aspect_ratio_height = 33;
            break;
        default:
            pParam->info.aspect_ratio_width  = m_decInfo->VisualObject.VideoObject.aspect_ratio_info_par_width;
            pParam->info.aspect_ratio_height = m_decInfo->VisualObject.VideoObject.aspect_ratio_info_par_height;
    }
    m_IsInit = true;
    return UMC_OK;
}


Status MPEG4VideoDecoder::GetFrame(MediaData_V51* in, MediaData_V51* out)
{
    Status  status = UMC_OK;
    double  pts = -1.0;

    if (in) {
        m_decInfo->bitoff = 0;
        m_decInfo->bufptr = m_decInfo->buffer = (Ipp8u *)in->GetDataPointer();
        m_decInfo->buflen = in->GetDataSize();
    }
    if (!m_IsInit) {
        if (in == NULL)
            return UMC_NULL_PTR;
        m_Param.m_pData0 = in;
        status = InsideInit(&m_Param);
        if (status != UMC_OK) {
            in->MoveDataPointer(in->GetDataSize() - m_decInfo->buflen);
            return UMC_FAILED_TO_INITIALIZE;
        }
    }
    do{
        if (in == NULL) {
            // show last frame (it can be only if (m_Param.lFlags & FLAG_VDEC_REORDER))
            if (!m_buffered_frame)
                return UMC_END_OF_STREAM;
            m_buffered_frame = false;
            m_decInfo->VisualObject.vFrame = m_decInfo->VisualObject.VideoObject.prevPlaneIsB ?
                                            &m_decInfo->VisualObject.nFrame : &m_decInfo->VisualObject.cFrame;
        } else {
            // Seeking the VOP start_code, and then begin the vop decoding
            if (m_decInfo->VisualObject.VideoObject.short_video_header) {
                if (!mp4_SeekShortVideoStartMarker(m_decInfo)) {
                    mp4_Error("Error: Failed seeking short_video_start_marker\n");
                    status = UMC_NOT_FIND_SYNCWORD;
                    break;
                }
            } 
#ifdef SUPPORT_H263_ONLY 
			else return MP4_STATUS_FILE_ERROR;
#else       
            else {
                if (!mp4_SeekStartCodeValue(m_decInfo, MP4_VIDEO_OBJECT_PLANE_SC)) {
                    mp4_Error("Error: Failed seeking VOP Start Code\n");
                    status = UMC_NOT_FIND_SYNCWORD;
                    break;
                }
            }
#endif
            
            // parse VOP header
            if ((mp4_Parse_VideoObjectPlane(m_decInfo)) != MP4_STATUS_OK) {
                status = UMC_BAD_STREAM;
                break;
            }
            if (m_IsReset && m_decInfo->VisualObject.VideoObject.VideoObjectPlane.coding_type != MP4_VOP_TYPE_I) {
                status = UMC_NOT_ENOUGH_DATA;
                break;
            }
            if (m_decInfo->VisualObject.VideoObject.VideoObjectPlane.coding_type == MP4_VOP_TYPE_B) {
                if (0 < m_is_skipped_b && !m_b_prev) {
                    m_is_skipped_b --;
                    m_skipped_fr ++;
                    m_b_prev = 1;
                    m_decInfo->bufptr = m_decInfo->buffer+ m_decInfo->buflen;
                    status = UMC_NOT_ENOUGH_DATA;
                    break;
                } else
                    m_b_prev = 0;
            }
            // decode VOP
            if ((mp4_DecodeVideoObjectPlane(m_decInfo)) != MP4_STATUS_OK) {
                status = UMC_BAD_STREAM;
            }
            if (m_decInfo->VisualObject.VideoObject.VideoObjectPlane.coded ||
                (m_decInfo->VisualObject.rFrame.time != m_decInfo->VisualObject.cFrame.time &&
                 m_decInfo->VisualObject.nFrame.time != m_decInfo->VisualObject.cFrame.time)) {
                m_decInfo->VisualObject.VideoObject.VOPindex ++;
            } else {
                status = UMC_NOT_ENOUGH_DATA;
                break;
            }
            if (m_IsReset && m_decInfo->VisualObject.VideoObject.VideoObjectPlane.coding_type == MP4_VOP_TYPE_I) {
                m_time_reset = (int)m_decInfo->VisualObject.cFrame.time;
                m_decInfo->VisualObject.vFrame = NULL;
                m_IsReset = false;
            }
            if ((m_Param.lFlags & FLAG_VDEC_REORDER) && (m_decInfo->VisualObject.vFrame == NULL)) {
                // buffer first frame in VDEC_REORDER mode
                status = UMC_NOT_ENOUGH_DATA;
                break;
            }
            if (!(m_Param.lFlags & FLAG_VDEC_REORDER))
                m_decInfo->VisualObject.vFrame = &m_decInfo->VisualObject.cFrame;
        }
        if (out) {
            VideoData_V51* pOutVideoData = DynamicCast<VideoData_V51>(out);
            FrameType ft;

#ifndef DROP_SPRITE
            if (m_decInfo->VisualObject.VideoObject.sprite_enable == MP4_SPRITE_STATIC)
                ft = I_PICTURE;
            else
#endif
                ft = (m_decInfo->VisualObject.vFrame->type) == MP4_VOP_TYPE_I ? I_PICTURE :
                     (m_decInfo->VisualObject.vFrame->type) == MP4_VOP_TYPE_B ? B_PICTURE :
                                                                                P_PICTURE;
            pOutVideoData->SetFrameType(ft);
            pOutVideoData->SetVideoParameters(m_decInfo->VisualObject.VideoObject.width,
                                              m_decInfo->VisualObject.VideoObject.height,
#ifndef DROP_COLOR_CONVERSION
											  m_Convert.ConversionInit.FormatDest
#else
											  NONE
#endif
											  );
#ifndef DROP_COLOR_CONVERSION            
			// set source pointer(s)
            m_Convert.lpSource0 = m_decInfo->VisualObject.vFrame->pY;
            m_Convert.lpSource2 =  m_decInfo->VisualObject.vFrame->pCb;
            m_Convert.lpSource1 =  m_decInfo->VisualObject.vFrame->pCr;
            m_Convert.PitchSource0 = m_decInfo->VisualObject.vFrame->stepY;
            m_Convert.PitchSource2 = m_decInfo->VisualObject.vFrame->stepCb;
            m_Convert.PitchSource1 = m_decInfo->VisualObject.vFrame->stepCr;
            m_Convert.lpDest0 = (Ipp8u*)pOutVideoData->m_lpDest[0];
            m_Convert.lpDest1 = (Ipp8u*)pOutVideoData->m_lpDest[1];
            m_Convert.lpDest2 = (Ipp8u*)pOutVideoData->m_lpDest[2];
            m_Convert.PitchDest0 = pOutVideoData->m_lPitch[0];
            m_Convert.PitchDest1 = pOutVideoData->m_lPitch[1];
            m_Convert.PitchDest2 = pOutVideoData->m_lPitch[2];
            if (NULL != m_pConverter) {
                status = m_pConverter->ConvertFrame(&m_Convert);
            } else {
                pOutVideoData->SetDest(m_Convert.lpSource0, m_Convert.lpSource1, m_Convert.lpSource2);
                pOutVideoData->SetPitch(m_Convert.PitchSource0, m_Convert.PitchSource1, m_Convert.PitchSource2);
            }
#else
            pOutVideoData->SetDest(m_decInfo->VisualObject.vFrame->pY, m_decInfo->VisualObject.vFrame->pCr, m_decInfo->VisualObject.vFrame->pCb);
            pOutVideoData->SetPitch(m_decInfo->VisualObject.vFrame->stepY, m_decInfo->VisualObject.vFrame->stepCr, m_decInfo->VisualObject.vFrame->stepCb);
#endif
        }
    }while(0);
    if (in)
        if (in->GetTime() != -1.0 && m_dec_time_base == -1.0)
            m_dec_time_base = in->GetTime();
    if ((m_dec_time_frinc > 0.0) || (m_decInfo->ftype == 1)) {
        if (m_Param.lFlags & FLAG_VDEC_REORDER)
            pts = m_dec_time_prev;
        if (in) {
            if (in->GetTime() != -1.0) {
                // take right PTS for I-, P- frames
                m_dec_time_prev = in->GetTime();
            } else {
                // when PB...  are in one AVI chunk, first PTS from in->GetTime() is right and second is -1.0
                m_dec_time_prev += m_dec_time_frinc;
            }
        }
        if (!(m_Param.lFlags & FLAG_VDEC_REORDER))
            pts = m_dec_time_prev;
    } else {
        bool  extPTS = false;
        if (in)
            if (in->GetTime() != -1.0)
                extPTS = true;
        if (extPTS) {
            pts = in->GetTime();
        } else {
            // for other internal MPEG-4 PTS is used
            if (m_decInfo->VisualObject.vFrame)
                pts = (double)(m_decInfo->VisualObject.vFrame->time - m_time_reset) / m_decInfo->VisualObject.VideoObject.vop_time_increment_resolution;
            else {
                pts = 0.0;
            }
            if (m_dec_time_base != -1.0)
                pts += m_dec_time_base;
        }
    }
    if (out)
        out->SetTime(pts);
    if (in) {
        size_t stDecidedData;
        if ((size_t)m_decInfo->bufptr - (size_t)m_decInfo->buffer < m_decInfo->buflen)
            stDecidedData = m_decInfo->buflen - ((size_t)m_decInfo->bufptr - (size_t)m_decInfo->buffer);
        else
            stDecidedData = 0;
        in->MoveDataPointer(in->GetDataSize() - static_cast<vm_var32>(stDecidedData));
        // can't calculate time for the next frame
        in->SetTime(-1.0);
    }
    // set interlaced info
    if (!m_decInfo->VisualObject.VideoObject.interlaced)
        m_Param.info.interlace_type = PROGRESSIVE;
    else
        m_Param.info.interlace_type = (m_decInfo->VisualObject.VideoObject.VideoObjectPlane.top_field_first) ? INTERLEAVED_TOP_FIELD_FIRST : INTERLEAVED_BOTTOM_FIELD_FIRST;
    return status;
}


Status MPEG4VideoDecoder::GetInfo(BaseCodecParams_V51 *lpInfo)
{
    Status umcRes = UMC_OK;
    VideoDecoderParams_V51 *lpParams = DynamicCast<VideoDecoderParams_V51> (lpInfo);

//    if (!m_IsInit)
//        return UMC_NOT_INITIALIZED;
    if (NULL == lpParams)
        return UMC_NULL_PTR;
    umcRes = VideoDecoder_V51::GetInfo(lpInfo);
    if (UMC_OK == umcRes) {
        lpParams->info = m_Param.info;
#ifndef DROP_COLOR_CONVERSION
        if (NULL != m_pConverter) {
            lpParams->info.clip_info.width = m_Convert.ConversionInit.SizeDest.width;
            lpParams->info.clip_info.height = m_Convert.ConversionInit.SizeDest.height;
        }
#endif
    }
    return umcRes;
}


Status MPEG4VideoDecoder::Close(void)
{
    if (m_IsInit) {
        mp4_FreeVOL(m_decInfo);
        m_IsInit = false;
        return UMC_OK;
    } else
        return UMC_NOT_INITIALIZED;
}


MPEG4VideoDecoder::MPEG4VideoDecoder(void)
{
    m_IsInit = false;
    m_decInfo = new mp4_Info;
    m_IsReset = false;
	m_dec_time = m_dec_time_prev = m_dec_time_base = m_dec_time_fr = m_dec_time_frinc = 0.0;
	m_buffered_frame = m_is_skipped_b = m_skipped_fr = m_b_prev = 0;
	m_time_reset = 0;
}

MPEG4VideoDecoder::~MPEG4VideoDecoder(void)
{
    Close();
    delete m_decInfo;
}

//***kinoma optimization
void MPEG4VideoDecoder::SetApprox(int level)
{
	mp4_SetApprox(m_decInfo, level);
}

Status  MPEG4VideoDecoder::ResetSkipCount()
{
    m_is_skipped_b = m_skipped_fr = m_b_prev = 0;
    return UMC_OK;
}


Status  MPEG4VideoDecoder::SkipVideoFrame(int count)
{
    if (!m_IsInit)
        return UMC_NOT_INITIALIZED;
    if (count < 0) {
        m_is_skipped_b = 0;
        return UMC_OK;
    }
    m_is_skipped_b += count;
    return UMC_OK;
}


vm_var32 MPEG4VideoDecoder::GetSkipedFrame(void)
{
    return m_skipped_fr;
}


Status MPEG4VideoDecoder::Reset(void)
{
    if (!m_IsInit)
        return UMC_NOT_INITIALIZED;
    else {
        m_IsReset = true;
        m_decInfo->VisualObject.VideoObject.vop_sync_time = 0;
        m_decInfo->VisualObject.VideoObject.vop_sync_time_b = 0;
        m_decInfo->VisualObject.VideoObject.GroupOfVideoObjectPlane.time_code = 0;
        m_dec_time_base = m_dec_time_prev = -1.0;
        return UMC_OK;
    }
}


Status MPEG4VideoDecoder::GetPerformance(double *perf)
{
    return UMC_NOT_IMPLEMENTED;
}


mp4_Frame* MPEG4VideoDecoder::GetCurrentFramePtr(void)
{
  return m_decInfo->VisualObject.vFrame;
}

} // end namespace UMC
