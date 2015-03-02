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

IppStatus (__STDCALL *ippiQuantInvInterInit_MPEG4_universal)	( const Ipp8u* pQuantMatrix, IppiQuantInvInterSpec_MPEG4* pSpec, int bitsPerPixel)=NULL;
IppStatus (__STDCALL *ippiQuantInvInterGetSize_MPEG4_universal) ( int* pSpecSize)=NULL;
IppStatus (__STDCALL *ippiQuantInvIntraInit_MPEG4_universal)	(const Ipp8u* pQuantMatrix, IppiQuantInvIntraSpec_MPEG4* pSpec, int bitsPerPixel)=NULL;
IppStatus (__STDCALL *ippiQuantInvIntraGetSize_MPEG4_universal) (int* pSpecSize)=NULL;
IppStatus (__STDCALL *ippiQuantInvIntra_MPEG4_16s_C1I_universal)			(Ipp16s* pCoeffs,int indxLastNonZero,const IppiQuantInvIntraSpec_MPEG4* pSpec, int QP,int blockType)=NULL;
IppStatus (__STDCALL *ippiQuantIntraInit_MPEG4_universal)			(const Ipp8u*  pQuantMatrix, IppiQuantIntraSpec_MPEG4* pSpec, int bitsPerPixel)=NULL;
IppStatus (__STDCALL *ippiQuantInterInit_MPEG4_universal) 			(const Ipp8u*  pQuantMatrix, IppiQuantInterSpec_MPEG4* pSpec, int bitsPerPixel)=NULL;
IppStatus (__STDCALL *ippiQuantInterGetSize_MPEG4_universal) 		(int* pSpecSize)=NULL;
IppStatus (__STDCALL *ippiQuantIntraGetSize_MPEG4_universal) 		(int* pSpecSize)=NULL;
IppStatus (__STDCALL *ippiQuantIntra_H263_16s_C1I_universal) 		(Ipp16s* pSrcDst,   int  QP,  int*  pCountNonZero,	int   advIntraFlag,  int   modQuantFlag)=NULL;
IppStatus (__STDCALL *ippiQuantIntra_MPEG4_16s_C1I_universal) 		(Ipp16s*  pCoeffs, const IppiQuantIntraSpec_MPEG4* pSpec, int  QP, int* pCountNonZero, int blockType)=NULL;
IppStatus (__STDCALL *ippiQuantInter_H263_16s_C1I_universal) 		(Ipp16s* pSrcDst, int QP, int* pCountNonZero, int modQuantFlag)=NULL;
IppStatus (__STDCALL *ippiQuantInter_MPEG4_16s_C1I_universal) 		(Ipp16s* pCoeffs, const IppiQuantInterSpec_MPEG4* pSpec, int  QP,  int* pCountNonZero)=NULL;
IppStatus (__STDCALL *ippiQuantInvIntra_H263_16s_C1I_universal) 		(Ipp16s* pSrcDst, int indxLastNonZero, int QP, int     advIntraFlag,int modQuantFlag)=NULL;
IppStatus (__STDCALL *ippiQuantInvInter_H263_16s_C1I_universal) 		(Ipp16s* pSrcDst, int indxLastNonZero, int QP, int modQuantFlag)=NULL;
IppStatus (__STDCALL *ippiQuantInvInter_MPEG4_16s_C1I_universal) 	(Ipp16s*   pCoeffs, int  indxLastNonZero, const IppiQuantInvInterSpec_MPEG4* pSpec, int QP)=NULL;


#define KNM_QUANT
#define KNM_INVQUANT

IppStatus __STDCALL ippiQuantIntraInit_MPEG4_c(
	const Ipp8u*  pQuantMatrix,IppiQuantIntraSpec_MPEG4* pSpec,int bitsPerPixel)
{
#ifdef KNM_QUANT
	k_IppiQuantIntraSpec_MPEG4 * pQuant = (k_IppiQuantIntraSpec_MPEG4*)pSpec;
	/* pQuantMatrix can be NULL*/
	if (!pSpec)
		return ippStsNullPtrErr;
	if (bitsPerPixel < 4 || bitsPerPixel > 12)
		return ippStsOutOfRangeErr;

	if (NULL != pQuantMatrix)
	{
		pQuant->bUseMat = 1;
		memcpy(pQuant->matrix, pQuantMatrix,64);
	}
	else 
		pQuant->bUseMat = 0;

#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantIntraInit_MPEG4",-1);
#endif

#ifdef ITL_QUANT
	{
		IppStatus sts;
		sts = ippiQuantIntraInit_MPEG4(pQuantMatrix, pSpec, bitsPerPixel);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippiQuantIntraGetSize_MPEG4_c(int* pSpecSize)
{
#ifdef KNM_QUANT
	if (!pSpecSize)
		return ippStsNullPtrErr;
	*pSpecSize = sizeof(k_IppiQuantIntraSpec_MPEG4);
#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantIntraGetSize_MPEG4",-1);
#endif

#ifdef ITL_QUANT
	{
		IppStatus sts;
		sts = ippiQuantIntraGetSize_MPEG4(pSpecSize);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippiQuantIntra_MPEG4_16s_C1I_c(
	Ipp16s*  pCoeffs, const IppiQuantIntraSpec_MPEG4* pSpec,
	int  QP, int* pCountNonZero, int blockType)
{
#ifdef KNM_QUANT
	int i,dc_scaler, countZero = 0;
	int ac1, ac2, ac3;
	Ipp16s	tmpCoeff;

	k_IppiQuantIntraSpec_MPEG4 *pQuant = (k_IppiQuantIntraSpec_MPEG4*) pSpec;

	if (!pCoeffs || !pSpec)
		return ippStsNullPtrErr;

	if (QP < 1)
		return ippStsQPErr;

	/* Intra DC */
	if (IPPVC_BLOCK_LUMA == blockType)
	{
		if (QP > 0 && QP < 5) dc_scaler = 8;
		else if (QP < 9) dc_scaler = QP << 1; /* 2* QP */
		else if (QP < 25) dc_scaler = QP + 8;
		else dc_scaler = (QP << 1) - 16;	/* 2 * QP - 16; */
	}
	else
	{
		if (QP > 0 && QP < 5) dc_scaler = 8;
		else if (QP < 25) dc_scaler = (QP + 13) >> 1;	/*((QP + 13) / 2;)*/
		else dc_scaler = QP - 6;
	} 
	tmpCoeff = (pCoeffs[0] > 0) ? (pCoeffs[0] + dc_scaler/2)/dc_scaler : (pCoeffs[0] - dc_scaler/2) /dc_scaler;
	pCoeffs[0] = CLIP_R(tmpCoeff,254,1);
	/* DC always not be 0 */

	if (pQuant->bUseMat)	 /*1 == quant_type*/
	{
		for (i = 1; i < 64; i++)
		{
			ac1 = ((pCoeffs[i]<<4) + ((pCoeffs[i]>0) ? 1 : (-1))*pQuant->matrix[i]/2) / pQuant->matrix[i];
			ac2 = CLIP_R(ac1,2047,-2048);
			ac3 = ( ac2 + SIGN0(ac2)*((3*QP + 2)>>2) )/(QP<<1);	
			if (!ac3)
				countZero++;
			pCoeffs[i] =  CLIP_R(ac3,2047,-2048);
		}
	}
	else	/*0 == quant_type */
	{
		for (i = 1; i < 64; i++)
		{
			tmpCoeff = (abs(pCoeffs[i]) / (QP<<1) ) * SIGN(pCoeffs[i]);
			if (!tmpCoeff)
				countZero++;
			pCoeffs[i] = CLIP_R(tmpCoeff,2047,-2048);
		}
	}

	*pCountNonZero = 64 - countZero;
#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantIntra_MPEG4_16s_C1I",-1);
#endif

#ifdef ITL_QUANT
	{
		IppStatus sts;
		sts = ippiQuantIntra_MPEG4_16s_C1I(pCoeffs, pSpec, QP, pCountNonZero,blockType);
		return sts;
	}
#endif

	return ippStsNoErr;
}


IppStatus __STDCALL ippiQuantInterInit_MPEG4_c(
	const Ipp8u*  pQuantMatrix, IppiQuantInterSpec_MPEG4* pSpec,  int bitsPerPixel)
{
#ifdef KNM_QUANT
	k_IppiQuantInterSpec_MPEG4 * pQuant = (k_IppiQuantInterSpec_MPEG4*)pSpec;
	/* pQuantMatrix can be NULL*/
	if (!pSpec)
		return ippStsNullPtrErr;
	if (bitsPerPixel < 4 || bitsPerPixel > 12)
		return ippStsOutOfRangeErr;

	if (NULL != pQuantMatrix)
	{
		pQuant->bUseMat = 1;
		memcpy(pQuant->matrix,pQuantMatrix,64);
	}
	else 
		pQuant->bUseMat = 0;
#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantInterInit_MPEG4",-1);
#endif

#ifdef ITL_QUANT
	{
		IppStatus sts;
		sts = ippiQuantInterInit_MPEG4(pQuantMatrix, pSpec, bitsPerPixel);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippiQuantInterGetSize_MPEG4_c(int* pSpecSize)
{
#ifdef KNM_QUANT
	if (!pSpecSize)
		return ippStsNullPtrErr;
	*pSpecSize = sizeof(k_IppiQuantInterSpec_MPEG4);
#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantInterGetSize_MPEG4",-1);
#endif

#ifdef ITL_QUANT
	{
		IppStatus sts;
		sts = ippiQuantInterGetSize_MPEG4(pSpecSize);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippiQuantInter_MPEG4_16s_C1I_c(
	Ipp16s* pCoeffs, const IppiQuantInterSpec_MPEG4* pSpec,
	int  QP,  int* pCountNonZero)
{
#ifdef KNM_QUANT
	int ac1, ac2;
	int i,countZero = 0;
	Ipp16s	tmpCoeff;
	k_IppiQuantInterSpec_MPEG4 * pQuant = (k_IppiQuantInterSpec_MPEG4*)pSpec;

	if (!pCoeffs || !pSpec)
		return ippStsNullPtrErr;

	if (QP < 1 )
		return ippStsQPErr;

	if (pQuant->bUseMat)	/*1 == quant_type */
	{
		//wait for test
		for (i = 0; i < 64; i++)
		{
			ac1 = ((pCoeffs[i]<<4) + ((pCoeffs[i]>0) ? 1 : (-1))*pQuant->matrix[i]/2) / pQuant->matrix[i]; 
			ac2 = ac1/(QP<<1);
			if (!ac2)
				countZero++;	/*	countZero += !ac2;*/
			pCoeffs[i] =  CLIP_R(ac2,2047,-2048);	
		}
	}
	else		/*0 == quant_type */
	{
		for (i = 0; i < 64; i++)
		{
			tmpCoeff = ((abs(pCoeffs[i])-(QP>>1)) / (QP<<1) ) * SIGN(pCoeffs[i]);
			if (!tmpCoeff)
				countZero++;	/* countZero += !tmpCoeff; */
			pCoeffs[i] = CLIP_R(tmpCoeff,2047,-2048);
		}
	}
	*pCountNonZero = 64 - countZero;
#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantInter_MPEG4_16s_C1I",-1);
#endif

#ifdef ITL_QUANT
	{
		IppStatus sts;
		sts = ippiQuantInter_MPEG4_16s_C1I(pCoeffs, pSpec, QP, pCountNonZero);
		return sts;
	}
#endif

	return ippStsNoErr;
}


/*
QuantInvIntraInit_MPEG4,
QuantInvInterInit_MPEG4
Initialize specification structures.
*/
IppStatus __STDCALL 
ippiQuantInvIntraInit_MPEG4_c(								
							   const Ipp8u*                 pQuantMatrix,
							   IppiQuantInvIntraSpec_MPEG4* pSpec,
							   int                          bitsPerPixel)
{
#ifdef KNM_INVQUANT
	k_IppiQuantInvIntraSpec_MPEG4 * pQuant = (k_IppiQuantInvIntraSpec_MPEG4*)pSpec;
	/* pQuantMatrix can be NULL*/
	if (!pSpec)
		return ippStsNullPtrErr;
	if (bitsPerPixel < 4 || bitsPerPixel > 12)
		return ippStsOutOfRangeErr;

	pQuant->quant_max = (1<<(bitsPerPixel-3)) - 1;
	pQuant->min = (1 << (bitsPerPixel+3));
	pQuant->max = pQuant->min - 1;
	pQuant->min = -pQuant->min;

	if (NULL != pQuantMatrix)
	{
		pQuant->bUseMat = 1;
		memcpy(pQuant->matrix, pQuantMatrix,64);
#ifdef PRINT_REF_INFO
		printf("the Intra quant matrix is:\n");
		//print(pQuantMatrix,8,8,8);
#endif
	}
	else 
		pQuant->bUseMat = 0;
#endif


#ifdef PRINT_REF_INFO
	dump("ippiQuantInvIntraInit_MPEG4",-1);
#endif

#ifdef ITL_INVQUANT
	{
		IppStatus sts;
		sts = ippiQuantInvIntraInit_MPEG4(pQuantMatrix,pSpec,bitsPerPixel);
		return sts;
	}
#endif
	return ippStsNoErr;

}



/*
QuantInvIntraGetSize_MPEG4,
QuantInvInterGetSize_MPEG4
Return size of specification structures.
*/
IppStatus __STDCALL 
ippiQuantInvIntraGetSize_MPEG4_c(								
								  int* pSpecSize)
{
#ifdef KNM_INVQUANT
	if (!pSpecSize)
		return ippStsNullPtrErr;
	*pSpecSize = sizeof(k_IppiQuantInvIntraSpec_MPEG4);
#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantInvIntraGetSize_MPEG4",-1);
#endif

#ifdef ITL_INVQUANT
	{
		IppStatus sts;
		sts = ippiQuantInvIntraGetSize_MPEG4(pSpecSize);
		return sts;
	}
#endif
	return ippStsNoErr;
}


IppStatus __STDCALL 
ippiQuantInvIntra_MPEG4_16s_C1I_c(								
								   Ipp16s*                            pCoeffs,
								   int                                indxLastNonZero,
								   const IppiQuantInvIntraSpec_MPEG4* pSpec,
								   int                                QP,
								   int                                blockType)
{
#ifdef KNM_INVQUANT
	int j,sum, dc_scaler;
	const int			QP_1 = QP <<1;
	const int			QP_2 = QP & 1 ? QP : (QP - 1);
	Ipp32s				tmpCoeff;

	k_IppiQuantInvIntraSpec_MPEG4 *pQuant = (k_IppiQuantInvIntraSpec_MPEG4*) pSpec;

	if (!pCoeffs || !pSpec)
		return ippStsNullPtrErr;

	if (QP < 1 || QP > pQuant->quant_max)
		return ippStsQPErr;

	/* Intra DC */
	if (IPPVC_BLOCK_LUMA == blockType)
	{
		if (QP > 0 && QP < 5) dc_scaler = 8;
		else if (QP < 9) dc_scaler = QP << 1; /* 2* QP */
		else if (QP < 25) dc_scaler = QP + 8;
		else dc_scaler = (QP << 1) - 16;	/* 2 * QP - 16; */
	}
	else
	{
		if (QP > 0 && QP < 5) dc_scaler = 8;
		else if (QP < 25) dc_scaler = (QP + 13) >> 1;	/*((QP + 13) / 2;)*/
		else dc_scaler = QP - 6;
	} 
	pCoeffs[0] *= dc_scaler;
	/* Intra DC Inverse quantization */

	if (indxLastNonZero < 1 || indxLastNonZero > 63)
		indxLastNonZero = 63;

	if (pQuant->bUseMat)	 /*1 == quant_type*/
	{
		sum=0;
		for (j = 1; j < indxLastNonZero+1; j++)
		{
			if (0 != pCoeffs[j])
			{
				pCoeffs[j] = ((pCoeffs[j]) * pQuant->matrix[j] * QP)/8; 
				//pCoeffs[j] = ((2* pCoeffs[j]) * pQuant->matrix[j] * QP)/16; 

				pCoeffs[j] = CLIP_R(pCoeffs[j],pQuant->max,pQuant->min);
				sum += pCoeffs[j];
			}/* if pCoeff is 0, we need do Nothing.*/
		}

		/*mismatch control*/
		sum += pCoeffs[0];
		if ((sum & 0x1) == 0) 
			pCoeffs[63] += ((pCoeffs[63] & 0x1)? -1:1); 
	}
	else	/*0 == quant_type */
	{
		for (j = 1; j < indxLastNonZero+1; j++)
		{
			if (pCoeffs[j] < 0) 
			{
				tmpCoeff = pCoeffs[j]*QP_1 - QP_2;
				pCoeffs[j] = (tmpCoeff < pQuant->min ? pQuant->min : tmpCoeff );
			} else if (pCoeffs[j] > 0)
			{
				tmpCoeff = pCoeffs[j]*QP_1 + QP_2;
				pCoeffs[j] = (tmpCoeff > pQuant->max ? pQuant->max : tmpCoeff );
			}		 /* if pCoeff is 0, we need do Nothing.*/
		}
	}
#endif 

#ifdef PRINT_REF_INFO
	{
		dump("ippiQuantInvIntra_MPEG4_16s_C1I		 with blockType(0,1): ",blockType);
		//		dump("ippiQuantInvIntra_MPEG4_16s_C1I		 with quant_type(0,1): ",pQuant->bUseMat);
	}
#endif

#ifdef ITL_INVQUANT
	{
		IppStatus sts;
		sts = ippiQuantInvIntra_MPEG4_16s_C1I( pCoeffs,indxLastNonZero,pSpec,QP,blockType);
		return sts;
	}
#endif

	return ippStsNoErr;
}



IppStatus __STDCALL 
ippiQuantInvInterInit_MPEG4_c(								
							   const Ipp8u*                 pQuantMatrix,
							   IppiQuantInvInterSpec_MPEG4* pSpec,
							   int                          bitsPerPixel)
{
#ifdef KNM_INVQUANT
	k_IppiQuantInvInterSpec_MPEG4 * pQuant = (k_IppiQuantInvInterSpec_MPEG4*)pSpec;
	/* pQuantMatrix can be NULL*/
	if (!pSpec)
		return ippStsNullPtrErr;
	if (bitsPerPixel < 4 || bitsPerPixel > 12)
		return ippStsOutOfRangeErr;

	pQuant->quant_max = (1<<(bitsPerPixel-3)) - 1;
	pQuant->min = (1 << (bitsPerPixel+3));
	pQuant->max =  pQuant->min -1;
	pQuant->min = -pQuant->min;

	if (NULL != pQuantMatrix)
	{
		pQuant->bUseMat = 1;
		memcpy(pQuant->matrix,pQuantMatrix,64);

#ifdef PRINT_REF_INFO
		printf("the Inter quant matrix is:\n");
		//		print(pQuantMatrix,8,8,8);
#endif
	}
	else 
		pQuant->bUseMat = 0;
#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantInvInterInit_MPEG4",-1);
#endif

#ifdef ITL_INVQUANT
	{
		IppStatus sts;
		sts = ippiQuantInvInterInit_MPEG4(pQuantMatrix,pSpec,bitsPerPixel);
		return sts;
	}
#endif
	return ippStsNoErr;
}


IppStatus __STDCALL 
ippiQuantInvInterGetSize_MPEG4_c(								
								  int* pSpecSize)
{
#ifdef KNM_INVQUANT
	if (!pSpecSize)
		return ippStsNullPtrErr;
	*pSpecSize = sizeof(k_IppiQuantInvInterSpec_MPEG4);
#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantInvInterGetSize_MPEG4",-1);
#endif

#ifdef ITL_INVQUANT
	{
		IppStatus sts;
		sts = ippiQuantInvInterGetSize_MPEG4(pSpecSize);
		return sts;
	}
#endif
	return ippStsNoErr;
}

IppStatus __STDCALL ippiQuantInvInter_MPEG4_16s_C1I_c(
	Ipp16s*   pCoeffs,    int  indxLastNonZero,
	const IppiQuantInvInterSpec_MPEG4* pSpec,   int  QP)
{
#ifdef KNM_INVQUANT
	int i,sum = 0;
	Ipp32s	tmpCoef;

	const int	QP_1 = QP <<1;
	const int	QP_2 = QP & 1 ? QP : (QP - 1);

	k_IppiQuantInvInterSpec_MPEG4 * pQuant = (k_IppiQuantInvInterSpec_MPEG4*)pSpec;

	if (!pCoeffs || !pSpec)
		return ippStsNullPtrErr;

	if (QP < 1 || QP > pQuant->quant_max)
		return ippStsQPErr;

	if (indxLastNonZero < 1 || indxLastNonZero > 63)
		indxLastNonZero = 63;

	if (pQuant->bUseMat)	/*1 == quant_type */
	{
		for (i = 0; i < indxLastNonZero+1; i++)
		{
			if (pCoeffs[i] != 0)
			{
				tmpCoef = (((pCoeffs[i]*2) + SIGN(pCoeffs[i])) * pQuant->matrix[i] * QP) / 16;	/* here can not right shift 4*/
				tmpCoef = CLIP_R(tmpCoef,pQuant->max,pQuant->min);
				sum += tmpCoef;
				pCoeffs[i] = tmpCoef;
			}
		}
		/*mismatch control*/
		if ((sum & 0x1) == 0) 
			pCoeffs[63] += ((pCoeffs[63] & 0x1)? -1:1); 

	}
	else		/*0 == quant_type */
	{
		for (i = 0; i < indxLastNonZero+1; i++)
		{
			if (pCoeffs[i] < 0) 
			{
				tmpCoef = pCoeffs[i]*QP_1 - QP_2;
				pCoeffs[i] = (tmpCoef < pQuant->min ? pQuant->min : tmpCoef);
			} else if (pCoeffs[i] > 0)
			{
				tmpCoef = pCoeffs[i]*QP_1 + QP_2;
				pCoeffs[i] = (tmpCoef > pQuant->max ? pQuant->max : tmpCoef);
			}	
		}
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantInvInter_MPEG4_16s_C1I",-1);
#endif

#ifdef ITL_INVQUANT
	{
		IppStatus sts;
		sts = ippiQuantInvInter_MPEG4_16s_C1I(pCoeffs,indxLastNonZero,pSpec, QP);
		return sts;
	}
#endif

	return ippStsNoErr;
}



IppStatus __STDCALL ippiQuantIntra_H263_16s_C1I_c(
	Ipp16s* pSrcDst,   int  QP,  int*  pCountNonZero,
	int   advIntraFlag,  int   modQuantFlag)
{
#if 1
	int i,countZero = 0;
	Ipp16s	tmpCoeff;

	if (!pSrcDst)
		return ippStsNullPtrErr;

	if (QP < 1 || QP > 31)
		return ippStsQPErr;

	/* in this Intel sample, 
	the modQuantFlag, advIntraFlag are both set with 0 obviously.
	I think modQuantFlag and advIntraFlag are used in H.263 only, not mpeg4
	short video header mode*/
#ifdef _DEBUG
	if (modQuantFlag || advIntraFlag)
	{
		printf("modQuantFlag or advIntraFlag is not used in short video header mode of mpeg4\n");
		return ippStsErr;
	}
#endif 

	/* Intra DC */
	tmpCoeff = (pSrcDst[0] > 0) ? (pSrcDst[0]+4) >> 3 : (pSrcDst[0]-4) >> 3;
	pSrcDst[0] = CLIP_R(tmpCoeff,254,1);
	/*	DC always not be 0*/

	for (i = 1; i < 64; i++)
	{
		tmpCoeff = (abs(pSrcDst[i]) / (2 *QP) ) * SIGN(pSrcDst[i]);
		if (!tmpCoeff)
			countZero++;		/*countZero += !tmpCoeff;*/
		pSrcDst[i] = CLIP_R(tmpCoeff,127,-127);

	}

	*pCountNonZero = 64 - countZero;

#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantIntra_H263_16s_C1I",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiQuantIntra_H263_16s_C1I(pSrcDst, QP, pCountNonZero, advIntraFlag, modQuantFlag);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippiQuantInter_H263_16s_C1I_c(
	Ipp16s* pSrcDst, int     QP,
	int*    pCountNonZero, int     modQuantFlag)
{
#if 1
	int i,countZero = 0;
	Ipp16s	tmpCoeff;

	if (!pSrcDst)
		return ippStsNullPtrErr;

	if (QP < 1 || QP > 31)
		return ippStsQPErr;

	/* in this Intel sample, 
	the modQuantFlag was set with 0 obviously.
	I think modQuantFlag is used in H.263 only, not mpeg4
	short video header mode*/
#ifdef _DEBUG
	if (modQuantFlag)
	{
		printf("modQuantFlag is not used in short video header mode of mpeg4\n");
		return ippStsErr;
	}
#endif 

	for (i = 0; i < 64; i++)
	{
		tmpCoeff = ((abs(pSrcDst[i])-QP/2) / (2 *QP) ) * SIGN(pSrcDst[i]);
		if (!tmpCoeff)
			countZero++;		/*  countZero += !tmpCoeff;  this is not good*/	
		pSrcDst[i] = CLIP_R(tmpCoeff,127,-127);
	}
	*pCountNonZero = 64 - countZero;

#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantInter_H263_16s_C1I",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiQuantInter_H263_16s_C1I(pSrcDst, QP,pCountNonZero,  modQuantFlag);
		return sts;
	}
#endif

	return ippStsNoErr;
}



IppStatus __STDCALL ippiQuantInvIntra_H263_16s_C1I_c(
	Ipp16s* pSrcDst, int     indxLastNonZero,
	int     QP, int     advIntraFlag,int     modQuantFlag)
{
#if 1
	Ipp32s              j;
	Ipp32s				tmpCoeff;
	const int			QP_1 = QP <<1;
	const int			QP_2 = QP & 1 ? QP : (QP - 1);

	if (!pSrcDst )
		return ippStsNullPtrErr;

	if (QP < 1 || QP > 31)
		return ippStsQPErr;

	/* in this Intel sample, 
	the modQuantFlag, advIntraFlag are both set with 0 obviously.
	I think modQuantFlag and advIntraFlag are used in H.263 only, not mpeg4 
	short video header mode*/
#ifdef _DEBUG
	if (modQuantFlag || advIntraFlag)
	{
		printf("modQuantFlag or advIntraFlag is not used in short video header mode of mpeg4\n");
		return ippStsErr;
	}
#endif 

	tmpCoeff = (Ipp32s)pSrcDst[0] << 3;
	pSrcDst[0] = CLIP_R(tmpCoeff, 2047,-2048);

	if (indxLastNonZero < 1 || indxLastNonZero > 63)
		indxLastNonZero = 63;

	for (j = 1; j < indxLastNonZero+1; j++)
	{
		if (pSrcDst[j] < 0) 
		{
			tmpCoeff = pSrcDst[j]*QP_1 - QP_2;
			pSrcDst[j] = (tmpCoeff < -2048 ? -2048 : tmpCoeff );
		} 
		else if (pSrcDst[j] > 0)
		{
			tmpCoeff = pSrcDst[j]*QP_1 + QP_2;
			pSrcDst[j] = (tmpCoeff > 2047 ? 2047 : tmpCoeff );
		}		 /* if pCoeff is 0, we need do Nothing.*/
	}

#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantInvIntra_H263_16s_C1I",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiQuantInvIntra_H263_16s_C1I(pSrcDst, indxLastNonZero,QP, advIntraFlag, modQuantFlag);
		return sts;
	}
#endif

	return ippStsNoErr;
}

IppStatus __STDCALL ippiQuantInvInter_H263_16s_C1I_c(
	Ipp16s* pSrcDst,  int     indxLastNonZero,
	int     QP, int     modQuantFlag)
{
#if 1
	Ipp32s              j;
	Ipp32s				tmpCoeff;
	const int			QP_1 = QP <<1;
	const int			QP_2 = QP & 1 ? QP : (QP - 1);

	if (!pSrcDst )
		return ippStsNullPtrErr;

	if (QP < 1 || QP > 31)
		return ippStsQPErr;

	/* in this Intel sample, 
	the modQuantFlag was set with 0 obviously.
	I think modQuantFlag is used in H.263 only, not mpeg4
	short video header mode*/
#ifdef _DEBUG
	if (modQuantFlag)
	{
		printf("modQuantFlag is not used in short video header mode of mpeg4\n");
		return ippStsErr;
	}
#endif 

	if (indxLastNonZero < 1 || indxLastNonZero > 63)
		indxLastNonZero = 63;

	for (j = 0; j < indxLastNonZero+1; j++)
	{
		if (pSrcDst[j] < 0) 
		{
			tmpCoeff = pSrcDst[j]*QP_1 - QP_2;
			pSrcDst[j] = (tmpCoeff < -2048 ? -2048 : tmpCoeff );
		} else if (pSrcDst[j] > 0)
		{
			tmpCoeff = pSrcDst[j]*QP_1 + QP_2;
			pSrcDst[j] = (tmpCoeff > 2047 ? 2047 : tmpCoeff );
		}		 /* if pCoeff is 0, we need do Nothing.*/
	}
#endif

#ifdef PRINT_REF_INFO
	dump("ippiQuantInvInter_H263_16s_C1I",-1);
#endif

#if 0
	{
		IppStatus sts;
		sts = ippiQuantInvInter_H263_16s_C1I(pSrcDst, indxLastNonZero, QP, modQuantFlag);
		return sts;
	}
#endif

	return ippStsNoErr;
}

