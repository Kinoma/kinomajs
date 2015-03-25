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
#include "ippvc.h"
#include "kinoma_avc_defines.h"
#include "kinoma_ipp_lib.h"
#include "kinoma_utilities.h"

IppStatus  (__STDCALL *ippiTransformDequantLumaDC_H264_16s_C1I_universal)  			(Ipp16s* pSrcDst, Ipp32s  QP)=NULL;
IppStatus  (__STDCALL *ippiTransformDequantChromaDC_H264_16s_C1I_universal) 			(Ipp16s* pSrcDst, Ipp32s  QP)=NULL;
IppStatus  (__STDCALL *ippiDequantTransformResidualAndAdd_H264_16s_C1I_universal)	(const Ipp8u*  pPred,Ipp16s* pSrcDst, const Ipp16s* pDC,Ipp8u*  pDst,Ipp32s  PredStep,Ipp32s  DstStep, Ipp32s  QP,Ipp32s  AC)=NULL;
IppStatus  (__STDCALL *ippiTransformQuantChromaDC_H264_16s_C1I_universal)			(Ipp16s* pSrcDst,Ipp16s* pTBlock, Ipp32s  QPCroma, Ipp8s*  NumLevels, Ipp8u   Intra, Ipp8u   NeedTransform)=NULL;
IppStatus  (__STDCALL *ippiTransformQuantResidual_H264_16s_C1I_universal) 			(Ipp16s* pSrcDst,Ipp32s  QP,Ipp8s*  NumLevels,Ipp8u   Intra,  const Ipp16s* pScanMatrix, Ipp8u*  LastCoeff)=NULL;
IppStatus  (__STDCALL *ippiTransformQuantFwd4x4_H264_16s_C1_universal )				(const Ipp16s *pSrc,Ipp16s *pDst,Ipp32s QP,Ipp32s *NumLevels,Ipp32s Intra,const Ipp16s *pScanMatrix,Ipp32s *LastCoeff,const Ipp16s *pScaleLevels ) = NULL;
IppStatus  (__STDCALL *ippiTransformQuantLumaDC_H264_16s_C1I_universal) 				(Ipp16s* pSrcDst,Ipp16s* pTBlock, Ipp32s  QP,   Ipp8s*  NumLevels,  Ipp8u   NeedTransform,   const Ipp16s* pScanMatrix,  Ipp8u*  LastCoeff)=NULL;

#ifdef __KINOMA_IPP__
// Some const array for DCT
_DECLSPEC static	const unsigned short dequant_coef[6][16] = {
	  {10, 13, 10, 13,   13, 16, 13, 16,   10, 13, 10, 13,   13, 16, 13, 16},
	  {11, 14, 11, 14,   14, 18, 14, 18,   11, 14, 11, 14,   14, 18, 14, 18},
	  {13, 16, 13, 16,   16, 20, 16, 20,   13, 16, 13, 16,   16, 20, 16, 20},
	  {14, 18, 14, 18,   18, 23, 18, 23,   14, 18, 14, 18,   18, 23, 18, 23},
	  {16, 20, 16, 20,   20, 25, 20, 25,   16, 20, 16, 20,   20, 25, 20, 25},
	  {18, 23, 18, 23,   23, 29, 23, 29,   18, 23, 18, 23,   23, 29, 23, 29}
	};
static const int quant_coef[6][16] = {
  {13107, 8066,13107, 8066, 8066, 5243, 8066, 5243,13107, 8066,13107, 8066, 8066, 5243, 8066, 5243},
  {11916, 7490,11916, 7490, 7490, 4660, 7490, 4660,11916, 7490,11916, 7490, 7490, 4660, 7490, 4660},
  {10082, 6554,10082, 6554, 6554, 4194, 6554, 4194,10082, 6554,10082, 6554, 6554, 4194, 6554, 4194},
  { 9362, 5825, 9362, 5825, 5825, 3647, 5825, 3647, 9362, 5825, 9362, 5825, 5825, 3647, 5825, 3647},
  { 8192, 5243, 8192, 5243, 5243, 3355, 5243, 3355, 8192, 5243, 8192, 5243, 5243, 3355, 5243, 3355},
  { 7282, 4559, 7282, 4559, 4559, 2893, 4559, 2893, 7282, 4559, 7282, 4559, 4559, 2893, 4559, 2893}
};

// QP range : luma[0--51], Chroma[0--39]
_DECLSPEC unsigned short qp_per_table[52] = { 
	0, 0, 0, 0, 0, 0, /*0  -- 5 */
	1, 1, 1, 1, 1, 1, /*6  -- 11 */
	2, 2, 2, 2, 2, 2, /*12 -- 17 */
	3, 3, 3, 3, 3, 3, /*18 -- 23 */
	4, 4, 4, 4, 4, 4, /*24 -- 29 */
	5, 5, 5, 5, 5, 5, /*30 -- 35 */
	6, 6, 6, 6, 6, 6, /*36 -- 41 */
	7, 7, 7, 7, 7, 7, /*42 -- 47 */
	8, 8, 8, 8        /*48 -- 51  */
};

_DECLSPEC unsigned short qp_rem_table[52] = {
	0, 1, 2, 3, 4, 5, /*0  -- 5 */
	0, 1, 2, 3, 4, 5, /*6  -- 11 */
	0, 1, 2, 3, 4, 5, /*12 -- 17 */
	0, 1, 2, 3, 4, 5, /*18 -- 23 */
	0, 1, 2, 3, 4, 5, /*24 -- 29 */
	0, 1, 2, 3, 4, 5, /*30 -- 35 */
	0, 1, 2, 3, 4, 5, /*36 -- 41 */
	0, 1, 2, 3, 4, 5, /*42 -- 47 */
	0, 1, 2, 3        /*48 -- 51  */
};


#define Q_BITS			15
#define DQ_BITS         6
#define DQ_ROUND        (1<<(DQ_BITS-1))


//Here are some definations for 4x4 Hadamard transform and 4x4 DCT transform
void HadamardT(Ipp16s* pDst, Ipp16s* pSrc)
{
	int	i;

	// horizontal
    for (i=0; i<16; i+=4)
	{
        int d0 = pSrc[i+0];
        int d1 = pSrc[i+1];
        int d2 = pSrc[i+2];
        int d3 = pSrc[i+3];
        
        int e0 = d0 + d2;
        int e1 = d0 - d2;
        int e2 = d1 - d3;
        int e3 = d1 + d3;

		pDst[i+0] = e0 + e3;
		pDst[i+1] = e1 + e2;
		pDst[i+2] = e1 - e2;
		pDst[i+3] = e0 - e3;
	}

	// vertical
	for (i=0;i<4;i++)
	{
        int f0 = pDst[i+0];
        int f1 = pDst[i+4];
        int f2 = pDst[i+8];
        int f3 = pDst[i+12];
       
		int g0 = f0 + f2;
        int g1 = f0 - f2;
        int g2 = f1 - f3;
        int g3 = f1 + f3;

		pDst[i+0]  = g0 + g3;
		pDst[i+4]  = g1 + g2;
		pDst[i+8]  = g1 - g2;
		pDst[i+12] = g0 - g3;
	}	
}

#if 1
void DCT4x4(Ipp16s* pDst, Ipp16s *pSrc)
{
    int i;

    /* Transform rows */
    for (i=0; i<16; i+=4)
    {
        int d0 = pSrc[i+0];
        int d1 = pSrc[i+1];
        int d2 = pSrc[i+2];
        int d3 = pSrc[i+3];
        int e0 = d0 + d3;
        int e1 = d0 - d3;
        int e2 = d1 + d2;
        int e3 = d1 - d2;
        int f0 = e0 + e2;
        int f1 = (e1 << 1) + e3;
        int f2 = e0 - e2;
        int f3 = e1 - (e3 << 1);
        pDst[i+0] = f0;
        pDst[i+1] = f1;
        pDst[i+2] = f2;
        pDst[i+3] = f3;
    }

    /* Transform columns */
    for (i=0; i<4; i++)
    {
        int f0 = pDst[i+0];
        int f1 = pDst[i+4];
        int f2 = pDst[i+8];
        int f3 = pDst[i+12];
        int g0 = f0 + f3;
        int g1 = f0 - f3;
        int g2 = f1 + f2;
        int g3 = f1 - f2;
        int h0 = g0 + g2;
        int h1 = (g1 << 1) + g3;
        int h2 = g0 - g2;
        int h3 = g1 - (g3 << 1);
        pDst[i+0]  = h0;
        pDst[i+4]  = h1;
        pDst[i+8]  = h2;
        pDst[i+12] = h3;
    }
}
#else

void DCT4x4(Ipp16s* Result, Ipp16s* pSrc)
{
	_DECLSPEC short		m7[16], m5[4];
	int	i,j;

	//  Horizontal transform
	for (j=0; j < 4; j++)
	{
		m5[0] = pSrc[0*4 +j] + pSrc[3*4 +j];
		m5[1] = pSrc[1*4 +j] + pSrc[2*4 +j];
		m5[3] = pSrc[0*4 +j] - pSrc[3*4 +j];
		m5[2] = pSrc[1*4 +j] - pSrc[2*4 +j];

		m7[0*4 +j] = (m5[0]   + m5[1]);
		m7[2*4 +j] = (m5[0]   - m5[1]);
		m7[1*4 +j] =  m5[3]*2 + m5[2];
		m7[3*4 +j] =  m5[3]   - m5[2]*2;
	}

	//  Vertical transform
	for (i=0; i < 4; i++)
	{
		m5[0] = m7[i*4 +0]+m7[i*4 +3];
		m5[1] = m7[i*4 +1]+m7[i*4 +2];
		m5[3] = m7[i*4 +0]-m7[i*4 +3];
		m5[2] = m7[i*4 +1]-m7[i*4 +2];

		Result[i*4 +0] = (m5[0]   + m5[1]);
		Result[i*4 +2] = (m5[0]   - m5[1]);
		Result[i*4 +1] =  m5[3]*2 + m5[2];
		Result[i*4 +3] =  m5[3]   - m5[2]*2;
	}
}
#endif

// Macro END

/* ///////////////////////////////////////////////////////////////////////////
//  Name:
//    ippiTransformDequantLumaDC_H264_16s_C1I_c
//    ippiTransformDequantChromaDC_H264_16s_C1I_c
//
//  Purpose:
//     Perform integer inverse transformation and dequantization
//     for 4x4 luma DC coefficients,
//     and 2x2 chroma DC coefficients respectively.
//
//  Parameters:
//     pSrcDst - pointer to initial coefficients and resultant DC,
//     QP      - quantization parameter.
//
//  Returns:
//    ippStsNoErr          No error
//    ippStsNullPtrErr     pSrcDst is NULL
//    ippStsOutOfRangeErr  QP is less than 1 or greater than 51
*/
//***bnie: arm v6 optimization???
IppStatus  __STDCALL  ippiTransformDequantLumaDC_H264_16s_C1I_c(
  Ipp16s* pSrcDst,
  Ipp32s  QP)
{
	_DECLSPEC short		dcResult[16];

	int		qp_per = qp_per_table[QP];
	int		qp_rem = qp_rem_table[QP];
	int		j;
	short   dequant = dequant_coef[qp_rem][0];

	// First Hadamard transform
	HadamardT(dcResult, pSrcDst);

	for (j=0;j< 16;j+=4)
	{
		pSrcDst[j+0] = ((((dcResult[j+0] *dequant) << qp_per) + 2) >> 2);
		pSrcDst[j+1] = ((((dcResult[j+1] *dequant) << qp_per) + 2) >> 2);
		pSrcDst[j+2] = ((((dcResult[j+2] *dequant) << qp_per) + 2) >> 2);
		pSrcDst[j+3] = ((((dcResult[j+3] *dequant) << qp_per) + 2) >> 2);
	}

	return ippStsNoErr;
}

//***bnie: arm v6 optimization???
IppStatus  __STDCALL ippiTransformDequantChromaDC_H264_16s_C1I_c(
  Ipp16s* pSrcDst,
  Ipp32s  QP)
{
	_DECLSPEC short		dcResultU[4];
	int		qp_per_uv = qp_per_table[QP];
	int		qp_rem_uv = qp_rem_table[QP];

	int		i;


	dcResultU[0] = (pSrcDst[0] + pSrcDst[1] + pSrcDst[2] + pSrcDst[3]);
	dcResultU[1] = (pSrcDst[0] - pSrcDst[1] + pSrcDst[2] - pSrcDst[3]);
	dcResultU[2] = (pSrcDst[0] + pSrcDst[1] - pSrcDst[2] - pSrcDst[3]);
	dcResultU[3] = (pSrcDst[0] - pSrcDst[1] - pSrcDst[2] + pSrcDst[3]);


	if(QP >= 6)
	{
		for (i=0;i<4;i++)
			pSrcDst[i] = (dcResultU[i]) *dequant_coef[qp_rem_uv][0]<< (qp_per_uv - 1);
	}
	else
	{
		for (i=0;i<4;i++)
			pSrcDst[i] = (dcResultU[i]) *dequant_coef[qp_rem_uv][0]>> 1;
	}

	return ippStsNoErr;
}

#if 0
IppStatus  __STDCALL  ippiDequantTransformResidualAndAdd_H264_16s_C1I_c(
    const Ipp8u*  pPred,
          Ipp16s* pSrcDst,
    const Ipp16s* pDC,
          Ipp8u*  pDst,
          Ipp32s  PredStep,
          Ipp32s  DstStep,
          Ipp32s  QP,
          Ipp32s  AC)
{
	_DECLSPEC int		m7[4][4], m6[4];
	short *pSrctmp, acArray[16];

	unsigned char *pDsttmp;

	int		i,j,k;

	int		qp_per, qp_rem;

 
	qp_per = qp_per_table[QP];
	qp_rem = qp_rem_table[QP];


	pSrctmp = pSrcDst;	/*Only have 16 elements!!  */

	i = 0;
	if(pDC != NULL)
	{
		// Need DC coeff
		acArray[0] = *pDC;			// Add DC coeff in scan order
		i = 1;
	}

	if(AC != 0)//added by WWD in 20090602 
		// Form acArray and do IQ
		for(; i<16; i++)
		{
			acArray[i] = (pSrcDst[i] * dequant_coef[qp_rem][i]) << (qp_per);
		}
	else
		for(; i<16; i++)
		{
			acArray[i] = 0;// Initialize to ZERo for IDCT
		}

	// Here we should use AC to simplify caculation -- if AC==0, do not need IDCT
	// AC != 0 , need IDCT
	pDsttmp = pDst;
	// Do Inverse transform
	{
			// horizontal
			for (j=0;j<4;j++)
			{
				m6[0] = (acArray[0+j*4]     + acArray[2+j*4]);
				m6[1] = (acArray[0+j*4]     - acArray[2+j*4]);
				m6[2] = (acArray[1+j*4]>>1) - acArray[3+j*4];
				m6[3] = acArray[1+j*4]      + (acArray[3+j*4]>>1);

				m7[j][0]  = m6[0] + m6[3];
				m7[j][3]  = m6[0] - m6[3];
				m7[j][1]  = m6[1] + m6[2];
				m7[j][2]  = m6[1] - m6[2];
			}

			// vertical
			for (i=0;i<4;i++)
			{
				m6[0]=(m7[0][i] + m7[2][i]);
				m6[1]=(m7[0][i] - m7[2][i]);
				m6[2]=(m7[1][i]>>1)-m7[3][i];
				m6[3]=m7[1][i] +(m7[3][i]>>1);
			
				m7[0][i]  = m6[0]+m6[3];
				m7[3][i]  = m6[0]-m6[3];
				m7[1][i]  = m6[1]+m6[2];
				m7[2][i]  = m6[1]-m6[2];
			}

			// MUST SAVE this result to array for next prediction
			for(i=0, k=0, j=0; j<4; j++)
			{
				pDsttmp[i + 0] = IClip(0, 255, (((pPred[k +0] << DQ_BITS) + m7[j][0] + DQ_ROUND ) >> DQ_BITS));
				pDsttmp[i + 1] = IClip(0, 255, (((pPred[k +1] << DQ_BITS) + m7[j][1] + DQ_ROUND ) >> DQ_BITS));
				pDsttmp[i + 2] = IClip(0, 255, (((pPred[k +2] << DQ_BITS) + m7[j][2] + DQ_ROUND ) >> DQ_BITS));
				pDsttmp[i + 3] = IClip(0, 255, (((pPred[k +3] << DQ_BITS) + m7[j][3] + DQ_ROUND ) >> DQ_BITS));

				i += DstStep;
				k += PredStep;
			}
	}
	
	return ippStsNoErr;
}

#endif

IppStatus  __STDCALL  ippiDequantTransformResidualAndAdd_H264_16s_C1I_NO_AC_c(
   const Ipp8u*  pPred,
         Ipp16s* pSrcDst,
   const Ipp16s* pDC,
         Ipp8u*  pDst,
         Ipp32s  PredStep,
         Ipp32s  DstStep,
         Ipp32s  QP)
{
#define RECON(x, y)		(unsigned char)(IClip(0, 255, ((( (x) << DQ_BITS) + DQ_ROUND + (y) ) >> DQ_BITS)))
	int             i,j,k;

	if(pDC != NULL)
		pSrcDst[0] = *pDC;                      // Add DC coeff in scan order
	else
	{
		int qp_per = qp_per_table[QP];
		int qp_rem = qp_rem_table[QP];
		pSrcDst[0] = (short)((pSrcDst[0] * dequant_coef[qp_rem][0]) << (qp_per));
	}

	// MUST SAVE this result to array for next prediction
	for(i=0, k=0, j=0; j<4; j++)
	{
		pDst[i + 0] = RECON(pPred[k +0], pSrcDst[0]);
		pDst[i + 1] = RECON(pPred[k +1], pSrcDst[0]);
		pDst[i + 2] = RECON(pPred[k +2], pSrcDst[0]);
		pDst[i + 3] = RECON(pPred[k +3], pSrcDst[0]);

		i += DstStep;
		k += PredStep;
	}

	return ippStsNoErr;
}


IppStatus  __STDCALL  ippiDequantTransformResidualAndAdd_H264_16s_C1I_c(
   const Ipp8u*  pPred,
         Ipp16s* pSrcDst,
   const Ipp16s* pDC,
         Ipp8u*  pDst,
         Ipp32s  PredStep,
         Ipp32s  DstStep,
         Ipp32s  QP,
         Ipp32s  AC)
{
#define RECON(x, y)		(unsigned char)(IClip(0, 255, ((( (x) << DQ_BITS) + (y) + DQ_ROUND ) >> DQ_BITS)))

	DECL_ALIGN_16 int   block[16];
	//_DECLSPEC int   block[16];
	short *pSrctmp, acArray[16];
	unsigned char *pDsttmp;

	int             i,j,k;
	int             qp_per, qp_rem;

	qp_per = qp_per_table[QP];
	qp_rem = qp_rem_table[QP];


	pSrctmp = pSrcDst;      /*Only have 16 elements!!  */
	pDsttmp = pDst;

	i = 0;
	// DC coeffs
	if(pDC != NULL)
		acArray[0] = *pDC;                      // Add DC coeff in scan order
	else
		acArray[0] = (short)((pSrcDst[0] * dequant_coef[qp_rem][0]) << (qp_per));

	// AC coeffs
	i = 1;
	if(AC == 0)		// Add by WWD in 20090602
	{
		// MUST SAVE this result to array for next prediction
		for(i=0, k=0, j=0; j<4; j++)
		{
			pDsttmp[i + 0] = RECON(pPred[k +0], acArray[0]);
			pDsttmp[i + 1] = RECON(pPred[k +1], acArray[0]);
			pDsttmp[i + 2] = RECON(pPred[k +2], acArray[0]);
			pDsttmp[i + 3] = RECON(pPred[k +3], acArray[0]);

			i += DstStep;
			k += PredStep;
		}

		return ippStsNoErr;
	}

	for(; i<16; i++)
		acArray[i] = (short)((pSrcDst[i] * dequant_coef[qp_rem][i]) << (qp_per));

	// Do Inverse transform
	// horizontal
	for(i=0; i< 16; i+=4)
	{
		const int z0=  acArray[0 + i]     +  acArray[2 + i];
		const int z1=  acArray[0 + i]     -  acArray[2 + i];
		const int z2= (acArray[1 + i]>>1) -  acArray[3 + i];
		const int z3=  acArray[1 + i]     + (acArray[3 + i]>>1);

		block[0 + i]= z0 + z3;
		block[1 + i]= z1 + z2;
		block[2 + i]= z1 - z2;
		block[3 + i]= z0 - z3;
	}

	for(i=0; i<4; i++)
	{
		const int z0=  block[i + 0]     +  block[i + 8];
		const int z1=  block[i + 0]     -  block[i + 8];
		const int z2= (block[i + 4]>>1) - block[i + 12];
		const int z3=  block[i + 4]     + (block[i + 12]>>1);


		block[i + 0]=  (z0 + z3);
		block[i + 4]=  (z1 + z2);
		block[i + 8]=  (z1 - z2);
		block[i + 12]=  (z0 - z3);
	}

	// MUST SAVE this result to array for next prediction
	for(i=0, k=0, j=0; j< 16; j+=4)
	{
		pDsttmp[i + 0] = RECON(pPred[k +0], block[j+0]);
		pDsttmp[i + 1] = RECON(pPred[k +1], block[j+1]);
		pDsttmp[i + 2] = RECON(pPred[k +2], block[j+2]);
		pDsttmp[i + 3] = RECON(pPred[k +3], block[j+3]);

		i += DstStep;
		k += PredStep;
	}

	return ippStsNoErr;
}



#if 0	//verify test code
IppStatus  __STDCALL  ippiDequantTransformResidualAndAdd_H264_16s_C1I_c(
    const Ipp8u*  pPred,
          Ipp16s* pSrcDst,
    const Ipp16s* pDC,
          Ipp8u*  pDst,
          Ipp32s  PredStep,
          Ipp32s  DstStep,
          Ipp32s  QP,
          Ipp32s  AC)
{
	Ipp16s pSrcDst1[16],pSrcDst2[16];
	Ipp8u  	pDst1[16], pDst2[16];
	int i,j;
	static int count = 0;

	count++;
	if( count == 705 )
	{
		int a = 1;
	}

	for( i = 0; i < 16; i++ )
	{
		pSrcDst1[i] = pSrcDst[i];
		pSrcDst2[i] = pSrcDst[i];
		pDst1[i ] = 0;
		pDst2[i ] = 0;
	}

	ippiDequantTransformResidualAndAdd_H264_16s_C1I_c_optm
		(
			pPred,
			pSrcDst1,
			pDC,
			pDst1,
			PredStep,
			4,
			QP,
			AC
		);

	ippiDequantTransformResidualAndAdd_H264_16s_C1I
		(
			pPred,
			pSrcDst2,
			pDC,
			pDst2,
			PredStep,
			4,
			QP,
			AC
		);

	for( i = 0; i < 16; i++ )
	{
		if( pDst1[i] != pDst2[i] )
		{
			fprintf( stderr, "error found!\n");
		}
	}

	for( i = 0; i < 16; i++ )
	{
		//pSrcDst[i] = pSrcDst1[i];
	}

	for( j = 0; j < 4; j++ )
	for( i = 0; i < 4; i++ )
	{
		pDst[j*DstStep+i]    = pDst2[j*4+i];
	}

 	return ippStsNoErr;
}


#endif

IppStatus  __STDCALL  ippiTransformQuantChromaDC_H264_16s_C1I_c(
        Ipp16s* pSrcDst,
        Ipp16s* pTBlock,
        Ipp32s  QPCroma,
        Ipp8s*  NumLevels,
        Ipp8u   Intra,
        Ipp8u   NeedTransform
)
{

	int		qp_per_uv, qp_rem_uv, q_bits, qp_const;
	int		nonZero_count;
	int		i;

	int		level;
	int		needSign = 0;

	/*NeedTransform always == 1  in this reference code */
	qp_per_uv = qp_per_table[QPCroma];
	qp_rem_uv = qp_rem_table[QPCroma];
	q_bits = Q_BITS + qp_per_uv;

	if (Intra)
		qp_const=(1<<q_bits)/3;    // intra
	else
		qp_const=(1<<q_bits)/6;    // inter

	if(NeedTransform)
	{
		//     2X2 transform of DC coeffs.
		pTBlock[0]=(pSrcDst[0]+ pSrcDst[1]+ pSrcDst[2]+ pSrcDst[3]);
		pTBlock[1]=(pSrcDst[0]- pSrcDst[1]+ pSrcDst[2]- pSrcDst[3]);
		pTBlock[2]=(pSrcDst[0]+ pSrcDst[1]- pSrcDst[2]- pSrcDst[3]);
		pTBlock[3]=(pSrcDst[0]- pSrcDst[1]- pSrcDst[2]+ pSrcDst[3]);

	}

	nonZero_count = 0;

	for (i=0; i < 4; i++)
	{
		level =(absm(pTBlock[i]) * quant_coef[qp_rem_uv][0] + 2*qp_const) >> (q_bits+1);

		if(level)
			nonZero_count ++;
		else
			if(i == 0)
				needSign = 1;

		if(pTBlock[i]<0)
			pSrcDst[i] = -level;
		else
			pSrcDst[i] = level;
	}

	if(!needSign)
		nonZero_count = -nonZero_count;

	*NumLevels = nonZero_count;

	return ippStsNoErr;
}

// Note: must use new scan_order
IppStatus  __STDCALL   ippiTransformQuantResidual_H264_16s_C1I_c(
        Ipp16s* pSrcDst,
        Ipp32s  QP,
        Ipp8s*  NumLevels ,
        Ipp8u   Intra,
        const Ipp16s* pScanMatrix,
        Ipp8u*  LastCoeff)

{
	short int		m7[16];
	int		qp_per, qp_rem, q_bits, qp_const;
	int		nonZero_count;
	int		i, j;

	int		level;
	int		needSign = 0;


	qp_per    = qp_per_table[QP];
	qp_rem    = qp_rem_table[QP];
	q_bits    = Q_BITS+qp_per;

	if (Intra)
		qp_const=(1<<q_bits)/3;    // intra
	else
		qp_const=(1<<q_bits)/6;    // inter

	// DCT: Using pSrcDst
	DCT4x4(m7, pSrcDst);

	// Q and Scan
	nonZero_count = 0;
	*LastCoeff = 0;
	for (i=0;i < 16;i++)
	{   
		j = pScanMatrix[i];
	    
		level = (absm (m7[j]) * quant_coef[qp_rem][j] + qp_const) >> q_bits;

		if((level != 0) &&(i==0))
			needSign =1;

		if(level)
		{
			nonZero_count ++;
			*LastCoeff = i;
		}


		pSrcDst[j] = (m7[j]<0)? -level:level;
	}

	// Caculate sign for NumLevels
	if(needSign)
		*NumLevels = -nonZero_count;
	else
		*NumLevels = nonZero_count;

	return ippStsNoErr;
}

#include "omxtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

void armVCM4P10_FwdTransformResidual4x4(OMX_S16* pDst, OMX_S16 *pSrc);

#ifdef __cplusplus
}
#endif

IppStatus  __STDCALL ippiTransformQuantFwd4x4_H264_16s_C1_c
(
	 const Ipp16s *pSrc,
		   Ipp16s *pDst,
		   Ipp32s  QP,
		   Ipp32s *NumLevels,
		   Ipp32s  Intra,
	 const Ipp16s *pScanMatrix,
		   Ipp32s *LastCoeff,
	 const Ipp16s *pScaleLevels
 )
{
	short int		m7[16];
	int		qp_per, qp_rem, q_bits, qp_const;
	int		nonZero_count;
	int		i, j;

	int		level;
	int		needSign = 0;

	qp_per    = qp_per_table[QP];
	qp_rem    = qp_rem_table[QP];
	q_bits    = Q_BITS+qp_per;

	if (Intra)
		qp_const=(1<<q_bits)/3;    // intra
	else
		qp_const=(1<<q_bits)/6;    // inter

	// DCT: Using pSrcDst
	//***bnie
	//DCT4x4(m7, (Ipp16s *)pSrc);
	armVCM4P10_FwdTransformResidual4x4((OMX_S16 *)m7, (OMX_S16 *)pSrc);

	// Q and Scan
	nonZero_count = 0;
	*LastCoeff = 0;
	for (i=0;i < 16;i++)
	{   
		j = pScanMatrix[i];
	    
		level = (absm (m7[j]) * quant_coef[qp_rem][j] + qp_const) >> q_bits;

		if((level != 0) &&(i==0))
			needSign =1;

		if(level)
		{
			nonZero_count ++;
			//*LastCoeff = i;	//***bnie_fix
			if( *LastCoeff < pScanMatrix[j] )
				*LastCoeff = pScanMatrix[j];
		}

		pDst[j] = (m7[j]<0)? -level:level;
		//if( *LastCoeff < pScanMatrix[j] )
		//	*LastCoeff = pScanMatrix[j];
	}

	// Caculate sign for NumLevels
	if(needSign)
		*NumLevels = -nonZero_count;
	else
		*NumLevels = nonZero_count;

	return ippStsNoErr;
}


// Do 4x4 Hadamard Transform
IppStatus __STDCALL ippiTransformQuantLumaDC_H264_16s_C1I_c(
        Ipp16s* pSrcDst,
        Ipp16s* pTBlock,
        Ipp32s  QP,
        Ipp8s*  NumLevels,
        Ipp8u   NeedTransform,
        const Ipp16s* pScanMatrix,
        Ipp8u*  LastCoeff)
{
	int		M5[4], M4[16];

	int		qp_per, qp_rem, q_bits, qp_const;
	int		nonZero_count;
	int		i, j;

	int		level;
	int		needSign = 0;

	qp_per    = qp_per_table[QP];
	qp_rem    = qp_rem_table[QP];
	q_bits    = Q_BITS+qp_per ;

	qp_const= ((1<<q_bits)/3) << 1;    // intra

	q_bits += 1;

	if(NeedTransform)
	{
		//HadamardT(pTBlock, pSrcDst);
		for (i=0;i<4;i++)
		{
			int f0 = pSrcDst[i+0];
			int f1 = pSrcDst[i+4];
			int f2 = pSrcDst[i+8];
			int f3 = pSrcDst[i+12];
			
			int g0 = f0 + f3;
			int g1 = f1 + f2;
			int g2 = f1 - f2;
			int g3 = f0 - f3;

			M4[0*4 +i] = g0 + g1;
			M4[1*4 +i] = g3 + g2;
			M4[3*4 +i] = g3 - g2;
			M4[2*4 +i] = g0 - g1;
		}

		// vertical
		for (i=0;i<4;i++)
		{
			M5[0] = M4[i*4 +0] + M4[i*4 +3];
			M5[3] = M4[i*4 +0] - M4[i*4 +3];
			M5[1] = M4[i*4 +1] + M4[i*4 +2];
			M5[2] = M4[i*4 +1] - M4[i*4 +2];

			pTBlock[i*4 +0] = (M5[0] + M5[1])>>1;
			pTBlock[i*4 +2] = (M5[0] - M5[1])>>1;
			pTBlock[i*4 +1] = (M5[3] + M5[2])>>1;
			pTBlock[i*4 +3] = (M5[3] - M5[2])>>1;
		}
	}

	// Q and Scan
	*LastCoeff = 0;
	nonZero_count = 0;
	for (i=0;i < 16;i++)
	{   
		j = pScanMatrix[i];
	    
		level = (absm(pTBlock[j]) * quant_coef[qp_rem][0] + qp_const)>>q_bits;

		if((level != 0) &&(i==0))
			needSign =1;

		if(level)
		{
			// MUST be add
			if(level > MAX_CAVLC_LEVEL_VALUE)
				return ippStsScaleRangeErr;

			nonZero_count ++;
			//*LastCoeff = i;		bnie_fix
		}

		pSrcDst[j] = (pTBlock[j]<0)? -level:level;
		//***bnie_fix
		if( pSrcDst[j] != 0 && pScanMatrix[j] > *LastCoeff )
			*LastCoeff = pScanMatrix[j];
	}

	// Caculate sign for NumLevels
	if(needSign)
		*NumLevels = -nonZero_count;
	else
		*NumLevels = nonZero_count;

	return ippStsNoErr;
}

#endif
