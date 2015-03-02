/*
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//    Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//
*/
//***
#include "kinoma_ipp_lib.h"

#include "umc_h264_segment_decoder.h"
#include "umc_h264_dec_slice_store.h"
//***kinoma update
#include "umc_h264_dec_deblocking.h"
#include "vm_event.h"
#include "vm_thread.h"

namespace UMC
{

H264SegmentDecoder::H264SegmentDecoder(H264SliceStore_ &Store) :
    m_pSliceStore(&Store)
{
    m_pCoefficientsBuffer = NULL;
    m_pCoefficientsBufferExt = NULL;
    m_nAllocatedCoefficientsBuffer = 0;
    m_nAllocatedCoefficientsBufferExt = 0;

    // set pointer to backward prediction buffer
    m_pPredictionBuffer = align_pointer<Ipp8u *>(m_BufferForBackwardPrediction, DEFAULT_ALIGN_VALUE);

    // fill static table (sometimes it isn't initialized)
	//***kinoma update
	UMC::IppLumaDeblocking[0]	= ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_x;
	UMC::IppLumaDeblocking[1]	= ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_x;
	UMC::IppChromaDeblocking[0]	= ippiFilterDeblockingChroma_VerEdge_H264_8u_C1IR_x;
	UMC::IppChromaDeblocking[1]	= ippiFilterDeblockingChroma_HorEdge_H264_8u_C1IR_x;
	
	H264SegmentDecoder::DeblockChroma[0] = &H264SegmentDecoder::DeblockChroma400;
    H264SegmentDecoder::DeblockChroma[1] = &H264SegmentDecoder::DeblockChroma420;
    H264SegmentDecoder::DeblockChroma[2] = &H264SegmentDecoder::DeblockChroma422;
    H264SegmentDecoder::DeblockChroma[3] = &H264SegmentDecoder::DeblockChroma444;

	//*** kinoma modification
	m_approx = 0;
	SetDefaultInterpolationProcs();
	m_packet_loss_defensive_mode = 0;

} // H264SegmentDecoder::H264SegmentDecoder(H264SliceStore &Store)

H264SegmentDecoder::~H264SegmentDecoder(void)
{
    Release();

} // H264SegmentDecoder::~H264SegmentDecoder(void)

void H264SegmentDecoder::Release(void)
{
    if (m_pCoefficientsBuffer)
        delete [] m_pCoefficientsBuffer;
    if (m_pCoefficientsBufferExt)
        delete [] m_pCoefficientsBufferExt;

    m_pCoefficientsBuffer = NULL;
    m_pCoefficientsBufferExt  = NULL;
    m_nAllocatedCoefficientsBuffer = 0;

} // void H264SegmentDecoder::Release(void)

Status H264SegmentDecoder::Init(Ipp32s iNumber)
{
    // release object before initialization
    Release();

    // save ordinal number
    m_iNumber = iNumber;

    m_pCoefficientsBuffer = new Ipp16s[COEFFICIENTS_BUFFER_SIZE + DEFAULT_ALIGN_VALUE];
    m_pCoefficientsBufferExt = new Ipp32s[COEFFICIENTS_BUFFER_SIZE + DEFAULT_ALIGN_VALUE];
    m_nAllocatedCoefficientsBuffer = 1;

    return UMC_OK;

} // Status H264SegmentDecoder::Init(Ipp32s sNumber)

void recover_frame(int mb_width, int hole_start, int hole_end, H264VideoDecoder::H264DecoderFrame *src, H264VideoDecoder::H264DecoderFrame *dst)
{
	Ipp8u *dst_y = dst->m_pYPlane;
	Ipp8u *dst_u = dst->m_pUPlane;
	Ipp8u *dst_v = dst->m_pVPlane;
	int	rowBytesY	 = dst->pitch();
	int	rowBytesCbCr = dst->pitch();
	int i, j;

	if( src == NULL )
	{//patch gray
		for( i = hole_start; i < hole_end; i++ )
		{
            int curMB_X   = (i % mb_width);
            int curMB_Y   = (i / mb_width);
            int   x, y;
			Ipp8u *d;
			
            x = curMB_X * 16;
            y = curMB_Y * 16;
			d =  dst_y + (y*rowBytesY) + x;
			for( j = 0; j < 16; j ++ )
			{
				*(long *)(d+0)  = 0x80808080; 
				*(long *)(d+4)  = 0x80808080; 
				*(long *)(d+8)  = 0x80808080; 
				*(long *)(d+12) = 0x80808080; 
				d += rowBytesY;
			}

            x = curMB_X * 8;
            y = curMB_Y * 8;
			d =  dst_u + (y*rowBytesY) + x;
			for( j = 0; j < 8; j ++ )
			{
				*(long *)(d+0)  = 0x80808080; 
				*(long *)(d+4)  = 0x80808080; 
				d += rowBytesCbCr;
			}

            x = curMB_X * 8;
            y = curMB_Y * 8;
			d =  dst_v + (y*rowBytesY) + x;
			for( j = 0; j < 8; j ++ )
			{
				*(long *)(d+0)  = 0x80808080; 
				*(long *)(d+4)  = 0x80808080; 
				d += rowBytesCbCr;
			}
		}
	}
	else
	{
		Ipp8u *src_y = src->m_pYPlane;
		Ipp8u *src_u = src->m_pUPlane;
		Ipp8u *src_v = src->m_pVPlane;

		for( i = hole_start; i < hole_end; i++ )
		{
            int   curMB_X   = (i % mb_width);
            int   curMB_Y   = (i / mb_width);
            int   x, y;
			Ipp8u *d, *s;
			
            x = curMB_X * 16;
            y = curMB_Y * 16;
			d =  dst_y + (y*rowBytesY) + x;
			s =  src_y + (y*rowBytesY) + x;
			for( j = 0; j < 16; j ++ )
			{
				*(long *)(d+0)  = *(long *)(s+0) ; 
				*(long *)(d+4)  = *(long *)(s+4) ; 
				*(long *)(d+8)  = *(long *)(s+8) ; 
				*(long *)(d+12) = *(long *)(s+12); 
				d += rowBytesY;
				s += rowBytesY;
			}

            x = curMB_X * 8;
            y = curMB_Y * 8;
			d =  dst_u + (y*rowBytesY) + x;
			s =  src_u + (y*rowBytesY) + x;
			for( j = 0; j < 8; j ++ )
			{
				*(long *)(d+0)  = *(long *)(s+0); 
				*(long *)(d+4)  = *(long *)(s+4); 
				d += rowBytesCbCr;
				s += rowBytesCbCr;
			}

            x = curMB_X * 8;
            y = curMB_Y * 8;
			d =  dst_v + (y*rowBytesY) + x;
			s =  src_v + (y*rowBytesY) + x;
			for( j = 0; j < 8; j ++ )
			{
				*(long *)(d+0)  = *(long *)(s+0); 
				*(long *)(d+4)  = *(long *)(s+4); 
				d += rowBytesCbCr;
				s += rowBytesCbCr;
			}
		}
	}

}

Status H264SegmentDecoder::ProcessSegment(void)
{
    H264Task Task(m_iNumber);

	//***
	kinoma_slice_total = 0;

    while (m_pSliceStore->GetNextSlice(&Task))
    {
        Status umcRes;

        VM_ASSERT(Task.pFunctionSingle);

        if (TASK_DEB_FRAME != Task.m_iTaskID)
        {
            m_pSlice = Task.m_pSlice;
            m_pCurrentFrame = m_pSlice->GetCurrentFrame();
            m_pSliceHeader = m_pSlice->GetSliceHeader();

			//***bnie: m_field_index_kinoma_always_zero = m_pSliceHeader->bottom_field_flag_kinoma_always_zero;

            // reset decoding variables
            m_pBitStream = m_pSlice->GetBitStream();
            mb_height = m_pSlice->GetMBHeight();
            mb_width = m_pSlice->GetMBWidth();
            m_pPicParamSet = m_pSlice->GetPicParam();/*
            m_CurrentPicParamSet = m_pSlice->GetPicParamSet();*/
            m_pPredWeight[0] = m_pSlice->GetPredWeigthTable(0);
            m_pPredWeight[1] = m_pSlice->GetPredWeigthTable(1);/*
            m_CurrentSeqParamSet = m_pSlice->GetSeqParamSet();*/
            m_pSeqParamSet = m_pSlice->GetSeqParam();
            m_pMBIntraTypes = m_pSlice->GetMBIntraTypes();

            m_bUseSpatialDirectMode = (m_pSliceHeader->direct_spatial_mv_pred_flag) ? (true) : (false);
            m_bNeedToCheckMBSliceEdges = m_pSlice->NeedToCheckSliceEdges();
            m_mbinfo = m_pSlice->GetMBInfo();
        }
        else
        {
            // reset decoding variables
            m_pSlice = NULL;
            m_pCurrentFrame = Task.m_pSlice->GetCurrentFrame();
            m_pSliceHeader = NULL;
            //***bnie: m_field_index_kinoma_always_zero = Task.m_pSlice->GetFieldIndex_kinoma_always_zero();
        }

        // do decoding
        {
            umcRes = (this->*(Task.pFunctionSingle))(Task.m_iFirstMB, Task.m_iMBToProcess);

            if (UMC_END_OF_STREAM == umcRes)
                Task.m_iMaxMB = Task.m_iFirstMB + Task.m_iMBToProcess;
            else if (UMC_OK != umcRes)
                goto patch_holes;	//***kinoma update  --bnie 7/11/2008
				//return umcRes;

            Task.m_bDone = true;
        }

		kinoma_slice_start[kinoma_slice_total] = Task.m_iFirstMB;
		kinoma_slice_end[kinoma_slice_total]   = Task.m_iMaxMB;
		kinoma_slice_total ++;

#ifndef DROP_MULTI_THREAD_nonono
		// return performed task
        m_pSliceStore->AddPerformedTask(&Task);
#endif
	}

patch_holes:
	//if(0)
	{
		int mb_total   = mb_width*mb_height;
		int hole_start = 0;
		int hole_end   = 0;
		int i;
		H264VideoDecoder::H264DecoderFrame **pRefPicList0;

		//***kinoma enhancement   --bnie 8/2/2008
		pRefPicList0 = m_pCurrentFrame->GetRefPicListSafe( m_iNumber, 0 );
		//pRefPicList0 = m_pCurrentFrame->GetRefPicList(m_iNumber, 0)->m_RefPicList;
		
		for( i = 0; i < kinoma_slice_total; i++ )
		{
			int this_start = kinoma_slice_start[i];
			int this_end   = kinoma_slice_end[i];

			if( this_start > hole_end )
			{
				hole_end = this_start;
				recover_frame( mb_width, hole_start, hole_end, pRefPicList0[0], m_pCurrentFrame );
				this->SetPacketLossDefensiveMode(1);
			}
			hole_start = this_end;
			hole_end   = hole_start;
		}

		if( hole_end < mb_total )
			recover_frame( mb_width, hole_end, mb_total, pRefPicList0[0], m_pCurrentFrame );

	}
    return UMC_OK;

} // Status H264SegmentDecoder::ProcessSegment(void)

#if 0
Status H264SegmentDecoder::ProcessSlice(Ipp32s iCurMBNumber, Ipp32s &iMBToProcess)
{
    Status umcRes = UMC_OK;
    Ipp32s iFirstMB = iCurMBNumber;
    Ipp32s iMBRowSize = m_pSlice->GetMBRowWidth();
    Ipp32s iMaxMBNumber = iCurMBNumber + iMBToProcess;

    bool bDoDeblocking;
    Ipp32s iFirstMBToDeblock;

    // set deblocking condition
    bDoDeblocking = m_pSlice->GetDeblockingCondition();
    iFirstMBToDeblock = iCurMBNumber;

    // this cicle for row of MB
    for (; iCurMBNumber < iMaxMBNumber;)
    {
        Ipp32u nBorder;

        // calculate last MB in row
        if (false == m_pSlice->IsSliceGroups())
        {
            if (iCurMBNumber == iFirstMB)
                nBorder = min(iMaxMBNumber, iCurMBNumber - (iCurMBNumber % iMBRowSize) + iMBRowSize);
            else
                nBorder = min(iMaxMBNumber, iCurMBNumber + iMBRowSize);
        }
        else
            nBorder = iMaxMBNumber;

        // perform decoding on current row(s)
#ifndef DROP_CABAC
        if (0 == m_pPicParamSet->entropy_coding_mode)
#endif
		{
            //***bnie: if (m_pCurrentFrame->m_PictureStructureForDec >= FRM_STRUCTURE)
            {
                    umcRes = DecodeSegmentCAVLC(&iCurMBNumber, nBorder - iCurMBNumber);
            }
        }
#ifndef DROP_CABAC
        else
        {
            if (m_pCurrentFrame->m_PictureStructureForDec >= FRM_STRUCTURE)
            {
					umcRes = DecodeSegmentCABAC(&iCurMBNumber, nBorder - iCurMBNumber);
			}
        }
#endif
        if (UMC_OK != umcRes)
            break;

        // perform deblocking on previous row(s)
        {
            Ipp32s nToDeblock = iCurMBNumber - iFirstMBToDeblock - iMBRowSize;

            if ((bDoDeblocking) &&
                (0 < nToDeblock))
            {
                DeblockSegment(iFirstMBToDeblock, nToDeblock);
                iFirstMBToDeblock += nToDeblock;
            }
        }
    }

    if ((UMC_OK == umcRes) ||
        (UMC_END_OF_STREAM == umcRes))
    {
        iMBToProcess = iCurMBNumber - iFirstMB;

        // perform deblocking of remain MBs
        if (bDoDeblocking)
            DeblockSegment(iFirstMBToDeblock, iCurMBNumber - iFirstMBToDeblock);

        // in any case it is end of slice
        umcRes = UMC_END_OF_STREAM;
    }
	else
		 iMBToProcess = iCurMBNumber - iFirstMB;//*** kinoma update: also report progress so that we know where to fill the hole  --bnie 7/11/2008


    return umcRes;

} // Status H264SegmentDecoder::ProcessSlice(Ipp32s iCurMBNumber, Ipp32s &iMBToProcess)

#else
//***bnie: simplified barebone version: 4/6/2009
Status H264SegmentDecoder::ProcessSlice(Ipp32s iCurMBNumber, Ipp32s &iMBToProcess)
{
    Status umcRes = UMC_OK;
    Ipp32s iFirstMB = iCurMBNumber;
    Ipp32s iMBRowSize = m_pSlice->GetMBRowWidth();
    Ipp32s iMaxMBNumber = iCurMBNumber + iMBToProcess;

    bool bDoDeblocking;
    Ipp32s iFirstMBToDeblock;

    // set deblocking condition
    bDoDeblocking = m_pSlice->GetDeblockingCondition();
    iFirstMBToDeblock = iCurMBNumber;

    // this cicle for row of MB
    for (; iCurMBNumber < iMaxMBNumber;)
    {
        Ipp32u nBorder;

        // calculate last MB in row
        if (iCurMBNumber == iFirstMB)
            nBorder = min(iMaxMBNumber, iCurMBNumber - (iCurMBNumber % iMBRowSize) + iMBRowSize);
        else
            nBorder = min(iMaxMBNumber, iCurMBNumber + iMBRowSize);

		umcRes = DecodeSegmentCAVLC(&iCurMBNumber, nBorder - iCurMBNumber);

        if (UMC_OK != umcRes)
            break;

        // perform deblocking on previous row(s)
        {
            Ipp32s nToDeblock = iCurMBNumber - iFirstMBToDeblock - iMBRowSize;

            if ((bDoDeblocking) && (0 < nToDeblock))
            {
                DeblockSegment(iFirstMBToDeblock, nToDeblock);
                iFirstMBToDeblock += nToDeblock;
            }
        }
    }

    if ((UMC_OK == umcRes) ||
        (UMC_END_OF_STREAM == umcRes))
    {
        iMBToProcess = iCurMBNumber - iFirstMB;

        // perform deblocking of remain MBs
        if (bDoDeblocking)
            DeblockSegment(iFirstMBToDeblock, iCurMBNumber - iFirstMBToDeblock);

        // in any case it is end of slice
        umcRes = UMC_END_OF_STREAM;
    }
	else
		 iMBToProcess = iCurMBNumber - iFirstMB;//*** kinoma update: also report progress so that we know where to fill the hole  --bnie 7/11/2008

    return umcRes;

} // Status H264SegmentDecoder::ProcessSlice(Ipp32s iCurMBNumber, Ipp32s &iMBToProcess)

#endif

Status H264SegmentDecoder::DeblockSlice(Ipp32s nCurMBNumber, Ipp32s &nMBToDeblock)
{
    // when there is slice groups
    if (NULL == m_pSlice)
        DeblockFrame(nCurMBNumber, nMBToDeblock);
    else
        DeblockSegment(nCurMBNumber, nMBToDeblock);

    return UMC_OK;

} // Status H264SegmentDecoder::DeblockSlice(Ipp32s nCurMBNumber, Ipp32s &nMBToDeblock)

Ipp16s *H264SegmentDecoder::GetCoefficientsBuffer(Ipp32u nNum)
{
    return align_pointer<Ipp16s *> (m_pCoefficientsBuffer +
                                    COEFFICIENTS_BUFFER_SIZE * nNum, DEFAULT_ALIGN_VALUE);

} // Ipp16s *H264SegmentDecoder::GetCoefficientsBuffer(Ipp32u nNum)
Ipp32s *H264SegmentDecoder::GetCoefficientsBufferExt(Ipp32u nNum)
{
    return align_pointer<Ipp32s *> (m_pCoefficientsBufferExt +
        COEFFICIENTS_BUFFER_SIZE * nNum, DEFAULT_ALIGN_VALUE);

} // Ipp16s *H264SegmentDecoder::GetCoefficientsBuffer(Ipp32u nNum)

} // namespace UMC
