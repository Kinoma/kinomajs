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

/*--------------------------------------------look here firstly---------------/ 
	here are functions like Intel Ipp declared in Ippvc.h. totally 31 useful
-----------------------------------------------thank you---------------------*/

#define MP4_FLV_INTERNAL	1
#include "ippvc.h"
//***
#include "kinoma_ipp_lib.h"
#include "kinoma_ipp_common.h"
#include "kinoma_mpeg4_com_bitstream.h"

#ifdef PRINT_REF_INFO
#include "dump.h"
#endif

/* event structure for transform coeffs */
typedef struct 
{
  Ipp32s last, run, level, sign;
} Tcoef;


extern Ipp32s DCT3Dtab0[][2];
extern Ipp32s DCT3Dtab1[][2];
extern Ipp32s DCT3Dtab2[][2];

Tcoef VlcDecodeInterTCOEF_flv(Ipp8u **ppBitStream, int *pBitOffset, int short_video_header, int h263_flv)
{
	Ipp32u    code;
	Ipp32s  *tab;
	Tcoef   tcoef =	{0, 0, 0};

	/*code = k_mp4_ShowBits (ppBitStream,pBitOffset,12);*/
	code = k_mp4_ShowBits12 (ppBitStream,pBitOffset);

	if (code > 511)
		tab = DCT3Dtab0[(code >> 5) - 16];
	else if (code > 127)
		tab = DCT3Dtab1[(code >> 2) - 32];
	else if (code > 7)
		tab = DCT3Dtab2[(code >> 0) - 8];
	else
	{
		return tcoef;
	}

	k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);

	if (tab[0] != 7167)
	{
		tcoef.run = (tab[0] >> 4) & 255;
		tcoef.level = tab[0] & 15;
		tcoef.last = (tab[0] >> 12) & 1;
		/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1); */
		tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);
		return tcoef;
	}
	/*if (tab[0] == 7167) ESCAPE */
	if (short_video_header) 
	{              
		if( h263_flv > 1 )
		{
			int is11 = k_mp4_GetBit1(ppBitStream, pBitOffset);;

			tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);          /*  1 bit LAST   */
			tcoef.run  = k_mp4_GetBit6(ppBitStream, pBitOffset);        /*  6 bit RUN    */
			//i+= tcoef.run;                                                  /* go forward "run" digits */
			
			if( is11 )
			{
				tcoef.level = k_mp4_GetBit11(ppBitStream, pBitOffset);       /*  11 bit LEVEL  */
				tcoef.sign = tcoef.level & 1024;                          /* bit 8 is sign bit ==> sign = sign(level); */
				if(tcoef.sign)
				{
					tcoef.sign = 1;
					tcoef.level ^= 0x07ff;              /* == 1s complement */
					tcoef.level ++;                     /* 2s complement if level is negative ==> level = abs(level); */
				}
			}
			else
			{
				tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset,7);       /*  7 bit LEVEL  */
				tcoef.sign = tcoef.level & 64;                          /* bit 8 is sign bit ==> sign = sign(level); */
				if(tcoef.sign)
				{
					tcoef.sign = 1;
					tcoef.level ^= 0x007f;              /* == 1s complement */
					tcoef.level ++;                     /* 2s complement if level is negative ==> level = abs(level); */
				}
			}
		}
		else
		{

			/* escape mode 4 - H.263 type */
			/*tcoef.last = k_mp4_GetBits(ppBitStream, pBitOffset, 1); */
			tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);

			/*tcoef.run = k_mp4_GetBits(ppBitStream, pBitOffset, 6); */
			tcoef.run = k_mp4_GetBit6(ppBitStream, pBitOffset); 

			/*tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset, 8); */
			tcoef.level = k_mp4_GetBit8(ppBitStream, pBitOffset);

			if (tcoef.level == 0 || tcoef.level == 128) 
			{
	#ifdef _DEBUG
				printf ("Illegal LEVEL for ESCAPE mode 4: 0 or 128\n");
	#endif
				return tcoef;
			}

			if (tcoef.level > 127) 
			{ 
				tcoef.sign = 1; 
				tcoef.level = 256 - tcoef.level; 
			} 
			else 
			{ 
				tcoef.sign = 0; 
			}
		}
	}

#ifdef SUPPORT_H263_ONLY
	else return tcoef;//MP4_STATUS_FILE_ERROR;
#else	
	else 
	{   /* not escape mode 4 */
		int level_offset;
		/*level_offset = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
		level_offset = k_mp4_GetBit1(ppBitStream, pBitOffset);

		if (!level_offset) 
		{
			/* first escape mode. level is offset */
			/*code = k_mp4_ShowBits(ppBitStream,pBitOffset, 12);*/
			code = k_mp4_ShowBits12(ppBitStream,pBitOffset);

			if (code > 511)
				tab = DCT3Dtab0[(code >> 5) - 16];
			else if (code > 127)
				tab = DCT3Dtab1[(code >> 2) - 32];
			else if (code > 7)
				tab = DCT3Dtab2[(code >> 0) - 8];
			else
			{
				return tcoef;
			}

			k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);

			tcoef.run = (tab[0]>> 4) & 255;
			tcoef.level = tab[0] & 15;
			tcoef.last = (tab[0] >> 12) & 1;

			/* need to add back the max level */
			tcoef.level = tcoef.level + inter_max_level[tcoef.last][tcoef.run];

			/* sign bit */
			/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);       */
			tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);       

		}
		else
		{
			int run_offset;
			/*run_offset = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
			run_offset = k_mp4_GetBit1(ppBitStream, pBitOffset);

			if (!run_offset) 
			{
				/* second escape mode. run is offset */
				/*code = k_mp4_ShowBits (ppBitStream,pBitOffset, 12);*/
				code = k_mp4_ShowBits12 (ppBitStream,pBitOffset);

				if (code > 511)
					tab = DCT3Dtab0[(code >> 5) - 16];
				else if (code > 127)
					tab = DCT3Dtab1[(code >> 2) - 32];
				else if (code > 7)
					tab = DCT3Dtab2[(code >> 0) - 8];
				else
				{
					return tcoef;
				}

				k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);

				tcoef.run = (tab[0] >> 4) & 255;
				tcoef.level = tab[0] & 15;
				tcoef.last = (tab[0] >> 12) & 1;

				/* need to add back the max run */
				if (tcoef.last)
					tcoef.run = tcoef.run + inter_max_run1[tcoef.level]+1;
				else
					tcoef.run = tcoef.run + inter_max_run0[tcoef.level]+1;

				/* sign bit */
				/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);        */
				tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);        

			}
			else
			{
				/* third escape mode. flc */
				/*tcoef.last = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
				tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);

				/*tcoef.run = k_mp4_GetBits(ppBitStream, pBitOffset, 6);*/
				tcoef.run = k_mp4_GetBit6(ppBitStream, pBitOffset);

				k_mp4_FlushBits(ppBitStream, pBitOffset, 1);

				/*tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset, 12);*/
				tcoef.level = k_mp4_GetBit12(ppBitStream, pBitOffset);

				k_mp4_FlushBits(ppBitStream, pBitOffset, 1);

				if (tcoef.level > 2047)
				{
					tcoef.sign = 1;
					tcoef.level = 4096 - tcoef.level;
				}
				else
				{
					tcoef.sign = 0;
				}

			} /* flc */
		}
	}
#endif

	return tcoef;
}

/*----------------------------------------------------------------------------/
 12 functions more which are used in this decoder sample
----------------------------------------------------------------------------*/
IppStatus __STDCALL 
ippiReconstructCoeffsIntra_H263_1u16s_flv(
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     cbp,
  int     QP,
  int     advIntraFlag,
  int     scan,
  int     modQuantFlag,
  int	  h263_flv)
{
#ifndef __KINOMA_IPP__
	if( h263_flv <= 1 )
		return ippiReconstructCoeffsIntra_H263_1u16s
				(
					ppBitStream,
					pBitOffset,
					pCoef,
					pIndxLastNonZero,
					cbp,
					QP,
					advIntraFlag,
					scan,
					modQuantFlag
				);
#endif
	
	{
	Ipp32s              i;
	Tcoef               run_level;
	Ipp32u				DC_coeff;
	Ipp32s				tmpCoef;
	const Ipp32s		*pZigzag = zigzag[scan+1];
	const int			QP_1 = QP <<1;
	const int			QP_2 = QP & 1 ? QP : (QP - 1);
	int  min_quant_level = -2048;
	int  max_quant_level = 2047;

	//***_FLV_
	//if( h263_flv > 1 )
	//{
	//	min_quant_level = -1023;
	//	max_quant_level = 1023;
	//}

	if (!ppBitStream || !pBitOffset || !pCoef)
		return ippStsNullPtrErr;

	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;
	
	if (QP < 1 || QP > 31)
		return ippStsQPErr;

#ifdef PRINT_REF_INFO
		{
			dump("ippiReconstructCoeffsIntra_H263_1u16s_flv		with cbp(0,!0): ",cbp);
			dump("ippiReconstructCoeffsIntra_H263_1u16s_flv		with scan(0,1,2): ",scan);
		}
#endif

	/* in this Intel sample, 
	  mp4decvop.c line # 791  and  mp4dec.h line # 410 use this function
	  and the modQuantFlag, advIntraFlag are both set with 0 obviously.
	  I think modQuantFlag and advIntraFlag are used in H.263, not mpeg4
	  short video header mode*/
	if (modQuantFlag || advIntraFlag)
	{
#ifdef _DEBUG
		printf("modQuantFlag or advIntraFlag is not used in short video header mode of mpeg4\n");
#endif 
		return ippStsErr;
	}

	/*DC_coeff = k_mp4_GetBits(ppBitStream, pBitOffset, 8);*/
	DC_coeff = k_mp4_GetBit8(ppBitStream, pBitOffset);

	if (DC_coeff == 128 || DC_coeff == 0) 
	{
#ifdef _DEBUG
		printf ("short header: Illegal DC coeff: 0 or 128\n");
#endif
		*pIndxLastNonZero = -1;
		return ippStsVLCErr;
	}
	if (DC_coeff == 255)
		DC_coeff = 128;

	pCoef[0] = DC_coeff << 3;	/* DC_coeff * 8 */
	pCoef[0] = CLIP_R(pCoef[0],max_quant_level,min_quant_level);	/*	(1 << (8 + 3)); input parameter don't give us bitsperpixel, so we use 8 */

	if (0 == cbp)	/* only return the intra DC */
	{
		*pIndxLastNonZero = 0;
		return ippStsNoErr;
	}

	/* other intra TCoeffs */
	memset(pCoef+1,0,63*sizeof(Ipp16s));

	i = 1;
	do
	{
		run_level = VlcDecodeInterTCOEF_flv(ppBitStream,pBitOffset,1,h263_flv);

		if ((i+= run_level.run) > 63)
		{
#ifdef _DEBUG
			printf ("Too Much AC Coesffs in short header Intra block!\n");
#endif
			*pIndxLastNonZero = -1;
			return ippStsVLCErr;
		}

		/*	inverse scan & inverse quant */
		tmpCoef = QP_1 * run_level.level + QP_2;

		if (run_level.sign)
			tmpCoef = -tmpCoef;

		pCoef[pZigzag[i]] = CLIP_R(tmpCoef,max_quant_level,min_quant_level);	/*	(1 << (8 + 3)); input parameter don't give us bitsperpixel, so we use 8 */

		i++;
	}while (!run_level.last);

	*pIndxLastNonZero = i-1;

	return ippStsNoErr;
	}
}


IppStatus __STDCALL 
ippiReconstructCoeffsInter_H263_1u16s_flv(
  Ipp8u** ppBitStream,
  int*    pBitOffset,
  Ipp16s* pCoef,
  int*    pIndxLastNonZero,
  int     QP,
  int     modQuantFlag,
  int	  h263_flv)
{
#ifndef __KINOMA_IPP__
	if( h263_flv <= 1 )
		return ippiReconstructCoeffsInter_H263_1u16s
				(
					ppBitStream,
					pBitOffset,
					pCoef,
					pIndxLastNonZero,
					QP,
					modQuantFlag
					);
#endif

	{
	Ipp32s              i;
	Tcoef               run_level;
	Ipp32s				tmpCoef;
	const Ipp32s		*pZigzag = zigzag[1];
	const int			QP_1 = QP << 1;
	const int			QP_2 = (QP & 1) ? QP : (QP - 1);
	int  min_quant_level = -2048;
	int  max_quant_level = 2047;

	//_FLV_
	//***if( h263_flv > 1 )
	//{
	//	min_quant_level = -1023;
	//	max_quant_level = 1023;
	//}

	if (!ppBitStream || !pBitOffset || !pCoef)
		return ippStsNullPtrErr;

	if (*pBitOffset < 0 || *pBitOffset > 7)
		return ippStsBitOffsetErr;
	
	if (QP < 1 || QP > 31)
		return ippStsQPErr;

#ifdef PRINT_REF_INFO
	dump("ippiReconstructCoeffsInter_H263_1u16s_flv", -1);
#endif

	/* in this Intel sample, 
		mp4dec.h line #428 line #503 and mp4decvop.c line #816
	  the modQuantFlag is set with 0 obviously.
	  I think modQuantFlag are used in H.263, not mpeg4
	  short video header mode*/
	if (modQuantFlag)
	{
#ifdef _DEBUG
		printf("modQuantFlag is not used in short video header mode of mpeg4\n");
#endif 
		return ippStsErr;
	}

	memset(pCoef,0,64*sizeof(Ipp16s));

	i = 0;
	do
	{
		run_level = VlcDecodeInterTCOEF_flv(ppBitStream,pBitOffset,1, h263_flv);

		if ((i+= run_level.run) > 63)
		{
#ifdef _DEBUG
			printf ("Too Much AC Coesffs in short header Inter block!\n");
#endif
			*pIndxLastNonZero = -1;
			return ippStsVLCErr;
		}

		tmpCoef = QP_1 * run_level.level + QP_2;

		if (run_level.sign)
			tmpCoef = -tmpCoef;

		pCoef[pZigzag[i]] = CLIP_R(tmpCoef,max_quant_level,min_quant_level);	/*	(1 << (8 + 3)); input parameter don't give us bitsperpixel, so we use 8 */

		i++;
	}while (!run_level.last);

	*pIndxLastNonZero = i-1;

	return ippStsNoErr;
	}
}
