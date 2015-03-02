/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2001-2006 Intel Corporation. All Rights Reserved.
//
//  Description:    Decodes MPEG-4 bitstream.
//
*/

//***
#include "kinoma_ipp_lib.h"

#include "mp4.h"
#include "mp4dec.h"

//#pragma warning(disable : 188)  // enumerated type mixed with another type ICL

/*
//  mp4_Info for decoding of Video Object Layer
*/
mp4_Status mp4_InitVOL(mp4_Info* pInfo)
{
    int mbPerRow, mbPerCol, specSize;

    pInfo->VisualObject.VideoObject.VOPindex = 0;
    if (pInfo->VisualObject.VideoObject.shape == MP4_SHAPE_TYPE_RECTANGULAR) {
        mbPerRow = (pInfo->VisualObject.VideoObject.width + 15) >> 4;
        mbPerCol = (pInfo->VisualObject.VideoObject.height + 15) >> 4;
        // current frame
        pInfo->VisualObject.cFrame.stepY = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 4;
        pInfo->VisualObject.cFrame.apY = ippsMalloc_8u_x(pInfo->VisualObject.cFrame.stepY * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 4));
        if (!pInfo->VisualObject.cFrame.apY) return MP4_STATUS_NO_MEM;
        pInfo->VisualObject.cFrame.pY = pInfo->VisualObject.cFrame.apY + pInfo->VisualObject.cFrame.stepY * 16 + 16;
        pInfo->VisualObject.cFrame.stepCb = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 3;
        pInfo->VisualObject.cFrame.apCb = ippsMalloc_8u_x(pInfo->VisualObject.cFrame.stepCb * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 3));
        if (!pInfo->VisualObject.cFrame.apCb) return MP4_STATUS_NO_MEM;
        pInfo->VisualObject.cFrame.pCb = pInfo->VisualObject.cFrame.apCb + pInfo->VisualObject.cFrame.stepCb * 8 + 8;
        pInfo->VisualObject.cFrame.stepCr = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 3;
        pInfo->VisualObject.cFrame.apCr = ippsMalloc_8u_x(pInfo->VisualObject.cFrame.stepCr * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 3));
        if (!pInfo->VisualObject.cFrame.apCr) return MP4_STATUS_NO_MEM;
        pInfo->VisualObject.cFrame.pCr = pInfo->VisualObject.cFrame.apCr + pInfo->VisualObject.cFrame.stepCr * 8 + 8;
		if (pInfo->VisualObject.VideoObject.sprite_enable != MP4_SPRITE_STATIC) {
            // ref in past frame
            pInfo->VisualObject.rFrame.stepY = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 4;
            pInfo->VisualObject.rFrame.apY = ippsMalloc_8u_x(pInfo->VisualObject.rFrame.stepY * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 4));
            if (!pInfo->VisualObject.rFrame.apY) return MP4_STATUS_NO_MEM;
            pInfo->VisualObject.rFrame.pY = pInfo->VisualObject.rFrame.apY + pInfo->VisualObject.rFrame.stepY * 16 + 16;
            pInfo->VisualObject.rFrame.stepCb = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 3;
            pInfo->VisualObject.rFrame.apCb = ippsMalloc_8u_x(pInfo->VisualObject.rFrame.stepCb * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 3));
            if (!pInfo->VisualObject.rFrame.apCb) return MP4_STATUS_NO_MEM;
            pInfo->VisualObject.rFrame.pCb = pInfo->VisualObject.rFrame.apCb + pInfo->VisualObject.rFrame.stepCb * 8 + 8;
            pInfo->VisualObject.rFrame.stepCr = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 3;
            pInfo->VisualObject.rFrame.apCr = ippsMalloc_8u_x(pInfo->VisualObject.rFrame.stepCr * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 3));
            if (!pInfo->VisualObject.rFrame.apCr) return MP4_STATUS_NO_MEM;
            pInfo->VisualObject.rFrame.pCr = pInfo->VisualObject.rFrame.apCr + pInfo->VisualObject.rFrame.stepCr * 8 + 8;
            // ref in future frame
            pInfo->VisualObject.nFrame.stepY = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 4;
            pInfo->VisualObject.nFrame.apY = ippsMalloc_8u_x(pInfo->VisualObject.nFrame.stepY * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 4));
            if (!pInfo->VisualObject.nFrame.apY) return MP4_STATUS_NO_MEM;
            pInfo->VisualObject.nFrame.pY = pInfo->VisualObject.nFrame.apY + pInfo->VisualObject.nFrame.stepY * 16 + 16;
            pInfo->VisualObject.nFrame.stepCb = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 3;
            pInfo->VisualObject.nFrame.apCb = ippsMalloc_8u_x(pInfo->VisualObject.nFrame.stepCb * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 3));
            if (!pInfo->VisualObject.nFrame.apCb) return MP4_STATUS_NO_MEM;
            pInfo->VisualObject.nFrame.pCb = pInfo->VisualObject.nFrame.apCb + pInfo->VisualObject.nFrame.stepCb * 8 + 8;
            pInfo->VisualObject.nFrame.stepCr = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 3;
            pInfo->VisualObject.nFrame.apCr = ippsMalloc_8u_x(pInfo->VisualObject.nFrame.stepCr * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 3));
            if (!pInfo->VisualObject.nFrame.apCr) return MP4_STATUS_NO_MEM;
            pInfo->VisualObject.nFrame.pCr = pInfo->VisualObject.nFrame.apCr + pInfo->VisualObject.nFrame.stepCr * 8 + 8;
            // motion info (not needed for Sprites)
            pInfo->VisualObject.VideoObject.MBinfo = (mp4_MacroBlock*)ippsMalloc_8u_x(mbPerRow*mbPerCol*sizeof(mp4_MacroBlock));
            if (!pInfo->VisualObject.VideoObject.MBinfo) return MP4_STATUS_NO_MEM;
#ifdef USE_NOTCODED_STATE
            // not_coded MB state
            pInfo->VisualObject.VideoObject.ncState = ippsMalloc_8u_x(mbPerCol * mbPerRow);
            if (!pInfo->VisualObject.VideoObject.ncState) return MP4_STATUS_NO_MEM;
            pInfo->VisualObject.VideoObject.ncStateCleared = 0;
#endif
        } 
#ifndef DROP_SPRITE
		else { // data for static sprite
            mbPerRow = pInfo->VisualObject.sFrame.mbPerRow = (pInfo->VisualObject.VideoObject.sprite_width + 15) >> 4;
            mbPerCol = pInfo->VisualObject.sFrame.mbPerCol = (pInfo->VisualObject.VideoObject.sprite_height + 15) >> 4;
            pInfo->VisualObject.sFrame.stepY = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 4;
            pInfo->VisualObject.sFrame.apY = ippsMalloc_8u_x(pInfo->VisualObject.sFrame.stepY * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 4));
            if (!pInfo->VisualObject.sFrame.apY) return MP4_STATUS_NO_MEM;
            pInfo->VisualObject.sFrame.pY = pInfo->VisualObject.sFrame.apY + pInfo->VisualObject.sFrame.stepY * 16 + 16;
            pInfo->VisualObject.sFrame.stepCb = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 3;
            pInfo->VisualObject.sFrame.apCb = ippsMalloc_8u_x(pInfo->VisualObject.sFrame.stepCb * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 3));
            if (!pInfo->VisualObject.sFrame.apCb) return MP4_STATUS_NO_MEM;
            pInfo->VisualObject.sFrame.pCb = pInfo->VisualObject.sFrame.apCb + pInfo->VisualObject.sFrame.stepCb * 8 + 8;
            pInfo->VisualObject.sFrame.stepCr = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 3;
            pInfo->VisualObject.sFrame.apCr = ippsMalloc_8u_x(pInfo->VisualObject.sFrame.stepCr * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 3));
            if (!pInfo->VisualObject.sFrame.apCr) return MP4_STATUS_NO_MEM;
            pInfo->VisualObject.sFrame.pCr = pInfo->VisualObject.sFrame.apCr + pInfo->VisualObject.sFrame.stepCr * 8 + 8;
        }
#endif
        pInfo->VisualObject.VideoObject.MacroBlockPerRow = mbPerRow;
        pInfo->VisualObject.VideoObject.MacroBlockPerCol = mbPerCol;
        pInfo->VisualObject.VideoObject.MacroBlockPerVOP = mbPerRow * mbPerCol;
        pInfo->VisualObject.VideoObject.mbns = mp4_GetMacroBlockNumberSize(pInfo->VisualObject.VideoObject.MacroBlockPerVOP);
#ifdef _OMP_KARABAS
        pInfo->num_threads = mp4_GetNumOfThreads();
        pInfo->pMBinfoMT = (mp4_MacroBlockMT*)ippsMalloc_8u_x(mbPerRow * pInfo->num_threads * sizeof(mp4_MacroBlockMT));
        if(!pInfo->pMBinfoMT) return MP4_STATUS_NO_MEM;
#endif // _OMP_KARABAS

        if (!pInfo->VisualObject.VideoObject.short_video_header) {
#ifdef SUPPORT_H263_ONLY 
			return MP4_STATUS_FILE_ERROR;
#else 
            // Intra Prediction info (not needed for SVH)
            pInfo->VisualObject.VideoObject.IntraPredBuff.quant = ippsMalloc_8u_x(mbPerRow + 1);
            if (!pInfo->VisualObject.VideoObject.IntraPredBuff.quant) return MP4_STATUS_NO_MEM;
            pInfo->VisualObject.VideoObject.IntraPredBuff.block = (mp4_IntraPredBlock*)ippsMalloc_8u_x((mbPerRow + 1)*6*sizeof(mp4_IntraPredBlock));
            if (!pInfo->VisualObject.VideoObject.IntraPredBuff.block) return MP4_STATUS_NO_MEM;
            {
                mp4_IntraPredBlock *mbCurr = pInfo->VisualObject.VideoObject.IntraPredBuff.block;
                mp4_IntraPredBlock *mbA = mbCurr, *mbB = pInfo->VisualObject.VideoObject.IntraPredBuff.dcB, *mbC = mbCurr + 6;
                int                 j;

                for (j = 0; j < mbPerRow; j ++) {
                    mbCurr[0].predA = &mbA[1];  mbCurr[0].predB = &mbB[3];  mbCurr[0].predC = &mbC[2];
                    mbCurr[1].predA = &mbC[0];  mbCurr[1].predB = &mbC[2];  mbCurr[1].predC = &mbC[3];
                    mbCurr[2].predA = &mbA[3];  mbCurr[2].predB = &mbA[1];  mbCurr[2].predC = &mbC[0];
                    mbCurr[3].predA = &mbC[2];  mbCurr[3].predB = &mbC[0];  mbCurr[3].predC = &mbC[1];
                    mbCurr[4].predA = &mbA[4];  mbCurr[4].predB = &mbB[4];  mbCurr[4].predC = &mbC[4];
                    mbCurr[5].predA = &mbA[5];  mbCurr[5].predB = &mbB[5];  mbCurr[5].predC = &mbC[5];
                    mbCurr += 6;  mbA += 6;  mbC += 6;
                }
            }
            if (pInfo->VisualObject.VideoObject.data_partitioned) {
                // DataPart info
                pInfo->VisualObject.VideoObject.DataPartBuff = (mp4_DataPartMacroBlock*)ippsMalloc_8u_x(mbPerRow*mbPerCol*sizeof(mp4_DataPartMacroBlock));
                if (!pInfo->VisualObject.VideoObject.DataPartBuff) return MP4_STATUS_NO_MEM;
            }
#ifndef DROP_FIELD
            if (pInfo->VisualObject.VideoObject.interlaced) {
                // Field MV for B-VOP
                pInfo->VisualObject.VideoObject.FieldMV = (IppMotionVector*)ippsMalloc_8u_x(mbPerRow*mbPerCol*sizeof(IppMotionVector)*2);
                if (!pInfo->VisualObject.VideoObject.FieldMV) return MP4_STATUS_NO_MEM;
            }
#endif
            ippiQuantInvIntraGetSize_MPEG4_x(&specSize);
            pInfo->VisualObject.VideoObject.QuantInvIntraSpec = (IppiQuantInvIntraSpec_MPEG4*)ippsMalloc_8u_x(specSize);
            ippiQuantInvIntraInit_MPEG4_x(pInfo->VisualObject.VideoObject.quant_type ? pInfo->VisualObject.VideoObject.intra_quant_mat : NULL, pInfo->VisualObject.VideoObject.QuantInvIntraSpec, 8);
            ippiQuantInvInterGetSize_MPEG4_x(&specSize);
            pInfo->VisualObject.VideoObject.QuantInvInterSpec = (IppiQuantInvInterSpec_MPEG4*)ippsMalloc_8u_x(specSize);
            ippiQuantInvInterInit_MPEG4_x(pInfo->VisualObject.VideoObject.quant_type ? pInfo->VisualObject.VideoObject.nonintra_quant_mat : NULL, pInfo->VisualObject.VideoObject.QuantInvInterSpec, 8);
#ifndef DROP_GMC
			ippiWarpGetSize_MPEG4_x(&specSize);
            pInfo->VisualObject.VideoObject.WarpSpec = (IppiWarpSpec_MPEG4*)ippsMalloc_8u_x(specSize);
#endif
        
#endif
        }         
    }
    return MP4_STATUS_OK;
}


/*
//  Free memory allocated for mp4_Info
*/
mp4_Status mp4_FreeVOL(mp4_Info* pInfo)
{
    pInfo->VisualObject.VideoObject.VOPindex = 0;
    if (pInfo->VisualObject.VideoObject.shape == MP4_SHAPE_TYPE_RECTANGULAR) {
        ippsFree_x(pInfo->VisualObject.cFrame.apY);  pInfo->VisualObject.cFrame.apY  = pInfo->VisualObject.cFrame.pY  = NULL;
        ippsFree_x(pInfo->VisualObject.cFrame.apCb); pInfo->VisualObject.cFrame.apCb = pInfo->VisualObject.cFrame.pCb = NULL;
        ippsFree_x(pInfo->VisualObject.cFrame.apCr); pInfo->VisualObject.cFrame.apCr = pInfo->VisualObject.cFrame.pCr = NULL;
        if (pInfo->VisualObject.VideoObject.sprite_enable != MP4_SPRITE_STATIC) {
            ippsFree_x(pInfo->VisualObject.rFrame.apY);  pInfo->VisualObject.rFrame.apY  = pInfo->VisualObject.rFrame.pY  = NULL;
            ippsFree_x(pInfo->VisualObject.rFrame.apCb); pInfo->VisualObject.rFrame.apCb = pInfo->VisualObject.rFrame.pCb = NULL;
            ippsFree_x(pInfo->VisualObject.rFrame.apCr); pInfo->VisualObject.rFrame.apCr = pInfo->VisualObject.rFrame.pCr = NULL;
            ippsFree_x(pInfo->VisualObject.nFrame.apY);  pInfo->VisualObject.nFrame.apY  = pInfo->VisualObject.nFrame.pY  = NULL;
            ippsFree_x(pInfo->VisualObject.nFrame.apCb); pInfo->VisualObject.nFrame.apCb = pInfo->VisualObject.nFrame.pCb = NULL;
            ippsFree_x(pInfo->VisualObject.nFrame.apCr); pInfo->VisualObject.nFrame.apCr = pInfo->VisualObject.nFrame.pCr = NULL;
            ippsFree_x(pInfo->VisualObject.VideoObject.MBinfo); pInfo->VisualObject.VideoObject.MBinfo = NULL;
#ifdef USE_NOTCODED_STATE
            ippsFree_x(pInfo->VisualObject.VideoObject.ncState);
#endif
        } 
#ifndef DROP_SPRITE		
		else {
            ippsFree_x(pInfo->VisualObject.sFrame.apY);  pInfo->VisualObject.sFrame.apY  = pInfo->VisualObject.sFrame.pY  = NULL;
            ippsFree_x(pInfo->VisualObject.sFrame.apCb); pInfo->VisualObject.sFrame.apCb = pInfo->VisualObject.sFrame.pCb = NULL;
            ippsFree_x(pInfo->VisualObject.sFrame.apCr); pInfo->VisualObject.sFrame.apCr = pInfo->VisualObject.sFrame.pCr = NULL;
        }
#endif

        if (!pInfo->VisualObject.VideoObject.short_video_header) {
#ifdef SUPPORT_H263_ONLY 
			return MP4_STATUS_FILE_ERROR;
#else        
            ippsFree_x(pInfo->VisualObject.VideoObject.IntraPredBuff.quant); pInfo->VisualObject.VideoObject.IntraPredBuff.quant = NULL;
            ippsFree_x(pInfo->VisualObject.VideoObject.IntraPredBuff.block); pInfo->VisualObject.VideoObject.IntraPredBuff.block = NULL;
            if (pInfo->VisualObject.VideoObject.data_partitioned) {
                ippsFree_x(pInfo->VisualObject.VideoObject.DataPartBuff); pInfo->VisualObject.VideoObject.DataPartBuff = NULL;
            }
#ifndef DROP_FIELD
            if (pInfo->VisualObject.VideoObject.interlaced) {
                ippsFree_x(pInfo->VisualObject.VideoObject.FieldMV); pInfo->VisualObject.VideoObject.FieldMV = NULL;
            }
#endif
            ippsFree_x(pInfo->VisualObject.VideoObject.QuantInvIntraSpec); pInfo->VisualObject.VideoObject.QuantInvIntraSpec = NULL;
            ippsFree_x(pInfo->VisualObject.VideoObject.QuantInvInterSpec); pInfo->VisualObject.VideoObject.QuantInvInterSpec = NULL;
#ifndef DROP_GMC
			ippsFree_x(pInfo->VisualObject.VideoObject.WarpSpec); pInfo->VisualObject.VideoObject.WarpSpec = NULL;
#endif
		
#endif		
		} 
		
#ifdef _OMP_KARABAS
        ippsFree_x(pInfo->pMBinfoMT); pInfo->pMBinfoMT = NULL;
#endif // _OMP_KARABAS
    }
#ifdef SUPPORT_H263_ONLY 
	else return MP4_STATUS_FILE_ERROR;
#else       
    else {
        // free current
        mp4_FreeVOPShape(pInfo);
        mp4_SWAP(mp4_Frame, pInfo->VisualObject.rFrame, pInfo->VisualObject.cFrame);
        // free ref
        mp4_FreeVOPShape(pInfo);
        mp4_SWAP(mp4_Frame, pInfo->VisualObject.nFrame, pInfo->VisualObject.cFrame);
        // free future
        mp4_FreeVOPShape(pInfo);
#ifndef DROP_SPRITE
        if (pInfo->VisualObject.VideoObject.sprite_enable == MP4_SPRITE_STATIC) {
            // free sprite
            mp4_SWAP(mp4_Frame, pInfo->VisualObject.sFrame, pInfo->VisualObject.cFrame);
            mp4_FreeVOPShape(pInfo);
        }
#endif
    }
#endif    
    
    return MP4_STATUS_OK;
}


#ifndef SUPPORT_H263_ONLY 
/*
//  Allocate memory for decoding Shape VOP
*/
mp4_Status mp4_InitVOPShape(mp4_Info* pInfo)
{
    int mbPerRow, mbPerCol, i, reAlloc;

#ifndef DROP_SPRITE
    if (pInfo->VisualObject.VideoObject.sprite_enable == MP4_SPRITE_STATIC) {
        mbPerRow = (pInfo->VisualObject.VideoObject.sprite_width + 15) >> 4;
        mbPerCol = (pInfo->VisualObject.VideoObject.sprite_height + 15) >> 4;
    } 
	else 
#endif	
	{
        mbPerRow = (pInfo->VisualObject.VideoObject.VideoObjectPlane.vop_width + 15) >> 4;
        mbPerCol = (pInfo->VisualObject.VideoObject.VideoObjectPlane.vop_height + 15) >> 4;
    }
    pInfo->VisualObject.VideoObject.MacroBlockPerRow = mbPerRow;
    pInfo->VisualObject.VideoObject.MacroBlockPerCol = mbPerCol;
    pInfo->VisualObject.VideoObject.MacroBlockPerVOP = mbPerRow * mbPerCol;
    pInfo->VisualObject.VideoObject.mbns = mp4_GetMacroBlockNumberSize(pInfo->VisualObject.VideoObject.MacroBlockPerVOP);
    reAlloc = (mbPerRow + 2 * MP4_NUM_EXT_MB) * (mbPerCol + 2 * MP4_NUM_EXT_MB) > (pInfo->VisualObject.cFrame.mbPerRow + 2 * MP4_NUM_EXT_MB) * (pInfo->VisualObject.cFrame.mbPerCol + 2 * MP4_NUM_EXT_MB);
    if (reAlloc) {
        mp4_FreeVOPShape(pInfo);
        pInfo->VisualObject.cFrame.mbPerRow = mbPerRow;
        pInfo->VisualObject.cFrame.mbPerCol = mbPerCol;
        pInfo->VisualObject.cFrame.stepY = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 4;
        pInfo->VisualObject.cFrame.apY = ippsMalloc_8u_x(pInfo->VisualObject.cFrame.stepY * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 4));
        if (!pInfo->VisualObject.cFrame.apY) return MP4_STATUS_NO_MEM;
        pInfo->VisualObject.cFrame.pY = pInfo->VisualObject.cFrame.apY + pInfo->VisualObject.cFrame.stepY * 16 + 16;
        pInfo->VisualObject.cFrame.stepCb = pInfo->VisualObject.cFrame.stepCr = (mbPerRow + 2 * MP4_NUM_EXT_MB) << 3;
        pInfo->VisualObject.cFrame.apCb = ippsMalloc_8u_x(pInfo->VisualObject.cFrame.stepCb * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 3));
        if (!pInfo->VisualObject.cFrame.apCb) return MP4_STATUS_NO_MEM;
        pInfo->VisualObject.cFrame.apCr = ippsMalloc_8u_x(pInfo->VisualObject.cFrame.stepCr * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 3));
        if (!pInfo->VisualObject.cFrame.apCr) return MP4_STATUS_NO_MEM;
        pInfo->VisualObject.cFrame.pCb = pInfo->VisualObject.cFrame.apCb + pInfo->VisualObject.cFrame.stepCb * 8 + 8;
        pInfo->VisualObject.cFrame.pCr = pInfo->VisualObject.cFrame.apCr + pInfo->VisualObject.cFrame.stepCr * 8 + 8;
        pInfo->VisualObject.VideoObject.MBinfo = (mp4_MacroBlock*)ippsMalloc_8u_x(mbPerRow*mbPerCol*sizeof(mp4_MacroBlock));
        if (!pInfo->VisualObject.VideoObject.MBinfo) return MP4_STATUS_NO_MEM;
//f account must be taken of Sprite !!!
        pInfo->VisualObject.VideoObject.ShapeInfo = (mp4_ShapeInfo*)ippsMalloc_8u_x(mbPerRow*mbPerCol*sizeof(mp4_ShapeInfo));
        if (!pInfo->VisualObject.VideoObject.ShapeInfo) return MP4_STATUS_NO_MEM;
        pInfo->VisualObject.cFrame.apB = ippsMalloc_8u_x(pInfo->VisualObject.cFrame.stepY * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 4));
        if (!pInfo->VisualObject.cFrame.apB) return MP4_STATUS_NO_MEM;
        pInfo->VisualObject.cFrame.pB = pInfo->VisualObject.cFrame.apB + pInfo->VisualObject.cFrame.stepY * 16 + 16;
ippsZero_8u_x(pInfo->VisualObject.cFrame.apB, pInfo->VisualObject.cFrame.stepY * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 4));
        if (pInfo->VisualObject.VideoObject.shape == MP4_SHAPE_TYPE_GRAYSCALE) {
            for (i = 0; i < mp4_aux_comp_count[pInfo->VisualObject.VideoObject.shape_extension]; i ++) {
                pInfo->VisualObject.cFrame.apA[i] = ippsMalloc_8u_x(pInfo->VisualObject.cFrame.stepY * ((mbPerCol + 2 * MP4_NUM_EXT_MB) << 4));
                if (!pInfo->VisualObject.cFrame.apA[i]) return MP4_STATUS_NO_MEM;
                pInfo->VisualObject.cFrame.pA[i] = pInfo->VisualObject.cFrame.apA[i] + pInfo->VisualObject.cFrame.stepY * 16 + 16;
            }
        }
    }
    return MP4_STATUS_OK;
}


/*
//  Free memory allocated for Shape VOP
*/
mp4_Status mp4_FreeVOPShape(mp4_Info* pInfo)
{
    ippsFree_x(pInfo->VisualObject.cFrame.apY);  pInfo->VisualObject.cFrame.apY  = pInfo->VisualObject.cFrame.pY  = NULL;
    ippsFree_x(pInfo->VisualObject.cFrame.apCb); pInfo->VisualObject.cFrame.apCb = pInfo->VisualObject.cFrame.pCb = NULL;
    ippsFree_x(pInfo->VisualObject.cFrame.apCr); pInfo->VisualObject.cFrame.apCr = pInfo->VisualObject.cFrame.pCr = NULL;
    ippsFree_x(pInfo->VisualObject.VideoObject.MBinfo); pInfo->VisualObject.VideoObject.MBinfo = NULL;

    ippsFree_x(pInfo->VisualObject.VideoObject.ShapeInfo); pInfo->VisualObject.VideoObject.ShapeInfo = NULL;
    ippsFree_x(pInfo->VisualObject.cFrame.apB);  pInfo->VisualObject.cFrame.apB = pInfo->VisualObject.cFrame.pB = NULL;
    if (pInfo->VisualObject.VideoObject.shape == MP4_SHAPE_TYPE_GRAYSCALE) {
        int  i;
        for (i = 0; i < mp4_aux_comp_count[pInfo->VisualObject.VideoObject.shape_extension]; i ++) {
            ippsFree_x(pInfo->VisualObject.cFrame.apA[i]);  pInfo->VisualObject.cFrame.apA[i] = pInfo->VisualObject.cFrame.pA[i] = NULL;
        }
    }
    return MP4_STATUS_OK;
}

#endif

mp4_Status mp4_DecodeMVD(mp4_Info *pInfo, int *mvdx, int *mvdy, int fcode)
{
    const mp4_VLC1 *pTab;
    int             mvd, sign;
    Ipp32u          code;
    int             factor = fcode - 1;

    /* decode MVDx */
    code = mp4_ShowBits(pInfo, 12);
    if (code >= 128)
        pTab = mp4_MVD_B12_2 + ((code - 128) >> 5);
    else if (code >= 2)
        pTab = mp4_MVD_B12_1 + (code - 2);
    else
        return MP4_STATUS_ERROR;
    mvd = pTab->code;
    mp4_FlushBits(pInfo, pTab->len);
    if (mvd) {
        sign = mp4_GetBit(pInfo);
        if (factor) {
            code = mp4_GetBits9(pInfo, factor);
            mvd = ((mvd - 1) << factor) + code + 1;
        }
        if (sign)
            mvd = -mvd;
    }
    *mvdx = mvd;
    /* decode MVDy */
    code = mp4_ShowBits(pInfo, 12);
    if (code >= 128)
        pTab = mp4_MVD_B12_2 + ((code - 128) >> 5);
    else if (code >= 2)
        pTab = mp4_MVD_B12_1 + (code - 2);
    else
        return MP4_STATUS_ERROR;
    mvd = pTab->code;
    mp4_FlushBits(pInfo, pTab->len);
    if (mvd) {
        sign = mp4_GetBit(pInfo);
        if (factor) {
            code = mp4_GetBits9(pInfo, factor);
            mvd = ((mvd - 1) << factor) + code + 1;
        }
        if (sign)
            mvd = -mvd;
    }
    *mvdy = mvd;
    return MP4_STATUS_OK;
}


mp4_Status mp4_DecodeMV(mp4_Info *pInfo, IppMotionVector *mv, int fcode)
{
    int  mvdx, mvdy, range, dx, dy;

    if (mp4_DecodeMVD(pInfo, &mvdx, &mvdy, fcode) != MP4_STATUS_OK)
        return MP4_STATUS_ERROR;
    range = 16 << fcode;
    dx = mv->dx + mvdx;
    if (dx < -range)
        dx = (dx + (range << 1));
    else if (dx >= range)
        dx = (dx - (range << 1));
    mv->dx = (Ipp16s)dx;
    dy = mv->dy + mvdy;
    if (dy < -range)
        dy = (dy + (range << 1));
    else if (dy >= range)
        dy = (dy - (range << 1));
    mv->dy = (Ipp16s)dy;
    return MP4_STATUS_OK;
}


mp4_Status mp4_Decode4MV(mp4_Info *pInfo, IppMotionVector *mv, int fcode)
{
    int  i, mvdx, mvdy, range, dx, dy;

    for (i = 0; i < 4; i ++) {
        if (mp4_DecodeMVD(pInfo, &mvdx, &mvdy, fcode) != MP4_STATUS_OK)
            return MP4_STATUS_ERROR;
        range = 16 << fcode;
        dx = mv[i].dx + mvdx;
        if (dx < -range)
            dx = (dx + (range << 1));
        else if (dx >= range)
            dx = (dx - (range << 1));
        mv[i].dx = (Ipp16s)dx;
        dy = mv[i].dy + mvdy;
        if (dy < -range)
            dy = (dy + (range << 1));
        else if (dy >= range)
            dy = (dy - (range << 1));
        mv[i].dy = (Ipp16s)dy;
    }
    return MP4_STATUS_OK;
}


mp4_Status mp4_DecodeMV_Direct(mp4_Info *pInfo, IppMotionVector mvC[4], IppMotionVector mvForw[4], IppMotionVector mvBack[4], int TRB, int TRD, int modb, int comb_type)
{
    int  mvdx, mvdy, i;

    if (modb == 2) {
        if (comb_type != IPPVC_MBTYPE_INTER4V) {
            mvForw[0].dx = mvForw[1].dx = mvForw[2].dx = mvForw[3].dx = (Ipp16s)((TRB * mvC[0].dx) / TRD);
            mvForw[0].dy = mvForw[1].dy = mvForw[2].dy = mvForw[3].dy = (Ipp16s)((TRB * mvC[0].dy) / TRD);
            mvBack[0].dx = mvBack[1].dx = mvBack[2].dx = mvBack[3].dx = (Ipp16s)(((TRB - TRD) * mvC[0].dx) / TRD);
            mvBack[0].dy = mvBack[1].dy = mvBack[2].dy = mvBack[3].dy = (Ipp16s)(((TRB - TRD) * mvC[0].dy) / TRD);
        } else
            for (i = 0; i < 4; i ++) {
                mvForw[i].dx = (Ipp16s)((TRB * mvC[i].dx) / TRD);
                mvForw[i].dy = (Ipp16s)((TRB * mvC[i].dy) / TRD);
                mvBack[i].dx = (Ipp16s)(((TRB - TRD) * mvC[i].dx) / TRD);
                mvBack[i].dy = (Ipp16s)(((TRB - TRD) * mvC[i].dy) / TRD);
            }
    } else {
        if (mp4_DecodeMVD(pInfo, &mvdx, &mvdy, 1) != MP4_STATUS_OK)
            return MP4_STATUS_ERROR;
        if (comb_type != IPPVC_MBTYPE_INTER4V) {
            mvForw[0].dx = mvForw[1].dx = mvForw[2].dx = mvForw[3].dx = (Ipp16s)((TRB * mvC[0].dx) / TRD + mvdx);
            mvForw[0].dy = mvForw[1].dy = mvForw[2].dy = mvForw[3].dy = (Ipp16s)((TRB * mvC[0].dy) / TRD + mvdy);
            if (mvdx == 0)
                mvBack[0].dx = mvBack[1].dx = mvBack[2].dx = mvBack[3].dx = (Ipp16s)(((TRB - TRD) * mvC[0].dx) / TRD);
            else
                mvBack[0].dx = mvBack[1].dx = mvBack[2].dx = mvBack[3].dx = (Ipp16s)(mvForw[0].dx - mvC[0].dx);
            if (mvdy == 0)
                mvBack[0].dy = mvBack[1].dy = mvBack[2].dy = mvBack[3].dy = (Ipp16s)(((TRB - TRD) * mvC[0].dy) / TRD);
            else
                mvBack[0].dy = mvBack[1].dy = mvBack[2].dy = mvBack[3].dy = (Ipp16s)(mvForw[0].dy - mvC[0].dy);
        } else
            for (i = 0; i < 4; i++) {
                mvForw[i].dx = (Ipp16s)((TRB * mvC[i].dx) / TRD + mvdx);
                mvForw[i].dy = (Ipp16s)((TRB * mvC[i].dy) / TRD + mvdy);
                if (mvdx == 0)
                    mvBack[i].dx = (Ipp16s)(((TRB - TRD) * mvC[i].dx) / TRD);
                else
                    mvBack[i].dx = (Ipp16s)(mvForw[i].dx - mvC[i].dx);
                if (mvdy == 0)
                    mvBack[i].dy = (Ipp16s)(((TRB - TRD) * mvC[i].dy) / TRD);
                else
                    mvBack[i].dy = (Ipp16s)(mvForw[i].dy - mvC[i].dy);
            }
    }
    return MP4_STATUS_OK;
}

#ifndef DROP_FIELD
mp4_Status mp4_DecodeMV_DirectField(mp4_Info *pInfo, int mb_ftfr, int mb_fbfr, IppMotionVector *mvTop, IppMotionVector *mvBottom, IppMotionVector *mvForwTop, IppMotionVector *mvForwBottom, IppMotionVector *mvBackTop, IppMotionVector *mvBackBottom, int TRB, int TRD, int modb)
{
    // field direct mode
    int  TRDt, TRDb, TRBt, TRBb, deltaTop, deltaBottom, mvdx, mvdy;

    deltaTop = mb_ftfr;
    deltaBottom = mb_fbfr - 1;
    if (pInfo->VisualObject.VideoObject.VideoObjectPlane.top_field_first) {
        deltaTop = -deltaTop;
        deltaBottom = -deltaBottom;
    }
    TRDt = mp4_DivRoundInf(TRD, pInfo->VisualObject.VideoObject.Tframe) * 2 + deltaTop;
    TRDb = mp4_DivRoundInf(TRD, pInfo->VisualObject.VideoObject.Tframe) * 2 + deltaBottom;
    TRBt = mp4_DivRoundInf(TRB, pInfo->VisualObject.VideoObject.Tframe) * 2 + deltaTop;
    TRBb = mp4_DivRoundInf(TRB, pInfo->VisualObject.VideoObject.Tframe) * 2 + deltaBottom;
    if (modb == 2) {
        // delta == 0
        mvdx = mvdy = 0;
    } else {
        if (mp4_DecodeMVD(pInfo, &mvdx, &mvdy, 1) != MP4_STATUS_OK)
            return MP4_STATUS_ERROR;
    }
    mvForwTop->dx = (Ipp16s)((TRBt * mvTop->dx) / TRDt + mvdx);
    if (mvdx == 0)
        mvBackTop->dx = (Ipp16s)(((TRBt - TRDt) * mvTop->dx) / TRDt);
    else
        mvBackTop->dx = (Ipp16s)(mvForwTop->dx - mvTop->dx);
    mvForwTop->dy = (Ipp16s)((TRBt * mvTop->dy * 2) / TRDt + mvdy);
    if (mvdy == 0)
        mvBackTop->dy = (Ipp16s)(((TRBt - TRDt) * mvTop->dy * 2) / TRDt);
    else
        mvBackTop->dy = (Ipp16s)(mvForwTop->dy - mvTop->dy * 2);
    mvForwBottom->dx = (Ipp16s)((TRBb * mvBottom->dx) / TRDb + mvdx);
    if (mvdx == 0)
        mvBackBottom->dx = (Ipp16s)(((TRBb - TRDb) * mvBottom->dx) / TRDb);
    else
        mvBackBottom->dx = (Ipp16s)(mvForwBottom->dx - mvBottom->dx);
    mvForwBottom->dy = (Ipp16s)((TRBb * mvBottom->dy * 2) / TRDb + mvdy);
    if (mvdy == 0)
        mvBackBottom->dy = (Ipp16s)(((TRBb - TRDb) * mvBottom->dy * 2) / TRDb);
    else
        mvBackBottom->dy = (Ipp16s)(mvForwBottom->dy - mvBottom->dy * 2);
    mvForwTop->dy >>= 1;
    mvBackTop->dy >>= 1;
    mvForwBottom->dy >>= 1;
    mvBackBottom->dy >>= 1;
    return MP4_STATUS_OK;
}


#endif

static void mp4_ExpandFrameReplicate(Ipp8u *pSrcDstPlane, int frameWidth, int frameHeight, int expandPels, int step)
{
    Ipp8u   *pDst1, *pDst2, *pSrc1, *pSrc2;
    int     i, j;
    Ipp32u  t1, t2;

    pDst1 = pSrcDstPlane + step * expandPels;
    pDst2 = pDst1 + frameWidth + expandPels;
    if (expandPels == 8) {
        for (i = 0; i < frameHeight; i ++) {
            t1 = pDst1[8] + (pDst1[8] << 8);
            t2 = pDst2[-1] + (pDst2[-1] << 8);
            t1 = (t1 << 16) + t1;
            t2 = (t2 << 16) + t2;
            ((Ipp32u*)pDst1)[0] = t1;
            ((Ipp32u*)pDst1)[1] = t1;
            ((Ipp32u*)pDst2)[0] = t2;
            ((Ipp32u*)pDst2)[1] = t2;
            pDst1 += step;
            pDst2 += step;
        }
    } else if (expandPels == 16) {
        for (i = 0; i < frameHeight; i ++) {
            t1 = pDst1[16] + (pDst1[16] << 8);
            t2 = pDst2[-1] + (pDst2[-1] << 8);
            t1 = (t1 << 16) + t1;
            t2 = (t2 << 16) + t2;
            ((Ipp32u*)pDst1)[0] = t1;
            ((Ipp32u*)pDst1)[1] = t1;
            ((Ipp32u*)pDst1)[2] = t1;
            ((Ipp32u*)pDst1)[3] = t1;
            ((Ipp32u*)pDst2)[0] = t2;
            ((Ipp32u*)pDst2)[1] = t2;
            ((Ipp32u*)pDst2)[2] = t2;
            ((Ipp32u*)pDst2)[3] = t2;
            pDst1 += step;
            pDst2 += step;
        }
    } else {
        for (i = 0; i < frameHeight; i ++) {
            ippsSet_8u_x(pDst1[expandPels], pDst1, expandPels);
            ippsSet_8u_x(pDst2[-1], pDst2, expandPels);
            pDst1 += step;
            pDst2 += step;
        }
    }
    pDst1 = pSrcDstPlane;
    pSrc1 = pSrcDstPlane + expandPels * step;
    pDst2 = pSrc1 + frameHeight * step;
    pSrc2 = pDst2 - step;
    j = frameWidth + 2 * expandPels;
    for (i = 0; i < expandPels; i ++) {
        ippsCopy_8u_x(pSrc1, pDst1, j);
        ippsCopy_8u_x(pSrc2, pDst2, j);
        pDst1 += step;
        pDst2 += step;
    }
}


void mp4_PadFrame(mp4_Info* pInfo)
{
#if 0
    /*
    //  padding VOP (for not complete blocks padd by
    //      0 for DivX(tm) 5.0 AVI streams
    //      128 for QuickTime(tm) MP4 streams
    //      replication for other
    */
    int  wL, hL, wC, hC, i;

    //if (pInfo->VisualObject.VideoObject.short_video_header)
    //    return;
    wL = pInfo->VisualObject.VideoObject.width;
    hL = pInfo->VisualObject.VideoObject.height;
    wC = pInfo->VisualObject.VideoObject.width >> 1;
    hC = pInfo->VisualObject.VideoObject.height >> 1;
    if ((pInfo->VisualObject.VideoObject.width & 15 || pInfo->VisualObject.VideoObject.height & 15) &&
        ((pInfo->ftype == 1 && pInfo->ftype_f == 0) || (pInfo->ftype == 2 && pInfo->ftype_f == 1))) {
        Ipp8u     pad = (Ipp8u)(pInfo->ftype == 1 ? 128 : 0);

        if (pInfo->VisualObject.VideoObject.width & 15) {
            Ipp8u *p;
            // pad one col
            p = pInfo->VisualObject.cFrame.pY + pInfo->VisualObject.VideoObject.width;
            for (i = 0; i < pInfo->VisualObject.VideoObject.height; i ++) {
                *p = pad;
                p += pInfo->VisualObject.cFrame.stepY;
            }
            p = pInfo->VisualObject.cFrame.pCb + (pInfo->VisualObject.VideoObject.width >> 1);
            for (i = 0; i < pInfo->VisualObject.VideoObject.height >> 1; i ++) {
                *p = pad;
                p += pInfo->VisualObject.cFrame.stepCb;
            }
            p = pInfo->VisualObject.cFrame.pCr + (pInfo->VisualObject.VideoObject.width >> 1);
            for (i = 0; i < pInfo->VisualObject.VideoObject.height >> 1; i ++) {
                *p = pad;
                p += pInfo->VisualObject.cFrame.stepCr;
            }
            wL ++;
            wC ++;
        }
        if (pInfo->VisualObject.VideoObject.height & 15) {
            // pad one row
            ippsSet_8u_x(pad, pInfo->VisualObject.cFrame.pY + pInfo->VisualObject.cFrame.stepY * pInfo->VisualObject.VideoObject.height, pInfo->VisualObject.VideoObject.width);
            ippsSet_8u_x(pad, pInfo->VisualObject.cFrame.pCb + pInfo->VisualObject.cFrame.stepCb * (pInfo->VisualObject.VideoObject.height >> 1), pInfo->VisualObject.VideoObject.width >> 1);
            ippsSet_8u_x(pad, pInfo->VisualObject.cFrame.pCr + pInfo->VisualObject.cFrame.stepCr * (pInfo->VisualObject.VideoObject.height >> 1), pInfo->VisualObject.VideoObject.width >> 1);
            hL ++;
            hC ++;
        }
    }
#else
    /*
    //  padding VOP for not complete blocks
    //  replication from macroblock boundary for DIVX and MP4 and from frame boundary for other
    */
    int  wL, hL, wC, hC;

    if ((pInfo->ftype == 1 && pInfo->ftype_f == 0) || (pInfo->ftype == 2 && pInfo->ftype_f == 1)) {
        wL = pInfo->VisualObject.VideoObject.MacroBlockPerRow * 16;
        hL = pInfo->VisualObject.VideoObject.MacroBlockPerCol * 16;
    } else {
        wL = pInfo->VisualObject.VideoObject.width;
        hL = pInfo->VisualObject.VideoObject.height;
    }
    wC = wL >> 1;
    hC = hL >> 1;
#endif
    mp4_ExpandFrameReplicate(pInfo->VisualObject.cFrame.apY, wL, hL, 16, pInfo->VisualObject.cFrame.stepY);
    mp4_ExpandFrameReplicate(pInfo->VisualObject.cFrame.apCb, wC, hC, 8, pInfo->VisualObject.cFrame.stepCb);
    mp4_ExpandFrameReplicate(pInfo->VisualObject.cFrame.apCr, wC, hC, 8, pInfo->VisualObject.cFrame.stepCr);
/*
    if (pInfo->VisualObject.VideoObject.interlaced) {
        Ipp8u *psb, *pdb, *pst, *pdt;
        // pad fields
        psb = pInfo->VisualObject.cFrame.pY + pInfo->VisualObject.cFrame.stepY - 16;
        pdb = psb - (pInfo->VisualObject.cFrame.stepY << 1);
        pst = pInfo->VisualObject.cFrame.pY + pInfo->VisualObject.cFrame.stepY * (pInfo->VisualObject.VideoObject.height - 2) - 16;
        pdt = pst + (pInfo->VisualObject.cFrame.stepY << 1);
        for (i = 0; i < 8; i ++) {
            ippsCopy_8u_x(psb, pdb, pInfo->VisualObject.cFrame.stepY);
            pdb -= (pInfo->VisualObject.cFrame.stepY << 1);
            ippsCopy_8u_x(pst, pdt, pInfo->VisualObject.cFrame.stepY);
            pdt += (pInfo->VisualObject.cFrame.stepY << 1);
        }
    }
*/
}


mp4_Status mp4_DecodeVideoObjectPlane(mp4_Info* pInfo)
{
    mp4_Status  status = MP4_STATUS_OK;
    Ipp64s      vop_time;

    // set VOP time
    if (pInfo->VisualObject.VideoObject.short_video_header) {
        vop_time = pInfo->VisualObject.VideoObject.vop_sync_time + pInfo->VisualObject.VideoObject.VideoObjectPlaneH263.temporal_reference * 1001;
        if (pInfo->VisualObject.cFrame.time > vop_time) {
            pInfo->VisualObject.VideoObject.vop_sync_time += 256 * 1001;
            vop_time += 256 * 1001;
        }
    } 
#ifdef SUPPORT_H263_ONLY 
    else return MP4_STATUS_FILE_ERROR;
#else    
    else {
        if (pInfo->VisualObject.VideoObject.VideoObjectPlane.coding_type == MP4_VOP_TYPE_B) {
            vop_time = pInfo->VisualObject.VideoObject.vop_sync_time_b + pInfo->VisualObject.VideoObject.VideoObjectPlane.modulo_time_base * pInfo->VisualObject.VideoObject.vop_time_increment_resolution + pInfo->VisualObject.VideoObject.VideoObjectPlane.time_increment;
        } else {
            if (pInfo->VisualObject.VideoObject.GroupOfVideoObjectPlane.time_code > pInfo->VisualObject.VideoObject.vop_sync_time)
                pInfo->VisualObject.VideoObject.vop_sync_time = pInfo->VisualObject.VideoObject.GroupOfVideoObjectPlane.time_code;
            vop_time = pInfo->VisualObject.VideoObject.vop_sync_time + pInfo->VisualObject.VideoObject.VideoObjectPlane.modulo_time_base * pInfo->VisualObject.VideoObject.vop_time_increment_resolution + pInfo->VisualObject.VideoObject.VideoObjectPlane.time_increment;
            if (pInfo->VisualObject.VideoObject.vop_sync_time_b < pInfo->VisualObject.VideoObject.vop_sync_time)
                pInfo->VisualObject.VideoObject.vop_sync_time_b = pInfo->VisualObject.VideoObject.vop_sync_time;
            if (pInfo->VisualObject.VideoObject.VideoObjectPlane.modulo_time_base != 0) {
                pInfo->VisualObject.VideoObject.vop_sync_time = vop_time - pInfo->VisualObject.VideoObject.VideoObjectPlane.time_increment;
            }
        }
    }
#endif    
//  if (pInfo->VisualObject.VideoObject.VideoObjectPlane.coded || vop_time != pInfo->VisualObject.rFrame.time) {
    if (pInfo->VisualObject.VideoObject.VideoObjectPlane.coded ||
        (vop_time != pInfo->VisualObject.cFrame.time &&
         vop_time != pInfo->VisualObject.rFrame.time &&
         vop_time != pInfo->VisualObject.nFrame.time)) {
        switch (pInfo->VisualObject.VideoObject.VideoObjectPlane.coding_type) {
        case MP4_VOP_TYPE_I :
            // set new video frame
            if (pInfo->VisualObject.VideoObject.VOPindex == 0) {
                pInfo->VisualObject.vFrame = NULL;
                pInfo->VisualObject.VideoObject.prevPlaneIsB = 0;
            } else {
                if (pInfo->VisualObject.VideoObject.prevPlaneIsB) {
                    mp4_SWAP(mp4_Frame, pInfo->VisualObject.rFrame, pInfo->VisualObject.nFrame);
                    pInfo->VisualObject.VideoObject.prevPlaneIsB = 0;
                } else {
					//***
					if( !pInfo->VisualObject.cFrame.droppable )
						mp4_SWAP(mp4_Frame, pInfo->VisualObject.rFrame, pInfo->VisualObject.cFrame);
                }
                pInfo->VisualObject.vFrame = &pInfo->VisualObject.rFrame;
            }
#ifndef DROP_SPRITE
            if (pInfo->VisualObject.VideoObject.sprite_enable == MP4_SPRITE_STATIC && pInfo->VisualObject.VideoObject.VOPindex == 0) {
                mp4_SWAP(mp4_Frame, pInfo->VisualObject.sFrame, pInfo->VisualObject.cFrame);
            }
#endif
            if (pInfo->VisualObject.VideoObject.VideoObjectPlane.coded) {
                if (pInfo->VisualObject.VideoObject.shape == MP4_SHAPE_TYPE_RECTANGULAR) {
#ifdef _OMP_KARABAS
                    if (!pInfo->VisualObject.VideoObject.data_partitioned && pInfo->num_threads >= 2)
                        status = mp4_DecodeVOP_I_MT(pInfo);
                    else
#endif
                    status = mp4_DecodeVOP_I(pInfo);
                } 
#ifdef SUPPORT_H263_ONLY 
				else return MP4_STATUS_FILE_ERROR;
#else                       
                else
                    status = mp4_DecodeVOP_I_Shape(pInfo);
#endif
                    
#ifndef DROP_SPRITE
                if (pInfo->VisualObject.VideoObject.sprite_enable == MP4_SPRITE_STATIC && pInfo->VisualObject.VideoObject.VOPindex == 0) {
                    mp4_SWAP(mp4_Frame, pInfo->VisualObject.sFrame, pInfo->VisualObject.cFrame);
                    mp4_ExpandFrameReplicate(pInfo->VisualObject.sFrame.apY, pInfo->VisualObject.VideoObject.sprite_width, pInfo->VisualObject.VideoObject.sprite_height, 16, pInfo->VisualObject.sFrame.stepY);
                    mp4_ExpandFrameReplicate(pInfo->VisualObject.sFrame.apCb, pInfo->VisualObject.VideoObject.sprite_width >> 1, pInfo->VisualObject.VideoObject.sprite_height >> 1, 8, pInfo->VisualObject.sFrame.stepCb);
                    mp4_ExpandFrameReplicate(pInfo->VisualObject.sFrame.apCr, pInfo->VisualObject.VideoObject.sprite_width >> 1, pInfo->VisualObject.VideoObject.sprite_height >> 1, 8, pInfo->VisualObject.sFrame.stepCr);
                } 
				else 
#endif				
				{
                    mp4_PadFrame(pInfo);//***flv special case needed here...
                }
                // set past and future time for B-VOP
                pInfo->VisualObject.VideoObject.rTime = pInfo->VisualObject.VideoObject.nTime;
                pInfo->VisualObject.VideoObject.nTime = vop_time;
#ifdef USE_NOTCODED_STATE
                // Clear not_coded MB state
                if ((pInfo->VisualObject.VideoObject.sprite_enable != MP4_SPRITE_STATIC) && pInfo->VisualObject.VideoObject.obmc_disable && !pInfo->VisualObject.VideoObject.ncStateCleared) {
                    ippsZero_8u_x(pInfo->VisualObject.VideoObject.ncState, pInfo->VisualObject.VideoObject.MacroBlockPerVOP);
                    pInfo->VisualObject.VideoObject.ncStateCleared = 1;
                }
#endif
            }
            mp4_StatisticInc(&pInfo->VisualObject.Statistic.nVOP_I);
            break;
        case MP4_VOP_TYPE_P :
            // set new video frame
            if (pInfo->VisualObject.VideoObject.prevPlaneIsB) {
                mp4_SWAP(mp4_Frame, pInfo->VisualObject.rFrame, pInfo->VisualObject.nFrame);
                pInfo->VisualObject.VideoObject.prevPlaneIsB = 0;
            } else {
				//***
				if( !pInfo->VisualObject.cFrame.droppable )
					mp4_SWAP(mp4_Frame, pInfo->VisualObject.rFrame, pInfo->VisualObject.cFrame);
            }
            pInfo->VisualObject.vFrame = &pInfo->VisualObject.rFrame;
            if (pInfo->VisualObject.VideoObject.VideoObjectPlane.coded) {
#ifdef _OMP_KARABAS
                if (!pInfo->VisualObject.VideoObject.data_partitioned && pInfo->num_threads >= 2)
                    status = mp4_DecodeVOP_P_MT(pInfo);
                else
#endif
                status = mp4_DecodeVOP_P(pInfo);
                mp4_PadFrame(pInfo);
                // set past and future time for B-VOP
                pInfo->VisualObject.VideoObject.rTime = pInfo->VisualObject.VideoObject.nTime;
                pInfo->VisualObject.VideoObject.nTime = vop_time;
            }
            mp4_StatisticInc(&pInfo->VisualObject.Statistic.nVOP_P);
#ifdef USE_NOTCODED_STATE
            pInfo->VisualObject.VideoObject.ncStateCleared = 0;
#endif
            break;

#ifndef SUPPORT_H263_ONLY 
        case MP4_VOP_TYPE_B :
            status = MP4_STATUS_OK;
            if (!pInfo->VisualObject.VideoObject.prevPlaneIsB) {
                mp4_SWAP(mp4_Frame, pInfo->VisualObject.nFrame, pInfo->VisualObject.cFrame);
                pInfo->VisualObject.VideoObject.prevPlaneIsB = 1;
            }
            // set Tframe for direct interlaced mode
            if (!pInfo->VisualObject.VideoObject.Tframe) {
                pInfo->VisualObject.VideoObject.Tframe = (int)(vop_time - pInfo->VisualObject.rFrame.time);
            }
            if (pInfo->VisualObject.VideoObject.VideoObjectPlane.coded) {
                pInfo->VisualObject.VideoObject.TRB = (int)(vop_time - pInfo->VisualObject.VideoObject.rTime);
                pInfo->VisualObject.VideoObject.TRD = (int)(pInfo->VisualObject.VideoObject.nTime - pInfo->VisualObject.VideoObject.rTime);
                // defense from bad streams when B-VOPs are before Past and/or Future
                if (pInfo->VisualObject.VideoObject.TRB <= 0)
                    pInfo->VisualObject.VideoObject.TRB = 1;
                if (pInfo->VisualObject.VideoObject.TRD <= 0)
                    pInfo->VisualObject.VideoObject.TRD = 2;
                if (pInfo->VisualObject.VideoObject.TRD <= pInfo->VisualObject.VideoObject.TRB) {
                    pInfo->VisualObject.VideoObject.TRB = 1;
                    pInfo->VisualObject.VideoObject.TRD = 2;
                }
#ifdef _OMP_KARABAS
                if (pInfo->num_threads >= 2)
                    status = mp4_DecodeVOP_B_MT(pInfo);
                else
#endif
                status = mp4_DecodeVOP_B(pInfo);
            }
            pInfo->VisualObject.vFrame = &pInfo->VisualObject.cFrame;
            mp4_StatisticInc(&pInfo->VisualObject.Statistic.nVOP_B);
            break;
#endif

#ifndef DROP_SPRITE
        case MP4_VOP_TYPE_S :
            // set new video frame
            if (pInfo->VisualObject.VideoObject.sprite_enable == MP4_SPRITE_GMC) {
                if (pInfo->VisualObject.VideoObject.prevPlaneIsB) {
                    mp4_SWAP(mp4_Frame, pInfo->VisualObject.rFrame, pInfo->VisualObject.nFrame);
                    pInfo->VisualObject.VideoObject.prevPlaneIsB = 0;
                } else {
                    mp4_SWAP(mp4_Frame, pInfo->VisualObject.rFrame, pInfo->VisualObject.cFrame);
                }
                pInfo->VisualObject.vFrame = &pInfo->VisualObject.rFrame;
            } else
                pInfo->VisualObject.vFrame = &pInfo->VisualObject.cFrame;
            if (pInfo->VisualObject.VideoObject.VideoObjectPlane.coded) {
#ifdef _OMP_KARABAS
                if (!pInfo->VisualObject.VideoObject.data_partitioned && pInfo->num_threads >= 2)
                    status = mp4_DecodeVOP_S_MT(pInfo);
                else
#endif
                status = mp4_DecodeVOP_S(pInfo);
#ifndef DROP_GMC
                if (pInfo->VisualObject.VideoObject.sprite_enable == MP4_SPRITE_GMC) {
                    mp4_PadFrame(pInfo);
                    // set past and future time for B-VOP
                    pInfo->VisualObject.VideoObject.rTime = pInfo->VisualObject.VideoObject.nTime;
                    pInfo->VisualObject.VideoObject.nTime = vop_time;
                }
#endif

#ifdef USE_NOTCODED_STATE
                // Clear not_coded MB state
                if ((pInfo->VisualObject.VideoObject.sprite_enable != MP4_SPRITE_STATIC) && pInfo->VisualObject.VideoObject.obmc_disable && !pInfo->VisualObject.VideoObject.ncStateCleared) {
                    ippsZero_8u_x(pInfo->VisualObject.VideoObject.ncState, pInfo->VisualObject.VideoObject.MacroBlockPerVOP);
                    pInfo->VisualObject.VideoObject.ncStateCleared = 1;
                }
#endif
            }
            mp4_StatisticInc(&pInfo->VisualObject.Statistic.nVOP_S);
            break;
#endif
        }
        if (!pInfo->VisualObject.VideoObject.VideoObjectPlane.coded) {
            ippsCopy_8u_x(pInfo->VisualObject.rFrame.apY, pInfo->VisualObject.cFrame.apY, pInfo->VisualObject.cFrame.stepY * ((pInfo->VisualObject.VideoObject.MacroBlockPerCol + 2) << 4));
            ippsCopy_8u_x(pInfo->VisualObject.rFrame.apCb, pInfo->VisualObject.cFrame.apCb, pInfo->VisualObject.cFrame.stepCb * ((pInfo->VisualObject.VideoObject.MacroBlockPerCol + 2) << 3));
            ippsCopy_8u_x(pInfo->VisualObject.rFrame.apCr, pInfo->VisualObject.cFrame.apCr, pInfo->VisualObject.cFrame.stepCr * ((pInfo->VisualObject.VideoObject.MacroBlockPerCol + 2) << 3));
#ifdef USE_NOTCODED_STATE
            ippsSet_8u_x(1, pInfo->VisualObject.VideoObject.ncState, pInfo->VisualObject.VideoObject.MacroBlockPerVOP);
            pInfo->VisualObject.VideoObject.ncStateCleared = 0;
#endif
        }
        mp4_StatisticInc(&pInfo->VisualObject.Statistic.nVOP);
    }
    // save current VOP type
    pInfo->VisualObject.cFrame.type = pInfo->VisualObject.VideoObject.VideoObjectPlane.coding_type;
    // save current VOP time
    pInfo->VisualObject.cFrame.time = vop_time;
	
	//***
	pInfo->VisualObject.cFrame.droppable = pInfo->VisualObject.VideoObject.VideoObjectPlane.droppable;

    return status;
}

void mp4_SetDefaultIDCTProcs( mp4_Info *pInfo ) 
{
	pInfo->m_ippiDCT8x8Inv_16s_C1I			= ippiDCT8x8Inv_16s_C1I_universal;
	pInfo->m_ippiDCT8x8Inv_4x4_16s_C1I		= ippiDCT8x8Inv_4x4_16s_C1I_universal;
	pInfo->m_ippiDCT8x8Inv_2x2_16s_C1I		= ippiDCT8x8Inv_2x2_16s_C1I_universal;
	pInfo->m_ippiDCT8x8Inv_16s8u_C1R		= ippiDCT8x8Inv_16s8u_C1R_universal;
	pInfo->m_ippiDCT8x8Inv_4x4_16s8u_C1R	= ippiDCT8x8Inv_4x4_16s8u_C1R_universal;
	pInfo->m_ippiDCT8x8Inv_2x2_16s8u_C1R	= ippiDCT8x8Inv_2x2_16s8u_C1R_universal;
}


void mp4_SetApprox( mp4_Info *pInfo, int level ) 
{	
	if( (level & (1<<5))  != 0 )
	{
		pInfo->m_ippiDCT8x8Inv_16s_C1I			= ippiDCT8x8Inv_DC_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_4x4_16s_C1I		= ippiDCT8x8Inv_DC_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_2x2_16s_C1I		= ippiDCT8x8Inv_DC_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_16s8u_C1R		= ippiDCT8x8Inv_DC_16s8u_C1R_universal;
		pInfo->m_ippiDCT8x8Inv_4x4_16s8u_C1R	= ippiDCT8x8Inv_DC_16s8u_C1R_universal;
		pInfo->m_ippiDCT8x8Inv_2x2_16s8u_C1R	= ippiDCT8x8Inv_DC_16s8u_C1R_universal;
	}
	else if( (level & (1<<4))  != 0 )
	{
		pInfo->m_ippiDCT8x8Inv_16s_C1I			= ippiDCT8x8Inv_2x2_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_4x4_16s_C1I		= ippiDCT8x8Inv_DC_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_2x2_16s_C1I		= ippiDCT8x8Inv_DC_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_16s8u_C1R		= ippiDCT8x8Inv_2x2_16s8u_C1R_universal;
		pInfo->m_ippiDCT8x8Inv_4x4_16s8u_C1R	= ippiDCT8x8Inv_DC_16s8u_C1R_universal;
		pInfo->m_ippiDCT8x8Inv_2x2_16s8u_C1R	= ippiDCT8x8Inv_DC_16s8u_C1R_universal;
	}
	else if( (level & (1<<3))  != 0 )
	{
		pInfo->m_ippiDCT8x8Inv_16s_C1I			= ippiDCT8x8Inv_2x2_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_4x4_16s_C1I		= ippiDCT8x8Inv_2x2_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_2x2_16s_C1I		= ippiDCT8x8Inv_DC_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_16s8u_C1R		= ippiDCT8x8Inv_2x2_16s8u_C1R_universal;
		pInfo->m_ippiDCT8x8Inv_4x4_16s8u_C1R	= ippiDCT8x8Inv_2x2_16s8u_C1R_universal;
		pInfo->m_ippiDCT8x8Inv_2x2_16s8u_C1R	= ippiDCT8x8Inv_DC_16s8u_C1R_universal;
	}
	else if( (level & (1<<2))  != 0 )
	{
		pInfo->m_ippiDCT8x8Inv_16s_C1I			= ippiDCT8x8Inv_2x2_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_4x4_16s_C1I		= ippiDCT8x8Inv_2x2_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_2x2_16s_C1I		= ippiDCT8x8Inv_2x2_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_16s8u_C1R		= ippiDCT8x8Inv_2x2_16s8u_C1R_universal;
		pInfo->m_ippiDCT8x8Inv_4x4_16s8u_C1R	= ippiDCT8x8Inv_2x2_16s8u_C1R_universal;
		pInfo->m_ippiDCT8x8Inv_2x2_16s8u_C1R	= ippiDCT8x8Inv_2x2_16s8u_C1R_universal;
	}
	else if( (level & (1<<1))  != 0 )
	{
		pInfo->m_ippiDCT8x8Inv_16s_C1I			= ippiDCT8x8Inv_4x4_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_4x4_16s_C1I		= ippiDCT8x8Inv_2x2_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_2x2_16s_C1I		= ippiDCT8x8Inv_2x2_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_16s8u_C1R		= ippiDCT8x8Inv_4x4_16s8u_C1R_universal;
		pInfo->m_ippiDCT8x8Inv_4x4_16s8u_C1R	= ippiDCT8x8Inv_2x2_16s8u_C1R_universal;
		pInfo->m_ippiDCT8x8Inv_2x2_16s8u_C1R	= ippiDCT8x8Inv_2x2_16s8u_C1R_universal;
	}
	else if( (level & (1<<0))  != 0 )
	{
		pInfo->m_ippiDCT8x8Inv_16s_C1I			= ippiDCT8x8Inv_4x4_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_4x4_16s_C1I		= ippiDCT8x8Inv_4x4_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_2x2_16s_C1I		= ippiDCT8x8Inv_2x2_16s_C1I_universal;
		pInfo->m_ippiDCT8x8Inv_16s8u_C1R		= ippiDCT8x8Inv_4x4_16s8u_C1R_universal;
		pInfo->m_ippiDCT8x8Inv_4x4_16s8u_C1R	= ippiDCT8x8Inv_4x4_16s8u_C1R_universal;
		pInfo->m_ippiDCT8x8Inv_2x2_16s8u_C1R	= ippiDCT8x8Inv_2x2_16s8u_C1R_universal;
	}
	else
		mp4_SetDefaultIDCTProcs(pInfo);
}

/*
//  Intra DC and AC reconstruction for SVH macroblock
*/
mp4_Status mp4_DecodeIntraMB_SVH(mp4_Info *pInfo, int pat, int quant, Ipp8u *pR[], int stepR[])
{
    __ALIGN16(Ipp16s, coeff, 64);
    int  blockNum, pm = 32, lnz;
    //***
	int h263_flv = pInfo->VisualObject.VideoObject.h263_flv;


    for (blockNum = 0; blockNum < 6; blockNum ++) {
    	//***
        if (ippiReconstructCoeffsIntra_H263_1u16s_flv(&pInfo->bufptr, &pInfo->bitoff, coeff, &lnz, pat & pm, quant, 0, IPPVC_SCAN_ZIGZAG, 0, h263_flv) != ippStsNoErr)
            return MP4_STATUS_ERROR;
        if (lnz > 0) 
		{
			//***kinoma optimization           
			if ((lnz <= 4) && (coeff[16] == 0)) 
				pInfo->m_ippiDCT8x8Inv_2x2_16s8u_C1R(coeff, pR[blockNum], stepR[blockNum]);
            else if ((lnz <= 13) && (coeff[32] == 0)) 
				pInfo->m_ippiDCT8x8Inv_4x4_16s8u_C1R(coeff, pR[blockNum], stepR[blockNum]);
            else 
				pInfo->m_ippiDCT8x8Inv_16s8u_C1R(coeff, pR[blockNum], stepR[blockNum]);
        } 
		else 
		{
            mp4_Set8x8_8u(pR[blockNum], stepR[blockNum], (Ipp8u)((coeff[0] + 4) >> 3));
        }
        if (pat & pm) {
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nB_INTRA_AC);
        } else {
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nB_INTRA_DC);
        }
        pm >>= 1;
    }
    return MP4_STATUS_OK;
}


mp4_Status mp4_DecodeInterMB_SVH(mp4_Info *pInfo, Ipp16s *coeffMB, int quant, int pat)
{
    int   i, lnz, pm = 32;
    Ipp16s *coeff = coeffMB;
    //***
	int h263_flv = pInfo->VisualObject.VideoObject.h263_flv;

    for (i = 0; i < 6; i ++) {
        if ((pat) & pm) {
        	//***
            if (ippiReconstructCoeffsInter_H263_1u16s_flv(&pInfo->bufptr, &pInfo->bitoff, coeff, &lnz, quant, 0, h263_flv) != ippStsNoErr)
                return MP4_STATUS_ERROR;
            if (lnz != 0) {
				//***kinoma optimization   
				if ((lnz <= 4) && (coeff[16] == 0))
                    pInfo->m_ippiDCT8x8Inv_2x2_16s_C1I(coeff);
                else if ((lnz <= 13) && (coeff[32] == 0))
                    pInfo->m_ippiDCT8x8Inv_4x4_16s_C1I(coeff);
                else
                    pInfo->m_ippiDCT8x8Inv_16s_C1I(coeff);
            } else {
                mp4_Set64_16s((Ipp16s)((coeff[0] + 4) >> 3), coeff);
            }
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nB_INTER_C);
        } else {
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nB_INTER_NC);
        }
        pm >>= 1;
        coeff += 64;
    }
    return MP4_STATUS_OK;
}


/*
//  Intra DC and AC reconstruction for macroblock
*/
mp4_Status mp4_DecodeIntraMB(mp4_Info *pInfo, int x, int pat, int quant, int dcVLC, int ac_pred_flag, Ipp8u *pR[], int stepR[])
{
    __ALIGN16(Ipp16s, coeff, 64);
    int         blockNum, lnz, predDir, scan, dc, dcA, dcB, dcC, dcP, k, nz, predQuantA, predQuantC, dcScaler, pm = 32;
    Ipp16s      *predAcA, *predAcC, sDC;
    mp4_IntraPredBlock *bCurr;

    for (blockNum = 0; blockNum < 6; blockNum ++) {
        // find prediction direction
        bCurr = &pInfo->VisualObject.VideoObject.IntraPredBuff.block[6*x+blockNum];
        dcA = bCurr->predA->dct_dc >= 0 ? bCurr->predA->dct_dc : 1024;
        dcB = bCurr->predB->dct_dc >= 0 ? bCurr->predB->dct_dc : 1024;
        dcC = bCurr->predC->dct_dc >= 0 ? bCurr->predC->dct_dc : 1024;
        if (mp4_ABS(dcA - dcB) < mp4_ABS(dcB - dcC)) {
            predDir = IPPVC_SCAN_HORIZONTAL;
            dcP = dcC;
        } else {
            predDir = IPPVC_SCAN_VERTICAL;
            dcP = dcA;
        }
        scan = IPPVC_SCAN_ZIGZAG;
        if (pInfo->VisualObject.VideoObject.VideoObjectPlane.alternate_vertical_scan_flag)
            scan = IPPVC_SCAN_VERTICAL;
        else if (ac_pred_flag)
            scan = predDir;
        // decode coeffs
        if (dcVLC) {
            if (ippiDecodeDCIntra_MPEG4_1u16s_x(&pInfo->bufptr, &pInfo->bitoff, coeff, (blockNum < 4) ? IPPVC_BLOCK_LUMA : IPPVC_BLOCK_CHROMA) != ippStsNoErr)
                return MP4_STATUS_ERROR;
        }
        if (pat & pm) {
            if (ippiDecodeCoeffsIntra_MPEG4_1u16s_x(&pInfo->bufptr, &pInfo->bitoff, coeff, &lnz, 0, dcVLC, scan) != ippStsNoErr)
                return MP4_STATUS_ERROR;
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nB_INTRA_AC);
        } else {
            if (dcVLC)
                sDC = coeff[0];
            mp4_Zero64_16s(coeff);
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nB_INTRA_DC);
            lnz = 0;
            if (dcVLC)
                coeff[0] = sDC;
        }
        // predict DC
        dcScaler = (blockNum < 4) ? mp4_DCScalerLuma[quant] : mp4_DCScalerChroma[quant];
        dc = coeff[0] + mp4_DivIntraDC(dcP, dcScaler);   // clip ??
        coeff[0] = (Ipp16s)dc;
        // predict AC
        nz = 0;
        if (ac_pred_flag) {
            if (predDir == IPPVC_SCAN_HORIZONTAL && (bCurr->predC->dct_dc >= 0)) {
                predAcC = bCurr->predC->dct_acC;
                predQuantC = (blockNum == 2 || blockNum == 3) ? quant : pInfo->VisualObject.VideoObject.IntraPredBuff.quant[x+1];
                if (predQuantC == quant)
                    for (k = 1; k < 8; k ++) {
                        coeff[k] = (Ipp16s)(coeff[k] + predAcC[k]); // clip ??
                        if (coeff[k])
                            nz = 1;
                    }
                else
                    for (k = 1; k < 8; k ++) {
                        coeff[k] = (Ipp16s)(coeff[k] + mp4_DivIntraAC(predAcC[k] * predQuantC, quant));
                        if (coeff[k])
                            nz = 1;
                    }
            } else if (predDir == IPPVC_SCAN_VERTICAL && (bCurr->predA->dct_dc >= 0)) {
                predAcA = bCurr->predA->dct_acA;
                predQuantA = (blockNum == 1 || blockNum == 3) ? quant : pInfo->VisualObject.VideoObject.IntraPredBuff.quant[x];
                if (predQuantA == quant)
                    for (k = 1; k < 8; k ++) {
                        coeff[k*8] = (Ipp16s)(coeff[k*8] + predAcA[k]);
                        if (coeff[k*8])
                            nz = 1;
                    }
                else
                    for (k = 1; k < 8; k ++) {
                        coeff[k*8] = (Ipp16s)(coeff[k*8] + mp4_DivIntraAC(predAcA[k] * predQuantA, quant));
                        if (coeff[k*8])
                            nz = 1;
                    }
            }
        }
        // copy predicted AC for future Prediction
        for (k = 1; k < 8; k ++) {
            bCurr[6].dct_acC[k] = coeff[k];
            bCurr[6].dct_acA[k] = coeff[k*8];
        }
        if ((nz | lnz) || (pInfo->VisualObject.VideoObject.quant_type == 1)) {
            ippiQuantInvIntra_MPEG4_16s_C1I_x(coeff, 63, pInfo->VisualObject.VideoObject.QuantInvIntraSpec, quant, (blockNum < 4) ? IPPVC_BLOCK_LUMA : IPPVC_BLOCK_CHROMA);
            //***kinoma optimization
			pInfo->m_ippiDCT8x8Inv_16s8u_C1R(coeff, pR[blockNum], stepR[blockNum]);
        } else {
            k = coeff[0] * dcScaler;
            coeff[0] = (Ipp16s)k; // clip ??
            k = (k + 4) >> 3;
            mp4_CLIP(k, 0, 255);
            mp4_Set8x8_8u(pR[blockNum], stepR[blockNum], (Ipp8u)k);
        }
        // copy DC for future Prediction
        if (blockNum >= 3)
            pInfo->VisualObject.VideoObject.IntraPredBuff.dcB[blockNum].dct_dc = bCurr[6].dct_dc;
        bCurr[6].dct_dc = coeff[0];
        // copy quant
        if (blockNum == 5)
            pInfo->VisualObject.VideoObject.IntraPredBuff.quant[x+1] = (Ipp8u)quant;
        pm >>= 1;
    }
    return MP4_STATUS_OK;
}


/*
//  Intra DC and AC reconstruction for DP macroblock
*/
mp4_Status mp4_DecodeIntraMB_DP(mp4_Info *pInfo, Ipp16s dct_dc[], int x, int pat, int quant, int dcVLC, int ac_pred_flag, Ipp8u *pR[], int stepR[])
{
    __ALIGN16(Ipp16s, coeff, 64);
    int         blockNum, lnz, predDir, scan, dc, dcA, dcB, dcC, dcP, k, nz, predQuantA, predQuantC, dcScaler, pm = 32;
    Ipp16s      *predAcA, *predAcC;
    mp4_IntraPredBlock *bCurr;

    for (blockNum = 0; blockNum < 6; blockNum ++) {
        // find prediction direction
        bCurr = &pInfo->VisualObject.VideoObject.IntraPredBuff.block[6*x+blockNum];
        dcA = bCurr->predA->dct_dc >= 0 ? bCurr->predA->dct_dc : 1024;
        dcB = bCurr->predB->dct_dc >= 0 ? bCurr->predB->dct_dc : 1024;
        dcC = bCurr->predC->dct_dc >= 0 ? bCurr->predC->dct_dc : 1024;
        if (mp4_ABS(dcA - dcB) < mp4_ABS(dcB - dcC)) {
            predDir = IPPVC_SCAN_HORIZONTAL;
            dcP = dcC;
        } else {
            predDir = IPPVC_SCAN_VERTICAL;
            dcP = dcA;
        }
        scan = (ac_pred_flag) ? predDir : IPPVC_SCAN_ZIGZAG;
        // decode coeffs
        if (pat & pm) {
            if (ippiDecodeCoeffsIntra_MPEG4_1u16s_x(&pInfo->bufptr, &pInfo->bitoff, coeff, &lnz, pInfo->VisualObject.VideoObject.reversible_vlc, dcVLC, scan) != ippStsNoErr)
                return MP4_STATUS_ERROR;
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nB_INTRA_AC);
        } else {
            mp4_Zero64_16s(coeff);
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nB_INTRA_DC);
            lnz = 0;
        }
        if (dcVLC)
            coeff[0] = dct_dc[blockNum];
        // predict DC
        dcScaler = (blockNum < 4) ? mp4_DCScalerLuma[quant] : mp4_DCScalerChroma[quant];
        dc = coeff[0] + mp4_DivIntraDC(dcP, dcScaler);   // clip ??
        coeff[0] = (Ipp16s)dc;
        // predict AC
        nz = 0;
        if (ac_pred_flag) {
            if (predDir == IPPVC_SCAN_HORIZONTAL && (bCurr->predC->dct_dc >= 0)) {
                predAcC = bCurr->predC->dct_acC;
                predQuantC = (blockNum == 2 || blockNum == 3) ? quant : pInfo->VisualObject.VideoObject.IntraPredBuff.quant[x+1];
                if (predQuantC == quant)
                    for (k = 1; k < 8; k ++) {
                        coeff[k] = (Ipp16s)(coeff[k] + predAcC[k]); // clip ??
                        if (coeff[k])
                            nz = 1;
                    }
                else
                    for (k = 1; k < 8; k ++) {
                        coeff[k] = (Ipp16s)(coeff[k] + mp4_DivIntraAC(predAcC[k] * predQuantC, quant));
                        if (coeff[k])
                            nz = 1;
                    }
            } else if (predDir == IPPVC_SCAN_VERTICAL && (bCurr->predA->dct_dc >= 0)) {
                predAcA = bCurr->predA->dct_acA;
                predQuantA = (blockNum == 1 || blockNum == 3) ? quant : pInfo->VisualObject.VideoObject.IntraPredBuff.quant[x];
                if (predQuantA == quant)
                    for (k = 1; k < 8; k ++) {
                        coeff[k*8] = (Ipp16s)(coeff[k*8] + predAcA[k]);
                        if (coeff[k*8])
                            nz = 1;
                    }
                else
                    for (k = 1; k < 8; k ++) {
                        coeff[k*8] = (Ipp16s)(coeff[k*8] + mp4_DivIntraAC(predAcA[k] * predQuantA, quant));
                        if (coeff[k*8])
                            nz = 1;
                    }
            }
        }
        // copy predicted AC for future Prediction
        for (k = 1; k < 8; k ++) {
            bCurr[6].dct_acC[k] = coeff[k];
            bCurr[6].dct_acA[k] = coeff[k*8];
        }
        if ((nz | lnz) || (pInfo->VisualObject.VideoObject.quant_type == 1)) {
            ippiQuantInvIntra_MPEG4_16s_C1I_x(coeff, 63, pInfo->VisualObject.VideoObject.QuantInvIntraSpec, quant, (blockNum < 4) ? IPPVC_BLOCK_LUMA : IPPVC_BLOCK_CHROMA);
            //***kinoma optimization
            pInfo->m_ippiDCT8x8Inv_16s8u_C1R(coeff, pR[blockNum], stepR[blockNum]);
        } else {
            k = coeff[0] * dcScaler;
            coeff[0] = (Ipp16s)k; // clip ??
            k = (k + 4) >> 3;
            mp4_CLIP(k, 0, 255);
            mp4_Set8x8_8u(pR[blockNum], stepR[blockNum], (Ipp8u)k);
        }
        // copy DC for future Prediction
        if (blockNum >= 3)
            pInfo->VisualObject.VideoObject.IntraPredBuff.dcB[blockNum].dct_dc = bCurr[6].dct_dc;
        bCurr[6].dct_dc = coeff[0];
        // copy quant
        if (blockNum == 5)
            pInfo->VisualObject.VideoObject.IntraPredBuff.quant[x+1] = (Ipp8u)quant;
        pm >>= 1;
    }
    return MP4_STATUS_OK;
}


/*
//  Intra DC and AC reconstruction for macroblock (w/o iDCT)
*/
mp4_Status mp4_ReconstructCoeffsIntraMB(mp4_Info *pInfo, int x, int pat, int quant, int dcVLC, int ac_pred_flag, Ipp16s *coeffMB, int lastNZ[])
{
    int         blockNum, lnz, predDir, scan, dc, dcA, dcB, dcC, dcP, k, nz, predQuantA, predQuantC, dcScaler, pm = 32;
    Ipp16s      *predAcA, *predAcC, sDC, *coeff = coeffMB;
    mp4_IntraPredBlock *bCurr;

    for (blockNum = 0; blockNum < 6; blockNum ++) {
        // find prediction direction
        bCurr = &pInfo->VisualObject.VideoObject.IntraPredBuff.block[6*x+blockNum];
        dcA = bCurr->predA->dct_dc >= 0 ? bCurr->predA->dct_dc : 1024;
        dcB = bCurr->predB->dct_dc >= 0 ? bCurr->predB->dct_dc : 1024;
        dcC = bCurr->predC->dct_dc >= 0 ? bCurr->predC->dct_dc : 1024;
        if (mp4_ABS(dcA - dcB) < mp4_ABS(dcB - dcC)) {
            predDir = IPPVC_SCAN_HORIZONTAL;
            dcP = dcC;
        } else {
            predDir = IPPVC_SCAN_VERTICAL;
            dcP = dcA;
        }
        scan = IPPVC_SCAN_ZIGZAG;
        if (pInfo->VisualObject.VideoObject.VideoObjectPlane.alternate_vertical_scan_flag)
            scan = IPPVC_SCAN_VERTICAL;
        else if (ac_pred_flag)
            scan = predDir;
        // decode coeffs
        if (dcVLC) {
            if (ippiDecodeDCIntra_MPEG4_1u16s_x(&pInfo->bufptr, &pInfo->bitoff, coeff, (blockNum < 4) ? IPPVC_BLOCK_LUMA : IPPVC_BLOCK_CHROMA) != ippStsNoErr)
                return MP4_STATUS_ERROR;
        }
        if (pat & pm) {
            if (ippiDecodeCoeffsIntra_MPEG4_1u16s_x(&pInfo->bufptr, &pInfo->bitoff, coeff, &lnz, 0, dcVLC, scan) != ippStsNoErr)
                return MP4_STATUS_ERROR;
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nB_INTRA_AC);
        } else {
            if (dcVLC)
                sDC = coeff[0];
            mp4_Zero64_16s(coeff);
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nB_INTRA_DC);
            lnz = 0;
            if (dcVLC)
                coeff[0] = sDC;
        }
        // predict DC
        dcScaler = (blockNum < 4) ? mp4_DCScalerLuma[quant] : mp4_DCScalerChroma[quant];
        dc = coeff[0] + mp4_DivIntraDC(dcP, dcScaler);   // clip ??
        coeff[0] = (Ipp16s)dc;
        // predict AC
        nz = 0;
        if (ac_pred_flag) {
            if (predDir == IPPVC_SCAN_HORIZONTAL && (bCurr->predC->dct_dc >= 0)) {
                predAcC = bCurr->predC->dct_acC;
                predQuantC = (blockNum == 2 || blockNum == 3) ? quant : pInfo->VisualObject.VideoObject.IntraPredBuff.quant[x+1];
                if (predQuantC == quant)
                    for (k = 1; k < 8; k ++) {
                        coeff[k] = (Ipp16s)(coeff[k] + predAcC[k]); // clip ??
                        if (coeff[k])
                            nz = 1;
                    }
                else
                    for (k = 1; k < 8; k ++) {
                        coeff[k] = (Ipp16s)(coeff[k] + mp4_DivIntraAC(predAcC[k] * predQuantC, quant));
                        if (coeff[k])
                            nz = 1;
                    }
            } else if (predDir == IPPVC_SCAN_VERTICAL && (bCurr->predA->dct_dc >= 0)) {
                predAcA = bCurr->predA->dct_acA;
                predQuantA = (blockNum == 1 || blockNum == 3) ? quant : pInfo->VisualObject.VideoObject.IntraPredBuff.quant[x];
                if (predQuantA == quant)
                    for (k = 1; k < 8; k ++) {
                        coeff[k*8] = (Ipp16s)(coeff[k*8] + predAcA[k]);
                        if (coeff[k*8])
                            nz = 1;
                    }
                else
                    for (k = 1; k < 8; k ++) {
                        coeff[k*8] = (Ipp16s)(coeff[k*8] + mp4_DivIntraAC(predAcA[k] * predQuantA, quant));
                        if (coeff[k*8])
                            nz = 1;
                    }
            }
        }
        // copy predicted AC for future Prediction
        for (k = 1; k < 8; k ++) {
            bCurr[6].dct_acC[k] = coeff[k];
            bCurr[6].dct_acA[k] = coeff[k*8];
        }
        if ((nz | lnz) || (pInfo->VisualObject.VideoObject.quant_type == 1)) {
            ippiQuantInvIntra_MPEG4_16s_C1I_x(coeff, 63, pInfo->VisualObject.VideoObject.QuantInvIntraSpec, quant, (blockNum < 4) ? IPPVC_BLOCK_LUMA : IPPVC_BLOCK_CHROMA);
            lnz = 63;
        } else {
            k = coeff[0] * dcScaler;
            coeff[0] = (Ipp16s)k; // clip ??
        }
        // copy DC for future Prediction
        if (blockNum >= 3)
            pInfo->VisualObject.VideoObject.IntraPredBuff.dcB[blockNum].dct_dc = bCurr[6].dct_dc;
        bCurr[6].dct_dc = coeff[0];
        // copy quant
        if (blockNum == 5)
            pInfo->VisualObject.VideoObject.IntraPredBuff.quant[x+1] = (Ipp8u)quant;
        lastNZ[blockNum] = lnz;
        pm >>= 1;
        coeff += 64;
    }
    return MP4_STATUS_OK;
}


mp4_Status mp4_DecodeInterMB(mp4_Info *pInfo, Ipp16s *coeffMB, int quant, int pat, int scan)
{
    int   i, lnz, pm = 32;
    Ipp16s *coeff = coeffMB;

    for (i = 0; i < 6; i ++) {
        if ((pat) & pm) {
            if (ippiReconstructCoeffsInter_MPEG4_1u16s_x(&pInfo->bufptr, &pInfo->bitoff, coeff, &lnz, pInfo->VisualObject.VideoObject.reversible_vlc, scan, pInfo->VisualObject.VideoObject.QuantInvInterSpec, quant) != ippStsNoErr)
                return MP4_STATUS_ERROR;
            if (pInfo->VisualObject.VideoObject.quant_type == 0 || (coeff[63] == 0)) {
                if (lnz != 0) {
                    if (scan == IPPVC_SCAN_ZIGZAG) {
						//***kinoma optimization   
                        if ((lnz <= 4) && (coeff[16] == 0))
                            pInfo->m_ippiDCT8x8Inv_2x2_16s_C1I(coeff);
                        else if ((lnz <= 13) && (coeff[32] == 0))
                            pInfo->m_ippiDCT8x8Inv_4x4_16s_C1I(coeff);
                        else
                            pInfo->m_ippiDCT8x8Inv_16s_C1I(coeff);
                    } else {  // IPPVC_SCAN_VERTICAL
                        if ((lnz <= 5) && (coeff[16] == 0) && (coeff[24] == 0))
                            pInfo->m_ippiDCT8x8Inv_2x2_16s_C1I(coeff);
                        else if (lnz <= 9)
                            pInfo->m_ippiDCT8x8Inv_4x4_16s_C1I(coeff);
                        else
                            pInfo->m_ippiDCT8x8Inv_16s_C1I(coeff);
                    }
                } else {
                    mp4_Set64_16s((Ipp16s)((coeff[0] + 4) >> 3), coeff);
                }
            } else {
                pInfo->m_ippiDCT8x8Inv_16s_C1I(coeff);
            }
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nB_INTER_C);
        } else {
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nB_INTER_NC);
        }
        pm >>= 1;
        coeff += 64;
    }
    return MP4_STATUS_OK;
}


void mp4_DCTInvCoeffsInterMB(mp4_Info *pInfo, Ipp16s *coeffMB, int lastNZ[], int pat, int scan)
{
    int   i, lnz, pm = 32;
    Ipp16s *coeff = coeffMB;

    for (i = 0; i < 6; i ++) {
        if ((pat) & pm) {
            if (pInfo->VisualObject.VideoObject.quant_type == 0 || (coeff[63] == 0)) {
                lnz = lastNZ[i];
                if (lnz != 0) {
                    if (scan == IPPVC_SCAN_ZIGZAG) {
						//***kinoma optimization   
                        if ((lnz <= 4) && (coeff[16] == 0))
                            pInfo->m_ippiDCT8x8Inv_2x2_16s_C1I(coeff);
                        else if ((lnz <= 13) && (coeff[32] == 0))
                            pInfo->m_ippiDCT8x8Inv_4x4_16s_C1I(coeff);
                        else
                            pInfo->m_ippiDCT8x8Inv_16s_C1I(coeff);
                    } else {  // IPPVC_SCAN_VERTICAL
                        if ((lnz <= 5) && (coeff[16] == 0) && (coeff[24] == 0))
                            pInfo->m_ippiDCT8x8Inv_2x2_16s_C1I(coeff);
                        else if (lnz <= 9)
                            pInfo->m_ippiDCT8x8Inv_4x4_16s_C1I(coeff);
                        else
                            pInfo->m_ippiDCT8x8Inv_16s_C1I(coeff);
                    }
                } else {
                    mp4_Set64_16s((Ipp16s)((coeff[0] + 4) >> 3), coeff);
                }
            } else {
                pInfo->m_ippiDCT8x8Inv_16s_C1I(coeff);
            }
        }
        pm >>= 1;
        coeff += 64;
    }
}


/*
//  decode mcbpc and set MBtype and ChromaPattern
*/
mp4_Status mp4_DecodeMCBPC_P(mp4_Info* pInfo, int *mbType, int *mbPattern, int stat)
{
    Ipp32u      code;
    int         type, pattern;

    code = mp4_ShowBits9(pInfo, 9);
    if (code >= 256) {
        type = IPPVC_MBTYPE_INTER;
        pattern = 0;
        mp4_FlushBits(pInfo, 1);
    } else {
        type = mp4_PVOPmb_type[code];
        pattern = mp4_PVOPmb_cbpc[code];
        mp4_FlushBits(pInfo, mp4_PVOPmb_bits[code]);
    }
    if (code == 0) {
        mp4_Error("Error when decode mcbpc of P-VOP macroblock");
        return MP4_STATUS_ERROR;
    }
    *mbType = type;
    *mbPattern = pattern;
    if (stat) {
        if (type == IPPVC_MBTYPE_INTER)
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB_INTER);
        else if (type == IPPVC_MBTYPE_INTER_Q)
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB_INTER_Q);
        else if (type == IPPVC_MBTYPE_INTRA)
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB_INTRA);
        else if (type == IPPVC_MBTYPE_INTRA_Q)
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB_INTRA_Q);
        else if (type == IPPVC_MBTYPE_INTER4V)
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB_INTER4V);
    }
    return MP4_STATUS_OK;
}


mp4_Status mp4_PredictDecode1MV(mp4_Info *pInfo, mp4_MacroBlock *MBcurr, int y, int x)
{
    IppMotionVector *mvLeft, *mvTop, *mvRight, *mvCurr;
    int              mbInRow = pInfo->VisualObject.VideoObject.MacroBlockPerRow;
    int              fcode = pInfo->VisualObject.VideoObject.VideoObjectPlane.fcode_forward;
    int              resync_marker_disable = pInfo->VisualObject.VideoObject.resync_marker_disable;

    // block 0
    mvCurr = MBcurr[0].mv;
    mvLeft  = MBcurr[-1].mv;
    mvTop   = MBcurr[-mbInRow].mv;
    mvRight = MBcurr[-mbInRow+1].mv;
    if (resync_marker_disable) {
        if ((y | x) == 0) {
            mvCurr[0].dx = mvCurr[0].dy = 0;
        } else if (x == 0) {
            mvCurr[0].dx = mp4_Median(0, mvTop[2].dx, mvRight[2].dx);
            mvCurr[0].dy = mp4_Median(0, mvTop[2].dy, mvRight[2].dy);
        } else if (y == 0) {
            mvCurr[0] = mvLeft[1];
        } else if (x == mbInRow - 1) {
            mvCurr[0].dx = mp4_Median(0, mvLeft[1].dx, mvTop[2].dx);
            mvCurr[0].dy = mp4_Median(0, mvLeft[1].dy, mvTop[2].dy);
        } else {
            mvCurr[0].dx = mp4_Median(mvLeft[1].dx, mvTop[2].dx, mvRight[2].dx);
            mvCurr[0].dy = mp4_Median(mvLeft[1].dy, mvTop[2].dy, mvRight[2].dy);
        }
    } else {
        int   validLeft, validTop, validRight;

        if (x > 0)
            validLeft = MBcurr[-1].validPred;
        else
            validLeft = 0;
        if (y > 0)
            validTop = MBcurr[-mbInRow].validPred;
        else
            validTop = 0;
        if ((y > 0) && (x < mbInRow - 1))
            validRight = MBcurr[-mbInRow+1].validPred;
        else
            validRight = 0;
        switch ((validLeft << 2) | (validTop << 1) | validRight) {
        case 7:
            mvCurr[0].dx = mp4_Median(mvLeft[1].dx, mvTop[2].dx, mvRight[2].dx);
            mvCurr[0].dy = mp4_Median(mvLeft[1].dy, mvTop[2].dy, mvRight[2].dy);
            break;
        case 6:
            mvCurr[0].dx = mp4_Median(mvLeft[1].dx, mvTop[2].dx, 0);
            mvCurr[0].dy = mp4_Median(mvLeft[1].dy, mvTop[2].dy, 0);
            break;
        case 5:
            mvCurr[0].dx = mp4_Median(mvLeft[1].dx, 0, mvRight[2].dx);
            mvCurr[0].dy = mp4_Median(mvLeft[1].dy, 0, mvRight[2].dy);
            break;
        case 4:
            mvCurr[0] = mvLeft[1];
            break;
        case 3:
            mvCurr[0].dx = mp4_Median(0, mvTop[2].dx, mvRight[2].dx);
            mvCurr[0].dy = mp4_Median(0, mvTop[2].dy, mvRight[2].dy);
            break;
        case 2:
            mvCurr[0] = mvTop[2];
            break;
        case 1:
            mvCurr[0] = mvRight[2];
            break;
        default:
            mvCurr[0].dx = mvCurr[0].dy = 0;
            break;
        }
    }
    return mp4_DecodeMV(pInfo, mvCurr, fcode);
}


mp4_Status mp4_PredictDecode4MV(mp4_Info *pInfo, mp4_MacroBlock *MBcurr, int y, int x)
{
    IppMotionVector *mvLeft, *mvTop, *mvRight, *mvCurr;
    int              mbInRow = pInfo->VisualObject.VideoObject.MacroBlockPerRow;
    int              fcode = pInfo->VisualObject.VideoObject.VideoObjectPlane.fcode_forward;
    int              resync_marker_disable = pInfo->VisualObject.VideoObject.resync_marker_disable;

    mvCurr = MBcurr[0].mv;
    mvLeft  = MBcurr[-1].mv;
    mvTop   = MBcurr[-mbInRow].mv;
    mvRight = MBcurr[-mbInRow+1].mv;
    if (resync_marker_disable) {
        // block 0
        if ((y | x) == 0) {
            mvCurr[0].dx = mvCurr[0].dy = 0;
        } else if (x == 0) {
            mvCurr[0].dx = mp4_Median(0, mvTop[2].dx, mvRight[2].dx);
            mvCurr[0].dy = mp4_Median(0, mvTop[2].dy, mvRight[2].dy);
        } else if (y == 0) {
            mvCurr[0] = mvLeft[1];
        } else if (x == mbInRow - 1) {
            mvCurr[0].dx = mp4_Median(0, mvLeft[1].dx, mvTop[2].dx);
            mvCurr[0].dy = mp4_Median(0, mvLeft[1].dy, mvTop[2].dy);
        } else {
            mvCurr[0].dx = mp4_Median(mvLeft[1].dx, mvTop[2].dx, mvRight[2].dx);
            mvCurr[0].dy = mp4_Median(mvLeft[1].dy, mvTop[2].dy, mvRight[2].dy);
        }
        if (mp4_DecodeMV(pInfo, mvCurr, fcode) != MP4_STATUS_OK)
            return MP4_STATUS_ERROR;
        // block 1
        if (y == 0) {
            mvCurr[1] = mvCurr[0];
        } else if (x == mbInRow - 1) {
            mvCurr[1].dx = mp4_Median(mvCurr[0].dx, mvTop[3].dx, 0);
            mvCurr[1].dy = mp4_Median(mvCurr[0].dy, mvTop[3].dy, 0);
        } else {
            mvCurr[1].dx = mp4_Median(mvCurr[0].dx, mvTop[3].dx, mvRight[2].dx);
            mvCurr[1].dy = mp4_Median(mvCurr[0].dy, mvTop[3].dy, mvRight[2].dy);
        }
        if (mp4_DecodeMV(pInfo, mvCurr+1, fcode) != MP4_STATUS_OK)
            return MP4_STATUS_ERROR;
        // block 2
        if (x == 0) {
            mvCurr[2].dx = mp4_Median(0, mvCurr[0].dx, mvCurr[1].dx);
            mvCurr[2].dy = mp4_Median(0, mvCurr[0].dy, mvCurr[1].dy);
        } else {
            mvCurr[2].dx = mp4_Median(mvLeft[3].dx, mvCurr[0].dx, mvCurr[1].dx);
            mvCurr[2].dy = mp4_Median(mvLeft[3].dy, mvCurr[0].dy, mvCurr[1].dy);
        }
        if (mp4_DecodeMV(pInfo, mvCurr+2, fcode) != MP4_STATUS_OK)
            return MP4_STATUS_ERROR;
        // block 3
        mvCurr[3].dx = mp4_Median(mvCurr[2].dx, mvCurr[0].dx, mvCurr[1].dx);
        mvCurr[3].dy = mp4_Median(mvCurr[2].dy, mvCurr[0].dy, mvCurr[1].dy);
        if (mp4_DecodeMV(pInfo, mvCurr+3, fcode) != MP4_STATUS_OK)
            return MP4_STATUS_ERROR;
    } else {
        int   validLeft, validTop, validRight;

        if (x > 0)
            validLeft = MBcurr[-1].validPred;
        else
            validLeft = 0;
        if (y > 0)
            validTop = MBcurr[-mbInRow].validPred;
        else
            validTop = 0;
        if ((y > 0) && (x < mbInRow - 1))
            validRight = MBcurr[-mbInRow+1].validPred;
        else
            validRight = 0;
        // block 0
        switch ((validLeft << 2) | (validTop << 1) | validRight) {
        case 7:
            mvCurr[0].dx = mp4_Median(mvLeft[1].dx, mvTop[2].dx, mvRight[2].dx);
            mvCurr[0].dy = mp4_Median(mvLeft[1].dy, mvTop[2].dy, mvRight[2].dy);
            break;
        case 6:
            mvCurr[0].dx = mp4_Median(mvLeft[1].dx, mvTop[2].dx, 0);
            mvCurr[0].dy = mp4_Median(mvLeft[1].dy, mvTop[2].dy, 0);
            break;
        case 5:
            mvCurr[0].dx = mp4_Median(mvLeft[1].dx, 0, mvRight[2].dx);
            mvCurr[0].dy = mp4_Median(mvLeft[1].dy, 0, mvRight[2].dy);
            break;
        case 4:
            mvCurr[0] = mvLeft[1];
            break;
        case 3:
            mvCurr[0].dx = mp4_Median(0, mvTop[2].dx, mvRight[2].dx);
            mvCurr[0].dy = mp4_Median(0, mvTop[2].dy, mvRight[2].dy);
            break;
        case 2:
            mvCurr[0] = mvTop[2];
            break;
        case 1:
            mvCurr[0] = mvRight[2];
            break;
        default:
            mvCurr[0].dx = mvCurr[0].dy = 0;
            break;
        }
        if (mp4_DecodeMV(pInfo, mvCurr, fcode) != MP4_STATUS_OK)
            return MP4_STATUS_ERROR;
        // block 1
        switch ((validTop << 1) | validRight) {
        case 3:
            mvCurr[1].dx = mp4_Median(mvCurr[0].dx, mvTop[3].dx, mvRight[2].dx);
            mvCurr[1].dy = mp4_Median(mvCurr[0].dy, mvTop[3].dy, mvRight[2].dy);
            break;
        case 2:
            mvCurr[1].dx = mp4_Median(mvCurr[0].dx, mvTop[3].dx, 0);
            mvCurr[1].dy = mp4_Median(mvCurr[0].dy, mvTop[3].dy, 0);
            break;
        case 1:
            mvCurr[1].dx = mp4_Median(mvCurr[0].dx, 0, mvRight[2].dx);
            mvCurr[1].dy = mp4_Median(mvCurr[0].dy, 0, mvRight[2].dy);
            break;
        default:
            mvCurr[1] = mvCurr[0];
            break;
        }
        if (mp4_DecodeMV(pInfo, mvCurr+1, fcode) != MP4_STATUS_OK)
            return MP4_STATUS_ERROR;
        // block 2
        if (validLeft) {
            mvCurr[2].dx = mp4_Median(mvLeft[3].dx, mvCurr[0].dx, mvCurr[1].dx);
            mvCurr[2].dy = mp4_Median(mvLeft[3].dy, mvCurr[0].dy, mvCurr[1].dy);
        } else {
            mvCurr[2].dx = mp4_Median(0, mvCurr[0].dx, mvCurr[1].dx);
            mvCurr[2].dy = mp4_Median(0, mvCurr[0].dy, mvCurr[1].dy);
        }
        if (mp4_DecodeMV(pInfo, mvCurr+2, fcode) != MP4_STATUS_OK)
            return MP4_STATUS_ERROR;
        // block 3
        mvCurr[3].dx = mp4_Median(mvCurr[2].dx, mvCurr[0].dx, mvCurr[1].dx);
        mvCurr[3].dy = mp4_Median(mvCurr[2].dy, mvCurr[0].dy, mvCurr[1].dy);
        if (mp4_DecodeMV(pInfo, mvCurr+3, fcode) != MP4_STATUS_OK)
            return MP4_STATUS_ERROR;
    }
    return MP4_STATUS_OK;
}

#ifndef DROP_FIELD
mp4_Status mp4_PredictDecodeFMV(mp4_Info *pInfo, mp4_MacroBlock *MBcurr, int y, int x, IppMotionVector *mvT, IppMotionVector *mvB)
{
    IppMotionVector  mvLeft, mvTop, mvRight, mvPred;
    int              mbInRow = pInfo->VisualObject.VideoObject.MacroBlockPerRow;
    int              fcode = pInfo->VisualObject.VideoObject.VideoObjectPlane.fcode_forward;
    int              resync_marker_disable = pInfo->VisualObject.VideoObject.resync_marker_disable;

    if (x > 0) {
        mvLeft = MBcurr[-1].mv[1];
        mvLeft.dy = (Ipp16s)mp4_Div2(mvLeft.dy);
    }
    if (y > 0) {
        mvTop = MBcurr[-mbInRow].mv[2];
        mvTop.dy = (Ipp16s)mp4_Div2(mvTop.dy);
        if (x < (mbInRow - 1)) {
            mvRight = MBcurr[-mbInRow+1].mv[2];
            mvRight.dy = (Ipp16s)mp4_Div2(mvRight.dy);
        }
    }
    if (resync_marker_disable) {
        if ((y | x) == 0) {
            mvPred.dx = mvPred.dy = 0;
        } else if (x == 0) {
            mvPred.dx = mp4_Median(0, mvTop.dx, mvRight.dx);
            mvPred.dy = mp4_Median(0, mvTop.dy, mvRight.dy);
        } else if (y == 0) {
            mvPred = mvLeft;
        } else if (x == mbInRow - 1) {
            mvPred.dx = mp4_Median(0, mvLeft.dx, mvTop.dx);
            mvPred.dy = mp4_Median(0, mvLeft.dy, mvTop.dy);
        } else {
            mvPred.dx = mp4_Median(mvLeft.dx, mvTop.dx, mvRight.dx);
            mvPred.dy = mp4_Median(mvLeft.dy, mvTop.dy, mvRight.dy);
        }
    } else {
        int   validLeft, validTop, validRight;

        if (x > 0)
            validLeft = MBcurr[-1].validPred;
        else
            validLeft = 0;
        if (y > 0)
            validTop = MBcurr[-mbInRow].validPred;
        else
            validTop = 0;
        if ((y > 0) && (x < mbInRow - 1))
            validRight = MBcurr[-mbInRow+1].validPred;
        else
            validRight = 0;
        switch ((validLeft << 2) | (validTop << 1) | validRight) {
        case 7:
            mvPred.dx = mp4_Median(mvLeft.dx, mvTop.dx, mvRight.dx);
            mvPred.dy = mp4_Median(mvLeft.dy, mvTop.dy, mvRight.dy);
            break;
        case 6:
            mvPred.dx = mp4_Median(mvLeft.dx, mvTop.dx, 0);
            mvPred.dy = mp4_Median(mvLeft.dy, mvTop.dy, 0);
            break;
        case 5:
            mvPred.dx = mp4_Median(mvLeft.dx, 0, mvRight.dx);
            mvPred.dy = mp4_Median(mvLeft.dy, 0, mvRight.dy);
            break;
        case 4:
            mvPred = mvLeft;
            break;
        case 3:
            mvPred.dx = mp4_Median(0, mvTop.dx, mvRight.dx);
            mvPred.dy = mp4_Median(0, mvTop.dy, mvRight.dy);
            break;
        case 2:
            mvPred = mvTop;
            break;
        case 1:
            mvPred = mvRight;
            break;
        default:
            mvPred.dx = mvPred.dy = 0;
            break;
        }
    }
    *mvT = mvPred;
    if (mp4_DecodeMV(pInfo, mvT, fcode) != MP4_STATUS_OK) {
        mp4_Error("Error when decode field motion vector");
        return MP4_STATUS_ERROR;
    }
    *mvB = mvPred;
    if (mp4_DecodeMV(pInfo, mvB, fcode) != MP4_STATUS_OK) {
        mp4_Error("Error when decode field motion vector");
        return MP4_STATUS_ERROR;
    }
    // update MV buffer for future prediction
    MBcurr->mv[0].dx = MBcurr->mv[1].dx = MBcurr->mv[2].dx = MBcurr->mv[3].dx = (Ipp16s)mp4_Div2Round(mvT->dx + mvB->dx);
    MBcurr->mv[0].dy = MBcurr->mv[1].dy = MBcurr->mv[2].dy = MBcurr->mv[3].dy = (Ipp16s)(mvT->dy + mvB->dy);
    return MP4_STATUS_OK;
}
#endif


void mp4_OBMC(mp4_Info *pInfo, mp4_MacroBlock *pMBinfo, IppMotionVector *mvCur, int colNum, int rowNum, IppiRect limitRectL, Ipp8u *pYc, int stepYc, Ipp8u *pYr, int stepYr, int cbpy, Ipp16s *coeffMB, int dct_type)
{
    IppMotionVector mvOBMCL, mvOBMCU, mvOBMCR, mvOBMCB, *mvLeft, *mvUpper, *mvRight;
    int  mbPerRow = pInfo->VisualObject.VideoObject.MacroBlockPerRow, dx, dy, rt;

    // get Right MV
    if (colNum == mbPerRow - 1)
        mvRight = &mvCur[1];
    else if (pMBinfo[1].type >= IPPVC_MBTYPE_INTRA)
        mvRight = &mvCur[1];
    else
        mvRight = pMBinfo[1].mv;
    // get Left MV
    if (colNum == 0)
        mvLeft = mvCur - 1;
    else if (pMBinfo[-1].type >= IPPVC_MBTYPE_INTRA)
        mvLeft = mvCur - 1;
    else
        mvLeft = pMBinfo[-1].mv;
    // get Upper MV
    if (rowNum == 0)
        mvUpper = mvCur - 2;
    else if (pMBinfo[-mbPerRow].type >= IPPVC_MBTYPE_INTRA)
        mvUpper = mvCur - 2;
    else
        mvUpper = pMBinfo[-mbPerRow].mv;
    dx = colNum * 16;
    dy = rowNum * 16;
    rt = pInfo->VisualObject.VideoObject.VideoObjectPlane.rounding_type;
    if (pInfo->VisualObject.VideoObject.quarter_sample) {
        mp4_LimitMVQ(&mvLeft[1], &mvOBMCL, &limitRectL, dx, dy, 8);
        mp4_LimitMVQ(&mvUpper[2], &mvOBMCU, &limitRectL, dx, dy, 8);
        mp4_LimitMVQ(&mvCur[1], &mvOBMCR, &limitRectL, dx, dy, 8);
        mp4_LimitMVQ(&mvCur[2], &mvOBMCB, &limitRectL, dx, dy, 8);
        ippiOBMC8x8QP_MPEG4_8u_C1R_x(pYr, stepYr, pYc, stepYc, &mvCur[0], &mvOBMCL, &mvOBMCR, &mvOBMCU, &mvOBMCB, rt);
        mp4_LimitMVQ(&mvCur[0], &mvOBMCL, &limitRectL, dx+8, dy, 8);
        mp4_LimitMVQ(&mvUpper[3], &mvOBMCU, &limitRectL, dx+8, dy, 8);
        mp4_LimitMVQ(&mvRight[0], &mvOBMCR, &limitRectL, dx+8, dy, 8);
        mp4_LimitMVQ(&mvCur[3], &mvOBMCB, &limitRectL, dx+8, dy, 8);
        ippiOBMC8x8QP_MPEG4_8u_C1R_x(pYr+8, stepYr, pYc+8, stepYc, &mvCur[1], &mvOBMCL, &mvOBMCR, &mvOBMCU, &mvOBMCB, rt);
        mp4_LimitMVQ(&mvLeft[3], &mvOBMCL, &limitRectL, dx, dy+8, 8);
        mp4_LimitMVQ(&mvCur[0], &mvOBMCU, &limitRectL, dx, dy+8, 8);
        mp4_LimitMVQ(&mvCur[3], &mvOBMCR, &limitRectL, dx, dy+8, 8);
        ippiOBMC8x8QP_MPEG4_8u_C1R_x(pYr+stepYr*8, stepYr, pYc+stepYc*8, stepYc, &mvCur[2], &mvOBMCL, &mvOBMCR, &mvOBMCU, &mvCur[2], rt);
        mp4_LimitMVQ(&mvCur[2], &mvOBMCL, &limitRectL, dx+8, dy+8, 8);
        mp4_LimitMVQ(&mvCur[1], &mvOBMCU, &limitRectL, dx+8, dy+8, 8);
        mp4_LimitMVQ(&mvRight[2], &mvOBMCR, &limitRectL, dx+8, dy+8, 8);
        ippiOBMC8x8QP_MPEG4_8u_C1R_x(pYr+8+stepYr*8, stepYr, pYc+8+stepYc*8, stepYc, &mvCur[3], &mvOBMCL, &mvOBMCR, &mvOBMCU, &mvCur[3], rt);
    } else {
        mp4_LimitMV(&mvLeft[1], &mvOBMCL, &limitRectL, dx, dy, 8);
        mp4_LimitMV(&mvUpper[2], &mvOBMCU, &limitRectL, dx, dy, 8);
        mp4_LimitMV(&mvCur[1], &mvOBMCR, &limitRectL, dx, dy, 8);
        mp4_LimitMV(&mvCur[2], &mvOBMCB, &limitRectL, dx, dy, 8);
        ippiOBMC8x8HP_MPEG4_8u_C1R_x(pYr, stepYr, pYc, stepYc, &mvCur[0], &mvOBMCL, &mvOBMCR, &mvOBMCU, &mvOBMCB, rt);
        mp4_LimitMV(&mvCur[0], &mvOBMCL, &limitRectL, dx+8, dy, 8);
        mp4_LimitMV(&mvUpper[3], &mvOBMCU, &limitRectL, dx+8, dy, 8);
        mp4_LimitMV(&mvRight[0], &mvOBMCR, &limitRectL, dx+8, dy, 8);
        mp4_LimitMV(&mvCur[3], &mvOBMCB, &limitRectL, dx+8, dy, 8);
        ippiOBMC8x8HP_MPEG4_8u_C1R_x(pYr+8, stepYr, pYc+8, stepYc, &mvCur[1], &mvOBMCL, &mvOBMCR, &mvOBMCU, &mvOBMCB, rt);
        mp4_LimitMV(&mvLeft[3], &mvOBMCL, &limitRectL, dx, dy+8, 8);
        mp4_LimitMV(&mvCur[0], &mvOBMCU, &limitRectL, dx, dy+8, 8);
        mp4_LimitMV(&mvCur[3], &mvOBMCR, &limitRectL, dx, dy+8, 8);
        ippiOBMC8x8HP_MPEG4_8u_C1R_x(pYr+stepYr*8, stepYr, pYc+stepYc*8, stepYc, &mvCur[2], &mvOBMCL, &mvOBMCR, &mvOBMCU, &mvCur[2], rt);
        mp4_LimitMV(&mvCur[2], &mvOBMCL, &limitRectL, dx+8, dy+8, 8);
        mp4_LimitMV(&mvCur[1], &mvOBMCU, &limitRectL, dx+8, dy+8, 8);
        mp4_LimitMV(&mvRight[2], &mvOBMCR, &limitRectL, dx+8, dy+8, 8);
        ippiOBMC8x8HP_MPEG4_8u_C1R_x(pYr+8+stepYr*8, stepYr, pYc+8+stepYc*8, stepYc, &mvCur[3], &mvOBMCL, &mvOBMCR, &mvOBMCU, &mvCur[3], rt);
    }
    if (!dct_type) {
        mp4_AddResidual(cbpy & 8, pYc, stepYc, coeffMB);
        mp4_AddResidual(cbpy & 4, pYc+8, stepYc, coeffMB+64);
        mp4_AddResidual(cbpy & 2, pYc+stepYc*8, stepYc, coeffMB+128);
        mp4_AddResidual(cbpy & 1, pYc+stepYc*8+8, stepYc, coeffMB+192);
    } else {
        mp4_AddResidual(cbpy & 8, pYc, stepYc*2, coeffMB);
        mp4_AddResidual(cbpy & 4, pYc+8, stepYc*2, coeffMB+64);
        mp4_AddResidual(cbpy & 2, pYc+stepYc, stepYc*2, coeffMB+128);
        mp4_AddResidual(cbpy & 1, pYc+stepYc+8, stepYc*2, coeffMB+192);
    }
}





void mp4_DecodeBABtype(mp4_Info* pInfo, int colNum, int rowNum, mp4_ShapeInfo *curShapeInfo, int mbPerRow)
{
    int     c, i, typeTL, typeT, typeTR, typeL;
    Ipp32u  code;

    typeTL = (colNum == 0 || rowNum == 0) ? MP4_BAB_TYPE_TRANSPARENT : curShapeInfo[-mbPerRow - 1].bab_type;
    typeT  = (rowNum == 0) ? MP4_BAB_TYPE_TRANSPARENT : curShapeInfo[-mbPerRow].bab_type;
    typeTR = (colNum == mbPerRow - 1 || rowNum == 0) ? MP4_BAB_TYPE_TRANSPARENT : curShapeInfo[-mbPerRow + 1].bab_type;
    typeL  = (colNum == 0) ? MP4_BAB_TYPE_TRANSPARENT : curShapeInfo[-1].bab_type;
    code = mp4_ShowBits9(pInfo, 3);
    i = (code >= 4) ? 0 : (code >= 2) ? 1 : 2;
    c = 27 * (typeTL - 2) + 9 * (typeT - 2) + 3 * (typeTR - 2) + (typeL - 2);
    mp4_FlushBits(pInfo, i+1);
    curShapeInfo->bab_type = mp4_BABtypeIntra[c][i];
}


Ipp8u mp4_CheckTransparency(Ipp8u *p, int step)
{
    int    i, j, s0, s1, s2, s3;

    s0 = s1 = s2 = s3 = 0;
    for (i = 0; i < 8; i ++) {
        for (j = 0; j < 8; j ++)
            s0 += p[j];
        for (j = 8; j < 16; j ++)
            s1 += p[j];
        p += step;
    }
    for (i = 0; i < 8; i ++) {
        for (j = 0; j < 8; j ++)
            s2 += p[j];
        for (j = 8; j < 16; j ++)
            s3 += p[j];
        p += step;
    }
    i = (s0 == 0) ? 0 : 1;
    if (s1 != 0)
        i += 2;
    if (s2 != 0)
        i += 4;
    if (s3 != 0)
        i += 8;
    return (Ipp8u)i;
}


#ifdef _OMP_KARABAS

int mp4_GetNumOfThreads(void)
{
    int maxThreads = 1;
#ifdef _OPENMP
#pragma omp parallel shared(maxThreads)
#endif
    {
#ifdef _OPENMP
#pragma omp master
#endif
    {
#ifdef _OPENMP
        maxThreads = omp_get_num_threads();
#endif
    }
    }
    return maxThreads;
}

#endif // _OMP_KARABAS
