/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#include "kinoma_ipp_lib.h"
#include "kinoma_ipp_common.h"
#include "kinoma_utilities.h"

//Memory
//ipps
Ipp8u*    (__STDCALL *ippsMalloc_8u_universal)			(int len)=NULL;
Ipp32s*   (__STDCALL *ippsMalloc_32s_universal)			(int len)=NULL;
Ipp32u*	  (__STDCALL *ippsMalloc_32u_universal)			(int len)=NULL;
void	  (__STDCALL *ippsFree_universal)				(void* ptr)=NULL;
IppStatus (__STDCALL *ippsZero_8u_universal)			(Ipp8u* pDst, int len)=NULL;
IppStatus (__STDCALL  *ippsZero_16s_universal)			(Ipp16s* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsZero_32s_universal)			(Ipp32s* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsZero_32sc_universal)			(Ipp32sc* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsSet_8u_universal)				(Ipp8u val, Ipp8u* pDst, int len)=NULL;
IppStatus (__STDCALL *ippiSet_8u_C1R_universal) 		(Ipp8u value, Ipp8u* pDst, int dstStep, IppiSize roiSize )=NULL;
IppStatus (__STDCALL *ippsSet_16s_universal)			( Ipp16s val, Ipp16s* pDst, int len )=NULL;
IppStatus (__STDCALL *ippsSet_32s_universal)			( Ipp32s val, Ipp32s* pDst, int len )=NULL;
IppStatus (__STDCALL *ippsCopy_1u_universal) 			(const Ipp8u *pSrc, int srcBitOffset, Ipp8u *pDst, int dstBitOffset, int len)=NULL;
IppStatus (__STDCALL *ippsCopy_8u_universal)			(const Ipp8u* pSrc, Ipp8u* pDst, int len)=NULL;
IppStatus (__STDCALL *ippiCopy_8u_C1R_universal) 		(const Ipp8u *pSrc, int srcStep, Ipp8u *pDst, int dstStep, IppiSize roiSize)=NULL;
IppStatus (__STDCALL *ippsCopy_16s_universal)			(const Ipp16s* pSrc, Ipp16s* pDst, int len )=NULL;
IppStatus (__STDCALL *ippsCopy_32s_universal)			(const Ipp32s* pSrc, Ipp32s* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsMove_32s_universal)			(const Ipp32s* pSrc, Ipp32s* pDst, int len)=NULL;
IppStatus  (__STDCALL *ippiResize_8u_C1R_universal)		(const Ipp8u* pSrc, IppiSize srcSize, int srcStep, IppiRect srcRoi,Ipp8u* pDst, int dstStep, IppiSize dstRoiSize, double xFactor, double yFactor, int interpolation)=NULL;


Ipp8u* __STDCALL ippsMalloc_8u_c(int len)
{
#if 1
	Ipp8u *mem_ptr;
	Ipp8u *tmp_ptr;
	const int align = 32;

	if ((tmp_ptr = (Ipp8u*) malloc(len + align)) != NULL) 
	{
		/* Align the tmp pointer */
		mem_ptr = 	(Ipp8u*) ((unsigned long) (tmp_ptr + align - 1) & (~(unsigned long) (align - 1)));

		if (mem_ptr == tmp_ptr)
			mem_ptr += align;

		*(mem_ptr - 1) = (Ipp8u) (mem_ptr - tmp_ptr);

		/* Return the aligned pointer */
		return ((Ipp8u *)mem_ptr);
	}
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsMalloc_8u_c",-1);
#endif

#if 0
	{
		return (ippsMalloc_8u_c(len));
	}
#endif

	return(NULL);
}

Ipp32u* __STDCALL  ippsMalloc_32u_c(int len)
{
#if 1
	Ipp8u *mem_ptr;
	Ipp8u *tmp_ptr;
	const int align = 32;

	if ((tmp_ptr = (Ipp8u *) malloc(len*sizeof(Ipp32u) + align)) != NULL) 
	{
		/* Align the tmp pointer */
		mem_ptr = 	(Ipp8u *) ((unsigned long) (tmp_ptr + align - 1) & (~(unsigned long) (align - 1)));

		if (mem_ptr == tmp_ptr)
			mem_ptr += align;

		*(mem_ptr - 1) = (Ipp8u) (mem_ptr - tmp_ptr);

		/* Return the aligned pointer */
		return ((Ipp32u *)mem_ptr);
	}
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsMalloc_32u",-1);
#endif

#if 0
	return (ippsMalloc_32u(len));
#endif
	return(NULL);
}

Ipp32s* __STDCALL  ippsMalloc_32s_c(int len)
{
#if 1
	Ipp8u *mem_ptr;
	Ipp8u *tmp_ptr;
	const int align = 32;

	if ((tmp_ptr = (Ipp8u *) malloc(len*sizeof(Ipp32s) + align)) != NULL) 
	{
		/* Align the tmp pointer */
		mem_ptr = 	(Ipp8u *) ((unsigned long) (tmp_ptr + align - 1) & (~(unsigned long) (align - 1)));

		if (mem_ptr == tmp_ptr)
			mem_ptr += align;

		*(mem_ptr - 1) = (Ipp8u) (mem_ptr - tmp_ptr);

		/* Return the aligned pointer */
		return ((Ipp32s *)mem_ptr);
	}
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsMalloc_32s",-1);
#endif

#if 0
	return (ippsMalloc_32s(len));
#endif
	
	return(NULL);
}


void __STDCALL ippsFree_c(void* ptr)
{
#if 1
	Ipp8u * tmp_ptr;

	if (ptr == NULL)
		return;

	tmp_ptr = (Ipp8u*)ptr;

	tmp_ptr -= *(tmp_ptr - 1);

	free(tmp_ptr);
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsFree_c",-1);
#endif

#if 0
	{
		ippsFree_c(ptr);
	}
#endif
}


IppStatus __STDCALL ippsCopy_8u_c(const Ipp8u* pSrc, Ipp8u* pDst, int len)
{
#if 1
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	if(len < 1)
		return ippStsSizeErr;

	memcpy(pDst, pSrc, len);
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsCopy_8u",-1);
#endif		

#if 0
	{
		IppStatus sts;
		sts = ippsCopy_8u(pSrc, pDst, len);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippiCopy_8u_C1R_c ( const Ipp8u* pSrc, int srcStep, Ipp8u* pDst, int dstStep,IppiSize roiSize )
{

	int	i, j, k;
	const Ipp8u *pSrctmp = pSrc;
	Ipp8u *pDsttmp=pDst;


	Profile_Start(ippiCopy_8u_C1R_c_profile);

	i = roiSize.width;
	j = roiSize.height;

	for(k=0; k<j; k++)
	{
		memcpy(pDsttmp, pSrctmp, i);
		pSrctmp += srcStep;
		pDsttmp += dstStep;

	}

	Profile_End(ippiCopy_8u_C1R_c_profile);

	return ippStsNoErr;

}


IppStatus __STDCALL ippsCopy_16s_c( const Ipp16s* pSrc, Ipp16s* pDst, int len )
{
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	if(len <=0)
		return ippStsSizeErr;

	memcpy(pDst,pSrc,len*sizeof(Ipp16s));

	return ippStsNoErr;
}



IppStatus __STDCALL ippsCopy_32s_c(const Ipp32s* pSrc, Ipp32s* pDst, int len)
{
#if 1
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	if(len <=0)
		return ippStsSizeErr;

	memcpy(pDst, pSrc, len*sizeof(Ipp32s));
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsCopy_32s_c",-1);
#endif	

#if 0
	{
		IppStatus sts;
		sts = ippsCopy_32s_c(pSrc, pDst, len);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippsZero_8u_c(Ipp8u* pDst, int len)
{
#if 1
	if(!pDst)
		return ippStsNullPtrErr;

	if(len < 1)
		return ippStsSizeErr;


	Profile_Start(ippsZero_8u_c_profile);


	memset((unsigned char*)pDst, 0, len);


	Profile_End(ippsZero_8u_c_profile);


#endif

#ifdef PRINT_REF_INFO
	//dump("ippsZero_8u_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsZero_8u_c(pDst, len);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsZero_16s_c(Ipp16s* pDst, int len)
{
#if 1
	if(!pDst)
		return ippStsNullPtrErr;

	if(len <=0)
		return ippStsSizeErr;

	memset((unsigned char*)pDst, 0, len*sizeof(Ipp16s));
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsZero_16s_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsZero_16s_c(pDst, len);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsZero_32s_c(Ipp32s* pDst, int len)
{
#if 1
	if(!pDst)
		return ippStsNullPtrErr;

	if(len <=0)
		return ippStsSizeErr;

	memset((unsigned char*)pDst, 0, len*sizeof(Ipp32s));
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsZero_32s_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsZero_32s_c(pDst, len);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsZero_32sc_c(Ipp32sc* pDst, int len)
{
#if 1
	if(!pDst)
		return ippStsNullPtrErr;

#ifdef KINOMA_DEBUG
	{
		int alignment = ((int)pDst)%4;

		if( alignment != 0 || len < 32 )
			fprintf( stderr, "ippsZero_32sc_c: alignment: %4d, len: %4d\n", alignment, len );
	}
#endif

	if(len <=0)
		return ippStsSizeErr;

	memset((unsigned char*)pDst, 0, len*sizeof(Ipp32sc));

#endif

#ifdef PRINT_REF_INFO
	//dump("ippsZero_32sc",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsZero_32sc(pDst, len);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsSet_8u_c(Ipp8u val, Ipp8u* pDst, int len)
{
#if 1
	if(!pDst)
		return ippStsNullPtrErr;

	if(len < 1)
		return ippStsSizeErr;

	Profile_Start(ippsSet_8u_c_profile);

	memset((unsigned char*)pDst, val, len);

	Profile_End(ippsSet_8u_c_profile);


#endif

#ifdef PRINT_REF_INFO
	//dump("ippsSet_8u_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsSet_8u_c(val, pDst, len);
		return sts;
	}
#endif

	return ippStsNoErr;

}
IppStatus __STDCALL ippiSet_8u_C1R_c( Ipp8u value, Ipp8u* pDst, int dstStep, IppiSize roiSize )
{
	int i;//, j;


	Profile_Start(ippiSet_8u_C1R_c_profile);

	for(i=0; i<roiSize.height; i++)
	{
		memset(pDst, value, roiSize.width);
		pDst += dstStep;
	}

	Profile_End(ippiSet_8u_C1R_c_profile);

	return ippStsNoErr;
}

IppStatus __STDCALL ippsSet_16s_c( Ipp16s val, Ipp16s* pDst, int len )
{
	int i;
	if (!pDst)
		return ippStsNullPtrErr;
	if (len < 1)
		return ippStsSizeErr;

	for (i = 0; i < len; i++)
	{
		pDst[i] = val;
	}
	return ippStsNoErr;
}

IppStatus __STDCALL ippsSet_32s_c( Ipp32s val, Ipp32s* pDst, int len )
{
	int i;
	if (!pDst)
		return ippStsNullPtrErr;
	if (len < 1)
		return ippStsSizeErr;

	for (i = 0; i < len; i++)
	{
		pDst[i] = val;
	}

	return ippStsNoErr;
}


IppStatus __STDCALL ippsMove_32s_c(const Ipp32s* pSrc, Ipp32s* pDst, int len)
{
#if 1
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	memmove((unsigned char*)pDst,(unsigned char*)pSrc,len*sizeof(Ipp32s));	
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsMove_32s",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMove_32s(pSrc, pDst, len);
		return sts;
	}
#endif

	return ippStsNoErr;
}


static const Ipp8u bitSrcMask[8] = 
{
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01	//10000000, 01000000, 00100000, 00010000, 00001000, 00000100, 00000010, 00000001
};
static const Ipp8u bitDstMask[8] = 
{
	0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE	//01111111, 10111111, 11011111, 11101111, 11110111, 11111011, 11111101, 11111110
};
IppStatus __STDCALL ippsCopy_1u_c(const Ipp8u *pSrc, int srcBitOffset, Ipp8u *pDst, int dstBitOffset, int len)
{
#if 1
	Ipp8u *pS, *pD, tmpS;
	int i;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	if(len < 1)
		return ippStsSizeErr;

	pS = (Ipp8u *)pSrc;
	pD = pDst;
	for (i = 0; i < len; i++)
	{
		tmpS = pS[0] & bitSrcMask[srcBitOffset];
		if (srcBitOffset < dstBitOffset)
			tmpS >>= (dstBitOffset - srcBitOffset);
		else
			tmpS <<= (srcBitOffset - dstBitOffset);

		pD[0] = (pD[0] & bitDstMask[dstBitOffset]) | tmpS;

		pS += (srcBitOffset+1) >> 3;
		pD += (dstBitOffset+1) >> 3;
		srcBitOffset = (srcBitOffset+1) & 7;
		dstBitOffset = (dstBitOffset+1) & 7;
	}
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsCopy_1u",-1);
#endif		

#if 0
	{
		IppStatus sts;
		sts = ippsCopy_1u(pSrc, srcBitOffset, pDst, dstBitOffset, len);
		return sts;
	}
#endif

	return ippStsNoErr;
}


// srcRoi.x/y must be ZERO
// and interpolation == IPPI_INTER_LINEAR ONLY
// So, that means this function is an specical functions of general ippiResize_8u_C1R in ipp 
//                                                                  -- WWD in 2006-09-17
IppStatus __STDCALL  ippiResize_8u_C1R_c (const Ipp8u* pSrc, IppiSize srcSize, int srcStep, IppiRect srcRoi,
                                      Ipp8u* pDst, int dstStep, IppiSize dstRoiSize,
                                      double xFactor, double yFactor, int interpolation)
{
	// interpolation == IPPI_INTER_LINEAR ONLY
	int	i;

	Ipp8u* pDsttmp = pDst;
	const Ipp8u* pSrctmp = pSrc;

	//assert(xFactor >= 1.0);
	//assert(yFactor >= 1.0);

#if 1	// This should be orginal picture
	for(i=0; i<srcRoi.height; i++)
	{
		// do not expand in X direction
		memcpy(pDsttmp, pSrctmp, srcRoi.width);

		// Here should expand right pixels


		pSrctmp = pSrc + i*srcStep;
		pDsttmp = pDst + i*dstStep;
	}

	pSrctmp -= srcStep;
	// next expand bottom
	for(;i<dstRoiSize.height; i++)
	{
		memcpy(pDsttmp, pSrctmp, srcRoi.width);
		pDsttmp += dstStep;
	}
#else
	// Linear interpolation. 
	//                 -- I think this is not good for most situation
	//                               -- WWD in 2006-09-19
#if 1	// Float method
	for(i=0; i<dstRoiSize.height; i++)
	{
		v = (float) (i/ yFactor);
		n = (int)(v +1.0) - v;
		
		currline = (int)v;
		nextline = (int)(v+1.0);

		// do not expand in X direction even required!!
		// But I think it should be expand
		for(j=0; j< dstRoiSize.width; j++)
		{
			u = (float) (j/ xFactor);

			b = (int)(u +1.0) - u;

			currcol  = (int)u;
			nextcol  = (int)(u+1.0);

			PA= pSrc + currline*srcSize.width + currcol;
			PB= pSrc + currline*srcSize.width + nextcol;
			PC= pSrc + nextline*srcSize.width + currcol;
			PD= pSrc + nextline*srcSize.width + nextcol;


			pDsttmp[j] = (int)(n*b*(*PA) + n*(1-b)*(*PB) + (1-n)*b*(*PC) + (1-n)*(1-b)*(*PD));

		}

		pDsttmp += dstStep;

	}
#else
	// Integer method
	for(i=0; i<dstRoiSize.height; i++)
	{
		v = i* srcSize.height /dstRoiSize.height;
		n = dstRoiSize.height - i * srcSize.height %dstRoiSize.height;
		
		currline = v;
		nextline = v + 1;

		// do not expand in X direction even required!!
		// But I think it should be expand
		for(j=0; j< dstRoiSize.width; j++)
		{
			u = j * srcSize.width / dstRoiSize.width;

			b = dstRoiSize.width - j * srcSize.width % dstRoiSize.width;

			currcol  = u;
			nextcol  = u + 1;

			PA= pSrc + currline*srcSize.width + currcol;
			PB= pSrc + currline*srcSize.width + nextcol;
			PC= pSrc + nextline*srcSize.width + currcol;
			PD= pSrc + nextline*srcSize.width + nextcol;


			pDsttmp[j] = (int)(n*b*(*PA) + n*(1-b)*(*PB) + (1-n)*b*(*PC) + (1-n)*(1-b)*(*PD));

		}

		pDsttmp += dstStep;

	}
#endif


#endif
	return ippStsNoErr;

}

#if defined( _WIN32_WINNT )

int CLZ_win32(int value)
{
    int result;

    _asm 
    {
        bsr eax, value
        mov ebx, value
        mov ecx, 0
        mov edx, 32
        cmp ebx, ecx
        mov ecx, 31
        cmovnz edx, eax
        sub ecx, edx
        mov result, ecx
    }
    
    return result;
}

#endif

int CLZ_c(int value)
{
    int result=0;
   
	if (0 == value)
		return 32;
 
	while( !(value & 0x80000000))
	{
		value<<=1;
		result++;
	}

    return result;
}




