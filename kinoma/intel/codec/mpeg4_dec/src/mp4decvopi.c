/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2001-2006 Intel Corporation. All Rights Reserved.
//
//  Description:    Decodes I-VOPs
//
*/

//***
#include "kinoma_ipp_lib.h"

#include "mp4.h"
#include "mp4dec.h"


/*
//  decode mcbpc and set MBtype and ChromaPattern
*/
static mp4_Status mp4_DecodeMCBPC_I(mp4_Info* pInfo, int *mbType, int *mbPattern)
{
    Ipp32u      code;
    int         type, pattern, fb;

    code = mp4_ShowBits9(pInfo, 9);
    if (code == 1) {
        type = IPPVC_MB_STUFFING;
        pattern = 0;
        fb = 9;
    } else if (code >= 64) {
        type = IPPVC_MBTYPE_INTRA;
        pattern = code >> 6;
        if (pattern >= 4) {
            pattern = 0;
            fb = 1;
        } else
            fb = 3;
    } else {
        type = IPPVC_MBTYPE_INTRA_Q;
        pattern = code >> 3;
        if (pattern >= 4) {
            pattern = 0;
            fb = 4;
        } else if (code >= 8) {
            fb = 6;
        } else {
            mp4_Error("Error when decode mcbpc of I-VOP macroblock");
            return MP4_STATUS_ERROR;
        }
    }
    mp4_FlushBits(pInfo, fb);
    *mbType = type;
    *mbPattern = pattern;
    if (type == IPPVC_MBTYPE_INTRA)
        mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB_INTRA);
    else if (type == IPPVC_MBTYPE_INTRA_Q)
        mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB_INTRA_Q);
    return MP4_STATUS_OK;
}


/*
//  decode cbpy for Intra nontransparent MB
*/
__INLINE mp4_Status mp4_DecodeCBPY_I(mp4_Info* pInfo, int *yPattern)
{
    Ipp32u      code;

    code = mp4_ShowBits9(pInfo, 6);
    *yPattern = mp4_cbpy4[code].code;
    if (mp4_cbpy4[code].len == 255) {
        mp4_Error("Error when decode cbpy of I-VOP macroblock");
        return MP4_STATUS_ERROR;
    } else {
        mp4_FlushBits(pInfo, mp4_cbpy4[code].len);
        return MP4_STATUS_OK;
    }
}


/*
//  decode cbpy for Intra Shape MB
*/
__INLINE mp4_Status mp4_DecodeCBPY_I_Shape(mp4_Info* pInfo, int nNonTransp, int *yPattern)
{
    Ipp32u      code, i;

    i = nNonTransp - 1;
    code = mp4_ShowBits9(pInfo, mp4_cbpy_b[i]);
    *yPattern = mp4_cbpy_t[i][code].code;
    if (mp4_cbpy_t[i][code].len == 255) {
        mp4_Error("Error when decode cbpy of I-VOP with Shape macroblock");
        return MP4_STATUS_ERROR;
    } else {
        mp4_FlushBits(pInfo, mp4_cbpy_t[i][code].len);
        return MP4_STATUS_OK;
    }
}


/*
//  decode MPEG-4 IVOP with Shape
*/
mp4_Status mp4_DecodeVOP_I_Shape(mp4_Info* pInfo)
{
/*
    Ipp32u      code;
    int         quant, quantPred, i, dcVLC, nmb, mbPerRow, mbPerCol;
    int         stepYc, stepCbc, stepCrc;
    IppStatus   status;
    Ipp8u      *pY, *pCb, *pCr, *pQuantMatIntra, *ppbq, *pB;
    Ipp16s     *ppbrY, *ppbrCb, *ppbrCr, *ppbcY, *ppbcCb, *ppbcCr;
    mp4_MacroBlock   MacroBlock = {0};
    int         conv_ratio, scan_type, blockSize, nOpaq, cbpy;
    int         mbCurr, mbInVideoPacket, colNum, rowNum, k;
    mp4_ShapeInfo *curShapeInfo;

    status = mp4_InitVOPShape(pInfo);
    if (status != MP4_STATUS_OK)
        return status;
    stepYc = pInfo->VisualObject.cFrame.stepY;
    stepCbc = pInfo->VisualObject.cFrame.stepCb;
    stepCrc = pInfo->VisualObject.cFrame.stepCr;
    mbPerRow = pInfo->VisualObject.cFrame.mbPerRow;
    mbPerCol = pInfo->VisualObject.cFrame.mbPerCol;
    pQuantMatIntra = pInfo->VisualObject.VideoObject.quant_type ? pInfo->VisualObject.VideoObject.intra_quant_mat : NULL;
    // for B-VOP
    ippsZero_8u_x(pInfo->VisualObject.VideoObject.MacroBlockNotCoded, mbPerCol*mbPerRow);
    ippsZero_8u_x((Ipp8u*)pInfo->VisualObject.VideoObject.MVfull, mbPerCol*mbPerRow*4*sizeof(IppMotionVector));
    // needed for ippVC predict mechanism
    ppbcY = pInfo->VisualObject.VideoObject.PredictBuffCol_Y;
    ppbcCb = pInfo->VisualObject.VideoObject.PredictBuffCol_Cb;
    ppbcCr = pInfo->VisualObject.VideoObject.PredictBuffCol_Cr;
    quant = pInfo->VisualObject.VideoObject.VideoObjectPlane.quant;
    curShapeInfo = pInfo->VisualObject.VideoObject.ShapeInfo;
    conv_ratio = 1;
    nmb = pInfo->VisualObject.VideoObject.MacroBlockPerVOP;
// decode data_partitioned I-VOP
    if (pInfo->VisualObject.VideoObject.data_partitioned) {
        mp4_MacroBlock *pMacroBlock;

        mbCurr = 0;
        colNum = rowNum = 0;
        // avoid warning C4701
        pY = pCb = pCr = ppbq = 0; ppbrY = ppbrCb = ppbrCr = 0;
        for (;;) {
            // reset ACDC predict buffer on new Video_packet
            for (i = 0; i <= mbPerRow; i ++) {
                pInfo->VisualObject.VideoObject.PredictBuffRow_Y[i*16] = -1;
                pInfo->VisualObject.VideoObject.PredictBuffRow_Y[i*16+8] = -1;
                pInfo->VisualObject.VideoObject.PredictBuffRow_Cb[i*8] = -1;
                pInfo->VisualObject.VideoObject.PredictBuffRow_Cr[i*8] = -1;
            }
            ppbcY[0] = ppbcY[8] = ppbcCb[0] = ppbcCr[0] = -1;
            pMacroBlock = &pInfo->VisualObject.VideoObject.MBfull[mbCurr];
            mbInVideoPacket = 0;
            // decode dc part
            for (;;) {
                // decode mcbpc
                if (mp4_DecodeMCBPC_I(pInfo, &pMacroBlock->type, &pMacroBlock->cbpc) != MP4_STATUS_OK)
                    return MP4_STATUS_ERROR;
                if (pMacroBlock->type != IPPVC_MB_STUFFING) {
                    quantPred = quant;
                    // decode dquant
                    if (pMacroBlock->type == IPPVC_MBTYPE_INTRA_Q)
                        mp4_UpdateQuant(pInfo, quant);
                    pMacroBlock->dquant = quant;
                    if (mbInVideoPacket == 0)
                        quantPred = quant;
                    dcVLC = (quantPred < mp4_DC_vlc_Threshold[pInfo->VisualObject.VideoObject.VideoObjectPlane.intra_dc_vlc_thr]) ? 1 : 0;
                    // decode DCs
                    if (dcVLC) {
                        for (k= 0; k < 6; k ++) {
                            status = ippiDecodeVLC_IntraDCVLC_MPEG4_1u16s(&pInfo->bufptr, &pInfo->bitoff, &pMacroBlock->dct_dc[k], k < 4 ? IPPVC_LUMINANCE : IPPVC_CHROMINANCE);
                            if (status != ippStsNoErr) {
                                mp4_Error("Error when decode coefficients of Intra block");
                                return MP4_STATUS_ERROR;
                            }
                        }
                    }
                    pMacroBlock ++;
                    mbInVideoPacket ++;
                }
                if (mp4_ShowBits(pInfo, 19) == MP4_DC_MARKER) {
                    code = mp4_GetBits(pInfo, 19);
                    break;
                }
            }
            pMacroBlock = &pInfo->VisualObject.VideoObject.MBfull[mbCurr];
            for (i = 0; i < mbInVideoPacket; i ++) {
                // decode ac_pred_flag
                pMacroBlock->ac_pred_flag = mp4_GetBit(pInfo);
                // decode cbpy
                if (mp4_DecodeCBPY_I(pInfo, &pMacroBlock->cbpy) != MP4_STATUS_OK)
                    return MP4_STATUS_ERROR;
                pMacroBlock ++;
            }
            pMacroBlock = &pInfo->VisualObject.VideoObject.MBfull[mbCurr];
            for (i = 0; i < mbInVideoPacket; i ++) {
                if (colNum == 0) {
                    pY = pInfo->VisualObject.cFrame.pY + rowNum * 16 * stepYc;
                    pCb = pInfo->VisualObject.cFrame.pCb + rowNum * 8 * stepCbc;
                    pCr = pInfo->VisualObject.cFrame.pCr + rowNum * 8 * stepCrc;
                    // needed for ippVC predict mechanism
                    if (rowNum)
                        ppbrY[-8] = ppbcY[8];   / * Update DC part of last MB per line * /
                    ppbrY = pInfo->VisualObject.VideoObject.PredictBuffRow_Y + 16;
                    ppbrCb = pInfo->VisualObject.VideoObject.PredictBuffRow_Cb + 8;
                    ppbrCr = pInfo->VisualObject.VideoObject.PredictBuffRow_Cr + 8;
                    ppbcY[0] = ppbcY[8] = ppbcCb[0] = ppbcCr[0] = -1;
                    ppbq = pInfo->VisualObject.VideoObject.PredictBuffQuant;
                }
                quant = pMacroBlock->dquant;
                quantPred = (i == 0) ? quant : pMacroBlock[-1].dquant;
                dcVLC = (quantPred < mp4_DC_vlc_Threshold[pInfo->VisualObject.VideoObject.VideoObjectPlane.intra_dc_vlc_thr]) ? 1 : 0;
                // decode ACs and reconstruct blocks
                if ((status = mp4_ReconBlockIntraDP_MPEG4(pInfo, pMacroBlock, dcVLC, pY, stepYc, ppbrY, ppbcY, ppbq, quant, 0, pInfo->VisualObject.VideoObject.reversible_vlc, pQuantMatIntra, pMacroBlock->cbpy & 8)) != ippStsNoErr)
                    return MP4_STATUS_ERROR;
                if ((status = mp4_ReconBlockIntraDP_MPEG4(pInfo, pMacroBlock, dcVLC, pY+8, stepYc, ppbrY+8, ppbcY, ppbq, quant, 1, pInfo->VisualObject.VideoObject.reversible_vlc, pQuantMatIntra, pMacroBlock->cbpy & 4)) != ippStsNoErr)
                    return MP4_STATUS_ERROR;
                if ((status = mp4_ReconBlockIntraDP_MPEG4(pInfo, pMacroBlock, dcVLC, pY+8*stepYc, stepYc, ppbrY, ppbcY+8, ppbq, quant, 2, pInfo->VisualObject.VideoObject.reversible_vlc, pQuantMatIntra, pMacroBlock->cbpy & 2)) != ippStsNoErr)
                    return MP4_STATUS_ERROR;
                if ((status = mp4_ReconBlockIntraDP_MPEG4(pInfo, pMacroBlock, dcVLC, pY+8*stepYc+8, stepYc, ppbrY+8, ppbcY+8, ppbq, quant, 3, pInfo->VisualObject.VideoObject.reversible_vlc, pQuantMatIntra, pMacroBlock->cbpy & 1)) != ippStsNoErr)
                    return MP4_STATUS_ERROR;
                if ((status = mp4_ReconBlockIntraDP_MPEG4(pInfo, pMacroBlock, dcVLC, pCb, stepCbc, ppbrCb, ppbcCb, ppbq, quant, 4, pInfo->VisualObject.VideoObject.reversible_vlc, pQuantMatIntra, pMacroBlock->cbpc & 2)) != ippStsNoErr)
                    return MP4_STATUS_ERROR;
                if ((status = mp4_ReconBlockIntraDP_MPEG4(pInfo, pMacroBlock, dcVLC, pCr, stepCrc, ppbrCr, ppbcCr, ppbq, quant, 5, pInfo->VisualObject.VideoObject.reversible_vlc, pQuantMatIntra, pMacroBlock->cbpc & 1)) != ippStsNoErr)
                    return MP4_STATUS_ERROR;
                // update predict buffers
                ppbq ++; ppbq[0] = (Ipp8u)quant;
                pY += 16; pCr += 8; pCb += 8;
                ppbrY += 16; ppbrCb += 8; ppbrCr += 8;
                mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB);
                pMacroBlock ++;
                colNum ++;
                if (colNum == mbPerRow) {
                    colNum = 0;
                    rowNum ++;
                }
            }
            mbCurr += mbInVideoPacket;
            if (mbCurr == nmb)
                break;
            if (!pInfo->VisualObject.VideoObject.resync_marker_disable)
                if (!mp4_DecodeVideoPacket(pInfo, &quant))
                    break;
        }
        return MP4_STATUS_OK;
    }
// decode not data partitioned I-VOP
    else {
        int         stepY, dct_type, pYoff2, pYoff3, nAux;
        Ipp8u      *pQuantMatIntraA[3], *ppbqA[3], *pA[3];
        Ipp16s     *ppbrA[3], *ppbcA[3];

        nAux = mp4_aux_comp_count[pInfo->VisualObject.VideoObject.shape_extension];
        mbCurr = 0;
        // avoid warning C4701
        pY = pCb = pCr = ppbq = pB = 0; ppbrY = ppbrCb = ppbrCr = 0;
        for (k = 0; k < nAux; k ++) {
            pA[k] = 0;
            ppbqA[k] = 0;
            ppbrA[k] = 0;
            pQuantMatIntraA[k] = pInfo->VisualObject.VideoObject.quant_type ? pInfo->VisualObject.VideoObject.intra_quant_mat_grayscale[k] : NULL;
        }
        colNum = rowNum = 0;
        dct_type = 0;
        stepY = stepYc;
        pYoff2 = 8 * stepYc;
        pYoff3 = 8 * stepYc + 8;
        for (;;) {
            // reset ACDC predict buffer on new Video_packet
            for (i = 0; i <= mbPerRow; i ++) {
                pInfo->VisualObject.VideoObject.PredictBuffRow_Y[i*16] = -1;
                pInfo->VisualObject.VideoObject.PredictBuffRow_Y[i*16+8] = -1;
                pInfo->VisualObject.VideoObject.PredictBuffRow_Cb[i*8] = -1;
                pInfo->VisualObject.VideoObject.PredictBuffRow_Cr[i*8] = -1;
                for (k = 0; k < nAux; k ++) {
                    pInfo->VisualObject.VideoObject.PredictBuffRow_A[k][i*16] = -1;
                    pInfo->VisualObject.VideoObject.PredictBuffRow_A[k][i*16+8] = -1;
                }
            }
            ppbcY[0] = ppbcY[8] = ppbcCb[0] = ppbcCr[0] = -1;
            for (k = 0; k < nAux; k ++) {
                ppbcA[k][0] = -1;
            }
            mbInVideoPacket = 0;
            // decode blocks
            for (;;) {
                if (colNum == 0) {
                    pY = pInfo->VisualObject.cFrame.pY + rowNum * 16 * stepYc;
                    pCb = pInfo->VisualObject.cFrame.pCb + rowNum * 8 * stepCbc;
                    pCr = pInfo->VisualObject.cFrame.pCr + rowNum * 8 * stepCrc;
                    ppbrY = pInfo->VisualObject.VideoObject.PredictBuffRow_Y + 16;
                    ppbrCb = pInfo->VisualObject.VideoObject.PredictBuffRow_Cb + 8;
                    ppbrCr = pInfo->VisualObject.VideoObject.PredictBuffRow_Cr + 8;
                    ppbcY[0] = ppbcY[8] = ppbcCb[0] = ppbcCr[0] = -1;
                    ppbq = pInfo->VisualObject.VideoObject.PredictBuffQuant;
                    pB = pInfo->VisualObject.cFrame.pB + rowNum * 16 * stepYc;
                    for (k = 0; k < nAux; k ++) {
                        pA[k] = pInfo->VisualObject.cFrame.pA[k] + rowNum * 16 * stepYc;
                        ppbrA[k] = pInfo->VisualObject.VideoObject.PredictBuffRow_A[k] + 16;
                        ppbcA[k][0] = ppbcA[k][8] = -1;
                        ppbqA[k] = pInfo->VisualObject.VideoObject.PredictBuffQuant_A[k];
                    }
                }
                // decode BAB
                mp4_DecodeBABtype(pInfo, colNum, rowNum, curShapeInfo, mbPerRow);
                if (curShapeInfo->bab_type == MP4_BAB_TYPE_INTRACAE) {
                    if (!pInfo->VisualObject.VideoObject.VideoObjectPlane.change_conv_ratio_disable)
                        conv_ratio = mp4_GetConvRatio(pInfo);
                    blockSize = conv_ratio == 1 ? 16 : conv_ratio == 2 ? 8 : 4;
                    scan_type = mp4_GetBit(pInfo);
                    {
                        Ipp8u  tb[18*4], *p;
                        int    i, j;

                        p = pB - 2 * stepYc - 2;
                        for (i = 0; i < 18; i ++) {
                            tb[i*4] = 0;
                            if (p[0])
                                tb[i*4] |= 2;
                            if (p[1])
                                tb[i*4] |= 1;
                            tb[i*4+1] = 0;
                            for (j = 0; j < 8; j ++) {
                                if (p[j+2])
                                    tb[i*4+1] |= (1 << (7 - j));
                            }
                            tb[i*4+2] = 0;
                            for (j = 0; j < 8; j ++) {
                                if (p[j+2+8])
                                    tb[i*4+2] |= (1 << (7 - j));
                            }
                            tb[i*4+3] = 0;
                            if (p[2+16])
                                tb[i*4+3] |= 128;
                            if (p[2+17])
                                tb[i*4+3] |= 64;
                            p += stepYc;
                        }
                        p = tb + 4 * 2 + 1;
                        if (scan_type == 1)
                            ippiDecodeCAEIntraH_MPEG4_1u8u(&pInfo->bufptr, &pInfo->bitoff, p, 4, blockSize);
                        else
                            ippiDecodeCAEIntraV_MPEG4_1u8u(&pInfo->bufptr, &pInfo->bitoff, p, 4, blockSize);
                        for (i = 0; i < 16; i ++) {
                            for (j = 0; j < 8; j ++) {
                                pB[i*stepYc+j] = (Ipp8u)((tb[2*4+i*4+1] & (1 << (7 - j))) ? 255 : 0);
                            }
                            for (j = 0; j < 8; j ++) {
                                pB[i*stepYc+j+8] = (Ipp8u)((tb[2*4+i*4+2] & (1 << (7 - j))) ? 255 : 0);
                            }
                        }
                    }
                    curShapeInfo->opaque = mp4_CheckTransparency(pB, stepYc);
                } else {
                    if (curShapeInfo->bab_type == MP4_BAB_TYPE_TRANSPARENT) {
                        mp4_Set16x16_8u(pB, stepYc, 0);
                        curShapeInfo->opaque = 0;
                        ppbcY[0] = ppbrY[8];
                        ppbrY[-8] = ppbcY[8];
                        ppbrY[0] = ppbrY[8] = ppbcY[8] = -1;
                        ppbcCb[0] = ppbrCb[0];
                        ppbrCb[0] = -1;
                        ppbcCr[0] = ppbrCr[0];
                        ppbrCr[0] = -1;
                    } else {
                        mp4_Set16x16_8u(pB, stepYc, 255);
                        curShapeInfo->opaque = 15;
                    }
                }
                if (pInfo->VisualObject.VideoObject.shape != MP4_SHAPE_TYPE_BINARYONLY && curShapeInfo->bab_type != MP4_BAB_TYPE_TRANSPARENT) {
                    // decode mcbpc
                    do {
                        if (mp4_DecodeMCBPC_I(pInfo, &MacroBlock.type, &MacroBlock.cbpc) != MP4_STATUS_OK)
                            return MP4_STATUS_ERROR;
                    } while (MacroBlock.type == IPPVC_MB_STUFFING);
                    // decode ac_pred_flag
                    MacroBlock.ac_pred_flag = mp4_GetBit(pInfo);
                    nOpaq = (curShapeInfo->opaque & 1) + ((curShapeInfo->opaque & 2) >> 1) + ((curShapeInfo->opaque & 4) >> 2) + ((curShapeInfo->opaque & 8) >> 3);
                    // decode cbpy
                    if (mp4_DecodeCBPY_I_Shape(pInfo, nOpaq, &MacroBlock.cbpy) != MP4_STATUS_OK)
                        return MP4_STATUS_ERROR;
                    quantPred = quant;
                    // decode dquant
                    if (MacroBlock.type == IPPVC_MBTYPE_INTRA_Q)
                        mp4_UpdateQuant(pInfo, quant);
                    if (mbCurr == 0)
                        quantPred = quant;
                    dcVLC = (quantPred < mp4_DC_vlc_Threshold[pInfo->VisualObject.VideoObject.VideoObjectPlane.intra_dc_vlc_thr]) ? 1 : 0;
                    if (pInfo->VisualObject.VideoObject.interlaced) {
                        dct_type = mp4_GetBit(pInfo);
                        if (dct_type) {
                            stepY = stepYc * 2;
                            pYoff2 = stepYc;  pYoff3 = stepYc + 8;
                        } else {
                            stepY = stepYc;
                            pYoff2 = 8 * stepYc;  pYoff3 = 8 * stepYc + 8;
                        }
                    }
                    // decode blocks
                    cbpy = MacroBlock.cbpy;
                    nOpaq --;
                    if (pInfo->VisualObject.VideoObject.VideoObjectPlane.alternate_vertical_scan_flag) {
                        if (curShapeInfo->opaque & 1) {
                            if ((status = mp4_ReconBlockIntraVertScan_MPEG4(pInfo, MacroBlock.ac_pred_flag, dcVLC, pY, stepY, ppbrY, ppbcY, ppbq, quant, 0, pQuantMatIntra, cbpy & (1 << nOpaq))) != ippStsNoErr)
                                return MP4_STATUS_ERROR;
                            nOpaq --;
                        } else {
                            ppbcY[0] = ppbrY[0];
                            ppbrY[0] = -1;
                        }
                        if (curShapeInfo->opaque & 2) {
                            if ((status = mp4_ReconBlockIntraVertScan_MPEG4(pInfo, MacroBlock.ac_pred_flag, dcVLC, pY+8, stepY, ppbrY+8, ppbcY, ppbq, quant, 1, pQuantMatIntra, cbpy & (1 << nOpaq))) != ippStsNoErr)
                                return MP4_STATUS_ERROR;
                            nOpaq --;
                        } else {
                            {
                                Ipp16s tmp;
                                tmp = ppbcY[8];
                                ppbcY[8] = ppbrY[8-16];
                                ppbrY[8-16] = tmp;
                            }
                            ppbcY[0] = ppbrY[8];
                            ppbrY[8] = -1;
                        }
                        if (curShapeInfo->opaque & 4) {
                            if ((status = mp4_ReconBlockIntraVertScan_MPEG4(pInfo, MacroBlock.ac_pred_flag, dcVLC, pY+pYoff2, stepY, ppbrY, ppbcY+8, ppbq, quant, 2, pQuantMatIntra, cbpy & (1 << nOpaq))) != ippStsNoErr)
                                return MP4_STATUS_ERROR;
                            nOpaq --;
                        } else {
                            ppbcY[8] = ppbrY[0];
                            ppbrY[0] = -1;
                        }
                        if (curShapeInfo->opaque & 8) {
                            if ((status = mp4_ReconBlockIntraVertScan_MPEG4(pInfo, MacroBlock.ac_pred_flag, dcVLC, pY+pYoff3, stepY, ppbrY+8, ppbcY+8, ppbq, quant, 3, pQuantMatIntra, cbpy & (1 << nOpaq))) != ippStsNoErr)
                                return MP4_STATUS_ERROR;
                            nOpaq --;
                        } else {
                            ppbcY[8] = -1;
                        }
                        if ((status = mp4_ReconBlockIntraVertScan_MPEG4(pInfo, MacroBlock.ac_pred_flag, dcVLC, pCb, stepCbc, ppbrCb, ppbcCb, ppbq, quant, 4, pQuantMatIntra, MacroBlock.cbpc & 2)) != ippStsNoErr)
                            return MP4_STATUS_ERROR;
                        if ((status = mp4_ReconBlockIntraVertScan_MPEG4(pInfo, MacroBlock.ac_pred_flag, dcVLC, pCr, stepCrc, ppbrCr, ppbcCr, ppbq, quant, 5, pQuantMatIntra, MacroBlock.cbpc & 1)) != ippStsNoErr)
                        return MP4_STATUS_ERROR;
                    } else {
                        if (curShapeInfo->opaque & 1) {
                            mp4_DecodeBlockIntra_MPEG4(MacroBlock.ac_pred_flag, cbpy & (1 << nOpaq), pY, stepY, ppbrY, ppbcY, 0);
                            nOpaq --;
                        } else {
                            ppbcY[0] = ppbrY[0];
                            ppbrY[0] = -1;
                        }
                        if (curShapeInfo->opaque & 2) {
                            mp4_DecodeBlockIntra_MPEG4(MacroBlock.ac_pred_flag, cbpy & (1 << nOpaq), pY+8, stepY, ppbrY+8, ppbcY, 1);
                            nOpaq --;
                        } else {
                            {
                                Ipp16s tmp;
                                tmp = ppbcY[8];
                                ppbcY[8] = ppbrY[8-16];
                                ppbrY[8-16] = tmp;
                            }
                            ppbcY[0] = ppbrY[8];
                            ppbrY[8] = -1;
                        }
                        if (curShapeInfo->opaque & 4) {
                            mp4_DecodeBlockIntra_MPEG4(MacroBlock.ac_pred_flag, cbpy & (1 << nOpaq), pY+pYoff2, stepY, ppbrY, ppbcY+8, 2);
                            nOpaq --;
                        } else {
                            ppbcY[8] = ppbrY[0];
                            ppbrY[0] = -1;
                        }
                        if (curShapeInfo->opaque & 8) {
                            mp4_DecodeBlockIntra_MPEG4(MacroBlock.ac_pred_flag, cbpy & (1 << nOpaq), pY+pYoff3, stepY, ppbrY+8, ppbcY+8, 3);
                            nOpaq --;
                        } else {
                            ppbcY[8] = -1;
                        }
                        mp4_DecodeBlockIntra_MPEG4(MacroBlock.ac_pred_flag, MacroBlock.cbpc & 2, pCb, stepCbc, ppbrCb, ppbcCb, 4);
                        mp4_DecodeBlockIntra_MPEG4(MacroBlock.ac_pred_flag, MacroBlock.cbpc & 1, pCr, stepCrc, ppbrCr, ppbcCr, 5);
                    }
                    if (pInfo->VisualObject.VideoObject.shape == MP4_SHAPE_TYPE_GRAYSCALE) {
                        for (k = 0; k < nAux; k ++) {
                            // only Classic Zigzag and Frame DCT for Aux
                            stepY = stepYc;
                            pYoff2 = 8 * stepYc;  pYoff3 = 8 * stepYc + 8;
                            // dc_vlc_thr is not used for Aux
                            dcVLC = 1;
                            if (mp4_GetBit(pInfo)) {
                                mp4_Set16x16_8u(pA[k], stepY, 255);
                            } else {
                                int  cbpa;
                                MacroBlock.ac_pred_flag = mp4_GetBit(pInfo);
                                if (mp4_DecodeCBPY_I_Shape(pInfo, nOpaq, &cbpa) != MP4_STATUS_OK)
                                    return MP4_STATUS_ERROR;
                                cbpa = 15 - cbpa;
                                nOpaq = (curShapeInfo->opaque & 1) + ((curShapeInfo->opaque & 2) >> 1) + ((curShapeInfo->opaque & 4) >> 2) + ((curShapeInfo->opaque & 8) >> 3);
                                nOpaq --;
                                if (curShapeInfo->opaque & 1) {
                                    mp4_DecodeBlockIntra_MPEG4(MacroBlock.ac_pred_flag, cbpa & (1 << nOpaq), pA[k], stepY, ppbrA[k], ppbcA[k], 0);
                                    nOpaq --;
                                } else {
                                    ppbcA[k][0] = ppbrA[k][0];
                                    ppbrA[k][0] = -1;
                                }
                                if (curShapeInfo->opaque & 2) {
                                    mp4_DecodeBlockIntra_MPEG4(MacroBlock.ac_pred_flag, cbpa & (1 << nOpaq), pA[k]+8, stepY, ppbrA[k]+8, ppbcA[k], 1);
                                    nOpaq --;
                                } else {
                                    {
                                        Ipp16s tmp;
                                        tmp = ppbcA[k][8];
                                        ppbcA[k][8] = ppbrA[k][8-16];
                                        ppbrA[k][8-16] = tmp;
                                    }
                                    ppbcA[k][0] = ppbrA[k][8];
                                    ppbrA[k][8] = -1;
                                }
                                if (curShapeInfo->opaque & 4) {
                                    mp4_DecodeBlockIntra_MPEG4(MacroBlock.ac_pred_flag, cbpa & (1 << nOpaq), pA[k]+pYoff2, stepY, ppbrA[k], ppbcA[k]+8, 2);
                                    nOpaq --;
                                } else {
                                    ppbcA[k][8] = ppbrA[k][0];
                                    ppbrA[k][0] = -1;
                                }
                                if (curShapeInfo->opaque & 8) {
                                    mp4_DecodeBlockIntra_MPEG4(MacroBlock.ac_pred_flag, cbpa & (1 << nOpaq), pA[k]+pYoff3, stepY, ppbrA[k]+8, ppbcA[k]+8, 3);
                                    nOpaq --;
                                } else {
                                    ppbcA[k][8] = -1;
                                }
                            }
                        }
                    }
                }
                mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB);
                mbCurr ++;
                if (mbCurr == nmb) {
                    // skip stuffing
                    while (mp4_ShowBits9(pInfo, 9) == 1)
                        mp4_FlushBits(pInfo, 9);
                    return MP4_STATUS_OK;
                }
                // update predict buffers
                ppbq ++; ppbq[0] = (Ipp8u)quant;
                ppbrY += 16;
                mbInVideoPacket ++;
                colNum ++;
                if (colNum == mbPerRow) {
                    colNum = 0;
                    rowNum ++;
                    // needed for ippVC predict mechanism
                    ppbrY[-8] = ppbcY[8];   / * Update DC part of last MB per line * /
                    for (k = 0; k < nAux; k ++)
                        ppbrA[k][-8] = ppbcA[k][8];   / * Update DC part of last MB per line * /
                } else {
                    pY += 16; pCr += 8; pCb += 8;
                    ppbrCb += 8; ppbrCr += 8;
                    pB += 16;
                    for (k = 0; k < nAux; k ++)
                        pA[k] += 16;
                }
                curShapeInfo ++;
                if (!pInfo->VisualObject.VideoObject.resync_marker_disable)
                    if (mp4_DecodeVideoPacket(pInfo, &quant))
                        break;
            }
        }
    }
*/
    return MP4_STATUS_OK;
}


/*
//  decode IVOP
*/
mp4_Status mp4_DecodeVOP_I(mp4_Info* pInfo)
{
    int     quant, quantPred, dcVLC, mb_type, cbpc, cbpy, ac_pred_flag;
    int     i, j, nmb, stepYc, stepCbc, stepCrc, stepFc[6], mbCurr, mbInVideoPacket, colNum, rowNum, mbPerRow, mbPerCol;
    Ipp8u  *pFc[6];

    stepYc = pInfo->VisualObject.cFrame.stepY;
    stepCbc = pInfo->VisualObject.cFrame.stepCb;
    stepCrc = pInfo->VisualObject.cFrame.stepCr;
    mbPerRow = pInfo->VisualObject.VideoObject.MacroBlockPerRow;
    mbPerCol = pInfo->VisualObject.VideoObject.MacroBlockPerCol;
    stepFc[0] = stepFc[1] = stepFc[2] = stepFc[3] = stepYc; stepFc[4] = stepCbc; stepFc[5] = stepCrc;
    pFc[0] = pInfo->VisualObject.cFrame.pY; pFc[1] = pInfo->VisualObject.cFrame.pY + 8;
    pFc[2] = pInfo->VisualObject.cFrame.pY + 8 * stepYc; pFc[3] = pInfo->VisualObject.cFrame.pY + 8 * stepYc + 8;
    pFc[4] = pInfo->VisualObject.cFrame.pCb; pFc[5] = pInfo->VisualObject.cFrame.pCr;
// decode short_video_header I-VOP
    if (pInfo->VisualObject.VideoObject.short_video_header) {
        quant = pInfo->VisualObject.VideoObject.VideoObjectPlaneH263.vop_quant;
        nmb = 0;
        pInfo->VisualObject.VideoObject.VideoObjectPlaneH263.gob_number = 0;
        for (i = 0; i < mbPerCol; i ++) {
            for (j = 0; j < mbPerRow; j ++) {
                do {
                    if (mp4_DecodeMCBPC_I(pInfo, &mb_type, &cbpc) != MP4_STATUS_OK)
                        return MP4_STATUS_ERROR;
                } while (mb_type == IPPVC_MB_STUFFING);
                if (mp4_DecodeCBPY_I(pInfo, &cbpy) != MP4_STATUS_OK)
                    return MP4_STATUS_ERROR;
                if (mb_type == IPPVC_MBTYPE_INTRA_Q)
                    mp4_UpdateQuant(pInfo, quant);
                if (mp4_DecodeIntraMB_SVH(pInfo, (cbpy << 2) + cbpc, quant, pFc, stepFc) != MP4_STATUS_OK) {
                    mp4_Error("Error when decode coefficients of Intra block");
                    return MP4_STATUS_ERROR;
                }
                pFc[0] += 16; pFc[1] += 16; pFc[2] += 16; pFc[3] += 16; pFc[4] += 8; pFc[5] += 8;
                mp4_CheckDecodeGOB_SVH(pInfo, nmb, nmb, i, quant);
                mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB);
            }
            pFc[0] += 2 * MP4_NUM_EXT_MB * 16 + (stepYc << 4) - stepYc;
            pFc[1] += 2 * MP4_NUM_EXT_MB * 16 + (stepYc << 4) - stepYc;
            pFc[2] += 2 * MP4_NUM_EXT_MB * 16 + (stepYc << 4) - stepYc;
            pFc[3] += 2 * MP4_NUM_EXT_MB * 16 + (stepYc << 4) - stepYc;
            pFc[4] += 2 * MP4_NUM_EXT_MB * 8 + (stepCbc << 3) - stepCbc;
            pFc[5] += 2 * MP4_NUM_EXT_MB * 8 + (stepCrc << 3) - stepCrc;
        }
        mp4_AlignBits(pInfo);
        return MP4_STATUS_OK;
    }
    
#ifdef SUPPORT_H263_ONLY 
	return MP4_STATUS_FILE_ERROR;
#else
    
    quant = pInfo->VisualObject.VideoObject.VideoObjectPlane.quant;
    nmb = pInfo->VisualObject.VideoObject.MacroBlockPerVOP;
    mbCurr = 0;
    colNum = rowNum = 0;
    if (pInfo->VisualObject.VideoObject.sprite_enable != MP4_SPRITE_STATIC)
        ippsZero_8u_x((Ipp8u*)pInfo->VisualObject.VideoObject.MBinfo, nmb * sizeof(mp4_MacroBlock));
// decode data_partitioned I-VOP
    if (pInfo->VisualObject.VideoObject.data_partitioned) {
        for (;;) {
            mp4_DataPartMacroBlock *pMBdp;
            // reset Intra prediction buffer on new Video_packet
            mp4_ResetIntraPredBuffer(pInfo);
            mbInVideoPacket = 0;
            pMBdp = &pInfo->VisualObject.VideoObject.DataPartBuff[mbCurr];
            // decode mb_type/cbpc/dquant/DC part
            for (;;) {
                if (mp4_DecodeMCBPC_I(pInfo, &mb_type, &cbpc) != MP4_STATUS_OK)
                    return MP4_STATUS_ERROR;
                if (mb_type != IPPVC_MB_STUFFING) {
                    quantPred = quant;
                    if (mb_type == IPPVC_MBTYPE_INTRA_Q)
                        mp4_UpdateQuant(pInfo, quant);
                    if (mbInVideoPacket == 0)
                        quantPred = quant;
                    dcVLC = (quantPred < mp4_DC_vlc_Threshold[pInfo->VisualObject.VideoObject.VideoObjectPlane.intra_dc_vlc_thr]) ? 1 : 0;
                    if (dcVLC) {
                        for (i = 0; i < 6; i ++) {
                            if (ippiDecodeDCIntra_MPEG4_1u16s_x(&pInfo->bufptr, &pInfo->bitoff, &pMBdp->dct_dc[i], (i < 4) ? IPPVC_BLOCK_LUMA : IPPVC_BLOCK_CHROMA) != ippStsNoErr) {
                                mp4_Error("Error when decode coefficients of Intra block");
                                return MP4_STATUS_ERROR;
                            }
                        }
                    }
                    pMBdp->quant = (Ipp8u)quant;
                    pMBdp->type = (Ipp8u)mb_type;
                    pMBdp->pat = (Ipp8u)cbpc;
                    pMBdp ++;
                    mbInVideoPacket ++;
                }
                if (mp4_ShowBits(pInfo, 19) == MP4_DC_MARKER) {
                    mp4_GetBits(pInfo, 19);
                    break;
                }
            }
            pMBdp = &pInfo->VisualObject.VideoObject.DataPartBuff[mbCurr];
            // decode ac_pred_flag/cbpy part
            for (i = 0; i < mbInVideoPacket; i ++) {
                pMBdp[i].ac_pred_flag = (Ipp8u)mp4_GetBit(pInfo);
                if (mp4_DecodeCBPY_I(pInfo, &cbpy) != MP4_STATUS_OK)
                    return MP4_STATUS_ERROR;
                pMBdp[i].pat = (Ipp8u)((cbpy << 2) + pMBdp[i].pat);
            }
            // decode AC part and reconstruct macroblocks
            for (i = 0; i < mbInVideoPacket; i ++) {
                if (colNum == 0) {
                    // reset B-prediction blocks on new row
                    mp4_ResetIntraPredBblock(pInfo);
                }
                quant = pMBdp[i].quant;
                quantPred = (i == 0) ? quant : pMBdp[i-1].quant;
                dcVLC = (quantPred < mp4_DC_vlc_Threshold[pInfo->VisualObject.VideoObject.VideoObjectPlane.intra_dc_vlc_thr]) ? 1 : 0;
                ac_pred_flag = pMBdp[i].ac_pred_flag;
                if (mp4_DecodeIntraMB_DP(pInfo, pMBdp[i].dct_dc, colNum, pMBdp[i].pat, quant, dcVLC, ac_pred_flag, pFc, stepFc) != MP4_STATUS_OK) {
                    mp4_Error("Error when decode coefficients of Intra block");
                    return MP4_STATUS_ERROR;
                }
                mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB);
                colNum ++;
                if (colNum == mbPerRow) {
                    colNum = 0;
                    rowNum ++;
                    pFc[0] += 3 * MP4_NUM_EXT_MB * 16 + (stepYc << 4) - stepYc;
                    pFc[1] += 3 * MP4_NUM_EXT_MB * 16 + (stepYc << 4) - stepYc;
                    pFc[2] += 3 * MP4_NUM_EXT_MB * 16 + (stepYc << 4) - stepYc;
                    pFc[3] += 3 * MP4_NUM_EXT_MB * 16 + (stepYc << 4) - stepYc;
                    pFc[4] += 3 * MP4_NUM_EXT_MB * 8 + (stepCbc << 3) - stepCbc;
                    pFc[5] += 3 * MP4_NUM_EXT_MB * 8 + (stepCrc << 3) - stepCrc;
                } else {
                    pFc[0] += 16; pFc[1] += 16; pFc[2] += 16; pFc[3] += 16; pFc[4] += 8; pFc[5] += 8;
                }
            }
            mbCurr += mbInVideoPacket;
            if (mbCurr == nmb)
                break;
            if (!pInfo->VisualObject.VideoObject.resync_marker_disable) {
                int  found;
                if (mp4_DecodeVideoPacket(pInfo, &quant, &found) == MP4_STATUS_OK) {
                    if (!found)
                        break;
                } else
                    return MP4_STATUS_ERROR;
            }
        }
        return MP4_STATUS_OK;
    }
// decode not data partitioned I-VOP
    else {
        int     pYoff23 = 8 * stepYc;
#ifndef DROP_FIELD
        int     stepY = stepYc, dct_type = 0;
#endif
        for (;;) {
            // reset Intra prediction buffer on new Video_packet
            mp4_ResetIntraPredBuffer(pInfo);
            mbInVideoPacket = 0;
            // decode blocks
            for (;;) {
                if (colNum == 0) {
                    // reset B-prediction blocks on new row
                    mp4_ResetIntraPredBblock(pInfo);
                }
                if (mp4_DecodeMCBPC_I(pInfo, &mb_type, &cbpc) != MP4_STATUS_OK)
                    return MP4_STATUS_ERROR;
                if (mb_type != IPPVC_MB_STUFFING) {
                    ac_pred_flag = mp4_GetBit(pInfo);
                    if (mp4_DecodeCBPY_I(pInfo, &cbpy) != MP4_STATUS_OK)
                        return MP4_STATUS_ERROR;
                    quantPred = quant;
                    if (mb_type == IPPVC_MBTYPE_INTRA_Q)
                        mp4_UpdateQuant(pInfo, quant);
                    if (mbCurr == 0)
                        quantPred = quant;
                    dcVLC = (quantPred < mp4_DC_vlc_Threshold[pInfo->VisualObject.VideoObject.VideoObjectPlane.intra_dc_vlc_thr]) ? 1 : 0;
#ifndef DROP_FIELD
					if (pInfo->VisualObject.VideoObject.interlaced) {
                        dct_type = mp4_GetBit(pInfo);
                        if (dct_type) {
                            stepY = stepYc * 2;
                            pYoff23 = stepYc;
                        } else {
                            stepY = stepYc;
                            pYoff23 = 8 * stepYc;
                        }
                        stepFc[0] = stepFc[1] = stepFc[2] = stepFc[3] = stepY;
					}
#endif
                    pFc[2] = pFc[0] + pYoff23; pFc[3] = pFc[1] + pYoff23;
                    if (mp4_DecodeIntraMB(pInfo, colNum, (cbpy << 2) + cbpc, quant, dcVLC, ac_pred_flag, pFc, stepFc) != MP4_STATUS_OK) {
                        mp4_Error("Error when decode coefficients of Intra block");
                        return MP4_STATUS_ERROR;
                    }
                    mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB);
                    mbCurr ++;
                    if (mbCurr == nmb) {
                        // skip stuffing
                        while (mp4_ShowBits9(pInfo, 9) == 1)
                            mp4_FlushBits(pInfo, 9);
                        return MP4_STATUS_OK;
                    }
                    mbInVideoPacket ++;
                    colNum ++;
                    if (colNum == mbPerRow) {
                        colNum = 0;
                        rowNum ++;
                        pFc[0] += 3 * MP4_NUM_EXT_MB * 16 + (stepYc << 4) - stepYc;
                        pFc[1] += 3 * MP4_NUM_EXT_MB * 16 + (stepYc << 4) - stepYc;
                        pFc[4] += 3 * MP4_NUM_EXT_MB * 8 + (stepCbc << 3) - stepCbc;
                        pFc[5] += 3 * MP4_NUM_EXT_MB * 8 + (stepCrc << 3) - stepCrc;
                    } else {
                        pFc[0] += 16; pFc[1] += 16; pFc[4] += 8; pFc[5] += 8;
                    }
                }
                if (!pInfo->VisualObject.VideoObject.resync_marker_disable) {
                    int  found;
                    if (mp4_DecodeVideoPacket(pInfo, &quant, &found) == MP4_STATUS_OK) {
                        if (found)
                            break;
                    } else
                        return MP4_STATUS_ERROR;
                }
            }
        }
    }
    
#endif 
   
}


#ifdef _OMP_KARABAS

static mp4_Status mp4_DecodeVOP_I_DecodeSlice(mp4_Info* pInfo, int curRow, mp4_MacroBlockMT* pMBinfoMT)
{
    int  mb_type, cbpc, cbpy, quant, quantPred, ac_pred_flag, dcVLC;
    int  j, mbPerRow, pat;

    mbPerRow = pInfo->VisualObject.VideoObject.MacroBlockPerRow;
    if (pInfo->VisualObject.VideoObject.short_video_header) {
        quant = pInfo->VisualObject.VideoObject.VideoObjectPlaneH263.vop_quant;
        for (j = 0; j < mbPerRow; j ++) {
            do {
                if (mp4_DecodeMCBPC_I(pInfo, &mb_type, &cbpc) != MP4_STATUS_OK)
                    return MP4_STATUS_ERROR;
            } while (mb_type == IPPVC_MB_STUFFING);
            if (mp4_DecodeCBPY_I(pInfo, &cbpy) != MP4_STATUS_OK)
                return MP4_STATUS_ERROR;
            if (mb_type == IPPVC_MBTYPE_INTRA_Q)
                mp4_UpdateQuant(pInfo, quant);
            pat = (cbpy << 2) + cbpc;
            mp4_ReconstructCoeffsIntraMB_SVH(pInfo, pMBinfoMT->dctCoeffs, pMBinfoMT->lnz, pat, quant);
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB);
            pMBinfoMT ++;
            mp4_CheckDecodeGOB_SVH(pInfo, pInfo->VisualObject.VideoObject.VideoObjectPlaneH263.nmb, pInfo->VisualObject.VideoObject.VideoObjectPlaneH263.frGOB, curRow, quant);
        }
        pInfo->VisualObject.VideoObject.VideoObjectPlaneH263.vop_quant = quant;
    } 
#ifdef SUPPORT_H263_ONLY 
	else return MP4_STATUS_FILE_ERROR;
#else
    else {
        // reset B-prediction blocks on new row
        mp4_ResetIntraPredBblock(pInfo);
        quant = quantPred = pInfo->VisualObject.VideoObject.VideoObjectPlane.quant;
        for (j = 0; j < mbPerRow;) {
            if (mp4_DecodeMCBPC_I(pInfo, &mb_type, &cbpc) != MP4_STATUS_OK)
                return MP4_STATUS_ERROR;
            if (mb_type != IPPVC_MB_STUFFING) {
                ac_pred_flag = mp4_GetBit(pInfo);
                if (mp4_DecodeCBPY_I(pInfo, &cbpy) != MP4_STATUS_OK)
                    return MP4_STATUS_ERROR;
                quantPred = quant;
                if (mb_type == IPPVC_MBTYPE_INTRA_Q)
                    mp4_UpdateQuant(pInfo, quant);
                if (curRow == 0 && j == 0)
                    quantPred = quant;
                dcVLC = (quantPred < mp4_DC_vlc_Threshold[pInfo->VisualObject.VideoObject.VideoObjectPlane.intra_dc_vlc_thr]) ? 1 : 0;
                if (pInfo->VisualObject.VideoObject.interlaced)
                    pMBinfoMT->dct_type = (Ipp8u)mp4_GetBit(pInfo);
                pat = (cbpy << 2) + cbpc;
                if (mp4_ReconstructCoeffsIntraMB(pInfo, j, pat, quant, dcVLC, ac_pred_flag, pMBinfoMT->dctCoeffs, pMBinfoMT->lnz) != MP4_STATUS_OK) {
                    mp4_Error("Error when decode coefficients of Intra block");
                    return MP4_STATUS_ERROR;
                }
                mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB);
                pMBinfoMT ++;
                j ++;
            }
            if (!pInfo->VisualObject.VideoObject.resync_marker_disable) {
                int  found;
                if (mp4_DecodeVideoPacket(pInfo, &quant, &found) == MP4_STATUS_OK) {
                    if (found) {
                        // reset Intra prediction buffer on new Video_packet
                        mp4_ResetIntraPredBuffer(pInfo);
                    }
                } else
                    return MP4_STATUS_ERROR;
            }
        }
        pInfo->VisualObject.VideoObject.VideoObjectPlane.quant = quant;

#endif
    } 
    
    return MP4_STATUS_OK;
}


static void mp4_DecodeVOP_I_ReconSlice(mp4_Info* pInfo, int curRow, mp4_MacroBlockMT* pMBinfoMT)
{
    int    j, stepYc, stepCbc, stepCrc, mbPerRow, stepFc[6];
    Ipp8u  *pYc, *pCbc, *pCrc, *pFc[6];

    mbPerRow = pInfo->VisualObject.VideoObject.MacroBlockPerRow;
    stepYc = pInfo->VisualObject.cFrame.stepY;
    stepCbc = pInfo->VisualObject.cFrame.stepCb;
    stepCrc = pInfo->VisualObject.cFrame.stepCr;
    pYc = pInfo->VisualObject.cFrame.pY   + curRow * 16 * stepYc;
    pCbc = pInfo->VisualObject.cFrame.pCb + curRow * 8 * stepCbc;
    pCrc = pInfo->VisualObject.cFrame.pCr + curRow * 8 * stepCrc;
    stepFc[0] = stepFc[1] = stepFc[2] = stepFc[3] = stepYc; stepFc[4] = stepCbc; stepFc[5] = stepCrc;
    pFc[0] = pYc; pFc[1] = pYc + 8; pFc[2] = pYc + 8 * stepYc; pFc[3] = pYc + 8 * stepYc + 8; pFc[4] = pCbc; pFc[5] = pCrc;
    if (pInfo->VisualObject.VideoObject.short_video_header) {
        for (j = 0; j < mbPerRow; j ++) {
            mp4_DCTInvCoeffsIntraMB(pMBinfoMT->dctCoeffs, pMBinfoMT->lnz, pFc, stepFc);
            pMBinfoMT ++;
            pFc[0] += 16; pFc[1] += 16; pFc[2] += 16; pFc[3] += 16; pFc[4] += 8; pFc[5] += 8;
        }
    } 
#ifdef SUPPORT_H263_ONLY 
	else return MP4_STATUS_FILE_ERROR;
#else
    else {
        int   stepY, pYoff23, interlaced;

        stepY = stepYc;
        pYoff23 = 8 * stepYc;
        interlaced = pInfo->VisualObject.VideoObject.interlaced;
        for (j = 0; j < mbPerRow; j ++) {
            if (interlaced) {
                if (pMBinfoMT->dct_type) {
                    stepY = stepYc * 2;
                    pYoff23 = stepYc;
                } else {
                    stepY = stepYc;
                    pYoff23 = 8 * stepYc;
                }
                stepFc[0] = stepFc[1] = stepFc[2] = stepFc[3] = stepY;
            }
            pFc[2] = pFc[0] + pYoff23; pFc[3] = pFc[1] + pYoff23;
            mp4_DCTInvCoeffsIntraMB(pMBinfoMT->dctCoeffs, pMBinfoMT->lnz, pFc, stepFc);
            pMBinfoMT ++;
            pFc[0] += 16; pFc[1] += 16; pFc[4] += 8; pFc[5] += 8;
        }
#endif   
    }             
}

#ifndef DROP_MULTI_THREAD
mp4_Status mp4_DecodeVOP_I_MT(mp4_Info* pInfo)
{
    int        i, mbPerCol, mbPerRow;
    mp4_Status sts = MP4_STATUS_OK;

    mbPerCol = pInfo->VisualObject.VideoObject.MacroBlockPerCol;
    mbPerRow = pInfo->VisualObject.VideoObject.MacroBlockPerRow;
    if (pInfo->VisualObject.VideoObject.short_video_header) {
        pInfo->VisualObject.VideoObject.VideoObjectPlaneH263.gob_number = 0;
        pInfo->VisualObject.VideoObject.VideoObjectPlaneH263.nmb = 0;
        pInfo->VisualObject.VideoObject.VideoObjectPlaneH263.frGOB = 0;
    } 
#ifdef SUPPORT_H263_ONLY 
	else return MP4_STATUS_FILE_ERROR;
#else    
    else {
        mp4_ResetIntraPredBuffer(pInfo);
        if (pInfo->VisualObject.VideoObject.sprite_enable != MP4_SPRITE_STATIC)
            ippsZero_8u_x((Ipp8u*)pInfo->VisualObject.VideoObject.MBinfo, pInfo->VisualObject.VideoObject.MacroBlockPerVOP * sizeof(mp4_MacroBlock));
    }
#endif
    
    i = 0;
#ifdef _OPENMP
#pragma  omp parallel shared(pInfo, i, mbPerCol, mbPerRow, sts) //num_threads(2)
#endif
    {
        int               idThread = 0;   /* the thread id of the calling thread. */
        int               curRow;
        mp4_MacroBlockMT* pMBinfoMT;

#ifdef _OPENMP
        idThread = omp_get_thread_num();
#endif
        pMBinfoMT = pInfo->pMBinfoMT + mbPerRow * idThread;
        curRow = i;
        while (curRow < mbPerCol) {
#ifdef _OPENMP
#pragma omp critical(HI_FOXY)
#endif
            {
                curRow = i;
                i ++;
                if (curRow < mbPerCol)
                    if (mp4_DecodeVOP_I_DecodeSlice(pInfo, curRow, pMBinfoMT) != MP4_STATUS_OK) {
                        sts = MP4_STATUS_ERROR;
                        curRow = mbPerCol;
                    }
            }
            if (curRow < mbPerCol)
                if (sts == MP4_STATUS_OK)
                    mp4_DecodeVOP_I_ReconSlice(pInfo, curRow, pMBinfoMT);
            curRow ++;
        }
    }
    if (pInfo->VisualObject.VideoObject.short_video_header)
        mp4_AlignBits(pInfo);
#ifdef SUPPORT_H263_ONLY 
	else return MP4_STATUS_FILE_ERROR;
#else
    else {
        // skip stuffing
        while (mp4_ShowBits(pInfo, 9) == 1)
            mp4_FlushBits(pInfo, 9);
    }
#endif
    
    return sts;
}

#endif

#endif // _OMP_KARABAS
