/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
#ifndef KINOMA_MPEG4_BITSTREAM_H_CSQ_20070310
#define KINOMA_MPEG4_BITSTREAM_H_CSQ_20070310

#include "kinoma_ipp_common.h"

/*############################################### MPEG4 Put Bitstream BEGIN ##################################################*/
static void __inline k_mp4_PutBit1(Ipp8u** ppBitStream, int* pBitOffset)
{
	if (*pBitOffset == 0) 
	{
		(*ppBitStream)[0] = (Ipp8u)(0x80);//((val & 1) ? 0x80 : 0x00);
		(*pBitOffset) ++;
	}
	else 
	{
		(*ppBitStream)[0] = (Ipp8u)((*ppBitStream)[0] | (Ipp8u)(0x01 << (7 - *pBitOffset)));

		(*ppBitStream) += (*pBitOffset + 1) >> 3;
		*pBitOffset = (*pBitOffset + 1) & 7;
	}
}

static void __inline k_mp4_PutBit0(Ipp8u** ppBitStream, int* pBitOffset)
{
	if (*pBitOffset == 0) 
	{
		(*ppBitStream)[0] = (Ipp8u)(0x00);
		(*pBitOffset) ++;
	}
	else 
	{
		(*ppBitStream)[0] = (Ipp8u)((*ppBitStream)[0] & (Ipp8u)(0xff << (8 - *pBitOffset)));
		(*ppBitStream) += (*pBitOffset + 1) >> 3;
		*pBitOffset = (*pBitOffset + 1) & 7;
	}
}

static void __inline k_mp4_PutBits(Ipp8u** ppBitStream, int* pBitOffset, Ipp16u val, int len)
{
	val <<= 16 - len;
	if (*pBitOffset == 0)
	{
		(*ppBitStream)[0] = (Ipp8u)(val >> 8);
		if (len > 8)
		{
			(*ppBitStream)[1] = (Ipp8u)(val);
		}
	}
	else
	{
		(*ppBitStream)[0] = (Ipp8u)(((*ppBitStream)[0] & (0xFF << (8 - *pBitOffset))) | (Ipp8u)(val >> (8+ *pBitOffset)));
		if (len > 8 - *pBitOffset)
		{
			val <<= 8 - *pBitOffset;
			(*ppBitStream)[1] = (Ipp8u) (val >> 8);
			if (len > 16 -  *pBitOffset) 
			{
               (*ppBitStream)[2] = (Ipp8u)(val);
			}
		}
	}

	*ppBitStream += (*pBitOffset + len) >> 3;
	*pBitOffset = (*pBitOffset + len ) & 7;

}

/*
static void __inline k32_mp4_PutBits(Ipp8u** ppBitStream, int* pBitOffset, Ipp32u val, int n)
{
    val <<= 32 - n;
    if (*pBitOffset == 0) {
        (*ppBitStream)[0] = (Ipp8u)(val >> 24);
        if (n > 8) {
            (*ppBitStream)[1] = (Ipp8u)(val >> 16);
            if (n > 16) {
                (*ppBitStream)[2] = (Ipp8u)(val >> 8);
                if (n > 24) {
                    (*ppBitStream)[3] = (Ipp8u)(val);
                }
            }
        }
    } else {
        (*ppBitStream)[0] = (Ipp8u)(((*ppBitStream)[0] & (0xFF << (8 - *pBitOffset))) | (Ipp8u)(val >> (24 + *pBitOffset)));
        if (n > 8 - *pBitOffset) {
            val <<= 8 - *pBitOffset;
            (*ppBitStream)[1] = (Ipp8u)(val >> 24);
            if (n > 16 - *pBitOffset) {
                (*ppBitStream)[2] = (Ipp8u)(val >> 16);
                if (n > 24 - *pBitOffset) {
                    (*ppBitStream)[3] = (Ipp8u)(val >> 8);
                    if (n > 32 - *pBitOffset) {
                        (*ppBitStream)[4] = (Ipp8u)val;
                    }
                }
            }
        }
    }
    (*ppBitStream) += (*pBitOffset + n) >> 3;
    *pBitOffset = (*pBitOffset + n) & 7;
}
*/
/*############################################### MPEG4 Put Bitstream END ##################################################*/


/*############################################### MPEG4 Get Bitstream BEGIN ##################################################*/
static Ipp32u __inline k_mp4_ShowBits(Ipp8u **ppBitStream,int *pBitOffset, int n)
{
    Ipp8u* ptr = *ppBitStream;
    Ipp32u tmp = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] <<  8) | (ptr[3]);
    tmp <<= *pBitOffset;
    tmp >>= 32 - n;
    return tmp;

};

static Ipp32u __inline  k_mp4_GetBits(Ipp8u **ppBitStream, int *pBitOffset, int n)
{
    Ipp8u* ptr = *ppBitStream;
    register Ipp32u tmp = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] <<  8) | (ptr[3]);
    tmp <<= *pBitOffset;
    tmp >>= 32 - n;
    n = n + *pBitOffset;
    *ppBitStream += n >> 3;
    *pBitOffset = n & 7;
    return tmp;
};

static void __inline  k_mp4_FlushBits(Ipp8u **ppBitStream, int *pBitOffset, int n)
{
    register int m = n + *pBitOffset;
    *ppBitStream += m >> 3;
    *pBitOffset = m & 7;
};

static Ipp32u __inline k_mp4_ShowBits11(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 16) | ((*ppBitStream)[1]<<8) | (*ppBitStream)[2];
	tmp >>= (13-*pBitOffset);
	return (tmp & 2047);
}

static Ipp32u __inline k_mp4_ShowBits12(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 16) | ((*ppBitStream)[1]<<8) | (*ppBitStream)[2];
	tmp >>= (12-*pBitOffset);
	return (tmp & 4095);
}

static Ipp32u __inline k_mp4_ShowBits15(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 16) | ((*ppBitStream)[1]<<8) | (*ppBitStream)[2];
	tmp >>= (9-*pBitOffset);
	return (tmp & 32767);
}

static Ipp32u __inline k_mp4_GetBit1(Ipp8u **ppBitStream,int *pBitOffset)
{
	//Ipp32u tmp = (*ppBitStream)[0];
	//register int n = 1 + *pBitOffset;
	//tmp >>= (7-*pBitOffset);

	//*ppBitStream += n >> 3;
	//*pBitOffset = n & 7;
 //
	//return (tmp & 1);

    register Ipp32u tmp = *ppBitStream[0];
    if (*pBitOffset == 7)
	{
        *pBitOffset = 0;
        (*ppBitStream)++;
    }
	else 
	{
        tmp >>= 7 - *pBitOffset;
        (*pBitOffset)++;
    }
    return (tmp & 1);
}

static Ipp32u __inline k_mp4_GetBit2(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 8) | (*ppBitStream)[1];
	register int n = 2 + *pBitOffset;
	tmp >>= (14-*pBitOffset);

	*ppBitStream += n >> 3;
	*pBitOffset = n & 7;
 
	return (tmp & 3);
}

static Ipp32u __inline k_mp4_GetBit5(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 8) | (*ppBitStream)[1];
	register int n = 5 + *pBitOffset;
	tmp >>= (11-*pBitOffset);

	*ppBitStream += n >> 3;
	*pBitOffset = n & 7;
	
	return (tmp & 31);
}

static Ipp32u __inline k_mp4_GetBit6(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 8) | (*ppBitStream)[1];
	register int n = 6 + *pBitOffset;
	tmp >>= (10-*pBitOffset);

	*ppBitStream += n >> 3;
	*pBitOffset = n & 7;
 
	return (tmp & 63);
}

static Ipp32u __inline k_mp4_GetBit8(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 8) | (*ppBitStream)[1];
	register int n = 8 + *pBitOffset;
	tmp >>= (8-*pBitOffset);

	*ppBitStream += 1;
	*pBitOffset = n & 7;
 
	return (tmp & 255);
}

static Ipp32u __inline k_mp4_GetBit11(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 16) | ((*ppBitStream)[1]<<8) | (*ppBitStream)[2];
	register int n = 11 + *pBitOffset;
	tmp >>= (13-*pBitOffset);

	*ppBitStream += n >> 3;
	*pBitOffset = n & 7;
 
	return (tmp & 2047);
}

static Ipp32u __inline k_mp4_GetBit12(Ipp8u **ppBitStream,int *pBitOffset)
{
	register Ipp32u tmp = ((*ppBitStream)[0]<< 16) | ((*ppBitStream)[1]<<8) | (*ppBitStream)[2];
	register int n = 12 + *pBitOffset;
	tmp >>= (12-*pBitOffset);

	*ppBitStream += n >> 3;
	*pBitOffset = n & 7;
 
	return (tmp & 4095);
}
/*############################################### MPEG4 Get Bitstream END ##################################################*/

#endif
