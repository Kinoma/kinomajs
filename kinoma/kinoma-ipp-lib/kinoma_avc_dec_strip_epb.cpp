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
#include <stdio.h>
#include <assert.h>

#include "kinoma_ipp_lib.h"
#include "kinoma_utilities.h"
#include "umc_media_data_ex.h"
#include "umc_h264_bitstream.h"

void SwapMemoryAndRemovePreventingBytes_misaligned_c(unsigned char *pDestination, unsigned char *pSource, int nSrcSize, int *nDstSize_in_out)
{
    Ipp32u *pDst__m_pDest;                                            // (Ipp32u *) pointer to destination buffer
    Ipp32u pDst__m_nByteNum;                                          // (Ipp32u) number of current byte in dword
    Ipp32u pDst__m_iCur;                                              // (Ipp32u) current dword
 
    Ipp8u *pSrc__m_pSource;                                           // (Ipp8u *) pointer to destination buffer
    Ipp8u  pSrc__m_CurBytes[4];                                        // (Ipp8u) current bytes in stream, one for alignment
    Ipp32u pSrc__m_nZeros;                                            // (Ipp32u) number of preceding zeros
    Ipp32u pSrc__m_nRemovedBytes;                                     // (Ipp32u) number of removed bytes
	int nDstSize;

	int i;

    // DwordPointer object is swapping written bytes
    // H264SourcePointer_ removes preventing start-code bytes

    // reset pointer(s)
	pSrc__m_pSource	    = (Ipp8u *) pSource;
	pSrc__m_CurBytes[0] = pSrc__m_pSource[0];
	pSrc__m_CurBytes[1] = pSrc__m_pSource[1];
	pSrc__m_CurBytes[2] = pSrc__m_pSource[2];
	pSrc__m_pSource    += 3;
	pSrc__m_nZeros      = 0;
	pSrc__m_nRemovedBytes = 0;

	if (0 == pSrc__m_CurBytes[1])
	{
		if (0 == pSrc__m_CurBytes[0])
			pSrc__m_nZeros = 2;
		else
			pSrc__m_nZeros = 1;
	}

    pDst__m_pDest = (Ipp32u *) pDestination;
    pDst__m_nByteNum = 0;
    pDst__m_iCur = 0;

    // do swapping
    i = 0;
    while (i < (int)((Ipp32u) nSrcSize - 2))
    {
		pDst__m_iCur = (pDst__m_iCur & ~0x0ff) | ((Ipp32u) pSrc__m_CurBytes[0]);
        if ((2 <= pSrc__m_nZeros) &&(3 == pSrc__m_CurBytes[2]))
        {
            pSrc__m_nRemovedBytes += 1;

            // first byte is zero
            pSrc__m_CurBytes[1] = pSrc__m_pSource[0];
            pSrc__m_CurBytes[2] = pSrc__m_pSource[1];
            pSrc__m_pSource += 2;

            if (0 == pSrc__m_CurBytes[2])
            {
                if (pSrc__m_CurBytes[1])
                    pSrc__m_nZeros = 2;
                else
                    pSrc__m_nZeros = 1;
            }
            else
                pSrc__m_nZeros = 0;

            i += 2;
        }
        else
        {
            // check current byte
            if (0 == pSrc__m_CurBytes[2])
                pSrc__m_nZeros += 1;
            else
                pSrc__m_nZeros = 0;

            // shift bytes
            pSrc__m_CurBytes[0] = pSrc__m_CurBytes[1];
            pSrc__m_CurBytes[1] = pSrc__m_CurBytes[2];
            pSrc__m_CurBytes[2] = pSrc__m_pSource[0];

            pSrc__m_pSource += 1;
            i += 1;
        }

		if (4 == ++pDst__m_nByteNum)
        {
            *pDst__m_pDest   = pDst__m_iCur;
            pDst__m_pDest   += 1;
            pDst__m_nByteNum = 0;
            pDst__m_iCur     = 0;
        }
        else
            pDst__m_iCur   <<= 8; 
	}

    // copy last 3 bytes
    while (i < (int)((Ipp32u) nSrcSize))
    {
		pDst__m_iCur = (pDst__m_iCur & ~0x0ff) | ((Ipp32u) pSrc__m_CurBytes[0]);

		pSrc__m_CurBytes[0] = pSrc__m_CurBytes[1];
        pSrc__m_CurBytes[1] = pSrc__m_CurBytes[2];
        pSrc__m_CurBytes[2] = (Ipp8u) -1;

        i += 1;

		if (4 == ++pDst__m_nByteNum)
        {
            *pDst__m_pDest = pDst__m_iCur;
            pDst__m_pDest += 1;
            pDst__m_nByteNum = 0;
            pDst__m_iCur = 0;
        }
        else
            pDst__m_iCur <<= 8; 
    }

    // write padding bytes
    nDstSize = nSrcSize - pSrc__m_nRemovedBytes;
    while (nDstSize & 3)
    {
		pDst__m_iCur = (pDst__m_iCur & ~0x0ff) | ((Ipp32u) 0);
        ++(nDstSize);
        if (4 == ++pDst__m_nByteNum)
        {
            *pDst__m_pDest = pDst__m_iCur;
            pDst__m_pDest += 1;
            pDst__m_nByteNum = 0;
            pDst__m_iCur = 0;
        }
        else
            pDst__m_iCur <<= 8; 
    }

	*nDstSize_in_out = nDstSize;
}


#define SWAP(s)  s = (((s>>0)&0xff)<<24)|(((s>>8)&0xff)<<16)|(((s>>16)&0xff)<<8)|(((s>>24)&0xff)<<0)

void SwapMemoryAndRemovePreventingBytes_aligned4_c(unsigned char *pDestination, unsigned char *pSource, int nSrcSize, int *nDstSize_int_out)
{
	unsigned long *src = (unsigned long *)pSource;
	unsigned long *dst = (unsigned long *)pDestination;
	unsigned long b1, b2, b3, tmp;
	int			  count = (nSrcSize>>2)-1;
	int nDstSize;

	b1 = *(src++);	
	b2 = *(src++);	
	SWAP(b1);

	while(1)
	{
		//check self								
		tmp = b1 >> 8;									
		if( tmp == 0x00000003 ) 
			goto generic_case_0;		

		tmp = b1 << 8;									
		if( tmp == 0x00000300 ) 
			goto generic_case_0;		

		//check connection						
		SWAP(b2);
		b3 = b1<<16;										
		b3 = (b3 | b2>>16 );							
														
		tmp = b3 >> 8;									
		if( tmp == 0x00000003 )
			goto generic_case_0;

		tmp = b3 << 8;									
		if( tmp == 0x00000300 ) 
			goto generic_case_0;		

		*(dst++) = b1;									
		//next
		b1 = b2;
		if( --count == 0 )
			break;

		b2 = *(src++);
	}

	nDstSize = (nSrcSize>>2)<<2;

	//leftover
	{
		int				left_bytes = nSrcSize&0x03;
		unsigned char	*sss = (unsigned char *)src;
		//unsigned char	*ddd = (unsigned char *)dst;
		unsigned char	d1=0, d2=0, d3=0;
		int				mark1 = 0, mark2 = 0;
		//int s1, s2, s3, s4, s5;
		
		if( left_bytes >= 1 )
			d3 = sss[0];
		if( left_bytes >= 2 )
			d2 = sss[1];
		if( left_bytes >= 3 )
			d1 = sss[2];
		
		/*check self*/									
		tmp = b1 >> 8;									
		if( tmp == 0x00000003 ) 
			mark1 = 1;		
														
		tmp = b1 << 8;									
		if( tmp == 0x00000300 ) 	
			mark1 = 2;		

		if( mark1 == 0 )
			*(dst++) = b1;

		if( left_bytes == 0 )
		{
			*nDstSize_int_out = nDstSize;
			return;
		}

		b2 = (d3<<24)|(d2<<16)|(d1<<8);
		/*check connection*/							
		b1 <<= 16;										
		b1   = (b1 | b2>>16 );							
														
		tmp = b1 >> 8;									
		if( tmp == 0x00000003 ) 
			mark2 = 1;		
														
		tmp = b1 << 8;									
		if( tmp == 0x00000300 )
			mark2 = 2;		

		if( mark1 + mark2 == 0 )
		{
			*(dst++) = b2;	
			nDstSize += 4;
			*nDstSize_int_out = nDstSize;
			return;
		}

		src++;	//consume b2
		if( mark1 == 0 )
			dst--;	//step back b1
		
		//tricky case goes to generic_case
	}

generic_case_0:
	{
		int		progress = (((unsigned char*)dst) - ((unsigned char*)pDestination));

		nSrcSize -= progress;
		src		 -= 2;		//step back 2 long, b1, b2
		SwapMemoryAndRemovePreventingBytes_misaligned_c( (unsigned char*)dst, (unsigned char*)src, nSrcSize, &nDstSize); 
		nDstSize += progress;
		*nDstSize_int_out = nDstSize;

		return;
	}
}


int SwapInQTMediaSample3(int spspps_only, int naluLengthSize, unsigned char *src, int size, void *m_void )
{
	MediaData3_V51	  *m   = (MediaData3_V51  *)m_void;
	unsigned char *dst = (unsigned char *)m->m_pBufferPointer;
	int			  buf_size = m->m_nBufferSize;
	int			  wanted_buf_size = 0;
	int			  s = 0;
	int			  d = 0;
    int			  i = 0;
 	
	wanted_buf_size = size + m->limit*3 + 128;
	if( wanted_buf_size > buf_size )
	{
		buf_size = wanted_buf_size;
		if( dst != NULL ) {
			free( dst );
			m->m_pBufferPointer = NULL;
		}

		dst = (unsigned char *)malloc(buf_size);
		if (!dst) {
			//@@
			return 0;
		}
		m->m_pBufferPointer = dst;
		m->m_nBufferSize	= wanted_buf_size;
	}
 
	while( s <= size - 3 )
	{
		int				src_size = 0;
		int				dst_size = 0;
		unsigned char	nalu_type;
		int				is_valid_nalu;

		if( naluLengthSize == 4 )
			src_size = (src[s+0]<<24)|(src[s+1]<<16)|(src[s+2]<<8)|(src[s+3]);
		else if(  naluLengthSize == 2 )
			src_size = (src[s+0]<<8)|(src[s+1]<<0);
		else
			src_size = src[s+0];

		s += naluLengthSize;

		if( s + src_size > size || src_size <= 0 )	//safe guard frame boundary by checking slice boundary
			break;

		//only let in NALU that we can handle			bnie  --2/19/2009
		nalu_type = src[s] & NAL_UNITTYPE_BITS;
		//is_valid_nalu;

		if( spspps_only )
			is_valid_nalu = (UMC::NAL_UT_SPS   == nalu_type )  || (UMC::NAL_UT_PPS       == nalu_type );
		else
			is_valid_nalu = (UMC::NAL_UT_SLICE == nalu_type )  || (UMC::NAL_UT_IDR_SLICE == nalu_type );

#if 0 //***bnie: 3/31/2009 inherit from intel code, no really necessary...
		if(src_size == 0)
			is_valid_nalu = 0;
		if (8 == src_size)
		{
			Ipp8u *src_buffer = src+s;
			// Need to access src.data as bytes, not as a couple of Ipp32u's,
			// because theoretically it might not be 4-byte aligned.
			// Check the later bytes first, since they're more likely to
			// be non-zero in the general case.
			if ((src_buffer[7] == 0) && (src_buffer[6] == 0) &&
				(src_buffer[5] == 0) && (src_buffer[4] == 0) &&
				(src_buffer[3] == 0) && (src_buffer[2] == 0) &&
				(src_buffer[1] == 0) && (src_buffer[0] == 0))
			{
				is_valid_nalu = 0;
			}
		}
#endif

		if ( is_valid_nalu )
		{
			unsigned char *this_src		 = src+s;
			unsigned char *this_dst		 = dst+d+4;

			*(int *)dst = 0x80808080;	//***bnie: used to be 0x00000001 start code,
#if 0
			int			  src_bit_offset = (((int)(this_src))&0x03)<<3;
			if( src_bit_offset != 0 && src_size >= 16 )
			{
				long *sss			= (long *)(this_src - (src_bit_offset>>3));
				long  round			= *sss;
				int	  round_size	= 4 - (src_bit_offset>>3);
				int	  this_src_size = src_size - round_size;

				round = (round>>src_bit_offset)<< src_bit_offset;
				SWAP(round);

				*(long*)this_dst = round;
				this_src += round_size;
#ifdef _WIN32_WCE
				SwapMemoryAndRemovePreventingBytes_aligned4_arm_v6(this_dst+4, this_src, this_src_size, &dst_size);
#else
				SwapMemoryAndRemovePreventingBytes_aligned4_c(this_dst+4, this_src, this_src_size, &dst_size);
#endif
				dst_size += 4;
				m->bit_offset_ary[i] = 31 - src_bit_offset;
			}
			else
#endif
			{
				SwapMemoryAndRemovePreventingBytes_misaligned_c(this_dst, this_src, src_size, &dst_size);
				m->bit_offset_ary[i] = 31;
			}

			m->data_ptr_ary[i]	 = (unsigned long *)this_dst;
			m->size_ary[i]		 = dst_size;
			m->nalu_type_ary[i]  = src[s]& NAL_UNITTYPE_BITS;

			i++;
			if( i > 20 )
				return -1;
			d += dst_size+4;
		}

		s += src_size;
	}

    m->count  = i;
	dst[d+0] = 0x80;
	dst[d+1] = 0x80;
	dst[d+2] = 0x80;
	dst[d+3] = 0x80;
	dst[d+4] = 0x80;
	dst[d+5] = 0x80;
	dst[d+6] = 0x80;
	dst[d+7] = 0x80;

	return 0;
}
