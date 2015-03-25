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
#ifdef __KINOMA_IPP__

#include <math.h>
#include <assert.h>

#include "ipps.h"
#include "ippdc.h"
#include "ippac.h"

#include "kinoma_aac_defines.h"

#ifndef NULL
#define NULL 0
#endif

IppStatus (__STDCALL *ippsThreshold_LT_16s_I_universal)			( Ipp16s* pSrcDst, int len,   Ipp16s level )=NULL;
IppStatus (__STDCALL *ippsConjPack_16sc_universal)				( const Ipp16s* pSrc, Ipp16sc* pDst, int lenDst )=NULL;
IppStatus (__STDCALL *ippsConjCcs_16sc_universal)				( const Ipp16s* pSrc, Ipp16sc* pDst, int lenDst )=NULL;


IppStatus __STDCALL ippsThreshold_LT_16s_I_c( Ipp16s* pSrcDst, int len,   Ipp16s level )
{
	int i;
	for(i=0; i<len; i++)
	{
		if(pSrcDst[i] < level)
			pSrcDst[i] = level;

	}
	return ippStsNoErr;

}

IppStatus __STDCALL ippsConjPack_16sc_c( const Ipp32s* pSrc, Ipp32sc* pDst, int lenDst )
{
	int i,c=0;

	pDst[0].re = pSrc[c++]; 
	pDst[0].im = 0;
	for(i=1; i<lenDst/2; i++)
	{
		pDst[i].re = pSrc[c++]; 
		pDst[i].im = pSrc[c++];
	}
	if (lenDst%2==0)
	{
		pDst[lenDst/2].re = pSrc[c];
		pDst[lenDst/2].im = 0;

		for (i = i+1; i < lenDst; i++)
		{
			pDst[i].im = -pSrc[--c];
			pDst[i].re = pSrc[--c];
		}
	}
	else
	{
		pDst[lenDst/2].re = pSrc[c++]; 
		pDst[lenDst/2].im =  pSrc[c];

		for (i = i+1; i < lenDst; i++)
		{
			pDst[i].im = -pSrc[c--];
			pDst[i].re = pSrc[c--];
		}
	}

	return ippStsNoErr;
}
IppStatus __STDCALL ippsConjCcs_16sc_c( const Ipp16s* pSrc, Ipp16sc* pDst, int lenDst )
{
	int i,c=0;

	for (i = 0; i <= lenDst/2; i++)
	{
		pDst[i].re = pSrc[c++];
		pDst[i].im = pSrc[c++];
	}
	if (lenDst%2==0)
	{
		c = c-2;
		for (i = i; i < lenDst; i++)
		{
			pDst[i].im = -pSrc[--c];
			pDst[i].re = pSrc[--c];
		}
	}
	else
	{
		c = c-1;
		for (; i < lenDst; i++)
		{
			pDst[i].im = -pSrc[c--];
			pDst[i].re = pSrc[c--];
		}
	}


	return ippStsNoErr;
}
#endif
