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

#ifndef __UMC_H264_SEGMENT_DECODER_H
#define __UMC_H264_SEGMENT_DECODER_H

#include "umc_h264_dec.h"
#include "umc_h264_slice_decoding.h"

#ifdef _KINOMA_LOSSY_OPT_
// WWD-200711
#include "kinoma_avc_defines.h"
#include "umc_h264_dec_deblocking.h"
#include "kinoma_ipp_lib.h"
#endif


namespace UMC
{

struct DeblockingParameters;
struct DeblockingParametersMBAFF;
class H264SliceStore_;
struct H264SliceHeader;

// implement array of IPP optimized luma deblocking functions
extern
IppStatus (__STDCALL *(IppLumaDeblocking[])) (Ipp8u *, Ipp32s, const Ipp8u *, const Ipp8u *, const Ipp8u *, const Ipp8u *);

// implement array of IPP optimized chroma deblocking functions
extern
IppStatus (__STDCALL *(IppChromaDeblocking[])) (Ipp8u *, Ipp32s, const Ipp8u *, const Ipp8u *, const Ipp8u *, const Ipp8u *);

//
// Class to incapsulate functions, implementing common decoding functional.
//

#pragma pack(16)

class H264SegmentDecoder
{
public:
    // Default constructor
    H264SegmentDecoder(H264SliceStore_ &Store);
    // Destructor
    virtual
    ~H264SegmentDecoder(void);

    // Initialize object
    virtual
    Status Init(Ipp32s iNumber);

    // Decode slice's segment
    virtual
    Status ProcessSegment(void);
/*
    // Get deblocking condition
    bool GetDeblockingCondition(void);
*/
    Status ProcessSlice(Ipp32s nCurMBNumber, Ipp32s &nMBToProcess);
    Status DeblockSlice(Ipp32s nCurMBNumber, Ipp32s &nMBToDeblock);

	//*** kinoma modification
	IppStatus (__STDCALL *m_ippiInterpolateLuma_H264_8u_C1R)		(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
	IppStatus (__STDCALL *m_ippiInterpolateLuma_H264_8u_C1R_16x16)	(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
	IppStatus (__STDCALL *m_ippiInterpolateLuma_H264_8u_C1R_16x8)	(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
	IppStatus (__STDCALL *m_ippiInterpolateLuma_H264_8u_C1R_8x16)	(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
	IppStatus (__STDCALL *m_ippiInterpolateLuma_H264_8u_C1R_8x8)	(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
	IppStatus (__STDCALL *m_ippiInterpolateLuma_H264_8u_C1R_8x4)	(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
	IppStatus (__STDCALL *m_ippiInterpolateLuma_H264_8u_C1R_4x8)	(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
	IppStatus (__STDCALL *m_ippiInterpolateLuma_H264_8u_C1R_4x4)	(const Ipp8u*   pSrc, Ipp32s   srcStep,Ipp8u*   pDst,Ipp32s   dstStep,Ipp32s   dx,	Ipp32s   dy, IppiSize roiSize);
	void SetDefaultInterpolationProcs( void ) 
	{
		m_ippiInterpolateLuma_H264_8u_C1R_16x16 = ippiInterpolateLuma_H264_8u_C1R_16x16_universal;
		m_ippiInterpolateLuma_H264_8u_C1R_16x8  = ippiInterpolateLuma_H264_8u_C1R_16x8_universal;
		m_ippiInterpolateLuma_H264_8u_C1R_8x16  = ippiInterpolateLuma_H264_8u_C1R_8x16_universal;
		m_ippiInterpolateLuma_H264_8u_C1R_8x8	= ippiInterpolateLuma_H264_8u_C1R_8x8_universal;
		m_ippiInterpolateLuma_H264_8u_C1R_8x4	= ippiInterpolateLuma_H264_8u_C1R_8x4_universal;
		m_ippiInterpolateLuma_H264_8u_C1R_4x8	= ippiInterpolateLuma_H264_8u_C1R_4x8_universal;
		m_ippiInterpolateLuma_H264_8u_C1R_4x4	= ippiInterpolateLuma_H264_8u_C1R_4x4_universal;
	}

	////*** WWD add approx functions for lossy De-blcoking
	//IppStatus (__STDCALL *m_ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR)		(Ipp8u *pSrcDst,	Ipp32s srcdstStep, Ipp8u *pAlpha,	Ipp8u *pBeta, Ipp8u *pThresholds, 	Ipp8u* pBs;
	//IppStatus (__STDCALL *m_ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR)		(Ipp8u *pSrcDst,	Ipp32s srcdstStep,	Ipp8u *pAlpha,	Ipp8u *pBeta,	Ipp8u *pThresholds,	Ipp8u *pBs);
	
	//***
	//for packet loss recovery
	int kinoma_slice_total;
	int kinoma_slice_start[10];
	int kinoma_slice_end[10];
	
	Ipp32s m_packet_loss_defensive_mode;
	void   SetPacketLossDefensiveMode(Ipp32s mode) { m_packet_loss_defensive_mode = mode; }
	Ipp32s GetPacketLossDefensiveMode() { return m_packet_loss_defensive_mode; }


	void SetApprox( int level ) 
	{	
		m_approx = level; 

#ifdef _KINOMA_LOSSY_OPT_
		if( (level & AVC_INTERPOLATION_SPEED)  != 0 )
		{
			m_ippiInterpolateLuma_H264_8u_C1R_16x16 = ippiInterpolateLuma_H264_8u_C1R_bilinear_approx_c;
			m_ippiInterpolateLuma_H264_8u_C1R_16x8  = ippiInterpolateLuma_H264_8u_C1R_bilinear_approx_c;
			m_ippiInterpolateLuma_H264_8u_C1R_8x16  = ippiInterpolateLuma_H264_8u_C1R_bilinear_approx_c;
			m_ippiInterpolateLuma_H264_8u_C1R_8x8	= ippiInterpolateLuma_H264_8u_C1R_bilinear_approx_c;
			m_ippiInterpolateLuma_H264_8u_C1R_8x4	= ippiInterpolateLuma_H264_8u_C1R_bilinear_approx_c;
			m_ippiInterpolateLuma_H264_8u_C1R_4x8	= ippiInterpolateLuma_H264_8u_C1R_bilinear_approx_c;
			m_ippiInterpolateLuma_H264_8u_C1R_4x4	= ippiInterpolateLuma_H264_8u_C1R_bilinear_approx_c;
		}
		else
#endif
		SetDefaultInterpolationProcs();

		// WWD 20041215, for de-blocking
		// This funciton should be the last init funciton for decoder, othewise,the init may be overwrited by other init functions!!
#ifdef _KINOMA_LOSSY_OPT_
		if( (level & AVC_DEBLOCKING_NOBRANCH)  == AVC_DEBLOCKING_NOBRANCH )
		{
			UMC::IppLumaDeblocking[0]	= ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_simple;
			UMC::IppLumaDeblocking[1]	= ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_simple;
		}
		else
#endif
		{
			UMC::IppLumaDeblocking[0]	= ippiFilterDeblockingLuma_VerEdge_H264_8u_C1IR_x;
			UMC::IppLumaDeblocking[1]	= ippiFilterDeblockingLuma_HorEdge_H264_8u_C1IR_x;
		}

#ifdef _KINOMA_LOSSY_OPT_
		AVC_Reconstruction_SetApprox(level);
#endif
	}

protected:
    // Release object
    void Release(void);

    // Function(s) to decode partition of macro block row
    Status DecodeSegmentCAVLC(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCAVLC_FLD(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCAVLC_H0(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCAVLC_H0_FLD(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCAVLC_H2(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCAVLC_H2_FLD(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCAVLC_H4(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCAVLC_H4_FLD(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCABAC(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCABAC_FLD(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCABAC_H0(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCABAC_H0_FLD(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCABAC_H2(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCABAC_H2_FLD(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCABAC_H4(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);
    Status DecodeSegmentCABAC_H4_FLD(Ipp32s *piCurMBNumber, Ipp32s iMacroBlocksToDecode);

    // Update info about current MB
    void UpdateCurrentMBInfo();
    //***bniebnie: Ipp32s CorrectAddrForMBAFF(Ipp32s MbAddr);
    Ipp32s MBAddr2MBCount(Ipp32s MbAddr);
    //***bniebnie: Ipp32s CorrectAddrFromMBAFF(Ipp32s MbAddr);
    Status AdjustIndex(Ipp8u ref_mb_is_bottom, Ipp8s ref_mb_is_field, Ipp8s &RefIdx);
    // Update neighbour's addresses
    void UpdateNeighbouringAddresses(Ipp8u IgnoreSliceEdges = 0);
    void UpdateNeighbouringAddressesField(void);
    // Update neighbouring's blocks info
    void UpdateNeighbouringBlocksField(void);
    inline void UpdateNeighbouringBlocksBMEH(Ipp8u DeblockCalls = 0);
    inline void UpdateNeighbouringBlocksH2(Ipp8u DeblockCalls = 0);
    inline void UpdateNeighbouringBlocksH4(Ipp8u DeblockCalls = 0);
    // Get context functions
    Ipp32u GetDCBlocksLumaContext()
    {
        bool use_above = m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num>=0;
        bool use_left = m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num>=0;
        Ipp8u above_coeffs=use_above?
            m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mb_above.block_num]:0;
        Ipp8u left_coeffs=use_left?
            m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mbs_left[0].block_num]:0;
        if(use_above && use_left) return (above_coeffs+left_coeffs+1)/2;
        else if (use_above ) return above_coeffs;
        else if (use_left) return left_coeffs;
        else return 0;
    }
    Ipp32u GetBlocksLumaContext(Ipp32s x,Ipp32s y)
    {
        bool use_above = y || m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num>=0;
        bool use_left = x || m_cur_mb.CurrentBlockNeighbours.mbs_left[y].mb_num>=0;
        Ipp8u above_coeffs=0;
        if (use_above) above_coeffs= y==0?
            m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mb_above.mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mb_above.block_num+x]:
            m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*4+x-4];
        Ipp8u left_coeffs=0;
        if (use_left)left_coeffs = x==0?
            m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mbs_left[y].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mbs_left[y].block_num]:
            m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*4+x-1];

        if(use_above && use_left) return (above_coeffs+left_coeffs+1)/2;
        else if (use_above ) return above_coeffs;
        else if (use_left) return left_coeffs;
        else return 0;
    }
    Ipp32u GetBlocksChromaContextBMEH(Ipp32s x,Ipp32s y,Ipp32s component)
    {
        Ipp8u above_coeffs=0;Ipp8u left_coeffs=0;
        bool use_above;bool use_left;
        if (component)
        {
            use_above = y || m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[1].mb_num>=0;
            use_left = x || m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[1][y].mb_num>=0;
            if (use_above)
                above_coeffs=y==0?
                    m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[1].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[1].block_num+x]:
                    m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*2+x-2+20];
            if (use_left)
                left_coeffs=x==0?
                    m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[1][y].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[1][y].block_num]:
                    m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*2+x-1+20];
        }
        else
        {
            use_above = y || m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[0].mb_num>=0;
            use_left = x || m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[0][y].mb_num>=0;
            if (use_above)
                above_coeffs=y==0?
                    m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[0].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[0].block_num+x]:
                    m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*2+x-2+16];
            if (use_left)
                left_coeffs=x==0?
                    m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[0][y].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[0][y].block_num]:
                    m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*2+x-1+16];
        }

        if(use_above && use_left) return (above_coeffs+left_coeffs+1)/2;
        else if (use_above ) return above_coeffs;
        else if (use_left) return left_coeffs;
        else return 0;
    }
    Ipp32u GetBlocksChromaContextH2(Ipp32s x,Ipp32s y,Ipp32s component)
    {
        Ipp8u above_coeffs=0;Ipp8u left_coeffs=0;
        bool use_above;bool use_left;
        if (component)
        {
            use_above = y || m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[1].mb_num>=0;
            use_left = x || m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[1][y].mb_num>=0;
            if (use_above)
                above_coeffs=y==0?
                m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[1].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[1].block_num+x]:
            m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*2+x-2+24];
            if (use_left)
                left_coeffs=x==0?
                m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[1][y].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[1][y].block_num]:
            m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*2+x-1+24];
        }
        else
        {
            use_above = y || m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[0].mb_num>=0;
            use_left = x || m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[0][y].mb_num>=0;
            if (use_above)
                above_coeffs=y==0?
                m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[0].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[0].block_num+x]:
            m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*2+x-2+16];
            if (use_left)
                left_coeffs=x==0?
                m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[0][y].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[0][y].block_num]:
            m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*2+x-1+16];
        }

        if(use_above && use_left) return (above_coeffs+left_coeffs+1)/2;
        else if (use_above ) return above_coeffs;
        else if (use_left) return left_coeffs;
        else return 0;
    }
    Ipp32u GetBlocksChromaContextH4(Ipp32s x,Ipp32s y,Ipp32s component)
    {
        Ipp8u above_coeffs=0;Ipp8u left_coeffs=0;
        bool use_above;bool use_left;
        if (component)
        {
            use_above = y || m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[1].mb_num>=0;
            use_left = x || m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[1][y].mb_num>=0;
            if (use_above)
                above_coeffs=y==0?
                m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[1].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[1].block_num+x]:
            m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*4+x-4+32];
            if (use_left)
                left_coeffs=x==0?
                m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[1][y].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[1][y].block_num]:
            m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*4+x-1+32];
        }
        else
        {
            use_above = y || m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[0].mb_num>=0;
            use_left = x || m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[0][y].mb_num>=0;
            if (use_above)
                above_coeffs=y==0?
                m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[0].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mb_above_chroma[0].block_num+x]:
            m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*4+x-4+16];
            if (use_left)
                left_coeffs=x==0?
                m_mbinfo.MacroblockCoeffsInfo[m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[0][y].mb_num].numCoeff[m_cur_mb.CurrentBlockNeighbours.mbs_left_chroma[0][y].block_num]:
            m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr].numCoeff[y*4+x-1+16];
        }

        if(use_above && use_left) return (above_coeffs+left_coeffs+1)/2;
        else if (use_above ) return above_coeffs;
        else if (use_left) return left_coeffs;
        else return 0;
    }
    // Decode macroblock type
    Status DecodeMacroBlockType(Ipp32u *pMBIntraTypes, Ipp32s *MBSkipCount, Ipp32s *PassFDFDecode);
    Status DecodeMBTypeBMEH_CABAC(Ipp32u *pMBIntraTypes, Ipp32s *MBSkipFlag, bool skip_next_fdf);
    Status DecodeMBTypeH2_CABAC(Ipp32u *pMBIntraTypes, Ipp32s *MBSkipFlag, bool skip_next_fdf);
    Status DecodeMBTypeH4_CABAC(Ipp32u *pMBIntraTypes, Ipp32s *MBSkipFlag, bool skip_next_fdf);
    Ipp32u Intra_MB_typeBMEH_CABAC();
    Ipp32u Intra_MB_typeH2_CABAC();
    Ipp32u Intra_MB_typeH4_CABAC();
    Ipp32u B_Picture_MB_type_CABAC();
    Ipp32u P_Picture_MB_type_CABAC();

    void ResetGMBData();

    // Decode intra block types
    Status DecodeIntraTypes4x4_CAVLC(Ipp32u *pMBIntraTypes, bool bUseConstrainedIntra);
    Status DecodeIntraTypes8x8_CAVLC(Ipp32u *pMBIntraTypes, bool bUseConstrainedIntra);
    Status DecodeIntraTypes4x4_CABAC(Ipp32u *pMBIntraTypes, bool bUseConstrainedIntra);
    Status DecodeIntraTypes8x8_CABAC(Ipp32u *pMBIntraTypes, bool bUseConstrainedIntra);
    // Decode the coefficients for a PCM macroblock, placing them
    // in m_pCoeffBlocksBuf.
    Status DecodeCoefficients_PCM(Ipp8u color_format);
    // Get colocated location
    Ipp32s GetColocatedLocation(H264VideoDecoder::H264DecoderFrame *pRefFrame, Ipp8u Field, Ipp32s &block, Ipp8s *scale = NULL);
    // Decode motion vectors
    void DecodeDirectMotionVectorsTemporal(Ipp32u sboffset, // offset into MV and RefIndex storage
                                           H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                           H264VideoDecoder::H264DecoderFrame **pRefPicList1,
                                           Ipp8s *pFields0,
                                           Ipp8s *pFields1,
                                           bool bIs8x8);
    // Decode motion vectors
    void DecodeDirectMotionVectorsTemporal_8x8Inference(H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                                        H264VideoDecoder::H264DecoderFrame **pRefPicList1,
                                                        Ipp8s *pFields0,
                                                        Ipp8s *pFields1,
                                                        Ipp32s iWhich8x8); // 0..3 if 8x8 block; -1 if 16x16
    // Decode motion vectors
    Status DecodeDirectMotionVectorsSpatial(H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                            H264VideoDecoder::H264DecoderFrame **pRefPicList1,
                                            Ipp8u Field,
                                            bool bUseDirect8x8Inference);
    // Compute  direct spatial reference indexes
    void ComputeDirectSpatialRefIdx(Ipp8s *pRefIndexL0, // neighbor RefIdx relative to this, forward result here
                                    Ipp8s *pRefIndexL1); // neighbor RefIdx relative to this, backward result here
    // Get reference indexes for block 4x4
    Status GetRefIdx4x4_CABAC(const Ipp32u nActive,
                                const Ipp8u* pBlkIdx,
                                const Ipp8u*  pCodRIx,
                                Ipp32u ListNum
                                );
    Status GetRefIdx4x4_CABAC(const Ipp32u nActive,
                                const Ipp8u  pCodRIx,
                                Ipp32u ListNum
                                );
    Status GetRefIdx4x4_16x8_CABAC(const Ipp32u nActive,
                                const Ipp8u*  pCodRIx,
                                Ipp32u ListNum
                                );
    Status GetRefIdx4x4_8x16_CABAC(const Ipp32u nActive,
                                const Ipp8u*  pCodRIx,
                                Ipp32u ListNum
                                );

    Ipp32s GetSE_RefIdx_CABAC(Ipp32u ListNum, Ipp32u BlockNum);
    // Get direct motion vectors for block 4x4
    Status GetMVD4x4_CABAC(
                            const Ipp8u* pBlkIdx,
                            const Ipp8u* pCodMVd,
                            Ipp32u ListNum
                            );
    Status GetMVD4x4_16x8_CABAC(
                            const Ipp8u* pCodMVd,
                            Ipp32u ListNum
                            );
    Status GetMVD4x4_8x16_CABAC(
                            const Ipp8u* pCodMVd,
                            Ipp32u ListNum
                            );
    Status GetMVD4x4_CABAC(
                            const Ipp8u pCodMVd,
                            Ipp32u      ListNum
                            );

    H264DecoderMotionVector GetSE_MVD_CABAC(Ipp32u ListNum, Ipp32u BlockNum);
    // Decode motion vectors
    Status DecodeMotionVectors(bool bIsBSlice);
    Status DecodeMotionVectors_CABAC();

    // Get direct motion vectors
    void GetDirectTemporalMV(Ipp32s MBCol,
                             Ipp32u ipos,
                             H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                             H264VideoDecoder::H264DecoderFrame **pRefPicList1,
                             H264DecoderMotionVector &MVL0, // return colocated MV here
                             Ipp8s &RefIndexL0); // return ref index here
    void GetDirectTemporalMVFLD(Ipp32s MBCol,
                                Ipp32u ipos,
                                H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                H264VideoDecoder::H264DecoderFrame **pRefPicList1,
                                Ipp8s *pFields0,
                                Ipp8s *pFields1,
                                H264DecoderMotionVector &MVL0, // return colocated MV here
                                Ipp8s &RefIndexL0); // return ref index here
    void GetDirectTemporalMVMBAFF(Ipp32s MBCol,
                                  Ipp32u ipos,
                                  H264VideoDecoder::H264DecoderFrame **pRefPicList0,
                                  H264VideoDecoder::H264DecoderFrame **pRefPicList1,
                                  H264DecoderMotionVector &MVL0, // return colocated MV here
                                  Ipp8s &RefIndexL0); // return ref index here
    // Decode and return the coded block pattern.
    // Return 255 is there is an error in the CBP.
    Ipp8u DecodeCBP_CAVLC(Ipp32u mbtype, Ipp8u color_format);
    Ipp32u DecodeCBP_CABAC(Ipp8u color_format);


    Status DecodeCoeffs16x16BMEH_CAVLC();
    Status DecodeCoeffs4x4BMEH_CAVLC();
    Status DecodeCoeffs8x8H0_CAVLC();

    Status DecodeCoeffs16x16H2_CAVLC();
    Status DecodeCoeffs4x4H2_CAVLC();
    Status DecodeCoeffs8x8H2_CAVLC();

    Status DecodeCoeffs16x16H4_CAVLC();
    Status DecodeCoeffs4x4H4_CAVLC();
    Status DecodeCoeffs8x8H4_CAVLC();

    Status DecodeChromaCoeffsBMEH_CAVLC();
    Status DecodeChromaCoeffsH2_CAVLC();
    Status DecodeChromaCoeffsH4_CAVLC();

    Status DecodeCoefficients4x4MH_CABAC();
    Status DecodeCoefficients4x4H2_CABAC();
    Status DecodeCoefficients4x4H4_CABAC();
    Status DecodeCoefficients8x8H0_CABAC();
    Status DecodeCoefficients8x8H2_CABAC();
    Status DecodeCoefficients8x8H4_CABAC();
    // Decode & skip motion vectors
    bool DecodeSkipMotionVectors();
    // Decode motion vector predictors
    void ComputeMotionVectorPredictors(Ipp8u ListNum,
                                       Ipp8s RefIndex, // reference index for this part
                                       Ipp32s block, // block or subblock number, depending on mbtype
                                       Ipp32s *pMVx, // resulting MV predictors
                                       Ipp32s *pMVy);
    // Decode slipped MB
    void DecodeMBSkipAndFDFBMEH(Ipp32s *MBSkipFlag, bool bIsBSlice);
    void DecodeMBSkipAndFDFH2(Ipp32s *MBSkipFlag, bool bIsBSlice);
    void DecodeMBSkipAndFDFH4(Ipp32s *MBSkipFlag, bool bIsBSlice);
    // Reconstruct PCM macroblock
    void ReconstructPCMMB(Ipp32u lumaOffset, Ipp32u chromaOffset,Ipp8u color_format);

    // Reconstruct MBAFF macroblock
    // Reconstruct MBAFF macroblock
    void ReconstructMacroblockMBAFFMEHC(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void ReconstructMacroblockMBAFFHM(Ipp8u *pDstY,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void ReconstructMacroblockMBAFFH2(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void ReconstructMacroblockMBAFFH4(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);

    void ReconstructMacroblockMBAFFNotWeightedFieldMEHC(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void ReconstructMacroblockMBAFFNotWeightedFieldHM(Ipp8u *pDstY,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void ReconstructMacroblockMBAFFNotWeightedFieldH2(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void ReconstructMacroblockMBAFFNotWeightedFieldH4(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void ReconstructMacroblockMBAFFNotWeightedFrameMEHC(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void ReconstructMacroblockMBAFFNotWeightedFrameHM(Ipp8u *pDstY,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void ReconstructMacroblockMBAFFNotWeightedFrameH2(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void ReconstructMacroblockMBAFFNotWeightedFrameH4(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    // Reconstruct common macroblock
    void ReconstructMacroblockBMEHC(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void ReconstructMacroblockHM(Ipp8u *pDstY,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void ReconstructMacroblockH2(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);
    void ReconstructMacroblockH4(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1);

    void ReconstructMacroblockFLDHM(Ipp8u *pDstY,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1,
        Ipp8s *pFields0,
        Ipp8s *pFields1);
    void ReconstructMacroblockFLDMEHC(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1,
        Ipp8s *pFields0,
        Ipp8s *pFields1);
    void ReconstructMacroblockFLDH2(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1,
        Ipp8s *pFields0,
        Ipp8s *pFields1);
    void ReconstructMacroblockFLDH4(Ipp8u *pDstY,
        Ipp8u *pDstV,
        Ipp8u *pDstU,
        Ipp32u mbXOffset, // for edge clipping
        Ipp32u mbYOffset,
        bool bUseDirect8x8Inference,
        H264VideoDecoder::H264DecoderFrame **pRefPicList0,
        H264VideoDecoder::H264DecoderFrame **pRefPicList1,
        Ipp8s *pFields0,
        Ipp8s *pFields1);

    // Get location functions
#ifdef _KINOMA_LOSSY_OPT_
	//WWD-200711
	void DeblockMacroblockChroma(DeblockingParameters *pParams);
#endif

    void GetLeftLumaBlockAcrossBoundary(H264DecoderBlockLocation *Block,Ipp8s y);
    void GetLeftChromaMacroBlockAcrossBoundary(Ipp32s& mb);
#if 0
    void GetLeftLocationForCurrentMBLumaMBAFF(H264DecoderBlockLocation *Block,Ipp32s AdditionalDecrement=0);
    void GetTopLocationForCurrentMBLumaMBAFF(H264DecoderBlockLocation *Block,Ipp8u DeblockCalls);
    void GetTopLeftLocationForCurrentMBLumaMBAFF(H264DecoderBlockLocation *Block);
    void GetTopRightLocationForCurrentMBLumaMBAFF(H264DecoderBlockLocation *Block);
    void GetTopLocationForCurrentMBChromaMBAFFBMEH(H264DecoderBlockLocation *Block);
    void GetLeftLocationForCurrentMBChromaMBAFFBMEH(H264DecoderBlockLocation *Block);
    void GetTopLocationForCurrentMBChromaMBAFFH2(H264DecoderBlockLocation *Block);
    void GetLeftLocationForCurrentMBChromaMBAFFH2(H264DecoderBlockLocation *Block);
    void GetTopLocationForCurrentMBChromaMBAFFH4(H264DecoderBlockLocation *Block);
    void GetLeftLocationForCurrentMBChromaMBAFFH4(H264DecoderBlockLocation *Block);
#endif

    void GetLeftLocationForCurrentMBLumaNonMBAFF(H264DecoderBlockLocation *Block);
    void GetTopLocationForCurrentMBLumaNonMBAFF(H264DecoderBlockLocation *Block);
   
    void GetTopLeftLocationForCurrentMBLumaNonMBAFF(H264DecoderBlockLocation *Block);
    void GetTopRightLocationForCurrentMBLumaNonMBAFF(H264DecoderBlockLocation *Block);
   
    void GetTopLocationForCurrentMBChromaNonMBAFFBMEH(H264DecoderBlockLocation *Block);
    void GetLeftLocationForCurrentMBChromaNonMBAFFBMEH(H264DecoderBlockLocation *Block);

    void GetTopLocationForCurrentMBChromaNonMBAFFH2(H264DecoderBlockLocation *Block);
    void GetLeftLocationForCurrentMBChromaNonMBAFFH2(H264DecoderBlockLocation *Block);

    void GetTopLocationForCurrentMBChromaNonMBAFFH4(H264DecoderBlockLocation *Block);
    void GetLeftLocationForCurrentMBChromaNonMBAFFH4(H264DecoderBlockLocation *Block);

    Ipp16s *GetCoefficientsBuffer(Ipp32u nNum = 0);
    Ipp16s *m_pCoefficientsBuffer;
    Ipp32u m_nAllocatedCoefficientsBuffer;
    Ipp32s *GetCoefficientsBufferExt(Ipp32u nNum = 0);
    Ipp32s *m_pCoefficientsBufferExt;
    Ipp32u m_nAllocatedCoefficientsBufferExt;

    H264Bitstream *m_pBitStream;                                // (H264Bitstream *) pointer to bit stream object
/*
    Ipp32s m_CurrentPicParamSet;                              // (Ipp32s) current picture parameter set number
    Ipp32s m_CurrentSeqParamSet;                              // (Ipp32s) current sequence parameter set number
*/
    Ipp32s mb_width;                                            // (Ipp32s) width in MB
    Ipp32s mb_height;                                           // (Ipp32s) height in MB
/**/    H264DecoderLocalMacroblockDescriptor m_mbinfo;              // (H264DecoderLocalMacroblockDescriptor) local MB data
    Ipp32s m_CurMBAddr;                                         // (Ipp32s) current macroblock address
    Ipp32s m_PairMBAddr;                                        // (Ipp32s) pair macroblock address (MBAFF only)
    Ipp16s *m_pCoeffBlocksWrite;                                // (Ipp16s *) pointer to write buffer
    Ipp16s *m_pCoeffBlocksRead;                                 // (Ipp16s *) pointer to read buffer
/*    struct H264DecoderStaticCoeffs
    {
        Ipp16s m_CoeffsBuffer[16 * 27 + DEFAULT_ALIGN_VALUE]; // (Ipp16s []) array of blocks to decode
        Ipp16s *Coeffs()
        {
            return align_pointer<Ipp16s *> (m_CoeffsBuffer, DEFAULT_ALIGN_VALUE);
        }
    } m_pCoeffBlocksBufStatic;*/
    Ipp32s m_CurMB_X;                                           // (Ipp32s) horizontal position of MB
    Ipp32s m_CurMB_Y;                                           // (Ipp32s) vertical position of MB
    H264DecoderCurrentMacroblockDescriptor m_cur_mb;            // (H264DecoderCurrentMacroblockDescriptor) current MB info
    bool m_bUseSpatialDirectMode;                               // (bool) flag of mc mode
    Ipp32s m_prev_dquant;
    H264SliceData *m_pSliceData;                                // (SliceData) current slice data
    bool m_bNeedToCheckMBSliceEdges;                            // (bool) flag to ...
    Ipp32s m_field_index_kinoma_always_zero;                                       // (Ipp32s) ordinal number of current field

    // external data
    H264PicParamSet *m_pPicParamSet;                            // (H264PicParamSet *) pointer to current picture parameters sets
    H264SeqParamSet *m_pSeqParamSet;                            // (H264SeqParamSet *) pointer to current sequence parameters sets
    H264VideoDecoder::H264DecoderFrame *m_pCurrentFrame;                              // (H264VideoDecoder::H264DecoderFrame *) pointer to frame being decoded
    Ipp32u *m_pMBIntraTypes;                                    // (Ipp32u *) pointer to array of intra types
    PredWeightTable *m_pPredWeight[2];                          // (PredWeightTable *) pointer to forward/backward (0/1) prediction table
    H264SliceStore_ * const m_pSliceStore;                        // (H264SliceStore *) pointer to slice store

    //
    // Deblocking tools
    //

#ifdef _KINOMA_LOSSY_OPT_
	// WWD-200711
	DeblockingParameters mParams;
#endif

    // forward declaration of internal types
    typedef void (H264SegmentDecoder::*DeblockingFunction)(Ipp32u nMBAddr);
    typedef void (H264SegmentDecoder::*ChromaDeblockingFunction)(Ipp32u dir, DeblockingParameters *pParams);

    static
    ChromaDeblockingFunction DeblockChroma[4];                      // (ChromaDeblockingFunction []) array of chroma deblocking functions

    // Perform deblocking on whole frame.
    // It is possible only for Baseline profile.
    void DeblockFrame(Ipp32u nCurMBNumber, Ipp32u nMacroBlocksToDeblock);
    // Function to de-block partition of macro block row
    void DeblockSegment(Ipp32u nCurMBNumber, Ipp32u nMacroBlocksToDeblock);
    // Working procedure for slice deblocking
    void DeblockMacroblocks(Ipp32u nFirstMBToDeblock, Ipp32u nMBToDeblock);

    //
    // Optimized deblocking functions
    //

    // Reset deblocking variables
#ifdef _KINOMA_LOSSY_OPT_
 	// WWD-200711
    void PrepareDeblockingParametersKinoma(DeblockingParameters *pParams);

	void DeblockLuma_noAFF(DeblockingParameters *pParams);
    void ResetDeblockingVariablesKinoma(DeblockingParameters *pParams);
    void ResetDeblockingVariablesKinoma2(DeblockingParameters *pParams);
#endif

    void ResetDeblockingVariables(DeblockingParameters *pParams);

    void ResetDeblockingVariablesMBAFF(DeblockingParametersMBAFF *pParams);
    // Function to do luma deblocking
    void DeblockLuma(Ipp32u dir, DeblockingParameters *pParams);
    void DeblockLumaVerticalMBAFF(DeblockingParametersMBAFF *pParams);
    void DeblockLumaHorizontalMBAFF(DeblockingParametersMBAFF *pParams);
    // Function to do chroma deblocking
    void DeblockChroma400(Ipp32u dir, DeblockingParameters *pParams);
    void DeblockChroma420(Ipp32u dir, DeblockingParameters *pParams);
    void DeblockChroma422(Ipp32u dir, DeblockingParameters *pParams);
    void DeblockChroma444(Ipp32u dir, DeblockingParameters *pParams);
    void DeblockChromaVerticalMBAFF(DeblockingParametersMBAFF *pParams);
    void DeblockChromaHorizontalMBAFF(DeblockingParametersMBAFF *pParams);
    // Function to prepare deblocking parameters for mixed MB types
    void DeblockMacroblockMSlice(Ipp32u MBAddr);

    //
    // Function to do deblocking on I slices
    //

    void DeblockMacroblockISlice(Ipp32u MBAddr);
    void PrepareDeblockingParametersISlice(DeblockingParameters *pParams);
    void DeblockMacroblockISliceMBAFF(Ipp32u MBAddr);
    void PrepareDeblockingParametersISliceMBAFF(DeblockingParametersMBAFF *pParams);

    //
    // Function to do deblocking on P slices
    //

    void DeblockMacroblockPSlice(Ipp32u MBAddr);
    void DeblockMacroblockPSliceMBAFF(Ipp32u MBAddr);
    void PrepareDeblockingParametersPSlice(DeblockingParameters *pParams);
    void PrepareDeblockingParametersPSliceMBAFF(DeblockingParametersMBAFF *pParams);
    // Prepare deblocking parameters for macroblocks from P slice
    // MbPart is 16, MbPart of opposite direction is 16
    void PrepareDeblockingParametersPSlice16(Ipp32u dir, DeblockingParameters *pParams);
    // Prepare deblocking parameters for macroblocks from P slice
    // MbPart is 8, MbPart of opposite direction is 16
    void PrepareDeblockingParametersPSlice8x16(Ipp32u dir, DeblockingParameters *pParams);
    // Prepare deblocking parameters for macroblocks from P slice
    // MbPart is 16, MbPart of opposite direction is 8
    void PrepareDeblockingParametersPSlice16x8(Ipp32u dir, DeblockingParameters *pParams);
    // Prepare deblocking parameters for macroblocks from P slice
    // MbParts of both directions are 4
    void PrepareDeblockingParametersPSlice4(Ipp32u dir, DeblockingParameters *pParams);
    void PrepareDeblockingParametersPSlice4MBAFFField(Ipp32u dir, DeblockingParameters *pParams);
    // Prepare deblocking parameters for macroblock from P slice,
    // which coded in frame mode, but above macroblock is coded in field mode
    void PrepareDeblockingParametersPSlice4MBAFFMixedExternalEdge(DeblockingParameters *pParams);
    // Prepare deblocking parameters for macroblock from P slice,
    // which coded in frame mode, but left macroblock is coded in field mode
    void PrepareDeblockingParametersPSlice4MBAFFComplexFrameExternalEdge(DeblockingParametersMBAFF *pParams);
    // Prepare deblocking parameters for macroblock from P slice,
    // which coded in field mode, but left macroblock is coded in frame mode
    void PrepareDeblockingParametersPSlice4MBAFFComplexFieldExternalEdge(DeblockingParametersMBAFF *pParams);

    //
    // Function to do deblocking on B slices
    //

    void DeblockMacroblockBSlice(Ipp32u MBAddr);
    void DeblockMacroblockBSliceMBAFF(Ipp32u MBAddr);
    void PrepareDeblockingParametersBSlice(DeblockingParameters *pParams);
    void PrepareDeblockingParametersBSliceMBAFF(DeblockingParametersMBAFF *pParams);
    // Prepare deblocking parameters for macroblocks from B slice
    // MbPart is 16, MbPart of opposite direction is 16
    void PrepareDeblockingParametersBSlice16(Ipp32u dir, DeblockingParameters *pParams);
    // Prepare deblocking parameters for macroblocks from B slice
    // MbPart is 8, MbPart of opposite direction is 16
    void PrepareDeblockingParametersBSlice8x16(Ipp32u dir, DeblockingParameters *pParams);
    // Prepare deblocking parameters for macroblocks from B slice
    // MbPart is 16, MbPart of opposite direction is 8
    void PrepareDeblockingParametersBSlice16x8(Ipp32u dir, DeblockingParameters *pParams);
    // Prepare deblocking parameters for macroblocks from B slice
    // MbParts of both directions are 4
    void PrepareDeblockingParametersBSlice4(Ipp32u dir, DeblockingParameters *pParams);
    void PrepareDeblockingParametersBSlice4MBAFFField(Ipp32u dir, DeblockingParameters *pParams);

    volatile
    DeblockingFunction m_pMacroblockDeblockingFunction;         // (DeblockingFunction) current function to deblock macroblock

    volatile
    bool m_bFrameDeblocking;                                    // (bool) frame deblocking flag

    Ipp32s m_iNumber;                                           // (Ipp32s) ordinal number of decoder
    H264Slice *m_pSlice;                                        // (H264Slice *) current slice pointer
    H264SliceHeader *m_pSliceHeader;                            // (H264SliceHeader *) current slice header

#ifdef _KINOMA_LOSSY_OPT_
    H264SliceHeader *m_pSliceHeader_deblk;			// WWD-200711,add this to eliminate branch for each MB
#endif													// It will be initilized in function DeblockMacroblockMSlice and DeblockSegment

    Ipp8u m_BufferForBackwardPrediction[16 * 16 * 3 + DEFAULT_ALIGN_VALUE]; // (Ipp8u []) allocated buffer for backward prediction
    Ipp8u *m_pPredictionBuffer;                                 // (Ipp8u *) pointer to aligned buffer for backward prediction

private:
    // we lock assignment operator to avoid any
    // accasionaly assignments
    H264SegmentDecoder & operator = (H264SegmentDecoder &)
    {
        return *this;

    } // H264SegmentDecoder & operator = (H264SegmentDecoder &)

#ifdef USE_DETAILED_TIMING
public:
    TimingInfo m_clsTimingInfo;
#endif // USE_DETAILED_TIMING
	
	//*** kinoma modification
	int m_approx;
};

#pragma pack()

// codes used to indicate the coding state of each block
const Ipp8u CodNone = 0;    // no code
const Ipp8u CodInBS = 1;    // read code from bitstream
const Ipp8u CodLeft = 2;    // code same as left 4x4 subblock
const Ipp8u CodAbov = 3;    // code same as above 4x4 subblock
const Ipp8u CodSkip = 4;    // skip for direct mode

// declaration of const table(s)
extern const
Ipp8u CodeToMBTypeB[];
extern const
Ipp8u CodeToBDir[][2];
extern const
Ipp32s NIT2LIN[16];
extern const
Ipp32s SBTYPESIZE[5][4];
extern const
Ipp8u SbPartNumMinus1[2][17];
extern const
Ipp8u sb8x8[4];
extern const
Ipp8u pCodFBD[5][4];
extern const
Ipp8u pCodTemplate[16];
extern const
Ipp32u sb_x[4][16];
extern const
Ipp32u sb_y[4][16];
extern const
Ipp32s sign_mask[2];
extern const
Ipp32s cbp_bits_table[4];

#ifdef _KINOMA_LOSSY_OPT_
//WWD-200711
inline
void H264SegmentDecoder::DeblockMacroblockChroma(DeblockingParameters *pParams)
{
	if( !(m_approx & AVC_DEBLOCKING_DROP_CHROMA))
    {
#ifdef H_422
        Ipp32u color_format = m_pSeqParamSet->chroma_format_idc;

        (this->*DeblockChroma[color_format])(VERTICAL_DEBLOCKING, &params);
        (this->*DeblockChroma[color_format])(HORIZONTAL_DEBLOCKING, &params);
#else
		DeblockChroma420(VERTICAL_DEBLOCKING, pParams);
		DeblockChroma420(HORIZONTAL_DEBLOCKING, pParams);
#endif
    }

}
#endif

inline
void H264SegmentDecoder::GetLeftLocationForCurrentMBLumaNonMBAFF(H264DecoderBlockLocation *Block)
{
    //luma
    if (BLOCK_IS_ON_LEFT_EDGE(Block->block_num))
    {
        Block->block_num+=3;
        Block->mb_num=m_cur_mb.CurrentMacroblockNeighbours.mb_A;
    }
    else
    {
        Block->block_num--;
        Block->mb_num=m_CurMBAddr;
    }

    return;

} // void H264SegmentDecoder::GetLeftLocationForCurrentMBLumaNonMBAFF(H264DecoderBlockLocation *Block)


inline
void H264SegmentDecoder::GetTopLocationForCurrentMBLumaNonMBAFF(H264DecoderBlockLocation *Block)
{
    //luma
    if (BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        Block->block_num+=12;
        Block->mb_num  = m_cur_mb.CurrentMacroblockNeighbours.mb_B;
    }
    else
    {
        Block->block_num-=4;
        Block->mb_num = m_CurMBAddr;
    }

    return;

} // void H264SegmentDecoder::GetTopLocationForCurrentMBLumaNonMBAFF(H264DecoderBlockLocation *Block)

inline
void H264SegmentDecoder::GetTopLeftLocationForCurrentMBLumaNonMBAFF(H264DecoderBlockLocation *Block)
{
    //luma
    if (BLOCK_IS_ON_LEFT_EDGE(Block->block_num) && BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        Block->block_num+=15;
        Block->mb_num = m_cur_mb.CurrentMacroblockNeighbours.mb_D;
    }
    else if (BLOCK_IS_ON_LEFT_EDGE(Block->block_num))
    {
        Block->block_num--;
        Block->mb_num = m_cur_mb.CurrentMacroblockNeighbours.mb_A;
    }
    else if ( BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        Block->block_num+=11;
        Block->mb_num = m_cur_mb.CurrentMacroblockNeighbours.mb_B;
    }
    else
    {
        Block->block_num-=5;
        Block->mb_num = m_CurMBAddr;
    }

    return;

} // void H264SegmentDecoder::GetTopLeftLocationForCurrentMBLumaNonMBAFF(H264DecoderBlockLocation *Block)

inline
void H264SegmentDecoder::GetTopRightLocationForCurrentMBLumaNonMBAFF(H264DecoderBlockLocation *Block)
{
    //luma
    if (Block->block_num==3)
    {
        Block->block_num+=9;
        Block->mb_num = m_cur_mb.CurrentMacroblockNeighbours.mb_C;
    }
    else if ( BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        Block->block_num+=13;
        Block->mb_num = m_cur_mb.CurrentMacroblockNeighbours.mb_B;
    }
    else if (!above_right_avail_4x4[Block->block_num])
    {
        Block->mb_num = -1;
    }
    else
    {
        Block->block_num-=3;
        Block->mb_num = m_CurMBAddr;
    }

    return;

} // void H264SegmentDecoder::GetTopRightLocationForCurrentMBLumaNonMBAFF(H264DecoderBlockLocation *Block)

#if 0
inline
void H264SegmentDecoder::GetLeftLocationForCurrentMBLumaMBAFF(H264DecoderBlockLocation *Block,Ipp32s AdditionalDecrement)
{
    bool curmbfff = !pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = !pGetMBBottomFlag(m_cur_mb.GlobalMacroblockInfo);
    //Ipp32s mb_width = m_pCurrentFrame->macroBlockSize().width;
    Ipp32s MB_X=m_cur_mb.CurrentMacroblockNeighbours.mb_A;
    Ipp32s MB_N;
        //luma
    if (BLOCK_IS_ON_LEFT_EDGE(Block->block_num))
    {
        if (MB_X>=0)
        {
            Ipp8u xfff=!GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[MB_X]);
            if (curmbfff)
            {
                if (curmbtf)
                {
                    if (xfff)
                    {
                        // 1 1 1
                        MB_N=MB_X;
                    }
                    else
                    {
                        // 1 1 0
                        Ipp32u yN = Block->block_num/4;
                        yN*=4;
                        yN-=AdditionalDecrement;
                        yN/=2;
                        Block->block_num=(yN/4)*4;
                        if (AdditionalDecrement)
                            MB_N=MB_X  + mb_width;
                        else
                            MB_N=MB_X;
                        AdditionalDecrement=0;
                    }
                }
                else
                {
                    if (xfff)
                    {
                        // 1 0 1
                        MB_N=MB_X + mb_width;
                    }
                    else
                    {
                        // 1 0 0
                        Ipp32u yN = Block->block_num/4;
                        yN*=4;
                        yN-=AdditionalDecrement;
                        yN+=16;
                        yN/=2;
                        Block->block_num=(yN/4)*4;
                        if (AdditionalDecrement)
                            MB_N=MB_X + mb_width;
                        else
                            MB_N=MB_X;
                        AdditionalDecrement=0;
                    }
                }
            }
            else
            {
                if (curmbtf)
                {
                    if (xfff)
                    {
                        //0 1 1
                        Ipp32u yN = Block->block_num/4;
                        yN*=4;
                        yN-=AdditionalDecrement;
                        yN*=2;
                        if (yN<16)
                        {
                            MB_N=MB_X;
                        }
                        else
                        {
                            yN-=16;
                            MB_N=MB_X + mb_width;
                        }
                        Block->block_num=(yN/4)*4;
                        AdditionalDecrement=0;
                    }
                    else
                    {
                        // 0 1 0
                        MB_N=MB_X;
                    }
                }
                else
                {
                    if (xfff)
                    {
                        // 0 0 1
                        Ipp32u yN = Block->block_num/4;
                        yN*=4;
                        yN-=AdditionalDecrement;
                        yN*=2;
                        if (yN<15)
                        {
                            yN++;
                            MB_N=MB_X;
                        }
                        else
                        {
                            yN-=15;
                            MB_N=MB_X + mb_width;
                        }
                        Block->block_num=(yN/4)*4;
                        AdditionalDecrement=0;
                    }
                    else
                    {
                        // 0 0 0
                        MB_N=MB_X + mb_width;
                    }
                }
            }
        }
        else
        {
            Block->mb_num = -1;//no left neighbours
            return;
        }
        Block->block_num+=3-4*AdditionalDecrement;
        Block->mb_num = MB_N;
    }
    else
    {
        Block->block_num--;
        Block->mb_num = m_CurMBAddr;
    }

    return;

} // void H264SegmentDecoder::GetLeftLocationForCurrentMBLumaMBAFF(H264DecoderBlockLocation *Block,Ipp32s AdditionalDecrement)


inline
void H264SegmentDecoder::GetTopLocationForCurrentMBLumaMBAFF(H264DecoderBlockLocation *Block,Ipp8u DeblockCalls)
{
    bool curmbfff = !pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = !pGetMBBottomFlag(m_cur_mb.GlobalMacroblockInfo);
    //Ipp32s mb_width = m_pCurrentFrame->macroBlockSize().width;
    Ipp32s pair_offset = (curmbtf) ? (mb_width) : (-((signed) mb_width));
    Ipp32s MB_X;
    Ipp32s MB_N;
    //luma
    if (BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        if (curmbfff && !curmbtf )
        {
            MB_N = m_CurMBAddr + pair_offset;
            Block->block_num+=12;
        }
        else
        {
            MB_X = m_cur_mb.CurrentMacroblockNeighbours.mb_B;
            if (MB_X>=0)
            {
                Ipp8u xfff=!GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[MB_X]);
                MB_N=MB_X;
                Block->block_num+=12;
                if (curmbfff || !curmbtf || xfff)
                {
                    if (!(curmbfff && curmbtf && !xfff && DeblockCalls))
                        MB_N+= mb_width;
                }
            }
            else
            {
                Block->mb_num = -1;
                return;
            }
        }
        Block->mb_num = MB_N;
        return ;
    }
    else
    {
        Block->block_num-=4;
        Block->mb_num = m_CurMBAddr;
        return;
    }

} // void H264SegmentDecoder::GetTopLocationForCurrentMBLumaMBAFF(H264DecoderBlockLocation *Block,Ipp8u DeblockCalls)


inline
void H264SegmentDecoder::GetTopLeftLocationForCurrentMBLumaMBAFF(H264DecoderBlockLocation *Block)
{
    bool curmbfff = !pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = !pGetMBBottomFlag(m_cur_mb.GlobalMacroblockInfo);
    Ipp32s MB_X;
    Ipp32s MB_N;
    //luma
    if (BLOCK_IS_ON_LEFT_EDGE(Block->block_num) && BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        if (curmbfff && !curmbtf )
        {
            MB_X = m_cur_mb.CurrentMacroblockNeighbours.mb_A;
            if (MB_X<0)
            {
                Block->mb_num = -1;
                return;
            }
            if (!GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[MB_X]))
            {
                MB_N = MB_X;
                Block->block_num+=15;
            }
            else
            {
                MB_N = MB_X + mb_width;
                Block->block_num+=7;
            }
        }
        else
        {
            MB_X = m_cur_mb.CurrentMacroblockNeighbours.mb_D;
            if (MB_X>=0)
            {
                if (curmbfff==curmbtf)
                {
                    MB_N=MB_X + mb_width;
                }
                else
                {
                    if (!GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[MB_X]))
                    {
                        MB_N=MB_X + mb_width;
                    }
                    else
                    {
                        MB_N=MB_X;
                    }
                }
                Block->block_num+=15;
            }
            else
            {
                Block->mb_num = -1;
                return;
            }
        }

        Block->mb_num = MB_N;
        return;
    }
    else if (BLOCK_IS_ON_LEFT_EDGE(Block->block_num))
    {
        //Block->block_num-=4;
        GetLeftLocationForCurrentMBLumaMBAFF(Block,1);
        return;
    }
    else if (BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        Block->block_num--;
        GetTopLocationForCurrentMBLumaMBAFF(Block,0);
        return;
    }
    else
    {
        Block->block_num-=5;
        Block->mb_num = m_CurMBAddr;
        return;
    }

} // void H264SegmentDecoder::GetTopLeftLocationForCurrentMBLumaMBAFF(H264DecoderBlockLocation *Block)

inline
void H264SegmentDecoder::GetTopRightLocationForCurrentMBLumaMBAFF(H264DecoderBlockLocation *Block)
{
    bool curmbfff = !pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = !pGetMBBottomFlag(m_cur_mb.GlobalMacroblockInfo);
    Ipp32s MB_X;
    Ipp32s MB_N;
    //luma
    if (Block->block_num==3)
    {
        if (curmbfff && !curmbtf )
        {
            Block->mb_num = -1;
            return;
        }
        else
        {
            MB_X = m_cur_mb.CurrentMacroblockNeighbours.mb_C;
            if (MB_X>=0)
            {
                if (curmbfff==curmbtf)
                {
                    MB_N=MB_X + mb_width;
                }
                else
                {
                    if (!GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[MB_X]))
                    {
                        MB_N=MB_X + mb_width;
                    }
                    else
                    {
                        MB_N=MB_X;
                    }
                }
                Block->block_num+=9;
            }
            else
            {
                Block->mb_num = -1;
                return;
            }
        }
        Block->mb_num = MB_N;
        return;
    }
    else if ( BLOCK_IS_ON_TOP_EDGE(Block->block_num))
    {
        Block->block_num++;
        GetTopLocationForCurrentMBLumaMBAFF(Block,0);
        return;
    }
    else if (!above_right_avail_4x4_lin[Block->block_num])
    {
        Block->mb_num = -1;
        return;
    }
    else
    {
        Block->block_num-=3;
        Block->mb_num = m_CurMBAddr;
    }

    return;

} // void H264SegmentDecoder::GetTopRightLocationForCurrentMBLumaMBAFF(H264DecoderBlockLocation *Block)

#endif

inline
void H264SegmentDecoder::GetLeftLocationForCurrentMBChromaNonMBAFFBMEH(H264DecoderBlockLocation *Block)
{
    //chroma
    if (CHROMA_BLOCK_IS_ON_LEFT_EDGE(Block->block_num-16,1))
    {
        Block->block_num+=1;
        Block->mb_num=m_cur_mb.CurrentMacroblockNeighbours.mb_A;
    }
    else
    {
        Block->block_num--;
        Block->mb_num=m_CurMBAddr;
    }

    return;

}

inline
void H264SegmentDecoder::GetLeftLocationForCurrentMBChromaNonMBAFFH2(H264DecoderBlockLocation *Block)
{
    //chroma
    if (CHROMA_BLOCK_IS_ON_LEFT_EDGE(Block->block_num-16,2))
    {
        Block->block_num+=1;
        Block->mb_num=m_cur_mb.CurrentMacroblockNeighbours.mb_A;
    }
    else
    {
        Block->block_num--;
        Block->mb_num=m_CurMBAddr;
    }

    return;

}

inline
void H264SegmentDecoder::GetLeftLocationForCurrentMBChromaNonMBAFFH4(H264DecoderBlockLocation *Block)
{
    //chroma
    if (CHROMA_BLOCK_IS_ON_LEFT_EDGE(Block->block_num-16,3))
    {
        Block->block_num+=3;
        Block->mb_num=m_cur_mb.CurrentMacroblockNeighbours.mb_A;
    }
    else
    {
        Block->block_num--;
        Block->mb_num=m_CurMBAddr;
    }

    return;

}

inline
void H264SegmentDecoder::GetTopLocationForCurrentMBChromaNonMBAFFBMEH(H264DecoderBlockLocation *Block)
{
    //chroma
    if (CHROMA_BLOCK_IS_ON_TOP_EDGE(Block->block_num-16,1))
    {
        Block->block_num+=2;
        Block->mb_num  = m_cur_mb.CurrentMacroblockNeighbours.mb_B;
    }
    else
    {
        Block->block_num-=2;
        Block->mb_num = m_CurMBAddr;
    }
    return;
}

inline
void H264SegmentDecoder::GetTopLocationForCurrentMBChromaNonMBAFFH2(H264DecoderBlockLocation *Block)
{
    //chroma
    if (CHROMA_BLOCK_IS_ON_TOP_EDGE(Block->block_num-16,2))
    {
        Block->block_num+=6;
        Block->mb_num  = m_cur_mb.CurrentMacroblockNeighbours.mb_B;
    }
    else
    {
        Block->block_num-=2;
        Block->mb_num = m_CurMBAddr;
    }
    return;
}

inline
void H264SegmentDecoder::GetTopLocationForCurrentMBChromaNonMBAFFH4(H264DecoderBlockLocation *Block)
{
    //chroma
    if (CHROMA_BLOCK_IS_ON_TOP_EDGE(Block->block_num-16,3))
    {
        Block->block_num+=12;
        Block->mb_num  = m_cur_mb.CurrentMacroblockNeighbours.mb_B;
    }
    else
    {
        Block->block_num-=4;
        Block->mb_num = m_CurMBAddr;
    }
    return;
}

inline

#if 0
void H264SegmentDecoder::GetLeftLocationForCurrentMBChromaMBAFFBMEH(H264DecoderBlockLocation *Block)
{
    bool curmbfff = !pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = !pGetMBBottomFlag(m_cur_mb.GlobalMacroblockInfo);
    //Ipp32s mb_width = m_pCurrentFrame->macroBlockSize().width;
    Ipp32s MB_X=m_cur_mb.CurrentMacroblockNeighbours.mb_A;
    Ipp32s MB_N;
    Block->block_num-=16;
    //chroma
    if (CHROMA_BLOCK_IS_ON_LEFT_EDGE(Block->block_num,1))
    {
        if (MB_X>=0) //left mb addr vaild?
        {
            Ipp8u xfff=!GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[MB_X]);
            if (curmbfff)
            {
                if (curmbtf)
                {
                    if (xfff)
                    {
                        // 1 1 1
                        MB_N=MB_X;
                    }
                    else
                    {
                        // 1 1 0
                        Ipp32u yN = Block->block_num/2, xN=Block->block_num%2;
                        yN/=2;
                        Block->block_num=yN*2+xN;
                        MB_N=MB_X;
                    }
                }
                else
                {
                    if (xfff)
                    {
                        // 1 0 1
                        MB_N=MB_X+mb_width;
                    }
                    else
                    {
                        // 1 0 0
                        Ipp32u yN = Block->block_num/2, xN=Block->block_num%2;
                        yN+=2;
                        yN/=2;
                        Block->block_num=yN*2+xN;
                        MB_N=MB_X;
                    }
                }
            }
            else
            {
                if (curmbtf)
                {
                    if (xfff)
                    {
                        //0 1 1
                        Ipp32u yN = Block->block_num/2, xN=Block->block_num%2;
                        if (yN<1)
                        {
                            yN*=2;
                            MB_N=MB_X;
                        }
                        else
                        {
                            yN*=2;
                            yN-=2;
                            MB_N=MB_X+mb_width;
                        }
                        Block->block_num=yN*2+xN;
                    }
                    else
                    {
                        // 0 1 0
                        MB_N=MB_X;
                    }
                }
                else
                {
                    if (xfff)
                    {
                        // 0 0 1
                        Ipp32u yN = Block->block_num/2, xN=Block->block_num%2;
                        if (yN<1)
                        {
                            yN*=8;
                            yN++;
                            MB_N=MB_X;
                        }
                        else
                        {
                            yN*=8;
                            yN-=7;
                            MB_N=MB_X + mb_width;
                        }
                        Block->block_num=(yN/4)*2+xN;
                    }
                    else
                    {
                        // 0 0 0
                        MB_N=MB_X + mb_width;
                    }
                }
            }
        }
        else
        {
            Block->mb_num = -1;//no left neighbours
            return;
        }
        Block->block_num+=16;
        Block->block_num+=1;
        Block->mb_num = MB_N;
    }
    else
    {
        Block->block_num+=16;
        Block->block_num--;
        Block->mb_num = m_CurMBAddr;
    }
    return;
}

inline
void H264SegmentDecoder::GetLeftLocationForCurrentMBChromaMBAFFH2(H264DecoderBlockLocation *Block)
{
    bool curmbfff = !pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = !pGetMBBottomFlag(m_cur_mb.GlobalMacroblockInfo);
    //Ipp32s mb_width = m_pCurrentFrame->macroBlockSize().width;
    Ipp32s MB_X=m_cur_mb.CurrentMacroblockNeighbours.mb_A;
    Ipp32s MB_N;
    Block->block_num-=16;
    //chroma
    if (CHROMA_BLOCK_IS_ON_LEFT_EDGE(Block->block_num,2))
    {
        if (MB_X>=0) //left mb addr vaild?
        {
            Ipp8u xfff=!GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[MB_X]);
            if (curmbfff)
            {
                if (curmbtf)
                {
                    if (xfff)
                    {
                        // 1 1 1
                        MB_N=MB_X;
                    }
                    else
                    {
                        // 1 1 0
                        Ipp32u yN = Block->block_num/2;
                        yN/=2;
                        Block->block_num=yN*2;
                        MB_N=MB_X;
                    }
                }
                else
                {
                    if (xfff)
                    {
                        // 1 0 1
                        MB_N=MB_X+mb_width;
                    }
                    else
                    {
                        // 1 0 0
                        Ipp32u yN = Block->block_num/2;
                        yN+=8;
                        yN/=2;
                        Block->block_num=yN*2;
                        MB_N=MB_X;
                    }
                }
            }
            else
            {
                if (curmbtf)
                {
                    if (xfff)
                    {
                        //0 1 1
                        Ipp32u yN = Block->block_num/2;
                        if (yN<1)
                        {
                            yN*=2;
                            MB_N=MB_X;
                        }
                        else
                        {
                            yN*=2;
                            yN-=2;
                            MB_N=MB_X+mb_width;
                        }
                        Block->block_num=yN*2;
                    }
                    else
                    {
                        // 0 1 0
                        MB_N=MB_X;
                    }
                }
                else
                {
                    if (xfff)
                    {
                        // 0 0 1
                        Ipp32u yN = Block->block_num/2;
                        if (yN<1)
                        {
                            yN*=8;
                            yN++;
                            MB_N=MB_X;
                        }
                        else
                        {
                            yN*=8;
                            yN-=15;
                            MB_N=MB_X + mb_width;
                        }
                        Block->block_num=(yN/4)*2;
                    }
                    else
                    {
                        // 0 0 0
                        MB_N=MB_X + mb_width;
                    }
                }
            }
        }
        else
        {
            Block->mb_num = -1;//no left neighbours
            return;
        }
        Block->block_num+=16;
        Block->mb_num = MB_N;
    }
    else
    {
        Block->block_num+=16;
        Block->block_num--;
        Block->mb_num = m_CurMBAddr;
    }
    return;
}

inline
void H264SegmentDecoder::GetLeftLocationForCurrentMBChromaMBAFFH4(H264DecoderBlockLocation *Block)
{
    bool curmbfff = !pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = !pGetMBBottomFlag(m_cur_mb.GlobalMacroblockInfo);
    //Ipp32s mb_width = m_pCurrentFrame->macroBlockSize().width;
    Ipp32s MB_X=m_cur_mb.CurrentMacroblockNeighbours.mb_A;
    Ipp32s MB_N;
    Block->block_num-=16;
    //chroma
    if (CHROMA_BLOCK_IS_ON_LEFT_EDGE(Block->block_num,2))
    {
        if (MB_X>=0) //left mb addr vaild?
        {
            Ipp8u xfff=!GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[MB_X]);
            if (curmbfff)
            {
                if (curmbtf)
                {
                    if (xfff)
                    {
                        // 1 1 1
                        MB_N=MB_X;
                    }
                    else
                    {
                        // 1 1 0
                        Ipp32u yN = Block->block_num/4;
                        yN/=2;
                        Block->block_num=yN*4;
                        MB_N=MB_X;
                    }
                }
                else
                {
                    if (xfff)
                    {
                        // 1 0 1
                        MB_N=MB_X+mb_width;
                    }
                    else
                    {
                        // 1 0 0
                        Ipp32u yN = Block->block_num/4;
                        yN+=16;
                        yN/=2;
                        Block->block_num=yN*4;
                        MB_N=MB_X;
                    }
                }
            }
            else
            {
                if (curmbtf)
                {
                    if (xfff)
                    {
                        //0 1 1
                        Ipp32u yN = Block->block_num/4;
                        if (yN<1)
                        {
                            yN*=2;
                            MB_N=MB_X;
                        }
                        else
                        {
                            yN*=2;
                            yN-=4;
                            MB_N=MB_X+mb_width;
                        }
                        Block->block_num=yN*4;
                    }
                    else
                    {
                        // 0 1 0
                        MB_N=MB_X;
                    }
                }
                else
                {
                    if (xfff)
                    {
                        // 0 0 1
                        Ipp32u yN = Block->block_num/2;
                        if (yN<1)
                        {
                            yN*=8;
                            yN++;
                            MB_N=MB_X;
                        }
                        else
                        {
                            yN*=8;
                            yN-=15;
                            MB_N=MB_X + mb_width;
                        }
                        Block->block_num=(yN/4)*4;
                    }
                    else
                    {
                        // 0 0 0
                        MB_N=MB_X + mb_width;
                    }
                }
            }
        }
        else
        {
            Block->mb_num = -1;//no left neighbours
            return;
        }
        Block->block_num+=16;
        Block->mb_num = MB_N;
    }
    else
    {
        Block->block_num+=16;
        Block->block_num--;
        Block->mb_num = m_CurMBAddr;
    }
    return;
}

inline
void H264SegmentDecoder::GetTopLocationForCurrentMBChromaMBAFFBMEH(H264DecoderBlockLocation *Block)
{
    bool curmbfff = !pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = !pGetMBBottomFlag(m_cur_mb.GlobalMacroblockInfo);
    //Ipp32s mb_width = m_pCurrentFrame->macroBlockSize().width;
    Ipp32s pair_offset = curmbtf? mb_width:-mb_width;
    Ipp32s MB_X;
    Ipp32s MB_N;
    //chroma
    if (CHROMA_BLOCK_IS_ON_TOP_EDGE(Block->block_num-16,1))
    {
        if (curmbfff && !curmbtf )
        {
            MB_N = m_CurMBAddr + pair_offset;
            Block->block_num+=2;
        }
        else
        {
            MB_X = m_cur_mb.CurrentMacroblockNeighbours.mb_B;
            if (MB_X>=0)
            {
                Ipp8u xfff=!GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[MB_X]);
                if (!curmbfff && curmbtf && !xfff)
                {
                    MB_N=MB_X;
                    Block->block_num+=2;
                }
                else
                {
                    //if (!curmbff && curmbtf && xfff)
                    //    Block->block_num+=0;
                    //else
                    Block->block_num+=2;
                    MB_N=MB_X + mb_width;
                }
            }
            else
            {
                Block->mb_num = -1;
                return;
            }
        }
        Block->mb_num = MB_N;
        return;
    }
    else
    {
        Block->block_num-=2;
        Block->mb_num = m_CurMBAddr;
        return;
    }

}

inline
void H264SegmentDecoder::GetTopLocationForCurrentMBChromaMBAFFH2(H264DecoderBlockLocation *Block)
{
    bool curmbfff = !pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = !pGetMBBottomFlag(m_cur_mb.GlobalMacroblockInfo);
    //Ipp32s mb_width = m_pCurrentFrame->macroBlockSize().width;
    Ipp32s pair_offset = curmbtf? mb_width:-mb_width;
    Ipp32s MB_X;
    Ipp32s MB_N;
    //chroma
    if (CHROMA_BLOCK_IS_ON_TOP_EDGE(Block->block_num-16,2))
    {
        if (curmbfff && !curmbtf )
        {
            MB_N = m_CurMBAddr + pair_offset;
            Block->block_num+=6;
        }
        else
        {
            MB_X = m_cur_mb.CurrentMacroblockNeighbours.mb_B;
            if (MB_X>=0)
            {
                Ipp8u xfff=!GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[MB_X]);
                if (!curmbfff && curmbtf && !xfff)
                {
                    MB_N=MB_X;
                    Block->block_num+=6;
                }
                else
                {
                    //if (!curmbff && curmbtf && xfff)
                    //    Block->block_num+=0;
                    //else
                    Block->block_num+=6;
                    MB_N=MB_X + mb_width;
                }
            }
            else
            {
                Block->mb_num = -1;
                return;
            }
        }
        Block->mb_num = MB_N;
        return;
    }
    else
    {
        Block->block_num-=2;
        Block->mb_num = m_CurMBAddr;
        return;
    }

}

inline
void H264SegmentDecoder::GetTopLocationForCurrentMBChromaMBAFFH4(H264DecoderBlockLocation *Block)
{
    bool curmbfff = !pGetMBFieldDecodingFlag(m_cur_mb.GlobalMacroblockInfo);
    bool curmbtf  = !pGetMBBottomFlag(m_cur_mb.GlobalMacroblockInfo);
    //Ipp32s mb_width = m_pCurrentFrame->macroBlockSize().width;
    Ipp32s pair_offset = curmbtf? mb_width:-mb_width;
    Ipp32s MB_X;
    Ipp32s MB_N;
    //chroma
    if (CHROMA_BLOCK_IS_ON_TOP_EDGE(Block->block_num-16,3))
    {
        if (curmbfff && !curmbtf )
        {
            MB_N = m_CurMBAddr + pair_offset;
            Block->block_num+=12;
        }
        else
        {
            MB_X = m_cur_mb.CurrentMacroblockNeighbours.mb_B;
            if (MB_X>=0)
            {
                Ipp8u xfff=!GetMBFieldDecodingFlag(m_pCurrentFrame->m_mbinfo.mbs[MB_X]);
                if (!curmbfff && curmbtf && !xfff)
                {
                    MB_N=MB_X;
                    Block->block_num+=12;
                }
                else
                {
                    //if (!curmbff && curmbtf && xfff)
                    //    Block->block_num+=0;
                    //else
                    Block->block_num+=12;
                    MB_N=MB_X + mb_width;
                }
            }
            else
            {
                Block->mb_num = -1;
                return;
            }
        }
        Block->mb_num = MB_N;
        return;
    }
    else
    {
        Block->block_num-=4;
        Block->mb_num = m_CurMBAddr;
        return;
    }

}
#endif

void H264SegmentDecoder::UpdateNeighbouringBlocksBMEH(Ipp8u DeblockCalls)
{
    H264DecoderBlockNeighboursInfo * pN = &m_cur_mb.CurrentBlockNeighbours;

    pN->mb_above.block_num=0;
    pN->mb_above_chroma[0].block_num=16;
    pN->mb_above_left.block_num=0;
    pN->mb_above_right.block_num=3;
    pN->mbs_left[0].block_num=0;
    pN->mbs_left[1].block_num=4;
    pN->mbs_left[2].block_num=8;
    pN->mbs_left[3].block_num=12;
    pN->mbs_left_chroma[0][0].block_num=16;
    pN->mbs_left_chroma[0][1].block_num=16+2;

    {
        GetLeftLocationForCurrentMBLumaNonMBAFF(&pN->mbs_left[0]);
        GetLeftLocationForCurrentMBLumaNonMBAFF(&pN->mbs_left[1]);
        GetLeftLocationForCurrentMBLumaNonMBAFF(&pN->mbs_left[2]);
        GetLeftLocationForCurrentMBLumaNonMBAFF(&pN->mbs_left[3]);
        GetTopLocationForCurrentMBLumaNonMBAFF(&pN->mb_above);
        GetTopRightLocationForCurrentMBLumaNonMBAFF(&pN->mb_above_right);
        GetTopLeftLocationForCurrentMBLumaNonMBAFF(&pN->mb_above_left);
        GetTopLocationForCurrentMBChromaNonMBAFFBMEH(&pN->mb_above_chroma[0]);
        GetLeftLocationForCurrentMBChromaNonMBAFFBMEH(&pN->mbs_left_chroma[0][0]);
        GetLeftLocationForCurrentMBChromaNonMBAFFBMEH(&pN->mbs_left_chroma[0][1]);
    }
    pN->mbs_left_chroma[1][0]=pN->mbs_left_chroma[0][0];
    pN->mbs_left_chroma[1][1]=pN->mbs_left_chroma[0][1];
    pN->mb_above_chroma[1] = pN->mb_above_chroma[0];
    pN->mbs_left_chroma[1][0].block_num+=4;
    pN->mbs_left_chroma[1][1].block_num+=4;
    pN->mb_above_chroma[1].block_num+=4;
} // void H264SegmentDecoder::UpdateNeighbouringBlocks(Ipp8u DeblockCalls)

void H264SegmentDecoder::UpdateNeighbouringBlocksH2(Ipp8u DeblockCalls)
{
    H264DecoderBlockNeighboursInfo * pN = &m_cur_mb.CurrentBlockNeighbours;

    pN->mb_above.block_num=0;
    pN->mb_above_chroma[0].block_num=16;
    pN->mb_above_left.block_num=0;
    pN->mb_above_right.block_num=3;
    pN->mbs_left[0].block_num=0;
    pN->mbs_left[1].block_num=4;
    pN->mbs_left[2].block_num=8;
    pN->mbs_left[3].block_num=12;
    pN->mbs_left_chroma[0][0].block_num=16;
    pN->mbs_left_chroma[0][1].block_num=16+2;
    pN->mbs_left_chroma[0][2].block_num=16+4;
    pN->mbs_left_chroma[0][3].block_num=16+6;

    {
        GetLeftLocationForCurrentMBLumaNonMBAFF(&pN->mbs_left[0]);
        GetLeftLocationForCurrentMBLumaNonMBAFF(&pN->mbs_left[1]);
        GetLeftLocationForCurrentMBLumaNonMBAFF(&pN->mbs_left[2]);
        GetLeftLocationForCurrentMBLumaNonMBAFF(&pN->mbs_left[3]);
        GetTopLocationForCurrentMBLumaNonMBAFF(&pN->mb_above);
        GetTopRightLocationForCurrentMBLumaNonMBAFF(&pN->mb_above_right);
        GetTopLeftLocationForCurrentMBLumaNonMBAFF(&pN->mb_above_left);
        GetTopLocationForCurrentMBChromaNonMBAFFH2(&pN->mb_above_chroma[0]);
        GetLeftLocationForCurrentMBChromaNonMBAFFH2(&pN->mbs_left_chroma[0][0]);
        GetLeftLocationForCurrentMBChromaNonMBAFFH2(&pN->mbs_left_chroma[0][1]);
        GetLeftLocationForCurrentMBChromaNonMBAFFH2(&pN->mbs_left_chroma[0][2]);
        GetLeftLocationForCurrentMBChromaNonMBAFFH2(&pN->mbs_left_chroma[0][3]);
    }
    pN->mbs_left_chroma[1][0]=pN->mbs_left_chroma[0][0];
    pN->mbs_left_chroma[1][1]=pN->mbs_left_chroma[0][1];
    pN->mbs_left_chroma[1][2]=pN->mbs_left_chroma[0][2];
    pN->mbs_left_chroma[1][3]=pN->mbs_left_chroma[0][3];
    pN->mb_above_chroma[1] = pN->mb_above_chroma[0];
    pN->mbs_left_chroma[1][0].block_num+=8;
    pN->mbs_left_chroma[1][1].block_num+=8;
    pN->mbs_left_chroma[1][2].block_num+=8;
    pN->mbs_left_chroma[1][3].block_num+=8;
    pN->mb_above_chroma[1].block_num+=8;
} // void H264SegmentDecoder::UpdateNeighbouringBlocks(Ipp8u DeblockCalls)

void H264SegmentDecoder::UpdateNeighbouringBlocksH4(Ipp8u DeblockCalls)
{
    H264DecoderBlockNeighboursInfo * pN = &m_cur_mb.CurrentBlockNeighbours;

    pN->mb_above.block_num=0;
    pN->mb_above_chroma[0].block_num=16;
    pN->mb_above_left.block_num=0;
    pN->mb_above_right.block_num=3;
    pN->mbs_left[0].block_num=0;
    pN->mbs_left[1].block_num=4;
    pN->mbs_left[2].block_num=8;
    pN->mbs_left[3].block_num=12;
    pN->mbs_left_chroma[0][0].block_num=16;
    pN->mbs_left_chroma[0][1].block_num=16+4;
    pN->mbs_left_chroma[0][2].block_num=16+8;
    pN->mbs_left_chroma[0][3].block_num=16+12;

    {
        GetLeftLocationForCurrentMBLumaNonMBAFF(&pN->mbs_left[0]);
        GetLeftLocationForCurrentMBLumaNonMBAFF(&pN->mbs_left[1]);
        GetLeftLocationForCurrentMBLumaNonMBAFF(&pN->mbs_left[2]);
        GetLeftLocationForCurrentMBLumaNonMBAFF(&pN->mbs_left[3]);
        GetTopLocationForCurrentMBLumaNonMBAFF(&pN->mb_above);
        GetTopRightLocationForCurrentMBLumaNonMBAFF(&pN->mb_above_right);
        GetTopLeftLocationForCurrentMBLumaNonMBAFF(&pN->mb_above_left);
        GetTopLocationForCurrentMBChromaNonMBAFFH4(&pN->mb_above_chroma[0]);
        GetLeftLocationForCurrentMBChromaNonMBAFFH4(&pN->mbs_left_chroma[0][0]);
        GetLeftLocationForCurrentMBChromaNonMBAFFH4(&pN->mbs_left_chroma[0][1]);
        GetLeftLocationForCurrentMBChromaNonMBAFFH4(&pN->mbs_left_chroma[0][2]);
        GetLeftLocationForCurrentMBChromaNonMBAFFH4(&pN->mbs_left_chroma[0][3]);
    }
    pN->mbs_left_chroma[1][0]=pN->mbs_left_chroma[0][0];
    pN->mbs_left_chroma[1][1]=pN->mbs_left_chroma[0][1];
    pN->mbs_left_chroma[1][2]=pN->mbs_left_chroma[0][2];
    pN->mbs_left_chroma[1][3]=pN->mbs_left_chroma[0][3];
    pN->mb_above_chroma[1] = pN->mb_above_chroma[0];
    pN->mbs_left_chroma[1][0].block_num+=16;
    pN->mbs_left_chroma[1][1].block_num+=16;
    pN->mbs_left_chroma[1][2].block_num+=16;
    pN->mbs_left_chroma[1][3].block_num+=16;
    pN->mb_above_chroma[1].block_num+=16;
} // void H264SegmentDecoder::UpdateNeighbouringBlocks(Ipp8u DeblockCalls)

inline
void H264SegmentDecoder::UpdateCurrentMBInfo()
{
    m_cur_mb.GlobalMacroblockInfo = &m_pCurrentFrame->m_mbinfo.mbs[m_CurMBAddr];
    m_cur_mb.LocalMacroblockInfo = &m_mbinfo.mbs[m_CurMBAddr];
    m_cur_mb.MacroblockCoeffsInfo = &m_mbinfo.MacroblockCoeffsInfo[m_CurMBAddr];
    m_cur_mb.MVs[0] = &m_pCurrentFrame->m_mbinfo.MV[0][m_CurMBAddr];
    m_cur_mb.MVs[1] = &m_pCurrentFrame->m_mbinfo.MV[1][m_CurMBAddr];
    m_cur_mb.MVs[2] = &m_mbinfo.MVDeltas[0][m_CurMBAddr];
    m_cur_mb.MVs[3] = &m_mbinfo.MVDeltas[1][m_CurMBAddr];
    m_cur_mb.MVFlags[0] = &m_mbinfo.MVFlags[0][m_CurMBAddr];
    m_cur_mb.MVFlags[1] = &m_mbinfo.MVFlags[1][m_CurMBAddr];
    m_cur_mb.RefIdxs[0] = &m_pCurrentFrame->m_mbinfo.RefIdxs[0][m_CurMBAddr];
    m_cur_mb.RefIdxs[1] = &m_pCurrentFrame->m_mbinfo.RefIdxs[1][m_CurMBAddr];
    //***bniebnie
	if(!pGetMBBottomFlag(m_cur_mb.GlobalMacroblockInfo))
    {
            m_PairMBAddr = m_CurMBAddr;
    }
    else
    {
        m_PairMBAddr = m_CurMBAddr-mb_width;
    }
    m_cur_mb.GlobalMacroblockPairInfo = &m_pCurrentFrame->m_mbinfo.mbs[m_PairMBAddr];
    m_cur_mb.LocalMacroblockPairInfo = &m_mbinfo.mbs[m_PairMBAddr];

} // void H264SegmentDecoder::UpdateCurrentMBInfo()

inline
void H264SegmentDecoder::UpdateNeighbouringAddresses(Ipp8u IgnoreSliceEdges)
{
    H264DecoderMacroblockNeighboursInfo * pN = &m_cur_mb.CurrentMacroblockNeighbours;

    //Ipp32s  mb_width = m_pCurrentFrame->macroBlockSize().width;
    if(!pGetMBBottomFlag(m_cur_mb.GlobalMacroblockInfo))//update only if top mb recieved
    {
        Ipp32s real_mb_width = mb_width;//***bnie: *(m_pSliceHeader->MbaffFrameFlag+1);
        pN->mb_A = m_CurMB_X>0?m_CurMBAddr-1:-1;
        pN->mb_B = m_CurMB_Y>0?m_CurMBAddr-real_mb_width:-1;
        pN->mb_C = (m_CurMB_Y > 0 && m_CurMB_X < ((signed)mb_width) - 1) ? (m_CurMBAddr - real_mb_width + 1) : (-1);
        pN->mb_D = m_CurMB_Y>0&&m_CurMB_X>0?m_CurMBAddr-real_mb_width-1:-1;
        if (m_bNeedToCheckMBSliceEdges && !IgnoreSliceEdges)
        {
            if (pN->mb_A>=0 &&
                (m_cur_mb.GlobalMacroblockInfo->slice_id != m_pCurrentFrame->m_mbinfo.mbs[pN->mb_A].slice_id))
                pN->mb_A = -1;
            if (pN->mb_B>=0 &&
                (m_cur_mb.GlobalMacroblockInfo->slice_id != m_pCurrentFrame->m_mbinfo.mbs[pN->mb_B].slice_id))
                pN->mb_B = -1;
            if (pN->mb_C>=0 &&
                (m_cur_mb.GlobalMacroblockInfo->slice_id != m_pCurrentFrame->m_mbinfo.mbs[pN->mb_C].slice_id))
                pN->mb_C = -1;
            if (pN->mb_D>=0 && (m_cur_mb.GlobalMacroblockInfo->slice_id != m_pCurrentFrame->m_mbinfo.mbs[pN->mb_D].slice_id))
                pN->mb_D = -1;
        }
        else if(m_bNeedToCheckMBSliceEdges && IgnoreSliceEdges)
        {
            if (pN->mb_A>=0 &&
                (m_cur_mb.GlobalMacroblockInfo->slice_id == -1))
                pN->mb_A = -1;
            if (pN->mb_B>=0 &&
                (m_cur_mb.GlobalMacroblockInfo->slice_id == -1))
                pN->mb_B = -1;
            if (pN->mb_C>=0 &&
                (m_cur_mb.GlobalMacroblockInfo->slice_id == -1))
                pN->mb_C = -1;
            if (pN->mb_D>=0 &&
                (m_cur_mb.GlobalMacroblockInfo->slice_id == -1))
                pN->mb_D = -1;
        }

    }
    m_cur_mb.GlobalMacroblockPairInfo = &m_pCurrentFrame->m_mbinfo.mbs[m_PairMBAddr];
    m_cur_mb.LocalMacroblockPairInfo = &m_mbinfo.mbs[m_PairMBAddr];

} // void H264SegmentDecoder::UpdateNeighbouringAddresses(Ipp8u IgnoreSliceEdges)


//***kinoma enhancement   --bnie  8/2/2008
#define CHECK_PACKET_LOSS_IN_DEFENSIVE_MODE		\
	if( this->GetPacketLossDefensiveMode())		\
	{											\
		if( m_pBitStream->ReachBoundary())		\
		{										\
			status = UMC_END_OF_STREAM;			\
			goto packet_loss_happened;			\
		}										\
	}											\

#define CHECK_PACKET_LOSS_IN_NORMAL_MODE		\
	if( m_pBitStream->ReachBoundary())			\
	{											\
		this->SetPacketLossDefensiveMode(1);	\
		status = UMC_END_OF_STREAM;				\
		goto packet_loss_happened;				\
	}											\

} // namespace UMC

#endif /* __UMC_H264_SEGMENT_DECODER_H */
