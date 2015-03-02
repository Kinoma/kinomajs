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

#include "umc_h264_segment_decoder.h"
#include "umc_h264_dec_deblocking.h"
#include "umc_h264_dec_slice_store.h"
#include "vm_thread.h"
#include "vm_debug.h"

// WWD-200711
#include "kinoma_avc_defines.h"
#include "kinoma_utilities.h"

#ifdef _KINOMA_LOSSY_OPT_

/*
    This version do not support FMO actually, please see for loop in function DeblockSegment.
	That means all MB are continued in de-blocking filtering. So,
	  1, Offset caculation can be simplied!
	  2, 


	These are two interface functions for de-blocking filter:  DeblockFrame and DeblockSegment
	   The first one do not support FILED mode, and second is more efficient for it combine loop in swicth
       So, do not use first function: DeblockFrame
       We have define "DROP_MBAFF" to drop filed de-blocking codes in second function


*/
#endif

namespace UMC
{

// initialize const array
H264SegmentDecoder::ChromaDeblockingFunction H264SegmentDecoder::DeblockChroma[4];

void H264SegmentDecoder::DeblockFrame(Ipp32u uFirstMB, Ipp32u uNumMBs)
{
    Ipp32u i;

#ifdef _KINOMA_LOSSY_OPT_
// WWD-200711
	//m_approx = Current_Drop_Setting;

	if( (m_approx& AVC_DEBLOCKING_DROP_ALL) == AVC_DEBLOCKING_DROP_ALL)
		return;
#endif

    m_bFrameDeblocking = true;

    for (i = uFirstMB; i < uFirstMB + uNumMBs; i++)
        DeblockMacroblockMSlice(i);

} // void H264SegmentDecoder::DeblockFrame(Ipp32u uFirstMB, Ipp32u uNumMBs)

void H264SegmentDecoder::DeblockSegment(Ipp32u uFirstMB, Ipp32u uNumMBs)
{
    Ipp32u i;

#ifdef _KINOMA_LOSSY_OPT_
// WWD-200711
	if( (m_approx& AVC_DEBLOCKING_DROP_ALL) == AVC_DEBLOCKING_DROP_ALL)
		return;

	m_pSliceHeader_deblk = m_pSliceHeader;

	// Reset some slice based de-blocking parameters (Reset only once for each slice and used for all MB in the slice)
	{
		Ipp32u	pic_pitch =  m_pCurrentFrame->pitch();
		int MBYAdjust = 0;

		// load planes
		mParams.pY = m_pCurrentFrame->m_pYPlane;
		mParams.pU = m_pCurrentFrame->m_pUPlane;
		mParams.pV = m_pCurrentFrame->m_pVPlane;

	    mParams.nCurrMB_X = ((uFirstMB) % mb_width);
		mParams.nCurrMB_Y = ((uFirstMB) / mb_width)- MBYAdjust;

		mParams.pitch = pic_pitch;
		mParams.nMaxMVector  = 4;//***bnie: (FRM_STRUCTURE > m_pCurrentFrame->m_PictureStructureForDec) ? (2) : (4);
		//***bnie: mParams.MBFieldCoded = 0;//***bnie: (FRM_STRUCTURE > m_pCurrentFrame->m_PictureStructureForDec);

    // set slice's variables
    mParams.nAlphaC0Offset = m_pSliceHeader->slice_alpha_c0_offset;
    mParams.nBetaOffset = m_pSliceHeader->slice_beta_offset;
	}
#endif

    // no filtering edges of this slice
    if (DEBLOCK_FILTER_OFF == m_pSliceHeader->disable_deblocking_filter_idc)
        return;

    // set frame deblocking flag
    m_bFrameDeblocking = false;

    {
        // select optimized deblocking function
        switch (m_pSliceHeader->slice_type)
        {
        case INTRASLICE:
            for (i = uFirstMB; i < uFirstMB + uNumMBs; i++)
                DeblockMacroblockISlice(i);
            break;

        case PREDSLICE:
            for (i = uFirstMB; i < uFirstMB + uNumMBs; i++)
                DeblockMacroblockPSlice(i);
            break;

        case BPREDSLICE:
            for (i = uFirstMB; i < uFirstMB + uNumMBs; i++)
                DeblockMacroblockBSlice(i);
            break;

        default:
            VM_ASSERT(false);
            break;
        }
    }

} // void H264SegmentDecoder::DeblockSegment(Ipp32u uFirstMB, Ipp32u uNumMBs)

void H264SegmentDecoder::DeblockMacroblockMSlice(Ipp32u MBAddr)
{
    H264SliceHeader *pHeader = m_pSliceStore->GetSliceHeader(m_pCurrentFrame->m_mbinfo.mbs[MBAddr].slice_id);

    // when deblocking isn't required
    if ((NULL == pHeader) ||
        (DEBLOCK_FILTER_OFF == pHeader->disable_deblocking_filter_idc))
        return;

#ifdef _KINOMA_LOSSY_OPT_
	m_pSliceHeader_deblk =   pHeader;

	// Reset some slice based de-blocking parameters (Reset only once for each slice and used for all MB in the slice)
	{
		Ipp32u	pic_pitch =  m_pCurrentFrame->pitch();
		int MBYAdjust = 0;

		// load planes
		mParams.pY = m_pCurrentFrame->m_pYPlane;
		mParams.pU = m_pCurrentFrame->m_pUPlane;
		mParams.pV = m_pCurrentFrame->m_pVPlane;

	    mParams.nCurrMB_X = ((MBAddr) % mb_width);
		mParams.nCurrMB_Y = ((MBAddr) / mb_width)- MBYAdjust;

		mParams.pitch = pic_pitch;
		mParams.nMaxMVector  = 4;//***bnie: (FRM_STRUCTURE > m_pCurrentFrame->m_PictureStructureForDec) ? (2) : (4);
		//***bnie: mParams.MBFieldCoded = 0;//(FRM_STRUCTURE > m_pCurrentFrame->m_PictureStructureForDec);

    // set slice's variables
		mParams.nAlphaC0Offset = m_pSliceHeader->slice_alpha_c0_offset;
		mParams.nBetaOffset = m_pSliceHeader->slice_beta_offset;
	}
#endif

    // select optimized deblocking function
    switch (pHeader->slice_type)
    {
    case INTRASLICE:
        DeblockMacroblockISlice(MBAddr);
        break;

    case PREDSLICE:
        DeblockMacroblockPSlice(MBAddr);
        break;

    case BPREDSLICE:
        DeblockMacroblockBSlice(MBAddr);
        break;

    default:
        // illegal case. it should never hapen.
        VM_ASSERT(false);
        break;
    }

} // void H264SegmentDecoder::DeblockMacroblockMSlice(Ipp32u MBAddr)

#ifndef _KINOMA_LOSSY_OPT_
// These functions are used as one backup which required by Bryan ONLY.
void H264SegmentDecoder::DeblockMacroblockISlice(Ipp32u MBAddr)
{
    __align(16)
    DeblockingParameters params;

	Profile_Start(ippiFilterDeblockingLuma);

#ifdef SPEEDUP_HACK
	if( (m_approx&2) )
		return;
#endif

    // prepare deblocking parameters
    params.nMBAddr = MBAddr;
    ResetDeblockingVariables(&params);
    PrepareDeblockingParametersISlice(&params);

	Profile_End(ippiFilterDeblockingLuma);

#ifdef SPEEDUP_HACK
	if( (m_approx&1)== 0 )
#endif
    {
        Ipp32u color_format = m_pSeqParamSet->chroma_format_idc;

        (this->*DeblockChroma[color_format])(VERTICAL_DEBLOCKING, &params);
        (this->*DeblockChroma[color_format])(HORIZONTAL_DEBLOCKING, &params);
    }

	// perform deblocking
	DeblockLuma(VERTICAL_DEBLOCKING, &params);
	DeblockLuma(HORIZONTAL_DEBLOCKING, &params);

} // void H264SegmentDecoder::DeblockMacroblockISlice(Ipp32u MBAddr)

void H264SegmentDecoder::DeblockMacroblockPSlice(Ipp32u MBAddr)
{
    __align(16)
    DeblockingParameters params;

	Profile_Start(ippiFilterDeblockingLuma);

#ifdef SPEEDUP_HACK
	if( (m_approx&2) )
		return;
#endif

    // prepare deblocking parameters
    params.nMBAddr = MBAddr;
    ResetDeblockingVariables(&params);
    PrepareDeblockingParametersPSlice(&params);

	Profile_End(ippiFilterDeblockingLuma);

#ifdef SPEEDUP_HACK
	if( (m_approx&1)== 0 )
#endif
	{
        Ipp32u color_format = m_pSeqParamSet->chroma_format_idc;

        (this->*DeblockChroma[color_format])(VERTICAL_DEBLOCKING, &params);
        (this->*DeblockChroma[color_format])(HORIZONTAL_DEBLOCKING, &params);
    }

    // perform deblocking
    DeblockLuma(VERTICAL_DEBLOCKING, &params);
    DeblockLuma(HORIZONTAL_DEBLOCKING, &params);

} // void H264SegmentDecoder::DeblockMacroblockPSlice(Ipp32u MBAddr)

void H264SegmentDecoder::DeblockMacroblockBSlice(Ipp32u MBAddr)
{
    __align(16)
    DeblockingParameters params;

	Profile_Start(ippiFilterDeblockingLuma);

#ifdef SPEEDUP_HACK
	if( (m_approx&2) )
		return;
#endif

    // prepare deblocking parameters
    params.nMBAddr = MBAddr;
    ResetDeblockingVariables(&params);
    PrepareDeblockingParametersBSlice(&params);

	Profile_End(ippiFilterDeblockingLuma);

#ifdef SPEEDUP_HACK
	if( (m_approx&1)== 0 )
#endif
	{
        Ipp32u color_format = m_pSeqParamSet->chroma_format_idc;

        (this->*DeblockChroma[color_format])(VERTICAL_DEBLOCKING, &params);
        (this->*DeblockChroma[color_format])(HORIZONTAL_DEBLOCKING, &params);
    }

    // perform deblocking
    DeblockLuma(VERTICAL_DEBLOCKING, &params);
    DeblockLuma(HORIZONTAL_DEBLOCKING, &params);

} // void H264SegmentDecoder::DeblockMacroblockBSlice(Ipp32u MBAddr)

#else
void H264SegmentDecoder::DeblockMacroblockISlice(Ipp32u MBAddr)
{
    // prepare deblocking parameters
    mParams.nMBAddr = MBAddr;
    ResetDeblockingVariablesKinoma(&mParams);

    // prepare deblocking parameters
	PrepareDeblockingParametersISlice(&mParams);

	// perform deblocking
	DeblockMacroblockChroma(&mParams);
	DeblockLuma_noAFF(&mParams);

	// Reset for next MB
    ResetDeblockingVariablesKinoma2(&mParams);
} 
void H264SegmentDecoder::DeblockMacroblockPSlice(Ipp32u MBAddr)
{
    Ipp32u mbtype = (m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->mbtype;

     // prepare deblocking parameters
    mParams.nMBAddr = MBAddr;
    ResetDeblockingVariablesKinoma(&mParams);

    // prepare deblocking parameters
    // when this macroblock is intra coded
	if((m_approx & AVC_DEBLOCKING_SIMPLYBS) == AVC_DEBLOCKING_SIMPLYBS)
	{
	    if (IS_INTRA_MBTYPE(mbtype))
		{
			PrepareDeblockingParametersISlice(&mParams);
		}
		else
			PrepareDeblockingParametersKinoma(&mParams);
	}
	else
	{
	    if (IS_INTRA_MBTYPE(mbtype))
	    {
		    PrepareDeblockingParametersISlice(&mParams);
		}
		else
    PrepareDeblockingParametersPSlice(&mParams);
	}

    // perform deblocking
	DeblockMacroblockChroma(&mParams);
    DeblockLuma_noAFF(&mParams);

	// Reset for next MB
    ResetDeblockingVariablesKinoma2(&mParams);
}
void H264SegmentDecoder::DeblockMacroblockBSlice(Ipp32u MBAddr)
{
    Ipp32u mbtype = (m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->mbtype;

    // prepare deblocking parameters
    mParams.nMBAddr = MBAddr;
    ResetDeblockingVariablesKinoma(&mParams);

    // prepare deblocking parameters
    // when this macroblock is intra coded
	if((m_approx & AVC_DEBLOCKING_SIMPLYBS) == AVC_DEBLOCKING_SIMPLYBS)
	{
	    if (IS_INTRA_MBTYPE(mbtype))
		{
			PrepareDeblockingParametersISlice(&mParams);
		}
		else
			PrepareDeblockingParametersKinoma(&mParams);
	}
	else
	{
	    if (IS_INTRA_MBTYPE(mbtype))
		{
			PrepareDeblockingParametersISlice(&mParams);
		}
		else
    PrepareDeblockingParametersBSlice(&mParams);
	}


    // perform deblocking
	DeblockMacroblockChroma(&mParams);
    DeblockLuma_noAFF(&mParams);

	// Reset for next MB
    ResetDeblockingVariablesKinoma2(&mParams);
}
void H264SegmentDecoder::DeblockLuma_noAFF(DeblockingParameters *pParams)
{
    Ipp8u *pY = pParams->pLuma;
    Ipp32s pic_pitch = pParams->pitch;
    Ipp32u MBAddr = pParams->nMBAddr;
    Ipp8u Clipping[16];
    Ipp8u Alpha[2];
    Ipp8u Beta[2];
	Ipp32s AlphaC0Offset = m_pSliceHeader_deblk->slice_alpha_c0_offset;	//pParams->nAlphaC0Offset;
	Ipp32s BetaOffset = m_pSliceHeader_deblk->slice_beta_offset;		//pParams->nBetaOffset;
    Ipp32s pmq_QP = m_mbinfo.mbs[MBAddr].QP;

	Ipp8u    Alpha_i,Beta_i, *pClipTab_i;
		//***
	int	const_51 = 51;

    int    index = pmq_QP + BetaOffset;
		
	Profile_Start(ippiFilterDeblockingLuma);
		
	Clip1(const_51, index);
		
	Beta_i = BETA_TABLE[index];

    index = pmq_QP + AlphaC0Offset;
	Clip1(const_51, index);
    Alpha_i = ALPHA_TABLE[index];
    pClipTab_i = CLIP_TAB[index];


    //dir
    // luma deblocking
    //
    if (pParams->DeblockingFlag[0])
    {
        Ipp8u *pClipTab;
        Ipp32s QP;
        Ipp8u *pStrength = pParams->Strength[0];

        if (pParams->ExternalEdgeFlag[0])
        {
            Ipp32s pmp_QP;

            // get neighbour block QP
            pmp_QP = m_mbinfo.mbs[pParams->nNeighbour[0]].QP;

            // luma variables
            QP = (pmp_QP + pmq_QP + 1) >> 1 ;

            // external edge variables
			//***index = IClip(0, 51, QP + BetaOffset);
			index = QP + BetaOffset; Clip1(const_51, index);
            Beta[0] = BETA_TABLE[index];

			//***index = IClip(0, 51, QP + AlphaC0Offset);
			index = QP + AlphaC0Offset; Clip1(const_51, index);
            Alpha[0] = ALPHA_TABLE[index];
            pClipTab = CLIP_TAB[index];


            // create clipping values
            Clipping[0] = pClipTab[pStrength[0]];
            Clipping[1] = pClipTab[pStrength[1]];
            Clipping[2] = pClipTab[pStrength[2]];
            Clipping[3] = pClipTab[pStrength[3]];

        }

        // internal edge variables
        {
            Ipp32u edge;

			Beta[1] = Beta_i; //BETA_TABLE[index];
			Alpha[1] = Alpha_i;		//ALPHA_TABLE[index];
			pClipTab = pClipTab_i;	//CLIP_TAB[index];

            for (edge = 4;edge < 16;edge += 4)
            {
                if (*((Ipp32u *) (pStrength + edge)))
                {
                    // create clipping values
                    Clipping[edge + 0] = pClipTab[pStrength[edge + 0]];
                    Clipping[edge + 1] = pClipTab[pStrength[edge + 1]];
                    Clipping[edge + 2] = pClipTab[pStrength[edge + 2]];
                    Clipping[edge + 3] = pClipTab[pStrength[edge + 3]];
                }
            }
        }

        // perform deblocking
        IppLumaDeblocking[0](pY,pic_pitch, Alpha, Beta,  Clipping, pStrength);
    }

    if (pParams->DeblockingFlag[1])
    {
        Ipp8u *pClipTab;
        Ipp32s QP;
        Ipp8u *pStrength = pParams->Strength[1];

        if (pParams->ExternalEdgeFlag[1])
        {
            Ipp32s pmp_QP;

            // get neighbour block QP
            pmp_QP = m_mbinfo.mbs[pParams->nNeighbour[1]].QP;

            // luma variables
            QP = (pmp_QP + pmq_QP + 1) >> 1 ;

            // external edge variables
			//***index = IClip(0, 51, QP + BetaOffset);
			index = QP + BetaOffset; Clip1(const_51, index);
            Beta[0] = BETA_TABLE[index];

			//***index = IClip(0, 51, QP + AlphaC0Offset);
			index = QP + AlphaC0Offset; Clip1(const_51, index);
            Alpha[0] = ALPHA_TABLE[index];
            pClipTab = CLIP_TAB[index];

            // create clipping values
            Clipping[0] = pClipTab[pStrength[0]];
            Clipping[1] = pClipTab[pStrength[1]];
            Clipping[2] = pClipTab[pStrength[2]];
            Clipping[3] = pClipTab[pStrength[3]];

        }

        // internal edge variables
        {
            Ipp32u edge;

			Beta[1] = Beta_i; //BETA_TABLE[index];
			Alpha[1] = Alpha_i;		//ALPHA_TABLE[index];
			pClipTab = pClipTab_i;	//CLIP_TAB[index];

            for (edge = 4;edge < 16;edge += 4)
            {
                if (*((Ipp32u *) (pStrength + edge)))
                {
                    // create clipping values
                    Clipping[edge + 0] = pClipTab[pStrength[edge + 0]];
                    Clipping[edge + 1] = pClipTab[pStrength[edge + 1]];
                    Clipping[edge + 2] = pClipTab[pStrength[edge + 2]];
                    Clipping[edge + 3] = pClipTab[pStrength[edge + 3]];
                }
            }
        }

        // perform deblocking
        IppLumaDeblocking[1](pY,pic_pitch,Alpha,Beta,Clipping,  pStrength);
    }

	Profile_End(ippiFilterDeblockingLuma);
} 
// WWD: when strength is one constant number in some condition
#define SetEdgeStrengthKinoma(edge, strength)    *((Ipp32u *) (edge)) = strength;

void H264SegmentDecoder::ResetDeblockingVariablesKinoma(DeblockingParameters *pParams)
{
    Ipp32u offset;
    Ipp32u mbXOffset, mbYOffset;
    Ipp32s pic_pitch = pParams->pitch;
    Ipp32u MBAddr = pParams->nMBAddr;
    Ipp32s nCurrMB_X = pParams->nCurrMB_X, nCurrMB_Y = pParams->nCurrMB_Y;		// signed int

    mbXOffset = nCurrMB_X * 16;
    mbYOffset = nCurrMB_Y * 16;

    // calc plane's offsets
    offset = mbXOffset + (mbYOffset * pic_pitch);
    pParams->pLuma = pParams->pY + offset;
	offset >>= 1;
    pParams->pChroma[0] = pParams->pU + offset;
    pParams->pChroma[1] = pParams->pV + offset;

    // set external edge variables
    pParams->ExternalEdgeFlag[VERTICAL_DEBLOCKING] = (nCurrMB_X != 0);
    pParams->ExternalEdgeFlag[HORIZONTAL_DEBLOCKING] = (nCurrMB_Y != 0);

    if (DEBLOCK_FILTER_ON_NO_SLICE_EDGES == m_pSliceHeader_deblk->disable_deblocking_filter_idc)
    {
        // don't filter at slice boundaries
        if (nCurrMB_X)
        {
            if (m_pCurrentFrame->m_mbinfo.mbs[MBAddr].slice_id !=
                m_pCurrentFrame->m_mbinfo.mbs[MBAddr - 1].slice_id)
                pParams->ExternalEdgeFlag[VERTICAL_DEBLOCKING] = 0;
        }

        if (nCurrMB_Y)
        {
            if (m_pCurrentFrame->m_mbinfo.mbs[MBAddr].slice_id !=
                m_pCurrentFrame->m_mbinfo.mbs[MBAddr - mb_width].slice_id)
                pParams->ExternalEdgeFlag[HORIZONTAL_DEBLOCKING] = 0;
        }
    }

    // reset external edges strength
    SetEdgeStrengthKinoma(pParams->Strength[VERTICAL_DEBLOCKING], 0);
    SetEdgeStrengthKinoma(pParams->Strength[HORIZONTAL_DEBLOCKING], 0);

    // set neighbour addreses
    pParams->nNeighbour[VERTICAL_DEBLOCKING] = MBAddr - 1;
    pParams->nNeighbour[HORIZONTAL_DEBLOCKING] = MBAddr - mb_width;

    // set deblocking flag(s)
    pParams->DeblockingFlag[VERTICAL_DEBLOCKING] = 0;
    pParams->DeblockingFlag[HORIZONTAL_DEBLOCKING] = 0;

} // void H264SegmentDecoder::ResetDeblockingVariablesKinoma(DeblockingParameters *pParams)

void H264SegmentDecoder::ResetDeblockingVariablesKinoma2(DeblockingParameters *pParams)
{
    Ipp32s nCurrMB_X = pParams->nCurrMB_X, nCurrMB_Y = pParams->nCurrMB_Y;		// signed int

	// prepare macroblock variables
    nCurrMB_X = nCurrMB_X +1;
	if(nCurrMB_X == mb_width)
	{
	    nCurrMB_Y ++;
		nCurrMB_X = 0;
		pParams->nCurrMB_Y = nCurrMB_Y;
	}

	pParams->nCurrMB_X = nCurrMB_X;
}
void H264SegmentDecoder::PrepareDeblockingParametersKinoma(DeblockingParameters *pParams)
{
 	Ipp32u MBAddr = pParams->nMBAddr;
	Ipp32u cbp4x4_luma = (m_mbinfo.mbs + MBAddr)->cbp4x4_luma; 
	int		nNeighbour;

	if((m_approx & AVC_DEBLOCKING_SIMPLYBS_INTRA) == AVC_DEBLOCKING_SIMPLYBS_INTRA)
	{
		// First, if current MB is one skipped MB.
		if (!(cbp4x4_luma & 0x1fffe))
		{
			pParams->DeblockingFlag[VERTICAL_DEBLOCKING]	= 0;
			pParams->DeblockingFlag[HORIZONTAL_DEBLOCKING]	= 0;

			// Vertiacal
			if (pParams->ExternalEdgeFlag[VERTICAL_DEBLOCKING])
			{
				nNeighbour = pParams->nNeighbour[VERTICAL_DEBLOCKING];
				H264DecoderMacroblockLocalInfo *pNeighbour;
				// select neighbour addres
				pNeighbour = m_mbinfo.mbs + nNeighbour;
				// when neighbour macroblock isn't intra
				if (IS_INTRA_MBTYPE((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->mbtype))
				{
					pParams->DeblockingFlag[VERTICAL_DEBLOCKING] = 1;
					SetEdgeStrengthKinoma(pParams->Strength[VERTICAL_DEBLOCKING], 0x04040404);	

					// MUST settall other internal edge to zero
					SetEdgeStrengthKinoma(pParams->Strength[VERTICAL_DEBLOCKING]+ 4, 0);	
					SetEdgeStrengthKinoma(pParams->Strength[VERTICAL_DEBLOCKING]+ 8, 0);	
					SetEdgeStrengthKinoma(pParams->Strength[VERTICAL_DEBLOCKING]+12, 0);	
				}
			}
			// Hori
			if (pParams->ExternalEdgeFlag[HORIZONTAL_DEBLOCKING])
			{
				nNeighbour = pParams->nNeighbour[HORIZONTAL_DEBLOCKING];
				H264DecoderMacroblockLocalInfo *pNeighbour;
				// select neighbour addres
				pNeighbour = m_mbinfo.mbs + nNeighbour;
				// when neighbour macroblock isn't intra
				if (IS_INTRA_MBTYPE((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->mbtype))
				{
					pParams->DeblockingFlag[HORIZONTAL_DEBLOCKING] = 1;
					SetEdgeStrengthKinoma(pParams->Strength[HORIZONTAL_DEBLOCKING], 0x04040404);			

					// MUST settall other internal edge to zero
					SetEdgeStrengthKinoma(pParams->Strength[HORIZONTAL_DEBLOCKING]+ 4, 0);	
					SetEdgeStrengthKinoma(pParams->Strength[HORIZONTAL_DEBLOCKING]+ 8, 0);	
					SetEdgeStrengthKinoma(pParams->Strength[HORIZONTAL_DEBLOCKING]+12, 0);	
				}
			}
			return; // 
		}
	}
	// Normal when AVC_DEBLOCKING_SIMPLYBS is set
	{
		Ipp8u  *pStrength;
		Ipp32u *pDeblockingFlag;
		int		dir;

		// Reset
		memset(pParams->Strength[0], 0, 16);
		memset(pParams->Strength[1], 0, 16);

		// If it is not skipped MB, Simply caculation for BS (P) use the following method
		for(dir=0; dir<2; dir++)
		{
			pStrength = pParams->Strength[dir];
			pDeblockingFlag = &(pParams->DeblockingFlag[dir]);
		 
			nNeighbour = pParams->nNeighbour[dir];

			// external edge
			if (pParams->ExternalEdgeFlag[dir])
			{
				H264DecoderMacroblockLocalInfo *pNeighbour;

				// select neighbour addres
				pNeighbour = m_mbinfo.mbs + nNeighbour;

				// when neighbour macroblock isn't intra
				if (!IS_INTRA_MBTYPE((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->mbtype))
				{
					int idx;

					// Reset BS for external edge of current MB according to residuals ONLY
					for (idx = 0;idx < 4;idx += 1)
					{
						Ipp32u blkQ, blkP;

						blkQ = EXTERNAL_BLOCK_MASK[dir][CURRENT_BLOCK][idx];
						blkP = EXTERNAL_BLOCK_MASK[dir][NEIGHBOUR_BLOCK][idx];

						// when one of couple of blocks has coeffs
						if ((cbp4x4_luma & blkQ) || (pNeighbour->cbp4x4_luma & blkP))
						{
							pStrength[idx] = 2;
						}
						else
						{
							// Actually, the BS can be "1" when we compare MV
							pStrength[idx] = 0;
						}
					}

					// 
					if(*((Ipp32u *) (pStrength)))
						*pDeblockingFlag = 1;
				}
				// external edge required in strong filtering
				else
				{
					SetEdgeStrength(pStrength + 0, 4);
					*pDeblockingFlag = 1;
				}
	
			} 

		}
			// Internal edge
		{
			if((m_approx & AVC_DEBLOCKING_DROP_INTERNAL) != AVC_DEBLOCKING_DROP_INTERNAL)
			{
				Ipp32u idx;

				// reset all strengths
				//SetEdgeStrengthKinoma(pParams->Strength[0] + 4, 0);
				//SetEdgeStrengthKinoma(pParams->Strength[0] + 8, 0);
				//SetEdgeStrengthKinoma(pParams->Strength[0] +12, 0);

				//SetEdgeStrengthKinoma(pParams->Strength[1] + 4, 0);
				//SetEdgeStrengthKinoma(pParams->Strength[1] + 8, 0);
				//SetEdgeStrengthKinoma(pParams->Strength[1] +12, 0);

				// WWD-200711
				// set deblocking flag
				if (cbp4x4_luma & 0x1fffe)
				{
					Ipp32u blkQ;

					pParams->DeblockingFlag[0] = 1;
					pParams->DeblockingFlag[1] = 1;


					// cicle of edge(s)
					// we do all edges in one cicle
					for (idx = 4;idx < 16;idx += 1)
					{
						blkQ = INTERNAL_BLOCKS_MASK[0][idx - 4];

						if (cbp4x4_luma & blkQ)
							pParams->Strength[0][idx] = 2;
					}
					for (idx = 4;idx < 16;idx += 1)
					{
						blkQ = INTERNAL_BLOCKS_MASK[1][idx - 4];

						if (cbp4x4_luma & blkQ)
							pParams->Strength[1][idx] = 2;
					}
				}
			}
		}
	}
	return;
}
#endif
void H264SegmentDecoder::DeblockLuma(Ipp32u dir, DeblockingParameters *pParams)
{
    Ipp8u *pY = pParams->pLuma;
    Ipp32s pic_pitch = pParams->pitch;
    Ipp32u MBAddr = pParams->nMBAddr;
    Ipp8u Clipping[16];
    Ipp8u Alpha[2];
    Ipp8u Beta[2];
    Ipp32s AlphaC0Offset = pParams->nAlphaC0Offset;
    Ipp32s BetaOffset = pParams->nBetaOffset;
    Ipp32s pmq_QP = m_mbinfo.mbs[MBAddr].QP;
	int	const_51 = 51;

    //
    // luma deblocking
    //
	Profile_Start(ippiFilterDeblockingLuma);
    if (pParams->DeblockingFlag[dir])
    {
        Ipp8u *pClipTab;
        Ipp32s QP;
        Ipp32s index;
        Ipp8u *pStrength = pParams->Strength[dir];
        //
        // correct strengths for high profile
        //
        if (pGetMB8x8TSFlag(m_pCurrentFrame->m_mbinfo.mbs + MBAddr))
        {
            SetEdgeStrength(pStrength + 4, 0);
            SetEdgeStrength(pStrength + 12, 0);
        }

        if (pParams->ExternalEdgeFlag[dir])
        {
            Ipp32s pmp_QP;

            // get neighbour block QP
            pmp_QP = m_mbinfo.mbs[pParams->nNeighbour[dir]].QP;

            // luma variables
            QP = (pmp_QP + pmq_QP + 1) >> 1 ;

            // external edge variables
            //***index = IClip(0, 51, QP + BetaOffset);
            index = QP + BetaOffset; Clip1(const_51, index);
            Beta[0] = BETA_TABLE[index];

            //***index = IClip(0, 51, QP + AlphaC0Offset);
            index = QP + AlphaC0Offset; Clip1(const_51, index);
			Alpha[0] = ALPHA_TABLE[index];
            pClipTab = CLIP_TAB[index];

            // create clipping values
            Clipping[0] = pClipTab[pStrength[0]];
            Clipping[1] = pClipTab[pStrength[1]];
            Clipping[2] = pClipTab[pStrength[2]];
            Clipping[3] = pClipTab[pStrength[3]];

        }

        // internal edge variables
        QP = pmq_QP;

        //***index = IClip(0, 51, QP + BetaOffset);
        index = QP + BetaOffset; Clip1(const_51, index);
		Beta[1] = BETA_TABLE[index];

        //***index = IClip(0, 51, QP + AlphaC0Offset);
        index = QP + AlphaC0Offset; Clip1(const_51, index);
		Alpha[1] = ALPHA_TABLE[index];
        pClipTab = CLIP_TAB[index];

        // create clipping values
        {
            Ipp32u edge;

            for (edge = 1;edge < 4;edge += 1)
            {
                if (*((Ipp32u *) (pStrength + edge * 4)))
                {
                    // create clipping values
                    Clipping[edge * 4 + 0] = pClipTab[pStrength[edge * 4 + 0]];
                    Clipping[edge * 4 + 1] = pClipTab[pStrength[edge * 4 + 1]];
                    Clipping[edge * 4 + 2] = pClipTab[pStrength[edge * 4 + 2]];
                    Clipping[edge * 4 + 3] = pClipTab[pStrength[edge * 4 + 3]];
                }
            }
        }

        // perform deblocking
        IppLumaDeblocking[dir](pY,
                               pic_pitch,
                               Alpha,
                               Beta,
                               Clipping,
                               pStrength);
    }

	Profile_End(ippiFilterDeblockingLuma);

} // void H264SegmentDecoder::DeblockLuma(Ipp32u dir, DeblockingParameters *pParams)
void H264SegmentDecoder::DeblockChroma400(Ipp32u, DeblockingParameters *)
{
    // there is nothing to deblock

} // void H264SegmentDecoder::DeblockChroma400(Ipp32u, DeblockingParameters *)

void H264SegmentDecoder::DeblockChroma420(Ipp32u dir, DeblockingParameters *pParams)
{
    if (pParams->DeblockingFlag[dir])
    {
        Ipp32s pic_pitch = pParams->pitch;
        Ipp32u MBAddr = pParams->nMBAddr;
        Ipp8u Clipping[16];
        Ipp8u Alpha[2];
        Ipp8u Beta[2];
        Ipp32s AlphaC0Offset = pParams->nAlphaC0Offset;
        Ipp32s BetaOffset = pParams->nBetaOffset;
        Ipp32s pmq_QP = m_mbinfo.mbs[MBAddr].QP;
        Ipp8u *pClipTab;
        Ipp32s QP;
        Ipp32s index;
        Ipp8u *pStrength = pParams->Strength[dir];
        Ipp32u nPlane;
        Ipp32s chroma_qp_offset = ~(m_pPicParamSet->chroma_qp_index_offset[0]);
		//***
		int	const_51 = 51;

		Profile_Start(ippiFilterDeblockingLuma);

        for (nPlane = 0; nPlane < 2; nPlane += 1)
        {
            if (chroma_qp_offset != m_pPicParamSet->chroma_qp_index_offset[nPlane])
            {
                chroma_qp_offset = m_pPicParamSet->chroma_qp_index_offset[nPlane];

                if (pParams->ExternalEdgeFlag[dir])
                {
                    Ipp32s pmp_QP;

                    // get left block QP
                    pmp_QP = m_mbinfo.mbs[pParams->nNeighbour[dir]].QP;

                    // external edge variables
					{
						/***QP = (QP_SCALE_CR[IClip(0, 51, pmp_QP + chroma_qp_offset)] +			\
							       QP_SCALE_CR[IClip(0, 51, pmq_QP + chroma_qp_offset)] + 1) >> 1;
						*/
						int index1, index2;
						index1 = pmp_QP + chroma_qp_offset; Clip1(const_51, index1);
						index2 = pmq_QP + chroma_qp_offset; Clip1(const_51, index2);
						QP = (QP_SCALE_CR[index1] +	QP_SCALE_CR[index2] + 1) >> 1;
					}
                    
					//***index = IClip(0, 51, QP + BetaOffset);
                    index = QP + BetaOffset; Clip1(const_51, index);
					Beta[0] = BETA_TABLE[index];

                    //***index = IClip(0, 51, QP + AlphaC0Offset);
                    index = QP + AlphaC0Offset; Clip1(const_51, index);
					Alpha[0] = ALPHA_TABLE[index];
                    pClipTab = CLIP_TAB[index];

                    // create clipping values
                    Clipping[0] = pClipTab[pStrength[0]];
                    Clipping[1] = pClipTab[pStrength[1]];
                    Clipping[2] = pClipTab[pStrength[2]];
                    Clipping[3] = pClipTab[pStrength[3]];

                }

                // internal edge variables
                //***QP = QP_SCALE_CR[IClip(0, 51, pmq_QP + chroma_qp_offset)];
                index = pmq_QP + chroma_qp_offset; Clip1(const_51, index);
                QP = QP_SCALE_CR[index];

                //***index = IClip(0, 51, QP + BetaOffset);
                index = QP + BetaOffset; Clip1(const_51, index);
				Beta[1] = BETA_TABLE[index];

                //***index = IClip(0, 51, QP + AlphaC0Offset);
                index = QP + AlphaC0Offset; Clip1(const_51, index);
				Alpha[1] = ALPHA_TABLE[index];
                pClipTab = CLIP_TAB[index];

                // create clipping values
                Clipping[4] = pClipTab[pStrength[8]];
                Clipping[5] = pClipTab[pStrength[9]];
                Clipping[6] = pClipTab[pStrength[10]];
                Clipping[7] = pClipTab[pStrength[11]];
            }

            // perform deblocking chroma component
            IppChromaDeblocking[dir](pParams->pChroma[nPlane],
                                     pic_pitch,
                                     Alpha,
                                     Beta,
                                     Clipping,
                                     pStrength);
        }
	  	Profile_End(ippiFilterDeblockingLuma);
    }

} // void H264SegmentDecoder::DeblockChroma420(Ipp32u dir, DeblockingParameters *pParams)

void H264SegmentDecoder::DeblockChroma422(Ipp32u, DeblockingParameters *)
{
    /* DEBUG : need to implement */

} // void H264SegmentDecoder::DeblockChroma422(Ipp32u, DeblockingParameters *)

void H264SegmentDecoder::DeblockChroma444(Ipp32u, DeblockingParameters *)
{
    /* DEBUG : need to implement */

} // void H264SegmentDecoder::DeblockChroma444(Ipp32u, DeblockingParameters *)

void H264SegmentDecoder::ResetDeblockingVariables(DeblockingParameters *pParams)
{
    Ipp8u *pY, *pU, *pV;
    Ipp32u offset;
    Ipp32s MBYAdjust = 0;
    Ipp32u mbXOffset, mbYOffset;
    Ipp32s pic_pitch = m_pCurrentFrame->pitch();
    Ipp32u MBAddr = pParams->nMBAddr;
    Ipp32u nCurrMB_X, nCurrMB_Y;
    H264SliceHeader *pHeader;

    // load slice header
    pHeader = (m_bFrameDeblocking) ?
              (m_pSliceStore->GetSliceHeader(m_pCurrentFrame->m_mbinfo.mbs[MBAddr].slice_id)) :
              (m_pSliceHeader);


    // load planes
    pY = m_pCurrentFrame->m_pYPlane;
    pU = m_pCurrentFrame->m_pUPlane;
    pV = m_pCurrentFrame->m_pVPlane;

    // prepare macroblock variables
    nCurrMB_X = (MBAddr % mb_width);
    nCurrMB_Y = (MBAddr / mb_width)- MBYAdjust;
    mbXOffset = nCurrMB_X * 16;
    mbYOffset = nCurrMB_Y * 16;

    // calc plane's offsets
    offset = mbXOffset + (mbYOffset * pic_pitch);
    pY += offset;
    offset >>= 1;
    pU += offset;
    pV += offset;

    // set external edge variables
    pParams->ExternalEdgeFlag[VERTICAL_DEBLOCKING] = (nCurrMB_X != 0);
    pParams->ExternalEdgeFlag[HORIZONTAL_DEBLOCKING] = (nCurrMB_Y != 0);

    if (DEBLOCK_FILTER_ON_NO_SLICE_EDGES == pHeader->disable_deblocking_filter_idc)
    {
        // don't filter at slice boundaries
        if (nCurrMB_X)
        {
            if (m_pCurrentFrame->m_mbinfo.mbs[MBAddr].slice_id !=
                m_pCurrentFrame->m_mbinfo.mbs[MBAddr - 1].slice_id)
                pParams->ExternalEdgeFlag[VERTICAL_DEBLOCKING] = 0;
        }

        if (nCurrMB_Y)
        {
            if (m_pCurrentFrame->m_mbinfo.mbs[MBAddr].slice_id !=
                m_pCurrentFrame->m_mbinfo.mbs[MBAddr - mb_width].slice_id)
                pParams->ExternalEdgeFlag[HORIZONTAL_DEBLOCKING] = 0;
        }
    }

    // reset external edges strength
    SetEdgeStrength(pParams->Strength[VERTICAL_DEBLOCKING], 0);
    SetEdgeStrength(pParams->Strength[HORIZONTAL_DEBLOCKING], 0);

    // set neighbour addreses
    pParams->nNeighbour[VERTICAL_DEBLOCKING] = MBAddr - 1;
    pParams->nNeighbour[HORIZONTAL_DEBLOCKING] = MBAddr - mb_width;

    // set deblocking flag(s)
    pParams->DeblockingFlag[VERTICAL_DEBLOCKING] = 0;
    pParams->DeblockingFlag[HORIZONTAL_DEBLOCKING] = 0;

    // save variables
    pParams->pLuma = pY;
    pParams->pChroma[0] = pU;
    pParams->pChroma[1] = pV;


    pParams->pitch = pic_pitch;
    pParams->nMaxMVector  = 4;//***bnie: (FRM_STRUCTURE > m_pCurrentFrame->m_PictureStructureForDec) ? (2) : (4);
    //***bnie: pParams->MBFieldCoded = 0;//(FRM_STRUCTURE > m_pCurrentFrame->m_PictureStructureForDec);

    // set slice's variables
    pParams->nAlphaC0Offset = pHeader->slice_alpha_c0_offset;
    pParams->nBetaOffset = pHeader->slice_beta_offset;
} // void H264SegmentDecoder::ResetDeblockingVariables(DeblockingParameters *pParams)

void H264SegmentDecoder::PrepareDeblockingParametersISlice(DeblockingParameters *pParams)
{
    // set deblocking flag(s)
    pParams->DeblockingFlag[VERTICAL_DEBLOCKING] = 1;
    pParams->DeblockingFlag[HORIZONTAL_DEBLOCKING] = 1;

    // calculate strengths
    if (pParams->ExternalEdgeFlag[VERTICAL_DEBLOCKING])
    {
        // deblocking with strong deblocking of external edge
        SetEdgeStrength(pParams->Strength[VERTICAL_DEBLOCKING] + 0, 4);
    }

    SetEdgeStrength(pParams->Strength[VERTICAL_DEBLOCKING] + 4, 3);
    SetEdgeStrength(pParams->Strength[VERTICAL_DEBLOCKING] + 8, 3);
    SetEdgeStrength(pParams->Strength[VERTICAL_DEBLOCKING] + 12, 3);

    if (pParams->ExternalEdgeFlag[HORIZONTAL_DEBLOCKING])
    {
        {
            // deblocking with strong deblocking of external edge
            SetEdgeStrength(pParams->Strength[HORIZONTAL_DEBLOCKING] + 0, 4);
        }
    }

    SetEdgeStrength(pParams->Strength[HORIZONTAL_DEBLOCKING] + 4, 3);
    SetEdgeStrength(pParams->Strength[HORIZONTAL_DEBLOCKING] + 8, 3);
    SetEdgeStrength(pParams->Strength[HORIZONTAL_DEBLOCKING] + 12, 3);

} // void H264SegmentDecoder::PrepareDeblockingParametersISlice(DeblockingParameters *pParams)

void H264SegmentDecoder::PrepareDeblockingParametersPSlice(DeblockingParameters *pParams)
{
    Ipp32u MBAddr = pParams->nMBAddr;
    Ipp32u mbtype = (m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->mbtype;

#ifndef _KINOMA_LOSSY_OPT_
    // when this macroblock is intra coded
    if (IS_INTRA_MBTYPE(mbtype))
    {
        PrepareDeblockingParametersISlice(pParams);
		return;
	}
#endif

    // try simplest function to prepare deblocking parameters
    switch (mbtype)
    {
        // when macroblock has type inter 16 on 16
    case MBTYPE_INTER:
    case MBTYPE_FORWARD:
    case MBTYPE_BACKWARD:
    case MBTYPE_BIDIR:
        PrepareDeblockingParametersPSlice16(VERTICAL_DEBLOCKING, pParams);
        PrepareDeblockingParametersPSlice16(HORIZONTAL_DEBLOCKING, pParams);
        break;
/*
        // when macroblock has type inter 16 on 8
    case MBTYPE_INTER_16x8:
        PrepareDeblockingParametersPSlice8x16(VERTICAL_DEBLOCKING, pParams);
        PrepareDeblockingParametersPSlice16x8(HORIZONTAL_DEBLOCKING, pParams);
        return;

        // when macroblock has type inter 8 on 16
    case MBTYPE_INTER_8x16:
        PrepareDeblockingParametersPSlice16x8(VERTICAL_DEBLOCKING, pParams);
        PrepareDeblockingParametersPSlice8x16(HORIZONTAL_DEBLOCKING, pParams);
        return;
*/
    default:
        PrepareDeblockingParametersPSlice4(VERTICAL_DEBLOCKING, pParams);
        PrepareDeblockingParametersPSlice4(HORIZONTAL_DEBLOCKING, pParams);
        break;
    }

} // void H264SegmentDecoder::PrepareDeblockingParametersPSlice(DeblockingParameters *pParams)

void H264SegmentDecoder::PrepareDeblockingParametersPSlice4(Ipp32u dir, DeblockingParameters *pParams)
{
    Ipp32u MBAddr = pParams->nMBAddr;
    Ipp32u cbp4x4_luma = (m_mbinfo.mbs + MBAddr)->cbp4x4_luma;
    Ipp8u *pStrength = pParams->Strength[dir];
    Ipp32u *pDeblockingFlag = &(pParams->DeblockingFlag[dir]);

    //
    // external edge
    //

    if (pParams->ExternalEdgeFlag[dir])
    {
        Ipp32u nNeighbour;

        // select neighbour addres
        nNeighbour = pParams->nNeighbour[dir];

        // when neighbour macroblock isn't intra
        if (!IS_INTRA_MBTYPE((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->mbtype))
        {
            H264DecoderMacroblockLocalInfo *pNeighbour;
            Ipp32u idx;

            // select neighbour
            pNeighbour = m_mbinfo.mbs + nNeighbour;

            // cicle on blocks
            for (idx = 0;idx < 4;idx += 1)
            {
                Ipp32u blkQ, blkP;

                blkQ = EXTERNAL_BLOCK_MASK[dir][CURRENT_BLOCK][idx];
                blkP = EXTERNAL_BLOCK_MASK[dir][NEIGHBOUR_BLOCK][idx];

                // when one of couple of blocks has coeffs
                if ((cbp4x4_luma & blkQ) ||
                    (pNeighbour->cbp4x4_luma & blkP))
                {
                    pStrength[idx] = 2;
                    *pDeblockingFlag = 1;
                }
                // compare motion vectors & reference indexes
                else
                {
                    Ipp32u nBlock, nNeighbourBlock;
                    Ipp32s iRefQ, iRefP;
                    Ipp32s nVectorDiffLimit = pParams->nMaxMVector;

                    // calc block and neighbour block number
                    if (VERTICAL_DEBLOCKING == dir)
                    {
                        nBlock = idx * 4;
                        nNeighbourBlock = idx * 4 + 3;
                    }
                    else
                    {
                        nBlock = idx;
                        nNeighbourBlock = idx + 12;
                    }

                    {
                        H264VideoDecoder::H264DecoderFrame **pRefPicList;
                        Ipp32s index;

                        // select reference index for current block
                        index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][MBAddr].RefIdxs[nBlock];
                        if (0 <= index)
                        {
							//***kinoma enhancement   --bnie 8/2/2008
							pRefPicList = m_pCurrentFrame->GetRefPicListSafe((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0 );
							//pRefPicList = m_pCurrentFrame->GetRefPicList(    (m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0)->m_RefPicList;
							iRefQ = pRefPicList == NULL ? -1 : pRefPicList[index]->DeblockPicID(0);
                        }
                        else
                            iRefQ = -1;

                        // select reference index for previous block
                        index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][nNeighbour].RefIdxs[nNeighbourBlock];
                        if (0 <= index)
                        {
							//***kinoma enhancement   --bnie 8/2/2008
							pRefPicList = m_pCurrentFrame->GetRefPicListSafe((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 0 );
							//pRefPicList = m_pCurrentFrame->GetRefPicList(  (m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 0 )->m_RefPicList;
                            //iRefP = pRefPicList[index]->DeblockPicID(0);
                            iRefP = pRefPicList == NULL ? -1 : pRefPicList[index]->DeblockPicID(0);

                        }
                        else
                            iRefP = -1;
                    }

                    VM_ASSERT((iRefP != -1) || (iRefQ != -1));

                    // when reference indexes are equal
                    if (iRefQ == iRefP)
                    {
                        H264DecoderMotionVector *pVectorQ, *pVectorP;

                        pVectorQ = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + nBlock;
                        pVectorP = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;

                        // compare motion vectors
                        if ((4 <= abs(pVectorQ->mvx - pVectorP->mvx)) ||
                            (nVectorDiffLimit <= abs(pVectorQ->mvy - pVectorP->mvy)))
                        {
                            pStrength[idx] = 1;
                            *pDeblockingFlag = 1;
                        }
                        else
                            pStrength[idx] = 0;
                    }
                    // when reference indexes are different
                    else
                    {
                        pStrength[idx] = 1;
                        *pDeblockingFlag = 1;
                    }
                }
            }
        }
        // external edge required in strong filtering
        else
        {
            SetEdgeStrength(pStrength + 0, 4);
            *pDeblockingFlag = 1;
        }
    }

    //
    // internal edge(s)
    //
    {
        Ipp32u idx;

        // cicle of edge(s)
        // we do all edges in one cicle
        for (idx = 4;idx < 16;idx += 1)
        {
            Ipp32u blkQ;

            blkQ = INTERNAL_BLOCKS_MASK[dir][idx - 4];

            if (cbp4x4_luma & blkQ)
            {
                pStrength[idx] = 2;
                *pDeblockingFlag = 1;
            }
            // compare motion vectors & reference indexes
            else
            {
                Ipp32u nBlock, nNeighbourBlock;
                Ipp32s iRefQ, iRefP;
                Ipp32s nVectorDiffLimit = pParams->nMaxMVector;

                // calc block and neighbour block number
                if (VERTICAL_DEBLOCKING == dir)
                {
                    nBlock = (idx & 3) * 4 + (idx >> 2);
                    nNeighbourBlock = nBlock - 1;
                }
                else
                {
                    nBlock = idx;
                    nNeighbourBlock = idx - 4;
                }

                VM_ASSERT(-1 == m_pCurrentFrame->m_mbinfo.RefIdxs[1][MBAddr].RefIdxs[nBlock]);
                VM_ASSERT(-1 == m_pCurrentFrame->m_mbinfo.RefIdxs[1][MBAddr].RefIdxs[nNeighbourBlock]);

                {
                    H264VideoDecoder::H264DecoderFrame **pRefPicList;
                    Ipp32s index;

                    pRefPicList = m_pCurrentFrame->GetRefPicListSafe((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0);
                    //***pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0)->m_RefPicList;

                    // select reference index for current block
                    index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][MBAddr].RefIdxs[nBlock];
                    iRefQ = (index < 0 || pRefPicList == NULL) ?
                            (-1) :
                            (pRefPicList[index]->DeblockPicID(0));

                    // select reference index for previous block
                    index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][MBAddr].RefIdxs[nNeighbourBlock];
                    iRefP = (index < 0 || pRefPicList == NULL) ?
                            (-1) :
                            pRefPicList[index]->DeblockPicID(0);
                }

                VM_ASSERT((iRefP != -1) || (iRefQ != -1));

                // when reference indexes are equal
                if (iRefQ == iRefP)
                {
                    H264DecoderMotionVector *pVectorQ, *pVectorP;

                    pVectorQ = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + nBlock;
                    pVectorP = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + nNeighbourBlock;

                    // compare motion vectors
                    if ((4 <= abs(pVectorQ->mvx - pVectorP->mvx)) ||
                        (nVectorDiffLimit <= abs(pVectorQ->mvy - pVectorP->mvy)))
                    {
                        pStrength[idx] = 1;
                        *pDeblockingFlag = 1;
                    }
                    else
                        pStrength[idx] = 0;
                }
                // when reference indexes are different
                else
                {
                    pStrength[idx] = 1;
                    *pDeblockingFlag = 1;
                }
            }
        }
    }

} // void H264SegmentDecoder::PrepareDeblockingParametersPSlice4(Ipp32u dir, DeblockingParameters *pParams)

void H264SegmentDecoder::PrepareDeblockingParametersPSlice16(Ipp32u dir, DeblockingParameters *pParams)
{
    Ipp32u MBAddr = pParams->nMBAddr;
    Ipp32u cbp4x4_luma = (m_mbinfo.mbs + MBAddr)->cbp4x4_luma;
    Ipp8u *pStrength = pParams->Strength[dir];
    Ipp32u *pDeblockingFlag = &(pParams->DeblockingFlag[dir]);

    //
    // external edge
    //
    if (pParams->ExternalEdgeFlag[dir])
    {
        Ipp32u nNeighbour;

        // select neighbour addres
        nNeighbour = pParams->nNeighbour[dir];

        // when neighbour macroblock isn't intra
        if (!IS_INTRA_MBTYPE((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->mbtype))
        {
            H264DecoderMacroblockLocalInfo *pNeighbour;
            Ipp32u idx;
            Ipp32s iRefQ;
            H264DecoderMotionVector *pVectorQ;

            // load reference index & motion vector for current block
            {
                // field coded image
                {
                    H264VideoDecoder::H264DecoderFrame **pRefPicList;
                    Ipp32s index;

                    // select reference index for current block
                    index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][MBAddr].RefIdxs[0];
                    if (0 <= index)
                    {
                        pRefPicList = m_pCurrentFrame->GetRefPicListSafe((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0);
                        //pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0)->m_RefPicList;
                        iRefQ = pRefPicList[index]->DeblockPicID(0);
                    }
                    else
                        iRefQ = -1;
                }

                pVectorQ = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors;
            }

            // select neighbour
            pNeighbour = m_mbinfo.mbs + nNeighbour;

            // cicle on blocks
            for (idx = 0;idx < 4;idx += 1)
            {
                Ipp32u blkQ, blkP;

                blkQ = EXTERNAL_BLOCK_MASK[dir][CURRENT_BLOCK][idx];
                blkP = EXTERNAL_BLOCK_MASK[dir][NEIGHBOUR_BLOCK][idx];

                // when one of couple of blocks has coeffs
                if ((cbp4x4_luma & blkQ) ||
                    (pNeighbour->cbp4x4_luma & blkP))
                {
                    pStrength[idx] = 2;
                    *pDeblockingFlag = 1;
                }
                // compare motion vectors & reference indexes
                else
                {
                    Ipp32u nNeighbourBlock;
                    Ipp32s iRefP;
                    Ipp32s nVectorDiffLimit = pParams->nMaxMVector;

                    // calc block and neighbour block number
                    if (VERTICAL_DEBLOCKING == dir)
                        nNeighbourBlock = idx * 4 + 3;
                    else
                        nNeighbourBlock = idx + 12;

                    // field coded image
                    {
                        H264VideoDecoder::H264DecoderFrame **pRefPicList;
                        Ipp32s index;

                        // select reference index for previous block
                        index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][nNeighbour].RefIdxs[nNeighbourBlock];
                        if (0 <= index)
                        {
                            pRefPicList = m_pCurrentFrame->GetRefPicListSafe((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 0);
                            //***pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 0)->m_RefPicList;
							iRefP = pRefPicList == NULL ? -1 : pRefPicList[index]->DeblockPicID(0);
                        }
                        else
                            iRefP = -1;
                    }

                    VM_ASSERT((iRefP != -1) || (iRefQ != -1));

                    // when reference indexes are equal
                    if (iRefQ == iRefP)
                    {
                        H264DecoderMotionVector *pVectorP;

                        pVectorP = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;

                        // compare motion vectors
                        if ((4 <= abs(pVectorQ->mvx - pVectorP->mvx)) ||
                            (nVectorDiffLimit <= abs(pVectorQ->mvy - pVectorP->mvy)))
                        {
                            pStrength[idx] = 1;
                            *pDeblockingFlag = 1;
                        }
                        else
                            pStrength[idx] = 0;
                    }
                    // when reference indexes are different
                    else
                    {
                        pStrength[idx] = 1;
                        *pDeblockingFlag = 1;
                    }
                }
            }
        }
        // external edge required in strong filtering
        else
        {
            SetEdgeStrength(pStrength + 0, 4);
            *pDeblockingFlag = 1;
        }
    }

    //
    // internal edge(s)
    //
    {
        Ipp32u idx;

        // reset all strengths
        SetEdgeStrength(pStrength + 4, 0);
        SetEdgeStrength(pStrength + 8, 0);
        SetEdgeStrength(pStrength + 12, 0);

        // set deblocking flag
        if (cbp4x4_luma & 0x1fffe)
            *pDeblockingFlag = 1;

        // cicle of edge(s)
        // we do all edges in one cicle
        for (idx = 4;idx < 16;idx += 1)
        {
            Ipp32u blkQ;

            blkQ = INTERNAL_BLOCKS_MASK[dir][idx - 4];

            if (cbp4x4_luma & blkQ)
                pStrength[idx] = 2;
        }
    }

} // void H264SegmentDecoder::PrepareDeblockingParametersPSlice16(Ipp32u dir, DeblockingParameters *pParams)

void H264SegmentDecoder::PrepareDeblockingParametersBSlice(DeblockingParameters *pParams)
{
    Ipp32u MBAddr = pParams->nMBAddr;
    Ipp32u mbtype = (m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->mbtype;

#ifndef _KINOMA_LOSSY_OPT_
    // when this macroblock is intra coded
    if (IS_INTRA_MBTYPE(mbtype))
    {
        PrepareDeblockingParametersISlice(pParams);
        return;
    }
#endif

    // try simplest function to prepare deblocking parameters
    switch (mbtype)
    {
        // when macroblock has type inter 16 on 16
    case MBTYPE_INTER:
    case MBTYPE_FORWARD:
    case MBTYPE_BACKWARD:
    case MBTYPE_BIDIR:
        PrepareDeblockingParametersBSlice16(VERTICAL_DEBLOCKING, pParams);
        PrepareDeblockingParametersBSlice16(HORIZONTAL_DEBLOCKING, pParams);
        break;

        // when macroblock has type inter 16 on 8
    case MBTYPE_INTER_16x8:
        PrepareDeblockingParametersBSlice8x16(VERTICAL_DEBLOCKING, pParams);
        PrepareDeblockingParametersBSlice16x8(HORIZONTAL_DEBLOCKING, pParams);
        return;

        // when macroblock has type inter 8 on 16
    case MBTYPE_INTER_8x16:
        PrepareDeblockingParametersBSlice16x8(VERTICAL_DEBLOCKING, pParams);
        PrepareDeblockingParametersBSlice8x16(HORIZONTAL_DEBLOCKING, pParams);
        return;

    default:
        PrepareDeblockingParametersBSlice4(VERTICAL_DEBLOCKING, pParams);
        PrepareDeblockingParametersBSlice4(HORIZONTAL_DEBLOCKING, pParams);
        break;
    }

} // void H264SegmentDecoder::PrepareDeblockingParametersBSlice(DeblockingParameters *pParams)

void H264SegmentDecoder::PrepareDeblockingParametersBSlice4(Ipp32u dir, DeblockingParameters *pParams)
{
    Ipp32u MBAddr = pParams->nMBAddr;
    Ipp32u cbp4x4_luma = (m_mbinfo.mbs + MBAddr)->cbp4x4_luma;
    Ipp8u *pStrength = pParams->Strength[dir];
    Ipp32u *pDeblockingFlag = &(pParams->DeblockingFlag[dir]);

    //
    // external edge
    //
    if (pParams->ExternalEdgeFlag[dir])
    {
        Ipp32u nNeighbour;

        // select neighbour addres
        nNeighbour = pParams->nNeighbour[dir];

        // when neighbour macroblock isn't intra
        if (!IS_INTRA_MBTYPE((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->mbtype))
        {
            H264DecoderMacroblockLocalInfo *pNeighbour;
            Ipp32u idx;

            // select neighbour
            pNeighbour = m_mbinfo.mbs + nNeighbour;

            // cicle on blocks
            for (idx = 0;idx < 4;idx += 1)
            {
                Ipp32u blkQ, blkP;

                blkQ = EXTERNAL_BLOCK_MASK[dir][CURRENT_BLOCK][idx];
                blkP = EXTERNAL_BLOCK_MASK[dir][NEIGHBOUR_BLOCK][idx];

                // when one of couple of blocks has coeffs
                if ((cbp4x4_luma & blkQ) ||
                    (pNeighbour->cbp4x4_luma & blkP))
                {
                    pStrength[idx] = 2;
                    *pDeblockingFlag = 1;
                }
                // compare motion vectors & reference indexes
                else
                {
                    Ipp32u nBlock, nNeighbourBlock;
                    Ipp32s iRefQFrw, iRefPFrw, iRefQBck, iRefPBck;
                    Ipp32s nVectorDiffLimit = pParams->nMaxMVector;

                    // calc block and neighbour block number
                    if (VERTICAL_DEBLOCKING == dir)
                    {
                        nBlock = idx * 4;
                        nNeighbourBlock = nBlock + 3;
                    }
                    else
                    {
                        nBlock = idx;
                        nNeighbourBlock = idx + 12;
                    }

                    // field coded image
                    {
                        H264VideoDecoder::H264DecoderFrame **pRefPicList;
                        Ipp32s index;

                        // select reference index for current block
                        index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][MBAddr].RefIdxs[nBlock];
                        if (0 <= index)
                        {
                            pRefPicList = m_pCurrentFrame->GetRefPicListSafe((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0);
                            //***pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0)->m_RefPicList;
							iRefQFrw = pRefPicList == NULL ? -1 : pRefPicList[index]->DeblockPicID(0);
                        }
                        else
                            iRefQFrw = -1;
                        index = m_pCurrentFrame->m_mbinfo.RefIdxs[1][MBAddr].RefIdxs[nBlock];
                        if (0 <= index)
                        {
                            pRefPicList = m_pCurrentFrame->GetRefPicListSafe((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 1);
                            //***pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 1)->m_RefPicList;
                            iRefQBck = pRefPicList == NULL ? -1 : pRefPicList[index]->DeblockPicID(0);
                        }
                        else
                            iRefQBck = -1;

                        // select reference index for previous block
                        index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][nNeighbour].RefIdxs[nNeighbourBlock];
                        if (0 <= index)
                        {
                            pRefPicList = m_pCurrentFrame->GetRefPicListSafe((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 0);
                            //***pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 0)->m_RefPicList;
                            iRefPFrw = pRefPicList == NULL ? -1 : pRefPicList[index]->DeblockPicID(0);
                        }
                        else
                            iRefPFrw = -1;
                        index = m_pCurrentFrame->m_mbinfo.RefIdxs[1][nNeighbour].RefIdxs[nNeighbourBlock];
                        if (0 <= index)
                        {
                            pRefPicList = m_pCurrentFrame->GetRefPicListSafe((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 1);
                            //***pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 1)->m_RefPicList;
                            iRefPBck = pRefPicList == NULL ? -1 : pRefPicList[index]->DeblockPicID(0);
                        }
                        else
                            iRefPBck = -1;
                    }

                    // when reference indexes are equal
                    if (((iRefQFrw == iRefPFrw) && (iRefQBck == iRefPBck)) ||
                        ((iRefQFrw == iRefPBck) && (iRefQBck == iRefPFrw)))
                    {
                        // set initial value of strength
                        pStrength[idx] = 0;

                        // when forward and backward reference pictures of previous block are different
                        if (iRefPFrw != iRefPBck)
                        {
                            H264DecoderMotionVector *pVectorQFrw, *pVectorQBck;
                            H264DecoderMotionVector *pVectorPFrw, *pVectorPBck;

                            // select current block motion vectors
                            pVectorQFrw = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + nBlock;
                            pVectorQBck = m_pCurrentFrame->m_mbinfo.MV[1][MBAddr].MotionVectors + nBlock;

                            // select previous block motion vectors
                            if (iRefQFrw == iRefPFrw)
                            {
                                pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;
                                pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[1][nNeighbour].MotionVectors + nNeighbourBlock;
                            }
                            else
                            {
                                pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[1][nNeighbour].MotionVectors + nNeighbourBlock;
                                pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;
                            }

                            // compare motion vectors
                            if ((4 <= abs(pVectorQFrw->mvx - pVectorPFrw->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorPFrw->mvy)) ||
                                (4 <= abs(pVectorQBck->mvx - pVectorPBck->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorPBck->mvy)))
                            {
                                pStrength[idx] = 1;
                                *pDeblockingFlag = 1;
                            }
                        }
                        // when forward and backward reference pictures of previous block are equal
                        else
                        {
                            H264DecoderMotionVector *pVectorQFrw, *pVectorQBck;
                            H264DecoderMotionVector *pVectorPFrw, *pVectorPBck;

                            // select current block motion vectors
                            pVectorQFrw = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + nBlock;
                            pVectorQBck = m_pCurrentFrame->m_mbinfo.MV[1][MBAddr].MotionVectors + nBlock;

                            // select previous block motion vectors
                            pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;
                            pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[1][nNeighbour].MotionVectors + nNeighbourBlock;

                            // compare motion vectors
                            if ((4 <= abs(pVectorQFrw->mvx - pVectorPFrw->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorPFrw->mvy)) ||
                                (4 <= abs(pVectorQBck->mvx - pVectorPBck->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorPBck->mvy)))
                            {
                                if ((4 <= abs(pVectorQFrw->mvx - pVectorPBck->mvx)) ||
                                    (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorPBck->mvy)) ||
                                    (4 <= abs(pVectorQBck->mvx - pVectorPFrw->mvx)) ||
                                    (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorPFrw->mvy)))
                                {
                                    pStrength[idx] = 1;
                                    *pDeblockingFlag = 1;
                                }
                            }
                        }
                    }
                    // when reference indexes are different
                    else
                    {
                        pStrength[idx] = 1;
                        *pDeblockingFlag = 1;
                    }
                }
            }
        }
        // external edge required in strong filtering
        else
        {
            SetEdgeStrength(pStrength + 0, 4);
            *pDeblockingFlag = 1;
        }
    }

    //
    // internal edge(s)
    //
    {
        Ipp32u idx;

        // cicle of edge(s)
        // we do all edges in one cicle
        for (idx = 4;idx < 16;idx += 1)
        {
            Ipp32u blkQ;

            blkQ = INTERNAL_BLOCKS_MASK[dir][idx - 4];

            if (cbp4x4_luma & blkQ)
            {
                pStrength[idx] = 2;
                *pDeblockingFlag = 1;
            }
            // compare motion vectors & reference indexes
            else
            {
                Ipp32u nBlock, nNeighbourBlock;
                Ipp32s iRefQFrw, iRefQBck, iRefPFrw, iRefPBck;
                Ipp32s nVectorDiffLimit = pParams->nMaxMVector;

                // calc block and neighbour block number
                if (VERTICAL_DEBLOCKING == dir)
                {
                    nBlock = (idx & 3) * 4 + (idx >> 2);
                    nNeighbourBlock = nBlock - 1;
                }
                else
                {
                    nBlock = idx;
                    nNeighbourBlock = idx - 4;
                }

                // field coded image
                {
                    H264VideoDecoder::H264DecoderFrame **pRefPicList;
                    Ipp32s index;

                    // select forward reference pictures list
                    pRefPicList = m_pCurrentFrame->GetRefPicListSafe((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0);
                    //***pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0)->m_RefPicList;
                    // select forward reference index for block(s)
                    index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][MBAddr].RefIdxs[nBlock];
                    iRefQFrw = (index < 0 || pRefPicList== NULL) ?
                            (-1) :
                            (pRefPicList[index]->DeblockPicID(0));
                    index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][MBAddr].RefIdxs[nNeighbourBlock];
                    iRefPFrw = (index < 0 || pRefPicList== NULL) ?
                            (-1) :
                            pRefPicList[index]->DeblockPicID(0);

                    // select backward reference pictures list
                    pRefPicList = m_pCurrentFrame->GetRefPicListSafe((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 1);
                    //***pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 1)->m_RefPicList;
                    // select backward reference index for block(s)
                    index = m_pCurrentFrame->m_mbinfo.RefIdxs[1][MBAddr].RefIdxs[nBlock];
                    iRefQBck = (index < 0 || pRefPicList== NULL) ?
                            (-1) :
                            (pRefPicList[index]->DeblockPicID(0));
                    index = m_pCurrentFrame->m_mbinfo.RefIdxs[1][MBAddr].RefIdxs[nNeighbourBlock];
                    iRefPBck = (index < 0 || pRefPicList== NULL) ?
                            (-1) :
                            pRefPicList[index]->DeblockPicID(0);
                }

                // when reference indexes are equal
                if (((iRefQFrw == iRefPFrw) && (iRefQBck == iRefPBck)) ||
                    ((iRefQFrw == iRefPBck) && (iRefQBck == iRefPFrw)))
                {
                    // set initial value of strength
                    pStrength[idx] = 0;

                    // when forward and backward reference pictures of previous block are different
                    if (iRefPFrw != iRefPBck)
                    {
                        H264DecoderMotionVector *pVectorQFrw, *pVectorQBck;
                        H264DecoderMotionVector *pVectorPFrw, *pVectorPBck;

                        // select current block motion vectors
                        pVectorQFrw = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + nBlock;
                        pVectorQBck = m_pCurrentFrame->m_mbinfo.MV[1][MBAddr].MotionVectors + nBlock;

                        // select previous block motion vectors
                        if (iRefQFrw == iRefPFrw)
                        {
                            pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + nNeighbourBlock;
                            pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[1][MBAddr].MotionVectors + nNeighbourBlock;
                        }
                        else
                        {
                            pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[1][MBAddr].MotionVectors + nNeighbourBlock;
                            pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + nNeighbourBlock;
                        }

                        // compare motion vectors
                        if ((4 <= abs(pVectorQFrw->mvx - pVectorPFrw->mvx)) ||
                            (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorPFrw->mvy)) ||
                            (4 <= abs(pVectorQBck->mvx - pVectorPBck->mvx)) ||
                            (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorPBck->mvy)))
                        {
                            pStrength[idx] = 1;
                            *pDeblockingFlag = 1;
                        }
                    }
                    // when forward and backward reference pictures of previous block are equal
                    else
                    {
                        H264DecoderMotionVector *pVectorQFrw, *pVectorQBck;
                        H264DecoderMotionVector *pVectorPFrw, *pVectorPBck;

                        // select current block motion vectors
                        pVectorQFrw = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + nBlock;
                        pVectorQBck = m_pCurrentFrame->m_mbinfo.MV[1][MBAddr].MotionVectors + nBlock;

                        // select previous block motion vectors
                        pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + nNeighbourBlock;
                        pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[1][MBAddr].MotionVectors + nNeighbourBlock;

                        // compare motion vectors
                        if ((4 <= abs(pVectorQFrw->mvx - pVectorPFrw->mvx)) ||
                            (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorPFrw->mvy)) ||
                            (4 <= abs(pVectorQBck->mvx - pVectorPBck->mvx)) ||
                            (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorPBck->mvy)))
                        {
                            if ((4 <= abs(pVectorQFrw->mvx - pVectorPBck->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorPBck->mvy)) ||
                                (4 <= abs(pVectorQBck->mvx - pVectorPFrw->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorPFrw->mvy)))
                            {
                                pStrength[idx] = 1;
                                *pDeblockingFlag = 1;
                            }
                        }
                    }
                }
                // when reference indexes are different
                else
                {
                    pStrength[idx] = 1;
                    *pDeblockingFlag = 1;
                }
            }
        }
    }

} // void H264SegmentDecoder::PrepareDeblockingParametersBSlice4(Ipp32u dir, DeblockingParameters *pParams)

void H264SegmentDecoder::PrepareDeblockingParametersBSlice16(Ipp32u dir, DeblockingParameters *pParams)
{
    Ipp32u MBAddr = pParams->nMBAddr;
    Ipp32u cbp4x4_luma = (m_mbinfo.mbs + MBAddr)->cbp4x4_luma;
    Ipp8u *pStrength = pParams->Strength[dir];
    Ipp32u *pDeblockingFlag = &(pParams->DeblockingFlag[dir]);

    //
    // external edge
    //
    if (pParams->ExternalEdgeFlag[dir])
    {
        Ipp32u nNeighbour;

        // select neighbour addres
        nNeighbour = pParams->nNeighbour[dir];

        // when neighbour macroblock isn't intra
        if (!IS_INTRA_MBTYPE((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->mbtype))
        {
            H264DecoderMacroblockLocalInfo *pNeighbour;
            Ipp32u idx;
            Ipp32s iRefQFrw, iRefQBck;
            H264DecoderMotionVector *pVectorQFrw, *pVectorQBck;

            // load reference indexes for current block
            {
                {
                    H264VideoDecoder::H264DecoderFrame **pRefPicList;
                    Ipp32s index;

                    // select reference index for current block
                    index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][MBAddr].RefIdxs[0];
                    if (0 <= index)
                    {
                        pRefPicList = m_pCurrentFrame->GetRefPicListSafe((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0);
                        //***pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0)->m_RefPicList;
						iRefQFrw = pRefPicList == NULL ? -1 : pRefPicList[index]->DeblockPicID(0);
                    }
                    else
                        iRefQFrw = -1;
                    index = m_pCurrentFrame->m_mbinfo.RefIdxs[1][MBAddr].RefIdxs[0];
                    if (0 <= index)
                    {
                        pRefPicList = m_pCurrentFrame->GetRefPicListSafe((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 1);
                        //***pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 1)->m_RefPicList;
                        iRefQBck = pRefPicList == NULL ? -1 : pRefPicList[index]->DeblockPicID(0);
                    }
                    else
                        iRefQBck = -1;
                }

                // select current block motion vectors
                pVectorQFrw = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors;
                pVectorQBck = m_pCurrentFrame->m_mbinfo.MV[1][MBAddr].MotionVectors;
            }

            // select neighbour
            pNeighbour = m_mbinfo.mbs + nNeighbour;

            // cicle on blocks
            for (idx = 0;idx < 4;idx += 1)
            {
                Ipp32u blkQ, blkP;

                blkQ = EXTERNAL_BLOCK_MASK[dir][CURRENT_BLOCK][idx];
                blkP = EXTERNAL_BLOCK_MASK[dir][NEIGHBOUR_BLOCK][idx];

                // when one of couple of blocks has coeffs
                if ((cbp4x4_luma & blkQ) ||
                    (pNeighbour->cbp4x4_luma & blkP))
                {
                    pStrength[idx] = 2;
                    *pDeblockingFlag = 1;
                }
                // compare motion vectors & reference indexes
                else
                {
                    Ipp32u nNeighbourBlock;
                    Ipp32s iRefPFrw, iRefPBck;
                    Ipp32s nVectorDiffLimit = pParams->nMaxMVector;

                    // calc block and neighbour block number
                    if (VERTICAL_DEBLOCKING == dir)
                        nNeighbourBlock = idx * 4 + 3;
                    else
                        nNeighbourBlock = idx + 12;

                    {
                        H264VideoDecoder::H264DecoderFrame **pRefPicList;
                        Ipp32s index;

                        // select reference index for previous block
                        index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][nNeighbour].RefIdxs[nNeighbourBlock];
                        if (0 <= index)
                        {
                            pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 0)->m_RefPicList;
                            iRefPFrw = pRefPicList[index]->DeblockPicID(0);
                        }
                        else
                            iRefPFrw = -1;
                        index = m_pCurrentFrame->m_mbinfo.RefIdxs[1][nNeighbour].RefIdxs[nNeighbourBlock];
                        if (0 <= index)
                        {
                            pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 1)->m_RefPicList;
                            iRefPBck = pRefPicList[index]->DeblockPicID(0);
                        }
                        else
                            iRefPBck = -1;
                    }

                    // when reference indexes are equal
                    if (((iRefQFrw == iRefPFrw) && (iRefQBck == iRefPBck)) ||
                        ((iRefQFrw == iRefPBck) && (iRefQBck == iRefPFrw)))
                    {
                        // set initial value of strength
                        pStrength[idx] = 0;

                        // when forward and backward reference pictures of previous block are different
                        if (iRefPFrw != iRefPBck)
                        {
                            H264DecoderMotionVector *pVectorPFrw, *pVectorPBck;

                            // select previous block motion vectors
                            if (iRefQFrw == iRefPFrw)
                            {
                                pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;
                                pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[1][nNeighbour].MotionVectors + nNeighbourBlock;
                            }
                            else
                            {
                                pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[1][nNeighbour].MotionVectors + nNeighbourBlock;
                                pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;
                            }

                            // compare motion vectors
                            if ((4 <= abs(pVectorQFrw->mvx - pVectorPFrw->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorPFrw->mvy)) ||
                                (4 <= abs(pVectorQBck->mvx - pVectorPBck->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorPBck->mvy)))
                            {
                                pStrength[idx] = 1;
                                *pDeblockingFlag = 1;
                            }
                        }
                        // when forward and backward reference pictures of previous block are equal
                        else
                        {
                            H264DecoderMotionVector *pVectorPFrw, *pVectorPBck;

                            // select previous block motion vectors
                            pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;
                            pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[1][nNeighbour].MotionVectors + nNeighbourBlock;

                            // compare motion vectors
                            if ((4 <= abs(pVectorQFrw->mvx - pVectorPFrw->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorPFrw->mvy)) ||
                                (4 <= abs(pVectorQBck->mvx - pVectorPBck->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorPBck->mvy)))
                            {
                                if ((4 <= abs(pVectorQFrw->mvx - pVectorPBck->mvx)) ||
                                    (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorPBck->mvy)) ||
                                    (4 <= abs(pVectorQBck->mvx - pVectorPFrw->mvx)) ||
                                    (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorPFrw->mvy)))
                                {
                                    pStrength[idx] = 1;
                                    *pDeblockingFlag = 1;
                                }
                            }
                        }
                    }
                    // when reference indexes are different
                    else
                    {
                        pStrength[idx] = 1;
                        *pDeblockingFlag = 1;
                    }
                }
            }
        }
        // external edge required in strong filtering
        else
        {
            SetEdgeStrength(pStrength + 0, 4);
            *pDeblockingFlag = 1;
        }
    }

    //
    // internal edge(s)
    //
    {
        Ipp32u idx;

        // reset all strengths
        SetEdgeStrength(pStrength + 4, 0);
        SetEdgeStrength(pStrength + 8, 0);
        SetEdgeStrength(pStrength + 12, 0);

        // set deblocking flag
        if (cbp4x4_luma & 0x1fffe)
            *pDeblockingFlag = 1;

        // cicle of edge(s)
        // we do all edges in one cicle
        for (idx = 4;idx < 16;idx += 1)
        {
            Ipp32u blkQ;

            blkQ = INTERNAL_BLOCKS_MASK[dir][idx - 4];

            if (cbp4x4_luma & blkQ)
                pStrength[idx] = 2;
        }
    }

} // void H264SegmentDecoder::PrepareDeblockingParametersBSlice16(DeblockingParameters *pParams)

void H264SegmentDecoder::PrepareDeblockingParametersBSlice16x8(Ipp32u dir, DeblockingParameters *pParams)
{
    Ipp32u MBAddr = pParams->nMBAddr;
    Ipp32u cbp4x4_luma = (m_mbinfo.mbs + MBAddr)->cbp4x4_luma;
    Ipp8u *pStrength = pParams->Strength[dir];
    Ipp32u *pDeblockingFlag = &(pParams->DeblockingFlag[dir]);
    Ipp32s iRefQFrw, iRefQBck;
    H264DecoderMotionVector *pVectorQFrw, *pVectorQBck;

    //
    // external edge
    //

    // load reference indexes & motion vector for first half of current block
    {
        {
            H264VideoDecoder::H264DecoderFrame **pRefPicList;
            Ipp32s index;

            // select reference index for current block
            index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][MBAddr].RefIdxs[0];
            if (0 <= index)
            {
                pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0)->m_RefPicList;
                iRefQFrw = pRefPicList[index]->DeblockPicID(0);
            }
            else
                iRefQFrw = -1;
            index = m_pCurrentFrame->m_mbinfo.RefIdxs[1][MBAddr].RefIdxs[0];
            if (0 <= index)
            {
                pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 1)->m_RefPicList;
                iRefQBck = pRefPicList[index]->DeblockPicID(0);
            }
            else
                iRefQBck = -1;
        }

        // select current block motion vectors
        pVectorQFrw = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors;
        pVectorQBck = m_pCurrentFrame->m_mbinfo.MV[1][MBAddr].MotionVectors;
    }

    // prepare deblocking parameter for external edge
    if (pParams->ExternalEdgeFlag[dir])
    {
        Ipp32u nNeighbour;

        // select neighbour addres
        nNeighbour = pParams->nNeighbour[dir];

        // when neighbour macroblock isn't intra
        if (!IS_INTRA_MBTYPE((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->mbtype))
        {
            H264DecoderMacroblockLocalInfo *pNeighbour;
            Ipp32u idx;

            // select neighbour
            pNeighbour = m_mbinfo.mbs + nNeighbour;

            // cicle on blocks
            for (idx = 0;idx < 4;idx += 1)
            {
                Ipp32u blkQ, blkP;

                blkQ = EXTERNAL_BLOCK_MASK[dir][CURRENT_BLOCK][idx];
                blkP = EXTERNAL_BLOCK_MASK[dir][NEIGHBOUR_BLOCK][idx];

                // when one of couple of blocks has coeffs
                if ((cbp4x4_luma & blkQ) ||
                    (pNeighbour->cbp4x4_luma & blkP))
                {
                    pStrength[idx] = 2;
                    *pDeblockingFlag = 1;
                }
                // compare motion vectors & reference indexes
                else
                {
                    Ipp32u nNeighbourBlock;
                    Ipp32s iRefPFrw, iRefPBck;
                    Ipp32s nVectorDiffLimit = pParams->nMaxMVector;

                    // calc block and neighbour block number
                    if (VERTICAL_DEBLOCKING == dir)
                        nNeighbourBlock = idx * 4 + 3;
                    else
                        nNeighbourBlock = idx + 12;

                    {
                        H264VideoDecoder::H264DecoderFrame **pRefPicList;
                        Ipp32s index;

                        // select reference index for previous block
                        index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][nNeighbour].RefIdxs[nNeighbourBlock];
                        if (0 <= index)
                        {
                            pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 0)->m_RefPicList;
                            iRefPFrw = pRefPicList[index]->DeblockPicID(0);
                        }
                        else
                            iRefPFrw = -1;
                        index = m_pCurrentFrame->m_mbinfo.RefIdxs[1][nNeighbour].RefIdxs[nNeighbourBlock];
                        if (0 <= index)
                        {
                            pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 1)->m_RefPicList;
                            iRefPBck = pRefPicList[index]->DeblockPicID(0);
                        }
                        else
                            iRefPBck = -1;
                    }

                    // when reference indexes are equal
                    if (((iRefQFrw == iRefPFrw) && (iRefQBck == iRefPBck)) ||
                        ((iRefQFrw == iRefPBck) && (iRefQBck == iRefPFrw)))
                    {
                        // set initial value of strength
                        pStrength[idx] = 0;

                        // when forward and backward reference pictures of previous block are different
                        if (iRefPFrw != iRefPBck)
                        {
                            H264DecoderMotionVector *pVectorPFrw, *pVectorPBck;

                            // select previous block motion vectors
                            if (iRefQFrw == iRefPFrw)
                            {
                                pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;
                                pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[1][nNeighbour].MotionVectors + nNeighbourBlock;
                            }
                            else
                            {
                                pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[1][nNeighbour].MotionVectors + nNeighbourBlock;
                                pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;
                            }

                            // compare motion vectors
                            if ((4 <= abs(pVectorQFrw->mvx - pVectorPFrw->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorPFrw->mvy)) ||
                                (4 <= abs(pVectorQBck->mvx - pVectorPBck->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorPBck->mvy)))
                            {
                                pStrength[idx] = 1;
                                *pDeblockingFlag = 1;
                            }
                        }
                        // when forward and backward reference pictures of previous block are equal
                        else
                        {
                            H264DecoderMotionVector *pVectorPFrw, *pVectorPBck;

                            // select previous block motion vectors
                            pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;
                            pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[1][nNeighbour].MotionVectors + nNeighbourBlock;

                            // compare motion vectors
                            if ((4 <= abs(pVectorQFrw->mvx - pVectorPFrw->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorPFrw->mvy)) ||
                                (4 <= abs(pVectorQBck->mvx - pVectorPBck->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorPBck->mvy)))
                            {
                                if ((4 <= abs(pVectorQFrw->mvx - pVectorPBck->mvx)) ||
                                    (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorPBck->mvy)) ||
                                    (4 <= abs(pVectorQBck->mvx - pVectorPFrw->mvx)) ||
                                    (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorPFrw->mvy)))
                                {
                                    pStrength[idx] = 1;
                                    *pDeblockingFlag = 1;
                                }
                            }
                        }
                    }
                    // when reference indexes are different
                    else
                    {
                        pStrength[idx] = 1;
                        *pDeblockingFlag = 1;
                    }
                }
            }
        }
        // external edge required in strong filtering
        else
        {
            SetEdgeStrength(pStrength + 0, 4);
            *pDeblockingFlag = 1;
        }
    }

    //
    // internal edge(s)
    //
    {
        Ipp32u idx;

        // cicle of edge(s)
        for (idx = 4;idx < 8;idx += 1)
        {
            Ipp32u blkQ;

            blkQ = INTERNAL_BLOCKS_MASK[dir][idx - 4];

            if (cbp4x4_luma & blkQ)
            {
                pStrength[idx] = 2;
                *pDeblockingFlag = 1;
            }
            // we haven't to compare motion vectors  - they are equal
            else
                pStrength[idx] = 0;
        }

        // load reference indexes & motion vector for second half of current block
        {
            Ipp32s iRefQFrw2, iRefQBck2;
            Ipp32u nStrength;
            Ipp32s nVectorDiffLimit = pParams->nMaxMVector;

            // load reference indexes for current block
            {
                H264VideoDecoder::H264DecoderFrame **pRefPicList;
                Ipp32s index;

                // select reference index for current block
                index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][MBAddr].RefIdxs[15];
                if (0 <= index)
                {
                    pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0)->m_RefPicList;
                    iRefQFrw2 = pRefPicList[index]->DeblockPicID(0);
                }
                else
                    iRefQFrw2 = -1;
                index = m_pCurrentFrame->m_mbinfo.RefIdxs[1][MBAddr].RefIdxs[15];
                if (0 <= index)
                {
                    pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 1)->m_RefPicList;
                    iRefQBck2 = pRefPicList[index]->DeblockPicID(0);
                }
                else
                    iRefQBck2 = -1;
            }

            // when reference indexes are equal
            if (((iRefQFrw == iRefQFrw2) && (iRefQBck == iRefQBck2)) ||
                ((iRefQFrw == iRefQBck2) && (iRefQBck == iRefQFrw2)))
            {
                // set initial value of strength
                nStrength = 0;

                // when forward and backward reference pictures of previous block are different
                if (iRefQFrw2 != iRefQBck2)
                {
                    H264DecoderMotionVector *pVectorQFrw2, *pVectorQBck2;

                    // select previous block motion vectors
                    if (iRefQFrw == iRefQFrw2)
                    {
                        pVectorQFrw2 = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + 15;
                        pVectorQBck2 = m_pCurrentFrame->m_mbinfo.MV[1][MBAddr].MotionVectors + 15;
                    }
                    else
                    {
                        pVectorQFrw2 = m_pCurrentFrame->m_mbinfo.MV[1][MBAddr].MotionVectors + 15;
                        pVectorQBck2 = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + 15;
                    }

                    // compare motion vectors
                    if ((4 <= abs(pVectorQFrw->mvx - pVectorQFrw2->mvx)) ||
                        (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorQFrw2->mvy)) ||
                        (4 <= abs(pVectorQBck->mvx - pVectorQBck2->mvx)) ||
                        (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorQBck2->mvy)))
                    {
                        nStrength = 1;
                        *pDeblockingFlag = 1;
                    }
                }
                // when forward and backward reference pictures of previous block are equal
                else
                {
                    H264DecoderMotionVector *pVectorQFrw2, *pVectorQBck2;

                    // select block second motion vectors
                    pVectorQFrw2 = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + 15;
                    pVectorQBck2 = m_pCurrentFrame->m_mbinfo.MV[1][MBAddr].MotionVectors + 15;

                    // compare motion vectors
                    if ((4 <= abs(pVectorQFrw->mvx - pVectorQFrw2->mvx)) ||
                        (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorQFrw2->mvy)) ||
                        (4 <= abs(pVectorQBck->mvx - pVectorQBck2->mvx)) ||
                        (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorQBck2->mvy)))
                    {
                        if ((4 <= abs(pVectorQFrw->mvx - pVectorQBck2->mvx)) ||
                            (nVectorDiffLimit <= abs(pVectorQFrw->mvy - pVectorQBck2->mvy)) ||
                            (4 <= abs(pVectorQBck->mvx - pVectorQFrw2->mvx)) ||
                            (nVectorDiffLimit <= abs(pVectorQBck->mvy - pVectorQFrw2->mvy)))
                        {
                            nStrength = 1;
                            *pDeblockingFlag = 1;
                        }
                    }
                }
            }
            // when reference indexes are different
            else
            {
                nStrength = 1;
                *pDeblockingFlag = 1;
            }

            // cicle of edge(s)
            for (idx = 8;idx < 12;idx += 1)
            {
                Ipp32u blkQ;

                blkQ = INTERNAL_BLOCKS_MASK[dir][idx - 4];

                if (cbp4x4_luma & blkQ)
                {
                    pStrength[idx] = 2;
                    *pDeblockingFlag = 1;
                }
                // we have compared motion vectors
                else
                    pStrength[idx] = (Ipp8u) nStrength;
            }
        }

        // cicle of edge(s)
        for (idx = 12;idx < 16;idx += 1)
        {
            Ipp32u blkQ;

            blkQ = INTERNAL_BLOCKS_MASK[dir][idx - 4];

            if (cbp4x4_luma & blkQ)
            {
                pStrength[idx] = 2;
                *pDeblockingFlag = 1;
            }
            // we haven't to compare motion vectors  - they are equal
            else
                pStrength[idx] = 0;
        }
    }

} // void H264SegmentDecoder::PrepareDeblockingParametersBSlice16x8(Ipp32u dir, DeblockingParameters *pParams)

void H264SegmentDecoder::PrepareDeblockingParametersBSlice8x16(Ipp32u dir, DeblockingParameters *pParams)
{
    Ipp32u MBAddr = pParams->nMBAddr;
    Ipp32u cbp4x4_luma = (m_mbinfo.mbs + MBAddr)->cbp4x4_luma;
    Ipp8u *pStrength = pParams->Strength[dir];
    Ipp32u *pDeblockingFlag = &(pParams->DeblockingFlag[dir]);

    //
    // external edge
    //
    if (pParams->ExternalEdgeFlag[dir])
    {
        Ipp32u nNeighbour;

        // select neighbour addres
        nNeighbour = pParams->nNeighbour[dir];

        // when neighbour macroblock isn't intra
        if (!IS_INTRA_MBTYPE((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->mbtype))
        {
            H264DecoderMacroblockLocalInfo *pNeighbour;
            Ipp32u idx;
            Ipp32s iRefQFrw[2], iRefQBck[2];
            H264DecoderMotionVector *(pVectorQFrw[2]), *(pVectorQBck[2]);

            // in following calculations we avoided multiplication on 15
            // by using formulae a * 15 = a * 16 - a

            // load reference indexes for current block
            for (idx = 0;idx < 2;idx += 1)
            {
                {
                    H264VideoDecoder::H264DecoderFrame **pRefPicList;
                    Ipp32s index;

                    // select reference index for current block
                    index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][MBAddr].RefIdxs[idx * 16 - idx];
                    if (0 <= index)
                    {
                        pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 0)->m_RefPicList;
                        iRefQFrw[idx] = pRefPicList[index]->DeblockPicID(0);
                    }
                    else
                        iRefQFrw[idx] = -1;
                    index = m_pCurrentFrame->m_mbinfo.RefIdxs[1][MBAddr].RefIdxs[idx * 16 - idx];
                    if (0 <= index)
                    {
                        pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + MBAddr)->slice_id, 1)->m_RefPicList;
                        iRefQBck[idx] = pRefPicList[index]->DeblockPicID(0);
                    }
                    else
                        iRefQBck[idx] = -1;
                }

                // select current block motion vectors
                pVectorQFrw[idx] = m_pCurrentFrame->m_mbinfo.MV[0][MBAddr].MotionVectors + (idx * 16 - idx);
                pVectorQBck[idx] = m_pCurrentFrame->m_mbinfo.MV[1][MBAddr].MotionVectors + (idx * 16 - idx);
            }

            // select neighbour
            pNeighbour = m_mbinfo.mbs + nNeighbour;

            // cicle on blocks
            for (idx = 0;idx < 4;idx += 1)
            {
                Ipp32u blkQ, blkP;

                blkQ = EXTERNAL_BLOCK_MASK[dir][CURRENT_BLOCK][idx];
                blkP = EXTERNAL_BLOCK_MASK[dir][NEIGHBOUR_BLOCK][idx];

                // when one of couple of blocks has coeffs
                if ((cbp4x4_luma & blkQ) ||
                    (pNeighbour->cbp4x4_luma & blkP))
                {
                    pStrength[idx] = 2;
                    *pDeblockingFlag = 1;
                }
                // compare motion vectors & reference indexes
                else
                {
                    Ipp32u nNeighbourBlock;
                    Ipp32s iRefPFrw, iRefPBck;
                    Ipp32s nVectorDiffLimit = pParams->nMaxMVector;

                    // calc block and neighbour block number
                    if (VERTICAL_DEBLOCKING == dir)
                        nNeighbourBlock = idx * 4 + 3;
                    else
                        nNeighbourBlock = idx + 12;

                    // field coded image
                    {
                        H264VideoDecoder::H264DecoderFrame **pRefPicList;
                        Ipp32s index;

                        // select reference index for previous block
                        index = m_pCurrentFrame->m_mbinfo.RefIdxs[0][nNeighbour].RefIdxs[nNeighbourBlock];
                        if (0 <= index)
                        {
                            pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 0)->m_RefPicList;
                            iRefPFrw = pRefPicList[index]->DeblockPicID(0);
                        }
                        else
                            iRefPFrw = -1;
                        index = m_pCurrentFrame->m_mbinfo.RefIdxs[1][nNeighbour].RefIdxs[nNeighbourBlock];
                        if (0 <= index)
                        {
                            pRefPicList = m_pCurrentFrame->GetRefPicList((m_pCurrentFrame->m_mbinfo.mbs + nNeighbour)->slice_id, 1)->m_RefPicList;
                            iRefPBck = pRefPicList[index]->DeblockPicID(0);
                        }
                        else
                            iRefPBck = -1;
                    }

                    // when reference indexes are equal
                    if (((iRefQFrw[idx / 2] == iRefPFrw) && (iRefQBck[idx / 2] == iRefPBck)) ||
                        ((iRefQFrw[idx / 2] == iRefPBck) && (iRefQBck[idx / 2] == iRefPFrw)))
                    {
                        // set initial value of strength
                        pStrength[idx] = 0;

                        // when forward and backward reference pictures of previous block are different
                        if (iRefPFrw != iRefPBck)
                        {
                            H264DecoderMotionVector *pVectorPFrw, *pVectorPBck;

                            // select previous block motion vectors
                            if (iRefQFrw[idx / 2] == iRefPFrw)
                            {
                                pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;
                                pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[1][nNeighbour].MotionVectors + nNeighbourBlock;
                            }
                            else
                            {
                                pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[1][nNeighbour].MotionVectors + nNeighbourBlock;
                                pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;
                            }

                            // compare motion vectors
                            if ((4 <= abs(pVectorQFrw[idx / 2]->mvx - pVectorPFrw->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQFrw[idx / 2]->mvy - pVectorPFrw->mvy)) ||
                                (4 <= abs(pVectorQBck[idx / 2]->mvx - pVectorPBck->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQBck[idx / 2]->mvy - pVectorPBck->mvy)))
                            {
                                pStrength[idx] = 1;
                                *pDeblockingFlag = 1;
                            }
                        }
                        // when forward and backward reference pictures of previous block are equal
                        else
                        {
                            H264DecoderMotionVector *pVectorPFrw, *pVectorPBck;

                            // select previous block motion vectors
                            pVectorPFrw = m_pCurrentFrame->m_mbinfo.MV[0][nNeighbour].MotionVectors + nNeighbourBlock;
                            pVectorPBck = m_pCurrentFrame->m_mbinfo.MV[1][nNeighbour].MotionVectors + nNeighbourBlock;

                            // compare motion vectors
                            if ((4 <= abs(pVectorQFrw[idx / 2]->mvx - pVectorPFrw->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQFrw[idx / 2]->mvy - pVectorPFrw->mvy)) ||
                                (4 <= abs(pVectorQBck[idx / 2]->mvx - pVectorPBck->mvx)) ||
                                (nVectorDiffLimit <= abs(pVectorQBck[idx / 2]->mvy - pVectorPBck->mvy)))
                            {
                                if ((4 <= abs(pVectorQFrw[idx / 2]->mvx - pVectorPBck->mvx)) ||
                                    (nVectorDiffLimit <= abs(pVectorQFrw[idx / 2]->mvy - pVectorPBck->mvy)) ||
                                    (4 <= abs(pVectorQBck[idx / 2]->mvx - pVectorPFrw->mvx)) ||
                                    (nVectorDiffLimit <= abs(pVectorQBck[idx / 2]->mvy - pVectorPFrw->mvy)))
                                {
                                    pStrength[idx] = 1;
                                    *pDeblockingFlag = 1;
                                }
                            }
                        }
                    }
                    // when reference indexes are different
                    else
                    {
                        pStrength[idx] = 1;
                        *pDeblockingFlag = 1;
                    }
                }
            }
        }
        // external edge required in strong filtering
        else
        {
            SetEdgeStrength(pStrength + 0, 4);
            *pDeblockingFlag = 1;
        }
    }

    //
    // internal edge(s)
    //
    {
        Ipp32u idx;

        // reset all strengths
        SetEdgeStrength(pStrength + 4, 0);
        SetEdgeStrength(pStrength + 8, 0);
        SetEdgeStrength(pStrength + 12, 0);

        // set deblocking flag
        if (cbp4x4_luma & 0x1fffe)
            *pDeblockingFlag = 1;

        // cicle of edge(s)
        // we do all edges in one cicle
        for (idx = 4;idx < 16;idx += 1)
        {
            Ipp32u blkQ;

            blkQ = INTERNAL_BLOCKS_MASK[dir][idx - 4];

            if (cbp4x4_luma & blkQ)
                pStrength[idx] = 2;
        }
    }

} // void H264SegmentDecoder::PrepareDeblockingParametersBSlice8x16(Ipp32u dir, DeblockingParameters *pParams)

} // namespace UMC
