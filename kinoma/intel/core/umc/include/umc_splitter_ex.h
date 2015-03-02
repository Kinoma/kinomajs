/* /////////////////////////////////////////////////////////////////////////////// */
/*
//
//              INTeL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//        Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//
*/

#ifndef __UMC_MPEG_SPLITTER_H__
#define __UMC_MPEG_SPLITTER_H__

#include "umc_splitter.h"
#include "umc_media_data_ex.h"

namespace UMC
{

    typedef struct
    {
        vm_var64s position;
        vm_var64s size;
    } sInfo_sample;

    class MpegSplitterParams : public SplitterParams
    {
        DYNAMIC_CAST_DECL(MpegSplitterParams, SplitterParams)
    public:
        MediaDataEx_V51* m_mediaData;
    };
}

#endif
