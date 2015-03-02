/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2001-2006 Intel Corporation. All Rights Reserved.
//
//  Description:    Decodes S-VOPs
//
*/

//***
#include "kinoma_ipp_lib.h"

#include "mp4.h"
#include "mp4dec.h"

#ifndef DROP_SPRITE
/*
//  decode MPEG-4 SVOP
*/

mp4_Status mp4_DecodeVOP_S(mp4_Info* pInfo)
{
    __ALIGN16(Ipp16s, coeffMB, 64*6);
    int             quant, quantPred, i, j, dcVLC, nmb, dx, dy, pat;
    int             stepYr, stepYc, stepCbr, stepCbc, stepCrr, stepCrc, mbPerRow;
    int             mbCurr, mbInVideoPacket, colNum, rowNum, stepF[6];
    Ipp8u           *pYc, *pCbc, *pCrc, *pYr, *pCbr, *pCrr, *pF[6];
    int             mb_not_coded, mb_type, cbpc, cbpy, ac_pred_flag, cbpyPrev, mcsel;
    int             scan, obmc_disable, rt, quarter_sample, interlaced, fcode_forward;
    IppiRect        limitRectL, limitRectC;
    IppMotionVector mvCur[4], mvPrev[4], mvTmp[4], mvCbCr;
    mp4_MacroBlock *pMBinfo;
    IppiRect        spriteRect, vopRect;

    mbPerRow = pInfo->VisualObject.VideoObject.MacroBlockPerRow;
    stepYc = pInfo->VisualObject.cFrame.stepY;
    stepYr = pInfo->VisualObject.rFrame.stepY;
    stepCbc = pInfo->VisualObject.cFrame.stepCb;
    stepCbr = pInfo->VisualObject.rFrame.stepCb;
    stepCrc = pInfo->VisualObject.cFrame.stepCr;
    stepCrr = pInfo->VisualObject.rFrame.stepCr;
    pYc = pInfo->VisualObject.cFrame.pY;
    pCbc = pInfo->VisualObject.cFrame.pCb;
    pCrc = pInfo->VisualObject.cFrame.pCr;
    pYr = pInfo->VisualObject.rFrame.pY;
    pCbr = pInfo->VisualObject.rFrame.pCb;
    pCrr = pInfo->VisualObject.rFrame.pCr;
    stepF[0] = stepF[1] = stepF[2] = stepF[3] = stepYc; stepF[4] = stepCbc; stepF[5] = stepCrc;
    // Bounding rectangle for MV limitation
    limitRectL.x = - 16;
    limitRectL.y = - 16;
    limitRectL.width = pInfo->VisualObject.VideoObject.width + 32;
    limitRectL.height = pInfo->VisualObject.VideoObject.height + 32;
    limitRectC.x = -8;
    limitRectC.y = -8;
    limitRectC.width = (pInfo->VisualObject.VideoObject.width >> 1) + 16;
    limitRectC.height = (pInfo->VisualObject.VideoObject.height >> 1) + 16;
    pMBinfo = pInfo->VisualObject.VideoObject.MBinfo;
    rt = pInfo->VisualObject.VideoObject.VideoObjectPlane.rounding_type;
    quarter_sample = pInfo->VisualObject.VideoObject.quarter_sample;
    obmc_disable = pInfo->VisualObject.VideoObject.obmc_disable;
    interlaced = pInfo->VisualObject.VideoObject.interlaced;
    scan = pInfo->VisualObject.VideoObject.VideoObjectPlane.alternate_vertical_scan_flag ? IPPVC_SCAN_VERTICAL : IPPVC_SCAN_ZIGZAG;
    nmb = pInfo->VisualObject.VideoObject.MacroBlockPerVOP;
    quant = quantPred = pInfo->VisualObject.VideoObject.VideoObjectPlane.quant;
    mbCurr = 0;
    colNum = rowNum = 0;
    cbpc = 0;
    // init WarpSpec for Sprites or GMC
    vopRect.x = 0;
    vopRect.y = 0;
    vopRect.width = pInfo->VisualObject.VideoObject.width;
    vopRect.height = pInfo->VisualObject.VideoObject.height;

    if (pInfo->VisualObject.VideoObject.sprite_enable == MP4_SPRITE_STATIC) {
        spriteRect.x = pInfo->VisualObject.VideoObject.sprite_left_coordinate;
        spriteRect.y = pInfo->VisualObject.VideoObject.sprite_top_coordinate;
        spriteRect.width = pInfo->VisualObject.VideoObject.sprite_width;
        spriteRect.height = pInfo->VisualObject.VideoObject.sprite_height;
        fcode_forward = 1;
    } 
	else 
	{
        spriteRect = vopRect; // for shapes they may be different !!!!!!
        fcode_forward = pInfo->VisualObject.VideoObject.VideoObjectPlane.fcode_forward;
    }
    ippiWarpInit_MPEG4_x(pInfo->VisualObject.VideoObject.WarpSpec,
                       pInfo->VisualObject.VideoObject.VideoObjectPlane.warping_mv_code_du,
                       pInfo->VisualObject.VideoObject.VideoObjectPlane.warping_mv_code_dv,
                       pInfo->VisualObject.VideoObject.sprite_warping_points,
                       pInfo->VisualObject.VideoObject.sprite_enable,
                       pInfo->VisualObject.VideoObject.sprite_warping_accuracy,
                       rt, quarter_sample, fcode_forward,
                       &spriteRect, &vopRect);
// decode basic sprites
    if (pInfo->VisualObject.VideoObject.sprite_enable == MP4_SPRITE_STATIC) {
        //if (pInfo->VisualObject.VideoObject.shape != MP4_SHAPE_TYPE_RECTANGULAR) {
        //    if (mp4_InitVOPShape(pInfo) != MP4_STATUS_OK)
        //        return MP4_STATUS_ERROR;
        //}
        ippiWarpLuma_MPEG4_8u_C1R_x(pInfo->VisualObject.sFrame.pY, pInfo->VisualObject.sFrame.stepY,
                                  pYc, stepYc, &vopRect, pInfo->VisualObject.VideoObject.WarpSpec);
        if (pInfo->VisualObject.VideoObject.sprite_brightness_change)
            ippiChangeSpriteBrightness_MPEG4_8u_C1IR_x(pYc, stepYc, vopRect.width, vopRect.height, pInfo->VisualObject.VideoObject.VideoObjectPlane.brightness_change_factor);
        vopRect.width >>= 1;  vopRect.height >>= 1;
        ippiWarpChroma_MPEG4_8u_P2R_x(pInfo->VisualObject.sFrame.pCb, pInfo->VisualObject.sFrame.stepCb,
                                    pInfo->VisualObject.sFrame.pCr, pInfo->VisualObject.sFrame.stepCr,
                                    pCbc, stepCbc, pCrc, stepCrc, &vopRect, pInfo->VisualObject.VideoObject.WarpSpec);
        return MP4_STATUS_OK;
    }
// decode data_partitioned S(GMC)-VOP
    if (pInfo->VisualObject.VideoObject.data_partitioned) {
        for (;;) {
            mp4_DataPartMacroBlock *pMBdp;
            int  x = colNum, y = rowNum;
            // reset Intra prediction buffer on new Video_packet
            mp4_ResetIntraPredBuffer(pInfo);
            pMBdp = &pInfo->VisualObject.VideoObject.DataPartBuff[mbCurr];
            pMBinfo = &pInfo->VisualObject.VideoObject.MBinfo[mbCurr];
            mbInVideoPacket = 0;
            // decode not_coded/mcsel/mb_type/cbpc/MV part
            for (;;) {
                mb_not_coded = mp4_GetBit(pInfo);
                if (mb_not_coded) {
                    mb_type = IPPVC_MBTYPE_INTER;
                    mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB_NOTCODED);
                } else {
                    if (mp4_DecodeMCBPC_P(pInfo, &mb_type, &cbpc, 1) != MP4_STATUS_OK)
                        return MP4_STATUS_ERROR;
                }
                if (mb_type != IPPVC_MB_STUFFING) {
                    pMBinfo->validPred = 1;
                    if (mb_type < IPPVC_MBTYPE_INTER4V && !mb_not_coded)
                        mcsel = mp4_GetBit(pInfo);
                    else
                        mcsel = mb_not_coded;
                    if (!mcsel) {
                        if (mb_type <= IPPVC_MBTYPE_INTER4V) {
                            if (mb_type != IPPVC_MBTYPE_INTER4V) {
                                if (mp4_PredictDecode1MV(pInfo, pMBinfo, y, x) != MP4_STATUS_OK) {
                                    mp4_Error("Error when decode motion vector");
                                    return MP4_STATUS_ERROR;
                                }
                                pMBinfo->mv[1] = pMBinfo->mv[2] = pMBinfo->mv[3] = pMBinfo->mv[0];
                            } else {
                                if (mp4_PredictDecode4MV(pInfo, pMBinfo, y, x) != MP4_STATUS_OK) {
                                    mp4_Error("Error when decode motion vector");
                                    return MP4_STATUS_ERROR;
                                }
                            }
                        } else {
                            mp4_Zero4MV(pMBinfo->mv);
                        }
                    } 
#ifndef DROP_GMC
					else {
                        ippiCalcGlobalMV_MPEG4_x(x << 4, y << 4, pMBinfo->mv, pInfo->VisualObject.VideoObject.WarpSpec);
                        pMBinfo->mv[1] = pMBinfo->mv[2] = pMBinfo->mv[3] = pMBinfo->mv[0];
                    }
#endif
                    pMBinfo->not_coded = (Ipp8u)mb_not_coded;
                    pMBinfo->type = (Ipp8u)mb_type;
                    pMBinfo ++;
                    pMBdp->pat = (Ipp8u)cbpc;
                    pMBdp->mcsel = (Ipp8u)mcsel;
                    pMBdp ++;
                    mbInVideoPacket ++;
                    x ++;
                    if (x == mbPerRow) {
                        x = 0;
                        y ++;
                    }
                }
                if (mp4_ShowBits(pInfo, 17) == MP4_MV_MARKER) {
                    mp4_GetBits(pInfo, 17);
                    break;
                }
            }
            pMBdp = &pInfo->VisualObject.VideoObject.DataPartBuff[mbCurr];
            pMBinfo = &pInfo->VisualObject.VideoObject.MBinfo[mbCurr];
            // decode ac_pred_flag/cbpy/dquant/IntraDC part
            for (i = 0; i < mbInVideoPacket; i ++) {
                if (!pMBinfo->not_coded) {
                    mb_type = pMBinfo->type;
                    if (mb_type >= IPPVC_MBTYPE_INTRA)
                        pMBdp->ac_pred_flag = (Ipp8u)mp4_GetBit(pInfo);
                    if (mp4_DecodeCBPY_P(pInfo, &cbpy, mb_type) != MP4_STATUS_OK)
                        return MP4_STATUS_ERROR;
                    pMBdp->pat = (Ipp8u)((cbpy << 2) + pMBdp->pat);
                    quantPred = quant;
                    if (mb_type == IPPVC_MBTYPE_INTER_Q || mb_type == IPPVC_MBTYPE_INTRA_Q)
                        mp4_UpdateQuant(pInfo, quant);
                    pMBdp->quant = (Ipp8u)quant;
                    if (i == 0)
                        quantPred = quant;
                    // decode DC coefficient of Intra blocks
                    dcVLC = (quantPred < mp4_DC_vlc_Threshold[pInfo->VisualObject.VideoObject.VideoObjectPlane.intra_dc_vlc_thr]) ? 1 : 0;
                    if ((mb_type >= IPPVC_MBTYPE_INTRA) && dcVLC) {
                        for (j = 0; j < 6; j ++) {
                            if (ippiDecodeDCIntra_MPEG4_1u16s_x(&pInfo->bufptr, &pInfo->bitoff, &pMBdp->dct_dc[j], j < 4 ? IPPVC_BLOCK_LUMA : IPPVC_BLOCK_CHROMA) != ippStsNoErr) {
                                mp4_Error("Error when decode coefficients of Intra block");
                                return MP4_STATUS_ERROR;
                            }
                        }
                    }
                } else
                    pMBdp->pat = 0;
                pMBinfo->not_coded = 0;  // for B-VOP all MB have MVs
                if (pMBdp->mcsel)
                    pMBinfo->type = IPPVC_MBTYPE_INTRA;  // for OBMC MVs
                pMBdp ++;
                pMBinfo ++;
            }
            if (mbCurr + mbInVideoPacket < nmb)
                pMBinfo->type = IPPVC_MBTYPE_INTRA;  // for OBMC set first MB of the next videopacket as invalid for right MV
            pMBdp = &pInfo->VisualObject.VideoObject.DataPartBuff[mbCurr];
            pMBinfo = &pInfo->VisualObject.VideoObject.MBinfo[mbCurr];
            // decode coeffs and reconstruct macroblocks
            for (i = 0; i < mbInVideoPacket; i ++) {
                if (colNum == 0) {
                    // reset B-prediction blocks on new row
                    mp4_ResetIntraPredBblock(pInfo);
                }
                quant = pMBdp->quant;
                mb_type = pMBinfo->type;
                mcsel = pMBdp->mcsel;
                if (!mcsel && (mb_type >= IPPVC_MBTYPE_INTRA)) {
                    quantPred = (i == 0) ? quant : pMBdp[-1].quant;
                    dcVLC = (quantPred < mp4_DC_vlc_Threshold[pInfo->VisualObject.VideoObject.VideoObjectPlane.intra_dc_vlc_thr]) ? 1 : 0;
                    pF[0] = pYc; pF[1] = pYc + 8; pF[2] = pYc + 8 * stepYc; pF[3] = pYc + 8 * stepYc + 8; pF[4] = pCbc; pF[5] = pCrc;
                    if (mp4_DecodeIntraMB_DP(pInfo, pMBdp->dct_dc, colNum, pMBdp->pat, quant, dcVLC, pMBdp->ac_pred_flag, pF, stepF) != MP4_STATUS_OK) {
                        mp4_Error("Error when decode coefficients of Intra block");
                        return MP4_STATUS_ERROR;
                    }
                } else {
                    mp4_UpdateIntraPredBuffInvalid(pInfo, colNum);
                    dx = colNum * 16;
                    dy = rowNum * 16;
                    pat = pMBdp->pat;
                    cbpy = pat >> 2;
                    cbpc = pat & 3;
                    if (pat)
                        if (mp4_DecodeInterMB(pInfo, coeffMB, quant, pat, scan) != MP4_STATUS_OK) {
                            mp4_Error("Error when decode coefficients of Inter block");
                            return MP4_STATUS_ERROR;
                        }
                    if (!mcsel) {
                        if (mb_type == IPPVC_MBTYPE_INTER4V) {
                            if (quarter_sample) {
                                mp4_Limit4MVQ(pMBinfo->mv, mvCur, &limitRectL, dx, dy, 8);
                                mp4_ComputeChroma4MVQ(pMBinfo->mv, &mvCbCr);
                            } else {
                                mp4_Limit4MV(pMBinfo->mv, mvCur, &limitRectL, dx, dy, 8);
                                mp4_ComputeChroma4MV(pMBinfo->mv, &mvCbCr);
                            }
                            mp4_LimitMV(&mvCbCr, &mvCbCr, &limitRectC, dx >> 1, dy >> 1, 8);
                        } else {
                            if (quarter_sample) {
                                mp4_LimitMVQ(pMBinfo->mv, mvCur, &limitRectL, dx, dy, 16);
                                mp4_ComputeChromaMVQ(mvCur, &mvCbCr);
                            } else {
                                mp4_LimitMV(pMBinfo->mv, mvCur, &limitRectL, dx, dy, 16);
                                mp4_ComputeChromaMV(mvCur, &mvCbCr);
                            }
                            mvCur[1] = mvCur[2] = mvCur[3] = mvCur[0];
                        }
                        if (obmc_disable) {
                            if (quarter_sample) {
                                if (mb_type == IPPVC_MBTYPE_INTER4V) {
                                    mp4_Copy8x8QP_8u(pYr, stepYr, pYc, stepYc, &mvCur[0], rt);
                                    mp4_Copy8x8QP_8u(pYr+8, stepYr, pYc+8, stepYc, &mvCur[1], rt);
                                    mp4_Copy8x8QP_8u(pYr+8*stepYr, stepYr, pYc+8*stepYc, stepYc, &mvCur[2], rt);
                                    mp4_Copy8x8QP_8u(pYr+8*stepYr+8, stepYr, pYc+8*stepYc+8, stepYc, &mvCur[3], rt);
                                } else
                                    mp4_Copy16x16QP_8u(pYr, stepYr, pYc, stepYc, &mvCur[0], rt);
                            } else {
                                if (mb_type == IPPVC_MBTYPE_INTER4V) {
                                    mp4_Copy8x8HP_8u(pYr, stepYr, pYc, stepYc, &mvCur[0], rt);
                                    mp4_Copy8x8HP_8u(pYr+8, stepYr, pYc+8, stepYc, &mvCur[1], rt);
                                    mp4_Copy8x8HP_8u(pYr+8*stepYr, stepYr, pYc+8*stepYc, stepYc, &mvCur[2], rt);
                                    mp4_Copy8x8HP_8u(pYr+8*stepYr+8, stepYr, pYc+8*stepYc+8, stepYc, &mvCur[3], rt);
                                } else
                                    mp4_Copy16x16HP_8u(pYr, stepYr, pYc, stepYc, &mvCur[0], rt);
                            }
                            mp4_AddResidual(cbpy & 8, pYc, stepYc, coeffMB);
                            mp4_AddResidual(cbpy & 4, pYc+8, stepYc, coeffMB+64);
                            mp4_AddResidual(cbpy & 2, pYc+stepYc*8, stepYc, coeffMB+128);
                            mp4_AddResidual(cbpy & 1, pYc+stepYc*8+8, stepYc, coeffMB+192);
                        } else {
                            mp4_OBMC(pInfo, pMBinfo, mvCur, colNum, rowNum, limitRectL, pYc, stepYc, pYr, stepYr, cbpy, coeffMB, 0);
                        }
                        mp4_MC_HP(cbpc & 2, pCbr, stepCbr, pCbc, stepCbc, coeffMB+256, &mvCbCr, rt);
                        mp4_MC_HP(cbpc & 1, pCrr, stepCrr, pCrc, stepCrc, coeffMB+320, &mvCbCr, rt);
                    } else {
                        IppiRect  mbRect;

                        mbRect.x = dx;  mbRect.y = dy;  mbRect.width = mbRect.height = 16;
                        ippiWarpLuma_MPEG4_8u_C1R_x(pInfo->VisualObject.rFrame.pY, stepYr, pYc, stepYc, &mbRect, pInfo->VisualObject.VideoObject.WarpSpec);
                        mbRect.x >>= 1;  mbRect.y >>= 1;  mbRect.width = mbRect.height = 8;
                        ippiWarpChroma_MPEG4_8u_P2R_x(pInfo->VisualObject.rFrame.pCb, stepCbr, pInfo->VisualObject.rFrame.pCr, stepCrr, pCbc, stepCbc, pCrc, stepCrc, &mbRect, pInfo->VisualObject.VideoObject.WarpSpec);
                        if (pat) {
                            mp4_AddResidual(cbpy & 8, pYc, stepYc, coeffMB);
                            mp4_AddResidual(cbpy & 4, pYc+8, stepYc, coeffMB+64);
                            mp4_AddResidual(cbpy & 2, pYc+stepYc*8, stepYc, coeffMB+128);
                            mp4_AddResidual(cbpy & 1, pYc+stepYc*8+8, stepYc, coeffMB+192);
                            mp4_AddResidual(cbpc & 2, pCbc, stepCbc, coeffMB+256);
                            mp4_AddResidual(cbpc & 1, pCrc, stepCrc, coeffMB+320);
                        }
                    }
                }
                mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB);
                pMBinfo ++;
                pMBdp ++;
                colNum ++;
                if (colNum == mbPerRow) {
                    colNum = 0;
                    rowNum ++;
                    pYc += 3 * MP4_NUM_EXT_MB * 16 + (stepYc << 4) - stepYc;
                    pCbc += 3 * MP4_NUM_EXT_MB * 8 + (stepCbc << 3) - stepCbc;
                    pCrc += 3 * MP4_NUM_EXT_MB * 8 + (stepCrc << 3) - stepCrc;
                    pYr += 3 * MP4_NUM_EXT_MB * 16 + (stepYr << 4) - stepYr;
                    pCbr += 3 * MP4_NUM_EXT_MB * 8 + (stepCbr << 3) - stepCbr;
                    pCrr += 3 * MP4_NUM_EXT_MB * 8 + (stepCrr << 3) - stepCrr;
                } else {
                    pYc += 16; pCrc += 8; pCbc += 8;
                    pYr += 16; pCrr += 8; pCbr += 8;
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
            // mark MBs the previous videopacket as invalid for prediction
            for (i = 1; i <= IPP_MIN(mbInVideoPacket, mbPerRow + 1); i ++)
                pMBinfo[-i].validPred = 0;
        }
        return MP4_STATUS_OK;
    }
// decode not data_partitioned S-VOP
    {
        int     stepY, pYoff23, field_prediction, dct_type, dct_typePrev, mb_ftfr, mb_fbfr;
        IppMotionVector mvCbCrT, mvCbCrB, *mvField = pInfo->VisualObject.VideoObject.FieldMV;

        // warning "variable may be used without having been initialized"
        dx = dy = ac_pred_flag = cbpy = cbpyPrev = field_prediction = dct_type = dct_typePrev = mb_ftfr = mb_fbfr = 0;
        mvCbCr.dx = mvCbCr.dy = mvCbCrT.dx = mvCbCrT.dy = mvCbCrB.dx = mvCbCrB.dy = 0;
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
                mb_not_coded = mp4_GetBit(pInfo);
                mb_type = IPPVC_MBTYPE_INTER;
                if (!mb_not_coded) {
                    if (mp4_DecodeMCBPC_P(pInfo, &mb_type, &cbpc, 1) != MP4_STATUS_OK)
                        return MP4_STATUS_ERROR;
                }
                if (mb_type != IPPVC_MB_STUFFING) {
                    if (!mb_not_coded) {
                        mcsel = (mb_type < IPPVC_MBTYPE_INTER4V) ? mp4_GetBit(pInfo) : 0;
                        if (mb_type >= IPPVC_MBTYPE_INTRA)
                            ac_pred_flag = mp4_GetBit(pInfo);
                        if (mp4_DecodeCBPY_P(pInfo, &cbpy, mb_type) != MP4_STATUS_OK)
                            return MP4_STATUS_ERROR;
                        quantPred = quant;
                        if (mb_type == IPPVC_MBTYPE_INTER_Q || mb_type == IPPVC_MBTYPE_INTRA_Q)
                            mp4_UpdateQuant(pInfo, quant);
#ifndef DROP_FIELD
						if (interlaced) {
                            dct_type = 0;
                            field_prediction = 0;
                            if (mb_type >= IPPVC_MBTYPE_INTRA || (cbpy + cbpc) != 0)
                                dct_type = mp4_GetBit(pInfo);
                            if ((mb_type == IPPVC_MBTYPE_INTER || mb_type == IPPVC_MBTYPE_INTER_Q) && !mcsel) {
                                field_prediction = mp4_GetBit(pInfo);
                                if (field_prediction) {
                                    mb_ftfr = mp4_GetBit(pInfo);
                                    mb_fbfr = mp4_GetBit(pInfo);
                                }
                            }
                        }
#endif
                    } else {
                        mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB_NOTCODED);
                        mcsel = 1;
                        field_prediction = 0;
                    }
                    pMBinfo->validPred = 1;
                    pMBinfo->not_coded = 0;  // for B-VOP all MB have MVs
                    pMBinfo->type = (Ipp8u)(mcsel ? IPPVC_MBTYPE_INTRA : mb_type);  // for OBMC MVs
                    pMBinfo->field_info = (Ipp8u)(field_prediction + (mb_ftfr << 1) + (mb_fbfr << 2));
                    if (mb_type >= IPPVC_MBTYPE_INTRA) {
                        if (mbCurr == 0)
                            quantPred = quant;
                        dcVLC = (quantPred < mp4_DC_vlc_Threshold[pInfo->VisualObject.VideoObject.VideoObjectPlane.intra_dc_vlc_thr]) ? 1 : 0;
                        if (dct_type) {
                            stepY = stepYc * 2;
                            pYoff23 = stepYc;
                        } else {
                            stepY = stepYc;
                            pYoff23 = 8 * stepYc;
                        }
                        stepF[0] = stepF[1] = stepF[2] = stepF[3] = stepY;
                        pF[0] = pYc; pF[1] = pYc + 8; pF[2] = pYc + pYoff23; pF[3] = pYc + pYoff23 + 8; pF[4] = pCbc; pF[5] = pCrc;
                        if (mp4_DecodeIntraMB(pInfo, colNum, (cbpy << 2) + cbpc, quant, dcVLC, ac_pred_flag, pF, stepF) != MP4_STATUS_OK) {
                            mp4_Error("Error when decode coefficients of Intra block");
                            return MP4_STATUS_ERROR;
                        }
                        mp4_Zero4MV(pMBinfo->mv);
                    } else {
                        mp4_UpdateIntraPredBuffInvalid(pInfo, colNum);
                        dx = colNum * 16;
                        dy = rowNum * 16;
                        if (!mcsel) {
                            if (!field_prediction) {
                                if (mb_type != IPPVC_MBTYPE_INTER4V) {
                                    if (mp4_PredictDecode1MV(pInfo, pMBinfo, rowNum, colNum) != MP4_STATUS_OK) {
                                        mp4_Error("Error when decode motion vector");
                                        return MP4_STATUS_ERROR;
                                    }
                                    pMBinfo->mv[1] = pMBinfo->mv[2] = pMBinfo->mv[3] = pMBinfo->mv[0];
                                    if (quarter_sample) {
                                        mp4_LimitMVQ(pMBinfo->mv, mvCur, &limitRectL, dx, dy, 16);
                                        mp4_ComputeChromaMVQ(mvCur, &mvCbCr);
                                    } else {
                                        mp4_LimitMV(pMBinfo->mv, mvCur, &limitRectL, dx, dy, 16);
                                        mp4_ComputeChromaMV(mvCur, &mvCbCr);
                                    }
                                    mvCur[1] = mvCur[2] = mvCur[3] = mvCur[0];
                                } else {
                                    if (mp4_PredictDecode4MV(pInfo, pMBinfo, rowNum, colNum) != MP4_STATUS_OK) {
                                        mp4_Error("Error when decode motion vector");
                                        return MP4_STATUS_ERROR;
                                    }
                                    if (quarter_sample) {
                                        mp4_Limit4MVQ(pMBinfo->mv, mvCur, &limitRectL, dx, dy, 8);
                                        mp4_ComputeChroma4MVQ(pMBinfo->mv, &mvCbCr);
                                    } else {
                                        mp4_Limit4MV(pMBinfo->mv, mvCur, &limitRectL, dx, dy, 8);
                                        mp4_ComputeChroma4MV(pMBinfo->mv, &mvCbCr);
                                    }
                                    mp4_LimitMV(&mvCbCr, &mvCbCr, &limitRectC, dx >> 1, dy >> 1, 8);
                                }
                            } 
#ifndef DROP_FIELD							
							else {
                                IppMotionVector  mvFT, mvFB;
                                if (mp4_PredictDecodeFMV(pInfo, pMBinfo, rowNum, colNum, &mvFT, &mvFB) != MP4_STATUS_OK) {
                                    mp4_Error("Error when decode motion vector");
                                    return MP4_STATUS_ERROR;
                                }
                                mvField[0] = mvFT; mvField[1] = mvFB;
                                if (quarter_sample) {
                                    mp4_LimitFMVQ(&mvFT, &mvCur[0], &limitRectL, dx, dy, 16);
                                    mp4_LimitFMVQ(&mvFB, &mvCur[2], &limitRectL, dx, dy, 16);
                                    mvTmp[0].dx = (Ipp16s)mp4_Div2(mvCur[0].dx);
                                    mvTmp[0].dy = (Ipp16s)(mp4_Div2(mvCur[0].dy << 1) >> 1);
                                    mvTmp[2].dx = (Ipp16s)mp4_Div2(mvCur[2].dx);
                                    mvTmp[2].dy = (Ipp16s)(mp4_Div2(mvCur[2].dy << 1) >> 1);
                                    mp4_ComputeChromaMV(&mvTmp[0], &mvCbCrT);
                                    mp4_ComputeChromaMV(&mvTmp[2], &mvCbCrB);
                                } else {
                                    mp4_LimitFMV(&mvFT, &mvCur[0], &limitRectL, dx, dy, 16);
                                    mp4_LimitFMV(&mvFB, &mvCur[2], &limitRectL, dx, dy, 16);
                                    mp4_ComputeChromaMV(&mvCur[0], &mvCbCrT);
                                    mp4_ComputeChromaMV(&mvCur[2], &mvCbCrB);
                                }
                            }
#endif
						} 
#ifndef DROP_GMC						
						else {
                            ippiCalcGlobalMV_MPEG4_x(dx, dy, pMBinfo->mv, pInfo->VisualObject.VideoObject.WarpSpec);
                            pMBinfo->mv[1] = pMBinfo->mv[2] = pMBinfo->mv[3] = pMBinfo->mv[0];
                        }
#endif
                    }
                    if (!obmc_disable) {
                        // OBMC for previous MB
                        if (colNum > 0)
                            if (pMBinfo[-1].type < IPPVC_MBTYPE_INTRA && !(pMBinfo[-1].field_info & 1)) // previous is marked as INTRA if it was mcsel
                                mp4_OBMC(pInfo, pMBinfo - 1, mvPrev, colNum - 1, rowNum, limitRectL, pYc - 16, stepYc, pYr - 16, stepYr, cbpyPrev, coeffMB, dct_typePrev);
                        if (mb_type < IPPVC_MBTYPE_INTRA && !field_prediction && !mcsel) {
                            cbpyPrev = cbpy;
                            dct_typePrev = dct_type;
                            mvPrev[0] = mvCur[0]; mvPrev[1] = mvCur[1]; mvPrev[2] = mvCur[2]; mvPrev[3] = mvCur[3];
                            if (mp4_DecodeInterMB(pInfo, coeffMB, quant, (cbpy << 2) + cbpc, scan) != MP4_STATUS_OK) {
                                mp4_Error("Error when decode coefficients of Inter block");
                                return MP4_STATUS_ERROR;
                            }
                            mp4_MC_HP(cbpc & 2, pCbr, stepCbr, pCbc, stepCbc, coeffMB+256, &mvCbCr, rt);
                            mp4_MC_HP(cbpc & 1, pCrr, stepCrr, pCrc, stepCrc, coeffMB+320, &mvCbCr, rt);
                            // OBMC current MB if it is the last in the row
                            if (colNum == mbPerRow - 1)
                                mp4_OBMC(pInfo, pMBinfo, mvPrev, colNum, rowNum, limitRectL, pYc, stepYc, pYr, stepYr, cbpyPrev, coeffMB, dct_typePrev);
                        }
                    }
                    if (mb_type < IPPVC_MBTYPE_INTRA && (obmc_disable || field_prediction || mcsel)) {
                        pat = mb_not_coded ? 0 : ((cbpy << 2) + cbpc);
                        if (pat)
                            if (mp4_DecodeInterMB(pInfo, coeffMB, quant, pat, scan) != MP4_STATUS_OK) {
                                mp4_Error("Error when decode coefficients of Inter block");
                                return MP4_STATUS_ERROR;
                            }
                        if (!mcsel) {
                            if (quarter_sample) {
                                if (!field_prediction) {
                                    if (mb_type == IPPVC_MBTYPE_INTER4V) {
                                        mp4_Copy8x8QP_8u(pYr, stepYr, pYc, stepYc, &mvCur[0], rt);
                                        mp4_Copy8x8QP_8u(pYr+8, stepYr, pYc+8, stepYc, &mvCur[1], rt);
                                        mp4_Copy8x8QP_8u(pYr+stepYr*8, stepYr, pYc+stepYc*8, stepYc, &mvCur[2], rt);
                                        mp4_Copy8x8QP_8u(pYr+8+stepYr*8, stepYr, pYc+8+stepYc*8, stepYc, &mvCur[3], rt);
                                    } else {
                                        mp4_Copy16x16QP_8u(pYr, stepYr, pYc, stepYc, &mvCur[0], rt);
                                    }
                                } else {
                                    mp4_Copy16x8QP_8u(pYr+stepYr*mb_ftfr, stepYr*2, pYc, stepYc*2, &mvCur[0], rt);
                                    mp4_Copy16x8QP_8u(pYr+stepYr*mb_fbfr, stepYr*2, pYc+stepYc, stepYc*2, &mvCur[2], rt);
                                }
                            } else {
                                if (!field_prediction) {
                                    if (mb_type == IPPVC_MBTYPE_INTER4V) {
                                        mp4_Copy8x8HP_8u(pYr, stepYr, pYc, stepYc, &mvCur[0], rt);
                                        mp4_Copy8x8HP_8u(pYr+8, stepYr, pYc+8, stepYc, &mvCur[1], rt);
                                        mp4_Copy8x8HP_8u(pYr+stepYr*8, stepYr, pYc+stepYc*8, stepYc, &mvCur[2], rt);
                                        mp4_Copy8x8HP_8u(pYr+8+stepYr*8, stepYr, pYc+8+stepYc*8, stepYc, &mvCur[3], rt);
                                    } else {
                                        mp4_Copy16x16HP_8u(pYr, stepYr, pYc, stepYc, &mvCur[0], rt);
                                    }
                                } else {
                                    mp4_Copy16x8HP_8u(pYr+stepYr*mb_ftfr, stepYr*2, pYc, stepYc*2, &mvCur[0], rt);
                                    mp4_Copy16x8HP_8u(pYr+stepYr*mb_fbfr, stepYr*2, pYc+stepYc, stepYc*2, &mvCur[2], rt);
                                }
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
                            if (!field_prediction) {
                                mp4_MC_HP(cbpc & 2, pCbr, stepCbr, pCbc, stepCbc, coeffMB+256, &mvCbCr, rt);
                                mp4_MC_HP(cbpc & 1, pCrr, stepCrr, pCrc, stepCrc, coeffMB+320, &mvCbCr, rt);
                            } else {
                                mp4_Copy8x4HP_8u(pCbr+(mb_ftfr ? stepCbr : 0), stepCbr*2, pCbc, stepCbc*2, &mvCbCrT, rt);
                                mp4_Copy8x4HP_8u(pCrr+(mb_ftfr ? stepCrr : 0), stepCrr*2, pCrc, stepCrc*2, &mvCbCrT, rt);
                                mp4_Copy8x4HP_8u(pCbr+(mb_fbfr ? stepCbr : 0), stepCbr*2, pCbc+stepCbc, stepCbc*2, &mvCbCrB, rt);
                                mp4_Copy8x4HP_8u(pCrr+(mb_fbfr ? stepCrr : 0), stepCrr*2, pCrc+stepCrc, stepCrc*2, &mvCbCrB, rt);
                                mp4_AddResidual(cbpc & 2, pCbc, stepCbc, coeffMB+256);
                                mp4_AddResidual(cbpc & 1, pCrc, stepCrc, coeffMB+320);
                            }
                        } else {
                            IppiRect  mbRect;

                            mbRect.x = dx;  mbRect.y = dy;  mbRect.width = mbRect.height = 16;
                            ippiWarpLuma_MPEG4_8u_C1R_x(pInfo->VisualObject.rFrame.pY, stepYr, pYc, stepYc, &mbRect, pInfo->VisualObject.VideoObject.WarpSpec);
                            mbRect.x >>= 1;  mbRect.y >>= 1;  mbRect.width = mbRect.height = 8;
                            ippiWarpChroma_MPEG4_8u_P2R_x(pInfo->VisualObject.rFrame.pCb, stepCbr, pInfo->VisualObject.rFrame.pCr, stepCrr, pCbc, stepCbc, pCrc, stepCrc, &mbRect, pInfo->VisualObject.VideoObject.WarpSpec);
                            if (pat) {
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
                                mp4_AddResidual(cbpc & 2, pCbc, stepCbc, coeffMB+256);
                                mp4_AddResidual(cbpc & 1, pCrc, stepCrc, coeffMB+320);
                            }
                        }
                    }
                    mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB);
                    mbCurr ++;
                    if (mbCurr == nmb) {
                        // skip stuffing
                        while (mp4_ShowBits(pInfo, 10) == 1)
                            mp4_FlushBits(pInfo, 10);
                        return MP4_STATUS_OK;
                    }
                    pMBinfo ++;
                    mvField += 2;
                    mbInVideoPacket ++;
                    colNum ++;
                    if (colNum == mbPerRow) {
                        colNum = 0;
                        rowNum ++;
                        pYc += 3 * MP4_NUM_EXT_MB * 16 + (stepYc << 4) - stepYc;
                        pCbc += 3 * MP4_NUM_EXT_MB * 8 + (stepCbc << 3) - stepCbc;
                        pCrc += 3 * MP4_NUM_EXT_MB * 8 + (stepCrc << 3) - stepCrc;
                        pYr += 3 * MP4_NUM_EXT_MB * 16 + (stepYr << 4) - stepYr;
                        pCbr += 3 * MP4_NUM_EXT_MB * 8 + (stepCbr << 3) - stepCbr;
                        pCrr += 3 * MP4_NUM_EXT_MB * 8 + (stepCrr << 3) - stepCrr;
                    } else {
                        pYc += 16; pCrc += 8; pCbc += 8;
                        pYr += 16; pCrr += 8; pCbr += 8;
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
            // mark MBs the previous videopacket as invalid for prediction
            for (i = 1; i <= IPP_MIN(mbInVideoPacket, mbPerRow + 1); i ++) {
                pMBinfo[-i].validPred = 0;
            }
        }
    }
}


#ifdef _OMP_KARABAS

static mp4_Status mp4_DecodeVOP_S_DecodeSlice(mp4_Info* pInfo, int curRow, mp4_MacroBlockMT* pMBinfoMT)
{
    int              mb_not_coded, mb_type, cbpc, cbpy, quant, quantPred, ac_pred_flag, dcVLC;
    int              i, j, mbPerRow, scan, pat;
    mp4_MacroBlock  *pMBinfo;
    int              mcsel, interlaced, field_prediction, dct_type, mb_ftfr, mb_fbfr;
    IppMotionVector *pMVField;

    mbPerRow = pInfo->VisualObject.VideoObject.MacroBlockPerRow;
    pMBinfo = pInfo->VisualObject.VideoObject.MBinfo + curRow * mbPerRow;
    // warning "variable may be used without having been initialized"
    cbpc = mb_type = ac_pred_flag = 0;
    quant = quantPred = pInfo->VisualObject.VideoObject.VideoObjectPlane.quant;
    scan = pInfo->VisualObject.VideoObject.VideoObjectPlane.alternate_vertical_scan_flag ? IPPVC_SCAN_VERTICAL : IPPVC_SCAN_ZIGZAG;
    interlaced = pInfo->VisualObject.VideoObject.interlaced;
    pMVField = interlaced ? pInfo->VisualObject.VideoObject.FieldMV + curRow * mbPerRow * 2 : 0;
    // init for non-interlaced
    field_prediction = dct_type = mb_ftfr = mb_fbfr = 0;
    // reset B-prediction blocks on new row
    mp4_ResetIntraPredBblock(pInfo);
    for (j = 0; j < mbPerRow;) {
        mb_not_coded = mp4_GetBit(pInfo);
        mb_type = IPPVC_MBTYPE_INTER;
        if (!mb_not_coded)
            if (mp4_DecodeMCBPC_P(pInfo, &mb_type, &cbpc, 1) != MP4_STATUS_OK)
                return MP4_STATUS_ERROR;
        if (mb_type != IPPVC_MB_STUFFING) {
            pMBinfo->validPred = 1;
            if (!mb_not_coded) {
                mcsel = (mb_type < IPPVC_MBTYPE_INTER4V) ? mp4_GetBit(pInfo) : 0;
                if (mb_type >= IPPVC_MBTYPE_INTRA)
                    ac_pred_flag = mp4_GetBit(pInfo);
                if (mp4_DecodeCBPY_P(pInfo, &cbpy, mb_type) != MP4_STATUS_OK)
                    return MP4_STATUS_ERROR;
                quantPred = quant;
                if (mb_type == IPPVC_MBTYPE_INTER_Q || mb_type == IPPVC_MBTYPE_INTRA_Q)
                    mp4_UpdateQuant(pInfo, quant);
                if (interlaced) {
                    dct_type = 0;
                    field_prediction = 0;
                    if (mb_type >= IPPVC_MBTYPE_INTRA || (cbpy + cbpc) != 0)
                        dct_type = mp4_GetBit(pInfo);
                    if ((mb_type == IPPVC_MBTYPE_INTER || mb_type == IPPVC_MBTYPE_INTER_Q) && !mcsel) {
                        field_prediction = mp4_GetBit(pInfo);
                        if (field_prediction) {
                            mb_ftfr = mp4_GetBit(pInfo);
                            mb_fbfr = mp4_GetBit(pInfo);
                        }
                    }
                }
                pat = (cbpy << 2) + cbpc;
                if (mb_type >= IPPVC_MBTYPE_INTRA) {
                    if (curRow == 0 && j == 0)
                        quantPred = quant;
                    dcVLC = (quantPred < mp4_DC_vlc_Threshold[pInfo->VisualObject.VideoObject.VideoObjectPlane.intra_dc_vlc_thr]) ? 1 : 0;
                    if (mp4_ReconstructCoeffsIntraMB(pInfo, j, pat, quant, dcVLC, ac_pred_flag, pMBinfoMT->dctCoeffs, pMBinfoMT->lnz) != MP4_STATUS_OK) {
                        mp4_Error("Error when decode coefficients of Intra block");
                        return MP4_STATUS_ERROR;
                    }
                    mp4_Zero4MV(pMBinfo->mv);
                } else {
                    mp4_UpdateIntraPredBuffInvalid(pInfo, j);
                    if (!mcsel) {
                        if (!field_prediction) {
                            if (mb_type != IPPVC_MBTYPE_INTER4V) {
                                if (mp4_PredictDecode1MV(pInfo, pMBinfo, curRow, j) != MP4_STATUS_OK) {
                                    mp4_Error("Error when decode motion vector");
                                    return MP4_STATUS_ERROR;
                                }
                                pMBinfo->mv[1] = pMBinfo->mv[2] = pMBinfo->mv[3] = pMBinfo->mv[0];
                            } else {
                                if (mp4_PredictDecode4MV(pInfo, pMBinfo, curRow, j) != MP4_STATUS_OK) {
                                    mp4_Error("Error when decode motion vector");
                                    return MP4_STATUS_ERROR;
                                }
                            }
                        } 					
						else {
                            if (mp4_PredictDecodeFMV(pInfo, pMBinfo, curRow, j, &pMVField[0], &pMVField[1]) != MP4_STATUS_OK) {
                                mp4_Error("Error when decode motion vector");
                                return MP4_STATUS_ERROR;
                            }
                        }
                    } else {
                        ippiCalcGlobalMV_MPEG4_x(j << 4, curRow << 4, pMBinfo->mv, pInfo->VisualObject.VideoObject.WarpSpec);
                        pMBinfo->mv[1] = pMBinfo->mv[2] = pMBinfo->mv[3] = pMBinfo->mv[0];
                    }
                    mp4_ReconstructCoeffsInterMB(pInfo, pMBinfoMT->dctCoeffs, pMBinfoMT->lnz, pat, 0, scan, quant);
                }
            } else {
                mp4_UpdateIntraPredBuffInvalid(pInfo, j);
                mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB_NOTCODED);
                //***mp4_Zero4MV(pMBinfo->mv);
                field_prediction = 0;
                mcsel = 1;
                pat = 0;
                ippiCalcGlobalMV_MPEG4_x(j << 4, curRow << 4, pMBinfo->mv, pInfo->VisualObject.VideoObject.WarpSpec);
                pMBinfo->mv[1] = pMBinfo->mv[2] = pMBinfo->mv[3] = pMBinfo->mv[0];
            }
            mp4_StatisticInc_(&pInfo->VisualObject.Statistic.nMB);
            pMBinfo->not_coded = 0;  // for B-VOP all MB have MVs
            pMBinfo->type = (Ipp8u)(mcsel ? IPPVC_MBTYPE_INTRA : mb_type);  // for OBMC MVs
            pMBinfoMT->mcsel = (Ipp8u)mcsel;
            pMBinfoMT->pat = (Ipp8u)pat;
            if (interlaced) {
                pMBinfoMT->dct_type = (Ipp8u)dct_type;
                pMBinfo->field_info = (Ipp8u)(field_prediction + (mb_ftfr << 1) + (mb_fbfr << 2));
                pMVField += 2;
            }
            pMBinfo ++;
            pMBinfoMT ++;
            j ++;
        }
        if (!pInfo->VisualObject.VideoObject.resync_marker_disable) {
            int  found;
            if (mp4_DecodeVideoPacket(pInfo, &quant, &found) == MP4_STATUS_OK) {
                if (found) {
                    // reset Intra prediction buffer on new Video_packet
                    mp4_ResetIntraPredBuffer(pInfo);
                    // mark MBs the previous videopacket as invalid for prediction
                    for (i = 1; i <= (curRow == 0 ? j : mbPerRow + 1); i ++) {
                        pMBinfo[-i].validPred = 0;
                    }
                }
            } else
                return MP4_STATUS_ERROR;
        }
    }
    pInfo->VisualObject.VideoObject.VideoObjectPlane.quant = quant;
    return MP4_STATUS_OK;
}


static void mp4_DecodeVOP_S_ReconSlice(mp4_Info* pInfo, int curRow, mp4_MacroBlockMT* pMBinfoMT)
{
    int             j, dx, dy, mbPerRow, pYoff23;
    int             stepYr, stepYc, stepCbr, stepCbc, stepCrr, stepCrc, stepFc[6], stepY;
    Ipp8u           *pYc, *pCbc, *pCrc, *pYr, *pCbr, *pCrr, *pFc[6];
    int             /*mb_not_coded,*/ mb_type, pat, scan, obmc_disable, rt, quarter_sample;
    int             interlaced, field_prediction, dct_type, mb_ftfr, mb_fbfr;
    IppiRect        limitRectL, limitRectC;
    mp4_MacroBlock *pMBinfo;
    IppMotionVector mvCur[4], mvCbCr, mvCbCrT, mvCbCrB, *pMVField, mvTmpT, mvTmpB;
    Ipp16s*         coeffMB;

    mbPerRow = pInfo->VisualObject.VideoObject.MacroBlockPerRow;
    stepYc = pInfo->VisualObject.cFrame.stepY;
    stepYr = pInfo->VisualObject.rFrame.stepY;
    stepCbc = pInfo->VisualObject.cFrame.stepCb;
    stepCbr = pInfo->VisualObject.rFrame.stepCb;
    stepCrc = pInfo->VisualObject.cFrame.stepCr;
    stepCrr = pInfo->VisualObject.rFrame.stepCr;
    pYc = pInfo->VisualObject.cFrame.pY   + curRow * 16 * stepYc;
    pCbc = pInfo->VisualObject.cFrame.pCb + curRow * 8 * stepCbc;
    pCrc = pInfo->VisualObject.cFrame.pCr + curRow * 8 * stepCrc;
    pYr = pInfo->VisualObject.rFrame.pY   + curRow * 16 * stepYr;
    pCbr = pInfo->VisualObject.rFrame.pCb + curRow * 8 * stepCbr;
    pCrr = pInfo->VisualObject.rFrame.pCr + curRow * 8 * stepCrr;
    dy = curRow * 16;
    dx = 0;
    stepFc[0] = stepFc[1] = stepFc[2] = stepFc[3] = stepYc; stepFc[4] = stepCbc; stepFc[5] = stepCrc;
    // Bounding rectangle for MV limitation
    limitRectL.x = - 16;
    limitRectL.y = - 16;
    limitRectL.width = pInfo->VisualObject.VideoObject.width + 32;
    limitRectL.height = pInfo->VisualObject.VideoObject.height + 32;
    pMBinfo = pInfo->VisualObject.VideoObject.MBinfo + curRow * mbPerRow;
    rt = pInfo->VisualObject.VideoObject.VideoObjectPlane.rounding_type;
    quarter_sample = pInfo->VisualObject.VideoObject.quarter_sample;
    obmc_disable = pInfo->VisualObject.VideoObject.obmc_disable;
    scan = pInfo->VisualObject.VideoObject.VideoObjectPlane.alternate_vertical_scan_flag ? IPPVC_SCAN_VERTICAL : IPPVC_SCAN_ZIGZAG;
    // Bounding rectangles for MV limitation
    limitRectC.x = -8;
    limitRectC.y = -8;
    limitRectC.width = (pInfo->VisualObject.VideoObject.width >> 1) + 16;
    limitRectC.height = (pInfo->VisualObject.VideoObject.height >> 1) + 16;
    interlaced = pInfo->VisualObject.VideoObject.interlaced;
    pMVField = interlaced ? pInfo->VisualObject.VideoObject.FieldMV + curRow * mbPerRow * 2 : 0;
    // init for non-interlaced
    stepY = stepYc;
    pYoff23 = 8 * stepYc;
    field_prediction = dct_type = mb_ftfr = mb_fbfr = 0;
    // warning "variable may be used without having been initialized"
    mvCbCr.dx = mvCbCr.dy = mvCbCrT.dx = mvCbCrT.dy = mvCbCrB.dx = mvCbCrB.dy = 0;
    for (j = 0; j < mbPerRow; j ++) {
       //*** mb_not_coded = pMBinfo->not_coded;
        mb_type = pMBinfo->type;
        coeffMB = pMBinfoMT->dctCoeffs;
        if (pMBinfoMT->mcsel) {
            IppiRect  mbRect;
            mbRect.x = dx;  mbRect.y = dy;  mbRect.width = mbRect.height = 16;
            ippiWarpLuma_MPEG4_8u_C1R_x(pInfo->VisualObject.rFrame.pY, stepYr, pYc, stepYc, &mbRect, pInfo->VisualObject.VideoObject.WarpSpec);
            mbRect.x >>= 1;  mbRect.y >>= 1;  mbRect.width = mbRect.height = 8;
            ippiWarpChroma_MPEG4_8u_P2R_x(pInfo->VisualObject.rFrame.pCb, stepCbr, pInfo->VisualObject.rFrame.pCr, stepCrr, pCbc, stepCbc, pCrc, stepCrc, &mbRect, pInfo->VisualObject.VideoObject.WarpSpec);
            pat = pMBinfoMT->pat;
            if (interlaced)
                dct_type = pMBinfoMT->dct_type;
            if (pat) {
                mp4_DCTInvCoeffsInterMB(pInfo, coeffMB, pMBinfoMT->lnz, pat, scan);
                if (!dct_type) {
                    mp4_AddResidual(pat & 32, pYc, stepYc, coeffMB);
                    mp4_AddResidual(pat & 16, pYc+8, stepYc, coeffMB+64);
                    mp4_AddResidual(pat & 8, pYc+stepYc*8, stepYc, coeffMB+128);
                    mp4_AddResidual(pat & 4, pYc+stepYc*8+8, stepYc, coeffMB+192);
                } else {
                    mp4_AddResidual(pat & 32, pYc, stepYc*2, coeffMB);
                    mp4_AddResidual(pat & 16, pYc+8, stepYc*2, coeffMB+64);
                    mp4_AddResidual(pat & 8, pYc+stepYc, stepYc*2, coeffMB+128);
                    mp4_AddResidual(pat & 4, pYc+stepYc+8, stepYc*2, coeffMB+192);
                }
                mp4_AddResidual(pat & 2, pCbc, stepCbc, coeffMB+256);
                mp4_AddResidual(pat & 1, pCrc, stepCrc, coeffMB+320);
            }
        } else if (mb_type >= IPPVC_MBTYPE_INTRA) {
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
            pFc[0] = pYc; pFc[1] = pYc + 8; pFc[2] = pYc + pYoff23; pFc[3] = pYc + pYoff23 + 8; pFc[4] = pCbc; pFc[5] = pCrc;
            mp4_DCTInvCoeffsIntraMB(coeffMB, pMBinfoMT->lnz, pFc, stepFc);
        } else  {
            //***if (mb_not_coded && obmc_disable) {
            //    ippiCopy16x16_8u_C1R(pYr, stepYr, pYc, stepYc);
            //    ippiCopy8x8_8u_C1R(pCbr, stepCbr, pCbc, stepCbc);
            //    ippiCopy8x8_8u_C1R(pCrr, stepCrr, pCrc, stepCrc);
            //} else {
                pat = pMBinfoMT->pat;
                if (interlaced) {
                    field_prediction = pMBinfo->field_info & 1;
                    mb_ftfr = (pMBinfo->field_info >> 1) & 1;
                    mb_fbfr = (pMBinfo->field_info >> 2) & 1;
                    dct_type = pMBinfoMT->dct_type;
                }
             //***   if (!mb_not_coded) {
                    if (!(interlaced && field_prediction)) {
                        if (mb_type != IPPVC_MBTYPE_INTER4V) {
                            if (quarter_sample) {
                                mp4_LimitMVQ(pMBinfo->mv, mvCur, &limitRectL, dx, dy, 16);
                                mp4_ComputeChromaMVQ(mvCur, &mvCbCr);
                            } else {
                                mp4_LimitMV(pMBinfo->mv, mvCur, &limitRectL, dx, dy, 16);
                                mp4_ComputeChromaMV(mvCur, &mvCbCr);
                            }
                            mvCur[1] = mvCur[2] = mvCur[3] = mvCur[0];
                        } else {
                            if (quarter_sample) {
                                mp4_Limit4MVQ(pMBinfo->mv, mvCur, &limitRectL, dx, dy, 8);
                                mp4_ComputeChroma4MVQ(pMBinfo->mv, &mvCbCr);
                            } else {
                                mp4_Limit4MV(pMBinfo->mv, mvCur, &limitRectL, dx, dy, 8);
                                mp4_ComputeChroma4MV(pMBinfo->mv, &mvCbCr);
                            }
                            mp4_LimitMV(&mvCbCr, &mvCbCr, &limitRectC, dx >> 1, dy >> 1, 8);
                        }
                    } else {
                        if (quarter_sample) {
                            mp4_LimitFMVQ(&pMVField[0], &mvCur[0], &limitRectL, dx, dy, 16);
                            mp4_LimitFMVQ(&pMVField[1], &mvCur[2], &limitRectL, dx, dy, 16);
                            mvTmpT.dx = (Ipp16s)mp4_Div2(mvCur[0].dx);
                            mvTmpT.dy = (Ipp16s)(mp4_Div2(mvCur[0].dy << 1) >> 1);
                            mvTmpB.dx = (Ipp16s)mp4_Div2(mvCur[2].dx);
                            mvTmpB.dy = (Ipp16s)(mp4_Div2(mvCur[2].dy << 1) >> 1);
                            mp4_ComputeChromaMV(&mvTmpT, &mvCbCrT);
                            mp4_ComputeChromaMV(&mvTmpB, &mvCbCrB);
                        } else {
                            mp4_LimitFMV(&pMVField[0], &mvCur[0], &limitRectL, dx, dy, 16);
                            mp4_LimitFMV(&pMVField[1], &mvCur[2], &limitRectL, dx, dy, 16);
                            mp4_ComputeChromaMV(&mvCur[0], &mvCbCrT);
                            mp4_ComputeChromaMV(&mvCur[2], &mvCbCrB);
                        }
                    }
                    if (pat)
                        mp4_DCTInvCoeffsInterMB(pInfo, coeffMB, pMBinfoMT->lnz, pat, scan);
                    if (obmc_disable || field_prediction) {
                        if (!field_prediction) {
                            if (quarter_sample) {
                                if (mb_type == IPPVC_MBTYPE_INTER4V) {
                                    mp4_Copy8x8QP_8u(pYr, stepYr, pYc, stepYc, &mvCur[0], rt);
                                    mp4_Copy8x8QP_8u(pYr+8, stepYr, pYc+8, stepYc, &mvCur[1], rt);
                                    mp4_Copy8x8QP_8u(pYr+8*stepYr, stepYr, pYc+8*stepYc, stepYc, &mvCur[2], rt);
                                    mp4_Copy8x8QP_8u(pYr+8*stepYr+8, stepYr, pYc+8*stepYc+8, stepYc, &mvCur[3], rt);
                                } else
                                    mp4_Copy16x16QP_8u(pYr, stepYr, pYc, stepYc, &mvCur[0], rt);
                            } else {
                                if (mb_type == IPPVC_MBTYPE_INTER4V) {
                                    mp4_Copy8x8HP_8u(pYr, stepYr, pYc, stepYc, &mvCur[0], rt);
                                    mp4_Copy8x8HP_8u(pYr+8, stepYr, pYc+8, stepYc, &mvCur[1], rt);
                                    mp4_Copy8x8HP_8u(pYr+8*stepYr, stepYr, pYc+8*stepYc, stepYc, &mvCur[2], rt);
                                    mp4_Copy8x8HP_8u(pYr+8*stepYr+8, stepYr, pYc+8*stepYc+8, stepYc, &mvCur[3], rt);
                                } else
                                    mp4_Copy16x16HP_8u(pYr, stepYr, pYc, stepYc, &mvCur[0], rt);
                            }
                        } else {
                            if (quarter_sample) {
                                mp4_Copy16x8QP_8u(pYr+stepYr*mb_ftfr, stepYr*2, pYc, stepYc*2, &mvCur[0], rt);
                                mp4_Copy16x8QP_8u(pYr+stepYr*mb_fbfr, stepYr*2, pYc+stepYc, stepYc*2, &mvCur[2], rt);
                            } else {
                                mp4_Copy16x8HP_8u(pYr+stepYr*mb_ftfr, stepYr*2, pYc, stepYc*2, &mvCur[0], rt);
                                mp4_Copy16x8HP_8u(pYr+stepYr*mb_fbfr, stepYr*2, pYc+stepYc, stepYc*2, &mvCur[2], rt);
                            }
                        }
                        if (!dct_type) {
                            mp4_AddResidual(pat & 32, pYc, stepYc, coeffMB);
                            mp4_AddResidual(pat & 16, pYc+8, stepYc, coeffMB+64);
                            mp4_AddResidual(pat & 8, pYc+stepYc*8, stepYc, coeffMB+128);
                            mp4_AddResidual(pat & 4, pYc+stepYc*8+8, stepYc, coeffMB+192);
                        } else {
                            mp4_AddResidual(pat & 32, pYc, stepYc*2, coeffMB);
                            mp4_AddResidual(pat & 16, pYc+8, stepYc*2, coeffMB+64);
                            mp4_AddResidual(pat & 8, pYc+stepYc, stepYc*2, coeffMB+128);
                            mp4_AddResidual(pat & 4, pYc+stepYc+8, stepYc*2, coeffMB+192);
                        }
                        if (!field_prediction) {
                            mp4_MC_HP(pat & 2, pCbr, stepCbr, pCbc, stepCbc, coeffMB+256, &mvCbCr, rt);
                            mp4_MC_HP(pat & 1, pCrr, stepCrr, pCrc, stepCrc, coeffMB+320, &mvCbCr, rt);
                        } else {
                            mp4_Copy8x4HP_8u(pCbr+(mb_ftfr ? stepCbr : 0), stepCbr*2, pCbc, stepCbc*2, &mvCbCrT, rt);
                            mp4_Copy8x4HP_8u(pCrr+(mb_ftfr ? stepCrr : 0), stepCrr*2, pCrc, stepCrc*2, &mvCbCrT, rt);
                            mp4_Copy8x4HP_8u(pCbr+(mb_fbfr ? stepCbr : 0), stepCbr*2, pCbc+stepCbc, stepCbc*2, &mvCbCrB, rt);
                            mp4_Copy8x4HP_8u(pCrr+(mb_fbfr ? stepCrr : 0), stepCrr*2, pCrc+stepCrc, stepCrc*2, &mvCbCrB, rt);
                            mp4_AddResidual(pat & 2, pCbc, stepCbc, coeffMB+256);
                            mp4_AddResidual(pat & 1, pCrc, stepCrc, coeffMB+320);
                        }
                    }
                //***}
                if (!obmc_disable && !field_prediction) {
                    //***if (mb_not_coded) {
                    //    ippiCopy8x8_8u_C1R(pCbr, stepCbr, pCbc, stepCbc);
                    //    ippiCopy8x8_8u_C1R(pCrr, stepCrr, pCrc, stepCrc);
                    //    pat = 0;
                    //    mp4_Zero4MV(mvCur);
                    //} else {
                        mp4_MC_HP(pat & 2, pCbr, stepCbr, pCbc, stepCbc, coeffMB+256, &mvCbCr, rt);
                        mp4_MC_HP(pat & 1, pCrr, stepCrr, pCrc, stepCrc, coeffMB+320, &mvCbCr, rt);
                    //***}
                    mp4_OBMC(pInfo, pMBinfo, mvCur, j, curRow, limitRectL, pYc, stepYc, pYr, stepYr, pat >> 2, coeffMB, dct_type);
                }
            //***}
        }
        pMBinfo ++;
        pMBinfoMT ++;
        pMVField += 2;
        pYc += 16; pCrc += 8; pCbc += 8;
        pYr += 16; pCrr += 8; pCbr += 8;
        dx += 16;
    }
}


#ifndef DROP_MULTI_THREAD
mp4_Status mp4_DecodeVOP_S_MT(mp4_Info* pInfo)
{
    int        i, mbPerCol, mbPerRow, fcode_forward;
    IppiRect   spriteRect, vopRect;
    mp4_Status sts = MP4_STATUS_OK;

    // init WarpSpec for Sprites or GMC
    vopRect.x = 0;
    vopRect.y = 0;
    vopRect.width = pInfo->VisualObject.VideoObject.width;
    vopRect.height = pInfo->VisualObject.VideoObject.height;
    if (pInfo->VisualObject.VideoObject.sprite_enable == MP4_SPRITE_STATIC) {
        spriteRect.x = pInfo->VisualObject.VideoObject.sprite_left_coordinate;
        spriteRect.y = pInfo->VisualObject.VideoObject.sprite_top_coordinate;
        spriteRect.width = pInfo->VisualObject.VideoObject.sprite_width;
        spriteRect.height = pInfo->VisualObject.VideoObject.sprite_height;
        fcode_forward = 1;
    } else {
        spriteRect = vopRect; // for shapes they may be different !!!!!!
        fcode_forward = pInfo->VisualObject.VideoObject.VideoObjectPlane.fcode_forward;
    }
    ippiWarpInit_MPEG4_x(pInfo->VisualObject.VideoObject.WarpSpec,
        pInfo->VisualObject.VideoObject.VideoObjectPlane.warping_mv_code_du,
        pInfo->VisualObject.VideoObject.VideoObjectPlane.warping_mv_code_dv,
        pInfo->VisualObject.VideoObject.sprite_warping_points,
        pInfo->VisualObject.VideoObject.sprite_enable,
        pInfo->VisualObject.VideoObject.sprite_warping_accuracy,
        pInfo->VisualObject.VideoObject.VideoObjectPlane.rounding_type,
        pInfo->VisualObject.VideoObject.quarter_sample, fcode_forward,
        &spriteRect, &vopRect);
    // decode basic sprites
    if (pInfo->VisualObject.VideoObject.sprite_enable == MP4_SPRITE_STATIC) {
        int  h;
        //if (pInfo->VisualObject.VideoObject.shape != MP4_SHAPE_TYPE_RECTANGULAR) {
        //    if (mp4_InitVOPShape(pInfo) != MP4_STATUS_OK)
        //        return MP4_STATUS_ERROR;
        //}
        h = (vopRect.height / pInfo->num_threads) & (~1); // must be even
#ifdef _OPENMP
#pragma  omp parallel shared(pInfo, h)
#endif
        {
            IppiRect   vopRect;
            int               idThread = 0;
#ifdef _OPENMP
            idThread = omp_get_thread_num();
#endif
            vopRect.x = 0;
            vopRect.width = pInfo->VisualObject.VideoObject.width;
            vopRect.y = idThread * h;
            vopRect.height = h;
            if (idThread == (pInfo->num_threads - 1))
                vopRect.height = pInfo->VisualObject.VideoObject.height - vopRect.y;
            ippiWarpLuma_MPEG4_8u_C1R_x(pInfo->VisualObject.sFrame.pY, pInfo->VisualObject.sFrame.stepY,
                pInfo->VisualObject.cFrame.pY + vopRect.y * pInfo->VisualObject.cFrame.stepY,
                pInfo->VisualObject.cFrame.stepY, &vopRect, pInfo->VisualObject.VideoObject.WarpSpec);
            if (pInfo->VisualObject.VideoObject.sprite_brightness_change)
                ippiChangeSpriteBrightness_MPEG4_8u_C1IR_x(pInfo->VisualObject.cFrame.pY, pInfo->VisualObject.cFrame.stepY, vopRect.width, vopRect.height, pInfo->VisualObject.VideoObject.VideoObjectPlane.brightness_change_factor);
            vopRect.y >>= 1; vopRect.width >>= 1; vopRect.height >>= 1;
            ippiWarpChroma_MPEG4_8u_P2R_x(pInfo->VisualObject.sFrame.pCb, pInfo->VisualObject.sFrame.stepCb,
                pInfo->VisualObject.sFrame.pCr, pInfo->VisualObject.sFrame.stepCr,
                pInfo->VisualObject.cFrame.pCb + vopRect.y * pInfo->VisualObject.cFrame.stepCb, pInfo->VisualObject.cFrame.stepCb,
                pInfo->VisualObject.cFrame.pCr + vopRect.y * pInfo->VisualObject.cFrame.stepCr, pInfo->VisualObject.cFrame.stepCr,
                &vopRect, pInfo->VisualObject.VideoObject.WarpSpec);
        }
        return MP4_STATUS_OK;
    }
    // decode S(GMC)-VOP
    mbPerCol = pInfo->VisualObject.VideoObject.MacroBlockPerCol;
    mbPerRow = pInfo->VisualObject.VideoObject.MacroBlockPerRow;
    mp4_ResetIntraPredBuffer(pInfo);
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
                    if (mp4_DecodeVOP_S_DecodeSlice(pInfo, curRow, pMBinfoMT) != MP4_STATUS_OK) {
                        sts = MP4_STATUS_ERROR;
                        curRow = mbPerCol;
                    }
            }
            if (curRow < mbPerCol)
                if (sts == MP4_STATUS_OK)
                    mp4_DecodeVOP_S_ReconSlice(pInfo, curRow, pMBinfoMT);
            curRow ++;
        }
    }
    // skip stuffing
    while (mp4_ShowBits(pInfo, 10) == 1)
        mp4_FlushBits(pInfo, 10);
    return sts;
}
#endif

#endif // _OMP_KARABAS


#endif	//DROP_SPRITE
