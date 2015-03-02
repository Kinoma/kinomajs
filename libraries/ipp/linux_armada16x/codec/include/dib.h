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

#ifndef _DIB_H
#define _DIB_H

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define  IIP_WIDTHBYTES_4B(x)  ((((x)+31)&(~31))>>3)    

/*---------------------------------------------------------------------------*/
#define BM              0x424D
#define BI_RGB          0L
#define BI_RLE8         1L
#define BI_RLE4         2L
#define BI_BITFIELDS    3L

/*---------------------------------------------------------------------------*/
#define WRITE_INT8(value, offset, stream)\
    {\
        (stream)[offset]   = (Ipp8u)(value);\
    }

#define WRITE_INT16(value, offset, stream)\
    {\
        register int nTemp=(value);\
        (stream)[offset]   = (Ipp8u)((nTemp >> 8) & 0xff);\
        (stream)[offset+1] = (Ipp8u)(nTemp & 0xff);\
    }

#define WRITE_INT32(value, offset, stream)\
    {\
        register int nTemp=(value);\
        (stream)[offset]   = (Ipp8u)((nTemp >> 24) & 0xff);\
        (stream)[offset+1] = (Ipp8u)((nTemp >> 16) & 0xff);\
        (stream)[offset+2] = (Ipp8u)((nTemp >> 8) & 0xff);\
        (stream)[offset+3] = (Ipp8u)(nTemp & 0xff);\
    }

#define READ_INT8(offset, stream)\
    ((Ipp8u)(stream)[offset])

#define READ_INT16(offset, stream)\
    ((Ipp16u)(stream)[offset+1]|((Ipp16u)(stream)[offset]<<8))

#define READ_INT32(offset, stream)	( (((Ipp32u)(stream)[offset+3])&0xff)|((((Ipp32u)(stream)[offset+2])<<8)&0xff00)|\
((((Ipp32u)(stream)[offset+1])<<16)&0xff0000)|((((Ipp32u)(stream)[offset])<<24)&0xff000000) )

/*---------------------------------------------------------------------------*/
typedef enum {
    YUV_PLANAR,
    YUV_PACKED
} IppYUVStorage;

typedef struct _MDIBSPEC
{
    int       nWidth;
    int       nHeight;
    int       nStep;
    int       nPrecision;
    int       nBitsPerpixel;
    int       nNumComponent;
    int       nClrMode;
    int       nDataSize;
    Ipp8u     *pBitmapData;
} MDIBSPEC;

typedef struct _MYUVSPEC
{
    int             nWidth;
    int             nHeight;
    int             nSamplingFormat;
    IppYUVStorage   nStorageFormat;
    int             nDataSize;
    Ipp8u           *pData;
} MYUVSPEC;

/*---------------------------------------------------------------------------*/

IppCodecStatus ReadDIBFile(MDIBSPEC *pDIBSpec, FILE *file);
IppCodecStatus WriteDIBFile(MDIBSPEC *pDIBSpec, FILE *file);
void DestroyDIB(MDIBSPEC *pDIBSpec);

#ifdef  __cplusplus
}
#endif

#endif

/* EOF */

