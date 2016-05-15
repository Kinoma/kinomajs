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

#include "ipps.h"

/*--------------------------------------------look here firstly---------------/ 
	some memory functions, mainly copied from Wang Wendong
-----------------------------------------------thank you---------------------*/

Ipp8u* __STDCALL 
ippsMalloc_8u_c(int len)
{
	unsigned char *mem_ptr;
	unsigned char *tmp_ptr;
	int align = 32;
	if (!align) 
	{
		/* Do not need alignment*/
		if ((mem_ptr = (unsigned char *) malloc(len + 1)) != NULL) 
		{
			*mem_ptr = 1;
			return ((void *)(mem_ptr+1));
		}
	} 
	else 
	{
		if ((tmp_ptr = (unsigned char *) malloc(len + align)) != NULL) 
		{
			/* Align the tmp pointer */
			mem_ptr = 	(unsigned char *) ((unsigned int) (tmp_ptr + align - 1) & (~(unsigned int) (align - 1)));

			if (mem_ptr == tmp_ptr)
				mem_ptr += align;

			*(mem_ptr - 1) = (unsigned char) (mem_ptr - tmp_ptr);

			/* Return the aligned pointer */
			return ((void *)mem_ptr);
		}
	}

	return(NULL);

}

void __STDCALL 
ippsFree_c(void* ptr)
{
	unsigned char * tmp_ptr;

	if (ptr == NULL)
		return;

	tmp_ptr = (unsigned char *)ptr;

	tmp_ptr -= *(tmp_ptr - 1);

	free(tmp_ptr);
	
}

IppStatus __STDCALL 
ippsZero_8u_c(Ipp8u* pDst, int len)
{
	if(!pDst)
		return ippStsNullPtrErr;

	if(len <=0)
		return ippStsSizeErr;

	memset((unsigned char*)pDst, 0, len);

	return ippStsNoErr;

}

IppStatus __STDCALL 
ippsSet_8u_c(Ipp8u val, Ipp8u* pDst, int len)
{
	if (!pDst)
		return ippStsNullPtrErr;

	if(len <=0)
		return ippStsSizeErr;

	memset(pDst, val, len);

	return ippStsNoErr;

}


IppStatus __STDCALL 
ippsCopy_8u_c(const Ipp8u* pSrc, Ipp8u* pDst, int len)
{
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	if(len <=0)
		return ippStsSizeErr;

	memcpy(pDst, pSrc, len);

	return ippStsNoErr;

}
