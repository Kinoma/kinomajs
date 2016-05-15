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
/* some ipps functions which used in Intel aac and mp3 sample code */

/* 
	NOTE:
	About the rounding method which the functions have "scaleFactor" , I only can say the Intel's IPP does not always 
	follow this rule like 4.5 = 5 and 4.4 = 4. in a strange way, some .5 will be discarded. Till now, I find in functions
	SUM, SUB, SQRT, CONVERT, and MUL

	-----------------------------------------------------------------------------------------------------------------------
	when scaleFactor = 1, after the sum, sub etc. operations and before we use scaling, the temp results if are
	1,    5,     9,   13,   17,   21,  25,......

	when scaleFactor = 2, after the sum, sub etc. operations and before we use scaling, the temp results if are
	2,   10,    18,  26,   34,   42,  50,......

	when scaleFactor = 3, after the sum, sub etc. operations and before we use scaling, the temp results if are
	4,   20,    36,  52,   68,   84, 100,......

	............

	when  scaleFactor = 32, after  the  operations and before we use scaling, the temp results if are
	(2^31)2147483648,         5*(2^31),         9*(2^31),       13*(2^31)......

	when  scaleFactor = 33, after  the  operations and before we use scaling, the temp results if are
	2*(2^31)4294967296,      2*5*(2^31),       2*9*(2^31),     2*13*(2^31)......	

	......................................................

	after this kind of numbers be scaled by the scaleFactor ( DIV by 2^scaleFactor), they are round to 0, discard the 0.5.
	I use ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) to express it, details in code.
	-----------------------------------------------------------------------------------------------------------------------

	except above, other  results' 0.5 will be round to 1.
	
	I don't konw why, it cost me a lot of time to find this,  maybe there is one simple rule to describe this phenomena,
	and I think Intel's code should be some easy way to implement this. If you can summarize more simple rule to implement
	it, please tell me or update them yourself. (email to : shiquancheng@hotmail.com)

*/


#ifdef __KINOMA_IPP__

#include "ipps.h"
#include "kinoma_ipp_common.h"

#ifdef PRINT_REF_INFO
#include "dump.h"
#endif

Ipp8u* __STDCALL  ippsMalloc_8u_c(int len)
{
#if 1
	Ipp8u *mem_ptr;
	Ipp8u *tmp_ptr;
	const int align = 32;

	if ((tmp_ptr = (Ipp8u*) malloc(len + align)) != NULL) 
	{
		/* Align the tmp pointer */
		mem_ptr = 	(Ipp8u*) ((unsigned int) (tmp_ptr + align - 1) & (~(unsigned int) (align - 1)));

		if (mem_ptr == tmp_ptr)
			mem_ptr += align;

		*(mem_ptr - 1) = (Ipp8u) (mem_ptr - tmp_ptr);

		/* Return the aligned pointer */
		return ((void *)mem_ptr);
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippsMalloc_8u_c",-1);
#endif

#if 0
	{
		return (ippsMalloc_8u(len));
	}
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
		mem_ptr = 	(Ipp8u *) ((unsigned int) (tmp_ptr + align - 1) & (~(unsigned int) (align - 1)));

		if (mem_ptr == tmp_ptr)
			mem_ptr += align;

		*(mem_ptr - 1) = (Ipp8u) (mem_ptr - tmp_ptr);

		/* Return the aligned pointer */
		return ((void *)mem_ptr);
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippsMalloc_32s",-1);
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
	dump("ippsFree",-1);
#endif

#if 0
	{
		ippsFree(ptr);
	}
#endif
}


IppStatus __STDCALL ippsCopy_8u_c(const Ipp8u* pSrc, Ipp8u* pDst, int len)
{
#if 1
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;

	if(len <=0)
		return ippStsSizeErr;

	memcpy(pDst, pSrc, len);
#endif

#ifdef PRINT_REF_INFO
	dump("ippsCopy_8u",-1);
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
	dump("ippsCopy_32s",-1);
#endif	

#if 0
	{
		IppStatus sts;
		sts = ippsCopy_32s(pSrc, pDst, len);
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

	if(len <=0)
		return ippStsSizeErr;

	memset((unsigned char*)pDst, 0, len);
#endif

#ifdef PRINT_REF_INFO
	dump("ippsZero_8u_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsZero_8u(pDst, len);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL  ippsZero_16s_c(Ipp16s* pDst, int len)
{
#if 1
	if(!pDst)
		return ippStsNullPtrErr;

	if(len <=0)
		return ippStsSizeErr;

	memset((unsigned char*)pDst, 0, len*sizeof(Ipp16s));
#endif

#ifdef PRINT_REF_INFO
	dump("ippsZero_16s",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsZero_16s(pDst, len);
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
	dump("ippsZero_32s",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsZero_32s(pDst, len);
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

	if(len <=0)
		return ippStsSizeErr;

	memset((unsigned char*)pDst, 0, len*sizeof(Ipp32sc));

#endif

#ifdef PRINT_REF_INFO
	dump("ippsZero_32sc",-1);
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


IppStatus __STDCALL ippsAdd_32s_Sfs_c(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s*
									pDst, int len, int scaleFactor)
{
#if 1
	int i;
	Ipp64s tmp;
	if (!pSrc1 || ! pSrc2 || ! pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;
	
	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (Ipp64s)pSrc1[i]+pSrc2[i];
			pDst[i] = (Ipp32s)CLIP(tmp,MAX_32S,MIN_32S);
		}
	}
	else if (scaleFactor > 0 )
	{
		if (scaleFactor < 32)	/* 1 --> 31 */
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			__INT64 times = (__INT64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc1[i]+pSrc2[i];
				if (tmp != 0)
				{
					sign = 1;
					if (tmp < 0)
					{
						sign = -1; tmp = -tmp;
					}
					if ((((unsigned __INT64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pDst[i] = (Ipp32s)(sign * tmp);
				}
				else
				{
					pDst[i] = 0;
				}
			}
		}
		else	/* right shift 32 bits, every 32bits integer will be 0, but MAX_32S+1 */
		{		
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 32)
					pDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrc1[i]+pSrc2[i];
					if (tmp -1 > MAX_32S)
						pDst[i] = 1;
					else if (-(tmp+1) > MAX_32S)
						pDst[i] = -1;
					else
						pDst[i] = 0;
				}	
			}
		}
	}
	else	
	{
		if (scaleFactor > -32)	/* -1 --> -31 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((Ipp64s)pSrc1[i]+pSrc2[i])<<scale;
				pDst[i] = (Ipp32s)CLIP(tmp,MAX_32S,MIN_32S);
			}
		}
		else					/* -32 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc1[i]+pSrc2[i];
				pDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
			}
		}
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippsAdd_32s_Sfs",-1);
#endif
	
#if 0
	{
		IppStatus sts;
		sts = ippsAdd_32s_Sfs(pSrc1, pSrc2, pDst, len, scaleFactor);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsAdd_32s_ISfs_c(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int scaleFactor)
{
#if 1
	int i;
	Ipp64s tmp;
	if (!pSrc || !pSrcDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;
	
	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (Ipp64s)pSrc[i]+pSrcDst[i];
			pSrcDst[i] = (Ipp32s)CLIP(tmp,MAX_32S,MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 32)	/* 1 --> 31 */
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			__INT64 times = (__INT64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc[i]+pSrcDst[i];
				if (tmp != 0)
				{
					sign = 1;
					if (tmp < 0)
					{
						sign = -1; tmp = -tmp;
					}
					if ((((unsigned __INT64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pSrcDst[i] = (Ipp32s)(sign * tmp);
				}
				else
				{
					pSrcDst[i] = 0;
				}
			}
		}
		else	/* right shift 32 bits, every 32bits integer will be 0, but MAX_32S+1 */
		{		
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 32)
					pSrcDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrc[i]+pSrcDst[i];
					if (tmp -1 > MAX_32S)
						pSrcDst[i] = 1;
					else if (-(tmp+1) > MAX_32S)
						pSrcDst[i] = -1;
					else
						pSrcDst[i] = 0;
				}					
			}
		}
	}
	else	
	{
		if (scaleFactor > -32)	/* -1 --> -31 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((Ipp64s)pSrc[i]+pSrcDst[i])<<scale;
				pSrcDst[i] = (Ipp32s)CLIP(tmp,MAX_32S,MIN_32S);
			}
		}
		else					/* -32 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc[i]+pSrcDst[i];
				pSrcDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
			}
		}
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippsAdd_32s_ISfs",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsAdd_32s_ISfs(pSrc, pSrcDst, len, scaleFactor);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippsMul_32s_ISfs_c(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int scaleFactor)
{
#if 1
	int i;
	__INT64 tmp;

	if (!pSrc || ! pSrcDst )
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (__INT64)pSrc[i]*pSrcDst[i];
			pSrcDst[i] = (Ipp32s)CLIP(tmp, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 64)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			__INT64 times = (__INT64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc[i]*pSrcDst[i];
				if (tmp != 0)
				{
					sign = 1;
					if (tmp < 0)
					{
						sign = -1; tmp = -tmp;
					}
					if ((((unsigned __INT64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pSrcDst[i] = (Ipp32s)CLIP((sign * tmp),MAX_32S, MIN_32S);
				}
				else
				{
					pSrcDst[i] = 0;
				}
			}
		}
		else
		{
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 64)
					pSrcDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrc[i]*pSrcDst[i];
					if (tmp -1 > MAX_64S)
						pSrcDst[i] = 1;
					else if (-(tmp+1) > MAX_64S)
						pSrcDst[i] = -1;
					else
						pSrcDst[i] = 0;
				}	
			}
		}
	}
	else  /*scaleFactor < 0*/
	{
		if (scaleFactor > -32)	/* -1 --> -31 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((__INT64)pSrc[i]*pSrcDst[i]);
				if (tmp>0 &&((MAX_32S >> scale) < tmp))
					pSrcDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
				else if (tmp< 0 &&(((-MIN_32S) >>scale) < (-tmp)))
					pSrcDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
				else
					pSrcDst[i] = (Ipp32s)(tmp << scale);
			}
		}
		else					/* -32 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (__INT64)pSrc[i]*pSrcDst[i];
				pSrcDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
			}
		}
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippsMul_32s_ISfs",-1);
#endif

#if 0
	{ 
		IppStatus sts;
		sts = ippsMul_32s_ISfs(pSrc, pSrcDst, len, scaleFactor);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMul_32s_Sfs_c(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst,int len, int scaleFactor)
{
#if 1	
	int i;
	__INT64 tmp;

	if (!pSrc1 || ! pSrc2 || ! pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (__INT64)pSrc1[i]*pSrc2[i];
			pDst[i] = (Ipp32s)CLIP(tmp, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 64)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			__INT64 times = (__INT64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc1[i]*pSrc2[i];
				if (tmp != 0)
				{
					sign = 1;
					if (tmp < 0)
					{
						sign = -1; tmp = -tmp;
					}
					if ((((unsigned __INT64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pDst[i] = (Ipp32s)CLIP((sign * tmp),MAX_32S, MIN_32S);
				}
				else
				{
					pDst[i] = 0;
				}
			}
		}
		else
		{
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 64)
					pDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrc1[i]*pSrc2[i];
					if (tmp -1 > MAX_64S)
						pDst[i] = 1;
					else if (-(tmp+1) > MAX_64S)
						pDst[i] = -1;
					else
						pDst[i] = 0;
				}	
			}
		}
	}
	else  /*scaleFactor < 0*/
	{
		if (scaleFactor > -32)	/* -1 --> -31 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((__INT64)pSrc1[i]*pSrc2[i]);
				if (tmp>0 &&((MAX_32S >> scale) < tmp))
					pDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
				else if (tmp< 0 &&(((-MIN_32S) >>scale) < (-tmp)))
					pDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
				else
					pDst[i] = (Ipp32s)(tmp << scale);
			}
		}
		else					/* -32 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (__INT64)pSrc1[i]*pSrc2[i];
				pDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
			}
		}
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippsMul_32s_Sfs",-1);
#endif

#if 0		
	{
		IppStatus sts;
		sts = ippsMul_32s_Sfs(pSrc1, pSrc2, pDst, len, scaleFactor);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippsSub_32s_Sfs_c(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s*
									pDst, int len, int scaleFactor)
{
#if 1
	int i;
	Ipp64s tmp;
	if (!pSrc1 || ! pSrc2 || ! pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;
	
	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (Ipp64s)pSrc2[i] - pSrc1[i];
			pDst[i] = (Ipp32s)CLIP(tmp, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 32)	/* 1 --> 31 */
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			__INT64 times = (__INT64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc2[i] - pSrc1[i];
				if (tmp != 0)
				{
					sign = 1;
					if (tmp < 0)
					{
						sign = -1; tmp = -tmp;
					}
					if ((((unsigned __INT64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pDst[i] = (Ipp32s)(sign * tmp);
				}
				else
				{
					pDst[i] = 0;
				}
			}
		}
		else
		{
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 32)
					pDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrc2[i] - pSrc1[i];
					if (tmp -1 > MAX_32S)
						pDst[i] = 1;
					else if (-(tmp+1) > MAX_32S)
						pDst[i] = -1;
					else
						pDst[i] = 0;
				}	
			}
		}
	}
	else	
	{
		if (scaleFactor > -32)	/* -1 --> -31 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((Ipp64s)pSrc2[i] - pSrc1[i])<<scale;
				pDst[i] = (Ipp32s)CLIP(tmp,MAX_32S,MIN_32S);
			}
		}
		else					/* -32 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc2[i] - pSrc1[i];
				pDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
			}
		}
	}

#endif
	
#ifdef PRINT_REF_INFO
	dump("ippsSub_32s_Sfs",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsSub_32s_Sfs(pSrc1, pSrc2,pDst, len, scaleFactor);
		return sts;
	}
#endif

	return ippStsNoErr;
}



IppStatus __STDCALL ippsSortAscend_32s_I_c(Ipp32s* pSrcDst, int len)
{
#if 1
	if (!pSrcDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	quick_sort(pSrcDst,0,len-1);
#endif

#ifdef PRINT_REF_INFO
		dump("ippsSortAscend_32s_I",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsSortAscend_32s_I( pSrcDst,len);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMinMax_32s_c(const Ipp32s* pSrc, int len, Ipp32s* pMin, Ipp32s* pMax)
{
#if 1
	int i;
	register Ipp32s tmpMin, tmpMax;
	if (!pSrc || !pMin)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	tmpMin = tmpMax = pSrc[0];
	for (i = 1; i < len; i++)
	{
		if (pSrc[i] > tmpMax) 
		{
			tmpMax = pSrc[i];
		}
		else if (pSrc[i] < tmpMin)
		{
			tmpMin = pSrc[i];
		}
	}

	*pMin = tmpMin;
	*pMax = tmpMax;
#endif

#ifdef PRINT_REF_INFO
	dump("ippsMinMax_32s",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMinMax_32s(pSrc, len, pMin, pMax);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMax_32s_c(const Ipp32s* pSrc, int len, Ipp32s* pMax)
{
#if 1
	int i;
	Ipp32s tmp;
	if (!pSrc || !pMax)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	tmp = pSrc[0];
	for (i = 1; i < len; i++)
	{
		if (tmp < pSrc[i])
			tmp = pSrc[i];
	}
	*pMax = tmp;
#endif

#ifdef PRINT_REF_INFO
	dump("ippsMax_32s",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMax_32s(pSrc, len, pMax);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMax_16s_c(const Ipp16s* pSrc, int len, Ipp16s* pMax)
{
#if 1
	int i;
	Ipp16s tmp;
	if (!pSrc || !pMax)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	tmp = pSrc[0];
	for (i = 1; i < len; i++)
	{
		if (tmp < pSrc[i])
			tmp = pSrc[i];
	}
	*pMax = tmp;
#endif

#ifdef PRINT_REF_INFO
	dump("ippsMax_16s",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMax_16s(pSrc, len, pMax);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMin_32s_c(const Ipp32s* pSrc, int len, Ipp32s* pMin)
{
#if 1
	int i;
	Ipp32s tmp;
	if (!pSrc || !pMin)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	tmp = pSrc[0];
	for (i = 1; i < len; i++)
	{
		if (tmp > pSrc[i])
			tmp = pSrc[i];
	}
	*pMin = tmp;
#endif

#ifdef PRINT_REF_INFO
	dump("ippsMin_32s",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMin_32s(pSrc, len, pMin);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMaxAbs_32s_c(const Ipp32s* pSrc, int len, Ipp32s* pMaxAbs)
{
#if 1
	int i;
	Ipp16s tmp;
	if (!pSrc || !pMaxAbs)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	tmp = abs(pSrc[0]);
	for (i = 1; i < len; i++)
	{
		if (tmp < abs(pSrc[i]))
			tmp = abs(pSrc[i]);
	}
	*pMaxAbs = tmp;
#endif

#ifdef PRINT_REF_INFO
	dump("ippsMaxAbs_32s",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMaxAbs_32s(pSrc, len, pMaxAbs);
		return sts;
	}
#endif

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
	dump("ippsMove_32s",-1);
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

IppStatus __STDCALL ippsDiv_32s_ISfs_c(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int ScaleFactor)
{
#if 1
	int i;
	__INT64 tmp;
	__INT64 div, base;
	int isMinus;
	Ipp64s rounding = ((Ipp64s)1<<(ScaleFactor-1));
	IppStatus sts = ippStsNoErr;

	if (!pSrc || !pSrcDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (ScaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			if(isDIVbyZERO(&isMinus, &base, &div, pSrc[i],&pSrcDst[i],&sts))
				continue;

			tmp = (div + (base>>1)) / base;
			if (isMinus)
				pSrcDst[i] = (Ipp32s)-tmp;
			else
				pSrcDst[i] = (Ipp32s)tmp;
		}

	}
	else if (ScaleFactor > 0)
	{
		if (ScaleFactor < 32)
		{
			for (i = 0; i < len; i++)
			{
				if(isDIVbyZERO(&isMinus, &base, &div, pSrc[i],&pSrcDst[i],&sts))
					continue;

				tmp = ((div ) / base + rounding) >> ScaleFactor;
				if (isMinus)
					pSrcDst[i] = (Ipp32s)-tmp;
				else
					pSrcDst[i] = (Ipp32s)tmp;
			}
		}
		else
		{
			for (i = 0; i < len; i++)
			{
				if(isDIVbyZERO(&isMinus, &base, &div, pSrc[i],&pSrcDst[i],&sts))
					continue;
				pSrcDst[i] = 0;
			}
		}
	}
	else 
	{
		__INT64 scale = ((__INT64)1 << (-ScaleFactor));
		for (i = 0; i < len; i++)
		{
			if(isDIVbyZERO(&isMinus, &base, &div, pSrc[i],&pSrcDst[i],&sts))
				continue;

			tmp = (__INT64)(((long double)div  / base) * scale + 0.5f);
			if (isMinus)
				tmp = -tmp;
			pSrcDst[i] = (Ipp32s)CLIP(tmp, MAX_32S, MIN_32S);

		}
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippsDiv_32s_ISfs",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsDiv_32s_ISfs(pSrc, pSrcDst, len, ScaleFactor);
		return sts;
	}
#endif

	return sts;
}



IppStatus __STDCALL ippsSqrt_64s_ISfs_c(Ipp64s* pSrcDst, int len, int scaleFactor)
{
#if 1
	int i;
	Ipp64s   remainder;
	unsigned __INT64 root;

	if (!pSrcDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			qk_sqrt( pSrcDst[i], &remainder, &root);
			if (remainder > root)
			{
				pSrcDst[i] = root+1;
			}
			else
			{
				pSrcDst[i] = root;
			}
		}
	}
	else if (scaleFactor > 0)
	{
		Ipp64s half = (__INT64)1<<(scaleFactor-1);
		int scale = (scaleFactor-1);
		__INT64 times = (__INT64)1<<scale;
		int shift = 64-scaleFactor;
		Ipp64s tmp;
		for (i = 0; i < len; i++)
		{
			tmp = pSrcDst[i];
			qk_sqrt(tmp, &remainder, &root);

			if ( (remainder==0) && (((root<<shift)>>shift ==times) && ((((root>>scale)-1)&0x3)==0)) )
				pSrcDst[i] = root >> scaleFactor;
			else
				pSrcDst[i] = (root+half) >> scaleFactor;
		}
	}
	else	// when scaleFactor < 0, it should be the floating root * pow(2, -scaleFactor), 
	{		// I didn't find a way to calc this part without float part of the sqrt root, so I use double here
			// And this Intel sample doesn't use this part. I tested this part in my sample code
		Ipp64s scale = (Ipp64s)1<<(-scaleFactor);
		double froot;
		for (i = 0; i < len; i++)
		{
			qk_sqrt( pSrcDst[i], &remainder, &root);
			froot = root + (double)float_part(remainder,root);
			if ((MAX_64S >> (-scaleFactor)) <= root)
				pSrcDst[i] = froot > 0 ? MAX_64S: 0;
			else
				pSrcDst[i] = (Ipp64s)(froot *  scale + 0.5f);
		}
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippsSqrt_64s_ISfs",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsSqrt_64s_ISfs(pSrcDst, len, scaleFactor);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsConvert_64s32s_Sfs_c(const Ipp64s* pSrc, Ipp32s* pDst, int len,
										  IppRoundMode rndMode, int scaleFactor)
{
#if 1
	int i;
	__INT64 tmp;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			pDst[i] = (Ipp32s)CLIP(pSrc[i],MAX_32S,MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (rndMode == ippRndZero)
		{
			for (i = 0; i < len; i++)
			{
				tmp = pSrc[i] >> scaleFactor;
				pDst[i] = (Ipp32s)CLIP(tmp,MAX_32S,MIN_32S);
			}
		}
		else
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			__INT64 times = (__INT64)1<<scale;
			int shift = 64-scaleFactor;

			for (i = 0; i < len; i++)
			{
				tmp = pSrc[i];
				if (tmp != 0)
				{
					sign = 1;
					if (tmp < 0)
					{
						sign = -1; tmp = -tmp;
					}
					if ((((unsigned __INT64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pDst[i] = (Ipp32s)CLIP((sign * tmp), MAX_32S, MIN_32S);
				}
				else
				{
					pDst[i] = 0;
				}
			}
		}
	}
	else if (scaleFactor < 0)
	{
		int scale = -scaleFactor;
		for (i = 0; i < len; i++)
		{
			tmp = pSrc[i];
			if ( (tmp > 0 && (MAX_32S>>scale) >= tmp)  || (tmp < 0 && (MIN_32S >> scale <= tmp)) )
				pDst[i] = (Ipp32s)(tmp << scale);
			else
				pDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
		}
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippsConvert_64s32s_Sfs",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsConvert_64s32s_Sfs(pSrc, pDst, len, rndMode, scaleFactor);
		return sts;
	}
#endif

	return ippStsNoErr;

}

IppStatus __STDCALL ippsConvert_32s16s_Sfs_c(const Ipp32s* pSrc, Ipp16s* pDst, int len, int scaleFactor)
{
#if 1
	int i;
	int tmp;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			pDst[i] = (Ipp16s)CLIP(pSrc[i],MAX_16S, MIN_16S);
		}
	}
	else if (scaleFactor > 0)
	{
		int sign;
		__INT64 rounding = ((__INT64)1<<(scaleFactor-1));
		int scale = (scaleFactor -1);
		__INT64 times = (__INT64)1<<scale;
		int shift = 64-scaleFactor;

		for (i = 0; i < len; i++)
		{
			tmp = pSrc[i];
			if (tmp != 0)
			{
				sign = 1;
				if (tmp < 0)
				{
					sign = -1; tmp = -tmp;
				}
				if ((((unsigned __INT64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
					tmp = (tmp)>>scaleFactor;
				else
					tmp = (tmp+rounding)>>scaleFactor;
				pDst[i] = (Ipp16s)CLIP((sign * tmp),MAX_16S, MIN_16S);
			}
			else
			{
				pDst[i] = 0;
			}
		}
	}
	else if (scaleFactor < 0)
	{
		int scale = -scaleFactor;
		for (i = 0; i < len; i++)
		{
			tmp = pSrc[i];
			if ( (tmp > 0 && (MAX_16S>>scale) >= tmp)  || (tmp < 0 && (MIN_16S >> scale <= tmp)) )
				pDst[i] = (Ipp16s)(tmp << scale);
			else
				pDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
		}
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippsConvert_32s16s_Sfs",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsConvert_32s16s_Sfs(pSrc, pDst,  len, scaleFactor);
		return sts;
	}
#endif
	return ippStsNoErr;
}

IppStatus  __STDCALL ippsLShiftC_32s_I_c(int val, Ipp32s* pSrcDst, int len)
{
#if 1
	int i;
	if (!pSrcDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (val > 0)
	{
		for (i = 0; i < len; i++)
		{
			pSrcDst[i] = pSrcDst[i] << val;
		}
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippsLShiftC_32s_I",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsLShiftC_32s_I(val, pSrcDst, len);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsRShiftC_32s_I_c(int val, Ipp32s* pSrcDst, int len)
{
#if 1
	int i;
	if (!pSrcDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (val > 0)
	{
		for (i = 0; i < len; i++)
		{
			pSrcDst[i] = pSrcDst[i] >> val;
		}
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippsRShiftC_32s_I",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsRShiftC_32s_I(val, pSrcDst, len);
		return sts;
	}
#endif

	return ippStsNoErr;
}





/* the following 4 functions used in MDCT*/
IppStatus __STDCALL ippsFFTInitAlloc_C_32sc_c(IppsFFTSpec_C_32sc** ppFFTSpec, int order,
											int flag, IppHintAlgorithm hint)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32sc *pkSpec;
	int i, halfsize;	
	double theta;
	
	if (!ppFFTSpec)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;
	
	pkSpec = (k_IppsFFTSpec_C_32sc*)ippsMalloc_8u_c(sizeof(k_IppsFFTSpec_C_32sc));
	if (!pkSpec)
		return ippStsMemAllocErr;

	pkSpec->sig = COMPLEX_SIG;							/*means complex fft*/
	pkSpec->order = order;
	pkSpec->hint = hint;
	pkSpec->sizeWorkBuf = (1<<(order+2)) * 8 + 16;		/*work buf size which alloc memory in MDCT*/
	pkSpec->alloc = 1;									/*means this is InitAlloc func, not Init*/
	pkSpec->len = 1 << order;							/* 2^order will be the input sample length */

	if (flag == IPP_FFT_NODIV_BY_ANY)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 0;
		pkSpec->normFwd = pkSpec->normInv = 0;
	}
	else if (flag == IPP_FFT_DIV_BY_SQRTN)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 1;
		pkSpec->normFwd = pkSpec->normInv = sqrt(pkSpec->len);	
	}
	else if (flag == IPP_FFT_DIV_INV_BY_N)
	{
		pkSpec->flagFwd = 0;
		pkSpec->normFwd = 0;
		pkSpec->flagInv = 1;
		pkSpec->normInv = pkSpec->len;
	}
	else
	{
		pkSpec->flagFwd = 1;
		pkSpec->normFwd = pkSpec->len;
		pkSpec->flagInv = 0;
		pkSpec->normInv = 0;
	}

	/*sin and cos table*/
	halfsize = pkSpec->len >> 1;
	pkSpec->cos = (double*)ippsMalloc_8u_c(sizeof(double)*halfsize);
	pkSpec->sin = (double*)ippsMalloc_8u_c(sizeof(double)*halfsize);
	for(i = 0; i < halfsize; i++)
	{
		theta = K_2PI * i / (double)pkSpec->len;
		pkSpec->cos[i] = cos(theta);
		pkSpec->sin[i] = -sin(theta);
	}

	/*bit reverse table*/
	//pkSpec->sizeReverseTbl = sizeof(int)*pkSpec->len;
	pkSpec->ReverseTbl = (int*)ippsMalloc_8u_c(sizeof(int)*pkSpec->len);
	for (i = 0; i < pkSpec->len; i++)
	{
		int reversed = 0;
		int b0;
		int tmp = i;

		for (b0 = 0; b0 < pkSpec->order; b0++)
		{
			reversed = (reversed << 1) | (tmp & 1);
			tmp >>= 1;
		}
		pkSpec->ReverseTbl[i] = reversed;
	}	

	*ppFFTSpec = (IppsFFTSpec_C_32sc*)pkSpec;

#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTInitAlloc_C_32sc_c",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTInitAlloc_C_32sc(ppFFTSpec,  order, flag,  hint);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTGetBufSize_C_32sc_c(const IppsFFTSpec_C_32sc* pFFTSpec, int* pSize)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32sc *pkSpec = (k_IppsFFTSpec_C_32sc*)pFFTSpec;

	if (!pFFTSpec)
		return ippStsNullPtrErr;

	if (pkSpec->sig != COMPLEX_SIG)
	{
		*pSize = 0;
		return ippStsContextMatchErr;	
	}

	*pSize = ((k_IppsFFTSpec_C_32sc*)pFFTSpec)->sizeWorkBuf + 16;
#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTGetBufSize_C_32sc_c",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTGetBufSize_C_32sc(pFFTSpec, pSize);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippsFFTFree_C_32sc_c(IppsFFTSpec_C_32sc* pFFTSpec)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32sc *pkSpec = (k_IppsFFTSpec_C_32sc*)pFFTSpec;
	
	if (!pFFTSpec)
		return ippStsNullPtrErr;

	if (pkSpec->sig != COMPLEX_SIG)		
		return ippStsContextMatchErr;	

	if (pkSpec->alloc)
	{
		ippsFree_c(pkSpec->sin);
		ippsFree_c(pkSpec->cos);
		ippsFree_c(pkSpec->ReverseTbl);
		ippsFree_c(pkSpec);
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTFree_C_32sc_c",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTFree_C_32sc(pFFTSpec);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTFwd_CToC_32sc_Sfs_c(const Ipp32sc* pSrc, Ipp32sc* pDst, const
											 IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32sc * pkSpec = (k_IppsFFTSpec_C_32sc*)pFFTSpec;

	int i,j;
	double rounding;

	int step, shift, pos;
	int size, exp, estep;

	long double *xr, *xi, tmp;

	if (!pkSpec || !pSrc || !pDst || !pBuffer)
		return ippStsNullPtrErr;

	if (pkSpec->sig != COMPLEX_SIG)		
		return ippStsContextMatchErr;	

	xr = (long double*)pBuffer;
	xi = (long double*)(pBuffer + pkSpec->len * sizeof(long double));
	for (i = 0; i < pkSpec->len; i++)
	{
		xr[i] = pSrc[i].re;
		xi[i] = pSrc[i].im;
	}

	/*bit reverse*/
	for (i = 0; i < pkSpec->len; i++)
	{
		j = pkSpec->ReverseTbl[i];
		if (j <= i)
			continue;

		tmp = xr[i];
		xr[i] = xr[j];
		xr[j] = tmp;

		tmp = xi[i];
		xi[i] = xi[j];
		xi[j] = tmp;	
	}
	/*reverse end*/

	/*fwd transform*/
	estep = size = pkSpec->len;
	for (step = 1; step < size; step *= 2)
	{
		int x1;
		int x2 = 0;
		estep >>= 1;
		for (pos = 0; pos < size; pos += (2 * step))
		{
			x1 = x2;
			x2 += step;
			exp = 0;
			for (shift = 0; shift < step; shift++)
			{
				long double v2r, v2i;

				v2r = xr[x2] * pkSpec->cos[exp] - xi[x2] * pkSpec->sin[exp];
				v2i = xr[x2] * pkSpec->sin[exp] + xi[x2] * pkSpec->cos[exp];

				xr[x2] = xr[x1] - v2r;
				xr[x1] += v2r;

				xi[x2] = xi[x1] - v2i;

				xi[x1] += v2i;

				exp += estep;

				x1++;
				x2++;
			}
		}
	}
	/*transform end*/

	/*normlizing & scaling*/
	if (scaleFactor)
		rounding = (1<< (scaleFactor -1 ))+0.5;
	else
		rounding = 0.5;
	if (pkSpec->flagFwd)
	{
		for (i = 0; i < pkSpec->len; i++)
		{
			if (xr[i] < 0)
				pDst[i].re = (Ipp32s)(((__INT64)((xr[i]/pkSpec->normFwd) - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp32s)(((__INT64)((xr[i]/pkSpec->normFwd) + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp32s)(((__INT64)((xi[i]/pkSpec->normFwd) - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp32s)(((__INT64)((xi[i]/pkSpec->normFwd) + rounding)) >> scaleFactor);
		}
	}
	else
	{
		for (i = 0; i < pkSpec->len; i++)
		{
			if (xr[i] < 0)
				pDst[i].re = (Ipp32s)(((__INT64)(xr[i] - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp32s)(((__INT64)(xr[i] + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp32s)(((__INT64)(xi[i] - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp32s)(((__INT64)(xi[i] + rounding)) >> scaleFactor);
		}
	}
	
#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTFwd_CToC_32sc_Sfs",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTFwd_CToC_32sc_Sfs(pSrc, pDst,pFFTSpec, scaleFactor, pBuffer);
		return sts;
	}
#endif

	return ippStsNoErr;
}


/* the following 6 functions exist in sbr code */
IppStatus __STDCALL ippsFFTGetSize_C_32sc_c(int order, int flag, IppHintAlgorithm hint,
								int* pSizeSpec, int* pSizeInit, int* pSizeBuf)
{
#ifdef KINOMA_FFT

	int len;

	if (!pSizeSpec || !pSizeInit || !pSizeBuf)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;

	*pSizeInit = 0;
	*pSizeBuf = (1<<(order+2)) * 8 + 32;		
	len = 1 << order;
	*pSizeSpec = sizeof(k_IppsFFTSpec_C_32sc) + sizeof(double)*len + sizeof(int)*len;
	
#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTGetSize_C_32sc",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTGetSize_C_32sc(order, flag, hint,pSizeSpec, pSizeInit,pSizeBuf);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTInit_C_32sc_c(IppsFFTSpec_C_32sc** ppFFTSpec, int order, int
									   flag, IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32sc *pkSpec;
	int i, halfsize;	
	double theta;

	if (!ppFFTSpec || !pMemSpec)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;

	pkSpec = (k_IppsFFTSpec_C_32sc*) pMemSpec;

	pkSpec->sig = COMPLEX_SIG;							/*means complex fft*/
	pkSpec->order = order;
	pkSpec->hint = hint;
	pkSpec->sizeWorkBuf = (1<<(order+2)) * 8 + 32;		/*work buf size which alloc memory in MDCT*/
	pkSpec->alloc = 0;									/* 0 means this is Init alloc memory*/
	pkSpec->len = 1 << order;							/* 2^order will be the input sample length */

	if (flag == IPP_FFT_NODIV_BY_ANY)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 0;
		pkSpec->normFwd = pkSpec->normInv = 0;
	}
	else if (flag == IPP_FFT_DIV_BY_SQRTN)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 1;
		pkSpec->normFwd = pkSpec->normInv = sqrt(pkSpec->len);	
	}
	else if (flag == IPP_FFT_DIV_INV_BY_N)
	{
		pkSpec->flagFwd = 0;
		pkSpec->normFwd = 0;
		pkSpec->flagInv = 1;
		pkSpec->normInv = pkSpec->len;
	}
	else
	{
		pkSpec->flagFwd = 1;
		pkSpec->normFwd = pkSpec->len;
		pkSpec->flagInv = 0;
		pkSpec->normInv = 0;
	}

	/*sin and cos table*/
	halfsize = pkSpec->len >> 1;
	pkSpec->cos = (double*)(pMemSpec + sizeof(k_IppsFFTSpec_C_32sc)); 
	pkSpec->sin = (double*)(pMemSpec + sizeof(k_IppsFFTSpec_C_32sc) + sizeof(double)*halfsize);
	for(i = 0; i < halfsize; i++)
	{
		theta = K_2PI * i / (double)pkSpec->len;
		pkSpec->cos[i] = cos(theta);
		pkSpec->sin[i] = -sin(theta);
	}

	/*bit reverse table*/
	pkSpec->ReverseTbl = (int*)(pMemSpec + sizeof(k_IppsFFTSpec_C_32sc)+ sizeof(double)*pkSpec->len);
	for (i = 0; i < pkSpec->len; i++)
	{
		int reversed = 0;
		int b0;
		int tmp = i;

		for (b0 = 0; b0 < pkSpec->order; b0++)
		{
			reversed = (reversed << 1) | (tmp & 1);
			tmp >>= 1;
		}
		pkSpec->ReverseTbl[i] = reversed;
	}	

	*ppFFTSpec = (IppsFFTSpec_C_32sc*) pkSpec;
#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTInit_C_32sc",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTInit_C_32sc(ppFFTSpec, order, flag, hint, pMemSpec, pBufInit);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTInv_CToC_32sc_Sfs_c(const Ipp32sc* pSrc, Ipp32sc* pDst, const
											IppsFFTSpec_C_32sc* pFFTSpec, int scaleFactor, Ipp8u* pBuffer)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32sc * pkSpec = (k_IppsFFTSpec_C_32sc*)pFFTSpec;

	int i,j;
	double rounding;

	int step, shift, pos;
	int size, exp, estep;

	long double *xr, *xi, tmp;

	if (!pkSpec || !pSrc || !pDst || !pBuffer)
		return ippStsNullPtrErr;

	if (pkSpec->sig != COMPLEX_SIG)		
		return ippStsContextMatchErr;	

	xr = (long double*)pBuffer;
	xi = (long double*)(pBuffer + pkSpec->len * sizeof(long double));
	for (i = 0; i < pkSpec->len; i++)
	{
		xr[i] = pSrc[i].re;
		xi[i] = -pSrc[i].im;
	}

	/*bit reverse*/
	for (i = 0; i < pkSpec->len; i++)
	{
		j = pkSpec->ReverseTbl[i];
		if (j <= i)
			continue;

		tmp = xr[i];
		xr[i] = xr[j];
		xr[j] = tmp;

		tmp = xi[i];
		xi[i] = xi[j];
		xi[j] = tmp;	
	}
	/*reverse end*/

	/*Inv transform*/
	estep = size = pkSpec->len;
	for (step = 1; step < size; step *= 2)
	{
		int x1;
		int x2 = 0;
		estep >>= 1;
		for (pos = 0; pos < size; pos += (2 * step))
		{
			x1 = x2;
			x2 += step;
			exp = 0;
			for (shift = 0; shift < step; shift++)
			{
				long double v2r, v2i;

				v2r = xr[x2] * pkSpec->cos[exp] - xi[x2] * pkSpec->sin[exp];
				v2i = xr[x2] * pkSpec->sin[exp] + xi[x2] * pkSpec->cos[exp];

				xr[x2] = xr[x1] - v2r;
				xr[x1] += v2r;

				xi[x2] = xi[x1] - v2i;

				xi[x1] += v2i;

				exp += estep;

				x1++;
				x2++;
			}
		}
	}
	/*transform end*/

	/*normlizing & scaling*/
	for (i = 0; i < pkSpec->len; i++)
	{
		xi[i] = -xi[i];
	}
	if (scaleFactor)
		rounding = (1<< (scaleFactor -1 ))+0.5;
	else
		rounding = 0.5;
	if (pkSpec->flagInv)
	{
		for (i = 0; i < pkSpec->len; i++)
		{
			if (xr[i] < 0)
				pDst[i].re = (Ipp32s)(((__INT64)((xr[i]/pkSpec->normInv) - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp32s)(((__INT64)((xr[i]/pkSpec->normInv) + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp32s)(((__INT64)((xi[i]/pkSpec->normInv) - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp32s)(((__INT64)((xi[i]/pkSpec->normInv) + rounding)) >> scaleFactor);
		}
	}
	else
	{
		for (i = 0; i < pkSpec->len; i++)
		{
			if (xr[i] < 0)
				pDst[i].re = (Ipp32s)(((__INT64)(xr[i] - rounding)) >> scaleFactor);
			else
				pDst[i].re = (Ipp32s)(((__INT64)(xr[i] + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDst[i].im = (Ipp32s)(((__INT64)(xi[i] - rounding)) >> scaleFactor);
			else
				pDst[i].im = (Ipp32s)(((__INT64)(xi[i] + rounding)) >> scaleFactor);
		}
	}
	
#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTInv_CToC_32sc_Sfs",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTInv_CToC_32sc_Sfs(pSrc,pDst, pFFTSpec, scaleFactor, pBuffer);
		return sts;
	}
#endif

	return ippStsNoErr;
}



IppStatus __STDCALL ippsFFTGetSize_C_32s_c(int order, int flag, IppHintAlgorithm hint, int*
										 pSizeSpec, int* pSizeInit, int* pSizeBuf)
{
#ifdef KINOMA_FFT
	int len;

	if (!pSizeSpec || !pSizeInit || !pSizeBuf)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;

	*pSizeInit = 0;
	*pSizeBuf = (1<<(order+2)) * 8 + 32;		
	len = 1 << order;
	*pSizeSpec = sizeof(k_IppsFFTSpec_C_32s) + sizeof(double)*len + sizeof(int)*len;

#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTGetSize_C_32s",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTGetSize_C_32s(order, flag, hint, pSizeSpec, pSizeInit, pSizeBuf);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTInit_C_32s_c(IppsFFTSpec_C_32s** ppFFTSpec, int order, int flag,
							IppHintAlgorithm hint, Ipp8u* pMemSpec, Ipp8u* pBufInit)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32s *pkSpec;
	int i, halfsize;	
	double theta;

	if (!ppFFTSpec || !pMemSpec)
		return ippStsNullPtrErr;
	if (order < 0)
		return ippStsFftOrderErr;
	if (flag!=IPP_FFT_DIV_FWD_BY_N && flag!=IPP_FFT_DIV_INV_BY_N && flag!=IPP_FFT_DIV_BY_SQRTN && flag!=IPP_FFT_NODIV_BY_ANY)
		return ippStsFftFlagErr;

	pkSpec = (k_IppsFFTSpec_C_32s*) pMemSpec;

	pkSpec->sig = REAL_SIG;							/*means complex fft*/
	pkSpec->order = order;
	pkSpec->hint = hint;
	pkSpec->sizeWorkBuf = (1<<(order+2)) * 8 + 32;		/*work buf size which alloc memory in MDCT*/
	pkSpec->alloc = 0;									/* 0 means this is Init alloc memory*/
	pkSpec->len = 1 << order;							/* 2^order will be the input sample length */

	if (flag == IPP_FFT_NODIV_BY_ANY)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 0;
		pkSpec->normFwd = pkSpec->normInv = 0;
	}
	else if (flag == IPP_FFT_DIV_BY_SQRTN)
	{
		pkSpec->flagFwd = pkSpec->flagInv = 1;
		pkSpec->normFwd = pkSpec->normInv = sqrt(pkSpec->len);	
	}
	else if (flag == IPP_FFT_DIV_INV_BY_N)
	{
		pkSpec->flagFwd = 0;
		pkSpec->normFwd = 0;
		pkSpec->flagInv = 1;
		pkSpec->normInv = pkSpec->len;
	}
	else
	{
		pkSpec->flagFwd = 1;
		pkSpec->normFwd = pkSpec->len;
		pkSpec->flagInv = 0;
		pkSpec->normInv = 0;
	}

	/*sin and cos table*/
	halfsize = pkSpec->len >> 1;
	pkSpec->cos = (double*)(pMemSpec + sizeof(k_IppsFFTSpec_C_32s)); 
	pkSpec->sin = (double*)(pMemSpec + sizeof(k_IppsFFTSpec_C_32s) + sizeof(double)*halfsize);
	for(i = 0; i < halfsize; i++)
	{
		theta = K_2PI * i / (double)pkSpec->len;
		pkSpec->cos[i] = cos(theta);
		pkSpec->sin[i] = -sin(theta);
	}

	/*bit reverse table*/
	pkSpec->ReverseTbl = (int*)(pMemSpec + sizeof(k_IppsFFTSpec_C_32s)+ sizeof(double)*pkSpec->len);
	for (i = 0; i < pkSpec->len; i++)
	{
		int reversed = 0;
		int b0;
		int tmp = i;

		for (b0 = 0; b0 < pkSpec->order; b0++)
		{
			reversed = (reversed << 1) | (tmp & 1);
			tmp >>= 1;
		}
		pkSpec->ReverseTbl[i] = reversed;
	}	

	*ppFFTSpec = (IppsFFTSpec_C_32s*) pkSpec;

#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTInit_C_32s",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTInit_C_32s(ppFFTSpec, order, flag, hint, pMemSpec,pBufInit);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsFFTInv_CToC_32s_Sfs_c(const Ipp32s* pSrcRe, const Ipp32s* pSrcIm,
											Ipp32s* pDstRe, Ipp32s* pDstIm, const IppsFFTSpec_C_32s* pFFTSpec, int
											scaleFactor, Ipp8u* pBuffer)
{
#ifdef KINOMA_FFT
	k_IppsFFTSpec_C_32s * pkSpec = (k_IppsFFTSpec_C_32s*)pFFTSpec;

	int i,j;
	double rounding;

	int step, shift, pos;
	int size, exp, estep;

	long double *xr, *xi, tmp;

	if (!pSrcRe || !pSrcIm || !pDstRe || !pDstIm || !pFFTSpec || !pBuffer)
		return ippStsNullPtrErr;

	if (pkSpec->sig != REAL_SIG)		
		return ippStsContextMatchErr;	

	xr = (long double*)pBuffer;
	xi = (long double*)(pBuffer + pkSpec->len * sizeof(long double));
	for (i = 0; i < pkSpec->len; i++)
	{
		xr[i] = pSrcRe[i];
		xi[i] = -pSrcIm[i];
	}

	/*bit reverse*/
	for (i = 0; i < pkSpec->len; i++)
	{
		j = pkSpec->ReverseTbl[i];
		if (j <= i)
			continue;

		tmp = xr[i];
		xr[i] = xr[j];
		xr[j] = tmp;

		tmp = xi[i];
		xi[i] = xi[j];
		xi[j] = tmp;	
	}
	/*reverse end*/

	/*Inv transform*/
	estep = size = pkSpec->len;
	for (step = 1; step < size; step *= 2)
	{
		int x1;
		int x2 = 0;
		estep >>= 1;
		for (pos = 0; pos < size; pos += (2 * step))
		{
			x1 = x2;
			x2 += step;
			exp = 0;
			for (shift = 0; shift < step; shift++)
			{
				long double v2r, v2i;

				v2r = xr[x2] * pkSpec->cos[exp] - xi[x2] * pkSpec->sin[exp];
				v2i = xr[x2] * pkSpec->sin[exp] + xi[x2] * pkSpec->cos[exp];

				xr[x2] = xr[x1] - v2r;
				xr[x1] += v2r;

				xi[x2] = xi[x1] - v2i;

				xi[x1] += v2i;

				exp += estep;

				x1++;
				x2++;
			}
		}
	}
	/*transform end*/

	/*normlizing & scaling*/
	for (i = 0; i < pkSpec->len; i++)
	{
		xi[i] = -xi[i];
	}
	if (scaleFactor)
		rounding = (1<< (scaleFactor -1 ))+0.5;
	else
		rounding = 0.5;
	if (pkSpec->flagInv)
	{
		for (i = 0; i < pkSpec->len; i++)
		{
			if (xr[i] < 0)
				pDstRe[i] = (Ipp32s)(((__INT64)((xr[i]/pkSpec->normInv) - rounding)) >> scaleFactor);
			else
				pDstRe[i] = (Ipp32s)(((__INT64)((xr[i]/pkSpec->normInv) + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDstIm[i] = (Ipp32s)(((__INT64)((xi[i]/pkSpec->normInv) - rounding)) >> scaleFactor);
			else
				pDstIm[i] = (Ipp32s)(((__INT64)((xi[i]/pkSpec->normInv) + rounding)) >> scaleFactor);
		}
	}
	else
	{
		for (i = 0; i < pkSpec->len; i++)
		{
			if (xr[i] < 0)
				pDstRe[i] = (Ipp32s)(((__INT64)(xr[i] - rounding)) >> scaleFactor);
			else
				pDstRe[i] = (Ipp32s)(((__INT64)(xr[i] + rounding)) >> scaleFactor);

			if (xi[i] < 0)
				pDstIm[i] = (Ipp32s)(((__INT64)(xi[i] - rounding)) >> scaleFactor);
			else
				pDstIm[i] = (Ipp32s)(((__INT64)(xi[i] + rounding)) >> scaleFactor);
		}
	}
	
#endif

#ifdef PRINT_REF_INFO
	dump("ippsFFTInv_CToC_32s_Sfs",-1);
#endif

#ifdef INTEL_FFT
	{
		IppStatus sts;
		sts = ippsFFTInv_CToC_32s_Sfs(pSrcRe, pSrcIm, pDstRe, pDstIm, pFFTSpec, scaleFactor, pBuffer);
		return sts;
	}
#endif

	return ippStsNoErr;
}




/*following functions are for 5.1  aac*/
IppStatus __STDCALL ippsMul_32sc_Sfs_c(const Ipp32sc* pSrc1, const Ipp32sc* pSrc2,
       Ipp32sc* pDst, int len, int scaleFactor)
{
#if 1
	int i;
	__INT64 tmpRe, tmpIm;

	if (!pSrc1 || ! pSrc2 || ! pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmpRe = (__INT64)pSrc1[i].re*pSrc2[i].re - (__INT64)pSrc1[i].im * pSrc2[i].im;
			tmpIm = (__INT64)pSrc1[i].re*pSrc2[i].im + (__INT64)pSrc1[i].im * pSrc2[i].re;
			pDst[i].re = (Ipp32s)CLIP(tmpRe, MAX_32S, MIN_32S);
			pDst[i].im = (Ipp32s)CLIP(tmpIm, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 64)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			__INT64 times = (__INT64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmpRe = (__INT64)pSrc1[i].re*pSrc2[i].re - (__INT64)pSrc1[i].im * pSrc2[i].im;
				tmpIm = (__INT64)pSrc1[i].re*pSrc2[i].im + (__INT64)pSrc1[i].im * pSrc2[i].re;
				if (tmpRe != 0)
				{
					sign = 1;
					if (tmpRe < 0)
					{
						sign = -1; tmpRe = -tmpRe;
					}
					if ((((unsigned __INT64)tmpRe<<shift)>>shift) ==times && ((((tmpRe>>scale)-1)&0x3)==0)) 
						tmpRe = (tmpRe)>>scaleFactor;
					else
						tmpRe = (tmpRe+rounding)>>scaleFactor;
					pDst[i].re = (Ipp32s)CLIP((sign * tmpRe),MAX_32S, MIN_32S);
				}
				else
				{
					pDst[i].re = 0;
				}

				if (tmpIm != 0)
				{
					sign = 1;
					if (tmpIm < 0)
					{
						sign = -1; tmpIm = -tmpIm;
					}
					if ((((unsigned __INT64)tmpIm<<shift)>>shift) ==times && ((((tmpIm>>scale)-1)&0x3)==0)) 
						tmpIm = (tmpIm)>>scaleFactor;
					else
						tmpIm = (tmpIm+rounding)>>scaleFactor;
					pDst[i].im = (Ipp32s)CLIP((sign * tmpIm),MAX_32S, MIN_32S);
				}
				else
				{
					pDst[i].im = 0;
				}
			}
		}
		else
		{
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 64)
				{
					pDst[i].re = 0;
					pDst[i].im = 0;
				}
				else
				{
					tmpRe = (__INT64)pSrc1[i].re*pSrc2[i].re - (__INT64)pSrc1[i].im * pSrc2[i].im;
					tmpIm = (__INT64)pSrc1[i].re*pSrc2[i].im + (__INT64)pSrc1[i].im * pSrc2[i].re;
					if (tmpRe -1 > MAX_64S)
						pDst[i].re = 1;
					else if (-(tmpRe+1) > MAX_64S)
						pDst[i].re = -1;
					else
						pDst[i].re = 0;

					if (tmpIm -1 > MAX_64S)
						pDst[i].im = 1;
					else if (-(tmpIm+1) > MAX_64S)
						pDst[i].im = -1;
					else
						pDst[i].im = 0;
				}	
			}
		}
	}
	else  /*scaleFactor < 0*/
	{
		if (scaleFactor > -32)	/* -1 --> -31 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmpRe = (__INT64)pSrc1[i].re*pSrc2[i].re - (__INT64)pSrc1[i].im * pSrc2[i].im;
				tmpIm = (__INT64)pSrc1[i].re*pSrc2[i].im + (__INT64)pSrc1[i].im * pSrc2[i].re;
				if (tmpRe>0 &&((MAX_32S >> scale) < tmpRe))
					pDst[i].re = (Ipp32s)OVER(tmpRe,MAX_32S, MIN_32S);
				else if (tmpRe< 0 &&(((-MIN_32S) >>scale) < (-tmpRe)))
					pDst[i].re = (Ipp32s)OVER(tmpRe,MAX_32S, MIN_32S);
				else
					pDst[i].re = (Ipp32s)(tmpRe << scale);

				if (tmpIm>0 &&((MAX_32S >> scale) < tmpIm))
					pDst[i].im = (Ipp32s)OVER(tmpIm,MAX_32S, MIN_32S);
				else if (tmpIm< 0 &&(((-MIN_32S) >>scale) < (-tmpIm)))
					pDst[i].im = (Ipp32s)OVER(tmpIm,MAX_32S, MIN_32S);
				else
					pDst[i].im = (Ipp32s)(tmpIm << scale);

			}
		}
		else					/* -32 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmpRe = (__INT64)pSrc1[i].re*pSrc2[i].re - (__INT64)pSrc1[i].im * pSrc2[i].im;
				tmpIm = (__INT64)pSrc1[i].re*pSrc2[i].im + (__INT64)pSrc1[i].im * pSrc2[i].re;
				pDst[i].re= (Ipp32s)OVER(tmpRe,MAX_32S, MIN_32S);
				pDst[i].im= (Ipp32s)OVER(tmpIm,MAX_32S, MIN_32S);
			}
		}
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippsMul_32sc_Sfs",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMul_32sc_Sfs(pSrc1, pSrc2, pDst, len,scaleFactor);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippsMul_32s32sc_Sfs_c(const Ipp32s* pSrc1, const Ipp32sc* pSrc2,
       Ipp32sc* pDst, int len, int scaleFactor)
{
#if 1
	int i;
	__INT64 tmpRe, tmpIm;

	if (!pSrc1 || ! pSrc2 || ! pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmpRe = (__INT64)pSrc1[i] * pSrc2[i].re;
			tmpIm = (__INT64)pSrc1[i] * pSrc2[i].im;
			pDst[i].re = (Ipp32s)CLIP(tmpRe, MAX_32S, MIN_32S);
			pDst[i].im = (Ipp32s)CLIP(tmpIm, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 64)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			__INT64 times = (__INT64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmpRe = (__INT64)pSrc1[i] * pSrc2[i].re;
				tmpIm = (__INT64)pSrc1[i] * pSrc2[i].im;
				if (tmpRe != 0)
				{
					sign = 1;
					if (tmpRe < 0)
					{
						sign = -1; tmpRe = -tmpRe;
					}
					if ((((unsigned __INT64)tmpRe<<shift)>>shift) ==times && ((((tmpRe>>scale)-1)&0x3)==0)) 
						tmpRe = (tmpRe)>>scaleFactor;
					else
						tmpRe = (tmpRe+rounding)>>scaleFactor;
					pDst[i].re = (Ipp32s)CLIP((sign * tmpRe),MAX_32S, MIN_32S);
				}
				else
				{
					pDst[i].re = 0;
				}

				if (tmpIm != 0)
				{
					sign = 1;
					if (tmpIm < 0)
					{
						sign = -1; tmpIm = -tmpIm;
					}
					if ((((unsigned __INT64)tmpIm<<shift)>>shift) ==times && ((((tmpIm>>scale)-1)&0x3)==0)) 
						tmpIm = (tmpIm)>>scaleFactor;
					else
						tmpIm = (tmpIm+rounding)>>scaleFactor;
					pDst[i].im = (Ipp32s)CLIP((sign * tmpIm),MAX_32S, MIN_32S);
				}
				else
				{
					pDst[i].im = 0;
				}
			}
		}
		else
		{
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 64)
				{
					pDst[i].re = 0;
					pDst[i].im = 0;
				}
				else
				{
					tmpRe = (__INT64)pSrc1[i] * pSrc2[i].re;
					tmpIm = (__INT64)pSrc1[i] * pSrc2[i].im;
					if (tmpRe -1 > MAX_64S)
						pDst[i].re = 1;
					else if (-(tmpRe+1) > MAX_64S)
						pDst[i].re = -1;
					else
						pDst[i].re = 0;

					if (tmpIm -1 > MAX_64S)
						pDst[i].im = 1;
					else if (-(tmpIm+1) > MAX_64S)
						pDst[i].im = -1;
					else
						pDst[i].im = 0;
				}	
			}
		}
	}
	else  /*scaleFactor < 0*/
	{
		if (scaleFactor > -32)	/* -1 --> -31 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmpRe = (__INT64)pSrc1[i] * pSrc2[i].re;
				tmpIm = (__INT64)pSrc1[i] * pSrc2[i].im;
				if (tmpRe>0 &&((MAX_32S >> scale) < tmpRe))
					pDst[i].re = (Ipp32s)OVER(tmpRe,MAX_32S, MIN_32S);
				else if (tmpRe< 0 &&(((-MIN_32S) >>scale) < (-tmpRe)))
					pDst[i].re = (Ipp32s)OVER(tmpRe,MAX_32S, MIN_32S);
				else
					pDst[i].re = (Ipp32s)(tmpRe << scale);

				if (tmpIm>0 &&((MAX_32S >> scale) < tmpIm))
					pDst[i].im = (Ipp32s)OVER(tmpIm,MAX_32S, MIN_32S);
				else if (tmpIm< 0 &&(((-MIN_32S) >>scale) < (-tmpIm)))
					pDst[i].im = (Ipp32s)OVER(tmpIm,MAX_32S, MIN_32S);
				else
					pDst[i].im = (Ipp32s)(tmpIm << scale);

			}
		}
		else					/* -32 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmpRe = (__INT64)pSrc1[i] * pSrc2[i].re;
				tmpIm = (__INT64)pSrc1[i] * pSrc2[i].im;
				pDst[i].re= (Ipp32s)OVER(tmpRe,MAX_32S, MIN_32S);
				pDst[i].im= (Ipp32s)OVER(tmpIm,MAX_32S, MIN_32S);
			}
		}
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippsMul_32s32sc_Sfs",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMul_32s32sc_Sfs(pSrc1, pSrc2, pDst, len,scaleFactor);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippsMulC_32s_Sfs_c(const Ipp32s*  pSrc, Ipp32s  val,Ipp32s*  pDst, int len, int scaleFactor)
{
#if 1
	int i;
	__INT64 tmp;

	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (__INT64)val*pSrc[i];
			pDst[i] = (Ipp32s)CLIP(tmp, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 64)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			__INT64 times = (__INT64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)val*pSrc[i];
				if (tmp != 0)
				{
					sign = 1;
					if (tmp < 0)
					{
						sign = -1; tmp = -tmp;
					}
					if ((((unsigned __INT64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pDst[i] = (Ipp32s)CLIP((sign * tmp),MAX_32S, MIN_32S);
				}
				else
				{
					pDst[i] = 0;
				}
			}
		}
		else
		{
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 64)
					pDst[i] = 0;
				else
				{
					tmp = (Ipp64s)val*pSrc[i];
					if (tmp -1 > MAX_64S)
						pDst[i] = 1;
					else if (-(tmp+1) > MAX_64S)
						pDst[i] = -1;
					else
						pDst[i] = 0;
				}	
			}
		}
	}
	else  /*scaleFactor < 0*/
	{
		if (scaleFactor > -32)	/* -1 --> -31 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((__INT64)val*pSrc[i]);
				if (tmp>0 &&((MAX_32S >> scale) < tmp))
					pDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
				else if (tmp< 0 &&(((-MIN_32S) >>scale) < (-tmp)))
					pDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
				else
					pDst[i] = (Ipp32s)(tmp << scale);
			}
		}
		else					/* -32 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (__INT64)val*pSrc[i];
				pDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
			}
		}
	}

#endif

#ifdef PRINT_REF_INFO
		dump("ippsMulC_32s_Sfs",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMulC_32s_Sfs(pSrc, val, pDst, len, scaleFactor);
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

	if(len <=0)
		return ippStsSizeErr;

	memset((unsigned char*)pDst, val, len);
#endif

#ifdef PRINT_REF_INFO
	dump("ippsSet_8u",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsSet_8u(val, pDst, len);
		return sts;
	}
#endif

	return ippStsNoErr;
}


/* following functions only used in mp3*/
IppStatus __STDCALL ippsAddC_32s_ISfs_c(Ipp32s val, Ipp32s* pSrcDst, int len, int scaleFactor)
{
#if 1
	int i;
	Ipp64s tmp;
	if (!pSrcDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;
	
	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (Ipp64s)val+pSrcDst[i];
			pSrcDst[i] = (Ipp32s)CLIP(tmp,MAX_32S,MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 32)	/* 1 --> 31 */
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			__INT64 times = (__INT64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)val+pSrcDst[i];
				if (tmp != 0)
				{
					sign = 1;
					if (tmp < 0)
					{
						sign = -1; tmp = -tmp;
					}
					if ((((unsigned __INT64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pSrcDst[i] = (Ipp32s)(sign * tmp);
				}
				else
				{
					pSrcDst[i] = 0;
				}
			}
		}
		else	/* right shift 32 bits, every 32bits integer will be 0, but MAX_32S+1 */
		{		
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 32)
					pSrcDst[i] = 0;
				else
				{
					tmp = (Ipp64s)val+pSrcDst[i];
					if (tmp -1 > MAX_32S)
						pSrcDst[i] = 1;
					else if (-(tmp+1) > MAX_32S)
						pSrcDst[i] = -1;
					else
						pSrcDst[i] = 0;
				}					
			}
		}
	}
	else	
	{
		if (scaleFactor > -32)	/* -1 --> -31 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((Ipp64s)val+pSrcDst[i])<<scale;
				pSrcDst[i] = (Ipp32s)CLIP(tmp,MAX_32S,MIN_32S);
			}
		}
		else					/* -32 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)val+pSrcDst[i];
				pSrcDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
			}
		}
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippsAddC_32s_ISfs",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsAddC_32s_ISfs(val, pSrcDst, len, scaleFactor);
		return sts;
	}
#endif
	return ippStsNoErr;
}


IppStatus __STDCALL ippsMulC_32s_ISfs_c(Ipp32s val, Ipp32s* pSrcDst, int len, int scaleFactor)
{
#if 1
	int i;
	__INT64 tmp;

	if (! pSrcDst )
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (__INT64)val*pSrcDst[i];
			pSrcDst[i] = (Ipp32s)CLIP(tmp, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 64)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			__INT64 times = (__INT64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)val*pSrcDst[i];
				if (tmp != 0)
				{
					sign = 1;
					if (tmp < 0)
					{
						sign = -1; tmp = -tmp;
					}
					if ((((unsigned __INT64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pSrcDst[i] = (Ipp32s)CLIP((sign * tmp),MAX_32S, MIN_32S);
				}
				else
				{
					pSrcDst[i] = 0;
				}
			}
		}
		else
		{
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 64)
					pSrcDst[i] = 0;
				else
				{
					tmp = (Ipp64s)val*pSrcDst[i];
					if (tmp -1 > MAX_64S)
						pSrcDst[i] = 1;
					else if (-(tmp+1) > MAX_64S)
						pSrcDst[i] = -1;
					else
						pSrcDst[i] = 0;
				}	
			}
		}
	}
	else  /*scaleFactor < 0*/
	{
		if (scaleFactor > -32)	/* -1 --> -31 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((__INT64)val*pSrcDst[i]);
				if (tmp>0 &&((MAX_32S >> scale) < tmp))
					pSrcDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
				else if (tmp< 0 &&(((-MIN_32S) >>scale) < (-tmp)))
					pSrcDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
				else
					pSrcDst[i] = (Ipp32s)(tmp << scale);
			}
		}
		else					/* -32 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (__INT64)val*pSrcDst[i];
				pSrcDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
			}
		}
	}

#endif

#ifdef PRINT_REF_INFO
		dump("ippsMulC_32s_ISfs",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMulC_32s_ISfs(val, pSrcDst, len, scaleFactor);
		return sts;
	}
#endif
	return ippStsNoErr;
}

#endif
