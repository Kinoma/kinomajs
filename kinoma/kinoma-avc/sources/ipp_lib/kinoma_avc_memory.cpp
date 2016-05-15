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

#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#ifdef __KINOMA_IPP__

#include "ipps.h"
#include "ippi.h"
//#include "defines.h"

// Memory allocation function
// Ensure aligned with 16/32 boundary, align =0/16/32
static inline void *K_Allocate(unsigned int size, unsigned char align)
{

	unsigned char *mem_ptr;
	unsigned char *tmp_ptr;

#if 0
	if (!align) 
	{

		// Do not need alignment
		if ((mem_ptr = (unsigned char *) malloc(size + 1)) != NULL) 
		{

			*mem_ptr = 1;

			return ((void *)(mem_ptr+1));
		}
	} 
	else 
#endif
	{
		if ((tmp_ptr = (unsigned char *) malloc(size + align)) != NULL) 
		{

			/* Align the tmp pointer */
			mem_ptr = 	(unsigned char *) ((unsigned int) (tmp_ptr + align - 1) &  (~(unsigned int) (align - 1)));

			if (mem_ptr == tmp_ptr)
				mem_ptr += align;

			*(mem_ptr - 1) = (unsigned char) (mem_ptr - tmp_ptr);

			/* Return the aligned pointer */
			return ((void *)mem_ptr);
		}
	}

	return(NULL);
}


Ipp8u*	__STDCALL ippsMalloc_8u(int len)
{

	Ipp8u  *ptr;

	ptr = (Ipp8u *)K_Allocate(len, 32);

	return ptr;
}

void	__STDCALL ippsFree(void* ptr)
{
	unsigned char * tmp_ptr;

	if (ptr == NULL)
		return;

	tmp_ptr = (unsigned char *)ptr;

	tmp_ptr -= *(tmp_ptr - 1);

	free(tmp_ptr);

}

IppStatus __STDCALL ippsZero_8u(Ipp8u* pDst, int len)
{

	if(!pDst)
		return ippStsNullPtrErr;

	if(len <=0)
		return ippStsSizeErr;

	memset((unsigned char*)pDst, 0, len);
	return ippStsNoErr;
}

IppStatus __STDCALL ippsSet_8u(Ipp8u val, Ipp8u* pDst, int len)
{

	memset(pDst, val, len);

	return ippStsNoErr;

}

IppStatus __STDCALL ippiCopy_8u_C1R
                    ( const Ipp8u* pSrc, int srcStep,
                      Ipp8u* pDst, int dstStep,IppiSize roiSize )
{

	int	i, j, k;
	const Ipp8u *pSrctmp = pSrc;
	Ipp8u *pDsttmp=pDst;

	i = roiSize.width;
	j = roiSize.height;

	for(k=0; k<j; k++)
	{
		memcpy(pDsttmp, pSrctmp, i);
		pSrctmp += srcStep;
		pDsttmp += dstStep;

	}

	return ippStsNoErr;

}


IppStatus __STDCALL ippsZero_32s(Ipp32s* pDst, int len)
{
	if(!pDst)
		return ippStsNullPtrErr;

	if(len <=0)
		return ippStsSizeErr;

	memset((unsigned char*)pDst, 0, len*sizeof(Ipp32s));

	return ippStsNoErr;
}



IppStatus __STDCALL ippsSet_16s( Ipp16s val, Ipp16s* pDst, int len )
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

IppStatus __STDCALL ippsCopy_16s( const Ipp16s* pSrc, Ipp16s* pDst, int len )
{
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	if(len <=0)
		return ippStsSizeErr;

	memcpy(pDst,pSrc,len*sizeof(Ipp16s));

	return ippStsNoErr;
}



IppStatus __STDCALL ippsCopy_32s(const Ipp32s* pSrc, Ipp32s* pDst, int len)
{
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	if(len <=0)
		return ippStsSizeErr;

	memcpy(pDst, pSrc, len*sizeof(Ipp32s));

	return ippStsNoErr;
}

#endif
