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
#include "kinoma_ipp_common.h"
#include "kinoma_ipp_lib.h"

IppStatus (__STDCALL *ippsAbs_16s_I_universal)				(Ipp16s* pSrcDst,int len)=NULL;
IppStatus (__STDCALL *ippsAbs_16s_universal)					(const Ipp16s* pSrc, Ipp16s* pDst,int len)=NULL;
IppStatus (__STDCALL *ippsAdd_32s_Sfs_universal)		(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsAdd_32s_ISfs_universal)		(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsAddC_32s_ISfs_universal)		(Ipp32s val, Ipp32s* pSrcDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsAdd_16s_Sfs_universal)				(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsAdd_16s_ISfs_universal)			(const Ipp16s*  pSrc, Ipp16s*  pSrcDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsAddC_16s_Sfs_universal)			(const Ipp16s*  pSrc, Ipp16s  val,Ipp16s*  pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsConvert_64s32s_Sfs_universal)	(const Ipp64s* pSrc, Ipp32s* pDst, int len, IppRoundMode rndMode, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsConvert_32s16s_Sfs_universal)	(const Ipp32s* pSrc, Ipp16s* pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsDiv_32s_ISfs_universal)		(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int ScaleFactor)=NULL;
IppStatus (__STDCALL *ippsDiv_16s_Sfs_universal)				(const Ipp16s* pSrc1, const Ipp16s* pSrc2,Ipp16s* pDst, int len, int ScaleFactor)=NULL;
IppStatus (__STDCALL *ippsDiv_16s_ISfs_universal)			(const Ipp16s* pSrc, Ipp16s* pSrcDst,int len, int ScaleFactor)=NULL;
IppStatus (__STDCALL *ippsDivC_16s_ISfs_universal)			(Ipp16s val, Ipp16s* pSrcDst, int len, int ScaleFactor)=NULL;
IppStatus (__STDCALL *ippsDotProd_16s32s32s_Sfs_universal)	( const Ipp16s* pSrc1, const Ipp32s* pSrc2,int len, Ipp32s* pDp, int scaleFactor )=NULL;
IppStatus (__STDCALL *ippsDotProd_16s32s_Sfs_universal)		( const Ipp16s* pSrc1, const Ipp16s* pSrc2, int len, Ipp32s* pDp, int scaleFactor )=NULL;
IppStatus (__STDCALL *ippsMagnitude_16sc_Sfs_universal)		(const Ipp16sc* pSrc,Ipp16s* pDst, int len,int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMagnitude_16s_Sfs_universal)		(const Ipp16s* pSrcRe,const Ipp16s* pSrcIm,Ipp16s* pDst,int len,int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMax_32s_universal)			(const Ipp32s* pSrc, int len, Ipp32s* pMax)=NULL;
IppStatus (__STDCALL *ippsMax_16s_universal)			(const Ipp16s* pSrc, int len, Ipp16s* pMax)=NULL;
IppStatus (__STDCALL *ippsMaxEvery_32s_I_universal)			(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len)=NULL;
IppStatus (__STDCALL *ippsMaxAbs_32s_universal)			(const Ipp32s* pSrc, int len, Ipp32s* pMaxAbs)=NULL;
IppStatus (__STDCALL *ippsMinMax_32s_universal)			(const Ipp32s* pSrc, int len, Ipp32s* pMin, Ipp32s* pMax)=NULL;
IppStatus (__STDCALL *ippsMinEvery_32s_I_universal)			(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len)=NULL;
IppStatus (__STDCALL *ippsMinMax_16s_universal)				(const Ipp16s* pSrc, int len, Ipp16s* pMin, Ipp16s* pMax)=NULL;
IppStatus (__STDCALL *ippsMin_32s_universal)			(const Ipp32s* pSrc, int len, Ipp32s* pMin)=NULL;
IppStatus (__STDCALL *ippsMinIndx_32s_universal)		(const Ipp32s* pSrc, int len, Ipp32s* pMin, int* pIndx)=NULL;

IppStatus (__STDCALL *ippsMul_32s_ISfs_universal)		(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMul_32s_Sfs_universal)		(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst,int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMul_16s_Sfs_universal)				(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMul_16s_ISfs_universal)			(const Ipp16s*  pSrc, Ipp16s*  pSrcDst,int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMul_16s_universal)					(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp16s*  pDst, int len)=NULL;
IppStatus (__STDCALL *ippsMul_16s32s_Sfs_universal)			(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp32s*  pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMul_32sc_Sfs_universal)		(const Ipp32sc* pSrc1, const Ipp32sc* pSrc2, Ipp32sc* pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMul_32s32sc_Sfs_universal)	(const Ipp32s* pSrc1, const Ipp32sc* pSrc2, Ipp32sc* pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMulC_32s_Sfs_universal)		(const Ipp32s*  pSrc, Ipp32s  val,Ipp32s*  pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMulC_16s_Sfs_universal)			(const Ipp16s*  pSrc, Ipp16s  val,Ipp16s*  pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsMulC_32s_ISfs_universal)		(Ipp32s val, Ipp32s* pSrcDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsLShiftC_32s_I_universal)		(int val, Ipp32s* pSrcDst, int len)=NULL;
IppStatus (__STDCALL *ippsLShiftC_16s_I_universal)			(int val, Ipp16s* pSrcDst, int len)=NULL;
IppStatus (__STDCALL *ippsRShiftC_32s_I_universal)		(int val, Ipp32s* pSrcDst, int len)=NULL;
IppStatus (__STDCALL *ippsRShiftC_32s_universal)				(const Ipp32s* pSrc, int val, Ipp32s* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsRShiftC_16s_universal)				(const Ipp16s* pSrc, int val, Ipp16s* pDst, int len)=NULL;
IppStatus (__STDCALL *ippsPow34_16s_Sfs_universal)			(const Ipp16s* pSrc, int inScaleFactor, Ipp16s* pDst, int ScaleFactor, int len)=NULL;
IppStatus (__STDCALL *ippsSum_16s32s_Sfs_universal)			(const Ipp16s*  pSrc, int len,Ipp32s*  pSum, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsSum_32s_Sfs_universal)				(const Ipp32s*  pSrc, int len, Ipp32s*  pSum, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsSub_16s_universal)					(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp16s*  pDst, int len)=NULL;
IppStatus (__STDCALL *ippsSub_16s_ISfs_universal)			(const Ipp16s*  pSrc, Ipp16s*  pSrcDst,int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsSub_16s_Sfs_universal)				(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor)=NULL;																	
IppStatus (__STDCALL *ippsSub_32s_Sfs_universal)		(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsSqrt_64s_ISfs_universal)		(Ipp64s* pSrcDst, int len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsSpread_16s_Sfs_universal)			(Ipp16s src1, Ipp16s src2, int inScaleFactor, Ipp16s* pDst)=NULL;
IppStatus (__STDCALL *ippsSortAscend_32s_I_universal)	(Ipp32s* pSrcDst, int len)=NULL;
IppStatus (__STDCALL *ippsLn_32s16s_Sfs_universal)			( const Ipp32s* pSrc, Ipp16s* pDst, int Len, int scaleFactor)=NULL;
IppStatus (__STDCALL *ippsDeinterleave_16s_universal)		(const Ipp16s* pSrc, int ch_num,int len, Ipp16s** pDst)=NULL;



#undef CLIP
#define	   CLIP(x, max, min) ((x)>(max)?(max):((x)<(min)?(min):(x)))
#define    OVER(x,max,min) ((x)>0?(max):((x)<0?(min):(x)))
#define    ABSM(a)  ((a)>=0)?(a):(-(a))
//#define	   MIN(x,y)   ((x) < (y)? (x) : (y))

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
			pDst[i] = (Ipp32s)CLIP_R(tmp,MAX_32S,MIN_32S);
		}
	}
	else if (scaleFactor > 0 )
	{
		if (scaleFactor < 32)	/* 1 --> 31 */
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
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
				pDst[i] = (Ipp32s)CLIP_R(tmp,MAX_32S,MIN_32S);
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
	//dump("ippsAdd_32s_Sfs",-1);
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
			pSrcDst[i] = (Ipp32s)CLIP_R(tmp,MAX_32S,MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 32)	/* 1 --> 31 */
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
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
				pSrcDst[i] = (Ipp32s)CLIP_R(tmp,MAX_32S,MIN_32S);
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
	//dump("ippsAdd_32s_ISfs_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsAdd_32s_ISfs_c(pSrc, pSrcDst, len, scaleFactor);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippsMul_32s_ISfs_c(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int scaleFactor)
{
#if 1
	int i;
	__int64 tmp;

	if (!pSrc || ! pSrcDst )
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (__int64)pSrc[i]*pSrcDst[i];
			pSrcDst[i] = (Ipp32s)CLIP_R(tmp, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 64)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pSrcDst[i] = (Ipp32s)CLIP_R((sign * tmp),MAX_32S, MIN_32S);
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
					/*if (tmp -1 > MAX_64S)
						pSrcDst[i] = 1;
					else if (-(tmp+1) > MAX_64S)
						pSrcDst[i] = -1;
					else*/
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
				tmp = ((__int64)pSrc[i]*pSrcDst[i]);
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
				tmp = (__int64)pSrc[i]*pSrcDst[i];
				pSrcDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
			}
		}
	}
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsMul_32s_ISfs_c",-1);
#endif

#if 0
	{ 
		IppStatus sts;
		sts = ippsMul_32s_ISfs_c(pSrc, pSrcDst, len, scaleFactor);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMul_32s_Sfs_c(const Ipp32s* pSrc1, const Ipp32s* pSrc2, Ipp32s* pDst,int len, int scaleFactor)
{
#if 1	
	int i;
	__int64 tmp;

	if (!pSrc1 || ! pSrc2 || ! pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (__int64)pSrc1[i]*pSrc2[i];
			pDst[i] = (Ipp32s)CLIP_R(tmp, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 64)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pDst[i] = (Ipp32s)CLIP_R((sign * tmp),MAX_32S, MIN_32S);
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
					/*if (tmp -1 > MAX_64S)
						pDst[i] = 1;
					else if (-(tmp+1) > MAX_64S)
						pDst[i] = -1;
					else*/
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
				tmp = ((__int64)pSrc1[i]*pSrc2[i]);
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
				tmp = (__int64)pSrc1[i]*pSrc2[i];
				pDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
			}
		}
	}
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsMul_32s_Sfs_c",-1);
#endif

#if 0		
	{
		IppStatus sts;
		sts = ippsMul_32s_Sfs_c(pSrc1, pSrc2, pDst, len, scaleFactor);
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
			pDst[i] = (Ipp32s)CLIP_R(tmp, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 32)	/* 1 --> 31 */
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
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
				pDst[i] = (Ipp32s)CLIP_R(tmp,MAX_32S,MIN_32S);
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
	//dump("ippsSub_32s_Sfs",-1);
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


/*  quick sort */
static __inline int  partition(int data[], int x, int y)
{
    int  n = data[x], i = x + 1, j = y, temp;
    while(1)
    {
        while(data[i] <= n) ++i;
        while(data[j] > n) --j;
        if(i >= j) break;
        temp = data[i]; 
		data[i] = data[j]; 
		data[j]=temp;
    }
    data[x] = data[j];
    data[j] = n;
    return j;
}

void quick_sort(int data[], int x, int y)
{
	int q;
    if(x >= y)
		return;
    q = partition(data, x, y);
    quick_sort(data, x, q-1);
    quick_sort(data, q+1, y);
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
	//dump("ippsSortAscend_32s_I",-1);
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
	//dump("ippsMinMax_32s_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMinMax_32s_c(pSrc, len, pMin, pMax);
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
	//dump("ippsMax_32s",-1);
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
	//dump("ippsMax_16s_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMax_16s_c(pSrc, len, pMax);
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
	//dump("ippsMin_32s",-1);
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


IppStatus __STDCALL ippsMinIndx_32s_c(const Ipp32s* pSrc, int len, Ipp32s* pMin, int* pIndx)
{
#if 1
	int i, idx;
	Ipp32s tmp;
	if (!pSrc || !pMin)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	idx = 0;
	tmp = pSrc[idx];
	for (i = 1; i < len; i++)
	{
		if (tmp > pSrc[i])
		{
			tmp = pSrc[i];
			idx = i;
		}
	}

	*pMin  = tmp;
	*pIndx = idx;
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsMin_32s",-1);
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
	//dump("ippsMaxAbs_32s",-1);
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



/* function for DIV */
static __inline int isDIVbyZERO(int *isMinus, __int64 *base, __int64 *div, const Ipp32s Src, Ipp32s *srcDst, IppStatus *sts)   			
{
	*isMinus = 0;	
	if ((*div = (__int64)(*srcDst)) < 0)	
	{					
		*div = -(*div);			
		*isMinus = !(*isMinus);		
	}							
	if ((*base = (__int64)Src) < 0)		
	{							
		*base = -(*base);			
		*isMinus = !(*isMinus);		
	}							
	if (*base == 0)				
	{							
		if (*div ==0)			
		{						
			*srcDst = 0;		
		}						
		else if (*isMinus)
		{
			*srcDst = (Ipp32s)MIN_32S;
		}
		else
		{
			*srcDst = (Ipp32s)MAX_32S;
		}
		*sts = ippStsDivByZero;
		return 1;
	}
	return 0;
}


IppStatus __STDCALL ippsDiv_32s_ISfs_c(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len, int ScaleFactor)
{
#if 1
	int i;
	__int64 tmp;
	__int64 div, base, tmpI, tmpR;
	int isMinus;
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

			tmpI = div/base;
			tmpR = div - tmpI*base; 
			if ( ((tmpR<<1)>base) || ((tmpI&1) && ((tmpR<<1)==base)) )
				tmp = tmpI +1;
			else 
				tmp = tmpI;

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

				base <<= ScaleFactor;
				tmpI = div/base;
				tmpR = div-tmpI*base;
				if ( ((tmpR<<1)>base) || ((tmpI&1) && ((tmpR<<1)==base)) )
					tmp = tmpI +1;
				else 
					tmp = tmpI;

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
		int scale = -ScaleFactor;
		for (i = 0; i < len; i++)
		{
			if(isDIVbyZERO(&isMinus, &base, &div, pSrc[i],&pSrcDst[i],&sts))
				continue;

			if ((MAX_64S>>scale)>div)
			{
				div <<= scale;
				tmpI = div/base;
				tmpR = div-tmpI*base;
				if ( ((tmpR<<1)>base) || ((tmpI&1) && ((tmpR<<1)==base)) )
					tmp = tmpI +1;
				else 
					tmp = tmpI;

				if (isMinus)
					tmp = -tmp;

				pSrcDst[i] = (Ipp32s)CLIP(tmp, MAX_32S, MIN_32S);
			}
			else
				pSrcDst[i] = (Ipp32s)OVER(pSrc[i],MAX_32S, MIN_32S);

		}
	}

#endif

#ifdef PRINT_REF_INFO
	//dump("ippsDiv_32s_ISfs_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsDiv_32s_ISfs_c(pSrc, pSrcDst, len, ScaleFactor);
		return sts;
	}
#endif

	return sts;
}



/* function used in SQRT,   data = root^2 + remainder , root & remainder are integers*/
static void __inline  qk_sqrt(__int64 data, __int64 *remainder, __int64 *root)
{
	__int64  squaredbit;

	if (data < 1)
	{
		*remainder = 0;
		*root = 0;
		return;
	}

	if (data > 2147483647)	//1 << 31 = 2147483648
	{
		squaredbit  = (__int64) ((((unsigned __int64) ~0) >> 1) & 
			~(((unsigned __int64) ~0) >> 2));
	}
	else 
	{
		squaredbit  = (int) ((((unsigned int) ~0) >> 1) & 
			~(((unsigned int) ~0) >> 2));
	}

	*remainder = data;  
	*root = 0;
	while (squaredbit > 0) {
		if (*remainder >= (squaredbit | *root)) 
		{
			*remainder -= (squaredbit | *root);
			*root >>= 1; 
			*root |= squaredbit;
		} 
		else 
		{
			*root >>= 1;
		}
		squaredbit >>= 2; 
	}
}

/* calc the float part of sqrt root
sqrt(data) = (Iroot+froot)   // Iroot is the integer part of root, froot is the float part
==> data = (Iroot+froot) * (Iroot+froot) = Iroot^2 + 2*Iroot*froot + froot^2
and also data = Iroot^2 + remainder
so, remainder = 2*Iroot*froot + froot^2
then, froot = (remainder - froot^2) / (2*Iroot)
here,we can use iterative method to approach the froot
*/
static long double  __inline float_part(Ipp64s remainder, Ipp64s root)
{
	long double froot,ftmp;
	const long double limit = 0.000000000001;

	Ipp64s Iroot2 = (root) * 2;

	if (root == 0 || remainder == 0)
		return 0.0f;

	froot = remainder / (long double)Iroot2;
	do
	{
		ftmp = froot;
		froot =(remainder - ftmp*ftmp) / (long double)Iroot2;
	}while (fabs(froot - ftmp) > limit);

	return froot;
}

IppStatus __STDCALL ippsSqrt_64s_ISfs_c(Ipp64s* pSrcDst, int len, int scaleFactor)
{
#if 1
	int i;
	Ipp64s   remainder;
	unsigned __int64 root;

	if (!pSrcDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			qk_sqrt( pSrcDst[i], &remainder, (__int64 *)&root);
			if (remainder > (__int64)root)
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
		Ipp64s half = (__int64)1<<(scaleFactor-1);
		int scale = (scaleFactor-1);
		unsigned __int64 times = (unsigned __int64)1<<scale;
		int shift = 64-scaleFactor;
		Ipp64s tmp;
		for (i = 0; i < len; i++)
		{
			tmp = pSrcDst[i];
			qk_sqrt(tmp, &remainder, (__int64 *)&root);

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
			qk_sqrt( pSrcDst[i], &remainder, (__int64 *)&root);
			froot = root + (double)float_part(remainder,root);
			if ((MAX_64S >> (-scaleFactor)) <= (__int64)root)
				pSrcDst[i] = froot > 0 ? MAX_64S: 0;
			else
				pSrcDst[i] = (Ipp64s)(froot *  scale + 0.5f);
		}
	}
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsSqrt_64s_ISfs",-1);
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
	__int64 tmp;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			pDst[i] = (Ipp32s)CLIP_R(pSrc[i],MAX_32S,MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (rndMode == ippRndZero)
		{
			for (i = 0; i < len; i++)
			{
				tmp = pSrc[i] >> scaleFactor;
				pDst[i] = (Ipp32s)CLIP_R(tmp,MAX_32S,MIN_32S);
			}
		}
		else
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pDst[i] = (Ipp32s)CLIP_R((sign * tmp), MAX_32S, MIN_32S);
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
	//dump("ippsConvert_64s32s_Sfs_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsConvert_64s32s_Sfs_c(pSrc, pDst, len, rndMode, scaleFactor);
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
			pDst[i] = (Ipp16s)CLIP_R(pSrc[i],MAX_16S, MIN_16S);
		}
	}
	else if (scaleFactor > 0)
	{
		int sign;
		__int64 rounding = ((__int64)1<<(scaleFactor-1));
		int scale = (scaleFactor -1);
		unsigned __int64 times = (unsigned __int64)1<<scale;
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
				if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
					tmp = (tmp)>>scaleFactor;
				else
					tmp = (int)((tmp+rounding)>>scaleFactor);
				pDst[i] = (Ipp16s)CLIP_R((sign * tmp),MAX_16S, MIN_16S);
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
	//dump("ippsConvert_32s16s_Sfs_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsConvert_32s16s_Sfs_c(pSrc, pDst,  len, scaleFactor);
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
	//dump("ippsLShiftC_32s_I",-1);
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
	//dump("ippsRShiftC_32s_I_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsRShiftC_32s_I_c(val, pSrcDst, len);
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
	__int64 tmpRe, tmpIm;

	if (!pSrc1 || ! pSrc2 || ! pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmpRe = (__int64)pSrc1[i].re*pSrc2[i].re - (__int64)pSrc1[i].im * pSrc2[i].im;
			tmpIm = (__int64)pSrc1[i].re*pSrc2[i].im + (__int64)pSrc1[i].im * pSrc2[i].re;
			pDst[i].re = (Ipp32s)CLIP_R(tmpRe, MAX_32S, MIN_32S);
			pDst[i].im = (Ipp32s)CLIP_R(tmpIm, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 64)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmpRe = (__int64)pSrc1[i].re*pSrc2[i].re - (__int64)pSrc1[i].im * pSrc2[i].im;
				tmpIm = (__int64)pSrc1[i].re*pSrc2[i].im + (__int64)pSrc1[i].im * pSrc2[i].re;
				if (tmpRe != 0)
				{
					sign = 1;
					if (tmpRe < 0)
					{
						sign = -1; tmpRe = -tmpRe;
					}
					if ((((unsigned __int64)tmpRe<<shift)>>shift) ==times && ((((tmpRe>>scale)-1)&0x3)==0)) 
						tmpRe = (tmpRe)>>scaleFactor;
					else
						tmpRe = (tmpRe+rounding)>>scaleFactor;
					pDst[i].re = (Ipp32s)CLIP_R((sign * tmpRe),MAX_32S, MIN_32S);
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
					if ((((unsigned __int64)tmpIm<<shift)>>shift) ==times && ((((tmpIm>>scale)-1)&0x3)==0)) 
						tmpIm = (tmpIm)>>scaleFactor;
					else
						tmpIm = (tmpIm+rounding)>>scaleFactor;
					pDst[i].im = (Ipp32s)CLIP_R((sign * tmpIm),MAX_32S, MIN_32S);
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
					tmpRe = (__int64)pSrc1[i].re*pSrc2[i].re - (__int64)pSrc1[i].im * pSrc2[i].im;
					tmpIm = (__int64)pSrc1[i].re*pSrc2[i].im + (__int64)pSrc1[i].im * pSrc2[i].re;
					/*if (tmpRe -1 > MAX_64S)
						pDst[i].re = 1;
					else if (-(tmpRe+1) > MAX_64S)
						pDst[i].re = -1;
					else*/
						pDst[i].re = 0;

					/*if (tmpIm -1 > MAX_64S)
						pDst[i].im = 1;
					else if (-(tmpIm+1) > MAX_64S)
						pDst[i].im = -1;
					else*/
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
				tmpRe = (__int64)pSrc1[i].re*pSrc2[i].re - (__int64)pSrc1[i].im * pSrc2[i].im;
				tmpIm = (__int64)pSrc1[i].re*pSrc2[i].im + (__int64)pSrc1[i].im * pSrc2[i].re;
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
				tmpRe = (__int64)pSrc1[i].re*pSrc2[i].re - (__int64)pSrc1[i].im * pSrc2[i].im;
				tmpIm = (__int64)pSrc1[i].re*pSrc2[i].im + (__int64)pSrc1[i].im * pSrc2[i].re;
				pDst[i].re= (Ipp32s)OVER(tmpRe,MAX_32S, MIN_32S);
				pDst[i].im= (Ipp32s)OVER(tmpIm,MAX_32S, MIN_32S);
			}
		}
	}
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsMul_32sc_Sfs",-1);
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
	__int64 tmpRe, tmpIm;

	if (!pSrc1 || ! pSrc2 || ! pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmpRe = (__int64)pSrc1[i] * pSrc2[i].re;
			tmpIm = (__int64)pSrc1[i] * pSrc2[i].im;
			pDst[i].re = (Ipp32s)CLIP_R(tmpRe, MAX_32S, MIN_32S);
			pDst[i].im = (Ipp32s)CLIP_R(tmpIm, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 64)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmpRe = (__int64)pSrc1[i] * pSrc2[i].re;
				tmpIm = (__int64)pSrc1[i] * pSrc2[i].im;
				if (tmpRe != 0)
				{
					sign = 1;
					if (tmpRe < 0)
					{
						sign = -1; tmpRe = -tmpRe;
					}
					if ((((unsigned __int64)tmpRe<<shift)>>shift) ==times && ((((tmpRe>>scale)-1)&0x3)==0)) 
						tmpRe = (tmpRe)>>scaleFactor;
					else
						tmpRe = (tmpRe+rounding)>>scaleFactor;
					pDst[i].re = (Ipp32s)CLIP_R((sign * tmpRe),MAX_32S, MIN_32S);
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
					if ((((unsigned __int64)tmpIm<<shift)>>shift) ==times && ((((tmpIm>>scale)-1)&0x3)==0)) 
						tmpIm = (tmpIm)>>scaleFactor;
					else
						tmpIm = (tmpIm+rounding)>>scaleFactor;
					pDst[i].im = (Ipp32s)CLIP_R((sign * tmpIm),MAX_32S, MIN_32S);
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
					tmpRe = (__int64)pSrc1[i] * pSrc2[i].re;
					tmpIm = (__int64)pSrc1[i] * pSrc2[i].im;
					
					//tmpRe and tmpIm are all __int64 integer
					/*if (tmpRe -1 > MAX_64S)
						pDst[i].re = 1;
					else if (-(tmpRe+1) > MAX_64S)
						pDst[i].re = -1;
					else*/
						pDst[i].re = 0;

					/*if (tmpIm -1 > MAX_64S)
						pDst[i].im = 1;
					else if (-(tmpIm+1) > MAX_64S)
						pDst[i].im = -1;
					else*/
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
				tmpRe = (__int64)pSrc1[i] * pSrc2[i].re;
				tmpIm = (__int64)pSrc1[i] * pSrc2[i].im;
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
				tmpRe = (__int64)pSrc1[i] * pSrc2[i].re;
				tmpIm = (__int64)pSrc1[i] * pSrc2[i].im;
				pDst[i].re= (Ipp32s)OVER(tmpRe,MAX_32S, MIN_32S);
				pDst[i].im= (Ipp32s)OVER(tmpIm,MAX_32S, MIN_32S);
			}
		}
	}
#endif

#ifdef PRINT_REF_INFO
	//dump("ippsMul_32s32sc_Sfs",-1);
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
	__int64 tmp;

	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (__int64)val*pSrc[i];
			pDst[i] = (Ipp32s)CLIP_R(tmp, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 64)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pDst[i] = (Ipp32s)CLIP_R((sign * tmp),MAX_32S, MIN_32S);
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
					/*if (tmp -1 > MAX_64S)
						pDst[i] = 1;
					else if (-(tmp+1) > MAX_64S)
						pDst[i] = -1;
					else*/
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
				tmp = ((__int64)val*pSrc[i]);
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
				tmp = (__int64)val*pSrc[i];
				pDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
			}
		}
	}

#endif

#ifdef PRINT_REF_INFO
	//dump("ippsMulC_32s_Sfs_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMulC_32s_Sfs_c(pSrc, val, pDst, len, scaleFactor);
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
			pSrcDst[i] = (Ipp32s)CLIP_R(tmp,MAX_32S,MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 32)	/* 1 --> 31 */
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
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
				pSrcDst[i] = (Ipp32s)CLIP_R(tmp,MAX_32S,MIN_32S);
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
	//dump("ippsAddC_32s_ISfs",-1);
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
	__int64 tmp;

	if (! pSrcDst )
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (__int64)val*pSrcDst[i];
			pSrcDst[i] = (Ipp32s)CLIP_R(tmp, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 64)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pSrcDst[i] = (Ipp32s)CLIP_R((sign * tmp),MAX_32S, MIN_32S);
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
					/*if (tmp -1 > MAX_64S)
						pSrcDst[i] = 1;
					else if (-(tmp+1) > MAX_64S)
						pSrcDst[i] = -1;
					else*/
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
				tmp = ((__int64)val*pSrcDst[i]);
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
				tmp = (__int64)val*pSrcDst[i];
				pSrcDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
			}
		}
	}

#endif

#ifdef PRINT_REF_INFO
	//dump("ippsMulC_32s_ISfs_c",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippsMulC_32s_ISfs_c(val, pSrcDst, len, scaleFactor);
		return sts;
	}
#endif
	return ippStsNoErr;
}

//
IppStatus __STDCALL ippsAbs_16s_I_c(Ipp16s* pSrcDst,int len)
{
	int i;
	if (!pSrcDst)
		return ippStsNullPtrErr;
	if (len < 1)
		return ippStsSizeErr;
	for (i = 0; i < len; i++)
	{
		if (pSrcDst[i] != MIN_16S)
			pSrcDst[i] = ABSM(pSrcDst[i]);
		else
			pSrcDst[i] = (Ipp16s)MAX_16S;
	}
	return ippStsNoErr;
}

// Add for AVC encoder
IppStatus __STDCALL ippsAbs_16s_c(const Ipp16s* pSrc, Ipp16s* pDst,int len)
{
	int i;
	if (!pSrc)
		return ippStsNullPtrErr;
	if (len < 1)
		return ippStsSizeErr;
	for (i = 0; i < len; i++)
	{
		if (pSrc[i] != MIN_16S)
			pDst[i] = ABSM(pSrc[i]);
		else
			pDst[i] = (Ipp16s)MAX_16S;
	}
	return ippStsNoErr;
}
IppStatus __STDCALL ippsMinEvery_32s_I_c(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len)
{
	int i;
	if (!pSrc || !pSrcDst)
		return ippStsNullPtrErr;
	if (len < 1)
		return ippStsSizeErr;

	for (i = 0; i < len; i++)
	{
		pSrcDst[i] = pSrc[i] < pSrcDst[i] ? pSrc[i] : pSrcDst[i];
	}

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMaxEvery_32s_I_c(const Ipp32s* pSrc, Ipp32s* pSrcDst, int len)
{
	int i;
	if (!pSrc || !pSrcDst)
		return ippStsNullPtrErr;
	if (len < 1)
		return ippStsSizeErr;

	for (i = 0; i < len; i++)
	{
		pSrcDst[i] = pSrc[i] > pSrcDst[i] ? pSrc[i] : pSrcDst[i];
	}

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMinMax_16s_c(const Ipp16s* pSrc, int len, Ipp16s* pMin, Ipp16s* pMax)
{
	int i;
	register Ipp16s tmpMin, tmpMax;
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

	return ippStsNoErr;
}
IppStatus __STDCALL ippsSum_16s32s_Sfs_c(const Ipp16s*  pSrc, int len,Ipp32s*  pSum, int scaleFactor)
{
	int i;
	Ipp64s tmp = 0;
	if (!pSrc)
		return ippStsNullPtrErr;
	if (len < 1)
		return ippStsSizeErr;

	for (i = 0; i < len; i++)
	{
		tmp += (Ipp64s)pSrc[i];
	}

	if (0 == scaleFactor)
	{
		*pSum = (Ipp32s)CLIP(tmp, MAX_32S, MIN_32S);
	}
	else if (scaleFactor > 0)
	{
		int sign;
		Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
		int scale = (scaleFactor -1);
		unsigned __int64 times = (unsigned __int64)1<<scale;
		int shift = 64-scaleFactor;
		if (tmp != 0)
		{
			sign = 1;
			if (tmp < 0)
			{
				sign = -1; tmp = -tmp;
			}
			if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
				tmp = (tmp)>>scaleFactor;
			else
				tmp = (tmp+rounding)>>scaleFactor;
			*pSum = (Ipp32s)CLIP((sign * tmp),MAX_32S, MIN_32S);
		}
		else
		{
			*pSum = 0;
		}
	}
	else
	{
		int scale = -scaleFactor;
		if (tmp>0 &&((MAX_32S >> scale) < tmp))
			*pSum = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
		else if (tmp< 0 &&(((-MIN_32S) >>scale) < (-tmp)))
			*pSum = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
		else
			*pSum = (Ipp32s)(tmp << scale);
	}

	return ippStsNoErr;

}

IppStatus __STDCALL ippsSum_32s_Sfs_c(const Ipp32s*  pSrc, int len, Ipp32s*  pSum, int scaleFactor)
{
	int i;
	Ipp64s tmp = 0;
	if (!pSrc)
		return ippStsNullPtrErr;
	if (len < 1)
		return ippStsSizeErr;

	for (i = 0; i < len; i++)
	{
		tmp += (Ipp64s)pSrc[i];
	}

	if (0 == scaleFactor)
	{
		*pSum = (Ipp32s)CLIP(tmp, MAX_32S, MIN_32S);
	}
	else if (scaleFactor > 0)
	{
		int sign;
		Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
		int scale = (scaleFactor -1);
		unsigned __int64 times = (unsigned __int64)1<<scale;
		int shift = 64-scaleFactor;
		if (tmp != 0)
		{
			sign = 1;
			if (tmp < 0)
			{
				sign = -1; tmp = -tmp;
			}
			if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
				tmp = (tmp)>>scaleFactor;
			else
				tmp = (tmp+rounding)>>scaleFactor;
			*pSum = (Ipp32s)CLIP((sign * tmp),MAX_32S, MIN_32S);
		}
		else
		{
			*pSum = 0;
		}
	}
	else
	{
		int scale = -scaleFactor;
		if (tmp>0 &&((MAX_32S >> scale) < tmp))
			*pSum = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
		else if (tmp< 0 &&(((-MIN_32S) >>scale) < (-tmp)))
			*pSum = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
		else
			*pSum = (Ipp32s)(tmp << scale);
	}

	return ippStsNoErr;

}

IppStatus __STDCALL ippsAdd_16s_Sfs_c(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor)
{
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
			pDst[i] = (Ipp16s)CLIP(tmp,MAX_16S,MIN_16S);
		}
	}
	else if (scaleFactor > 0 )
	{
		if (scaleFactor < 16)	/* 1 --> 31 */
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pDst[i] = (Ipp16s)(sign * tmp);
				}
				else
				{
					pDst[i] = 0;
				}
			}
		}
		else	/* right shift 16 bits, every 16bits integer will be 0, but MAX_16S+1 */
		{		
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 16)
					pDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrc1[i]+pSrc2[i];
					if (tmp -1 > MAX_16S)
						pDst[i] = 1;
					else if (-(tmp+1) > MAX_16S)
						pDst[i] = -1;
					else
						pDst[i] = 0;
				}	
			}
		}
	}
	else	
	{
		if (scaleFactor > -16)	/* -1 --> -15 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((Ipp64s)pSrc1[i]+pSrc2[i])<<scale;
				pDst[i] = (Ipp16s)CLIP(tmp,MAX_16S,MIN_16S);
			}
		}
		else					/* -16 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc1[i]+pSrc2[i];
				pDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
			}
		}
	}

	return ippStsNoErr;
}


IppStatus __STDCALL ippsAdd_16s_ISfs_c(const Ipp16s*  pSrc, Ipp16s*  pSrcDst, int len, int scaleFactor)
{
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
			pSrcDst[i] = (Ipp16s)CLIP(tmp,MAX_16S,MIN_16S);
		}
	}
	else if (scaleFactor > 0 )
	{
		if (scaleFactor < 16)	/* 1 --> 31 */
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pSrcDst[i] = (Ipp16s)(sign * tmp);
				}
				else
				{
					pSrcDst[i] = 0;
				}
			}
		}
		else	/* right shift 16 bits, every 16bits integer will be 0, but MAX_16S+1 */
		{		
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 16)
					pSrcDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrc[i]+pSrcDst[i];
					if (tmp -1 > MAX_16S)
						pSrcDst[i] = 1;
					else if (-(tmp+1) > MAX_16S)
						pSrcDst[i] = -1;
					else
						pSrcDst[i] = 0;
				}	
			}
		}
	}
	else	
	{
		if (scaleFactor > -16)	/* -1 --> -15 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((Ipp64s)pSrc[i]+pSrcDst[i])<<scale;
				pSrcDst[i] = (Ipp16s)CLIP(tmp,MAX_16S,MIN_16S);
			}
		}
		else					/* -16 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc[i]+pSrcDst[i];
				pSrcDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
			}
		}
	}

	return ippStsNoErr;
}


IppStatus __STDCALL ippsAddC_16s_Sfs_c(const Ipp16s*  pSrc, Ipp16s  val,Ipp16s*  pDst, int len, int scaleFactor)
{
	int i;
	Ipp64s tmp;
	if (!pSrc || ! pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;
	
	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (Ipp64s)pSrc[i]+val;
			pDst[i] = (Ipp16s)CLIP(tmp,MAX_16S,MIN_16S);
		}
	}
	else if (scaleFactor > 0 )
	{
		if (scaleFactor < 16)	/* 1 --> 31 */
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc[i]+val;
				if (tmp != 0)
				{
					sign = 1;
					if (tmp < 0)
					{
						sign = -1; tmp = -tmp;
					}
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pDst[i] = (Ipp16s)(sign * tmp);
				}
				else
				{
					pDst[i] = 0;
				}
			}
		}
		else	/* right shift 16 bits, every 16bits integer will be 0, but MAX_16S+1 */
		{		
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 16)
					pDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrc[i]+val;
					if (tmp -1 > MAX_16S)
						pDst[i] = 1;
					else if (-(tmp+1) > MAX_16S)
						pDst[i] = -1;
					else
						pDst[i] = 0;
				}	
			}
		}
	}
	else	
	{
		if (scaleFactor > -16)	/* -1 --> -15 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((Ipp64s)pSrc[i]+val)<<scale;
				pDst[i] = (Ipp16s)CLIP(tmp,MAX_16S,MIN_16S);
			}
		}
		else					/* -16 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc[i]+val;
				pDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
			}
		}
	}

	return ippStsNoErr;
}


IppStatus __STDCALL ippsSub_16s_c(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp16s*  pDst, int len)
{
	int i;
	Ipp32s tmp;
	if (!pSrc1 || !pSrc2 || !pDst)
		return ippStsNullPtrErr;
	if (len < 1)
		return ippStsSizeErr;

	for (i = 0; i < len; i++)
	{
		tmp = (Ipp32s)pSrc2[i] - pSrc1[i];
		pDst[i] = (Ipp16s)CLIP(tmp,MAX_16S,MIN_16S);
	}

	return ippStsNoErr;
}


IppStatus __STDCALL ippsSub_16s_ISfs_c(const Ipp16s*  pSrc, Ipp16s*  pSrcDst,int len, int scaleFactor)
{
	int i;
	Ipp64s tmp;
	if (!pSrc || ! pSrcDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;
	
	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (Ipp64s)pSrcDst[i] - pSrc[i];
			pSrcDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 16)	/* 1 -->16 */
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrcDst[i] - pSrc[i];
				if (tmp != 0)
				{
					sign = 1;
					if (tmp < 0)
					{
						sign = -1; tmp = -tmp;
					}
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pSrcDst[i] = (Ipp16s)(sign * tmp);
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
				if (scaleFactor > 16)
					pSrcDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrcDst[i] - pSrc[i];
					if (tmp -1 > MAX_16S)
						pSrcDst[i] = 1;
					else if (-(tmp+1) > MAX_16S)
						pSrcDst[i] = -1;
					else
						pSrcDst[i] = 0;
				}	
			}
		}
	}
	else	
	{
		if (scaleFactor > -16)	/* -1 --> -16 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((Ipp64s)pSrcDst[i] - pSrc[i])<<scale;
				pSrcDst[i] = (Ipp16s)CLIP(tmp,MAX_16S,MIN_16S);
			}
		}
		else					/* -16 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrcDst[i] - pSrc[i];
				pSrcDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
			}
		}
	}

	return ippStsNoErr;
}

IppStatus __STDCALL ippsSub_16s_Sfs_c(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor)
{
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
			pDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 16)	/* 1 -->16 */
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pDst[i] = (Ipp16s)(sign * tmp);
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
				if (scaleFactor > 16)
					pDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrc2[i] - pSrc1[i];
					if (tmp -1 > MAX_16S)
						pDst[i] = 1;
					else if (-(tmp+1) > MAX_16S)
						pDst[i] = -1;
					else
						pDst[i] = 0;
				}	
			}
		}
	}
	else	
	{
		if (scaleFactor > -16)	/* -1 --> -16 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((Ipp64s)pSrc2[i] - pSrc1[i])<<scale;
				pDst[i] = (Ipp16s)CLIP(tmp,MAX_16S,MIN_16S);
			}
		}
		else					/* -16 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc2[i] - pSrc1[i];
				pDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
			}
		}
	}

	return ippStsNoErr;
}



// Multiply
IppStatus __STDCALL ippsMul_16s_Sfs_c(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2,Ipp16s*  pDst, int len, int scaleFactor)
{
	int i;
	__int64 tmp;

	if (!pSrc1 || ! pSrc2 || ! pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (__int64)pSrc1[i]*pSrc2[i];
			pDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 32)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
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
		else
		{
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 32)
					pDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrc1[i]*pSrc2[i];
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
	else  /*scaleFactor < 0*/
	{
		if (scaleFactor > -16)	/* -1 --> -15 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((__int64)pSrc1[i]*pSrc2[i]);
				if (tmp>0 &&((MAX_16S >> scale) < tmp))
					pDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
				else if (tmp< 0 &&(((-MIN_16S) >>scale) < (-tmp)))
					pDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
				else
					pDst[i] = (Ipp16s)(tmp << scale);
			}
		}
		else					/* -16 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (__int64)pSrc1[i]*pSrc2[i];
				pDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
			}
		}
	}

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMul_16s_ISfs_c(const Ipp16s*  pSrc, Ipp16s*  pSrcDst,int len, int scaleFactor)
{
	int i;
	__int64 tmp;

	if (!pSrc || ! pSrcDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (__int64)pSrc[i]*pSrcDst[i];
			pSrcDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 32)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
						tmp = (tmp)>>scaleFactor;
					else
						tmp = (tmp+rounding)>>scaleFactor;
					pSrcDst[i] = (Ipp16s)CLIP((sign * tmp),MAX_16S, MIN_16S);
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
				if (scaleFactor > 32)
					pSrcDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrc[i]*pSrcDst[i];
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
	else  /*scaleFactor < 0*/
	{
		if (scaleFactor > -16)	/* -1 --> -15 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((__int64)pSrc[i]*pSrcDst[i]);
				if (tmp>0 &&((MAX_16S >> scale) < tmp))
					pSrcDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
				else if (tmp< 0 &&(((-MIN_16S) >>scale) < (-tmp)))
					pSrcDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
				else
					pSrcDst[i] = (Ipp16s)(tmp << scale);
			}
		}
		else					/* -16 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (__int64)pSrc[i]*pSrcDst[i];
				pSrcDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
			}
		}
	}

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMul_16s32s_Sfs_c(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp32s*  pDst, int len, int scaleFactor)
{
	int i;
	__int64 tmp;

	if (!pSrc1 || ! pSrc2 || ! pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (__int64)pSrc1[i]*pSrc2[i];
			pDst[i] = (Ipp32s)CLIP(tmp, MAX_32S, MIN_32S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 32)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
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
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
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
				if (scaleFactor > 32)
					pDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrc1[i]*pSrc2[i];
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
	else  /*scaleFactor < 0*/
	{
		if (scaleFactor > -32)	/* -1 --> -31 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((__int64)pSrc1[i]*pSrc2[i]);
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
				tmp = (__int64)pSrc1[i]*pSrc2[i];
				pDst[i] = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
			}
		}
	}

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMulC_16s_Sfs_c(const Ipp16s*  pSrc, Ipp16s  val,Ipp16s*  pDst, int len, int scaleFactor)
{
	int i;
	__int64 tmp;

	if (!pSrc || ! pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (__int64)pSrc[i]*val;
			pDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);
		}
	}
	else if (scaleFactor > 0)
	{
		if (scaleFactor < 32)
		{
			int sign;
			Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
			int scale = (scaleFactor -1);
			unsigned __int64 times = (unsigned __int64)1<<scale;
			int shift = 64-scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = (Ipp64s)pSrc[i]*val;
				if (tmp != 0)
				{
					sign = 1;
					if (tmp < 0)
					{
						sign = -1; tmp = -tmp;
					}
					if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
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
		else
		{
			for (i = 0; i < len; i++)
			{
				if (scaleFactor > 32)
					pDst[i] = 0;
				else
				{
					tmp = (Ipp64s)pSrc[i]*val;
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
	else  /*scaleFactor < 0*/
	{
		if (scaleFactor > -16)	/* -1 --> -15 */
		{
			int scale = -scaleFactor;
			for (i = 0; i < len; i++)
			{
				tmp = ((__int64)pSrc[i]*val);
				if (tmp>0 &&((MAX_16S >> scale) < tmp))
					pDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
				else if (tmp< 0 &&(((-MIN_16S) >>scale) < (-tmp)))
					pDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
				else
					pDst[i] = (Ipp16s)(tmp << scale);
			}
		}
		else					/* -16 --> neg infinite*/
		{
			for (i = 0; i < len; i++)
			{
				tmp = (__int64)pSrc[i]*val;
				pDst[i] = (Ipp16s)OVER(tmp,MAX_16S, MIN_16S);
			}
		}
	}

	return ippStsNoErr;
}

IppStatus __STDCALL ippsMul_16s_c(const Ipp16s*  pSrc1, const Ipp16s*  pSrc2, Ipp16s*  pDst, int len)
{
	int i;
	Ipp32s tmp;

	if (!pSrc1 || !pSrc2 || !pDst)
		return ippStsNullPtrErr;
	if (len < 1)
		return ippStsSizeErr;

	for (i = 0; i < len; i++)
	{
		tmp = (Ipp32s)pSrc1[i] * pSrc2[i];
		pDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);
	}

	return ippStsNoErr;
}


// Psrc2/pSrc1 = pDst
static __inline int isDIVbyZERO16(int *isMinus, __int64 *base, __int64 *div, const Ipp16s Src2, const Ipp16s Src1, Ipp16s *pDst, IppStatus *sts)   			
{
	*isMinus = 0;	
	if ((*div = (__int64)(Src2)) < 0)	
	{					
		*div = -(*div);			
		*isMinus = !(*isMinus);		
	}							
	if ((*base = (__int64)Src1) < 0)		
	{							
		*base = -(*base);			
		*isMinus = !(*isMinus);		
	}							
	if (*base == 0)				
	{							
		if (*div ==0)			
		{						
			*pDst = 0;		
		}						
		else if (*isMinus)
		{
			*pDst = (Ipp16s)MIN_16S;
		}
		else
		{
			*pDst = (Ipp16s)MAX_16S;
		}
		*sts = ippStsDivByZero;
		return 1;
	}
	return 0;
}
IppStatus __STDCALL ippsDiv_16s_Sfs_c(const Ipp16s* pSrc1, const Ipp16s* pSrc2,Ipp16s* pDst, int len, int ScaleFactor)
{
	int i;
	__int64 tmp;
	__int64 div, base, tmpI, tmpR;
	int isMinus;
	IppStatus sts = ippStsNoErr;

	if (!pSrc1 || !pSrc2 ||!pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (ScaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			if(isDIVbyZERO16(&isMinus, &base, &div, pSrc2[i], pSrc1[i],&pDst[i],&sts))
				continue;

			tmpI = div/base;
			tmpR = div - tmpI*base; 
			if ( ((tmpR<<1)>base) || ((tmpI&1) && ((tmpR<<1)==base)) )
				tmp = tmpI +1;
			else 
				tmp = tmpI;

			if (isMinus)
				pDst[i] = (Ipp16s)-tmp;
			else
				pDst[i] = (Ipp16s)tmp;
		}

	}
	else if (ScaleFactor > 0)
	{
		if (ScaleFactor < 16)
		{
			for (i = 0; i < len; i++)
			{
				if(isDIVbyZERO16(&isMinus, &base, &div, pSrc2[i], pSrc1[i],&pDst[i],&sts))
					continue;

				base <<= ScaleFactor;
				tmpI = div/base;
				tmpR = div-tmpI*base;
				if ( ((tmpR<<1)>base) || ((tmpI&1) && ((tmpR<<1)==base)) )
					tmp = tmpI +1;
				else 
					tmp = tmpI;

				if (isMinus)
					pDst[i] = (Ipp16s)-tmp;
				else
					pDst[i] = (Ipp16s)tmp;
			}
		}
		else
		{
			for (i = 0; i < len; i++)
			{
				if(isDIVbyZERO16(&isMinus, &base, &div, pSrc2[i], pSrc1[i],&pDst[i],&sts))
					continue;
				pDst[i] = 0;
			}
		}
	}
	else 
	{
		int scale = -ScaleFactor;
		for (i = 0; i < len; i++)
		{
			if(isDIVbyZERO16(&isMinus, &base, &div, pSrc2[i], pSrc1[i],&pDst[i],&sts))
				continue;

			if ((MAX_32S>>scale)>div)
			{
				div <<= scale;
				tmpI = div/base;
				tmpR = div-tmpI*base;
				if ( ((tmpR<<1)>base) || ((tmpI&1) && ((tmpR<<1)==base)) )
					tmp = tmpI +1;
				else 
					tmp = tmpI;

				if (isMinus)
					tmp = -tmp;

				pDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);

			}
			else
				pDst[i] = (Ipp16s)OVER(pSrc1[i],MAX_16S, MIN_16S);

		}
	}

	return sts;
}
IppStatus __STDCALL ippsDiv_16s_ISfs_c(const Ipp16s* pSrc, Ipp16s* pSrcDst,int len, int ScaleFactor)
{
	int i;
	__int64 tmp;
	__int64 div, base, tmpI, tmpR;
	int isMinus;
	IppStatus sts = ippStsNoErr;

	if (!pSrc || !pSrcDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (ScaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			if(isDIVbyZERO16(&isMinus, &base, &div,pSrcDst[i], pSrc[i],&pSrcDst[i],&sts))
				continue;

			tmpI = div/base;
			tmpR = div - tmpI*base; 
			if ( ((tmpR<<1)>base) || ((tmpI&1) && ((tmpR<<1)==base)) )
				tmp = tmpI +1;
			else 
				tmp = tmpI;

			if (isMinus)
				pSrcDst[i] = (Ipp16s)-tmp;
			else
				pSrcDst[i] = (Ipp16s)tmp;
		}

	}
	else if (ScaleFactor > 0)
	{
		if (ScaleFactor < 16)
		{
			for (i = 0; i < len; i++)
			{
				if(isDIVbyZERO16(&isMinus, &base, &div,pSrcDst[i], pSrc[i],&pSrcDst[i],&sts))
					continue;

				base <<= ScaleFactor;
				tmpI = div/base;
				tmpR = div-tmpI*base;
				if ( ((tmpR<<1)>base) || ((tmpI&1) && ((tmpR<<1)==base)) )
					tmp = tmpI +1;
				else 
					tmp = tmpI;

				if (isMinus)
					pSrcDst[i] = (Ipp16s)-tmp;
				else
					pSrcDst[i] = (Ipp16s)tmp;
			}
		}
		else
		{
			for (i = 0; i < len; i++)
			{
				if(isDIVbyZERO16(&isMinus, &base, &div, pSrcDst[i], pSrc[i],&pSrcDst[i],&sts))
					continue;
				pSrcDst[i] = 0;
			}
		}
	}
	else 
	{
		int scale = -ScaleFactor;
		for (i = 0; i < len; i++)
		{
			if(isDIVbyZERO16(&isMinus, &base, &div,pSrcDst[i], pSrc[i],&pSrcDst[i],&sts))
				continue;

			if ((MAX_32S>>scale)>div)
			{
				div <<= scale;
				tmpI = div/base;
				tmpR = div-tmpI*base;
				if ( ((tmpR<<1)>base) || ((tmpI&1) && ((tmpR<<1)==base)) )
					tmp = tmpI +1;
				else 
					tmp = tmpI;

				if (isMinus)
					tmp = -tmp;

				pSrcDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);
			}
			else
				pSrcDst[i] = (Ipp16s)OVER(pSrc[i],MAX_16S, MIN_16S);

		}
	}

	return sts;
}
IppStatus __STDCALL ippsDivC_16s_ISfs_c(Ipp16s val, Ipp16s* pSrcDst, int len, int ScaleFactor)
{
	int i;
	__int64 tmp;
	__int64 div, base, tmpI, tmpR;
	int isMinus;
	IppStatus sts = ippStsNoErr;

	if (!pSrcDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (ScaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			if(isDIVbyZERO16(&isMinus, &base, &div, pSrcDst[i], val,&pSrcDst[i],&sts))
				continue;

			tmpI = div/base;
			tmpR = div - tmpI*base; 
			if ( ((tmpR<<1)>base) || ((tmpI&1) && ((tmpR<<1)==base)) )
				tmp = tmpI +1;
			else 
				tmp = tmpI;

			if (isMinus)
				pSrcDst[i] = (Ipp16s)-tmp;
			else
				pSrcDst[i] = (Ipp16s)tmp;
		}

	}
	else if (ScaleFactor > 0)
	{
		if (ScaleFactor < 16)
		{
			for (i = 0; i < len; i++)
			{
				if(isDIVbyZERO16(&isMinus, &base, &div, pSrcDst[i], val,&pSrcDst[i],&sts))
					continue;

				base <<= ScaleFactor;
				tmpI = div/base;
				tmpR = div-tmpI*base;
				if ( ((tmpR<<1)>base) || ((tmpI&1) && ((tmpR<<1)==base)) )
					tmp = tmpI +1;
				else 
					tmp = tmpI;

				if (isMinus)
					pSrcDst[i] = (Ipp16s)-tmp;
				else
					pSrcDst[i] = (Ipp16s)tmp;
			}
		}
		else
		{
			for (i = 0; i < len; i++)
			{
				if(isDIVbyZERO16(&isMinus, &base, &div, pSrcDst[i],val,&pSrcDst[i],&sts))
					continue;
				pSrcDst[i] = 0;
			}
		}
	}
	else 
	{
		int scale = -ScaleFactor;
		for (i = 0; i < len; i++)
		{
			if(isDIVbyZERO16(&isMinus, &base, &div, pSrcDst[i],val,&pSrcDst[i],&sts))
				continue;

			if ((MAX_32S>>scale)>div)
			{
				div <<= scale;
				tmpI = div/base;
				tmpR = div-tmpI*base;
				if ( ((tmpR<<1)>base) || ((tmpI&1) && ((tmpR<<1)==base)) )
					tmp = tmpI +1;
				else 
					tmp = tmpI;

				if (isMinus)
					tmp = -tmp;

				pSrcDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);
			}
			else
				pSrcDst[i] = (Ipp16s)OVER(val,MAX_16S, MIN_16S);

		}
	}

	return sts;
}

// Other math
IppStatus __STDCALL ippsSpread_16s_Sfs_c(Ipp16s src1, Ipp16s src2, int inScaleFactor, Ipp16s* pDst)
{
	double tmpx, tmpy, tmpz, tmpr;
	double dsrc1, dsrc2;
	if (!pDst)
		return ippStsNullPtrErr;
	

	if (inScaleFactor == 0)
	{
		dsrc1 = src1;
		dsrc2 = src2;
	}
	else if (inScaleFactor > 0)
	{
		dsrc1 = src1 << inScaleFactor;
		dsrc2 = src2 << inScaleFactor;
	}
	else
	{
		int sf = 1 << (-inScaleFactor);
		dsrc1 = (double)src1 / (double)sf;
		dsrc2 = (double)src2 / (double)sf;
	}


	if (dsrc2 < dsrc1)
	{
		tmpx = 1.5 * (dsrc2-dsrc1);
	}
	else
	{
		tmpx = 3.0 * (dsrc2-dsrc1);
	}

	tmpz = 8 * MIN(pow((tmpx-0.5),2) - 2*(tmpx-0.5),0);

	tmpy = 15.811389 + 7.5*(tmpx + 0.474) - 17.5* sqrt(1.0+ pow((tmpx + 0.474),2));

	if (tmpy < -100)
	{
		tmpr = 0;
	}
	else
	{
		tmpr =  pow(10.0, (tmpz + tmpy)*0.1);
	}

	*pDst = (Ipp16s)CLIP((int)(tmpr * 32768), MAX_16S, MIN_16S);

	return ippStsNoErr;
}


IppStatus __STDCALL ippsPow34_16s_Sfs_c(const Ipp16s* pSrc, int inScaleFactor, Ipp16s* pDst, int ScaleFactor, int len)
{
	int i, tmp;
	double tmpSrc[1024];	//1024 is only for aac encoder, if this function used in other application, u maybe need modify this number

	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
	if (len < 1)
		return ippStsSizeErr;

	if (inScaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmpSrc[i] = (double)(abs((int)pSrc[i]));
		}
	}
	else if (inScaleFactor > 0)
	{
		for (i = 0; i < len; i++)
		{
			tmpSrc[i] = (double)(abs((int)pSrc[i]) << inScaleFactor);
		}
	}
	else
	{
		double inS = (double)(1<<(-inScaleFactor));
		for (i = 0; i < len; i++)
		{
			tmpSrc[i] = fabs((double)pSrc[i]) / inS;
		}
	}

	if (ScaleFactor == 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (int)(pow(tmpSrc[i],0.75) + 0.5);
			pDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);
		}
	}
	else if (ScaleFactor > 0)
	{
		for (i = 0; i < len; i++)
		{
			tmp = (int)(pow(tmpSrc[i],0.75) + 0.5);
			tmp = tmp >> ScaleFactor;
			pDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);
		}
	}
	else
	{
		int sf = 1 << (-ScaleFactor);
		for (i = 0; i < len; i++)
		{
			tmp = (int)(pow(tmpSrc[i],0.75) * sf + 0.5);
			pDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);
		}
	}
	return ippStsNoErr;
}
// scaleFactor == 1
IppStatus __STDCALL ippsMagnitude_16sc_Sfs_c(const Ipp16sc* pSrc,Ipp16s* pDst, int len,int scaleFactor)
{
	int i;
	__int64 *tmpD;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	tmpD = malloc(sizeof(__int64)*len);

	for (i = 0; i < len; i++)
	{
		tmpD[i] = (__int64)pSrc[i].re*pSrc[i].re + (__int64)pSrc[i].im * pSrc[i].im;
	}

	ippsSqrt_64s_ISfs_c(tmpD,len,scaleFactor);

	for (i = 0; i < len; i++)
	{
		pDst[i] = (Ipp16s)CLIP(tmpD[i],MAX_16S, MIN_16S);
	}

	free(tmpD);
	return ippStsNoErr;
}

// WWD: we need this function to process scaleFactor ==-3 only in AAC
IppStatus __STDCALL ippsMagnitude_16s_Sfs_c(const Ipp16s* pSrcRe,const Ipp16s* pSrcIm,Ipp16s* pDst,int len,int scaleFactor)
{
	int i;
	__int64 *tmpD;
	if (!pSrcRe || !pSrcIm || !pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	tmpD = malloc(sizeof(__int64)*len);

	for (i = 0; i < len; i++)
	{
		tmpD[i] = (__int64)pSrcRe[i]*pSrcRe[i] + (__int64)pSrcIm[i] * pSrcIm[i];
	}

	ippsSqrt_64s_ISfs_c(tmpD,len,scaleFactor);

	for (i = 0; i < len; i++)
	{
		pDst[i] = (Ipp16s)CLIP(tmpD[i],MAX_16S, MIN_16S);
	}

	free(tmpD);
	return ippStsNoErr;
}


IppStatus __STDCALL ippsRShiftC_32s_c(const Ipp32s* pSrc, int val, Ipp32s* pDst, int len)
{
	int i;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (val > 0)
	{
		for (i = 0; i < len; i++)
		{
			pDst[i] = pSrc[i] >> val;
		}
	}
	return ippStsNoErr;
}

IppStatus __STDCALL ippsRShiftC_16s_c(const Ipp16s* pSrc, int val, Ipp16s* pDst, int len)
{
	int i;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
	if (len <= 0)
		return ippStsSizeErr;

	if (val > 0)
	{
		for (i = 0; i < len; i++)
		{
			pDst[i] = pSrc[i] >> val;
		}
	}
	return ippStsNoErr;
}

IppStatus __STDCALL ippsLShiftC_16s_I_c(int val, Ipp16s* pSrcDst, int len)
{
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
	return ippStsNoErr;
}

//
IppStatus __STDCALL ippsDotProd_16s32s32s_Sfs_c( const Ipp16s* pSrc1, const Ipp32s* pSrc2,int len, Ipp32s* pDp, int scaleFactor )
{
	int i;
	__int64 tmp = 0;
	if (!pSrc1 || !pSrc2 || !pDp)
		return ippStsNullPtrErr;
	if (len < 1)
		return ippStsSizeErr;

	for (i = 0; i < len; i++)
	{
		tmp += (__int64)pSrc1[i] * pSrc2[i];
	}

	if (0 == scaleFactor)
	{
		*pDp = (Ipp32s)CLIP(tmp, MAX_32S, MIN_32S);
	}
	else if (scaleFactor > 0)
	{
		int sign;
		Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
		int scale = (scaleFactor -1);
		unsigned __int64 times = (unsigned __int64)1<<scale;
		int shift = 64-scaleFactor;
		if (tmp != 0)
		{
			sign = 1;
			if (tmp < 0)
			{
				sign = -1; tmp = -tmp;
			}
			if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0)) 
				tmp = (tmp)>>scaleFactor;
			else
				tmp = (tmp+rounding)>>scaleFactor;
			*pDp = (Ipp32s)CLIP((sign * tmp),MAX_32S, MIN_32S);
		}
		else
		{
			*pDp = 0;
		}
	}
	else
	{
		int scale = -scaleFactor;
		if (tmp>0 &&((MAX_32S >> scale) < tmp))
			*pDp = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
		else if (tmp< 0 &&(((-MIN_32S) >>scale) < (-tmp)))
			*pDp = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
		else
			*pDp = (Ipp32s)(tmp << scale);
	}
	return ippStsNoErr;
}

// Here scaleFactor is in range [0, 31]
IppStatus __STDCALL ippsDotProd_16s32s_Sfs_c( const Ipp16s* pSrc1, const Ipp16s* pSrc2, int len, Ipp32s* pDp, int scaleFactor )
{
	int i;
	__int64 tmp = 0;
	if (!pSrc1 || !pSrc2 || !pDp)
		return ippStsNullPtrErr;
	if (len < 1)
		return ippStsSizeErr;

	for (i = 0; i < len; i++)
	{
		tmp += (__int64)(pSrc1[i] * pSrc2[i]);
	}

	if(scaleFactor>31)
		scaleFactor = 31;
	else if(scaleFactor<-31)
		scaleFactor = -31;

	if (0 == scaleFactor)
	{
		*pDp = (Ipp32s)CLIP(tmp, MAX_32S, MIN_32S);
	}
	else if (scaleFactor > 0)
	{
		int sign;
		Ipp64s rounding = ((Ipp64s)1<<(scaleFactor-1));
		int scale = (scaleFactor -1);
		unsigned __int64 times = (unsigned __int64)1<<scale;
		int shift = 64-scaleFactor;
		if (tmp != 0)
		{
			sign = 1;
			if (tmp < 0)
			{
				sign = -1; tmp = -tmp;
			}
			if ((((unsigned __int64)tmp<<shift)>>shift) ==times && ((((tmp>>scale)-1)&0x3)==0))
				tmp = (tmp)>>scaleFactor;
			else
				tmp = (tmp+rounding)>>scaleFactor;
			*pDp = (Ipp32s)CLIP((sign * tmp),MAX_32S, MIN_32S);
		}
		else
		{
			*pDp = 0;
		}
	}
	else
	{
		int scale = -scaleFactor;
		if (tmp>0 &&((MAX_32S >> scale) < tmp))
			*pDp = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
		else if (tmp< 0 &&(((-MIN_32S) >>scale) < (-tmp)))
			*pDp = (Ipp32s)OVER(tmp,MAX_32S, MIN_32S);
		else
			*pDp = (Ipp32s)(tmp << scale);
	}

	return ippStsNoErr;
}


IppStatus __STDCALL ippsLn_32s16s_Sfs_c( const Ipp32s* pSrc, Ipp16s* pDst, int Len, int scaleFactor)
{
	int i, tmp;
	const double e = 2.718281828459045;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
	if (Len < 1)
		return ippStsSizeErr;

	if (scaleFactor == 0)
	{
		for (i = 0; i < Len; i++)
		{
			tmp = (int)(log((double)pSrc[i]) / log(e) + 0.5);
			pDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);		
		}
	}
	else if (scaleFactor > 0)
	{
		for (i = 0; i < Len; i++)
		{
			tmp = (int)(log((double)pSrc[i]) / log(e) + 0.5);
			tmp >>= scaleFactor;
			pDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);		
		}
	}
	else
	{
		int sf = 1 << (-scaleFactor);
		for (i = 0; i < Len; i++)
		{
			tmp = (int)(log((double)pSrc[i]) / log(e) * sf + 0.5);
			pDst[i] = (Ipp16s)CLIP(tmp, MAX_16S, MIN_16S);		
		}
	}

	return ippStsNoErr;

}
/*Other four functions */
IppStatus __STDCALL ippsDeinterleave_16s_c(const Ipp16s* pSrc, int ch_num,int len, Ipp16s** pDst)
{
	int i,j,tmp;
	if (!pSrc || !pDst)
		return ippStsNullPtrErr;
	if (len < 1 || ch_num < 1)
		return ippStsSizeErr;

	for (j = 0; j < len; j++)
	{
		tmp = j * ch_num;
		for (i = 0; i < ch_num; i++)
		{
			pDst[i][j] = pSrc[i+tmp];
		}
	}
	return ippStsNoErr;
}
