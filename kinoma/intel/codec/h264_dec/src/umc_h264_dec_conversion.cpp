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

#include "umc_h264_dec.h"
#include "umc_h264_dec_conversion.h"
#include "umc_video_data.h"
#include "vm_thread.h"
#include "vm_event.h"

namespace UMC
{

#ifdef STORE_DISPLAY_PICS
FILE *pics_disp;
int num_display_pics;
#endif // STORE_DISPLAY_PICS

#ifndef DROP_COLOR_CONVERSION
void H264VideoDecoder::InitializeConversionThreading(void)
{
    vm_status res;
    bool bErr = false;

    // set quit flag
    m_bQuit = false;

    // create objects
    res = vm_event_init(&m_hStartConversion, 0, 0);
    if (VM_OK != res)
        bErr = true;

    res = vm_event_init(&m_hStopConversion, 0, 0);
    if (VM_OK != res)
        bErr = true;

    {
        int iThreadCreateRes;

        iThreadCreateRes = vm_thread_create(&m_hConversionThread,
                                            ConvertFrameAsyncSecondThread,
                                            this);
        if (0 == iThreadCreateRes)
            bErr = true;
    }

    // deinitialize when error occurs. Its not critical.
    if (bErr)
        ReleaseConversionThreading();

} // void H264VideoDecoder::InitializeConversionThreading(void)

void H264VideoDecoder::ReleaseConversionThreading(void)
{
    if (vm_thread_is_valid(&m_hConversionThread))
    {
        m_bQuit = true;
        vm_event_signal(&m_hStartConversion);

        vm_thread_wait(&m_hConversionThread);
        vm_thread_close(&m_hConversionThread);
    }

    // close handles
    if (vm_event_is_valid(&m_hStartConversion))
        vm_event_destroy(&m_hStartConversion);
    if (vm_event_is_valid(&m_hStopConversion))
        vm_event_destroy(&m_hStopConversion);

    // reset handles
    vm_thread_set_invalid(&m_hConversionThread);
    vm_event_set_invalid(&m_hStartConversion);
    vm_event_set_invalid(&m_hStopConversion);

    m_bQuit = false;

} // void H264VideoDecoder::ReleaseConversionThreading(void)
static ColorFormat color_formats[4]={
    YUV420,YUV420,YUV422,YUV444
};
void H264VideoDecoder::InitColorConverter(H264DecoderFrame *source, Ipp8u force_field)
{
    // set correct size in color converter
    Ipp32s crop_left   = source->m_crop_left;
    Ipp32s crop_right  = source->m_crop_right;
    Ipp32s crop_top    = source->m_crop_top;
    Ipp32s crop_bottom = source->m_crop_bottom;
    Ipp32s width = source->lumaSize().width;
    Ipp32s height= source->lumaSize().height;
    width-=2*(crop_left+crop_right);//-+crop_flag;
    height-=2*(crop_top+crop_bottom);//+crop_flag;

    if ((0 == m_Convert.ConversionInit.SizeSource.width) ||
        ((Ipp32u) (m_Convert.ConversionInit.SizeSource.width) != source->lumaSize().width))
    {
        m_Convert.ConversionInit.SizeSource.width = source->lumaSize().width;
    }

    if ((0 == m_Convert.ConversionInit.SizeSource.height) ||
        ((Ipp32u) (m_Convert.ConversionInit.SizeSource.height) != (source->lumaSize().height >> force_field)))
    {
        m_Convert.ConversionInit.SizeSource.height = source->lumaSize().height>>force_field;
    }

    m_Convert.ConversionInit.SrcCropRect.left = (short) (2 * crop_left);
    m_Convert.ConversionInit.SrcCropRect.top = (short) (2 * crop_top);
    m_Convert.ConversionInit.SrcCropRect.right = (short) (2*crop_left+width);
    m_Convert.ConversionInit.SrcCropRect.bottom = (short) ((2*crop_top+height) >> force_field);
    m_Convert.ConversionInit.FormatSource = color_formats[source->m_chroma_format];

    if(ulResized)
    {
        m_Convert.ConversionInit.SizeDest.width  = width / ulResized;
        m_Convert.ConversionInit.SizeDest.height = height / ulResized;

        m_Convert.ConversionInit.SrcCropRect.left = (short) (m_Convert.ConversionInit.SrcCropRect.left / ulResized);
        m_Convert.ConversionInit.SrcCropRect.top = (short) (m_Convert.ConversionInit.SrcCropRect.top / ulResized);
        m_Convert.ConversionInit.SrcCropRect.right = (short) (m_Convert.ConversionInit.SrcCropRect.right / ulResized);
        m_Convert.ConversionInit.SrcCropRect.bottom = (short) (m_Convert.ConversionInit.SrcCropRect.bottom / ulResized);
    }

    if (NULL != m_pConverter)
        m_pConverter->Init(m_Convert.ConversionInit);

    if (m_bTwoPictures)
    {
        m_ConvertPreview.ConversionInit.SizeSource.width = source->lumaSize().width;
        m_ConvertPreview.ConversionInit.SizeSource.height = source->lumaSize().height>>force_field;
        if (NULL != m_pConverter)
            m_pConverter->Init(m_ConvertPreview.ConversionInit);
    }

} // void H264VideoDecoder::InitColorConverter(H264DecoderFrame *source, Ipp8u force_field)
#endif

H264VideoDecoder::H264DecoderFrame *H264VideoDecoder::GetFrameToDisplay(Ipp32u nStage)
{
    if (BEFORE_DECODING == nStage)
    {
        H264DecoderFrame *pTmp;

        // there are no frames to show
        if ((1 >= m_dpbSize) ||
            (m_H264DecoderFramesList.countNumDisplayable() < (m_dpbSize - 1)))
            return NULL;

        // sometimes current frame may be shown after decoding
        // pTmp = m_H264DecoderFramesList.findOldestDisplayable();

        {
            pTmp = NULL;
        }

        return pTmp;
    }
    else
    {
        // show oldest frame
        if ((m_H264DecoderFramesList.countNumDisplayable() >= m_dpbSize) ||
            (ABSENT_DECODING == nStage))
            return m_H264DecoderFramesList.findOldestDisplayable();
        // there are no frames to show
        else
            return NULL;
    }

} // H264DecoderFrame *GetFrameToDisplay(bool bAfterDecodingCall)

Status H264VideoDecoder::OutputFrame(Ipp32u nStage)
{
    Status ps = UMC_OK;

    // we suppose to m_pDisplayFrame will be the same
    // before and after current frame decoding

    switch (nStage)
    {
    case PREPARE_DECODING:
        m_pDisplayFrame = NULL;
        break;

    case BEFORE_DECODING:
        m_pDisplayFrame = GetFrameToDisplay(nStage);
        break;

    case AFTER_DECODING:
    case ABSENT_DECODING:
        {
            if (m_pDisplayFrame)
            {
                H264DecoderFrame *pTmp = GetFrameToDisplay(nStage);

                VM_ASSERT((m_pDisplayFrame == pTmp) ||
                          (NULL == m_pDisplayFrame) ||
                          (NULL == pTmp));

                //***if (dst)
                {
                    // sometimes we have converted unnecessary frame
                    m_pDisplayFrame = pTmp;
                }
            }
            else
            {
                m_pDisplayFrame = GetFrameToDisplay(nStage);
            }

            if (m_pDisplayFrame )//***&& dst)
            {
                m_pDisplayFrame->setWasOutputted();
            }
            else
                ps = UMC_END_OF_STREAM;
        }
        break;

    default :
        break;
    }

    return ps;

} // Status H264VideoDecoder::OutputFrame(MediaData *dst, Ipp32u nStage)

#ifndef DROP_COLOR_CONVERSION
Status H264VideoDecoder::ConvertFrameAsync(MediaData *dst, Ipp32u nStage)
{
    Status res = UMC_OK;
    VideoData *pVData = DynamicCast<VideoData> (dst);

    if ((m_pDisplayFrame) &&
        (pVData))
    {
        switch (nStage)
        {
        case BEFORE_DECODING:
            {
                H264DecYUVWorkSpace *pDisplayBuffer;
                pDisplayBuffer = m_pDisplayFrame;

                // Perform output color conversion and video effects, if we didn't
                // already write our output to the application's buffer.

                if (NULL != m_pConverter)
                    InitColorConverter(m_pDisplayFrame, 0);

                m_Convert.lpDest0 = (Ipp8u *) pVData->m_lpDest[0];
                m_Convert.PitchDest0 = pVData->m_lPitch[0];
                m_Convert.lpDest1 = NULL;
                m_Convert.PitchDest1 = 0;
                m_Convert.lpDest2 = NULL;
                m_Convert.PitchDest2 = 0;
                m_Convert.lpSource0 = pDisplayBuffer->m_pYPlane;
                m_Convert.lpSource2 = pDisplayBuffer->m_pUPlane;
                m_Convert.lpSource1 = pDisplayBuffer->m_pVPlane;
                m_Convert.PitchSource0 = pDisplayBuffer->pitch();
                m_Convert.PitchSource1 = pDisplayBuffer->pitch();
                m_Convert.PitchSource2 = pDisplayBuffer->pitch();

                vm_event_signal(&m_hStartConversion);
            }
            break;

        case AFTER_DECODING:
        case ABSENT_DECODING:
        case ERROR_DECODING:
            // wait conversion thread
            vm_event_wait(&m_hStopConversion);
            break;

        case PREPARE_DECODING:
        default:
            break;
        }
    }

    return res;

} // Status H264VideoDecoder::ConvertFrameAsync(MediaData *dst, Ipp32u nStage)

unsigned int H264VideoDecoder::ConvertFrameAsyncSecondThread(void *p)
{
    H264VideoDecoder *pObj = (H264VideoDecoder *) p;

    if (pObj)
    {
        // wait for start of conversion
        vm_event_wait(&pObj->m_hStartConversion);

        while (false == pObj->m_bQuit)
        {
            // do conversion
            if (NULL != pObj->m_pConverter)
            {
                pObj->m_pConverter->ConvertFrame(&pObj->m_Convert);
#ifdef STORE_OUTPUT_YUV
                if (yuv_file==NULL)
                {
                    yuv_file=fopen(__YUV_FILE__,"w+b");
                }
                if (yuv_file)
                {
                    Ipp32s pitch = pObj->m_Convert.PitchSource0;
                    Ipp32s width = pObj->m_Convert.ConversionInit.SizeSource.width;
                    Ipp32s height = pObj->m_Convert.ConversionInit.SizeSource.height;
                    Ipp8u *y  = pObj->m_Convert.lpSource0;
                    Ipp8u *u  = pObj->m_Convert.lpSource2;
                    Ipp8u *v  = pObj->m_Convert.lpSource1;
                    int i;
                    for (i=0;i<height;i++,y+=pitch)
                    {
                        fwrite(y,width,1,yuv_file);
                    }
                    switch(pObj->m_Convert.ConversionInit.FormatSource)
                    {
                    case YUV420:
                        width/=2;
                        height/=2;
                        break;
                    case YUV422:
                        width/=2;
                        break;
                    case YUV444:
                        break;
                    }
                    for (i=0;i<height;i++,u+=pitch)
                    {
                        fwrite(u,width,1,yuv_file);
                    }
                    for (i=0;i<height;i++,v+=pitch)
                    {
                        fwrite(v,width,1,yuv_file);
                    }
                    fflush(yuv_file);
                }
#endif
            }
            // set stop event
            vm_event_signal(&pObj->m_hStopConversion);

            // wait for start of conversion
            vm_event_wait(&pObj->m_hStartConversion);
        }
    }

    return 0x05deccc;

} // unsigned int H264VideoDecoder::ConvertFrameAsyncSecondThread(void *p)

Status H264VideoDecoder::ConvertFrame(MediaData *dst)
{
    VideoData *pVData = DynamicCast<VideoData> (dst);
    Status ps = UMC_OK;
    H264DecYUVWorkSpace *pDisplayBuffer;

    VM_ASSERT(m_pDisplayFrame);

    pDisplayBuffer = m_pDisplayFrame;

    // Perform output color conversion and video effects, if we didn't
    // already write our output to the application's buffer.

    if (pVData)
    {

        if (NULL != m_pConverter)
        {
            InitColorConverter(m_pDisplayFrame, 0);
        }
        m_Convert.lpDest0 = (Ipp8u *) pVData->m_lpDest[0];
        m_Convert.PitchDest0 = pVData->m_lPitch[0];
        m_Convert.lpDest1 = NULL;
        m_Convert.PitchDest1 = 0;
        m_Convert.lpDest2 = NULL;
        m_Convert.PitchDest2 = 0;
        m_Convert.lpSource0 = pDisplayBuffer->m_pYPlane;
        m_Convert.lpSource2 = pDisplayBuffer->m_pUPlane;
        m_Convert.lpSource1 = pDisplayBuffer->m_pVPlane;
        m_Convert.PitchSource0 = pDisplayBuffer->pitch();
        m_Convert.PitchSource1 = pDisplayBuffer->pitch();
        m_Convert.PitchSource2 = pDisplayBuffer->pitch();

        if (NULL != m_pConverter)
        {
            m_pConverter->ConvertFrame(&m_Convert);
#ifdef STORE_OUTPUT_YUV
            if (yuv_file==NULL)
            {
                yuv_file=fopen(__YUV_FILE__,"w+b");
            }
            if (yuv_file)
            {
                Ipp32s pitch = m_Convert.PitchSource0;
                Ipp32s width = m_Convert.ConversionInit.SizeSource.width;
                Ipp32s height = m_Convert.ConversionInit.SizeSource.height;
                Ipp8u *y  = m_Convert.lpSource0;
                Ipp8u *u  = m_Convert.lpSource2;
                Ipp8u *v  = m_Convert.lpSource1;
                int i;
                for (i=0;i<height;i++,y+=pitch)
                {
                    fwrite(y,width,1,yuv_file);
                }
                switch(m_Convert.ConversionInit.FormatSource)
                {
                case YUV420:
                    width/=2;
                    height/=2;
                    break;
                case YUV422:
                    width/=2;
                    break;
                case YUV444:
                    break;
                }
                for (i=0;i<height;i++,u+=pitch)
                {
                    fwrite(u,width,1,yuv_file);
                }
                for (i=0;i<height;i++,v+=pitch)
                {
                    fwrite(v,width,1,yuv_file);
                }

                fflush(yuv_file);
            }
#endif
        }

        pVData->SetVideoParameters(m_Convert.ConversionInit.SizeDest.width,
                                   m_Convert.ConversionInit.SizeDest.height,
                                   m_Convert.ConversionInit.FormatDest);
    }

    return ps;

} // Status H264VideoDecoder::ConvertFrame(MediaData *dst)

#ifdef USE_SEI

Status H264VideoDecoder::OutputHalfFrame(H264DecoderFrame *pDisplayFrame, MediaData *dst, Ipp8u WhichField)
{
    VideoData *pVData = DynamicCast<VideoData> (dst);
    Status ps = UMC_OK;
    H264DecYUVWorkSpace *pDisplayBuffer;
    for (;;)
        // Not really a loop, just use break instead of goto on error
    {
        if (!pDisplayFrame)
        {
            ps = UMC_BAD_STREAM;
            break;
        }

        pDisplayBuffer = pDisplayFrame;

        // Perform output color conversion and video effects, if we didn't
        // already write our output to the application's buffer.

        if (pDisplayBuffer && pVData)
        {

            // we always use color space converter
            //            if (NULL == m_pConverter)
            //                m_pConverter = new ColorSpaceConverter();
            //            if (NULL == m_pConverter)
            //                return UMC_NOT_INITIALIZED;
            //            else
                if (NULL != m_pConverter)
                {
                    InitColorConverter(pDisplayFrame,1);
                }
                //and in the current frame
                Ipp8u cur_field,dest_field;
                cur_field = pDisplayFrame->m_PictureStructureFromSEI==6 || pDisplayFrame->m_PictureStructureFromSEI==4;
                dest_field = WhichField;
                m_Convert.lpDest0        = (Ipp8u *) pVData->m_lpDest[0]+dest_field*pVData->m_lPitch[0];
                m_Convert.PitchDest0    = pVData->m_lPitch[0]*2;
                m_Convert.lpDest1        = NULL;
                m_Convert.PitchDest1    = 0;
                m_Convert.lpDest2        = NULL;
                m_Convert.PitchDest2    = 0;
                m_Convert.lpSource0     = pDisplayBuffer->m_pYPlane+cur_field*pDisplayBuffer->pitch();
                m_Convert.lpSource2     = pDisplayBuffer->m_pUPlane+cur_field*pDisplayBuffer->pitch();
                m_Convert.lpSource1     = pDisplayBuffer->m_pVPlane+cur_field*pDisplayBuffer->pitch();
                m_Convert.PitchSource0  = pDisplayBuffer->pitch()*2;
                m_Convert.PitchSource1  = pDisplayBuffer->pitch()*2;
                m_Convert.PitchSource2  = pDisplayBuffer->pitch()*2;


                if (NULL != m_pConverter)
                {
                    m_pConverter->ConvertFrame(&m_Convert);
                }
            //pDisplayFrame->setWasDisplayed();
        }
        break;
    }

    return ps;

} // Status H264VideoDecoder::OutputHalfFrame(H264DecoderFrame *pDisplayFrame, MediaData *dst, Ipp8u WhichField)

Status H264VideoDecoder::OutputReverseFrame(H264DecoderFrame *pDisplayFrame, MediaData *dst)
{
    VideoData *pVData = DynamicCast<VideoData> (dst);
    Status ps = UMC_OK;
    H264DecYUVWorkSpace *pDisplayBuffer;
    for (;;)
        // Not really a loop, just use break instead of goto on error
    {
        if (!pDisplayFrame)
        {
            ps = UMC_BAD_STREAM;
            break;
        }

        pDisplayBuffer = pDisplayFrame;

        // Perform output color conversion and video effects, if we didn't
        // already write our output to the application's buffer.

        if (pDisplayBuffer && pVData)
        {

            // we always use color space converter
            //            if (NULL == m_pConverter)
            //                m_pConverter = new ColorSpaceConverter();
            //            if (NULL == m_pConverter)
            //                return UMC_NOT_INITIALIZED;
            //            else
            if (NULL != m_pConverter)
            {
                InitColorConverter(pDisplayFrame,1);
            }
            //and in the current frame
            m_Convert.lpDest0        = (Ipp8u *) pVData->m_lpDest[0]+pVData->m_lPitch[0];
            m_Convert.PitchDest0    = pVData->m_lPitch[0]*2;
            m_Convert.lpDest1        = NULL;
            m_Convert.PitchDest1    = 0;
            m_Convert.lpDest2        = NULL;
            m_Convert.PitchDest2    = 0;
            m_Convert.lpSource0     = pDisplayBuffer->m_pYPlane;
            m_Convert.lpSource2     = pDisplayBuffer->m_pUPlane;
            m_Convert.lpSource1     = pDisplayBuffer->m_pVPlane;
            m_Convert.PitchSource0  = pDisplayBuffer->pitch()*2;
            m_Convert.PitchSource1  = pDisplayBuffer->pitch()*2;
            m_Convert.PitchSource2  = pDisplayBuffer->pitch()*2;


            if (NULL != m_pConverter)
            {
                m_pConverter->ConvertFrame(&m_Convert);
            }
            m_Convert.lpDest0        = (Ipp8u *) pVData->m_lpDest[0];
            m_Convert.PitchDest0    = pVData->m_lPitch[0]*2;
            m_Convert.lpDest1        = NULL;
            m_Convert.PitchDest1    = 0;
            m_Convert.lpDest2        = NULL;
            m_Convert.PitchDest2    = 0;
            m_Convert.lpSource0     = pDisplayBuffer->m_pYPlane+pDisplayBuffer->pitch();
            m_Convert.lpSource2     = pDisplayBuffer->m_pUPlane+pDisplayBuffer->pitch();
            m_Convert.lpSource1     = pDisplayBuffer->m_pVPlane+pDisplayBuffer->pitch();
            m_Convert.PitchSource0  = pDisplayBuffer->pitch()*2;
            m_Convert.PitchSource1  = pDisplayBuffer->pitch()*2;
            m_Convert.PitchSource2  = pDisplayBuffer->pitch()*2;


            if (NULL != m_pConverter)
            {
                m_pConverter->ConvertFrame(&m_Convert);
            }
            //pDisplayFrame->setWasDisplayed();
        }
        break;
    }

    return ps;

} // Status H264VideoDecoder::OutputReverseFrame(H264DecoderFrame *pDisplayFrame, MediaData *dst)

#endif // USE_SEI

#endif	//DROP_COLOR_CONVERSION

} // namespace UMC
