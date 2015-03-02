/***************************************************************************************** 
Copyright (c) 2009, Marvell International Ltd. 
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Marvell nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY MARVELL ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL MARVELL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************************/


#ifndef _IPPIE_H
#define _IPPIE_H

#include "ippdefs.h"

typedef struct _IppUsmfilterParam {	
	float fAmount;
    float fRadius;
    int iThreshold;
} IppUsmfilterParam;

typedef enum _IppiFormat {
    /* BGR */
    FORMAT_BGRA,
    FORMAT_BGR888,
    FORMAT_BGR555,
    FORMAT_BGR565,
    FORMAT_GRAY8,

    /* RGB */
    FORMAT_RGBA,
    FORMAT_RGB888,
    FORMAT_RGB555,
    FORMAT_RGB565,

} IppiFormat;


IPPAPI(IppStatus, ippiUSMSharpen, (Ipp8u *pSrcRGB, unsigned int phAddrSrcRGB, int iSrcStep, IppiFormat eSrcFormat,
Ipp8u *pDstRGB, unsigned int phAddrDstRGB, int iDstStep, IppiFormat eDstFormat,
int iWidth, int iHeight,
float fRadius, int iThreshold, float fAmount,
float *pReserved1, int iReserved2, Ipp16s *pReserved3))

#endif
/* EOF */


